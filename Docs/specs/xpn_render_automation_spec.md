# XPN Render Automation — `renderNoteToWav()` Implementation Spec

**Status:** Ready for Implementation (Opus-level)
**Date:** 2026-03-16
**Priority:** P0 Critical — blocks all pack production at scale
**Estimated effort:** 3–5 Opus sessions

---

## Executive Summary

`Source/Export/XPNExporter.h` lines 602–616 contain a silent placeholder for
`renderNoteToWav()`. All surrounding infrastructure is complete: buffer allocation,
WAV writing, normalization, `SoundShapeClassifier`, parallel batch dispatch, XPM
generation. The single missing piece is audio output from the DSP chain.

This spec defines exactly how to fill that gap: instantiate a temporary processor
with a borrowed APVTS, drive it with a synthesized MIDI sequence, render offline in
a worker thread, and write the result. A CLI shim (`xpn_render_cli`) exposes this
capability to `oxport.py` via a new `render` pipeline stage.

---

## 1. Scope

`renderNoteToWav()` must:

- Accept a MIDI note, velocity, hold duration, and release tail
- Instantiate or borrow a `SynthEngine` for the target engine
- Load a named preset into that engine's APVTS
- Render audio offline (worker thread, no real-time constraint)
- Write a normalized WAV at the specified bit depth and sample rate

It must **not**:

- Allocate on the audio thread (offline context — no constraint, but stay consistent)
- Access the live `XOlokunProcessor` instance (render is fully self-contained)
- Assume 44100 Hz (derive from `RenderParams::sampleRate`)
- Block the message thread

---

## 2. Updated Interface

The existing `renderNoteToWav()` signature in `XPNExporter.h` is replaced with the
following. The existing `RenderSettings` struct is kept for batch-level config; the
new `RenderParams` struct carries per-note data.

```cpp
// Source/Export/XPNExporter.h

struct RenderParams {
    int         midiNote;        // 0–127
    int         velocity;        // 1–127
    float       durationSeconds; // note-on hold time
    float       releaseSeconds;  // render time after note-off (release + FX tail)
    int         sampleRate;      // 44100 or 48000
    int         bitDepth;        // 16 or 24
    bool        stereo;          // false = sum to mono after render
    juce::File  outputPath;
};

struct RenderResult {
    bool         success;
    juce::String errorMessage;
    double       peakLevel;      // pre-normalization peak (0.0–1.0+)
    double       rmsLevel;       // RMS of hold region only
    double       durationActual; // seconds actually rendered
};

// Replaces the silent stub at lines 591–623.
RenderResult renderNoteToWav(const RenderParams&                    params,
                              SynthEngine*                           engine,
                              juce::AudioProcessorValueTreeState&    apvts);
```

The existing `IOResult renderNoteToWav(const PresetData&, int, float,
const RenderSettings&, const juce::File&)` is kept as a thin wrapper that
constructs `RenderParams` from `RenderSettings` and calls the new overload.

---

## 3. JUCE Implementation Strategy

### 3.1 Engine Lifecycle for Offline Render

The caller (batch dispatcher) is responsible for constructing and owning the engine.
`renderNoteToWav()` does not create the engine — it receives a prepared instance.

```
Caller (worker thread):
  1. engine = EngineRegistry::create(engineId)
  2. apvts  = buildApvts(*engine)          // minimal standalone APVTS
  3. PresetManager::loadInto(preset, apvts) // set all parameter values
  4. engine->attachParameters(apvts)
  5. engine->prepare(sampleRate, blockSize) // allocates voice buffers
  6. renderNoteToWav(params, engine.get(), apvts)
  7. engine->releaseResources()
```

This matches the existing `SynthEngine` lifecycle contract and requires zero changes
to any engine module.

### 3.2 Block Size

Use 512 samples. This is large enough to amortize envelope ramp calculations and
small enough that the block loop terminates quickly. Do not use a single mega-block —
engines with LFOs or modulation may have per-block state that must advance correctly.

```cpp
static constexpr int RENDER_BLOCK_SIZE = 512;
```

