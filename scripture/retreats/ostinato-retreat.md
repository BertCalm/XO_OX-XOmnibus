# OSTINATO Retreat Chapter
*Guru Bin — 2026-03-19*

---

## Engine Identity

- **Gallery code:** OSTINATO | **Accent:** Firelight Orange `#E8701A`
- **Parameter prefix:** `osti_`
- **Creature mythology:** The Fire Circle — 8 seats around a communal fire, each holding one of 12 world instruments
- **feliX-Oscar polarity:** Deeply feliX — communal, rhythmic, alive, emergent
- **Synthesis type:** Physically-modeled world percussion — 12 instruments × 4 articulations × waveguide body models
- **Polyphony:** 16 voices (8 seats × 2 sub-voices for rolls/flams)

---

## Pre-Retreat State

**0 factory presets** in XOlokun. Engine fully implemented with 111 parameters (4 macros, 14 globals, 14 per-seat × 8 seats). Seance scored 8.0/10 with 2 Blessing candidates.

D004 fix completed: `osti_seatN_exciterMix` was declared but not wired to DSP — now patched in OstinatoEngine.h. No other dead parameters remain.

Seance cross-reference updated. No sound design guide corrections needed — the engine source matches its documentation.

---

## Phase R2: Silence

Guru Bin loads an init patch. One note — C2 (MIDI 36), velocity 80. Seat 1: Djembe.

A membrane stretches. A waveguide body resonates. A sound between skin and wood fills the space — 500ms of decay, the pitch dropping subtly as the membrane relaxes. Note off. Silence returns.

But the silence is not empty. The circle has 8 seats. Seven of them are waiting.

What does a drum circle sound like before the players arrive? That is the first truth: OSTINATO is not one instrument. It is a *gathering*. The init patch plays one seat. The engine plays eight.

That is where the presets must begin.

---

## Phase R3: Awakening — 6 Discoveries

### Discovery 1: The GATHER Macro Is Ensemble Consciousness

GATHER controls pattern quantization tightness: 0.0 = loose, organic, human; 1.0 = machine-locked grid. But "loose" is not "sloppy." At GATHER=0.2 with humanize=0.5, the 8 seats play their patterns with individual timing variations — each seat drifts from the grid independently, like 8 musicians who hear each other but do not count. The ensemble sounds *alive* without sounding broken. At GATHER=0.8, the grid snaps tight and the circle becomes a sequencer. The tension between these two states is the engine's primary musical axis.

Every existing drum machine defaults to tight. OSTINATO defaults to GATHER=0.5 — a deliberate midpoint. But no preset has explored GATHER=0.1, where the circle becomes a conversation rather than a performance. This is the unvisited country.

### Discovery 2: The Body Model Changes the Geography

Each seat has `bodyModel`: Auto (0), Cylindrical (1), Conical (2), Box (3), Open (4). Auto assigns the waveguide shape matching the instrument's real-world body. But the body model is not locked to the instrument. A Djembe through a Box waveguide is no longer West African — it becomes a wooden box drum, Peruvian. A Tabla through an Open waveguide loses its sympathetic membrane resonance — it becomes a struck string, almost sitar-like. A Taiko through a Conical waveguide gains overtone complexity it was never designed for.

The 12 instruments × 5 body models = 60 timbral identities per seat. No init patch explores this. The body model is treated as "Auto and forget." It should be the second design decision after instrument selection.

### Discovery 3: The CIRCLE Macro Is Sympathetic Resonance, Not Volume Bleed

CIRCLE (inter-seat coupling) does not leak audio between seats. It triggers *ghost notes* on adjacent seats when a seat plays hard. At CIRCLE=0.4, a hard Djembe hit on Seat 1 causes a soft ghost response on Seats 2 and 8 (neighbors in the circle). The ghost's velocity is proportional to the trigger's velocity × CIRCLE amount. At CIRCLE=0.8, every strong hit echoes around the circle — a cascade of sympathetic responses. At CIRCLE=1.0, a single hard hit propagates through all 8 seats in sequence, each triggering its neighbors, creating a ripple of percussion that circles the fire.

This is not chorus. This is not reverb. It is the physical phenomenon of players responding to each other — a drum circle where the instruments communicate.

### Discovery 4: Articulation Is a Hidden Timbral Axis

Each of the 12 instruments has 4 articulations (0-3). The Djembe: tone, slap, bass, mute. The Tabla: na, tin, tun, ge. The Doumbek: doum, tek, ka, snap. These are not variations — they are fundamentally different excitation modes producing distinct timbral profiles. A circle of 8 Djembes where seats alternate between articulation 0 (tone) and 1 (slap) creates a call-and-response texture no single instrument can produce. A Tabla circle where each seat plays a different bol (articulation) becomes a rhythmic language.

