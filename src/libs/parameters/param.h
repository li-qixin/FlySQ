#pragma once

#include <stdint.h>

namespace tarox
{

typedef int32_t param_t;

#define PARAM_INVALID (-1)

class ParamStore
{
public:
	static ParamStore &instance();

	param_t find(const char *name) const;
	int get(param_t param, void *value) const;
	int set(param_t param, const void *value);
};

} // namespace tarox
