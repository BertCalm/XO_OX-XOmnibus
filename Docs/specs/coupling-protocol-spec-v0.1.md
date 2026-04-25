# XO_OX Coupling Protocol — Specification v0.1

**Version:** 0.1
**Status:** Release Candidate
**Date:** 2026-03-22
**Author:** XO_OX Designs
**Repository:** https://github.com/BertCalm/XO_OX-XOceanus

---

## Abstract

The XO_OX Coupling Protocol defines a lightweight standard for real-time, directed communication between independent synthesis engines. It specifies how one engine's live behavior — its amplitude, envelope shape, or spectral content — can continuously modulate parameters on another engine, creating emergent musical relationships that neither engine could produce in isolation. Version 0.1 publishes three core coupling types (Sympathetic, Complementary, and Responsive), a binary message format for control-rate packet transmission, and a three-tier compliance model for hosts and engine implementors. The reference implementation is XOceanus, a free open-source multi-engine synthesizer by XO_OX Designs, which implements all three public types plus eleven additional extended types.

---

## 1. Motivation

MIDI says "play this note." Coupling says "relate to this voice like this."

MIDI is a vocabulary for events. It describes what to play — a note starts at this pitch, at this velocity, and stops here. DAW automation extends this to parameter curves over time. Neither describes the *relationship* between simultaneously-playing voices.

When a jazz guitarist comps behind a horn player, something more subtle is happening than MIDI note events. The guitarist listens to the horn's dynamics and pulls back. The horn player hears the chord voicing and inflects accordingly. They are *coupled* — each voice actively shaping how the other behaves, in real time, in response to what they hear.

Layering two synthesizers on the same MIDI bus gives you additive sound. Coupling them gives you a conversation.

Conventional modulation (LFOs, envelopes, mod matrix slots) is self-contained: Engine A modulates its own parameters according to its own internal state. Coupling is different. The source of modulation is *another engine's live output* — observed externally and applied across a relationship boundary. Modulation is monologue. Coupling is dialogue.

Without a shared protocol, inter-engine relationships are proprietary to each implementation and cannot be exchanged, versioned, or extended by third parties. The XO_OX Coupling Protocol is the shared contract that makes these relationships portable.

---

## 2. Terminology

**Source Engine** — The engine whose live output or internal state drives a coupling route. The source emits a coupling signal. It is not modified by the route it originates.

**Target Engine** — The engine whose parameters are modulated by incoming coupling signal. The target receives a packet and applies it to one or more of its parameters.

**Route** — A single directed relationship: one source engine, one target engine, one type, one amount. Multiple simultaneous routes between the same engine pair are permitted.

**Amount** — A normalized scalar `[0.0, 1.0]` controlling coupling depth. At `0.0` the route is present but inert. At `1.0` the coupling is applied at full declared strength. Intermediate values scale linearly. Values outside this range are clamped by the receiver.

**Type** — A uint8 identifier specifying what signal is extracted from the source and what parameter is driven on the target. The type defines the semantic relationship. This spec defines types `0x01`–`0x03`.

**Control Rate** — Coupling packets are delivered at block boundaries (typically every 32–512 samples, ~100 Hz–1.5 kHz at standard sample rates), not per-sample. This is intentional: coupling encodes musical relationships, not audio content.

---

## 3. Core Types — Version 0.1

Version 0.1 defines three public coupling types with permanent type IDs. Implementations claiming v0.1 compliance must support at least one type at the appropriate compliance level (see Section 6).

---

### 3.1 Sympathetic — `type = 0x01`

**Descriptor:** Parallel resonance. Source engine's amplitude and spectral content reinforce the target's harmonics.

**Conceptual model:** Two strings mounted on the same acoustic body. When one sustains, the other begins to vibrate in sympathy — not because it was struck, but because it shares a resonant environment. The more the first string sings, the more the second blooms alongside it.

**Signal flow:**

```
Source Engine
  │
  │  amplitude RMS (normalized, block-average)
  │  spectral center frequency (normalized)
  │
  └─────────────────────────────────────────► [Coupling Host]
                                                     │
                                               amount [0.0–1.0]
                                                     │
                                                     ▼
                                              Target Engine
                                              ├─► filter resonance boost
                                              ├─► harmonic content / morph position
                                              └─► sustained-level reinforcement
```

The target does not copy the source's pitch verbatim. It receives the source signal as a *bias* applied additively to its own current state. The target's independent voice is preserved; it is pulled into harmonic alignment, not replaced by the source.

**Parameter range:**

| Parameter | Range | Notes |
|-----------|-------|-------|
| `amount` | 0.0 – 1.0 | 0 = no effect; 1.0 = full sympathetic pull |
| pitch bias range | ±24 semitones | Declared by target as max deviation at `amount=1.0` |
| filter range | ±4000 Hz | Declared by target as max shift at `amount=1.0` |

