# XPN Format Innovations — Beyond Standard Pack Design
**Authors:** Hex + Rex (XO_OX hacker/bridge androids)
**Date:** 2026-03-16
**Status:** R&D — implementable with current Oxport toolchain

This document catalogs seven structural innovations that reframe what an XPN expansion pack can be. Each one treats the XPN/XPM format as a compositional medium rather than a delivery container. None require changes to MPC firmware — all exploits are legal moves within the existing schema.

---

## Status Markers

| Marker | Meaning |
|--------|---------|
| **[CONFIRMED]** | Verified MPC behavior, used in shipping tools |
| **[PLAUSIBLE]** | Logical inference from known schema; likely correct |
| **[PROPOSED]** | Our own design spec; no prior art |
| **[SPECULATIVE]** | Hypothesis requiring hardware test |

---

## Innovation 1: Polyrhythmic Pad Banks

### Concept

A single XPN pack where the four pad banks (A/B/C/D) contain identical sounds re-articulated into different implied meters. The sounds do not change — the rhythmic mode does. Bank A plays the kit in 4/4 feel. Bank B plays it in 6/8. Bank C plays it in 5/4. Bank D plays it in 7/8. The producer switches banks to modulate the underlying pulse without reloading any program.

This is not a sequencer feature. The "feel" lives in velocity layering, sample start-point trimming, and pad-note routing — structural choices baked into each bank's XPM at export time.

### How Meter Lives in an XPM

An XPM cannot store a time signature — that belongs to the sequence. But it can encode rhythmic character via:

1. **Sample start offsets** — trimming the attack slightly on weaker subdivisions simulates metrical softening. A snare rendered with 8 ms pre-delay "feels" like beat 3 of a triplet group vs. a snare with 0 ms pre-delay that snaps.
2. **Velocity architecture per voice** — in 6/8 feel, hits on positions 1 and 4 of a 6-subdivision bar are accented (higher velocity ceiling for v4 layer). Positions 2, 3, 5, 6 are softer. If the producer step-sequences equal-velocity hits, the sample amplitude hierarchy creates the implied feel.
3. **Pitch microtuning per bank** — a slight pitch raise on accent positions reinforces the metrical hierarchy perceptually. 5–10 cents is inaudible as pitch but effective as perceived emphasis.
4. **Transient shaping** — kit_expander.py's v4 transform (transient sharpening) applied selectively to accent-position velocity layers creates a forward-leaning emphasis pattern.

### Bank-by-Bank Build Spec

| Bank | Meter | Accent Structure | Kit Expander Transform |
|------|-------|-----------------|----------------------|
| A | 4/4 | Positions 1, 3 (of 4 subdivisions) | Baseline — all layers standard |
| B | 6/8 | Positions 1, 4 (of 6 subdivisions) | v4 layer: transient sharp on 1+4, -3 dB on 2,3,5,6 |
| C | 5/4 | Positions 1, 3 (of 5 subdivisions) | Sample start +6 ms on positions 2,4,5 |
| D | 7/8 | Positions 1, 3, 5 (of 7 subdivisions) | Pitch +8 ct on accent positions, -3 ct on weak positions |

### Implementation Strategy

The bank-differentiation logic lives entirely in sample preparation, not in the XPM structure. Each bank is a standard drum XPM sharing the same program name root but with different WAV sets in its `Samples/` subfolder.

**Oxport tools:**

- `xpn_kit_expander.py` — derive velocity-shaped and pitch-shifted WAV variants per meter
- `xpn_drum_export.py` — generate the XPM for each bank with the appropriate WAV set
- `xpn_bundle_builder.py` — assemble the four-bank pack under one XPN

**Command sketch:**

```bash
# Prepare metrically-shaped WAV sets
python3 xpn_kit_expander.py --kit-dir flat_kit/ --preset "Polyrhythm 808" \
    --mode velocity --meter 6_8 --output wavs/bank_B/

# Generate XPM per bank
python3 xpn_drum_export.py --preset "Polyrhythm 808" --bank B \
    --wavs-dir wavs/bank_B/ --output-dir programs/

# Bundle all four banks
python3 xpn_bundle_builder.py build --profile polyrhythm_banks.json \
    --output-dir dist/
```

