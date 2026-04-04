# OxBuild/Oxport XPN Export Pipeline — Complete Dossier

**Last Updated:** 2026-04-04
**Pipeline Status:** COMPLETE (10-stage architecture, 245+ supporting Python utilities)
**Current Use:** mpce-perc-001 pack (633 presets, 12,800 render jobs running)

---

## Executive Overview

The **Export Pyramid** is a three-tier architecture for XPN/XPM expansion pack creation:

```
ORIGINATE  ← User-facing desktop UI wizard (sample import)
    ↓
OUTSHINE   ← C++ DSP engine (processing: classification, pitch detection, LUFS normalization)
    ↓
OXPORT     ← Batch CLI factory (Python orchestrator, 10-stage pipeline)
```

**OXPORT** is the production-scale entry point. It orchestrates the full **SELECT → RENDER → ASSEMBLE → VALIDATE** pipeline from `.xometa` presets → render specs → WAV files → XPM programs → `.xpn` archives.

### File Inventory

**Core Pipeline**
- `oxport.py` (3,124 lines) — Main orchestrator, 10-stage runner, CLI interface
- `oxport_render.py` (1,500+ lines) — MIDI → BlackHole loopback audio capture, 24-bit WAV writer
- `xpn_render_spec.py` (1,000+ lines) — Generate render specs from .xometa presets (75+ engine strategies)
- `xpn_batch_export.py` (350+ lines) — Multi-engine batch orchestrator, parallel execution, job validation
- `Docs/specs/export-architecture.md` — High-level architecture overview

**Export Stage Runners** (xpn_*_export.py)
- `xpn_drum_export.py` (45K) — Onset drum programs (velocity/cycle/random/smart modes, 8 pads, per-voice defaults)
- `xpn_keygroup_export.py` (48K) — Keygroup programs (note-based, 4 velocity layers, loop detection, DNA-adaptive curves)

**Support Tools** (Stage 2-10 runners)
- `xpn_complement_renderer.py` (200+ lines) — Artwork collection primary/complement color variants (5 shade blends per preset)
- `xpn_bundle_builder.py` (1,437 lines) — Multi-mode preset bundling (custom/category/engine/collection)
- `xpn_kit_expander.py` — Drum kit velocity/cycle expansion (fills in missing WAV layers)
- `xpn_sample_categorizer.py` — Voice classification (kick/snare/hat/clap/tom/perc/fx/unknown)
- `xpn_qa_checker.py` — Perceptual QA checks (auto-fix clipping, phase cancellation, DC offset, silence tails)

**Configuration & Specs**
- `onset_render_spec.json` (116K) — Concrete render spec for ONSET: 20 presets, 640 total WAVs, 8 voices × 4 velocity layers
- `bundle_profiles/` — Reusable bundle configuration templates

**Archive & Utilities** (50+ additional tools)
- 10 engine-specific preset generators (xpn_obrix_preset_gen, xpn_organism_preset_gen, etc.)
- 1 Manifesto/metadata generator
- 2 DNA analysis tools (auto-DNA computation)
- 2 Report generators (QA reports, export reports, pack profilers)
- Multiple taggers, crossfaders, and variant helpers

---

## Pipeline Architecture: The 10-Stage Orchestration

### Stage 0: **Input Discovery**
- Load `.xometa` presets from `Presets/XOceanus/{mood}/{engine}/`
- Resolve engine aliases (legacy names → canonical O-word names)
- Optional preset filtering by name
- Detect Entangled mood presets (coupling data warning)

### Stage 1: **render_spec** — SELECT Phase
**What:** Generate render specifications from .xometa presets
**Inputs:** `.xometa` preset files
**Outputs:** `{preset_slug}_render_spec.json` in `build_dir/specs/`

**Engine-Specific Strategies** (75+ documented in `xpn_render_spec.py`):

| Engine | Type | Strategy | Example |
|--------|------|----------|---------|
| **ONSET** | Drum | 8 voices × 4 velocity layers = 32 WAVs | kick/snare/hat/clap/tom/perc/fx + 1 extra |
| **ODYSSEY** | Keygroup | Every minor 3rd C1-C6, 4 velocity layers = 84 WAVs | Full chromatic sampling |
| **OBESE** | Keygroup | C1-C3 bass focus, 4 velocity layers = 36 WAVs | Distortion signature preserved |
| **OBRIX** | Keygroup | C1-C5, 4 velocity layers, 2 variants (field_off/on) = 176 WAVs | JI attractor variants |
| **OPAL** | Stem | 30-60s texture renders (not pitched) | Granular textures only |
| **OPTIC** | Stem | 30s AutoPulse rhythm output (visual-only, may be silent) | Visual modulation engine |
| **OVERWORLD** | Keygroup | C1-C4 chip register, 2 velocity layers = 32 WAVs | NES/Genesis character |
| **OVERBITE** | Keygroup | C1-C4 bass focus, 4 velocity layers = 48 WAVs | Low register emphasis |
| **OCTOPUS** | Keygroup | C1-C5, 4 velocity layers, 2 arm-depth variants = 176 WAVs | Distributed intelligence variants |
| **ORCHID** (Artwork) | Keygroup | Full range, 4 layers, 5 shade variants (Primary/Tint/Tone/Shade/Pure) = 420 WAVs | Color-space morphing |

