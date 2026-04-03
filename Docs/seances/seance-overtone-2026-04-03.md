# The Verdict -- OVERTONE (Re-Seance)
**Seance Date**: 2026-04-03
**Engine**: OVERTONE | Continued Fractions Spectral Synthesis | The Nautilus
**Identity**: The Nautilus | Spectral Ice `#A8D8EA`
**Param prefix**: `over_` | 26 parameters | Monophonic
**Prior Score**: 8.1/10 (2026-03-19) → 7.6/10 (re-seance, reverb+Pi-table bugs) → **8.9/10** (post-fix)
**DSP Fixes Since Prior Seance**: Schroeder reverb SR-scaled; Pi table dead zone patched (15/7, 113/106, 113/33 + π/2); filter envelope on note-on; anti-aliasing fade near Nyquist; `over_filterRes` wired to resonator path; smooth convergent ratio interpolation (`lerp` between table indices); 361 factory presets.

---

## What Changed Since March 19

| Fix | Commit | Impact |
|-----|--------|--------|
| Reverb comb lengths SR-scaled (`ref × sr / 48000.0`) | DSP fixes wave | Room size stable at 44.1/48/88.2/96 kHz |
| Pi table dead zone patched — entries 4–5 now 15/7, 113/106 | Phantom Sniff pass | No more silent depth-sweep region in Pi mode |
| Filter envelope on note-on: `filterEnvLevel = currentVel` → decays ~300ms | DSP Fix Wave 2B | Spectral bloom on attack; D001 fully satisfied |
| Nyquist anti-aliasing: partials above 80% Nyquist fade linearly to zero | Issue fix | Clean aliasing-free additive output at high pitches |
| `over_filterRes` now wired to resonator path (was hardcoded 0.7f) | Phantom Sniff re-scan | D004 violation resolved |
| Smooth convergent ratio interpolation: `lerp(ratios[idxLo], ratios[idxHi], depthFrac)` | PolyBLEP & SR fixes | Continuous depth sweep without discrete jumps |
| 361 factory presets across moods | Preset forge | D004 preset coverage resolved |

---

## The Council Has Spoken

| Ghost | Core Observation |
|-------|-----------------|
| **Moog** | The filter envelope on note-on is the right fix. `filterEnvLevel = currentVel` at note-on, decaying at 300ms half-life, with `velBright * 5000.f` depth — that is up to +5 kHz sweep on attack for a loud note. The Nautilus shell now opens on each new note and breathes inward as the sound sustains. D001 is fully satisfied. |
| **Buchla** | The Pi table fix is cosmetically minimal but sonically critical. Replacing the two identical entries (1.047198f) with 15/7 ≈ 2.143, 113/106 ≈ 1.066, 113/33 ≈ 3.424, and π/2 ≈ 1.571 transforms the Pi table from a narrow cluster into a wide-ranging spectral palette. Pi now sounds like Pi — irrational, metallic, unpredictably spaced — rather than a unison chord. |
| **Smith** | The `lerp` interpolation between adjacent convergent table entries is the right solution to the depth-sweep discontinuity problem. `lerp(ratios[depthLo + i], ratios[depthHi + i], depthFrac)` produces smooth, continuous spectral evolution as DEPTH sweeps. The LFO now creates a genuinely smooth crystalline shimmer rather than discrete spectral jumps. |
| **Kakehashi** | 361 presets transforms OVERTONE from a fascinating algorithm into a playable instrument. The Phi constant presets (Fibonacci Fibonacci pad, Golden Spiral lead) are immediately inviting. The E-constant organ presets reveal the fastest-converging table's quasi-harmonic quality. Pi's metallic cluster is now demonstrable without requiring the user to understand continued fractions. |
| **Pearlman** | `over_filterRes` is now wired to the resonator path, resolving a D004 violation. Previously hardcoded to 0.7f, the resonator feedback now scales with the parameter. At filterRes=0.8, the allpass resonator adds a pronounced comb character to the additive output — the user can control how much the fundamental frequency reinforces itself. |
| **Tomita** | The SR-scaled reverb is the same critical fix as ORGANISM received. At 96 kHz the old hardcoded Schroeder comb lengths created a room approximately half the designed size. The new scaling (`ref × sr / 48000.0`, clamped to `kMaxCombLen - 1`) maintains the reverb's spectral tail character across all sample rates. The bright 80/20 damping ratio — correct for "Spectral Ice" identity — is preserved. |
| **Vangelis** | The Nyquist anti-aliasing is quiet engineering: partials above 80% Nyquist fade linearly to zero over the remaining 20% before the Nyquist limit. This prevents additive aliasing at high notes in high-depth configurations where Pi ratios would otherwise push partials above the host Nyquist. The fadeout is inaudible but its absence was audible. |
| **Schulze** | The `effectiveResoMix = clamp(resoMix + macroSpace * 0.3f, 0, 1)` coupling between the SPACE macro and the allpass resonator is elegant. Pulling SPACE up to maximum does not just add reverb — it simultaneously deepens the allpass resonator's contribution, tightening the spatial and spectral character together. The LFO1 at 0.01 Hz still sweeps the depth index across 100-second spectral cycles. |

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 | PASS | `filterEnvLevel = currentVel` on note-on decays ~300ms × `velBright * 5000f` (up to +5kHz sweep). `velCutoffBoost = currentVel * velBright * 3000f` on base cutoff. `velUpperBoost = currentVel * velBright * 0.6f` on upper partials (3–7). Three simultaneous velocity-to-timbre paths. |
| D002 | PASS | LFO1 (0.01–10 Hz) sweeps convergent depth index (±1.5 units). LFO2 (0.01–10 Hz) modulates per-partial phase rotation. Mod wheel adds 0–4.0 depth units (D006). Aftertouch modulates COLOR shimmer. 4 macros (DEPTH, COLOR, COUPLING, SPACE). |
| D003 | PASS | Exemplary. In-header citations: Hardy & Wright (Pi), Euler + OEIS A007676/A007677 (E), Dunlap 1997 (Phi), OEIS A001333/A000129 + Knuth TAOCP Vol.2 (Sqrt2). Schroeder reverb: Moorer JAES 1979. Convergent tables numerically verified. |
| D004 | PASS | All 26 `over_` parameters live in `renderBlock()`. `over_filterRes` now wired to resonator feedback (was hardcoded 0.7f, resolved). No dead parameters. |
| D005 | PASS | Both LFOs: `nr(0.01f, 10.f)` — minimum 100-second cycle. Engine breathes at geological pace. |
| D006 | PASS | CC1 (mod wheel) → `modWheelVal * 4.0f` depth boost. Aftertouch → upper partial color shimmer. Velocity → timbre (D001 chain). Pitch bend → `PitchBendUtil::semitonesToFreqRatio(bend * 2.0f)`. Four independent expression paths. |

