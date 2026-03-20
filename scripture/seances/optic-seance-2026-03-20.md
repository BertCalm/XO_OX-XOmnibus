# OPTIC Seance Verdict -- 2026-03-20

**Engine:** XOptic | **Gallery Code:** OPTIC | **Accent:** Phosphor Green #00FF41
**Creature:** The Comb Jelly | **Param Prefix:** `optic_`

---

## Ghost Council Score: 8.3 / 10

**Previous verdict:** Revolutionary (non-numeric)

---

## Paradigm Note

OPTIC is a **zero-audio modulation engine**. It generates no sound. Traditional doctrine criteria (D001 velocity->timbre, D005 LFO breathing, D006 expression) must be evaluated against its modulation-only identity. The Ghost Council's previous "Revolutionary" verdict recognized this paradigm; the numeric score reflects OPTIC's quality within its own terms.

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 Velocity->Timbre | N/A (EXEMPT) | OPTIC has 0 voices and no audio output. Velocity has no meaning. |
| D002 Modulation Depth | PASS (adapted) | AutoPulse = self-evolving rhythmic LFO. 8 spectral mod outputs. Evolve parameter creates feedback loop. No traditional LFO needed -- OPTIC *is* the modulation source. |
| D003 Physics Rigor | N/A | Not a physical model. |
| D004 No Dead Params | PASS | All 16 parameters affect the modulation output or visualization. Verified: reactivity, inputGain, autoPulse, pulseRate, pulseShape, pulseSwing, pulseEvolve, pulseSubdiv, pulseAccent, modDepth, modMixPulse, modMixSpec, vizMode, vizFeedback, vizSpeed, vizIntensity. |
| D005 Must Breathe | PASS (adapted) | AutoPulse drift oscillator runs at 0.07 Hz (~14-second cycle). The spectral-evolve feedback loop creates autonomous modulation evolution. |
| D006 Expression | PARTIAL | No MIDI processing at all (midi parameter is ignored). No aftertouch, no mod wheel, no velocity. This is the biggest gap: MIDI CC could modulate pulse rate or mod depth. |

---

## DSP Quality Assessment

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Anti-aliasing | N/A | No audio oscillators. |
| Denormal protection | EXCELLENT | flushDenormal on all band energies, spectral centroid, spectral flux, pulse level decay. Comprehensive. |
| Sample rate independence | EXCELLENT | All rates derived from cachedSampleRate. Envelope follower cutoffs, smoothing filters, pulse timing all sample-rate-aware. |
| Parameter smoothing | GOOD | 15 Hz lowpass smoothing on 5 mod channels. 10 Hz input smoothing. Pulse/flux/transient intentionally unsmoothed for fast response. |
| Spectral analysis | EXCELLENT | 8-band Cytomic SVF filter bank with Gaussian-mean center frequencies. Rectification + envelope following. Centroid + flux computation. Matches analog spectrum analyzer topology. |

---

## Strengths

1. **B005 Zero-Audio Identity** -- A genuinely novel concept: a synth engine that generates no sound, only modulation. 10 years ahead of commercial synthesizers.
2. **AutoPulse** -- Self-evolving rhythmic pulse with spectral feedback. Rate, shape, swing, subdivision, accent -- it is a drum machine for modulation.
3. **8-channel mod bus** -- Lock-free atomic outputs (pulse, bass, mid, high, centroid, flux, energy, transient). Clean separation of concerns.
4. **Spectral analysis quality** -- Cytomic SVF filter bank provides zero-delay-feedback accuracy.

## Issues Preventing Higher Score

1. **No MIDI expression** -- OPTIC ignores all MIDI. Adding CC1 mod wheel -> pulse rate and aftertouch -> mod depth would make it performable.
2. **No macros** -- Missing the standard M1-M4 macro system. OPTIC should have macros that reshape its modulation character.
3. **Viz params are UI-only** -- vizMode, vizFeedback, vizSpeed, vizIntensity only affect the CRT visualizer, not the modulation output. Acceptable but noted.

---

## Path to 9.0

1. Add MIDI processing: aftertouch -> mod depth, CC1 mod wheel -> pulse rate, velocity -> pulse accent on note-on. ~40 LOC.
2. Add 4 standard macros (CHARACTER -> reactivity+inputGain, MOVEMENT -> pulseRate+pulseShape, COUPLING -> modMixPulse+modMixSpec, SPACE -> pulseSwing+pulseEvolve). ~30 LOC.

**Estimated effort:** 70 LOC, 45 minutes.
