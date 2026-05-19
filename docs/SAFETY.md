# G6 Brain Safety & Unhappy-Path Engineering — v1.0.0-beta3

**This is not a happy-path optimizer.**  
The G6 Brain is deliberately engineered to **fail safe** under real-world conditions on Bitaxe Gamma hardware.

---

## Core Safety Philosophy

> **"Start safe. Learn. Then optimize. ⚡"**

The brain must prioritize hardware longevity and stability over marginal hashrate gains. Every decision passes through multiple protective layers before any frequency or voltage change is recommended.

---

## Implemented Safety Mechanisms (Current — beta3)

The following safety behaviors are **fully active**:

1. **Thermal Protection**  
   Hard `G6_TEMP_CEILING` (Kconfig) with proactive scaling 5 °C below the limit. Always runs via `goto safety_layer` pattern.

2. **Voltage & Range Protection**  
   Hard BM1370 limits (400–950 MHz, 1050–1350 mV) + ripple/out-of-range clamping on every update.

3. **Sample Quality State Machine**  
   Settle time + measurement window + minimum share count validation + thermal gate before any RLS update.

4. **Numerical Stability**  
   Covariance symmetrization, diagonal clamping, ridge regularization, trace monitoring, innovation gating, and Variable Forgetting Factor. All Kconfig-wired.

5. **Error Rate Handling**  
   Conservative back-off + model quality reduction when NER exceeds `G6_NER_THRESHOLD`.

6. **Power Sanity Check**  
   Rejects obviously impossible power values.

7. **Control Mode Enforcement**  
   - `G6_MODE_OBSERVE_ONLY`: Safety only — no best_f/v mutation  
   - `G6_MODE_RECOMMEND` (default): Computes optimal but never mutates setpoints  
   - `G6_MODE_AUTO`: Full optimizer  
   All modes still run the complete safety layer.

8. **NVS Warm-Start**  
   Full theta + P-matrix + power model auto-saved. Survives power cycles.

9. **Fail-Closed Design**  
   Safety logic (thermal, NER, voltage, clamps) **always** executes even on rejected/invalid samples.

10. **Model Quality Gates (beta3)**  
    The J/TH efficiency optimizer is protected by **both** `model_quality >= 0.6` **and** `power_model_quality >= 0.6`. On cold boots or early learning, efficiency optimization is safely skipped.

---

## Efficiency Reality (beta3)

- The brain is a **safe hashrate maximizer** by default.
- When `G6_ENABLE_EFFICIENCY_MODE` is enabled, it can optimize for true **J/TH** using a fast **analytical Dinkelbach solver** (exact 2×2 quadratic minimization).
- This is still **opt-in** and guarded. It is not enabled by default.
- The solver is mathematically stronger than the previous grid-search approach, but it is still relatively new. Thorough field testing is recommended before relying on it in production.

---

## When the Brain Refuses to Tune

The brain stays conservative when:
- Temperature is near/at ceiling
- Error rate is elevated
- `model_quality` or `power_model_quality` is low
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

- Core logic: `components/g6_brain/g6_brain.c`
- Public API & constants: `components/g6_brain/g6_brain.h`
- Engineering principles: `AGENTS.md`

---

**Version:** v1.0.0-beta3 (May 2026)  
**Philosophy:** Start safe. Learn. Then optimize. ⚡
