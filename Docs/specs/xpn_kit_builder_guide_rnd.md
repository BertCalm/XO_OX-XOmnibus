# XPN Kit Builder Guide — R&D Reference
**Status**: Active Reference | **Updated**: 2026-03-16
**Scope**: ONSET drum kits (primary) + any drum-type XPN program

---

## Purpose

This document is a practitioner's reference for building drum kits within XPN packs. It covers the full chain from sample selection through XPM configuration. While written with ONSET as the primary target, the principles apply to any drum-type XPN program assembled from rendered or sampled sources.

The goal is not to produce technically correct kits — it is to produce kits that feel like a single instrument.

---

## 1. Kit Anatomy — 16-Pad Standard Layout

A complete XO_OX drum kit occupies 16 pads in a single XPM program. The layout below is the canonical assignment. Deviating without reason introduces confusion for performers and library users.

| Pad | Role | Voice (ONSET) |
|-----|------|---------------|
| 1 | Kick — main | Kick |
| 2 | Kick — sub | Kick (alt preset) |
| 3 | Kick — punchy | Kick (alt preset) |
| 4 | Kick — FX / texture | FX |
| 5 | Snare — main | Snare |
| 6 | Snare — rimshot | Snare (alt) |
| 7 | Snare — open / fat | Snare (alt) |
| 8 | Snare — ghost | Snare (low vel render) |
| 9 | Hi-hat — closed | CHat |
| 10 | Hi-hat — open | OHat |
| 11 | Hi-hat — pedal | CHat (alt) |
| 12 | Ride or crash | OHat (alt) / FX |
| 13 | Clap | Clap |
| 14 | Tom or Perc 1 | Tom |
| 15 | Perc 2 | Perc |
| 16 | FX / unique hit | FX |

**Design note**: Pads 4 and 16 are intentional wildcard slots. Use them for the sound that makes the kit distinctive — a tuned noise hit, a reverse tail, a pitched transient. If a kit has no obvious wildcard, default to a second clap variant (pad 16) and an FX tail (pad 4).

---

## 2. Choke Group Assignments

Choke groups enforce mutual exclusivity between pads — triggering one pad silences the other. Wrong choke assignments are the most common kit-building error and the hardest to catch on a static listen.

### Rules

**Open hat / closed hat — always choke together**
- Pads 9 and 10 share choke group 1 (by convention).
- Pedal hat (pad 11) joins the same group if it is a closed-hat variant.
- Ride (pad 12) does NOT join this group — ride and hat co-exist in real performance.

**Snare variants — judgment call**
- If pads 5–8 represent the same physical drum at different dynamics or articulations, assign them to choke group 2.
- If they represent genuinely different drums (e.g., snare + rimshot on a different drum), leave them unchoked.
- Ghost notes (pad 8) should almost never choke the main snare — ghosting and hitting simultaneously is a common technique.

**Kicks — no choke**
- Layering multiple kicks is a production technique. Choking defeats it.
- Exception: if pad 2 is a sub kick that would cause muddiness when layered with pad 1, a shared choke group is acceptable. Document this in the kit notes.

**Clap and snare — separate groups always**
- Clap (pad 13) and snare (pads 5–8) must never share a choke group. The snare+clap layer is one of the most common production moves in electronic music.

### XPM XML

```xml
<PadNote>
  <Number>9</Number>
  <!-- closed hat -->
  <Choke>1</Choke>
</PadNote>
<PadNote>
  <Number>10</Number>
  <!-- open hat — same choke group -->
  <Choke>1</Choke>
</PadNote>
```

Set `<Choke>0</Choke>` (or omit the element) for pads that should not choke.

---

## 3. Velocity Layer Design

Velocity layers are the primary expressive dimension of a drum kit. More layers are not always better. Choose the layer count based on the character of the kit, not on maximizing realism.

### Layer Count Guidelines

**1 layer — electronic / machine kits**
Consistency is the character. A TR-808 does not have velocity layers; pretending it does undermines the aesthetic. Use a single render at full velocity. Dynamic feel comes from programming variation in velocity values at the sequence level, not from the sample itself responding to velocity.

**2 layers — soft/hard**
Boundary at velocity 64. Soft layer: 1–64. Hard layer: 65–127. This is the minimum for acoustic-influenced or hybrid kits. The soft layer is typically 6–10 dB quieter and may have a noticeably different transient character.

**4 layers — Vibe Musical Curve (XO_OX standard)**
The preferred layer scheme for XO_OX production kits. Boundary points:
- Layer 1: velocity 1–40 (ghost / barely there)
- Layer 2: velocity 41–90 (working dynamic — widest range, most-played region)
- Layer 3: velocity 91–110 (accent hit)
- Layer 4: velocity 111–127 (peak / full force)

Layer 2 covers the widest velocity band because it is where most programmed and performed notes land. The curve is asymmetric by design — it is not four equal quarters.

