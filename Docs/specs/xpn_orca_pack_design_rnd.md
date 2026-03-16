<!-- rnd: orca-pack -->

# XPN Pack Design: ORCA — First Dedicated Pack

**Engine**: ORCA | **Prefix**: `orca_` | **Color**: Deep Ocean `#1B2838`
**Date**: 2026-03-16 | **Status**: R&D / Pre-Production

---

## 1. Pack Identity

**Pack Name**: *BREACH*
**Tagline**: "From the deep — and then gone."

*BREACH* is the inaugural ORCA pack. The name captures the engine's defining dramatic arc in a single word: the apex predator rising from total silence into explosive surface violence, then vanishing back into pressure and dark. No other word describes what ORCA does more completely.

**DNA Position**: ORCA is deep Oscar — high aggression (0.75), low warmth (0.35), high movement (0.7), high density (0.7). It occupies the extreme cold-aggressive-kinetic corner of the fleet. Where OBLONG is immovable mass and BITE is gritty warmth, ORCA is pure directed lethality with no decorative intent.

**Tier Recommendation**: Tier 2 — Signature Pack. 16 pads, premium price point (~$18). ORCA's extreme DNA position means the pack serves a specific producer archetype — cinematic, sound design, experimental hip-hop — and should be positioned accordingly, not buried in a budget bundle.

---

## 2. ORCA's Sonic Character

ORCA presets are defined by three signature elements working in arc:

**Wavetable Teeth**: ORCA's wavetable oscillator carries hard spectral edges — FM-adjacent brightness with sub-octave tracking that drops like a pressure gradient. Unlike warm wavetable engines, ORCA's tables are designed to feel anatomical: you hear bone, cartilage, the blunt geometry of mass at speed.

**Echolocation Rhythm**: The engine's echolocation modulation creates internal rhythmic pulsing — clicks that scan the frequency space and return. These are not decorative tremolos. They are searching. At low velocities they feel investigative; at high velocities they collapse into a texture-blur that signals commitment.

**Hunt→Breach→Return Arc**: The defining structure of every ORCA preset is temporal. The engine is designed to move through three emotional states: patience (low energy, deep sub content), acceleration (mid-range clarity rising, echolocation increasing), and surface breach (full spectral aggression, transient attack, maximum movement). Return is the cool-down: resonant, heavy, receding. Every pad in this pack maps to one phase of that arc.

**Deep Ocean Pressure**: ORCA sounds heavy even when it is quiet. The low-end density (0.7) never fully evacuates — it establishes presence before the breach arrives.

---

## 3. Pack Structure — 16 Pads

### Row A: Hunt Phase (Pads 1–4)
*Patient. Submerged. Absolute stillness that is not absence.*

- **A1 — Thermocline**: Cold sub drone, minimal movement, wavetable locked to root. Waiting architecture.
- **A2 — Pod Silence**: Sparse echolocation pulse, barely audible. The group is present but not announcing.
- **A3 — Depth Pressure**: Dense low-mid texture, static envelope. Physically heavy without aggression.
- **A4 — Slow Stalk**: Gradual wavetable sweep, extended attack. Three seconds before you know it moved.

### Row B: Pursuit Phase (Pads 5–8)
*Accelerating. Echolocation active. The distance between hunter and hunted shrinking.*

- **B5 — Click Train**: Rhythmic echolocation pulse, mid-frequency, tempo-agnostic but internally driven.
- **B6 — Closing Vector**: Rising wavetable scan, movement = 0.6 and climbing. Velocity increases scan speed.
- **B7 — Subsurface Wake**: Turbulent mid texture, density peaking. The water is disturbed.
- **B8 — Terminal Approach**: Tight envelope, bright harmonic edge arriving, echolocation near-maximum rate.

### Row C: Breach Phase (Pads 9–12)
*Explosive. Full surface. Maximum aggression. The moment of contact.*

- **C9 — First Breach**: Hard transient, full wavetable teeth, aggression at ceiling (0.95). The loudest moment in the pack.
- **C10 — Aerial**: Suspended mid-air — top octave shimmer with residual sub rumble. The brief float before re-entry.
- **C11 — Impact Return**: Water-entry crash, dense low-mid explosion, movement maximum. Everything happening at once.
- **C12 — Secondary Surge**: Slightly softer breach variant, second in a series. Confirmation that the first was not accidental.

