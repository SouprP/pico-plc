#include <cstdio>
#include <pico/stdio.h>
#include "pico/stdlib.h"

// #include "hardware"
#include "hardware/pwm.h"
#include "pico-utils/custom_dac.h"
#include "hardware/clocks.h"

#define PWM_PIN 0
#define PWM_RESOLUTION 12  // 12-bit resolution (0-4095)

void setup() {
    stdio_init_all();

    // gpio_init(PICO_DEFAULT_LED_PIN);
}

void loop() {

}

// int main() {
//     stdio_init_all();
//     // setup_pwm_dac(PWM_PIN, PWM_RESOLUTION);
//     //
//     // printf("PWM DAC Ramp Test Started\n");
//     // // wow
//     // printf("Resolution: %d bits (0-%d)\n", PWM_RESOLUTION, (1 << PWM_RESOLUTION) - 1);
//     //
//     // uint16_t max_value = (1 << PWM_RESOLUTION) - 1;
//     //
//     // while (true) {
//     //     // Ramp up from 0V to 3.3V over 3 seconds
//     //     printf("Ramping up: 0V -> 3.3V\n");
//     //     for (uint16_t i = 0; i <= max_value; i++) {
//     //         set_dac_value(PWM_PIN, i);
//     //         sleep_us(732);  // 3000ms / 4096 steps ≈ 732µs per step
//     //     }
//     //
//     //     // Hold at 3.3V for 3 seconds
//     //     printf("Holding at 3.3V\n");
//     //     sleep_ms(3000); //
//     //
//     //     // Ramp down from 3.3V to 0V over 3 seconds
//     //     printf("Ramping down: 3.3V -> 0V\n");
//     //     for (uint16_t i = max_value; i > 0; i--) {
//     //         set_dac_value(PWM_PIN, i);
//     //         sleep_us(732);
//     //     }
//     //     set_dac_value(PWM_PIN, 0);
//     //
//     //     // Small pause before repeating
//     //     sleep_ms(1000);
//     // }
//
//     while (true) {
//         printf("RP2350 Clock Frequencies:\n");
//         printf("System:     %u MHz\n", clock_get_hz(clk_sys) / 1000000);
//         printf("Peripheral: %u MHz\n", clock_get_hz(clk_peri) / 1000000);
//         printf("USB:        %u MHz\n", clock_get_hz(clk_usb) / 1000000);
//         printf("ADC:        %u MHz\n", clock_get_hz(clk_adc) / 1000000);
//         sleep_ms(6000);
//     }
//
//
//
//
//     return 0;
// }

void print_pwm_frequency(uint gpio) {
    uint slice = pwm_gpio_to_slice_num(gpio);

    // Get PWM configuration
    uint32_t wrap = pwm_hw->slice[slice].top + 1;  // Wrap value (top + 1)
    uint32_t div_int = pwm_hw->slice[slice].div >> PWM_CH0_DIV_INT_LSB;
    uint32_t div_frac = (pwm_hw->slice[slice].div >> PWM_CH0_DIV_FRAC_LSB) & 0xF;
    float clk_div = (float)div_int + ((float)div_frac / 16.0f);

    // Calculate frequency
    uint32_t sys_clk = clock_get_hz(clk_sys);
    float pwm_freq = (float)sys_clk / (clk_div * wrap);

    printf("PWM GPIO %d:\n", gpio);
    printf("  System Clock: %u Hz (%.2f MHz)\n", sys_clk, sys_clk / 1000000.0f);
    printf("  Clock Divider: %.4f\n", clk_div);
    printf("  Wrap Value: %u\n", wrap);
    printf("  PWM Frequency: %.2f Hz (%.2f kHz)\n", pwm_freq, pwm_freq / 1000.0f);
}

int main() {
    stdio_init_all();
    sleep_ms(2000);

    // Setup 12-bit PWM
    uint gpio = 0;
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, 4095);  // 12-bit
    pwm_config_set_clkdiv(&config, 1.0f);
    pwm_init(slice, &config, true);

    // Print frequency info
    print_pwm_frequency(gpio);

    while (true) {
        tight_loop_contents();
    }
}