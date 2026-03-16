# ONSET Drum Essentials Pack — Design Spec

*XO_OX Designs | April 2026 Launch | Pack R&D Document*
*Date: 2026-03-16*

---

## Overview

**Pack Name:** ONSET Drum Essentials
**Engine:** XOnset (ONSET) — Electric Blue `#0066FF`
**Format:** XPN drum kit program files (.xpn) for MPC standalone
**Launch target:** April 2026 — the first commercial XO_OX expansion pack
**Kit count:** 20 kits (16 genre/application kits + 4 MPCe-native kits)

This is not a genre pack. It is not a vibe pack. It is the canonical demonstration of what XO_OX drum synthesis can do across every context a working producer encounters — from the studio to the club to the film score.

---

## Marketing Hook

Three sentences that explain why this pack is different from every other drum pack on the market:

> Every other drum pack sells you recordings of someone else's drum machine. ONSET Drum Essentials synthesizes every hit from scratch — no samples, two paradigms (circuit analog and digital algorithm) blended per voice, 111 parameters shaped into 20 kits covering every genre and tempo a working producer needs. This is the sound of XO_OX hitting a surface.

---

## Pack Identity

### What ONSET Is

XOnset holds two synthesis paradigms simultaneously per voice: a **circuit layer (X)** modeled on analog drum machines (TR-808 bridged-T oscillator, TR-909 pitch-swept saw, self-oscillating filters, noise bursts), and an **algorithm layer (O)** built from FM synthesis, Karplus-Strong, modal resonators, and phase distortion. A single **Blend** axis morphs continuously between them.

The result is percussion that does not exist in any other drum machine: an 808-weight sub kick that can dissolve into a modal metallic ring, a noise-burst snare that crosses into Karplus-Strong shimmer, a hi-hat that moves between metallic hiss and crystalline tone. Neither layer. Both layers. The spectrum between.

### What This Pack Proves

1. ONSET can anchor any genre — boom bap, Afrobeats, techno, jazz, bossa nova — with genre-authentic character
2. ONSET's Blend axis is a live performance tool, not just a preset parameter
3. ONSET's 8-voice architecture (Kick, Snare, CHat, OHat, Clap, Tom, Perc, FX) covers a complete kit with no gaps
4. ONSET's XVC cross-voice coupling (B002/B006 Seance Blessings) produces sounds impossible with any standard drum machine

---

## Kit Architecture

### 20 Kits — Complete List

| # | Kit Name | Character | Primary Genre | Loudness Target |
|---|----------|-----------|---------------|-----------------|
| 01 | MPCe Native | Quad-corner optimized, feliX-Oscar per pad | Format showcase | -12 LUFS integrated |
| 02 | Foundation | Balanced, works on everything | All-purpose house kit | -14 LUFS |
| 03 | Boom Bap | Punchy kicks, snappy snares, hip-hop swing | Golden era hip-hop | -10 LUFS |
| 04 | UK Garage | Off-beat rhythmic character, shuffled hats | UK Garage / UKG | -11 LUFS |
| 05 | Afrobeats | Polyrhythmic, layered percussion, melodic bass drums | Afrobeats / Amapiano | -12 LUFS |
| 06 | Electronic / Techno | Industrial, distorted, machine-like | Techno / Industrial | -10 LUFS |
| 07 | Jazz Brushes | Soft, dynamic, velocity-sensitive | Jazz / Acoustic | -18 LUFS (dynamic) |
| 08 | Bossa Nova | Gentle, complex, syncopated | Bossa / Brazilian | -16 LUFS |
| 09 | 808 Kit | Deep sub kicks, trap-influenced | Trap / Hip-hop | -9 LUFS |
| 10 | Acoustic Room | Natural, roomy, recorded feel | Session / Singer-songwriter | -14 LUFS |
| 11 | XO_OX Signature | feliX-Oscar polarity fully showcased | XO_OX brand statement | -13 LUFS |
| 12 | Drill | Dark, compressed, sliding 808s | UK Drill / Chicago Drill | -10 LUFS |
| 13 | Dancehall | Bright, sharp, riddim-ready | Reggaeton / Dancehall | -11 LUFS |
| 14 | Broken Beat | Complex, syncopated, genre-fluid | Broken Beat / Neo-soul | -14 LUFS |
| 15 | Lo-Fi | Vintage warmth, cassette texture, slow swing | Lo-fi hip-hop | -16 LUFS |
| 16 | Ambient Perc | Textural, spatial, tonal hits | Ambient / Experimental | -20 LUFS (dynamic) |
| 17 | New Wave | Synthetic, rigid, angular | Post-punk / New Wave | -11 LUFS |
| 18 | Club Fusion | Wide, modern, club-ready | House / Future Bass | -10 LUFS |
| 19 | Cinematic | Epic, large-space, designed | Film score / Trailer | -16 LUFS |
| 20 | XVC Live | Cross-voice coupling showcased in kit form | Live performance | -12 LUFS |

