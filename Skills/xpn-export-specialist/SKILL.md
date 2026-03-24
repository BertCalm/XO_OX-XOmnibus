# Skill: /xpn-export-specialist

**Invoke with:** `/xpn-export-specialist`
**Status:** LIVE
**Last Updated:** 2026-03-20 | **Version:** 1.0 | **Next Review:** On XPN format changes or new MPC compatibility requirements
**Purpose:** End-to-end guide for building XPN (MPC-compatible) export bundles from XOlokun presets — covering render specs, WAV rendering, XPM generation, bundle assembly, and the 3 critical XPM rules.

---

## When to Use This Skill

Use this skill when:
- Preparing an XPN export bundle for a preset pack
- Rendering individual presets to WAV for MPC
- Building drum programs or keygroup programs
- Debugging XPM files that cause ghost triggering, wrong root note, or pitch tracking issues
- Running the full XPN pipeline from preset to deliverable bundle

---

## The 3 Critical XPM Rules — Never Break These

These apply to every `.xpm` file in every export. No exceptions.

1. **`KeyTrack = True`** — Samples must transpose across keyboard zones. Without this, every zone plays at the same pitch and the keygroup is useless for melodic instruments.

2. **`RootNote = 0`** — MPC convention meaning "no fixed root; determine pitch from zone mapping." Do NOT set this to 60 (MIDI middle C) or any other value. When `RootNote` is set to a specific note number, the MPC applies a pitch offset relative to that root, causing every zone to play at the wrong pitch. Setting it to 0 disables that offset and lets the keyboard zone mapping define the pitch.

3. **Empty layer `VelStart = 0`** — Empty layers (no WAV assigned) must have VelStart=0. If an empty layer has VelStart=1 or higher, it can ghost-trigger on velocity-0 note-off messages, causing phantom sounds.

---

## Pipeline Overview

```
.xometa presets
    │
    ▼
xpn_render_spec.py  →  render specs (JSON instructions per preset)
    │
    ▼
[DAW/Engine rendering]  →  WAV files per note/velocity zone
    │
    ▼
xpn_keygroup_export.py  →  .xpm keygroup programs
xpn_drum_export.py      →  .xpm drum programs
    │
    ▼
xpn_kit_expander.py  →  DSP transforms (stretch, resample, tune)
    │
    ▼
xpn_bundle_builder.py  →  final .xpn bundle (zip of programs + wavs + art)
    │
    ▼
xpn_cover_art.py  →  cover art PNG
    │
    ▼
.xpn deliverable
```

---

## Tool Reference

All tools live in `Tools/`. Run from repo root.

| Tool | Command | Purpose |
|------|---------|---------|
| `xpn_render_spec.py` | `python3 Tools/xpn_render_spec.py --preset path.xometa` | Generate render instructions for one preset |
| `xpn_render_spec.py` | `python3 Tools/xpn_render_spec.py --engine OddfeliX` | Generate specs for all presets of an engine |
| `xpn_render_spec.py` | `python3 Tools/xpn_render_spec.py --all --output-dir /tmp/specs` | All presets → JSON spec files |
| `xpn_keygroup_export.py` | `python3 Tools/xpn_keygroup_export.py --preset NAME --wavs-dir /path` | Build keygroup XPM from WAVs |
| `xpn_drum_export.py` | `python3 Tools/xpn_drum_export.py --preset NAME` | Build drum XPM |
| `xpn_kit_expander.py` | `python3 Tools/xpn_kit_expander.py --source audio.wav --voice NAME` | Expand one WAV to full velocity/note matrix |
| `xpn_bundle_builder.py` | `python3 Tools/xpn_bundle_builder.py build --profile profile.json --wavs-dir /path --output-dir /out` | Full bundle assembly |
| `xpn_bundle_builder.py` | `python3 Tools/xpn_bundle_builder.py build --profile profile.json --dry-run` | Dry run (no files written) |
| `xpn_sample_categorizer.py` | `python3 Tools/xpn_sample_categorizer.py` | Categorize samples into program types |
| `xpn_cover_art.py` | `python3 Tools/xpn_cover_art.py` | Generate cover art |
| `xpn_packager.py` | `python3 Tools/xpn_packager.py` | Final .xpn packaging |

---

## Step-by-Step: Full Export Workflow

### Step 1: Generate Render Spec

```bash
# Single preset
python3 Tools/xpn_render_spec.py --preset Presets/XOlokun/Foundation/Deep_Cut.xometa --json

# All Foundation presets
python3 Tools/xpn_render_spec.py --engine Oblong --output-dir /tmp/specs
```

The render spec tells you:
- Which MIDI notes to render (usually C0–C8 per zone, or specific note set for drums)
- Which velocity layers (typically 4: pp=1–31, mp=32–63, mf=64–95, ff=96–127)
- Target loudness (-18 LUFS for melodic, -12 LUFS for drum transients)
- Recommended sample length (sustain + release tail)

### Step 2: Render WAVs

Render WAVs from the XOlokun plugin using the spec. File naming convention:

```
{preset_slug}__{note}__{velocity_layer}.WAV
```

Examples:
```
Deep_Cut__C3__pp.WAV
Deep_Cut__C3__mp.WAV
Deep_Cut__C3__mf.WAV
Deep_Cut__C3__ff.WAV
Deep_Cut__C4__pp.WAV
...
```

