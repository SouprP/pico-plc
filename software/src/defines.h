#ifndef PICO_PLC_DEFINES_H
#define PICO_PLC_DEFINES_H

#define MD_BAUDRATE 4800
#define MD_UART uart0

#define SLAVE_ADDRESS 2

#define RS485_DE_PIN -1  // Driver Enable (active HIGH) - set to -1 if not using RS485
#define RS485_RE_PIN -1  // Receiver Enable (active LOW) - set to -1 if not using RS485

#endif //PICO_PLC_DEFINES_H