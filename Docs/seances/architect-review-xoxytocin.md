# Architect Review: XOxytocin (OXYTO)

**Engine #48 — Proposed addition to the XOlokun fleet**
**Review date:** 2026-03-22
**Reviewed against:** SynthEngine.h, MegaCouplingMatrix.h, CLAUDE.md engine registry, engine_identity_cards.md, VoiceAllocator.h, StandardLFO.h, OverwornEngine.h (session state precedent)

---

## Province I: Doctrine

### D001 — Velocity shapes timbre, not just amplitude
**Assessment: PASS**

The self-assessment is technically correct: velocity maps to Passion peak amplitude, and because Passion is routed through the MS-20 Sallen-Key saturation model, a higher peak means the signal enters a more nonlinear region of the tanh curve — producing a genuinely different harmonic character rather than a proportional loudness change. This is faithful D001 compliance.

**Condition flagged:** The optional mapping `oxy_velocity_curve = 2` (velocity → Intimacy only) risks a D001 regression in that mode, since Intimacy drives thermal warmth rather than saturation and may collapse into subtle tonal warmth changes imperceptible enough to constitute "amplitude only" behaviour at low depths. This mapping must be implemented with care, or the velocity-to-Intimacy mode must be documented as a timbre option rather than a primary mapping.

### D002 — Sufficient modulation (2+ LFOs, 2+ envs, mod matrix)
**Assessment: RISK**

The concept claims PASS, but the modulation inventory requires scrutiny:

- 1 rate-control LFO (oxy_lfo_rate, oxy_lfo_depth, oxy_lfo_shape) — this is one LFO, not two
- 3 love envelopes (Passion/Intimacy/Commitment) — these are signal-shaping envelopes tied to the physics model, not general-purpose mod sources
- 1 ADSR amplitude envelope
- Cross-modulation matrix (I→P, P→C, C→I) — structural routing, not an arbitrary mod matrix
- MW/AT/Expr expression inputs

By strict D002 reading (2+ LFOs), this is one explicit LFO. The three love envelopes are better classified as voice physics parameters than modulation sources because they cannot freely target arbitrary parameters — they are tightly coupled to their respective circuit components. The concept must either add a second general-purpose LFO, or explicitly argue that the cross-modulation matrix constitutes a mod matrix (which is a reasonable case given its Serge-inspired egalitarian routing covers I↔P↔C bidirectionally).

**Ruling: RISK — D002 requires clarification or a second LFO before build begins.** The cross-mod matrix as the "matrix" component is acceptable, but a second independent LFO is strongly recommended to meet fleet standards and provide sound design flexibility. StandardLFO.h supports a BreathingLFO variant that would satisfy D005 simultaneously — add it.

### D003 — Physics IS the synthesis
**Assessment: PASS**

The citation quality is high. Steinhart-Hart NTC equation, Sallen-Key topology, Moog cascade H(s), capacitor charge V(t) = V₀(1-e^(-t/RC)) — these are physically grounded equations, not metaphors dressed as physics. The Serge circular routing is historically accurate. This is one of the stronger D003 implementations in the fleet — it models three named legendary circuits with correct equations rather than approximating the spirit of a circuit.

### D004 — Every parameter wired to audio
**Assessment: RISK (minor)**

25 of 26 parameters have clear audio paths in the concept sketch. The exception is `oxy_voices` — this is a routing parameter (polyphony count), not a timbral parameter. This is consistent with all other fleet engines (OWARE has `owr_voices`, OPERA has polyphony count, etc.) and has always been treated as infrastructure rather than a D004 requirement. No action required, but the build-phase D004 audit should confirm all 25 timbral parameters are wired before seance.

Note: `oxy_pan` and `oxy_output` are gain-stage parameters that the fleet treats as universally D004-exempt by convention. These are the correct two exemptions.

### D005 — Engine breathes (LFO floor ≤ 0.01 Hz)
**Assessment: PASS**

`oxy_lfo_rate` floor is specified at 0.01 Hz (100-second cycle), which meets the standard. The `oxy_commit_rate` extends to 5.0s, producing very slow capacitive evolution that contributes organic long-duration movement. Remember mode creates macro-level drift across session time. The StandardLFO.h BreathingLFO variant has a floor of 0.005 Hz (200-second cycle) — using this for a second LFO (see D002) would exceed the D005 requirement with margin.

