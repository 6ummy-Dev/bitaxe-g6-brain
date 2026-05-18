/*
 * g6_brain.c
 * Bitaxe G6 Brain — v1.0.0-beta2 (QA Hardened + Fixes)
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

static const char *TAG = "G6_BRAIN";

static const char *NVS_NAMESPACE = "g6_brain";
static const char *NVS_FINGERPRINT_KEY = "theta_fingerprint";

/* ====================== RLS HELPERS ====================== */

static float compute_gradient_vff(float err, float sigma_sq) {
    if (sigma_sq < 1e-8f) sigma_sq = 1e-8f;
    float L = (err * err) / sigma_sq;
    return RLS_LAMBDA_MIN + (1.0f - RLS_LAMBDA_MIN) * powf(2.0f, -L);
}

static bool has_significant_innovation(const G6BrainState *brain, const float x[RLS_N]) {
    float innovation = 0.0f;
    for (int i = 0; i < RLS_N; i++) {
        float px = 0.0f;
        for (int j = 0; j < RLS_N; j++) px += brain->P[i][j] * x[j];
        innovation += x[i] * px;
    }
    return innovation > 1e-4f;
}

static float trace_P(const G6BrainState *brain) {
    float tr = 0.0f;
    for (int i = 0; i < RLS_N; i++) tr += brain->P[i][i];
    return tr;
}

static void rls_symmetrize_clamp_and_stabilize(G6BrainState *brain) {
    for (int i = 0; i < RLS_N; i++) {
        for (int j = i + 1; j < RLS_N; j++) {
            float s = 0.5f * (brain->P[i][j] + brain->P[j][i]);
            brain->P[i][j] = s;
            brain->P[j][i] = s;
        }
    }
    for (int i = 0; i < RLS_N; i++) {
        if (brain->P[i][i] > RLS_P_CLAMP_MAX) brain->P[i][i] = RLS_P_CLAMP_MAX;
        if (brain->P[i][i] < RLS_P_CLAMP_MIN) brain->P[i][i] = RLS_P_CLAMP_MIN;
    }
}

static bool quadratic_has_valid_maximum(float a, float b, float c) {
    float h11 = 2.0f * a;
    float det = 4.0f * a * b - c * c;
    return isfinite(h11) && isfinite(det) && h11 < -1e-6f && det > 1e-6f;
}

/* ====================== SELF-CONTAINED SAFETY ====================== */

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

/* ====================== NVS ====================== */

esp_err_t g6_brain_load_nvs_fingerprint(G6BrainState *brain) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK) return err;

    size_t size = sizeof(brain->theta) + sizeof(brain->P);
    uint8_t buffer[sizeof(brain->theta) + sizeof(brain->P)];
    err = nvs_get_blob(nvs, NVS_FINGERPRINT_KEY, buffer, &size);

    if (err == ESP_OK && size == sizeof(buffer)) {
        memcpy(brain->theta, buffer, sizeof(brain->theta));
        memcpy(brain->P, buffer + sizeof(brain->theta), sizeof(brain->P));
        brain->nvs_valid = true;
        brain->cold_start = false;
    }
    nvs_close(nvs);
    return err;
}

esp_err_t g6_brain_save_nvs_fingerprint(const G6BrainState *brain) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;

    uint8_t buffer[sizeof(brain->theta) + sizeof(brain->P)];
    memcpy(buffer, brain->theta, sizeof(brain->theta));
    memcpy(buffer + sizeof(brain->theta), brain->P, sizeof(brain->P));

    err = nvs_set_blob(nvs, NVS_FINGERPRINT_KEY, buffer, sizeof(buffer));
    if (err == ESP_OK) err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
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

    brain->ridge_epsilon = 1e-5f;
    brain->cold_start = true;
    brain->update_count = 0;
    brain->model_quality = 0.0f;
    brain->nvs_valid = false;
    brain->sample_state = BRAIN_STATE_IDLE;

    for (int i = 0; i < RLS_N; i++)
        for (int j = 0; j < RLS_N; j++)
            brain->P[i][j] = (i == j) ? 1.0e5f : 0.0f;

    brain->temp_ceiling  = 70.0f;
    brain->ner_threshold = 2.5f;
    brain->dfs_step_mhz  = 25.0f;
    brain->Kp = 0.8f; brain->Ki = 0.05f; brain->Kd = 0.2f;

    g6_brain_load_nvs_fingerprint(brain);

    ESP_LOGI(TAG, "G6 Brain v1.0.0-beta2 initialized successfully");
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

    // NEW-2: Power sanity check
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

    /* RLS */
    float fn = (f_mhz - BM1370_F_CENTER) / BM1370_F_SCALE;
    float vn = (v_mv - BM1370_V_CENTER) / BM1370_V_SCALE;
    float x[RLS_N] = {fn*fn, vn*vn, fn*vn, fn, vn, 1.0f};

    float y_pred = 0.0f;
    for (int i = 0; i < RLS_N; i++) y_pred += brain->theta[i] * x[i];
    float err = hr_ths - y_pred;

    if (has_significant_innovation(brain, x) && trace_P(brain) <= RLS_TRACE_MAX) {
        float lambda_eff = brain->cold_start ? 0.985f : compute_gradient_vff(fabsf(err), 0.008f);
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

        rls_symmetrize_clamp_and_stabilize(brain);

        brain->model_quality = fmaxf(0.0f, 1.0f - fabsf(err) / (hr_ths + 1.0f));
        brain->update_count++;
        if (brain->update_count > 25) brain->cold_start = false;
    }

safety_layer:
    g6_safety_proactive_thermal_scale(brain, temp_c);
    g6_safety_check_voltage_ripple(brain, v_mv);

    // NOTE: NER handling is now ONLY inside safety_layer via the call above when we goto from the check.
    // Removed duplicate call before goto to fix double-execution (NEW-1).

    g6_brain_get_optimal(brain, &brain->best_f, &brain->best_v, NULL);

    if (brain->best_f < BM1370_F_MIN) brain->best_f = BM1370_F_MIN;
    if (brain->best_f > BM1370_F_MAX) brain->best_f = BM1370_F_MAX;
    if (brain->best_v < BM1370_V_MIN) brain->best_v = BM1370_V_MIN;
    if (brain->best_v > BM1370_V_MAX) brain->best_v = BM1370_V_MAX;

    brain->last_efficiency = (hr_ths > 0.0f) ? (power_w / hr_ths) : 0.0f;

    if (fabsf(brain->best_f - f_mhz) > 8.0f || fabsf(brain->best_v - v_mv) > 8.0f) {
        brain->last_setting_change_tick = now;
        brain->sample_state = BRAIN_STATE_APPLY_CANDIDATE;
        brain->measure_start_tick = 0;
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
            if (fabsf(brain->P[i][j] - brain->P[j][i]) > 1e-4f) ok = false;
    }

    float cond = (min_diag > 1e-9f) ? (max_diag / min_diag) : 0.0f;
    if (cond > 5e5f) ok = false;

    ESP_LOGI(TAG, "Self-test: %s (quality=%.3f, eff=%.2f J/TH, cond=%.1f)", 
             ok ? "PASSED" : "DEGRADED", brain->model_quality, brain->last_efficiency, cond);

    return ok ? ESP_OK : ESP_FAIL;
}
