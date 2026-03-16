# Fleet Render Automation -- Technical Specification

**Status:** Design Document (Pre-Implementation)
**Date:** 2026-03-15
**Author:** XO_OX Designs
**Scope:** Automated offline rendering of all XOmnibus presets into WAV samples for the Oxport XPN pipeline

---

## 1. Problem Statement

XOmnibus has 2,451 factory presets across 34 engines. To create MPC-compatible XPN expansion packs, each preset must be rendered as WAV samples at specific notes, velocity layers, and variant positions. The `xpn_render_spec.py` tool already generates the exact render specifications -- what notes, velocities, and variants to capture per engine. But the actual rendering is manual: load a preset, play each note, record the output, save WAVs.

For the full fleet, the render spec generates approximately **250,000+ individual WAV files**. At 2 minutes per sample (load, play, record, save, name), that is 8,000+ hours of manual work. Fleet Render Automation eliminates this bottleneck by rendering programmatically.

---

## 2. Architecture Options

### Option A: JUCE Headless CLI (Recommended)

**Approach:** Build a headless command-line target into the XOmnibus CMake project. This target links the same engine code but has no UI. It instantiates `XOmnibusProcessor` directly, loads presets via `PresetManager`, sends MIDI, and renders to `juce::AudioBuffer<float>` using the processor's `processBlock()` in a tight loop.

**The groundwork already exists.** `XPNExporter` in `Source/Export/XPNExporter.h` already has:
- `renderNoteToWav()` -- currently a placeholder that produces silence, but the scaffolding (buffer allocation, hold/tail timing, WAV writing, normalization) is complete
- `SoundShapeClassifier` -- classifies presets into 6 render profiles (Transient/Sustained/Evolving/Bass/Texture/Rhythmic) with appropriate hold and tail durations
- Parallel batch rendering with `std::async` and `std::atomic` progress tracking
- XPM keygroup program generation with all 3 critical MPC rules enforced
- Size estimation, validation, and atomic export (write to temp dir, rename on success)

The missing piece is lines 602-616 of `XPNExporter.h` -- the `renderNoteToWav()` method currently generates silence instead of actually processing audio through the engine. The implementation path:

1. Create a temporary `XOmnibusProcessor` instance per worker thread
2. Call `prepareToPlay(sampleRate, blockSize)` on it
3. Call `applyPreset(presetData)` to load engines + parameters + coupling routes
4. Send `noteOn` via `juce::MidiBuffer`, call `processBlock()` in a loop for `holdSamples` blocks
5. Send `noteOff`, continue calling `processBlock()` for `tailSamples` blocks
6. The rendered `AudioBuffer<float>` is already written to WAV by the existing code

**Evaluation:**

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Implementation effort | **Low** | 80% of the code exists in `XPNExporter.h`. Fill in `renderNoteToWav()` (~40 lines), add a CLI `main()` (~60 lines) |
| Audio quality | **Perfect** | Identical to live playback -- same code path, same DSP |
| Reliability | **High** | No external dependencies, no IPC, no DAW quirks |
| Preset loading | **Full** | `applyPreset()` handles engines, parameters, coupling, macros |
| Parameter automation | **Full** | Direct `apvts` access for variant rendering (e.g., set BOND macro to 0.5) |
| Speed | **Faster than realtime** | `processBlock()` in a tight loop with no audio I/O overhead. ~10-50x realtime depending on engine complexity. Parallel across CPU cores. |
| Coupling support | **Full** | `MegaCouplingMatrix` runs inside `processBlock()` -- coupled presets render correctly |

### Option B: MIDI/OSC DAW Automation

**Approach:** Run XOmnibus as an AU/VST3 in a DAW (or standalone). Use a Python script to send MIDI notes via virtual MIDI port, load presets via OSC or MIDI program change, and capture audio output via loopback or DAW bounce.

