# CELLAR Quad — Visionary Deep Dive

*Culinary Collection | Bass Engines | March 2026*
*The Foundation. The Gravity. The Thing You Only Notice When It's Gone.*

---

## First: The Naming Collision

**XOxbow conflicts with the existing OXBOW engine** (entangled reverb, Chiasmus FDN, Oxbow Teal `#1A6B5A`).

OXBOW already owns the word. Its entire mythology is the river cutting itself off — a geological metaphor. The Culinary Cellar's second bass engine needs a different word entirely.

The culinary concept: *Aged Wine. Patient complexity. Warm, fat, developed through decades of circuit evolution. Terroir. The slow river bend where sediment collects.*

That last phrase — "sediment collects" — suggests the solution is already in the original description. What word captures aged wine, warm circuit character, warm analog bass terroir?

**Recommended rename options, ranked:**

1. **XOlate** — from *oblate*, the shape of a full-bodied wine glass; also *fermentation oblate*, the yeast bed at the bottom of a cask. Hidden resonance: "oblate" = flattened at the poles, like a warm low frequency. Param prefix: `olate_`. **Top choice.**

2. **XOllect** — from *collect*, as in what a cellar does: collects, ages, archives. Also "collector" circuits in analog design. Obscure enough to be interesting. Param prefix: `olct_`.

3. **XOaken** is already the third engine (Cured Wood / Acoustic Bass). Keep it.

4. **XOxide** — the patina of oxidation, which is exactly what happens to wine during aging. Also what happens to analog circuits left to mature. But `xobserve-xoxide-concepts.md` already has XOxide in the vault. Check before committing.

5. **XOld** — clean, direct, absolutely names the concept. "Old bass." The aged, settled, patient analog character. Too blunt? Or perfectly honest? Param prefix: `old_`.

6. **XOlive** — olive oil is the other "aged liquid" in culinary tradition; EVOO aged in dark cellars develops complexity analog to fine wine. The olive's warmth, its Mediterranean patience. Param prefix: `oliv_`. Also: olive green accent color would be beautiful.

**Recommendation: XOlate** — it sits in the O-word naming convention comfortably, carries wine-glass geometry, and `olate_` has no conflicts in the current prefix registry.

---

## Level 1 — Gravitational Coupling: The DSP

The overview document says gravitational coupling "pulls everything downward." That's a starting description, not a specification. Here is the actual physics.

### What Real Gravity Does

In Newtonian gravity, a massive body curves spacetime around it. Objects in that field don't get pushed — they bend toward the attractor. Their trajectories curve. They don't arrive at the mass, they orbit it.

This is the key distinction: **gravitational coupling is not a low-pass filter applied to other engines.** A low-pass filter removes information. Gravitational coupling *redirects* it.

### The Frequency-Domain Gravity Model

When CELLAR engine C is running at fundamental frequency `f_c` with harmonic series `{f_c, 2f_c, 3f_c, ...}`, a coupled engine E running at fundamental `f_e` experiences gravitational pull proportional to:

```
F_gravity(f) = G × M_cellar / |f - f_nearest|²
```

Where:
- `G` = gravitational constant (user-controllable: the PULL parameter)
- `M_cellar` = the "mass" of the cellar engine — proportional to sub-energy below 80Hz
- `f_nearest` = the nearest harmonic of the cellar engine to frequency `f`
- `|f - f_nearest|²` = squared distance in frequency space

The result: partials in the coupled engine that are *near* a harmonic of the cellar engine get pulled toward it. Partials far from any cellar harmonic are less affected. This creates **harmonic alignment without harmonic destruction** — the coupled engine's partials drift toward just-intonation alignment with the bass fundamental.

### The Orbital Model: Stable vs. Unstable

Not all partials fall into the attractor. Some orbit it. This is where the physics gets interesting:

- **Captured partials**: Partials close to a cellar harmonic (within ~15 cents) cross the capture radius and lock. They become resonant satellites — slightly detuned copies orbiting the cellar harmonic.
- **Escaped partials**: Partials far from any cellar harmonic (>80 cents away) experience weak gravity and remain free. They give the coupled engine its non-bass character.
- **Unstable orbits**: Partials at the midpoint between two cellar harmonics experience competing pulls and oscillate — creating natural beating and intermodulation between the two gravitational attractors.

