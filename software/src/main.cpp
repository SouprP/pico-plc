#include <cstdio>
#include <pico/stdio.h>
#include "pico/stdlib.h"

#include "pico/rand.h"
#include "pico/unique_id.h"

#include "pico-modbus/master/master.h"
#include "pico-modbus/common/md_base.h"

#include "pico/cyw43_arch.h"
#include "boards/pico2_w.h"

// #include "hardware/i2c.h"

void setup() {
    stdio_init_all();

    // gpio_init(PICO_DEFAULT_LED_PIN);
}

void loop() {

}

int main() {
    stdio_init_all();
    // setup();
    ModbusBase modbus;
    modbus.on_message([](const uint8_t id, const modbus_frame_t frame) {
        printf("id: %u\n", id);
    });
    if (cyw43_arch_init()) {
        return -1;
    }

    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);

    // get_rand_32() ;

    while (true) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(2500);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(2500);
        modbus.read(nullptr, 1);
        // printf("LED\n");
    }

    return 0;
}