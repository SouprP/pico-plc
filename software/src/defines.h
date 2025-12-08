#ifndef PICO_PLC_DEFINES_H
#define PICO_PLC_DEFINES_H

#include "pico-modbus/common/md_common.h"
#define MD_UART uart0

constexpr ModbusParity parity = ModbusParity::NONE;
constexpr uint16_t MD_BAUDRATE = 4800;
constexpr uint8_t SLAVE_ADDRESS = 2;

#define RS485_DE_PIN -1  // Driver Enable (active HIGH) - set to -1 if not using RS485
#define RS485_RE_PIN -1  // Receiver Enable (active LOW) - set to -1 if not using RS485

#endif //PICO_PLC_DEFINES_H