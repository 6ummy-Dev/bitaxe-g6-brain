# G6 Brain Installation & Integration Guide — v1.0.0-beta3

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

(Use `list(APPEND …)` rather than `set(…)` so you don't overwrite any existing `EXTRA_COMPONENT_DIRS` entries from your project.)

---

## Step 3: menuconfig (Fully Functional)

```bash
idf.py menuconfig
```

Navigate to:

```
Component config → G6 Brain Configuration
```

All options are live, including:
- `G6_ENABLE_EFFICIENCY_MODE` (opt-in J/TH optimization)
- `G6_JTH_MAX_OUTER_ITERS`
- Safety and RLS tuning parameters

---

## Step 4: Integrate the Brain (Recommended)

Use the main integration example:

→ **`docs/INTEGRATION_EXAMPLE.c`**

**Quick integration:**
1. Copy `docs/INTEGRATION_EXAMPLE.c` into your project.
2. Call it from `app_main()` after WiFi and ASIC initialization.
3. **Set `brain.control_mode = G6_MODE_RECOMMEND`** (safest starting point).
4. Replace placeholder telemetry with real values from your miner.

---

## Step 5: Build & First Run

```bash
idf.py fullclean && idf.py build
idf.py flash monitor
```

Watch for:
- “G6 Brain initialized”
- NVS fingerprint auto-saves
- Control mode and model quality logs

---

## Phase Recommendations (beta3)

- **Start in `G6_MODE_RECOMMEND`** (computes optimal but never mutates `best_f`/`best_v`).
- Only switch to `G6_MODE_AUTO` after monitoring for 24–48 hours.
- Use `G6_MODE_OBSERVE_ONLY` for pure telemetry/safety validation.
- **Efficiency mode** (`G6_ENABLE_EFFICIENCY_MODE`) is opt-in. It now uses a fast analytical Dinkelbach solver. Enable it only after the brain has collected enough data (`model_quality` and `power_model_quality` reasonably high).

---

## Next Steps

- Recommended example → [`docs/INTEGRATION_EXAMPLE.c`](INTEGRATION_EXAMPLE.c)
- Full API reference → [`docs/API.md`](API.md)
- Kconfig options → [`docs/KCONFIG.md`](KCONFIG.md)
- Safety principles → [`AGENTS.md`](AGENTS.md)
- Testing guide → [`docs/TESTING.md`](TESTING.md)

---

**Version:** v1.0.0-beta3 (May 2026)  
**Note:** The J/TH efficiency path now uses an analytical O(1) Dinkelbach solver with dual model quality gates.

---

**The brain your Bitaxe always wanted.** Start safe. Learn. Then optimize. ⚡
