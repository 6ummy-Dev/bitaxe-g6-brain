# G6 Brain Installation & Integration Guide — v1.0 Beta

**Target:** Bitaxe ESP-Miner (Gamma 602+ / BM1370) running AxeOS or custom ESP-IDF firmware.

**Time to integrate:** ~10–15 minutes for experienced developers.

---

## Prerequisites

- **ESP-IDF v5.3 or newer** (v5.4+ recommended)
- Your project must already build and run on the target Bitaxe hardware
- At least **64 KB free PSRAM** and **8 KB free IRAM** after integration (brain task uses ~4 KB stack)
- Git access (recommended) or manual copy of the `g6_brain` component

**Recommended starting point:** Use the **production integration example** at  
`docs/main_integration_v1.0_beta.c` (drop-in FreeRTOS task + real telemetry extraction).

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

Edit your project’s top-level `CMakeLists.txt` (or the `CMakeLists.txt` of your main application component) and add:

```cmake
set(EXTRA_COMPONENT_DIRS 
    ${CMAKE_CURRENT_LIST_DIR}/components/g6_brain
)
```

If you already have `EXTRA_COMPONENT_DIRS`, just append the path.

---

## Step 3: Enable in menuconfig

```bash
idf.py menuconfig
```

Navigate to:

```
Component config →
    G6 Brain Configuration
```

**Required options to enable:**

- `CONFIG_G6_BRAIN_ENABLE` → **Yes** (default)
- `CONFIG_G6_BRAIN_NVS_FINGERPRINT` → **Yes** (strongly recommended for per-chip learning)

Optional but useful:
- `CONFIG_G6_BRAIN_DEBUG` → Yes (during first integration)
- Adjust `CONFIG_G6_BRAIN_TEMP_CEILING` (default 70 °C)
- Adjust max step sizes if you want more aggressive tuning

Save and exit.

---

## Step 4: Integrate the Brain (Recommended Path)

Use the **drop-in task** from `docs/main_integration_v1.0_beta.c`:

1. Copy `docs/main_integration_v1.0_beta.c` into your project (e.g. `main/brain_task.c`)
2. Include it:
   ```c
   #include "main_integration_v1.0_beta.h"   // or just paste the functions
   ```
3. Call from `app_main()` **after** WiFi, ASIC init, and GlobalState is valid:

   ```c
   extern void start_g6_brain(GlobalState *state);
   ...
   start_g6_brain(&gState);   // gState = your GlobalState instance
   ```

The task will:
- Initialize the brain
- Extract real telemetry every 30 seconds
- Compute optimal `f` / `v`
- Log via `ESP_LOGI("G6_BRAIN_INTEGRATION", ...)`

**You decide** when/how to apply `opt_f` and `opt_v` (see comments in the example).

---

## Alternative: Minimal Manual Integration

If you prefer to call the API directly in your existing control loop:

```c
#include "g6_brain.h"

static G6BrainState g6_brain;

void your_miner_loop(void) {
    static bool initialized = false;
    if (!initialized) {
        g6_brain_init(&g6_brain);
        initialized = true;
    }

    // Read current values from your SYSTEM_MODULE / GlobalState
    float f = ...;
    float v = ...;
    float hr = ...;
    float pwr = ...;
    float temp = ...;
    float err = ...;

    g6_brain_update(&g6_brain, f, v, hr, pwr, temp, err);

    float opt_f, opt_v;
    g6_brain_get_optimal(&g6_brain, &opt_f, &opt_v, NULL);

    // Apply only if you want to (with slew limiting, confirmation, etc.)
    // asic_set_frequency((uint32_t)opt_f);
    // asic_set_voltage((uint32_t)opt_v);
}
```

---

## Step 5: Build & First Run

```bash
idf.py build
idf.py flash monitor
```

**Expected first logs:**
```
I (xxxx) G6_BRAIN_INTEGRATION: G6 Brain initialized successfully
I (xxxx) G6_BRAIN: Cold start — collecting initial samples...
```

After ~5–10 minutes you should see improving `model_quality` and the brain beginning to suggest new setpoints.

---

## Troubleshooting

| Symptom                        | Likely Cause                          | Fix |
|--------------------------------|---------------------------------------|-----|
| `g6_brain_init` returns error  | NULL pointer or heap issue            | Check stack size (min 4096) |
| No telemetry updates           | Wrong GlobalState fields or timing    | Verify `state->SYSTEM_MODULE.*` names match your firmware |
| Brain stays in cold-start      | Too few valid samples or high error rate | Lower `CONFIG_G6_BRAIN_TEMP_CEILING` temporarily |
| Voltage/frequency jumps too fast | Missing slew limiting in your apply code | Implement 5–10 MHz / 5 mV steps with delay |
| NVS fingerprint not saving     | NVS partition full or not initialized | Call `nvs_flash_init()` before brain init |
| High CPU usage                 | Calling `update()` too frequently     | Use 20–30 s interval (recommended) |

**Debug tip:** Enable `CONFIG_G6_BRAIN_DEBUG` and watch for `ESP_LOGD` output from the RLS core.

---

## Post-Integration Checklist (Aerospace QA Style)

- [ ] Brain task runs without crashes for 30+ minutes
- [ ] `g6_brain_self_test(&brain)` returns `ESP_OK`
- [ ] Model quality > 0.7 after 20+ samples
- [ ] No voltage undershoot events in logs
- [ ] NVS fingerprint saved successfully (if enabled)
- [ ] You have reviewed and accepted the recommended `opt_f` / `opt_v` before applying

---

## Next Steps

- **Full API reference** → [API.md](API.md)
- **All Kconfig options explained** → [KCONFIG.md](KCONFIG.md)
- **Production-ready drop-in task** → `docs/main_integration_v1.0_beta.c`
- **Safety invariants & unhappy paths** → Root `AGENTS.md`

---

**Questions?** Open an issue on the repo or ping 6ummy-Dev.

**You are now running the most advanced open-source Bitaxe brain available.**  
Welcome to the future of autonomous mining. ⚡
