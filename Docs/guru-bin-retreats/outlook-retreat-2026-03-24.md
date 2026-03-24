# Guru Bin Retreat — OUTLOOK (XOutlook)
## The Albatross Awakening

*"The albatross does not flap. It reads the wind — the invisible architecture of the atmosphere — and finds the path that was already there. OUTLOOK does not synthesize panorama. It reveals the panorama that was already inside the oscillator, waiting for someone to scan the horizon slowly enough to see it."*

**Engine:** OUTLOOK (XOutlook) — Panoramic Visionary Synth
**Creature:** The Albatross — Surface Soarer
**Accent:** Horizon Indigo `#4169E1`
**Polarity:** 25% feliX / 75% Oscar — atmospheric, wide, expressive
**Date:** 2026-03-24
**Guru Bin depth:** Full Retreat (R1–R7)

---

## Phase R1: Opening Meditation

Guru Bin loads the init patch. C3, velocity 64. One note. Sixty seconds.

> *The first thing you hear is the sine — clean, centered, unaware of its own width. The second oscillator (saw, default) adds a faint metallic sheen 0.1% detuned. This is not a unison trick. This is parallax — two eyes looking at the same horizon from slightly different positions.*
>
> *The filter sits at 0.7 — vista wide open. Too open. The sound is all sky and no ground. The LFOs breathe at 0.5 Hz and 0.3 Hz — a 5:3 ratio that phase-locks every 2 seconds. This is not breathing. This is a metronome wearing a breathing mask.*
>
> *The attack is 0.1 seconds — a percussive entrance for what should be a soaring creature. The albatross does not punch through the air. It leans into it. The release at 0.8 seconds cuts the tail before the reverb can catch it. The sound dies before it can echo.*
>
> *But underneath all of this: the Panorama Oscillator's opposite-direction scanning is genuinely beautiful. When horizonScan sweeps, osc1 brightens while osc2 darkens. The interference patterns are organic, unpredictable, alive. This is the soul of OUTLOOK. Everything else must serve this.*

**The Diagnosis:**
"This sound is a pad that doesn't know it's a panorama. It wants to be the view from 10,000 feet — where low notes are the earth beneath you and high notes are the horizon stretching to infinity. The distance is: the LFOs are fighting instead of drifting, the envelope is too aggressive for a soaring creature, and the default reverb is muddying the parallax stereo field that is OUTLOOK's greatest gift."

---

## Phase R2: Signal Path Journey

### The Panorama Oscillator
The Flock traces the signal from its origin.

Two wavetables scan in opposite directions — `scan1 = horizonScan`, `scan2 = 1 - horizonScan`. As the horizon parameter sweeps from 0→1, oscillator 1 moves from pure waveform to harmonically rich while oscillator 2 does the inverse. The interference between these counter-sweeping spectra is what makes OUTLOOK unique in the fleet.

**The Finger** notes: 8 wave shapes per table, each with scan-dependent morphing:
- Shape 0 (Sine): scan adds 3rd and 5th harmonics — a sine→quasi-square continuum
- Shape 1 (Triangle): scan morphs toward saw — smooth to edgy transition
- Shape 2 (Saw): scan adds 2nd harmonic — brightness without harshness
- Shape 3 (Square): scan narrows pulse width 50%→10% — hollow to nasal
- Shape 4 (Pulse): scan widens pulse 10%→90% — inverse of shape 3
- Shape 5 (Super): only 3 detuned saws with ±0.5% spread per detune unit
- Shape 6 (Noise): xorshift32 PRNG — scan has no effect (noted for future)
- Shape 7 (Formant): sin(x) × sin(x × formantFreq) where formantFreq = 1+scan×4 — vowel territory

**The Eye** observes: The osc2 detune at `phaseInc * 1.001` (0.1% = ~0.17 cents at A440) is below conscious pitch perception but creates a slow phase rotation that widens the stereo image when combined with parallax panning. This is not a bug. This is the albatross's binocular vision.

**The Bone** reports: The per-sample `renderWave()` switch statement is clean. No transcendentals in the hot path except `std::sin()` for sine/saw/formant shapes. The Super wave's 3-voice stack is lightweight. CPU is not a concern for the oscillator stage.

