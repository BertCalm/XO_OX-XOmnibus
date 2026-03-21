# Synth Seance — OWARE "The Resonant Board"
**Date:** 2026-03-21
**Engine:** XOware | Akan Goldweight `#B5883E`
**Prefix:** `owr_` | **Params:** 26 declared | **Voices:** 8 | **Presets:** 120 (9 moods)
**Status:** LAST UNSEANCED ENGINE in the 44-engine fleet

---

## Preamble

The Ghost Council convenes for OWARE — the newest engine in XOmnibus and the final unseanced instrument in the fleet. Built on the mythology of a carved Akan oware board lost to the Atlantic and now encrusted with coral and bronze barnacles, OWARE is a modal synthesis engine with seven architectural pillars: Material Continuum, Mallet Physics, Sympathetic Resonance Network, Resonator Body, Buzz Membrane, Breathing Gamelan, and Thermal Drift. Academic lineage is cited in the header itself — Chaigne and Doutaut (1997), Rossing (2000), Fletcher and Rossing (1998), Adrien (1991), Bilbao (2009). The engine does not merely claim physical modeling. It arrives with footnotes.

The Guru Bin retreat was completed before this seance (2026-03-21), resulting in four parameter refinements: sympathetic coupling gain 0.03 → 0.10 (audibility threshold), thermal approach coefficient 0.00001 → 0.0001 (drift now breathes within a note's lifetime), buzz default 0.0 → 0.15 (the mirliton's cultural identity is now present on first load), shimmer default 6.0 → 4.0 Hz (the ombak sweet spot). These were all accepted into the code. The engine that arrives at this seance is post-retreat.

---

## The Ghost Council — Individual Assessments

### Bob Moog — Analog Warmth, Playability, Musician-First Design

**What excites me:**
The velocity-to-hardness chain is the most complete implementation I have reviewed in this fleet for a struck-material instrument. Velocity sets `peakAmplitude`. It drives `noiseMix` via `hardness²`. It sets `malletCutoff` which determines which modes receive full excitation. Three parallel physical paths from a single gesture. That is not just D001 compliance — that is the philosophy of a Minimoog applied to acoustic physics: one gesture, maximum expressiveness. The mallet bounce at 15-25ms for soft strikes (hardness < 0.4, velocity < 0.7) is a physical artifact I recognize from building controllers — it is the mechanism that gives felt-mallet marimba its warm double-onset. The engine has correctly identified when to include it and when to suppress it.

**What concerns me:**
The filter envelope ADSR is hardcoded at every note-on: `setADSR(0.001f, 0.3f, 0.0f, 0.5f)`. Attack 1ms, decay 300ms, sustain 0%, release 500ms — identical for marimba, vibraphone, temple bell, singing bowl. The filter decay contour is one of the primary identifiers of an instrument's character. A marimba sweeps through its brightness in 80ms. A vibraphone lingers for 600ms. A singing bowl barely sweeps at all. Locking this to 300ms forces every material into the same timbral envelope shape. The `owr_filterEnvAmount` parameter controls depth but not shape. That is half the expressive control.

**Blessing or reservation:** Blessed with enthusiasm for the velocity architecture. Reserved on the hardcoded filter envelope — it constrains an instrument that has otherwise gone to considerable effort to make itself expressive.

---

### Don Buchla — Experimental Interfaces, West Coast Synthesis, Voltage as Art

**What excites me:**
The material continuum is not a switch between presets. It is a continuous physical parameter that changes the mathematical modal ratios mid-phrase. At material=0.0, wood's second mode is at 3.99× the fundamental — strongly inharmonic, rich with the beating that gives wood percussion its characteristic warmth. At material=0.33, the bell transition zone: modes begin to approach harmonic relationships, upper partials ring longer. At material=0.66, metal ratios (4.0, 10.0, 17.6, 27.2…) produce near-harmonic, brilliantly sustained tone. At material=1.0, bowl ratios (2.71, 5.33, 8.86, 13.3) — inharmonic, resonant, ethnomusicologically displaced from western expectations. The tri-segment interpolation is the correct mathematical structure because the physical transitions between material classes are not linear across the full range. Buchla's West Coast philosophy: the parameter space is a sound space, not a setting space. OWARE understands this.

