# XObbligato — Concept Brief

*Phase 0 | March 2026 | XO_OX Designs*
*Two mischievous water sprites play post-punk wind instruments — the goddess's sons*

---

## Identity

**XO Name:** XObbligato
**Gallery Code:** OBBLIGATO
**Accent Color:** Rascal Coral `#FF8A7A`
**Parameter Prefix:** `obbl_`
**Plugin Code:** `Xobl`
**Engine Dir:** `Source/Engines/Obbligato/`

**Thesis:** XObbligato is a dual wind instrument engine where two brothers — Peter (bright flute-family winds) and the Wolf (dark reed-family winds) — play together, fight, cry, and make up. Each brother has his own FX chain. The BOND macro tells their entire emotional story.

**Sound family:** Hybrid — wind textures / post-punk leads / dual-voice interaction instrument

**Unique capability:** Per-voice FX splitting combined with the BOND sibling interaction system. No other engine in the gallery models the emotional dynamics between two independent voice engines. The BOND macro creates a continuously evolving relationship — from harmony to rivalry to vulnerability to reconciliation — that shapes every DSP parameter simultaneously. Combined with physical-modeled wind instruments from traditions worldwide, XObbligato creates post-punk wind duets where the brothers' relationship IS the sound.

---

## Aquatic Identity

Flying fish. Two of them. Sons of the siphonophore goddess who holds the surface.

Flying fish live at the exact boundary their mother defines — the air-water interface. But where the siphonophore stays put, patient and vast, her sons can't sit still. They're always launching from the water, gliding through air on translucent wing-fins, splashing back down, chasing each other in spiraling arcs above and below the surface. They breathe water but they fly through air. Wind instruments made flesh.

Brother A is the bright one — Peter. He catches the light when he leaps. His wing-fins are iridescent blue-silver, catching feliX's neon shimmer. He plays the high winds: flute, shakuhachi, bansuri, pan flute, tin whistle. Quick, darting, melodic. He's the bird in Prokofiev's fairy tale — always moving, always announcing himself. His FX chain is the air above: chorus shimmer, bright delay like echoes off clouds, plate reverb like open sky. New wave energy — angular arpeggios, staccato runs, the bright side of post-punk where Talking Heads meets Devo meets the B-52s.

Brother B is the dark one — the Wolf. He glides longer, lower, closer to the water's surface. His wing-fins are darker, more opaque. He plays the low winds: oboe, duduk, ney, didgeridoo, bassoon, suona. Growling, resonant, menacing. He's the wolf and the grandfather — patient, lurking, then suddenly powerful. His FX chain is the water below: phaser sweeps like undercurrents, dark delay like echoes in a cave, spring reverb like sound bouncing off a rock wall, tape saturation like pressure. Post-punk energy — bass drones, moody reeds, the dark side where Joy Division meets Bauhaus meets Siouxsie.

But the brothers' defining feature isn't their individual voices — it's their RELATIONSHIP. The BOND macro tells the story of two siblings who can't live with each other and can't live without each other. At BOND=0 they're in harmony — playing together, matching each other's phrases, consonant intervals, complementary FX. As BOND rises they start competing — one ducks the other, pitch spread widens, FX diverge. By BOND=0.5 they're fighting — dissonant clashing, aggressive sidechain, distortion creeping in. At BOND=0.65 one brother takes it too far — crushing the other's voice, dominating the mix. At BOND=0.8 both get hurt — voices drop, fragile, detuned, unstable. At BOND=0.9 they're crying — wide trembling vibrato, soft and vulnerable, minor intervals, everything drenched in reverb. At BOND=1.0 they make up — voices reconverge, tenderness, warmth, unison.

The cycle is the performance. The player drives the drama. And because making up sounds like the beginning of harmony, the arc can loop forever — brothers who fight and love and fight and love, the eternal sibling bond.

The Rascal Coral accent is the warm complement of their mother's Siren Seafoam — family connection through color theory. Coral is aquatic. Warm is energetic. It's the color of a flying fish's belly catching sunset light as it leaps from the water.

---

## Polarity

