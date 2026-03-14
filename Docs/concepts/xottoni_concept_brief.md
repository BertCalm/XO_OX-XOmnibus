# XOttoni — Concept Brief

*Phase 0 | March 2026 | XO_OX Designs*
*Three lake sturgeon cousins from a Planet of Great Lakes play brass and sax — growing up in real time*

---

## Identity

**XO Name:** XOttoni
**Gallery Code:** OTTONI
**Accent Color:** Patina `#5B8A72`
**Parameter Prefix:** `otto_`
**Plugin Code:** `Xott`
**Engine Dir:** `Source/Engines/Ottoni/`

**Thesis:** XOttoni is a triple-voice brass and sax engine where three cousins of different ages — toddler, tween, and teen — each bring their own instrument complexity, expressiveness, and dedicated FX chain. The GROW macro tells a coming-of-age story, sweeping from raw innocent blasts to fully expressive virtuosity. They're from a neighboring freshwater planet, so their harmonics are a little off, a little foreign, a little weird.

**Sound family:** Hybrid — brass textures / worldwide winds / triple-voice interaction instrument

**Unique capability:** Three-voice architecture with age-scaled complexity. No other engine has voices that differ not just in timbre but in *capability* — the toddler physically cannot play what the teen plays. Each voice has its own FX chain scaled to its maturity. The GROW macro crossfades between three levels of musical sophistication. Combined with the FOREIGN parameter that shifts the overtone series away from Earth-standard brass, XOttoni creates sounds that are familiar enough to read as "brass" but alien enough to feel like they come from somewhere else entirely.

---

## Aquatic Identity

Lake sturgeon. Living fossils from a cold freshwater world.

On a neighboring planet — one with no oceans, only vast interconnected freshwater lakes stretching horizon to horizon — three cousins swim through water so cold and clear you can see the bottom a hundred meters down. Their planet is the Great Lakes writ cosmic. Michigan made mythological. Muskegon as a star system.

Lake sturgeon are ancient armored fish. They've existed for 150 million years. They have bony scutes instead of scales, whisker barbels for sensing the bottom, and they grow from tiny smooth fingerlings into six-foot armored giants over decades. Three cousins at three life stages look like three different species — the baby is a translucent wisp, the juvenile is developing its armor plates, the sub-adult is a full prehistoric warrior with a face only a mother could love.

The toddler sturgeon is all mouth and no control. She picks up a conch shell and BLASTS — one note, maximum volume, pure unfiltered joy. No vibrato, no dynamics, no technique. Just: SOUND. Her FX chain is a single enormous reverb, because when you're three years old, everything should echo forever. She has maybe four parameters. She doesn't need more. She has enthusiasm.

The tween sturgeon is developing his plates. He's found a trumpet and he's practicing — scales, melodies, attempting vibrato (overdoing it), trying to match what his older cousin does. He's eager and earnest and slightly awkward. His FX chain has chorus (because chorus is cool, right?) and a delay (because delay makes everything sound more professional) and a light drive (because the teen has distortion and he wants some too). He's trying SO HARD. He has seven or eight parameters — enough to express something real, not enough to express everything he feels.

The teen sturgeon is fully armored. She plays French horn, trombone, tenor sax, baritone sax — instruments that require years of breath control, embouchure precision, and musical intelligence. She has vibrato that makes you ache, growl techniques, flutter-tongue, multiphonics. She can play the dungchen (Tibetan long horn), the kakaki (West African royal trumpet), the alphorn. She has ALL the pedals — distortion, wah, phaser, delay, reverb. Her FX chain is as complex as her playing. She has ten or twelve parameters and she uses every one. She's technically brilliant but she has ATTITUDE. She's a teen. Of course she does.

The Patina accent color is oxidized brass — the color copper turns when it sits in cold freshwater. Green-gray, muted, ancient. The color of the sturgeon's bony plates. The color of a trumpet left on a Great Lakes shoreline for a hundred years. It's not the bright gold of polished brass — it's brass that has lived in cold water, that has weathered, that has character. Like the cousins themselves.

---

## Polarity

**Position:** SUNLIT SHALLOWS (cold current) — a freshwater tributary running through the warm shallows, near their cousins XObbligato. A visiting species from a cold-water system, distinct from the warm-water natives.
**feliX-Oscar balance:** 55% feliX / 45% Oscar — brass is bright and transient (feliX) but has depth and weight (Oscar). The teen leans Oscar (deeper, more complex). The toddler leans feliX (bright, impulsive, surface). The tween is exactly 50/50.