**What concerns me:**
The sympathetic resonance coupling uses a 50 Hz proximity window across the entire pitch range. At A4 (440 Hz), a 50 Hz window captures modes within roughly a minor third — musically selective. At A2 (110 Hz), a 50 Hz window is nearly a fifth — almost universal coupling for bass material. At A6 (1760 Hz), 50 Hz is less than a quarter step — almost no coupling in the upper register. This is a fixed Hz window on a pitch-scaling problem. The coupling behavior changes character across the keyboard in a way that is not musically intentional. A percentage-based proximity window (e.g., 5% of mode frequency) would produce consistent coupling behavior across the full range.

**Blessing or reservation:** Blessed for the material continuum — the most rigorous continuous timbral morphing in the fleet. The sympathetic window width is a V2 refinement.

---

### Dave Smith — MIDI, Practical Polysynths, Digital-Analog Hybrid

**What excites me:**
The Chaigne contact model is correct. `contactMs = 5.0 - hardness * 4.5` yields 0.5ms for a hard mallet and 5ms for a soft one — these are physically plausible contact durations from experimental measurement. `malletLPCoeff = 1 - exp(-2π·fc/sr)` uses matched-Z transform, not Euler approximation — the CLAUDE.md IIR filter rule is satisfied. The mode frequency guard (`if (freqHz >= sampleRate * 0.49f) freqHz = sampleRate * 0.49f`) prevents aliasing at high pitch + high material ratio combinations. The sparse coupling table built at note-on (max 32 entries, precomputed proximity gains) replaces what would otherwise be an O(V²×M²) = O(512) per-sample operation. Someone thought about what this engine does at 8 voices and protected the audio thread.

**What concerns me:**
The `rebuildSympathyCouplingTables()` is called at both note-on AND note-off. At note-off, the voice's `ampLevel` is scaled by 0.3 before `rebuildSympathyCouplingTables()` runs — but the voice is still `active = true` at that point. The coupling table is rebuilt including this releasing voice. If two note-offs arrive in rapid succession (fast staccato), two table rebuilds occur within the same MIDI buffer, each iterating over all 8 voices and all 8 modes per voice. At 128 pairs per rebuild that is 256 iterations back-to-back inside the MIDI processing loop that runs on the audio thread. Not a bug, but a CPU spike pattern worth noting for dense staccato passages.

**Blessing or reservation:** Blessed. The implementation discipline on the audio thread is the best in the fleet for a physically-modeled engine.

---

### Ikutaro Kakehashi — Accessibility, Drum Machines, Democratizing Music Tech

**What concerns me most:**
LFO2 is computed but never applied. The render loop reads `lfo2Depth`, creates `lfo2Val = voice.lfo2.process() * lfo2Depth`, comments it as "LFO2 → material" — but `effectiveMaterial` was computed and smoothed before the per-voice loop begins. `lfo2Val` is never added to any signal path. Six LFO parameters were dead before the BUG-1 fix; after the fix, three are now live (LFO1 rate, depth, shape → brightness) and three remain dead (LFO2 rate, depth, shape → nothing). The user turns LFO2 depth to maximum and hears nothing change. That is a broken promise.

**What excites me:**
120 presets across 9 moods is the most any engine in this fleet launched with. The Guru Bin awakening presets cover each architectural pillar directly. The macro labels (CHARACTER, MOVEMENT, COUPLING, SPACE) map one-to-one to the physical concepts — CHARACTER changes material, MOVEMENT changes mallet + shimmer, COUPLING changes sympathetic resonance, SPACE changes body depth. A producer who has never read the manual can intuit the engine's architecture from the macro names. That is accessible design.