---

## Per-Kit XPN Structure

### Standard Kit Structure (Kits 02–20 except MPCe Native)

```
Per Kit:
  16 pads total
  Pads 01–08:  One pad per voice (Kick, Snare, CHat, OHat, Clap, Tom, Perc, FX)
  Pads 09–12:  Velocity-layered variations (soft/medium/hard for Kick, Snare, Tom, Clap)
  Pads 13–16:  Character variants (Blend-shifted: X-dominant, center, O-dominant, extreme O)

  Per pad:
    Velocity layers: 3 (D001 compliance — velocity shapes filter brightness, not just amplitude)
      Layer 1: velo 0–59    (soft — Layer X dominant, low-frequency weight)
      Layer 2: velo 60–99   (medium — centered Blend)
      Layer 3: velo 100–127 (hard — Layer O edge, transient brightness, harmonic complexity)
    Round-robin cycles: 3 per velocity layer (prevents machine-gun effect)
    Total samples per standard voice: 9 (3 velocity × 3 RR)

  Q-Link Assignments (per kit):
    Q1: Kick pitch / sub frequency
    Q2: Snare snap / transient sharpness
    Q3: Overall Blend (Layer X ↔ Layer O)
    Q4: FX voice send level / tail length
```

### Loudness-Sensitive Kits — Modified Structure

**Boom Bap / 808 / Techno / Drill (loud/compressed kits):**
- Velocity layer thresholds compressed: 0–49 / 50–89 / 90–127
- All three layers land within 3 dB of each other (brick-wall feel)
- Kick pad: 2 additional pads (sub-only variant + click-only variant) for layering

**Jazz Brushes / Bossa Nova / Ambient Perc / Cinematic (dynamic kits):**
- Velocity layer thresholds expanded: 0–42 / 43–84 / 85–127
- Dynamic range 12–18 dB across velocity range (hard hits are genuinely louder, not just filtered)
- Brush rolls: CHat and Snare pads include 5-velocity layers (adds pianissimo and fortissimo edge)

---

## Kit-by-Kit Design Notes

### Kit 01 — MPCe Native (detailed spec in Section below)

Quad-corner design. See dedicated section.

---

### Kit 02 — Foundation

The "house kit." Every voice balanced, no extreme Blend positions, works at any tempo, fits any genre without needing EQ.

- **Kick:** Blend 0.35 (mostly X, clean sub + controlled attack click)
- **Snare:** Blend 0.5 (exact center — X body + O shimmer in equal measure)
- **CHat:** Blend 0.6 (O-leaning — metallic precision, clean decay)
- **OHat:** Blend 0.55 (slight O — bell-top shimmer, controlled wash)
- **Clap:** Blend 0.45 (X-leaning — noise burst body with slight modal definition)
- **Tom:** Blend 0.3 (X dominant — warm pitched membrane)
- **Perc:** Blend 0.65 (O-leaning — tonal, works as shaker or rim variant)
- **FX:** Blend 0.8 (full O — designed sound, textural hit)

DNA target: brightness 5, warmth 6, movement 4, density 5, space 5, aggression 4

---

### Kit 03 — Boom Bap

Punchy, compressed, swings at 85–92 BPM. The vinyl-era sound. Kicks with weight, snares with snap.

- **Kick:** Blend 0.15 — deep X circuit, heavy sub, 808-adjacent pitch envelope but not excessive. Pitch sweeps from 180Hz down to 45Hz in 60ms.
- **Snare:** Blend 0.25 — X body dominant. Noise burst + ring, tight decay. Filtered above 8kHz.
- **CHat:** Blend 0.4 — metallic but short. Classic closed hat tick. Very low O presence.
- **OHat:** Blend 0.3 — warm wash, not bright. Rolls naturally under the hat pad.
- **Clap:** Blend 0.2 — tight noise burst, multi-burst envelope (TR-808 clap architecture). Attack snap, fast tail.
- **Tom:** Blend 0.1 — maximum X, deep membrane thwack.
- **Perc:** Blend 0.45 — conga-adjacent, warm Attack, moderate decay.
- **FX:** Blend 0.7 — Vinyl crackle character via phase distortion algorithm.

