/*
 * system_conf.h
 *
 *  Created on: 19.03.21
 *      Author: jsteggall
 */

#ifndef SYSTEM_CONF_H_
#define SYSTEM_CONF_H_

#define TIMER_OSLOOP				TIM6
#define OSLOOP_TIMER_IRQHandler		TIM6_IRQHandler

#define IODRV_PIN_IO1				0u
#define IODRV_PIN_IO1_GPIO			GPIOA
#define IODRV_PIN_IO1_GPIO_PIN_Pos	7u
#define IODRV_PIN_IO1_ADC_CHANNEL	ANALOG_CHANNEL_IO1

#define IODRV_PIN_IO2				1u
#define IODRV_PIN_IO2_GPIO			GPIOA
#define IODRV_PIN_IO2_GPIO_PIN_Pos	8u

#define IODRV_PIN_SW1				2u
#define IODRV_PIN_SW1_GPIO			GPIOB
#define IODRV_PIN_SW1_GPIO_PIN_Pos	12u

#define IODRV_PIN_SW2				3u
#define IODRV_PIN_SW2_GPIO			GPIOC
#define IODRV_PIN_SW2_GPIO_PIN_Pos	13u

#define IODRV_PIN_SW3				4u
#define IODRV_PIN_SW3_GPIO			GPIOB
#define IODRV_PIN_SW3_GPIO_PIN_Pos	2u

#define IODRV_MAX_IO_PINS			5u

#define ANALOG_CHANNEL_CS1			0u
#define ANALOG_CHANNEL_CS2			1u
#define ANALOG_CHANNEL_VBAT			2u
#define ANALOG_CHANNEL_NTC			3u
#define ANALOG_CHANNEL_POW_DET		4u
#define ANALOG_CHANNEL_BATTYPE		5u
#define ANALOG_CHANNEL_IO1			6u
#define ANALOG_CHANNEL_MPUTEMP		7u
#define ANALOG_CHANNEL_INTREF		8u

#define MAX_ANALOG_CHANNELS			9u

#define TEMP30_CAL_ADDR 			((uint16_t*)((uint32_t)0x1FFFF7B8u))
#define VREFINT_CAL_ADDR 			((uint16_t*)((uint32_t)0x1FFFF7BAu))
#define ADC_TO_MV_K					52800u
#define ADC_TO_BATTMV_K				72547u

#define IODRV_PIN_DEBOUNCE_COUNT	5u

#define IODRV_PIN_UPDATE_PERIOD_MS	10u
#define ADC_SAMPLE_PERIOD_MS		50u


#endif /* SYSTEM_CONF_H_ */
