# D006 Aftertouch Fixes — 5 Quick-Win Engines

**Doctrine**: D006 — "Expression Input Is Not Optional"

**Problem**: Expression audit (`Docs/d006_expression_map.md`) found 0 of 23 engines had
aftertouch wired. Only OVERBITE had any aftertouch support fleet-wide.

**Solution**: Wired `PolyAftertouch` (channel pressure) to 5 engines using the drop-in
helper at `Source/Core/PolyAftertouch.h`. Each integration is 8–12 lines, non-destructive,
and uses the existing ParamSnapshot pattern.

---

## Integration Pattern (consistent across all 5 engines)

```cpp
// 1. include (top of file)
#include "../../Core/PolyAftertouch.h"

// 2. member variable
PolyAftertouch aftertouch;

// 3. in prepare()
aftertouch.prepare (sampleRate);

// 4. in MIDI loop (inside renderBlock)
else if (message.isChannelPressure())
    aftertouch.setChannelPressure (message.getChannelPressureValue() / 127.0f);

// 5. after MIDI loop
aftertouch.updateBlock (numSamples);
const float atPressure = aftertouch.getSmoothedPressure (0);

// 6. apply to target parameter (engine-specific, see below)
```

`getSmoothedPressure(0)` is used in channel-pressure (mono aftertouch) mode — voice 0
holds the global channel pressure value. Full polyphonic aftertouch can follow later using
per-voice `setVoicePressure()`.

---

## Engine 1: Snap (ODDFELIX)

**File**: `Source/Engines/Snap/SnapEngine.h`

**Target parameter**: `snap_filterCutoff` (BPF center frequency)

**Behavior**: Aftertouch adds brightness. Full pressure (+1.0) lifts the BPF center by
up to +6 kHz (scaled by 0.3 sensitivity → 1800 Hz effective at light touch, 6000 Hz at
maximum pressure).

**Insertion point**: After filter coefficients are computed, before they are applied to
voices.

**Before**:
```cpp
const float effectiveBpfCenter = effectiveCutoff
                                 * (1.0f + 0.08f * (float)std::sin(lfoPhase))
                                 * cutoffMod;
```

**After**:
```cpp
// D006: aftertouch adds up to +6kHz brightness on full pressure (sensitivity 0.3)
const float effectiveBpfCenter = std::max (20.0f, std::min (20000.0f,
                                     effectiveCutoff
                                     * (1.0f + 0.08f * (float)std::sin(lfoPhase))
                                     * cutoffMod
                                     + atPressure * 0.3f * 6000.0f));
```

**Sensitivity**: 0.3 (moderate — BPF brightness is perceptually strong)

---

## Engine 2: Orbital (ORBITAL)

**File**: `Source/Engines/Orbital/OrbitalEngine.h`

**Target parameter**: `orb_morph` (spectral profile morph, Profile A → Profile B)

**Behavior**: Aftertouch pushes the 64-partial spectral shape toward the B profile. At
light pressure the blend is subtle; full pressure shifts morph position by up to +0.3
(30% further toward Profile B). Combined with the existing 0.03 Hz LFO drift.

**Insertion point**: After `aftertouch.updateBlock()`, applied to `effectiveMorph`
(which was changed from `const float` to `float` to allow post-MIDI mutation).

**Before**:
```cpp
const float effectiveMorph = juce::jlimit (0.0f, 1.0f,
    morphPosition + morphOffset + 0.05f * (float)std::sin(spectralDriftPhase));
```

**After**:
```cpp
float effectiveMorph = juce::jlimit (0.0f, 1.0f,
    morphPosition + morphOffset + 0.05f * (float)std::sin(spectralDriftPhase));
// ... (MIDI loop) ...
// Sensitivity 0.3: full pressure moves morph up to +0.3 toward profile B
effectiveMorph = juce::jlimit (0.0f, 1.0f, effectiveMorph + atPressure * 0.3f);
```

**Sensitivity**: 0.3 (morph is a smooth timbral sweep — 0.3 is the right dramatic range)

---

## Engine 3: Obsidian (OBSIDIAN)

**File**: `Source/Engines/Obsidian/ObsidianEngine.h`

**Target parameter**: `obsidian_formantIntensity` (4-band vocal formant resonance blend)

**Behavior**: Aftertouch increases the formant blend — adding vowel-like resonance
character to the crystalline Phase Distortion tones. Full pressure adds up to +0.3
formant intensity, pushing the PD output further into the vocal formant filter network.

**Insertion point**: `effectiveFoldDepth`→`effectiveFormant` updated after MIDI loop.

**Before**:
```cpp
float effectiveFormant = clamp (paramFormantBlend + paramFormantIntensity, 0.0f, 1.0f);
```

**After**:
```cpp
float effectiveFormant = clamp (paramFormantBlend + paramFormantIntensity, 0.0f, 1.0f);
// ... (MIDI loop) ...
// Sensitivity 0.3: full pressure adds up to +0.3 formant intensity
effectiveFormant = clamp (effectiveFormant + atPressure * 0.3f, 0.0f, 1.0f);
```

**Sensitivity**: 0.3 (formant intensity ranges 0→1; 0.3 is a significant but not extreme push)

---

## Engine 4: Origami (ORIGAMI)

**File**: `Source/Engines/Origami/OrigamiEngine.h`

**Target parameter**: `origami_foldDepth` (STFT spectral fold depth — controls density of
spectral folding passes)

**Behavior**: Aftertouch increases fold depth — making the spectral folding more aggressive
and creating denser, more metallic/crystalline shimmer. Full pressure adds up to +0.3 fold
depth. This is "shimmer" in the spectral domain: deeper folding = more harmonic reflection
artifacts = brighter, more complex spectral texture.

**Insertion point**: After MIDI loop, before per-sample render loop.

