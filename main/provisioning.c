/*******************************************************************************
  * @file       Serial Provisioning (protocol §1.2: SetupWifi / SetupServer /
  *             SetupDevice)
  * @author
  * @version
  * @date
  * @brief      See provisioning.h.
  ******************************************************************************
  * @attention
  *
  *
*******************************************************************************/

/*-------------------------------- Includes ----------------------------------*/
#include "provisioning.h"

#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"

#include "MsgType.h"

#define TAG "provisioning"

// NVS namespace + keys for provisioned values. Key names are kept <=15
// bytes (the NVS key length limit).
#define PROV_NVS_NAMESPACE   "ubprov"
#define PROV_KEY_WIFI_SSID   "wifi_ssid"
#define PROV_KEY_WIFI_PASS   "wifi_pass"
#define PROV_KEY_WIFI_TYPE   "wifi_type"
#define PROV_KEY_HTTP_HOST   "http_host"
#define PROV_KEY_HTTP_PORT   "http_port"
#define PROV_KEY_SN          "sn"

// Sized generously above real-world limits (WiFi SSID <=32 bytes, WPA2
// password <=64 bytes) rather than tightly, so a slightly-too-long value is
// rejected with a clear error instead of silently truncated.
#define PROV_SSID_MAX_LEN     33
#define PROV_PASSWORD_MAX_LEN 65
#define PROV_TYPE_MAX_LEN     16
#define PROV_HOST_MAX_LEN     128
// Matches the server's Device.SN column (docs §3: size:64) plus a
// terminator.
#define PROV_SN_MAX_LEN       65

#define PROV_UART_NUM       (CONFIG_ESP_CONSOLE_UART_NUM)
#define PROV_UART_BAUDRATE  (CONFIG_ESP_CONSOLE_UART_BAUDRATE)
#define PROV_UART_RX_BUF    1024
#define PROV_LINE_MAX_LEN   256

// Active configuration, RAM-only -- reloaded from Kconfig defaults + NVS
// overrides every boot by Provision_Init() (deep sleep wipes RAM, so nothing
// here can be assumed to survive between wake-ups; NVS is the only thing
// that does).
static char s_wifi_ssid[PROV_SSID_MAX_LEN];
static char s_wifi_password[PROV_PASSWORD_MAX_LEN];
static char s_wifi_type[PROV_TYPE_MAX_LEN];
static char s_http_host[PROV_HOST_MAX_LEN];
static uint16_t s_http_port;
static char s_sn[PROV_SN_MAX_LEN];

/**
 * @brief  Installs the UART driver on the console UART (idempotent -- safe
 *         to call even if a previous provisioning window already installed
 *         it) so Provision_RunWindow can read bytes with uart_read_bytes().
 *         Baud rate/frame format matches CONFIG_ESP_CONSOLE_UART_BAUDRATE
 *         (the same settings idf.py monitor / ubibot-serial-sync already
 *         connect at for log output), and pins are left as whatever the
 *         console already uses.
 */
static void prov_uart_init(void)
{
  if (uart_is_driver_installed(PROV_UART_NUM))
  {
    return;
  }
  uart_config_t cfg = {
    .baud_rate = PROV_UART_BAUDRATE,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
  };
  ESP_ERROR_CHECK(uart_param_config(PROV_UART_NUM, &cfg));
  ESP_ERROR_CHECK(uart_driver_install(PROV_UART_NUM, PROV_UART_RX_BUF, 0, 0, NULL, 0));
}

/**
 * @brief  Opens the provisioning NVS namespace read-write, writes the given
 *         WiFi fields, commits, and closes it.
 * @return ESP_OK on success; the first failing NVS call's error otherwise.
 */
static esp_err_t prov_save_wifi(const char *ssid, const char *password, const char *type)
{
  nvs_handle_t h;
  esp_err_t err = nvs_open(PROV_NVS_NAMESPACE, NVS_READWRITE, &h);
  if (err != ESP_OK)
  {
    return err;
  }
  err = nvs_set_str(h, PROV_KEY_WIFI_SSID, ssid);
  if (err == ESP_OK) err = nvs_set_str(h, PROV_KEY_WIFI_PASS, password);
  if (err == ESP_OK) err = nvs_set_str(h, PROV_KEY_WIFI_TYPE, type);
  if (err == ESP_OK) err = nvs_commit(h);
  nvs_close(h);
  return err;
}

/**
 * @brief  Opens the provisioning NVS namespace read-write, writes the given
 *         server fields, commits, and closes it.
 * @return ESP_OK on success; the first failing NVS call's error otherwise.
 */
