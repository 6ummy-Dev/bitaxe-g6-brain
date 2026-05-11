# G6 Brain Installation & Integration Guide

## Quick Start

1. Copy the `g6_brain` folder into `components/g6_brain` of your ESP-Miner project.
2. Enable the component in menuconfig:
   ```
   Component config → G6 Brain Configuration
   ```
3. Call in your main loop:
   ```c
   g6_brain_init(&brain);
   g6_brain_update(&brain, freq_mhz, voltage_mv, hashrate_ths, power_w, temp_c, error_pct);
   ```

See `docs/INTEGRATION_EXAMPLE.c` for a complete example.

## Build Instructions

- ESP-IDF v5.3 or newer recommended
- Run `idf.py menuconfig` → enable G6 Brain
- Build with `idf.py build`

Detailed Kconfig options → [KCONFIG.md](KCONFIG.md)