**Position:** SUNLIT SHALLOWS — just below their mother's surface, always trying to leap up to her level
**feliX-Oscar balance:** 65% feliX / 35% Oscar — energetic, transient, bright, but with emotional depth underneath (the crying, the vulnerability, the making up)

---

## Lineage

**Mother:** XOrphica (the siphonophore goddess at THE SURFACE)
**Relationship:** XOrphica splits by register (bass/treble — horizontal). XObbligato splits by voice (Brother A/Brother B — vertical). Same dual-FX philosophy, different split axis. The goddess holds the horizontal boundary between worlds. Her sons hold the vertical duality between personalities.

**Generation:** Third Generation — New Niches. The brothers evolved from their mother's surface-spanning architecture but specialized into wind instrument territory that no other species occupies.

---

## DSP Architecture

```
MIDI Note ──→ Voice Splitter
              (notes assigned to Brother A, Brother B, or both)
              |                                    |
    ┌─────────┴──── BROTHER A ────────┐  ┌────────┴──── BROTHER B ────────┐
    │ "Peter" — the bright rascal     │  │ "The Wolf" — the dark rascal   │
    │                                 │  │                                │
    │ Exciter A (air-jet family)      │  │ Exciter B (reed family)        │
    │   breath → noise shaping        │  │   breath → reed nonlinearity   │
    │   embouchure → spectral tilt    │  │   embouchure → reed stiffness  │
    │         ↓                       │  │         ↓                      │
    │ Waveguide A (bright bore)       │  │ Waveguide B (dark bore)        │
    │   cylindrical tube model        │  │   conical tube model           │
    │   tone holes → harmonic series  │  │   bore shape → formant color   │
    │   overblow → register jump      │  │   overblow → growl             │
    │         ↓                       │  │         ↓                      │
    │ Character A                     │  │ Character B                    │
    │   post-punk brightness          │  │   post-punk darkness           │
    │   angular attack shaping        │  │   gritty saturation            │
    │         ↓                       │  │         ↓                      │
    │ FX Chain A ("The Air")          │  │ FX Chain B ("The Water")       │
    │   Chorus (new wave shimmer)     │  │   Phaser (undercurrent sweep)  │
    │   Bright Delay (angular echo)   │  │   Dark Delay (cave echo)      │
    │   Plate Reverb (open sky)       │  │   Spring Reverb (rock wall)    │
    │   Harmonic Exciter (presence)   │  │   Tape Saturation (pressure)   │
    │         ↓                       │  │         ↓                      │
    └─────────┬───────────────────────┘  └────────┬───────────────────────┘
              │                                    │
              └──────────┬─────────────────────────┘
                         │
                   BOND Interaction Engine
                   (sidechain, pitch mod, level balance,
                    interval control, vibrato sync)
                         │
                         ↓
                   Master Output
```

### Wind Synthesis — Waveguide Physical Modeling

Each brother runs a simplified waveguide wind instrument model:

**Exciter (the breath):**
- Noise source shaped by envelope — the "blow" that starts and sustains the note
- Exciter type determines the instrument family:
  - Air-jet (Brother A): filtered noise burst, spectral tilt controls brightness
  - Reed (Brother B): noise through nonlinear waveshaping, stiffness controls growl
- Breath pressure (velocity/aftertouch) affects timbre, not just volume
- Overblow threshold: increase pressure beyond a point and the instrument jumps to a higher register (octave/12th depending on bore type)

**Waveguide (the tube):**
- Delay line + lowpass filter = the resonant air column
- Delay length = pitch (MIDI note)
- Damping filter = bore material and air absorption
- Bore shape parameter: cylindrical (clarinet-like, odd harmonics) vs conical (oboe-like, all harmonics)
- Tone hole model: simple allpass chain that shifts harmonic emphasis
- Reflection coefficient at bell: controls how much energy radiates vs reflects

**Key difference from mother's harp:** Wind notes SUSTAIN as long as you hold the key. XOrphica's strings decay. The goddess plucks and lets go. Her sons blow and hold on. Generational difference made audible.

### The Worldwide Wind Gallery

