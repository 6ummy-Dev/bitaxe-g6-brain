# G6 Brain Configuration — v1.0.0-beta7.1

All options live under: **Component config → G6 Brain Configuration**

---

## RLS Optimizer

### `G6_RLS_LAMBDA_MIN`
- **Type:** int (900–999)
- **Default:** `950` (= 0.950)
- **Description:** Minimum effective forgetting factor for the Variable Forgetting Factor algorithm.

### `G6_RLS_RIDGE_EPSILON`
- **Type:** int (1–1000)
- **Default:** `10` (= 10 × 1e-6)
- **Description:** Ridge regularization term added to the covariance diagonal to prevent numerical matrix collapse.

### `G6_RLS_TRACE_MAX`
- **Type:** int (100000–100000000)
- **Default:** `10000000`
- **Description:** Maximum allowed trace value for the covariance matrix. Exceeding this limit mathematically zero-fills the active polynomial surfaces and resets matrix confidence to safely arrest runaway divergence and prevent recursive gain explosions.

### `G6_RLS_VFF_SIGMA_SQ`
- **Type:** int (10–200)
- **Default:** `80` (= 0.0080)
- **Description:** Expected measurement noise variance scale used to tune the VFF tracking speed response.

---

## Safety & Thermal

### `G6_TEMP_CEILING`
- **Type:** int (50–85)
- **Default:** `70` (°C)
- **Description:** Absolute thermal limit for the ASIC die. Proactive scaling steps activate at `G6_TEMP_PROACTIVE_MARGIN` °C below this threshold.

### `G6_TEMP_PROACTIVE_MARGIN`
- **Type:** int (2–15)
- **Default:** `5` (°C)
- **Description:** Degrees below `G6_TEMP_CEILING` where proactive ASIC frequency and voltage step-back begins. Also controls the threshold at which upward slew-rate is suspended to prevent oscillation at the thermal edge. Mirrors `G6_VR_TEMP_PROACTIVE_MARGIN` for the ASIC die.

### `G6_VR_TEMP_CEILING`
- **Type:** int (70–105)
- **Default:** `85` (°C)
- **Description:** Hard thermal ceiling for the voltage regulator (VR). When the VR temperature reaches or exceeds this value, both voltage and frequency are stepped back to reduce power through the regulator.  
  Use `G6_VR_TEMP_NO_SENSOR` (`-1.0f`) when no VR temperature sensor is available.

### `G6_VR_TEMP_PROACTIVE_MARGIN`
- **Type:** int (2–15)
- **Default:** `5` (°C)
- **Description:** Degrees below `G6_VR_TEMP_CEILING` where proactive voltage reduction begins. Only voltage is reduced in this zone. Frequency reduction only occurs at the hard ceiling.

### `G6_NER_THRESHOLD`
- **Type:** int (50–2000)
- **Default:** `250` (= 2.50%)
- **Description:** Nonce Error Rate threshold in 0.01% units. Exceeding this value forces conservative fallback behavior.

### `G6_ENABLE_TEMP_PLAUSIBILITY`
- **Type:** bool
- **Default:** `n` (disabled)
- **Description:** Opt-in defense-in-depth gate. When enabled, finite-but-implausible ASIC/VR temperature readings outside `[G6_TEMP_PLAUSIBILITY_MIN, G6_TEMP_PLAUSIBILITY_MAX]` are routed fail-closed to the safety layer with `G6_SAFETY_INPUT_RANGE`. This catches stuck-low / stuck-high sensors — e.g. a `-50 °C` reading that the finiteness-only check would treat as a cold, healthy chip, letting the brain train the RLS models on a thermally-stressed ASIC. **Default OFF:** out of the box the brain validates temperature for finiteness only and trusts the integrator's telemetry layer for sensor health. The VR no-sensor sentinel (`-1.0`) is always exempt from this band, enabled or not.

### `G6_TEMP_PLAUSIBILITY_MIN`
- **Type:** int (−40–40), °C
- **Default:** `0`
- **Depends on:** `G6_ENABLE_TEMP_PLAUSIBILITY`
- **Description:** Lower bound of the optional temperature plausibility band. Readings below this (and finite, and not the VR sentinel) are rejected fail-closed when the band is enabled.

### `G6_TEMP_PLAUSIBILITY_MAX`
- **Type:** int (90–200), °C
- **Default:** `120`
- **Depends on:** `G6_ENABLE_TEMP_PLAUSIBILITY`
- **Description:** Upper bound of the optional temperature plausibility band. Readings above this are rejected fail-closed when the band is enabled. Note the hard thermal ceiling (`G6_TEMP_CEILING`) still applies independently — this band guards against implausible *sensor* values, not normal over-temperature, which the thermal layer already handles.
  
---

## Optimization Behavior

### `G6_DFS_STEP_MHZ`
- **Type:** int (5–50)
- **Default:** `25` (MHz)
- **Description:** Frequency delta boundary applied by the internal tracking loop for step-by-step slew rate control.

### `G6_ENABLE_EFFICIENCY_MODE`
- **Type:** bool
- **Default:** `n`
- **Description:** Configures the tracking loop to minimize Watts per TH/s using a parallel power-estimator surface and a bounded analytical Dinkelbach J/TH solver — O(1)-per-step fractional minimization (exact closed form for interior optima; clamped boundary point otherwise), gated on both model qualities ≥ 0.6.

### `G6_JTH_MAX_OUTER_ITERS`
- **Type:** int (3–15)
- **Default:** `7`
- **Description:** Maximum allowable outer fractional refinement steps for the analytical efficiency search loop.
