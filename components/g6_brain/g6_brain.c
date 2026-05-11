/*
 * g6_brain.c
 * Bitaxe G6 Brain — v1.0 Beta
 * RLS core + Stochastic Nonce Offsetting + low-latency job hook
 */

#include "g6_brain.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include <string.h>
#include <math.h>

static const char *TAG = "G6_BRAIN";

static const G6BrainInterface g6_brain_interface = {
    .init = g6_brain_init,
    .update = g6_brain_update,
    .get_optimal = g6_brain_get_optimal,
    .get_model_quality = g6_brain_get_model_quality,
    .get_full_telemetry = g6_brain_get_full_telemetry,
    .self_test = g6_brain_self_test
};

/* ====================== INTERNAL HELPERS ====================== */
static float normalize_f(float f_mhz) { return (f_mhz - 700.0f) / 200.0f; }
static float normalize_v(float v_mv) { return (v_mv - 1200.0f) / 150.0f; }

/* ====================== PUBLIC API ====================== */

esp_err_t g6_brain_init(G6BrainState *brain) {
    if (!brain) return ESP_ERR_INVALID_ARG;
    memset(brain, 0, sizeof(G6BrainState));

    brain->lambda = CONFIG_G6_RLS_LAMBDA;
    brain->ridge_epsilon = 1e-5f;
    brain->cold_start = true;
    brain->update_count = 0;
    brain->i2c_hard_fault_threshold = CONFIG_G6_I2C_HARD_FAULT_THRESHOLD;
    brain->nvs_write_interval = CONFIG_G6_NVS_WRITE_INTERVAL;
    brain->interface = &g6_brain_interface;

    // PID & safety defaults (preserved)
    brain->Kp = CONFIG_G6_KP; brain->Ki = CONFIG_G6_KI; brain->Kd = CONFIG_G6_KD;
    brain->temp_ceiling = CONFIG_G6_TEMP_CEILING;
    brain->dfs_step_mhz = CONFIG_G6_DFS_STEP;
    brain->ner_threshold = CONFIG_G6_NER_THRESHOLD;

    // NEW: Stochastic Nonce defaults
    brain->nonce_offset = 0;
    brain->enable_low_latency_jobs = CONFIG_G6_LOW_LATENCY_JOBS;

    brain->last_update_timestamp = xTaskGetTickCount();
    brain->nvs_last_write_tick = brain->last_update_timestamp;

    ESP_LOGI(TAG, "G6 Brain v1.0 Beta initialized — RLS + Stochastic Nonce");
    return ESP_OK;
}

esp_err_t g6_brain_update(...) { /* RLS core + QA safety exactly as before — unchanged */ 
    // ... (full RLS update, thermal scale, ripple check, non-blocking tune, NVS wear-leveling — identical to last version)
    // NEW: On every update we can refresh nonce if needed (miner usually calls get_stochastic_nonce_start on new job)
    return ESP_OK;
}

void g6_brain_get_optimal(...) { /* unchanged */ }
float g6_brain_get_model_quality(...) { /* unchanged */ }
void g6_brain_get_full_telemetry(...) { /* unchanged + nonce_offset in JSON */ }
bool g6_brain_self_test(...) { /* unchanged */ }

/* ====================== NEW STOCHASTIC NONCE ====================== */

uint32_t g6_brain_get_stochastic_nonce_start(G6BrainState *brain) {
    if (!brain) return 0;
    // Hardware RNG — true random start per job
    brain->nonce_offset = esp_random();
    ESP_LOGD(TAG, "Stochastic nonce offset set: 0x%08lX", brain->nonce_offset);
    return brain->nonce_offset;
}

/* QA functions (proactive_thermal_scale, voltage_ripple, asic_error_handle_non_blocking, i2c_guardian) — unchanged from previous */
