#ifndef PICO_PLC_MD_BASE_H
#define PICO_PLC_MD_BASE_H

#include <memory>
#include <functional>

#include "md_stream.h"
#include "md_common.h"

class ModbusBase {
private:
    std::unique_ptr<ModbusStream> stream;

    std::function<void(const uint8_t&, const modbus_frame_t&)> debug_callback;
    std::function<void(const uint8_t&, const modbus_frame_t&)> error_callback;
    std::function<void(const uint8_t&, const modbus_frame_t&)> message_callback;

    struct {
        uint16_t BUS_MESSAGE_COUNT = 0;
        uint16_t BUS_COMMUNICATION_ERROR_COUNT = 0;
        uint16_t SLAVE_EXCEPTION_ERROR_COUNT = 0;
        uint16_t SLAVE_MESSAGE_COUNT = 0;
        uint16_t SLAVE_NO_RESPONSE_COUNT = 0;
        uint16_t SLAVE_NAK_COUNT = 0;
        uint16_t SLAVE_BUSY_COUNT = 0;
        uint16_t BUS_CHARACTER_OVERRUN_COUNT = 0;
    } diagnostic_counters;

    void process_diagnostic();
    void process_errors();

public:
    // bitmask for function codes enum?
    // probably a bad idea
    // void supported_functions()

    static void write(modbus_frame_t* frame);
    void read(modbus_frame_t* frame, bool timeout) const;

    void on_debug(const std::function<void(const uint8_t&, const modbus_frame_t&)>& callback);
    void on_error(const std::function<void(const uint8_t&, const modbus_frame_t&)>& callback);
    void on_message(const std::function<void(const uint8_t &, const modbus_frame_t &)> &callback);
};

#endif //PICO_PLC_MD_BASE_H