### 3.3 MIDI Sequence Construction

Build the MIDI buffer once before the render loop. Stamp note-on at sample 0 and
note-off at the sample corresponding to `durationSeconds`.

```cpp
juce::MidiBuffer buildMidiSequence(const RenderParams& p)
{
    juce::MidiBuffer midi;
    int holdSamples = (int)(p.durationSeconds * p.sampleRate);

    midi.addEvent(juce::MidiMessage::noteOn(1, p.midiNote, (uint8_t)p.velocity), 0);
    midi.addEvent(juce::MidiMessage::noteOff(1, p.midiNote, (uint8_t)0), holdSamples);
    return midi;
}
```

### 3.4 Render Loop

Iterate in `RENDER_BLOCK_SIZE` chunks. For each block, extract the MIDI events that
fall within the current sample window using `juce::MidiBuffer::Iterator` (or the
range constructor). Call `engine->renderBlock()` with the block-local MIDI buffer.

```cpp
RenderResult renderNoteToWav(const RenderParams& params,
                              SynthEngine* engine,
                              juce::AudioProcessorValueTreeState& /*apvts*/)
{
    const int totalSamples = (int)((params.durationSeconds + params.releaseSeconds)
                                   * params.sampleRate);
    const int channels     = params.stereo ? 2 : 1;

    juce::AudioBuffer<float> accumulator(2, totalSamples); // always render stereo internally
    accumulator.clear();

    juce::MidiBuffer fullMidi = buildMidiSequence(params);

    int samplePos = 0;
    while (samplePos < totalSamples)
    {
        const int blockSamples = std::min(RENDER_BLOCK_SIZE, totalSamples - samplePos);

        // Slice the global MIDI buffer to this block's window
        juce::MidiBuffer blockMidi;
        for (const auto meta : fullMidi)
        {
            int t = meta.samplePosition;
            if (t >= samplePos && t < samplePos + blockSamples)
                blockMidi.addEvent(meta.getMessage(), t - samplePos);
        }

        // Sub-buffer pointing into accumulator — no copy
        juce::AudioBuffer<float> block(accumulator.getArrayOfWritePointers(),
                                       2, samplePos, blockSamples);

        engine->renderBlock(block, blockMidi, blockSamples);
        samplePos += blockSamples;
    }

    // Mix to mono if requested
    if (!params.stereo)
    {
        accumulator.addFrom(0, 0, accumulator, 1, 0, totalSamples, 0.5f);
        accumulator.applyGain(0, 0, totalSamples, 0.5f);
    }

    // Measure levels before normalization
    RenderResult result;
    result.peakLevel    = (double)accumulator.getMagnitude(0, totalSamples);
    result.rmsLevel     = (double)accumulator.getRMSLevel(0, 0,
                              (int)(params.durationSeconds * params.sampleRate));
    result.durationActual = (double)totalSamples / params.sampleRate;

    // Normalize
    normalizeBuffer(accumulator, -1.0f); // -1 dBFS ceiling

    // Write
    juce::AudioBuffer<float> output(channels, totalSamples);
    for (int ch = 0; ch < channels; ++ch)
        output.copyFrom(ch, 0, accumulator, ch, 0, totalSamples);

    auto ioResult = writeWav(params.outputPath, output,
                              params.sampleRate, params.bitDepth);
    result.success      = ioResult.success;
    result.errorMessage = ioResult.errorMessage;
    return result;
}
```

### 3.5 Denormal Protection

Wrap the render loop body in a JUCE denormal flush scope:

```cpp
juce::ScopedNoDenormals noDenormals;
while (samplePos < totalSamples) { ... }
```

This is required for engines with feedback paths (OUROBOROS, OVERDUB, OHM, ORACLE,
ORGANON). Without it, denormals in feedback loops cause CPU spikes that make offline
render 10–100× slower than expected.

### 3.6 Thread Safety

- `renderNoteToWav()` is called from `std::async` worker threads (already the case in
  the existing batch dispatcher).
- The `engine` pointer is **not shared** between workers. Each worker creates its own
  `SynthEngine` instance via `EngineRegistry::create()`.