**8 layers — cinematic / realistic**
Use only when acoustic realism is the explicit brief. Eight layers with round-robin at 4 RR samples each = 32 audio files per voice. A 16-pad kit at this density will consume substantial MPC RAM. Budget carefully. Typical MPC X peak RAM ceiling is ~2 GB for a project. At 48kHz/24-bit stereo, a 2-second sample is ~576 KB. Run the math before committing to 8-layer design.

### XPM Layer Declaration

```xml
<Layer>
  <Number>1</Number>
  <VelocityStart>1</VelocityStart>
  <VelocityEnd>40</VelocityEnd>
  <SampleFile>kick_ghost.wav</SampleFile>
  <Volume>-8.0</Volume>
</Layer>
<Layer>
  <Number>2</Number>
  <VelocityStart>41</VelocityStart>
  <VelocityEnd>90</VelocityEnd>
  <SampleFile>kick_mid.wav</SampleFile>
  <Volume>0.0</Volume>
</Layer>
```

---

## 4. Pan and Level Defaults

These are starting points, not rules. Trust ears over formula. The values below reflect what works acoustically for most XO_OX kit styles.

| Voice | Pan | Level |
|-------|-----|-------|
| Kick (main) | Center (0%) | 0 dB |
| Kick (sub) | Center (0%) | -1 dB |
| Kick (punchy) | Center (0%) | 0 dB |
| Kick (FX) | Slight right (5%) | -3 dB |
| Snare (main) | Slight right (2%) | 0 dB |
| Snare (rimshot) | Slight right (3%) | -1 dB |
| Snare (open) | Slight right (2%) | 0 dB |
| Snare (ghost) | Slight right (2%) | -6 dB |
| Hi-hat (closed) | Right (12%) | -1 dB |
| Hi-hat (open) | Right (15%) | -1 dB |
| Hi-hat (pedal) | Right (10%) | -2 dB |
| Ride / crash | Right (20–30%) | -2 dB |
| Clap | Center (0%) | -1 dB |
| Tom / Perc 1 | Varies | 0 dB |
| Perc 2 | Varies | -1 dB |
| FX hit | Varies | -3 dB |

**Pan conventions**: MPC pan is expressed as a percentage (0–100%) centered at 50%. "Slight right (2%)" means 52%. The table above uses offset notation for readability — apply accordingly in XPM XML.

**Hi-hat level**: Hi-hats always sit below kicks and snares in mix weight. The -1 dB default is conservative; many finished kits run hats at -3 dB or lower.

**Clap placement**: The snare and clap often land on the same beat. Keeping both centered prevents the combined hit from pulling left or right. If they are not intended to be layered, panning the clap slightly left (48%) creates separation without overcrowding the right side.

---

## 5. Round-Robin Configuration

Round-robin (RR) prevents the machine gun effect — the unnatural repetition of the exact same sample on consecutive hits. It is most audible on hi-hats and least important on kicks.

### Priority by Voice

| Voice | RR Priority | Notes |
|-------|-------------|-------|
| Closed hat | High | Machine gun effect is immediately obvious |
| Open hat | High | Especially on fast passages |
| Pedal hat | Medium | Less frequent hits; 2 RR samples sufficient |
| Clap | Medium | 2–4 RR samples; varies by aesthetic |
| Snare | Low–Medium | Realistic kits benefit; machine kits often don't |
| Kick | Low | Kick character benefits from consistency |
| Perc | Varies | Tuned percs: no RR. Noise percs: yes |

### Minimum and Optimal Sample Counts

- **Minimum**: 2 RR samples. Below this the alternation pattern becomes its own artifact.
- **Optimal**: 4 RR samples. Covers most playing speeds without obvious pattern repeat.
- **Diminishing returns**: Beyond 4 RR samples per voice, the improvement is rarely audible at production tempos. Reserve 6–8 RR for cinematic or acoustic-first kits.

### XPM CycleType / CycleGroup

```xml
<PadNote>
  <Number>9</Number>
  <!-- closed hat -->
  <CycleType>2</CycleType>
  <CycleGroup>1</CycleGroup>
</PadNote>
<PadNote>
  <Number>10</Number>
  <!-- open hat — separate cycle group -->
  <CycleType>2</CycleType>
  <CycleGroup>2</CycleGroup>
</PadNote>
```

`CycleType=2` enables sequential round-robin. Each distinct voice that uses RR needs a unique `CycleGroup` value — sharing a group between hat voices would cause them to share the RR counter, producing incorrect cycling.

**CycleType values for reference**:
- `0` = no cycling (single sample or random)
- `1` = random
- `2` = sequential round-robin (preferred)

---

## 6. ONSET-Specific Considerations

ONSET has 8 named synthesis voices: **Kick, Snare, CHat, OHat, Clap, Tom, Perc, FX**. Each voice has its own DSP chain within the engine. When exporting ONSET presets to XPN format, the voice-to-pad mapping must be preserved explicitly.

### Voice Export Mapping

