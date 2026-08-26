/*******************************************************************************
  * @file       HTTP Client Application Task
  * @author 
  * @version
  * @date 
  * @brief
  ******************************************************************************
  * @attention
  *
  *
*******************************************************************************/

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

/*-------------------------------- Includes ----------------------------------*/
#include "esp_http_client.h"

typedef int (*RespFunc)(char*response_buf);

/**
 * @brief  Parses the JSON data returned in the server's POST response: extracts the "c" (result code) field for log output, and extracts the "t" (timestamp) field to update the system's Unix time accordingly.
 * @param  ptr Pointer to the JSON-formatted string to parse (input).
 * @return FAILURE (-1) means the input pointer was NULL or JSON parsing failed.
 */
extern int Parse_Response(char *ptr);

/**
 * @brief  Issues an HTTP GET request to the specified host; once connected, reads the server response, and if a valid response is read, calls the callback function to process the response data.
 * @param  host Target server host address (IP or domain name)
 * @param  url Requested URL path
 * @param  port Target server port number
 * @param  resp_func Response data handling callback; invoked to parse the response content once a valid response is received
 * @param  rx_buf_len Length of the buffer used to receive HTTP response data, in bytes
 * @return Returns the result of resp_func or the HTTP response status code on success; returns -1 if allocating the receive buffer fails, opening the connection fails, or reading fails.
 */
extern int HTTP_Get_Method(char *host,char *url,uint16_t port,char *http_rx_buf,uint16_t rx_buf_len,RespFunc resp_func);

/**
 * @brief  Sends data (typically a JSON string) to the specified host via POST; once sending completes, reads the server response, and if a valid response is read, calls the callback function to process it.
 * @param  host Target server host address (IP or domain name)
 * @param  url Requested URL path
 * @param  port Target server port number
 * @param  pbuf Buffer holding the POST data to be sent
 * @param  pbuf_len Length of the data to be sent in pbuf, in bytes
 * @param  resp_func Response data handling callback; invoked to parse the response content once a valid response is received
 * @param  rx_buf_len Length of the buffer used to receive HTTP response data, in bytes
 * @return Returns the result of resp_func on success; returns -1 if allocating the receive buffer fails, opening the connection fails, or writing the data fails.
 */
extern int HTTP_Post_Method(char *host,char *url,uint16_t port,char *http_tx_buf,uint16_t tx_buf_len,char *http_rx_buf,uint16_t rx_buf_len,RespFunc resp_func);

#endif // HTTP_CLIENT_H

/*******************************************************************************
                                      END
*******************************************************************************/
