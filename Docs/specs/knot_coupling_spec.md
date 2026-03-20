# KNOT Coupling — Technical Specification

**Feature:** V2 Theorem — approved Pi Day 2026-03-14
**CouplingType enum value:** `KnotTopology` (appended at end of enum — ordinal safe)
**Symbol:** The Kelp Knot
**Accent:** `#8E4585`

---

## What KNOT Is

All other `CouplingType` values in XOmnibus are directional: Engine A modulates Engine B's
parameter (sender → receiver). The route has a clear source slot and destination slot.

`KnotTopology` breaks this directionality. It creates **mutual, co-evolving entanglement**:

- Engine A's amplitude modulates Engine B's parameter
- Simultaneously, Engine B's amplitude modulates Engine A's corresponding parameter
- Both engines react to each other in the same processing pass
- You cannot remove one direction without destroying the coupling entirely

This mirrors the mathematical concept of a knot invariant: a configuration that cannot be
simplified to an unknot (a plain loop) without cutting the strand. Applied to synthesis:
a KNOT-coupled pair cannot be decoupled without fundamentally changing both engines' behavior.

---

## How the Linking Number Works

The **linking number** (1–5) controls how many parameter pairs are entangled. It is encoded
in the `CouplingRoute.amount` field using the full 0.0–1.0 range:

```
linkingNum   = juce::roundToInt(amount * 4.0f) + 1   →  [1, 5]
scaledAmount = static_cast<float>(linkingNum) / 5.0f
```

| amount (stored) | linkingNum | scaledAmount | Entanglement depth |
|----------------|------------|--------------|-------------------|
| 0.00           | 1          | 0.20         | Minimal — one parameter pair loosely entangled |
| 0.25           | 2          | 0.40         | Shallow — two pairs, audible mutual push-pull |
| 0.50           | 3          | 0.60         | Medium — three pairs, clear co-evolution |
| 0.75           | 4          | 0.80         | Deep — four pairs, strong mutual dependency |
| 1.00           | 5          | 1.00         | Maximum — five pairs, full irreducible tangle |

The `amount` field simultaneously encodes linking depth AND modulation strength. This is
a compact single-float encoding consistent with the `CouplingRoute.amount` convention
(0.0–1.0). Higher linking numbers produce perceptibly stronger entanglement because both
`scaledAmount` and the number of engine parameters being driven increase together.

---

## Signal Flow

`KnotTopology` routes are processed as **control-rate** modulation — the same decimated
path used by `AmpToFilter`, `LFOToPitch`, `EnvToMorph`, etc. (kControlRateRatio = 32,
~1.5 kHz at 48 kHz sample rate). KNOT modulation signals are inherently slow-moving
amplitude envelopes; audio-rate resolution is neither necessary nor beneficial.

The `processKnotRoute()` method in `MegaCouplingMatrix` executes two passes per block:

**Pass A (standard direction):**
```
fillControlRateBuffer(source)
dest->applyCouplingInput(AmpToFilter, scaledAmount, buffer, numSamples)
```

**Pass B (reverse direction — the KNOT difference):**
```
fillControlRateBuffer(dest)
source->applyCouplingInput(AmpToFilter, scaledAmount, buffer, numSamples)
```

Both passes reuse the same `couplingBuffer` scratch allocation (Pass A is complete before
Pass B overwrites it). Both directions apply identical `scaledAmount` — the entanglement
is symmetric by design.

`AmpToFilter` semantics are used as the universal fallback: every engine in the fleet is
guaranteed to handle `AmpToFilter` correctly. Future engines that want richer KNOT behavior
can inspect `CouplingType::KnotTopology` in their `applyCouplingInput()` implementation and
route accordingly, while still remaining compatible with the existing fallback path.

**Self-route guard:** routes with `sourceSlot == destSlot` are skipped silently. An engine
cannot form a KNOT with itself.

---

## Example Preset Patches

### "Chromatophore Mirror" — OCEANIC + OCTOPUS
- Slot A: OCEANIC (phosphorescent teal — ocean separation, bioluminescent fade)
- Slot B: OCTOPUS (chromatophore magenta — arm depth, color pulse)
- Route: `{ sourceSlot: 0, destSlot: 1, type: KnotTopology, amount: 0.75 }`
- Linking number: 4 — deep entanglement
- Result: OCEANIC's amplitude swell triggers OCTOPUS chromatophore spread; OCTOPUS arm
  activity folds back into OCEANIC's separation depth. The two engines breathe as one organism.
- Mood: Submerged / Entangled

### "Kelp Forest Resonance" — OPAL + ORACLE
- Slot A: OPAL (granular, lavender — grain size, scatter)
- Slot B: ORACLE (GENDY stochastic, prophecy indigo — breakpoint density)
- Route: `{ sourceSlot: 0, destSlot: 1, type: KnotTopology, amount: 0.50 }`
- Linking number: 3 — medium entanglement
- Result: OPAL grain clouds modulate ORACLE's stochastic breakpoint rate; ORACLE's
  unpredictable amplitude bursts reshape OPAL's grain scatter in real time. The patch
  evolves differently on every playback — true generative co-evolution.
- Mood: Aether / Entangled

