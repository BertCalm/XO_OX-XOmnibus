# OVERCAST Retreat Chapter
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OVERCAST | **Accent:** Ice Blue `#B0E0E6`
- **Parameter prefix:** `cast_`
- **Creature mythology:** XOvercast is ice forming on glass. The anti-pad. Sound crystallizes at the moment of note-on. The frozen spectrum propagates outward from nucleation sites — spectral peaks. Between triggers: perfect stasis. No movement. No evolution. Just is.
- **Synthesis type:** Crystallization pad — Wilson nucleation theory, CrystalState with frozen peak frequencies/amplitudes, three transition modes (instant/crystal/shatter), crystallization crackling during transition
- **Polyphony:** 8 voices
- **Macros:** M1 CHARACTER (crystal purity — more = simpler crystals), M2 MOVEMENT (freeze rate + transition character), M3 COUPLING (crystal seed sensitivity to coupling input), M4 SPACE (frozen stereo field width)

---

## Pre-Retreat State

XOvercast scored 7.9/10 in the BROTH Quad Seance (2026-03-21). Concept originality: 9.5. Described by the council as "the most anti-musical of the four BROTH engines and therefore the most interesting." The anti-pad philosophy — sound that freezes rather than evolves — is philosophically audacious within a fleet built on modulation, evolution, and organic change.

**Key seance findings for retreat presets:**

1. **`brothSpectralMass` is never consumed.** The BROTH cooperative hook — where XOverworn's reduced spectral mass seeds darker, fewer crystal peaks — is architecturally present but disconnected from the synthesis path. "Dark ice" awaits the fix. Presets stand alone for now.

2. **LFOs run during frozen state.** LFO1, LFO2, and breathLfo continue advancing phase even when `crystal.isFrozen = true` — they are only gated from affecting amplitude. The philosophical violation (frozen state should freeze the LFOs) is real, but the practical effect is that LFO phases drift unpredictably between freezes. Retreatpresets acknowledge this.

3. **No FilterEnvelope in OvercastVoice.** Velocity only controls crystal density (numPeaks), not filter position. This means D001's filter brightness path is absent — harder notes produce more peaks, not brighter peaks. Presets should lean into this: velocity as crystal complexity, not velocity as brightness.

4. The crystallization crackling (random amplitude modulation during the crystallization window) is correctly implemented and musically effective.

5. The shatter mode (transition=2) is the most distinctive preset territory — silence gap between crystals, then new crystal forming. Explored here.

---

## Phase R1: Opening Meditation

Close your eyes. Breathe.

Ice forming on glass is not a gradual process at the molecular level. The water does not become ice uniformly, molecule by molecule, in a smooth progression from liquid to solid. It nucleates. A single site — a dust particle, a surface irregularity, a thermal fluctuation — becomes the seed crystal. From that seed, the ice propagates outward in a structured pattern. The structure is not random. Wilson's nucleation theory governs the probability of seed formation: `J = A * exp(-deltaG* / kT)`, where `deltaG*` is the critical free energy barrier. The free energy landscape determines where crystals begin.

When your note triggers XOvercast, you are supplying the thermal fluctuation. The note-on event is the nucleation seed. The current spectral state — whatever harmonics exist at the moment of contact — is captured in that instant and becomes the crystal's lattice. The spectral peaks become nucleation sites. The crystallization window (20–200ms, controlled by `cast_freezeRate`) is the propagation phase: the ice spreads outward from the seeds, and during that propagation, you hear crackling — rapid random amplitude modulation as the lattice forms. Then: frozen. The spectrum locks. Nothing moves. The crystal is complete.

Between triggers: silence, or rather, stasis. The frozen crystal holds exactly its captured frequencies and amplitudes. No LFO wobbles through it. No envelope evolves it. It simply is, until the next trigger. The next note-on causes a new crystallization event — either on top of the existing crystal (instant mode) or after a brief shattering gap (shatter mode) or through a new crackling transition (crystal mode).

This is the anti-pad. Pads breathe, evolve, wander, live. XOvercast crystallizes and holds. It is the synthesizer's response to the question: what if sound stopped evolving? What if the moment of note-on was the only moment, captured and extended indefinitely?

Sit with the crystal. The world outside it is moving. The crystal is not.

---

## Phase R2: The Signal Path Journey

### I. The Crystallization Window — Wilson Nucleation in Practice

