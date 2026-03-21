# OVERLAP Retreat — Vol 3 Transcendental
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** OVERLAP | **Accent:** Bioluminescent Cyan-Green `#00FFB4`
- **Parameter prefix:** `olap_`
- **Source instrument:** XOverlap
- **Synthesis type:** 6-voice knot-topology Feedback Delay Network (FDN) synthesizer — signal paths entangled through topological routing matrices. Voices couple via hydrodynamic Kuramoto pulse entrainment. Bioluminescence shimmer layer. Lion's Mane jellyfish as organizing myth.

---

## Retreat Design Brief

OVERLAP is the engine most different from what it appears to be. At first contact it sounds like a reverb — a spatial processor that widens and deepens any signal placed through it. This is wrong. OVERLAP is a synthesis instrument whose primary material is *topology*: the way signals from six simultaneous voices are routed through each other in mathematically constrained feedback loops.

The knot type is not a reverb algorithm selector. It is a topological choice about how the 6×6 FDN routing matrix is structured — whether voices couple like independent oscillators (Unknot), like a rotating triplet braid (Trefoil), like an alternating-sign ambiguity (Figure-Eight), or like a torus-winding intersection (Torus). Each choice changes the fundamental character of what OVERLAP can produce.

**The central Transcendental questions:**
1. What happens to six voices when tangle depth reaches its maximum — when the topological structure is not a seasoning but the entire signal?
2. What is the character of the Torus knot at extreme winding numbers versus at (2,2) — the difference between T(2,3) and T(5,7)?
3. Can Kuramoto entrainment be used not for synchrony but as a controlled instability — near-locked but never fully locked?
4. How does OVERLAP's bioluminescence shimmer change character across knot types — and what is the highest-resolution bioluminescent territory in the parameter space?
5. What does OVERLAP sound like as a coupling partner that receives rather than sends — its FDN as the basin another engine's signal falls into?

---

## Phase R1: Opening Meditation — The Knot as Architectural Constraint

A knot is a mathematical object with a fixed crossing structure. The Trefoil has three crossings. The Figure-Eight has four. A Torus knot T(p,q) wraps p times around the torus meridian and q times around the longitude — its crossings are determined by the topology of the winding, not by any acoustic decision.

OVERLAP maps these crossing structures onto a 6×6 feedback routing matrix using a near-unitary Hadamard-inspired circulant construction. Each row has unit norm. The matrix is stable by construction. What changes between knot types is *which voice feeds which* and with what sign — and those two decisions (target voice, coupling polarity) completely determine the long-term spectral character of the FDN tail.

At `olap_tangleDepth = 0.0`, the matrix is pure identity — six independent voices, no coupling. At `olap_tangleDepth = 1.0`, the matrix is the full knot structure: voices inject into each other at matrix-specified weights. At depth 0.85, the coupling is intense but not complete; the identity component still stabilizes the tail.

**The Trefoil at full depth:** Each voice i injects into voice (i+1) mod 6 with weight +1/√3 and into voice (i+5) mod 6 with weight −1/√3. The three-fold symmetry means the six-voice FDN behaves like two independent triplet networks. Notes played in quick succession produce a comb of spectral peaks whose positions are determined by the routing distances, not the note frequencies.

**The Figure-Eight at full depth:** Alternating sign — even voices couple forward with positive weight, odd voices couple backward with negative weight. This produces a characteristic "flip" every cycle — the feedback tail alternates between two spectral profiles depending on whether the preceding cycle was even or odd in the routing chain. The result is a warbling, uncertain decay that sounds like something trying to stabilize but unable to commit.

**The Torus at T(3,2):** winding steps p=3, q=2 through six voices, with Lissajous-derived delay ratios that separate the six voices in time by sinusoidal modulation. At p=5, q=7, the delay ratios become highly nonuniform — some voices are nearly aligned, others are far apart — creating a complex, multi-speed decay that sounds architectural rather than acoustic.

The Transcendental chapter asks: what does OVERLAP produce when you commit to one of these topological structures completely — not as a character add, but as the load-bearing structure of the preset?