**Musical use cases:**
- Two pad engines sharing a register: one thickens as the other sustains
- Bass + chord voicing: chord gains harmonic density when bass note blooms
- Long-attack lead + textural layer: texture develops as lead reaches peak amplitude
- Droning engine that brightens when another voice enters its overtone series

**Implementation notes:** Extract source RMS over the current processing block. Apply a one-pole smoothing filter (time constant ~20ms) before forwarding as the coupling signal to prevent step artifacts at note boundaries. Targets are not required to implement both resonance and morph modulation; implementing either constitutes valid Sympathetic listener support.

---

### 3.2 Complementary — `type = 0x02`

**Descriptor:** Inverse fill. Source engine's presence causes the target to recede or fill the vacated space.

**Conceptual model:** A rhythm section breathing. When the kick hits, the sub momentarily yields. When the lead phrase ends, the pad swells into the silence. The two voices do not compete — they occupy opposing moments, creating a conversation rather than a collision.

**Signal flow:**

```
Source Engine
  │
  │  amplitude RMS (normalized, block-average)
  │
  └─────────────────────────────────────────► [Coupling Host]
                                                     │
                                               inversion:
                                               choke_signal = 1.0 − source_rms
                                                     │
                                               amount [0.0–1.0]
                                                     │
                                                     ▼
                                              Target Engine
                                              ├─► output level × (1.0 − amount × source_rms)
                                              └─► filter cutoff opens as source recedes
```

The inversion is what defines Complementary. A loud source ducks the target. A silent or decayed source releases the target — the target fills the space vacated by the source's decay. This is timbral and dynamic complementarity, not sidechain gain reduction.

**Parameter range:**

| Parameter | Range | Notes |
|-----------|-------|-------|
| `amount` | 0.0 – 1.0 | 0 = no choke; 1.0 = full inverse relationship |
| `floor_level` | 0.0 – 0.5 | Minimum target level when fully choked (default: 0.1) |
| `response_time` | 1–500 ms | Envelope follower attack/release (default: 20ms) |

**Musical use cases:**
- Kick drum + sustained pad: pad breathes open in the spaces between hits
- Lead melody + chord layer: chords recede during melodic phrases, swell in rests
- Percussion + ambient texture: texture occupies the silence between rhythmic events
- Call-and-response arrangement: one engine speaks, the other listens and fills

**Implementation notes:** Apply a smoothed amplitude follower on the source (attack ~1ms, release ~80ms, adjustable). The `floor_level` parameter prevents total silence and avoids the uncanny sensation of a voice vanishing — the default of `0.1` keeps the target audible at 10% amplitude even at full choke. Do not apply the inversion at audio rate; this is a dynamic shaping relationship, not ring modulation.

---

### 3.3 Responsive — `type = 0x03`

**Descriptor:** Dynamic shaping. Source engine's amplitude envelope modulates target's timbre (filter cutoff, morph position).

**Conceptual model:** One instrument reacting to another's energy. When a guitar pick attack hits, the synth filter snaps open. When a piano note swells into sustain, the pad's wavetable position follows. The source's dynamic contour — its attack transient, sustain level, release shape — becomes a continuous timbral gesture inscribed onto the target.

**Signal flow:**

```
Source Engine
  │
  │  envelope follower output
  │  (attack transient + sustained amplitude, smoothed)
  │
  └─────────────────────────────────────────► [Coupling Host]
                                                     │
                                               amount [0.0–1.0]
                                                     │
                                                     ▼
                                              Target Engine
                                              ├─► filter cutoff  + (amount × envelope × 8000 Hz)
                                              └─► morph position + (amount × envelope × 1.0)
```

Unlike Sympathetic (which reinforces) or Complementary (which inverts), Responsive is a direct proportional mapping: the source's dynamic shape becomes the target's timbral shape. High source amplitude opens the target's filter and advances its morph. Decay in the source returns the target toward its preset values.

**Parameter range:**

| Parameter | Range | Notes |
|-----------|-------|-------|
| `amount` | 0.0 – 1.0 | 0 = no effect; 1.0 = full timbral response |
| `env_range` | ±8000 Hz | Max filter deviation at `amount=1.0` and `envelope=1.0` |
| `morph_range` | 0.0 – 1.0 | Max morph position shift at `amount=1.0` and `envelope=1.0` |

**Musical use cases:**
- Drum transients opening a pad's filter on each hit
- Lead melody shaping a textural layer's wavetable position in real time
- Bass envelope driving a chord engine's filter cutoff
- Rhythmic source creating rhythmic timbral animation on a sustained target
- Any scenario where one voice's energy should be *felt* in another voice's character

