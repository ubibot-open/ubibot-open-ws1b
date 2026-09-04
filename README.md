Build environment: Windows, VS Code, ESP-IDF v6.0.2

## Configuration

Device identity, WiFi, and server parameters are configured via `menuconfig` instead of being hardcoded in the source:

```bash
idf.py menuconfig
```

Go to the `UbiBot WS1B Configuration` menu and adjust as needed:

- **WiFi Configuration**: WiFi SSID / password / country code
- **Server Configuration**: server host / port
- **Device Identity**: device PID / serial number (SN — each physical device should use its own unique value)

The resulting configuration is saved to the local `sdkconfig` file (excluded via `.gitignore`, so it is never committed) and is generated automatically on first build or after running `idf.py menuconfig`. Default values are defined in [main/Kconfig.projbuild](main/Kconfig.projbuild).

## Serial provisioning (WiFi / server / serial number, no reflash needed)

The menuconfig values above are only the **factory default** baked in at build time. WiFi
credentials, the server address, and the device's own serial number can also be set at runtime
over the same USB/UART console used for logs and `idf.py monitor` (115200 baud) — see
[main/provisioning.c](main/provisioning.c), implementing protocol §1.2 of the
[Hardware Communication Protocol](https://github.com/ubibot-open/ubibot-open-doc/blob/main/protocol/hardware-communication-protocol.md#12-serial-provisioning).
A provisioned value is persisted in NVS and takes priority over the menuconfig default on every
subsequent boot, until re-provisioned. In particular, `SetupDevice` (below) is what lets one
compiled firmware image (one pid, no sn baked in) be flashed onto a whole production batch, with
each unit's own sn set afterward over serial instead of recompiling per unit.

**Window**: right after a power-on (not on the periodic timer wake-ups used for routine
reporting), the device listens on the console UART for up to 60s, closing early after 5s of
silence, before continuing on to connect WiFi. Send commands during this window with
`idf.py monitor`, [ubibot-serial-sync](https://github.com/ubibot-open/ubibot-serial-sync), or any
serial terminal — one JSON object per line:

```jsonc
// Set WiFi
{"command":"SetupWifi","ssid":"MyHomeWiFi","password":"12345678","type":"WPA2"}
// Set the data server
{"command":"SetupServer","host":"192.168.2.71","port":8080}
// Set this unit's own serial number
{"command":"SetupDevice","sn":"RV41554WS1B"}
```

Each command gets a one-line JSON ack back, e.g. `{"c":0,"msg":"wifi saved"}` on success or
`{"c":1,"msg":"<reason>"}` on a validation error (`c":2` means the NVS write itself failed).
`ssid`/`host`/`port`/`sn` are required; `password` may be omitted/empty for an open network;
`type` is required by the protocol but is only stored today, not yet used to constrain the WiFi
auth-mode threshold when connecting. There's no `SetupDevice` support for `pid` — unlike `sn`,
every unit built from the same firmware image is expected to share it.

Bluetooth provisioning (protocol §1.1) is **not supported** by this firmware — serial
provisioning is the only way to change WiFi/server settings without a full rebuild+reflash.

## Server-issued commands (reboot / report interval)

A data-report response may carry an optional `cmd` object — an admin-queued command, delivered
piggybacked on this device's own next report (protocol
[§9](https://github.com/ubibot-open/ubibot-open-doc/blob/main/protocol/hardware-communication-protocol.md#9-command-delivery-admin-triggered-optional)).
This firmware implements it in [main/command.c](main/command.c), handled from the same response
parser that reads `c`/`t` ([app_driver/net/http_client.c](app_driver/net/http_client.c)):

- `{"action":"reboot"}` — restarts immediately (`esp_restart()`).
- `{"action":"set_interval","seconds":600}` — persists the new report interval to NVS; applies
  starting with the *next* sleep cycle (the one already under way keeps whatever interval was
  active when it started). Read back via `Command_GetReportIntervalSeconds()`, used in place of
  the `DEFAULT_FN` constant when the device goes to sleep.

There's no ack sent back to the server for either action — delivery is fire-and-forget, same as
serial provisioning's philosophy above. Queue a command from a device's detail page in the admin
console (see the [deployment guide](https://github.com/ubibot-open/ubibot-open-doc/blob/main/guides/deployment-flashing-guide.md#25-sending-a-command-to-a-device-optional)).

## Contributing

See the [org-wide CONTRIBUTING.md](https://github.com/ubibot-open/.github/blob/main/CONTRIBUTING.md).

## License

This project is open-sourced under the [MIT License](LICENSE).
