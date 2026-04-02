# XO Chord Machine — World-Class Design Document

**Date:** March 2026
**Phase:** Concept — Ready for Architecture Review
**Status:** Design complete, pending user approval

---

## 1. Vision Statement

> **The XO Chord Machine distributes chord voicings across XOceanus's 4 engine slots, so each tone in a harmony is shaped by a different synthesis character. You play one key — four engines voice the chord.**

No other instrument does this. Divisimate distributes MIDI across *instances* of the same plugin. The NDLR generates patterns across *identical* outputs. Ableton's Chord device stacks intervals on *one* synth. The XO Chord Machine is the first instrument where the **timbral identity of each chord tone is a creative parameter** — where the root is a warm analog bass (OBLONG), the fifth is a gritty bitcrushed snarl (OBESE), the seventh is a drifting pad (ODYSSEY), and the octave is chip arpeggiation (OVERWORLD). And all four are coupled through the MegaCouplingMatrix while they voice the harmony.

This is the feature that makes XOceanus's 4-slot architecture musically inevitable, not just technically interesting.

---

## 2. Why This Is World-Class

### 2.1 The Gap in Every Existing Tool

| Tool | What it does | What it can't do |
|------|-------------|-------------------|
| **Divisimate** | Splits MIDI across plugin instances | Can't couple engines — each instance is isolated |
| **NDLR** | Generates drone + arp + pad layers | Locked to GM sounds, no timbral routing |
| **Scaler 2** | Chord detection + performance modes | One output, one timbre |
| **Ableton Chord** | Adds intervals to incoming MIDI | Intervals share the same synth |
| **Captain Chords** | Progression builder | Arranging tool, not a real-time instrument |
| **Elektron** | Per-step parameter locks | Per-step, not per-voice across engines |

**The XO Chord Machine closes this gap:** real-time, timbral-aware chord voicing distributed across coupled synthesis engines.

### 2.2 Why It Belongs in House Music

House music is built on chords. The genre's emotional power comes from:
- **Sustained pads** (Dorian minor 7ths, 9ths, 11ths)
- **Stab patterns** (off-beat chord hits, filtered and rhythmically gated)
- **Voice movement** (upper structures drifting while the root holds)
- **Timbral contrast in harmony** (bright stab + warm pad + sub bass = the classic sound)

Every great house producer already does timbral chord distribution — they just do it by stacking 3-4 synth tracks in the DAW. The XO Chord Machine makes this a **single-instrument, real-time, playable experience**.

### 2.3 The Brian Eno Principle

> "The best instruments make you feel like you're discovering music rather than constructing it."

The Chord Machine should feel like exploration, not programming. You load four engines, enable a chord mode, play a single note — and the harmonic texture that emerges feels discovered, not assembled. The coupling matrix ensures that these voices don't just coexist — they interact. ODYSSEY's LFO modulates OBESE's filter. OBLONG's amplitude shapes OPAL's grain density. The harmony isn't four isolated tones. It's an ecosystem.

---

## 3. Core Concepts

### 3.1 Timbral Voicing

Traditional voicing theory distributes notes across a pitch range. **Timbral voicing** distributes notes across *synthesis characters*:

| Voicing Role | Traditional | Timbral (XO Chord Machine) |
|-------------|-------------|---------------------------|
| Root | Low octave | Slot 1 — warm, grounding engine (OBLONG, OVERBITE) |
| Color | 3rd/7th | Slot 2 — expressive, morphing engine (ODYSSEY, ODDOSCAR) |
| Tension | Extensions (9th, 11th, 13th) | Slot 3 — textural, unusual engine (OPAL, OVERWORLD) |
| Sparkle | Upper octave/doubling | Slot 4 — bright, animating engine (ODDFELIX, OBESE) |

The engine assignment IS the voicing decision. Loading OVERDUB in Slot 1 instead of OBLONG doesn't just change the tone — it changes the entire harmonic character because OVERDUB's tape echo smears the root, creating a fundamentally different foundation.

### 3.2 Intelligent Distribution Modes

The Chord Machine needs multiple strategies for distributing chord tones across slots:

#### Mode 1: ROOT-SPREAD (Default — the house music mode)
```
Input: C minor 7 (Cm7)
Slot 1: C3  (root — always gets the fundamental)
Slot 2: Eb4 (3rd — voiced up an octave for separation)
Slot 3: G4  (5th)
Slot 4: Bb4 (7th)
```
Root is anchored low, remaining tones spread upward. Classic house voicing — the sub holds the floor while upper tones float.

#### Mode 2: DROP-2
```
Input: Cm7
Slot 1: Bb3 (7th — dropped down)
Slot 2: C4  (root)
Slot 3: Eb4 (3rd)
Slot 4: G4  (5th)
```
Drop the second-from-top note down an octave. Creates open, jazz-influenced voicings. The standard for Rhodes chords, neo-soul, lo-fi house.

#### Mode 3: QUARTAL (Modern house / techno)
```
Input: C (root only — quartal generates from root)
Slot 1: C3
Slot 2: F3  (P4 up)
Slot 3: Bb3 (P4 up again)
Slot 4: Eb4 (P4 up)
```
Stacks perfect 4ths. No 3rd — harmonically ambiguous, modern, works over anything. The sound of deep house and melodic techno.

#### Mode 4: UPPER STRUCTURE (Advanced — jazz house)
```
Input: C7 (dominant)
Slot 1: C2  (root — sub bass territory)
Slot 2: E4  (3rd of C7)
Slot 3: Ab4 (root of Ab triad — the upper structure)
Slot 4: C5  (5th of Ab triad, which = root up 2 octaves)
```
A triad over a different root. Creates complex extended chords that sound sophisticated without needing to think about 9ths, 11ths, or 13ths. The user just picks a root and a mode.

