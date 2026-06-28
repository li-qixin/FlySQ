#include "bldc.h"

#include <board.h>
#include <as5600/as5600.h>
#include <motor/park.h>
#include <motor/svpwm.h>
#include <tarox_gpio.h>
#include <tarox_pwm.h>

#include <errno.h>
#include <stdarg.h>
#include <math.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define BLDC_CTRL_HZ           1000U
#define BLDC_CTRL_PERIOD_US    (1000000U / BLDC_CTRL_HZ)
#define BLDC_LOG_PERIOD_S      0.05f
#define BLDC_TWO_PI            6.283185307179586f
#define BLDC_DT_MAX_S          0.5f
#define BLDC_VF_REF_HZ              20.0f
#define BLDC_SPEED_MEAS_PERIOD_S    0.03f
#define BLDC_SPEED_FILT_ALPHA       0.20f
#define BLDC_SPEED_NOISE_COUNTS     6

/* Closed-loop speed control: Vq = feed-forward(V/f) + Kp*err + Ki*∫err, where
 * err is in mechanical Hz (target - encoder-measured). The feed-forward gives
 * fast startup torque; the PI trims out load/friction and steady-state error.
 * The integral is clamped (anti-windup) to the Vq limit, and frozen while the
 * output is saturated so it cannot wind up when the target is unreachable.
 */

#define BLDC_SPEED_PI_KP            0.18f
#define BLDC_SPEED_PI_KI            1.20f

/* Commutation lag compensation. The encoder angle used for FOC is ~one control
 * cycle old; at high speed that delay becomes an electrical-angle error that
 * kills torque. Advance the applied angle by (factor * dt) of electrical
 * rotation to cancel it. This is ONLY engaged above a speed threshold and hard
 * clamped: at low speed the (noisy) measured speed and the jittery, I2C-paced
 * loop dt would otherwise inject a large, fluctuating angle error and wreck
 * torque, so low speed commutates straight from the encoder like position mode.
 */

#define BLDC_COMMUT_ADVANCE         1.0f
#define BLDC_COMMUT_ADV_MIN_HZ      1.5f   /* mech Hz before advance engages */
#define BLDC_COMMUT_ADV_MAX_RAD     0.35f  /* hard clamp (~20 deg elec) */
#define BLDC_COMMUT_ADV_DT_MAX_S    0.002f /* clamp dt used for advance */
/* Position: LPF error + PD -> Vq.  Single loop; no speed cascade (AS5600
 * count-delta speed is too noisy and was the root cause of limit cycles).
 */

#define BLDC_POS_ERR_FILT_ALPHA     0.20f
#define BLDC_POS_VEL_FILT_ALPHA     0.18f
#define BLDC_POS_ERR_FULL_RAD       (12.0f * BLDC_PI / 180.0f)
#define BLDC_POS_DEADBAND_RAD       (0.40f * BLDC_PI / 180.0f)
#define BLDC_POS_VEL_ZERO_HZ        0.02f
#define BLDC_POS_KP                 1.00f   /* vq_max at |err|=ERR_FULL */
#define BLDC_POS_KD                 0.40f   /* damping vs filtered vel */
#define BLDC_POS_VEL_REF_HZ         0.50f   /* ~30 rpm Kd reference */
#define BLDC_POS_SPD_FILT_ALPHA     0.08f   /* telemetry only */
#define BLDC_POS_HOLD_SPD_HZ        0.02f
#define BLDC_ALIGN_MS               600U
#define BLDC_ALIGN_VQ_PU            0.15f
#define BLDC_AUTO_ALIGN_VQ_PU       0.20f
#define BLDC_ALIGN_STEP_RAD         2.094395102f
#define BLDC_PI                     3.141592653589793f

static int          g_pwm_fd = TAROX_PWM_FD_INVALID;
static int          g_en_fd = TAROX_GPIO_FD_INVALID;
static pid_t        g_keeper_pid = (pid_t)-1;
static bool         g_running;
static bldc_mode_e  g_mode = BLDC_MODE_NONE;
static float        g_elec_hz;
static float        g_vq_max_pu;
static float        g_theta;
static float        g_vq_cmd;
static float        g_vq_show_pu;
static float        g_target_mech_rad;
static int          g_dir = 1;
static uint16_t     g_enc_raw;
static uint16_t     g_prev_enc_raw;
static float        g_meas_mech_hz;
static float        g_target_mech_hz;
static float        g_err_filt_rad;
static float        g_err_prev_filt_rad;
static float        g_vel_err_hz;
static bool         g_pos_filter_reset;
static float        g_speed_pi_i;
static float        g_spd_filt_hz;
static float        g_prev_meas_hz;
static float        g_speed_meas_dt;
static bool         g_speed_sample_fresh;
static float        g_log_period_s = BLDC_LOG_PERIOD_S;
static struct timespec g_log_last_ts;
static bool         g_log_last_valid;
static int32_t      g_speed_delta_accum;
static float        g_speed_dt_accum;
static float        g_elec_offset_rad = BOARD_AS5600_ELEC_OFFSET_RAD;
static int          g_vq_sign = BOARD_BLDC_VQ_SIGN;
static int          g_enc_dir = 1;
static bool         g_foc_calibrated;
static volatile bool g_busy;

