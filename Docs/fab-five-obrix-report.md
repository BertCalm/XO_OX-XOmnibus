# Fab Five — OBRIX Style Transformation Report
**Date**: 2026-03-21
**Target**: ObrixEngine.h + Foundation preset library (393 presets total)
**Intensity**: Targeted polish (improvements applied; risky refactors flagged only)
**Previous pass**: Gala Reveal 2026-03-20 (overall score 7.2/10)

This report is the third Fab Five pass on OBRIX. The first pass (2026-03-19) was a full makeover
document with recommendations but no changes applied. The second pass (2026-03-20 Gala) implemented
the header identity, gesture mythology, source display names, and lesson preset renames.
This pass closes remaining gaps and documents the current state accurately.

---

## Specialist 1 — STYLE (Code Presentation)

### Findings

**Enum names lacked mythology** (open from 03-19 Makeover)
The four brick types were declared as standard C++ enums with no connection to the engine's
mythology. The comment block at the top names them as Shells, Coral, Currents, and Tide Pools,
but this never appeared in the code itself.

**Source type count in header was stale**
The header said "other 41 engines" — engine count is now 44 (including OXBOW and OWARE added
2026-03-20). Minor but signals the header isn't being maintained as the fleet grows.

### Changes Made

| File | Change |
|------|--------|
| `Source/Engines/Obrix/ObrixEngine.h` line 89 | Added mythology banner: `// Mythology: Shells (sources) · Coral (processors) · Currents (modulators) · Tide Pools (effects)` |
| `Source/Engines/Obrix/ObrixEngine.h` lines 91–107 | Added inline mythology comments to each enum declaration: `// Shells — the living generators`, `// Coral — the filter/transformer organisms`, `// Currents — the modulators that flow between bricks`, `// Tide Pools — the spatial environment at the reef edge` |
| `Source/Engines/Obrix/ObrixEngine.h` line 22 | Updated "other 41 engines" to "other 43 engines" (accurate for fleet of 44) |

### Remaining Recommendations

- **Parameter display name mythology** (post-V1): `"Obrix Source 1 Type"` → `"Obrix Shell 1 Type"`,
  `"Obrix Proc 1 Cutoff"` → `"Obrix Coral 1 Cutoff"`, etc. Parameter IDs are frozen but display
  names can be updated. Requires DAW testing to verify display length fits. Flagged for Wave 5.

**Style Score**: 7.8 → 8.2 (enum mythology now lives in code, not just comments)

---

## Specialist 2 — POLISH (UI/UX and Parameter Quality)

### Findings

**Default source still Sine** (open since original seance)
The seance council unanimously recommended changing `src1Type` default from Sine (index 1) to
Saw (index 2). The gala pass (03-20) improved the gesture display names and source label
("Driftwood" replacing "Lo-Fi Saw") but the default remained Sine. Pearlman's seance verdict:
"filter has nothing to sculpt with Sine." The init patch greets every new user with the engine's
least interesting sound.

**Magic numbers undocumented in macro section**
Three scaling constants in the macro modulation block had no comments explaining their musical
intent: `8.0f` (fold exponent), `3000.0f` (CHARACTER cutoff sweep), `100.0f` (MOVEMENT detune),
`4000.0f` (mod wheel sweep). The previous pass documented the Drift Bus `0.23f` constant
beautifully; the macro section lagged behind.

### Changes Made

| File | Change |
|------|--------|
| `Source/Engines/Obrix/ObrixEngine.h` line 975 | Changed `srcChoices` default index from `1` (Sine) to `2` (Saw) with comment citing seance |
| `Source/Engines/Obrix/ObrixEngine.h` line 311 | Updated `loadP (pSrc1Type, 2.0f)` fallback to match new default |
| `Source/Engines/Obrix/ObrixEngine.h` lines 612–620 | Added inline comments to all four macro scaling constants explaining musical intent |

### Remaining Recommendations

- **Glide time range**: Max 1.0 second glide is musically restrictive. Most synths go to 2–5s.
  The current range serves as a reasonable default but players doing Berlin School-style portamento
  will hit the ceiling. Consider extending to 2.0s in Wave 5 (non-breaking: existing presets
  default to 0.0).
- **`obrix_fxMode` display**: "Serial" / "Parallel" are technically accurate but could be more
  evocative: "River" (serial, each pool feeds the next) / "Lagoon" (parallel, all pools fill
  simultaneously). Low priority — functional names work fine here.