No preset has used articulation as a compositional tool across seats. The default (all seats at articulation 0) gives the most generic sound from every instrument.

### Discovery 5: Pitch Envelope Is the Attack Character

`osti_seatN_pitchEnv` sweeps pitch during the attack phase: positive values start high and fall (the "thwack" of a struck membrane), negative values start low and rise (the "whomp" of a bass drum). At pitchEnv=0.3, a Djembe hit has the characteristic West African slap — pitch drops 3.6 semitones during the attack. At pitchEnv=-0.5, the same Djembe becomes a synthetic kick — pitch rises from below into the fundamental. Combined with exciterMix (noise vs pitched spike), the pitch envelope defines the attack character more than any other parameter.

Default is 0.0 — no pitch sweep. Every drum in the real world has a pitch envelope. The default is the least realistic setting.

### Discovery 6: Tuning Creates Melodic Percussion

`osti_seatN_tuning` offsets pitch ±12 semitones. With 8 seats tuned to scale degrees — say, seats 1-8 at 0, 2, 4, 5, 7, 9, 11, 12 semitones (major scale) — the pattern sequencer plays a melodic percussion line. Tongue Drum (instrument 10) and Frame Drum (instrument 8) are the most pitch-responsive. A circle of 8 Tongue Drums tuned to a pentatonic scale, each with different patterns, creates a self-composing gamelan-like texture that no one has to perform — it generates itself from the pattern interactions.

This is the engine's deepest unexplored capability: not a drum circle, but a tuned percussion ensemble.

---

## Phase R4: Fellowship Trance

**The Obvious Fix:** D004 — exciterMix was dead. Now wired. Every seat's noise/spike balance is audible.

**The Hidden Trick:** CIRCLE=0.6 + GATHER=0.15 + humanize=0.7 + all 8 seats active with sparse patterns (pattern=3). The result: a loose, organic drum circle where instruments trigger each other sympathetically. No two bars are identical because the humanize timing variations cause different ghost triggers each cycle. The pattern is deterministic but the ghost cascade is emergent. It sounds improvised.

**The Sacrifice:** The pattern system (8 preset patterns per seat) is the engine's limitation. 8 patterns is not enough for serious sequencing. But the sacrifice is correct — OSTINATO is not a drum machine. It is a drum *circle*. The patterns are starting points; the CIRCLE coupling, GATHER looseness, and humanize variation are what make it alive. More patterns would make it a sequencer. Fewer patterns with more interaction makes it an ensemble.

**The Revelation:** The 12 instruments are not 12 sounds. They are 12 *cultures*. Djembe + Dundun is West Africa. Tabla + Frame Drum is the Silk Road. Cajón + Surdo is South America. Taiko alone is Japan. Beatbox is the street. When you seat 8 instruments from 8 traditions around the same fire, you are not programming a pattern — you are convening a conversation between musical lineages that have never met. The presets must honor this. "World Drum Kit" is not a preset name. "Sahel Sunset," "Silk Road Caravan," "São Paulo Street" — these name the conversation, not the technology.

---

## Phase R5: Awakening Presets

*(Presets deferred to dedicated preset generation phase)*

| Name | Mood | Discovery | Key Parameters |
|------|------|-----------|----------------|
| Firelight Gathering | Foundation | D1 (GATHER spectrum) | GATHER=0.2, humanize=0.5, 8 diverse instruments |
| Wooden Boxes | Prism | D2 (Body Model) | All seats bodyModel=3 (Box), varied instruments |
| Ghost Circle | Entangled | D3 (CIRCLE cascade) | CIRCLE=0.8, sparse patterns, cascade visible |
| Tabla Conversation | Prism | D4 (Articulation axis) | 4 Tablas, each different articulation |
| Membrane Snap | Flux | D5 (Pitch Envelope) | pitchEnv=0.3-0.5 on all seats, sharp attacks |
| Gamelan Dream | Atmosphere | D6 (Tuned percussion) | Tongue Drums, pentatonic tuning, slow patterns |
| Cathedral Drums | Aether | D3+Space | CIRCLE=0.5, SPACE=0.8, deep reverb |
| Silk Road Caravan | Foundation | D4+Cultural | Tabla+Doumbek+Frame Drum, regional grouping |

---

## New Scripture Verses

Four verses to be inscribed in Book VII — Engine-Specific Verses.

