#pragma once

#include <tarox/module.hpp>

#include <stdbool.h>
#include <stdint.h>

namespace tarox
{

enum BldcMode
{
  BLDC_MODE_NONE = 0,
  BLDC_MODE_SPEED,
  BLDC_MODE_POSITION,
};

class Bldc : public ModuleBase<Bldc>
{
public:
	static int startCommand(int argc, char *argv[]);
	static int stopCommand();
	static int statusCommand();
	static int customCommand(int argc, char *argv[]);
	static int printUsage();

	static int init();
	static int start(float elec_hz, float vq_pu);
	static int gotoMechDeg(float mech_deg, float vq_pu);
	static int hold(float vq_pu);
	static int stop();
	static int setElecHz(float elec_hz);
	static int reverse();
	static bool isRunning();
	static BldcMode mode();
	static float targetMechDeg();
	static int encoderRead(uint16_t *raw);
	static int align(float vq_pu);
	static float elecOffsetRad();
	static int vqSign();
	static bool isFocCalibrated();
};

} // namespace tarox
