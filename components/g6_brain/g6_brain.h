#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "nvs.h"

#define G6_BRAIN_VERSION "1.8.5 revised"

// Consolidated G6BrainState with all features from Phase 1 (RLS, PID, safety) + Phase 2 (I2C Guardian, Fixed-Point, Zero-Copy Stratum, Smart-Throttling DFS, P-VUS, NVS wear-leveling, Atomic counters, DMA UART, Dual Stratum)
// Revised for 1.8.5: fixed I2C watchdog logic, NVS safety, NER tracking from err_pct, anti-windup in PID, denom guard in RLS, telemetry prep
typedef struct {
    float theta[6];           // RLS quadratic coefficients for HR(f,v)
    float P[6][6];            // Covariance matrix
    float lambda;             // RLS forgetting factor
    float ridge_epsilon;      // Ridge regularization
    float best_f, best_v;
    float best_score;
    float model_quality;
    bool auto_tune_enabled;
    // Multi-objective weights
    float lambda_power, lambda_temp, lambda_error;
    // Puzzle solving helpers
    uint32_t recommended_nonce_start;
    uint32_t recommended_nonce_range;
    // Thermal PID + derivative
    float Kp, Ki, Kd;
    float last_temp;
    float integral;
    float derivative;
    // I2C Guardian
    uint32_t i2c_timeout_ms;
    uint32_t last_i2c_transaction;
    // P-VUS NER tracking
    float ner_threshold;
    uint32_t z_nonce_count;
    uint32_t total_nonce_count;
    // Smart DFS
    float temp_ceiling;
    uint32_t dfs_step_mhz;
    // NVS wear-leveling
    uint32_t nvs_write_count;
    // Atomic counters
    uint64_t total_shares;
    uint64_t total_hashrate;
    // Safety
    float max_temp_c;
    float max_voltage_mv;
    // Cold-start guard for RLS stability (new in this update)
    bool cold_start;
    int update_count;
} G6BrainState;

// All function prototypes
esp_err_t g6_brain_init(G6BrainState *brain);
void g6_brain_update(G6BrainState *brain, float f_mhz, float v_mv, float hr_ths, float power_w, float temp_c, float err_pct);
void g6_brain_auto_step(G6BrainState *brain, float current_f, float current_v);
void g6_brain_get_optimal(G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr);
void g6_brain_print_full_status(const G6BrainState *brain);
void g6_brain_i2c_guardian_recover(i2c_port_t port);
float g6_brain_pid_compute(G6BrainState *brain, float current_temp, float target_temp);
void g6_brain_smart_dfs(G6BrainState *brain, float current_temp);
void g6_brain_pvus_check(G6BrainState *brain, float current_v);
void g6_brain_nvs_log(G6BrainState *brain);
void g6_brain_fixed_point_efficiency(uint64_t power_mw, uint64_t hashrate_ghs, char *output);
uint32_t g6_brain_get_recommended_nonce_start(G6BrainState *brain);
uint32_t g6_brain_get_recommended_nonce_range(G6BrainState *brain);
void g6_brain_predict_thermal_rise(G6BrainState *brain, float hr, float power, float *rise_c);
void g6_puzzle_extras_run(G6BrainState *brain);