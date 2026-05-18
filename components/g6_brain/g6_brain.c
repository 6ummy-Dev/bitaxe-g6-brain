/*
 * g6_brain.c
 * Bitaxe G6 Brain — v1.0.0-beta2 Final (Hardened)
 */

#include "g6_brain.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>
#include <math.h>

static const char *TAG = "G6_BRAIN";

/* ====================== RLS HELPERS ====================== */

static float compute_gradient_vff(float err, float sigma_sq)
{
    if (sigma_sq < 1e-8f) sigma_sq = 1e-8f;
    float L = (err * err) / sigma_sq;
    return RLS_LAMBDA_MIN + (1.0f - RLS_LAMBDA_MIN) * powf(2.0f, -L);
}

static bool has_significant_innovation(const G6BrainState *brain, const float x[RLS_N])
{
    float innovation = 0.0f;
    for (int i = 0; i < RLS_N; i++) {
        float px = 0.0f;
        for (int j = 0; j < RLS_N; j++) {
            px += brain->P[i][j] * x[j];
        }
        innovation += x[i] * px;
    }
    return innovation > 1e-4f;
}

static float trace_P(const G6BrainState *brain)
{
    float tr = 0.0f;
    for (int i = 0; i < RLS_N; i++) {
        tr += brain->P[i][i];
    }
    return tr;
}

static void rls_symmetrize_clamp_and_stabilize(G6BrainState *brain)
{
    for (int i = 0; i < RLS_N; i++) {
        for (int j = i + 1; j < RLS_N; j++) {
            float s = 0.5f * (brain->P[i][j] + brain->P[j][i]);
            brain->P[i][j] = s;
            brain->P[j][i] = s;
        }
    }

    for (int i = 0; i < RLS_N; i++) {
        if (brain->P[i][i] < RLS_P_CLAMP_MIN) brain->P[i][i] = RLS_P_CLAMP_MIN;
        if (brain->P[i][i] > RLS_P_CLAMP_MAX) brain->P[i][i] = RLS_P_CLAMP_MAX;
    }
}

/* ====================== SAFETY LAYER ====================== */

static void safety_layer(G6BrainState *brain, float temp_c, float power_w, float err_pct)
{
    if (!brain) return;

    /* Thermal protection */
    if (temp_c > brain->temp_ceiling) {
        brain->best_f *= 0.98f;
        if (brain->best_f < BM1370_F_MIN) brain->best_f = BM1370_F_MIN;
    }

    /* NER backoff */
    if (err_pct > brain->ner_threshold) {
        brain->best_f *= 0.97f;
        if (brain->best_f < BM1370_F_MIN) brain->best_f = BM1370_F_MIN;
    }

    /* Power sanity */
    if (power_w < 5.0f || power_w > 35.0f) {
        brain->best_f = 650.0f;
        brain->best_v = 1220.0f;
    }
}

/* ====================== PUBLIC API ====================== */

esp_err_t g6_brain_init(G6BrainState *brain)
{
    if (!brain) return ESP_ERR_INVALID_ARG;

    memset(brain, 0, sizeof(G6BrainState));

    for (int i = 0; i < RLS_N; i++) {
        for (int j = 0; j < RLS_N; j++) {
            brain->P[i][j] = (i == j) ? 1.0f : 0.0f;
        }
        brain->theta[i] = 0.0f;
    }

    brain->ridge_epsilon     = 1e-6f;
    brain->model_quality     = 0.0f;
    brain->last_efficiency   = 0.0f;
    brain->cold_start        = true;
    brain->last_update_timestamp = xTaskGetTickCount();

    /* Safe cold-start defaults */
    brain->best_f = 650.0f;
    brain->best_v = 1220.0f;

    brain->temp_ceiling   = 70.0f;
    brain->ner_threshold  = 2.5f;

    ESP_LOGI(TAG, "G6 Brain initialized (beta2 hardened)");
    return ESP_OK;
}