Where `{note}` = MIDI note name (C3, D#4, G5, etc.) and `{velocity_layer}` = pp/mp/mf/ff.

**WAV spec:**
- Format: 24-bit stereo
- Sample rate: 44.1kHz (or 48kHz for film sync)
- No DC offset (verify with Tools)
- No clipping (peaks < -0.1 dBFS)
- Silence trim: trim leading silence but preserve release tail

### Step 3: Expand to Full Keygroup (if using kit_expander)

If you only have one or a few source WAVs per preset and need full keyboard coverage:

```bash
python3 Tools/xpn_kit_expander.py \
  --source /path/to/Deep_Cut__C3__ff.WAV \
  --voice "Deep Cut" \
  --preset "Deep_Cut" \
  --output-dir /tmp/expanded
```

The kit expander applies pitch shifting and time-stretching to create a full keyboard matrix from a small source set.

### Step 4: Build the XPM Program

```bash
# Keygroup program (melodic instruments, pads, basses)
python3 Tools/xpn_keygroup_export.py \
  --preset "Deep Cut" \
  --wavs-dir /path/to/rendered_wavs \
  --output /tmp/programs

# Drum program (percussion, kits)
python3 Tools/xpn_drum_export.py \
  --preset "Deep Cut" \
  --wavs-dir /path/to/rendered_wavs \
  --output /tmp/programs
```

**Always verify the output XPM against the 3 critical rules before proceeding.**

### Step 5: Verify XPM Against Critical Rules

Open the generated `.xpm` in a text editor and check:

```xml
<!-- ✅ CORRECT -->
<KeyGroup>
    <KeyTrack>True</KeyTrack>
    <RootNote>0</RootNote>
    ...
    <Layer number="3">
        <SampleName></SampleName>
        <VelStart>0</VelStart>  <!-- ✅ Empty layer MUST be 0 -->
        <VelEnd>0</VelEnd>
    </Layer>
</KeyGroup>
```

```xml
<!-- ❌ WRONG — will cause bugs -->
<KeyTrack>False</KeyTrack>     <!-- ❌ pitches won't transpose -->
<RootNote>60</RootNote>        <!-- ❌ pitch offset error -->
<VelStart>1</VelStart>         <!-- ❌ ghost triggering on empty layer -->
```

### Step 6: Build the Bundle

Set up a bundle profile in `Tools/bundle_profiles/`:

```json
{
  "name": "Foundation Pack Vol. 1",
  "version": "1.0.0",
  "description": "Bass, kicks, and rhythmic anchors from XOlokun",
  "presets": [
    {
      "name": "Deep Cut",
      "engine": "Oblong",
      "type": "keygroup",
      "path": "Foundation/Deep_Cut.xometa"
    },
    {
      "name": "Kick Pulse",
      "engine": "Onset",
      "type": "drum",
      "path": "Foundation/Kick_Pulse.xometa"
    }
  ]
}
```

Then build:

```bash
# Dry run first
python3 Tools/xpn_bundle_builder.py build \
  --profile Tools/bundle_profiles/foundation_pack_v1.json \
  --wavs-dir /path/to/rendered_wavs \
  --output-dir /tmp/bundles \
  --dry-run

# Full build
python3 Tools/xpn_bundle_builder.py build \
  --profile Tools/bundle_profiles/foundation_pack_v1.json \
  --wavs-dir /path/to/rendered_wavs \
  --output-dir /tmp/bundles
```

### Step 7: Add Cover Art

```bash
python3 Tools/xpn_cover_art.py \
  --bundle-name "Foundation Pack Vol. 1" \
  --output /tmp/bundles/cover.png
```

### Step 8: Package

```bash
python3 Tools/xpn_packager.py \
  --bundle-dir /tmp/bundles \
  --output /releases/foundation_pack_v1.xpn
```

---

## Common Errors and Fixes

| Error | Likely Cause | Fix |
|-------|-------------|-----|
| Notes play at wrong pitch | `RootNote` ≠ 0 | Set `RootNote=0` in XPM |
| All zones play the same pitch | `KeyTrack=False` | Set `KeyTrack=True` |
| Ghost notes on release | Empty layer `VelStart` > 0 | Set all empty layer `VelStart=0` |
| WAV not found in bundle | Filename slug mismatch | Check slug uses `_` not spaces/hyphens |
| Velocity layers not triggering | VelStart/VelEnd overlap | Ensure non-overlapping: 1–32, 33–64, 65–96, 97–127 |
| Kit expander creates aliasing | Source pitch too far from target | Use source WAVs at every octave |

---

## WAV Filename Slugs

Bundle tools use slugs derived from preset names:
```python
slug = preset_name.replace(" ", "_").replace("/", "-").replace("'", "")
```

"Deep Cut" → `Deep_Cut`
"Dub's Echo" → `Dubs_Echo`
"Low/High Frequency" → `Low-High_Frequency`

Keep preset names clean (no apostrophes, no colons, no special characters) to avoid slug mismatches.

---

## XPN Output Format Reference

The final `.xpn` is a ZIP archive containing:

```
{bundle_name}.xpn/
├── Programs/
│   ├── {preset_slug}.xpm           # Keygroup or drum program
│   └── ...
├── Samples/
│   ├── {preset_slug}__C3__ff.WAV   # Rendered WAVs
│   └── ...
├── Metadata/
│   ├── manifest.json               # Bundle metadata
│   └── {preset_slug}.xometa        # Original preset file
└── Art/
    └── cover.png                   # 500×500px cover art
```

---

## Dependency Check

Before running any XPN tools:

```bash
pip install soundfile scipy  # Required for kit_expander DSP features
```

- `soundfile` — required for WAV read/write (will error without it)
- `scipy` — recommended for high-quality DSP transforms; approximate mode used without it
