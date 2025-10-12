#include "md_base.h"

void ModbusBase::on_debug(const std::function<void(const uint8_t&, const modbus_frame_t&)>& callback) {
    debug_callback = callback;
}

void ModbusBase::on_error(const std::function<void(const uint8_t&, const modbus_frame_t&)>& callback) {
    error_callback = callback;
}

void ModbusBase::on_message(const std::function<void(const uint8_t&, const modbus_frame_t&)>& callback) {
    message_callback = callback;
}

