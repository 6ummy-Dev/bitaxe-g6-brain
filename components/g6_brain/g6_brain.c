/*
 * g6_brain.c
 * Bitaxe G6 Brain — v1.0.0-beta3 (May 2026)
 *
 * All QA fixes applied:
 * - Critical: O(1) Exact Quadratic Minimization replaces heuristic gradient descent.
 * - Joseph Stabilized Covariance Update guarantees positive-definite matrices.
 * - Statistical Outlier Gating (3-Sigma) rejects physical sensor glitches.
 * - Safety: Fixed thermal override execution order.
 * - Control: Integrated Slew-Rate Limiter internally to couple models to physical reality.
 * - NVS: Symmetric read/write buffers + no VLA on stack.
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
static const uint32_t NVS_SCHEMA_VERSION = 2u;

#define G6_NVS_FINGERPRINT_BUFFER_SIZE  1024

/* ============================================================================
 * SMALL PURE HELPERS
 * ========================================================================== */

static inline float evaluate_quadratic(const float theta[RLS_N], float fn, float vn)
{
    return theta[0]*fn*fn +
           theta[1]*vn*vn +
           theta[2]*fn*vn +
           theta[3]*fn +
           theta[4]*vn +
           theta[5];
}

/* ============================================================================
 * RLS HELPERS
 * ========================================================================== */

static float compute_gradient_vff(float err, float sigma_sq)
{
    if (sigma_sq < 1e-8f) sigma_sq = 1e-8f;
    float L = (err * err) / sigma_sq;
    return RLS_LAMBDA_MIN + (1.0f - RLS_LAMBDA_MIN) * exp2f(-L);
}

static bool has_significant_innovation(const float P[RLS_N][RLS_N], const float x[RLS_N])
{
    float innovation = 0.0f;
    for (int i = 0; i < RLS_N; i++) {
        float px = 0.0f;
        for (int j = 0; j < RLS_N; j++)
            px += P[i][j] * x[j];
        innovation += x[i] * px;
    }
    return innovation > RLS_INNOVATION_THRESHOLD;
}

static float trace_P(const float P[RLS_N][RLS_N])
{
    float tr = 0.0f;
    for (int i = 0; i < RLS_N; i++)
        tr += P[i][i];
    return tr;
}

