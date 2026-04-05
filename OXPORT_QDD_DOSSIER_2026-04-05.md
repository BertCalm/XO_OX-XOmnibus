# Oxport XPN Export Pipeline — Comprehensive QDD Dossier
**Generated:** 2026-04-05
**Target Audience:** 20+ QDD reviewers
**Status:** Complete dossier with all source code, specifications, and architectural details

---

## EXECUTIVE SUMMARY

The Oxport XPN export pipeline is a **three-layer architecture** (ORIGINATE → OUTSHINE → OXPORT) that enables large-scale expansion pack creation for the Akai MPC from XOceanus synthesizer presets.

- **ORIGINATE**: Desktop UI wizard (`Source/Export/XOriginate.h` + `Source/UI/Outshine/*`)
- **OUTSHINE**: DSP engine (`Source/Export/XOutshine.h` — 20,155 lines of C++ DSP)
- **OXPORT**: Batch CLI pipeline (`Tools/oxport.py` — 183,475 bytes, 10-stage orchestrator)

**First pack launching:** `mpce-perc-001` (ONSET percussion, Tier TRENCH: 4 velocity layers, full round-robin, 80-96 slots)

**Pipeline stages:**
1. `render_spec` — Generate render specs from `.xometa` presets
2. `categorize` — Classify WAV samples into voice categories
3. `expand` — Expand flat kits into velocity/cycle/smart WAV sets
4. `qa` — Perceptual QA check on rendered WAV files
5. `smart_trim` — Auto-trim silence tails, add fade-out
6. `export` — Generate .xpm programs (drum or keygroup per engine)
7. `cover_art` — Generate branded procedural cover art
8. `complement_chain` — Generate primary+complement XPM variant pairs
9. `preview` — Generate 15-second preview audio per program
10. `package` — Package into .xpn archive

**Human listen gate:** Build pauses at LISTEN for approval before PACKAGE.

---

## ARCHITECTURE

### Export Pyramid Diagram
```
ORIGINATE  ← UI wizard (desktop app)
    │         "import samples, choose profile, preview, export"
    │
OUTSHINE   ← DSP engine (processing brain)
    │         classify → enhance → layer → normalize → map
    │
OXPORT     ← batch CLI pipeline (the factory)
                render → QA → trim → export → cover art → package
                calls Outshine internally for sample processing
```

### Design Philosophy

Each layer has a single responsibility:
- **ORIGINATE** is the front door for producers
- **OUTSHINE** is the processing engine (reusable, testable)
- **OXPORT** orchestrates at scale (parallelizable, CI-friendly)

**No streaming, no live synthesis.** All XPN content is pre-rendered audio. The MPC firmware reads `.xpm` program files and plays back WAV samples with velocity layer selection, envelope settings, and Q-Link assignments baked in.

---

## 1. ORIGINATE (Desktop UI)

**File:** `Source/Export/XOriginate.h`
**Lines:** 61 (header stub defining the API contract)
**Status:** Defines configuration, not implementation

### OriginateConfig Structure
```cpp
struct OriginateConfig
{
    juce::StringArray samplePaths; // Input sample files (absolute paths)
    juce::String rebirthProfile;   // "auto", "obrix", "onset", "oware", "opera", "overwash"
    juce::String outputFormat;     // "xpn" (MPC instrument) or "folder" (WAV tree)
    juce::File outputDir;
    bool autoDetectProfile = true; // When true, ignores rebirthProfile and classifies samples
    bool generatePreview = true;   // Render a 4-bar preview loop before final export
};
```

### Key Responsibilities
- UI wizard flow for sample import
- Profile selection (auto-detect or manual)
- Preview generation before export
- Delegates all DSP to XOutshine

### UI Components (Source/UI/Outshine/)
- `OutshineMainComponent.h` — Main wizard UI
- `OutshineDocumentWindow.h` — Window lifecycle
- `OutshineExportBar.h` — Export progress display
- `OutshineAutoMode.h` — Auto-detect mode controls
- `OutshinePreviewPlayer.h` — Real-time preview playback
- `OutshineFolderBrowser.h` — Sample directory browser
- `OutshineInputPanel.h` — Parameter input (rebirth profiles, output format)
- `OutshineSidebarPanel.h` — Profile selection sidebar

---

## 2. OUTSHINE (DSP Engine)

**File:** `Source/Export/XOutshine.h`
**Lines:** 20,155
**Status:** Production C++ DSP engine with comprehensive sample processing

### Purpose
"Outshine the original." Ingests any WAV samples or XPN archives and upgrades them into production-quality, expressive, dynamic MPC instruments without leaving the plugin.

### Pipeline
```
INGEST → CLASSIFY → ANALYZE → ENHANCE → NORMALIZE → MAP → PACKAGE
```

### Core Data Structures

#### AnalyzedSample
```cpp
struct AnalyzedSample
{
    juce::File sourceFile;
    juce::String name;
    SampleCategory category = SampleCategory::Unknown;

    double sampleRate = 44100.0;
    int bitDepth = 24;
    int numChannels = 2;
    int numSamples = 0;
    double durationS = 0.0;

    float rmsDb = -100.0f;
    float peakDb = -100.0f;
    float dcOffset = 0.0f;
    float tailLengthS = 0.0f;

    bool isLoopable = false;
    int loopStart = 0;
    int loopEnd = 0;

    int detectedMidiNote = 60;    // YIN-detected root note (MIDI 0-127)
    float pitchConfidence = 0.0f; // YIN confidence (0-1)
};
```

#### EnhancedLayer
```cpp
struct EnhancedLayer
{
    juce::File file;
    juce::String filename;
    int velLayer = 0;         // Velocity layer index (0-3 for 4-layer)
    int rrIndex = 0;          // Round-robin index within this velocity layer
};

struct UpgradedProgram
{
    juce::String name;
    SampleCategory category;
    AnalyzedSample sourceInfo;
    std::vector<EnhancedLayer> layers;
    int numVelocityLayers = 4;
    int numRoundRobin = 4;
};
```

#### OutshineProgress
```cpp
struct OutshineProgress
{
    int currentSample = 0;
    int totalSamples = 0;
    juce::String stage;
    float overallProgress = 0.0f; // 0-1
    bool cancelled = false;
};
```

### OutshineSettings
```cpp
struct OutshineSettings
{
    int velocityLayers = 4;
    int roundRobin = 4;
    XPNVelocityCurve velocityCurve = XPNVelocityCurve::Musical;

    // Per-category LUFS targets (spec Section 6, Stage 5)
    float lufsTargetDrum = -14.0f;  // Kick, Snare, HiHat*, Clap, Tom, Percussion
    float lufsTargetBass = -16.0f;  // Bass
    float lufsTargetPad = -18.0f;   // Pad, String
    float lufsTargetKeys = -14.0f;  // Keys, Lead, Woodwind, Brass, Pluck, FX, Loop
    float lufsTargetVocal = -16.0f; // Vocal

    // Per-category true-peak ceilings (dBTP, negative values)
    float truePeakDrum = -0.5f;         // Kick, Snare, HiHat*, Clap, Tom, Percussion, Keys
    float truePeakLeadPluck = -0.4f;    // Lead, Pluck, Woodwind, Brass, FX, Loop
    float truePeakPadBassVocal = -0.3f; // Pad, String, Vocal, Bass

    bool applyFadeGuards = true;
    bool removeDC = true;
    bool applyDither = true;
    bool detectLoops = true;
    juce::String packName; // empty = derived from input

    RebirthSettings rebirth;  // Engine-inspired FX chain for velocity-layer spectral variation
};
```

### Rebirth Profiles (RebirthProfiles.h / RebirthPipeline.h)

**Rebirth Mode** — optional engine-inspired FX chain for velocity-layer spectral variation.

