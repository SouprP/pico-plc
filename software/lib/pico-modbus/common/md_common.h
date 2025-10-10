#ifndef PICO_PLC_MD_COMMON_H
#define PICO_PLC_MD_COMMON_H

#include "pico/stdlib.h"
#include <type_traits>

typedef struct {
    uint8_t address;
    uint8_t function_code;
    uint8_t* data;
    uint8_t data_length;
    uint16_t crc;
} modbus_frame_t;

enum class BaudRate {
    BAUD_1200 = 1200,
    BAUD_2400 = 2400,
    BAUD_4800 = 4800,
    BAUD_9600 = 9600,
    BAUD_19200 = 19200,
    BAUD_38400 = 38400,
    BAUD_57600 = 57600,
    BAUD_115200 = 115200
};

/* Modbus function codes */
enum class ModbusFunctionCode : uint8_t {
    READ_COILS = 0x01,
    READ_DISCRETE_INPUTS = 0x02,
    READ_HOLDING_REGISTERS = 0x03,
    READ_INPUT_REGISTERS = 0x04,
    WRITE_SINGLE_COIL = 0x05,
    WRITE_SINGLE_REGISTER = 0x06,
    READ_DIAGNOSTICS = 0x08,
    WRITE_MULTIPLE_COILS = 0x0F,
    WRITE_MULTIPLE_REGISTERS = 0x10
};

/* Exception codes */
enum class ModbusExceptionCode : uint8_t {
    ILLEGAL_FUNCTION = 0x01,
    ILLEGAL_DATA_ADDRESS = 0x02,
    ILLEGAL_DATA_VALUE = 0x03,
    SLAVE_DEVICE_FAILURE = 0x04,
    ACKNOWLEDGE = 0x05,
    SLAVE_DEVICE_BUSY = 0x06,
    MEMORY_PARITY_ERROR = 0x08,
    GATEWAY_PATH_UNAVAILABLE = 0x0A,
    GATEWAY_TARGET_RESPOND_FAILED = 0x0B
};

/* Diagnostic codes */
enum class ModbusDiagnosticCode : uint16_t {
    RETURN_QUERY_DATA = 0x0000,
    RESTART_COMMUNICATIONS_OPTION = 0x0001,
    RETURN_DIAGNOSTIC_REGISTER = 0x0002,
    CHANGE_ASCII_INPUT_DELIMITER = 0x0003,
    FORCE_LISTEN_ONLY_MODE = 0x0004,
    CLEAR_COUNTERS_AND_DIAGNOSTIC_REGISTER = 0x000A,
    BUS_MESSAGE_COUNT = 0x000B,
    BUS_COMMUNICATION_ERROR_COUNT = 0x000C,
    SLAVE_EXCEPTION_ERROR_COUNT = 0x000D,
    SLAVE_MESSAGE_COUNT = 0x000E,
    SLAVE_NO_RESPONSE_COUNT = 0x000F,
    SLAVE_NAK_COUNT = 0x0010,
    SLAVE_BUSY_COUNT = 0x0011,
    BUS_CHARACTER_OVERRUN_COUNT = 0x0012,
    CLEAR_OVERRUN_COUNTER_AND_FLAG = 0x0014
};

template<typename T>
constexpr auto enum_value(T t){
    return static_cast<std::underlying_type_t<T>>(t);
    // return static_cast<typename std::underlying_type<T>::type>(t);
}

uint16_t calculate_crc(const uint8_t* data, size_t length);
bool check_crc(const modbus_frame_t* frame);

#endif //PICO_PLC_MD_COMMON_H