- The `apvts` is also per-worker. Do not share APVTS across concurrent renders.
- `EngineRegistry::create()` must be thread-safe (factory returns a new `unique_ptr`
  each call — verify this holds; no static mutable state allowed in the factory).

---

## 4. CLI Shim — `xpn_render_cli`

### 4.1 Purpose

`oxport.py` cannot call C++ directly. The CLI shim is a standalone JUCE executable
that reads a JSON render manifest, executes `renderNoteToWav()` for each entry, and
exits with code 0 on success or 1 on any failure.

### 4.2 Files

```
Source/Export/XPNRenderCLI.h    — render manifest parsing + orchestration
Source/Export/XPNRenderCLI.cpp  — one-line stub: #include "XPNRenderCLI.h"
CMakeLists.txt                  — new juce_add_console_app(XPNRenderCLI ...)
```

### 4.3 CMake Target

```cmake
juce_add_console_app(XPNRenderCLI
    PRODUCT_NAME "XPN Render CLI")

target_sources(XPNRenderCLI PRIVATE
    Source/Export/XPNRenderCLI.cpp)

target_link_libraries(XPNRenderCLI PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_core
    XOlokunEngines)   # shared static lib containing all engine objects
```

`XOlokunEngines` is a new `add_library(XOlokunEngines STATIC ...)` target that
compiles all engine `.cpp` stubs and `EngineRegistry.cpp`. The main XOlokun AU/VST3
target and `XPNRenderCLI` both link against it, avoiding duplicate compilation.

### 4.4 CLI Usage

```
./XPNRenderCLI --manifest /path/to/render_manifest.json [--jobs 4] [--dry-run]
```

Arguments:

| Flag | Default | Description |
|------|---------|-------------|
| `--manifest` | (required) | Path to render manifest JSON |
| `--jobs` | CPU count | Parallel worker threads |
| `--dry-run` | false | Validate manifest, print plan, exit without rendering |
| `--verbose` | false | Per-file progress to stdout |

Exit codes: `0` = all renders succeeded, `1` = one or more failures (partial output
may exist), `2` = manifest parse error or missing preset.

### 4.5 `XPNRenderCLI.h` Structure

```cpp
// Source/Export/XPNRenderCLI.h

#include "XPNExporter.h"
#include "../Core/EngineRegistry.h"
#include "../Core/PresetManager.h"
#include <juce_core/juce_core.h>

namespace xolokun {

struct ManifestEntry {
    int         note;
    int         velocity;
    float       duration;       // optional, default from SoundShapeClassifier
    float       release;        // optional, default from SoundShapeClassifier
    juce::File  outputPath;
};

struct RenderManifest {
    juce::String            engineId;
    juce::String            presetName;
    int                     sampleRate  = 44100;
    int                     bitDepth    = 24;
    bool                    stereo      = true;
    std::vector<ManifestEntry> renders;
};

RenderManifest parseManifest(const juce::File& manifestFile);
int            runCLI(int argc, char** argv);

} // namespace xolokun
```

---

## 5. Render Manifest Format

```json
{
  "engine":     "ONSET",
  "preset":     "Iron Hat",
  "sampleRate": 44100,
  "bitDepth":   24,
  "stereo":     true,
  "renders": [
    {
      "note":     36,
      "velocity": 40,
      "duration": 0.5,
      "release":  0.3,
      "output":   "kick_soft.wav"
    },
    {
      "note":     36,
      "velocity": 100,
      "duration": 0.5,
      "release":  0.3,
      "output":   "kick_hard.wav"
    },
    {
      "note":     60,
      "velocity": 80,
      "output":   "mid_c.wav"
    }
  ]
}
```

Field rules:

- `engine` — canonical engine ID from `EngineRegistry` (e.g. `"ONSET"`, `"OPAL"`)
- `preset` — exact name matching a `.xometa` file in `Presets/XOlokun/`
- `sampleRate` — `44100` or `48000`; default `44100`
- `bitDepth` — `16` or `24`; default `24`
- `stereo` — default `true`
- `renders[].duration` — optional; if absent, derived from `SoundShapeClassifier`
- `renders[].release` — optional; if absent, derived from `SoundShapeClassifier`
- `renders[].output` — relative path resolved against the manifest's parent directory

