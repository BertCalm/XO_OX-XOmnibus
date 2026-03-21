# Seance Verdict — BROTH Quad + FUSION Quad
*Eight engines. Two collections. March 21, 2026.*
*Council: Moog, Buchla, Kakehashi | Fleet avg ~8.7*

---

## Overview

The BROTH Quad (XOverwash, XOverworn, XOverflow, XOvercast) and FUSION Quad (XOasis, XOddfellow, XOnkolo, XOpcode) represent two of the most conceptually coherent engine families in the fleet. BROTH is the Kitchen Collection's pad engine set — four instruments sharing a cooking-as-synthesis metaphor with cooperative spectral coupling. FUSION is the Kitchen Collection's electric piano set — four instruments modeling distinct EP traditions via physical modeling and additive/FM synthesis.

Both quads are registered and included in the build. Neither has been seanced before.

---

## Cross-Quad Analysis Findings (Read First)

### BROTH Cooperative Coupling: BROKEN in Production

The BROTH cooperative coupling architecture is elegant in design but **not wired in XOmnibusProcessor.cpp**. Each engine exposes setters and getters:
- `OverwornEngine::getSessionAge()`, `getConcentrateDark()`, `getSpectralMass()`
- `OverwashEngine::setBrothSessionAge(float)`
- `OverflowEngine::setBrothConcentrateDark(float)`
- `OvercastEngine::setBrothSpectralMass(float)`

A search of `XOmnibusProcessor.cpp` finds **zero calls** to any `setBroth*` or `getSessionAge/getConcentrate/getSpectralMass` methods. The BROTH coordinator that was supposed to pump values between engines after each `renderBlock` was never written. The four engines run completely independently. The cross-engine flavor chemistry — Overwash becoming more viscous as Overworn reduces, Overflow's threshold dropping as the broth concentrates, Overcast seeding dark crystals from reduced spectral mass — exists only in the individual engines, waiting for a coordinator that does not exist.

**This is the single most important finding of this seance.**

---

### XOverworn ReductionState: Correct but Invisible

The `worn_sessionAge` is declared as an APVTS parameter (read-only, labeled accordingly) but **the engine never writes its internal `reduction.sessionAge` back to the parameter**. The `pSessionAgeParam` pointer is attached in `attachParameters()` but is only ever read (it reads the default 0.0), never written. A UI display of session age will always show 0.0 regardless of how long the session has run.

The **ReductionState itself works correctly** as an accumulator. The math is sound:
- `baseRate = pReductionRate / (sessionTargetSec * srF)` — correct
- Per-band logarithmic evaporation with `bandRate *= remaining` — correctly decelerates as bands empty
- Maillard reaction via `fastTanh` saturation — musically appropriate
- The `sessionStarted` guard means idle time does not reduce — correct behavior

What does not work: **the reset parameter** has a subtle race-condition-style bug. The engine reads `pReset > 0.5f` per block, and sets `resetTriggered = true` to prevent double-firing. But `pReset` is an APVTS `float` parameter, not a button/momentary trigger. A user dialing the "Start Fresh" knob from 0 to 0.6 then back will trigger the reset on the way up. On the way down, `resetTriggered` will be false again, but nothing fires because `pReset <= 0.5f`. This is correct behavior for a toggle knob, but incorrect for the intended "button" semantic. Typical JUCE practice for a reset button would be a one-shot via `paramListener` or a dedicated boolean param. The current approach will work in practice but is fragile.

**Preset schema bug**: `Bone_Broth.xometa` contains `"worn_warmth": 0.8` as a parameter key. No parameter with ID `worn_warmth` exists in `OverwornEngine::addParametersImpl()`. This key will be silently ignored on load. Not a crash, but a sign that the preset was written against a slightly different parameter list than the current engine.

---

### FUSION SpectralFingerprint: Correct Architecture, Cosmetic Issue

The `SpectralFingerprint` struct is defined four times (once per engine header) guarded by `#ifndef XOMNIBUS_SPECTRAL_FINGERPRINT_DEFINED`. This is correct preprocessor practice but creates a maintenance problem: if the struct ever changes in one file, the guard prevents the update from propagating. The struct should live in a shared header (`Source/DSP/SpectralFingerprint.h`) and all four engines should include it.

The struct content is sound. The 38-float / 152-byte claim in the architecture doc is correct (8 + 8 + 10 named floats + 2 padding = 28 floats = 112 bytes by my count, not 152). The doc is wrong but the code itself does not hardcode 152 bytes anywhere — it is metadata, not a binary protocol — so this is a documentation bug, not a runtime bug.

The `getSpectralFingerprint()` implementations are mostly correct but share one common flaw: **`attackTransience` is never populated in Oddfellow and Opcode**. Oasis and Onkolo have `attackTransientTracker`/`attackTracker` members that are updated and exported. Oddfellow and Opcode return the default-initialized 0.0 for `attackTransience`, which means a 5th-slot engine reading these fingerprints cannot differentiate a hard attack from a soft sustain for those two engines. This matters because Oddfellow's grit is most visible in attack transients.

---

### FUSION Source Tradition Tests: Mostly Pass

1. **XOasis (Rhodes)**: PASS with qualification. The `RhodesToneGenerator` partial ratios are correct (1-6 harmonics). The 3rd partial boost (`kPartialAmps[2] = 0.55f`) captures the characteristic bell. The `RhodesPickupModel` uses a one-pole LP, which is a simplification — a full comb-filter model would be more accurate — but for a synthesizer engine rather than a sample player, this is acceptable. The `RhodesAmpStage` asymmetric clipping is correctly directional (positive clips harder). The bark is there. **Verdict: plays as a jazz EP.**

2. **XOddfellow (Wurlitzer)**: PASS. The inharmonic reed ratios (`1.0, 2.01, 3.03, 4.08, 5.15`) are exactly right for reed clamping. The `WurliPreamp` minimum drive of 1.5x ensures it can never be clean. The warble (`warbleDepth = (1.0f - reedStiffness) * 0.003f`) is subtle and organic. The always-on saturation is the defining Wurli property and it is implemented correctly. **Verdict: sounds like it has been on the road.**

3. **XOnkolo (Clavinet)**: PASS. The `ClaviStringModel` with 8 harmonics, odd-harmonic emphasis, and pickup-position comb filtering is physically coherent. The key-off clunk via LCG noise burst is a thoughtful touch. The `AutoWahEnvelope` with envelope-following BPF sweeping 400–5400 Hz will deliver funk. The fast damp on key release (`sr * 0.05f`) correctly captures the Clavinet's property of being a short-sustain instrument. **Verdict: the riff works.**

4. **XOpcode (DX7)**: PASS. The `DXModulationEnvelope` concept — FM index envelope creating "bell attack into warm sustain" — is the exact mechanism of the DX EP preset. The three algorithm modes (series/parallel/feedback) are the right primitives. The `velToIndex` parameter correctly captures velocity-sensitive FM character. The 2-operator simplification is honest — the DX7 used 6 operators but the EP patches were fundamentally 2-op in character. **Verdict: feels like 1983.**

