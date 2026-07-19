#pragma once

#include "WorkItem.hpp"

class WorkQueue
{
public:
	void Add(WorkItem *item)
	{
		if (item != nullptr) {
			item->Run();
		}
	}
};

WorkQueue &tarox_default_work_queue();

