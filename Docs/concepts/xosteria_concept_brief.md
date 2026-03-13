# XOsteria — Concept Brief

**Date:** March 2026
**Phase:** 0 — Ideation
**Status:** Draft
**Companion engine:** XOsprey (the ocean). XOsteria is the shore.

---

## Identity

- **Name:** XOsteria
- **Gallery code:** OSTERIA
- **Thesis:** XOsteria is an ensemble-synthesis engine where a jazz quartet absorbs the folk character of coastal cultures, stretches across shores through temporal space, and gathers in seaside taverns where music, conversation, and merriment fill the room. The human answer to the ocean's inhuman vastness.
- **Sound family:** Ensemble / Pad / Lead / Texture / Hybrid
- **Unique capability:** Four independent voice channels (bass, harmony, melody, rhythm) each with their own position on a continuous shore axis, connected by elastic rubber-band coupling. Voices absorb the local folk instrument character of whatever coastline they're on. When voices stretch apart across shores, the harmonic tension of being in two places at once becomes audible. Cross-pollination memory means borrowed influences persist — the quartet accumulates a living history of everywhere it's been. No other engine models an ensemble that travels through timbral space.

---

## Character

- **Personality in 3 words:** Communal, Nomadic, Elastic
- **Engine approach:** Multi-voice resonator ensemble with per-voice shore morphing, elastic inter-voice coupling, timbral memory, and environmental room modeling
- **Why this approach serves the character:**
  Music is communal. A lone oscillator can be beautiful, but four voices listening and responding to each other — absorbing local color, stretching apart, snapping back — that's how human music actually works. A jazz quartet on tour doesn't play the same in Lisbon as in Oslo. The room changes them. The local music seeps in. They carry it to the next city. XOsteria models this process as synthesis: the quartet IS the oscillator bank, the shore IS the timbre selector, the rubber band IS the modulation, and the memory IS the waveshaper.
- **The coupling thesis:**
  XOsteria's natural partner is XOsprey. OSPREY provides the ocean — turbulence, creature voices, fluid dynamics. OSTERIA provides the human response — the music made at the water's edge. Coupled together, they create a complete world: the storm outside the tavern window, the whale call beneath the quartet's melody, the swell rhythm driving the bodhran pattern. But OSTERIA couples beautifully with any engine — its quartet voices can absorb timbral character from coupling inputs, treating any engine's audio as "the local music" of an imaginary shore.

---

## The XO Concept Test

1. **XO word:** XOsteria — an Italian tavern/inn, universally understood as a warm gathering place. The osteria is where the fisherman goes after the sea. Where stories become songs. Where strangers become an ensemble.
2. **One-sentence thesis:** "XOsteria is an ensemble-synthesis engine where a jazz quartet stretches across coastal cultures, absorbing folk instrument character through elastic rubber-band coupling, gathering in seaside taverns that change the sound of everything inside them."
3. **Sound only this can make:** A bass voice playing with Atlantic guitarra weight while the melody voice carries Mediterranean ney breathiness — the harmonic tension of being on two coastlines simultaneously, with oud warmth bleeding into the bass from a shore it visited three notes ago. No combination of layered patches, detuned oscillators, or world-music sample packs can replicate this because the timbral morphing is continuous, the elastic coupling is dynamic, and the memory is cumulative.

---

## Gallery Gap Filled

| Existing Coverage | What It Does |
|-------------------|-------------|
| OSPREY | Environmental dynamics — the ocean as instrument |
| OBESE | 13-oscillator unison stacking — width through copies |
| OVERWORLD | Multi-engine chip synthesis — 6 retro engines |
| ORBITAL | Additive partial manipulation — psychoacoustic |
| **OSTERIA** | **Ensemble synthesis — independent voices with elastic coupling and timbral memory** |

The gallery has stacking (OBESE), multi-engine switching (OVERWORLD), and partial control (ORBITAL) — but no engine where **multiple independent voices interact as an ensemble**, each with its own timbral identity and position in a continuous space. OBESE stacks copies of one sound. OSTERIA layers four *different* voices that influence each other.

