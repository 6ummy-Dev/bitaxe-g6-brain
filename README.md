# bitaxe-g6-brain v1.6.0

**Pure Math RLS Quadratic Optimizer for Bitaxe Gamma 602**

The cleanest, most mathematically rigorous on-device brain for finding the optimal frequency/voltage sweet spot on BM1370 ASICs.

## Features (v1.6.0)
- Full Recursive Least Squares (RLS) with forgetting factor
- Analytical closed-form maximum finding + Hessian validation
- NVS persistence (model survives reboots)
- Numerical stability (ridge + periodic covariance reset)
- Live model quality metric
- Complete `auto_step` with intelligent recommendations
- Zero bloat — pure math and safety

## Quick Start

```c
// In your main telemetry loop (every 20-40s)
g6_brain_update(&brain, freq_mhz, voltage_mv, hashrate, power, temp, error_pct);
g6_brain_auto_step(&brain, freq_mhz, voltage_mv);
```

Call `g6_brain_load_from_nvs(&brain)` after init and `g6_brain_save_to_nvs(&brain)` periodically.

## Math Foundation
Models `HR(f,v) = a·f² + b·v² + c·f·v + d·f + e·v + g`
Solves ∇HR = 0 with full second-derivative check to guarantee a true maximum.

**This is the best version yet.**

Flash it. Let the math speak.