static float timespec_delta_s(const struct timespec *start,
                              const struct timespec *end)
{
  return (float)(end->tv_sec - start->tv_sec)
       + (float)(end->tv_nsec - start->tv_nsec) * 1e-9f;
}

static bool bldc_log_ready(void)
{
  struct timespec now;

  clock_gettime(CLOCK_MONOTONIC, &now);
  if (!g_log_last_valid)
    {
      g_log_last_ts = now;
      g_log_last_valid = true;
      return true;
    }

  if (timespec_delta_s(&g_log_last_ts, &now) < g_log_period_s)
    {
      return false;
    }

  g_log_last_ts = now;
  return true;
}

static int bldc_log_printf(const char *fmt, ...)
{
  va_list ap;
  int ret;

  if (!bldc_log_ready())
    {
      return 0;
    }

  va_start(ap, fmt);
  ret = vprintf(fmt, ap);
  va_end(ap);
  return ret;
}

static float bldc_wrap_theta_val(float theta)
{
  while (theta >= BLDC_TWO_PI)
    {
      theta -= BLDC_TWO_PI;
    }

  while (theta < 0.0f)
    {
      theta += BLDC_TWO_PI;
    }

  return theta;
}

static float bldc_mech_angle_error(float target, float current)
{
  float err = target - current;

  if (err > BLDC_PI)
    {
      err -= BLDC_TWO_PI;
    }
  else if (err < -BLDC_PI)
    {
      err += BLDC_TWO_PI;
    }

  return err;
}

static float bldc_elec_angle(uint16_t raw)
{
  float e = (float)g_enc_dir * (float)BOARD_BLDC_POLE_PAIRS
          * as5600_raw_to_mech_rad(raw) + g_elec_offset_rad;
  return bldc_wrap_theta_val(e);
}

/* Finite-difference mechanical speed (mech Hz): accumulate encoder counts over
 * a measurement window, then low-pass filter.
 */

static float bldc_meas_mech_hz(uint16_t raw, float dt)
{
  int32_t delta;
  float inst_hz;

  delta = (int32_t)raw - (int32_t)g_prev_enc_raw;
  if (delta > 2048)
    {
      delta -= 4096;
    }
  else if (delta < -2048)
    {
      delta += 4096;
    }

  g_prev_enc_raw = raw;

  if (dt <= 0.0f)
    {
      return g_meas_mech_hz;
    }

  g_speed_delta_accum += delta;
  g_speed_dt_accum += dt;

  if (g_speed_dt_accum < BLDC_SPEED_MEAS_PERIOD_S)
    {
      return g_meas_mech_hz;
    }

  if (g_speed_delta_accum > -BLDC_SPEED_NOISE_COUNTS
   && g_speed_delta_accum < BLDC_SPEED_NOISE_COUNTS)
    {
      inst_hz = 0.0f;
    }
  else
    {
      inst_hz = ((float)g_speed_delta_accum / (float)AS5600_RAW_MAX)
              / g_speed_dt_accum;
    }

  g_speed_meas_dt = g_speed_dt_accum;
  g_speed_sample_fresh = true;
  g_speed_delta_accum = 0;
  g_speed_dt_accum = 0.0f;

  g_meas_mech_hz = BLDC_SPEED_FILT_ALPHA * inst_hz
                 + (1.0f - BLDC_SPEED_FILT_ALPHA) * g_meas_mech_hz;

  g_spd_filt_hz = BLDC_POS_SPD_FILT_ALPHA * g_meas_mech_hz
                + (1.0f - BLDC_POS_SPD_FILT_ALPHA) * g_spd_filt_hz;
  return g_meas_mech_hz;
}

static float bldc_effective_vq(float elec_hz, float vq_pu)
{
  float abs_hz = fabsf(elec_hz);
  float scaled = vq_pu * (abs_hz / BLDC_VF_REF_HZ);
  float vmin = vq_pu * 0.12f;

  if (scaled > vq_pu)
    {
      scaled = vq_pu;
    }

  if (abs_hz > 0.05f && scaled < vmin)
    {
      scaled = vmin;
    }

  return scaled;
}

/* Single-loop position PD on filtered error (target constant between goto). */

