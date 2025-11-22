#ifndef PICO_PLC_MD_BASE_H
#define PICO_PLC_MD_BASE_H

#include <memory>
#include <functional>
#include <queue>
#include <map>
#include "pico/mutex.h"

#include "md_stream.h"
#include "md_common.h"

// Transaction context for matching requests to responses
struct modbus_transaction_t {
    uint16_t transaction_id;
    uint8_t address;
    uint8_t function_code;
    std::function<void(const modbus_frame_t&)> callback;
    uint64_t timestamp_us;
    uint32_t timeout_ms;
};

class ModbusBase {
private:
    std::unique_ptr<ModbusStream> stream;

    std::function<void(const uint8_t&, const modbus_frame_t&)> debug_callback;
    std::function<void(const uint8_t&, const modbus_frame_t&)> error_callback;
    std::function<void(const uint8_t&, const modbus_frame_t&)> message_callback;

    mutable struct {
        uint16_t BUS_MESSAGE_COUNT = 0;
        uint16_t BUS_COMMUNICATION_ERROR_COUNT = 0;
        uint16_t SLAVE_EXCEPTION_ERROR_COUNT = 0;
        uint16_t SLAVE_MESSAGE_COUNT = 0;
        uint16_t SLAVE_NO_RESPONSE_COUNT = 0;
        uint16_t SLAVE_NAK_COUNT = 0;
        uint16_t SLAVE_BUSY_COUNT = 0;
        uint16_t BUS_CHARACTER_OVERRUN_COUNT = 0;
    } diagnostic_counters;
    
    // TX queue for background sending
    std::queue<modbus_frame_t> tx_queue;
    mutex_t tx_queue_mutex;
    
    // Transaction tracking for matching responses to requests
    std::queue<modbus_transaction_t> pending_transactions;
    mutex_t transaction_mutex;
    uint16_t next_transaction_id;

    void process_diagnostic();
    void process_errors();
    void handle_received_frame(const modbus_frame_t& frame);
    void check_transaction_timeouts();

public:
    ModbusBase(uart_inst_t* uart, uint baudrate);
    ~ModbusBase();
    
    // Queue a frame to send (non-blocking, thread-safe)
    void queue_write(const modbus_frame_t& frame);
    
    // Send request with callback for response (returns transaction ID)
    uint16_t send_request(const modbus_frame_t& frame, 
                          const std::function<void(const modbus_frame_t&)>& callback,
                          uint32_t timeout_ms = 1000);
    
    // Check and send queued frames (call periodically or from timer)
    void process_tx_queue();

    void on_debug(const std::function<void(const uint8_t&, const modbus_frame_t&)>& callback);
    void on_error(const std::function<void(const uint8_t&, const modbus_frame_t&)>& callback);
    void on_message(const std::function<void(const uint8_t &, const modbus_frame_t &)> &callback);
};

#endif //PICO_PLC_MD_BASE_H