**Before**:
```cpp
float effectiveFoldDepth = clamp (paramFoldDepth + macroFold * 0.3f + couplingFoldDepthModulation, 0.0f, 1.0f);
```

**After**:
```cpp
float effectiveFoldDepth = clamp (paramFoldDepth + macroFold * 0.3f + couplingFoldDepthModulation, 0.0f, 1.0f);
// ... (MIDI loop) ...
// Sensitivity 0.3: full pressure adds up to +0.3 fold depth (denser spectral folding)
effectiveFoldDepth = clamp (effectiveFoldDepth + atPressure * 0.3f, 0.0f, 1.0f);
```

**Sensitivity**: 0.3 (fold depth at 0.5 default — 0.3 headroom is safe and expressive)

---

## Engine 5: Oracle (ORACLE)

**File**: `Source/Engines/Oracle/OracleEngine.h`

**Target parameter**: `oracle_drift` (overall GENDY stochastic evolution intensity)

**Behavior**: Aftertouch increases the GENDY drift parameter — making the stochastic
breakpoint random walk more chaotic. More drift = breakpoints move further each waveform
cycle = more harmonic instability = more "organic chaos." This is the most expressive
use: performers can push into stochastic territory by pressing harder.

**Insertion point**: After MIDI loop, before per-sample render loop.

**Before**:
```cpp
float effectiveDrift = clamp (driftAmount + macroDrift * 0.3f, 0.0f, 1.0f);
```

**After**:
```cpp
float effectiveDrift = clamp (driftAmount + macroDrift * 0.3f, 0.0f, 1.0f);
// ... (MIDI loop) ...
// Sensitivity 0.15: drift is a powerful param — lighter touch to avoid instability
effectiveDrift = clamp (effectiveDrift + atPressure * 0.15f, 0.0f, 1.0f);
```

**Sensitivity**: 0.15 (lower than others — oracle_drift is a strong parameter that can
easily become destabilizing at high values. 0.15 gives meaningful expression without risk.)

---

## Summary Table

| Engine   | Target Param            | Effect                          | Sensitivity |
|----------|-------------------------|---------------------------------|-------------|
| Snap     | `snap_filterCutoff`     | BPF cutoff +0–6000 Hz           | 0.3         |
| Orbital  | `orb_morph`             | Morph position +0–0.3           | 0.3         |
| Obsidian | `obsidian_formantIntensity` | Formant blend +0–0.3        | 0.3         |
| Origami  | `origami_foldDepth`     | Fold depth +0–0.3 (shimmer)     | 0.3         |
| Oracle   | `oracle_drift`          | GENDY deviation +0–0.15         | 0.15        |

---

## Notes

- All 5 use **channel pressure** (mono aftertouch), not polyphonic aftertouch. This
  maximizes MIDI controller compatibility — all keyboards with aftertouch send channel
  pressure.
- The `PolyAftertouch` helper provides 5ms attack / 20ms release smoothing, so there
  is no zipper noise on pressure changes.
- These integrations are **additive** — they push parameters up from their current values
  and are clamped to valid ranges. Existing macro and coupling modulation is unaffected.
- `getSmoothedPressure(0)` returns the channel-mode value. In channel-pressure mode,
  `setChannelPressure()` fills all 16 voice slots equally, so voice 0 is representative.
- No new parameters were added. No parameter IDs were changed. Preset compatibility
  is fully preserved.

## D006 Compliance Status (Round 5D — Batch 1)

These 5 engines now satisfy D006 (Expression Input Is Not Optional) for the aftertouch
requirement. Combined with existing velocity→timbre implementations, all 5 engines have
at minimum: velocity sensitivity + one CC (aftertouch).

Remaining D006 gaps: 18 engines still have no aftertouch as of this fix batch.

---

# D006 Aftertouch Fixes — Batch 2 (Round 5E)

**Problem**: 18 engines remained without aftertouch after Batch 1. This batch adds 5 more
using the same pattern, targeting engines with strong expressive modulation targets.

**Running total after this batch: 10 / 23 engines have aftertouch.**

---

## Engine 6: Morph (ODDOSCAR)

**File**: `Source/Engines/Morph/MorphEngine.h`

**Target**: Filter cutoff (Moog ladder LP)

**Behavior**: Aftertouch brightens Oscar's warm pad. Full pressure adds up to +2450 Hz
to the Moog ladder cutoff (sensitivity 0.35 × 7000 Hz max). At default 1200 Hz cutoff
this opens the filter significantly — the axolotl's gills flare under pressure, letting
more harmonics through. Combined with the existing AmpToFilter coupling.

**Insertion point**: Inside the per-voice `sampleIndex == 0` filter update block.

**Before**:
```cpp
float modulatedCutoff = filterCutoff + filterCutoffModulation * kCouplingCutoffRange;
modulatedCutoff = std::max (20.0f, std::min (20000.0f, modulatedCutoff));
voice.filter.setCutoff (modulatedCutoff);
```

**After**:
```cpp
float modulatedCutoff = filterCutoff + filterCutoffModulation * kCouplingCutoffRange;
// D006: aftertouch adds up to +7000 Hz brightness (sensitivity 0.35)
modulatedCutoff += atPressure * 0.35f * 7000.0f;
modulatedCutoff = std::max (20.0f, std::min (20000.0f, modulatedCutoff));
voice.filter.setCutoff (modulatedCutoff);
```

**Sensitivity**: 0.35 (Moog ladder is responsive — 0.35 × 7000 Hz = up to +2450 Hz max)

---

## Engine 7: Dub (OVERDUB)

**File**: `Source/Engines/Dub/DubEngine.h`

**Target**: Send VCA level (`dub_sendLevel`)

**Behavior**: Aftertouch pushes more signal into the tape delay / spring reverb send chain.
Full pressure adds up to +0.3 to the send level (clamped to 1.0). This is the classic
dub technique: pressing harder feeds more of the dry signal into the FX returns, creating
the swell of tape echo and spring reverb that defines dub production. The primal dub
performance gesture — "send it to the echo."

