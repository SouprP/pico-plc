#include "md_slave.h"
#include "common/md_common.h"
#include <cassert>
#include <cstring>
#include <cstdio>

ModbusSlave::ModbusSlave(uint8_t address, uart_inst_t* uart, uint baudrate)
    : ModbusBase(uart, baudrate),
      device_address(address),
      holding_registers_enabled(false),
      input_registers_enabled(false),
      coils_enabled(false),
      discrete_inputs_enabled(false),
      auto_sync_gpio(false),
      holding_registers(nullptr),
      holding_registers_size(0),
      input_registers(nullptr),
      input_registers_size(0) {
    // Slave must have address 1-247 (0=broadcast)
    assert(address >= 1 && address <= 247 && "Slave address must be 1-247");
}

void ModbusSlave::handle_received_frame(const modbus_frame_t& frame) {
    // Call debug callback if set
    invoke_debug_callback(frame);
    
    // Only process frames addressed to us or broadcast (address 0)
    if (frame.address != get_address() && frame.address != 0) {
        return;  // Not for us
    }
    
    bool is_broadcast = (frame.address == 0);
    uint8_t func = frame.function_code;
    
    // Parse common request fields
    uint16_t start_addr = 0;
    uint16_t count = 0;
    if (frame.data_length >= 4) {
        start_addr = (frame.data[0] << 8) | frame.data[1];
        count = (frame.data[2] << 8) | frame.data[3];
    }
    
    // Handle function codes
    switch (func) {
        case enum_value(ModbusFunctionCode::READ_COILS):
            handle_read_coils(frame, start_addr, count);
            break;
            
        case enum_value(ModbusFunctionCode::READ_DISCRETE_INPUTS):
            handle_read_discrete_inputs(frame, start_addr, count);
            break;
            
        case enum_value(ModbusFunctionCode::READ_HOLDING_REGISTERS):
            handle_read_holding_registers(frame, start_addr, count);
            break;
            
        case enum_value(ModbusFunctionCode::READ_INPUT_REGISTERS):
            handle_read_input_registers(frame, start_addr, count);
            break;
            
        case enum_value(ModbusFunctionCode::WRITE_SINGLE_COIL):
            handle_write_single_coil(frame, start_addr);
            break;
            
        case enum_value(ModbusFunctionCode::WRITE_SINGLE_REGISTER):
            handle_write_single_register(frame, start_addr);
            break;
            
        case enum_value(ModbusFunctionCode::WRITE_MULTIPLE_COILS):
            handle_write_multiple_coils(frame, start_addr, count);
            break;
            
        case enum_value(ModbusFunctionCode::WRITE_MULTIPLE_REGISTERS):
            handle_write_multiple_registers(frame, start_addr, count);
            break;
            
        case enum_value(ModbusFunctionCode::READ_DIAGNOSTICS):
            handle_read_diagnostics(frame);
            break;
            
        default:
            // Unsupported function code
            if (!is_broadcast) {
                send_exception(func, ModbusExceptionCode::ILLEGAL_FUNCTION);
            } else {
                // Broadcast - no response, but count as no-response
                diagnostic_counters.SLAVE_NO_RESPONSE_COUNT++;
            }
            break;
    }
    
    // Increment message counter after successful processing (per spec)
    diagnostic_counters.SLAVE_MESSAGE_COUNT++;
    
    // Call application message callback
    invoke_message_callback(frame);
}

void ModbusSlave::send_reply(const modbus_frame_t& frame) {
    // Don't send replies to broadcast messages (address 0)
    if (frame.address == 0) {
        diagnostic_counters.SLAVE_NO_RESPONSE_COUNT++;
        return;
    }
    queue_write(frame);
    process_tx_queue();
}

void ModbusSlave::send_exception(uint8_t function_code, ModbusExceptionCode exception_code) {
    diagnostic_counters.SLAVE_EXCEPTION_ERROR_COUNT++;
    send_reply(exception_response(get_address(), function_code, exception_code));
}

// Enable register types
void ModbusSlave::enable_holding_registers(uint16_t size) {
    holding_registers = std::make_unique<uint16_t[]>(size);
    holding_registers_size = size;
    holding_registers_enabled = true;
    std::memset(holding_registers.get(), 0, size * sizeof(uint16_t));
}

void ModbusSlave::enable_input_registers(uint16_t size) {
    input_registers = std::make_unique<uint16_t[]>(size);
    input_registers_size = size;
    input_registers_enabled = true;
    std::memset(input_registers.get(), 0, size * sizeof(uint16_t));
}

