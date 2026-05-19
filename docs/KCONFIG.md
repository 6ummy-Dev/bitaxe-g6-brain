# G6 Brain Kconfig Options — v1.0.0-beta3

All options live under:  
**Component config → G6 Brain Configuration**

---

## Optimization Behavior

### `G6_ENABLE_EFFICIENCY_MODE`
- Enable true J/TH efficiency optimization using the Dinkelbach-based solver.

### `G6_JTH_MAX_OUTER_ITERS`
- **Type:** int (3–15)  
- **Default:** `7`  
- Maximum outer iterations for the J/TH Dinkelbach optimizer.

### `G6_JTH_INNER_STEPS`
- **Type:** int (2–10)  
- **Default:** `5`  
- Number of gradient steps inside each Dinkelbach iteration.

---

**Version:** v1.0.0-beta3
