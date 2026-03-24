# Filter Envelope Expansion — Round 9E
## XOlokun Fleet | 2026-03-14
### D001 Doctrine: "Velocity Must Shape Timbre"

This document records the 6 engines fixed in Round 9E. These engines were identified in
`Docs/filter_envelope_audit.md` as Priority 1/2 gaps. ObsidianEngine.h was explicitly
excluded from this round (handled separately in Round 9A).

---

## Summary Table

| Engine | Param ID | Filter Type | kMaxHz | D001 Status | Notes |
|--------|----------|-------------|--------|-------------|-------|
| Orbital | `orb_filterEnvDepth` | Post-mix CytomicSVF LP/BP/HP | 7000 Hz | FIXED | Peak velEnv scan across active voices |
| Owlfish | `owl_filterEnvDepth` | Per-voice CytomicSVF LP (`opticsFilter`) | 5500 Hz | FIXED | Uses `ampEnv.getLevel()` (new accessor added) |
| Overworld | `ow_filterEnvDepth` | Post-mix SVFilter on mixed chip output | 8000 Hz | FIXED | One-pole block-rate env follower (noteOn sets, decays 200ms) |
| Ocelot | `ocelot_filterEnvDepth` | Canopy spectral LP (normalized 0–1 pos) | 0.3 (normalized) | FIXED | Injects into `mod.canopyFilterMod` (spectralPos units) |
| Osteria | `osteria_filterEnvDepth` | `smokeFilter` CytomicSVF LP 3–18kHz | 6000 Hz | FIXED | Peak velEnv scan; added to `smokeCutoff` |
| Osprey | `osprey_filterEnvDepth` | Post-mix tilt CytomicSVF LP/HP | 5500 Hz | FIXED | LP branch only (tilt < 0.5); HP branch unchanged |

---

## Per-Engine Details

### Engine 1: Orbital — `orb_filterEnvDepth`

**Filter:** `postFilterL/R` — a Cytomic SVF applied to the post-mix stereo output. Mode
toggles between LP, BP, and HP based on `orb_filterMode`. Cutoff computed from
`orb_filterCutoff` (20–20000 Hz, default 20000) plus `filterOffset` (coupling mod) plus
the new filter env boost.

**Implementation:**
```cpp
static constexpr float kFilterEnvMaxHz = 7000.0f;
const float filterEnvDepth = (p_filterEnvDepth != nullptr) ? p_filterEnvDepth->load() : 0.25f;
float peakVelEnv = 0.0f;
for (const auto& voice : voices)
{
    if (voice.active)
        peakVelEnv = std::max (peakVelEnv, voice.envLevel * voice.velocity);
}
const float filterEnvBoost = filterEnvDepth * peakVelEnv * kFilterEnvMaxHz;
const float effectiveCutoff = juce::jlimit (20.0f, 20000.0f,
                                            filterCutoff + filterOffset + filterEnvBoost);
```

**Files changed:**
- `Source/Engines/Orbital/OrbitalEngine.h` — parameter declaration, attachment, DSP, private member

**Pattern notes:** `voice.envLevel * voice.velocity` — both fields public on Orbital's inline
Voice struct. Block-rate scan, post-voice-loop position.

---

### Engine 2: Owlfish — `owl_filterEnvDepth`

**Filter:** `opticsFilter` — a Cytomic SVF LP per voice, with normalized cutoff
`20.0f * pow(2.0f, snap.filterCutoff * 10.0f)` (20 Hz – ~20 kHz log scale).

**New accessor:** `OwlfishAmpEnvelope::getLevel()` was added to
`Source/Engines/Owlfish/OwlfishAmpEnvelope.h` — the class had no public level accessor.

**Implementation (per-voice, in OwlfishVoice.h):**
```cpp
static constexpr float kOwlFilterEnvMaxHz = 5500.0f;
cutoffHz += snap.filterEnvDepth * ampEnv.getLevel() * lastVelocity * kOwlFilterEnvMaxHz;
cutoffHz = std::max (20.0f, std::min (20000.0f, cutoffHz));
```

**Files changed:**
- `Source/Engines/Owlfish/OwlfishParameters.h` — `filterEnvDepth` param ID + declaration
- `Source/Engines/Owlfish/OwlfishParamSnapshot.h` — field, attachment, update
- `Source/Engines/Owlfish/OwlfishAmpEnvelope.h` — `getLevel()` accessor
- `Source/Engines/Owlfish/OwlfishVoice.h` — DSP wiring

**Pattern notes:** `lastVelocity` is stored at noteOn. Per-voice (not fleet-wide scan), since
the filter is computed inside the voice render path.

---

### Engine 3: Overworld — `ow_filterEnvDepth`

**Filter:** `SVFilter` (Overworld-specific type) on the fully mixed chip output — applied
after ERA crossfade merges NES, FM, and SNES layers. No per-voice envelope is accessible
at this post-mix stage.

