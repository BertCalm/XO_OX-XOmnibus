# XOstinato Engine Specification

**Date:** March 16, 2026
**Phase:** 1 — Architecture R&D
**Status:** V1 Scope — DSP build pending
**Gallery code:** OSTINATO
**Accent color:** Firelight Orange `#E8701A`
**Parameter prefix:** `osti_`
**Aquatic identity:** The drum circle on the shore — where land and water meet, where all species gather

---

## 1. Concept

XOstinato is a communal drum circle engine. Eight physically-modeled world percussion instruments play interlocking ostinato patterns — and you can sit down and join.

An ostinato is a repeating musical pattern. The magic of a drum circle comes from how individual ostinato patterns interlock, breathe, and evolve together. XOstinato does not just synthesize drums — it synthesizes the communal interaction between players. Each of the eight seats holds one instrument. Each instrument plays its own pattern. The patterns interlock. Players react to each other's accents in real time. The circle is always playing. You sit down and join. You leave, it carries on.

### Design Lineage

The engine draws from three distinct intellectual traditions:

**West African communal drumming** — specifically the Ewe Agbadza tradition, where interlocking bell patterns (the gankogui bell timeline), supporting patterns (kagan, kidi), and lead drum call-and-response create a communal sonic fabric where no single part is dominant. The whole is greater than any part. The circle holds.

**Steve Reich's phasing studies** — particularly "Drumming" (1971) and "Clapping Music" (1972). Reich demonstrated that phased polyrhythm — two identical patterns slowly drifting out and back into alignment — generates emergent musical complexity without compositional intervention. Phase relationships as a creative act. OSTINATO's entrainment system is a direct homage.

**ARP 2500 step sequencer** — the voltage-controlled step sequencer that made repeating patterns into expressive instruments. Patterns are not just playback — they are living, malleable things that can be stretched, compressed, and reshaped in real time.

### The Synthesis Philosophy

A drum circle is acoustic, physical, human. The synthesis must feel like real hands on real skin on real wood. Modal resonators capture membrane expressiveness. Waveguide bodies give each drum its physical cavity — the goblet of a djembe, the barrel of a taiko, the box of a cajón. Together they produce organic, living quality that samples cannot match and pure synthesis cannot reach alone.

**Signal path per seat:**
```
Impulse Exciter (noise burst + transient click)
        ↓
Modal Membrane Resonator (2D plate model)
        ↓
Waveguide Body Cavity (cylindrical or goblet)
        ↓
Radiation Filter (directional + room)
        ↓
Pattern Gate (rhythm engine controls note density/timing)
        ↓
Circle Spatial Engine (position in the 8-seat circle)
```

---

## 2. Architecture

### 2.1 The 8-Seat Circle

OSTINATO has eight seats arranged in a circle. Any of the twelve instruments can occupy any seat — there are no tradition restrictions. This creates a multicultural meeting ground: a djembe and a tabla can share the same circle. A taiko and a cajon can interlock.

Each seat is independent:
- Its own instrument choice
- Its own pattern selection (one of the 8 patterns per instrument)
- Its own phase offset relative to the global cycle
- Its own velocity sensitivity and humanization curve
- Its own position in the stereo/spatial field (governed by Circle Spatial Engine)
- Its own coupling reactivity (how much it listens to and responds to other seats)

**Polyphony:** 16 voices total — 2 per seat to support rolls and flams without voice stealing.

### 2.2 The 12 Instruments

| ID | Instrument | Origin | Body type | Articulations |
|----|-----------|--------|-----------|---------------|
| `DJEMBE` | Djembe | West Africa | Goblet | Bass, Tone, Slap, Open Slap |
| `TABLA` | Tabla (right hand / bayan pair) | South Asia | Cylindrical | Ge, Na, Tin, Ka |
| `TAIKO` | O-taiko | Japan | Barrel | Strike, Rim, Roll, Press |
| `CAJON` | Cajón | Peru/Flamenco | Box | Tap, Bass, Slap, Rim Knock |
| `DOUMBEK` | Doumbek (darbuka) | Middle East | Goblet | Doum, Tek, Ka, Snap |
| `CONGA` | Conga pair | Cuba/Africa | Cylindrical | Open, Muted, Slap, Heel-Toe |
| `UDU` | Udu pot drum | Igbo, West Africa | Vessel | Body, Hole, Side, Flick |
| `FRAME` | Riq / frame drum | Middle East/Mediterranean | Frame | Center, Edge, Roll, Shake |
| `TALKING` | Talking drum (dùndún) | Yoruba | Hourglass | Low, Mid, High, Pitch Slide |
| `KPANLOGO` | Kpanlogo | Ga, Ghana | Cylindrical | Open, Muted, Rim, Accent |
| `KENDANG` | Kendang (gendang) | Indonesia/Malaysia | Double-headed | Left, Right, Both, Rim |
| `BODHRAN` | Bodhrán | Ireland | Frame | Back, Tip, Ricochet, Strike |