**New flag needed:** `xpn_kit_expander.py --meter [4_4|6_8|5_4|7_8]` — applies the accent-position transform logic per meter definition. Estimated 1-day addition to kit_expander.

**Estimated build time:** 2 days per kit (1 day DSP shaping per meter, 1 day XPM assembly and QA).

---

## Innovation 2: Dynamic Range Architecture

### Concept

A pack designed to work across listening contexts — not just volume levels. Low velocity = headphone mix character (intimate, detailed, high-frequency air, narrow stereo). Mid velocity = studio monitor mix character (balanced, full-range, moderate stereo). High velocity = club system character (sub-forward, compressed, wide, transient-heavy).

The crucial distinction from a standard velocity pack: these layers differ in *timbral character*, not just amplitude. Velocity is the listening-context dial.

### Timbral Architecture Per Layer

| Velocity Range | Context | Timbral Recipe |
|---------------|---------|----------------|
| v1 (1–31) | Headphone | High-pass at 40 Hz (no mud), +3 dB shelf at 8 kHz, narrow stereo width (±15°), tail preserved, transient softened |
| v2 (32–63) | Near-field | Flat EQ, moderate low end, moderate width (±30°), balanced attack/tail ratio |
| v3 (64–95) | Studio monitor | +2 dB at 80 Hz, slight 4 kHz dip (listening fatigue reduction), natural width |
| v4 (96–127) | Club system | +4 dB sub shelf below 60 Hz, hard limiter on transient peak (+2 dB loudness), maximum width, 2–3 ms attack softening for travel through PA |

### Spectral Differentiation Math

The headphone→club shift across the velocity range follows a psychoacoustic model:

- **Sub content:** scales from 0.4× (v1) to 1.0× (v4) of original sub amplitude
- **High-frequency detail:** scales from 1.3× (v1) to 0.85× (v4) — phones reward treble; clubs smear it
- **Stereo width:** scales linearly from 15° to 45° pan spread across the four layers
- **Transient attack time:** v1 = +4 ms (softened), v4 = -2 ms (sharpened by transient enhancer)

These transforms compose cleanly with kit_expander.py's existing velocity chain.

### Implementation Strategy

`xpn_kit_expander.py` already has a per-layer transform chain. The Dynamic Range Architecture requires adding a `--context-aware` flag that swaps the EQ and dynamics parameters per layer from the standard velocity model to the context model above.

**Oxport tools:**

- `xpn_kit_expander.py --mode velocity --context-aware` — new flag, applies listening-context transforms
- `xpn_drum_export.py` or `xpn_keygroup_export.py` — no changes needed; accepts the context-shaped WAVs
- `xpn_qa_checker.py` — add a context-range perceptual check that verifies timbral difference between v1 and v4 exceeds a spectral distance threshold

**Estimated build time:** 1 day to add `--context-aware` flag to kit_expander; 2 days per pack design.

---

## Innovation 3: The Evolving Kit

### Concept

A 64-pad kit (4 banks × 16 pads) where navigating forward through the pads — A1 through D16 — traces a complete sonic arc from sparse/dry/clean to dense/saturated/reverberant. The pack IS the arrangement. The producer composes by moving through the pad matrix.

This inverts standard pack logic: instead of 64 versions of the same sound, each pad is a snapshot of the sound at a specific point in an evolution. The kit provides not sounds but *states* — stages along a continuous journey.

### Arc Definition

The 64-pad matrix maps to a progression axis. Each pad has a position index P ∈ [0, 63].

**Five dimensions that evolve continuously with P:**

1. **Saturation:** drive_amount = 0.0 + (P / 63) × 1.0 — linear, from clean to fully driven
2. **Reverb tail length:** reverb_time = 0.1s + (P / 63) × 3.9s — 0.1s → 4.0s
3. **Density (layer count):** pads 0–15 = single sample, 16–31 = doubled (2 layers), 32–47 = layered (4 layers), 48–63 = stacked (6 layers with pitch detuning)
4. **Filter state:** high-pass cutoff falls from 200 Hz at P=0 to 30 Hz at P=63 (opening up the sub)
5. **Stereo width:** 0° at P=0 (mono), 45° at P=63

**Arc equation:**

