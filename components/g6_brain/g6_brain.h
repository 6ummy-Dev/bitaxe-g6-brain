#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "nvs.h"
#include <stdint.h>

/* =============================================
   Bitaxe Brains Project — v1.0 Beta
   Modular + QA Hardened + Stochastic Nonce
   ============================================= */

#define G6_BRAIN_VERSION "v1.0 Beta"

// Modular brain interface
typedef struct G6BrainState G6BrainState;
typedef struct {
    esp_err_t (*init)(G6BrainState *brain);
    esp_err_t (*update)(G6BrainState *brain, float f_mhz, float v_mv, float hr_ths,
                        float power_w, float temp_c, float err_pct);
    void      (*get_optimal)(const G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr);
    float     (*get_model_quality)(const G6BrainState *brain);
    void      (*get_full_telemetry)(const G6BrainState *brain, char *json_buf, size_t buf_size);
    bool      (*self_test)(G6BrainState *brain);
} G6BrainInterface;

struct G6BrainState {
    // RLS quadratic model (core — untouched)
    float theta[6];
    float P[6][6];
    float lambda;
    float ridge_epsilon;
    float model_quality;
    int update_count;
    bool cold_start;
    float best_f;
    float best_v;

    // QA Hardening (preserved)
    float last_temp_c;
    float temp_rise_rate;
    uint32_t last_update_timestamp;
    float voltage_history[8];
    uint8_t voltage_hist_idx;
    float voltage_variance;
    uint32_t i2c_timeout_count;
    uint32_t i2c_hard_fault_threshold;
    bool i2c_hard_fault_triggered;
    uint32_t nvs_last_write_tick;
    uint32_t nvs_write_interval;
    float Kp, Ki, Kd;
    float integral;
    float temp_ceiling;
    uint32_t dfs_step_mhz;
    float ner_threshold;

    // NEW: Stochastic Nonce Offsetting (feedback addition)
    uint32_t nonce_offset;
    bool enable_low_latency_jobs;

    // Modular interface pointer
    const G6BrainInterface *interface;
};

// Public API (unchanged + new nonce helper)
esp_err_t g6_brain_init(G6BrainState *brain);
esp_err_t g6_brain_update(G6BrainState *brain, float f_mhz, float v_mv, float hr_ths,
                         float power_w, float temp_c, float err_pct);
void g6_brain_get_optimal(const G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr);
float g6_brain_get_model_quality(const G6BrainState *brain);
void g6_brain_get_full_telemetry(const G6BrainState *brain, char *json_buf, size_t buf_size);
bool g6_brain_self_test(G6BrainState *brain);

// QA-hardened functions (preserved)
void g6_safety_proactive_thermal_scale(G6BrainState *brain, float current_temp);
void g6_safety_check_voltage_ripple(G6BrainState *brain, float measured_v);
void g6_asic_error_handle_non_blocking(G6BrainState *brain);
void g6_brain_i2c_guardian_recover(i2c_port_t port);

// NEW: Stochastic Nonce helper (called by miner on new job)
uint32_t g6_brain_get_stochastic_nonce_start(G6BrainState *brain);
