#include "md_stream.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "pico-utils/common_utils.h"

// Static member initialization
ModbusStream* ModbusStream::instance = nullptr;

ModbusStream::ModbusStream(uart_inst_t* uart, uint baudrate) 
    : uart(uart), baudrate(baudrate), rx_index(0), frame_ready(false), last_rx_time_us(0) {
    
    // Store instance for static IRQ handler access
    instance = this;
    led_gpio_init();
    
    // Calculate character time in microseconds
    // Modbus RTU: 1 start bit + 8 data bits + parity + 1 stop bit = 11 bits per character
    uint32_t char_time_us = (11 * 1000000) / baudrate;
    
    // Calculate timing thresholds
    t1_5_us = (char_time_us * 3) / 2;  // 1.5 character times
    t3_5_us = (char_time_us * 7) / 2;  // 3.5 character times
    
    // For baudrates > 19200, use fixed timing per Modbus spec
    if (baudrate > 19200) {
        t1_5_us = 750;   // 750 microseconds
        t3_5_us = 1750;  // 1750 microseconds
    }
    
    // Initialize UART
    uart_init(uart, baudrate);
    
    // Set UART format: 8E1 (8 data bits, even parity, 1 stop bit)
    uart_set_format(uart, 8, 1, UART_PARITY_EVEN);
    
    // Enable UART FIFO (32 bytes deep)
    uart_set_fifo_enabled(uart, true);
    
    // Set up UART RX interrupt
    int uart_irq = (uart == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(uart_irq, uart_irq_handler);
    irq_set_enabled(uart_irq, true);
    
    // Enable RX interrupt when FIFO has data
    uart_set_irq_enables(uart, true, false);
    
    reset_rx_buffer();
}

ModbusStream::~ModbusStream() {
    int uart_irq = (uart == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_enabled(uart_irq, false);
    
    uart_deinit(uart);
    instance = nullptr;
}

void ModbusStream::uart_irq_handler() {
    if (instance) {
        instance->handle_uart_rx();
    }
}

void ModbusStream::handle_uart_rx() {
    // Read all available bytes from FIFO
    while (uart_is_readable(uart) && rx_index < MODBUS_MAX_FRAME_SIZE) {
        uint8_t byte = uart_getc(uart);
        rx_buffer[rx_index++] = byte;
        
        // Update timestamp of last received byte
        last_rx_time_us = time_us_64();
    }
    
    // Check for buffer overflow
    if (rx_index >= MODBUS_MAX_FRAME_SIZE) {
        printf("[ERROR] Buffer overflow!\n");
        reset_rx_buffer();
    }
}

void ModbusStream::process_if_ready() {
    // Check if we have data and if T3.5 silence has elapsed
    if (rx_index > 0 && last_rx_time_us > 0) {
        uint64_t elapsed = time_us_64() - last_rx_time_us;
        
        if (elapsed >= t3_5_us) {
            // T3.5 silence detected - process frame
            process_received_frame();
        }
    }
}

void ModbusStream::reset_rx_buffer() {
    rx_index = 0;
    frame_ready = false;
    last_rx_time_us = 0;
    memset(rx_buffer, 0, MODBUS_MAX_FRAME_SIZE);
}

void ModbusStream::process_received_frame() {
    // Minimum frame: Address + Function + CRC (2 bytes) = 4 bytes
    if (rx_index < 4) {
        reset_rx_buffer();
        return;
    }
    
    // Build frame structure
    modbus_frame_t frame;
    frame.address = rx_buffer[0];
    frame.function_code = rx_buffer[1];
    frame.data_length = rx_index - 4; // Minus address, function, and CRC
    
    // Allocate and copy data if present
    if (frame.data_length > 0) {
        frame.data = (uint8_t*)malloc(frame.data_length);
        if (frame.data) {
            memcpy(frame.data, &rx_buffer[2], frame.data_length);
        }
    } else {
        frame.data = nullptr;
    }
    
    // Extract CRC (little-endian)
    frame.crc = rx_buffer[rx_index - 2] | (rx_buffer[rx_index - 1] << 8);
    
    // Verify CRC
    bool crc_valid = check_crc(&frame);
    
    // Reset buffer for next frame
    reset_rx_buffer();
    
    // Call callback if CRC is valid and callback is set
    if (crc_valid && frame_callback) {
        frame_callback(frame);
    }
    
    // Free allocated data
    if (frame.data) {
        free(frame.data);
    }
}

void ModbusStream::on_frame_received(const std::function<void(const modbus_frame_t&)>& callback) {
    frame_callback = callback;
}

void ModbusStream::write(const modbus_frame_t* frame) {
    led_gpio_set(1);
    // Wait for T3.5 silence before transmitting (bus idle)
    sleep_us(t3_5_us);
    
    // Build frame: Address + Function + Data + CRC
    uint8_t tx_buffer[MODBUS_MAX_FRAME_SIZE];
    uint16_t tx_length = 0;
    
    tx_buffer[tx_length++] = frame->address;
    tx_buffer[tx_length++] = frame->function_code;
    
    // Copy data
    if (frame->data && frame->data_length > 0) {
        memcpy(&tx_buffer[tx_length], frame->data, frame->data_length);
        tx_length += frame->data_length;
    }
    
    // Calculate and append CRC
    uint16_t crc = calculate_crc(tx_buffer, tx_length);
    tx_buffer[tx_length++] = crc & 0xFF;        // CRC low byte
    tx_buffer[tx_length++] = (crc >> 8) & 0xFF; // CRC high byte
    
    // Transmit frame
    uart_write_blocking(uart, tx_buffer, tx_length);
    led_gpio_set(0);
}