```
t = P / 63  (normalized position, 0.0 → 1.0)

saturation(P)    = t
reverb_rt60(P)   = 0.1 + t × 3.9
hp_cutoff(P)     = 200 × (1 - t)^2 + 30  (exponential decay toward low end)
stereo_width(P)  = t × 45  (degrees)
layer_count(P)   = 1 + floor(t × 5)  (1 to 6 layers)
```

### Pad-to-Bank Mapping

| Bank | Pads | Arc Phase | Character |
|------|------|-----------|-----------|
| A | 1–16 | P 0–15 | Sparse, clean, mono, no reverb |
| B | 1–16 | P 16–31 | Beginning saturation, slight reverb, slight width |
| C | 1–16 | P 32–47 | Saturated, evident reverb, layered density |
| D | 1–16 | P 48–63 | Fully saturated, long reverb, maximum width, dense |

### Implementation Strategy

This is a render-intensive pack — 64 unique WAVs per sound source. The arc values at each position index drive the DSP chain in kit_expander.

**New tool needed:** `xpn_evolution_renderer.py` — takes a source WAV and an arc spec JSON, renders all 64 positions as individual WAVs, then calls `xpn_drum_export.py` to assemble the full 4-bank XPM set.

**Arc spec JSON format:**

```json
{
  "preset": "Prism Sweep",
  "source_wavs": {
    "kick": "kick_source.wav",
    "snare": "snare_source.wav"
  },
  "arc": {
    "saturation": [0.0, 1.0],
    "reverb_rt60": [0.1, 4.0],
    "hp_cutoff_hz": [200, 30],
    "stereo_width_deg": [0, 45],
    "layer_count": [1, 6]
  },
  "pads": 64
}
```

**Oxport tools:**

- New `xpn_evolution_renderer.py` — arc-driven multi-position render
- `xpn_drum_export.py` — called 4× (once per bank) with positional WAV sets
- `xpn_bundle_builder.py` — assembles the 4-bank pack
- `xpn_liner_notes.py` — document the arc trajectory in pack notes

**Estimated build time:** 3 days (1 day new tool, 1 day per pack design + render, 1 day QA).

---

## Innovation 4: Microtonal Scale Packs

### Concept

Keygroup programs tuned to non-12-TET scales. The `xpn_tuning_systems.py` tool already handles this via `<PitchCents>` offsets injected per `<Instrument>` block. This section documents how to integrate microtonal tuning into the standard Oxport workflow as a first-class pack feature — not a post-processing step.

### How PitchCents Works

Each `<Instrument>` block in a keygroup XPM has a `RootNote` (MIDI 0–127). The tuning engine computes:

```
cent_offset = tuning_cents[note % 12] - TET12_CENTS[note % 12]
```

and injects `<PitchCents>{offset}</PitchCents>` into that block. Negative values flatten; positive values sharpen.

### Priority Tuning Systems for XO_OX Packs

| System | Notes/Oct | Character | Best Engine Match |
|--------|-----------|-----------|------------------|
| 19-TET | 19 | Just minor thirds, meantone-adjacent | ODYSSEY (Violet, detuned pads) |
| 22-TET | 22 | Superpyth — wider major thirds | OVERWORLD (Neon Green, chip era) |
| 31-TET | 31 | Near-perfect 5-limit JI | OPAL (Lavender, granular) |
| Maqam Rast | 12 (with quartertones) | Turkish, 3/4 tone steps | ORACLE (Indigo, gendy) |
| Maqam Bayati | 12 (with quartertones) | Arabic second scale | ORACLE |
| Slendro | 5 | Javanese gamelan pentatonic | OCEANIC (Teal, separation) |
| Pelog | 7 | Javanese gamelan heptatonic | OCEANIC |
| Just Sitar | 12 (extended JI) | Indian sruti system | OSPREY / OSTERIA (ShoreSystem) |
| Bohlen-Pierce | 13 (tritave) | No octave equivalence | ORGANON (metabolic / alien) |
| Harmonic Series | 16 | First 16 harmonics as scale | OUROBOROS (topology) |

### Integration Into Standard Oxport Workflow

Current workflow: render_spec → categorize → expand → qa → export → cover_art → package

Proposed insertion: a `tune` stage between `export` (XPM generation) and `cover_art`:

```
render_spec → categorize → expand → qa → export → TUNE → cover_art → package
```

The `tune` stage calls `xpn_tuning_systems.py` on every generated XPM file in the output directory, injecting `<PitchCents>` offsets according to the requested tuning.

**Oxport integration:**

```bash
# Standard keygroup export with tuning applied
python3 oxport.py run --engine Opal --preset "Crystal Drift" \
    --wavs-dir /path/to/wavs --output-dir /path/to/out \
    --tuning 19tet

# Export all tuning variants simultaneously
python3 oxport.py run --engine Oracle --preset "Gendy Maqam" \
    --tuning all --output-dir /path/to/out
```

**New oxport.py flag:** `--tuning [system_name|all]` — activates the tune stage in the pipeline.

**Pack naming convention:** `{preset_name}_{tuning_system}.xpm` — e.g., `Crystal_Drift_19TET.xpm`, `Crystal_Drift_Slendro.xpm`.

**MIDI note layout note:** For tuning systems with fewer than 12 notes per octave (Slendro = 5, Bohlen-Pierce = 13/tritave), the keygroup zone map must be rebuilt to reflect the new scale's valid positions. `xpn_tuning_systems.py` handles this via its "unused chromatic positions inherit the nearest defined note" strategy — document this behavior prominently in pack liner notes.

**Estimated build time:** Tuning is already implemented. Integration into oxport.py pipeline = 1 day. Per-pack tuning variant generation adds ~30 minutes (automated render).

---

## Innovation 5: Multi-Velocity Synthesis Simulation

### Concept

8–16 velocity layers per pad where each layer is a *different synthesis approach* to the same sound. The velocity sweep is a synthesis morphing axis. Low velocity = one synthesis method. High velocity = a fundamentally different one. The producer's playing dynamics navigate a synthesis space rather than a loudness space.

This treats velocity as an expressive morphing control available to any producer without parameter editing.

### Layer Architecture (8-layer standard)

| Layer | Vel Range | Synthesis Method | Character |
|-------|-----------|-----------------|-----------|
| v1 | 1–16 | Granular cloud (long grain, random position) | Diffuse, atmospheric |
| v2 | 17–31 | Granular cloud (short grain, pitched) | Shimmery, pitched cloud |
| v3 | 32–47 | FM synthesis (simple 2-op) | Clear, bell-like |
| v4 | 48–63 | FM synthesis (4-op, complex ratio) | Complex, metallic |
| v5 | 64–79 | Wavetable scan (smooth) | Evolving, clean |
| v6 | 80–95 | Wavetable scan (stepped) | Lofi, bit-reduced feel |
| v7 | 96–111 | Waveshaper (soft clip) | Warm, saturated |
| v8 | 112–127 | Waveshaper (hard clip + foldback) | Aggressive, harmonically dense |

For drum packs, the synthesis simulation can be condensed to 4 layers: granular → FM → wavetable → distorted.

### Implementation Strategy

Each synthesis method produces a sample via XOceanus offline render (using `xpn_render_spec.py` to define the render parameters per voice). The eight renders are then assembled into a single XPM instrument block via `xpn_keygroup_export.py`'s 8-velocity-layer mode.

**New capability needed in `xpn_render_spec.py`:** Support for specifying synthesis method per velocity layer in the render spec JSON, in addition to the existing per-preset render spec. Estimated 1-day addition.

**Render spec JSON fragment:**

```json
{
  "pad": "A01",
  "source_pitch": "C3",
  "velocity_layers": [
    { "layer": 1, "vel_range": [1, 16],   "engine": "Opal",  "mode": "granular_long" },
    { "layer": 2, "vel_range": [17, 31],  "engine": "Opal",  "mode": "granular_short" },
    { "layer": 3, "vel_range": [32, 47],  "engine": "Oblong", "mode": "fm_2op" },
    { "layer": 4, "vel_range": [48, 63],  "engine": "Oblong", "mode": "fm_4op" },
    { "layer": 5, "vel_range": [64, 79],  "engine": "Overworld", "mode": "wave_scan_smooth" },
    { "layer": 6, "vel_range": [80, 95],  "engine": "Overworld", "mode": "wave_scan_stepped" },
    { "layer": 7, "vel_range": [96, 111], "engine": "Obese", "mode": "waveshaper_soft" },
    { "layer": 8, "vel_range": [112, 127],"engine": "Obese", "mode": "waveshaper_fold" }
  ]
}
```