---

## Lineage

**Mother's sister's children:** Cousins of XObbligato (the flying fish brothers), who are sons of XOrphica (the siphonophore goddess). The cousins come from a different planet — same extended family, different ecosystem. Where XOrphica's ocean world is warm and salty, XOttoni's lake planet is cold and fresh.

**Generation:** Third Generation — New Niches. But from a parallel evolutionary line. The sturgeon cousins diverged from the main XO_OX column early, developing in isolation on their freshwater planet. When they visit the main column, they bring unfamiliar harmonics and strange techniques.

**The Family Architecture:**
- **XOrphica** (mother): splits by REGISTER (2 paths — bass/treble). Signature: SURFACE.
- **XObbligato** (sons): splits by PERSONALITY (2 paths — Peter/Wolf). Signature: BOND.
- **XOttoni** (cousins): splits by MATURITY (3 paths — toddler/tween/teen). Signature: GROW.

Each generation adds a path. Each signature macro tells a different universal story: environment, relationship, coming of age.

---

## DSP Architecture

```
MIDI Note ──→ Voice Router
              (notes assigned by mode: age-split, layer, round-robin)
              |                    |                    |
    ┌─────────┴─── TODDLER ──┐  ┌┴─── TWEEN ───────┐  ┌┴─── TEEN ─────────┐
    │ Age ~3 | Raw | Simple   │  │ Age ~11 | Eager   │  │ Age ~16 | Skilled │
    │                         │  │                   │  │                    │
    │ Exciter: lip buzz       │  │ Exciter: lip buzz │  │ Exciter: lip buzz │
    │   (loose, uncontrolled) │  │   (developing)    │  │   (precise/growl) │
    │         ↓               │  │         ↓         │  │         ↓         │
    │ Waveguide: simple tube  │  │ Waveguide: valved │  │ Waveguide: full   │
    │   (no valves, 1 mode)   │  │   tube (3 modes)  │  │   bore model      │
    │   conch/bugle/shofar    │  │   trumpet/alto sax│  │   (8+ modes)      │
    │         ↓               │  │         ↓         │  │   horn/trombone/   │
    │ Expression: none        │  │ Expression: basic  │  │   tenor sax/tuba  │
    │   on/off, loud/louder   │  │   vibrato (wobbly)│  │         ↓         │
    │                         │  │   basic dynamics   │  │ Expression: full  │
    │         ↓               │  │         ↓         │  │   vibrato, growl, │
    │ FX Chain T              │  │ FX Chain TW        │  │   flutter-tongue, │
    │   BIG Reverb            │  │   Chorus           │  │   multiphonics    │
    │   (that's it.           │  │   Delay            │  │         ↓         │
    │    everything echoes     │  │   Light Drive     │  │ FX Chain TN       │
    │    when you're 3)       │  │                   │  │   Distortion/Wah  │
    │                         │  │                   │  │   Phaser           │
    │ Params: ~5              │  │ Params: ~8        │  │   Delay            │
    │                         │  │                   │  │   Reverb            │
    └─────────┬───────────────┘  └────────┬─────────┘  │                    │
              │                           │            │ Params: ~12        │
              │                           │            └────────┬───────────┘
              │                           │                     │
              └───────────────┬───────────┴─────────────────────┘
                              │
                        GROW Crossfade Engine
                        (age-weighted mix of all three voices)
                              │
                        Foreign Harmonics Processor
                        (overtone series shift, microtonal drift)
                              │
                        Master Output
```

### Wind Synthesis — Brass Waveguide

Same waveguide core as XObbligato (family shared DSP), but configured for brass/sax instead of flute/reed:

**Exciter (the embouchure):**
- Lip buzz model — nonlinear oscillation of the "lips" against a mouthpiece
- Tightness parameter: loose buzz (toddler, fat/unfocused) → medium (tween, developing) → tight precision (teen, focused/powerful)
- Overblow: increase pressure to jump registers. Toddler can't control it (random cracks). Tween sometimes gets it. Teen commands it.
- Sax exciter variant: single reed model for sax-family instruments

**Waveguide (the bore):**
- Delay line + lowpass filter = the resonant air column
- Bore shape: cylindrical (trumpet-like) vs conical (sax-like) vs flared (horn-like)
- Bell radiation model: controls how much energy leaves vs reflects
- Slide model (trombone): continuous pitch via delay line length modulation

