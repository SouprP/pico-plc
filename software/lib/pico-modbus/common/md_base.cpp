#include "md_base.h"

// #include "hardware/i"
// #include "pico/stdlib.h"
#include "pico/rand.h"

void ModbusBase::on_debug(const std::function<void(const uint8_t&, const modbus_frame_t&)>& callback) {
    debug_callback = callback;
}

void ModbusBase::on_error(const std::function<void(const uint8_t&, const modbus_frame_t&)>& callback) {
    error_callback = callback;
}

void ModbusBase::on_message(const std::function<void(const uint8_t&, const modbus_frame_t&)>& callback) {
    message_callback = callback;
}

void ModbusBase::read(modbus_frame_t *frame, bool timeout) const {
    // auto time = get_absolute_time();

    // get_rand_32() % 100;
    // uint8_t data = get_rand_32() % 100;
    // message_callback(data, *frame);
}

void ModbusBase::write(modbus_frame_t* frame) {

}