**Key distinction from OBESE:** OBESE creates width through 13 copies of one oscillator. OSTERIA creates richness through 4 independent voices that absorb different timbral characters and interact through elastic coupling.

**Key distinction from OVERWORLD:** OVERWORLD switches between 6 chip engines (one at a time). OSTERIA runs 4 voices simultaneously, each potentially on a different shore, blending in real-time.

---

## Core DSP Architecture (Phase 0 Sketch)

```
Shore System (shared conceptual model with OSPREY)
├── Atlantic (0.0): guitarra, kora, uilleann, bodhran
├── Nordic (1.0): hardingfele, langspil, kulning, Sámi drum
├── Mediterranean (2.0): bouzouki, oud, ney, darbuka
├── Pacific (3.0): koto, shakuhachi, conch, taiko
└── Southern (4.0): cavaquinho, valiha, gamelan, djembe
        │
        ▼
The Quartet (4 independent voice channels)
├── BASS voice ─── osteria_qBassShore ──┐
├── HARMONY voice ─ osteria_qHarmShore ─┤
├── MELODY voice ── osteria_qMelShore ──┼── Elastic Coupling Engine
└── RHYTHM voice ── osteria_qRhythmShore┘   ├── Centroid calculation
Each voice contains:                         ├── Spring force per voice
├── Resonator bank (shore-selected partials) ├── Stretch limit
├── Formant model (instrument character)     ├── Snap-back dynamics
├── Timbral memory buffer (accumulated       └── Memory blend
│   shore influences from past positions)
├── Per-voice filter + envelope
└── Per-voice pan position (stereo field)
        │
        ▼
Ensemble Mix
├── Voice balance (bass/harmony/melody/rhythm levels)
├── Ensemble width (tight center → wide stereo spread)
├── Inter-voice sympathy (how much voices excite each other's resonators)
└── Blend mode: jazz (voices complement) / unison (voices converge)
        │
        ▼
Tavern Room Model
├── Room selection: per-shore tavern character
│   ├── Atlantic: stone walls, low ceiling, wood bar, fireplace
│   ├── Nordic: deep timber paneling, heavy insulation, warm hearth
│   ├── Mediterranean: open-air terrace, tile floor, sea breeze
│   ├── Pacific: paper screens, tatami absorption, garden beyond
│   └── Southern: corrugated roof, open sides, tropical air
├── Human murmur: broadband texture with formant clusters
│   (language cadence, laughter character, singing style per shore)
├── Micro-transients: glass clinks, chair scrapes, fire crackle, rain
├── Warmth filter: proximity EQ (nearfield intimacy)
└── Ocean bleed: how much of the outside world seeps in
        │
        ▼
Character Stage
├── Patina: subtle harmonic aging (old wood, worn strings)
├── Porto: warm saturation (wine-dark overdrive)
└── Smoke: gentle HF haze (woodfire atmosphere)
        │
        ▼
FX Chain
├── Session Delay: conversational echo (short, warm, rhythmic)
├── Hall: from intimate pub corner to cathedral harbor wall
├── Chorus: paired-string shimmer (guitarra-inspired beating)
└── Tape: lo-fi warmth (field recording character)
        │
        ▼
Output (stereo)
```

**Voice model:** MIDI input drives all four quartet voices simultaneously — each voice interprets the same MIDI through its current shore's instrument character. The bass voice naturally occupies the low register, melody the high, harmony the mid, rhythm produces percussive articulations. Up to 8 notes of polyphony (each note activates all 4 quartet voices). Legato mode: voices slide between notes with per-shore portamento character.

**Elastic coupling model:** Each voice has a position on the shore axis (0.0–4.0). A spring-force model calculates pull toward the ensemble centroid. The spring constant is `osteria_qElastic`. When stretch exceeds `osteria_qStretch`, the force increases nonlinearly — the rubber band tightens. Voices don't snap instantly; they have inertia and overshoot, creating natural-feeling ensemble dynamics.

