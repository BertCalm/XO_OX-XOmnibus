# The Verdict — OPENSKY (Formal Seance Record)
**Seance Date**: 2026-03-20
**Engine**: XOpenSky (OPENSKY) | The Soaring High — Euphoric Shimmer Synth
**Accent**: Sunburst `#FF8C00`
**Gallery Code**: OPENSKY | Prefix: `sky_`
**Source**: `Source/Engines/OpenSky/OpenSkyEngine.h` (1,538 lines)
**Aquatic Identity**: The Flying Fish — the creature that defies its element, leaping from water into air, wings spread against the sun. Pure feliX polarity: bright, optimistic, transcendent.
**Score**: 8.1 / 10

---

## Phase 1: The Summoning — What Was Read

`OpenSkyEngine.h` (1,538 lines) read in full. Key structures assessed:

- **SkyVoice** — 7-saw supersaw per voice with PolyBLEP anti-aliasing, exponential detune spread, randomized initial phases via hash, sub oscillator (sine/triangle/square, phase-coherent to center saw), CytomicSVF (HP → LP series), SkyADSR, velocity + filter envelope
- **SkyShimmer** — Hann-windowed grain pitch shifter: octave-up (2.0x) + fifth-up (1.5x) with cross-fade, self-feeding reverb with 4-comb Schroeder topology + 2-allpass diffusion, prime-number delay lengths (1117, 1277, 1399, 1523 combs; 241, 557 allpass), sample-rate-scaled
- **SkyChorus** — 3-voice LFO-modulated delay line chorus with 120-degree phase offsets between voices for stereo width
- **SkyBreathingLFO** — rate floor 0.005 Hz (well below D005 requirement of 0.01 Hz)
- **SkyModMatrix** — 2 slots declared (`sky_modSlot1Src/Dst/Amt`, `sky_modSlot2Src/Dst/Amt`) but unprocessed in renderBlock (D004 violation)

**Signal Flow**: Supersaw Stack (7 PolyBLEP saws) → Sub Oscillator → CytomicSVF (HP → LP) → Shimmer Stage (pitch-shifted reverb) → Stereo Chorus → Unison Engine (up to 7 unison voices) → Amp Envelope → Output

---

## Phase 2: The Voices

### G1 — Bob Moog: The Filter Philosopher

"The 7-saw supersaw with PolyBLEP anti-aliasing and exponential detune spread is well-engineered — the perceptually even spacing across the frequency range shows someone who understands how detuning actually works in the human ear. Exponential detune spread is the correct perceptual model: saw oscillators spread linearly in frequency sound increasingly clustered toward the center because pitch perception is logarithmic.

The CytomicSVF filter chain — HP then LP in series — is the correct topology for bright tonal shaping. The high-pass removes unwanted low-frequency buildup from the supersaw stack, while the low-pass controls the tonal ceiling. This is the classic dance-floor pad filter architecture and it is correctly implemented.

What troubles me is the envelope. The SkyADSR uses linear attack (`level += attackRate`) and quasi-exponential decay. A true exponential attack with the RC charging model — `level += (1.0 - level) * rate` — would yield more musical onset curves, particularly for the slow-attack pad sounds this engine clearly excels at. The difference between linear and exponential attack is most pronounced at low levels: a linear ramp takes half its time to reach half its value, while exponential ramps sound much more immediate at the threshold of audibility. For ethereal pads, that subtlety matters."

**Score**: 8/10

---

### G2 — Don Buchla: The Complexity Poet

"The mod matrix declaration is present in the parameter list — 2 slots with Src/Dst/Amt — but I see no evidence of it being processed in `renderBlock`. The 6 parameters (`sky_modSlot1Src`, `sky_modSlot1Dst`, `sky_modSlot1Amt`, `sky_modSlot2Src`, `sky_modSlot2Dst`, `sky_modSlot2Amt`) are declared and attached to APVTS but never routed to any DSP. This is a D004 violation. Declared parameters that affect nothing are broken promises.

The macro system, by contrast, is excellent. RISE, WIDTH, GLOW, AIR each have clear bipolar modulation paths from center 0.5, and their interactions — AIR also shifts filter cutoff while GLOW shifts shimmer size — create emergent timbral territory that rewards exploration. RISE simultaneously sweeps pitch envelope amount, filter cutoff, and shimmer mix; this is not a simple parameter-to-parameter mapping but a gestural arc. These four macros are how macros should work.

