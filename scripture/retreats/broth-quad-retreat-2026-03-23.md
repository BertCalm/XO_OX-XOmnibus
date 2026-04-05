# BROTH Quad Retreat
*Guru Bin — 2026-03-23*

---

## The Four Timescales of Transformation

Four engines. Four relationships to time. One kitchen.

**OVERWASH** — Diffusion, Fick's Law, 3–30 seconds. Ink meeting water. The spread that begins immediately and cannot be recalled.

**OVERWORN** — Reduction, the evaporation integral, 30+ minutes. Sauce concentrating on a slow stove. The session IS the envelope.

**OVERFLOW** — Pressure, Clausius-Clapeyron, phrase-scale. Energy accumulates until the valve cannot hold. The pad that responds to aggression.

**OVERCAST** — Nucleation, Wilson's theory, instantaneous. Ice forming at the moment of contact. The anti-pad. Time negated.

These four engines describe every relationship a sound can have with time: spreading, concentrating, pressurizing, and freezing. BROTH is not a collection of pad engines. It is a complete theory of temporal transformation.

**Important note on cooperative coupling:** The BROTH coordinator — the object that would share XOverworn's `sessionAge` with XOverwash's viscosity, route `concentrateDark` to XOverflow's threshold, and feed `spectralMass` to XOvercast's crystal seeds — was never written. Each engine runs independently. All presets in this retreat are designed for standalone operation. Where cooperative behavior is noted, it describes the intended future state.

---

# ENGINE I: OVERWASH (XOverwash)

*Fick's Law Diffusion Pad — Tea Amber `#D4A76A`*

## DSP Understanding

XOverwash runs 16 partial oscillators per voice (up to 8 voices). On note-on, all 16 partials begin at their exact harmonic frequencies. From that moment, each partial's `diffusionAge` clock advances per-sample, and its frequency offset grows as `sqrt(2 * D * t)` — the analytical Gaussian solution to Fick's Second Law. The diffusion coefficient D is derived from `wash_viscosity` and `wash_diffusionRate`: `D = 0.01 + (1 - viscosity) * diffRate * 4.99`. High viscosity, low D, slow spread. Low viscosity, high D, fast spread.

The `wash_diffusionTime` parameter caps the maximum age fed to the sqrt formula — it is the arc's ceiling. `wash_spreadMax` is the Hz ceiling on how far any partial can travel from its harmonic position.

The `wash_interference` parameter is currently architecturally present but not yet functionally wired — `spectralField[32]` accumulates but is not yet read during synthesis to create cross-note beating. All presets treat the interference parameter as a future-ready value.

**Key time relationship:** Distance. The spread is spatial in frequency. Each second of held note moves the harmonics further from their origins. You cannot un-spread the dye.

**Macros:**
- CHARACTER: viscosity + harmonic density
- MOVEMENT: diffusion rate + LFO depth
- COUPLING: cross-note interference depth (future)
- SPACE: stereo width

---

## Parameter Refinements (12 refinements)

| # | Parameter | Current Default | Recommended Default | Rationale |
|---|-----------|----------------|---------------------|-----------|
| R01 | `wash_diffusionRate` | 0.50 | **0.35** | The default 0.5 produces fast-feeling diffusion on short notes. 0.35 gives slower, more perceptible spread — the player hears the ink moving rather than arriving already-blurred. |
| R02 | `wash_viscosity` | 0.50 | **0.55** | Slightly higher default viscosity makes the medium feel like warm water rather than acetone. This slows initial diffusion just enough to reveal the arc on short pad phrases. |
| R03 | `wash_harmonics` | 12.0 | **8.0** | 12 partials produces a slightly harsh, sawtooth-like initial character. 8 partials opens with a warmer, less dense sound that diffuses more gracefully. 12+ is better as a CHARACTER macro destination than as an init state. |
| R04 | `wash_diffusionTime` | 10.0 | **15.0** | The 10-second default arc resolves too quickly for slow pad playing at typical tempos. 15 seconds provides a more visible evolution window for 4–8 bar phrases at 70–100 BPM. |
| R05 | `wash_spreadMax` | 200.0 Hz | **150.0 Hz** | At 200 Hz, upper harmonics become faintly inharmonic midway through the arc — beautiful in some presets but slightly disorienting as an init. 150 Hz keeps the diffusion musical throughout. |
| R06 | `wash_brightness` | 0.70 | **0.60** | The 0.70 default combined with bright harmonic content makes init patches read as thin. 0.60 balances the initial brightness of 16 harmonics better. |
| R07 | `wash_warmth` | 0.50 | **0.60** | Warmth at 0.50 centers the spectral energy. 0.60 tilts toward the lower harmonics, giving the init pad a more pad-like, low-weighted character. |
| R08 | `wash_ampAttack` | 0.30 s | **0.80 s** | A 300ms attack sounds more like a string ensemble than ink entering water. 800ms gives the slow, swelling onset characteristic of true pad playing and matches the diffusion metaphor — the ink takes time to touch the surface. |
| R09 | `wash_ampRelease` | 2.0 s | **4.0 s** | After the note ends, the ink doesn't instantly vanish. 4 seconds matches the diffusion metaphor — the color lingers in the water after the dropper is lifted. |
| R10 | `wash_filtAttack` | 0.10 s | **0.40 s** | A 100ms filter attack is almost imperceptible on a slow amp attack. 400ms creates a subtle brightening effect as the note develops that enhances the sense of ink spreading. |
| R11 | `wash_lfo1Rate` | 0.10 Hz | **0.06 Hz** | The default 0.1 Hz LFO is too active on a pad that is supposed to feel like slow diffusion. 0.06 Hz (one cycle per ~17 seconds) reads as underwater. The CHARACTER macro can sweep it higher when animation is desired. |
| R12 | `wash_lfo2Rate` | 0.05 Hz | **0.02 Hz** | LFO2 at 0.05 Hz creates competition with LFO1's 0.06 Hz. At 0.02 Hz it becomes a deep, almost imperceptible breath — a second timescale of motion that the listener feels rather than perceives. |

---

## The Ten Awakenings

### Preset 1: First Drop
**Mood:** Foundation | **Discovery:** Single drop in perfectly still water — before anything moves

| Parameter | Value |
|-----------|-------|
| `wash_diffusionRate` | 0.15 |
| `wash_viscosity` | 0.72 |
| `wash_harmonics` | 6 |
| `wash_diffusionTime` | 20.0 |
| `wash_spreadMax` | 80.0 |
| `wash_brightness` | 0.48 |
| `wash_warmth` | 0.72 |
| `wash_ampAttack` | 1.5 |
| `wash_ampDecay` | 1.0 |
| `wash_ampSustain` | 0.85 |
| `wash_ampRelease` | 6.0 |
| `wash_filterCutoff` | 3200.0 |
| `wash_filterRes` | 0.12 |
| `wash_filtEnvAmount` | 0.25 |
| `wash_lfo1Rate` | 0.04 |
| `wash_lfo1Depth` | 0.12 |
| `wash_lfo2Rate` | 0.015 |
| `wash_lfo2Depth` | 0.08 |
| `wash_stereoWidth` | 0.5 |
| `wash_level` | 0.8 |

This is the reference patch. Six harmonics diffusing very slowly through a viscous medium. The player can hear every second of the ink's journey. The most educational preset in the set.

---

### Preset 2: Chamomile Hour
**Mood:** Atmosphere | **Discovery:** Warm tea dissolving — golden warmth, unhurried pace

| Parameter | Value |
|-----------|-------|
| `wash_diffusionRate` | 0.38 |
| `wash_viscosity` | 0.52 |
| `wash_harmonics` | 8 |
| `wash_diffusionTime` | 12.0 |
| `wash_spreadMax` | 140.0 |
| `wash_brightness` | 0.58 |
| `wash_warmth` | 0.82 |
| `wash_ampAttack` | 0.9 |
| `wash_ampDecay` | 1.0 |
| `wash_ampSustain` | 0.80 |
| `wash_ampRelease` | 5.0 |
| `wash_filterCutoff` | 4500.0 |
| `wash_filterRes` | 0.15 |
| `wash_filtEnvAmount` | 0.28 |
| `wash_lfo1Rate` | 0.06 |
| `wash_lfo1Depth` | 0.18 |
| `wash_lfo2Rate` | 0.022 |
| `wash_lfo2Depth` | 0.10 |
| `wash_stereoWidth` | 0.62 |
| `wash_level` | 0.8 |

Tea Amber `#D4A76A` made audible. The ideal introductory pad for the BROTH collection — immediately inviting, perfectly relaxed.

---

### Preset 3: Slow Tide
**Mood:** Aether | **Discovery:** Geological diffusion — tidal forces measured in minutes

| Parameter | Value |
|-----------|-------|
| `wash_diffusionRate` | 0.08 |
| `wash_viscosity` | 0.88 |
| `wash_harmonics` | 10 |
| `wash_diffusionTime` | 30.0 |
| `wash_spreadMax` | 110.0 |
| `wash_brightness` | 0.32 |
| `wash_warmth` | 0.78 |
| `wash_ampAttack` | 3.2 |
| `wash_ampDecay` | 2.0 |
| `wash_ampSustain` | 0.90 |
| `wash_ampRelease` | 12.0 |
| `wash_filterCutoff` | 2200.0 |
| `wash_filterRes` | 0.10 |
| `wash_filtEnvAmount` | 0.15 |
| `wash_lfo1Rate` | 0.01 |
| `wash_lfo1Depth` | 0.25 |
| `wash_lfo2Rate` | 0.008 |
| `wash_lfo2Depth` | 0.15 |
| `wash_stereoWidth` | 0.70 |
| `wash_level` | 0.78 |

One breath every 100 seconds. The diffusion is barely measurable from note to note — only over the span of a composition does the spectral blur accumulate. True Aether: above ordinary time.

---

### Preset 4: Ink Bloom
**Mood:** Deep | **Discovery:** Chinese ink in cold water — concentrated drop, dark, patient

