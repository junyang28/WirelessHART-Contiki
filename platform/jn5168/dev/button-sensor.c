#include "contiki.h"
#include "lib/sensors.h"
#include "dev/button-sensor.h"
#include <AppHardwareApi.h>
#include "dev/button-sensor.h"
#include "Button.h"
#include "dev/leds.h"

#define BUTTON_MAP_DR1199

#if (defined BUTTON_MAP_DR1199)

	typedef enum {
		APP_E_BUTTONS_BUTTON_1 = 0,
		APP_E_BUTTONS_BUTTON_SW4,
		APP_E_BUTTONS_BUTTON_SW3,
		APP_E_BUTTONS_BUTTON_SW2,
		APP_E_BUTTONS_BUTTON_SW1
	} APP_teButtons;

    #define APP_BUTTONS_NUM             (5UL)
    #define APP_BUTTONS_BUTTON_1        (8)
	#define APP_BUTTONS_BUTTON_SW4		(1)
	#define APP_BUTTONS_BUTTON_SW3		(11)
	#define APP_BUTTONS_BUTTON_SW2		(12)
	#define APP_BUTTONS_BUTTON_SW1		(17)
    #define APP_BUTTONS_DIO_MASK        ((1 << APP_BUTTONS_BUTTON_1)|(1 << APP_BUTTONS_BUTTON_SW4) | (1 << APP_BUTTONS_BUTTON_SW3) | (1 << APP_BUTTONS_BUTTON_SW2) | (1 << APP_BUTTONS_BUTTON_SW1))
	#define APP_BUTTONS_DIO_MASK_FOR_DEEP_SLEEP        ((1 << APP_BUTTONS_BUTTON_SW4) | (1 << APP_BUTTONS_BUTTON_SW3) | (1 << APP_BUTTONS_BUTTON_SW2) | (1 << APP_BUTTONS_BUTTON_SW1))

#else
	typedef enum {
		APP_E_BUTTONS_BUTTON_1 = 0,
		APP_E_BUTTONS_BUTTON_2,
		APP_E_BUTTONS_BUTTON_3
	} APP_teButtons;

    #define APP_BUTTONS_NUM             (3UL)
		#define APP_BUTTONS_BUTTON_3        (8)
		#define APP_BUTTONS_BUTTON_1        (9)
    #define APP_BUTTONS_BUTTON_2        (10)
    #define APP_BUTTONS_DIO_MASK        ( (1 << APP_BUTTONS_BUTTON_3) | (1 << APP_BUTTONS_BUTTON_1) | (1 << APP_BUTTONS_BUTTON_2) )
#endif

#if (defined BUTTON_MAP_DR1199)
    PRIVATE uint8 s_u8ButtonDebounce[APP_BUTTONS_NUM] = { 0xff,0xff,0xff,0xff,0xff };
    PRIVATE uint8 s_u8ButtonState[APP_BUTTONS_NUM] = { 0,0,0,0,0 };
    PRIVATE const uint8 s_u8ButtonDIOLine[APP_BUTTONS_NUM] =
    {
        APP_BUTTONS_BUTTON_1,
        APP_BUTTONS_BUTTON_SW4,
        APP_BUTTONS_BUTTON_SW3,
        APP_BUTTONS_BUTTON_SW2,
        APP_BUTTONS_BUTTON_SW1

    };
#else
    PRIVATE uint8 s_u8ButtonDebounce[APP_BUTTONS_NUM] = { 0xff, 0xff, 0xff };
    PRIVATE uint8 s_u8ButtonState[APP_BUTTONS_NUM] = { 0, 0, 0 };
    PRIVATE const uint8 s_u8ButtonDIOLine[APP_BUTTONS_NUM] =
    {
    		APP_BUTTONS_BUTTON_1,
        APP_BUTTONS_BUTTON_2,
    		APP_BUTTONS_BUTTON_3
    };
#endif

#define BUTTONS_ENABELED_CONF 1

#ifdef BUTTONS_ENABELED_CONF
#define BUTTONS_ENABELED BUTTONS_ENABELED_CONF
#else
#define BUTTONS_ENABELED 0
#endif

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

const struct sensors_sensor button_sensor;
static uint32_t volatile irq_val=0,
                             val=0;
#define BUTTON0_DIO (BUTTON_0_PIN)
#define BUTTONS_DIO BUTTON0_DIO

#define BUTTON0_INT E_AHI_DIO8_INT

PROCESS(debounce_process, "button debounce");
#define DEBOUNCE_TIME (CLOCK_SECOND/20)
volatile static uint8_t button_enabled_status = 0;
/****************************************************************************
 *
 * NAME: APP_bButtonInitialise
 *
 * DESCRIPTION:
 * Button Initialization
 *
 * PARAMETER: void
 *
 * RETURNS: bool
 *
 ****************************************************************************/
