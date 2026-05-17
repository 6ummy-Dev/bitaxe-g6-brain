/*
 * G6 Brain Integration Example — v1.0.0-beta2
 *
 * This is the recommended integration example for the G6 Brain.
 *
 * Purpose:
 * - Show a clean, practical way to integrate the brain into ESP-Miner.
 * - Balance between simplicity and real-world usability.
 *
 * Recommendation:
 * - Start with G6_MODE_OBSERVE_ONLY or G6_MODE_RECOMMEND.
 * - Only move to G6_MODE_AUTO after you have monitored behavior for several hours/days.
 *
 * Place this logic in your main control task or create a dedicated brain task.
 */

#include "g6_brain.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "G6_BRAIN_EXAMPLE";

static G6BrainState brain;

/**
 * Call this function from your app_main() or from a dedicated task.
 * It runs forever and feeds telemetry into the G6 Brain every 30 seconds.
 */
void g6_brain_example_task(void *arg)
{
    // Initialize the brain (call this after nvs_flash_init())
    esp_err_t ret = g6_brain_init(&brain);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize G6 Brain: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "G6 Brain v1.0.0-beta2 initialized successfully");

    // Optional: Set control mode here if needed
    // brain.control_mode = G6_MODE_RECOMMEND;   // Recommended starting mode

    while (1) {
        // ============================================================
        // === READ TELEMETRY FROM YOUR MINER ===
        // Replace these with real values from your GlobalState / SYSTEM_MODULE
        // ============================================================
        float freq_mhz  = 650.0f;     // Current frequency
        float volt_mv   = 1220.0f;    // Measured core voltage
        float hashrate  = 118.0f;     // TH/s
        float power_w   = 16.2f;      // Power consumption in Watts
        float temp_c    = 57.0f;      // ASIC temperature
        float error_pct = 0.7f;       // Hardware error rate (%)

        // Feed data into the brain
        ret = g6_brain_update(&brain, freq_mhz, volt_mv, hashrate, power_w, temp_c, error_pct);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "g6_brain_update failed: %s", esp_err_to_name(ret));
        }

        // Get the recommended safe operating point
        float opt_f = 0.0f, opt_v = 0.0f;
        g6_brain_get_optimal(&brain, &opt_f, &opt_v, NULL);

        float quality = g6_brain_get_model_quality(&brain);

        ESP_LOGI(TAG,
                 "Quality=%.2f | Current: %.1f MHz @ %.0f mV | Recommended: %.1f MHz @ %.0f mV",
                 quality, freq_mhz, volt_mv, opt_f, opt_v);

        // ============================================================
        // === WHEN TO APPLY opt_f / opt_v ===
        //
        // DO NOT blindly apply the recommended values in production.
        //
        // Good practice:
        // - Start in OBSERVE_ONLY or RECOMMEND mode
        // - Add your own slew limiting when applying changes
        // - Only apply when model_quality is good (> 0.7)
        // - Consider adding confirmation / manual approval first
        //
        // Example (commented out):
        // if (quality > 0.75f && brain.control_mode == G6_MODE_RECOMMEND) {
        //     // Apply with slew limiting here
        //     // asic_set_frequency_with_slew((uint32_t)opt_f);
        //     // asic_set_voltage_with_slew((uint32_t)opt_v);
        // }
        // ============================================================

        // Run every 30 seconds (recommended)
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
