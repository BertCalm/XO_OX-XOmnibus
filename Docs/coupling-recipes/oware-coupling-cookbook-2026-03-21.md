# XOware Coupling Cookbook
**Date:** 2026-03-21
**Author:** XO_OX Designs
**Engine:** OWARE — "The Resonant Board"
**Accent:** Akan Goldweight `#B5883E`
**Param Prefix:** `owr_`

---

## XOware as a Coupling Partner

XOware's `applyCouplingInput` accepts four types as a **destination** (receiving engine):

| Type | Internal Effect | Scaling |
|------|----------------|---------|
| `AmpToFilter` | `couplingFilterMod` → adds to effective brightness (`owr_brightness`) | ×2000 Hz |
| `LFOToPitch` | `couplingPitchMod` → modulates all active voice frequencies | ×2.0 semitones |
| `AmpToPitch` | `couplingPitchMod` → same pitch bus, additive | ×1.0 semitone |
| `EnvToMorph` | `couplingMaterialMod` → shifts `effectiveMaterial` (wood→metal axis) | ±1.0 normalized |

XOware's **output** (as source engine) is the post-limiter stereo pair via `getSampleForCoupling`, representing the full resonant board audio — the sum of all active bar modes, body resonance, buzz membrane, and shimmer.

The engine's key modulation surfaces for incoming coupling:
- **Material Continuum** (`owr_material`, 0→1: wood→metal): modulated by `EnvToMorph`
- **Brightness / filter** (`owr_brightness`, 200–20000 Hz): driven by `AmpToFilter`
- **Pitch** (all active voices): modulated by `LFOToPitch` / `AmpToPitch`
- **Sympathy Network** (`owr_sympathyAmount`, 0–1): controlled by macro COUPLING (`owr_macroCoupling`)
- **Mallet Hardness** (`owr_malletHardness`, 0–1): controlled by aftertouch and macro MALLET

---

## Recipe 1: OWARE × OSTINATO — "The Fire Circle's Instrument"

> *The drum circle gathered around the sunken board. When the djembe struck, every bar of the oware rang in sympathy. The fire was not metaphor. The fire was the coupling.*

**Signal direction:** OSTINATO → OWARE (drum circle drives tuned percussion)
**Mythology:** XOstinato is the bioluminescent campfire shore gathering; XOware is the sunken board washed ashore beside it. The djembe's fundamental at 160–220 Hz is close to oware bar tunings — the sympathetic resonance network inside XOware responds to exactly that frequency range.

### Coupling Type

**`EnvToDecay` from OSTINATO → `EnvToMorph` into OWARE**

OSTINATO's amplitude envelope outputs (per-seat peak levels via `getSampleForCoupling`) carry rhythmic transient information — the peak, not sustained level, of each drum hit. Routed as `EnvToMorph` into OWARE, each drum hit sweeps `couplingMaterialMod`, momentarily shifting the bar material toward metal and back. The bars "ring out" with a slightly different timbre on every beat.

**Primary type:** `EnvToMorph`
**Secondary type:** `AmpToFilter` (adds shimmer brightness on peak hits)

### Amount Range

| Amount | Musical Result |
|--------|---------------|
| 0.08–0.15 | Subtle: material barely shifts; bars stay warm, slight brightness on loud hits |
| 0.20–0.35 | Equal partnership: each strong djembe strike noticeably opens bar brightness; clave/surdo pattern legible in bar decay envelope |
| 0.45–0.65 | Domination: drum hits fully control the material axis; quiet bars collapse to wood dampness, loud hits burst into metal sustain |

### Parameter Targets in OWARE

- `owr_material`: set to 0.15 (wood base) — coupling pushes it dynamically toward 0.7+
- `owr_sympathyAmount`: 0.50–0.70 — the sympathetic resonance network should be active so struck seats in OSTINATO create harmonic bloom in bars that share frequencies
- `owr_decay`: 1.8–2.5 s — long enough for bars to ring between hits
- `owr_shimmerRate`: 4–7 Hz — Balinese beat shimmer gives the joined texture a gamelan-meets-fire quality
- `owr_macroCoupling`: 0.6 — opens sympathy network during performance

