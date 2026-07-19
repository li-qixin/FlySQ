#pragma once

#include <stdint.h>

namespace tarox
{

class PerfCounter
{
public:
	explicit PerfCounter(const char *name);

	void begin();
	void end();
	uint32_t eventCount() const;
	uint64_t elapsedTimeUs() const;

private:
	const char *_name;
	uint32_t _event_count{0};
	uint64_t _start_us{0};
	uint64_t _elapsed_us{0};
};

} // namespace tarox
