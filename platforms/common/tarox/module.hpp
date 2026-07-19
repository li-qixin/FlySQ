#pragma once

#include <tarox/defines.h>

#include <string.h>

template<class T>
class ModuleBase
{
public:
	static int main(int argc, char *argv[])
	{
		if (argc < 2) {
			return T::printUsage();
		}

		if (strcmp(argv[1], "start") == 0) {
			return T::startCommand(argc - 1, argv + 1);
		}

		if (strcmp(argv[1], "stop") == 0) {
			return T::stopCommand();
		}

		if (strcmp(argv[1], "status") == 0) {
			return T::statusCommand();
		}

		return T::customCommand(argc - 1, argv + 1);
	}
};
