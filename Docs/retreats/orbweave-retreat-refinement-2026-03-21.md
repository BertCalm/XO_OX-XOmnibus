# ORBWEAVE Parameter Refinement Log
*2026-03-21 — Guru Bin Second Retreat*

---

## Engine Summary

**ORBWEAVE** — The Kelp Knot. Topological knot-coupling synthesis.
- 4 oscillator strands per voice, phase-coupled through a 4×4 topology matrix
- 4 knot types: Trefoil, Figure-Eight, Torus (P/Q variable), Solomon
- 8 voices max | 33 parameters | Prefix: `weave_`
- Blessings: B021 (Knot Phase Coupling Matrix), B022 (MACRO KNOT Continuous Topology Morphing)

---

## Parameter Analysis

### weave_strandType (0=Off, 1=Sine, 2=Saw, 3=Square, 4=Triangle)
- **Default:** 1 (Sine)
- **Assessment:** Default is correct. Sine produces mathematically pure coupling (see WEAVE-II). The other waveforms are legitimate texture choices but must be used with awareness that their coupling is approximate, not exact.
- **Refinement:** No change. Document in preset descriptions whether Sine (pure) or other (rich) is chosen and why.

### weave_strandTune (-24 to +24 semitones)
- **Default:** 0.0
- **Assessment:** Default is appropriate for single-pitch topologies (Trefoil, Figure-Eight, Torus). For Solomon knots, zero means both rings are at the same pitch — the architecture's unique chord capability is dormant.
- **Refinement:** When knotType=3 (Solomon), strandTune should almost always be non-zero. Five canonical values: 0.0 (unison doubling), 3.86 (minor third), 5.0 (major third), 7.02 (fifth), 12.0 (octave). Each is a distinct instrument.

### weave_knotType (0=Trefoil, 1=Figure-Eight, 2=Torus, 3=Solomon)
- **Default:** 0 (Trefoil)
- **Assessment:** Trefoil is a safe default — asymmetric but understandable. The library over-relies on it. Figure-Eight is more complex (alternating signs in coupling matrix: +0.6, -0.4). Torus is symmetric by default but P/Q unlocks its full range. Solomon is the chord architecture.
- **Distribution observation:** 70%+ of existing presets use knotType=0 or 3. knotType=1 and knotType=2 with non-default P/Q values are underrepresented. Design goal: at least 20% each.

### weave_braidDepth (0.0 to 1.0)
- **Default:** 0.5
- **Assessment:** 0.5 is the Default Trap. The threshold for audible topology is ~0.7. Values 0.4–0.65 are a "gray zone" — neither clearly detuned nor clearly topological.
- **Refinement:** Design presets with intention: either below 0.35 (subtle texture / detuned zone) or above 0.72 (topology zone). Avoid 0.4–0.65 unless the gray zone is intentional. When in doubt, go higher.
- **Optimal ranges:** Textural: 0.0–0.3 | Threshold: 0.65–0.78 | Full: 0.82–1.0

### weave_torusP (1 to 8) / weave_torusQ (1 to 8)
- **Default:** P=2, Q=3
- **Assessment:** (2,3) is the trefoil-torus — a reasonable default but widely overused. Five distinct P/Q families with meaningful sonic differences are now documented:
  - (2,3): pqScale=0.933 — warm, standard
  - (2,5): pqScale=0.976 — bright, cinquefoil star
  - (3,4): pqScale=0.854 — intermediate complexity, shimmer
  - (3,5): pqScale=0.975 — near-pentagram, complex overtones
  - (5,8): pqScale=0.962 — golden ratio winding, maximum polarization
- **Refinement:** Any Torus preset should consciously choose its P/Q. No Torus preset should default to (2,3) without verifying that (2,5) or (5,8) doesn't serve better.

### weave_filterCutoff (20 to 20000 Hz, skew 0.3)
- **Default:** 8000 Hz
- **Assessment:** High default makes velocity response negligible (velocity adds +2000 Hz — from 8000 to 10000 is barely perceptible). For velocity-expressive design, cutoff must be below 2000 Hz. For velocity-insensitive design, above 6000 Hz is correct.
- **Refinement (WEAVE-VI):**
  - Velocity-expressive lead/bass presets: 300–1500 Hz
  - Velocity-responsive but not dramatic: 1500–3000 Hz
  - Velocity-insensitive pad: 5000–12000 Hz
  - Band-pass rhythm presets: 200–600 Hz with resonance ≥0.5