static float bldc_position_pd(float err_mech_rad, float dt)
{
  float abs_err;
  float inst_vel;
  float vq_p;
  float vq_d;
  float vq;
  float vq_lim;

  if (g_pos_filter_reset)
    {
      g_err_filt_rad = err_mech_rad;
      g_err_prev_filt_rad = err_mech_rad;
      g_vel_err_hz = 0.0f;
      g_pos_filter_reset = false;
    }

  if (dt <= 0.0f)
    {
      dt = 1.0f / (float)BLDC_CTRL_HZ;
    }

  g_err_filt_rad += BLDC_POS_ERR_FILT_ALPHA * (err_mech_rad - g_err_filt_rad);
  inst_vel = -(g_err_filt_rad - g_err_prev_filt_rad) / dt;
  g_vel_err_hz += BLDC_POS_VEL_FILT_ALPHA * (inst_vel - g_vel_err_hz);
  g_err_prev_filt_rad = g_err_filt_rad;

  abs_err = fabsf(g_err_filt_rad);

  if (abs_err < BLDC_POS_DEADBAND_RAD
      && fabsf(g_vel_err_hz) < BLDC_POS_VEL_ZERO_HZ)
    {
      g_target_mech_hz = 0.0f;
      return 0.0f;
    }

  vq_p = BLDC_POS_KP * (g_err_filt_rad / BLDC_POS_ERR_FULL_RAD) * g_vq_max_pu;
  vq_d = BLDC_POS_KD * (g_vel_err_hz / BLDC_POS_VEL_REF_HZ) * g_vq_max_pu;
  vq = vq_p - vq_d;

  vq_lim = g_vq_max_pu * sqrtf(fminf(abs_err / BLDC_POS_ERR_FULL_RAD, 1.0f));
  if (vq > vq_lim)
    {
      vq = vq_lim;
    }
  else if (vq < -vq_lim)
    {
      vq = -vq_lim;
    }

  if (vq > g_vq_max_pu)
    {
      vq = g_vq_max_pu;
    }
  else if (vq < -g_vq_max_pu)
    {
      vq = -g_vq_max_pu;
    }

  g_target_mech_hz = copysignf(vq_lim, g_err_filt_rad);
  return vq;
}

static void bldc_position_reset(uint16_t raw)
{
  g_prev_enc_raw = raw;
  g_meas_mech_hz = 0.0f;
  g_target_mech_hz = 0.0f;
  g_err_filt_rad = 0.0f;
  g_err_prev_filt_rad = 0.0f;
  g_vel_err_hz = 0.0f;
  g_pos_filter_reset = true;
  g_spd_filt_hz = 0.0f;
  g_prev_meas_hz = 0.0f;
  g_speed_meas_dt = 0.0f;
  g_speed_sample_fresh = false;
  g_speed_delta_accum = 0;
  g_speed_dt_accum = 0.0f;
}

static int bldc_driver_enable(bool on)
{
  int ret;

  if (g_en_fd < 0)
    {
      g_en_fd = tarox_gpio_open(TAROX_GPIO_BLDC_EN);
      if (g_en_fd < 0)
        {
          return g_en_fd;
        }
    }

  ret = tarox_gpio_write(g_en_fd, on);
  if (ret < 0 && !on)
    {
      tarox_gpio_close(g_en_fd);
      g_en_fd = TAROX_GPIO_FD_INVALID;
    }

  return ret;
}

static void bldc_driver_disable(void)
{
  if (g_en_fd >= 0)
    {
      tarox_gpio_write(g_en_fd, false);
      tarox_gpio_close(g_en_fd);
      g_en_fd = TAROX_GPIO_FD_INVALID;
    }
}

static int bldc_encoder_update(float dt)
{
  uint16_t raw;
  int ret;

  ret = as5600_read_raw(&raw);
  if (ret < 0)
    {
      return ret;
    }

  bldc_meas_mech_hz(raw, dt);
  g_enc_raw = raw;
  return 0;
}

static void bldc_position_control_update(float dt)
{
  float mech_rad = as5600_raw_to_mech_rad(g_enc_raw);
  float pos_err = bldc_mech_angle_error(g_target_mech_rad, mech_rad);
  float vq;

  /* Filtered-error PD -> Vq (single loop). */

  vq = bldc_position_pd(pos_err, dt);

  g_vq_cmd = (float)g_vq_sign * vq;
  g_vq_show_pu = g_vq_cmd;

  g_theta = bldc_elec_angle(g_enc_raw);
}

