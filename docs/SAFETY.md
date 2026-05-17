# G6 Brain Safety & Unhappy-Path Engineering — v1.0.0-beta2

**This is not a happy-path optimizer.**  
The G6 Brain is deliberately engineered to **fail safe** under real-world conditions on Bitaxe Gamma hardware.

---

## Core Safety Philosophy

The brain will **never** sacrifice hardware longevity or stability for marginal hashrate gains. Every decision passes through multiple protective layers before any frequency or voltage change is recommended.

---

## Implemented Safety Mechanisms (Current)

### 1. Thermal Protection
- Hard `temp_ceiling` (configurable, default 70 °C)
- Proactive thermal scaling when approaching the ceiling
- Automatic fallback to last known safe setpoint on thermal events

### 2. Voltage & Power Integrity
- Voltage range clamping
- Slew-rate limiting on frequency and voltage
- Basic voltage ripple/out-of-range detection

### 3. Mathematical & Model Integrity
- Ridge regularization + covariance symmetrization + diagonal clamping
- Hessian validity check in optimum solver
- `g6_brain_self_test()` for numerical sanity
- Model quality tracking

### 4. Sample Quality State Machine
- Settle time + measurement window
- Minimum share count validation (when provided)
- Temperature safety gate before accepting samples

### 5. NVS Wear-Leveling & Persistence
- Persists both `theta` and full covariance `P` matrix
- NVS readiness guard in `init()`

### 6. Error Rate Handling
- Conservative back-off when error rate exceeds threshold

---

## Phase 2 — Planned / Not Yet Implemented

The following features are **planned** but are **not yet implemented** in the current code:

- I2C/SPI 9-clock recovery / Guardian logic
- Hardware WDT monitoring on BM1370 `READY` pin
- Advanced P-VUS (Predictive Voltage Undershoot) detection
- Active `ΔT/dt` (temperature slope) detection and automated response
- RTC RAM counters for temporary temperature tracking
- Full power-cycle / "zombie ASIC" recovery logic

These items are targeted for **Phase 2**.

---

## When the Brain Refuses to Tune

The brain will stay conservative when:

- Temperature is at or above `temp_ceiling`
- Error rate is elevated
- Model quality is poor
- Sample has not passed quality gates

---

## Recommended Monitoring

Monitor the following values in production:

- `model_quality`
- `best_f` / `best_v`
- `temp_c`
- `update_count`
- Error rate

---

## File References

- Core logic: `components/g6_brain/g6_brain.c`
- Public API & constants: `components/g6_brain/g6_brain.h`
- Engineering principles: `AGENTS.md`

---

**Version:** v1.0.0-beta2 — May 2026  
**Philosophy:** Fail safe. Learn fast. Never compromise the hardware.