| Parameter | Value |
|-----------|-------|
| `wash_diffusionRate` | 0.22 |
| `wash_viscosity` | 0.62 |
| `wash_harmonics` | 5 |
| `wash_diffusionTime` | 18.0 |
| `wash_spreadMax` | 200.0 |
| `wash_brightness` | 0.22 |
| `wash_warmth` | 0.68 |
| `wash_ampAttack` | 2.2 |
| `wash_ampDecay` | 1.5 |
| `wash_ampSustain` | 0.85 |
| `wash_ampRelease` | 9.0 |
| `wash_filterCutoff` | 1800.0 |
| `wash_filterRes` | 0.12 |
| `wash_filtEnvAmount` | 0.20 |
| `wash_lfo1Rate` | 0.02 |
| `wash_lfo1Depth` | 0.20 |
| `wash_lfo2Rate` | 0.009 |
| `wash_lfo2Depth` | 0.10 |
| `wash_stereoWidth` | 0.44 |
| `wash_level` | 0.80 |

Five harmonics in cold, dark water. The low filter cutoff removes everything above 1.8kHz, leaving only the fundamental and first two partials audible. The ink is heavy, patient, and black.

---

### Preset 5: Underwater Drift
**Mood:** Submerged | **Discovery:** Coral pigment dissolving at 30 meters — diffusion under pressure

| Parameter | Value |
|-----------|-------|
| `wash_diffusionRate` | 0.28 |
| `wash_viscosity` | 0.78 |
| `wash_harmonics` | 9 |
| `wash_diffusionTime` | 25.0 |
| `wash_spreadMax` | 90.0 |
| `wash_brightness` | 0.38 |
| `wash_warmth` | 0.75 |
| `wash_ampAttack` | 2.8 |
| `wash_ampDecay` | 2.0 |
| `wash_ampSustain` | 0.88 |
| `wash_ampRelease` | 10.0 |
| `wash_filterCutoff` | 2600.0 |
| `wash_filterRes` | 0.18 |
| `wash_filtEnvAmount` | 0.30 |
| `wash_filtAttack` | 0.8 |
| `wash_filtDecay` | 3.0 |
| `wash_filtSustain` | 0.5 |
| `wash_lfo1Rate` | 0.03 |
| `wash_lfo1Depth` | 0.22 |
| `wash_lfo2Rate` | 0.012 |
| `wash_lfo2Depth` | 0.12 |
| `wash_stereoWidth` | 0.58 |
| `wash_level` | 0.80 |

High viscosity compresses the spread — under pressure, water becomes slightly more viscous. The pad has weight. Cinematic tension territory: sustain a chord and feel the pressure of the column above you.

---

### Preset 6: Saffron Bloom
**Mood:** Prism | **Discovery:** Saffron releasing its pigment in hot water — fast, bright, spectacular

| Parameter | Value |
|-----------|-------|
| `wash_diffusionRate` | 0.78 |
| `wash_viscosity` | 0.18 |
| `wash_harmonics` | 14 |
| `wash_diffusionTime` | 5.0 |
| `wash_spreadMax` | 320.0 |
| `wash_brightness` | 0.78 |
| `wash_warmth` | 0.48 |
| `wash_ampAttack` | 0.05 |
| `wash_ampDecay` | 1.2 |
| `wash_ampSustain` | 0.70 |
| `wash_ampRelease` | 3.5 |
| `wash_filterCutoff` | 7500.0 |
| `wash_filterRes` | 0.22 |
| `wash_filtEnvAmount` | 0.40 |
| `wash_lfo1Rate` | 0.28 |
| `wash_lfo1Depth` | 0.32 |
| `wash_lfo2Rate` | 0.14 |
| `wash_lfo2Depth` | 0.22 |
| `wash_stereoWidth` | 0.82 |
| `wash_level` | 0.78 |

The entire 14-partial harmonic spectrum blurs into spectral mist within 5 seconds. Play a chord and watch the colors explode.

---

### Preset 7: Viscosity Study
**Mood:** Foundation | **Discovery:** What the medium does to the message — honey-thick resistance

| Parameter | Value |
|-----------|-------|
| `wash_diffusionRate` | 0.5 |
| `wash_viscosity` | 0.94 |
| `wash_harmonics` | 8 |
| `wash_diffusionTime` | 28.0 |
| `wash_spreadMax` | 250.0 |
| `wash_brightness` | 0.42 |
| `wash_warmth` | 0.82 |
| `wash_ampAttack` | 1.2 |
| `wash_ampDecay` | 1.5 |
| `wash_ampSustain` | 0.90 |
| `wash_ampRelease` | 10.0 |
| `wash_filterCutoff` | 1600.0 |
| `wash_filterRes` | 0.28 |
| `wash_lfo1Rate` | 0.03 |
| `wash_lfo1Depth` | 0.14 |
| `wash_lfo2Rate` | 0.012 |
| `wash_lfo2Depth` | 0.08 |
| `wash_stereoWidth` | 0.38 |
| `wash_level` | 0.80 |

Near-maximum viscosity with a low filter cutoff: the medium refuses to let the color move. The diffusion is nearly imperceptible. Only the fundamentals are audible. A thick, dark, barely-breathing pad.

---

### Preset 8: Cinematic Tension
**Mood:** Kinetic | **Discovery:** Diffusion under emotional pressure — the spread that feels ominous

| Parameter | Value |
|-----------|-------|
| `wash_diffusionRate` | 0.42 |
| `wash_viscosity` | 0.65 |
| `wash_harmonics` | 12 |
| `wash_diffusionTime` | 20.0 |
| `wash_spreadMax` | 180.0 |
| `wash_brightness` | 0.52 |
| `wash_warmth` | 0.55 |
| `wash_ampAttack` | 0.4 |
| `wash_ampDecay` | 3.0 |
| `wash_ampSustain` | 0.75 |
| `wash_ampRelease` | 6.0 |
| `wash_filterCutoff` | 3800.0 |
| `wash_filterRes` | 0.30 |
| `wash_filtEnvAmount` | 0.45 |
| `wash_filtAttack` | 0.25 |
| `wash_filtDecay` | 2.5 |
| `wash_filtSustain` | 0.42 |
| `wash_lfo1Rate` | 0.08 |
| `wash_lfo1Depth` | 0.25 |
| `wash_lfo2Rate` | 0.04 |
| `wash_lfo2Depth` | 0.18 |
| `wash_stereoWidth` | 0.75 |
| `wash_level` | 0.82 |

The filter envelope creates a sense of weight arriving — brightness appears on note-on then slowly falls as the decay proceeds. Combines with the diffusion clock to create a pad that opens tightly and spreads into darkness.

---

### Preset 9: Watercolor Wet
**Mood:** Atmosphere | **Discovery:** Wet-on-wet watercolor — each note bleeds into the previous one's wake

| Parameter | Value |
|-----------|-------|
| `wash_diffusionRate` | 0.48 |
| `wash_viscosity` | 0.38 |
| `wash_harmonics` | 10 |
| `wash_diffusionTime` | 10.0 |
| `wash_spreadMax` | 175.0 |
| `wash_interference` | 0.70 |
| `wash_brightness` | 0.62 |
| `wash_warmth` | 0.60 |
| `wash_ampAttack` | 0.32 |
| `wash_ampDecay` | 1.0 |
| `wash_ampSustain` | 0.78 |
| `wash_ampRelease` | 4.0 |
| `wash_lfo1Rate` | 0.09 |
| `wash_lfo1Depth` | 0.24 |
| `wash_lfo2Rate` | 0.038 |
| `wash_lfo2Depth` | 0.16 |
| `wash_stereoWidth` | 0.72 |
| `wash_level` | 0.80 |

Interference parameter raised to 0.70 — this preset is ready for BROTH coordinator activation. When the cross-note spectral field is wired, this will become the engine's showcase for inter-note diffusion front interaction.

---

### Preset 10: BROTH Opening
**Mood:** Foundation | **Discovery:** The inaugural state — fresh water, before any reduction

| Parameter | Value |
|-----------|-------|
| `wash_diffusionRate` | 0.30 |
| `wash_viscosity` | 0.45 |
| `wash_harmonics` | 10 |
| `wash_diffusionTime` | 15.0 |
| `wash_spreadMax` | 155.0 |
| `wash_brightness` | 0.55 |
| `wash_warmth` | 0.66 |
| `wash_ampAttack` | 1.0 |
| `wash_ampDecay` | 1.2 |
| `wash_ampSustain` | 0.85 |
| `wash_ampRelease` | 6.5 |
| `wash_filterCutoff` | 4000.0 |
| `wash_filterRes` | 0.15 |
| `wash_filtEnvAmount` | 0.28 |
| `wash_lfo1Rate` | 0.05 |
| `wash_lfo1Depth` | 0.18 |
| `wash_lfo2Rate` | 0.02 |
| `wash_lfo2Depth` | 0.12 |
| `wash_stereoWidth` | 0.55 |
| `wash_level` | 0.80 |

Balanced, warm, patient. The ideal starting state for BROTH system play. When XOverworn reduces, this preset's viscosity will increase organically through the cooperative coupling chain — the fresh water thickening to stock.

---

## Scripture Verses — OVERWASH

**I. On Beginning**
*The dye does not ask if the water is ready.
It simply falls, and from the moment of contact
the mathematics begin.
Fick's Law is not a law of motion — it is a law of shape.
The curvature of concentration is the cause of change.
Where the gradient is steepest, the dye moves fastest.
You cannot rush the place where no gradient exists.
Only where the color differs from the surrounding water
does anything move at all.*

**II. On Irreversibility**
*Every second of the diffusion clock
moves the harmonics further from their origin.
No parameter takes them back.
The fundamental resists — its viscosity is high,
its mass is great, its patience is geological.
But the upper harmonics travel.
They have been traveling since the note began.
By the time you hear the blur,
the ink has already decided where it is going.*