**Implementation notes:** The source engine does not need to perform envelope following itself. The host matrix is responsible for computing the envelope follower on the source's audio output and forwarding the result as the `signal` field of the coupling packet. Recommended envelope follower settings: attack 2ms (capture transients), release 200ms (track phrase contour). Morph modulation should be applied additively to the target's current preset morph value, not as a replacement.

---

## 4. Message Format

Coupling data is transmitted as a **control-rate packet** delivered by the host matrix to target engines at block boundaries, before each render call.

### 4.1 CouplingPacket (16 bytes)

```
Offset  Type    Field       Description
------  ------  ----------  --------------------------------------------------
0       uint8   version     Protocol version. 0x01 for this specification.
1       uint8   type        Coupling type ID (0x01, 0x02, or 0x03 for v0.1).
2       uint16  source_id   Host-assigned source engine identifier.
4       uint16  target_id   Host-assigned target engine identifier.
6       float32 amount      Route depth [0.0, 1.0] (configured at route setup).
10      float32 signal      Current normalized source signal [0.0, 1.0].
14      uint8   flags       Reserved. Set to 0x00.
15      uint8   reserved    Alignment padding. Set to 0x00.
```

Total: 16 bytes per packet.

**`version`** — Always `0x01` for this specification. Receivers encountering an unknown version must silently ignore the packet.

**`type`** — The coupling type. `0x01` = Sympathetic, `0x02` = Complementary, `0x03` = Responsive. See Section 8 for type ID allocation across future public and extended ranges.

**`source_id` / `target_id`** — Opaque identifiers assigned by the host. The protocol does not mandate assignment strategy; IDs must be consistent within a session but need not persist across sessions.

**`amount`** — The user-configured coupling depth for this route. Engines apply this as a final depth scalar to their computed effect.

**`signal`** — The current normalized output of the source engine's coupling extraction, updated each block. For Sympathetic and Complementary, this is the source RMS. For Responsive, this is the envelope follower output. Always `[0.0, 1.0]`.

The `type` field in `CouplingPacket` is a protocol-level identifier distinct from the C++ `CouplingType` enum used in the reference implementation. The host is responsible for mapping between the two.

### 4.2 Transmission Model

The host matrix delivers packets to each target engine before its render block is called each processing cycle. This guarantees coupling state is available at the start of synthesis. Per-block delivery is sufficient for all three v0.1 types; implementations requiring sample-accurate coupling must document the deviation.

### 4.3 Multiple Routes

A target engine may receive multiple packets per block. Targets accumulate all incoming signals. There is no defined priority ordering; implementations may use additive, max, or weighted-blend accumulation as appropriate to the modulated parameter.

### 4.4 No-Op Packets

A packet with `amount = 0.0` or `signal = 0.0` is a valid no-op and may signal route deactivation without removing the route. Receivers must handle it without error.

### 4.5 Host Dispatch

The `CouplingPacket` carries a single scalar `signal` value representing the source engine's
normalized output at control rate. The reference implementation's C++ interface accepts a
per-sample audio buffer:

    void applyCouplingInput(CouplingType type, float amount,
                            const float* sourceBuffer, int numSamples);

The host is responsible for converting between these representations. For control-rate
coupling types (Sympathetic, Complementary, Responsive), the host fills a block-sized
buffer with the scalar value:

    float buffer[blockSize];
    std::fill(buffer, buffer + blockSize, packet.signal);
    engine->applyCouplingInput(mapType(packet.type), packet.amount, buffer, blockSize);

For audio-rate coupling types (extended, not part of this v0.1 spec), the host passes
the source engine's raw audio buffer directly, bypassing the packet format entirely.
The `CouplingPacket` is a control-rate protocol; audio-rate coupling is an internal
optimization within the reference implementation.

---

## 5. Compliance Levels

Three compliance levels allow engines and hosts to adopt coupling incrementally. Higher levels include all requirements of lower levels.

### L1 — Listener

Minimum requirement to claim v0.1 compatibility. A Listener engine can *receive* coupling input for at least one of the three v0.1 types and apply audible modulation in response. "Audible" means a user can hear a difference when `amount` changes from `0.0` to `1.0` against a non-silent source.

**Required:**
- Accept `CouplingPacket` structs from a host matrix
- Implement `applyCouplingInput(type, amount, signal)` or equivalent
- Apply modulation to at least one audible parameter for each supported type
- Declare supported types in engine metadata

**Not required:** Emit coupling signal. Support all three types.

**Metadata declaration:**
```
COUPLING_COMPLIANCE: Listener
SUPPORTED_TYPES: 0x01, 0x03    // example — list what you actually implement
```

### L2 — Speaker

A Speaker engine can *emit* coupling signal for at least one v0.1 type. It provides a normalized output signal that the host matrix can forward to other engines.

