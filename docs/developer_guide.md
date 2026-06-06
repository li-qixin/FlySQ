# Tarox 开发者手册

## 1. 项目概述

Tarox 是基于 **NuttX RTOS** 的嵌入式固件框架，面向 STM32F4 系列微控制器（Cortex-M4），提供模块化的应用开发架构。

### 技术栈

| 组件 | 说明 |
|------|------|
| 操作系统 | NuttX RTOS |
| 微控制器 | STM32F407ZG (Demo Board) |
| 构建工具 | CMake + Ninja/Make |
| 通信接口 | NSH Shell (USART3, 115200 8N1) |
| 烧录工具 | ST-Link / OpenOCD |

---

## 2. 目录结构

```
tarox/
|-- Makefile                 # 构建入口
|-- CMakeLists.txt           # CMake 根配置
|-- Kconfig                  # Tarox 模块配置
|
|-- boards/ # 板级配置
|   `-- demo/ # Demo板 (STM32F407)
|       |-- default.taroxconfig # 默认配置
|       |-- src/             # 板级驱动 (GPIO/PWM)
|       `-- nuttx-config/   # NuttX 配置
|
|-- platforms/              # 平台抽象层
|   |-- common/include/     # 通用 API 头文件
|   |       |-- tarox_gpio.h
|   |       `-- tarox_pwm.h
|   `-- nuttx/             # NuttX 平台实现
|       |-- src/           # API 实现 (.cpp)
|       `-- cmake/         # CMake 工具链
|
|-- src/modules/           # 应用模块
|   |-- led_demo/          # LED 控制示例
|   `-- pwm_demo/         # PWM/舵机控制示例
|
`-- cmake/                 # CMake辅助模块
```

---

## 3. 编译指南

### 基本命令

```bash
# 构建默认配置 (demo)
make

# 构建指定配置
make demo

# 构建并烧录 (ST-Link)
make demo flash

# 构建并烧录 (OpenOCD)
make demo flash-openocd

# 清理构建
make clean

# 列出所有可用配置
make list
```

### 构建输出

编译产物位于 `build/<config>/` 目录：

- `tarox.bin` - 可直接烧录的固件文件
- `tarox.elf` - 调试文件

### 环境变量

| 变量 | 说明 |
|------|------|
| `TAROX_CMAKE_BUILD_TYPE` | `Debug` 或 `Release` |
| `VERBOSE=1` | 详细编译输出 |
| `NO_NINJA_BUILD=1` | 使用 Make 而非 Ninja |

---

## 4. 模块使用手册

### 4.1 led_demo 模块

控制 Demo板的 LED 灯。

**NSH 命令：**

```bash
nsh> led_demo on    # 点亮 LED
nsh> led_demo off   # 熄灭 LED
```

**实现原理：**

```
led_demo_main() → tarox_gpio_open("/dev/demo_led")
                → tarox_gpio_write(fd, true/false)
                → tarox_gpio_close(fd)
```

**源码位置：** `src/modules/led_demo/led_demo.c`

---

### 4.2 pwm_demo 模块

PWM 输出控制，支持普通 PWM 和舵机模式。

**NSH 命令：**

```bash
# 启动 PWM（默认 1000Hz, 30% 占空比）
nsh> pwm_demo start

# 启动 PWM（自定义频率和占空比）
nsh> pwm_demo start 50 75 # 50Hz, 75‰ 占空比
nsh> pwm_demo start 1000 500   # 1000Hz, 50% 占空比

# 设置到中位（50Hz, 75‰，舵机中立位）
nsh> pwm_demo mid

# 启动舵机往复运动（50Hz, 25-125‰ 扫描）
nsh> pwm_demo run_servo

# 停止 PWM 输出
nsh> pwm_demo stop
```

**参数说明：**

- `freq` - 频率 (Hz)，范围无限制
- `duty_permille` - 占空比 (0-1000)，即千分比

**舵机安全范围：**

当频率在 40-60Hz（舵机常用频段）时，占空比自动限制在 25-125‰ 范围（对应 0.5ms-2.5ms 脉冲），保护舵机机械限位。

**源码位置：** `src/modules/pwm_demo/pwm_demo.c`

---

## 5. 平台 API 参考

### 5.1 GPIO API

头文件：`platforms/common/include/tarox_gpio.h`

```c
#include <tarox_gpio.h>

// 打开 GPIO 设备，返回文件描述符
int fd = tarox_gpio_open("/dev/demo_led");

// 写入值 (true=高, false=低)
tarox_gpio_write(fd, true);

// 关闭设备
tarox_gpio_close(fd);
```

**返回值：**
- 成功：文件描述符 (>= 0)
- 失败：-1

---

### 5.2 PWM API

头文件：`platforms/common/include/tarox_pwm.h`

```c
#include <tarox_pwm.h>

// 打开 PWM 设备
int fd = tarox_pwm_open("/dev/demo_pwm");

