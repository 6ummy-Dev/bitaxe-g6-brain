# AGENTS.md — Engineering Principles & Safety Invariants

**G6 Brain v1.0.0-beta2**

This document defines the engineering rules and safety philosophy for the G6 Brain project.

---

## Core Philosophy

> **"Fail safe. Learn fast. Never compromise the hardware."**

The brain must prioritize hardware longevity and stability over marginal hashrate gains.

---

## Current Safety Behavior

The following safety behaviors are **currently implemented**:

1. **Thermal Protection**  
   Hard thermal ceiling with proactive scaling near the limit.

2. **Voltage & Range Protection**  
   Hard limits on frequency and voltage (BM1370 safe ranges). Slew-rate limiting is recommended at the integration level.

3. **Sample Quality Gate**  
   Samples must pass settle time and (when provided) minimum share count before being used for RLS updates.

4. **Numerical Stability**  
   Covariance symmetrization, diagonal clamping, ridge regularization, and trace monitoring on every RLS update.

5. **Error Rate Handling**  
   Conservative back-off when error rate exceeds the configured threshold.

6. **Fail-Closed Design**  
   Safety logic runs even on rejected/invalid samples via the safety layer pattern.

---

## Planned / aspirational invariants (not yet fully enforced)

These are desired behaviors for future versions:

- **Model Quality Gating**  
  When `model_quality` is low (< 0.6) after sufficient samples, the brain should remain conservative and avoid aggressive setpoint changes.  
  → *Currently not strictly enforced in `get_optimal()`. Targeted for improvement.*

- **Advanced Thermal Slope Detection**  
  Active `ΔT/dt` monitoring and automated response.  
  → *Planned for Phase 2.*

---

## Design Principles

- **Numerical Stability Over Speed**
- **Fail-Closed Philosophy** — When in doubt, stay safe.
- **Defensive Programming** — Assume bad data, bad power, and bad conditions.
- **Transparency** — Important internal state should be observable.

---

## Forbidden Patterns

- Never bypass thermal or voltage limits "for testing".
- Never remove safety checks to chase small performance gains.
- Never apply large frequency/voltage changes without slew limiting at the integration layer.

---

## Contribution Rules

All changes should respect the current safety behavior and clearly document any new planned invariants.

---

**Last updated:** May 2026  
**Maintainer:** 6ummy-Dev + Grok (xAI)
