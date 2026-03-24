# XOlokun — Sound Shape XPN Rendering Specification

**Version:** 1.0
**Author:** XO_OX Designs
**Date:** 2026-03-11
**Status:** Specification Complete
**Depends on:** `xo_mega_tool_xpn_export.md`, `xo_organon_phase1_architecture.md`
**Extends:** Section 4 (Rendering Pipeline) of the XPN Export spec

---

## 1. Problem Statement

The XPN export pipeline currently treats all presets identically: same render length, same velocity layers, same keygroup structure. But a percussive pluck, a 10-second evolving pad, and a tempo-locked rhythmic texture have fundamentally different requirements for faithful MPC playback.

Additionally, the MPC imposes a hard constraint: **`ZonePlay` is per-keygroup, and velocity switching (`1`) and round robin/cycle (`0`) are mutually exclusive.** A single keygroup cannot do both.

Sound Shapes solve this by classifying each preset's temporal and timbral behavior, then applying shape-specific rendering and keygroup strategies that maximize fidelity within MPC's constraints.

---

## 2. Sound Shape Taxonomy

Every XOlokun preset is assigned one of six Sound Shapes. The shape is stored in the `.xometa` file as `"soundShape"` and inferred automatically if absent.

| Shape | Character | Typical Duration | Engines |
|-------|-----------|-----------------|---------|
| **Transient** | Percussive attack, rapid decay | < 2s | ODDFELIX, ONSET, OBLONG (pluck patches) |
| **Sustained** | Stable tone with clear attack/sustain/release | 2–6s | OBLONG, OBESE, ODDOSCAR, ONSET (tonal) |
| **Evolving** | Timbral movement over time, no stable sustain | 4–15s | ODYSSEY, ORGANON, ODDOSCAR (wavetable sweeps) |
| **Bass** | Low-frequency focus, tight response | 1–4s | OBESE, OVERDUB, OBLONG (sub patches) |
| **Texture** | Ambient, non-melodic, atmospheric | 6–20s | OVERWORLD, OPAL, ORGANON (atmospheric) |
| **Rhythmic** | Tempo-locked pulsing or sequenced | 2–8 bars | ONSET, OVERDUB (delay rhythms), ORGANON (lock-in) |

### 2.1 Shape Assignment

**Explicit:** Preset authors set `"soundShape": "Evolving"` in the `.xometa` file.

**Inferred (fallback):** When `soundShape` is absent, the export pipeline infers it from DNA + engine + tags:

```python
def infer_sound_shape(preset):
    tags = set(preset.get("tags", []))
    dna = preset["dna"]
    engines = preset["engines"]

    # Tag-based shortcuts
    if tags & {"drum", "kit", "percussion"}:
        return "Transient"
    if tags & {"bass", "sub", "808"}:
        return "Bass"
    if tags & {"ambient", "texture", "atmosphere", "drone"}:
        return "Texture"
    if tags & {"arp", "sequence", "rhythmic", "pulse"}:
        return "Rhythmic"

    # DNA-based inference
    if dna["movement"] > 0.7 and dna["density"] > 0.5:
        return "Evolving"
    if dna["aggression"] > 0.6 and dna["movement"] < 0.3:
        return "Transient"
    if dna["warmth"] > 0.6 and dna["brightness"] < 0.3:
        return "Bass"
    if dna["space"] > 0.7 and dna["movement"] > 0.5:
        return "Texture"

    # Engine-based fallback
    if "XOnset" in engines:
        return "Transient"
    if "XObese" in engines or "XOverdub" in engines:
        return "Bass"
    if "XOdyssey" in engines or "XOrganon" in engines:
        return "Evolving"
    if "XOverworld" in engines or "XOpal" in engines:
        return "Texture"

    return "Sustained"  # safe default
```

---

## 3. Render Settings Per Shape

Each shape overrides the default render settings from the XPN Export spec (Section 4.2).

