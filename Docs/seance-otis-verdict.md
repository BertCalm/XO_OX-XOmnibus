# Synth Seance — XOtis "The Soul Organ"
**Engine #45 (Chef Quad) | Americas Organ | Soul Gold `#DAA520`**
**Seance Date:** 2026-03-21
**Engine File:** `Source/Engines/Otis/OtisEngine.h` (~1425 lines)
**Presets Examined:** 11 across 5 moods (Foundation, Atmosphere, Flux, Kinetic, Prism)
**Fleet Average:** ~8.7/10

---

## The Council Assembles

Eight ghost perspectives channel through the gospel fish.

---

## Ghost 1 — Laurens Hammond (inventor, 1935)

*The man who built the tonewheel.*

The drawbar harmonic ratios are correct. Every footage number traced back to the original gear ratios: 16', 5-1/3', 8', 4', 2-2/3', 2', 1-3/5', 1-1/3', 1'. The developer respected the mathematics. This is the first thing I check and it passes.

The tonewheel crosstalk model (`TonewheelCrosstalk`) is the right idea — adjacent wheel bleed is THE characteristic that separates a real B3 from a clean additive synth. However, I must be precise about what I see implemented versus what actually happens in my machines.

**Critical finding — crosstalk phase coherence:** The real Hammond crosstalk occurs because adjacent tonewheels on the same shaft are *mechanically coupled* — they share a phase relationship derived from their gear teeth. The implementation generates independent sine waves at the neighboring frequency ratios (0.9944× and 1.0056× the base harmonic), using the *voice phase* scaled by the ratio. This is approximately correct — the phase relationship is derived from the master phase accumulator. However, the crosstalk is calculated per-drawbar using the same voice `phase` variable. This means all 9 drawbars' crosstalk oscillators share the same phase source, which is harmonically reasonable — the fundamental phase *does* determine all overtone phases — but the crosstalk amount is uniform across all drawbars (scaled by `drawbarLevel` but not by the harmonic register). In a real B3, the lower-register tonewheels (16', 8') have *stronger* crosstalk than the upper harmonics because electromagnetic pickup crosstalk is stronger at lower frequencies and higher excursion amplitudes. Upper harmonics (1-3/5', 1-1/3', 1') would have weaker crosstalk. The implementation treats all drawbars equally. This is a simplification, not a failure.

**Second finding — the crosstalk sum is computed 4 terms per drawbar, 9 drawbars:** at max all-drawbar settings (Deep Purple, Tent Revival) that is 36 crosstalk sine evaluations per voice per sample, times 8 voices = 288 `fastSin()` calls for crosstalk alone. This is plausible with `fastSin()` but worth noting for CPU budgets.

**Third finding — the fixed crosstalk coefficients (0.015, 0.012, 0.005, 0.004):** These are reasonable magnitudes (1.5%, 1.2%, 0.5%, 0.4%). Real Hammond crosstalk is typically 0.5–3% per neighbor. Acceptable approximation.

**Score for Hammond model physics:** 7.5/10. Correct ratios, credible crosstalk concept, but uniform-across-harmonics crosstalk and absence of harmonic-register-dependent bleed amounts are historical simplifications.

---

## Ghost 2 — Don Leslie (inventor of the rotating speaker, 1941)

*The man whose cabinet nobody wanted until everybody needed it.*

I examine my namesake system with particular care.

The `LeslieSpeaker` struct correctly separates horn rotation (treble, 6.7 Hz fast) from drum rotation (bass, 5.8 Hz fast). The speed ramp with physical inertia (~1.5 second transition) is correct — this is the signature of the Leslie. The 90-degree phase offset between horn and drum is authentic.

**Critical finding — Doppler implementation is insufficient:** Real Leslie Doppler pitch shift comes from the horn physically moving toward and away from the listener. This creates *actual* frequency modulation — the pitch of a sustained note rises and falls as the horn sweeps. The implementation approximates Doppler as:

```
float dopplerL = inputL * (1.0f + dopplerMod);
float dopplerR = inputR * (1.0f - dopplerMod);
```