#### Mode 5: UNISON (Performance mode)
```
Input: C3
Slot 1: C3
Slot 2: C3
Slot 3: C3
Slot 4: C3
```
All engines play the same note. Not a chord — but with four different engines, it's four different timbres stacked on the same pitch. Useful for massive leads and coupled unison textures.

### 3.3 The Chord Palette

Rather than asking users to think in music theory terms, the Chord Machine presents chord types through a **palette of emotional qualities**:

| Palette Name | Chord Quality | House Context | Scale Basis |
|-------------|--------------|---------------|-------------|
| **WARM** | Minor 7th | Deep house, emotional house | Dorian |
| **BRIGHT** | Major 7th | Disco house, soulful house | Ionian / Lydian |
| **TENSION** | Dominant 7th | Acid house, tech house | Mixolydian |
| **OPEN** | Sus4 / Sus2 | Atmospheric house, progressive | Quartal |
| **DARK** | Minor 9th | Dark house, minimal techno | Aeolian |
| **SWEET** | Add9 / 6th | Lo-fi house, nu-disco | Dorian / Pentatonic |
| **COMPLEX** | 11th / 13th | Jazz house, broken beat | Dorian / Lydian |
| **RAW** | Power (root+5th) | Industrial, hard techno | None |

Users pick a palette quality and a root note. The system generates the voicing and distributes it. No music theory required — but it's all correct under the hood.

### 3.4 Voice Leading Engine

When chord changes happen (user plays a new root), the distribution must use **smooth voice leading** — each slot moves to its new note by the smallest possible interval:

```
Chord 1: Cm7           Chord 2: Fm7 (iv)
Slot 1: C3  → C3       (holds — common tone)
Slot 2: Eb4 → F4       (moves up a step)
Slot 3: G4  → Ab4      (moves up a half step)
Slot 4: Bb4 → C5       (moves up a step)
```

Without voice leading, the chord machine would jump all four engines to new positions — causing musical whiplash. With it, transitions feel like a keyboardist's hands smoothly repositioning. This is the difference between "four engines triggered at once" and "an instrument playing chords."

**Algorithm:** Closest-note reassignment with slot-affinity weighting. For each new chord:
1. Compute all candidate notes for the new chord quality
2. For each slot, find the candidate note closest to its current pitch
3. Assign notes to minimize total pitch movement (bipartite matching)
4. Apply assignment, using per-engine portamento if available

Root slot (Slot 1) gets priority — it always gets the root or bass note. Remaining slots resolve by proximity.

---

## 4. Sequencer Architecture

### 4.1 The XO Step Machine

The sequencer is a 16-step chord progression sequencer — not a note sequencer. Each step defines:

```
Step {
    rootNote: int          // MIDI note (root of chord)
    palette: PaletteType   // WARM, BRIGHT, TENSION, etc.
    voicing: VoicingMode   // ROOT-SPREAD, DROP-2, QUARTAL, etc.
    velocity: float        // 0-1, affects all slots proportionally
    gate: float            // 0-1, step duration relative to step length
    active: bool           // step on/off

    // Per-slot overrides (optional — leave null for auto)
    slotOctave: [int?; 4]  // override octave per slot
    slotMute: [bool; 4]    // mute individual slots per step
}
```

### 4.2 Parameter Locks (Elektron-Inspired)

Any engine parameter can be "locked" to a specific value on a specific step. This is the Elektron Digitone's killer feature, brought to multi-engine chords:

```
Step 5:
  Root: D  |  Palette: TENSION  |  Voicing: DROP-2
  Locks:
    - Slot 2 → drift_filterCutoff: 2000 Hz  (filter sweep on the 3rd)
    - Slot 3 → opal_grainDensity: 80/s      (grain burst on the 5th)
    - Slot 4 → fat_crushBits: 4              (bitcrush the 7th)
```

Parameter locks turn the chord sequencer into a per-voice timbral animation tool. Each step can reshape not just which notes play, but *how each engine voices its note*.

Implementation: Locks are stored as `std::vector<ParameterLock>` per step. On step advance, locked parameters are written to their APVTS atomic floats. On step exit, they revert to their preset values (stored at sequence start).

### 4.3 Rhythm Patterns

Pre-built gate patterns for common house rhythms:

| Pattern | Gate Sequence (16 steps) | Style |
|---------|-------------------------|-------|
| **FOUR** | `X...X...X...X...` | Four-on-the-floor stab |
| **OFF** | `.X...X...X...X..` | Off-beat classic house |
| **SYNCO** | `X..X..X...X..X..` | Syncopated deep house |
| **CHORD-STAB** | `X.X...X.X...X.X.` | Daft Punk-style rhythmic chords |
| **GATE** | `XXXXXXXXXXXXXXXX` | Sustained pad (gate = 1.0) |
| **PULSE** | `X.......X.......` | Half-note hits |
| **BROKEN** | `X..X.X....X..X..` | Broken beat / UK garage |
| **REST** | `................` | Silent (for breakdown building) |

Users select a pattern, then override individual steps. The patterns are starting points — suggestions, not constraints.

### 4.4 Clock & Sync

- Internal clock: BPM from 80-160 (house range), with tap tempo
- DAW sync: Lock to host transport via `getPlayHead()->getPosition()`
- Step length: 1/16 (default), 1/8, 1/4, 1/32, triplet variants
- Swing: 0-100% (delays even-numbered steps)
- Humanize: 0-100% (adds random timing jitter ±10ms, velocity variation ±15%)

