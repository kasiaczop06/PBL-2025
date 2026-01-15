idf_component_register(SRCS "clock_idf.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver nvs_flash esp_wifi esp_netif lwip lcd_i2c hx711)
