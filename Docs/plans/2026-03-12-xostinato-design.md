> **STATUS: COMPLETED** | OSTINATO shipped 2026-03-18 and seance completed 2026-03-20. See `Docs/seances/ostinato_seance_verdict.md`.

# XOstinato — Design Document

**Date:** March 12, 2026
**Phase:** 0 — Ideation complete
**Status:** Approved for Phase 1 architecture

---

## Identity

- **Name:** XOstinato
- **Gallery code:** OSTINATO
- **Accent color:** Firelight Orange `#E8701A`
- **Parameter prefix:** `osti_`
- **Thesis:** Communal drum circle engine — 8 seats, 12 world percussion instruments, hybrid physical modeling. The fire at the center of the circle.
- **Values:** Multiculturalism, peace, unity, love, community, family
- **Max voices:** 16 (2 per seat)
- **CPU budget:** <25% (all 8 seats active with rolls)
- **Personality in 3 words:** Communal, Welcoming, Alive
- **Sound family:** Rhythmic / Percussive / World / Groove

**What only XOstinato can do:**
A self-playing world drum circle where 8 physically-modeled instruments from different traditions lock into interlocking ostinato patterns — and you can sit down in any seat and join live. No other synth engine models the communal interaction between percussion traditions.

---

## The XO Concept Test

1. **XO word:** XOstinato — an ostinato is a repeating musical pattern, the heartbeat of every drum circle ✓
2. **One-sentence thesis:** "XOstinato is a communal drum circle engine where 8 physically-modeled world percussion instruments play interlocking ostinato patterns — and you can sit down and join." ✓
3. **Sound only this can make:** A self-playing, physically-modeled world drum circle with inter-seat interaction — djembe, tabla, and taiko players reacting to each other's accents in real time. No DAW plugin or XOmnibus engine does this. ✓

---

## Gallery Gap Filled

| Existing engines | Synthesis dimension |
|-----------------|---------------------|
| ODDFELIX, ODDOSCAR, ODYSSEY, OBLONG, OBESE, OVERBITE, OVERWORLD | Harmonic (oscillators, wavetables, FM, samples) |
| OVERDUB | Temporal FX (delays, tape echo) |
| ONSET | Algorithmic drum synthesis (FM/Modal/KS/PhaseDist) |
| OPAL | Granular time manipulation |
| **OSTINATO** | **Communal world percussion — physical modeling + pattern interaction** |

ONSET is a drum machine. OSTINATO is a drum gathering. ONSET synthesizes individual drum hits algorithmically. OSTINATO models the physical instruments AND the communal interaction between players.

---

## Signal Architecture

### Per-Seat DSP Chain

```
EXCITER (per articulation)
├── Slap: Short broadband noise burst, sharp attack
├── Tone: Filtered impulse, mid-frequency emphasis
├── Bass: Low-passed impulse, center-strike energy
├── Muted: Damped impulse, hand stays on membrane
│
▼
MODAL MEMBRANE (6-8 tuned bandpass resonators)
├── Resonator ratios define the drum type
├── Articulation selects which modes are excited
├── Decay rates per mode (slap = short, tone = ring)
├── Pitch param shifts all ratios together
│
▼
WAVEGUIDE BODY (short delay-line cavity)
├── Length = body size (djembe goblet vs taiko barrel)
├── Damping = body material (wood vs clay vs metal)
├── Opening = port size (open bottom vs closed box)
│
▼
RADIATION FILTER (output character)
├── LP roll-off models distance from drum head
├── Presence peak for cut and projection
│
▼
PER-SEAT OUTPUT → into FX Chain
```

### FX Chain

