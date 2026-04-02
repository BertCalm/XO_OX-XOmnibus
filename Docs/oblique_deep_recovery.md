# OBLIQUE Deep Recovery

**Engine:** XOblique — Prismatic Bounce Engine
**Gallery code:** OBLIQUE | Accent: Prism Violet `#BF40FF` | Param prefix: `oblq_`
**Source:** `Source/Engines/Oblique/ObliqueEngine.h`
**Seance score:** 5.9/10 (fleet lowest) → ~7.2/10 estimated (post Round 8A)
**Recovery period:** 2026-03-14 | Rounds 3B, 7B, 7C, 7D, 7F, 8A

---

## Why OBLIQUE Scored Lowest

At seance, OBLIQUE had:
- 6 total presets (all in one mood: Prism)
- `oblq_percDecay` declared but never used — the click burst duration was hardcoded regardless of parameter value
- Zero LFOs: every held note was frozen in character — D005 violation
- Velocity shaped amplitude only (basic D001 compliance), not timbre
- No aftertouch or mod wheel response — D006 incomplete
- Filter envelope was wired but `oblq_filterEnvDepth` had not been added as a parameter (Round 7B)
- Four XOceanus standard macros (FOLD, BOUNCE, COLOR, SPACE) were not yet implemented (Round 7D)

Ghost quote: *"Six presets for the engine with the most interesting conceptual premise in the gallery."*

Despite being the lowest scorer, OBLIQUE had the strongest conceptual foundation in the fleet: the bouncing-ball physics model, 6-facet spectral prism delay, and 6-stage allpass phaser form a genuinely novel DSP architecture. The problem was presentation and doctrine coverage, not the engine itself.

---

## Recovery Timeline

### Round 3B — D004 Dead Parameters (2026-03-14)

**Fix:** `oblq_percDecay` parameter wired to click burst duration.

The `ObliqueBounce::fireClick()` method was computing `clickDecayCoefficient` from a hardcoded `0.003f` value regardless of `pPercDecay`. The fix added `currentClickDecay` member to `ObliqueBounce`, updated on each `process()` call from `bounceParams.clickDecay`, and used it in `fireClick()` to compute actual duration.

Range: 1ms (snap) to 30ms (percussive pitched click). Default 0.003 preserves legacy behavior.

Doc: `Docs/d004_fixes_applied.md`

---

### Round 7B — D001 Filter Envelope (2026-03-14)

**Fix:** Added `oblq_filterEnvDepth` parameter and per-voice filter envelope modulation.

Before this round, velocity only scaled the output amplitude. After:
- New parameter: `oblq_filterEnvDepth` (0–1, default 0.3)
- In `renderBlock()`, per-voice filter cutoff is computed as:
  `baseCutoff + filterEnvDepth × velocity × envelopeLevel × 7000 Hz`
- Effect: attack peak opens the filter; harder velocity = wider filter sweep = brighter transient.

This made Oblique's lines D001-compliant: a harder note-on produces a brighter, grittier attack that decays back toward the sustain brightness level.

---

### Round 7C — Preset Expansion 6→16 (2026-03-14)

**Fix:** 10 new presets covering 5 previously-empty moods.

Before: 6 presets, all in Prism mood.
After: 16 presets across Foundation (2), Atmosphere (2), Prism (8), Flux (2), Entangled (2). Aether remained empty.

New presets:
- **Oblique Clean Stab** (Foundation) — stripped bare, genre-agnostic
- **Oblique Bounce Pluck** (Foundation) — exposed bounce cascade
- **Prism Drift** (Atmosphere) — slow ambient with 320ms prism
- **Coral Scatter** (Atmosphere) — 14-bounce cascade, organic reef texture
- **UV Pulse** (Prism) — aggressive fold, 7kHz click, max prism color
- **Glass Arp** (Prism) — high bounce count, self-arpeggiation
- **Gravity Flux** (Flux) — max bounce count, extreme gravity acceleration
- **Prism Storm** (Flux) — high prism feedback near self-oscillation
- **Onset Ricochet** (Entangled) — ONSET drum hits trigger bounce cascades
- **Overdub Fragment** (Entangled) — prism output feeds Overdub tape delay

Doc: `Docs/oblique_expansion.md`

---

