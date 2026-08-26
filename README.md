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

## License

This project is open-sourced under the [MIT License](LICENSE).