void ModbusSlave::enable_coils(const std::map<uint16_t, bool>& initial_coils, bool auto_gpio) {
    coils = initial_coils;
    coils_enabled = true;
    auto_sync_gpio = auto_gpio;
    
    // Initialize GPIO pins if auto-sync enabled
    if (auto_sync_gpio) {
        for (const auto& pair : coils) {
            uint16_t gpio_num = pair.first;
            if (gpio_num < 30) {  // Valid GPIO range for RP2040/RP2350
                gpio_init(gpio_num);
                gpio_set_dir(gpio_num, GPIO_OUT);
                gpio_put(gpio_num, pair.second ? 1 : 0);
            }
        }
    }
}

void ModbusSlave::sync_gpio_outputs() {
    if (!auto_sync_gpio || !coils_enabled) return;
    
    for (const auto& pair : coils) {
        uint16_t gpio_num = pair.first;
        bool state = pair.second;
        if (gpio_num < 30) {  // Valid GPIO range
            gpio_put(gpio_num, state ? 1 : 0);
        }
    }
}

void ModbusSlave::update_gpio_outputs() {
    sync_gpio_outputs();
}

void ModbusSlave::enable_discrete_inputs(const std::map<uint16_t, bool>& initial_inputs) {
    discrete_inputs = initial_inputs;
    discrete_inputs_enabled = true;
}

// Holding register access
bool ModbusSlave::check_hregister_exist(uint16_t address, uint16_t count) {
    // Check if address range is valid (address + count must not overflow)
    if (address > holding_registers_size || count > holding_registers_size) {
        return false;
    }
    // Check if the range fits within the allocated size
    if (address + count > holding_registers_size) {
        return false;
    }
    return true;
}

bool ModbusSlave::set_holding_register(uint16_t address, uint16_t value) {
    if (!holding_registers_enabled || address >= holding_registers_size) {
        return false;
    }
    holding_registers[address] = value;
    return true;
}

bool ModbusSlave::get_holding_register(uint16_t address, uint16_t& value) const {
    if (!holding_registers_enabled || address >= holding_registers_size) {
        return false;
    }
    value = holding_registers[address];
    return true;
}

// Input register access
bool ModbusSlave::check_iregister_exist(uint16_t address, uint16_t count) {
    // Check if address range is valid
    if (address > input_registers_size || count > input_registers_size) {
        return false;
    }
    // Check if the range fits within the allocated size
    if (address + count > input_registers_size) {
        return false;
    }
    return true;
}

bool ModbusSlave::set_input_register(uint16_t address, uint16_t value) {
    if (address >= input_registers_size) {
        return false;
    }
    input_registers[address] = value;
    return true;
}

bool ModbusSlave::get_input_register(uint16_t address, uint16_t& value) const {
    if (!input_registers_enabled || address >= input_registers_size) {
        return false;
    }
    value = input_registers[address];
    return true;
}

// Coil access
bool ModbusSlave::check_coils_exist(uint16_t starting_address, uint16_t count) const {
    // Check if all coil addresses in the range exist in the map
    for (uint16_t i = 0; i < count; i++) {
        if (coils.find(starting_address + i) == coils.end()) {
            return false;
        }
    }
    return true;
}

bool ModbusSlave::set_coil(uint16_t address, bool value) {
    if (!coils_enabled || coils.find(address) == coils.end()) {
        return false;
    }
    coils[address] = value;
    return true;
}

bool ModbusSlave::get_coil(uint16_t address, bool& value) const {
    auto it = coils.find(address);
    if (!coils_enabled || it == coils.end()) {
        return false;
    }
    value = it->second;
    return true;
}

// Discrete input access
bool ModbusSlave::check_discrete_exist(uint16_t address, uint16_t count) {
    // Check if all discrete input addresses in the range exist in the map
    for (uint16_t i = 0; i < count; i++) {
        if (discrete_inputs.find(address + i) == discrete_inputs.end()) {
            return false;
        }
    }
    return true;
}

bool ModbusSlave::set_discrete_input(uint16_t address, bool value) {
    if (discrete_inputs.find(address) == discrete_inputs.end()) {
        return false;
    }
    discrete_inputs[address] = value;
    return true;
}

bool ModbusSlave::get_discrete_input(uint16_t address, bool& value) const {
    auto it = discrete_inputs.find(address);
    if (!discrete_inputs_enabled || it == discrete_inputs.end()) {
        return false;
    }
    value = it->second;
    return true;
}

// ===== FUNCTION CODE HANDLERS =====