---

### Strongest and Weakest

**Strongest engines in both quads:**
- XOverworn — most radical conceptual engine, sound design potential is extraordinary
- XOpcode — cleanest implementation, best doctrine compliance in both quads
- XOnkolo — most physically grounded, most immediately playable

**Weakest engines:**
- XOvercast — the anti-pad concept is philosophically interesting but the no-LFO-during-frozen promise is partially broken (LFO1 and breathLfo still run, they are just gated in synthesis; they still consume CPU and remain in state)
- XOverwash — the spectral field accumulator (`spectralField[32]`) is declared and initialized but **never written to or read from** in the render block. The entire cross-note interference mechanism exists in the struct but the render loop never populates the field. The `wash_interference` parameter modulates `pInterference` which in turn modulates... nothing in the actual synthesis path. D004 violation.

---

## BROTH Quad Seances

---

## XOverwash — "Ink in Water" (Spectral Diffusion Pad)
*Tea Amber #D4A76A | prefix: wash_*

### The Council

**Moog:** "Fick's Second Law as a synthesis metaphor — this is the kind of disciplined physical rigor I respect. The idea that spectral energy diffuses outward like dye in water, with each partial following the analytical Gaussian solution, is genuinely original. I have not seen this approach elsewhere. The implementation is clean and mathematically principled. My concern is that the `spectralField` accumulator — the mechanism for cross-note interference — is declared but never populated. The field exists. The parameter `wash_interference` exists. But when I follow the signal path in `renderBlock`, the `spectralField` array is never written and never read during synthesis. The interference concept is a ghost of itself."

**Buchla:** "The idea of harmonics diffusing outward in frequency space is beautiful — it inverts the normal relationship between time and spectrum. Notes do not decay; they spread. I am interested in the direction mechanism: odd partials spread up, even partials spread down, with LFO modulation to the direction. This creates beating between adjacent notes as their diffusion fronts overlap. That is legitimate inter-note interaction even without the spectral field. But I agree with Robert — the `wash_interference` parameter, labeled prominently as the COUPLING macro, does nothing audible. This is a broken promise."

**Kakehashi:** "For the performer, this instrument is genuinely different. A pad that becomes less focused over time, that blurs, that spreads — this is a sound players will reach for at specific moments. The default diffusion time of 10 seconds and the spread maximum of 200 Hz mean the first 10 seconds of any note are a journey. I like the breathing LFO adding organic frequency offset to the partials. My concern is practical: with 16 partials across 8 voices, the inner loop processes 128 partial calculations per sample. At 48kHz this is 6.1 million operations per second just for this engine. Has this been profiled?"

### Score: 7.8/10

### Dimensions
| Dimension | Score | Notes |
|-----------|-------|-------|
| Concept Originality | 9.5 | Fick's Law diffusion as pad synthesis — unprecedented |
| DSP Correctness | 7.0 | Core diffusion works; interference mechanic is dead code |
| D-Doctrine Compliance | 7.5 | D004 violation: wash_interference / spectralField unconnected |
| Sonic Identity | 8.5 | Diffusing pad is immediately recognizable and unique |
| Preset Quality | 8.0 | Good coverage (Aether, Atmosphere, Flux, Foundation, Prism) |
| Playability | 7.5 | Slow evolution can make it hard to preview in context |
| Integration Potential | 8.0 | BROTH coordinator would unlock real interference |
| Polish | 7.0 | Dead spectralField is a significant completeness gap |

### Top 3 Concerns

1. **P0 — D004 Violation: `wash_interference` is a dead parameter.** The `spectralField[32]` accumulator is never written during synthesis and never read. The `pInterference` value modulates a local variable that affects... the `pInterference` value only (it is used in the macro-combination code but then the result `pInterference` is not applied to any synthesis path). The COUPLING macro effectively controls nothing.

2. **P1 — BROTH coordinator missing.** The `setBrothSessionAge()` method exists and the viscosity-increase-with-age logic is in place, but `brothSessionAge` remains 0.0 for life because no coordinator calls the setter. This is the same finding as the cross-quad analysis.

3. **P2 — CPU profile unknown.** 16 partials × 8 voices × `std::sqrt()` per partial per sample is expensive. The `std::sqrt(2.0f * D * std::min(t, maxT))` call inside the per-partial inner loop runs every sample. Consider moving the `spread` calculation to control rate (once per block) since `diffusionClock` advances smoothly and the spread changes slowly.

### Top 3 Enhancements

1. **Wire the spectral field.** Build a simple 32-bin accumulator during the voice-synthesis loop: for each active partial, add its energy to the bin corresponding to its current frequency. Then, in the interference path, use the accumulated field to add subtle beating/cross-modulation to each partial. This makes `wash_interference` into a real thing — the promised interference fringes where diffusion fronts from different notes overlap.

2. **Add a "Dye" parameter.** The current engine always produces sawtooth-like harmonic weightings (1/n). A `wash_dye` parameter that selects between harmonic series families — 1/n (sawtooth character), 1/n² (triangle character), or odd-only — would dramatically expand the color palette without adding complexity.

3. **Preset starting states.** The engine concept is strongest when played with specific diffusion times. Add presets that start with a tight spectral snapshot (low diffusion time) versus a nearly-fully-diffused state (high age initialized). This gives players a "photographic" versus "watercolor" starting point.

### Blessing Candidates

None ratified. The spectral field interference, if actually implemented, would be a strong candidate — but cannot be blessed in its current non-functional state.

---

## XOverworn — "Sauce Reducing on a Stove" (Spectral Reduction Pad)
*Reduced Wine #4A1A2E | prefix: worn_*

### The Council

**Moog:** "This is the most radical synthesizer engine I have encountered in this fleet. The claim — that the ReductionState accumulates over a 30-minute session and cannot be erased except by explicit reset — is not a metaphor. It is a design decision with real consequences for workflow, for presets, for user experience. This is the kind of commitment to a singular vision that I respect deeply. The implementation is correct. The frequency-dependent evaporation with logarithmic slowdown at each band is exactly how a real reduction works — early evaporation is fast, late evaporation slows as concentration increases. The Maillard reaction via fastTanh saturation is musically smart. My reservation is that the session state is invisible to the player."

**Buchla:** "An instrument that remembers. That cannot forget within the session. This breaks every convention of what a synthesizer is. I am entirely in favor. The philosophical position — that the most radical synthesizer behavior is irreversibility, because all synthesizers reset on program load — is correct. The cooking metaphor is not decoration. It is the physics. My technical concern is the infusion mechanic: voices with `velocity < 0.3f && holdDuration > 8.0f` are flagged `isInfusion = true`. But `isInfusion` is set and never read. It is declared in `OverwornVoice` but the render loop does not check it. The infusion event — soft long tones adding spectral character without accelerating reduction — exists as a concept but not as code."

