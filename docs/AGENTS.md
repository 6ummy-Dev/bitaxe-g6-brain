# AGENTS.md — Engineering Principles & Safety Invariants

**G6 Brain v1.0.0-beta5**

This document defines the engineering rules and safety philosophy for the Bitaxe G6 Brain project.

---

## Core Philosophy

> **"Start safe. Learn. Then optimize."**

The brain must prioritize hardware longevity and stability over marginal hashrate gains.

---

## Current Safety Behavior (beta5)

The following safety behaviors are **now implemented and enforced**:

1. **Deterministic Safety Priority**: Core overrides (thermal ceiling, voltage ripple, error thresholds, power sanity) run post-optimization as the final step before returning recommendations.

2. **Two-tier Thermal Protection**:
   - **ASIC die temperature** gates RLS learning updates.
   - **VR regulator temperature** runs only in the safety layer as a proactive + hard constraint on setpoints.

3. **Thermal Protection**: Hard `G6_TEMP_CEILING` with proactive scaling. VR has its own independent ceiling (`G6_VR_TEMP_CEILING`) and proactive margin.

4. **Voltage & Range Protection**: BM1370 hard limits + ripple clamping applied unconditionally on every operational loop.

5. **Internal Slew Limiting**: Slew-rate constraints are managed directly inside the tracking loop.

6. **Numerical Stability**:
   - Joseph Form covariance updates
   - Ridge regularization
   - Trace constraints
   - Adaptive Variable Forgetting Factor

7. **Statistical Outlier Gating**: 3-Sigma innovation variance validation.

8. **Error Rate Handling**: Conservative back-off when NER exceeds `G6_NER_THRESHOLD`.

9. **Control Mode Enforcement**:
   - `G6_MODE_OBSERVE_ONLY`
   - `G6_MODE_RECOMMEND` (safe default)
   - `G6_MODE_AUTO`

10. **NVS Fingerprint Checkpointing**: Background saving of model parameters.

11. **Fail-Closed Execution**: Safety handlers always run, even on samples rejected by data quality or outlier gates.

---

## Efficiency Optimization

True J/TH efficiency tracking uses a discrete secondary power model and an exact $O(1)$ analytical fraction minimizer. Solver updates are strictly gated by independent model convergence thresholds (`model_quality >= 0.6` and `power_model_quality >= 0.6`).

---

## Planned Invariants (Phase 2)

- Active thermal slope tracking (`ΔT/dt`)
- Persistent Excitation (ESC Dither)
- PID fan control coupling
- Predictive Voltage Undershoot (P-VUS) protections

---

## Forbidden Patterns

- Never bypass safety priority checks.
- Never clear tracking matrices without valid re-initialization invariants.
- Never use unbound floating-point computations or unregularized updates.
