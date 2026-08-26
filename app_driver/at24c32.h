/*******************************************************************************
  * @file       AT24C32 EEPROM CHIP DRIVER APPLICATION     
  * @author 
  * @version
  * @date 
  * @brief
  ******************************************************************************
  * @attention
  *
  *
*******************************************************************************/
#ifndef AT24C32_H
#define AT24C32_H

/*-------------------------------- Includes ----------------------------------*/
#include "stdint.h"
#include "stdbool.h"

#define AT24C32_ADDR          0x54    //IIC ADDRESS

/**
 * @brief  Writes an arbitrary length of data to a given address in the AT24C32 EEPROM. Internally splits the data at 16-byte page boundaries into an initial unaligned remainder, a number of full pages, and a trailing remainder, calling the page-write function for each segment.
 * @param  addr Starting address to write to.
 * @param  buf Pointer to the buffer holding the data to write (input).
 * @param  size Total number of bytes to write, range 1-256.
 */
extern void at24c32_write_buf(uint16_t addr, uint8_t *buf, uint8_t size);

/**
 * @brief  Reads data from a given address in the AT24C32 EEPROM.
 * @param  addr Starting address to read from.
 * @param  buf Pointer to the buffer that receives the read data (output).
 * @param  size Number of bytes to read.
 */
extern void at24c32_read_buf(uint16_t addr, uint8_t *buf, uint8_t size);

#endif  // AT24C32_H

/*******************************************************************************
                                      END         
*******************************************************************************/