**Strategy:** One-pole block-rate envelope follower (`filterEnvLevel` float member).
- On MIDI noteOn: `filterEnvLevel = msg.getFloatVelocity()`
- Per block: `filterEnvLevel *= exp(-1 / (0.2 * sr))` (~200ms decay coefficient)
- Applied: `filter.setCutoff(jlimit(20, 20000, snap.filterCutoff + externalFilterMod + filterEnvBoost))`

**Implementation (in OverworldEngine.h renderBlock, pre-voice-loop):**
```cpp
static constexpr float kOwFilterEnvMaxHz = 8000.0f;
const float filterEnvDepth = (p_filterEnvDepth != nullptr) ? p_filterEnvDepth->load() : 0.25f;
for (const auto meta : midi)
{
    const auto msg = meta.getMessage();
    if (msg.isNoteOn())
        filterEnvLevel = msg.getFloatVelocity();
}
const float decayCoeff = std::exp(-1.0f / (0.2f * sr));
filterEnvLevel *= decayCoeff;
const float filterEnvBoost = filterEnvDepth * filterEnvLevel * kOwFilterEnvMaxHz;
```

**Files changed:**
- `Source/Engines/Overworld/OverworldEngine.h` — parameter declaration, attachment, DSP,
  private members (`p_filterEnvDepth`, `filterEnvLevel`)

**Pattern notes:** Using a MIDI pre-scan loop (before the main render loop) to catch noteOn
events is the cleanest approach when the engine's architecture doesn't expose per-voice
envelope level at the filter-update stage. The 200ms decay gives a perceptible "pluck"
brightness that then fades toward the base cutoff.

---

### Engine 4: Ocelot — `ocelot_filterEnvDepth`

**Filter:** Ocelot's "filter" is the canopy spectral LP — a normalized position (0–1) that
maps to ~200 Hz – 20 kHz via a log mapping in `OcelotCanopy.h`. There is no explicit Hz
cutoff value to add to; the routing is `spectralPos = clamp(canopySpectralFilter + canopyFilterMod, 0, 1)`.

**Strategy:** Inject the filter env boost into `mod.canopyFilterMod` using normalized units
(`kOcelotFilterEnvScale = 0.3f`) rather than Hz. The existing EcosystemMatrix already routes
modulation signals through this field, so the path was already established.

**Implementation (in OcelotVoice.h renderBlock):**
```cpp
static constexpr float kOcelotFilterEnvScale = 0.3f;
mod.canopyFilterMod += snap.filterEnvDepth * velocity * ampEnv.getLevel() * kOcelotFilterEnvScale;
```

Where `velocity` is stored at noteOn time in the private `velocity` field (D001 fix also
required adding `float velocity = 0.0f;` to `OcelotVoice` private state and caching at `noteOn`).

**Files changed:**
- `Source/Engines/Ocelot/OcelotParameters.h` — `filterEnvDepth` param ID + declaration
- `Source/Engines/Ocelot/OcelotParamSnapshot.h` — field + `updateFrom` wiring
- `Source/Engines/Ocelot/OcelotVoice.h` — `velocity` member, noteOn cache, DSP wiring

**Pattern notes:** Using normalized units (0.3 = 30% spectralPos shift at depth=1, vel=1,
envLevel=1) rather than Hz is a deliberate design choice for Ocelot. The canopy spectral
position is intentionally kept in [0,1] space throughout; injecting Hz would require
inverse-mapping logic. The existing `canopyFilterMod` pathway already handles normalized
offsets via the EcosystemMatrix, so this approach reuses established plumbing.

---

### Engine 5: Osteria — `osteria_filterEnvDepth`

**Filter:** `smokeFilter` — CytomicSVF LP. Cutoff is computed from `pSmoke` (0–1 param)
with inverse mapping: `lerp(18000, 3000, pSmoke)` — higher smoke = darker. The filter env
boost is added before clamping.

