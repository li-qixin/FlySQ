#include "PWMOut.hpp"

#include <uORBTopics.h>

#include <tarox/drv_pwm_output.h>
#include <tarox/time.h>

#include <board.h>

#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

namespace tarox
{

PWMOut &PWMOut::instance()
{
	static PWMOut driver;
	return driver;
}

int PWMOut::startCommand(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	return instance().startTask();
}

int PWMOut::stopCommand()
{
	return instance().stopTask();
}

int PWMOut::statusCommand()
{
	return instance().status();
}

int PWMOut::customCommand(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	return printUsage();
}

int PWMOut::printUsage()
{
	printf("Usage: pwm_out {start|stop|status}\n");
	return -EINVAL;
}

int PWMOut::startTask()
{
	if (_task >= 0) {
		return 0;
	}

	int ret = start();

	if (ret < 0) {
		return ret;
	}

	_task = task_create("pwm_out", 100, 2048, taskMain, nullptr);

	if (_task < 0) {
		ret = _task;
		stop();
		_task = (pid_t)-1;
		return ret;
	}

	return 0;
}

int PWMOut::stopTask()
{
	if (_task >= 0) {
		pid_t pid = _task;
		_task = (pid_t)-1;
		stop();
		kill(pid, SIGKILL);
	}

	return 0;
}

int PWMOut::start()
{
	int ret = tarox_pwm_servo_init(BOARD_BLDC_PWM_FREQ_HZ, 3);

	if (ret < 0) {
		return ret;
	}

	ret = tarox_pwm_duty_set3(0.5f, 0.5f, 0.5f);

	if (ret < 0) {
		tarox_pwm_servo_deinit();
		return ret;
	}

	ret = tarox_pwm_servo_arm();

	if (ret < 0) {
		tarox_pwm_servo_deinit();
		return ret;
	}

	_running = true;
	return 0;
}

void PWMOut::stop()
{
	_running = false;
	tarox_pwm_servo_disarm();
	tarox_pwm_servo_deinit();
}

void PWMOut::run()
{
	while (_running) {
		struct actuator_phase_duty_s duty;

		if (TopicManager::instance().readActuatorPhaseDuty(duty)) {
			if (duty.armed) {
				tarox_pwm_duty_set3(duty.duty_u, duty.duty_v, duty.duty_w);
			} else {
				tarox_pwm_duty_set3(0.5f, 0.5f, 0.5f);
			}
		}

		tarox_usleep(1000);
	}
}

bool PWMOut::running() const
{
	return _running;
}

int PWMOut::status() const
{
	printf("pwm_out: %s\n", running() ? "running" : "stopped");
	return 0;
}

int PWMOut::taskMain(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	instance().run();
	return 0;
}

} // namespace tarox

extern "C" int pwm_out_main(int argc, char *argv[])
{
	return tarox::PWMOut::main(argc, argv);
}