### 2.3 The 96 Patterns

Each instrument has 8 patterns, rooted in authentic rhythmic traditions. Pattern naming follows a difficulty/energy axis:

| Slot | Character |
|------|-----------|
| P1 | Foundational — the first thing taught, the heartbeat |
| P2 | Supporting — locks in under the lead |
| P3 | Call — the phrase that invites response |
| P4 | Response — the answer |
| P5 | Build — increasing density, energy rising |
| P6 | Peak — maximum density, full ceremony |
| P7 | Break — deliberate space, the silence between |
| P8 | Chaos — intentional desynchronization, the edge |

### 2.4 Pattern Generator

Patterns are stored as binary rhythm grids with expressive metadata: velocity curves, timing offset distributions, and accent weights. They are not audio samples — they are performance instructions fed to the synthesis engine.

The pattern generator supports:
- **Cycle length:** 8, 12, 16, or 24 steps
- **Subdivision:** 1/8, 1/8T, 1/16, 1/16T
- **Density modulation:** real-time thinning or filling of the grid
- **Accent tracking:** which hits are strong, medium, weak
- **Timing humanization:** stochastic offset per hit drawn from ethnomusicological timing distributions (see Section 4)

### 2.5 Phase Tracking and Entrainment

The entrainment system is the heart of OSTINATO. It governs the phase relationships between all eight seats.

**Phase offset:** each seat has an `osti_seat{N}_phaseOffset` parameter — a fractional position within the global cycle where this seat's pattern begins. A phase offset of 0.25 means this seat starts one quarter of the way through the cycle relative to seat 1.

**Entrainment:** the `osti_entrainmentRate` parameter controls how fast phase-drifted seats are pulled back toward alignment. At 0 (pure Oscar), each seat drifts freely — the players are improvising in their own time. At 1 (pure feliX), all seats snap to perfect grid alignment — mathematical polyrhythm.

**Lead voice pull:** seat 1 (the Lead seat) exerts a gravitational influence on all other seats proportional to `osti_leadVoicePull`. High pull = the lead drummer keeps everyone in formation. Low pull = full democratic drift.

**Polyrhythm ratio:** `osti_polyrhythmRatio` selects the ratio between the slow and fast cycles — 3:4, 4:5, 5:7, 7:8, or free (user-defined). This creates the interlocking rhythmic fabric characteristic of West African and Indonesian ensemble playing.

### 2.6 Live Override

When you play MIDI notes, the circle does not stop. The pattern engine continues running. Your MIDI input is layered on top. When you stop playing, the circle fades back to its uninterrupted state over `osti_liveOverrideFade` milliseconds. You are a ninth player joining and leaving.

MIDI note mapping:
- Notes below C3: bass drum range → trigger the Bass seat instrument
- Notes C3–B3: hand drum range → trigger the Supporting seat instrument
- Notes C4 and above: lead range → trigger the Lead seat instrument
- Pitch content maps to articulation selection (root = fundamental strike, ±1 semitone = adjacent articulations)

---

## 3. Parameter List (~140 parameters, `osti_` prefix)