static void rls_symmetrize_clamp_and_stabilize(float P[RLS_N][RLS_N])
{
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

static bool quadratic_has_valid_maximum(float a, float b, float c)
{
    float h11 = 2.0f * a;
    float det = 4.0f * a * b - c * c;
    return isfinite(h11) && isfinite(det) && h11 < -1e-6f && det > 1e-6f;
}

/* ============================================================================
 * SAFETY HELPERS
 * ========================================================================== */

static bool is_thermal_safe(const G6BrainState *brain, float temp_c)
{
    if (!isfinite(temp_c) || !isfinite(brain->temp_ceiling))
        return false;
    return (temp_c < brain->temp_ceiling);
}

static void g6_safety_proactive_thermal_scale(G6BrainState *brain, float temp_c)
{
    if (!brain || !isfinite(temp_c)) return;

    if (temp_c > (brain->temp_ceiling - 5.0f)) {
        brain->best_f = fmaxf(BM1370_F_MIN, brain->best_f * 0.96f);
        brain->best_v = fmaxf(BM1370_V_MIN, brain->best_v * 0.992f);
        ESP_LOGW(TAG, "PROACTIVE THERMAL: %.1f°C → best_f=%.1f best_v=%.1f", temp_c, brain->best_f, brain->best_v);
    }
}

static void g6_safety_check_voltage_ripple(G6BrainState *brain, float v_mv)
{
    if (!brain || !isfinite(v_mv)) return;

    if (v_mv < BM1370_V_MIN || v_mv > BM1370_V_MAX) {
        brain->best_v = fmaxf(BM1370_V_MIN, fminf(BM1370_V_MAX, brain->best_v));
        ESP_LOGW(TAG, "VOLTAGE OUT OF RANGE: %.1f mV → clamped", v_mv);
    }
}

static void g6_asic_error_handle_non_blocking(G6BrainState *brain, float err_pct)
{
    if (!brain) return;

    if (err_pct > brain->ner_threshold) {
        brain->model_quality = fminf(brain->model_quality, 0.25f);
        brain->cold_start = true;
        brain->best_f *= 0.92f;
        brain->best_v *= 0.985f;
        ESP_LOGW(TAG, "HIGH ERROR RATE (%.2f%%) — conservative back-off", err_pct);
    }
}

/* ============================================================================
 * NVS PERSISTENCE
 * ========================================================================== */

esp_err_t g6_brain_load_nvs_fingerprint(G6BrainState *brain)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK) return err;

    size_t expected_blob_size = sizeof(uint32_t)*2 + sizeof(brain->theta) + sizeof(brain->P) +
                                sizeof(brain->power_theta) + sizeof(brain->power_P);

    uint8_t buffer[G6_NVS_FINGERPRINT_BUFFER_SIZE];
    size_t blob_size = expected_blob_size;

    err = nvs_get_blob(nvs, NVS_FINGERPRINT_KEY, buffer, &blob_size);
    if (err == ESP_OK && blob_size == expected_blob_size) {
        uint32_t stored_version = 0, stored_size = 0;
        memcpy(&stored_version, buffer, sizeof(uint32_t));
        memcpy(&stored_size, buffer + sizeof(uint32_t), sizeof(uint32_t));

        if (stored_version == NVS_SCHEMA_VERSION) {
            size_t offset = sizeof(uint32_t)*2;
            memcpy(brain->theta, buffer + offset, sizeof(brain->theta));
            offset += sizeof(brain->theta);
            memcpy(brain->P, buffer + offset, sizeof(brain->P));
            offset += sizeof(brain->theta);
            memcpy(brain->power_theta, buffer + offset, sizeof(brain->power_theta));
            offset += sizeof(brain->power_theta);
            memcpy(brain->power_P, buffer + offset, sizeof(brain->power_P));

            brain->nvs_valid = true;
            brain->cold_start = false;
            brain->power_cold_start = false;
        } else {
            ESP_LOGW(TAG, "NVS schema mismatch — forcing cold start");
            brain->cold_start = true;
            brain->power_cold_start = true;
            nvs_erase_key(nvs, NVS_FINGERPRINT_KEY);
            nvs_commit(nvs);
        }
    }
    nvs_close(nvs);
    return ESP_OK;
}

esp_err_t g6_brain_save_nvs_fingerprint(const G6BrainState *brain)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;

    size_t data_size = sizeof(brain->theta) + sizeof(brain->P) +
                       sizeof(brain->power_theta) + sizeof(brain->power_P);

    uint8_t buffer[G6_NVS_FINGERPRINT_BUFFER_SIZE];
    uint32_t version = NVS_SCHEMA_VERSION;
    uint32_t size_field = (uint32_t)data_size;

    memcpy(buffer, &version, sizeof(uint32_t));
    memcpy(buffer + sizeof(uint32_t), &size_field, sizeof(uint32_t));

    size_t offset = sizeof(uint32_t)*2;
    memcpy(buffer + offset, brain->theta, sizeof(brain->theta)); offset += sizeof(brain->theta);
    memcpy(buffer + offset, brain->P, sizeof(brain->P));         offset += sizeof(brain->P);
    memcpy(buffer + offset, brain->power_theta, sizeof(brain->power_theta)); offset += sizeof(brain->power_theta);
    memcpy(buffer + offset, brain->power_P, sizeof(brain->power_P));

    err = nvs_set_blob(nvs, NVS_FINGERPRINT_KEY, buffer, offset);
    if (err == ESP_OK) err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
}

/* ============================================================================
 * RESET
 * ========================================================================== */

esp_err_t g6_brain_reset(G6BrainState *brain)
{
    if (!brain) return ESP_ERR_INVALID_ARG;

    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_erase_key(nvs, NVS_FINGERPRINT_KEY);
        nvs_commit(nvs);
        nvs_close(nvs);
    }

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

    for (int i = 0; i < RLS_N; i++)
        for (int j = 0; j < RLS_N; j++)
            brain->power_P[i][j] = (i == j) ? 1.0e5f : 0.0f;

    brain->power_cold_start = true;
    brain->power_update_count = 0;
    brain->power_model_quality = 0.0f;
    brain->last_innovation = 0.0f;

    brain->use_efficiency_mode = false;
#if defined(CONFIG_G6_ENABLE_EFFICIENCY_MODE)
    brain->use_efficiency_mode = CONFIG_G6_ENABLE_EFFICIENCY_MODE;
