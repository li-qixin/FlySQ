#pragma once

class WorkItem
{
public:
	virtual ~WorkItem() = default;
	virtual void Run() = 0;

	void ScheduleNow()
	{
		Run();
	}
};