esp_err_t g6_brain_update(G6BrainState *brain,
                          float f_mhz,
                          float v_mv,
                          float hr_ths,
                          float power_w,
                          float temp_c,
                          float err_pct,
                          uint32_t share_count)
{
    if (!brain) return ESP_ERR_INVALID_ARG;

    /* Input validation */
    if (!isfinite(f_mhz) || !isfinite(v_mv) || !isfinite(hr_ths) ||
        !isfinite(power_w) || !isfinite(temp_c) || !isfinite(err_pct)) {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t now = xTaskGetTickCount();
    float dt = (now - brain->last_update_timestamp) * portTICK_PERIOD_MS / 1000.0f;

    if (dt < 2.0f) {
        goto safety_layer;
    }

    brain->last_update_timestamp = now;

    /* Share count validation */
    if (share_count > 0 && share_count < 20) {
        goto safety_layer;
    }

    /* Feature vector */
    float x[RLS_N] = {
        1.0f,
        f_mhz,
        v_mv,
        f_mhz * v_mv,
        f_mhz * f_mhz,
        v_mv * v_mv
    };

    float y = hr_ths;

    float y_hat = 0.0f;
    for (int i = 0; i < RLS_N; i++) {
        y_hat += brain->theta[i] * x[i];
    }
    float err = y - y_hat;

    float lambda = compute_gradient_vff(err, 1.0f);

    if (!has_significant_innovation(brain, x)) {
        goto safety_layer;
    }

    float Px[RLS_N] = {0};
    for (int i = 0; i < RLS_N; i++) {
        for (int j = 0; j < RLS_N; j++) {
            Px[i] += brain->P[i][j] * x[j];
        }
    }

    float denom = lambda;
    for (int i = 0; i < RLS_N; i++) {
        denom += x[i] * Px[i];
    }
    if (denom < 1e-8f) denom = 1e-8f;

    float K[RLS_N];
    for (int i = 0; i < RLS_N; i++) {
        K[i] = Px[i] / denom;
    }

    for (int i = 0; i < RLS_N; i++) {
        brain->theta[i] += K[i] * err;
    }

    for (int i = 0; i < RLS_N; i++) {
        for (int j = 0; j < RLS_N; j++) {
            brain->P[i][j] = (brain->P[i][j] - K[i] * Px[j]) / lambda;
        }
    }

    rls_symmetrize_clamp_and_stabilize(brain);

    /* Update best operating point */
    if (power_w > 0.1f) {
        float efficiency = hr_ths / power_w;
        if (efficiency > brain->last_efficiency) {
            brain->best_f = f_mhz;
            brain->best_v = v_mv;
            brain->last_efficiency = efficiency;
        }
    }

    brain->update_count++;
    brain->cold_start = false;

safety_layer:
    safety_layer(brain, temp_c, power_w, err_pct);
    return ESP_OK;
}

void g6_brain_get_optimal(const G6BrainState *brain,
                          float *opt_f,
                          float *opt_v,
                          float *pred_hr)
{
    if (!brain || !opt_f || !opt_v) return;

    *opt_f = brain->best_f;
    *opt_v = brain->best_v;

    if (pred_hr) {
        float fn = (*opt_f - BM1370_F_CENTER) / BM1370_F_SCALE;
        float vn = (*opt_v - BM1370_V_CENTER) / BM1370_V_SCALE;

        float a = brain->theta[4];
        float b = brain->theta[5];
        float c = brain->theta[3];
        float d = brain->theta[1];
        float e = brain->theta[2];
        float g = brain->theta[0];

        *pred_hr = a*fn*fn + b*vn*vn + c*fn*vn + d*fn + e*vn + g;
    }
}

float g6_brain_get_model_quality(const G6BrainState *brain)
{
    return brain ? brain->model_quality : 0.0f;
}

float g6_brain_get_cov_condition(const G6BrainState *brain)
{
    if (!brain) return 0.0f;

    float min_diag = 1e30f;
    float max_diag = 0.0f;

    for (int i = 0; i < RLS_N; i++) {
        if (brain->P[i][i] < min_diag) min_diag = brain->P[i][i];
        if (brain->P[i][i] > max_diag) max_diag = brain->P[i][i];
    }

    return (min_diag > 1e-9f) ? (max_diag / min_diag) : 0.0f;
}

esp_err_t g6_brain_self_test(G6BrainState *brain)
{
    if (!brain) return ESP_ERR_INVALID_ARG;

    bool ok = true;
    float min_diag = 1e30f;
    float max_diag = 0.0f;

    for (int i = 0; i < RLS_N; i++) {
        if (brain->P[i][i] < RLS_P_CLAMP_MIN || brain->P[i][i] > RLS_P_CLAMP_MAX)
            ok = false;

        if (brain->P[i][i] < min_diag) min_diag = brain->P[i][i];
        if (brain->P[i][i] > max_diag) max_diag = brain->P[i][i];

        for (int j = i + 1; j < RLS_N; j++) {
            if (fabsf(brain->P[i][j] - brain->P[j][i]) > 1e-4f)
                ok = false;
        }
    }

    float cond = (min_diag > 1e-9f) ? (max_diag / min_diag) : 0.0f;
    if (cond > 5e5f) ok = false;

    ESP_LOGI(TAG, "Self-test: %s (updates=%lu)", ok ? "PASS" : "DEGRADED", (unsigned long)brain->update_count);
    return ok ? ESP_OK : ESP_FAIL;
}

/* ====================== NVS STUBS ====================== */

esp_err_t g6_brain_load_nvs_fingerprint(G6BrainState *brain)
{
    if (!brain) return ESP_ERR_INVALID_ARG;
    brain->nvs_valid = false;
    return ESP_OK;
}

esp_err_t g6_brain_save_nvs_fingerprint(const G6BrainState *brain)
{
    if (!brain) return ESP_ERR_INVALID_ARG;
    return ESP_OK;
}
