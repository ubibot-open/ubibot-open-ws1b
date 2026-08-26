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
#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

#define CONNECTED_BIT (1 << 0)    //wifi connected flag
#define WIFI_S_I_BIT (1 << 1)     //wifi start initialization flag
#define WIFI_I_BIT (1 << 2)       //wifi initialized flag
#define WIFI_S_BIT (1 << 3)       //wifi started flag
#define Nets_Group_ALL_BIT (CONNECTED_BIT|\
                             WIFI_S_I_BIT|\
                             WIFI_I_BIT|\
                             WIFI_S_BIT\
                            )
                            
/**
 * @brief  Initializes the WiFi module: sets the "initialization started" flag, creates the network interface and the default event loop, creates the default STA and AP network interfaces, registers the WiFi and IP event callback, initializes the WiFi driver with the default configuration and sets it to maximum power-saving mode, and finally sets the "initialization complete" flag.
 */
extern void WiFi_Init(void);

/**
 * @brief  Stops WiFi. If WiFi is currently in the started state, clears the started-state flag and calls esp_wifi_stop to turn off WiFi; if the WiFi driver has not been initialized, returns immediately; if WiFi itself is not started, simply prints an informational log.
 */
extern void WiFi_Stop(void);

/**
 * @brief  Starts WiFi and blocks waiting for it to connect successfully (i.e. waits for CONNECTED_BIT to be set), with the maximum wait time determined by DEFAULT_WIFI_CNT_TIMEOUT (30 seconds).
 * @return Returns SUCCESS(0) if connected successfully within the timeout period; returns FAILURE(-1) if still not connected when the timeout elapses.
 */
extern int WiFi_Connect(char *ssid, char *password,char *country_code);

/**
 * @brief  Disconnects the current WiFi STA connection and stops WiFi, used to release network resources and reduce power consumption after the data reporting process finishes.
 */
extern void WiFi_Disconnect(void);

#endif  //WIFI_CONNECT_H

/*******************************************************************************
                                      END         
*******************************************************************************/







