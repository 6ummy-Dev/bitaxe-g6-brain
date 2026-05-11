/*
 * INTEGRATION EXAMPLE - G6 Brain v1.0 Beta (Recommended Production Version)
 * Drop-in brain_task for AxeOS / ESP-Miner (Gamma 602+)
 *
 * This is the RECOMMENDED way to integrate the G6 Brain.
 *
 * Usage:
 *   1. Copy this file into your project (e.g. main/brain_integration.c)
 *   2. Call start_g6_brain(&gState) from app_main() AFTER WiFi + ASIC init
 *   3. Implement the actual asic_set_frequency() / asic_set_voltage() calls
 *      according to your firmware's slew limiting and confirmation logic.
 *
 * Safety Note:
 *   The brain returns RECOMMENDED values. You are responsible for applying
 *   them safely (slew rate, confirmation, thermal headroom, etc.).
 *   See docs/SAFETY.md and AGENTS.md for full safety invariants.
 */

#include "g6_brain.h"
#include "esp_log.h"

static const char *TAG = "G6_BRAIN_INTEGRATION";

static G6BrainState g6_brain;

static void brain_task(void *pvParameters)
{
    GlobalState *state = (GlobalState *)pvParameters;

    esp_err_t ret = g6_brain_init(&g6_brain);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "G6 Brain init failed: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);   // Abort task on failure
        return;
    }

    ESP_LOGI(TAG, "G6 Brain initialized successfully (v1.0 Beta)");

    while (1) {
        /* === Extract real telemetry from AxeOS GlobalState === */
        float f_mhz   = (float)state->SYSTEM_MODULE.asic_freq;     // MHz
        float v_mv    = (float)state->SYSTEM_MODULE.asic_voltage;  // mV (measured)
        float hr_ths  = state->SYSTEM_MODULE.current_hashrate;     // TH/s
        float power_w = state->SYSTEM_MODULE.power;                // Watts
        float temp_c  = state->SYSTEM_MODULE.temp;                 // °C
        float err_pct = state->SYSTEM_MODULE.error_rate;           // %

        /* Feed the brain */
        g6_brain_update(&g6_brain, f_mhz, v_mv, hr_ths, power_w, temp_c, err_pct);

        /* Get recommended safe operating point */
        float opt_f, opt_v;
        g6_brain_get_optimal(&g6_brain, &opt_f, &opt_v, NULL);

        /* === Apply settings (IMPLEMENT YOUR OWN SAFE APPLY LOGIC) === */
        // Example (add your slew limiting + confirmation here):
        // if (should_apply_new_settings(opt_f, opt_v)) {
        //     asic_set_frequency((uint32_t)opt_f);
        //     asic_set_voltage((uint32_t)opt_v);
        // }

        /* Recommended update interval: 20–30 seconds */
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

/**
 * @brief Start the G6 Brain background task.
 *
 * Call this once from app_main() after the GlobalState is valid.
 *
 * @param state Pointer to your GlobalState instance
 */
void start_g6_brain(GlobalState *state)
{
    xTaskCreate(
        brain_task,
        "g6_brain",
        4096,           // Stack size (adjust if needed)
        state,
        4,              // Priority (adjust to your needs)
        NULL
    );
}