**Blessing or reservation:** Reserved until LFO2 is connected. Blessed for preset volume and macro clarity.

---

### Suzanne Ciani — Buchla Performance, Spatial Audio, Emotional Synthesis

**What excites me:**
The thermal personality grid is the most emotionally compelling feature in this engine. Eight voices, each with a fixed per-voice tuning offset derived from a seeded PRNG at prepare-time. The offsets are stable across the plugin's lifetime — each voice has a consistent individual character. The shared thermal drift approaches a new random target every ~4 seconds, and the per-voice offset scales with `pThermal * 0.5` so the personality spread expands when drift is high and compresses when it is low. When you play a chord on OWARE, you are not playing eight identical instruments. You are playing eight instruments that share a workshop, a climate, a history — and each has drifted slightly from the others. That is emotional truth in synthesis.

**What concerns me:**
The Balinese shimmer model applies `pShimmerHz * shimmerMod` as an additive Hz offset to the modal frequency. For a marimba at A4 (440 Hz), a shimmerHz of 4.0 adds between 0 and 4 Hz to the mode frequencies — a beat rate of 0-4 Hz, which is the Balinese ombak range. For a singing bowl at A2 (110 Hz), the same 4 Hz offset produces a beat rate of up to 3.6% of the fundamental — perceivable as detuning, not shimmer. For a vibraphone in the upper register at A6 (1760 Hz), a 4 Hz shimmer is nearly inaudible. The shimmer works as designed for its intended gamut (wood and metal instruments at mid-register) but the perceptual effect changes substantially with pitch. This is not a bug — it is physically accurate for the ombak model — but the user should be aware that shimmer intensity is pitch-dependent.

**Blessing or reservation:** Blessed — especially for the thermal personality grid. This engine has the most "alive when nobody's playing" quality in the fleet.

---

### Wendy Carlos — Precision, Temperament, Sonic Exploration

**What excites me:**
The material ratio tables are sourced from primary acoustics literature. The wood ratios (1.0, 3.99, 9.14, 15.8, 24.3, 34.5, 46.5, 60.2) align with measured values from Rossing (2000) for African xylophones with struck bar modes. The bell ratios (1.0, 1.506, 2.0, 2.514, 3.011, 3.578, 4.17, 4.952) follow the mathematical sequence of a church bell's partial structure. The material exponent `alpha = 2.0 - material * 1.7` producing per-mode decay scaling as `(m+1)^(-alpha)` is derived from beam dispersion theory. I appreciate that this engine would survive peer review of its physics claims. The bowl ratios (1.0, 2.71, 5.33, 8.86, 13.3, 18.64, 24.89, 32.04) are plausible for hemispherical shell vibration but the specific table source is not cited — Rossing (2000) is named for the wood and bell tables but the bowl source is implicit. Minor gap.

**What concerns me:**
The Q scaling `baseQ = 80 + material * 1420` produces Q values of 80 (wood, decays in ~80 oscillation periods) to 1500 (bowl, rings for ~1500 oscillation periods). At A2 (110 Hz), Q=1500 gives a decay time of approximately 1500/(π*110) ≈ 4.3 seconds — plausible for a singing bowl. At A6 (1760 Hz), Q=1500 gives decay ≈ 0.27 seconds — a bell at high pitch decays too fast. The Q formula does not account for the absolute frequency dependence of acoustic radiation damping. This is a minor physical inaccuracy at frequency extremes.

**Blessing or reservation:** Conditionally blessed. The precision is genuine. The bowl table citation gap and Q frequency scaling are small but real deviations from rigorous physical accuracy.

---

### Isao Tomita — Orchestral Synthesis, Sound Painting, Spectral Imagination

