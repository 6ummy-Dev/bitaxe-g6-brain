/*
 * G6 Brain Integration Example — v1.0.0-beta3 (Final)
 *
 * Recommended integration for Bitaxe ESP-Miner (Gamma 602+ / BM1370).
 */

#include "g6_brain.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "G6_BRAIN_EXAMPLE";

static G6BrainState brain;

/* Example helper: safe slew-rate limiting */
static void apply_with_slew(float target_f, float target_v, float current_f, float current_v)
{
    float new_f = current_f;
    float new_v = current_v;

    if (target_f > current_f) new_f = fminf(current_f + MAX_FREQ_STEP, target_f);
    else if (target_f < current_f) new_f = fmaxf(current_f - MAX_FREQ_STEP, target_f);

    if (target_v > current_v) new_v = fminf(current_v + MAX_VOLT_STEP, target_v);
    else if (target_v < current_v) new_v = fmaxf(current_v - MAX_VOLT_STEP, target_v);

    // TODO: call your ASIC driver here
    ESP_LOGI(TAG, "SLEW → f=%.1f MHz @ v=%.0f mV", new_f, new_v);
}

void g6_brain_example_task(void *arg)
{
    esp_err_t ret = g6_brain_init(&brain);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize G6 Brain: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    brain.control_mode = G6_MODE_RECOMMEND;   // safest starting point

    ESP_LOGI(TAG, "G6 Brain started — RECOMMEND mode");

    while (1) {
        // === READ REAL TELEMETRY FROM YOUR MINER ===
        float freq_mhz  = 650.0f;
        float volt_mv   = 1220.0f;
        float hashrate  = 118.0f;
        float power_w   = 16.2f;
        float temp_c    = 57.0f;
        float error_pct = 0.7f;
        uint32_t share_count = 50;

        ret = g6_brain_update(&brain, freq_mhz, volt_mv, hashrate,
                              power_w, temp_c, error_pct, share_count);

        float opt_f = 0.0f, opt_v = 0.0f;
        g6_brain_get_optimal(&brain, &opt_f, &opt_v, NULL);

        float quality = g6_brain_get_model_quality(&brain);

        ESP_LOGI(TAG, "Quality=%.2f | Mode=%d | Current: %.1f MHz @ %.0f mV | Recommended: %.1f MHz @ %.0f mV",
                 quality, brain.control_mode, freq_mhz, volt_mv, opt_f, opt_v);

        // Optional: safe application with slew limiting
        if (brain.control_mode == G6_MODE_AUTO && quality > 0.75f) {
            // apply_with_slew(opt_f, opt_v, freq_mhz, volt_mv);
        }

        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

/* ====================== TELEMETRY USAGE EXAMPLE ====================== */
/*
 * Lightweight snapshot of brain state.
 * Call after g6_brain_update(). Single-threaded only.
 */
static void example_telemetry_usage(G6BrainState *brain)
{
    G6BrainTelemetry tel = {0};
    g6_brain_get_telemetry(brain, &tel);

    ESP_LOGI("G6_TELEMETRY", "=== BRAIN SNAPSHOT ===");
    ESP_LOGI("G6_TELEMETRY", "Quality: %.3f", g6_brain_get_model_quality(brain));
    ESP_LOGI("G6_TELEMETRY", "Cov trace (hashrate): %.2e", tel.trace_P_hashrate);
    ESP_LOGI("G6_TELEMETRY", "Cov trace (power):    %.2e", tel.trace_P_power);
    ESP_LOGI("G6_TELEMETRY", "Last innovation:      %.4f TH/s", tel.last_innovation);
    ESP_LOGI("G6_TELEMETRY", "Efficiency mode:      %s",
             tel.efficiency_mode_active ? "ENABLED (J/TH)" : "DISABLED (hashrate max)");
    ESP_LOGI("G6_TELEMETRY", "Last recommended V:   %.1f mV", tel.last_recommended_voltage);

    if (tel.safety_status != G6_SAFETY_OK) {
        ESP_LOGW("G6_TELEMETRY", "Safety status: %d", tel.safety_status);
    }

    // Example: send to MQTT / serial / dashboard
    // mqtt_publish_telemetry(&tel);
}
