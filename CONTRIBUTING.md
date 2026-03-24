# Contributing to XOlokun

Thanks for your interest in contributing to XOlokun! This document covers everything you need to get started.

## Environment Setup

XOlokun requires:
- **CMake** 3.22+
- **Ninja** build system
- **C++17** compiler (Xcode on macOS, GCC/Clang on Linux)
- **JUCE** 8.0.4+ (included as submodule)
- **Node.js** 20 via fnm (for tooling scripts):
  ```bash
  eval "$(fnm env)" && fnm use 20
  ```

### Build

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Validate (macOS)

```bash
auval -a  # List all Audio Units
auval -v aumu Xomn Xoox  # Validate XOlokun AU
```

## Architecture Overview

- **Engines** (`Source/Engines/`): Each engine implements `SynthEngine` interface. DSP lives inline in `.h` headers.
- **Core** (`Source/Core/`): Engine registry, coupling matrix, preset manager.
- **DSP** (`Source/DSP/`): Shared DSP library (filters, effects, SRO framework).
- **UI** (`Source/UI/`): Gallery Model components.
- **Presets** (`Presets/XOlokun/`): `.xometa` JSON files organized by mood.
- **Tools** (`Tools/`): Python utilities for preset management and XPN export.

## Code Style

- All DSP lives in inline `.h` headers; `.cpp` files are one-line stubs
- Parameter IDs are namespaced by engine prefix (e.g., `snap_filterCutoff`, `opal_grainSize`)
- **Never** allocate memory on the audio thread
- **Never** perform blocking I/O on the audio thread
- **Never** rename stable parameter IDs after release
- Denormal protection required in all feedback/filter paths
- Use `ParamSnapshot` pattern: cache all parameter pointers once per block

## Submitting Changes

1. Fork the repository
2. Create a feature branch from `main`
3. Make your changes with clear commit messages
4. Ensure `cmake --build build` succeeds with no warnings
5. Run `auval -v aumu Xomn Xoox` if you touched audio code
6. Open a Pull Request with:
   - What you changed and why
   - Which engines are affected
   - Whether presets are affected

## What Makes a Good Contribution

- **Bug fixes** with clear reproduction steps
- **Preset contributions** following the `.xometa` format and naming conventions (2-3 words, evocative, max 30 chars)
- **Documentation improvements** to engine guides or build instructions
- **DSP optimizations** that are perceptually transparent (no audible change)

## The Six Doctrines

All engines must comply with these. If your change affects an engine, verify:

1. **D001** — Velocity must shape timbre (not just amplitude)
2. **D002** — Minimum: 2 LFOs, mod wheel/aftertouch, 4 working macros
3. **D003** — Physical models require rigor and citation
4. **D004** — Every declared parameter must affect audio output
5. **D005** — At least one LFO with rate floor ≤ 0.01 Hz
6. **D006** — Velocity→timbre + at least one CC (aftertouch/mod wheel/expression)

## Response Time

This is a solo-developer project. PRs are reviewed on a best-effort basis — typically within 1-2 weeks. Please be patient and responsive to feedback.