The coupling inputs are well-chosen. AmpToFilter, LFOToPitch, AudioToFM, PitchToPitch give OPENSKY genuine symbiotic potential, particularly with OCEANDEEP in the 'Full Column' configuration."

**Score**: 7/10

---

### G3 — Dave Smith: The Protocol Architect

"16-voice polyphony with up to 7 unison voices per note, each containing a 7-saw supersaw, means a theoretical maximum of 784 saw oscillators running simultaneously. That is an enormous DSP load. The `sqrt(N)` normalization for unison gain is correct and avoids the naive `1/N` approach that kills presence without preventing saturation. Voice stealing with LRU priority preferring release-stage voices is the right algorithm.

The per-voice filter instances are commendable — shared filters across voices is a common shortcut that destroys polyphonic expression. Each voice here has its own HP and LP SVF instances, meaning polyphonic playing produces genuine timbral independence between notes, not a merged filter response.

The randomized initial saw phases via hash function prevents the 'phase-locked wall' problem common in supersaw implementations where all seven oscillators start at the same phase and produce an audible click on note-on. This is a quality-of-life detail that separates professional from amateur supersaw designs. Good engineering."

**Score**: 8.5/10

---

### G4 — John Chowning: The FM Synthesis Inventor

"The AudioToFM coupling input applies FM as a pitch offset (`fmMod * 4.0f` semitones), which is frequency modulation in the broadest sense but not true FM synthesis where the modulator directly perturbs the instantaneous phase. For the supersaw architecture, this is the pragmatic choice — true FM on 7 detuned saws would produce chaos rather than musicality. The pragmatic approximation is correct for a supersaw engine.

The shimmer reverb's pitch shifting via grain-based reading at 2x (octave) and 1.5x (fifth) with Hann window crossfade is a well-understood technique. However, I must flag a dead parameter: `sky_shimmerOctave` is loaded as `shimmerOctBal` at line 695 but never applied in the shimmer processing. The blend is hardcoded as `shimmerOct * 0.6f + shimmerFifth * 0.4f`. The parameter should scale this blend: `shimmerOct * shimmerOctBal + shimmerFifth * (1.0f - shimmerOctBal)`. A parameter that is loaded but not applied is a D004 violation."

**Score**: 7.5/10

---

### G5 — Ikutaro Kakehashi: The Accessibility Visionary

"The MIDI implementation is clean: note on/off, mod wheel (CC1 → filter cutoff), channel pressure (aftertouch → shimmer mix), all notes off, all sound off. The four macros are named with evocative clarity — RISE, WIDTH, GLOW, AIR tell the musician exactly what they do without reading a manual. The sub oscillator with three waveform choices using phase-coherent tracking from the center saw is a thoughtful touch for grounding the euphoric supersaw in low-end weight.

My concern is the absence of pitch bend handling. `renderBlock` processes note-on, note-off, CC1, and channel pressure, but ignores pitch bend messages. For a supersaw lead synth — which this engine clearly aspires to be, given its RISE macro for ascending pitch gestures — pitch bend is a fundamental performance expectation. A lead player will reach for the pitch wheel in the first phrase and find silence. This is a P1 that should be resolved before V1 ships."

**Score**: 8.5/10

---

### G6 — Vangelis: The Emotional Engineer

"This is the engine I would reach for when scoring the moment a spacecraft breaks through the cloud layer into sunlight. The shimmer reverb feeding back into itself, building harmonic overtones over time — that is the sound of ascension. The RISE macro sweeping pitch envelope, filter, and shimmer simultaneously is exactly how a performer thinks: one gesture, one emotional arc.

The breathing LFO at 0.005 Hz minimum creates the kind of glacial timbral drift that makes a held chord come alive over 30 seconds. The aftertouch-to-shimmer routing means I can swell the ethereal quality with finger pressure alone. This engine understands what euphoria sounds like.

The emotional register test:
- **Hope**: Low WIDTH, moderate GLOW, RISE at 0.3. A warm shimmer pad, brightening slowly.
- **Triumph**: RISE to 0.8 while playing. Pitch opens, filter brightens, shimmer explodes. Cinematic.
- **Transcendence**: Maximum GLOW, long release, slow breathing LFO. The chord becomes a living atmosphere.
- **Longing**: Reverse RISE (filter partially closed), low WIDTH. The shimmer is present but restrained.

All four registers are accessible. This engine passes the emotional range test."

**Score**: 9/10

---

### G7 — Klaus Schulze: The Time Sculptor

