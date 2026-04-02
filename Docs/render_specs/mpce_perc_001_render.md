# Render Spec: XO_OX MPCe Percussion Vol. 1

**Pack ID:** xoox-mpce-perc-001
**Engine:** ONSET
**Corner Pattern:** Dynamic Expression (ghost / accent / standard / effect)
**Total Samples Needed:** 256 (16 pads x 4 corners x 4 velocity layers)

---

## Render Setup

| Setting | Value |
|---------|-------|
| Sample Rate | 44100 Hz |
| Bit Depth | 24-bit |
| Format | WAV (no compression) |
| Normalization | Per-layer (each velocity layer normalized independently) |
| Tail | 2 seconds max (no baked reverb — users add MPC FX) |
| Stereo | Mono for kicks/snares, stereo for hats/cymbals/FX |

---

## Velocity Layers (Vibe's Musical Curve)

| Layer | MIDI Velocity | Character | Render Technique |
|-------|--------------|-----------|-----------------|
| v1 | 20 | Ghost | Soft trigger, filter at 60%, minimal transient |
| v2 | 50 | Light | Medium trigger, filter at 75%, natural attack |
| v3 | 90 | Medium | Firm trigger, filter at 90%, full body |
| v4 | 127 | Hard | Max trigger, filter fully open, peak transient |

---

## Corner Variants

For each pad voice, render 4 corner variants:

| Corner | Variant | Processing |
|--------|---------|------------|
| NW | Ghost/Muted | Low velocity preset, HPF at 300Hz, short decay, gentle |
| NE | Accent/Bright | High velocity preset, no HPF, sharp transient, alternate articulation |
| SW | Standard | Default preset, balanced EQ, natural character |
| SE | Effect/Textured | Processed: add chorus OR flam OR buzz OR saturation |

---

## Per-Pad Render Instructions

### A01 — Kick (MIDI 36)
**Preset source:** Select ONSET preset with DNA: warmth > 0.6, aggression > 0.5
- NW: Reduce `perc_noiseLevel` to 0.1, `perc_filterCutoff` to 0.4 → sub thump
- NE: Increase `perc_noiseLevel` to 0.7, `perc_filterCutoff` to 0.9 → beater click
- SW: Default preset values → full kick
- SE: Layer two renders (offset 5ms) + mild saturation → layered boom
- **4 velocity layers each = 16 WAVs**

### A02 — Snare (MIDI 38)
**Preset source:** Select ONSET preset with DNA: brightness > 0.5, aggression > 0.4
- NW: `perc_noiseLevel` low, velocity 20, filter closed → ghost tap
- NE: Pitch snare body up 2 semitones, boost 2-4kHz → rimshot character
- SW: Default → center hit
- SE: Rapid double trigger (8ms spacing) → buzz roll
- **16 WAVs**

### A03 — Closed Hat (MIDI 42)
**Preset source:** Select ONSET preset with DNA: brightness > 0.7, density < 0.4
- NW: Very short decay, filter closed → tick
- NE: Slightly open, brighter → pedal splash
- SW: Default closed hat → standard
- SE: Replace with shaker sample or filtered noise burst → shaker
- **16 WAVs**
- **MuteGroup: 1** (chokes with A04)

### A04 — Open Hat (MIDI 46)
**Preset source:** Same preset as A03, longer decay
- NW: Partially open → half open
- NE: Blend with crash tail → crash bleed
- SW: Full open → standard open hat
- SE: Add sizzle (high resonance, slow LFO on cutoff) → sizzle
- **16 WAVs**
- **MuteGroup: 1** (chokes with A03)

### A05–A08 — Color (Clap, Low Tom, Rimshot, Crash)
Follow the same NW/NE/SW/SE pattern:
- NW = softest/most muted variant
- NE = brightest/most aggressive alternate articulation
- SW = default/standard voice
- SE = processed/effected/layered variant
- **64 WAVs total**