**Kakehashi:** "I must speak to the user experience. A player loads a preset that says 'Bone Broth.' The preset description says 'Two-hour slow simmer.' They play the instrument for 20 minutes, notice it has darkened. They save the session. Tomorrow they return. What does the engine sound like? The `worn_sessionAge` parameter is read-only and labeled 'read-only' in the parameter list, which is correct — but the engine never writes back its `reduction.sessionAge` to the APVTS parameter. Any DAW automation of this parameter, any host display of it, will show 0.0 permanently. This is a silent failure. The concept demands a persistent visible indicator."

### Score: 8.6/10

### Dimensions
| Dimension | Score | Notes |
|-----------|-------|-------|
| Concept Originality | 10.0 | Irreversibility as synthesis — unprecedented, publishable |
| DSP Correctness | 8.5 | Core reduction correct; infusion mechanic unimplemented |
| D-Doctrine Compliance | 8.0 | D001: velocity shapes reduction rate (correct); D006: aftertouch→heat (correct) |
| Sonic Identity | 9.0 | Unique timbre arc, no other engine does this |
| Preset Quality | 8.5 | Strong naming (Demi-Glace, Consomme, Maillard Fire) |
| Playability | 8.5 | The long arc is its identity — plays differently in practice |
| Integration Potential | 7.5 | BROTH coordinator missing; exports exist but are unused |
| Polish | 7.5 | worn_warmth orphaned in preset; sessionAge never displayed; isInfusion never consumed |

### Top 3 Concerns

1. **P0 — `worn_sessionAge` never written back to APVTS.** The `pSessionAgeParam` pointer is attached but the engine never calls `pSessionAgeParam->store(reduction.sessionAge)` (or equivalent). This means any host parameter display, DAW automation lane, or UI widget bound to `worn_sessionAge` will show a permanently frozen 0.0. The Overworn's defining feature — the visible reduction arc — is invisible to the host. Fix: write `reduction.sessionAge` to the APVTS parameter once per block (use `store()` with `std::memory_order_relaxed`).

2. **P1 — `isInfusion` flag is set but never consumed.** `OverwornVoice::isInfusion` is assigned `true` when `velocity < 0.3f && holdDuration > 8.0f`. Nothing in the render loop reads `isInfusion`. The promised behavior — that infusion events add spectral character without accelerating reduction — does not happen. Infusion notes currently accelerate reduction exactly like any other note (because `heatMultiplier` uses `activeVoiceCount` without checking whether those voices are infusing). This is both a D004 violation (a structural concept exists but produces no different audio) and a promise broken.

3. **P1 — `worn_warmth` orphaned in preset.** `Bone_Broth.xometa` contains `"worn_warmth": 0.8`. No parameter with ID `worn_warmth` is declared in `addParametersImpl()`. On preset load, `PresetManager` will skip this key silently. This means the preset is writing a parameter that does not exist. Either the parameter was removed from the engine after the preset was written, or the preset was drafted against a different parameter list. Clean this up.

### Top 3 Enhancements

1. **Visual reduction meter.** The most important enhancement is not DSP — it is UX. A UI element showing the 8-band `spectralMass[]` array as a frequency-domain bar chart would give players visual feedback on where they are in the reduction arc. The data exists (exportable via `getSpectralMass()`). The display does not.

2. **Implement infusion.** When `isInfusion = true`, add a gentle spectral enrichment: briefly re-energize the top 2 reduction bands by a small amount (`spectralMass[6,7] += 0.02f` capped at 1.0f), and do NOT apply the heat multiplier to those voices. This would make the soft-long-tone behavior real — the cook knowing to add a splash of water at low heat.

3. **Preset mid-reduction starts.** The `worn_sessionAge` parameter is stored in presets and is loadable. Design 3-4 presets that initialize `worn_sessionAge` at 0.3, 0.6, 0.8 — giving players a "this sauce has been reducing for a while" starting point. "Demi-Glace Start" (sessionAge=0.7) would be a compelling name.

### Blessing Candidates

**B-CANDIDATE: Spectral Irreversibility as Synthesis Parameter** — the `ReductionState` concept, if fully implemented with UI feedback, would be the first synthesizer that treats session time as a first-class timbral dimension. Buchla would give this 10/10 "as a philosophical position." Not ratified because the concept is only 70% implemented.

---

## XOverflow — "Pressure Cooker" (Pressure Pad)
*Steam White #E8E8E8 | prefix: flow_*

### The Council

**Moog:** "The Clausius-Clapeyron pressure model applied to synthesizer playing density — this is a genuinely clever mapping of physical chemistry to musical behavior. The idea that the pad has 'consequences,' that aggressive playing builds pressure and triggers release events, inverts the usual synthesizer contract where input intensity is proportional to output. Playing density is the parameter. I appreciate the precision in the interval-tension calculation: minor seconds, major seconds, and tritones add extra pressure. This is musically informed. My technical concern: the pressure decay coefficient `(1.0f - 0.00001f * inverseSr * 44100.0f)` hardcodes 44100 in a fleet that always passes `sampleRate` and explicitly forbids 44100 hardcoding."

**Buchla:** "The three valve types are exactly right as a taxonomy of release: gradual, explosive, and whistle. Each creates a distinct compositional relationship between the player and the instrument. The 'over-pressure catastrophic mode' — where sustained non-release causes saturation and then shattering — is the kind of playful violence I enjoy. My concern is the valve silence mechanic for the explosive type: `if (valveTypeInt == 1) duck = 0.0f;` silences ALL voices including those that have nothing to do with the pressure event. All 8 voices go silent on every explosive release. In a polyphonic context with multiple notes sounding, this is crude — it punishes uninvolved voices."

**Kakehashi:** "I want to comment on the performer experience. The pressure threshold at 0.7, the accumulation rate at 0.5, the gradual valve type — a first-time player will not know a release event is coming. They will be in the middle of a phrase and suddenly the sound changes. For some players this will feel like magic; for others it will feel like the instrument attacking them. I recommend a visible pressure meter — analogous to Overworn's needed reduction display — so the performer knows how close they are to the valve. The DSP is there. The feedback is not."

### Score: 8.0/10

### Dimensions
| Dimension | Score | Notes |
|-----------|-------|-------|
| Concept Originality | 9.0 | Pressure accumulation with consequence — genuinely new |
| DSP Correctness | 7.5 | Hardcoded 44100 in decay coefficient; explosive duck hits all voices |
| D-Doctrine Compliance | 8.5 | D001 correct; D006 correct (aftertouch adds pressure); D004 clean |
| Sonic Identity | 8.5 | Consequential pad — no other engine has this social contract |
| Preset Quality | 8.5 | Boiling Point, Explosion, Steam Whistle — good range |
| Playability | 7.5 | Surprise behavior needs feedback UI |
| Integration Potential | 7.5 | BROTH coordinator missing |
| Polish | 7.5 | 44100 hardcode; explosive duck is too aggressive |

### Top 3 Concerns