**Render Spec JSON Structure**
```json
{
  "preset_name": "Crystal Gentle",
  "preset_slug": "Crystal_Gentle",
  "engine": "Onset",
  "program_type": "drum",
  "wav_count": 32,
  "voices": ["kick", "snare", "chat", "ohat", "clap", "tom", "perc", "fx"],
  "velocity_layers": 4,
  "velocity_midi_values": [20, 50, 90, 127],
  "duration_ms": 2000,
  "wavs": [
    {"filename": "Crystal_Gentle_kick_v1.wav", "voice": "kick", "velocity_layer": 1, "midi_velocity": 20},
    ...
  ],
  "dna": {
    "brightness": 0.95,
    "warmth": 0.30,
    "movement": 0.10,
    "density": 0.30,
    "space": 0.55,
    "aggression": 0.02
  },
  "source_file": "Presets/XOceanus/Atmosphere/Onset/Crystal_Gentle.xometa",
  "coupling_warning": false
}
```

**Key Rules:**
- Velocity layers: `[20, 50, 90, 127]` (Guru Bin tuning: NOT equal ranges, musical distribution)
- Drum voices: Always in order `[kick, snare, chat, ohat, clap, tom, perc, fx]` (pad layout)
- Keygroup notes: Named as `{NOTE}` (e.g., `C2`, `F#3`, `Bb4`)
- Duration: 2000ms default (adjustable per engine)
- **Entangled presets warning:** Coupling data is lost in XPN export

---

### Stage 2: **categorize** — Voice Classification
**What:** Classify WAV samples into voice categories (kick/snare/hat/etc.)
**Inputs:** WAV files from `--wavs-dir`
**Outputs:** Category map + per-sample DNA cache

**DNA Pre-flight Analysis** (First 5 WAVs)
- Computes 6D Sonic DNA (brightness, warmth, movement, density, space, aggression)
- Outputs summary: `brightness=X.XX, warmth=X.XX, aggression=X.XX`
- Flags packs: High aggression → "aggressive" tag, Low brightness → "dark" tag

**Full DNA Scan** (All WAVs)
- Populates `ctx.sample_dna_cache[wav_stem.lower()] = {dna_dict}`
- **Legend Feature #1:** DNA cache consumed by export stage for per-sample velocity curve sculpting

**Voice Categorizer** (xpn_sample_categorizer.py)
- Classifies samples into: kick, snare, chat, ohat, clap, tom, perc, fx, unknown
- Uses audio analysis (pitch, loudness, envelope shape, transient profile)
- Output format: `{"kick": [list of paths], "snare": [...], ...}`

---

### Stage 3: **expand** — Kit Expansion (Drums Only)
**What:** Fill in missing drum kit velocity/cycle layers via interpolation/duplication
**Inputs:** Categorized WAV files (from Stage 2)
**Outputs:** Expanded `{preset_slug}/{voice}_{variant}.wav` files in `build_dir/Samples/`

**Kit Expansion Modes**

| Mode | Layer Naming | Use Case | Example |
|------|--------------|----------|---------|
| **velocity** | `{voice}_v{1-4}.wav` | Rendered at exact MIDI velocities (20/50/90/127) | Default for kick, snare, tom |
| **cycle** | `{voice}_c{1-4}.wav` | Round-robin takes (one per hit) | Prevents machine-gun effect on hats |
| **random** | `{voice}_c{1-4}.wav` | Random pick per hit (may repeat) | Organic claps/FX |
| **random-norepeat** | `{voice}_c{1-4}.wav` | Random but never same twice | Sustained FX layers |
| **smart** | Per-voice auto-select | Kick/snare/tom→velocity, hat→cycle, clap→random, fx→random-norepeat | Recommended for first packs |

