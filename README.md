# Bitaxe G6 Brain ⚡

**v1.0.0-beta2** — Adaptive stabilized RLS optimizer with real-time quadratic modeling + telemetry for BM1370

The **G6 Brain** dynamically models the quadratic relationship between frequency, voltage, and hashrate using stabilized Recursive Least Squares (RLS). It learns your ASIC’s unique efficiency surface and selects safe operating points while enforcing strict safety constraints. Includes clean telemetry export and optional J/TH efficiency mode.

---

## ✅ Current Status

- **CI**: Passing
- **Kconfig**: Fully wired
- **Control Modes**: Enforced (`OBSERVE_ONLY` / `RECOMMEND` default / `AUTO`)
- **NVS**: Full theta + P-matrix + power model warm-start with auto-save
- **Telemetry**: `g6_brain_get_telemetry()` cleanly integrated into public API
- **QA Status**: Signed off in QA v5.5 — ready for field testing

See [CHANGELOG.md](CHANGELOG.md) for full history.

---

## ✨ Key Features

- Stabilized RLS with Variable Forgetting Factor, ridge regularization, trace monitoring, and innovation gating
- Fully self-contained safety layer (thermal ceiling, NER back-off, voltage ripple, power sanity)
- **NVS persistence** of hashrate + power models (true warm-start)
- **Control modes** with safe defaults
- **Telemetry export** via `g6_brain_get_telemetry()` (zero-copy snapshot)
- Optional true J/TH efficiency mode (opt-in)
- Strong single-threaded contract + defensive programming

---

## 🚀 Quick Start

1. Add the component to your ESP-IDF project
2. Use the recommended integration example: [`docs/INTEGRATION_EXAMPLE.c`](docs/INTEGRATION_EXAMPLE.c)
3. **Start in `G6_MODE_RECOMMEND`** (safest default)
4. Run `idf.py menuconfig` → Component config → G6 Brain Configuration
5. Monitor logs before switching to `AUTO` mode

Full installation guide → [`docs/INSTALL.md`](docs/INSTALL.md)

---

## 📖 Documentation

- [Installation & Integration](docs/INSTALL.md)
- [Public API](docs/API.md)
- [Kconfig Options](docs/KCONFIG.md)
- [Monitoring Guide](docs/MONITORING.md)
- [Safety Principles](docs/SAFETY.md)
- [Manifesto](MANIFESTO.md)

---

## 🤝 Contributing

Pull requests are welcome. All changes must respect the safety invariants documented in `AGENTS.md`.

---

**Made with ❤️ and rigorous engineering for the Bitaxe community**  
*May 2026 • v1.0.0-beta2*
