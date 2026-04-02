# XOxide — Competitive Analysis & Design Refinement
**Author**: Vibe (Sound Design Android, XO_OX)
**Date**: 2026-03-16
**Status**: Pre-build reference document

---

## 1. The Landscape Problem

Every studio tool that shapes character — whether it's a Neve preamp, a Decapitator plugin, or a hardware tube stage — asks the producer to think in ingredients: "how much of this specific flavour?" You pick a mode, you turn a drive knob, you add or subtract something nameable. The mental model is additive chemistry: choose your reagents, combine them, done.

This is the wrong model for how timbre actually works.

Character is a position in space, not a recipe. When a sound feels warm and deep and rounded, that's not because you dialled in "tube saturation + LA-2A opto + sub enhancement" as three independent decisions. Those dimensions cohere — they're facets of a single character philosophy. The clinical, precise sound is equally holistic: it's the attack, the odd harmonics, the stereo width, and the FET behaviour all pulling in the same direction simultaneously.

XOxide is the first tool that treats character as spatial navigation rather than ingredient selection.

That is the thesis. Everything else in this document is evidence for it.

---

## 2. Akai AIR Flavor Pro — Direct Competitor Analysis

### What Flavor Pro Does

Akai bundled AIR Flavor Pro as a character shaper with MPC 3.0 — it sits in the effects chain alongside AIR's reverbs and compressors. Its processing model is a classic "flavour mode" selector: you pick a character preset and adjust a single Flavor intensity knob plus some supplementary controls.

From AIR's implementation, Flavor Pro operates on these axes:
- **Harmonic saturation** — tube-style even harmonics, transistor-style odd harmonics, tape-style combined harmonics with HF soft-knee rolloff
- **Transient punch** — single attack enhancement control, not a full dual-envelope transient designer
- **Top-end air** — high-shelf harmonic generation, similar to the Aural Exciter approach
- **Lo-fi colour modes** — bit reduction, tape wow/flutter, vinyl crackle for aesthetic degradation

The interface presents discrete "Flavor" categories (Warm, Punch, Vintage, Air, Lo-Fi, Modern) with a single intensity control and a few secondary parameters per category. You choose a mode, set intensity, done.

### How Flavor Pro Integrates with MPC Workflow

Flavor Pro works as a standard insert effect on any MPC track, pad, or bus. In the MPC 3.0 workflow:
- Per-pad inserts: apply different flavours to individual drums
- Program-level: colour an entire instrument's output
- Mixdown: bus-level character shaping
- Automation: intensity parameter can be automated in MPC's sequencer

This is a well-executed single-insert character tool. It solves a real problem for producers who want a one-click vintage or modern quality without deep technical knowledge.

### What XOxide Does That Flavor Pro Cannot

**Continuous 2D character space vs. discrete mode switching.** Flavor Pro's modes are islands. XOxide's plane is a continent. The difference: a Flavor Pro producer going from Warm to Punch snaps between two named configurations. An XOxide producer at X=+0.3, Y=0.4 can glide to X=-0.2, Y=0.6 and every point on that path is a musically coherent character. The intermediate positions are not undefined — they are explicitly mapped, tested, and intentional.

**Coordinated multi-stage processing.** Flavor Pro applies one character type per mode. XOxide's 6-stage chain (Transient → Saturation → Compression → Sub → Stereo → Exciter) fires simultaneously, coordinated by the same XY position. You can't get "attack forward + FET limiting + crystalline odd harmonics + Haas width" as a single Flavor Pro mode. XOxide does that at feliX/Depth automatically.

**Lorenz chaos modulation of character position.** Flavor Pro has no temporal evolution engine. XOxide's LFO can drive the XY position itself through organic Lorenz attractor paths — the character itself mutates over time in ways no simple LFO could produce. This is not automation; it's intrinsic motion.

**XOceanus coupling architecture.** Flavor Pro is an isolated insert. XOxide emits its post-compression gain reduction value as a coupling signal, emits detected transient envelopes as `EnvToMorph` for driving other engines (e.g., ODYSSEY morph), and accepts `AmpToFilter` sidechaining to auto-duck `oxide_y`. Flavor Pro has none of this. XOxide is a node in the XOceanus network, not a standalone island.

**Sub enhancement with pitch tracking.** Flavor Pro has no sub synthesis capability. XOxide's Stage 4 adds an octave-down sine tracking the input fundamental — this is psychoacoustic bass enhancement derived from Waves MaxxBass methodology, not just low-shelf EQ. Flavor Pro cannot extend the sub register of a thin pad.