Q-Link assignments: Kick tune / Snare tension / Hat crispness / Overall swing depth

---

### Kit 04 — UK Garage

Shuffled hats, off-beat character, slightly forward midrange.

- CHat: 3 variations tuned to the UKG shuffle — 2/3-step, reverse-swing hi-hat pattern implied by timing
- OHat: open position used on the "and" — designed to decay in exactly 1/8 note at 130 BPM
- Clap: slightly pitch-shifted (+2 semitones from foundation) — the UKG upward snap
- Kick: tight sub, minimal body — the garage kick punches up, not out

---

### Kit 05 — Afrobeats

Layered percussion is the kit's center of gravity, not the kick/snare axis.

- **Perc 1 (pad 07):** Djembe-adjacent. Blend 0.2. Warm attack, tuned to A2.
- **Perc 2 (pad 13):** Shaker/caxixi character. Blend 0.7. O-dominant noise algorithm, bright and short.
- **Perc 3 (pad 14):** Talking drum inflection. Blend 0.35. Pitch modulated by velocity — soft hit = lower pitch, hard hit = 4-semitone rise.
- **Kick:** Melodic bass drum — pitched lower than standard (B1), sub-weight. The Amapiano "log drum" character.
- **Tom:** Second melodic layer — mid-range, tuned to E2 for harmonic pairing with kick.
- **FX:** Shekere/agogo hybrid. Full O-layer. Metallic tonal hit.

---

### Kit 06 — Electronic / Techno

Industrial precision. Every voice is harsh until it isn't.

- **Kick:** Blend 0.1. Maximum X circuit weight. Pitch descends from 200Hz to 50Hz in 40ms. Hard clip on output at -0.5 dB.
- **Snare:** Blend 0.6. FM-algorithm layer dominant in algorithm section. Metallic body, noise transient, sharp.
- **CHat:** Blend 0.85. Full O. Phase distortion generates icy metallic texture — not a hat, a frequency burst.
- **Clap:** Blend 0.7. O-modal resonator driven hard. Sounds like metal hitting concrete.
- **FX:** Blend 1.0. Pure algorithm. Karplus-Strong 4-note stab, 80ms decay.
- Target loudness: -10 LUFS. No headroom. Limit hard.

---

### Kit 07 — Jazz Brushes

The dynamic range kit. Velocity is the performance tool.

- All 8 voices: soft-velocity layers are genuinely quiet (-18 LUFS at velo 40). Hard-velocity layers reach only -14 LUFS.
- **CHat:** Blend 0.25. Brush drag emulation — slow attack (20ms), textured decay via noise modulation in X layer.
- **Snare:** Blend 0.2. Wire brush snare — not a snap, a wash. Long tail. X circuit noise dominant.
- **Ride (OHat pad):** Bell hit (hard velocity) vs. bow wash (soft velocity). Blend transitions 0.55→0.3 across velocity layers.
- **Kick:** Soft felt beater character. Blend 0.1. No sub below 80Hz at low velocity. Sub emerges only at velocity 100+.
- D001 compliance: velocity here shapes transient character fundamentally — at velocity 40, kick has no attack click.

---

### Kit 08 — Bossa Nova

Gentle, syntactically complex, suitable for slow-burn grooves at 120–140 BPM.

- **Surdo (Kick pad):** Deep but soft. Blend 0.15. Slow attack (8ms). Pitch sits at D2.
- **Clave (Clap pad):** Pure X wood block model. Blend 0.05. Short, dry, hard.
- **Shaker (CHat pad):** 2-step shaker pattern implied by round-robin pairs (forward shake, back shake).
- **Tom:** Conga low + conga high split across Tom (pad 06) and Perc (pad 07).
- **FX:** Pandeiro — Blend 0.55. Percussive tonal hit + metallic jingle via O-layer resonators.

---

### Kit 09 — 808

Sub-dominant. Trap-adjacent. Slides and sustains.