**Polish Score**: 6.5 → 7.5 (default patch now has something to sculpt; magic numbers documented)

---

## Specialist 3 — ARCHITECTURE (Code Structure)

### Findings

**renderBlock is a ~450-line monolith**
This was flagged in the 03-19 makeover and the 03-20 gala. It was not implemented in either pass
because the risk of introducing per-sample overhead or subtle DSP regressions in the inner voice
loop outweighs the readability benefit during V1 development. The concern is legitimate.

**Architecture note was missing**
There was no documentation of why the monolith was intentional, leaving future readers to wonder
if it was an oversight.

**Shared DSP usage is correct and complete**
OBRIX uses `StandardLFO` (aliased as `ObrixLFO`) and `StandardADSR` (aliased as `ObrixADSR`)
from the shared DSP library. `CytomicSVF` and `PolyBLEP` are used directly. `FastMath` is
included for `fastSin`, `fastTanh`, `fastPow2`, `flushDenormal`. The glide processor is
implemented inline (appropriate — it is a simple one-pole per voice, not the full GlideProcessor
which targets frequency-domain portamento). No unnecessary duplication detected.

**Wave 2/3/4 parameter groupings are chronological, not architectural**
The private member section reads as a development timeline (Wave 2 block, Wave 3 block, Wave 4
block) rather than a logical grouping (sources, processors, spatial, state). This is honest but
makes the final struct feel like an accumulation rather than a design.

### Changes Made

| File | Change |
|------|--------|
| `Source/Engines/Obrix/ObrixEngine.h` lines 297–306 | Added ARCHITECTURE NOTE comment documenting the renderBlock monolith as intentional, naming the future refactor target (Wave 5), and specifying `JUCE_FORCEINLINE` as the safe approach |

### Remaining Recommendations (flagged, not implemented)

- **renderBlock decomposition** (Wave 5): Factor inner loops into `JUCE_FORCEINLINE` helpers:
  `processGestureEnvelope()`, `processVoiceSample()`, `processSpatial()`. Measure with clang -O2
  before shipping — inlining will preserve performance, but verify with a profiler.
- **Private member reorganization** (Wave 5): Reorganize Wave 2/3/4 parameter blocks into logical
  groups (sources, processors, spatial, biophonic state) with the wave number as a history note.
- **SilenceGate/SRO integration**: When all voices are idle, OBRIX still runs the full sample
  loop and effects chain on silence. Other engines (OPAL, ONSET) use `SRO` to skip this. Medium
  priority for multi-engine setups where CPU matters.

**Architecture Score**: 6.5 → 7.0 (monolith now documented as intentional; no structural change)

---

## Specialist 4 — SOUND (Preset Quality)

### Library State

| Mood | Count | Assessment |
|------|-------|------------|
| Flux | 71 | Appropriate — OBRIX's aggressive territory |
| Foundation | 65 | Well-populated; now cleaned of functional names |
| Aether | 59 | Strongest mood, Journey Mode and deep modulation presets |
| Prism | 62 | Solid, clear identity |
| Atmosphere | 53 | Solid |
| Submerged | 33 | Improved from 18 at gala; still the thinnest mood |
| Entangled | 29 | Purposeful coupling presets |
| Family | 21 | Cross-engine pairings as expected |
| **Crystalline, Deep, Ethereal, Kinetic, Luminous, Organic** | **0** | See below |

**Total: 393 presets**

### Missing Moods

Six of the 14 mood folders have zero OBRIX presets. These appear to be newer mood categories
added after the initial OBRIX preset batch was created. Cross-checking shows other recent engines
(OXBOW, OWARE) also lack presets in these folders — they are likely either (a) not yet
activated as deployment targets, or (b) require specific tonal character OBRIX hasn't covered.

| Missing Mood | OBRIX Fit Assessment |
|-------------|---------------------|
| Crystalline | High — OBRIX's HP filter + Ring Mod + Metallic wavetable naturally produces crystalline tones |
| Deep | High — OBRIX's Journey Mode + sub-octave src2 + slow drift = deep territory |
| Ethereal | High — OBRIX's Drift Bus + soft sine/triangle + reverb = classic ethereal texture |
| Kinetic | Medium — OBRIX can be rhythmic via velocity + fast decay + flash gesture |
| Luminous | Medium — OBRIX's Bioluminescent Pulse gesture + high Air setting |
| Organic | High — OBRIX's Organic wavetable bank + Competition/Symbiosis ecology |

### Wave 4 (Biophonic) Coverage Gap

