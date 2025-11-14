#ifndef PICO_PLC_MD_STREAM_H
#define PICO_PLC_MD_STREAM_H

#include "md_common.h"

class ModbusStream {
private:

    uint32_t silence_duration();

public:
    ModbusStream(uart_inst_t* uart);
    ~ModbusStream();

    void write(modbus_frame_t* frame);
    void await_read(modbus_frame_t* frame, bool timeout);
};

#endif //PICO_PLC_MD_STREAM_H