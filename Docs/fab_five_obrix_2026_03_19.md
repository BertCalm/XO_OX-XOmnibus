# The Fab Five Makeover -- OBRIX
**Date**: 2026-03-19
**Target**: ObrixEngine.h (1424 lines, 65 params, Waves 1-3 complete)
**Intensity**: Makeover (full treatment)

---

## Phase 1: The Walk-Through

OBRIX is at a fascinating inflection point. Three waves of DSP development are complete, 174 factory presets exist across all 7 moods, the seance council scored it 7.2/10 with a roadmap to 9.4/10. The architecture is genuinely novel within XOlokun -- a brick-pool modular synth where you snap ocean bricks together. The Constructive Collision signal flow (split processing, post-mix insert) is a real design idea, not just a topology.

What is working: the brick pool metaphor is coherent, the signal flow is correct, the DSP is solid (CytomicSVF, PolyBLEP, matched-Z spatial filters), Wave 3 additions (Drift Bus, Journey Mode, Spatial) add genuine depth. 174 presets with evocative names. Coupling architecture with brick complexity output is clever.

What needs the Fab Five: the code carries visible signs of being built in waves -- naming inconsistencies between Wave 1 and Wave 3, the header comment growing but not restructured, internal helpers that lack the poetic quality of the best XOlokun engines (compare ONSET's creature identity header). The brick metaphor is stated but not deeply threaded through the code itself.

---

## Phase 2: The Five Specialists

---

### F1 -- The Stylist (Visual Presentation)

*"Does this look like it was designed, or like it just happened?"*

#### Findings

**1. The header block is informational but not evocative.**
Compare OBRIX's opening:
```
// ObrixEngine -- Ocean Bricks: The Living Reef
// A runtime-configurable synthesis toy box where you snap "ocean bricks"
// together to build sound.
```
To ONSET's opening, which tells a creature story: where it lives in the water column, what it does, what sonic lineage it draws from. OBRIX reads like a product spec. The "baby brother of XOlokun" line diminishes the engine's identity -- flagship engines do not call themselves baby brothers.

**Recommendation**: Rewrite the header as a creature identity block. OBRIX is a coral reef: a living structure that grows by accretion, each polyp (brick) a simple organism that creates complex architecture through accumulation. The reef does not design itself -- it emerges. This is the story.

**2. Enum naming is functional, not evocative.**
`ObrixSourceType`, `ObrixProcType`, `ObrixModType`, `ObrixEffectType` -- all correct, all forgettable. The brick metaphor names them as Shells (sources), Coral (processors), Currents (modulators), and Tide Pools (effects) in the comment, but these beautiful names never appear in the code itself.

**Recommendation**: Add brief inline comments on the enum declarations that echo the mythology:
```cpp
enum class ObrixSourceType {  // Shells -- the living generators
```
This costs nothing and rewards every reader.

**3. Preset naming is strong overall, with standouts.**
`Reef at Noon`, `Tide Pool Morning`, `Coral Memory`, `Journey Begin`, `The Collision`, `Storm Inside` -- these are evocative and brand-aligned. The `TB_` (Transcendental Batch) naming convention is clear. The Family presets (`Reef x Onset`, `Reef x Opal`) are well-structured.

**Weaknesses**: Some Foundation presets are too instructional: `Add Filter`, `Chorus Width`, `Delay Echo`, `FM Depth`, `Reverb Space`. These are lessons masquerading as presets. The names describe what the preset demonstrates rather than what it sounds like.

**Recommendation**: Rename lesson presets to carry their pedagogical purpose in the tags while giving the preset an evocative name:
- `Add Filter` --> `First Reef Wall` (tag: lesson, filter)
- `Chorus Width` --> `Tide Shift` (tag: lesson, chorus)
- `Delay Echo` --> `Coral Echo` (tag: lesson, delay)
- `FM Depth` --> `Harmonic Undertow` (tag: lesson, fm)
- `Reverb Space` --> `Open Water` (tag: lesson, reverb)

**4. Parameter display names lack poetry.**
`Obrix Source 1 Type`, `Obrix Proc 1 Cutoff`, `Obrix Effect 2 Mix` -- technically correct, but compare to the internal language: Shells, Coral, Tide Pools. The display names users see in DAW automation could carry the mythology.

**Recommendation** (low priority, post-V1): Consider `Obrix Shell 1 Type`, `Obrix Coral 1 Cutoff`, `Obrix Tide Pool 2 Mix` for display names. Parameter IDs must never change, but display names can be updated.

| Area | Score Before | Score After |
|------|-------------|------------|
| Header identity | 5 | 8 |
| Preset naming | 7 | 8.5 |
| Code aesthetics | 6 | 7.5 |

---

### F2 -- The Polisher (Code Elegance)

*"When someone reads this code, do they feel respect -- or neglect?"*

#### Findings

**1. The `renderBlock` method is a 435-line monolith (lines 326-761).**
This is the most significant polish issue. The method handles: parameter snapshot, FLASH gesture, MIDI processing, coupling accumulators, the full sample loop (gesture envelope, drift bus, voice rendering with portamento, modulation, source rendering, split processor routing, wavefolder, ring mod, amp envelope, panning), effects chain, spatial processing, level scaling, buffer writing, voice counting, and brick complexity calculation.

Compare to best-practice engines like ONSET, which factor out `processVoice()`, `processMIDI()`, `processEffects()` as separate methods. OBRIX's monolith is functionally correct but violates code rhythm.

**Recommendation**: Factor the sample loop body into named methods:
- `processGestureEnvelope()` -- lines 489-502
- `processVoiceSample()` -- lines 510-686 (the per-voice inner loop)
- `processSpatial()` -- lines 699-728

This does not change the DSP or the header-only architecture. It names what is currently anonymous.

**2. Magic numbers are mostly documented but a few remain bare.**
Good: `0.23f` is documented as "irrational spacing" for Drift Bus. The reverb comb lengths `{1087, 1283, 1511, 1789}` are annotated as "prime-adjacent."

Bare: `charFoldScale = 1.0f + macroChar * macroChar * 8.0f` -- what is 8.0f? Why squared? `cutoffMod += modWheel_ * 4000.0f` -- why 4000? `pitchMod += macroMove * 100.0f` -- 100 cents = 1 semitone, worth stating.

**Recommendation**: Add brief inline comments for the scaling constants that explain the musical intent:
```cpp
float charFoldScale = 1.0f + macroChar * macroChar * 8.0f; // exponential fold: 1x-9x drive
cutoffMod += modWheel_ * 4000.0f;  // mod wheel sweeps up to 4 kHz (musically useful range)
pitchMod  += macroMove * 100.0f;   // MOVEMENT: +1 semitone stereo detune at full
```

**3. Variable naming is generally good but has inconsistencies.**
- `sr` (sample rate) is terse but universal in DSP code -- acceptable.
- `vi` for voice index is fine.
- `sig1`, `sig2` for processed signals is clear.
- `wL`, `wR` for wet signals is too compressed -- `wetL`, `wetR` reads better.
- `s` for sample index in the main loop shadow the common `s` for signal -- `sampleIdx` or `n` (DSP convention) would be cleaner.
- `dt` for phase increment is correct DSP notation.

**4. The `applyEffect` method packs three full effect implementations into one switch.**
Each case (Delay, Chorus, Reverb) is 15-30 lines of self-contained DSP. These could be `applyDelay()`, `applyChorus()`, `applyReverb()` for readability. The current structure means you must read the full switch to find the reverb implementation at line 1158.

**5. Comment quality is mixed.**
Excellent: `"Schulze's ultra-slow ensemble LFO"`, `"Tomita's scene-making"`, `"Guru Bin fix"`, `"Guild Lo-Fi specialist request"` -- these comments tell stories. They credit the design lineage.

Adequate: `"// epsilon prevents denormals at floor"` -- functional but not inspiring.

Missing: The Constructive Collision signal flow (lines 612-686) is the heart of the engine but has no poetic framing. A 2-line comment acknowledging this as the defining moment of OBRIX's sound would elevate the reading experience.

| Area | Score Before | Score After |
|------|-------------|------------|
| Method decomposition | 4 | 7 |
| Naming clarity | 7 | 8 |
| Comment quality | 7 | 8.5 |
| Magic number docs | 6 | 8 |

---

### F3 -- The Architect (Structural Beauty)

*"Does the space flow? Does everything have a home?"*

#### Findings

**1. Single-file architecture is the right call for this engine.**
1424 lines in one header is at the upper end of comfortable, but OBRIX is self-contained: no external DSP modules beyond the shared `CytomicSVF`, `PolyBLEP`, and `FastMath`. The brick pool is inherently local -- voices own their bricks. Splitting into multiple files would scatter a coherent design.

**2. Struct ordering tells the right story.**
The file reads top-down as: Enums (brick types) --> ADSR --> LFO --> FX State --> Voice --> Engine. This is the correct order: primitives first, composites second, the system last. A reader builds understanding incrementally.

**3. Missing: a `BrickPool` or `VoiceProcessor` intermediate abstraction.**
Currently, source rendering, processor routing, modulation routing, and effects processing all live directly in `ObrixEngine::renderBlock`. The engine class is doing the work of the bricks rather than delegating to them. While a full refactor would be overkill for V1, the conceptual layers are clear:

```
Conceptual layers (for future wave):
  BrickSource      -- oscillator rendering + FM
  BrickProcessor   -- filter + feedback + wavefolder + ring mod
  BrickModulator   -- envelope + LFO + velocity + aftertouch routing
  BrickTidePool    -- delay + chorus + reverb
```

This is an architectural observation, not a V1 recommendation. Document the aspiration.

**4. Parameter declaration symmetry is excellent.**
The `addParametersImpl` method groups cleanly: Sources (7) --> Processors (9) --> Amp Envelope (4) --> Modulators (16) --> Effects (9) --> Level (1) --> Macros (4) --> Voice Mode (1) --> Expression (2) --> FLASH (2) --> Wave 2 (5) --> Wave 3 (5). Each group is commented with its count. This is well-structured.

**5. Wave 2/Wave 3 additions are appended rather than integrated.**
The parameter pointers section has three blocks: original, Wave 2, Wave 3. The `renderBlock` param snapshot similarly has `// Wave 2 params` and `// Wave 3 params` sections appended at the end. This layered history is honest but makes the final architecture feel like a timeline rather than a design.

**Recommendation**: For V1.4 or post-launch, reorganize the private members into logical groups (sources, processors, modulators, effects, spatial, state) rather than chronological groups (original, wave 2, wave 3). The wave comments could become inline history notes rather than structural dividers.

**6. Lifecycle symmetry is good.**
`prepare()`, `releaseResources()`, `reset()` form a clean triple. All voice/FX state is properly initialized. The `buildWavetables()` call in `prepare()` is correctly placed.

**7. Missing: SilenceGate / SRO integration.**
Peer engines (OPAL, ONSET) use `SilenceGate` from the SRO (Smart Resource Optimization) system. OBRIX does not. When all voices are idle, OBRIX still runs the full sample loop, spatial filters, and effects chain on silence. Adding SRO would be a meaningful efficiency improvement for multi-engine setups.

| Area | Score Before | Score After |
|------|-------------|------------|
| File structure | 8 | 8.5 |
| Abstraction layers | 6 | 7 |
| Lifecycle correctness | 8 | 8 |
| SRO integration | 3 | 3 (not yet implemented) |

---

### F4 -- The Sound Designer (Sonic Palette)

*"Does this nourish the ear, or just fill the silence?"*

#### Findings

**1. The wavetable banks are musically meaningful but small.**
Four banks (Analog, Vocal, Metallic, Organic) at 512 samples each. The Analog bank is a 12-harmonic additive saw -- functional but indistinguishable from the PolyBLEP saw at moderate frequencies. The Vocal bank emphasizes formant-like peaks (2nd + 4th harmonics) -- this is the most characterful. The Metallic bank's inharmonic stretch is subtle but real. The Organic bank's micro-detuning creates gentle beating.

**Recommendation**: The seance council specifically requested ocean-sourced wavetable banks. Consider adding 2-4 banks in a future wave:
- **Reef**: Recorded coral reef hydrophone spectra (bright, clicking, crackling)
- **Kelp**: Slow harmonic sway, beating fundamentals, green texture
- **Abyss**: Deep sub-harmonics, pressure-compressed spectra
- **Bioluminescent**: Irregular sparkle, stochastic brightness peaks

**2. The init patch (Sine + LP@8kHz) is too polite.**
The seance council flagged this unanimously. A sine wave through a fully-open lowpass filter is silence with a fundamental. The engine's character is invisible at first touch.

**Recommendation**: Change the default to Saw + LP@2500Hz + Mod1 Env->Cutoff depth 0.6. This gives the first keypress a sweep, a body, and a direction. The Constructive Collision should be one button press away, not a configuration exercise.

**3. The ADSR is linear attack, exponential decay/release.**
This is the correct topology for musical envelopes (linear attack avoids the initial sluggishness of exponential curves; exponential decay sounds natural). The denormal flushing in decay/release is proper. However, the decay equation `level -= dRate * (level - sLevel + 0.0001f)` creates an exponential curve that asymptotically approaches sustain, which is correct.

**4. FM implementation is musically useful.**
Src1 -> Src2 FM at +/-24 semitones deviation is the right range. At low depth values this creates subtle harmonic animation; at high values it crosses into classic FM metallic territory. The bipolar `fmDepth` (-1 to +1) allows inverted FM, which is a real sound design tool.

**5. Filter feedback with tanh saturation is the right choice.**
Self-oscillation via feedback into a tuned filter is how analog synths achieve their screaming character. The `fastTanh` saturation prevents runaway while preserving the harmonic character. The `* 4.0f` scaling on the feedback amount gives enough gain for self-oscillation without requiring extreme parameter values.

**6. Drift Bus is genuinely beautiful.**
The irrational phase offset (0.23) between voice slots creates ensemble drift that is mathematically guaranteed to never phase-lock. This is Schulze's technique, properly implemented. The ultra-slow rate range (0.001-0.05 Hz, periods of 20-1000 seconds) is correct for the Berlin School aesthetic.

**7. Spatial processing (Distance + Air) is well-designed.**
Matched-Z 1-pole LP for distance (air absorption) and LP/HP split with tilt for atmospheric character. The coefficient hoisting to block rate is the right optimization. The tilt range (0.7-1.3 gain ratio) is subtle enough to be usable across the full range.

**8. Missing sonic territory in the preset library.**
174 presets is substantial. However, scanning the names reveals gaps:
- Few presets showcase Journey Mode + Drift Bus together (the "Schulze" combination)
- No presets that explore extreme filter feedback (self-oscillating filter as a sound source)
- No presets that use Air at extremes (fully warm or fully cold atmosphere)
- The unison + FM combination is underexplored
- No "chaos" preset that pushes all parameters toward their boundaries simultaneously

| Area | Score Before | Score After |
|------|-------------|------------|
| Init patch | 3 | 7 (with recommendation) |
| Wavetable character | 6 | 6 (content expansion needed) |
| DSP correctness | 9 | 9 |
| Preset coverage | 7 | 8 (with gap-fill) |

---

### F5 -- The Storyteller (Brand Soul)

*"What story does this tell? Does it move people?"*

#### Findings

**1. The coral reef mythology is stated but not embodied.**
The header says "Ocean Bricks: The Living Reef" and the naming convention (Shells, Coral, Currents, Tide Pools) is beautiful. But these words appear only in the top-level comment block. They vanish as soon as you enter the code. The LFO is `ObrixLFO`, not a Current. The effect state is `ObrixFXState`, not a Tide Pool. The voice is `ObrixVoice`, not a Polyp.

This is the difference between mythology that is *bolted on* and mythology that is *grown through*. The code does not need to be renamed (parameter IDs are frozen), but the story could be more present in comments, method names, and internal documentation.

**2. The "baby brother" framing undermines the flagship identity.**
Line 18: "OBRIX is the baby brother of XOlokun." This is humble but wrong for a flagship engine. OBRIX should be framed as a reef -- an ecosystem within the ecosystem. A reef is not a baby anything; it is a foundational structure that supports entire worlds of life. The framing should be: OBRIX is the substrate. Other engines are the creatures that live on it.

**Recommendation**: Replace "baby brother" with language that positions OBRIX as the reef that supports the fleet:
```
OBRIX is the living reef of XOlokun -- a substrate engine that grows through
periodic brick drops, accumulating new organisms (source types, processor modes,
effect algorithms) like a reef accumulates coral. Each brick is simple; the
architecture they create together is complex, alive, and unique to each player.
```

**3. The "Constructive Collision" is a beautiful concept that deserves more stage time.**
The split processing topology (Src1->Proc1, Src2->Proc2, mix->Proc3) is called "the Constructive Collision" in the signal flow diagram but this name appears only twice in the entire file. For a concept this central, it should be the lens through which the engine is understood: two independent timbral streams meeting and being transformed by their meeting.

**4. Historical grounding is present but could be deeper.**
Schulze is credited (Drift Bus), Tomita is credited (Spatial), Pearlman is credited (default routing). The seance council ghosts are referenced. But OBRIX's modular brick-pool concept has rich historical lineage that is not mentioned:
- **ARP 2600**: semi-modular with normalled routing, but patchable -- OBRIX's philosophy
- **Korg MS-20**: two sources, two filters in series, a patchbay -- close structural cousin
- **Serge Modular**: "patch-programmable" rather than "modular" -- OBRIX's exact approach
- **Buchla 200**: voltage-controlled routing as a creative act, not just signal flow

Adding 1-2 lines of historical lineage to the header would ground the engine in real synth history.

**5. The coupling narrative is strong.**
Brick complexity as a coupling output (channel 2 of `getSampleForCoupling`) is a genuine innovation -- OBRIX doesn't just send audio, it sends architectural metadata. This means coupled engines can respond to *how complex* OBRIX's current configuration is, not just what it sounds like. This story should be told more prominently.

**6. Blessing B016 (Brick Independence) is the right soul anchor.**
"Bricks must remain individually addressable regardless of coupling state." This is the ethical core of the engine: each brick retains its identity even when connected to others. This is a philosophical statement about modularity, autonomy, and emergence. It maps perfectly to the coral reef: each polyp is independent, yet the reef is more than the sum of its polyps.

| Area | Score Before | Score After |
|------|-------------|------------|
| Mythology depth | 5 | 8 (with recommendations) |
| Historical grounding | 6 | 8 |
| Brand voice | 6 | 8 |
| Coupling narrative | 7 | 8.5 |

---

## Phase 3: The Reveal

### Before & After Summary

| Specialist | Before | After (projected) | Key Change |
|-----------|--------|---------|------------|
| F1 Style | 6.0 | 8.0 | +2.0 -- Header identity, preset rename, mythology in code |
| F2 Polish | 6.0 | 7.8 | +1.8 -- Method decomposition, magic number docs, naming |
| F3 Architecture | 6.5 | 7.5 | +1.0 -- SRO integration, layer documentation, wave reorganization |
| F4 Sound | 6.5 | 7.8 | +1.3 -- Init patch fix, wavetable expansion, preset gap-fill |
| F5 Soul | 5.5 | 8.0 | +2.5 -- Mythology embodiment, historical lineage, reef framing |
| **Overall** | **6.1** | **7.8** | **+1.7** |

### The Vibe Shift

Before the Fab Five: OBRIX reads as a technically competent engine with a beautiful concept (the brick pool, the Constructive Collision) that stays trapped in the comment block. The code beneath is solid DSP assembled in waves, honest about its history but not yet unified into a single coherent personality. The coral reef mythology is a label, not a lived experience.

After applying these recommendations: OBRIX becomes an engine whose code tells its own story. The reef metaphor breathes through comments and structure. The first keypress sounds like something worth exploring. The historical lineage grounds the design in real synth tradition. Readers of the code -- and players of the presets -- feel that every decision was intentional.

### What We Left for Next Time

1. **Wavetable expansion** (4 ocean-sourced banks) -- requires audio content creation, not just code
2. **SilenceGate integration** -- meaningful performance optimization, requires testing under multi-engine load
3. **Filter key tracking** (Moog's seance request) -- a Wave 4 DSP feature, not a style pass item
4. **LFO ceiling increase** (30 Hz -> 80+ Hz for audio-rate modulation) -- Schulze's seance request, DSP change
5. **renderBlock decomposition** -- the largest single improvement for code readability; should be done carefully to avoid introducing performance regressions from function call overhead in the inner sample loop (though with inlining hints, this is unlikely)
6. **Parameter display name mythology** (Shell/Coral/Tide Pool) -- post-V1, requires DAW testing for display length
7. **"Chaos" preset** and extreme-parameter preset coverage -- sound design session needed

---

## Phase 4: After-Care

### Aesthetic Choices to Document
- **Brick pool metaphor**: Shells (sources), Coral (processors), Currents (modulators), Tide Pools (effects)
- **The Constructive Collision**: two independent timbral streams meeting at the mix point
- **Accent**: Reef Jade `#1E8B7E` -- cool, organic, alive
- **Blessing B016**: Brick Independence as ethical and architectural principle
- **Historical lineage**: ARP 2600 (semi-modular with normals), Serge (patch-programmable), Schulze (drift), Tomita (spatial)

### The Style Bar (Best Files for Reference)
- `Source/Engines/Onset/OnsetEngine.h` -- the gold standard for creature identity headers and structural comments
- `Source/Engines/Opal/OpalEngine.h` -- parameter ID namespace pattern (`OpalParam::`) that OBRIX could adopt in V2

### Style Champions Within OBRIX
- The Drift Bus implementation (lines 504-589) -- clean, well-documented, mathematically grounded
- The wavetable `buildWavetables()` method (lines 1371-1421) -- clear, self-documenting, each bank has character
- The coupling architecture (lines 763-787) -- brick complexity as metadata is genuinely novel

### Style Debts
- `renderBlock` monolith (435 lines, no decomposition)
- Wave-layered private member organization (chronological, not logical)
- "Baby brother" framing in the header
- Lesson preset names that describe the feature rather than the sound
- Missing SRO/SilenceGate integration