| ONSET Voice | Default Pad | Export Note |
|-------------|-------------|-------------|
| Kick | 1 | Export at multiple preset states for pads 2–4 variants |
| Snare | 5 | Export at multiple preset states for pads 6–8 variants |
| CHat | 9 | RR variants exported as CycleGroup 1 |
| OHat | 10 | RR variants exported as CycleGroup 2 |
| Clap | 13 | RR variants exported as CycleGroup 3 |
| Tom | 14 | Single render unless kit brief specifies layering |
| Perc | 15 | Single render unless kit brief specifies layering |
| FX | 16 (and 4) | One render per FX pad |

### Variant Generation Strategy

ONSET's per-voice DSP means variant renders are produced by parameter changes within the same voice, not by switching to a different voice. Workflow:

1. Set the primary preset state for a voice (e.g., Kick).
2. Render at all required velocity layers and RR positions.
3. Adjust parameters for the variant (e.g., increase punch, remove sub).
4. Re-render for the variant pad.
5. Document the parameter delta in the kit's notes file.

This produces kit variants that are acoustically related — they share the same engine character — which is the goal.

### Preset State Preservation

The ONSET voice parameters that produced each sample render should be saved alongside the kit. The XPN Tools `render_spec.py` handles this by writing a `render_manifest.json` that logs parameter snapshots per voice per render. Do not skip this step. If a client or future engineer asks why pad 3 sounds the way it does, the manifest has the answer.

---

## 7. The Glue Test

A technically correct kit is not necessarily a good kit. The glue test is a subjective but reproducible check for acoustic coherence: **does the kit sound like it was recorded in the same room at the same time?**

### What breaks glue

- **Inconsistent reverb tail length**: Kick decays in 400 ms, snare has a 1.2-second room tail, hi-hats are dry. These sound like three different kits.
- **Mismatched low-end rolloff**: Sub kick has a 30 Hz rumble while snare has been high-passed at 200 Hz. The spectral floor is inconsistent.
- **Stereo width mismatch**: Kick is mono, hi-hats are hard-panned stereo, clap is mid/side processed. Width variation across voices creates a disjointed image.
- **Transient character mismatch**: Kick has a soft, rounded attack; all other voices have sharp digital transients. The kit does not feel like one machine or one room.

### Glue techniques

**Shared reverb character**: Route all kit voices through a shared reverb bus during render — even at low wet levels (5–10%). This adds the same acoustic fingerprint to every voice. For electronic kits, use a tight room reverb (pre-delay 5–10 ms, RT60 200–400 ms). For organic kits, use the room IR that matches the kit's aesthetic.

**Matched low-end rolloff**: Apply the same high-pass filter cutoff (or within 20 Hz) across all voices. If the kick is processed with a 40 Hz HPF, pass all other voices through the same filter. This creates a consistent spectral floor.

**Transient bus**: Run all hits through a shared transient shaper at unity gain before final render. This normalizes the attack character without changing level. Subtle but audible.

**Width budget**: Decide on a stereo width ceiling for the kit and enforce it. For most production kits: kick and bass voices are mono or near-mono; hats and ride can be up to 30% wide; percussion can go wider if the aesthetic supports it. Do not allow any voice to exceed the width budget without a documented reason.

### The practical test

After rendering all 16 pads, load them into a single MPC project and play a simple groove: four-on-the-floor kick, snare on 2 and 4, closed hat on every eighth note. If the groove does not feel like one instrument, find the voice that breaks the room — it is usually the snare or the clap — and reprocess it until it sits with the others.

---

## 8. File Naming and Delivery

### Naming Convention

```
[KitName]_[VoiceCode]_[Variant]_[Layer]_[RR].wav
```

Example:
```
BiteKit_CHat_Closed_L2_RR3.wav
```

Voice codes: `KK` (kick), `SN` (snare), `CH` (closed hat), `OH` (open hat), `CL` (clap), `TM` (tom), `PC` (perc), `FX` (fx)

### Directory Structure

```
KitName/
  Samples/
    KK/
    SN/
    CH/
    OH/
    CL/
    TM/
    PC/
    FX/
  KitName.xpm
  render_manifest.json
  kit_notes.txt
```

### Deliverable Checklist

- [ ] All 16 pads have at least one sample assigned
- [ ] Choke groups verified by live playback test (not just visual inspection)
- [ ] Velocity layers tested at velocities 10, 50, 80, 100, 120
- [ ] Round-robin verified by rapid repeated hits on hat pads
- [ ] Glue test passed (shared groove playback)
- [ ] render_manifest.json present with parameter snapshots
- [ ] All WAV files are 44.1 kHz or 48 kHz, 24-bit, normalized to -1 dBFS peak
- [ ] XPM XML validates against MPC schema (load in MPC software and confirm no errors)

---

## Reference Links

- [xpn-tools.md](../concepts/xpn-tools.md) — XPN Tool Suite documentation
- [onset-engine.md](../concepts/onset-engine.md) — ONSET voice architecture
- `Tools/` — XPN export scripts (`drum_export.py`, `render_spec.py`, `packager.py`)
