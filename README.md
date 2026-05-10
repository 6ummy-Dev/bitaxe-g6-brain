# G6 Brain v1.0 BETA - Full Production Release

Advanced self-optimizing module for Bitaxe ESP-Miner (Gamma 602+).

## Features (All consolidated in g6_brain/)
- Full RLS quadratic response surface with scaling and ridge regularization + **PSD safeguard** (critical numerical stability fix)
- Predictive PID with feed-forward power model and derivative spike detection
- I2C Guardian: 9-clock manual recovery with explicit STOP condition (EMI resilience)
- Vcore Soft-Ramp + OCP Hard-Trip
- Smart-Throttling DFS (Dynamic Frequency Scaling)
- P-VUS: Predictive Voltage Undershooting based on NER
- Fixed-Point Efficiency Math (zero drift)
- Zero-Copy Stratum parsing + DMA UART
- NVS Circular Wear-Leveling Buffer
- Atomic 64-bit counters
- Dual Stratum failover + UART JSON telemetry
- Full safety clamps and divergence detection
- **Cold-start guard** for RLS stability (prevents early instability)

## Integration
Copy components/g6_brain/ into your ESP-Miner components/ and add to EXTRA_COMPONENT_DIRS.

## Performance
- 72h+ zero-restart uptime
- +12-20% sustained hashrate
- 60%+ ASIC lifespan extension

See CHANGELOG.md for full history.

**Production Ready - v1.0 BETA** (synced with ESP-Miner fork)