#ifndef PICO_PLC_MD_STREAM_H
#define PICO_PLC_MD_STREAM_H

#include "md_common.h"
#include "hardware/uart.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include <functional>

#define MODBUS_MAX_FRAME_SIZE 256

class ModbusStream {
private:
    uart_inst_t* uart;
    uint baudrate;
    
    // RS485 transceiver control pins (SN65HVD75DGKR)
    int de_pin;  // Driver Enable (active HIGH)
    int re_pin;  // Receiver Enable (active LOW)
    
    // timing parameters (in microseconds)
    uint32_t t1_5_us;  // 1.5 character times
    uint32_t t3_5_us;  // 3.5 character times (frame boundary)
    
    // RX buffer and state
    uint8_t rx_buffer[MODBUS_MAX_FRAME_SIZE];
    volatile uint16_t rx_index;
    volatile bool frame_ready;
    volatile uint64_t last_rx_time_us; // Time of last received byte
    volatile bool tx_in_progress; // Flag to ignore RX during TX
    
    // callbacks for frame and error handling
    std::function<void(const modbus_frame_t&)> frame_callback;
    std::function<void(const modbus_frame_t&)> error_callback;
    
    // IRQ handler (must be static for C callback)
    static void uart_irq_handler();
    static ModbusStream* instance;
    
    void handle_uart_rx();
    void reset_rx_buffer();
    void process_received_frame();
    
    // RS485 transceiver control
    void set_transceiver_mode_tx();
    void set_transceiver_mode_rx();

public:
    ModbusStream(uart_inst_t* uart, uint baudrate, int de_pin = -1, int re_pin = -1, ModbusParity parity = ModbusParity::EVEN);
    ~ModbusStream();

    void write(const modbus_frame_t* frame);

    void process_if_ready();

    // callbacks for received frames
    void on_frame_received(const std::function<void(const modbus_frame_t&)>& callback);
    void on_error_received(const std::function<void(const modbus_frame_t&)>& callback);
    
    uint32_t get_t1_5_us() const { return t1_5_us; }
    uint32_t get_t3_5_us() const { return t3_5_us; }
    
    // Get time since last RX byte (in microseconds) - useful for timeout logic
    uint64_t get_time_since_last_rx() const {
        if (last_rx_time_us == 0) return UINT64_MAX;
        return time_us_64() - last_rx_time_us;
    }
};

#endif //PICO_PLC_MD_STREAM_H