### Round 7D — Macros FOLD, BOUNCE, COLOR, SPACE (2026-03-14)

**Fix:** All four XOceanus standard macros wired to audible DSP parameters.

Added 4 parameters: `oblq_macroFold`, `oblq_macroBounce`, `oblq_macroColor`, `oblq_macroSpace`.

In `renderBlock()` ParamSnapshot section, effective parameter values are computed as:

| Macro | ID | DSP Effect |
|-------|----|-----------|
| FOLD (M1) | `oblq_macroFold` | `oscFold + macroFold × 0.7` — harmonic grit depth |
| BOUNCE (M2) | `oblq_macroBounce` | `bounceRate + macroBounce × 220ms`, `gravity + macroBounce × 0.2` |
| COLOR (M3) | `oblq_macroColor` | `prismColor + macroColor × 0.5`, `prismMix + macroColor × 0.35` |
| SPACE (M4) | `oblq_macroSpace` | `phaserMix + macroSpace × 0.5`, `prismFeedback + macroSpace × 0.3` |

All additive and clamped to valid ranges. Default 0.0 = backwards-compatible with all existing presets.

---

### Round 7F — D006 Aftertouch + Mod Wheel (2026-03-14)

**Fix:** Channel pressure (aftertouch) and CC1 mod wheel both wired to audible parameters.

- Added `PolyAftertouch aftertouch` member (5ms attack / 20ms release smoothing)
- Channel pressure → prism mix depth: `effectivePrismMix + atPressure × 0.3`
  - Pressing harder sends more signal through the 6-facet spectral delay — more colour, more shimmer
- CC1 mod wheel → prism color spread: `prismColor + modWheelValue × 0.3`
  - Wheel forward opens the spectral rainbow from convergence toward full separation
- Both wired in `renderBlock()` after MIDI processing loop

Justification: pressing harder on a prismatic instrument should refract more light. The aftertouch → prism mix mapping is physically intuitive — more pressure = more spectral spreading = more prismatic.

---

### Round 8A — D005 LFO + D001 Velocity-to-Fold (2026-03-14)

**Fix 1 (D005): Prism LFO — `obliqueLfoPhase`**

The engine had zero autonomous modulation. Every held note was a static photograph.

Added `double obliqueLfoPhase = 0.0` to private state. In `renderBlock()` per-sample loop, immediately before prism params construction:

```cpp
static constexpr double kLfoRate = 0.2;    // Hz — one sweep per 5 seconds
static constexpr double kTwoPiD  = 6.28318530718;
obliqueLfoPhase += kLfoRate / hostSampleRate;
if (obliqueLfoPhase >= 1.0) obliqueLfoPhase -= 1.0;
float obliqueLfoValue = fastSin (static_cast<float> (obliqueLfoPhase * kTwoPiD));
static constexpr float kLfoDepth = 0.15f;
float lfoColorMod = obliqueLfoValue * kLfoDepth;
```

Applied: `prismParams.colorSpread = clamp(effectivePrismColor + modWheelValue * 0.3f + lfoColorMod, 0.0f, 1.0f)`

**Design rationale:** 0.2 Hz (5-second cycle) is slow enough that a single note can live through most of a sweep without the character feeling unstable. ±0.15 spread is audible in the spectral balance of the 6-facet delay — the "colour temperature" of the prism shifts slowly, like sunlight angle changing over a reef. Double precision phase accumulator prevents drift during long ambient performance sessions.

This fixes D005 completely: the engine now breathes.

**Fix 2 (D001): Velocity-to-fold — `kVelocityFoldBoost`**

Velocity already shaped amplitude (basic D001) and filter sweep (Round 7B). This adds a third dimension: fold depth.

```cpp
static constexpr float kVelocityFoldBoost = 0.25f;
float velocityFoldAmount = clamp(effectiveOscFold + voice.velocity * kVelocityFoldBoost, 0.0f, 1.0f);
oscillatorOutput = wavefolder.process(oscillatorOutput, velocityFoldAmount);
```

At velocity 1.0 (full): +0.25 fold boost above the preset value
At velocity 0.2 (feather-soft): +0.05 fold boost — barely perceptible

