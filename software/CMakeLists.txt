cmake_minimum_required(VERSION 3.31.6)

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(pico_plc)
set(CMAKE_CXX_STANDARD 11)

pico_sdk_init()

add_executable(pico_plc main.cpp)

target_link_libraries(pico_plc
        pico_stdlib
        pico_multicore
        hardware_i2c
        pico_rand
)




pico_enable_stdio_usb(pico_plc 1)
pico_enable_stdio_uart(pico_plc 0)

# Create map/bin/hex file etc.
pico_add_extra_outputs(pico_plc)