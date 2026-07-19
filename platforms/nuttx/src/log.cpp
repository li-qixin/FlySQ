#include <tarox/log.h>

#include <cstdarg>
#include <cstdio>

extern "C"
{

static void tarox_log_print(const char *level, const char *fmt, va_list ap)
{
	std::printf("[%s] ", level);
	std::vprintf(fmt, ap);
}

void tarox_log_info(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	tarox_log_print("info", fmt, ap);
	va_end(ap);
}

void tarox_log_warn(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	tarox_log_print("warn", fmt, ap);
	va_end(ap);
}

void tarox_log_error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	tarox_log_print("error", fmt, ap);
	va_end(ap);
}

}
