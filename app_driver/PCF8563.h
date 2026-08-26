/*******************************************************************************
  * @file       PCF8563 Driver  
  * @author 
  * @version
  * @date 
  * @brief
  ******************************************************************************
  * @attention
  *
  *
*******************************************************************************/
#ifndef PCF8563_H
#define PCF8563_H

/*-------------------------------- Includes ----------------------------------*/
#include "stdint.h"
#include "stdbool.h"
#include "time.h"

#define PCF8563_ADDR            0x51

#define Control_status_1        0x00
#define Control_status_2        0x01

#define VL_seconds              0x02
#define Minutes                 0x03
#define Hours                   0x04
#define Days                    0x05
#define Weekdays                0x06
#define Century_months          0x07
#define Years                   0x08

#define Minute_alarm            0x09
#define Hour_alarm              0x0a
#define Day_alarm               0x0b
#define Weekday_alarm           0x0c

#define CLKOUT_control          0x0d
#define Timer_control           0x0e
#define Timer_value             0x0f

typedef struct tm* tm_struct;

/**
 * @brief  Initializes the PCF8563: clears the control/status registers, disables the minute/hour/day/weekday alarms, and disables CLKOUT output and the timer
 */
extern void Timer_IC_Init(void);

/**
 * @brief  Resets the PCF8563 time registers to a fixed default time value
 */
extern void Timer_IC_Reset_Time(void);

/**
 * @brief  Reads the current time registers from the PCF8563 and converts the BCD-encoded year/month/day/hour/minute/second into a Unix timestamp
 * @return The resulting Unix timestamp (seconds)
 */
extern unsigned long Read_UnixTime(void);

/**
 * @brief  Converts the given Unix timestamp to local time and writes it to the PCF8563 time registers, completing a clock time-set
 * @param  unix_tm Unix timestamp to write (seconds)
 */
extern void Update_UnixTime(uint32_t unix_tm);

/**
 * @brief  Reads the current time registers from the PCF8563, converts them, and formats the result as an ISO 8601 UTC string (YYYY-MM-DDTHH:MM:SSZ)
 * @param  buffer   Output parameter; buffer that receives the formatted UTC time string
 * @param  buf_size Size of the buffer, in bytes
 */
extern void Read_UTCtime(char *buffer,uint8_t buf_size);

#endif //  PCF8563_H

/*******************************************************************************
                                      END         
*******************************************************************************/




