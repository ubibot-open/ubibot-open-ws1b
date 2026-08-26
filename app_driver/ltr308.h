/*******************************************************************************
  * @file       Light Sensor DRIVER APPLICATION      
  * @author 
  * @version
  * @date 
  * @brief
  ******************************************************************************
  * @attention
  *
  *
*******************************************************************************/
#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

/*-------------------------------- Includes ----------------------------------*/
#include "stdint.h"

#define ERROR_CODE      0xffff

/**
 *  Address registers
 */
#define LTR308_ADDR		  0X53
#define LTR308_MAIN_CTRL		(0x00)
#define LTR308_ALS_MEAS_RATE	(0x04)
#define LTR308_ALS_GAIN			(0x05)
#define LTR308_PART_ID			(0x06)
#define LTR308_MAIN_STATUS		(0x07)
#define LTR308_ALS_DATA_0		(0x0D)
#define LTR308_ALS_DATA_1		(0x0E)
#define LTR308_ALS_DATA_2		(0x0F)
#define LTR308_INT_CFG			(0x19)
#define LTR308_INT_PST 			(0x1A)
#define LTR308_ALS_THRES_UP_0	(0x21)
#define LTR308_ALS_THRES_UP_1	(0x22)
#define LTR308_ALS_THRES_UP_2	(0x23)
#define LTR308_ALS_THRES_LOW_0	(0x24)
#define LTR308_ALS_THRES_LOW_1	(0x25)
#define LTR308_ALS_THRES_LOW_2	(0x26)

/** Default values loaded in probe function */
#define LTR308_PARTID           (0xB1)  /** part_id value */

/**
 * @brief  Initializes the light sensor: reads the device's PART ID register and compares it against the expected value to confirm the device is present; once the check passes, sets the sensor to ALS standby mode.
 * @return SUCCESS (0) means the device ID check passed and initialization succeeded; FAILURE (-1) means the device ID check failed.
 */
extern int LightSensor_Init(void);

/**
 * @brief  Measures the current ambient light level. First checks the device ID; once it passes, configures the sensor for ALS active mode (20-bit resolution, ~400ms conversion time, 1x gain), polls for the data-ready flag, and once ready reads the ALS data registers and converts them to a light value (lux); the sensor is returned to standby mode once the measurement is complete.
 * @param  lightvalue Output parameter that receives the measured light value (lux); set to FAILURE if the device ID check fails or polling times out without new data.
 */
extern void LightSensor_value(float *lightvalue);

#endif //  LIGHT_SENSOR_H

/*******************************************************************************
                                      END         
*******************************************************************************/




