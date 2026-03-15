# The Culinary Collection — Overview

*Phase 0 | March 2026 | XO_OX Designs*
*Six quads. Twenty-four engines. Every essential instrument group, reframed through the kitchen.*

---

## The Idea

Every synth workstation ships the same instrument categories: organ, piano, electric piano, strings, bass, pads. They're the kitchen essentials — the tools no studio can function without.

The Culinary Collection takes these six essential instrument groups and rebuilds each as an XO_OX **quad** — four engines per group, each shaped by a culinary metaphor that determines *how* the instruments are modeled, *how* they couple, and *what new sounds emerge* when the kitchen comes alive.

This isn't a sample library with categories. Each quad is a **synthesis philosophy** applied to an instrument family. The culinary metaphor isn't decoration — it's the DSP architecture.

---

## The Six Quads

```
┌──────────────────────────────────────────────────────┐
│                THE CULINARY COLLECTION                │
│                                                      │
│   CHEF ──── Organ ──────── The Cooks (adversarial)   │
│   KITCHEN ─ Piano ──────── The Surfaces (resonant)   │
│   FUSION ── Electric Piano ─ The Crossroads (migratory) │
│   GARDEN ── Strings ────── The Ingredients (evolutionary) │
│   CELLAR ── Bass ────────── The Foundation (gravitational) │
│   BROTH ─── Pads ────────── The Medium (cooperative) │
│                                                      │
│   6 quads × 4 engines = 24 engines                   │
└──────────────────────────────────────────────────────┘
```

Each quad answers a different question about sound:

| Quad | Question | Culinary Parallel |
|------|----------|-------------------|
| Chef | *Who* is shaping it? | The cook's ego, technique, regional identity |
| Kitchen | *Where* does it resonate? | The surface — cast iron, copper, stone, glass |
| Fusion | *Why* do these sounds meet? | Trade routes, diaspora, genre collision |
| Garden | *What* is growing? | Ingredients — raw, cultivated, harvested, fermented |
| Cellar | *What* holds it up? | Stored depth — aged, pressurized, foundational |
| Broth | *How* is it transforming? | The liquid medium — simmer, reduce, infuse, clarify |

---

## Quad 1: CHEF — Organs (The Cooks)

**Status:** Fully designed (see `chef_quad_concept_brief.md`)
**Coupling mode:** Adversarial — ego, stakes, competition
**Instrument group:** Organs — reed, pipe, electric, free-reed

Four world-class seafood chefs in regional competition. Each chef brings a loadout of 4 regional organs + 4 boutique synth wildcards + 4 FX recipes = 64 configurations per chef, 256 total.

| Engine | Chef | Region | Organs |
|--------|------|--------|--------|
| **XOto** | Oto (音) | East Asia | Shō, Sheng, Khene, Melodica |
| **XOctave** | Octave | Western Europe | Cavaillé-Coll pipe, Baroque positiv, Musette accordion, Farfisa |
| **XOleg** | Oleg | Baltic/Eastern Europe | Bayan, Hurdy-gurdy, Bandoneon, Garmon |
| **XOtis** | Otis | Americas | Hammond B3, Calliope, Blues harmonica, Zydeco accordion |

**Shared organ parameter vocabulary:** Cluster density, chiff transient, unison detune, buzz intensity, pressure instability, crosstalk/leakage.

**Prefix:** `oto_`, `oct_`, `oleg_`, `otis_`

---

## Quad 2: KITCHEN — Pianos (The Surfaces)

**Status:** Concept
**Coupling mode:** Resonant — material physics, sympathetic vibration
**Instrument group:** Acoustic pianos — grand, upright, prepared, toy/celesta

The kitchen IS the instrument. Hammers hit strings inside resonant bodies — but the *surface material* determines everything about how energy transfers, how harmonics develop, how sound decays. Four materials. Four piano archetypes. Four completely different responses to the same gesture.

| Engine | Surface | Piano Archetype | Character |
|--------|---------|----------------|-----------|
| **XOven** | Cast Iron | Concert Grand | Massive thermal mass — dark, sustained, absorbs and radiates slowly. Heavy harmonic body. Retains everything. The 9-foot Steinway of synthesis. |
| **XOchre** | Copper | Upright Piano | Quick, responsive, warm conductivity. Thinner body, faster decay, brighter attack. Intimate. The piano in the room where you actually practice. |
| **XObelisk** | Stone / Marble | Prepared Piano | Cold, dense, unyielding. Objects placed on strings. Unexpected resonances. Percussive attacks with metallic, mineral overtones. John Cage's kitchen counter. |
| **XOpaline** | Glass / Porcelain | Toy Piano / Celesta | Fragile, crystalline, rings when struck. Narrow bandwidth, pure partials, quick decay. Beautiful and breakable. The teacup you play with a spoon. |

**Coupling behavior:** Resonant — when Kitchen engines couple, they transfer energy through sympathetic vibration. XOven's massive body excites XOpaline's fragile partials. XObelisk's prepared objects rattle against XOchre's copper warmth. The coupling IS the physics of materials touching.

**Shared parameter vocabulary:**
- **Hammer hardness** — felt to steel (attack character)
- **Body resonance** — how much the enclosure sings
- **Sympathetic ring** — other strings ringing in sympathy
- **Damper behavior** — sustain pedal physics (lift, half, full)
- **Material density** — affects harmonic series weighting
- **Surface temperature** — metaphor for tonal warmth (cold marble → warm copper)

**Prefix:** `oven_`, `ochre_`, `obel_`, `opal2_`

---

## Quad 3: CHOIR — Voices (The Table)

**Status:** Concept
**Coupling mode:** Conversational — call and response, harmony, counterpoint, unison
**Instrument group:** Voices — sacred, operatic, percussive, harmonic

The table is where the food is served and the meal becomes an occasion. Food without people is inventory. People without food is just a room. The table is the human element — conversation, toasts, grace, singing, laughter, the voices that transform ingredients into memories. Voices are the most fundamental instrument. Every culture sings before it builds a piano or stretches a string. Choir is the table where the Culinary Collection gathers.

| Engine | Table Role | Voice Archetype | Character |
|--------|-----------|----------------|-----------|
| **XOrison** | Grace / blessing | Sacred Choir | Reverent, blended, cathedral. Many voices becoming one. The prayer before the meal — gratitude made audible. From solo cantor to massed choir, the sound of devotion across every tradition. Orison = a prayer. |
| **XOpera** | The toast | Operatic Voice | Dramatic, solo, projecting. One voice filling the room. The toast that silences the table — grand, emotional, commanding. Soprano to bass, bel canto to sprechstimme. Every emotion at full volume. |
| **XOrate** | Kitchen calls | Beatbox / Vocal Percussion | "Order up!" "Behind!" "Yes, Chef!" The rhythmic vocal percussion of a working kitchen. Mouth as instrument, body as drum. Beatbox, mouth percussion, tongue clicks, breath hits — the voice stripped of melody and rebuilt as rhythm. |
| **XOvertone** | The hum | Throat Singing / Harmonics | The low conversation underneath everything. One voice containing multitudes — Tuvan khoomei, Sardinian tenore, Tibetan chant. The background hum of a full restaurant. Fundamentals so rich that harmonics emerge as separate melodies. |

**Coupling behavior:** Conversational — when Choir engines couple, they enter dialogue. XOrison's choir responds to XOpera's solo like a congregation answering a cantor. XOrate's rhythms lock to XOvertone's drone like table percussion over ambient conversation. The coupling IS dinner table conversation — voices weaving around each other, sometimes in harmony, sometimes in argument, always in relationship.

**Shared parameter vocabulary:**
- **Breath** — air in the voice (breathy whisper → pure tone → pressed shout)
- **Vowel shape** — formant position (ah → ee → oo → oh → mm)
- **Vibrato** — the human tremor (none → gentle → operatic → extreme)
- **Ensemble size** — solo → quartet → choir → crowd → stadium
- **Diction** — consonant clarity (mumbled → crisp → percussive)
- **Register** — bass → baritone → tenor → alto → soprano (continuous, not stepped)

**Prefix:** `oris_`, `opera_`, `orate_`, `overt_`

---

## SECRET QUAD: FUSION — Electric Pianos (The 5th Slot)

**Status:** Concept — Secret unlock mechanic
**Coupling mode:** Migratory — cultural collision, genre-crossing, hybrid identity
**Instrument group:** Electric pianos — tine, reed, pickup, FM
**Access:** Unlocked as a **5th engine slot** when all 4 Kitchen engines are loaded

Electric pianos exist because genres collided. Jazz needed amplification. Funk needed percussive keys. Pop needed shimmer. R&B needed warmth-at-volume. Every EP is a fusion instrument — acoustic principles married to electric amplification, creating sounds that belong to no single tradition. The crossroads where blues met jazz met soul met pop met electronic.