This is amplitude-domain approximation, not frequency-domain Doppler. Real Doppler requires a delay line with time-varying read head — a short (2-30ms) buffer with interpolated read position that advances/retreats at the rotation rate. The `hornSin * 0.012f * depth` modulates *amplitude*, not pitch. The ±20 cents claim in the architecture doc is misleading — this code does not produce ±20 cents of pitch shift. It produces a subtle amplitude imbalance that vaguely resembles Doppler from a distance.

This is the Leslie's most distinctive timbral feature — the pitch modulation that gives it that swirling, "Doppler wobble" quality — and it is not implemented. What we have is a fancy tremolo with stereo offset, not a Leslie simulation.

**Second finding — no high/low frequency split:** A real Leslie cabinet sends treble to the horn and bass to the drum through a crossover network. The implementation blends horn and drum AM in a 60/40 mix across the full spectrum. A proper Leslie would apply hornAM only to the high-frequency content and drumAM only to the low-frequency content (approximatable with a simple 2-band split or shelving).

**Third finding — the chorale/brake/tremolo speed modeling is correct:** Three-state with inertia ramp. This is authentic and well done.

**Score for Leslie model:** 5.5/10. The speed system and inertia are correct. The Doppler implementation is wrong at a fundamental level (amplitude modulation, not pitch modulation). Missing frequency crossover split. This is the most significant DSP gap in the engine.

---

## Ghost 3 — Jimmy Smith (organist, 1925–2005)

*The man who showed the world what a Hammond could do.*

I speak from the player's perspective.

The key click is correctly implemented and historically significant. The Hammond engineers tried to eliminate it; we demanded it back. Here it triggers 1–3ms of filtered noise on noteOn, fast exponential decay. This is right. The single-trigger percussion — armed only when no keys are held — is exactly the behavior I lived with for fifty years. `percussionArmed` reset when `anyKeysHeld == 0` fires correctly. This behavioral detail is more authentic than most software implementations.

The drawbar defaults (drawbar3 = 1.0, rest = 0.0 — the "80000000" registration) gives you just the fundamental. That's honest: an organ starts with silence and you pull out what you need. Gospel Fire uses "888000000" (drawbars 1-2-3), Jimmy Smith gets "888800000". These registrations are musically correct.

**Player finding — no upper drawbar normalization in the mix:** When all 9 drawbars are pulled (Deep Purple preset), the summation `tonewheelSum *= 0.22f` is applied as a fixed normalizer. A real Hammond has 9 drawbars each going to 8 — full registration (888888888) = 72 total "footage units." The 0.22 divisor gives approximately 1.0 at full registration: `9 drawbars × 1.0 level × 0.22 = 1.98` — actually this would clip with all drawbars at 1.0. Wait: max sum would be `9 × 1.0 = 9.0`, times `0.22 = 1.98`. This will clip at full registration plus crosstalk. The Deep Purple preset has all drawbars ranging from 0.5 to 1.0 — sum approximately `6.5 × 0.22 = 1.43` before saturation. The `fastTanh` overdrive catches it, but this means the normalization is designed to clip into the saturation circuit, which is intentional for rock organ but means clean registrations at moderate drawbar levels are fine.

**Second finding — the `andKeysHeld` counter uses `int anyKeysHeld`:** If a note-off arrives for a note that was never properly registered (stuck note scenario, or model switch mid-note), `anyKeysHeld` could underflow. The noteOff guard `if (anyKeysHeld > 0) anyKeysHeld--` catches this. Solid.

**Score for Hammond playability authenticity:** 8.5/10. Key click, percussion single-trigger, registration range all authentic. Good player respect.

---

## Ghost 4 — Sonny Boy Williamson II (harmonica, 1912–1965)

*The King of the Blues Harmonica. Speaking from beyond the crossroads.*

