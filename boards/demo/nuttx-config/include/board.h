#ifndef __ARCH_BOARD_BOARD_H
#define __ARCH_BOARD_BOARD_H

#include <nuttx/config.h>
#ifndef __ASSEMBLY__
# include <stdint.h>
#endif

#include <stm32.h>

/* Clocking — STM32F407 demo board, 8 MHz HSE, 168 MHz SYSCLK. */

#define STM32_BOARD_XTAL        8000000ul

#define STM32_HSI_FREQUENCY     16000000ul
#define STM32_LSI_FREQUENCY     32000
#define STM32_HSE_FREQUENCY     STM32_BOARD_XTAL

#define STM32_PLLCFG_PLLM       RCC_PLLCFG_PLLM(8)
#define STM32_PLLCFG_PLLN       RCC_PLLCFG_PLLN(336)
#define STM32_PLLCFG_PLLP       RCC_PLLCFG_PLLP_2
#define STM32_PLLCFG_PLLQ       RCC_PLLCFG_PLLQ(7)

#define STM32_SYSCLK_FREQUENCY  168000000ul

#define STM32_RCC_CFGR_HPRE     RCC_CFGR_HPRE_SYSCLK
#define STM32_HCLK_FREQUENCY    STM32_SYSCLK_FREQUENCY
#define STM32_BOARD_HCLK        STM32_HCLK_FREQUENCY

#define STM32_RCC_CFGR_PPRE1    RCC_CFGR_PPRE1_HCLKd4
#define STM32_PCLK1_FREQUENCY   (STM32_HCLK_FREQUENCY / 4)

#define STM32_APB1_TIM4_CLKIN   (2 * STM32_PCLK1_FREQUENCY)
#define BOARD_TIM4_FREQUENCY    STM32_APB1_TIM4_CLKIN

#define STM32_RCC_CFGR_PPRE2    RCC_CFGR_PPRE2_HCLKd2
#define STM32_PCLK2_FREQUENCY   (STM32_HCLK_FREQUENCY / 2)

#define STM32_APB2_TIM1_CLKIN   (2 * STM32_PCLK2_FREQUENCY)

/* USART3 — serial console (CONFIG_STM32_USART3). */

#define GPIO_USART3_RX          GPIO_USART3_RX_3
#define GPIO_USART3_TX          GPIO_USART3_TX_3

/* GPIO outputs — PA5 demo LED (CONFIG_DEV_GPIO). */

#define BOARD_GPIO_DEMO_LED       (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_50MHz | GPIO_OUTPUT_CLEAR | GPIO_PORTA | GPIO_PIN5)
#define TAROX_GPIO_DEMO_LED         "/dev/demo_led"

/* PWM outputs — TIM4 CH2 on PD13 (CONFIG_STM32_TIM4_PWM). */

#define BOARD_PWM_DEMO_TIMER      4
#define BOARD_PWM_DEMO_CHANNEL    2
#define GPIO_TIM4_CH2OUT          GPIO_TIM4_CH2OUT_2
#define BOARD_PWM_DEMO_GPIO       GPIO_TIM4_CH2OUT
#define TAROX_PWM_DEMO              "/dev/demo_pwm"

/* BLDC — TIM1 CH1/2/3 on PA8/PA9/PA10 (MS8313 INU/INV/INW). */

#define TAROX_BLDC_PWM              "/dev/bldc_pwm"
#define BOARD_BLDC_TIMER            1
#define GPIO_TIM1_CH1OUT            GPIO_TIM1_CH1OUT_1
#define GPIO_TIM1_CH2OUT            GPIO_TIM1_CH2OUT_1
#define GPIO_TIM1_CH3OUT            GPIO_TIM1_CH3OUT_1
#define BOARD_BLDC_GPIO_U           GPIO_TIM1_CH1OUT
#define BOARD_BLDC_GPIO_V           GPIO_TIM1_CH2OUT
#define BOARD_BLDC_GPIO_W           GPIO_TIM1_CH3OUT
#define BOARD_BLDC_PWM_FREQ_HZ        20000

/* MS8313 driver enable — PB6, active high. */

#define BOARD_GPIO_BLDC_EN          (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_50MHz | GPIO_OUTPUT_CLEAR | GPIO_PORTB | GPIO_PIN6)
#define TAROX_GPIO_BLDC_EN            "/dev/bldc_en"

/* I2C1 — AS5600 on PB8(SCL) / PB9(SDA). */

#define GPIO_I2C1_SCL               GPIO_I2C1_SCL_2
#define GPIO_I2C1_SDA               GPIO_I2C1_SDA_2
#define TAROX_I2C1_DEV                "/dev/i2c1"

/* BLDC closed-loop: pole pairs and AS5600 electrical angle offset (rad). */

#define BOARD_BLDC_POLE_PAIRS         7
#define BOARD_AS5600_ELEC_OFFSET_RAD  0.0f

/* FOC Vq sign: flip to -1 if speed loop drives the wrong way. */

#define BOARD_BLDC_VQ_SIGN              (-1)

#endif /* __ARCH_BOARD_BOARD_H */
