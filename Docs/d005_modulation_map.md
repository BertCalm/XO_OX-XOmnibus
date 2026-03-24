# D005 Modulation Map — Round 4 Audit
**Doctrine D005:** "An Engine That Cannot Breathe Is a Photograph."
Every engine must have at least one modulation source operating between the per-sample envelope and
per-block macro sweeps — ideally an LFO with rates down to 0.01 Hz.

**Audit date:** 2026-03-14
**Auditor:** Modulation Auditor (Round 4 Prism Sweep)

---

## Summary Table

| Engine | LFO Count | LFO Rate Range | LFO Targets | Filter Env | Pitch Drift | Min Rate Possible | D005 Status |
|--------|-----------|----------------|-------------|------------|-------------|-------------------|-------------|
| **Snap** (OddfeliX) | 0 | — | — | No | No | — | **FAIL** |
| **Fat** (Obese) | 0 | — | — | Yes (→ cutoff) | Yes (MojoDrift ~10 Hz) | ~10 Hz | **PARTIAL** |
| **Morph** (OddOscar) | 1 (hardwired) | 0.3 Hz fixed | Output to coupling only | No | No | 0.3 Hz | **PARTIAL** |
| **Orbital** | 0 | — | — | No | No (coupling only) | — | **FAIL** |
| **Organon** | 0 (phason proxy) | 0.1–10 Hz | Metabolic rate modulation | No | No | 0.1 Hz | **PARTIAL** |
| **Onset** | 0 (MUTATE macro) | per-block stochastic | Blend + character noise | No | No | >1 Hz (per-block) | **PARTIAL** |
| **Overworld** | 0 in adapter | — | — | No | No (ERA Drift not wired) | — | **FAIL** |
| **Owlfish** | 0 | — | — | No | No | — | **FAIL** |
| **Osprey** | 1 | 0.05–1.0 Hz | Sea state amplitude | No | No | 0.05 Hz | **PASS** |
| **Bob** (Oblong) | 2 | LFO1: 0.01–20 Hz; LFO2: 0.05 Hz hardwired | Filter, pitch, osc shape, FX depth + slow random | Yes (filter env) | Yes (OscDrift) | 0.01 Hz | **PASS** |
| **Drift** (Odyssey) | 1 | 0.1–20 Hz | Pitch, filter, amp | Yes (→ cutoff) | Yes (VoyagerDrift 0.05–2 Hz) | 0.05 Hz | **PASS** |
| **Dub** (Overdub) | 1 | 0.1–20 Hz | Pitch, filter, amp | No (pitch env instead) | Yes (AnalogDrift ≤0.5 Hz LP) | sub-0.5 Hz | **PASS** |
| **Bite** (Overbite) | 1+ | 0.01–50 Hz | Multiple macro-controlled | No | Yes (OscA/B drift) | 0.01 Hz | **PASS** |
| **Oblique** | 1 (phaser only) | 0.05–8 Hz | Phaser allpass sweep only | No | No | 0.05 Hz | **PARTIAL** |
| **Obscura** | 2 | 0.01–30 Hz each | LFO1→ scan width; LFO2→ excite position | No | No | 0.01 Hz | **PASS** |
| **Obsidian** | 2 | 0.01–30 Hz each | Phase distortion parameters | No | No | 0.01 Hz | **PASS** |
| **Oceanic** | 2 | 0.01–30 Hz each | Swarm position + particle modulation | No | No | 0.01 Hz | **PASS** |
| **Optic** | 1 (AutoPulse) | 0.5–16 Hz (+ 0.07 Hz drift) | Self-evolving rhythmic pulse | No | No | 0.07 Hz (drift) | **PASS** |
| **Oracle** | 2 | 0.01–30 Hz each | GENDY time step + amplitude step | No | No | 0.01 Hz | **PASS** |
| **Origami** | 2 | 0.01–30 Hz each | Fold point + rotate amount | No | No | 0.01 Hz | **PASS** |
| **Osteria** | 2 (hardwired) | 0.5 Hz fixed both | Formant positions + chorus delay | No | No | 0.5 Hz | **PASS** |
| **Ouroboros** | 0 | — | — | No | No | — | **PARTIAL** |
| **Ocelot** | 2+ | Canopy breathe ~0.1–0.6 Hz; TapeWarp 0.5–5.5 Hz | Canopy amplitude breathing; tape flutter delay | No | Yes (emergent pitch mod) | ~0.1 Hz | **PASS** |
| **Opal** | 2 | 0.01–20 Hz each | Mod matrix (12 destinations: filter, shimmer, frost, etc.) | Yes (→ cutoff) | No | 0.01 Hz | **PASS** |