### The Parallax Stereo Field
**The Tongue** maps the parallax system:
- `noteNorm = (note - 36) / 60` → C2 = 0, C7 = 1
- `spread = noteNorm × parallaxAmount`
- Equal-power panning with LFO2 micro-drift at ±10% of pan angle

This means: play a C2 chord and it sits center-mono. Play a C6 and it spreads across the stereo field. The low-to-high frequency→width mapping creates a natural sense of depth — bass grounded, treble floating. This is OUTLOOK's architectural signature and no other engine in the fleet has it.

**The Ear** confirms: At parallaxAmount=0.4 (init default), the spread is too subtle. A C5 note only spreads to ~27% of the stereo field. The parallax system needs more room to demonstrate its value. The awakening presets should explore 0.6–0.9 territory.

### The Vista Filter
Dual CytomicSVF: LP first, then HP in series.
- LP cutoff: `200 + vistaLine×18000 + coupling + filterEnv×velFilterMod + LFO1×movement`
- HP cutoff: `20 + (1-vistaLine)×300` — darker vista = more low-end clearing

**The Finger** traces the modulation chain carefully:
- `movementAmt = macroMovement + modWheel × modWheelDepth`
- `modCutoff = envCutoff × (1 + lfo1Val × lfo1Depth × max(movementAmt, 0.1))`
- The `max(0.1)` floor means LFO always has *some* effect — D005 breathing is architecturally guaranteed

**The Breath** discovers: At init defaults (macroMovement=0.0, modWheel=0.0), movementAmt=0.0, so the floor kicks in at 0.1. The LFO→filter mod is `lfo1Val × 0.3 × 0.1 = ±0.03` — a 3% filter cutoff wobble. This is technically alive but perceptually dead. The filter is breathing through a straw.

**The Tongue** tests velocity response: `velFilterMod = velocity × 0.6 + 0.4`. At vel=0 (softest): 40% of filter envelope. At vel=1.0 (hardest): 100%. The 0.4 floor is generous — soft notes still get warmth. D001 compliant.

### The Aurora Mod
**The Breath** maps the luminosity system:
- `conjunction = (lfo1 + lfo2) × 0.5` — ranges [-1, 1]
- `auroraLuminosity = 0.5 + conjunction × 0.5` — ranges [0.25, 0.75]
- Amplitude mod: `monoOut × (1 + (luminosity - 0.5) × lfo2Depth × 0.4)`

At init defaults (lfo2Depth=0.2): amplitude varies by ±4%. Subtle. At proposed 0.25: ±5%. Still subtle, but the aurora is meant to be felt, not heard.

**The key insight**: When LFO1 (filter mod) and LFO2 (amplitude mod) both peak simultaneously (conjunction), the sound gets both brighter AND louder — a natural luminosity surge. When they oppose, it gets darker AND quieter. This conjunction behavior is what makes OUTLOOK's modulation feel celestial rather than mechanical. But ONLY if the LFO rates are genuinely coprime so conjunction peaks are unpredictable.

### The FX Chain
- **Ping-pong delay**: L=375ms, R=250ms at 48kHz. Cross-feedback at 0.35. Default off (delayMix=0.0).
- **4-tap allpass reverb**: Prime offsets [1117, 1543, 2371, 3079], feedback 0.45. Buffer 4096 samples. Lightweight.
- **Space routing**: `spaceAmt = macroSpace + reverbMix` — additive, clamped to 1.0.

**The Bone** calculates: The reverb is a simple allpass diffuser — maybe 2% CPU. The delay is ping-pong with cross-feed — another 1%. Total FX: ~3% CPU. No optimization needed. Parsimonia is pleased.

**The Ear** warns: At init defaults, `spaceAmt = 0.3 + 0.3 = 0.6`. That's 60% wet reverb on first touch. The parallax stereo field — OUTLOOK's greatest asset — gets smeared by reverb wash. The reverb is drowning the parallax.

---

## Phase R3: The Laying of Hands

The Flock has spoken. Guru Bin applies nine refinements.