### A09–A12 — Texture (Ride, Mid Tom, High Tom, Floor Tom)
Same pattern. For toms: muted/rim/open/roll variations.
- **64 WAVs total**

### A13–A16 — Atmosphere (Tambourine, Cowbell, Clave, Wood Block)
Same pattern. These can be more experimental in SE corner.
- **64 WAVs total**

---

## File Naming Convention

```
{Pad}_{Voice}_{Corner}_{Velocity}.wav

Examples:
A01_Kick_NW_v1.wav     (Ghost corner, ghost velocity)
A01_Kick_NW_v2.wav     (Ghost corner, light velocity)
A01_Kick_NE_v4.wav     (Accent corner, hard velocity)
A02_Snare_SW_v3.wav    (Standard corner, medium velocity)
```

---

## Output Directory Structure

```
renders/mpce_perc_001/
├── A01_Kick/
│   ├── A01_Kick_NW_v1.wav
│   ├── A01_Kick_NW_v2.wav
│   ├── A01_Kick_NW_v3.wav
│   ├── A01_Kick_NW_v4.wav
│   ├── A01_Kick_NE_v1.wav
│   ├── ...
│   └── A01_Kick_SE_v4.wav    (16 files per pad)
├── A02_Snare/
│   └── ...
├── ...
└── A16_WoodBlock/
    └── ...
```

---

## Post-Render Pipeline

After all 256 WAVs are rendered:

```bash
# 1. Validate samples
python Tools/xpn_sample_audit.py renders/mpce_perc_001/

# 2. Build MPCe XPM (quad-corner program)
python Tools/xpn_mpce_quad_builder.py \
    --engine ONSET \
    --presets-dir Presets/XOceanus/ \
    --output-dir output/mpce_perc_001/

# 3. Build standard XPM (velocity-layered fallback)
python Tools/xpn_drum_export.py \
    --mode smart \
    --input renders/mpce_perc_001/ \
    --output output/mpce_perc_001/

# 4. Generate intent sidecar
python Tools/xpn_intent_generator.py \
    --pack-name "XO_OX MPCe Percussion Vol. 1" \
    --engine ONSET \
    --corner-pattern dynamic_expression \
    --presets-dir Presets/XOceanus/ \
    --output output/mpce_perc_001/xpn_intent.json

# 5. Copy MPCE_SETUP.md
cp Docs/templates/MPCE_SETUP.md output/mpce_perc_001/Docs/

# 6. Generate cover art
python Tools/xpn_cover_art_generator_v2.py \
    --engine ONSET \
    --name "MPCe Percussion Vol. 1" \
    --badge "MPCe Exclusive" \
    --output output/mpce_perc_001/

# 7. Package as .xpn
python Tools/xpn_packager.py \
    --input output/mpce_perc_001/ \
    --output "XO_OX_MPCe_Percussion_Vol1.xpn"

# 8. Validate final pack
python Tools/xpn_validator.py "XO_OX_MPCe_Percussion_Vol1.xpn"
```

---

## Preset Selection Criteria

The quad builder tool will auto-select from the 508 ONSET presets using DNA distance
maximization. For manual curation, prioritize:

1. **DNA diversity** — opposite corners should be maximally different
2. **Velocity response** — presets must sound meaningfully different at vel 20 vs vel 127
3. **Macro responsiveness** — CHARACTER macro should audibly shape each voice
4. **CPU efficiency** — avoid presets with high polyphony or long reverb tails (MPC CPU)
5. **Transient clarity** — drums need clean attacks that survive sample conversion

---

## Quality Gate

Before shipping, verify:
- [ ] All 256 WAVs render without clipping
- [ ] Hat choke groups work (A03 ↔ A04)
- [ ] Corner morphing sounds smooth (no abrupt transitions)
- [ ] Standard version plays correctly on non-MPCe hardware
- [ ] MPCE_SETUP.md is accurate and helpful
- [ ] xpn_intent.json validates against schema
- [ ] Cover art renders at 2000x2000 and 1000x1000
- [ ] Total pack size < 500MB
