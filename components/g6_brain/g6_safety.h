#pragma once
#include "g6_brain.h"

typedef enum {
    G6_SAFETY_OK,
    G6_SAFETY_VOLTAGE_HIGH,
    G6_SAFETY_TEMP_HIGH,
    G6_SAFETY_DIVERGENCE
} g6_safety_status_t;

g6_safety_status_t g6_safety_check(const G6BrainState *brain, float f, float v, float temp_c);
void g6_safety_apply_clamps(float *f, float *v);
