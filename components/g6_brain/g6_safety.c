#include "g6_safety.h"
#include "esp_log.h"

static const char *TAG = "g6_safety";

g6_safety_status_t g6_safety_check(const G6BrainState *brain, float f, float v, float temp_c) {
    if (v > 1350.0f) return G6_SAFETY_VOLTAGE_HIGH;
    if (temp_c > 70.0f) return G6_SAFETY_TEMP_HIGH;
    // Divergence check on P trace
    return G6_SAFETY_OK;
}

void g6_safety_apply_clamps(float *f, float *v) {
    if (*v > 1350.0f) *v = 1350.0f;
    if (*f > 900.0f) *f = 900.0f;
    if (*f < 500.0f) *f = 500.0f;
}
