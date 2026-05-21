/*
 * g6_brain.c
 * Bitaxe G6 Brain — v1.0.0-beta5
 */

#include "g6_brain.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include <string.h>
#include <math.h>
#include <inttypes.h>

static const char *TAG = "G6_BRAIN";
static const char *NVS_NAMESPACE = "g6_brain";
static const char *NVS_FINGERPRINT_KEY = "theta_fingerprint";
static const uint32_t NVS_SAVE_INTERVAL_TICKS = 300000UL;
#define G6_NVS_FINGERPRINT_BUFFER_SIZE 1024

static inline float evaluate_quadratic(const float theta[RLS_N], float fn, float vn)
{
    return theta[0]*fn*fn + theta[1]*vn*vn + theta[2]*fn*vn +
           theta[3]*fn + theta[4]*vn + theta[5];
}

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
        for (int j = 0; j < RLS_N; j++) px += P[i][j] * x[j];
        innovation += x[i] * px;
    }
    return innovation > RLS_INNOVATION_THRESHOLD;
}

static float trace_P(const float P[RLS_N][RLS_N])
{
    float tr = 0.0f;
    for (int i = 0; i < RLS_N; i++) tr += P[i][i];
    return tr;
}

static void rls_symmetrize_clamp_and_stabilize(float P[RLS_N][RLS_N])
{
    for (int i = 0; i < RLS_N; i++) {
        for (int j = i + 1; j < RLS_N; j++) {
            float s = 0.5f * (P[i][j] + P[j][i]);
            P[i][j] = s; P[j][i] = s;
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

static bool is_thermal_safe(const G6BrainState *brain, float temp_c)
{
    if (!isfinite(temp_c) || !isfinite(brain->temp_ceiling)) return false;
    return (temp_c < brain->temp_ceiling);
}

static void g6_safety_proactive_thermal_scale(G6BrainState *brain, float temp_c)
{
    if (!brain || !isfinite(temp_c)) return;
    if (temp_c > (brain->temp_ceiling - brain->temp_proactive_margin)) {
        brain->best_f = fmaxf(BM1370_F_MIN, brain->best_f * 0.96f);
        brain->best_v = fmaxf(BM1370_V_MIN, brain->best_v * 0.992f);
        brain->last_safety_status = G6_SAFETY_THERMAL;
    }
}

static void g6_safety_check_voltage_ripple(G6BrainState *brain, float v_mv)
{
    if (!brain || !isfinite(v_mv)) return;
    if (v_mv < BM1370_V_MIN || v_mv > BM1370_V_MAX) {
        brain->best_v = fmaxf(BM1370_V_MIN, fminf(BM1370_V_MAX, brain->best_v));
        brain->last_safety_status = G6_SAFETY_VOLTAGE;
    }
}

static void g6_safety_proactive_vr_thermal_scale(G6BrainState *brain, float vr_temp_c)
{
    if (!brain) return;
    /* G6_VR_TEMP_NO_SENSOR (-1.0f) and any negative value disables VR monitoring.
     * Use an explicit sentinel check rather than a bare < 0.0f to avoid silently
     * disabling protection on a glitched sensor returning e.g. -0.5°C. */
    if (vr_temp_c <= G6_VR_TEMP_NO_SENSOR || !isfinite(vr_temp_c)) return;
    if (!isfinite(brain->vr_temp_ceiling) || brain->vr_temp_ceiling <= 0.0f) return;

    float proactive_threshold = brain->vr_temp_ceiling - brain->vr_temp_proactive_margin;

    if (vr_temp_c >= brain->vr_temp_ceiling) {
        brain->best_v = fmaxf(BM1370_V_MIN, brain->best_v * 0.985f);
        brain->best_f = fmaxf(BM1370_F_MIN, brain->best_f * 0.96f);
        brain->last_safety_status = G6_SAFETY_VR_THERMAL;
    } else if (vr_temp_c > proactive_threshold) {
        brain->best_v = fmaxf(BM1370_V_MIN, brain->best_v * 0.992f);
        brain->last_safety_status = G6_SAFETY_VR_THERMAL;
    }
}

static void g6_asic_error_handle_non_blocking(G6BrainState *brain, float err_pct)
{
    if (!brain) return;
    if (err_pct > brain->ner_threshold) {
        brain->model_quality = fminf(brain->model_quality, 0.25f);
        brain->cold_start = true;
        brain->best_f = fmaxf(BM1370_F_MIN, brain->best_f * 0.92f);
        brain->best_v = fmaxf(BM1370_V_MIN, brain->best_v * 0.985f);
        brain->last_safety_status = G6_SAFETY_NER_BACKOFF;
    }
}

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
        uint32_t stored_version = 0;
        memcpy(&stored_version, buffer, sizeof(uint32_t));

        if (stored_version == G6_NVS_SCHEMA_VERSION) {
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
        } else {
            brain->cold_start = true;
            brain->power_cold_start = true;
            nvs_erase_key(nvs, NVS_FINGERPRINT_KEY);
            nvs_commit(nvs);
        }
    } else if (err == ESP_OK && blob_size != expected_blob_size) {
        /* Blob exists but is the wrong size — schema corruption or partial write.
         * Erase it so we don't silently carry a bad blob across reboots. */
        ESP_LOGW(TAG, "NVS blob size mismatch (got %u, expected %u) — erasing",
                 (unsigned)blob_size, (unsigned)expected_blob_size);
        brain->cold_start = true;
        brain->power_cold_start = true;
        nvs_erase_key(nvs, NVS_FINGERPRINT_KEY);
        nvs_commit(nvs);
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
    uint32_t version = G6_NVS_SCHEMA_VERSION;
    uint32_t size_field = (uint32_t)data_size;

    memcpy(buffer, &version, sizeof(uint32_t));
    memcpy(buffer + sizeof(uint32_t), &size_field, sizeof(uint32_t));

    size_t offset = sizeof(uint32_t)*2;
    memcpy(buffer + offset, brain->theta, sizeof(brain->theta)); offset += sizeof(brain->theta);
    memcpy(buffer + offset, brain->P, sizeof(brain->P)); offset += sizeof(brain->P);
    memcpy(buffer + offset, brain->power_theta, sizeof(brain->power_theta)); offset += sizeof(brain->power_theta);
    memcpy(buffer + offset, brain->power_P, sizeof(brain->power_P)); offset += sizeof(brain->power_P);

    err = nvs_set_blob(nvs, NVS_FINGERPRINT_KEY, buffer, offset);
    if (err == ESP_OK) err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
}

/* Shared default-state initialization. Called from both g6_brain_init and
 * g6_brain_reset. Assumes the caller has already memset the struct to zero. */
static void g6_brain_set_defaults(G6BrainState *brain)
{
    brain->best_f = BM1370_F_CENTER;
    brain->best_v = BM1370_V_CENTER;
    brain->ridge_epsilon = RLS_RIDGE_EPSILON;
    brain->cold_start = true;
    brain->update_count = 0;
    brain->model_quality = 0.0f;
    brain->nvs_valid = false;
    brain->sample_state = BRAIN_STATE_IDLE;
    brain->control_mode = G6_MODE_RECOMMEND;
    brain->last_safety_status = G6_SAFETY_OK;

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
    brain->temp_proactive_margin = G6_TEMP_PROACTIVE_MARGIN_DEFAULT;
    brain->vr_temp_ceiling = G6_VR_TEMP_CEILING_DEFAULT;
    brain->vr_temp_proactive_margin = G6_VR_TEMP_PROACTIVE_MARGIN_DEFAULT;

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
}

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
    g6_brain_set_defaults(brain);
    return ESP_OK;
}

static void optimize_jth_dinkelbach(const G6BrainState *brain, float *opt_f, float *opt_v)
{
    if (!brain || !opt_f || !opt_v) return;
    if (!brain->use_efficiency_mode) return;
    if (brain->model_quality < 0.6f || brain->power_model_quality < 0.6f) return;

    float f = *opt_f;
    float v = *opt_v;
    float fn = (f - BM1370_F_CENTER) / BM1370_F_SCALE;
    float vn = (v - BM1370_V_CENTER) / BM1370_V_SCALE;

    float hr = evaluate_quadratic(brain->theta, fn, vn);
    float pw = evaluate_quadratic(brain->power_theta, fn, vn);
    if (hr < 8.0f) return;

    float lambda = pw / hr;

    for (int outer = 0; outer < G6_JTH_MAX_OUTER_ITERS; outer++) {
        float A = brain->power_theta[0] - lambda * brain->theta[0];
        float B = brain->power_theta[1] - lambda * brain->theta[1];
        float C = brain->power_theta[2] - lambda * brain->theta[2];
        float D = brain->power_theta[3] - lambda * brain->theta[3];
        float E = brain->power_theta[4] - lambda * brain->theta[4];

        float det = (4.0f * A * B) - (C * C);
        float fn_inner = fn;
        float vn_inner = vn;

        if (det > 1e-6f && A > 0.0f) {
            fn_inner = (C * E - 2.0f * B * D) / det;
            vn_inner = (C * D - 2.0f * A * E) / det;

            float fn_min = (BM1370_F_MIN - BM1370_F_CENTER) / BM1370_F_SCALE;
            float fn_max = (BM1370_F_MAX - BM1370_F_CENTER) / BM1370_F_SCALE;
            float vn_min = (BM1370_V_MIN - BM1370_V_CENTER) / BM1370_V_SCALE;
            float vn_max = (BM1370_V_MAX - BM1370_V_CENTER) / BM1370_V_SCALE;

            fn_inner = fmaxf(fn_min, fminf(fn_max, fn_inner));
            vn_inner = fmaxf(vn_min, fminf(vn_max, vn_inner));
        } else {
            break;
        }

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
            if (outer > 2 && fabsf(prev_lambda - lambda) < 0.001f) break;
        } else {
            break;
        }
    }

    *opt_f = f;
    *opt_v = v;
}