**You can't do fusion cooking without a fully equipped kitchen.** Load all 4 Kitchen surfaces (Cast Iron, Copper, Stone, Glass) into the coupling matrix and a 5th slot appears — the Fusion slot. The kitchen is complete, the chef is ready, and now fusion cuisine becomes possible. This is XOmnibus's prestige configuration: commitment to the Kitchen quad rewards you with a secret instrument group that couples with all 4 surfaces simultaneously.

*(See "The 5th Slot — Fusion Unlock Mechanic" section below for full details.)*

| Engine | Cuisine Influence | EP Archetype | Character |
|--------|-------------------|-------------|-----------|
| **XOasis** | Spice Route (East→West) | Rhodes / Tine EP | Warm bell-tones. Tine struck by hammer, amplified by pickup. The sound of every genre borrowing from every other. Smooth enough for jazz, gritty enough for neo-soul, shimmering enough for lo-fi. |
| **XOddfellow** | Night Market (street food fusion) | Wurlitzer / Reed EP | Reedy, gritty, intimate. Reed vibrating near a pickup — raw, slightly distorted, full of character. The busker instrument. Sounds best slightly broken. |
| **XOnkolo** | West African → diaspora | Clavinet / Pickup Keys | Funky, percussive, string-slap energy. Strings struck and picked up magnetically. The instrument Stevie Wonder and Bernie Worrell used to bridge continents. Named for the nkolo (a Central African thumb piano ancestor). |
| **XOpcode** | Silicon Valley → Tokyo | DX / FM Electric Piano | Crystalline, digital, impossibly clean. FM synthesis creating bell-like tones that don't exist in nature. The sound of the 1980s imagining the future. Algorithm as recipe. |

**Coupling behavior:** Migratory — when Fusion engines couple, sounds adopt characteristics of each other's tradition. XOasis absorbs XOnkolo's percussive attack; XOpcode inherits XOddfellow's reed warmth. The coupling is cultural exchange — each engine returns from the encounter carrying something new.

**5th-slot coupling:** When loaded in the Fusion slot, the EP engine couples with ALL 4 Kitchen surfaces simultaneously. XOasis on cast iron AND copper AND stone AND glass at once. The coupling weights are controlled by the Kitchen engines' positions in the matrix — each surface contributes its character proportionally.

**Shared parameter vocabulary:**
- **Pickup position** — where the transducer sits relative to the vibrating element
- **Drive stage** — preamp character (clean tube → broken transistor)
- **Chorus depth** — the universal EP effect (ensemble shimmer)
- **Velocity curve** — how hard you hit determines not just volume but timbre
- **Key-off noise** — the mechanical sound of release (damper, key return)
- **Era dial** — continuous morph through decades of EP evolution

**Prefix:** `oasis_`, `oddf_`, `onko_`, `opco_`

---

## Quad 4: GARDEN — Strings (The Ingredients)

**Status:** Concept
**Coupling mode:** Evolutionary — growth, cultivation, seasonal change
**Instrument group:** Strings — orchestral, solo, chamber, synth

The garden is where raw ingredients grow. Strings are the most *alive* of instrument families — they vibrate, they breathe, they respond to touch with infinite gradation. A bowed string is a cultivated plant — sustained, nurtured, shaped over time. A plucked string is a harvest — one decisive gesture, then decay. Strings can be raw (open air, solo) or cultivated (orchestral, layered, blended into something greater than any single voice).

| Engine | Garden Zone | String Archetype | Character |
|--------|------------|-----------------|-----------|
| **XOrchard** | Orchard (cultivated trees) | Orchestral Strings | Lush, blended, the sound of 60 players breathing together. Cultivated over centuries of tradition. The most "cooked" string sound — layers of rosin, vibrato, ensemble blend. |
| **XOvergrow** | Wild Garden (untamed growth) | Solo Strings | Raw, exposed, every imperfection audible. One voice, no ensemble to hide behind. Finger slides, bow scratch, rosin dust. The ingredient before the chef touches it. |
| **XOsier** | Herb Garden (woven, flexible) | Chamber Strings | Intimate, intertwined. 4-8 players, each voice audible but woven together like willow branches. Osier = willow used for basket-weaving. The string quartet's conversation. |
| **XOxalis** | Geometric Garden (pattern, structure) | Synth Strings | Filtered, phased, stacked. The string sound that never existed acoustically — Solina, Elka, JP-8 pads. Geometric patterns of detuned sawtooths. Oxalis = clover-like plant with geometric leaf patterns. |

**Coupling behavior:** Evolutionary — when Garden engines couple, they grow into each other over time. XOvergrow's solo voice gradually develops XOrchard's ensemble lushness. XOxalis's synthetic patterns slowly absorb XOsier's organic weaving. The coupling IS seasonal change — sound evolves from raw seed to full bloom across the duration of a note, a phrase, a session.

**Shared parameter vocabulary:**
- **Bow pressure** — weight on string (light harmonic → heavy grunt)
- **Vibrato rate/depth** — the human pulse in the sound
- **Rosin texture** — surface friction character
- **Section size** — 1 voice → 60 voices (solo → orchestral continuum)
- **Growth rate** — how quickly the evolutionary coupling develops
- **Season** — metaphor for timbral lifecycle (spring/bright → winter/dark)

**Prefix:** `orch_`, `grow_`, `osier_`, `oxal_`

---

## Quad 5: CELLAR — Bass (The Foundation)

**Status:** Concept
**Coupling mode:** Gravitational — weight, pull, everything settles toward it
**Instrument group:** Bass — sub, analog, acoustic, FM

Every kitchen has a cellar. It's underneath. It's dark. It stores what everything else rests on — the aged wine, the cured meat, the root vegetables, the fermented essentials. You don't see the cellar, but you'd notice instantly if it disappeared. Bass is the cellar of music — foundational, felt more than heard, the gravity that pulls everything else into coherence.

| Engine | Cellar Stock | Bass Archetype | Character |
|--------|-------------|---------------|-----------|
| **XOgre** | Root Vegetables (massive, earthy) | Sub Bass | Below hearing, felt in the body. Pure weight. The potatoes and turnips of sound — unglamorous, essential, the foundation everything else is built on. Sine waves and filtered noise at the bottom of human perception. |
| **XOxbow** | Aged Wine (patient, complex) | Analog Bass | Warm, fat, developed over decades of circuit evolution. Moog, TB-303, SH-101 — each a vintage with terroir. The oxbow is the slow river bend where sediment collects, where depth develops through patience. Resonant filter as flavor profile. |
| **XOaken** | Cured Wood (resonant, deep) | Acoustic / Upright Bass | Wooden body, gut or steel strings, the sound of hands on an instrument. Standup jazz bass, orchestral double bass, folk bass. The oak barrel that shapes the wine. Physical, warm, human. |
| **XOmega** | Distillation (reduced to essence) | FM / Digital Bass | Pure, concentrated, mathematically precise. FM synthesis reducing complex harmonics to their essence. DX bass, Reese bass, metallic sub. Omega = the end, the final reduction. Distilled to maximum potency. |

**Coupling behavior:** Gravitational — when Cellar engines couple with any other quad, they pull the sound downward. Not destructively — gravitationally. XOgre beneath XOpaline's glass piano gives the crystalline tone a foundation it never had. XOxbow beneath XOasis's Rhodes adds analog warmth to digital shimmer. Everything settles toward the cellar.

**Shared parameter vocabulary:**
- **Sub weight** — how much energy lives below 80Hz
- **Saturation** — harmonic density from gentle warmth to aggressive overdrive
- **Filter cutoff** — the bass player's right hand (where you pluck/pick)
- **Glide time** — portamento, the slide between notes
- **Octave layer** — sub-octave doubling depth
- **Age** — metaphor for harmonic complexity (young/simple → aged/complex)

**Prefix:** `ogre_`, `oxbow_`, `oaken_`, `omega_`

---

## Quad 6: BROTH — Pads (The Medium)

**Status:** Concept (evolves from Water Quad — see `water_quad_concept_brief.md`)
**Coupling mode:** Cooperative — physics, no ego, transformation through immersion
**Instrument group:** Pads / Atmosphere — evolving, ambient, textural

The broth is the liquid medium everything else cooks in. It's not an ingredient — it's the *environment*. A good broth transforms everything immersed in it. A pad does the same thing in music — it's the atmospheric bed that changes the character of every sound played over it. The broth is patient. It simmers. It reduces. It infuses. It clarifies.

The four engines map to four culinary liquid processes, each operating on a different time scale — inheriting the Water Quad's temporal framework.