### "Tidal Lock" — OVERDUB + ORGANON
- Slot A: OVERDUB (dub synth, olive — tape delay feedback, spring reverb decay)
- Slot B: ORGANON (metabolic engine, bioluminescent cyan — metabolic rate, energy)
- Route: `{ sourceSlot: 0, destSlot: 1, type: KnotTopology, amount: 1.00 }`
- Linking number: 5 — maximum tangle
- Result: OVERDUB's reverb tail amplitude drives ORGANON's metabolic rate (faster decay =
  higher metabolism); ORGANON's energy output feeds back into OVERDUB's tape delay
  regeneration. Once locked, the two engines sustain indefinitely in mutual resonance.
  Releasing all keys causes a slow mutual decay — neither engine falls silent first.
- Mood: Entangled / Atmosphere

### "Drift Prism" — ODYSSEY + OVERWORLD
- Slot A: ODYSSEY (wavetable/FM, violet — morph position, LFO depth)
- Slot B: OVERWORLD (chip/ERA, neon green — era crossfade, chip character)
- Route: `{ sourceSlot: 0, destSlot: 1, type: KnotTopology, amount: 0.25 }`
- Linking number: 2 — shallow entanglement
- Result: ODYSSEY wavetable drift nudges OVERWORLD's ERA crossfade; OVERWORLD's chip
  amplitude textures feed subtle modulation back into ODYSSEY's morph depth. At low
  linking numbers the entanglement is subliminal — the patch "breathes" without an
  obvious modulation source. Ideal for long ambient pieces.
- Mood: Atmosphere / Flux

---

## Which Engine Pairs Make Interesting KNOT Patches

### Tier 1 — Strongest natural entanglement (recommended for presets)

| Pair | Why |
|------|-----|
| OPAL + ORACLE | Granular clouds ↔ stochastic breakpoints — unpredictable generative loops |
| OVERDUB + ORGANON | Reverb tail ↔ metabolic rate — sustain lock, mutual resonance |
| OCEANIC + OCTOPUS | Bioluminescent swell ↔ chromatophore pulse — visual + timbral co-evolution |
| OUROBOROS + ORIGAMI | Chaos topology ↔ fold point — nonlinear feedback, on the edge of instability |
| OVERWORLD + OPAL | ERA crossfade ↔ grain scatter — retro-to-granular textural weave |

### Tier 2 — Strong character contrast (interesting tension)

| Pair | Why |
|------|-----|
| ODYSSEY + OVERWORLD | Wavetable drift ↔ chip ERA — smooth vs. quantized timbral exchange |
| OBLONG + OBBLIGATO | Resonant bass ↔ dual wind breath — physical model cross-breathing |
| OHM + OHROPAX | Hippy drone ↔ percussive — slow commune axis hitting a fast transient |
| ORACLE + OUROBOROS | GENDY stochastic ↔ chaos attractor — two unpredictable engines mutually constraining each other |
| OVERLAP + OCEANIC | FDN reverb topology ↔ ocean separation — spatial depth locked to timbral depth |

### Tier 3 — Subtle / atmospheric (long-form pieces)

| Pair | Why |
|------|-----|
| ORPHICA + OMBRE | Microsound harp ↔ dual-narrative blend — delicate mutual sculpting |
| OSPREY + OSTERIA | Shore system shared — KNOT reinforces the pre-existing cultural bond |
| OBSIDIAN + OBSCURA | Crystal resonance ↔ stochastic stiffness — fragile, glass-like entanglement |

---

## Design Rationale

### Why not add a separate "reverse route" instead?

A user could manually add two routes: A→B (AmpToFilter) and B→A (AmpToFilter). This would
produce similar output but loses semantic meaning. With two separate routes:

- The UI shows two independent arrows, not a bond
- Removing one direction does not signal intent — it just breaks the patch silently
- The linking number concept has no representation
- Preset authors cannot express "these engines are one thing" in the data model

`KnotTopology` preserves intent. A preset that uses KNOT communicates: "these two engines
are irreducibly bound — this is the patch, not a side effect of routing."

### Why AmpToFilter as the universal fallback?

Every engine in the XOmnibus fleet handles `CouplingType::AmpToFilter`. It maps source
amplitude to a filter cutoff modulation destination — a safe, audible, musically useful
result for any pairing. Using it as the KNOT fallback guarantees that:

1. KNOT patches work on all 34 current engines without any engine-side changes
2. New engines get KNOT compatibility for free by implementing AmpToFilter (which they
   already must do for normalled routes)
3. Engine authors can opt into richer KNOT behavior by checking for `KnotTopology`
   explicitly — this is additive, not breaking

### Why control-rate, not audio-rate?

KNOT models parameter entanglement, not audio-rate feedback (which would require
AudioToFM or AudioToRing). The signals being exchanged are amplitude envelopes — slowly
evolving values where 1.5 kHz control resolution is indistinguishable from audio rate
to the human ear. Control-rate decimation saves ~97% of `getSampleForCoupling` calls
with zero perceptible quality loss (same SRO optimization used by all other modulation
coupling types).

If audio-rate bidirectional feedback is desired in a future theorem, it should be a
separate coupling type (e.g., `AudioFeedbackRing`) — not an extension of KNOT.

---

## Implementation Locations

| File | Change |
|------|--------|
| `Source/Core/SynthEngine.h` | `KnotTopology` added at end of `CouplingType` enum |
| `Source/Core/MegaCouplingMatrix.h` | KNOT dispatch in `processBlock()`; `processKnotRoute()` private method |
| `Docs/specs/knot_coupling_spec.md` | This document |

The enum value `KnotTopology` is appended as the last entry in `CouplingType`. All
existing enum values retain their ordinal positions. No serialized preset data references
coupling types by integer ordinal — presets store engine IDs and slot indices only —
so this addition is fully backward compatible.
