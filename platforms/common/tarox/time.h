#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t tarox_abstime;

tarox_abstime tarox_absolute_time_us(void);
int tarox_usleep(uint32_t usec);

#ifdef __cplusplus
}
#endif

