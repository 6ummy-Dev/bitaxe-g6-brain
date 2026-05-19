# G6 Brain Safety & Unhappy-Path Engineering — v1.0.0-beta3 (Phase 1)

**This is not a happy-path optimizer.** The G6 Brain is deliberately engineered to **fail safe** under real-world conditions on Bitaxe Gamma hardware.

---

## Core Safety Philosophy (unchanged)

> **"Fail safe. Learn fast. Never compromise the hardware."**

The brain must prioritize hardware longevity and stability over marginal hashrate gains. Every decision passes through multiple protective layers before any frequency or voltage change is recommended.

---

## Implemented Safety Mechanisms (Current — Phase 1)

The following safety behaviors are **fully active**:

1. **Thermal Protection** Hard `G6_TEMP_CEILING` (Kconfig, default 70 °C) with proactive scaling 5 °C below the limit. Always runs via `goto safety_layer` pattern.

2. **Voltage & Range Protection** Hard BM1370 limits (400–950 MHz, 1050–1350 mV) + ripple/out-of-range clamping. Enforced on every update.

3. **Sample Quality State Machine** Settle time + measurement window + minimum share count validation (when provided). Thermal gate before any RLS update.

4. **Numerical Stability** Covariance symmetrization, diagonal clamping, ridge regularization (`G6_RLS_RIDGE_EPSILON`), trace monitoring (`G6_RLS_TRACE_MAX`), innovation gating, and Variable Forgetting Factor. All Kconfig-wired.

5. **Error Rate Handling** Conservative back-off + model reset when NER exceeds `G6_NER_THRESHOLD` (Kconfig, default 2.5%).

6. **Power Sanity Check** Rejects obviously impossible power values (<0 W or >100 W).

7. **Control Mode Enforcement**
   - `G6_MODE_OBSERVE_ONLY`: Safety only — no best_f/v mutation  
   - `G6_MODE_RECOMMEND` (safe default): Computes optimal but never mutates best_f/v  
   - `G6_MODE_AUTO`: Full optimizer
   All modes still run the complete safety layer.

8. **NVS Warm-Start**
   Full theta + P-matrix auto-saved every ~5 minutes after 10+ updates. Survives power cycles.

9. **Fail-Closed Design** Safety logic (thermal, NER, voltage, clamps) **always** executes even on rejected/invalid samples.

10. **Dual-Gated Optimizer Activation** The J/TH analytical solver is hard-gated by both `model_quality` and `power_model_quality`. It refuses to mutate setpoints if either model falls below 0.6, preventing noise-induced destabilization.

---

## Efficiency Reality (Phase 1)

True J/TH efficiency optimization is now implemented and available via Kconfig (`G6_ENABLE_EFFICIENCY_MODE`). It uses an exact $O(1)$ analytical minimum solver protected by the dual model-quality gates mentioned above.

---

## Phase 2 — Planned / Not Yet Implemented

The following features are **still planned** but are **not yet implemented**:

- Active ΔT/dt (temperature slope) detection and automated response (N=7 Thermal-Coupled RLS)
- Persistent Excitation (ESC Dither)
- PID fan control integration
- Advanced P-VUS (Predictive Voltage Undershoot)
- I2C/SPI guardian logic, hardware WDT, etc.

These items remain targeted for **Phase 2**.

---

## When the Brain Refuses to Tune

The brain stays conservative when:
- Temperature is near/at ceiling
- Error rate is elevated
- Model quality (or power model quality) is poor (< 0.6)
- Control mode is not `AUTO`
- Sample has not passed quality gates

---

## Recommended Monitoring

Monitor these values in production:
- `model_quality` and `power_model_quality`
- `control_mode`
- `best_f` / `best_v`
- `temp_c`
- NVS auto-save messages
- `g6_brain_get_cov_condition()`

---

## File References

- Core logic: `components/g6_brain/g6_brain.c` (fully Kconfig + control_mode wired)
- Public API & constants: `components/g6_brain/g6_brain.h`
- Engineering principles: `AGENTS.md`

---

**Version:** v1.0.0-beta3 (Phase 1 — May 2026)  
**Philosophy:** Fail safe. Learn fast. Never compromise the hardware.
