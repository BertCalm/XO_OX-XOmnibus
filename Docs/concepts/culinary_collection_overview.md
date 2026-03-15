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

## Quad 3: FUSION — Electric Pianos (The Crossroads)

**Status:** Concept
**Coupling mode:** Migratory — cultural collision, genre-crossing, hybrid identity
**Instrument group:** Electric pianos — tine, reed, pickup, FM

Electric pianos exist because genres collided. Jazz needed amplification. Funk needed percussive keys. Pop needed shimmer. R&B needed warmth-at-volume. Every EP is a fusion instrument — acoustic principles married to electric amplification, creating sounds that belong to no single tradition. The crossroads where blues met jazz met soul met pop met electronic.

| Engine | Cuisine Influence | EP Archetype | Character |
|--------|-------------------|-------------|-----------|
| **XOasis** | Spice Route (East→West) | Rhodes / Tine EP | Warm bell-tones. Tine struck by hammer, amplified by pickup. The sound of every genre borrowing from every other. Smooth enough for jazz, gritty enough for neo-soul, shimmering enough for lo-fi. |
| **XOddfellow** | Night Market (street food fusion) | Wurlitzer / Reed EP | Reedy, gritty, intimate. Reed vibrating near a pickup — raw, slightly distorted, full of character. The busker instrument. Sounds best slightly broken. |
| **XOnkolo** | West African → diaspora | Clavinet / Pickup Keys | Funky, percussive, string-slap energy. Strings struck and picked up magnetically. The instrument Stevie Wonder and Bernie Worrell used to bridge continents. Named for the nkolo (a Central African thumb piano ancestor). |
| **XOpcode** | Silicon Valley → Tokyo | DX / FM Electric Piano | Crystalline, digital, impossibly clean. FM synthesis creating bell-like tones that don't exist in nature. The sound of the 1980s imagining the future. Algorithm as recipe. |

**Coupling behavior:** Migratory — when Fusion engines couple, sounds adopt characteristics of each other's tradition. XOasis absorbs XOnkolo's percussive attack; XOpcode inherits XOddfellow's reed warmth. The coupling is cultural exchange — each engine returns from the encounter carrying something new.

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

## Cross-Quad Coupling: The Full Kitchen

The Culinary Collection's power isn't in individual quads — it's in what happens when the kitchen operates as a whole. Six coupling modes create 15 unique quad-pair interactions:

### Coupling Mode Matrix

```
         CHEF    KITCHEN  FUSION   GARDEN   CELLAR   BROTH
CHEF      —      Temper   Season   Harvest  Ferment  Poach
KITCHEN   —       —       Plate    Press    Cure     Steam
FUSION    —       —        —       Graft    Pickle   Marinate
GARDEN    —       —        —        —       Root     Steep
CELLAR    —       —        —        —        —       Dissolve
BROTH     —       —        —        —        —        —
```

### Signature Cross-Quad Behaviors

**Chef × Kitchen (Temper):** The cook meets the surface. Oto's shō on XOven's cast iron — the cluster sustain takes on massive thermal resonance. Otis's Hammond on XOpaline's glass — tonewheel warmth through crystalline fragility.

**Chef × Broth (Poach):** The cook meets the liquid. Oleg's bayan immersed in XOverwash — the bellows diffuse into watercolor. Octave's pipe organ pressurized by XOverflow — builds until the cathedral explodes.

**Kitchen × Cellar (Cure):** Surface meets foundation. XObelisk's prepared piano on XOgre's sub — stone percussion with seismic undertow. XOchre's copper on XOxbow's analog — warm on warm, maple syrup depth.

**Garden × Broth (Steep):** Ingredient meets liquid. XOvergrow's raw solo string steeped in XOverworn — the vibrato erodes imperceptibly over minutes. XOxalis's synth strings infused by XOverwash — geometric patterns dissolving into organic flow.

**Fusion × Garden (Graft):** Crossroads meets ingredient. XOasis's Rhodes absorbs XOrchard's orchestral lushness — the tine bell-tone develops string sustain. XOnkolo's clavinet funk grafted onto XOsier's chamber weave — percussive pluck with intimate bowed tail.

**Cellar × Broth (Dissolve):** Foundation meets medium. XOgre's sub dissolved in XOvercast — sub-bass flash-frozen into crystalline structure. XOmega's FM bass reduced by XOverworn — digital precision slowly eroded into analog warmth.

---

## Four Signature Confluences

When specific engines from different quads align, special behaviors emerge — the Culinary Collection's equivalent of the Water Quad's locked Confluences.

### 1. "Sunday Gravy" — XOtis × XOven × XOxbow × XOverworn
*Hammond B3 on cast iron, over analog bass, reducing all day.*

Session-long preset. Starts as bright tonewheel funk over fat Moog bass. Over 20 minutes, XOverworn imperceptibly steals harmonics from both. By the end, you have a dark, thick, reduced texture that sounds nothing like where it started — but every moment of the transition was musically valid. Sunday gravy takes eight hours. This takes twenty minutes. Worth every second.

### 2. "Omakase" — XOto × XOpaline × XOpcode × XOvercast
*Shō cluster through glass, over FM bass, flash-frozen.*

The chef's choice. Oto's 11-note aitake cluster resonates through XOpaline's crystalline body. XOpcode's FM harmonics add digital shimmer underneath. Then XOvercast triggers — everything flash-freezes. The moment captured. No two triggers sound the same because the cluster, glass resonance, and FM ratios are all in continuous motion. The chef serves what the moment demands.