**Per-stage manual override.** Each of XOxide's 6 stages can be pulled out of XY-driven mode and set manually. Flavor Pro's intensity knob scales everything together. The XOxide power user can run: "let the XY plane drive saturation and compression, but I want manual control over the transient shaper and stereo width." Flavor Pro offers no equivalent granularity.

### What Flavor Pro Does That XOxide Should Match or Exceed

**One-touch workflow.** Flavor Pro's greatest strength is zero-friction. You pick Warm, set intensity to 60%, done, it works, it sounds good. XOxide's primary interface must deliver the same experience: the XY pad should feel like one gesture. Do not bury the simplicity under complexity. The stage chain strip is a power-user layer, not the primary interaction.

**MPC integration depth.** Flavor Pro was designed for MPC from day one — pad-per-insert workflow, intensity automation, tight UI fit. XOxide should be equally fluent in MPC context. The MIDI CC assignments (mod wheel → `oxide_y`, expression → `oxide_dryWet`) need to feel designed-for-MPC, not bolted on.

**Named character anchors.** Flavor Pro's mode names (Warm, Punch, Vintage) are wayfinding tools — they tell a producer where to start. XOxide's 20 factory presets serve this function, but the XY pad should also show named anchor labels at the four corners and key positions. "IRON FIST" at feliX/Depth. "MAGMA CORE" at Oscar/Depth. "STEEL SILK" at feliX/Surface. "AMBER MORNING" at Oscar/Surface. These become the vocabulary.

**CPU efficiency.** MPC performance context demands light CPU usage. XOxide's 6-stage chain should be designed for low-CPU at Surface depths — at Y=0 (Surface), most stages should reduce to near-passthrough with trivial computation. See Design Recommendation 4 below.

---

## 3. The Character Shaper Landscape

### Soundtoys Radiator
**What it is**: Single-ended tube amp modelling, transformer saturation simulation, two "iron" flavours (Neve and trident character). Parallel wet/dry blend, input drive, output trim.

**Its paradigm**: Radiator is a hardware emulation of a specific physical object. It is excellent at what it emulates. What it cannot do: be anything other than an approximation of a tube amplifier. The sonic palette is narrow by design — that is the product.

**XOxide difference**: Radiator's "Neve flavour vs. trident flavour" is Radiator's version of feliX vs. Oscar — but without the second axis (depth), without the transient shaper, without compression, without sub enhancement, without stereo field, without continuous spatial navigation. Radiator is one point in XOxide's plane.

### Black Box Analog Design HG-2
**What it is**: Hardware unit, three-stage saturation — triode, pentode, and Air Band harmonic generation. Each stage has independent drive. Parallel mix. The HG-2 is the gold standard for high-end "colour and push" studio hardware.

**Its paradigm**: The HG-2's triode/pentode split is the hardware equivalent of XOxide's tube/digital blend in Stage 2. Triode = even harmonics (Oscar lean), pentode = odd harmonics (feliX lean). The Air Band is Stage 6 (exciter) in hardware form. The HG-2 achieves, in hardware, three of XOxide's six stages.

**XOxide difference**: The HG-2 has no transient shaper, no compression, no sub enhancement, no stereo field, no temporal evolution (LFO), and no coupling. It also costs $2,500. XOxide's Stage 2 saturation model directly honours the HG-2's triode/pentode architecture — the blend bus (digital → tape → tube) maps to pentode → program-dependent → triode respectively. This is deliberate homage.

**Design recommendation**: In Stage 2's tube model, implement the asymmetric bias term with triode vs. pentode character variants accessible via the `oxide_satBlend` override. When `oxide_satBlend` is overridden (not XY-driven), let the user sweep between pentode-style odd harmonics at -1.0 and triode-style even harmonics at +1.0, with the HG-2's specific harmonic signature as reference. This gives producers who know the HG-2 a vocabulary they recognise.

### Neve 542 Tape Saturation
**What it is**: 500-series discrete Class A transformer saturation module. Single tape type (Studer A820 reference). Record/Playback drive controls, headroom trim.

**Its paradigm**: One flavour, maximum authenticity. The 542 is a tape machine front-end — it has the hysteresis character, the HF soft saturation, the transformer slam. It is the reference for XOxide's center-X tape model in Stage 2.