**Evaluation:**

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Implementation effort | **High** | Need OSC server in plugin, MIDI routing, audio capture, DAW scripting |
| Audio quality | **Perfect** | Same plugin running live |
| Reliability | **Low** | DAW-dependent, audio routing fragile, race conditions between MIDI send and audio capture |
| Preset loading | **Partial** | No standard protocol for loading arbitrary `.xometa` presets via MIDI/OSC. Would need custom OSC handler. |
| Parameter automation | **Partial** | MIDI CC covers 128 params max; OSC would need full implementation |
| Speed | **1x realtime** | Must play in realtime. 250K samples at ~5s each = 347 hours |
| Coupling support | **Full** | Plugin runs normally |

**Verdict: Rejected.** Realtime constraint makes this 300x slower than Option A. Fragile external dependencies. No existing OSC infrastructure in XOmnibus.

### Option C: Python Plugin Host (pedalboard / DawDreamer)

**Approach:** Use Spotify's `pedalboard` or DawDreamer to load XOmnibus as a VST3 in Python, send MIDI programmatically, and capture rendered audio as numpy arrays.

**Evaluation:**

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Implementation effort | **Medium** | Python host code is simple. But preset loading requires VST3 state restoration via `setStateInformation()` -- must serialize `.xometa` to the plugin's binary state format. |
| Audio quality | **Perfect** | Same plugin binary |
| Reliability | **Medium** | Plugin host libraries can crash on edge cases. DawDreamer more battle-tested for offline rendering than pedalboard. |
| Preset loading | **Complex** | VST3 state blobs are opaque. Must either: (a) implement `setStateInformation()` parsing from Python, or (b) load presets by manipulating the plugin's parameter tree, which requires knowing every parameter ID. |
| Parameter automation | **Full** | Both libraries support setting VST3 parameters by index or name |
| Speed | **Faster than realtime** | DawDreamer renders offline. Comparable to Option A. |
| Coupling support | **Uncertain** | Coupling routes are set up during preset loading. If state restore works, coupling works. If using parameter-by-parameter approach, coupling setup is manual. |

**Verdict: Viable fallback.** Useful if someone wants to render from the built plugin binary without recompiling. But Option A is strictly better for an integrated pipeline -- same speed, simpler preset loading, no external dependency on Python plugin host libraries.

### Decision: Option A (JUCE Headless CLI)

Option A leverages 80% existing code, runs faster than realtime, handles all edge cases (coupling, variants, sound shapes), and requires no external dependencies. The `XPNExporter` was clearly designed with this in mind -- the placeholder in `renderNoteToWav()` is the only gap.

---

## 3. Render Pipeline Design

### 3.1 Pipeline Stages

```
.xometa presets
      |
      v
[xpn_render_spec.py] --all --json
      |
      v
  render_specs.json (what to render: notes, velocities, variants per preset)
      |
      v
[fleet-render CLI]  <-- NEW: the headless JUCE CLI
      |
      v
  WAV files on disk (named per convention)
      |
      v
[oxport.py] stages: categorize -> expand -> export -> cover_art -> package
      |
      v
  .xpn expansion pack
```

### 3.2 fleet-render CLI Interface

```bash
# Render all presets for one engine
fleet-render --engine Onset --output-dir /path/to/wavs

# Render a single preset
fleet-render --preset /path/to/preset.xometa --output-dir /path/to/wavs

# Render all presets for all engines (full fleet)
fleet-render --all --output-dir /path/to/wavs

# Use a render spec JSON instead of scanning presets
fleet-render --spec /path/to/render_specs.json --output-dir /path/to/wavs

# Control parallelism
fleet-render --workers 8 --output-dir /path/to/wavs --all

# Dry run: show what would be rendered
fleet-render --all --dry-run

# Resume interrupted render (skip existing WAVs)
fleet-render --all --output-dir /path/to/wavs --resume
```

### 3.3 Render Process Per Sample

For each WAV file specified in the render spec:

1. **Instantiate processor** (one per worker thread, reused across samples)
   - `auto proc = std::make_unique<XOmnibusProcessor>()`
   - `proc->prepareToPlay(48000.0, 512)`

