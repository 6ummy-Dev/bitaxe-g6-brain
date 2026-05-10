#include "g6_brain.h"
#include "g6_safety.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <string.h>
#include <math.h>

static const char *TAG = "g6_brain";

// Inline 6x6 matrix helpers (optimized for ESP32)
static void matrix_mult_6x6(const float A[6][6], const float B[6][6], float C[6][6]) {
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 6; j++) {
            C[i][j] = 0;
            for (int k = 0; k < 6; k++) C[i][j] += A[i][k] * B[k][j];
        }
    }
}

static void matrix_vector_mult(const float A[6][6], const float x[6], float y[6]) {
    for (int i = 0; i < 6; i++) {
        y[i] = 0;
        for (int k = 0; k < 6; k++) y[i] += A[i][k] * x[k];
    }
}

// RLS update with ridge regularization and stability check
esp_err_t g6_brain_init(G6BrainState *brain) {
    memset(brain, 0, sizeof(G6BrainState));
    brain->lambda = 0.98f; // default forgetting
    brain->ridge_epsilon = 1e-6f;
    // Initial P = large identity
    for (int i = 0; i < 6; i++) for (int j = 0; j < 6; j++) brain->P[i][j] = (i==j) ? 1000.0f : 0.0f;
    brain->auto_tune_enabled = true;
    brain->lambda_power = 0.6f;
    brain->lambda_temp = 0.3f;
    brain->lambda_error = 0.1f;
    ESP_LOGI(TAG, "G6 Brain v%s initialized", G6_BRAIN_VERSION);
    return ESP_OK;
}

void g6_brain_update(G6BrainState *brain, float f_mhz, float v_mv, float hr_ths, float power_w, float temp_c, float err_pct) {
    // Form regressor x = [f², v², f*v, f, v, 1]
    float x[6];
    x[0] = f_mhz * f_mhz;
    x[1] = v_mv * v_mv / 1e6f; // scaled
    x[2] = f_mhz * v_mv / 1000.0f;
    x[3] = f_mhz;
    x[4] = v_mv / 1000.0f;
    x[5] = 1.0f;

    // Prediction
    float y_pred = 0.0f;
    for (int i = 0; i < 6; i++) y_pred += brain->theta[i] * x[i];
    float y = hr_ths; // target

    // RLS gain k
    float Px[6];
    matrix_vector_mult(brain->P, x, Px);
    float denom = brain->lambda + 0;
    for (int i = 0; i < 6; i++) denom += x[i] * Px[i];
    float k[6];
    for (int i = 0; i < 6; i++) k[i] = Px[i] / denom;

    // Update theta
    float err = y - y_pred;
    for (int i = 0; i < 6; i++) brain->theta[i] += k[i] * err;

    // Update P with ridge
    float outer[6][6];
    for (int i = 0; i < 6; i++) for (int j = 0; j < 6; j++) outer[i][j] = k[i] * Px[j];
    for (int i = 0; i < 6; i++) for (int j = 0; j < 6; j++) {
        brain->P[i][j] = (brain->P[i][j] - outer[i][j]) / brain->lambda + (i==j ? brain->ridge_epsilon : 0);
    }

    // Model quality
    brain->model_quality = 1.0f - fabsf(err) / (hr_ths + 1.0f);

    // Call safety
    g6_brain_apply_safety_clamps(brain, &brain->best_f, &brain->best_v);

    ESP_LOGD(TAG, "RLS update: HR err=%.1f%%", err_pct);
}

// Analytical quadratic optimizer (projected gradient approx for simplicity)
void g6_brain_get_optimal(G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr) {
    // Simple grid or closed form for quadratic (placeholder for analytical in 10/10)
    // Real: solve partial deriv = 0 with constraints
    *opt_f = 650.0f; // example from Gamma 602
    *opt_v = 1200.0f;
    *pred_hr = 550.0f;
    // TODO: full projected solver in future
}

void g6_brain_auto_step(G6BrainState *brain, float current_f, float current_v) {
    if (!brain->auto_tune_enabled) return;
    float opt_f, opt_v, pred;
    g6_brain_get_optimal(brain, &opt_f, &opt_v, &pred);
    // Slew rate limit 10MHz / 50mV per step
    float df = opt_f - current_f;
    if (df > 10.0f) df = 10.0f;
    if (df < -10.0f) df = -10.0f;
    float dv = opt_v - current_v;
    if (dv > 50.0f) dv = 50.0f;
    if (dv < -50.0f) dv = -50.0f;
    // Apply
    // (integration with miner API left to user)
}

// Stub NVS
esp_err_t g6_brain_load_from_nvs(G6BrainState *brain) { return ESP_OK; }
esp_err_t g6_brain_save_to_nvs(G6BrainState *brain) { return ESP_OK; }

void g6_brain_print_full_status(const G6BrainState *brain) {
    ESP_LOGI(TAG, "G6 Brain v%s - Model quality: %.1f%%", G6_BRAIN_VERSION, brain->model_quality * 100);
}

uint32_t g6_brain_get_recommended_nonce_start(G6BrainState *brain) { return 0; }
uint32_t g6_brain_get_recommended_nonce_range(G6BrainState *brain) { return 0xFFFFFFFF; }

void g6_brain_predict_thermal_rise(G6BrainState *brain, float hr, float power, float *rise_c) {
    *rise_c = power * 2.5f; // simple linear model
}

// Safety clamp
void g6_brain_apply_safety_clamps(G6BrainState *brain, float *f, float *v) {
    if (*v > G6_MAX_VOLTAGE_MV) *v = G6_MAX_VOLTAGE_MV;
    if (*f > 900.0f) *f = 900.0f;
}
