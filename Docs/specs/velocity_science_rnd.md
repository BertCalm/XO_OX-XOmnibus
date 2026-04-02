# Velocity Science R&D — MPC XPN Pack Design

**Status**: Research document | March 2026
**Scope**: Technical reference for XO_OX XPN pack velocity curve design

---

## 1. MPC Velocity Interpretation

The Akai MPC OS maps pad strike force to MIDI velocity (0–127) using a **piecewise nonlinear curve** that is hardware-model-dependent but generally follows a mild exponential shape — lighter taps produce values in the 10–40 range while moderate strikes land around 60–90. Hard strikes saturate near 120–127. The curve is not purely linear; the lower register (1–40) is compressed to prevent near-silent accidental triggers, while the upper register (90–127) is widened to give hard hits expressive headroom.

**User-adjustable sensitivity**: MPC OS exposes a per-pad Sensitivity parameter (typically 1–10 or 1–100 depending on firmware). Setting sensitivity higher shifts the curve left — moderate force reads as high velocity. Setting it lower gives more dynamic range for expressive performance but makes soft hits harder to trigger consistently.

**Implications for layer boundaries**: Because most real-world pad strikes cluster between velocity 50 and 110, placing layer boundaries at equal 32-unit intervals (32/64/96/127) wastes resolution in the tails. A boundary at 32 is rarely reached in performance; a boundary at 96 splits what feels like one expressive zone (the "normal hit" range).

---

## 2. Velocity Layer Boundary Math

Equal division for N layers across 0–127:

| Layers | Equal boundaries |
|--------|-----------------|
| 2 | 63 / 127 |
| 4 | 31 / 63 / 95 / 127 |
| 8 | 15 / 31 / 47 / 63 / 79 / 95 / 111 / 127 |
| 16 | every 8 units |

Equal division treats velocity space as uniform, which it is not perceptually or physically.

**The Vibe Musical Curve** (XO_OX tools standard) redistributes boundaries toward the high-density 50–110 performance zone, with wider low-velocity buckets for ghost notes and a compressed top bucket for peaks:

| Layers | Vibe boundaries (upper limit of each layer) |
|--------|---------------------------------------------|
| 2 | 80 / 127 |
| 4 | 40 / 72 / 100 / 127 |
| 8 | 20 / 40 / 58 / 72 / 84 / 96 / 108 / 127 |
| 16 | 10 / 20 / 30 / 42 / 54 / 64 / 72 / 80 / 87 / 93 / 98 / 103 / 108 / 113 / 119 / 127 |

The 8-layer Vibe curve is the standard for XO_OX drum packs. The 4-layer curve is standard for melodic/pad instruments. The 16-layer curve is reserved for hero instruments requiring maximum expression (e.g., a snare with rimshot crossfade).

---

## 3. Velocity to Timbre vs. Velocity to Amplitude

MPC keygroups support two distinct velocity response modes that should be applied intentionally:

**Velocity to amplitude only**: Appropriate for simple loops, one-shot stabs, electronic drums where the character should not change with force. Use for pad atmospheres where the goal is a consistent timbre at any dynamic.

**Velocity to timbre (layer switching)**: The primary mechanism for realism in acoustic drum packs, plucked strings, and piano. A harder snare hit is not simply louder — it is brighter, denser, and has more transient energy. Layers capture this discontinuity in a way that amplitude scaling cannot.

**Blend strategy by instrument type**:

- **Drum hits**: Timbre-first. 4–8 velocity layers. Amplitude difference between layers should be pre-baked into the recordings. Do not apply additional velocity-to-volume scaling in the keygroup — it double-attenuates soft hits into inaudibility.
- **Melodic instruments (keys, plucked strings)**: Amplitude + timbre blend. Use layers for timbral shift at major velocity thresholds (e.g., 40, 80), and let the keygroup's velocity-to-volume curve handle expression between layer boundaries. A gentle square-root volume curve (not linear) feels most natural.
- **Pads and atmospheres**: Subtle timbre shift only. A single layer with gentle velocity-to-volume and perhaps a velocity-to-filter-cutoff mod (+10–20% at max velocity) is sufficient and avoids jarring layer crossings in slow ambient contexts.

---

## 4. Round-Robin vs. Velocity Layers

Velocity layers address dynamic realism. Round-robin (RR) addresses machine-gun artifact — the unnatural repetition of an identical waveform on consecutive hits. They solve different problems and combine multiplicatively.

**When RR beats velocity layers**: For instruments where the primary variation between hits is not loudness but physical inconsistency — hi-hats, snare buzz variations, finger-plucked strings. A hi-hat with 4 RR samples at one velocity sounds more natural than 4 velocity layers with no RR.

**Combined strategy**: `V velocity layers × R round-robins = V×R samples per note`. Practical XO_OX targets:

| Instrument | Velocity layers | RR count | Total samples/note |
|------------|----------------|----------|--------------------|
| Kick | 4 | 2 | 8 |
| Snare | 4 | 3 | 12 |
| Hi-hat (closed) | 2 | 4 | 8 |
| Tom | 4 | 2 | 8 |
| Clap | 2 | 4 | 8 |

**XPN CycleType/CycleGroup implementation**: Set `CycleType="cycle"` for pure round-robin. Set `CycleType="random"` for variation without strict ordering. Group samples within the same velocity layer under a shared `CycleGroup` integer — the MPC advances the cycle index within each group independently. This means velocity layer selection and RR advancement are orthogonal axes, which is the correct behavior.

---

## 5. Ghost Notes and Microvelocity

Velocity 1–20 is the ghost note zone. Without deliberate treatment, these hits are either inaudible (sample too quiet) or sound like attenuated loud hits (no timbral character). Neither is musical.

**Techniques**:

- **Pre-gain on soft layers**: Normalize or boost the quiet sample so it sits at a listenable level acoustically, then rely on the keygroup volume to reduce loud layers. The soft layer should be recorded or processed to have its own character — more room, more noise floor, more rattle — not simply a quieter version of the main hit.
- **Noise floor preservation**: Acoustic drum recordings at soft velocity have audible stick-on-drum noise and room tone that disappears on loud hits. Retain this; it is what makes ghost notes feel physical. Do not gate it out.
- **Compression curve on soft layers**: Apply light upward compression (1.5:1 above threshold) during mastering of the ghost note sample. This gives the hit presence without making it loud. Target a ghost note peak at approximately -18 dBFS before the keygroup volume applies its scaling.
- **Microvelocity floor**: Set the keygroup's low velocity limit for the softest layer to 1, not 0. Velocity 0 is a note-off in MIDI. Setting the floor to 1 ensures ghost notes always trigger.

---

## 6. Velocity-Sensitive Filter Sweeps

Velocity-to-filter-cutoff is one of the most expressive tools in XPN keygroup design and one of the most commonly over-applied.

**Natural ranges**: For a drum hit, a velocity sweep of +20–40% cutoff from soft to hard feels physically correct — harder strikes produce more high-frequency content. For melodic instruments, +10–20% is sufficient; beyond that the timbre change overshadows the pitch identity of the note.

**Implementation in XPN**: Use the `KeygroupVelocityToFilter` attribute on the keygroup element. Value range is typically -100 to +100, representing percentage of filter range. Recommended values:

| Instrument | VelocityToFilter value |
|------------|----------------------|
| Kick | +25 |
| Snare | +30 |
| Hi-hat | +15 |
| Pad/atmosphere | +8 |
| Melodic synth | +12 |
| Plucked string | +20 |

Pair with a base cutoff that places the instrument's core tone in the right register at moderate velocity (around velocity 80). At velocity 127 the filter opens further; at velocity 20 it narrows, lending softness naturally.

**Pitfall**: Avoid velocity-to-filter on instruments with prominent high-frequency identity markers (e.g., a hi-hat at very low cutoff sounds like a thud, not a quiet hi-hat). In those cases, rely on layer switching for timbral change rather than filter modulation.

---

## 7. XO_OX feliX-Oscar Velocity Axis

The feliX-Oscar polarity — feliX as the bright, forward, assertive pole; Oscar as the warm, recessed, soulful pole — maps naturally and intuitively onto the velocity axis.

**Design rule**: Soft velocity is Oscar territory. Hard velocity is feliX territory.

- **At velocity 1–40 (Oscar zone)**: Warm, filtered, intimate. Emphasize room tone and low-frequency body. Filter cutoff reduced. Slight pitch softening if appropriate for the instrument character.
- **At velocity 80–127 (feliX zone)**: Bright, present, transient-forward. Filter opens. High-frequency content emphasized. Faster attack character.
- **Midrange velocity 41–79**: The coupling zone. Neither pole dominates. This is where most melodic phrases live and where the instrument's core identity should be cleanest and most balanced.

This rule should be applied as a quality check on every XPN pack: play the patch at velocity 20 and confirm it feels Oscar (warm, quiet, intimate); play at velocity 110 and confirm it feels feliX (bright, forward, present). If the polarity is reversed or flat, the velocity design is not yet complete.

For engines with explicit feliX-Oscar coupling parameters (e.g., OVERLAP, OUTWIT), the velocity axis should reinforce the same polarity that those parameters produce — soft hits should lean toward the same sonic character as Oscar-biased coupling settings, not contradict it.

---

*Reference: XPN Tools — `~/Documents/GitHub/XO_OX-XOceanus/Tools/` | Vibe musical curve implementation: `xpn_keygroup_export.py`*
