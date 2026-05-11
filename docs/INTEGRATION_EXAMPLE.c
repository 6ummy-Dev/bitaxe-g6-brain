/*
 * INTEGRATION EXAMPLE - G6 Brain v1.0 Beta
 * Place this in your main control loop
 */

#include "g6_brain.h"

static G6BrainState brain;

void app_main(void) {
    g6_brain_init(&brain);

    while (1) {
        // Read telemetry from miner
        float freq = ...;
        float volt = ...;
        float hr   = ...;
        float pwr  = ...;
        float temp = ...;
        float err  = ...;

        g6_brain_update(&brain, freq, volt, hr, pwr, temp, err);

        float opt_f, opt_v;
        g6_brain_get_optimal(&brain, &opt_f, &opt_v, NULL);

        // Apply opt_f / opt_v to ASIC
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
