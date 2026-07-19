#pragma once

#include "Subscription.hpp"

#include <WorkItem.hpp>

namespace uORB
{

template<typename T>
class SubscriptionCallbackWorkItem : public Subscription<T>
{
public:
	explicit SubscriptionCallbackWorkItem(WorkItem *work_item) : _work_item(work_item) {}

	bool copy(T *data)
	{
		const bool did_copy = Subscription<T>::copy(data);

		if (did_copy && _work_item != nullptr) {
			_work_item->ScheduleNow();
		}

		return did_copy;
	}

private:
	WorkItem *_work_item;
};

} // namespace uORB