### 3.1 Global Parameters

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `osti_tempoSync` | 0/1 | 1 | Lock to DAW tempo or run free |
| `osti_freeTempo` | 40–240 BPM | 120 | Free-running tempo when not synced |
| `osti_polyrhythmRatio` | 0–4 (enum) | 2 | 3:4 / 4:5 / 5:7 / 7:8 / Free |
| `osti_polyrhythmFreeNumer` | 1–13 | 4 | Free numerator |
| `osti_polyrhythmFreeDenom` | 1–13 | 5 | Free denominator |
| `osti_cycleLength` | 0–3 (enum) | 1 | 8 / 12 / 16 / 24 steps |
| `osti_subdivision` | 0–3 (enum) | 2 | 1/8 / 1/8T / 1/16 / 1/16T |
| `osti_entrainmentRate` | 0–1 | 0.5 | How fast drifted seats lock back (0 = free drift, 1 = instant snap) |
| `osti_leadVoicePull` | 0–1 | 0.6 | How strongly seat 1 attracts others |
| `osti_grooveHumanize` | 0–1 | 0.3 | Global timing humanization depth |
| `osti_velocitySpread` | 0–1 | 0.4 | Dynamic range variation across hits |
| `osti_liveOverrideFade` | 100–2000 ms | 500 | How fast circle re-emerges after MIDI stops |
| `osti_circleVolume` | 0–1 | 0.8 | Overall circle level |
| `osti_liveVolume` | 0–1 | 1.0 | Your MIDI playing level |

### 3.2 Per-Seat Parameters (Seats 1–8, shown for seat N)

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `osti_seatN_instrument` | 0–11 (enum) | varies | Instrument selection (DJEMBE…BODHRAN) |
| `osti_seatN_pattern` | 0–7 (enum) | varies | Pattern P1–P8 |
| `osti_seatN_phaseOffset` | 0–1 | varies | Position in global cycle (0 = aligned with seat 1) |
| `osti_seatN_active` | 0/1 | 1 | Seat on/off (a chair is empty) |
| `osti_seatN_volume` | 0–1 | 0.7 | Seat level |
| `osti_seatN_pan` | -1–1 | auto | Stereo position (auto follows circle geometry) |
| `osti_seatN_density` | 0–1 | 0.6 | Pattern fill — 0 thins hits, 1 fills all grid positions |
| `osti_seatN_velocitySensitivity` | 0–1 | 0.7 | How much pattern accent map affects velocity |
| `osti_seatN_humanize` | 0–1 | 0.3 | Per-seat timing humanization |
| `osti_seatN_couplingReactivity` | 0–1 | 0.5 | How strongly this seat responds to others' accents |
| `osti_seatN_articulationBias` | 0–1 | 0.5 | Leans toward softer (0) or harder (1) articulations |

### 3.3 Per-Instrument Synthesis Parameters

Each instrument has synthesis parameters controlling its physical model. Shown for the active instrument in any seat:

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `osti_synth_tuning` | 0.25–4× | 1.0 | Fundamental frequency scalar (1.0 = true pitch) |
| `osti_synth_membraneDecay` | 10–2000 ms | varies | Membrane vibration decay time |
| `osti_synth_noiseMix` | 0–1 | varies | Attack noise (transient click) vs. tone balance |
| `osti_synth_bodyResonance` | 0–1 | varies | Waveguide cavity resonance depth |
| `osti_synth_bodySize` | 0.5–2× | 1.0 | Physical body size scalar (affects pitch and decay) |
| `osti_synth_skinTension` | 0–1 | varies | Membrane tension (higher = brighter, shorter) |
| `osti_synth_strikePosition` | 0–1 | varies | Membrane strike location (center = fundamental, edge = overtones) |
| `osti_synth_radiationAngle` | 0–1 | varies | Radiation pattern (omnidirectional → directional) |

### 3.4 FX Chain Parameters

| Parameter ID | Range | Default | Description |
|-------------|-------|---------|-------------|
| `osti_fxSpatialRadius` | 0–1 | 0.7 | Circle Spatial Engine — how wide the circle sits in stereo field |
| `osti_fxFireStage` | 0–1 | 0.5 | Fire Stage distortion / warming on the circle bus |
| `osti_fxGatherReverbMix` | 0–1 | 0.4 | Gathering Reverb send amount |
| `osti_fxGatherReverbSize` | 0–1 | 0.6 | Room size — intimate fire pit to outdoor ceremony ground |
| `osti_fxPulseCompThreshold` | -40–0 dB | -12 | Pulse Compressor threshold |
| `osti_fxPulseCompRatio` | 1–20 | 4 | Pulse Compressor ratio |

### 3.5 Macro Parameters (M1–M4)

