#include "md_master.h"
#include "common/md_common.h"
#include <cstdlib>
#include <cstdio>

ModbusMaster::ModbusMaster(uart_inst_t* uart, uint baudrate, int de_pin, int re_pin)
    : ModbusBase(uart, baudrate, de_pin, re_pin), pending_request(nullptr), request_sent(false) {
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
    if (pending_request != nullptr && request_sent) {
        uint64_t elapsed_us = time_us_64() - pending_request->timestamp_us;
        uint64_t timeout_us = (uint64_t)pending_request->timeout_ms * 1000ULL;
        
        // Only timeout if enough time has passed AND we're not currently receiving a frame
        // We must wait at least T3.5 after the last received byte before declaring timeout
        // This gives the frame time to be detected and processed by the T3.5 silence timer
        uint64_t time_since_last_rx = get_stream()->get_time_since_last_rx();
        uint32_t t3_5_us = get_stream()->get_t3_5_us();
        
        // If we received ANY bytes, wait at least T3.5 for silence detection to trigger
        bool waiting_for_frame_completion = (time_since_last_rx < t3_5_us);
        
        if (elapsed_us > timeout_us && !waiting_for_frame_completion) {
            MODBUS_DEBUG_PRINT("[Master] Request TIMEOUT (addr=%d, func=0x%02X)\n", 
                   pending_request->address, pending_request->function_code);
            
            // Timeout - remove request
            delete pending_request;
            pending_request = nullptr;
            request_sent = false;
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
    // Always check for incoming frames (T3.5 silence detection)
    // This is critical because ModbusBase::process_tx_queue() is skipped
    // when we are waiting for a response, but we still need to process RX!
    get_stream()->process_if_ready();

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

void ModbusMaster::send_read_holding_registers_request(uint8_t slave_addr, uint16_t start_addr, uint16_t count,
                                                       const std::function<void(const modbus_frame_t&)>& callback,
                                                       uint32_t timeout_ms) {
    modbus_frame_t request = read_holding_registers_request(slave_addr, start_addr, count);
    send_request(request, callback, timeout_ms);
    free_frame(request);
}

void ModbusMaster::send_read_input_registers_request(uint8_t slave_addr, uint16_t start_addr, uint16_t count,
                                                     const std::function<void(const modbus_frame_t&)>& callback,
                                                     uint32_t timeout_ms) {
    modbus_frame_t request = read_input_registers_request(slave_addr, start_addr, count);
    send_request(request, callback, timeout_ms);
    free_frame(request);
}

void ModbusMaster::send_write_single_register_request(uint8_t slave_addr, uint16_t reg_addr, uint16_t value,
                                                      const std::function<void(const modbus_frame_t&)>& callback,
                                                      uint32_t timeout_ms) {
    modbus_frame_t request = write_single_register_request(slave_addr, reg_addr, value);
    send_request(request, callback, timeout_ms);
    free_frame(request);
}

void ModbusMaster::send_write_single_coil_request(uint8_t slave_addr, uint16_t coil_addr, bool value,
                                                  const std::function<void(const modbus_frame_t&)>& callback,
                                                  uint32_t timeout_ms) {
    modbus_frame_t request = write_single_coil_request(slave_addr, coil_addr, value);
    send_request(request, callback, timeout_ms);
    free_frame(request);
}

void ModbusMaster::send_read_coils_request(uint8_t slave_addr, uint16_t start_addr, uint16_t count,
                                           const std::function<void(const modbus_frame_t&)>& callback,
                                           uint32_t timeout_ms) {
    modbus_frame_t request = read_coils_request(slave_addr, start_addr, count);
    send_request(request, callback, timeout_ms);
    free_frame(request);
}

void ModbusMaster::send_read_discrete_inputs_request(uint8_t slave_addr, uint16_t start_addr, uint16_t count,
                                                     const std::function<void(const modbus_frame_t&)>& callback,
                                                     uint32_t timeout_ms) {
    modbus_frame_t request = read_discrete_inputs_request(slave_addr, start_addr, count);
    send_request(request, callback, timeout_ms);
    free_frame(request);
}

void ModbusMaster::send_read_single_coil_request(uint8_t slave_addr, uint16_t coil_addr,
                                                 const std::function<void(const modbus_frame_t&)>& callback,
                                                 uint32_t timeout_ms) {
    send_read_coils_request(slave_addr, coil_addr, 1, callback, timeout_ms);
}

void ModbusMaster::send_read_single_discrete_input_request(uint8_t slave_addr, uint16_t input_addr,
                                                           const std::function<void(const modbus_frame_t&)>& callback,
                                                           uint32_t timeout_ms) {
    send_read_discrete_inputs_request(slave_addr, input_addr, 1, callback, timeout_ms);
}

void ModbusMaster::send_read_single_holding_register_request(uint8_t slave_addr, uint16_t reg_addr,
                                                             const std::function<void(const modbus_frame_t&)>& callback,
                                                             uint32_t timeout_ms) {
    send_read_holding_registers_request(slave_addr, reg_addr, 1, callback, timeout_ms);
}

void ModbusMaster::send_read_single_input_register_request(uint8_t slave_addr, uint16_t reg_addr,
                                                           const std::function<void(const modbus_frame_t&)>& callback,
                                                           uint32_t timeout_ms) {
    send_read_input_registers_request(slave_addr, reg_addr, 1, callback, timeout_ms);
}

void ModbusMaster::send_write_multiple_coils_request(uint8_t slave_addr, uint16_t start_addr, uint16_t count, const uint8_t* values,
                                                     const std::function<void(const modbus_frame_t&)>& callback,
                                                     uint32_t timeout_ms) {
    modbus_frame_t request = write_multiple_coils_request(slave_addr, start_addr, count, values);
    send_request(request, callback, timeout_ms);
    free_frame(request);
}

void ModbusMaster::send_write_multiple_registers_request(uint8_t slave_addr, uint16_t start_addr, uint16_t count, const uint16_t* values,
                                                         const std::function<void(const modbus_frame_t&)>& callback,
                                                         uint32_t timeout_ms) {
    modbus_frame_t request = write_multiple_registers_request(slave_addr, start_addr, count, values);
    send_request(request, callback, timeout_ms);
    free_frame(request);
}

bool ModbusMaster::is_request_pending() {
    mutex_enter_blocking(&request_mutex);
    bool pending = (pending_request != nullptr);
    mutex_exit(&request_mutex);
    return pending;
}
