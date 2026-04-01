# D006 — Expression Input Is Not Optional

**Status:** RESOLVED — all MIDI-capable engines have velocity→timbre + CC response (Round 12C)

## Definition

Every MIDI-capable engine must respond to:
1. Velocity → timbre (see D001 for full requirements)
2. At least one continuous controller: aftertouch, mod wheel (CC1), or expression pedal (CC11)

Expression is what makes a synthesizer playable rather than merely programmable. An instrument that responds the same way regardless of how you play it is not an instrument.

## Requirements

- Velocity must shape timbre (not just amplitude) — see D001
- Aftertouch OR mod wheel OR expression pedal must route to at least one audible timbral parameter
- Both mod wheel and aftertouch are strongly preferred (they serve different performance contexts)
- The CC response must be audible — not a subtle effect that only appears in test conditions

## Exceptions

- Visual/utility engines (OPTIC) are exempt from aftertouch if they have no audio output
- Engines that serve as coupling sources only may have reduced expression requirements

## Common Failure Patterns

- Mod wheel handler registered but `modWheelAmount_` never applied to filter or LFO depth
- Aftertouch handler updates a variable but the variable is not in the signal path
- Expression pedal (CC11) not handled at all

## Fleet Compliance

- Mod wheel: 22/22 MIDI-capable engines — FULLY RESOLVED (Round 12C, 2026-03-14)
- Aftertouch: 23/23 engines — FULLY RESOLVED (OPTIC intentionally exempt — visual engine)
- Velocity→timbre: all engines — FULLY RESOLVED (Round 9E)

## Performance Context

**Aftertouch** is the continuous pressure applied after a note is struck. It is ideal for sustained expression: vibrato, filter swell, amplitude modulation during a held note.

**Mod wheel** is a pre-strike gesture control. It is ideal for setting a performance character before playing: depth of vibrato, filter brightness, resonance character.

Both are necessary because they serve orthogonal moments in performance.
