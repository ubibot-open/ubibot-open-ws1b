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

/*-------------------------------- Includes ----------------------------------*/
#include "power_mgmt.h"
#include "osi.h"
#include "pinmux.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"

#define TAG "power_mgmt"

extern OsiSyncObj_t SW1_Binary;  //For Button interrupt task, defined in main.c
extern bool dev_power_on;        //device power on, defined in main.c

// Set by WakeUp_Process() when the device woke via button 1 and consumed by
// SW1_Task(); only used between these two functions, so it lives here rather
// than as a cross-file global.
static bool sw_wakeup = false;

/**
 * @brief  Button 1 interrupt handling task. Blocks waiting for the button interrupt sync signal; after receiving it, delays briefly for debouncing, and if the button is still detected as pressed or a wake-up flag is set, triggers the buzzer as an audible alert; then clears the sync signal and loops waiting for the next interrupt.
 * @param  pvParameters Parameter passed in when the FreeRTOS task is created (unused)
 */
void SW1_Task( void *pvParameters )
{
  for(;;)
  {
    osi_SyncObjWait(&SW1_Binary,OSI_WAIT_FOREVER);  //Waite Button GPIO Interrupt Message

    osi_Sleep(10);
    if((gpio_get_level(sw1int_pin))||(sw_wakeup==1))
    {
      sw_wakeup=0;
      buzzer_on(200);
    }
    osi_SyncObjClear(&SW1_Binary);  //clear task message
  }
}

/**
 * @brief  Handles the reason the device woke from deep sleep. If the wake-up cause is the EXT1 external pin and it is determined to have been triggered by button 1 (sw1int_pin), and the device is not currently in a power-on state, sets the software wake-up flag and sends the sync signal to the button 1 task from ISR context.
 */
void WakeUp_Process(void)
{
  uint64_t wakeup_pin_mask;
  unsigned long ret_val = esp_sleep_get_wakeup_cause();
  switch(ret_val)
  {
    case ESP_SLEEP_WAKEUP_EXT1:
    {
      wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
      if(((wakeup_pin_mask>>sw1int_pin)&0x01)&&(dev_power_on==0))
      {
        ESP_LOGI(TAG, "%d,sw1 int", __LINE__);
        sw_wakeup = 1;
        osi_SyncObjSignalFromISR(&SW1_Binary);
      }
    }
    break;
    default:
    break;
  }
}

/**
 * @brief  Puts the device into deep sleep mode, enabling both the timer wake-up source and the button 1 (EXT1, level-triggered) wake-up source.
 * @param  slp_time Sleep duration for the timer wake-up, in seconds
 */
void Enter_Sleep(unsigned long slp_time)
{
  ESP_LOGI(TAG,"slp_time=%ld", slp_time);
  esp_sleep_enable_timer_wakeup(slp_time * 1000000);  //
  const int ext_wakeup_pin_1 = sw1int_pin;
  const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin_1;
  esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
  ESP_LOGI(TAG, "SET BUTTON1 wakeup");
	esp_deep_sleep_start();
}

/*******************************************************************************
                                      END
*******************************************************************************/
