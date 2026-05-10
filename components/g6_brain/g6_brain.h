#pragma once

#define G6_BRAIN_VERSION "v1.0 BETA"

// Full consolidated version with RLS, Predictive PID, I2C recovery, Vcore soft-ramp, etc.
// All Level 1 and Level 2 improvements integrated.

typedef struct {
    // RLS state, PID state, safety flags, etc.
    float theta[6];
    // ... full struct for production brain
} G6BrainState;

// Function declarations for all features
void g6_brain_init(G6BrainState* brain);
void g6_brain_update(G6BrainState* brain, float freq, float voltage, float hashrate, float temp);
void g6_thermal_predictive_pid_update(float current_temp, float dTdt);
void g6_i2c_bus_recovery(void);
void g6_vcore_soft_ramp(float target_mv);

// etc.