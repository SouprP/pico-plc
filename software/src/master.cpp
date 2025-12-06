#include <cstdio>
#include "pico/stdlib.h"
#include "pico-modbus/md_master.h"
#include "pico-modbus/common/md_common.h"

#define MASTER_UART uart0
#define MASTER_BAUDRATE 19200
#define SLAVE_ADDRESS 2

int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\n=== Modbus Master - Diagnostic Registers Test ===\n");
    printf("UART: %s, Baudrate: %d\n", MASTER_UART == uart0 ? "UART0" : "UART1", MASTER_BAUDRATE);
    printf("Pins: TX=GP16, RX=GP17\n\n");
    
    gpio_set_function(16, GPIO_FUNC_UART);
    gpio_set_function(17, GPIO_FUNC_UART);
    
    ModbusMaster master(MASTER_UART, MASTER_BAUDRATE);
    
    printf("Reading slave diagnostic registers every 5 seconds...\n\n");
    
    while (true) {
        printf("--- Slave Diagnostics ---\n");
        
        // Read Bus Message Count (0x000B)
        master.send_diagnostic_request(SLAVE_ADDRESS, 0x000B, 0x0000,
            [](const modbus_frame_t& response) {
                if (response.function_code == 0x08 && response.data_length >= 4) {
                    uint16_t count = (response.data[2] << 8) | response.data[3];
                    printf("  Slave Bus Message Count (sent): %u\n", count);
                } else if (response.function_code & 0x80) {
                    printf("  ERROR: Exception 0x%02X\n", response.data[0]);
                }
            }, 1000);
        
        for (int i = 0; i < 200; i++) {
            master.process_tx_queue();
            sleep_ms(1);
        }
        sleep_ms(500);
        
        // Read Slave Message Count (0x000E)
        master.send_diagnostic_request(SLAVE_ADDRESS, 0x000E, 0x0000,
            [](const modbus_frame_t& response) {
                if (response.function_code == 0x08 && response.data_length >= 4) {
                    uint16_t count = (response.data[2] << 8) | response.data[3];
                    printf("  Slave Message Count (received): %u\n", count);
                } else if (response.function_code & 0x80) {
                    printf("  ERROR: Exception 0x%02X\n", response.data[0]);
                }
            }, 1000);
        
        for (int i = 0; i < 200; i++) {
            master.process_tx_queue();
            sleep_ms(1);
        }
        sleep_ms(500);
        
        // Read Bus Communication Error Count (0x000C)
        master.send_diagnostic_request(SLAVE_ADDRESS, 0x000C, 0x0000,
            [](const modbus_frame_t& response) {
                if (response.function_code == 0x08 && response.data_length >= 4) {
                    uint16_t count = (response.data[2] << 8) | response.data[3];
                    printf("  Slave Communication Errors (CRC): %u\n", count);
                } else if (response.function_code & 0x80) {
                    printf("  ERROR: Exception 0x%02X\n", response.data[0]);
                }
            }, 1000);
        
        for (int i = 0; i < 200; i++) {
            master.process_tx_queue();
            sleep_ms(1);
        }
        sleep_ms(500);
        
        // Read Slave Exception Error Count (0x000D)
        master.send_diagnostic_request(SLAVE_ADDRESS, 0x000D, 0x0000,
            [](const modbus_frame_t& response) {
                if (response.function_code == 0x08 && response.data_length >= 4) {
                    uint16_t count = (response.data[2] << 8) | response.data[3];
                    printf("  Slave Exception Errors: %u\n", count);
                } else if (response.function_code & 0x80) {
                    printf("  ERROR: Exception 0x%02X\n", response.data[0]);
                }
            }, 1000);
        
        for (int i = 0; i < 200; i++) {
            master.process_tx_queue();
            sleep_ms(1);
        }
        
        printf("\n--- Master Local Diagnostics ---\n");
        printf("  Master Bus Messages (sent): %u\n", master.get_bus_message_count());
        printf("  Master Timeouts: %u\n", master.get_slave_no_response_count());
        printf("  Master Communication Errors (CRC): %u\n\n", master.get_bus_communication_error_count());
        
        sleep_ms(5000);
    }
    
    return 0;
}