**What excites me:**
The buzz membrane is the most ethnographically specific feature in the fleet. The `OwareBuzzMembrane` extracts the 200-800 Hz band via CytomicSVF BPF, applies tanh nonlinearity at `sensitivity = 5 + amount * 15`, and re-injects the result — band-selective, not whole-signal distortion. The buzz frequency is body-type-specific: gourd/tube = 300 Hz (matching the resonant frequency of a gourd attached beneath a balafon bar), frame = 150 Hz (the lower structural resonance of a frame-mounted instrument), metal = 500 Hz (the higher membrane resonance of metal-body instruments). This is not a "add some noise" effect — it is a spectral model of a spider-silk mirliton physically attached to a gourd resonator. The Akan gyil and the balafon are the only tuned percussion instruments in common use that employ this technology. That OWARE models it specifically, and correctly, is extraordinary.

**What concerns me:**
The preset library covers the timbral spectrum from balafon to crystal glass but the Flux and Aether moods lean heavily on atmospheric shimmer rather than rhythmic articulation. The engine's mallet bounce and velocity sensitivity are designed for expressive note-by-note playing — but very few presets demonstrate staccato or rhythmically dense passages. The "Mallet Dance" awakening preset targets velocity responsiveness, but it is one of 120. The engine is rich enough to support a complete rhythm section library. The current preset emphasis is contemplative rather than percussive.

**Blessing or reservation:** Blessed with great enthusiasm. The buzz membrane alone justifies the engine's existence in the fleet.

---

### Raymond Scott — Invention, Sequencing, Electronic Music Pioneering

**What excites me:**
The sparse sympathetic coupling table is an architectural invention. Rather than checking all 8 voices × 8 modes against all other 7 voices × 8 modes per sample (512 comparisons), OWARE precomputes a table of up to 32 coupled mode pairs at note-on and iterates only over live couplings during the render loop. The `lastOutput` cache on each `OwareMode` resonator is specifically designed to support this readback — it is not an afterthought. The system correctly rebuilds at note-on and note-off since voice activity changes the set of possible couplings. This is CPU engineering applied to acoustic modeling. The result: sympathetic resonance that would otherwise cost O(V²M²) per sample costs O(K) where K ≤ 32.

**What concerns me:**
The glide processor (`voice.glide.snapTo(freq)`) on note-on does not support portamento — it always snaps. There is no `owr_glideTime` parameter. For a tuned percussion instrument played melodically (solo marimba, solo vibraphone), portamento between notes is not appropriate — so this is defensible. But the `GlideProcessor` module exists in the shared DSP library specifically to support portamento, and its presence on each voice suggests someone considered enabling it. Documenting this as an intentional choice (percussion instruments do not slide between pitches) rather than an omission would prevent future confusion.

