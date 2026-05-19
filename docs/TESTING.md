# G6 Brain Testing Guide (beta2)

This guide is intended for community members testing the **v1.0.0-beta2** release.

## Recommended Starting Point

- Start in **`G6_MODE_RECOMMEND`** (this is the default and safest mode).
- Do **not** switch to `AUTO` mode until you have monitored behavior for several hours/days.
- Use `G6_MODE_OBSERVE_ONLY` if you only want safety monitoring without any optimization.

## What to Monitor

When testing, pay attention to:

- Frequency and voltage suggestions from the brain
- Temperature behavior and thermal derating
- Nonce Error Rate (NER)
- Hashrate stability
- Whether the brain settles on reasonable operating points

You can monitor these through:
- ESP-IDF logs
- The telemetry exported via `g6_brain_get_telemetry()`

## Basic Testing Flow

1. Flash with beta2
2. Start in `RECOMMEND` mode
3. Let it run for several hours while observing logs
4. Check if the brain is making reasonable adjustments
5. Report any strange behavior (aggressive changes, instability, safety triggers, etc.)

## Reporting Issues

When reporting issues, please include:
- ESP32 target and firmware version
- Mode you were running (`RECOMMEND`, `AUTO`, etc.)
- Relevant log output
- Observed behavior vs expected behavior

## Notes

- Beta2 is considered stable for field testing.
- The new J/TH efficiency features are still under active development in beta3.
- Safety is the top priority. The brain should never push the ASIC into unsafe territory.

Thank you for helping test the G6 Brain!
```