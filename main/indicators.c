/*******************************************************************************
  * @file       LED / Buzzer Indicator Task
  * @author
  * @version
  * @date
  * @brief
  ******************************************************************************
  * @attention
  *
  *
*******************************************************************************/

/*-------------------------------- Includes ----------------------------------*/
#include "indicators.h"
#include "osi.h"
#include "pinmux.h"
#include "driver/gpio.h"

/**
 * @brief  Turns on the green LED indicator.
 */
void SET_GREEN_LED_ON(void)
{
  gpio_set_level(led1_pin, 1);
}

/**
 * @brief  Turns off the green LED indicator.
 */
void SET_GREEN_LED_OFF(void)
{
  gpio_set_level(led1_pin, 0);
}

/**
 * @brief  Turns on the red LED indicator.
 */
void SET_RED_LED_ON(void)
{
  gpio_set_level(led2_pin, 1);
}

/**
 * @brief  Turns off the red LED indicator.
 */
void SET_RED_LED_OFF(void)
{
  gpio_set_level(led2_pin, 0);
}

/**
 * @brief  Green LED blink task. Loops, alternately turning the green LED on and off at 500ms intervals, to indicate that the network data reporting process is in progress; the task itself never exits.
 * @param  pvParameters Parameter passed in when the FreeRTOS task is created (unused)
 */
void G_Led_Task(void *pvParameters)
{
  for(;;)
  {
    SET_GREEN_LED_ON();
    osi_Sleep(500);  //delay 500ms

    SET_GREEN_LED_OFF();
    osi_Sleep(500);  //delay 500ms
  }
}

/*******************************************************************************
                                      END
*******************************************************************************/
