/*******************************************************************************
  * @file        Temperature Sensor Application      
  * @author 
  * @version
  * @date 
  * @brief
  ******************************************************************************
  * @attention
  *
  *
*******************************************************************************/
#ifndef UB_DT_P1_H
#define UB_DT_P1_H

/*-------------------------------- Includes ----------------------------------*/
#include "stdint.h"

#define ERROR_CODE 0xffff

/**
 * @brief  Read the temperature conversion results from sensors 1 and 2: after starting conversion, reset each sensor and read its 9 bytes of scratchpad data, then compute the actual temperature value after CRC verification
 * @param  temp_value1 Output parameter used to return sensor 1's temperature value, in degrees Celsius; remains ERROR_CODE if the read is not successful
 * @param  temp_value2 Output parameter used to return sensor 2's temperature value, in degrees Celsius; remains ERROR_CODE if the read is not successful
 */
extern void ub_dt_p1_get_temp(float *temp_value1,float *temp_value2);

#endif //  UB_DT_P1_H

/*******************************************************************************
                                      END         
*******************************************************************************/