**Timbral memory model:** Each voice maintains a circular buffer of recent shore positions (last N seconds). `osteria_qMemory` controls how much the accumulated history influences the current resonator coefficients. At memory=0, only the current shore matters. At memory=1, the voice is a palimpsest — every shore it's visited leaves a permanent trace in its timbre. The memory is per-voice, creating the effect of four musicians with different travel histories playing together.

**CPU strategy:** 4 resonator voice channels × 8 polyphony = 32 resonator instances. Each is a lightweight modal filter bank (~4 formants per voice). Shore morphing uses interpolated coefficient sets. The tavern room model is a small FDN reverb + noise texture. Elastic coupling is trivial math (spring forces). Estimated <12% CPU single-engine at 44.1kHz / 512 block on M1.

---

## Parameter Namespace

All parameter IDs use `osteria_` prefix. Key parameters:

| ID | Range | Description |
|----|-------|-------------|
| | | **— The Quartet —** |
| `osteria_qBassShore` | 0.0-4.0 | Bass voice's shore position (Atlantic→Southern) |
| `osteria_qHarmShore` | 0.0-4.0 | Harmony voice's shore position |
| `osteria_qMelShore` | 0.0-4.0 | Melody voice's shore position |
| `osteria_qRhythmShore` | 0.0-4.0 | Rhythm voice's shore position |
| `osteria_qElastic` | 0.0-1.0 | Rubber band strength. 0=voices drift freely. 1=tight ensemble, voices snap together. |
| `osteria_qStretch` | 0.0-1.0 | Max shore distance before elastic pull increases. The tension threshold. |
| `osteria_qMemory` | 0.0-1.0 | Cross-pollination persistence. 0=clean voice. 1=permanently stained by every shore visited. |
| `osteria_qSympathy` | 0.0-1.0 | Inter-voice sympathetic resonance. How much voices excite each other. |
| | | **— Voice Balance —** |
| `osteria_bassLevel` | 0.0-1.0 | Bass voice level |
| `osteria_harmLevel` | 0.0-1.0 | Harmony voice level |
| `osteria_melLevel` | 0.0-1.0 | Melody voice level |
| `osteria_rhythmLevel` | 0.0-1.0 | Rhythm voice level |
| `osteria_ensWidth` | 0.0-1.0 | Ensemble stereo spread (0=mono center, 1=wide field) |
| `osteria_blendMode` | 0.0-1.0 | Jazz (0, voices complement) → Unison (1, voices converge on same character) |
| | | **— Tavern —** |
| `osteria_tavernMix` | 0.0-1.0 | Open air (0) → deep inside the pub (1). Room model intensity. |
| `osteria_tavernShore` | 0.0-4.0 | Which shore's tavern we're in. Can differ from quartet shore positions — the room doesn't move with the musicians. |
| `osteria_murmur` | 0.0-1.0 | Human conversation/laughter texture level |
| `osteria_warmth` | 0.0-1.0 | Proximity EQ — nearfield intimacy, wood absorption |
| `osteria_oceanBleed` | 0.0-1.0 | How much of the outside (ocean, wind, rain) seeps into the room |
| | | **— Character —** |
| `osteria_patina` | 0.0-1.0 | Harmonic aging (old wood, worn strings, lived-in quality) |
| `osteria_porto` | 0.0-1.0 | Warm saturation (wine-dark overdrive) |
| `osteria_smoke` | 0.0-1.0 | Gentle HF haze (woodfire atmosphere) |
| | | **— Envelope —** |
| `osteria_attack` | 0.001-4s | Note attack |
| `osteria_decay` | 0.05-4s | Note decay |
| `osteria_sustain` | 0.0-1.0 | Sustain level |
| `osteria_release` | 0.05-8s | Note release |
| | | **— FX —** |
| `osteria_sessionDelay` | 0.0-1.0 | Conversational delay mix |
| `osteria_hall` | 0.0-1.0 | Room size (pub corner → harbor cathedral) |
| `osteria_chorus` | 0.0-1.0 | Paired-string shimmer |
| `osteria_tape` | 0.0-1.0 | Lo-fi warmth (field recording character) |