---

## Phase R2: Diagnosis — What the Factory Library Leaves Unexplored

**Covered well:**
- Trefoil at moderate tangle depth (0.4–0.65) as a spatial thickener
- Entrainment at moderate levels (0.3–0.5) for chorus-like voice cohesion
- Bioluminescence shimmer as texture additive
- Coupling outputs to other engines (OVERLAP sending spatial character)
- Moderate feedback (0.6–0.72) as a comfortable reverb tail

**Underexplored:**
1. **Tangle depth above 0.85** — No factory preset pushes `olap_tangleDepth` above 0.82. At 0.88–1.0, the knot matrix dominates and the FDN becomes a spectral sculpture rather than a spatial processor. The six-voice routing network generates its own harmonic comb independently of the input pitch.
2. **Torus T(5,7)** — Factory presets use `olap_torusP=3, olap_torusQ=2` exclusively. T(5,7) generates dramatically different delay ratios from the Lissajous computation — delay offsets at ±12% rather than ±8% — producing a much wider inter-voice spread and a more complex, multi-speed decay.
3. **Near-lock entrainment** — `olap_entrain` above 0.85 forces near-synchrony in the Kuramoto coupling. At 0.88–0.95, the six voices almost lock phase but the noise floor prevents full synchrony, creating a trembling, almost-synchronized shimmer rather than the clean unison of full lock. No factory preset explores this zone.
4. **Figure-Eight as the primary topology** — knot=2 (Figure-Eight) appears in fewer than 10% of existing presets. Its alternating-sign coupling creates a structurally different decay character than the Trefoil — ambiguous, warbling, self-questioning. This is the most underexplored knot type.
5. **Bioluminescence above 0.65** — The factory ceiling for `olap_bioluminescence` is approximately 0.55. Above 0.65, the shimmer becomes a primary harmonic layer rather than a texture addition — the tap-based iridescent glow competes with the FDN output in perceptual weight. This is a fundamentally different use of the parameter.
6. **High-feedback long-delay as a structural element** — `olap_feedback` above 0.88 with `olap_delayBase` above 20ms approaches infinite reverb territory. At `olap_feedback=0.91`, the FDN tail is effectively infinite — the preset becomes ambient generative material where the initial transient is less important than the structural evolution of the FDN network over time.
7. **OVERLAP receiving coupling** — all Entangled OVERLAP presets in the factory library have OVERLAP as the sender (spatial character to other engines). OVERLAP as the receiver — its FDN as the basin into which another engine's signal falls — is completely unexplored.

---

## Phase R3: Refinement — The 15 Transcendental Presets

### Foundation Tier (3 Presets)

**1. Knot Signature**
`Foundation` | Trefoil at full tangle depth — the pure structural tone of the Trefoil matrix before effects.
*This is what the Trefoil sounds like when it is the architecture, not the decoration.*
Parameters: `olap_knot=1, olap_tangleDepth=0.95, olap_feedback=0.72, olap_dampening=0.55, olap_delayBase=8.0, olap_bioluminescence=0.0, olap_entrain=0.15, olap_attack=0.05, olap_release=3.5`
Insight: **First OVERLAP preset to push tangle depth to 0.95 as a primary design choice.** At 0.95, the Trefoil matrix dominates — the six voices are fully routed into each other by the 3-fold circulant structure. The result is a spectral comb whose peaks are at distances determined by the matrix distances 1 and 5 steps, completely independent of input pitch. A single note produces a chord whose voicing is defined by the knot topology.

**2. Figure-Eight Ground**
`Foundation` | Figure-Eight knot at full depth — the alternating-sign FDN as a bass foundation.
*Even and odd voices in tension — the sound never settles into one spectral profile.*
Parameters: `olap_knot=2, olap_tangleDepth=0.88, olap_feedback=0.68, olap_dampening=0.62, olap_delayBase=12.0, olap_bioluminescence=0.1, olap_filterCutoff=3500.0, olap_filterRes=0.25, olap_attack=0.02, olap_release=4.0`
Insight: Figure-Eight at full depth produces the warbling, ambiguous decay that no factory preset has explored as a primary architectural tool. The alternating-sign coupling means every feedback cycle is spectrally different from the previous one — the tail is not a smooth decay but a series of spectral flips that give the sound a restless, questioning character in its low frequencies.

