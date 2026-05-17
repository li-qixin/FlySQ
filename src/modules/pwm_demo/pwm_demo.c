#include "pwm_demo.h"

#include <board.h>
#include <tarox_pwm.h>

#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Builtin commands run in a short-lived child task; PWM stays open in a
 * helper until pwm_demo stop sends SIGUSR1.
 */

#define PWM_DEMO_KEEPER_STACK 2048
#define PWM_DEMO_KEEPER_PRIO  100

/* When frequency is in a typical hobby-servo band (40-60 Hz), permille is a
 * fraction of PWM period: pulse_ms ≈ permille / freq_hz (e.g. 50 Hz →
 * 25‰=0.5 ms … 125‰=2.5 ms). That span is the usual *electrical* full swing;
 * mechanical ±90° depends on the servo and horn — avoid crushing mechanical
 * stops. Values are clamped only to this 25-125‰ band at servo-ish freqs.
 */
#define PWM_SERVO_BAND_LO_HZ           40U
#define PWM_SERVO_BAND_HI_HZ           60U
#define PWM_SERVO_PERMILLE_MIN_SAFE    25U
#define PWM_SERVO_PERMILLE_MAX_SAFE    125U

#define SERVO_PWM_HZ 50
/* run_servo sweep across the same 0.5-2.5 ms span @ 50 Hz (25-125‰). */
#define SERVO_PERMILLE_MIN     25U
#define SERVO_PERMILLE_MAX     125U
#define SERVO_PERMILLE_STEP    5
#define SERVO_SWEEP_PERIOD_NS  100000000L

static pid_t g_pwm_keeper_pid = (pid_t)-1;

/* Set in pwm_demo_main before task_create; keeper reads these so it does not
 * depend on NuttX argv layout (some setups differ from argv[1]/argv[2]).
 */
static uint32_t g_keeper_run_freq_hz;
static uint32_t g_keeper_run_permille;

static char       g_keeper_name[] = "pwm_demo_keeper";
static char       g_servo_keeper_name[] = "pwm_demo_servo";

static int parse_u32(const char *s, uint32_t *out)
{
  char *end;
  unsigned long v;

  v = strtoul(s, &end, 10);
  if (end == s || *end != '\0' || v == 0)
    {
      return -1;
    }

  *out = (uint32_t)v;
  return 0;
}

static int parse_permille(const char *s, uint32_t *out)
{
  char *end;
  unsigned long v;

  v = strtoul(s, &end, 10);
  if (end == s || *end != '\0' || v > 1000)
    {
      return -1;
    }

  *out = (uint32_t)v;
  return 0;
}

static int pwm_demo_keeper_main(int argc, char *argv[])
{
  sigset_t waitset;
  uint32_t freq;
  uint32_t permille;
  int      fd = TAROX_PWM_FD_INVALID;
  int      sig;
  int      ret;

  (void)argc;
  (void)argv;

  freq = g_keeper_run_freq_hz;
  permille = g_keeper_run_permille;
  if (freq == 0 || permille > 1000)
    {
      return 1;
    }

  sigemptyset(&waitset);
  sigaddset(&waitset, SIGUSR1);
  if (sigprocmask(SIG_BLOCK, &waitset, NULL) != 0)
    {
      return 1;
    }

  fd = tarox_pwm_open(TAROX_PWM_DEMO);
  if (fd < 0)
    {
      return 1;
    }

  ret = tarox_pwm_apply(fd, freq, permille);
  if (ret < 0)
    {
      goto out_close;
    }

  ret = tarox_pwm_run(fd);
  if (ret < 0)
    {
      goto out_close;
    }

  ret = sigwait(&waitset, &sig);
  if (ret != 0)
    {
      goto out_stop;
    }

out_stop:
  tarox_pwm_halt(fd);
out_close:
  tarox_pwm_close(fd);

  return ret != 0 ? 1 : 0;
}

/* 50 Hz servo sweep: chunky permille steps + slower updates (see SERVO_* macros). */

