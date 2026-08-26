/*******************************************************************************
  * @file           
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#ifndef __MSG_TYPE_H__
#define __MSG_TYPE_H__

#define USR_POST_DATA_SUM 20

// Device identity / WiFi / server settings are configured via `idf.py
// menuconfig` -> "UbiBot WS1B Configuration" (see main/Kconfig.projbuild)
// rather than hardcoded here, so per-device values (serial number, WiFi
// credentials, server address) live in the local, gitignored `sdkconfig`
// instead of being committed to source.
#define USR_PID           CONFIG_USR_PID
#define USR_SN            CONFIG_USR_SN
#define USR_SSID          CONFIG_USR_WIFI_SSID
#define USR_PASSWORD      CONFIG_USR_WIFI_PASSWORD
#define USR_CONCTRY_CODE  CONFIG_USR_WIFI_COUNTRY_CODE
#define USR_HTTP_HOST     CONFIG_USR_HTTP_HOST
#define USR_HTTP_PORT     CONFIG_USR_HTTP_PORT
#define USR_HTTP_TIME_URL "/api/v1/auth/time"
#define USR_HTTP_DATA_URL "/api/v1/data/report"
#define UPDATE_TIME_MODE  0x01
#define DATA_POST_MODE    0x02
#define HTTP_TX_BUFF_LEN  4096
#define HTTP_RX_BUFF_LEN  4096

#define TEMP_NUM        1
#define HUMI_NUM        2
#define LIGHT_NUM       3
#define BAT_NUM         4
#define RSSI_NUM        5
#define EXT1_TEMP_NUM   6
#define EXT2_TEMP_NUM   7

#define DEFAULT_FN            300
#define DEFAULT_WIFI_BAND     0
#define DEFAULT_WIFI_CNT_TIMEOUT  30

typedef struct
{
  unsigned long ts;
  uint8_t sensornum;    //sensor field
  float   sensorval;    //sensor value
} SensorMessage;

#endif

/*******************************************************************************
                                      END         
*******************************************************************************/