/* Returns true if the sample is fit for an RLS update.
 * Input finiteness and hr_ths > 0 are already enforced by g6_brain_update's
 * early validation; those checks are intentionally omitted here to avoid
 * dead code. NER is gated here as defense-in-depth even though the early
 * goto in g6_brain_update handles the side-effects (backoff, cold_start). */
static bool is_sample_valid(const G6BrainState *brain, float hr_ths,
                            float temp_c, float err_pct, uint32_t shares)
{
    if (err_pct > brain->ner_threshold) return false;
    if (shares < MIN_SHARE_COUNT) return false;
    if (!is_thermal_safe(brain, temp_c)) return false;
    (void)hr_ths; /* validated upstream; kept in signature for call-site clarity */
    return true;
}

static void advance_sample_state(G6BrainState *brain, uint32_t now)
{
    switch (brain->sample_state) {
        case BRAIN_STATE_IDLE:
            brain->sample_state = BRAIN_STATE_APPLY_CANDIDATE;
            break;
        case BRAIN_STATE_APPLY_CANDIDATE:
            brain->settle_start_tick = now;
            brain->sample_state = BRAIN_STATE_SETTLE_WAIT;
            break;
        case BRAIN_STATE_SETTLE_WAIT:
            if ((now - brain->settle_start_tick) * portTICK_PERIOD_MS >= SETTLE_MS)
                brain->sample_state = BRAIN_STATE_MEASURE_WINDOW;
            break;
        case BRAIN_STATE_MEASURE_WINDOW:
            if (brain->measure_start_tick == 0) brain->measure_start_tick = now;
            if ((now - brain->measure_start_tick) * portTICK_PERIOD_MS >= MIN_WINDOW_MS) {
                brain->sample_state = BRAIN_STATE_VALIDATE_SAMPLE;
                brain->measure_start_tick = 0;
            }
            break;
        case BRAIN_STATE_VALIDATE_SAMPLE:
            brain->sample_state = BRAIN_STATE_RLS_UPDATE;
            break;
        case BRAIN_STATE_RLS_UPDATE:
            brain->sample_state = BRAIN_STATE_DECIDE_NEXT;
            break;
        case BRAIN_STATE_DECIDE_NEXT:
            brain->sample_state = BRAIN_STATE_IDLE;
            break;
        default:
            brain->sample_state = BRAIN_STATE_IDLE;
            break;
    }
}

