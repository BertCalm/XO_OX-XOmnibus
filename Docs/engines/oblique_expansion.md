# Oblique Preset Expansion

**Date**: 2026-03-14
**Seance score before**: 5.9/10 (lowest in fleet; "6 presets only")

## What Was Added

10 new presets across 4 moods, raising Oblique from 6 to 16 presets total.

### Foundation (2 new)
- **Oblique Clean Stab** — Sine wave, minimal fold, single bounce tap, no phaser. Engine stripped bare for direct, genre-agnostic use.
- **Oblique Bounce Pluck** — Saw wave, light fold, exposed bounce cascade (6 taps, medium gravity). Rhythm lives in the click decay; works for basslines and leads.

### Atmosphere (2 new)
- **Prism Drift** — Triangle wave held long through 320ms prism reflections, slow phaser swell, minimal clicks. Pure ambient meditation.
- **Coral Scatter** — Square wave with 14-bounce cascade, gentle swing, wide prism spread. Organic, alive texture — like listening to a reef.

### Prism (2 new, total 8)
- **UV Pulse** — Pulse wave with aggressive fold (0.75), 7kHz click tone, high prism color. Maximum chromatic brightness.
- **Glass Arp** — Triangle with moderate fold and short decay. High bounce count creates self-arpeggiation. The click cascade IS the melody.

### Flux (2 new)
- **Gravity Flux** — Max bounce count, extreme gravity (0.35 = fastest acceleration), glide enabled. Dense accelerating cascade that collapses unpredictably.
- **Prism Storm** — High prism feedback near self-oscillation (0.78) with fast 4Hz phaser. Held notes build into a spectral storm.

### Entangled (2 new)
- **Onset Ricochet** — ONSET drum hits trigger Oblique's bounce cascade via AmpToFilter + RhythmToBlend coupling. Every kick/snare fires a prismatic ricochet.
- **Overdub Fragment** — Oblique's prism output feeds Overdub's tape delay. Bridges feliX-surface brightness with Oscar-depth warmth. Classic dub echo architecture.

## Coverage Assessment

| Mood       | Before | After |
|------------|--------|-------|
| Foundation | 0      | 2     |
| Atmosphere | 0      | 2     |
| Prism      | 6      | 8     |
| Flux       | 0      | 2     |
| Entangled  | 0      | 2     |
| Aether     | 0      | 0     |

Aether remains empty and is the next expansion target.

## Snap Sweep Bidirectionality (co-delivered)

`snap_sweepDirection` parameter added to SnapEngine (float, -1.0 to +1.0, default -1.0).

- -1.0 = classic downward sweep (starts 24st above target) — all legacy presets unchanged
- +1.0 = upward sweep (starts 24st below target) — effect snares, pitched toms, risers
- Values between: partial sweep, useful for subtle pitch-upward texture

Implementation: `voice.currentPitch = baseMidiNote - sweepDir * kMaxPitchSweepSemitones * snapAmount`

The negation ensures -1.0 (downward) produces a positive semitone offset (starts above), and +1.0 (upward) produces a negative offset (starts below). Default -1.0 is backwards-compatible with all existing presets.