static uint8_t APP_bButtonInitialise(void)
{
    /* Set DIO lines to inputs with buttons connected */
    vAHI_DioSetDirection(APP_BUTTONS_DIO_MASK, 0);

    /* Turn on pull-ups for DIO lines with buttons connected */
    vAHI_DioSetPullup(APP_BUTTONS_DIO_MASK, 0);

    /* Set the edge detection for falling edges */
    vAHI_DioInterruptEdge(0, APP_BUTTONS_DIO_MASK);

    /* Enable interrupts to occur on selected edge */
    vAHI_DioInterruptEnable(APP_BUTTONS_DIO_MASK, 0);

    uint32 u32Buttons = u32AHI_DioReadInput() & APP_BUTTONS_DIO_MASK;
    if (u32Buttons != APP_BUTTONS_DIO_MASK)
    {
        return true;
    }
    return false;
}
/****************************************************************************
 *
 * NAME: vISR_SystemController
 *
 * DESCRIPTION:
 * ISR called on system controller interrupt. This may be from the synchronous driver
 * (if used) or user pressing a button the the DK4 build
 *
 * PARAMETER:
 *
 * RETURNS:
 *
 ****************************************************************************/
/*---------------------------------------------------------------------------*/
void button_ISR(uint32 u32DeviceId,	uint32 u32ItemBitmap)
{
  if (BUTTONS_ENABELED) {
//  	if(! (u32DeviceId & E_AHI_DEVICE_SYSCTRL && u32ItemBitmap & BUTTON0_INT)) {
//  		return;
//  	}

  	/* clear pending DIO changed bits by reading register */
  		uint8 u8WakeInt = u8AHI_WakeTimerFiredStatus();
  		uint32 u32IOStatus=u32AHI_DioInterruptStatus();

  	    if( u32IOStatus & APP_BUTTONS_DIO_MASK )
  	    {
  	    	/* disable this IRQ while the button stops oscillating */
  	    	/* disable edge detection until scan complete */
  	    	vAHI_DioInterruptEnable(0, APP_BUTTONS_DIO_MASK);
  	    	/* announce the change */
  	    	sensors_changed(&button_sensor);
  	  		/* let the debounce process know about this to enable it again */
  	  		process_poll(&debounce_process);
  	  	  leds_on(LEDS_ALL);
  	  		PRINTF("button_ISR\n");
  	    }

  		if (u8WakeInt & E_AHI_WAKE_TIMER_MASK_1)
  		{
  			/* wake timer interrupt got us here */
//  			DBG_vPrintf(TRACE_APP_BUTTON, "APP: Wake Timer 1 Interrupt\n");
//  			PWRM_vWakeInterruptCallback();
  			PRINTF("button_ISR: u8WakeInt\n");
  		}
  }
}
/*---------------------------------------------------------------------------*/
void
button_press(void)
{
  sensors_changed(&button_sensor);
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
	switch (type & BUTTONS_ENABELED) {
	case BUTTON_0_MASK:
		return val & BUTTON_0_MASK;
//	case BUTTON_1_MASK:
//		return val & BUTTON_1_MASK;
	}
	return BUTTONS_ENABELED && button_enabled_status && (u32AHI_DioReadInput() & APP_BUTTONS_DIO_MASK); //& type;
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  switch (type) {
  case SENSORS_ACTIVE:
  case SENSORS_READY:
    return button_enabled_status;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int value)
{
	if (BUTTONS_ENABELED) {
		switch (type) {
		case SENSORS_HW_INIT:
		case SENSORS_ACTIVE:
			PRINTF("SENSORS_ACTIVE %u\n", value);
			if (value) {
				if (!status(SENSORS_ACTIVE)) {
					uint8_t buttons_init = APP_bButtonInitialise();
					button_enabled_status = 1;
					PRINTF("SENSORS_HW_INIT %d\n", buttons_init);
					vAHI_SysCtrlRegisterCallback(button_ISR);
					//activate
					//process_start(&debounce_process, NULL);
				}
			} else {
				//deactivate by disabling IRQ
				vAHI_DioInterruptEnable(0, APP_BUTTONS_DIO_MASK);
				button_enabled_status = 0;
			}
			break;
		case SENSORS_READY:
			PRINTF("SENSORS_READY\n");
			return 1;
		default:
			return 0;
		}
	}
	return 1;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(debounce_process, ev, data)
{
  static struct etimer et;

	if (BUTTONS_ENABELED) {
		PROCESS_BEGIN();
		PRINTF("debounce_process begin\n");
		//xxx
		//etimer_set(&et, DEBOUNCE_TIME);
		while (1) {
			PROCESS_YIELD_UNTIL(ev==PROCESS_EVENT_POLL || ev==PROCESS_EVENT_TIMER);

			if(ev==PROCESS_EVENT_TIMER) {
//				if (val != irq_val) {
//					PRINTF("val,irq_val=%d,%d\n",val,irq_val);
//					val = irq_val;
//					sensors_changed(&button_sensor);
//				}
				PRINTF("debounce_process reactivate IRQ\n");
				//reactivate IRQ
				//vAHI_DioInterruptEnable(BUTTON0_DIO, 0);
		    /* Enable interrupts to occur on selected edge */
		    vAHI_DioInterruptEnable(APP_BUTTONS_DIO_MASK, 0);
			  leds_off(LEDS_ALL);
			} else if(ev==PROCESS_EVENT_POLL) {
				etimer_set(&et, DEBOUNCE_TIME);
				PRINTF("debounce_process set timer to reactivate IRQ\n");
			}
		}
		PROCESS_END();
	}
	return 0;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(button_sensor, BUTTON_SENSOR,
	       value, configure, status);
