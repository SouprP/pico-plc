#include <cstdio>
#include "pico/stdlib.h"
#include "pico-modbus/common/md_base.h"

#define MASTER_UART uart0
#define MASTER_BAUDRATE 9600
#define SLAVE_ADDRESS 1

// Simulated holding registers to read from slave
uint16_t read_register_start = 0;
uint16_t read_register_count = 10;

int main() {
    stdio_init_all();
    sleep_ms(2000); // Wait for USB serial
    
    printf("\n=== Modbus Master Test ===\n");
    printf("UART: %s, Baudrate: %d\n", MASTER_UART == uart0 ? "UART0" : "UART1", MASTER_BAUDRATE);
    printf("Pins: TX=GP16, RX=GP17\n\n");
    
    // Configure UART pins
    gpio_set_function(16, GPIO_FUNC_UART); // TX
    gpio_set_function(17, GPIO_FUNC_UART); // RX
    
    // Initialize Modbus Master
    ModbusBase modbus_master(MASTER_UART, MASTER_BAUDRATE);
    
    printf("Master initialized. Starting to poll slave...\n\n");
    
    uint32_t request_count = 0;
    
    while (true) {
        // Build Read Holding Registers request (Function 0x03)
        modbus_frame_t request;
        request.address = SLAVE_ADDRESS;
        request.function_code = 0x03; // Read holding registers
        
        uint8_t request_data[4];
        request_data[0] = (read_register_start >> 8) & 0xFF;  // Starting address high
        request_data[1] = read_register_start & 0xFF;          // Starting address low
        request_data[2] = (read_register_count >> 8) & 0xFF;   // Quantity high
        request_data[3] = (read_register_count & 0xFF);          // Quantity low
        
        request.data = request_data;
        request.data_length = 4;
        
        // Send request with specific callback
        uint32_t local_request_count = ++request_count;
        modbus_master.send_request(request, [local_request_count](const modbus_frame_t& response) {
            printf("[Master] Response #%lu from slave %d\n", local_request_count, response.address);
            printf("  Function: 0x%02X\n", response.function_code);
            
            if (response.function_code == 0x03 && response.data_length > 0) {
                uint8_t byte_count = response.data[0];
                printf("  Register values: ");
                
                for (int i = 0; i < byte_count / 2; i++) {
                    uint16_t value = (response.data[1 + i*2] << 8) | response.data[1 + i*2 + 1];
                    printf("%d ", value);
                }
                printf("\n");
            }
            printf("\n");
        }, 1000); // 1 second timeout
        
        printf("[Master] Request #%lu: Read %d registers from address %d\n", 
               request_count, read_register_count, read_register_start);
        
        // Process TX queue and handle any responses
        for (int i = 0; i < 50; i++) {
            modbus_master.process_tx_queue();
            sleep_ms(10);
        }
        
        // Wait before next poll
        sleep_ms(1000);
    }
    
    return 0;
}