# D001 — Velocity Must Shape Timbre

**Status:** RESOLVED — all fleet engines compliant (Round 9E)

## Definition

Velocity drives filter brightness and harmonic content, not just amplitude. Harder notes must be brighter, richer, or more harmonically dense than soft notes. Amplitude scaling alone does not satisfy D001.

## Requirements

- Velocity must route to at least one timbral parameter (filter cutoff, harmonic brightness, resonance, formant, saturation drive, or equivalent)
- The velocity-to-timbre relationship must be audible across the full velocity range (1–127)
- Amplitude-only velocity response (simple volume scaling) is a D001 failure

## Common Failure Patterns

- Velocity wired only to output gain (`gainFactor = velocity / 127.0f`) with no timbral modulation
- Velocity-to-filter envelope amount of 0.0 (parameter exists but is not wired)
- Velocity read but stored and never applied to DSP

## Test Criteria

1. Play a note at velocity 1 and at velocity 127
2. Compare timbral character — brightness, density, or harmonic content must differ
3. If the only audible difference is loudness, D001 is failed

## Fleet Compliance

All 74 registered engines — COMPLIANT as of Round 9E (2026-03-14 Prism Sweep completion).
