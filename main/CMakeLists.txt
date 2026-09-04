# Embed the server root certificate into the final binary
#
# (If this was a component, we would set COMPONENT_EMBED_TXTFILES here.)
set(requires esp-tls nvs_flash esp_event esp_netif esp_http_client driver esp_driver_uart esp_adc bt esp_wifi app_update app_driver)

idf_component_register(SRCS "main.c" "osi.c" "indicators.c" "power_mgmt.c" "json_payload.c" "provisioning.c" "command.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES ${requires}
                    )



                    