Implementation approach: Use a per-partial frequency-domain attractor. For polyphonic audio, operate on the FFT spectral magnitudes, computing for each bin the net gravitational displacement, then re-synthesize with phase preservation. For real-time efficiency, limit computation to the top-N magnitude bins per frame.

### The Mass Parameter

CELLAR engine mass `M` is a synthesis parameter, not a static value. It should track:

- **Sub energy** — more energy below 80Hz = more mass
- **Note duration** — a bass note that has been sustaining for 2 seconds has more gravitational authority than a fresh attack. Mass accumulates with time.
- **Velocity** — harder attacks generate more initial mass, which then decays

This means: *the longer the bass note sustains, the more it pulls everything else toward itself.* A pedal tone at the start of a session exerts one level of gravity. The same pitch held for 30 seconds exerts vastly more. Everything in the mix gradually bends toward it.

### The Escape Velocity Concept

Every gravitational system has escape velocity — the speed needed to break free. In CELLAR coupling, this maps to:

**Escape velocity** = the transient energy needed for a coupled engine to temporarily override gravity.

A snappy attack (fast percussive transient) from a Garden string or Chef organ can momentarily exceed escape velocity and "launch" free of the gravitational field. The note begins in free space — its natural tuning — then falls back toward the cellar attractor as the transient decays. You hear the gravity working: the attack is free, the sustain bends toward the bass.

This is musically beautiful. The drummer's snare crack is free. The decay falls into the pocket.

### DSP Implementation Path

```
// Per-frame processing (coupled engine E under cellar C)
for each spectral bin f in E:
    f_nearest = find_nearest_harmonic(f_c, harmonic_series, f)
    distance = abs(f - f_nearest)
    capture_radius = 0.15 semitones * (1.0 + age_factor)

    if distance < capture_radius:
        // Captured: pull toward attractor
        displacement = (f_nearest - f) * G * M_cellar * capture_strength
        f_out = f + displacement * dt
    elif distance < escape_boundary:
        // Orbital: oscillating pull
        displacement = gravity_force(f, f_nearest) * sin(orbit_phase * dt)
        f_out = f + displacement
    else:
        // Escaped: minimal effect, natural tuning
        f_out = f × (1.0 - G * M_cellar * 0.001)

    update age_factor (integrates over note duration)
```

The `age_factor` is the secret weapon. It makes gravitational coupling temporal — bass notes don't just dominate space, they dominate time.

---

## Level 2 — Time-Based Transformation: Aging as Synthesis

Food science teaches that the most complex flavors are time-locked. You cannot rush a 10-year aged cheddar. You cannot compress a vintage Burgundy. The transformation requires time because the molecules themselves change — proteins denature, esters form, fatty acids oxidize. The chemistry is irreversible. You cannot un-age a thing.

Bass synthesis has never used real time. Every bass plugin sounds the same on minute one as on minute sixty. That is a failure of imagination.

### The Aging Synthesis Framework

CELLAR engines should operate on three time scales simultaneously:

**Scale 1: Note time** (milliseconds to seconds)
The ADSR envelope, as always. But for CELLAR engines, the *harmonic content* of the envelope should shift over note duration. A bass note that starts bright should gradually warm and darken not through a filter sweep, but through actual harmonic redistribution — odd harmonics giving way to even, upper partials decaying faster than the fundamental. This is acoustically correct for most bass instruments. Implement it properly.

**Scale 2: Session time** (minutes)
The engine accumulates a *harmonic history* during play. Every note played adds to a running spectrum of "what this player tends to play." This accumulated spectrum becomes a resonant ghost — a very subtle, very slow spectral bias that shifts the engine's character toward the player's own harmonic signature. After 20 minutes of playing root notes, the engine has learned the key. After 40 minutes of playing walking bass lines, it has learned the vocabulary.

This is not generative music. The notes still come from the player. But the tone develops — like a tube amp warming up, but deeper. The engine remembers.

Parameter name: **SESSION AGE** — a display-only readout showing how "warmed up" the engine is (0 = fresh, 100 = fully developed). Resets on session close, unless the player explicitly saves "aged state" to a preset.

