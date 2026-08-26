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

/*-------------------------------- Includes ----------------------------------*/
#include "stdlib.h"
#include "osi.h"
#include "iic.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "driver/i2c_master.h"

#define TAG "iic"

i2c_master_bus_handle_t bus_handle;

/**
 * @brief  Delays for the specified number of milliseconds (implemented internally using a microsecond-level delay).
 * @param  nms Delay duration, in milliseconds.
 */
void ets_delay_ms(uint32_t nms)
{
  esp_rom_delay_us(1000*nms);
}

/**
 * @brief  Initializes the I2C bus. Configures the I2C master bus parameters (clock source, port number, SCL/SDA pins, internal pull-up enable, etc.) and creates the I2C master bus handle bus_handle.
 */
void I2C_Init(void)
{
  i2c_master_bus_config_t i2c_mst_config = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = I2C_MASTER_NUM,
      .scl_io_num = I2C_MASTER_SCL_IO,
      .sda_io_num = I2C_MASTER_SDA_IO,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };
  esp_err_t err = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
  if(err != ESP_OK)
  {
    ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
  }
}

/**
 * @brief  Writes one byte of data to a given register on an I2C slave device.
 * @param  sla_addr I2C slave device address (7-bit address).
 * @param  reg_addr Register address.
 * @param  val Data byte to write.
 */
void IIC_Write_Reg(uint8_t sla_addr,uint8_t reg_addr,uint8_t val)
{
  i2c_device_config_t dev_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = sla_addr,
      .scl_speed_hz = I2C_MASTER_FREQ_HZ,
  };
  uint8_t write_buf[2] = {reg_addr, val};
  i2c_master_dev_handle_t dev_handle;
  esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
  if(err != ESP_OK)
  {
    ESP_LOGE(TAG, "add_device(0x%02x) failed: %s", sla_addr, esp_err_to_name(err));
    return;
  }
  err = i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
  if(err != ESP_OK)
  {
    ESP_LOGE(TAG, "write reg 0x%02x@0x%02x failed: %s", reg_addr, sla_addr, esp_err_to_name(err));
  }
  i2c_master_bus_rm_device(dev_handle);
}

/**
 * @brief  Reads one byte of data from a given register on an I2C slave device.
 * @param  sla_addr I2C slave device address (7-bit address).
 * @param  reg_addr Register address.
 * @param  val Pointer to store the read data (output).
 */
void IIC_Read_Reg(uint8_t sla_addr,uint8_t reg_addr,uint8_t *val)
{
  i2c_device_config_t dev_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = sla_addr,
      .scl_speed_hz = I2C_MASTER_FREQ_HZ,
  };
  i2c_master_dev_handle_t dev_handle;
  esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
  if(err != ESP_OK)
  {
    ESP_LOGE(TAG, "add_device(0x%02x) failed: %s", sla_addr, esp_err_to_name(err));
    return;
  }
  err = i2c_master_transmit_receive(dev_handle, &reg_addr, 1, val, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
  if(err != ESP_OK)
  {
    ESP_LOGE(TAG, "read reg 0x%02x@0x%02x failed: %s", reg_addr, sla_addr, esp_err_to_name(err));
  }
  i2c_master_bus_rm_device(dev_handle);
}

/**
 * @brief  Writes multiple bytes of data starting at a given register address on an I2C slave device.
 * @param  sla_addr I2C slave device address (7-bit address).
 * @param  reg_addr Starting register address.
 * @param  buf Pointer to the buffer holding the data to write (input).
 * @param  len Number of bytes to write.
 */
void IIC_Write_Buf(uint8_t sla_addr,uint8_t reg_addr,uint8_t *buf,uint8_t len)
{
  i2c_device_config_t dev_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = sla_addr,
      .scl_speed_hz = I2C_MASTER_FREQ_HZ,
      .scl_wait_us = 20000,
  };
  uint8_t *write_buf=heap_caps_malloc(len+2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if(write_buf==NULL)
  {
    ESP_LOGE(TAG, "write_buf heap_caps_malloc(%d) failed", len+2);
    return;
  }
  memset(write_buf,0,len+2);
  write_buf[0] = reg_addr;
  mem_copy(write_buf+1,buf,len);
  i2c_master_dev_handle_t dev_handle;
  esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
  if(err != ESP_OK)
  {
    ESP_LOGE(TAG, "add_device(0x%02x) failed: %s", sla_addr, esp_err_to_name(err));
    free(write_buf);
    return;
  }
  err = i2c_master_transmit(dev_handle, write_buf, len+1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
  if(err != ESP_OK)
  {
    ESP_LOGE(TAG, "write buf@0x%02x(len=%d) failed: %s", sla_addr, len, esp_err_to_name(err));
  }
  i2c_master_bus_rm_device(dev_handle);
  free(write_buf);
}

/**
 * @brief  Reads multiple bytes of data starting at a given register address on an I2C slave device.
 * @param  sla_addr I2C slave device address (7-bit address).
 * @param  reg_addr Starting register address.
 * @param  buf Pointer to the buffer that receives the read data (output).
 * @param  len Number of bytes to read.
 */
void IIC_Read_Buf(uint8_t sla_addr,uint8_t reg_addr,uint8_t *buf,uint8_t len)
{
  i2c_device_config_t dev_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = sla_addr,
      .scl_speed_hz = I2C_MASTER_FREQ_HZ,
  };
  i2c_master_dev_handle_t dev_handle;
  esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
  if(err != ESP_OK)
  {
    ESP_LOGE(TAG, "add_device(0x%02x) failed: %s", sla_addr, esp_err_to_name(err));
    return;
  }
  err = i2c_master_transmit_receive(dev_handle, &reg_addr, 1, buf, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
  if(err != ESP_OK)
  {
    ESP_LOGE(TAG, "read buf@0x%02x(len=%d) failed: %s", sla_addr, len, esp_err_to_name(err));
  }
  i2c_master_bus_rm_device(dev_handle);
}

/*******************************************************************************
                                      END
*******************************************************************************/
