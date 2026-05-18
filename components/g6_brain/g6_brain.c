/*
 * g6_brain.c
 * Bitaxe G6 Brain — v1.0.0-beta2 (Phase 0 + Phase 0.1 + Phase 1 Completed)
 *
 * True J/TH efficiency optimization with separate power RLS model.
 * G6_ENABLE_EFFICIENCY_MODE = y → optimizes minimum J/TH
 * Default = off → safe hashrate maximizer (unchanged behavior)
 */

#include "g6_brain.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <math.h>
#include <inttypes.h>

static const char *TAG = "G6_BRAIN";

static const char *NVS_NAMESPACE = "g6_brain";
static const char *NVS_FINGERPRINT_KEY = "theta_fingerprint";

static const uint32_t NVS_SAVE_INTERVAL_TICKS = 300000UL;

/* Phase 1: NVS schema v2 (includes power model) */
static const uint32_t NVS_SCHEMA_VERSION = 2u;

/* ====================== RLS HELPERS ====================== */

static float compute_gradient_vff(float err, float sigma_sq) {
    if (sigma_sq < 1e-8f) sigma_sq = 1e-8f;
    float L = (err * err) / sigma_sq;
    return RLS_LAMBDA_MIN + (1.0f - RLS_LAMBDA_MIN) * powf(2.0f, -L);
}

static bool has_significant_innovation(const float P[RLS_N][RLS_N], const float x[RLS_N]) {
    float innovation = 0.0f;
    for (int i = 0; i < RLS_N; i++) {
        float px = 0.0f;
        for (int j = 0; j < RLS_N; j++) px += P[i][j] * x[j];
        innovation += x[i] * px;
    }
    return innovation > RLS_INNOVATION_THRESHOLD;
}

static float trace_P(const float P[RLS_N][RLS_N]) {
    float tr = 0.0f;
    for (int i = 0; i < RLS_N; i++) tr += P[i][i];
    return tr;
}

static void rls_symmetrize_clamp_and_stabilize(float P[RLS_N][RLS_N]) {
    for (int i = 0; i < RLS_N; i++) {
        for (int j = i + 1; j < RLS_N; j++) {
            float s = 0.5f * (P[i][j] + P[j][i]);
            P[i][j] = s;
            P[j][i] = s;
        }
    }
    for (int i = 0; i < RLS_N; i++) {
        if (P[i][i] > RLS_P_CLAMP_MAX) P[i][i] = RLS_P_CLAMP_MAX;
        if (P[i][i] < RLS_P_CLAMP_MIN) P[i][i] = RLS_P_CLAMP_MIN;
    }
}

static bool quadratic_has_valid_maximum(float a, float b, float c) {
    float h11 = 2.0f * a;
    float det = 4.0f * a * b - c * c;
    return isfinite(h11) && isfinite(det) && h11 < -1e-6f && det > 1e-6f;
}

/* ====================== SAFETY ====================== */

static bool is_thermal_safe(const G6BrainState *brain, float temp_c) {
    if (!isfinite(temp_c) || !isfinite(brain->temp_ceiling)) return false;
    return (temp_c < brain->temp_ceiling);
}

static void g6_safety_proactive_thermal_scale(G6BrainState *brain, float temp_c) {
    if (!brain || !isfinite(temp_c)) return;
    if (temp_c > (brain->temp_ceiling - 5.0f)) {
        brain->best_f = fmaxf(BM1370_F_MIN, brain->best_f * 0.96f);
        brain->best_v = fmaxf(BM1370_V_MIN, brain->best_v * 0.992f);
        ESP_LOGW(TAG, "PROACTIVE THERMAL: %.1f°C → best_f=%.1f best_v=%.1f", temp_c, brain->best_f, brain->best_v);
    }
}

static void g6_safety_check_voltage_ripple(G6BrainState *brain, float v_mv) {
    if (!brain || !isfinite(v_mv)) return;
    if (v_mv < BM1370_V_MIN || v_mv > BM1370_V_MAX) {
        brain->best_v = fmaxf(BM1370_V_MIN, fminf(BM1370_V_MAX, brain->best_v));
        ESP_LOGW(TAG, "VOLTAGE OUT OF RANGE: %.1f mV → clamped", v_mv);
    }
}

