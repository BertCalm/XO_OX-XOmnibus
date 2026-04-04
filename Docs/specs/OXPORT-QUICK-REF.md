# OXPORT Pipeline — Quick Reference

## 10-Stage Pipeline At A Glance

| # | Stage | Input | Output | Purpose |
|----|-------|-------|--------|---------|
| 1 | **render_spec** | .xometa presets | {slug}_render_spec.json | SELECT: Define what to render |
| 2 | **categorize** | WAV files | voice map + DNA cache | Classify samples (kick/snare/etc) |
| 3 | **expand** | Categorized samples | Expanded WAV layers | Fill missing velocity/cycle layers |
| 4 | **qa** | WAV files | QA report ± auto-fixes | Validate quality (clipping, DC offset) |
| 5 | **smart_trim** | WAV files | Trimmed WAVs | Remove silence tails |
| 6 | **export** | Samples + render spec | .xpm program files | ASSEMBLE: Generate MPC programs |
| 7 | **cover_art** | Engine color + preset name | .png cover art | Create branded pack cover |
| 8 | **complement_chain** | XPM + cover art | 5 variant XPM/covers | Artwork collection shades |
| 9 | **preview** | Samples | preview.wav (15s) | Short demo audio |
| 10 | **package** | XPM + art + samples | .xpn archive | Bundle into MPC pack |

## File Paths (Absolute)

```
/Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/

Tools/
  oxport.py                        (3,124 lines) — Main orchestrator
  oxport_render.py                 (1,500+ lines) — MIDI→audio via BlackHole
  xpn_render_spec.py               (1,000+ lines) — Generate render specs
  xpn_batch_export.py              (350+ lines) — Batch job runner
  xpn_drum_export.py               (45K) — Onset XPM generation
  xpn_keygroup_export.py           (48K) — Keygroup XPM generation
  xpn_complement_renderer.py       (200+ lines) — Artwork variants
  xpn_bundle_builder.py            (1,437 lines) — Preset bundling
  xpn_kit_expander.py              — Drum kit expansion
  xpn_sample_categorizer.py        — Voice classification
  xpn_qa_checker.py                — QA checks + auto-fixes
  onset_render_spec.json           (116K) — Concrete example

Docs/specs/
  export-architecture.md           (High-level overview)
  oxbuild-oxport-full-dossier.md   (THIS FILE'S SOURCE)
  OXPORT-QUICK-REF.md              (This quick ref)

Presets/XOceanus/
  {Mood}/{Engine}/*.xometa         (Input presets)
```

## Command Examples

### Single Pack Build
```bash
cd /Users/joshuacramblet/Documents/GitHub/XO_OX-XOmnibus/
python3 Tools/oxport.py run \
  --engine Onset \
  --wavs-dir /path/to/wavs \
  --output-dir ./dist/onset/ \
  --kit-mode smart \
  --version 1.0
```

### Batch Export (3 parallel)
```bash
python3 Tools/xpn_batch_export.py \
  --config Tools/batch.json \
  --parallel 3 \
  --skip-failed
```

### Dry Run
```bash
python3 Tools/oxport.py run --engine Onset --output-dir /tmp/test --dry-run
```

### Validate
```bash
python3 Tools/oxport.py validate --presets --strict
python3 Tools/oxport.py validate --output-dir ./dist/onset/
```

## Key Constants

### Drum Pad Layout (8 pads, GM convention)
```
MIDI 36 (C2)   → kick
MIDI 38 (D2)   → snare
MIDI 39 (D#2)  → clap
MIDI 42 (F#2)  → closed hat  (MuteGroup 1)
MIDI 46 (A#2)  → open hat    (MuteGroup 1, muted by closed hat)
MIDI 41 (F2)   → tom
MIDI 43 (G2)   → percussion
MIDI 49 (C#3)  → fx/cymbal
```

### Velocity Layers (Musical Curve)
```
v1 (1–20)    [0.30] ghost    — barely touching
v2 (21–50)   [0.55] light    — gentle playing
v3 (51–90)   [0.75] mid      — expressive sweet spot
v4 (91–127)  [0.95] hard     — full force
```

### Engine Render Strategies

| Engine | Type | Notes |
|--------|------|-------|
| ONSET | Drum | 8 voices × 4 vel = 32 WAVs |
| ODYSSEY | Keygroup | C1-C6, every minor 3rd, 4 vel = 84 WAVs |
| OBESE | Keygroup | C1-C3 bass focus, 4 vel = 36 WAVs |
| OBRIX | Keygroup | C1-C5, 4 vel, 2 variants (field_off/on) = 176 WAVs |
| OPAL | Stem | 30-60s texture renders (not pitched) |
| OPTIC | Stem | 30s AutoPulse (visual-only, may be silent) |
| OSTINATO | Drum | 8 voices × 4 vel = 32 WAVs |

**See xpn_render_spec.py for all 75+ engine strategies**