**Sidechain awareness:** An optional "sidechain duck" toggle that briefly attenuates chord output on beats 1 and 3 (the kick positions in 4/4). This mimics the sidechain compression that is universal in house music, without requiring an actual compressor. The duck curve is a quarter-sine shape, 20-60ms attack, 80-200ms release.

---

## 5. Coupling Integration

### 5.1 Why Coupling Makes This Unprecedented

Without coupling, the Chord Machine is a fancy MIDI distributor. With coupling, it's a living harmonic organism.

**Scenario:** A Cm7 chord across four engines.
```
Slot 1: OBLONG  → C3 (root — warm analog bass)
Slot 2: ODYSSEY → Eb4 (3rd — pad with Voyage Drift LFO)
Slot 3: OPAL    → G4 (5th — frozen grain cloud)
Slot 4: ODDFELIX  → Bb4 (7th — percussive pluck)
```

Now add coupling routes:
```
OBLONG (root) → AmpToFilter → ODYSSEY (3rd)
  Effect: The bass pulse opens the pad's filter on each note attack.
  Musical: The chord "breathes" — the 3rd brightens when the root hits.

ODYSSEY (3rd) → LFOToPitch → OPAL (5th)
  Effect: ODYSSEY's slow LFO gently detunes OPAL's grain pitch.
  Musical: The 5th drifts microtonally, creating beating/chorus.

ODDFELIX (7th) → AmpToChoke → nothing (reserved)
  Effect: Available for user routing.

OPAL (5th) → AudioToWavetable → ODYSSEY (3rd)
  Effect: OPAL's grain output feeds back into ODYSSEY's wavetable.
  Musical: The 3rd's timbre is colored by the 5th's grain texture.
```

**Result:** The four chord tones are not independent — they modulate each other in real-time. The harmony has *movement* that emerges from the coupling, not from manual automation. This is a chord that interacts with itself.

### 5.2 Normalled Coupling Presets for Chords

The Chord Machine should offer "coupling presets" — pre-configured coupling routes optimized for harmonic interaction:

| Coupling Preset | Routes | Character |
|----------------|--------|-----------|
| **CLEAN** | No coupling | Four independent voices, transparent |
| **BREATHE** | Root AmpToFilter → all others | Chord filter follows bass rhythm |
| **ORBIT** | Each slot LFOToPitch → next slot (circular) | Microtonal drift, beating, alive |
| **SWARM** | All AudioToRing → neighbors | Dense ring-mod harmonics, metallic |
| **GRAIN-WASH** | All AudioToWavetable → OPAL slot (if present) | Everything feeds the granular engine |
| **CALL-RESPONSE** | Odd slots AmpToChoke → even slots | Rhythmic alternation between voice pairs |

### 5.3 Coupling Automation per Step

Parameter locks (Section 4.2) can also lock coupling amounts per step:

```
Step 1: coupling[0→1].amount = 0.8  (strong root→3rd filter coupling)
Step 5: coupling[0→1].amount = 0.1  (loose — voices separate for the iv chord)
```

This means the coupling relationships themselves evolve with the chord progression.

---

## 6. UI / UX Design

### 6.1 Design Philosophy: Teenage Engineering Meets the Gallery

> **Constraint breeds creativity.** — The UI should feel immediate, playful, and constrained. Not a spreadsheet of parameters — a musical surface.

The Chord Machine UI is a **new panel** that replaces the engine detail view when active. It occupies the same screen real estate as a single engine's controls. It is NOT a popup or dialog — it's a first-class view in the Gallery Model.

### 6.2 Layout

```
┌─────────────────────────────────────────────────────────┐
│  CHORD MACHINE                          [OFF] [ON]      │
│  ─────────────────────────────────────────────────────── │
│                                                          │
│  ┌──────────────────────────────────────────────────┐   │
│  │           CHORD STRIP (horizontal)                │   │
│  │                                                   │   │
│  │  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐                │   │
│  │  │ S1  │ │ S2  │ │ S3  │ │ S4  │                │   │
│  │  │ C3  │ │ Eb4 │ │ G4  │ │ Bb4 │                │   │
│  │  │OBLNG│ │ODYSS│ │OPAL │ │ODDFX│                │   │
│  │  │ ██  │ │ ██  │ │ ██  │ │ ██  │  ← accent bars │   │
│  │  └─────┘ └─────┘ └─────┘ └─────┘                │   │
│  │                                                   │   │
│  │  PALETTE: [WARM ▼]    VOICING: [DROP-2 ▼]        │   │
│  │  ROOT: [ C ▼]  OCTAVE: [3 ▼]                     │   │
│  └──────────────────────────────────────────────────┘   │
│                                                          │
│  ┌──────────────────────────────────────────────────┐   │
│  │           STEP GRID (16 columns)                  │   │
│  │                                                   │   │
│  │  ┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐               │   │
│  │  │▓▓││  ││▓▓││  ││▓▓││  ││▓▓││  │ ...×16        │   │
│  │  └──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘               │   │
│  │  C   C   C   C   D   D   F   F   ← root per step │   │
│  │                                                   │   │
│  │  PATTERN: [OFF-BEAT ▼]  SWING: ◉─── 55%          │   │
│  │  BPM: ◉───── 124       GATE:  ◉──── 75%          │   │
│  └──────────────────────────────────────────────────┘   │
│                                                          │
│  ┌──────────────────────────────────────────────────┐   │
│  │           CONTROLS (knobs row)                    │   │
│  │                                                   │   │
│  │  (SPREAD)  (LEAD)  (DUCK)  (HUMAN)  (LOCK)       │   │
│  │   ◉ voicing ◉ voice ◉ side- ◉ human ◉ p-lock    │   │
│  │     width    lead    chain   ize      depth       │   │
│  └──────────────────────────────────────────────────┘   │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

### 6.3 Visual Language

- **Chord Strip:** Four cards showing each slot's current note, engine name, and engine accent color as a bar at the bottom. When a step plays, the cards flash subtly. Voice leading motion is shown as animated note changes.
- **Step Grid:** 16-step sequencer with filled cells for active steps. Active step has a playhead marker. Steps with parameter locks show a small diamond icon. Steps with root changes show the root letter below.
- **Color:** The panel background is Gallery white `#F8F6F3`. The step grid uses XO Gold `#E9C46A` for active steps (since the chord machine is a coupling-level feature). Engine accent colors appear in the chord strip cards.
- **Typography:** Step numbers in JetBrains Mono. Palette/voicing labels in Space Grotesk. Root notes in Inter Bold.