```
8 Seat Outputs
    │
    ▼
CIRCLE SPATIAL ENGINE
├── 8 seats positioned evenly around stereo field
├── osti_circleWidth: 0 = mono center, 1 = full L/R spread
├── osti_circleRotate: rotates all positions (automate for spinning circle)
├── Seat 1 at 0°, Seat 2 at 45°, Seat 3 at 90°... Seat 8 at 315°
├── Distance modeling: subtle volume + LP filter per seat position
│
    ▼
FIRE STAGE (warmth saturation)
├── Soft-clip saturation, analog tape + tube warmth
├── osti_fireDrive: 0 = clean, 1 = bonfire glow (never harsh, always warm)
├── osti_fireTone: dark (campfire intimacy) ↔ bright (festival energy)
├── Character: even harmonics emphasized — warmth, not distortion
│
    ▼
GATHERING REVERB
├── Algorithmic reverb tuned for percussion (tight early reflections)
├── osti_reverbSize: 0 = small room → 1 = outdoor amphitheater
├── osti_reverbDamp: high-freq absorption
├── osti_reverbMix: dry/wet
├── Pre-delay scales with size (5ms → 40ms)
│
    ▼
PULSE COMPRESSOR
├── Groove-reactive — sidechain listens to combined seat output
├── osti_pulseMix: 0 = off, 1 = heavy pump
├── osti_pulseRelease: fast (tight snap) ↔ slow (breathing, wave-like)
├── Makes 8 independent players feel like ONE organism
│
    ▼
MASTER OUTPUT (stereo)
```

---

## Instrument Roster (12 instruments × 3-4 articulations)

| # | Instrument | Origin | Articulations | Character |
|---|-----------|--------|---------------|-----------|
| 1 | Djembe | West Africa | Tone, Slap, Bass, Ghost | Goblet body, goatskin head. The voice of the circle |
| 2 | Dundun | West Africa | Open, Muted, Stick, Bell | Cylindrical double-headed. The deep pulse |
| 3 | Conga | Afro-Cuban | Open, Slap, Muted, Heel-Toe | Tall stave body. Warm, singing resonance |
| 4 | Bongos | Cuba | Open (Hembra), Open (Macho), Slap, Rim | Paired high/low. Bright fills and chatter |
| 5 | Cajón | Peru/Flamenco | Bass, Snare, Tap, Ghost | Wooden box. Intimate, modern, snare buzz on top |
| 6 | Taiko | Japan | Center (Don), Edge (Ka), Rim (Teke), Thunder | Barrel body. Massive power, ceremony |
| 7 | Tabla | India | Na, Tin, Tun, Tirakita | Paired drums. Melodic, extraordinary pitch expression |
| 8 | Doumbek | Middle East | Doum, Tek, Ka, Roll | Goblet ceramic. Sharp, bright, intricate |
| 9 | Frame Drum | Global | Open, Finger, Thumb, Shake | Bodhrán/Tar/Riq hybrid. Ancient, universal |
| 10 | Surdo | Brazil | Open, Muted, Rim, Roll | Deep wide barrel. Samba heartbeat, carnival |
| 11 | Tongue Drum | Modern | Strike 1-4 (pitched) | Steel tank. Melodic, meditative, peaceful |
| 12 | Beatbox | Street/Global | Kick, Snare, Hat, FX | Human voice synthesis. Ultimate inclusivity |

**48 total articulations.** Same modal+waveguide DSP chain for all drums — identity comes from modal ratios, waveguide length, and exciter shapes.

### How Articulations Map to DSP

- **Modal ratios:** Djembe = 1.0, 1.58, 2.08, 2.65 (goblet harmonics). Taiko = 1.0, 1.35, 1.66, 2.0 (barrel). Each drum has researched partial ratios.
- **Waveguide length:** Tabla bayan = long (deep cavity). Bongos = short (small shell). Cajón = medium (box).
- **Exciter per articulation:** Slap = 2ms broadband burst. Tone = 5ms filtered pulse. Bass = 8ms low-passed. Muted = damped modes after exciter.
- **Tongue Drum special case:** Modal resonators tuned to specific pitches (pentatonic), making it melodic.
- **Beatbox special case:** Exciter uses shaped noise + formant filter to mimic vocal tract.

---

## Parameter Architecture

### Per-Seat Parameters (~14 per seat × 8 seats = ~112)

