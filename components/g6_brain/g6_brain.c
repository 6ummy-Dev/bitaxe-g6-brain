/*
 * g6_brain.c
 * Bitaxe G6 Brain — v1.0.0-beta2 (QA Hardened v2)
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

/* ====================== LOCAL DEFINES ====================== */
#define RLS_P0_DIAG 1.0f

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

/* ====================== PUBLIC API ====================== */

esp_err_t g6_brain_init(G6BrainState *brain)
{
    if (!brain) return ESP_ERR_INVALID_ARG;

    memset(brain, 0, sizeof(G6BrainState));

    for (int i = 0; i < RLS_N; i++) {
        for (int j = 0; j < RLS_N; j++) {
            brain->P[i][j] = (i == j) ? RLS_P0_DIAG : 0.0f;
        }
        brain->theta[i] = 0.0f;
    }

    brain->model_quality = 0.0f;
    brain->last_efficiency = 0.0f;
    brain->cold_start = true;
    brain->last_update_timestamp = xTaskGetTickCount();

    ESP_LOGI(TAG, "G6 Brain initialized (RLS stabilized)");
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

    uint32_t now = xTaskGetTickCount();
    brain->last_update_timestamp = now;

    // Feature vector for RLS (quadratic model)
    float x[RLS_N] = {
        1.0f,
        f_mhz,
        v_mv,
        f_mhz * v_mv,
        f_mhz * f_mhz,
        v_mv * v_mv
    };

    float y = hr_ths;

    float err = y;
    for (int i = 0; i < RLS_N; i++) {
        err -= brain->theta[i] * x[i];
    }

    float lambda = compute_gradient_vff(err, 1.0f);

    // Basic efficiency tracking
    if (power_w > 0.1f) {
        brain->last_efficiency = hr_ths / power_w;
    }

    // TODO: Full RLS matrix update + safety logic will go here in next iteration
    (void)lambda;
    (void)share_count;
    (void)temp_c;
    (void)err_pct;

    return ESP_OK;
}

void g6_brain_get_optimal(const G6BrainState *brain,
                          float *opt_f,
                          float *opt_v,
                          float *pred_hr)
{
    if (!brain || !opt_f || !opt_v || !pred_hr) return;

    *opt_f = brain->best_f;
    *opt_v = brain->best_v;

    // Quadratic prediction using current theta
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

float g6_brain_get_model_quality(const G6BrainState *brain)
{
    return brain ? brain->model_quality : 0.0f;
}

float g6_brain_get_cov_condition(const G6BrainState *brain)
{
    if (!brain) return 0.0f;

    float min_diag = 1e30f, max_diag = 0.0f;
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
    float min_diag = 1e30f, max_diag = 0.0f;

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

    ESP_LOGI(TAG, "Self-test: %s (q=%.3f, eff=%.2f, cond=%.1f)",
             ok ? "PASS" : "DEGRADED",
             brain->model_quality,
             brain->last_efficiency,
             cond);

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
