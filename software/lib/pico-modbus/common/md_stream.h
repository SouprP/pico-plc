#ifndef PICO_PLC_MD_STREAM_H
#define PICO_PLC_MD_STREAM_H

#include "md_common.h"
#include "hardware/uart.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include <functional>

#define MODBUS_MAX_FRAME_SIZE 256

class ModbusStream {
private:
    uart_inst_t* uart;
    uint baudrate;
    
    // Timing parameters (in microseconds)
    uint32_t t1_5_us;  // 1.5 character times
    uint32_t t3_5_us;  // 3.5 character times (frame boundary)
    
    // RX buffer and state
    uint8_t rx_buffer[MODBUS_MAX_FRAME_SIZE];
    volatile uint16_t rx_index;
    volatile bool frame_ready;
    volatile uint64_t last_rx_time_us; // Time of last received byte
    
    // Frame received callback
    std::function<void(const modbus_frame_t&)> frame_callback;
    
    // IRQ handler (must be static for C callback)
    static void uart_irq_handler();
    static ModbusStream* instance; // For accessing from static IRQ handler
    
    void handle_uart_rx();
    void reset_rx_buffer();
    void process_received_frame();

public:
    ModbusStream(uart_inst_t* uart, uint baudrate);
    ~ModbusStream();

    void write(const modbus_frame_t* frame);
    
    // Check if frame is ready and process it (call from main loop!)
    void process_if_ready();
    
    // Set callback for received frames
    void on_frame_received(const std::function<void(const modbus_frame_t&)>& callback);
    
    uint32_t get_t1_5_us() const { return t1_5_us; }
    uint32_t get_t3_5_us() const { return t3_5_us; }
};

#endif //PICO_PLC_MD_STREAM_H