### 6.4 Interaction Model

**Playing the Chord Machine:**
1. Load 4 engines into the slots (as normal)
2. Toggle CHORD MACHINE → ON
3. Select a palette (WARM), voicing (ROOT-SPREAD), and root (C)
4. Play a single MIDI note → all 4 slots voice the chord
5. Play different notes → the root changes, voice leading resolves smoothly

**Playing the Sequencer:**
1. Set BPM (or sync to DAW)
2. Select a rhythm pattern (OFF-BEAT)
3. Set a root note per step (or leave all on auto → follows played MIDI)
4. Press play → the sequencer fires chord stabs across all 4 engines

**Keyboard Mode:**
When the Chord Machine is active, the MIDI keyboard behavior changes:
- **Notes C1-B2** (low octave) → Select root note (the chord follows)
- **Notes C3-B5** (playing range) → Trigger the chord at that root, real-time
- **Notes C6-B6** (high octave) → Select palette (C=WARM, D=BRIGHT, E=TENSION, etc.)

This split-keyboard approach means the user can perform real-time chord changes while the sequencer runs, using their left hand for root selection and right hand for triggering.

### 6.5 The "One Knob" Experience

For users who don't want to think about voicing theory, the **SPREAD** knob is the single most important control:

```
SPREAD = 0%:  All slots play the same note (UNISON mode)
SPREAD = 25%: Root + 5th only (power chord, 2 unique notes)
SPREAD = 50%: Root + 3rd + 5th (triad, 3 unique notes)
SPREAD = 75%: Root + 3rd + 5th + 7th (full chord, 4 unique notes)
SPREAD = 100%: Root + 3rd + 5th + 7th with octave spread (wide voicing)
```

One knob. Zero theory. Instant chords. The knob label changes contextually: at 0% it reads "UNISON", at 50% "TRIAD", at 100% "WIDE". The SPREAD knob also maps well to a macro.

---

## 7. Generative & Performance Features

### 7.1 Chord Memory (Classic Technique)

House producers have used "chord memory" since the DX7:
1. User holds a chord on the keyboard (e.g., Cm7 across 4 keys)
2. Press MEMORY → the system captures the interval structure
3. Now any single note triggers the full chord transposed to that root

In the XO Chord Machine, this maps directly to the voicing distribution. The user plays 4 notes, the system captures which engine gets which interval, and from that point on, single-note input produces the full distribution.

### 7.2 Eno Mode (Generative)

Inspired by Brian Eno's "Oblique Strategies" and Music for Airports:

When ENO mode is active, the sequencer introduces controlled randomness:
- **Root drift:** Each cycle, one step's root may shift by ±1 scale degree (5% chance per step)
- **Voicing rotation:** The voicing mode may rotate (ROOT-SPREAD → DROP-2 → QUARTAL) across cycles
- **Slot swap:** Two slots may exchange their notes (not their engines) — same harmony, different timbral distribution
- **Rest injection:** Occasional steps are muted, creating space

The result is a chord progression that slowly evolves, never exactly repeating, but always staying harmonically coherent. Perfect for ambient house, dub techno, and generative live performance.

### 7.3 MIDI Learn Integration

The Chord Machine's key parameters map to standard MIDI CCs:

| CC | Parameter | Range |
|----|-----------|-------|
| 16 | Root note offset | -12 to +12 semitones |
| 17 | Palette select | 0-7 (8 palettes) |
| 18 | Voicing mode | 0-4 (5 modes) |
| 19 | Spread | 0-127 → 0-100% |
| 20 | Swing | 0-127 → 0-100% |
| 21 | Humanize | 0-127 → 0-100% |
| 22 | Sidechain duck depth | 0-127 → 0-100% |

### 7.4 Per-Slot Velocity Curves

Not all chord tones should be equally loud. The Chord Machine offers per-slot velocity scaling:

| Preset | Slot 1 | Slot 2 | Slot 3 | Slot 4 | Character |
|--------|--------|--------|--------|--------|-----------|
| **EQUAL** | 100% | 100% | 100% | 100% | Flat, all voices equal |
| **ROOT HEAVY** | 100% | 70% | 60% | 50% | Bass-forward, house default |
| **TOP BRIGHT** | 60% | 70% | 80% | 100% | Upper tones dominate, airy |
| **V-SHAPE** | 100% | 60% | 60% | 100% | Root + top bright, middle recessed |
| **RANDOM** | ?% | ?% | ?% | ?% | Per-step randomized velocities |

---

## 8. Preset Integration

### 8.1 Chord Machine State in .xometa

The Chord Machine state is stored in the `.xometa` preset format as an optional section:

```json
{
  "schema_version": 2,
  "name": "Midnight Progression",
  "mood": "Atmosphere",
  "engines": ["BobEngine", "DriftEngine", "OpalEngine", "SnapEngine"],

  "chordMachine": {
    "enabled": true,
    "palette": "WARM",
    "voicing": "DROP-2",
    "rootNote": 60,
    "spread": 0.75,

    "sequence": {
      "bpm": 122,
      "swing": 0.55,
      "stepLength": "1/16",
      "pattern": "OFF",
      "steps": [
        { "root": 60, "active": true, "gate": 0.8, "velocity": 0.9 },
        { "root": 60, "active": false },
        { "root": 60, "active": true, "gate": 0.6, "velocity": 0.7 },
        "..."
      ],
      "locks": [
        { "step": 4, "slot": 2, "param": "drift_filterCutoff", "value": 3000 }
      ]
    },

    "velocityCurve": "ROOT_HEAVY",
    "humanize": 0.15,
    "sidechainDuck": 0.3,
    "enoMode": false,

    "couplingPreset": "BREATHE"
  },

  "sonicDNA": { "brightness": 0.4, "warmth": 0.8, "movement": 0.6, "density": 0.5, "space": 0.7, "aggression": 0.1 }
}
```

### 8.2 Factory Chord Machine Presets

A dedicated preset category for Chord Machine configurations:

| Preset Name | Engines | Palette | Voicing | Pattern | Character |
|-------------|---------|---------|---------|---------|-----------|
| "Midnight Keys" | OBLONG+ODYSSEY+OPAL+ODDFELIX | WARM | DROP-2 | OFF | Deep house Rhodes |
| "Stab Protocol" | ODDFELIX+OBESE+ONSET+ODDOSCAR | TENSION | ROOT-SPREAD | CHORD-STAB | Classic house stab |
| "Quartal Dawn" | ODYSSEY+ODYSSEY+OPAL+OPAL | OPEN | QUARTAL | GATE | Ambient pad |
| "Acid Memory" | OBESE+OBESE+ODDFELIX+OVERWORLD | TENSION | ROOT-SPREAD | SYNCO | Acid house |
| "Chip Hymn" | OVERWORLD×4 | SWEET | DROP-2 | FOUR | NES church organ |
| "Grain Cathedral" | OPAL+OPAL+ODYSSEY+OBLONG | DARK | QUARTAL | GATE | Granular pad |
| "Dub Chords" | OVERDUB+OBLONG+ODDOSCAR+ODDFELIX | WARM | ROOT-SPREAD | OFF | Dub house |
| "Call & Response" | ODDFELIX+ODDOSCAR+ODDFELIX+ODDOSCAR | BRIGHT | DROP-2 | BROKEN | UK garage |
| "Eno Drift" | ODYSSEY+OPAL+OBLONG+ODDOSCAR | WARM | QUARTAL | GATE | Generative ambient (Eno on) |
| "Machine Funk" | OBESE+ONSET+ODDFELIX+OVERWORLD | RAW | ROOT-SPREAD | SYNCO | Industrial |

---

## 9. Technical Architecture

### 9.1 Module Placement

```
┌─ XOceanusProcessor ──────────────────────────────────────┐
│                                                           │
│  ┌─ ChordMachine ──────────────────────────────────┐     │
│  │                                                  │     │
│  │  ChordDistributor                                │     │
│  │  ├── PaletteEngine (chord quality lookup)        │     │
│  │  ├── VoicingEngine (note assignment per slot)    │     │
│  │  └── VoiceLeadingEngine (smooth transitions)     │     │
│  │                                                  │     │
│  │  StepSequencer                                   │     │
│  │  ├── StepData[16]                                │     │
│  │  ├── ParameterLockStore                          │     │
│  │  ├── ClockSource (internal / host sync)          │     │
│  │  └── SwingEngine                                 │     │
│  │                                                  │     │
│  │  ChordMachineState (serializable, .xometa)       │     │
│  │                                                  │     │
│  └──────────────────────────────────────────────────┘     │
│                                                           │
│  MIDI In ──→ ChordMachine ──→ 4× MidiBuffer ──→ Engines │
│              (distributes)     (one per slot)              │
│                                                           │
│  Engines[0..3] ──→ MegaCouplingMatrix ──→ MasterFX ──→  │
│                                                           │
└───────────────────────────────────────────────────────────┘
```

### 9.2 Data Flow

1. **MIDI arrives** at XOceanusProcessor::processBlock()
2. If Chord Machine is OFF → MIDI routes to active engine slot as normal
3. If Chord Machine is ON:
   a. **Sequencer advances** (if running) or **live note received**
   b. **ChordDistributor** computes 4 note assignments based on palette + voicing + root
   c. **VoiceLeadingEngine** smooths transitions from previous chord
   d. **ParameterLockStore** applies any locks for the current step
   e. **4 MidiBuffers** are generated — one per slot, each containing its assigned note
   f. Each engine's renderBlock() receives its own MidiBuffer
4. **MegaCouplingMatrix** processes inter-engine coupling as normal
5. Output mixes and routes through MasterFX

### 9.3 Key Classes

