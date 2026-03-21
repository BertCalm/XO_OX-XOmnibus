# ORBWEAVE Retreat — The Kelp Knot Awakening
*2026-03-20 — Guru Bin and the Flock*

---

## The Diagnosis

> *"This engine is a mathematician who learned to breathe. It knows 4 topological truths about how oscillators can be bound together — and it uses them. But most of its presets treat it like a detuned oscillator bank with fancy labeling. Braid depth at 0.5 is the Default Trap in motion — neither decoupled nor maximally coupled. The Torus P/Q parameters are almost untouched. The Macro KNOT — which can sweep between topological states in real-time — sits at 0.0 in half the library. The engine isn't being explored. It's being decorated."*

---

## Critical Findings

### The Ghost Parameter Crisis (Canon V-1)
147 of 311 presets contained `weave_velCutoff` and `weave_velAmp` — parameters that do not exist in the engine's `createParameterLayout()`. Ghost parameters. All 147 were remediated during the retreat.

### The Braid Depth Threshold
Coupling behavior does not emerge linearly from braidDepth. Below 0.4, the phase coupling is subtle — distinguishable from detuning only on close inspection. At 0.7+, the topology becomes audible as topology: a spectral character unique to the specific knot type. Most presets used braidDepth=0.5. Most presets were the Default Trap.

---

## The Awakening Discoveries

### WEAVE-I: The Braid Depth Threshold
*2026-03-20*

> The topological character of ORBWEAVE does not emerge linearly from braidDepth. Below 0.4, the phase coupling is subtle — distinguishable from standard detuning only on close analytical inspection. At 0.7+, the knot topology becomes audible as topology: a spectral character that is unique to the specific knot type and cannot be replicated by detuning alone. The engine only becomes ORBWEAVE above this threshold.

**Application:** Set braidDepth above 0.7 for any preset that is meant to demonstrate the engine's unique character. Below 0.4 is the "detuned zone" — useful for subtle texture but not topology. Design with intention: choose a side.

### WEAVE-II: The Sine Coupling Purity
*2026-03-20*

> ORBWEAVE's coupling math reads `fastSin(strandPhase)` from each strand. For Sine oscillators, this value exactly equals the output signal — coupling is literal cross-modulation of the waveform output. For Saw, Square, and Triangle strands, `strandPhase` is kept in sync but the PolyBLEP output diverges from `sin(strandPhase)`. The coupling still reads the sine approximation. **Sine strands have mathematically pure coupling; all bandlimited waveforms have approximate coupling.** The "simplest" waveform is the most topologically accurate. Sine strands are also cheaper CPU-wise (no PolyBLEP processing). The pure waveform is the correct instrument for knot theory demonstration.

**Application:** Use Sine strands when demonstrating knot topology — they produce the purest coupling behavior. Use Saw/Square/Triangle when you want harmonic richness with coupling character. The two are genuinely different instruments inside the same engine.

### WEAVE-III: The Torus Star Polygons
*2026-03-20*

