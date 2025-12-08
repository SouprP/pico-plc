#include <cstdio>
#include <cstdlib>
#include "pico/stdlib.h"

// Debug messages are enabled via global flag
// #define MODBUS_DEBUG // No longer used

#include "defines.h"
#include "pico-modbus/md_master.h"
#include "pico-modbus/common/md_common.h"

// #define MASTER_UART uart0
// #define MASTER_BAUDRATE MD_BAUDRATE

int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    // Enable Modbus debug output
    // modbus_debug_enabled = true;
    
    printf("\n=== Modbus Master - Multicore Slave Example ===\n");
    printf("UART: %s, Baudrate: %d\n", MD_UART == uart0 ? "UART0" : "UART1", MD_BAUDRATE);
    printf("Pins: TX=GP16, RX=GP17\n");
    printf("RS485: DE=GP%d, RE=GP%d (SN65HVD75DGKR)\n\n", RS485_DE_PIN, RS485_RE_PIN);
    
    gpio_set_function(16, GPIO_FUNC_UART);
    gpio_set_function(17, GPIO_FUNC_UART);
    
    ModbusMaster master(MD_UART, MD_BAUDRATE, RS485_DE_PIN, RS485_RE_PIN);
    
    printf("Sending one request every 3 seconds...\n\n");
    
    uint8_t request_type = 0;
    static bool coil_state = false;
    
    while (true) {
        // Cycle through different request types
        switch (request_type) {
            case 0:
                printf("--- Reading Register 0 ---\n");
                master.send_read_holding_registers_request(SLAVE_ADDRESS, 0, 1,
                    [](const modbus_frame_t& response) {
                        if (response.function_code == 0x03 && response.data_length >= 3) {
                            uint16_t val = (response.data[1] << 8) | response.data[2];
                            printf("  Register 0: %u\n", val);
                        }
                    }, 5000);
                break;
                
            case 1: {
                uint16_t rand_val = rand() % 65536;
                printf("--- Writing Random Value to Register 1 (%u) ---\n", rand_val);
                master.send_write_single_register_request(SLAVE_ADDRESS, 1, rand_val,
                    [rand_val](const modbus_frame_t& response) {
                        if (response.function_code == 0x06) {
                            printf("  Register 1 written: %u\n", rand_val);
                        }
                    }, 5000);
                break;
            }
                
            case 2:
                coil_state = !coil_state;
                printf("--- Setting Coil 0 to %s ---\n", coil_state ? "ON" : "OFF");
                master.send_write_single_coil_request(SLAVE_ADDRESS, 28, coil_state,
                    [](const modbus_frame_t& response) {
                        if (response.function_code == 0x05) {
                            printf("  Coil 0 set successfully\n");
                        }
                    }, 5000);
                break;
        }
        
        // Wait for request to complete
        while (master.is_request_pending()) {
            master.process_tx_queue();
            sleep_ms(1); // 1ms polling interval
        }
        
        printf("  Master Stats - Messages: %u, Timeouts: %u, CRC Errors: %u\n\n",
               master.get_bus_message_count(),
               master.get_slave_no_response_count(),
               master.get_bus_communication_error_count());
        
        // Move to next request type
        request_type = (request_type + 1) % 3;
        
        // Keep processing during delay to handle any late responses
        for (int i = 0; i < 3000; i++) {
            master.process_tx_queue();
            sleep_ms(1);
        }
    }
    
    return 0;
}