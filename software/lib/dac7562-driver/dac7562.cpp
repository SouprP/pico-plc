#include "dac7562.h"
#include "hardware/gpio.h"
#include <algorithm>
#include <pico/stdlib.h>

DAC7562::DAC7562(spi_inst_t* spi_instance,
                 uint cs_pin,
                 uint sck_pin,
                 uint mosi_pin,
                 uint ldac_pin,
                 uint clr_pin)
    : spi_(spi_instance),
      cs_pin_(cs_pin),
      sck_pin_(sck_pin),
      mosi_pin_(mosi_pin),
      ldac_pin_(ldac_pin),
      clr_pin_(clr_pin) {
}

void DAC7562::begin() {
    spi_init(spi_, 20 * 1000 * 1000);
    spi_set_format(spi_, 8, SPI_CPOL_0, SPI_CPHA_1, SPI_MSB_FIRST);

    gpio_set_function(sck_pin_, GPIO_FUNC_SPI);
    gpio_set_function(mosi_pin_, GPIO_FUNC_SPI);

    gpio_init(cs_pin_);
    gpio_set_dir(cs_pin_, GPIO_OUT);
    gpio_put(cs_pin_, 1);

    gpio_init(ldac_pin_);
    gpio_set_dir(ldac_pin_, GPIO_OUT);
    gpio_put(ldac_pin_, 1);

    gpio_init(clr_pin_);
    gpio_set_dir(clr_pin_, GPIO_OUT);
    gpio_put(clr_pin_, 1);

    // Włącz referencję 2.5V i ustaw gain=1
    send(DAC_CMD_REFERENCE, DAC_ADDR_A, 0x0001);
    send(DAC_CMD_WRITE_INPUT_REG, DAC_ADDR_GAIN, 0x0003); // Gain=1

    // Ustaw synchroniczną aktualizację (LDAC niepotrzebny)
    send(DAC_CMD_LDAC_REGISTER, DAC_ADDR_A, 0x0003);
    send(DAC_CMD_LDAC_REGISTER, DAC_ADDR_B, 0x0003);
}

void DAC7562::send(uint8_t cmd, uint8_t addr, uint16_t data) {
    uint32_t frame = (uint32_t(cmd) << 19) | (uint32_t(addr) << 16) | (data << 4);

    uint8_t tx[3] = {
        uint8_t(frame >> 16),
        uint8_t(frame >> 8),
        uint8_t(frame)
    };

    gpio_put(cs_pin_, 0);
    spi_write_blocking(spi_, tx, 3);
    gpio_put(cs_pin_, 1);
}

void DAC7562::setA(float normalized) {
    float voltage = normalized * 2.5f;

    if (voltage < 0) voltage = 0;
    if (voltage > 2.5) voltage = 2.5;
    uint16_t code = uint16_t((voltage / 2.5) * 4095);
    send(DAC_CMD_WRITE_UPDATE_N, DAC_ADDR_A, code);
}

void DAC7562::setB(float normalized) {
    float voltage = normalized * 2.5f;

    if (voltage < 0) voltage = 0;
    if (voltage > 2.5) voltage = 2.5;
    uint16_t code = uint16_t((voltage / 2.5) * 4095);
    send(DAC_CMD_WRITE_UPDATE_N, DAC_ADDR_B, code);
}

void DAC7562::setBoth(float normalized) {
    float voltage = normalized * 2.5f;

    if (voltage < 0) voltage = 0;
    if (voltage > 2.5) voltage = 2.5;
    uint16_t code = uint16_t((voltage / 2.5) * 4095);
    setA(voltage);
    setB(voltage);
}

void DAC7562::clear() {
    gpio_put(clr_pin_, 0);
    sleep_us(100);
    gpio_put(clr_pin_, 1);
}

void DAC7562::update() {
    gpio_put(ldac_pin_, 0);
    sleep_us(1);
    gpio_put(ldac_pin_, 1);
}