| Engine | Liquid Process | Pad Archetype | Time Scale | Character |
|--------|---------------|--------------|------------|-----------|
| **XOverwash** | Infusion (flavor bleeds slowly) | Diffusion Pad | Seconds | Slow spectral crossfade — one timbre bleeds into another like watercolor. Tea steeping in hot water. The pad that gradually absorbs the color of whatever is played over it. |
| **XOverworn** | Reduction (boiled down over hours) | Erosion Pad | Session-long | Imperceptible spectral theft — partials removed so slowly you don't notice until the sound is fundamentally different. Stock reducing on low heat for eight hours. The pad that was bright when you started playing and is dark when you stop. |
| **XOverflow** | Pressure Cooking (sealed, builds, releases) | Pressure Pad | Phrases | Energy accumulates until it bursts. Sealed harmonics compress, saturate, then the valve opens and everything erupts. The pressure cooker whistle. The pad that builds tension phrase by phrase until release. |
| **XOvercast** | Flash Freeze (instant state change) | Crystallization Pad | Instant | One trigger transforms everything — liquid to solid, warm to frozen, moving to still. Flash-freezing broth into ice. The pad that captures a moment and locks it in crystal. |

**Coupling behavior:** Cooperative — when Broth engines couple, they transform each other through natural physics. No ego, no competition — just the inevitable result of two liquids mixing. XOverwash bleeds into XOverworn. XOverflow pressurizes XOvercast. The coupling IS the cooking.

**Shared parameter vocabulary:**
- **Temperature** — rate of transformation (cold/slow → hot/fast)
- **Density** — thickness of the atmospheric bed
- **Clarity** — transparent broth → opaque stew
- **Simmer rate** — LFO-like periodic bubbling in the texture
- **Immersion depth** — how deeply coupled sounds are affected
- **Seasoning** — subtle harmonic additions (like adding salt — small input, large perceptual change)

**Prefix:** `wash_`, `worn_`, `flow_`, `cast_`

---

## The 5th Slot — Fusion Unlock Mechanic

XOmnibus has 4 engine slots. Always has. The Culinary Collection introduces a **secret 5th slot** — the Fusion slot — that appears only under a specific condition:

### How to Unlock

```
SLOT 1: XOven    (Cast Iron)    ─┐
SLOT 2: XOchre   (Copper)       │ ALL 4 KITCHEN ENGINES LOADED
SLOT 3: XObelisk (Stone)        │
SLOT 4: XOpaline (Glass)        ─┘
                                 ↓
         ╔═══════════════════════════╗
         ║  SLOT 5: FUSION UNLOCKED  ║
         ║  Choose: XOasis           ║
         ║          XOddfellow       ║
         ║          XOnkolo          ║
         ║          XOpcode          ║
         ╚═══════════════════════════╝
```

**The rule:** Load all 4 Kitchen surfaces into the coupling matrix. When the last Kitchen engine drops into place, the 5th slot fades in — a new position in the UI that wasn't there before. Select one of the 4 Fusion engines to fill it.

### Why This Works

**Thematically:** You can't do fusion cooking without a fully equipped kitchen. Master all four surfaces — cast iron, copper, stone, glass — and you've earned the right to attempt fusion. Every great fusion chef first mastered classical technique. The 5th slot IS that mastery reward.

**Musically:** The Fusion engine couples with ALL 4 Kitchen surfaces simultaneously. XOasis's Rhodes bell-tones processed through cast iron's mass AND copper's responsiveness AND stone's cold resonance AND glass's fragility at once. Five-way coupling. No other configuration in XOmnibus can do this.

**As discovery mechanic:** Players who load a full Kitchen quad discover the unlock organically. No tutorial tells them. The slot just appears. Word spreads through the community: "Load all 4 Kitchen engines and see what happens." This is the XOmnibus equivalent of a fighting game's hidden character — a reward for commitment that deepens the experience for those who find it.

### 5th Slot Coupling Rules

