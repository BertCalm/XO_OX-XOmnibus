# ORIGAMI Seance Verdict -- 2026-03-20

**Engine:** XOrigami | **Gallery Code:** ORIGAMI | **Accent:** Vermillion Fold #E63946
**Creature:** The Paper Nautilus | **Param Prefix:** `origami_`

---

## Ghost Council Score: 8.0 / 10

**Previous verdict:** Not formally scored (non-numeric)

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 Velocity->Timbre | PASS | Velocity stored per voice. Velocity * amplitudeLevel * crossfadeGain applied to output. Filter/fold depth respond to velocity through envelope scaling. |
| D002 Modulation Depth | PASS | 2 LFOs (OrigamiLFO, 5 shapes: Sine/Tri/Saw/Square/S&H). LFO1->fold point, LFO2->rotate amount. Aftertouch + mod wheel. 4 macros (FOLD/MOTION/COUPLING/SPACE via M1-M4). |
| D003 Physics Rigor | N/A | Spectral folding synthesis (STFT-based), not physical modeling. |
| D004 No Dead Params | PASS | All params wired. Previous note that "instantaneousFreq variable is a spectral compass no preset has used" -- this is a DSP internal, not a user param. |
| D005 Must Breathe | PASS | LFO rates go down to 0.01 Hz minimum (origami_lfo1Rate/lfo2Rate NormalisableRange). Both LFOs free-running per voice. |
| D006 Expression | PASS | Aftertouch -> fold depth (+0.3 shimmer on pressure). Mod wheel CC1 -> STFT fold depth (+0.3 more spectral processing). Both smoothed via PolyAftertouch. |

---

## DSP Quality Assessment

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Anti-aliasing | ADEQUATE | STFT-based spectral processing. P0-03 race condition (hopSampleCounter + overlap-add for blockSize < 512) was fixed with block size guard. |
| Denormal protection | GOOD | Standard patterns. |
| Sample rate independence | GOOD | All timing derived from sampleRate. |
| Parameter smoothing | ADEQUATE | Per-block reads. LFO modulation applied per sample. |
| STFT stability | IMPROVED | P0-03 block size guard prevents the race condition that caused artifacts at blockSize < 512. |

---

## Strengths

1. **Spectral folding synthesis** -- Unique synthesis technique. STFT analysis -> fold/rotate/transform -> resynthesis. Not available in any commercial synth.
2. **2 LFOs with 5 shapes** -- Full modulation complement with musical destinations (fold point, rotate amount).
3. **Macro system** -- M2 MOTION adds 50% to rotate and 30% to LFO1 depth. Well-designed macro interactions.
4. **P0-03 fixed** -- Block size guard prevents the STFT race condition.

## Issues Preventing Higher Score

1. **STFT block size constraint** -- Still requires blockSize >= 512 for safe operation. At smaller block sizes the guard prevents crashes but the spectral processing may degrade.
2. **No mod matrix** -- LFO destinations hardcoded (LFO1->fold, LFO2->rotate).
3. **Spectral resolution** -- kHopSize=512 limits spectral resolution at high sample rates.
4. **No post-filter** -- No SVF filter for traditional timbral shaping after spectral processing.

---

## Path to 8.5

1. Add post-output CytomicSVF filter with velocity-envelope depth. ~50 LOC.
2. Scale hop size with sample rate for consistent spectral resolution. ~15 LOC.

## Path to 9.0

3. Add mod matrix (4 slots). ~100 LOC.
4. Add adaptive STFT overlap for small block sizes (double-buffering). ~80 LOC.

**Estimated effort to 8.5:** 65 LOC, 45 minutes.
**Estimated effort to 9.0:** 245 LOC, 3 hours.