*Full parameter list defined in Phase 1 architecture.*

---

## Macro Mapping (M1-M4)

| Macro | Label | Controls | Behavior |
|-------|-------|----------|----------|
| M1 | CHARACTER | `osteria_blendMode` + per-voice shore convergence | At 0, the quartet is spread across shores — maximum cultural diversity, maximum timbral tension. At 1, all voices converge on the same shore — unified folk ensemble. The identity dial. |
| M2 | MOVEMENT | `osteria_qElastic` (inverse) + `osteria_qStretch` + voice shore drift rate | At 0, tight ensemble locked together. At 1, voices wander freely across shores, stretching rubber bands, accumulating memory. The nomad dial. |
| M3 | COUPLING | `osteria_qSympathy` + `osteria_qMemory` + coupling amount when coupled | At 0, voices are independent — parallel monologues. At 1, voices deeply influence each other AND carry heavy memory of every shore. In coupled mode, controls how much external engine audio is treated as "local music." The conversation dial. |
| M4 | SPACE | `osteria_tavernMix` + `osteria_hall` + `osteria_oceanBleed` | At 0, quartet plays in open air — vast, exposed, wind-carried. At 1, deep inside the pub — warm, intimate, sheltered. The hearth dial. |

All 4 macros produce audible, dramatic change at every point in their range in every preset.

---

## Cultural Lens: Pan-Maritime (Shared with OSPREY)

### The Shore Is the Meeting Place

Where OSPREY hears the ocean's voice, OSTERIA hears the human voice answering it. Every maritime culture builds gathering places at the water's edge — and in those places, music happens that could happen nowhere else. The sea is in the room. The salt is in the wood. The longing is in every note.

### The Taverns

**Alfama Fado House** (Lisbon, Atlantic) — Low stone ceiling, candlelight, guitarra portuguesa on the wall. The fadista stands and sings without amplification. The room falls silent. Saudade fills the space like smoke. When the song ends, the room exhales. The ocean is two streets away.

**Nordic Sjøhus** (Bergen, Nordic) — Timber-framed waterfront warehouse converted to gathering hall. Hardingfele music echoes off thick wooden walls. The cold outside is absolute; the warmth inside is earned. Someone is always playing langspil in the corner, the drone a constant beneath conversation.

**Piraeus Rembetika Den** (Athens, Mediterranean) — Open-air terrace, tile floor, bouzouki and wine. Rembetika — the Greek blues — born in the harbor among refugees, sailors, and outcasts. The darbuka keeps time. The ney player closes his eyes. The Mediterranean night is warm and the music spills into the street.

**Izakaya by the Harbor** (Osaka, Pacific) — Paper screens, low tables, the clink of ceramic sake cups. A shakuhachi player performs in the garden beyond the screens. Koto notes drift in like rain. The Pacific is patient and the music reflects it — unhurried, precise, full of space.

**Morna Bar** (Mindelo, Southern) — Corrugated tin roof, open walls, the tropical night breathing through. Cesária Évora's barefoot spirit lives here. Cavaquinho and violin. The rhythm of morna — slow waltz of the trade winds. Children play outside. The Southern ocean is warm and the melancholy is soft.

### The Quartet's Journey

The jazz quartet is not a sample library. It's a synthesis concept: four voices in conversation, each carrying the resonator DNA of a specific folk instrument tradition, modifiable in real-time by their position on the shore axis.

The quartet arrives at a shore. They listen. They absorb. The bass player picks up the local low-end character — not by copying, but by letting the shore's resonator profile shape his voice. The melody player finds the local melodic instrument's formant structure entering her timbre. They play together, and what comes out is jazz inflected by the local tradition.

Then one voice starts drifting toward a new shore. The rubber band stretches. The ensemble sound acquires tension — two tuning systems, two sets of formants, two traditions pulling at each other. The other voices feel the pull and begin to follow, or resist. The music is alive in a way no static patch can be.

