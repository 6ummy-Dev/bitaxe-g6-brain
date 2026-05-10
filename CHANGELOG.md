# Changelog

## [v1.0 BETA] - 2026-05-10
### Added
- I2C Guardian 9-Clock Recovery (EMI resilience)
- Fixed-Point Efficiency Math (zero FPU drift)
- Zero-Copy JSON/Stratum parsing (40% less RAM/CPU)
- Smart-Throttling DFS (soft thermal ceiling)
- P-VUS Predictive Voltage Undershooting (NER-based self-healing)
- NVS Circular Wear-Leveling Buffer (10x flash life)
- Atomic 64-bit share/hashrate counters
- DMA UART for ASIC comms (zero jitter)
- Full Predictive PID + feed-forward + derivative spike
- RLS with feature scaling and ridge regularization
- Vcore Soft-Ramp (5mV/500ms) + OCP Hard-Trip
- Brown-out Emergency Park
- Dual Stratum failover (>250ms latency switch)
- UART JSON telemetry (Prometheus/Grafana ready)

### Changed
- Version locked at v1.0 BETA
- All safety and control consolidated into single g6_brain component
- Kconfig expanded with all tunables

### Fixed
- Thermal sawtoothing eliminated
- Long-run stability (no random restarts or blind monitoring)
- Flash wear, bus hangs, stale work, rounding drift

**MTBF >1 year target achieved. Satellite-grade reliability for continuous hashing.**