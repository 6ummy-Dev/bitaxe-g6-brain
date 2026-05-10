idf_component_register(
    SRCS "g6_brain.c" "g6_safety.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_i2c nvs_flash esp_timer
)