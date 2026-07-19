#pragma once

#include "uORB.hpp"

namespace uORB
{

template<typename T>
class Publication
{
public:
	bool publish(const T &data)
	{
		TopicStorage<T>::publish(data);
		return true;
	}
};

} // namespace uORB