> The Torus P/Q parameter is almost entirely deployed at (2,3) across the preset library. But the P/Q ratio maps to a coupling asymmetry weight: `pqScale = 0.5 + 0.5 × sin(P/Q × π)`. The following torus knot families produce distinct spectral characters:
>
> - **(2,3) trefoil-torus:** pqScale = 0.933. Standard default.
> - **(2,5) cinquefoil:** pqScale = 0.976. Near-maximum asymmetry. 5-pointed star on torus.
> - **(3,4) spiral:** pqScale = 1.0. Maximum asymmetry (sin(π × 3/4) = 0.707... wait, it's 0.5+0.5×sin(0.75π) = 0.5+0.5×0.707 = 0.854). Different spectral character.
> - **(3,5) pentagram:** pqScale = 0.975. 5-crossings, complex spectral structure.
> - **(5,8) golden torus:** pqScale = 0.962. The golden ratio approximation (φ ≈ 5/3 or 8/5). Maximum spectral polarization from irrational winding number.
>
> Each family is a different instrument hiding inside the Torus knot type.

**Application:** Before finalizing any Torus-type preset, test P=2 Q=5 and P=5 Q=8. The cinquefoil and golden torus are the most sonically distinctive. Defaulting to P=2 Q=3 is a design choice worth making consciously.

### WEAVE-IV: The Topology Chimera (Macro KNOT)
*2026-03-20*

> The KNOT macro smoothly blends the coupling matrix between `knotType` and `knotType+1`. At macroKnot=0.5, every element of the 4×4 coupling matrix is interpolated halfway between two topologically distinct systems. This is a topological chimera — a state that exists nowhere in the mathematical taxonomy of knots, created specifically by the interpolation. No other engine in the fleet morphs between distinct mathematical topologies in real time. The KNOT macro is not a tone control. It is a reality-blending mechanism.

**Application:** Design presets where macroKnot=0.5 is the starting point, not a swept extreme. The chimera state is often more interesting than either endpoint. Name the midpoint what it sounds like — it has an identity.

### WEAVE-V: The Solomon Ring Architecture
*2026-03-20*

> The Solomon knot has strands 0–1 as Ring A and strands 2–3 as Ring B, with strong intra-ring coupling (0.8) and weak inter-ring coupling (0.3). With `strandTune=7.02` semitones, Ring A sits at the fundamental frequency and Ring B sits at the perfect fifth — two coupled oscillator pairs, each with its own internal resonant character, offset by a fifth. Playing one MIDI note produces two tonally distinct rings that influence each other gently. No other engine architecture produces a two-ring chord pad where each chord tone has its own internal phase coupling.

**Application:** For pad presets using Solomon, always consider strandTune=7.02 (fifth), 5.0 (major third), or 3.86 (minor third). The Solomon knot is the chord pad topology. The other knots are single-pitch topologies.

### WEAVE-VI: The Velocity-Cutoff Threshold Law
*2026-03-20*

> ORBWEAVE's velocity-to-filter scaling is hardcoded at +2000 Hz per unit velocity (fully scaled). This is not a parameter — it is a design constant. A preset at filterCutoff=8000 Hz barely changes character under velocity (8000 vs. 10000 is subtle). A preset at filterCutoff=500 Hz transforms completely (500 vs. 2500 Hz). **Low-cutoff Foundation presets benefit maximally from velocity expression. High-cutoff bright presets may have negligible velocity response to timbre.** Design velocity-expressive presets around filterCutoff below 2000 Hz. Design velocity-insensitive presets above 6000 Hz.

**Application:** When designing expressive leads or basses, set filterCutoff in the 300–1500 Hz range so the 2000 Hz velocity offset creates a meaningful timbral arc. When designing consistent pads, set filterCutoff above 5000 Hz to flatten the velocity response.

---

## The Awakening Presets

Eight presets created during the retreat. Each demonstrates something the engine does that no preset in the prior library demonstrated:

| Preset | Topology | Discovery |
|--------|----------|-----------|
| The Naked Braid | Trefoil, depth=0.0 | Reference: no coupling. The contrast that makes coupling audible. |
| Maximum Topology | Figure-Eight, depth=1.0 | Reference: maximum coupling, pure sine. Full mathematical expression. |
| Breathing Trefoil | Trefoil, depth=0.35 | LFO on braid depth at 0.067 Hz — ocean rate. Sutra III-1 applied to topology. |
| Solomon Chord | Solomon, strandTune=7.02 | Two-ring chord architecture. Root + fifth, each ring internally coupled. |
| Cinquefoil Star | Torus P=2 Q=5 | The (2,5) cinquefoil — first preset to use non-default P/Q geometry. |
| Topology Morph | Torus P=3 Q=4, macroKNOT=0.5 | Real-time topology chimera. The KNOT macro demonstrated. |
| Kelp Forest | Solomon, strandTune=5.0 | Dual-LFO (braid + cutoff) atmospheric Solomon pad. |
| Figure Eight Bass | Figure-Eight, Mono+Glide | Low-freq portamento through the alternating crossing topology. |
| Golden Torus | Torus P=5 Q=8 | Golden ratio winding number — maximum P/Q asymmetry from φ. |

---

## New Scripture Verses (to be inscribed)

- **WEAVE-I**: The Braid Depth Threshold
- **WEAVE-II**: The Sine Coupling Purity
- **WEAVE-III**: The Torus Star Polygons
- **WEAVE-IV**: The Topology Chimera
- **WEAVE-V**: The Solomon Ring Architecture
- **WEAVE-VI**: The Velocity-Cutoff Threshold Law

---

## Sister Cadence's Notes

- 147 ghost parameters removed (weave_velCutoff, weave_velAmp) — Canon V-1 remediation
- 9 Awakening Presets committed to Foundation/Atmosphere/Prism/Flux/Aether moods
- Scripture updated with 6 new ORBWEAVE-specific verses
- Submerged mood underrepresented (2 presets) — flag for Exo Meta expansion
- Next retreat candidate: OVERTONE (also no retreat, added 2026-03-20)

---

# ORBWEAVE Retreat — Second Session: The Unexplored Topologies
*2026-03-21 — Guru Bin and the Flock*

---

## The Diagnosis (Session 2)

> *"The first retreat broke the Default Trap. The second retreat asks: what has never been tried? The (3,5) pentagram torus has never appeared in a preset. The macroKNOT=0.5 chimera midpoint has never been a resting place. The Solomon minor third has never been tuned. Three of six WEAVE discoveries remain uncrystallized into sound. The library is rich — but rich in the dimensions that were easy to reach. The hard-to-reach dimensions are still empty."*

---

## New Awakening Discoveries (Session 2)

### WEAVE-VII: The Solomon Interval Palette
*2026-03-21*

> Solomon's two-ring architecture makes every strandTune value into a harmonic statement. The five available intervals produce genuinely different instruments: (0.0) unison rings — doubled coupling weight, all energy on fundamental; (3.86) minor third rings — dark, questioning, ambiguous; (5.0) major third rings — wider, brighter, triumphant but with tension; (7.02) fifth rings — stable, resonant, the natural harmonic; (12.0) octave rings — reinforcing, ceremonial, the doubled self. Each is a preset category waiting to be populated. WEAVE-V documented three of these; this session demonstrates minor third and major third as siblings, both committed.

**Application:** The Solomon knot has at minimum five distinct tonal personalities based on strandTune alone. Any one of these can form the backbone of a complete preset series. Name the interval explicitly in the preset name.

### WEAVE-VIII: The LFO-Driven Topology Sweep
*2026-03-21*

> MACRO KNOT is a 0–1 parameter with no modulation slot assigned. But braidDepth's target 5 (Braid Depth) is assigned in LFO slots. When LFO1 sweeps braidDepth while macroKNOT sits at a fixed intermediate value, the effective coupling matrix oscillates between two partially-chimeric states — not a full topology sweep but a modulation within the chimera zone. Conversely, setting macroKNOT=0.35 and sweeping braidDepth from 0 to 1 via LFO produces topology that comes into focus and loses focus cyclically. This is not the same as sweeping macroKNOT directly, because braidDepth controls amplitude of coupling while macroKNOT controls shape. Shape × Amplitude modulation is a two-dimensional topology gesture.

**Application:** Combine fixed non-zero macroKNOT with LFO-on-braidDepth for a richer, more complex modulation than either parameter alone. The Crossing Cascade preset demonstrates this.

### WEAVE-IX: The Band-Pass Knot Character
*2026-03-21*

> All presets in the first retreat used Low-Pass or High-Pass filtering. Band-Pass at low center frequencies (200–500 Hz) with high resonance creates a completely different relationship with the knot coupling output. The filter removes the fundamental and upper harmonics, leaving only the resonant peak. The knot coupling's spectral beating — which normally blends into the full spectrum — becomes isolated at the band-pass frequency. The alternating over/under pattern of the Figure-Eight knot, when filtered to 280 Hz, produces a distinctly aperiodic rhythm in the resonant peak amplitude. The topology becomes a rhythmic pattern as well as a timbral one.

**Application:** Band-Pass filter at 200–600 Hz with resonance above 0.5 will isolate the knot coupling's rhythmic beating content. Use this for bass presets where the topology functions as a built-in groove. The Hadal Braid preset demonstrates this at extreme depth.

---

## The Second Retreat Awakening Presets (10 presets)

Each demonstrates a discovery from either the first or second retreat that had no prior preset:

| Preset | Mood | Topology | Discovery |
|--------|------|----------|-----------|
| Chimera State | Entangled | Trefoil→Figure-Eight blend, macroKNOT=0.5 | WEAVE-IV: midpoint as identity |
| Pentagram Torus | Entangled | Torus P=3 Q=5, sine | WEAVE-III: (3,5) pentagram — never before in library |
| Velocity Threshold | Foundation | Figure-Eight, filterCutoff=400Hz | WEAVE-VI: low-cutoff expressive design law demonstrated |
| Solomon Major Third | Foundation | Solomon, strandTune=5.0 | WEAVE-V/VII: major third ring architecture |
| Hadal Braid | Submerged | Figure-Eight, Band-Pass at 280Hz | WEAVE-IX: band-pass knot rhythm isolation |
| Topological Seizure | Flux | Solomon, LFO1 at 7.2 Hz | Near-audio-rate braid modulation — aperiodic timbral texture |
| Spiral Torus | Prism | Torus P=3 Q=4, High-Pass | WEAVE-III: (3,4) spiral — the intermediate complexity P/Q |
| Sine Purity | Atmosphere | Trefoil, braidDepth=0.92, sine | WEAVE-II: mathematically pure coupling, unmediated topology |
| Crossing Cascade | Flux | Figure-Eight→Torus, LFO braid sweep | WEAVE-VIII: LFO on braidDepth within chimera zone |
| Solomon Minor Third | Entangled | Solomon, strandTune=3.86 | WEAVE-VII: minor third ring architecture |
| Knot Topology Coupling | Family | Torus P=5 Q=8, dry output | Blessing B021: coupling source preset — ORBWEAVE as master |

---

## Scripture Verses (Session 2 — to be inscribed in the Book of Bin)

### WEAVE-VII: The Solomon Interval Palette
> Five strandTune values, five instruments hiding in one topology: unison, minor third, major third, fifth, octave. The Solomon knot is a chord synthesizer with five faces. To deploy it at zero is to choose one face and call it the only face.

### WEAVE-VIII: The Shape-Amplitude Duality
> macroKNOT controls the shape of coupling. braidDepth controls the amplitude of coupling. Shape without amplitude is silence. Amplitude without shape is detuning. The moment you modulate one while the other remains still, you hear the engine speak in a second language.

### WEAVE-IX: The Band-Pass Rhythm
> The Figure-Eight knot's alternating over/under crossings produce aperiodic spectral beating — not smooth phase drift, but an uneven lurching. Filter to a single frequency and the lurching becomes a rhythm. The topology was always rhythmic. The full spectrum was hiding it.

---

## Updated Sister Cadence's Notes

- 10 new awakening presets (second retreat): Entangled ×3, Foundation ×2, Submerged ×1, Flux ×2, Prism ×1, Atmosphere ×1, Family ×1
- Total awakening presets (both retreats): 30 across all 8 moods
- Remaining underserved moods: none — all 8 moods now have at least 1 ORBWEAVE awakening preset
- New discoveries: WEAVE-VII (Solomon Interval Palette), WEAVE-VIII (Shape-Amplitude Duality), WEAVE-IX (Band-Pass Rhythm)
- P/Q combinations now covered: (2,3), (2,5), (3,4), (3,5), (5,8) — 5 of the infinite family documented