### 3. "Fermentation" — XOleg × XObelisk × XOaken × XOverflow
*Bayan on stone, over upright bass, pressure building.*

Oleg's bellows push air across XObelisk's cold marble. XOaken's wooden body resonates below. XOverflow seals the system — pressure builds phrase by phrase. Harmonics compress, distortion accumulates, everything gets tighter and hotter until the valve opens and the fermented release is rich, complex, and slightly dangerous. Controlled rot as art form.

### 4. "Mole" — XOctave × XOchre × XOnkolo × XOxalis
*Baroque organ on copper, with clavinet funk, through synth string sauce.*

Thirty ingredients. Three days. One sauce. Octave's baroque chiff transients hit XOchre's responsive copper surface. XOnkolo adds rhythmic clavinet funk from below. XOxalis wraps everything in filtered synth-string texture — the mole sauce that somehow tastes like chocolate and chili and smoke and fruit all at once. Complexity that reads as unity. The coupling that takes the longest to tune but rewards patience with sounds nothing else can produce.

---

## Engine Summary Table

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
| 9 | Fusion | XOasis | Rhodes / Tine EP | `oasis_` | — |
| 10 | Fusion | XOddfellow | Wurlitzer / Reed EP | `oddf_` | — |
| 11 | Fusion | XOnkolo | Clavinet / Pickup Keys | `onko_` | — |
| 12 | Fusion | XOpcode | DX / FM EP | `opco_` | — |
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

---

## Relationship to Existing Quads

The Culinary Collection absorbs and extends the existing quad concepts:

- **Chef Quad** → Culinary Collection Quad 1 (unchanged, fully designed)
- **Water Quad** → Evolves into **Broth Quad** (Quad 6). Same four engines (Overwash, Overworn, Overflow, Overcast), same time-scale framework, but now explicitly mapped to the Pad/Atmosphere instrument group and reframed as liquid cooking processes rather than raw water phenomena.
- **Botanical Quad** (previously discussed) → The botanical concepts (ferment, reduce, infuse) are distributed across the Garden and Broth quads rather than forming a standalone quad. Growth lives in Garden (strings). Liquid transformation lives in Broth (pads). This avoids a phenomena-only quad with no instrument anchor.

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
**Full collection:** 6 quads × 256 = **1,536 configurations before coupling**

### What Each Axis Does

- **VOICE** — The instrument variant. In Chef Quad these are organs; in Kitchen they're piano types; in Cellar they're bass modes. Locked per engine. The "ingredient."
- **FX RECIPE** — The chef's signature processing chain. 4 per engine, regionally/thematically flavored. The "cuisine." (TBD across all quads.)
- **WILDCARD** — A specific boutique synth company's character injected as a modulation/texture layer. 4 products per company. The "spice."

### Wildcard Philosophy

The wildcard isn't a preset — it's a *coloring agent*. When you select "OP-1" as Oto's wildcard, it doesn't turn the Shō into an OP-1. It applies OP-1's *character* — its lo-fi sampling grain, its quirky sequencing artifacts, its cassette-deck warmth — as a processing and modulation layer over the organ voice. The instrument stays itself; the wildcard changes *how it's perceived*.

**Each company appears exactly once across all 24 engines. No repeats.**

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

### Quad 3: FUSION — Electric Pianos

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

## Complete Wildcard Map (24 Companies)

```
CHEF (Organ)         KITCHEN (Piano)       FUSION (EP)
├─ Teenage Eng.      ├─ Moog               ├─ Chase Bliss
├─ Arturia           ├─ Elektron           ├─ Dreadbox
├─ Erica Synths      ├─ Ciat-Lonbarde      ├─ Gamechanger Audio
└─ The Oddballs      └─ Bastl Instruments   └─ Waldorf

GARDEN (Strings)     CELLAR (Bass)         BROTH (Pads)
├─ Sequential        ├─ Noise Engineering  ├─ Strymon
├─ Soma Laboratory   ├─ Make Noise         ├─ Empress Effects
├─ Instruo           ├─ Folktek            ├─ Endorphin.es
└─ Mutable Inst.     └─ 4ms                └─ Polyend
```

**24 engines. 24 companies. Zero repeats.**

Each company's character becomes a spice that transforms the base instrument. The wildcard never replaces the voice — it *seasons* it. A Shō is still a Shō with OP-1 selected as wildcard; it just gains OP-1's lo-fi cassette warmth and sequencing quirks. A concert grand is still a concert grand with Moog selected; it just gains Moog's filter resonance and analog saturation as processing layers.

---

## Open Questions

1. **Accent colors** — Does each quad share a color family, or does each engine get its own? The culinary metaphor suggests warm palette families (copper tones for Kitchen, deep reds for Cellar, greens for Garden, etc.)
2. **FX recipes** — The third axis of 4×4×4. Each engine needs 4 signature processing chains ("cuisines"). TBD across all quads. Chef Quad's FX recipes should be designed first as the template.
3. **Aquatic identity** — Does the Culinary Collection sit within the water column mythology? If so, where? The kitchen is on land — on the shore, perhaps. The osteria at the water's edge.
4. **Integration priority** — Which quad builds first after Chef? Kitchen (Piano) and Cellar (Bass) are the most universally needed.
5. **XOpaline vs XOpal** — Name collision. XOpal already exists in the main engine roster. XOpaline is distinct but close. Consider alternatives: XObjet, XOrnament, XOverture.
6. **Preset strategy** — Do Culinary Collection presets use the same 6 moods, or develop culinary-specific categories (Appetizer, Entrée, Dessert, etc.)?