### weave_filterReso (0.0 to 1.0)
- **Default:** 0.0
- **Assessment:** Zero resonance means filtering has minimal character. Resonance above 0.3 gives the filter a voice. For knot topology work, resonance between 0.2–0.4 highlights the spectral beating without overshadowing it. Values above 0.6 should be used deliberately (Hadal Braid: 0.62 with Band-Pass).
- **Refinement:** Most presets should have filterReso between 0.1–0.35. Reserve 0.5+ for intentionally resonant or filtered presets.

### weave_filterType (0=LP, 1=HP, 2=BP)
- **Assessment:** LP used almost universally. HP useful for spectral isolation (Spiral Torus: removes fundamental, exposes coupling structure). BP at low frequencies creates rhythm patterns from knot coupling's aperiodic beating.
- **Refinement:** Always consider HP for presets where you want the coupling structure exposed without the fundamental. BP deserves more use in bass and percussive presets.

### weave_ampA / weave_ampD / weave_ampS / weave_ampR
- **Ranges:** A: 0.0–10.0s | D: 0.0–10.0s | S: 0.0–1.0 | R: 0.0–20.0s
- **Assessment:** The wide release range (up to 20s) is valuable for ambient work. Most presets use R between 0.5–6.0, leaving the 6–20s range unexplored.
- **Refinement:** For meditation/aether presets, R at 8–15s is worth investigating. The long release allows the knot coupling to continue evolving during the decay phase.

### weave_lfo1Type / weave_lfo1Target / weave_lfo1Depth / weave_lfo1Rate (same for LFO2)
- **LFO Types:** 0=Off, 1=Sine, 2=Triangle, 3=Saw, 4=Square, 5=S&H
- **LFO Targets:** 0=None, 1=Pitch, 2=FilterCutoff, 3=FilterReso, 4=Volume, 5=BraidDepth
- **Assessment:** BraidDepth (target 5) is the most powerful LFO target for this engine — it modulates the fundamental character of the topology, not just a surface parameter. Yet it is underused relative to FilterCutoff (target 2).
- **Key interactions:**
  - LFO on BraidDepth at 0.01–0.15 Hz: breathing topology (WEAVE-I applied dynamically)
  - LFO on BraidDepth at 5–10 Hz: near-audio-rate aperiodic texture
  - LFO on FilterCutoff + LFO on BraidDepth simultaneously: two-dimensional evolution
- **Rate sweet spots:**
  - 0.01–0.05 Hz: geological / oceanic (D005 floor, breathes)
  - 0.05–0.25 Hz: slow organic evolution
  - 0.2–0.8 Hz: noticeable sweep
  - 1.0–4.0 Hz: tremolo/vibrato territory
  - 5.0–10.0 Hz: near-audio-rate texture
- **Refinement:** Target 5 (BraidDepth) deserves equal representation with Target 2 (FilterCutoff). Dual-LFO presets where both targets are active simultaneously are more expressive than single-LFO presets.

### weave_fx1Type / weave_fx1Mix / weave_fx1Param (same for FX2, FX3)
- **FX Types:** 0=Off, 1=Delay, 2=Chorus, 3=Reverb
- **Chain order:** FX1 → FX2 → FX3 (series)
- **MACRO SPACE:** Multiplies all FX mix values toward 1.0 simultaneously
- **Assessment:** Reverb (type 3) is the most-used FX. Delay (type 1) is underused — a delay with knot coupling on the sustained note creates interesting cross-cutting between the delay repeat and the evolving topology. Chorus (type 2) with Torus knot can create near-physical resonance.
- **Refinement for dry presets (coupling sources):** All FX should be off (type 0, mix 0.0). Dry output makes ORBWEAVE a cleaner coupling signal source.

### weave_macroWeave (0.0 to 1.0)
- **DSP effect:** `effBraidDepth = braidDepth + macroWeave * (1.0 - braidDepth)`
- **Assessment:** Opens braid depth toward 1.0 from any starting point. Useful as a performance control (sweep from designed depth toward maximum). Most presets leave it at 0.0.
- **Refinement:** For any preset where braidDepth < 0.8, macroWeave can serve as a "more topology" performance control. Design patches with this axis in mind.

