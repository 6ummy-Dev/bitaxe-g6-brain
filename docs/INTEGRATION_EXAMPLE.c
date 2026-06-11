/*
 * G6 Brain Integration Example — v1.0.0-beta7.5
 * Bitaxe ESP-Miner (Gamma 602+ / BM1370)
 */

#include "g6_brain.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "G6_BRAIN_EXAMPLE";

static G6BrainState brain;

/* Integration state — window delta tracking */
static uint32_t prev_shares  = 0;
static uint32_t prev_errors  = 0;
static int64_t  prev_us      = 0;
static bool     droop_seeded = false;

#define G6_REF_POOL_DIFFICULTY   1000.0f
#define G6_MAX_DIFFICULTY_WEIGHT 3.0f

void g6_brain_example_task(void *arg)
{
    esp_err_t ret = g6_brain_init(&brain);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Brain init failed: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }

    brain.control_mode = G6_MODE_RECOMMEND;
    ESP_LOGI(TAG, "G6 Brain started — RECOMMEND mode");

    while (1) {

        /* === READ FROM YOUR TELEMETRY STRUCT === */
        float    freq_mhz         = 800.0f;
        float    volt_mv          = 1207.0f;  /* coreVoltageActual */
        float    volt_set_mv      = 1210.0f;  /* coreVoltage */
        /* AxeOS reports hashRate/hashRate_10m in GH/s; g6_brain_update() expects
         * TH/s. Convert on read so every downstream use (the brain update below
         * and the H/s conversion in the NER block) sees the correct unit. */
        float    hashrate_live    = 1639.8f / 1000.0f;  /* hashRate (GH/s) → TH/s */
        float    hashrate_10m     = 1637.1f / 1000.0f;  /* hashRate_10m (GH/s) → TH/s */
        float    power_w          = 24.12f;
        float    temp_c           = 58.75f;
        float    vr_temp_c        = 74.0f;    /* vrTemp */
        float    error_pct        = 0.681f;   /* errorPercentage — fallback only */
        uint32_t shares_accepted  = 1708;     /* sharesAccepted (cumulative) */
        uint32_t error_count_raw  = 28746;    /* hashrateMonitor.asics[0].errorCount */
        uint32_t pool_difficulty  = 2000;     /* poolDifficulty */

        int64_t now_us = esp_timer_get_time();

        /* P5 — Log droop coefficient once on boot */
        if (!droop_seeded && power_w > 1.0f) {
            ESP_LOGI(TAG, "VRM droop: %.4f mV/W", (volt_set_mv - volt_mv) / power_w);
            /* brain.droop_mv_per_watt = (volt_set_mv - volt_mv) / power_w; // Phase 2 */
            droop_seeded = true;
        }

        /* P3 — Use 10m avg during thermal transient, live once settled */
        float quality  = g6_brain_get_model_quality(&brain);
        float hr_input = (quality < 0.5f || brain.cold_start) ? hashrate_10m : hashrate_live;

        /* P4 — Differential NER from raw errorCount */
        if (prev_us > 0 && error_count_raw >= prev_errors) {
            float    dt_s   = (float)(now_us - prev_us) / 1e6f;
            float    hr_hs  = hashrate_10m * 1e12f;
            if (dt_s > 0.5f && hr_hs > 0.0f)
                error_pct = ((float)(error_count_raw - prev_errors) / (hr_hs * dt_s)) * 100.0f;
        }
        prev_errors = error_count_raw;
        prev_us     = now_us;

        /* P2 — Window delta shares   P6 — Difficulty-weighted */
        uint32_t window_shares  = (shares_accepted >= prev_shares)
                                  ? (shares_accepted - prev_shares) : shares_accepted;
        prev_shares = shares_accepted;

        float    diff_weight    = fminf(G6_MAX_DIFFICULTY_WEIGHT,
                                        (float)pool_difficulty / G6_REF_POOL_DIFFICULTY);
        uint32_t weighted_shares = (uint32_t)((float)window_shares * diff_weight);

        /* === BRAIN UPDATE === */
        ret = g6_brain_update(&brain,
                              freq_mhz, volt_mv, hr_input, power_w,
                              temp_c, vr_temp_c, error_pct, weighted_shares);

        float opt_f = 0.0f, opt_v = 0.0f;
        g6_brain_get_optimal(&brain, &opt_f, &opt_v, NULL);

        ESP_LOGI(TAG, "Q=%.2f | %s | NER=%.4f%% | shares=%u(x%.1f) | "
                      "cur %.0f/%.0f | rec %.0f/%.0f",
                 quality, brain.cold_start ? "10m" : "live",
                 error_pct, window_shares, diff_weight,
                 freq_mhz, volt_mv, opt_f, opt_v);

        if (brain.control_mode == G6_MODE_AUTO && quality > 0.75f) {
            /* asic_set_voltage((uint16_t)opt_v); */
            /* asic_set_frequency((uint16_t)opt_f); */
        }

        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

/* === TELEMETRY SNAPSHOT === */
static void example_telemetry_usage(G6BrainState *b)
{
    G6BrainTelemetry tel = {0};
    g6_brain_get_telemetry(b, &tel);
    ESP_LOGI("G6_TEL", "Q=%.3f | trace_hr=%.2e | trace_pw=%.2e | innov=%.4f | V=%.1f mV",
             g6_brain_get_model_quality(b),
             tel.trace_P_hashrate, tel.trace_P_power,
             tel.last_innovation, tel.last_recommended_voltage);
    if (tel.safety_status != G6_SAFETY_OK)
        ESP_LOGW("G6_TEL", "Safety status: %d", tel.safety_status);
}
