# Seance Verdict: OORT
**Date:** 2026-04-12
**Seance Type:** First seance — no prior record
**Score: 8.5 / 10**
**V1 Status: READY (subject to preset expansion to ≥50)**
**Blessing: B046 "The Composer Axis" awarded**

---

## Architecture
- GENDY polyphonic breakpoint waveform synthesis (Xenakis 1991)
- Waveform = N breakpoints (amplitude, duration) linearly interpolated, mutating per cycle
- Per-cycle mutation via 4 probability distributions: Gaussian, Cauchy, Logistic, Uniform
- Markov chain state memory biases walk toward previous steps
- Poisson-distributed event overlay for granular textures
- DC Block (essential for Cauchy distribution stability)
- Wavefold (foldAmt parameter)
- CytomicSVF filter per-voice (L+R) + VCA
- 2 LFOs (floor 0.01Hz) with routable targets
- 4-slot mod matrix
- 4 macros: SOLIDARITY (scatter control), INTENT (distribution morph, Cage↔Xenakis), DRIFT (mutation), SPACE
- Identity: Oort Cloud | Accent: Oort Cloud Violet #9B7FD4 | Prefix: `oort_`

## Citations (D003)
- Xenakis, GENDY algorithm (1991)
- Xenakis, "Formalized Music" (1971)
- Einstein, Brownian motion (1905)
- Markov, Markov chains (1906)

---

## Ghost Panel

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| Moog | 7.0 | "No voltage, no warmth. But the Brownian motion in the breakpoints behaves like an oscillator that never settled down." |
| Buchla | 9.0 | "The INTENT axis — Cage at zero, Xenakis at one — is exactly the question I was always trying to pose. Four distribution types for four philosophies of chance." |
| Smith | 8.0 | "Markov state memory is correct engineering. Clean MIDI handling. Poisson events add transient density without polyphony overhead." |
| Kakehashi | 8.0 | "Athens1944. Pithoprakta. The preset names teach the engine's history. Design with cultural generosity." |
| Ciani | 8.5 | "Poisson events appear in random stereo positions. Brownian mutation creates slow spatial drift. The engine breathes through physics, not automation." |
| Schulze | 9.0 | "The waveform never repeats. The Markov memory ensures it evolves but remembers where it has been. This is the engine I would have used on Irrlicht." |
| Vangelis | 8.0 | "Velocity narrows scatter — harder playing brings order from chaos. The most expressively coherent velocity implementation in today's seance series." |
| Tomita | 8.0 | "Gaussian to Cauchy to Logistic to Uniform — four distribution types are four distinct timbral textures. CauchyTails is a specific orchestral presence." |

---

## Doctrine Compliance

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 | ✅ PASS (excellent) | `velScatter = scatter * (1.0f - velTimbre * velocity)` — velocity narrows scatter; harder playing = more order from chaos. Most coherent D001 mechanic of today's seances. |
| D002 | ✅ PASS | 2 LFOs (0.01Hz floor), 4-slot mod matrix, Markov chain as persistent state-memory modulation. All 4 macros wired and conceptually meaningful. |
| D003 | ✅ PASS (exemplary) | 4 citations: Xenakis GENDY 1991, Formalized Music 1971, Einstein 1905, Markov 1906. Most citations of any new engine this session. |
| D004 | ✅ PASS | All parameters live. foldAmt/eventDensity default to 0 but activate when non-zero — by-default-off, not dead. |
| D005 | ✅ PASS (exceptional) | LFO floor 0.01Hz. GENDY mutation is per-cycle — the waveform never repeats. Markov memory creates ongoing directional drift. |
| D006 | ✅ PASS | Mod wheel → scatter (+0.3). Aftertouch → intent (+0.3). Both wired and musically coherent. |

---

## Preset Assessment

**Count:** 21 presets — all in `Source/Engines/Oort/Presets/Foundation/`

**V1 threshold status:** Score passes (8.5). Recommend ≥50 presets before shipping (Oto precedent: 34 presets / V1-confirmed).

**Naming quality:** Exceptional — best of all three today. Athens1944, SolidarityChant, Pithoprakta, FormalizedMusic, RiotPhase, TheArgument. Every name is a cultural reference or precise image.

**Macro effectiveness:** All 4 unconditional and semantically loaded:
- SOLIDARITY: 2x scatter (chaos) → 0 scatter (order) — philosophically coherent
- INTENT: Cage ↔ Xenakis distribution morph
- DRIFT: mutation rate × convergePitch
- SPACE: filter cutoff × spread

---

## Coupling

**Output:**
- ch0/ch1: Post-filter stereo
- ch2: **Current scatter value 0–1** — encodes present degree of randomness; unique coupling signal in the fleet

**Input types:**
- `AudioToFM` → breakpoint duration modulation (FM-like)
- `AmpToFilter` → filter cutoff (RMS-based)
- `EnvToMorph` → scatter modulation
- `RhythmToBlend` → triggers Poisson events

**Natural partners:** Onset (rhythm → Poisson triggers), Oracle (parallel GENDY streams), Oort ↔ Oort (two stochastic clouds with different INTENT = emergent structure from two random systems).

---

## Blessing

**B046: "The Composer Axis"**

The INTENT parameter (0 = John Cage, 1 = Iannis Xenakis) is the only synthesis parameter in the fleet — possibly in any synthesizer — that names its philosophical positions after composers. At Cage=0, pure uniform randomness. At Xenakis=1, shaped Gaussian probability toward musical positions. Every intermediate value is a position in the 20th century argument about chance in music. A performer using this parameter is a participant in that argument, not just a knob-turner.

---

## Recommendations

### P0 — None
Score passes V1 threshold at 8.5.

### P1 — Important
1. **Expand presets to ≥50 before V1** — 21 is thin. Target 50 well-curated presets across 5 moods. The naming quality is already exceptional.
2. **Entangled preset: Oort ↔ Oracle** — the fleet's two GENDY engines should have a coupled landmark preset.

### P2 — Nice to have
3. **Move presets to correct mood folders** — XenakisCloud tagged Submerged in JSON but in Foundation folder.
4. **Write `Docs/oort_synthesis_guide.md`** — the INTENT axis and distribution types deserve documentation.

---

## Path Forward

**V1-ready at 8.5.** Recommend ≥50 presets, distribution to mood folders, Oort ↔ Oracle coupling preset before ship. Then consider a targeted re-seance for potential 9.0+ if the full library demonstrates the INTENT axis's range.

---

*First seance — 2026-04-12 | Ringleader RAC Session | B046 awarded*