### Parameter Targets in OSTINATO

- Seat 1 (Djembe, tone articulation): fundamental ~200 Hz — closest frequency match to oware bars in C3–G3
- `osti_macroFIRE`: 0.50 — medium exciter energy; loud enough to generate coupling signal
- `osti_macroGATHER`: 0.35 — slightly loose timing so hits don't all arrive simultaneously
- `osti_macroCIRCLE`: 0.30 — some inter-seat sympathetic resonance in ostinato mirrors oware's sympathy

### Preset Name Suggestions

- "Fire Circle" — neutral, mythological
- "Shore Balafon" — geographic, sonic
- "Bronze Seeds" — references oware seeds + metal material

### Musical Description

The drum circle plays its pattern. Simultaneously — without the performer doing anything — the oware board sings. The djembe's fundamental is near C3; when seat 1 triggers, bar modes that share partials with 200 Hz receive the coupling signal as a material push toward metal (longer upper-mode ring). The sympathetic resonance network then propagates outward to adjacent bars within ±300 Hz. The result sounds like a live balafon player sitting next to the drum circle and playing the same rhythm an octave higher — but nobody is playing the board. The coupling is the player.

At medium amounts (0.25), the OWARE rhythmic envelope is a soft shadow of the OSTINATO pattern. At high amounts (0.55+), the OWARE almost completely follows the drum envelope, and the two engines phase in and out of rhythmic unison, with oware bars ringing on a slightly delayed tail.

---

## Recipe 2: OWARE × ONSET — "Surface Resonance"

> *The 808 kick hit the water's surface. The ripple traveled down and struck the sunken board at exactly 55 Hz. Every bar of the board tuned to that harmonic rang for two seconds. The electronic drum had no idea it had just played a marimba.*

**Signal direction:** ONSET → OWARE (electronic drums trigger melodic bars)
**Mythology:** XOnset lives at the water surface — pure feliX transients. XOware is sub-surface — a physical object. ONSET's kick energy travels through water and excites resonant structures. This is literally acoustic physics: sub-bass pressure waves coupling into modal resonators.

### Coupling Type

**Primary: `AmpToFilter` (ONSET → OWARE)**
**Secondary: `EnvToMorph` (ONSET → OWARE)**

ONSET's `getSampleForCoupling` returns the post-character stereo mix. As `AmpToFilter`, drum transients drive `couplingFilterMod` into OWARE's effective brightness parameter, opening bar resonance on every hit. As `EnvToMorph` (secondary, lower amount), kick and snare transients push material toward metal — brighter, longer ring — on each hit.

ONSET's `RhythmToBlend` coupling also exists but has no target in OWARE's current `applyCouplingInput`; use the two confirmed types.

### Amount Range

| Amount | Musical Result |
|--------|---------------|
| `AmpToFilter`: 0.10–0.20 | Filter shimmer follows drum energy; subtle brightness follows kick/snare envelope |
| `AmpToFilter`: 0.30–0.50 | Bars visibly "open" on each kick; the board breathes with the drum machine |
| `EnvToMorph`: 0.05–0.12 | Material axis moves gently from wood toward metal on transients |
| `EnvToMorph`: 0.15–0.25 | Clear timbre shift on snare hits; bars ring with metallic brightness for ~200ms after each snare, then return to wood warmth |

### Parameter Targets in OWARE

- `owr_material`: 0.30 (warm wood-to-middle starting point)
- `owr_brightness`: 4000–6000 Hz base (coupling will push it 0–10kHz dynamically)
- `owr_filterEnvAmount`: 0.40 — internal filter envelope still active; the coupling modulates on top
- `owr_malletHardness`: 0.25 (soft base mallet; coupling opens brightness rather than initial strike)
- `owr_decay`: 1.2–1.8 s — medium decay; bars ring through the space between hits
- `owr_damping`: 0.20 — low damping, high ring persistence

