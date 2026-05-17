# G6 Brain Installation & Integration Guide — v1.0.0-beta2

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

## Step 3: menuconfig (Optional)

```bash
idf.py menuconfig
```

Navigate to:

```
Component config → G6 Brain Configuration
```

Here you can adjust:
- Thermal ceiling
- RLS forgetting factor (`G6_RLS_LAMBDA`)
- DFS step size
- NER threshold

There is currently **no** `CONFIG_G6_BRAIN_ENABLE` flag required. The component becomes active once it is included in the build.

---

## Step 4: Integrate the Brain (Recommended)

Use the main integration example:

→ **`docs/INTEGRATION_EXAMPLE.c`**

**Quick integration:**
1. Copy `docs/INTEGRATION_EXAMPLE.c` into your project.
2. Call it from `app_main()` after WiFi and ASIC initialization.
3. Replace the placeholder telemetry reads with your actual values.
4. Start in `OBSERVE_ONLY` or `RECOMMEND` mode.

---

## Step 5: Build & First Run

```bash
idf.py build
idf.py flash monitor
```

Watch `model_quality` improve over the first 10–30 minutes.

---

## Recommendations

- Start in conservative mode (`OBSERVE_ONLY` or `RECOMMEND`)
- Monitor behavior for at least 24–48 hours before using `AUTO` mode
- Review logs regularly during the initial period

---

## Next Steps

- Recommended example → `docs/INTEGRATION_EXAMPLE.c`
- Full API reference → `docs/API.md`
- All Kconfig options → `docs/KCONFIG.md`
- Safety principles → `AGENTS.md`

---

**Version:** v1.0.0-beta2 — May 2026
