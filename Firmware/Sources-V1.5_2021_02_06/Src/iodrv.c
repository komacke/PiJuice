// ----------------------------------------------------------------------------
/*!
 * @file		iodrv.c
 * @author    	John Steggall
 * @date       	19 March 2021
 * @brief       Handles the GPIO functions and monitors pins for digital changes
 * 				where the pins are configured as such. The module uses the actual
 * 				pin settings so no need to configure in this module if already set
 * 				by cubemx or using the hal drivers. As long as there is an entry
 * 				in the array the module will monitor it. Configuration options
 * 				allow for inverted operation and digital inputs are debounced.
 *
 */
// ----------------------------------------------------------------------------
// Include section - add all #includes here:

#include <main.h>

#include "system_conf.h"
#include "ave_filter.h"
#include "time_count.h"
#include "adc.h"
#include "util.h"

#include "iodrv.h"

// ----------------------------------------------------------------------------
// Defines section - add all #defines here:

#define PINMODE_INPUT				0u
#define PINMODE_OUTPUT				1u
#define PINMODE_ALTERNATE			2u
#define PINMODE_ANALOG				3u


// ----------------------------------------------------------------------------
// Function prototypes for functions that only have scope in this module:

static void IODRV_UpdatePins(const uint32_t sysTime);


// ----------------------------------------------------------------------------
// Variables that have scope from outside this module:


// ----------------------------------------------------------------------------
// Variables that only have scope in this module:

