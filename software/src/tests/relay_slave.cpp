#include <cstdio>
#include "pico/stdlib.h"
#include "pico-modbus/md_slave.h"

#define SLAVE_UART uart0
#define SLAVE_BAUDRATE 19200
#define MY_SLAVE_ADDRESS 2

int main() {
    stdio_init_all();
    sleep_ms(3000);

    gpio_set_function(16, GPIO_FUNC_UART); // TX
    gpio_set_function(17, GPIO_FUNC_UART); // RX
    
    // Initialize Modbus Slave
    ModbusSlave slave(MY_SLAVE_ADDRESS, SLAVE_UART, SLAVE_BAUDRATE);

    std::map<uint16_t, bool> initial_coils = {
        {0, false},
        {1, false},
        {28, false}
    };
    slave.enable_coils(initial_coils, true);

    while (true) {
        // Process any queued responses
        // GPIO is automatically synced when coils are written!
        slave.process_tx_queue();
        sleep_ms(1);
    }
    
    return 0;
}