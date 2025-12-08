#include <cstdio>
#include "pico/stdlib.h"

// Debug messages are enabled via global flag

#include "defines.h"
#include "pico-modbus/md_slave.h"
#include "pico-modbus/common/md_common.h"

// #define SLAVE_UART uart0
// #define SLAVE_BAUDRATE MD_BAUDRATE
// #define MY_SLAVE_ADDRESS 2

int main() {
    stdio_init_all();
    sleep_ms(2000); // Wait for USB serial
    
    // Enable Modbus debug output
    // modbus_debug_enabled = true;
    
    static uint32_t boot_count = 0;
    boot_count++;
    
    printf("\n=== Modbus Slave - Simple Example ===\n");
    printf("***** BOOT #%lu *****\n", boot_count);
    printf("Slave Address: %d\n", SLAVE_ADDRESS);
    printf("UART: %s, Baudrate: %d\n", MD_UART == uart0 ? "UART0" : "UART1", MD_BAUDRATE);
    printf("Pins: TX=GP16, RX=GP17\n");
    printf("RS485: DE=GP%d, RE=GP%d (SN65HVD75DGKR)\n\n", RS485_DE_PIN, RS485_RE_PIN);
    
    gpio_set_function(16, GPIO_FUNC_UART); // TX
    gpio_set_function(17, GPIO_FUNC_UART); // RX
    
    ModbusSlave slave(SLAVE_ADDRESS, MD_UART, MD_BAUDRATE, RS485_DE_PIN, RS485_RE_PIN);

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
    
    // Initialize test registers with fixed values
    slave.set_holding_register(0, 0);   // random value
    slave.set_holding_register(1, 0);      // written by master
    
    // Enable coils (writable by master)
    std::map<uint16_t, bool> initial_coils = {
        {28, false},  // Coil 28
    };
    slave.enable_coils(initial_coils, true);
    
    printf("===== SLAVE READY AND LISTENING =====\n\n");
    
    // Simple counter for periodic register updates
    uint32_t loop_count = 0;
    uint16_t counter = 0;
    
    while (true) {
        // Process Modbus communication - this must be fast!
        slave.process_tx_queue();
        
        // Every ~1 second (1000 loops), increment test values
        if (++loop_count >= 1000) {
            loop_count = 0;
            counter++;
            slave.set_holding_register(0, counter);
            
            // Increment sensor value
            // uint16_t sensor = 0;
            // if (slave.get_holding_register(10, sensor)) {
            //     slave.set_holding_register(10, sensor + 1);
            // }
        }
        
        sleep_ms(1);
    }
    
    return 0;
}