static IODRV_Pin_t m_pins[IODRV_MAX_IO_PINS] =
{
		{
				.adcChannel = IODRV_PIN_IO1_ADC_CHANNEL,
				.gpioPin_bm = (1u << IODRV_PIN_IO1_GPIO_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_IO1_GPIO_PIN_Pos,
				.gpioPort = IODRV_PIN_IO1_GPIO,
				.canConfigure = true,
				.invert_bm = IODRV_PIN_IO1_INVERT_bm,
				.analogConversionFactor = ADC_TO_MV_K,
				.index = IODRV_PIN_IO1
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_IO2_GPIO_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_IO2_GPIO_PIN_Pos,
				.gpioPort = IODRV_PIN_IO2_GPIO,
				.invert_bm = IODRV_PIN_IO2_INVERT_bm,
				.canConfigure = true,
				.index = IODRV_PIN_IO2
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_SW1_GPIO_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_SW1_GPIO_PIN_Pos,
				.gpioPort = IODRV_PIN_SW1_GPIO,
				.invert_bm = IODRV_PIN_SW1_INVERT_bm,
				.canConfigure = false,
				.index = IODRV_PIN_SW1
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_SW2_GPIO_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_SW2_GPIO_PIN_Pos,
				.gpioPort = IODRV_PIN_SW2_GPIO,
				.invert_bm = IODRV_PIN_SW2_INVERT_bm,
				.canConfigure = false,
				.index = IODRV_PIN_SW2
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_SW3_GPIO_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_SW3_GPIO_PIN_Pos,
				.gpioPort = IODRV_PIN_SW3_GPIO,
				.invert_bm = IODRV_PIN_SW3_INVERT_bm,
				.canConfigure = false,
				.index = IODRV_PIN_SW3
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_POWDET_EN_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_POWDET_EN_PIN_Pos,
				.gpioPort = IODRV_PIN_POWDET_EN_GPIO,
				.invert_bm = IODRV_PIN_POWDET_EN_INVERT_bm,
				.canConfigure = false,
				.index = IODRV_PIN_POWDET_EN
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_POW_EN_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_POW_EN_PIN_Pos,
				.gpioPort = IODRV_PIN_POW_EN_GPIO,
				.invert_bm = IODRV_PIN_POW_EN_INVERT_bm,
				.canConfigure = true,
				.index = IODRV_PIN_POW_EN
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_EXTVS_EN_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_EXTVS_EN_PIN_Pos,
				.gpioPort = IODRV_PIN_EXTVS_EN_GPIO,
				.invert_bm = IODRV_PIN_EXTVS_INVERT_bm,
				.canConfigure = true,
				.index = IODRV_PIN_EXTVS_EN
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_TS_CTR1_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_TS_CTR1_PIN_Pos,
				.gpioPort = IODRV_PIN_TS_CTR1_GPIO,
				.invert_bm = IODRV_PIN_TS_CTR1_INVERT_bm,
				.canConfigure = false,
				.index = IODRV_PIN_TS_CTR1
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_TS_CTR2_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_TS_CTR2_PIN_Pos,
				.gpioPort = IODRV_PIN_TS_CTR2_GPIO,
				.invert_bm = IODRV_PIN_TS_CTR2_INVERT_bm,
				.canConfigure = false,
				.index = IODRV_PIN_TS_CTR2
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_CH_INT_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_CH_INT_PIN_Pos,
				.gpioPort = IODRV_PIN_CH_INT_GPIO,
				.invert_bm = IODRV_PIN_CH_INT_INVERT_bm,
				.canConfigure = false,
				.index = IODRV_PIN_CH_INT
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_ESYSLIM_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_ESYSLIM_PIN_Pos,
				.gpioPort = IODRV_PIN_ESYSLIM_GPIO,
				.invert_bm = IODRV_PIN_ESYSLIM_INVERT_bm,
				.canConfigure = false,
				.index = IODRV_PIN_ESYSLIM
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_BGINT_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_BGINT_PIN_Pos,
				.gpioPort = IODRV_PIN_BGINT_GPIO,
				.invert_bm = IODRV_PIN_BGINT_INVERT_bm,
				.canConfigure = false,
				.index = IODRV_PIN_BGINT
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_EE_A_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_EE_A_PIN_Pos,
				.gpioPort = IODRV_PIN_EE_A_GPIO,
				.invert_bm = IODRV_PIN_EE_A_INVERT_bm,
				.canConfigure = false,
				.index = IODRV_PIN_EE_A
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_EE_WP_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_EE_WP_PIN_Pos,
				.gpioPort = IODRV_PIN_EE_WP_GPIO,
				.invert_bm = IODRV_PIN_EE_WP_INVERT_bm,
				.canConfigure = false,
				.index = IODRV_PIN_EE_WP
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_RUN_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_RUN_PIN_Pos,
				.gpioPort = IODRV_PIN_RUN_GPIO,
				.invert_bm = IODRV_PIN_RUN_INVERT_bm,
				.canConfigure = false,
				.index = IODRV_PIN_RUN
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_LED1_R_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_LED1_R_PIN_Pos,
				.gpioPort = IODRV_PIN_LED1_R_GPIO,
				.invert_bm = IODRV_PIN_LED1_R_INVERT_bm,
				.canConfigure = true,
				.index = IODRV_PIN_LED1_R
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_LED1_G_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_LED1_G_PIN_Pos,
				.gpioPort = IODRV_PIN_LED1_G_GPIO,
				.invert_bm = IODRV_PIN_LED1_G_INVERT_bm,
				.canConfigure = true,
				.index = IODRV_PIN_LED1_G
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_LED1_B_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_LED1_B_PIN_Pos,
				.gpioPort = IODRV_PIN_LED1_B_GPIO,
				.invert_bm = IODRV_PIN_LED1_G_INVERT_bm,
				.canConfigure = true,
				.index = IODRV_PIN_LED1_B
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_LED2_R_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_LED2_R_PIN_Pos,
				.gpioPort = IODRV_PIN_LED2_R_GPIO,
				.invert_bm = IODRV_PIN_LED2_R_INVERT_bm,
				.canConfigure = true,
				.index = IODRV_PIN_LED2_R
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_LED2_G_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_LED2_G_PIN_Pos,
				.gpioPort = IODRV_PIN_LED2_G_GPIO,
				.invert_bm = IODRV_PIN_LED2_G_INVERT_bm,
				.canConfigure = true,
				.index = IODRV_PIN_LED2_G
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_LED2_B_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_LED2_B_PIN_Pos,
				.gpioPort = IODRV_PIN_LED2_B_GPIO,
				.invert_bm = IODRV_PIN_LED2_B_INVERT_bm,
				.canConfigure = true,
				.index = IODRV_PIN_LED2_B
		},
		{
				.adcChannel = MAX_ANALOG_CHANNELS,
				.gpioPin_bm = (1u << IODRV_PIN_RPI_GPIO3_PIN_Pos),
				.gpioPin_pos = IODRV_PIN_RPI_GPIO3_PIN_Pos,
				.gpioPort = IODRV_PIN_RPI_GPIO3_GPIO,
				.invert_bm = IODRV_PIN_RPI_GPIO3_PIN_INVERT_bm,
				.canConfigure = true,
				.index = IODRV_PIN_RPI_GPIO3
		}
};

static uint32_t m_lastPinUpdateTime;


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH GLOBAL SCOPE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ****************************************************************************
/*!
 * IODRV_Init configures the module to a known initial state
 * @param	sysTime		current value of the ms tick timer
 * @retval	none
 */
// ****************************************************************************
void IODRV_Init(const uint32_t sysTime)
{
	uint8_t i;

	for (i = 0u; i < IODRV_MAX_IO_PINS; i++)
	{
		m_pins[i].value = 0u;
	}

	MS_TIMEREF_INIT(m_lastPinUpdateTime, sysTime);
}


void IODRV_Shutdown(void)
{

}


// ****************************************************************************
/*!
 * IODRV_Service performs periodic updates for this module
 *
 * @param	sysTime		current value of the ms tick timer
 * @retval	none
 */
// ****************************************************************************
void IODRV_Service(const uint32_t sysTime)
{
	if (MS_TIMEREF_TIMEOUT(m_lastPinUpdateTime, sysTime, IODRV_PIN_UPDATE_PERIOD_MS))
	{
		IODRV_UpdatePins(sysTime);
	}
}


// ****************************************************************************
/*!
 * IODRV_ReadPin returns the value of the pin after it has been processed by the
 * average filter, could be analog (0..4095) or digital (0..1). The caller is
 * expected to know!!
 *
 * @param	pin			index of the pin that is required
 * @retval	uint16_t	value of the pin that is required
 */
// ****************************************************************************
uint16_t IODRV_ReadPinValue(const uint8_t pin)
{
	if ( pin >= IODRV_MAX_IO_PINS )
	{
		return 0u;
	}

	return m_pins[pin].value;
}


// ****************************************************************************
/*!
 * IODRV_ReadPinOutputState returns the value of the output data register, giving
 * the expected drive level (could still be pulled down).
 *
 * Note: this ignores the invert bitmap.
 *
 * @param	pin			index of the pin that is required
 * @retval	uint8_t		value of the pin that is required
 */
// ****************************************************************************
bool IODRV_ReadPinOutputState(const uint8_t pin)
{
	if ( pin >= IODRV_MAX_IO_PINS )
	{
		return 0u;
	}

	return (m_pins[pin].gpioPort->ODR & m_pins[pin].gpioPin_bm) != 0u;
}


// ****************************************************************************
/*!
 * IODRV_WritePin sets the digital output value of an io pin, checks to make sure
 * the pin is in bounds and the value is in bounds. Will return false if the pin
 * is not a digital output.
 *
 * @param	pin			index of the pin that is required
 * @param	newValue	value to be set, maps to GPIO_PIN_SET and GPIO_RESET
 * @retval	bool		returns true is the pin is an output, false if an input
 */
// ****************************************************************************
bool IODRV_WritePin(const uint8_t pin, const bool newValue)
{
	if ( (m_pins[pin].pinType == IOTYPE_DIGOUT_PUSHPULL) || (m_pins[pin].pinType <= IOTYPE_DIGOUT_OPENDRAIN) )
	{
		return IODRV_SetPin(pin, newValue);
	}

	return false;
}


// ****************************************************************************
/*!
 * IODRV_SetPin sets the digital output value of an io pin, checks to make sure
 * the pin is in bounds and the value is in bounds.
 *
 * @param	pin			index of the pin that is required
 * @param	newValue	value to be set, maps to GPIO_PIN_SET and GPIO_RESET
 * @retval	bool		returns true if the arguments are correct
 */
// ****************************************************************************
bool IODRV_SetPin(const uint8_t pin, const bool newValue)
{
	const GPIO_PinState outVal = (newValue ^ m_pins[pin].invert_bm);

	if ( (pin >= IODRV_MAX_IO_PINS) )
	{
		return false;
	}

	HAL_GPIO_WritePin(m_pins[pin].gpioPort, m_pins[pin].gpioPin_bm, outVal);

	return true;
}


// ****************************************************************************
/*!
 * IODRV_SetPinType sets the required operation for the io pin, this can be analog,
 * digital, or PWM. The output and PWM modes are push pull or open drain. The PWM
 * must be configured separately.
 *
 * @param	pin			index of the pin that is required to be set
 * @param	newType		type of io to be configured
 * @retval	bool		true if the pin has been set
 */
// ****************************************************************************
bool IODRV_SetPinType(const uint8_t pin, const IODRV_PinType_t newType)
{
	const uint32_t otyper_bm = (1u << m_pins[pin].gpioPin_pos);
	const uint32_t moder_pos = (m_pins[pin].gpioPin_pos * 2u);
	const uint32_t ospeed_pos = (m_pins[pin].gpioPin_pos * 2u);

	if ( (pin >= IODRV_MAX_IO_PINS) || (false == m_pins[pin].canConfigure) )
	{
		return false;
	}

	m_pins[pin].gpioPort->MODER &= ~(3u << moder_pos);
	m_pins[pin].gpioPort->OTYPER &= ~(otyper_bm);
	m_pins[pin].gpioPort->OSPEEDR &= ~(3u << ospeed_pos);

	switch (newType)
	{
	case IOTYPE_DIGOUT_OPENDRAIN:
		m_pins[pin].gpioPort->OTYPER |= otyper_bm;
	case IOTYPE_DIGOUT_PUSHPULL:
		m_pins[pin].gpioPort->MODER |= (PINMODE_OUTPUT << moder_pos);
		break;

	case IOTYPE_PWM_OPENDRAIN:
		m_pins[pin].gpioPort->OTYPER |= otyper_bm;
	case IOTYPE_PWM_PUSHPULL:
		// Set output pin to be high speed
		m_pins[pin].gpioPort->OSPEEDR |= (3u << ospeed_pos);
		m_pins[pin].gpioPort->MODER |= (PINMODE_ALTERNATE << moder_pos);
		break;

	case IOTYPE_ANALOG:
		m_pins[pin].gpioPort->MODER |= (PINMODE_ANALOG << moder_pos);
		break;

	default:
		break;
	}

	return true;

}


// ****************************************************************************
/*!
 * IODRV_SetPinPullDir sets the required pull direction for the io pin, this can be
 * up, down or none.
 *
 * @param	pin				index of the pin that is required to be set
 * @param	pullDirection	direction the pin is to be pulled:
 * 							GPIO_NOPULL, GPIO_PULLUP, GPIO_PULLDOWN
 * @retval	bool			true if the pin has been set
 */
// ****************************************************************************
bool IODRV_SetPinPullDir(const uint8_t pin, const uint32_t pullDirection)
{
	const uint32_t pupdr_pos = (m_pins[pin].gpioPin_pos * 2u);

	if ( (pin >= IODRV_MAX_IO_PINS) || (false == m_pins[pin].canConfigure) || (pullDirection > 2u))
	{
		return false;
	}

	m_pins[pin].gpioPort->PUPDR &= ~(3u << pupdr_pos);
	m_pins[pin].gpioPort->PUPDR |= (pullDirection << pupdr_pos);

	return true;
}


// ****************************************************************************
/*!
 * IODRV_GetPinInfo gets a const pointer to the pin informationd
 *
 * @param	pin				index of the pin that is required to be set
 * @retval	IODRV_Pin_t*	const pointer to the pin data, NULL if pin not valid
 */
// ****************************************************************************
const IODRV_Pin_t * IODRV_GetPinInfo(const uint8_t pin)
{
	if ( (pin >= IODRV_MAX_IO_PINS) )
	{
		return NULL;
	}

	return &m_pins[pin];
}


void IORDV_ClearPinEdges(const uint8_t pinIdx)
{
	m_pins[pinIdx].lastNegPulseWidthTimeMs = 0u;
	m_pins[pinIdx].lastPosPulseWidthTimeMs = 0u;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// FUNCTIONS WITH LOCAL SCOPE
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ****************************************************************************
/*!
 * IODRV_UpdatePins updates the value for each pin
 *
 * @param	none
 * @retval	none
 */
// ****************************************************************************
static void IODRV_UpdatePins(const uint32_t sysTime)
{
	uint8_t pin;
	uint16_t value = 0u;
	uint8_t pinType;
	uint32_t moder_pos;

	for (pin = 0u; pin < IODRV_MAX_IO_PINS; pin++)
	{
		moder_pos = (m_pins[pin].gpioPin_pos * 2u);
		pinType = (m_pins[pin].gpioPort->MODER >> moder_pos) & 0x03u;

		if (pinType == PINMODE_ANALOG)
		{
			// Read analog pin value, will be 0 if not connected to the ADC
			value = UTIL_FixMul_U32_U16(
					m_pins[pin].analogConversionFactor,
					ADC_CalibrateValue(ADC_GetAverageValue(m_pins[pin].adcChannel))
					);
		}
		else if (pinType == PINMODE_ALTERNATE)
		{
			value = 0u;
		}
		else // Must be digital then
		{
			value = m_pins[pin].value;

			if (GPIO_PIN_SET == HAL_GPIO_ReadPin(m_pins[pin].gpioPort, m_pins[pin].gpioPin_bm))
			{
				if (m_pins[pin].debounceCounter < IODRV_PIN_DEBOUNCE_COUNT)
				{
					m_pins[pin].debounceCounter++;
				}
				else if (m_pins[pin].value != GPIO_PIN_SET)
				{
					/* log positive pulse width */
					m_pins[pin].lastNegPulseWidthTimeMs = MS_TIMEREF_DIFF(m_pins[pin].lastDigitalChangeTime, sysTime);

					/* log the change time */
					MS_TIMEREF_INIT(m_pins[pin].lastDigitalChangeTime, sysTime);

					value = GPIO_PIN_SET;
				}
				else
				{
					// Value already set
				}
			}
			else
			{
				if (m_pins[pin].debounceCounter > 0u)
				{
					m_pins[pin].debounceCounter--;
				}
				else if (m_pins[pin].value != GPIO_PIN_RESET)
				{
					/* log negative pulse width */
					m_pins[pin].lastPosPulseWidthTimeMs = MS_TIMEREF_DIFF(m_pins[pin].lastDigitalChangeTime, sysTime);

					/* log the change time */
					MS_TIMEREF_INIT(m_pins[pin].lastDigitalChangeTime, sysTime);

					value = GPIO_PIN_RESET;
				}
				else
				{
					// Value already set
				}
			}

			// Clear pulse widths after one day, button routine might catch uint32_t timeref roll over and cause havok!
			if (MS_TIMEREF_TIMEOUT(m_pins[pin].lastDigitalChangeTime, sysTime, MS_ONE_DAY))
			{
				m_pins[pin].lastNegPulseWidthTimeMs = 0u;
				m_pins[pin].lastPosPulseWidthTimeMs = 0u;
			}
		}

		m_pins[pin].value = (value ^ m_pins[pin].invert_bm);
	}
}