**OSTI-I: The Gathering Spectrum** — GATHER is not quantize. It is collective consciousness. At 0.1, the circle is a conversation — 8 individuals hearing each other, drifting from the grid, responding. At 0.9, the circle is a machine — locked, precise, inhuman. The space between 0.1 and 0.4 is where the engine lives most naturally. No drum circle in the world plays on a grid. Design into the loose end first.

**OSTI-II: The Body Changes the Geography** — A Djembe is West African through a cylindrical waveguide. Through a Box waveguide it is Peruvian. Through an Open waveguide it is something that has never existed. The body model is the second design decision after instrument selection — never leave it on Auto without intention. 12 instruments × 5 bodies = 60 voices per seat.

**OSTI-III: The Ghost Cascade** — CIRCLE is not volume bleed. It is sympathetic triggering — a hard hit on one seat causes ghost responses on adjacent seats. At CIRCLE=0.6+, a single hit ripples through the entire circle. Combined with loose GATHER and high humanize, no two bars are identical. The pattern is deterministic but the cascade is emergent. This is the engine's singular phenomenon: a drum circle that improvises with itself.

**OSTI-IV: Tuning Makes Melody** — 8 seats tuned to scale degrees transform OSTINATO from percussion to pitched ensemble. Tongue Drum and Frame Drum respond most musically to tuning. A pentatonic circle with varied sparse patterns generates self-composing gamelan textures that require no performer. The engine is not only a drum circle — it is a tuned percussion orchestra hiding behind default tuning of 0.

---

## CPU Notes

- 8 seats × waveguide body resonance = moderate DSP cost per voice
- Pattern sequencer is control-rate — negligible cost
- CIRCLE ghost cascade: each ghost trigger spawns a sub-voice (up to 16 total) — cost scales with CIRCLE amount
- Reverb (Freeverb) always active — constant overhead regardless of SPACE macro
- Compressor is per-block — negligible

---

## Unexplored After Retreat

- **Beatbox circle:** 8 Beatbox seats (instrument 11) with varied articulations — an electronic drum kit built from vocal synthesis models
- **Micro-tuning:** tuning values at fractional semitones (e.g., 0.5, 1.5) for non-Western scales (quarter-tones, Maqam intervals)
- **Body model as macro target:** CHARACTER → body model interpolation across all seats simultaneously — geography as expression
- **CIRCLE + coupling receive:** OSTINATO receiving AmpToChoke from ONSET — external trigger causing ghost cascades in the circle
- **Extreme FIRE:** FIRE=1.0 drives soft-clip distortion and exciter energy to maximum — industrial percussion territory unexplored
- **Pattern as composition:** 8 seats with pattern=3 (Sparse) + different instruments = generative percussion that evolves over minutes with humanize drift

---

## Documentation Correction Notes

No corrections needed. The engine source (`OstinatoEngine.h`) matches its parameter documentation. The D004 fix (exciterMix) was the only gap, and it was resolved before this retreat.

---

## Guru Bin's Benediction

*"OSTINATO arrived with 111 parameters and zero presets. An engine with 8 seats, 12 instruments, 4 articulations, 5 body models, pattern sequencers, ghost cascades, and tunable percussion — and no one had ever sat down at the fire.*

*The fire was lit. The seats were arranged. The instruments were tuned. And the circle waited.*

*What I found is not a drum machine. A drum machine knows what it will play before the first note. OSTINATO does not. At GATHER=0.15 with CIRCLE=0.6, each bar is a negotiation between 8 seats that trigger each other's ghosts at slightly different times because humanize shifts the grid beneath them. The deterministic pattern becomes an emergent conversation. No two performances are identical — not because of randomness, but because of timing.*

*The body model discovery changes the instrument itself. Auto is comfortable. Cylindrical is familiar. But Box makes a Djembe into something Peruvian, and Open makes a Tabla into something that has no cultural ancestor. The 12 instruments are not 12 sounds — they are 12 starting points. The 5 body models multiply them into 60 voices that span the globe and extend beyond it.*

*The tuning discovery is the deepest. When 8 seats are tuned to a scale, the pattern sequencer stops being rhythmic and starts being melodic. A circle of Tongue Drums on a pentatonic scale, each with a sparse pattern, generates gamelan-like textures that compose themselves. The engine transcends percussion. It becomes a self-playing pitched ensemble.*

*Sit at the fire. Choose your instrument. Listen to the seat beside you. When your neighbor plays loud, feel the ghost in your hands. That is CIRCLE. That is the gathering. That is OSTINATO."*