void ModbusSlave::handle_read_coils(const modbus_frame_t& frame, uint16_t start_addr, uint16_t count) {
    // Check if function is supported
    if (!is_coils_enabled()) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_FUNCTION);
        return;
    }
    
    // Validate count
    if (count == 0 || count > 2000) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_DATA_VALUE);
        return;
    }
    
    // Check if addresses exist
    if (!check_coils_exist(start_addr, count)) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_DATA_ADDRESS);
        return;
    }
    
    // Build response
    uint8_t byte_count = (count + 7) / 8;
    uint8_t* coil_bytes = new uint8_t[byte_count];
    memset(coil_bytes, 0, byte_count);
    
    for (uint16_t i = 0; i < count; i++) {
        bool value;
        if (get_coil(start_addr + i, value) && value) {
            coil_bytes[i / 8] |= (1 << (i % 8));
        }
    }
    
    modbus_frame_t response = read_coils_response(get_address(), count, coil_bytes);
    send_reply(response);
    free_frame(response);
    delete[] coil_bytes;
}

void ModbusSlave::handle_read_discrete_inputs(const modbus_frame_t& frame, uint16_t start_addr, uint16_t count) {
    if (!is_discrete_inputs_enabled()) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_FUNCTION);
        return;
    }
    
    if (count == 0 || count > 2000) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_DATA_VALUE);
        return;
    }
    
    if (!check_discrete_exist(start_addr, count)) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_DATA_ADDRESS);
        return;
    }
    
    uint8_t byte_count = (count + 7) / 8;
    uint8_t* input_bytes = new uint8_t[byte_count];
    memset(input_bytes, 0, byte_count);
    
    for (uint16_t i = 0; i < count; i++) {
        bool value;
        if (get_discrete_input(start_addr + i, value) && value) {
            input_bytes[i / 8] |= (1 << (i % 8));
        }
    }
    
    modbus_frame_t response = read_discrete_inputs_response(get_address(), count, input_bytes);
    send_reply(response);
    free_frame(response);
    delete[] input_bytes;
}

void ModbusSlave::handle_read_holding_registers(const modbus_frame_t& frame, uint16_t start_addr, uint16_t count) {
    if (!is_holding_registers_enabled()) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_FUNCTION);
        return;
    }
    
    if (count == 0 || count > 125) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_DATA_VALUE);
        return;
    }
    
    if (!check_hregister_exist(start_addr, count)) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_DATA_ADDRESS);
        return;
    }
    
    uint16_t* values = new uint16_t[count];
    for (uint16_t i = 0; i < count; i++) {
        get_holding_register(start_addr + i, values[i]);
    }
    
    modbus_frame_t response = read_registers_response(get_address(), 0x03, count, values);
    send_reply(response);
    free_frame(response);
    delete[] values;
}

void ModbusSlave::handle_read_input_registers(const modbus_frame_t& frame, uint16_t start_addr, uint16_t count) {
    if (!is_input_registers_enabled()) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_FUNCTION);
        return;
    }
    
    if (count == 0 || count > 125) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_DATA_VALUE);
        return;
    }
    
    if (!check_iregister_exist(start_addr, count)) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_DATA_ADDRESS);
        return;
    }
    
    uint16_t* values = new uint16_t[count];
    for (uint16_t i = 0; i < count; i++) {
        get_input_register(start_addr + i, values[i]);
    }
    
    modbus_frame_t response = read_registers_response(get_address(), 0x04, count, values);
    send_reply(response);
    free_frame(response);
    delete[] values;
}

void ModbusSlave::handle_write_single_coil(const modbus_frame_t& frame, uint16_t coil_addr) {
    if (!is_coils_enabled()) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_FUNCTION);
        return;
    }
    
    // Parse value (must be 0xFF00 or 0x0000)
    uint16_t value_word = (frame.data[2] << 8) | frame.data[3];
    if (value_word != 0xFF00 && value_word != 0x0000) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_DATA_VALUE);
        return;
    }
    
    bool value = (value_word == 0xFF00);
    
    if (!set_coil(coil_addr, value)) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_DATA_ADDRESS);
        return;
    }
    
    // Sync GPIO if auto-sync enabled
    sync_gpio_outputs();
    
    // Echo back request
    modbus_frame_t response = write_single_coil_response(get_address(), coil_addr, value);
    send_reply(response);
    free_frame(response);
}

void ModbusSlave::handle_write_single_register(const modbus_frame_t& frame, uint16_t reg_addr) {
    if (!is_holding_registers_enabled()) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_FUNCTION);
        return;
    }
    
    uint16_t value = (frame.data[2] << 8) | frame.data[3];
    
    if (!set_holding_register(reg_addr, value)) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_DATA_ADDRESS);
        return;
    }
    
    // Echo back request
    modbus_frame_t response = write_single_register_response(get_address(), reg_addr, value);
    send_reply(response);
    free_frame(response);
}