When they arrive at the new shore, they're not the same quartet that left. `osteria_qMemory` ensures that. The oud warmth from the Mediterranean visit lives in the bass player's timbre now, blending with the koto precision of the Pacific shore they've just reached. The quartet is a living map of everywhere it's been.

### Instrument Heritage (Per Shore)

**Atlantic:**
- **Guitarra Portuguesa** (Lisbon/Coimbra) — 12-string, paired courses, natural shimmer
- **Kora** (West Africa/Mandé) — 21-string bridge harp, rolling arpeggios like swells
- **Uilleann Pipes** (Ireland) — bellows-driven, bag as fluid pressure system
- **Bodhran** (Ireland) — frame drum, the heartbeat of the session

**Nordic:**
- **Hardingfele** (Norway) — sympathetic strings creating shimmering halos
- **Langspil** (Iceland) — bowed drone, the sound of still fjord water
- **Kulning** (Scandinavia) — herding call, high formant cry across valleys
- **Sámi Drum** (Sápmi) — frame drum with joik vocal tradition

**Mediterranean:**
- **Bouzouki** (Greece) — metallic shimmer, rembetika soul
- **Oud** (Arab world/Turkey) — warm plucked fretless, maqam inflections
- **Ney** (Turkey/Persia) — end-blown flute, breath AS sound
- **Darbuka** (Middle East/North Africa) — goblet drum, crisp articulation

**Pacific:**
- **Koto** (Japan) — 13-string zither, precise harmonics
- **Shakuhachi** (Japan) — bamboo flute, breath and silence equally important
- **Conch Shell** (Polynesia) — horn resonance, calling across water
- **Taiko** (Japan) — massive drums, thunderous ensemble

**Southern:**
- **Cavaquinho** (Cape Verde/Brazil) — small bright guitar, morna rhythm
- **Valiha** (Madagascar) — bamboo tube zither, delicate melody
- **Gamelan** (Java/Bali) — inharmonic metallic interlocking patterns
- **Djembe** (West Africa) — hand drum, three voices (bass/tone/slap)

---

## Coupling Interface Design

### OSTERIA as Target (receiving from other engines)

| Coupling Type | What XOsteria Does | Musical Effect |
|---------------|-------------------|----------------|
| `AudioToWavetable` | Source audio treated as "local music" — shapes quartet resonator character | Any engine becomes a shore. OBSIDIAN crystal becomes a folk tradition the quartet absorbs. |
| `AmpToFilter` | Source amplitude modulates ensemble tightness | Rhythmic pumping of the elastic — quartet stretches and snaps to another engine's dynamics. |
| `AudioToFM` | Source audio excites the tavern room model | Another engine's sound reverberates in the pub. The ocean (OSPREY) bleeds through the walls. |
| `EnvToMorph` | Source envelope drives shore drift | External dynamics push voices between coastlines. |

**Primary coupling:** `AudioToWavetable` — any engine's output absorbed as "local folk character." This is what makes OSTERIA a universal transformer: feed it OBSIDIAN and the quartet plays crystal-inflected jazz. Feed it OUROBOROS and they play chaos-inflected folk.

**The OSPREY × OSTERIA coupling (the diptych):**

| Direction | Type | What Happens |
|-----------|------|-------------|
| OSPREY → OSTERIA | `AudioToFM` | Ocean turbulence bleeds into the tavern. Storm rattles the windows. Waves enter the room model. The quartet plays louder to be heard over the gale. |
| OSPREY → OSTERIA | `AmpToFilter` | Sea state modulates ensemble tightness. Calm ocean = relaxed session. Storm = frantic, tight, urgent playing. |
| OSTERIA → OSPREY | `AudioToFM` | The quartet's music excites OSPREY's resonators. Human music becomes ocean energy. The sea responds to the song. |
| OSTERIA → OSPREY | `AmpToFilter` | Quartet dynamics modulate wave intensity. The fisherman's song calms the storm. |

### OSTERIA as Source (sending to other engines)

