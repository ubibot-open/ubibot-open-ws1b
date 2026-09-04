/*******************************************************************************
  * @file       MAIN FUNCTION PROTOTYPES
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

#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_sleep.h"

#include "cJSON.h"
#include "iic.h"
#include "user_spi.h"
#include "at24c32.h"
#include "w25q128.h"
#include "PCF8563.h"
#include "sht30dis.h"
#include "ltr308.h"
#include "ub_dt_p1.h"
#include "stk8323.h"
#include "power_adc.h"
#include "http_client.h"
#include "wifi_connect.h"
#include "MsgType.h"

#include "indicators.h"
#include "power_mgmt.h"
#include "json_payload.h"
#include "provisioning.h"
#include "command.h"

#define TAG "MAIN"

EventGroupHandle_t Nets_Group;
OsiSyncObj_t SW1_Binary;  //For Button interrupt task
OsiMsgQ_t Data_Queue;  //Used field data save

bool dev_power_on = 0;  //device power on

/**
 * @brief  Samples all onboard/external sensors (temperature & humidity, ambient light, battery voltage, and the two external temperature probes) once and pushes each valid reading into the sensor message queue for later reporting. Readings equal to ERROR_CODE are discarded and not enqueued.
 */
void Sensors_Data_Update(void)
{
  float temp_value=0,humi_value=0;
  float lightvalue=0;
  float bat_voltage=0;
  float temp_value_1=0,temp_value_2=0;
  SensorMessage sMsg={0};

  sMsg.ts = Read_UnixTime();
  sht30_SingleShotMeasure(&temp_value,&humi_value);  //read temperature humidity sensor
  ESP_LOGI(TAG, "%d,temp=%.4f,humi=%.4f", __LINE__,temp_value,humi_value);
  if((temp_value!=ERROR_CODE)&&(humi_value!=ERROR_CODE))
  {
    sMsg.sensornum=TEMP_NUM;  //Message Number
    sMsg.sensorval=temp_value;  //Message Value
    osi_MsgQWrite(&Data_Queue,&sMsg,OSI_SAVE_WAIT);  //Send Message
    sMsg.sensornum=HUMI_NUM;  //Message Number
    sMsg.sensorval=humi_value;  //Message Value
    osi_MsgQWrite(&Data_Queue,&sMsg,OSI_SAVE_WAIT);  //Send Message
  }

  LightSensor_value(&lightvalue);  //Read Light sensor
  ESP_LOGI(TAG, "%d,light=%.4f", __LINE__,lightvalue);
  if(lightvalue!=FAILURE)
  {
    sMsg.sensornum=LIGHT_NUM;  //Message Number
    sMsg.sensorval=lightvalue;  //Message Value
    osi_MsgQWrite(&Data_Queue,&sMsg,OSI_SAVE_WAIT);  //Send Message
  }

  bat_voltage=power_adcValue();  //Read Battery Voltage Value
  ESP_LOGI(TAG, "%d,bat_voltage=%.4f", __LINE__,bat_voltage);
  if(bat_voltage!=ERROR_CODE)
  {
    sMsg.sensornum=BAT_NUM;  //Message Number
    sMsg.sensorval=bat_voltage;  //Message Value
    osi_MsgQWrite(&Data_Queue,&sMsg,OSI_SAVE_WAIT);  //Send Message
  }

  ub_dt_p1_get_temp(&temp_value_1,&temp_value_2);  //read ub_dt_p1 sensor
  ESP_LOGI(TAG, "%d,temp_value_1=%.4f,temp_value_2=%.4f", __LINE__,temp_value_1,temp_value_2);
  if(temp_value_1 != ERROR_CODE)
  {
    sMsg.sensornum=EXT1_TEMP_NUM;  //Message Number
    sMsg.sensorval=temp_value_1;  //Message Value
    osi_MsgQWrite(&Data_Queue,&sMsg,OSI_SAVE_WAIT);   //send message
  }
  if(temp_value_2 != ERROR_CODE)
  {
    sMsg.sensornum=EXT2_TEMP_NUM;  //Message Number
    sMsg.sensorval=temp_value_2;  //Message Value
    osi_MsgQWrite(&Data_Queue,&sMsg,OSI_SAVE_WAIT);   //send message
  }
}