| Parameter | Range | Purpose |
|-----------|-------|---------|
| `osti_seatN_instrument` | 0-11 | Which of 12 drums |
| `osti_seatN_pattern` | 0-7 | Pattern selection |
| `osti_seatN_level` | 0-1 | Seat volume |
| `osti_seatN_pan` | -1 to 1 | Override circle position |
| `osti_seatN_pitch` | -12 to +12 | Membrane tuning (semitones) |
| `osti_seatN_decay` | 0-1 | Modal ring time |
| `osti_seatN_bodySize` | 0-1 | Waveguide length |
| `osti_seatN_bodyDamp` | 0-1 | Material damping |
| `osti_seatN_exciterBright` | 0-1 | Exciter tone shaping |
| `osti_seatN_swing` | 0-1 | Per-seat swing amount |
| `osti_seatN_velocity` | 0-1 | Pattern hit intensity |
| `osti_seatN_humanize` | 0-1 | Timing drift per seat |
| `osti_seatN_active` | 0/1 | Seat on/off |
| `osti_seatN_density` | 0-1 | Pattern fill density |

### Global Parameters (~28)

| Parameter | Range | Purpose |
|-----------|-------|---------|
| `osti_tempo` | 40-200 | Circle BPM |
| `osti_masterLevel` | 0-1 | Master output |
| `osti_globalSwing` | 0-1 | Swing applied to all seats |
| `osti_globalHumanize` | 0-1 | Timing drift for all seats |
| `osti_circleWidth` | 0-1 | Stereo spread of seat positions |
| `osti_circleRotate` | 0-1 | Rotate seat positions in stereo field |
| `osti_fireDrive` | 0-1 | Warmth saturation amount |
| `osti_fireTone` | 0-1 | Saturation color (dark ↔ bright) |
| `osti_reverbSize` | 0-1 | Intimate room → outdoor festival |
| `osti_reverbMix` | 0-1 | Dry/wet |
| `osti_reverbDamp` | 0-1 | High-frequency absorption |
| `osti_pulseMix` | 0-1 | Groove compressor amount |
| `osti_pulseRelease` | 0-1 | Compressor breathing speed |
| `osti_macro_gather` | 0-1 | M1: Sync/tightness |
| `osti_macro_fire` | 0-1 | M2: Intensity/energy |
| `osti_macro_circle` | 0-1 | M3: Inter-seat interaction |
| `osti_macro_space` | 0-1 | M4: Environment scale |

**Total: ~140 canonical parameters** (112 per-seat + 28 global)

### Macro Mappings

| Macro | Label | Targets | Behavior |
|-------|-------|---------|----------|
| M1 | GATHER | All `seatN_humanize` (inverse), `seatN_swing` (converge), `globalSwing` | 0 = loose warmup, everyone drifting. 1 = locked grid, tight unison |
| M2 | FIRE | All `seatN_velocity`, `seatN_density`, `fireDrive`, `seatN_exciterBright` | 0 = ghost notes, gentle. 1 = full force, all hands, raucous |
| M3 | CIRCLE | Inter-seat accent sync, call-response triggers, density cross-modulation | 0 = independent players. 1 = deep conversation, seats reacting to each other |
| M4 | SPACE | `reverbSize`, `reverbMix`, `circleWidth`, `pulseMix` | 0 = intimate, dry, close. 1 = outdoor festival, wide, enveloping |

---

## Pattern System

### Pattern Library

Each instrument ships with 8 patterns rooted in authentic rhythmic traditions. **96 total patterns** (12 instruments × 8).

| Slot | Role | Example (Djembe) |
|------|------|-------------------|
| 0 | Signature | Kuku — most recognizable rhythm for that tradition |
| 1 | Pulse | Steady foundation groove |
| 2 | Syncopated | Off-beat accents, push-pull |
| 3 | Call | Lead phrase, asks a question |
| 4 | Response | Answer phrase, complements Call |
| 5 | Sparse | Minimal, breathing, space |
| 6 | Dense | Busy fills, high energy |
| 7 | Universal | Blends with any other tradition |

### Pattern Data Format

