#pragma once

#include "Mixer.hpp"

#include <actuator_motor_setpoint.h>

class MotorMixer : public Mixer
{
public:
	void set_setpoint(const actuator_motor_setpoint_s &setpoint)
	{
		_setpoint = setpoint;
	}

	bool mix(float *outputs, unsigned output_count) override
	{
		if (outputs == nullptr || output_count == 0) {
			return false;
		}

		const unsigned count = output_count < ACTUATOR_MOTOR_SETPOINT_MAX_CONTROLS
		                       ? output_count
		                       : ACTUATOR_MOTOR_SETPOINT_MAX_CONTROLS;

		for (unsigned i = 0; i < count; i++) {
			outputs[i] = _setpoint.control[i];
		}

		return true;
	}

private:
	actuator_motor_setpoint_s _setpoint{};
};

