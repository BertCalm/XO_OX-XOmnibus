# XOutshine Sample Instrument Forge — Format Specification v1.0
**Rex's Domain — Authoritative Reference for Universal Instrument Building**
*2026-03-22 — Supersedes the drum-only pipeline assumptions in the original xoutshine.py docstring*

---

## Table of Contents

1. [Scope and Versioning](#1-scope-and-versioning)
2. [Expanded SampleCategory Enum](#2-expanded-samplecategory-enum)
3. [Multi-Source Keygroup Format](#3-multi-source-keygroup-format)
4. [Velocity Strategy Definitions](#4-velocity-strategy-definitions)
5. [Round-Robin Specification](#5-round-robin-specification)
6. [Formant Preservation Metadata](#6-formant-preservation-metadata)
7. [FX Routing Metadata](#7-fx-routing-metadata)
8. [Expression Mapping](#8-expression-mapping)
9. [The 9-Stage Pipeline](#9-the-9-stage-pipeline)
10. [XPM XML Structure Examples](#10-xpm-xml-structure-examples)
11. [MPC Compatibility Constraints](#11-mpc-compatibility-constraints)
12. [Rex's Extended Golden Rules](#12-rexs-extended-golden-rules)

---

## 1. Scope and Versioning

### 1.1 What XOutshine Is

XOutshine is the **Sample Instrument Forge** — it takes raw material (WAV files, existing XPN archives,
or folders of heterogeneous samples) and upgrades them into production-quality MPC instruments. The
original pipeline handled drum-focused sample packs only. This specification defines the **expanded
universal pipeline** capable of building:

- Drum programs (unchanged from v1)
- Single-sample keygroup instruments (melodic stretch)
- **Multi-source keygroup instruments** (pad + organ + horn stacked or crossfaded)
- Round-robin melodic instruments (piano, organ, pluck)
- Velocity-crossfade instruments (orchestral, hybrid)
- Formant-aware instruments (vocal, brass, wind)

### 1.2 Document Authority

This document is authoritative over:
- `Source/Export/XOutshine.h` — C++ implementation
- `Tools/xoutshine.py` — Python CLI
- Any future `.xoforge` descriptor files

When code and this spec conflict, **this spec wins** until a new spec version is published.

### 1.2.1 Implementation Note (2026-03-25)

The Python CLI (`Tools/xoutshine.py`) is the **PROTOTYPE** implementation. The C++ header
(`Source/Export/XOutshine.h`) is the **CANONICAL** implementation for the XOceanus desktop
application. When behavior diverges, the C++ implementation is authoritative. The Python CLI
may be used for batch processing and testing but is not the primary delivery path.

### 1.3 Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-03-22 | Initial universal forge spec — multi-source, formant, FX routing, 9-stage pipeline |

---

## 2. Expanded SampleCategory Enum

### 2.1 Full Enum Definition

The existing `SampleCategory` enum in `XOutshine.h` has melodic types declared but the pipeline
treats everything non-drum as a generic keygroup. The expanded enum adds subcategories needed for
intelligent routing, XPM program type selection, and per-category default behavior.

```cpp
enum class SampleCategory
{
    // ── Drum / Percussion ──────────────────────────────────────────────────
    Kick,           // Bass drum — low ZCR, high sub energy, short attack
    Snare,          // Snare — mid ZCR, transient peak in 150–800 Hz
    HiHatClosed,    // Closed hat — high ZCR (>3kHz energy), short decay
    HiHatOpen,      // Open hat — high ZCR, longer sustain tail
    Clap,           // Hand clap — multi-transient envelope (3–5 impulses)
    Rimshot,        // Rimshot / side stick — sharp transient, pitched crack
    Tom,            // Rack/floor tom — pitched transient, moderate ZCR
    Percussion,     // General percussion — tambourine, shaker, cowbell, etc.
    Cymbal,         // Crash/ride — broad spectrum, long release
    FX,             // FX one-shots — risers, impacts, reverse hits

    // ── Bass ──────────────────────────────────────────────────────────────
    Bass,           // Bass synth / DI bass — sub energy, pitched, short-medium
    SubBass,        // 808 / sub — fundamental below 100 Hz, minimal harmonics

    // ── Melodic / Tonal ───────────────────────────────────────────────────
    Pad,            // Atmospheric pad — slow attack, long sustain, low brightness
    Lead,           // Synth lead — fast attack, bright, expressive
    Keys,           // Piano, Rhodes, electric piano — fast transient, mid decay
    Organ,          // Organ / tonewheel — zero attack, stable sustain (loopable)
    Pluck,          // Guitar, harp, mallet — fast attack, exponential decay
    String,         // Bowed strings — slow attack, sustained, formant-rich
    Brass,          // Trumpet, trombone, horn — fast attack, formant cluster
    Wind,           // Flute, sax, clarinet — breathy attack, formant-dependent
    Vocal,          // Voice / choir — strong formant structure, pitch-sensitive
    Bell,           // Bell, glockenspiel — inharmonic sustain, clear fundamental
    Mallet,         // Marimba, vibraphone — pitched transient, fast decay
    Choir,          // Choral texture — dense harmonic structure, slow evolution

    // ── Texture / Atmosphere ──────────────────────────────────────────────
    Texture,        // Evolving texture / noise — unpitched or weakly pitched
    Loop,           // Rhythmic loop — tempo-synced content
    Drone,          // Sustained drone — minimal movement, long sustain
    Foley,          // Environmental / sound design one-shots

    Unknown         // Classification failed — triggers manual override prompt
};
```

### 2.2 Classification Heuristics

The classify stage uses a three-pass strategy: **filename keywords → audio features → SonicDNA**.
Audio classification is only invoked when filename classification returns `Unknown`.

#### Pass 1: Filename Keywords

| Category | Trigger Keywords (case-insensitive) |
|----------|-------------------------------------|
| Kick | kick, bd, bassdrum, bass_drum, kik |
| Snare | snare, snr, sd, sn |
| Rimshot | rim, rimshot, sidestick, side_stick, rs |
| HiHatClosed | hihat, hh, hat, hhcl, chh, closed |
| HiHatOpen | ohh, ohat, openhat, open_hat, oh |
| Clap | clap, clp, handclap, cp |
| Tom | tom, fl_tom, floor, rack, flr |
| Cymbal | crash, ride, cym, crsh |
| Percussion | perc, shaker, tamb, cowbell, clave, wood, stick, bongo, conga |
| FX | fx, sfx, riser, impact, whoosh, reverse, swish, foley |
| SubBass | 808, sub, subbass, sub_bass |
| Bass | bass, sub (without 808 context) |
| Pad | pad, atmo, ambient, atmosphere, wash, drone (if short: no) |
| Lead | lead, ld, synth, arp (with short attack) |
| Keys | key, piano, rhodes, ep, keys, wurlitzer, clav, clavinet |
| Organ | organ, org, b3, hammond, tonewheel |
| Pluck | pluck, guitar, gtr, harp, mallet |
| String | string, vln, violin, viola, cello, str, orch |
| Brass | brass, trumpet, trombone, horn, tuba, brs |
| Wind | flute, sax, clarinet, oboe, bassoon, wind |
| Vocal | vox, vocal, voice, choir, choral, sing |
| Bell | bell, glock, glocken, celesta, chime |
| Mallet | marimba, vibraphone, vibe, xylophone |
| Texture | texture, noise, grain, shimmer, granular |
| Drone | drone |
| Loop | loop, lp (+ length heuristic: duration % common BPM grids) |
| Foley | foley, env, environment, room, ambience |

#### Pass 2: Audio Feature Heuristics

Applied only when Pass 1 returns `Unknown`.

```
Given: duration (s), ZCR (zero-crossing rate / sample_rate),
       subEnergy (0–200 Hz ratio), midEnergy (200–4kHz ratio),
       highEnergy (4k+ Hz ratio), attackTime (ms to peak),
       tailLength (s at -60 dBFS), isLoopable

Drum branch (duration < 2.5s AND attack < 40ms):
    ZCR < 500   → Kick   (dominant sub energy)
    ZCR 500–1500 AND subEnergy > 0.3  → Tom
    ZCR 500–2000 → Snare
    ZCR > 3000  → HiHatClosed
    ZCR > 2000 AND tailLength > 0.3s → HiHatOpen

Tonal branch (duration >= 0.5s):
    attackTime > 200ms AND isLoopable AND midEnergy > 0.4  → Pad
    attackTime > 200ms AND NOT isLoopable                   → Texture
    attackTime < 20ms AND duration < 3s AND ZCR < 1500     → Pluck
    attackTime < 20ms AND isLoopable                        → Keys
    duration > 5s AND isLoopable                            → Organ or Pad
        subEnergy > 0.4  → Bass
        highEnergy > 0.5 → Lead

Default fallback:
    duration < 0.5s   → Percussion
    duration < 3.0s   → Pluck
    duration >= 3.0s  → Pad
```

#### Pass 3: SonicDNA Override

The SonicDNA (6D fingerprint computed in xoutshine.py) can override Pass 2 classifications:

| DNA signature | Overrides to |
|--------------|--------------|
| brightness > 0.8, aggression > 0.7, duration < 0.5s | HiHatClosed |
| warmth > 0.7, brightness < 0.3, movement < 0.2 | SubBass or Drone |
| movement > 0.7, density > 0.6 | Texture |
| brightness > 0.6, warmth > 0.5, movement 0.3–0.6, isLoopable | Organ |
| brightness > 0.5, aggression > 0.5, attackTime < 10ms | Lead |

### 2.3 Program Type Selection

| Category Group | XPM Program Type | KeyTrack | OneShot |
|---------------|-----------------|---------|---------|
| Kick, Snare, Rimshot, HiHatClosed, HiHatOpen, Clap, Tom, Cymbal, Percussion, FX, Foley | Drum | False | True |
| Bass, SubBass, Lead, Keys, Organ, Pluck, Bell, Mallet | Keygroup | True | False |
| Pad, String, Brass, Wind, Vocal, Choir, Texture, Drone | Keygroup | True | False |
| Loop | Drum (tempo-synced) or Keygroup depending on pitch content | Conditional | True |

### 2.4 Default ZonePlay per Category

Inherits from the Bible's Smart Mode table, extended for melodic types:

| Category | ZonePlay | CycleType | Rationale |
|----------|---------|-----------|-----------|
| Kick | 1 (velocity) | — | Dynamics are the point |
| Snare | 1 (velocity) | — | Dynamics are the point |
| HiHatClosed | 2 (cycle) | RoundRobin | Machine-gun prevention |
| HiHatOpen | 2 (cycle) | RoundRobin | Machine-gun prevention |
| Clap | 3 (random) | Random | Organic feel |
| Tom | 1 (velocity) | — | Dynamics matter |
| Cymbal | 2 (cycle) | RoundRobin | Natural variation |
| Percussion | 2 (cycle) | RoundRobin | Variation prevents repetition |
| FX | 4 (random-norepeat) | RandomNoRepeat | Never same FX twice |
| Keys | 2 (cycle) | RoundRobin | Piano machine-gun prevention |
| Pluck | 2 (cycle) | RoundRobin | Guitar realism |
| Mallet | 2 (cycle) | RoundRobin | Marimba variation |
| Bell | 2 (cycle) | RoundRobin | Subtle variation |
| All other melodic | 1 (velocity) | — | Dynamics primary |

---

## 3. Multi-Source Keygroup Format

### 3.1 Concept

A **multi-source keygroup** is a single playable instrument zone built from multiple independent
sample sources. Sources are stacked within a zone, each with independent velocity, pitch, and
formant parameters. The XOutshine DESIGN stage creates these composite zones.

Example: a hybrid organ-pad-brass stack:
- **Zone**: C2–C6, root C4
- **Layer A** (Pad): active vel 1–60, full range
- **Layer B** (Organ): active vel 30–90, crossfade with Pad 30–60
- **Layer C** (Brass): active vel 80–127, crossfade with Organ 70–90

### 3.2 In-Memory Representation (C++)

The `UpgradedProgram` struct must be extended:

```cpp
// Source sample within a composite zone
struct CompositeSource
{
    juce::File       sourceFile;
    juce::String     sourceName;
    SampleCategory   sourceCategory  = SampleCategory::Unknown;

    // Velocity mapping for this source within the zone
    int  velStart        = 1;      // MIDI velocity range (active range)
    int  velEnd          = 127;
    int  xfadeDownStart  = 0;      // Fade-in start vel (0 = hard cut at velStart)
    int  xfadeUpEnd      = 0;      // Fade-out end vel (0 = hard cut at velEnd)

    // Pitch
    int   rootNote       = 60;     // Detected or user-set MIDI note (C4)
    int   transposeRange = 24;     // Max semitones above/below root before artifacts
    float tuneFine       = 0.0f;   // Fine tune in cents (-100 to +100)

    // Formant
    float formantFreqHz  = 0.0f;   // 0 = not a formant-sensitive source
    float formantLockSemitones = 0.0f;  // Semitones of safe transposition

    // Layering order
    int   stackOrder     = 0;      // 0 = bottom, higher = on top
    float gainDb         = 0.0f;   // Per-source gain offset
};

// Complete composite zone — one zone maps to one XPM Keygroup
struct CompositeKeygroup
{
    int  lowNote   = 0;
    int  highNote  = 127;
    int  rootNote  = 60;           // Zone root (for pitch display only)

    std::vector<CompositeSource> sources;

    // Zone-level loop
    bool   hasLoop     = false;
    int    loopStart   = 0;
    int    loopEnd     = 0;
    int    loopXfadeMs = 0;

    // Zone-level envelope
    float attackMs   = 0.0f;
    float decayMs    = 0.0f;
    float sustain    = 1.0f;
    float releaseMs  = 200.0f;
};

// Extended UpgradedProgram — replaces the existing struct
struct UpgradedProgram
{
    juce::String     name;
    SampleCategory   category;       // Dominant category
    AnalyzedSample   sourceInfo;     // Primary source analysis
    VelocityStrategy velocityStrategy = VelocityStrategy::Layer;

    // Single-source layers (existing pipeline, drum + simple keygroup)
    std::vector<EnhancedLayer> layers;
    int numVelocityLayers = 4;
    int numRoundRobin     = 4;

    // Multi-source composite (new — melodic instruments)
    bool isComposite = false;
    std::vector<CompositeKeygroup> keygroups;  // One entry per note zone

    // FX routing metadata (Section 7)
    FXRoutingMetadata fxRouting;

    // Expression mapping metadata (Section 8)
    ExpressionMapping expression;
};
```

### 3.3 Zone Boundary Definitions

When a multi-source instrument has samples recorded at multiple root notes, the DESIGN stage
computes zone boundaries to minimize pitch stretching:

#### Zone layout strategies

**Every-minor-3rd (default for pitched instruments):**
Given root notes C2, Eb2, F#2, A2, C3, Eb3, ...
- Zone C2: LowNote = A#1 (lowest MIDI note – 1), HighNote = D2 (midpoint to Eb2)
- Zone Eb2: LowNote = D#2, HighNote = F2 (midpoint to F#2)
- Each zone spans ±1.5 semitones from root

Midpoint calculation:
```
zoneHigh[i] = floor((rootNote[i] + rootNote[i+1]) / 2)
zoneLow[i]  = zoneHigh[i-1] + 1
```
First zone low = 0, last zone high = 127.

**Octaves-only strategy (organs, pads with minimal pitch artifact concern):**
- One sample per octave, each zone spans 12 semitones exactly

**Crossfade overlap regions:**
For smooth transitions between zones, sources may share a velocity-overlap region.
The DESIGN stage adds a `xfadeDownStart` and `xfadeUpEnd` to each `CompositeSource`:

```
Source A active range:  velStart=1, velEnd=60
Source B active range:  velStart=40, velEnd=127
Overlap: vel 40–60 — A fades out, B fades in
xfadeUpEnd[A]     = 60   (A is full level to 60, then ramps down — implemented via Volume)
xfadeDownStart[B] = 40   (B starts ramping up at vel 40)
```

**Implementation note:** MPC does not natively render amplitude crossfades between layers in real
time based on velocity curves. Crossfades are baked into the WAV files during the ENHANCE stage
when `VelocityStrategy::Crossfade` is selected. See Section 4.3.

---

## 4. Velocity Strategy Definitions

### 4.1 Strategy Enum

```cpp
enum class VelocityStrategy
{
    Layer,      // Different source WAVs per velocity range (hard switch)
    Crossfade,  // Blended WAVs at transition zones (baked amplitude crossfade)
    Switch,     // Identical to Layer but with no transition overlap
    Generate,   // Single source → synthetic multi-layer via DSP
};
```

### 4.2 LAYER Strategy

Each velocity range maps to a distinct source WAV. Transitions are hard cuts at VelStart/VelEnd
boundaries. This is the current `xoutshine.py` default for drum programs and single-source keygroups.

**Use when:** Sources are genuinely different recordings (soft vs. hard hit, pad vs. organ vs. brass).
This creates the most timbral contrast across the velocity range.

**XPM representation:**
```xml
<Layers>
  <!-- Layer 0: pad, vel 1–60 -->
  <Layer index="0">
    <SampleName>HybridKeys__pad__v1.wav</SampleName>
    <VelStart>1</VelStart>
    <VelEnd>60</VelEnd>
    <Volume>0.75</Volume>
    <RootNote>0</RootNote>
    <KeyTrack>True</KeyTrack>
  </Layer>

  <!-- Layer 1: organ, vel 61–127 -->
  <Layer index="1">
    <SampleName>HybridKeys__organ__v2.wav</SampleName>
    <VelStart>61</VelStart>
    <VelEnd>127</VelEnd>
    <Volume>1.00</Volume>
    <RootNote>0</RootNote>
    <KeyTrack>True</KeyTrack>
  </Layer>

  <!-- Empty layer slots — CRITICAL: VelStart=0 prevents ghost triggering -->
  <Layer index="2">
    <VelStart>0</VelStart>
    <VelEnd>0</VelEnd>
    <Active>False</Active>
  </Layer>
  <Layer index="3">
    <VelStart>0</VelStart>
    <VelEnd>0</VelEnd>
    <Active>False</Active>
  </Layer>
</Layers>
```

### 4.3 CROSSFADE Strategy

Velocity boundary zones have baked amplitude crossfades. The ENHANCE stage generates
additional WAV files for the crossfade region where Source A fades out and Source B fades in.

**Bake formula for overlap region [velLo, velHi]:**
```
For each baked velocity step v in [velLo, velHi]:
    t = (v - velLo) / (velHi - velLo)  // 0.0 at velLo, 1.0 at velHi
    gainA = cos(t * PI/2)^2             // Equal-power fade-out (Source A)
    gainB = sin(t * PI/2)^2             // Equal-power fade-in  (Source B)
    baked_wav[v] = A * gainA + B * gainB
```

**XPM representation — baked crossfade layers:**
```xml
<!-- Source A: full level -->
<Layer index="0">
  <SampleName>Hybrid__pad__v1_full.wav</SampleName>
  <VelStart>1</VelStart>
  <VelEnd>39</VelEnd>
  <Volume>0.75</Volume>
  <RootNote>0</RootNote>
  <KeyTrack>True</KeyTrack>
</Layer>

<!-- Crossfade region: baked blend of A+B -->
<Layer index="1">
  <SampleName>Hybrid__xfade__v2.wav</SampleName>
  <VelStart>40</VelStart>
  <VelEnd>60</VelEnd>
  <Volume>0.88</Volume>
  <RootNote>0</RootNote>
  <KeyTrack>True</KeyTrack>
</Layer>

<!-- Source B: full level -->
<Layer index="2">
  <SampleName>Hybrid__organ__v3_full.wav</SampleName>
  <VelStart>61</VelStart>
  <VelEnd>127</VelEnd>
  <Volume>1.00</Volume>
  <RootNote>0</RootNote>
  <KeyTrack>True</KeyTrack>
</Layer>

<Layer index="3">
  <VelStart>0</VelStart>
  <VelEnd>0</VelEnd>
  <Active>False</Active>
</Layer>
```

**Layer count:** Crossfade uses 1 extra layer per crossfade point. A 2-source crossfade with 1
transition region = 3 layers. 3-source with 2 transitions = 5 layers. Maximum MPC layers per
keygroup is hardware-dependent — target 4 to guarantee compatibility; 8 is available on modern MPC.

### 4.4 SWITCH Strategy

Identical to LAYER but **no VelEnd/VelStart overlap** and no transition blending. Each layer has
a non-overlapping, contiguous velocity range. Use for programs that want hard sonic cuts at exact
velocity thresholds (e.g., unplugged vs. overdriven guitar at vel 80).

**XPM representation:** Same as LAYER but ensure adjacent VelEnd and VelStart are adjacent integers:
`Layer0 VelEnd=79`, `Layer1 VelStart=80`.

### 4.5 GENERATE Strategy

Single source WAV → 4 synthetic velocity layers via DSP. This is what the current `enhance()` in
`XOutshine.h` does. The pipeline applies:

| Layer | Amplitude | Low-pass alpha | Transient shaping |
|-------|-----------|---------------|-------------------|
| Ghost (vel 1–20) | 0.20 | 0.30 (heavy filter) | Transient attenuated 60% |
| Soft (vel 21–50) | 0.45 | 0.55 (moderate filter) | Transient attenuated 30% |
| Medium (vel 51–90) | 0.70 | 0.85 (light filter) | Transient natural |
| Hard (vel 91–127) | 1.00 | 1.00 (no filter) | Transient enhanced +20% |

**Transient shaper:** Detect onset via derivative peak detection. Boost/attenuate a ±5ms window
around the detected onset. This replicates the physical reality that hard hits have sharper, louder
transients and bright high-frequency content; soft hits have rounded, muted transients.

**Saturation curve for hard layer:**
```
y = tanh(x * 1.1) / tanh(1.1)   // soft clip, ~+0.8 dB harmonic addition
```

**GENERATE pipeline-specific XPM annotation:**
Append `_gen` to SampleName filenames so generated layers are distinguishable from source:
`Pad_C3_gen_v1.wav`, `Pad_C3_gen_v2.wav`, etc.

---

## 5. Round-Robin Specification

### 5.1 Storage Model: Pre-rendered Variants

XOutshine uses **pre-rendered variants** stored as distinct WAV files. This guarantees MPC
standalone compatibility without requiring the XOceanus plugin.

Each round-robin variant is a unique WAV file with micro-variations applied at render time.
The existing naming convention is extended:

```
{SampleName}__{source}_v{velLayer}_c{rrIndex}.wav

Examples:
  Piano_C3__keys_v2_c1.wav    (vel layer 2, RR variant 1 — unmodified master)
  Piano_C3__keys_v2_c2.wav    (vel layer 2, RR variant 2 — micro-pitch +2.1 cents)
  Piano_C3__keys_v2_c3.wav    (vel layer 2, RR variant 3 — micro-pitch -1.8 cents)
  Piano_C3__keys_v2_c4.wav    (vel layer 2, RR variant 4 — micro-pitch +3.3 cents)
```

RR variant 1 (`c1`) is ALWAYS the unmodified source for that velocity layer. Variants c2–cN
receive micro-variations.

### 5.2 Micro-Variation Parameters

| Parameter | Drum range | Melodic range | Implementation |
|-----------|-----------|---------------|----------------|
| Pitch deviation | ±5 cents | ±3 cents | Linear interpolation resample |
| Timing offset | ±2 ms pre-delay | 0 ms (no timing shift on melodic) | Leading silence pad |
| Filter shift | ±200 Hz LPF cutoff | ±100 Hz LPF cutoff | One-pole IIR |
| Amplitude jitter | ±0.5 dB | ±0.3 dB | Linear gain multiply |
| Saturation | 0–3% tanh drive | 0–1.5% tanh drive | Normalized tanh |

**Deterministic seeding:** Each variant uses a seeded RNG so re-runs produce identical outputs:
```cpp
std::mt19937 rng(static_cast<unsigned>(numSamples * 1000 + velLayer * 100 + rrIndex));
```

### 5.3 CycleType Values

| Value | MPC Behavior | Use Case |
|-------|-------------|---------|
| `RoundRobin` | Cycles 1→2→3→4→1→... | Hats, keys, plucks — predictable cycling |
| `Random` | Random selection on each trigger | Claps, snares — organic feel |
| `RandomNoRepeat` | Random but never the same RR twice in a row | FX, textures |

### 5.4 XPM Round-Robin Encoding

All RR variants for the same velocity layer share an identical `CycleGroup` number. Layers from
different velocity tiers must have different `CycleGroup` values to prevent cross-tier cycling.

```xml
<!-- Velocity layer 2 (vel 51–90), 3 RR variants -->
<Layer index="4">
  <SampleName>Piano_C3__keys_v2_c1.wav</SampleName>
  <VelStart>51</VelStart>
  <VelEnd>90</VelEnd>
  <Volume>0.75</Volume>
  <RootNote>0</RootNote>
  <KeyTrack>True</KeyTrack>
  <CycleType>RoundRobin</CycleType>
  <CycleGroup>2</CycleGroup>          <!-- Group 2 = vel layer 2 -->
  <ZonePlay>2</ZonePlay>
</Layer>

<Layer index="5">
  <SampleName>Piano_C3__keys_v2_c2.wav</SampleName>
  <VelStart>51</VelStart>
  <VelEnd>90</VelEnd>
  <Volume>0.75</Volume>
  <RootNote>0</RootNote>
  <KeyTrack>True</KeyTrack>
  <CycleType>RoundRobin</CycleType>
  <CycleGroup>2</CycleGroup>
  <ZonePlay>2</ZonePlay>
</Layer>

<Layer index="6">
  <SampleName>Piano_C3__keys_v2_c3.wav</SampleName>
  <VelStart>51</VelStart>
  <VelEnd>90</VelEnd>
  <Volume>0.75</Volume>
  <RootNote>0</RootNote>
  <KeyTrack>True</KeyTrack>
  <CycleType>RoundRobin</CycleType>
  <CycleGroup>2</CycleGroup>
  <ZonePlay>2</ZonePlay>
</Layer>
```

**CycleGroup assignment rule:** `CycleGroup = velocityLayerIndex + 1` (1-based). Ensures vel layer
1 → CycleGroup 1, vel layer 2 → CycleGroup 2, etc.

### 5.5 Maximum Layer Count Warning

MPC hardware has a per-keygroup/pad layer limit. The current known limits:

| MPC Model | Max layers per keygroup zone | Max total instruments |
|-----------|-----------------------------|-----------------------|
| MPC One / Live / X | 8 | 128 |
| MPC Studio | 8 | 128 |
| MPC Key 37/61 | 8 | 128 |
| MPC 2.x firmware | 4 (confirmed safe) | 64 |

**Safe target:** 4 velocity layers × 4 RR variants = 16 layers per zone. This exceeds the 8-layer
limit on all known hardware. **Recommendation:** Target 4 vel layers × 2 RR variants = 8 layers
max per zone for guaranteed compatibility. When `numRoundRobin > 2`, the VALIDATE stage must emit
a warning if total layers per zone exceed 8.

---

## 6. Formant Preservation Metadata

### 6.1 Why Formants Matter

For Vocal, Brass, Wind, String, and Choir categories, pitch transposition via `KeyTrack=True`
shifts the sample's fundamental pitch **and** the formant frequencies. This destroys the timbral
character at large intervals (a soprano voice transposed +1 octave becomes a chipmunk; a trumpet
transposed down 2 octaves loses its harmonic brightness entirely).

The formant metadata system tracks the "safe transposition range" per zone and stores formant
center frequencies so future tools (XOceanus runtime, iOS player) can apply formant correction.

### 6.2 Formant Metadata Struct

```cpp
struct FormantMetadata
{
    float formantF1Hz    = 0.0f;   // First formant center frequency (Hz)
    float formantF2Hz    = 0.0f;   // Second formant center frequency (Hz)
    float formantF3Hz    = 0.0f;   // Third formant center frequency (Hz, 0 = not detected)

    float safeTransposeSemitonesUp   = 12.0f;  // Max safe upward transposition
    float safeTransposeSemitonesDown = 12.0f;  // Max safe downward transposition

    bool  formantLock    = false;   // True = preserve formants during transposition
                                    // (requires XOceanus runtime — MPC standalone ignores)
    float formantShiftDb = 0.0f;    // Manual formant boost/cut at root note (dB)
};
```

### 6.3 Per-Category Safe Transposition Defaults

When no audio formant analysis is run, use these defaults:

| Category | safeUp | safeDown | formantLock default |
|----------|--------|---------|---------------------|
| Vocal | 3 | 5 | true |
| Choir | 4 | 6 | true |
| Brass | 7 | 9 | false |
| Wind | 6 | 8 | false |
| String | 9 | 12 | false |
| Keys | 12 | 12 | false |
| Organ | 24 | 24 | false |
| Pad | 24 | 24 | false |
| All drums | N/A | N/A | false (KeyTrack=False) |

### 6.4 Formant Detection Algorithm

When an audio analysis pass is run (triggered by `detectFormants=true` in `OutshineSettings`):

1. Load sample buffer, resample to 16 kHz mono for LPC analysis.
2. Apply 25ms Hamming window, 50% overlap.
3. Compute LPC coefficients (order 12 for speech-range, order 16 for instruments).
4. Find LPC roots, extract resonant peaks in 200–3500 Hz band.
5. Sort peaks by bandwidth: narrowest bandwidth = strongest formant.
6. Assign F1 (lowest-frequency strong peak), F2 (second), F3 (third).

If LPC fails or formants are below confidence threshold, set all `formantFNHz = 0.0f` and use
category defaults above.

### 6.5 XPM Annotation for Formants

MPC does not have native formant metadata fields. Store formant data as an XML comment block
in the XPM `<Keygroup>` element, readable by XOceanus runtime and future tools:

```xml
<Keygroup index="0">
  <LowNote>53</LowNote>
  <HighNote>65</HighNote>
  <!-- XO:Formant F1="720" F2="1240" F3="2800"
       SafeUp="3" SafeDown="5" Lock="true" -->
  <Layers>
    ...
  </Layers>
</Keygroup>
```

The `<!-- XO:Formant ... -->` comment format is the extension namespace. MPC firmware ignores
comments. XOceanus parser reads them via regex match on `XO:Formant`.

---

## 7. FX Routing Metadata

### 7.1 FX Architecture Context

XOceanus has a 25-stage MasterFX chain. When XOriginate exports a preset to XPN, it renders the
FX chain baked into the WAV files. XOutshine's use case is different: it processes **external
samples** that have no XOceanus FX chain. The FX routing metadata system defines which FX stages
(from the XOceanus MasterFX indices) should be applied in the enhancement pass when Outshine is
running inside the XOceanus desktop application.

**Two deployment contexts:**

| Context | FX Application |
|---------|---------------|
| Standalone xoutshine.py CLI | FX baked into WAV during ENHANCE (no runtime FX) |
| XOceanus desktop app | FX applied via MasterFXChain at playback time (not baked) |

### 7.2 FX Routing Metadata Struct

```cpp
struct FXStageParam
{
    juce::String paramId;   // MasterFX parameter ID (e.g., "aqua_reefDepth")
    float        value;     // 0.0–1.0 normalized
};

struct FXChainSnapshot
{
    int                       stageIndex;   // MasterFX stage index (0-based)
    juce::String              stageName;    // Human label (e.g., "Reef", "Fathom")
    bool                      enabled;
    std::vector<FXStageParam> params;
};

struct FXRoutingMetadata
{
    // Global routing: single FX chain applied to the entire program
    bool                       globalFX     = true;
    std::vector<FXChainSnapshot> globalChain;

    // Per-layer FX: override FX for specific velocity layers
    // Key = velLayer index (0-based), Value = FX chain for that layer
    std::map<int, std::vector<FXChainSnapshot>> perLayerChain;

    // When false, per-layer chains REPLACE global; when true, they stack after global
    bool perLayerStacksAfterGlobal = false;
};
```

### 7.3 MasterFX Stage Index Reference

Stages 0–21 are the original MasterFX chain. Stages 22–24 are the Singularity Collection.

| Index | Stage Name | Class | Use for |
|-------|-----------|-------|---------|
| 0 | Pre-Gain | — | Input level trim |
| 1 | Reef | AquaticFXSuite | Coral resonance, organic texture add |
| 2 | Fathom | AquaticFXSuite | Deep filter, pressure-style EQ |
| 3 | Drift | AquaticFXSuite | Gentle detune/chorus |
| 4 | Tide | AquaticFXSuite | Rhythmic modulation/tremolo |
| 5–7 | Entropy/Voronoi/Quantum | MathFXChain | Glitch, spatial, quantum |
| 8 | Attractor | MathFXChain | Chaotic modulation |
| 9–12 | Anomaly/Archive/Cathedral/Submersion | BoutiqueFXChain | Character FX |
| 13 | Compressor | — | Peak control |
| 14 | Multiband EQ | — | Tonal shaping |
| 15 | Stereo Width | — | Width control |
| 16 | Limiter | — | Ceiling protection |
| 17–21 | [Reserved] | — | Future use |
| 22 | fXOnslaught | fXOnslaught.h | Chorus → PM collapse |
| 23 | fXObscura | fXObscura.h | Chiaroscuro spectral degradation |
| 24 | fXOratory | fXOratory.h | Meter delay |

### 7.4 Category-to-FX Recommendations

Default FX chain snapshots per SampleCategory, applied during ENHANCE when running inside
XOceanus desktop (not baked in standalone CLI):

| Category | Recommended Stages | Rationale |
|----------|--------------------|-----------|
| Kick | 13 (Compressor), 16 (Limiter) | Control peaks, nothing added |
| Snare | 1 (Reef low), 13, 16 | Subtle crack enhancement |
| HiHatClosed | 15 (Stereo Width) | Slight widening |
| Pad | 2 (Fathom), 3 (Drift), 9 (Anomaly) | Depth and movement |
| Lead | 3 (Drift), 13 | Subtle detune, level control |
| Vocal | 4 (Tide), 15 | Gentle modulation, mono-to-width |
| Brass | 1 (Reef), 13 | Presence, dynamics |
| Texture | 7 (Quantum), 3 (Drift) | Space and movement |
| Keys | 3 (Drift subtle), 13 | Chorus shimmer, control |
| Organ | 3 (Drift), 4 (Tide subtle) | Leslie simulation approximation |
| Bell | 2 (Fathom), 12 (Submersion) | Tail extension, diffusion |

### 7.5 XPM Annotation for FX

Store FX routing as a comment block at the `<Program>` level:

```xml
<Program type="Keygroup">
  <ProgramName>Hybrid Coral Keys</ProgramName>
  <!-- XO:FXRouting global="true"
       stages="1,3,13,16"
       1_reef_depth="0.3" 1_reef_resonance="0.2"
       3_drift_rate="0.15" 3_drift_depth="0.1"
       13_comp_threshold="-12" 13_comp_ratio="3"
       16_limit_ceiling="-0.3" -->
  <Instruments>
    ...
  </Instruments>
</Program>
```

**Per-layer FX override annotation:**
```xml
<!-- XO:LayerFX layer="2" stages="22" 22_onslaught_depth="0.4" -->
```

---

## 8. Expression Mapping

### 8.1 Standard Expression Map

Every XOutshine program receives a default expression map based on category. This is embedded
directly in the XPM file.

```cpp
struct ExpressionMapping
{
    // Aftertouch
    juce::String atDestination   = "FilterCutoff";
    int          atAmount        = 30;    // -127 to +127

    // Mod Wheel
    juce::String mwDestination   = "FilterCutoff";
    int          mwAmount        = 70;

    // Pitch Bend
    int          pitchBendRange  = 12;    // semitones

    // Q-Link macro knobs (4 per program)
    struct QLink { juce::String destination; int amount; };
    QLink qlink[4];
};
```

**Per-category defaults:**

| Category | AT destination | AT amt | MW destination | MW amt | PB range | Q1 | Q2 | Q3 | Q4 |
|----------|--------------|--------|----------------|--------|---------|----|----|----|----|
| Drums | FilterCutoff | 30 | FilterCutoff | 20 | 2 | Tone | Attack | Tune | Pan |
| Bass | FilterCutoff | 50 | FilterCutoff | 80 | 12 | Tone | Filter | Drive | Space |
| Pad | FilterCutoff | 40 | VibratoDepth | 60 | 2 | Tone | Attack | Release | Space |
| Lead | FilterCutoff | 60 | VibratoDepth | 80 | 24 | Tone | Attack | Character | Filter |
| Keys | FilterCutoff | 30 | FilterCutoff | 50 | 12 | Tone | Attack | Release | Space |
| Organ | FilterCutoff | 20 | Volume | 60 | 2 | Tone | Drive | Tremolo | Space |
| Vocal | FilterCutoff | 35 | VibratoDepth | 70 | 7 | Vowel | Breath | Vibrato | Space |
| Pluck | FilterCutoff | 40 | VibratoDepth | 50 | 12 | Tone | Attack | Resonance | Space |

### 8.2 MPCe Quad-Corner Assignment Metadata

The MPCe 3D pads (MPCe = MPC expanded with XOceanus 3D pad surface) use a quad-corner assignment
system: each corner of the pad surface maps to a parameter. The DESIGN stage annotates programs
with the intended quad-corner assignments for the MPCe runtime.

```cpp
struct MPCeQuadCorner
{
    juce::String destination;  // Parameter name or XPM Q-Link label
    float        minValue;     // Value at corner rest (finger off corner)
    float        maxValue;     // Value at corner full press
    juce::String axis;         // "X", "Y", "Z", or "XY" (diagonal)
};

struct MPCeMapping
{
    MPCeQuadCorner corners[4];  // TL, TR, BL, BR
    juce::String   xAxisTarget; // XY pad X-axis destination
    juce::String   yAxisTarget; // XY pad Y-axis destination
};
```

**Default MPCe mapping per category:**

| Category | TL | TR | BL | BR | X-axis | Y-axis |
|----------|----|----|----|----|----|-----|
| Drums | Tune | Attack | Saturation | Pan | Filter | Volume |
| Bass | Filter | Drive | Sub | Presence | Filter | Volume |
| Pad | Filter | Release | Reverb | Width | Brightness | Depth |
| Lead | Filter | Attack | Drive | Detune | Brightness | Vibrato |
| Keys | Filter | Resonance | Reverb | Chorus | Brightness | Volume |
| Vocal | Vowel | Breath | Vibrato | Reverb | Brightness | Formant |

### 8.3 XY Axis Modulation

XPM `<QLinkAssignment>` blocks encode the four macro knobs. The MPCe XY map is stored as an
XO-namespace comment:

```xml
<QLinkAssignment>
  <QLink1>
    <Destination>FilterCutoff</Destination>
    <Amount>80</Amount>
    <Label>TONE</Label>
  </QLink1>
  <QLink2>
    <Destination>Attack</Destination>
    <Amount>60</Amount>
    <Label>ATTACK</Label>
  </QLink2>
  <QLink3>
    <Destination>Resonance</Destination>
    <Amount>50</Amount>
    <Label>BITE</Label>
  </QLink3>
  <QLink4>
    <Destination>Reverb</Destination>
    <Amount>70</Amount>
    <Label>SPACE</Label>
  </QLink4>
</QLinkAssignment>
<!-- XO:MPCe TL="FilterCutoff" TR="Attack" BL="Saturation" BR="Pan"
     X="FilterCutoff" Y="Volume" -->
```

---

## 9. The 9-Stage Pipeline

### 9.1 Pipeline Overview

```
INGEST → CLASSIFY → ANALYZE → DESIGN → ENHANCE → NORMALIZE → MAP → PACKAGE → VALIDATE
  1          2          3         4         5           6        7       8          9
```

The original 8-stage pipeline inserts **DESIGN** between ANALYZE and ENHANCE. The DESIGN stage
is where multi-source composite decisions are made — it has no audio processing side effects.
ENHANCE now receives DESIGN's blueprint and executes it.

### 9.2 Stage 1: INGEST

**Inputs:** `.xpn` archive | WAV folder | loose WAV files | `.xoforge` descriptor

**Outputs:** `std::vector<AnalyzedSample>` with `sourceFile` and `name` populated; metadata empty.

**Logic:**
- XPN: decompress ZIP, extract all `.wav` and `.aif` files to work directory
- WAV folder: recursive scan for `*.wav`, `*.aif`, `*.aiff`
- `.xoforge`: parse JSON descriptor → may specify exact sources with pre-assigned categories
- Deduplicate by content hash if input contains duplicates

**xoforge descriptor format (new in Forge v1):**
```json
{
  "packName": "Coral Keys",
  "sources": [
    { "file": "samples/pad_C3.wav", "category": "Pad", "rootNote": 60 },
    { "file": "samples/organ_C3.wav", "category": "Organ", "rootNote": 60 },
    { "file": "samples/brass_C3.wav", "category": "Brass", "rootNote": 60 }
  ],
  "design": {
    "strategy": "Crossfade",
    "velocityLayers": 3,
    "roundRobin": 2,
    "formantAnalysis": true,
    "fxCategory": "Keys"
  }
}
```

### 9.3 Stage 2: CLASSIFY

**Inputs:** `std::vector<AnalyzedSample>` with sourceFile/name

**Outputs:** `category` field populated on every sample

**Logic:** Three-pass heuristic (Section 2.2). Samples sourced from `.xoforge` descriptors skip
classification — their `category` is already set from the descriptor.

**Override mechanism:** If `category` is set to anything other than `Unknown` in the descriptor
or by a prior classify pass, the stage preserves it (no overwrite).

### 9.4 Stage 3: ANALYZE

**Inputs:** Classified samples

**Outputs:** All `AnalyzedSample` fields populated:
- `sampleRate`, `bitDepth`, `numChannels`, `numSamples`, `durationS`
- `rmsDb`, `peakDb`, `dcOffset`, `tailLengthS`
- `isLoopable`, `loopStart`, `loopEnd`
- `sonicDNA` (SonicDNA struct — if `analyzeDNA=true` in settings)
- `formantMetadata` (FormantMetadata — if `detectFormants=true` in settings)
- `detectedPitch` (float, Hz — fundamental frequency estimate for tonal categories)

**Pitch detection:** Applied to non-drum categories. Algorithm: autocorrelation in 50ms windows,
majority-vote over stable regions, reported as Hz. Used by DESIGN for root note confirmation.

### 9.5 Stage 4: DESIGN (New Stage)

**Inputs:** All analyzed samples

**Outputs:** `std::vector<UpgradedProgram>` with `keygroups`, `velocityStrategy`, `fxRouting`,
and `expression` populated. No WAV files written yet.

**This stage makes all creative decisions.** The ENHANCE stage executes them without deviation.

**DESIGN logic:**

1. **Group samples** by detected root note (for multi-note instruments) or treat all as single-zone.

2. **Select VelocityStrategy:**
   - If all samples in a group have the same `SampleCategory` → `Generate`
   - If samples span ≥ 2 categories → `Layer` (default) or `Crossfade` (if `crossfadeMode=true`)
   - If `.xoforge` descriptor specifies strategy → use that

3. **Build CompositeKeygroup layout:**
   - Single sample → 1 keygroup zone, full range, rootNote from analysis
   - Multi-note → N zones, boundaries computed by midpoint rule (Section 3.3)
   - Multi-source → assign sources to zones, assign velocity ranges per source

4. **Assign formant metadata** per zone from ANALYZE output or category defaults.

5. **Assign FX routing** per program from category recommendations (Section 7.4).

6. **Assign expression mapping** per program from category defaults (Section 8.1).

7. **Emit DESIGN log:** A JSON summary of all decisions for audit and reproducibility:
```json
{
  "programName": "Coral Keys",
  "strategy": "Crossfade",
  "zones": [
    {
      "low": 0, "high": 65, "root": 60,
      "sources": [
        { "file": "pad_C3.wav", "velRange": [1,60], "xfadeOut": [40,60] },
        { "file": "organ_C3.wav", "velRange": [40,127], "xfadeIn": [40,60] }
      ]
    }
  ],
  "formant": { "F1": 720, "F2": 1240, "safeUp": 3, "safeDown": 5 },
  "fxStages": [1, 3, 13, 16],
  "expression": { "AT": "FilterCutoff:40", "MW": "VibratoDepth:60" }
}
```

### 9.6 Stage 5: ENHANCE

**Inputs:** `UpgradedProgram` list from DESIGN

**Outputs:** WAV files on disk, `EnhancedLayer` lists populated in each program

**For GENERATE strategy:** Existing `XOutshine::enhance()` logic — amplitude + filter shaping +
micro-variation per velocity layer.

**For LAYER/SWITCH strategy:** Load each source WAV, apply per-layer gain and fade guards,
write N×M WAV files (N vel layers × M RR variants per source). No amplitude blending between sources.

**For CROSSFADE strategy:**
- Load source A and source B WAVs at full resolution.
- Generate baked crossfade WAVs for the overlap velocity region using equal-power crossfade.
- Write: source_A_full, crossfade_blend, source_B_full files.
- Apply RR micro-variations to each of the above.

**Common to all strategies:**
- DC offset removal
- Fade guards (2ms fade-in, 10ms fade-out or to natural tail)
- TPDF dither on bit-depth reduction to 16-bit
- File naming: `{SampleName}__{sourceTag}_v{velLayer}_c{rrIndex}.wav`

### 9.7 Stage 6: NORMALIZE

**Inputs:** Enhanced WAV files

**Outputs:** LUFS-adjusted WAV files (same paths, overwritten)

**Algorithm:**
1. Find the loudest layer (highest vel, first RR variant = c1) for the program.
2. Measure integrated LUFS (ITU-R BS.1770-4) of that reference layer.
3. Compute gain = `lufsTarget - measuredLufs` (clamped ±24 dB).
4. Apply same gain to ALL layers in the program (preserves inter-layer dynamics).
5. True-peak limit at -0.3 dBTP after gain application.

**LUFS targets by category:**

| Category | Target LUFS |
|----------|-------------|
| Drums | -14 LUFS |
| Bass | -16 LUFS |
| Pad, Texture | -18 LUFS |
| Lead, Keys | -14 LUFS |
| Vocal | -14 LUFS |
| Organ, String | -16 LUFS |

These defaults can be overridden via `OutshineSettings::lufsTarget`.

### 9.8 Stage 7: MAP

**Inputs:** Normalized WAV files + DESIGN blueprint

**Outputs:** XPM XML files

**Logic:**
- For drum programs: `buildDrumXPM()` — unchanged from existing implementation.
- For simple single-source keygroup: `buildKeygroupXPM()` — extended with formant comments,
  expression mapping, Q-Link assignments, and XO namespace metadata.
- For composite multi-source keygroup: `buildCompositeKeygroupXPM()` — new. Builds multiple
  `<Keygroup>` elements (one per zone), each with its `<Layers>` block reflecting all sources
  and RR variants. Includes formant and FX metadata comments.

**Critical rules enforced by MAP:**
1. `KeyTrack=True` on all keygroup layers (Rule #1).
2. `RootNote=0` on all layers (Rule #2 — MPC auto-detect).
3. `VelStart=0, VelEnd=0, Active=False` on all empty layer slots (Rule #3).
4. Empty layer padding: if total layers < 4, pad with empty layers to fill slot count.
5. CycleGroup numbering: one unique group per velocity tier (not per source).
6. Relative sample paths: `Samples/{ProgramName}/{filename}.wav`.

### 9.9 Stage 8: PACKAGE

**Inputs:** XPM files + WAV files in work directory

**Outputs:** `.xpn` ZIP archive or organized folder

**XPN structure:**
```
{PackName}.xpn (ZIP)
├── Expansions/
│   └── Manifest.xml        (name, author, version, program count)
├── Programs/
│   ├── Program1.xpm
│   └── Program2.xpm
└── Samples/
    ├── Program1/
    │   ├── Sample__pad_v1_c1.wav
    │   └── Sample__pad_v1_c2.wav
    └── Program2/
        └── ...
```

**Manifest.xml fields (extended from original):**
```xml
<Expansion>
  <Name>Coral Keys</Name>
  <Author>XOutshine by XO_OX Designs</Author>
  <Version>1.0</Version>
  <Description>Upgraded by XOutshine v1.0</Description>
  <ProgramCount>3</ProgramCount>
  <ForgeSpec>1.0</ForgeSpec>       <!-- New: spec version for forward-compat -->
  <Strategy>Crossfade</Strategy>   <!-- New: velocity strategy used -->
  <Categories>Keys,Organ,Pad</Categories>  <!-- New: source categories present -->
</Expansion>
```

### 9.10 Stage 9: VALIDATE

**Inputs:** Output XPN archive or folder

**Outputs:** Integer error count + `juce::StringArray errors_`

**Validation checklist:**

| Check | Severity | Rule |
|-------|---------|------|
| Manifest exists and has Name= | Error | Bible §1 |
| All `<File>` / `<SampleName>` paths resolve inside ZIP | Error | Bible §8 |
| No `VelStart > 0` on empty (no-file) layers | Error | Bible §5, Golden Rule #3 |
| No duplicate `SampleName` + `VelStart` + `VelEnd` combination | Error | Prevents ghost layers |
| `KeyTrack=True` on all keygroup layers | Error | Golden Rule #1 |
| `RootNote=0` on all layers | Warning | Golden Rule #2 |
| `Active=False` on empty instrument slots | Warning | Golden Rule #9 |
| CycleGroup values are unique per velocity tier | Warning | Section 5.4 |
| Total layers per zone ≤ 8 | Warning | Section 5.5 |
| WAV files are 16-bit or 24-bit, ≤ 48 kHz | Warning | Bible §8 |
| Individual WAV < 100 MB | Warning | Bible §10 |
| Boolean fields use `True`/`False` (not `true`/`1`) | Error | Bible §9 |
| XML is valid UTF-8 with proper entity escaping | Error | Bible §9 |
| Preview file exists and matches XPM basename | Info | Bible §11 |

---

## 10. XPM XML Structure Examples

### 10.1 Multi-Source Keygroup with Velocity Crossfade

This example shows a single zone (C0–G9) with a pad and organ source, crossfaded across vel 40–60.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<MPCVObject type="com.akaipro.mpc.keygroup.program">
  <Version>1.7</Version>
  <ProgramName>Coral Keys</ProgramName>

  <!-- Expression -->
  <AfterTouch>
    <Destination>FilterCutoff</Destination>
    <Amount>40</Amount>
  </AfterTouch>
  <ModWheel>
    <Destination>VibratoDepth</Destination>
    <Amount>60</Amount>
  </ModWheel>
  <PitchBendRange>12</PitchBendRange>

  <!-- Q-Link macro assignments -->
  <QLinkAssignment>
    <QLink1><Destination>FilterCutoff</Destination><Amount>80</Amount><Label>TONE</Label></QLink1>
    <QLink2><Destination>Attack</Destination><Amount>60</Amount><Label>ATTACK</Label></QLink2>
    <QLink3><Destination>Resonance</Destination><Amount>50</Amount><Label>BITE</Label></QLink3>
    <QLink4><Destination>Reverb</Destination><Amount>70</Amount><Label>SPACE</Label></QLink4>
  </QLinkAssignment>

  <!-- XO metadata (ignored by MPC firmware, read by XOceanus runtime) -->
  <!-- XO:FXRouting global="true" stages="1,3,13,16"
       1_reef_depth="0.30" 3_drift_rate="0.15" 3_drift_depth="0.10"
       13_comp_threshold="-12.0" 16_limit_ceiling="-0.30" -->
  <!-- XO:MPCe TL="FilterCutoff" TR="Attack" BL="Saturation" BR="Pan"
       X="FilterCutoff" Y="Volume" -->

  <Keygroups>
    <Keygroup index="0">
      <LowNote>0</LowNote>
      <HighNote>127</HighNote>

      <!-- XO:Formant F1="680" F2="1100" F3="0"
           SafeUp="12" SafeDown="12" Lock="false" -->

      <Layers>
        <!-- Layer 0: Pad source, vel 1–39 (full level, no crossfade) -->
        <Layer index="0">
          <SampleName>Samples/CoralKeys/CoralKeys__pad_v1_c1.wav</SampleName>
          <VelStart>1</VelStart>
          <VelEnd>39</VelEnd>
          <Volume>0.75</Volume>
          <RootNote>0</RootNote>
          <KeyTrack>True</KeyTrack>
          <TuneCoarse>0</TuneCoarse>
          <TuneFine>0</TuneFine>
          <LoopStart>-1</LoopStart>
          <LoopEnd>-1</LoopEnd>
          <Active>True</Active>
        </Layer>

        <!-- Layer 1: Crossfade blend (Pad fading out, Organ fading in), vel 40–60 -->
        <Layer index="1">
          <SampleName>Samples/CoralKeys/CoralKeys__xfade_v2_c1.wav</SampleName>
          <VelStart>40</VelStart>
          <VelEnd>60</VelEnd>
          <Volume>0.88</Volume>
          <RootNote>0</RootNote>
          <KeyTrack>True</KeyTrack>
          <TuneCoarse>0</TuneCoarse>
          <TuneFine>0</TuneFine>
          <LoopStart>-1</LoopStart>
          <LoopEnd>-1</LoopEnd>
          <Active>True</Active>
        </Layer>

        <!-- Layer 2: Organ source, vel 61–127 (full level) -->
        <Layer index="2">
          <SampleName>Samples/CoralKeys/CoralKeys__organ_v3_c1.wav</SampleName>
          <VelStart>61</VelStart>
          <VelEnd>127</VelEnd>
          <Volume>1.00</Volume>
          <RootNote>0</RootNote>
          <KeyTrack>True</KeyTrack>
          <TuneCoarse>0</TuneCoarse>
          <TuneFine>0</TuneFine>
          <LoopStart>-1</LoopStart>
          <LoopEnd>-1</LoopEnd>
          <Active>True</Active>
        </Layer>

        <!-- Layer 3: Empty slot — CRITICAL: VelStart=0 prevents ghost trigger -->
        <Layer index="3">
          <SampleName></SampleName>
          <VelStart>0</VelStart>
          <VelEnd>0</VelEnd>
          <Active>False</Active>
        </Layer>
      </Layers>
    </Keygroup>
  </Keygroups>
</MPCVObject>
```

### 10.2 Round-Robin Melodic Instrument (Piano, 4 vel layers × 2 RR)

```xml
<?xml version="1.0" encoding="UTF-8"?>
<MPCVObject type="com.akaipro.mpc.keygroup.program">
  <Version>1.7</Version>
  <ProgramName>Ivory Strike</ProgramName>

  <AfterTouch><Destination>FilterCutoff</Destination><Amount>30</Amount></AfterTouch>
  <ModWheel><Destination>FilterCutoff</Destination><Amount>50</Amount></ModWheel>
  <PitchBendRange>12</PitchBendRange>

  <Keygroups>
    <!-- Zone 1: C2–Eb2, root C2 (MIDI 36) -->
    <Keygroup index="0">
      <LowNote>24</LowNote>
      <HighNote>37</HighNote>

      <Layers>
        <!-- Vel layer 1 (ghost), RR variant 1 — ZonePlay=2 (cycle) -->
        <Layer index="0">
          <SampleName>Samples/IvoryStrike/IvoryStrike_C2__keys_v1_c1.wav</SampleName>
          <VelStart>1</VelStart><VelEnd>20</VelEnd>
          <Volume>0.30</Volume>
          <RootNote>0</RootNote><KeyTrack>True</KeyTrack>
          <CycleType>RoundRobin</CycleType><CycleGroup>1</CycleGroup>
          <ZonePlay>2</ZonePlay>
          <LoopStart>-1</LoopStart><LoopEnd>-1</LoopEnd>
          <Active>True</Active>
        </Layer>
        <!-- Vel layer 1, RR variant 2 -->
        <Layer index="1">
          <SampleName>Samples/IvoryStrike/IvoryStrike_C2__keys_v1_c2.wav</SampleName>
          <VelStart>1</VelStart><VelEnd>20</VelEnd>
          <Volume>0.30</Volume>
          <RootNote>0</RootNote><KeyTrack>True</KeyTrack>
          <CycleType>RoundRobin</CycleType><CycleGroup>1</CycleGroup>
          <ZonePlay>2</ZonePlay>
          <LoopStart>-1</LoopStart><LoopEnd>-1</LoopEnd>
          <Active>True</Active>
        </Layer>
        <!-- Vel layer 2 (soft), RR variant 1 — CycleGroup=2 -->
        <Layer index="2">
          <SampleName>Samples/IvoryStrike/IvoryStrike_C2__keys_v2_c1.wav</SampleName>
          <VelStart>21</VelStart><VelEnd>50</VelEnd>
          <Volume>0.55</Volume>
          <RootNote>0</RootNote><KeyTrack>True</KeyTrack>
          <CycleType>RoundRobin</CycleType><CycleGroup>2</CycleGroup>
          <ZonePlay>2</ZonePlay>
          <LoopStart>-1</LoopStart><LoopEnd>-1</LoopEnd>
          <Active>True</Active>
        </Layer>
        <!-- Vel layer 2, RR variant 2 — CycleGroup=2 -->
        <Layer index="3">
          <SampleName>Samples/IvoryStrike/IvoryStrike_C2__keys_v2_c2.wav</SampleName>
          <VelStart>21</VelStart><VelEnd>50</VelEnd>
          <Volume>0.55</Volume>
          <RootNote>0</RootNote><KeyTrack>True</KeyTrack>
          <CycleType>RoundRobin</CycleType><CycleGroup>2</CycleGroup>
          <ZonePlay>2</ZonePlay>
          <LoopStart>-1</LoopStart><LoopEnd>-1</LoopEnd>
          <Active>True</Active>
        </Layer>
        <!-- Vel layers 3+4 continue pattern — CycleGroup 3 and 4 respectively -->
        <!-- [layers 4–7 elided for brevity — follow identical pattern] -->
      </Layers>
    </Keygroup>

    <!-- Zone 2, 3, ... follow the same pattern for remaining root notes -->
  </Keygroups>
</MPCVObject>
```

### 10.3 FX Routing Metadata in Context

```xml
<?xml version="1.0" encoding="UTF-8"?>
<MPCVObject type="com.akaipro.mpc.keygroup.program">
  <Version>1.7</Version>
  <ProgramName>Vocal Drift</ProgramName>

  <!-- XO:FXRouting global="true"
       stages="4,15,13,16"
       4_tide_rate="0.08" 4_tide_depth="0.20"
       15_stereoWidth="0.65"
       13_comp_threshold="-10.0" 13_comp_ratio="4.0"
       16_limit_ceiling="-0.30" -->

  <!-- XO:LayerFX layer="3" stages="23"
       23_obscura_depth="0.35" 23_obscura_threshold="-18.0" -->

  <!-- XO:Formant F1="800" F2="1200" F3="2600"
       SafeUp="3" SafeDown="5" Lock="true" -->

  <!-- XO:MPCe TL="Vowel" TR="Breath" BL="Vibrato" BR="Reverb"
       X="FilterCutoff" Y="FormantShift" -->

  <Keygroups>
    <Keygroup index="0">
      <LowNote>0</LowNote>
      <HighNote>127</HighNote>
      <Layers>
        <!-- 3 velocity layers: soft vocal, med vocal, full + fXObscura treatment -->
        <Layer index="0">
          <SampleName>Samples/VocalDrift/VocalDrift__vox_v1_c1.wav</SampleName>
          <VelStart>1</VelStart><VelEnd>40</VelEnd>
          <Volume>0.40</Volume>
          <RootNote>0</RootNote><KeyTrack>True</KeyTrack>
          <Active>True</Active>
        </Layer>
        <Layer index="1">
          <SampleName>Samples/VocalDrift/VocalDrift__vox_v2_c1.wav</SampleName>
          <VelStart>41</VelStart><VelEnd>90</VelEnd>
          <Volume>0.70</Volume>
          <RootNote>0</RootNote><KeyTrack>True</KeyTrack>
          <Active>True</Active>
        </Layer>
        <Layer index="2">
          <!-- Hard layer: full voice — per-layer fXObscura applied at runtime -->
          <SampleName>Samples/VocalDrift/VocalDrift__vox_v3_c1.wav</SampleName>
          <VelStart>91</VelStart><VelEnd>127</VelEnd>
          <Volume>1.00</Volume>
          <RootNote>0</RootNote><KeyTrack>True</KeyTrack>
          <Active>True</Active>
        </Layer>
        <Layer index="3">
          <VelStart>0</VelStart><VelEnd>0</VelEnd>
          <Active>False</Active>
        </Layer>
      </Layers>
    </Keygroup>
  </Keygroups>
</MPCVObject>
```

---

## 11. MPC Compatibility Constraints

### 11.1 What MPC Hardware Natively Supports

These features work on MPC standalone with no XOceanus plugin required:

| Feature | Native MPC Support | Notes |
|---------|-------------------|-------|
| KeyTrack (pitch transpose) | YES | Firmware ≥ 2.0 |
| Velocity layers | YES | Up to 8 per zone (firmware 2.10+) |
| RoundRobin / Random / RandomNoRepeat | YES (CycleType) | Firmware ≥ 2.10 |
| MuteGroup (hat choke) | YES | All firmware versions |
| Loop points (LoopStart/LoopEnd) | YES | All firmware versions |
| Loop crossfade (LoopCrossfade) | YES | ms value in XPM |
| Aftertouch destination | YES | FilterCutoff, Volume, Pitch, Vibrato |
| Mod wheel destination | YES | Same as aftertouch |
| Pitch bend range | YES | Semitones |
| Q-Link assignments (QLinkAssignment) | YES | 4 macros per program |
| ZonePlay (velocity vs. cycle vs. random) | YES | Firmware ≥ 2.10 |
| CycleGroup | YES | Firmware ≥ 2.10 |
| Multi-keygroup zones (multiple Keygroup elements) | YES | Firmware ≥ 2.10 |

### 11.2 Features Requiring XOceanus Runtime

These features are stored as XO-namespace XML comments in XPM files. MPC firmware ignores them.
The XOceanus MPC desktop app or iOS player reads and activates them:

| Feature | XO Comment Tag | MPC Standalone Behavior |
|---------|---------------|------------------------|
| Formant lock / correction | `XO:Formant` | No formant correction — stretches naturally |
| FX routing (MasterFX stages) | `XO:FXRouting` | No FX applied — dry playback |
| Per-layer FX override | `XO:LayerFX` | No FX applied — dry playback |
| MPCe quad-corner assignment | `XO:MPCe` | Standard Q-Link behavior only |
| Formant shift (manual) | `XO:Formant formantShiftDb` | Ignored |

### 11.3 Graceful Degradation Guarantee

Every XOutshine program MUST sound acceptable on MPC standalone without XOceanus. The baked WAV
files carry all the musical value. XOceanus features are enhancements, not dependencies.

This means:
- All velocity layers must be musically coherent even without formant correction.
- All FX character (e.g., the organ's Leslie shimmer) must be baked into the WAVs if the program
  targets standalone use. When `bakeStandaloneFX=true` in settings, the ENHANCE stage processes
  samples through the recommended MasterFX stages before writing WAVs.
- Loop points must work without LoopCrossfade if firmware doesn't support it.

### 11.4 Non-Negotiable Golden Rules (Inherited from XPN Bible)

These 12 rules have no exceptions. Violating any one will cause silence, ghost triggers, or
load failure on MPC hardware:

```
1.  KeyTrack = True          — Always on keygroup layers. KeyTrack = False on drum layers.
2.  RootNote = 0             — Always. MPC filename-based auto-detect convention.
3.  VelStart = 0             — On empty velocity layer slots (pairs with VelEnd=0 + Active=False).
4.  Preview filename match   — MyProgram.xpm → MyProgram.mp3 (exact basename, audio extension).
5.  Relative paths only      — Never absolute. Always Samples/{ProgramName}/filename.wav.
6.  XPN = ZIP structure      — Samples/, Programs/, Expansions/manifest. Exact case.
7.  KeyTrack = False         — On drum program layers (opposite of rule 1).
8.  OneShot = True           — On drum/percussion pad layers.
9.  Active = False           — On empty instrument/layer slots.
10. VelEnd = 0               — Pairs with VelStart=0 on empty slots.
11. Escape XML               — & → &amp;, < → &lt; in all string values.
12. Boolean casing           — True/False (capital T/F). Not true, not 1/0.
```

### 11.5 Forge-Specific Additional Constraints

Rules specific to the expanded Forge pipeline:

```
13. CycleGroup = velLayerIndex + 1   — Groups are 1-based, unique per velocity tier.
14. Max 8 layers per keygroup zone   — Hard limit on MPC Key 37/61/One/Live/X.
15. Max 4 layers per zone (safe)     — Use 4 if targeting unknown firmware versions.
16. Crossfade WAVs are baked         — No runtime crossfade assumed. Bake it in ENHANCE.
17. VelStart=1 minimum on active     — Never VelStart=0 on any active (non-empty) layer.
18. Formant metadata in comments     — Use XO:Formant namespace. Never invent new XML elements.
19. FX metadata in comments          — Use XO:FXRouting namespace. Same reason.
20. LUFS normalization is per-program — Apply identical gain offset to all layers in a program.
```

---

## 12. Rex's Extended Golden Rules

The original 12 golden rules (XPN Bible §12) apply unchanged. The following extend them for
the Forge's universal instrument scope:

```
Extended Rules for Melodic / Composite Programs:

F1. Multi-source programs must sound good on MPC standalone (baked WAVs carry the value).
F2. DESIGN stage makes all creative decisions — ENHANCE executes them without deviation.
F3. Equal-power crossfade (cos²/sin² curves) for velocity blends — NOT linear cross-fade.
F4. CycleGroup must be unique per velocity tier across ALL sources in a zone.
F5. Formant-sensitive categories (Vocal, Brass, Wind) always get XO:Formant annotations.
F6. GENERATE strategy micro-pitch ≤ ±5 cents (drums), ≤ ±3 cents (melodic) — intonation preservation.
F7. LUFS normalization applies identical gain to all velocity layers in a program.
F8. xoforge descriptor values override all classification and analysis defaults.
F9. The DESIGN log (JSON) must be written for every export — required for QA reproducibility.
F10. Safe transposition defaults (Section 6.3) apply whenever formant analysis is skipped.
```

---

*XOutshine Sample Instrument Forge Format Specification v1.0*
*Rex — XPN/XPM Format Bible Keeper, XO_OX Kai Android Team*
*2026-03-22*
