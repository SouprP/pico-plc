#include <stdio.h>
#include "pico/stdlib.h"

// GPIO pins for optocouplers
const uint OPT_GPIO[] = {5, 6, 7, 8};  // Actually 4 pins (0,1,2,3 indices)
const uint NUM_OPTO = sizeof(OPT_GPIO) / sizeof(OPT_GPIO[0]);

// Initialize optocoupler inputs
// Change to PULL DOWN
void opto_init() {
    for (uint i = 0; i < NUM_OPTO; i++) {
        gpio_init(OPT_GPIO[i]);
        gpio_set_dir(OPT_GPIO[i], GPIO_IN);
        gpio_pull_down(OPT_GPIO[i]);  // PULL DOWN, not UP
    }
}

// Read all optocouplers
void read_opto_states(bool states[]) {
    for (uint i = 0; i < NUM_OPTO; i++) {
        states[i] = gpio_get(OPT_GPIO[i]);
    }
}

// Example usage
int main() {
    stdio_init_all();
    opto_init();

    bool opto_states[NUM_OPTO];

    while (true) {
        read_opto_states(opto_states);

        for (uint i = 0; i < NUM_OPTO; i++) {
            printf("Opto %d: %s\n", i, opto_states[i] ? "HIGH" : "LOW");
        }

        sleep_ms(100);
    }
}