- **Kick:** Blend 0.05. Maximum X sub architecture. Pitch envelope: starts at 240Hz, slides to 40Hz over 800ms.
  - 4 pad variants: standard, short (200ms), long (1200ms), slide (pitch envelope reversed — rises)
- **Snare:** Blend 0.3. Wide, compressed, slightly noisy. Less snap than Boom Bap. More of a smear.
- **CHat:** Blend 0.55. Roland-808 metallic model — 6-oscillator metallic hat character via O-layer.
- Q-Link: Q1 = Kick decay time, Q2 = Kick pitch center, Q3 = Hat decay, Q4 = Snare reverb tail

---

### Kit 10 — Acoustic Room

Room character without samples. Pure synthesis.

- **Kick:** Blend 0.2. X-dominant with a room reflection impulse synthesized in the O-layer — modal resonators set to room modes (axial frequency based on 3m × 4m room geometry at ~430Hz, ~860Hz).
- **Snare:** Blend 0.4. Wire coil resonance via Karplus-Strong in O-layer. X layer provides the membrane thud.
- **Overhead (OHat pad):** Blend 0.6. O-layer modal resonator emulates cymbal + room splash.
- Velocity response: attack transient always has pre-delay of 2–4ms (simulates room distance from kit).

---

### Kit 11 — XO_OX Signature

Every pad showcases feliX-Oscar polarity. This is the brand statement kit.

- **Pad 01 (Kick):** Oscar-weighted. Sub mass, slow, inevitable.
- **Pad 02 (Snare):** feliX-weighted. Bright snap, high harmonic content, Blend 0.75.
- **Pad 03 (CHat):** feliX extreme. Blend 0.9. Crystalline, fast, precise.
- **Pad 04 (OHat):** Oscar-feliX center. Blend 0.5 exact. The balance point.
- **Pad 05 (Clap):** feliX-leaning. Bright transient, modal tail.
- **Pad 06 (Tom):** Oscar-weighted. Warm, pitched membrane, minimal high-frequency.
- **Pad 07 (Perc):** feliX-extreme O-algorithm. Karplus-Strong metallic ring.
- **Pad 08 (FX):** Oscar-extreme X-circuit. Pure analog impulse — the rawest X output in the pack.
- Character variants (pads 09–16): Blend positions shifted to opposite polarity of primary pad.
- This kit is designed to be demoed live — alternate between pads 01 and 02 to hear the full feliX-Oscar spectrum.

---

### Kits 12–15 (Drill / Dancehall / Broken Beat / Lo-Fi)

**Drill:** Kick modeled on Chicago/UK drill conventions. Long sub decay (600–900ms), minimal attack click. Snare: high-tension metallic snap. Hats: rolled, semi-open, varied velocity.

**Dancehall:** Kick sits forward in the mix — more 200Hz body than sub. Rimshot-dominant snare. Bright hi-hat variants. Perc pad includes woodblock for riddim pattern.

**Broken Beat:** Unquantized velocity layers produce natural swing without time-shifting. Snare includes ghost note variant (separate pad, -18 dB, very short). CHat round-robin includes accent, ghost, and dead variations.

**Lo-Fi:** X-layer dominant throughout. Bitcrusher in the FX chain (not as synthesis — as character). All hits have slight pitch instability modeled via sub-audio LFO in the X circuit layer. CHat has cassette hiss character via filtered noise tail.

---

### Kits 16–19 (Ambient / New Wave / Club Fusion / Cinematic)

**Ambient Perc:** Every voice is tonal — Blend 0.6–0.9, O-layer dominant. Kicks are low-frequency tones at B1 or D2, 2-second decay. Hats are bell-resonators. FX is a 4-second metallic sustain.

**New Wave:** Rigid, synthetic, unhuman precision. TR-808 model through X-layer, then pushed into O-layer for digital hardness. No round-robin variation — intentionally mechanical.

**Club Fusion:** Four-on-the-floor kick with sub extension. Clap on 2/4 with wide stereo spread. Hi-hats at 16th-note density. FX: risers and downlifters on pads 13–16.

**Cinematic:** Large-room emulation via modal resonators. Kick: 50ms attack (slow-roll) — more of a cannon than a drum. Snare: 3-second reverb tail synthesized in the O-layer FDN. Tom: orchestral concert tom character, tuned to a minor pentatonic stack.

---

### Kit 20 — XVC Live