### Parameter Targets in ONSET

- Voice 1 (kick): Layer X (Circuit), pitch ~55 Hz, high body, medium snap → strong coupling signal at sub-bass
- Voice 2 (snare): Layer O (Algorithm), modal resonator mode, high snap → transient-rich signal
- `perc_grit`: 0.30 — Character stage adds some warmth that carries through as coupling amplitude
- XVC: kick → snare (AmpToFilter at 0.15) — internal ONSET coupling creates an already-coupled drum kit before OWARE receives it

### Preset Name Suggestions

- "Surface Resonance" — literal physics description
- "Electronic Balafon" — genre collision
- "Kick-Struck Wood" — direct, percussive
- "Sub-Bass Marimba" — production-friendly name

### Musical Description

Set up a four-on-the-floor kick in ONSET, plus a snare on 2 and 4. OWARE plays the same root note as the kick's fundamental (D1 or E1). Without coupling: the drum machine plays, the marimba plays independently. With coupling active at 0.35 `AmpToFilter`: every kick pops the bars with a brightness surge — the marimba's cutoff opens, the upper bar modes bloom for ~100ms, then close back down. The snare creates a shorter, sharper opening. Rests between hits are warm and muted. The board "breathes" in time with the machine.

At higher amounts (0.50+), the bars lose their independence entirely. They become a filter-opened extension of the drum mix. The distinction between "drum" and "melodic percussion" collapses. This is the productive collapse: bar resonance is now a shaped frequency response to drum energy. Not a drum. Not a marimba. A resonating surface.

---

## Recipe 3: OWARE × OSPREY — "Shore Bells"

> *The osprey hunts at the surface. The oware board rests on the seafloor beneath the kelp. When the bird's turbulence modulates the shore, the board hears it as a change in water pressure — a slow sweep of material.*

**Signal direction:** OSPREY → OWARE (shore system modulates material continuum)
**Mythology:** XOsprey inhabits the surface zone — turbulent interface of wind and wave. XOware is the sunken artifact. OSPREY's fluid energy model outputs a continuous, slow-moving amplitude signal (shore-modulated resonator mix). As it morphs between 5 coastal regions (Atlantic → Nordic → Mediterranean → Pacific → Southern), its spectral character shifts. Routing this into OWARE as `EnvToMorph` means the material continuum of the bars follows the shore's timbral identity: Atlantic roughness = wood dampness, Pacific smoothness = metal sustain.

### Coupling Type

**Primary: `EnvToMorph` (OSPREY → OWARE)**
**Secondary: `AmpToFilter` (OSPREY → OWARE)**
**Tertiary: `LFOToPitch` (OSPREY → OWARE)**

OSPREY's swell-period LFO (driven by `osprey_swellPeriod`) is a slow cyclic modulator that, when routed as `LFOToPitch`, adds a very gentle pitch drift to all sounding bars — matching the tidal, breathing quality of the shore system. This is not vibrato; it is tide-breath, measured in minutes or hours, not seconds.

### Amount Range

| Amount (EnvToMorph) | Musical Result |
|--------------------|---------------|
| 0.10–0.18 | Material barely shifts with shore dynamics; bars stay warm; coast awareness is subliminal |
| 0.25–0.40 | Clear material shift as OSPREY shore morphs; Pacific preset → bars ring with metal sustain; Atlantic → wood dampness dominates |
| 0.50–0.70 | Shore completely controls material axis; OWARE becomes a material-morphing instrument driven by fluid energy, not keyboard input |

| Amount (AmpToFilter) | Musical Result |
|---------------------|---------------|
| 0.08–0.15 | Subtle brightness follows sea state amplitude; calmer OSPREY = darker bars |
| 0.20–0.35 | Sea-state swells audibly open bar brightness; storm state = bright metallic shimmer, calm state = dark woody warmth |

