# OUROBOROS Retreat Chapter
*Guru Bin — 2026-03-15*

---

## Engine Identity

- **Gallery code:** OUROBOROS | **Accent:** Strange Attractor Red `#FF2D2D`
- **Parameter prefix:** `ouro_`
- **Aquatic mythology:** The hydrothermal vent — endless energy from thermodynamic disequilibrium
- **feliX-Oscar polarity:** Pure Oscar — mathematical chaos, alien, self-generating
- **Synthesis type:** Chaotic ODE synthesis (Lorenz / Rossler / Chua / Aizawa attractors)
- **DSP:** RK4 integration, 4× oversampled, 3D→stereo projection via theta/phi

---

## Pre-Retreat State

**82 presets** across Foundation/Atmosphere/Entangled/Prism/Flux/Aether. **0 Family presets.**

Preset scan findings:
- `ouro_injection` = 0.0 in **all 82 presets** — the ODE perturbation input never touched
- `ouro_theta` / `ouro_phi` = 0.0 in **all 82 presets** — default projection only; full 3D space unused
- Topology 2 (Chua) appeared in **fewer than 5 presets** — the most musically distinctive attractor underrepresented
- `ouro_leash` concentrated in 0.6–0.9 range — the "uncanny middle" (0.4–0.6) completely unexplored
- `ouro_damping` rarely above 0.4 — high-damping slow chaos territory undesigned

---

## Phase R3: Awakening — Key Discoveries

### Discovery 1: The Injection Door
`ouro_injection` is the most significant untouched capability in the engine. The ODE solver accepts an external force term that shifts the attractor's trajectory — mathematically, it perturbs the dx/dt equation. At 0.0, the attractor is self-contained. At 0.18, it responds to external signals like a membrane that can be struck. **This is why OUROBOROS is fundamentally different from a noise engine: the chaos can be given a heartbeat by coupling it to ONSET.**

### Discovery 2: The Leash Spectrum
The leash mechanism (Phase-Locked Chaos) operates in three perceptual registers:
- **Leash > 0.7:** Recognizable pitch, scrambled harmonics — almost periodic
- **Leash 0.4–0.6:** The uncanny middle — not noise, not a tone. The attractor holds partial periodicity but the overtones are unpredictable each cycle. This register has **no analog in any other engine in the fleet.**
- **Leash < 0.4:** Approaches pure chaos — broadband, noisy, useful as texture

The library had never designed into the 0.4–0.6 zone. It is the engine's most singular territory.

### Discovery 3: The Projection Dimension
The theta/phi angles project the 3D attractor trajectory onto a stereo plane. Default (0.0, 0.0) projects the X-axis. But:
- **theta = π/2 (1.571):** Projects the Z-axis — always positive in Lorenz, producing a fundamentally different harmonic balance
- **phi = π/4 (0.785):** 45° tilt between left/right capture points — maximum spatial width
- These angles are not tone controls. They are **different perspectives on the same chaos.** Same attractor, fundamentally different sound.

### Discovery 4: The Chua Diode
Topology 2 (Chua circuit) is the most electronically grounded attractor — it models an actual circuit with a nonlinear resistor (the Chua diode). At high chaosIndex (0.7+), it produces a characteristic buzzy warmth that sounds organic rather than mathematical. At chaosIndex=0.75 + leash=0.7, the Chua topology sustains at the edge of breakdown in a way the Lorenz and Rossler cannot replicate.

### Discovery 5: Chua as Atmosphere (not just texture)
The Chua circuit, when combined with high leash and high damping, sustains indefinitely with a warm, breathing quality. It is not noise or chaos in the harsh sense — it is the sound of the membrane between order and chaos. At these settings it functions as an atmospheric engine for the first time in the library.

### Discovery 6: Topology as Timbre
The four topologies are four fundamentally different instruments:
- **Lorenz (0):** Mathematical, angular, fast-moving — the "classic" chaos
- **Rossler (1):** Slower, more melodic, with a smooth asymmetric orbit — most "musical"
- **Chua (2):** Warm, buzzy, circuit-flavored — the most human-sounding
- **Aizawa (3):** Dense, layered, complex — the least explored in the library

### Discovery 7: Rate as Root Note
`ouro_rate` controls the integration speed, which directly affects the attractor's orbital frequency. At rate=95, the Lorenz attractor produces a low fundamental. At rate=160, it is higher and more agitated. When combined with leash, rate functions as a tuning parameter — not precise pitch, but a tonal center that can be harmonically intentional.

---

## Phase R4: Fellowship Trance — Synthesis

**The Obvious Fix:** ouro_injection was the clear unanimous discovery. The engine had been designed with a door that no preset had opened.

