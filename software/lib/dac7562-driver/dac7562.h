#ifndef PICO_PLC_DAC7562_H
#define PICO_PLC_DAC7562_H

#include "hardware/spi.h"

// Stałe komend
#define DAC_CMD_WRITE_INPUT_REG      0b000  // Zapisz do rejestru wejściowego
#define DAC_CMD_UPDATE_DAC_REG       0b001  // Aktualizuj rejestr DAC
#define DAC_CMD_WRITE_UPDATE_ALL     0b010  // Zapisz i aktualizuj wszystkie
#define DAC_CMD_WRITE_UPDATE_N       0b011  // Zapisz i aktualizuj wybrany
#define DAC_CMD_POWER_DOWN           0b100  // Tryb oszczędzania energii
#define DAC_CMD_SOFTWARE_RESET       0b101  // Reset programowy
#define DAC_CMD_LDAC_REGISTER        0b110  // Konfiguracja LDAC
#define DAC_CMD_REFERENCE            0b111  // Sterowanie referencją

// Stałe adresów
#define DAC_ADDR_A       0b000  // Kanał A
#define DAC_ADDR_B       0b001  // Kanał B
#define DAC_ADDR_GAIN    0b010  // Rejestr wzmocnienia
#define DAC_ADDR_ALL     0b111  // Oba kanały

class DAC7562 {
public:
    DAC7562(spi_inst_t* spi_instance,
           uint cs_pin,
           uint sck_pin,
           uint mosi_pin,
           uint ldac_pin,
           uint clr_pin);

    void begin();                   // Inicjalizacja
    void setA(float voltage);       // 0-2.5V na kanał A
    void setB(float voltage);       // 0-2.5V na kanał B
    void setBoth(float voltage);    // 0-2.5V na oba kanały
    void clear();                   // Zeruj przez CLR
    void update();                  // Aktualizuj przez LDAC

private:
    void send(uint8_t cmd, uint8_t addr, uint16_t data);

    spi_inst_t* spi_;
    uint cs_pin_, sck_pin_, mosi_pin_;
    uint ldac_pin_, clr_pin_;
};

#endif //PICO_PLC_DAC7562_H