Each brother selects from a curated set of global wind instruments, implemented as waveguide presets (exciter type + bore parameters + character tuning):

**Brother A — Peter's Winds (bright, air-driven):**

| Instrument | Origin | Exciter | Bore | Character |
|-----------|--------|---------|------|-----------|
| Western Flute | Europe | Air-jet, wide | Cylindrical, open | Pure, clear, agile |
| Shakuhachi | Japan | Air-jet, breathy | Cylindrical, end-blown | Meditative, airy, wind-like |
| Bansuri | India | Air-jet, warm | Cylindrical, side-blown | Sweet, ornamental, flowing |
| Pan Flute | Andes | Air-jet, hollow | Short cylindrical, stopped | Breathy, hollow, haunting |
| Tin Whistle | Ireland | Air-jet, thin | Narrow cylindrical | Bright, quick, folk energy |
| Piccolo | Europe | Air-jet, piercing | Short cylindrical, open | Shrill, cutting, electric |
| Native American Flute | Americas | Air-jet, soft | Dual-chamber | Warm, spiritual, intimate |
| Recorder | Europe | Air-jet, gentle | Cylindrical, fipple | Sweet, simple, childhood |

**Brother B — The Wolf's Winds (dark, reed-driven):**

| Instrument | Origin | Exciter | Bore | Character |
|-----------|--------|---------|------|-----------|
| Oboe | Europe | Double reed, nasal | Conical | Quacking, expressive, THE Peter & Wolf voice |
| Duduk | Armenia | Double reed, warm | Cylindrical, wide | Deeply melancholic, ancient, mournful |
| Ney | Middle East | Air-edge, breathy | Conical, open | Spiritual, breathy, Sufi resonance |
| Didgeridoo | Australia | Lip buzz, drone | Conical, very long | Deep drone, circular breathing, primal |
| Bassoon | Europe | Double reed, heavy | Conical, folded | Grumpy, low, grandfather energy |
| Suona | China | Double reed, loud | Conical, flared bell | Piercing, festive, overwhelming |
| Bagpipe Chanter | Scotland | Single reed, bright | Cylindrical, drone | Insistent, modal, never stops |
| Zurna | Turkey | Double reed, sharp | Conical, short | Cutting, ceremonial, outdoor power |

### BOND Interaction Engine — The Signature System

The BOND macro (M2) controls a multi-parameter relationship model between the two brothers. Every stage affects multiple DSP parameters simultaneously:

| BOND Range | Stage | Level Balance | Interval | Internal Mod | FX Character | Vibrato |
|-----------|-------|---------------|----------|-------------|--------------|---------|
| 0.0 – 0.1 | **HARMONY** | Equal (50/50) | Unison / octave / 5th | None — matched | Complementary, moderate | Gentle, synchronized |
| 0.1 – 0.25 | **PLAYFUL** | Slight asymmetry (55/45) | Consonant, syncopated | Gentle call-and-response | Slightly diverged | Bouncy, independent |
| 0.25 – 0.4 | **COMPETITIVE** | Shifting (one rises, other dips) | Spread widening | Amplitude sidechain (rhythmic ducking) | Clearly different | Faster, competitive |
| 0.4 – 0.55 | **FIGHTING** | Volatile (rapid swings) | Dissonant (minor 2nd, tritone) | Aggressive sidechain + pitch mod | Clashing, both driven hard | Erratic, unstable |
| 0.55 – 0.7 | **TOO FAR** | Extreme (80/20 — one dominates) | Crushed interval | Dominant voice distorts the other | One overdriven, other thin | Dominant = none, crushed = shaking |
| 0.7 – 0.85 | **HURT** | Both drop (40/40) | Detuned, unstable | Both voices fragile, pitch drift | Both muted, filter closing | Wide, slow, unstable |
| 0.85 – 0.95 | **CRYING** | Very soft (20/20) | Minor 2nd / minor 3rd | Trembling, no interaction | Drenched in reverb, soft | Wide, trembling, vulnerable |
| 0.95 – 1.0 | **MAKING UP** | Reconverging (→50/50) | Resolving to unison | Warmth returning | Complementary, gentle | Settling, tender |