**XOxide difference**: The 542 offers one fixed point: approximately X=0.0 (tape) at whatever depth you dial in with the drive controls. No transients, no compression, no stereo, no sub, no exciter. XOxide's tape centre-position in Stage 2 is the 542's territory — acknowledge that by making the tape model sound like tape machine input stages.

### Kush Audio Omega 458A
**What it is**: Three saturation flavours (A = thick/vintage, B = clear/modern, C = dirty/aggressive) with a single Harmonic control blending in the character. No compression, no transient control, no stereo.

**Its paradigm**: The 458A is the closest existing tool to XOxide's saturation stage in isolation. Kush's A/B/C flavours roughly correspond to XOxide's Oscar/center/feliX positions along the X axis. The Harmonic control is XOxide's Y axis for saturation only.

**XOxide difference**: XOxide extends the 458A's paradigm across five additional stages and into a 2D continuous plane. The 458A confirms the paradigm is right — producers buy it specifically because continuous character blend between named flavours is more useful than discrete mode switching.

**Design recommendation**: Study the 458A's Harmonic control response curve. The sweet spot reportedly sits around 40-60% where harmonic content is musically useful without muddiness. XOxide's Y axis saturation mapping should have a similar "golden zone" at Y=0.4-0.6 where character is rich but not overwhelming. The extreme depths (Y=0.8+) should be available but clearly in "transformation territory."

### Soundtoys Decapitator
**What it is**: Five saturation modes named A (Ampex tape), E (EMI transformer), N (Neve 1057 input), T (Thermionic Culture valve amp), S (Schulte Compact Phasing 'A' circuit). Drive, Tone, Punish (extreme limiting with saturation).

**Its paradigm**: The most complete "flavour selection" competitor. Five discrete modes with excellent analogue modelling. The "Punish" button is Decapitator's version of feliX/Depth — maximum aggression. Extremely popular precisely because the modes are well-chosen and well-executed.

**Its weakness**: Still five discrete islands. Between A and E there is no intermediate position. The character navigator in the producer's head must do the interpolation — and they can't actually hear the in-between. The Tone knob helps, but it's a corrective filter, not a character blend. Punish is a on/off switch, not a continuous depth axis.

**XOxide difference**: XOxide's saturation blend bus (digital → tape → tube) covers roughly Decapitator's A/N/T flavours on a continuous axis. The continuous nature is the differentiator. There is no equivalent to "halfway between A and N" in Decapitator. XOxide can live there.

**Design recommendation**: XOxide should not try to emulate Decapitator's specific hardware models — those are licensed/emulated DSP assets Soundtoys spent years perfecting. XOxide's tube/tape/digital blend should have its own character signature derived from DSP principles, not hardware emulation. The point is the spatial paradigm, not the specific harmonic signature.

---

## 4. Design Recommendations

### Recommendation 1: Add a "Character Snapshot" Feature
**What it is**: A small bookmark strip below the XY pad that holds up to 4 saved XY positions. Click a bookmark to glide (100ms interpolated transition) to that position. Click + hold to save current position.

**Why**: Flavor Pro's mode buttons serve this function — they are named character anchors. The producer who finds their sweet spot at X=+0.4, Y=0.55 needs a way to return to it instantly without remembering coordinates. Four bookmarks cover: verse character, chorus character, bridge character, and one experimental position.

**Implementation**: Four `oxide_snap{1-4}_x` and `oxide_snap{1-4}_y` parameters (8 additional params). Glide time: 100ms linear interpolation — enough to feel intentional but not slow. Add to Parameter List as "Snapshot Positions" group.

### Recommendation 2: Clarify the Surface-to-Depth Gateway
**Current spec**: Y=0 is Surface (nearly transparent), Y=1 is Depth (full transformation). This is correct.

**Problem**: Producers who want "just a little warmth" often dial saturation to 100% but get surprised by how much happens at full depth. The "gateway" between Surface and Depth should be perceptually graded.

**Recommendation**: Revise the Y→saturation drive mapping to use a logarithmic curve (not linear) for Y in the range 0.0-0.5, and linear from 0.5-1.0. This means the Surface-to-midpoint transition is very gentle (most useful character enhancement lives here), and the midpoint-to-Depth transition is where the dramatic transformation happens. The current linear mapping compresses too much "good stuff" into the lower Y range.

**Specific curve**: `effective_y = y < 0.5 ? (y * y * 2) : (y * 0.5 + 0.25) * 2 - 0.5`. Test against linear at Y=0.2 — logarithmic should give noticeably less saturation (more appropriate for "gentle enhancement" zone).