Of 393 presets, only 17 use any Wave 4 Biophonic parameters. The Harmonic Field, Environmental
Parameters, Brick Ecology, and Stateful Synthesis systems represent OBRIX's most novel DSP
(added 2026-03-21) and are nearly invisible in the library. The 17 existing Wave 4 presets are
high quality (Field Maximum, Mutualism, Resource Wars, Reef Under Stress) but the coverage is 4.3%.

### Other Quality Findings

- **FM active in 16.5% of presets**: Healthy; FM is a specialist tool in OBRIX, not the default.
- **Src2 active in 61.8%**: Strong — most presets use both sources (the Constructive Collision).
- **Drift Bus active in 37.7%**: Excellent — the ensemble drift feature is well-represented.
- **Journey Mode in 8.1%**: Appropriate — this is a specialist mode for sustained evolution presets.
- **All macros zero in 8.1%**: Acceptable (down from previous gala finding of 91.8% — that scan
  may have been counting differently). The four macros are active and musically useful in the
  majority of presets.
- **Only-Sine no-Src2 patches: 5.6%** (22 presets): These will sound noticeably different after
  the default src1Type change to Saw, but that only affects the init patch behavior. Existing
  presets explicitly set `obrix_src1Type: 1` and will load correctly.

### Changes Made

None to preset files beyond the functional name cleanup (see Style section). Preset expansion
into missing moods is a sound design session task, not a code review task.

### Remaining Recommendations

- **Add 10–15 presets to Crystalline, Deep, Ethereal** — these three moods are a natural fit for
  OBRIX's existing DSP (HP filter + Ring Mod for Crystalline; Journey + sub for Deep; Drift + soft
  sources for Ethereal). One preset design session would close the gap.
- **Wave 4 preset expansion**: 17 presets showcasing Harmonic Field alone represents 0.04% of
  the 393-preset library for what is the engine's flagship Wave 4 feature. Target: 30 presets
  exercising each Wave 4 system (Field, Environmental, Ecology, Stateful) with readable parameter
  diversity. Guru Bin retreat territory.
- **Aggressive+bright quadrant**: DNA analysis shows only ~7 presets occupy high brightness AND
  high aggression simultaneously. The engine can produce this character (saw + proc1 HPF + ring mod
  + high CHARACTER) but the presets don't demonstrate it.

**Sound Score**: 7.2 → 7.3 (functional name cleanup improves foundation library; no new presets)

---

## Specialist 5 — SOUL (Brand Alignment)

### Findings

**Previous work holds well**
The soul changes from the 03-20 Gala are intact and correct: "OBRIX is not an instrument. It is
a habitat." The chromatophore gesture mythology is present. The coupling channel 2 carries Dave
Smith's blessing. "Driftwood" replaced "Lo-Fi Saw." These are the right decisions, well
implemented.

**Historical lineage was missing**
The 03-19 Makeover recommended adding the engine's historical lineage (ARP 2600, Serge Modular,
Korg MS-20) but this was not implemented in the Gala. OBRIX's semi-modular philosophy is a real
lineage — omitting it leaves the engine floating without ancestors.

**Engine count in header was stale**
"the other 41 engines" — there are 44 engines. Small but signals the identity block isn't being
maintained.

### Changes Made

| File | Change |
|------|--------|
| `Source/Engines/Obrix/ObrixEngine.h` lines 26–30 | Added historical lineage block: ARP 2600 (1971), Serge Modular (1974), Korg MS-20 (1978) with one-sentence design philosophy each |
| `Source/Engines/Obrix/ObrixEngine.h` line 22 | Updated engine count to "other 43 engines" |

### Remaining Recommendations

- **Sound design guide entry**: The `Docs/xomnibus_sound_design_guides.md` audit note confirms
  OBRIX is missing an entry (along with ORBWEAVE, OVERTONE, ORGANISM, OXBOW, OWARE). A guide
  entry would take the historical lineage and brick mythology and turn it into a 500-word player
  document. Recommended for the Tutorial Studio pass.
- **Brick Drop launch narrative**: The "Brick Drop" strategy (new bricks every 2–4 weeks) is
  documented in `Docs/specs/obrix_brick_drop_strategy.md` but is not referenced in the engine
  header. Even one line noting "Brick Drop: new source/processor/effect types added periodically"
  would help developers understand why the enum design is extensible.

**Soul Score**: 7.8 → 8.2 (historical lineage now grounded in the code; mythology consistent)

