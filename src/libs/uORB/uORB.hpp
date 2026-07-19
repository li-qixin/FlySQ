#pragma once

#include <stdint.h>

#include <cstring>

namespace uORB
{

using Generation = uint32_t;

template<typename T>
class TopicStorage
{
public:
	static void publish(const T &data)
	{
		instance() = data;
		generation()++;
	}

	static bool copy(T &data, Generation &last_generation)
	{
		if (last_generation == generation()) {
			return false;
		}

		data = instance();
		last_generation = generation();
		return true;
	}

	static Generation current_generation()
	{
		return generation();
	}

private:
	static T &instance()
	{
		static T data{};
		return data;
	}

	static Generation &generation()
	{
		static Generation gen{0};
		return gen;
	}

};

} // namespace uORB
