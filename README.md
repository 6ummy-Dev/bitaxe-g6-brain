# bitaxe-g6-brain (v1.5.5 Pure Math Core)

**The leanest, most mathematically rigorous RLS quadratic optimizer for Bitaxe Gamma 602+ (BM1370 ASIC).**

No puzzle extras. No nonce heuristics. No on-device solver demos. No bloat.  
Just the best possible real-time math model for finding the optimal frequency/voltage sweet spot on your silicon.

## What it does (pure math)

- Fits **HR(f, v) = a f² + b v² + c f v + d f + e v + g** in real time using stabilized RLS.
- Analytically finds the **global maximum** of the fitted surface (closed-form 2×2 solve from partial derivatives + Hessian check to confirm it's a true max).
- Respects hard safety limits (temp ≤ 65°C, error rate ≤ 1.5%).
- Only suggests retune when predicted gain > 2% **and** model confidence >30%.
- Tracks best observed point + live model quality metric (inverse parameter uncertainty).

## Why v1.5.5 is the best

- Every line serves the math. Zero unnecessary features (stripped all previous puzzle/non-mining code).
- Numerical guards: ridge regularization, det/hessian checks, NaN/Inf protection, trace-based confidence.
- Clean, commented, production-ready for ESP32 (FreeRTOS compatible, ~2KB flash).
- Faster, smaller, more reliable than bloated v1.x versions. Evidence-based optimization only.

## Quick integration (ESP-Miner fork or custom)

1. Drop `components/g6_brain/` into your ESP-IDF tree.
2. Add `g6_brain` to `main/CMakeLists.txt` PRIV_REQUIRES.
3. In your telemetry/monitor task (call every 30s with live data):
   ```c
   #include "g6_brain.h"
   static G6BrainState g6_brain;
   ...
   g6_brain_init(&g6_brain);
   ...
   g6_brain_update(&g6_brain, f_mhz, v_mv, hr, power, temp, err_pct);
   g6_brain_auto_step(&g6_brain, f_mhz, v_mv);
   ```
4. In `auto_step`, when it decides to retune: wire to your ASIC driver (e.g. `bap_set_asic_frequency(new_f)` or direct register write).
5. Build & flash. Watch serial for model evolution and optimal recommendations.

## API (math expert edition)

- `g6_brain_init()` — strong prior + high uncertainty (learns fast from your runs).
- `g6_brain_update(...)` — feed one telemetry point; updates model safely.
- `g6_brain_get_optimal(...)` — returns clamped f_opt, v_opt, predicted HR (false if no valid max).
- `g6_brain_auto_step(current_f, current_v)` — decides if retune is worth it (checks gain + confidence).
- `g6_brain_reset()` — wipe and restart learning.

## Math notes (rigorous)

The critical point is solved from setting gradients to zero:
```
∂HR/∂f = 2a f + c v + d = 0
∂HR/∂v = 2b v + c f + e = 0
```
Hessian test (a < 0 && det > 0) guarantees maximum before any suggestion.

RLS update: standard recursive least squares with forgetting λ=0.97 for tracking non-stationary (temp/silicon drift).

## License & spirit

MIT. Fork it, improve the math, PR back.  
This is pure truth-seeking collaboration — no fluff, just the best optimizer we can make for your Gamma.

**Flash it. Let the math work. Report your hashrate gains.**

https://github.com/6ummy-Dev/bitaxe-g6-brain