`oxport.py`'s `render_spec` stage already produces a superset of this data in
`ctx.render_specs`. The `render` stage (§6) converts that list to manifest JSON and
invokes the CLI.

---

## 6. Integration with `oxport.py`

### 6.1 New Stage: `render`

Insert `render` between `render_spec` and `categorize` in `STAGES`:

```python
STAGES = [
    "render_spec",
    "render",        # NEW — calls XPNRenderCLI
    "categorize",
    "expand",
    "qa",
    "export",
    "cover_art",
    "complement_chain",
    "package",
]
```

```python
STAGE_DESCRIPTIONS = {
    ...
    "render": "Render WAV files from presets via XPNRenderCLI",
    ...
}
```

### 6.2 `_stage_render()` Implementation

```python
def _stage_render(ctx: PipelineContext) -> None:
    """Stage 2: Invoke XPNRenderCLI to render WAV files from preset specs."""
    import json, subprocess, shutil

    cli_path = Path(os.environ.get("XPN_RENDER_CLI",
                    "build/XPNRenderCLI_artefacts/Release/XPNRenderCLI"))
    if not cli_path.exists():
        raise RuntimeError(
            f"XPNRenderCLI not found at {cli_path}. "
            "Build with cmake --build build --target XPNRenderCLI or "
            "set XPN_RENDER_CLI env var."
        )

    ctx.wav_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = ctx.output_dir / f"{ctx.preset_slug}_render_manifest.json"

    # Convert render_specs → manifest format
    manifest = {
        "engine":     ctx.engine,
        "preset":     ctx.preset,
        "sampleRate": ctx.sample_rate,
        "bitDepth":   ctx.bit_depth,
        "stereo":     ctx.stereo,
        "renders":    [
            {
                "note":     s["note"],
                "velocity": s["velocity"],
                "duration": s.get("duration"),
                "release":  s.get("release"),
                "output":   str(ctx.wav_dir / s["wav_filename"]),
            }
            for s in ctx.render_specs
        ]
    }
    manifest_path.write_text(json.dumps(manifest, indent=2))

    jobs = os.cpu_count() or 4
    cmd  = [str(cli_path), "--manifest", str(manifest_path), "--jobs", str(jobs)]
    if ctx.dry_run:
        cmd.append("--dry-run")

    print(f"    Invoking XPNRenderCLI ({len(manifest['renders'])} renders, {jobs} jobs)")
    result = subprocess.run(cmd, capture_output=True, text=True)

    if result.returncode != 0:
        raise RuntimeError(
            f"XPNRenderCLI failed (exit {result.returncode}):\n{result.stderr}"
        )

    print(f"    Rendered {len(manifest['renders'])} WAV files → {ctx.wav_dir}")
```

### 6.3 `PipelineContext` Additions

```python
@dataclass
class PipelineContext:
    ...
    sample_rate: int = 44100   # NEW
    bit_depth:   int = 24      # NEW
    stereo:      bool = True   # NEW
    wav_dir:     Path = ...    # already exists as output_dir / "wavs"
```

### 6.4 Env Var Override

`XPN_RENDER_CLI` environment variable lets CI and local dev point to different build
artifacts without touching the script:

```bash
XPN_RENDER_CLI=~/build-release/XPNRenderCLI python Tools/oxport.py ONSET "Iron Hat"
```

---

## 7. Audio Thread Safety Checklist

| Concern | Mitigation |
|---------|-----------|
| Memory allocation in renderBlock | All engines pre-allocate in `prepare()`. No allocation in block loop. |
| Denormals in feedback paths | `juce::ScopedNoDenormals` wraps entire render loop |
| APVTS thread safety | Each worker has its own APVTS instance — no sharing |
| EngineRegistry factory | Must return a new `unique_ptr` each call — verify no static mutable state |
| `normalizeBuffer()` on silent input | Guard: if `peak < 1e-6f`, skip normalization and log a warning — do not divide by zero |
| Engine `reset()` before render | Call `engine->reset()` after `attachParameters()`, before the render loop, to clear any lingering envelope state |
| MegaCouplingMatrix in headless context | Render is single-engine — do not instantiate MegaCouplingMatrix. Pass nullptr or a NullCouplingMatrix stub to engines that query it |