2. **Load preset**
   - `PresetData preset = PresetManager::loadFromFile(xometaPath)`
   - `proc->applyPreset(preset)`
   - Wait 1 block for engine initialization (call `processBlock()` once with empty MIDI to let engines settle)

3. **Set variant parameters** (if applicable)
   - For Obbligato `bond_blend` variant: `apvts.getParameter("obbl_macroBond")->setValueNotifyingHost(0.5f)`
   - For Ohm `full` variant: `apvts.getParameter("ohm_macroMeddling")->setValueNotifyingHost(0.7f)`
   - Wait 1 block for parameter smoothing

4. **Send note-on**
   - `midi.addEvent(juce::MidiMessage::noteOn(1, midiNote, velocity), 0)`

5. **Render hold phase**
   - Loop: `processBlock(buffer, midi)` for `holdSamples / blockSize` iterations
   - Accumulate output into a large `AudioBuffer<float>`

6. **Send note-off**
   - `midi.addEvent(juce::MidiMessage::noteOff(1, midiNote), 0)`

7. **Render tail phase**
   - Continue `processBlock()` for `tailSamples / blockSize` iterations
   - This captures release envelopes, reverb tails, delay echoes

8. **Post-process**
   - Trim leading silence (threshold: -80 dBFS, keep 5ms pre-attack)
   - Trim trailing silence (threshold: -80 dBFS, keep 50ms post-decay)
   - Normalize to -0.3 dBFS peak (existing `normalizeBuffer()` method)
   - Apply 2ms fade-in and 10ms fade-out to prevent clicks

9. **Write WAV**
   - 48 kHz, 24-bit stereo WAV (existing `writeWav()` method)
   - File naming per convention (see section 3.4)

### 3.4 File Naming Convention

Output follows the conventions established in `xpn_render_spec.py`:

```
# Keygroup presets (no variant)
{output_dir}/{engine}/{preset_slug}/{preset_slug}__{note}__v{vel}.WAV

# Keygroup presets (with variant)
{output_dir}/{engine}/{preset_slug}/{preset_slug}_{variant}__{note}__v{vel}.WAV

# Drum presets
{output_dir}/{engine}/{preset_slug}/{preset_slug}_{voice}_{vel_suffix}.wav

# Stem presets (Opal, Optic)
{output_dir}/{engine}/{preset_slug}/{preset_slug}_stem.wav
```

Examples:
```
wavs/Odyssey/Astral_Drift/Astral_Drift__C3__v2.WAV
wavs/Onset/808_Reborn/808_Reborn_kick_v3.wav
wavs/Obbligato/Wind_Duet/Wind_Duet_bond_blend__Eb2__v1.WAV
wavs/Opal/Frozen_Lake/Frozen_Lake_stem.wav
```

### 3.5 Parallelism Strategy

**Thread model:** One `XOmnibusProcessor` instance per worker thread. Processor instances are independent -- no shared mutable state between them.

**Granularity:** Parallelize at the **preset level**, not the note level. Each worker thread loads one preset and renders all its notes/velocities/variants sequentially. This avoids repeated preset loading overhead.

**Worker count:** Default to `hardware_concurrency() - 1`. Each worker consumes:
- ~50 MB RAM (processor + engine buffers + render buffer)
- One CPU core (DSP is CPU-bound)

For an 8-core machine: 7 workers, ~350 MB total RAM.

**Preset-level parallelism is safe because:**
- Each `XOmnibusProcessor` owns its own `EngineRegistry` slot instances, APVTS, coupling matrix
- No global mutable state in the DSP path
- File I/O uses distinct output paths per preset

**Note-level parallelism within a preset** is already implemented in `XPNExporter.h` (the `renderBatch` lambda with `std::async`). This is useful for presets with many notes but can be disabled if preset-level parallelism saturates cores.

### 3.6 Resume / Incremental Rendering

The `--resume` flag skips WAV files that already exist on disk and have non-zero size. This handles:
- Interrupted renders (crash, power loss)
- Adding new presets to an engine already rendered
- Re-rendering a single failed preset without re-rendering the entire engine

