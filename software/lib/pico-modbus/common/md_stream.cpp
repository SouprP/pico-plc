#include "md_stream.h"
#include "md_common.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "pico-utils/common_utils.h"

// Static member initialization
ModbusStream* ModbusStream::instance = nullptr;

ModbusStream::ModbusStream(uart_inst_t* uart, uint baudrate, int de_pin, int re_pin, ModbusParity parity) 
    : uart(uart), baudrate(baudrate), de_pin(de_pin), re_pin(re_pin),
      rx_index(0), frame_ready(false), last_rx_time_us(0), tx_in_progress(false) {

    instance = this;
    
    // Initialize RS485 transceiver control pins (SN65HVD75DGKR)
    if (de_pin >= 0) {
        gpio_init(de_pin);
        gpio_set_dir(de_pin, GPIO_OUT);
        gpio_put(de_pin, 0); // DE=0: Driver disabled (receive mode)
        MODBUS_DEBUG_PRINT("[RS485] DE pin %d initialized (Driver Disable)\n", de_pin);
    }
    
    if (re_pin >= 0) {
        gpio_init(re_pin);
        gpio_set_dir(re_pin, GPIO_OUT);
        gpio_put(re_pin, 0); // RE=0: Receiver enabled
        MODBUS_DEBUG_PRINT("[RS485] RE pin %d initialized (Receiver Enable)\n", re_pin);
    }
    
    // If both pins configured, confirm RX mode
    if (de_pin >= 0 && re_pin >= 0) {
        MODBUS_DEBUG_PRINT("[RS485] SN65HVD75DGKR in receive mode (DE=0, RE=0)\n");
    }
    
    // calculation of character time in microseconds
    // Modbus RTU character: 1 start bit + 8 data bits + parity + 1 stop bit = 11 bits
    // If parity is NONE, 2 stop bits are used, so still 11 bits (1 start + 8 data + 2 stop)
    uint32_t char_time_us = (11 * 1000000) / baudrate;

    t1_5_us = (char_time_us * 3) / 2;  // 1.5 character times
    t3_5_us = (char_time_us * 7) / 2;  // 3.5 character times
    
    // for baudrates > 19200 use fixed timing per Modbus spec
    if (baudrate > 19200) {
        t1_5_us = 750;   // 750 microseconds
        t3_5_us = 1750;  // 1750 microseconds
    }

    uart_init(uart, baudrate);
    
    // Configure Parity and Stop Bits
    switch (parity) {
        case ModbusParity::NONE:
            // 8 Data bits, No Parity, 2 Stop bits (8N2)
            uart_set_format(uart, 8, 2, UART_PARITY_NONE);
            MODBUS_DEBUG_PRINT("[UART] Configured 8N2 (No Parity, 2 Stop Bits)\n");
            break;
        case ModbusParity::ODD:
            // 8 Data bits, Odd Parity, 1 Stop bit (8O1)
            uart_set_format(uart, 8, 1, UART_PARITY_ODD);
            MODBUS_DEBUG_PRINT("[UART] Configured 8O1 (Odd Parity, 1 Stop Bit)\n");
            break;
        case ModbusParity::EVEN:
        default:
            // 8 Data bits, Even Parity, 1 Stop bit (8E1) - Default
            uart_set_format(uart, 8, 1, UART_PARITY_EVEN);
            MODBUS_DEBUG_PRINT("[UART] Configured 8E1 (Even Parity, 1 Stop Bit)\n");
            break;
    }
    
    // Disable FIFO to ensure immediate interrupts for strict Modbus timing
    // This prevents "perceived gaps" caused by FIFO timeout delays
    uart_set_fifo_enabled(uart, false);
    
    // RX interrupt for UART
    int uart_irq = (uart == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(uart_irq, uart_irq_handler);
    
    // Set UART IRQ to highest priority to prevent USB/other IRQs from blocking it
    irq_set_priority(uart_irq, 0x00);  // 0 = highest priority
    
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
    // Ignore RX during TX to prevent echo
    if (tx_in_progress) {
        // Flush echo bytes
        while (uart_is_readable(uart)) {
            uart_getc(uart);
        }
        return;
    }
    
    // read all available bytes from FIFO
    while (uart_is_readable(uart) && rx_index < MODBUS_MAX_FRAME_SIZE) {
        // Check for T1.5 violation (inter-character timeout)
        if (rx_index > 0) {
            uint64_t current_time = time_us_64();
            uint64_t elapsed = current_time - last_rx_time_us;
            
            if (elapsed > t1_5_us) {
                // T1.5 violation detected - discard partial frame
                // The current byte will be treated as the start of a new frame
                rx_index = 0;
            }
        }

        // Read raw data from DR register to get error flags
        // Bits: 11=OE (Overrun), 10=BE (Break), 9=PE (Parity), 8=FE (Framing)
        uint32_t dr = uart_get_hw(uart)->dr;
        
        if (dr & 0xF00) {
            // Hardware error detected (Parity, Framing, Overrun, Break)
            // Modbus spec requires ignoring the frame if a parity/framing error occurs.
            // We reset the buffer to discard the current frame.
            reset_rx_buffer();
            
            // We continue to read to clear the FIFO, but we don't store the data
            // Update timestamp to prevent T3.5 from triggering immediately on next byte
            last_rx_time_us = time_us_64();
            continue;
        }

        uint8_t byte = dr & 0xFF;
        rx_buffer[rx_index++] = byte;
        // NOTE: No printf in IRQ handler - it blocks subsequent byte reception!
        
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
            MODBUS_DEBUG_PRINT("[T3.5] Silence detected, processing frame\n");
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
    MODBUS_DEBUG_PRINT("[FRAME] Processing %d bytes\n", rx_index);
    
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
        MODBUS_DEBUG_PRINT("[FRAME] All zeros (floating line noise), discarding\n");
        reset_rx_buffer();
        return;
    }

    // minimum frame: address + function + CRC (2 bytes) = 4 bytes
    // Per spec: frames < 3 bytes also count as communication errors
    if (rx_index < 4) {
        MODBUS_DEBUG_PRINT("[FRAME] Too short, discarding\n");
        if (error_callback) {
            // Create dummy frame for error reporting
            modbus_frame_t error_frame = {0};
            error_callback(error_frame);
        }
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
    MODBUS_DEBUG_PRINT("[FRAME] Addr=%d Func=0x%02X CRC=%s\n", 
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
    // led_gpio_set(1);
    
    // Disable RX IRQ immediately to prevent echo during TX
    uart_set_irq_enables(uart, false, false);
    
    // Set flag to ignore RX during TX
    tx_in_progress = true;
    
    // Reset RX buffer to discard any partial data
    rx_index = 0;
    // DO NOT clear last_rx_time_us - let IRQ handler update it when response arrives
    
    // Flush UART FIFO
    while (uart_is_readable(uart)) {
        uart_getc(uart);
    }
    
    MODBUS_DEBUG_PRINT("[TX] Sending frame: Addr=%d Func=0x%02X DataLen=%d\n",
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
    
    MODBUS_DEBUG_PRINT("[TX] Total %d bytes, CRC=0x%04X\n", tx_length, crc);
    
    // Switch RS485 transceiver to TX mode (SN65HVD75DGKR: DE=1, RE=1)
    set_transceiver_mode_tx();
    
    // transmit frame
    uart_write_blocking(uart, tx_buffer, tx_length);
    
    // Wait for transmission to complete
    uart_tx_wait_blocking(uart);
    
    // Switch RS485 transceiver back to RX mode (SN65HVD75DGKR: DE=0, RE=0)
    set_transceiver_mode_rx();
    
    // Clear the TX flag BEFORE re-enabling IRQ
    tx_in_progress = false;
    
    // Re-enable RX IRQ
    uart_set_irq_enables(uart, true, false);
    
    MODBUS_DEBUG_PRINT("[TX] Frame sent\n");
    // led_gpio_set(0);
}

void ModbusStream::set_transceiver_mode_tx() {
    // SN65HVD75DGKR: DE=1 (Driver Enable), RE=1 (Receiver Disable)
    if (de_pin >= 0) {
        gpio_put(de_pin, 1);
    }
    if (re_pin >= 0) {
        gpio_put(re_pin, 1);
    }
    // Small delay for transceiver to switch modes (typically ~100ns, but we add margin)
    sleep_us(1);
}

void ModbusStream::set_transceiver_mode_rx() {
    // SN65HVD75DGKR: DE=0 (Driver Disable), RE=0 (Receiver Enable)
    if (de_pin >= 0) {
        gpio_put(de_pin, 0);
    }
    if (re_pin >= 0) {
        gpio_put(re_pin, 0);
    }
    // Small delay for transceiver to switch modes
    sleep_us(1);
}