static float bldc_speed_pi(float target_hz, float dt)
{
  float err = target_hz - g_meas_mech_hz;
  float ff;
  float vq;
  bool saturated;

  /* Feed-forward: open-loop V/f voltage estimate, signed by the target. */

  ff = copysignf(bldc_effective_vq(g_elec_hz, g_vq_max_pu), target_hz);

  /* Unsaturated command before adding fresh integral, to detect saturation. */

  vq = ff + BLDC_SPEED_PI_KP * err + g_speed_pi_i;
  saturated = (vq >= g_vq_max_pu && err > 0.0f)
           || (vq <= -g_vq_max_pu && err < 0.0f);

  /* Freeze the integral while saturated and still pushing the same way, so an
   * unreachable target can't wind it up. Otherwise accumulate (clamped).
   */

  if (!saturated)
    {
      g_speed_pi_i += BLDC_SPEED_PI_KI * err * dt;
      if (g_speed_pi_i > g_vq_max_pu)
        {
          g_speed_pi_i = g_vq_max_pu;
        }
      else if (g_speed_pi_i < -g_vq_max_pu)
        {
          g_speed_pi_i = -g_vq_max_pu;
        }
    }

  vq = ff + BLDC_SPEED_PI_KP * err + g_speed_pi_i;

  if (vq > g_vq_max_pu)
    {
      vq = g_vq_max_pu;
    }
  else if (vq < -g_vq_max_pu)
    {
      vq = -g_vq_max_pu;
    }

  return vq;
}

static void bldc_speed_control_update(float dt)
{
  /* Target mechanical speed (Hz) in the encoder frame; g_vq_sign maps positive
   * Vq to "encoder angle increasing", same convention as the position loop.
   */

  float target_hz = (float)g_dir * g_elec_hz / (float)BOARD_BLDC_POLE_PAIRS;
  float vq = bldc_speed_pi(target_hz, dt);
  float adv = 0.0f;

  g_target_mech_hz = target_hz;
  g_vq_cmd = (float)g_vq_sign * vq;
  g_vq_show_pu = g_vq_cmd;

  /* Commutate straight from the encoder. Only at higher speed add a small,
   * clamped lag-compensation advance (d(theta_e)/dt = enc_dir*pole*2*pi*mech_hz),
   * using a clamped dt so a slow/jittery loop can't over-advance.
   */

  if (fabsf(g_meas_mech_hz) > BLDC_COMMUT_ADV_MIN_HZ)
    {
      float adv_dt = (dt < BLDC_COMMUT_ADV_DT_MAX_S)
                   ? dt : BLDC_COMMUT_ADV_DT_MAX_S;
      float omega_e = (float)g_enc_dir * BLDC_TWO_PI
                    * (float)BOARD_BLDC_POLE_PAIRS * g_meas_mech_hz;

      adv = omega_e * (BLDC_COMMUT_ADVANCE * adv_dt);
      if (adv > BLDC_COMMUT_ADV_MAX_RAD)
        {
          adv = BLDC_COMMUT_ADV_MAX_RAD;
        }
      else if (adv < -BLDC_COMMUT_ADV_MAX_RAD)
        {
          adv = -BLDC_COMMUT_ADV_MAX_RAD;
        }
    }

  g_theta = bldc_wrap_theta_val(bldc_elec_angle(g_enc_raw) + adv);
}

static void bldc_commutate_step(float dt)
{
  (void)dt;

  /* Both speed and position modes are now closed-loop FOC: the electrical angle
   * is taken from the encoder in the per-cycle control update, so there is no
   * free-running angle to integrate here.
   */
}

static int bldc_apply_dq(float vd, float vq, float theta)
{
  float va;
  float vb;
  float du;
  float dv;
  float dw;

  motor_inv_park(vd, vq, theta, &va, &vb);
  motor_svpwm(va, vb, &du, &dv, &dw);

  return tarox_pwm_set_duties_3(g_pwm_fd, du, dv, dw);
}

static int bldc_pwm_apply(void)
{
  return bldc_apply_dq(0.0f, g_vq_cmd, g_theta);
}

static void bldc_angle_log_tick(void)
{
  float mech_rad = as5600_raw_to_mech_rad(g_enc_raw);
  float mech_deg = mech_rad * 180.0f / BLDC_PI;
  float target_deg;
  float err_deg;
  float tgt_spd_hz;
  float meas_spd_hz = g_meas_mech_hz;
  float spd_err_hz;
  float tgt_spd_rpm;
  float meas_spd_rpm;
  float spd_err_rpm;

  if (g_mode == BLDC_MODE_SPEED)
    {
      /* Open-loop V/f: no position target. Report commanded mechanical speed
       * (elec_hz / pole_pairs) so the chart shows target vs actual speed.
       * The rotor always follows the commanded direction, but the encoder may
       * count the opposite way (no align in speed mode), so express the
       * measured speed in the commanded-direction frame to match signs.
       */

      target_deg = mech_deg;
      err_deg = 0.0f;
      tgt_spd_hz = (float)g_dir * g_elec_hz / (float)BOARD_BLDC_POLE_PAIRS;
    }
  else
    {
      float err_rad = bldc_mech_angle_error(g_target_mech_rad, mech_rad);

      target_deg = g_target_mech_rad * 180.0f / BLDC_PI;
      err_deg = err_rad * 180.0f / BLDC_PI;

      tgt_spd_hz = g_target_mech_hz;
      meas_spd_hz = g_vel_err_hz;
    }

  spd_err_hz = tgt_spd_hz - meas_spd_hz;
  tgt_spd_rpm = tgt_spd_hz * 60.0f;
  meas_spd_rpm = meas_spd_hz * 60.0f;
  spd_err_rpm = spd_err_hz * 60.0f;

  bldc_log_printf("[log] angle: mech=%.1f target=%.1f err=%.1f deg "
                  "spd=%.1f tspd=%.1f serr=%.1f rpm vq=%.0f permille "
                  "| cal off=%.1f dir=%d sgn=%d\n",
                  (double)mech_deg, (double)target_deg, (double)err_deg,
                  (double)meas_spd_rpm, (double)tgt_spd_rpm,
                  (double)spd_err_rpm,
                  (double)(g_vq_show_pu * 1000.0f),
                  (double)(g_elec_offset_rad * 180.0f / BLDC_PI),
                  g_enc_dir, g_vq_sign);
}

