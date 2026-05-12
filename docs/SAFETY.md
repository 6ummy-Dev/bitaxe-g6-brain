# G6 Brain Safety & Unhappy-Path Engineering — v1.0.0-beta1

**This is not a happy-path optimizer.**  
The G6 Brain is deliberately engineered to **fail safe** under real-world conditions on Bitaxe Gamma hardware.

---

## Core Safety Philosophy

The brain will **never** sacrifice hardware longevity or stability for marginal hashrate gains. Every decision passes through multiple protective layers before any frequency or voltage change is recommended.

---

## Implemented Safety Mechanisms

### 1. Thermal Protection
- Hard `temp_ceiling` (configurable, default 70 °C)
- Proactive `ΔT/dt` scaling — if temperature is rising faster than `MAX_TEMP_SLOPE`, the brain immediately reduces frequency/voltage
- Automatic fallback to last known safe setpoint on thermal runaway detection

### 2. Voltage & Power Integrity
- Real-time voltage undershoot history tracking
- `P-VUS` (Predictive Voltage Undershoot) guard — refuses to increase voltage if recent undershoots were detected
- Slew-rate limiting on both frequency (`MAX_FREQ_STEP`) and voltage (`MAX_VOLT_STEP`)
- BM1370 non-blocking error auto +5 mV compensation

### 3. Signal Integrity & ASIC Communication
- I2C/SPI re-init sequence on communication faults
- Hardware WDT hooks monitoring BM1370 `READY` pin
- Full power-cycle recovery on “zombie” ASIC state

### 4. Mathematical & Model Integrity
- Ridge regression + PSD safeguard to prevent singular matrices
- Denominator guards in the quadratic solver
- `g6_brain_self_test()` — synthetic data validation of the optimum finder + covariance condition number
- Model quality gating (`g6_brain_get_model_quality()`) — poor models (< 0.6) force conservative behavior

### 5. Sample Quality State Machine
The brain only trusts data after:
- Sufficient settle time (`SETTLE_MS`)
- Minimum valid share count (`MIN_VALID_SHARES`)
- Stable temperature slope

Invalid or noisy windows are discarded without affecting the model.

### 6. NVS Wear-Leveling & Persistence
- RTC RAM counters for temp tracking (reduces NVS writes)
- Explicit heap hygiene
- Per-chip fingerprint (theta + full covariance P) stored only when model has converged

### 7. Nonce / Puzzle Extras Safeguards
- `nonce_offset` and low-latency job mode are gated behind safety checks
- Never enabled if error rate or temperature is elevated

---

## Triple-8 Certification Path (Production Validation)

To certify a board + brain combination for long-term deployment:

| Test                        | Duration     | Conditions                              | Pass Criteria |
|-----------------------------|--------------|-----------------------------------------|---------------|
| **Stress**                  | 8 hours      | 105% of current best frequency          | No crashes, < 2% error rate, temp < ceiling |
| **WiFi Interference**       | 8 hours      | Intermittent WiFi drops + reconnects    | Brain recovers without manual intervention |
| **Dirty Power Cycles**      | 8 cycles     | Abrupt power removal while mining       | Clean recovery, NVS fingerprint intact, model quality preserved |

**How to run:**
1. Enable `CONFIG_G6_BRAIN_DEBUG=y`
2. Use the self-test + telemetry logging
3. Monitor `model_quality`, `best_f`, `best_v`, and error rate
4. Document results in your own validation log

Passing all three tests = **Triple-8 Certified** for that specific silicon + cooling + power setup.

---

## When the Brain Refuses to Tune

The brain will **lock to the last safe setpoint** and refuse further changes if any of the following are true:

- Temperature ≥ `temp_ceiling`
- Recent voltage undershoot detected
- Model quality < 0.6 after sufficient samples
- Error rate > configurable `ner_threshold`
- Communication fault with BM1370
- Rapid temperature rise (`ΔT/dt` violation)

In these states the brain continues to collect data and will automatically resume optimization once conditions improve.

---

## Recommended Monitoring (Production)

Log and alert on these values (exposed via WebUI / WebSocket):

- `model_quality`
- `best_f` / `best_v`
- `temp_c` + `ΔT/dt`
- Voltage undershoot count
- `update_count` (should increase steadily)
- `sample_state` (should cycle through `MEASURE_WINDOW` → `RLS_UPDATE` → `DECIDE_NEXT`)
- Covariance condition number (via self-test or future getter)

Set alerts if:
- `model_quality` stays < 0.65 for > 2 hours
- Temperature repeatedly hits ceiling
- Brain stops calling `update()` (task watchdog)
- Condition number > 1e6 (ill-conditioned covariance warning)

---

## File References

- Safety logic: fully integrated inside `components/g6_brain/g6_brain.c` (no separate g6_safety.* files)
- Core RLS + guards + self-test: `g6_brain.c`
- Constants + public API: `g6_brain.h` (RLS_*, BM1370_*, MAX_*_STEP, etc.)
- Agents / invariants: root `AGENTS.md`

---

## Summary

The G6 Brain does not just optimize — it **defends** your hardware.

Every line of the safety layer was written with the explicit goal of surviving real Bitaxe deployments: dirty power, marginal cooling, WiFi instability, and silicon variation.

**Use it. Trust it. But always keep an eye on the logs during the first 48 hours.**

---

**Version:** v1.0.0-beta1 — May 2026  
**Philosophy:** Fail safe. Learn fast. Never compromise the hardware.
