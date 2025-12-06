#include "md_master.h"
#include <cstdlib>
#include <cstdio>

ModbusMaster::ModbusMaster(uart_inst_t* uart, uint baudrate)
    : ModbusBase(uart, baudrate), pending_request(nullptr), request_sent(false) {
    // Master has no address in Modbus RTU
    mutex_init(&request_mutex);
}

ModbusMaster::~ModbusMaster() {
    if (pending_request != nullptr) {
        delete pending_request;
    }
}

void ModbusMaster::handle_received_frame(const modbus_frame_t& frame) {
    // Check if this matches the pending request
    mutex_enter_blocking(&request_mutex);
    
    bool matched = false;
    if (pending_request != nullptr) {
        // Match by address and function code (handle normal and exception responses)
        bool is_normal_response = (pending_request->function_code == frame.function_code);
        bool is_exception_response = (frame.function_code == (pending_request->function_code | 0x80));

        if (pending_request->address == frame.address && (is_normal_response || is_exception_response)) {
            // Call the specific callback for this request
            if (pending_request->callback) {
                pending_request->callback(frame);
            }
            delete pending_request;
            pending_request = nullptr;
            matched = true;
        }
    }
    
    mutex_exit(&request_mutex);
}

void ModbusMaster::send_request(const modbus_frame_t& frame, 
                                const std::function<void(const modbus_frame_t&)>& callback,
                                uint32_t timeout_ms) {
    // Queue the frame for sending
    queue_write(frame);
    
    mutex_enter_blocking(&request_mutex);
    
    // Clear any existing pending request (should not happen in normal use)
    if (pending_request != nullptr) {
        delete pending_request;
    }
    
    // Create new pending request
    pending_request = new pending_request_t();
    pending_request->address = frame.address;
    pending_request->function_code = frame.function_code;
    pending_request->callback = callback;
    pending_request->timestamp_us = time_us_64();
    pending_request->timeout_ms = timeout_ms;
    request_sent = false; // Mark as not sent yet
    
    mutex_exit(&request_mutex);
}

void ModbusMaster::check_request_timeouts() {
    mutex_enter_blocking(&request_mutex);
    
    bool timed_out = false;
    if (pending_request != nullptr) {
        uint64_t elapsed_us = time_us_64() - pending_request->timestamp_us;
        uint64_t timeout_us = (uint64_t)pending_request->timeout_ms * 1000ULL;
        
        if (elapsed_us > timeout_us) {
            printf("[Master] Request TIMEOUT (addr=%d, func=0x%02X)\n", 
                   pending_request->address, pending_request->function_code);
            
            // Timeout - remove request
            delete pending_request;
            pending_request = nullptr;
            timed_out = true;
        }
    }
    
    mutex_exit(&request_mutex);
    
    // Increment no response counter after releasing mutex
    if (timed_out) {
        diagnostic_counters.SLAVE_NO_RESPONSE_COUNT++;
    }
}

void ModbusMaster::process_tx_queue() {
    mutex_enter_blocking(&request_mutex);
    bool has_pending = (pending_request != nullptr);
    bool sent = request_sent;
    mutex_exit(&request_mutex);
    
    // Allow sending if: no pending request OR pending request hasn't been sent yet
    if (!has_pending || !sent) {
        ModbusBase::process_tx_queue();
        
        // Mark request as sent after calling base class
        mutex_enter_blocking(&request_mutex);
        if (pending_request != nullptr) {
            request_sent = true;
        }
        mutex_exit(&request_mutex);
    }
    
    // Check for request timeouts
    check_request_timeouts();
}

void ModbusMaster::send_diagnostic_request(uint8_t slave_addr, uint16_t sub_function, uint16_t data,
                                           const std::function<void(const modbus_frame_t&)>& callback,
                                           uint32_t timeout_ms) {
    modbus_frame_t request = read_diagnostics_request(slave_addr, sub_function, data);
    send_request(request, callback, timeout_ms);
    free_frame(request);
}
