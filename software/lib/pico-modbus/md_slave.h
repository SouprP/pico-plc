#ifndef PICO_PLC_SLAVE_H
#define PICO_PLC_SLAVE_H

#include "common/md_base.h"
#include <memory>
#include <map>

class ModbusSlave : public ModbusBase {
private:
    uint8_t device_address;
    bool is_broadcast_request; // Flag to track if current request is broadcast
    
    // Register type enable flags
    bool coils_enabled;
    bool discrete_inputs_enabled;
    bool input_registers_enabled;
    bool holding_registers_enabled;
    
    // Auto-sync GPIO for coils (when enabled)
    bool auto_sync_gpio;
    
    // Holding Registers (R/W, 16-bit) - contiguous array
    std::unique_ptr<uint16_t[]> holding_registers;
    uint16_t holding_registers_size;
    
    // Input Registers (R, 16-bit) - contiguous array
    std::unique_ptr<uint16_t[]> input_registers;
    uint16_t input_registers_size;
    
    // Coils (R/W, 1-bit) - sparse map of address -> value
    std::map<uint16_t, bool> coils;
    
    // Discrete Inputs (R, 1-bit) - sparse map of address -> value
    std::map<uint16_t, bool> discrete_inputs;
    
    // Sync GPIO outputs with coil states
    void sync_gpio_outputs();
    
    // Handler methods for each function code
    void handle_read_coils(const modbus_frame_t& frame, uint16_t start_addr, uint16_t count);
    void handle_read_discrete_inputs(const modbus_frame_t& frame, uint16_t start_addr, uint16_t count);
    void handle_read_holding_registers(const modbus_frame_t& frame, uint16_t start_addr, uint16_t count);
    void handle_read_input_registers(const modbus_frame_t& frame, uint16_t start_addr, uint16_t count);
    void handle_write_single_coil(const modbus_frame_t& frame, uint16_t coil_addr);
    void handle_write_single_register(const modbus_frame_t& frame, uint16_t reg_addr);
    void handle_write_multiple_coils(const modbus_frame_t& frame, uint16_t start_addr, uint16_t count);
    void handle_write_multiple_registers(const modbus_frame_t& frame, uint16_t start_addr, uint16_t count);
    void handle_read_diagnostics(const modbus_frame_t& frame);

protected:
    void handle_received_frame(const modbus_frame_t& frame) override;
    
public:
    ModbusSlave(uint8_t address, uart_inst_t* uart, uint baudrate, int de_pin = -1, int re_pin = -1, ModbusParity parity = ModbusParity::EVEN);
    
    // Enable register types
    void enable_holding_registers(uint16_t size);
    void enable_input_registers(uint16_t size);
    void enable_coils(const std::map<uint16_t, bool>& initial_coils, bool auto_gpio = true);
    void enable_discrete_inputs(const std::map<uint16_t, bool>& initial_inputs);

    void update_gpio_outputs();

    uint16_t* get_holding_registers() { return holding_registers.get(); }
    uint16_t* get_input_registers() { return input_registers.get(); }
    std::map<uint16_t, bool>* get_coils() { return &coils; }
    std::map<uint16_t, bool>* get_discrete_inputs() { return &discrete_inputs; }

    bool is_holding_registers_enabled() const { return holding_registers_enabled; }
    bool is_input_registers_enabled() const { return input_registers_enabled; }
    bool is_coils_enabled() const { return coils_enabled; }
    bool is_discrete_inputs_enabled() const { return discrete_inputs_enabled; }
    
    // Holding register access (R/W by Modbus)
    bool check_hregister_exist(uint16_t address, uint16_t count);
    bool set_holding_register(uint16_t address, uint16_t value);
    bool get_holding_register(uint16_t address, uint16_t& value) const;
    
    // Input register access (R by Modbus, W by application)
    bool check_iregister_exist(uint16_t address, uint16_t count);
    bool set_input_register(uint16_t address, uint16_t value);  // Application updates
    bool get_input_register(uint16_t address, uint16_t& value) const;
    
    // Coil access (R/W by Modbus)
    bool check_coils_exist(uint16_t starting_address, uint16_t count) const;
    bool set_coil(uint16_t address, bool value);
    bool get_coil(uint16_t address, bool& value) const;
    
    // Discrete input access (R by Modbus, W by application)
    bool check_discrete_exist(uint16_t address, uint16_t count);
    bool set_discrete_input(uint16_t address, bool value);  // Application updates
    bool get_discrete_input(uint16_t address, bool& value) const;
    
    uint8_t get_address() const { return device_address; }
    
    void send_reply(const modbus_frame_t& frame);
    void send_exception(uint8_t function_code, ModbusExceptionCode exception_code);
};

#endif //PICO_PLC_SLAVE_H