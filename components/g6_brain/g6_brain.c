/*
 * g6_brain.c
 * Bitaxe G6 Brain — v1.0 Beta
 * Modular RLS quadratic optimizer + QA hardening + Stochastic Nonce
 * Bitaxe Brains Project
 */

#include "g6_brain.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "nvs_flash.h"
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

    brain->Kp = CONFIG_G6_KP;
    brain->Ki = CONFIG_G6_KI;
    brain->Kd = CONFIG_G6_KD;
    brain->temp_ceiling = CONFIG_G6_TEMP_CEILING;
    brain->dfs_step_mhz = CONFIG_G6_DFS_STEP;
    brain->ner_threshold = CONFIG_G6_NER_THRESHOLD;

    brain->nonce_offset = 0;
    brain->enable_low_latency_jobs = CONFIG_G6_LOW_LATENCY_JOBS;

    brain->last_update_timestamp = xTaskGetTickCount();
    brain->nvs_last_write_tick = brain->last_update_timestamp;

    ESP_LOGI(TAG, "G6 Brain v1.0 Beta initialized — RLS + Stochastic Nonce");
    return ESP_OK;
}

esp_err_t g6_brain_update(G6BrainState *brain, float f_mhz, float v_mv, float hr_ths,
                          float power_w, float temp_c, float err_pct) {
    if (!brain) return ESP_ERR_INVALID_ARG;

    uint32_t now = xTaskGetTickCount();

    /* RLS QUADRATIC UPDATE */
    float x[6] = {
        normalize_f(f_mhz) * normalize_f(f_mhz),
        normalize_v(v_mv) * normalize_v(v_mv),
        normalize_f(f_mhz) * normalize_v(v_mv),
        normalize_f(f_mhz),
        normalize_v(v_mv),
        1.0f
    };

    float y_pred = 0.0f;
    for (int i = 0; i < 6; i++) y_pred += brain->theta[i] * x[i];
    float err = hr_ths - y_pred;

    float Px[6] = {0};
    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 6; j++)
            Px[i] += brain->P[i][j] * x[j];

    float denom = brain->lambda;
    for (int i = 0; i < 6; i++) denom += x[i] * Px[i];
    if (denom < 1e-9f) denom = 1e-9f;

    float k[6];
    for (int i = 0; i < 6; i++) k[i] = Px[i] / denom;

    for (int i = 0; i < 6; i++) brain->theta[i] += k[i] * err;

    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 6; j++) {
            brain->P[i][j] = (brain->P[i][j] - k[i] * Px[j]) / brain->lambda;
        }
        brain->P[i][i] += brain->ridge_epsilon;
        if (brain->P[i][i] < 1e-6f) brain->P[i][i] = 1e-6f;
    }

    brain->model_quality = 1.0f - fabsf(err) / (hr_ths + 1.0f);
    brain->update_count++;
    if (brain->update_count > 30) brain->cold_start = false;

    /* QA SAFETY LAYER */
    g6_safety_proactive_thermal_scale(brain, temp_c);
    g6_safety_check_voltage_ripple(brain, v_mv);
    if (err_pct > brain->ner_threshold) g6_asic_error_handle_non_blocking(brain);

    /* Update optimal setpoints */
    g6_brain_get_optimal(brain, &brain->best_f, &brain->best_v, NULL);

    /* Safety clamps */
    if (brain->best_f < 400.0f) brain->best_f = 400.0f;
    if (brain->best_f > 850.0f) brain->best_f = 850.0f;
    if (brain->best_v < 1050.0f) brain->best_v = 1050.0f;
    if (brain->best_v > 1350.0f) brain->best_v = 1350.0f;

    return ESP_OK;
}

void g6_brain_get_optimal(const G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr) {
    if (!brain || !opt_f || !opt_v) return;

    float a = brain->theta[0], b = brain->theta[1], c = brain->theta[2];
    float d = brain->theta[3], e = brain->theta[4], g = brain->theta[5];

    float det = 4.0f * a * b - c * c;
    float f_norm = 0.0f, v_norm = 0.0f;

    if (fabsf(det) > 1e-6f) {
        f_norm = (2.0f * b * (-d) - c * (-e)) / det;
        v_norm = (2.0f * a * (-e)) / det;
    }

    *opt_f = f_norm * 200.0f + 700.0f;
    *opt_v = v_norm * 150.0f + 1200.0f;

    if (pred_hr) {
        *pred_hr = a*(*opt_f)*(*opt_f) + b*(*opt_v)*(*opt_v) + c*(*opt_f)*(*opt_v) +
                   d*(*opt_f) + e*(*opt_v) + g;
    }
}

float g6_brain_get_model_quality(const G6BrainState *brain) {
    return brain ? brain->model_quality : 0.0f;
}