**Age-Scaled Capability:**

| Capability | Toddler | Tween | Teen |
|-----------|---------|-------|------|
| Note range | ~5 notes (partial series) | Full chromatic | Full + extended |
| Vibrato | None | Basic (wobbly, 4-7Hz) | Controlled (rate, depth, delay) |
| Dynamics | ff only | p to ff | ppp to fff with control |
| Articulation | Blast/silence | Tongued, slurred | Tongued, slurred, staccato, marcato, flutter |
| Extended techniques | Accidental squeaks | Occasional growl attempt | Growl, flutter-tongue, multiphonics, fall-offs |
| Pitch accuracy | ±30 cents (can't center) | ±10 cents (mostly there) | ±2 cents (precise when she wants, intentionally bent when she doesn't) |

### The Worldwide Brass & Sax Gallery

**Toddler Instruments (no valves, no keys — just blow):**

| Instrument | Origin | Character |
|-----------|--------|-----------|
| Conch Shell | Polynesia / Caribbean | Single deep blast, ocean call |
| Vuvuzela | South Africa | One note, maximum volume, maximum enthusiasm |
| Shofar | Jewish tradition | Raw, ancient, ceremonial — two modes (tekiah, teruah) |
| Hunting Horn | European | Simple fanfare, 3-4 natural harmonics |
| Bull Horn | Various African | Deep, bellowing, primal |
| Bugle | Military / worldwide | No valves — only the harmonic series |

**Tween Instruments (learning valves and keys):**

| Instrument | Origin | Character |
|-----------|--------|-----------|
| Trumpet | Western | Bright, heroic, the instrument everyone starts with |
| Cornet | Western | Warmer, rounder trumpet — slightly easier for small hands |
| Alto Saxophone | Western (Belgian invention, worldwide adoption) | Sweet, smooth, the "cool" sax |
| Soprano Saxophone | Western | Bright, singing, Kenny G ambitions |
| Flugelhorn | Western | Dark, velvety trumpet — the romantic one |
| Mellophone | Western (marching band) | Mid-range, accessible, the tween's comfort zone |

**Teen Instruments (full mastery):**

| Instrument | Origin | Character |
|-----------|--------|-----------|
| French Horn | Western | The most difficult brass — wide range, complex partials, noble |
| Trombone | Western | THE SLIDE — continuous pitch, jazz growl, power |
| Tenor Saxophone | Western / Jazz | The voice of jazz — Coltrane, Rollins, Shorter |
| Baritone Saxophone | Western / Jazz | Deep, rich, anchoring — Mulligan, Pepper Adams |
| Tuba | Western | The foundation — massive, grounding, surprising agility |
| Euphonium | Western | Warm, lyrical, the tuba's singing cousin |
| Dungchen | Tibet | Enormous long horn — ceremonial, deep, otherworldly |
| Kakaki | West Africa (Hausa) | Royal long trumpet — ceremonial, commanding, 3-4 meters long |
| Alphorn | Switzerland | Mountain-spanning wooden horn — natural harmonics, vast reverb |
| Didjeridu-brass hybrid | Conceptual | Circular breathing + brass embouchure — drone + melody |

### The GROW System — Coming of Age as Modulation

The GROW macro (M2) is XOttoni's signature. It crossfades between the three cousins' voices AND their FX chains simultaneously, telling a complete coming-of-age story:

| GROW Range | Stage | Dominant Voice | Expression Level | FX Complexity | Emotional Quality |
|-----------|-------|---------------|-----------------|---------------|-------------------|
| 0.0 – 0.15 | **INFANT** | Toddler only | On/off, no control | Just reverb (huge) | Pure joy, zero skill, maximum enthusiasm |
| 0.15 – 0.3 | **CHILD** | Toddler + hint of tween | Basic pitch control | Reverb + filter wobble | Discovery, first melodies, delightful clumsiness |
| 0.3 – 0.45 | **TWEEN** | Tween dominant | Vibrato (wobbly), dynamics | Chorus + delay + light drive | Eagerness, trying hard, occasionally beautiful |
| 0.45 – 0.6 | **ADOLESCENT** | Tween + hint of teen | Developing vibrato, some articulation | Chorus + delay + phaser appearing | Awkward confidence, voice cracking, growing |
| 0.6 – 0.75 | **TEEN** | Teen dominant | Full vibrato, growl, flutter | Full chain: dist + wah + phaser + delay + reverb | Attitude, skill, rebellion, brilliance |
| 0.75 – 0.9 | **VIRTUOSO** | Teen fully expressed | Multiphonics, extended techniques | FX as instruments, self-oscillating | Transcendence — the cousin who surprises everyone |
| 0.9 – 1.0 | **PRODIGY** | All three layered | All three playing together | All three FX chains active | The full family playing — toddler's joy + tween's eagerness + teen's mastery |

