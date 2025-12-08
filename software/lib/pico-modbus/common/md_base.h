#ifndef PICO_PLC_MD_BASE_H
#define PICO_PLC_MD_BASE_H

#include <memory>
#include <functional>
#include <queue>
#include "pico/mutex.h"

#include "md_stream.h"
#include "md_common.h"

// Request context for matching responses
struct pending_request_t {
    uint8_t address;
    uint8_t function_code;
    std::function<void(const modbus_frame_t&)> callback;
    uint64_t timestamp_us;
    uint32_t timeout_ms;
};

class ModbusBase {
private:
    std::unique_ptr<ModbusStream> stream;

    std::function<void(const modbus_frame_t&)> debug_callback;
    std::function<void(const modbus_frame_t&)> error_callback;
    std::function<void(const modbus_frame_t&)> message_callback;
    
    // TX queue for background sending
    std::queue<modbus_frame_t> tx_queue;
    mutex_t tx_queue_mutex;

    void process_diagnostic();
    void process_errors();

protected:
    // Diagnostic counters accessible by derived classes
    struct {
        uint16_t BUS_MESSAGE_COUNT = 0;
        uint16_t BUS_COMMUNICATION_ERROR_COUNT = 0;
        uint16_t SLAVE_EXCEPTION_ERROR_COUNT = 0;
        uint16_t SLAVE_MESSAGE_COUNT = 0;
        uint16_t SLAVE_NO_RESPONSE_COUNT = 0;
        uint16_t SLAVE_NAK_COUNT = 0;
        uint16_t SLAVE_BUSY_COUNT = 0;
        uint16_t BUS_CHARACTER_OVERRUN_COUNT = 0;
    } diagnostic_counters;
    
    std::unique_ptr<ModbusStream>& get_stream() { return stream; }
    
    // Helper to invoke callbacks from derived classes
    void invoke_message_callback(const modbus_frame_t& frame) {
        if (message_callback) {
            message_callback(frame);
        }
    }
    
    void invoke_debug_callback(const modbus_frame_t& frame) {
        if (debug_callback) {
            debug_callback(frame);
        }
    }
    
    void invoke_error_callback(const modbus_frame_t& frame) {
        if (error_callback) {
            error_callback(frame);
        }
    }
    
    // Virtual method for derived classes to handle received frames
    virtual void handle_received_frame(const modbus_frame_t& frame) = 0;

public:
    ModbusBase(uart_inst_t* uart, uint baudrate, int de_pin = -1, int re_pin = -1);
    virtual ~ModbusBase();
    
    // Queue a frame to send (non-blocking, thread-safe)
    void queue_write(const modbus_frame_t& frame);
    
    // Check and send queued frames (call periodically or from timer)
    void process_tx_queue();

    void on_debug(const std::function<void(const modbus_frame_t&)>& callback);
    void on_error(const std::function<void(const modbus_frame_t&)>& callback);
    void on_message(const std::function<void(const modbus_frame_t &)> &callback);;
    
    // Diagnostic counter access
    uint16_t get_bus_message_count() const { return diagnostic_counters.BUS_MESSAGE_COUNT; }
    uint16_t get_bus_communication_error_count() const { return diagnostic_counters.BUS_COMMUNICATION_ERROR_COUNT; }
    uint16_t get_slave_exception_error_count() const { return diagnostic_counters.SLAVE_EXCEPTION_ERROR_COUNT; }
    uint16_t get_slave_message_count() const { return diagnostic_counters.SLAVE_MESSAGE_COUNT; }
    uint16_t get_slave_no_response_count() const { return diagnostic_counters.SLAVE_NO_RESPONSE_COUNT; }
    uint16_t get_slave_nak_count() const { return diagnostic_counters.SLAVE_NAK_COUNT; }
    uint16_t get_slave_busy_count() const { return diagnostic_counters.SLAVE_BUSY_COUNT; }
    uint16_t get_bus_character_overrun_count() const { return diagnostic_counters.BUS_CHARACTER_OVERRUN_COUNT; }
};

#endif //PICO_PLC_MD_BASE_H