Implementation: Before rendering each WAV, check `outputFile.existsAsFile() && outputFile.getSize() > 44` (44 = WAV header size). Skip if true.

---

## 4. Integration with Oxport

### 4.1 New Pipeline Stage: `render`

Insert a `render` stage between `render_spec` and `categorize` in `oxport.py`:

```python
STAGES = [
    "render_spec",   # Generate render specifications from .xometa presets
    "render",        # NEW: Automated WAV rendering via fleet-render CLI
    "categorize",    # Classify WAV samples into voice categories
    "expand",        # Expand flat kits into velocity/cycle/smart WAV sets
    "export",        # Generate .xpm programs
    "cover_art",     # Generate branded procedural cover art
    "package",       # Package into .xpn archive
]
```

### 4.2 Stage Implementation

```python
def _stage_render(ctx: PipelineContext) -> None:
    """Stage 2: Render presets to WAV via fleet-render CLI."""
    if not ctx.render_specs:
        print("    [SKIP] No render specs to process")
        return

    # Write render specs to temp file for fleet-render to consume
    specs_file = ctx.specs_dir / "batch_render_spec.json"
    if not ctx.dry_run:
        with open(specs_file, "w") as f:
            json.dump(ctx.render_specs, f, indent=2)

    # Invoke fleet-render CLI
    fleet_render = REPO_ROOT / "build" / "fleet-render"
    if not fleet_render.exists():
        print("    [FAIL] fleet-render binary not found. Build with:")
        print(f"           cmake --build {REPO_ROOT / 'build'} --target fleet-render")
        raise FileNotFoundError(str(fleet_render))

    cmd = [
        str(fleet_render),
        "--spec", str(specs_file),
        "--output-dir", str(ctx.samples_dir),
        "--workers", str(max(1, os.cpu_count() - 1)),
        "--resume",
    ]

    if ctx.dry_run:
        print(f"    [DRY] Would run: {' '.join(cmd)}")
        return

    import subprocess
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"    [FAIL] fleet-render exited with code {result.returncode}")
        print(result.stderr[-500:] if result.stderr else "")
        raise RuntimeError("fleet-render failed")

    # Update wavs_dir to point at rendered output
    ctx.wavs_dir = ctx.samples_dir
    print(f"    Rendered WAVs to {ctx.samples_dir}")
```

### 4.3 Backward Compatibility

The `render` stage is skippable via `--skip render`. If `--wavs-dir` is provided (pre-rendered WAVs), the render stage detects existing WAVs and skips gracefully. This preserves the existing manual workflow as a fallback.

---

## 5. Edge Cases

### 5.1 Long Release Tails (Pads, Reverbs)

**Problem:** Some presets (especially Opal textures, Overlap reverbs, Ohm communes) have 10+ second release tails. A fixed 2-second tail truncates the sound.

**Solution:** `SoundShapeClassifier` already handles this. The classifier reads the preset's Sonic DNA and assigns appropriate tail durations:
- Transient: 0.5s tail
- Sustained: 2.0s tail
- Evolving: 3.0s tail
- Bass: 1.5s tail
- Texture: 2.5s tail
- Rhythmic: 0.3s tail

Additionally, the engine-specific strategies in `xpn_render_spec.py` encode hold durations:
- Ohm: "Hold 4+ seconds" for commune sustain
- Overlap: "Hold 4+ seconds for reverb tail to develop"
- Opal: 30-second texture stems

**Implementation:** The render CLI reads both the `SoundShapeClassifier` output and the render spec's engine strategy. It uses the longer of the two durations. For Opal stems, it uses a fixed 30s + 5s tail.

**Silence-based tail truncation:** After rendering the full hold+tail duration, scan backward from the end to find the first sample above -80 dBFS. Truncate there, plus a 50ms safety margin and 10ms fade-out. This automatically handles presets where the release decays faster than the allocated tail time.

### 5.2 Velocity-Sensitive Presets

**Problem:** Different engines respond to velocity differently. Some use velocity for amplitude only; others (per Doctrine D001) use velocity to shape timbre (filter brightness, harmonic content).