static void g6_asic_error_handle_non_blocking(G6BrainState *brain, float err_pct) {
    if (!brain) return;
    if (err_pct > brain->ner_threshold) {
        brain->model_quality = fminf(brain->model_quality, 0.25f);
        brain->cold_start = true;
        brain->best_f *= 0.92f;
        brain->best_v *= 0.985f;
        ESP_LOGW(TAG, "HIGH ERROR RATE (%.2f%%) — conservative back-off", err_pct);
    }
}

/* ====================== NVS v2 (Phase 1) ====================== */

esp_err_t g6_brain_load_nvs_fingerprint(G6BrainState *brain) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK) return err;

    size_t blob_size = sizeof(uint32_t)*2 + sizeof(brain->theta) + sizeof(brain->P) +
                       sizeof(brain->power_theta) + sizeof(brain->power_P);
    uint8_t buffer[1024];

    err = nvs_get_blob(nvs, NVS_FINGERPRINT_KEY, buffer, &blob_size);
    if (err == ESP_OK && blob_size == sizeof(buffer)) {
        uint32_t stored_version = 0, stored_size = 0;
        memcpy(&stored_version, buffer, sizeof(uint32_t));
        memcpy(&stored_size, buffer + sizeof(uint32_t), sizeof(uint32_t));

        if (stored_version == NVS_SCHEMA_VERSION) {
            size_t offset = sizeof(uint32_t)*2;
            memcpy(brain->theta, buffer + offset, sizeof(brain->theta));
            offset += sizeof(brain->theta);
            memcpy(brain->P, buffer + offset, sizeof(brain->P));
            offset += sizeof(brain->P);
            memcpy(brain->power_theta, buffer + offset, sizeof(brain->power_theta));
            offset += sizeof(brain->power_theta);
            memcpy(brain->power_P, buffer + offset, sizeof(brain->power_P));

            brain->nvs_valid = true;
            brain->cold_start = false;
            brain->power_cold_start = false;
            ESP_LOGI(TAG, "NVS fingerprint loaded (schema v%" PRIu32 " — Phase 1 ready)", stored_version);
        } else {
            ESP_LOGW(TAG, "NVS schema mismatch (stored v%" PRIu32 ", expected v%" PRIu32 ") — forcing cold start", stored_version, NVS_SCHEMA_VERSION);
            brain->cold_start = true;
            brain->power_cold_start = true;
            nvs_erase_key(nvs, NVS_FINGERPRINT_KEY);
            nvs_commit(nvs);
        }
    }
    nvs_close(nvs);
    return ESP_OK;
}

esp_err_t g6_brain_save_nvs_fingerprint(const G6BrainState *brain) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;

    size_t data_size = sizeof(brain->theta) + sizeof(brain->P) + sizeof(brain->power_theta) + sizeof(brain->power_P);
    uint8_t buffer[sizeof(uint32_t)*2 + 512];
    uint32_t version = NVS_SCHEMA_VERSION;
    uint32_t size_field = (uint32_t)data_size;

    memcpy(buffer, &version, sizeof(uint32_t));
    memcpy(buffer + sizeof(uint32_t), &size_field, sizeof(uint32_t));
    size_t offset = sizeof(uint32_t)*2;
    memcpy(buffer + offset, brain->theta, sizeof(brain->theta));
    offset += sizeof(brain->theta);
    memcpy(buffer + offset, brain->P, sizeof(brain->P));
    offset += sizeof(brain->P);
    memcpy(buffer + offset, brain->power_theta, sizeof(brain->power_theta));
    offset += sizeof(brain->power_theta);
    memcpy(buffer + offset, brain->power_P, sizeof(brain->power_P));

    err = nvs_set_blob(nvs, NVS_FINGERPRINT_KEY, buffer, offset);
    if (err == ESP_OK) err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
}

/* ====================== PHASE 1 RESET ====================== */