1. **P0 — Hardcoded 44100 in pressure decay.** Line: `pressureState.pressure *= (1.0f - 0.00001f * inverseSr * 44100.0f)`. At 48kHz, `inverseSr = 1/48000`, so the actual decay coefficient becomes `1.0 - 0.00001 * (1/48000) * 44100 = 1.0 - 0.00000919 ≈ 0.999991`. At 44100Hz it would be exactly `1.0 - 0.00001 = 0.999990`. The discrepancy is tiny (~9%) but the principle is violated — the decay rate is sample-rate dependent. Fix: use `1.0f - 0.0001f * inverseSr` and pick a musically intended decay time, then derive the coefficient from `exp(-1.0f / (decaySeconds * sr))`.

2. **P1 — Explosive valve type ducks all voices regardless of culpability.** When `valveTypeInt == 1`, `duck = 0.0f` silences every active voice. If a player holds a long chord and a pressure event fires from a new note, all held notes go silent. The intended behavior — a release burst followed by silence — should duck only the voices that triggered the pressure event, or at minimum apply a short fade rather than hard silence. Use a per-voice `duckEnvelope` rather than a global duck.

3. **P2 — No visual pressure feedback.** The `getStrainLevel()` method exists and exports `strainLevel` [0, 1.5]. No UI meter is described in the architecture. Given that surprise behavior is the feature, the performer needs to see the needle approaching the red zone. This is a UX issue, not a DSP issue, but it determines whether the engine feels like magic or malice.

### Top 3 Enhancements

1. **Per-voice pressure culpability weighting.** Instead of ducking all voices on release, introduce a `voicePressureContribution[]` array. Each note-on adds its velocity contribution to the voice's pressure weight. On release, duck voices proportional to their contribution. Silent notes (sustained from before the pressure event) are unaffected.

2. **Pressure pre-squeal.** At `strainLevel > 0.8`, begin a subtle high-frequency modulation above 8kHz — a physical "groan before the release." This telegraphs the imminent event musically without requiring a visual display.

3. **BROTH threshold coupling.** The `setBrothConcentrateDark()` mechanism lowers the threshold when the broth is concentrated. This is correctly wired in the engine. The BROTH coordinator not existing means this never fires. When the coordinator is built, Overflow's dynamic pressure threshold will be one of the most interesting BROTH interactions.

### Blessing Candidates

None. The concept is strong but needs the BROTH coordinator and per-voice duck fix before a blessing conversation makes sense.

---

## XOvercast — "Ice Forming on Glass" (Crystallization Pad)
*Ice Blue #B0E0E6 | prefix: cast_*

### The Council

**Moog:** "Wilson's nucleation theory applied to pad synthesis is the most unexpected physical model I have encountered. A pad that freezes — that captures a spectral state at the moment of note-on and holds it without evolution — is the philosophical opposite of every pad synthesizer I have built. I spent my life creating instruments that breathe, evolve, wander. XOvercast says: no. Sound crystallizes. It stops. It waits. This is a valid artistic position. The crackling transition during crystallization — random amplitude modulation creating the sound of ice forming — is particularly well-realized. My concern: the 'frozen state implies no modulation' promise is broken. LFO1, LFO2, and breathLfo continue running even when frozen; they are only gated from affecting amplitude in the synthesis section. They still consume CPU and their phase state continues to evolve."

**Buchla:** "The shatter mode — silence gap between crystals — is the strongest preset territory here. The shatter gap at 100ms, with a brief noise burst (shards), then new crystal forming, creates a rhythmic property the other BROTH engines lack. This is a pad that can play in time if the player treats note-on as a trigger. The lattice snap parameter — snapping partials to exact harmonic ratios during freeze — captures the physical process of crystallization imposing order on chaos. I find this beautiful. The Overcast is the most anti-musical of the four BROTH engines and therefore the most interesting."

**Kakehashi:** "The time relationship — 'Negation' — is the most radical philosophical position in the BROTH quad. Not just different from conventional synthesis, but opposed to it. I am concerned about one structural issue: the `brothSpectralMass` field is set correctly via `setBrothSpectralMass()` but is **never used in the render block**. The architecture document says 'Reads XOverworn's spectralMass to seed crystals. Reduced spectral mass = fewer, darker nucleation sites.' The architecture comment in the engine header says the same. But in the `renderBlock`, `brothSpectralMass` is read in no DSP path. The crystal seeding in `noteOn()` uses the fundamental harmonics directly. Dark ice from reduced broth is a concept, not a feature."

### Score: 7.9/10

### Dimensions
| Dimension | Score | Notes |
|-----------|-------|-------|
| Concept Originality | 9.5 | The anti-pad — instantaneous capture, no evolution — philosophically audacious |
| DSP Correctness | 7.5 | brothSpectralMass never consumed; LFOs run during frozen state |
| D-Doctrine Compliance | 8.0 | D001 correct (velocity → crystal density); D005 technically compliant but spiritually violated |
| Sonic Identity | 9.0 | Flash Freeze, Pure Crystal, Black Ice — the sonic territory is distinctive |
| Preset Quality | 8.0 | Good range across 6 moods; shatter presets would be welcome |
| Playability | 7.5 | Anti-conventional — players must relearn expectations |
| Integration Potential | 7.0 | Key BROTH hook (brothSpectralMass) unused |
| Polish | 7.0 | Multiple partial promises undelivered |

### Top 3 Concerns

1. **P0 — `brothSpectralMass` is never consumed.** The field is set, the architecture describes its use, but the `noteOn()` and `renderBlock()` paths never read `brothSpectralMass`. Dark ice from a reduced broth does not exist. This is the Overcast's equivalent of Overwash's dead interference field — the key cooperative hook is present but disconnected.

2. **P1 — LFOs run during frozen state.** The comment at line 515-519 correctly gates breath and lfo modulation: `if (!crystal.isFrozen) { amp *= (1.0f + breathVal * ...) }`. But `lfo1.process()`, `lfo2.process()`, and `breathLfo.process()` still advance their phase every sample even when fully frozen. This is both a CPU waste and a philosophical violation — the frozen state should freeze the LFOs too. When the crystal thaws (shatter mode) and refreezes, the LFO phases will have drifted unpredictably. Add `if (!crystal.isFrozen) { /* advance LFOs */ }` or use a per-voice freeze flag to gate LFO processing.

3. **P2 — No FilterEnvelope in OvercastVoice.** Every other BROTH engine has a `FilterEnvelope filterEnv` member in its voice struct. Overcast does not. The voice filter cutoff in the render block uses `pFilterCut` directly (sweeping during crystallization, locked during freeze). This is intentional — freeze semantics demand no envelope — but it means D001's filter brightness path (velocity-scaled) is absent. Velocity only controls crystal density (numPeaks), not filter position. A gentle velocity-to-initial-cutoff scaling would make the crystal brighter or darker depending on attack energy.

### Top 3 Enhancements

1. **Consume `brothSpectralMass`.** In `noteOn()`, after setting `crystal.peakAmps[]`, attenuate upper-harmonic peak amplitudes by `(1.0f - (1.0f - brothSpectralMass) * float(p) / nPeaks)`. A fully reduced broth (spectralMass=0.0) would produce crystals with almost no upper peaks — dark, fundamental-only ice. This delivers the promised "dark ice, not bright ice" behavior.