```
{
  step:         0-63        // position in 64-step grid (4 bars at 16th resolution)
  articulation: 0-3         // which of the 4 articulations
  velocity:     0.0-1.0     // hit strength
  probability:  0.0-1.0     // chance this step fires (enables natural variation)
}
```

### Live Override

```
Pattern playing → User hits MIDI note mapped to seat
               → Pattern MUTES for that seat
               → User plays live
               → No input for 2 bars
               → Pattern FADES BACK IN over 1 bar
```

- MIDI mapping: 8 seats × 4 notes = 32 MIDI notes. 4 notes per seat trigger the 4 articulations
- Velocity from MIDI controls hit intensity directly
- Fade-back prevents jarring transitions — the circle absorbs you back in

### CIRCLE Macro: Inter-Seat Interaction

- **Accent sync:** Strong hit on one seat → neighboring seats accent their next hit
- **Call-Response triggering:** Call patterns trigger Response patterns in paired seats
- **Density reaction:** Sparse seat causes adjacent seats to fill in, and vice versa
- **Implementation:** Each seat exposes `peakAmplitude`. Neighbors read peaks and modulate velocity/probability. Same concept as ONSET's XVC but tuned for musical conversation.

### GATHER Macro: Timing Behavior

| GATHER Value | Feel |
|-------------|------|
| 0.0 | ±30ms drift. Loose, human, warming up |
| 0.25 | ±15ms. Finding the groove |
| 0.5 | ±5ms. Locked in but breathing |
| 0.75 | ±2ms. Tight. The circle is ON |
| 1.0 | Grid-locked. Machine precision. Maximum power |

---

## Coupling Matrix (XOmnibus)

### OSTINATO as Target (receiving)

| Source Engine | Coupling Type | Musical Effect |
|-------------|---------------|----------------|
| OVERBITE | AmpToFilter | Bass amplitude opens Fire Stage drive — bass makes circle roar |
| ODYSSEY | EnvToMorph | Climax bloom sweeps GATHER macro — psychedelic tightening |
| OVERWORLD | AudioToFM | Chip audio FM-modulates drum exciters — 8-bit percussion |
| OPAL | AmpToFilter | Grain density modulates pattern density — clouds trigger fills |
| OBESE | AmpToFilter | 13-osc amplitude drives FIRE macro — massive energy injection |

### OSTINATO as Source (sending)

| Target Engine | Coupling Type | Musical Effect |
|-------------|---------------|----------------|
| OVERDUB | getSample | Drum circle through dub tape echo + spring — world dub |
| OVERBITE | AmpToFilter | Drum accents pump bass filter — rhythmic snarl |
| OPAL | AudioToWavetable | Drum hits scattered into grain clouds — percussion particles |
| ODYSSEY | AmpToFilter | Circle energy drives pad filter — drums breathe the pads |
| OBLONG | AmpToFilter | Drum dynamics shape warm texture — living ambient |

### Signature Coupling Routes

- **OSTINATO × OVERDUB** — "World Dub" — Entire drum circle through Jamaican dub FX
- **OSTINATO × OPAL** — "Scattered Gathering" — Drum hits granulated into particle clouds
- **ONSET × OSTINATO** — "Machine Meets Human" — Algorithmic drums alongside physical circle

### Coupling Types NOT Received

- `PitchToPitch` — drums tuned per-seat, external pitch creates chaos
- `AmpToChoke` — choking kills communal energy (antithetical to thesis)

---

## Preset Strategy

**120 factory presets:**

| Category | Count | Character |
|----------|-------|-----------|
| Heritage Circles | 20 | Single-tradition ensembles. Honor each tradition on its own |
| Global Gathering | 25 | Cross-cultural fusion. Everyone at the fire |
| Pulse & Foundation | 15 | 2-3 seats, deep pocket. Rhythmic bed for other engines |
| Festival | 20 | Maximum energy. 6-8 seats, high FIRE, raucous and joyful |
| Meditation | 15 | Tongue drum, frame drum, sparse, peaceful, breathing |
| Conversation | 15 | High CIRCLE macro. Call-response, seats reacting to each other |
| Coupling Showcase | 10 | Designed for XOmnibus pairings |