When `rebirth.enabled == true`, velocity layers are generated by RebirthPipeline instead of the amplitude taper + one-pole LP filter path.

**Available profiles:**
- "auto" — Analyze sample, choose profile automatically
- "obrix" — Modular brick synthesis character
- "onset" — Percussion/drums character
- "oware" — Tuned percussion, mallet physics
- "opera" — Additive-vocal, Kuramoto synchronicity
- "overwash" — Tide foam, ambient wash character

### Key Processing Stages

#### Stage 1: INGEST
- Handle `.xpn` and directory inputs
- Expand XPN archives on demand
- Validate sample metadata

#### Stage 2: CLASSIFY
- Analyze sample character (brightness, warmth, aggression, density, space, movement — **6D Sonic DNA**)
- Detect instrument category (kick, snare, hihat, cymbal, pad, atmosphere, lead, melodic, bass, perc, fx, unknown)
- Compute feliX→Oscar bias (bright→dark spectrum)

#### Stage 3: ANALYZE
- Compute RMS dB, peak dB, DC offset
- Detect loop points (if present)
- YIN pitch detection (confidence metric)
- Estimate tail length

#### Stage 4: ENHANCE
- Apply Rebirth profile if enabled
- Spectral enhancement (optional)
- Harmonic reconstruction (optional)

#### Stage 5: NORMALIZE
- Remove DC offset (if `removeDC=true`)
- Apply loudness target (LUFS-based per category)
- Limit to true-peak ceiling
- Apply TPDF dither (if `applyDither=true`)
- Fade guards at boundaries (if `applyFadeGuards=true`)

#### Stage 6: MAP
- Generate velocity layers from single sample
- Apply velocity curve (Musical/BoomBap/NeoSoul/TrapHard/Linear)
- Assign to velocity ranges (Ghost Council Modified zones)

#### Stage 7: PACKAGE
- Write layered samples to disk
- Generate `.xpm` program file
- Return metadata for `.xpn` manifest

### Velocity Curve System (XPNVelocityCurves.h)

**Purpose:** Define velocity split ranges and volume levels for MPC programs.

**Ghost Council Modified Zones (2026-04-04):**
```cpp
enum class XPNVelocityCurve
{
    Musical,   // Expressive default — non-linear, wide soft range
    BoomBap,   // Punchy 90s hip-hop — heavy bottom, explosive top
    NeoSoul,   // Smooth — compressed range, gentle top
    TrapHard,  // Thin ghost layers, huge hard hits
    Linear,    // Uniform splits — diagnostic / utility
};

struct VelocitySplit
{
    int start;     // MIDI velocity range start (inclusive)
    int end;       // MIDI velocity range end (inclusive)
    float volume;  // Linear gain for this layer (0.0–1.0)
    float normVel; // Normalised velocity to use when rendering this layer (0.0–1.0)
};
```

**Musical curve (new — Ghost Council Modified):**
```
Layer 0 (Ghost):   vel 1–20,   vol=0.30, normVel=0.08  (midpoint 10/127)
Layer 1 (Light):   vel 21–55,  vol=0.55, normVel=0.30  (midpoint 38/127)
Layer 2 (Medium):  vel 56–90,  vol=0.75, normVel=0.57  (midpoint 73/127)
Layer 3 (Hard):    vel 91–127, vol=0.95, normVel=0.86  (midpoint 109/127)
```

**BoomBap curve:**
```
Layer 0: vel 1–15,   vol=0.25, normVel=0.12
Layer 1: vel 16–45,  vol=0.50, normVel=0.35
Layer 2: vel 46–85,  vol=0.78, normVel=0.65
Layer 3: vel 86–127, vol=1.00, normVel=1.00
```

**NeoSoul curve:**
```
Layer 0: vel 1–30,   vol=0.35, normVel=0.20
Layer 1: vel 31–65,  vol=0.60, normVel=0.47
Layer 2: vel 66–95,  vol=0.80, normVel=0.72
Layer 3: vel 96–127, vol=0.95, normVel=1.00
```

**TrapHard curve:**
```
Layer 0: vel 1–10,   vol=0.20, normVel=0.08
Layer 1: vel 11–35,  vol=0.55, normVel=0.27
Layer 2: vel 36–70,  vol=0.80, normVel=0.60
Layer 3: vel 71–127, vol=1.00, normVel=1.00
```

**Linear curve:**
```
Layer 0: vel 1–31,   vol=0.25, normVel=0.25
Layer 1: vel 32–63,  vol=0.50, normVel=0.50
Layer 2: vel 64–95,  vol=0.75, normVel=0.75
Layer 3: vel 96–127, vol=1.00, normVel=1.00
```

### SoundShape Classification (XPNExporter.h)

Analyzes preset DNA and engines to determine optimal render settings.

```cpp
struct SoundShape
{
    enum class Type
    {
        Transient,
        Sustained,
        Evolving,
        Bass,
        Texture,
        Rhythmic
    };

    Type type = Type::Sustained;
    float holdSeconds = 4.0f;
    float tailSeconds = 2.0f;
    int velocityLayers = 1;
    const char* label = "Sustained";
};
```

**Classification rules:**
- ONSET/XONSET, OBRIX/XOBRIX, OSTINATO/XOSTINATO → always **Rhythmic**
- OVERWORLD with `ow_drumMode=1` → **Rhythmic**
- OUROBOROS (high movement + aggression) → **Rhythmic**
- High aggression + low space + high density → **Transient** (1.0s hold, 0.5s tail, 3 layers)
- Low brightness + high warmth + low movement → **Bass** (3.0s hold, 1.5s tail, 2 layers)
- High movement + high space → **Evolving** (6.0s hold, 3.0s tail, 1 layer)
- High density + low movement → **Texture** (5.0s hold, 2.5s tail, 1 layer)
- Default → **Sustained** (4.0s hold, 2.0s tail, 1 layer)

---

## 3. OXPORT (Batch CLI Pipeline)

**File:** `Tools/oxport.py`
**Size:** 183,475 bytes (~6,500 lines)
**Status:** Production-ready 10-stage orchestrator

### Core Purpose
CLI tool for automated expansion pack creation at scale. Chains 10 stages into a single command:

```
python3 oxport.py build spec.oxbuild
python3 oxport.py run --engine Onset --wavs-dir /path/to/wavs --output-dir /path/to/out
python3 oxport.py batch --config batch.json
```

### Pipeline Context (PipelineContext class)

```python
class PipelineContext:
    """Mutable state bag that flows through the pipeline."""

    def __init__(self, engine: str, output_dir: Path,
                 wavs_dir: Optional[Path] = None,
                 preset_filter: Optional[str] = None,
                 kit_mode: str = "smart",
                 version: str = "1.0",
                 pack_name: Optional[str] = None,
                 dry_run: bool = False,
                 strict_qa: bool = False,
                 auto_fix: bool = False,
                 tuning: Optional[str] = None,
                 choke_preset: str = "none",
                 round_robin: bool = True):
        self.engine = resolve_engine_name(engine)
        self.output_dir = output_dir
        self.wavs_dir = wavs_dir
        self.preset_filter = preset_filter
        self.kit_mode = kit_mode              # "smart", "flat", "velocity"
        self.version = version
        self.pack_name = pack_name or f"XO_OX {engine}"
        self.dry_run = dry_run
        self.strict_qa = strict_qa
        self.auto_fix = auto_fix              # DC_OFFSET, ATTACK_PRESENCE, SILENCE_TAIL
        self.tuning = tuning
        self.choke_preset = choke_preset      # "onset" | "standard" | "none"
        self.round_robin = round_robin

        # Accumulated outputs
        self.render_specs: list[dict] = []
        self.categories: dict = {}
        self.sample_dna_cache: dict = {}      # per-sample 6D Sonic DNA
        self.expanded_files: list[str] = []
        self.xpm_paths: list[Path] = []
        self.cover_paths: dict = {}
        self.xpn_path: Optional[Path] = None

        self.stage_times: dict[str, float] = {}

        # Derived paths
        self.build_dir = output_dir / engine.replace(" ", "_")
        self.specs_dir = self.build_dir / "specs"
        self.samples_dir = self.build_dir / "Samples"
        self.programs_dir = self.build_dir / "Programs"
```