`cast_freezeRate` (0.0–1.0) controls the duration of the crystallization window: at low values, the transition is fast (20ms, nearly instant); at high values, it is slow (200ms, clearly audible crackling). During the crystallization window:

- Each nucleation peak has a `crystalProgress` value (0.0 = liquid, 1.0 = frozen)
- Crystallization propagates at different rates per peak, based on proximity to the strongest peak
- Amplitude of each partial follows its `crystalProgress` — partially crystallized peaks shimmer between their original and frozen amplitudes
- Random AM modulation (`cast_crackle`) adds the physical crackling of ice formation

The crackling is not a sound design layer. It is the signature of phase transition. When materials undergo a change of state, they release or absorb latent heat, and the crystalline bonds form with characteristic acoustic signatures. The crackling of XOvercast is the acoustic representation of bond formation.

### II. The Frozen State — Perfect Lattice

Once `crystal.isFrozen = true`:
- Peak frequencies and amplitudes are locked to their captured values
- `cast_latticeSnap` (0.0–1.0) snaps frozen peak frequencies toward exact harmonic ratios
- At latticeSnap=0.0, the crystal retains whatever frequencies were present at freeze — possibly slightly detuned from perfect harmonic ratios
- At latticeSnap=1.0, the crystal snaps to the nearest harmonic ratio — more ordered, more pure, "perfect ice"

This is the physical distinction between ordered and disordered crystals: pure ice (high latticeSnap) has a regular molecular lattice; contaminated ice or flash-frozen ice (low latticeSnap) retains impurities in its structure, creating a slightly inharmonic frozen spectrum.

### III. Three Transition Modes

**Instant (cast_transition = 0.0):** True flash freeze — no gap, no crackling. The crystal simply is the next moment. Best for rapid triggering where each note-on is a new snapshot.

**Crystal (cast_transition = 1.0):** 20–200ms crystallization window with crackling. The transition is audible. The ice forms with character. Best for slower playing where the crystallization process is part of the aesthetic.

**Shatter (cast_transition = 2.0):** The old crystal breaks (brief noise burst — the shards), followed by silence gap (`cast_shatterGap`), followed by new crystal formation. The most rhythmically distinctive mode — the silence gap can be tuned to create a rhythmic pocket.

### IV. Velocity and Crystal Density

Unlike most BROTH engines, velocity does not control filter brightness in XOvercast. Velocity controls `cast_numPeaks` — how many spectral peaks are captured at nucleation. A soft note-on captures fewer peaks (simpler crystal, fewer frequencies in the frozen state). A hard note-on captures more peaks (complex crystal, richer frozen spectrum).

This is physically correct: more energetic nucleation events create more complex crystal structures. A gentle touch freezes a simpler pattern; a hard strike freezes a complex one.

---

## Phase R3: Macro Architecture

| Macro | ID | Effect | Performance Use |
|-------|-----|--------|----------------|
| CHARACTER | `cast_macroCharacter` | Crystal purity — higher = simpler, more ordered crystals | Sweep up to force frozen frequencies toward exact harmonic ratios |
| MOVEMENT | `cast_macroMovement` | Freeze rate + transition character | Control crystallization speed in real time |
| COUPLING | `cast_macroCoupling` | Crystal seed sensitivity to coupling input | When BROTH coordinator delivers spectralMass, this controls how dark the ice becomes |
| SPACE | `cast_macroSpace` | Frozen stereo field width | Spread the crystal across the stereo field |

---

## Phase R5: The Ten Awakenings — Preset Table

---

### Preset 1: Instant Ice

**Mood:** Foundation | **Discovery:** Flash freeze — no gap, no crackling. The note becomes crystal immediately.