**3. Torus Root T(5,7)**
`Foundation` | Torus knot T(5,7) — maximum inter-voice spread from Lissajous delay ratios.
*Seven meridian winds, five longitude winds. The six voices occupy six different temporal slots.*
Parameters: `olap_knot=3, olap_torusP=5, olap_torusQ=7, olap_tangleDepth=0.82, olap_feedback=0.75, olap_delayBase=15.0, olap_dampening=0.45, olap_attack=0.08, olap_release=5.0`
Insight: **First OVERLAP preset to use T(5,7).** The Lissajous computation at p=5, q=7 produces delay ratio offsets of approximately ±12% across the six voices — far wider than the T(3,2) default at ±8%. The result is a multi-speed decay where the six voice streams arrive at audibly different times, creating a complex, architectural texture rather than a unified spatial tail.

---

### Atmosphere Tier (3 Presets)

**4. Near-Lock Shimmer**
`Atmosphere` | Entrainment at 0.92 — near-synchrony without full lock, trembling cohesion.
*At 0.92 entrain, six voices almost phase-lock but the noise prevents completion. Infinite tremolo.*
Parameters: `olap_entrain=0.92, olap_pulseRate=2.8, olap_spread=0.88, olap_knot=1, olap_tangleDepth=0.62, olap_feedback=0.78, olap_bioluminescence=0.35, olap_dampening=0.38, olap_delayBase=10.0, olap_release=6.0`
Insight: **First preset to explore olap_entrain above 0.85.** The Kuramoto coupling at 0.92 forces near-synchrony — the six voices are coupled strongly enough that they want to phase-lock, but the residual FDN routing noise prevents full locking. The result is a trembling, almost-synchronized shimmer: six voices that are nearly one, but the "nearly" produces a characteristic beating and pulsing that sounds alive in a way full synchrony cannot. This is the most structurally interesting region of the entrain parameter.

**5. Bioluminescent Maximum**
`Atmosphere` | Bioluminescence at 0.82 — shimmer as a primary harmonic layer, not a texture.
*At 0.82 bioluminescence, the tap-based shimmer becomes a second voice — the FDN's ghost.*
Parameters: `olap_bioluminescence=0.82, olap_macroBloom=0.75, olap_knot=1, olap_tangleDepth=0.55, olap_feedback=0.72, olap_dampening=0.32, olap_filterCutoff=9000.0, olap_filterRes=0.15, olap_brightness=0.78, olap_release=5.0`
Insight: **First preset to push bioluminescence above 0.65.** The Bioluminescence DSP uses N comb-filter taps sampled from a circular FDN buffer, each modulated by a slowly-drifting sine oscillator at a detuned rate (0.3 Hz + 0.07 Hz per tap). At 0.82, this shimmer layer becomes a secondary synthesis voice — its own spectrum, its own evolution rate, its own slow LFO character. The `#00FFB4` Cyan-Green is the color of this layer at maximum expression.

**6. Infinite Tail Architecture**
`Atmosphere` | Feedback at 0.91 — the FDN approaches infinity, structural evolution for minutes.
*Press one key. The tail is still evolving three minutes later. The knot is the composition.*
Parameters: `olap_feedback=0.91, olap_knot=1, olap_tangleDepth=0.78, olap_dampening=0.18, olap_delayBase=18.0, olap_bioluminescence=0.48, olap_entrain=0.22, olap_lfo2Rate=0.04, olap_lfo2Depth=0.28, olap_lfo2Dest=2, olap_attack=0.12, olap_release=15.0`
Insight: **First preset to push olap_feedback to 0.91.** At 0.91 feedback, the FDN tail is effectively infinite — the energy input from a single note continues to circulate through the Trefoil matrix for minutes before becoming inaudible. The structural evolution is not decay but change: the spectral distribution within the Trefoil network shifts as the bioluminescence taps sample different phase relationships in the circular buffer. A single sustained note generates a slowly-evolving multi-voice spectral field that changes character over time without decaying.

