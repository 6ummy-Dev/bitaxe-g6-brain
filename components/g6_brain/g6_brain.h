#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "nvs.h"
#include <stdint.h>

/* =============================================
   Bitaxe Brains Project — v1.0 Beta
   Fully Modular + Aerospace QA Hardened
   ============================================= */

#define G6_BRAIN_VERSION "v1.0 Beta"

// Modular brain interface (unchanged contract)
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
    // RLS quadratic model (preserved)
    float theta[6];
    float P[6][6];
    float lambda;
    float ridge_epsilon;
    float model_quality;
    int update_count;
    bool cold_start;
    float best_f;
    float best_v;

    // QA Hardening — Thermal & Voltage
    float last_temp_c;
    float temp_rise_rate;           // °C/s for proactive ΔT/dt scaling
    uint32_t last_update_timestamp;

    // Voltage ripple / undershoot tracking (64-bit friendly)
    float voltage_history[8];
    uint8_t voltage_hist_idx;
    float voltage_variance;

    // I2C/ASIC guardian + unhappy path
    uint32_t i2c_timeout_count;
    uint32_t i2c_hard_fault_threshold;
    bool i2c_hard_fault_triggered;

    // NVS wear-leveling (RTC RAM strategy)
    uint32_t nvs_last_write_tick;
    uint32_t nvs_write_interval;

    // PID + safety
    float Kp, Ki, Kd;
    float integral;
    float temp_ceiling;
    uint32_t dfs_step_mhz;
    float ner_threshold;

    // Modular interface pointer
    const G6BrainInterface *interface;
};

// Public API
esp_err_t g6_brain_init(G6BrainState *brain);
esp_err_t g6_brain_update(G6BrainState *brain, float f_mhz, float v_mv, float hr_ths,
                         float power_w, float temp_c, float err_pct);
void g6_brain_get_optimal(const G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr);
float g6_brain_get_model_quality(const G6BrainState *brain);
void g6_brain_get_full_telemetry(const G6BrainState *brain, char *json_buf, size_t buf_size);
bool g6_brain_self_test(G6BrainState *brain);

// QA-hardened functions (modular)
void g6_safety_proactive_thermal_scale(G6BrainState *brain, float current_temp);
void g6_safety_check_voltage_ripple(G6BrainState *brain, float measured_v);
void g6_asic_error_handle_non_blocking(G6BrainState *brain);  // +5mV tune on BM1366 non-blocking
void g6_brain_i2c_guardian_recover(i2c_port_t port);