### Row D: Return Phase (Pads 13–16)
*Resonant. Satisfied. The predator descending with its catch into depth.*

- **D13 — Descent Arc**: Long release, falling wavetable sweep, sub content reemerging as mid clears.
- **D14 — Cold Bloom**: Slow reverb tail, harmonic resonance stretching wide. The water settling.
- **D15 — Deep Memory**: Static texture at depth, echolocation gone quiet. The pod reforms in silence.
- **D16 — Return to Dark**: Near-silence. Sub only. Full cycle complete.

---

## 4. Preset Design Philosophy

**Velocity Mapping**: ORCA presets treat velocity as depth. Quiet = submerged. Forte = breach. This is not metaphor — the velocity curve should physically sculpt the wavetable brightness, echolocation rate, and envelope attack across its full range. A pad played at pp should feel like 200 meters down. The same pad at ff should feel like open air.

**Macro Assignments (Hunt–Breach Arc)**:
- **Macro 1 — DEPTH**: Controls sub density and echolocation suppression. Full left = deepest submersion.
- **Macro 2 — HUNT**: Echolocation rate and mid-frequency scan range. Center = pursuing; right = committed.
- **Macro 3 — BREACH**: Wavetable aggression, transient hardness, attack sharpening. Right = full surface attack.
- **Macro 4 — PRESSURE**: Overall low-end density independent of phase. The ocean itself, always present.

---

## 5. MPCe Quad-Corner Design

ORCA's four quad corners map the hunt-breach arc spatially:

| Corner | Position | Character |
|--------|----------|-----------|
| **NW** | Hunt / Lurk | Sub-heavy, minimal movement, patience. Rows A–B quiet variants. |
| **NE** | Hunt / Pursue | Echolocation active, rising tension. Row B peak variants. |
| **SW** | Breach / Attack | Full aggression, maximum velocity. Row C. |
| **SE** | Return / Deep | Long release, descending arc. Row D. |

The NW→SW vertical sweep (lurk to breach) is the primary performance axis. The NW→NE horizontal sweep (lurk to pursue) governs the escalation before commitment. Producers should map these to MPCe touch strips for live dramatic control.

---

## 6. Pack DNA Profile

| Row | Phase | Aggression | Warmth | Movement | Density | Brightness | Complexity |
|-----|-------|-----------|--------|----------|---------|------------|------------|
| A (1–4) | Hunt | 0.30 | 0.20 | 0.15 | 0.75 | 0.10 | 0.25 |
| B (5–8) | Pursuit | 0.55 | 0.25 | 0.60 | 0.70 | 0.40 | 0.55 |
| C (9–12) | Breach | 0.95 | 0.30 | 0.90 | 0.65 | 0.85 | 0.70 |
| D (13–16) | Return | 0.40 | 0.35 | 0.25 | 0.80 | 0.20 | 0.35 |

Row C targets the extreme aggression/movement corner identified in fleet diversity analysis — this is intentional. No other pack in the catalog should occupy this exact zone. BREACH's Row C is the hardest, brightest, fastest content in the XO_OX library.

---

## 7. Coupling Recommendations

**ONSET** (Primary): ORCA's echolocation rhythm needs percussion anchor. ONSET's Kick and Perc voices provide the rhythmic grid that makes ORCA's internal pulse legible rather than chaotic. BREACH Row B over an ONSET pattern is the archetypal pairing.

**OBLONG** (Foundational)**: ORCA is movement; OBLONG is mass. Layering ORCA pursuit pads over OBLONG's static sub-bass creates the physical sensation of something enormous accelerating through dense medium. ORCA supplies the drama; OBLONG supplies the weight that makes the drama credible.

**OUROBOROS** (Chaos Pair): For producers who want ORCA past the breach and into dissolution — OUROBOROS's chaotic feedback structures extend the Row C material into something genuinely destabilizing. The pairing is extreme and intentional: hunt, breach, unravel.

**Avoid pairing with**: OPAL (granular warmth cancels ORCA's cold aggression), OLE (cultural warmth creates tonal contradiction). ORCA is a solo hunter or pairs with engines that reinforce cold/structural/rhythmic properties.