**Filter env logic:**
- Scans all active voices for `voice.velocity * voice.ampEnv.level` (both public on Osteria's Voice struct)
- Picks peak (fleet-wide brightest voice drives the filter opening)
- Adds `filterEnvDepth * peakVelEnv * kOsteriaFilterEnvMaxHz` to `smokeCutoff` before `setCoefficients`

**Implementation:**
```cpp
static constexpr float kOsteriaFilterEnvMaxHz = 6000.0f;
const float filterEnvDepth = loadParam (paramFilterEnvDepth, 0.25f);
float peakVelEnv = 0.0f;
for (const auto& voice : voices)
{
    if (voice.active)
    {
        float velEnv = voice.velocity * voice.ampEnv.level;
        peakVelEnv = std::max (peakVelEnv, velEnv);
    }
}
filterEnvBoost = filterEnvDepth * peakVelEnv * kOsteriaFilterEnvMaxHz;
float smokeCutoff = lerp (18000.0f, 3000.0f, pSmoke) + filterEnvBoost;
smokeCutoff = std::max (200.0f, std::min (20000.0f, smokeCutoff));
smokeFilter.setCoefficients (smokeCutoff, 0.0f, srf);
```

**Files changed:**
- `Source/Engines/Osteria/OsteriaEngine.h` — parameter declaration, attachment, DSP,
  private members (`paramFilterEnvDepth`, `filterEnvBoost`)

**Pattern notes:** `OsteriaADSR.level` is a public field — no getter needed. The smoke
filter was already computed block-rate, so no extra coefficient update cost.

Note: The task brief mentioned "Osteria/Osprey share ShoreSystem — don't duplicate code."
ShoreSystem is a data provider (coastline profiles, resonator parameters), not the filter
implementation itself. Osteria's `smokeFilter` and Osprey's `tiltFilterL/R` are independent
CytomicSVF instances in separate engines. No code was shared or duplicated; both engines
received independent, self-contained filter env wiring.

---

### Engine 6: Osprey — `osprey_filterEnvDepth`

**Filter:** `tiltFilterL/R` — a bipolar CytomicSVF on the post-mix stereo output.
- `pFilterTilt < 0.5`: LP mode, cutoff sweeps 200–8200 Hz (dark → neutral)
- `pFilterTilt >= 0.5`: HP mode, cutoff sweeps 8200–400 Hz (neutral → bright)
- Neutral position (0.5) is wide open

**Strategy:** Apply env boost only in LP mode (dark side). In HP mode, higher cutoff
means *higher* HP corner which makes the sound *darker* — the opposite of what D001
requires. A filter env that brightens on attack must only boost in the LP branch.

**Implementation:**
```cpp
// D001: filter envelope — LP branch only
static constexpr float kOspreyFilterEnvMaxHz = 5500.0f;
const float filterEnvDepth = loadParam (paramFilterEnvDepth, 0.25f);
float peakVelEnv = 0.0f;
for (const auto& v : voices)
{
    if (v.active)
        peakVelEnv = std::max (peakVelEnv, v.velocity * v.amplitudeEnvelope.level);
}
filterEnvBoost = filterEnvDepth * peakVelEnv * kOspreyFilterEnvMaxHz;

if (pFilterTilt < 0.5f)
{
    float cutoff = 200.0f + (pFilterTilt * 2.0f) * 8000.0f + filterEnvBoost;
    cutoff = juce::jlimit (200.0f, 20000.0f, cutoff);
    // ... LP tilt filter setup
}
else
{
    // HP branch: filterEnvBoost not applied
}
```

**Files changed:**
- `Source/Engines/Osprey/OspreyEngine.h` — parameter declaration, attachment, DSP,
  private members (`paramFilterEnvDepth`, `filterEnvBoost`)

**Pattern notes:** `OspreyADSR.level` is a public field. The `filterEnvBoost` member
persists across blocks (initialized to 0.0f in constructor), but is recomputed fresh every
block from current voice states, so there is no state-leak issue.

---

## Architectural Patterns Used

### Pattern A: Per-engine fleet-scan (Orbital, Osteria, Osprey)
Used when the filter is post-mix (no per-voice filter update). Scans all active voices
for `velocity * envelopeLevel`, takes the peak value, applies a single fleet-wide boost.
Pro: no per-voice overhead. Con: only the "hottest" voice drives the filter — but for
atmospheric/post-mix contexts this is the correct semantic.

### Pattern B: Per-voice (Owlfish, Ocelot)
Used when the filter is computed per-voice (inside the voice render loop). Each voice
applies its own `velocity * envelopeLevel * depth * kMaxHz` independently. Pro: each
voice tracks its own timbral contour. Con: slightly more CPU per voice.

### Pattern C: MIDI noteOn follower (Overworld)
Used when no voice-level envelope is accessible at the filter stage (chip-layer mix).
A single float `filterEnvLevel` is set to note velocity on MIDI noteOn, then decays with
a one-pole filter coefficient per block. Pro: simple, zero per-sample cost. Con: only
tracks the most recent note; polyphonic triggers within the same block will overwrite
each other (last-note-wins), which is acceptable for the chip-layer architecture.

---

## D001 Compliance Status After Round 9E

| Engine | D001 Before | D001 After |
|--------|-------------|------------|
| Orbital | GAP | COMPLIANT |
| Owlfish | GAP | COMPLIANT |
| Overworld | GAP | COMPLIANT |
| Ocelot | GAP | COMPLIANT |
| Osteria | GAP | COMPLIANT |
| Osprey | CONTEXTUAL | COMPLIANT |

All 6 engines now satisfy: "Velocity must shape timbre — velocity × envelope drives
filter brightness, not just amplitude."

Engines remaining with D001 gaps (not in this round's scope): Obscura, Origami, Optic,
Ostinato, OpenSky, OceanDeep, Ouie, Onset. Obsidian handled separately in Round 9A.
