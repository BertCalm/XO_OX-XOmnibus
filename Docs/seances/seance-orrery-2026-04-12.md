# Seance Verdict: ORRERY
**Date:** 2026-04-12
**Seance Type:** First seance — no prior record
**Score: 8.2 / 10**
**V1 Status: NOT READY — preset library below minimum threshold**
**Blessing: B047 "The Captured Orbit" awarded**

---

## Architecture
- 4-source Vector Synthesis with 2D bilinear blend pad (NW/NE/SW/SE corners)
- Per-corner oscillators: 7 wave types (Sine, Saw, Square, Tri, Pulse, Noise, Coupled)
- 4 orbital path shapes: Circle, Ellipse, Figure-8, Custom Ephemeris
- Custom Ephemeris: records X/Y trajectory at 30Hz → cubic Catmull-Rom playback as custom orbit
- 3 loop modes: Free, OneShot, Pendulum
- Orbit tilt parameter rotates the trajectory in 2D space
- Libration LFOs (2, floor 0.01Hz) routable to X, Y, Both, OrbitSpeed
- Gravity wells: position displaced by coupling energy or ZCR-based spectral centroid from coupled engine
- CytomicSVF per-voice (Filter Cutoff, Reso, Type, KeyTrack) + Filter ADSR
- Amp ADSR + 4-slot mod matrix (sources: vel/mod wheel/aftertouch/LFO1/2/env/keytrack → destinations: filter cutoff/PosX/PosY/orbit speed/orbit depth/amp)
- 12 max voices (Poly8/Poly12/Mono/Legato)
- Identity: Orrery / Brass Orrery | Accent: Orrery Brass #C8A84B | Prefix: `orry_`

---

## Ghost Panel

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| Moog | 7.5 | "No voltage. But Sine, Saw, Square, Tri in the corners — my vocabulary, deployed as spatial objects in a 2D field. I can find warmth if I choose the right corners." |
| Buchla | 9.0 | "Vector synthesis is the architecture I was building toward. Four sound sources, a 2D space to inhabit — and an orbit to trace through it. The Figure-8 and Custom Ephemeris shapes are not shortcuts: they are new compositional topologies." |
| Smith | 8.0 | "Twelve-voice budget, clean MIDI handling, 4-slot mod matrix with all expected sources. D001 wiring is correct engineering: velocity drives filter envelope depth, not just amplitude." |
| Kakehashi | 7.5 | "KeplerDream. TidalOrbit. GravityShepherd. The names teach the physics. Twenty-one presets is insufficient for an engine of this spatial complexity — a student cannot explore it yet." |
| Ciani | 8.5 | "The orbit traces a continuous path through four distinct timbral zones. The LFO can modulate orbit speed — not just position — which means the orbital breath itself accelerates and contracts. That is spatial composition." |
| Schulze | 9.0 | "An elliptical orbit at 0.125Hz, 3.5-second amp release, sine NW meeting square SE. That is Irrlicht in synthesis form. The slow orbit is not an effect — it is the instrument." |
| Vangelis | 8.0 | "Velocity drives filter envelope depth. A harder note opens the filter more dramatically — in proportion to how much filter envelope you have set. The coupling is natural. The mod matrix lets me route aftertouch to orbit speed for continuous expressive variation." |
| Tomita | 8.0 | "Seven wave types per corner. The Coupled source at corner index 6 routes incoming audio into the blend, making external instruments part of the harmonic palette. Four distinct corners with independent wave and character settings = genuine orchestral range." |

---

## Doctrine Compliance

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 | ✅ PASS | `fltEnvLevel * fltEnvAmt * 10000.0f * v.velocity` — velocity scales filter envelope depth; harder playing opens filter more. `velAmp = v.velocity` scales amplitude. Two timbre axes. |
| D002 | ✅ PASS | 2 LFOs (0.01Hz floor), 4-slot mod matrix with full source set, all 4 macros unconditional and wired. |
| D003 | N/A | Vector synthesis — not physically modeled. |
| D004 | ✅ PASS | All parameters live. LIBRATION and GRAVITY default to 0 in Init (architecturally honest — blank canvas); orbit still runs from Init. Non-zero presets activate both dimensions correctly. |
| D005 | ✅ PASS | LFO floor 0.01Hz. Orbit phase advances per-sample. Custom Ephemeris provides user-defined aperiodic evolution. Orbit speed minimum 0.01Hz (100-second cycle). |
| D006 | ✅ PASS | Mod wheel → mod matrix source (routes to any destination). Aftertouch → resonance direct (+0.3 max) and amplitude (+30%) hardwired, plus mod matrix source for custom routing. |

