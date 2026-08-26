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

#define TAG "MAIN"

EventGroupHandle_t Nets_Group;
OsiSyncObj_t SW1_Binary;  //For Button interrupt task
OsiMsgQ_t Data_Queue;  //Used field data save

bool sw_wakeup=0;
bool dev_power_on = 0;  //device power on

/**
 * @brief  Turns on the green LED indicator.
 */
void SET_GREEN_LED_ON(void)
{
  gpio_set_level(led1_pin, 1);
}
/**
 * @brief  Turns off the green LED indicator.
 */
void SET_GREEN_LED_OFF(void)
{
  gpio_set_level(led1_pin, 0);
}
/**
 * @brief  Turns on the red LED indicator.
 */
void SET_RED_LED_ON(void)
{
  gpio_set_level(led2_pin, 1);
}
/**
 * @brief  Turns off the red LED indicator.
 */
void SET_RED_LED_OFF(void)
{
  gpio_set_level(led2_pin, 0);
}

/**
 * @brief  Green LED blink task. Loops, alternately turning the green LED on and off at 500ms intervals, to indicate that the network data reporting process is in progress; the task itself never exits.
 * @param  pvParameters Parameter passed in when the FreeRTOS task is created (unused)
 */
void G_Led_Task(void *pvParameters)
{
  for(;;)
  {
    SET_GREEN_LED_ON();
    osi_Sleep(500);  //delay 500ms
      
    SET_GREEN_LED_OFF();
    osi_Sleep(500);  //delay 500ms
  }
}

/**
 * @brief  Button 1 interrupt handling task. Blocks waiting for the button interrupt sync signal; after receiving it, delays briefly for debouncing, and if the button is still detected as pressed or a wake-up flag is set, triggers the buzzer as an audible alert; then clears the sync signal and loops waiting for the next interrupt.
 * @param  pvParameters Parameter passed in when the FreeRTOS task is created (unused)
 */
void SW1_Task( void *pvParameters )
{
  for(;;)
  {
    osi_SyncObjWait(&SW1_Binary,OSI_WAIT_FOREVER);  //Waite Button GPIO Interrupt Message 

    osi_Sleep(10);
    if((gpio_get_level(sw1int_pin))||(sw_wakeup==1))
    {
      sw_wakeup=0;
      buzzer_on(200);
    }
    osi_SyncObjClear(&SW1_Binary);  //clear task message
  }
}

/**
 * @brief  Handles the reason the device woke from deep sleep. If the wake-up cause is the EXT1 external pin and it is determined to have been triggered by button 1 (sw1int_pin), and the device is not currently in a power-on state, sets the software wake-up flag and sends the sync signal to the button 1 task from ISR context.
 */
void WakeUp_Process(void)
{
  uint64_t wakeup_pin_mask;
  unsigned long ret_val = esp_sleep_get_wakeup_cause();
  switch(ret_val)
  {
    case ESP_SLEEP_WAKEUP_EXT1:
    {
      wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
      if(((wakeup_pin_mask>>sw1int_pin)&0x01)&&(dev_power_on==0))
      {
        ESP_LOGI(TAG, "%d,sw1 int", __LINE__);
        sw_wakeup = 1;
        osi_SyncObjSignalFromISR(&SW1_Binary);
      }
    }
    break;
    default:
    break;
  }
}

/**
 * @brief  Puts the device into deep sleep mode, enabling both the timer wake-up source and the button 1 (EXT1, level-triggered) wake-up source.
 * @param  slp_time Sleep duration for the timer wake-up, in seconds
 */
void Enter_Sleep(unsigned long slp_time)
{
  ESP_LOGI(TAG,"slp_time=%ld", slp_time);
  esp_sleep_enable_timer_wakeup(slp_time * 1000000);  //
  const int ext_wakeup_pin_1 = sw1int_pin;
  const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin_1;
  esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
  ESP_LOGI(TAG, "SET BUTTON1 wakeup");
	esp_deep_sleep_start();
}