static int bldc_ctrl_keeper_main(int argc, char *argv[])
{
  struct timespec t_prev;
  struct timespec t_now;

  (void)argc;
  (void)argv;

  clock_gettime(CLOCK_MONOTONIC, &t_prev);

  while (g_running)
    {
      float dt;
      uint32_t sub;
      uint32_t nsubs;
      int ret;

      clock_gettime(CLOCK_MONOTONIC, &t_now);
      dt = timespec_delta_s(&t_prev, &t_now);
      if (dt <= 0.0f)
        {
          usleep(BLDC_CTRL_PERIOD_US);
          continue;
        }

      if (dt > BLDC_DT_MAX_S)
        {
          dt = 1.0f / (float)BLDC_CTRL_HZ;
        }

      if (g_mode == BLDC_MODE_POSITION)
        {
          ret = bldc_encoder_update(dt);
          if (ret < 0)
            {
              g_running = false;
              break;
            }

          bldc_position_control_update(dt);
          bldc_angle_log_tick();
        }
      else if (g_mode == BLDC_MODE_SPEED)
        {
          /* Closed-loop FOC speed control. Ignore occasional I2C read errors
           * (keep the last speed estimate) so a glitch never stops the motor.
           */

          bldc_encoder_update(dt);
          bldc_speed_control_update(dt);
          bldc_angle_log_tick();
        }

      nsubs = (uint32_t)(dt * (float)BLDC_CTRL_HZ + 0.5f);
      if (nsubs < 1)
        {
          nsubs = 1;
        }

      dt /= (float)nsubs;

      for (sub = 0; sub < nsubs && g_running; sub++)
        {
          bldc_commutate_step(dt);

          if (bldc_pwm_apply() < 0)
            {
              g_running = false;
              break;
            }
        }

      t_prev = t_now;
    }

  return 0;
}

static int bldc_motor_begin(uint16_t raw, float vq_pu)
{
  int ret;

  g_enc_raw = raw;
  g_log_last_valid = false;
  g_vq_max_pu = vq_pu;
  g_vq_cmd = 0.0f;

  /* Reset the finite-difference speed baseline for both modes so the first
   * telemetry samples are valid (speed mode otherwise reuses a stale prev).
   */

  g_prev_enc_raw = raw;
  g_meas_mech_hz = 0.0f;
  g_speed_delta_accum = 0;
  g_speed_dt_accum = 0.0f;
  g_speed_pi_i = 0.0f;

  if (g_mode == BLDC_MODE_POSITION)
    {
      bldc_position_reset(raw);
    }

  g_theta = bldc_elec_angle(raw);

  g_pwm_fd = tarox_pwm_open(TAROX_BLDC_PWM);
  if (g_pwm_fd < 0)
    {
      return g_pwm_fd;
    }

  ret = bldc_driver_enable(true);
  if (ret < 0)
    {
      tarox_pwm_close(g_pwm_fd);
      g_pwm_fd = TAROX_PWM_FD_INVALID;
      return ret;
    }

  ret = tarox_pwm_apply_3(g_pwm_fd, BOARD_BLDC_PWM_FREQ_HZ, 0.5f, 0.5f, 0.5f);
  if (ret < 0)
    {
      bldc_driver_disable();
      tarox_pwm_close(g_pwm_fd);
      g_pwm_fd = TAROX_PWM_FD_INVALID;
      return ret;
    }

  ret = tarox_pwm_run(g_pwm_fd);
  if (ret < 0)
    {
      bldc_driver_disable();
      tarox_pwm_close(g_pwm_fd);
      g_pwm_fd = TAROX_PWM_FD_INVALID;
      return ret;
    }

  g_running = true;

  g_keeper_pid = task_create("bldc_ctrl", 100, 2048,
                             bldc_ctrl_keeper_main, NULL);
  if (g_keeper_pid < 0)
    {
      g_running = false;
      tarox_pwm_halt(g_pwm_fd);
      bldc_driver_disable();
      tarox_pwm_close(g_pwm_fd);
      g_pwm_fd = TAROX_PWM_FD_INVALID;
      return g_keeper_pid;
    }

  return 0;
}

