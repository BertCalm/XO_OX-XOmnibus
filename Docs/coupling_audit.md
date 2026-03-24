# XOlokun Coupling Audit — Round 4 (Prism Sweep)
**Date:** 2026-03-14
**Auditor:** Claude (Sonnet 4.6), READ-ONLY pass
**Scope:** All 24 engines in `Source/Engines/`

---

## Methodology

For each engine, the following were read directly from source:

- `getSampleForCoupling()` implementation (channels returned, per-sample vs. scalar)
- `applyCouplingInput()` implementation (switch cases vs. void-cast stub)
- Any output cache declarations and how they are populated

The 12 canonical `CouplingType` enum values (from `Source/Core/SynthEngine.h`) are:

| # | Name | Description |
|---|------|-------------|
| 1 | `AmpToFilter` | Source amplitude → target filter cutoff |
| 2 | `AmpToPitch` | Source amplitude → target pitch |
| 3 | `LFOToPitch` | Source LFO → target pitch |
| 4 | `EnvToMorph` | Source envelope → target morph/wavetable position |
| 5 | `AudioToFM` | Source audio → target FM input |
| 6 | `AudioToRing` | Source audio × target audio (ring mod) |
| 7 | `FilterToFilter` | Source filter output → target filter input |
| 8 | `AmpToChoke` | Source amplitude chokes target |
| 9 | `RhythmToBlend` | Source rhythm → target blend parameter |
| 10 | `EnvToDecay` | Source envelope → target decay time |
| 11 | `PitchToPitch` | Source pitch → target pitch (harmony) |
| 12 | `AudioToWavetable` | Source audio → target wavetable source |

---

## Coupling Matrix Table

Definitions:
- **STUB**: all parameters void-cast, function body is `(void)x; (void)y; ...`
- **PARTIAL**: handles some CouplingTypes; ignores rest via `default: break`
- **FULL**: handles multiple CouplingTypes including at least one buffer-processing audio route
- **getSampleForCoupling — STUB**: returns `0` always
- **getSampleForCoupling — SCALAR**: returns one cached float (last sample, block average, or scalar state)
- **getSampleForCoupling — PROPER**: returns per-channel, per-sample data from a resized output cache