Showcases ONSET's XVC cross-voice coupling (B002/B006 Seance Blessings). Voices are coupled — the Kick's amplitude envelope modulates the Snare's filter cutoff; the FX voice gates the OHat.

- **Pad 01 (Kick):** On hard hit, sends amplitude spike to Snare filter — the snare brightens on the "1" of every bar
- **Pad 08 (FX):** Sends gate signal to OHat — open hat only sounds while FX voice is decaying
- **Pad 16 (XVC master):** Triggers the entire kit's XVC network simultaneously
- Performance notes: Designed to be played live with Q-Links controlling coupling depth

---

## MPCe Native Kit — Full Specification (Kit 01)

### Philosophy

Each of the 8 ONSET voices gets one MPC pad. Each pad surface is divided into four corners using Architecture 1 (feliX-Oscar Polarity). The player navigates the pad surface to access the full feliX-Oscar synthesis spectrum in real time.

### Quad-Corner Map (Applies to All 8 Pads)

```
NW: feliX Dry              │ NE: feliX Processed
  (Layer O dominant,        │   (Layer O + FX send:
   Blend 0.75, no reverb,   │    Blend 0.8, reverb on,
   bright, precise)         │    spatial width at 100%)
──────────────────────────────────────────────────────
SW: Oscar Dry              │ SE: Oscar Processed
  (Layer X dominant,        │   (Layer X + FX send:
   Blend 0.2, no reverb,    │    Blend 0.15, tape sat on,
   warm, physical)          │    space tail at 75%)
```

### Per-Voice Corner Assignments

#### Pad 01 — Kick

| Corner | Blend | Character | Use Case |
|--------|-------|-----------|----------|
| NW (feliX Dry) | 0.75 | Punchy digital click, low sub, clean | EDM main kick |
| NE (feliX Processed) | 0.80 | Reverb tail, spatial, sidechain-ready | Room kick, festival |
| SW (Oscar Dry) | 0.20 | Deep analog sub, slow pitch envelope | 808-weight, trap |
| SE (Oscar Processed) | 0.15 | Sub + tape saturation, warm decay | Lo-fi, boom bap |

Velocity layers: 3 across all corners. Pitch envelope depth varies by velocity — hard hits sweep wider.

#### Pad 02 — Snare

| Corner | Blend | Character | Use Case |
|--------|-------|-----------|----------|
| NW | 0.80 | Modal metallic snap, very bright | Karplus snare, tech |
| NE | 0.75 | Metallic + reverb haze | Rock snare, open |
| SW | 0.25 | Classic noise burst, fat body | Hip-hop snare |
| SE | 0.20 | Fat + tape, slightly smeared | Boom bap, dirty |

#### Pad 03 — CHat (Closed Hi-Hat)

| Corner | Blend | Character | Use Case |
|--------|-------|-----------|----------|
| NW | 0.90 | Crystalline tick, almost tonal | Electronic, minimal |
| NE | 0.85 | Crystal + stereo spread | House, wide |
| SW | 0.30 | Metallic hiss, short decay | Classic hat |
| SE | 0.25 | Hiss + warm drive | Analog machine |

#### Pad 04 — OHat (Open Hi-Hat)

| Corner | Blend | Character | Use Case |
|--------|-------|-----------|----------|
| NW | 0.85 | Bell-resonator wash, long tonal decay | Ambient, jazz-adjacent |
| NE | 0.80 | Bell + reverb, huge space | Room, cinematic |
| SW | 0.35 | Metallic wash, analog character | Standard open hat |
| SE | 0.30 | Warm wash + slight drive | Soul, gospel |

#### Pad 05 — Clap

| Corner | Blend | Character | Use Case |
|--------|-------|-----------|----------|
| NW | 0.70 | Crisp multi-burst, modal tail | Electronic, techno |
| NE | 0.65 | Modal + reverb hall | Arena, big |
| SW | 0.20 | Noise-burst body, classic snap | 808 clap, hip-hop |
| SE | 0.18 | Wide noise + tape warmth | Vintage, lo-fi |

#### Pad 06 — Tom

| Corner | Blend | Character | Use Case |
|--------|-------|-----------|----------|
| NW | 0.70 | Tonal membrane ring, FM shimmer | Electronic tom |
| NE | 0.65 | Ring + reverb + spatial | Cinematic tom |
| SW | 0.15 | Deep membrane thud | Acoustic, live |
| SE | 0.12 | Thud + room wash | Natural room kit |