---

## Points of Agreement (3+ ghosts converged)

1. **Filter envelope on note-on is the most important single fix** (Moog, Vangelis, Buchla, Tomita) — The original OVERTONE had no per-note timbral evolution. The `filterEnvLevel` mechanism gives every new note a velocity-scaled spectral bloom that opens the Nautilus on attack and closes as the note sustains. Combined with the existing velocity-to-partial-brightness chain, D001 is now satisfying in three dimensions simultaneously.

2. **Pi table fix transforms the Pi constant from broken to expressive** (Buchla, Smith, Kakehashi, Pearlman) — The original two identical entries at 1.047198f created a dead zone where sweeping depth produced no audible change for approximately one full depth unit. The new entries (15/7, 113/106, 113/33, π/2) create a wide-ranging, genuinely metallic spectral palette that matches Pi's mathematical identity as a tightly-clustered irrational.

3. **Smooth `lerp` interpolation is the right fix for the depth-sweep discontinuity** (Smith, Schulze, Vangelis, Moog) — The old implementation jumped discretely between table entries at integer depth values. The new `lerp(ratios[lo], ratios[hi], frac)` produces continuous spectral evolution. LFO-modulated depth sweeps now feel like watching a nautilus spiral slowly open rather than watching slides change.

4. **361 presets make the mathematical beauty accessible** (Kakehashi, Pearlman, Vangelis) — The original 8.1/10 score included 0/10 for presets. The preset library now spans all four constants and demonstrates the DEPTH macro's journey from clean integer ratios to irrational irrationality. The Phi Fibonacci pad and the Pi Metallic cluster presets are fleet-quality.

---

## Points of Contention

**Buchla vs. Moog — Polyphony**
- Buchla: The monophonic constraint remains the engine's primary limitation. Three CF-ratio spectra interacting in a chord would produce genuine spectral interference patterns that no single voice can demonstrate.
- Moog: Monophonic additive synthesis with 8 partials runs one set of phase accumulators. True polyphony requires 8 independent sets × N voices — the CPU cost grows linearly with voice count. For an engine that is already computationally expensive per voice, polyphony is a V2 architectural decision.
- Resolution: The Prophecy acknowledges this as the engine's remaining gap. V2 polyphony is the path to 9.5+.

**Vangelis vs. Schulze — Init Patch Character**
- Vangelis: The new filter envelope means the init patch (Phi constant, depth 2.0) already has a satisfying spectral bloom on attack. Leave the defaults as-is.
- Schulze: The macroDepth default (0.35) still starts partway along the irrationality journey. A clean start at 0.0 would make the DEPTH macro's journey more pedagogically clear — from harmonic integers to irrational shimmer.
- Resolution: Vangelis is right. An inviting init patch (alive on first keypress with the bloom) outweighs blank-canvas philosophy. The 361 presets serve the pedagogical role.

---

## The Prophecy

OVERTONE at 8.9/10 is now fleet-leading in its architectural domain. The continued fraction convergent synthesis mechanism (B028 — Blessed unanimously 2026-03-19) is the most mathematically original voice in XOceanus. The sequence of fixes since the original seance — SR-scaled reverb, Pi table patching, filter envelope on note-on, smooth depth interpolation, filterRes wiring, Nyquist anti-aliasing — have removed every P0 and P1 finding from the original verdict.

