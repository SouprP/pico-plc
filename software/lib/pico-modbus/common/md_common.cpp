#include "md_common.h"

#include "md_stream.h"

/* calculate Modbus CRC-16 (polynomial 0xA001) */
uint16_t calculate_crc(const uint8_t* data, const size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}

bool check_crc(const modbus_frame_t* frame) {
    uint8_t buffer[MODBUS_MAX_FRAME_SIZE];
    uint16_t length = 0;
    
    buffer[length++] = frame->address;
    buffer[length++] = frame->function_code;
    
    if (frame->data && frame->data_length > 0) {
        for (uint8_t i = 0; i < frame->data_length; i++) {
            buffer[length++] = frame->data[i];
        }
    }
    
    // calculate CRC and compare
    uint16_t calculated_crc = calculate_crc(buffer, length);
    return (frame->crc == calculated_crc);
}

// Helper to build frame with calculated CRC
static modbus_frame_t build_frame(uint8_t address, uint8_t function_code, uint8_t* data, uint8_t data_length) {
    modbus_frame_t frame;
    frame.address = address;
    frame.function_code = function_code;
    frame.data = data;
    frame.data_length = data_length;
    
    // Calculate CRC
    uint8_t buffer[256];
    buffer[0] = address;
    buffer[1] = function_code;
    for (uint8_t i = 0; i < data_length; i++) {
        buffer[2 + i] = data[i];
    }
    frame.crc = calculate_crc(buffer, 2 + data_length);
    
    return frame;
}

// ===== REQUEST BUILDERS (Master -> Slave) =====

modbus_frame_t read_coils_request(uint8_t slave_addr, uint16_t start_addr, uint16_t count) {
    uint8_t* data = new uint8_t[4];
    data[0] = (start_addr >> 8) & 0xFF;
    data[1] = start_addr & 0xFF;
    data[2] = (count >> 8) & 0xFF;
    data[3] = count & 0xFF;
    return build_frame(slave_addr, enum_value(ModbusFunctionCode::READ_COILS), data, 4);
}

modbus_frame_t read_discrete_inputs_request(uint8_t slave_addr, uint16_t start_addr, uint16_t count) {
    uint8_t* data = new uint8_t[4];
    data[0] = (start_addr >> 8) & 0xFF;
    data[1] = start_addr & 0xFF;
    data[2] = (count >> 8) & 0xFF;
    data[3] = count & 0xFF;
    return build_frame(slave_addr, enum_value(ModbusFunctionCode::READ_DISCRETE_INPUTS), data, 4);
}

modbus_frame_t read_holding_registers_request(uint8_t slave_addr, uint16_t start_addr, uint16_t count) {
    uint8_t* data = new uint8_t[4];
    data[0] = (start_addr >> 8) & 0xFF;
    data[1] = start_addr & 0xFF;
    data[2] = (count >> 8) & 0xFF;
    data[3] = count & 0xFF;
    return build_frame(slave_addr, enum_value(ModbusFunctionCode::READ_HOLDING_REGISTERS), data, 4);
}

modbus_frame_t read_input_registers_request(uint8_t slave_addr, uint16_t start_addr, uint16_t count) {
    uint8_t* data = new uint8_t[4];
    data[0] = (start_addr >> 8) & 0xFF;
    data[1] = start_addr & 0xFF;
    data[2] = (count >> 8) & 0xFF;
    data[3] = count & 0xFF;
    return build_frame(slave_addr, enum_value(ModbusFunctionCode::READ_INPUT_REGISTERS), data, 4);
}

modbus_frame_t write_single_coil_request(uint8_t slave_addr, uint16_t coil_addr, bool value) {
    uint8_t* data = new uint8_t[4];
    data[0] = (coil_addr >> 8) & 0xFF;
    data[1] = coil_addr & 0xFF;
    data[2] = value ? 0xFF : 0x00;  // 0xFF00 = ON, 0x0000 = OFF
    data[3] = 0x00;
    return build_frame(slave_addr, enum_value(ModbusFunctionCode::WRITE_SINGLE_COIL), data, 4);
}

modbus_frame_t write_single_register_request(uint8_t slave_addr, uint16_t reg_addr, uint16_t value) {
    uint8_t* data = new uint8_t[4];
    data[0] = (reg_addr >> 8) & 0xFF;
    data[1] = reg_addr & 0xFF;
    data[2] = (value >> 8) & 0xFF;
    data[3] = value & 0xFF;
    return build_frame(slave_addr, enum_value(ModbusFunctionCode::WRITE_SINGLE_REGISTER), data, 4);
}

modbus_frame_t write_multiple_coils_request(uint8_t slave_addr, uint16_t start_addr, uint16_t count, const uint8_t* values) {
    uint8_t byte_count = (count + 7) / 8;  // Round up to nearest byte
    uint8_t* data = new uint8_t[5 + byte_count];
    data[0] = (start_addr >> 8) & 0xFF;
    data[1] = start_addr & 0xFF;
    data[2] = (count >> 8) & 0xFF;
    data[3] = count & 0xFF;
    data[4] = byte_count;
    for (uint8_t i = 0; i < byte_count; i++) {
        data[5 + i] = values[i];
    }
    return build_frame(slave_addr, enum_value(ModbusFunctionCode::WRITE_MULTIPLE_COILS), data, 5 + byte_count);
}

modbus_frame_t write_multiple_registers_request(uint8_t slave_addr, uint16_t start_addr, uint16_t count, const uint16_t* values) {
    uint8_t byte_count = count * 2;
    uint8_t* data = new uint8_t[5 + byte_count];
    data[0] = (start_addr >> 8) & 0xFF;
    data[1] = start_addr & 0xFF;
    data[2] = (count >> 8) & 0xFF;
    data[3] = count & 0xFF;
    data[4] = byte_count;
    for (uint16_t i = 0; i < count; i++) {
        data[5 + i * 2] = (values[i] >> 8) & 0xFF;
        data[6 + i * 2] = values[i] & 0xFF;
    }
    return build_frame(slave_addr, enum_value(ModbusFunctionCode::WRITE_MULTIPLE_REGISTERS), data, 5 + byte_count);
}

