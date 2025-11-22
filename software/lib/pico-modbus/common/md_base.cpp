#include "md_base.h"
#include <cstdlib>
#include <cstring>

ModbusBase::ModbusBase(uart_inst_t* uart, uint baudrate) : next_transaction_id(1) {
    // Initialize mutexes for thread-safe access
    mutex_init(&tx_queue_mutex);
    mutex_init(&transaction_mutex);
    
    // Create stream with automatic frame reception
    stream = std::make_unique<ModbusStream>(uart, baudrate);
    
    // Set up callback for received frames (called from IRQ!)
    stream->on_frame_received([this](const modbus_frame_t& frame) {
        this->handle_received_frame(frame);
    });
}

ModbusBase::~ModbusBase() {
    mutex_exit(&tx_queue_mutex);
    // mutex_deinit(&tx_queue_mutex);
}

void ModbusBase::handle_received_frame(const modbus_frame_t& frame) {
    // This is called from IRQ context - keep it fast!
    diagnostic_counters.BUS_MESSAGE_COUNT++;
    
    // Check if this matches a pending transaction
    mutex_enter_blocking(&transaction_mutex);
    
    bool matched = false;
    if (!pending_transactions.empty()) {
        modbus_transaction_t& trans = pending_transactions.front();
        
        // Match by address and function code
        if (trans.address == frame.address && trans.function_code == frame.function_code) {
            // Call the specific callback for this transaction
            if (trans.callback) {
                trans.callback(frame);
            }
            pending_transactions.pop();
            matched = true;
        }
    }
    
    mutex_exit(&transaction_mutex);
    
    // Also call general message callback if no specific match or if set
    if (!matched && message_callback) {
        message_callback(frame.address, frame);
    }
}

void ModbusBase::queue_write(const modbus_frame_t& frame) {
    // Thread-safe enqueue
    mutex_enter_blocking(&tx_queue_mutex);
    
    // Make a copy of the frame with allocated data
    modbus_frame_t frame_copy = frame;
    if (frame.data && frame.data_length > 0) {
        frame_copy.data = (uint8_t*)malloc(frame.data_length);
        memcpy(frame_copy.data, frame.data, frame.data_length);
    }
    
    tx_queue.push(frame_copy);
    mutex_exit(&tx_queue_mutex);
}

uint16_t ModbusBase::send_request(const modbus_frame_t& frame, 
                                   const std::function<void(const modbus_frame_t&)>& callback,
                                   uint32_t timeout_ms) {
    // Create transaction
    modbus_transaction_t trans;
    trans.transaction_id = next_transaction_id++;
    trans.address = frame.address;
    trans.function_code = frame.function_code;
    trans.callback = callback;
    trans.timestamp_us = time_us_64();
    trans.timeout_ms = timeout_ms;
    
    // Add to pending transactions
    mutex_enter_blocking(&transaction_mutex);
    pending_transactions.push(trans);
    mutex_exit(&transaction_mutex);
    
    // Queue the frame for sending
    queue_write(frame);
    
    return trans.transaction_id;
}

void ModbusBase::check_transaction_timeouts() {
    mutex_enter_blocking(&transaction_mutex);
    
    while (!pending_transactions.empty()) {
        modbus_transaction_t& trans = pending_transactions.front();
        uint64_t elapsed_us = time_us_64() - trans.timestamp_us;
        
        if (elapsed_us > (trans.timeout_ms * 1000)) {
            // Timeout - remove transaction and optionally notify
            // printf("[MODBUS] Transaction %d timed out\n", trans.transaction_id);
            diagnostic_counters.SLAVE_NO_RESPONSE_COUNT++;
            pending_transactions.pop();
        } else {
            // Transactions are in order, so if this one hasn't timed out, neither have the rest
            break;
        }
    }
    
    mutex_exit(&transaction_mutex);
}

void ModbusBase::process_tx_queue() {
    // First, check for incoming frames (T3.5 silence detection)
    stream->process_if_ready();
    
    // Check for transaction timeouts
    check_transaction_timeouts();
    
    // Then check if there are frames to send
    mutex_enter_blocking(&tx_queue_mutex);
    
    if (!tx_queue.empty()) {
        modbus_frame_t frame = tx_queue.front();
        tx_queue.pop();
        mutex_exit(&tx_queue_mutex);
        
        // Send the frame
        stream->write(&frame);
        diagnostic_counters.BUS_MESSAGE_COUNT++;
        
        // Free allocated data
        if (frame.data) {
            free(frame.data);
        }
    } else {
        mutex_exit(&tx_queue_mutex);
    }
}

void ModbusBase::on_debug(const std::function<void(const uint8_t&, const modbus_frame_t&)>& callback) {
    debug_callback = callback;
}

void ModbusBase::on_error(const std::function<void(const uint8_t&, const modbus_frame_t&)>& callback) {
    error_callback = callback;
}

void ModbusBase::on_message(const std::function<void(const uint8_t&, const modbus_frame_t&)>& callback) {
    message_callback = callback;
}