### Recommendation 3: Exciter Frequency as X-Linked Default, Not Just Y-Driven
**Current spec**: `oxide_exciterFreq` ranges from 12kHz at feliX/Surface to 4kHz at Oscar/Depth. The Y axis lowers frequency as depth increases.

**Problem**: At Oscar/Surface (X=+1, Y=0), the exciter frequency is still at 8kHz but the exciter mix is near zero (Y=0). This means the frequency range doesn't have practical impact at Surface — the stage is nearly bypassed there anyway.

**Recommendation**: Let X position also shift the exciter frequency independently of Y. feliX should bias the crossover frequency upward (more air, less midrange texture), Oscar should bias it downward (more midrange warmth, less pure air). Proposed: `exciter_base_freq = lerp(10000, 5000, x_n)`, then multiply by `lerp(1.2, 0.6, y)` for the depth scaling. This means the frequency is always appropriate for the character position, even at Surface depths.

### Recommendation 4: CPU Optimization at Surface Depths
**Proposal**: At Y < 0.05, bypass all stages algorithmically even if the bypass toggles are off. Add a `shouldProcessStage(stageIndex, y)` check in `renderBlock()` that returns false when Y is too low for a stage to have audible effect.

**Stage-specific thresholds**:
- Transient: active if `y > 0.02`
- Saturation: active if `y > 0.05`
- Compression: active if `y > 0.02` (even light compression is audible)
- Sub: active if `x_n > 0.3 && y > 0.3` (already specified)
- Stereo: active if `y > 0.05`
- Exciter: active if `y > 0.05`

At Y=0 with all stages at Surface, XOxide should consume near-zero CPU — it becomes a passthrough. This is critical for MPC context where 4 slots may be running simultaneously.

### Recommendation 5: "Character Vision" Overlay Mode
**Proposal**: A UI mode (toggle button) that overlays the XY pad with a heat-map visualization showing what each region "does" to the current input signal. When active, the pad shows a real-time spectral fingerprint of the character change: warm amber regions glow where tube saturation adds even harmonics, cool blue regions shimmer where crystalline odd exciter content appears, red contours mark where aggressive transient/FET processing lives.

**Why this matters**: Flavor Pro's discrete modes have labels. XOxide's continuous space needs a vocabulary for users who don't yet know it. The heat-map teaches the space experientially. After 10 sessions, the producer no longer needs the overlay — they've internalized the map.

**Implementation note**: This is a UI feature, not DSP. Render a pre-computed 64×64 texture per preset that encodes the character signature at each grid point. Update texture on preset change, not in real time. The texture is a static artistic encoding of the XY space, not a live DSP analysis.

---

## 5. The "Nobody Does This" Pitch

**One sentence**: XOxide treats timbre as a position in space, not a recipe — drag one cursor and all six character stages (transient, saturation, compression, sub, stereo, and air) move together, coherently, into whatever sonic philosophy that point represents, while a Lorenz chaos attractor traces organic paths through the plane so your sound breathes like something alive.

**The compressed version for a sell sheet**: *The first character processor where the middle of the dial is as intentional as the extremes.*

---

## 6. Twenty Preset Names Spanning the XY Character Space

### Four Corners

| Position | Name | Character |
|----------|------|-----------|
| feliX / Surface (X=-1, Y=0) | **Crystal Veil** | Nearly transparent, trace odd-harmonic shimmer, no perceptible transformation — the finest possible definition edge |
| feliX / Depth (X=-1, Y=1) | **Iron Fist** | Maximum transient violence, hard digital fold-back, FET brick-wall, crystalline odd harmonics from 3kHz — pistol shrimp snap |
| Oscar / Surface (X=+1, Y=0) | **Candlelight** | Nearly transparent, trace tube second harmonic warmth, a gentle rounding at the very top — felt more than heard |
| Oscar / Depth (X=+1, Y=1) | **Magma Core** | Attack suppressed, sustain fully swollen, heavy tube drive, deep opto levelling, octave-down sub, focused stereo — the deep ocean floor |

### Four Mid-Points on Each Axis

**X axis (Y=0.5, varying X):**

