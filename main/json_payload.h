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
#ifndef JSON_PAYLOAD_H
#define JSON_PAYLOAD_H

#include "stdint.h"

/**
 * @brief  Builds the JSON payload used for the time-synchronization HTTP request, containing only the device PID and serial number, and copies the serialized string into the caller-provided buffer.
 * @param  read_buf Output buffer that receives the serialized JSON string.
 * @param  buf_len Size of read_buf, in bytes; the serialized string is only copied in if it fits within this length.
 */
void Device_PostData_Read(char *read_buf,uint16_t buf_len);

/**
 * @brief  Drains the sensor message queue and builds the JSON payload used for the data-reporting HTTP request (device PID, serial number, timestamp, and a "payloads" array with one field/value entry per queued sensor message), then copies the serialized string into the caller-provided buffer.
 * @param  read_buf Output buffer that receives the serialized JSON string.
 * @param  buf_len Size of read_buf, in bytes; the serialized string is only copied in if it fits within this length.
 */
void Sensors_PostData_Read(char *read_buf,uint16_t buf_len);

#endif //  JSON_PAYLOAD_H

/*******************************************************************************
                                      END
*******************************************************************************/
