#pragma once

#include <tarox/module.hpp>

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

namespace tarox
{

constexpr uint32_t AS5600RawMax = 4096U;

class As5600 : public ModuleBase<As5600>
{
public:
	static As5600 &instance();

	static int startCommand(int argc, char *argv[]);
	static int stopCommand();
	static int statusCommand();
	static int customCommand(int argc, char *argv[]);
	static int printUsage();

	int run();

	static float rawToMechRad(uint16_t raw);
	static float rawToElecRad(uint16_t raw, unsigned int pole_pairs,
	                          float offset_rad);

private:
	int start();
	int stop();
	int status() const;
	int init(const char *i2c_dev);
	void deinit();
	int readRaw(uint16_t &raw);
	int transfer(uint8_t reg, uint8_t *buf, size_t len, bool read);
	void publishRaw(uint16_t raw);
	int _i2c_fd {-1};
	bool _running {false};
	pid_t _task {(pid_t)-1};
};

} // namespace tarox