// 配置频率和占空比
// freq_hz: 频率 (Hz)
// duty_permille: 占空比 (0-1000)
tarox_pwm_apply(fd, 1000, 500); // 1000Hz, 50%

// 启动 PWM 输出
tarox_pwm_run(fd);

// 停止 PWM 输出
tarox_pwm_halt(fd);

// 关闭设备
tarox_pwm_close(fd);
```

---

## 6. 添加新模块指南

### 步骤 1：创建模块目录

```
src/modules/my_module/
|-- my_module.c        # 主函数实现
|-- my_module.h        # 头文件
`-- CMakeLists.txt     # 模块构建配置
```

### 步骤 2：编写模块代码

```c
// my_module.h
#pragma once

int my_module_main(int argc, char *argv[]);
```

```c
// my_module.c
#include "my_module.h"
#include <stdio.h>

int my_module_main(int argc, char *argv[])
{
    printf("Hello from my_module!\n");
    return 0;
}
```

### 步骤 3：编写 CMakeLists.txt

```cmake
tarox_add_module(
    MODULE my_module
    MAIN my_module_main
    STACK 2048              # 栈大小 (字节)
    PRIORITY 100            # 任务优先级 (数值越小优先级越高)
    SRCS
        my_module.c
    DEPENDS
        tarox_platform
)
```

### 步骤 4：在配置中启用

编辑 `boards/demo/default.taroxconfig`，添加：

```
CONFIG_MODULES_MY_MODULE=y
```

### 步骤 5：重新编译

```bash
make clean
make demo
```

---

## 7. 添加新 GPIO/PWM 设备

### 添加 GPIO 设备

编辑 `boards/demo/src/stm32_gpio.c`：

```c
// 1. 定义 GPIO 引脚
static const struct gpio_pin_s g_demo_output = {
    .pin = GPIO_DEMO_OUT, // 定义在 board.h 中
    .mode = GPIO_OUTPUT_FLOAT
};

// 2. 注册设备
ret = gpio_pin_register(g_gpio, "/dev/demo_output", &g_demo_output);
```

### 添加 PWM 设备

编辑 `boards/demo/src/stm32_pwm.c`：

```c
// 1. 定义 PWM 配置
static struct stm32_pwmdev_s g_demo_pwm = {
    .pwm   = 4,              // TIM4
    .ch = 2,              // Channel 2
    .pin = GPIO_TIM4_CH2,  // 引脚定义
    .freq = 1000            // 默认频率
};

// 2. 注册设备
ret = stm32_pwmdev_register(&g_demo_pwm, "/dev/demo_pwm");
```

---

## 8. 配置说明

### .taroxconfig 格式

```bash
# 平台选择
CONFIG_PLATFORM_NUTTX=y

# 工具链
CONFIG_BOARD_TOOLCHAIN="arm-none-eabi"

# 架构
CONFIG_BOARD_ARCHITECTURE="cortex-m4"

# NuttX 配置名称
CONFIG_NUTTX_CONFIG="nsh"

#启用模块
CONFIG_MODULES_LED_DEMO=y
CONFIG_MODULES_PWM_DEMO=y
```

### tarox_add_module 参数

| 参数 |必填 | 说明 | 默认值 |
|------|------|------|--------|
| `MODULE` | 是 | 模块名称 | - |
| `MAIN` | 是 | 主函数名 | - |
| `SRCS` | 是 | 源文件列表 | - |
| `STACK` | 否 | 栈大小 (字节) | 2048 |
| `PRIORITY` | 否 | 任务优先级 | 100 |
| `DEPENDS` | 否 | 依赖库 | - |

---

## 9. 架构图

```
+------------------+ +------------------+
|   User/NSH CLI   |     |  Application |
+------------------+     |  Modules |
         |               |  (led_demo,     |
         |               |   pwm_demo)     |
         v +--------+---------+
+--------+---------+              |
|  tarox_gpio API  |              |
|  tarox_pwm API   |              |
+--------+---------+              |
         |               +--------v---------+
         +------+-------->  tarox_nuttx    |
                |        library           |
                | (tarox_gpio.cpp,       |
                |   tarox_pwm.cpp)        |
                |                +--------v---------+
                +------+-------->  NuttX Drivers |
                         |  (GPIO/PWM Framework)    |
                         |                +--------v---------+
                         +------+-------->  STM32 HAL       |
                                  |  (GPIO/PWM Hardware)    |
                                  |                +--------v---------+
                                  +------+-------->  STM32F407ZG |
                                           +------------------+
```

---

## 10. 常见问题

**Q: 编译报错 "ninja not found"?**
A: 安装 Ninja 或使用 `NO_NINJA_BUILD=1` 切换到 Make

**Q: 如何调试?**
A: 使用 `make VERBOSE=1` 查看完整编译命令，固件 `.elf` 文件可用于 GDB调试

**Q: 如何添加外部模块?**
A: 设置 `EXTERNAL_MODULES_LOCATION` 环境变量指向模块目录