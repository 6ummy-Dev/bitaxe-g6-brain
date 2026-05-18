# G6 Brain Safety & Unhappy-Path Engineering — v1.0.0-beta2 (Phase 0 — fully wired)

**This is not a happy-path optimizer.**  
The G6 Brain is deliberately engineered to **fail safe** under real-world conditions on Bitaxe Gamma hardware.

---

## Core Safety Philosophy (unchanged)

> **"Fail safe. Learn fast. Never compromise the hardware."**

The brain must prioritize hardware longevity and stability over marginal hashrate gains. Every decision passes through multiple protective layers before any frequency or voltage change is recommended.

---

## Implemented Safety Mechanisms (Current — Phase 0)

The following safety behaviors are **now fully active** after Phase 0 fixes:

1. **Thermal Protection**  
   Hard `G6_TEMP_CEILING` (Kconfig, default 70 °C) with proactive scaling 5 °C below the limit. Always runs via `goto safety_layer` pattern.

2. **Voltage & Range Protection**  
   Hard BM1370 limits (400–950 MHz, 1050–1350 mV) + ripple/out-of-range clamping. Enforced on every update.

3. **Sample Quality State Machine**  
   Settle time + measurement window + minimum share count validation (when provided). Thermal gate before any RLS update.

4. **Numerical Stability**  
   Covariance symmetrization, diagonal clamping, ridge regularization (`G6_RLS_RIDGE_EPSILON`), trace monitoring (`G6_RLS_TRACE_MAX`), innovation gating, and Variable Forgetting Factor. All Kconfig-wired.

5. **Error Rate Handling**  
   Conservative back-off + model reset when NER exceeds `G6_NER_THRESHOLD` (Kconfig, default 2.5%).

6. **Power Sanity Check**  
   Rejects obviously impossible power values (<0 W or >100 W).

7. **Control Mode Enforcement** (Phase 0)  
   - `G6_MODE_OBSERVE_ONLY`: Safety only — no best_f/v mutation  
   - `G6_MODE_RECOMMEND` (new safe default): Computes optimal but never mutates best_f/v  
   - `G6_MODE_AUTO`: Full optimizer (original behavior)  
   All modes still run the complete safety layer.

8. **NVS Warm-Start** (Phase 0)  
   Full theta + P-matrix auto-saved every ~5 minutes after 10+ updates. Survives power cycles.

9. **Fail-Closed Design**  
   Safety logic (thermal, NER, voltage, clamps) **always** executes even on rejected/invalid samples.

---

## Efficiency Reality (Phase 0 Honesty Patch)

The brain is currently a **safe hashrate maximizer** (quadratic argmax of HR(f,v) with hard safety clamps).  
True J/TH efficiency optimization (separate power model + predicted efficiency surface) is targeted for Phase 1.

---

## Phase 2 — Planned / Not Yet Implemented

The following features are **still planned** but are **not yet implemented**:

- Active ΔT/dt (temperature slope) detection and automated response
- PID fan control integration
- Advanced P-VUS (Predictive Voltage Undershoot)
- I2C/SPI guardian logic, hardware WDT, etc.

These items remain targeted for **Phase 2**.

---

## When the Brain Refuses to Tune

The brain stays conservative when:
- Temperature is near/at ceiling
- Error rate is elevated
- Model quality is poor (< 0.6)
- Control mode is not `AUTO`
- Sample has not passed quality gates

---

## Recommended Monitoring (Phase 0)

Monitor these values in production:
- `model_quality`
- `control_mode`
- `best_f` / `best_v`
- `temp_c`
- NVS auto-save messages
- `g6_brain_get_cov_condition()`

---

## File References

- Core logic: `components/g6_brain/g6_brain.c` (now fully Kconfig + control_mode wired)
- Public API & constants: `components/g6_brain/g6_brain.h`
- Engineering principles: `AGENTS.md`

---

**Version:** v1.0.0-beta2 (Phase 0 fixes applied — May 2026)  
**Philosophy:** Fail safe. Learn fast. Never compromise the hardware.