### weave_macroTension (0.0 to 1.0)
- **DSP effect:** Adds `macroTension * 0.4` to filterReso
- **Assessment:** Adds up to 0.4 resonance boost — can push a 0.0 resonance preset into 0.4 resonant territory. This is subtle at low base resonance and dramatic at high base resonance. Most presets leave it at 0.0.
- **Refinement:** Best used with filterReso between 0.3–0.5 so TENSION sweeps from "present resonance" to "self-oscillating resonance."

### weave_macroKnot (0.0 to 1.0)
- **DSP effect:** Blends coupling matrix from knotType to knotType+1 (mod 4)
- **Assessment:** The most powerful macro in the engine. Creates topological chimera states. Most presets use 0.0 (no blend). This is a significant underuse of Blessing B022.
- **Refinement:** At least 25% of presets should have macroKNOT ≠ 0.0. Fixed midpoint values (0.5) deserve explicit preset design. The KNOT macro is not a tone control — it is topology morphing.

### weave_macroSpace (0.0 to 1.0)
- **DSP effect:** Multiplies all active FX mix values toward 1.0 simultaneously
- **Assessment:** Clean scaling control for reverb/chorus/delay presence. Works well as a performance control for "how much space" the player wants.
- **Refinement:** For any preset with FX below 0.7 mix, macroSpace sweeps it to full wet. Design presets with FX mix at 0.3–0.5 so SPACE provides a meaningful range.

### weave_voiceMode (0=Mono, 1=Legato, 2=Poly4, 3=Poly8)
- **Default:** 3 (Poly8)
- **Assessment:** Most presets use Poly8. Legato (mode 1) activates glide between notes and is designed for lead work. Mono (mode 0) retriggers envelope on each note — useful for percussive presets.
- **Refinement:** Poly8 is correct for pads. For leads, Legato with glideTime > 0 activates the portamento system. The strandTune tuning passes through glide smoothly.

### weave_glideTime (0.0 to 1.0, skew 0.3)
- **Assessment:** Legato mode without glide is under-designed. When glideTime > 0 in Legato mode, the portamento path carries the topology through note changes — the coupling matrix remains active during the slide. This creates a uniquely topological portamento that no pitch-only glide can replicate.
- **Refinement:** Any Legato preset should have glideTime > 0. Values 0.2–0.6 give noticeably musical glide.

---

## Cross-Parameter Interactions (Most Important)

| Interaction | Why It Matters |
|-------------|----------------|
| braidDepth × knotType | Below 0.7, all knot types sound similar. Above 0.7, they diverge dramatically. |
| strandType × braidDepth | Sine: exact coupling. Saw/Square/Triangle: approximate. Difference most audible above braidDepth=0.8. |
| knotType=Solomon × strandTune | The Solomon knot IS a chord architecture. strandTune = the chord interval. |
| filterCutoff × velocity | +2000 Hz at full velocity. Low cutoff = expressive. High cutoff = velocity-insensitive. |
| macroKNOT (fixed) × braidDepth LFO | Shape × amplitude modulation. Two-dimensional topology gesture. |
| filterType=BP × filterReso | Band-Pass at high resonance isolates the knot's aperiodic beating into rhythm. |

---

## Design Recommendations Summary

1. **Default Trap remediation**: braidDepth should be either < 0.35 or > 0.72. The 0.35–0.65 zone should be explicitly chosen.
2. **Solomon discipline**: Every Solomon preset should specify its strandTune interval. Document the interval in the name.
3. **Torus diversity**: P/Q should vary. The five documented families cover the meaningful range. Use at least two when designing a batch of Torus presets.
4. **macroKNOT deployment**: Design at least one-quarter of presets with macroKNOT ≠ 0.0. The chimera state (0.5) deserves its own preset archetype.
5. **LFO target 5**: BraidDepth modulation is underrepresented. It is the most topologically meaningful LFO target.
6. **Velocity-expressive design**: filterCutoff below 1500 Hz for any preset intended to be velocity-sensitive.
7. **Dry presets for coupling**: For Family mood presets used as coupling sources, turn off all FX for a clean signal.

---

*Log completed 2026-03-21*
