# Voice Mode Completion — Round 11D

**Date:** 2026-03-14
**Scope:** Oblique glide fix + Orbital/Ocelot voice mode audit

---

## 1. Oblique — Unconditional Glide Bug Fixed

### Problem

`ObliqueEngine.h` had a glide condition at the normal (Poly/Mono) voice allocation path:

```cpp
// BEFORE (buggy):
if (glideTime > kMinGlideTime && voice.currentFrequency > kMinFrequencyForGlide)
    voice.targetFrequency = noteFrequency;
else { snap... }
```

The check `voice.currentFrequency > kMinFrequencyForGlide` was true for any voice
that had ever been used — even one that had fully released and was being freshly
reallocated. Result: staccato notes glided from the *previous* note's pitch even
when that previous note had finished, producing unintended portamento whenever
`oblq_glide > 0`.

### Fix Applied

Captured `wasAlreadyActive` **before** resetting `voice.active`:

```cpp
// AFTER (fixed):
const bool wasAlreadyActive = voice.active && voice.currentFrequency > kMinFrequencyForGlide;
// ...reset voice state...
if (wasAlreadyActive && glideTime > kMinGlideTime)
    voice.targetFrequency = noteFrequency;   // glide from outgoing pitch
else { snap to noteFrequency immediately }
```

`wasAlreadyActive` is true only when the voice was still playing (gate open or
being stolen) at the time a new note arrives. Freshly allocated idle voices
always snap to pitch regardless of the glide setting.

### Pattern alignment

This matches the Origami/Morph pattern described in the Round 11D brief.
Oblique already had `oblq_voiceMode` (added Round 10F) and correct Legato-mode
handling. Only the Poly/Mono path was defective.

**File modified:** `Source/Engines/Oblique/ObliqueEngine.h`
Lines ~1537–1578 (voice allocation + frequency assignment).

---

## 2. Orbital — `orb_voiceMode` Added

### Justification

Orbital is a 64-partial additive synthesis engine with long-sustaining, pad-like
voices. It is used for melodic lines (bell tones, evolving pad chords, spectral
leads). Legato playing — connecting notes without retriggering the 4-group envelope
system — is musically valuable and expected by players using this engine.

### What Was Added

**Parameter:** `orb_voiceMode` (Choice, version 1)
- `0` = Poly (default — always retrigger, full polyphony. Preserves all existing preset behavior.)
- `1` = Mono (single voice, always retrigger)
- `2` = Legato (single voice, suppress envelope retrigger when gate open)

**New private method:** `legatoSlideVoice(voice, newNote, inharmonicity, fmRatio)`

When a gate-open voice is found in Legato mode, this method:
1. Updates `voice.noteNumber` and `voice.fundamentalHz` to the new note.
2. Recomputes all 64 `phaseIncrement[]` and `fmPhaseIncrement[]` arrays.
3. **Does NOT reset** `voice.phase[]` or `voice.fmPhase[]` — the partial waves
   continue unbroken, creating a smooth pitch transition (additive synthesis legato).
4. Sets `formantDirty = true` so the formant filter rebuilds for the new fundamental.

**Envelope behavior:** The existing ADSR stages (`envStage`, `groupStage[]`) are
unchanged — the legato note inherits the envelope position of the previous note,
so long-attack presets don't re-bloom.

**Files modified:**
- `Source/Engines/Orbital/OrbitalEngine.h`
  - `addParametersImpl()`: added `orb_voiceMode` AudioParameterChoice
  - `attachParameters()`: wired `p_voiceMode`
  - `renderBlock()` ParamSnapshot section: reads `voiceMode`
  - MIDI loop: legato detection before `triggerVoice` call
  - New private method `legatoSlideVoice()`
  - Member field `p_voiceMode`

---

## 3. Ocelot — Voice Mode Not Added (Acceptable Gap)

### Assessment

`OcelotEngine` delegates to `OcelotVoicePool` → `OcelotVoice`, which manages
four independent strata:

| Stratum | Type | Pitch-sensitive? |
|---------|------|------------------|
| Floor | Physical percussion (berimbau, kalimba, etc.) | Loosely (tuning param) |
| Understory | Sample mangler / tape effect | No |
| Canopy | Spectral additive pad | Yes (MIDI note → partial frequencies) |
| Emergent | Creature call sounds | Indirectly (creaturePitch param) |

### Why Legato is Acceptable to Skip

1. **Identity conflict.** Ocelot is an ecosystem atmosphere engine — its identity
   is layered textural/percussive character, not melodic lines. Legato semantics
   are ambiguous: the Floor stratum is inherently percussive and *should* retrigger
   on each note. Suppressing retriggering for the whole voice to support Canopy
   legato would break Floor's percussive behavior.

2. **Architecture cost.** Adding `ocelot_voiceMode` would require threading the
   param through `OcelotParamSnapshot` → `OcelotVoicePool::noteOn()` →
   `OcelotVoice::noteOn()` → and selective retrigger suppression per stratum.
   This is a multi-file change for minimal musical benefit given the engine's
   intended use.

3. **No seance demand.** The 24 seances did not flag Ocelot as lacking legato
   — its character is atmosphere, chop, and ecosystem interaction, not linear
   melody.

4. **`ocelot_swing` / `ocelot_density` / ecosystem params** provide enough
   variation for textural playing without needing legato.

**Verdict:** Ocelot remains Poly-only. This is musically appropriate for the
engine's identity as a multi-stratum ecosystem texture engine.

---

## 4. Snap — Voice Mode Not Added (Explicitly Acceptable)

Snap is the percussive "OddfeliX" engine. Percussive engines always retrigger
on every note — legato is not a meaningful concept for drum/pluck synthesis.
Confirmed acceptable per the Round 11D brief.

---

## 5. Fleet-Wide Voice Mode Coverage After Round 11D

| Engine | voiceMode param | Legato | Notes |
|--------|----------------|--------|-------|
| Oblique | ✅ `oblq_voiceMode` (Round 10F) | ✅ (legato was already correct; **glide bug fixed Round 11D**) | |
| Orbital | ✅ `orb_voiceMode` (**added Round 11D**) | ✅ via `legatoSlideVoice()` | |
| Origami | ✅ `origami_voiceMode` | ✅ | Reference implementation |
| Morph | ✅ `morph_voiceMode` (Round 9D) | ✅ | |
| Ocelot | ❌ not added | — | Acceptable: multi-stratum ecosystem engine |
| Snap | ❌ not needed | — | Acceptable: percussive engine by design |
| Other engines | varies | varies | Out of scope for Round 11D |

---

## Summary

Round 11D delivered two targeted changes:

1. **Oblique glide bug**: staccato notes no longer glide unintentionally when
   `oblq_glide > 0`. Fixed by capturing `wasAlreadyActive` before voice reset.

2. **Orbital voiceMode**: `orb_voiceMode` (0=Poly/1=Mono/2=Legato) added.
   Legato mode preserves the envelope bloom while sliding pitch via
   `legatoSlideVoice()` — appropriate for Orbital's long-sustain additive character.

Both Ocelot and Snap are justified as acceptable gaps, for ecosystem-texture and
percussive reasons respectively.
