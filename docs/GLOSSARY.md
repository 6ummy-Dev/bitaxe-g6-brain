# GLOSSARY.md — Terminology

**G6 Brain v1.0.0-beta3 (Phase 1 complete + Phase 2 early)**

This glossary defines key terms used throughout the codebase, documentation, and discussions.

---

## Core Concepts

**G6 Brain** The self-optimizing control module for Bitaxe Gamma (BM1370) that uses Recursive Least Squares (RLS) quadratic modeling to dynamically tune frequency and voltage while enforcing safety constraints.

**RLS (Recursive Least Squares)** An adaptive filtering algorithm that continuously updates a model of hashrate as a quadratic function of frequency and voltage:  
`HR(f, v) = a·f² + b·v² + c·f·v + d·f + e·v + g`

**Quadratic Response Surface** The 6-coefficient mathematical model used by the brain to predict optimal operating points.

**Model Quality** A metric (0.0–1.0) indicating how well the current RLS model fits observed data.  
- > 0.85 → Excellent  
- 0.6–0.85 → Acceptable  
- < 0.6 → Poor (conservative mode)

**Cold Start** The initial phase after power-on or reset when the brain has insufficient data and operates conservatively while collecting samples.

**Warm Start / NVS Fingerprint** The learned RLS coefficients (`theta`) + full covariance matrix (`P`) + power model state, stored per physical chip in NVS. Enables warm-start after reboot. Auto-saved after 10+ successful updates and once `NVS_SAVE_INTERVAL_TICKS = 300000` ticks have elapsed since the last save (at default 100 Hz tick rate that is ~50 minutes; at 1 kHz it is ~5 minutes).

**Joseph Form Update** A mathematically stabilized formulation of the covariance update equation. Guarantees matrix symmetry and positive semi-definiteness under floating-point precision limits, preventing numerical divergence.

**Statistical Outlier Gating** A data validation layer that calculates expected innovation variance. Telemetry errors that exceed a 3-sigma statistical bound are blocked to preserve surface model accuracy.

---

## Safety & State Machine

**BrainSampleState** Internal state machine that controls when samples are trusted:  
`IDLE → APPLY_CANDIDATE → SETTLE_WAIT → MEASURE_WINDOW → VALIDATE_SAMPLE → RLS_UPDATE → DECIDE_NEXT`

**Triple-8 Certification** Production validation protocol: 8 hours at 105% best frequency, 8 hours with WiFi interference, and 8 dirty power cycles.

**ΔT/dt (Delta Temperature over Delta Time)** Rate of temperature change. Used for proactive thermal protection (Phase 2).

**P-VUS (Predictive Voltage Undershoot)** Safety guard that blocks voltage increases if recent undershoots were detected (Phase 2).

**NER (Nonce Error Rate)** Hardware error rate reported by the BM1370. Used as a key input to the safety logic (`G6_NER_THRESHOLD`).

**G6ControlMode** Runtime control modes:  
- `G6_MODE_OBSERVE_ONLY` — safety only  
- `G6_MODE_RECOMMEND` — compute optimal but never mutate setpoints (safe default)  
- `G6_MODE_AUTO` — full optimizer

---

## Configuration & Limits (Kconfig — fully wired)

**G6_TEMP_CEILING** Hard thermal ceiling (°C). Default: 70. Configurable via `menuconfig`.

**G6_NER_THRESHOLD** Nonce Error Rate threshold (×100). Default: 250 (= 2.5 %).

**G6_DFS_STEP_MHZ** Frequency step size (MHz). Used for internal step-by-step loop constraint regulation.

**Slew Rate** The internally controlled rate of change for frequency and voltage to mitigate thermal shock and supply voltage droop.

---

## Telemetry & Monitoring

**best_f / best_v** The currently recommended safe operating point (frequency in MHz, voltage in mV). Only mutated in `AUTO` mode.

**theta[6]** The 6 RLS coefficients of the quadratic model.

**P[6][6]** The covariance matrix used by RLS.

**update_count** Number of successful RLS updates performed since initialization.

---

**Last updated:** May 2026 (v1.0.0-beta3)  
**Maintainer:** 6ummy-Dev
