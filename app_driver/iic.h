/*******************************************************************************
  * @file       IIC BUS DRIVER APPLICATION       
  * @author 
  * @version
  * @date 
  * @brief
  ******************************************************************************
  * @attention
  *
  *
*******************************************************************************/
#ifndef __IIC_H__
#define __IIC_H__

/*-------------------------------- Includes ----------------------------------*/
#include "stdint.h"
#include "pinmux.h"

#define I2C_MASTER_NUM    I2C_NUM_0 /*!< I2C port number for master dev */
#define I2C_MASTER_SCL_IO iic_scl_pin     /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO iic_sda_pin     /*!< gpio number for I2C master data  */

#define I2C_MASTER_FREQ_HZ        100000   /*!< I2C master clock frequency */
#define I2C_MASTER_TIMEOUT_MS     1000  

/**
 * @brief  Delays for the specified number of milliseconds (implemented internally using a microsecond-level delay).
 * @param  ms Delay duration, in milliseconds.
 */
extern void ets_delay_ms(uint32_t ms);

/**
 * @brief  Initializes the I2C bus. Configures the I2C master bus parameters (clock source, port number, SCL/SDA pins, internal pull-up enable, etc.) and creates the I2C master bus handle bus_handle.
 */
extern void I2C_Init(void);

/**
 * @brief  Writes one byte of data to a given register on an I2C slave device.
 * @param  sla_addr I2C slave device address (7-bit address).
 * @param  reg_addr Register address.
 * @param  val Data byte to write.
 */
extern void IIC_Write_Reg(uint8_t sla_addr,uint8_t reg_addr,uint8_t val);

/**
 * @brief  Reads one byte of data from a given register on an I2C slave device.
 * @param  sla_addr I2C slave device address (7-bit address).
 * @param  reg_addr Register address.
 * @param  val Pointer to store the read data (output).
 */
extern void IIC_Read_Reg(uint8_t sla_addr,uint8_t reg_addr,uint8_t *val);

/**
 * @brief  Writes multiple bytes of data starting at a given register address on an I2C slave device.
 * @param  sla_addr I2C slave device address (7-bit address).
 * @param  reg_addr Starting register address.
 * @param  buf Pointer to the buffer holding the data to write (input).
 * @param  len Number of bytes to write.
 */
extern void IIC_Write_Buf(uint8_t sla_addr,uint8_t reg_addr,uint8_t *buf,uint8_t len);

/**
 * @brief  Reads multiple bytes of data starting at a given register address on an I2C slave device.
 * @param  sla_addr I2C slave device address (7-bit address).
 * @param  reg_addr Starting register address.
 * @param  buf Pointer to the buffer that receives the read data (output).
 * @param  len Number of bytes to read.
 */
extern void IIC_Read_Buf(uint8_t sla_addr,uint8_t reg_addr,uint8_t *buf,uint8_t len);

#endif //  __IIC_H__

/*******************************************************************************
                                      END         
*******************************************************************************/















