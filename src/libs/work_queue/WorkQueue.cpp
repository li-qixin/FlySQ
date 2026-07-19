#include "WorkQueue.hpp"

WorkQueue &tarox_default_work_queue()
{
	static WorkQueue queue;
	return queue;
}

