# AGENTS.md — Engineering Principles & Safety Invariants

**G6 Brain v1.0 Beta**

This document defines the non-negotiable engineering rules and safety invariants for the G6 Brain project. All code, documentation, and future contributions must respect these principles.

---

## Core Philosophy

> **"Fail safe. Learn fast. Never compromise the hardware."**

The brain must **never** trade hardware longevity or stability for marginal hashrate gains. Every optimization decision passes through multiple protective layers.

---

## Safety Invariants (Non-Negotiable)

1. **Thermal Ceiling is Absolute**  
   The brain will never recommend a frequency or voltage increase if `temp_c >= CONFIG_G6_BRAIN_TEMP_CEILING`. Proactive `ΔT/dt` scaling is mandatory.

2. **Voltage Undershoot Protection**  
   Any detected voltage undershoot immediately blocks further voltage increases and triggers conservative fallback.

3. **Slew Rate Limits are Hard**  
   Frequency changes are capped at `CONFIG_G6_BRAIN_MAX_FREQ_STEP`. Voltage changes are capped at `CONFIG_G6_BRAIN_MAX_VOLT_STEP`. These limits are enforced in `g6_brain_update()`.

4. **Model Quality Gating**  
   If `model_quality < 0.6` after sufficient samples, the brain must refuse to apply new setpoints and remain in conservative mode.

5. **Sample Quality Gate**  
   No RLS update is performed unless the sample passes settle time, minimum share count, and stable temperature slope checks.

6. **NVS Wear-Leveling**  
   Persistent writes are minimized. RTC RAM counters are used for temporary state. NVS fingerprint is only written when the model has meaningfully converged.

7. **Single-Threaded State**  
   `G6BrainState` is updated from a single task. No mutex is currently required. Any future multi-tasking must add proper synchronization.

8. **Self-Test Integrity**  
   `g6_brain_self_test()` must pass before trusting any new optimization logic.

---

## Design Principles

- **Modularity First**  
  The `G6BrainInterface` must remain clean. Future brains (ML, heuristic, multi-ASIC) must be swappable without touching ESP-Miner core.

- **Numerical Stability Over Speed**  
  Ridge regularization, PSD safeguard, and trace monitoring are mandatory. Better a slightly slower but stable model than a fast unstable one.

- **Fail-Closed Philosophy**  
   When in doubt, the brain must default to the last known safe operating point.

- **Telemetry is Sacred**  
  All important internal state (`theta`, `P`, `model_quality`, `best_f`, `best_v`, `sample_state`) must be exposed for WebUI/WebSocket monitoring.

- **No Happy-Path Only**  
  Every code path must consider unhappy scenarios: dirty power, WiFi drops, thermal runaway, communication faults, noisy hashrate readings.

---

## Forbidden Patterns

- Never bypass slew limits or thermal ceiling "for testing"
- Never write to NVS on every update cycle
- Never apply `opt_f` / `opt_v` without user-controlled confirmation layer in production code
- Never remove safety checks to "improve performance"

---

## Contribution Rules

Any pull request that violates these invariants will be rejected.

All new features must include:
- Updated safety analysis
- Test coverage for new logic
- Documentation updates in the relevant `.md` files

---

**Last updated:** May 2026  
**Maintainer:** 6ummy-Dev + Grok (xAI)
