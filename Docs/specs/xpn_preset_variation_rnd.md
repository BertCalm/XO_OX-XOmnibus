# XPN Preset Variation R&D
**Generating systematic preset variations for XPN packs — expanding 1 preset into N with controlled parameter mutations**

---

## 1. Variation Axes for XPN

An XPM keygroup preset exposes the following axes for meaningful variation:

| Axis | XPM Parameter(s) | Variation Range |
|---|---|---|
| Attack | `AmpEnvAttack` | 0ms – 200ms |
| Decay/Release | `AmpEnvDecay`, `AmpEnvRelease` | 10ms – 2000ms |
| Filter cutoff | `FilterFrequency` | 80Hz – 18kHz |
| Filter resonance | `FilterResonance` | 0 – 100 |
| Pitch/tune | `Tune` (coarse), `FineTune` | ±12st coarse, ±50ct fine |
| Velocity sensitivity | `VelocitySensitivity` | 0 – 127 |
| Velocity-to-filter | `VelToFilterFreq` | -100 – +100 |
| Pan position | `Pan` | -100 (L) – +100 (R) |
| Sample layer selection | `SampleFile` across keygroup layers | swap between velocity layers or alt samples |
| LFO rate / depth | `LFORate`, `LFODepth`, `LFODest` | rate: 0.1–20Hz; depth: 0–100 |

Eight to ten axes are sufficient for pack-scale variation work. Avoid varying `BeatRoot`, `Interpolation`, or `LoopMode` — these change playback character too radically and break the "same preset family" identity.

---

## 2. Structured Variation vs Random Mutation

**Grid variations** systematically sweep two parameters across a matrix. A 4×4 grid of `FilterFrequency × AmpEnvAttack` produces 16 distinctly positioned presets with predictable tonal movement. This is ideal when a pack has a clear tonal concept — the grid guarantees coverage and eliminates duplicates by construction.

**Random mutation with constraints** randomizes all axes simultaneously within musical bounds, rejects any result that fails quality-control checks (see Section 6), and retries until N valid variants are produced. This produces more organic, surprising results but requires more rejection overhead and can cluster around the center of the parameter space.

**Recommendation for pack production:**
- Use grid mode for the "structural" presets in a pack — the variants that define the tonal range (bright ↔ dark, slow ↔ fast).
- Use random mode to fill remaining slots between grid anchors, seeded from the grid extremes rather than the source preset. This combines coverage guarantees with organic surprise.
- Grid is also more auditionable: a producer can walk the grid in sequence and hear the movement.

---

## 3. feliX-Oscar Variation Axis (Polarity Sweep)

The feliX-Oscar axis maps the XO_OX polarity doctrine onto a 5-step parameter sweep. feliX = bright, open, fast; Oscar = dark, compressed, slow.

| Step | Name suffix | FilterFreq | FilterRes | AmpEnvAttack | AmpEnvRelease | VelToFilterFreq | Sample Layer |
|---|---|---|---|---|---|---|---|
| 1 | `Oscar` | 400Hz | 60 | 80ms | 1800ms | -40 | darkest alt |
| 2 | `Deep` | 900Hz | 45 | 40ms | 1200ms | -20 | dark alt |
| 3 | `Core` | 2200Hz | 30 | 15ms | 600ms | 0 | source |
| 4 | `Bright` | 5500Hz | 20 | 6ms | 300ms | +20 | bright alt |
| 5 | `feliX` | 12000Hz | 10 | 1ms | 120ms | +40 | brightest alt |

Apply this sweep to any source preset by computing offsets relative to the source values rather than setting absolutes — this preserves the character of the original while moving it along the polarity axis.

---

## 4. Naming Conventions for Variations

MPC program names are capped at 32 characters. Given a base name like `Coral Bell` (9 chars), suffixes must stay under 22 chars including the separator space.

**Taxonomy:**

- Tonal suffix: `Bright`, `Dark`, `Deep`, `Thin`, `Warm`, `Cold`
- Envelope suffix: `Fast`, `Slow`, `Punch`, `Swell`, `Tail`
- Polarity suffix: `feliX`, `Oscar`, `Core`
- Intensity suffix: `Hot`, `Soft`, `Wide`, `Tight`

**Compound suffixes** (for grid intersections): `{Base} Fast Bright`, `{Base} Slow Dark` — use only when both axes are meaningfully distinct.

**Format rule:** `{BaseName} {TonalSuffix}` or `{BaseName} {EnvSuffix}` — single suffix preferred. Truncate BaseName to 24 chars before appending. Never append a number alone (`Coral Bell 3`) — it communicates nothing to the producer.

---

## 5. Proposed Tool: `xpn_variation_generator.py`

**Location:** `Tools/xpn_variation_generator.py`

**CLI:**
```
python xpn_variation_generator.py \
  --xpm base.xpm \
  --variations 20 \
  --mode grid|sweep|random \
  --axis felix-oscar|adsr|filter \
  --output-dir ./output/
```

**Parameter mutation ranges per axis:**

`adsr` axis: attack ±0–150ms, decay ±0–500ms, sustain ±0–30 (level units), release ±0–1500ms. Grid: 4×4 on attack × release.

`filter` axis: cutoff sweep 200Hz–16kHz (log scale), resonance 0–80. Grid: 4×4 on cutoff × resonance.

`felix-oscar` axis: 5-step sweep per Section 3 table. Mode is always `sweep`; `--variations` is ignored (always produces exactly 5).

`random` mode: randomize all 10 axes simultaneously within per-axis bounds. Each generated variant is validated against quality-control rules; invalid variants are discarded and regenerated (max 3× retries per slot before falling back to source ± small delta).

**Output:** One `.xpm` file per variant in `--output-dir`, named `{BaseName}_{Suffix}.xpm`. Also writes `manifest.json` listing all generated filenames and the axis/value that distinguishes each.

---

## 6. Quality Control Guardrails

Before writing any variant, validate:

- `FilterFrequency` floor: 80Hz minimum — below this, most samples become inaudible mud.
- `FilterResonance` ceiling: 85 maximum — above this, self-oscillation artifacts appear on most MPC filter models.
- `AmpEnvAttack` floor: 0.5ms minimum — 0ms attack on percussive samples causes inter-sample clicks at pack boundaries.
- `AmpEnvRelease` ceiling: 4000ms maximum — longer tails create mix collisions in kit contexts.
- `VelocitySensitivity` floor: 5 minimum — a value of 0 disables velocity entirely, collapsing dynamic range across all layers and defeating the Sonic DNA velocity mapping.
- `Tune` bounds: ±24 semitones maximum — beyond this, pitch-shifted samples sound transparently artificial on the MPC's interpolation engine.
- Reject any variant where `FilterFrequency` + `FilterResonance` combination falls in the self-oscillation danger zone (freq < 300Hz AND res > 70).

Any variant failing one or more checks is either clamped to the nearest valid value (for continuous parameters) or regenerated (for discrete choices like sample layer selection).