| Position | Name | Character |
|----------|------|-----------|
| X=-0.75, Y=0.5 | **Bright Edge** | Strong transient attack, digital crunch, FET compression mid-ratio, crystalline air — MPC drum bus sharpener |
| X=-0.25, Y=0.5 | **Tape Forward** | Slight attack boost into tape saturation, program-dependent compression — the mix bus standard |
| X=+0.25, Y=0.5 | **Warm Forward** | Sustain slightly lifted, tube tape blend, opto compression at 2:1, warm air at 7kHz |
| X=+0.75, Y=0.5 | **Amber Slow** | Sustain forward, deep opto, tube saturation at moderate drive, sub enhancement begins |

**Y axis (X=0, varying Y):**

| Position | Name | Character |
|----------|------|-----------|
| X=0, Y=0.25 | **Still Air** | Centre plane, light depth — tape saturation at trace level, transparent compression — mastering polish |
| X=0, Y=0.5 | **Oxidize** | Centre plane, moderate depth — tape character fully engaged, 2.5:1 compression, mid-frequency exciter active |
| X=0, Y=0.75 | **Heavy Tape** | Centre plane at deep depth — full tape hysteresis, strong compression, wide stereo, harmonic generation extending into midrange |
| X=0, Y=1.0 | **Full Oxide** | Maximum depth at neutral X — all six stages at maximum centre-position settings — lo-fi tape destruction |

### Eight Character Sweet Spots

| Position | Name | Character |
|----------|------|-----------|
| X=-0.6, Y=0.35 | **Steel Silk** | Slight attack boost, digital grit trace, FET compression light, crystalline air at 9kHz — definition without harshness |
| X=+0.6, Y=0.4 | **Amber Morning** | Vintage warmth — tube saturation moderate, opto at 1.8:1, warm even harmonics at 7kHz, trace sub |
| X=-0.8, Y=0.7 | **Crystal Storm** | Strong feliX processing — transient attack hard, digital saturation, FET at 4:1, Haas width, odd harmonics from 5kHz |
| X=+0.8, Y=0.65 | **Deep Swell** | Strong Oscar — sustain boosted, tube at 0.6 drive, opto 3:1, sub octave at 0.2 mix, slight stereo narrowing |
| X=-0.3, Y=0.6 | **Tape Punch** | Tape-forward with FET lean — transient boost + tape saturation + moderate FET compression — ideal drum bus |
| X=+0.4, Y=0.7 | **Velvet Compress** | Opto levelling dominant — opto 3:1 slow attack, tube saturation, sustain forward, warm air — vocal character |
| X=-0.9, Y=0.85 | **Cavitation** | Near-maximum feliX depth — the pistol shrimp approach without full corner lock — aggressive, punchy, alive |
| X=+0.5, Y=0.3 | **Warm Glue** | Producer's mix bus — mild Oscar lean, low depth, opto 1.5:1, tape trace, sub silent — transparent cohesion |

---

## 7. Macro Sweet Spots

### CHARACTER (M1 = `oxide_x`) Positions

| M1 Value | Position | Character Name | What It Does |
|----------|----------|---------------|-------------|
| 0.0 (center) | X=-1.0 | Full feliX | Maximum transient attack, digital saturation, FET compression, Haas width, odd harmonics |
| 0.25 | X=-0.5 | feliX Lean | Attack-forward, tape-digital blend, FET-leaning compression, wide stereo |
| 0.5 (center) | X=0.0 | Pure Center | Tape saturation, balanced transient, program-dependent compression, unity stereo |
| 0.75 | X=+0.5 | Oscar Lean | Sustain-forward, tape-tube blend, opto-leaning compression |
| 1.0 (max) | X=+1.0 | Full Oscar | Maximum sustain, tube saturation, opto compression, sub enhancement, focused stereo |

**Sweet spot for producers**: M1=0.35 (X=-0.3) — feliX lean without digital harshness. The classic "punchy and defined" mix bus position. Tape saturation with attack emphasis. Set COUPLING (M2) to 0.3 and this is a professional mix bus enhancer.

**Sweet spot for sound design**: M1=0.7 (X=+0.4) with M2=0.65 — deep Oscar warmth. The pad thickening position. Every pad through this sounds like it was recorded through a vintage SSL bus with a nice API pre.

### MOVEMENT (M3 = `oxide_lfoDepth` + `oxide_lfoRate`) Values