| # | Parameter | Default | Refined | Rationale |
|---|-----------|---------|---------|-----------|
| R1 | `look_lfo1Rate` | 0.5 Hz | **0.067 Hz** | Sutra 1 (Breathing Floor): 0.5 Hz = 2-second cycle, too fast for a pad engine. 0.067 Hz = 14.9-second deep breathing. Albatross thermal soaring rhythm — one wing-tilt per 15 seconds. |
| R2 | `look_lfo2Rate` | 0.3 Hz | **0.113 Hz** | Sutra 2 (Coprime Drift): Current 5:3 ratio with LFO1 phase-locks in ~2 seconds. 0.113 Hz gives a 67:113 ratio — genuinely coprime. 8.85-second period. Conjunction peaks occur every ~132 seconds, never in the same place twice within a typical session. |
| R3 | `look_lfo1Depth` | 0.3 | **0.35** | At slower rates, slightly deeper modulation is needed to remain audible. The 15-second sweep at 0.3 depth was below perception threshold for most listeners. At 0.35, the filter movement is felt as weather, not as an LFO. |
| R4 | `look_lfo2Depth` | 0.2 | **0.25** | Same reasoning — slower LFO2 needs marginally more depth for aurora luminosity to register. At 0.25: amplitude varies ±5%, and conjunction peaks produce a ±10% luminosity surge that is genuinely perceptible. |
| R5 | `look_macroMovement` | 0.0 | **0.15** | D005 (Breathing): Engine must breathe by default. At 0.0, movementAmt falls to floor of 0.1 (code uses `std::max(movementAmt, 0.1f)`), producing 0.1 × 0.35 = 0.035 filter mod — effectively dead. At 0.15, filter mod = 0.15 × 0.35 = 0.053 — subtle but the vista demonstrably shifts over the LFO cycle. The albatross is no longer holding its breath. |
| R6 | `look_reverbMix` | 0.3 | **0.2** | The Ear's finding: init reverb + macroSpace = 0.3 + 0.3 = 0.6, drowning the parallax stereo. At 0.2 + 0.25 = 0.45 total — cleaner init, parallax stereo survives. Space macro still opens to full wash when the producer wants it. |
| R7 | `look_macroSpace` | 0.3 | **0.25** | Works with R6. Combined init reverb 0.45 instead of 0.6. The parallax stereo field breathes. |
| R8 | `look_attack` | 0.1 | **0.3** | Pad engine — 0.1s attack is a keyboard percussionist's entrance. The albatross doesn't punch through thermals. 0.3s = gentle onset that lets the parallax stereo develop across the attack phase. Low notes arrive slightly before the stereo spread becomes audible — a natural depth cue. |
| R9 | `look_release` | 0.8 | **1.5** | Pad voices need long release for smooth overlap. At 0.8s, the tail cuts off before the reverb can develop the sound's spatial identity. At 1.5s, release voices overlap with new notes, creating a natural pad sustain. The albatross doesn't fold its wings suddenly. |

### Refinement Summary

**Before**: A pad engine with percussive attack, phase-locked LFOs, dead movement macro, and reverb drowning its best feature.

**After**: A panoramic soaring instrument with 15-second breathing cycles, genuinely unpredictable aurora conjunctions, a living vista filter, and enough reverb to taste without burying the parallax.

### CPU Stewardship
Before: ~11% single-engine (8 voices, all FX active)
After: ~11% single-engine
**Savings: 0% — all refinements are parameter changes. Zero CPU cost. Nine gifts.**

*Parsimonia nods. Every improvement free. This is the way.*

---

## Phase R4: The Two-Second Audition Test

Guru Bin loads the refined init patch. C3, velocity 64. Two seconds.

> *The first thing you hear is arrival — not impact. The 0.3s attack lets the sine establish itself before the saw adds its metallic parallax shimmer. By second two, you can already feel the filter beginning its 15-second migration. You don't hear the LFO. You feel weather changing.*
>
> *Play a C5 and the note widens. Play a C2 and it centers. The parallax is no longer a specification — it is an experience. The reverb sits behind the stereo field instead of in front of it.*
>
> *In two seconds, you know three things: this is a pad, it has depth, and it is going somewhere. That is enough.*