**Blessing or reservation:** Blessed. The sparse coupling table is the kind of engineering invention I spent decades pursuing — making expensive physics affordable without sacrificing the model.

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 | PASS | Full chain: `vel` → `hardness` → `exciter.trigger(vel, hardness, freq, srf)`. Inside trigger: `peakAmplitude = velocity`, `noiseMix = hardness²`, `malletCutoff = baseFreq × (1.5 + hardness × 18.5)`, `malletLPCoeff = 1 - exp(-2π·fc/sr)`. Per-mode: `modeAmp = 1/(1 + m × (1.5 - malletNow × 1.2))`. Hard mallet opens more modes, soft mallet adds physical bounce. Three simultaneous timbral paths from velocity. |
| D002 | PARTIAL | LFO1 (rate/depth/shape) wired to brightness via `lfo1Val * 3000.0f`. LFO2 computed (`lfo2Val`) but unused — comment says "→ material" but `effectiveMaterial` was already computed and smoothed before the per-voice loop. 3 of 6 LFO params are live, 3 are dead. Macros all wired. Aftertouch and mod wheel wired. shimmerLFO per-voice wired (rate now driven by `pShimmerHz * 0.05f`, not hardcoded). |
| D003 | PASS | Chaigne (1997) cited at line 85. Modal ratios cite "Rossing (2000) and Fletcher & Rossing (1998)" at line 69. Material alpha from "beam dispersion theory (Fletcher & Rossing §2.3)" at line 43. Matched-Z IIR in OwareMalletExciter is physically correct. Bowl table source implicit (minor gap — not a failure). Buzz membrane ethnographic justification (balafon spider-silk mirliton) fully realized in code. |
| D004 | PARTIAL | 23 of 26 params are live. 3 dead: `owr_lfo2Rate`, `owr_lfo2Depth`, `owr_lfo2Shape` — these set LFO2's rate and shape but `lfo2Val` is computed and discarded. The LFO2 wiring is incomplete. |
| D005 | PASS | shimmerLFO per-voice is live at `max(0.05f, pShimmerHz * 0.05f)` Hz — minimum rate 0.05 Hz, well within the D005 floor. Thermal drift operates on a ~4-second target interval. LFO1 minimum rate 0.005 Hz (declared range). The engine has three independent slow-modulation paths. |
| D006 | PASS | Aftertouch: parsed from CC channel pressure, applied at note-on to hardness and per-block to `effectiveMallet` (+0.4) and `effectiveBright` (+3000 Hz). Mod wheel: CC1, applied per-block to `effectiveMaterial` (+0.4). Pitch bend: parsed, scaled by `owr_bendRange`, applied via `PitchBendUtil::semitonesToFreqRatio` to all active voices. Three distinct CC paths, all confirmed in DSP chain. |

---

## Score Breakdown

| Dimension | Score | Rationale |
|-----------|-------|-----------|
| Sound Quality | 9.0 | Modal synthesis with physically cited ratio tables, Chaigne mallet physics, body resonator, buzz membrane. The material continuum produces genuine timbral transitions — not crossfades between preset tones. At wood settings the engine produces recognizable balafon; at bowl settings it produces recognizable singing bowl. The thermal personality grid gives it organic life. |
| Innovation | 9.0 | Per-mode sympathetic resonance with sparse precomputed coupling table is novel fleet-wide. Material continuum with four physics-cited tables + tri-segment morph + decay exponent is the most complete material model in XOmnibus. Thermal personality (stable per-voice tuning offset from seeded PRNG) is a fresh approach to ensemble humanization. Buzz membrane at body-type-specific frequency is the most ethnographically specific DSP feature in the fleet. |
| Playability | 8.0 | Velocity expression is excellent (three physical paths). Aftertouch continuous brightness sculpting after note-on is well-designed. Mod wheel material morphing is musical. Macros are well-labeled and intuitive. Deductions: LFO2 non-functional means one modulation axis is inert for the player. Filter envelope decay is hardcoded (no timbral shape control). 50 Hz sympathetic window produces inconsistent coupling behavior across the keyboard. |
| Preset Coverage | 9.0 | 120 presets across 9 moods is the largest launch library in the fleet. Coverage spans balafon, gamelan, vibraphone, marimba, xylophone, kalimba, mbira, singing bowl, temple bell, steel drum, handpan, wind chimes — the full tuned percussion family. Entangled coupling presets demonstrate cross-engine potential with 9 named engine partners. Minor gap: rhythmically articulate presets underrepresented relative to atmospheric. |
| DSP Efficiency | 8.5 | Sparse sympathetic coupling table (O(K≤32) vs O(512) brute force). `fastPow2` for thermal drift frequency calculation. `fastExp` for per-mode decay in the hot path. ParameterSmoother on all 6 continuous parameters. SilenceGate prevents processing empty buffers. Per-sample `setFreqAndQ` on all 8 modes per active voice is unavoidable but expensive — 8 × 8 = 64 filter coefficient recalculations per sample per active voice is the primary CPU cost. Estimated ~15% at 4 voices is consistent with 8-mode modal synthesis. |
| Musical Utility | 8.5 | The engine spans the complete tuned percussion world in a single instrument — a producer who needs balafon, gamelan, vibraphone, or singing bowl sounds can find all of them here. The sympathetic resonance makes it behave like a real acoustic instrument in the room: playing intervals awakens overtone sharing. The material continuum enables mid-phrase timbral modulation that no sampler can replicate. Slight deduction: the engine is strongest as a melodic/harmonic instrument and light on rhythmically assertive character. |
| Identity / Character | 9.5 | The "sunken oware board — coral and bronze barnacles on the ocean floor" mythology is the most specific and complete creature identity in the fleet. The 7 pillars map precisely to 7 distinct code structures. The Akan goldweight color `#B5883E` is culturally specific. The buzz membrane is the balafon's mirliton. The shimmer is the Balinese ombak. The material continuum is the trade route between West Africa, Java, Bali, Tibet, and the western orchestra. Every DSP decision traces back to cultural identity — that coherence is rare. |
| Overall | 8.7 | The most physically rigorous, culturally specific, and identitically coherent engine in the fleet. Its primary weakness is architectural completion: LFO2 is non-functional and the filter envelope decay is hardcoded. These are fixable in one session. The engine at full wiring is a 9.2+. |