| Setting | Transient | Sustained | Evolving | Bass | Texture | Rhythmic |
|---------|-----------|-----------|----------|------|---------|----------|
| **Render length** | 2s | 4s | 8s | 3s | 10s | 8 bars¹ |
| **Tail capture** | 1s | 2s | 3s | 1.5s | 4s | 2s |
| **Note range** | C2–C5 m3 | C1–C6 m3 | C2–C5 m3 | C0–C3 m3 | C2–C4 m3 | C2–C4 m3 |
| **Note count** | 13 | 21 | 13 | 13 | 9 | 9 |
| **Velocity layers** | 3 | 2 | 1 | 2 | 1 | 1 |
| **ZonePlay mode** | 1 (Velocity) | 1 (Velocity) | 0 (Cycle)² | 1 (Velocity) | 2 (Random) | 1 (Velocity) |
| **Loop mode** | Off | Off | One-shot³ | Off | One-shot³ | Off |
| **Normalization** | -0.3 dB | -0.3 dB | -1.0 dB | -0.3 dB | -3.0 dB | -0.3 dB |
| **Est. WAVs/preset** | 39 | 42 | 26⁴ | 26 | 9 | 9 |
| **Est. size/preset** | ~2 MB | ~10 MB | ~25 MB | ~5 MB | ~22 MB | ~9 MB |

¹ Rhythmic presets render at the preset's stored `tempo` (or 120 BPM default). Length = 8 bars at that tempo.
² Evolving presets use Cycle mode with 2 round-robin variations to prevent identical repetitions.
³ One-shot mode (no MPC loop). Long samples play fully on each trigger. See Section 5 for the looping decision.
⁴ 13 notes × 2 round-robin variations = 26 WAVs.

### 3.1 Why These Choices

**Transient — 3 velocity layers, no RR:**
Velocity IS the sound for percussive patches. A soft pluck and a hard snap are acoustically different timbres, not just volume changes. The attack transient shape, harmonic content, and noise burst all change with velocity. Round robin matters less because transient sounds are short enough that repetition is less fatiguing than with sustained tones.

**Sustained — 2 velocity layers:**
Expression through dynamics. Soft notes have different filter/resonance behavior than hard ones. Two layers (soft/hard split at velocity 80) capture the essential dynamic range without excessive file size.

**Evolving — 2 RR variations, no velocity layers:**
The defining feature of evolving sounds is that they change over time. Velocity barely matters — the metabolic bloom, wavetable sweep, or filter journey is the sound. But playing the same 8-second sample twice in a row is immediately obvious. Two round-robin variations (rendered with different random seeds or slight parameter offsets) prevent this.

**Bass — 2 velocity layers, narrow range:**
Bass occupies C0–C3. Above that it's no longer bass. Velocity matters because sub-bass response differs at soft vs hard velocities (filter opens, saturation engages). No round robin needed — repetition in bass is expected (it's a groove element).

**Texture — Random layer selection, 1 layer:**
Textures are atmospheric; velocity response is irrelevant. Random playback (`ZonePlay=2`) would be ideal for variety, but with only 1 layer there's nothing to randomize. The single layer keeps file sizes reasonable for these long samples. ZonePlay=2 is set for future-proofing if the user adds layers manually on the MPC.

**Rhythmic — single velocity, tempo-rendered:**
The rhythm IS the preset. Velocity layers would duplicate the rhythmic pattern at different volumes, which is wasteful. The rendered audio already contains the dynamic contour of the sequence/arp.

---

## 4. The Round Robin vs Velocity Decision

MPC imposes a hard constraint: **within a single keygroup (Instrument), `ZonePlay` selects exactly one mode.** You cannot combine velocity switching with round robin in the same zone.

### 4.1 Decision Matrix

| Priority | Sound Shape | ZonePlay | Rationale |
|----------|------------|----------|-----------|
| Velocity > RR | Transient | `1` (Velocity) | Dynamics define percussive character |
| Velocity > RR | Sustained | `1` (Velocity) | Expression matters for melodic playing |
| RR > Velocity | Evolving | `0` (Cycle) | Avoiding repetition matters more than dynamics |
| Velocity > RR | Bass | `1` (Velocity) | Punch vs soft sub is the difference |
| RR > Velocity | Texture | `2` (Random) | No velocity response; randomization prevents same-loop fatigue |
| Velocity only | Rhythmic | `1` (Velocity) | Tempo-locked patterns have baked-in dynamics |

### 4.2 Overlapping Keygroup Technique (Advanced)

For presets that truly need BOTH velocity switching AND round robin, use overlapping keygroups:

```
Keygroup A (C2–C2): ZonePlay=0 (Cycle), Layers 1-2 = RR variation A/B
                     VelStart=80, VelEnd=127  (hard hits only)

Keygroup B (C2–C2): ZonePlay=0 (Cycle), Layers 1-2 = RR variation A/B
                     VelStart=1, VelEnd=79   (soft hits only)
```