Tom is pitch-mapped to C3 center, ±7 semitones via velocity curve.

#### Pad 07 — Perc

| Corner | Blend | Character | Use Case |
|--------|-------|-----------|----------|
| NW | 0.90 | Pure algorithm — metallic tonal hit | Clave, rim, bell |
| NE | 0.88 | Tonal + long reverb | Orchestral perc |
| SW | 0.25 | Warm thud, wood/skin | Conga, djembe-adj. |
| SE | 0.22 | Warm + subtle drive | Afrobeats, reggae |

#### Pad 08 — FX

| Corner | Blend | Character | Use Case |
|--------|-------|-----------|----------|
| NW | 1.00 | Pure O-algorithm — designed sound | Riser, texture |
| NE | 1.00 | Designed + full wet FX chain | Drop, transition |
| SW | 0.05 | Raw X-circuit impulse — purest analog | Rave stab, noise hit |
| SE | 0.08 | X impulse + tape color | Vintage FX, lo-fi |

---

## Quality Bar — DNA Diversity Requirements

### Fleet-Wide DNA Requirements (All 20 Kits)

No two kits may have identical DNA vectors. Minimum Euclidean distance between any two kit DNA signatures: 1.5 units across 6-dimensional space.

| DNA Dimension | Kit Range Allowed | Forbidden Cluster |
|---------------|-------------------|-------------------|
| Brightness | 2.0–9.5 | No more than 4 kits between 4.5–5.5 |
| Warmth | 1.5–9.0 | No more than 4 kits between 4.5–5.5 |
| Movement | 1.0–8.0 | — |
| Density | 3.0–9.5 | — |
| Space | 1.0–9.0 | — |
| Aggression | 1.0–9.5 | No more than 3 kits above 8.5 |

### Kit Category DNA Targets

| Category | Brightness | Warmth | Movement | Density | Space | Aggression |
|----------|-----------|--------|----------|---------|-------|------------|
| Boom Bap | 5.5 | 7.0 | 4.0 | 8.0 | 4.0 | 6.5 |
| 808 | 4.0 | 8.5 | 2.0 | 9.0 | 3.0 | 6.0 |
| Techno | 7.5 | 3.0 | 3.0 | 9.5 | 2.5 | 9.0 |
| Jazz Brushes | 4.5 | 7.5 | 6.0 | 3.0 | 7.0 | 1.5 |
| Bossa Nova | 5.0 | 7.0 | 5.5 | 3.5 | 6.5 | 1.0 |
| Afrobeats | 6.5 | 7.5 | 8.0 | 7.0 | 5.5 | 5.0 |
| XO_OX Signature | 7.0 | 5.0 | 5.0 | 6.0 | 6.0 | 6.0 |
| Acoustic Room | 5.5 | 7.5 | 4.0 | 5.5 | 7.5 | 3.0 |
| Cinematic | 5.0 | 6.5 | 3.0 | 6.5 | 9.0 | 5.5 |
| Ambient Perc | 6.0 | 5.5 | 7.5 | 2.0 | 9.5 | 1.5 |

### Velocity Sensitivity Requirements (D001 Compliance)

All 20 kits must pass the following D001 test per kit:

1. **Filter brightness test:** At velocity 40, all pitched voices must have measurably lower high-frequency content than at velocity 100. Minimum difference: 4 dB at 8 kHz. Automated test via `perc_velFilterSensitivity` parameter — must be ≥ 0.4 on all voices.

2. **Amplitude range test:** Dynamic kits (Jazz, Bossa, Acoustic, Ambient, Cinematic) must have velocity amplitude range ≥ 18 dB between velocity 10 and velocity 127. Loud kits (Boom Bap, 808, Techno, Drill) may compress to 8 dB range, but must not drop below 6 dB.

3. **Transient character test:** At least 4 voices per kit must have velocity-sensitive transient character (not just amplitude) — meaning the attack shape changes, not just the level. Validated via `perc_velBlendSensitivity` parameter — must be ≥ 0.2 for transient-critical voices.

### LUFS Targets Per Kit Category