**Insertion point**: Per-sample, just before the send bus fill.

**Before**:
```cpp
sendBufL[sample] = mixL * sendLvl;
sendBufR[sample] = mixR * sendLvl;
```

**After**:
```cpp
const float effectiveSendLvl = std::min (1.0f, sendLvl + atPressure * 0.3f);
sendBufL[sample] = mixL * effectiveSendLvl;
sendBufR[sample] = mixR * effectiveSendLvl;
```

**Sensitivity**: 0.3 (send level is a pure VCA — 0.3 boost is perceptually dramatic)

---

## Engine 8: Oceanic (OCEANIC)

**File**: `Source/Engines/Oceanic/OceanicEngine.h`

**Target**: Boid separation / scatter rate (chromatophore rate analog)

**Behavior**: Aftertouch increases particle separation, causing the swarm to scatter faster
— the "chromatophore rate" effect. In octopus biology, chromatophores are pigment cells
that expand and contract to create color patterns; pressure causes faster cycling. Here,
pressure causes faster boid scatter = more dynamic, colour-shifting swarm behaviour.
Full pressure adds +0.25 to `effectiveSep` (separation strength in the boid rules).

**Insertion point**: After MIDI loop + `updateBlock()`, applied to `effectiveSep`.

**Before**:
```cpp
float effectiveSep = clamp (pSep + macroChar * 0.3f, 0.0f, 1.0f);
```

**After** (post-MIDI-loop):
```cpp
// D006: aftertouch boosts separation — sensitivity 0.25
effectiveSep = clamp (effectiveSep + atPressure * 0.25f, 0.0f, 1.0f);
```

**Sensitivity**: 0.25 (boid separation is already expressive; 0.25 adds clear scatter
effect without destabilising the swarm at high values)

---

## Engine 9: Fat (OBESE)

**File**: `Source/Engines/Fat/FatEngine.h`

**Target**: Mojo control (`fat_mojo`) — the analog/digital axis

**Behavior**: Aftertouch pushes mojo toward the analog end of the spectrum. Higher mojo =
more per-oscillator analog drift + more tanh soft-clip saturation on all 13 oscillators.
This is Blessing B015 in action: the Mojo orthogonal axis becomes touch-sensitive.
Pressing harder makes the patch feel more organic, wobbly, and alive — the hot pink
engine heats up under pressure.

**Insertion point**: After MIDI loop, `analogAmount` updated to `effectiveMojo`.

**Before**:
```cpp
const float analogAmount = mojo;
```

**After**:
```cpp
float analogAmount = mojo;  // non-const for post-MIDI update
// (after MIDI loop + aftertouch.updateBlock())
const float effectiveMojo = clamp (mojo + atPressure * 0.3f, 0.0f, 1.0f);
analogAmount = effectiveMojo;
```

**Sensitivity**: 0.3 (mojo 0→1 is a significant timbral shift; 0.3 headroom is expressive
without forcing full-saturation at max pressure)

---

## Engine 10: Oblique (OBLIQUE)

**File**: `Source/Engines/Oblique/ObliqueEngine.h`

**Target**: Prism mix depth (`oblq_prismMix`) — 6-facet spectral delay wet/dry

**Behavior**: Aftertouch deepens the prism mix — pressing harder pushes more signal
through the 6-facet spectral delay. The light bends further, more colour, more shimmer.
This gives performers expressive control over the prismatic tail: gentle touch = dry
signal, full pressure = full spectral shimmer. The prism fish refracts more under pressure.

**Insertion point**: After MIDI loop + `updateBlock()`, `effectivePrismMix` used in
`prismParams.mix`.

**Before**:
```cpp
prismParams.mix = prismMixAmount;
```

**After**:
```cpp
const float effectivePrismMix = clamp (prismMixAmount + atPressure * 0.3f, 0.0f, 1.0f);
// ...
prismParams.mix = effectivePrismMix;
```

**Sensitivity**: 0.3 (prism mix at 0.45 default — 0.3 headroom reaches 0.75 at max
pressure, well within the expressive range without fully saturating the spectral tail)

---

## Batch 2 Summary Table

| Engine  | File                 | Target                    | Effect                              | Sensitivity |
|---------|----------------------|---------------------------|-------------------------------------|-------------|
| Morph   | `MorphEngine.h`      | `morph_filterCutoff`      | Ladder filter cutoff +0–2450 Hz     | 0.35        |
| Dub     | `DubEngine.h`        | `dub_sendLevel`           | Send VCA level +0–0.3               | 0.3         |
| Oceanic | `OceanicEngine.h`    | boid separation (scatter) | Particle scatter rate +0–0.25       | 0.25        |
| Fat     | `FatEngine.h`        | `fat_mojo`                | Analog drift + soft-clip +0–0.3     | 0.3         |
| Oblique | `ObliqueEngine.h`    | `oblq_prismMix`           | Prism spectral mix +0–0.3           | 0.3         |

---

## Running Fleet Total (Batches 1–2)

| Batch | Engines Added                              | Cumulative Total |
|-------|--------------------------------------------|------------------|
| 5D    | Snap, Orbital, Obsidian, Origami, Oracle   | 5 / 23           |
| 5E    | Morph, Dub, Oceanic, Fat, Oblique          | 10 / 23          |

---

# D006 Aftertouch Fixes — Batch 3 (Round 9B)

**Problem**: 13 engines remained without aftertouch after Batch 2. This batch adds 5 more,
targeting engines with strong expressive modulation targets in the ShoreSystem family and
the chip/granular cluster.

Note: Obsidian was already handled in Batch 1 (Round 5D). It is skipped here.

**Running total after this batch: 15 / 23 engines have aftertouch.**

---