| Macro | ID | Label | Controls | Behavior |
|-------|-----|-------|----------|----------|
| M1 | `osti_macroGather` | GATHER | `osti_entrainmentRate`, `osti_leadVoicePull`, all `osti_seatN_couplingReactivity` | 0 = everyone in their own world. 1 = perfectly locked communal grid. The unity dial. |
| M2 | `osti_macroDensity` | DENSITY | all `osti_seatN_density`, `osti_grooveHumanize` (inverse), `osti_velocitySpread` | 0 = sparse, spacious, meditative. 1 = full-grain, every slot filled, overwhelming presence. The fill dial. |
| M3 | `osti_macroCeremony` | CEREMONY | `osti_fxFireStage`, `osti_fxGatherReverbMix`, `osti_fxGatherReverbSize`, all `osti_seatN_articulationBias` | 0 = casual jam, dry, close. 1 = full ritual — the fire is high, the sound carries, the ceremony has begun. |
| M4 | `osti_macroChaos` | CHAOS | all `osti_seatN_phaseOffset` (randomize), `osti_entrainmentRate` (inverse), all `osti_seatN_humanize` | 0 = perfect order. 1 = deliberate desynchronization — the circle is falling apart in the most beautiful way. |

---

## 4. DSP Approach

### 4.1 Modal Membrane Synthesis

Each drum voice uses a 2D modal membrane model. A circular membrane has eigenfrequency modes at ratios derived from the zeros of Bessel functions: 1.00 : 1.59 : 2.14 : 2.30 : 2.65 : 2.92 : 3.16...

Practical implementation uses 6–12 parallel resonator banks (second-order IIR filters) tuned to these ratios, each with independent decay time and initial amplitude. The relative amplitudes of modes determine the characteristic sound of the instrument — a djembe bass tone has strong fundamental and low modes; a slap has boosted upper modes.

Strike position shifts the mode amplitude distribution: center strikes excite the fundamental heavily; edge strikes push energy into overtones.

**IIR coefficients use matched-Z transform** (`exp(-2*PI*fc/sr)`) — never the Euler approximation. This is critical for correct decay behavior at 44.1kHz and 48kHz sample rates.

### 4.2 Waveguide Body Cavity

A cylindrical waveguide models the drum body. Length determines the cavity resonance frequencies. Goblet drums (djembe, doumbek, udu) use a tapered model that shifts the lowest resonance upward. The cavity adds body and presence — it is the difference between a drum head suspended in space and a drum you can feel in your chest.

Implementation: all-pass-filtered delay lines in a feedback loop. Feedback gain controls the cavity resonance depth (`osti_synth_bodyResonance`). Denormal protection required — guard with `std::abs(x) < 1e-15f ? 0.f : x` before each feedback accumulation.

### 4.3 Stochastic Humanization

Timing humanization is drawn from ethnomusicological timing data, not pure Gaussian noise. Real drummers in real circles have characteristic "feel curves" — systematic early/late tendencies that define musical groove.

The engine ships with three humanization models:
- **Pocket** — slight late tendency on beat 2 and 4 (funk/hip-hop feel)
- **Laid Back** — uniform late tendency, relaxed
- **On Top** — slight early tendency, urgent, forward

These are blendable distributions parameterized by `osti_grooveHumanize`. At 0, all hits are grid-perfect. At higher values, draws from the selected distribution increase. The distributions are stored as look-up tables (512 entries) with linear interpolation.

### 4.4 Inter-Seat Communication

The coupling reactivity system (`osti_seatN_couplingReactivity`) listens to accent events from all other active seats. When any seat fires a strong accent (velocity above a threshold), it broadcasts an accent event to the circle bus. Each other seat reads this bus and has a probability of inserting a reactive fill or accent on its next available hit.

This is what makes OSTINATO feel alive: the doumbek player hears the djembe's accent and responds. The response is stochastic — not guaranteed — which preserves the feel of real human musicians rather than mechanical reaction.

**Reaction delay:** responses are delayed by 8–24 ms (random per event) — a physical reaction time that matches human response latency and prevents the circle from feeling mechanical.

### 4.5 Pattern Phase Entrainment

Phase entrainment uses a phase-locked loop model. Each seat has a phase accumulator advancing at the global tempo. Phase offset is the current difference from seat 1. The entrainment system applies a correction velocity toward zero offset each step, proportional to `osti_entrainmentRate`.

