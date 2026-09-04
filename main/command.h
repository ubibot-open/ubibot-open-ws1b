/*******************************************************************************
  * @file       Server-Issued Device Commands (protocol §9)
  * @author
  * @version
  * @date
  * @brief      Handles the optional "cmd" object a data-report response may
  *             carry (protocol §9: an admin-queued command, delivered
  *             piggybacked on this device's own next report). Two actions
  *             are supported: "reboot" and "set_interval". Delivery is
  *             fire-and-forget from the server's point of view -- there is
  *             no ack -- so acting on a command here is the only
  *             confirmation that will ever exist that it was received.
  ******************************************************************************
  * @attention
  *
  *
*******************************************************************************/

#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>

#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Loads the active report interval into RAM: the most recently
 *         received `set_interval` command's value if any (persisted in
 *         NVS), otherwise the Kconfig/MsgType.h default (DEFAULT_FN). Must
 *         be called once per boot, same as Provision_Init() and for the
 *         same reason -- deep sleep wipes RAM, so nothing here can be
 *         assumed to survive between wake-ups except what's in NVS.
 */
void Command_Init(void);

/**
 * @brief  Looks for an optional "cmd" object on a parsed HTTP response body
 *         (protocol §9) and, if present, acts on it:
 *         - `{"action":"reboot"}`: restarts the device immediately
 *           (esp_restart(), does not return).
 *         - `{"action":"set_interval","seconds":N}`: persists N to NVS as
 *           the new report interval and applies it starting with the
 *           *next* sleep cycle (the one already under way finishes with
 *           whatever interval was active when it started).
 *         Any other/malformed "cmd" is logged and ignored. Safe to call on
 *         every response this device ever parses, including ones with no
 *         "cmd" field at all (the common case, and a no-op here).
 * @param  root Parsed JSON root object of the HTTP response body. May be
 *         NULL (treated as "no command").
 */
void Command_HandleResponse(const cJSON *root);

/**
 * @brief  Returns the report interval to sleep for between cycles: the
 *         most recently commanded value if any, otherwise DEFAULT_FN.
 *         Valid only after Command_Init().
 */
uint32_t Command_GetReportIntervalSeconds(void);

#ifdef __cplusplus
}
#endif

#endif // COMMAND_H

/*******************************************************************************
                                      END
*******************************************************************************/
