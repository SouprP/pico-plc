#ifndef PICO_PLC_MASTER_H
#define PICO_PLC_MASTER_H

#include "common/md_base.h"

class ModbusMaster : public ModbusBase {
private:
    // Single pending request (only one at a time in Modbus RTU)
    pending_request_t* pending_request;
    mutex_t request_mutex;
    bool request_sent;
    
    void check_request_timeouts();
    
protected:
    void handle_received_frame(const modbus_frame_t& frame) override;
    
public:
    ModbusMaster(uart_inst_t* uart, uint baudrate);  // Master has no address
    ~ModbusMaster();
    
    // Send request with callback for response
    void send_request(const modbus_frame_t& frame, 
                      const std::function<void(const modbus_frame_t&)>& callback,
                      uint32_t timeout_ms = 5000);
    
    // Convenience method for diagnostic requests
    void send_diagnostic_request(uint8_t slave_addr, uint16_t sub_function, uint16_t data,
                                 const std::function<void(const modbus_frame_t&)>& callback,
                                 uint32_t timeout_ms = 5000);
    
    // Override process_tx_queue to add request/response sequencing
    void process_tx_queue();
};


#endif //PICO_PLC_MASTER_H