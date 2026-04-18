# Pi 5 + Elk Audio OS — Hardware Independence R&D

> Status: RESEARCH | Priority: Long-term
> Owner: Hex | Effort: XL (ongoing investigation)

## Concept

Run XOceanus as a standalone hardware instrument on Raspberry Pi 5,
bypassing the locked MPC firmware entirely.

## Why Elk Audio OS

- Ultra-low-latency Linux audio kernel (~1ms round-trip)
- Designed for commercial audio products (Moog, Mind Music Labs)
- Runs on ARM (Pi 4/5 compatible)
- Open for third-party plugin development (unlike MPC OS)
- XOceanus DSP is all inline .h headers — portable C++ with no platform deps

## Current Status (2026-03)

- Pi 5 hardware: available, sufficient CPU for single-engine rendering
- Elk Audio OS: v1.0 released, Pi 5 support in beta
- XOceanus on ARM: not tested, but DSP layer has no x86 dependencies
- JUCE on Elk: documented workflow exists (JUCE cross-compile for ARM Linux)

## Open Questions

1. Can XOceanus's full 4-engine architecture fit in Pi 5's thermal envelope?
2. What UI does Elk provide? (Typically: headless + web UI or hardware controls)
3. Audio I/O: which HAT/interface? (HiFiBerry, Pisound, custom)
4. How does preset loading work without a filesystem browser?
5. What's the licensing model for Elk OS in a commercial product?

## Next Steps

1. Cross-compile XOceanus DSP layer for ARM64 Linux (test build only)
2. Measure single-engine CPU on Pi 5 (target: <30% for 8-voice poly)
3. If feasible: prototype with Pisound HAT + 4 rotary encoders for macros
4. Decision gate: if single-engine works, design a hardware enclosure concept

## Not a Priority Until

- Core ships and adoption is validated
- MPC firmware remains closed (if Akai opens VST3 standalone, this becomes less urgent)
- Patreon/funding reaches "Growth" tier ($5K/month) to justify hardware R&D time