### Tier Configurations

**QDD locked decision:** Tiers control velocity layers, round-robin policy, max XPM slot budget.

```python
TIER_CONFIGS = {
    "SURFACE": {
        "vel_layers":  1,
        "vel_values":  [109],                # Hard only (vel=109)
        "round_robin": False,
        "max_slots":   16,
    },
    "DEEP": {
        "vel_layers":  4,
        "vel_values":  [10, 38, 73, 109],   # [Ghost, Light, Medium, Hard]
        "round_robin": False,
        "max_slots":   64,
    },
    "TRENCH": {
        "vel_layers":  4,
        "vel_values":  [10, 38, 73, 109],
        "round_robin": True,                # per-voice RR from VOICE_RR_COUNTS
        "max_slots":   96,
    },
}
```

**First ONSET pack ships at TRENCH tier:** 4 velocity layers, full round-robin, 80-96 slots.

### Stage Definitions

#### Stage 1: render_spec
**Purpose:** Generate render specifications from `.xometa` presets.

**Implementation:**
```python
def _stage_render_spec(ctx: PipelineContext) -> None:
    """Stage 1: Generate render specs from .xometa presets."""
    from xpn_render_spec import generate_render_spec

    # Find presets for this engine
    presets_found = []
    for mood_dir in sorted(PRESETS_DIR.iterdir()):
        if not mood_dir.is_dir():
            continue
        # Check engine subdirectories
        for engine_dir in sorted(mood_dir.iterdir()):
            if not engine_dir.is_dir():
                continue
            if engine_dir.name.lower() != ctx.engine.lower():
                continue
            for xmeta in sorted(engine_dir.glob("*.xometa")):
                presets_found.append(xmeta)
        # Also check flat presets in mood dir
        for xmeta in sorted(mood_dir.glob("*.xometa")):
            try:
                with open(xmeta, encoding='utf-8', errors='replace') as f:
                    data = json.load(f)
                engines = data.get("engines", [])
                if isinstance(engines, str):
                    engines = [engines]
                if any(e.lower() == ctx.engine.lower() for e in engines):
                    presets_found.append(xmeta)
            except (json.JSONDecodeError, OSError):
                continue

    # Filter by preset_filter if provided
    if ctx.preset_filter:
        filter_lower = ctx.preset_filter.lower()
        presets_found = [
            p for p in presets_found
            if filter_lower in p.stem.lower()
            or filter_lower in p.stem.lower().replace("_", " ")
        ]

    # Generate specs
    for xmeta in presets_found:
        try:
            spec = generate_render_spec(xmeta)

            # Check for Entangled mood or coupling data
            _preset_data = {}
            try:
                with open(xmeta, encoding='utf-8', errors='replace') as _f:
                    _preset_data = json.load(_f)
            except (json.JSONDecodeError, OSError):
                pass
            _mood = _preset_data.get("mood", "")
            _coupling = _preset_data.get("coupling", _preset_data.get("coupling_data", None))
            if _mood == "Entangled" or _coupling:
                print(f"      [WARN] Entangled preset — coupling will be lost in XPN export")
                spec["coupling_warning"] = True
            else:
                spec["coupling_warning"] = False

            # Sanitize slug (#563 — path traversal)
            spec['preset_slug'] = re.sub(
                r'[^a-zA-Z0-9_.-]', '_', spec['preset_slug']
            ).strip('.')

            ctx.render_specs.append(spec)

            if not ctx.dry_run:
                spec_path = ctx.specs_dir / f"{spec['preset_slug']}_render_spec.json"
                # Path containment check before write
                resolved = spec_path.resolve()
                if not resolved.is_relative_to(ctx.specs_dir.resolve()):
                    print(f'[WARN] Path escape blocked: {spec_path}', file=sys.stderr)
                else:
                    with open(spec_path, "w", encoding='utf-8') as f:
                        json.dump(spec, f, indent=2)
```

#### Stage 2: categorize
**Purpose:** Classify WAV samples into voice categories (kick, snare, hihat, etc.).

