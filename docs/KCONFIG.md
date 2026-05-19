# G6 Brain Kconfig Options — v1.0.0-beta3

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
- **Description:** Maximum allowed trace value for the covariance matrix before dropping into cold-start recovery.

### `G6_RLS_VFF_SIGMA_SQ`
- **Type:** int (10–200)
- **Default:** `80` (= 0.0080)
- **Description:** Expected measurement noise variance scale used to tune the VFF tracking speed response.

---

## Safety & Thermal

### `G6_TEMP_CEILING`
- **Type:** int (50–85)
- **Default:** `70` (°C)
- **Description:** Absolute thermal limit for the ASIC. Proactive scaling steps activate 5 °C below this threshold.

### `G6_NER_THRESHOLD`
- **Type:** int (50–2000)
- **Default:** `250` (= 2.50%)
- **Description:** Nonce Error Rate limit in 0.01% units. Exceeding this value forces conservative fallback targets.

---

## Optimization Behavior

### `G6_DFS_STEP_MHZ`
- **Type:** int (5–50)
- **Default:** `25` (MHz)
- **Description:** Frequency delta boundary applied by the internal tracking loop for step-by-step slew rate control.

### `G6_ENABLE_EFFICIENCY_MODE`
- **Type:** bool
- **Default:** `n`
- **Description:** Configures the tracking loop to search for the absolute minimum Watts per TH/s operating coordinate using a parallel power estimator surface and an exact algebraic fractional optimization solver.

### `G6_JTH_MAX_OUTER_ITERS`
- **Type:** int (3–15)
- **Default:** `7`
- **Description:** Maximum allowable outer fractional refinement steps for the analytical efficiency search loop.