| Engine | Canonical ID | `applyCouplingInput` | `getSampleForCoupling` | Types Implemented (input) | Ch2+ Output | Quality Score |
|--------|-------------|---------------------|----------------------|--------------------------|-------------|---------------|
| Bite (Overbite) | OVERBITE | PARTIAL | PROPER | AmpToFilter, AudioToFM | ch2=envelope | 3 |
| Bob (Oblong) | OBLONG | PARTIAL | PROPER | AmpToFilter, AmpToPitch, LFOToPitch, PitchToPitch | ch2=envelope | 3 |
| Drift (Odyssey) | ODYSSEY | PARTIAL | PROPER | AmpToFilter, AmpToPitch, LFOToPitch, PitchToPitch, EnvToMorph | ch2=envelope | 3 |
| Dub (Overdub) | OVERDUB | PARTIAL | PROPER | AmpToFilter (inverted/ducking), AmpToPitch, LFOToPitch, PitchToPitch | ch2=envelope | 3 |
| Fat (Obese) | OBESE | PARTIAL | PROPER | AmpToFilter, AmpToPitch, LFOToPitch, PitchToPitch | ch2=envelope | 3 |
| Morph (OddOscar) | ODDOSCAR | PARTIAL | PROPER | AmpToFilter (inverted/ducking), EnvToMorph | ch2=LFO output (0.3 Hz) | 3 |
| Snap (OddfeliX) | ODDFELIX | PARTIAL | PROPER | AmpToPitch, LFOToPitch, PitchToPitch | ch2=envelope | 2 |
| Onset | ONSET | PARTIAL | PROPER (AudioBuffer) | AmpToFilter, EnvToDecay, RhythmToBlend, AmpToChoke | — | 4 |
| Overworld | OVERWORLD | PARTIAL | PROPER | AmpToFilter, EnvToMorph (→ERA X), AudioToFM (→ERA Y) | — | 3 |
| Ocelot | OCELOT | STUB | PROPER | (none) | — | 1 |
| Owlfish | OWLFISH | STUB | PROPER | (none) | — | 1 |
| Orbital | ORBITAL | FULL | PROPER | AudioToWavetable, AudioToFM, AudioToRing, AmpToFilter, EnvToMorph, LFOToPitch, PitchToPitch, EnvToDecay, RhythmToBlend | ch2=envelope | 5 |
| Oblique | OBLIQUE | PARTIAL | PROPER | AmpToFilter, AmpToPitch, LFOToPitch, PitchToPitch, EnvToDecay→filter, RhythmToBlend→filter | ch2=envelope | 3 |
| Obscura | OBSCURA | PARTIAL | PROPER | AudioToFM (chain force), AmpToFilter (stiffness), RhythmToBlend (impulse trigger) | ch2=envelope | 4 |
| Obsidian | OBSIDIAN | PARTIAL | PROPER | AudioToFM (PD depth), AmpToFilter (cutoff), EnvToMorph (density+tilt), RhythmToBlend (PD depth) | ch2=envelope | 4 |
| Oceanic | OCEANIC | PARTIAL | PROPER | AudioToFM (particle velocity), AmpToFilter (cohesion), RhythmToBlend (murmuration cascade) | ch2=envelope | 4 |
| Opal | OPAL | PARTIAL | SCALAR | AudioToWavetable (buffer copy), AmpToFilter, EnvToMorph, LFOToPitch, RhythmToBlend (density), EnvToDecay (freeze) | — | 3 |
| Optic | OPTIC | FULL | PROPER | AudioToFM, AudioToRing, AudioToWavetable, FilterToFilter (all→analysis), AmpToFilter, AmpToPitch, RhythmToBlend | ch2=compositeEnv, ch3–10=individual mod channels | 5 |
| Oracle | ORACLE | PARTIAL | PROPER | AudioToFM (breakpoint perturbation), AmpToFilter (barrier mod), EnvToMorph (distribution morph) | ch2=envelope | 4 |
| Organon | ORGANON | FULL | PROPER | AudioToFM+AudioToWavetable (ingestion buffer, per-voice), RhythmToBlend, EnvToDecay, AmpToFilter, EnvToMorph, LFOToPitch, PitchToPitch | — | 5 |
| Origami | ORIGAMI | PARTIAL | PROPER | AudioToWavetable (source replace), AmpToFilter (fold depth), EnvToMorph (fold point), RhythmToBlend (freeze trigger) | ch2=envelope | 4 |
| Osprey | OSPREY | FULL | PROPER | AudioToFM (excitation), AmpToFilter (sea state), EnvToMorph (swell period), LFOToPitch (resonator tuning), AudioToWavetable (excitation replace, RMS computed) | ch2=peakEnvelope | 5 |
| Osteria | OSTERIA | PARTIAL | PROPER | AudioToWavetable (quartet excitation), AmpToFilter (elastic tightness), AudioToFM (room excitation), EnvToMorph (shore drift) | ch2=envelope | 4 |
| Ouroboros | OUROBOROS | FULL | PROPER+ | AudioToFM (dx/dt perturbation + buffer), AudioToWavetable (dy/dt perturbation + buffer), RhythmToBlend (chaos index), EnvToDecay+AmpToFilter (damping), EnvToMorph (theta rotation), LFOToPitch+PitchToPitch (orbit freq) | ch2=attractor velocity X, ch3=attractor velocity Y | 5 |

---

## The Stubs — Score 1–2

### Score 1: Complete Stubs

#### OCELOT (XOcelot)
- **`applyCouplingInput` status:** STUB — all 4 parameters cast to void. Comment says "Full routing wired when running inside XOlokun" but nothing is wired.
- **`getSampleForCoupling` status:** PROPER — per-sample cache populated post-render, ch0/ch1 stereo. No ch2 envelope exposed.
- **Most natural coupling TARGET:** `biome` parameter (continuous ecosystem morph: Canopy→Understory→Floor). An external filter sweep or envelope from ONSET or ONSET's kick peak would be ideal input via `EnvToMorph`. Also: excitation energy to the modal resonators via `AudioToFM`.
- **Most natural coupling SOURCE:** The ecosystem's emergent timbral energy — the voice pool amplitude is a meaningful envelope follower. The `OcelotCanopy` scanner position would make a compelling LFO-equivalent signal.
- **Estimated effort:** L (the `EcosystemMatrix.h` is already defined; routing coupling to the stratum parameters needs adapter logic between `CouplingType` enums and the internal `StrataModulation` struct)