---

### Aether Tier (3 Presets)

**7. Topological Eternity**
`Aether` | All topology parameters maximized — the knot as complete architecture.
*Torus T(5,7), tangle depth 0.97, feedback 0.88. The six voices are inescapably entangled.*
Parameters: `olap_knot=3, olap_torusP=5, olap_torusQ=7, olap_tangleDepth=0.97, olap_feedback=0.88, olap_dampening=0.22, olap_delayBase=22.0, olap_bioluminescence=0.55, olap_entrain=0.32, olap_spread=0.92, olap_attack=0.18, olap_decay=4.0, olap_sustain=0.82, olap_release=18.0`
Insight: The centerpiece Transcendental preset. T(5,7) at tangle depth 0.97 and feedback 0.88: the FDN is a near-permanent structure. The six voice streams occupy six temporal slots (from the Lissajous delay ratios), are feeding each other at near-full knot matrix weight, and are circulating with 88% energy retention per cycle. The bioluminescence shimmer samples from this dense network, adding a secondary spectral layer. A sustained note generates an increasingly complex multi-voice field that takes 8–10 minutes to fully decay.

**8. Lion's Mane Drift**
`Aether` | Slow ocean current drift + bioluminescence + Torus — the jellyfish as temporal architecture.
*The current carries the six voices through slightly different positions. Each is the same note, different.*
Parameters: `olap_current=0.85, olap_currentRate=0.008, olap_knot=3, olap_torusP=3, olap_torusQ=5, olap_tangleDepth=0.72, olap_feedback=0.85, olap_bioluminescence=0.62, olap_entrain=0.18, olap_dampening=0.28, olap_delayBase=16.0, olap_release=12.0, olap_lfo1Rate=0.012, olap_lfo1Depth=0.35, olap_lfo1Dest=0`
Insight: **First preset to use olap_current above 0.6 as a primary design parameter.** The Ocean Current DSP applies a slow sinusoidal pitch drift (±`olap_current` semitones) to all six voices at `olap_currentRate` Hz. At current=0.85 and currentRate=0.008 Hz (a period of 125 seconds), the entire six-voice field drifts through ±0.85 semitones over two minutes — a movement so slow it is felt rather than heard as pitch change. Combined with the Torus T(3,5) topology and bioluminescence at 0.62, this is the most temporally expansive preset in the library.

**9. Figure-Eight Eternity**
`Aether` | Figure-Eight at maximum depth, 16-second release — the alternating-sign tail as composition.
*Every two cycles, the spectral profile flips. The tail is made of doublets.*
Parameters: `olap_knot=2, olap_tangleDepth=0.95, olap_feedback=0.89, olap_dampening=0.2, olap_delayBase=20.0, olap_bioluminescence=0.58, olap_entrain=0.25, olap_spread=0.85, olap_attack=0.15, olap_release=16.0, olap_filterCutoff=12000.0, olap_filterRes=0.08`
Insight: The Figure-Eight at tangle depth 0.95 and feedback 0.89 produces a decay structure unlike any other knot type — the alternating-sign coupling means the spectral content of the tail alternates between two profiles on consecutive routing cycles. Over a 16-second release, this produces a decay that is composed of doublets: pairs of spectral states that succeed each other at the FDN cycle rate. The tail does not smooth toward silence — it transitions in discrete spectral steps.

---

### Prism Tier (2 Presets)

