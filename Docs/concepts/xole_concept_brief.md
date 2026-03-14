# XOlé — Concept Brief

*Phase 0 | March 2026 | XO_OX Designs*
*Three fiery aunts argue through strummed strings while their husbands get reluctantly dragged in*

---

## Identity

**XO Name:** XOlé
**Gallery Code:** OLE
**Accent Color:** Hibiscus `#C9377A`
**Parameter Prefix:** `ole_`
**Plugin Code:** `Xole`
**Engine Dir:** `Source/Engines/Ole/`

**Thesis:** XOlé is a six-voice strummed string engine where three Afro-Latin aunts — tres cubano, berimbau, and charango — form shifting 2-vs-1 alliances, each with their own FX chain. As the DRAMA macro rises, their three husbands (Palestinian oud, Greek bouzouki, Thai pin) get reluctantly dragged into the argument. It's a family dinner where everyone has an opinion and nobody's backing down.

**Sound family:** Hybrid — strummed/plucked strings / ensemble interaction / multi-tradition world instrument

**Unique capability:** Triangle alliance system with escalating voice count. No other engine has a three-way relationship where alliances shift — two voices synchronize against an isolated third, and the configuration rotates. The DRAMA macro escalates from 3 voices to 6 as the husbands' instruments enter, adding Palestinian, Greek, and Thai string traditions to the Afro-Latin foundation. The 2-vs-1 dynamic creates rhythmic patterns impossible with static voice relationships — syncopation through social dynamics.

---

## Aquatic Identity

Parrotfish. Three of them, three different color morphs, arguing on a reef they built themselves.

The aunts' planet is tropical — crystal clear water so transparent you can see the white sand bottom from the surface. Warm currents, shallow reefs, bioluminescent plankton that lights up the nighttime shallows like scattered gemstones. It's paradise. And the parrotfish made it. They literally eat coral with their fused-tooth beaks, digest it, and excrete fine white sand. The tropical beaches of their planet — the crystal clear shallows, the white sand flats, the paradise everyone admires — exist because the parrotfish built them one bite at a time. And they argue about who did the most work.

Aunt 1 is the Stoplight Parrotfish — large, green-to-red color change, dramatic. She plays tres cubano, the three-course Cuban instrument that drives the montuno pattern — rhythmic, insistent, the heartbeat of son cubano and salsa. She starts the arguments. She always starts the arguments. Her strumming is fierce — rasgueo patterns that cut through the water like her beak cuts through coral. Her FX chain has a warm overdrive and a rhythmic delay that turns her patterns into cascading walls of percussive harmony. She's the loudest. She knows it. She doesn't care.

Aunt 2 is the Blue Parrotfish — entirely blue, unusual, stands out. She plays berimbau, the single-string Brazilian instrument with a gourd resonator — percussive, hypnotic, the sound of capoeira and candomblé. She's the unpredictable one. Sometimes she sides with Aunt 1, sometimes with Aunt 3, sometimes she goes rogue and argues with both. Her berimbau creates rhythmic patterns that don't line up with anyone else's — she's playing in a different time signature on purpose. Her FX chain has a resonant filter and a granular stutter that fragments her already percussive tone into something between string instrument and talking drum. She's the wildcard. Nobody knows which side she'll choose.

Aunt 3 is the Rainbow Parrotfish — the largest species, iridescent scales, regal presence. She plays charango, the tiny Andean instrument with a huge sound — ten strings, rapid arpeggios, bird-like brightness. She talks the fastest. Her charango arpeggios are machine-gun rapid, cascading runs that fill every gap in the conversation. Her FX chain has a shimmer reverb and a fast chorus that makes her arpeggios sparkle like sunlight on tropical water. She's usually the peacemaker — until she's not, and then she's the most devastating arguer of all three because she's been keeping score.

The husbands swim nearby, in a loose formation, getting along perfectly. The Palestinian oud player plucks ornamental maqam phrases. The Greek bouzouki player drives steady rebetiko patterns. The Thai pin player picks gentle pentatonic melodies. They communicate through their instruments — no arguments, no raised voices, just three men from different worlds who figured out how to harmonize. When the aunts' argument reaches critical mass (DRAMA > 0.7), the husbands drift closer. "Maybe we should go," the oud says in microtonal arabesques. "Just let them work it out," the bouzouki replies in driving eighth notes. "..." says the pin, playing quietly in the corner, waiting for it to blow over.