**Implementation:** The BOND value feeds a multi-breakpoint modulation table. Each destination parameter has its own breakpoint curve across the BOND range, interpolated smoothly. This creates organic, non-linear transitions between emotional states. The player controls the drama with a single gesture.

### FX Chain A — "The Air" (Brother A / Peter)

| Slot | DSP | Key Params | Character |
|------|-----|-----------|-----------|
| Chorus | Stereo chorus, 90° offset | Rate, depth, spread | New wave shimmer — the Cure, OMD |
| Bright Delay | Short-medium delay, HP in feedback | Time (sync), feedback, tone | Angular echo — staccato reflections off clouds |
| Plate Reverb | Dattorro plate, pre-LP bypass | Size, decay, mix | Open sky — wide, bright, airy |
| Harmonic Exciter | Parallel saturation + HP blend | Amount, frequency, mix | Presence and cut — makes the bright brother CUT |

### FX Chain B — "The Water" (Brother B / The Wolf)

| Slot | DSP | Key Params | Character |
|------|-----|-----------|-----------|
| Phaser | 6-stage allpass, feedback | Rate, depth, feedback | Undercurrent sweep — moody, slow, Bauhaus |
| Dark Delay | Long delay, LP in feedback | Time (sync), feedback, dampen | Cave echo — dark, murky, sound sinking |
| Spring Reverb | 6-allpass diffuser chain | Decay, dampen, drip | Rock wall — metallic, dark, physical |
| Tape Saturation | Asymmetric tanh + LP rolloff | Drive, warmth, compression | Pressure — the weight of water above |

### Voice Routing

Notes can be assigned to brothers in multiple ways:

| Mode | How Notes Split | Musical Use |
|------|----------------|-------------|
| **Alternate** | Odd notes → A, even notes → B | Every note switches brother — rapid interplay |
| **Split** | Below split point → B, above → A | Like mother's crossover — bass wolf, treble peter |
| **Layer** | Every note → both A and B | Both brothers play everything — maximum interaction |
| **Round Robin** | Rotate through A, B, A, B... | Sequential — one speaks, then the other |
| **Velocity** | Soft → A, hard → B | Playing dynamics choose the brother |

---

## Macro System

| Macro | Name | Controls | Musical Intent |
|-------|------|----------|---------------|
| M1 | **BREATH** | Exciter intensity + air amount + overblow threshold + noise color | How hard the brothers blow — gentle whisper → full forte → overblown screech → shattering |
| M2 | **BOND** | THE SIGNATURE — sibling relationship arc across all parameters | The emotional story: harmony → play → fight → too far → hurt → cry → make up |
| M3 | **MISCHIEF** | Post-punk processing amount + angular attack + character drive + rhythmic shaping | How punk the winds get — pure acoustic → angular new wave → full post-punk → punk chaos |
| M4 | **WIND** | FX depth for both chains + space + stereo spread | Dry intimate → processed → vast drenched soundscape |

---

## Voice Architecture

- **Max voices:** 12 — 6 per brother maximum (winds are monophonic instruments, but multiple notes create ensemble)
- **Voice stealing:** Oldest note per brother — each brother manages his own voice pool
- **Legato mode:** Yes — wind instruments naturally slur between notes. Legato within each brother independently.
- **Glide:** Portamento per brother — wind instruments bend between notes naturally. Rate controlled by BREATH.

---

## Coupling Thesis

XObbligato lives in the shallows, just below their mother. They're fast, darting, and interact with everything that passes through their territory. The brothers couple best with engines that provide rhythmic structure (giving them something to play over) or atmospheric beds (giving them space to dart through).

### As Coupling Source (XObbligato → others)

| Route | What It Sends | Partner | Musical Effect |
|-------|--------------|---------|---------------|
| `getSampleForCoupling()` | Combined brothers output | Any | Wind melody modulating other engines |
| Brother A output only | Bright wind signal | OPENSKY | Flute drives euphoric shimmer — birds in the sky |
| Brother B output only | Dark wind signal | OCEANDEEP | Reed drives sub-bass — wolf in the deep |
| BOND value | Relationship state as mod source | ORPHICA (mother) | Sons' drama moves mother's SURFACE — the family system |