**Smart Mode Defaults** (xpn_drum_export.py SMART_MODE dict)
```python
SMART_MODE = {
    "kick":  "velocity",
    "snare": "velocity",
    "chat":  "cycle",           # Closed hat: machine-gun prevention
    "ohat":  "cycle",           # Open hat: machine-gun prevention
    "clap":  "random",          # Organic feel
    "tom":   "velocity",
    "perc":  "cycle",
    "fx":    "random-norepeat", # Sustained cymbal/noise
}
```

**Interpolation Strategy** (xpn_kit_expander.py)
- If only 1 layer provided: duplicate across all 4 velocities
- If 2 layers (soft + hard): interpolate 2 intermediate layers
- If 3 layers: interpolate 1 intermediate
- If 4 layers: use as-is

**Per-Voice Physical Behavior** (hardcoded, not per-mode):
```
kick:    mono=True, polyphony=1, oneshot=True, velocity_to_pitch=0.05
snare:   velocity_to_filter=0.30
ohat:    polyphony=2 (rings until muted by chat)
clap:    polyphony=2 (stacked hits)
fx:      polyphony=4 (sustained overlapping)
```

---

### Stage 4: **qa** — Perceptual Quality Assurance
**What:** Check rendered WAV files for common issues
**Inputs:** WAV files from `build_dir/Samples/` or `--wavs-dir`
**Outputs:** QA report, optionally auto-fixed WAVs

**Check Types** (xpn_qa_checker.py)

| Check | Issue | Auto-Fix | Impact |
|-------|-------|----------|--------|
| **CLIPPING** | Peak > 1.0 (digital clipping) | No (destructive) | BLOCKING |
| **PHASE_CANCELLED** | Left/right channels cancel | No | BLOCKING |
| **DC_OFFSET** | Mean ≠ 0.0 | Yes (remove offset) | SAFE |
| **ATTACK_PRESENCE** | 0dB @ sample 0 (no fade-in) | Yes (add 1ms fade) | SAFE |
| **SILENCE_TAIL** | >0.5s silence @ end | Yes (trim + crossfade) | SAFE |

**Auto-Remediation** (--auto-fix flag)
- Only applies `AUTO_FIX_SAFE` fixes (DC_OFFSET, ATTACK_PRESENCE, SILENCE_TAIL)
- Re-checks after fix
- Logs: `[QA] Auto-fixed SILENCE_TAIL in kick_v1.wav`

**Blocking Issues** (strict-qa mode)
- CLIPPING, PHASE_CANCELLED → Job fails, operator review required

**Output**
```
    Checking 640 WAV file(s)... (auto-fix enabled)
      [PASS] 638 files
      [FAIL] 2 files
      [QA] Auto-fixed SILENCE_TAIL in snare_v4.wav
      [WARN] CLIPPING in tom_v3.wav — manual intervention required
```

---

### Stage 5: **smart_trim** — Silence Tail Auto-Trim
**What:** Remove trailing silence and add fade-out on rendered WAVs
**Inputs:** WAV files from `build_dir/Samples/`
**Outputs:** Trimmed WAV files (in-place or copy)

**Silence Detection** (--silence-threshold, default -60 dB)
- Scan from end backwards
- Find last sample above threshold
- Add crossfade window (Hann window, 50ms default)

**Benefits:**
- Reduces file sizes (especially for sustain-heavy instruments)
- Prevents click when sample ends mid-release
- Better for MPC pack loading (smaller total archive)

---

### Stage 6: **export** — XPM Program Generation
**What:** Generate `.xpm` program files (MPC-compatible XML) from categorized samples
**Inputs:** Render spec + WAV files + sample DNA cache
**Outputs:** `.xpm` program files in `build_dir/Programs/`

**Drum Programs** (xpn_drum_export.py)

**Pad Layout** (GM convention, 8 active pads)
```
Pad 0 (A1)  → MIDI note 36  (C2)   → Kick
Pad 1 (A2)  → MIDI note 38  (D2)   → Snare
Pad 2 (A3)  → MIDI note 39  (D#2)  → Clap
Pad 3 (A4)  → MIDI note 42  (F#2)  → Closed Hat (MuteGroup 1)
Pad 4 (B1)  → MIDI note 46  (A#2)  → Open Hat   (MuteGroup 1, muted by closed hat)
Pad 5 (B2)  → MIDI note 41  (F2)   → Tom
Pad 6 (B3)  → MIDI note 43  (G2)   → Percussion
Pad 7 (B4)  → MIDI note 49  (C#3)  → FX/Cymbal
```