**Scale 3: Accumulated play history** (sessions)
Optional, opt-in. The engine can write a compact "character fingerprint" to a local file — the harmonic bias, the favored register, the attack velocity distribution. On next load, the engine wakes up already warm. Like a well-played instrument that has developed its own voice from years of use.

This is the synthesizer as instrument that improves with age. No other bass plugin offers this. Most plugins are explicitly stateless — every session is identical. CELLAR engines develop character.

### The Fermentation Model

Fermentation is microbiology operating on sugar, producing acid and alcohol and CO2 — byproducts that change the entire nature of the substrate. The input is simple. The output is complex. The process is time and biology.

For XOlate (the Aged Wine / Analog Bass engine specifically):

**Fermentation mapping:**
- Sugar (simple input) → pure sine wave at fundamental
- Yeast (biological catalyst) → the analog circuit's nonlinearity
- Alcohol (complex byproduct) → even-harmonic saturation (the warm analog characteristic)
- Acid (sharp byproduct) → odd-harmonic edge (the Moog filter's bite)
- CO2 (volatile byproduct) → the slight noise floor, the breathing of analog circuits
- Time → literal time. The engine's harmonic complexity increases over note sustain.

**DSP implementation**: A "fermentation integrator" — a leaky integrator that accumulates nonlinear waveshaping at very low rates. Early in note time: mostly fundamental, minimal harmonics. Late in note time: harmonics have grown, cross-modulation has occurred, the simple tone has fermented into complexity. This is the opposite of most synth envelopes, which start complex (the attack transient) and simplify (the sustain). Fermented bass starts simple and grows rich.

### The Curing Model

Curing (meat, wood, leather) removes moisture — concentrating flavor, hardening structure, creating preservation through dessication. Applied to XOaken (the Acoustic/Upright Bass):

**Curing mapping:**
- Water content → high-frequency energy (the "freshness" of the attack)
- Curing agent (salt/smoke) → the drying of the attack over time
- Cured result → darker, drier, more fundamental-focused tone

A fresh-struck upright bass has a complex, noisy attack — the bow or pluck introduces substantial high-frequency transient energy. Over sustain, this dries out. The "cured" version of the same note is drier, darker, more fundamental. The CURING parameter controls how aggressively time removes high-frequency content from the sustain — a very slow, very deep filter that only operates during sustain (not attack, where the freshness lives).

### The Distillation Model

Distillation separates components by boiling point. The most volatile escape first. What remains is the concentrated essence. For XOmega (FM/Digital Bass):

**Distillation mapping:**
- Initial FM signal → complex, multi-operator, rich with modulation
- Heat (distillation) → mathematical reduction of the operator ratio tree
- First distillate → the bright, volatile overtones that evaporate early
- Late distillate → the concentrated fundamental, the heavy bottom-end essence

The DISTILLATION process for XOmega: over note sustain, the FM modulation index gradually decreases. The complex FM spectrum simplifies. The multi-operator tree collapses toward a simpler algorithm. What started as a Reese bass (rich, modulating, full of beating) finishes as a near-pure sine — the distilled essence of pitch. The process is irreversible within the note. Every note distills from complexity to purity.

---

## Level 3 — The Foundation That Listens: What CELLAR Does That No Bass Plugin Has Done

Every bass synthesizer is a soloist. It plays its notes. The notes leave. The synth forgets. The next note comes. The synth plays it without reference to what surrounded the last note.

This is acoustically absurd. Real bass instruments — upright bass, electric bass, bass guitar — are *reactive* instruments. The player listens to the drummer's kick, the rhythm of the chord stabs, the harmonic center being established. Bass lines are responses, not declarations.

**CELLAR introduces the Listening Architecture**: the bass engine receives real signals from other loaded engines and *responds* to what they play.

### The Reactive Foundation

CELLAR engines do not generate sound in isolation. They generate sound in response to what is happening above them.

**What CELLAR listens to:**

1. **Harmonic center** from coupled engines — the implied key from the notes being played by Garden strings, Chef organs, Kitchen pianos. CELLAR extracts the chroma distribution in real time and biases its own pitch toward the most statistically likely bass note for that harmonic context.

2. **Rhythmic density** from coupled engines — the number of transients per bar, derived from peak detection in the coupled engine's output. High rhythmic density → CELLAR becomes sparser, locking to key beats. Low rhythmic density → CELLAR can be more elaborate, filling the pocket.

3. **Dynamic center** from coupled engines — the RMS over the last 4 bars. If other engines are loud, CELLAR pulls back dynamically and spectrally (gravitational coupling becomes more subtle, making room). If other engines are quiet, CELLAR swells forward (the bass breathes when the room breathes).

4. **Spectral gap** from coupled engines — where in the frequency spectrum are the other engines NOT playing? CELLAR identifies the spectral gaps and biases its harmonic distribution toward filling them. This is not an EQ — it is CELLAR deliberately choosing to occupy the space that nobody else is occupying.

None of this overrides what the player is playing on the bass. The player's notes, velocity, expression — those are sacred. What changes is *how* those notes are synthesized: the harmonic content, the envelope shape, the saturation character, the spectral emphasis within the note. The player plays a C. The note is definitely a C. But the *character* of that C responds to what the rest of the band is doing.

This is what a great bassist does. They don't just play the root. They listen and shape the root to fit the moment.

### The Foundation Paradox

Here is the deepest CELLAR concept:

**The bass isn't the lowest note. It's the lowest *idea*.**

Most bass synthesis starts from the bottom and builds up — start with a sub-frequency oscillator and add harmonics on top. CELLAR inverts this: start with the full harmonic environment and find the foundational frequency that makes the most contextual sense.

The engine works by analyzing the harmonic series of *everything else in the patch* and computing the most structurally coherent bass note — the note that, if played below all the other voices, would make them all make sense. Then it plays that note. Or more precisely: it plays whatever the player is playing, but shapes the synthesis to *function* as the most coherent foundation for the current harmonic environment.

If you're playing a wrong note (a note that doesn't harmonically support the other engines), CELLAR doesn't correct you — but it resists. The synthesis is slightly thinner, slightly less resonant, slightly less "right-feeling" when you play against the harmonic field. When you hit the correct foundation note, the synthesis opens up — richer, more resonant, the harmonics locking in. The instrument rewards harmonic correctness not by auto-correcting you, but by sounding more alive when you're right.

