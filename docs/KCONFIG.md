# G6 Brain Kconfig Options — v1.0.0-beta2 (Phase 0 — fully wired)

All options live under:  
**Component config → G6 Brain Configuration**

These are **now live** and read at runtime via `sdkconfig.h`.

---

## RLS Optimizer

### `G6_RLS_LAMBDA_MIN`
- **Type:** int (900–999)  
- **Default:** `950`  
- **Description:** Minimum forgetting factor ×1000 (0.950). Higher = more stable model, lower = faster adaptation to silicon variation.

### `G6_RLS_RIDGE_EPSILON`
- **Type:** int (1–1000)  
- **Default:** `10`  
- **Description:** Ridge regularization ×1e-6 added to P-matrix diagonal. Prevents covariance collapse.

### `G6_RLS_TRACE_MAX`
- **Type:** int  
- **Default:** `10000000`  
- **Description:** Maximum allowed trace(P). Exceeding forces conservative/cold-start behavior.

---

## Safety & Thermal

### `G6_TEMP_CEILING`
- **Type:** int  
- **Default:** `70` (°C)  
- **Description:** Hard thermal ceiling. Proactive derating starts 5 °C below this value.

### `G6_NER_THRESHOLD`
- **Type:** int (×100)  
- **Default:** `250` (= 2.5%)  
- **Description:** Nonce Error Rate threshold. Triggers model reset + conservative back-off.

---

## Optimization Behavior

### `G6_DFS_STEP_MHZ`
- **Type:** int  
- **Default:** `25`  
- **Description:** Frequency step size used internally for optimization and slew-rate limiting.

---

## Phase 0 Changes Now Active

- **Control Modes** (`G6_MODE_OBSERVE_ONLY` / `RECOMMEND` / `AUTO`) are now **enforced** in `g6_brain_update()` and `g6_brain_get_optimal()`. Default is `RECOMMEND`.
- **NVS auto-save** — full theta + P matrix is now saved every ~5 minutes after 10 updates (warm-start works out of the box).
- **Kconfig fallbacks** — all values are now read from menuconfig with safe compile-time defaults.
- **Efficiency note** (honesty patch): The brain is currently a **safe hashrate maximizer** (quadratic argmax of HR(f,v) with hard safety clamps). True J/TH efficiency optimization (separate power model) is planned for Phase 1.

---

## Recommended Starting Config (Gamma 602+ with good cooling)

```
G6_RLS_LAMBDA_MIN=950
G6_TEMP_CEILING=70
G6_NER_THRESHOLD=250
G6_DFS_STEP_MHZ=25
G6_RLS_RIDGE_EPSILON=10
G6_RLS_TRACE_MAX=10000000
```

After changing any Kconfig value, always do a full clean build:

```bash
idf.py fullclean && idf.py build
```

---

**Next Steps**  
- Full API reference → [API.md](API.md)  
- Recommended integration example → [INTEGRATION_EXAMPLE.c](INTEGRATION_EXAMPLE.c)  
- Safety philosophy → [AGENTS.md](../AGENTS.md)

**Version:** v1.0.0-beta2 (Phase 0 fixes applied — May 2026)