static esp_err_t prov_save_server(const char *host, uint16_t port)
{
  nvs_handle_t h;
  esp_err_t err = nvs_open(PROV_NVS_NAMESPACE, NVS_READWRITE, &h);
  if (err != ESP_OK)
  {
    return err;
  }
  err = nvs_set_str(h, PROV_KEY_HTTP_HOST, host);
  if (err == ESP_OK) err = nvs_set_u16(h, PROV_KEY_HTTP_PORT, port);
  if (err == ESP_OK) err = nvs_commit(h);
  nvs_close(h);
  return err;
}

/**
 * @brief  Opens the provisioning NVS namespace read-write, writes sn,
 *         commits, and closes it.
 * @return ESP_OK on success; the first failing NVS call's error otherwise.
 */
static esp_err_t prov_save_sn(const char *sn)
{
  nvs_handle_t h;
  esp_err_t err = nvs_open(PROV_NVS_NAMESPACE, NVS_READWRITE, &h);
  if (err != ESP_OK)
  {
    return err;
  }
  err = nvs_set_str(h, PROV_KEY_SN, sn);
  if (err == ESP_OK) err = nvs_commit(h);
  nvs_close(h);
  return err;
}

/**
 * @brief  Handles one parsed `{"command":"SetupWifi",...}` line (protocol
 *         §1.2): validates `ssid`/`password`/`type`, persists them to NVS,
 *         updates the in-RAM active config so the change also applies to
 *         the current boot (not just the next one), and prints a one-line
 *         JSON ack/error back over the console UART.
 * @param  root Parsed JSON object for the command line (not consumed/freed
 *         here -- the caller owns it).
 */
static void prov_handle_setup_wifi(const cJSON *root)
{
  const cJSON *ssid = cJSON_GetObjectItemCaseSensitive(root, "ssid");
  const cJSON *password = cJSON_GetObjectItemCaseSensitive(root, "password");
  const cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");

  if (!cJSON_IsString(ssid) || ssid->valuestring[0] == '\0')
  {
    printf("{\"c\":1,\"msg\":\"ssid is required\"}\r\n");
    return;
  }
  if (strlen(ssid->valuestring) >= sizeof(s_wifi_ssid))
  {
    printf("{\"c\":1,\"msg\":\"ssid too long\"}\r\n");
    return;
  }
  // password is optional (open networks); type is required per protocol
  // §1.2 but is only stored today -- this firmware does not yet use it to
  // constrain the WiFi auth-mode threshold when connecting.
  const char *password_str = cJSON_IsString(password) ? password->valuestring : "";
  if (strlen(password_str) >= sizeof(s_wifi_password))
  {
    printf("{\"c\":1,\"msg\":\"password too long\"}\r\n");
    return;
  }
  if (!cJSON_IsString(type))
  {
    printf("{\"c\":1,\"msg\":\"type is required\"}\r\n");
    return;
  }
  if (strlen(type->valuestring) >= sizeof(s_wifi_type))
  {
    printf("{\"c\":1,\"msg\":\"type too long\"}\r\n");
    return;
  }

  if (prov_save_wifi(ssid->valuestring, password_str, type->valuestring) != ESP_OK)
  {
    printf("{\"c\":2,\"msg\":\"failed to save wifi config\"}\r\n");
    return;
  }

  strlcpy(s_wifi_ssid, ssid->valuestring, sizeof(s_wifi_ssid));
  strlcpy(s_wifi_password, password_str, sizeof(s_wifi_password));
  strlcpy(s_wifi_type, type->valuestring, sizeof(s_wifi_type));

  ESP_LOGI(TAG, "wifi provisioned: ssid=%s type=%s", s_wifi_ssid, s_wifi_type);
  printf("{\"c\":0,\"msg\":\"wifi saved\"}\r\n");
}

/**
 * @brief  Handles one parsed `{"command":"SetupServer",...}` line (protocol
 *         §1.2): validates `host`/`port`, persists them to NVS, updates the
 *         in-RAM active config so the change also applies to the current
 *         boot, and prints a one-line JSON ack/error back over the console
 *         UART.
 * @param  root Parsed JSON object for the command line (not consumed/freed
 *         here -- the caller owns it).
 */