2. **Shatter preset series.** The shatter mode (transition=2) combined with different shatter gap durations creates a rhythmic vocabulary. A preset series at 50ms / 100ms / 200ms / 400ms gaps would create quarter-note / eighth-note / etc. rhythmic crystallization in different tempos. Add a tempo-sync option for `cast_shatterGap`.

3. **Frozen state LFO suspension.** When `crystal.isFrozen`, stop advancing LFO phases. Store `frozenLfoPhase` at the freeze moment and restore it on thaw (shatter mode). This makes refreezes deterministic — the crystal sounds the same each time it forms, which is physically correct.

---

## FUSION Quad Seances

---

## XOasis — "The Spice Route Rhodes" (Rhodes / Tine EP)
*Cardamom Gold #C49B3F | prefix: oasis_*

### The Council

**Moog:** "The `RhodesToneGenerator` is physically accurate at the level that matters for synthesis. The partial amplitudes — 1.0, 0.35, 0.55, 0.15, 0.08, 0.04 — correctly represent the measured spectral output of a Mk I Stage 73 (third partial boosted relative to second, upper partials rapidly decaying). The `RhodesPickupModel` one-pole LP is a simplification of the true pickup comb-filter physics, but it captures the essential tonal shift. The `RhodesAmpStage` asymmetric clipping — harder on positive excursions — is exactly how tube asymmetry produces even harmonics, the characteristic 'bark.' This engine understands what it is modeling."

**Buchla:** "I am less interested in the accuracy and more interested in what the `migration` parameter unlocks. At `migration = 0`, this is a Rhodes. At `migration = 1`, the Spectral Fingerprint from coupled Kitchen engines modulates the timbre. This means a player can morph between a pure Rhodes and a hybrid influenced by the Kitchen Collection's resonators. This is the FUSION concept. My concern: the `migration` parameter loads and smooths but I do not see in the render code exactly how `smoothMigration.process()` feeds into the synthesis. I would like to see the migration-to-spectral-coupling path more explicitly. The architecture document describes it; the code needs scrutiny."

**Kakehashi:** "This is the electric piano I would have wanted at every session I produced in the 1970s and 1980s. The tremolo — per-voice `StandardLFO` at stereo width — is correct Suitcase behavior. The velocity-to-bell-boost quadratic scaling means playing hard gives you the classic bark nonlinearly. This is how real keyboards respond. The glide processor on a Rhodes is unusual but useful for players who bend into notes. My concern is practical: the source tradition test asks if someone can play McCoy Tyner's 'Afro Blue' voicings and have it feel right. The tine decay rates are correct (upper partials decay in 2-0.75 seconds, lower partials sustain longer). This will pass the test."

### Score: 8.7/10

### Dimensions
| Dimension | Score | Notes |
|-----------|-------|-------|
| Concept Originality | 8.0 | Solid physical model; migration concept is novel |
| DSP Correctness | 8.5 | Partial ratios accurate; RhodesPickupModel simplified but acceptable |
| D-Doctrine Compliance | 9.0 | All 6 doctrines met; velocity→bark quadratic (correct) |
| Source Tradition Test | 9.0 | PASS — bell, warmth, tremolo all present and correct |
| Sonic Identity | 8.5 | Instantly recognizable Rhodes character |
| Preset Quality | 8.0 | 10 presets across 9 moods; "Midnight at the Kissa" named appropriately |
| Integration Potential | 9.0 | SpectralFingerprint exports functional; migration path needs verification |
| Polish | 8.5 | attackTransientTracker present and exported; good implementation quality |

### Top 3 Concerns

1. **P1 — Migration coupling path needs explicit verification.** `smoothMigration.process()` produces a value that is used in the render block, but the actual mechanism by which the SpectralFingerprint from Kitchen engines influences Rhodes timbre is not visible in the code sections reviewed. The migration parameter exists and smooths, but how it modifies the tine model, pickup position, or amp stage needs a code audit to confirm it is wired.

2. **P1 — `attackTransientTracker` decay rate is implicit.** The field tracks recent transient energy for SpectralFingerprint export. The architecture document says it "decays ~50ms." The actual decay coefficient is not visible in the reviewed sections. Verify the decay matches 50ms at all sample rates (should be `exp(-1/(sr * 0.05))`).

3. **P2 — DC blocker in `RhodesAmpStage` uses a fixed coefficient** (`dcBlock += 0.0001f * ...`). This is a first-order IIR with a cutoff that varies with sample rate. At 96kHz the cutoff is approximately 1.5 Hz; at 44.1kHz it is approximately 0.7 Hz. The cutoff should be derived from sample rate for consistent low-frequency behavior.

### Top 3 Enhancements

1. **Explicit migration display.** Add a `migration_fingerprint` UI element showing which Kitchen engine's influence is dominant. A cultural route map — Spice Route → current geographic position based on coupled engine — would be visually compelling and help players understand what migration 0.5 actually sounds like.

2. **Tine type variants.** The current model uses fixed partial amplitudes for Mk I Stage 73. Add a `oasis_tineType` parameter selecting from 3 tine geometries: Stage 73 (current), Suitcase (more fundamental, warmer), and Mark V (slightly more modern, brighter). This expands the Rhodes vocabulary without requiring a new engine.

3. **Sympathetic string resonance.** The Suitcase Rhodes has audible sympathetic resonance between adjacent tines. A very subtle (0–5%) energy coupling between voices at octave/fifth relationships would add the characteristic "shimmer" of playing chord voicings.

### Blessing Candidates

None ratified at this time. The SpectralFingerprint export with `attackTransientTracker` is a strong implementation — the first in the fleet with transient-sensitive fingerprinting. If migration coupling proves fully wired, this warrants consideration.

---

## XOddfellow — "The Night Market Wurlitzer" (Wurlitzer / Reed EP)
*Neon Night #FF6B35 | prefix: oddf_*

### The Council

**Moog:** "The inharmonic partial ratios — 1.0, 2.01, 3.03, 4.08, 5.15 — are the most important design decision in this engine, and they are correct. Reed clamping imperfection produces exactly this kind of mild inharmonicity, biased toward sharp upper partials. The warble implementation — a fixed 4.5 Hz sine modulating the fundamental frequency — is correct in rate (Wurlitzer warble is typically 3–6 Hz) and in depth (`warbleDepth = (1.0f - reedStiffness) * 0.003f`). At maximum warble, this produces ±0.3% frequency deviation, which translates to approximately ±5 cents — authentic. The always-on minimum drive (1.5x) in WurliPreamp is the single most important character decision and it is right."

**Buchla:** "The dual fastTanh in WurliPreamp — `fastTanh(driven * 0.8f) * 0.7f + fastTanh(driven * 1.5f) * 0.3f` — creates a more complex saturation curve than a single tanh. The two-stage version mimics the Wurlitzer preamp's cascaded stages. I notice the `attackTransience` field in the SpectralFingerprint is not populated for Oddfellow (no `attackTracker` member). For a reed instrument, the attack transient is significant and should be exported. This weakens the 5th-slot coupling metadata for this engine specifically."