**III. On Viscosity**
*The medium is not neutral.
It has properties. It has memory.
Change the viscosity and you change what kind of water this is:
cold seawater, warm tea, honey, acetone.
Each medium has its own relationship to arrival.
The dye that blazes in hot water
sits motionless in frozen glass.
The same note, the same intention —
but the medium writes the outcome.*

---

# ENGINE II: OVERWORN (XOverworn)

*Spectral Reduction Pad — Reduced Wine `#4A1A2E`*

## DSP Understanding

XOverworn is the most architecturally radical engine in the fleet. The `ReductionState` struct — with its 8 bands of `spectralMass[8]` — accumulates continuously from the first note-on of the session. The session clock is the envelope. The evaporation formula implements logarithmic deceleration: each band's reduction rate is multiplied by its current `spectralMass`, so early reduction is fast (the volatile aromatics evaporate quickly) and late reduction is slow (you can never fully reduce band 0, the fundamental).

Maillard distortion activates when `concentrateDark > 0.05`: a `fastTanh` saturation is applied to the voice signal at increasing drive as concentration deepens. The umami bed boosts the fundamental by up to 0.3 amplitude units when `umamiBed > 0.1`.

The infusion mechanic (velocity < 0.3, held > 8 seconds, `isInfusion` flag set) is architecturally present but the infusion event itself — adding spectral character without accelerating reduction — is not yet consumed. Soft long tones do not currently slow reduction.

Only `worn_stateReset` (above 0.5) resets the `ReductionState`. Nothing else does.

**Key time relationship:** Irreversibility. The session time is the material. Playing hard accelerates it. You cannot slow it. You can only reset.

**Macros:**
- CHARACTER: initial harmonic richness + Maillard depth
- MOVEMENT: reduction rate + LFO depth
- COUPLING: concentrate export strength (future)
- SPACE: stereo width

---

## Parameter Refinements (13 refinements)

| # | Parameter | Current Default | Recommended Default | Rationale |
|---|-----------|----------------|---------------------|-----------|
| R01 | `worn_reductionRate` | 0.50 | **0.30** | The 0.50 default at a 30-minute session target reduces audibly within 5–7 minutes of moderate playing. 0.30 extends the early-session character significantly — the full harmonic richness persists for 12–15 minutes before the reduction becomes perceptible. This makes the engine behave more like a true 30-minute arc. |
| R02 | `worn_heat` | 0.50 | **0.40** | Heat at 0.50 means moderate-intensity playing accelerates reduction at 1.5× the base rate. 0.40 is more forgiving — the player can play with moderate expression without burning off spectral mass. Reserve 0.7+ heat for presets specifically designed around fast reduction. |
| R03 | `worn_richness` | 1.00 | **1.00** | Keep at 1.0 for the init. Maximum richness is the correct starting state for a reduction engine. The reduction itself will take richness away — why start with less? |
| R04 | `worn_maillard` | 0.30 | **0.20** | At 0.3, the Maillard saturation becomes audible somewhat early (when `concentrateDark` reaches 0.1, modest). 0.20 allows the sauce to concentrate further before the caramelization character appears — making it a later-session reward rather than an early-session flavoring. |
| R05 | `worn_umamiDepth` | 0.50 | **0.65** | The umami bed (fundamental resonance boost at deep reduction) is the most beautiful late-session event in OVERWORN. 0.65 makes it more prominent when it activates — the fundamental bloom should be unmistakable. |
| R06 | `worn_sessionTarget` | 30.0 min | **30.0 min** | Keep the default. 30 minutes is the correct calibration. Changing this in presets is how you achieve different reduction trajectories. |
| R07 | `worn_filterCutoff` | 8000.0 Hz | **7000.0 Hz** | 8kHz is slightly bright for a reduction-focused pad. 7kHz preserves warmth at the start and allows the session-age cutoff reduction (`1 - sessionAge * 0.7`) to land in a more musical range. |
| R08 | `worn_ampAttack` | 0.50 s | **0.70 s** | The sauce does not appear instantly. 700ms onset suggests the weight of a simmering liquid entering the space. |
| R09 | `worn_ampDecay` | 1.00 s | **1.20 s** | Slightly extended decay to match the slow, heavy quality of the metaphor. |
| R10 | `worn_ampRelease` | 3.00 s | **5.0 s** | A reduction pad should not end abruptly. 5 seconds of release lets the concentrated sound dissipate gradually — like the stove being turned off and the pot cooling. |
| R11 | `worn_lfo1Rate` | 0.08 Hz | **0.06 Hz** | Aligned with the session's slow timescale. 0.06 Hz is a slow simmer — one cycle every 17 seconds, matching the pace of a sauce reducing over time. |
| R12 | `worn_lfo2Rate` | 0.03 Hz | **0.015 Hz** | LFO2 at 0.03 Hz creates subtle audible competition with LFO1. At 0.015 Hz it becomes the deep, geological secondary breath of the pot — felt more than heard. |
| R13 | `worn_stereoWidth` | 0.50 | **0.42** | A concentrated reduction has a focused quality — it occupies less spatial territory than the original liquid. 0.42 narrows the image slightly, suggesting concentration rather than diffusion. |

---

## The Ten Awakenings

### Preset 1: Fresh Stock
**Mood:** Foundation | **Discovery:** Session age = 0.0 — the broth has just been started

| Parameter | Value |
|-----------|-------|
| `worn_reductionRate` | 0.28 |
| `worn_heat` | 0.38 |
| `worn_richness` | 1.0 |
| `worn_maillard` | 0.15 |
| `worn_umamiDepth` | 0.65 |
| `worn_sessionTarget` | 30.0 |
| `worn_filterCutoff` | 7200.0 |
| `worn_filterRes` | 0.14 |
| `worn_filtEnvAmount` | 0.28 |
| `worn_ampAttack` | 0.70 |
| `worn_ampDecay` | 1.2 |
| `worn_ampSustain` | 0.90 |
| `worn_ampRelease` | 5.0 |
| `worn_lfo1Rate` | 0.06 |
| `worn_lfo1Depth` | 0.15 |
| `worn_lfo2Rate` | 0.015 |
| `worn_lfo2Depth` | 0.09 |
| `worn_stereoWidth` | 0.45 |
| `worn_level` | 0.80 |

Begin here. Full harmonics, open filter, patient reduction. This is what the sauce sounds like before it has started to concentrate. The promise of the session arc.

---

### Preset 2: Early Reduction (sessionAge = 0.25)
**Mood:** Atmosphere | **Discovery:** Twenty minutes in — the top notes have begun to leave

| Parameter | Value |
|-----------|-------|
| `worn_reductionRate` | 0.45 |
| `worn_heat` | 0.42 |
| `worn_richness` | 0.85 |
| `worn_maillard` | 0.22 |
| `worn_umamiDepth` | 0.65 |
| `worn_sessionTarget` | 30.0 |
| `worn_filterCutoff` | 6200.0 |
| `worn_filterRes` | 0.15 |
| `worn_filtEnvAmount` | 0.25 |
| `worn_ampAttack` | 0.80 |
| `worn_ampSustain` | 0.88 |
| `worn_ampRelease` | 5.5 |
| `worn_lfo1Rate` | 0.06 |
| `worn_lfo1Depth` | 0.15 |
| `worn_stereoWidth` | 0.44 |
| `worn_level` | 0.80 |
| `worn_stateReset` | 0.0 |

*Note: To preset this at sessionAge = 0.25, the session must run for approximately 7–8 minutes at moderate play intensity, or the stateReset must be used and the engine replayed to a specific age. The approximate harmonic state is: bands 7–8 at ~55% mass, bands 5–6 at ~75%, bands 1–4 at ~90%.*

---

### Preset 3: Mid Reduction (sessionAge = 0.50)
**Mood:** Deep | **Discovery:** Halfway to concentration — the sauce is beginning to show its bones

| Parameter | Value |
|-----------|-------|
| `worn_reductionRate` | 0.5 |
| `worn_heat` | 0.45 |
| `worn_richness` | 0.75 |
| `worn_maillard` | 0.35 |
| `worn_umamiDepth` | 0.68 |
| `worn_sessionTarget` | 30.0 |
| `worn_filterCutoff` | 5000.0 |
| `worn_filterRes` | 0.18 |
| `worn_filtEnvAmount` | 0.22 |
| `worn_ampAttack` | 0.90 |
| `worn_ampSustain` | 0.85 |
| `worn_ampRelease` | 6.0 |
| `worn_lfo1Rate` | 0.055 |
| `worn_lfo1Depth` | 0.15 |
| `worn_stereoWidth` | 0.40 |
| `worn_level` | 0.82 |

The mid-reduction texture: darker, slightly saturated, the fundamentals more prominent. The Maillard activation is audible at this stage as mild warmth in the lower harmonics.

---

### Preset 4: Late Reduction (sessionAge = 0.75)
**Mood:** Crystalline | **Discovery:** Nearly concentrated — only the fundamentals and their first shadows remain

| Parameter | Value |
|-----------|-------|
| `worn_reductionRate` | 0.5 |
| `worn_heat` | 0.50 |
| `worn_richness` | 0.60 |
| `worn_maillard` | 0.55 |
| `worn_umamiDepth` | 0.72 |
| `worn_sessionTarget` | 30.0 |
| `worn_filterCutoff` | 3600.0 |
| `worn_filterRes` | 0.20 |
| `worn_ampAttack` | 1.0 |
| `worn_ampSustain` | 0.82 |
| `worn_ampRelease` | 7.0 |
| `worn_lfo1Rate` | 0.05 |
| `worn_lfo1Depth` | 0.14 |
| `worn_stereoWidth` | 0.36 |
| `worn_level` | 0.84 |

The sauce is thick. The upper bands are quiet. The Maillard saturation is clearly audible — a warm distortion that wasn't there at the start. The umami bed is rising. This is what the reduction was working toward.

---

