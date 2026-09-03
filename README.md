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

## Serial provisioning (WiFi / server, no reflash needed)

The menuconfig values above are only the **factory default** baked in at build time. WiFi
credentials and the server address can also be set at runtime over the same USB/UART console
used for logs and `idf.py monitor` (115200 baud) — see [main/provisioning.c](main/provisioning.c),
implementing protocol §1.2 of
[UbiBot开放平台硬件通信协议](../ubibot-open-server/docs/UbiBot开放平台硬件通信协议.md). A
provisioned value is persisted in NVS and takes priority over the menuconfig default on every
subsequent boot, until re-provisioned.

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
```

Each command gets a one-line JSON ack back, e.g. `{"c":0,"msg":"wifi saved"}` on success or
`{"c":1,"msg":"<reason>"}` on a validation error (`c":2` means the NVS write itself failed).
`ssid`/`host`/`port` are required; `password` may be omitted/empty for an open network; `type` is
required by the protocol but is only stored today, not yet used to constrain the WiFi auth-mode
threshold when connecting.

Bluetooth provisioning (protocol §1.1) is **not supported** by this firmware — serial
provisioning is the only way to change WiFi/server settings without a full rebuild+reflash.

## License

This project is open-sourced under the [MIT License](LICENSE).