**Kakehashi:** "The source tradition test — 'Night Market Grit' — asks for Ray Charles-style block chords with reed warble, preamp grit, and pulsing tremolo. The tremolo parameters default to 5.5 Hz rate and 0.4 depth — this is the authentic Wurlitzer 200A tremolo rate. The preamp drive at 0.3 default is appropriately mild, and the user can push it to full grit. My concern is that the filter brightness default (4000 Hz) may leave the Wurlitzer sounding slightly open and modern for the traditional character. A lower default (around 2500–3000 Hz) would capture more of the classic Wurlitzer 'boxy' warmth."

### Score: 8.5/10

### Dimensions
| Dimension | Score | Notes |
|-----------|-------|-------|
| Concept Originality | 8.0 | Wurlitzer physical model with migration; solid execution |
| DSP Correctness | 8.5 | Partial ratios, warble depth, preamp structure all correct |
| D-Doctrine Compliance | 8.5 | D001-D006 met; velocity→reed stiffness is correct |
| Source Tradition Test | 8.5 | PASS — warble, grit, tremolo present |
| Sonic Identity | 9.0 | Immediately recognizable Wurlitzer character |
| Preset Quality | 8.0 | 10 presets; "night market" aesthetic holds |
| Integration Potential | 8.5 | SpectralFingerprint exports; attackTransience missing |
| Polish | 8.0 | Filter default too bright; attackTransience unpopulated |

### Top 3 Concerns

1. **P1 — `attackTransience` not populated in SpectralFingerprint.** The `getSpectralFingerprint()` method does not include an `attackTracker` member or any analogous mechanism. `fp.attackTransience` defaults to 0.0. For a reed instrument whose character is dominated by attack, this is a meaningful gap in the coupling metadata. A 5th-slot engine cannot distinguish a hard Wurlitzer hit from a soft sustain using this fingerprint.

2. **P1 — Filter brightness default 4000 Hz is too open.** The Wurlitzer 200A had a characteristically warm, slightly boxy high-frequency response. A default filter cutoff of 4000 Hz is more "clean Fender Rhodes" than "road-worn Wurli." Recommend lowering to 3000 Hz as default. The user can always brighten it.

3. **P2 — Warble is monophonic.** The `warblePhase` in `WurliReedModel` advances at a fixed 4.5 Hz for all voices identically — all voices have the same warble phase simultaneously. Real Wurlitzers have manufacturing variation between reeds, so each note has slightly different warble. A per-voice `warblePhase` with a small random offset on trigger would add authenticity. The current implementation produces a "unison warble" effect when playing chords.

### Top 3 Enhancements

1. **Per-reed warble rate variance.** On voice trigger, set `warbleFreq = 4.5f + (voice index * 0.2f - 0.7f)` so voices have warble rates between 3.7 and 5.3 Hz. This creates the beating-between-reeds character of playing Wurlitzer chords.

2. **Preamp channel strip.** Add `oddf_eq_mid` — a gentle mid-boost parameter (1-4 kHz range, ±6dB) to capture the Wurlitzer's presence peak. Many classic Wurlitzer sounds were shaped by the DI going through a console strip with a mid boost.

3. **Road wear parameter.** `oddf_wear` (0-1): at 0 = pristine instrument (tighter inharmonicity, less warble, cleaner preamp). At 1 = well-traveled instrument (wider inharmonicity ratios, more warble, more preamp harmonic saturation). This gives access to the full character range described in the mythology.

### Blessing Candidates

None at this time. The Wurlitzer physical model is strong but not novel enough on its own for a fleet-level blessing.

---

## XOnkolo — "The Diaspora Clavinet" (Clavinet / Pickup Keys)
*Kente Gold #FFB300 | prefix: onko_*

### The Council

**Moog:** "The `ClaviStringModel` with key-off clunk is one of the best instrument-specific mechanical details I have seen in this fleet. The Clavinet's damper pad clunk is often ignored in emulations — it is part of the instrument's character, particularly in staccato playing. The LCG noise burst implementation (`clunkNoiseState = clunkNoiseState * 1664525u + 1013904223u`) is the correct Knuth TAOCP LCG for audio noise. The fast key-release damping time (`sr * 0.05f` = 50ms time constant) is accurate. This is a physically informed model."

**Buchla:** "The pickup position comb filter — `nodeProximity = |sin(pickupPos * harmonicNum * π)|` — is a simplified but conceptually correct model of how a magnetic pickup at position P on a string captures harmonics based on the mode shape amplitude at P. At neck position (pickupPos=0), the formula correctly attenuates harmonics that have nodes near the neck. This is the right physics even if the specific formula is an approximation. I am interested in the cultural mythology: named for `nkolo`, Central African ancestor of the kalimba. The lineage matters — it explains why this instrument is called 'The Diaspora Clavinet' rather than just a Clavinet model."

**Kakehashi:** "The `AutoWahEnvelope` with 2ms attack and 100ms release, sweeping 400–6000 Hz, is the quintessential Clavinet effect. The velocity-plus-envelope combination for the wah center frequency is exactly right: harder playing + higher energy = higher wah peak = more aggressive sweep. The 'Superstition' test will pass. My concern is that the `onko_funk` parameter at 0 (clean, no wah) produces a straight additive string — the Clavinet without wah is still the Clavinet, but it should still sound funky, not like a generic pad. The clean tone needs to have enough pickupiness and percussive attack to stand alone."

### Score: 8.8/10

### Dimensions
| Dimension | Score | Notes |
|-----------|-------|-------|
| Concept Originality | 8.5 | Physical model with cultural mythology; key-off clunk is rare |
| DSP Correctness | 9.0 | String model, pickup comb, autowah — all physically coherent |
| D-Doctrine Compliance | 9.0 | All doctrines met; velocity→string energy correct |
| Source Tradition Test | 9.5 | PASS — "Superstition" would pass convincingly |
| Sonic Identity | 9.0 | Unmistakably Clavinet; diaspora mythology adds depth |
| Preset Quality | 8.0 | 10 presets; funk vocabulary well covered |
| Integration Potential | 8.5 | SpectralFingerprint with attackTracker; impedance 0.7 (correct) |
| Polish | 8.5 | Clean implementation; some nuances missing |

### Top 3 Concerns

1. **P1 — Wah-off clean tone character.** At `onko_funk = 0.0`, the engine produces a raw additive string output without any envelope-following BPF. The Clavinet's clean sound has more character than a simple additive string — the pickup position filtering alone (neck vs bridge) is the main character differentiator. The `onko_pickup` parameter handles this, but the default pickup position (0.5, midpoint) produces a moderate-sounding tone that lacks the Clavinet's signature midrange presence. Consider changing the default pickup to 0.7 (closer to bridge).