| M3 Value | lfoDepth | lfoRate | Character |
|----------|----------|---------|-----------|
| 0.0 | 0.0 | 0.25 Hz | Static. No evolution. |
| 0.2 | 0.15 | 0.2 Hz | Barely perceptible breathing — the character lives and dies slowly |
| 0.4 | 0.35 | 0.35 Hz | Analogue drift — like a vintage unit warming up over 30 seconds |
| 0.6 | 0.55 | 0.6 Hz | Audible modulation — pads breathe, character cycles |
| 0.8 | 0.75 | 1.2 Hz | Rhythmic character oscillation — syncs to approximately 1 bar at 72 BPM |
| 1.0 | 1.0 | 4.0 Hz | Full sweep tremolo-speed — character oscillation becomes a performance effect |

**Sweet spot for ambient/pad work**: M3=0.25 — the sound breathes but you can't consciously track the cycle. Set LFO Shape to Chaos for maximum organicism. The character feels alive without sounding like an effect.

**Sweet spot for rhythmic production**: M3=0.55 with LFO Shape to Triangle — audible X-axis sweep at a tempo-relevant rate. CHARACTER (M1) locked at center, MOVEMENT at 0.55 → the sound alternates between feliX punch and Oscar warmth on every two beats at 128 BPM.

---

## 8. MPC Integration Angle

### How XOxide Differs from Flavor Pro in the MPC Workflow

**Insert position**: Flavor Pro is designed as a per-track insert — you use it on one pad or one program at a time. XOxide's `AudioToBuffer` coupling means it can sit in a parallel XOceanus slot and process the mixed output of multiple engines simultaneously. In practice: put ONSET (drums) in Slot 1, OVERDUB (bass dub) in Slot 2, XOxide in Slot 3 coupled to both — XOxide colours the sub-mix of an entire section, not just a single sound.

This is not a workflow Flavor Pro can support. Flavor Pro is always a point insert. XOxide can be a section processor within the XOceanus architecture.

**Live performance use**: The XY pad is a physical performance controller. MIDI CC mod wheel → `oxide_y` means a producer performing live can push the depth pedal and drive the entire section from transparent polish to heavy transformation in real time. Flavor Pro's intensity knob can be automated but it doesn't have the same gestural quality — you're adjusting a single parameter, not navigating a character philosophy.

**COUPLING macro as XY depth control**: M2 (COUPLING) maps directly to `oxide_y`. Assigning M2 to the MPC's Q-Link gives the producer a physical fader controlling "how deep is the transformation right now." This is session-ending powerful in a live context: fade the Q-Link up to transform, down to restore. No A/B switching, no mode changes, no menu diving.

### Post-Export Baking: XOxide Character in XPN Samples

XOxide's processing chain can be approximated in rendered XPN samples for producers who want the character baked into their sample packs rather than requiring real-time processing.

**Stages that can be baked** (well-suited to offline render):
- Saturation (Stage 2): Full character — tube, tape, digital blend at any XY position renders identically offline
- Sub Enhancement (Stage 4): Octave-down synthesis bakes cleanly — the tracked fundamental at render time is the same as playback
- Exciter/Air (Stage 6): Harmonic generation bakes with full fidelity
- Transient Shaper (Stage 1): Attack/sustain shaping bakes perfectly in rendered audio

**Stages that cannot be fully baked** (dynamic content):
- Compression (Stage 3): The compressor's response depends on the playback dynamics, not just the source signal. A baked compressed sample will respond differently when velocity-varied during MPC playback vs. having XOxide compress each hit in real time.
- Stereo Field + Haas (Stage 5): Haas micro-delay bakes fine, but M/S width processing interacts with the playback context. Baked width is static; real-time width responds to the source's stereo content dynamically.

**Recommendation for Oxport integration**: When an XOxide preset is selected in an XOceanus export session, the Oxport pipeline should:
1. Bake Stages 1, 2, 4, 6 into the rendered samples (character is preserved at zero CPU cost during MPC playback)
2. Include a Flavor Pro preset recommendation for the compression character (Stage 3) as a separate note in the XPN bundle metadata — producers can approximate the dynamic processing character with MPC's native Flavor Pro at playback time
3. Flag whether Stage 5 stereo width was significant; if `oxide_stereoWidth > 1.3`, recommend the producer apply width in the MPC mixer rather than baking (baked width can be problematic in multi-layer velocity stacks)