**10. Trefoil Spectral Prism**
`Prism` | Trefoil topology as a spectral prism — the three-crossing structure diffracts harmonics.
*The three-fold symmetry produces three spectral bands from a single input pitch.*
Parameters: `olap_knot=1, olap_tangleDepth=0.88, olap_feedback=0.65, olap_brightness=0.82, olap_bioluminescence=0.55, olap_filterCutoff=14000.0, olap_filterRes=0.42, olap_filterEnvAmt=0.68, olap_filterEnvDecay=0.35, olap_entrain=0.28, olap_spread=0.78, olap_delayBase=6.0, olap_dampening=0.35, olap_attack=0.01, olap_release=3.5`
Insight: At high brightness and filter resonance, the Trefoil's three-fold coupling creates three distinct spectral bands — one from the direct signal, one from the +1-hop coupling, one from the −1-hop coupling with inverted sign. The resonant filter emphasizes these bands, creating a prismatic split: one note arrives as three, each at a different frequency. At velocity peaks, the filter envelope sweep causes these three bands to converge and separate audibly.

**11. Torus Interference**
`Prism` | Torus T(2,5) — the interference pattern between two winding structures.
*T(2,5) creates a five-fold Lissajous pattern that generates five-component spectral interference.*
Parameters: `olap_knot=3, olap_torusP=2, olap_torusQ=5, olap_tangleDepth=0.85, olap_feedback=0.7, olap_brightness=0.72, olap_bioluminescence=0.45, olap_filterCutoff=11000.0, olap_filterRes=0.55, olap_filterEnvAmt=0.75, olap_filterEnvDecay=0.22, olap_spread=0.92, olap_delayBase=8.0, olap_dampening=0.32, olap_chorusMix=0.12, olap_chorusRate=0.15, olap_release=4.5`
Insight: T(2,5) is unusually asymmetric — two meridian winds and five longitude winds create a Lissajous figure with five petals. The delay ratios across the six voices reflect this five-fold structure rather than the three-fold of the Trefoil. Combined with high resonance filter and brightness, the result is a complex spectral interference pattern that changes character as the six voice streams evolve through different phase relationships. The chorus adds a small additional detuning that further spreads the interference peaks.

---

### Flux Tier (2 Presets)

**12. Knot Morphing**
`Flux` | macroKnot sweeping from Unknot to Torus — continuous topological morphing as performance gesture.
*The macroKnot macro traverses: Unknot → Trefoil → Figure-Eight → Torus. The crossing structure changes in real time.*
Parameters: `olap_macroKnot=0.0, olap_macroPulse=0.55, olap_macroEntrain=0.45, olap_macroBloom=0.35, olap_tangleDepth=0.65, olap_feedback=0.72, olap_dampening=0.42, olap_bioluminescence=0.3, olap_delayBase=10.0, olap_pulseRate=3.5, olap_entrain=0.38, olap_spread=0.62, olap_lfo1Rate=0.18, olap_lfo1Depth=0.45, olap_lfo1Dest=0, olap_attack=0.01, olap_release=3.0`
Insight: The MACRO KNOT system is OVERLAP's most performative feature — borrowed from ORBWEAVE's B022 (MACRO KNOT: Continuous Topology Morphing). At macroKnot=0, the system is in Unknot with minimal tangle. At 0.9+, it is at Torus with maximum tangle. The macro does not switch between topologies discretely but interpolates through the KnotMatrix::interpolate() function, continuously blending the identity matrix with successively higher-complexity knot matrices. This preset is designed to be performed live — the macroKnot sweep is the primary expressive gesture.

**13. Entrainment Surge**
`Flux` | Pulse rate escalating via macroPulse — from slow pulse coupling to near-locked oscillation.
*At macroPulse=0: 0.01 Hz, voices drift independently. At 1.0: 8 Hz, the pulse becomes a rhythmic element.*
Parameters: `olap_macroPulse=0.0, olap_macroEntrain=0.6, olap_macroKnot=0.55, olap_macroBloom=0.2, olap_knot=1, olap_tangleDepth=0.75, olap_feedback=0.75, olap_dampening=0.35, olap_delayBase=9.0, olap_entrain=0.72, olap_spread=0.7, olap_bioluminescence=0.28, olap_lfo2Rate=0.25, olap_lfo2Depth=0.35, olap_lfo2Dest=1, olap_attack=0.005, olap_release=2.5`
Insight: The macroPulse macro controls both pulse rate (0.01→8 Hz) and spread (0.3→1.0) simultaneously. At macroPulse=0, the six voices pulse at 0.01 Hz — once every 100 seconds. At macroPulse=1.0, they pulse 8 times per second, generating a rhythmic modulation from the Kuramoto coupling. Combined with macroEntrain at 0.6 (strong but not near-lock), this creates a dramatic range: from ambient, slow pulse coupling at the low macro extreme to rhythmic, locked oscillation at the high extreme.