**PASS: 15 engines** | **PARTIAL: 6 engines** | **FAIL: 4 engines**

---

## The 4 FAIL Engines

These engines have zero autonomous modulation sources. All parameters are static until the user
moves a knob or a coupling signal arrives from a partner engine.

---

### 1. Snap (OddfeliX)

**Identity:** Percussive neon tetra — every note is a flash, bright attack, rapid decay.

**Why it fails D005:** The engine's DSP infrastructure contains no phase accumulator or
rate-controlled oscillator beyond the audio-frequency oscillators (PolyBLEP) and the pitch-sweep
envelope. The pitch snap sweep operates at audio rate (it decays from a high starting pitch to the
target pitch over a few milliseconds), not LFO rates. The three Cytomic SVF filters, envelope, and
KarplusStrong delay-line are all statically parameterized per block.

**Existing infrastructure that could host an LFO:**
- `SnapVoice` has `float envelopeLevel` — a per-sample value that could trivially drive a parallel
  slow sine
- `KarplusStrongOscillator` has a `double delaySamples` member — modulating this would create
  subtle pitch drift on the string model (≡ de-tune at sub-Hz rates)
- `SnapEngine` renders per-sample in a loop; a single `double lfoPhase` member and one `lfoPhase +=
  lfoRate / sampleRate` line would fit naturally before the filter coefficients are set

**Parameter that would benefit most:** Filter cutoff. At rest the BPF is static; a 0.05 Hz
sinusoidal sweep of ±200 Hz on the bandpass center would create the impression of a slowly
rotating resonance, matching feliX's darting-through-sunlit-water identity.

**Minimum viable addition:**
```cpp
// In SnapEngine member state (1 member):
double lfoPhase = 0.0;

// In renderBlock, before filter coefficients are set (3 lines):
lfoPhase += 0.05 / sampleRate;  // 0.05 Hz — one cycle every 20 seconds
if (lfoPhase > 1.0) lfoPhase -= 1.0;
const float effectiveCutoff2 = effectiveCutoff
    + 200.0f * fastSin(static_cast<float>(lfoPhase * 6.28318f));
```

---

### 2. Orbital

**Identity:** 64-partial additive synthesis. 4-group envelopes create a "living spectrum."

**Why it fails D005:** The 4 group envelopes (`groupEnvLevel[4]`) are ADSR envelopes that control
partial-band amplitude over time — they modulate timbre (spectral balance) but only within the
note's ADSR lifecycle. Once the sustain phase is reached, the spectrum is frozen until note-off.
No autonomous oscillator modulates any synthesis parameter between per-sample envelope updates
and per-block macro changes.

The `externalPitchMod`, `externalMorphMod`, `externalFilterMod` fields accumulate incoming coupling
values only — they require a partner engine running an LFO to have any effect.

**Existing infrastructure that could host an LFO:**
- 64 phase accumulators (`double phase[64]`) demonstrate the engine already knows how to advance
  phase — a 65th accumulator running at LFO rate would be structurally identical
- The `spectralCouplingOffset` array (`float[64]`) is pre-allocated but driven from coupling. An
  internal LFO could write into a parallel internal offset array of the same shape
- The `formantFilter.rebuild()` path updates partial envelopes at `formantDirty` intervals; the
  same path could accept a slow LFO modulating `brightness` or `morph`