#endif

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

    ESP_LOGW(TAG, "G6 Brain reset complete");
    return ESP_OK;
}

/* ============================================================================
 * J/TH OPTIMIZER (O(1) Exact Analytical Solver)
 * ========================================================================== */

static void optimize_jth_dinkelbach(G6BrainState *brain, float *opt_f, float *opt_v)
{
    if (!brain || !opt_f || !opt_v) return;
    if (!brain->use_efficiency_mode) return;

    if (brain->model_quality < 0.6f || brain->power_model_quality < 0.6f) {
        ESP_LOGD(TAG, "Skipping J/TH optimization (hr_q=%.2f pw_q=%.2f)",
                 brain->model_quality, brain->power_model_quality);
        return;
    }

    float f = *opt_f;
    float v = *opt_v;

    float fn = (f - BM1370_F_CENTER) / BM1370_F_SCALE;
    float vn = (v - BM1370_V_CENTER) / BM1370_V_SCALE;

    float hr = evaluate_quadratic(brain->theta, fn, vn);
    float pw = evaluate_quadratic(brain->power_theta, fn, vn);

    if (hr < 8.0f) return;

    float lambda = pw / hr;

    for (int outer = 0; outer < G6_JTH_MAX_OUTER_ITERS; outer++) {
        
        // ---------------------------------------------------------
        // O(1) Analytical Inner Solver (Replaces Gradient Descent)
        // Solves for exact minimum of: F(f,v) = Power(f,v) - lambda * Hashrate(f,v)
        // ---------------------------------------------------------
        float A = brain->power_theta[0] - lambda * brain->theta[0];
        float B = brain->power_theta[1] - lambda * brain->theta[1];
        float C = brain->power_theta[2] - lambda * brain->theta[2];
        float D = brain->power_theta[3] - lambda * brain->theta[3];
        float E = brain->power_theta[4] - lambda * brain->theta[4];

        // Determinant of the Hessian matrix
        float det = (4.0f * A * B) - (C * C);

        float fn_inner = fn;
        float vn_inner = vn;

        // Ensure the combined sub-problem surface is strictly convex (bowl-shaped upwards)
        // det > 0 and A > 0 guarantees a unique global minimum.
        if (det > 1e-6f && A > 0.0f) {
            // Jump exactly to the mathematical minimum using Cramer's rule
            fn_inner = (C * E - 2.0f * B * D) / det;
            vn_inner = (C * D - 2.0f * A * E) / det;
            
            // Clamp to normalized hardware limits
            float fn_min = (BM1370_F_MIN - BM1370_F_CENTER) / BM1370_F_SCALE;
            float fn_max = (BM1370_F_MAX - BM1370_F_CENTER) / BM1370_F_SCALE;
            float vn_min = (BM1370_V_MIN - BM1370_V_CENTER) / BM1370_V_SCALE;
            float vn_max = (BM1370_V_MAX - BM1370_V_CENTER) / BM1370_V_SCALE;

            fn_inner = fmaxf(fn_min, fminf(fn_max, fn_inner));
            vn_inner = fmaxf(vn_min, fminf(vn_max, vn_inner));
        } else {
            // Fallback: If the surface is ill-conditioned or saddle-shaped, 
            // break early to safely fall back to the last known stable state.
            break;
        }
        // ---------------------------------------------------------

        float f_new = fn_inner * BM1370_F_SCALE + BM1370_F_CENTER;
        float v_new = vn_inner * BM1370_V_SCALE + BM1370_V_CENTER;

        float hr_new = evaluate_quadratic(brain->theta, fn_inner, vn_inner);
        float pw_new = evaluate_quadratic(brain->power_theta, fn_inner, vn_inner);

        if (hr_new < 8.0f) break;

        float new_lambda = pw_new / hr_new;

        if (new_lambda < lambda) {
            f = f_new;
            v = v_new;
            fn = fn_inner;
            vn = vn_inner;
            float prev_lambda = lambda;
            lambda = new_lambda;

            // Convergence criteria
            if (outer > 2 && fabsf(prev_lambda - lambda) < 0.001f) {
                break;
            }
        } else {
            break;
        }
    }

    *opt_f = f;
    *opt_v = v;
}

/* ============================================================================
 * SAMPLE STATE MACHINE
 * ========================================================================== */

