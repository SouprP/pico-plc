#include <cstdio>
#include "pico/stdlib.h"
#include "pico-modbus/md_slave.h"

#define SLAVE_UART uart0
#define SLAVE_BAUDRATE 19200
#define MY_SLAVE_ADDRESS 2

int main() {
    stdio_init_all();
    sleep_ms(3000); // Wait for USB serial
    
    printf("\n=== Modbus Slave Test - Auto GPIO Mapping ===\n");
    printf("Slave Address: %d\n", MY_SLAVE_ADDRESS);
    printf("UART: %s, Baudrate: %d\n", SLAVE_UART == uart0 ? "UART0" : "UART1", SLAVE_BAUDRATE);
    printf("Pins: TX=GP16, RX=GP17\n\n");
    
    // Configure UART pins
    gpio_set_function(16, GPIO_FUNC_UART); // TX
    gpio_set_function(17, GPIO_FUNC_UART); // RX
    
    // Initialize Modbus Slave
    ModbusSlave slave(MY_SLAVE_ADDRESS, SLAVE_UART, SLAVE_BAUDRATE);
    
    // Enable coil 28 - it will automatically control GPIO28!
    // The coil number maps directly to the GPIO number
    std::map<uint16_t, bool> initial_coils = {
        {0, false},
        {1, false},
        {28, false}  // Coil 28 automatically controls GPIO28
    };
    slave.enable_coils(initial_coils, true);  // true = auto GPIO sync
    
    printf("Slave initialized and listening...\n");
    printf("Configuration:\n");
    printf("  - Coil 28: Auto-mapped to GPIO28 (relay)\n\n");

    
    while (true) {
        // Process any queued responses
        // GPIO is automatically synced when coils are written!
        slave.process_tx_queue();
        sleep_ms(1);
    }
    
    return 0;
}