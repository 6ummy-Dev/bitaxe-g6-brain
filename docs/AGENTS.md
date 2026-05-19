# AGENTS.md — Engineering Principles & Safety Invariants

**G6 Brain v1.0.0-beta3 (Phase 1)**

This document defines the engineering rules and safety philosophy for the Bitaxe G6 Brain project.

---

## Core Philosophy

> **"Start safe. Learn. Then optimize."**

The brain must prioritize hardware longevity and stability over marginal hashrate gains.

---

## Current Safety Behavior (Phase 1 — fully active)

The following safety behaviors are **now implemented and enforced**:

1. **Deterministic Safety Priority**: Core overrides (thermal ceiling, voltage ripple, error thresholds) run post-optimization as the final step before returning recommendations, guaranteeing priority execution.

2. **Thermal Protection**: Hard `G6_TEMP_CEILING` (Kconfig, default 70 °C) with proactive scaling steps 5 °C below the limit.

3. **Voltage & Range Protection**: BM1370 hard limits + ripple clamping applied unconditionally on every operational loop.

4. **Internal Slew Limiting**: Slew-rate constraints are managed directly inside the tracking loop based on active physical state changes to keep empirical models coupled to real plant transitions.

5. **Numerical Stability**: 
   - **Joseph Form Updates**: Replaces conventional subtraction to mathematically ensure covariance matrix symmetry and positive semi-definiteness under floating-point constraints.
   - Centralized parameters for ridge regularization (`G6_RLS_RIDGE_EPSILON`), trace constraints (`G6_RLS_TRACE_MAX`), and adaptive Variable Forgetting Factor.

6. **Statistical Outlier Gating**: 3-Sigma innovation variance validation to automatically filter and reject corrupted sensor frames before updating estimators.

7. **Error Rate Handling**: Conservative back-off targets when NER exceeds `G6_NER_THRESHOLD` (Kconfig, default 2.5 %).

8. **Control Mode Enforcement**:
   - `G6_MODE_OBSERVE_ONLY`: Safety validation only, no setpoint mutations.
   - `G6_MODE_RECOMMEND` (safe default): Computes targets but does not apply mutations.
   - `G6_MODE_AUTO`: Active state machine optimization + safety.

9. **NVS Fingerprint Checkpointing**: Automatic background saving of model parameters every 5 minutes after initialization thresholds are satisfied.

10. **Fail-Closed Execution**: Safety handlers always run, even on samples rejected by data quality or outlier gates.

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