esp_err_t g6_brain_init(G6BrainState *brain)
{
    if (!brain) return ESP_ERR_INVALID_ARG;

    nvs_handle_t nvs_test;
    esp_err_t nvs_err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_test);
    if (nvs_err == ESP_ERR_NVS_NOT_INITIALIZED) return ESP_ERR_NVS_NOT_INITIALIZED;
    if (nvs_err == ESP_OK) nvs_close(nvs_test);

    memset(brain, 0, sizeof(G6BrainState));
    g6_brain_set_defaults(brain);
    g6_brain_load_nvs_fingerprint(brain);
    return ESP_OK;
}

esp_err_t g6_brain_update(G6BrainState *brain,
                          float f_mhz, float v_mv, float hr_ths,
                          float power_w, float temp_c, float vr_temp_c,
                          float err_pct, uint32_t share_count)
{
    if (!brain) return ESP_ERR_INVALID_ARG;

    uint32_t now = xTaskGetTickCount();

    if (!isfinite(f_mhz) || !isfinite(v_mv) || !isfinite(hr_ths) ||
        !isfinite(power_w) || !isfinite(temp_c) || !isfinite(err_pct) ||
        hr_ths <= 0.0f || f_mhz < BM1370_F_MIN || v_mv < BM1370_V_MIN) {
        return ESP_ERR_INVALID_ARG;
    }

    if (power_w < 0.0f || power_w > 100.0f) {
        brain->last_safety_status = G6_SAFETY_POWER_SANITY;
        goto safety_layer;
    }

    brain->last_safety_status = G6_SAFETY_OK;

    if (!is_thermal_safe(brain, temp_c)) {
        brain->last_safety_status = G6_SAFETY_THERMAL;
        goto safety_layer;
    }

    if (err_pct > brain->ner_threshold) {
        g6_asic_error_handle_non_blocking(brain, err_pct);
        goto safety_layer;
    }

    brain->last_update_timestamp = now;

    bool valid = is_sample_valid(brain, hr_ths, temp_c, err_pct, share_count);
    if (valid || brain->sample_state == BRAIN_STATE_SETTLE_WAIT ||
        brain->sample_state == BRAIN_STATE_MEASURE_WINDOW) {
        advance_sample_state(brain, now);
    }

    if (!valid) goto safety_layer;

    float fn = (f_mhz - BM1370_F_CENTER) / BM1370_F_SCALE;
    float vn = (v_mv - BM1370_V_CENTER) / BM1370_V_SCALE;

    float x[RLS_N] = {fn*fn, vn*vn, fn*vn, fn, vn, 1.0f};

    float Px[RLS_N] = {0};
    for (int i = 0; i < RLS_N; i++)
        for (int j = 0; j < RLS_N; j++)
            Px[i] += brain->P[i][j] * x[j];

    float xPx = 0.0f;
    for (int i = 0; i < RLS_N; i++) xPx += x[i] * Px[i];

    float power_Px[RLS_N] = {0};
    float power_xPx = 0.0f;
    if (brain->use_efficiency_mode) {
        for (int i = 0; i < RLS_N; i++)
            for (int j = 0; j < RLS_N; j++)
                power_Px[i] += brain->power_P[i][j] * x[j];
        for (int i = 0; i < RLS_N; i++) power_xPx += x[i] * power_Px[i];
    }

    float y_pred = evaluate_quadratic(brain->theta, fn, vn);
    float err = hr_ths - y_pred;

    if (!has_significant_innovation(brain->P, x)) goto safety_layer;
    if (brain->use_efficiency_mode && !has_significant_innovation(brain->power_P, x)) goto safety_layer;

    bool hr_outlier = (err * err > 9.0f * (xPx + 0.5f));
    bool pw_outlier = false;
    if (brain->use_efficiency_mode) {
        float y_power_pred = evaluate_quadratic(brain->power_theta, fn, vn);
        float power_err = power_w - y_power_pred;
        pw_outlier = (power_err * power_err > 9.0f * (power_xPx + 0.5f));
    }

    if (hr_outlier || pw_outlier) {
        if (hr_outlier) brain->last_safety_status = G6_SAFETY_SAMPLE_QUALITY;
        if (pw_outlier) {
            brain->last_safety_status = G6_SAFETY_POWER_SANITY;
            ESP_LOGW(TAG, "Power Outlier Rejected");
        }
        goto safety_layer;
    }

    if (trace_P(brain->P) <= RLS_TRACE_MAX) {
        float lambda_eff = brain->cold_start ? 0.985f : compute_gradient_vff(fabsf(err), RLS_VFF_SIGMA_SQ);
        if (lambda_eff < RLS_LAMBDA_MIN) lambda_eff = RLS_LAMBDA_MIN;

        float denom = lambda_eff + xPx;
        if (denom < 1e-9f) denom = 1e-9f;

        float k[RLS_N];
        for (int i = 0; i < RLS_N; i++) k[i] = Px[i] / denom;

        for (int i = 0; i < RLS_N; i++) brain->theta[i] += k[i] * err;

        float M[RLS_N][RLS_N];
        for (int i = 0; i < RLS_N; i++)
            for (int j = 0; j < RLS_N; j++)
                M[i][j] = (i == j ? 1.0f : 0.0f) - k[i] * x[j];

        float T_mat[RLS_N][RLS_N] = {0};
        for (int i = 0; i < RLS_N; i++)
            for (int j = 0; j < RLS_N; j++)
                for (int l = 0; l < RLS_N; l++)
                    T_mat[i][j] += M[i][l] * brain->P[l][j];

        for (int i = 0; i < RLS_N; i++) {
            for (int j = 0; j < RLS_N; j++) {
                float sum = 0.0f;
                for (int l = 0; l < RLS_N; l++) sum += T_mat[i][l] * M[j][l];
                brain->P[i][j] = sum / lambda_eff;
            }
            brain->P[i][i] += brain->ridge_epsilon;
        }

        brain->last_innovation = err;
        rls_symmetrize_clamp_and_stabilize(brain->P);
        brain->model_quality = fmaxf(0.0f, 1.0f - fabsf(err) / (hr_ths + 1.0f));
        brain->update_count++;
        if (brain->update_count > 25) brain->cold_start = false;
    }

    if (brain->use_efficiency_mode && trace_P(brain->power_P) <= RLS_TRACE_MAX) {
        float y_power_pred = evaluate_quadratic(brain->power_theta, fn, vn);
        float power_err = power_w - y_power_pred;
        float lambda_eff = brain->power_cold_start ? 0.985f : compute_gradient_vff(fabsf(power_err), RLS_VFF_SIGMA_SQ);
        if (lambda_eff < RLS_LAMBDA_MIN) lambda_eff = RLS_LAMBDA_MIN;

        float denom = lambda_eff + power_xPx;
        if (denom < 1e-9f) denom = 1e-9f;

        float k[RLS_N];
        for (int i = 0; i < RLS_N; i++) k[i] = power_Px[i] / denom;

        for (int i = 0; i < RLS_N; i++) brain->power_theta[i] += k[i] * power_err;

        float M[RLS_N][RLS_N];
        for (int i = 0; i < RLS_N; i++)
            for (int j = 0; j < RLS_N; j++)
                M[i][j] = (i == j ? 1.0f : 0.0f) - k[i] * x[j];

        float T_mat[RLS_N][RLS_N] = {0};
        for (int i = 0; i < RLS_N; i++)
            for (int j = 0; j < RLS_N; j++)
                for (int l = 0; l < RLS_N; l++)
                    T_mat[i][j] += M[i][l] * brain->power_P[l][j];

        for (int i = 0; i < RLS_N; i++) {
            for (int j = 0; j < RLS_N; j++) {
                float sum = 0.0f;
                for (int l = 0; l < RLS_N; l++) sum += T_mat[i][l] * M[j][l];
                brain->power_P[i][j] = sum / lambda_eff;
            }
            brain->power_P[i][i] += brain->ridge_epsilon;
        }

        rls_symmetrize_clamp_and_stabilize(brain->power_P);
        brain->power_model_quality = fmaxf(0.0f, 1.0f - fabsf(power_err) / (power_w + 1.0f));
        brain->power_update_count++;
        if (brain->power_update_count > 25) brain->power_cold_start = false;
    }

safety_layer:
    bool vr_safety_active = (vr_temp_c > G6_VR_TEMP_NO_SENSOR && isfinite(vr_temp_c) &&
                              brain->vr_temp_ceiling > 0.0f &&
                              vr_temp_c > (brain->vr_temp_ceiling - brain->vr_temp_proactive_margin));

    bool safety_active = (temp_c > (brain->temp_ceiling - brain->temp_proactive_margin)) ||
                         (err_pct > brain->ner_threshold) ||
                         vr_safety_active;

    float candidate_f, candidate_v;
    g6_brain_get_optimal(brain, &candidate_f, &candidate_v, NULL);

    if (brain->control_mode == G6_MODE_AUTO && !safety_active) {
        float df = candidate_f - brain->best_f;
        if (fabsf(df) > brain->dfs_step_mhz) df = copysignf(brain->dfs_step_mhz, df);
        brain->best_f += df;

        float dv = candidate_v - brain->best_v;
        if (fabsf(dv) > 5.0f) dv = copysignf(5.0f, dv);
        brain->best_v += dv;
    }

    if (brain->best_f < BM1370_F_MIN) brain->best_f = BM1370_F_MIN;
    if (brain->best_f > BM1370_F_MAX) brain->best_f = BM1370_F_MAX;
    if (brain->best_v < BM1370_V_MIN) brain->best_v = BM1370_V_MIN;
    if (brain->best_v > BM1370_V_MAX) brain->best_v = BM1370_V_MAX;

    /* Safety helpers run after the hard clamps. Order matters for last_safety_status:
     * each helper overwrites the status only when its condition triggers.
     * VR thermal runs first so ASIC thermal (the higher-priority condition) wins
     * on any tick where both fire simultaneously. */
    g6_safety_check_voltage_ripple(brain, v_mv);
    g6_safety_proactive_vr_thermal_scale(brain, vr_temp_c);
    g6_safety_proactive_thermal_scale(brain, temp_c);

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
        optimize_jth_dinkelbach(brain, opt_f, opt_v);
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
        if (brain->P[i][i] < RLS_P_CLAMP_MIN || brain->P[i][i] > RLS_P_CLAMP_MAX) ok = false;
        if (brain->P[i][i] < min_diag) min_diag = brain->P[i][i];
        if (brain->P[i][i] > max_diag) max_diag = brain->P[i][i];
        for (int j = i + 1; j < RLS_N; j++)
            if (fabsf(brain->P[i][j] - brain->P[j][i]) > RLS_SYMMETRY_TOLERANCE) ok = false;
    }

    float cond = (min_diag > 1e-9f) ? (max_diag / min_diag) : 0.0f;
    if (cond > 5e5f) ok = false;

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
    out->safety_status = brain->last_safety_status;
    out->efficiency_mode_active = brain->use_efficiency_mode;
    out->last_recommended_voltage = brain->best_v;
}
