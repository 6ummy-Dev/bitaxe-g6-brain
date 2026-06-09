# G6 Brain Installation & Integration Guide — v1.0.0-beta7

**Target:** Bitaxe ESP-Miner (Gamma 602+ / BM1370)

**Time to integrate:** ~10–15 minutes

---

## Prerequisites

- ESP-IDF v5.3 or newer
- Your project already builds and runs on Bitaxe hardware
- Git (recommended) or manual copy of the component

---

## Step 1: Add the Component

### Recommended: Git Submodule

```bash
cd your-esp-miner-project
git submodule add https://github.com/6ummy-Dev/bitaxe-g6-brain.git components/g6_brain
git submodule update --init --recursive
```

### Alternative: Manual Copy

Copy the `components/g6_brain/` folder into your project’s `components/` directory.

---

## Step 2: Register the Component

Add the component path in your top-level `CMakeLists.txt`:

```cmake
list(APPEND EXTRA_COMPONENT_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/components/g6_brain
)
```

---

## Step 3: menuconfig (Fully Functional)

```bash
idf.py menuconfig
```

Navigate to:

```
Component config → G6 Brain Configuration
```

All options are live, including the thermal safety settings:
- `G6_TEMP_PROACTIVE_MARGIN`
- `G6_VR_TEMP_CEILING`
- `G6_VR_TEMP_PROACTIVE_MARGIN`

---

## Step 4: Integrate the Brain (Recommended)

Use the main integration example:

→ **`docs/INTEGRATION_EXAMPLE.c`**

**Quick integration:**
1. Copy `docs/INTEGRATION_EXAMPLE.c` into your project.
2. Call it from `app_main()` after WiFi and ASIC initialization.
3. **Set `brain.control_mode = G6_MODE_RECOMMEND`** (safest starting point).
4. Replace placeholder telemetry with real values from your miner.
5. If your hardware provides VR temperature, pass it to `g6_brain_update()`. Otherwise use `G6_VR_TEMP_NO_SENSOR`.

---

## Step 5: Build & First Run

```bash
idf.py fullclean && idf.py build
idf.py flash monitor
```

The brain is intentionally quiet on the console — it only logs the events you genuinely need to know about (see [MONITORING.md](MONITORING.md) for the full list). On a healthy first boot you should see **no warnings** from the `G6_BRAIN` tag. Telemetry from your integration code (`G6_TEL` tag in the example) is where the visibility lives — log the snapshot fields periodically.

If you see any of these, investigate before continuing:
- `NVS schema mismatch ... — erasing` — automatic, one-time per brain version upgrade. Safe to ignore on first boot of a new version; suspicious if it appears every reboot.
- `NVS blob size mismatch ... — erasing` — same as above; struct evolution forced a fresh cold-start. One-time only.
- `P matrix diverged — cold-start recovery applied` — should not appear on a fresh install. If it does, your integration is feeding the brain pathological telemetry. Check the values you pass to `g6_brain_update()`.
- `Power Outlier Rejected` — sporadic single events are expected; chronic occurrences mean your power sensor (INA219/INA260) has noise or wiring issues.

---

## Phase Recommendations

**Phase 1 — Observation (first 24h):**
- Start in `G6_MODE_RECOMMEND` (computes optimal but never mutates `best_f`/`best_v`).
- Or use `G6_MODE_OBSERVE_ONLY` for pure telemetry/safety validation if you don't yet want recommendations.
- Watch `model_quality` climb. Below 0.60 is normal during early learning.
- If your hardware provides VR temperature, pass the value to enable two-tier thermal protection.

**Phase 2 — Validation (next 24h):**
- Confirm `safety_status` stays at `G6_SAFETY_OK` during normal operation.
- Investigate any persistent non-OK status — see the [Safety Status Reference](SAFETY.md#safety-status-reference) in `SAFETY.md` for what each one means and what to check.
- Expected transient statuses on a healthy system: `G6_SAFETY_SAMPLE_QUALITY` (occasional 3-sigma outliers) and `G6_SAFETY_THERMAL` (brushing the proactive zone briefly).
- Unexpected statuses to investigate: `G6_SAFETY_INPUT_RANGE` (your integration is feeding bad telemetry), `G6_SAFETY_P_MATRIX_SINGULAR` (estimator divergence — should be rare).

**Phase 3 — Activation (after 48h of clean operation):**
- Switch to `G6_MODE_AUTO`. The brain will start applying internally-slew-rate-limited setpoints.
- Continue monitoring telemetry. Roll back to `RECOMMEND` immediately if you observe instability or unexpected safety events.

---

## Next Steps

- Recommended example → [`docs/INTEGRATION_EXAMPLE.c`](INTEGRATION_EXAMPLE.c)
- Full API reference → [`docs/API.md`](API.md)
- Kconfig options → [`docs/KCONFIG.md`](KCONFIG.md)
- Safety mechanisms & status reference → [`docs/SAFETY.md`](SAFETY.md)
- Real-time monitoring & telemetry → [`docs/MONITORING.md`](MONITORING.md)
- Engineering principles → [`AGENTS.md`](AGENTS.md)

---

**Version:** v1.0.0-beta7 (June 2026)

---

**The brain your Bitaxe always wanted.** Start safe. Learn. Then optimize. ⚡
