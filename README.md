# Tarox

Embedded firmware built on NuttX. The default board configuration is **demo** (STM32F407 family, etc.—see `boards/demo` for specifics).

## Dependencies (example: Ubuntu)

- Git, CMake (≥ 3.16), Ninja (recommended) or Make
- **arm-none-eabi** GCC toolchain
- Python 3 (required by configuration / build scripts)
- Flashing: **ST-Link** and **`st-flash`** ([stlink tools](https://github.com/stlink-org/stlink)); for OpenOCD-based flashing, install `openocd` (optional target `flash-openocd`)

Run `make help` for a short summary that matches the top-level Makefile.

## Build

List all board configuration names (from `boards/**/*.taroxconfig`):

```bash
make list
```

Build a specific configuration (artifacts go under `build/<config>/`):

```bash
make demo_default
```

If the configuration comes from `default.taroxconfig`, you may omit the `_default` suffix:

```bash
make demo
```

This is equivalent to building `demo_default`.

## Flashing firmware

The image is produced from the ELF as **`build/<config>/tarox.bin`**.

### ST-Link (`st-flash`, recommended)

1. Connect ST-Link and the target board over USB; confirm the host sees the debugger.
2. **Put the configuration name first**; the second goal is the flash step:

```bash
make demo flash
```

or:

```bash
make demo_default flash
```

This configures and builds if needed, then runs `st-flash write …` on `tarox.bin`. The default load address is **internal flash base `0x08000000`** (typical for STM32F4-class parts).

To change the address, reconfigure CMake in that build directory, for example:

```bash
cd build/demo_default
cmake . -DTAROX_FLASH_ADDR=0x08000000
cd ../..
make demo flash
```

Relevant cache variable: `TAROX_FLASH_ADDR`.

### OpenOCD

If **OpenOCD** is installed and CMake generated the target:

```bash
make demo flash-openocd
```

Defaults use **`interface/stlink.cfg`** and **`target/stm32f4x.cfg`**. Override in `build/<config>/` with `cmake . -D…`:

- `TAROX_OPENOCD_INTERFACE`
- `TAROX_OPENOCD_TARGET`

### Manual flash

With an existing `tarox.bin` you can run:

```bash
st-flash write build/demo_default/tarox.bin 0x08000000
```

(Use an address that matches your MCU and linker script.)

## Serial console

For the **demo** board NuttX setup, the **serial console is USART3** at **115200 8N1**. Authoritative settings are in `boards/demo/nuttx-config/nsh/defconfig` (`CONFIG_USART3_*`).

### Hardware

- **On-board ST-Link virtual COM (USB CDC)**: connect with a USB cable; on Linux the device is often **`/dev/ttyACM0`** or similar.
- **Separate USB–TTL adapter**: tie **GND** common, **cross RX/TX**; map USART3 pins from your schematic or `boards/demo/nuttx-config/include/board.h` (`GPIO_USART3_RX` / `GPIO_USART3_TX`).

### Finding the device on Linux

```bash
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
```

Compare before/after plugging in the cable, or use `dmesg | tail`, to identify the right node.

### Terminal programs

Replace **`/dev/ttyACM0`** with your actual device.

**picocom:**

```bash
picocom -b 115200 /dev/ttyACM0
```

(Common exit: `Ctrl+A`, then `Ctrl+X`.)

**screen:**

```bash
screen /dev/ttyACM0 115200
```

**minicom:**

```bash
minicom -D /dev/ttyACM0 -b 115200
```

### Permissions

If opening the serial device is denied, add your user to the **`dialout`** group and log in again:

```bash
sudo usermod -aG dialout "$USER"
```

After a successful flash, power-cycle or reset the board; you should see NuttX **NSH** (or other) boot output on that port. If there is no output, verify **115200 baud**, wiring to **USART3**, and that you selected the correct **`/dev`** node for your USB cable or adapter.