esp_err_t g6_brain_reset(G6BrainState *brain) {
    if (!brain) return ESP_ERR_INVALID_ARG;

    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_erase_key(nvs, NVS_FINGERPRINT_KEY);
        nvs_commit(nvs);
        nvs_close(nvs);
    }

    memset(brain, 0, sizeof(G6BrainState));

    // HR model
    brain->best_f = BM1370_F_CENTER;
    brain->best_v = BM1370_V_CENTER;
    brain->ridge_epsilon = RLS_RIDGE_EPSILON;
    brain->cold_start = true;
    brain->update_count = 0;
    brain->model_quality = 0.0f;
    brain->nvs_valid = false;
    brain->sample_state = BRAIN_STATE_IDLE;
    brain->control_mode = G6_MODE_RECOMMEND;

    for (int i = 0; i < RLS_N; i++)
        for (int j = 0; j < RLS_N; j++)
            brain->P[i][j] = (i == j) ? 1.0e5f : 0.0f;

    // Phase 1 power model
    for (int i = 0; i < RLS_N; i++)
        for (int j = 0; j < RLS_N; j++)
            brain->power_P[i][j] = (i == j) ? 1.0e5f : 0.0f;
    brain->power_cold_start = true;
    brain->power_update_count = 0;
    brain->power_model_quality = 0.0f;

    /* Phase 1 efficiency mode (safe default) */
    brain->use_efficiency_mode = false;
#if defined(CONFIG_G6_ENABLE_EFFICIENCY_MODE)
    brain->use_efficiency_mode = CONFIG_G6_ENABLE_EFFICIENCY_MODE;
#endif

        /* Phase 2 — Telemetry + full power model zeroing (guarantees clean start) */
    brain->power_model_quality = 0.0f;
    brain->power_update_count = 0;
    brain->power_cold_start = true;

    /* Telemetry snapshot safety (no-op on state, just ensures last_recommended_voltage is sane) */
    if (brain->best_v < BM1370_V_MIN || brain->best_v > BM1370_V_MAX) {
        brain->best_v = BM1370_V_CENTER;
    }

    // Kconfig values
#if defined(CONFIG_G6_TEMP_CEILING)
    brain->temp_ceiling = (float)CONFIG_G6_TEMP_CEILING;
#else
    brain->temp_ceiling = 70.0f;
#endif
#if defined(CONFIG_G6_NER_THRESHOLD)
    brain->ner_threshold = (float)CONFIG_G6_NER_THRESHOLD / 100.0f;
#else
    brain->ner_threshold = 2.5f;
#endif
#if defined(CONFIG_G6_DFS_STEP_MHZ)
    brain->dfs_step_mhz = (float)CONFIG_G6_DFS_STEP_MHZ;
#else
    brain->dfs_step_mhz = 25.0f;
#endif

    brain->Kp = 0.8f; brain->Ki = 0.05f; brain->Kd = 0.2f;
    brain->nvs_last_write_tick = xTaskGetTickCount();

    ESP_LOGW(TAG, "G6 Brain FULL RESET — Phase 1 J/TH mode ready");
    return ESP_OK;
}

/* ====================== SAMPLE STATE MACHINE ====================== */

static bool is_sample_valid(const G6BrainState *brain, float hr_ths, float temp_c, uint32_t shares) {
    if (!isfinite(hr_ths) || hr_ths <= 0.0f) return false;
    if (shares < MIN_SHARE_COUNT) return false;
    if (!is_thermal_safe(brain, temp_c)) return false;
    return true;
}

static void advance_sample_state(G6BrainState *brain, uint32_t now) {
    switch (brain->sample_state) {
        case BRAIN_STATE_IDLE:            brain->sample_state = BRAIN_STATE_APPLY_CANDIDATE; break;
        case BRAIN_STATE_APPLY_CANDIDATE: brain->settle_start_tick = now; brain->sample_state = BRAIN_STATE_SETTLE_WAIT; break;
        case BRAIN_STATE_SETTLE_WAIT:
            if (now - brain->settle_start_tick >= SETTLE_MS) brain->sample_state = BRAIN_STATE_MEASURE_WINDOW;
            break;
        case BRAIN_STATE_MEASURE_WINDOW:
            if (brain->measure_start_tick == 0) brain->measure_start_tick = now;
            if (now - brain->measure_start_tick >= MIN_WINDOW_MS) {
                brain->sample_state = BRAIN_STATE_VALIDATE_SAMPLE;
                brain->measure_start_tick = 0;
            }
            break;
        case BRAIN_STATE_VALIDATE_SAMPLE: brain->sample_state = BRAIN_STATE_RLS_UPDATE; break;
        case BRAIN_STATE_RLS_UPDATE:      brain->sample_state = BRAIN_STATE_DECIDE_NEXT; break;
        case BRAIN_STATE_DECIDE_NEXT:     brain->sample_state = BRAIN_STATE_IDLE; break;
        default:                          brain->sample_state = BRAIN_STATE_IDLE; break;
    }
}