### Heritage Circle Presets (honoring traditions)

| Preset | Seats | Tradition |
|--------|-------|-----------|
| Kuku Circle | Djembe × 3 + Dundun | Guinean celebration |
| Rumba de Cajón | Conga × 2 + Bongos + Cajón | Afro-Cuban |
| Taiko Ensemble | Taiko × 2 + pitched variant | Japanese ceremonial |
| Tabla Jugalbandi | Tabla × 2 | North Indian duet |
| Samba Batucada | Surdo × 2 + Conga + Bongos | Brazilian carnival |
| Halqa | Doumbek × 2 + Frame Drum × 2 | Middle Eastern/North African |
| Bodhrán Session | Frame Drum × 3 + Tongue Drum | Celtic |
| ...and 13 more | | Covering every tradition in the roster |

### Naming Convention

Evocative, warm, human. Names feel like invitations:
- "Come Sit Down" — gentle intro, 4 seats, low FIRE
- "We're All Here" — all 8 seats, max FIRE
- "The Fire Is Warm" — tongue drum + frame drum, high SPACE
- "Call Across The Water" — djembe call, doumbek response
- "First Light" — surdo + cajón, dawn-energy groove
- "One Heartbeat" — all seats locked, max GATHER, unison pulse

### Mood Affinity

| Mood | Affinity | Why |
|------|----------|-----|
| Foundation | **High** | Drum circle IS foundation — rhythmic bedrock |
| Atmosphere | Medium | Meditation presets, sparse circles |
| Entangled | **High** | Coupling showcase presets |
| Prism | Medium | Bright festival energy |
| Flux | **High** | High CIRCLE macro, conversation — always moving |
| Aether | Low | Earthy, not ethereal (some tongue drum presets qualify) |

---

## UI Concept

### Page 1: The Circle View

```
              [Seat 1]
        [Seat 8]    [Seat 2]
                🔥
      [Seat 7]  FIRE  [Seat 3]

        [Seat 6]    [Seat 4]
              [Seat 5]
```

- 8 seat nodes arranged in a circle around a center fire element
- Each seat shows instrument icon, active state, pattern activity (pulses on hits)
- Click seat to select — bottom panel shows that seat's parameters
- Fire center glows brighter as FIRE macro increases
- 4 macro knobs across top: GATHER | FIRE | CIRCLE | SPACE
- Active patterns show as animated dots orbiting each seat
- Live MIDI override: seat node glows differently — "someone just sat down"

### Page 2: Pattern Editor

- Step grid for selected seat (64 steps)
- Articulation selector per step
- Probability slider per step
- Pattern library browser (8 patterns per instrument)

### Color Palette

- Warm dark background (charcoal brown, not pure black)
- Firelight Orange `#E8701A` accents
- Cream text
- Feels like nighttime around a fire

---

## Design Constraints

- Any instrument in any seat — no tradition restrictions. Total freedom. The circle doesn't care where you're from.
- Presets handle curated tradition combos; the user can always remix
- No walls. Everyone welcome. That is the engine's soul.
- Pattern library preserves cultural authenticity — real rhythmic traditions, not randomized hits

---

## Decision Gate: Phase 0 → Phase 1

- [x] Concept approved
- [x] XO word confirmed (XOstinato — ostinato = repeating pattern, heartbeat of drum circle)
- [x] Gallery gap clear (no communal world percussion engine)
- [x] Coupling partners defined (OVERDUB, OPAL, OVERBITE, ONSET, ODYSSEY)
- [x] Synthesis approach locked (hybrid: exciter + modal membrane + waveguide body)
- [x] Full design approved

**→ Proceed to Phase 1: Architecture**
*Invoke: `/new-xo-engine phase=1 name=XOstinato identity="Communal drum circle engine — 8 seats, 12 world percussion instruments, hybrid physical modeling" code=XOst`*

---

*XO_OX Designs | Engine: OSTINATO | Accent: #E8701A | Prefix: osti_*