This gives: 2 velocity bands × 2 round robins = 4 distinct samples per note.

**Cost:** Doubles keygroup count per note. For 13 notes, that's 26 keygroups (of 128 max).

**When to use:** Only for Transient presets flagged `"expressiveRR": true` in `.xometa`. This is opt-in because the file size and complexity cost is significant.

---

## 5. The Looping Decision

MPC keygroup programs support sample looping (`LoopOnOff`, `LoopStart`, `LoopEnd`, `LoopCrossfade`). The question: should XOlokun-rendered WAVs use MPC looping for sustaining sounds?

### 5.1 Decision: One-Shot for All Shapes

**All Sound Shapes use one-shot playback (no MPC loop).** The XPM sets `LoopOnOff=False` for every layer.

**Rationale:**

1. **Evolving sounds are the hardest to loop.** MPC's loop tools are basic — no Kontakt-style crossfade engine. Finding a loop point in an 8-second timbral journey that doesn't produce an audible click or timbral reset is extremely difficult. The evolution IS the sound; looping defeats the purpose.

2. **Sustained sounds bake in the sustain.** A 4-second render at the preset's sustain level captures the sustain character. If the MPC user holds a note longer than 4 seconds, it fades to silence — which is acceptable because MPC is a sampler, not a synth. Users expect sample-length limits.

3. **Organon presets especially resist looping.** The metabolic bloom, VFE adaptation, and free energy accumulation create unique timbral trajectories that never repeat exactly. Any loop point would sound artificial.

4. **Consistency.** One-shot across all shapes means the export pipeline doesn't need per-preset loop-point detection, crossfade tuning, or loop quality validation.

5. **User can add loops on MPC.** Advanced MPC users can manually set loop points in Program Edit if they want sustaining behavior. Our job is to deliver the highest-fidelity capture of the preset's character.

### 5.2 Render Length Compensates

Since we don't loop, render lengths are generous:

| Shape | Render + Tail | Total Sample Length | Why |
|-------|---------------|--------------------|----|
| Transient | 2s + 1s | 3s | More than enough for any percussive decay |
| Sustained | 4s + 2s | 6s | Covers a full musical phrase |
| Evolving | 8s + 3s | 11s | Captures the full timbral journey |
| Bass | 3s + 1.5s | 4.5s | Bass notes sustain but don't evolve |
| Texture | 10s + 4s | 14s | Long enough for atmospheric use |
| Rhythmic | 8 bars + 2s | ~18s at 120 BPM | Full rhythmic phrase with tail |

### 5.3 Exception: Sustain-Critical Bass

For bass presets tagged `"sustainLoop": true`, the pipeline renders a longer sample (8s) and attempts automatic loop-point detection:

1. Analyze the last 4 seconds for a stable amplitude/spectral region
2. Find zero-crossing pairs with minimal spectral difference (FFT comparison)
3. If a valid loop region is found (spectral similarity > 0.95), set `LoopOnOff=True` with `LoopCrossfade=50`ms
4. If no valid loop found, fall back to one-shot

This is opt-in and rare. Most bass presets work fine as one-shot.

---

## 6. Organon-Specific Rendering Considerations

Organon's metabolic synthesis creates unique challenges for sample rendering.

### 6.1 Bloom Time

Organon doesn't produce sound on note-on — it must metabolize input before harmonics emerge. The **bloom time** varies by preset:

| Archetype | Metabolic Rate | Approx. Bloom Time | Render Strategy |
|-----------|---------------|--------------------|----|
| Warm Start | 0.5 Hz | ~0.8s | Standard — bloom completes within render |
| Hyper-Metabolic Lead | 8.0 Hz | ~0.1s | Fast — essentially instant |
| Cellular Bloom | 0.1 Hz | ~4s | Extended — render length must exceed bloom time |
| Hibernation Cycle | Coupling-dependent | Variable | Render with internal noise at medium flux |

**Rule:** Render length must be at least `3 / metabolicRate` seconds to capture the full bloom. The pipeline calculates this per-preset:

```python
def organon_render_length(preset):
    met_rate = preset["parameters"]["XOrganon"]["organon_metabolicRate"]
    bloom_time = 3.0 / met_rate
    base_length = SHAPE_DEFAULTS[preset["soundShape"]]["render_length"]
    return max(base_length, bloom_time + 2.0)  # bloom + 2s sustain
```