static void prov_handle_setup_server(const cJSON *root)
{
  const cJSON *host = cJSON_GetObjectItemCaseSensitive(root, "host");
  const cJSON *port = cJSON_GetObjectItemCaseSensitive(root, "port");

  if (!cJSON_IsString(host) || host->valuestring[0] == '\0')
  {
    printf("{\"c\":1,\"msg\":\"host is required\"}\r\n");
    return;
  }
  if (strlen(host->valuestring) >= sizeof(s_http_host))
  {
    printf("{\"c\":1,\"msg\":\"host too long\"}\r\n");
    return;
  }
  if (!cJSON_IsNumber(port) || port->valuedouble < 1 || port->valuedouble > 65535)
  {
    printf("{\"c\":1,\"msg\":\"port must be a number in 1..65535\"}\r\n");
    return;
  }
  uint16_t port_val = (uint16_t)port->valuedouble;

  if (prov_save_server(host->valuestring, port_val) != ESP_OK)
  {
    printf("{\"c\":2,\"msg\":\"failed to save server config\"}\r\n");
    return;
  }

  strlcpy(s_http_host, host->valuestring, sizeof(s_http_host));
  s_http_port = port_val;

  ESP_LOGI(TAG, "server provisioned: host=%s port=%u", s_http_host, s_http_port);
  printf("{\"c\":0,\"msg\":\"server saved\"}\r\n");
}

/**
 * @brief  Handles one parsed `{"command":"SetupDevice","sn":"..."}` line
 *         (protocol §1.2): lets a whole production batch share one
 *         compiled firmware image (one pid, no sn baked in) by setting
 *         each physical unit's own serial number after flashing, over
 *         serial, instead of recompiling per unit. Validates `sn`,
 *         persists it to NVS, updates the in-RAM active value so the
 *         change also applies to the current boot, and prints a one-line
 *         JSON ack/error back over the console UART. There is no
 *         `SetupDevice` support for pid -- unlike sn, every unit built
 *         from the same firmware image is expected to share it.
 * @param  root Parsed JSON object for the command line (not consumed/freed
 *         here -- the caller owns it).
 */
static void prov_handle_setup_device(const cJSON *root)
{
  const cJSON *sn = cJSON_GetObjectItemCaseSensitive(root, "sn");

  if (!cJSON_IsString(sn) || sn->valuestring[0] == '\0')
  {
    printf("{\"c\":1,\"msg\":\"sn is required\"}\r\n");
    return;
  }
  if (strlen(sn->valuestring) >= sizeof(s_sn))
  {
    printf("{\"c\":1,\"msg\":\"sn too long\"}\r\n");
    return;
  }

  if (prov_save_sn(sn->valuestring) != ESP_OK)
  {
    printf("{\"c\":2,\"msg\":\"failed to save sn\"}\r\n");
    return;
  }

  strlcpy(s_sn, sn->valuestring, sizeof(s_sn));

  ESP_LOGI(TAG, "device provisioned: sn=%s", s_sn);
  printf("{\"c\":0,\"msg\":\"sn saved\"}\r\n");
}

/**
 * @brief  Parses one received line as JSON and dispatches it to
 *         prov_handle_setup_wifi/prov_handle_setup_server/
 *         prov_handle_setup_device based on its `command` field, printing
 *         a one-line JSON error back over the console UART for anything
 *         that isn't valid JSON or isn't a recognized command.
 * @param  line NUL-terminated line received from the console UART (no
 *         trailing CR/LF).
 */
static void prov_handle_line(const char *line)
{
  if (line[0] == '\0')
  {
    return;  // blank line (e.g. a lone \r\n) -- nothing to do
  }

  cJSON *root = cJSON_Parse(line);
  if (root == NULL)
  {
    printf("{\"c\":1,\"msg\":\"invalid json\"}\r\n");
    return;
  }

  const cJSON *command = cJSON_GetObjectItemCaseSensitive(root, "command");
  if (!cJSON_IsString(command) || command->valuestring == NULL)
  {
    printf("{\"c\":1,\"msg\":\"missing command\"}\r\n");
  }
  else if (strcmp(command->valuestring, "SetupWifi") == 0)
  {
    prov_handle_setup_wifi(root);
  }
  else if (strcmp(command->valuestring, "SetupServer") == 0)
  {
    prov_handle_setup_server(root);
  }
  else if (strcmp(command->valuestring, "SetupDevice") == 0)
  {
    prov_handle_setup_device(root);
  }
  else
  {
    printf("{\"c\":1,\"msg\":\"unknown command\"}\r\n");
  }

  cJSON_Delete(root);
}