### Kit Modes (Drums)
```
smart      — Per-voice auto-select (recommended)
velocity   — All voices use velocity layers (dynamic)
cycle      — All voices use round-robin (machine-gun prevention)
random     — All voices use random pick (organic)
```

### Smart Mode Assignments
```
kick  → velocity  (dynamics matter)
snare → velocity  (dynamics matter)
chat  → cycle     (machine-gun prevention)
ohat  → cycle     (machine-gun prevention)
clap  → random    (organic feel)
tom   → velocity
perc  → cycle
fx    → random-norepeat
```

### XPM Critical Rules (Never Break)
```
1. KeyTrack = True   (samples transpose across zones)
2. RootNote = 0      (MPC auto-detect convention)
3. VelStart = 0      (empty layers prevent ghost triggering)
```

## Silent Sample Detection

**Current Status:** Not yet automated

**When It Matters:**
- OPTIC engine: visual-only, may produce no audio
- Unmapped voice routes: some presets don't route audio to certain voices
- Broken signal paths: filter cutoff at 0 Hz, amplitude = 0

**Current Workaround:**
- QA stage flags silent files (via RMS check)
- Manual review + fix + re-render

**Planned Solution:**
- Smart voice map: pre-analyze preset routing, skip silence-producing jobs
- Post-render energy detection: validate no unexpected silence

## Render Specs (INPUT → SELECT)

**Format:** JSON (one per preset)
**Location:** `build_dir/specs/{preset_slug}_render_spec.json`
**Size:** ~2-5 KB per spec
**Key Fields:**
```json
{
  "preset_name": "Crystal Gentle",
  "preset_slug": "Crystal_Gentle",
  "engine": "Onset",
  "program_type": "drum",
  "wav_count": 32,
  "voices": ["kick", "snare", "chat", "ohat", "clap", "tom", "perc", "fx"],
  "velocity_midi_values": [20, 50, 90, 127],
  "dna": { "brightness": 0.95, "warmth": 0.30, ... },
  "wavs": [
    { "filename": "Crystal_Gentle_kick_v1.wav", "voice": "kick", "velocity_layer": 1, "midi_velocity": 20 },
    ...
  ]
}
```

## XPM Output (ASSEMBLE → EXPORT)

**Format:** Akai MPC SysEx XML
**Example:** `{preset_slug}.xpm`
**Structure:**
```xml
<Program type="Drum|Keygroup" name="..." color="...">
  <Instrument index="0" name="..." zone_play="1|2|3|4">
    <Layer vel_start="1" vel_end="31" sample="..." />
    <Layer vel_start="32" vel_end="63" sample="..." />
    ...
  </Instrument>
  ...
</Program>
```

## XPN Archive (FINAL OUTPUT → PACKAGE)

**Format:** ZIP (Akai MPC expansion pack)
**Example:** `onset_1.0.xpn` (425 MB)
**Contents:**
```
onset_1.0.xpn
├── Program.xpm
├── cover.png
├── Samples/ (all WAV files)
├── metadata.json
└── manifest.txt
```

## QA Auto-Fix Features

**Safe Fixes** (applied with --auto-fix):
- DC_OFFSET: Remove mean offset
- ATTACK_PRESENCE: Add 1ms fade-in
- SILENCE_TAIL: Trim + crossfade

**Blocking Issues** (require manual intervention):
- CLIPPING: Peak > 1.0 (must re-render)
- PHASE_CANCELLED: L/R channels cancel (routing error)

## Current Status: MPCE-PERC-001

- **Presets:** 633
- **Total WAVs:** 12,800
- **Render Jobs:** Running in parallel
- **Parallel Slots:** 4 (recommended)
- **Expected Duration:** ~7-10 hours (depends on render complexity)
- **Known Issue:** Many silent warnings expected (unmapped voices)

## Performance Tips

1. **Parallelization:** `--parallel 4` balances CPU/memory
2. **Dry-run first:** Validate config before full run
3. **DNA cache:** Computed once (stage 2), reused (stage 6)
4. **Smart trim:** 10-15% size reduction
5. **Auto-fix:** Use for large batches (saves manual review)

## Troubleshooting

| Problem | Fix |
|---------|-----|
| "No WAV files found" | Check --wavs-dir path, verify render completed |
| "Engine not recognized" | Check spelling, see ENGINE_ALIASES in oxport.py |
| "CLIPPING in N files" | Re-render at lower gain |
| "Path escape blocked" | Sanitize preset names (no `../`) |
| "Timeout after 600s" | Job may be stuck; check audio loopback, MIDI routing |

## Documentation

- **Full Dossier:** `/Docs/specs/oxbuild-oxport-full-dossier.md` (this master doc)
- **Architecture:** `/Docs/specs/export-architecture.md` (quick overview)
- **CLAUDE.md:** XPN export rules + XPM critical rules
- **Code:** `/Tools/oxport.py` — read docstring + class PipelineContext for deep dive

---

**Quick Ref Last Updated:** 2026-04-04