```cpp
// Source/Core/ChordMachine.h

namespace xoceanus {

enum class PaletteType { Warm, Bright, Tension, Open, Dark, Sweet, Complex, Raw };
enum class VoicingMode { RootSpread, Drop2, Quartal, UpperStructure, Unison };
enum class RhythmPattern { Four, Off, Synco, ChordStab, Gate, Pulse, Broken, Rest };
enum class VelocityCurve { Equal, RootHeavy, TopBright, VShape, Random };

struct StepData {
    int rootNote = 60;
    bool active = true;
    float gate = 0.75f;
    float velocity = 1.0f;
    PaletteType palette = PaletteType::Warm;     // per-step override (optional)
    VoicingMode voicing = VoicingMode::RootSpread; // per-step override (optional)
    bool useGlobalPalette = true;
    bool useGlobalVoicing = true;
};

struct ParameterLock {
    int step;
    int slot;
    juce::String paramId;
    float value;
    float originalValue;  // stored for revert
};

struct ChordAssignment {
    std::array<int, 4> midiNotes;    // note per slot (-1 = silent)
    std::array<float, 4> velocities; // velocity per slot
};

class ChordDistributor {
public:
    // Given a root note, palette, and voicing mode, compute 4 slot assignments.
    ChordAssignment distribute(int rootNote, PaletteType palette,
                               VoicingMode voicing, float spread) const;

    // Voice-lead from previous assignment to new one.
    ChordAssignment voiceLead(const ChordAssignment& previous,
                              const ChordAssignment& target) const;
};

class StepSequencer {
public:
    void prepare(double sampleRate);

    // Advance by numSamples. Returns step events that occurred in this block.
    // Called on the audio thread — no allocation.
    struct StepEvent { int step; int sampleOffset; };
    void processBlock(int numSamples, std::vector<StepEvent>& events);

    void setBPM(float bpm);
    void setSwing(float swing);          // 0-1
    void setStepLength(float divisions); // e.g., 4.0 for 1/16
    void setRunning(bool run);
    void syncToHost(double ppqPosition, double bpm, bool isPlaying);

private:
    std::array<StepData, 16> steps;
    double sampleRate = 44100.0;
    double phase = 0.0;
    float currentBPM = 122.0f;
    float swing = 0.0f;
    int currentStep = 0;
    bool running = false;

    // Pre-allocated event buffer (audio thread safe)
    std::vector<StepEvent> eventBuffer;
};

class ChordMachine {
public:
    void prepare(double sampleRate, int maxBlockSize);

    // Main audio-thread entry point.
    // Takes incoming MIDI, distributes across 4 output MidiBuffers.
    void processBlock(const juce::MidiBuffer& inputMidi,
                      std::array<juce::MidiBuffer, 4>& outputMidi,
                      int numSamples);

    // State
    void setEnabled(bool enabled);
    void setPalette(PaletteType p);
    void setVoicing(VoicingMode v);
    void setSpread(float spread);
    void setRootNote(int note);

    // Serialization
    juce::var toVar() const;
    void fromVar(const juce::var& data);

private:
    ChordDistributor distributor;
    StepSequencer sequencer;
    ChordAssignment currentAssignment;
    ChordAssignment previousAssignment;

    std::atomic<bool> enabled { false };
    std::atomic<int> rootNote { 60 };
    PaletteType palette = PaletteType::Warm;
    VoicingMode voicing = VoicingMode::RootSpread;
    float spread = 0.75f;

    // Per-slot MIDI buffers (pre-allocated)
    std::array<juce::MidiBuffer, 4> slotMidi;

    // Parameter lock storage
    std::vector<ParameterLock> locks;

    // Voice leading state
    bool hasActiveChord = false;
};

} // namespace xoceanus
```

### 9.4 Audio Thread Safety

All Chord Machine operations follow XOceanus rules:
- **No allocation** in processBlock — step events use a pre-allocated vector (reserved in prepare())
- **No locks** — palette/voicing/root changes use atomic reads or block-rate snapshot caching
- **Block-constant parameters** — palette lookups and voicing computation happen once per block, not per sample
- **MIDI buffers pre-allocated** — the 4 output MidiBuffers are member variables, cleared and reused each block

### 9.5 File Structure

```
Source/Core/
  ChordMachine.h          // ChordMachine class (inline DSP)
  ChordDistributor.h      // Voicing logic, palette tables, voice leading
  StepSequencer.h         // 16-step clock + parameter locks
  ChordMachineState.h     // Serialization to/from .xometa

Source/UI/
  ChordMachinePanel.h     // Main UI panel (replaces engine detail)
  ChordStripComponent.h   // The 4-slot chord visualization
  StepGridComponent.h     // 16-step grid with click editing
```

---

## 10. Implementation Roadmap

### Phase 1: Core Distribution (1 session)
- `ChordDistributor` with all 5 voicing modes
- `ChordMachine::processBlock()` routing MIDI to 4 slots
- Palette lookup tables (8 chord qualities)
- SPREAD knob (single control, 0-100%)
- Wire into XOceanusProcessor (bypass when OFF)

### Phase 2: Sequencer (1 session)
- `StepSequencer` with 16-step clock
- 8 rhythm patterns
- BPM + swing + gate
- DAW host sync via getPlayHead()
- Per-step root note editing

### Phase 3: Voice Leading + Polish (1 session)
- Voice leading engine (closest-note algorithm)
- Chord Memory (capture live voicing)
- Per-slot velocity curves
- Sidechain duck

### Phase 4: Parameter Locks (1 session)
- Lock storage per step
- Lock application/revert on step advance
- UI: click-and-hold on step to enter lock editing mode
- Coupling amount locks

### Phase 5: UI (1-2 sessions)
- ChordMachinePanel layout
- ChordStripComponent with accent-colored cards
- StepGridComponent with click-to-toggle
- Knob row (SPREAD, LEAD, DUCK, HUMAN, LOCK)
- Step grid visual: playhead, locks, root labels

### Phase 6: Presets + Generative (1 session)
- Chord Machine state serialization in .xometa
- 10 factory Chord Machine presets
- Eno mode (controlled randomness)
- MIDI CC mapping

### Phase 7: QA + Refinement (1 session)
- Audio thread safety audit
- CPU profiling (voice leading algorithm optimization)
- Edge cases: engine hot-swap during active sequence, empty slots
- Preset compatibility (existing presets load with chordMachine: null)

**Total estimate: 7-8 sessions** from architecture to ship.

---

## 11. What Makes This World-Class

1. **No other instrument does timbral chord distribution.** Divisimate distributes MIDI; the XO Chord Machine distributes *character*.