```
correction = phaseError * entrainmentRate * dt
phaseOffset += correction
```

When `osti_entrainmentRate` is 0, correction is 0 — pure free drift. When 1, correction is maximum — fast locking. The `osti_leadVoicePull` parameter scales the phase error signal from seat 1 before it propagates to other seats.

---

## 5. feliX/Oscar Polarity

OSTINATO is the most explicitly communal engine in the XOmnibus gallery. Its feliX/Oscar axis maps directly onto two musical philosophies of percussion:

**feliX pole — tight, locked, mathematical polyrhythm.** Patterns interlocked at perfect phase relationships. Ewe bell patterns where the timeline governs everything. Steve Reich's phase music at its most rigorous. The GATHER macro at maximum. Every player in formation.

**Oscar pole — loose, human, organic temporal drift.** Musicians listening to each other, drifting, catching each other, losing each other again. The CHAOS macro at full. Nothing is locked — everything is feeling. The fire is burning hot and the circle is moving.

Both poles are musically valid. The GATHER and CHAOS macros are designed to move fluidly between them, and both extremes should produce compelling, usable sound.

**The deepest point of Oscar expression in OSTINATO** is the deliberate desynchronization of CHAOS M4 — where all phase offsets randomize and entrainment is suppressed. This sounds like the end of a long ceremony when the form has dissolved and everyone is playing for pure joy.

---

## 6. Coupling Potential

### 6.1 Signature Coupling Routes

| Route | Type | Musical Effect |
|-------|------|---------------|
| OSTINATO → OVERDUB | `AmpToFilter` + `getSample` | World percussion through dub echo delay. The drum circle from another time zone. |
| OSTINATO → OPAL | `AudioToWavetable` | Drum transients granulated into scattered light particles. Physical → spectral. |
| ONSET ↔ OSTINATO | Bidirectional `AmpToFilter` | Machine meets human — the drum machine and the drum circle trade accents. |
| OSTINATO → ORACLE | `EnvToMorph` | Stochastic timing drives Oracle's generative voice — the circle speaks prophecy. |
| OSTINATO → OUROBOROS | `AmpToFilter` | Percussion accent triggers chaos engine — the circle calls the storm. |

### 6.2 OSTINATO as Coupling Source

OSTINATO can send accent events as coupling signals. This is a unique capability: the inter-seat communication system broadcasts accent events that can be routed to other engines via the MegaCouplingMatrix. A djembe accent in seat 1 can trigger a filter sweep in OVERDUB, a granular burst in OPAL, or a chaos event in OUROBOROS.

Coupling output signal: amplitude-normalized accent gate (0/1, gated per hit), available per seat or as circle-wide mixed output.

### 6.3 OSTINATO as Coupling Target

| Coupling Type | What OSTINATO Does |
|--------------|-------------------|
| `AmpToFilter` | External amplitude drives `osti_fxFireStage` warmth |
| `EnvToMorph` | External envelope drives `osti_macroDensity` |
| `LFOToPitch` | External LFO drives `osti_synth_tuning` on selected seats |
| `AmpToVoice` | External amplitude triggers the live override voice |

### 6.4 Coupling Types OSTINATO Should NOT Receive

- `AmpToChoke` — the circle does not choke. Voices complete.
- `AudioToFM` — FM of a modal resonator creates artifacts. Not musically useful.

---

## 7. XPN Export Strategy

OSTINATO is a pattern-generating engine. Exporting to XPN (MPC format) requires a different approach than a conventional synthesis engine.

### 7.1 The Problem

An MPC XPN program expects discrete samples mapped to pads. OSTINATO synthesizes patterns in real time — there are no pre-rendered samples. The export must bridge this gap.

### 7.2 The Solution: Pattern Render Export

When the user invokes XPN export, the export pipeline:

1. **Renders each seat's pattern individually** — 1 bar of each of the 8 patterns × 8 seats = up to 64 stems. Each stem is a dry, isolated render of that seat's pattern using the current synthesis parameters.

2. **Exports per-articulation one-shots** — additionally renders single-hit samples for each articulation of the active instrument in each seat (4 articulations × 8 seats = up to 32 one-shot samples). These map to individual pads for live re-performance on the MPC.

