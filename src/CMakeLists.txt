# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "."
    REQUIRES
        cjson
        esp_driver_gpio
        esp_event
        esp_netif
        esp_system
        esp_timer
        esp_wifi
        freertos
        log
        mbedtls
        mqtt
        nvs_flash
)