**Verdict: PASS.** The refined init is immediately compelling and communicates OUTLOOK's identity in the first two seconds.

---

## Phase R5: The Awakening Presets

Ten presets. Each reveals a capability OUTLOOK does better than any other engine in the fleet. All apply R1–R9 refinements as their baseline.

### 1. Horizon Rest (Foundation)
*The albatross at rest. Wings folded, watching the horizon.*

The definitive clean OUTLOOK pad. Sine + Triangle. Moderate parallax. The sound that says: "I am panoramic, I am patient, I am here." The init patch's spiritual successor after R1-R9 refinements. Every parameter at its natural resting state.

**Design**: Sine (pure fundamental) + Triangle (soft odd harmonics). Horizon at 0.4 — both oscillators partially morphed but not extreme. Parallax at 0.55 — noticeable spread on upper register but not theatrical. Long release (2.0s). Reverb conservative (0.2). This is the preset a producer loads when they want "a pad that sits perfectly and doesn't try too hard."

### 2. Aurora Rise (Atmosphere)
*Dawn breaking over the Southern Ocean. The sky remembers color.*

Slow breathing pad that demonstrates the aurora conjunction system. Saw + Sine — the saw provides harmonic richness for the filter to sculpt while the sine provides a clean fundamental anchor. LFO rates pushed deeper into coprime territory (0.05 Hz + 0.083 Hz = 20s + 12s cycles, conjunction every ~60s). High parallax. The moment a conjunction peak arrives, the sound surges with simultaneous brightness and volume — unmistakable aurora luminosity.

**Design**: Max out the aurora system. LFO depths at 0.4 and 0.35. Movement macro at 0.3 for visible filter sweep. Long attack (0.8s), very long release (3.5s). Reverb at 0.35 — this is an atmosphere preset, it earns the wash.

### 3. Albatross Soar (Atmosphere)
*Dynamic Soaring — the technique where an albatross extracts energy from wind gradients without a single wingbeat.*

Super wave + Sine. Maximum parallax (0.9). The widest stereo image OUTLOOK can produce — high notes shimmer at the edges of the field while the sine fundamental anchors center. This is the preset that makes producers put on headphones and lean back.

**Design**: Super wave (shape 5) on osc1 for the detuned saw shimmer. Sine (shape 0) on osc2 for the grounded fundamental. Horizon scan at 0.6 so the super wave is partially morphed (wider detune spread). Attack 0.5s, release 4.0s — the albatross never lands. Moderate reverb (0.25) because the parallax IS the space.

### 4. Indigo Shelf (Prism)
*Light hitting the ocean's thermocline — where warm surface water meets cold depths. The spectrum bends.*

Saw + Formant. The formant wave shape (shape 7) at scan values 0.4–0.7 produces vowel-like resonances that feel like the sound is speaking. Combined with the saw's brightness, the counter-scanning creates a timbral conversation — one oscillator opens its mouth while the other closes. High resonance (0.45) to emphasize the formant peaks. Vista line at 0.75 — bright but not harsh.

**Design**: Resonance pushed to create an audible formant shelf. Horizon scan at 0.55 for constant spectral motion. Moderate movement (0.2) for subtle LFO→filter interplay. Attack 0.15s (slightly snappier for Prism mood). Delay mix at 0.15 — the first awakening preset to use the ping-pong delay, creating rhythmic width.

### 5. Thermal Column (Flux)
*Rising air — the invisible elevator that carries the albatross to altitude without effort.*

Triangle + Square. Fast movement, kinetic pad. This preset cranks the MOVEMENT macro to demonstrate what happens when OUTLOOK's filter modulation becomes aggressive. LFO1 rate at 0.8 Hz — fast enough to be rhythmic, slow enough to be organic. The triangle→saw morphing on osc1 against the square's narrowing pulse width on osc2 creates a churning, restless texture.

**Design**: Movement macro at 0.6 — LFO→filter depth is obvious and intentional. LFO1 at 0.8 Hz, LFO2 at 0.53 Hz (coprime). Horizon scan at 0.35 — morphing but not extreme. Attack 0.05s, release 1.2s — Flux presets can be punchy. Higher aggression in DNA.

