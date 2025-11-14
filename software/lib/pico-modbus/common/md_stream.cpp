#include "md_stream.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pico/stdio.h>
#include "pico/stdlib.h"

uint8_t rx_buffer[256];

void on_uart_rx() {
    memset(rx_buffer, 0, 256);
    uint16_t total_received = 0;

    while (uart_is_readable(uart0)) {
        rx_buffer[total_received++] = uart_getc(uart0);
        printf("%u \n", total_received);
        // chars_rxed++;
    }

    for (int i = 0; i < total_received; i++) {
        // printf("%u ", rx_buffer[i]);
    }
    printf("\n");
}

ModbusStream::ModbusStream(uart_inst_t* uart) {
    uart_init(uart, 115200);

    // Set TX pin (GPIO 0) and RX pin (GPIO 1) for UART0
    // For Pico 2: GPIO 16 = TX, GPIO 17 = RX for UART0
    gpio_set_function(16, GPIO_FUNC_UART);
    gpio_set_function(17, GPIO_FUNC_UART);

    // UART setup
    uart_set_hw_flow(uart, false, false);  // No hardware flow control
    uart_set_format(uart, 8, 1, UART_PARITY_EVEN);  // 8 bits, 1 stop bit, parity even

    // UART interrupt handler
    int UART_IRQ = uart == uart0 ? UART0_IRQ : UART1_IRQ;
    uart_set_fifo_enabled(uart, false);
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(uart, true, false);

    // sleep_ms(100);  // Let UART stabilize
    // uart_puts(uart, "Hello world!\n");
}

ModbusStream::~ModbusStream() {
    uart_deinit(uart0);
}

void ModbusStream::write(modbus_frame_t* frame) {
    uint8_t data[3];
    data[0] = 5;
    data[1] = 2;
    data[2] = 21;

    uart_write_blocking(uart0, data, 3);
    printf("Sent: %02X %02X %02X\n", data[0], data[1], data[2]);
}

void ModbusStream::await_read(modbus_frame_t* frame, bool timeout) {
    uint8_t rx_buffer[256];

    // Wait for data with timeout (in microseconds)
    if (uart_is_readable_within_us(uart0, 1000000)) {  // 1 second timeout
        // Read available bytes (don't assume 256!)
        size_t bytes_read = 0;

        while (uart_is_readable(uart0) && bytes_read < 256) {
            rx_buffer[bytes_read++] = uart_getc(uart0);
        }

        printf("Received %zu bytes: ", bytes_read);
        for (size_t i = 0; i < bytes_read; i++) {
            printf("%02X ", rx_buffer[i]);
        }
        printf("\n");
    } else {
        printf("Timeout waiting for data\n");
    }
}