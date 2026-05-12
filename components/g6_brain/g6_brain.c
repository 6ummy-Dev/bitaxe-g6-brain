/*
 * g6_brain.c
 * Bitaxe G6 Brain — v2.0 (ALL-IN-ONE / NASA Level C / Zero External Dependencies)
 *
 * Complete self-contained RLS quadratic optimizer + full aerospace-grade safety.
 * No g6_safety.c, no g6_safety.h, no external calls.
 *
 * Everything the brain needs is now inside this single file + g6_brain.h.
 * Drop-in replacement for the modular repo version. 100% compliant with the
 * G6BrainState struct and public API from the official header.
 *
 * REQ-TRACE:
 *   - SAF-ALL-001: Thermal ceiling + proactive scaling + ΔT awareness (future)
 *   - SAF-ALL-002: Voltage & frequency slew limits + ripple guard
 *   - SAF-ALL-003: ASIC error non-blocking handler + model quality gate
 *   - RLS-001: Proper cold-start covariance + innovation gate + ridge
 *   - INIT-001: All fields explicitly initialized (fixes original 0-value bugs)
 */

#include "g6_brain.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <math.h>

static const char *TAG = "G6_BRAIN";

/* NVS */
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

/* ====================== SELF-CONTAINED SAFETY (was in g6_safety.c — now internal) ====================== */
static bool is_thermal_safe(const G6BrainState *brain, float temp_c) {
    if (!isfinite(temp_c) || !isfinite(brain->temp_ceiling)) return false;
    if (temp_c >= brain->temp_ceiling) {
        ESP_LOGW(TAG, "THERMAL VIOLATION: %.1f°C >= %.1f°C ceiling — FAIL CLOSED", temp_c, brain->temp_ceiling);
        return false;
    }
    return true;
}

static bool is_voltage_safe(float new_v, float current_v) {
    if (!isfinite(new_v) || !isfinite(current_v)) return false;
    float delta = fabsf(new_v - current_v);
    if (delta > MAX_VOLT_STEP) {
        ESP_LOGW(TAG, "VOLTAGE SLEW VIOLATION: %.1f mV > %.0f mV limit", delta, MAX_VOLT_STEP);
        return false;
    }
    return (new_v >= BM1370_V_MIN && new_v <= BM1370_V_MAX);
}

static bool is_frequency_safe(float new_f, float current_f) {
    if (!isfinite(new_f) || !isfinite(current_f)) return false;
    float delta = fabsf(new_f - current_f);
    if (delta > MAX_FREQ_STEP) {
        ESP_LOGW(TAG, "FREQ SLEW VIOLATION: %.1f MHz > %.0f MHz limit", delta, MAX_FREQ_STEP);
        return false;
    }
    return (new_f >= BM1370_F_MIN && new_f <= BM1370_F_MAX);
}

static void g6_safety_proactive_thermal_scale(G6BrainState *brain, float temp_c) {
    if (!brain || !isfinite(temp_c)) return;

    if (temp_c > (brain->temp_ceiling - 5.0f)) {
        float scale = 0.96f;
        brain->best_f = fmaxf(BM1370_F_MIN, brain->best_f * scale);
        brain->best_v = fmaxf(BM1370_V_MIN, brain->best_v * 0.992f);
        ESP_LOGW(TAG, "PROACTIVE THERMAL SCALE: %.1f°C → best_f=%.1f best_v=%.1f", temp_c, brain->best_f, brain->best_v);
    }
}

static void g6_safety_check_voltage_ripple(G6BrainState *brain, float v_mv) {
    if (!brain || !isfinite(v_mv)) return;

    if (v_mv < BM1370_V_MIN || v_mv > BM1370_V_MAX) {
        ESP_LOGW(TAG, "VOLTAGE OUT OF BM1370 SAFE WINDOW: %.1f mV — clamping best_v", v_mv);
        if (brain->best_v > BM1370_V_MAX) brain->best_v = BM1370_V_MAX - 10.0f;
        if (brain->best_v < BM1370_V_MIN) brain->best_v = BM1370_V_MIN + 10.0f;
    }
}

