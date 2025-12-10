if(NOT TARGET pico_utils)
    set(SRC_FILES
            common_utils.cpp
            custom_dac.cpp
            

    )

    set(INC_FILES
            common_utils.h
            custom_dac.h
    )

    add_library(pico_utils ${SRC_FILES} ${INC_FILES})

    target_link_libraries(pico_utils PUBLIC
            pico_stdlib
            pico_rand
            pico_multicore
            hardware_i2c
            pico_cyw43_arch_none

            hardware_pwm
            hardware_adc
            hardware_gpio
    )

    target_include_directories(pico_utils PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/..
    )
endif()