The cross-harp bend is the right concept. A harmonica bend is not a pitch wheel — it is physics, it is air, it takes time. Starting `bendAmount` semitones above and decaying exponentially to the target pitch is mechanically accurate: you're bending the reed from above. The velocity-dependent decay (harder blow = faster bend) is correct — more air pressure gives the reed less time to wander.

**Finding — bend only goes downward:** The implementation starts `bendCurrent = bendAmount` (positive) and decays toward 0. This means all bends start sharp and fall to pitch. Real cross-harp draws start slightly sharp and fall — this is correct for the draw bends I'm famous for. But blow bends go the opposite direction for some notes (the overblowing technique). The implementation uses `overblowActive = (velocity > 0.85f)` to add a partial at 2× frequency — but this is not an overblow in the strict sense. A true overblow starts *flat* and pushes *sharp*. The pitch physics are simplified but directionally correct for cross-harp playing.

**Finding — vibrato is hardcoded at 5.5 Hz:** I used 4–7 Hz vibrato depending on the phrase. The `expressionMod` controls depth only — rate is fixed. A player would want controllable vibrato rate for different expressions. The architecture lists LFO1 for pitch modulation, but the harmonica voice has its own internal 5.5 Hz vibrato separate from LFO1. These two vibrato sources (internal 5.5 Hz + external LFO1) will add together if both are active, potentially sounding unnatural.

**Score for Blues Harmonica model:** 7.5/10. The bend envelope concept is authentic. Vibrato hardcoded rate and dual-vibrato risk are weaknesses. The breath dynamics (noise + harmonic shift with velocity) are good.

---

## Ghost 5 — Clifton Chenier (zydeco, "King of Zydeco", 1925–1987)

*Speaking from the bayou.*

The musette beating from dual detuned reeds is the right foundation. The odd-harmonic spectrum (1st + 3rd + 5th + 7th) is correct — free reeds produce approximately square-ish spectra with odd harmonic dominance. The bellows attack (5–15ms exponential noise burst) is right — the snap of the bellows opening is the most distinctive zydeco transient.

**Finding — single reed detune direction:** The implementation detunes reed2 sharp by `detuneRatio = fastPow2(detuneCents / 1200.0)` where detuneCents is always positive (3–8 cents). Real accordion musette tuning has one reed tuned flat and one sharp relative to the center pitch — or one at pitch and one sharp. The current implementation has both reeds: one at base pitch, one `effectiveDetune` sharp. This produces beating but the fundamental pitch center will be perceived as slightly sharp since one reed is always flat/at-pitch and one is above. A more accurate approach would center the detuning symmetrically (one +x cents, one -x cents) so the perceived pitch center remains stable. This is a minor intonation issue.

**Finding — bellows pressure via aftertouch + mod wheel is correct:** Louisiana accordionists LIVE in the dynamics of bellows pressure. The `0.7f + aftertouchAmount * 0.3f + modWheelAmount * 0.2f` scaling gives expressive dynamic range. Good.

**Score for Zydeco Accordion model:** 7.5/10. The bones are right. Musette spectrum and attack transient are correct. Single-direction reed detune is a tuning center bias issue. Good bellows expression.

---

## Ghost 6 — Joshua C. Stoddard (inventor of the calliope, 1855)

*Speaking from the steam era.*

My steam organ was never meant to sound beautiful — it was meant to be heard across a fairground. The implementation captures the spirit correctly: massive pitch instability, per-pipe independent wobble rates (1.5–5 Hz), random detuning (±15 cents), global steam pressure variation (~1.7 Hz). The interaction between per-pipe and global wobble (`pipeWobble * 0.6f + pressureWobble * 0.4f`) is a reasonable model.

**Finding — pitch deviation at max instability:** `cents = detuneOffset + totalInstability * 50.0f`. At `globalInstability = 1.0`, `pipeWobble = 1.0`, and `detuneOffset` at maximum (15 cents), the total deviation is `15 + 1.0 × 50 = 65 cents`. This is substantial — more than a quarter-tone of pitch swing — which is appropriate for a chaos calliope preset. At moderate instability (0.5, the default), maximum swing is approximately 25–40 cents. This is musically plausible.