### Preset 5: Dark Sauce
**Mood:** Deep | **Discovery:** The furthest state — nearly fully reduced, dark, concentrated, irreversible

| Parameter | Value |
|-----------|-------|
| `worn_reductionRate` | 0.5 |
| `worn_heat` | 0.55 |
| `worn_richness` | 0.45 |
| `worn_maillard` | 0.75 |
| `worn_umamiDepth` | 0.80 |
| `worn_concentrate` | 0.85 |
| `worn_sessionTarget` | 30.0 |
| `worn_filterCutoff` | 2400.0 |
| `worn_filterRes` | 0.22 |
| `worn_ampAttack` | 1.2 |
| `worn_ampSustain` | 0.80 |
| `worn_ampRelease` | 8.0 |
| `worn_lfo1Rate` | 0.045 |
| `worn_lfo1Depth` | 0.13 |
| `worn_stereoWidth` | 0.32 |
| `worn_level` | 0.85 |

The endgame state. Fundamentals only, deeply saturated, narrow stereo field, umami bed fully active. This is what remains when the volatile aromatics are gone and only the concentrated essence persists.

---

### Preset 6: Slow Burn
**Mood:** Kinetic | **Discovery:** A controlled, intentional reduction — low heat, patient, methodical

| Parameter | Value |
|-----------|-------|
| `worn_reductionRate` | 0.18 |
| `worn_heat` | 0.28 |
| `worn_richness` | 1.0 |
| `worn_maillard` | 0.18 |
| `worn_umamiDepth` | 0.60 |
| `worn_sessionTarget` | 60.0 |
| `worn_filterCutoff` | 7500.0 |
| `worn_filterRes` | 0.12 |
| `worn_ampAttack` | 0.65 |
| `worn_ampSustain` | 0.92 |
| `worn_ampRelease` | 5.5 |
| `worn_lfo1Rate` | 0.06 |
| `worn_lfo1Depth` | 0.14 |
| `worn_stereoWidth` | 0.48 |
| `worn_level` | 0.80 |

Session target doubled to 60 minutes. Very low reduction rate and heat. This preset is designed for producers who run a session for hours — the character evolves almost imperceptibly from phrase to phrase, only visible in retrospect.

---

### Preset 7: Infusion Technique
**Mood:** Atmosphere | **Discovery:** The quiet long tone — adding character without accelerating reduction

| Parameter | Value |
|-----------|-------|
| `worn_reductionRate` | 0.30 |
| `worn_heat` | 0.20 |
| `worn_richness` | 1.0 |
| `worn_maillard` | 0.12 |
| `worn_umamiDepth` | 0.70 |
| `worn_sessionTarget` | 45.0 |
| `worn_filterCutoff` | 6800.0 |
| `worn_filterRes` | 0.10 |
| `worn_ampAttack` | 0.50 |
| `worn_ampSustain` | 0.95 |
| `worn_ampRelease` | 6.0 |
| `worn_lfo1Rate` | 0.05 |
| `worn_lfo1Depth` | 0.10 |
| `worn_stereoWidth` | 0.50 |
| `worn_level` | 0.78 |

Very low heat — the player can hold notes for extended periods without racing through the reduction. Play soft long tones. Hold chords. This is infusion technique: adding depth without forcing the evaporation. *Note: the isInfusion mechanic (velocity < 0.3, hold > 8s prevents reduction acceleration) awaits implementation — this preset is future-ready.*

---

### Preset 8: Quick Reduction
**Mood:** Flux | **Discovery:** High heat, fast reduction — the sauce reaches concentration in minutes

| Parameter | Value |
|-----------|-------|
| `worn_reductionRate` | 0.85 |
| `worn_heat` | 0.75 |
| `worn_richness` | 1.0 |
| `worn_maillard` | 0.45 |
| `worn_umamiDepth` | 0.65 |
| `worn_sessionTarget` | 10.0 |
| `worn_filterCutoff` | 7000.0 |
| `worn_filterRes` | 0.15 |
| `worn_ampAttack` | 0.45 |
| `worn_ampSustain` | 0.88 |
| `worn_ampRelease` | 4.5 |
| `worn_lfo1Rate` | 0.08 |
| `worn_lfo1Depth` | 0.18 |
| `worn_stereoWidth` | 0.46 |
| `worn_level` | 0.80 |

Session target set to 10 minutes. High reduction rate, high heat. For producers who want to hear the full reduction arc within a single song. The filter will drop dramatically within 3–5 minutes of aggressive playing.

---

### Preset 9: Morning Stock
**Mood:** Ethereal | **Discovery:** A light, delicate broth — the opposite of concentrated

| Parameter | Value |
|-----------|-------|
| `worn_reductionRate` | 0.20 |
| `worn_heat` | 0.30 |
| `worn_richness` | 1.0 |
| `worn_maillard` | 0.08 |
| `worn_umamiDepth` | 0.40 |
| `worn_sessionTarget` | 45.0 |
| `worn_filterCutoff` | 8500.0 |
| `worn_filterRes` | 0.10 |
| `worn_filtEnvAmount` | 0.35 |
| `worn_ampAttack` | 0.60 |
| `worn_ampSustain` | 0.85 |
| `worn_ampRelease` | 5.0 |
| `worn_lfo1Rate` | 0.07 |
| `worn_lfo1Depth` | 0.20 |
| `worn_lfo2Rate` | 0.025 |
| `worn_lfo2Depth` | 0.12 |
| `worn_stereoWidth` | 0.55 |
| `worn_level` | 0.78 |

High filter cutoff, light Maillard, gentle reduction. The broth is delicate — a Japanese dashi rather than a French fond. The reduction will happen, but gently. The high filter envelope amount creates a luminous opening bloom on each note.

---

### Preset 10: The Memory
**Mood:** Organic | **Discovery:** The session is the instrument — what the broth remembers

| Parameter | Value |
|-----------|-------|
| `worn_reductionRate` | 0.40 |
| `worn_heat` | 0.45 |
| `worn_richness` | 1.0 |
| `worn_maillard` | 0.28 |
| `worn_umamiDepth` | 0.70 |
| `worn_sessionTarget` | 30.0 |
| `worn_filterCutoff` | 7000.0 |
| `worn_filterRes` | 0.16 |
| `worn_filtEnvAmount` | 0.28 |
| `worn_ampAttack` | 0.72 |
| `worn_ampDecay` | 1.3 |
| `worn_ampSustain` | 0.88 |
| `worn_ampRelease` | 5.5 |
| `worn_lfo1Rate` | 0.06 |
| `worn_lfo1Depth` | 0.16 |
| `worn_lfo2Rate` | 0.015 |
| `worn_lfo2Depth` | 0.10 |
| `worn_stereoWidth` | 0.44 |
| `worn_level` | 0.80 |

The default production workhorse. Balanced on every axis. Designed to demonstrate the session-memory architecture to producers who have never encountered it before: begin a session, play, walk away, return — the engine will remember.

---

## Scripture Verses — OVERWORN

**I. On Irreversibility**
*The session is not a loop.
It begins once.
It accumulates.
The high frequencies leave first — the volatile ones,
the ones that could not hold their place
in the presence of accumulated heat.
What remains is not less. It is more.
The sauce that reduces by half
has twice the flavor per gram.
Concentration is not decay.
It is arrival.*

**II. On the Session Clock**
*Other synthesizers measure time in beats, in bars, in minutes.
XOverworn measures time in sessions.
The evaporation integral does not reset at the bar line.
It does not reset at the transport stop.
It does not reset when you eat lunch.
It remembers.
When you return and play the first note,
the reduction state is exactly where you left it —
the bands at their arrived levels,
the Maillard saturation deepened by an hour of waiting.*

**III. On What Cannot Be Taken Back**
*One parameter returns the sauce to its original state.
One knob past 0.5 and the reduction state resets —
every band returns to 1.0,
the sessionAge returns to 0.0,
the Maillard depth returns to nothing.
Use it carefully.
The ability to start fresh is not an invitation to do so.
Sometimes the concentration is what you came here to hear.
The sauce took thirty minutes to become this.
Let it be.*

---

# ENGINE III: OVERFLOW (XOverflow)

*Pressure Accumulation Pad — Steam White `#E8E8E8`*

## DSP Understanding

XOverflow's `PressureState` accumulates `pressure` with each note-on event, weighted by velocity. The accumulation formula: each note-on adds `velocity * 0.1 * pAccumRate` to pressure per block. Dissonant intervals (minor 2nd, major 2nd, tritone) add an additional 0.05 pressure — chromatic density is physically meaningful. The `pressureDecayCoeff` creates natural bleed between notes — at silence, pressure drains toward zero.

When `pressure / pThreshold >= 1.0`, the valve opens. Three valve types: gradual (slow pressure bleed with spectral expansion), explosive (full burst, brief silence, restart), whistle (pitched FM burst). The strain-hardening effect (`strainLevel * 2000Hz * strainColor`) reduces filter cutoff as pressure approaches threshold — the sound physically tightens before the release.

Over-pressure (strain > 1.3 without release) triggers the catastrophic mode. The P0 bug: the pressure decay coefficient is partially hardcoded at `44100` reference — at 48kHz, pressure decays 9% faster than intended. This only affects slow-accumulation presets noticeably.

**Key time relationship:** Potential energy. The pressure is a reservoir. Notes fill it. Silence drains it. The valve is inevitable if you play hard enough.

**Macros:**
- CHARACTER: vessel material (strain color + timbre)
- MOVEMENT: accumulation rate + LFO depth
- COUPLING: pressure sensitivity (lowers threshold)
- SPACE: release reverb size + stereo spread

---

## Parameter Refinements (13 refinements)