/* ====================== PUBLIC API ====================== */

esp_err_t g6_brain_init(G6BrainState *brain) {
    if (!brain) return ESP_ERR_INVALID_ARG;

    nvs_handle_t nvs_test;
    esp_err_t nvs_err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_test);
    if (nvs_err == ESP_ERR_NVS_NOT_INITIALIZED) {
        ESP_LOGE(TAG, "NVS NOT INITIALIZED");
        return ESP_ERR_NVS_NOT_INITIALIZED;
    }
    if (nvs_err == ESP_OK) nvs_close(nvs_test);

    memset(brain, 0, sizeof(G6BrainState));

    brain->best_f = BM1370_F_CENTER;
    brain->best_v = BM1370_V_CENTER;

    brain->ridge_epsilon = RLS_RIDGE_EPSILON;
    brain->cold_start = true;
    brain->update_count = 0;
    brain->model_quality = 0.0f;
    brain->nvs_valid = false;
    brain->sample_state = BRAIN_STATE_IDLE;
    brain->control_mode = G6_MODE_RECOMMEND;

    for (int i = 0; i < RLS_N; i++)
        for (int j = 0; j < RLS_N; j++)
            brain->P[i][j] = (i == j) ? 1.0e5f : 0.0f;

    /* PHASE 1 POWER MODEL INIT */
    for (int i = 0; i < RLS_N; i++)
        for (int j = 0; j < RLS_N; j++)
            brain->power_P[i][j] = (i == j) ? 1.0e5f : 0.0f;
    brain->power_cold_start = true;
    brain->power_update_count = 0;
    brain->power_model_quality = 0.0f;

    /* Phase 1 efficiency mode (safe default) */
    brain->use_efficiency_mode = false;
#if defined(CONFIG_G6_ENABLE_EFFICIENCY_MODE)
    brain->use_efficiency_mode = CONFIG_G6_ENABLE_EFFICIENCY_MODE;
#endif

    /* Kconfig wiring */
#if defined(CONFIG_G6_TEMP_CEILING)
    brain->temp_ceiling = (float)CONFIG_G6_TEMP_CEILING;
#else
    brain->temp_ceiling = 70.0f;
#endif
#if defined(CONFIG_G6_NER_THRESHOLD)
    brain->ner_threshold = (float)CONFIG_G6_NER_THRESHOLD / 100.0f;
#else
    brain->ner_threshold = 2.5f;
#endif
#if defined(CONFIG_G6_DFS_STEP_MHZ)
    brain->dfs_step_mhz = (float)CONFIG_G6_DFS_STEP_MHZ;
#else
    brain->dfs_step_mhz = 25.0f;
#endif

    brain->Kp = 0.8f; brain->Ki = 0.05f; brain->Kd = 0.2f;

    g6_brain_load_nvs_fingerprint(brain);
    brain->nvs_last_write_tick = xTaskGetTickCount();

    ESP_LOGI(TAG, "G6 Brain v1.0.0-beta2 initialized (Phase 1 J/TH %s)", brain->use_efficiency_mode ? "ENABLED" : "DISABLED");
    return ESP_OK;
}