---

## 8. Edge Cases

**Silent output.** If `peakLevel < 1e-6` after rendering, `renderNoteToWav()` sets
`result.success = false` with `errorMessage = "Engine rendered silence — check preset
and engine ID"`. The batch dispatcher logs this as a warning and writes a zero-byte
sentinel file so downstream stages can detect the gap.

**ONSET engine.** ONSET is polyphonic-per-voice and note-triggered. MIDI note 36 maps
to Kick, 38 to Snare, etc. — this mapping is already encoded in `xpn_drum_export.py`
`PAD_NOTE_MAP`. The manifest generator must use those same note values; the CLI does
not need special-case logic.

**Release longer than allocated buffer.** If `releaseSeconds` causes the engine's
release envelope to be truncated, the rendered file will have a hard clip at the end.
Add a 5% buffer: `totalSamples *= 1.05f` and trim trailing silence with
`XPNExporter::trimSilence()` (already implemented).

**Multi-engine presets (Entangled mood).** Phase 1 only renders the primary engine.
MegaCouplingMatrix coupling is not active in headless single-engine renders. This is
acceptable — Entangled presets are not currently in the XPN pack targets. Document as
a known limitation; revisit in Phase 2.

---

## 9. Build Integration

The new `XPNRenderCLI` target must be added to `CMakeLists.txt`. It shares all engine
source via the `XOlokunEngines` static library target. No engine `.h` files change.

Estimated `CMakeLists.txt` additions: ~30 lines.

CI pipeline addition: after `cmake --build build`, add:
```
cmake --build build --target XPNRenderCLI
```

The CLI binary is not shipped in the plugin bundle — it is a development tool only.
Add to `.gitignore`: `build/XPNRenderCLI_artefacts/`.

---

## 10. Why This Is Opus-Level Work

This spec is complete enough that an experienced JUCE developer could implement it.
The Opus sessions are required because:

1. **Engine lifecycle correctness.** `prepare()` → `attachParameters()` → `reset()`
   must happen in the right order. Getting this wrong produces silence or crashes
   silently. Each engine must be smoke-tested in headless context.

2. **EngineRegistry factory audit.** All 34 engine factories must be verified to have
   no static mutable state (thread safety for parallel workers). Some engines may
   cache DSP tables in static locals — these need `std::call_once` guards.

3. **APVTS construction for standalone use.** The live `XOlokunProcessor` constructs
   a merged APVTS with all 34 engine layouts. For headless rendering, a single-engine
   APVTS is needed. The merge logic in `XOlokunProcessor::createParameterLayout()`
   must be extractable into a reusable helper.

4. **CMake refactor.** Extracting `XOlokunEngines` as a shared static library
   requires touching the build system carefully — the main AU/VST3 target must
   continue to work without regressions.

5. **End-to-end validation.** Each of the 34 engines must render at least one note
   non-silently in CI before this feature ships.

**Estimated sessions:** Session 1 (CMake + APVTS standalone), Session 2 (render loop
+ denormals + silence guard), Session 3 (CLI shim + manifest parsing), Session 4
(oxport.py integration + smoke test all 34 engines), Session 5 (edge cases + CI).

---

## 11. Success Criteria

- [ ] `renderNoteToWav()` produces non-silent WAV for all 34 registered engines
- [ ] `XPNRenderCLI --manifest` completes without error on a 128-note × 4-velocity
      fleet render for ONSET in under 60 seconds on an M1 Mac
- [ ] `oxport.py ONSET "Iron Hat"` runs end-to-end with `render` stage active and
      produces a valid `.xpn` bundle
- [ ] `auval` result for the main XOlokun AU is unchanged after CMake refactor
- [ ] No engine produces silence for its canonical root note at velocity 100