void Provision_Init(void)
{
  // Start from the idf.py menuconfig (Kconfig) compile-time defaults...
  strlcpy(s_wifi_ssid, USR_SSID, sizeof(s_wifi_ssid));
  strlcpy(s_wifi_password, USR_PASSWORD, sizeof(s_wifi_password));
  s_wifi_type[0] = '\0';
  strlcpy(s_http_host, USR_HTTP_HOST, sizeof(s_http_host));
  s_http_port = USR_HTTP_PORT;
  strlcpy(s_sn, USR_SN, sizeof(s_sn));

  // ...then override whichever fields have a previously-provisioned value
  // in NVS. Read into a scratch buffer first and only copy over on success,
  // so a partially-provisioned device (e.g. WiFi set, server never touched)
  // keeps the Kconfig default for the untouched field instead of an
  // undefined/garbage value.
  nvs_handle_t h;
  if (nvs_open(PROV_NVS_NAMESPACE, NVS_READONLY, &h) == ESP_OK)
  {
    char tmp[PROV_HOST_MAX_LEN];  // largest of the string fields
    size_t len;
    uint16_t port_val;

    len = sizeof(s_wifi_ssid);
    if (nvs_get_str(h, PROV_KEY_WIFI_SSID, tmp, &len) == ESP_OK)
    {
      strlcpy(s_wifi_ssid, tmp, sizeof(s_wifi_ssid));
    }

    len = sizeof(s_wifi_password);
    if (nvs_get_str(h, PROV_KEY_WIFI_PASS, tmp, &len) == ESP_OK)
    {
      strlcpy(s_wifi_password, tmp, sizeof(s_wifi_password));
    }

    len = sizeof(s_wifi_type);
    if (nvs_get_str(h, PROV_KEY_WIFI_TYPE, tmp, &len) == ESP_OK)
    {
      strlcpy(s_wifi_type, tmp, sizeof(s_wifi_type));
    }

    len = sizeof(s_http_host);
    if (nvs_get_str(h, PROV_KEY_HTTP_HOST, tmp, &len) == ESP_OK)
    {
      strlcpy(s_http_host, tmp, sizeof(s_http_host));
    }

    if (nvs_get_u16(h, PROV_KEY_HTTP_PORT, &port_val) == ESP_OK)
    {
      s_http_port = port_val;
    }

    len = sizeof(s_sn);
    if (nvs_get_str(h, PROV_KEY_SN, tmp, &len) == ESP_OK)
    {
      strlcpy(s_sn, tmp, sizeof(s_sn));
    }

    nvs_close(h);
  }

  ESP_LOGI(TAG, "active config: sn=%s ssid=%s host=%s port=%u", s_sn, s_wifi_ssid, s_http_host, s_http_port);
}

void Provision_RunWindow(uint32_t idle_timeout_ms, uint32_t max_total_ms)
{
  prov_uart_init();

  char line[PROV_LINE_MAX_LEN];
  size_t line_len = 0;
  TickType_t start = xTaskGetTickCount();
  TickType_t last_activity = start;
  const TickType_t idle_ticks = pdMS_TO_TICKS(idle_timeout_ms);
  const TickType_t max_ticks = pdMS_TO_TICKS(max_total_ms);

  printf("{\"c\":0,\"msg\":\"provisioning window open, send SetupWifi/SetupServer/SetupDevice JSON now\"}\r\n");

  for (;;)
  {
    uint8_t byte;
    int n = uart_read_bytes(PROV_UART_NUM, &byte, 1, pdMS_TO_TICKS(200));
    TickType_t now = xTaskGetTickCount();

    if (n > 0)
    {
      last_activity = now;
      if (byte == '\n' || byte == '\r')
      {
        if (line_len > 0)
        {
          line[line_len] = '\0';
          prov_handle_line(line);
          line_len = 0;
        }
      }
      else if (line_len < sizeof(line) - 1)
      {
        line[line_len++] = (char)byte;
      }
      else
      {
        // Line too long for our buffer -- drop it rather than overflow, and
        // resync on the next newline.
        ESP_LOGW(TAG, "provisioning line too long, discarding");
        line_len = 0;
      }
    }

    if ((now - last_activity) >= idle_ticks)
    {
      break;  // quiet for long enough -- nobody's provisioning, move on
    }
    if ((now - start) >= max_ticks)
    {
      ESP_LOGW(TAG, "provisioning window hit its hard time cap, closing");
      break;
    }
  }

  printf("{\"c\":0,\"msg\":\"provisioning window closed\"}\r\n");
}

const char *Provision_GetWifiSsid(void)
{
  return s_wifi_ssid;
}

const char *Provision_GetWifiPassword(void)
{
  return s_wifi_password;
}

const char *Provision_GetHttpHost(void)
{
  return s_http_host;
}

uint16_t Provision_GetHttpPort(void)
{
  return s_http_port;
}

const char *Provision_GetSN(void)
{
  return s_sn;
}

/*******************************************************************************
                                      END
*******************************************************************************/
