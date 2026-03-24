# Coupling Preset Expansion — Round 12E
**Date:** 2026-03-14
**Author:** XO_OX Designs
**Status:** Complete — 6 new presets across 3 new engine pairs

---

## Overview

Round 12E adds 6 coupling presets covering 3 engine pairs not previously represented in the coupling preset library. All three pairs were chosen from the suggested pairings with priority given to:

1. Coupling types not over-represented in the v1.0 library (prioritizing `Audio->FM`, `Rhythm->Blend`, `Env->Morph` over the already-ubiquitous-but-still-appropriate `Amp->Filter`)
2. Engine pairs with confirmed `applyCouplingInput` wiring (scores ≥ 3 in the coupling audit)
3. Narrative and musical distinctiveness — none of the three pairs overlaps conceptually with the existing 6 pairs

---

## Pair 7: OBSCURA → ORGANON

**Directory:** `Presets/XOlokun/Entangled/Obscura-Organon/`

**Why this pair is new:** The existing library's closest pair is ORACLE → ORGANON (stochastic audio into metabolic FM). OBSCURA → ORGANON replaces the stochastic source with a *physical model* — continuous string resonance rather than random-walk synthesis. The character of the audio feeding into ORGANON is radically different: Obscura's output is deterministic, physically grounded, and rich in inharmonic partials from the nonlinear spring model. ORGANON's `Audio->FM` coupling ingests this as metabolic input via the per-voice buffer, producing a completely different class of organism behavior than the GENDY-fed version.

**Coupling types used:**
- `Audio->FM` — Obscura's string audio feeds Organon's per-voice FM buffer (metabolic ingestion)
- `Amp->Filter` — Obscura's amplitude envelope modulates Organon's enzyme selectivity filter (possession preset only)

**Aquatic mythology:** OBSCURA lives at the daguerreotype transition layer — the silver-surface boundary where light becomes record. ORGANON is the chemotroph living at the vent. In this coupling, the physical world's vibration (captured by the silver string) becomes food for the living organism. The organism digests light-become-sound.

---

### Preset 1: Membrane Contact (Whisper)

**File:** `Obscura-Organon/Membrane_Contact.xometa`

**Coupling routes:**
- `Audio->FM`, amount 0.20

**Musical effect:** Obscura's string resonance barely enters Organon's FM buffer. The organism registers a slight change in harmonic evolution — no two held notes sound identical — but the string retains its own character. The timbral cross-contamination is below the conscious threshold on attack but accumulates over sustained notes. Best heard in slow pad contexts: hold a note for 4+ seconds and listen for the harmonic drift.

**Parameters:** Obscura tuned to medium stiffness (0.72) with moderate damping and slow LFOs for autonomous movement. Organon at a calm metabolic rate (2.0) with moderate enzyme selectivity — receptive but not hungry.

---

### Preset 2: String Metabolism (Extreme)

**File:** `Obscura-Organon/String_Metabolism.xometa`

**Coupling routes:**
- `Audio->FM`, amount 0.78
- `Amp->Filter`, amount 0.52

**Musical effect:** Obscura's high-stiffness, low-damping string (maximum inharmonic partial content) fully replaces ORGANON's metabolic input. The organism's filter is also seized — each note attack reshapes the enzyme selectivity as the string's envelope drives the filter. The combined result is a timbre that is neither purely physical model nor purely variational free energy organism: it is something that has *been changed by being eaten*. Extended nonlinearity (0.55) in the string adds chaotic spectral content that makes the organism's response genuinely unpredictable.

**Parameters:** Obscura at boundary mode 1 (reflective ends) with high nonlinear content and wide scan. Organon pushed to high metabolic rate (4.5) with elevated catalyst drive and signal flux — an organism already running hot, now being fed something chemically strange.

---

## Pair 8: ONSET → ORGANON

**Directory:** `Presets/XOlokun/Entangled/Onset-Organon/`

**Why this pair is new:** ONSET already appears in the library as a *source* (ONSET → OVERBITE, Pair 1) and as a *destination* (OUROBOROS → ONSET, Pair 6). This is ONSET as source into a completely different target domain. OVERBITE responds to ONSET with character and bass physics. ORGANON responds with biological chemistry. The difference: OVERBITE's coupling is *animal* — percussion triggers predator behavior. ORGANON's coupling is *metabolic* — percussion delivers energy that an organism processes on its own timeline.