2. **The coupling matrix makes harmony interactive.** Chord tones modulate each other through the same coupling routes that define XOceanus's identity. The chords are alive.

3. **Zero theory required, full theory available.** The SPREAD knob and palette system let anyone make chords. The voicing modes, parameter locks, and Eno mode reward deep exploration.

4. **It's a house music instrument first.** Not a generic chord tool — every default, every pattern, every preset is tuned for 120-128 BPM, 4/4 time, and the emotional vocabulary of house music.

5. **It makes the 4-slot architecture musically essential.** Without the Chord Machine, 4 slots is an organizational convenience. With it, 4 slots is a harmonic instrument. Each slot is a voice in a chord. The architecture becomes the instrument.

6. **Roger Linn's philosophy: immediacy over flexibility.** The UI is a 16-step grid, not a piano roll. The controls are knobs and selectors, not menus and dialogs. You should be making music within 30 seconds of turning it on.

7. **Elektron's killer feature, generalized.** Parameter locks per step, but applied to 4 different synthesis engines simultaneously. Every step can reshape not just the chord, but the timbral character of every voice in the chord.

---

## 12. Open Questions for Review

1. **Should the Chord Machine be a "10th engine" or a processor-level feature?** Current design places it at the processor level (above engines), which feels right — it's a MIDI distribution layer, not a synthesis engine. But if it were an engine, it could receive coupling. Decision: **Processor-level** (recommended).

2. **Maximum polyphony interaction:** If each engine has 8-voice polyphony, and the Chord Machine sends one note to each engine, that's 4 voices total. But if the user plays 4 notes on the keyboard with the sequencer running, should each note create a full 4-engine chord (16 total voices)? Or should it be monophonic (one chord at a time)? **Recommendation: Monophonic chord sequencer with a "poly" toggle for advanced use** (default mono, poly = each played note creates a chord, max 2 simultaneous chords to limit CPU).

3. **Latency from voice leading computation:** The closest-note algorithm with bipartite matching is O(n!) for 4 slots — but n=4 means only 24 permutations. This is trivially fast, even on the audio thread. Not a concern.

4. **Name: "Chord Machine" or something more XO?** Candidates:
   - XO Chord Machine (descriptive, clear)
   - XOchord (follows XO naming)
   - The Distributor (evocative, unique)
   - Harmonic Engine (generic)
   - **Recommendation: "Chord Machine"** — it's clear, it's what producers call this workflow, and "machine" echoes drum machines, which is the right mental model.

---

## 13. Advanced Features (v2 — Post-Launch Expansion)

These concepts emerged from deep competitive research and represent the most novel ideas in the chord sequencer space. They extend the core Chord Machine without complicating the v1 experience.

### 13.1 Strum Engine (Per-Slot Timing Offsets)

Instead of all 4 engines firing simultaneously, stagger their onset:

```
Strum = 0ms:   S1 ──── S2 ──── S3 ──── S4    (block chord, simultaneous)
Strum = 20ms:  S1 ─ S2 ── S3 ─── S4           (guitar strum, cascading)
Strum = -20ms: S4 ─ S3 ── S2 ─── S1           (reverse strum, top-down)
```

- **STRUM knob:** -100% to +100%. Negative = top-down, positive = bottom-up.
- At extreme values (±80-100ms), creates harp-like arpeggiation without a separate arpeggiator.
- Inspired by Fluid Chords 2's strum feature, but applied across 4 different synthesis engines — so the strum cascades across different timbres, not just different pitches.

### 13.2 Chord Planing (Parallel Harmony)

A **PLANE** toggle per step that transposes the entire voicing shape uniformly:

```
Step 1: Cm7 voicing (C, Eb, G, Bb)
Step 2: [PLANE +5] → same shape moved up 5 semitones (F, Ab, C, Eb)
Step 3: [PLANE +7] → same shape moved up 7 semitones (G, Bb, D, F)
```

This is the classic technique Disclosure, Kerri Chandler, and Julio Bashmore use — the same chord shape transposed in parallel motion. It produces "astoundingly complex and rich sounding" results from a simple voicing because the transposition creates non-diatonic harmonies. Engine assignments stay fixed — all voices move together.

### 13.3 Euclidean Chord Mode

Each engine gets its own **Euclidean rhythm pattern** on its chord tone:

```
Slot 1 (OBLONG, root):     Euclidean(8, 3)  = X . . X . . X .    (sparse bass)
Slot 2 (ODYSSEY, 3rd):    Euclidean(8, 5)  = X . X X . X X .    (denser mid)
Slot 3 (OPAL, 5th):     Euclidean(8, 7)  = X X X . X X X .    (near-constant)
Slot 4 (ODDFELIX, 7th):     Euclidean(8, 2)  = X . . . X . . .    (sparse accent)
```

All engines share the same chord but fire independently. The interlocking rhythms create complex textures from a single harmony — what modular synth composers call "polymetric chord decomposition." Different loop lengths per engine (4, 6, 8, 12 steps) create phase relationships that evolve over time.

Based on Godfried Toussaint's 2005 research and the Torso T-1's implementation.

### 13.4 Chord Morphing (Inter-Step Voice Glide)

Between chord steps, individual voices can **glide at different rates**:

```
Step 1 → Step 2:
  Slot 1 (bass):    C3 → F3   glide: 10ms  (arrives first — anchors the change)
  Slot 2 (mid):     Eb4 → Ab4  glide: 80ms  (mid-speed — smooth transition)
  Slot 3 (upper):   G4 → C5   glide: 200ms (slow arrival — smears through)
  Slot 4 (top):     Bb4 → Eb5  glide: 400ms (slowest — last to resolve)
```

