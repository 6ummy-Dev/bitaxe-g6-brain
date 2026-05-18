/*
 * G6 Brain Integration Example — v1.0.0-beta2 (Phase 0 — fully wired)
 *
 * This is the recommended integration example for the G6 Brain.
 *
 * Phase 0 updates:
 * - Control modes are now enforced (default = RECOMMEND)
 * - NVS auto-save of theta + full P matrix every ~5 min
 * - Kconfig options are live
 * - Start in RECOMMEND or OBSERVE_ONLY for safety
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

    ESP_LOGI(TAG, "G6 Brain v1.0.0-beta2 initialized (Kconfig + control_mode + NVS auto-save)");

    // Phase 0: Choose your starting mode (RECOMMEND is safest for first runs)
    brain.control_mode = G6_MODE_RECOMMEND;   // ← Change to G6_MODE_AUTO only after monitoring

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

        // NOTE: share_count should come from your actual share counter in the current window.
        // Pass 0 if unknown — the brain still performs all other safety checks.
        uint32_t share_count = 0;

        // Feed data into the brain
        ret = g6_brain_update(&brain, freq_mhz, volt_mv, hashrate,
                              power_w, temp_c, error_pct, share_count);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "g6_brain_update failed: %s", esp_err_to_name(ret));
        }

        // Get the recommended safe operating point
        float opt_f = 0.0f, opt_v = 0.0f;
        g6_brain_get_optimal(&brain, &opt_f, &opt_v, NULL);

        float quality = g6_brain_get_model_quality(&brain);

        ESP_LOGI(TAG,
                 "Quality=%.2f | Mode=%d | Current: %.1f MHz @ %.0f mV | Recommended: %.1f MHz @ %.0f mV",
                 quality, brain.control_mode, freq_mhz, volt_mv, opt_f, opt_v);

        // ============================================================
        // === WHEN TO APPLY opt_f / opt_v ===
        //
        // DO NOT blindly apply the recommended values in production.
        //
        // Good practice (Phase 0):
        // - OBSERVE_ONLY: never apply
        // - RECOMMEND:   compute only (safe for monitoring)
        // - AUTO:        full optimizer (enable only after 24-48h soak)
        //
        // Example (uncomment when ready):
        // if (brain.control_mode == G6_MODE_AUTO && quality > 0.75f) {
        //     // Apply with your own slew limiting here
        //     // asic_set_frequency_with_slew((uint32_t)opt_f);
        //     // asic_set_voltage_with_slew((uint32_t)opt_v);
        // }
        // ============================================================

        // Run every 30 seconds (recommended)
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
