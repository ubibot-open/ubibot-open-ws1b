set(requires esp-tls esp_event esp_netif esp_http_client driver json esp_adc esp_wifi esp_timer main)

idf_component_register(SRCS "crc_check.c" "wifi_connect.c" "http_client.c"
                    "pinmux.c" "power_adc.c" "iic.c" "at24c32.c" "PCF8563.c" "user_spi.c" "w25q128.c"
                    "ub_dt_p1.c" "sht30dis.c" "ltr308.c" "stk8323.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES ${requires}
                    )



                    