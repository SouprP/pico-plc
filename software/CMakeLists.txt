cmake_minimum_required(VERSION 3.31.6)

# Pico-SDK setup
set(PICO_PLATFORM rp2350)
set(PICO_BOARD pico2_w)
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

# Project setup
project(pico_plc)
project(master_plc)
project(slave_plc)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)


pico_sdk_init()

# Executables for all projects
add_executable(pico_plc src/main.cpp src/defines.h)
add_executable(master_plc src/master.cpp src/defines.h)
add_executable(slave_plc src/slave.cpp src/defines.h)

# Add all custom libraries
add_subdirectory(lib)

set(LIBS
        pico_stdlib
        pico_rand
        pico_multicore
        hardware_i2c
        pico_cyw43_arch_none
        hardware_pwm

        pico_utils
        pico_modbus
)

target_link_libraries(pico_plc ${LIBS})
target_link_libraries(master_plc ${LIBS})
target_link_libraries(slave_plc ${LIBS})

# USB / Serial port connection (COM ports)
pico_enable_stdio_usb(pico_plc 1)
pico_enable_stdio_uart(pico_plc 0)
pico_enable_stdio_usb(master_plc 1)
pico_enable_stdio_uart(master_plc 0)
pico_enable_stdio_usb(slave_plc 1)
pico_enable_stdio_uart(slave_plc 0)

# Create map/bin/hex file etc.
pico_add_extra_outputs(master_plc)
pico_add_extra_outputs(slave_plc)
pico_add_extra_outputs(pico_plc)

# pico-tool auto-flash command
set(DEBUG_BUILD 1)
set(ALL_BUILD 0)
set(SLAVE_BUILD 1)
set(MASTER_BUILD 1)

set(SLAVE_ID FC8CC809375EF532)
set(MASTER_ID 4A8F1AAABAD7DE92)

# General .uf2 load
#if (NOT DEBUG_BUILD)
#    add_custom_command(TARGET pico_plc POST_BUILD
#    #        COMMAND picotool info || true
#            COMMAND picotool reboot -f -u || true
#            COMMAND ${CMAKE_COMMAND} -E sleep 1
#            COMMAND picotool load pico_plc.uf2 ||  true
#            COMMAND ${CMAKE_COMMAND} -E sleep 1
#            COMMAND picotool reboot || true
#    )
#endif ()

# All information on how these commands work can be found here:
# https://github.com/raspberrypi/picotool

# Master .uf2 build load
if (NOT DEBUG_BUILD AND (${MASTER_BUILD} OR ${ALL_BUILD}))
    add_custom_command(TARGET master_plc POST_BUILD
            COMMAND picotool load -f --ser ${MASTER_ID} -x master_plc.uf2

    )
endif ()

# If there is no delay, the program flashing crashes
execute_process(COMMAND ${CMAKE_COMMAND} -E sleep 3.0)

# Slave .uf2 build load
if (NOT DEBUG_BUILD AND (${SLAVE_BUILD} OR ${ALL_BUILD}))
    add_custom_command(TARGET slave_plc POST_BUILD
            COMMAND picotool load -f --ser ${SLAVE_ID} -x slave_plc.uf2
    )
endif ()

# Test .uf2 file
if (DEBUG_BUILD AND (${SLAVE_BUILD} OR ${ALL_BUILD}))
    add_custom_command(TARGET slave_plc POST_BUILD
            COMMAND picotool load -f --ser ${SLAVE_ID} -x pico_plc.uf2
    )
endif ()