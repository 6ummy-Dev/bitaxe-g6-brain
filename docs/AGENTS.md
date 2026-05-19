# AGENTS.md — Engineering Principles & Safety Invariants

**G6 Brain v1.0.0-beta3 (Phase 1)**

This document defines the engineering rules and safety philosophy for the Bitaxe G6 Brain project.

---

## Core Philosophy (unchanged)

> **"Fail safe. Learn fast. Never compromise the hardware."**

The brain must prioritize hardware longevity and stability over marginal hashrate gains.

---

## Current Safety Behavior (Phase 1 — fully active)

The following safety behaviors are **now implemented and enforced**:

1. **Thermal Protection** Hard `G6_TEMP_CEILING` (Kconfig, default 70 °C) with proactive scaling. Always executes via `goto safety_layer`.

2. **Voltage & Range Protection** BM1370 hard limits + ripple clamping on every cycle.

3. **Sample Quality State Machine** Settle + measure windows + share count validation + thermal gate before RLS updates.

4. **Numerical Stability** Covariance symmetrization, ridge regularization (`G6_RLS_RIDGE_EPSILON`), trace monitoring (`G6_RLS_TRACE_MAX`), innovation gating, Variable Forgetting Factor — all Kconfig-driven.

5. **Error Rate Handling** Conservative back-off when NER > `G6_NER_THRESHOLD` (Kconfig, default 2.5 %).

6. **Control Mode Enforcement**
   - `G6_MODE_OBSERVE_ONLY`: safety only, no setpoint mutation  
   - `G6_MODE_RECOMMEND` (safe default): computes optimal but never mutates `best_f`/`best_v`  
   - `G6_MODE_AUTO`: full optimizer + safety  
   Enforced in `g6_brain_update()` and `g6_brain_get_optimal()`.

7. **NVS Warm-Start**
   Full theta + P-matrix auto-saved every ~5 minutes after 10+ updates. True per-chip persistence.

8. **Fail-Closed Design** Safety layer runs **even on rejected/invalid samples**.

9. **Power Sanity & Input Validation** Full defensive checks on every `update()` call.

---

## Efficiency Reality (Phase 1)

True J/TH efficiency optimization is now implemented via a separate power surface model and an $O(1)$ analytical minimum solver. It is strictly gated by both `model_quality` and `power_model_quality`.

---

## Planned / aspirational invariants (Phase 2)

- Active thermal slope detection (`ΔT/dt`) and automated response
- Persistent Excitation (ESC Dither) to prevent covariance collapse
- PID fan control integration
- Advanced P-VUS (Predictive Voltage Undershoot)
- Full power-cycle / zombie ASIC recovery logic

These remain **not yet implemented**.

---

## Design Principles

- Numerical Stability Over Speed
- Fail-Closed Philosophy — When in doubt, stay safe.
- Defensive Programming — Assume bad data, bad power, bad conditions.
- Transparency — Internal state (quality, mode, covariance) is observable.
- Kconfig + runtime configurability

---

## Forbidden Patterns

- Never bypass thermal or voltage limits.
- Never remove safety checks to chase performance.
- Never apply large frequency/voltage changes without slew limiting in the integration layer.

---

## Contribution Rules

All changes must respect the current safety behavior and clearly document any new planned invariants.

---

**Last updated:** May 2026 (v1.0.0-beta3)  
**Maintainer:** 6ummy-Dev + Grok (xAI)
