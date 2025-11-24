#include "custom_dac.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

void setup_pwm_dac(uint gpio, uint resolution_bits) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);

    // Calculate wrap value for desired resolution
    uint32_t wrap = (1 << resolution_bits) - 1;

    // Configure PWM
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.0f);  // No clock divider
    pwm_config_set_wrap(&config, wrap);
    pwm_init(slice_num, &config, true);

    // Start at 0
    pwm_set_gpio_level(gpio, 0);
}

void set_dac_value(uint gpio, uint16_t value) {
    pwm_set_gpio_level(gpio, value);
}