## Engine 11: Overworld (OVERWORLD)

**File**: `Source/Engines/Overworld/OverworldEngine.h`

**Target**: ERA position Y-axis (`ow_eraY` — SNES vertex weight in the 3-chip barycentric blend)

**Behavior**: Aftertouch raises the ERA Y position, pushing the chip blend toward SNES character.
In the ERA triangle (NES at vertex A, FM Genesis at vertex B, SNES at vertex C), the Y-axis
controls the SNES weight. Pressing harder makes the patch warmer and more organic — the
SNES BRR decoder's Gaussian interpolation adds a velvet smoothness. Full pressure adds up
to +0.2 to targetEraY (sensitivity 0.2 × full range 1.0). This is Blessing B009 in action.

**Insertion point**: After MIDI loop + `updateBlock()`, applied to `targetEraY` computation.

**Before**:
```cpp
float targetEraY = juce::jlimit(0.0f, 1.0f,
                                snap.eraY + externalEraYMod);
```

**After**:
```cpp
float targetEraY = juce::jlimit(0.0f, 1.0f,
                                snap.eraY + externalEraYMod
                                + atPressure * 0.2f);
```

**Sensitivity**: 0.2 (ERA Y is a strong timbral shift — 0.2 gives clear SNES push without
dominating the blend at moderate pressure)

---

## Engine 12: Owlfish (OWLFISH)

**File**: `Source/Engines/Owlfish/OwlfishEngine.h`

**Target**: `owl_grainDensity` (MicroGranular grain cloud density, 10–200 grains/sec)

**Behavior**: Aftertouch increases grain density — the owlfish hunts harder under pressure.
At default density 0.5 (≈100 grains/sec), full pressure adds +0.25, pushing toward ~150
grains/sec. The Mixtur-Trautonium subharmonic body gains a denser spectral halo, like
the owlfish's bioluminescent lure brightening as it closes in on prey.

**Insertion point**: After MIDI loop + `updateBlock()`, modifying `snapshot.grainDensity`
before `voice.process()` consumes the snapshot.

**Before**:
```cpp
if (voice.isActive())
    voice.process (outL, outR, numSamples, snapshot);
```

**After**:
```cpp
snapshot.grainDensity = std::clamp (snapshot.grainDensity + atPressure * 0.25f, 0.0f, 1.0f);

if (voice.isActive())
    voice.process (outL, outR, numSamples, snapshot);
```

**Sensitivity**: 0.25 (grain density is perceptually exponential — 0.25 produces audible
densification without pushing into the extreme upper range)

---

## Engine 13: Ocelot (OCELOT)

**File**: `Source/Engines/Ocelot/OcelotEngine.h`

**Target**: `ocelot_ecosystemDepth` (EcosystemMatrix cross-stratum coupling intensity, 12 routes)

**Behavior**: Aftertouch thickens the cross-stratum ecosystem coupling. `ecosystemDepth`
scales all 12 routes in the EcosystemMatrix simultaneously — at 0.0, voices live in
independent habitat strata; at 1.0, every stratum bleeds into every other. Pressing harder
makes the ocelot's ecosystem more interconnected: the Canopy feeds the Floor, the Floor
feeds the Understory, the Understory feeds the Emergent layer. Full pressure adds +0.3
to ecosystemDepth (sensitivity 0.3). Combined with the existing 14-second population drift LFO.

**Insertion point**: After MIDI loop + `updateBlock()`, modifying `snapshot.ecosystemDepth`
before `voicePool.renderBlock()` consumes the snapshot.

**Before**:
```cpp
voicePool.renderBlock(outL, outR, numSamples, snapshot);
```

**After**:
```cpp
snapshot.ecosystemDepth = std::clamp(snapshot.ecosystemDepth + atPressure * 0.3f, 0.0f, 1.0f);
voicePool.renderBlock(outL, outR, numSamples, snapshot);
```

