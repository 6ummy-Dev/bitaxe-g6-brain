# G6 Brain Testing Guide (beta3)

This guide is intended for community members testing the **v1.0.0-beta3** release.

## What's New in beta3

- **Dinkelbach-based J/TH optimizer** — replaces the previous brute-force grid search for efficiency mode.
- **`power_model_quality`** monitoring and gating — the brain now refuses J/TH optimization if the power model is not yet reliable.
- New Kconfig options for fine-tuning the solver:
  - `G6_JTH_MAX_OUTER_ITERS`
  - `G6_JTH_INNER_STEPS`

These changes are **opt-in** via `G6_ENABLE_EFFICIENCY_MODE`.

## Recommended Starting Point

- Start in **`G6_MODE_RECOMMEND`** (this is the default and safest mode).
- Do **not** switch to `AUTO` mode until you have monitored behavior for several hours/days.
- Use `G6_MODE_OBSERVE_ONLY` if you only want safety monitoring without any optimization.
- If you want to test the new efficiency mode, enable `G6_ENABLE_EFFICIENCY_MODE` in `menuconfig` **after** the brain has collected enough samples (model_quality and power_model_quality both reasonably high).

## What to Monitor

When testing, pay attention to:

- Frequency and voltage suggestions from the brain
- Temperature behavior and thermal derating
- Nonce Error Rate (NER)
- Hashrate stability
- `model_quality` and `power_model_quality` values
- Whether the brain settles on reasonable operating points
- In efficiency mode: whether J/TH actually improves over time without aggressive or unstable changes

You can monitor these through:
- ESP-IDF logs (especially `G6_BRAIN` and `Quality=` lines)
- The telemetry exported via `g6_brain_get_telemetry()`

## Basic Testing Flow

1. Flash with beta3
2. Start in `RECOMMEND` mode (default)
3. Let it run for several hours while observing logs and `model_quality`
4. (Optional) Enable efficiency mode once quality metrics look stable
5. Check if the brain is making reasonable adjustments
6. Report any strange behavior (aggressive changes, instability, safety triggers, poor efficiency decisions, etc.)

## Testing the New Dinkelbach J/TH Solver (Efficiency Mode)

When `G6_ENABLE_EFFICIENCY_MODE` is enabled:

- The brain will attempt to minimize J/TH using the new analytical solver.
- It is protected by **both** `model_quality >= 0.6` **and** `power_model_quality >= 0.6`.
- On fresh boots or after reset, efficiency optimization will be skipped until the power model has enough data.
- Watch logs for messages like:
  - `"Skipping J/TH optimization (hr_q=... pw_q=...)"`
  - Any unusual jumps in frequency/voltage when efficiency mode activates

## Reporting Issues

When reporting issues, please include:

- ESP32 target and firmware version
- Mode you were running (`RECOMMEND`, `AUTO`, efficiency mode on/off)
- Relevant log output (especially around model quality and solver decisions)
- Observed behavior vs expected behavior
- Whether this is a cold boot or warm start (NVS)

## Notes

- beta3 focuses on code quality, the new analytical J/TH solver, and better safety gating.
- Safety remains the top priority. The brain should never push the ASIC into unsafe territory.
- The Dinkelbach solver is still relatively new — thorough field testing with efficiency mode enabled is very valuable.

Thank you for helping test the G6 Brain!
