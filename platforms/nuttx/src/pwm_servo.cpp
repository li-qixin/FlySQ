#include <tarox/drv_pwm_output.h>

#include <board.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <fixedmath.h>
#include <nuttx/timers/pwm.h>

extern "C"
{

static int g_pwm_fd = -1;
static uint32_t g_pwm_freq_hz;
static uint32_t g_pwm_channel_count;

static ub16_t permille_to_driver_duty(uint32_t permille)
{
	if (permille == 0) {
		return 1;
	}

	return b16divi(uitoub16(permille), 1000);
}

static ub16_t float_to_driver_duty(float duty)
{
	if (duty <= 0.0f) {
		return 1;
	}

	if (duty >= 1.0f) {
		return b16ONE;
	}

	return (ub16_t)(duty * 65536.0f);
}

static void pwm_set_channel(struct pwm_info_s *info, uint32_t channel, uint32_t duty)
{
#ifdef CONFIG_PWM_MULTICHAN
	info->channels[0].channel = channel;
	info->channels[0].duty    = duty;
	info->channels[0].cpol    = PWM_CPOL_LOW;
	info->channels[0].dcpol   = PWM_DCPOL_LOW;
#else
	(void)channel;
	info->duty  = duty;
	info->cpol  = PWM_CPOL_LOW;
	info->dcpol = PWM_DCPOL_LOW;
#endif
}

static void pwm_set_3channels(struct pwm_info_s *info,
                              float duty_u, float duty_v, float duty_w)
{
#ifdef CONFIG_PWM_MULTICHAN
	info->channels[0].channel = 1;
	info->channels[0].duty    = float_to_driver_duty(duty_u);
	info->channels[0].cpol    = PWM_CPOL_LOW;
	info->channels[0].dcpol   = PWM_DCPOL_LOW;

	info->channels[1].channel = 2;
	info->channels[1].duty    = float_to_driver_duty(duty_v);
	info->channels[1].cpol    = PWM_CPOL_LOW;
	info->channels[1].dcpol   = PWM_DCPOL_LOW;

	info->channels[2].channel = 3;
	info->channels[2].duty    = float_to_driver_duty(duty_w);
	info->channels[2].cpol    = PWM_CPOL_LOW;
	info->channels[2].dcpol   = PWM_DCPOL_LOW;
#else
	(void)duty_u;
	(void)duty_v;
	(void)duty_w;
#endif
}

int tarox_pwm_servo_init(uint32_t freq_hz, uint32_t channel_count)
{
	if (channel_count == 0 || channel_count > TAROX_PWM_OUTPUT_MAX_CHANNELS) {
		return -EINVAL;
	}

	if (g_pwm_fd >= 0) {
		return 0;
	}

	g_pwm_fd = open(TAROX_BLDC_PWM, O_RDONLY);

	if (g_pwm_fd < 0) {
		return g_pwm_fd;
	}

	g_pwm_freq_hz = freq_hz;
	g_pwm_channel_count = channel_count;

	if (channel_count >= 3) {
		return tarox_pwm_duty_set3(0.5f, 0.5f, 0.5f);
	}

	return 0;
}

int tarox_pwm_servo_deinit(void)
{
	if (g_pwm_fd >= 0) {
		ioctl(g_pwm_fd, PWMIOC_STOP, 0);
		close(g_pwm_fd);
	}

	g_pwm_fd = -1;
	g_pwm_freq_hz = 0;
	g_pwm_channel_count = 0;
	return 0;
}

int tarox_pwm_servo_arm(void)
{
	if (g_pwm_fd < 0) {
		return -EINVAL;
	}

	return ioctl(g_pwm_fd, PWMIOC_START, 0);
}

int tarox_pwm_servo_disarm(void)
{
	if (g_pwm_fd < 0) {
		return -EINVAL;
	}

	return ioctl(g_pwm_fd, PWMIOC_STOP, 0);
}

int tarox_pwm_duty_set(uint32_t channel, float duty)
{
	struct pwm_info_s info;

	if (g_pwm_fd < 0) {
		return -EINVAL;
	}

	if (channel >= g_pwm_channel_count) {
		return -EINVAL;
	}

	if (g_pwm_channel_count == 1) {
		uint32_t permille = (uint32_t)(duty * 1000.0f);

		if (permille > 1000) {
			return -EINVAL;
		}

		memset(&info, 0, sizeof(info));
		info.frequency = g_pwm_freq_hz;
		pwm_set_channel(&info, channel + 1, permille_to_driver_duty(permille));
		return ioctl(g_pwm_fd, PWMIOC_SETCHARACTERISTICS, (unsigned long)(uintptr_t)&info);
	}

	return -ENOTSUP;
}

int tarox_pwm_duty_set3(float duty_u, float duty_v, float duty_w)
{
	struct pwm_info_s info;

	if (g_pwm_fd < 0) {
		return -EINVAL;
	}

#ifndef CONFIG_PWM_MULTICHAN
	return -ENOTSUP;
#else
	memset(&info, 0, sizeof(info));
	info.frequency = g_pwm_freq_hz;
	pwm_set_3channels(&info, duty_u, duty_v, duty_w);
	return ioctl(g_pwm_fd, PWMIOC_SETCHARACTERISTICS, (unsigned long)(uintptr_t)&info);
#endif
}

}