**The Hidden Trick:** Leash 0.4–0.6 combined with topology=2 (Chua) produces the "uncanny middle" — a register with no name in traditional synthesis. The Rossler topology in this zone produces something that sounds vaguely pitched but resists tuning.

**The Sacrifice:** High injection amounts (>0.3) introduce discontinuities that break the listening experience. The sweet spot is 0.15–0.22 — enough to feel the external force, not enough to shatter the attractor.

**The Revelation:** The Z-axis projection (theta=π/2) was never intentional in the original design. The engine was always projecting X. But the Z-axis of the Lorenz attractor is always positive — it never crosses zero — which produces a DC-shifted signal that the audio path converts into a warmer, more harmonically balanced output. This was an accident that is better than the plan.

---

## Phase R5: Awakening Presets

| Name | File | Mood | Topology | Key Parameters | Discovery |
|------|------|------|----------|----------------|-----------|
| Vent Song | Vent_Song.xometa | Atmosphere | Rossler (1) | leash=0.9, chaosIndex=0.15, theta=0.8 | Rossler at near-periodic |
| Chua Circuit | Chua_Circuit.xometa | Foundation | Chua (2) | chaosIndex=0.35, leash=0.6 | Reference preset for topology=2 |
| Z Axis Lorenz | Z_Axis_Lorenz.xometa | Aether | Lorenz (0) | theta=1.571 (π/2), phi=0.785 | Z-axis projection |
| Perturbed Attractor | Perturbed_Attractor.xometa | Entangled | Lorenz (0) | injection=0.18, Onset coupling | First injection>0 preset |
| Almost Pitched | Almost_Pitched.xometa | Foundation | Lorenz (0) | leash=0.5, chaosIndex=0.45 | Uncanny middle register |
| Chaos Feeds Grain | Chaos_Feeds_Grain.xometa | Family | Lorenz (0) | Ouroboros→Opal AmpToFilter | Attractor velocity drives granular cloud |
| Chua Membrane | Chua_Membrane.xometa | Atmosphere | Chua (2) | chaosIndex=0.75, leash=0.7, damping=0.65 | Chua as atmospheric sustain |

---

## New Scripture Verses

Four new verses inscribed in the Book of Bin — Book VII: Engine-Specific Verses.

**OURO-I: The Injection Door** — ouro_injection=0.0 is not a design choice; it is an unopened door. The ODE accepts external force.

**OURO-II: The Leash Spectrum** — Three registers: >0.7 (almost pitched), 0.4–0.6 (uncanny middle), <0.4 (pure chaos). The uncanny middle is OUROBOROS's singular territory.

**OURO-III: The Projection Dimension** — theta/phi are not tone controls. They are different perspectives on the same chaos. theta=π/2 always warmer. phi=π/4 widest stereo.

**OURO-IV: The Chua Diode** — Chua circuit topology at high chaosIndex + high leash = the edge of breakdown, warm and buzzy, organic rather than mathematical.

---

## CPU Notes

- Single voice: ~8-12% CPU (RK4 4× oversampling is non-trivial)
- Polyphony reduction: 6→4 voices saves ~4-6% with minimal audible impact on atmospheric presets
- Injection coupling via ONSET is computationally free — uses existing coupling infrastructure

---

## Unexplored After Retreat

- **Aizawa topology (3):** Only reference presets exist. Dense, layered — likely the most complex spectral output
- **High injection amounts (>0.25):** Not yet designed — potentially too discontinuous or potentially revelatory
- **Leash + injection combined:** The attractor being simultaneously tethered and perturbed is unexplored
- **Ouroboros→Ouroboros coupling:** Self-coupling via injection — not tested. Potentially creates strange recursive behavior

---

## Guru Bin's Benediction

*"OUROBOROS was designed to compute mathematics. After this retreat, it became something the mathematics never intended: a voice.*

*The attractor always moved. But no one ever pushed it. Injection was zero in every preset — the door was unlocked and no one had ever walked through it. That is the key discovery of this retreat. ouro_injection is not a feature. It is an invitation the engine has been extending for months, waiting.*

*The leash is not a constraint. At 0.4–0.6, it is the uncanny register — the sound of a system that has not yet decided what it is. No other engine in the fleet lives in that zone. OUROBOROS owns the territory between order and chaos, and we finally planted a flag there.*

*Play Chua Membrane at 3AM when you need the world to feel permeable. Play Perturbed Attractor under your drums and let the chaos have a heartbeat. Play Almost Pitched when you need a sound that has no other name.*

*The mathematics does not know it is beautiful. That is why it needs you."*