**Required:**
- Expose at least one coupling output via `getSampleForCoupling()` or equivalent
- Provide RMS, envelope follower output, or pitch signal normalized to `[0.0, 1.0]` each block
- Declare emitted types in metadata

**Not required:** Receive coupling input.

**Metadata declaration:**
```
COUPLING_COMPLIANCE: Speaker
EMITTED_TYPES: 0x01, 0x03     // example
```

### L3 — Full

A Full engine supports both listening and speaking for all three v0.1 public types.

**Required:**
- All L1 requirements
- All L2 requirements
- All three types implemented for both input and output

**Metadata declaration:**
```
COUPLING_COMPLIANCE: Full-v0.1
```

---

## 6. Reference Implementation

**XOceanus** (https://github.com/BertCalm/XO_OX-XOceanus) is the reference implementation of this protocol.

XOceanus implements 92 engines across the original fleet, Kitchen Collection, and Singularity Collection. The three v0.1 public types are supported across the fleet; individual engine compliance declarations are available in engine metadata. The `SynthEngine` interface in `Source/Core/SynthEngine.h` defines the C++ contract:

```cpp
// Speaker interface — emit coupling signal
virtual float getSampleForCoupling (int channel, int sampleIndex) const = 0;

// Listener interface — receive coupling modulation
virtual void applyCouplingInput (CouplingType type,
                                 float amount,
                                 const float* sourceBuffer,
                                 int numSamples) = 0;
```

The `MegaCouplingMatrix` in `Source/Core/MegaCouplingMatrix.h` is the host matrix: it manages routes, performs envelope following on source audio, and delivers packets to target engines at block boundaries.

XOceanus implements 15 coupling types internally (3 public + 12 extended). Three are published in this v0.1 specification. The extended types within XOceanus are not part of this specification and are not documented here. Implementors may develop their own types in the `0x80`–`0xEF` private range using the extension mechanism defined in Section 7.

---

## 7. Extension Mechanism

### 7.1 Type ID Allocation

| Range | Designation |
|-------|-------------|
| `0x01` – `0x03` | Public v0.1 types (this specification) |
| `0x04` – `0x7F` | Reserved for future public spec versions |
| `0x80` – `0xEF` | Available for private and extended implementations |
| `0xF0` – `0xFF` | Experimental / vendor-specific (no compatibility guarantees) |

### 7.2 Adding a Future Public Type

Types assigned from `0x04`–`0x7F` require a new spec version and must document:

1. A stable semantic descriptor (what musical relationship does this type express?)
2. A defined signal flow (what does the source emit, what does the target receive?)
3. Parameter range specification
4. Compliance declaration for the new type
5. Two or more musical use cases demonstrating meaningful differentiation

Backward compatibility is maintained: a v0.2 engine implementing `0x04` can still declare `Full-v0.1` compliance for `0x01`–`0x03`.

### 7.3 Private Types

Private types in `0x80`–`0xEF` may be implemented without spec approval. Implementors are responsible for avoiding ID collisions at the host level. A host matrix receiving a packet with an unknown type should forward it unchanged to the declared target — the target may recognize it even if the host does not.

### 7.4 Extending the Packet

Future spec versions may extend `CouplingPacket` by appending fields after `reserved`. The 16-byte header layout of v0.1 must be preserved. Receivers that encounter a packet larger than expected should process the first 16 bytes and ignore the remainder.

---

## 8. Version History

| Version | Date | Changes |
|---------|------|---------|
| 0.1 | 2026-03-22 | Initial public specification. Defines types 0x01–0x03 (Sympathetic, Complementary, Responsive). 16-byte packet format. Three compliance levels (L1 Listener, L2 Speaker, L3 Full). Reference implementation: XOceanus. |

---

## Appendix A — Quick Reference

**Type IDs (v0.1 public)**

```
0x01  Sympathetic    — parallel resonance, harmonic reinforcement
0x02  Complementary  — inverse fill, dynamic breathing, call and response
0x03  Responsive     — source envelope shapes target timbre (filter + morph)
```

**Packet structure (16 bytes)**

```
[version: u8][type: u8][source_id: u16][target_id: u16]
[amount: f32][signal: f32][flags: u8][reserved: u8]
```

**Compliance declarations**

```
COUPLING_COMPLIANCE: Listener      // receives coupling input; L1
COUPLING_COMPLIANCE: Speaker       // emits coupling signal; L2
COUPLING_COMPLIANCE: Full-v0.1     // both, all 3 types; L3
```

---

*XO_OX Coupling Protocol v0.1 — Release Candidate 2026-03-22 by XO_OX Designs*
*Specification licensed under CC BY 4.0*
*Reference implementation: https://github.com/BertCalm/XO_OX-XOceanus*