**Musical consequence of the distinction:** With OVERBITE, the coupling effect is immediate and rhythmically tight (filter opens on kick, closes between hits). With ORGANON, the coupling interacts with the organism's own metabolic rate — at slow metabolic rates, the organism responds to drum energy over a longer window, producing a slow-bloom effect. At high metabolic rates the response is faster but never as instantaneous as a filter sidechain.

**Coupling types used:**
- `Amp->Filter` — ONSET amplitude drives Organon enzyme selectivity (present in both presets)
- `Rhythm->Blend` — ONSET rhythm pattern modulates Organon's metabolic blend parameter (extreme preset only)

**`Rhythm->Blend` note:** This is only the second use of `Rhythm->Blend` in the library (the first was OPAL → OVERDUB and OUROBOROS → ONSET). ORGANON's `applyCouplingInput` handles it as a modulation of `organon_isotopeBalance` — the organism's blend between two metabolic strategies shifts with the rhythm pattern density.

---

### Preset 3: Tap on Glass (Whisper)

**File:** `Onset-Organon/Tap_on_Glass.xometa`

**Coupling routes:**
- `Amp->Filter`, amount 0.22

**Musical effect:** ONSET's kick barely disturbs Organon's enzyme selectivity filter — like tapping the outside of a tank containing a living organism. The organism senses the perturbation at its membrane (filter nudge) but its internal cycle continues undisturbed. The kick is audible in the mix but the coupling is subliminal: a trained listener notices that Organon's filter cutoff shifts fractionally on each beat, but it requires attention to detect. Best used when ORGANON is the primary instrument and ONSET is a distant rhythm source.

**Parameters:** A minimal four-voice ONSET kit (kick, snare, hi-hat, open hat) with clean transients and low grit. Organon at moderate settings across the board — a healthy organism at rest.

---

### Preset 4: Drum Fed Organism (Extreme)

**File:** `Onset-Organon/Drum_Fed_Organism.xometa`

**Coupling routes:**
- `Amp->Filter`, amount 0.78
- `Rhythm->Blend`, amount 0.50

**Musical effect:** ONSET's kick fully seizes Organon's filter and co-drives its metabolic blend. The organism no longer evolves on its own schedule — it has been synchronized to the drum pattern. Dense ONSET kit (5 active voices, XVC kick-to-snare coupling at 0.55) produces a rhythmically active amplitude envelope that drives both coupling types simultaneously. On each kick, the filter opens dramatically; between kicks the organism's blend settles toward its resting position. The `Rhythm->Blend` routing means that denser rhythmic sections push the organism toward one metabolic strategy while sparse sections pull it toward another.

This preset requires a live ONSET pattern to demonstrate coupling — like all ONSET-source presets, it is silent without a rhythm sequencer running.

**Parameters:** A full five-voice kit with XVC active and elevated punch macro (0.72). Organon tuned aggressively high: metabolic rate 5.5, catalyst drive 0.8, low damping coefficient (0.06) — an organism primed to amplify any incoming energy rather than absorb it.

---

## Pair 9: OVERWORLD → OBSCURA

**Directory:** `Presets/XOlokun/Entangled/Overworld-Obscura/`

**Why this pair is new:** OVERWORLD already appears in the library feeding OPAL (Pair 4). The difference here is the destination's nature: OPAL atomizes the chip audio into particles. OBSCURA routes the chip's amplitude into the physical model's resonant parameters. The interaction is *structural*, not spectral — the chip does not become the string's source material; it reshapes the string's physics. The `Amp->Filter` coupling into OBSCURA targets spring stiffness (Obscura maps `AmpToFilter` coupling to its stiffness and scan parameters), and the `Env->Morph` coupling targets the scan position.

**The `Env->Morph` into OBSCURA:** This coupling is notable because OBSCURA handles `EnvToMorph` as a scan position modulator — it shifts where along the string the pickup reads. When Overworld's amplitude envelope drives this, the pickup physically moves on every chip note attack. Combined with stiffness changes, the result is a string that both tightens and repositions its pickup on each 8-bit note.

