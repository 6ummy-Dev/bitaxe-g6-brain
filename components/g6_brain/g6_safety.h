// g6_safety.h - Extended safety for Phase 2
#pragma once
#include "g6_brain.h"

typedef enum {
    G6_SAFETY_OK,
    G6_SAFETY_VOLTAGE_HIGH,
    G6_SAFETY_TEMP_HIGH,
    G6_SAFETY_DIVERGENCE,
    G6_SAFETY_I2C_HANG,
    G6_SAFETY_OCP_TRIP
} g6_safety_status_t;

g6_safety_status_t g6_safety_check(const G6BrainState *brain, float f, float v, float temp_c);
void g6_safety_apply_clamps(float *f, float *v);
void g6_safety_ocp_hard_trip();