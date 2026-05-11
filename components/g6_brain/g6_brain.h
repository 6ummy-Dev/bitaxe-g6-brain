#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "nvs.h"

/* =============================================
   Bitaxe Brains Project — FULLY MODULAR DESIGN
   G6 Brain is the first swappable module.
   Future brains implement the exact same interface.
   ============================================= */

#define G6_BRAIN_VERSION "v1.1 Beta (Modular)"

// Forward declare
typedef struct G6BrainState G6BrainState;

// Modular brain interface — this is the contract for the entire Brains Project
typedef struct {
    esp_err_t (*init)(G6BrainState *brain);
    esp_err_t (*update)(G6BrainState *brain, float f_mhz, float v_mv, float hr_ths,
                        float power_w, float temp_c, float err_pct);
    void      (*get_optimal)(const G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr);
    float     (*get_model_quality)(const G6BrainState *brain);
    void      (*get_full_telemetry)(const G6BrainState *brain, char *json_buf, size_t buf_size);
    bool      (*self_test)(G6BrainState *brain);
} G6BrainInterface;

// Main Brain State — kept clean, extensible, and modular
struct G6BrainState {
    // RLS quadratic model
    float theta[6];
    float P[6][6];
    float lambda;
    float ridge_epsilon;
    float model_quality;
    int update_count;
    bool cold_start;

    // Optimal setpoints
    float best_f;
    float best_v;

    // Safety hardening (now native & modular)
    uint32_t i2c_timeout_count;
    uint32_t i2c_hard_fault_threshold;
    float    voltage_undershoot_history[16];
    uint8_t  undershoot_idx;
    float    last_measured_v;
    bool     i2c_hard_fault_triggered;

    // PID + thermal
    float Kp, Ki, Kd;
    float last_temp;
    float integral;

    // Config (Kconfig driven)
    float temp_ceiling;
    uint32_t dfs_step_mhz;
    float ner_threshold;

    // Puzzle / extras
    uint32_t recommended_nonce_start;
    uint32_t recommended_nonce_range;

    // NVS
    uint32_t nvs_write_count;

    // Modular interface pointer (for future multi-brain runtime swap)
    const G6BrainInterface *interface;
};

// Public modular API — these are the only calls the miner firmware ever sees
esp_err_t g6_brain_init(G6BrainState *brain);
esp_err_t g6_brain_update(G6BrainState *brain, float f_mhz, float v_mv, float hr_ths,
                         float power_w, float temp_c, float err_pct);
void g6_brain_get_optimal(const G6BrainState *brain, float *opt_f, float *opt_v, float *pred_hr);
float g6_brain_get_model_quality(const G6BrainState *brain);
void g6_brain_get_full_telemetry(const G6BrainState *brain, char *json_buf, size_t buf_size);
bool g6_brain_self_test(G6BrainState *brain);

// I2C Guardian (still public for manual calls from i2c_bitaxe wrapper)
void g6_brain_i2c_guardian_recover(i2c_port_t port);
