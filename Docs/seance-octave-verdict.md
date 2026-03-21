# Synth Seance — XOctave Verdict

**Engine**: XOctave (OCTAVE) — "The Four Cathedrals"
**Date**: 2026-03-21
**Source**: `Source/Engines/Octave/OctaveEngine.h`
**Accent Color**: Bordeaux `#6B2D3E`
**Parameter Prefix**: `oct_`
**Chef Quad Role**: Octave (Western Europe / Arturia wildcard / SW quadrant)
**Presets Reviewed**: 22 total (Bordeaux Cathedral, Garage Organ, Tracker Action, Rue Mouffetard, Stone Breath, Nave Infinity, Bellows Fury, Transistor Fuzz, Valse Brillante, Schnitger Glass + 12 coupling/entangled presets)

---

## The Eight Ghosts

Aristide Cavaille-Coll · Arp Schnitger · Paolo Soprani · Silvio Nascimbeni · Robert Moog · Wendy Carlos · Raymond Scott · Isao Tomita

---

## Ghost Perspectives

---

### 1. Aristide Cavaille-Coll (1811-1899) — Romantic Pipe Organ Builder

*"I spent forty years building the instrument that fills this stone room. Let me hear if you have understood what fills the stone."*

The Cavaille-Coll model is — I must be honest — deeply respectful. Twelve drawbar partials sourced from Audsley (1905) and Jaffe & Smith (1983). The registration scheme dividing 8', 4', and 2' ranks across the partial array is correct in spirit even if it is a simplification of a real multi-rank instrument with separate windchests. The attack multiplier of 3× is a proper nod to the slow "speech" of a large Romantic pipe — air takes time to fill a 16-foot bourdon.

The wind noise, `OctaveWindNoise`, is present and runs continuously as it should. The room resonance at 120 Hz, 380 Hz, and 1200 Hz represents the three principal body modes of a cathedral — architecturally reasonable.

What troubles me: the room resonance is a per-voice object. In reality, the acoustic space is shared. This means eight voices accumulate eight independent room models that should be one. The result will smear rather than blend. The `OctaveRoomResonance` should be global, or at minimum its coefficients should be identical (they are, via `prepare()`), but the state is per-voice, which is both physically wrong and potentially expensive.

Also: my great organs were for counterpoint and full-choir registration. There is no way to lock registration per-partial rank independently (no true drawbar control). The `oct_registration` parameter sweeps the entire 8'/4'/2' blend as a single bipolar axis. This is an acceptable simplification, but it forecloses the expressive range of registration that made my instruments distinctive.

Score contribution: **8.3** — historically informed, architecturally present, but spatially incorrect room model and no true multi-rank control.

---

### 2. Arp Schnitger (1648-1719) — Baroque Organ Master

*"The tracker action is the soul. Without mechanical directness, the organist has nothing. Let me examine this chiff."*

The `OctaveChiffGenerator` is a genuine attempt. Half-sine windowed noise burst, duration derived from chiff amount (5-30ms inverse relationship to brightness), LP-filtered to pipe resonance. The filter coefficient uses the correct `exp(-2π·fc/sr)` matched-Z formula. The Baroque partial amplitudes (`kBaroquePartialAmps`) give proper weight to the 2nd and 4th harmonics over the fundamental — this is correct for a mixture-rich Schnitger principal chorus.

The chiff trigger per model is thoughtfully weighted: `chiffWeights[4] = { 0.3f, 1.0f, 0.0f, 0.0f }`. The Baroque positiv gets full chiff amplitude; the Romantic gets 30% (a "bloom" rather than a click); Musette and Farfisa get zero (correct — reeds and transistors do not chiff).

What is missing: a tracker action has *mechanical latency variation* per note — this engine has none. Also, the Baroque model shares partial phases reset to zero on every note-on. Real tracker organs have pipe speech that begins from random phase. But the chiff generator's noise LCG seed is seeded from frequency (`baseFreq * 1000 + 54321`), which means the same pitch always gets the same chiff character. This is a limitation but not a catastrophe.

The chiff noise LCG constant 1664525 / 1013904223 is the Numerical Recipes standard (correct). The filter uses `filterState += filterCoeff * (out - filterState)` — a first-order RC approximation, fine for transient shaping.

