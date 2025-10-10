#include <cstdio>
#include <pico/stdio.h>
#include "pico/stdlib.h"


#include "pico-modbus/master/master.h"
#include "pico-modbus/common/md_base.h"

void setup() {
    stdio_init_all();
}

void loop() {

}

int main() {
    setup();
    ModbusBase modbus;
    modbus.on_message([](const uint8_t id, const modbus_frame_t frame) {
        frame.
    });
    printf("Hello World!\n");

    return 0;
}