/**
 * @brief  Builds the JSON payload used for the time-synchronization HTTP request, containing only the device PID and serial number, and copies the serialized string into the caller-provided buffer.
 * @param  read_buf Output buffer that receives the serialized JSON string.
 * @param  buf_len Size of read_buf, in bytes; the serialized string is only copied in if it fits within this length.
 */
void Device_PostData_Read(char *read_buf,uint16_t buf_len)
{
  char *out_buf;
  cJSON *pJsonRoot;
  pJsonRoot=cJSON_CreateObject();
  cJSON_AddStringToObject(pJsonRoot,"pid",USR_PID);
  cJSON_AddStringToObject(pJsonRoot,"sn",USR_SN);
  out_buf = cJSON_PrintUnformatted(pJsonRoot);  
  if(strlen(out_buf)<buf_len) mem_copy(read_buf,out_buf,strlen(out_buf)); 
  free(out_buf);
  cJSON_Delete(pJsonRoot);  //delete cjson root
}

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
 * @brief  Drains the sensor message queue and builds the JSON payload used for the data-reporting HTTP request (device PID, serial number, timestamp, and a "payloads" array with one field/value entry per queued sensor message), then copies the serialized string into the caller-provided buffer.
 * @param  read_buf Output buffer that receives the serialized JSON string.
 * @param  buf_len Size of read_buf, in bytes; the serialized string is only copied in if it fits within this length.
 */
void Sensors_PostData_Read(char *read_buf,uint16_t buf_len)
{
  char *out_buf;
  cJSON *pJsonRoot;
  OsiReturnVal_e msg_result = OSI_FAILURE;
  SensorMessage sMsg={0};
  char field[9]={0};
  sMsg.ts = Read_UnixTime();
  pJsonRoot=cJSON_CreateObject();
  cJSON_AddStringToObject(pJsonRoot,"pid",USR_PID);
  cJSON_AddStringToObject(pJsonRoot,"sn",USR_SN);
  cJSON_AddNumberToObject(pJsonRoot,"ts",sMsg.ts);
  cJSON *json_arry,*json_arrys = cJSON_CreateArray();
  cJSON_AddItemToObject(pJsonRoot,"payloads",json_arrys);
  for(uint8_t j=0;j<USR_POST_DATA_SUM;j++)
  {
    msg_result = osi_MsgQRead(&Data_Queue,&sMsg,OSI_NO_WAIT);  //read field Value Message
    if(msg_result != OSI_OK) break;
    mem_set(field,0,sizeof(field));
    snprintf(field,sizeof(field),"field%d",sMsg.sensornum);  //fields number
    json_arry = cJSON_CreateObject();
    cJSON_AddItemToArray(json_arrys,json_arry);
    cJSON_AddNumberToObject(json_arry,"ts",sMsg.ts);  //
    cJSON_AddNumberToObject(json_arry,field,sMsg.sensorval);
  }
  out_buf = cJSON_PrintUnformatted(pJsonRoot);
  if(strlen(out_buf)<buf_len) mem_copy(read_buf,out_buf,strlen(out_buf)); 
  free(out_buf);
  cJSON_Delete(pJsonRoot);  //delete cjson root
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
      ESP_LOGI(TAG, "http_tx_buf heap_caps_malloc failed %d", __LINE__);
    }
    http_rx_buf = heap_caps_malloc(HTTP_RX_BUFF_LEN, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);  //Allocate HTTP response buffer from SPIRAM
    if(http_rx_buf==NULL)
    {
      ESP_LOGI(TAG, "http_rx_buf heap_caps_malloc failed %d", __LINE__);
    }
    if((http_tx_buf!=NULL)&&(http_rx_buf!=NULL))
    {
      res_val=WiFi_Connect(USR_SSID, USR_PASSWORD,USR_CONCTRY_CODE);  //Connect to the AP
      if(res_val == SUCCESS)
      {
        if(Read_UnixTime()<1767196800)  //before 2026-1-1,update time - Read_UnixTime() gets current local time from RTC
        {
          mem_set(http_tx_buf,0,HTTP_TX_BUFF_LEN);
          mem_set(http_rx_buf,0,HTTP_RX_BUFF_LEN);

          Device_PostData_Read(http_tx_buf,HTTP_TX_BUFF_LEN);  //Build the time-sync request JSON payload
          
          //Send time-sync request and parse the response
          res_val = HTTP_Post_Method(USR_HTTP_HOST,USR_HTTP_TIME_URL,USR_HTTP_PORT,http_tx_buf,strlen(http_tx_buf),http_rx_buf,HTTP_RX_BUFF_LEN,Parse_Response);  
          if(res_val == SUCCESS)
          {
            ESP_LOGI(TAG, "%d,update time success.", __LINE__);
          }
          else
          {
            ESP_LOGI(TAG, "%d,update time failed,code=%d.", __LINE__,res_val);
          }
        }

        mem_set(http_tx_buf,0,HTTP_TX_BUFF_LEN);
        mem_set(http_rx_buf,0,HTTP_RX_BUFF_LEN);
        
        Sensors_Data_Update();  //Sample all sensors and enqueue readings
        Sensors_PostData_Read(http_tx_buf,HTTP_TX_BUFF_LEN);  //Build the data-report request JSON payload from the queue
        
        //Send sensor data report and parse the response
        res_val = HTTP_Post_Method(USR_HTTP_HOST,USR_HTTP_DATA_URL,USR_HTTP_PORT,http_tx_buf,strlen(http_tx_buf),http_rx_buf,HTTP_RX_BUFF_LEN,Parse_Response);
        if(res_val == SUCCESS)
        {
          ESP_LOGI(TAG, "%d,post data success.", __LINE__);
        } 
        else
        {
          ESP_LOGI(TAG, "%d,post data failed,code=%d.", __LINE__,res_val);
        }
      }
      else
      {
        ESP_LOGI(TAG, "WiFi_Connect failed! \n");
      }
      WiFi_Disconnect();  //Disconnect from the AP
      free(http_tx_buf);
      free(http_rx_buf);
    }
    Enter_Sleep(DEFAULT_FN);  //Enter deep sleep until next timer/button wake-up
  }
}

