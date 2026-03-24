# XPN Sample Curation Philosophy — R&D Spec
**Date**: 2026-03-16
**Status**: Canonical

---

## 1. The Sample Selection Philosophy

Every sample that enters an XO_OX expansion pack must pass three tests. Fail one, and it doesn't ship — regardless of technical quality.

**The Gut Test**: Does it make you want to reach for a pad immediately? Not "this is technically clean" or "this would work." The gut test is about pull. When a producer hears a sample for the first time, their hand should move toward the hardware before their brain finishes processing. If you find yourself rationalizing a sample into the pack ("it's versatile," "producers will find uses for it"), it already failed. The gut test is binary.

**The Context Test**: Does the sample serve its role without requiring the producer to fight it? A kick drum that needs 6dB of 200Hz shelved before it sits in a mix is a production problem disguised as a sample. A snare that clashes with every kick in the kit until you notch it is the same problem. Context fitness means the sample drops into a session and occupies its frequency and dynamic territory without negotiation. This doesn't mean every sample must be neutral — it means every sample must be honest about what it is and where it lives.

**The Time Test**: Will this sample still work in a track six months from now? Trend-chasing is the fastest way to build a pack that feels dated before it ships. The question isn't whether a sound is contemporary — it's whether the sound has enough structural identity to outlast the trend that spawned it. A 808 sub doesn't age. A vocal chop pitched to match last season's dominant key center does.

---

## 2. Technical Requirements

These are non-negotiable minimums, not targets.

- **Resolution**: 44.1kHz / 24-bit minimum. Anything below this floor doesn't enter the pipeline.
- **Room character**: No pre-baked room sound unless the room is intentional vintage character (drum room, tape machine, spring reverb decay). If a sample has ambient bleed, that bleed must be the point — not an artifact of a careless recording session.
- **Clipping**: Zero clipping at any velocity layer. Check all four layers independently. A sample that clips only on Layer 4 (accent) still fails.
- **DC offset**: Below 0.05. Run a DC offset check before any other processing. DC-contaminated samples phase-cancel unpredictably when layered.
- **Start point**: True zero-crossing. Clicks at sample start are a sequencing liability — they appear at different velocities, in different DAW buffer sizes, at different tempos. Eliminate them at the source.
- **Tail length**: Silence tail at 200ms or less, unless the tail is the sound. Reverb, resonant decay, and the physical release of an acoustic instrument are sounds. Dead silence following a sample that finished 800ms ago is wasted kit space.

---

## 3. Character Over Neutrality

XO_OX packs are not tools. They are instruments with a point of view.

The sample library market has an abundance of neutral, well-recorded, professionally processed sounds that do nothing interesting and offend no one. That is not what XO_OX ships. Every pack carries the feliX/Oscar polarity at its core, and every sample should reflect which side of that axis it occupies.

**feliX samples** are precise. Sharp transients, clear high-frequency content, fast attack envelopes, forward presence in the mix. A feliX kick drum lands exactly where the grid says it should and occupies the upper-low-end with surgical confidence. A feliX hat has cut, not shimmer.

**Oscar samples** are organic. Decay is the character, not the aftermath. Lower harmonic complexity, slower envelope behavior, textures that breathe. An Oscar pad tail is not the sample fading out — it is the sound continuing to exist in its own time. An Oscar snare's body matters more than its attack.

Samples that live in the middle — that could be feliX or Oscar depending on the producer's mood — belong in utility libraries. XO_OX packs make a commitment and hold it.

---

## 4. The Ensemble Test

A kit is not a collection of individually good samples. It is an ensemble that must function as a unit.

Samples in the same kit should share an acoustic logic — either the same recording environment, or a deliberately committed contrast (close-mic vs. room, dry vs. wet, acoustic vs. synthetic). Half-and-half is not a concept. It is ambivalence.

Tonal coexistence is required. The kick and snare should occupy different frequency centers without manual surgery. If the kick is centered around 60-80Hz and the snare is centered around 200-250Hz, the tom should not also be centered at 200Hz. Curators are responsible for the harmonic map of the entire kit, not just individual elements.

Velocity coherence means the instrument stays the same instrument across all four layers. Louder does not mean different — it means more of the same character, pushed further. A kit that sounds like four different sample libraries layered together has failed the ensemble test.

---

## 5. Uniqueness Standard

Before a sample is confirmed for inclusion, ask one question: is this substantially different from anything already in the XO_OX catalog?

Run `xpn_sample_fingerprinter.py` against the full catalog before finalizing any pack. Near-duplicate detection catches the obvious failures — a reskinned version of an existing kick, a pad that shares spectral DNA with three sounds already in OPAL. The goal is a catalog where every sample earns its unique position.

The target is 80% original content: XOlokun engine renders or custom recordings. The remaining 20% is ceiling, not standard.

---

## 6. Layer Relationship Design

The four velocity layers are not four versions of the same sample. They are four moments in the life of the same instrument.

- **Layer 1 (ghost)**: The sound at rest. Intimate, soft, played with minimum effort. This layer should feel like it costs nothing — like the instrument barely moved.
- **Layer 2 (medium)**: The body of the sound. This is the layer most producers will spend the most time in. It should be the most balanced, the most useful, the most honest representation of the instrument.
- **Layer 3 (hard)**: Timbral shift, not just volume increase. More harmonic complexity, brighter presence, increased attack. Something in the sound's character changes as force increases — because that is what real instruments do.
- **Layer 4 (accent)**: Timbral peak. Cracked, saturated, percussively intense. This layer should feel like the instrument is giving everything. It should not simply be Layer 3 louder.

If four consecutive velocity layers sound like the same sample at four gain stages, the layer design failed.

---

## 7. Source Priority

The sourcing hierarchy is fixed. Exceptions require explicit justification.

1. **XOlokun engine renders** — the primary source, using the renderNoteToWav() pipeline once available. Every pack should be a product of the instrument ecosystem.
2. **Custom recordings** — field recording, acoustic instruments, hardware synths captured in controlled conditions. Original material that cannot exist anywhere else.
3. **Pre-recorded samples processed through XOlokun engines** — source material transformed through the engine signal chain. The transformation must be audible and meaningful, not cosmetic.
4. **Stock library samples** — last resort, limited to texture and FX layers only. Never for primary kit voices (kick, snare, hat). Document source and license in the pack manifest.

The goal of this hierarchy is a catalog that sounds like XO_OX made it — not a catalog that sounds like XO_OX assembled it.