**Parameter that would benefit most:** Spectral morph position. A 0.03 Hz sine sweep of ±0.05
on the A↔B morph would give Orbital the slow "orbit" its name implies — partials smoothly
redistributing their weight as if circling a central point.

**Minimum viable addition:**
```cpp
// In OrbitalEngine member state (1 member):
double spectralDriftPhase = 0.0;

// In renderBlock, after param snapshot (3 lines):
spectralDriftPhase += 0.03 / cachedSampleRate;
if (spectralDriftPhase > 1.0) spectralDriftPhase -= 1.0;
const float morphMod = morphPosition
    + 0.05f * fastSin(static_cast<float>(spectralDriftPhase * 6.28318f));
// Then use morphMod instead of morphPosition when building spectral envelope
```

---

### 3. Overworld (XOlokun Adapter)

**Identity:** Chip synthesis — NES/FM Genesis/SNES and 3 more. ERA triangle crossfade.

**Why it fails D005:** The standalone XOverworld instrument has a fully-implemented ERA Drift LFO
(`ow_eraDriftRate`, range 0.0–4.0 Hz, 4 shapes: Orbit/Triangle/Pendulum/Wander) that continuously
modulates the ERA triangle position. However, the XOlokun adapter (`OverworldEngine.h`) does not
implement this functionality. The adapter passes `eraDriftRate/Depth/Shape` values to the
`xoverworld::VoicePool` via `applyParams(snap)`, but the `voicePool.applyParams()` method stores
snapshot values without advancing any phase accumulator. No `eraPhase` member exists in
`OverworldEngine`.

The standalone's ERA drift lives in `PluginProcessor.cpp` (lines 267–355), which advances
`eraPhase` at `snap.eraDriftRate * numSamples / sr` and applies orbit/triangle/pendulum/wander
shapes to `era`/`eraY` before passing them to the voice pool. This logic is entirely absent from
the XOlokun adapter.

**Existing infrastructure:** The adapter already has `eraSmooth` and `eraYSmooth` IIR smoothing
applied per-sample in `renderBlock`. Adding an `eraPhase` float and copying the ~60 lines of
drift logic from `PluginProcessor.cpp` would port the feature without touching the voice pool.

**Parameter that would benefit most:** ERA position. Slow orbital drift (0.03–0.1 Hz) on the ERA
triangle would make chip timbres "breathe" between eras — the nautilus drifting between its
own chambers.

**Minimum viable addition (in OverworldEngine state, 1 member):**
```cpp
float eraPhase = 0.0f;
```
Then, in `renderBlock` before the per-sample loop (3 lines, referencing the standalone logic):
```cpp
if (snap.eraDriftRate > 0.001f) {
    eraPhase = std::fmod(eraPhase + snap.eraDriftRate * numSamples / sr, 1.0f);
    targetEra += snap.eraDriftDepth * 0.35f * std::sin(eraPhase * 6.28318f);
}
```

---

### 4. Owlfish

**Identity:** Mixtur-Trautonium subharmonic oscillator + micro-granular + sacrificial armor burst.

**Why it fails D005:** `OwlfishVoice.h` contains no phase accumulator, LFO struct, or
rate-controlled oscillator beyond the audio-frequency `SubharmonicOsc`. All five processing stages
(portamento, subharmonic osc, compressor, filter, granular, armor) accept only per-block snapshot
values with no intra-block variation. The filter cutoff is static per block; the grain parameters
do not cycle.

**Existing infrastructure that could host an LFO:**
- `OwlfishVoice` maintains `currentFreq` as a per-sample smoothed value (portamento IIR). The same
  IIR smoothing structure applied to a 0.05 Hz target would create a pitch drift LFO at minimal
  cost
- `MicroGranular` has a `grainSize` parameter; a slow oscillation on `grainSize` (from 2 ms to 10
  ms at 0.05 Hz) would create the distinctive granular "breathing" that defines owlfish's abyssal
  character