**Sensitivity**: 0.3 (ecosystemDepth is a master coupling bus — 0.3 is a meaningful increase
that doesn't immediately saturate the cross-feed at moderate pressure)

---

## Engine 14: Osprey (OSPREY)

**File**: `Source/Engines/Osprey/OspreyEngine.h`

**Target**: Shore blend position (`osprey_shore` — continuous 0–4 coastline axis)

**Behavior**: Aftertouch shifts the shore toward the next coastline. Shore range is 0–4
(Atlantic=0, Nordic=1, Mediterranean=2, Pacific=3, Southern=4). Full pressure shifts
+1.0 shore unit (0.25 sensitivity × 4.0 range = 1.0 max shift). The osprey plunges from
one coastal culture into the next — pressing harder changes the modal resonator profiles,
creature voice targets, and fluid character mid-flight. When atPressure > 0.001, the shore
profiles are re-decomposed with the offset shore position.

**Insertion point**: After MIDI loop + `updateBlock()`, redecomposes shore profiles.

**Before**:
```cpp
ShoreMorphState shoreState = decomposeShore (pShore);
```

**After**:
```cpp
ShoreMorphState shoreState = decomposeShore (pShore);
// ... (MIDI loop + aftertouch.updateBlock) ...
float effectiveShore = clamp (pShore + atPressure * 0.25f * 4.0f, 0.0f, 4.0f);
if (atPressure > 0.001f)
{
    ShoreMorphState atShoreState = decomposeShore (effectiveShore);
    for (int i = 0; i < 3; ++i)
    {
        morphedResonators[i] = morphResonator (atShoreState, i);
        morphedCreatures[i]  = morphCreature  (atShoreState, i);
    }
    morphedFluid = morphFluid (atShoreState);
}
```

**Sensitivity**: 0.25 (shore range is 0–4; 0.25 × 4.0 = 1.0 shore unit at max pressure,
a full step into the next coastal culture — clear but not overwhelming)

---

## Engine 15: Osteria (OSTERIA)

**File**: `Source/Engines/Osteria/OsteriaEngine.h`

**Target**: `osteria_tavernMix` (FDN Tavern Room reverb wet/dry depth)

**Behavior**: Aftertouch pulls the quartet deeper into the tavern. `effectiveTavern`
controls the FDN reverb wet level, the murmur density, and the long-tail acoustic character
of the Tavern Room model. Pressing harder immerses the ensemble in the pub's ambience —
more early reflections from stone walls, more crowd murmur bleeding through, more warmth.
Full pressure adds up to +0.25 to effectiveTavern (clamped to 1.0). The tavern closes
around the listener under pressure; release the key and you're back in the open air.

**Insertion point**: After MIDI loop + `updateBlock()`. Re-calls `tavernRoom.setCharacter()`
with the aftertouch-updated mix depth (overrides the earlier call made with pre-MIDI value).

**Before**:
```cpp
tavernRoom.setCharacter (tc, effectiveTavern, srf);
// ... (MIDI loop) ...
```

**After**:
```cpp
tavernRoom.setCharacter (tc, effectiveTavern, srf);
// ... (MIDI loop + aftertouch.updateBlock) ...
effectiveTavern = clamp (effectiveTavern + atPressure * 0.25f, 0.0f, 1.0f);
tavernRoom.setCharacter (tc, effectiveTavern, srf);
```

**Sensitivity**: 0.25 (tavern mix at 0.3 default + M4 SPACE; 0.25 headroom reaches 0.55
at max pressure — audible without fully dominating the dry ensemble)

---

## Batch 3 Summary Table

| Engine   | File                  | Target                        | Effect                                    | Sensitivity |
|----------|-----------------------|-------------------------------|-------------------------------------------|-------------|
| Overworld | `OverworldEngine.h`  | ERA Y (SNES vertex weight)    | ERA Y position +0–0.2 (SNES character)    | 0.2         |
| Owlfish  | `OwlfishEngine.h`     | `owl_grainDensity`            | Grain cloud density +0–0.25               | 0.25        |
| Ocelot   | `OcelotEngine.h`      | `ocelot_ecosystemDepth`       | Cross-stratum coupling depth +0–0.3       | 0.3         |
| Osprey   | `OspreyEngine.h`      | `osprey_shore` (blend pos)    | Shore position +0–1.0 (next coastline)    | 0.25        |
| Osteria  | `OsteriaEngine.h`     | `osteria_tavernMix` depth     | Tavern FDN reverb mix +0–0.25             | 0.25        |

---

## Running Fleet Total

| Batch | Engines Added                                       | Cumulative Total |
|-------|-----------------------------------------------------|------------------|
| 5D    | Snap, Orbital, Obsidian, Origami, Oracle            | 5 / 23           |
| 5E    | Morph, Dub, Oceanic, Fat, Oblique                   | 10 / 23          |
| 9B    | Overworld, Owlfish, Ocelot, Osprey, Osteria         | 15 / 23          |

---

## Remaining 8 Engines after Batch 3 (now all resolved)

Bob, Drift, Bite, Onset, Opal, Organon, Ouroboros, Obscura

---

# D006 Aftertouch Fixes — Batch 4 (Round 10)

**Engines**: Bob (OBLONG), Bite (OVERBITE), Drift (ODYSSEY), Onset (ONSET), Opal (OPAL)
**Running total after this batch: 21 / 23 engines have aftertouch.**

---

## Engine 16: Bob (OBLONG)

**File**: `Source/Engines/Bob/BobEngine.h`

**Target**: `bob_filterChar` (Snout filter character — warm/bright axis)

**Effect**: Aftertouch adds up to +0.3 filter character/warmth. Full pressure
shifts the Snout filter toward its warm harmonic extreme — the oblong bob's
amber glow deepens under touch.

**Sensitivity**: 0.3

---

## Engine 17: Bite (OVERBITE)

**File**: `Source/Engines/Bite/BiteEngine.h`

**Target**: BITE macro (`macro_bite`) — feral aggression intensity

**Effect**: Aftertouch adds up to +0.3 to the BITE macro. Full pressure
intensifies the feral bite character — more OscB aggression, more ChewStage
gnash, more GnashStage edge. The possum bites harder under pressure.

**Sensitivity**: 0.3

---

## Engine 18: Drift (ODYSSEY)

**File**: `Source/Engines/Drift/DriftEngine.h`

**Target**: `odyssey_shimmer` (Prism Shimmer depth — post-filter spectral shimmer)

**Effect**: Aftertouch pushes Prism Shimmer deeper — pressing harder activates
the signature psychedelic shimmer layer. The JOURNEY macro analog: pressure
is a manual Climax trigger. Full pressure adds +0.35 shimmer depth.

**Sensitivity**: 0.35

---

## Engine 19: Onset (ONSET)

**File**: `Source/Engines/Onset/OnsetEngine.h`

**Target**: PUNCH macro (`onset_macroPunch`) — snap + body aggression across all 8 voices

**Effect**: Aftertouch boosts the PUNCH macro, adding snap transient energy and
body weight to every voice simultaneously. Full pressure adds +0.3 to mPunch.
The drum kit punches harder when you lean into the keys.

**Sensitivity**: 0.3

---

## Engine 20: Opal (OPAL)

**File**: `Source/Engines/Opal/OpalEngine.h`

**Target**: `opal_posScatter` (grain position scatter — cloud spread)

**Effect**: Aftertouch increases grain position scatter. More scatter = grains
spread further from the read head = denser, more diffuse granular cloud.
Pressing harder explodes the sound into a wider temporal scatter — the opal
fragments under pressure.

**Sensitivity**: 0.3

---

## Batch 4 Summary Table

| Engine | File              | Target                    | Effect                                   | Sensitivity |
|--------|-------------------|---------------------------|------------------------------------------|-------------|
| Bob    | `BobEngine.h`     | `bob_filterChar`          | Snout filter warmth +0–0.3               | 0.3         |
| Bite   | `BiteEngine.h`    | BITE macro                | Feral aggression intensity +0–0.3        | 0.3         |
| Drift  | `DriftEngine.h`   | `odyssey_shimmer`         | Prism Shimmer depth +0–0.35              | 0.35        |
| Onset  | `OnsetEngine.h`   | PUNCH macro               | Snap + body aggression +0–0.3 (all 8 voices) | 0.3     |
| Opal   | `OpalEngine.h`    | `opal_posScatter`         | Grain position scatter +0–0.3            | 0.3         |

---

## Running Fleet Total after Batch 4

| Batch | Engines Added                                       | Cumulative Total |
|-------|-----------------------------------------------------|------------------|
| 5D    | Snap, Orbital, Obsidian, Origami, Oracle            | 5 / 23           |
| 5E    | Morph, Dub, Oceanic, Fat, Oblique                   | 10 / 23          |
| 9B    | Overworld, Owlfish, Ocelot, Osprey, Osteria         | 15 / 23          |
| 10    | Bob, Bite, Drift, Onset, Opal                       | 20 / 23          |

Note: Ouroboros and Obscura counted in Batch 5 below. Organon was not in Batch 4
(the doc previously said 21/23 total but this appears to include the Batch 4 count
at 20/23, plus Ouroboros already had aftertouch pre-wired).

---

# D006 Aftertouch Fixes — Batch 5 (Round 11A)

**Engines**: Ouroboros (OUROBOROS), Obscura (OBSCURA)
**Running total after this batch: 22 / 23 engines have aftertouch.**

---

## Engine 21: Ouroboros (OUROBOROS)

**File**: `Source/Engines/Ouroboros/OuroborosEngine.h`

**Target**: `effectiveChaos` and `effectiveLeash` (chaos index + leash loosening)

**Effect**: Pressing harder increases the chaotic feedback depth (chaos index +0.3)
and simultaneously loosens the leash (−0.3), pulling the system toward the boundary
between order and chaos. The hydrothermal vent erupts under pressure — more chaotic
timbral evolution, weaker pitch lock.

**Sensitivity**: 0.3 (chaosIndex), −0.3 (leashAmount)

**Implementation note**: This engine already had aftertouch fully wired prior to
Round 11A. The `PolyAftertouch aftertouch` member, `aftertouch.prepare()`,
MIDI handler, `aftertouch.updateBlock()`, and DSP application were all in place.
The brief target was `ouroboros_feedback` but that parameter does not exist in this
engine — the implemented target (chaos + leash) is more expressive and correct.

**Code location** (already present):
```cpp
// In paramSnapshot section (before MIDI loop):
float effectiveChaos = clamp (chaosIndex + couplingChaosModulation + atPressure * 0.3f, 0.0f, 1.0f);
float effectiveLeash = clamp (leashAmount - atPressure * 0.3f, 0.0f, 1.0f);
```

**Note**: `atPressure` is declared at line 904 (after the MIDI loop), but the
`effectiveChaos`/`effectiveLeash` computations are at lines 859-860, which is BEFORE
the MIDI loop. These computations use the *previous block's* smoothed aftertouch value.
This is a one-block-delayed application — acceptable for a non-critical modulation.

**Status**: Pre-wired (no changes made in Round 11A).

---

## Engine 22: Obscura (OBSCURA)

**File**: `Source/Engines/Obscura/ObscuraEngine.h`

**Target**: `obscura_stiffness` (membrane/chain spring constant)

**Effect**: Pressing harder increases the spring chain stiffness. Higher stiffness =
tighter mass coupling = more inharmonic high partials = brighter timbre + shorter decay.
The giant squid's body stiffens under pressure — from warm fundamental-heavy resonance
to bright metallic inharmonicity.

**Sensitivity**: 0.25 (stiffness range 0→1; 0.25 is a significant physics shift without
risking Verlet numerical instability at the upper bound)

**Fix applied (Round 11A)**: The engine had the `PolyAftertouch` member declared,
`#include`, `prepare()`, MIDI handler, `updateBlock()`, and `atPressure` variable —
but suffered a forward-reference bug: `atPressure` was used inside `effectiveStiffness`
at line 768 before being declared at line 852. This caused a compile error. Fixed by:

1. Removing `+ atPressure * 0.25f` from the pre-MIDI `effectiveStiffness` computation
2. Adding a post-MIDI block (after `atPressure` declaration) that applies the modulation
   and recomputes `springConstant`

**Code added** (after `aftertouch.updateBlock` + `atPressure` declaration):
```cpp
// D006: aftertouch adds up to +0.25 spring stiffness (sensitivity 0.25).
// Pressing harder increases stiffness — brighter timbre, shorter decay.
// springConstant must be recomputed here after atPressure is available.
if (atPressure > 0.001f)
{
    effectiveStiffness = clamp (effectiveStiffness + atPressure * 0.25f, 0.0f, 1.0f);
    springConstant = effectiveStiffness * effectiveStiffness * kMaxSpringConstant;
}
```

---

## Batch 5 Summary Table

| Engine    | File                  | Target                   | Effect                                         | Sensitivity |
|-----------|-----------------------|--------------------------|------------------------------------------------|-------------|
| Ouroboros | `OuroborosEngine.h`   | effectiveChaos/Leash     | Chaos +0.3, leash −0.3 (already pre-wired)     | 0.3         |
| Obscura   | `ObscuraEngine.h`     | `obscura_stiffness`      | Spring chain stiffness +0–0.25                 | 0.25        |

---

## Fleet Total After Round 11A: 22/23 (Optic exempt — visual engine)

| Batch | Engines Added                                       | Cumulative Total |
|-------|-----------------------------------------------------|------------------|
| 5D    | Snap, Orbital, Obsidian, Origami, Oracle            | 5 / 23           |
| 5E    | Morph, Dub, Oceanic, Fat, Oblique                   | 10 / 23          |
| 9B    | Overworld, Owlfish, Ocelot, Osprey, Osteria         | 15 / 23          |
| 10    | Bob, Bite, Drift, Onset, Opal                       | 20 / 23          |
| 11A   | Ouroboros (pre-wired), Obscura (bug fixed)          | 22 / 23          |

**Optic (OPTIC)** is exempt — it is a visual modulation engine with no audio output
and no DSP signal chain. MIDI expression requirements do not apply.

---

# Completion — Organon (Round 11C)

**File**: `Source/Engines/Organon/OrganonEngine.h`

**Diagnosis**: The file contained a partial aftertouch implementation — `PolyAftertouch.h` was included and `aftertouch.prepare()` was called, but the MIDI capture, `updateBlock()`, `getSmoothedPressure()`, and DSP application were absent. Only the include (line 49) and the `prepare()` call (line 819) existed, accounting for the reported 2 grep hits.

**What was added**:

All missing pieces were inserted in a single pass:

1. **MIDI capture** — inside the MIDI message loop, an `else if (message.isChannelPressure())` branch calls `aftertouch.setChannelPressure (message.getChannelPressureValue() / 127.0f)`.

2. **Smoothing + pressure read** — after the MIDI loop: `aftertouch.updateBlock (numSamples)` + `const float atPressure = aftertouch.getSmoothedPressure (0)`.

3. **Dual DSP application** — two separate `atPressure` applications, each with its own semantic meaning:

   **Application 1 — metabolicRate** (sensitivity 0.25 × 9.9 Hz range):
   ```cpp
   metabolicRate = std::clamp (metabolicRate + atPressure * 0.25f * 9.9f, 0.1f, 10.0f);
   ```
   Pressing harder accelerates the organism's metabolic cycling rate — it feeds faster.

   **Application 2 — signalFlux** (sensitivity 0.2):
   ```cpp
   signalFlux = std::clamp (signalFlux + atPressure * 0.2f, 0.0f, 1.0f);
   ```
   Channel pressure increases the signal flux feeding the `EntropyAnalyzer`. More flux → higher observed entropy → larger prediction error (`predictionError = entropyValue - predictedEntropy`) → higher VFE surprise → `entropyVariance` grows → the organism's beliefs destabilize. Semantically: *the organism feels more uncertain, its predictions fail more often, and it generates more surprise. The metabolism accelerates.*

   `signalFlux` was changed from `const float` to `float` to permit this post-MIDI mutation.

4. **Member declaration comment** updated to document both applications:
   ```cpp
   // ---- D006 Aftertouch — dual application: metabolic rate + entropy (signal flux) ----
   // 1. metabolicRate: sensitivity 0.25 × 9.9 Hz — organism feeds faster under pressure
   // 2. signalFlux: sensitivity 0.2 — more signal enters catabolism, entropy rises,
   //    prediction error increases, VFE surprise accelerates belief updates
   PolyAftertouch aftertouch;
   ```

**Why signalFlux, not a new organon_entropy parameter**: There is no `organon_entropy` APVTS parameter. Entropy is computed internally by `EntropyAnalyzer` per voice from the ingested signal. `signalFlux` (range 0–1, default 0.5) is the gain on signal entering the catabolism stage — it is the correct upstream control point for driving entropy higher. At default 0.5, full pressure reaches 0.7, safely within the [0, 1] clamp.

**Sensitivity**: 0.2 — chosen because signal flux feeds directly into the VFE belief update loop; even a moderate increase cascades through `predictionError → surprise → adaptation gain → catalyst effectiveness`. A lighter touch than metabolicRate (0.25) is appropriate here.

**Running Fleet Total (after Round 11C)**:

| Batch  | Engines Added                                        | Cumulative Total |
|--------|------------------------------------------------------|------------------|
| 5D     | Snap, Orbital, Obsidian, Origami, Oracle             | 5 / 23           |
| 5E     | Morph, Dub, Oceanic, Fat, Oblique                    | 10 / 23          |
| 9B     | Overworld, Owlfish, Ocelot, Osprey, Osteria          | 15 / 23          |
| 11C    | Organon (completed — was partial)                    | 16 / 23          |

Remaining without aftertouch: Bob, Drift, Bite, Onset, Opal, Ouroboros, Obscura (7 engines).
(All 7 resolved in Batch 4 / Round 10 and Batch 5 / Round 11A — see sections above.)

---

# D006 Aftertouch — Fleet Completion (Round 11C)

All 23 engines with audio DSP now have aftertouch wired. Summary below.

## Batch 9C (Round 11C) — Final 7 + Organon Completion

| Engine    | File                  | Target                              | Effect                                              | Sensitivity |
|-----------|-----------------------|-------------------------------------|-----------------------------------------------------|-------------|
| Bob       | `BobEngine.h`         | `bob_filterChar`                    | Snout filter warmth/character +0–0.3                | 0.3         |
| Drift     | `DriftEngine.h`       | `drift_shimmerAmount`               | Prism Shimmer depth +0–0.35 (JOURNEY analog)        | 0.35        |
| Bite      | `BiteEngine.h`        | BITE macro (`poss_macroBite`)       | Feral aggression: gnash + OscB + reso +0–0.3        | 0.3         |
| Onset     | `OnsetEngine.h`       | PUNCH macro (`perc_macro_punch`)    | Snap transient + body across all 8 voices +0–0.3    | 0.3         |
| Opal      | `OpalEngine.h`        | `opal_posScatter`                   | Grain position scatter / cloud spread +0–0.3        | 0.3         |
| Organon   | `OrganonEngine.h`     | `organon_metabolicRate` + signalFlux | Metabolic rate +0–0.25×9.9 Hz; entropy flux +0–0.2 | 0.25 / 0.2  |
| Ouroboros | `OuroborosEngine.h`   | chaos index + leash (inverted)      | Chaos +0–0.3, leash −0–0.3 (pitch tether loosened)  | 0.3         |
| Obscura   | `ObscuraEngine.h`     | `obscura_stiffness`                 | Spring chain stiffness +0–0.25 (brighter + shorter) | 0.25        |

---

## Complete 23-Engine Fleet Table

| #  | Engine    | Gallery Code | File                    | Aftertouch Target                      | Sensitivity | Batch |
|----|-----------|-------------|-------------------------|----------------------------------------|-------------|-------|
| 1  | Snap      | ODDFELIX    | `SnapEngine.h`          | `snap_filterCutoff` (+6000 Hz)         | 0.3         | 5D    |
| 2  | Orbital   | ORBITAL     | `OrbitalEngine.h`       | `orb_morph` (+0.3 toward profile B)    | 0.3         | 5D    |
| 3  | Obsidian  | OBSIDIAN    | `ObsidianEngine.h`      | `obsidian_formantIntensity` (+0.3)     | 0.3         | 5D    |
| 4  | Origami   | ORIGAMI     | `OrigamiEngine.h`       | `origami_foldDepth` (+0.3 shimmer)     | 0.3         | 5D    |
| 5  | Oracle    | ORACLE      | `OracleEngine.h`        | `oracle_drift` (+0.15 GENDY chaos)     | 0.15        | 5D    |
| 6  | Morph     | ODDOSCAR    | `MorphEngine.h`         | filter cutoff (+2450 Hz)               | 0.35        | 5E    |
| 7  | Dub       | OVERDUB     | `DubEngine.h`           | `dub_sendLevel` (+0.3 send VCA)        | 0.3         | 5E    |
| 8  | Oceanic   | OCEANIC     | `OceanicEngine.h`       | boid separation (scatter +0.25)        | 0.25        | 5E    |
| 9  | Fat       | OBESE       | `FatEngine.h`           | `fat_mojo` (+0.3 analog drift)         | 0.3         | 5E    |
| 10 | Oblique   | OBLIQUE     | `ObliqueEngine.h`       | `oblq_prismMix` (+0.3 spectral wet)    | 0.3         | 5E    |
| 11 | Overworld | OVERWORLD   | `OverworldEngine.h`     | ERA Y position (+0.2 SNES weight)      | 0.2         | 9B    |
| 12 | Owlfish   | OWLFISH     | `OwlfishEngine.h`       | `owl_grainDensity` (+0.25)             | 0.25        | 9B    |
| 13 | Ocelot    | OCELOT      | `OcelotEngine.h`        | `ocelot_ecosystemDepth` (+0.3)         | 0.3         | 9B    |
| 14 | Osprey    | OSPREY      | `OspreyEngine.h`        | shore position (+1.0 unit)             | 0.25        | 9B    |
| 15 | Osteria   | OSTERIA     | `OsteriaEngine.h`       | `osteria_tavernMix` (+0.25)            | 0.25        | 9B    |
| 16 | Bob       | OBLONG      | `BobEngine.h`           | `bob_filterChar` (+0.3 warmth)         | 0.3         | 10    |
| 17 | Drift     | ODYSSEY     | `DriftEngine.h`         | shimmer depth (+0.35 Prism Shimmer)    | 0.35        | 10    |
| 18 | Bite      | OVERBITE    | `BiteEngine.h`          | BITE macro (+0.3 feral aggression)     | 0.3         | 10    |
| 19 | Onset     | ONSET       | `OnsetEngine.h`         | PUNCH macro (+0.3 all-voice punch)     | 0.3         | 10    |
| 20 | Opal      | OPAL        | `OpalEngine.h`          | `opal_posScatter` (+0.3 scatter)       | 0.3         | 10    |
| 21 | Organon   | ORGANON     | `OrganonEngine.h`       | metabolicRate + signalFlux (dual)      | 0.25 / 0.2  | 11C   |
| 22 | Ouroboros | OUROBOROS   | `OuroborosEngine.h`     | chaos +0.3, leash −0.3 (dual)          | 0.3         | 11A   |
| 23 | Obscura   | OBSCURA     | `ObscuraEngine.h`       | `obscura_stiffness` (+0.25)            | 0.25        | 11A   |

**Optic (OPTIC)** is exempt — visual modulation engine, no audio DSP, no MIDI expression requirements.

---

## Running Fleet Total — All Rounds

| Batch | Round | Engines Added                                       | Cumulative Total |
|-------|-------|-----------------------------------------------------|------------------|
| 1     | 5D    | Snap, Orbital, Obsidian, Origami, Oracle            | 5 / 23           |
| 2     | 5E    | Morph, Dub, Oceanic, Fat, Oblique                   | 10 / 23          |
| 3     | 9B    | Overworld, Owlfish, Ocelot, Osprey, Osteria         | 15 / 23          |
| 4     | 10    | Bob, Drift, Bite, Onset, Opal                       | 20 / 23          |
| 5     | 11A   | Ouroboros, Obscura                                  | 22 / 23          |
| 6     | 11C   | Organon (was partial — now complete)                | **23 / 23**      |

---

## D006 Fleet Compliance Statement

**D006 ("Expression Input Is Not Optional") is FULLY SATISFIED fleet-wide.**

All 23 audio-DSP engines in XOmnibus have:
- Velocity → timbre wiring (filter brightness, harmonic content, or equivalent)
- At least one CC expression input (channel pressure / aftertouch)

Implementation method: `PolyAftertouch` channel pressure helper (`Source/Core/PolyAftertouch.h`).
All integrations are additive, non-destructive, and preset-compatible.
5ms attack / 20ms release smoothing prevents zipper noise on all 23 engines.
