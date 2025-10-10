cmake_minimum_required(VERSION 3.31.6)

# Pico-SDK setup
set(PICO_PLATFORM rp2350)
set(PICO_BOARD pico2)
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

# Project setup
project(pico_plc)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)


pico_sdk_init()

add_executable(pico_plc
        src/main.cpp
)

add_subdirectory(lib)

target_link_libraries(pico_plc
        pico_stdlib
        pico_multicore
        hardware_i2c

        pico_modbus
)

pico_enable_stdio_usb(pico_plc 1)
pico_enable_stdio_uart(pico_plc 0)

# Create map/bin/hex file etc.
pico_add_extra_outputs(pico_plc)

# pico-tool auto-flash command
add_custom_command(TARGET pico_plc POST_BUILD
        COMMAND picotool load ${CMAKE_PROJECT_NAME}.uf2 || true
)