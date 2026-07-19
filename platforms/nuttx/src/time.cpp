#include <tarox/time.h>

#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>

extern "C"
{

tarox_abstime tarox_absolute_time_us(void)
{
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	return (tarox_abstime)tv.tv_sec * 1000000ULL + (tarox_abstime)tv.tv_usec;
}

int tarox_usleep(uint32_t usec)
{
	return usleep(usec);
}

}