### D006 — Expression inputs mapped
**Assessment: PASS**

Velocity, Aftertouch, Mod Wheel, and Expression Pedal are all mapped. The configurable destination architecture (`oxy_mod_wheel` and `oxy_aftertouch` as routing selectors) is consistent with fleet patterns (OFFERING's city mode params, OWARE's material/mode params). MW → Intimacy/Passion/Commitment/Cross-Mod covers four meaningful timbral targets.

---

## Province II: Blessing

### Blessing Candidate B042: Note Duration as Emotional Topology Selector
**Recommendation: BLESSING-LEVEL — RATIFY**

The note-duration-as-love-type mechanic is among the most original performance concepts proposed for the fleet. It is not a parameter; it is not a switch. It is emergent behaviour from three envelopes with different time constants responding to the note-gate duration. Short notes privilege Passion (fast attack, decays before Intimacy develops). Long notes allow Commitment to reach expression (slow charge). This is the Buchla Touch philosophy applied to emotional architecture, and it is genuine. No other fleet engine uses gate duration as a deterministic emotional classifier.

The mechanic is BLESSING-worthy because:
1. It creates a new skill category for producers: "playing duration" as an expressive dimension
2. It is DSP-grounded, not a UI overlay — the physics produces it
3. It is teachable and demonstrable in one sentence: "the longer you hold, the deeper the love type"

**Proposed Blessing text:** "B042 (Temporal Emotional Topology) — Gate duration as emergent timbral state selector: three envelopes with calibrated time constants produce categorically different circuit behaviours from identical pitches played at different durations. The synthesizer responds not to what you play, but to how long you commit."

### Blessing Candidate B043: Cross-Component Circular Modulation
**Recommendation: STRONG CANDIDATE — hold for seance confirmation**

