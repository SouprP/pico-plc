#include <cstdio>
#include "pico/stdlib.h"
#include "pico-modbus/md_master.h"
#include "pico-modbus/common/md_common.h"

#define MASTER_UART uart0
#define MASTER_BAUDRATE 19200
#define SLAVE_ADDRESS 2
#define RELAY_COIL 28  // Coil 28 maps to GPIO28 on slave

int main() {
    stdio_init_all();
    sleep_ms(2000); // Wait for USB serial
    
    printf("\n=== Modbus Master Test - Relay Control ===\n");
    printf("UART: %s, Baudrate: %d\n", MASTER_UART == uart0 ? "UART0" : "UART1", MASTER_BAUDRATE);
    printf("Pins: TX=GP16, RX=GP17\n");
    printf("Controlling coil %d (GPIO28 on slave)\n\n", RELAY_COIL);
    
    // Configure UART pins
    gpio_set_function(16, GPIO_FUNC_UART); // TX
    gpio_set_function(17, GPIO_FUNC_UART); // RX
    
    // Initialize Modbus Master
    ModbusMaster master(MASTER_UART, MASTER_BAUDRATE);

    printf("Master initialized. Toggling relay every 5 seconds...\n\n");
    
    bool relay_state = false;
    uint32_t cycle_count = 0;
    
    while (true) {
        cycle_count++;
        
        // Toggle relay every cycle (5 seconds)
        relay_state = !relay_state;
        
        printf("Cycle %lu: Setting coils %s\n", cycle_count, relay_state ? "ON" : "OFF");
        
        // Step 1: Write Multiple Coils (0x0F) to control coils 0 and 1
        // Coil values are packed in bytes: bit 0 = coil 0, bit 1 = coil 1
        uint8_t coil_bytes[1];
        if (relay_state) {
            coil_bytes[0] = 0x03;  // Binary: 00000011 (both coils ON)
        } else {
            coil_bytes[0] = 0x00;  // Binary: 00000000 (both coils OFF)
        }
        
        modbus_frame_t write_request = write_multiple_coils_request(SLAVE_ADDRESS, 0, 2, coil_bytes);
        
        master.send_request(write_request, [relay_state](const modbus_frame_t& response) {
            if (response.function_code == 0x0F) {
                printf("Write multiple coils confirmed: %s\n", relay_state ? "ON" : "OFF");
            } else if (response.function_code & 0x80) {
                printf("Write exception: 0x%02X\n", response.data[0]);
            }
        }, 1000);
        
        free_frame(write_request);
        
        // Process write response
        for (int i = 0; i < 200; i++) {
            master.process_tx_queue();
            sleep_ms(1);
        }
        
        // Wait between commands to ensure slave is ready
        sleep_ms(500);
        
        // Step 2: Read Coils (0x01) to verify state of coils 0 and 1
        modbus_frame_t read_request = read_coils_request(SLAVE_ADDRESS, 0, 2);
        
        master.send_request(read_request, [](const modbus_frame_t& response) {
            if (response.function_code == 0x01) {
                uint8_t coil_data = response.data[1];
                bool coil_0 = (coil_data & 0x01) != 0;
                bool coil_1 = (coil_data & 0x02) != 0;
                printf("Read coils: Coil 0=%s, Coil 1=%s\n\n", 
                       coil_0 ? "ON" : "OFF", 
                       coil_1 ? "ON" : "OFF");
            } else if (response.function_code & 0x80) {
                printf("Read exception: 0x%02X\n\n", response.data[0]);
            }
        }, 1000);
        
        free_frame(read_request);
        
        // Process read response
        for (int i = 0; i < 200; i++) {
            master.process_tx_queue();
            sleep_ms(1);
        }
        
        // Wait before next toggle
        sleep_ms(4100);
    }
    
    return 0;
}