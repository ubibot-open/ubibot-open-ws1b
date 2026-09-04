/*******************************************************************************
  * @file       Server-Issued Device Commands (protocol §9)
  * @author
  * @version
  * @date
  * @brief      See command.h.
  ******************************************************************************
  * @attention
  *
  *
*******************************************************************************/

/*-------------------------------- Includes ----------------------------------*/
#include "command.h"

#include <string.h>

#include "cJSON.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs.h"

#include "MsgType.h"

#define TAG "command"

// NVS namespace/key for the commanded report interval. Key kept <=15
// bytes (the NVS key length limit).
#define CMD_NVS_NAMESPACE   "ubcmd"
#define CMD_KEY_REPORT_IVL  "report_ivl_s"

// Active report interval, RAM-only -- reloaded from NVS (falling back to
// DEFAULT_FN) every boot by Command_Init(), same reasoning as
// provisioning.c's active WiFi/server config: deep sleep wipes RAM, so
// this can't be assumed to survive a wake-up on its own.
static uint32_t s_report_interval_s;

void Command_Init(void)
{
  s_report_interval_s = DEFAULT_FN;

  nvs_handle_t h;
  if (nvs_open(CMD_NVS_NAMESPACE, NVS_READONLY, &h) == ESP_OK)
  {
    uint32_t v;
    if ((nvs_get_u32(h, CMD_KEY_REPORT_IVL, &v) == ESP_OK) && (v > 0))
    {
      s_report_interval_s = v;
    }
    nvs_close(h);
  }

  ESP_LOGI(TAG, "active report interval: %lus", (unsigned long)s_report_interval_s);
}

/**
 * @brief  Persists a new report interval to NVS and, only once that
 *         succeeds, updates the in-RAM value Command_GetReportIntervalSeconds
 *         returns -- so a failed NVS write never leaves RAM and storage
 *         disagreeing about what's "active".
 */
static void command_save_report_interval(uint32_t seconds)
{
  nvs_handle_t h;
  esp_err_t err = nvs_open(CMD_NVS_NAMESPACE, NVS_READWRITE, &h);
  if (err != ESP_OK)
  {
    ESP_LOGW(TAG, "failed to open NVS to persist report interval");
    return;
  }
  err = nvs_set_u32(h, CMD_KEY_REPORT_IVL, seconds);
  if (err == ESP_OK)
  {
    err = nvs_commit(h);
  }
  nvs_close(h);
  if (err != ESP_OK)
  {
    ESP_LOGW(TAG, "failed to persist report interval to NVS");
    return;
  }
  s_report_interval_s = seconds;
}

void Command_HandleResponse(const cJSON *root)
{
  if (root == NULL)
  {
    return;
  }

  cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
  if ((cmd == NULL) || !cJSON_IsObject(cmd))
  {
    return;  // no pending command -- the overwhelming common case
  }

  cJSON *action = cJSON_GetObjectItem(cmd, "action");
  if (!cJSON_IsString(action) || (action->valuestring == NULL))
  {
    ESP_LOGW(TAG, "cmd object missing a string \"action\"");
    return;
  }

  if (strcmp(action->valuestring, "reboot") == 0)
  {
    ESP_LOGW(TAG, "server requested reboot -- restarting now");
    esp_restart();  // never returns
  }
  else if (strcmp(action->valuestring, "set_interval") == 0)
  {
    cJSON *seconds = cJSON_GetObjectItem(cmd, "seconds");
    if (!cJSON_IsNumber(seconds) || (seconds->valuedouble <= 0))
    {
      ESP_LOGW(TAG, "set_interval command missing a positive \"seconds\"");
      return;
    }
    ESP_LOGI(TAG, "server requested report interval: %ds (applies next cycle)", (int)seconds->valuedouble);
    command_save_report_interval((uint32_t)seconds->valuedouble);
  }
  else
  {
    ESP_LOGW(TAG, "unrecognized cmd.action: %s", action->valuestring);
  }
}

uint32_t Command_GetReportIntervalSeconds(void)
{
  return s_report_interval_s;
}

/*******************************************************************************
                                      END
*******************************************************************************/