| Category | Integrated LUFS | Peak dBFS | Dynamic Range (LRA) |
|----------|----------------|-----------|---------------------|
| Boom Bap | -10 ± 1.5 | -1.0 | 6–9 LU |
| 808 / Drill | -9 ± 1.0 | -0.5 | 4–7 LU |
| Techno / Club | -10 ± 1.0 | -0.5 | 4–6 LU |
| House / Dancehall | -11 ± 1.5 | -1.0 | 5–8 LU |
| Hip-hop (general) | -12 ± 2.0 | -1.5 | 6–10 LU |
| All-purpose (Foundation, MPCe, Broken Beat) | -12 to -14 | -1.5 | 7–11 LU |
| Lo-Fi / Acoustic | -14 to -16 | -2.0 | 9–14 LU |
| Jazz / Bossa | -16 to -18 | -2.0 | 12–18 LU |
| Ambient / Cinematic | -18 to -22 | -3.0 | 15–24 LU |

All measurements taken on full-kit loops at appropriate tempo (100 BPM for general; 90 BPM for boom bap; 80 BPM for bossa nova; 130 BPM for garage/techno).

---

## Blend Distribution Requirements — Anti-Redundancy

To prevent the pack from being 20 variations of the same sound, the following Blend coverage requirements apply:

| Blend Zone | Label | Kits That Must Include ≥ 2 voices here |
|-----------|-------|---------------------------------------|
| 0.00–0.20 | Deep X (pure analog circuit) | 808, Boom Bap, Bossa Nova, Lo-Fi, XVC Live |
| 0.21–0.40 | X-dominant | Foundation, Acoustic Room, Jazz Brushes, Afrobeats |
| 0.41–0.60 | Center blend | Foundation, XO_OX Signature, Broken Beat, UK Garage |
| 0.61–0.80 | O-dominant | Techno, Cinematic, Ambient Perc, Dancehall |
| 0.81–1.00 | Deep O (pure algorithm) | Electronic Techno, New Wave, Ambient Perc, MPCe NW/NE |

Kick voice may not appear in the 0.81–1.00 zone for more than 3 kits (a kick at full O-algorithm stops being a kick and becomes a tonal hit — this is a design boundary, not a prohibition).

---

## Production Notes

### Render Pipeline

All kits rendered via `XPN Export Tool` (Tools/ directory). Render spec:

- Sample rate: 44.1 kHz (MPC standard)
- Bit depth: 24-bit WAV
- Normalize: NO — preserve inter-kit level relationships
- Filename convention: `{KitName}_{Voice}_{VeloLayer}_{RRcycle}.wav`
- Example: `BoomBap_Kick_V2_RR1.wav`

### Q-Link Assignment Standard

Q-Links follow this convention across the full pack:

| Q | Default Function | Override Allowed |
|---|-----------------|-----------------|
| Q1 | Kick fundamental pitch | Yes, per kit |
| Q2 | Snare/clap tonal character (Blend on snare) | Yes, per kit |
| Q3 | Hat decay time | Yes, per kit |
| Q4 | FX tail / reverb depth on all voices | No override — consistency for live use |

Q4 is locked across the pack so users can always ride Q4 as the "space" knob regardless of kit.

### XPN File Naming

```
ONSET_Drum_Essentials_[KitName].xpn
```

Pack folder: `ONSET Drum Essentials/`
Subfolder: `ONSET Drum Essentials/Kits/`
Cover art: `ONSET Drum Essentials/Artwork/` (Electric Blue `#0066FF` theme, ONSET logo)

---

## Seance Demands — Pack Compliance

From the XPN Tools Seance (5 demands — all must be addressed):

| Demand | Compliance in This Pack |
|--------|------------------------|
| Transitional morph snapshots | XVC Live kit includes pad variants at Blend 0.0, 0.25, 0.5, 0.75, 1.0 for Kick and Snare |
| Dry variants | Every kit includes NW corner (MPCe) or pads 09/13 (standard) as dry variants |
| Stereo width velocity | Foundation, XO_OX Signature, Club Fusion: stereo width increases with velocity (mono at velo <40, full width at velo 100+) |
| Emotional engine distinctness | Each kit has a 1-sentence emotional brief in the .xpn metadata field `"characterBrief"` |
| Invisible intelligence | Round-robin cycles include micro-pitch variation of ±4 cents between cycles (human drummer inconsistency model) |

---

*XO_OX Designs | ONSET Drum Essentials | First commercial expansion | April 2026*
*Electric Blue `#0066FF` | The attack transient, synthesized from first principles*
