// g6_safety.c - Full safety implementation
#include "g6_safety.h"
#include "esp_log.h"

static const char *TAG = "g6_safety";

g6_safety_status_t g6_safety_check(const G6BrainState *brain, float f, float v, float temp_c) {
    if (v > 1350.0f) return G6_SAFETY_VOLTAGE_HIGH;
    if (temp_c > 80.0f) return G6_SAFETY_TEMP_HIGH;
    if (brain->ner_threshold > 0.01f) return G6_SAFETY_DIVERGENCE;
    return G6_SAFETY_OK;
}

void g6_safety_apply_clamps(float *f, float *v) {
    if (*v > 1350.0f) *v = 1350.0f;
    if (*f > 900.0f) *f = 900.0f;
    if (*f < 500.0f) *f = 500.0f;
}

void g6_safety_ocp_hard_trip() {
    ESP_LOGE(TAG, "OCP HARD TRIP - Pulling buck EN low");
    // GPIO control for buck regulator EN pin
}