**No bass synth has done this.** Every bass plugin treats the player as fully autonomous. CELLAR treats the bass player as part of a system — responsive, contextual, structurally aware.

### The WEIGHT / PATIENCE / PULL Architecture

Three new concepts unique to CELLAR engines:

**WEIGHT** — A continuous 0–1 parameter representing "how heavy" this bass sound currently is. Not amplitude. Not sub energy specifically. Weight is a composite of:
- Duration of current note (longer = heavier)
- Sub energy below 60Hz
- Harmonic lock-in ratio (more partials aligned to cellar harmonics = heavier)
- Session age factor

Weight is displayed as a real-time readout. It is also an output signal in the coupling matrix — other engines can receive WEIGHT as a modulation source. When CELLAR is heavy, you can modulate a coupled strings engine's filter cutoff with it (heavier bass → darker strings). Weight as modulation source is the coupling language of gravity.

**PATIENCE** — The inverse of responsiveness. A highly patient CELLAR engine changes slowly: slow attacks, long glide times, gradual harmonic evolution, infrequent reactive adjustments. An impatient CELLAR engine changes quickly: punchy attacks, fast reactive corrections, rapid aging. PATIENCE maps to the session time scale. Patient engines develop richer aged character (more time accumulating in each stable state). Impatient engines cover more harmonic territory but develop less depth in any one place.

This is a genuine musical philosophy embedded in a parameter. The slow bass player vs. the busy bass player. Charlie Mingus vs. Victor Wooten. Both valid. One parameter.

**PULL** — The gravitational coupling strength broadcast to other engines. This is already specified in Level 1. But as a first-class UI parameter on the CELLAR engine itself, PULL gives the bass player control over how much gravity they're exerting. Turn PULL to zero: the bass is a soloist, perfectly isolated, no gravitational effect. Turn PULL to maximum: everything else in the patch bends toward the CELLAR's fundamental. The bass player becomes the gravitational center of the entire patch.

---

## Recipe Connections Per Engine

The Translation Table in `recipe-design-process.md` is the key. Here is the bridge sentence for each CELLAR engine:

**XOgre (Sub Bass / Root Vegetables)**
*"The way root vegetables grown in heavy clay soil develop dense, concentrated flavor through slow underground pressure is the same way sub-frequency sine oscillators develop physical presence — both exist below the glamorous surface, both are what the entire structure rests on."*

