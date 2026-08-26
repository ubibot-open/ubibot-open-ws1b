/*******************************************************************************
  * @file       SPI BUS DRIVER APPLICATION       
  * @author 
  * @version
  * @date 
  * @brief
  ******************************************************************************
  * @attention
  *
  *
*******************************************************************************/
#ifndef __SPI_H__
#define __SPI_H__

/*-------------------------------- Includes ----------------------------------*/
#include "stdint.h"
#include "driver/gpio.h"
#include "pinmux.h"

#define SET_SPI1_CS_ON()    gpio_set_level(spi_cs_pin, 1); 
#define SET_SPI1_CS_OFF()   gpio_set_level(spi_cs_pin, 0);

/**
 * @brief  Initializes the SPI interface: completes SPI hardware initialization and sets the CS chip-select pin to its initial level.
 */
extern void UserSpiInit(void);

/**
 * @brief  Sends one byte of data over the SPI bus while simultaneously receiving one byte returned on the bus.
 * @param  addr The byte of data to send.
 * @return The byte of data received from the SPI bus.
 */
extern uint8_t SPI_SendReciveByte(uint8_t addr);

#endif //  __SPI_H__

/*******************************************************************************
                                      END         
*******************************************************************************/