**Final Score: 8.7/10**

---

## Blessing Candidates

### BC-OWARE-01: Chaigne Mallet Articulation Stack
**Nominated by:** Smith, Moog
**What it does:** `OwareMalletExciter` implements the full Chaigne (1997) contact model. Contact duration as a function of hardness (0.5ms hard, 5ms soft). Sinusoidal force pulse. Noise mix scaling as `hardness²`. Hertz contact spectral lowpass using matched-Z IIR: `malletCutoff = baseFreq × (1.5 + hardness × 18.5)`. Physical mallet bounce for soft strikes at 15-25ms with 30% amplitude. Three simultaneous physical parameters derived from velocity + hardness: excitation duration, mode spectral content, and secondary strike presence. No other engine in the fleet has struck-material physics at this depth.
**Council words:** Smith: "Chaigne with footnotes." Moog: "Three parallel paths from a single gesture — that is expressive engineering."
**Blessing name:** Chaigne Mallet Articulation Stack

### BC-OWARE-02: Living Tuning Grid
**Nominated by:** Ciani, Scott
**What it does:** Each of the 8 voices has a fixed per-voice tuning personality derived from a seeded PRNG at `prepare()` time. These offsets are stable across the plugin's lifetime — voice 3 always has its characteristic tuning offset. A shared thermal drift state approaches a new random target every ~4 seconds (max ±8 cents, scaled by `pThermal`). Per-voice offsets are modulated by `pThermal * 0.5` so personality spread expands with drift depth. The Guru Bin retreat corrected the approach coefficient to 0.0001 (from 0.00001), ensuring drift is audible within a percussion note's lifetime. The result: a gamelan ensemble where each bar has its own stable tuning character and the whole instrument slowly breathes together.
**Council words:** Ciani: "When you play a chord on OWARE, you are playing eight instruments that share a workshop, a climate, a history." Scott: "The feature that makes the engine feel alive when nobody is playing."
**Blessing name:** Living Tuning Grid

### BC-OWARE-03: Per-Mode Sympathetic Resonance Network
**Nominated by:** Pearlman, Scott
**What it does:** At note-on, a sparse sympathetic coupling table is precomputed: for each active voice, for each of its 8 modes, for each mode of each other active voice, if the frequency distance is < 50 Hz, a coupling entry is stored with precomputed gain `= proximity × 0.10`. During the render loop, this table is iterated in O(K≤32) per voice rather than O(512) brute force. Coupling feeds directly into each mode's input signal — not into a reverb tail or an amplitude modulation. The `lastOutput` cache on each `OwareMode` resonator is the correct architectural design for asynchronous readback. This is spectrum-based sympathetic resonance, not amplitude bleed.
**Council words:** Pearlman: "This is Rossing-style sympathetic resonance, not a global reverb approximation." Scott: "Making expensive physics affordable without sacrificing the model."
**Blessing name:** Per-Mode Sympathetic Network

