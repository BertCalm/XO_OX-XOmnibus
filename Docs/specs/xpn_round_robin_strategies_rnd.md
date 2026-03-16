# XPN Round-Robin Strategies — R&D

**Status**: Research / Draft
**Date**: 2026-03-16
**Scope**: MPC XPM round-robin implementation for ONSET drum packs

---

## 1. MPC CycleType Options

XPM XML exposes three `CycleType` values per keygroup layer set:

| CycleType | Behavior | MPC Internal Name |
|---|---|---|
| `Cycle` | Sequential: plays sample 1, 2, 3... then wraps back to 1 | Round Robin |
| `Random` | Picks any sample at random each hit — same sample can repeat consecutively | Random |
| `Random NoRepeat` | Random selection but never repeats the same sample back-to-back | Random No Repeat |

**When to use each:**

- **Cycle** — Best for acoustic simulation where slight tonal shift should feel deliberate and patterned. Also preferred when producers are programming on a grid and want predictable behavior during arrangement (they can mentally track which RR is playing). Hip-hop, boom-bap, lo-fi.
- **Random** — Appropriate for very large RR pools (6+) where repetition is statistically unlikely anyway. Avoids the "machine gun" effect without the overhead of tracking state. Use sparingly — consecutive repeats break the illusion.
- **Random NoRepeat** — Best default for most acoustic drum voices. Eliminates the machine-gun effect while keeping variation unpredictable. Recommended for snare, clap, and hi-hat.

---

## 2. CycleGroup Mechanics

`CycleGroup` is an integer assigned per layer within a keygroup. Samples sharing the same `CycleGroup` value are pooled together and cycle as a unit.

**Key behaviors:**
- Each unique `CycleGroup` integer maintains its own independent playback counter/position.
- Different velocity layers can have different `CycleGroup` values — meaning a soft hit pool and a hard hit pool can cycle independently. This is useful: a pp snare at position 2 does not force the ff snare to also be at position 2.
- Cross-instrument cycle groups (e.g., kick and snare sharing group 1) are technically definable by using the same integer across pads, but the MPC tracks state per-keygroup, not globally. Cross-pad group synchronization is not supported in XPM v2.0 — assign unique group integers per drum voice to avoid collisions.
- Recommended convention: assign `CycleGroup` values in bands — kick uses 10–13, snare uses 20–23, hi-hat uses 30–35, etc.

---

## 3. Optimal RR Counts by Drum Type

| Voice | Recommended RR | Rationale |
|---|---|---|
| Kick | 2–3 | Low-end consistency is critical; >3 RR introduces audible tonal variation that muddies the low end in a mix. 2 RR already breaks the machine-gun effect. |
| Snare | 3–4 | Most variation needed — snares are the most exposed voice. 3 RR is minimum for acoustic realism; 4 RR is sweet spot. |
| Closed Hi-Hat | 4–6 | Most mechanically repetitive without RR. Fast 16th-note hats are where the machine-gun effect is most obvious. 4 minimum, 6 for premium packs. |
| Open Hi-Hat | 2–3 | Less frequent hits; 3 RR is sufficient. |
| Cymbal (ride/crash) | 2–3 | Infrequent hits; 2 RR breaks repetition, 3 adds nuance. |
| Clap | 4–6 | As mechanically obvious as hi-hats when repeated. Electronic claps tolerate 4; acoustic layered claps need 6. |
| Toms | 2–3 | Rarely hit in rapid succession; 2 RR sufficient. 3 for fills-heavy packs. |
| Perc / FX | 2–3 | Context-dependent; 3 is safe default. |

**Diminishing returns threshold**: Beyond 4 RR on most voices, producers in blind listening tests cannot distinguish the variation in a full mix. Beyond 6 RR on any voice, the benefit is inaudible at mastered levels. Budget RR counts accordingly.

---

## 4. Velocity × RR Matrix

For ONSET drums with 4 velocity layers × N round-robins:

**Separate RR pools per velocity layer** is strongly preferred. Rationale: a drummer hitting softly uses different parts of the stick and a different stroke angle than a hard hit — those are physically independent events and should cycle independently.

Sharing RR across velocity layers creates a subtle but audible artifact: hitting ghost notes (pp) and accents (ff) in alternation steps through the RR in a way that makes neither pool feel natural.

**Recommended matrix for ONSET:**

| Tier | Vel Layers | RR per Layer | Samples per Voice | Notes |
|---|---|---|---|---|
| Essential | 2 | 2 | 4 | Minimum viable; good for MPC Live budget packs |
| Standard | 4 | 3 | 12 | Best balance of realism and file count |
| Premium | 4 | 4–6 | 16–24 | For hi-hats and claps only; kick/snare stay at 4×3 |

Each velocity layer gets its own `CycleGroup` integer. The MPC handles the per-layer cycling independently with no additional XML complexity.

---

## 5. Random vs Cycle by Genre

| Genre | Recommended CycleType | Reason |
|---|---|---|
| Hip-hop / Boom-bap | `Cycle` | Grid-locked programming; predictability helps producers track which layer is playing during arrangement |
| Lo-fi | `Cycle` | Subtle, consistent variation is the aesthetic — too much randomness feels wrong |
| Acoustic simulation | `Random NoRepeat` | Mimics real-world stochastic variance; consecutive repeats break the illusion |
| Electronic / Club | `Random NoRepeat` | High BPM patterns expose machine-gun effect fastest; NoRepeat is safest default |
| Jazz brushes | `Random` | Large pool (6 RR) makes true random statistically safe and feels most organic |
| Default / Unknown | `Random NoRepeat` | Safe for all contexts, no consecutive repeats, never feels mechanical |

---

## 6. XPN File Size Implications

**Sample count math:**

| Configuration | Samples per Voice | 16 Pads | Avg 1MB/sample | Total |
|---|---|---|---|---|
| 2vel × 2RR | 4 | 64 | — | ~64MB |
| 4vel × 3RR | 12 | 192 | — | ~192MB |
| 4vel × 4RR | 16 | 256 | — | ~256MB |
| 4vel × 6RR | 24 | 384 | — | ~384MB |

**MPC RAM limits by hardware:**

- MPC Live II / MPC X: 2GB RAM, ~1.5GB usable for samples
- MPC One+: 2GB RAM, ~1.2GB usable
- MPC Studio (software): host RAM dependent

**Recommendations by pack tier:**

- **Essential Pack** (broad compatibility): 4vel × 2RR = 8 samples/pad × 16 pads = 128 files (~128MB). Fits all MPC hardware comfortably.
- **Standard Pack** (ONSET default): 4vel × 3RR = 12 samples/pad × 16 pads = 192 files (~192MB). Well within limits on all current MPC hardware.
- **Premium Pack** (hi-hats and claps at 4vel × 6RR, kick/snare/toms at 4vel × 3RR): ~240–280 files (~250MB). Still safe; leaves >900MB headroom on MPC Live II for additional kits loaded simultaneously.
- **Hard ceiling**: Keep total pack under 500MB to allow producers to load 2–3 kits in RAM simultaneously for live performance.

**Practical note**: At 44.1kHz/24-bit stereo, a 1-second drum hit WAV is ~264KB. Most drum samples are 0.5–1.5 seconds. The 1MB/sample estimate is conservative — real-world 192-file Standard packs typically land at 80–120MB, leaving generous headroom.

---

## Summary Recommendations for ONSET Packs

1. Use `Random NoRepeat` as the default `CycleType` for all voices.
2. Assign separate `CycleGroup` integers per velocity layer per drum voice.
3. Target **Standard tier** as the shipping default: 4vel × 3RR = 12 samples per voice.
4. Apply **Premium RR** (4–6) selectively to hi-hats and claps only — the voices where machine-gun effect is most audible.
5. Keep kick and toms at 2–3 RR — consistency matters more than variation in the low end.
6. Total pack budget: stay under 300 files / 250MB for universal MPC compatibility.
