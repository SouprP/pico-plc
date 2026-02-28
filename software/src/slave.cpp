#include <cstdio>
#include "pico/stdlib.h"

#include "defines.h"
#include "pico-modbus/md_slave.h"
#include "pico-modbus/common/md_common.h"

int main() {
    stdio_init_all();
    sleep_ms(2000); // Wait for USB serial

    gpio_set_function(16, GPIO_FUNC_UART); // TX
    gpio_set_function(17, GPIO_FUNC_UART); // RX
    
    ModbusSlave slave(SLAVE_ADDRESS, MD_UART, MD_BAUDRATE, RS485_DE_PIN, RS485_RE_PIN, parity);

    slave.on_message([&slave](const modbus_frame_t& frame) {
        uint16_t reg0 = 0;
        uint16_t reg1 = 0;
        bool coil = false;

        slave.get_holding_register(0, reg0);
        slave.get_holding_register(1, reg1);
        slave.get_coil(28, coil);

        printf("Reg0: %d, reg1: %d, coil status: %s \n\n", reg0, reg1, coil ? "ON" : "OFF");
    });

    // Enable holding registers (readable/writable)
    slave.enable_holding_registers(2);

    slave.set_holding_register(0, 0);   // random value
    slave.set_holding_register(1, 0);   // written by master

    // Enable coils (writable by master)
    std::map<uint16_t, bool> initial_coils = {
        {28, false},  // Coil 28
    };
    slave.enable_coils(initial_coils, true);

    uint32_t loop_count = 0;
    uint16_t counter = 0;
    
    while (true) {
        slave.process_tx_queue();

        if (++loop_count >= 1000) {
            loop_count = 0;
            counter++;
            slave.set_holding_register(0, counter);
        }
        
        sleep_ms(1);
    }

}