At DRAMA = 1.0, everyone is playing at once. Three aunts arguing through three Afro-Latin string instruments, three husbands trying to mediate through three completely different world string traditions, six voices tangled in a family dinner where the food is getting cold and nobody cares because Aunt 1 JUST SAID SOMETHING about Aunt 3's charango technique and now it's ON. And somehow, impossibly, the six voices together create the most beautiful, complex, rhythmically intricate music in the entire gallery. Because that's what family is: chaos that sounds like love if you step back far enough.

The Hibiscus accent is the color of tropical flowers that grow on the shore — deep magenta-pink, fiery, passionate, impossible to ignore. Like the aunts.

---

## Polarity

**Position:** THE REEF (tropical visitor) — warm, colorful, enclosed coral formations. The aunts visit from their own tropical planet — different reef, same energy.
**feliX-Oscar balance:** 60% feliX / 40% Oscar — the arguing is feliX (bright, transient, electric, rhythmic). The love underneath is Oscar (warm, sustaining, patient). The husbands are 40% feliX / 60% Oscar (calmer, deeper, more patient — they've learned).

---

## Lineage

**XOrphica's sisters:** The aunts are the goddess's siblings from a warmer, shallower, more social world. Where XOrphica is solitary and divine, her sisters are communal and fiery. Same family, different temperaments.

**Generation:** Third Generation — New Niches. Parallel evolution on a tropical planet. The aunts diverged early, developing in warm shallow reef systems rather than the deep ocean column. Their instruments evolved for social communication — they play TO each other, not just FOR an audience.

**The Family Architecture:**
- **XOrphica** (mother): splits by REGISTER (2 paths — bass/treble). Signature: SURFACE.
- **XObbligato** (sons): splits by PERSONALITY (2 paths — Peter/Wolf). Signature: BOND.
- **XOttoni** (cousins): splits by MATURITY (3 paths — toddler/tween/teen). Signature: GROW.
- **XOlé** (aunts): splits by ALLIANCE (3→6 paths — 2-vs-1 triangle + husbands). Signature: DRAMA.

Each engine adds complexity. Mother has 2 static paths. Sons have 2 dynamic paths. Cousins have 3 progressive paths. Aunts have 3 shifting paths that expand to 6. The family grows.

---

## DSP Architecture

```
MIDI Note ──→ Voice Router
              (notes assigned by mode: round-robin, split, layer)
              |                    |                    |
    ┌─────────┴── AUNT 1 ────┐  ┌─┴── AUNT 2 ───────┐  ┌─┴── AUNT 3 ───────┐
    │ Tres Cubano             │  │ Berimbau           │  │ Charango           │
    │ Cuban montuno energy    │  │ Brazilian capoeira │  │ Andean arpeggios   │
    │                         │  │                    │  │                    │
    │ Exciter: strum/rasp     │  │ Exciter: stick hit │  │ Exciter: rapid pick│
    │ Waveguide: 3 double     │  │ + gourd resonance  │  │ Waveguide: 10      │
    │   courses (6 strings)   │  │ Waveguide: 1 string│  │   strings, bright  │
    │         ↓               │  │ + caxixi rattle    │  │         ↓          │
    │ FX Chain A              │  │         ↓          │  │ FX Chain C         │
    │   Warm Overdrive        │  │ FX Chain B         │  │   Shimmer Reverb   │
    │   Rhythmic Delay        │  │   Resonant Filter  │  │   Fast Chorus      │
    │   Compression           │  │   Granular Stutter │  │   Bright Delay     │
    │   Room Reverb           │  │   Phaser           │  │   Harmonic Exciter │
    │                         │  │   Spring Reverb    │  │                    │
    └─────────┬───────────────┘  └────────┬──────────┘  └────────┬───────────┘
              │                           │                      │
              └───────────┬───────────────┴──────────────────────┘
                          │
                    Alliance Engine
                    (2-vs-1 rotation, rhythmic sync/desync,
                     level balance, interval control)
                          │
                          │  ← DRAMA > 0.7 triggers:
                          │
    ┌─────────────────────┼─────────────────────────────┐
    │                     │                             │
    ┌──── HUSBAND 1 ──┐  ┌──── HUSBAND 2 ──┐  ┌──── HUSBAND 3 ──┐
    │ Oud              │  │ Bouzouki         │  │ Pin/Phin         │
    │ Palestinian      │  │ Greek            │  │ Thai             │
    │ Deep, ornamental │  │ Bright, driving  │  │ Gentle, mellow   │
    │ Microtonal       │  │ Rebetiko energy  │  │ Pentatonic folk  │
    │ maqam phrases    │  │ 8th note drive   │  │ Quiet melodies   │
    │                  │  │                  │  │                  │
    │ FX: Tape Echo    │  │ FX: Chorus       │  │ FX: Soft Reverb  │
    │ (meditative)     │  │ (accidentally    │  │ (hoping it       │
    │                  │  │  escalating)     │  │  blows over)     │
    └────────┬─────────┘  └────────┬─────────┘  └────────┬─────────┘
             │                     │                      │
             └─────────────┬───────┴──────────────────────┘
                           │
                     Master Output
```

### String Synthesis — Strummed/Attacked Waveguide

Same waveguide core as XOrphica (family shared DSP — vibrating string model), but with completely different excitation:

**Exciter types (per aunt):**

| Aunt | Exciter | Character |
|------|---------|-----------|
| Tres | Strum model: multiple strings excited in rapid sequence (rasgueo). Time spread between string onsets = strum speed. Direction: up/down. | Percussive, rhythmic, driving — each strum is a chord attack |
| Berimbau | Stick impact on wire + gourd resonance body. Coin/stone presses wire for pitch bend. Caxixi rattle adds noise layer. | Percussive, metallic, hypnotic — more percussion than string |
| Charango | Rapid plectrum on paired courses. Tremolo technique: sustained notes via rapid re-picking. | Bright, quick, singing — like a mandolin made of armadillo shell |

**Waveguide (the string):**
- Delay line + lowpass filter = the resonant string
- Body resonance model per instrument: tres (wood box), berimbau (gourd), charango (armadillo shell / wood)
- Sympathetic coupling between strings within each instrument (tres has 6 strings, charango has 10)
- Strum direction and speed affect the onset character

**Key difference from mother's harp:** Mother plucks individual strings. The aunts STRUM chords — multiple strings excited simultaneously or in rapid sequence. Mother is vertical (one string at a time, different depths). The aunts are horizontal (many strings at once, same depth, different attacks). Mother lets go. The aunts hold on.

**Husband instruments — secondary waveguide voices:**

| Husband | Waveguide | Character |
|---------|-----------|-----------|
| Oud | Fretless delay line (continuous pitch), paired courses, large pear-shaped body resonance | Deep, warm, ornamental — microtonal grace notes, maqam scales |
| Bouzouki | Fretted delay line, bright metallic courses, small body resonance | Bright, cutting, driving — steady rhythmic patterns, rebetiko energy |
| Pin/Phin | 3-string simple delay line, medium body, silk strings | Mellow, gentle, pentatonic — the quietest voice in any room |

### The Alliance Engine — 2 vs 1 Triangle

The core interaction system. At any moment, two aunts are allied and one is isolated:

**Alliance configurations:**

| Config | Allied Pair | Isolated | Musical Effect |
|--------|-------------|----------|---------------|
| **A** | Aunt 1 (Tres) + Aunt 2 (Berimbau) | Aunt 3 (Charango) | Cuban-Brazilian groove locks in, charango arpeggios fight for space against the rhythmic wall |
| **B** | Aunt 1 (Tres) + Aunt 3 (Charango) | Aunt 2 (Berimbau) | Melodic-harmonic alliance, berimbau's odd-meter patterns clash against the consonant pair |
| **C** | Aunt 2 (Berimbau) + Aunt 3 (Charango) | Aunt 1 (Tres) | Percussive-bright alliance, tres cubano's heavy montuno fights alone against the agile pair |

**What the alliance does to DSP:**

| Parameter | Allied Pair | Isolated Aunt |
|-----------|------------|---------------|
| Rhythmic sync | Locked — same downbeat, complementary patterns | Offset — syncopated against the pair, hemiola |
| Intervals | Consonant — 3rds, 5ths, octaves | Dissonant — 2nds, tritones, minor 7ths |
| Stereo position | Grouped — closer together in the stereo field | Separated — pushed to opposite side |
| FX character | Matched — similar reverb/delay settings | Contrasting — different FX emphasis |
| Dynamic level | Slightly louder (strength in numbers) | Slightly quieter initially, then louder as she fights back |
| Articulation | Smooth, flowing, complementary | Aggressive, staccato, interrupting |

**SIDES macro (M3) controls the configuration:**
- 0.0 = Config A (Tres + Berimbau vs Charango)
- 0.33 = Transition: alliances shifting, brief moment of all-three harmony
- 0.5 = Config B (Tres + Charango vs Berimbau)
- 0.66 = Transition: shifting again
- 0.83 = Config C (Berimbau + Charango vs Tres)
- 1.0 = Full rotation back toward Config A

At moderate DRAMA, the SIDES position creates stable alliances. At high DRAMA, SIDES becomes unstable — alliances shift rapidly, creating chaotic rhythmic interplay where nobody knows who's on whose side.

### The DRAMA System — Family Dinner as Modulation

| DRAMA Range | Stage | Aunts | Husbands | Sound |
|------------|-------|-------|----------|-------|
| 0.0 – 0.1 | **PEACE** | All three playing together — matched rhythms, consonant intervals, complementary FX | Silent — reading the paper, enjoying the quiet | Beautiful Afro-Latin ensemble. Warm, flowing, the family at its best. |
| 0.1 – 0.25 | **BANTER** | Playful teasing — slight rhythmic displacement, one aunt plays a cheeky ornament | Silent — smiling at each other over the table | Call-and-response, syncopated interplay, humor in the music |
| 0.25 – 0.4 | **SHADE** | Subtle digs — 2-vs-1 forming, one aunt's volume slightly drops, the other two tighten | Silent — exchanging knowing glances, "here we go" | One voice starts to separate from the texture. The allied pair gets rhythmically tighter. |
| 0.4 – 0.55 | **SQUABBLE** | Open disagreement — isolated aunt fights back, rhythmic clashing, intervals widen | Starting to notice — "everything ok in there?" | Polyrhythmic tension. Three patterns competing. The isolated aunt plays OVER the pair. |
| 0.55 – 0.7 | **ARGUMENT** | Full argument — all three voices loud, the pair gangs up, the isolated aunt escalates | Leaning in from the next room | Dissonant, aggressive strumming, dynamic swells, FX pushed harder |
| 0.7 – 0.85 | **HUSBANDS ENTER** | Aunts at peak argument intensity | Reluctantly joining — oud plays calming phrases, bouzouki accidentally adds fuel, pin plays quietly in the corner | 6 voices. Three Afro-Latin strings + oud + bouzouki + pin. Cross-cultural chaos begins. |
| 0.85 – 0.95 | **EVERYBODY IN** | Aunts now arguing about the husbands getting involved | Fully engaged — oud and bouzouki debating, pin still trying to hide | Full family. All 6 instruments going. Four cultural traditions colliding. Magnificent chaos. |
| 0.95 – 1.0 | **LA MESA** | Maximum volume, maximum passion, maximum love | Everyone playing at full intensity | The dinner table is vibrating. Someone knocked over a glass. The food is cold. Nobody cares. It's the most alive sound in the entire gallery. And it's somehow beautiful because every note, every clash, every interruption comes from love. |

### FX Chain A — Aunt 1 (Tres Cubano) "The Firestarter"

| Slot | DSP | Character |
|------|-----|-----------|
| Warm Overdrive | Tube-style asymmetric saturation | The tres through a cranked amp — son cubano meets garage rock |
| Rhythmic Delay | Tempo-synced delay, dotted 8th or triplet | Montuno patterns cascade into rhythmic echoes |
| Bus Compression | Medium attack, musical release | Glues the tres's percussive strums into a wall of groove |
| Room Reverb | Small room, short decay | Intimate — like a Havana club at 2am |

### FX Chain B — Aunt 2 (Berimbau) "The Wildcard"

| Slot | DSP | Character |
|------|-----|-----------|
| Resonant Filter | Envelope-follower bandpass, high Q | The berimbau's gourd resonance amplified and animated |
| Granular Stutter | Micro-repeat / grain scatter, rhythm-synced | Fragments the already percussive tone — between string and drum |
| Phaser | 4-stage, moderate feedback | Undulating sweep that makes the single string sound like many |
| Spring Reverb | Medium, metallic drip character | Wet, physical, the wire vibrating through metal springs |

### FX Chain C — Aunt 3 (Charango) "The Scorekeeper"

| Slot | DSP | Character |
|------|-----|-----------|
| Shimmer Reverb | Octave-up pitch shift in reverb tail | Arpeggios ascend into crystalline shimmer — tropical light |
| Fast Chorus | Stereo chorus, quick rate, moderate depth | Ten strings become twenty — the charango multiplied |
| Bright Delay | Short delay, high feedback, HP in loop | Rapid echoes that turn arpeggios into cascading waterfalls |
| Harmonic Exciter | Parallel saturation + HP blend | Presence and sparkle — makes the charango CUT through the aunts' argument |

### Husband FX — Minimal, Meditative

| Husband | FX | Character |
|---------|----| ---------|
| Oud (Palestinian) | Tape Echo — long, warm, modulated | Meditative — each ornamental phrase echoes into calm. Trying to soothe. |
| Bouzouki (Greek) | Chorus — bright, wide, rhythmic | Accidentally energetic — the chorus makes the driving patterns MORE intense. He's helping but not helping. |
| Pin (Thai) | Soft Reverb — large hall, very gentle | Barely there — the pin disappears into a vast quiet space. He's physically present but spiritually elsewhere. |

---

## Macro System

| Macro | Name | Controls | Musical Intent |
|-------|------|----------|---------------|
| M1 | **FUEGO** | Strum intensity + attack sharpness + drive amount + playing speed | How hard the aunts play — gentle fingerpicking → fierce rasgueo → explosive percussive attack. The fire in their hands. |
| M2 | **DRAMA** | THE SIGNATURE — family argument intensity + husband emergence threshold | Peace → banter → squabble → argument → husbands dragged in → la mesa. One macro tells the entire family dinner story. |
| M3 | **SIDES** | Alliance rotation — which aunt is the odd one out | Continuously rotates the 2-vs-1 configuration. At high DRAMA, rotation becomes unstable and rapid — alliances shift mid-phrase. |
| M4 | **ISLA** | FX depth for all chains + tropical reverb character + stereo width | Dry intimate → processed → vast tropical soundscape. Crystal clear water reverb — warm, transparent, impossibly wide. |

---

## Voice Architecture

- **Max voices:** 18 — 4 per aunt (strummed chords need multiple simultaneous strings), 2 per husband
- **Voice stealing:** Oldest note per instrument — each instrument manages its own pool
- **Legato mode:** No for aunts (strummed strings don't slur — they attack). Yes for oud (fretless glide) and pin (gentle bends).
- **Strum model:** When a chord is played, individual strings within each aunt's instrument are triggered in rapid sequence (1-15ms apart) with alternating up/down strum direction. Strum speed is a performance parameter tied to FUEGO.

---

## Coupling Thesis

XOlé visits the reef from their tropical planet. They bring warmth, rhythm, and drama to the water column. Their coupling is rhythmic — the aunts' strumming patterns drive other engines' parameters with percussive energy. The husbands add unexpected cross-cultural modulation when DRAMA escalates.

### As Coupling Source (XOlé → others)

| Route | What It Sends | Partner | Musical Effect |
|-------|--------------|---------|---------------|
| `getSampleForCoupling()` | Combined family output (drama-weighted) | Any | Strummed string texture modulating other engines |
| Tres output | Rhythmic montuno pattern | ONSET | Montuno pattern drives drum parameters — salsa rhythm section |
| Berimbau output | Percussive wire hits | OVERBITE | Berimbau strikes drive bass filter — capoeira meets anglerfish |
| Charango output | Rapid arpeggios | ORPHICA (sister) | Charango arpeggios trigger harp sympathetic resonance — sisters harmonizing |
| DRAMA value | Family tension as mod source | OBBLIGATO (nephews) | Aunts' drama affects the brothers' BOND — family stress cascades |

### As Coupling Target (others → XOlé)

| Route | Source | What It Does | Musical Effect |
|-------|--------|-------------|---------------|
| `AmpToFilter` | ONSET | Drum amplitude → strum intensity | Drums drive the aunts' playing — rhythmic engine |
| `EnvToMorph` | ORPHICA | Harp envelope → instrument morph | Sister's pluck chooses which aunt's instrument rings |
| `LFOToPitch` | OBBLIGATO | Brothers' wind → aunts' pitch | Nephews' flutes make the aunts' strings waver — family vibrato |
| `AmpToFilter` | OTTONI | Cousins' brass → alliance rotation speed | When the cousins play loud, the aunts' alliances shift faster |

### Signature Pairings

| Pairing | Name | What Happens |
|---------|------|-------------|
| **ORPHICA × OLÉ** | "Sisters" | Harp and strummed strings — the goddess and the firestarters. Divine calm meets tropical fire. |
| **OBBLIGATO × OLÉ** | "Nephews & Aunts" | Wind brothers play over the aunts' argument — family jam. |
| **OTTONI × OLÉ** | "Cousins' Visit" | Cold-water brass meets tropical strings — the full extended family reunion. |
| **OLÉ → OVERDUB** | "Tropical Dub" | Strummed strings through tape delay + spring reverb — Havana meets Kingston. |
| **ONSET → OLÉ** | "Latin Percussion Section" | Drums drive the aunts' strumming patterns — full salsa rhythm section. |
| **OLÉ × OLÉ** | "Both Sides of the Table" | Two instances — six aunts, six husbands. The Thanksgiving dinner nobody escapes. |

### Unsupported Coupling Types

| Type | Why |
|------|-----|
| `AmpToChoke` | The aunts don't stop talking. Ever. Choking is not in their vocabulary. |
| `AudioToWavetable` | Waveguide-based — no wavetable target |

---

## Signature Sound

Set DRAMA to 0. Three aunts play together — tres cubano laying down a montuno pattern, berimbau adding hypnotic rhythmic counterpoint, charango sparkling with rapid arpeggios above. It's a warm, beautiful Afro-Latin ensemble. Crystal clear, like the water of their home planet. Now move SIDES to 0.5. Aunt 2 (berimbau) becomes isolated — her rhythm starts to syncopate against the pair. Tres and charango lock together, consonant, allied. The berimbau pushes back — louder, more aggressive, odd-meter patterns fighting for space. Turn up DRAMA to 0.5. Now it's a real squabble. The berimbau is playing OVER the allied pair. Intervals clash. Rhythmic patterns collide. It's still musical, still beautiful, but there's tension — you can hear the argument in the syncopation.

Push DRAMA past 0.7. The oud enters — soft, ornamental, trying to lay down a calming maqam phrase between the arguing aunts. Then the bouzouki — bright, driving, accidentally adding fuel. The pin, barely audible, plays a gentle pentatonic melody in the far corner of the stereo field. Six instruments. Four cultural traditions. Three arguments. Zero resolution. Maximum love.

At DRAMA=1.0, it's la mesa. Everyone is playing at full intensity. The tres is hammering montuno chords. The berimbau is firing wire strikes. The charango is cascading arpeggios. The oud is ornamental arabesques. The bouzouki is driving eighths. The pin is... still quietly playing in the corner. It's chaos. It's family. It's the most rhythmically complex, culturally rich, emotionally honest sound in the entire gallery. And when you pull DRAMA back to 0, they all settle down, find the groove again, and play together like nothing happened. Because that's what family does.

---

## The Tropical Planet — Design Inspiration

A crystal clear freshwater-meets-saltwater world where Afro-Latin musical traditions evolved in the warm shallows.

1. **Crystal clear acoustics** — the ISLA macro's reverb models warm, transparent water with a white sand bottom. Sound travels far and clean. No murk, no mud — just pure reflections off coral and sand.
2. **Afro-Latin rhythm** — the clave is the DNA. Whether it's son clave (3-2), rumba clave (3-2 shifted), or bossa clave (Brazilian 2-3), the rhythmic foundation of every aunt's pattern references the clave. The SIDES macro's alliance rotation creates new rhythmic relationships that all orbit the clave.
3. **Multicultural husbands** — Palestinian, Greek, and Thai string traditions represent the global diaspora that meets in tropical waters. The oud's maqam, the bouzouki's rebetiko, and the pin's pentatonic scales are musical cultures that have all interacted with Latin American music through immigration, colonialism, and cultural exchange. The husbands aren't random — they're historically resonant connections.
4. **Parrotfish ecology** — the aunts build their own paradise and argue about the results. Every preset should feel like it was built by the instruments themselves — the sound creates its own environment, then the environment shapes the sound.

---

## Visual Identity

- **Accent color:** Hibiscus `#C9377A` — tropical flower, deep magenta-pink, fiery, impossible to ignore. Like the aunts.
- **Material/texture:** Coral reef — colorful, textured, organic, built by the inhabitants. Three overlapping parrotfish scales in different colors (Aunt 1 = warm red-green, Aunt 2 = blue, Aunt 3 = rainbow iridescent).
- **Icon concept:** Three string instruments crossed in a triangle formation — tres (left), berimbau (top), charango (right). Smaller oud, bouzouki, and pin in the background, partially hidden. The triangle formation mirrors the 2-vs-1 alliance dynamic.
- **Panel character:** Three columns (one per aunt) with a triangular "alliance indicator" in the center that shows the current 2-vs-1 configuration. Below, a "husband row" that fades in as DRAMA increases. The panel should feel warm, tropical, colorful — hibiscus flowers, coral textures, crystal water reflections.

---

## Mood Affinity

| Mood | Affinity | Why |
|------|----------|-----|
| **Foundation** | High | Peace-state Afro-Latin ensemble provides warm rhythmic foundation |
| **Atmosphere** | Medium | High ISLA settings create tropical atmospheric beds |
| **Entangled** | High | The alliance system + family coupling = deeply entangled voices |
| **Prism** | Medium | Charango shimmer + chorus = prismatic brightness |
| **Flux** | High | DRAMA + SIDES sweeps create constantly shifting alliances — never static |
| **Aether** | Low | Strummed strings are too present/rhythmic for aetheric stillness |

---

## Parameter Count Estimate

| Category | Params | Examples |
|----------|--------|---------|
| Aunt 1 (Tres) | 6 | instrument variant, strum speed, strum direction, body resonance, course detune, OD amount |
| Aunt 2 (Berimbau) | 6 | wire tension, gourd size, coin press, caxixi amount, filter Q, stutter rate |
| Aunt 3 (Charango) | 6 | instrument variant, tremolo speed, arpeggio rate, body resonance, shimmer amount, chorus depth |
| Husband Voices | 4 | oud ornament density, bouzouki drive, pin volume, husband blend |
| Alliance Engine | 3 | sides position, rotation speed, alliance strength |
| Macros | 4 | FUEGO, DRAMA, SIDES, ISLA |
| Master | 3 | volume, pan, strum sync (tempo) |
| **Total** | **~32** | Compact — the alliance system provides complexity without parameter bloat |

---

## Family System Summary

| Engine | Role | Planet | Split | Paths | Macro | Story | Species |
|--------|------|--------|-------|-------|-------|-------|---------|
| **XOrphica** | Mother | Ocean (warm salt) | Register | 2 | SURFACE | Environment | Siphonophore |
| **XObbligato** | Sons | Ocean (warm salt) | Personality | 2 | BOND | Relationship | Flying fish |
| **XOttoni** | Cousins | Great Lakes (cold fresh) | Maturity | 3 | GROW | Coming of age | Lake sturgeon |
| **XOlé** | Aunts (+husbands) | Tropical (crystal clear) | Alliance | 3→6 | DRAMA | Family dynamics | Parrotfish |

Four engines. Four split philosophies. Four universal human stories. One family.

One more branch to complete the pentagon.

---

*XO_OX Designs | XOlé — three aunts, three husbands, one argument, all love*