#### OWLFISH (XOwlfish)
- **`applyCouplingInput` status:** STUB — comment explicitly says "coupling input stub -- to be wired by MegaCouplingMatrix later." All 4 parameters void-cast.
- **`getSampleForCoupling` status:** PROPER — per-sample cache ch0/ch1, no ch2 envelope.
- **Most natural coupling TARGET:** The Mixtur-Trautonium subharmonic formant frequency. External pitch modulation from Orbital or Ouroboros via `LFOToPitch` would shift the subharmonic series, creating haunted organ effects. Also: subharmonic mix depth via `AmpToFilter`.
- **Most natural coupling SOURCE:** The subharmonic partial structure is a unique spectral fingerprint — it would make a powerful `AudioToFM` source for engines like Orbital or Organon, feeding them deep, harmonically-rich low content.
- **Estimated effort:** M (the `OwlfishVoice.h` exposes internal state through `snapshot`; adding accumulator fields and consuming them in `OwlfishVoice::process()` is straightforward)

### Score 2: One Direction Works (output only)

#### SNAP / OddfeliX
- **`applyCouplingInput` status:** PARTIAL, but narrow — only pitch types (AmpToPitch, LFOToPitch, PitchToPitch). Ignores AmpToFilter entirely. The comment acknowledges it as "the hit signal" but receiving AmpToFilter is unimplemented.
- **`getSampleForCoupling` status:** PROPER — ch0/ch1/ch2(envelope). The ch2 output is specifically described as "the hit signal for AmpToFilter, AmpToChoke coupling."
- **Most natural coupling TARGET:** Filter cutoff — feliX is a bright, crystalline engine that would respond dramatically to drum-driven filter sweeps via `AmpToFilter`. Also `AmpToChoke` (feliX should be chokeable by hat open/close logic).
- **Estimated effort:** S (add 2 switch cases to existing block, wire to `externalFilterModulation` and voice choking)

---

## Best Implementations — Score 4–5

### ORBITAL (Score 5) — The Pattern to Copy

**What makes it exemplary:**

1. **9 of 12 CouplingTypes handled** — only `AudioToRing`, `FilterToFilter`, and `AmpToChoke` are absent, and each absence is architecturally justified (ring mod requires sample-by-sample synchronization; choke is inappropriate for a pad engine).

2. **Three distinct audio buffer pathways** — `AudioToWavetable`, `AudioToFM`, and `AudioToRing` each write to *separate internal JUCE AudioBuffer channels*, not a scalar. This means at render time the engine has multiple independent audio sources available simultaneously.