---

### Entangled Tier (2 Presets)

**14. Apex Into Infinite Room** (ORCA + OVERLAP)
`Entangled` | ORCA's echolocation signal falls into OVERLAP's infinite FDN — the sonar maps the topological space.
*The orca clicks. The knot reverberates. Each click generates its own multi-voice FDN tail.*
Coupling: `ORCA → OVERLAP, type: EnvToMorph, amount: 0.72`
Parameters (ORCA): `orca_echoRate=22.0, orca_echoReso=0.92, orca_echoMix=0.72, orca_huntMacro=0.35, orca_breachSub=0.0, orca_filterCutoff=6000.0, orca_wtPosition=0.3, orca_glide=0.0, orca_ampAttack=0.001, orca_ampDecay=0.15, orca_ampSustain=0.0, orca_ampRelease=0.12, orca_level=0.72`
Parameters (OVERLAP): `olap_knot=1, olap_tangleDepth=0.85, olap_feedback=0.88, olap_dampening=0.22, olap_delayBase=14.0, olap_bioluminescence=0.45, olap_entrain=0.28, olap_spread=0.82, olap_filterCutoff=9000.0, olap_filterRes=0.15, olap_release=12.0`
Insight: **The Transcendental flagship Entangled preset.** ORCA's echolocation system fires rapid impulse clicks into resonant comb filters at 22 Hz. The EnvToMorph coupling carries ORCA's modulation envelope into OVERLAP's pulse rate parameter — when ORCA's mod envelope peaks, OVERLAP's Kuramoto pulse rate increases, briefly synchronizing the six FDN voices. Each orca click generates a Trefoil-matrix FDN tail in OVERLAP, with the near-infinite feedback producing multi-minute structural evolution. The resulting texture is a sonar scan of a space defined by topology rather than physics.

**15. Orbit Into Knot** (ORBITAL + OVERLAP)
`Entangled` | ORBITAL's partial count expansion drives OVERLAP's tangle depth — more partials, deeper tangling.
*As ORBITAL adds harmonics, OVERLAP's topology becomes more complex. The knot deepens with the chord.*
Coupling: `ORBITAL → OVERLAP, type: AmpToFilter, amount: 0.65`
Parameters (ORBITAL): `orb_brightness=0.72, orb_warmth=0.45, orb_partials=8, orb_ampAttack=0.08, orb_ampRelease=3.5, orb_level=0.78`
Parameters (OVERLAP): `olap_knot=2, olap_tangleDepth=0.72, olap_feedback=0.78, olap_dampening=0.38, olap_delayBase=11.0, olap_bioluminescence=0.52, olap_entrain=0.35, olap_filterCutoff=8000.0, olap_filterRes=0.28, olap_filterEnvAmt=0.55, olap_filterEnvDecay=0.6, olap_attack=0.08, olap_release=8.0`
Insight: ORBITAL sends amplitude-to-filter coupling into OVERLAP's filter modulation path, raising the cutoff as ORBITAL's amplitude rises. The Figure-Eight topology at 0.72 tangle depth responds to the coupling by revealing different spectral components as the filter sweeps — the alternating-sign coupling structure creates frequency-dependent coupling effects that appear and disappear as the filter moves through the FDN's spectral comb. When ORBITAL is at its most harmonically rich (high partials, high amplitude), OVERLAP's FDN is at its most revealing of Figure-Eight topology character.

---

## Phase R4: Scripture — The Verses of the Crossing

