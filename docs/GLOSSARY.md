# GLOSSARY.md — Terminology

**G6 Brain v1.0.0-beta2**

This glossary defines key terms used throughout the codebase, documentation, and discussions.

---

## Core Concepts

**G6 Brain**  
The self-optimizing control module for Bitaxe Gamma (BM1370) that uses Recursive Least Squares (RLS) quadratic modeling to dynamically tune frequency and voltage while enforcing safety constraints.

**RLS (Recursive Least Squares)**  
An adaptive filtering algorithm that continuously updates a model of hashrate as a quadratic function of frequency and voltage:  
`HR(f, v) = a·f² + b·v² + c·f·v + d·f + e·v + g`

**Quadratic Response Surface**  
The 6-coefficient mathematical model used by the brain to predict optimal operating points.

**Model Quality**  
A metric (0.0–1.0) indicating how well the current RLS model fits observed data.  
- > 0.85 → Excellent  
- 0.6–0.85 → Acceptable  
- < 0.6 → Poor (conservative mode)

**Cold Start**  
The initial phase after power-on or reset when the brain has insufficient data and operates conservatively while collecting samples.

**Warm Start / NVS Fingerprint**  
The learned RLS coefficients and best setpoint stored per physical chip in NVS. Enables fast recovery after reboot.

---

## Safety & State Machine

**BrainSampleState**  
Internal state machine that controls when samples are trusted:  
`IDLE → APPLY_CANDIDATE → SETTLE_WAIT → MEASURE_WINDOW → VALIDATE_SAMPLE → RLS_UPDATE → DECIDE_NEXT`

**Triple-8 Certification**  
Production validation protocol:  
- 8 hours at 105% best frequency  
- 8 hours with WiFi interference  
- 8 dirty power cycles  

**ΔT/dt (Delta Temperature over Delta Time)**  
Rate of temperature change. Used for proactive thermal protection.

**P-VUS (Predictive Voltage Undershoot)**  
Safety guard that blocks voltage increases if recent undershoots were detected.

**NER (Nonce Error Rate)**  
Hardware error rate reported by the BM1370. Used as a key input to the safety logic.

---

## Configuration & Limits

**CONFIG_G6_BRAIN_TEMP_CEILING**  
Hard thermal limit (°C). Default: 70.

**CONFIG_G6_BRAIN_MAX_FREQ_STEP**  
Maximum allowed frequency change per update (MHz). Default: 25.

**CONFIG_G6_BRAIN_MAX_VOLT_STEP**  
Maximum allowed voltage change per update (mV). Default: 12.5.

**Slew Rate**  
The controlled rate of change for frequency and voltage to prevent thermal shock and voltage droop.

---

## Telemetry & Monitoring

**best_f / best_v**  
The currently recommended safe operating point (frequency in MHz, voltage in mV).

**theta[6]**  
The 6 RLS coefficients of the quadratic model.

**P[6][6]**  
The covariance matrix used by RLS (used for numerical stability monitoring).

**update_count**  
Number of successful RLS updates performed since initialization.

---

## Project Terms

**Bitaxe Brains Project**  
The broader initiative to create swappable, modular optimization "brains" for Bitaxe hardware (G6 Brain is the flagship module).

**G6 Puzzle Extras**  
Optional nonce/work optimization features for improved mining puzzle handling (currently gated behind safety checks).

**Aerospace QA Hardening**  
The engineering approach of applying rigorous safety, signal integrity, and unhappy-path analysis typically found in aerospace electronics.

---

**Last updated:** May 2026  
**Maintainer:** 6ummy-Dev + Grok (xAI)