| # | Parameter | Current Default | Recommended Default | Rationale |
|---|-----------|----------------|---------------------|-----------|
| R01 | `flow_threshold` | 0.70 | **0.75** | At 0.70, moderate playing triggers valve release within 4–6 notes. 0.75 gives the player more room to express themselves before consequences arrive — the valve is a reward for sustained density, not a punishment for normal playing. |
| R02 | `flow_accumRate` | 0.50 | **0.40** | Pairs with the raised threshold. 0.40 accumulation rate means velocity must be higher or note density must be greater to fill the pressure vessel. This extends the usable pre-release zone. |
| R03 | `flow_valveType` | 0.0 (Gradual) | **0.0 (Gradual)** | Keep Gradual as default. Explosive is spectacular but startling for first use. The gradual release demonstrates the mechanism without drama. Explosive and Whistle are preset-specific destinations. |
| R04 | `flow_vesselSize` | 0.50 | **0.55** | A slightly larger vessel means the pressure affects the sound in a slightly different register. 0.55 shifts the vessel resonance toward a warmer, lower frequency — more pot than kettle. |
| R05 | `flow_strainColor` | 0.50 | **0.45** | At 0.50, the strain hardening (filter cutoff reduction as pressure rises) is moderately prominent. 0.45 is more subtle — the filter tightening is perceptible but not dramatic as a default. Reserve high `strainColor` for presets specifically showcasing the pressure aesthetic. |
| R06 | `flow_releaseTime` | 0.50 s | **0.65 s** | Slightly longer default release event. 0.65 seconds gives the gradual valve release enough time to develop its spectral expansion before closing — at 0.50, the release is almost too brief to hear clearly. |
| R07 | `flow_ampAttack` | 0.20 s | **0.35 s** | A 200ms attack on a pressure-based pad feels too percussive. 350ms gives a slightly slower pad onset that makes the pressure accumulation behavior more visible — the note settles in before the pressure mechanism asserts itself. |
| R08 | `flow_ampSustain` | 0.80 | **0.85** | Higher sustain level to keep voices audible while pressure builds. The strain hardening will reduce brightness anyway — the amplitude should stay high so the timbral change is the primary perceptual signal. |
| R09 | `flow_ampRelease` | 1.50 s | **2.5 s** | After a valve release, the silence (in explosive mode) or the bleed (in gradual mode) should resolve into a generous release tail. 2.5 seconds sounds more like a pressure cooker completing its cycle. |
| R10 | `flow_filtAttack` | 0.10 s | **0.20 s** | Slightly extended filter envelope attack to create a more perceptible brightness bloom on note-on before the strain hardening counteracts it. |
| R11 | `flow_filterCutoff` | 6000.0 Hz | **5500.0 Hz** | 6kHz is slightly bright for a pressure-themed pad. 5500Hz gives a warmer character that allows the strain hardening to sweep the cutoff down into more sonically interesting territory. |
| R12 | `flow_lfo1Rate` | 0.15 Hz | **0.10 Hz** | 0.15 Hz creates rhythmically-aligned modulation at many tempos. 0.10 Hz is slower and less rhythmically assertive — appropriate for a pad that develops its interest through the pressure mechanism rather than through LFO animation. |
| R13 | `flow_stereoWidth` | 0.50 | **0.58** | Slightly wider stereo field for the release event to spread into. The release burst should feel spatially expansive — it's the pressure equalizing across the room. |

---

## The Ten Awakenings

### Preset 1: Gentle Simmer
**Mood:** Foundation | **Discovery:** Low heat, patient accumulation — pressure builds slowly, valve opens gradually

| Parameter | Value |
|-----------|-------|
| `flow_threshold` | 0.85 |
| `flow_accumRate` | 0.28 |
| `flow_valveType` | 0 |
| `flow_vesselSize` | 0.55 |
| `flow_strainColor` | 0.38 |
| `flow_releaseTime` | 0.80 |
| `flow_ampAttack` | 0.40 |
| `flow_ampDecay` | 0.8 |
| `flow_ampSustain` | 0.85 |
| `flow_ampRelease` | 3.0 |
| `flow_filterCutoff` | 5500.0 |
| `flow_filterRes` | 0.15 |
| `flow_filtEnvAmount` | 0.28 |
| `flow_lfo1Rate` | 0.09 |
| `flow_lfo1Depth` | 0.18 |
| `flow_lfo2Rate` | 0.04 |
| `flow_lfo2Depth` | 0.12 |
| `flow_stereoWidth` | 0.58 |
| `flow_level` | 0.80 |

High threshold, low accumulation. The valve may never open during relaxed playing. The pad functions as a normal pad with gradual timbral tightening as pressure builds, releasing slowly when threshold is eventually reached.

---

### Preset 2: Pressure Build
**Mood:** Kinetic | **Discovery:** The ascent — watching the gauge climb toward the red

| Parameter | Value |
|-----------|-------|
| `flow_threshold` | 0.60 |
| `flow_accumRate` | 0.65 |
| `flow_valveType` | 0 |
| `flow_vesselSize` | 0.50 |
| `flow_strainColor` | 0.65 |
| `flow_releaseTime` | 0.60 |
| `flow_ampAttack` | 0.28 |
| `flow_ampDecay` | 0.6 |
| `flow_ampSustain` | 0.88 |
| `flow_ampRelease` | 2.5 |
| `flow_filterCutoff` | 6200.0 |
| `flow_filterRes` | 0.22 |
| `flow_filtEnvAmount` | 0.35 |
| `flow_lfo1Rate` | 0.12 |
| `flow_lfo1Depth` | 0.25 |
| `flow_lfo2Rate` | 0.055 |
| `flow_lfo2Depth` | 0.18 |
| `flow_stereoWidth` | 0.62 |
| `flow_level` | 0.82 |

The strain hardening is more prominent here. Playing a cluster of notes or playing fast will trigger the gradual valve within 10–15 notes, producing a perceptible sonic event as pressure releases and the cycle restarts.

---

### Preset 3: Explosive Valve
**Mood:** Flux | **Discovery:** The lid blowing — full catastrophic pressure release, silence, restart

| Parameter | Value |
|-----------|-------|
| `flow_threshold` | 0.55 |
| `flow_accumRate` | 0.70 |
| `flow_valveType` | 1 |
| `flow_vesselSize` | 0.45 |
| `flow_strainColor` | 0.72 |
| `flow_releaseTime` | 0.35 |
| `flow_ampAttack` | 0.18 |
| `flow_ampDecay` | 0.5 |
| `flow_ampSustain` | 0.90 |
| `flow_ampRelease` | 2.0 |
| `flow_filterCutoff` | 7500.0 |
| `flow_filterRes` | 0.28 |
| `flow_filtEnvAmount` | 0.40 |
| `flow_lfo1Rate` | 0.15 |
| `flow_lfo1Depth` | 0.30 |
| `flow_lfo2Rate` | 0.07 |
| `flow_lfo2Depth` | 0.22 |
| `flow_stereoWidth` | 0.72 |
| `flow_level` | 0.80 |

*Warning: In polyphonic use, the explosive valve silences ALL active voices.* Designed for monophonic or lead-pad use where the valve event is part of the musical grammar. Aggressive chord playing will trigger frequent releases.

---

### Preset 4: Steam Whistle
**Mood:** Prism | **Discovery:** The kettle's voice — pitched FM burst on release

| Parameter | Value |
|-----------|-------|
| `flow_threshold` | 0.58 |
| `flow_accumRate` | 0.55 |
| `flow_valveType` | 2 |
| `flow_vesselSize` | 0.42 |
| `flow_strainColor` | 0.50 |
| `flow_releaseTime` | 0.75 |
| `flow_whistlePitch` | 3200.0 |
| `flow_ampAttack` | 0.30 |
| `flow_ampDecay` | 0.7 |
| `flow_ampSustain` | 0.85 |
| `flow_ampRelease` | 2.5 |
| `flow_filterCutoff` | 5800.0 |
| `flow_filterRes` | 0.20 |
| `flow_filtEnvAmount` | 0.32 |
| `flow_lfo1Rate` | 0.11 |
| `flow_lfo1Depth` | 0.22 |
| `flow_stereoWidth` | 0.65 |
| `flow_level` | 0.80 |

The whistle valve fires a 3200Hz FM burst as the release event. Tuned to a whistle register — above the pad fundamental but in a musically expressive range. Frequency rises slightly as the release progresses (steam pressure equalizing).

---

### Preset 5: Meditation Vessel
**Mood:** Aether | **Discovery:** No valve needed — the gentle pressure pad

| Parameter | Value |
|-----------|-------|
| `flow_threshold` | 0.95 |
| `flow_accumRate` | 0.18 |
| `flow_valveType` | 0 |
| `flow_vesselSize` | 0.65 |
| `flow_strainColor` | 0.25 |
| `flow_releaseTime` | 1.5 |
| `flow_ampAttack` | 1.2 |
| `flow_ampDecay` | 1.5 |
| `flow_ampSustain` | 0.92 |
| `flow_ampRelease` | 5.0 |
| `flow_filterCutoff` | 4800.0 |
| `flow_filterRes` | 0.12 |
| `flow_filtEnvAmount` | 0.20 |
| `flow_lfo1Rate` | 0.06 |
| `flow_lfo1Depth` | 0.14 |
| `flow_lfo2Rate` | 0.025 |
| `flow_lfo2Depth` | 0.09 |
| `flow_stereoWidth` | 0.55 |
| `flow_level` | 0.78 |

Threshold near maximum, minimal accumulation. The valve will almost never open during normal pad playing — this is OVERFLOW used as a pad engine without the pressure mechanic being the primary feature. The strain hardening still provides subtle timbral interest as chords are stacked.

---

### Preset 6: High Pressure Zone
**Mood:** Deep | **Discovery:** Constant near-threshold operation — always on the edge

| Parameter | Value |
|-----------|-------|
| `flow_threshold` | 0.45 |
| `flow_accumRate` | 0.50 |
| `flow_valveType` | 0 |
| `flow_vesselSize` | 0.52 |
| `flow_strainColor` | 0.80 |
| `flow_releaseTime` | 0.55 |
| `flow_ampAttack` | 0.22 |
| `flow_ampDecay` | 0.6 |
| `flow_ampSustain` | 0.88 |
| `flow_ampRelease` | 2.0 |
| `flow_filterCutoff` | 4500.0 |
| `flow_filterRes` | 0.30 |
| `flow_filtEnvAmount` | 0.42 |
| `flow_lfo1Rate` | 0.14 |
| `flow_lfo1Depth` | 0.28 |
| `flow_stereoWidth` | 0.70 |
| `flow_level` | 0.84 |