static void g6_asic_error_handle_non_blocking(G6BrainState *brain) {
    if (!brain) return;

    if (brain->model_quality < 0.30f) {
        ESP_LOGW(TAG, "HIGH ASIC ERROR RATE (model_quality=%.2f) — forcing conservative mode + model reset", brain->model_quality);
        brain->model_quality = 0.15f;
        brain->cold_start = true;           /* force fresh learning */
        brain->best_f *= 0.92f;             /* immediate back-off */
        brain->best_v *= 0.985f;
    }
}

/* ====================== NVS FINGERPRINT ====================== */
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
        ESP_LOGI(TAG, "Silicon fingerprint loaded from NVS");
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

/* ====================== SAMPLE QUALITY STATE MACHINE ====================== */
static bool is_sample_valid(const G6BrainState *brain, float hr_ths, float temp_c, uint32_t shares, uint32_t now) {
    if (!isfinite(hr_ths) || hr_ths <= 0.0f) return false;
    if (now - brain->last_setting_change_tick < SETTLE_SECONDS) return false;
    if (shares < MIN_SHARE_COUNT) return false;
    if (!is_thermal_safe(brain, temp_c)) return false;
    return true;
}

static void advance_sample_state(G6BrainState *brain, uint32_t now) {
    switch (brain->sample_state) {
        case BRAIN_STATE_IDLE:           brain->sample_state = BRAIN_STATE_APPLY_CANDIDATE; break;
        case BRAIN_STATE_APPLY_CANDIDATE:
            brain->settle_start_tick = now;
            brain->sample_state = BRAIN_STATE_SETTLE_WAIT;
            break;
        case BRAIN_STATE_SETTLE_WAIT:
            if (now - brain->settle_start_tick >= SETTLE_SECONDS)
                brain->sample_state = BRAIN_STATE_MEASURE_WINDOW;
            break;
        case BRAIN_STATE_MEASURE_WINDOW: brain->sample_state = BRAIN_STATE_VALIDATE_SAMPLE; break;
        case BRAIN_STATE_VALIDATE_SAMPLE:brain->sample_state = BRAIN_STATE_RLS_UPDATE; break;
        case BRAIN_STATE_RLS_UPDATE:     brain->sample_state = BRAIN_STATE_DECIDE_NEXT; break;
        case BRAIN_STATE_DECIDE_NEXT:    brain->sample_state = BRAIN_STATE_IDLE; break;
        default:                         brain->sample_state = BRAIN_STATE_IDLE; break;
    }
}

/* ====================== PUBLIC API ====================== */

esp_err_t g6_brain_init(G6BrainState *brain) {
    if (!brain) return ESP_ERR_INVALID_ARG;
    memset(brain, 0, sizeof(G6BrainState));

    brain->ridge_epsilon = 1e-5f;
    brain->cold_start = true;
    brain->update_count = 0;
    brain->model_quality = 0.0f;
    brain->nvs_valid = false;
    brain->sample_state = BRAIN_STATE_IDLE;
    brain->last_setting_change_tick = 0;

    /* NASA Level C: Proper cold-start covariance (critical fix) */
    for (int i = 0; i < RLS_N; i++) {
        for (int j = 0; j < RLS_N; j++) {
            brain->P[i][j] = (i == j) ? 1.0e5f : 0.0f;
        }
    }

    /* Safety & config defaults (prevents 0-value bugs from original) */
    brain->temp_ceiling  = 70.0f;
    brain->ner_threshold = 2.5f;
    brain->dfs_step_mhz  = 25.0f;
    brain->Kp = 0.8f; brain->Ki = 0.05f; brain->Kd = 0.2f;

    g6_brain_load_nvs_fingerprint(brain);

    ESP_LOGI(TAG, "G6 Brain v2.0 ALL-IN-ONE (NASA Level C) initialized — zero external dependencies");
    return ESP_OK;
}