2. **P1 — Polyphonic note-off targeting.** The `releaseKey()` method on `ClaviStringModel` is called when the voice's note-off is processed. But in a polyphonic context, `OnkoloEngine::noteOff()` calls `VoiceAllocator::findVoiceForNote()` and then triggers the voice's note-off. The `ClaviStringModel::releaseKey()` should be called explicitly to trigger the clunk. It needs verification that this call chain is complete in the render code — i.e., that `voice.string.releaseKey()` is called on note-off, not just `voice.ampEnv.noteOff()`.

3. **P2 — LFO targeting in Clavinet context.** The Clavinet was not an instrument typically played with LFO vibrato — its character is percussive and rhythmic, not sustained. Both LFOs default to pitch/filter modulation targets that are more Rhodes/Wurli appropriate. For the Clavinet, LFO1 should default to targeting `onko_funk` (auto-wah depth modulation) rather than pitch. A wah that pulses rhythmically is more characteristic than a Clavinet with vibrato.

### Top 3 Enhancements

1. **Funk rhythm mode.** Add `onko_funkRate` — an optional auto-wah rate parameter that makes the wah pulse rhythmically rather than following the envelope. At 0, pure envelope following (current). At 1, pure LFO-driven wah. At midpoint, a blend of both. This unlocks the rhythmically pulsing wah character used in classic funk records.

2. **String material parameter.** The `oaken_stringTension` concept from XOaken could be adapted here: `onko_string` selecting from rubber pad (0 = most damped, fastest decay), nylon (0.5), and metal (1.0 = brightest, most sustain). The Clavinet had limited string choice but the rubber pad hardness affected tone significantly.

3. **Harmonic body resonance.** The Clavinet body has a small resonant peak around 600–800 Hz that adds mid warmth to the string output. A single-band BPF resonator (Q=2, gain=+3dB at 700 Hz) in the signal path would add this body character without significantly increasing CPU.

### Blessing Candidates

**B-CANDIDATE: Key-Off Mechanical Authenticity** — the implementation of key-off clunk as a first-class synthesized event (LCG noise burst, per-key velocity scaling, fast damper simulation) is the most mechanically faithful detail in the electric piano quad. It would not reach Blessing status alone but is a strong contribution to the argument that the FUSION quad takes physical modeling seriously.

---

## XOpcode — "Algorithm, 1983" (DX7 / FM EP)
*Digital Cyan #00CED1 | prefix: opco_*

### The Council

**Moog:** "The `DXModulationEnvelope` — a four-stage envelope controlling FM index over time — is the correct understanding of what makes the DX EP sound the way it does. The index falls from peak to sustain during the note, and this decay is what creates the bell attack into warm sustain. The implementation is clean: linear attack, exponential decay, sustain, exponential release. The choice to use only 2 operators is honest — it would have been easy to add more operators for cosmetic complexity, but the DX EP patches were 2-op in spirit. This restraint is correct."

**Buchla:** "I am interested in the feedback algorithm mode. Self-feedback in FM synthesis produces phase noise — not true noise, but a rapid phase accumulation that creates metallic, harsh timbres. The implementation: `feedbackState = carrier.process(freq, feedbackState * pFeedback)` (approximately). If the feedback is too high, this is numerically unstable — unbounded phase accumulation. There should be a saturation or clamp on `feedbackState` before it feeds back. I would want to see the exact feedback path to be certain it is stable. But the concept is correct."

**Kakehashi:** "I produced many records in the 1980s with the DX7. The sound that defines a generation. What XOpcode has understood is that the DX EP sound is not about the operators — it is about the modulation envelope arc. The bell attack. The warmth that remains. Playing E Piano 1 on a DX7 is an emotional experience precisely because the FM index envelope creates a sense of resolution — the initial complexity decays toward simplicity, and simplicity feels like arrival. The `velToIndex` parameter correctly captures the velocity-sensitivity of the original preset: harder playing = more initial FM index = brighter, more complex attack. This is what gives the DX EP its dynamic range."

### Score: 9.0/10

### Dimensions
| Dimension | Score | Notes |
|-----------|-------|-------|
| Concept Originality | 8.5 | Clean FM EP with honest scope; 3-algorithm taxonomy is right |
| DSP Correctness | 9.0 | Modulation envelope correct; feedback mode needs stability check |
| D-Doctrine Compliance | 9.5 | All 6 doctrines clearly met; velToIndex is excellent D001 |
| Source Tradition Test | 9.5 | PASS — "1983" bell-to-warmth decay arc is correct |
| Sonic Identity | 9.0 | Unmistakably DX EP character |
| Preset Quality | 8.0 — | 10 presets; E Piano 1 equivalent should be Foundation starter |
| Integration Potential | 9.0 | SpectralFingerprint with harmonicDensity=0.95 (correct for FM) |
| Polish | 9.0 | Best-implemented engine in the FUSION quad |

### Top 3 Concerns

1. **P1 — Feedback algorithm stability not verified.** The feedback mode involves `feedbackState` feeding back into the modulator's phase modulation. In DX7 hardware, feedback was quantized to 6 bits and clamped. In a floating-point implementation, high feedback values can cause `feedbackState` to grow without bound over long sustained notes, eventually producing INF or NaN. A `clamp(feedbackState, -4.0f, 4.0f)` before the feedback path is standard practice for FM feedback stability.

2. **P1 — `attackTransience` not populated.** Same finding as XOddfellow. The DX EP is highly transient-dominant — the FM index envelope creates a very pronounced attack. The SpectralFingerprint exports `attackTransience = 0.0` by default because no tracker exists. A simple leaky rectifier (`attackTracker = max(attackTracker * 0.999f, |new sample|)`) would capture this.

3. **P2 — Algorithm parameter is a float range 0–2 with integer rounding.** `paramAlgorithm` is a `juce::AudioParameterFloat` in range [0.0, 2.0] with integer rounding (`static_cast<int>(loadP(paramAlgorithm, 0.0f))`). The standard JUCE practice for enum-like parameters is `juce::AudioParameterChoice` or `juce::AudioParameterInt`. A float with rounding works but creates awkward UI behavior (the knob has three valid positions but the full range is shown).

### Top 3 Enhancements

1. **Operator ratio library.** The `opco_ratio` parameter covers 0.5–16.0 continuously, but FM synthesis is most musical at specific ratios (1:1, 1:2, 1:3, 2:3, 1:4, 7:8, etc.). Add a `opco_snapRatio` toggle that snaps the modulator ratio to the nearest musically useful FM interval. This recreates the DX7's algorithm table behavior without locking the user to presets.

2. **Third operator (optional).** A minimal `opco_op3Enable` parameter could add a secondary carrier operating in parallel, producing the organ-like tones the DX EP could make in Algorithm 2 and 3 configurations. This would expand the engine beyond strict DX EP territory into the wider DX7 vocabulary.

3. **Phase distortion blend.** The DX EP had a characteristic "thump" at note-on — a very brief moment before the FM index establishes itself. A phase distortion oscillator blended at the attack transient (`attackTransience > 0.5f`) would add this thump without requiring a separate synthesis stage.

### Blessing Candidates