**The arc:** Growing up is gaining skill but losing innocence. At GROW=0, the sound has a raw purity the teen can never recapture. At GROW=0.7, the technical mastery is breathtaking but the innocence is gone. At GROW=1.0, all three play together — the prodigy who somehow retains every stage of growth simultaneously. The most complex, most emotional setting.

### FX Chain T — "Toddler" (The Echo)

| Slot | DSP | Character |
|------|-----|-----------|
| Big Reverb | Hall reverb, very long decay, wide | Everything echoes when you're 3. The world is VAST and every sound is AMAZING. |

That's it. One effect. The toddler doesn't need processing. The toddler IS processing.

### FX Chain TW — "Tween" (The Tryhard)

| Slot | DSP | Character |
|------|-----|-----------|
| Chorus | Stereo chorus, slightly too deep | Because chorus makes everything sound "professional" (the tween thinks) |
| Echo Delay | Medium delay, moderate feedback | Copies of yourself = instant band (the tween's logic) |
| Light Drive | Soft-clip saturation, gentle | The tween heard the teen's distortion and wants SOME. Not too much. Mom would hear. |

### FX Chain TN — "Teen" (The Pedalboard)

| Slot | DSP | Character |
|------|-----|-----------|
| Distortion | Asymmetric tube-style overdrive | Grit, edge, attitude — the sound of not caring what adults think |
| Wah Filter | Envelope-follower or LFO-driven bandpass | Expressiveness — the wah pedal is the teen's VOICE |
| Phaser | 6-stage allpass with feedback | Moody sweep — Joy Division's bass player had a phaser |
| Tape Delay | Modulated delay with tape saturation in feedback | Dark, warm echoes that build into self-oscillation |
| Plate Reverb | Medium plate, moderate decay | Space, but controlled — the teen knows how to use reverb tastefully (mostly) |

### Foreign Harmonics Processor

A global processor that shifts XOttoni's entire harmonic output away from Earth-standard brass:

- **Overtone stretch:** Shifts the spacing of harmonics — standard brass has integer-ratio partials (1:2:3:4...). XOttoni's cold-water physics creates slightly stretched or compressed ratios (1:2.02:3.01:4.04...). Subtle but uncanny.
- **Microtonal drift:** Base tuning floats ±15 cents from A=440Hz on a slow LFO (~0.03Hz). Their planet doesn't have a fixed A.
- **Cold timbre filter:** A subtle high-shelf that adds a "glassy" quality — the clarity of cold freshwater vs warm saltwater.
- **Controlled by FOREIGN macro (M3):** At 0, all foreign processing is bypassed — standard Earth brass. At 1, full alien harmonics. The cousins can "code-switch" between sounding normal and sounding like themselves.

---

## Macro System

| Macro | Name | Controls | Musical Intent |
|-------|------|----------|---------------|
| M1 | **EMBOUCHURE** | Lip tightness + buzz character + overblow threshold + mouthpiece size | How the cousins blow — from loose toddler buzz to precise teen embouchure to overblown squealing |
| M2 | **GROW** | THE SIGNATURE — age-weighted crossfade between all three voices + FX chains | Coming of age: toddler joy → tween eagerness → teen mastery → prodigy transcendence |
| M3 | **FOREIGN** | Overtone stretch + microtonal drift + cold timbre filter | How alien the cousins sound — Earth brass ↔ their planet's cold-water brass |
| M4 | **LAKE** | FX depth for all three chains + freshwater reverb character + stereo width | Dry intimate → processed → vast Great Lakes reverb. Cold, clear, deep. |

---

## Voice Architecture

- **Max voices:** 12 — 4 per cousin maximum (brass instruments are naturally monophonic, multiple notes = ensemble/section)
- **Voice stealing:** Oldest note per cousin — each cousin manages their own pool
- **Legato mode:** Yes for tween and teen (brass instruments slur naturally). No for toddler (toddlers don't slur — they blast individual notes with silence between).
- **Glide:** Trombone slide for teen (continuous pitch glide). Valve half-step slurs for tween. None for toddler (pitch jumps only — the harmonic series).

---

## Coupling Thesis

XOttoni visits from another planet. They bring unfamiliar harmonics and strange techniques to the main water column. Their coupling is colored by their foreignness — the overtone mismatch between their cold-water brass and the warm-water engines creates harmonic tension that is musically productive.

### As Coupling Source (XOttoni → others)

| Route | What It Sends | Partner | Musical Effect |
|-------|--------------|---------|---------------|
| `getSampleForCoupling()` | Combined three-cousin output (age-weighted by GROW) | Any | Brass melody/texture modulating other engines |
| Toddler output only | Raw blast signal | ONSET | Toddler blast triggers drum hits — baby plays the kit |
| Teen output only | Complex expressive signal | ODYSSEY | Teen sax drives JOURNEY — jazz-psychedelic fusion |
| GROW value | Maturity state as mod source | ORPHICA (aunt) | Cousins growing up moves the water's SURFACE |

### As Coupling Target (others → XOttoni)

| Route | Source | What It Does | Musical Effect |
|-------|--------|-------------|---------------|
| `AmpToFilter` | ONSET | Drum amplitude → bore resonance | Drums shake the brass — rhythmic resonance changes |
| `EnvToMorph` | ORPHICA | Harp envelope → instrument morph | Aunt's pluck chooses which brass the cousins play |
| `LFOToPitch` | OBBLIGATO | Brothers' wind → cousins' pitch | Family vibrato — sons and cousins trembling together |
| `AudioToFM` | OVERWORLD | Chip audio → exciter modulation | 8-bit brass — retro trumpet with digital buzz |
| `AmpToFilter` | OCEANDEEP | Sub amplitude → embouchure looseness | Deep bass loosens the toddler's buzz even further |

### Signature Pairings

| Pairing | Name | What Happens |
|---------|------|-------------|
| **ORPHICA → OTTONI** | "Aunt & Cousins" | Harp plucks trigger brass responses — aunt plays, cousins answer. Extended family jam. |
| **OBBLIGATO × OTTONI** | "The Reunion" | Sons and cousins together — winds and brass, Peter/Wolf meets toddler/tween/teen. Full family wind ensemble. |
| **OTTONI → OVERDUB** | "Lake Dub" | Cold-water brass through warm tape delay — the cousins visiting the ocean's dub architecture |
| **ONSET → OTTONI** | "Brass Band" | Drums drive brass bore changes — marching band from another planet |
| **OTTONI → OPENSKY** | "Cold Sky" | Freshwater brass reaching into the warm sky — alien horns at sunset |
| **ODYSSEY → OTTONI** | "Jazz Journey" | JOURNEY macro drives GROW — as the journey progresses, the cousins grow up |

### Unsupported Coupling Types

| Type | Why |
|------|-----|
| `AmpToChoke` | The toddler doesn't understand "stop." None of them do. Brass sustains. |
| `AudioToWavetable` | Waveguide-based — no wavetable target |

---

## Signature Sound

Set GROW to 0. The toddler picks up a conch shell and BLASTS. One note. Maximum volume. An enormous reverb tail spreads across the stereo field. It's pure, raw, joyful, and kind of terrifying. Now slowly sweep GROW toward 0.3. The tween emerges — a trumpet attempting a melody, vibrato wobbling, chorus effect making it shimmer. The conch shell fades. The melody is earnest, slightly clumsy, beautiful in its imperfection. Keep sweeping. At 0.7, the teen takes over — a French horn playing a complex passage, vibrato perfectly controlled, growl technique adding edge, the full pedalboard of distortion and wah and phaser creating a wall of expressive brass. Now turn up FOREIGN. The harmonics shift. The brass sounds wrong — not bad, but DIFFERENT. The overtones stretch, the tuning drifts, a cold clarity enters the timbre. This isn't Earth brass anymore. This is brass from a planet where the water is cold and fresh and the lakes are wider than oceans.

At GROW=1.0, all three cousins play together — the toddler's raw blast, the tween's eager melody, the teen's virtuosic counterpoint. Three ages, three FX chains, three levels of complexity, all sounding simultaneously. The prodigy moment: a brass section that contains every stage of growing up in one chord.

---

## The Great Lakes Connection — Design Inspiration

A nod to Muskegon, Michigan — a Great Lakes city where the water is cold, fresh, and vast like an inland sea.

1. **Cold freshwater acoustics** — the FOREIGN processor models how cold fresh water affects brass harmonics differently than warm salt water. Cleaner, colder, more transparent.
2. **Industrial heritage** — Michigan's industrial history (steel mills, auto plants) connects to brass instruments. The Patina accent color is oxidized metal left in lake water.
3. **Motown brass** — Detroit is Michigan. Motown's horn sections (Funk Brothers, Tower of Power influence) are the spiritual ancestors of XOttoni's brass. The tween is learning Motown riffs. The teen is playing Coltrane's sheets of sound on a sax from another planet.
4. **Lake effect** — the LAKE macro's reverb has a unique character: cold, deep, wide, with the specific early reflections of sound bouncing across a flat, vast body of fresh water. Not ocean. Not cave. Lake.
5. **Sturgeon as living fossils** — Lake sturgeon have been in the Great Lakes for 150 million years. They're ancient, patient, slightly alien. The cousins carry this ancient quality — their brass sounds old even when the toddler is playing it for the first time.

---

## Visual Identity

- **Accent color:** Patina `#5B8A72` — oxidized brass in cold freshwater. Green-gray, ancient, weathered.
- **Material/texture:** Verdigris metal — the green patina that forms on brass exposed to cold, wet conditions. Rough, textured, organic. Three overlapping circles of different sizes (toddler smallest, teen largest) like sturgeon scales or brass bell cross-sections.
- **Icon concept:** Three brass bells of increasing size arranged in a line — small (toddler), medium (tween), large (teen). The smallest is simple and round (like a bugle bell), the largest is complex and flared (like a French horn bell). A subtle fish-scale pattern on each.
- **Panel character:** Three vertical columns of increasing visual complexity — left column (toddler) has one big knob and a reverb. Middle column (tween) has several knobs and moderate FX. Right column (teen) has many knobs and full FX. The panel literally shows the age difference through UI density. A horizontal "GROW" slider at the bottom crossfades between all three.

---

## Mood Affinity

| Mood | Affinity | Why |
|------|----------|-----|
| **Foundation** | Medium | Earth-mode brass (FOREIGN=0) provides solid harmonic foundation |
| **Atmosphere** | Medium | Toddler reverb blasts + teen sustain create atmospheric brass beds |
| **Entangled** | High | GROW system + family coupling creates rich entanglement |
| **Prism** | Medium | Tween chorus + teen wah = prismatic brass color |
| **Flux** | High | GROW sweeps create constantly evolving complexity — never static |
| **Aether** | Low | Brass is too present/physical for aetheric stillness (but FOREIGN=1 toddler reverb approaches it) |

---

## Parameter Count Estimate

| Category | Params | Examples |
|----------|--------|---------|
| Toddler Voice | 5 | instrument select, blow intensity, pitch (harmonic), volume, reverb size |
| Tween Voice | 8 | instrument select, bore shape, embouchure, vibrato rate+depth, dynamics, chorus mix, delay time |
| Teen Voice | 12 | instrument select, bore shape, embouchure, vibrato rate+depth+delay, growl, flutter, articulation, distortion drive, wah freq, phaser rate, delay+reverb mix |
| Voice Routing | 3 | mode (age-split/layer/round-robin), split point, balance |
| Foreign Processor | 3 | overtone stretch, microtonal drift, cold timbre |
| Macros | 4 | EMBOUCHURE, GROW, FOREIGN, LAKE |
| Master | 3 | volume, pan, glide rate |
| **Total** | **~38** | Deliberately asymmetric — the parameter count difference IS the engine's identity |

---

## Family System Summary

| Engine | Split Axis | Paths | Signature Macro | Story Type | Species | Position |
|--------|-----------|-------|----------------|-----------|---------|----------|
| **XOrphica** | Register (pitch) | 2 (bass/treble) | SURFACE | Environment — moving the boundary | Siphonophore | THE SURFACE |
| **XObbligato** | Personality (voice) | 2 (Peter/Wolf) | BOND | Relationship — sibling dynamics | Flying fish | SUNLIT SHALLOWS |
| **XOttoni** | Maturity (age) | 3 (toddler/tween/teen) | GROW | Coming of age — gaining skill, losing innocence | Lake sturgeon | SHALLOWS (cold current) |

Three engines. Three split philosophies. Three universal human stories. One family.

---

*XO_OX Designs | XOttoni — three cousins, three ages, one family growing up in cold clear water*