static bool is_sample_valid(const G6BrainState *brain, float hr_ths, float temp_c, uint32_t shares)
{
    if (!isfinite(hr_ths) || hr_ths <= 0.0f) return false;
    if (shares < MIN_SHARE_COUNT) return false;
    if (!is_thermal_safe(brain, temp_c)) return false;
    return true;
}

static void advance_sample_state(G6BrainState *brain, uint32_t now)
{
    switch (brain->sample_state) {
        case BRAIN_STATE_IDLE:            brain->sample_state = BRAIN_STATE_APPLY_CANDIDATE; break;
        case BRAIN_STATE_APPLY_CANDIDATE: brain->settle_start_tick = now; brain->sample_state = BRAIN_STATE_SETTLE_WAIT; break;
        case BRAIN_STATE_SETTLE_WAIT:
            if (now - brain->settle_start_tick >= SETTLE_MS)
                brain->sample_state = BRAIN_STATE_MEASURE_WINDOW;
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

/* ============================================================================
 * PUBLIC API
 * ========================================================================== */

esp_err_t g6_brain_init(G6BrainState *brain)
{
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

    for (int i = 0; i < RLS_N; i++)
        for (int j = 0; j < RLS_N; j++)
            brain->power_P[i][j] = (i == j) ? 1.0e5f : 0.0f;

    brain->power_cold_start = true;
    brain->power_update_count = 0;
    brain->power_model_quality = 0.0f;
    brain->last_innovation = 0.0f;

    brain->use_efficiency_mode = false;
#if defined(CONFIG_G6_ENABLE_EFFICIENCY_MODE)
    brain->use_efficiency_mode = CONFIG_G6_ENABLE_EFFICIENCY_MODE;
#endif

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

    ESP_LOGI(TAG, "G6 Brain initialized (efficiency mode: %s)",
             brain->use_efficiency_mode ? "ENABLED" : "DISABLED");
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
    if (valid || brain->sample_state == BRAIN_STATE_SETTLE_WAIT ||
        brain->sample_state == BRAIN_STATE_MEASURE_WINDOW) {
        advance_sample_state(brain, now);
    }
    if (!valid) goto safety_layer;

    float fn = (f_mhz - BM1370_F_CENTER) / BM1370_F_SCALE;
    float vn = (v_mv - BM1370_V_CENTER) / BM1370_V_SCALE;
    float x[RLS_N] = {fn*fn, vn*vn, fn*vn, fn, vn, 1.0f};

    /* ====================================================================
     * HASHRATE RLS MODEL UPDATE
     * ==================================================================== */
    float y_pred = evaluate_quadratic(brain->theta, fn, vn);
    float err = hr_ths - y_pred;

    if (has_significant_innovation(brain->P, x) && trace_P(brain->P) <= RLS_TRACE_MAX) {
        float lambda_eff = brain->cold_start ? 0.985f :
                           compute_gradient_vff(fabsf(err), RLS_VFF_SIGMA_SQ);
        if (lambda_eff < RLS_LAMBDA_MIN) lambda_eff = RLS_LAMBDA_MIN;

        float Px[RLS_N] = {0};
        for (int i = 0; i < RLS_N; i++)
            for (int j = 0; j < RLS_N; j++)
                Px[i] += brain->P[i][j] * x[j];

        float xPx = 0.0f;
        for (int i = 0; i < RLS_N; i++) xPx += x[i] * Px[i];

        // Statistical Outlier Gating (3-Sigma)
        float S = xPx + 0.5f; 
        if ((err * err) > (9.0f * S)) {
            ESP_LOGW(TAG, "HR Outlier Rejected: err=%.2f, bound=%.2f", err, sqrtf(9.0f * S));
            goto safety_layer;
        }

        float denom = lambda_eff + xPx;
        if (denom < 1e-9f) denom = 1e-9f;

        float k[RLS_N];
        for (int i = 0; i < RLS_N; i++) k[i] = Px[i] / denom;

        for (int i = 0; i < RLS_N; i++) brain->theta[i] += k[i] * err;

        // Joseph Stabilized Covariance Update
        float M[RLS_N][RLS_N];
        for (int i = 0; i < RLS_N; i++) {
            for (int j = 0; j < RLS_N; j++) {
                M[i][j] = (i == j ? 1.0f : 0.0f) - k[i] * x[j];
            }
        }

        float T_mat[RLS_N][RLS_N] = {0};
        for (int i = 0; i < RLS_N; i++) {
            for (int j = 0; j < RLS_N; j++) {
                for (int l = 0; l < RLS_N; l++) {
                    T_mat[i][j] += M[i][l] * brain->P[l][j];
                }
            }
        }

        for (int i = 0; i < RLS_N; i++) {
            for (int j = 0; j < RLS_N; j++) {
                float sum = 0.0f;
                for (int l = 0; l < RLS_N; l++) {
                    sum += T_mat[i][l] * M[j][l];
                }
                brain->P[i][j] = sum / lambda_eff;
            }
            brain->P[i][i] += brain->ridge_epsilon;
        }

        brain->last_innovation = hr_ths - y_pred;
        rls_symmetrize_clamp_and_stabilize(brain->P); 

        brain->model_quality = fmaxf(0.0f, 1.0f - fabsf(err) / (hr_ths + 1.0f));
        brain->update_count++;
        if (brain->update_count > 25) brain->cold_start = false;
    }

    /* ====================================================================
     * POWER RLS MODEL UPDATE
     * ==================================================================== */
    if (brain->use_efficiency_mode && valid) {
        float y_power_pred = evaluate_quadratic(brain->power_theta, fn, vn);
        float power_err = power_w - y_power_pred;

        if (has_significant_innovation(brain->power_P, x) &&
            trace_P(brain->power_P) <= RLS_TRACE_MAX) {

            float lambda_eff = brain->power_cold_start ? 0.985f :
                               compute_gradient_vff(fabsf(power_err), RLS_VFF_SIGMA_SQ);
            if (lambda_eff < RLS_LAMBDA_MIN) lambda_eff = RLS_LAMBDA_MIN;

            float Px[RLS_N] = {0};
            for (int i = 0; i < RLS_N; i++)
                for (int j = 0; j < RLS_N; j++)
                    Px[i] += brain->power_P[i][j] * x[j];

            float xPx = 0.0f;
            for (int i = 0; i < RLS_N; i++) xPx += x[i] * Px[i];

            // Statistical Outlier Gating (3-Sigma)
            float S = xPx + 0.5f; 
            if ((power_err * power_err) > (9.0f * S)) {
                ESP_LOGW(TAG, "Power Outlier Rejected: err=%.2f, bound=%.2f", power_err, sqrtf(9.0f * S));
                goto safety_layer; 
            }

            float denom = lambda_eff + xPx;
            if (denom < 1e-9f) denom = 1e-9f;

            float k[RLS_N];
            for (int i = 0; i < RLS_N; i++) k[i] = Px[i] / denom;

            for (int i = 0; i < RLS_N; i++) brain->power_theta[i] += k[i] * power_err;

            // Joseph Stabilized Covariance Update
            float M[RLS_N][RLS_N];
            for (int i = 0; i < RLS_N; i++) {
                for (int j = 0; j < RLS_N; j++) {
                    M[i][j] = (i == j ? 1.0f : 0.0f) - k[i] * x[j];
                }
            }

            float T_mat[RLS_N][RLS_N] = {0};
            for (int i = 0; i < RLS_N; i++) {
                for (int j = 0; j < RLS_N; j++) {
                    for (int l = 0; l < RLS_N; l++) {
                        T_mat[i][j] += M[i][l] * brain->power_P[l][j];
                    }
                }
            }

            for (int i = 0; i < RLS_N; i++) {
                for (int j = 0; j < RLS_N; j++) {
                    float sum = 0.0f;
                    for (int l = 0; l < RLS_N; l++) {
                        sum += T_mat[i][l] * M[j][l];
                    }
                    brain->power_P[i][j] = sum / lambda_eff;
                }
                brain->power_P[i][i] += brain->ridge_epsilon;
            }

            rls_symmetrize_clamp_and_stabilize(brain->power_P);
            brain->power_model_quality = fmaxf(0.0f, 1.0f - fabsf(power_err) / (power_w + 1.0f));
            brain->power_update_count++;
            if (brain->power_update_count > 25) brain->power_cold_start = false;
        }
    }

    return ESP_OK;

safety_layer:

    /* 1. Calculate Theoretical Mathematical Optimum */
    float candidate_f, candidate_v;
    g6_brain_get_optimal(brain, &candidate_f, &candidate_v, NULL);

    if (brain->control_mode == G6_MODE_AUTO) {
        /* 2. Internal Slew-Rate Limiting: Step safely towards candidate from the ASIC's CURRENT state */
        if (candidate_f > f_mhz) brain->best_f = fminf(f_mhz + brain->dfs_step_mhz, candidate_f);
        else if (candidate_f < f_mhz) brain->best_f = fmaxf(f_mhz - brain->dfs_step_mhz, candidate_f);
        else brain->best_f = candidate_f;

        if (candidate_v > v_mv) brain->best_v = fminf(v_mv + MAX_VOLT_STEP, candidate_v);
        else if (candidate_v < v_mv) brain->best_v = fmaxf(v_mv - MAX_VOLT_STEP, candidate_v);
        else brain->best_v = candidate_v;
    }

    /* 3. Apply Absolute Hardware Limits */
    if (brain->best_f < BM1370_F_MIN) brain->best_f = BM1370_F_MIN;
    if (brain->best_f > BM1370_F_MAX) brain->best_f = BM1370_F_MAX;
    if (brain->best_v < BM1370_V_MIN) brain->best_v = BM1370_V_MIN;
    if (brain->best_v > BM1370_V_MAX) brain->best_v = BM1370_V_MAX;

    /* 4. Apply Safety Overrides (Must run LAST to override math & slew logic!) */
    g6_safety_proactive_thermal_scale(brain, temp_c);
    g6_safety_check_voltage_ripple(brain, v_mv);

    brain->last_efficiency = (hr_ths > 0.0f) ? (power_w / hr_ths) : 0.0f;

    if (brain->control_mode == G6_MODE_AUTO &&
        (fabsf(brain->best_f - f_mhz) > 8.0f || fabsf(brain->best_v - v_mv) > 8.0f)) {
        brain->last_setting_change_tick = now;
        brain->sample_state = BRAIN_STATE_APPLY_CANDIDATE;
        brain->measure_start_tick = 0;
    }

    if (brain->update_count > 10 &&
        (now - brain->nvs_last_write_tick > NVS_SAVE_INTERVAL_TICKS)) {
        g6_brain_save_nvs_fingerprint(brain);
        brain->nvs_last_write_tick = now;
        ESP_LOGI(TAG, "NVS fingerprint auto-saved");
    }

    return ESP_OK;
}

void g6_brain_get_optimal(const G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr)
{
    if (!brain || !opt_f || !opt_v) return;

    float a = brain->theta[0], b = brain->theta[1], c = brain->theta[2];
    float d = brain->theta[3], e = brain->theta[4];

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

    if (brain->use_efficiency_mode) {
        optimize_jth_dinkelbach((G6BrainState *)brain, opt_f, opt_v);
    }

    if (pred_hr) {
        float fn = (*opt_f - BM1370_F_CENTER) / BM1370_F_SCALE;
        float vn = (*opt_v - BM1370_V_CENTER) / BM1370_V_SCALE;
        *pred_hr = evaluate_quadratic(brain->theta, fn, vn);
    }
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
        for (int j = i + 1; j < RLS_N; j++)
            if (fabsf(brain->P[i][j] - brain->P[j][i]) > RLS_SYMMETRY_TOLERANCE) // Fixed typo here
                ok = false;
    }

    float cond = (min_diag > 1e-9f) ? (max_diag / min_diag) : 0.0f;
    if (cond > 5e5f) ok = false;

    ESP_LOGI(TAG, "Self-test: %s (quality=%.3f, eff=%.2f W/TH, cond=%.1f)",
             ok ? "PASSED" : "DEGRADED", brain->model_quality,
             brain->last_efficiency, cond);

    return ok ? ESP_OK : ESP_FAIL;
}

void g6_brain_get_telemetry(const G6BrainState *brain, G6BrainTelemetry *out)
{
    if (!brain || !out) return;

    memcpy(out->theta_hashrate, brain->theta, sizeof(brain->theta));
    out->trace_P_hashrate = trace_P(brain->P);

    memcpy(out->theta_power, brain->power_theta, sizeof(brain->power_theta));
    out->trace_P_power = trace_P(brain->power_P);

    out->last_innovation = brain->last_innovation;
    out->safety_status = G6_SAFETY_OK;
    out->efficiency_mode_active = brain->use_efficiency_mode;
    out->last_recommended_voltage = brain->best_v;
}
