#pragma once

#include "param.h"

namespace tarox
{

template<typename T>
class Parameter
{
public:
	explicit Parameter(const char *name) : _handle(ParamStore::instance().find(name)) {}

	bool update()
	{
		return ParamStore::instance().get(_handle, &_value) == 0;
	}

	const T &get() const
	{
		return _value;
	}

	bool set(const T &value)
	{
		return ParamStore::instance().set(_handle, &value) == 0;
	}

private:
	param_t _handle{PARAM_INVALID};
	T _value{};
};

} // namespace tarox
