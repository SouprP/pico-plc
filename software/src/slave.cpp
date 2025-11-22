#include <cstdio>
#include "pico/stdlib.h"
#include "pico-modbus/common/md_base.h"

#define SLAVE_UART uart0
#define SLAVE_BAUDRATE 9600
#define MY_SLAVE_ADDRESS 1

// Simulated holding registers (256 registers)
uint16_t holding_registers[256];

// Initialize registers with some test data
void init_registers() {
    for (int i = 0; i < 256; i++) {
        holding_registers[i] = 100 + i; // Values 100, 101, 102, ...
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000); // Wait for USB serial
    
    printf("\n=== Modbus Slave Test ===\n");
    printf("Slave Address: %d\n", MY_SLAVE_ADDRESS);
    printf("UART: %s, Baudrate: %d\n", SLAVE_UART == uart0 ? "UART0" : "UART1", SLAVE_BAUDRATE);
    printf("Pins: TX=GP16, RX=GP17\n\n");
    
    // Initialize registers
    init_registers();
    
    // Configure UART pins
    gpio_set_function(16, GPIO_FUNC_UART); // TX
    gpio_set_function(17, GPIO_FUNC_UART); // RX
    
    // Initialize Modbus Slave
    ModbusBase modbus_slave(SLAVE_UART, SLAVE_BAUDRATE);
    
    // Set up callback for requests from master
    modbus_slave.on_message([&modbus_slave](const uint8_t& address, const modbus_frame_t& frame) {
        // Ignore messages not addressed to us
        if (address != MY_SLAVE_ADDRESS) {
            return;
        }
        
        printf("[Slave] Received request from master\n");
        printf("  Address: %d\n", address);
        printf("  Function: 0x%02X\n", frame.function_code);
        
        // Handle Read Holding Registers (0x03)
        if (frame.function_code == 0x03) {
            if (frame.data_length >= 4) {
                // Parse request
                uint16_t start_addr = (frame.data[0] << 8) | frame.data[1];
                uint16_t quantity = (frame.data[2] << 8) | frame.data[3];
                
                printf("  Start Address: %d\n", start_addr);
                printf("  Quantity: %d\n", quantity);
                
                // Validate request
                if (start_addr + quantity > 256) {
                    printf("  ERROR: Register address out of range\n\n");
                    // TODO: Send exception response
                    return;
                }
                
                if (quantity < 1 || quantity > 125) {
                    printf("  ERROR: Invalid quantity\n\n");
                    // TODO: Send exception response
                    return;
                }
                
                // Build response
                modbus_frame_t response;
                response.address = MY_SLAVE_ADDRESS;
                response.function_code = 0x03;
                
                uint8_t byte_count = quantity * 2;
                uint8_t* response_data = new uint8_t[1 + byte_count];
                response_data[0] = byte_count; // Byte count
                
                // Fill with register values
                for (int i = 0; i < quantity; i++) {
                    uint16_t reg_value = holding_registers[start_addr + i];
                    response_data[1 + i*2] = (reg_value >> 8) & 0xFF;     // High byte
                    response_data[1 + i*2 + 1] = reg_value & 0xFF;        // Low byte
                }
                
                response.data = response_data;
                response.data_length = 1 + byte_count;
                
                // Queue response
                modbus_slave.queue_write(response);
                printf("  Sending response with %d registers\n\n", quantity);
                
                delete[] response_data;
            }
        }
        // Handle Write Single Register (0x06)
        else if (frame.function_code == 0x06) {
            if (frame.data_length >= 4) {
                uint16_t reg_addr = (frame.data[0] << 8) | frame.data[1];
                uint16_t reg_value = (frame.data[2] << 8) | frame.data[3];
                
                printf("  Register Address: %d\n", reg_addr);
                printf("  Value: %d\n", reg_value);
                
                if (reg_addr < 256) {
                    holding_registers[reg_addr] = reg_value;
                    
                    // Echo back the request as response
                    modbus_frame_t response;
                    response.address = MY_SLAVE_ADDRESS;
                    response.function_code = 0x06;
                    response.data = frame.data;
                    response.data_length = frame.data_length;
                    
                    modbus_slave.queue_write(response);
                    printf("  Register updated, sending response\n\n");
                }
            }
        }
        else {
            printf("  Unsupported function code\n\n");
        }
    });
    
    printf("Slave initialized and listening...\n\n");
    
    // Main loop - process TX queue and update registers
    uint32_t loop_count = 0;
    uint32_t last_print = 0;
    
    while (true) {
        // Process any queued responses
        modbus_slave.process_tx_queue();
        
        // Simulate changing register values (for testing)
        if (loop_count % 100 == 0) {
            holding_registers[0]++; // Increment first register periodically
        }
        
        loop_count++;
        sleep_ms(10);
    }
    
    return 0;
}