Recipe direction: Slow-cooked root vegetable dishes. Potatoes dauphinois (French — slow cream absorption), Clapshot (Scottish turnip/potato), Ethiopian gomen (collards with turmeric), Persian ash reshteh (thick root-vegetable noodle soup). The dish must be hearty, foundational, cheap, unglamorous, deeply satisfying. The sub-bass player's character.

Cultural territory: Northern European, Ethiopian highlands, Persian winter. Peasant food that became essential cuisine.

**XOlate (Analog Bass / Aged Wine — replacing XOxbow)**
*"The way wine develops complexity over years in an oak barrel — esters forming, harsh tannins mellowing, volatile acids finding equilibrium — is the same way analog circuits develop their sonic character through the slow heating of capacitors, the drift of transistors, the settling of a TB-303 into its own nonlinearity."*

Recipe direction: Wine-braised dishes. French boeuf bourguignon (Burgundy beef braise), Portuguese caldo verde (with a glass of Vinho Verde worked in), Georgian chakapuli (lamb with tarragon + white wine). The dish should include wine as a cooking ingredient — the bass sound literally aged by the same substance that names it.

Cultural territory: French, Iberian, South Caucasus. Wine-producing cultures. Terroir is the concept.

**XOaken (Acoustic/Upright Bass / Cured Wood)**
*"The way wood smoke slowly penetrates meat during curing — not violently, but through sustained, patient contact — transforming the surface chemistry while leaving the interior pure is the same way a bowed wooden bass body transfers energy from string to air: the wood absorbs, transforms, and re-radiates with its own molecular character."*

Recipe direction: Smoked and cured foods. Scandinavian gravlax (salmon cured in salt/sugar/dill — no heat, pure time), Hungarian füstölt szalonna (smoked pork backfat), American smoked brisket (Texas low-and-slow), Japanese katsuobushi (smoked, fermented, dried bonito — the bass note of dashi). The dish should involve time and smoke or salt as the transformative agents.

Cultural territory: Scandinavian, Central European, Southern US, Japanese. Preservation traditions.

**XOmega (FM/Digital Bass / Distillation)**
*"The way distillation purifies a fermented liquid through repeated boiling — removing impurities and volatile compounds until only the essential alcohol remains — is the same way FM synthesis, at high modulation indices, can be walked backward to reveal the pure mathematical carrier: every complex FM bass sound is a distillation away from simplicity."*

Recipe direction: Distilled spirits as cooking ingredients. Scottish cock-a-leekie with whisky, French flambe dishes (banana foster, steak Diane), Korean sundubu jjigae finished with soju, Italian risotto with grappa reduction. The dish should use distilled spirits — alcohol that has already been through the distillation process.

Cultural territory: Scottish, French, Korean, Italian. Distillation cultures. The alchemy of reduction.

---

## Parameter Vocabulary — The CELLAR Lexicon

The overview document lists 6 parameters. These are correct as starting points but insufficient. Here is the full vocabulary, organized by time scale:

### Per-Note Parameters (ADSR territory)

| Parameter | Range | Description |
|-----------|-------|-------------|
| **Depth** | 0–100 | Sub-energy below 60Hz. Not volume — sub presence. The mass of the note. |
| **Character** | Clean → Driven | Saturation character from transparent to aggressive overdrive |
| **Strike** | Pluck → Bow → Sine | Attack type — how the note begins (transient shape, not just ADSR) |
| **Glide** | 0–5000ms | Portamento between notes. At long values: the bass slides, never jumps. |
| **Body** | Closed → Open | Resonance of the acoustic body (XOaken mostly; XOlate as "circuit resonance") |
| **Age** | 0–100 | Per-note aging speed: how fast the note ferments/distills/cures during sustain |

### Session Parameters (Session time scale)

| Parameter | Range | Description |
|-----------|-------|-------------|
| **Session Age** | 0–100 | Read-only display. How warm/developed the engine currently is. |
| **Patience** | Impatient → Patient | Rate of reactive adaptation. Slow = deep character development. Fast = nimble. |
| **Memory** | None → Long | How far back the harmonic history extends (affects reactive behavior) |

### Coupling Parameters (Cross-engine territory)

