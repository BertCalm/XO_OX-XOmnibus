# ORBWEAVE Retreat — Vol 3 Transcendental
*Guru Bin — 2026-03-21*

---

## Engine Identity

- **Gallery code:** ORBWEAVE | **Accent:** Kelp Knot Purple `#8E4585`
- **Parameter prefix:** `weave_`
- **Blessings:** B021 (Knot Phase Coupling Matrix), B022 (MACRO KNOT: Continuous Topology Morphing)
- **Synthesis type:** Phase-braided oscillator synthesis — 4 oscillator strands coupled through a configurable knot-topology matrix (Trefoil / Figure-Eight / Torus / Solomon). Each strand's phase increment is modified each sample by the weighted phase states of its neighbors.
- **Polyphony:** Up to 8 voices (Mono / Legato / Poly4 / Poly8)
- **Macros:** M1 WEAVE (braid depth push), M2 TENSION (resonance + coupling feedback), M3 KNOT (topology morphing), M4 SPACE (FX mix expansion)
- **Expression:** Velocity → filter cutoff (+2000 Hz at full velocity). Aftertouch → braid depth (+0.3). Mod wheel CC1 → filter cutoff (+4000 Hz).
- **Coupling types accepted:** AudioToFM, AmpToFilter, LFOToPitch, AmpToPitch, AmpToChoke

---

## Retreat Design Brief

ORBWEAVE has been seanced, retreated (Vol 1, 2026-03-21), and carries 435 factory presets across all 8 moods. The question for Vol 3 is not "what is ORBWEAVE?" but "what precise regions of ORBWEAVE's parameter space have not been committed to in 435 attempts?"

The factory library demonstrates ORBWEAVE's knot topologies broadly. It does not commit to them at maximum expression. The Vol 3 Transcendental chapter is about commitment: to the most extreme braidDepth values, to the most underused topology (Trefoil in Foundation), to the MACRO KNOT as a structural premise rather than a swept effect, and to the Solomon ring architecture in its most explicit harmonic form.

**The four Transcendental commitments:**
1. **High braidDepth in Foundation presets** — Foundation has only 2 presets above braidDepth 0.8. At 0.88–1.0, the phase lock is complete: the knot becomes the sound.
2. **Solomon's ring architecture with strandTune harmonies** — Solomon + strandTune ≠ 7.02 (fifth) has never been systematically explored. Minor third, major third, octave — each creates a doubly-linked chord pair with a different character.
3. **MACRO KNOT as primary premise** — No existing preset opens with macroKnot=0.5 as its *starting point*. All presets begin at 0.0 and treat KNOT as a performance sweep. The chimera state has its own identity. Vol 3 names it.
4. **LFO on braidDepth as a living topology** — LFO targets "Braid Depth" (target=5) appears in fewer than 10% of factory presets. A slow LFO on braidDepth continuously sweeps the coupling through the threshold, creating a sound that breathes between the detuned zone and the topology zone.

---

## Phase R1: Opening Meditation — The Knot as Architecture

Pull the four strands taut.

You are holding the ends of a braid. The middle of the braid is the knot — a specific arrangement of crossings that gives the braid its topology. The Trefoil has three crossings, all the same handedness (writhe +3). The Figure-Eight has four crossings, alternating handedness. The Solomon has linking number 2 — two rings that pass through each other twice. The Torus spirals around a donut surface at angles P and Q.

These are not approximations. They are exact descriptions of the mathematical objects whose coupling matrices govern the DSP. The Trefoil's matrix encodes the writhe: coupling flows counterclockwise around the three-strand ring (0→1→2→0) because the writhe goes that direction. The Figure-Eight alternates positive (0.6) and negative (−0.4) matrix entries because the crossings alternate handedness. The Solomon's intra-ring strength (0.8) and inter-ring strength (0.3) come from the linking number: two full interpenetrations means tight internal coupling and weak external coupling.

When braidDepth rises above the WEAVE-I threshold (~0.7), you are not adjusting a "character" parameter. You are pulling the four strands tighter together until the topology becomes the primary acoustic fact.

At braidDepth=1.0, every strand's phase is perturbed by up to ±160 Hz per sample by the combined coupling from its neighbors (200 Hz coupling scale × maximum matrix coefficient 0.8). A note on A4 (440 Hz) at Solomon knotType, braidDepth=1.0: Strand 0 is being simultaneously pushed toward Strand 1 (0.8 × 200 = 160 Hz) and pulled by Strand 2 (0.3 × 200 = 60 Hz). The result is a phase relationship determined not by the player's detuning choice but by the doubly-linked ring structure. This is knot synthesis.

