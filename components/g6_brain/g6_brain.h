#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "nvs.h"

#define G6_BRAIN_VERSION "v1.0 Beta"

// Consolidated G6BrainState ... (v1.0 Beta - thread safety prep + Kconfig integration)
typedef struct {
    float theta[6];
    float P[6][6];
    float lambda;
    float ridge_epsilon;
    float best_f, best_v;
    float best_score;
    float model_quality;
    bool auto_tune_enabled;
    float lambda_power, lambda_temp, lambda_error;
    uint32_t recommended_nonce_start;
    uint32_t recommended_nonce_range;
    float Kp, Ki, Kd;
    float last_temp;
    float integral;
    float derivative;
    uint32_t i2c_timeout_ms;
    uint32_t last_i2c_transaction;
    float ner_threshold;
    uint32_t z_nonce_count;
    uint32_t total_nonce_count;
    float temp_ceiling;
    uint32_t dfs_step_mhz;
    uint32_t nvs_write_count;
    uint64_t total_shares;
    uint64_t total_hashrate;
    float max_temp_c;
    float max_voltage_mv;
    bool cold_start;
    int update_count;
} G6BrainState;

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