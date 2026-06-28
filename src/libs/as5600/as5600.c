#include <as5600/as5600.h>

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include <nuttx/i2c/i2c_master.h>

#define AS5600_I2C_ADDR        0x36u
#define AS5600_REG_ANGLE_H     0x0cu
#define AS5600_I2C_HZ          400000
#define AS5600_TWO_PI          6.283185307179586f

static int      g_i2c_fd = -1;

static int as5600_transfer(uint8_t reg, uint8_t *buf, size_t len, bool read)
{
  struct i2c_msg_s msgs[2];
  struct i2c_transfer_s xfer;
  int ret;

  if (g_i2c_fd < 0)
    {
      return -ENODEV;
    }

  msgs[0].frequency = AS5600_I2C_HZ;
  msgs[0].addr      = AS5600_I2C_ADDR;
  msgs[0].flags     = 0;
  msgs[0].buffer    = &reg;
  msgs[0].length    = 1;

  msgs[1].frequency = AS5600_I2C_HZ;
  msgs[1].addr      = AS5600_I2C_ADDR;
  msgs[1].flags     = read ? I2C_M_READ : 0;
  msgs[1].buffer    = buf;
  msgs[1].length    = (ssize_t)len;

  xfer.msgv = msgs;
  xfer.msgc = 2;

  ret = ioctl(g_i2c_fd, I2CIOC_TRANSFER, (unsigned long)(uintptr_t)&xfer);
  if (ret < 0)
    {
      return -EIO;
    }

  return 0;
}

int as5600_init(const char *i2c_dev)
{
  if (g_i2c_fd >= 0)
    {
      return 0;
    }

  if (i2c_dev == NULL)
    {
      return -EINVAL;
    }

  g_i2c_fd = open(i2c_dev, O_RDWR);
  if (g_i2c_fd < 0)
    {
      return -ENODEV;
    }

  return 0;
}

void as5600_deinit(void)
{
  if (g_i2c_fd >= 0)
    {
      close(g_i2c_fd);
      g_i2c_fd = -1;
    }
}

bool as5600_is_ready(void)
{
  return g_i2c_fd >= 0;
}

int as5600_read_raw(uint16_t *raw)
{
  uint8_t buf[2];
  int ret;

  if (raw == NULL)
    {
      return -EINVAL;
    }

  ret = as5600_transfer(AS5600_REG_ANGLE_H, buf, sizeof(buf), true);
  if (ret < 0)
    {
      return ret;
    }

  *raw = (uint16_t)(((uint16_t)(buf[0] & 0x0fu) << 8) | buf[1]);
  return 0;
}

float as5600_raw_to_mech_rad(uint16_t raw)
{
  return ((float)raw / (float)AS5600_RAW_MAX) * AS5600_TWO_PI;
}

float as5600_raw_to_elec_rad(uint16_t raw, unsigned int pole_pairs,
                             float offset_rad)
{
  float theta = as5600_raw_to_mech_rad(raw) * (float)pole_pairs + offset_rad;

  while (theta >= AS5600_TWO_PI)
    {
      theta -= AS5600_TWO_PI;
    }

  while (theta < 0.0f)
    {
      theta += AS5600_TWO_PI;
    }

  return theta;
}
