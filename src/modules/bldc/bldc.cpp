#include "bldc.h"

#include <board.h>
#include <uORBTopics.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace tarox;

#define BLDC_MAX_VQ_PERMILLE      577UL   /* SVPWM linear limit: 1/sqrt(3) */
#define BLDC_DEFAULT_VQ_PERMILLE  BLDC_MAX_VQ_PERMILLE
#define BLDC_ENCODER_RAW_MAX      4096U
#define BLDC_TWO_PI               6.283185307179586f

static float bldc_encoder_raw_to_mech_rad(uint16_t raw)
{
  return ((float)raw / (float)BLDC_ENCODER_RAW_MAX) * BLDC_TWO_PI;
}

static float bldc_encoder_raw_to_elec_rad(uint16_t raw,
                                          unsigned int pole_pairs,
                                          float offset_rad)
{
  float theta = bldc_encoder_raw_to_mech_rad(raw) * (float)pole_pairs
              + offset_rad;

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

static int parse_vq_permille(char *const *argv, int argc, int idx, float *vq_pu)
{
  unsigned long vq_permille;
  char *end;

  if (idx >= argc)
    {
      vq_permille = BLDC_DEFAULT_VQ_PERMILLE;
    }
  else
    {
      vq_permille = strtoul(argv[idx], &end, 10);
      if (end == argv[idx] || *end != '\0' || vq_permille > BLDC_MAX_VQ_PERMILLE)
        {
          return -1;
        }
    }

  *vq_pu = (float)vq_permille / 1000.0f;
  return 0;
}

static void bldc_print_start_error(int ret, const char *cmd)
{
  if (ret == -EBUSY)
    {
      printf("%s failed: already running (run 'bldc stop' first)\n", cmd);
    }
  else if (ret == -EINVAL)
    {
      printf("%s failed: invalid mode or argument\n", cmd);
    }
  else
    {
      printf("%s failed: %d\n", cmd, ret);
    }
}

int Bldc::printUsage()
{
  printf("Usage:\n");
  printf("  bldc speed <rpm> [vq_permille]       speed mode (motor rpm)\n");
  printf("  bldc goto <mech_deg> [vq_permille]   position mode\n");
  printf("  bldc hold [vq_permille]              hold current angle\n");
  printf("  bldc align [vq_permille]             calibrate encoder offset\n");
  printf("  bldc stop\n");
  printf("  bldc reverse                         speed mode only\n");
  printf("  bldc status\n");
  printf("  bldc encoder\n");
  printf("\n");
  printf("  mech_deg: target mechanical angle 0-360 (single turn)\n");
  printf("  vq_permille: max torque voltage 0-%lu (default %lu, SVPWM linear)\n",
         BLDC_MAX_VQ_PERMILLE, BLDC_DEFAULT_VQ_PERMILLE);
  printf("  AS5600 on I2C1 PB8/PB9, pole_pairs=%u\n",
         (unsigned)BOARD_BLDC_POLE_PAIRS);
  return EXIT_FAILURE;
}

static const char *bldc_mode_name(BldcMode mode)
{
  switch (mode)
    {
    case BLDC_MODE_SPEED:
      return "speed";
    case BLDC_MODE_POSITION:
      return "position";
    default:
      return "stopped";
    }
}

int Bldc::startCommand(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  printUsage();
  return EXIT_FAILURE;
}

int Bldc::stopCommand()
{
  int ret = stop();
  if (ret < 0)
    {
      printf("bldc stop failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  printf("BLDC stopped\n");
  return EXIT_SUCCESS;
}

int Bldc::statusCommand()
{
  if (isRunning())
    {
      printf("BLDC running (%s)", bldc_mode_name(mode()));
      if (mode() == BLDC_MODE_POSITION)
        {
          printf(", target=%.1f deg", (double)targetMechDeg());
        }

      printf("\n");
    }
  else
    {
      printf("BLDC stopped");
      if (isFocCalibrated())
        {
          printf(", FOC calibrated offset=%.3f rad vq_sign=%d",
                 (double)elecOffsetRad(),
                 vqSign());
        }
      else
        {
          printf(", FOC not calibrated (run 'bldc align')");
        }

      printf("\n");
    }

  return EXIT_SUCCESS;
}

int Bldc::customCommand(int argc, char *argv[])
{
  init();

  if (argc < 1)
    {
      return printUsage();
    }

  if (strcmp(argv[0], "speed") == 0)
    {
      float rpm;
      float elec_hz;
      float vq_pu;
      int ret;

      if (argc < 2 || argc > 3)
        {
          return printUsage();
        }

      if (parse_float(argv[1], &rpm) != 0)
        {
          printf("Invalid rpm: %s\n", argv[1]);
          return EXIT_FAILURE;
        }

      if (parse_vq_permille(argv, argc, 2, &vq_pu) != 0)
        {
          printf("Invalid vq_permille\n");
          return EXIT_FAILURE;
        }

      elec_hz = rpm * (float)BOARD_BLDC_POLE_PAIRS / 60.0f;

      ret = start(elec_hz, vq_pu);
      if (ret < 0)
        {
          bldc_print_start_error(ret, "bldc speed");
          return EXIT_FAILURE;
        }

      printf("BLDC speed mode: %.1f rpm, Vq max=%.0f permille\n",
             (double)rpm, (double)(vq_pu * 1000.0f));
      return EXIT_SUCCESS;
    }

  if (strcmp(argv[0], "goto") == 0)
    {
      float mech_deg;
      float vq_pu;
      int ret;

      if (argc < 2 || argc > 3)
        {
          return printUsage();
        }

      if (parse_float(argv[1], &mech_deg) != 0)
        {
          printf("Invalid mech_deg: %s\n", argv[1]);
          return EXIT_FAILURE;
        }

      if (parse_vq_permille(argv, argc, 2, &vq_pu) != 0)
        {
          printf("Invalid vq_permille\n");
          return EXIT_FAILURE;
        }

      ret = gotoMechDeg(mech_deg, vq_pu);
      if (ret < 0)
        {
          bldc_print_start_error(ret, "bldc goto");
          return EXIT_FAILURE;
        }

      printf("BLDC position mode: goto %.1f deg, Vq max=%.0f permille\n",
             (double)mech_deg, (double)(vq_pu * 1000.0f));
      return EXIT_SUCCESS;
    }

  if (strcmp(argv[0], "hold") == 0)
    {
      float vq_pu;
      int ret;

      if (argc > 2)
        {
          return printUsage();
        }

      if (parse_vq_permille(argv, argc, 1, &vq_pu) != 0)
        {
          printf("Invalid vq_permille\n");
          return EXIT_FAILURE;
        }

      ret = hold(vq_pu);
      if (ret < 0)
        {
          bldc_print_start_error(ret, "bldc hold");
          return EXIT_FAILURE;
        }

      printf("BLDC position mode: hold at %.1f deg, Vq max=%.0f permille\n",
             (double)targetMechDeg(), (double)(vq_pu * 1000.0f));
      return EXIT_SUCCESS;
    }

  if (strcmp(argv[0], "align") == 0)
    {
      float vq_pu;
      int ret;

      if (argc > 2)
        {
          return printUsage();
        }

      if (parse_vq_permille(argv, argc, 1, &vq_pu) != 0)
        {
          printf("Invalid vq_permille\n");
          return EXIT_FAILURE;
        }

      ret = align(vq_pu);
      if (ret < 0)
        {
          bldc_print_start_error(ret, "bldc align");
          return EXIT_FAILURE;
        }

      return EXIT_SUCCESS;
    }

  if (strcmp(argv[0], "reverse") == 0)
    {
      int ret = reverse();
      if (ret < 0)
        {
          printf("bldc_reverse failed: speed mode only\n");
          return EXIT_FAILURE;
        }

      printf("BLDC direction reversed\n");
      return EXIT_SUCCESS;
    }

  if (strcmp(argv[0], "encoder") == 0)
    {
      uint16_t raw;
      int ret;

      ret = encoderRead(&raw);
      if (ret < 0)
        {
          printf("encoder read failed: %d\n", ret);
          return EXIT_FAILURE;
        }

      printf("AS5600 raw=%u  mech=%.1f deg  elec=%.1f deg (pp=%u offset=%.2f deg)\n",
             (unsigned)raw,
             (double)(bldc_encoder_raw_to_mech_rad(raw) * 180.0f / 3.14159265f),
             (double)(bldc_encoder_raw_to_elec_rad(raw, BOARD_BLDC_POLE_PAIRS,
                                              elecOffsetRad())
                      * 180.0f / 3.14159265f),
             (unsigned)BOARD_BLDC_POLE_PAIRS,
             (double)(elecOffsetRad() * 180.0f / 3.14159265f));
      return EXIT_SUCCESS;
    }

  return printUsage();
}

extern "C" int bldc_main(int argc, char *argv[])
{
  return tarox::Bldc::main(argc, argv);
}
