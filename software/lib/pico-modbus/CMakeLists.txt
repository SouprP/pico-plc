if(NOT TARGET pico_modbus)
#    set(PICO_PLATFORM rp2350)
#    set(PICO_BOARD pico2_w)
#    include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)
#    message($ENV{PICO_SDK_PATH})
    set(SRC_FILES
            md_master.cpp
            md_slave.cpp
            common/md_common.cpp
            common/md_base.cpp
            common/md_stream.cpp

    )

    set(INC_FILES
            md_master.h
            md_slave.h
            common/md_common.h
            common/md_base.h
            common/md_stream.h
    )

    add_library(pico_modbus ${SRC_FILES} ${INC_FILES})
    
    # Enable debug messages if MODBUS_DEBUG is defined in parent CMakeLists.txt
    # Comment out for production to improve performance
    # target_compile_definitions(pico_modbus PUBLIC MODBUS_DEBUG)

    target_link_libraries(pico_modbus PUBLIC
            pico_stdlib
            pico_multicore
            hardware_uart
            hardware_timer
            hardware_irq

            pico_utils
    )

    target_include_directories(pico_modbus PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/..
    )
endif()