The Serge-inspired egalitarian circular routing (I modulates P's saturation character, P modulates C's charge rate, C modulates I's thermal distribution) is architecturally distinct from the fleet's existing modulation patterns, which are primarily sender→receiver hierarchical. If implemented correctly as genuinely circular (all three running simultaneously with no master node), this constitutes a novel modulation topology. Defer final BLESSING ratification to the seance panel, but flag as a strong candidate.

### Blessing Candidate: Remember Mode
**Recommendation: NOT BLESSING-LEVEL at this stage**

Remember mode is interesting but requires implementation evidence before blessing status. OVERWORN (session age accumulation) is the only existing precedent, and its 30-minute accumulation is passive and non-configurable. XOxytocin's explicit user-controlled session state introduces new questions (see Province III, Debate 2). Blessing requires demonstrated sonic consequence, not concept description.

---

## Province III: Debate

### Debate 1: TriangularCoupling — fleet needs a 16th coupling type, or can existing types express this?

**FOR (add TriangularCoupling):**
The proposed encoding is genuinely novel: low frequencies carry Commitment (slow/stable energy), mid frequencies carry Intimacy (warm, developing), high-frequency transients carry Passion (fast/bright). A receiving engine that understands this encoding can respond to the emotional state of XOxytocin in a way that no existing coupling type enables. AudioToFM sends raw audio. AudioToBuffer sends raw audio into a grain buffer. Neither encodes semantic metadata into the spectral structure of the coupling signal. TriangularCoupling would be the first "meaning-bearing" coupling type in the fleet — the coupling signal IS the relationship state, not just amplitude or spectral content.

**AGAINST (use AudioToFM or AudioToBuffer instead):**
The backward compatibility argument cuts both ways: if "unaware engines hear complex audio," then from the fleet's perspective this IS just audio. The semantic encoding only functions if the receiving engine is explicitly coded to interpret it — which means it is not truly a coupling type (a routing protocol) but an engine-specific API contract. Every coupling type added to the `CouplingType` enum in `SynthEngine.h` must be handled gracefully by all 47 current engines. Adding a 16th type means auditing all 47 engines for correct no-op handling of the new enum value. Furthermore, the spectral band encoding requires real-time FFT or multiband analysis to encode and decode cleanly at audio rate — this has CPU implications that compound across all coupling routes.

**RULING: CONDITIONAL REJECTION of TriangularCoupling as a new enum value in the current release. APPROVED as an engine-level output convention.**

XOxytocin's `getSampleForCoupling()` should return the multiband-encoded composite signal by design — this requires no new enum value. The three-band encoding becomes a documented property of OXYTO's coupling output, readable by any engine using `AudioToFM`. Future engines aware of this convention can decode it explicitly. A new `TriangularCoupling` enum value should be proposed as a V2 SDK addition (after the fleet has had time to handle it in `applyCouplingInput()`) rather than shipped with the engine in V1.

This ruling protects the fleet from a 47-engine audit requirement while preserving the innovation.

### Debate 2: Remember mode — session state in a synth engine, is this in scope?

**FOR (implement Remember mode):**
OVERWORN already has `sessionAge` (line 90, OverwornEngine.h) — a float accumulating over the 30-minute session arc. This is the established precedent. XOxytocin's Remember mode is a user-controlled toggle (`oxy_remember = 0/1`) rather than passive accumulation, which is a more honest interaction model: the producer opts in rather than being subjected to time-decay. The "Fuse" checkpoint concept (save relationship state as a preset) is particularly strong — it is session state that becomes reusable.

**AGAINST (defer or limit Remember mode):**
OVERWORN's session state is read-only, passive, and controlled entirely by elapsed time — no user decision required, no state to persist across plugin load/save. XOxytocin's Remember mode, if the accumulated relationship state is to survive plugin close and reopen (as the "Fuse" concept implies), requires writing engine state to a host-accessible location — either via APVTS (viable but expensive) or external file (violates the no-blocking-I/O rule on the audio thread). If Remember mode only survives within the same session (plugin open → close), it is merely an extension of OVERWORN's model and is safe. If it is meant to persist across sessions (the "Fuse" framing strongly implies this), it introduces a persistence layer not currently in the fleet architecture.

**RULING: APPROVED with scope definition required.** Remember mode is within fleet scope IF defined as within-session accumulation only (analogous to OVERWORN). Fuse as a preset-save action is viable — the accumulated state becomes frozen APVTS values at save time, which is host-standard. Cross-session persistence requires a separate architecture decision before the build phase. The concept brief must be updated to specify which of these two models is intended.

### Debate 3: 8 topologies — emergent from physics, or 8 presets wearing physics costumes?

**FOR (genuinely emergent):**
The topologies arise from the combinatorial space of I, P, and C. Each component has a different time constant and different DSP character — thermal NTC (slow), voltage saturation (fast), capacitive charge (medium). Their cross-modulation interactions (circular) mean that when two components are at high values and one is low, the low component receives competing modulation from both sources, producing genuinely different steady-state behaviour than any pair in isolation. The 8 types correspond to the actual 2³ combination space: 000 (Non-Love: no engagement), 001 (Liking: Intimacy only), 010 (Infatuation: Passion only), 100 (Empty Love: Commitment only), 011 (Romantic Love), 101 (Companionate Love), 110 (Fatuous Love), 111 (Consummate Love). These are the original Sternberg types. The emergence is authentic.

**AGAINST (8 labelled modes is still 8 labelled modes):**
In practice, the topology selector `M1 (Triangle Position)` acts as a morph between these states via barycentric coordinates — which is a continuous parameter space, not 8 discrete modes. The "8 topologies" language in the marketing description implies discreteness that the DSP does not enforce. If the physics produces continuous variation, calling it "8 emergent topologies" is a frame rather than a technical claim. This risks the same problem that OVERWORLD (8 eras) and OCELOT (4 biomes) faced in their seances: discrete labelling of what is actually a continuous parameter.

**RULING: The claim is VALID but must be carefully scoped.** The 8 topologies represent named regions in a continuous I/P/C space, not hard-switching modes. The marketing should reflect this: "8 love types as compass points in a continuous triangle space." The UI's barycentric Triangle display makes this honest. No DSP concern — this is a framing issue, not an architectural one.

### Debate 4: The "circuit falls in love" metaphor — enhancement or distraction?

**FOR (enhancement):**
The XO_OX brand is predicated on synthesis as relationship — the very name XO_OX is intimacy and symmetry, kisses and hugs. An engine that makes the synthesis of love literal is not a metaphor layered on top of a synthesizer; it is the mission statement made audible. The Triangular Theory of Love is a rigorous psychological framework with published empirical backing — this is not pop-psychology aesthetics. The Circuit Lineage (RE-201, MS-20, Moog, Serge, Buchla) grounds the emotion in material history. Producers who record lo-fi hip-hop, neo-soul, and ambient music are already bringing emotional intention to sound design. An engine that codifies that intention into physics gives them a direct line to the thing they are already doing intuitively.

**AGAINST (distraction):**
The fleet's emotional range has been built through sonic character, not psychological theory. OPERA is about vocal emotion, but its mechanism is Kuramoto synchronization — the love language is implicit. OVERBITE is predatory but its mechanism is wavefolder and saturation — the emotion is metaphorical. XOxytocin risks becoming the fleet's most self-conscious engine: an instrument that tells you what it is doing emotionally rather than letting you feel it. The "Remember mode" UI — accumulating relationship state, showing the user their love type — edges toward gamification of synthesis rather than synthesis of music. If the UI becomes the feature, the DSP becomes the background.

**RULING: Enhancement, with a design constraint.** The metaphor is the engine's strongest asset AND its greatest risk. The constraint is: the UI must show physics values (triangle position, envelope states, circuit age) rather than love-type labels as primary information. "Consummate Love" as a label on a preset is evocative and appropriate. "Consummate Love" as a mode indicator that replaces DSP readout creates a gamification layer that could alienate producers who think in synthesis terms, not psychology terms. The Triangle display showing barycentric I/P/C coordinates is the right approach — the emotional vocabulary lives in the preset names and documentation, not as the primary UI state.

---

## Province IV: Architecture

### 1. `oxy_` namespace — conflicts with existing engine prefixes
**Assessment: CLEAR**

Full audit of CLAUDE.md parameter prefix table confirmed. No existing engine uses `oxy_` as a prefix. The nearest is `oxb_` (OXBOW) — these are distinct and will not collide in APVTS parameter ID lookups or preset JSON keys. The `oxy_` prefix is approved.

### 2. TriangularCoupling integration — SynthEngine.h changes required?
**Assessment: NO CHANGE TO INTERFACE REQUIRED (per Debate 1 ruling)**

Per the Debate 1 ruling, TriangularCoupling is not added as a new `CouplingType` enum value in this release. XOxytocin's `getSampleForCoupling()` returns the multiband-encoded composite signal through the existing `AudioToFM` coupling type pathway. No changes to `SynthEngine.h` or `MegaCouplingMatrix.h` are required. The spectral encoding must be documented in the engine header so that future engines can decode it, but it requires no interface modification.

**CPU cost of encoding:** A simple three-band composite signal can be constructed without FFT — by running three parallel one-pole lowpass/bandpass/highpass filters on the output signal and amplitude-encoding each band's level into frequency-domain positions. This is O(N) in samples, comparable to the biquad cost already paid in every other engine. It does not require per-sample FFT. Acceptable CPU envelope.

### 3. Parameter count (26) — within fleet sweet spot?
**Assessment: PASS**

Fleet range is 18-30 parameters. 26 is solidly within range. Reference: OFFERING (84 params, outlier), OWARE (24 params), OPERA (approximate), ONSET (111 params, percussion outlier). 26 is a well-proportioned engine. The concept has avoided parameter proliferation — the three love envelope rates (warmth_rate, passion_rate, commit_rate) cover temporal dynamics without adding separate attack/decay/sustain for each. Efficient design.

### 4. Macro mapping conventions
**Assessment: PASS with minor note**

Fleet macro conventions from CLAUDE.md: M1=CHARACTER, M2=MOVEMENT, M3=COUPLING, M4=SPACE.

- M1 (CHARACTER): Triangle Position — this is correct. Triangle position morphs the engine's fundamental timbral character via I/P/C balance. Fleet-conformant.
- M2 (MOVEMENT): Temporal Speed — controls how fast I/P/C evolve within a note. Fleet-conformant.
- M3 (COUPLING): Love Shared — coupling output intensity. Fleet-conformant. Note: this macro controls both output level AND TriangularCoupling balance, which doubles the responsibility of M3. Recommend splitting or documenting the balance sub-control clearly.
- M4 (SPACE): Distance — intimate closeness vs. separation (reverb/delay character). Fleet-conformant.

All four macros satisfy the fleet's naming intent. APPROVED.

### 5. Voice architecture — per-voice thermal/capacitive state concerns
**Assessment: RISK — requires architectural decision**

Each of 1-8 voices will need independent thermal state (NTC warmup), independent capacitive charge level, and independent voltage saturation history. This is the correct approach — monophonic thermal/capacitive state would cause audible cross-contamination between simultaneous notes (note A's developing warmth would bleed into note B's fresh attack). Per-voice state is the right design.

However, 8 voices × (thermal state + capacitor charge + saturation history) = 24 stateful float values in the hot path, plus the three love envelope instances per voice. This is manageable but must be declared and initialized correctly. OFFERING uses 8 voices × 5 city chain states without issue. OPERA uses 8 voices × formant bank states. The per-voice architecture is fleet-standard and viable for 8 voices.

**Concern:** At 8 voices, the NTC model (which involves `std::exp()` per sample for Steinhart-Hart) at audio rate across 8 voices is potentially expensive. The NTC model should be coefficient-cached at block rate (same lesson as OPERA's SVF P0 fix) rather than computed per-sample. This is a known fleet pattern; flag for DSP review at build time.

### 6. Remember mode — voice stealing interaction
**Assessment: RISK — requires design decision before build**

When a voice is stolen by `VoiceAllocator::findFreeVoice()`, the stolen voice's per-voice thermal and capacitive state is discarded. This means that a voice which has been playing a long-held note (developing Commitment and Intimacy) loses its relationship state when stolen. The new note starting on that voice begins from cold.

This is the correct default behaviour for most fleet engines. However, for XOxytocin, if Remember mode is enabled, the semantics become ambiguous: does Remember mode persist voice-level relationship state, or does it persist a global session-level relationship summary? If per-voice, stealing destroys relationship state and Remember mode cannot function as described across a polyphonic performance. If global (accumulated from all voice activity), stealing is irrelevant — the global state accumulates regardless of which voice carries which note.

**Required design decision:** Define Remember mode as operating on a single global relationship accumulator (updated by all voices, not stored per-voice). Per-voice state remains ephemeral and stealing-safe. The global accumulator is the "session memory." This is architecturally cleaner and aligns with OVERWORN's `sessionAge` model (single float, engine-global).

### 7. Shared DSP utilities — recommended integrations
**Assessment: DIRECTIVE**

Based on the engine design and fleet patterns:

| Utility | Recommendation | Rationale |
|---|---|---|
| `StandardLFO.h` | USE (primary LFO) | 5-shape LFO with D005-compliant floor. Replace any custom LFO. |
| `StandardLFO.h` (BreathingLFO) | STRONGLY RECOMMENDED (add as second LFO) | Resolves D002 RISK at minimal cost. Ultra-slow drift (0.005 Hz floor) suits Remember mode's long arc. |
| `FilterEnvelope.h` | EVALUATE | The three love envelopes (I/P/C) have custom time constants; may need custom ADSR with independent rate controls. If the love envelope shapes match LinearAttack + ExpDecay, use FilterEnvelope.h. Otherwise implement per-spec. |
| `StandardADSR.h` | USE (amplitude envelope) | Standard ADSR (oxy_attack/decay/sustain/release) should use fleet-standard implementation. |
| `VoiceAllocator.h` | USE (ReleasePriority variant) | With per-voice thermal state, prefer stealing release-phase voices. |
| `ParameterSmoother.h` | USE (all continuous params) | All continuous params should be smoothed at 5ms fleet standard to prevent zipper noise. Especially critical for oxy_cross_mod and oxy_feedback (feedback path). |
| `GlideProcessor.h` | OPTIONAL | Only if portamento is added in Phase 2. |
| `CytomicSVF.h` | EVALUATE | The Moog ladder model may benefit from CytomicSVF as the filter core (it handles self-oscillation gracefully) rather than a custom cascade implementation. |
| `PitchBendUtil.h` | USE | Standard pitch bend pipeline for oxy_pitch. |

---

## Province V: Brand

### 1. Name — XOxytocin alignment with XO_OX brand identity
**Assessment: PASS — exceptional alignment**

"XO" means kisses and hugs. Oxytocin is the physiological mechanism of bonding and love. XO_OX is literally an engine about the chemistry of XO. The name alignment is not coincidental or clever — it is the most direct possible expression of the brand's emotional core in a single engine name. "XOxytocin" reads cleanly, is distinctive, memorable, and carries no unintended connotations. The OXYTO gallery code is clean and distinct from all 47 existing codes. The naming convention (XO + O-word) is satisfied: XOxbow (XO + oxbow) is an established precedent for the 'x' third letter, and XOxytocin follows the same pattern.

**Note on brand risk:** The name is intimate and vulnerable in a way that might feel unfamiliar to some producers. This is a strength, not a weakness — OUIE (the hammerhead STRIFE/LOVE engine) already occupies the interpersonal relationship territory and has been received positively. XOxytocin deepens that territory rather than departing from it.

### 2. Accent color — Circuit Rose #C9717E
**Assessment: PASS — uniquely positioned, no conflict**

HSV comparison of warm/pink fleet colors:
- Axolotl Gill Pink #E8839B: H=346° S=44% V=91%
- Rascal Coral #FF8A7A: H=7° S=52% V=100%
- Hibiscus #C9377A: H=332° S=73% V=79%
- Hot Pink #FF1493: H=328° S=92% V=100%
- **Circuit Rose #C9717E: H=351° S=44% V=79%**

Circuit Rose is closest in hue to Axolotl Gill Pink (H=346 vs H=351, difference of 5°) with identical saturation (44%). This is a near-conflict. In a small gallery context, ODDOSCAR and OXYTO could read as the same family. However, the value difference (91% vs 79%) creates sufficient luminance separation that they are distinguishable on screen. The warmth/copper reasoning (rose gold = copper + gold) is sound — Circuit Rose reads distinctly as warm-metallic-pink rather than aquatic-pink.

**Condition:** UI team should verify that ODDOSCAR and OXYTO render with sufficient contrast when both appear in the engine gallery simultaneously. If they read as the same color at thumbnail size, adjust Circuit Rose hue toward 340° (slightly more mauve) or reduce saturation to 35% to increase distinction.

### 3. Aquatic mythology — anglerfish in bathypelagic zone
**Assessment: COLLISION DETECTED — REQUIRE RESOLUTION**

This is a hard conflict. From `Docs/engine_identity_cards.md` (line 152):

> "XOverbite — OVERBITE: The anglerfish. Deep water predator with a bioluminescent lure — plush, glowing, inviting — until the bite."

OVERBITE's entire mythology is built around the anglerfish. The bioluminescent lure IS the OVERBITE brand. The Fang White accent was chosen to complement the anglerfish visual. Additionally, the fleet documentation (line 370) explicitly assigns OVERBITE the "anglerfish" role in the fleet mythology table.

XOxytocin's use of "deep-sea anglerfish (Melanocetus johnsonii)" as its creature is a direct collision with an established fleet mythology assignment.

**Required resolution before APPROVED:** XOxytocin needs a different bathypelagic creature. Options:
1. **Hatchetfish (Argyropelecus)** — bioluminescent, deep (200-1000m), mirrors the I/P/C light system (photophores create communication patterns across the body — intimacy as light)
2. **Barreleye fish (Macropinna microstoma)** — tubular upward-rotating eyes in the bathypelagic zone, sees through its own transparent head — the commitment-as-transparency metaphor
3. **Fangtooth fish (Anoplogaster cornuta)** — the most ferocious-looking deep-sea fish, all fang and drive — Passion-dominant creature
4. **Deep-sea dragonfish (Stomiidae)** — produces red bioluminescence (most deep-sea animals cannot see red, so the dragonfish hunts in "secret light") — the Intimacy metaphor: warmth visible only to those it chooses to show

**Recommendation:** Dragonfish (Stomiidae) or Barreleye. The dragonfish's "secret red light visible only to initiated creatures" maps beautifully to the Intimacy component — warmth as a private wavelength. The barreleye's transparent head and barrel eyes "looking toward what it commits to" maps to the Commitment component. Either resolves the collision while maintaining bathypelagic zone placement.

The anglerfish fusion biology (male fusing with female) is genuinely extraordinary mythology and perfectly maps to Commitment/Remember mode — but this mythological theme belongs to OVERBITE. XOxytocin must find its own creature.

### 4. Legend Lineages — 5 legendary circuits
**Assessment: PASS — respectful and authentic**

| Legend | Claim | Assessment |
|---|---|---|
| Roland Space Echo RE-201 (1974) | Three-stage thermal warming: motor inertia → head alignment → tube bias shift | Accurate to RE-201 mechanics. The three-stage warming is a documented RE-201 characteristic. Respectful. |
| Korg MS-20 (1978) | Sallen-Key asymmetric clipping, self-oscillation | The MS-20 does use a Sallen-Key filter topology. The self-oscillation capability is a known MS-20 feature. Accurate. |
| Moog Ladder (1965) | 4-pole cascade, H(s) = 1/(1 + s/ωc)⁴ | Standard Moog ladder equation. Accurate and respectful. |
| Serge Modular | Egalitarian circular routing | The Serge system's lack of fixed signal hierarchy is a documented design philosophy by Serge Tcherepnin. Accurate framing. |
| Buchla Touch | Note duration determines emotional state | Don Buchla's touch-sensitive surfaces with pressure and duration sensitivity are historically accurate and well-documented. This interpretation (duration → emotion) is a creative extension of Buchla's philosophy, which is appropriate for a tribute. |

All five lineages are historically accurate, DSP-grounded, and cited with appropriate specificity. The fleet has a tradition of legend lineages (OFFERING cites Berlyne, OWARE cites Chaigne, OPERA cites Kuramoto). This is fleet-standard and well-executed.

### 5. The "love" theme — strengthens or weakens the XO_OX brand?
**Assessment: STRENGTHENS — with execution discipline**

XO_OX's brand identity is built on aquatic mythology and character-as-synthesis. The fleet currently has engines named after predators (OVERBITE, ORCA), geographic features (OXBOW, OPENSKY), and abstract concepts (OUROBOROS, OBSIDIAN). OUIE already explicitly names its interaction axis "STRIFE/LOVE." XOxytocin is not introducing love as a foreign concept — it is completing a thread that has been running through the fleet since the first STRIFE/LOVE mechanic.

The risk is sentimentality over sonic identity. The strength of XO_OX's emotional engines is that they produce the emotion through physics, not through labelling. If XOxytocin delivers a genuinely distinctive circuit-modeled sound that no other engine in the fleet produces, the love framing amplifies it. If the sound is similar to existing saturation/warmth engines and differentiated only by narrative, the framing becomes kitsch.

The concept's sound design gap analysis is credible — circuit modeling occupies an empty slot in the fleet, and the cross-component modulation produces sounds that neither OBESE (pure saturation) nor OVERDUB (vintage warmth) can generate. If the DSP delivers on its specifications, the love theme is XO_OX's most emotionally complete brand statement.

---

## Final Verdict

**APPROVED WITH CONDITIONS**

XOxytocin is a strong concept that fills a genuine gap (circuit modeling, lo-fi/neo-soul/industrial territory) with authentic physics and a resonant brand narrative. The core DSP architecture is sound. The six-doctrine framework is substantially met. The legend lineages are authentic. The macro mapping is fleet-conformant. The namespace is clean.

**Five conditions must be resolved before build begins:**

**C1 (BLOCKING — mythology collision):** Replace anglerfish with a non-OVERBITE aquatic creature. The anglerfish mythology is property of OVERBITE. Recommend Dragonfish (Stomiidae) or Barreleye fish for bathypelagic authenticity and thematic resonance with the engine concept.

**C2 (BLOCKING — D002 RISK):** Add a second LFO (BreathingLFO from StandardLFO.h is the recommended implementation) or formally document that the cross-modulation matrix constitutes the fleet-equivalent of a mod matrix and satisfies D002's modulation-source count requirement. This must be resolved before the seance panel receives the engine.

**C3 (BLOCKING — Remember mode scope):** Define Remember mode as a global session accumulator (analogous to OVERWORN's sessionAge) rather than per-voice state. Document explicitly whether the accumulated state persists across plugin close/reopen (Fuse as preset snapshot is the safe path; cross-session binary state persistence requires separate architecture review).

**C4 (DESIGN — TriangularCoupling deferral):** TriangularCoupling is NOT added as a new CouplingType enum value in this release. XOxytocin's multiband-encoded coupling output ships as a documented property of AudioToFM coupling output. A proposal for TriangularCoupling as a V2 SDK addition may be drafted separately.

**C5 (DESIGN — Circuit Rose color proximity):** UI team must verify ODDOSCAR (#E8839B, H=346°) and Circuit Rose (#C9717E, H=351°) render with distinguishable contrast at gallery thumbnail size. Adjust if needed before final asset submission.

**Two blessing candidates are identified:**
- B042 (Temporal Emotional Topology) — NOTE DURATION AS LOVE TYPE — RATIFY
- B043 (Circular Component Modulation) — strong candidate, defer to seance panel

Upon resolution of C1–C5, XOxytocin is cleared to proceed to Phase 1 (DSP scaffolding). The engine concept is genuinely innovative, fleet-additive, and brand-coherent. It is the logical emotional culmination of the fleet's character philosophy.