| Parameter | Range | Description |
|-----------|-------|-------------|
| **Pull** | 0.0–1.0 | Gravitational coupling broadcast strength to other engines |
| **Listen** | 0.0–1.0 | How much the engine adapts synthesis to coupled-engine harmonic context |
| **Escape Velocity** | Low → High | How much transient energy is needed for coupled engines to momentarily escape gravity |
| **Weight** | Read-only | Current gravitational mass readout. Also available as coupling modulation output. |

### Per-Engine Specialty Parameters

**XOgre (Sub Bass)**:
- `Tectonic` — low-frequency movement speed: continental drift vs. earthquake (sub-frequency LFO rate, extremely low)
- `Density` — how much sub-frequency energy is truly below hearing (pure infrasound presence)
- `Soil` — filter character (clay → sandy → rocky: each has different resonance)

**XOlate (Analog Bass)**:
- `Vintage` — where on the analog timeline (early transistor → Moog → TB-303 → modern → unknown future)
- `Warmth` — tube vs. transistor character
- `Terroir` — the regional "flavor" of the circuit voice (West Coast cool, East Coast grit, UK mid-forward, Japanese transparent)

**XOaken (Acoustic Bass)**:
- `Bow Pressure` — arco character (light harmonic → heavy, grinding col legno)
- `String Tension` — gut vs. steel vs. synthetic (different harmonic series weighting)
- `Room` — the acoustic space the instrument sits in (studio dead → jazz club → concert hall)

**XOmega (FM/Digital Bass)**:
- `Algorithm` — FM operator tree structure (simple carrier:modulator → complex 4-op network)
- `Ratio` — carrier:modulator frequency ratio (the tuning of the distillation)
- `Distill` — how quickly the FM complexity reduces toward pure carrier during sustain

---

## Gravitational Coupling Mechanics — Complete Specification

### Coupling Verb: GRAVITY

When any engine couples to a CELLAR engine using the **Gravity** verb:

**What the CELLAR engine sends:**
1. `gravity_fundamental` — the current pitch (MIDI note + pitch bend) as a frequency value
2. `gravity_mass` — the current WEIGHT parameter value (0.0–1.0)
3. `gravity_harmonics[8]` — the amplitude-weighted harmonic series (first 8 harmonics)
4. `gravity_pull` — the PULL parameter value (the overall coupling strength coefficient)

**What the receiving engine does with it:**

The MegaCouplingMatrix routes the gravity signals to a **GravityCouplingProcessor** in the receiving engine. This processor:

1. For each active voice in the receiving engine, computes spectral displacement toward the nearest CELLAR harmonic
2. Scales displacement by `gravity_pull × gravity_mass`
3. Applies displacement as pitch modulation to partials within the capture radius
4. Applies subtle volume attenuation to voices that are spectrally "far" from any CELLAR harmonic (they sound slightly thinner when not supported by the bass)
5. Applies subtle volume boost to voices that lock onto a CELLAR harmonic (they sound richer when the bass is underneath them)

**The temporal dynamics:**

```
gravity_mass accumulates during note sustain:
  mass(t) = min(1.0, mass(t-1) + delta_t × M_accumulation_rate)
  M_accumulation_rate = depth × 0.1 Hz (slow integration, ~10 seconds to full mass)

gravity_mass decays after note release:
  mass(t) = mass(t-1) × decay_coefficient
  decay_coefficient = exp(-delta_t / M_release_time)
  M_release_time = patience × 5.0 seconds (patient engines hold their gravity longer)
```

A patient, deep-sub bass note held for 8 seconds will reach near-maximum gravitational mass. The coupled engines above it will have been steadily pulled toward its harmonic series for those 8 seconds. When the note releases, the gravity lingers proportional to PATIENCE — it doesn't snap off. The gravity decays, and the coupled engines gradually regain their free tuning.

This is audible. You can feel it happen. The moment the bass player lifts their finger, the other instruments subtly "float" back to their natural tuning. A 16th-note bass figure won't have time to accumulate mass. A whole-note or held pedal point builds enormous gravitational authority.

### The Dissolve Coupling (CELLAR × BROTH)