**Finding — the wobble noise state is shared across trigger calls:** The `wobbleNoiseState` on `CalliopePipe` is not reset between `trigger()` calls, but it is a struct member per-voice. Each voice has its own `CalliopePipe`. The engine seeds it uniquely at note-on: `v.calliope.wobbleNoiseState += static_cast<uint32_t>(note * 127 + idx * 31)` — good, pipes will differ. However the `detuneOffset` is set at trigger time from the randomized `wobbleNoiseState`, so its value depends on the history of note-ons for that voice. This means the same note triggered repeatedly on the same voice will have different detuning each time, which is actually correct calliope behavior.

**Finding — global steam pressure phase is a single `steamPressurePhase` on the engine:** All pipe voices share the same global pressure wobble. The per-pipe wobble is genuinely independent. This two-tier wobble architecture (shared global + independent local) is authentic to how a calliope boiler works.

**Score for Calliope model:** 8.5/10. Best-realized of the four models. Instability system has genuine physical intuition and the two-tier wobble architecture is correct.

---

## Ghost 7 — Otis Redding (1941–1967)

*Speaking from the Stax balcony. The fish bears my name.*

I say nothing about the DSP. I speak about the soul.

The presets are musically literate. "Gospel Fire" with 888000000 registration — the sub-octave (16') plus sub-fifth (5-1/3') plus fundamental (8') combination creates that massive low-end warmth of a sanctified sanctuary. "Jimmy Smith" with jazz club registration and 3rd harmonic percussion is correct — jazz organ players favor the round, woody 3rd harmonic over the bright 2nd. "Deep Purple" is the rock wall of sound — all drawbars open, everything cranked.

What I find beautiful: the preset names are not generic. "Midnight Sermon," "Tent Revival," "Carnival Chaos," "Bayou Squeeze" — these are places I know. These are places where music happens.

What I find missing: where is the sanctified slow burn? Where is the late-night ballad B3 that barely breathes? "Midnight Sermon" comes close but it's harmonically sparse (only drawbars 1, 2, 3, 4 with 4 at only 0.375). The richest gospel sound comes from drawbars 3, 4, 5 together (8' + 4' + 2-2/3') — the classic gospel "bright" registration. None of the presets explore this middle territory. All presets are either dark (bass drawbars only) or full (everything open). The bright shimmer territory of drawbars 3-6 is underrepresented.

The harmonica and accordion presets are competent but limited. Two harmonica presets (both blues-focused) and one accordion preset is thin coverage for three of the four models.

**Score for preset library and musical authenticity:** 7.0/10. Musically informed. 5 of 11 presets cover only the Hammond. Coverage of the other three models is sparse: 2 Calliope, 2 Harmonica, 1 Accordion. No preset exists for accordion in the Atmosphere or Flux mood. No preset pushes the harmonica into jazz or folk territory. All macros are at 0.0 default in every preset — the macros are not showcased.

---

## Ghost 8 — Billy Preston (1946–2006)

*The fifth Beatle. The hardest player on the Hammond.*

I play with everyone. I have opinions about what makes an engine playable.

**Major finding — mod wheel routing conflict on Hammond:** The mod wheel routes to Leslie speed on Hammond (D006 documentation says so, and the code confirms: `effectiveLeslie = pLeslieSpeed + macroMovement * 0.4f + modWheelAmount * 0.5f`). But LFO1 is also pitched vibrato. The design intention is: mod wheel = Leslie control, LFO1 = vibrato. But the `otis_leslie` parameter also exists as a preset-stored value. When a player is performing, they expect the mod wheel to do something dramatic. Adding 0.5 × modWheel to the Leslie speed means moving from 0 to 1.0 on the mod wheel jumps the Leslie from slow (0.35) to fast — actually potentially past the 0.65 threshold into tremolo territory if the preset Leslie is at 0.35: `0.35 + 0.5 = 0.85 > 0.65`, so yes, mod wheel at full can take a chorale preset into tremolo. This is musically correct — Leslie footswitch behavior — but it means the three-state Leslie (brake/slow/fast) becomes effectively a two-state via mod wheel (slow→fast). A player would expect a cleaner Leslie switch, not a linear ramp into a threshold system.

