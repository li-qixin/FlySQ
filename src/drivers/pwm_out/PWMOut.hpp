#pragma once

#include <tarox/module.hpp>

#include <sys/types.h>

namespace tarox
{

class PWMOut : public ModuleBase<PWMOut>
{
public:
	static PWMOut &instance();

	static int startCommand(int argc, char *argv[]);
	static int stopCommand();
	static int statusCommand();
	static int customCommand(int argc, char *argv[]);
	static int printUsage();

	int startTask();
	int stopTask();
	int start();
	void stop();
	void run();
	bool running() const;
	int status() const;

private:
	static int taskMain(int argc, char *argv[]);

	bool _running{false};
	pid_t _task{(pid_t)-1};
};

} // namespace tarox
