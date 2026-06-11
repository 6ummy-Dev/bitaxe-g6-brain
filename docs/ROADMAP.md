# Bitaxe G6 Brain — Roadmap

*What's next for the G6 Brain. Unlike [`../MANIFESTO.md`](../MANIFESTO.md) — which states
enduring values and is meant to stay stable — this roadmap is expected to move as we learn.
It records intent and priority, not promises or dates.*

The throughline never changes: **highest stable hashrate with zero hardware risk**, delivered
as a clean, modular, well-tested component. Everything below serves that, or it doesn't ship.

---

## Now — the road to 1.0 / PROD

A production-ready, modular brain: strong math, strong safety, clean integration, excellent
diagnostics. In scope for 1.0, *provided each earns its place without breaking modularity or
elegance*:

- Two-tier thermal protection (independent ASIC and VR limits with proactive margins).
- Predictive safety elements, where they increase safety without adding coupling or complexity.
- Model-health monitoring and honest, inspectable telemetry (condition estimate, traces,
  innovation, safety status).
- Fail-closed input handling and covariance-divergence recovery (trace **and** non-PSD).

**Exit bar for 1.0 — a release is not "done" until:**
- The full Unity suite is **executed** (hardware or QEMU) and green — not merely compiled.
  (See the testing non-negotiable in `MANIFESTO.md` §3.)
- At least one sustained closed-loop (`AUTO`) hardware soak has been run and its telemetry
  reviewed — slew behaviour, recovery frequency, J/TH convergence, and thermal interaction
  under live control. Baseline (no-brain) data and host-side simulation are necessary but not
  sufficient; 1.0 should be *field-proven*, not only proven in theory.
- No public API signature changes left pending; `CHANGELOG.md` and the docs reflect reality.

## Next — post-1.0 (v1.5+)

- **On-device operating-point exploration.** This is the highest-leverage post-1.0 item, and
  it is more than an optimizer upgrade. At a *fixed* operating point the quadratic response
  surface is **unidentifiable**: only the constant basis term is excited, the covariance grows
  ill-conditioned, and the optimizer's output is no better than a constant fit. The beta6.5
  divergence guard makes that state *safe and visible* (it triggers recovery instead of
  silently freezing) — but it does not make the model *identifiable*. A small, bounded amount
  of deliberate operating-point variation is therefore a **precondition for the brain to learn
  a real surface at all**, not merely a way to optimize better. Treat exploration as
  load-bearing for the math, and prioritize it accordingly. (beta7 added the passive
  `model_under_excited` telemetry signal so operators can *see* the unidentified state and
  withhold trust from the recommendations; it does not supply the variation. This feature does.)
- **Puzzle-solver features.**
- **More advanced active-learning techniques**, building on the exploration primitive above.

## Later / out of scope for now

- Firmware-level improvements and deeper firmware integration. The brain stays a modular
  component that *recommends*; it does not grow to *own* the firmware. The broader firmware
  can be addressed separately, later — it is deliberately not part of the initial brain
  releases.

---

## Engineering follow-ups worth tracking

Smaller, concrete items surfaced during QA — not blockers, but good hygiene as scope expands:

- **Deterministic covariance-magnitude regression test.** The current suite guards the
  point-estimate (theta) recursion and the non-PSD recovery path, but does not assert
  covariance *magnitude*. A deterministic tracking-based test would lock in the exact-RLS
  posterior (the `+ k kᵀ` term) inside the suite itself, rather than relying on host-side
  numerical checks.
- **Outlier-gate / quality-floor calibration.** The channel floors
  (`G6_HR_OUTLIER_VAR_FLOOR_THS2`, `G6_PW_OUTLIER_VAR_FLOOR_W2`,
  `G6_QUALITY_DENOM_FLOOR_*`) are physically-reasoned and validated against baseline sensor
  noise with comfortable headroom. The power floor in particular is looser than it needs to
  be and can be tightened once closed-loop field data is in hand. The beta7 under-excitation
  warn level (`G6_EXCITATION_COND_WARN`) belongs in the same calibration pass: it is set
  conservatively high pending a closed-loop soak, where the genuinely-converged covariance
  condition number can be measured and the threshold tuned to fire on real under-excitation
  without crying wolf on a healthy model.
- **PSD maintenance vs. recovery (optional).** beta6.5 *recovers* from a non-PSD covariance.
  If we ever want to *prevent* it rather than recover, eigenvalue flooring in the stabilizer
  is the principled (heavier) option. Not needed today; revisit only if field data shows the
  recovery cadence is disruptive.
- **Innovation-gate deduplication.** `g6_brain_update()` computes the predicted variance
  `xᵀPx` for the covariance-divergence guard, then `has_significant_innovation()` recomputes
  the identical quadratic form (and again for the power channel in efficiency mode).
  Bit-identical today — same per-element accumulation order, verified by hand-trace — so this
  is pure redundancy: ~70 extra FLOPs per tick and a duplicated expression that could drift
  apart under future edits. Deliberately not touched in the beta7.5 point release (it sits in
  the estimator hot path); fold into the next cycle that touches the update loop, with an
  equivalence assertion in the suite when it lands.
- **`model_quality` smoothing (EMA).** Quality is an instantaneous per-sample fit metric, so
  the NER back-off's 0.25 clamp lasts exactly one accepted update and the ≥ 0.6 solver gate
  can re-arm (or flicker) on a single sample. An EMA or windowed quality would make the
  back-off's "re-arm only after observable recovery" intent real and de-noise the gate.
  Control-behavior change — exploration-cycle material, not a point release.

---

*Roadmap items move, merge, and get reprioritized. If something here ever conflicts with
`MANIFESTO.md`, the manifesto wins.*