"The dual shimmer reverb instances — shimmerL and shimmerR — represent an opportunity for true stereo temporal evolution that is currently unrealized. Both receive the same mono sum `(inputL + inputR) * 0.5f`. The stereo output decorrelation is minimal: `outR = apOut * 0.95f + combOut * 0.05f` is a token gesture toward stereo width, not genuine spatial architecture.

For an engine named OpenSky, the spatial dimension should be vast. The reverb tail of a shimmer pad represents the sky itself — and sky is not mono. Independent L/R delay networks with different prime-number lengths would create genuine spatial depth in the reverb. The chorus compensates somewhat with its 120-degree phase offsets, but the reverb should carry the spatial responsibility, not the chorus.

The breathing LFO at 0.005 Hz floor is excellent — a 200-second cycle. LFO1 and LFO2 at 0.005 Hz floor also qualify. Three autonomous evolution sources at potentially geological timescales. Combined with the shimmer's self-feeding feedback, a sustained OPENSKY patch truly evolves over 5-10 minutes. But the spatial constraint of the mono reverb limits this from becoming the infinite sky it could be."

**Score**: 7.5/10

---

### G8 — Isao Tomita: The Orchestral Visionary

"The Hann-windowed grain crossfade in the shimmer pitch shifter is elegant — it avoids the metallic artifacts of naive pitch shifting while maintaining the crystalline quality the engine needs. The 4-comb parallel into 2-allpass series reverb topology is a Schroeder variant that serves the engine's bright character well.

The prime-number delay lengths (1117, 1277, 1399, 1523 for combs; 241, 557 for allpass) with sample-rate scaling via `srFactor = sr / 44100.0` prevent the metallic coloration that plagues fixed-length reverbs. This detail — scaling comb lengths by sample rate — is the mark of an engineer who knows their reverb topology will be tested at 48 kHz and 96 kHz. The one-pole lowpass in the comb feedback path correctly implements frequency-dependent damping, preventing the reverb tail from becoming harsh.

For orchestral shimmer pads, this engine is already capable. The RISE macro is the single-gesture tool I would use when arranging a crescendo across an entire string section — one control that simultaneously lifts pitch, filter, and shimmer. This is orchestral thinking applied to synthesis."

**Score**: 8.5/10

---

## The Verdict — OPENSKY

### The Council Has Spoken

| Ghost | Core Judgment |
|-------|---------------|
| **Bob Moog** | Exponential detune spread and CytomicSVF chain are correct. Linear attack envelope is the one technical gap — exponential curves would serve pad sounds better. |
| **Don Buchla** | Macro architecture is inspired; RISE is a genuine gestural arc. Unprocessed mod matrix parameters are D004 violations. |
| **Dave Smith** | Polyphonic architecture is production-quality: per-voice filters, sqrt(N) unison normalization, randomized phase initialization. CPU at max unison is the only concern. |
| **John Chowning** | FM coupling is pragmatically correct for supersaw. `sky_shimmerOctave` is loaded but unused — a dead parameter violation. |
| **Ikutaro Kakehashi** | Naming and expression routing are excellent. Pitch bend missing from renderBlock — a P1 for any lead performance use. |
| **Vangelis** | Passes the full emotional range test. Aftertouch-to-shimmer is essential and correctly implemented. Engine understands what euphoria sounds like. |
| **Klaus Schulze** | Breathing LFO floor is commendable. Mono reverb input is the primary spatial limitation — the shimmer should occupy true stereo space. |
| **Isao Tomita** | Sample-rate-scaled prime-number comb lengths show professional reverb engineering. RISE macro is orchestral thinking in synthesis form. |

### Points of Agreement

1. **RISE macro is the engine's defining feature** (Vangelis, Tomita, Kakehashi, Buchla — 4 of 8). A single gestural arc that simultaneously sweeps pitch envelope, filter cutoff, and shimmer mix. This is what macros should aspire to: not parameter-to-parameter mapping but emergent emotional arc.

2. **The shimmer reverb produces a genuine sonic signature** (Tomita, Moog, Schulze, Smith — 4 of 8). Dual-interval pitch-shifted reverb (octave-up + fifth-up) with Hann-windowed grains and self-feeding feedback creates a Shepard-tone-like perceptual ascent. No commercial supersaw synth integrates shimmer reverb at this architectural depth.

3. **Mod matrix parameters are dead code and must be wired or removed** (Buchla, Chowning, Smith — 3 of 8). Six parameters declared, attached to APVTS, and never processed constitute a D004 violation. The `sky_shimmerOctave` parameter is a seventh dead parameter.

