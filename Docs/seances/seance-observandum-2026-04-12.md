# Seance Verdict: OBSERVANDUM
**Date:** 2026-04-12
**Seance Type:** First seance — no prior record
**Score: 8.1 / 10**
**V1 Status: NOT READY — preset library below minimum threshold**

---

## Architecture
- Variable 2–8 Phase Distortion oscillators ("facets") per voice
- 8 mathematically-derived distortion transfer-function curves, morphable via curve morph parameter
- 24-oscillator budget: 2 facets = 12 voices; 8 facets = 3 voices (dynamic polyphony)
- 3 ADSRs: amp, filter, distortion amount — each independently enveloped
- 2 LFOs (floor 0.01Hz): LFO1 → morph position, LFO2 → filter cutoff
- 4-slot configurable mod matrix (velocity, mod wheel, aftertouch, LFO1/2 → filter, morph, pitch, amp, distortion)
- CytomicSVF per-voice L+R independent (fixes XObsidian single-filter bug)
- 2x oversampling for anti-aliasing
- Environmental Curve Modifier: 3-mode input bus (Coupling / Sidechain / Parametric) warps distortion curve in real time
- Parametric models: Wave, Turbulence (smoothed noise), Tidal (3-sine superposition), Drift (Brownian motion)
- Identity: Cuttlefish, Epipelagic zone | Accent: Teal #4ECDC4 | Prefix: `observ_`

---

## Ghost Panel

| Ghost | Score | Key Comment |
|-------|-------|-------------|
| Moog | 7.5 | "The filter sings but I came looking for warmth and found cut crystal. Not my room, but a beautiful room." |
| Buchla | 9.0 | "Eight transfer function curves morphed in real-time by an environmental signal bus — West Coast thinking in digital form." |
| Smith | 8.5 | "Polyphony budget architecture is elegant. MIDI handling is clean. Mod matrix destinations are well-chosen." |
| Kakehashi | 7.0 | "Twenty-one presets for an engine this complex is too thin for a musician to explore." |
| Ciani | 8.5 | "Phase spread creates genuine spatial differentiation — not fake stereo but true multi-phase geometry." |
| Schulze | 8.5 | "LFO floor at 0.01Hz opens cosmic territory. Brownian drift runs for minutes before repeating." |
| Vangelis | 8.0 | "Aftertouch → distortion boost is physically coherent. Velocity into both filter AND distortion gives every note character." |
| Tomita | 8.0 | "Timbral range from Init to Acid Facet to Thermocline Pad is genuinely wide. A solo voice, not a section." |

---

## Doctrine Compliance

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 | ✅ PASS | `voice.velocity * 0.15f` → distortion; `fltLevel * paramFltEnvAmt * voice.velocity` → filter. Two timbre axes. |
| D002 | ✅ PASS | 2 LFOs (0.01Hz floor), 4-slot mod matrix, 3 ADSRs as mod sources, macros all wired. |
| D003 | N/A | Phase Distortion, not physical modeling. |
| D004 | ⚠️ CONDITIONAL | MOVEMENT macro → envDepth only. In Coupling mode (default) with no coupling signal, MOVEMENT is inert. |
| D005 | ✅ PASS | LFO1/LFO2 floor 0.01Hz. Parametric env models provide autonomous evolution indefinitely. |
| D006 | ✅ PASS | Mod wheel → filter cutoff (10000Hz range). Aftertouch → distortion boost (+0.2 max). Both wired. |

---

## Preset Assessment

**Count:** 21 presets — all in `Source/Engines/Observandum/Presets/Foundation/`

**V1 minimum:** 100 presets recommended | **Gap:** 79 presets

**Naming quality:** Excellent (Glass Tide, Ink Cloud, Vent Crawler, Halocline Keys, Thermocline Pad).

**Macro effectiveness:** CHARACTER (morph+dist), COUPLING (phase spread), SPACE (width+detune) all audible. MOVEMENT conditionally inert (see D004).

**Mood diversity:** Zero — all in one folder. No Atmosphere, Kinetic, Crystalline, Flux, Entangled presets exist as file-system categories.

---

## Coupling

**Output:** Post-filter stereo + envelope follower (ch2). Good source for AudioToFM / AmpToFilter routing.

**Input types handled:**
- `AudioToFM` (PhaseDeflection stand-in) → phase spread deflection
- `AmpToFilter` → filter cutoff
- `EnvToMorph` → curve morph position (most distinctive type)
- `RhythmToBlend` → curve modifier

**Outstanding:** `PhaseDeflection` CouplingType not yet in `SynthEngine.h` enum. Using `AudioToFM` as stand-in pending enum addition.

**Natural partners:** Opal (granular env → morph), Onset (rhythm → curve warp), Oxbow (FDN reverb → phase spread).

---

## Recommendations

### P0 — V1 blockers
1. **Expand preset library to ≥100** — distribute across Foundation, Atmosphere, Crystalline, Kinetic, Flux minimum. This is the single biggest quality gap.

### P1 — Important
2. **Fix MOVEMENT macro** — route to LFO1 depth (unconditional) rather than envDepth alone. MOVEMENT must work without coupling signal present.
3. **Add PhaseDeflection to CouplingType enum** — the coupling intent is complete; the enum entry is the only missing piece.

### P2 — Nice to have
4. **Move existing presets to correct mood folders** — Glass Tide → Crystalline, Facet Bell → Luminous, etc.
5. **Consider Init distortion default** — 0.5 is a deliberate character choice but creates DB003 tension. Revisit for blank-canvas consistency.

---

## Path to V1

**Required:** Preset library ≥100, MOVEMENT macro unconditional routing fixed.
**Estimated score at that point:** 8.5–8.7. Architecture already supports it.

---

*First seance — 2026-04-12 | Ringleader RAC Session*