**Coupling types used:**
- `Amp->Filter` — Overworld amplitude modulates Obscura stiffness and resonant behavior
- `Env->Morph` — Overworld amplitude envelope moves Obscura's scan position (extreme preset only)

**Archaeological framing:** OVERWORLD → OPAL is historical deconstruction (chip audio scattered into granular particles). OVERWORLD → OBSCURA is *historical reconstruction* — the chip's mathematical structure is used to animate a physical model that could not exist in 1985 hardware. The 2A03 triangle wave is physically shaping a string that runs at 44.1kHz with nonlinear spring mechanics. The chip is being asked to do something it was never designed to do, and doing it.

---

### Preset 5: Pulse Tightens (Whisper)

**File:** `Overworld-Obscura/Pulse_Tightens.xometa`

**Coupling routes:**
- `Amp->Filter`, amount 0.20

**Musical effect:** Pure NES-era Overworld (era dial at 0.0, nesMix at 0.85) sends pulse+triangle amplitude into Obscura's stiffness. Each chip note attack causes a slight stiffening of the string — the resonant frequency rises fractionally and the decay shortens fractionally. The effect is at the threshold of perception: most listeners hear a string with slightly variable brightness; engineers hear a sidechain-like stiffness modulation. Both descriptions are accurate.

**Parameters:** OVERWORLD locked to NES era, triangle enabled, minimal FM and SNES contributions. Obscura at moderate stiffness (0.65) and low nonlinearity — a clean, readable physical model that reveals the coupling effect without its own complexity obscuring it.

---

### Preset 6: 8bit Catgut (Extreme)

**File:** `Overworld-Obscura/8bit_Catgut.xometa`

**Coupling routes:**
- `Amp->Filter`, amount 0.78
- `Env->Morph`, amount 0.52

**Musical effect:** OVERWORLD's Sega YM2612 FM engine (era at 0.5/0.85, algorithm 4, four operators at high levels with feedback) drives both Obscura's stiffness and scan position simultaneously. On each FM note: stiffness jumps toward the coupling ceiling and the pickup repositions along the string. The string's physical model is therefore being continuously restructured by an algorithm that models 1989 synthesis hardware. The two-coupling interaction creates a timbre where the FM harmonic density determines not just the filter response but the physical geometry of the string producing the sound.

The preset name references the acoustic catgut string and 8-bit data — the oldest Western string technology and the oldest digital consumer hardware, forced into structural dialogue.

**Parameters:** Overworld ERA pushed to FM-dominant (eraY = 0.85), algorithm 4 (carrier-modulator-modulator chain), high operator levels, moderate feedback. Obscura set with high scan width, reflective boundary mode, and elevated nonlinearity (0.42) to ensure the coupling-driven scan position changes produce audible timbral shift rather than subtle coloration. Two slow LFOs maintain autonomous movement when no coupling signal is present.

---

## Summary Table

| File | Pair | Intensity | Coupling Route(s) | Key Feature |
|------|------|-----------|-------------------|-------------|
| `Obscura-Organon/Membrane_Contact.xometa` | OBSCURA → ORGANON | Whisper | `Audio->FM` 0.20 | Physical string harmonics barely disturb metabolic FM |
| `Obscura-Organon/String_Metabolism.xometa` | OBSCURA → ORGANON | Extreme | `Audio->FM` 0.78, `Amp->Filter` 0.52 | String audio fully ingested; organism's filter and FM rewritten |
| `Onset-Organon/Tap_on_Glass.xometa` | ONSET → ORGANON | Whisper | `Amp->Filter` 0.22 | Drum transients barely disturb organism filter |
| `Onset-Organon/Drum_Fed_Organism.xometa` | ONSET → ORGANON | Extreme | `Amp->Filter` 0.78, `Rhythm->Blend` 0.50 | Kick drives filter + metabolic blend; organism synchronized to pattern |
| `Overworld-Obscura/Pulse_Tightens.xometa` | OVERWORLD → OBSCURA | Whisper | `Amp->Filter` 0.20 | NES pulse attacks fractionally stiffen the string |
| `Overworld-Obscura/8bit_Catgut.xometa` | OVERWORLD → OBSCURA | Extreme | `Amp->Filter` 0.78, `Env->Morph` 0.52 | YM2612 FM rewrites string stiffness and scan position simultaneously |

