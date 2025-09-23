#pragma once
namespace flysq {
typedef unsigned long second;
typedef unsigned long millisecond;
typedef unsigned long microsecond;

second clock_s();
millisecond clock_ms();
microsecond clock_us();
}  // namespace flysq