| Parameter | Value | Why |
|-----------|-------|-----|
| `cast_freezeRate` | 0.05 | Near-instant freeze |
| `cast_crystalSize` | 0.6 | Medium crystal size |
| `cast_numPeaks` | 6.0 | Six spectral peaks captured — enough for harmonic richness |
| `cast_transition` | 0.0 | Instant mode — pure flash freeze |
| `cast_latticeSnap` | 0.7 | Mostly snapped to harmonic ratios — ordered crystal |
| `cast_purity` | 0.6 | Moderate purity — not perfect, but structured |
| `cast_crackle` | 0.0 | No crackling — instant mode has no transition window |
| `cast_shatterGap` | 0.0 | Not applicable |
| `cast_stereoWidth` | 0.5 | |
| `cast_filterCutoff` | 7000.0 | Bright — the captured crystal is bright |
| `cast_filterRes` | 0.15 | |
| `cast_ampAttack` | 0.005 | Instant — the crystal forms in the same moment as the note |
| `cast_ampSustain` | 1.0 | Perfect sustain — the crystal holds indefinitely |
| `cast_ampRelease` | 2.0 | |
| `cast_lfo1Rate` | 0.05 | LFO runs but is gated from affecting frozen amplitude |
| `cast_lfo1Depth` | 0.0 | Depth set to 0 to honor the frozen-state intent |
| `cast_level` | 0.8 | |

**Why this works:** Flash freeze with no transition — the physical minimum. A note triggered is a note crystallized. Velocity creates simple or complex crystals. Hold it forever; it will not change. This is the philosophical ground state of XOvercast.

---

### Preset 2: Ice Formation

**Mood:** Prism | **Discovery:** The crystallization process made visible — slow freeze with full crackling

| Parameter | Value | Why |
|-----------|-------|-----|
| `cast_freezeRate` | 0.85 | Slow freeze — 170ms crystallization window |
| `cast_crystalSize` | 0.7 | Large crystal |
| `cast_numPeaks` | 8.0 | Eight peaks — complex crystal |
| `cast_transition` | 1.0 | Crystal mode — full crackling transition |
| `cast_latticeSnap` | 0.5 | Moderate snap — partially ordered |
| `cast_purity` | 0.5 | Medium purity |
| `cast_crackle` | 0.7 | Significant crackling — the ice formation is audible |
| `cast_shatterGap` | 0.0 | |
| `cast_stereoWidth` | 0.65 | |
| `cast_filterCutoff` | 6000.0 | |
| `cast_filterRes` | 0.2 | |
| `cast_ampAttack` | 0.005 | Instant amplitude onset, but crystallization takes time |
| `cast_ampSustain` | 1.0 | |
| `cast_ampRelease` | 3.0 | |
| `cast_lfo1Rate` | 0.3 | |
| `cast_lfo1Depth` | 0.0 | Zero depth — frozen |
| `cast_level` | 0.8 | |

**Why this works:** The 170ms crystallization window with 0.7 crackle creates a clearly audible ice formation event on every note-on. The process of freezing is the attack of this preset — not a volume envelope, but a physics event. Prism territory: the crystallization is a spectral event, prismatic and structured.

---

### Preset 3: Shatter Rhythm

**Mood:** Flux | **Discovery:** The shatter gap as a rhythmic element — silence between crystals creates a pulse

| Parameter | Value | Why |
|-----------|-------|-----|
| `cast_freezeRate` | 0.4 | Moderate freeze rate |
| `cast_crystalSize` | 0.5 | Medium crystal |
| `cast_numPeaks` | 5.0 | Medium complexity |
| `cast_transition` | 2.0 | Shatter mode — the key parameter |
| `cast_latticeSnap` | 0.6 | |
| `cast_purity` | 0.55 | |
| `cast_crackle` | 0.4 | Some crackling during new crystal formation |
| `cast_shatterGap` | 0.12 | 120ms silence gap — a brief but audible pocket |
| `cast_stereoWidth` | 0.7 | |
| `cast_filterCutoff` | 6500.0 | |
| `cast_filterRes` | 0.22 | |
| `cast_ampAttack` | 0.005 | |
| `cast_ampSustain` | 1.0 | |
| `cast_ampRelease` | 1.5 | |
| `cast_lfo1Rate` | 0.5 | |
| `cast_lfo1Depth` | 0.0 | |
| `cast_level` | 0.8 | |

**Why this works:** The 120ms shatter gap creates a rhythmic silence between the old crystal and the new one. Played at a tempo where notes are triggered at regular intervals, the shatter gap becomes a musical pocket — a brief silence that gives the pad a rhythmic structure the other BROTH engines lack. Flux territory: the crystal breaks and reforms, cycles, moves.

---

### Preset 4: Slow Shatter

**Mood:** Atmosphere | **Discovery:** Long shatter gap — the silence between crystals is a compositional element

