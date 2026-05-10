#include "g6_brain.h"
#include <math.h>
#include <string.h>
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "G6_BRAIN";

static void mat_vec_mul(const float mat[6][6], const float vec[6], float out[6]) {
    for (int i = 0; i < 6; i++) {
        out[i] = 0.0f;
        for (int j = 0; j < 6; j++) out[i] += mat[i][j] * vec[j];
    }
}

static void rank1_update(float mat[6][6], const float k[6], const float phi[6], float lambda) {
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 6; j++) {
            mat[i][j] = (mat[i][j] - k[i] * phi[j]) / lambda;
        }
    }
}

void g6_brain_init(G6BrainState *brain) {
    memset(brain, 0, sizeof(G6BrainState));
    brain->lambda = 0.965f;
    brain->max_temp_c = 68.0f;
    brain->max_error_pct = 1.8f;
    brain->min_improvement_pct = 1.8f;
    brain->auto_tune_enabled = true;
    brain->model_quality = 0.1f;

    for (int i = 0; i < G6_PARAMS; i++) {
        for (int j = 0; j < G6_PARAMS; j++) {
            brain->P[i][j] = (i == j) ? 2000.0f : 0.0f;
        }
    }

    // Strong prior based on typical Gamma 602 data
    brain->theta[0] = -0.0000015f;
    brain->theta[1] = -0.00015f;
    brain->theta[2] = 0.000014f;
    brain->theta[3] = 0.0023f;
    brain->theta[4] = 0.52f;
    brain->theta[5] = -95.0f;

    ESP_LOGI(TAG, "G6 Brain v%s initialized - Pure RLS Quadratic Optimizer", G6_BRAIN_VERSION);
}

void g6_brain_update(G6BrainState *brain, float f, float v, float hr, float power, float temp, float err) {
    if (temp > brain->max_temp_c || err > brain->max_error_pct || isnan(hr) || isinf(hr)) return;

    float phi[G6_PARAMS] = {f*f, v*v, f*v, f, v, 1.0f};
    float y = hr;

    float phiT_theta = 0.0f;
    for (int i = 0; i < G6_PARAMS; i++) phiT_theta += phi[i] * brain->theta[i];
    float e = y - phiT_theta;

    float P_phi[G6_PARAMS];
    mat_vec_mul(brain->P, phi, P_phi);

    float denom = brain->lambda + 1e-7f;
    for (int i = 0; i < G6_PARAMS; i++) denom += phi[i] * P_phi[i];

    float k[G6_PARAMS];
    for (int i = 0; i < G6_PARAMS; i++) k[i] = P_phi[i] / denom;

    for (int i = 0; i < G6_PARAMS; i++) brain->theta[i] += k[i] * e;
    rank1_update(brain->P, k, phi, brain->lambda);

    brain->update_count++;

    // Periodic covariance reset to prevent numerical collapse
    if (brain->update_count % 5000 == 0) {
        for (int i = 0; i < G6_PARAMS; i++) {
            for (int j = 0; j < G6_PARAMS; j++) {
                brain->P[i][j] = (i == j) ? 500.0f : 0.0f;
            }
        }
    }

    float eff = hr / fmaxf(power, 0.1f);
    if (eff > brain->best_eff) {
        brain->best_f = f; brain->best_v = v;
        brain->best_hr = hr; brain->best_eff = eff;
    }

    // Better model quality using full trace
    float trace = 0.0f;
    for (int i = 0; i < G6_PARAMS; i++) trace += fabsf(brain->P[i][i]);
    brain->model_quality = 1.0f / (1.0f + 0.0005f * trace);
    if (brain->model_quality > 1.0f) brain->model_quality = 1.0f;
}

bool g6_brain_get_optimal(const G6BrainState *brain, float *out_f, float *out_v, float *out_pred_hr) {
    float a = brain->theta[0], b = brain->theta[1], c = brain->theta[2];
    float d = brain->theta[3], e = brain->theta[4];

    float det = 4.0f*a*b - c*c;
    if (fabsf(det) < 1e-6f || a >= 0.0f || det <= 0.0f) return false;

    float f_opt = (2.0f*b*(-d) - c*(-e)) / det;
    float v_opt = (2.0f*a*(-e) - c*(-d)) / det;

    f_opt = fmaxf(520.0f, fminf(f_opt, 960.0f));
    v_opt = fmaxf(1100.0f, fminf(v_opt, 1380.0f));

    float phi[G6_PARAMS] = {f_opt*f_opt, v_opt*v_opt, f_opt*v_opt, f_opt, v_opt, 1.0f};
    float pred = 0.0f;
    for (int i = 0; i < G6_PARAMS; i++) pred += phi[i] * brain->theta[i];

    *out_f = f_opt;
    *out_v = v_opt;
    *out_pred_hr = pred;
    return true;
}

void g6_brain_auto_step(G6BrainState *brain, float current_f, float current_v) {
    if (!brain->auto_tune_enabled || brain->model_quality < 0.35f) return;

    float new_f, new_v, pred_hr;
    if (!g6_brain_get_optimal(brain, &new_f, &new_v, &pred_hr)) return;

    float phi_cur[G6_PARAMS] = {current_f*current_f, current_v*current_v, current_f*current_v, current_f, current_v, 1.0f};
    float curr_pred = 0.0f;
    for (int i = 0; i < G6_PARAMS; i++) curr_pred += phi_cur[i] * brain->theta[i];

    float improvement_pct = (pred_hr - curr_pred) / fmaxf(curr_pred, 1.0f) * 100.0f;

    if (improvement_pct > brain->min_improvement_pct) {
        ESP_LOGI(TAG, "G6 v%s OPTIMUM: %.0f MHz @ %.0fmV (pred +%.1f%%) | Quality: %.2f", 
                 G6_BRAIN_VERSION, new_f, new_v, improvement_pct, brain->model_quality);
        // TODO: Call your ASIC setter here, e.g.:
        // asic_set_frequency_and_voltage((uint16_t)new_f, (uint16_t)new_v);
    }
}

void g6_brain_reset(G6BrainState *brain) {
    g6_brain_init(brain);
}

// NVS Persistence
esp_err_t g6_brain_save_to_nvs(const G6BrainState *brain) {
    nvs_handle_t nvs;
    if (nvs_open("g6_brain", NVS_READWRITE, &nvs) != ESP_OK) return ESP_FAIL;
    nvs_set_blob(nvs, "theta", brain->theta, sizeof(brain->theta));
    nvs_set_blob(nvs, "P", brain->P, sizeof(brain->P));
    nvs_set_float(nvs, "best_f", brain->best_f);
    nvs_set_float(nvs, "best_eff", brain->best_eff);
    esp_err_t err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
}

esp_err_t g6_brain_load_from_nvs(G6BrainState *brain) {
    nvs_handle_t nvs;
    if (nvs_open("g6_brain", NVS_READONLY, &nvs) != ESP_OK) return ESP_FAIL;
    size_t len = sizeof(brain->theta);
    nvs_get_blob(nvs, "theta", brain->theta, &len);
    len = sizeof(brain->P);
    nvs_get_blob(nvs, "P", brain->P, &len);
    nvs_get_float(nvs, "best_f", &brain->best_f);
    nvs_get_float(nvs, "best_eff", &brain->best_eff);
    nvs_close(nvs);
    return ESP_OK;
}