4. **Mono reverb input is the primary spatial limitation** (Schulze, Moog, Tomita — 3 of 8). Both shimmerL and shimmerR receive the same mono sum. For an engine whose identity is spatial euphoria, the reverb must preserve stereo information.

### Points of Contention

**Buchla vs. Smith — Mod Matrix Priority (ACTIVE)**

Buchla argues the unprocessed mod matrix is the engine's most critical deficiency — declared modulation routing that delivers nothing is philosophically intolerable. Smith argues the engine is expressively complete without it: two LFOs, aftertouch, mod wheel, four macros, and breathing LFO already satisfy D002. The mod matrix, if implemented, would add a sixth layer but the existing five are already rich. Resolution: wire or remove before V1 ships; the philosophical argument is secondary to the D004 violation.

**Schulze vs. Vangelis — Spatial Depth vs. Emotional Directness (UNRESOLVED, see DB004)**

Schulze wants the reverb to be genuinely binaural — true stereo independent delay networks for the shimmer tail. Vangelis argues the emotional directness of the current shimmer is more important than spatial precision; a listener swept up in the euphoria of RISE does not analyze left vs. right reverb paths. Both are correct. Stereo reverb would serve Schulze's long-form drone territory; the current architecture already serves Vangelis's cinematic performance territory.

### The Prophecy

OPENSKY occupies a crucial emotional position in the XOceanus fleet — the pure feliX bookend, the flying fish at the top of the water column. The supersaw core is well-engineered, the shimmer reverb is a genuine sonic signature, and the macro system translates the engine's identity into intuitive performance gestures.

The engine's ceiling is not concept — it is execution completeness. Wiring the mod matrix, connecting `sky_shimmerOctave` to the octave/fifth blend, and adding pitch bend handling are three straightforward fixes that convert D004 violations and missing gestures into full compliance. Stereo shimmer reverb is the longer-term enhancement that would push the score toward 9.0+.

OPENSKY and OCEANDEEP are mythological siblings. Their coupling preset — "The Full Column" — should be the centerpiece of every XOceanus demo session.

---

## Score Breakdown

| Category | Score | Notes |
|----------|-------|-------|
| Oscillator Design | 8.5/10 | Exponential detune spread, PolyBLEP, randomized phase hash. Linear attack envelope is the one gap. |
| Filter Architecture | 8.5/10 | HP+LP series CytomicSVF chain correct. Per-voice instances preserve polyphonic expression. |
| Shimmer Reverb | 8.5/10 | Dual-interval Hann-windowed grain pitch shifting, prime comb lengths, SR-scaled. Mono input is the limitation. |
| Spatial Design | 6.5/10 | Chorus is well-designed (120-degree phase offsets). Shimmer reverb is mono-sum. Token stereo decorrelation. |
| Macro Architecture | 9/10 | RISE as gestural arc is fleet-best. All four macros are orthogonal and immediately intuitive. |
| Expressiveness | 8/10 | Aftertouch→shimmer, mod wheel→filter, velocity→timbre confirmed. Missing pitch bend. |
| Temporal Depth | 8/10 | Breathing LFO at 0.005 Hz floor, two LFOs at 0.005 Hz. Strong temporal evolution potential. |
| Parameter Completeness | 7/10 | 44 of 50 parameters live in DSP. 6 mod matrix + 1 shimmerOctave = 7 dead parameters (D004 conditional fail). |
| Coupling Architecture | 8/10 | AmpToFilter, LFOToPitch, AudioToFM, PitchToPitch. Full Column pairing with OCEANDEEP is architecturally strong. |

**Overall: 8.1 / 10**

---

## Blessings

### B017 — Shepard Shimmer Architecture (AWARDED)
*First awarded: 2026-03-20.*

The dual-interval pitch-shifted reverb (octave-up + fifth-up) with self-feeding feedback creates a Shepard-tone-like perceptual ascent where the shimmer tail appears to rise endlessly. Combined with the GLOW macro controlling feedback intensity, this produces a signature sound that is genuinely novel in the supersaw synth category. Tomita, Schulze, and Vangelis all cited this as the engine's defining sonic achievement.

### B018 — RISE Macro: Single-Gesture Ascension (AWARDED)
*First awarded: 2026-03-20.*