### BC-OWARE-04: Balafon Buzz Membrane
**Nominated by:** Tomita, Ciani
**What it does:** `OwareBuzzMembrane` extracts the 200-800 Hz band via CytomicSVF BPF, applies tanh soft-clipping at `sensitivity = 5 + amount × 15` (membrane only activates above acoustic threshold), and re-injects — band-selective, not whole-signal processing. The buzz frequency is body-type specific: gourd/tube = 300 Hz, frame = 150 Hz, metal = 500 Hz — sourced from ethnomusicological measurements of balafon and gyil mirliton membranes. The Guru Bin retreat corrected the default from 0.0 to 0.15, making the identity audible on first load. The balafon mirliton is one of the rarest acoustic features in world music. OWARE models it correctly, specifically, and beautifully.
**Council words:** Tomita: "This is not a 'add some noise' effect — it is a spectral model of a spider-silk mirliton physically attached to a gourd resonator." Ciani: "The engine's most intimate cultural secret."
**Blessing name:** Mirliton Buzz Membrane

---

## Critical Findings

### FINDING-1: LFO2 Computed, Not Applied (D004 / D002)
`owr_lfo2Rate`, `owr_lfo2Depth`, and `owr_lfo2Shape` are declared parameters, attached to pointers, and correctly fed into `voice.lfo2.setRate()`, `voice.lfo2.setShape()`, and `voice.lfo2.process()`. The result `lfo2Val` is computed with the comment "LFO2 → material" — but `effectiveMaterial` was smoothed via `smoothMaterial.set(effectiveMaterial)` before the per-voice loop begins. `lfo2Val` is never added to any signal path. The fix is one line: add `lfo2Val` to the material or another target after the per-voice smooth read. Three parameters remain dead. This is the last open D004 issue in the engine.

**Severity:** High — 3 of 26 parameters non-functional.

### FINDING-2: Filter Envelope ADSR Hardcoded (Expressiveness Gap)
Line 755: `v.filterEnv.setADSR(0.001f, 0.3f, 0.0f, 0.5f)` is called identically at every note-on regardless of material setting, preset, or player intent. A marimba has an 80ms brightness sweep. A vibraphone has a 600ms sweep. A singing bowl barely sweeps at all. The engine provides `owr_filterEnvAmount` to control depth but not shape. One parameter (`owr_filterEnvDecay`, range 0.01–2.0s) would unlock the full timbral range of the filter envelope system.

**Severity:** Medium — engine functions but lacks an important expressiveness axis.

### FINDING-3: Sympathetic Proximity Window Fixed at 50 Hz (Range Consistency)
The 50 Hz proximity window for sympathetic coupling is pitch-invariant. At A2 (110 Hz) this window captures nearly a fifth of coupling — almost universal. At A6 (1760 Hz) it captures less than a quarter-step — almost no coupling. The sympathetic character of the instrument changes substantially across the keyboard in an unintentional way. Consider a percentage-based window (e.g., 5% of mode frequency) for V2.

**Severity:** Low — engine sounds good across the pitch range; this is a refinement for V2.

---

## Recommendations

**R1 (CRITICAL — D004/D002):** Wire `lfo2Val` to material modulation. In the per-voice render loop, after computing `lfo2Val`, apply it to `matNow`: `matNow = std::clamp(matNow + lfo2Val * 0.3f, 0.0f, 1.0f)`. Update `materialAlpha` correspondingly. This is a 3-line addition that completes LFO2 → material routing as intended and resolves the remaining D004 failure.