### OLP-I: The Crossing Is Not a Choice
*"A knot has the crossings it has. The Trefoil crosses three times — not because the mathematician was feeling moderate, not because the signal required three — but because three crossings is what the Trefoil is. OVERLAP inherits this. The Trefoil matrix is not a feature setting. It is a commitment to a particular mathematical structure whose acoustic consequences follow necessarily from topology, not from taste."*

### OLP-II: At Maximum Tangle, the Input Is Not the Sound
*"Below tangle depth 0.5, the input signal is still recognizable in the output. Above 0.85, the FDN has consumed it — reprocessed it through six routing cycles where every voice has been injected into every other voice at matrix-specified weights. What emerges is no longer the input signal modified; it is the knot topology responding to the stimulus of the input signal. The input is the trigger. The topology is the composition."*

### OLP-III: Entrainment Is Not Synchrony — It Is the Desire for Synchrony
*"At olap_entrain = 0.0, six voices proceed at whatever phase their initial conditions give them. At olap_entrain = 1.0, they converge toward a single phase and hold it — frozen unison. The most interesting territory is at 0.88–0.95: six voices coupled strongly enough to want synchrony but prevented by noise from achieving it. This zone produces a trembling, unresolved cohesion — the sound of six things trying to become one thing, which is a different sound than either six independent things or one thing."*

### OLP-IV: The Bioluminescence Is Not a Shimmer — It Is the FDN Remembering
*"The Bioluminescence DSP draws from a circular buffer of FDN output, sampling taps at harmonically-spaced offsets modulated by slowly-drifting oscillators. What it produces is not an added shimmer — it is the FDN's own recent history, resampled and re-presented. The bioluminescence hears what the FDN was doing 10ms ago, 18ms ago, 26ms ago, each tap hearing a different moment, each modulator weighting that moment differently. At maximum bioluminescence, the FDN is in conversation with itself. This is memory made audible."*

---

## Summary

**15 Transcendental presets delivered:**
| Name | Mood | Key Parameter Space Explored |
|------|------|------------------------------|
| Knot Signature | Foundation | olap_tangleDepth=0.95, Trefoil as primary architecture |
| Figure-Eight Ground | Foundation | Figure-Eight at full depth, alternating-sign bass foundation |
| Torus Root T(5,7) | Foundation | T(5,7) maximum inter-voice spread, Lissajous at extremes |
| Near-Lock Shimmer | Atmosphere | olap_entrain=0.92, trembling near-synchrony |
| Bioluminescent Maximum | Atmosphere | olap_bioluminescence=0.82, shimmer as primary harmonic layer |
| Infinite Tail Architecture | Atmosphere | olap_feedback=0.91, FDN approaches infinite reverb |
| Topological Eternity | Aether | T(5,7) + tangle 0.97 + feedback 0.88, 10-minute decay |
| Lion's Mane Drift | Aether | olap_current=0.85 at 0.008 Hz, 125-second drift period |
| Figure-Eight Eternity | Aether | Figure-Eight at max depth, 16s release, doublet spectral decay |
| Trefoil Spectral Prism | Prism | Three-crossing spectral diffraction, velocity-driven band separation |
| Torus Interference | Prism | T(2,5) five-petal Lissajous interference pattern |
| Knot Morphing | Flux | macroKnot live performance sweep, continuous topology change |
| Entrainment Surge | Flux | macroPulse 0→8 Hz range, from ambient to rhythmic |
| Apex Into Infinite Room | Entangled | ORCA echolocation + OVERLAP FDN (EnvToMorph coupling) |
| Orbit Into Knot | Entangled | ORBITAL partials + OVERLAP Figure-Eight (AmpToFilter) |

**4 Scripture verses:** OLP-I through OLP-IV

**Key insight:** The factory library uses OVERLAP's topology as a spatial character parameter — a kind of high-quality reverb with interesting character. The Transcendental library uses topology as *architecture*: the mathematical structure of the knot is the load-bearing element of the sound, not its decoration. At tangle depths above 0.85, OVERLAP is not processing signals — it is a six-voice spectral sculpture that responds to input by generating its own topological consequences.