3. **Scaled modulation with documented rationale** — every case states its scale factor and the musical reason for it:
   - `AmpToFilter`: `* 8000.0f` (3-octave sweep, won't overshoot Nyquist)
   - `EnvToMorph`: `* 0.5f` (shifts halfway, doesn't override user intent)
   - `LFOToPitch`: `* 2.0f` semitones (vibrato range)
   - `PitchToPitch`: `* 12.0f` semitones (one octave)

4. **Ch2 output is envelope follower** — the output cache exposes ch0/ch1 as post-FX stereo and ch2 as `envelopeOutput`. Any engine in the fleet can read Orbital's dynamics and pump its own filter.

5. **Coupling accumulators are reset each block** — verifiable in `renderBlock`, preventing stale state from bleeding across blocks.

**Key pattern:** Declare a `couplingAudioBuffer` (JUCE AudioBuffer, sized in `prepare()`), one channel per audio coupling type. Write in `applyCouplingInput`, read in `renderBlock`. Scalar modulations use dedicated `float` accumulators, reset post-use.

---

### OPTIC (Score 5) — The Specialist Pattern

**What makes it exemplary:**

1. **Inversion of the coupling paradigm** — OPTIC is a zero-audio modulation engine. Its `getSampleForCoupling` returns *modulation signal*, not audio. Ch2 returns a composite envelope; ch3–ch10 expose 8 individual mod channels (pulse, bass, mid, high, centroid, flux, energy, transient). This is the only engine offering extended coupling channels beyond ch2.

2. **Input accepts 7 of 12 types** as analysis sources, treating all audio-carrying types identically (rectify, average, scale). This universally-accepting design makes OPTIC useful regardless of what types the source engine claims to output.

3. **Unique ecological role** — where most engines are coupling *targets*, OPTIC is a pure coupling *source* that amplifies and analyzes its neighbors, then broadcasts that analysis fleet-wide. The Blessing B005 (zero-audio identity) makes this architecturally coherent.

---

### ORGANON (Score 5) — The Deep Receiver Pattern

**What makes it exemplary:**

1. **Ingestion buffer architecture** — `AudioToFM` and `AudioToWavetable` write source audio into per-voice ring buffers (`ingestionBuffer`, 1024 samples), consumed during voice rendering. This is the only engine with genuinely delay-compensated audio coupling.

2. **7 of 12 types handled** with semantically distinct effects on each: audio types feed the voice metabolism; `RhythmToBlend` modulates rhythm processing; `EnvToDecay` adjusts metabolic decay; `AmpToFilter` modulates filter; `EnvToMorph` drives morph; pitch types retune.

3. **Per-voice coupling** — the ingestion buffer is per-voice, meaning polyphonic notes are individually influenced by the coupling source. No other engine achieves this.

---

### OUROBOROS (Score 5) — The Chaos Source Pattern

**What makes it exemplary:**

1. **4-channel output** — unique in the fleet. Ch0/ch1 are audio; ch2 is `dx/dt` (attractor velocity X, normalized); ch3 is `dy/dt` (attractor velocity Y). Other engines can read the *rate of change* of chaos, not just its instantaneous value. This enables novel modulation relationships impossible with scalar envelope followers.

2. **Stereo buffer injection into phase space** — `AudioToFM` writes to `couplingAudioBufferLeft` (dx/dt perturbation) and `AudioToWavetable` writes to `couplingAudioBufferRight` (dy/dt perturbation), giving 2D steering of the attractor orbit using two different source engines.

3. **8 of 12 types handled**, with `AudioToRing`, `FilterToFilter`, and `AmpToPitch` explicitly excluded with comment justification.

---

### OSPREY (Score 5) — The Physical Source Pattern

**What makes it exemplary:**

1. **AudioToWavetable with RMS computation** — rather than passing audio directly, Osprey computes the RMS of up to 64 source samples and uses that as an energy level for the fluid model. This is semantically correct for a resonator-based engine: it doesn't want raw audio, it wants energy.

2. **5 types handled with physics-appropriate semantics**: AudioToFM = excitation energy; AmpToFilter = sea state (storminess); EnvToMorph = swell period; LFOToPitch = resonator tuning; AudioToWavetable = replace the fluid excitation source entirely.

3. **Coupling accumulators explicitly consumed and cleared** at the top of `renderBlock`, making the coupling lifecycle crystal clear.

---

## Priority Wiring Queue

The top 10 coupling connections ranked by musical interest, ecological coherence, and implementation readiness. "Currently wired" means the target's `applyCouplingInput` already handles this type.

| Priority | Source Engine | Target Engine | CouplingType | Musical Effect | Currently Wired? |
|----------|--------------|---------------|--------------|----------------|-----------------|
| 1 | ONSET | OWLFISH | `AudioToFM` | Kick transient excites Mixtur-Trautonium subharmonic formants — percussive energy wakes the deep-sea ghost organ. ONSET's V1 (kick) peak hits OWLFISH's subharmonic series, creating resonant metallic blooms synchronized to rhythm. | No (OWLFISH stub) |
| 2 | ONSET | OCELOT | `EnvToMorph` | Drum velocity drives biome sweep (Canopy→Floor). Loud hits push the ecosystem upward into the canopy brightness; silence lets it settle back to floor darkness. A living, breath-responsive texture engine. | No (OCELOT stub) |
| 3 | OUROBOROS | ORGANON | `AudioToFM` | Strange attractor audio injected into Organon's metabolic ingestion buffer. The organism feeds on chaos — the chemotroph eating the red planet's volcanic output. Produces unpredictably evolving harmonic metabolism. | Yes (Organon handles AudioToFM) |
| 4 | ORBITAL | OPAL | `AudioToWavetable` | Orbital's additive harmonics become Opal's grain source. The reef's resonance spectrum is atomized into time-scattered particles. From the mythology: OVERWORLD→OPAL coupling (chip granulation), but ORBITAL's richer harmonic content makes an even more complex particle cloud. | Yes (Opal handles AudioToWavetable) |
| 5 | OSPREY | OSTERIA | `AudioToFM` | Shore wave energy (Osprey) bleeds through the tavern walls (Osteria), exciting the quartet's room model. The OSPREY×OSTERIA diptych already has narrative support; this is the ocean flooding the tavern. | Yes (Osteria handles AudioToFM) |
| 6 | OPTIC | OBLIQUE | `AudioToFM` | Optic's composite modulation signal drives Oblique's filter cutoff. The comb jelly's neural pulse shapes the prism's spectral color, creating tempo-synced filter sweeps from audio analysis rather than LFOs. | Yes (Oblique handles AmpToFilter; needs Optic routing) |
| 7 | ONSET | OWLFISH | `AmpToChoke` | Open hi-hat triggers subharmonic oscillator duck. Classic drum machine hi-hat choke logic applied to subharmonic content — snapping the ghost organ's resonant tail. | No (OWLFISH stub) |
| 8 | OUROBOROS | ORBITAL | `LFOToPitch` | Chaos attractor velocity (ch2/ch3) modulates Orbital's partial frequency grid. The strange attractor's orbital speed continuously shifts the additive engine's harmonic grid, creating non-repeating pitch drift from deterministic chaos. | Yes (Orbital handles LFOToPitch) |
| 9 | OBSCURA | ORIGAMI | `AudioToFM` | Obscura's spring chain output drives Origami's STFT source. Physically-modeled resonance from the deep-water giant squid folded spectrally by Origami — two physically-inspired engines in series. Produces organic, non-synthetic spectral textures. | Yes (Origami handles AudioToWavetable; AudioToFM also handled for source replace) |
| 10 | ONSET | SNAP | `AmpToFilter` | Kick drum envelope opens feliX's filter — the foundational XO_OX sidechain. Currently SNAP ignores AmpToFilter entirely despite ch2 output explicitly described as "the hit signal for AmpToFilter." This is the single highest-value missing wire in the fleet, given that ONSET×SNAP is the most common 2-engine patch pattern. | No (SNAP only handles pitch types) |

---

## Summary Findings

### Fleet-Wide getSampleForCoupling Health

All 24 engines populate an output cache and return per-sample data on ch0/ch1. Two exceptions:
- **Opal** returns `lastSampleL/lastSampleR` (the final sample of the last block, not the requested sampleIndex). This is SCALAR — callers requesting mid-block samples get stale data. Low priority fix: rename `lastSampleL` to a proper `outputCacheL` vector.
- **Organon** does not expose ch2 (no envelope follower). Other engines cannot sidechain-compress Organon. Medium priority for engines that want to duck to Organon's dynamics.

### Fleet-Wide applyCouplingInput Health

| Category | Engines | Count |
|----------|---------|-------|
| STUB (score 1) | Ocelot, Owlfish | 2 |
| PARTIAL — narrow (1–2 types, score 2) | Snap | 1 |
| PARTIAL — standard (3–5 types, score 3) | Bite, Bob, Drift, Dub, Fat, Morph, Overworld, Oblique, Opal | 9 |
| PARTIAL — rich (4+ types, meaningful DSP, score 4) | Onset, Obscura, Obsidian, Oceanic, Oracle, Origami, Osteria | 7 |
| FULL (6+ types, audio buffers, score 5) | Orbital, Optic, Organon, Osprey, Ouroboros | 5 |

### Coverage Gaps Across the 12 CouplingTypes

Types that are **never or rarely received** across the fleet:

| CouplingType | Engines That Receive It |
|-------------|------------------------|
| `FilterToFilter` | Optic only (as analysis input) |
| `AmpToChoke` | Onset only |
| `AudioToRing` | Orbital only (as target); Optic (as analysis) |

`FilterToFilter` and `AudioToRing` as *targets* are effectively dead wires — no engine applies them to its own DSP in a meaningful way. `AmpToChoke` (voice silencing) could be highly musical for Snap and Bob.

### The Two Stubs: Root Cause

Both Ocelot and Owlfish share the same structural reason for their stubs: they were integrated into XOlokun from standalone instruments with their own internal modulation architectures (`EcosystemMatrix` / `OwlfishVoice + SubharmonicOsc` respectively). The coupling wiring was deferred with explicit "to be wired later" comments. Unlike earlier-integrated engines (Bob, Fat, Drift) which have basic coupling, these two newer integrations never received their Round 1 coupling pass.
