# G6 Brain Kconfig Options

| Option                              | Default | Description |
|-------------------------------------|---------|-----------|
| CONFIG_G6_BRAIN_ENABLE              | y       | Enable G6 Brain component |
| CONFIG_G6_BRAIN_DEBUG               | n       | Enable verbose debug logs |
| CONFIG_G6_BRAIN_TEMP_CEILING        | 70      | Thermal ceiling (°C) |
| CONFIG_G6_BRAIN_MAX_FREQ_STEP       | 25      | Maximum frequency step (MHz) |
| CONFIG_G6_BRAIN_MAX_VOLT_STEP       | 12.5    | Maximum voltage step (mV) |
| CONFIG_G6_BRAIN_NVS_FINGERPRINT     | y       | Enable per-chip NVS warm-start |

All options are under `Component config → G6 Brain Configuration`.