Score contribution: **8.0** — the chiff is genuine. Minor complaint: no per-pipe stochastic speech variation, no mechanical latency jitter.

---

### 3. Paolo Soprani (1844-1918) — French Musette Accordion Pioneer

*"I made reeds from bronze. They beat against each other to make the Parisian heart ache. Tell me what you have done with three reeds."*

The Musette model is the most evocative of the three I can comment on. Three oscillators detuned at ±`detuneNow` Hz from center — `detuneNow = 1 + detuneNow * 8 Hz` gives a 1–9 Hz beating range. This is physically accurate: a French musette has a "wet" tuning where the center reed is flanked by sharp and flat reeds to create the characteristic tremolo.

The reed waveform is credible: fundamental + 3rd (0.33) + 5th (0.15) + 7th (0.08). This is an odd-harmonic mixture, which is indeed what a free reed produces when clamped at both ends (like a clarinet reed, but bisonoric). The scaling factors are approximate but musically appropriate.

The bellows dynamics — `bellows = velocity * 0.6 + pressure * 0.4` — is thoughtful. Accordion players speak of "bellows drive" as distinct from key velocity. The aftertouch feeding `effectivePressure` which then feeds bellows is the correct mapping. This is performatively alive.

The buzz saturation path `fastTanh(sample * (3 + buzzAmt * 8)) * buzzAmt * 0.3` will produce reed rattle at high pressure. Genuine. I am not displeased.

What is absent: a real musette has a cassotto (inner chamber) on the left side that darkens the bass reeds. There is no spectral darkening by register here. Also: the three reeds should have slightly different waveforms (the two outer reeds are slightly less complex due to shorter length) — the model gives all three identical waveforms. Minor authenticity gap.

Score contribution: **8.5** — triple-reed beating works. Bellows physics are alive. Cassotto character and reed differentiation absent.

---

### 4. Silvio Nascimbeni (Farfisa — 1964 era) — Transistor Organ Architect

*"We built something deliberately cheap and deliberately buzzy. Cheap in material, rich in character. The square wave is the sound of freedom from the pipe. Did you honor it?"*

The Farfisa model uses `PolyBLEP::Square` — correct choice. Bandlimited square is what the original circuit produced (square oscillators into a bandpass ladder). The vibrato at exactly 5.5 Hz is a deliberate historical detail (the Compact Deluxe ran at approximately this rate). Using `detuneNow * 0.015` for vibrato depth is modest and appropriate.

The octave-up simulation via `partialPhases[0]` as a naive hard square (polarity flip at 0.5) is an approximation. Real Farfisa had a divide-by-2 circuit that produced a clean octave above. The naive square from phase accumulation will alias slightly at high notes — but this is a minor error and arguably adds to the "transistor" character.

The buzz saturation `fastTanh(sample * (1 + buzz * 6)) * 0.8` — the drive range 1-7× with tanh softclipping is correct for transistor saturation modeling. Farfisa players loved to push the input stage.

