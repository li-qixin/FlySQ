#include "param.h"

#include <cstring>

namespace
{

enum ParamType {
	PARAM_TYPE_FLOAT,
	PARAM_TYPE_INT32,
};

struct ParamEntry {
	const char *name;
	ParamType type;
	union {
		float f;
		int32_t i;
	} value;
};

ParamEntry g_params[] = {
	{"MC_PWM_RATE", PARAM_TYPE_INT32, {.i = 20000}},
	{"MC_SVPWM_SIGN", PARAM_TYPE_INT32, {.i = -1}},
	{"MC_ELEC_OFFSET", PARAM_TYPE_FLOAT, {.f = 0.0f}},
};

constexpr uint32_t param_count()
{
	return sizeof(g_params) / sizeof(g_params[0]);
}

} // namespace

namespace tarox
{

ParamStore &ParamStore::instance()
{
	static ParamStore store;
	return store;
}

param_t ParamStore::find(const char *name) const
{
	if (name == nullptr) {
		return PARAM_INVALID;
	}

	for (uint32_t i = 0; i < param_count(); i++) {
		if (std::strcmp(g_params[i].name, name) == 0) {
			return (param_t)i;
		}
	}

	return PARAM_INVALID;
}

int ParamStore::get(param_t param, void *value) const
{
	if (param < 0 || (uint32_t)param >= param_count() || value == nullptr) {
		return -1;
	}

	if (g_params[param].type == PARAM_TYPE_FLOAT) {
		*(float *)value = g_params[param].value.f;
	} else {
		*(int32_t *)value = g_params[param].value.i;
	}

	return 0;
}

int ParamStore::set(param_t param, const void *value)
{
	if (param < 0 || (uint32_t)param >= param_count() || value == nullptr) {
		return -1;
	}

	if (g_params[param].type == PARAM_TYPE_FLOAT) {
		g_params[param].value.f = *(const float *)value;
	} else {
		g_params[param].value.i = *(const int32_t *)value;
	}

	return 0;
}

} // namespace tarox
