# GLOSSARY.md — Terminology

**G6 Brain v1.0.0-beta4**

This glossary defines key terms used throughout the codebase, documentation, and discussions.

---

## Core Concepts

**G6 Brain**  
The self-optimizing control module for Bitaxe Gamma (BM1370) that uses Recursive Least Squares (RLS) quadratic modeling to dynamically tune frequency and voltage while enforcing safety constraints.

**RLS (Recursive Least Squares)**  
An adaptive filtering algorithm that continuously updates a model of hashrate as a quadratic function of frequency and voltage.

**Quadratic Response Surface**  
The 6-coefficient mathematical model used by the brain to predict optimal operating points.

**Model Quality**  
A metric (0.0–1.0) indicating how well the current RLS model fits observed data.

**Cold Start**  
The initial phase after power-on or reset when the brain has insufficient data and operates conservatively.

**Warm Start / NVS Fingerprint**  
The learned RLS coefficients (`theta`) + full covariance matrix (`P`) + power model state, stored per physical chip in NVS.

**Joseph Form Update**  
A mathematically stabilized formulation of the covariance update equation that guarantees symmetry and positive semi-definiteness.

**Statistical Outlier Gating**  
A data validation layer that calculates expected innovation variance and rejects samples exceeding a 3-sigma statistical bound.

---

## Safety & Thermal (beta4)

**Two-tier Thermal Safety**  
The beta4 architecture that treats ASIC die temperature and VR regulator temperature differently:
- ASIC temperature gates learning.
- VR temperature only constrains setpoints in the safety layer.

**VR Temperature (`vr_temp_c`)**  
Voltage regulator temperature passed to `g6_brain_update()`. Use `G6_VR_TEMP_NO_SENSOR` (`-1.0f`) when no VR sensor is available.

**Proactive Zone**  
The temperature range below the hard ceiling where the brain begins gentle voltage reduction as an early warning.

**Hard Ceiling**  
The temperature at which aggressive setpoint reduction is applied (both voltage and frequency for VR).

---

## Safety & State Machine

**BrainSampleState**  
Internal state machine that controls when samples are trusted.

**NER (Nonce Error Rate)**  
Hardware error rate reported by the BM1370. Used as a key input to the safety logic.

**G6ControlMode**  
Runtime control modes:
- `G6_MODE_OBSERVE_ONLY`
- `G6_MODE_RECOMMEND` (safe default)
- `G6_MODE_AUTO`

**last_safety_status**  
Internal tracking of the most recent safety condition triggered (thermal, VR thermal, voltage, power sanity, outlier, etc.). Exposed via telemetry.

---

## Configuration & Limits

**G6_TEMP_CEILING**  
Hard thermal ceiling for the ASIC (°C).

**G6_VR_TEMP_CEILING**  
Hard thermal ceiling for the voltage regulator (°C).

**G6_VR_TEMP_PROACTIVE_MARGIN**  
Temperature margin below the VR ceiling where proactive voltage reduction begins.

**Slew Rate**  
The internally controlled rate of change for frequency and voltage.

---

**Last updated:** May 2026 (v1.0.0-beta4)
