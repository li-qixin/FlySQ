#pragma once

#include "uORB.hpp"

namespace uORB
{

template<typename T>
class Subscription
{
public:
	bool updated() const
	{
		return TopicStorage<T>::current_generation() != _generation;
	}

	bool copy(T *data)
	{
		if (data == nullptr) {
			return false;
		}

		return TopicStorage<T>::copy(*data, _generation);
	}

private:
	Generation _generation{0};
};

} // namespace uORB

