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

    adc_init_system();
    // DAC7562 dac(spi0,
    //             DAC_CS_PIN,
    //             DAC_SCK_PIN,
    //             DAC_MOSI_PIN,
    //             DAC_CLR_PIN,
    //             DAC_LDAC_PIN);
    //
    // dac.begin();
    // dac.enableInternalReference();
    //
    // while (true) {
    //     cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    //
    //     // 0V
    //     dac.clearOutputs(true);
    //     sleep_ms(2000);
    //
    //     // 1.25V
    //     dac.setBoth(2048, true);
    //     sleep_ms(2000);
    //
    //     // Channel A: 0.5V, Channel B: 2.0V
    //     dac.setVoltage(DAC7562::Channel::A, 0.5f, true);
    //     dac.setVoltage(DAC7562::Channel::B, 2.0f, true);
    //     sleep_ms(2000);
    //
    //     // Both: 2.5V
    //     dac.setBothVoltage(2.5f, true);
    //     sleep_ms(2000);
    //
    //     cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    //     sleep_ms(500);
    // }

    cyw43_arch_deinit();
    return 0;
}