### 6.2 Standalone vs Coupled Rendering

Organon presets that require coupling input (`"couplingIntensity": "High"`) pose a problem: the coupled sound can't be reproduced without the source engine.

**Strategy:**
- **Standalone presets** (`couplingIntensity` = "None"): Render normally. Internal noise substrate feeds the organism.
- **Coupled presets** (`couplingIntensity` = "Medium"/"High"): Render through the full coupling chain (per existing XPN Export spec §4.3). The coupled sound is baked in.
- **Coupling-dependent presets** (e.g., Hibernation Cycle with near-zero standalone output): Render with `signalFlux` temporarily boosted to 0.6 so the internal noise substrate produces audible output. Flag these in the XPN manifest as `"couplingNote": "Best with external input"`.

### 6.3 VFE Non-Determinism

Organon's Active Inference means two renders of the same preset produce slightly different results (the VFE adaptation path varies with numerical precision). This is actually beneficial for round-robin variations:

**For Evolving-shape Organon presets:** The two round-robin layers are simply two renders of the same preset. The VFE non-determinism naturally produces different timbral journeys, providing organic variation without any parameter changes.

### 6.4 Organon Sound Shape Distribution

Based on the 7 preset archetypes and 120 factory presets:

| Sound Shape | Organon Preset Count | Notes |
|-------------|---------------------|-------|
| Evolving | ~55 | The primary shape — metabolic bloom IS evolution |
| Sustained | ~25 | Faster metabolic rates that reach stable state quickly |
| Texture | ~20 | Atmospheric, low-catalyst, high-space presets |
| Bass | ~10 | Entropy Sink archetype — sub-focused organisms |
| Rhythmic | ~10 | Pace-of-Life archetype — tempo-locked pulsing |
| Transient | ~0 | Organon doesn't do transients (bloom time prevents it) |

---

## 7. XPM Generation Per Shape

### 7.1 Transient Shape XPM

```xml
<Instrument number="0">
  <!-- C2 zone: 3 velocity layers -->
  <LowNote>36</LowNote>
  <HighNote>38</HighNote>
  <ZonePlay>1</ZonePlay>  <!-- Velocity switching -->
  <Layers>
    <Layer number="0">
      <SampleName>Preset__C2__v3.WAV</SampleName>
      <RootNote>0</RootNote>
      <KeyTrack>True</KeyTrack>
      <VelStart>100</VelStart>
      <VelEnd>127</VelEnd>
    </Layer>
    <Layer number="1">
      <SampleName>Preset__C2__v2.WAV</SampleName>
      <RootNote>0</RootNote>
      <KeyTrack>True</KeyTrack>
      <VelStart>50</VelStart>
      <VelEnd>99</VelEnd>
    </Layer>
    <Layer number="2">
      <SampleName>Preset__C2__v1.WAV</SampleName>
      <RootNote>0</RootNote>
      <KeyTrack>True</KeyTrack>
      <VelStart>1</VelStart>
      <VelEnd>49</VelEnd>
    </Layer>
    <Layer number="3">
      <SampleName></SampleName>
      <VelStart>0</VelStart>  <!-- CRITICAL: empty layer = 0 -->
      <VelEnd>0</VelEnd>
    </Layer>
  </Layers>
  <AmpAttack>0</AmpAttack>
  <AmpDecay>0</AmpDecay>
  <AmpSustain>1.0</AmpSustain>
  <AmpRelease>0.1</AmpRelease>  <!-- Short release for transients -->
</Instrument>
```

### 7.2 Evolving Shape XPM (Round Robin)

