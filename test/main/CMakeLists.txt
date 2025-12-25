idf_component_register(
    SRCS "main.c" "integration_test.c"
    INCLUDE_DIRS ""
    REQUIRES espsol nvs_flash esp_wifi esp_netif esp_event
)
