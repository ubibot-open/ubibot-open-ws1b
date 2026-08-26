/*******************************************************************************
  * @file       Pin config DRIVER APPLICATION      
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
#include "stdlib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h" 
#include "freertos/queue.h" 
#include "osi.h" 
#include "pinmux.h"
#include "iic.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "driver/ledc.h"
#include "esp_sleep.h"
#include "MsgType.h"

#define TAG "pinmux"

#define ESP_INTR_FLAG_DEFAULT 0

extern OsiSyncObj_t   SW1_Binary;   //For Button1 interrupt task

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          buzzer_pin // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4096) // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_FREQUENCY          (2850) // Frequency in Hertz. Set frequency at xxxx

// Prepare and then apply the LEDC PWM channel configuration
ledc_channel_config_t ledc_channel = {
    .speed_mode     = LEDC_MODE,
    .channel        = LEDC_CHANNEL,
    .timer_sel      = LEDC_TIMER,
    .intr_type      = LEDC_INTR_DISABLE,
    .gpio_num       = LEDC_OUTPUT_IO,
    .duty           = 0, // Set duty to 0%
    .hpoint         = 0
};

/**
 * @brief  GPIO interrupt service routine (ISR) callback. Sends the GPIO pin number that triggered the interrupt to the GPIO event queue from interrupt context.
 * @param  arg Argument passed in when the interrupt fires; actually the GPIO pin number that triggered the interrupt (cast to void* by the caller).
 */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
  uint32_t gpio_num = (uint32_t)arg;
  if (gpio_num == sw1int_pin)  //button push wake up
  {
    if(SW1_Binary != NULL) osi_SyncObjSignalFromISR(&SW1_Binary);
  }
}

/**
 * @brief  Initializes peripheral pin configuration. Configures the output GPIOs (power mode, indicator LEDs, SPI chip select, EEPROM write protect, etc.) and input interrupt GPIOs (button, accelerometer interrupt, USB detect), creates the GPIO event queue and interrupt handling task, registers the GPIO interrupt service and callback, and completes the LEDC PWM timer and channel initialization for the buzzer.
 */
void PinMuxConfig(void)
{
  gpio_config_t io_conf = {0};

  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask =((1ULL << pwr_mode_pin)|(1ULL << ub_dt_p1_pin1)|(1ULL << ub_dt_p1_pin2)|(1ULL << led1_pin)|(1ULL << led2_pin)
                          |(1ULL << spi_cs_pin)|(1ULL << eepromwp_pin));
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  gpio_set_level(pwr_mode_pin, 1);
  gpio_set_level(ub_dt_p1_pin1, 1);
  gpio_set_level(ub_dt_p1_pin2, 1);
  gpio_set_level(led1_pin, 0);
  gpio_set_level(led2_pin, 0);
  gpio_set_level(spi_cs_pin, 1);
  gpio_set_level(eepromwp_pin, 1);

  io_conf.intr_type = GPIO_INTR_POSEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = ((1ULL << sw1int_pin)|(1ULL << acce_int1));
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);
  io_conf.intr_type = GPIO_INTR_ANYEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << usbint_pin);
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  gpio_isr_handler_add(sw1int_pin, gpio_isr_handler, (void *)sw1int_pin);
  gpio_isr_handler_add(usbint_pin, gpio_isr_handler, (void *)usbint_pin);
  gpio_isr_handler_add(acce_int1, gpio_isr_handler, (void *)acce_int1);

  // Prepare and then apply the LEDC PWM timer configuration
  ledc_timer_config_t ledc_timer = {
      .speed_mode       = LEDC_MODE,
      .duty_resolution  = LEDC_DUTY_RES,
      .timer_num        = LEDC_TIMER,
      .freq_hz          = LEDC_FREQUENCY,  // Set output frequency
      .clk_cfg          = LEDC_AUTO_CLK
  };
  ledc_timer_config(&ledc_timer);
  ledc_channel_config(&ledc_channel);
}

/**
 * @brief  Drives the buzzer to sound for a specified duration: sets the LEDC duty cycle to make the buzzer sound, then after a delay clears the duty cycle back to zero to stop it.
 * @param  n_msec Duration the buzzer sounds, in milliseconds.
 */
void buzzer_on(uint32_t n_msec)
{
  ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY);
  ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
  ets_delay_ms(n_msec);
  ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
  ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

/*******************************************************************************
                                      END         
*******************************************************************************/




