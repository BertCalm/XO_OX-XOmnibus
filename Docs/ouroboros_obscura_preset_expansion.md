# Ouroboros + Obscura Preset Expansion — Round 11G

**Date:** 2026-03-14
**Task:** 10 new presets each for OUROBOROS and OBSCURA, covering full sonic palette.

---

## OUROBOROS — 10 New Presets

Prior preset count: ~34 across all moods (Foundation, Atmosphere, Flux, Aether, Prism, Entangled).
New presets add to Aether (5) and Flux (5). Engine ID: `Ouroboros`. Prefix: `ouro_`.

### Design Strategy

The 10 presets cover four distinct axes of the OUROBOROS parameter space:

1. **Free chaos** (leash=0) — drone/texture uses, topology exploration
2. **Controlled-chaos sweet spot** (leash 0.3–0.6) — expressive melodic and lead
3. **Tight leash** (leash 0.85–1.0) — pitched chordal, Poincare reset audible
4. **Blessing B007 / velocity coupling** — expressive dynamic range, injection transients
5. **Transition** — leash as automation target, transformation over time

---

### Aether Presets

| Filename | Name | DNA (B/W/M/D/S/A) | Description |
|----------|------|-------------------|-------------|
| `Ouro_Thermal_Drone.xometa` | Thermal Drone | 0.18/0.82/0.22/0.35/0.72/0.08 | Rossler free-running at low chaos (index 0.28). Near a limit cycle — almost tonal but never exactly. Heavy damping (0.78) filters fast variations leaving slow orbital heat. The warmest, most ambient OUROBOROS setting. |
| `Ouro_Aizawa_Mobile.xometa` | Aizawa Mobile | 0.42/0.38/0.75/0.28/0.85/0.12 | Aizawa toroidal attractor at sub-audio orbit rate (1.8 Hz) — functions as a complex aperiodic LFO source. Route velocity coupling outputs (ch2/ch3) to other engines for non-repeating modulation. |
| `Ouro_Rossler_Spiral.xometa` | Rossler Spiral | 0.40/0.62/0.55/0.45/0.68/0.22 | Rossler at mid-chaos (0.55), leash 0.65. Softer and more melodic than Lorenz. Projection at theta=pi/4 shows the single-scroll spiral cross-section. Warm atmospheric pitched pad. |
| `Ouro_Velocity_Fist.xometa` | Velocity Fist | 0.55/0.38/0.70/0.48/0.50/0.55 | Blessing B007 showcase — loose leash (0.35) makes velocity injection the main event. Soft touches barely perturb the orbit; hard hits kick the attractor into new trajectories. The 50ms injection boost is visceral at forte. |
| `Ouro_Orbit_Crystal.xometa` | Orbit Crystal | 0.78/0.22/0.60/0.50/0.82/0.32 | Aizawa at moderate chaos (0.70), leash 0.75, both theta and phi at revealing angles. The compact torus viewed from its most complex cross-section. Bright, crystalline, each note shimmer-unique. |

### Flux Presets

| Filename | Name | DNA (B/W/M/D/S/A) | Description |
|----------|------|-------------------|-------------|
| `Ouro_Lorenz_Lead.xometa` | Lorenz Lead | 0.58/0.42/0.65/0.52/0.48/0.38 | Lorenz at the controlled-chaos sweet spot (chaos 0.45, leash 0.55). Every note has the same fundamental but a unique harmonic character — timbral fingerprint drawn from attractor state at note-on. No two performances of a melody sound identical. |
| `Ouro_Chua_Clang.xometa` | Chua Clang | 0.90/0.12/0.55/0.70/0.25/0.92 | Chua circuit at high chaos (0.78), tight leash (0.85), near-zero damping. The piecewise-linear diode nonlinearity creates buzzy metallic "electronic failure" harmonics. Aggressive pitched clang for industrial/noise applications. |
| `Ouro_Hard_Synced_Chord.xometa` | Hard Synced Chord | 0.52/0.55/0.42/0.60/0.45/0.28 | Lorenz at full leash (1.0) — Poincare reset fires every period. Strong fundamental, unusual but stable overtones shaped by attractor geometry rather than a wavetable. Best OUROBOROS setting for chordal or harmonic contexts. |
| `Ouro_Serpent_Strain.xometa` | Serpent Strain | 0.72/0.30/0.88/0.78/0.35/0.75 | High chaos (0.88) fighting a high leash (0.72) — the attractor straining against its collar. Richest, most turbulent harmonic content. Near-tonal resolutions at each Poincare reset. The most "alive" zone in the parameter space. |
| `Ouro_Leash_Loosen.xometa` | Leash Loosening | 0.60/0.38/0.58/0.55/0.42/0.45 | Starts tight and pitched (leash 0.90); automate `ouro_leash` downward to 0.1 over 8 bars and the fundamental dissolves into free chaos. A transformation preset designed for the leash as an automation target — the collar coming off in real time. |