- `OwlfishCytomicSVF` is configured once per block. A slow phase accumulator driving ±200 Hz on
  `cutoffHz` would add resonant movement without restructuring the signal chain

**Parameter that would benefit most:** Grain size. Owlfish's Mixtur-Trautonium oscillator already
generates a multi-layered subharmonic texture — adding a 0.05 Hz LFO on grain size from 2ms to
8ms would create a slowly pulsing granular density that feels like the owlfish's bioluminescent
pulse (the "sacrificial armor" aesthetic).

**Minimum viable addition (in OwlfishVoice, 1 member):**
```cpp
float grainLfoPhase = 0.0f;
```
In the per-sample loop of `OwlfishVoice::process()` (3 lines, before diet.setParams):
```cpp
grainLfoPhase += 0.05f / static_cast<float>(sampleRate);
if (grainLfoPhase > 1.0f) grainLfoPhase -= 1.0f;
float lfoGrainSize = snap.grainSize + 0.003f * std::sin(grainLfoPhase * 6.28318f); // ±3ms
diet.setParams(lfoGrainSize, snap.grainDensity, snap.grainPitch, snap.feedRate);
```

---

## PARTIAL Engines — Analysis

### Fat (Obese) — PARTIAL
**What it has:** `FatMojoDrift` — a per-oscillator smooth random walk at ~10 Hz (new target every
100ms, smoothed). Modulates pitch of all 13 oscillators. `FatEnvelope filterEnv` — modulates
filter cutoff on every voice (envelope depth parameter: `fat_fltEnvAmt`). **What it lacks:** No
LFO at rates ≤1 Hz. The MojoDrift operates at ~10 Hz (too fast to be a "breathing" modulation).

### Morph (OddOscar) — PARTIAL
**What it has:** A hardwired 0.3 Hz sine LFO (`lfoPhase`, constant `kCouplingLfoRateHz = 0.3`).
This LFO is exported on coupling channel 2 as "Oscar's slow breath" for partner engines to use.
**What it lacks:** The LFO does not modulate any of Morph's own synthesis parameters (oscillator
scan, filter, formant). It is a service LFO for other engines, not a self-breathing one.

### Organon — PARTIAL
**What it has:** `phasonClock` — a phase accumulator advancing at `lockedMetabolicRate` Hz
(range 0.1–10 Hz). It drives sinusoidal modulation of each voice's metabolic rate via
`phasonModulations[voiceIndex] = std::sin(kTwoPi * voicePhase) * phasonShift`. This DOES create
slowly pulsing timbral variation in the modal array. **What it lacks:** The phasonClock is a proxy
for metabolic pulsing, not a named LFO targeting a conventional synth parameter. Rate floor is
0.1 Hz (D005 asks for ≤0.01 Hz capability). Additionally, the modulation only fires when
`phasonShift > 0.001f`.

### Onset — PARTIAL
**What it has:** MUTATE macro (`perc_macro_mutate`) adds `mutateRng.process() * mMutate * 0.2f`
to blend and character per block. This is stochastic per-block drift, not an LFO. **What it
lacks:** No rate-controlled oscillator. The MUTATE drift operates at block-rate (typically 86–172
Hz) — far too fast to qualify as sub-1 Hz modulation in D005's sense. The random walk adds
character but not the slow "breathing" D005 targets.

### Ouroboros — PARTIAL
**What it has:** The `leashPhasorPhase` advances at audio pitch frequency — this is a fundamental
oscillator, not an LFO. The 3-topology chaotic attractor (Lorenz, Duffing, Chua) produces
"self-modulation" in the broadest sense: the output signal feeds back into itself through the chaos
equations, creating continuous spectral evolution. **What it lacks:** No independent sub-Hz
oscillator. The chaos is intrinsic to the signal generation, not a separate modulation layer. The
`couplingPitchModulation` field accepts external LFO input but generates none internally.

