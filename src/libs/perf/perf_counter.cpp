#include "perf_counter.h"

#include <tarox/time.h>

namespace tarox
{

PerfCounter::PerfCounter(const char *name) : _name(name)
{
}

void PerfCounter::begin()
{
	_start_us = tarox_absolute_time_us();
}

void PerfCounter::end()
{
	_event_count++;
	_elapsed_us += tarox_absolute_time_us() - _start_us;
}

uint32_t PerfCounter::eventCount() const
{
	return _event_count;
}

uint64_t PerfCounter::elapsedTimeUs() const
{
	return _elapsed_us;
}

} // namespace tarox