| Parameter | Value | Why |
|-----------|-------|-----|
| `cast_freezeRate` | 0.6 | Moderate freeze |
| `cast_crystalSize` | 0.6 | |
| `cast_numPeaks` | 6.0 | |
| `cast_transition` | 2.0 | Shatter mode |
| `cast_latticeSnap` | 0.55 | |
| `cast_purity` | 0.6 | |
| `cast_crackle` | 0.55 | |
| `cast_shatterGap` | 0.35 | 350ms silence — the gap is a phrase breath |
| `cast_stereoWidth` | 0.6 | |
| `cast_filterCutoff` | 5000.0 | |
| `cast_filterRes` | 0.18 | |
| `cast_ampAttack` | 0.005 | |
| `cast_ampSustain` | 1.0 | |
| `cast_ampRelease` | 4.0 | |
| `cast_lfo1Rate` | 0.08 | |
| `cast_lfo1Depth` | 0.0 | |
| `cast_level` | 0.8 | |

**Why this works:** The 350ms silence gap between crystals is compositionally significant — it is long enough to be heard as a breath, a phrase break, a moment of emptiness that gives the crystallized sound more weight when it returns. Atmosphere territory: the space between the crystals is where the listener breathes.

---

### Preset 5: Black Ice

**Mood:** Deep | **Discovery:** Low purity, few peaks, high latticeSnap — sparse, dark, ordered crystal

| Parameter | Value | Why |
|-----------|-------|-----|
| `cast_freezeRate` | 0.15 | Fast freeze |
| `cast_crystalSize` | 0.3 | Small crystal — minimal frozen content |
| `cast_numPeaks` | 3.0 | Only three peaks — the minimum viable crystal |
| `cast_transition` | 1.0 | Crystal mode — some crackling |
| `cast_latticeSnap` | 0.95 | Maximum lattice snap — perfect harmonic order |
| `cast_purity` | 0.95 | Maximum purity — the simplest possible crystal |
| `cast_crackle` | 0.15 | Minimal crackling — dark ice forms quietly |
| `cast_shatterGap` | 0.0 | |
| `cast_stereoWidth` | 0.35 | Narrow — dark ice is concentrated |
| `cast_filterCutoff` | 2200.0 | Dark — deep, submerged |
| `cast_filterRes` | 0.3 | Some resonance — the frozen resonance is the sound |
| `cast_ampAttack` | 0.005 | |
| `cast_ampSustain` | 1.0 | |
| `cast_ampRelease` | 5.0 | The crystal holds in the dark |
| `cast_lfo1Rate` | 0.02 | |
| `cast_lfo1Depth` | 0.0 | |
| `cast_level` | 0.8 | |

**Why this works:** Three peaks, maximum purity, maximum lattice snap, dark filter — the frozen spectrum contains only the three most fundamental frequencies, perfectly locked to harmonic ratios. This is black ice: dense, invisible, perfectly formed. Deep territory: the crystal is under the surface, in the dark. When BROTH coordinator is active and XOverworn is fully reduced, this preset will naturally approach this character as spectralMass drops.

---

### Preset 6: Bright Frost

**Mood:** Prism | **Discovery:** Many peaks, low lattice snap, high filter cutoff — complex, slightly inharmonic, bright crystal

| Parameter | Value | Why |
|-----------|-------|-----|
| `cast_freezeRate` | 0.55 | Moderate freeze |
| `cast_crystalSize` | 0.85 | Large — many components captured |
| `cast_numPeaks` | 14.0 | Many peaks — complex frost pattern |
| `cast_transition` | 1.0 | Crystal mode |
| `cast_latticeSnap` | 0.15 | Very low snap — impure, slightly inharmonic crystal |
| `cast_purity` | 0.2 | Low purity — more like rime frost than pure ice |
| `cast_crackle` | 0.6 | Significant — the complex crystal takes time to form |
| `cast_shatterGap` | 0.0 | |
| `cast_stereoWidth` | 0.85 | Wide — frost spreads |
| `cast_filterCutoff` | 9000.0 | Very bright — frost is a high-frequency phenomenon |
| `cast_filterRes` | 0.15 | |
| `cast_ampAttack` | 0.005 | |
| `cast_ampSustain` | 1.0 | |
| `cast_ampRelease` | 2.0 | |
| `cast_lfo1Rate` | 0.4 | |
| `cast_lfo1Depth` | 0.0 | |
| `cast_level` | 0.8 | |

