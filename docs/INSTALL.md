# G6 Brain Installation & Integration Guide — v1.0.0-beta2

**Target:** Bitaxe ESP-Miner (Gamma 602+ / BM1370) running AxeOS or custom ESP-IDF firmware.

**Time to integrate:** ~10–15 minutes for experienced developers.

---

## Prerequisites

- **ESP-IDF v5.3 or newer** (v5.4+ recommended)
- Your project must already build and run on the target Bitaxe hardware
- At least **64 KB free PSRAM** and **8 KB free IRAM** after integration (brain task uses ~4 KB stack)
- Git access (recommended) or manual copy of the `g6_brain` component

---

## Step 1: Add the Component

### Option A — Git Submodule (Recommended)

```bash
cd your-esp-miner-project
git submodule add https://github.com/6ummy-Dev/bitaxe-g6-brain.git components/g6_brain
git submodule update --init --recursive
```

### Option B — Manual Copy

1. Download or clone this repo
2. Copy the entire `components/g6_brain/` folder into your project’s `components/` directory
3. Delete the `.git` folder inside the copied component (optional but cleaner)

---

## Step 2: Register the Component

Edit your project’s top-level `CMakeLists.txt` and add:

```cmake
set(EXTRA_COMPONENT_DIRS 
    ${CMAKE_CURRENT_LIST_DIR}/components/g6_brain
)
```

---

## Step 3: Enable in menuconfig

```bash
idf.py menuconfig
```

Go to:

```
Component config → G6 Brain Configuration
```

Enable at minimum:
- `CONFIG_G6_BRAIN_ENABLE`
- `CONFIG_G6_BRAIN_NVS_FINGERPRINT` (recommended)

---

## Step 4: Integrate the Brain (Recommended)

Use the **recommended integration example**:

→ **`docs/INTEGRATION_EXAMPLE.c`**

This file contains a clean, practical integration you can adapt into your firmware.

**Quick integration path:**

1. Copy `docs/INTEGRATION_EXAMPLE.c` into your project (e.g. as `main/g6_brain_task.c`)
2. Call `g6_brain_example_task()` from `app_main()` after WiFi and ASIC initialization.
3. Replace the placeholder telemetry reads with your actual values from `GlobalState`.

**Important notes:**
- Start in `OBSERVE_ONLY` or `RECOMMEND` mode.
- Only move to `AUTO` mode after monitoring behavior.
- The example includes guidance on when (and when **not**) to apply the recommended `opt_f` / `opt_v`.

---

## Alternative: Minimal Manual Integration

If you prefer calling the API directly in your existing loop, see the comments inside `INTEGRATION_EXAMPLE.c`.

---

## Step 5: Build & First Run

```bash
idf.py build
idf.py flash monitor
```

After a few minutes you should see improving `model_quality` and the brain suggesting new operating points.

---

## Troubleshooting

See the troubleshooting table in the previous version or check logs with `CONFIG_G6_BRAIN_DEBUG=y`.

---

## Post-Integration Checklist

- [ ] Brain runs stably for 30+ minutes
- [ ] `g6_brain_self_test()` returns `ESP_OK`
- [ ] `model_quality` improves over time
- [ ] You understand when the brain applies or refuses changes

---

## Next Steps

- **Recommended Integration Example** → `docs/INTEGRATION_EXAMPLE.c`
- **Full API reference** → [API.md](API.md)
- **Kconfig options** → [KCONFIG.md](KCONFIG.md)
- **Safety principles** → [AGENTS.md](../AGENTS.md)

---

**You are now running the G6 Brain.**  
Start conservative. Monitor first. Optimize later. ⚡