**Solution:** The render spec defines velocity layers per engine. Standard MIDI velocity values:

| Layer | Suffix | MIDI Velocity | Normalized (0-1) |
|-------|--------|--------------|-------------------|
| v1 (ghost/pp) | v1 | 20 | 0.157 |
| v2 (soft/mp) | v2 | 50 | 0.394 |
| v3 (medium/mf) | v3 | 80 | 0.630 |
| v4 (hard/ff) | v4 | 120 | 0.945 |

For 2-layer engines (Overworld, Oceanic, Overlap, Ohm, Ombre, OpenSky):

| Layer | Suffix | MIDI Velocity | Normalized |
|-------|--------|--------------|------------|
| v1 (soft) | v1 | 50 | 0.394 |
| v2 (hard) | v2 | 120 | 0.945 |

These values match the `xpn_render_spec.py` documentation and the `XPNExporter::velocityForLayer()` method.

### 5.3 Coupled (Multi-Engine) Presets

**Problem:** Entangled mood presets use 2-4 engines with coupling routes. The coupled output is a single mixed signal, not individual engine outputs.

**Solution:** Render the **final mixed output** of `processBlock()`, which already includes coupling and the master FX chain. This is the sound the user designed. Do not attempt to render individual engines separately for coupled presets.

**Exception:** If a preset's render spec includes variants that isolate specific coupling states, render each variant as a separate WAV set. Currently no engine strategy requires this.

### 5.4 Randomization / Modulation Presets

**Problem:** Some engines use random elements (Ouroboros strange attractors, Oracle GENDY stochastic synthesis, Outwit cellular automata). Each render may sound slightly different.

**Solution:** Render **one take** per note/velocity/variant. The randomization is part of the character. For the XPN use case, the user will play the sample -- they don't need multiple takes of the same note.

**Exception:** If a preset is flagged as highly stochastic (DNA movement > 0.8 AND density > 0.7), render **2 takes** and keep the one with higher RMS energy (the one that "caught" a more interesting random state). The second take uses a different random seed by calling `processBlock()` with 1 second of silence before the note-on to let the random state diverge.

### 5.5 Optic Engine (Visual-Only)

**Problem:** Optic is a visual modulation engine. Some Optic presets produce no audio.

**Solution:** After rendering, check if the output buffer's peak amplitude is below -60 dBFS. If so, skip the WAV write and log a warning. The render spec already notes: "only render audio-producing presets."

### 5.6 Preset Parameter Variant Rendering

Several engines require rendering at different macro positions to capture the full sound:

| Engine | Variant | Parameter ID | Values |
|--------|---------|-------------|--------|
| Obbligato | bond_a / bond_blend / bond_b | `obbl_macroBond` | 0.0 / 0.5 / 1.0 |
| Ottoni | grow_solo / grow_ensemble | `otto_macroGrow` | 0.0 / 1.0 |
| Ole | calm / drama | `ole_macroDrama` | 0.0 / 1.0 |
| Ohm | calm / full | `ohm_macroMeddling` | 0.0 / 0.7 |
| Overdub | dry / wet | `dub_sendAmount` | 0.0 / 1.0 |
| Ouroboros | leash_tight / leash_loose | `ouro_leash` | 0.2 / 0.9 |
| OddOscar | morph_a / morph_mid / morph_b | `morph_morph` | 0.0 / 0.5 / 1.0 |
| Origami | fold_low / fold_high | `origami_foldPoint` | 0.1 / 0.9 |
| Orca | calm_hunt / active_hunt / breach | `orca_huntMacro` | 0.0 / 0.5 / 1.0 |

The render CLI reads the `variants` array from the render spec and maps each variant name to the correct parameter ID and value. This mapping is defined in a configuration table within the CLI, derived from the render spec descriptions.

---

## 6. Estimated Scale

### 6.1 WAV Count Per Engine

Calculated from `ENGINE_STRATEGIES` in `xpn_render_spec.py`, multiplied by estimated preset counts per engine:

| Engine | Type | Notes | Vel | Variants | WAVs/Preset | Est. Presets | Total WAVs |
|--------|------|-------|-----|----------|-------------|-------------|------------|
| Onset | drum | 8 voices | 4 | 1 | 32 | 115 | 3,680 |
| Odyssey | keygroup | 21 | 4 | 1 | 84 | 120 | 10,080 |
| Oblong | keygroup | 17 | 4 | 1 | 68 | 100 | 6,800 |
| Obese | keygroup | 9 | 4 | 1 | 36 | 80 | 2,880 |
| OddfeliX | keygroup | 21 | 4 | 1 | 84 | 100 | 8,400 |
| OddOscar | keygroup | 21 | 4 | 3 | 252 | 80 | 20,160 |
| Overdub | keygroup | 17 | 2 | 2 | 68 | 70 | 4,760 |
| Overworld | keygroup | 13 | 2 | 1 | 26 | 60 | 1,560 |
| Opal | stem | 1 | 1 | 1 | 1 | 80 | 80 |
| Overbite | keygroup | 13 | 4 | 1 | 52 | 80 | 4,160 |
| Orphica | keygroup | 21 | 4 | 1 | 84 | 50 | 4,200 |
| Obbligato | keygroup | 17 | 4 | 3 | 204 | 50 | 10,200 |
| Ottoni | keygroup | 13 | 4 | 2 | 104 | 50 | 5,200 |
| Ole | keygroup | 17 | 4 | 2 | 136 | 50 | 6,800 |
| Ohm | keygroup | 17 | 2 | 2 | 68 | 50 | 3,400 |
| Orbital | keygroup | 21 | 4 | 1 | 84 | 60 | 5,040 |
| Organon | keygroup | 17 | 4 | 2 | 136 | 50 | 6,800 |
| Ouroboros | keygroup | 17 | 4 | 2 | 136 | 50 | 6,800 |
| Obsidian | keygroup | 21 | 4 | 1 | 84 | 50 | 4,200 |
| Origami | keygroup | 17 | 4 | 2 | 136 | 50 | 6,800 |
| Oracle | keygroup | 17 | 4 | 1 | 68 | 50 | 3,400 |
| Obscura | keygroup | 17 | 4 | 1 | 68 | 50 | 3,400 |
| Oceanic | keygroup | 17 | 2 | 2 | 68 | 50 | 3,400 |
| Ocelot | keygroup | 17 | 4 | 2 | 136 | 50 | 6,800 |
| Optic | stem | 1 | 1 | 1 | 1 | 30 | 30 |
| Oblique | keygroup | 21 | 4 | 1 | 84 | 50 | 4,200 |
| Osprey | keygroup | 17 | 4 | 2 | 136 | 50 | 6,800 |
| Osteria | keygroup | 17 | 4 | 1 | 68 | 50 | 3,400 |
| Owlfish | keygroup | 13 | 4 | 1 | 52 | 50 | 2,600 |
| Overlap | keygroup | 17 | 2 | 3 | 102 | 50 | 5,100 |
| Outwit | keygroup | 17 | 4 | 2 | 136 | 50 | 6,800 |
| Ombre | keygroup | 17 | 2 | 2 | 68 | 50 | 3,400 |
| Orca | keygroup | 17 | 4 | 3 | 204 | 50 | 10,200 |
| Octopus | keygroup | 17 | 4 | 2 | 136 | 50 | 6,800 |
| **TOTAL** | | | | | | **~2,451** | **~196,330** |

Plus 4 concept engines (Ostinato, OpenSky, OceanDeep, Ouie) adding an estimated 10,000-15,000 WAVs when built.

**Grand total estimate: ~200,000-210,000 WAV files.**

### 6.2 Disk Space Estimate

Average WAV duration (hold + tail): ~5 seconds
Sample rate: 48,000 Hz
Bit depth: 24-bit (3 bytes per sample)
Channels: 2 (stereo)

**Per WAV file:** 5s x 48,000 x 3 bytes x 2 channels = **1.44 MB**