// ===== RESPONSE BUILDERS (Slave -> Master) =====

modbus_frame_t read_coils_response(uint8_t slave_addr, uint16_t count, const uint8_t* coil_bytes) {
    uint8_t byte_count = (count + 7) / 8;
    uint8_t* data = new uint8_t[1 + byte_count];
    data[0] = byte_count;
    for (uint8_t i = 0; i < byte_count; i++) {
        data[1 + i] = coil_bytes[i];
    }
    return build_frame(slave_addr, enum_value(ModbusFunctionCode::READ_COILS), data, 1 + byte_count);
}

modbus_frame_t read_discrete_inputs_response(uint8_t slave_addr, uint16_t count, const uint8_t* input_bytes) {
    uint8_t byte_count = (count + 7) / 8;
    uint8_t* data = new uint8_t[1 + byte_count];
    data[0] = byte_count;
    for (uint8_t i = 0; i < byte_count; i++) {
        data[1 + i] = input_bytes[i];
    }
    return build_frame(slave_addr, enum_value(ModbusFunctionCode::READ_DISCRETE_INPUTS), data, 1 + byte_count);
}

modbus_frame_t read_registers_response(uint8_t slave_addr, uint8_t function_code, uint16_t count, const uint16_t* values) {
    uint8_t byte_count = count * 2;
    uint8_t* data = new uint8_t[1 + byte_count];
    data[0] = byte_count;
    for (uint16_t i = 0; i < count; i++) {
        data[1 + i * 2] = (values[i] >> 8) & 0xFF;
        data[2 + i * 2] = values[i] & 0xFF;
    }
    return build_frame(slave_addr, function_code, data, 1 + byte_count);
}

modbus_frame_t write_single_coil_response(uint8_t slave_addr, uint16_t coil_addr, bool value) {
    // Echo back the request
    uint8_t* data = new uint8_t[4];
    data[0] = (coil_addr >> 8) & 0xFF;
    data[1] = coil_addr & 0xFF;
    data[2] = value ? 0xFF : 0x00;
    data[3] = 0x00;
    return build_frame(slave_addr, enum_value(ModbusFunctionCode::WRITE_SINGLE_COIL), data, 4);
}

modbus_frame_t write_single_register_response(uint8_t slave_addr, uint16_t reg_addr, uint16_t value) {
    // Echo back the request
    uint8_t* data = new uint8_t[4];
    data[0] = (reg_addr >> 8) & 0xFF;
    data[1] = reg_addr & 0xFF;
    data[2] = (value >> 8) & 0xFF;
    data[3] = value & 0xFF;
    return build_frame(slave_addr, enum_value(ModbusFunctionCode::WRITE_SINGLE_REGISTER), data, 4);
}

modbus_frame_t write_multiple_coils_response(uint8_t slave_addr, uint16_t start_addr, uint16_t count) {
    uint8_t* data = new uint8_t[4];
    data[0] = (start_addr >> 8) & 0xFF;
    data[1] = start_addr & 0xFF;
    data[2] = (count >> 8) & 0xFF;
    data[3] = count & 0xFF;
    return build_frame(slave_addr, enum_value(ModbusFunctionCode::WRITE_MULTIPLE_COILS), data, 4);
}

modbus_frame_t write_multiple_registers_response(uint8_t slave_addr, uint16_t start_addr, uint16_t count) {
    uint8_t* data = new uint8_t[4];
    data[0] = (start_addr >> 8) & 0xFF;
    data[1] = start_addr & 0xFF;
    data[2] = (count >> 8) & 0xFF;
    data[3] = count & 0xFF;
    return build_frame(slave_addr, enum_value(ModbusFunctionCode::WRITE_MULTIPLE_REGISTERS), data, 4);
}

// ===== EXCEPTION RESPONSE =====

modbus_frame_t exception_response(uint8_t slave_addr, uint8_t function_code, ModbusExceptionCode exception_code) {
    uint8_t* data = new uint8_t[1];
    data[0] = enum_value(exception_code);
    return build_frame(slave_addr, function_code | 0x80, data, 1);
}

// ===== DIAGNOSTIC FUNCTIONS (0x08) =====

modbus_frame_t read_diagnostics_request(uint8_t slave_addr, uint16_t sub_function, uint16_t data) {
    uint8_t* payload = new uint8_t[4];
    payload[0] = (sub_function >> 8) & 0xFF;
    payload[1] = sub_function & 0xFF;
    payload[2] = (data >> 8) & 0xFF;
    payload[3] = data & 0xFF;
    return build_frame(slave_addr, enum_value(ModbusFunctionCode::READ_DIAGNOSTICS), payload, 4);
}

modbus_frame_t read_diagnostics_response(uint8_t slave_addr, uint16_t sub_function, uint16_t data) {
    uint8_t* payload = new uint8_t[4];
    payload[0] = (sub_function >> 8) & 0xFF;
    payload[1] = sub_function & 0xFF;
    payload[2] = (data >> 8) & 0xFF;
    payload[3] = data & 0xFF;
    return build_frame(slave_addr, enum_value(ModbusFunctionCode::READ_DIAGNOSTICS), payload, 4);
}

// ===== HELPER =====

void free_frame(modbus_frame_t& frame) {
    if (frame.data != nullptr) {
        delete[] frame.data;
        frame.data = nullptr;
    }
    frame.data_length = 0;
}