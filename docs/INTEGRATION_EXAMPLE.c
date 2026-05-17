/*
 * INTEGRATION EXAMPLE - G6 Brain v1.0.0-beta2
 *
 * Simple integration example.
 * Place the logic inside your existing miner control loop.
 *
 * Recommended: Start in OBSERVE_ONLY or RECOMMEND mode.
 * Only move to AUTO after you have validated behavior.
 */

#include "g6_brain.h"
#include "esp_log.h"

static const char *TAG = "G6_INTEGRATION_EXAMPLE";

static G6BrainState brain;

void example_g6_brain_integration(void)
{
    // Initialize once (call after nvs_flash_init())
    esp_err_t ret = g6_brain_init(&brain);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize G6 Brain: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "G6 Brain initialized successfully (beta2)");

    while (1) {
        // === Read real telemetry from your miner ===
        // Replace these with actual values from your GlobalState / SYSTEM_MODULE
        float freq_mhz   = 650.0f;   // current frequency
        float volt_mv    = 1220.0f;  // measured core voltage
        float hashrate   = 115.0f;   // TH/s
        float power_w    = 16.0f;    // power consumption
        float temp_c     = 58.0f;    // ASIC temperature
        float error_pct  = 0.8f;     // hardware error rate

        // Feed telemetry into the brain
        ret = g6_brain_update(&brain, freq_mhz, volt_mv, hashrate, power_w, temp_c, error_pct);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "g6_brain_update returned: %s", esp_err_to_name(ret));
        }

        // Get recommended safe operating point
        float opt_f, opt_v;
        g6_brain_get_optimal(&brain, &opt_f, &opt_v, NULL);

        // === IMPORTANT ===
        // Do NOT blindly apply opt_f / opt_v in production.
        // - Start in OBSERVE_ONLY mode
        // - Add your own slew limiting and confirmation logic
        // - Only apply when you trust model_quality

        float quality = g6_brain_get_model_quality(&brain);
        ESP_LOGI(TAG, "Model quality: %.2f | Recommended: %.1f MHz @ %.0f mV",
                 quality, opt_f, opt_v);

        // Example: only log for now (safe starting point)
        // Later you can add:
        // if (quality > 0.75f && control_mode == G6_MODE_RECOMMEND) {
        //     // apply with slew limiting
        // }

        vTaskDelay(pdMS_TO_TICKS(30000)); // 30 seconds recommended
    }
}
