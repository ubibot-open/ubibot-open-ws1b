/*******************************************************************************
  * @file       HTTP Request JSON Payload Builders
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
#include "json_payload.h"
#include "string.h"
#include "stdlib.h"
#include "osi.h"
#include "esp_log.h"
#include "cJSON.h"
#include "PCF8563.h"
#include "MsgType.h"
#include "provisioning.h"

#define TAG "json_payload"

extern OsiMsgQ_t Data_Queue;  //Used field data save, defined in main.c

/**
 * @brief  Builds the JSON payload used for the time-synchronization HTTP request, containing only the device PID and serial number (the latter via Provision_GetSN() -- a SetupDevice command over serial, protocol §1.2, takes priority over the menuconfig CONFIG_USR_SN baked in at build time), and copies the serialized string into the caller-provided buffer.
 * @param  read_buf Output buffer that receives the serialized JSON string.
 * @param  buf_len Size of read_buf, in bytes; the serialized string is only copied in if it fits within this length.
 */
void Device_PostData_Read(char *read_buf,uint16_t buf_len)
{
  char *out_buf;
  cJSON *pJsonRoot;
  pJsonRoot=cJSON_CreateObject();
  cJSON_AddStringToObject(pJsonRoot,"pid",USR_PID);
  cJSON_AddStringToObject(pJsonRoot,"sn",Provision_GetSN());
  out_buf = cJSON_PrintUnformatted(pJsonRoot);
  if(strlen(out_buf)<buf_len)
  {
    mem_copy(read_buf,out_buf,strlen(out_buf));
  }
  else
  {
    ESP_LOGW(TAG, "time-sync payload (%d bytes) truncated to fit %d-byte buffer", (int)strlen(out_buf), buf_len);
  }
  free(out_buf);
  cJSON_Delete(pJsonRoot);  //delete cjson root
}

/**
 * @brief  Drains the sensor message queue and builds the JSON payload used for the data-reporting HTTP request (device PID, serial number -- via Provision_GetSN(), see Device_PostData_Read's comment above --, timestamp, and a "payloads" array with one field/value entry per queued sensor message), then copies the serialized string into the caller-provided buffer.
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
  cJSON_AddStringToObject(pJsonRoot,"sn",Provision_GetSN());
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
  if(strlen(out_buf)<buf_len)
  {
    mem_copy(read_buf,out_buf,strlen(out_buf));
  }
  else
  {
    ESP_LOGW(TAG, "data-report payload (%d bytes) truncated to fit %d-byte buffer", (int)strlen(out_buf), buf_len);
  }
  free(out_buf);
  cJSON_Delete(pJsonRoot);  //delete cjson root
}

/*******************************************************************************
                                      END
*******************************************************************************/
