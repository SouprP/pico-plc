#ifndef CUSTOM_DAC_COMMON_UTILS_H
#define CUSTOM_DAC_COMMON_UTILS_H

#include "pico/stdlib.h"

void setup_pwm_dac(uint gpio, uint resolution_bits);
void set_dac_value(uint gpio, uint16_t value);


#endif