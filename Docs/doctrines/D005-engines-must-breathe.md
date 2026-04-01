# D005 — An Engine That Cannot Breathe Is a Photograph

**Status:** RESOLVED — all engines have LFO with rate floor ≤ 0.01 Hz (Round 5A + engine recoveries)

## Definition

Every engine must have at least one LFO with a rate floor at or below 0.01 Hz. A static sound is not an instrument — it is a snapshot. The engine must be capable of autonomous, slow movement over time.

A rate floor of 0.01 Hz = a full LFO cycle in ~100 seconds. This creates breathing, living motion that the performer can set and forget. Without this floor, engines are frozen.

## Requirements

- At least one LFO must have a rate floor ≤ 0.01 Hz
- The LFO must be wired to an audible timbral target (filter, pitch, amplitude, waveshape)
- The rate must be continuously variable from the floor to at least 10 Hz
- The LFO must operate independently of note on/off (it breathes even between notes)

## Rationale

From the ghost council (Schulze, Vangelis): "A synthesizer that cannot breathe slowly is a toy. The long cycle — the thing that takes minutes to complete — is where synthesis becomes environmental. The performer sets a direction and lets the instrument travel."

## Common Failure Patterns

- LFO rate parameter minimum set to 0.1 Hz or higher (10x too fast for slow breathing)
- LFO rate parameter minimum set to 0.0 Hz (correct floor), but the LFO only starts on note-on (no persistent breathing)
- LFO depth wired to 0.0 default with no user control to increase it

## Test Criteria

1. Set LFO rate to minimum — verify it is ≤ 0.01 Hz
2. Set LFO depth to maximum
3. Play a note, release it
4. Wait 30 seconds — verify the engine is still producing audible slow modulation
5. If the modulation stops when the note ends, D005 is marginal (acceptable for gated engines, not for pads)

## Fleet Compliance

All 74 registered engines — COMPLIANT.
Rate floors verified across all engines. Engines with gate-responsive LFOs (percussion, transient engines) have secondary slow LFOs that breathe independently.