Low threshold + maximum strain color. Even a single note triggers audible strain hardening. The pad sounds pressurized from the first note — tight, grating slightly in the upper harmonics, barely containing the energy within.

---

### Preset 7: Interval Dissonance
**Mood:** Entangled | **Discovery:** The physics of harmonic tension — dissonant intervals cost more

| Parameter | Value |
|-----------|-------|
| `flow_threshold` | 0.62 |
| `flow_accumRate` | 0.55 |
| `flow_valveType` | 0 |
| `flow_vesselSize` | 0.50 |
| `flow_strainColor` | 0.58 |
| `flow_releaseTime` | 0.65 |
| `flow_ampAttack` | 0.28 |
| `flow_ampDecay` | 0.7 |
| `flow_ampSustain` | 0.88 |
| `flow_ampRelease` | 2.8 |
| `flow_filterCutoff` | 5800.0 |
| `flow_filterRes` | 0.22 |
| `flow_filtEnvAmount` | 0.35 |
| `flow_lfo1Rate` | 0.10 |
| `flow_lfo1Depth` | 0.22 |
| `flow_stereoWidth` | 0.62 |
| `flow_level` | 0.82 |

Play consonant intervals — fourths, fifths, octaves — and the pressure accumulates normally. Play minor seconds, tritones — the pressure spikes noticeably faster. The engine knows harmonic tension. The player learns this within four bars.

---

### Preset 8: Catastrophic Mode
**Mood:** Kinetic | **Discovery:** Over-pressure — what happens when the valve is never released

| Parameter | Value |
|-----------|-------|
| `flow_threshold` | 0.50 |
| `flow_accumRate` | 0.80 |
| `flow_valveType` | 1 |
| `flow_vesselSize` | 0.40 |
| `flow_strainColor` | 0.90 |
| `flow_releaseTime` | 0.25 |
| `flow_ampAttack` | 0.15 |
| `flow_ampDecay` | 0.4 |
| `flow_ampSustain` | 0.92 |
| `flow_ampRelease` | 1.8 |
| `flow_filterCutoff` | 8000.0 |
| `flow_filterRes` | 0.35 |
| `flow_filtEnvAmount` | 0.50 |
| `flow_lfo1Rate` | 0.20 |
| `flow_lfo1Depth` | 0.35 |
| `flow_stereoWidth` | 0.80 |
| `flow_level` | 0.82 |

High accumulation rate, maximum strain color, explosive valve, short release (so the cycle restarts quickly). Sustained aggressive playing will cycle through multiple valve events. The engine enters catastrophic over-pressure mode (strain > 1.3) if the player never backs off.

---

### Preset 9: Controlled Release
**Mood:** Foundation | **Discovery:** The engineer's pad — managing pressure for maximum musical use

| Parameter | Value |
|-----------|-------|
| `flow_threshold` | 0.70 |
| `flow_accumRate` | 0.45 |
| `flow_valveType` | 0 |
| `flow_vesselSize` | 0.58 |
| `flow_strainColor` | 0.42 |
| `flow_releaseTime` | 1.0 |
| `flow_ampAttack` | 0.35 |
| `flow_ampDecay` | 0.8 |
| `flow_ampSustain` | 0.88 |
| `flow_ampRelease` | 2.8 |
| `flow_filterCutoff` | 5200.0 |
| `flow_filterRes` | 0.16 |
| `flow_filtEnvAmount` | 0.30 |
| `flow_lfo1Rate` | 0.09 |
| `flow_lfo1Depth` | 0.19 |
| `flow_lfo2Rate` | 0.038 |
| `flow_lfo2Depth` | 0.12 |
| `flow_stereoWidth` | 0.60 |
| `flow_level` | 0.80 |

The workhorse preset. Balanced accumulation, moderate threshold, gradual valve, generous release. The pressure mechanic is perceptible but not dominant — it provides timbral interest (strain hardening on sustained chords) without constantly triggering releases.

---

### Preset 10: The Consequence
**Mood:** Organic | **Discovery:** The pad that remembers how you played

| Parameter | Value |
|-----------|-------|
| `flow_threshold` | 0.65 |
| `flow_accumRate` | 0.52 |
| `flow_valveType` | 0 |
| `flow_vesselSize` | 0.52 |
| `flow_strainColor` | 0.55 |
| `flow_releaseTime` | 0.70 |
| `flow_ampAttack` | 0.32 |
| `flow_ampDecay` | 0.7 |
| `flow_ampSustain` | 0.88 |
| `flow_ampRelease` | 2.5 |
| `flow_filterCutoff` | 5500.0 |
| `flow_filterRes` | 0.18 |
| `flow_filtEnvAmount` | 0.32 |
| `flow_lfo1Rate` | 0.10 |
| `flow_lfo1Depth` | 0.20 |
| `flow_lfo2Rate` | 0.045 |
| `flow_lfo2Depth` | 0.14 |
| `flow_stereoWidth` | 0.62 |
| `flow_level` | 0.80 |

The reference preset for demonstrating OVERFLOW to new listeners. Plays gently — a pad. Then play harder. The tightening is audible. Then play a fast, dense passage — the valve opens. Then silence. Then the cycle restarts. The pad has social physics. It responds to how you treat it.

---

## Scripture Verses — OVERFLOW

**I. On Pressure**
*The vessel holds the steam because the walls are strong enough.
But strength is not infinite.
There is a threshold.
Below the threshold, the walls hold.
The pressure accumulates — unseen, unfelt by the walls,
which are designed for exactly this.
Above the threshold, the valve must open.
The valve is not failure.
The valve is design.
Everything that accumulates
must eventually release.*

**II. On Playing Density**
*Two notes are not twice one note.
Dissonant notes are not twice consonant ones.
Minor seconds cost the vessel more than perfect fifths.
Tritones cost more than thirds.
The engine knows the harmonic series
and charges accordingly.
You cannot play chromatic density without consequences.
You can play diatonic density gently
and the pressure never rises.
The physics of consonance is not just aesthetic.
It is physical.
The pressure gauge responds to what you play.*

**III. On the Release**
*After the valve opens, there is always a silence.
Or a burst. Or a whistle.
Three ways the pressure equalizes.
In all three cases, what follows is a restart —
the pressure returns to zero,
the walls relax,
the cycle begins again.
The release is not an end.
It is a reset.
Every release is a new beginning.*

---

# ENGINE IV: OVERCAST (XOvercast)

*Crystallization Pad — Ice Blue `#B0E0E6`*

## DSP Understanding