/**
 * @brief  Program entry point. Initializes NVS storage (erasing and reinitializing on failure), creates the event group, the button sync object, and the sensor message queue, and completes peripheral pin and I2C bus initialization; determines whether this is a power-on boot based on the reset reason, and if so, sounds the buzzer and completes RTC clock and light sensor initialization; processes the sleep wake-up source, then creates the button interrupt task, the green LED blink task, and the main business task.
 */
void app_main(void)
{
  esp_err_t ret = nvs_flash_init();  //Initialize NVS
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
  {
    ret = nvs_flash_erase();  //Erase NVS partition
    if(ret!=ESP_OK)
    {
      ESP_LOGI(TAG, "%dv,nvs_flash_erase failed.", __LINE__);
    }
    ret = nvs_flash_init();  //Re-initialize NVS after erase
  }
  if(ret!=ESP_OK)
  {
    ESP_LOGI(TAG, "%dv,nvs_flash_init failed.", __LINE__);
  }

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
  }
  WakeUp_Process();  //Handle deep-sleep wake-up cause

  osi_TaskCreate(SW1_Task, NULL,2048,NULL, 9, NULL);  //Create Button Interrupt_Task 
  osi_TaskCreate(G_Led_Task, NULL,2048, NULL, 7,NULL);  //Create GREEN LED Blink Task
  osi_TaskCreate(Main_Task, NULL,8192,NULL, 5, NULL);  //Create net Task
}

/*******************************************************************************
                                      END         
*******************************************************************************/