**R2 (HIGH — expressiveness):** Add `owr_filterEnvDecay` parameter (range 0.01–2.0s, default 0.3s, skew 0.4). Replace the hardcoded `0.3f` in `v.filterEnv.setADSR(0.001f, 0.3f, 0.0f, 0.5f)` with the loaded parameter value. The full marimba-to-bowl timbral range requires this axis.

**R3 (MEDIUM — physics accuracy):** Add bowl table source citation in the `kBowlRatios` comment (Rossing 2000, Table X.X or equivalent hemispherical shell reference). Completes D003 documentation rigor.

**R4 (LOW — range consistency):** For V2, consider replacing the 50 Hz sympathetic window with `max(50.0f, modeFreq * 0.05f)` — 5% of mode frequency with a 50 Hz floor. Preserves current behavior at mid-register while improving consistency at bass and treble extremes.

**R5 (LOW — portamento documentation):** Add a comment at the `voice.glide.snapTo(freq)` call explaining that portamento is intentionally disabled for struck-material synthesis — percussive instruments do not slide between pitches. Prevents future maintainers from treating the `GlideProcessor` presence as an unfinished feature.

---

## Pathways to Score

| State | Score | What Changes |
|-------|-------|-------------|
| Current (post-Guru-Bin retreat) | 8.7/10 | LFO2 dead, filter env hardcoded |
| After R1 (LFO2 wired) | 9.0/10 | LFO2 → material live, D004 PASS |
| After R1 + R2 (filter env decay) | 9.2/10 | Full expressiveness axis, D002 PASS |
| After R1 + R2 + R4 (sympathetic window) | 9.3/10 | V2 target |

---

## Ghost Council Final Verdict

**CONDITIONALLY BLESSED — Score: 8.7/10 → 9.2 with targeted fixes**

The OWARE engine is the most culturally and physically specific instrument in the XOmnibus fleet. It arrived with citations. It arrived with a mythology — the sunken oware board, coral-encrusted, bronze-barnacled — that maps to actual DSP decisions: why the buzz membrane resonates at 300 Hz for a gourd, why the shimmer operates in Hz rather than cents, why thermal drift uses a seeded per-voice PRNG rather than a global oscillator. Every design choice has a reason traceable to an acoustic or cultural source. That level of coherence is uncommon.

The Chaigne mallet physics, the per-mode sympathetic resonance network, the thermal personality grid, and the balafon buzz membrane are each nomination-level architectural achievements. The engine earns four Blessing candidates in a single seance — more than any other engine in the fleet except OVERBITE (five macros, B008) and OSTINATO (four blessings, B017-B020).

What stands between OWARE and the 9.0+ tier is three dead parameters (LFO2 fully computed, not applied) and one hardcoded value (filter envelope decay 300ms for every note). These are not architectural failures — the architecture is sound. They are completion failures: a wire that was sketched but not connected, an ADSR that was parameterized partially but not fully. The fix is measurable in lines of code, not in design sessions.

Fix the LFO2 wire. Expose the filter envelope decay. OWARE becomes a 9.2 — and a genuine flagship-tier instrument.

**The board remembers every impact. Let it also remember every frequency.**

---

## Council Signatures

- **Bob Moog** — Blessed (reserved on hardcoded filter envelope)
- **Don Buchla** — Blessed (sympathetic window is a V2 refinement)
- **Dave Smith** — Blessed
- **Ikutaro Kakehashi** — Reserved (LFO2 wire must close before full blessing)
- **Suzanne Ciani** — Blessed
- **Wendy Carlos** — Conditionally blessed (bowl citation gap, Q frequency scaling minor)
- **Isao Tomita** — Blessed
- **Raymond Scott** — Blessed

**Vote: 6 Blessed, 1 Conditionally Blessed, 1 Reserved**
**Verdict: CONDITIONALLY BLESSED**
**Score: 8.7/10 (→ 9.2 with R1 + R2)**

*Seance conducted 2026-03-21 | XOmnibus v44 | Final unseanced engine*