void g6_brain_get_full_telemetry(const G6BrainState *brain, char *json_buf, size_t buf_size) {
    if (!brain || !json_buf) return;
    snprintf(json_buf, buf_size,
        "{\"version\":\"v1.0 Beta\",\"theta\":[%.6f,%.6f,%.6f,%.6f,%.6f,%.6f],"
         "\"model_quality\":%.3f,\"best_f\":%.1f,\"best_v\":%.1f,"
         "\"temp_rise_rate\":%.2f,\"voltage_variance\":%.3f,"
         "\"nonce_offset\":%lu,\"i2c_hard_fault\":%d}",
        brain->theta[0], brain->theta[1], brain->theta[2],
        brain->theta[3], brain->theta[4], brain->theta[5],
        brain->model_quality, brain->best_f, brain->best_v,
        brain->temp_rise_rate, brain->voltage_variance,
        brain->nonce_offset, brain->i2c_hard_fault_triggered);
}

bool g6_brain_self_test(G6BrainState *brain) {
    ESP_LOGI(TAG, "=== G6 BRAIN SELF-TEST START ===");
    float test_f[] = {600,650,700,750,800};
    float test_v[] = {1150,1200,1250,1300,1350};
    float true_theta[6] = {0.001f, 0.0005f, -0.0008f, 2.5f, 1.8f, -1200.0f};

    for (int i = 0; i < 50; i++) {
        int idx = i % 5;
        float f = test_f[idx] + (rand() % 20 - 10);
        float v = test_v[idx] + (rand() % 30 - 15);
        float hr = true_theta[0]*f*f + true_theta[1]*v*v + true_theta[2]*f*v +
                   true_theta[3]*f + true_theta[4]*v + true_theta[5] + (rand()%50)/100.0f;
        g6_brain_update(brain, f, v, hr, 0.0f, 60.0f, 0.0f);
    }

    bool quality_ok = brain->model_quality > 0.85f;
    float opt_f, opt_v, pred_hr;
    g6_brain_get_optimal(brain, &opt_f, &opt_v, &pred_hr);
    bool opt_ok = (opt_f > 650.0f && opt_f < 800.0f && opt_v > 1150.0f && opt_v < 1350.0f);

    ESP_LOGI(TAG, "Self-test result: quality=%.3f opt_ok=%d", brain->model_quality, opt_ok);
    return quality_ok && opt_ok;
}

/* ====================== STOCHASTIC NONCE ====================== */

uint32_t g6_brain_get_stochastic_nonce_start(G6BrainState *brain) {
    if (!brain) return 0;
    brain->nonce_offset = esp_random();
    ESP_LOGD(TAG, "Stochastic nonce offset set: 0x%08lX", brain->nonce_offset);
    return brain->nonce_offset;
}

/* ====================== QA SAFETY FUNCTIONS (inlined) ====================== */

void g6_safety_proactive_thermal_scale(G6BrainState *brain, float current_temp) {
    uint32_t now = xTaskGetTickCount();
    float dt = (now - brain->last_update_timestamp) / 1000.0f;
    if (dt > 0.0f) {
        brain->temp_rise_rate = (current_temp - brain->last_temp_c) / dt;
        if (brain->temp_rise_rate > CONFIG_G6_PROACTIVE_DFS_THRESHOLD) {
            brain->best_f -= brain->dfs_step_mhz;
            ESP_LOGW(TAG, "PROACTIVE THERMAL SCALE: ΔT/dt=%.1f°C/s", brain->temp_rise_rate);
        }
    }
    brain->last_temp_c = current_temp;
    brain->last_update_timestamp = now;
}

void g6_safety_check_voltage_ripple(G6BrainState *brain, float measured_v) {
    brain->voltage_history[brain->voltage_hist_idx] = measured_v;
    brain->voltage_hist_idx = (brain->voltage_hist_idx + 1) % 8;

    float mean = 0.0f;
    for (int i = 0; i < 8; i++) mean += brain->voltage_history[i];
    mean /= 8.0f;

    brain->voltage_variance = 0.0f;
    for (int i = 0; i < 8; i++) {
        float diff = brain->voltage_history[i] - mean;
        brain->voltage_variance += diff * diff;
    }
    brain->voltage_variance /= 8.0f;

    if (brain->voltage_variance > 0.05f * mean) {
        brain->best_v -= 10.0f;
        ESP_LOGW(TAG, "VOLTAGE RIPPLE DETECTED (%.1f%%)", brain->voltage_variance / mean * 100.0f);
    }
}

void g6_asic_error_handle_non_blocking(G6BrainState *brain) {
    brain->best_v += 5.0f;
    ESP_LOGW(TAG, "BM1366 NON-BLOCKING ERROR — auto +5mV tune");
}

void g6_brain_i2c_guardian_recover(i2c_port_t port) {
    ESP_LOGW(TAG, "I2C guardian recover triggered on port %d", port);
}
