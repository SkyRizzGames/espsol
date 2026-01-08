idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS ""
    REQUIRES espsol
    PRIV_REQUIRES esp_wifi nvs_flash
)