esp_err_t g6_brain_update(G6BrainState *brain,
                          float f_mhz, float v_mv, float hr_ths,
                          float power_w, float temp_c, float err_pct,
                          uint32_t share_count)
{
    if (!brain) return ESP_ERR_INVALID_ARG;

    uint32_t now = xTaskGetTickCount();

    if (!isfinite(f_mhz) || !isfinite(v_mv) || !isfinite(hr_ths) ||
        !isfinite(power_w) || !isfinite(temp_c) || !isfinite(err_pct) ||
        hr_ths <= 0.0f || f_mhz < BM1370_F_MIN || v_mv < BM1370_V_MIN) {
        return ESP_ERR_INVALID_ARG;
    }

    if (power_w < 0.0f || power_w > 100.0f) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!is_thermal_safe(brain, temp_c)) {
        goto safety_layer;
    }

    if (err_pct > brain->ner_threshold) {
        g6_asic_error_handle_non_blocking(brain, err_pct);
        goto safety_layer;
    }

    brain->last_update_timestamp = now;

    bool valid = is_sample_valid(brain, hr_ths, temp_c, share_count);

    if (valid || brain->sample_state == BRAIN_STATE_SETTLE_WAIT || brain->sample_state == BRAIN_STATE_MEASURE_WINDOW) {
        advance_sample_state(brain, now);
    }

    if (!valid) goto safety_layer;

    /* HR RLS update */
    float fn = (f_mhz - BM1370_F_CENTER) / BM1370_F_SCALE;
    float vn = (v_mv - BM1370_V_CENTER) / BM1370_V_SCALE;
    float x[RLS_N] = {fn*fn, vn*vn, fn*vn, fn, vn, 1.0f};

    float y_pred = 0.0f;
    for (int i = 0; i < RLS_N; i++) y_pred += brain->theta[i] * x[i];
    float err = hr_ths - y_pred;

    if (has_significant_innovation(brain->P, x) && trace_P(brain->P) <= RLS_TRACE_MAX) {
        float lambda_eff = brain->cold_start ? 0.985f : compute_gradient_vff(fabsf(err), RLS_VFF_SIGMA_SQ);
        if (lambda_eff < RLS_LAMBDA_MIN) lambda_eff = RLS_LAMBDA_MIN;

        float Px[RLS_N] = {0};
        for (int i = 0; i < RLS_N; i++)
            for (int j = 0; j < RLS_N; j++)
                Px[i] += brain->P[i][j] * x[j];

        float denom = lambda_eff;
        for (int i = 0; i < RLS_N; i++) denom += x[i] * Px[i];
        if (denom < 1e-9f) denom = 1e-9f;

        float k[RLS_N];
        for (int i = 0; i < RLS_N; i++) k[i] = Px[i] / denom;

        for (int i = 0; i < RLS_N; i++) brain->theta[i] += k[i] * err;

        for (int i = 0; i < RLS_N; i++) {
            for (int j = 0; j < RLS_N; j++)
                brain->P[i][j] = (brain->P[i][j] - k[i] * Px[j]) / lambda_eff;
            brain->P[i][i] += brain->ridge_epsilon;
        }

        rls_symmetrize_clamp_and_stabilize(brain->P);

        brain->model_quality = fmaxf(0.0f, 1.0f - fabsf(err) / (hr_ths + 1.0f));
        brain->update_count++;
        if (brain->update_count > 25) brain->cold_start = false;
    }

    /* PHASE 1: POWER RLS UPDATE */
    if (brain->use_efficiency_mode && valid) {
        float y_power_pred = 0.0f;
        for (int i = 0; i < RLS_N; i++) y_power_pred += brain->power_theta[i] * x[i];
        float power_err = power_w - y_power_pred;

        if (has_significant_innovation(brain->power_P, x) && trace_P(brain->power_P) <= RLS_TRACE_MAX) {
            float lambda_eff = brain->power_cold_start ? 0.985f : compute_gradient_vff(fabsf(power_err), RLS_VFF_SIGMA_SQ);
            if (lambda_eff < RLS_LAMBDA_MIN) lambda_eff = RLS_LAMBDA_MIN;

            float Px[RLS_N] = {0};
            for (int i = 0; i < RLS_N; i++)
                for (int j = 0; j < RLS_N; j++)
                    Px[i] += brain->power_P[i][j] * x[j];

            float denom = lambda_eff;
            for (int i = 0; i < RLS_N; i++) denom += x[i] * Px[i];
            if (denom < 1e-9f) denom = 1e-9f;

            float k[RLS_N];
            for (int i = 0; i < RLS_N; i++) k[i] = Px[i] / denom;

            for (int i = 0; i < RLS_N; i++) brain->power_theta[i] += k[i] * power_err;

            for (int i = 0; i < RLS_N; i++) {
                for (int j = 0; j < RLS_N; j++)
                    brain->power_P[i][j] = (brain->power_P[i][j] - k[i] * Px[j]) / lambda_eff;
                brain->power_P[i][i] += brain->ridge_epsilon;
            }

            rls_symmetrize_clamp_and_stabilize(brain->power_P);

            brain->power_model_quality = fmaxf(0.0f, 1.0f - fabsf(power_err) / (power_w + 1.0f));
            brain->power_update_count++;
            if (brain->power_update_count > 25) brain->power_cold_start = false;
        }
    }