---

## OBSCURA — 10 New Presets

Prior preset count: **0** (first-ever OBSCURA presets in the fleet).
New presets go to Foundation (5) and Atmosphere (5). Engine ID: `Obscura`. Prefix: `obscura_`.

### Design Strategy

The 10 presets cover the five axes of OBSCURA's physical modeling space:

1. **Plucked / struck** (sustain=0) — impulse-only excitation, various boundary conditions
2. **Bowed / continuous** (sustain>0) — physics envelope shapes the energy injection
3. **Bright / stiff** (stiffness>0.7) — fast wave propagation, bells, glass, metal
4. **Warm / floppy** (stiffness<0.3) — slow wave propagation, resonant drones, pads
5. **Evolving texture** (LFO modulation of scan width or excite position)

All presets exploit physics effects unavailable in other synthesis methods: self-consistent boundary resonances, amplitude-dependent brightness from nonlinear springs, and physically symplectic (energy-conserving) chain decay.

---

### Foundation Presets

| Filename | Name | DNA (B/W/M/D/S/A) | Description |
|----------|------|-------------------|-------------|
| `Obscura_Plucked_String.xometa` | Plucked String | 0.62/0.52/0.18/0.30/0.32/0.15 | Fixed boundary, medium stiffness (0.52), narrow excitation near quarter-point. Integer harmonic series, natural impulse decay. No bowing force. Guitar/plucked-string character from pure physics — not a Karplus-Strong approximation but actual mass-spring dynamics. |
| `Obscura_Bowed_Cello.xometa` | Bowed Cello | 0.28/0.80/0.32/0.55/0.42/0.18 | Fixed boundary, warm stiffness (0.38), full bowing force (0.72) with slow physics envelope attack (0.35s). Chain builds resonance gradually like a real cello stroke. Legato mode, 0.18s glide. Wide scanner (0.65) for thick low-string character. |
| `Obscura_Bell_Strike.xometa` | Bell Strike | 0.88/0.18/0.12/0.42/0.70/0.35 | Periodic boundary (ring topology), high stiffness (0.88), heavy nonlinear (0.45). Inharmonic bell-like partials emerge from ring geometry + fast wave propagation. Velocity brightens tone via cubic nonlinear term — exactly as real bells behave. Long natural decay. |
| `Obscura_Membrane_Hit.xometa` | Membrane Hit | 0.45/0.48/0.15/0.38/0.35/0.52 | Free boundary (odd harmonics), low-medium stiffness (0.35), heavy damping (0.55). Broad excitation width models a drumstick tip. Hollow drum-head resonance with fast amplitude decay. Velocity controls brightness via nonlinear term. |
| `Obscura_Glass_Rod.xometa` | Glass Rod | 0.92/0.12/0.28/0.58/0.30/0.80 | Free boundary at very high stiffness (0.82), near-maximum nonlinear (0.70), excitation at extreme position (0.08) for maximum brightness. Bowing force builds over 0.12s. Glassy, cutting lead tones where velocity creates dramatic brightness jumps. Legato mode. |

### Atmosphere Presets