static int bldc_prepare_encoder(uint16_t *raw)
{
  int ret;

  ret = as5600_init(TAROX_I2C1_DEV);
  if (ret < 0)
    {
      return ret;
    }

  ret = as5600_read_raw(raw);
  if (ret < 0)
    {
      as5600_deinit();
    }

  return ret;
}

int bldc_init(void)
{
  /* Only reset runtime motor state; keep FOC calibration across NSH commands.
   * Never touch state while a motor task or align is active.
   */

  if (g_running || g_busy)
    {
      return 0;
    }

  g_pwm_fd = TAROX_PWM_FD_INVALID;
  g_en_fd = TAROX_GPIO_FD_INVALID;
  g_keeper_pid = (pid_t)-1;
  g_mode = BLDC_MODE_NONE;
  g_elec_hz = 0.0f;
  g_vq_max_pu = 0.0f;
  g_theta = 0.0f;
  g_vq_cmd = 0.0f;
  g_vq_show_pu = 0.0f;
  g_target_mech_rad = 0.0f;
  g_dir = 1;
  g_enc_raw = 0;
  g_prev_enc_raw = 0;
  g_meas_mech_hz = 0.0f;
  g_target_mech_hz = 0.0f;
  g_err_filt_rad = 0.0f;
  g_err_prev_filt_rad = 0.0f;
  g_vel_err_hz = 0.0f;
  g_pos_filter_reset = true;
  g_speed_pi_i = 0.0f;
  g_spd_filt_hz = 0.0f;
  g_log_last_valid = false;
  g_speed_delta_accum = 0;
  g_speed_dt_accum = 0.0f;
  return 0;
}

bool bldc_is_running(void)
{
  return g_running;
}

bldc_mode_e bldc_get_mode(void)
{
  return g_mode;
}

float bldc_get_target_mech_deg(void)
{
  return g_target_mech_rad * 180.0f / (float)M_PI;
}

int bldc_start(float elec_hz, float vq_pu)
{
  uint16_t raw;
  int ret;

  if (g_busy)
    {
      return -EBUSY;
    }

  if (elec_hz <= 0.0f || vq_pu <= 0.0f || vq_pu > 1.0f)
    {
      return -EINVAL;
    }

  /* Closed-loop FOC speed control needs the electrical-angle calibration. */

  if (!g_running && !g_foc_calibrated)
    {
      ret = bldc_align(BLDC_AUTO_ALIGN_VQ_PU);
      if (ret < 0)
        {
          return ret;
        }
    }

  /* Already running: update speed/torque live (and switch into speed mode
   * from position) without requiring 'bldc stop' first.
   */

  if (g_running)
    {
      if (g_mode != BLDC_MODE_SPEED)
        {
          g_speed_pi_i = 0.0f;
        }

      g_elec_hz = elec_hz;
      g_vq_max_pu = vq_pu;
      g_mode = BLDC_MODE_SPEED;
      return 0;
    }

  ret = bldc_prepare_encoder(&raw);
  if (ret < 0)
    {
      return ret;
    }

  g_elec_hz = elec_hz;
  g_mode = BLDC_MODE_SPEED;

  ret = bldc_motor_begin(raw, vq_pu);
  if (ret < 0)
    {
      as5600_deinit();
      g_mode = BLDC_MODE_NONE;
      return ret;
    }

  return 0;
}

int bldc_goto_mech_deg(float mech_deg, float vq_pu)
{
  uint16_t raw;
  int ret;

  if (vq_pu <= 0.0f || vq_pu > 1.0f)
    {
      return -EINVAL;
    }

  if (g_busy)
    {
      return -EBUSY;
    }

  /* Auto-calibrate FOC on first position command (when not already running). */

  if (!g_running && !g_foc_calibrated)
    {
      ret = bldc_align(BLDC_AUTO_ALIGN_VQ_PU);
      if (ret < 0)
        {
          return ret;
        }
    }

  g_target_mech_rad = bldc_wrap_theta_val(mech_deg * (float)M_PI / 180.0f);

  if (g_running)
    {
      if (g_mode != BLDC_MODE_POSITION || !as5600_is_ready())
        {
          return -EINVAL;
        }
      raw = g_enc_raw;

      g_vq_max_pu = vq_pu;
      bldc_position_reset(raw);
      g_enc_raw = raw;

      if (!g_foc_calibrated)
        {
          bldc_log_printf("[log] BLDC warn: run 'bldc align' first for position control\n");
        }

      return 0;
    }

  ret = bldc_prepare_encoder(&raw);
  if (ret < 0)
    {
      return ret;
    }

  g_mode = BLDC_MODE_POSITION;

  ret = bldc_motor_begin(raw, vq_pu);
  if (ret < 0)
    {
      as5600_deinit();
      g_mode = BLDC_MODE_NONE;
      return ret;
    }

  if (!g_foc_calibrated)
    {
      bldc_log_printf("[log] BLDC warn: run 'bldc align' first for position control\n");
    }

  return 0;
}