The Transcendental chapter asks: what happens when you commit to staying in that zone?

---

## Phase R2: Diagnosis — What the Factory Library Leaves Unexplored

**Covered well:**
- Torus topology variations across many P/Q ratios (P=2 Q=3, P=2 Q=5, P=5 Q=8)
- Figure-Eight in Flux presets (high movement, alternating polarity as texture)
- macroKnot sweeping as a performance gesture (used well in Flux)
- Moderate braidDepth (0.3–0.65) as the central operating zone
- Coupling output into other engines (OBRIX→ORBWEAVE pairs in Entangled)

**Underexplored:**

1. **Foundation presets at braidDepth above 0.8** — Foundation has 93 ORBWEAVE presets. Only 2 use braidDepth above 0.8. The high-braid-depth regime changes the character fundamentally — the four strands begin to phase-lock into topologically determined ratios rather than floating freely. Foundation presets should demonstrate this "committed" state, not just the approach.

2. **Solomon + non-fifth strandTune harmonies** — Solomon is uniquely suited to chord synthesis: Ring A (strands 0–1) and Ring B (strands 2–3) can be offset by strandTune to create a two-part harmony where each part has internal phase resonance. The factory library uses Solomon + strandTune = 7.02 (fifth) occasionally. Solomon + 5.0 (major third), Solomon + 3.86 (minor third), Solomon + 12.0 (octave) are absent.

3. **LFO target = Braid Depth as a primary design axis** — LFO target 5 (Braid Depth) appears in fewer than 10% of factory presets. A slow LFO (0.03–0.1 Hz) sweeping braidDepth continuously moves the sound between the "detuning zone" (<0.4) and the "topology zone" (>0.7). The perceptual event — the coupling character appearing and disappearing with the LFO — is a fundamentally different sonic phenomenon than static braid depth.

4. **macroKnot=0.5 as a stable preset identity** — The chimera between consecutive knot types has its own sonic identity. Trefoil-to-Figure-Eight chimera (knotType=0, macroKnot=0.5): the three-strand ring coupling of the Trefoil blends with the four-strand alternating polarity of the Figure-Eight — the result has both the directional asymmetry of the Trefoil and the tension-release quality of the Figure-Eight simultaneously. No existing preset treats macroKnot=0.5 as a *starting point* rather than a destination of a macro sweep.

5. **Poly8 voice mode with long release at high braidDepth** — When 8 simultaneous voices each run their own knot coupling, the 8 coupling matrices produce 8 independent phase trajectories for each of the 4 strands — 32 strand evolutions at once. With long release (>3 seconds) and high braidDepth, held notes accumulate into a polyphonic topology cloud. The factory library uses Poly8 with moderate release. Extended polyphony at high braidDepth creates a harmonic accumulation that the existing library doesn't document.

6. **Waveform × topology combinations beyond Sine** — Sine + Trefoil is the canonical teaching combination. Square + Solomon is largely absent. At high braidDepth, Square strands through the Solomon matrix create intermodulation between the odd harmonics of the two rings — the coupled output contains sum and difference tones at odd multiples of the ring frequency offsets. This is an unexplored timbral region.

---

## Phase R3: Refinement — The 15 Transcendental Presets

### Foundation Tier (3 Presets)

**1. Trefoil Locked**
`Foundation` | Trefoil topology, braidDepth=0.88, Sine strands, filterCutoff=2500 Hz.
*The three-strand ring at near-maximum coupling. Phase locking audible as topology.*
Parameters: `weave_knotType=0, weave_braidDepth=0.88, weave_strandType=1, weave_filterCutoff=2500, weave_filterReso=0.3`
Insight: **First Foundation preset in the library to commit to Trefoil above braidDepth=0.85.** At 0.88, the three-strand ring (0→1→2→0) has partially locked: strands 0 and 1 are being pulled together by the 0.7 coupling coefficient, while Strand 3 is gently tethered at 0.2. The phase locking manifests as a spectral crystallization — the four-oscillator blur of lower braidDepths resolves into discrete peaks. The sound is no longer "four oscillators near each other." It is "the Trefoil knot."