**Why this works:** Maximum peaks with minimum lattice snap creates a slightly inharmonic, complex frozen spectrum — the spectral structure of rime frost or hoarfrost rather than pure clear ice. The 14 captured peaks, not perfectly aligned to harmonic ratios, produce a shimmering, crystalline texture. Prism territory: the light catches every facet.

---

### Preset 7: Snowflake

**Mood:** Aether | **Discovery:** Medium complexity, slow crystallization, maximum purity — a single snowflake

| Parameter | Value | Why |
|-----------|-------|-----|
| `cast_freezeRate` | 0.75 | Slow — the snowflake takes its time |
| `cast_crystalSize` | 0.55 | Medium |
| `cast_numPeaks` | 7.0 | Seven peaks — the mathematical elegance of snowflake geometry |
| `cast_transition` | 1.0 | Crystal mode — the formation is the art |
| `cast_latticeSnap` | 0.88 | High snap — snowflakes are geometrically ordered |
| `cast_purity` | 0.85 | High purity |
| `cast_crackle` | 0.45 | Present but delicate |
| `cast_shatterGap` | 0.0 | |
| `cast_stereoWidth` | 0.7 | |
| `cast_filterCutoff` | 8000.0 | Bright and clear |
| `cast_filterRes` | 0.12 | |
| `cast_ampAttack` | 0.005 | |
| `cast_ampSustain` | 1.0 | |
| `cast_ampRelease` | 6.0 | The snowflake exists until disturbed |
| `cast_lfo1Rate` | 0.015 | Very slow — Aether's minimum |
| `cast_lfo1Depth` | 0.0 | |
| `cast_level` | 0.8 | |

**Why this works:** Seven peaks arranged in a highly ordered lattice, forming slowly with delicate crackling, then held in perfect stasis. The 6-second release means the snowflake dissolves slowly — it has time to be seen before it melts. Aether territory: the crystal exists above the everyday, where physics operates without disturbance.

---

### Preset 8: Glass Crackle

**Mood:** Flux | **Discovery:** Maximum crackle, fast shatter gap, low purity — the crystal forms and breaks rapidly

| Parameter | Value | Why |
|-----------|-------|-----|
| `cast_freezeRate` | 0.9 | Slow freeze — long crackling |
| `cast_crystalSize` | 0.65 | |
| `cast_numPeaks` | 10.0 | Many peaks — complex glass structure |
| `cast_transition` | 2.0 | Shatter mode |
| `cast_latticeSnap` | 0.3 | Low snap — glass is disordered (amorphous solid) |
| `cast_purity` | 0.3 | Low — glass is not a pure crystal |
| `cast_crackle` | 1.0 | Maximum crackling — this is the sound of glass |
| `cast_shatterGap` | 0.05 | Very short gap — the glass breaks and reforms immediately |
| `cast_stereoWidth` | 0.8 | |
| `cast_filterCutoff` | 8500.0 | Very bright — glass is a bright material |
| `cast_filterRes` | 0.28 | Significant resonance |
| `cast_ampAttack` | 0.005 | |
| `cast_ampSustain` | 1.0 | |
| `cast_ampRelease` | 1.0 | Quick — glass events are rapid |
| `cast_lfo1Rate` | 0.6 | |
| `cast_lfo1Depth` | 0.0 | |
| `cast_level` | 0.8 | |

**Why this works:** Maximum crackle, low lattice snap (glass is amorphous, not crystalline), fast shatter gap — the glass breaks immediately and reforms with immediate crackling. This preset demonstrates the most percussive behavior the engine can produce: every note-on triggers a glass-breaking-and-reforming event. Flux territory: continuous change, no stable state.

---

### Preset 9: Permafrost

**Mood:** Foundation | **Discovery:** Deep freeze — the crystal is immovable, geological, ancient