int bldc_hold(float vq_pu)
{
  uint16_t raw;
  int ret;

  if (vq_pu <= 0.0f || vq_pu > 1.0f)
    {
      return -EINVAL;
    }

  if (g_busy)
    {
      return -EBUSY;
    }

  /* Auto-calibrate FOC on first position command (when not already running). */

  if (!g_running && !g_foc_calibrated)
    {
      ret = bldc_align(BLDC_AUTO_ALIGN_VQ_PU);
      if (ret < 0)
        {
          return ret;
        }
    }

  if (g_running)
    {
      if (g_mode != BLDC_MODE_POSITION || !as5600_is_ready())
        {
          return -EINVAL;
        }
      raw = g_enc_raw;

      g_target_mech_rad = as5600_raw_to_mech_rad(raw);
      g_vq_max_pu = vq_pu;
      bldc_position_reset(raw);
      return 0;
    }

  ret = bldc_prepare_encoder(&raw);
  if (ret < 0)
    {
      return ret;
    }

  g_target_mech_rad = as5600_raw_to_mech_rad(raw);
  return bldc_goto_mech_deg(g_target_mech_rad * 180.0f / BLDC_PI, vq_pu);
}

int bldc_stop(void)
{
  if (!g_running)
    {
      as5600_deinit();
      g_mode = BLDC_MODE_NONE;
      return 0;
    }

  g_running = false;

  if (g_keeper_pid >= 0)
    {
      pid_t pid = g_keeper_pid;
      unsigned int n;

      g_keeper_pid = (pid_t)-1;
      kill(pid, SIGKILL);

      for (n = 0; n < 5000 && kill(pid, 0) == 0; n++)
        {
          usleep(1000);
        }
    }

  if (g_pwm_fd >= 0)
    {
      tarox_pwm_halt(g_pwm_fd);
      bldc_driver_disable();
      tarox_pwm_close(g_pwm_fd);
      g_pwm_fd = TAROX_PWM_FD_INVALID;
    }

  as5600_deinit();
  g_mode = BLDC_MODE_NONE;
  return 0;
}

int bldc_set_elec_hz(float elec_hz)
{
  if (elec_hz <= 0.0f || g_mode != BLDC_MODE_SPEED)
    {
      return -EINVAL;
    }

  g_elec_hz = elec_hz;
  return 0;
}

int bldc_reverse(void)
{
  if (g_mode != BLDC_MODE_SPEED)
    {
      return -EINVAL;
    }

  g_dir = -g_dir;
  g_speed_pi_i = 0.0f;
  return 0;
}

int bldc_encoder_read(uint16_t *raw)
{
  int ret;

  if (raw == NULL)
    {
      return -EINVAL;
    }

  ret = as5600_init(TAROX_I2C1_DEV);
  if (ret < 0)
    {
      return ret;
    }

  ret = as5600_read_raw(raw);
  as5600_deinit();
  return ret;
}