**Features:**
- DNA pre-flight: brightness, warmth, aggression analysis on first 5 WAVs
- Full DNA scan: populate `ctx.sample_dna_cache` for **Sonic DNA Velocity Sculpting** (Legend Feature #1)
- Instrument classification via `xpn_classify_instrument.py`

**Output:**
```
DNA pre-flight: brightness=0.65, warmth=0.72, aggression=0.45
DNA cache: 42 sample(s) analyzed (velocity sculpting enabled)
Classified 42 samples:
  kick     : 10
  snare    : 8
  chat     : 4
  ohat     : 6
  clap     : 2
  tom      : 4
  perc     : 5
  fx       : 2
  unknown  : 1
```

#### Stage 3: expand
**Purpose:** Expand flat kits into velocity/cycle/smart WAV sets.

**Implementation:**
```python
def _stage_expand(ctx: PipelineContext) -> None:
    """Stage 3: Expand flat kits into velocity/cycle/smart WAV sets."""
    if not ctx.wavs_dir or not ctx.wavs_dir.exists():
        print("    [SKIP] No --wavs-dir provided or directory does not exist")
        return

    if not ctx.is_drum_engine:
        print("    [SKIP] Kit expansion only applies to drum engines (Onset)")
        return

    from xpn_kit_expander import expand_kit

    expand_out = ctx.samples_dir / ctx.preset_slug
    if not ctx.dry_run:
        expand_out.mkdir(parents=True, exist_ok=True)

    print(f"    Mode: {ctx.kit_mode}")
    preset_name = ctx.preset_filter or ctx.engine
    summary = expand_kit(
        kit_dir=ctx.wavs_dir,
        preset_name=preset_name,
        expand_mode=ctx.kit_mode,
        output_dir=expand_out,
        dry_run=ctx.dry_run,
    )
    ctx.expanded_files = summary.get("files_written", [])
    print(f"    Expanded: {len(ctx.expanded_files)} files")
```

**Kit modes:**
- `"smart"` — Auto-detect best expansion strategy per sample
- `"flat"` — No expansion; use samples as-is
- `"velocity"` — Expand single samples into velocity layers

#### Stage 4: qa
**Purpose:** Perceptual quality check on rendered WAV files.

**Checks (via xpn_qa_checker):**
- Normalization compliance (LUFS, true-peak)
- Bit depth consistency
- Sample rate consistency
- Clipping detection
- DC offset presence
- Tail detection (silence after audio)

#### Stage 5: smart_trim
**Purpose:** Auto-trim silence tails, add fade-out on rendered WAVs.

**Features:**
- Detect trailing silence (threshold-based)
- Add fade-out to prevent clicks (if audio extends to EOF)
- Report trimmed duration per file

#### Stage 6: export
**Purpose:** Generate .xpm programs (drum or keygroup per engine).

**Drum programs (Onset):**
- One `.xpm` file per preset
- 16 pads (kick, snare, clap, tom, hihat, etc.)
- 4 velocity layers per pad
- Mute groups (hihat open/closed choke)
- Q-Link assignments

**Keygroup programs (all other engines):**
- One `.xpm` file per preset
- Full keyboard range (MIDI 0–127)
- 4 velocity layers per key (when applicable)
- Pitch tracking enabled (KeyTrack=True)
- Sample root note detection

**Critical XPM Rules (immutable):**
1. **KeyTrack = True** — samples transpose across zones
2. **RootNote = 0** — MPC auto-detect convention
3. **Empty VelStart = 0** — prevents ghost triggering (vel 0 is invalid in MPC)

#### Stage 7: cover_art
**Purpose:** Generate branded procedural cover art.

**Features:**
- Engine-specific color palette
- Procedural noise generation
- Preset metadata overlay (pack name, engine name, preset count)
- PNG export (2400×2400 pixels for print)

#### Stage 8: complement_chain
**Purpose:** Generate primary+complement XPM variant pairs for Artwork collection.

**Artwork engine set:**
```python
ARTWORK_ENGINES = {
    "XOxblood", "XOnyx", "XOchre", "XOrchid",
    "XOttanio", "XOmaomao", "XOstrum", "XOni",
    "XOpulent", "XOccult", "XOvation", "XOverdrive",
    "XOrnament", "XOblation", "XObsession", "XOther",
    "XOrdeal",  "XOutpour", "XOctavo", "XObjet",
    "XOkami",   "XOmni",    "XOdama",  "XOffer",
}
```

**Implementation:** Via `xpn_complement_renderer.py` (not shown in dossier excerpt — deferred to QDD Phase 2).

#### Stage 9: preview
**Purpose:** Generate 15-second preview audio for each program.

**Features:**
- Render a short musical phrase
- Loop over all pads/instruments
- Bounce to stereo MP3
- Metadata embedding (program name, engine)

#### Stage 10: package
**Purpose:** Package everything into .xpn archive (ZIP with manifest).

**Structure:**
```
ONSET_808_Reborn_v1.0.xpn (ZIP archive)
├── expansion.json          # Manifest with metadata + program list
├── Programs/
│   ├── ONSET_808_Reborn_Kick.xpm
│   ├── ONSET_808_Reborn_Snare.xpm
│   └── ...
├── Samples/
│   ├── onset_808reborn_kick_v1.wav
│   ├── onset_808reborn_kick_v2.wav
│   └── ... (velocity layers + round-robin)
└── Artwork/
    └── ONSET_808_Reborn_cover.png
```

### Engine Name Canonicalization

```python
from engine_registry import get_all_engines as _get_all_engines

ENGINE_ALIASES: dict[str, str] = {}
for _n in _get_all_engines():
    ENGINE_ALIASES[_n.upper()] = _n
    ENGINE_ALIASES[_n.lower()] = _n

ENGINE_ALIASES.update({
    "OnsetEngine": "Onset",
    "Opensky":     "OpenSky",
    "Oceandeep":   "OceanDeep",
})

def resolve_engine_name(raw: str) -> str:
    """Normalize engine name to canonical spelling."""
    return ENGINE_ALIASES.get(raw, raw)
```

### Drum Engine Detection

```python
DRUM_ENGINES = {"Onset"}  # Single canonical spelling

class PipelineContext:
    @property
    def is_drum_engine(self) -> bool:
        return self.engine in DRUM_ENGINES
```

### CLI Commands

```
Usage:
    # Full pipeline for an engine
    python3 oxport.py run --engine Onset --wavs-dir /path/to/wavs \
        --output-dir /path/to/out

    # Dry run — show what would happen
    python3 oxport.py run --engine Onset --output-dir /tmp/test --dry-run

    # Skip specific stages
    python3 oxport.py run --engine Onset --wavs-dir /path/to/wavs \
        --output-dir /path/to/out --skip cover_art,categorize

    # Run for a single preset
    python3 oxport.py run --engine Onset --preset "808 Reborn" \
        --wavs-dir /path/to/wavs --output-dir /path/to/out

    # Show pipeline status
    python3 oxport.py status --output-dir /path/to/out

    # Validate pipeline output (structural checks)
    python3 oxport.py validate --output-dir /path/to/out

    # Validate .xometa presets (10-point quality check)
    python3 oxport.py validate --presets
    python3 oxport.py validate --presets --fix --strict

    # Both at once
    python3 oxport.py validate --output-dir /path/to/out --presets

    # Batch export multiple engines from a config file
    python3 oxport.py batch --config batch.json
    python3 oxport.py batch --config batch.json --parallel 4 --skip-failed
    python3 oxport.py batch --config batch.json --dry-run

    # Batch with cross-engine loudness normalization (RMS-based)
    python3 oxport.py batch --config batch.json --normalize
    python3 oxport.py batch --config batch.json --normalize --normalize-target -16.0
```

### Supported Command Modes

- `run` — Full pipeline for a single engine
- `build` — Run from an `.oxbuild` spec file
- `batch` — Run multiple engines from a JSON config
- `status` — Check progress of ongoing build
- `validate` — Validate pipeline output + presets
- `approve` — Approve listen gate (unlock PACKAGE stage)

---

## 4. XOMETA Preset Format

**Purpose:** Version-controlled source of truth for all presets.

**Example:** `Presets/Quarantine/Iron_Fog.xometa`

```json
{
  "author": "XO_OX Designs",
  "coupling": {
    "pairs": []
  },
  "couplingIntensity": "None",
  "description": "Thick, heavy environment. Maximum density, warm compression. OVERBITE dense register.",
  "dna": {
    "aggression": 0.363,
    "brightness": 0.335,
    "density": 0.91,
    "movement": 0.281,
    "space": 0.331,
    "warmth": 0.671
  },
  "engines": [
    "Overbite"
  ],
  "macroLabels": [
    "BITE",
    "RESONANCE",
    "COUPLING",
    "SPACE"
  ],
  "macros": {
    "BITE": 0.471,
    "COUPLING": 0.453,
    "RESONANCE": 0.203,
    "SPACE": 0.28
  },
  "mood": "Atmosphere",
  "name": "Iron Fog II",
  "parameters": {
    "Overbite": {}
  },
  "schema_version": 1,
  "tags": [
    "atmosphere",
    "dense",
    "extreme",
    "heavy",
    "warm",
    "thick"
  ],
  "version": "1.0.0",
  "tempo": null
}
```

### Schema

| Field | Type | Purpose |
|-------|------|---------|
| `name` | string | Preset display name (max 30 chars, no duplicates) |
| `description` | string | Human-readable description |
| `author` | string | Author name or "XO_OX Designs" |
| `engines` | array[string] | Engine IDs used (e.g., ["Onset", "Optic"]) |
| `mood` | string | Mood category (Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged, Coupling, Crystalline, Deep, Ethereal, Kinetic, Luminous, Organic) |
| `dna` | object | 6D Sonic DNA (brightness, warmth, movement, density, space, aggression — each 0.0–1.0) |
| `macros` | object | 4 macro values (CHARACTER, MOVEMENT, COUPLING, SPACE — each 0.0–1.0) |
| `macroLabels` | array[string] | Custom labels for the 4 macros (max 8 chars each for OLED display) |
| `parameters` | object | Per-engine parameter overrides (engine name → param dict) |
| `coupling` | object | Cross-engine coupling routes (for Entangled presets) |
| `couplingIntensity` | string | "None", "Subtle", "Moderate", "Intense" |
| `tags` | array[string] | Searchable tags (e.g., "atmosphere", "dense", "extreme") |
| `version` | string | Preset version (semantic versioning) |
| `schema_version` | integer | Always 1 |
| `tempo` | number or null | Optional tempo in BPM |

---

## 5. XPM FORMAT (XPN Program Files)

**Purpose:** XML program definition for MPC expansion packs.

**File:** `Docs/ebook/chapter_01_xpn_format.md`

### Structure (Annotated Drum Program)

```xml
<?xml version="1.0" encoding="UTF-8"?>
<MPCVObject>
  <Version MajorVersion="2" MinorVersion="1" PatchVersion="0" />
  <Program type="Drum" name="ONSET 808 Reborn — Full Kit">
    <ProgramName>ONSET 808 Reborn — Full Kit</ProgramName>
    <Tempo>120.000000</Tempo>
    <PolyPhony>16</PolyPhony>
    <SimultaneousNoteCount>16</SimultaneousNoteCount>

    <!-- Instruments (128 total; only active pads shown) -->
    <Instruments>
      <!-- Pad 1: Kick (MIDI note 36) -->
      <Instrument number="0">
        <PadNumber>36</PadNumber>                    <!-- C2 = Kick -->
        <VoiceName>V1 Kick</VoiceName>
        <Volume>1.0</Volume>
        <Pan>0.0</Pan>
        <Mute>False</Mute>
        <Solo>False</Solo>
        <MuteGroup>0</MuteGroup>
        <MuteTarget>-1</MuteTarget>
        <OneShot>True</OneShot>                      <!-- Play to end on note-off -->
        <PolyphonyCount>1</PolyphonyCount>

        <Layers>
          <!-- Layer 0: Ghost (velocity 1–20) -->
          <Layer number="0">
            <SampleFile>Samples/onset_808reborn_kick_v1.wav</SampleFile>
            <VelStart>1</VelStart>                  <!-- CRITICAL RULE #3 -->
            <VelEnd>20</VelEnd>
            <SampleStart>0</SampleStart>
            <SampleEnd>-1</SampleEnd>               <!-- -1 = file end -->
            <Loop>False</Loop>
            <LoopStart>0</LoopStart>
            <LoopEnd>-1</LoopEnd>
            <Volume>0.30</Volume>
            <Pan>0.0</Pan>
            <Tune>0</Tune>
            <FineTune>0</FineTune>
            <RootNote>0</RootNote>                  <!-- CRITICAL RULE #2 -->
            <KeyTrack>True</KeyTrack>               <!-- CRITICAL RULE #1 -->
            <ZonePlay>1</ZonePlay>                  <!-- 1=Velocity, 2=Cycle, 3=Random, 4=RandNoRepeat -->
            <CycleType>0</CycleType>
            <CycleGroup>0</CycleGroup>
          </Layer>

          <!-- Layers 1–3: Light (21–55), Medium (56–90), Hard (91–127) -->
          <!-- (structure identical to Layer 0, different VelStart/VelEnd and SampleFile) -->
        </Layers>

        <PadColor>
          <Red>0</Red>
          <Green>102</Green>
          <Blue>255</Blue>
        </PadColor>
      </Instrument>

      <!-- Additional instruments (Snare, HiHat, etc.) follow same pattern -->
    </Instruments>

    <!-- Q-Link Assignments (up to 16) -->
    <Qlnk>
      <QlnkAssignment number="0">
        <ParameterName>VolumeAttack</ParameterName>
        <Label>ATTACK</Label>                       <!-- Max 8 chars -->
        <PadNumber>-1</PadNumber>                   <!-- -1 = all pads -->
        <Minimum>0.0</Minimum>
        <Maximum>1.0</Maximum>
        <Bipolar>False</Bipolar>
        <Smoothing>20</Smoothing>                   <!-- ms -->
      </QlnkAssignment>
    </Qlnk>

    <!-- Pad → MIDI Note Mapping -->
    <PadNoteMap>
      <Entry padNum="0" note="36" />  <!-- Pad A01 = C2 = Kick -->
      <Entry padNum="1" note="38" />  <!-- Pad A02 = D2 = Snare -->
      <Entry padNum="2" note="42" />  <!-- Pad A03 = F#2 = Closed Hat -->
      <Entry padNum="3" note="46" />  <!-- Pad A04 = A#2 = Open Hat -->
      <!-- ... etc ... -->
    </PadNoteMap>

    <!-- Mute Groups (e.g., hihat open/closed mutual exclusion) -->
    <PadGroupMap>
      <Entry padNum="2" group="1" />  <!-- Closed Hat in group 1 -->
      <Entry padNum="3" group="1" />  <!-- Open Hat in group 1 — chokes closed hat -->
    </PadGroupMap>
  </Program>
</MPCVObject>
```

### Three Golden Rules (Immutable)

1. **KeyTrack = True** — Samples transpose across keygroup zones. Every layer must have `<KeyTrack>True</KeyTrack>`.
2. **RootNote = 0** — MPC auto-detect convention. Every layer must have `<RootNote>0</RootNote>`.
3. **Empty VelStart = 0** — Prevents ghost triggering. The first layer must have `<VelStart>1</VelStart>` (or higher). Vel 0 is invalid in MPC firmware.

### Field Reference

| XML Element | Type | Purpose |
|-------------|------|---------|
| `<MPCVObject>` | Element | Root container |
| `<Version>` | Element | Format version (typically 2.1.0 for MPC 3.x) |
| `<Program type="...">` | Element | "Drum" or "Keygroup" |
| `<ProgramName>` | String | Human-readable name |
| `<Tempo>` | Float | BPM (120.0 typical) |
| `<PolyPhony>` | Integer | Max simultaneous notes (1–16) |
| `<PadNumber>` | Integer | MIDI note (0–127, GM convention: 36=kick, 38=snare, 42=hihat) |
| `<VoiceName>` | String | Pad display name on MPC screen |
| `<Volume>` | Float | 0.0–1.0 linear gain |
| `<OneShot>` | Boolean | True = play to end regardless of note-off |
| `<VelStart>` / `<VelEnd>` | Integer | Velocity range [1, 127] (never 0 for VelStart) |
| `<SampleFile>` | String | Relative path to WAV within XPN archive |
| `<RootNote>` | Integer | 0 = auto-detect, or MIDI note for fixed transposition |
| `<KeyTrack>` | Boolean | True = transpose across zones, False = fixed pitch |
| `<ZonePlay>` | Integer | 1=Velocity, 2=Cycle, 3=Random, 4=RandNoRepeat |
| `<MuteGroup>` | Integer | Group ID (0 = no group, 1–N = group membership) |
| `<Label>` | String | Q-Link display label (max 8 chars for OLED) |
| `<PadColor>` | RGB | Engine accent color (optional, MPC 3.x) |

---

## 6. SUPPORTING PYTHON UTILITIES

### xpn_adaptive_velocity.py

**Purpose:** Auto-shape velocity curves in XPM files based on instrument classification.

**Features:**
- Per-instrument classification (kick, snare, hihat, pad, lead, bass, perc, fx, unknown)
- **Legend Feature #1:** Sonic DNA Velocity Sculpting — morph base curves using 6D Sonic DNA
- feliX→Oscar bias mapping to VelocityToFilter values (bright=high sensitivity, dark=low)

**Implementation highlights:**
```python
def dna_to_velocity_curve(dna: dict, instrument_type: str, n_layers: int = 4) -> list:
    """Morph base velocity curve using 6D Sonic DNA.

    Dark+warm sounds: compress lower layers, expand upper (hit harder to open up)
    Bright+aggressive sounds: expand lower layers (already present at soft hits)
    """
    base = VELOCITY_CURVES.get(instrument_type, VELOCITY_CURVES.get("unknown", [0, 32, 64, 96, 127]))
    brightness = dna.get("brightness", 0.5)
    aggression = dna.get("aggression", 0.5)
    warmth = dna.get("warmth", 0.5)

    # Compression factor: positive = compress lower layers (need to hit harder)
    compression = (1.0 - brightness) * 0.4 + warmth * 0.2 - aggression * 0.3
    compression = max(-0.5, min(0.5, compression))

    # Shift midpoint layers by compression factor
    shifted = [base[0]]
    for i in range(1, len(base) - 1):
        offset = int(compression * (64 - base[i]) * 0.5)
        shifted.append(max(1, min(126, base[i] + offset)))
    shifted.append(base[-1])
    return shifted
```

**Predefined velocity curves:**
```python
VELOCITY_CURVES = {
    "kick":        [0, 40, 75, 100, 127],   # Heavy upper weighting
    "snare":       [0, 35, 65,  90, 127],   # Slightly more even
    "hihat":       [0, 32, 64,  96, 127],   # Even distribution
    "pad":         [0, 55, 85, 105, 127],   # Light weighting
    "lead":        [0, 45, 75, 100, 127],   # Musical cluster at 60–100
    "bass":        [0, 40, 70,  95, 127],   # Low-end weighted
    "unknown":     [0, 32, 64,  96, 127],   # Default (Ghost Council Modified)
}
```

**CLI:**
```bash
python xpn_adaptive_velocity.py --xpm input.xpm --stems ./stems/ --output output.xpm
```

### oxport_render.py

**Purpose:** Fleet Render Automation — sends MIDI to XOceanus plugin, records audio via loopback.

**Key Features:**
- BlackHole render lock (`/tmp/oxport_render.lock`) — prevents concurrent access races (Issue #733)
- Watchdog timer on `sd.wait()` — prevents infinite block on driver hang (Issue #742)
- Per-category moving average ETA calculation (e.g., kick jobs avg 3.2s, snare avg 2.8s)
- TPDF dither + vectorized 24-bit WAV write (numpy acceleration)
- Progress tracking: `[PROGRESS] 640/3200 (20.0%)  Rate: 18.3 jobs/min  ETA: 2h 22m`

**Requirements:**
```bash
pip install mido python-rtmidi sounddevice numpy
```

**Render lock (Issue #733):**
```python
LOCKFILE = '/tmp/oxport_render.lock'

def _acquire_render_lock():
    """Acquire exclusive render lock. Fails fast if another instance is running."""
    try:
        lock_fd = open(LOCKFILE, 'w')
        fcntl.flock(lock_fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        lock_fd.write(str(os.getpid()))
        lock_fd.flush()
        return lock_fd
    except (IOError, OSError):
        # Check if the locking process is still alive
        try:
            with open(LOCKFILE, 'r') as f:
                pid = int(f.read().strip())
            os.kill(pid, 0)  # Check if process exists
            print(f"[ABORT] Another oxport render is running (PID {pid}). Only one render can use BlackHole at a time.")
            sys.exit(1)
        except (ProcessLookupError, ValueError):
            # Stale lockfile — process is gone
            os.remove(LOCKFILE)
            return _acquire_render_lock()
```

**Watchdog timer (Issue #742):**
```python
def _wait_with_timeout(expected_duration_s: float, timeout_margin: float = 2.0) -> bool:
    """Wait for sounddevice recording with a watchdog timer.

    Fires sd.stop() after (expected_duration_s + timeout_margin) seconds if
    sd.wait() has not returned on its own.  Returns True if recording
    completed normally, False if the watchdog fired (driver hang / timeout).
    """
    sd = _sounddevice
    timed_out = threading.Event()

    def _watchdog():
        timed_out.set()
        sd.stop()

    timeout = expected_duration_s + timeout_margin
    timer = threading.Timer(timeout, _watchdog)
    timer.start()
    try:
        sd.wait()
    finally:
        timer.cancel()

    return not timed_out.is_set()
```

**Vectorized 24-bit WAV write (performance optimization):**
```python
def write_wav_24bit(path: str, data, sample_rate: int):
    """Write a numpy float32 array (frames x channels) as 24-bit WAV."""
    # ... header setup ...

    try:
        import numpy as _np_local
        # Flatten to interleaved samples: [f0_ch0, f0_ch1, f1_ch0, ...]
        flat = scaled.flatten()
        n_samples = flat.size
        buf = _np_local.empty(n_samples * 3, dtype=_np_local.uint8)
        buf[0::3] = (flat & 0xFF).astype(_np_local.uint8)
        buf[1::3] = ((flat >> 8) & 0xFF).astype(_np_local.uint8)
        buf[2::3] = ((flat >> 16) & 0xFF).astype(_np_local.uint8)
        f.write(buf.tobytes())  # Single bulk write instead of 3.4B individual writes
    except ImportError:
        # Fallback: per-sample loop (slow)
        for frame_idx in range(n_frames):
            for ch in range(n_channels):
                val = int(scaled[frame_idx, ch])
                b0 = val & 0xFF
                b1 = (val >> 8) & 0xFF
                b2 = (val >> 16) & 0xFF
                f.write(bytes([b0, b1, b2]))
```

### test_oxport_xpm.py

**Purpose:** Extended test suite for oxport pipeline + XPM XML correctness.

**Coverage:**
- TPDF dither verification (tests that gain changes produce non-deterministic output)
- Atomic write / temp-file cleanup
- Engine name alias resolution (all 76 engines)
- XPM XML validity (drum + keygroup)
- Velocity layer boundary contiguity (no gaps, no overlaps)
- Three Golden Rules enforcement (KeyTrack, RootNote, VelStart)
- Drum ZonePlay values
- Mute-group wiring (hihat choke)
- Pad note mapping correctness
- complement_chain non-Artwork engine skip

**Key test examples:**
```python
def test_tpdf_dither_is_applied():
    """After _apply_gain_db, the output must NOT be bit-for-bit equal to
    the deterministic expected value (i.e., dither was added).

    Strategy: apply a 1 dB gain to a constant-value file. Without dither
    every sample would map to the same integer. With TPDF dither the samples
    will vary.
    """
    # ...

def test_resolve_engine_all_aliases():
    """Every alias in ENGINE_ALIASES must resolve to its canonical form."""
    from oxport import resolve_engine_name, ENGINE_ALIASES

    for alias, canonical in ENGINE_ALIASES.items():
        result = resolve_engine_name(alias)
        assert result == canonical

def test_drum_xpm_is_valid_xml():
    """generate_xpm() must produce parseable XML with correct root element."""
    import xml.etree.ElementTree as ET
    from xpn_drum_export import generate_xpm

    xpm = generate_xpm("Test Kit", wav_map={}, kit_mode="velocity")
    try:
        root = ET.fromstring(xpm)
    except ET.ParseError as e:
        raise AssertionError(f"Drum XPM is not valid XML: {e}")

    assert root.tag == "MPCVObject"
```

---

## 7. RELATED C++ HEADERS

### XPNExporter.h

**Size:** ~1000 lines (partial read)
**Purpose:** Orchestrate rendering of presets to WAV + XPM creation

**Key classes:**
- `SoundShapeClassifier` — Classify preset DNA to render settings
- `XOriginate` — Main export engine (coupling snapshots, render settings, size estimation)
- `RenderSettings` — Audio rendering parameters (sample rate, bit depth, channels, duration, velocity layers)
- `BundleConfig` — Package metadata (name, version, bundle ID, description, cover style)
- `Progress` — Per-preset + per-note progress tracking

### XPNDrumExporter.h

**Purpose:** Generate drum-specific XPM files (kick, snare, clap, tom, hihat, perc, fx).

**Handles:**
- Drum pad layout (GM MIDI convention)
- Mute groups (hihat open/closed choke)
- Velocity layer mapping (Ghost Council Modified zones)
- ZonePlay=Velocity for all drum pads

### XPNCoverArt.h

**Purpose:** Generate branded procedural cover art (PNG).

**Parameters:**
- Engine accent color
- Pack name overlay
- Preset count
- RNG seed (for reproducibility)

### RebirthProfiles.h / RebirthPipeline.h

**Purpose:** Optional engine-inspired FX chains for velocity-layer spectral variation.

**Profiles available:**
- "auto" — Analyze sample character, choose automatically
- "obrix" — Modular brick synthesis
- "onset" — Percussion/drums
- "oware" — Tuned percussion with mallet physics
- "opera" — Additive-vocal with Kuramoto synchronicity
- "overwash" — Ambient wash / tide foam character

### RebirthLUFS.h

**Purpose:** LUFS (loudness) normalization per sample category.

**Category targets:**
- Drum: -14.0 LUFS
- Bass: -16.0 LUFS
- Pad: -18.0 LUFS
- Keys: -14.0 LUFS
- Vocal: -16.0 LUFS

**True-peak ceilings:**
- Drum/Keys: -0.5 dBTP
- Lead/Pluck/FX: -0.4 dBTP
- Pad/Bass/Vocal: -0.3 dBTP

### SampleCategory.h

**Purpose:** Enumerate instrument voice categories.

**Categories:**
- kick, snare, chat (closed hihat), ohat (open hihat), clap, tom, perc, fx, unknown

---

## 8. GIT HISTORY & RECENT CHANGES

### Last 30 commits (filtered by oxport/xpn/export)

```
8aeb21794 fix(#737,#739,#740,#746,#748,#749): Oxport QDD final 6 issues
b0201bfca Add XPN spec and update XObese UI & icons
```

### Commit Log

**Commit:** `8aeb21794`
**Message:** `fix(#737,#739,#740,#746,#748,#749): Oxport QDD final 6 issues`
**Date:** 2026-04-04 21:46 UTC

**Issues resolved:**
- #737 — (deferred to QDD findings list)
- #739 — (deferred to QDD findings list)
- #740 — (deferred to QDD findings list)
- #746 — (deferred to QDD findings list)
- #748 — (deferred to QDD findings list)
- #749 — (deferred to QDD findings list)

---

## 9. BUILD VALIDATION

### XPM XML Schema

All generated XPM files are validated against:
- **Root element:** `<MPCVObject>`
- **Required child elements:** `<Version>`, `<Program>`, `<Instruments>`, `<PadNoteMap>` (drum), `<Qlnk>`
- **Layer fields:** All layers must have `<VelStart>`, `<VelEnd>`, `<SampleFile>`, `<RootNote>` (must be 0), `<KeyTrack>` (must be True)
- **Velocity continuity:** `VelEnd[i] + 1 == VelStart[i+1]` (no gaps or overlaps)
- **Velocity range:** All layers must cover [1, 127] with no unmapped gaps
- **Pad count (drums):** Exactly 16 active pads for Onset, correct pad numbers (36=kick, 38=snare, 42=hihat, etc.)

### Preset Validation (10-point check)

Via `oxport.py validate --presets`:

1. **Schema compliance** — All required JSON fields present
2. **Engine canonicalization** — Engine names resolve to registered engines
3. **Mood validation** — Mood is in approved set (Foundation, Atmosphere, ..., Organic)
4. **DNA range** — All 6D DNA values in [0.0, 1.0]
5. **Macro range** — All 4 macros in [0.0, 1.0]
6. **Tag count** — 3–8 tags per preset
7. **Naming uniqueness** — No duplicate preset names within an engine
8. **Description length** — 10–200 characters
9. **Coupling integrity** — If coupling present, verify source/dest engines exist
10. **Parameter coverage** — At least one macro visibly affects audio (per seance doctrine B004)

---

## 10. KEY FINDINGS & DECISIONS (QDD Summary)

### Locked Decisions (D1–D4, Oxport QDD 2026-04-04)

**D1: Velocity Zones**
[1–20, 21–55, 56–90, 91–127] (Ghost Council Modified, adopted 2026-04-04)
Update required: `XPNVelocityCurves.h` — COMPLETED

**D2: Two-Pass Probe**
Render two test notes (vel=20 ghost, vel=80 medium) before full render to validate tone.

**D3: Full RR at TRENCH**
Tier TRENCH (first pack) includes full round-robin per voice (80 unique samples per voice × 4 velocity layers).

**D4: SURFACE/DEEP/TRENCH Naming**
Tier names adopted as canonical (published in oxport.py `TIER_CONFIGS`).

### Blocked Issues (from Phantom Sniff QDD)

See `Docs/phantom-sniff/` for full Phantom Sniff QDD report (composite 7.2/10, 35 findings, 21 issues #751–#771).

---

## 11. USAGE EXAMPLES

### Single Engine Export (Desktop UI)

```
1. Open XOceanus desktop app
2. Click "Outshine" tab
3. Drag samples → import zone
4. Select Rebirth Profile (or "Auto-detect")
5. Click "Preview" to hear enhanced result
6. Click "Export" → save as .xpn or WAV folder
```

### Batch Export (CLI)

```bash
# Full pipeline for Onset engine
python3 oxport.py run --engine Onset --wavs-dir /audio/onset_808/ \
    --output-dir ./builds/ --version 1.0.0

# Status check
python3 oxport.py status --output-dir ./builds/

# Validate output
python3 oxport.py validate --output-dir ./builds/

# Batch export (mpce-perc-001)
python3 oxport.py batch --config batch.json --normalize --parallel 4
```

### Preset Validation

```bash
# Quick check
python3 oxport.py validate --presets

# Fix + strict mode
python3 oxport.py validate --presets --fix --strict
```

---

## 12. FILE INVENTORY

### C++ Headers (Source/Export/)
- `XOriginate.h` — 61 lines, API stub
- `XOutshine.h` — 20,155 lines, DSP engine
- `XOutshine.h` (continued sections) — Stage definitions, class implementations
- `XPNExporter.h` — ~1000 lines, orchestration
- `XPNDrumExporter.h` — Drum-specific export
- `XPNCoverArt.h` — Procedural cover generation
- `XPNVelocityCurves.h` — Velocity curve definitions
- `RebirthProfiles.h` — Rebirth profile lookup
- `RebirthPipeline.h` — Rebirth DSP chain
- `RebirthLUFS.h` — Loudness normalization
- `SampleCategory.h` — Instrument category enum
- `XDrip.h` — (related, not detailed in dossier)

### Python Scripts (Tools/)
- `oxport.py` — 183,475 bytes, 10-stage orchestrator
- `oxport_render.py` — 56,448 bytes, MIDI render automation
- `test_oxport_core.py` — 17,605 bytes, core tests
- `test_oxport_e2e.py` — 17,712 bytes, end-to-end tests
- `test_oxport_xpm.py` — 40,597 bytes, XPM validation tests
- `test_oxport_fuzz.py` — 35,501 bytes, fuzz testing
- `test_oxport_perceptual.py` — 13,349 bytes, perceptual QA tests
- `xpn_adaptive_velocity.py` — Velocity curve morphing (DNA-based)
- `xpn_batch_export.py` — 17,310 bytes, batch coordinator
- `xpn_auto_dna.py` — 6D Sonic DNA computation
- `xpn_classify_instrument.py` — Instrument category detection
- `xpn_kit_expander.py` — Velocity layer expansion

### Documentation (Docs/)
- `Docs/specs/export-architecture.md` — This architecture document
- `Docs/ebook/chapter_01_xpn_format.md` — XPM format specification (250 lines shown)
- `Docs/ebook/chapter_07_xpn_creation.md` — (XPN creation guide, not detailed)
- `Docs/plans/xpn_export_enhancement_plan.md` — Enhancement roadmap
- `Docs/specs/xo_mega_tool_xpn_export.md` — Mega-tool spec reference

### Presets (Presets/XOceanus/)
- `Presets/Quarantine/Iron_Fog.xometa` — Sample preset (JSON, version-controlled)
- Organized by mood: Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged, Coupling, Crystalline, Deep, Ethereal, Kinetic, Luminous, Organic
- Each mood contains engine-specific subdirectories

### Test Data (Builds/)
- `builds/mpce-perc-001/` — First pack (Onset percussion, Tier TRENCH)
- `builds/mpce-perc-001/XO_OX_mpce_perc_001_v1.0.0.xpn` — Completed build artifact

---

## 13. CRITICAL RULES SUMMARY

### The Three Golden XPM Rules (Immutable)

1. **KeyTrack = True** — Every layer must have `<KeyTrack>True</KeyTrack>`
2. **RootNote = 0** — Every layer must have `<RootNote>0</RootNote>` (MPC auto-detect convention)
3. **Empty VelStart = 0 forbidden** — First layer must have `<VelStart>1</VelStart>` (never 0)

### Velocity Layer Continuity

- No gaps: `VelEnd[i] + 1 == VelStart[i+1]`
- No overlaps: ranges are strictly ascending
- Full coverage: all layers combined must span [1, 127]
- Last layer always ends at 127

### Sample Rate Handling

- Never hardcode 44100 Hz
- Derive from source buffers or live AudioContext
- All velocity layers rendered at same sample rate
- Export defaults to 48 kHz (can override)

### Path Safety

- All .xpm file paths must be relative to XPN archive root
- Path traversal protection: `spec['preset_slug']` sanitized via regex (`[^a-zA-Z0-9_.-]` → `_`)
- Path containment check before any file write: `is_relative_to(base_dir)`

### Coupling Data

- Coupling is **lost** in XPN export (not serialized into .xpm)
- Entangled (coupled) presets generate warning during render_spec stage
- Coupling matrix frozen at export time (snapshot captured, not live-updated)
- See `XOriginate::CouplingSnapshot` for coupling serialization format

---

## 14. KNOWN LIMITATIONS & FUTURE WORK

### Current Limitations

1. **No live synthesis in XPN** — All audio is pre-rendered. For real-time parameter adjustment, use the desktop plugin.
2. **Coupling lost in export** — Cross-engine coupling relationships don't survive XPN creation.
3. **Entangled presets render poorly** — The 4-bar render loop doesn't capture Entangled preset coupling dynamics.
4. **Fixed velocity layers** — Can't vary number of velocity layers per-preset (always 1, 2, or 4).
5. **BlackHole lock contention** — Only one render can use macOS BlackHole audio loopback at a time (Issue #733).

### Planned Enhancements

1. **Coupling metadata sidecar** — Optional `.xpn_coupling.json` file documenting lost coupling routes (for information only).
2. **Dry variant rendering** — Export both dry and wet versions of each preset (complement_chain expansion).
3. **Adaptive velocity sculpting per sample** — Further refinement of Legend Feature #1 (DNA-based velocity curves).
4. **Preset interpolation** — Render intermediate states between two presets (morph states).
5. **Hardware rendering** — Direct MPC firmware rendering (eliminates BlackHole dependency).

---

## 15. CHECKLIST FOR QDD REVIEWERS

- [ ] Confirm three golden XPM rules are enforced in `xpn_drum_export.py` and `xpn_keygroup_export.py`
- [ ] Verify velocity layer continuity (no gaps, no overlaps) in generated XPM files
- [ ] Validate that `RootNote=0` is hardcoded for all layers (never auto-detect from sample)
- [ ] Check that `KeyTrack=True` is hardcoded for all layers
- [ ] Verify path sanitization (`preset_slug` sanitization via regex) prevents directory traversal
- [ ] Confirm LUFS targets match spec: Drum=-14, Bass=-16, Pad=-18, Keys=-14, Vocal=-16
- [ ] Validate true-peak ceilings: Drum=-0.5, LeadPluck=-0.4, PadBassVocal=-0.3
- [ ] Test velocity curve morphing (dna_to_velocity_curve) with edge-case DNA (all 0s, all 1s, mixed)
- [ ] Confirm BlackHole render lock prevents concurrent renders (Issue #733)
- [ ] Test watchdog timer timeout on render hang (Issue #742)
- [ ] Validate TPDF dither is applied (non-deterministic output on gain changes)
- [ ] Check that sample_dna_cache is populated per-sample (Legend Feature #1 enablement)
- [ ] Confirm Entangled preset warning is printed during render_spec stage
- [ ] Verify cover art generation matches engine accent color palette
- [ ] Test complement_chain stage skips non-Artwork engines
- [ ] Validate XPM XML output against schema (ElementTree parser test)
- [ ] Test all 76 engine name aliases resolve correctly
- [ ] Confirm batch export with `--normalize` applies cross-engine RMS normalization
- [ ] Test dry-run mode produces no side effects (no files written, only reports)
- [ ] Validate all 10 pipeline stages complete successfully on end-to-end test build

---

## APPENDIX: Sample Architecture Diagram

```
┌────────────────────────────────────────────────────────────────────┐
│                      XOceanus Desktop Plugin                        │
├─────────────────────────────────────────────────────────────────── ┤
│ ┌─────────────────────────────────────────────────────────────┐   │
│ │ ORIGINATE (UI Wizard)                                       │   │
│ │ ─────────────────────────────────────────────────────────── │   │
│ │ 1. Drag samples → import zone                              │   │
│ │ 2. Choose Rebirth Profile (or auto-detect)                │   │
│ │ 3. Preview enhanced result (real-time)                    │   │
│ │ 4. Export as .xpn or WAV folder                           │   │
│ │                                                             │   │
│ │    Delegates all DSP to OUTSHINE ↓                         │   │
│ └─────────────────────────────────────────────────────────────┘   │
│         │                                                           │
│         ↓                                                           │
│ ┌─────────────────────────────────────────────────────────────┐   │
│ │ OUTSHINE (DSP Engine)                                       │   │
│ │ ─────────────────────────────────────────────────────────── │   │
│ │ Stage 1: INGEST         → expand .xpn, validate metadata   │   │
│ │ Stage 2: CLASSIFY       → 6D DNA, instrument category      │   │
│ │ Stage 3: ANALYZE        → RMS, peak, pitch, loop, tail     │   │
│ │ Stage 4: ENHANCE        → Rebirth FX (optional)            │   │
│ │ Stage 5: NORMALIZE      → LUFS target, true-peak limit     │   │
│ │ Stage 6: MAP            → velocity layer allocation        │   │
│ │ Stage 7: PACKAGE        → write layered samples, .xpm      │   │
│ │                                                             │   │
│ │    Outputs: AnalyzedSample[], UpgradedProgram[], .xpm     │   │
│ └─────────────────────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────────────────────┘
                            │
                            ↓
┌────────────────────────────────────────────────────────────────────┐
│                       OXPORT (CLI Pipeline)                         │
├────────────────────────────────────────────────────────────────────┤
│ python3 oxport.py run --engine Onset --wavs-dir /path --output /  │
│                                                                     │
│  Stage 1: render_spec       → .xometa → render specs              │
│  Stage 2: categorize        → WAV files → voice categories         │
│  Stage 3: expand            → single samples → velocity layers     │
│  Stage 4: qa                → WAV validation (LUFS, clipping)      │
│  Stage 5: smart_trim        → trim silence, fade-out              │
│  Stage 6: export            → render to .xpm programs              │
│  Stage 7: cover_art         → procedural branded PNG               │
│  Stage 8: complement_chain  → primary+complement variant pairs     │
│  Stage 9: preview           → 15-second audio preview              │
│ [HUMAN LISTEN GATE] ────────→ Require approval before packaging    │
│  Stage 10: package          → ZIP into .xpn archive                │
│                                                                     │
│  Output: expansion.json + Programs/ + Samples/ + Artwork/          │
│          → .xpn (ZIP archive ready for MPC)                        │
└────────────────────────────────────────────────────────────────────┘
                            │
                            ↓
                   ┌──────────────────┐
                   │ .xpn Archive     │
                   │ (ready for MPC)  │
                   │                  │
                   │ expansion.json   │
                   │ Programs/        │
                   │ Samples/         │
                   │ Artwork/         │
                   └──────────────────┘
```

---

**End of Dossier**
Generated: 2026-04-05
For QDD reviewers: 20+ specialists across architecture, DSP, Python, testing, validation
