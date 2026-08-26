/*******************************************************************************
  * @file       at24c32 EEPROM CHIP DRIVER APPLICATION     
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
#include "string.h"
#include "stdlib.h"
#include "osi.h" 
#include "iic.h"
#include "at24c32.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TAG "at24c32"

extern i2c_master_bus_handle_t bus_handle;

/**
 * @brief  Writes multiple bytes of data to a given register address on the AT24C32 memory chip (acquires an I2C device handle and sends the data in one transfer, then delays to allow the chip to finish writing).
 * @param  sla_addr I2C slave device address (7-bit address).
 * @param  reg_addr Starting memory cell address.
 * @param  buf Pointer to the buffer holding the data to write (input).
 * @param  len Number of bytes to write.
 */
void at24c32_write(uint8_t sla_addr,uint16_t reg_addr,uint8_t *buf,uint8_t len)
{
  i2c_device_config_t dev_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = sla_addr,
      .scl_speed_hz = I2C_MASTER_FREQ_HZ,
      .scl_wait_us = 20000,
  };
  uint8_t *write_buf = heap_caps_malloc(len+3, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if(write_buf==NULL)
  {
    ESP_LOGE(TAG, "at24c32_write: malloc failed");
    return;
  }
  memset(write_buf,0,len+3);
  write_buf[0] = reg_addr/256;
  write_buf[1] = reg_addr%256;
  mem_copy(write_buf+2,buf,len);
  i2c_master_dev_handle_t dev_handle;
  i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
  i2c_master_transmit(dev_handle, write_buf, len+2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
  i2c_master_bus_rm_device(dev_handle);
  free(write_buf);
  osi_Sleep(20);
}

/**
 * @brief  Reads multiple bytes of data from a given register address on the AT24C32 memory chip.
 * @param  sla_addr I2C slave device address (7-bit address).
 * @param  reg_addr Starting memory cell address.
 * @param  buf Pointer to the buffer that receives the read data (output).
 * @param  len Number of bytes to read.
 */
void at24c32_read(uint8_t sla_addr,uint16_t reg_addr,uint8_t *buf,uint8_t len)
{
    uint8_t write_buf[2] = {reg_addr/256,reg_addr%256};
    i2c_device_config_t dev_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = sla_addr,
      .scl_speed_hz = I2C_MASTER_FREQ_HZ,
      .scl_wait_us = 20000,
  };
  i2c_master_dev_handle_t dev_handle;
  i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
  i2c_master_transmit_receive(dev_handle, write_buf, 2, buf, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
  i2c_master_bus_rm_device(dev_handle);
}

/**
 * @brief  Writes one page of data to the AT24C32 EEPROM. Pulls the write-protect pin low before writing to enable the write operation, then pulls it back high after the write completes to restore write protection.
 * @param  reg_addr Starting address to write to, range 0-4096.
 * @param  buffer Pointer to the buffer holding the data to write (input).
 * @param  buf_len Number of bytes to write, range 1-16 (single-page capacity limit).
 */
static void at24c32_write_Page(uint16_t reg_addr, uint8_t *buffer, uint8_t buf_len)
{
  gpio_set_level(eepromwp_pin, 0);
  at24c32_write(AT24C32_ADDR,reg_addr,buffer,buf_len);  //
  gpio_set_level(eepromwp_pin, 1); 
}

/**
 * @brief  Writes an arbitrary length of data to a given address in the AT24C32 EEPROM. Internally splits the data at 16-byte page boundaries into an initial unaligned remainder, a number of full pages, and a trailing remainder, calling the page-write function for each segment.
 * @param  addr Starting address to write to, range 0-4096.
 * @param  buf Pointer to the buffer holding the data to write (input).
 * @param  size Total number of bytes to write, range 1-256.
 */
void at24c32_write_buf(uint16_t addr, uint8_t *buf, uint8_t size)
{
  uint8_t i,add=0;
  uint8_t remain;
  if(size)
  {
    remain = 16 - addr % 16;
    if (remain)
    {
      remain = size > remain ? remain : size;
      at24c32_write_Page(addr, buf, remain);
      addr += remain;
      add += remain;
      size -= remain;
    }

    remain = size / 16;
    for (i = 0; i < remain; i++)
    {
      at24c32_write_Page(addr, buf+add, 16);
      addr += 16;
      add += 16;
    }

    remain = size % 16;
    if (remain)
    {
      at24c32_write_Page(addr, buf+add, remain);
      addr += remain;
    }
  }
}

/**
 * @brief  Reads data from a given address in the AT24C32 EEPROM.
 * @param  reg_addr Starting address to read from, range 0-4096.
 * @param  buf Pointer to the buffer that receives the read data (output).
 * @param  size Number of bytes to read.
 */
void at24c32_read_buf(uint16_t reg_addr,uint8_t *buf,uint8_t size)
{
  at24c32_read(AT24C32_ADDR, reg_addr, buf, size);
}

/*******************************************************************************
                                      END         
*******************************************************************************/