---

## Coupling Type Coverage After Round 12E

Round 12E adds the following types to the library's coverage:

| Type | Status Before 12E | Status After 12E |
|------|------------------|-----------------|
| `Audio->FM` | Present (Overworld→Opal, Oracle→Organon) | Extended to Obscura→Organon |
| `Rhythm->Blend` | Present (Opal→Overdub, Ouroboros→Onset) | Extended to Onset→Organon |
| `Env->Morph` | Present (Odyssey→Opal, Overworld→Opal, Oracle→Organon) | Extended to Overworld→Obscura |
| `Amp->Filter` | Present in all 6 v1.0 pairs | Extended to all 3 new pairs |

No new coupling types are introduced. The focus of 12E is *destination diversity* — ORGANON and OBSCURA were not receiving engines in the v1.0 library, making all six presets here genuinely new demonstrations of how those engines respond to external modulation.

---

## What Makes Each Pairing Musically Interesting

**OBSCURA → ORGANON: Physical → Biological**

This is the only pairing in the library where a deterministic physical model feeds a biological model. Obscura's string is governed by partial differential equations (wave equation with damping and nonlinear spring term). Organon's metabolism is governed by variational free energy minimization. Two completely different mathematical frameworks for synthesizing timbre are placed in contact, with the physical model's output becoming the organism's food. The musical result is a timbre that evolves on two timescales simultaneously: the string's own resonant decay (fast, deterministic) and the organism's metabolic adaptation (slow, probabilistic). Neither timescale dominates at Whisper intensity; at Extreme, the organism absorbs the string's character and re-emits it transformed.

**ONSET → ORGANON: Rhythmic → Metabolic**

Existing ONSET coupling (→ OVERBITE) demonstrates that percussion can drive *character* synthesis. This new pairing demonstrates that percussion can drive *biological* synthesis. The difference is audible: OVERBITE's sidechain response is fast and lockstep with the beat. ORGANON's metabolic response integrates incoming energy over time, creating a slow-burn organism that is not quite synchronized to the beat but has been *shaped by it*. The `Rhythm->Blend` coupling is particularly unusual — the organism's metabolic strategy shifts with pattern density, meaning a fill section and a sparse groove section produce different harmonic output from ORGANON even when playing the same note.

**OVERWORLD → OBSCURA: Digital Architecture → Physical Architecture**

OVERWORLD is the fleet's only engine with explicit historical hardware references — three real consoles with documented frequency tables, algorithmic structures, and signal paths. OBSCURA is the fleet's most physically grounded engine — a string governed by physics that predates digital synthesis. When the amplitude envelope of 1989 FM synthesis rewrites the resonant geometry of an analytic string model, the coupling is a collision of two incompatible engineering paradigms. The chip's amplitude curves (determined by YM2612 operator envelope attack and decay rates) physically reshape what kind of string is being plucked. This is *not* the chip audio being fed as a signal into the string — the chip is restructuring the string's own mechanics. At Extreme intensity, the string no longer knows what it is: it has been continuously reformed by an entity that has no concept of physical media.

---

## Library Status After Round 12E

- **Total coupling pairs represented:** 9 (was 6)
- **Total coupling presets:** 24 (was 18)
- **Engines now serving as coupling destinations:** OPAL, OVERDUB, OVERBITE, ONSET, ORGANON, OBSCURA (6 unique targets)
- **Engines now serving as coupling sources:** ONSET, OPAL, ODYSSEY, OVERWORLD, ORACLE, OUROBOROS, OBSCURA (7 unique sources)
- **New coupling types added to library:** None — existing types extended to new destinations
- **Types still not in library:** `FilterToFilter`, `AudioToRing`, `PitchToPitch`, `AmpToChoke`, `AudioToWavetable`, `AudioToBuffer`

Priority targets for Round 13 expansion: `FilterToFilter` (OBLIQUE → OBSCURA or ORACLE → OBSCURA would be natural candidates given spectral complexity on both sides) and `AudioToBuffer` (any engine → OPAL demonstrating the Time Telescope coupling type — the only AudioToBuffer receiver currently available).