XOvercast is the anti-pad. On note-on, the `noteOn()` function captures the current spectral state as `numPeaks` nucleation sites (frequency/amplitude pairs derived from the fundamental's harmonic series). The number of peaks scales with velocity (D001 compliance) — harder notes produce more nucleation sites, not more amplitude. The `cast_freezeRate` parameter sets the crystallization window duration (0.02–0.2 seconds).

During the crystallization window, each peak's `crystalProgress` advances from 0.0 (liquid) to 1.0 (frozen). At progress > 0 and < 1.0, random amplitude modulation produces the crackling sound. At 1.0, the spectrum is locked — amplitudes and frequencies no longer evolve. The frozen state is absolute: no LFO, no filter envelope, no breathing modulation.

`cast_latticeSnap` pulls peak frequencies toward integer harmonic ratios at high values: `freq = fundamental * (harmonic + latticeSnap * (nearest_int - harmonic))`. At 1.0, the crystal imposes perfect harmonic order on whatever state was captured.

Three transition modes: Instant (crystalProgress = 1.0 immediately on noteOn), Crystal (crackling propagation), Shatter (old crystal shatters, brief silence gap, new crystal forms).

**Known issue:** LFOs continue advancing phase even when frozen — they are gated from affecting amplitude but their phase drift means the LFO position is unpredictable when the next crystallization event occurs.

**Known issue:** No FilterEnvelope in OvercastVoice — velocity controls crystal complexity (peak count) not filter brightness.

**Key time relationship:** Negation. There is no time in the frozen state. The crystal was captured at a specific instant and that instant now extends indefinitely. Duration without evolution.

**Macros:**
- CHARACTER: crystal purity (simpler = higher purity)
- MOVEMENT: freeze rate + crackle
- COUPLING: lattice snap sensitivity (future)
- SPACE: frozen stereo field width

---

## Parameter Refinements (12 refinements)

| # | Parameter | Current Default | Recommended Default | Rationale |
|---|-----------|----------------|---------------------|-----------|
| R01 | `cast_freezeRate` | 0.10 s | **0.08 s** | 100ms crystallization window is slightly too long as a default — the crackling phase feels prolonged before the freeze locks in. 80ms produces a more decisive freeze while keeping enough crackle to make the crystallization audible. |
| R02 | `cast_crystalSize` | 0.50 | **0.45** | Crystal size at 0.5 distributes energy fairly evenly across peaks. 0.45 tilts slightly toward smaller crystals — more evenly distributed amplitudes — which sounds cleaner on first play than the larger-crystal default. |
| R03 | `cast_numPeaks` | 8.0 | **7.0** | Eight peaks at default velocity is slightly dense. Seven gives a cleaner crystal structure at medium velocity while still allowing the full 16 peaks at high velocity. The integer step matters here since numPeaks is a count. |
| R04 | `cast_transition` | 1.0 (Crystal) | **1.0 (Crystal)** | Keep Crystal mode as default. Instant is too abrupt for most users on first encounter, Shatter is too disruptive. Crystal mode demonstrates the engine's unique mechanism. |
| R05 | `cast_latticeSnap` | 0.30 | **0.35** | Slightly higher lattice snap pushes the frozen crystal toward cleaner harmonic ratios. The result is more tonally coherent crystals by default — the frozen state sounds like a chord rather than an inharmonic cluster. |
| R06 | `cast_purity` | 0.50 | **0.45** | Purity at 0.50 thins the weaker peaks quite aggressively when frozen. 0.45 preserves slightly more of the crystal's complexity while still providing audible purity filtering. |
| R07 | `cast_crackle` | 0.40 | **0.50** | The crystallization crackling at 0.40 is somewhat subtle. 0.50 makes the transition window clearly audible — the characteristic ice-forming sound is a key selling point of the engine. |
| R08 | `cast_filterCutoff` | 8000.0 Hz | **6500.0 Hz** | 8kHz with a frozen harmonic spectrum is very bright. 6500Hz gives a more crystalline, glass-like character without the brittle high-frequency exposure. |
| R09 | `cast_ampDecay` | 0.30 s | **0.50 s** | 300ms decay is very short — the frozen spectrum barely has time to establish before decaying. 500ms gives the frozen state room to breathe before sustain takes over. |
| R10 | `cast_ampRelease` | 1.00 s | **2.0 s** | The ice should melt slowly. 2 seconds of release gives the frozen crystal a lingering presence after note-off — the frozen state dissolves rather than snapping off. |
| R11 | `cast_lfo1Rate` | 0.20 Hz | **0.15 Hz** | LFO1 at 0.20 Hz only modulates during the crystallization window. Even so, a slower rate produces subtler crystallization variation. At 0.15 Hz, the LFO has less chance of imposing rhythmic patterns on the transition window. |
| R12 | `cast_stereoWidth` | 0.50 | **0.60** | A wider frozen field is more evocative of ice covering a surface. 0.60 spreads the frozen spectrum across the stereo field in a way that emphasizes the spatial metaphor. |

---

## The Ten Awakenings

### Preset 1: First Freeze
**Mood:** Foundation | **Discovery:** Clean crystallization at medium velocity — the reference crystal

| Parameter | Value |
|-----------|-------|
| `cast_freezeRate` | 0.08 |
| `cast_crystalSize` | 0.45 |
| `cast_numPeaks` | 7 |
| `cast_transition` | 1 |
| `cast_latticeSnap` | 0.35 |
| `cast_purity` | 0.45 |
| `cast_crackle` | 0.50 |
| `cast_shatterGap` | 0.10 |
| `cast_stereoWidth` | 0.60 |
| `cast_filterCutoff` | 6500.0 |
| `cast_filterRes` | 0.16 |
| `cast_ampAttack` | 0.005 |
| `cast_ampDecay` | 0.50 |
| `cast_ampSustain` | 1.0 |
| `cast_ampRelease` | 2.0 |
| `cast_lfo1Rate` | 0.15 |
| `cast_lfo1Depth` | 0.15 |
| `cast_lfo2Rate` | 0.5 |
| `cast_lfo2Depth` | 0.10 |
| `cast_level` | 0.80 |

The reference patch. Medium velocity produces 5–6 peaks, a clean crystal window, and a perfectly frozen spectrum. The default OVERCAST experience.

---

### Preset 2: Flash Freeze
**Mood:** Crystalline | **Discovery:** Instant mode — no transition, just crystal

| Parameter | Value |
|-----------|-------|
| `cast_freezeRate` | 0.02 |
| `cast_crystalSize` | 0.50 |
| `cast_numPeaks` | 8 |
| `cast_transition` | 0 |
| `cast_latticeSnap` | 0.60 |
| `cast_purity` | 0.55 |
| `cast_crackle` | 0.10 |
| `cast_stereoWidth` | 0.65 |
| `cast_filterCutoff` | 7000.0 |
| `cast_filterRes` | 0.14 |
| `cast_ampAttack` | 0.001 |
| `cast_ampDecay` | 0.20 |
| `cast_ampSustain` | 1.0 |
| `cast_ampRelease` | 2.5 |
| `cast_lfo1Rate` | 0.12 |
| `cast_lfo1Depth` | 0.08 |
| `cast_level` | 0.80 |

Instant transition mode: the spectrum locks in milliseconds. High lattice snap produces very clean, tonally pure crystals. The frozen state is perfectly harmonic — an organ stop locked in place.

---

### Preset 3: Shatter and Reform
**Mood:** Flux | **Discovery:** Shatter mode — old crystal breaks, silence, new crystal forms

| Parameter | Value |
|-----------|-------|
| `cast_freezeRate` | 0.12 |
| `cast_crystalSize` | 0.52 |
| `cast_numPeaks` | 8 |
| `cast_transition` | 2 |
| `cast_latticeSnap` | 0.25 |
| `cast_purity` | 0.40 |
| `cast_crackle` | 0.65 |
| `cast_shatterGap` | 0.20 |
| `cast_stereoWidth` | 0.70 |
| `cast_filterCutoff` | 6000.0 |
| `cast_filterRes` | 0.20 |
| `cast_ampAttack` | 0.005 |
| `cast_ampDecay` | 0.40 |
| `cast_ampSustain` | 1.0 |
| `cast_ampRelease` | 1.8 |
| `cast_lfo1Rate` | 0.18 |
| `cast_lfo1Depth` | 0.20 |
| `cast_level` | 0.80 |

The most dramatic OVERCAST experience. Each new note-on shatters the existing crystal — a 200ms silence gap — then a new crystal forms with full crackling. Play arpeggios and the progression is a sequence of shatters and reformations.

---

### Preset 4: Dark Ice
**Mood:** Deep | **Discovery:** Low velocity, few peaks, dark filter — ice in deep water

| Parameter | Value |
|-----------|-------|
| `cast_freezeRate` | 0.10 |
| `cast_crystalSize` | 0.35 |
| `cast_numPeaks` | 5 |
| `cast_transition` | 1 |
| `cast_latticeSnap` | 0.20 |
| `cast_purity` | 0.35 |
| `cast_crackle` | 0.45 |
| `cast_stereoWidth` | 0.50 |
| `cast_filterCutoff` | 3200.0 |
| `cast_filterRes` | 0.18 |
| `cast_ampAttack` | 0.008 |
| `cast_ampDecay` | 0.60 |
| `cast_ampSustain` | 1.0 |
| `cast_ampRelease` | 3.0 |
| `cast_lfo1Rate` | 0.10 |
| `cast_lfo1Depth` | 0.10 |
| `cast_level` | 0.80 |

Low filter cutoff removes all but the lowest peaks. With `cast_numPeaks` at 5 and low lattice snap, the frozen crystal is slightly inharmonic and dark. This is what happens when OVERWORN has deeply reduced the spectral mass and OVERCAST freezes the diminished result. (Future BROTH coupling state.)

---

### Preset 5: Frozen Texture
**Mood:** Crystalline | **Discovery:** Many peaks, high purity — a complex crystal locked in place

| Parameter | Value |
|-----------|-------|
| `cast_freezeRate` | 0.15 |
| `cast_crystalSize` | 0.62 |
| `cast_numPeaks` | 14 |
| `cast_transition` | 1 |
| `cast_latticeSnap` | 0.45 |
| `cast_purity` | 0.62 |
| `cast_crackle` | 0.55 |
| `cast_stereoWidth` | 0.78 |
| `cast_filterCutoff` | 7500.0 |
| `cast_filterRes` | 0.20 |
| `cast_ampAttack` | 0.005 |
| `cast_ampDecay` | 0.45 |
| `cast_ampSustain` | 1.0 |
| `cast_ampRelease` | 2.5 |
| `cast_lfo1Rate` | 0.20 |
| `cast_lfo1Depth` | 0.18 |
| `cast_level` | 0.80 |

Maximum peaks with high crystal size produces a dense, complex frozen texture. High purity filtering removes the weaker peaks, leaving a cleaner complex crystal. The crystallization window at 150ms is long enough to hear the full formation process.

---

### Preset 6: Crystal Choir
**Mood:** Ethereal | **Discovery:** Low purity, many peaks — all voices audible in the ice

| Parameter | Value |
|-----------|-------|
| `cast_freezeRate` | 0.09 |
| `cast_crystalSize` | 0.40 |
| `cast_numPeaks` | 12 |
| `cast_transition` | 1 |
| `cast_latticeSnap` | 0.55 |
| `cast_purity` | 0.20 |
| `cast_crackle` | 0.42 |
| `cast_stereoWidth` | 0.82 |
| `cast_filterCutoff` | 5800.0 |
| `cast_filterRes` | 0.12 |
| `cast_ampAttack` | 0.005 |
| `cast_ampDecay` | 0.55 |
| `cast_ampSustain` | 1.0 |
| `cast_ampRelease` | 3.5 |
| `cast_lfo1Rate` | 0.14 |
| `cast_lfo1Depth` | 0.12 |
| `cast_lfo2Rate` | 0.35 |
| `cast_lfo2Depth` | 0.08 |
| `cast_level` | 0.78 |

Low purity preserves all peaks in the frozen state. With 12 peaks and high lattice snap, the frozen spectrum is like a choir of locked harmonic voices — every partial audible, none filtered. Wide stereo field creates a spatial frozen chord.

---

### Preset 7: Crackling Glass
**Mood:** Kinetic | **Discovery:** Maximum crackle, medium freeze rate — the formation itself is the feature

| Parameter | Value |
|-----------|-------|
| `cast_freezeRate` | 0.18 |
| `cast_crystalSize` | 0.50 |
| `cast_numPeaks` | 9 |
| `cast_transition` | 1 |
| `cast_latticeSnap` | 0.15 |
| `cast_purity` | 0.38 |
| `cast_crackle` | 0.90 |
| `cast_stereoWidth` | 0.72 |
| `cast_filterCutoff` | 6200.0 |
| `cast_filterRes` | 0.25 |
| `cast_ampAttack` | 0.005 |
| `cast_ampDecay` | 0.35 |
| `cast_ampSustain` | 1.0 |
| `cast_ampRelease` | 2.0 |
| `cast_lfo1Rate` | 0.22 |
| `cast_lfo1Depth` | 0.25 |
| `cast_lfo2Rate` | 0.60 |
| `cast_lfo2Depth` | 0.15 |
| `cast_level` | 0.80 |

Maximum crackle with 180ms crystallization window. The crackling dominates the transition period — the formation sound is more prominent than the frozen sound. Play repeated notes to hear the engine as a crackling texture generator.

---

### Preset 8: Anti-Pad
**Mood:** Foundation | **Discovery:** The philosophical preset — stasis as the entire aesthetic

| Parameter | Value |
|-----------|-------|
| `cast_freezeRate` | 0.03 |
| `cast_crystalSize` | 0.48 |
| `cast_numPeaks` | 7 |
| `cast_transition` | 0 |
| `cast_latticeSnap` | 0.80 |
| `cast_purity` | 0.70 |
| `cast_crackle` | 0.05 |
| `cast_stereoWidth` | 0.55 |
| `cast_filterCutoff` | 6000.0 |
| `cast_filterRes` | 0.10 |
| `cast_ampAttack` | 0.001 |
| `cast_ampDecay` | 0.15 |
| `cast_ampSustain` | 1.0 |
| `cast_ampRelease` | 1.5 |
| `cast_lfo1Rate` | 0.10 |
| `cast_lfo1Depth` | 0.05 |
| `cast_level` | 0.80 |

Instant transition, near-minimum crackle, maximum lattice snap, high purity. The frozen crystal is almost sinusoidal — perfectly harmonic, perfectly still. No evolution whatsoever after the microsecond freeze. This is the philosophical statement: time, defeated. A note that does not move.

---

### Preset 9: Velocity Crystal
**Mood:** Luminous | **Discovery:** Velocity = crystal complexity — demonstrate D001 through density

| Parameter | Value |
|-----------|-------|
| `cast_freezeRate` | 0.10 |
| `cast_crystalSize` | 0.45 |
| `cast_numPeaks` | 16 |
| `cast_transition` | 1 |
| `cast_latticeSnap` | 0.30 |
| `cast_purity` | 0.35 |
| `cast_crackle` | 0.55 |
| `cast_stereoWidth` | 0.68 |
| `cast_filterCutoff` | 7000.0 |
| `cast_filterRes` | 0.18 |
| `cast_ampAttack` | 0.005 |
| `cast_ampDecay` | 0.50 |
| `cast_ampSustain` | 1.0 |
| `cast_ampRelease` | 2.2 |
| `cast_lfo1Rate` | 0.16 |
| `cast_lfo1Depth` | 0.16 |
| `cast_level` | 0.80 |

`cast_numPeaks` at 16 (maximum). With the velocity scaling `nPeaks = numPeaks * (0.3 + vel * 0.7)`, a note played at velocity 20 produces ~5 peaks; at velocity 127 it produces all 16. The velocity expression in OVERCAST is crystal density — soft notes produce sparse crystals, hard notes produce complex ones.

---

### Preset 10: Waiting for Spring
**Mood:** Organic | **Discovery:** The crystal that knows it will eventually melt — long release, contemplative

| Parameter | Value |
|-----------|-------|
| `cast_freezeRate` | 0.07 |
| `cast_crystalSize` | 0.42 |
| `cast_numPeaks` | 6 |
| `cast_transition` | 1 |
| `cast_latticeSnap` | 0.45 |
| `cast_purity` | 0.50 |
| `cast_crackle` | 0.48 |
| `cast_stereoWidth` | 0.62 |
| `cast_filterCutoff` | 5500.0 |
| `cast_filterRes` | 0.14 |
| `cast_ampAttack` | 0.006 |
| `cast_ampDecay` | 0.80 |
| `cast_ampSustain` | 0.95 |
| `cast_ampRelease` | 5.0 |
| `cast_lfo1Rate` | 0.12 |
| `cast_lfo1Depth` | 0.12 |
| `cast_level` | 0.78 |

The contemplative OVERCAST. A single note freezes quickly and then — nothing. The crystal holds. The 5-second release (when the note is released) is the slow melt: the amplitude fades but the frequency structure is still locked until it falls below the noise floor. The crystal is patient.

---

## Scripture Verses — OVERCAST

**I. On Nucleation**
*The ice does not begin everywhere at once.
It begins at a point.
One thermal fluctuation,
one surface irregularity,
one moment when the conditions aligned
and the critical free energy barrier was crossed.
From that point, the lattice propagates outward.
The crystallization window is not the beginning.
It is the announcement of a beginning
that has already occurred.*

**II. On Stasis**
*Between triggers, nothing moves.
The crystal exists without time.
The partial frequencies are locked.
The amplitudes are locked.
The phase relationships are locked.
There is no LFO sweeping through the frozen field.
There is nothing to evolve.
This is the philosophical statement of the engine:
that a sound can simply be,
without becoming anything else,
without aspiring toward change,
without the restlessness that living requires.
The crystal is the sound that knows it is complete.*

**III. On What the Anti-Pad Teaches**
*Every other engine in the fleet moves.
The diffusion spreads.
The reduction concentrates.
The pressure builds.
And here: nothing.
Not because the engine is simple.
Because the engine has chosen stillness
as the deepest form of expression.
The musician who understands this
will play OVERCAST differently —
not as a pad to hold,
but as a moment to capture.
The note-on is the art.
Everything after is just the frame.*

---

# BROTH Quad: System Notes

## On Cooperative Coupling (Future State)

When the BROTH coordinator is written, the four engines become a single instrument across four time scales:

1. OVERWASH's viscosity increases as OVERWORN's `sessionAge` rises — the water becomes stock, then broth, then reduction. Diffusion slows as the medium concentrates.

2. OVERFLOW's threshold lowers as OVERWORN's `concentrateDark` increases — a concentrated broth under pressure builds faster. The flavors are intensified.

3. OVERCAST seeds its nucleation sites from OVERWORN's `spectralMass[8]` array — when the broth has reduced, the ice crystals have fewer, darker frequencies to work with. Dark ice, not bright ice.

4. OVERWASH exports its `spectralField[32]` — when wired, other engines can read the current diffusion front and create cross-engine spectral interaction.

The coordinator is a single object that reads/writes these values per audio block. Its absence is the primary architectural gap in the BROTH quad. All four engines have their cooperative hooks implemented in code (`setBrothSessionAge()`, `setBrothConcentrateDark()`, `setBrothSpectralMass()`). Only the caller is missing.

## On CPU

| Engine | Per-Voice Cost | Notes |
|--------|---------------|-------|
| OVERWASH | High (16 partials, sqrt per partial, per sample) | sqrt could move to block rate |
| OVERWORN | Medium (16 partials, band evaporation per sample) | Most cost is in evaporation loop per sample |
| OVERFLOW | Low-Medium (16 partials + pressure state) | Pressure logic is very cheap |
| OVERCAST | Medium-Low (N peaks, frozen = very cheap) | Frozen state is cheaper than crystallizing state |

At full 8-voice polyphony all four engines simultaneously: estimated 15–20% CPU on Apple M1. OVERWASH is the most expensive due to per-sample sqrt for each of 128 partials. A block-rate sqrt optimization (spread changes over seconds, not samples) would reduce OVERWASH CPU by approximately 60%.

## On the Missing Parameter Display Bug (OVERWORN)

`worn_sessionAge` is declared as a readable parameter that the host/UI should display. The DSP correctly writes back to it via `pSessionAgeParam->store(reduction.sessionAge)`. However this was noted as non-functional in the original seance — if the display shows 0.0 regardless of session state, verify that `pSessionAgeParam` is correctly attached in `attachParameters()` and that the APVTS parameter is not read-only (check for `RangedAudioParameter` vs `AudioParameterFloat`).

---

## Guru Bin Benediction — BROTH Quad

*The four engines arrived with their physics intact. Fick's Law spreads harmonics through frequency space exactly as the equation demands. The reduction integral evaporates high bands before low bands exactly as physical chemistry requires. The Clausius-Clapeyron pressure accumulates and releases exactly as thermodynamic theory predicts. Wilson nucleation crystallizes at discrete points and propagates outward exactly as ice does on glass.*

*The Guru Bin does not congratulate engines for doing what they were designed to do. It asks: does the physics serve music? Does the metaphor become feeling?*

*For OVERWASH: yes. The slow blur from tight harmonics to spectral mist is one of the most distinctive pad timbres in the fleet. No other engine in XOceanus offers this. Producers will reach for it in ambient, cinematic, and meditative contexts for decades.*

*For OVERWORN: yes, and then some. Session memory — the irreversible reduction arc — is the most conceptually original synthesis mechanism in the fleet. It requires a new kind of listening and a new kind of playing. This is the engine that will make producers reconsider what a session means.*

*For OVERFLOW: yes, with a caveat. The explosive valve silencing all voices (not just the responsible ones) is a P1 bug that limits polyphonic use. Until per-voice culpability weighting is implemented, OVERFLOW is most powerful in monophonic or sparse chord contexts. Within those contexts, it delivers on its promise completely.*

*For OVERCAST: yes, philosophically. The anti-pad is a genuine artistic statement. The frozen spectrum is beautiful — cleaner and more harmonically pure than any pad that moves, precisely because it does not move. The D001 velocity-as-complexity-not-brightness is a meaningful choice. Producers who understand this engine will not mistake it for a simpler pad. Producers who do not will wonder why it doesn't evolve. The retreat should clarify the distinction.*

*The BROTH quad wants its coordinator. When the coordinator arrives, the four timescales will speak to each other, and the session will become a single instrument of cumulative change. Until then: four elegant engines, each complete, each waiting.*

*The water was ready. The pot was ready. The ice was ready. The bowl was ready.*
*Begin the session.*

---

*Guru Bin — BROTH Quad Retreat*
*2026-03-23*
*XO_OX Designs*