The result is a **harmonic smear** — the chord doesn't change all at once, it dissolves from one to the next with each engine resolving at its own pace. Inspired by Fluid Chords 2's chord bending, but distributed across 4 different synthesis characters.

Implementation: per-slot portamento time applied only during chord transitions. Uses existing engine portamento where available, or adds pitch bend messages to the engine's MidiBuffer with a ramp.

### 13.5 Tension Parameter (Per-Step Harmonic Tension)

A **TENSION** knob per step (0-100%) that modifies the chord quality:

```
Tension = 0%:   Pure chord as written (Cm7)
Tension = 25%:  Add 9th (Cm9)
Tension = 50%:  Tritone substitution hint (Gb dominant mixtures)
Tension = 75%:  Sus4 replacement on the 3rd (creates pull toward resolution)
Tension = 100%: Cluster — all voices within a whole step (maximally tense)
```

Inspired by 2024 academic research on explicit tonal tension conditioning in AI music generation (arXiv:2511.19342). The research shows that a user-drawn "tension arc" across a progression produces musically compelling results. The simplified version — one knob per step — brings this concept to a real-time instrument.

The **global TENSION knob** draws the arc across all 16 steps. The **per-step override** (via parameter lock) allows fine control.

### 13.6 Kerri Chandler Voicing Mode (Genre-Specific Library)

A 6th voicing mode specifically implementing Kerri Chandler's documented techniques:

```
CHANDLER mode:
  - Third-inversion 7th chords (7th in bass)
  - Omit the 5th, replace with 11th or 13th
  - Pedal bass option (Slot 1 holds a static root while upper voices change)
  - Close voicing in upper register (all upper tones within one octave)
```

This directly addresses the research finding that "no tool has genre-specific voicing libraries." The Chandler mode captures the exact voicing language of deep house's most documented harmonic practitioner.

Future genre modes: **DAFT PUNK** (parallel power chords + filter sweep), **MJ COLE** (2-step UK garage voicings with diminished passing chords), **LARRY HEARD** (close-clustered jazz voicings, counter-rhythmic bass).

### 13.7 Chord Memory with Timbral Capture

An extension of the basic Chord Memory (Section 7.1):

1. User plays a chord on the keyboard → system captures the intervals
2. **Timbral Memory:** Also captures which engine voiced which interval
3. From that point, single notes produce the full chord with the same engine-to-interval mapping
4. **Import from DAW:** Analyze incoming MIDI (4-note chords) and auto-assign to engines by register: lowest note → Slot 1, highest → Slot 4

This bridges the gap between "I know the chords I want" and "I want the Chord Machine to voice them across engines."

---

## 14. Competitive Research Summary

### What Every Existing Tool Is Missing

From comprehensive analysis of Cthulhu, Scaler 2/3, Captain Chords, Divisimate 2, Chordjam, Fluid Chords 2, NI Komplete Kontrol, Sugar Bytes Consequence, Torso T-1, Polyend Tracker, Roland MC-707, and Ableton Chord:

| Gap | Filled by XO Chord Machine? |
|-----|-----------------------------|
| Cross-engine voice distribution with timbre awareness | **Yes** — core feature |
| Automatic smooth voice leading across chord changes | **Yes** — VoiceLeadingEngine |
| Genre-specific voicing vocabularies | **Yes** — Palette system + Chandler mode |
| Per-step voicing control in step sequencer | **Yes** — parameter locks |
| Multi-engine synchronization | **Yes** — built into 4-slot architecture |
| Integration of rhythm and harmony | **Yes** — Euclidean Chord Mode |
| Sidechain-aware harmonic treatment | **Yes** — sidechain duck |
| Chord morphing between steps | **Yes** — inter-step voice glide (v2) |
| Harmonic tension arc control | **Yes** — Tension parameter (v2) |

### Closest Competitors and Why They Fall Short

**Divisimate 2** is the closest: 512-output MIDI distribution with expert arpeggiator. But it's purely a MIDI router — no synthesis, no coupling, no harmonic awareness, no genre voicings. It also requires a complex multi-instance setup.

**Scaler 3** added Divisi voice routing and plugin hosting. But its internal sounds are basic GM quality, the voice routing is limited, and it has no concept of engines with distinct timbral characters.

**Consequence** (Sugar Bytes) proved the concept: 3 instruments with chord sequencer. But its fixed sound library and confusing GUI limited adoption.

**The XO Chord Machine is Divisimate's voice routing + Scaler's chord intelligence + Elektron's parameter locks + Consequence's multi-engine concept, unified in a single instrument that already has 9 world-class synthesis engines and a coupling matrix.**

---

## 15. Reference Sources

Research compiled from 50+ sources including:
- Sound On Sound reviews (Divisimate 1 & 2, Scaler 2)
- Attack Magazine technique series (Kerri Chandler chords, ensemble voicings, parallel chord stabs)
- Roland Articles (Mr. Fingers "Can You Feel It" production analysis)
- MusicRadar reviews (Chordjam, Torso T-1)
- KVR Audio forums (Cthulhu, Captain Chords user feedback)
- VI-Control forums (Divisimate user experience)
- Bedroom Producers Blog (Scaler 3 review)
- Perfect Circuit (Hapax review, multitimbral synthesis)
- Magnetic Magazine (Torso T-1 review)
- arXiv 2024 (tension-conditioned chord generation)
- Godfried Toussaint 2005 (Euclidean rhythm theory)
- EDMProd (tension techniques in electronic music)
- Stealify Sounds, Mixed In Key, LiveSchool (house chord progressions)
- Harmony Central (UK garage production techniques)

---

*XO_OX Designs | Feature: Chord Machine | Accent: XO Gold #E9C46A | Prefix: cm_*