`getSampleForCoupling()` returns: post-room ensemble mix, stereo, normalized ±1.

| Target Engine | Coupling Type | Musical Effect |
|---------------|---------------|----------------|
| OSPREY | AudioToFM | Quartet music excites ocean resonators — the song becomes the sea |
| OPAL | AudioToWavetable | Ensemble output granulated — the session shattered into time particles |
| ORIGAMI | AudioToWavetable | Quartet audio spectrally folded — folk kaleidoscope |
| OVERDUB | getSample | Tavern session through dub echo chain — maritime dub |
| OBSIDIAN | AmpToFilter | Ensemble dynamics modulate crystal stiffness — the band shapes the crystal |

### Coupling types OSTERIA should NOT receive

| Type | Why |
|------|-----|
| `AmpToChoke` | Choking the ensemble kills all four voices simultaneously — musically destructive, not creative |
| `PitchToPitch` | Each voice already has its own pitch identity; external pitch coupling overrides the shore system |

---

## Visual Identity

- **Accent color:** Porto Wine `#722F37`
  - Deep burgundy — the color of port wine poured in every seaside tavern from Lisbon to Piraeus
  - Warm, human, lived-in — the counterpoint to OSPREY's cold Azulejo Blue
  - Distinguishes from existing reds: Strange Attractor Red `#FF2D2D` (OUROBOROS), Vermillion Fold `#E63946` (ORIGAMI), Warm Red `#FF6B6B` (ORBITAL) — all bright; Porto Wine is deep and dark
- **Material/texture:** Worn wood grain — the surface of a tavern table polished by a century of elbows, wine glasses, and conversation. Warm, organic, imperfect.
- **Gallery panel character:** The warm white gallery shell frames a deep wine-dark panel. The surface should evoke aged wood — subtle grain texture, warm patina. Thin copper/brass trim suggesting tavern hardware (hinges, handles, taps). Subtle animation: a gentle warm glow that breathes like firelight.
- **Icon concept:** Four figures around a table — abstracted to simple shapes, suggesting both musicians and drinking companions. Rendered in Porto Wine and warm cream. Circular composition suggesting the round table, the gathering, the ensemble.

---

## Mood Affinity

| Mood | Affinity | Why |
|------|----------|-----|
| Foundation | **High** | Tight ensemble presets with unified shore position — warm, stable, grounding |
| Atmosphere | **High** | Stretched quartet + tavern room — immersive, spatial, narrative |
| Entangled | **High** | OSPREY×OSTERIA coupling presets. Quartet absorbing other engines' character. The engine's reason for existing in XOmnibus. |
| Prism | **High** | Bright stretched presets where voices on different shores create shimmering harmonic tension |
| Flux | **High** | Journey presets with voices drifting between shores, memory accumulating |
| Aether | Medium | Deep memory presets where voices carry so much history they become ghostly, indistinct |

Primary moods: Foundation, Atmosphere, Entangled, Prism, Flux. (Uniquely, OSTERIA has high affinity for 5 of 6 moods — the ensemble concept is versatile.)

---

## Preset Strategy (Phase 0 Sketch)

**200 presets at v1.0:**

| Category | Count | Character |
|----------|-------|-----------|
| **Tavern Sessions** | 30 | Tight quartet in shore-specific pubs. High tavernMix, warm room. Each shore gets 6 presets capturing local atmosphere + instrument character. Foundation/Atmosphere. |
| **Shore Folk** | 25 | Quartet locked to one shore, playing pure folk-inflected jazz. The "reference" presets — what does Atlantic jazz sound like? Nordic? Mediterranean? Foundation/Prism. |
| **Stretched Quartet** | 25 | Voices on different shores. Low-medium elasticity, high memory. Harmonic tension of being in two places at once. Entangled/Prism. |
| **Rubber Band Journeys** | 25 | Voices drifting between shores via M2 automation. The sound of traveling. Memory accumulates. Narrative presets. Flux/Aether. |
| **Ocean to Hearth** | 20 | M4 sweeps — designed to transition from open-air quartet to deep pub interior (or reverse). Atmosphere/Foundation. |
| **Cross-Pollinated** | 20 | High memory presets where the quartet has "traveled" extensively — voices carry traces of 3-4 shores simultaneously. Thick, complex, layered timbres. Prism/Flux. |
| **The Diptych** | 20 | Designed for OSPREY×OSTERIA coupling. Ocean + shore. Storm + hearth. Creature + quartet. Entangled. |
| **Ensemble Minimalist** | 15 | One or two voices prominent, others nearly silent. Intimate, sparse, breathing. Aether/Atmosphere. |
| **Coupling Showcases** | 20 | OSTERIA absorbing other engines: OBSIDIAN×OSTERIA (crystal folk), OUROBOROS×OSTERIA (chaos session), ONSET×OSTERIA (drum-driven folk). Entangled. |

