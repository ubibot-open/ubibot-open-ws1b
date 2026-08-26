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
#ifndef SHT30DIS_H
#define SHT30DIS_H

/*-------------------------------- Includes ----------------------------------*/
#include "stdint.h"

#define ERROR_CODE              0xffff
#define sht30dis_addr           0x44    //7 MSB address 0x44

/**
 * @brief  Reads data from the SHT30 temperature and humidity sensor using single-shot mode (command 0x2400, high repeatability, clock stretching disabled). Sends the measurement command, waits, then reads 6 bytes of data; performs a CRC-8 check on the temperature and humidity data separately, and converts the values to actual temperature and humidity once the check passes.
 * @param  temp Output parameter that receives the converted temperature value (in degrees Celsius); set to ERROR_CODE if the temperature data fails its CRC check or both temperature and humidity end up as 0 (abnormal).
 * @param  humi Output parameter that receives the converted humidity value (in %RH); set to ERROR_CODE if the humidity data fails its CRC check or both temperature and humidity end up as 0 (abnormal).
 */
extern void sht30_SingleShotMeasure(float *temp,float *humi);

#endif //  SHT30DIS_H

/*******************************************************************************
                                      END         
*******************************************************************************/




