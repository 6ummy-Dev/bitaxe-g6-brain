# Bitaxe G6 Brain ⚡

**v1.0.0-beta2** — Adaptive stabilized RLS optimizer with real-time quadratic efficiency modeling for BM1370

The **G6 Brain** dynamically learns the unique quadratic efficiency surface (frequency × voltage × hashrate) of your ASIC in real time using a hardened conventional RLS algorithm.

---

## ✅ Status
- **CI**: Passing (Docker ESP-IDF build + Unity compilation)
- **Version**: Hardened beta2 with critical safety fixes
- Ready for community field testing

See [CHANGELOG.md](CHANGELOG.md) for full history.

---

## ✨ Key Features
- Stabilized conventional RLS with Variable Forgetting Factor & covariance protection
- Fully self-contained safety layer (thermal, NER, voltage ripple, power sanity)
- Sample quality state machine + NVS warm-start persistence
- BM1370 hard limits + true J/TH efficiency optimization
- Defensive programming & extensive input validation
- Unity test suite

---

## 🚀 Quick Start
1. Add as ESP-IDF component
2. Use the recommended integration: [`docs/INTEGRATION_EXAMPLE.c`](docs/INTEGRATION_EXAMPLE.c)
3. Start in `OBSERVE_ONLY` or `RECOMMEND` mode
4. Monitor before enabling `AUTO`

Full guide → [`docs/INSTALL.md`](docs/INSTALL.md)

---

## 📖 Documentation
- [Installation & Integration](docs/INSTALL.md)
- [Public API](docs/API.md)
- [Kconfig Options](docs/KCONFIG.md)
- [Safety & Engineering](docs/SAFETY.md)
- [AGENTS.md](AGENTS.md) — Core engineering principles

---

## 🤝 Contributing
Pull requests welcome. All changes must respect the safety invariants in `AGENTS.md`.

---

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![CI Status](https://github.com/6ummy-Dev/bitaxe-g6-brain/actions/workflows/build.yml/badge.svg)](https://github.com/6ummy-Dev/bitaxe-g6-brain/actions/workflows/build.yml)

---

**Made with ❤️ and rigorous engineering for the Bitaxe community**
