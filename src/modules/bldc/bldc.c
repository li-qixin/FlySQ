#include "bldc.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_float(const char *s, float *out)
{
  char *end;
  float v;

  v = strtof(s, &end);
  if (end == s || *end != '\0')
    {
      return -1;
    }

  *out = v;
  return 0;
}

static void bldc_usage(void)
{
  printf("Usage:\n");
  printf("  bldc start <elec_hz> <vq_permille>\n");
  printf("  bldc stop\n");
  printf("  bldc reverse\n");
  printf("  bldc status\n");
  printf("\n");
  printf("  elec_hz: electrical frequency (Hz)\n");
  printf("  vq_permille: q-axis voltage at 20Hz ref (0-1000), auto V/f scaled\n");
  printf("  mech RPM ~= elec_hz * 60 / pole_pairs\n");
}

int bldc_main(int argc, char *argv[])
{
  if (argc < 2)
    {
      bldc_usage();
      return EXIT_FAILURE;
    }

  if (strcmp(argv[1], "start") == 0)
    {
      float elec_hz;
      float vq_pu;
      unsigned long vq_permille;
      char *end;
      int ret;

      if (argc != 4)
        {
          bldc_usage();
          return EXIT_FAILURE;
        }

      if (parse_float(argv[2], &elec_hz) != 0)
        {
          printf("Invalid elec_hz: %s\n", argv[2]);
          return EXIT_FAILURE;
        }

      vq_permille = strtoul(argv[3], &end, 10);
      if (end == argv[3] || *end != '\0' || vq_permille > 1000)
        {
          printf("Invalid vq_permille: %s\n", argv[3]);
          return EXIT_FAILURE;
        }

      vq_pu = (float)vq_permille / 1000.0f;

      ret = bldc_start(elec_hz, vq_pu);
      if (ret < 0)
        {
          if (ret == -EBUSY)
            {
              printf("bldc_start failed: already running (run 'bldc stop' first)\n");
            }
          else
            {
              printf("bldc_start failed: %d\n", ret);
            }
          return EXIT_FAILURE;
        }

      printf("BLDC running: %.2f Hz elec, Vq=%lu permille (ref@20Hz)\n",
             (double)elec_hz, vq_permille);
      return EXIT_SUCCESS;
    }

  if (strcmp(argv[1], "stop") == 0)
    {
      int ret = bldc_stop();
      if (ret < 0)
        {
          printf("bldc_stop failed: %d\n", ret);
          return EXIT_FAILURE;
        }

      printf("BLDC stopped\n");
      return EXIT_SUCCESS;
    }

  if (strcmp(argv[1], "reverse") == 0)
    {
      int ret = bldc_reverse();
      if (ret < 0)
        {
          printf("bldc_reverse failed: %d\n", ret);
          return EXIT_FAILURE;
        }

      printf("BLDC direction reversed\n");
      return EXIT_SUCCESS;
    }

  if (strcmp(argv[1], "status") == 0)
    {
      printf("BLDC %s\n", bldc_is_running() ? "running" : "stopped");
      return EXIT_SUCCESS;
    }

  bldc_usage();
  return EXIT_FAILURE;
}
