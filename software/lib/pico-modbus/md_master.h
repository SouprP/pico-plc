#ifndef PICO_PLC_MASTER_H
#define PICO_PLC_MASTER_H

#include "common/md_base.h"

class ModbusMaster : public ModbusBase {
private:
    pending_request_t* pending_request;
    mutex_t request_mutex;
    bool request_sent;
    
    void check_request_timeouts();
    
protected:
    void handle_received_frame(const modbus_frame_t& frame) override;
    
public:
    ModbusMaster(uart_inst_t* uart, uint baudrate, int de_pin = -1, int re_pin = -1, ModbusParity parity = ModbusParity::EVEN);  // Master has no address
    ~ModbusMaster();
    
    // Send request with callback for response
    void send_request(const modbus_frame_t& frame, 
                      const std::function<void(const modbus_frame_t&)>& callback,
                      uint32_t timeout_ms = 5000);

    void send_diagnostic_request(uint8_t slave_addr, uint16_t sub_function, uint16_t data,
                                 const std::function<void(const modbus_frame_t&)>& callback,
                                 uint32_t timeout_ms = 5000);

    void send_read_holding_registers_request(uint8_t slave_addr, uint16_t start_addr, uint16_t count,
                                             const std::function<void(const modbus_frame_t&)>& callback,
                                             uint32_t timeout_ms = 5000);
    
    void send_read_input_registers_request(uint8_t slave_addr, uint16_t start_addr, uint16_t count,
                                           const std::function<void(const modbus_frame_t&)>& callback,
                                           uint32_t timeout_ms = 5000);

    void send_write_single_register_request(uint8_t slave_addr, uint16_t reg_addr, uint16_t value,
                                            const std::function<void(const modbus_frame_t&)>& callback,
                                            uint32_t timeout_ms = 5000);
    
    void send_write_single_coil_request(uint8_t slave_addr, uint16_t coil_addr, bool value,
                                        const std::function<void(const modbus_frame_t&)>& callback,
                                        uint32_t timeout_ms = 5000);
    
    void send_read_coils_request(uint8_t slave_addr, uint16_t start_addr, uint16_t count,
                                 const std::function<void(const modbus_frame_t&)>& callback,
                                 uint32_t timeout_ms = 5000);

    void send_read_discrete_inputs_request(uint8_t slave_addr, uint16_t start_addr, uint16_t count,
                                           const std::function<void(const modbus_frame_t&)>& callback,
                                           uint32_t timeout_ms = 5000);

    void send_read_single_coil_request(uint8_t slave_addr, uint16_t coil_addr,
                                       const std::function<void(const modbus_frame_t&)>& callback,
                                       uint32_t timeout_ms = 5000);

    void send_read_single_discrete_input_request(uint8_t slave_addr, uint16_t input_addr,
                                                 const std::function<void(const modbus_frame_t&)>& callback,
                                                 uint32_t timeout_ms = 5000);

    void send_read_single_holding_register_request(uint8_t slave_addr, uint16_t reg_addr,
                                                   const std::function<void(const modbus_frame_t&)>& callback,
                                                   uint32_t timeout_ms = 5000);

    void send_read_single_input_register_request(uint8_t slave_addr, uint16_t reg_addr,
                                                 const std::function<void(const modbus_frame_t&)>& callback,
                                                 uint32_t timeout_ms = 5000);

    void send_write_multiple_coils_request(uint8_t slave_addr, uint16_t start_addr, uint16_t count, const uint8_t* values,
                                           const std::function<void(const modbus_frame_t&)>& callback,
                                           uint32_t timeout_ms = 5000);

    void send_write_multiple_registers_request(uint8_t slave_addr, uint16_t start_addr, uint16_t count, const uint16_t* values,
                                               const std::function<void(const modbus_frame_t&)>& callback,
                                               uint32_t timeout_ms = 5000);

    void process_tx_queue();

    bool is_request_pending();
};


#endif //PICO_PLC_MASTER_H