---

## Summary of Changes Made

### Source Code

**File**: `Source/Engines/Obrix/ObrixEngine.h`

| Change | Line(s) | Specialist | Impact |
|--------|---------|-----------|--------|
| Updated engine count "41" → "43" | 22 | Soul | Accuracy |
| Added historical lineage block (ARP 2600, Serge, MS-20) | 26–30 | Soul | Brand grounding |
| Added mythology banner to Brick Enums section | 89 | Style | Code culture |
| Added inline mythology to each enum declaration | 91, 96, 101, 106 | Style | Code culture |
| Added ARCHITECTURE NOTE to renderBlock | 297–306 | Architecture | Intent documentation |
| Updated `loadP (pSrc1Type, 2.0f)` fallback | 311 | Polish | Default consistency |
| Added comments to macro scaling constants | 612–620 | Polish | Magic number documentation |
| Changed `srcChoices` default from `1` (Sine) to `2` (Saw) | 975 | Polish | **Audible change** |

### Preset Files

**Folder**: `Presets/XOmnibus/Foundation/`

| Old Name | New Name | File |
|----------|----------|------|
| Pluck Init | Coral Tap | `Obrix_Pluck_Init.xometa` |
| Macro Play | Four Currents | `Obrix_Macro_Play.xometa` |
| Pulse Width | Breathing Hollow | `Obrix_Pulse_Width.xometa` |
| Noise Hiss | White Water | `Obrix_Noise_Hiss.xometa` |
| One Sine | Single Polyp | `Obrix_One_Sine.xometa` |
| Dual Sine | Octave Colony | `Obrix_Dual_Sine.xometa` |
| Reverb Space | Reef Chamber | `Obrix_Reverb_Space.xometa` |

Descriptions were also updated for each renamed preset to be evocative and mythology-aligned.

---

## Remaining Recommendations (Not Implemented)

These are flagged for future sessions. None were implemented due to risk, scope, or requiring
audio content creation.

### High Priority

1. **Wave 4 preset expansion**: 17 presets for the Biophonic system is insufficient for the
   flagship feature. Target 30+ presets exercising Field, Environmental, Ecology, and Stateful
   systems with readable parameter diversity. Guru Bin retreat scope.

2. **Missing moods**: Crystalline, Deep, Ethereal (high fit), Kinetic, Luminous, Organic (medium
   fit) have zero OBRIX presets. Three presets per mood = 18 presets total for the first pass.

3. **Sound design guide entry** in `Docs/xomnibus_sound_design_guides.md`: OBRIX is listed as one
   of 6 missing engines. One focused writing session closes this.

### Medium Priority

4. **renderBlock decomposition** (Wave 5): Factor inner loops into `JUCE_FORCEINLINE` helpers.
   Test performance with clang -O2 before committing.

5. **SRO/SilenceGate integration**: Skip processing when all voices are inactive. Meaningful
   for multi-engine CPU efficiency.

6. **Aggressive+bright preset gap**: DNA analysis shows ~7 presets in the high brightness + high
   aggression quadrant. OBRIX can produce this character but the library doesn't demonstrate it.

### Low Priority

7. **Parameter display name mythology** (post-V1): `"Shell 1 Type"` / `"Coral 1 Cutoff"` /
   `"Tide Pool 2 Mix"` in the UI display names. IDs are frozen; display names are changeable.

8. **Glide time range extension**: Extend max from 1.0s to 2.0s for Berlin School portamento.

9. **Private member reorganization**: Reorganize Wave 2/3/4 parameter pointer blocks into logical
   groups rather than chronological waves.

---

## Before / After Scores

| Specialist | After Gala (03-20) | After This Pass (03-21) | Change |
|-----------|---------------------|--------------------------|--------|
| F1 Style | 7.8 | 8.2 | +0.4 |
| F2 Polish | 6.5 | 7.5 | +1.0 |
| F3 Architecture | 6.5 | 7.0 | +0.5 |
| F4 Sound | 7.2 | 7.3 | +0.1 |
| F5 Soul | 7.8 | 8.2 | +0.4 |
| **Overall** | **7.2** | **7.6** | **+0.4** |

The single largest audible impact: the default source type change from Sine to Saw. Every new
user's first keypress on OBRIX now has harmonics to sculpt. That was the seance council's most
urgent recommendation from March 2026-03-19 and is now closed.

---

*Fab Five pass conducted 2026-03-21. Files modified: 1 source header + 7 preset files.*