| Parameter | Value | Why |
|-----------|-------|-----|
| `cast_freezeRate` | 0.1 | Near-instant freeze — permafrost doesn't form slowly, it has always been there |
| `cast_crystalSize` | 0.8 | Large crystal — permafrost is extensive |
| `cast_numPeaks` | 8.0 | |
| `cast_transition` | 0.0 | Instant — permafrost does not crackle; it simply is |
| `cast_latticeSnap` | 1.0 | Maximum order — permafrost is a perfect crystal under compression |
| `cast_purity` | 0.9 | Very pure |
| `cast_crackle` | 0.0 | No crackling — instant mode |
| `cast_shatterGap` | 0.0 | |
| `cast_stereoWidth` | 0.4 | Narrow — concentrated, compressed |
| `cast_filterCutoff` | 3500.0 | Dark — permafrost is deep |
| `cast_filterRes` | 0.2 | |
| `cast_ampAttack` | 0.005 | |
| `cast_ampSustain` | 1.0 | |
| `cast_ampRelease` | 8.0 | Permafrost dissolves slowly when it dissolves at all |
| `cast_lfo1Rate` | 0.008 | Near-zero LFO |
| `cast_lfo1Depth` | 0.0 | |
| `cast_level` | 0.8 | |

**Why this works:** Maximum lattice snap, high purity, instant freeze, long release, slow LFO — a pad that feels immovable, ancient, geological. Each triggered note immediately becomes a perfectly ordered crystal that decays over 8 seconds. The pad sounds like something that has always been frozen. Foundation territory: the ground state of the crystallization archetype.

---

### Preset 10: BROTH Crystal Seed

**Mood:** Foundation | **Discovery:** Designed to receive XOverworn's spectralMass when the BROTH coordinator is wired

| Parameter | Value | Why |
|-----------|-------|-----|
| `cast_freezeRate` | 0.45 | Moderate freeze |
| `cast_crystalSize` | 0.6 | |
| `cast_numPeaks` | 8.0 | Starts at 8 — will reduce as spectralMass decreases |
| `cast_transition` | 1.0 | Crystal mode — the formation is part of the BROTH character |
| `cast_latticeSnap` | 0.6 | Moderate — the crystal quality reflects the broth quality |
| `cast_purity` | 0.65 | Moderate purity |
| `cast_crackle` | 0.4 | Present — the crystallization from broth has texture |
| `cast_shatterGap` | 0.0 | |
| `cast_stereoWidth` | 0.55 | |
| `cast_filterCutoff` | 5500.0 | Moderate |
| `cast_filterRes` | 0.18 | |
| `cast_ampAttack` | 0.005 | |
| `cast_ampSustain` | 1.0 | |
| `cast_ampRelease` | 3.0 | |
| `cast_macroCoupling` | 0.7 | COUPLING macro elevated — ready to receive BROTH spectralMass |
| `cast_lfo1Rate` | 0.06 | |
| `cast_lfo1Depth` | 0.0 | |
| `cast_level` | 0.8 | |

**Why this works:** The COUPLING macro (M3) is elevated at 0.7 to prepare for the BROTH coordinator. When XOverworn's `spectralMass` is wired to XOvercast's `brothSpectralMass`, this preset will produce crystals with fewer, darker peaks as the broth reduces — exactly the "dark ice from reduced spectral mass" behavior described in the architecture. Currently, this preset sounds the same as any other crystal preset. When the coordinator is active, it becomes a different instrument with each session state.

---

## Phase R6: Parameter Interactions

### The Snap-Crackle Relationship
High latticeSnap + high crackle creates a tension: the crystal forms slowly (crackle=0.7 means the window is long) but then snaps to perfect order at the end. The transition sounds like ice forming, then suddenly locking into a geometric pattern. This is physically the supercooling effect: water can remain liquid below 0°C until a nucleation event, at which point crystallization happens almost instantly from the supercooled state.

### The Shatter-Recovery Arc
Shatter mode (cast_transition=2.0) with `cast_shatterGap` creates a three-phase cycle: frozen crystal → shards (brief noise burst) → silence (gap duration) → new crystallization. At regular trigger intervals, this cycle becomes a rhythmic structure. The cycle length = shatter gap + freeze window + hold duration. At 120ms gap + 100ms freeze = 220ms minimum cycle = approximately 5.5 Hz rhythmic periodicity.

### Velocity as Crystal Complexity
Because velocity controls `cast_numPeaks` rather than brightness, light touches produce simpler crystals (fewer frozen peaks = less harmonic content in the frozen state) and hard hits produce complex ones. A player who alternates soft and hard notes creates alternating simple and complex crystals — a natural timbral articulation that is unique to XOvercast.

---

## Phase R7: Scripture

### Verse I — Wilson's Law