**XPM Structure** (Akai MPC SysEx XML format)
```xml
<Program type="Drum" name="Crystal Gentle" color="#0066FF">
  <Instrument index="0" name="kick" zone_play="1" mono="true" polyphony="1" oneshot="true">
    <Layer vel_start="1"  vel_end="31"  sample="Crystal_Gentle_kick_v1.wav" />
    <Layer vel_start="32" vel_end="63"  sample="Crystal_Gentle_kick_v2.wav" />
    <Layer vel_start="64" vel_end="95"  sample="Crystal_Gentle_kick_v3.wav" />
    <Layer vel_start="96" vel_end="127" sample="Crystal_Gentle_kick_v4.wav" />
  </Instrument>
  <!-- 7 more instruments ... -->
</Program>
```

**Velocity Layer Mapping** (xpn_drum_export.py VEL_LAYERS)

| Layer | Velocity Range | Volume (Guru Bin) | Curve | Intent |
|-------|-----------------|-------------------|-------|--------|
| v1 (pp) | 1–20   | 0.30 | Musical | Ghost (barely touching) |
| v2 (mp) | 21–50  | 0.55 | Musical | Light (gentle playing) |
| v3 (mf) | 51–90  | 0.75 | Musical | Mid (expressive sweet spot) |
| v4 (ff) | 91–127 | 0.95 | Musical | Hard (full force) |

**XPM Rules (CRITICAL, from CLAUDE.md)**
```
1. KeyTrack = True  (samples transpose across zones)
2. RootNote = 0     (MPC auto-detect convention)
3. VelStart = 0     (empty layers prevent ghost triggering)
```

**Per-Voice ZonePlay Mode**
```
ZONE_PLAY = {
    "velocity":        1,  # velocity-switched layers
    "cycle":           2,  # round-robin layers
    "random":          3,  # random per hit
    "random-norepeat": 4,  # random, never repeat
}
```

**Keygroup Programs** (xpn_keygroup_export.py)

**Note Naming Convention**
```
{preset_slug}__{NOTE}__{variant}.WAV
e.g., Deep_Drift__C2__v1.WAV
      Deep_Drift__Eb3__v2.WAV  (velocity layer 2)
      Deep_Drift__F#4__c1.WAV  (cycle/round-robin take 1)
      Deep_Drift__C2__rel.WAV  (release layer, optional)
```