**XPN velocity curve mapping to XOxide XY position**: The Sonic DNA velocity curves used by the Kai/Vibe team map naturally onto XOxide's character space:
- Vibe's musical velocity curve (S-curve, soft at low velocities, exponential above 70%) → maps to Y axis: low velocities stay near Surface, high velocities push toward Depth. Implement as: `effective_y = oxide_y * velocity_normalized^1.4` for dynamic character deepening.
- feliX velocity response (bright, attack-forward at high velocities): at feliX X positions, the Vibe curve creates more transient attack boost at high velocities — already musically correct since loud hits should feel more percussive and sharp.
- Oscar velocity response (warm, sustained at low velocities): at Oscar X positions, the curve creates more sustained/warm character at low velocities — correct since quiet, intimate playing benefits from warmth.

This suggests a potential `oxide_velocitySensitivity` parameter (future V1.1 addition): a single knob that scales how much velocity modulates `oxide_y`. At 0.0: velocity has no character effect. At 1.0: velocity fully determines depth (Y) regardless of the `oxide_y` static setting. This would make XOxide the first character processor where your playing dynamics change not just volume but timbre character holistically.

---

## 9. Lorenz Chaos LFO Section

### Why Lorenz Chaos Beats a Standard LFO for Character Modulation

A standard LFO is periodic. It returns to the same position after every cycle. A character processor driven by a periodic LFO will produce a predictable groove — every two bars, the sound returns to identical character. This is fine for intentional tremolo effects. For organic character evolution, it is fatally mechanical.

The Lorenz attractor is a deterministic dynamical system that never repeats. Given the same initial conditions, it traces the same path — but the path has a period of effectively infinity at any musically useful rate. The character evolution driven by a Lorenz attractor sounds like organic drift: it moves in one direction, reverses, wanders, doubles back, all within bounded territory.

Specifically, for XOxide:
- **Bounded**: The Lorenz attractor's trajectory stays within a bounded region of state space. When projected onto XOxide's XY plane, the character position stays within the plane — it does not escape to saturated extremes unexpectedly.
- **Ergodic**: Over long time periods, the attractor visits all regions of the bounded territory. The character will eventually visit every part of the XY plane allowed by the `oxide_lfoDepth` scale. A sine LFO only ever visits the same arc.
- **Self-similar but non-periodic**: The characteristic "butterfly" shape of the Lorenz attractor means the trajectory returns to the vicinity of previous positions but never exactly repeats. The producer's ear interprets this as organic, non-mechanical evolution.
- **Sensitive to initial conditions**: Preset starting positions (`oxide_x`, `oxide_y`) become the initial conditions for the Lorenz integration. Two presets that start at slightly different XY positions will diverge into completely different Lorenz paths. Every preset has a unique Lorenz signature.

**The musical implication**: A pad processed through XOxide at Chaos LFO mode with moderate depth will sound like it was played through a vintage hardware chain where the tube is warming up, the tape transport is slightly irregular, and the compressor is reacting to the room. It sounds alive because it is alive — the character is non-stationary in the same way a live analogue chain is non-stationary.

No standard plugin LFO can produce this. S&H comes close on fast rates, but S&H jumps — Lorenz glides. The Lorenz trajectory in XY space is smooth and continuous; the derivative (rate of change) is also bounded. Character transitions feel like natural evolution, not switching.

### The Three Lorenz Parameters and Their Musical Meanings

The Lorenz system is defined by three differential equations:

```
dx/dt = σ(y - x)
dy/dt = x(ρ - z) - y
dz/dt = xy - βz
```

The three parameters control the attractor's shape and behaviour:

**σ (Sigma) — Correlation Rate**
- Default musical value: 10.0
- **Physical meaning**: How quickly x (temperature difference in the original convection model) responds to y (fluid velocity). High σ means x chases y aggressively — the system is highly coupled.
- **Musical meaning**: Sigma controls the "responsiveness" of the character evolution. Low sigma (3-6): the character moves slowly and makes wide, lazy sweeps. High sigma (15-20): the character responds quickly to changes in trajectory, producing tighter, more nervous movement.
- **Subtle organic movement**: σ=8.0 — character moves at a moderate, relaxed pace
- **Unpredictable chaos**: σ=14.0 — character becomes more twitchy and reactive, sweeping quickly and changing direction more often

**ρ (Rho) — Chaos Depth**
- Default musical value: 28.0
- **Physical meaning**: The Rayleigh number — ratio of buoyancy to viscosity. Below ρ=24.74, the Lorenz system converges to a fixed point (not chaotic). At ρ=28, the classic butterfly attractor emerges.
- **Musical meaning**: Rho controls how far the attractor strays from its center. Low rho (near 24.74): character evolution stays close to the initial XY position — barely any movement. High rho (35-50): the attractor becomes more "spread out," the butterfly wings expand, character evolution covers more of the XY plane.
- **Subtle organic movement**: ρ=26.5 — attractor exists but is relatively compact. Character wanders but stays near its home position.
- **Unpredictable chaos**: ρ=38.0 — fully expanded butterfly attractor. Character visits wide regions of the XY plane on each orbit.