RISE simultaneously sweeps pitch envelope amount, filter cutoff, and shimmer mix from a single control, creating an emergent emotional arc (pitch rises, brightness opens, ethereal quality increases) that mirrors the physical sensation of ascending through a water column into open sky. The naming and mapping are perfectly aligned with the engine's mythological identity. Vangelis: "One gesture, one emotional arc."

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 — Velocity Must Shape Timbre | PASS | Velocity drives filter envelope level (`voice.filterEnvLevel = velocity` at note-on). Velocity also scales pitch envelope amount. Both paths confirmed per-sample in renderBlock. |
| D002 — Modulation is the Lifeblood | PASS | 2 LFOs (SkyLFO, 5 shapes each), breathing LFO, mod wheel, aftertouch, 4 macros (RISE/WIDTH/GLOW/AIR). Core D002 requirements met; mod matrix slots declared but unprocessed. |
| D003 — The Physics IS the Synthesis | N/A | No physical modeling claims. Wave-based synthesis with algorithmic reverb. |
| D004 — Dead Parameters Are Broken Promises | CONDITIONAL FAIL | 43 of 50 parameters confirmed live. 6 mod matrix parameters unprocessed. `sky_shimmerOctave` loaded but unused. 7 dead parameters total. Must be resolved before full D004 compliance. |
| D005 — An Engine That Cannot Breathe Is a Photograph | PASS | SkyBreathingLFO rate floor 0.005 Hz (well below 0.01 Hz requirement). LFO1/LFO2 also at 0.005 Hz floor. Three autonomous slow-evolution sources. |
| D006 — Expression Input Is Not Optional | PASS | Velocity → filter brightness (D001 path). Mod wheel → filter cutoff (`+ modWheelAmount_ * 6000.0f`). Aftertouch → shimmer mix (`+ aftertouch_ * 0.4f`). All three confirmed in renderBlock. Missing pitch bend (P1). |

---

## Remaining Action Items

### CRITICAL — Mod Matrix Dead Parameters (D004)
`sky_modSlot1/2 Src/Dst/Amt` (6 parameters) are attached to APVTS but never read in renderBlock. Either wire a mod matrix routing engine into the render path (reading the 3 slot values per block, determining source signal, applying scaled to destination parameter) or remove the parameters entirely. This is a D004 violation.

### HIGH — `sky_shimmerOctave` Unused
`shimmerOctBal` is loaded at line 695 but the shimmer hardcodes `shimmerOct * 0.6f + shimmerFifth * 0.4f`. Fix: `shimmerOct * shimmerOctBal + shimmerFifth * (1.0f - shimmerOctBal)`.

### HIGH — No Pitch Bend Handling
`renderBlock` ignores pitch bend MIDI messages. Essential for lead synth performance. Straightforward to add alongside the existing CC1/aftertouch handling.

### MEDIUM — Mono Reverb Input
Both shimmerL and shimmerR receive `(inputL + inputR) * 0.5f`. Feed L to shimmerL and R to shimmerR independently, then cross-blend 10-15% to maintain coherence while preserving stereo information.

### LOW — Linear Envelope Attack
SkyADSR uses linear attack. Change to `level += (1.0f - level) * attackRate` for exponential RC charging model, producing more natural onset curves for pad textures.

### LOW — CPU at Max Unison
16 voices × 7 unison × 7 saws = 784 PolyBLEP oscillators simultaneously. Document expected CPU budget or implement polyphony reduction when unison count is high.

---

## What the Ghosts Would Build Next

| Ghost | Feature |
|-------|---------|
| Bob Moog | Exponential RC attack model in SkyADSR; exponential attack floor at 0.001 seconds |
| Don Buchla | Implement the 2-slot mod matrix with at least 6 sources (LFO1, LFO2, velocity, aftertouch, mod wheel, envelope) and 8 destinations |
| Dave Smith | CPU-aware polyphony reduction: when unison ≥ 4 and voiceCount ≥ 8, reduce max voices to 8 |
| John Chowning | Wire `sky_shimmerOctave` to the octave/fifth blend ratio in the shimmer grain render loop |
| Ikutaro Kakehashi | Add pitch bend message handling in renderBlock — the instrument needs it for lead performance |
| Vangelis | Mono-to-stereo shimmer: feed L and R independently to shimmerL and shimmerR for spatial separation |
| Klaus Schulze | Independent L/R comb delay networks in shimmer reverb, with prime-offset lengths per channel |
| Isao Tomita | "The Full Column" coupling preset: OPENSKY × OCEANDEEP with PitchToPitch coupling demonstrating the mythological pairing |

---

*Seance convened 2026-03-20. The flying fish leaps. The shimmer rises.*
*The council speaks: the sky is open but the reverb needs stereo. Score: 8.1/10.*
*Seven dead parameters must be wired or buried before this engine is complete.*