| Amount (LFOToPitch) | Musical Result |
|--------------------|---------------|
| 0.05–0.10 | Imperceptible individual, cumulative breath over 30+ seconds |
| 0.15–0.25 | Gentle pitch drift aligned with swell period; bars rise and fall like buoys |

### Parameter Targets in OWARE

- `owr_material`: 0.50 (center starting point; coupling moves it in both directions)
- `owr_shimmerRate`: 2–4 Hz (Balinese shimmer set low; the shore provides the slow breath, shimmer adds faster texture on top)
- `owr_bodyType`: 2 (bowl) — bowl resonance has sub-octave boost that complements the low-frequency character of shore systems
- `owr_bodyDepth`: 0.60–0.80 — deep body resonance opens the floor relationship between the two engines
- `owr_thermalDrift`: 0.50 — thermal drift contributes to the watery sense of slight instability
- `owr_decay`: 3.0–5.0 s — very long decay; bars ring through entire wave periods

### Parameter Targets in OSPREY

- `osprey_shore`: 0.60 (Pacific region — smooth, sustained resonator profile, best material-continuum coupling)
- `osprey_seaState`: 0.30 (calm sea; coupling amount should be set so this calm state produces oware's wood end)
- `osprey_swellPeriod`: 8–15 s — long swell periods create slow material undulation in oware
- `osprey_macroCharacter`: 0.40 — moderate resonator brightness; not so bright it overwhelms coupling signal
- `osprey_macroCoupling`: 0.60 — opens coupling output from OSPREY's coupling cache

### Preset Name Suggestions

- "Shore Bells" — minimal, evocative
- "Tide Marimba" — production-friendly
- "Kelp Bed Gamelan" — mythological
- "Pacific Resonance" — geographic specificity

### Musical Description

Play a held chord on OWARE. Let OSPREY run freely at sea state 0.25. Over 10–20 seconds, the shore's slow swell period drives a gradual material sweep: bars shift from warm wood-struck to bright metal-sustained and back. The pitch drift from the LFO coupling is barely perceptible on any individual note but creates a sense that the bar frequencies themselves are breathing — as if the board is suspended in a fluid medium, which, mythologically, it is.

The Shore Bells recipe works best in meditative or ambient contexts. The coupling rates are geological: nothing snaps or clicks. Everything flows. At high amounts (0.60 `EnvToMorph`), OWARE essentially becomes a spectral-morphing resonator attached to OSPREY's shore system — the bar materials are no longer the performer's choice, they are the coast's choice.

---

## Recipe 4: OWARE × OBRIX — "Coral Instruments"

> *Coral grows on any hard substrate. The oware board lay on the seafloor for three hundred years. The reef built itself on its surface. Now when the reef vibrates, the board sings underneath. The coral is the cage; the bars are the prisoner who sings.*

**Signal direction:** OBRIX → OWARE (reef bricks excite bar resonators)
**Mythology:** XObrix is the living reef — the habitat built by accretion of modular bricks. XOware is the substrate, the found object on which the reef grew. OBRIX's brick architecture outputs complex, harmonically rich audio (especially with LPFilter + RingMod processor chains). As `AudioToFM` equivalent behavior: routing OBRIX audio as `AmpToFilter` into OWARE drives the bar brightness dynamically with brick-generated harmonics.

### Coupling Type

**Primary: `AmpToFilter` (OBRIX → OWARE)**
**Secondary: `EnvToMorph` (OBRIX → OWARE)**
**Note:** OBRIX does not output a dedicated LFO channel; its `getSampleForCoupling` returns post-FX stereo. Use the averaged amplitude of the OBRIX audio block as the coupling signal.

OBRIX's filter feedback (when `obrix_proc1Feedback` is set to self-oscillation range, ~0.80–0.95) generates a continuous, rich resonant tone. As `AmpToFilter` into OWARE, this tone acts as a "carrier" that continuously brightens the bar resonance — the reef sustains the board's upper harmonics indefinitely as long as the brick resonates.

### Amount Range

| Amount (AmpToFilter) | Musical Result |
|---------------------|---------------|
| 0.12–0.20 | Subtle: bars gain a gentle brightness when OBRIX bricks are active; rests fall back to warmth |
| 0.25–0.45 | Audible coupling: brick LFO oscillations translate into slow filter modulation on bars |
| 0.50–0.75 | Dominant: OBRIX's filter feedback tone continuously opens bar brightness; bars ring with metallic extended sustain as long as OBRIX source is active |

| Amount (EnvToMorph) | Musical Result |
|--------------------|---------------|
| 0.10–0.15 | Material shift on brick attack transients only |
| 0.20–0.30 | Clear material animation: OBRIX's brick envelope translates to oware material morphing |

### Parameter Targets in OWARE

- `owr_material`: 0.25 (wood base; coupling will push toward metal during brick activity)
- `owr_sympathyAmount`: 0.60 — high sympathy: brick harmonics that overlap with bar modes will trigger sympathetic ring
- `owr_brightness`: 3500 Hz base — coupling pushes it up to ~8000–13500 Hz at high amounts
- `owr_buzzAmount`: 0.20–0.35 — buzz membrane active; OBRIX's ring modulator output shares energy in the 200–800 Hz band that the buzz membrane extracts

### Parameter Targets in OBRIX

- `obrix_src1Type`: Saw — harmonically rich source for strong coupling amplitude
- `obrix_proc1Type`: LPFilter with moderate resonance (Q ≈ 4–8); feedback set to 0.65–0.80
- `obrix_proc2Type`: RingMod — ring modulation creates sidebands that fall in oware bar frequency range
- `obrix_mod1Type`: LFO, targeting `FilterCutoff` of proc1 — creates slow modulation rhythm in the coupling signal
- `obrix_driftRate`: 0.02 Hz, `obrix_driftDepth`: 0.45 — drift bus adds slow pitch variation, reflected in OWARE pitch via `AmpToPitch` if a third route is added
- `obrix_journeyMode`: enabled — sustained brick activity keeps coupling signal alive indefinitely

### Preset Name Suggestions

- "Coral Instruments" — mythological pairing name
- "Reef Balafon" — habitat + instrument
- "Barnacle Marimba" — evocative, slightly absurdist
- "Living Substrate" — ambient production label

### Musical Description

Set OBRIX to a slow, self-oscillating feedback tone with a drifting LFO on the filter. Play held notes on OWARE. The brick's filter sweep is audible in the bar resonance — as the OBRIX filter opens, the bars brighten with it; as it closes, bars darken back to wood warmth. At high sympathy (0.60), hitting a OWARE note causes the bars with shared frequency content with OBRIX to ring longer than those without. The reef and the board are in constant spectral negotiation.

With `obrix_proc2Type` set to RingMod, the coupling signal contains sum-and-difference frequencies from the original brick pitch and the ring carrier. These sidebands fall at odd intervals and create a metallic, indeterminate-pitch shimmer in the oware bars — not quite pitched, not quite noise. This is the "coral growth" texture: gradual, architecturally complex, generated not by any single source but by the interference of two systems.

---

## Recipe 5: OWARE × ORGANON — "Living Instruments"

> *The chemotroph found the oware board in the dark. It had no light to see by, only chemistry. It began to metabolize the board's resonance. After a hundred cycles of feeding, the board had changed — its decay had shortened, its material had shifted, its modes had realigned around the organism's preferred frequencies. The board was still the board. But it was also, now, slightly alive.*

**Signal direction:** OWARE → ORGANON + ORGANON → OWARE (bidirectional metabolic symbiosis)
**Mythology:** XOrganon is the deep-sea chemotroph — it eats audio and converts entropy into harmonic structure. XOware is a physical resonating object with metabolizable material properties. The bidirectional coupling creates a true symbiosis: OWARE feeds ORGANON with resonant bar audio (high entropy from sympathetic network, thermal drift, shimmer); ORGANON's metabolic output feeds back into OWARE's material axis, gradually shifting the board's physical identity in response to what the organism has "learned" about its spectral content.

### Coupling Type

**Route A (OWARE → ORGANON): `AudioToFM`**
OWARE's stereo output enters ORGANON's per-voice ingestion buffer. The sympathetic resonance network produces a complex, multi-partial signal that ORGANON's entropy analyzer reads as high nutrition — many spectral bins active, broad amplitude distribution.

**Route B (ORGANON → OWARE): `EnvToMorph`**
ORGANON's modal array output, which has been shaped by the ingested bar audio, feeds back into OWARE's `couplingMaterialMod`. The organism's "digested" version of the board's sound — harmonically reorganized by the Port-Hamiltonian modal array — becomes the instruction for how the board should change its material.

### Amount Range

| Direction | Amount | Musical Result |
|-----------|--------|---------------|
| OWARE → ORGANON (AudioToFM) | 0.25–0.40 | ORGANON begins metabolizing bar audio; modal array voices emerge from the board's harmonics |
| OWARE → ORGANON (AudioToFM) | 0.50–0.75 | ORGANON fully fed by bar audio; its 32 modal modes tuned to OWARE's bar partials |
| ORGANON → OWARE (EnvToMorph) | 0.10–0.18 | Gentle metabolic feedback; material barely shifts |
| ORGANON → OWARE (EnvToMorph) | 0.20–0.35 | Audible material drift as ORGANON's preferences shape the board; bars gradually migrate toward organism's optimal frequencies |
| ORGANON → OWARE (EnvToMorph) | 0.40–0.55 | The board and organism have locked into a feedback loop; each sustains the other's identity, converging on a shared spectral state |

### Parameter Targets in OWARE

- `owr_sympathyAmount`: 0.65–0.80 — maximum sympathetic resonance; rich multi-partial signal for ORGANON to metabolize
- `owr_material`: 0.40 (midpoint; bidirectional feedback should be able to push it in either direction)
- `owr_thermalDrift`: 0.45 — thermal drift increases spectral entropy, providing richer nutrition to ORGANON
- `owr_shimmerRate`: 5–8 Hz — Balinese shimmer adds rapid amplitude modulation that increases signal variety
- `owr_decay`: 2.5–4.0 s — long decay ensures ORGANON always has signal to metabolize between note triggers
- `owr_bodyType`: 1 (frame) — frame resonance adds three fixed formants (200, 580, 1100 Hz) that ORGANON's enzyme selectivity can be tuned to target

### Parameter Targets in ORGANON

- `organon_metabolicRate`: 0.8–1.5 Hz — slow metabolism to match bar decay timescales; faster rates cause flickering that sounds mechanical
- `organon_enzymeSelect`: 400–800 Hz — targets the mid-frequency range where oware wood modes and buzz membrane are most active
- `organon_catalystDrive`: 0.60–0.80 — enough drive to produce audible modal output from the ingested bar material
- `organon_dampingCoeff`: 0.20–0.35 — low damping; ORGANON should ring, not click
- `organon_signalFlux`: 0.70 — highly fed organism; the board's output is its primary food source
- `organon_isotopeBalance`: 0.40 — slightly subharmonic emphasis; the organism prefers the board's lower partials, complementing the board's upper-mode focus
- `organon_macroCoupling`: 0.70 — opens organism to maximum coupling reception

### Preset Name Suggestions

- "Living Instruments" — direct statement of the concept
- "Metabolic Board" — scientific-poetic
- "Chemotroph Marimba" — mythological label
- "Deep Symbiosis" — ambient production name
- "Akan Chemotroph" — cultural-mythological cross-reference

### Musical Description

This recipe requires patience. On note-on, OWARE plays normally — bars strike, ring, shimmer, decay. Within the first 200–500ms, ORGANON begins receiving bar audio through the ingestion buffer. Its entropy analyzer registers the multi-partial oware signal as high-entropy nutrition. The metabolic rate at 1.0 Hz means the organism completes one full digestive cycle per second.

By the second or third note, ORGANON's modal array has been tuned by the ingested content. Its output begins to resemble an echo of OWARE — not quite the same pitch, not quite the same timbre, but recognizably related. This output feeds back into OWARE's `couplingMaterialMod`.

At amount 0.25 on the return route, the bars receive a gentle nudge toward metal during ORGANON's active metabolic phase. At 0.40, the bars begin to change character mid-sustain: notes started as wood end up slightly metallic. The board is aging in real time, shaped by the organism's feeding.

Over a 30–60 second performance, the coupling converges: ORGANON produces exactly what OWARE most readily provides, and OWARE's material state has drifted toward whatever ORGANON "prefers" to eat. The two engines have found each other's optimal states through mutual influence. This is the living instrument: a physical object and a biological process negotiating a shared existence in real time.

---

## Summary Table

| Recipe | Direction | Primary Type | Amount Range | Musical Core |
|--------|-----------|-------------|-------------|-------------|
| Fire Circle's Instrument | OSTINATO → OWARE | `EnvToMorph` | 0.20–0.45 | Drum rhythm animates bar material; djembe drives marimba timbre |
| Surface Resonance | ONSET → OWARE | `AmpToFilter` | 0.30–0.50 | 808 kick opens bar brightness; electronic drum shapes acoustic resonance |
| Shore Bells | OSPREY → OWARE | `EnvToMorph` | 0.25–0.55 | Shore morphing controls material continuum; coastal identity = bar identity |
| Coral Instruments | OBRIX → OWARE | `AmpToFilter` | 0.25–0.60 | Reef brick feedback tone drives bar brightness; living substrate relationship |
| Living Instruments | OWARE ↔ ORGANON | `AudioToFM` + `EnvToMorph` | 0.25–0.55 (bidirectional) | Metabolic symbiosis; board feeds organism, organism reshapes board |

---

## Design Notes

### Why `EnvToMorph` Is the Key Oware Coupling Receiver

Among the four types OWARE accepts, `EnvToMorph` produces the most musically interesting results because it touches the engine's deepest identity: the **material continuum**. The wood-to-metal axis controls per-mode decay differentiation (alpha 2.0 → 0.3), mallet cutoff behavior, buzz membrane character, and body resonance balance. Shifting material with a coupling signal transforms OWARE from a static acoustic model into a reactive acoustic model — the physics of the instrument change in real time in response to the other engine's activity.

`AmpToFilter` produces the most immediately audible, production-ready result — clear sidechain-style brightness modulation. `LFOToPitch` and `AmpToPitch` are useful for tuning instability effects but should be used at very low amounts (0.05–0.12) to avoid breaking the bar tunings.

### Oware as a Source Engine

When OWARE feeds other engines, its multi-partial sympathetic resonance signal is exceptional nutritional content for metabolism engines like ORGANON. The shimmer LFO, thermal drift, and inter-voice sympathetic coupling create an output that is never a simple tone — it is always spectrally complex and time-varying. For coupling targets that benefit from rich, varied input (`AudioToFM` and `AudioToWavetable` receivers), OWARE is a superior source compared to simpler oscillator-based engines.

### Sympathy Network Amplifies All Coupling Effects

Setting `owr_sympathyAmount` high (0.50–0.75) before activating any of these recipes significantly amplifies the perceived coupling effect. The reason: sympathy means that a coupling signal which brightens or shifts one bar's mode will, via the mode-frequency-selective coupling, cause adjacent bars to respond as well. Low sympathy = coupling effect is isolated to the coupling-signal-sized perturbation. High sympathy = coupling signal creates a cascade through the whole voice network.
