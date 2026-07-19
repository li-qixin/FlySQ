#include "as5600.h"

#include <uORBTopics.h>

#include <tarox/time.h>

#include <board.h>

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include <nuttx/i2c/i2c_master.h>

#define AS5600_I2C_ADDR    0x36u
#define AS5600_REG_ANGLE_H 0x0cu
#define AS5600_I2C_HZ      400000
#define AS5600_TWO_PI      6.283185307179586f

namespace tarox
{

As5600 &As5600::instance()
{
	static As5600 driver;
	return driver;
}

int As5600::transfer(uint8_t reg, uint8_t *buf, size_t len, bool read)
{
	struct i2c_msg_s msgs[2];
	struct i2c_transfer_s xfer;
	int ret;

	if (_i2c_fd < 0) {
		return -ENODEV;
	}

	msgs[0].frequency = AS5600_I2C_HZ;
	msgs[0].addr      = AS5600_I2C_ADDR;
	msgs[0].flags     = 0;
	msgs[0].buffer    = &reg;
	msgs[0].length    = 1;

	msgs[1].frequency = AS5600_I2C_HZ;
	msgs[1].addr      = AS5600_I2C_ADDR;
	msgs[1].flags     = read ? I2C_M_READ : 0;
	msgs[1].buffer    = buf;
	msgs[1].length    = (ssize_t)len;

	xfer.msgv = msgs;
	xfer.msgc = 2;

	ret = ioctl(_i2c_fd, I2CIOC_TRANSFER, (unsigned long)(uintptr_t)&xfer);

	if (ret < 0) {
		return -EIO;
	}

	return 0;
}

int As5600::init(const char *i2c_dev)
{
	if (_i2c_fd >= 0) {
		return 0;
	}

	if (i2c_dev == NULL) {
		return -EINVAL;
	}

	_i2c_fd = open(i2c_dev, O_RDWR);

	if (_i2c_fd < 0) {
		return -ENODEV;
	}

	return 0;
}

void As5600::deinit()
{
	if (_i2c_fd >= 0) {
		close(_i2c_fd);
		_i2c_fd = -1;
	}
}

int As5600::readRaw(uint16_t &raw)
{
	uint8_t buf[2];
	int ret;

	ret = transfer(AS5600_REG_ANGLE_H, buf, sizeof(buf), true);

	if (ret < 0) {
		return ret;
	}

	raw = (uint16_t)(((uint16_t)(buf[0] & 0x0fu) << 8) | buf[1]);
	return 0;
}

float As5600::rawToMechRad(uint16_t raw)
{
	return ((float)raw / (float)AS5600RawMax) * AS5600_TWO_PI;
}

float As5600::rawToElecRad(uint16_t raw, unsigned int pole_pairs,
                           float offset_rad)
{
	float theta = rawToMechRad(raw) * (float)pole_pairs + offset_rad;

	while (theta >= AS5600_TWO_PI) {
		theta -= AS5600_TWO_PI;
	}

	while (theta < 0.0f) {
		theta += AS5600_TWO_PI;
	}

	return theta;
}

void As5600::publishRaw(uint16_t raw)
{
	struct encoder_angle_s angle {};

	angle.timestamp = tarox_absolute_time_us();
	angle.raw = raw;
	angle.mech_rad = rawToMechRad(raw);
	angle.valid = true;

	TopicManager::instance().publishEncoderAngle(angle);
}

int As5600::run()
{
	while (_running) {
		uint16_t raw;

		if (readRaw(raw) == 0) {
			publishRaw(raw);
		}

		tarox_usleep(1000);
	}

	return 0;
}

static int as5600_task_main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	return As5600::instance().run();
}

int As5600::start()
{
	if (_task >= 0) {
		return 0;
	}

	int ret = init(TAROX_I2C1_DEV);

	if (ret < 0) {
		return ret;
	}

	_running = true;
	_task = task_create("as5600", 100, 2048, as5600_task_main, nullptr);

	if (_task < 0) {
		ret = _task;
		_running = false;
		deinit();
		_task = (pid_t)-1;
		return ret;
	}

	return 0;
}

int As5600::stop()
{
	if (_task >= 0) {
		pid_t pid = _task;
		_task = (pid_t)-1;
		_running = false;
		kill(pid, SIGKILL);
	}

	deinit();
	return 0;
}

int As5600::status() const
{
	printf("as5600: %s\n", _task >= 0 ? "running" : "stopped");
	return 0;
}

int As5600::startCommand(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	return instance().start();
}

int As5600::stopCommand()
{
	return instance().stop();
}

int As5600::statusCommand()
{
	return instance().status();
}

int As5600::customCommand(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	return printUsage();
}

int As5600::printUsage()
{
	printf("Usage: as5600 {start|stop|status}\n");
	return -EINVAL;
}

} // namespace tarox

extern "C" int as5600_main(int argc, char *argv[])
{
	return tarox::As5600::main(argc, argv);
}