| Filename | Name | DNA (B/W/M/D/S/A) | Description |
|----------|------|-------------------|-------------|
| `Obscura_Mantle_Drone.xometa` | Mantle Drone | 0.18/0.72/0.38/0.80/0.75/0.15 | Periodic boundary (ring), very low stiffness (0.22), near-zero damping, full bowing force. Both LFOs at slow rates modulating scan width and excite position. Dense modal texture — the giant squid's resonant collagen mantle vibrating under abyssal pressure. |
| `Obscura_Timbral_Breath.xometa` | Timbral Breath | 0.48/0.62/0.72/0.50/0.58/0.18 | LFO1 at 0.28 Hz modulates scan width by 0.65 depth. The scanner aperture opens and closes like breathing — darkening and brightening the tone with no filter movement. LFO2 wanders excite position. This timbral tremolo emerges from physics, not from a VCA or VCF. |
| `Obscura_Decay_Sculpture.xometa` | Decay Sculpture | 0.38/0.58/0.55/0.42/0.68/0.12 | Physics envelope sustain is near-zero with a very slow decay (4.5s). The bowing force drains away during the held note, deflating the chain's physical energy independently of the amplitude envelope. The tone physically dies while the amplitude envelope sustains — bowed to bare resonance. |
| `Obscura_Deep_Resonance.xometa` | Deep Resonance | 0.15/0.78/0.30/0.35/0.88/0.08 | Very low stiffness (0.15), near-zero damping (0.05), no bowing force. Chain rings for seconds on impulse energy alone. LFO2 at 0.05 Hz drifts the excite position so which modes are emphasized changes imperceptibly slowly. Spacious, warm, minimal. |
| `Obscura_Random_Hadal.xometa` | Random Hadal | 0.52/0.42/0.65/0.60/0.65/0.35 | Random initial chain shape — each note begins from a seeded-by-note-number unique chaotic displacement. Periodic boundary, S&H LFOs on both scan width and excite position. Poly8 mode. Every note in a cluster has different timbral history. Alien, varied, texturally unpredictable. |

---

## Sonic DNA Coverage Summary

### OUROBOROS — Before vs. After

| DNA Dimension | Gap Before | New Presets Filling Gap |
|---------------|------------|------------------------|
| Warmth > 0.75 | Thin | Thermal Drone (0.82) |
| Brightness > 0.85 | Thin | Chua Clang (0.90), Orbit Crystal (0.78) |
| Aggression > 0.70 | Thin | Chua Clang (0.92), Serpent Strain (0.75) |
| Low movement (static) | Thin | Thermal Drone (0.22), Hard Synced Chord (0.42) |

### OBSCURA — Baseline (all new)

Full palette from first presets: warmth 0.08–0.80, brightness 0.15–0.92, aggression 0.08–0.80, movement 0.12–0.72, space 0.30–0.88.

---

## File Locations

### OUROBOROS (new)
- `Presets/XOlokun/Aether/Ouro_Thermal_Drone.xometa`
- `Presets/XOlokun/Aether/Ouro_Aizawa_Mobile.xometa`
- `Presets/XOlokun/Aether/Ouro_Rossler_Spiral.xometa`
- `Presets/XOlokun/Aether/Ouro_Velocity_Fist.xometa`
- `Presets/XOlokun/Aether/Ouro_Orbit_Crystal.xometa`
- `Presets/XOlokun/Flux/Ouro_Lorenz_Lead.xometa`
- `Presets/XOlokun/Flux/Ouro_Chua_Clang.xometa`
- `Presets/XOlokun/Flux/Ouro_Hard_Synced_Chord.xometa`
- `Presets/XOlokun/Flux/Ouro_Serpent_Strain.xometa`
- `Presets/XOlokun/Flux/Ouro_Leash_Loosen.xometa`

### OBSCURA (new — first-ever presets)
- `Presets/XOlokun/Foundation/Obscura_Plucked_String.xometa`
- `Presets/XOlokun/Foundation/Obscura_Bowed_Cello.xometa`
- `Presets/XOlokun/Foundation/Obscura_Bell_Strike.xometa`
- `Presets/XOlokun/Foundation/Obscura_Membrane_Hit.xometa`
- `Presets/XOlokun/Foundation/Obscura_Glass_Rod.xometa`
- `Presets/XOlokun/Atmosphere/Obscura_Mantle_Drone.xometa`
- `Presets/XOlokun/Atmosphere/Obscura_Timbral_Breath.xometa`
- `Presets/XOlokun/Atmosphere/Obscura_Decay_Sculpture.xometa`
- `Presets/XOlokun/Atmosphere/Obscura_Deep_Resonance.xometa`
- `Presets/XOlokun/Atmosphere/Obscura_Random_Hadal.xometa`
