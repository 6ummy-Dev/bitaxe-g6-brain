/*
 * INTEGRATION EXAMPLE - G6 Brain v1.0 Beta (Final Polished Version)
 * Drop-in brain_task for AxeOS / ESP-Miner
 *
 * This is the recommended way to integrate the G6 Brain.
 */

#include "g6_brain.h"
#include "esp_log.h"

static const char *TAG = "G6_BRAIN_INTEGRATION";

static G6BrainState g6_brain;

static void brain_task(void *pvParameters)
{
    GlobalState *state = (GlobalState *)pvParameters;

    g6_brain_init(&g6_brain);
    ESP_LOGI(TAG, "G6 Brain initialized successfully");

    while (1) {
        /* === Extract real telemetry from AxeOS GlobalState === */
        float f_mhz   = (float)state->SYSTEM_MODULE.asic_freq;           // current frequency
        float v_mv    = (float)state->SYSTEM_MODULE.asic_voltage;        // actual measured voltage
        float hr_ths  = state->SYSTEM_MODULE.current_hashrate;           // hashrate in TH/s
        float power_w = state->SYSTEM_MODULE.power;                      // power in Watts
        float temp_c  = state->SYSTEM_MODULE.temp;                       // ASIC temp
        float err_pct = state->SYSTEM_MODULE.error_rate;                 // error rate

        /* Feed the brain */
        g6_brain_update(&g6_brain, f_mhz, v_mv, hr_ths, power_w, temp_c, err_pct);

        /* Get the recommended optimal settings */
        float opt_f, opt_v;
        g6_brain_get_optimal(&g6_brain, &opt_f, &opt_v, NULL);

        /* === Apply the new settings (you decide how) === */
        // Example: asic_set_frequency((uint32_t)opt_f);
        // Example: asic_set_voltage((uint32_t)opt_v);

        vTaskDelay(pdMS_TO_TICKS(30000));   // update every 30 seconds (recommended)
    }
}

/* Call this from app_main() after system init */
void start_g6_brain(GlobalState *state)
{
    xTaskCreate(brain_task, "g6_brain", 4096, state, 4, NULL);
}
