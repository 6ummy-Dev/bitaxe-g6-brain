# G6 Brain Kconfig Options — v1.0.0-beta2

All options live under:  
**Component config → G6 Brain Configuration**

These control behavior, safety limits, debug output, and persistence.

---

## Core Options

### `G6_RLS_LAMBDA`
- **Type:** float (0.90–0.999)  
- **Default:** `0.98`  
- **Description:** Forgetting factor for the stabilized RLS. Lower = faster adaptation to silicon changes; higher = more stable model.  
- **Recommendation:** Start at 0.98. Drop to 0.95 only during initial 24 h tuning on a new chip.

### `G6_LOW_LATENCY_JOBS`
- **Type:** bool  
- **Default:** `y`  
- **Description:** Enables low-latency job mode hook (double-buffering placeholder for future stochastic nonce improvements).  
- **Recommendation:** Leave enabled unless you have a specific reason to disable.

---

## Safety & Thermal Limits

### `G6_TEMP_CEILING`
- **Type:** int (0–120)  
- **Default:** `70` (°C)  
- **Description:** Hard thermal limit. The brain will **never** increase frequency or voltage if ASIC temperature ≥ this value. It will also proactively down-tune on fast temperature rise (`ΔT/dt > G6_PROACTIVE_DFS_THRESHOLD`).  
- **Recommendation:**  
  - Stock heatsink + good airflow: `68–72`  
  - Upgraded heatsink / Noctua / immersion: `75–80`  
  - Always leave 3–5 °C headroom below your observed max stable temp.

### `G6_PROACTIVE_DFS_THRESHOLD`
- **Type:** float  
- **Default:** `2.0` (°C/s)  
- **Description:** Temperature rise rate that triggers proactive frequency/voltage derate.  
- **Recommendation:** Default is conservative and safe for most Gamma 602+ boards.

### `G6_DFS_STEP`
- **Type:** int (1–100)  
- **Default:** `25` (MHz)  
- **Description:** Maximum single-step frequency change the brain is allowed to request per update cycle (slew-rate limit).  
- **Recommendation:** Conservative: `15–20`. Aggressive first 24 h: `25–40`. Do not exceed 50 unless you have excellent power delivery and cooling.

### `G6_VOLTAGE_RIPPLE_MAX`
- **Type:** float  
- **Default:** `5.0` (%)  
- **Description:** Maximum allowed voltage ripple/undershoot before the brain logs a warning and clamps the setpoint.  
- **Recommendation:** Default 5% is excellent. Noisy USB-C or long cables: lower to 3–4%.

### `G6_NER_THRESHOLD`
- **Type:** float  
- **Default:** `0.001`  
- **Description:** P-VUS / NER (nonce error rate) threshold. Above this the brain triggers conservative back-off.  
- **Recommendation:** Leave at default unless you see excessive NER on your specific board.

### `G6_I2C_HARD_FAULT_THRESHOLD`
- **Type:** int (3–20)  
- **Default:** `5`  
- **Description:** Consecutive I2C timeouts before the brain considers the ASIC link degraded (future guardian hook).  
- **Recommendation:** Default is fine for most boards.

---

## Persistence & Learning

### `G6_NVS_WRITE_INTERVAL`
- **Type:** int  
- **Default:** `30000` (ticks)  
- **Description:** How often to persist the learned RLS coefficients + covariance to NVS (wear-leveling).  
- **Recommendation:** Default is good. Lower = more flash wear; higher = longer cold-start after power loss.

---

## PID / Future (currently stubs, wired for Phase 2)

### `G6_KP`, `G6_KI`, `G6_KD`
- **Type:** float  
- **Default:** `0.8`, `0.05`, `0.2`  
- **Description:** PID gains for future fan / thermal control integration (currently logged only).  
- **Recommendation:** Do not change unless you are actively developing Phase 2 fan control.

---

## Recommended Starting Configuration (v1.0.0-beta2)

For a typical Bitaxe Gamma 602+ with good cooling:

```
G6_RLS_LAMBDA=0.98
G6_TEMP_CEILING=70
G6_DFS_STEP=25
G6_VOLTAGE_RIPPLE_MAX=5.0
G6_NER_THRESHOLD=0.001
G6_PROACTIVE_DFS_THRESHOLD=2.0
G6_NVS_WRITE_INTERVAL=30000
G6_LOW_LATENCY_JOBS=y
```

After 24–48 h of stable operation you can experiment with slightly higher `G6_DFS_STEP` if your power delivery and cooling allow it.

---

## How These Options Interact with the Brain

- `TEMP_CEILING` + `PROACTIVE_DFS_THRESHOLD` + `VOLTAGE_RIPPLE_MAX` implement the **multi-layer predictive safety** system.
- `NVS_WRITE_INTERVAL` controls persistence of both theta and the full covariance matrix (true warm-start).
- All limits are **hard-enforced** inside `g6_brain_update()` — the brain will refuse unsafe requests even if your calling code tries to override them.
- `DEBUG` (if you add it via menuconfig) adds visibility into model quality and sample state machine.

---

## Next Steps

- **Full API & integration** → [API.md](API.md) and `docs/main_integration_v1.0_beta.c`
- **Installation guide** → [INSTALL.md](INSTALL.md)
- **Safety philosophy & edge cases** → Root `AGENTS.md`

**Pro tip:** After changing any Kconfig value, always do a full clean build:

```bash
idf.py fullclean
idf.py build
```

---

**Version:** v1.0.0-beta2 — May 2026  
**Maintainer:** 6ummy-Dev + Grok (xAI)