3. **Packages both sets** — the one-shots go into a Keygroup program (pitched, playable). The pattern stems go into a Drum program (each pad = one bar of one seat's current pattern).

4. **Metadata preservation** — the XPN package includes a human-readable manifest listing the instrument, pattern slot, and synthesis parameters used for each render, so the user can recreate the patch on re-import.

### 7.3 XPN Pad Bank Layout

**Bank A (pads 1–16):** One-shot articulations for seats 1–4 (4 pads per seat)
**Bank B (pads 1–16):** One-shot articulations for seats 5–8 (4 pads per seat)
**Bank C (pads 1–16):** Pattern stems — one pad per seat (8 seats × current pattern = 8 stems), plus 4 full-mix renders at different density levels, plus 4 coupling showcase renders

**Velocity layers:** Each one-shot exported at 3 velocity layers (soft/medium/hard) using MPC velocity cycling.

---

## 8. Historical Homage

### ARP 2500 Step Sequencer (1970)

The ARP 2500's Model 1027 step sequencer was among the first instruments to demonstrate that repeating voltage-controlled patterns were expressive instruments in their own right — not just clock divisions. Patterns could be voltage-controlled, gated, skipped, and modulated in ways that produced musical complexity from simple rules. OSTINATO's pattern system honors this: patterns are not just playback grids. They are living parameters.

### Ewe Agbadza and West African Bell Patterns

The gankogui bell timeline of Ewe ceremonial music is one of the most studied examples of asymmetric rhythm in ethnomusicology — a 12-step pattern that functions as a "time line," anchoring all other parts while no single hit coincides with a simple metric accent. The Agbadza is a community ceremony: it is not performed for an audience, it is performed by the community for the community. Every person present is a participant. OSTINATO's circle model — no fixed roles, any seat open, the circle carries on when you leave — is a direct translation of this social architecture into synthesis.

### Steve Reich — Phasing Studies (1965–1972)

"It's Gonna Rain" (1965), "Piano Phase" (1967), "Drumming" (1971), "Clapping Music" (1972). Reich demonstrated that phased polyrhythm — identical patterns drifting apart and realigning — produces emergent musical phenomena more complex than either pattern alone. He called this process music. OSTINATO's entrainment and phase offset system is Reich's insight made interactive: you control the rate of drift and the pull toward resolution.

---

## 9. UI Concept

**Circle View** — the primary view is a top-down view of 8 seats arranged in a circle. A central fire (animated, warm orange) sits in the middle. Each seat shows the active instrument icon, current pattern slot, and a real-time activity indicator (brightens on hits, dims during rest).

Selecting a seat opens a panel showing its synthesis parameters, pattern editor, and coupling reactivity control. The global parameters (tempo sync, polyrhythm ratio, macro sliders) live in a strip at the bottom.

The fire at the center is not decorative — it pulses to the global accent, grows with the CEREMONY macro, and flickers with the CHAOS macro. It is the visual center of gravity of the circle.

**Color palette:** Warm charcoal brown `#2C2216`, firelight orange `#E8701A`, cream text `#F5EFE0`. Nighttime around a fire. The only XOmnibus engine with a dark-dominant UI panel — appropriate for the ceremony.

---

## 10. Preset Strategy

**150 factory presets** across 7 categories (fleet standard):

| Category | Count | Character |
|----------|-------|-----------|
| One Fire | 25 | Foundational circles — 2–3 active seats, clean, spacious |
| The Gathering | 25 | Full 8-seat arrangements, all instruments locking |
| Ceremony | 25 | High CEREMONY macro, spatial, reverberant, ritualistic |
| Machine Meets Human | 20 | Coupling showcases with ONSET — mechanical + organic |
| World Drift | 20 | High CHAOS macro, polyrhythmic, drifting |
| Live Playground | 20 | Designed for live MIDI layering over the circle |
| Fire Shots | 15 | Maximum energy, maximum density, maximum ceremony |

### Naming Convention
Names should feel communal, warm, physical:
- "First Fire"
- "Agbadza Call"
- "Eight Players"
- "The Circle Holds"
- "Reich Study No. 1"
- "Taiko and Tabla"
- "Ceremony Ground"
- "Everyone Is Welcome"

---

*XO_OX Designs | Engine: OSTINATO | Accent: #E8701A | Prefix: osti_ | V1 scope | Phase 1 R&D*