- Stems (Opal, Optic): 30s x 48,000 x 3 x 2 = **8.64 MB** each (110 stems = ~950 MB)
- Drums (Onset): 0.8s average = **0.23 MB** each (3,680 drums = ~850 MB)
- Keygroups: ~1.44 MB average

**Total estimate:**
- ~196,000 keygroup WAVs x 1.44 MB = ~282 GB
- ~110 stems x 8.64 MB = ~0.95 GB
- ~3,680 drums x 0.23 MB = ~0.85 GB
- **Grand total: ~284 GB uncompressed**

After silence trimming (typically removes 30-50% of file size): **~150-200 GB**

### 6.3 Render Time Estimate

**Offline rendering speed:** The `processBlock()` loop runs at roughly 10-50x realtime depending on engine complexity.

- Simple engines (Overworld, Onset): ~50x realtime
- Complex engines (Orca, Outwit, Organon with coupling): ~10x realtime
- Average across fleet: ~20x realtime

**Per-WAV render time at 20x realtime:**
- 5 second WAV renders in 0.25 seconds
- 30 second stem renders in 1.5 seconds

**Sequential (1 worker):**
- ~200,000 WAVs x 0.25s = 50,000 seconds = **~14 hours**

**Parallel (7 workers on 8-core machine):**
- 50,000s / 7 = 7,143 seconds = **~2 hours**

**Parallel (15 workers on 16-core M2 Ultra / similar):**
- 50,000s / 15 = 3,333 seconds = **~55 minutes**

These estimates include only render time. WAV I/O adds ~10%. Preset loading adds ~5%. Total wall clock on an 8-core machine: **~2.5 hours** for the entire fleet.

---

## 7. Build Integration

### 7.1 CMake Target

Add a `fleet-render` executable target to the XOmnibus CMake project:

```cmake
# Tools/CMakeLists.txt (or added to top-level CMakeLists.txt)
add_executable(fleet-render
    Tools/FleetRender/Main.cpp
)

target_link_libraries(fleet-render PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_core
    XOmnibusEngine  # shared library target containing all engine + DSP code
)

target_compile_definitions(fleet-render PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    FLEET_RENDER_HEADLESS=1
)
```

The `FLEET_RENDER_HEADLESS=1` define lets the processor skip UI-related initialization.

### 7.2 Main.cpp Structure

```
Tools/FleetRender/
    Main.cpp          -- CLI argument parsing, worker thread management
    RenderWorker.h    -- Per-thread processor instance, render loop
    VariantMap.h      -- Maps variant names to parameter IDs and values
    ProgressReporter.h -- Console progress bar, JSON progress output
```

**Main.cpp responsibilities:**
1. Parse CLI arguments
2. Load render specs (from JSON file or by scanning presets)
3. Partition work across worker threads
4. Launch workers, collect results
5. Report summary (WAVs rendered, errors, disk usage, elapsed time)

### 7.3 XOmnibusEngine Shared Target

Currently all engine code compiles into the plugin target. For fleet-render, extract the non-UI code into a shared static library:

```cmake
add_library(XOmnibusEngine STATIC
    Source/XOmnibusProcessor.cpp
    Source/Core/*.cpp
    Source/DSP/*.cpp
    Source/Engines/**/*.cpp
    Source/Export/XPNExporter.h
    # Exclude: Source/UI/*, Source/XOmnibusEditor.*
)
```

This library is linked by both the plugin target and the fleet-render CLI.

---

## 8. Progress Reporting

### 8.1 Console Output

```
Fleet Render Automation -- XO_OX Designs
  Engine: all | Presets: 2,451 | Workers: 7

  [Onset]  115 presets, 32 WAVs each = 3,680 total
    808 Reborn ................ 32/32  [========================================] 0.8s
    Dirty Trap ............... 32/32  [========================================] 0.9s
    ...

  [Odyssey]  120 presets, 84 WAVs each = 10,080 total
    Astral Drift ............. 84/84  [========================================] 4.2s
    ...

  COMPLETE
    Total WAVs:    196,330
    Total size:    183.2 GB
    Elapsed:       2h 14m 33s
    Errors:        0
    Skipped:       12 (Optic silent presets)
```

