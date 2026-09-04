/*******************************************************************************
  * @file       Serial Provisioning (protocol §1.2: SetupWifi / SetupServer)
  * @author
  * @version
  * @date
  * @brief      Lets a technician set the device's WiFi credentials, server
  *             address, and serial number over the same USB/UART console
  *             used for log output and `idf.py monitor`, instead of baking
  *             them into `sdkconfig` at compile time -- in particular, this
  *             is what lets one compiled firmware image (one pid, no sn
  *             baked in) be flashed onto a whole production batch, with
  *             each physical unit's own sn set afterward over serial
  *             instead of recompiling per unit. Values are persisted to
  *             NVS and take priority over the `idf.py menuconfig` (Kconfig)
  *             defaults on every subsequent boot; menuconfig values remain
  *             the factory default for a device that has never been
  *             provisioned this way.
  ******************************************************************************
  * @attention
  *
  *  Bluetooth provisioning (protocol §1.1) is not supported by this
  *  firmware; this is the only supported way to configure WiFi/server
  *  settings without a full rebuild+reflash.
  *
*******************************************************************************/

#ifndef PROVISIONING_H
#define PROVISIONING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// How long the provisioning window (see Provision_RunWindow) stays open
// after the last byte received before it gives up and lets the device get
// on with its normal WiFi-connect/report/sleep cycle -- long enough to type
// or paste a JSON line by hand, short enough not to burn battery on every
// power-on when nobody is actually there to provision the device.
#define PROV_WINDOW_IDLE_TIMEOUT_MS   5000

// Hard cap on the window's total duration regardless of activity, so a
// technician who keeps sending commands (or a noisy/stuck line) can't keep
// the device awake on USB power indefinitely.
#define PROV_WINDOW_MAX_TOTAL_MS      60000

/**
 * @brief  Loads the active WiFi/server configuration into RAM: starts from
 *         the `idf.py menuconfig` (Kconfig) compile-time defaults, then
 *         overrides any field that has a previously-provisioned value saved
 *         in NVS. Must be called once per boot (after nvs_flash_init, before
 *         the network task reads any of the Provision_Get* accessors below)
 *         -- deep sleep wipes RAM, so this has to happen fresh every wake.
 */
void Provision_Init(void);

/**
 * @brief  Blocks for up to `max_total_ms`, listening on the console UART for
 *         single-line JSON provisioning commands (`SetupWifi`/`SetupServer`/
 *         `SetupDevice`, protocol §1.2), applying and persisting each one it
 *         understands to NVS as it arrives and printing a one-line JSON
 *         ack/error back over the same UART. The window closes early once
 *         `idle_timeout_ms` passes with no byte received, so an unattended
 *         device does not wait the full `max_total_ms` on every power-on.
 * @param  idle_timeout_ms Milliseconds of silence after the last received
 *         byte before the window closes.
 * @param  max_total_ms Hard cap on the window's total duration regardless of
 *         activity.
 */
void Provision_RunWindow(uint32_t idle_timeout_ms, uint32_t max_total_ms);

/**
 * @brief  Returns the WiFi SSID to connect with: the most recently
 *         provisioned value if any, otherwise the Kconfig default. Valid
 *         only after Provision_Init().
 */
const char *Provision_GetWifiSsid(void);

/**
 * @brief  Returns the WiFi password to connect with: the most recently
 *         provisioned value if any, otherwise the Kconfig default. Valid
 *         only after Provision_Init().
 */
const char *Provision_GetWifiPassword(void);

/**
 * @brief  Returns the data server host (IP or domain) to report to: the
 *         most recently provisioned value if any, otherwise the Kconfig
 *         default. Valid only after Provision_Init().
 */
const char *Provision_GetHttpHost(void);

/**
 * @brief  Returns the data server port to report to: the most recently
 *         provisioned value if any, otherwise the Kconfig default. Valid
 *         only after Provision_Init().
 */
uint16_t Provision_GetHttpPort(void);

/**
 * @brief  Returns the device serial number to identify itself with (docs
 *         §3): the most recently provisioned value if any, otherwise the
 *         Kconfig default (`CONFIG_USR_SN`). Valid only after
 *         Provision_Init(). The product ID (pid) is not provisionable over
 *         serial -- it's expected to be shared by every device flashed
 *         from the same firmware build, unlike sn.
 */
const char *Provision_GetSN(void);

#ifdef __cplusplus
}
#endif

#endif // PROVISIONING_H

/*******************************************************************************
                                      END
*******************************************************************************/
