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
#ifndef INDICATORS_H
#define INDICATORS_H

/**
 * @brief  Turns on the green LED indicator.
 */
void SET_GREEN_LED_ON(void);

/**
 * @brief  Turns off the green LED indicator.
 */
void SET_GREEN_LED_OFF(void);

/**
 * @brief  Turns on the red LED indicator.
 */
void SET_RED_LED_ON(void);

/**
 * @brief  Turns off the red LED indicator.
 */
void SET_RED_LED_OFF(void);

/**
 * @brief  Green LED blink task. Loops, alternately turning the green LED on and off at 500ms intervals, to indicate that the network data reporting process is in progress; the task itself never exits.
 * @param  pvParameters Parameter passed in when the FreeRTOS task is created (unused)
 */
void G_Led_Task(void *pvParameters);

#endif //  INDICATORS_H

/*******************************************************************************
                                      END
*******************************************************************************/