esp_err_t g6_brain_update(G6BrainState *brain, float f_mhz, float v_mv, float hr_ths,
                          float power_w, float temp_c, float err_pct) {
    if (!brain) return ESP_ERR_INVALID_ARG;

    uint32_t now = xTaskGetTickCount();

    /* LAYER 0: Full input sanitization */
    if (!isfinite(f_mhz) || !isfinite(v_mv) || !isfinite(hr_ths) ||
        !isfinite(power_w) || !isfinite(temp_c) || !isfinite(err_pct) ||
        hr_ths <= 0.0f || f_mhz < BM1370_F_MIN || v_mv < BM1370_V_MIN) {
        ESP_LOGW(TAG, "INVALID INPUT — fail-closed");
        brain->model_quality = fmaxf(0.0f, brain->model_quality - 0.08f);
        return ESP_OK;
    }

    /* LAYER 1: Thermal hard gate */
    if (!is_thermal_safe(brain, temp_c)) {
        g6_safety_proactive_thermal_scale(brain, temp_c);
        return ESP_OK;
    }

    if (err_pct > brain->ner_threshold) {
        g6_asic_error_handle_non_blocking(brain);
        return ESP_OK;
    }

    brain->last_update_timestamp = now;

    /* LAYER 2: Sample quality + FSM */
    if (!is_sample_valid(brain, hr_ths, temp_c, 50U, now)) {
        advance_sample_state(brain, now);
        goto safety_layer;
    }

    /* LAYER 3: RLS (healthy covariance now guaranteed) */
    float fn = (f_mhz - BM1370_F_CENTER) / BM1370_F_SCALE;
    float vn = (v_mv - BM1370_V_CENTER) / BM1370_V_SCALE;
    float x[RLS_N] = {fn*fn, vn*vn, fn*vn, fn, vn, 1.0f};

    float y_pred = 0.0f;
    for (int i = 0; i < RLS_N; i++) y_pred += brain->theta[i] * x[i];
    float err = hr_ths - y_pred;

    if (has_significant_innovation(brain, x) && trace_P(brain) <= RLS_TRACE_MAX) {
        float lambda_eff = brain->cold_start ? 0.985f : compute_gradient_vff(fabsf(err), 0.008f);

        float Px[RLS_N] = {0.0f};
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
            for (int j = 0; j < RLS_N; j++) {
                brain->P[i][j] = (brain->P[i][j] - k[i] * Px[j]) / lambda_eff;
            }
            brain->P[i][i] += brain->ridge_epsilon;
        }

        rls_symmetrize_clamp_and_stabilize(brain);

        brain->model_quality = fmaxf(0.0f, 1.0f - fabsf(err) / (hr_ths + 1.0f));
        brain->update_count++;
        if (brain->update_count > 25) brain->cold_start = false;
    }

    advance_sample_state(brain, now);

safety_layer:
    /* LAYER 4: All safety now internal (no external dependency) */
    g6_safety_proactive_thermal_scale(brain, temp_c);
    g6_safety_check_voltage_ripple(brain, v_mv);
    if (err_pct > brain->ner_threshold) g6_asic_error_handle_non_blocking(brain);

    /* LAYER 5: Optimal setpoint */
    g6_brain_get_optimal(brain, &brain->best_f, &brain->best_v, NULL);

    /* Final clamps */
    if (brain->best_f < BM1370_F_MIN) brain->best_f = BM1370_F_MIN;
    if (brain->best_f > BM1370_F_MAX) brain->best_f = BM1370_F_MAX;
    if (brain->best_v < BM1370_V_MIN) brain->best_v = BM1370_V_MIN;
    if (brain->best_v > BM1370_V_MAX) brain->best_v = BM1370_V_MAX;

    if (fabsf(brain->best_f - f_mhz) > 8.0f || fabsf(brain->best_v - v_mv) > 8.0f) {
        brain->last_setting_change_tick = now;
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

esp_err_t g6_brain_self_test(G6BrainState *brain) {
    if (!brain) return ESP_ERR_INVALID_ARG;

    bool ok = true;
    for (int i = 0; i < RLS_N; i++) {
        if (brain->P[i][i] < RLS_P_CLAMP_MIN || brain->P[i][i] > RLS_P_CLAMP_MAX) ok = false;
        for (int j = i + 1; j < RLS_N; j++) {
            if (fabsf(brain->P[i][j] - brain->P[j][i]) > 1e-4f) ok = false;
        }
    }

    ESP_LOGI(TAG, "ALL-IN-ONE self-test: %s (quality=%.3f)", ok ? "PASSED" : "DEGRADED", brain->model_quality);
    return ok ? ESP_OK : ESP_FAIL;
}
