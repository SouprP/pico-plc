#ifndef PICO_PLC_ADC_UTILS_H
#define PICO_PLC_ADC_UTILS_H

#include "pico/stdlib.h"
#include "hardware/adc.h"

// Stałe kalibracyjne wynikające z dzielników napięcia
// VREF = 3.3V, Max Input = 3.0V
const uint16_t ADC_VAL_3V0 = 3723; // Wartość dla 3.0V (100% zakresu)
const uint16_t ADC_VAL_0V6 = 745;  // Wartość dla 0.6V (dolny próg 4mA)

// Inicjalizacja ADC
inline void adc_init_system() {
    adc_init();
}

// Konfiguracja pinu (GPIO 26-29)
inline void adc_init_pin(uint channel) {
    adc_gpio_init(26 + channel);
}

// Odczyt surowy (0-4095)
inline uint16_t adc_read_raw(uint channel) {
    adc_select_input(channel);
    return adc_read();
}

// Odczyt dla wejścia 0-10V (skalowane 0-3V -> 0.0-1.0)
inline float adc_read_voltage_norm(uint channel) {
    uint16_t raw = adc_read_raw(channel);
    // Zabezpieczenie przed wyjściem poza zakres kalibracyjny
    if (raw > ADC_VAL_3V0) return 1.0f;

    return (float)raw / (float)ADC_VAL_3V0;
}

// Odczyt dla pętli 4-20mA (skalowane 0.6-3V -> 0.0-1.0)
inline float adc_read_current_norm(uint channel) {
    uint16_t raw = adc_read_raw(channel);

    // Odcięcie wartości poniżej 4mA (martwa strefa / błąd)
    if (raw < ADC_VAL_0V6) return 0.0f;
    // Zabezpieczenie góry zakresu
    if (raw > ADC_VAL_3V0) return 1.0f;

    // Skalowanie z uwzględnieniem offsetu (tzw. żywe zero)
    return (float)(raw - ADC_VAL_0V6) / (float)(ADC_VAL_3V0 - ADC_VAL_0V6);
}

#endif