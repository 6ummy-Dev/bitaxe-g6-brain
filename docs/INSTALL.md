# G6 Brain Installation & Integration Guide — v1.0.0-beta2 (Phase 0 — fully wired)

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
set(EXTRA_COMPONENT_DIRS 
    ${CMAKE_CURRENT_LIST_DIR}/components/g6_brain
)
```

---

## Step 3: menuconfig (Now Fully Functional — Phase 0)

```bash
idf.py menuconfig
```

Navigate to:

```
Component config → G6 Brain Configuration
```

**All options are now live**:
- `G6_TEMP_CEILING`, `G6_NER_THRESHOLD`, `G6_DFS_STEP_MHZ`, `G6_RLS_LAMBDA_MIN`, etc.
- Changes are read automatically at runtime via `sdkconfig.h`

---

## Step 4: Integrate the Brain (Recommended)

Use the main integration example:

→ **`docs/INTEGRATION_EXAMPLE.c`**

**Phase 0 Quick integration:**
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
- “G6 Brain v1.0.0-beta2 initialized (Kconfig + control_mode + NVS auto-save)”
- NVS fingerprint auto-saves every ~5 minutes
- Control mode logged in every update

---

## Phase 0 Recommendations

- **Start in `G6_MODE_RECOMMEND`** (computes optimal but never mutates `best_f`/`best_v`).
- Only switch to `G6_MODE_AUTO` after 24–48 hours of monitoring + soak testing.
- Use `G6_MODE_OBSERVE_ONLY` for pure telemetry/safety validation.
- Monitor logs for `model_quality`, `control_mode`, and NVS save messages.

---

## Next Steps

- Recommended example → [`docs/INTEGRATION_EXAMPLE.c`](INTEGRATION_EXAMPLE.c)
- Full API reference → [`docs/API.md`](API.md)
- Kconfig options → [`docs/KCONFIG.md`](KCONFIG.md)
- Safety principles → [`AGENTS.md`](../AGENTS.md)

---

**Version:** v1.0.0-beta2 (Phase 0 fixes applied — May 2026)  
**All Kconfig, control modes, and NVS features are now fully wired and production-ready.**
