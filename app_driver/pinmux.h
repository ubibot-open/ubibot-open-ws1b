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

#ifndef PINMUX_H
#define PINMUX_H
#include "stdint.h"

#define usbint_pin      2
#define acce_int1       3
#define batvol_pin      4
#define sw1int_pin      5
#define pwr_mode_pin    6

#define led1_pin        7
#define led2_pin        8
#define iic_scl_pin     9      //SCL GPIO PIN
#define iic_sda_pin     10      //SDA GPIO PIN
#define ub_dt_p1_pin1    13
#define ub_dt_p1_pin2    14

#define spi_sck_pin     24
#define spi_mosi_pin    23
#define spi_miso_pin    25
#define spi_cs_pin      28

#define buzzer_pin      26
#define eepromwp_pin    27

#define uart0tx_pin     (UART_PIN_NO_CHANGE)
#define uart0rx_pin     (UART_PIN_NO_CHANGE)
#define uart0_rts_pin   (UART_PIN_NO_CHANGE)
#define uart0_cts_pin   (UART_PIN_NO_CHANGE)

/**
 * @brief  Initializes peripheral pin configuration. Configures the output GPIOs (power mode, indicator LEDs, SPI chip select, EEPROM write protect, etc.) and input interrupt GPIOs (button, accelerometer interrupt, USB detect), creates the GPIO event queue and interrupt handling task, registers the GPIO interrupt service and callback, and completes the LEDC PWM timer and channel initialization for the buzzer.
 */
extern void PinMuxConfig(void);

/**
 * @brief  Drives the buzzer to sound for a specified duration: sets the LEDC duty cycle to make the buzzer sound, then after a delay clears the duty cycle back to zero to stop it.
 * @param  n_msec Duration the buzzer sounds, in milliseconds.
 */
extern void buzzer_on(uint32_t n_msec);

#endif //  PINMUX_H




/*******************************************************************************
                                      END         
*******************************************************************************/