---

## Preset Assessment

**Count:** 21 presets — all in `Source/Engines/Orrery/Presets/Foundation/`

**V1 minimum:** 50 recommended (orbital engine complexity; Orrery ↔ Oort precedent) | **Gap:** 29 presets

**Init design:** "Conjunction Point" — true blank canvas. Orbit depth 0, LFO depths 0, LIBRATION/GRAVITY macros at 0. Orbit runs when depth is raised. Correct for an engine with this many live dimensions.

**Naming quality:** Outstanding. KeplerDream, LibrationFields, GravityShepherd, TidalOrbit, FigureEight, WorldAxis, StellarNursery, OrbitalDrone, GravityStorm, PendulumGlass, EclipticBass, FractureOrbit, ApogeeHorizon, MemphisDrift. Every name earns its place.

**GravityShepherd note:** This preset (Entangled mood, two Coupled corner sources, gravX=0.8) is a template for the engine's coupling intent. It is the best single preset for demonstrating what Orrery uniquely does.

**Macro effectiveness:**
- ORBIT (M1): 0×→2× orbit depth + speed multiplier — continuous, unconditional
- LIBRATION (M2): 0×→2× LFO depth multiplier — requires non-zero LFO depth in preset
- GRAVITY (M3): 0×→2× gravity well magnitude — requires coupling signal
- SPACE (M4): 0.5×→1.5× filter cutoff — continuous, unconditional

3 of 4 macros unconditional at any non-zero LFO depth setting.

---

## Coupling

**Output:**
- ch0/ch1: Post-filter stereo
- ch2: **Orbit phase 0–1** — continuous signal encoding current position in the trajectory; unique in the fleet; maps one orbital cycle to a full 0..1 ramp usable as a mod source in other engines

**Input types:**
- `AudioToFM` → round-robin corner injection (coupled audio enters corner synthesis directly as a waveform source, wave index 6 = "Coupled")
- `AmpToFilter` → filter cutoff
- `EnvToMorph` → X position push (couplingEnvToMorphX, decays per-sample)
- `RhythmToBlend` → Y position push (couplingRhythmToY, decays per-sample)

**Gravity well integration:** coupling energy (RMS from coupled engine) smoothly displaces the orbit position via gravity well coefficients. The coupled engine exerts physical gravitational influence on Orrery's trajectory.

**Natural partners:** Osprey (tidal/wave amplitude → gravity well), Onset (percussive rhythm → Y position burst), Orrery ↔ Orrery (two orbital bodies with independent trajectories modulating each other's gravity wells).

---

## Blessing

**B047: "The Captured Orbit"**

Custom Ephemeris mode records the performer's X/Y vector pad gestures at 30Hz, stores them as a position buffer, and plays them back with cubic Catmull-Rom interpolation as a continuously repeating orbital path. The orbit is not algorithmic — it is autobiographical. No prior fleet engine captures performance gesture as a primary synthesis trajectory. When you record your hands on the pad, the engine performs that gesture forever, smoothly, at any speed you choose. The orbit becomes the composition.

---

## Recommendations

### P0 — None
No DSP P0 bugs. All doctrines pass.

### P1 — Important
1. **Expand presets to ≥50 before V1** — 21 is thin for an engine with orbital path shapes, gravity wells, and four corner waveforms to explore. Target: 10 Foundation (varied orbit shapes), 10 Entangled (GravityShepherd-class coupling presets), 10 Ethereal (slow KeplerDream-class pads), 10 Kinetic (faster orbits / Figure-8 / FractureOrbit-class), 10 mixed.
2. **Wire mod wheel → orbit speed by default** in at least one preset-layer example. Currently requires user mod matrix setup. The most natural expressive control for live performance.

### P2 — Nice to have
3. **Orrery ↔ Osprey Entangled preset** — ocean amplitude → gravity well driving orbital position. Named "Tidal Pull" or "Perigee."
4. **Distribute presets to mood folders** — KeplerDream → Ethereal, GravityShepherd → Entangled, EclipticBass → Foundation/deep.
5. **Tempo-sync LFOs when orbitSync is active** — coherent rhythmic patches require LFOs and orbit to share tempo reference.

---

## Path to V1

**Required:** Preset library ≥50 across multiple mood folders.
**Estimated score at that point:** 8.5–8.7. No architectural weaknesses to fix — only library depth.

---

*First seance — 2026-04-12 | Ringleader RAC Session | B047 awarded*