### 8.2 JSON Progress (for CI/scripting)

With `--progress json`, emit one JSON line per completed preset:

```json
{"engine":"Onset","preset":"808 Reborn","wavs":32,"elapsed_s":0.8,"total_progress":0.0016}
```

### 8.3 Resume State

Write a `.fleet_render_state.json` to the output directory after each engine completes:

```json
{
  "started": "2026-03-15T10:00:00",
  "engines_complete": ["Onset", "Odyssey"],
  "engines_remaining": ["Oblong", "..."],
  "total_wavs_rendered": 13760,
  "total_bytes": 19834880000
}
```

---

## 9. Quality Assurance

### 9.1 Automated Validation (Post-Render)

After rendering each WAV, validate:
1. **File size > 44 bytes** (not just a header)
2. **Peak amplitude > -60 dBFS** (not silent) -- warn but don't fail for Optic
3. **No NaN or Inf samples** (indicates DSP bug)
4. **Duration within 10% of expected** (silence trimming didn't remove too much)
5. **Sample rate matches settings** (48 kHz)

### 9.2 Spot-Check Listening Pass

After a full fleet render, randomly select 1% of WAVs (~ 2,000 files) across all engines and generate a concatenated "spot-check" WAV with 0.5s silence between samples. A human listens through this ~3-hour file to catch systematic issues (clicks, distortion, wrong pitch, missing release tails).

### 9.3 A/B Comparison

For critical presets (one per engine), render via fleet-render AND manually record from the standalone app. Compare spectrograms to verify the offline render matches the live output exactly.

---

## 10. Implementation Roadmap

### Phase 1: Core Renderer (1-2 days)
- [ ] Fill in `XPNExporter::renderNoteToWav()` with actual processor rendering (~40 lines)
- [ ] Test with a single preset end-to-end (load, render, write WAV, verify audio)
- [ ] Verify coupling renders correctly for an Entangled preset

### Phase 2: CLI Tool (1 day)
- [ ] Create `Tools/FleetRender/Main.cpp` with argument parsing
- [ ] Add `fleet-render` CMake target
- [ ] Implement `--engine`, `--preset`, `--spec`, `--output-dir`, `--dry-run` flags
- [ ] Implement `--resume` (skip existing WAVs)

### Phase 3: Variant Rendering (0.5 day)
- [ ] Build `VariantMap.h` mapping variant names to parameter IDs/values
- [ ] Test variant rendering for Obbligato (3 variants) and Orca (3 variants)

### Phase 4: Parallelism (0.5 day)
- [ ] Implement worker thread pool with per-thread processor instances
- [ ] Test with `--workers 1` and `--workers N` for correctness
- [ ] Benchmark: measure speedup vs. single-threaded

### Phase 5: Oxport Integration (0.5 day)
- [ ] Add `render` stage to `oxport.py`
- [ ] Test full pipeline: `oxport.py run --engine Onset --output-dir /tmp/test`
- [ ] Update `oxport.py` `STAGES` list and `STAGE_DESCRIPTIONS`

### Phase 6: Fleet Validation (1 day)
- [ ] Run full fleet render (`--all`)
- [ ] Run automated validation on all output WAVs
- [ ] Generate spot-check listening file
- [ ] Fix any issues found

**Total estimated effort: 4-5 days**

---

## 11. Future Enhancements (V2)

- **Cloud rendering:** Distribute across multiple machines via a work queue (Redis/SQS). Each machine runs fleet-render on a subset of presets.
- **Incremental re-render:** Track preset modification dates. Only re-render presets that changed since last render.
- **Perceptual hash cache:** Store a perceptual hash of each WAV. On re-render, compare hashes to detect if the output actually changed (useful for CI).
- **GPU-accelerated DSP:** For engines with heavy convolution (Overlap FDN), explore GPU offloading via Metal/CUDA.
- **Option C fallback:** Package a pre-built XOmnibus VST3 with a DawDreamer Python script as a zero-compile rendering path for non-developers.
