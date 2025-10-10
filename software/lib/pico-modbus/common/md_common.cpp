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
    if(frame->crc == calculate_crc(frame->data, frame->data_length))
        return true;

    return false;
}