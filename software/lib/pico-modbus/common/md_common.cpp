#include "md_common.h"

/* Calculate Modbus CRC-16 (polynomial 0xA001) */
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
    // Build buffer with address + function + data
    uint8_t buffer[256];
    uint16_t length = 0;
    
    buffer[length++] = frame->address;
    buffer[length++] = frame->function_code;
    
    if (frame->data && frame->data_length > 0) {
        for (uint8_t i = 0; i < frame->data_length; i++) {
            buffer[length++] = frame->data[i];
        }
    }
    
    // Calculate CRC and compare
    uint16_t calculated_crc = calculate_crc(buffer, length);
    return (frame->crc == calculated_crc);
}