**B-CANDIDATE: Modulation Envelope as Timbre Trajectory** — the `DXModulationEnvelope` controlling FM index is not just an implementation detail; it is the correct identification of what FM synthesis IS for electric piano: a timbre trajectory, not a static timbre. The decision to make this the engine's central concept, rather than adding more operators, shows genuine understanding of the source tradition. If feedback stability is confirmed and attackTransience is populated, this engine has fleet-level blessing potential.

---

## Summary Scores

### BROTH Quad

| Engine | Score | Strongest Quality | Key Deficit |
|--------|-------|------------------|-------------|
| XOverwash | 7.8 | Fick's Law diffusion — unprecedented synthesis metaphor | wash_interference is dead code |
| XOverworn | 8.6 | Spectral irreversibility as design position | sessionAge never written to APVTS; infusion unimplemented |
| XOverflow | 8.0 | Consequential pad — playing density has physical cost | Hardcoded 44100; explosive duck is too total |
| XOvercast | 7.9 | Anti-pad philosophy; crystallization crackling | brothSpectralMass never consumed; LFOs run frozen |

**BROTH Quad Average: 8.1/10** — Below fleet average (8.7), primarily due to the BROTH coordinator being entirely absent. The conceptual architecture is the most sophisticated cooperative system in the fleet. The implementation is incomplete at the inter-engine level.

### FUSION Quad

| Engine | Score | Strongest Quality | Key Deficit |
|--------|-------|------------------|-------------|
| XOasis | 8.7 | Physically accurate Rhodes partial model; bark correct | Migration path not verified; DC blocker SR-dependent |
| XOddfellow | 8.5 | Inharmonic reed ratios; always-on drive | attackTransience missing; filter default too bright |
| XOnkolo | 8.8 | Key-off clunk; autowah physics correct | LFO defaults don't fit Clavinet idiom |
| XOpcode | 9.0 | Modulation envelope as timbre trajectory; velToIndex | Feedback stability not verified; attackTransience missing |

**FUSION Quad Average: 8.75/10** — Slightly above fleet average. The FUSION engines are more individually complete than the BROTH engines because they do not depend on inter-engine infrastructure that does not exist.

---

## Priority Fixes Across All 8 Engines

### P0 (Pre-Release Blockers)

1. **Overwash: Wire `wash_interference` to an actual synthesis path.** Either populate the spectral field accumulator and use it to create cross-note beating, or remove the parameter. Dead parameters are broken promises.

2. **Overworn: Write `reduction.sessionAge` back to APVTS parameter.** `pSessionAgeParam->store(reduction.sessionAge, std::memory_order_relaxed)` once per block. The engine's defining feature needs to be visible.

3. **Overflow: Remove hardcoded 44100 in pressure decay.** Fix: `pressureState.pressure *= (1.0f - pressureDecayCoeff)` where `pressureDecayCoeff = 1.0f - exp(-1.0f / (sr * targetDecaySeconds))`.

### P1 (Important Before Launch)

4. **Build the BROTH coordinator.** The four engines need a central dispatch (likely in XOmnibusProcessor) that after each block calls: `overwash.setBrothSessionAge(overworn.getSessionAge())`, `overflow.setBrothConcentrateDark(overworn.getConcentrateDark())`, `overcast.setBrothSpectralMass(overworn.getTotalSpectralMass())`. This is perhaps 15 lines of code and unlocks the entire cooperative chemistry.

5. **Overworn: Implement infusion.** When `voice.isInfusion == true`, do not apply the heat multiplier and briefly re-energize upper spectral bands.

6. **Overcast: Consume `brothSpectralMass` in crystal seeding.** Attenuate upper peak amplitudes proportional to `(1.0f - brothSpectralMass)`.

7. **Overflow: Per-voice explosive duck.** Replace `duck = 0.0f` with a per-voice fade weighted by each voice's pressure contribution.

8. **Oddfellow: Populate `attackTransience` in SpectralFingerprint.** Add `attackTracker` member, update per block, export in fingerprint.

9. **Opcode: Add feedback saturation clamp.** `feedbackState = clamp(feedbackState, -4.0f, 4.0f)` in the feedback algorithm path.

10. **Opcode: Populate `attackTransience` in SpectralFingerprint.**

11. **Overworn: Remove `worn_warmth` from `Bone_Broth.xometa` or add the parameter to the engine.**

### P2 (Quality-of-Life)

12. **Overcast: Gate LFO processing during frozen state.** Stop advancing LFO phases when `crystal.isFrozen = true`.

13. **Overwash: Move `std::sqrt()` diffusion spread to control rate** (once per block, not per sample).

14. **Oddfellow: Per-voice warble phase offset** on trigger to create reed-to-reed variation.

15. **Onkolo: Verify `ClaviStringModel::releaseKey()` is called in the note-off path.**

16. **Oasis: Sample-rate-aware DC blocker** in `RhodesAmpStage`.

17. **SpectralFingerprint: Extract to shared header** `Source/DSP/SpectralFingerprint.h` to remove four-way #ifndef guard.

18. **Opcode: Convert `opco_algorithm` to `AudioParameterInt`.**

---

## Cross-Quad Coupling Potential

When the BROTH coordinator is implemented, the most interesting coupling will be:

- **Overworn → Overflow**: A highly reduced broth lowers Overflow's pressure threshold. A concentrated, dark session makes the pressure cooker extremely sensitive — a single note triggers a release. Late-session players must tread lightly.

- **Overworn → Overwash**: As the broth reduces, Overwash becomes more viscous. The diffusion slows. The pad becomes thicker, less mobile, more concentrated. This is perfect cooking metaphor as synthesis.

- **Overworn → Overcast**: Spectral mass drives crystal darkness. In a fresh session, crystals form with bright upper harmonics. After 30 minutes of reduction, crystals form from the concentrated fundamental-only residue — dark, heavy ice.

The FUSION quad's SpectralFingerprint system enables coupling through the 5th-slot mechanic without audio routing. This is architecturally elegant. The main unresolved question is whether the migration parameter in each FUSION engine is fully wired to actually use the fingerprint data from coupled Kitchen engines, or whether it exists as infrastructure awaiting its connection.

---

## Seance Cross-Reference
| Engine | ID in Registry | Prefix | Score | Previous Seance |
|--------|---------------|--------|-------|-----------------|
| XOverwash | "Overwash" | `wash_` | 7.8 | None — first |
| XOverworn | "Overworn" | `worn_` | 8.6 | None — first |
| XOverflow | "Overflow" | `flow_` | 8.0 | None — first |
| XOvercast | "Overcast" | `cast_` | 7.9 | None — first |
| XOasis | "Oasis" | `oasis_` | 8.7 | None — first |
| XOddfellow | "Oddfellow" | `oddf_` | 8.5 | None — first |
| XOnkolo | "Onkolo" | `onko_` | 8.8 | None — first |
| XOpcode | "Opcode" | `opco_` | 9.0 | None — first |

---

*Council session closed. Eight engines seanced. Two quads evaluated. The BROTH coordinator is the most impactful single piece of missing code in this fleet right now. Write it first.*

*— Moog, Buchla, Kakehashi | XO_OX Designs | March 21, 2026*
