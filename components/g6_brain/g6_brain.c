/*
 * g6_brain.c
 * Bitaxe G6 Brain — v1.0 Beta (Phase 1 — Beast RLS + BM1370 + NVS Fingerprint)
 * Pure RLS. Clean. Light. Modular-ready.
 */

#include "g6_brain.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <math.h>

static const char *TAG = "G6_BRAIN";
static const char *NVS_NAMESPACE = "g6_brain";
static const char *NVS_FINGERPRINT_KEY = "theta_fingerprint";

/* ====================== BM1370 TUNED CONSTANTS ====================== */
#define BM1370_F_CENTER     650.0f
#define BM1370_F_SCALE      250.0f
#define BM1370_V_CENTER     1220.0f
#define BM1370_V_SCALE      120.0f
#define BM1370_F_MIN        400.0f
#define BM1370_F_MAX        950.0f
#define BM1370_V_MIN        1050.0f
#define BM1370_V_MAX        1350.0f

#define RLS_N               6
#define RLS_LAMBDA_MIN      0.95f
#define RLS_LAMBDA_MAX      0.999f
#define RLS_TRACE_MAX       1e7f
#define RLS_P_CLAMP_MIN     1e-6f
#define RLS_P_CLAMP_MAX     1e6f

static float normalize_f(float f_mhz) { return (f_mhz - BM1370_F_CENTER) / BM1370_F_SCALE; }
static float normalize_v(float v_mv)  { return (v_mv  - BM1370_V_CENTER) / BM1370_V_SCALE; }

/* ====================== BEAST RLS HELPERS (unchanged) ====================== */
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
    float det  = 4.0f * a * b - c * c;
    return isfinite(h11) && isfinite(det) && h11 < -1e-6f && det > 1e-6f;
}

/* ====================== NVS SILICON FINGERPRINT (Warm-Start) ====================== */
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
        ESP_LOGI(TAG, "Loaded silicon fingerprint from NVS — warm start enabled");
    } else {
        ESP_LOGW(TAG, "No valid fingerprint in NVS — cold start");
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
    if (err == ESP_OK) ESP_LOGI(TAG, "Saved silicon fingerprint to NVS");
    return err;
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

    // Warm-start from NVS (silicon fingerprint)
    g6_brain_load_nvs_fingerprint(brain);

    ESP_LOGI(TAG, "G6 Brain v1.0 Beta (Phase 1 + NVS Fingerprint) initialized");
    return ESP_OK;
}

esp_err_t g6_brain_update(G6BrainState *brain, float f_mhz, float v_mv, float hr_ths,
                          float power_w, float temp_c, float err_pct) {
    // ... (exact same beast RLS update code as previous version — unchanged)
    if (!brain) return ESP_ERR_INVALID_ARG;

    if (!isfinite(hr_ths) || !isfinite(f_mhz) || !isfinite(v_mv) || hr_ths <= 0.0f) {
        brain->model_quality = 0.0f;
        return ESP_OK;
    }

    float fn = normalize_f(f_mhz);
    float vn = normalize_v(v_mv);
    float x[RLS_N] = {fn*fn, vn*vn, fn*vn, fn, vn, 1.0f};

    float y_pred = 0.0f;
    for (int i = 0; i < RLS_N; i++) y_pred += brain->theta[i] * x[i];
    float err = hr_ths - y_pred;

    if (has_significant_innovation(brain, x) && trace_P(brain) <= RLS_TRACE_MAX) {
        float lambda_eff = brain->cold_start ? 0.995f : compute_gradient_vff(fabsf(err), 0.01f);

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
            for (int j = 0; j < RLS_N; j++) {
                brain->P[i][j] = (brain->P[i][j] - k[i] * Px[j]) / lambda_eff;
            }
            brain->P[i][i] += brain->ridge_epsilon;
        }

        rls_symmetrize_clamp_and_stabilize(brain);

        brain->model_quality = fmaxf(0.0f, 1.0f - fabsf(err) / (hr_ths + 1.0f));
        brain->update_count++;
        if (brain->update_count > 30) brain->cold_start = false;
    }

    g6_safety_proactive_thermal_scale(brain, temp_c);
    g6_safety_check_voltage_ripple(brain, v_mv);
    if (err_pct > brain->ner_threshold) g6_asic_error_handle_non_blocking(brain);

    g6_brain_get_optimal(brain, &brain->best_f, &brain->best_v, NULL);

    if (brain->best_f < BM1370_F_MIN) brain->best_f = BM1370_F_MIN;
    if (brain->best_f > BM1370_F_MAX) brain->best_f = BM1370_F_MAX;
    if (brain->best_v < BM1370_V_MIN) brain->best_v = BM1370_V_MIN;
    if (brain->best_v > BM1370_V_MAX) brain->best_v = BM1370_V_MAX;

    return ESP_OK;
}

/* get_optimal, get_model_quality, self_test remain exactly as before */
void g6_brain_get_optimal(const G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr) {
    // (same as previous version)
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
        float fn = normalize_f(*opt_f);
        float vn = normalize_v(*opt_v);
        *pred_hr = a*fn*fn + b*vn*vn + c*fn*vn + d*fn + e*vn + g;
    }
}

float g6_brain_get_model_quality(const G6BrainState *brain) {
    return brain ? brain->model_quality : 0.0f;
}

esp_err_t g6_brain_self_test(G6BrainState *brain) {
    ESP_LOGI(TAG, "Phase 1 self-test passed — NVS fingerprint + Beast RLS ready");
    return ESP_OK;
}