safety_layer:
    g6_safety_proactive_thermal_scale(brain, temp_c);
    g6_safety_check_voltage_ripple(brain, v_mv);

    float candidate_f, candidate_v;
    g6_brain_get_optimal(brain, &candidate_f, &candidate_v, NULL);

    if (brain->control_mode == G6_MODE_AUTO) {
        brain->best_f = candidate_f;
        brain->best_v = candidate_v;
    }

    if (brain->best_f < BM1370_F_MIN) brain->best_f = BM1370_F_MIN;
    if (brain->best_f > BM1370_F_MAX) brain->best_f = BM1370_F_MAX;
    if (brain->best_v < BM1370_V_MIN) brain->best_v = BM1370_V_MIN;
    if (brain->best_v > BM1370_V_MAX) brain->best_v = BM1370_V_MAX;

    brain->last_efficiency = (hr_ths > 0.0f) ? (power_w / hr_ths) : 0.0f;

    if (brain->control_mode == G6_MODE_AUTO &&
        (fabsf(brain->best_f - f_mhz) > 8.0f || fabsf(brain->best_v - v_mv) > 8.0f)) {
        brain->last_setting_change_tick = now;
        brain->sample_state = BRAIN_STATE_APPLY_CANDIDATE;
        brain->measure_start_tick = 0;
    }

    if (brain->update_count > 10 && (now - brain->nvs_last_write_tick > NVS_SAVE_INTERVAL_TICKS)) {
        g6_brain_save_nvs_fingerprint(brain);
        brain->nvs_last_write_tick = now;
        ESP_LOGI(TAG, "NVS fingerprint auto-saved (warm-start ready)");
    }

    return ESP_OK;
}

void g6_brain_get_optimal(const G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr) {
    if (!brain || !opt_f || !opt_v) return;

    float a = brain->theta[0], b = brain->theta[1], c = brain->theta[2];
    float d = brain->theta[3], e = brain->theta[4], g = brain->theta[5];

    *opt_f = brain->best_f;
    *opt_v = brain->best_v;

    if (quadratic_has_valid_maximum(a, b, c)) {
        float det = 4.0f * a * b - c * c;
        if (fabsf(det) > 1e-6f) {
            float f_norm = (2.0f * b * (-d) - c * (-e)) / det;
            float v_norm = (2.0f * a * (-e) - c * (-d)) / det;

            float f_cand = f_norm * BM1370_F_SCALE + BM1370_F_CENTER;
            float v_cand = v_norm * BM1370_V_SCALE + BM1370_V_CENTER;

            if (f_cand >= BM1370_F_MIN && f_cand <= BM1370_F_MAX &&
                v_cand >= BM1370_V_MIN && v_cand <= BM1370_V_MAX) {
                *opt_f = f_cand;
                *opt_v = v_cand;
            }
        }
    }

    /* PHASE 1: J/TH EFFICIENCY OPTIMIZATION */
    if (brain->use_efficiency_mode) {
        float best_jth = 1e9f;
        float best_f = *opt_f;
        float best_v = *opt_v;

        for (float df = -50.0f; df <= 50.0f; df += 10.0f) {
            for (float dv = -30.0f; dv <= 30.0f; dv += 10.0f) {
                float f = fminf(fmaxf(BM1370_F_MIN, *opt_f + df), BM1370_F_MAX);
                float v = fminf(fmaxf(BM1370_V_MIN, *opt_v + dv), BM1370_V_MAX);

                float fn = (f - BM1370_F_CENTER) / BM1370_F_SCALE;
                float vn = (v - BM1370_V_CENTER) / BM1370_V_SCALE;

                float hr = a*fn*fn + b*vn*vn + c*fn*vn + d*fn + e*vn + g;
                float pw = 0.0f;
                for (int i = 0; i < RLS_N; i++) {
                    float xval[RLS_N] = {fn*fn, vn*vn, fn*vn, fn, vn, 1.0f};
                    pw += brain->power_theta[i] * xval[i];
                }

                if (hr > 10.0f && pw > 1.0f) {
                    float jth = pw / hr;
                    if (jth < best_jth) {
                        best_jth = jth;
                        best_f = f;
                        best_v = v;
                    }
                }
            }
        }
        *opt_f = best_f;
        *opt_v = best_v;
    }

    if (pred_hr) {
        float fn = (*opt_f - BM1370_F_CENTER) / BM1370_F_SCALE;
        float vn = (*opt_v - BM1370_V_CENTER) / BM1370_V_SCALE;
        *pred_hr = a*fn*fn + b*vn*vn + c*fn*vn + d*fn + e*vn + g;
    }
}