When CELLAR couples with BROTH engines (the pad/atmosphere quad), the coupling verb is **Dissolve** — the foundation doesn't dominate the medium, it infuses it. The broth pad absorbs the bass's harmonic character and becomes harmonically saturated with it. Over time, the pad sounds like it was made from the bass — they share the same fundamental, the same overtone weighting.

The DSP: BROTH engines receive the gravity harmonics vector and slowly shift their internal spectral envelope toward alignment. The rate is very slow — over 4-8 bars at typical tempos. The result: start a session with CELLAR and BROTH loaded, and by the end of the first A section, the pad has dissolved into the harmonic world the bass established.

### The Root Coupling (GARDEN × CELLAR)

Garden strings couple to CELLAR via **Root** — the strings ground themselves in the bass frequency. The lowest voice of any Garden string engine automatically shifts toward the CELLAR fundamental × 2 (one octave above). This isn't a transposition — the player can still play any note. But the intonation on open strings, the resonance of the body, the harmonic series of the string vibration itself all subtly shift toward the CELLAR's root. The string instruments "tune themselves" to the bass.

---

## The CELLAR Identity Card

**What the Cellar is:** The foundation. The part of the kitchen you don't see. The thing that makes everything else make sense.

**What CELLAR engines have that no bass plugin has:**
1. Gravitational coupling that bends other engines' harmonics toward the bass fundamental
2. Temporal synthesis — bass notes that age, ferment, cure, and distill over their sustain duration
3. Session-scale character development — the engine warms up and develops identity over a playing session
4. Reactive listening — synthesis character responds to what other engines are playing harmonically and rhythmically
5. WEIGHT as a coupling modulation output — the bass player broadcasts gravitational authority that other engines receive as modulation

**What the Cellar is NOT:**
- Not a low-pass filter applied to other engines
- Not a side-chain compressor making room for the bass
- Not a simple frequency-domain limiter below 200Hz
- Not the bass as a separate isolated element

**The core statement:** The Cellar is the frequency territory that holds up everything else. When it plays, the room settles. When it stops, everything floats. This is not metaphor — it is the DSP architecture.

---

## Accent Color Assignments

These names and colors need finalization for the registry:

| Engine | Full Name | Accent Color | Hex |
|--------|-----------|-------------|-----|
| XOgre | XOgre (Root Cellar) | Deep Earth Brown | `#4A2C0A` |
| XOlate | XOlate (Aged Wine Analog) | Burgundy | `#6B1A2A` |
| XOaken | XOaken (Cured Wood Acoustic) | Dark Walnut | `#3D2412` |
| XOmega | XOmega (Distillation Digital) | Copper Still | `#B04010` |

The CELLAR quad's palette: dark, warm, deep, earthy. No bright colors. These are cellar instruments.

---

## Open Questions for Architecture Session

1. **GravityCouplingProcessor location**: Does it live in MegaCouplingMatrix (centralized) or in each receiving engine (distributed)? Centralized is cleaner but requires the Matrix to understand engine-internal spectral structure. Distributed puts spectral-domain logic in engines that don't currently have it.

2. **FFT or pitch-domain?** The spectral gravity model requires either FFT-domain processing (per-bin displacement) or a pitch-domain approximation (per-voice fundamental displacement only). FFT is accurate but expensive. Pitch-domain is cheap but ignores overtone-level gravity.

3. **Session Age persistence**: Where does session age state live? AudioProcessorValueTreeState doesn't persist session-granular data across DAW saves normally. Need a separate persistence mechanism — perhaps a companion `.xosession` file written alongside the DAW project.

4. **Reactive listening**: Does CELLAR receive audio from other engines (requiring audio routing), or does it receive parameter state (lighter weight, just coupling metadata)? Audio routing is accurate but architecturally heavier. Parameter state is an approximation but fits existing coupling infrastructure.

5. **XOmega vs. XOmega conflict**: Check if "OMEGA" (XOmega) conflicts with any existing engine shortname. Current engine table has no OMEGA — clean.

6. **XOgre prefix**: `ogre_` is available in the prefix registry. Confirm.

7. **XOlate prefix**: `olate_` — check the registry. Not yet assigned.

8. **XOaken prefix**: `oaken_` — check the registry. Not yet assigned.

9. **XOmega prefix**: `omega_` — check the registry. Not yet assigned.

---

*Written by The Visionary, March 2026.*
*Three levels down. The gravity is real. Build this.*