static int pwm_demo_servo_keeper_main(int argc, char *argv[])
{
  sigset_t        waitset;
  struct timespec delay;
  int32_t         p;
  int32_t         dir;
  int             fd;
  sigset_t        pending;
  int             sig;

  (void)argc;
  (void)argv;

  sigemptyset(&waitset);
  sigaddset(&waitset, SIGUSR1);
  if (sigprocmask(SIG_BLOCK, &waitset, NULL) != 0)
    {
      return 1;
    }

  fd = tarox_pwm_open(TAROX_PWM_DEMO);
  if (fd < 0)
    {
      return 1;
    }

  p = (int32_t)((SERVO_PERMILLE_MIN + SERVO_PERMILLE_MAX) / 2U);
  dir = (int32_t)SERVO_PERMILLE_STEP;

  if (tarox_pwm_apply(fd, SERVO_PWM_HZ, (uint32_t)p) < 0 ||
      tarox_pwm_run(fd) < 0)
    {
      tarox_pwm_close(fd);
      return 1;
    }

  delay.tv_sec = 0;
  delay.tv_nsec = SERVO_SWEEP_PERIOD_NS;

  for (;;)
    {
      nanosleep(&delay, NULL);

      sigemptyset(&pending);
      if (sigpending(&pending) == 0 && sigismember(&pending, SIGUSR1))
        {
          sigwait(&waitset, &sig);
          break;
        }

      p += dir;

      if (p > (int32_t)SERVO_PERMILLE_MAX)
        {
          p = (int32_t)SERVO_PERMILLE_MAX;
          dir = -(int32_t)SERVO_PERMILLE_STEP;
        }
      else if (p < (int32_t)SERVO_PERMILLE_MIN)
        {
          p = (int32_t)SERVO_PERMILLE_MIN;
          dir = (int32_t)SERVO_PERMILLE_STEP;
        }

      if (tarox_pwm_apply(fd, SERVO_PWM_HZ, (uint32_t)p) < 0)
        {
          break;
        }
    }

  tarox_pwm_halt(fd);
  tarox_pwm_close(fd);
  return 0;
}

static int stop_keeper(void)
{
  pid_t    pid;
  unsigned n;

  pid = g_pwm_keeper_pid;
  if (pid < 0)
    {
      return 0;
    }

  if (kill(pid, SIGUSR1) < 0)
    {
      if (errno == ESRCH)
        {
          g_pwm_keeper_pid = (pid_t)-1;
          return 0;
        }

      g_pwm_keeper_pid = (pid_t)-1;
      return -errno;
    }

  for (n = 0; n < 5000 && kill(pid, 0) == 0; n++)
    {
      usleep(1000);
    }

  g_pwm_keeper_pid = (pid_t)-1;
  return 0;
}

int pwm_demo_main(int argc, char *argv[])
{
  uint32_t freq = 1000;
  uint32_t permille = 300;
  pid_t    pid;
  int      i;

  /* Builtin may be started with task_spawn(..., &argv[1], ...) so argv[0] is
   * the subcommand; with posix_spawn the full argv has argv[0] == "pwm_demo".
   */

  if (argc < 1 || argv == NULL || argv[0] == NULL)
    {
      return -1;
    }

  i = 0;
  if (strcmp(argv[0], "pwm_demo") == 0)
    {
      i = 1;
      if (argc <= i || argv[i] == NULL)
        {
          return -1;
        }
    }

  if (strcmp(argv[i], "stop") == 0)
    {
      return stop_keeper();
    }

  if (strcmp(argv[i], "run_servo") == 0)
    {
      if (g_pwm_keeper_pid >= 0)
        {
          stop_keeper();
        }

      pid = task_create(g_servo_keeper_name, PWM_DEMO_KEEPER_PRIO,
                        PWM_DEMO_KEEPER_STACK, pwm_demo_servo_keeper_main, NULL);
      if (pid < 0)
        {
          return -errno;
        }

      g_pwm_keeper_pid = pid;
      return 0;
    }

  if (strcmp(argv[i], "mid") == 0)
    {
      freq = 50;
      permille = 75;
    }
  else if (strcmp(argv[i], "start") == 0)
    {
      if (argc > i + 1 && parse_u32(argv[i + 1], &freq) != 0)
        {
          return -1;
        }

      if (argc > i + 2 && parse_permille(argv[i + 2], &permille) != 0)
        {
          return -1;
        }
    }
  else
    {
      return -1;
    }

  if (freq >= PWM_SERVO_BAND_LO_HZ && freq <= PWM_SERVO_BAND_HI_HZ)
    {
      if (permille < PWM_SERVO_PERMILLE_MIN_SAFE)
        {
          permille = PWM_SERVO_PERMILLE_MIN_SAFE;
        }
      else if (permille > PWM_SERVO_PERMILLE_MAX_SAFE)
        {
          permille = PWM_SERVO_PERMILLE_MAX_SAFE;
        }
    }

  if (g_pwm_keeper_pid >= 0)
    {
      stop_keeper();
    }

  g_keeper_run_freq_hz = freq;
  g_keeper_run_permille = permille;

  pid = task_create(g_keeper_name, PWM_DEMO_KEEPER_PRIO,
                    PWM_DEMO_KEEPER_STACK, pwm_demo_keeper_main, NULL);
  if (pid < 0)
    {
      return -errno;
    }

  g_pwm_keeper_pid = pid;
  return 0;
}