---

## The Diptych: OSPREY × OSTERIA

These two engines are designed as counterparts — one narrative, two instruments:

```
     THE OCEAN                          THE SHORE
   ┌──────────────┐                 ┌──────────────┐
   │   OSPREY     │ ◄────────────► │   OSTERIA    │
   │              │   coupling      │              │
   │ Turbulence   │                 │ The Quartet  │
   │ Creature     │   ocean bleeds  │ The Tavern   │
   │ voices       │   into tavern   │ The Memory   │
   │ Fluid        │                 │ The Rubber   │
   │ dynamics     │   music calms   │ Bands        │
   │              │   the storm     │              │
   │ Azulejo Blue │                 │ Porto Wine   │
   └──────────────┘                 └──────────────┘
   Force. Vastness.                 Warmth. Community.
   Inhuman.                         Human.
```

They share the Shore System — the same 5 coastlines, the same folk instrument families. But OSPREY hears the ocean's resonances and creature voices; OSTERIA hears the human instruments and communal warmth. They are two perspectives on the same geography.

**The coupling sweet spot:** OSPREY's sea state driving OSTERIA's ensemble tightness, while OSTERIA's quartet music feeds back into OSPREY's resonator bank. The fisherman sings to the sea. The sea answers. The song changes. The sea responds to the change. A feedback loop of human and nature, mediated by music.

---

## What Makes This Different From Existing Engines

1. **Not a sampler.** The folk instrument sounds come from resonator/formant modeling, not recordings. They're alive, responsive, MIDI-driven.

2. **Not a layer stack.** The four voices interact through elastic coupling — they influence each other, pull on each other, share memory. They're not four independent synth patches; they're an ensemble.

3. **Not a preset morph.** Shore position is a continuous synthesis parameter, not a crossfade between preset banks. At shore=1.5, you get a timbre that exists nowhere in the real world — Nordic-Mediterranean hybrid folk jazz. The synthesis creates novel timbres, not just blends.

4. **Memory is unprecedented.** No existing synth engine accumulates timbral history. A note played NOW sounds different from the same note played 30 seconds ago, because the voice has traveled to a new shore and returned carrying traces. The sound has a past.

---

## Decision Gate: Phase 0 → Phase 1

- [x] Concept brief written
- [x] XO word feels right (XOsteria — the gathering place, the tavern, the human answer)
- [x] Gallery gap is clear (no ensemble synthesis with elastic inter-voice coupling)
- [x] At least 2 coupling partner ideas (OSPREY, OBSIDIAN, OUROBOROS, OPAL, ORIGAMI, ONSET)
- [x] Unique capability defined (rubber-band quartet with shore morphing and timbral memory)
- [x] Cultural lens defined (pan-maritime, shared shore system with OSPREY)
- [x] Diptych relationship with OSPREY defined
- [ ] Excited about the sound ← **your call**

**→ Pending approval for Phase 1: Architect**

---

*XO_OX Designs | Engine: OSTERIA | Accent: #722F37 | Prefix: osteria_*
*Companion: OSPREY | Accent: #1B4F8A | Prefix: osprey_*
*Together: The Diptych — ocean and shore, force and warmth, nature and humanity*