### Oblique — PARTIAL
**What it has:** A single phaser LFO (`lfoPhase`, `phaserParams.rate`, range 0.05–8 Hz) that
sweeps allpass filter stages to create a comb-notch sweep. This does make the sound "breathe."
**What it lacks:** The phaser LFO modulates a post-synthesis effect only; it does not modulate
any oscillator frequency, synthesis filter, or timbral parameter of the bouncing-ball oscillator
model itself. The synthesis chain is static per-block.

---

## Best Practice Reference

### 1. Bob (Oblong) — Exemplary

Bob's `BobCuriosityLFO` is the fleet's most sophisticated implementation:
- **LFO1** (user-controllable, 0.01–20 Hz, 4 shapes): routes to filter cutoff, pitch, oscillator
  wave shape, or FX depth via `lfo1Target` — covering all useful synthesis destinations
- **LFO2** (0.05 Hz hardwired): random sample-and-hold with `smooth2 += (snh2 - smooth2) * 0.0005f`
  — a deeply smoothed drift that operates independently of LFO1, ensuring the engine always has
  sub-1 Hz motion even with LFO1 off
- **5 Curiosity modes** (Sniff, Wander, Investigate, Twitch, Nap): each changes the behavioral
  character of how the curiosity output modulates the filter — making modulation feel like a
  personality trait, not a technical parameter
- **Per-voice offset** (`voiceOffset`): LFO1 and LFO2 phases are staggered across voices, so
  polyphonic playing creates natural ensemble movement rather than synchronous pulsing

**Key lesson:** Separate the user-controllable LFO (explicit, routeable) from a background drift
oscillator (always running, very slow). The latter is the "heartbeat" that D005 actually demands.

### 2. Opal — Best Mod Matrix Design

Opal's mod matrix is the fleet's most general:
- **2 LFOs** (0.01–20 Hz each, with tempo sync and retrigger options)
- **Filter envelope** as a third modulation source
- **12 mod destinations** routed via `applyModMatrix()` — including filter cutoff, shimmer,
  frost, grain scatter, and pitch
- **Sources include LFO1, LFO2, filter env, amp env, velocity, key track, mod wheel** — the mod
  wheel and key track are placeholders (source value = 0), but the architecture is forward-looking

**Key lesson:** Build the mod matrix generically from day one. `applyModMatrix()` in 27 lines
handles all routing without per-destination special cases. New destinations cost one array entry.

### 3. Osprey — Best Macro-to-LFO Integration

Osprey's `seaStateLFO` (Round 3B fix) demonstrates how to tie LFO rate to a macro for maximum
expressiveness:
- Rate formula: `lfoRateHz = 0.05f + macroMovement * 0.95f` — at M2 MOVEMENT=0, the sea breathes
  once every 20 seconds (0.05 Hz); at M2=1, it pulses at 1 Hz
- The LFO modulates `effectiveSeaState` — a master parameter that controls both the excitation
  energy and turbulence of the FluidEnergyModel
- Net result: the MOVEMENT macro changes both the *character* of the modulation (via seaState) and
  the *speed* of modulation (via LFO rate) simultaneously — one knob, two perceptual dimensions

**Key lesson:** Pair LFO rate to a macro so users have one knob that meaningfully controls the
"tempo" of organic motion. This is more expressive than an isolated rate parameter.

---

## Notes on D005 Threshold

D005 specifies rates "down to 0.01 Hz." A 0.01 Hz LFO produces a 100-second cycle — near the
theoretical minimum useful to a live performer. Several PASS engines include this floor:

- **Bob, Bite, Obscura, Obsidian, Oceanic, Oracle, Origami, Opal:** NormalisableRange minimum
  exactly `0.01f` Hz
- **Osprey, Drift, Oblique, Optic (drift):** minimum between 0.05–0.1 Hz — close but technically
  below the doctrine floor
- **Dub:** DubAnalogDrift uses a 0.5 Hz LP filter on noise, producing sub-1 Hz content — passes
  in spirit

The PARTIAL engines listed above produce modulation but at rates or in patterns that do not satisfy
the "slow, breathing, always-on" quality D005 intends. The 4 FAIL engines produce no autonomous
modulation at any rate.