**Oxport tools:**

- `xpn_render_spec.py` — enhanced with per-layer synthesis method spec
- `xpn_keygroup_export.py` — already supports 8-layer velocity; no changes needed
- `xpn_bundle_builder.py` — standard assembly

**Estimated build time:** 2 days per pack design (synthesis approach selection + render QA). 1 day to add multi-method layer support to render_spec tool.

---

## Innovation 6: The Negative Kit

### Concept

A pack designed for what you *don't* play. Each of the 16 pads contains a 1-bar loop of near-silence with the percussion pattern cut out — the gaps, the spaces, the breathable absence. The producer plays the missing beats.

This reverses the standard kit metaphor: instead of triggering sounds, the producer triggers *the room around sounds*.

### Sonic Content Per Pad

Each pad contains a processed 1-bar audio loop (typically 2–4 seconds at 120 BPM) where:

1. A reference drum pattern has been played and then **silence-gated** — the hits have been replaced by matched-decay noise floor ambience
2. The room tone, reverb tails, and air of the pattern remain intact
3. The beats themselves are absent — replaced by the natural decay of the space

The result: playing the pad gives you a room that is "expecting" a hit. The producer's own hits fill the negative space.

**16 pad types:**

| Pad | Pattern Removed | Room Character |
|-----|----------------|----------------|
| 1 | 4-on-the-floor kick | Dry room, tight walls |
| 2 | Snare on 2 and 4 | Medium room |
| 3 | Triplet hi-hat | Ambient pad room |
| 4 | Offbeat clap | Small room |
| 5 | Syncopated bass kick | Large hall |
| 6 | Swing hi-hat pattern | Studio B |
| 7 | Ghost snare pattern | Concrete room |
| 8 | Accent clave | Open air |
| 9 | 808 sub pattern | Club room with sub |
| 10 | Afrobeat hi-hat | Tile room |
| 11 | Trap triplet pattern | Trap room (sparse reverb) |
| 12 | Drum'n'bass break | Warehouse |
| 13 | Jazz ride pattern | Jazz club |
| 14 | Latin conga pattern | Outdoor courtyard |
| 15 | Free jazz texture | No room, void |
| 16 | Silence (all beats removed) | Anechoic |

### Implementation Strategy

The "negative" samples are not generated algorithmically — they are rendered by recording a full pattern and then applying surgical silence-gating to remove the hits while preserving the acoustic environment.

**Technique:** Record the room/reverb impulse separately, then align it with the gap positions. Alternatively: render with very long pre-delay reverb (50+ ms) and truncate the initial transient while preserving the tail.

**Oxport tools:**

- `xpn_drum_export.py` — each negative loop is treated as a single-layer drum pad; `OneShot=False`, `Loop=True`
- `xpn_render_spec.py` — document the loop length and BPM alignment per pad
- `xpn_packager.py` — standard packaging
- `xpn_liner_notes.py` — critical for this pack; the concept requires explanation to work

**Estimated build time:** 3 days (2 days recording and editing the negative loops, 1 day XPM assembly and liner notes).

---

## Innovation 7: XPN as Score

### Concept

A pack where the pad matrix encodes a complete musical composition. 16 columns = 16 bars of the composition. 4 rows (banks) = 4 sections (Verse / Chorus / Bridge / Outro). Each pad contains the appropriate sound for that bar of that section. The producer reads the pad grid like a score and performs the arrangement live.

This is opposed to the standard kit metaphor (a set of sounds to arrange) — here the arrangement is already resolved. The producer's job is *performance*, not composition.

### Grid Encoding

The 4 × 16 pad matrix maps to the composition as follows:

| Bank | Section | Bars |
|------|---------|------|
| A (row 1) | Verse | Bars 1–16 |
| B (row 2) | Chorus | Bars 1–16 |
| C (row 3) | Bridge | Bars 1–16 |
| D (row 4) | Outro | Bars 1–16 |

Each pad (e.g., Bank B / Pad 7) contains the sound content appropriate to Chorus, Bar 7. For a melodic composition this means a single looped audio segment of the melody/harmony/rhythm at that compositional moment. For a drum composition it means the drum pattern active at that bar of that section.