**2. Solomon Minor Third**
`Foundation` | Solomon topology, strandTune=3.86 semitones (minor third), braidDepth=0.65, Saw strands.
*Ring A at root, Ring B at minor third. Two internally resonant rings, minor chord.*
Parameters: `weave_knotType=3, weave_strandTune=3.86, weave_braidDepth=0.65, weave_strandType=2, weave_filterCutoff=4000, weave_filterReso=0.2`
Insight: The Solomon knot's ring architecture provides two internally coupled pairs. strandTune=7.02 (fifth) is the documented power chord form. strandTune=3.86 (minor third) creates a doubly-linked minor chord where the internal resonance of each ring adds a characteristic richness to each chord tone that normal additive synthesis cannot replicate. The Saw waveform brings harmonic density — the minor third interval creates rich intermodulation through the cross-ring coupling (0.3). A foundation minor chord that the topology makes three-dimensional.

**3. Figure-Eight Foundation**
`Foundation` | Figure-Eight topology, braidDepth=0.6, low filterCutoff=800 Hz, high velocity response.
*The alternating over-under crossings as a bass/mid foundation character.*
Parameters: `weave_knotType=1, weave_braidDepth=0.6, weave_filterCutoff=800, weave_filterReso=0.25, weave_strandType=2, weave_ampA=0.008, weave_ampD=0.4, weave_ampS=0.72, weave_ampR=0.8`
Insight: Figure-Eight in Foundation presets is largely absent (it's used primarily in Flux for its movement quality). At filterCutoff=800 Hz, the 2000 Hz velocity offset creates a 3.5× brightness range — soft notes stay thick and low, hard notes push through the midrange. The alternating positive/negative coupling at 0.6 braidDepth creates a small amount of internal opposition that makes the bass feel "fought for" — there's a compression quality at the center that standard oscillator mixing doesn't produce.

---

### Atmosphere Tier (3 Presets)

**4. Braid Breath**
`Atmosphere` | LFO 1 targeting Braid Depth (target=5), rate 0.04 Hz, sweeping from 0.2 to 0.9.
*The topology appears and disappears on a 25-second cycle.*
Parameters: `weave_knotType=2, weave_braidDepth=0.2, weave_torusP=3, weave_torusQ=5, weave_lfo1Type=1, weave_lfo1Target=5, weave_lfo1Depth=0.7, weave_lfo1Rate=0.04, weave_strandType=1, weave_fx1Type=3, weave_fx1Mix=0.35`
Insight: **The first preset in the entire library to use LFO targeting Braid Depth (target=5) as its primary design premise.** At rate=0.04 Hz, the 25-second cycle means braidDepth sweeps from 0.2 (detuning zone) to 0.9 (topology zone) and back over half a minute. The WEAVE-I threshold crossing (around 0.45 on the sweep up) is audible as a timbral phase transition — the sound changes character at that point. A sustained chord goes through two distinct characters per minute. The listener experiences the topology as something that arrives and departs.

**5. Chimera Atmosphere**
`Atmosphere` | knotType=0 (Trefoil), macroKnot=0.5 as stable starting point, slow LFO on filter, reverb.
*The Trefoil-to-Figure-Eight chimera as a sustained pad identity.*
Parameters: `weave_knotType=0, weave_macroKnot=0.5, weave_braidDepth=0.58, weave_strandType=1, weave_lfo2Type=1, weave_lfo2Target=2, weave_lfo2Depth=0.2, weave_lfo2Rate=0.07, weave_fx2Type=2, weave_fx2Mix=0.25, weave_fx3Type=3, weave_fx3Mix=0.4`
Insight: macroKnot=0.5 creates an equal blend of the Trefoil matrix (three-strand ring, writhe +3) and the Figure-Eight matrix (alternating positive/negative polarity). The result: directional flow from the Trefoil combines with tension-release opposition from the Figure-Eight. Neither topology dominates — the sound is the product of their equal interpolation. This chimera state has been swept through in performance presets but never held. At braidDepth=0.58, both topological characters are audible in the static pad.

**6. Solomon Octave Ring**
`Atmosphere` | Solomon topology, strandTune=12.0 semitones (octave), braidDepth=0.7, Sine strands, long release.
*Ring A at fundamental, Ring B one octave above. The octave pair internally resonant.*
Parameters: `weave_knotType=3, weave_strandTune=12.0, weave_braidDepth=0.7, weave_strandType=1, weave_ampR=4.5, weave_filterCutoff=5000, weave_filterReso=0.15, weave_fx3Type=3, weave_fx3Mix=0.5, weave_fx3Param=0.7`
Insight: Solomon with octave strandTune creates what the WEAVE-V verse calls a "two-ring chord pad" — but at the octave rather than the fifth, the two rings are harmonically unified (Ring B is the octave of Ring A) while still being structurally separate (each ring has its own internal coupling). The intra-ring strength (0.8) causes Ring B (octave) to have its own phase resonance independent of Ring A. The cross-ring coupling (0.3) creates a gentle modulation of the fundamental by the octave — not a simple octave addition, but a coupled octave that the lower ring feels.

---

### Aether Tier (3 Presets)

**7. Full Lock**
`Aether` | braidDepth=1.0, Trefoil, Sine, Poly8, long release (8 seconds), reverb.
*Maximum coupling — the four strands are held by the topology.*
Parameters: `weave_knotType=0, weave_braidDepth=1.0, weave_strandType=1, weave_voiceMode=3, weave_ampR=8.0, weave_ampA=0.3, weave_ampS=0.6, weave_filterCutoff=1800, weave_filterReso=0.35, weave_fx3Type=3, weave_fx3Mix=0.65, weave_fx3Param=0.8`
Insight: **No factory preset reaches braidDepth=1.0.** At maximum coupling, each strand's phase increment is perturbed by up to ±160 Hz per sample. With Trefoil topology: Strand 0 is pulled toward Strand 1 at ±140 Hz (0.7 coefficient × 200 Hz scale) and pushed by Strand 3 at ±60 Hz (−0.3 × 200). The four strands attempt to phase-lock into the Trefoil's topologically stable ratio. They partially succeed, partially fail — the result is a dense, harmonically rich pad with an internal structure determined entirely by mathematics. With Poly8 and 8-second release, up to 8 independent topology instances accumulate.

**8. Topology Archive**
`Aether` | macroKnot=0.75, knotType=1 (Figure-Eight → Solomon blend), braidDepth=0.85, slow LFO.
*Three-quarter position between Figure-Eight and Solomon — 25% Figure-Eight, 75% Solomon.*
Parameters: `weave_knotType=1, weave_macroKnot=0.75, weave_braidDepth=0.85, weave_strandType=1, weave_strandTune=5.0, weave_lfo1Type=1, weave_lfo1Target=1, weave_lfo1Depth=0.08, weave_lfo1Rate=0.02, weave_ampA=0.5, weave_ampR=10.0, weave_fx2Type=3, weave_fx2Mix=0.7, weave_fx2Param=0.85`
Insight: macroKnot=0.75 blends the Figure-Eight matrix (75% weight → 25% remaining) with the Solomon matrix (25% weight → 75% active). The result: the alternating polarity of the Figure-Eight is visible in the coupling but dominated by Solomon's ring architecture. At strandTune=5.0 (major third), Ring B (strands 2–3 at Solomon) sits a major third above Ring A — a coupled major chord chimera. The 10-second release means that a single keystroke evolves for 10 seconds through the chimera's topology.

**9. The Kelp Forest**
`Aether` | All four macro parameters nonzero, braidDepth=0.75, Torus P=5 Q=8 (golden torus), very long release.
*The golden torus at near-maximum WEAVE, TENSION, KNOT, SPACE simultaneously.*
Parameters: `weave_knotType=2, weave_torusP=5, weave_torusQ=8, weave_braidDepth=0.75, weave_macroWeave=0.15, weave_macroTension=0.3, weave_macroKnot=0.4, weave_macroSpace=0.5, weave_strandType=1, weave_ampA=0.8, weave_ampR=12.0, weave_ampS=0.55, weave_lfo1Type=1, weave_lfo1Target=5, weave_lfo1Depth=0.15, weave_lfo1Rate=0.03, weave_fx3Type=3, weave_fx3Mix=0.75, weave_fx3Param=0.9`
Insight: WEAVE-III verse identifies the golden torus (P=5, Q=8, φ≈8/5) as the maximum polarization Torus variant. Combined with SPACE=0.5 pushing reverb open and KNOT=0.4 blending toward the Solomon matrix, the golden torus coupling creates a slow-rotating spiral character in the pad that changes shape as the KNOT blend shifts the matrix. A 12-second release means the full forest — all the coupling, all the reverb, the golden spiral — decays for almost quarter of a minute.

---

### Prism Tier (2 Presets)

**10. Trefoil Spectrum**
`Prism` | Trefoil, Square strands, braidDepth=0.72, bandpass filter, high resonance.
*Odd-harmonic intermodulation through the three-strand ring.*
Parameters: `weave_knotType=0, weave_braidDepth=0.72, weave_strandType=3, weave_filterCutoff=1200, weave_filterReso=0.55, weave_filterType=2, weave_lfo1Type=2, weave_lfo1Target=2, weave_lfo1Depth=0.35, weave_lfo1Rate=0.18, weave_fx2Type=1, weave_fx2Mix=0.22, weave_fx2Param=0.4`
Insight: Square strands through the Trefoil matrix at braidDepth=0.72. The Square's odd harmonic stack (fundamental, 3rd, 5th, 7th...) is cross-modulated through the three-strand ring. The directional writhe (+3) creates a harmonic rotation: odd harmonics of Strand 1 modulate Strand 0, which is already modulating Strand 2. The intermodulation products fall at sum-and-difference intervals in the odd harmonic series. Through a bandpass filter at 1200 Hz with resonance 0.55, this produces a focused spectral peak that moves with the LFO — a formant-like effect produced by topology rather than vocal modeling.

**11. Figure-Eight Prism**
`Prism` | Figure-Eight, Triangle strands, braidDepth=0.55, high LFO on both braid and filter simultaneously.
*Soft harmonic content through alternating positive/negative coupling.*
Parameters: `weave_knotType=1, weave_braidDepth=0.55, weave_strandType=4, weave_filterCutoff=3500, weave_filterReso=0.4, weave_lfo1Type=1, weave_lfo1Target=5, weave_lfo1Depth=0.25, weave_lfo1Rate=0.22, weave_lfo2Type=1, weave_lfo2Target=2, weave_lfo2Depth=0.3, weave_lfo2Rate=0.15, weave_fx2Type=2, weave_fx2Mix=0.3, weave_fx2Param=0.45`
Insight: Triangle strands bring the fundamental and second partial only — the narrowest harmonic spectrum of any waveform. Through the Figure-Eight alternating coupling, the opposition creates subtle difference tones between the two partials of adjacent strands — the Figure-Eight becomes a gentle beating machine. Two simultaneous LFOs (braidDepth at 0.22 Hz, filter at 0.15 Hz) create two independent rhythmic cycles that are incommensurate — the full pattern repeat is `lcm(1/0.22, 1/0.15)` seconds. The Prism character emerges from the incommensurate LFO pair continuously refracting the spectral content.

---

### Flux Tier (2 Presets)

**12. Braid Convulsion**
`Flux` | braidDepth=0.9, fast S&H LFO on braid depth, Figure-Eight topology.
*The topology at near-maximum, randomly interrupted each sample.*
Parameters: `weave_knotType=1, weave_braidDepth=0.9, weave_strandType=2, weave_lfo1Type=5, weave_lfo1Target=5, weave_lfo1Depth=0.5, weave_lfo1Rate=6.0, weave_lfo2Type=1, weave_lfo2Target=2, weave_lfo2Depth=0.4, weave_lfo2Rate=0.3, weave_filterCutoff=4500, weave_filterReso=0.38, weave_fx1Type=1, weave_fx1Mix=0.25, weave_fx1Param=0.55, weave_voiceMode=0`
Insight: S&H LFO at 6 Hz on braid depth with depth=0.5 means braidDepth jumps randomly between 0.9±0.5 — alternating between 0.4 (detuning zone) and 1.0 (full topology lock) approximately 6 times per second. The random jumps produce a stuttering, convulsive character — the topology crystallizes and dissolves at audio-adjacent rates. The Figure-Eight's alternating polarity amplifies the instability. This is MACRO KNOT and braid depth both in extreme territory simultaneously.

**13. Knot Storm**
`Flux` | macroKnot fast LFO (rate=0.8 Hz, depth=0.95), high braidDepth=0.78, Solomon.
*The topology itself in rapid motion.*
Parameters: `weave_knotType=3, weave_macroKnot=0.0, weave_braidDepth=0.78, weave_strandType=1, weave_lfo1Type=1, weave_lfo1Target=2, weave_lfo1Depth=0.45, weave_lfo1Rate=0.8, weave_lfo2Type=1, weave_lfo2Target=1, weave_lfo2Depth=0.1, weave_lfo2Rate=0.25, weave_filterCutoff=5500, weave_filterReso=0.45, weave_fx2Type=2, weave_fx2Mix=0.35, weave_macroKnot=0.0, weave_macroSpace=0.3`

*Note: Use MACRO KNOT (weave_macroKnot) to automate the topology sweep in performance — the LFO is on the filter, but KNOT at 0.8 Hz via automation creates the storm.*

Insight: Solomon at braidDepth=0.78 is already in the deep topology zone — the two rings are strongly coupled. MACRO KNOT sweeping at 0.8 Hz moves the coupling matrix from Solomon toward Torus (knotType+1=0, which wraps to Trefoil) and back. The coupling matrix itself is changing at sub-second speed. The Solomon ring structure dissolves and reforms on each cycle. The sound has no stable topology — it exists between knot identities.

---

### Entangled Tier (2 Presets)

**14. Knot Entanglement** (ORBWEAVE + OUTWIT)
`Entangled` | OUTWIT CA rhythms modulate ORBWEAVE braid depth via KnotTopology coupling.
*The octopus arm patterns reshape the topology in real time.*
Parameters — ORBWEAVE: `weave_knotType=3, weave_braidDepth=0.55, weave_strandType=1, weave_strandTune=7.02, weave_filterCutoff=3500, weave_filterReso=0.3, weave_ampR=2.5, weave_macroWeave=0.2, weave_macroKnot=0.3, weave_fx3Type=3, weave_fx3Mix=0.45`
Parameters — OUTWIT: `owit_arm0Rule=110, owit_arm3Rule=30, owit_synapse=0.45, owit_chromAmount=0.4, owit_stepRate=3.5, owit_denMix=0.2`
Insight: The existing Outwit_Orbweave_Irreducible_Link preset uses bidirectional KnotTopology coupling at moderate levels (0.13/0.11). This Transcendental version uses OUTWIT's arm rhythm patterns (Rule 110 = complex, Rule 30 = chaotic) via AudioToFM coupling to modulate ORBWEAVE's phase — the CA step events drive frequency modulation on the strands. The knot topology then processes this modulation through its matrix. OUTWIT's cellular automaton is feeding the knot. Solomon's doubly-linked rings distribute the CA input across both ring pairs differently.

**15. Trefoil Bloom** (ORBWEAVE + OXBOW)
`Entangled` | ORBWEAVE Trefoil output feeds OXBOW's Chiasmus FDN via AmpToFilter coupling.
*The phase-braided topology blooms through the entangled reverb.*
Parameters — ORBWEAVE: `weave_knotType=0, weave_braidDepth=0.82, weave_strandType=1, weave_filterCutoff=2200, weave_filterReso=0.28, weave_ampA=0.02, weave_ampR=3.0, weave_macroWeave=0.25`
Coupling: `source=Orbweave, target=Oxbow, type=AmpToFilter, amount=0.35`
Insight: OXBOW's Chiasmus FDN produces an "entangled reverb" whose four delay lines are cross-coupled. When ORBWEAVE's amplitude modulates OXBOW's filter (AmpToFilter), the knot topology coupling in ORBWEAVE's strands creates an amplitude envelope that contains the topology's character — the periodic crystallizations and dissolutions of the Trefoil's phase lock become amplitude events that reshape OXBOW's reverb character. Two types of entanglement: phase entanglement inside ORBWEAVE, and amplitude coupling between ORBWEAVE and OXBOW.

---

## Phase R4: Scripture — New Verses for Book VII

### OBW-VII: The Full Lock State
*"At braidDepth=1.0, the four strands are no longer four independent oscillators. They are one topological object expressed through four voices. Each strand knows the current phase of its neighbors with a precision that free oscillators never achieve. The Trefoil ring has locked at its mathematically preferred ratio. The Solomon rings have found their doubly-coupled stability. You are not playing four oscillators. You are playing the knot."*

**Application:** Do not treat braidDepth=1.0 as extreme or dangerous. It is a complete state — the endpoint of the braid, the full commitment to topology. Foundation presets that demonstrate this state make the engine's unique character maximally legible.

### OBW-VIII: The LFO Teaches the Listener
*"A preset that holds braidDepth=0.7 demonstrates one topology. A preset whose LFO sweeps braidDepth from 0.2 to 0.9 teaches the listener what the topology is. The sweep through the WEAVE-I threshold (~0.45) is audible as a categorical change: the sound crosses from the detuning zone into the topology zone. The listener hears this boundary even without knowing its name. Design presets that make this crossing an event, not an accident."*

**Application:** LFO target = Braid Depth (target=5) is ORBWEAVE's most powerful pedagogical parameter. Rate 0.03–0.07 Hz places the topology crossing on a human-scale timeline (14–33 seconds per sweep). This rate range teaches topology through time rather than through documentation.

### OBW-IX: The Chimera Has an Identity
*"macroKnot=0.5 is not the halfway point between two topologies. It is a third topology that does not appear in any mathematical classification. The Trefoil-to-Figure-Eight chimera exists only during the interpolation — it has its own coupling matrix, its own sonic character, its own relationship between strands. When you set macroKnot=0.5 as the fixed starting point of a preset and design the sound from there, you are designing from within a topology that has no name. Name it. It deserves one."*

**Application:** Design at least one preset per session starting with macroKnot set to a fixed nonzero value (0.25, 0.5, or 0.75). The chimera state is reproducible and stable — it will sound the same every time you open that preset. Give it a name that describes what you hear, not what the topology mathematics say.

### OBW-X: Solomon Is the Chord Engine
*"Every other knot topology in ORBWEAVE treats all four strands as part of one structure. The Solomon knot treats strands 0–1 as Ring A and strands 2–3 as Ring B, with different coupling strengths within each ring (0.8) and between them (0.3). This is the only ORBWEAVE topology that is explicitly bipartite — designed around two groups. When strandTune offsets Ring B by a harmonic interval, ORBWEAVE becomes a chord synthesizer where each chord tone has its own internal phase resonance. The fifth, third, octave, and second are all available. No other synthesizer produces two-part harmony this way."*

**Application:** For all Solomon presets with strandTune ≠ 0: choose the interval musically before choosing any other parameter. The strandTune interval determines the entire harmonic character of the preset. Set it first, then find the braidDepth and filterCutoff that make that interval speak most clearly.

---

## Summary

**15 Transcendental presets delivered:**

| Name | Mood | Key Territory Explored |
|------|------|------------------------|
| Trefoil Locked | Foundation | braidDepth=0.88, first Foundation preset above 0.85 |
| Solomon Minor Third | Foundation | Solomon + strandTune=3.86 (minor third) chord architecture |
| Figure-Eight Foundation | Foundation | Figure-Eight as foundation character, low cutoff + velocity |
| Braid Breath | Atmosphere | LFO → Braid Depth as topology appearance/disappearance |
| Chimera Atmosphere | Atmosphere | macroKnot=0.5 as stable preset identity |
| Solomon Octave Ring | Atmosphere | Solomon + strandTune=12.0 (octave), internally resonant octave pair |
| Full Lock | Aether | braidDepth=1.0, first in library at maximum coupling |
| Topology Archive | Aether | macroKnot=0.75 as stable Figure-Eight→Solomon chimera |
| The Kelp Forest | Aether | Golden torus P=5 Q=8, all macros nonzero, 12s release |
| Trefoil Spectrum | Prism | Square strands through Trefoil, bandpass, odd harmonic intermod |
| Figure-Eight Prism | Prism | Triangle + Figure-Eight + incommensurate LFO pair |
| Braid Convulsion | Flux | S&H LFO on braidDepth at 6 Hz — topology crystallizes/dissolves |
| Knot Storm | Flux | MACRO KNOT at sub-second speed, Solomon dissolving and reforming |
| Knot Entanglement | Entangled | OUTWIT CA rhythms via KnotTopology reshaping ORBWEAVE topology |
| Trefoil Bloom | Entangled | ORBWEAVE→OXBOW AmpToFilter — topology coupling feeds entangled reverb |

**4 Scripture verses:** OBW-VII through OBW-X

**Key insight:** The factory library demonstrates ORBWEAVE's topologies across a wide parameter range. The Transcendental library commits to specific, extreme, or underrepresented states: maximum braidDepth, Solomon chord architectures, LFO-as-topology-teacher, and the chimera state as a stable identity rather than a transient destination.

---

*Guru Bin — ORBWEAVE Vol 3 Retreat complete*
*"The knot does not describe the braid. The braid instantiates the knot."*
