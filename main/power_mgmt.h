/*******************************************************************************
  * @file       Button / Deep-sleep Wake-up Management
  * @author
  * @version
  * @date
  * @brief
  ******************************************************************************
  * @attention
  *
  *
*******************************************************************************/
#ifndef POWER_MGMT_H
#define POWER_MGMT_H

/**
 * @brief  Button 1 interrupt handling task. Blocks waiting for the button interrupt sync signal; after receiving it, delays briefly for debouncing, and if the button is still detected as pressed or a wake-up flag is set, triggers the buzzer as an audible alert; then clears the sync signal and loops waiting for the next interrupt.
 * @param  pvParameters Parameter passed in when the FreeRTOS task is created (unused)
 */
void SW1_Task(void *pvParameters);

/**
 * @brief  Handles the reason the device woke from deep sleep. If the wake-up cause is the EXT1 external pin and it is determined to have been triggered by button 1 (sw1int_pin), and the device is not currently in a power-on state, sets the software wake-up flag and sends the sync signal to the button 1 task from ISR context.
 */
void WakeUp_Process(void);

/**
 * @brief  Puts the device into deep sleep mode, enabling both the timer wake-up source and the button 1 (EXT1, level-triggered) wake-up source.
 * @param  slp_time Sleep duration for the timer wake-up, in seconds
 */
void Enter_Sleep(unsigned long slp_time);

#endif //  POWER_MGMT_H

/*******************************************************************************
                                      END
*******************************************************************************/
