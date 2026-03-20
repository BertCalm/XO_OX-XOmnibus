# ORBITAL Seance Verdict -- 2026-03-20

**Engine:** XOrbital | **Gallery Code:** ORBITAL | **Accent:** Warm Red #FF6B6B
**Creature:** The Circling Current | **Param Prefix:** `orb_`

---

## Ghost Council Score: 8.7 / 10

**Previous verdict:** APPROVED (non-numeric)

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 Velocity->Timbre | PASS | Filter envelope depth param (orb_filterEnvDepth, default 0.25). Velocity * envLevel * 7000 Hz sweep on post-SVF filter. Harder hits = brighter timbre. |
| D002 Modulation Depth | PASS | Spectral drift LFO (0.03 Hz + mod wheel scales up to 0.33 Hz). 4 macros (SPECTRUM/EVOLVE/COUPLING/SPACE). Aftertouch + mod wheel. 4-group envelope system (B001). No user-controllable dedicated LFO with shape/rate params, but drift LFO plus group envelopes provide deep modulation. |
| D003 Physics Rigor | PASS | Fletcher's piano string inharmonicity formula (f_n = n * f_1 * sqrt(1 + B*n^2)). Peterson & Barney formant frequencies. Fourier series profiles. |
| D004 No Dead Params | PASS | All 33 parameters verified wired to DSP. orb_filterEnvDepth added during Prism Sweep. |
| D005 Must Breathe | PASS | spectralDriftPhase at 0.03 Hz (33-second cycle) modulates morph position +/-0.05. Rate floor well below 0.01 Hz. |
| D006 Expression | PASS | Aftertouch -> morph position (+0.3 toward profile B). Mod wheel CC1 -> spectral drift rate (+0.3 Hz). Both smoothed via PolyAftertouch. |

---

## DSP Quality Assessment

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Anti-aliasing | GOOD | Additive synthesis with sine partials -- inherently alias-free. Phase accumulation uses double precision. |
| Denormal protection | EXCELLENT | ScopedNoDenormals at block start. flushDenormal on envelope levels (decay, release, group envelopes). Voice fadeOutLevel flushed. |
| Sample rate independence | EXCELLENT | All envelope coefficients computed from sampleRate. Phase increments use 2*pi/sampleRate. No hardcoded rates. |
| Parameter smoothing | ADEQUATE | Per-block parameter reads (ParamSnapshot). Formant filter rebuilt only on change (dirty flag). Filter coefficients set per-block. |
| Memory safety | EXCELLENT | Fixed arrays (64 partials per voice). Double-precision phase wrapping every 1024 samples. No audio-thread allocation. |

---

## Strengths

1. **B001 Group Envelope System** -- 4-band independent AD envelopes (body/presence/air/shimmer) create "living spectrum" evolution. Crowned by Moog + Smith.
2. **64-partial additive synthesis** -- Up to 384 simultaneous partials (64 * 6 voices). Spectral morphing between 8 profiles.
3. **Formant filter** -- Peterson & Barney vowel formants (A/E/I/O/U) + spectral tilt/odd-even mode. Rebuilt only when params change.
4. **Comprehensive coupling** -- AudioToWavetable (spectral DNA transfer), AudioToFM, AudioToRing, AmpToFilter, EnvToMorph, LFOToPitch, PitchToPitch, EnvToDecay, RhythmToBlend.
5. **Voice modes** -- Poly/Mono/Legato with smooth pitch slide (phase accumulators preserved).

## Issues Preventing Higher Score

1. **No dedicated user LFO** -- The 0.03 Hz drift is automatic. There is no user-controllable LFO with rate/depth/shape params. The group envelopes compensate somewhat.
2. **No mod matrix** -- LFO/expression destinations are hardcoded.
3. **Per-block filter coefficient update** -- At large block sizes (1024+), filter sweeps may sound stepped.

---

## Path to 9.0

1. Add 2 user LFOs with rate (0.01-30 Hz), depth, shape params. Route to morph, brightness, formant shift. ~80 LOC.

## Path to 9.5

2. Add 4-slot mod matrix (source: LFO1/LFO2/AmpEnv/FilterEnv, dest: any param). ~100 LOC.
3. Per-sample filter coefficient interpolation for smooth sweeps. ~20 LOC.

**Estimated effort to 9.0:** 80 LOC, 1 hour.
