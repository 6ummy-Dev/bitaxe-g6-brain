# G6 Brain Kconfig Options — v1.0 Beta

All options live under:  
**Component config → G6 Brain Configuration**

These control behavior, safety limits, debug output, and persistence.

---

## Core Options

### `CONFIG_G6_BRAIN_ENABLE`
- **Type:** bool  
- **Default:** `y`  
- **Description:** Master switch for the entire G6 Brain component.  
- **When to disable:** Only for pure benchmarking or when you want to completely remove the brain without deleting files.  
- **Effect:** When `n`, all brain code is compiled out (zero overhead).

---

### `CONFIG_G6_BRAIN_DEBUG`
- **Type:** bool  
- **Default:** `n`  
- **Description:** Enables verbose `ESP_LOGD` output from the RLS core, safety module, and sample state machine.  
- **Recommended:**  
  - `y` during first integration and tuning  
  - `n` in production (saves ~2–3 KB flash and reduces log spam)

---

## Safety & Thermal Limits

### `CONFIG_G6_BRAIN_TEMP_CEILING`
- **Type:** int (0–120)  
- **Default:** `70` (°C)  
- **Description:** Hard thermal limit. The brain will **never** increase frequency or voltage if ASIC temperature ≥ this value. It will also proactively down-tune if temperature is rising too fast (`ΔT/dt`).  
- **Recommendation:**  
  - Stock heatsink + good airflow: `68–72`  
  - Upgraded heatsink / Noctua / immersion: `75–80`  
  - Always leave 3–5 °C headroom below your observed max stable temp.

---

### `CONFIG_G6_BRAIN_MAX_FREQ_STEP`
- **Type:** int (1–100)  
- **Default:** `25` (MHz)  
- **Description:** Maximum single-step frequency change the brain is allowed to request per update cycle.  
- **Purpose:** Prevents thermal shock and voltage droop on the BM1370.  
- **Recommendation:**  
  - Conservative / long-term stability: `15–20`  
  - Aggressive tuning (first 24 h): `25–40`  
  - Do **not** exceed `50` unless you have excellent power delivery and cooling.

---

### `CONFIG_G6_BRAIN_MAX_VOLT_STEP`
- **Type:** float (0.5–50.0)  
- **Default:** `12.5` (mV)  
- **Description:** Maximum single-step voltage change (in millivolts).  
- **Purpose:** Protects against voltage undershoot and BM1370 brown-out.  
- **Recommendation:**  
  - Default `12.5` is excellent for most boards.  
  - Very clean power: you can raise to `15–20`.  
  - Noisy USB-C or long cables: lower to `8–10`.

---

## Persistence & Learning

### `CONFIG_G6_BRAIN_NVS_FINGERPRINT`
- **Type:** bool  
- **Default:** `y`  
- **Description:** Enables per-chip NVS storage of learned RLS coefficients (`theta`), best setpoint, and model quality.  
- **Benefits:**  
  - Warm-start on every reboot (no 10–15 min cold-start)  
  - Each physical Bitaxe learns its own silicon characteristics  
- **Recommendation:** **Always leave enabled** unless you are doing controlled lab experiments.

**Storage cost:** ~256 bytes per chip in NVS (negligible).

---

## Advanced / Internal (usually leave default)

| Option                        | Default     | When you might change it |
|-------------------------------|-------------|--------------------------|
| `CONFIG_G6_BRAIN_RLS_LAMBDA`  | 0.98        | Lower (0.95) for faster adaptation, higher (0.999) for very stable environments |
| `CONFIG_G6_BRAIN_RIDGE`       | 1e-4        | Increase only if you see matrix singularity warnings in debug logs |
| `CONFIG_G6_BRAIN_SETTLE_SEC`  | 8           | How long to wait after applying new settings before collecting samples |
| `CONFIG_G6_BRAIN_MIN_SHARES`  | 20          | Minimum valid shares before trusting a sample window |

These are exposed for power users and future tuning. Most deployments should never touch them.

---

## Recommended Starting Configuration (v1.0 Beta)

For a typical Bitaxe Gamma with good cooling:

```
CONFIG_G6_BRAIN_ENABLE=y
CONFIG_G6_BRAIN_DEBUG=n          # switch to y only while tuning
CONFIG_G6_BRAIN_TEMP_CEILING=70
CONFIG_G6_BRAIN_MAX_FREQ_STEP=25
CONFIG_G6_BRAIN_MAX_VOLT_STEP=12.5
CONFIG_G6_BRAIN_NVS_FINGERPRINT=y
```

After 24–48 hours of stable operation you can experiment with slightly higher `MAX_FREQ_STEP` if your power delivery and cooling allow it.

---

## How These Options Interact with the Brain

- `TEMP_CEILING` + `MAX_*_STEP` together implement the **predictive safety layer**.
- `NVS_FINGERPRINT` makes the RLS model persistent across reboots.
- `DEBUG` adds visibility into the quadratic model quality and sample state machine (`BRAIN_STATE_*`).
- All limits are **hard-enforced** inside `g6_brain_update()` — the brain will refuse unsafe requests even if your calling code tries to override them.

---

## Next Steps

- **Full API** → [API.md](API.md)
- **Installation guide** → [INSTALL.md](INSTALL.md)
- **Production example** → `docs/main_integration_v1.0_beta.c`
- **Safety & edge-case handling** → Root `AGENTS.md` + `g6_safety.h`

---

**Pro tip:** After changing any Kconfig value, always do a full clean build:

```bash
idf.py fullclean
idf.py build
```

---

**Version:** v1.0 Beta — May 2026  
**Maintainer:** 6ummy-Dev + Grok (xAI)
