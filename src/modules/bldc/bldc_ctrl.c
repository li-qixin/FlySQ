#include "bldc.h"

#include <board.h>
#include <motor/park.h>
#include <motor/svpwm.h>
#include <tarox_pwm.h>

#include <errno.h>
#include <math.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#define BLDC_CTRL_HZ           1000U
#define BLDC_CTRL_PERIOD_US    (1000000U / BLDC_CTRL_HZ)
#define BLDC_TWO_PI            6.283185307179586f
#define BLDC_DT_MAX_S          0.5f
#define BLDC_VF_REF_HZ         20.0f

static int      g_pwm_fd = TAROX_PWM_FD_INVALID;
static pid_t    g_keeper_pid = (pid_t)-1;
static bool     g_running;
static float    g_elec_hz;
static float    g_vq_pu;
static float    g_theta;
static int      g_dir = 1;

static float timespec_delta_s(const struct timespec *start,
                              const struct timespec *end)
{
  return (float)(end->tv_sec - start->tv_sec)
       + (float)(end->tv_nsec - start->tv_nsec) * 1e-9f;
}

static void bldc_wrap_theta(void)
{
  while (g_theta >= BLDC_TWO_PI)
    {
      g_theta -= BLDC_TWO_PI;
    }

  while (g_theta < 0.0f)
    {
      g_theta += BLDC_TWO_PI;
    }
}

static float bldc_effective_vq(float elec_hz, float vq_pu)
{
  float scaled = vq_pu * (elec_hz / BLDC_VF_REF_HZ);

  if (scaled > 1.0f)
    {
      scaled = 1.0f;
    }

  if (scaled < vq_pu)
    {
      scaled = vq_pu;
    }

  return scaled;
}

static int bldc_ctrl_update(float dt)
{
  float vq = bldc_effective_vq(g_elec_hz, g_vq_pu);
  float va;
  float vb;
  float du;
  float dv;
  float dw;

  g_theta += (float)g_dir * BLDC_TWO_PI * g_elec_hz * dt;
  bldc_wrap_theta();

  motor_inv_park(0.0f, vq, g_theta, &va, &vb);
  motor_svpwm(va, vb, &du, &dv, &dw);

  return tarox_pwm_set_duties_3(g_pwm_fd, du, dv, dw);
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

      /* Split each tick window into ~1 kHz PWM updates (no heavy ioctl burst). */

      nsubs = (uint32_t)(dt * (float)BLDC_CTRL_HZ + 0.5f);
      if (nsubs < 1)
        {
          nsubs = 1;
        }

      dt /= (float)nsubs;

      for (sub = 0; sub < nsubs && g_running; sub++)
        {
          if (bldc_ctrl_update(dt) < 0)
            {
              g_running = false;
              break;
            }
        }

      t_prev = t_now;
    }

  return 0;
}

int bldc_init(void)
{
  g_pwm_fd = TAROX_PWM_FD_INVALID;
  g_keeper_pid = (pid_t)-1;
  g_running = false;
  g_elec_hz = 0.0f;
  g_vq_pu = 0.0f;
  g_theta = 0.0f;
  g_dir = 1;
  return 0;
}

bool bldc_is_running(void)
{
  return g_running;
}

int bldc_start(float elec_hz, float vq_pu)
{
  int ret;

  if (g_running)
    {
      return -EBUSY;
    }

  if (elec_hz <= 0.0f || vq_pu <= 0.0f || vq_pu > 1.0f)
    {
      return -EINVAL;
    }

  g_pwm_fd = tarox_pwm_open(TAROX_BLDC_PWM);
  if (g_pwm_fd < 0)
    {
      return g_pwm_fd;
    }

  ret = tarox_pwm_driver_enable(TAROX_BLDC_PWM, true);
  if (ret < 0)
    {
      tarox_pwm_close(g_pwm_fd);
      g_pwm_fd = TAROX_PWM_FD_INVALID;
      return ret;
    }

  ret = tarox_pwm_apply_3(g_pwm_fd, BOARD_BLDC_PWM_FREQ_HZ, 0.5f, 0.5f, 0.5f);
  if (ret < 0)
    {
      tarox_pwm_driver_enable(TAROX_BLDC_PWM, false);
      tarox_pwm_close(g_pwm_fd);
      g_pwm_fd = TAROX_PWM_FD_INVALID;
      return ret;
    }

  ret = tarox_pwm_run(g_pwm_fd);
  if (ret < 0)
    {
      tarox_pwm_driver_enable(TAROX_BLDC_PWM, false);
      tarox_pwm_close(g_pwm_fd);
      g_pwm_fd = TAROX_PWM_FD_INVALID;
      return ret;
    }

  g_elec_hz = elec_hz;
  g_vq_pu = vq_pu;
  g_theta = 0.0f;
  g_running = true;

  g_keeper_pid = task_create("bldc_ctrl", 100, 2048,
                             bldc_ctrl_keeper_main, NULL);
  if (g_keeper_pid < 0)
    {
      g_running = false;
      tarox_pwm_halt(g_pwm_fd);
      tarox_pwm_driver_enable(TAROX_BLDC_PWM, false);
      tarox_pwm_close(g_pwm_fd);
      g_pwm_fd = TAROX_PWM_FD_INVALID;
      return g_keeper_pid;
    }

  return 0;
}

int bldc_stop(void)
{
  if (!g_running)
    {
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
      tarox_pwm_driver_enable(TAROX_BLDC_PWM, false);
      tarox_pwm_close(g_pwm_fd);
      g_pwm_fd = TAROX_PWM_FD_INVALID;
    }

  return 0;
}

int bldc_set_elec_hz(float elec_hz)
{
  if (elec_hz <= 0.0f)
    {
      return -EINVAL;
    }

  g_elec_hz = elec_hz;
  return 0;
}

int bldc_reverse(void)
{
  g_dir = -g_dir;
  return 0;
}