**Note Mapping**
- Converts note names (C2, F#3, Bb4) to MIDI numbers (C-1 = 0)
- Creates zones spanning each sample's note
- Typical range: C1-C6 (full keyboard) or C1-C5 (5-octave sampling)

**Velocity Layer Ranges** (4-layer standard)
```
v1: 1–20   (ghost)
v2: 21–50  (light)
v3: 51–90  (mid)
v4: 91–127 (hard)
```

**Per-Family Velocity Curves** (xpn_keygroup_export.py)

| Family | v1 Range | v1 Label | v4 Label | Notes |
|--------|----------|----------|----------|-------|
| Piano | 1–20 | pianissimo (barely touching) | fortissimo (full hammer) | Hammer weight axis |
| Strings | 1–25 | sul tasto (gentle) | col legno (percussive) | Bow pressure axis |
| Brass | 1–22 | warm (relaxed) | aggressive (split tone) | Lip tension axis |
| Synth | 1–20 | subthreshold | explosive | Generic musical curve |

**DNA-Adaptive Velocity Curves** (--dna-adaptive flag, Stage 6 option)
- Reads preset `.xometa` DNA (brightness, aggression, warmth, etc.)
- Sculpts velocity curve using per-sample DNA cache
- Bright presets: wider velocity range (more responsive)
- Dark presets: narrower velocity range (more control)

---

### Stage 7: **cover_art** — Procedural Cover Art Generation
**What:** Generate branded pack cover images
**Inputs:** Engine accent color, preset name, (optional) Artwork collection color variants
**Outputs:** `.png` cover images

**Cover Art Modes**

| Mode | Generator | Use Case |
|------|-----------|----------|
| **Standard** | `xpn_cover_art.py` | Single-engine cover (engine accent color) |
| **Artwork/Complement** | `xpn_complement_renderer.py` | 5-variant shade gradient (Primary/Tint/Tone/Shade/Pure) |
| **Multi-Engine** | Bundle builder | Mixed-engine pack (accent color bar) |

**Artwork Collection Shade Definitions** (xpn_complement_renderer.py)
```python
SHADE_DEFS = [
    {"name": "Primary", "blend": 0.00, "suffix": "Primary"},  # 0% complement
    {"name": "Tint",    "blend": 0.25, "suffix": "Tint"},      # 25% complement
    {"name": "Tone",    "blend": 0.50, "suffix": "Tone"},      # 50/50 blend
    {"name": "Shade",   "blend": 0.75, "suffix": "Shade"},     # 75% complement
    {"name": "Pure",    "blend": 1.00, "suffix": "Pure"},      # 100% complement
]
```

**Example: XOxblood Complement Chain**
- **Primary:** Oxblood red (#6A0D0D) — warm blood character
- **Tint (25%):** Seafoam blend — "erhu heard through shallow water"
- **Tone (50%):** True teal — equal tension (warm ↔ cold)
- **Shade (75%):** Deep teal — underwater, frequencies absorbed
- **Pure (100%):** Abyssal teal (#0D6A6A) — full inversion, submarine instrument

**Sonic Mapping** (xpn_complement_renderer.py)
- Each shade has a `sonic_shift` descriptor
- Primary: Engine at full character
- Pure: Engine tuned to opposite emotional pole (warm→cold, aggressive→serene, etc.)
- Used by DSP team to adjust filter cutoff, drive, sustain, etc. during rendering

---

### Stage 8: **complement_chain** — Artwork Collection Variants
**What:** Generate primary + complement variant XPM pairs for Artwork/Color collection engines
**Inputs:** XPM programs (from Stage 6) + cover art (from Stage 7)
**Outputs:** 5 variant programs per preset (Primary/Tint/Tone/Shade/Pure)

**Engagement Model**
- Player unlocks Primary variant (default, full engine character)
- Unlocking higher tiers (Tint → Tone → Shade → Pure) reveals new sonic territory
- **Wildcard axis:** Single continuous slider sweeps through the 5 shades

**Gating** (oxport.py ARTWORK_ENGINES set)
- Only 24 engines support Artwork collection:
  - XOxblood, XOnyx, XOchre, XOrchid (Quad 1)
  - XOttanio, XOmaomao, XOstrum, XOni (Quad 2)
  - XOpulent, XOccult, XOvation, XOverdrive (Quad 3)
  - XOrnament, XOblation, XObsession, XOther (Quad 4)
  - XOrdeal, XOutpour, XOctavo, XObjet (Quad 5)
  - XOkami, XOmni, XOdama, XOffer (Quad 6)
- Non-Artwork engines: complement_chain stage skips automatically

---

### Stage 9: **preview** — 15-Second Preview Audio
**What:** Generate short preview WAV (15 seconds) for each program
**Inputs:** Render spec + WAV files
**Outputs:** `{preset_slug}_preview.wav`

**Generation Strategy**
- Concatenate or mix samples from all voices/layers
- Trim/fade to 15 seconds
- Optional: sequence drum pattern (random 16-bar groove)

**Use Case**
- MPC UI preview on pack selection screen
- Web player on XO_OX site (aquarium creatures, Field Guide)

---

### Stage 10: **package** — XPN Archive Assembly
**What:** Bundle XPM programs + cover art + metadata into `.xpn` archive
**Inputs:** XPM files + cover art + manifest
**Outputs:** `{engine}_{version}.xpn` (ZIP-based)

**XPN Archive Structure** (Akai MPC expansion pack format)
```
my-pack-v1.0.xpn
├── Program.xpm                 # Main program file
├── Program_Tint.xpm            # (if Artwork collection)
├── Program_Tone.xpm
├── Program_Shade.xpm
├── Program_Pure.xpm
├── cover.png                   # Pack cover art
├── cover_Tint.png              # (if Artwork collection)
├── cover_Tone.png
├── cover_Shade.png
├── cover_Pure.png
├── Samples/                    # All WAV files
│   ├── kick_v1.wav
│   ├── kick_v2.wav
│   └── ...
├── metadata.json               # Pack metadata
└── manifest.txt                # File listing + checksums
```

**Metadata** (metadata.json)
```json
{
  "pack_name": "ONSET Drum Essentials",
  "version": "1.0",
  "engine": "Onset",
  "engine_color": "#0066FF",
  "preset_count": 20,
  "total_wavs": 640,
  "total_size_mb": 425,
  "created": "2026-04-01T12:34:56Z",
  "preset_names": [
    "808 Reborn", "Boom Bap OG", "Crystal Gentle", ...
  ]
}
```

**Compression**
- ZIP format (industry standard)
- Compression level: 6 (balanced speed/size)
- Sample files: Stored uncompressed (WAVs don't compress well)
- Metadata: Compressed

---

## SELECT → RENDER → ASSEMBLE → VALIDATE Loop

### SELECT Phase (Stage 1)
1. Read `.xometa` presets from disk
2. Extract engine, voices, velocity specs
3. Generate render specification JSON
4. Output: `{preset_slug}_render_spec.json`

### RENDER Phase (Stage 3, needs manual recording)
**Currently semi-automated via oxport_render.py**
1. Send MIDI notes to XOceanus plugin (BlackHole loopback on macOS)
2. Record audio output via sounddevice
3. Write 24-bit stereo WAVs to disk
4. Output: Raw WAV files → categorized → expanded

**Automation Challenges:**
- No silent sample detection / skipping in oxport_render.py
- All 633 jobs for mpce-perc-001 run in parallel (12,800 total WAVs)
- "Many silent warnings expected (unmapped voices)" — some presets may not generate audio for certain voices
- **TODO:** Implement smart voice map to detect unmapped voice routes and skip render jobs for silence-producing combinations

### ASSEMBLE Phase (Stages 6-10)
1. **Generate XPM:** Map samples → zones → velocity layers
2. **Create cover art:** Branded PNG images
3. **Add variants:** Complement chain (if Artwork engine)
4. **Generate preview:** 15-second mix
5. **Package:** Zip into `.xpn` archive

### VALIDATE Phase (Stage 4)
1. QA check on rendered WAVs
2. Structural validation on XPM XML
3. Archive integrity check

---

## Customization Options

### Kit Mode Selection
```
oxport.py run --engine Onset --kit-mode smart
```
- `smart` (recommended): Per-voice auto-selection
- `velocity`: All voices use velocity layers
- `cycle`: All voices use round-robin
- `random`: All voices use random pick
- `random-norepeat`: No repeat on random

### Velocity Curve
```
# In xpn_drum_export.py:
_set_vel_curve("musical")  # Default (Vibe-approved curve)
_set_vel_curve("even")     # Equal-width bands
```

### Auto-Fix QA Issues
```
oxport.py run --engine Onset --auto-fix --strict-qa
```
- `--auto-fix`: Automatically remove DC offset, add fade-ins, trim silence
- `--strict-qa`: Fail on clipping/phase cancellation

### DNA-Adaptive Velocity Curves
```
xpn_keygroup_export.py ... --dna-adaptive
```
- Sculpts velocity ranges based on per-sample DNA
- Bright samples: wider velocity range (more responsive)
- Dark samples: narrower range (more control)

### Per-Preset Customization
```json
{
  "preset_name": "Custom Sound",
  "velocity_layers": 2,      # Override to 2 layers instead of 4
  "duration_ms": 3000,       # 3-second render duration
  "force_kit_mode": "cycle", # Override smart mode for this preset
  "skip_complement": true,   # Skip artwork variants (if applicable)
  "coverage": "minimal"      # Render only subset of notes (C2, F2, C3)
}
```

---

## Silent Sample Detection & Handling

### Current Status
**Not yet implemented.** The feedback document `[mpce-render-progress.md](mpce-render-progress.md)` notes:

> "Many silent warnings expected (unmapped voices)."

### The Problem
1. **Optic engine** (visual-only): May produce silence in purely visual modes
2. **Unmapped voice routes:** Some presets don't connect audio to specific voices
3. **Zero-amplitude envelopes:** Preset sets amplitude to 0 for certain voices
4. **Filter cutoffs at 0 Hz:** Suppresses all output

### Potential Solutions (Not Yet Coded)

**Option A: Smart Voice Map** (recommended)
- Before rendering: analyze preset `.xometa` signal routing
- Detect muted/silent voice paths
- Skip render jobs for silence-producing voices
- Reduces job count, faster turnaround
- Estimated savings: 10-20% of render jobs (for typical packs)

**Option B: Post-Render Detection**
- After WAV is recorded, analyze RMS energy
- If RMS < -80 dB (silence threshold), flag/skip
- Re-check in QA stage, auto-delete silent files

**Option C: Voice Energy Thresholding**
- During MIDI rendering: measure peak amplitude per voice
- If peak < 0.01 (near-silence), don't record WAV
- Log as "skipped (silent)" in summary

### Recommended Approach
Combine **Option A + Option B**:
1. **Preset analysis** (smart voice map): Skip obvious silence-producers before rendering
2. **Post-render check** (energy detection): Validate no unexpected silence slipped through
3. **QA stage:** Final gate on all WAVs

---

## Render Parallelization & Job Management

### Current Fleet Architecture
- **633 presets** in mpce-perc-001 pack
- **12,800 total render jobs** (averaging 20 WAVs per preset)
- **Parallel execution:** xpn_batch_export.py with `--parallel N` flag
- **Job timeout:** 600 seconds per job (per-engine wall-clock timeout)
- **Failure handling:** `--skip-failed` flag allows batch to continue on individual job failure

### xpn_batch_export.py Features

**Whitelist Validation** (issue #430 — arg injection prevention)
```python
VALID_KIT_MODES = {"smart", "velocity", "cycle", "random", ""}
VALID_SKIP_STAGES = {"render_spec", "categorize", "expand", "qa", "smart_trim", "export", "cover_art", "complement_chain", "preview", "package"}
```

**Job Result Tracking**
```python
class JobResult:
    engine: str
    start_time, end_time: float
    exit_code: Optional[int]
    output_xpn: Optional[Path]
    stderr_tail: list[str]

    @property
    def duration(self) -> float
    @property
    def status_label(self) -> str  # "PASS", "FAIL", "RUNNING"
```

**Parallel Execution Loop**
```
1. Fill active job slots (up to --parallel N)
2. Poll running processes (enforce 600s timeout)
3. Collect stderr on completion
4. Print status summary (engine, status, duration)
5. On failure: --skip-failed=True allows batch to continue, False aborts
6. Output per-job .xpn artifact path
```

---

## File Size & Archive Optimization

### Typical Pack Sizes

| Engine | Presets | Total WAVs | Pack Size | Notes |
|--------|---------|-----------|-----------|-------|
| ONSET | 20 | 640 (8 voices × 4 vel) | 425 MB | Drums, compact |
| ODYSSEY | 15 | 315 (21 notes × 4 vel, 5 octaves) | 920 MB | Keygroup, pitched |
| ARTWORK (XOxblood) | 20 | 3,200 (640 × 5 shades) | 1.8 GB | Full Artwork collection |

### Compression Strategy
- **WAV samples:** Stored uncompressed (WAV data doesn't compress, ZIP overhead not worth it)
- **XPM/metadata/cover art:** Compressed (ZIP level 6)
- **Smart trim:** 10-15% size reduction by removing silence tails

### Caching & Reuse
- Sample DNA cache: Computed once per categorize stage, reused in export stage
- Cover art: Generated once, cached as PNG
- Complement variants: Generate from primary XPM (no re-render needed)

---

## Configuration Files & Entry Points

### Command-Line Invocation

**Single Engine, Full Pipeline**
```bash
python3 Tools/oxport.py run \
  --engine Onset \
  --wavs-dir /path/to/wavs \
  --output-dir /path/to/out \
  --kit-mode smart \
  --version 1.0
```

**Batch Export**
```bash
python3 Tools/xpn_batch_export.py \
  --config batch.json \
  --parallel 4 \
  --skip-failed
```

**Batch Config** (batch.json)
```json
{
  "batch_name": "Kitchen_Collection_v1",
  "output_base_dir": "./dist/kitchen/",
  "jobs": [
    {"engine": "OTO",     "wavs_dir": "./wavs/oto/",  "kit_mode": "velocity", "version": "1.0"},
    {"engine": "OCTAVE",  "wavs_dir": "./wavs/octave/", "kit_mode": "velocity", "version": "1.0"},
    {"engine": "OLEG",    "wavs_dir": "./wavs/oleg/",  "kit_mode": "velocity", "version": "1.0"}
  ]
}
```

### Dry-Run & Validation

```bash
# Dry run: show what would happen
python3 oxport.py run --engine Onset --output-dir /tmp/test --dry-run

# Validate presets
python3 oxport.py validate --presets --strict

# Validate pipeline output
python3 oxport.py validate --output-dir /path/to/out

# Status check
python3 oxport.py status --output-dir /path/to/out
```

---

## Error Handling & Recovery

### Auto-Fixes (--auto-fix flag)
- `DC_OFFSET`: Remove mean offset (safe)
- `ATTACK_PRESENCE`: Add 1ms fade-in (safe)
- `SILENCE_TAIL`: Trim + crossfade (safe)

### Blocking Issues (require manual intervention)
- `CLIPPING`: Peak > 1.0 (destructive, must re-render)
- `PHASE_CANCELLED`: L/R channels cancel (usually a render error)

### Retry Strategy
1. Validate input (preset JSON, WAV files exist)
2. Clear stale cached state (old DNA cache, failed XPM)
3. Re-run stage from last failure point
4. Collect new error logs for debugging

### Common Failures

| Symptom | Cause | Fix |
|---------|-------|-----|
| "No WAV files found" | --wavs-dir empty or wrong path | Verify WAV dir exists, check render completion |
| "CLIPPING in X files" | Render level too hot | Re-render at lower gain, or reduce input volume in plugin |
| "Path escape blocked" | Preset name contains `../` | Sanitize preset names (CLAUDE.md rule #563) |
| "Engine not recognized" | Typo in engine name | Check ENGINE_ALIASES dict in oxport.py |

---

## Hands-On Example: MPCE-PERC-001 Pack

### Setup
```bash
# Config batch.json
{
  "batch_name": "MPCE_Perc_001",
  "output_base_dir": "./dist/perc/",
  "jobs": [
    {"engine": "ONSET",    "wavs_dir": "./wavs/onset/",    "kit_mode": "smart", "version": "1.0"},
    {"engine": "OSTINATO", "wavs_dir": "./wavs/ostinato/", "kit_mode": "smart", "version": "1.0"},
    {"engine": "OFFERING", "wavs_dir": "./wavs/offering/", "kit_mode": "smart", "version": "1.0"}
  ]
}
```

### Execution
```bash
# Render (needs XOceanus + BlackHole running)
python3 Tools/oxport_render.py --spec Onset_render_spec.json --output-dir ./wavs/onset/

# Build packs
python3 Tools/xpn_batch_export.py --config batch.json --parallel 3

# (12,800 jobs running across 3 parallel slots)
```

### Monitoring
```bash
# Check status
python3 Tools/oxport.py status --output-dir ./dist/perc/onset/

# Validate outputs
python3 Tools/oxport.py validate --output-dir ./dist/perc/onset/

# List final artifacts
ls -lh ./dist/perc/*/*.xpn
```

### Output Tree
```
dist/perc/
├── onset/
│   ├── onset_1.0.xpn        (425 MB)
│   ├── programs/
│   ├── samples/
│   └── specs/
├── ostinato/
│   ├── ostinato_1.0.xpn     (520 MB)
│   └── ...
└── offering/
    ├── offering_1.0.xpn     (480 MB)
    └── ...
```

---

## Key Dependencies

### Python Libraries
- **mido**: MIDI output (oxport_render.py)
- **sounddevice**: Audio device I/O (oxport_render.py)
- **numpy**: WAV file I/O, DSP operations (oxport_render.py, various tools)
- **Pillow**: Image generation (xpn_cover_art.py)

### System Requirements
- **macOS:** BlackHole virtual audio interface (for loopback recording)
- **Audio DAW:** XOceanus plugin (standalone or DAW plugin)
- **Python 3.10+**

### Build System
- **CMake 3.22+:** XOceanus compilation
- **Ninja:** Build generator
- **Xcode Command Line Tools:** Compiler toolchain

---

## Summary: The Export Pyramid in Action

```
ORIGINATE (UI) ← Producer imports samples, previews with DSP
    ↓
OUTSHINE (C++ DSP) ← Processes samples: pitch detect, LUFS norm, velocity mapping
    ↓
OXPORT (Python CLI) ← Orchestrates 10-stage pipeline:
    ├─ Stage 1: render_spec (SELECT)
    ├─ Stages 2-3: categorize + expand (RENDER prep)
    ├─ Stage 4: qa (VALIDATE)
    ├─ Stage 5: smart_trim (ASSEMBLE)
    ├─ Stage 6: export (ASSEMBLE XPM)
    ├─ Stage 7: cover_art (ASSEMBLE UI)
    ├─ Stage 8: complement_chain (ASSEMBLE variants)
    ├─ Stage 9: preview (ASSEMBLE demo)
    └─ Stage 10: package (ASSEMBLE .xpn)
    ↓
[Output] ← .xpn expansion pack (ready for MPC)
```

**End-to-End Pipeline:**
- Input: `.xometa` preset files + WAV samples
- Processing: 10 stages, fully parallelizable, 245+ supporting tools
- Output: Akai MPC-compatible `.xpn` archives
- Customization: 6+ tuning options (kit mode, velocity curve, DNA adaptation, etc.)
- Status: PRODUCTION-READY (mpce-perc-001 running now)

---

## Resources

- **Main Orchestrator:** `/Tools/oxport.py` (3,124 lines)
- **Render Spec Generator:** `/Tools/xpn_render_spec.py` (1,000+ lines, 75+ engine strategies)
- **Drum Export:** `/Tools/xpn_drum_export.py` (45K)
- **Keygroup Export:** `/Tools/xpn_keygroup_export.py` (48K)
- **Batch Orchestrator:** `/Tools/xpn_batch_export.py` (350+ lines)
- **Architecture Doc:** `/Docs/specs/export-architecture.md`
- **Concrete Spec Example:** `/Tools/onset_render_spec.json` (116K, ONSET 20 presets)
- **Render Progress:** `/Docs/mpce-render-progress.md` (current status: 12,800 jobs)

---

**Prepared:** 2026-04-04
**For:** XO_OX Designs
**Re:** Full OxBuild/Oxport Export Pipeline Documentation