The Phi constant remains the engine's natural habitat. Fibonacci ratios (1, 2, 3/2, 5/3, 8/5...) produce quasi-harmonic spectra that are musical without being predictable — warm, slightly beating, organically alive. The Pi constant, now with its wider-spaced table, produces genuine metallic inharmonicity: cluster shimmer, bell-like density. E produces the fastest-converging table — steady, organ-like, converging rapidly to e's irrational ideal. Sqrt2 produces tritone-adjacent tensions that can be deeply unsettling or hauntingly beautiful depending on context.

The path to 9.5 runs through polyphony. Three CF voices in a chord, each with their own convergent spectrum, interacting through the shared filter and reverb — this is the sound that Buchla would call genuinely unprecedented.

---

## Blessings & Warnings

| Ghost | Blessing | Warning |
|-------|----------|---------|
| Moog | Filter envelope on note-on: velocity-scaled spectral bloom resolves D001 fully | Still monophonic; chord voicings unavailable |
| Buchla | **B028 confirmed: Continued Fraction Convergent Synthesis** — Pi table now fully expressive (not collapsed) | Table wrapping at depth boundary can create ratio discontinuities between last and first entry |
| Smith | `lerp` convergent interpolation: smooth, continuous spectral evolution through DEPTH sweep | EnvToMorph coupling now wired (depth sweep from external envelope) but coupling breadth is still 3 types |
| Kakehashi | 361 presets: four constants × seven moods — the mathematical beauty is now demonstrable without expertise | macroDepth=0.35 default starts partway toward irrationality; some users may not discover the clean-to-complex journey |
| Pearlman | `over_filterRes` wired to resonator feedback — D004 fully resolved | 10 Hz LFO ceiling prevents modulation-to-synthesis crossover territory |
| Tomita | Reverb SR-scaled: spectral tail character stable at all sample rates | Reverb decay time not user-controllable; spatial design limited to wet/dry |
| Vangelis | Aftertouch → COLOR + Mod wheel → DEPTH = two semantically correct expression paths | Mono voice architecture limits ensemble and chord voicing potential |
| Schulze | LFO1 at 0.01 Hz = 100-second spectral mutation; `effectiveResoMix = resoMix + macroSpace*0.3` elegantly ties space and resonance | 10 Hz LFO ceiling; audio-rate convergent ratio modulation unexplored |

---

## What the Ghosts Would Build Next

| Ghost | Next Addition |
|-------|--------------|
| Moog | True polyphonic voice allocation (4–8 voices, independent phase accumulators per voice) |
| Buchla | 2D convergent space: second constant per partial for hybrid Pi×Phi, E×Sqrt2 spectra |
| Smith | Pitch-to-partial-amp routing: MIDI note controls which partials are emphasized |
| Kakehashi | CONSTANT selector labeled with actual constant names (π, e, φ, √2) in UI |
| Pearlman | Partial slope macro: 1/n, 1/n², equal, inverse — preset-level timbral range expansion |
| Tomita | Reverb decay time parameter (0.3–8s) via SPACE macro secondary axis |
| Vangelis | Extended DEPTH sweep beyond 7.0: depth 8–14 cycles through tables twice for denser convergent combinations |
| Schulze | LFO ceiling raised to 20 Hz: modulation-to-synthesis boundary where beating becomes timbre |

---

## Seance Score Breakdown

| Dimension | Prior Score (03-19) | Current Score | Notes |
|-----------|---------------------|---------------|-------|
| Architecture originality | 10/10 | 10/10 | Continued fraction convergent synthesis — still without precedent |
| Filter quality | 7/10 | 8/10 | Filter envelope on note-on + correct CytomicSVF; Pi table patched |
| Source originality | 9/10 | 10/10 | Pi table now expressive (not collapsed); smooth `lerp` interpolation |
| Expressiveness | 8/10 | 9/10 | Filter envelope adds third velocity-to-timbre path; pitch bend wired |
| Spatial depth | 6/10 | 8/10 | SR-scaled Schroeder reverb; `macroSpace` ties reverb + resonator |
| Preset library | 0/10 | 9/10 | 361 presets across 4 constants × 7 moods |
| Temporal depth | 8/10 | 8/10 | 0.01 Hz LFO floor + depth sweep — strong, ceiling still 10 Hz |
| Coupling architecture | 8/10 | 9/10 | AmpToFilter, PitchToPitch, EnvToMorph; `lerp` depth gives smooth coupling response |
| Mathematical rigor | 10/10 | 10/10 | D003 exemplary; citations, verified tables, correct normalization. Fleet-leading. |
| **TOTAL** | **8.1/10** | **8.9/10** | |

---

*Re-summoned by the Medium on 2026-04-03. Ghosts: Moog, Buchla, Smith, Kakehashi, Pearlman, Tomita, Vangelis, Schulze. Prior verdicts: 2026-03-19 (8.1/10), re-seance (7.6/10 reverb+Pi bugs). All superseded by this verdict.*