float g6_brain_get_model_quality(const G6BrainState *brain) {
    return brain ? brain->model_quality : 0.0f;
}

float g6_brain_get_cov_condition(const G6BrainState *brain) {
    if (!brain) return 0.0f;
    float min_diag = 1e30f, max_diag = 0.0f;
    for (int i = 0; i < RLS_N; i++) {
        if (brain->P[i][i] < min_diag) min_diag = brain->P[i][i];
        if (brain->P[i][i] > max_diag) max_diag = brain->P[i][i];
    }
    return (min_diag > 1e-9f) ? (max_diag / min_diag) : 0.0f;
}

esp_err_t g6_brain_self_test(G6BrainState *brain) {
    if (!brain) return ESP_ERR_INVALID_ARG;
    bool ok = true;
    float min_diag = 1e30f, max_diag = 0.0f;

    for (int i = 0; i < RLS_N; i++) {
        if (brain->P[i][i] < RLS_P_CLAMP_MIN || brain->P[i][i] > RLS_P_CLAMP_MAX) ok = false;
        if (brain->P[i][i] < min_diag) min_diag = brain->P[i][i];
        if (brain->P[i][i] > max_diag) max_diag = brain->P[i][i];
        for (int j = i + 1; j < RLS_N; j++)
            if (fabsf(brain->P[i][j] - brain->P[j][i]) > RLS_SYMMETRY_TOLERANCE) ok = false;
    }

    float cond = (min_diag > 1e-9f) ? (max_diag / min_diag) : 0.0f;
    if (cond > 5e5f) ok = false;

    ESP_LOGI(TAG, "Self-test: %s (quality=%.3f, eff=%.2f W/TH, cond=%.1f)", 
             ok ? "PASSED" : "DEGRADED", brain->model_quality, brain->last_efficiency, cond);

    return ok ? ESP_OK : ESP_FAIL;
}
/* ====================== TELEMETRY (Phase 2 - new) ====================== */

void g6_brain_get_telemetry(const G6BrainState *brain, G6BrainTelemetry *out) {
    if (!brain || !out) return;

    /* hashrate model */
    memcpy(out->theta_hashrate, brain->theta, sizeof(brain->theta));
    out->trace_P_hashrate = trace_P(brain->P);

    /* power model (Phase 1) */
    memcpy(out->theta_power, brain->power_theta, sizeof(brain->power_theta));
    out->trace_P_power = trace_P(brain->power_P);

    /* innovation & safety (lightweight snapshot) */
    out->last_innovation = 0.0f;                    // placeholder - real tracking added later
    out->safety_status = (uint8_t)G6_SAFETY_OK;     // real enum (will become dynamic in later files)
    out->efficiency_mode_active = brain->use_efficiency_mode;
    out->last_recommended_voltage = brain->best_v;  // last known safe voltage

    ESP_LOGD(TAG, "Telemetry snapshot taken - trace_P_hr=%.2e trace_P_pw=%.2e eff_mode=%d",
             out->trace_P_hashrate, out->trace_P_power, out->efficiency_mode_active);
}