```xml
<Instrument number="0">
  <!-- C2 zone: 2 round-robin layers -->
  <LowNote>36</LowNote>
  <HighNote>38</HighNote>
  <ZonePlay>0</ZonePlay>  <!-- Cycle (round robin) -->
  <Layers>
    <Layer number="0">
      <SampleName>Preset__C2__rr1.WAV</SampleName>
      <RootNote>0</RootNote>
      <KeyTrack>True</KeyTrack>
      <VelStart>1</VelStart>
      <VelEnd>127</VelEnd>
    </Layer>
    <Layer number="1">
      <SampleName>Preset__C2__rr2.WAV</SampleName>
      <RootNote>0</RootNote>
      <KeyTrack>True</KeyTrack>
      <VelStart>1</VelStart>
      <VelEnd>127</VelEnd>
    </Layer>
    <Layer number="2">
      <SampleName></SampleName>
      <VelStart>0</VelStart>
      <VelEnd>0</VelEnd>
    </Layer>
    <Layer number="3">
      <SampleName></SampleName>
      <VelStart>0</VelStart>
      <VelEnd>0</VelEnd>
    </Layer>
  </Layers>
  <AmpAttack>0</AmpAttack>
  <AmpDecay>0</AmpDecay>
  <AmpSustain>1.0</AmpSustain>
  <AmpRelease>0.5</AmpRelease>  <!-- Longer release for pads -->
</Instrument>
```

### 7.3 WAV Naming Extension

Round-robin files use `__rr{N}` instead of `__v{N}`:

| Shape | Naming Pattern | Example |
|-------|---------------|---------|
| Transient | `{NAME}__{NOTE}__v{1-3}.WAV` | `Hard_Pluck__C2__v3.WAV` |
| Sustained | `{NAME}__{NOTE}__v{1-2}.WAV` | `Warm_Pad__C2__v2.WAV` |
| Evolving | `{NAME}__{NOTE}__rr{1-2}.WAV` | `Cellular_Bloom__C2__rr1.WAV` |
| Bass | `{NAME}__{NOTE}__v{1-2}.WAV` | `Sub_Pressure__C1__v1.WAV` |
| Texture | `{NAME}__{NOTE}.WAV` | `Deep_Fog__C2.WAV` |
| Rhythmic | `{NAME}__{NOTE}.WAV` | `Pulse_Grid__C2.WAV` |

Texture and Rhythmic shapes have no velocity or RR suffix (single layer).

### 7.4 Velocity Split Points

| Layers | Split | Velocity Ranges |
|--------|-------|----------------|
| 1 | None | v1: 1–127 |
| 2 | Even | v1 (soft): 1–79, v2 (hard): 80–127 |
| 3 | Thirds | v1 (soft): 1–49, v2 (medium): 50–99, v3 (hard): 100–127 |

Render velocities (MIDI velocity sent during rendering):

| Layer | MIDI Velocity |
|-------|--------------|
| v1 (soft) | 40 |
| v2 (medium) | 85 |
| v3 (hard) | 120 |

---

## 8. Size Impact Analysis

### 8.1 Per-Preset Size Estimates

| Shape | Notes | Layers | Render+Tail | WAV Size/ea | WAVs | Total |
|-------|-------|--------|-------------|-------------|------|-------|
| Transient | 13 | 3 vel | 3s | ~130 KB | 39 | ~5 MB |
| Sustained | 21 | 2 vel | 6s | ~260 KB | 42 | ~11 MB |
| Evolving | 13 | 2 RR | 11s | ~480 KB | 26 | ~12 MB |
| Bass | 13 | 2 vel | 4.5s | ~195 KB | 26 | ~5 MB |
| Texture | 9 | 1 | 14s | ~610 KB | 9 | ~5.5 MB |
| Rhythmic | 9 | 1 | 18s | ~780 KB | 9 | ~7 MB |

Estimates assume 44.1kHz, 24-bit, mono. Actual sizes vary with silence trimming.

### 8.2 Full Library Estimate (1000 presets + 120 Organon)

Assuming shape distribution across the full library:

| Shape | Preset Count | Avg Size | Subtotal |
|-------|-------------|----------|----------|
| Transient | ~180 | 5 MB | ~900 MB |
| Sustained | ~300 | 11 MB | ~3.3 GB |
| Evolving | ~200 | 12 MB | ~2.4 GB |
| Bass | ~150 | 5 MB | ~750 MB |
| Texture | ~100 | 5.5 MB | ~550 MB |
| Rhythmic | ~70 | 7 MB | ~490 MB |
| **Total** | **~1000** | | **~8.4 GB** |

This is larger than the original 5 GB estimate because shape-aware rendering uses longer samples and more layers where they matter. The tradeoff is worthwhile — the MPC user gets dramatically better fidelity.

### 8.3 Size Optimization Options

If the full library is too large:

| Strategy | Savings | Impact |
|----------|---------|--------|
| Drop Evolving RR to 1 layer | -1.2 GB | Repetition becomes audible |
| Reduce Sustained to 1 vel layer | -1.65 GB | Less expressive |
| Reduce Texture render to 8s | -200 MB | Shorter atmospheric samples |
| Use 16-bit for Texture/Rhythmic | -500 MB | Minimal quality loss |
| Widen note spacing to P4 for Texture | -200 MB | More pitch-shifting artifacts |

**Recommended budget strategy:** Ship the "Complete" bundle with all shapes at full quality (~8.4 GB). Per-mood bundles are much smaller (~500 MB–1.5 GB each).

---

## 9. .xometa Schema Extension

Add `soundShape` and optional `expressiveRR` fields:

```json
{
  "schema_version": 1,
  "name": "Cellular Bloom",
  "mood": "Aether",
  "engines": ["XOrganon"],
  "soundShape": "Evolving",
  "expressiveRR": false,
  "parameters": { ... },
  "dna": { ... }
}
```

| Field | Type | Required | Values |
|-------|------|----------|--------|
| `soundShape` | string | No (inferred if absent) | `"Transient"`, `"Sustained"`, `"Evolving"`, `"Bass"`, `"Texture"`, `"Rhythmic"` |
| `expressiveRR` | bool | No (default false) | `true` enables overlapping-keygroup technique for combined velocity + RR |

---

## 10. Export Pipeline Integration

### 10.1 Updated Rendering Step

The pipeline from XPN Export spec §4.1 becomes:

```
Step 1: Load .xometa preset
        ↓
Step 2: Determine Sound Shape (explicit or inferred)
        ↓
Step 3: Apply shape-specific render settings (this spec, §3)
        ↓
Step 4: For Organon presets: calculate bloom-adjusted render length (§6.1)
        ↓
Step 5: For each note in shape's range:
        ↓ For each velocity layer OR round-robin variation (per shape):
        ↓   Send MIDI noteOn
        ↓   Render to float buffer (shape-specific length)
        ↓   Capture noteOff + tail
        ↓   Trim silence, normalize (shape-specific ceiling)
        ↓   Save WAV (shape-specific naming)
Step 6: Build XPM with shape-specific ZonePlay mode
        ↓ Apply the 3 critical rules
Step 7: Build XPN manifest, package
```

### 10.2 RenderSettings Extension

```cpp
struct RenderSettings {
    // Existing fields (from XPN Export spec)
    double sampleRate = 44100.0;
    int bitDepth = 24;
    NoteSelection noteSelection = NoteSelection::EveryMinor3rd;
    int velocityLayers = 1;
    float renderLengthSeconds = 4.0f;
    float tailSeconds = 2.0f;
    float normalizationDb = -0.3f;

    // Sound Shape extensions
    SoundShape soundShape = SoundShape::Sustained;
    int roundRobinLayers = 1;
    ZonePlayMode zonePlayMode = ZonePlayMode::Velocity;
    bool enableSustainLoop = false;
    bool expressiveRR = false;   // overlapping keygroup technique

    // Apply shape defaults
    static RenderSettings forShape(SoundShape shape);
};

enum class SoundShape {
    Transient,
    Sustained,
    Evolving,
    Bass,
    Texture,
    Rhythmic
};

enum class ZonePlayMode {
    Cycle = 0,    // Round robin
    Velocity = 1, // Velocity switching
    Random = 2    // Random layer selection
};
```

### 10.3 Validation Additions

Add to XPN Export spec §10.1 (Post-Export Checks):

| Check | Method | Pass Criteria |
|-------|--------|--------------|
| Sound Shape consistency | Verify ZonePlay matches shape | Evolving → Cycle, Transient → Velocity, etc. |
| Bloom coverage | For Organon: check WAV length ≥ bloom time | No silent/near-silent exports |
| RR variation | For Cycle mode: compare RR1 vs RR2 spectral content | Layers are meaningfully different (> 5% spectral deviation) |
| Velocity layer ordering | For Velocity mode: v1 RMS < v2 RMS < v3 RMS | Louder layers are louder |

---

## 11. ONSET Drum Kits (Unchanged)

ONSET drum kit export (XPN Export spec §5) is **not affected** by Sound Shapes. Drum kits already have their own dedicated rendering pipeline with per-voice velocity layers and GM pad mapping. The `soundShape` field is ignored for presets with `"engines": ["XOnset"]` that are tagged as drum kits.

---

*CONFIDENTIAL — XO_OX Internal Design Document*