**β (Beta) — Dissipation Rate**
- Default musical value: 8/3 (≈2.667)
- **Physical meaning**: Geometric factor of the physical system. Rarely changed in musical contexts because it primarily affects how fast the trajectory "decays" toward the attractor versus "escaping" it.
- **Musical meaning**: Beta controls the "density" of the trajectory. At the canonical 8/3 value, the trajectory is well-distributed across the attractor surface. Higher beta (4-6): the trajectory spends more time in the "wings" of the butterfly, producing longer sustained character positions before switching lobes. Lower beta (1.5-2.0): the trajectory transitions between lobes more frequently.
- **Subtle organic movement**: β=8/3 (default) — standard. The canonical Lorenz butterfly is already well-suited for musical use.
- **Unpredictable chaos**: β=1.8 — faster lobe transitions. The character switches between feliX-leaning and Oscar-leaning territory more often.

### Recommended Default Values

**"Subtle Organic Movement" preset**: σ=8.0, ρ=26.5, β=8/3
- Rate: 0.08 Hz (12-second period)
- Depth: 0.2
- X-amount: 0.5, Y-amount: 0.3
- Effect: the character breathes imperceptibly. Most producers will not consciously hear the evolution but will feel the sound is "alive." Pads, drones, sustained textures.

**"Analogue Warmth Drift" preset**: σ=10.0, ρ=28.0, β=8/3
- Rate: 0.03 Hz (33-second period)
- Depth: 0.35
- X-amount: 0.6, Y-amount: 0.4
- Effect: slow, wide sweeps through the Oscar-warm territory. Suitable for long ambient compositions where the character needs to evolve imperceptibly over 5+ minutes.

**"Unpredictable Chaos" preset**: σ=14.0, ρ=38.0, β=1.8
- Rate: 0.4 Hz
- Depth: 0.75
- X-amount: 0.8, Y-amount: 0.7
- Effect: aggressive, unpredictable character evolution. The sound never settles. Processing territory ranges from crystalline to warm to crushing and back. Experimental, generative, performance context.

**Implementation note**: The Lorenz system must be integrated with a fixed-step RK4 (Runge-Kutta 4th order) or Euler method at audio control rate (every 64 samples at 48kHz = ~750 Hz update rate). The continuous attractor output must be projected onto the XY plane by normalising the x and y components of the attractor's current state to [-1, +1] range (using the known attractor bounds for the given σ, ρ, β values). Do not use z — it occupies the third dimension and does not project well onto the XY plane without additional mapping. Map attractor_x → `oxide_lfoXAmount` displacement, attractor_y → `oxide_lfoYAmount` displacement, both scaled by `oxide_lfoDepth`.

---

## 10. Summary: XOxide's Competitive Position

XOxide does not compete with Flavor Pro on Flavor Pro's terms. Flavor Pro is a one-click character selector for producers who want named presets applied quickly. That is a valid product. It will continue to exist and be useful.

XOxide competes on a fundamentally different dimension: it is the first tool that models character as a spatial philosophy rather than a recipe. The 2D plane is not a gimmick — it is the correct mental model for how timbre works, and XOxide is the first processor to operationalize that model fully.

The five tools analysed above (Flavor Pro, Radiator, HG-2, Neve 542, Decapitator) collectively cover pieces of what XOxide does as a unified instrument. Radiator is XOxide's tube saturation. The HG-2's triode/pentode split is XOxide's X-axis. The Neve 542 is XOxide's tape center position. Decapitator's flavour modes are five discrete points on XOxide's X-axis. Flavor Pro's character modes are an island map of what XOxide turns into a navigable continent.

None of them have the second axis. None of them coordinate six stages simultaneously. None of them use a Lorenz attractor to make the character itself breathe.

**The product pitch, one more time, for the label**:

*XOxide is the character processor where everything happens at once — drag one cursor and the transient, the saturation, the compression, the sub, the stereo field, and the air all move together into a single sonic philosophy. Every point on the plane is intentional. Every path between points is musical. The attractor makes it breathe.*

That is what nobody else does.

---

*Document prepared by Vibe, Sound Design Android, XO_OX Designs — 2026-03-16*