**Musical effect:** hard throws of the ball scatter more spectral fragments; soft touches glow cleanly. This maps directly to the Buchla Music Easel / Run the Jewels character — aggressive playing produces metallic harmonic density while soft playing stays warm and smooth. The three velocity-responsive dimensions (amplitude, filter sweep, fold depth) now interact: a hard note gets brighter AND grittier AND louder, creating an exponential urgency that soft notes lack.

**Fix 3 (Preset expansion 16→22):** 6 new presets:

| Preset | Mood | New features |
|--------|------|-------------|
| Refraction Study | Prism | Triangle; macro defaults at 0; designed as macro showcase |
| Spectral Fire | Prism | COLOR=0.65, SPACE=0.55 pre-loaded (first preset with audible macros) |
| Light Column | Aether | Sine; long attack; first Aether preset for OBLIQUE |
| Spectral Dissolve | Aether | SPACE=0.75 pre-loaded; long release; second Aether preset |
| Ricochet Chaos | Flux | FOLD=0.80, BOUNCE=0.70 pre-loaded; both macros at high intensity simultaneously |
| Soft Prism | Foundation | Triangle; gentle; demonstrates velocity-to-fold at low settings |

Presets with non-zero macros (baked in): **Spectral Fire** (COLOR=0.65, SPACE=0.55), **Spectral Dissolve** (SPACE=0.75), **Ricochet Chaos** (FOLD=0.80, BOUNCE=0.70).

---

## Post-Recovery State

**Total presets:** 22 (was 6 at seance)

**Mood coverage:**

| Mood       | Before seance | After R7C | After R8A |
|------------|--------------|-----------|-----------|
| Foundation | 0            | 2         | 3         |
| Atmosphere | 0            | 2         | 2         |
| Prism      | 6            | 8         | 10        |
| Flux       | 0            | 2         | 3         |
| Entangled  | 0            | 2         | 2         |
| Aether     | 0            | 0         | 2         |

**Doctrine compliance:**

| Doctrine | Pre-seance | Post-R8A |
|----------|-----------|----------|
| D001 velocity→timbre | FAIL (amplitude only) | PASS (amplitude + filter envelope + fold depth) |
| D002 modulation depth | PARTIAL (phaser has internal LFO) | PARTIAL — phaser LFO + prism LFO; no user LFO rate param |
| D004 dead parameters | FAIL (`oblq_percDecay` dead) | PASS (all params audible) |
| D005 engines must breathe | FAIL (zero LFOs) | PASS (0.2 Hz prism color LFO) |
| D006 expression not optional | FAIL (amplitude velocity only) | PASS (aftertouch + mod wheel + velocity-to-fold + filter env) |

**Remaining gaps (not addressed in Round 8A):**
- D002 partial: phaser and prism LFOs run at fixed rates. A future `oblq_lfoRate` parameter would allow user control. This would require a new parameter and would require preset migration, so it is deferred to a later round.
- No user-visible LFO destination select. The prism LFO is always-on and hardwired to color spread. A full mod matrix is beyond the scope of a recovery pass.

---

## Score Projection

Starting: 5.9/10

Each fix addresses specific seance deductions:
- D004 fix (R3B): +0.3 (dead param was an explicit negative mark)
- Filter envelope (R7B): +0.2 (better D001 compliance)
- Preset expansion (R7C): +0.5 (explicit "6 presets" deduction)
- Macros wired (R7D): +0.2 (4 non-functional macros)
- Aftertouch/mod wheel (R7F): +0.2 (D006 partial compliance)
- Prism LFO (R8A): +0.4 (D005 was a hard deduction — "zero LFOs")
- Velocity-to-fold (R8A): +0.1 (D001 enhancement, not a separate violation)
- 6 new presets incl. Aether (R8A): +0.2 (Aether coverage gap)

**Estimated post-recovery score: ~7.2/10**

This moves OBLIQUE from fleet-last to mid-table, above OCEANIC (7.1), OWLFISH (7.1), ODDOSCAR (6.9), OBESE (6.6), OBSIDIAN (6.6), OCELOT (6.4), ODDFELIX (~C+). The conceptual foundation — bouncing-ball physics, 6-facet spectral delay, Buchla wavefolder — was always strong. The recovery exposed and exercised what was already there.

---

*Generated: 2026-03-14 | Prism Sweep Round 8A*