### As Coupling Target (others → XObbligato)

| Route | Source | What It Does | Musical Effect |
|-------|--------|-------------|---------------|
| `AmpToFilter` | ONSET | Drum hits open/close the brothers' bores | Rhythmic wind — drums play the flutes |
| `EnvToMorph` | ORPHICA | Harp envelope → wind instrument selection morph | Mother's pluck chooses which instrument the sons play |
| `LFOToPitch` | ODYSSEY | Journey drift → pitch waver | Psychedelic wind — drifting, wandering melody |
| `AudioToFM` | OVERWORLD | Chip audio → exciter modulation | 8-bit wind — retro flutes with digital excitation |
| `AmpToFilter` | OVERBITE | Bass amplitude → bore openness | Bass weight opens the wind — breathing with the bass |

### Signature Pairings

| Pairing | Name | What Happens |
|---------|------|-------------|
| **ORPHICA → OBBLIGATO** | "Mother & Sons" | Harp plucks trigger wind notes — mother plays, sons respond. Family ensemble. |
| **OBBLIGATO → OVERDUB** | "Punk Dub Winds" | Post-punk wind through tape delay + spring reverb. Joy Division meets King Tubby. |
| **ONSET → OBBLIGATO** | "Wind Machine" | Drum patterns drive wind bore changes — rhythmic wind ensemble. |
| **OBBLIGATO → OPENSKY** | "Flight" | Flying fish brothers leaping into the sky — bright winds drive euphoric shimmer. |
| **ODYSSEY → OBBLIGATO** | "Wind Journey" | JOURNEY macro modulates BOND — as the journey progresses, the brothers' relationship evolves. |
| **OBBLIGATO × OBBLIGATO** | "Quartet" | Two instances = four brothers. Full wind ensemble with two independent BOND arcs. |

### Unsupported Coupling Types

| Type | Why |
|------|-----|
| `AmpToChoke` | Wind instruments need to breathe — choking kills the sustain that defines them |
| `AudioToWavetable` | The brothers are waveguide-based — wavetable input has no meaningful target |

---

## Signature Sound

Play a chord in Layer mode. Both brothers play every note — Peter's flute and the Wolf's oboe sounding together. The chord is full, warm, consonant. Now sweep BOND from 0 toward 0.5. The brothers start competing — Peter's flute gets louder, the Wolf's oboe ducks, then surges back. The intervals widen. Peter's FX chain brightens (chorus shimmer, echo bouncing), the Wolf's darkens (phaser sweeping, delay sinking). At BOND=0.5 they're fighting — dissonant intervals, aggressive sidechain pumping, both voices distorted by MISCHIEF. Keep pushing. At 0.65 the Wolf takes it too far — his oboe crushes Peter's flute, dominating the mix with dark growling reed. At 0.8 both brothers go quiet — fragile, detuned, pitch wandering. At 0.9 they're crying — wide trembling vibrato, both voices soft, drenched in reverb, a minor second interval that aches. At 1.0 they make up — voices reconverge to unison, vibrato settles, warmth returns.

You just told a complete emotional story with one macro sweep. No other engine does this.

Now switch to Split mode, turn up MISCHIEF, and play a bass line on the Wolf's reed while Peter's flute plays angular post-punk arpeggios above. The brothers are in different registers with different FX, playing different parts. It's a post-punk duo — bass clarinet and tin whistle playing a Joy Division song if Joy Division were flying fish.

---

## Peter and the Wolf — Design Reference

Prokofiev's Op. 67 assigns each character an instrument. XObbligato maps this to the brothers:

| Prokofiev Character | Instrument | Brother | Influence |
|-------------------|-----------|---------|-----------|
| Bird | Flute | A (Peter) | Quick, darting, high — Brother A's default energy |
| Duck | Oboe | B (Wolf) | Quacking, comedic, expressive — Brother B's playful side |
| Cat | Clarinet | B (Wolf) | Sneaky, slinky, register-jumping — Brother B's mischief |
| Grandfather | Bassoon | B (Wolf) | Heavy, grumpy, low — Brother B at low BREATH |
| Peter | Strings (→ adapted to winds) | A (Peter) | Heroic, melodic, brave — Brother A's core |
| Wolf | French Horns (→ adapted to winds) | B (Wolf) | Menacing, powerful — Brother B at high BREATH |
| Hunters | Timpani (→ ONSET coupling) | External | The drum engine driving the wind brothers |

The fairy tale IS the BOND macro. Peter and the Wolf start as enemies, go through conflict, and end in a triumphant procession together. Every performance of XObbligato is a retelling of Prokofiev's story.

---

## Visual Identity

- **Accent color:** Rascal Coral `#FF8A7A` — warm complement of mother's Siren Seafoam `#7FDBCA`. Family through color theory.
- **Material/texture:** Translucent wing-fins — like a flying fish's pectoral fin stretched thin against sunlight. Iridescent at the edges, warm coral center. Two overlapping fins (one brighter, one darker) for the two brothers.
- **Icon concept:** Two wind instruments (a flute and an oboe) crossed like swords — but playfully, like kids play-fighting. The instruments' bells form an X (the XO_OX mark).
- **Panel character:** Split panel — left side bright/airy (Brother A), right side dark/moody (Brother B). The BOND macro sits in the center where the two halves meet. As BOND increases, visual tension increases — colors clash, edges sharpen. At "crying," both sides soften and blur. At "making up," they blend together warmly.

---

## Mood Affinity

| Mood | Affinity | Why |
|------|----------|-----|
| **Foundation** | Medium | Harmony-state wind duets serve as melodic beds |
| **Atmosphere** | Medium | Sustained winds with deep WIND macro create atmospheric textures |
| **Entangled** | High | The BOND system IS entanglement — two voices in dynamic relationship |
| **Prism** | High | Peter's bright winds through chorus/shimmer = prismatic color |
| **Flux** | High | BOND sweeps create constantly shifting interaction — never static |
| **Aether** | Low | Wind instruments are too present/physical for aetheric stillness |

---

## Parameter Count Estimate

| Category | Params | Examples |
|----------|--------|---------|
| Brother A Waveguide | 6 | instrument select, bore shape, exciter type, brightness, overblow, embouchure |
| Brother B Waveguide | 6 | instrument select, bore shape, exciter type, darkness, overblow, reed stiffness |
| Voice Routing | 3 | mode (alt/split/layer/RR/velocity), split point, balance |
| BOND System | 3 | bond amount, bond speed (how fast transitions happen), bond asymmetry (which brother dominates) |
| FX Chain A | 6 | chorus rate+depth, delay time+feedback, reverb size+mix |
| FX Chain B | 6 | phaser rate+depth, delay time+feedback, reverb decay+mix |
| Macros | 4 | BREATH, BOND, MISCHIEF, WIND |
| Character | 4 | punk drive A, punk drive B, angular attack, rhythmic gate |
| Master | 3 | volume, pan, glide rate |
| **Total** | **~41** | Focused — the BOND system provides complexity without parameter bloat |

---

## Emily the Harpist — Inherited Design DNA

Like their mother's inspiration from Emily the Harpist, the brothers inherit the dual-FX philosophy but express it through their own personality:

1. **The split** — mother splits by register, sons split by identity. Same architecture, different axis.
2. **The bend** — wind instruments naturally bend between notes. Glide/portamento is core, not an add-on.
3. **The loop** — sustained winds don't need a loop engine — they sustain naturally. But the BOND system creates evolving loops of emotional state.
4. **The build** — start at BOND=0 (harmony), gradually increase through the drama, the performance arc from peace to chaos to vulnerability to resolution.
5. **The FX** — each brother's chain reflects his personality, just as mother's LOW/HIGH paths reflect the deep and the light.

---

*XO_OX Designs | XObbligato — two brothers, one bond, a fairy tale told in wind and punk*
