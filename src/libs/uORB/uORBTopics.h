#pragma once

#include <actuator_phase_duty.h>
#include <encoder_angle.h>

namespace tarox
{

class TopicManager
{
public:
	static TopicManager &instance();

	void publishEncoderAngle(const encoder_angle_s &data);
	bool readEncoderAngle(encoder_angle_s &data) const;
	void publishActuatorPhaseDuty(const actuator_phase_duty_s &data);
	bool readActuatorPhaseDuty(actuator_phase_duty_s &data) const;

private:
	encoder_angle_s _encoder_angle {};
	bool _encoder_angle_valid {false};
	actuator_phase_duty_s _actuator_phase_duty {};
	bool _actuator_phase_duty_valid {false};
};

} // namespace tarox
