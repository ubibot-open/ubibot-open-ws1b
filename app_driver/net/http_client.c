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

/*-------------------------------- Includes ----------------------------------*/
#include <string.h>
#include <stdlib.h>
#include "osi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "http_client.h"
#include "PCF8563.h"
#include "cJSON.h"

#define TAG "http_client"

/**
 * @brief  HTTP client event callback. Prints debug logs depending on the HTTP event type, retrieves and prints the underlying mbedTLS error information when the connection is disconnected, and configures the client to follow redirects on a redirect event.
 * @param  evt Pointer to the HTTP client event structure, containing the event type (event_id), the associated HTTP client handle, and event-related data (such as response headers, data, etc.)
 * @return Always returns ESP_OK, indicating the event has been handled.
 */
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
  switch(evt->event_id) 
  {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_STATUS_CODE:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_STATUS_CODE");
        break;
    case HTTP_EVENT_ON_HEADERS_COMPLETE:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADERS_COMPLETE");
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
        if (err != 0) {
            ESP_LOGW(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGW(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        esp_http_client_set_redirection(evt->client);
        break;
    default:
        ESP_LOGD(TAG, "unhandled http event id=%d", evt->event_id);
        break;
  }
  return ESP_OK;
}

/**
 * @brief  Parses the JSON data returned in the server's POST response: extracts the "c" (result code) field for log output, and extracts the "t" (timestamp) field to update the system's Unix time accordingly.
 * @param  ptr Pointer to the JSON-formatted string to parse (input).
 * @return FAILURE (-1) means the input pointer was NULL or JSON parsing failed.
 */
int Parse_Response(char *ptr)
{
  int res_val = FAILURE;
  if(NULL == ptr)
  {
    return res_val;
  }
  cJSON *pJson = cJSON_Parse(ptr);
  if(pJson ==NULL )
  { 
    return res_val;
  }
  cJSON *pSub = cJSON_GetObjectItem(pJson, "c");  //c,0 success
  if(NULL!=pSub)
  {
    res_val=pSub->valueint;
    ESP_LOGI(TAG, "\"c\":%d\r\n",pSub->valueint);
  }  
  pSub = cJSON_GetObjectItem(pJson, "t");  //time
  if(NULL!=pSub)
  { 
    ESP_LOGI(TAG, "\"t\":%d\r\n",pSub->valueint);
    Update_UnixTime(pSub->valueint);  //update time
  }
  cJSON_Delete(pJson);  //delete pJson 
  return res_val;
}

/**
 * @brief  Reads the response headers and body from an already-connected HTTP client and writes them into the specified buffer.
 * @param  httpClient HTTP client handle with an open connection
 * @param  http_rx_buf Buffer used to store the read HTTP response data (output parameter)
 * @param  rx_buf_len Maximum length of the http_rx_buf buffer, in bytes
 * @return Returns the HTTP response status code (e.g. 200) on success; returns -1 if fetching the headers or reading the data fails.
 */
static int readResponse(esp_http_client_handle_t httpClient,char *http_rx_buf,uint16_t rx_buf_len)
{
  long lRetVal = -1;
  int content_length = esp_http_client_fetch_headers(httpClient);
  if (content_length >= 0)
  {
    ESP_LOGI(TAG, "content_length=%d",content_length);
    lRetVal = esp_http_client_get_status_code(httpClient);
    int data_read = esp_http_client_read_response(httpClient, http_rx_buf, rx_buf_len);
    if (data_read >= 0)
    {
      ESP_LOGI(TAG, "HTTP Status = %ld, data_read = %d,GET Request READ:\n%s",lRetVal,data_read,http_rx_buf);
    }
    else
    {
      ESP_LOGE(TAG, "Failed to read response, data_read = %d",data_read);
    }
  }
  else
  {
    ESP_LOGE(TAG, "HTTP client fetch headers failed.content_length=%d",content_length);
  }
  return lRetVal;
}

/**
 * @brief  Issues an HTTP GET request to the specified host; once connected, reads the server response, and if a valid response is read, calls the callback function to process the response data.
 * @param  host Target server host address (IP or domain name)
 * @param  url Requested URL path
 * @param  port Target server port number
 * @param  resp_func Response data handling callback; invoked to parse the response content once a valid response is received
 * @param  rx_buf_len Length of the buffer used to receive HTTP response data, in bytes
 * @return Returns the result of resp_func or the HTTP response status code on success; returns -1 if allocating the receive buffer fails, opening the connection fails, or reading fails.
 */
int HTTP_Get_Method(char *host,char *url,uint16_t port,char *http_rx_buf,uint16_t rx_buf_len,RespFunc resp_func)
{
  int lRetVal = -1;
  esp_http_client_config_t config = {
    .transport_type = HTTP_TRANSPORT_OVER_TCP,
    .host = host,
    .port = port,
    .path = "/",
    .timeout_ms = 30000,
    .event_handler = _http_event_handler,
  };
  
  esp_http_client_handle_t httpClient = esp_http_client_init(&config);
  if(httpClient!=NULL) 
  {
    esp_http_client_set_url(httpClient, url);
    esp_http_client_set_method(httpClient, HTTP_METHOD_GET);
    lRetVal = esp_http_client_open(httpClient, 0);
    if (lRetVal == ESP_OK)
    {
      ESP_LOGI(TAG, "Connection to server successfully\r\n");
      memset(http_rx_buf,0,rx_buf_len);
      lRetVal = readResponse(httpClient,http_rx_buf,rx_buf_len);
      if(lRetVal>0)
      {
        lRetVal = resp_func(http_rx_buf);
      }
      esp_http_client_close(httpClient);
      esp_http_client_cleanup(httpClient);
    }
    else
    {
      ESP_LOGE(TAG, "esp_http_client_open FAIL:%s", esp_err_to_name(lRetVal));
    }
  }
  return lRetVal;
}

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
int HTTP_Post_Method(char *host,char *url,uint16_t port,char *http_tx_buf,uint16_t tx_buf_len,char *http_rx_buf,uint16_t rx_buf_len,RespFunc resp_func)
{
  long lRetVal = -1;
  esp_http_client_config_t config = {
    .transport_type = HTTP_TRANSPORT_OVER_TCP,
    .host = host,
    .port = port,
    .path = "/",
    .timeout_ms = 30000,
    .event_handler = _http_event_handler,
  };

  ESP_LOGI(TAG, "tx_buf_len=%d,http_tx_buf: %s",tx_buf_len,http_tx_buf);
  esp_http_client_handle_t httpClient = esp_http_client_init(&config);
  if(httpClient!=NULL) 
  {
    esp_http_client_set_url(httpClient, url);
    esp_http_client_set_method(httpClient, HTTP_METHOD_POST);
    esp_http_client_set_header(httpClient, "Content-Type", "application/json");

    lRetVal = esp_http_client_open(httpClient, tx_buf_len);  //
    if (lRetVal == ESP_OK)
    {
      lRetVal = esp_http_client_write(httpClient, http_tx_buf, tx_buf_len); //
      if (lRetVal >= 0)
      {
        ESP_LOGI(TAG, "http Write success %d", __LINE__);
        memset(http_rx_buf,0,rx_buf_len);
        lRetVal = readResponse(httpClient,http_rx_buf,rx_buf_len); //read respose from server
        if(lRetVal>0)
        {
          lRetVal = resp_func(http_rx_buf);
        }
        else
        {
          ESP_LOGE(TAG, "http Read failed %d", __LINE__);
        }
        esp_http_client_close(httpClient);
        esp_http_client_cleanup(httpClient);
      }
      else
      {
        ESP_LOGE(TAG, "http Write failed %d", __LINE__);
      }
    }
    else
    {
      ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(lRetVal));
    }
  }
  return lRetVal;
}

/*******************************************************************************
                                      END         
*******************************************************************************/