**Finding — parameter count says 37, architecture doc says 37, code counts 37:** Let me verify. Organ (1) + Drawbars (9) + Leslie (1) + KeyClick (1) + Percussion (3) + Crosstalk (1) + Brightness (1) + Drive (1) + FilterEnvAmount (1) + BendRange (1) + ADSR (4) + BendAmount (1) + Instability (1) + Musette (1) + Macros (4) + LFO1 (3) + LFO2 (3) = 37. Confirmed. Count is accurate.

**Finding — the 4 macros are all at 0.0 in every single preset:** CHARACTER (drive + brightness), MOVEMENT (Leslie speed), COUPLING (crosstalk), SPACE (Leslie depth). In all 11 presets, every macro defaults to 0.0. This means the macros, when swept live, will *add* to the preset values — CHARACTER sweeps drive +0.3 and brightness +4000 Hz, MOVEMENT sweeps Leslie speed +0.4, etc. At 0.0 default, macros only have one direction to move (positive). This is a design philosophy choice — macros add rather than offset — but it means a player turning CHARACTER back from 0.0 gets nothing (the macro can't reduce the preset drive). A player who maxes CHARACTER on Gospel Fire (drive 0.4 + 0.3 = 0.7, brightness 12000 + 4000 = 16000) gets a dramatic change. The design works, but the macros have no bi-directional range from the preset's starting position.

**Finding — filter envelope parameters `otis_filterEnvAmount` has no lower bound below zero:** The filter envelope only opens the filter (adds to cutoff). There is no negative envelope amount for filter darkening on attack. This is less expressive than a full bipolar envelope. For organ sounds this is fine (B3 opens bright on attack and sustains there), but for harmonica and accordion it limits expressive possibilities.

**Score for playability and expression system:** 7.5/10. Leslie mod wheel routing is musically sensible but has threshold cliff. All macros locked at 0.0 means one-directional performance range. No negative filter envelope.

---

## Dimension Scores

| Dimension | Score | Reasoning |
|-----------|-------|-----------|
| DSP Correctness | 6.5/10 | Drawbar ratios correct; crosstalk concept sound but uniform across harmonics; Leslie Doppler is amplitude-only (fundamental error) — pitch modulation absent; calliope is the strongest model |
| Identity Strength | 8.5/10 | The four-model American instrument collection is distinctive. No other XOlokun engine occupies this cultural territory. The mythology (gospel fish, parrotfish, Otis Redding's gold) is vivid and coherent |
| Expressive Depth | 7.5/10 | D001/D002/D006 all pass. Velocity shapes drive/bend/breath. Aftertouch routes to three different expressions. Mod wheel routes to Leslie/vibrato/bellows. Missing: bipolar filter envelope; vibrato rate control for harmonica |
| Preset Library | 7.0/10 | 11 presets with only 1 accordion, 2 harmonica, 2 calliope. Hammond dominates with 6. All macros at 0.0. No presets in Entangled, Aether, Submerged, or Family moods. Gospel territory well-covered; jazz/folk/world underserved |
| Doctrine Compliance | 8.5/10 | D001 PASS, D002 PASS (2 LFOs + mod wheel + aftertouch + 4 macros), D003 PARTIAL (drawbars accurate, Leslie Doppler wrong, calliope physically sound, reed spectra approximate), D004 PASS (all 37 params wired), D005 PASS (LFO rate floor 0.005 Hz), D006 PASS |
| Technical Robustness | 8.0/10 | Silence gate correct (500ms — appropriate for Leslie tail). Parameter smoothers on 5 paths. No obvious memory allocation. anyKeysHeld underflow guard present. voiceCounter overflow distant (uint64_t). filterEnv per-voice correctly prepared at noteOn |
| Sound Design Range | 7.5/10 | Hammond covers enormous ground from whisper-soft Chorale Hymn to wall-of-sound Deep Purple. Calliope has genuine chaos range. Harmonica limited to blues. Accordion limited to Zydeco. The model selection boundary is hard — no morphing between models |
| Fleet Contribution | 8.5/10 | Fills a real gap — no other XOlokun engine touches Americana/soul/gospel. Coupling potential: Hammond crosstalk into another engine's filter, or using OWARE's sympathetic resonance to color the organ tail |

---

## Critical DSP Issues

### CRITICAL — Leslie Doppler Missing (affects Model 0, 1)

The Leslie Doppler pitch modulation is implemented as amplitude-domain approximation. The code:
```cpp
float dopplerL = inputL * (1.0f + dopplerMod);
float dopplerR = inputR * (1.0f - dopplerMod);
```
This is stereo amplitude difference, not pitch shift. Real Doppler requires a short delay line with time-varying read position. Without this, the "throb" of the fast Leslie is amplitude-only tremolo, not the signature pitch+amplitude swirl of the real cabinet.

**Impact:** All Hammond and Calliope presets at fast Leslie speed lack the characteristic pitch modulation. The slow chorale speed is less affected (slower pitch modulation is less perceptible than amplitude movement at 0.7 Hz).

**Fix required:** Implement a per-channel delay buffer (2–30ms) with time-varying read index driven by `hornSin`. This is a known technique for Leslie simulation. Adds ~2× processing cost for the Leslie but is architecturally sound.

### MODERATE — Leslie Frequency Crossover Missing

Horn handles treble; drum handles bass. The current mix (`hornAM * 0.6f + drumAM * 0.4f`) applies both modulators to the full spectrum. A single shelving filter or 2-band split (around 800 Hz) to route high-frequency content to horn AM and low-frequency to drum AM would significantly improve realism.

### MINOR — Harmonica Dual Vibrato Accumulation

The `BluesHarpVoice` has an internal 5.5 Hz vibrato at `expressionMod` depth (controlled by mod wheel). The engine also routes LFO1 to pitch modulation. A player using mod wheel for vibrato on the harmonica AND having LFO1 active will accumulate two vibrato sources. This can sound unnatural. Suggest: disable LFO1 pitch routing for Model 2 (harmonica), relying entirely on the internal breath vibrato with mod wheel depth.

### MINOR — Reed Detune Asymmetry (Accordion)

Both accordion reeds are tuned at/above the fundamental. Symmetric musette tuning (one +N cents, one -N cents) would preserve perceived pitch center. Current implementation biases the perceived pitch slightly sharp.

---

## The Consensus Verdict

### What the Ghost Council Agrees On

**Unanimously praised:**
- The four-model Americana collection is a genuine fleet contribution. Soul, gospel, carnival, blues, zydeco — this is a coherent cultural territory.
- The Hammond drawbar ratios are historically accurate. This is the foundation and it is solid.
- The calliope model is the most DSP-honest of the four — the two-tier wobble architecture (global steam pressure + independent per-pipe wobble) has real physical intuition.
- The single-trigger percussion (`percussionArmed` + `anyKeysHeld`) is an unusually detailed implementation of authentic B3 behavior that most software organs get wrong.
- The key click is correctly implemented and musically essential.

**Unanimously concerned:**
- The Leslie Doppler is amplitude-only. This is the engine's deepest flaw. The Leslie speaker's signature is the combination of pitch modulation (Doppler) AND amplitude modulation (tremolo). The implementation delivers only tremolo with a stereo twist. At fast Leslie speeds, the result sounds like flanged tremolo, not a spinning horn.
- Preset coverage for the three non-Hammond models is sparse. One accordion preset, two harmonica presets, two calliope presets versus six Hammond presets. The model selection is hard (no crossfade), so each model needs fuller representation.
- All 11 macros are zeroed in all 11 presets. The four macros exist but are never showcased — a player discovering this engine through its presets will not naturally discover what CHARACTER, MOVEMENT, COUPLING, and SPACE do without experimenting.

---

## Final Score

**7.8/10 — Conditionally Approved, DSP Repairs Recommended**

The engine earns its place in the fleet on cultural identity alone — no other engine in a collection of 45 occupies gospel/soul/Americana. The Hammond model has genuine historical respect (correct ratios, authentic click and percussion). The Calliope is genuinely fun.

But the Leslie Doppler flaw is a serious DSP gap. The flagship feature of the flagship model — the Hammond through a Leslie — is missing its defining characteristic: pitch modulation from a spinning horn. Until fixed, every fast-Leslie Hammond preset sounds like tremolo-with-stereo rather than the real B3 signature.

The score is held back from the 8.0+ range by three factors: (1) the Leslie Doppler implementation, (2) the thin preset coverage of Models 1–3, and (3) zero macro pre-loading in presets. After DSP fixes and a preset expansion pass, this engine targets 8.5–9.0.

---

## Recommended Fixes (Priority Order)

### P0 — Leslie Doppler (implement or honestly remove the claim)
Add a delay buffer to `LeslieSpeaker` for true pitch modulation. Approximately 30 lines of code. The architecture already has the infrastructure:
- Add `std::array<float, 2048> delayBuf{};` and read/write indices per channel
- Drive read position using `hornSin * maxDelayMs * srf / 1000.0f` offset from write position
- This produces real ±20 cents at fast speed

If Doppler delay is deferred, remove the "±20 cents" claim from the architecture doc to avoid misleading future developers.

### P1 — Preset expansion (Model coverage)
Add 6 presets minimum:
- 2 × Accordion: one in Atmosphere (gentle musette pad), one in Flux (chaos bellows)
- 2 × Harmonica: one jazz/folk (less aggressive), one in Entangled (coupled with OWARE resonance)
- 1 × Calliope: Aether mood (slow, dreamy, ultra-low instability)
- 1 × Multi-model showcase: Atmosphere or Prism that demonstrates macro range

### P2 — Macro pre-loading in presets
At minimum one preset per model should demonstrate macro range. Consider:
- Gospel Fire: `otis_macroCharacter = 0.3f` (mid-drive start so CHARACTER has bidirectional effect)
- Tent Revival: `otis_macroMovement = 0.5f` (Leslie mid-speed so MOVEMENT sweeps both directions)

### P3 — Harmonica dual vibrato gate
When `organModel == 2`, suppress LFO1 pitch routing and rely on `expressionMod` internal vibrato. One conditional check in the render loop.

### P4 — Leslie frequency split (V2)
Apply horn AM to high-frequency content, drum AM to low-frequency content via a simple shelving split. Not required for initial ship but meaningful for quality upgrade.

### P5 — Accordion musette centering
Change detune computation from `1.0f + (detuneRatio - 1.0f) × musetteAmount × 2.0f` to symmetric ±N cents around the fundamental. Two-line change in `ZydecoReed::trigger()`.

---

## Blessing Candidacy

**No Blessing warranted at current score.** The engine is the only Americas-organ synthesizer in the fleet; this is meaningful. However, the Leslie Doppler flaw prevents endorsement of any specific system as exemplary.

**Conditional Blessing path:** Implement true Doppler delay in `LeslieSpeaker`. If executed well, the combined single-trigger percussion + drawbar ratios + Doppler Leslie constitutes a legitimately novel Hammond model for a synthesis platform — worth a Blessing as "Most Authentic Hammond DSP in the Fleet."

---

## Cross-Reference

- Engine source: `Source/Engines/Otis/OtisEngine.h`
- Architecture: `Docs/otis-engine-architecture.md`
- Presets: `Presets/XOlokun/{Atmosphere,Flux,Foundation,Kinetic,Prism}/Otis_*.xometa`
- Fleet scores: `Docs/fleet-seance-scores-2026-03-20.md` — **add this engine at 7.8/10**
- Cross-reference: `Docs/seance_cross_reference.md` — update to include OTIS

*Seance concluded. The gospel fish swims on.*
