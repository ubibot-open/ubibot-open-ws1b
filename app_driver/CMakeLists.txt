set(requires esp-tls esp_event esp_netif esp_http_client driver esp_driver_gpio esp_driver_i2c esp_driver_ledc esp_driver_spi esp_adc esp_wifi esp_timer main)

idf_component_register(SRCS "utils/crc_check.c" "net/wifi_connect.c" "net/http_client.c"
                    "power/pinmux.c" "power/power_adc.c" "bus/iic.c" "bus/user_spi.c"
                    "storage/at24c32.c" "storage/w25q128.c" "rtc/PCF8563.c"
                    "sensors/ub_dt_p1.c" "sensors/sht30dis.c" "sensors/ltr308.c" "sensors/stk8323.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES ${requires}
                    )



                    