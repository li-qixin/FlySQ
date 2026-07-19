#pragma once

#include "WorkItem.hpp"

#include <stdint.h>

#include <tarox/time.h>

class ScheduledWorkItem : public WorkItem
{
public:
	void ScheduleDelayed(uint32_t delay_us)
	{
		tarox_usleep(delay_us);
		ScheduleNow();
	}
};