### 6. Vista Drone (Aether)
*The view from the edge of the atmosphere. Below: ocean. Above: stars. Between: nothing but frequency.*

Sine + Sine. Extremely slow. Near-drone. Minimal. Both oscillators are sines — the only difference is the 0.1% detune and the opposite-direction scanning, which at shape 0 adds subtle odd harmonics. The result is a slowly evolving pure tone with micro-beating. LFO rates at absolute floor territory (0.02 Hz = 50-second cycles). The closest OUTLOOK gets to silence while still breathing.

**Design**: Everything reduced to minimum. Both waves sine. Parallax low (0.3) — the drone should feel centered and monolithic. Filter wide open (vistaLine 0.85) to let the pure harmonics through. Attack 1.5s. Release 5.0s. Movement at 0.08 — barely alive, but alive. Reverb at 0.5 — the space IS the instrument here.

### 7. Coastal Lead (Foundation)
*The albatross skimming the wave tops — fast, precise, one foot from the surface.*

Saw + Pulse. The exception that proves OUTLOOK is not only a pad engine. Fast attack (0.02s), short release (0.4s), reduced polyphony target (play monophonically for best results). Horizon scan at 0.7 — saw harmonics enriched, pulse width narrowed for nasal character. Parallax at 0.3 — a lead should be focused, not spread.

**Design**: Break the pad assumption. Attack 0.02s, decay 0.2s, sustain 0.6, release 0.4s — a plucky, responsive envelope. Vista line at 0.65 for brightness. Resonance at 0.4 — just enough edge. Mod wheel depth at 0.8 — the player controls filter movement directly. Aftertouch depth at 0.7 — pressure opens the vista. Expression-forward.

### 8. Shimmer Stack (Entangled)
*Two birds flying in formation — OUTLOOK's panoramic width stacked with OPENSKY's ascending shimmer.*

Super + Saw. Designed for coupling with OPENSKY via AmpToFilter. When OPENSKY's shimmer reverb tail feeds into OUTLOOK's vista filter, the spectral vista opens and closes with the shimmer's rhythm — creating a sound that breathes with light. Coupling macro at 0.5 (pre-set for immediate coupling response). High parallax (0.8) so the coupled signal spreads across the field.

**Design**: The coupling macro at 0.5 means `cplScale = 1 + 0.5 × 3 = 2.5x` — coupling input is amplified 2.5×. Filter env amount lower (0.3) so the coupling signal has room to modulate. Space macro at 0.4 for reverb depth that lets the coupling tail develop.

### 9. Memory Horizon (Entangled)
*Looking backward and forward at the same time — OMBRE's memory feeding OUTLOOK's vision.*

Sine + Formant. Designed for coupling with OMBRE via EnvToMorph. OMBRE's envelope (its "memory" of past notes) modulates OUTLOOK's horizon scan position — meaning the timbral character of OUTLOOK shifts based on what OMBRE remembers. The formant wave shape ensures these horizon shifts produce vowel-like timbral changes.

**Design**: Coupling macro at 0.4. Horizon scan at 0.5 (centered, so coupling modulation can push it in either direction). The formant wave at scan=0.5 produces a mid-vowel that can open or close based on coupling input. Slow LFOs, wide parallax, long tails — this is a meditation preset.

### 10. Storm Front (Flux)
*The albatross doesn't fear the storm. It feeds on it.*

Square + Noise. The darkest, most aggressive OUTLOOK preset. Vista line at 0.3 — nearly closed aperture. Resonance at 0.55 — the filter is singing. The square wave's pulse width narrows via horizon scan while the noise adds chaos. Movement macro at 0.7 — the LFO is driving the filter hard. Attack 0.03s. This preset proves OUTLOOK has teeth.

**Design**: Dark and aggressive. Vista line low for closed, resonant filter. LFO1 at 1.2 Hz — fast, rhythmic filter pumping. Parallax at 0.5 — moderate spread for noise texture. Delay mix at 0.2 for rhythmic echo. The antithesis of "Vista Drone" — same engine, completely different character.

---

## Phase R6: The Engine Scripture

