#ifndef PICO_PLC_COMMON_UTILS_H
#define PICO_PLC_COMMON_UTILS_H

const char* get_unique_id();

void led_gpio_init();
void led_gpio_set(bool state);



#endif