void ModbusSlave::handle_read_coils(const modbus_frame_t& frame, uint16_t start_addr, uint16_t count) {
    if (!is_coils_enabled()) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_FUNCTION);
        return;
    }
    
    if (count == 0 || count > 1968) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_DATA_VALUE);
        return;
    }
    
    uint8_t byte_count = frame.data[4];
    uint8_t expected_byte_count = (count + 7) / 8;
    if (byte_count != expected_byte_count || frame.data_length < 5 + byte_count) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_DATA_VALUE);
        return;
    }
    
    if (!check_coils_exist(start_addr, count)) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_DATA_ADDRESS);
        return;
    }
    
    // Write coils
    for (uint16_t i = 0; i < count; i++) {
        bool value = (frame.data[5 + i / 8] >> (i % 8)) & 0x01;
        set_coil(start_addr + i, value);
    }
    
    // Sync GPIO if auto-sync enabled
    sync_gpio_outputs();
    
    modbus_frame_t response = write_multiple_coils_response(get_address(), start_addr, count);
    send_reply(response);
    free_frame(response);
}

void ModbusSlave::handle_write_multiple_registers(const modbus_frame_t& frame, uint16_t start_addr, uint16_t count) {
    if (!is_holding_registers_enabled()) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_FUNCTION);
        return;
    }
    
    if (count == 0 || count > 123) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_DATA_VALUE);
        return;
    }
    
    uint8_t byte_count = frame.data[4];
    if (byte_count != count * 2 || frame.data_length < 5 + byte_count) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_DATA_VALUE);
        return;
    }
    
    if (!check_hregister_exist(start_addr, count)) {
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_DATA_ADDRESS);
        return;
    }
    
    // Write registers
    for (uint16_t i = 0; i < count; i++) {
        uint16_t value = (frame.data[5 + i * 2] << 8) | frame.data[6 + i * 2];
        set_holding_register(start_addr + i, value);
    }
    
    modbus_frame_t response = write_multiple_registers_response(get_address(), start_addr, count);
    send_reply(response);
    free_frame(response);
}

void ModbusSlave::handle_read_diagnostics(const modbus_frame_t& frame) {
    if (frame.data_length < 4) {
        diagnostic_counters.SLAVE_EXCEPTION_ERROR_COUNT++;
        send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_DATA_VALUE);
        return;
    }
    
    uint16_t sub_function = (frame.data[0] << 8) | frame.data[1];
    uint16_t data_field = (frame.data[2] << 8) | frame.data[3];
    
    uint16_t response_data = 0;
    
    // Handle diagnostic sub-functions
    switch (sub_function) {
        case 0x0000: // Return Query Data (echo)
            response_data = data_field;
            break;
            
        case enum_value(ModbusDiagnosticCode::BUS_MESSAGE_COUNT):
            response_data = get_bus_message_count();
            printf("[DIAG READ] BUS_MESSAGE_COUNT = %u (direct: %u)\n", response_data, diagnostic_counters.BUS_MESSAGE_COUNT);
            break;
            
        case enum_value(ModbusDiagnosticCode::BUS_COMMUNICATION_ERROR_COUNT):
            response_data = get_bus_communication_error_count();
            break;
            
        case enum_value(ModbusDiagnosticCode::SLAVE_EXCEPTION_ERROR_COUNT):
            response_data = get_slave_exception_error_count();
            break;
            
        case enum_value(ModbusDiagnosticCode::SLAVE_MESSAGE_COUNT):
            response_data = get_slave_message_count();
            break;
            
        case enum_value(ModbusDiagnosticCode::SLAVE_NO_RESPONSE_COUNT):
            response_data = get_slave_no_response_count();
            break;
            
        case enum_value(ModbusDiagnosticCode::SLAVE_NAK_COUNT):
            response_data = get_slave_nak_count();
            break;
            
        case enum_value(ModbusDiagnosticCode::SLAVE_BUSY_COUNT):
            response_data = get_slave_busy_count();
            break;
            
        case enum_value(ModbusDiagnosticCode::BUS_CHARACTER_OVERRUN_COUNT):
            response_data = get_bus_character_overrun_count();
            break;
            
        default:
            // Unsupported sub-function
            diagnostic_counters.SLAVE_EXCEPTION_ERROR_COUNT++;
            send_exception(frame.function_code, ModbusExceptionCode::ILLEGAL_FUNCTION);
            return;
    }
    
    modbus_frame_t response = read_diagnostics_response(get_address(), sub_function, response_data);
    printf("[DIAG RESPONSE] Sending: sub_func=0x%04X, data=0x%04X (%u)\n", sub_function, response_data, response_data);
    send_reply(response);
    free_frame(response);
}