The registration for Farfisa uses the octave-up blended at `regNow * 0.3` — this gives some tab-register emulation but it is simplistic. A real Farfisa had 8 tab switches (Flute 16', Flute 8', Reed 8', etc.) each independently togglable. The single `registration` axis flattening these to a blend is a significant oversimplification.

No room resonance on Farfisa — correct. No chiff — correct. Immediate attack floor of 0.001s — correct.

Score contribution: **8.2** — spirit and character captured. Vibrato hardcoded correctly. Tab system oversimplified, octave-up aliasing potential.

---

### 5. Robert Moog (1934-2005) — Synthesizer Pioneer / Voice Architecture

*"Four different synthesis architectures under one parameter set. I want to know: does the architecture hold? Are the parameters honest?"*

Structurally, the Chef parameter vocabulary is elegant. Six parameters (cluster, chiff, detune, buzz, pressure, crosstalk) that mean different things to each model — this is parameter polymorphism done right. `Cluster` on Cavaille-Coll is voice-spread detuning (±15 cents spread across 8 voices). On Musette it is unused. On Farfisa it is unused. So `cluster` is really only meaningful in one of four models. That is a missed opportunity: a "cluster" density for Farfisa could stack register tabs, and for Baroque it could engage mixture stops.

The `oct_competition` parameter is declared, attached, but never consumed in the DSP. This is a D004 violation — a dead parameter is a broken promise. I invented Minimoog because every knob had to mean something.

The `macroCoupling` variable is loaded from `paramMacroCoupling` at line 416 but is never applied to any DSP path — it is loaded into `const float macroCoupling` and then the block continues without using it. `macroCoupling` does not appear in any `effectiveXxx` computation. This is a second D004 violation.

The modulation architecture (2 LFOs, aftertouch, mod wheel, 4 macros) is solid otherwise. LFO1 → filter brightness is well-placed. LFO2 → vibrato (organist's tremulant) is historically accurate. Default lfo2Depth is 0.0, which means vibrato is silent until the player moves it — this is correct for authentic organ performance.

Score contribution: **7.8** — elegance of concept slightly undermined by two dead DSP parameters.

---

### 6. Wendy Carlos (1939-) — Electronic Music Pioneer / Synthesis Precision

*"I transposed J.S. Bach onto a Moog and heard every voice clearly. I need to hear if Octave's voices can coexist with the same clarity."*

The voice architecture has an important architectural concern: `voice.ampEnv.setADSR(effectiveAttack, pDecay, pSustain, pRelease)` is called every sample inside the per-voice loop (line 681). This sets ADSR parameters on each sample, not once per block — and `StandardADSR::setADSR` almost certainly updates internal coefficients that should not change per-sample. If `setADSR` recomputes exponential time constants on every call, this is extremely expensive. Looking at the call pattern: effectiveAttack derives from a per-block constant, so the ADSR values don't actually change per-sample, but the per-sample `setADSR` call is a pattern risk.

The stereo spread logic is excellent: `panAngle = (note % 12) / 12.0 * 0.6 + 0.5` — notes spread across the keyboard get different pan positions via chromatic circle spread. For a pipe organ, this is accurate (pipes are physically spread across the width of the organ loft). For Farfisa this feels arbitrary, but it is a consistent system.

The phase reset to zero on every note-on (`partialPhases.fill(0.0f)`) means all voices start at identical phase. For polyphonic playing this creates a subtle "phasing" artifact when two voices of the same pitch play simultaneously. Real organ pipes maintain continuous air flow and random phase; digital additive synthesis should accumulate phase without reset for sustained tones. For the Baroque chiff mode, the reset is fine (the chiff masks it), but for legato Cavaille-Coll playing it creates a "click" when the note restarts. Using `v.glide.snapTo(freq)` without phase continuity is a missed refinement.

LFO phases are staggered at initialization: lfo1 offset by `i/8`, lfo2 by `i * 0.37` (golden ratio spread). This is sophisticated and correct — eight voices at staggered LFO phases will give natural ensemble width rather than synchronized tremulant.

Score contribution: **8.1** — elegant stereo model, LFO stagger correct, per-sample ADSR setting is a potential performance issue, phase reset is a legato accuracy concern.

---

### 7. Raymond Scott (1908-1994) — Electronic Music Inventor / Mechanist

*"I built machines that made sounds no human could control directly. Your Farfisa model — can it surprise? Can it glitch? Can it exceed its own design?"*

The Farfisa buzz path is the most generative element in this engine. With `buzz = 0.9` and `pressure = 0.9`, the tanh saturation drives hard enough to produce non-linear spectra. The interplay between PolyBLEP square + registration octave-up + tanh clipping creates what could be interesting aliasing harmonics that no single clean oscillator would produce.

But the engine is fundamentally conservative in its modulation architecture. There are no self-oscillating states, no feedback paths between models, no way to get the Farfisa into "runaway" territory. The `couplingOrganMod` value — which arrives via `EnvToMorph` coupling — is accepted (line 357) but then immediately zeroed at line 460 and is never applied to organ model morphing. This is the `couplingOrganMod` dead-end problem: coupling input is accepted but discarded.

The `oct_competition` parameter was clearly designed for the Chef Quad adversarial coupling system — but the competition parameter is entirely unrealized. In the Chef Quad vision, when two chefs compete, their competition score should influence voice stealing, amplitude, or register priority. Currently it is a placeholder with no effect.

Score contribution: **7.6** — the engine plays within strict walls. Competition and morphing coupling are unexploited design intentions.

---

### 8. Isao Tomita (1932-2016) — Synthesizer Orchestrator / Atmospheric Pioneer

*"I made Debussy breathe through circuitry. I made Holst roar through oscillators. This engine — can it fill the space between notes? Can it sustain the cathedral inside a computer?"*

The atmosphere of this engine is its greatest strength. The combination of:
- Continuous wind noise at 2-5% amplitude
- Slow LFO1 at 0.01 Hz (the 0.005 Hz floor is below D005's 0.01 Hz minimum — actually compliant!)
- Room resonance at three cathedral modes (120, 380, 1200 Hz)
- Long release times (up to 10 seconds)

...creates something that sounds genuinely inhabited. The "Stone Breath" preset (Cavaille-Coll, roomDepth 0.9, attack 0.5s, release 5s, lfo1Rate 0.02 Hz) is exactly what I would have programmed for a Debussy transcription — the notes dissolve into the room before the next phrase arrives.

The "Nave Infinity" preset (cluster 0.5, roomDepth 1.0, attack 1s, release 8s, lfo2Rate 0.005 Hz) is extraordinary in concept. Voice cluster at ±15 cents spread across 8 voices with a full second of attack — this will bloom and hover in a way that rewards patience.

But there is a hole: the Atmosphere and Aether presets push toward infinite sustain, and the engine has no feedback mechanism, no self-evolution during the sustain phase (beyond LFO). A "Stone Breath" should actually breathe — subtle amplitude variation from a very slow LFO or from pressure drift. The preset sets `lfo1Depth: 0.12` which helps, but the wind noise is hardcoded at `0.02 + pressure * 0.03` — it does not modulate with the LFO, so the "breath" is only in the filter, not in the presence of air itself.

Score contribution: **8.6** — beautiful atmospheric architecture. Wind noise could be dynamically coupled to LFO for true breathing. Sustain evolution limited to filter LFO.

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 — Velocity shapes timbre | PASS | Cavaille-Coll: `velBright` scales upper partial amplitudes. Baroque: `velBright = 0.7 + velocity * 0.3 * (harmonicNum/12)`. Musette: `bellows = velocity * 0.6 + pressure * 0.4`. Filter envelope `* voice.velocity` at line 686. All four models shape timbre via velocity. |
| D002 — 2+ LFOs, mod wheel, aftertouch, 4 macros | PASS | LFO1 (filter/brightness), LFO2 (vibrato/tremulant). Mod wheel → registration. Aftertouch → pressure + brightness. 4 macros (CHARACTER, MOVEMENT, COUPLING, SPACE). **Partial concern**: LFO2 depth defaults to 0.0 — vibrato off by default. Correct for organ authenticity but reduces default expressiveness. |
| D003 — Physics rigor | PASS | Audsley (1905), Jaffe & Smith (1983) cited for partial amplitudes. Nolle (1979) cited for chiff. Millot et al. (2001) for accordion beating. Filter coefficients use matched-Z `exp(-2π·fc/sr)`. |
| D004 — All declared parameters wired | **PARTIAL FAIL** | `oct_competition` is declared, `attachParameters` stores it, but it is never read in `renderBlock` or any DSP code. `macroCoupling` is loaded at line 416 but never applied — no `effectiveCoupling` or similar is computed from it. Two dead parameters. |
| D005 — LFO rate floor ≤ 0.01 Hz | PASS | `oct_lfo1Rate` min: 0.005 Hz. `oct_lfo2Rate` min: 0.005 Hz. Both below the 0.01 Hz doctrine floor. Compliant. |
| D006 — Expression input not optional | PASS | Aftertouch → pressure (line 428), aftertouch → brightness (line 434). Mod wheel → registration (line 431). Both are musically correct mappings (aftertouch = bellows pressure for reeds, mod wheel = swell pedal for pipe organs). |

**Doctrine Score**: 5 PASS, 1 PARTIAL FAIL (D004), 0 FAIL

---

## Eight-Dimension Scoring

| Dimension | Score | Notes |
|-----------|-------|-------|
| **DSP Architecture** | 8.3 | Four genuinely distinct synthesis models in one engine. Additive, triple-reed beating, PolyBLEP square. Chiff transient is real. Per-voice room resonance is spatially incorrect but functional. |
| **Historical Authenticity** | 8.5 | Citation depth is fleet-best for an organ engine. Cavaille-Coll partial amplitudes are sourced. Chiff is cited. Reed beating physics is cited. Farfisa vibrato at 5.5 Hz is exact. Minor: no cassotto, no multi-rank drawbar independence, Farfisa tabs oversimplified. |
| **Doctrine Compliance** | 7.5 | D001/D002/D003/D005/D006 all pass. D004 fails on two counts: `oct_competition` and `macroCoupling` are dead parameters. This is a real deficiency, not a technicality. |
| **Modulation Richness** | 7.8 | Two LFOs, aftertouch, mod wheel, 4 macros. The Chef params (cluster, chiff, detune, buzz, pressure, crosstalk) add a layer of organic control. Missing: mod matrix, no LFO→buzz routing, LFO2 defaults silent, competition coupling unrealized. |
| **Preset Quality** | 8.4 | 22 presets across all four models and all moods. Names are specific and evocative (Bordeaux Cathedral, Tracker Action, Rue Mouffetard, Schnitger Glass). DNA assignments are accurate. Presets cover the full spectrum from dark Romantic pad to aggressive Farfisa fuzz. Preset count is moderate (22) relative to fleet leaders. |
| **Sonic Identity** | 8.7 | The four-model concept is the engine's most distinctive feature in the fleet. No other engine offers this span from Baroque counterpoint to garage rock transistor organ within a single instrument. The Chef identity (classically trained, structurally precise) is embodied. |
| **Coupling Integration** | 6.8 | Four coupling types accepted: AmpToFilter, LFOToPitch, AmpToPitch, EnvToMorph. `EnvToMorph` accumulates `couplingOrganMod` but the accumulated value is immediately zeroed at block end and never applied to organ model morphing. `couplingOrganMod` is a dead coupling input. This limits the Chef Quad adversarial potential significantly. |
| **Performance Architecture** | 8.1 | 8 voices. Silence gate present. ParameterSmoothers for all hot-path parameters. LFO phase stagger for ensemble depth. `ampEnv.setADSR()` called per-sample (inefficiency risk depending on implementation). Phase reset on note-on rather than phase accumulation. |

**Weighted Average: 8.01 / 10**

---

## Critical Findings

### FINDING 1 — D004 Violation: `oct_competition` is a dead parameter

`oct_competition` is declared at line 860, attached at line 894, stored at line 942, but never loaded or used in `renderBlock`. Every preset has `oct_competition: 0.0`. This parameter exists because the Chef Quad adversarial coupling system was designed to use it, but the implementation was deferred. It is a broken promise.

**Fix**: Either wire competition to voice stealing priority (louder pressure = more voices granted), or to an amplitude duck when another Chef Quad engine is coupled, or remove the parameter until the Chef Quad adversarial system is built.

### FINDING 2 — D004 Violation: `macroCoupling` loaded but unused

`const float macroCoupling = loadP(paramMacroCoupling, 0.0f)` at line 416. This value is never referenced again in the render block. All other macros are applied: CHARACTER → pressure + brightness, MOVEMENT → registration, SPACE → room depth. COUPLING has no target.

**Fix**: Apply `macroCoupling` to `couplingOrganMod` (treat the COUPLING macro as an internal organ model blend toward a second model), or to crosstalk depth (higher coupling = more voice bleeding between organs), or to LFO cross-depth (coupling macro modulates LFO2 depth as tremulant intensity).

### FINDING 3 — `couplingOrganMod` accepted but silently discarded

`CouplingType::EnvToMorph` accumulates into `couplingOrganMod` (line 357). The intent was clearly organ model morphing via coupling (e.g., a coupled envelope could gradually blend from Baroque to Romantic). But `couplingOrganMod` is used nowhere in the synthesis loop — it accumulates, then is reset to zero at line 460. The `organModel` integer is read from `paramOrgan` only, not modified by coupling.

**Fix**: Use `couplingOrganMod` to morph between adjacent organ models, or map it to blend `kCCPartialAmps` vs `kBaroquePartialAmps` for smooth Cavaille-Coll↔Baroque transition under coupling.

### FINDING 4 — Per-voice room resonance (spatial incorrectness)

`OctaveRoomResonance` is a per-voice object. Eight simultaneous voices process through eight independent room resonators. The room state diverges across voices, creating eight different reflection patterns. A cathedral has one acoustic space. The additive mixing of eight independently-filtered voices does not correspond to any physical acoustic model.

**Fix**: Move room resonance to a single global post-mix object, or sum all voice outputs first, then apply one room resonator to the mix. This is a CPU optimization as well as an accuracy fix.

### FINDING 5 — Preset coverage gaps: no Family or Submerged presets

There are no Octave-native presets in Family or Submerged moods. The 22 presets cover Foundation, Atmosphere, Aether, Flux, Prism, and Entangled (coupling). Family (heritage, nostalgia, warmth) is a natural fit for a French musette accordion — a "Sunday Afternoon" or "Aunt Irma's Parlor" preset would be immediately compelling. Submerged could use the Cavaille-Coll at maximum room depth and minimal brightness — "Drowned Cathedral."

---

## Consensus Verdict

XOctave is the fleet's most historically specific engine. The combination of four genuine organ synthesis architectures — each with distinct DSP character, correct attack curves, and model-weighted parameter behavior — represents serious craft. The chiff transient is one of the most nuanced single-feature implementations in the entire fleet. The Chef identity as a classically trained, structurally precise figure is embodied: notes are placed with music-theory intent, the partial amplitudes are sourced from real organ acoustics literature, and the Musette bellows dynamics are physically grounded.

The engine's weakness is largely one of incompleteness rather than wrongness. Two parameters are declared but dead (`oct_competition`, `macroCoupling`). A coupling input pathway is accepted but discarded (`couplingOrganMod`). These are D004 violations that represent design intentions — the Chef Quad adversarial system — that have not been implemented. The engine is waiting for a feature that hasn't arrived.

The four models are not equally developed in the modulation space: `cluster` only meaningfully affects Cavaille-Coll; `chiff` only meaningfully affects the first two models; `buzz` works on Musette and Farfisa only. The six Chef parameters do not all fire across all four models. This is a mapping design gap — some Chef parameters are bystanders in 2 of 4 organ models.

Preset quality is high. The naming is specific, historically grounded, and evocative. The DNA assignments are accurate. Coverage at 22 presets is adequate but not fleet-leading. The absence of Family-mood presets is a notable gap for an engine with a warm, nostalgic musette accordion model.

At fleet average ~8.7, XOctave sits approximately 0.7 below the average in its current state due to the D004 violations and the dead coupling pathway. With those fixed, the engine moves to approximately 8.7, which is fleet-average. Full resolution of the Chef Quad adversarial competition system (which this engine was designed to anchor) would push it to 9.0+.

**Consensus Score: 8.01 / 10**

*Adjustment note: If the two D004 violations are fixed (2 hours of work), the score rises to approximately 8.5. If `couplingOrganMod` is wired to actual organ morphing, the score rises to approximately 8.7.*

---

## Priority Fix List

| Priority | Finding | Estimated Work | Score Impact |
|----------|---------|----------------|--------------|
| P1 | Wire `macroCoupling` to a DSP target (crosstalk or LFO cross-depth) | 30 min | +0.3 |
| P1 | Wire or remove `oct_competition` | 30 min | +0.2 |
| P2 | Wire `couplingOrganMod` to partial amplitude blend (CC↔Baroque morphing) | 2 hr | +0.3 |
| P2 | Move room resonance to post-mix global object | 1 hr | +0.1 |
| P3 | Extend `cluster` to engage Farfisa tab simulation and Baroque mixture blending | 3 hr | +0.2 |
| P3 | Add 4 Family-mood presets (musette accordion, Sunday warmth) | 2 hr | +0.1 |
| P3 | Add 2 Submerged presets (drowned cathedral, underwater organ) | 1 hr | +0.05 |

**Post-fix target score: ~8.7 — 8.8 (fleet average)**
**Post-Chef-Quad-adversarial target: ~9.0+**

---

*Seance conducted 2026-03-21. Eight ghost perspectives: Cavaille-Coll, Schnitger, Soprani, Nascimbeni, Moog, Carlos, Scott, Tomita.*
