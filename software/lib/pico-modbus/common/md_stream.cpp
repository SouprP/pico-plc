#include "md_stream.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "pico-utils/common_utils.h"

// Static member initialization
ModbusStream* ModbusStream::instance = nullptr;

ModbusStream::ModbusStream(uart_inst_t* uart, uint baudrate) 
    : uart(uart), baudrate(baudrate), rx_index(0), frame_ready(false), last_rx_time_us(0) {

    instance = this;
    
    // calculation of character time in microseconds
    // Modbus RTU character: 1 start bit + 8 data bits + parity + 1 stop bit = 11 bits
    uint32_t char_time_us = (11 * 1000000) / baudrate;

    t1_5_us = (char_time_us * 3) / 2;  // 1.5 character times
    t3_5_us = (char_time_us * 7) / 2;  // 3.5 character times
    
    // for baudrates > 19200 use fixed timing per Modbus spec
    if (baudrate > 19200) {
        t1_5_us = 750;   // 750 microseconds
        t3_5_us = 1750;  // 1750 microseconds
    }

    uart_init(uart, baudrate);
    uart_set_format(uart, 8, 1, UART_PARITY_EVEN);
    uart_set_fifo_enabled(uart, true);
    
    // RX interrupt for UART
    int uart_irq = (uart == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(uart_irq, uart_irq_handler);
    irq_set_enabled(uart_irq, true);
    uart_set_irq_enables(uart, true, false);
    
    // flush any garbage in RX FIFO
    while (uart_is_readable(uart)) {
        uart_getc(uart);
    }
    
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
    // read all available bytes from FIFO
    while (uart_is_readable(uart) && rx_index < MODBUS_MAX_FRAME_SIZE) {
        uint8_t byte = uart_getc(uart);
        rx_buffer[rx_index++] = byte;
        printf("[IRQ] RX byte: 0x%02X (total=%d)\n", byte, rx_index);
        
        // update timestamp
        last_rx_time_us = time_us_64();
    }
    
    // check for buffer overflow
    if (rx_index >= MODBUS_MAX_FRAME_SIZE) {
        reset_rx_buffer();
    }
}

void ModbusStream::process_if_ready() {
    // Check if we have data and if T3.5 silence has elapsed
    if (rx_index > 0 && last_rx_time_us > 0) {
        uint64_t elapsed = time_us_64() - last_rx_time_us;
        
        if (elapsed >= t3_5_us) {
            printf("[T3.5] Silence detected, processing frame\n");
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
    printf("[FRAME] Processing %d bytes\n", rx_index);
    
    // minimum frame: address + function + CRC (2 bytes) = 4 bytes
    // Per spec: frames < 3 bytes also count as communication errors
    if (rx_index < 4) {
        printf("[FRAME] Too short, discarding\n");
        if (error_callback) {
            // Create dummy frame for error reporting
            modbus_frame_t error_frame = {0};
            error_callback(error_frame);
        }
        reset_rx_buffer();
        return;
    }
    
    // filter out obvious noise (all zeros = floating RS485 line)
    // add termination resistors between lines or pull ups / down downs on lines
    bool all_zeros = true;
    for (int i = 0; i < rx_index; i++) {
        if (rx_buffer[i] != 0x00) {
            all_zeros = false;
            break;
        }
    }
    if (all_zeros) {
        printf("[FRAME] All zeros (floating line noise), discarding\n");
        reset_rx_buffer();
        return;
    }

    modbus_frame_t frame;
    frame.address = rx_buffer[0];
    frame.function_code = rx_buffer[1];
    frame.data_length = rx_index - 4; // Minus address, function, and CRC

    if (frame.data_length > 0) {
        frame.data = (uint8_t*)malloc(frame.data_length);
        if (frame.data) {
            memcpy(frame.data, &rx_buffer[2], frame.data_length);
        }
    } else {
        frame.data = nullptr;
    }

    frame.crc = rx_buffer[rx_index - 2] | (rx_buffer[rx_index - 1] << 8);

    bool crc_valid = check_crc(&frame);
    printf("[FRAME] Addr=%d Func=0x%02X CRC=%s\n", 
           frame.address, frame.function_code, crc_valid ? "OK" : "FAIL");

    reset_rx_buffer();
    
    // received a frame, invoke callback
    if (crc_valid && frame_callback) {
        frame_callback(frame);
    } else if (!frame_callback) {
        // there is nothing we can do...... Viva La Napoleon!
    } else if (!crc_valid && error_callback) {
        error_callback(frame);
    }

    if (frame.data) {
        free(frame.data);
    }
}

void ModbusStream::on_frame_received(const std::function<void(const modbus_frame_t&)>& callback) {
    frame_callback = callback;
}

void ModbusStream::on_error_received(const std::function<void(const modbus_frame_t&)>& callback) {
    error_callback = callback;
}

void ModbusStream::write(const modbus_frame_t* frame) {
    led_gpio_set(1);
    printf("[TX] Sending frame: Addr=%d Func=0x%02X DataLen=%d\n",
           frame->address, frame->function_code, frame->data_length);
    
    // wait for T3.5 silence before transmitting (bus idle)
    sleep_us(t3_5_us);

    uint8_t tx_buffer[MODBUS_MAX_FRAME_SIZE];
    uint16_t tx_length = 0;
    
    tx_buffer[tx_length++] = frame->address;
    tx_buffer[tx_length++] = frame->function_code;

    if (frame->data && frame->data_length > 0) {
        memcpy(&tx_buffer[tx_length], frame->data, frame->data_length);
        tx_length += frame->data_length;
    }

    uint16_t crc = calculate_crc(tx_buffer, tx_length);
    tx_buffer[tx_length++] = crc & 0xFF;        // CRC low byte
    tx_buffer[tx_length++] = (crc >> 8) & 0xFF; // CRC high byte
    
    printf("[TX] Total %d bytes, CRC=0x%04X\n", tx_length, crc);
    
    // Disable RX IRQ to prevent receiving our own transmission (RS485 echo)
    uart_set_irq_enables(uart, false, false);
    
    // Flush any bytes that arrived during preparation
    while (uart_is_readable(uart)) {
        uart_getc(uart);
    }
    
    // transmit frame
    uart_write_blocking(uart, tx_buffer, tx_length);
    
    // Wait for transmission to complete
    uart_tx_wait_blocking(uart);
    
    // Flush any echo bytes
    sleep_us(t3_5_us);
    while (uart_is_readable(uart)) {
        uart_getc(uart);
    }
    
    // Re-enable RX IRQ
    uart_set_irq_enables(uart, true, false);
    
    printf("[TX] Frame sent\n");
    led_gpio_set(0);
}