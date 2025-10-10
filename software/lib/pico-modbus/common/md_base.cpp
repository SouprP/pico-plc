#include "md_base.h"

void ModbusBase::on_debug(std::function<void(const uint8_t&, const modbus_frame_t&)>& callback) {
    debug_callback = std::move(callback);
}

void ModbusBase::on_error(std::function<void(const uint8_t&, const modbus_frame_t&)>& callback) {
    error_callback = std::move(callback);
}

void ModbusBase::on_message(std::function<void(const uint8_t&, const modbus_frame_t&)>& callback) {
    message_callback = std::move(callback);
}