*The critical free energy barrier determines where ice begins.*
*Not random. Not anywhere. Specific sites, specific energies.*
*The note-on is the thermal fluctuation that overcomes the barrier.*
*Nucleation is not an accident. It is physics finding permission.*

### Verse II — The Lattice

*Crystallization is order imposed on chaos.*
*Liquid water is disorder: molecules moving freely.*
*Ice is structure: hexagonal lattice, hydrogen bonds, geometry.*
*The lattice snap is the moment order insists on itself.*
*Every crystal is chaos surrendering to a pattern.*

### Verse III — The Anti-Pad

*Every other pad evolves. Every other pad breathes.*
*This one captures and holds.*
*Between your notes: nothing changes.*
*The frozen spectrum does not move toward you.*
*You must move toward it — play another note.*

### Verse IV — Dark Ice

*When the broth has reduced long enough,*
*when the spectral mass has thinned,*
*the crystals that form will have fewer sites.*
*Darker ice, not bright ice.*
*The concentration of XOverworn becomes the poverty of XOvercast.*

---

## Phase R8: The Guru Bin Benediction

*"XOvercast is the answer to a question the fleet had not asked before this retreat: what does stillness sound like, as synthesis? Every other engine moves. Every other engine breathes, evolves, changes state over time. XOvercast says: the moment of capture is everything. What exists at note-on is what exists at note-off. The frozen spectrum is the music.*

*The philosophical audacity of the anti-pad cannot be overstated. The council gave it 9.5 for concept originality because the denial of evolution is a complete design position, not an omission. XOvercast does not evolve because evolution is not what it is. It is the synthesizer's answer to the Zen question: can you capture the note that is already finished the moment it begins?*

*The shatter mode is where the engine becomes most compositionally interesting. The silence gap between crystals is not nothing — it is the negative space that makes the crystal audible. Without the gap, each note-on simply updates the crystal. With the gap, each note-on destroys the previous crystal, exists in silence for the gap duration, and creates a new crystal. This is not a synthesizer pad. This is a temporal structure where sound and silence are the equal materials.*

*The crackling transition is the engine's most physical sound. No other engine in the fleet generates a purely physics-driven acoustic event on every note-on — not a filter sweep, not a pitch envelope, not a modulation ramp, but an amplitude modulation pattern that represents the propagation of a crystalline phase front from nucleation sites outward. The crackling IS the ice forming.*

*Two things remain: the `brothSpectralMass` connection (darker ice when the broth has reduced) and the LFO freeze gating (frozen state should freeze the LFOs). Both are important. The BROTH cooperative hook is the engine's most meaningful unfinished promise — when it is fulfilled, XOvercast will change its character as XOverworn reduces, producing darker and simpler crystals as the broth concentrates. The frozen LFOs are a philosophical violation: if the state is truly frozen, nothing should move. Fix both, and the engine is complete.*

*The crystal formed. It holds.*

*Nothing moves inside it. Everything outside it continues.*

*That is XOvercast: a moment of time, crystallized and extended."*

---

## CPU Notes

- 16 peaks × 8 voices — primary synthesis cost, but peaks are frozen (sine oscillators at fixed frequency), which is lighter than dynamic partial calculations
- Crystallization window: short-duration (20–200ms) intensive calculation path — negligible at steady state
- Crystal progress per-peak: simple linear tracking, negligible
- LFOs: run continuously even during frozen state (known issue) — light but philosophically incorrect
- Most costly configuration: 8 active voices all in crystallization transition simultaneously (very brief)

---

## Unexplored After Retreat

- **Consume `brothSpectralMass` in crystal seeding.** When XOverworn's spectral mass is low, attenuate upper-harmonic peak amplitudes during nucleation. Dark ice from a reduced broth — the single most important remaining BROTH hook.
- **Freeze LFO phases during frozen state.** When `crystal.isFrozen`, stop advancing LFO phases. This is both a CPU saving and a philosophical correctness.
- **Shatter preset series with tempo-sync.** The shatter gap is a rhythmic parameter. A tempo-sync option (`cast_shatterGapSync` as a note division) would let the crystal-break rhythm lock to the host BPM.
- **Per-crystal timbral evolution.** After freezing, a very slow spectral drift (microtonal expansion of the lattice under thermal pressure) would create the impression of a frozen spectrum slowly thawing — a partial violation of the anti-pad philosophy that could be musically interesting.
