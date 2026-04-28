# Contributing to XOceanus

XOceanus is free because it has to be. MIT-licensed, no subscriptions, no gatekeeping. That promise extends to who can contribute. If you have something to add — a bug fix, a preset, an engine concept, a documentation correction — this is your instrument too.

Here's how the deep works.

## Environment Setup

XOceanus requires:
- **CMake** 3.22+
- **Ninja** build system
- **Xcode Command Line Tools** (macOS) or **GCC/Clang** (Linux) — C++17 required
- **JUCE** 8.0.4+ (included as submodule — `git submodule update --init --recursive`)
- **Python** 3.9+ (optional — for `Tools/` preset management and XPN export scripts)

### Build

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Pre-commit Hooks

XOceanus ships a pre-commit hook that catches compile errors, dead parameters (D004), and audio-thread violations before they reach CI:

```bash
git config core.hooksPath .githooks
```

The hook runs in < 30 seconds on incremental builds and is skippable with `git commit --no-verify` when needed. Checks performed:

1. **Compile** — incremental `cmake --build build --target XOceanus_AU` on staged `.h`/`.cpp` files
2. **Dead-param guard** — flags `(void)param;` casts in engine files (D004 violation)
3. **Audio-thread lint** — bans `new`/`delete`/`malloc`/`free`, `push_back`/`resize`, `std::mutex`, `juce::String` construction, and file I/O inside `renderBlock`
4. **clang-format** — runs `clang-format --dry-run --Werror` on staged files (requires `clang-format` on `$PATH`)

### Validate (macOS)

```bash
auval -v aumu Xocn XoOx
```

If `auval` passes, the engine is trustworthy. If it fails, don't ship it.

### Python Tools Setup

`Tools/` contains Python utilities for preset management, XPN export, and DNA analysis. Most tools use only the standard library, but a few optional dependencies enable cover art generation and DSP expansion:

```bash
# Install optional Python dependencies (Python 3.9+ required)
pip install -r Tools/requirements.txt

# Or install only what you need:
pip install Pillow numpy        # cover art tools (xpn_cover_art.py)
pip install soundfile scipy     # DSP expansion (xpn_kit_expander.py)
```

Most tools run without any of these installed — they degrade gracefully when optional deps are absent.

## Architecture

- **Engines** (`Source/Engines/`): Each engine implements the `SynthEngine` interface. DSP lives inline in `.h` headers — portable, testable, no `.cpp` bloat.
- **Core** (`Source/Core/`): Engine registry, MegaCouplingMatrix, preset manager.
- **DSP** (`Source/DSP/`): Shared DSP library — StandardLFO, FilterEnvelope, PitchBendUtil, VoiceAllocator, GlideProcessor, ParameterSmoother.
- **Presets** (`Presets/XOceanus/`): `.xometa` JSON files organized by mood. 16 moods. 19,859+ presets.
- **Tools** (`Tools/`): Python utilities for preset management and XPN export.
- **Scripture** (`scripture/`): The Book of Bin — accumulated DSP wisdom and ghost council verdicts.

## The Six Doctrines

Every engine must pass these. If your change affects DSP, check all six.

The ghost council — Moog, Buchla, Smith, Kakehashi, Pearlman, Vangelis, Schulze, Tomita — handed these down during the Seance process. They are not guidelines. They are engineering commandments.

| # | Doctrine | What it means |
|---|----------|----------------|
| **D001** | Velocity shapes timbre | Velocity drives filter brightness and harmonic content — not just amplitude. Harder notes are brighter. This is how real instruments work. |
| **D002** | Modulation is the lifeblood | Minimum: 2 LFOs, mod wheel, aftertouch, 4 working macros, 4+ mod matrix slots. |
| **D003** | The physics is the synthesis | Physical models require rigor and academic citation. If you're simulating mallet physics, cite Chaigne 1997. The math is the art. |
| **D004** | Dead parameters are broken promises | Every declared parameter must affect audio output. A UI knob that does nothing is a lie. |
| **D005** | An engine that cannot breathe is a photograph | Every engine needs at least one LFO with a rate floor at or below 0.01 Hz. Static sound is not sound — it's a snapshot. |
| **D006** | Expression input is not optional | Velocity→timbre plus at least one CC (aftertouch, mod wheel, or expression) must be wired. |

## Code Style

```
Never allocate memory on the audio thread.
Never perform blocking I/O on the audio thread.
Never rename stable parameter IDs after release.
```

These are the three rules that will never change. Everything else is preference.

Additional conventions:
- All DSP in inline `.h` headers; `.cpp` files are one-line `#include` stubs
- Parameter IDs namespaced by engine prefix: `snap_filterCutoff`, `opal_grainSize`, `oxy_intimacy`
- Use `ParamSnapshot` pattern: cache all parameter pointers once per block, never inside the audio callback
- Denormal protection required in all feedback and filter paths
- Engine hot-swap uses 50ms crossfade — never cause a click

## Submitting Changes

1. Fork the repository
2. Create a feature branch from `main`
3. Make your changes
4. Ensure `cmake --build build` passes with no errors
5. Run `auval -v aumu Xocn XoOx` if you touched audio code
6. Open a Pull Request describing:
   - What you changed and why
   - Which engines are affected
   - Whether presets need updating
   - Your auval result if relevant

## What Makes a Good Contribution

**Bug fixes** with clear reproduction steps are always welcome. The three categories most likely to contain bugs: race conditions in coupling routes, missing denormal guards in filter feedback paths, and Python tools that use string matching against stale engine name lists.

**Preset contributions** are product. Follow the format:
- `.xometa` JSON (see `Docs/xoceanus_master_specification.md` for schema)
- 2-3 words, evocative, max 30 characters
- No jargon. No duplicates. No filler.
- 6D Sonic DNA filled in: brightness, warmth, movement, density, space, aggression
- All four macros must produce audible change

**Engine contributions** start as standalone instruments, not as XOceanus integrations. Build the engine with its own character and reason to exist. Then write a thin adapter implementing `SynthEngine`. See `Docs/specs/xoceanus_new_engine_process.md` and invoke `/new-xo-engine` if you're working in the XO_OX development environment.

**Documentation** — if something confused you, fix it. The Field Guide, sound design guides, and scripture benefit from perspectives other than the original author's.

## The Coupling Contract

If your contribution touches coupling:
- Coupling must never propagate back to affect MIDI routing or voice stealing (B016 — the amended Brick Independence doctrine)
- Synthesis-layer interdependence is permitted; MIDI-layer is inviolable
- Test coupling routes with at least two engine combinations
- Document which `CouplingType` enums your engine accepts in the engine header

## Response Time

XOceanus is a solo-developer project. PRs are reviewed on a best-effort basis — typically within 1-2 weeks. Please be patient and responsive to feedback. The depth rewards patience.

---

*"XOceanus — for all."*