int bldc_align(float vq_pu)
{
  uint16_t raw0;
  uint16_t raw1;
  float mech0;
  float mech1;
  float delta_mech;
  int ret;

  if (g_running || g_busy)
    {
      return -EBUSY;
    }

  if (vq_pu <= 0.0f || vq_pu > 1.0f)
    {
      vq_pu = BLDC_ALIGN_VQ_PU;
    }

  g_busy = true;

  ret = bldc_prepare_encoder(&raw0);
  if (ret < 0)
    {
      g_busy = false;
      return ret;
    }

  mech0 = as5600_raw_to_mech_rad(raw0);

  g_pwm_fd = tarox_pwm_open(TAROX_BLDC_PWM);
  if (g_pwm_fd < 0)
    {
      as5600_deinit();
      g_busy = false;
      return g_pwm_fd;
    }


    ret = tarox_pwm_apply_3(g_pwm_fd, BOARD_BLDC_PWM_FREQ_HZ, 0.5f, 0.5f, 0.5f);
    if (ret < 0)
      {
        goto align_fail;
      }

  ret = tarox_pwm_apply_3(g_pwm_fd, BOARD_BLDC_PWM_FREQ_HZ, 0.5f, 0.5f, 0.5f);
  if (ret < 0)
    {
      goto align_fail;
    }

  ret = tarox_pwm_apply_3(g_pwm_fd, BOARD_BLDC_PWM_FREQ_HZ, 0.5f, 0.5f, 0.5f);
  if (ret < 0)
    {
      goto align_fail;
    }

  ret = tarox_pwm_apply_3(g_pwm_fd, BOARD_BLDC_PWM_FREQ_HZ, 0.5f, 0.5f, 0.5f);
  if (ret < 0)
    {
      goto align_fail;
    }
  ret = bldc_driver_enable(true);
  if (ret < 0)
    {
      tarox_pwm_close(g_pwm_fd);
      g_pwm_fd = TAROX_PWM_FD_INVALID;
      as5600_deinit();
      g_busy = false;
      return ret;
    }

  ret = tarox_pwm_apply_3(g_pwm_fd, BOARD_BLDC_PWM_FREQ_HZ, 0.5f, 0.5f, 0.5f);
  if (ret < 0)
    {
      goto align_fail;
    }

  ret = tarox_pwm_run(g_pwm_fd);
  if (ret < 0)
    {
      goto align_fail;
    }

  /* Step A: drive the D-axis (vd>0, vq=0) at electrical angle 0. The rotor
   * d-axis aligns to the field, settling at electrical angle 0 exactly.
   * The PWM duty latches in hardware, so set it once and sleep the whole
   * settle time (usleep granularity is one OS tick, so per-ms looping would
   * inflate the wait enormously).
   */

  if (bldc_apply_dq(vq_pu, 0.0f, 0.0f) < 0)
    {
      ret = -EIO;
      goto align_fail;
    }

  usleep(BLDC_ALIGN_MS * 1000U);

  ret = as5600_read_raw(&raw0);
  if (ret < 0)
    {
      goto align_fail;
    }

  mech0 = as5600_raw_to_mech_rad(raw0);

  /* Step B: advance the D-axis field by a fixed step, settle, read again.
   * The rotor follows, so the measured mechanical change reveals the encoder
   * direction (and confirms the rotor actually moved).
   */

  if (bldc_apply_dq(vq_pu, 0.0f, BLDC_ALIGN_STEP_RAD) < 0)
    {
      ret = -EIO;
      goto align_fail;
    }

  usleep(BLDC_ALIGN_MS * 1000U);

  ret = as5600_read_raw(&raw1);
  if (ret < 0)
    {
      goto align_fail;
    }

  mech1 = as5600_raw_to_mech_rad(raw1);
  delta_mech = bldc_mech_angle_error(mech1, mech0);

  if (fabsf(delta_mech) < 0.01f)
    {
      /* Rotor did not move: raise vq and retry. */

      bldc_log_printf("[log] BLDC align: rotor did not move (delta=%.2f deg), "
                      "increase vq\n",
                      (double)(delta_mech * 180.0f / BLDC_PI));
      ret = -EIO;
      goto align_fail;
    }

  /* Encoder direction: +1 if mech increases with electrical angle. */

  g_enc_dir = (delta_mech > 0.0f) ? 1 : -1;

  /* Step A settled the rotor at electrical angle 0, so the commutation angle
   * there must be 0:  enc_dir * pp * mech_A + offset = 0
   * offset = -enc_dir * pp * mech_A
   */

  g_elec_offset_rad = -(float)g_enc_dir * (float)BOARD_BLDC_POLE_PAIRS * mech0;
  g_elec_offset_rad = bldc_wrap_theta_val(g_elec_offset_rad);

  /* With offset/direction correct, positive Vq yields positive electrical
   * torque, which moves mech in the enc_dir sense. Position/speed loops work
   * in encoder frame, so vq_sign must equal enc_dir.
   */

  g_vq_sign = g_enc_dir;
  g_foc_calibrated = true;

  bldc_log_printf("[log] BLDC align: mechA=%.1f mechB=%.1f delta=%.1f deg, "
                  "enc_dir=%d offset=%.3f rad (%.1f deg) vq_sign=%d\n",
                  (double)(mech0 * 180.0f / BLDC_PI),
                  (double)(mech1 * 180.0f / BLDC_PI),
                  (double)(delta_mech * 180.0f / BLDC_PI),
                  g_enc_dir,
                  (double)g_elec_offset_rad,
                  (double)(g_elec_offset_rad * 180.0f / BLDC_PI),
                  g_vq_sign);
  bldc_log_printf("[log] BLDC align: copy to board.h -> "
                  "BOARD_AS5600_ELEC_OFFSET_RAD %.4ff\n",
                  (double)g_elec_offset_rad);

  ret = 0;

align_fail:
  if (g_pwm_fd >= 0)
    {
      tarox_pwm_halt(g_pwm_fd);
      bldc_driver_disable();
      tarox_pwm_close(g_pwm_fd);
      g_pwm_fd = TAROX_PWM_FD_INVALID;
    }

  as5600_deinit();
  g_busy = false;
  return ret;
}

float bldc_get_elec_offset_rad(void)
{
  return g_elec_offset_rad;
}

int bldc_get_vq_sign(void)
{
  return g_vq_sign;
}

bool bldc_is_foc_calibrated(void)
{
  return g_foc_calibrated;
}
