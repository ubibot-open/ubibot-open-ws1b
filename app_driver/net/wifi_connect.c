/*******************************************************************************
  * @file       WiFi config Application Task  
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
#include <stdlib.h>
#include "osi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "PCF8563.h"
#include "MsgType.h"
#include "wifi_connect.h"

#define TAG "wifi_connect"

extern OsiMsgQ_t Data_Queue;  //
extern EventGroupHandle_t Nets_Group;

bool scan_flag = false;
esp_netif_t *STA_netif_t;
esp_netif_t *AP_netif_t;

/**
 * @brief  Unified callback handler for WiFi and IP events. Depending on the event base and event ID, it handles AP station connect/disconnect, STA start/connected/disconnected, and IP-acquired events, maintains the corresponding bits in the network status event group, attempts to reconnect on disconnection based on the current state, and starts/stops the WiFi timeout timer on connection success/IP acquisition respectively.
 * @param  arg User argument passed in when the event handler was registered (unused)
 * @param  event_base The base class the event belongs to, e.g. WIFI_EVENT or IP_EVENT
 * @param  event_id The specific event type ID
 * @param  event_data Data associated with the event; its actual type depends on event_base and event_id (e.g. wifi_event_ap_staconnected_t, ip_event_got_ip_t, etc.)
 */
static void event_handler(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) 
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d",MAC2STR(event->mac), event->aid);
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) 
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d, reason:%d",MAC2STR(event->mac), event->aid, event->reason);
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "event_base == WIFI_EVENT,event_id == WIFI_EVENT_STA_START\r\n");
        if (scan_flag == false)
        {
            esp_wifi_connect();
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI(TAG, "event_base == WIFI_EVENT,event_id == WIFI_EVENT_STA_CONNECTED\r\n");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG,"event_base == WIFI_EVENT,event_id == WIFI_EVENT_STA_DISCONNECTED\r\n");
        wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGI(TAG, "Wi-Fi disconnected,reason:%d", event->reason);
        xEventGroupClearBits(Nets_Group, CONNECTED_BIT);
        if (scan_flag == false)
        {
            if ((xEventGroupGetBits(Nets_Group) & WIFI_S_BIT) == WIFI_S_BIT) esp_wifi_connect();
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(TAG,"event_base == IP_EVENT,event_id == IP_EVENT_STA_GOT_IP\r\n");
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(Nets_Group, CONNECTED_BIT);
    }
    else
    {
        ESP_LOGI(TAG, "event_base=%s,event_id=%d", event_base, event_id);
    }
}

/**
 * @brief  Initializes the WiFi module: sets the "initialization started" flag, creates the network interface and the default event loop, creates the default STA and AP network interfaces, registers the WiFi and IP event callback, initializes the WiFi driver with the default configuration and sets it to maximum power-saving mode, and finally sets the "initialization complete" flag.
 */
void WiFi_Init(void) //
{
    xEventGroupSetBits(Nets_Group, WIFI_S_I_BIT);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    STA_netif_t = esp_netif_create_default_wifi_sta();
    AP_netif_t = esp_netif_create_default_wifi_ap();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MAX_MODEM)); //

    xEventGroupSetBits(Nets_Group, WIFI_I_BIT);
}

/**
 * @brief  Stops WiFi. If WiFi is currently in the started state, clears the started-state flag and calls esp_wifi_stop to turn off WiFi; if the WiFi driver has not been initialized, returns immediately; if WiFi itself is not started, simply prints an informational log.
 */
void WiFi_Stop(void)
{
    if ((xEventGroupGetBits(Nets_Group) & WIFI_S_BIT) == WIFI_S_BIT)
    {
        xEventGroupClearBits(Nets_Group, WIFI_S_BIT);
        esp_err_t err = esp_wifi_stop();
        if (err == ESP_ERR_WIFI_NOT_INIT)
        {
            return;
        }
        ESP_LOGI(TAG, "turn off WIFI! \n");
    }
    else
    {
        ESP_LOGI(TAG, "WIFI not start! \n");
    }
}

/**
 * @brief  Starts WiFi and connects to the preset access point in STA mode. If WiFi has not yet finished initialization, initialization is performed first; if WiFi is already in the started state, it is stopped first and then reconfigured; the country code, STA SSID/password, and other connection parameters are then set, the mode is configured as STA and the DHCP client is started, and finally WiFi is started.
 */
int WiFi_Connect(char *ssid, char *password,char *country_code)
{
    esp_err_t err;
    wifi_ap_record_t wifidata_t;
    SensorMessage sMsg={0};

    if ((xEventGroupGetBits(Nets_Group) & WIFI_S_I_BIT) != WIFI_S_I_BIT)
    {
        WiFi_Init();
    }

    ESP_LOGI(TAG, "%d,set_user_wifi", __LINE__);
    if ((xEventGroupGetBits(Nets_Group) & WIFI_S_BIT) == WIFI_S_BIT)
    {
        err = esp_wifi_stop();
        if (err == ESP_ERR_WIFI_NOT_INIT)
        {
            return FAILURE;
        }
        ESP_ERROR_CHECK(err);
    }
    xEventGroupSetBits(Nets_Group, WIFI_S_BIT);

    err = esp_wifi_set_country_code(country_code, 1);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set country code");
    }
    wifi_config_t s_staconf;
    memset(&s_staconf.sta, 0, sizeof(s_staconf));
    mem_copy((char *)s_staconf.sta.ssid, ssid,strlen(ssid));
    mem_copy((char *)s_staconf.sta.password, password,strlen(password));
    s_staconf.sta.scan_method = 1;
    s_staconf.sta.channel = 0;
    s_staconf.sta.sort_method = 0;
    s_staconf.sta.listen_interval = 0;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &s_staconf));
    esp_wifi_set_band_mode(WIFI_BAND_MODE_AUTO);
    esp_netif_dhcpc_start(STA_netif_t);

    ESP_LOGI(TAG, "%d,start_user_wifi", __LINE__);
    ESP_ERROR_CHECK(esp_wifi_start());

    // 30s timeout
    if ((xEventGroupWaitBits(Nets_Group, CONNECTED_BIT, false, true, DEFAULT_WIFI_CNT_TIMEOUT*1000 / portTICK_PERIOD_MS) & CONNECTED_BIT) == CONNECTED_BIT)
    {
        esp_wifi_sta_get_ap_info(&wifidata_t);
        ESP_LOGI(TAG, "%d,wifidata_t.rssi=%d.", __LINE__,wifidata_t.rssi);
        sMsg.ts = Read_UnixTime();
        sMsg.sensornum=RSSI_NUM;         //Message Number
        sMsg.sensorval=wifidata_t.rssi;        //Message Value
        osi_MsgQWrite(&Data_Queue,&sMsg,OSI_SAVE_WAIT);   //Send Message
        return SUCCESS;
    }
    else
    {   
        return FAILURE;
    }
}

/**
 * @brief  Disconnects the current WiFi STA connection and stops WiFi, used to release network resources and reduce power consumption after the data reporting process finishes.
 */
void WiFi_Disconnect(void)
{
    esp_wifi_disconnect();
    WiFi_Stop();
}

/*******************************************************************************
                                      END         
*******************************************************************************/








