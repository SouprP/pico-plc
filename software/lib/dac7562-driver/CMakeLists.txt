if(NOT TARGET dac7562-driver)
    set(SRC_FILES
            dac7562.cpp


    )

    set(INC_FILES
            dac7562.h
    )

    add_library(pico_utils ${SRC_FILES} ${INC_FILES})

    target_link_libraries(pico_utils PUBLIC
            pico_stdlib
            pico_rand
            hardware_spi
            pico_cyw43_arch_none

    )

    target_include_directories(pico_utils PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/..
    )
endif()