#include <pico/unique_id.h>
#include <stdio.h>
#include <string.h>

#ifdef PICO_CYW43_SUPPORTED
#include "pico/cyw43_arch.h"
#endif

const char* get_unique_id() {
    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);
    char* unique_id[64];

    for (int i = 0; i < 8; i++){
        memcpy(unique_id[i*8], &id.id[i], 8);
    }

    return *unique_id;
}

void led_gpio_init() {
#ifdef PICO_CYW43_SUPPORTED
    if (cyw43_arch_init()) {
        return;
    }
#else

#endif
}

void led_gpio_set(bool state) {
#ifdef PICO_CYW43_SUPPORTED
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state);
#else

#endif
}