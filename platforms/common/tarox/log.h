#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void tarox_log_info(const char *fmt, ...);
void tarox_log_warn(const char *fmt, ...);
void tarox_log_error(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#define TAROX_INFO(fmt, ...) tarox_log_info(fmt, ##__VA_ARGS__)
#define TAROX_WARN(fmt, ...) tarox_log_warn(fmt, ##__VA_ARGS__)
#define TAROX_ERR(fmt, ...)  tarox_log_error(fmt, ##__VA_ARGS__)

