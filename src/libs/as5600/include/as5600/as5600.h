#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AS5600_RAW_MAX  4096U

int   as5600_init(const char *i2c_dev);
void  as5600_deinit(void);
bool  as5600_is_ready(void);

int   as5600_read_raw(uint16_t *raw);
float as5600_raw_to_mech_rad(uint16_t raw);
float as5600_raw_to_elec_rad(uint16_t raw, unsigned int pole_pairs,
                             float offset_rad);

#ifdef __cplusplus
}
#endif
