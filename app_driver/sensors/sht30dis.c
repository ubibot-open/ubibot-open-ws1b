/*******************************************************************************

  * @file       Temperature and Humility Sensor Application Driver     
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
#include "sht30dis.h"
#include "iic.h"
#include "crc_check.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

#define TAG "sht32dis"

extern i2c_master_bus_handle_t bus_handle;

/**
 * @brief  Reads data from the SHT30 temperature and humidity sensor using single-shot mode (command 0x2400, high repeatability, clock stretching disabled). Sends the measurement command, waits, then reads 6 bytes of data; performs a CRC-8 check on the temperature and humidity data separately, and converts the values to actual temperature and humidity once the check passes.
 * @param  temp Output parameter that receives the converted temperature value (in degrees Celsius); set to ERROR_CODE if the temperature data fails its CRC check or both temperature and humidity end up as 0 (abnormal).
 * @param  humi Output parameter that receives the converted humidity value (in %RH); set to ERROR_CODE if the humidity data fails its CRC check or both temperature and humidity end up as 0 (abnormal).
 */
void sht30_SingleShotMeasure(float *temp,float *humi)
{
  uint8_t recive[7]={0};
  uint16_t tempval=0,humival=0;
  *temp=ERROR_CODE;
  *humi=ERROR_CODE;
  i2c_device_config_t dev_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = sht30dis_addr,
      .scl_speed_hz = I2C_MASTER_FREQ_HZ,
  };
  uint8_t write_buf[2] = {0x24,0x00};
  i2c_master_dev_handle_t dev_handle;
  esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
  if(err != ESP_OK)
  {
    ESP_LOGE(TAG, "add_device failed: %s", esp_err_to_name(err));
    return;
  }
  err = i2c_master_transmit(dev_handle, write_buf, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
  if(err != ESP_OK)
  {
    ESP_LOGE(TAG, "measure command write failed: %s", esp_err_to_name(err));
    i2c_master_bus_rm_device(dev_handle);
    return;
  }
  osi_Sleep(100);

  err = i2c_master_receive(dev_handle, recive, 6, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
  i2c_master_bus_rm_device(dev_handle);
  if(err != ESP_OK)
  {
    ESP_LOGE(TAG, "measurement read failed: %s", esp_err_to_name(err));
    return;
  }
  if(Data_Crc_Check(&recive[0],3)==0)
  {
    tempval=recive[0];
    tempval=tempval<<8;
    tempval+=recive[1];
    *temp=175*(float)tempval/65535-45;
  }
  if(Data_Crc_Check(&recive[3],3)==0)
  {
    humival=recive[3];
    humival=humival<<8;
    humival+=recive[4];
    *humi=100*(float)humival/65535; 
  }
  // ESP_LOGI(TAG, "%d,%.4f,%.4f", __LINE__,*temp,*humi);
}

/*******************************************************************************
                                      END         
*******************************************************************************/


