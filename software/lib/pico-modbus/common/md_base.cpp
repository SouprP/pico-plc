#include "md_base.h"
#include "md_common.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>

ModbusBase::ModbusBase(uart_inst_t* uart, uint baudrate, int de_pin, int re_pin) {
    mutex_init(&tx_queue_mutex);

    // UART handler class with RS485 transceiver control
    stream = std::make_unique<ModbusStream>(uart, baudrate, de_pin, re_pin);
    
    // callback for received frames (called from IRQ!)
    stream->on_frame_received([this](const modbus_frame_t& frame) {
        this->handle_received_frame(frame);
    });
    
    // error callback for CRC failures
    stream->on_error_received([this](const modbus_frame_t& frame) {
        this->diagnostic_counters.BUS_COMMUNICATION_ERROR_COUNT++;
    });
}

ModbusBase::~ModbusBase() {
    mutex_exit(&tx_queue_mutex);
    // mutex_deinit(&tx_queue_mutex);
}

void ModbusBase::queue_write(const modbus_frame_t& frame) {
    mutex_enter_blocking(&tx_queue_mutex);

    modbus_frame_t frame_copy = frame;
    if (frame.data && frame.data_length > 0) {
        frame_copy.data = (uint8_t*)malloc(frame.data_length);
        memcpy(frame_copy.data, frame.data, frame.data_length);
    }
    
    tx_queue.push(frame_copy);
    mutex_exit(&tx_queue_mutex);
}

void ModbusBase::process_tx_queue() {
    // Check for incoming frames (T3.5 silence detection)
    stream->process_if_ready();
    
    // Then check if there are frames to send
    mutex_enter_blocking(&tx_queue_mutex);
    
    if (!tx_queue.empty()) {
        modbus_frame_t frame = tx_queue.front();
        tx_queue.pop();
        mutex_exit(&tx_queue_mutex);

        // Send the frame
        stream->write(&frame);
        diagnostic_counters.BUS_MESSAGE_COUNT++;
        MODBUS_DEBUG_PRINT("[DIAG] BUS_MESSAGE_COUNT incremented to %u\n", diagnostic_counters.BUS_MESSAGE_COUNT);

        if (frame.data) {
            free(frame.data);
        }
    } else {
        mutex_exit(&tx_queue_mutex);
    }
}

void ModbusBase::on_debug(const std::function<void( const modbus_frame_t&)>& callback) {
    debug_callback = callback;
}

void ModbusBase::on_error(const std::function<void(const modbus_frame_t&)>& callback) {
    error_callback = callback;
}

void ModbusBase::on_message(const std::function<void(const modbus_frame_t&)>& callback) {
    message_callback = callback;
}


