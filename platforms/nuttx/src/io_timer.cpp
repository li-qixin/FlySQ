#include <stdint.h>

extern "C"
{

int tarox_io_timer_init(uint32_t timer, uint32_t frequency_hz)
{
	(void)timer;
	(void)frequency_hz;
	return 0;
}

}

