# Copilot Instructions — XOceanus (XO_OX-XOmnibus)

Full project context lives in [`CLAUDE.md`](../CLAUDE.md) at the repo root. Read that first.

## Key conventions

- **Language:** C++ (JUCE framework), AU/VST3 audio plugin
- **Engine naming:** All engines follow the `XO + O-word` pattern (ONSET, OWARE, OXYTOCIN, etc.)
- **Presets:** `.xometa` JSON format with 6D Sonic DNA; stored in `Presets/XOceanus/`
- **Dark mode default:** UI palette uses dark-primary design; light mode is a toggle
- **No audio-thread allocations:** Never allocate memory on the audio thread
- **Sample rate:** Always derive from `AudioProcessor::getSampleRate()` — never hardcode 44100

## Directory map

```
Source/          C++ engine and DSP source files
Presets/         Factory preset library (.xometa files)
Docs/            Design docs, specs, session notes, fleet audits
  engines/       Engine-specific architecture docs
  seances/       Seance verdict reports
  fleet-audit/   Fleet-wide audit and health reports
  sweeps/        Automated sweep reports
  sessions/      Session handoffs and morning plans
  specs/         Detailed feature specifications
  design/        UI/UX design docs
.github/         CI workflows, issue templates, this file
```

## Critical patterns (from CLAUDE.md)

- After `await`, re-read DSP state — closures go stale
- Use `cancelAndHoldAtTime` when interrupting envelopes (prevents clicks)
- IIR filter coefficients: use matched-Z (`exp(-2*PI*fc/sr)`), not Euler approximation
- Bipolar modulation: check `!== 0`, not `> 0` — negative amounts sweep downward
- Runtime-validate parsed strings before `as` type assertions on external files

## Docs reorganization (in progress)

Root-level `Docs/` files are being migrated into subdirectories (issue #60).
Use the subdirectory map above when creating new documentation.