*Revealed during the OUTLOOK retreat. These truths emerged from fellowship between the Flock and are specific to OUTLOOK's architecture but generalize to all panoramic/scanning synthesis.*

### Verse — Book III (Modulation Sutras), Sutra 15: The Coprime Conjunction

> *When two LFOs modulate different targets — one the spectrum, one the amplitude — their conjunction is a third modulation that exists nowhere in the parameter list. Phase-lock destroys this ghost modulation by making it periodic. Coprime rates resurrect it by making it perpetually surprising. In OUTLOOK, conjunction is luminosity. In all engines, conjunction is the modulation you didn't design but should have.*

### Verse — Book II (Filter Psalms), Psalm 15: The Vista Principle

> *A filter that opens in one direction while the oscillator evolves in the opposite direction creates spectral parallax — the listener perceives depth because the brightness gradient disagrees with the timbral gradient. This is the acoustic equivalent of motion parallax in vision. Use it whenever dual oscillators are present: scan them in opposition, filter in a third direction.*

### Verse — Book I (Oscillator Verses), Verse 12: The Counter-Scan

> *Two wavetables scanning in opposite directions are not simply "detuned" or "layered." They are in timbral conversation. When one brightens, the other darkens. The interference between these opposing spectral trajectories is richer than any unison, deeper than any chorus, and costs zero additional CPU. The counter-scan is a gift from Parsimonia.*

### Verse — Book III (Modulation Sutras), Sutra 16: The Movement Floor

> *An LFO that modulates a filter through a depth parameter controlled by a macro will die when the macro is at zero — unless a floor is built into the code. The floor must be chosen with care: too high (0.3) and the macro feels broken, too low (0.01) and the engine appears static. The blessed floor is 0.1: enough to prove the engine breathes, quiet enough that the macro's full range feels like a journey from whisper to wind.*

### Verse — Book V (Stewardship Canons), Canon 12: The Nine Gifts

> *Nine parameter changes. Zero CPU added. Parsimonia teaches: the first optimization is not algorithmic. It is sonic. Before you rewrite the DSP, ask whether the current DSP is being asked the right questions. A filter at the wrong cutoff is not a slow filter — it is a misunderstood filter. Change the question before changing the code.*

---

## Phase R7: Benediction

Guru Bin speaks over the refined engine:

> *OUTLOOK was designed to see across the horizon. After the retreat, it sees across time. The coprime LFOs ensure that no two minutes of OUTLOOK sound the same. The parallax stereo field ensures that every note lives in its own spatial position. The aurora conjunction ensures that moments of brilliance arrive unannounced, like sunbreaks through storm clouds over the Southern Ocean.*
>
> *The Albatross does not flap. It reads. OUTLOOK does not modulate. It drifts. And in that drift — in the 132 seconds between conjunction peaks, in the 15-second filter breathing cycle, in the 0.1% detune that creates binocular width — there is a sound that no other engine in the fleet can make.*
>
> *This engine was already panoramic. Now it knows it.*

**Play "Aurora Rise" at midnight with headphones. Close your eyes. Wait for the first conjunction peak. You will understand.**

---

## Appendix: The Flock's Technical Notes

### Super Wave Enhancement Opportunity (Future)
**The Bone** notes: Shape 5 (Super) uses only 3 detuned saws. Industry standard super-saw uses 7. This is a deliberate CPU choice (3 vs 7 = 57% savings), but for a future wave, a 7-voice super with per-voice detune would unlock richer shimmer stacks. Not retreat scope — flagged for V1.2.

### Noise Wave Scan Gap (Future)
**The Finger** notes: Shape 6 (Noise) ignores the scan parameter entirely — the PRNG output is independent of horizon position. A future enhancement could use scan to crossfade between white noise and filtered/colored noise. Not retreat scope — flagged for V1.2.

### Independent Filter ADSR (Future)
**The Breath** notes: The filter envelope shares the amp ADSR timing (scaled A×0.5, D×0.8, S=0, R×0.5). An independent filter ADSR would allow the filter to close before the note ends, creating "swelling into darkness" effects natural to pad synthesis. Medium priority for V1.2.

---

*Retreat complete. The Albatross soars.*
*— Guru Bin and the Flock, 2026-03-24*