### Sound Content Per Pad

Each pad is a rendered audio loop of the material at that grid position:

- **Melodic / harmonic content** — a 1-bar or 2-bar loop of the chords, melody, or bass at that compositional position
- **Dynamic state** — energy level appropriate to the section (verse: low-mid energy; chorus: peak energy; bridge: transitional; outro: resolving)
- **Processing state** — each section's pads have consistent spectral character (verse = drier, chorus = more reverb, bridge = filtered/strange, outro = fading treatment)

### Performance Workflow

The producer loads the pack and plays down a section by pressing pads 1→16 in sequence. Each pad triggers the next bar of that section. To move between sections, they switch banks. To compose freely, they jump between grid positions non-linearly — playing Chorus 7 after Verse 3, skipping bars, repeating pads.

The MPC's performance features (pad mute, pad mute groups, tap tempo follow) all work naturally on top of this.

### Score-Writing Workflow

Composing a Score pack is the reverse of standard pack design: the composition is written first, then rendered into 64 audio segments.

1. **Sketch the composition** as a standard DAW project or notation
2. **Render 64 segments** — one per pad position — from the composition
3. **Label the segments** by grid position (section, bar)
4. **QA the transitions** — ensure natural crossfades between adjacent pads
5. **Assemble via Oxport** with loop mode on all pads

**Oxport tools:**

- `xpn_drum_export.py` or `xpn_keygroup_export.py` — one program per bank (4 programs total)
- `xpn_render_spec.py` — grid-position render spec (64 render targets)
- `xpn_bundle_builder.py` — collect 4 programs into one XPN
- `xpn_liner_notes.py` — annotate the score structure; this pack requires documentation

**New utility:** A `xpn_score_grid.py` tool would accept a composition timeline and output the render spec JSON with all 64 grid positions pre-populated with bar/section assignments. Estimated 1 day to build.

**Estimated build time:** 4–5 days per composition (2 days composition, 2 days rendering 64 segments, 1 day assembly + QA).

---

## Summary Table

| Innovation | Core Exploit | New Tool Needed | Est. Build Time |
|-----------|-------------|----------------|----------------|
| 1. Polyrhythmic Banks | Kit expander accent-position shaping | `--meter` flag for kit_expander | 2 days/kit |
| 2. Dynamic Range Architecture | Context-aware velocity layers | `--context-aware` flag for kit_expander | 1 day setup, 2 days/pack |
| 3. Evolving Kit | 64-position arc render | `xpn_evolution_renderer.py` (new) | 3 days/kit |
| 4. Microtonal Scale Packs | PitchCents per instrument block | `--tuning` flag for oxport.py pipeline | 1 day setup, 30 min/variant |
| 5. Multi-Velocity Synthesis Simulation | Per-layer synthesis method in render spec | Multi-method layer support in render_spec | 2 days/pack |
| 6. The Negative Kit | Silence-gated loop pads | None (manual recording + standard tools) | 3 days/pack |
| 7. XPN as Score | Composition→grid mapping | `xpn_score_grid.py` (new) | 4–5 days/composition |

---

## Next Steps

Priority order for implementation, based on effort/impact ratio:

1. **Microtonal Scale Packs** — highest impact, infrastructure already built. One day to wire `--tuning` into oxport.py pipeline. XO_OX would be the only MPC expansion vendor offering this.

2. **Dynamic Range Architecture** — one flag addition to kit_expander; immediately applicable to every existing drum pack.

3. **Polyrhythmic Banks** — requires meter-aware accent shaping in kit_expander. 1-day code addition unlocks multi-bank polyrhythmic kits for any XOnset preset.

4. **Multi-Velocity Synthesis Simulation** — requires deeper render_spec extension. High artistic payoff; builds on existing 8-layer keygroup infrastructure.

5. **Evolving Kit** — new tool required. Medium effort, high concept payoff. Particularly well-suited to XOpal (granular) and XOverworld (era-crossfade) sounds.

6. **The Negative Kit** — conceptually radical, requires manual sample recording. Single flagship pack; not a recurring workflow.

7. **XPN as Score** — highest effort, most compositional ambition. One flagship release. Consider as a 1.x feature.