- The Fusion engine occupies a new 5th position in the MegaCouplingMatrix
- It can send/receive modulation to/from all 4 Kitchen engines
- Coupling type with Kitchen is always **Plate** (Kitchen × Fusion's native coupling verb)
- The Fusion engine CANNOT couple with non-Kitchen engines in this configuration (it only exists BECAUSE the Kitchen is complete)
- All 4 Fusion FX recipes (Dashi, Mole, Jus, Chimichurri) remain available
- Wildcards for the Fusion engine function normally

### The Math

Normal configuration: 4 engines × 4 voices × 4 FX × 4 wildcards = **1,024 configurations**
Full Kitchen + Fusion: 4 Kitchen engines (locked) × 4 Fusion voices × 4 FX × 4 wildcards × 5-way coupling = **64 Fusion configurations layered over the locked Kitchen**

This is the prestige mode. The reward for commitment. The secret menu.

### UI Behavior

- Before unlock: 4 engine slots visible, normal operation
- During unlock: When the 4th Kitchen engine is loaded, the 5th slot animates in with a 500ms fade. A subtle XO Gold shimmer sweeps across the interface — the visual equivalent of the kitchen coming alive
- The slot position: Below or beside the 4 standard slots (never displaces them)
- If any Kitchen engine is removed, the 5th slot fades out (100ms). The Fusion engine is gracefully crossfaded to silence before removal
- Preset recall: Presets that use the 5th slot auto-load all 4 Kitchen engines + the Fusion engine. The unlock is implicit in the preset — users who load a Fusion preset discover the mechanic through preset browsing

---

## Cross-Quad Coupling: The Full Kitchen

The Culinary Collection's power isn't in individual quads — it's in what happens when the kitchen operates as a whole. Six coupling modes create 15 unique quad-pair interactions:

### Coupling Mode Matrix (6 Standard Quads)

```
         CHEF    KITCHEN  CHOIR    GARDEN   CELLAR   BROTH
CHEF      —      Temper   Chant    Harvest  Ferment  Poach
KITCHEN   —       —       Announce Press    Cure     Steam
CHOIR     —       —        —       Serenade Toast    Hymn
GARDEN    —       —        —        —       Root     Steep
CELLAR    —       —        —        —        —       Dissolve
BROTH     —       —        —        —        —        —
```

**New Choir coupling verbs:**
- **Chef × Choir (Chant):** The cook sings while cooking. Organ drones beneath choral voices — liturgical, meditative, the hum of a working kitchen.
- **Kitchen × Choir (Announce):** "Order up!" Voice calls over piano surfaces. Percussive vocal attacks triggering surface resonances.
- **Choir × Garden (Serenade):** Voice over strings. The most ancient pairing in music. Lieder, opera arias, folk songs — voice and string in conversation.
- **Choir × Cellar (Toast):** Voice over bass. The toast — one voice rising over the low hum of the room. Call and response between registers.
- **Choir × Broth (Hymn):** Voice immersed in atmosphere. Cathedral acoustics, ambient choir, the voice dissolved into reverberant space.

**Fusion coupling:** When the 5th slot is active, Fusion couples with all 4 Kitchen engines via the **Plate** verb. Fusion does NOT appear in the standard matrix — it exists outside the grid, connected only through its Kitchen prerequisite.

```
         ┌─── KITCHEN ───┐
         │  Oven  Ochre   │
FUSION ──┤                ├── (Plate coupling to all 4)
         │  Obelisk Opaline│
         └────────────────┘
```

### Signature Cross-Quad Behaviors

**Chef × Kitchen (Temper):** The cook meets the surface. Oto's shō on XOven's cast iron — the cluster sustain takes on massive thermal resonance. Otis's Hammond on XOpaline's glass — tonewheel warmth through crystalline fragility.

**Chef × Broth (Poach):** The cook meets the liquid. Oleg's bayan immersed in XOverwash — the bellows diffuse into watercolor. Octave's pipe organ pressurized by XOverflow — builds until the cathedral explodes.

**Kitchen × Cellar (Cure):** Surface meets foundation. XObelisk's prepared piano on XOgre's sub — stone percussion with seismic undertow. XOchre's copper on XOxbow's analog — warm on warm, maple syrup depth.

**Garden × Broth (Steep):** Ingredient meets liquid. XOvergrow's raw solo string steeped in XOverworn — the vibrato erodes imperceptibly over minutes. XOxalis's synth strings infused by XOverwash — geometric patterns dissolving into organic flow.

**Choir × Garden (Serenade):** Voice meets string. XOpera's operatic voice over XOrchard's orchestral strings — the oldest pairing in Western music. XOvertone's harmonics woven into XOsier's chamber texture — overtone singing becomes the viola's ghost.

**Choir × Cellar (Toast):** Voice meets foundation. XOrate's beatbox over XOgre's sub — mouth percussion with seismic weight. XOrison's choir over XOxbow's analog bass — hymn with analog gravity.

**Cellar × Broth (Dissolve):** Foundation meets medium. XOgre's sub dissolved in XOvercast — sub-bass flash-frozen into crystalline structure. XOmega's FM bass reduced by XOverworn — digital precision slowly eroded into analog warmth.

---

## Five Signature Confluences

When specific engines from different quads align, special behaviors emerge — the Culinary Collection's equivalent of the Water Quad's locked Confluences. Two of these (Omakase, Mole) require the 5th Slot Fusion unlock — they're prestige configurations.

### 1. "Sunday Gravy" — XOtis × XOven × XOxbow × XOverworn
*Hammond B3 on cast iron, over analog bass, reducing all day.*

Session-long preset. Starts as bright tonewheel funk over fat Moog bass. Over 20 minutes, XOverworn imperceptibly steals harmonics from both. By the end, you have a dark, thick, reduced texture that sounds nothing like where it started — but every moment of the transition was musically valid. Sunday gravy takes eight hours. This takes twenty minutes. Worth every second.

### 2. "Omakase" — XOto × XOpaline × XOpcode × XOvercast *(Requires 5th Slot)*
*Shō cluster through glass, with FM EP shimmer, flash-frozen.*

The chef's choice — and the ultimate prestige confluence. This one REQUIRES the Fusion unlock: XOpaline is one of the 4 Kitchen engines, and XOpcode is a Fusion engine in the 5th slot. But it also uses XOto (Chef) and XOvercast (Broth), which means two of the Kitchen slots are occupied by non-Kitchen engines... unless this is a special 5-engine confluence: all 4 Kitchen engines + XOpcode in slot 5, with XOto and XOvercast coupled from outside the Collection. The chef's choice indeed — the most complex configuration in the entire system. Oto's aitake cluster resonates through the full kitchen. XOpcode adds FM shimmer from the Fusion slot. XOvercast flash-freezes. No two triggers sound the same.

### 3. "Fermentation" — XOleg × XObelisk × XOaken × XOverflow
*Bayan on stone, over upright bass, pressure building.*

Oleg's bellows push air across XObelisk's cold marble. XOaken's wooden body resonates below. XOverflow seals the system — pressure builds phrase by phrase. Harmonics compress, distortion accumulates, everything gets tighter and hotter until the valve opens and the fermented release is rich, complex, and slightly dangerous. Controlled rot as art form.

### 4. "Mole" — XOctave × XOchre × XOnkolo × XOxalis *(Requires 5th Slot)*
*Baroque organ on copper, with clavinet funk, through synth string sauce.*

Thirty ingredients. Three days. One sauce. Another prestige confluence requiring the Fusion unlock — XOnkolo (Clavinet) lives in the 5th slot, coupled into the full Kitchen where XOchre's copper is one of the 4 loaded surfaces. Octave's baroque chiff transients hit copper's responsive surface. XOnkolo adds rhythmic clavinet funk through the Fusion slot's 5-way coupling. XOxalis wraps everything in filtered synth-string texture — the mole sauce that somehow tastes like chocolate and chili and smoke and fruit all at once. The coupling that takes the longest to tune but rewards patience with sounds nothing else can produce.

### 5. "Grace" — XOrison × XOrchard × XOaken × XOverwash *(NEW — Choir Confluence)*
*Sacred choir over orchestral strings, upright bass below, diffusion washing through.*

The dinner prayer. XOrison's blended choir voice hovers over XOrchard's orchestral strings — the most ancient pairing in Western music. XOaken's upright bass provides the wooden, warm foundation (the table itself). XOverwash slowly infuses everything — the choir's vowels bleed into the strings, the strings absorb the bass's warmth, and over time the four voices become one gently pulsing atmosphere. The meal hasn't started yet. Everyone's eyes are closed. This is gratitude made audible.

---

## Engine Summary Table

### Standard Engines (24 — available in normal 4-slot mode)

| # | Quad | Engine | Instrument | Prefix | Accent Color (TBD) |
|---|------|--------|-----------|--------|---------------------|
| 1 | Chef | XOto | Shō / Sheng / Khene / Melodica | `oto_` | — |
| 2 | Chef | XOctave | Pipe / Positiv / Musette / Farfisa | `oct_` | — |
| 3 | Chef | XOleg | Bayan / Hurdy-gurdy / Bandoneon / Garmon | `oleg_` | — |
| 4 | Chef | XOtis | Hammond / Calliope / Harmonica / Zydeco | `otis_` | — |
| 5 | Kitchen | XOven | Concert Grand (Cast Iron) | `oven_` | — |
| 6 | Kitchen | XOchre | Upright Piano (Copper) | `ochre_` | — |
| 7 | Kitchen | XObelisk | Prepared Piano (Stone) | `obel_` | — |
| 8 | Kitchen | XOpaline | Toy Piano / Celesta (Glass) | `opal2_` | — |
| 9 | Choir | XOrison | Sacred Choir | `oris_` | — |
| 10 | Choir | XOpera | Operatic Voice | `opera_` | — |
| 11 | Choir | XOrate | Beatbox / Vocal Percussion | `orate_` | — |
| 12 | Choir | XOvertone | Throat Singing / Harmonics | `overt_` | — |
| 13 | Garden | XOrchard | Orchestral Strings | `orch_` | — |
| 14 | Garden | XOvergrow | Solo Strings | `grow_` | — |
| 15 | Garden | XOsier | Chamber Strings | `osier_` | — |
| 16 | Garden | XOxalis | Synth Strings | `oxal_` | — |
| 17 | Cellar | XOgre | Sub Bass | `ogre_` | — |
| 18 | Cellar | XOxbow | Analog Bass | `oxbow_` | — |
| 19 | Cellar | XOaken | Acoustic / Upright Bass | `oaken_` | — |
| 20 | Cellar | XOmega | FM / Digital Bass | `omega_` | — |
| 21 | Broth | XOverwash | Diffusion Pad | `wash_` | — |
| 22 | Broth | XOverworn | Erosion Pad | `worn_` | — |
| 23 | Broth | XOverflow | Pressure Pad | `flow_` | — |
| 24 | Broth | XOvercast | Crystallization Pad | `cast_` | — |

### Secret Engines (4 — 5th slot, requires Full Kitchen unlock)

| # | Quad | Engine | Instrument | Prefix | Accent Color (TBD) |
|---|------|--------|-----------|--------|---------------------|
| 25 | Fusion | XOasis | Rhodes / Tine EP | `oasis_` | — |
| 26 | Fusion | XOddfellow | Wurlitzer / Reed EP | `oddf_` | — |
| 27 | Fusion | XOnkolo | Clavinet / Pickup Keys | `onko_` | — |
| 28 | Fusion | XOpcode | DX / FM EP | `opco_` | — |

**28 total engines: 24 standard + 4 secret.**

---

## Relationship to Existing Quads

The Culinary Collection absorbs and extends the existing quad concepts:

- **Chef Quad** → Culinary Collection Quad 1 (unchanged, fully designed)
- **Water Quad** → Evolves into **Broth Quad** (Quad 6). Same four engines (Overwash, Overworn, Overflow, Overcast), same time-scale framework, but now explicitly mapped to the Pad/Atmosphere instrument group and reframed as liquid cooking processes rather than raw water phenomena.
- **Botanical Quad** (previously discussed) → The botanical concepts (ferment, reduce, infuse) are distributed across the Garden and Broth quads rather than forming a standalone quad. Growth lives in Garden (strings). Liquid transformation lives in Broth (pads). This avoids a phenomena-only quad with no instrument anchor.
- **Fusion Quad** → Promoted from standard quad to **secret 5th-slot unlock**. EP instruments were always fusion instruments by nature — acoustic principles married to electric amplification. Making them a secret reward for committing to a full Kitchen loadout is thematically perfect: you can't do fusion cooking without mastering the fundamentals first.
- **Choir Quad** → **NEW** — fills the voice-shaped hole in the instrument taxonomy. Organ, piano, strings, bass, pads were all covered — but no voices. The Table is where the meal becomes an occasion.

The `dual_quad_relationships.md` Confluences (Crank the Serpent, Aitake Dissolves, Yellowstone Glass, Cathedral Inhales) remain valid — they describe cross-quad coupling with existing XOmnibus engines (Ouroboros, Oceanic, Obsidian, Opal). The four new Culinary Confluences (Sunday Gravy, Omakase, Fermentation, Mole) are internal to the Collection.

---

## The 4×4×4 Combinatorial Framework

Every quad follows the same architecture established by the Chef Quad:

```
         ENGINE
        /  |  \
   VOICE   FX   WILDCARD
   (1of4) (1of4) (1of4)
```

**Per engine:** 4 voices × 4 FX recipes × 4 wildcards = **64 configurations**
**Per quad:** 4 engines × 64 = **256 configurations**
**Standard collection:** 6 quads × 256 = **1,536 configurations before coupling**
**With Fusion unlock:** + 4 Fusion engines × 64 = **+256 secret configurations (1,792 total)**

### What Each Axis Does

- **VOICE** — The instrument variant. In Chef Quad these are organs; in Kitchen they're piano types; in Cellar they're bass modes. Locked per engine. The "ingredient."
- **FX RECIPE** — The quad's signature processing philosophy, expressed as 4 culinary techniques. Each engine in the quad interprets the same 4 recipe names through its own character. The "cuisine."
- **WILDCARD** — A specific boutique synth company's character injected as a modulation/texture layer. 4 products per company. The "spice."

### Wildcard Philosophy

The wildcard isn't a preset — it's a *coloring agent*. When you select "OP-1" as Oto's wildcard, it doesn't turn the Shō into an OP-1. It applies OP-1's *character* — its lo-fi sampling grain, its quirky sequencing artifacts, its cassette-deck warmth — as a processing and modulation layer over the organ voice. The instrument stays itself; the wildcard changes *how it's perceived*.

**Each company appears exactly once across all 28 engines. No repeats.**

---

## FX Recipes — The Third Axis

Each quad has 4 named FX recipes — culinary techniques that determine the processing chain. Every engine in the quad shares the same 4 recipe *names*, but interprets them through its own instrument character, regional identity, and wildcard coloring.

The recipe is the **final stage** of the signal path:

```
VOICE (what you hear) → WILDCARD (how it's colored) → FX RECIPE (how it's processed)
```

A recipe isn't a single effect — it's a **philosophy of processing** expressed as a chain of 2-4 effects in a specific order with specific character. The same recipe name produces different results on different engines because the source material and wildcard coloring are different.

---

### Chef Recipes: COOKING TECHNIQUES

The chef's cuisine. Four approaches to heat and preparation that define regional cooking identity.

| Recipe | Technique | FX Chain | Character |
|--------|-----------|----------|-----------|
| **Raw** | Sashimi / crudo / tartare | Light EQ → transparent compressor → air reverb | Minimal intervention. Reveals the instrument's true voice. No hiding — every detail of the organ is exposed. The recipe that separates great voices from adequate ones. |
| **Grilled** | Direct flame, char, Maillard | Tube saturation → cabinet simulation → short room | Direct heat. Aggressive harmonics from saturation, body from cabinet modeling, space from a tight room. Smoky, charred, decisive. The sound of commitment. |
| **Braised** | Low heat, covered, liquid | Long warm reverb → gentle compression → low-pass filter | Slow, enveloped, patient. Everything softens and merges. The organ loses its edges and becomes part of the sauce. Sunday afternoon energy. |
| **Fermented** | Time, culture, controlled decay | Chorus → phaser → light distortion → medium delay | Living movement. The chorus creates bacterial shimmer, the phaser adds slow cycling instability, the distortion adds acid bite, the delay adds echo-memory. Controlled rot. Kimchi, kvass, kombucha — alive and unpredictable. |

**Per-chef interpretation:**

| | Raw | Grilled | Braised | Fermented |
|---|---|---|---|---|
| **Oto** | Surgical precision. Sashimi-grade clarity. The cut is everything. | Yakitori — quick, precise char over binchotan. Clean smoke. | Ramen broth — 12-hour pork bone. Milky, deep, umami. | Miso paste — years of patience. Quiet complexity. |
| **Octave** | Plateau de fruits de mer — pristine, arranged, classical presentation. | Steak frites — confident, no-nonsense, perfectly executed. | Cassoulet — heavy, rich, layered over hours. | Roquefort — cave-aged, pungent, refined decay. |
| **Oleg** | Zakuski — cold table, clean, ceremonial. Vodka clarity. | Shashlik — open flame, primal, outdoor. | Borscht — beet-deep, slow, soulful. | Kvass — bread ferment, industrial-sacred, ancient process. |
| **Otis** | Raw oyster bar — Gulf coast, lemon, nothing to hide. | Memphis BBQ — low and slow over hickory, smoke ring. | Gumbo — roux-dark, everything in the pot, hours of love. | Hot sauce — vinegar ferment, pepper mash, capsaicin burn. |

---

### Kitchen Recipes: HEAT METHODS

How the surface is heated. Four ways energy transfers through material into sound.

| Recipe | Heat Method | FX Chain | Character |
|--------|-----------|----------|-----------|
| **Sear** | Maximum heat, brief contact | Fast-attack limiter → hard-clip saturation → bright short reverb | Impact. All energy concentrated into the first milliseconds. The piano hammer at maximum velocity, bouncing off the surface instantly. Attack-forward, explosive decay. |
| **Bake** | Surrounded by even, dry heat | Warm convolution reverb → gentle saturation → mid-focused EQ | Enveloped. Heat from all sides, even development, golden crust. The piano in a concert hall — surrounded by warmth, every note given time to develop evenly. |
| **Smoke** | Indirect heat, flavored by fuel source | Convolution reverb (wood room IR) → tape saturation → gradual low-pass | Hazy. The sound arrives through a veil of smoke. Indirect, mysterious, the fuel (oak, cherry, mesquite) determines the flavor. Each surface material produces different smoke character. |
| **Torch** | Precise, directed, crème brûlée | Narrow band-pass sweep → focused distortion → dry output | Surgical intensity. One specific frequency range caramelized while everything else stays raw. The chef's torch on the sugar crust — precision destruction that creates beauty. |

**Per-surface interpretation:**

| | Sear | Bake | Smoke | Torch |
|---|---|---|---|---|
| **XOven** (Cast Iron) | Blackened. Screaming hot iron, instant crust. Maximum Maillard. | Slow roast. Even heat radiating from massive thermal mass. | Smoked brisket. 14 hours, oak wood, bark formation. | Impossible — you don't torch cast iron. Instead: concentrated heat spot, glowing red. |
| **XOchre** (Copper) | Flash sauté. Copper responds instantly — fastest sear possible. | Copper bake — even heat, responsive, French pastry precision. | Stovetop smoking in a copper pot. Delicate, refined smoke. | Flambé — brandy ignited in a copper pan. Dramatic, brief, transformative. |
| **XObelisk** (Stone) | Pizza stone at 900°F. Explosive bottom-heat, leopard char. | Tandoor — clay oven, radiant stone heat, naan and tikka. | Stone pit. Polynesian imu, underground smoke. Ancient. | Lava rock grilling. Torch meets stone, sparks fly. |
| **XOpaline** (Glass) | Glass crack. Thermal shock — beautiful and dangerous. | Kiln firing. Glass in the oven, slowly transforming. | Blown glass with smoke trapped inside. | Glass torch — lampwork. The art of melting glass into shape. |

---

### Choir Recipes: SERVICE STYLES

How the meal is presented. Four front-of-house philosophies that determine how the voice reaches the listener.

| Recipe | Service | FX Chain | Character |
|--------|---------|----------|-----------|
| **Tableside** | Prepared at your table, intimate, personal | Close room reverb → gentle compression → presence EQ boost | Right there. No distance. You can hear the breath, the lip sounds, the humanity. Caesar salad tossed in front of you. The voice at arm's length — intimate, unmediated, every detail audible. |
| **Banquet** | Grand hall, formal service, silver and crystal | Large hall reverb → soft compression → warm low-mid EQ | The wedding reception. Formal, spacious, every voice echoing in the grand room. The toast that reaches 200 guests. Silver service, white tablecloths, the sound of occasion. |
| **Tapas** | Small plates, many courses, casual, social | Short slapback delay → chorus → bright EQ → tight room | Quick, varied, social. Barcelona energy. The voice darting between conversations, picking up fragments, alive with movement. Each phrase a small plate — distinct, flavorful, gone before you're ready. |
| **Kaiseki** | Multi-course, seasonal, each plate a poem | Granular processing → spectral freeze → evolving reverb → randomized gentle filter | You don't choose — the chef presents what the moment demands. Each note is its own course, prepared and presented with ritual precision. Unpredictable, surprising, trust-based. The recipe that changes every time because the season changed. |

**Per-voice interpretation:**

| | Tableside | Banquet | Tapas | Kaiseki |
|---|---|---|---|---|
| **XOrison** (Sacred Choir) | Chapel — small stone room, 6 monks, you sit among them. | Cathedral — Notre-Dame, 40 voices, the reverb IS the instrument. | Taizé — short repeated chants, communal, many small prayers. | Hildegard von Bingen — each chant a seasonal offering, mystical, unique. |
| **XOpera** (Operatic) | Lieder recital — singer and piano, drawing room, 30 people. | La Scala — full production, orchestra, 2,000 seats, the big night. | Cabaret — Weill, Piaf, small stage, quick songs, cigarette smoke. | Sprechstimme — Schoenberg, each phrase its own world, unpredictable delivery. |
| **XOrate** (Beatbox) | Cypher — the circle, one beatboxer, you're in the crowd. | Stadium — Rahzel at a hip-hop arena, the mouth filling the space. | Battle — quick rounds, trading bars, response energy. | Sō Percussion — prepared-vocal, each sound a composed gesture, John Cage meets the mouth. |
| **XOvertone** (Throat Singing) | Yurt — one singer, felt walls, the harmonics bounce between you. | Monastery — Tibetan chant in the great hall, walls vibrating. | Tuvan festival — singers trading techniques, competitive overtones. | Alvin Lucier — "I Am Sitting in a Room," the voice gradually becoming the space itself. |

---

### Fusion Recipes: SAUCE BASES (Secret — 5th Slot Only)

What binds the fusion. Four mother-sauce philosophies that hold disparate traditions together.

| Recipe | Sauce | FX Chain | Character |
|--------|-------|----------|-----------|
| **Dashi** | Japanese stock — kombu + bonito, invisible umami | Transparent reverb → subtle harmonic enhancer → high-pass clarity | The flavor you can't identify but miss when it's gone. Transparent depth. The FX chain that makes everything sound better without being detectable. Umami processing. |
| **Mole** | Mexican — 30+ ingredients, days of preparation | Parallel processing: chorus ‖ phaser ‖ delay ‖ reverb, all at low wet mix | Irreducible complexity. Each effect at 15-20% creates a gestalt that's greater than its parts. You can't taste the individual chocolate, chili, cinnamon — you taste *mole*. |
| **Jus** | French — reduced pan drippings, concentrated essence | High-ratio compression → resonant EQ boost → short tight room | Concentrated. Everything non-essential removed. What remains is the pure essence of the sound, amplified. The fond scraped from the pan and deglazed into liquid gold. |
| **Chimichurri** | Argentine — raw herbs, oil, acid, no cooking | Flanger → bright EQ shelf → stereo widener → no reverb | Fresh, bright, raw, green. No cooking involved — just chopped and combined. The anti-reverb recipe. Immediate, present, in-your-face. Parsley-and-garlic energy. |

**Per-EP interpretation:**

| | Dashi | Mole | Jus | Chimichurri |
|---|---|---|---|---|
| **XOasis** (Rhodes) | Classic jazz Rhodes — you feel the warmth but can't locate the effect. Ghost reverb. | Neo-soul Rhodes — chorus + phaser + delay + plate, all gentle, all essential. Erykah Badu. | Funk Rhodes — compressed, EQ'd, biting. Herbie Hancock "Chameleon." | Lo-fi Rhodes — raw, detuned, no reverb, close-mic'd. Tom Misch bedroom recordings. |
| **XOddfellow** (Wurli) | Surf Wurli — hidden spring reverb adding depth to the reedy buzz. | Psych Wurli — everything on at once, swirling, paisley. | Garage Wurli — cranked preamp, the reed IS the sound, nothing else needed. | Punk Wurli — bright, aggressive, no effects, three chords, go. |
| **XOnkolo** (Clav) | Funk Clav — subtle wah creating invisible movement. | Dub Clav — phase + delay + spring, heavy but somehow light. | P-Funk Clav — auto-wah into compressor, Worrell's weapon. | Afrobeat Clav — dry, rhythmic, Tony Allen grooves, no space. |
| **XOpcode** (DX EP) | City Pop DX — the 1985 Tokyo shimmer. Chorus so subtle it's just "width." | Vaporwave DX — reverb + chorus + pitch artifacts, synthetic nostalgia. | Modern R&B DX — compressed FM bells, tight, present, Disclosure. | Chiptune DX — raw FM, no effects, 4-op precision, the algorithm IS the sound. |

---

### Garden Recipes: PRESERVATION METHODS

How ingredients are kept. Four ways to extend the life and transform the character of what the garden grows.

| Recipe | Method | FX Chain | Character |
|--------|--------|----------|-----------|
| **Fresh** | Just picked, morning dew | Light room reverb → natural dynamics → open EQ | No processing philosophy — the anti-recipe. The string as it comes off the instrument, in a room, with air. Seasonal, ephemeral, this-moment-only. |
| **Dried** | Sun-dried, concentrated, shelf-stable | Gentle bit reduction → warm saturation → tight bandwidth EQ | Moisture removed, flavor concentrated. The string sound with its transient bloom reduced, leaving dense harmonic core. Sun-dried tomato vs fresh tomato — same ingredient, completely different food. |
| **Pickled** | Vinegar, acid, tang, preserved in brine | Ring modulation → comb filter → short slapback delay | Acidic, sharp, tangy. The comb filter adds metallic brine overtones, the ring mod adds acid bite, the slapback adds the jar's resonance. Preserved but transformed — you know it was a cucumber, but it's a pickle now. |
| **Composted** | Broken down, returned to earth, feeding new growth | Granular processing → long dark reverb → heavy low-pass | Decomposition as creation. The string sound broken into grains, scattered into dark space, filtered until only the deepest frequencies remain. Humus — the rich black soil that feeds next season's garden. |

**Per-string interpretation:**

| | Fresh | Dried | Pickled | Composted |
|---|---|---|---|---|
| **XOrchard** (Orchestral) | Film score — lush section in a scoring stage. Spielberg. | Vintage vinyl strings — saturated, bandwidth-limited, warm patina. | Stravinsky — acidic orchestration, dissonant, The Rite of Spring. | Drone — orchestral strings granularized into earth-tone ambient. |
| **XOvergrow** (Solo) | Recital — one violin in a quiet room. Every breath audible. | Dried herb bundle — concentrated solo tone, meditation bell sustain. | Extended technique — col legno, sul ponticello, scratch tone. | Pauline Oliveros — deep listening, the solo voice dissolved into space. |
| **XOsier** (Chamber) | Salon concert — quartet in a drawing room, intimate, alive. | Preserved manuscripts — aged chamber music, warm and brittle. | Bartók quartets — folk-acid, strange intervals, Eastern European tang. | Late Feldman — chamber ensemble dissolving into near-silence over hours. |
| **XOxalis** (Synth Strings) | Solina through a JC-120 — clean chorus, immediate, 1978. | Vangelis — compressed, saturated, Blade Runner warmth. | Boards of Canada — detuned, warped, acid-washed nostalgia. | William Basinski — loops decaying, tape disintegrating, sublime entropy. |

---

### Cellar Recipes: AGING METHODS

How the foundation develops over time. Four stages of maturation that transform raw stock into something with depth and character.

| Recipe | Method | FX Chain | Character |
|--------|--------|----------|-----------|
| **Green** | Young, unaged, just arrived in the cellar | Clean preamp → room mic → no coloration | What just walked in. No barrel, no time, no transformation. Raw potential. The bass sound before the cellar does its work. Bright, rough, honest. |
| **Barrel** | Oak-aged, tannin, vanilla, oxidation | Tube saturation → warm low-shelf EQ boost → medium hall reverb | The classic. Oak imparts vanilla, tannin adds structure, controlled oxidation rounds the edges. The bass equivalent of a well-aged bourbon — smooth, warm, complex, dangerous. |
| **Cave** | Natural cave aging, mineral, constant temperature | Convolution reverb (cave/tunnel IR) → gentle compression → resonant band-pass | Stone walls, constant humidity, mineral character. The bass sound processed through geological time. Damp, ancient, crystalline deposits forming on the surface. Roquefort's cave. Champagne's chalk tunnels. |
| **Charred** | Charred barrel interior, caramel, smoke, carbon | Heavy distortion → speaker cabinet simulation → brick-wall limiter | The inside of the barrel burned black before the spirit goes in. Maximum transformation — the char layer filters everything through carbon, adding smoke, caramel, and intensity. The difference between bourbon and whiskey. Aggressive aging. |

**Per-bass interpretation:**

| | Green | Barrel | Cave | Charred |
|---|---|---|---|---|
| **XOgre** (Sub) | Raw sine wave. 30Hz test tone. The ingredient before aging. | Sub through warm saturation — you feel it in the floor, but it glows. | Underground — sub bass in a cavern, resonant modes of stone. | Distorted sub — EDM tearout, bass face, the ogre is ANGRY. |
| **XOxbow** (Analog) | New Moog out of the box — that first filter sweep. | TB-303 acid — the barrel aged it into something nobody planned. Legend. | Deep house — analog bass in the tunnel, 3am, reverb from the architecture. | Industrial — analog bass through broken amps, nine inch nails. |
| **XOaken** (Acoustic) | Fresh-strung upright — gut strings, bright, woody, alive. | Jazz bass — years of play, worn fretboard, finger-oil patina. Mingus. | Bluegrass bass — played in a barn, natural reverb, whiskey on the shelf. | Rockabilly slap — the upright pushed past its limits, percussive, aggressive. |
| **XOmega** (FM/Digital) | Clean FM — carrier + modulator, textbook ratio, educational. | DX bass — the FM patch that became a genre. Detuned, warm, impossible. | Reese bass — the subterranean classic. Dark, moving, cavernous. | Skrillex — FM bass destroyed by distortion, rebuilt as a weapon. |

---

### Broth Recipes: LIQUID PREPARATIONS

What the medium becomes. Four fundamental approaches to liquid that determine what everything immersed in it will taste like.

| Recipe | Preparation | FX Chain | Character |
|--------|------------|----------|-----------|
| **Clear** | Consommé — clarified, perfectly transparent | Precision EQ (surgical cuts) → transparent compressor → pristine stereo delay | Everything unnecessary removed. The pad is there but you see right through it. No mud, no haze — pure, refined, the product of careful clarification. The broth that took all day to make and looks like it took no effort at all. |
| **Bone** | 24-hour bone broth — collagen, gelatin, thick, nourishing | Thick plate reverb → heavy compression → warm saturation → low-pass at 4kHz | Maximum body. The pad that coats everything in richness. Collagen = harmonic density. Gelatin = the way it clings to whatever it touches. The most caloric, most nourishing, most enveloping recipe. |
| **Bisque** | Shellfish bisque — cream, brandy, luxury | Lush chorus → warm spring reverb → soft-knee compression → gentle drive | Smooth, rich, decadent. The chorus adds cream shimmer, the spring reverb adds kitchen clatter, the compression makes it velvety, the drive adds brandy warmth. The recipe that costs $40 a bowl and earns it. |
| **Dipping** | Fondue / hot pot — the communal dip, interactive | Sidechain ducker → resonant filter (key-tracked) → ping-pong delay | Interactive. The pad responds to what's played over it — ducking to make room, filtering to match the melody's register, echoing fragments back. The hot pot that changes flavor depending on what you dip in it. |

**Per-pad interpretation:**

| | Clear | Bone | Bisque | Dipping |
|---|---|---|---|---|
| **XOverwash** (Diffusion) | Watercolor on wet paper — transparent wash, barely there. | Thick oil paint — heavy, pigmented, covers everything. | Gouache — opaque but smooth, cream-mixed pigment. | Dip-dye — the fabric takes the color unevenly, beautifully. |
| **XOverworn** (Erosion) | Erosion revealing clean stone — stripping to clarity over time. | Erosion into mud — thick, dark, sediment-heavy. Slow river delta. | Erosion into sand — fine-grained, warm, beach-smooth. | Tidal pool — erosion that responds to what the tide brings in. |
| **XOverflow** (Pressure) | Pressure cooker with clear stock — clean, intense, fast. | Pressure cooker with bones — thick, dark, concentrated to maximum. | Pressure cooker with cream — dangerous (it can explode) but divine. | Fondue pot — pressure from below, you control what enters. |
| **XOvercast** (Crystallization) | Ice — clear, geometric, pure water frozen into structure. | Frozen bone broth cubes — dense, opaque, nutritious cold blocks. | Frozen custard — smooth, rich, crystallized luxury. | Ice cream dip — the warm thing hits the cold thing, shell forms instantly. |

---

## FX Recipe Summary

```
CHEF (Organ)           KITCHEN (Piano)        CHOIR (Voices)
├─ Raw                 ├─ Sear                ├─ Tableside
├─ Grilled             ├─ Bake                ├─ Banquet
├─ Braised             ├─ Smoke               ├─ Tapas
└─ Fermented           └─ Torch               └─ Kaiseki

GARDEN (Strings)       CELLAR (Bass)          BROTH (Pads)
├─ Fresh               ├─ Green               ├─ Clear
├─ Dried               ├─ Barrel              ├─ Bone
├─ Pickled             ├─ Cave                ├─ Bisque
└─ Composted           └─ Charred             └─ Dipping

SECRET: FUSION (EP) — 5th Slot
├─ Dashi
├─ Mole
├─ Jus
└─ Chimichurri
```

**28 recipe names. Each interpreted 4 ways (per engine in quad) = 112 unique FX chains.**
**Standard: 24 engines × 64 configs = 1,536. With Fusion unlock: 1,792 total.**

---

## Boutique Wildcard Assignments

### Quad 1: CHEF — Organs (already locked)

| Engine | Company | Region/Vibe | Products |
|--------|---------|-------------|----------|
| **XOto** | Teenage Engineering | Stockholm / playful minimalism | OP-1, OP-XY, Riddim, Pocket Operators |
| **XOctave** | Arturia | Grenoble / French precision | MicroFreak, MiniBrute, MatrixBrute, BruteFactor |
| **XOleg** | Erica Synths | Riga / Baltic industrial | Syntrx, Pērkons, Bullfrog, Black series |
| **XOtis** | The Oddballs | USA / garage inventors | Telepathic Instruments, Critter & Guitari, Torso Electronics, OXI |

### Quad 2: KITCHEN — Pianos

| Engine | Company | Why This Pairing | Products |
|--------|---------|-----------------|----------|
| **XOven** | **Moog** | Cast iron needs Moog's mass. Heavy, warm, enormous analog presence. The Moog IS the cast iron skillet of synthesis — seasoned, indestructible, gets better with age. | Subsequent 37, Grandmother, Matriarch, Moog One |
| **XOchre** | **Elektron** | Copper's fast heat transfer = Elektron's immediate, responsive, hands-on workflow. Pattern-based sequencing adds rhythmic life to the upright's intimate character. Quick to respond, efficient, always ready. | Digitone, Analog Four, Syntakt, Model:Cycles |
| **XObelisk** | **Ciat-Lonbarde** | Stone meets Peter Blasser's tactile chaos. Banana-jack patching, hand-built wooden instruments, unpredictable circuit paths. The ultimate prepared piano wildcard — objects on strings, but the objects are alive. | Sidrax Organ, Plumbutter, Cocoquantus, Tetrax Organ |
| **XOpaline** | **Bastl Instruments** | Glass meets Czech DIY playfulness. microGranny's lo-fi sampling, Kastle's tiny chaotic synthesis — fragile, charming, unexpectedly beautiful sounds from minimal circuits. The toy piano of the modular world. | microGranny, Kastle, Softpop, Thyme |

### Quad 3: CHOIR — Voices

| Engine | Company | Why This Pairing | Products |
|--------|---------|-----------------|----------|
| **XOrison** | **Eventide** | Sacred choir needs Eventide's harmonizer heritage. H9000's pitch algorithms are practically liturgical — stacking voices in perfect intervals, creating choirs from single tones. The company that invented harmonization as an instrument. 50 years of making one voice sound like many. | H9000, H90, UltraTap, MicroPitch |
| **XOpera** | **Hologram Electronics** | Operatic drama meets Portland's granular dreamers. Dream Sequence's textural processing turns a single voice into a spectral aria. Infinite Jets sustains vocal tones into infinity. The diva and the pedal — theatrical voice dissolved into particles, then reassembled as something larger than life. | Dream Sequence, Infinite Jets, Microcosm, Chroma Console |
| **XOrate** | **KOMA Elektronik** | Beatbox needs Berlin's physical, industrial energy. KOMA builds instruments you TOUCH — Field Kit's contact mic sampling, Kommander's gesture control. The mouth is the most physical instrument; KOMA makes electronics equally physical. Hands, lips, throat, circuits — all body. | Field Kit, Kommander, Poltergeist, RH301 |
| **XOvertone** | **Qu-Bit Electronix** | Throat singing's harmonic series meets Qu-Bit's spectral processing. Nebulae's granular sampling can isolate individual overtones. Data Bender's digital destruction reveals hidden harmonics. The voice that contains multitudes, processed by circuits that can hear each one separately. | Nebulae, Data Bender, Surface, Chord v2 |

### Secret Quad: FUSION — Electric Pianos (5th Slot Unlock)

| Engine | Company | Why This Pairing | Products |
|--------|---------|-----------------|----------|
| **XOasis** | **Chase Bliss** | Rhodes warmth deserves Chase Bliss's expressive, dreamy processing. Mood's micro-looping, Blooper's tape artifacts, Habit's memory — these are the pedals on every neo-soul Rhodes player's board. The oasis shimmer. | Mood, Blooper, Habit, Automatone |
| **XOddfellow** | **Dreadbox** | Wurlitzer's reedy grit meets Greek analog warmth. Dreadbox builds raw, characterful analog synths with Mediterranean flair — Typhon's dual-filter growl is the Wurli's soulmate. Odd company for an odd fellow. | Typhon, Nyx v2, Erebus, Hypnosis |
| **XOnkolo** | **Gamechanger Audio** | Clavinet funk needs Gamechanger's Latvian innovation. Plasma Drive's actual lightning-bolt distortion, Motor's spinning speaker — physical energy transferred to sound. The clavinet was always about energy transfer (hammer → string → pickup). Gamechanger makes energy audible. | Plasma Drive, Motor Synth, Bigsby Pedal, Plus Pedal |
| **XOpcode** | **Waldorf** | FM EP precision meets German wavetable mastery. Waldorf's Iridium and Quantum understand digital synthesis at a molecular level — the same engineering DNA that made the DX7 possible, evolved 40 years forward. Algorithm as recipe. | Iridium, Quantum, Blofeld, M |

### Quad 4: GARDEN — Strings

| Engine | Company | Why This Pairing | Products |
|--------|---------|-----------------|----------|
| **XOrchard** | **Sequential** | Orchestral lushness needs Sequential's legendary poly pads. The Prophet-5 IS the sound of synth strings done at scale — 40 years of cultivated analog polyphony. An orchard of voices, each slightly different, breathing together. | Prophet-5, Prophet Rev2, OB-6, Take 5 |
| **XOvergrow** | **Soma Laboratory** | Solo strings' raw exposure meets Soma's organic chaos. Lyra-8's touch-sensitive feedback, Pulsar-23's biological rhythm — Vlad Kreimer builds instruments that feel alive. Raw growth, uncultivated, wild. | Lyra-8, Pulsar-23, Ornament-8, Terra |
| **XOsier** | **Instruo** | Chamber strings' intimate weaving meets Scottish hand-built precision. Jason Lim's modules are the chamber ensemble of Eurorack — every detail considered, every interaction intentional. Willow-weave craft. | Saïch, Cès, Arbhar, Tròsg |
| **XOxalis** | **Mutable Instruments** | Synth strings' geometric patterns meet Émilie Gillet's mathematical elegance. Plaits' 24 synthesis models, Rings' resonant structures, Beads' granular clouds — open-source beauty with geometric precision. The clover-leaf algorithm. | Plaits, Rings, Beads, Stages |

### Quad 5: CELLAR — Bass

| Engine | Company | Why This Pairing | Products |
|--------|---------|-----------------|----------|
| **XOgre** | **Noise Engineering** | Sub bass needs NE's aggressive digital depth. Basimilus Iteritas Alter's metallic growl, BIA's impact — sounds designed to be felt in the chest. The ogre in the basement, shaking the foundations. | Basimilus Iteritas Alter, Loquelic Iteritas Percido, Manis Iteritas, BIA |
| **XOxbow** | **Make Noise** | Analog bass's patient depth meets Tony Rolando's experimental analog. Strega's dark magic, 0-Coast's patchable signal path — deep, exploratory, the slow bend of the river where things accumulate. | Strega, 0-Coast, 0-Ctrl, Morphagene |
| **XOaken** | **Folktek** | Acoustic bass's wooden resonance meets Folktek's hand-built resonant instruments. Arius Blaze welds, solders, and sculpts instruments from raw materials. The sound of wood and metal shaped by hands. The oak barrel. | Mescaline, Matter, Resonant Garden, Conduit |
| **XOmega** | **4ms** | FM bass's mathematical precision meets 4ms's spectral processing. Ensemble Oscillator's additive architecture, Spherical Wavetable Navigator's mathematical beauty — the final reduction, distilled to pure frequency ratios. | Ensemble Oscillator, Spherical Wavetable Navigator, Tapographic Delay, STS |

### Quad 6: BROTH — Pads

| Engine | Company | Why This Pairing | Products |
|--------|---------|-----------------|----------|
| **XOverwash** | **Strymon** | Diffusion pads need Strymon's reverb mastery. BigSky's infinite shimmer, NightSky's granular reverb, Volante's tape echo — the definitive atmosphere machines. Infusion = reverb. The flavor that bleeds into everything. | BigSky, NightSky, Volante, Deco |
| **XOverworn** | **Empress Effects** | Erosion pads need Empress's deep processing. ZOIA's modular-in-a-box can build any signal chain — session-long parameter automation, gradual filter sweeps, imperceptible degradation programmed across minutes. Patient reduction. | ZOIA, Reverb, Echosystem, Multidrive |
| **XOverflow** | **Endorphin.es** | Pressure pads need Endorphin.es's aggressive Ukrainian energy. Furthrrrr Generator's powerful oscillator, Ghost's reverb-into-distortion — systems that build pressure and explode. The sealed pot, the steam, the release. | Furthrrrr Generator, Ghost, Cockpit 2, Shuttle System |
| **XOvercast** | **Polyend** | Crystallization pads need Polyend's capture-and-freeze approach. Tracker's sample mangling, Play's generative sequencing — freezing a moment and examining it from every angle. Flash-freeze as creative tool. | Tracker, Play, Synth, Medusa |

---

## Complete Wildcard Map (28 Companies)

```
CHEF (Organ)         KITCHEN (Piano)       CHOIR (Voices)
├─ Teenage Eng.      ├─ Moog               ├─ Eventide
├─ Arturia           ├─ Elektron           ├─ Hologram Electronics
├─ Erica Synths      ├─ Ciat-Lonbarde      ├─ KOMA Elektronik
└─ The Oddballs      └─ Bastl Instruments   └─ Qu-Bit Electronix

GARDEN (Strings)     CELLAR (Bass)         BROTH (Pads)
├─ Sequential        ├─ Noise Engineering  ├─ Strymon
├─ Soma Laboratory   ├─ Make Noise         ├─ Empress Effects
├─ Instruo           ├─ Folktek            ├─ Endorphin.es
└─ Mutable Inst.     └─ 4ms                └─ Polyend

SECRET: FUSION (EP) — 5th Slot
├─ Chase Bliss
├─ Dreadbox
├─ Gamechanger Audio
└─ Waldorf
```

**28 engines. 28 companies. Zero repeats.**

Each company's character becomes a spice that transforms the base instrument. The wildcard never replaces the voice — it *seasons* it. A Shō is still a Shō with OP-1 selected as wildcard; it just gains OP-1's lo-fi cassette warmth and sequencing quirks. A concert grand is still a concert grand with Moog selected; it just gains Moog's filter resonance and analog saturation as processing layers.

---

## Open Questions

1. **Accent colors** — Does each quad share a color family, or does each engine get its own? The culinary metaphor suggests warm palette families (copper tones for Kitchen, deep reds for Cellar, greens for Garden, etc.). Choir might use throat/flesh tones or warm neutrals.
2. **FX recipes — DSP implementation** — Recipe archetypes are designed for all 7 quads (28 recipes total). Next step: translate each recipe's FX chain into specific DSP module selections, parameter ranges, and routing from the XOmnibus shared DSP library.
3. **Aquatic identity** — Does the Culinary Collection sit within the water column mythology? If so, where? The kitchen is on land — on the shore, perhaps. The osteria at the water's edge.
4. **Integration priority** — Which quad builds first after Chef? Kitchen (Piano) and Cellar (Bass) are the most universally needed. Kitchen is now even more critical since it gates the Fusion unlock.
5. **XOpaline vs XOpal** — Name collision. XOpal already exists in the main engine roster. XOpaline is distinct but close. Consider alternatives: XObjet, XOrnament, XOverture.
6. **Preset strategy** — Do Culinary Collection presets use the same 6 moods, or develop culinary-specific categories (Appetizer, Entrée, Dessert, etc.)?
7. **5th slot architecture** — The Fusion unlock requires extending the MegaCouplingMatrix from 4×4 to 5×5 (or 4+1). How does this interact with the existing engine slot management in `EngineRegistry.h`? Does the 5th slot get its own DSP bus, or does it share with one of the Kitchen slots?
8. **Other unlock paths?** — Currently only Full Kitchen unlocks the 5th slot. Could other full-quad loadouts (all 4 Chefs, all 4 Cellars) unlock different secret behaviors? Or is Fusion the only secret quad?
9. **Choir voice synthesis** — What synthesis approach for vocal engines? Formant synthesis, sample-based, physical modeling of vocal tract, or hybrid? Each engine might use a different approach (XOrison = sample choir, XOrate = physical model, XOvertone = additive harmonics).
