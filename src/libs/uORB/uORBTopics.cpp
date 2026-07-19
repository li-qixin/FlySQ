#include "uORBTopics.h"

#include "Publication.hpp"

namespace tarox
{

TopicManager &TopicManager::instance()
{
	static TopicManager manager;
	return manager;
}

void TopicManager::publishEncoderAngle(const encoder_angle_s &data)
{
	_encoder_angle = data;
	_encoder_angle_valid = data.valid;

	uORB::Publication<encoder_angle_s> pub;
	pub.publish(data);
}

bool TopicManager::readEncoderAngle(encoder_angle_s &data) const
{
	if (!_encoder_angle_valid) {
		return false;
	}

	data = _encoder_angle;
	return true;
}

void TopicManager::publishActuatorPhaseDuty(const actuator_phase_duty_s &data)
{
	_actuator_phase_duty = data;
	_actuator_phase_duty_valid = true;

	uORB::Publication<actuator_phase_duty_s> pub;
	pub.publish(data);
}

bool TopicManager::readActuatorPhaseDuty(actuator_phase_duty_s &data) const
{
	if (!_actuator_phase_duty_valid) {
		return false;
	}

	data = _actuator_phase_duty;
	return true;
}

}