/**
 * @brief  Main business loop task: if the local time has not yet been synchronized (earlier than 2026-1-1), connects to the network first to synchronize time with the server; then successively collects temperature/humidity, light level, battery voltage, and the two external temperature sensor readings and writes them into the message queue; connects to the network again to package and report the queued data to the server; and finally enters deep sleep to wait for the next wake-up before repeating the cycle.
 * @param  pvParameters Parameter passed in when the FreeRTOS task is created (unused)
 */
void Main_Task(void *pvParameters)
{
  short res_val=-1;
  char *http_tx_buf;
  char *http_rx_buf;
  for(;;)
  {
    ESP_LOGI(TAG, "heap: free=%u min_free=%u internal_free=%u spiram_free=%u",
            esp_get_free_heap_size(), esp_get_minimum_free_heap_size(),heap_caps_get_free_size(MALLOC_CAP_INTERNAL),heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    http_tx_buf = heap_caps_malloc(HTTP_TX_BUFF_LEN, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);  //Allocate HTTP request buffer from SPIRAM
    if(http_tx_buf==NULL)
    {
      ESP_LOGE(TAG, "http_tx_buf heap_caps_malloc failed %d", __LINE__);
    }
    http_rx_buf = heap_caps_malloc(HTTP_RX_BUFF_LEN, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);  //Allocate HTTP response buffer from SPIRAM
    if(http_rx_buf==NULL)
    {
      ESP_LOGE(TAG, "http_rx_buf heap_caps_malloc failed %d", __LINE__);
    }
    if((http_tx_buf!=NULL)&&(http_rx_buf!=NULL))
    {
      res_val=WiFi_Connect((char *)Provision_GetWifiSsid(), (char *)Provision_GetWifiPassword(),USR_CONCTRY_CODE);  //Connect to the AP, using the provisioned SSID/password if this device has been set up over serial (protocol §1.2), else the menuconfig default
      if(res_val == SUCCESS)
      {
        if(Read_UnixTime()<1767196800)  //before 2026-1-1,update time - Read_UnixTime() gets current local time from RTC
        {
          mem_set(http_tx_buf,0,HTTP_TX_BUFF_LEN);
          mem_set(http_rx_buf,0,HTTP_RX_BUFF_LEN);

          Device_PostData_Read(http_tx_buf,HTTP_TX_BUFF_LEN);  //Build the time-sync request JSON payload

          //Send time-sync request and parse the response
          res_val = HTTP_Post_Method((char *)Provision_GetHttpHost(),USR_HTTP_TIME_URL,Provision_GetHttpPort(),http_tx_buf,strlen(http_tx_buf),http_rx_buf,HTTP_RX_BUFF_LEN,Parse_Response);
          if(res_val == SUCCESS)
          {
            ESP_LOGI(TAG, "%d,update time success.", __LINE__);
          }
          else
          {
            ESP_LOGW(TAG, "%d,update time failed,code=%d.", __LINE__,res_val);
          }
        }

        mem_set(http_tx_buf,0,HTTP_TX_BUFF_LEN);
        mem_set(http_rx_buf,0,HTTP_RX_BUFF_LEN);

        Sensors_Data_Update();  //Sample all sensors and enqueue readings
        Sensors_PostData_Read(http_tx_buf,HTTP_TX_BUFF_LEN);  //Build the data-report request JSON payload from the queue

        //Send sensor data report and parse the response
        res_val = HTTP_Post_Method((char *)Provision_GetHttpHost(),USR_HTTP_DATA_URL,Provision_GetHttpPort(),http_tx_buf,strlen(http_tx_buf),http_rx_buf,HTTP_RX_BUFF_LEN,Parse_Response);
        if(res_val == SUCCESS)
        {
          ESP_LOGI(TAG, "%d,post data success.", __LINE__);
        }
        else
        {
          ESP_LOGW(TAG, "%d,post data failed,code=%d.", __LINE__,res_val);
        }
      }
      else
      {
        ESP_LOGW(TAG, "WiFi_Connect failed! \n");
      }
      WiFi_Disconnect();  //Disconnect from the AP
      free(http_tx_buf);
      free(http_rx_buf);
    }
    Enter_Sleep(Command_GetReportIntervalSeconds());  //Enter deep sleep until next timer/button wake-up; interval is the server-commanded value if one was ever set (protocol §9), else DEFAULT_FN
  }
}

/**
 * @brief  Program entry point. Initializes NVS storage (erasing and reinitializing on failure), loads the active WiFi/server config and report interval (provisioned-over-serial values / server-commanded values from NVS, falling back to the menuconfig/DEFAULT_FN defaults -- protocol §1.2/§9), creates the event group, the button sync object, and the sensor message queue, and completes peripheral pin and I2C bus initialization; determines whether this is a power-on boot based on the reset reason, and if so, sounds the buzzer, completes RTC clock and light sensor initialization, and opens the serial provisioning window (protocol §1.2) for a technician to (re)configure WiFi/server settings; processes the sleep wake-up source, then creates the button interrupt task, the green LED blink task, and the main business task.
 */
void app_main(void)
{
  esp_err_t ret = nvs_flash_init();  //Initialize NVS
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ret = nvs_flash_erase();  //Erase NVS partition
    if(ret!=ESP_OK)
    {
      ESP_LOGE(TAG, "%dv,nvs_flash_erase failed.", __LINE__);
    }
    ret = nvs_flash_init();  //Re-initialize NVS after erase
  }
  if(ret!=ESP_OK)
  {
    ESP_LOGE(TAG, "%dv,nvs_flash_init failed.", __LINE__);
  }

  Provision_Init();  //Load WiFi/server config: NVS-provisioned values (protocol §1.2) override the menuconfig defaults, every boot -- deep sleep wipes RAM so this can't be assumed to persist on its own.
  Command_Init();  //Load the active report interval: the last server-commanded value (protocol §9) if any, else DEFAULT_FN -- same every-boot reasoning as Provision_Init() above.

  Nets_Group = xEventGroupCreate();  //Create event group for network status flags
  xEventGroupClearBits(Nets_Group, Nets_Group_ALL_BIT);  //Clear all network status bits
  osi_SyncObjCreate(&SW1_Binary);  //Button Interrupt Task
  osi_MsgQCreate(&Data_Queue,"Data_Queue",sizeof(SensorMessage),USR_POST_DATA_SUM);   //create queue used for field data save

  PinMuxConfig();  //Configure The gpio
  I2C_Init();  //Configure IIC Bus

  unsigned long ulResetCause = esp_reset_reason();  //Get the reason for the last reset
  ESP_LOGI(TAG, "%d,%d", __LINE__,(int)ulResetCause);
  if (ulResetCause == ESP_RST_POWERON) //power on
  {
    dev_power_on = 1;
    SET_GREEN_LED_ON();  //Turn on green LED
    buzzer_on(200);  //Sound buzzer for 200ms
    SET_GREEN_LED_OFF();  //Green led off

    Timer_IC_Init();  //PCF8563 init
    Timer_IC_Reset_Time();  //PCF8563 Reset Time 2018-01-01 00:00:00
    LightSensor_Init();  // light sensor init

    // Give a technician connected over USB a window to send SetupWifi/
    // SetupServer (protocol §1.2) right after power-up, before this boot
    // goes on to connect WiFi with whatever config it just loaded. Only on
    // power-on (not on every periodic timer wake-up) so routine reporting
    // cycles aren't slowed down waiting on a UART nobody is watching.
    Provision_RunWindow(PROV_WINDOW_IDLE_TIMEOUT_MS, PROV_WINDOW_MAX_TOTAL_MS);
  }
  WakeUp_Process();  //Handle deep-sleep wake-up cause

  osi_TaskCreate(SW1_Task, NULL,2048,NULL, 9, NULL);  //Create Button Interrupt_Task
  osi_TaskCreate(G_Led_Task, NULL,2048, NULL, 7,NULL);  //Create GREEN LED Blink Task
  osi_TaskCreate(Main_Task, NULL,8192,NULL, 5, NULL);  //Create net Task
}

/*******************************************************************************
                                      END
*******************************************************************************/
