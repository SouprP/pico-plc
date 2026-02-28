#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "dac7562-driver/dac7562.h"

#include "pico-utils/analog_utils.h"

#define DAC_CS_PIN   17
#define DAC_SCK_PIN  18
#define DAC_MOSI_PIN 19
#define DAC_CLR_PIN  20
#define DAC_LDAC_PIN 21


int main() {
    cyw43_arch_init();

    DAC7562 dac(spi0,
                DAC_CS_PIN,
                DAC_SCK_PIN,
                DAC_MOSI_PIN,
                DAC_CLR_PIN,
                DAC_LDAC_PIN);

    dac.begin();
    dac.setA(2.5);

    cyw43_arch_deinit();
    return 0;
}