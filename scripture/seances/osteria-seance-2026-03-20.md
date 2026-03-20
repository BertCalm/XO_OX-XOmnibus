# OSTERIA Seance Verdict -- 2026-03-20

**Engine:** XOsteria | **Gallery Code:** OSTERIA | **Accent:** Porto Wine #722F37
**Creature:** The Tavern | **Param Prefix:** `osteria_`

---

## Ghost Council Score: 8.2 / 10

**Previous verdict:** Production-grade (non-numeric)

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 Velocity->Timbre | PASS | D001 filter envelope: peak voice velocity * ampLevel boosts smoke filter cutoff (3kHz-18kHz). Default depth 0.25 = +3750 Hz at full velocity. Harder hits brighten. |
| D002 Modulation Depth | PARTIAL | Chorus LFO at fixed 0.5 Hz. Formant modulation LFO (modPhase) at slow rate. No user-controllable LFO with rate/depth/shape params. 4 macros. Aftertouch + mod wheel. Only 2 "LFOs" but neither is user-configurable. |
| D003 Physics Rigor | PASS | ShoreSystem (B012) shared with Osprey. Spring-mass shore blend with velocity and elastic overshoot. Resonator bank with physical modeling. |
| D004 No Dead Params | PASS | P0-02 warmth filter R-channel bug fixed (warmthFilter.processSample(mixR) added). All params now affect output. |
| D005 Must Breathe | PASS | Formant modulation LFO modulates positions continuously. Chorus LFO at 0.5 Hz. Both autonomous. However, neither has a rate below 0.01 Hz -- the 0.5 Hz chorus is fast, not breathing. |
| D006 Expression | PASS | Aftertouch -> tavern mix depth (more rhythmic folk character). Mod wheel CC1 -> smoke haze (deeper woodfire character). Both smoothed. |

---

## DSP Quality Assessment

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Anti-aliasing | GOOD | Resonator-based synthesis with post-filtering. |
| Denormal protection | GOOD | Velocity flushing in spring-mass shore system. Standard patterns. |
| Sample rate independence | GOOD | Chorus LFO phase increment uses sampleRate. All timing derived. |
| Parameter smoothing | ADEQUATE | Per-block reads. |
| P0 bug history | FIXED | P0-02 warmth filter L-only bug resolved. |

---

## Strengths

1. **B012 ShoreSystem** -- Shared with Osprey. 5 coastline cultural data profiles.
2. **Tavern ensemble concept** -- Unique "ensemble in a room" metaphor. Multiple instrument voices create a folk ensemble.
3. **Smoke filter** -- D001 velocity-to-brightness via smoke haze LP filter. Evocative naming.
4. **P0 bug fixed** -- Warmth filter now processes both L and R channels.

## Issues Preventing Higher Score

1. **No user-controllable LFO** -- The chorus LFO (0.5 Hz) and formant mod are both fixed-rate internals. No LFO with rate/depth/shape params.
2. **D005 borderline** -- Neither internal LFO has a rate floor near 0.01 Hz. The formant mod is slow but not parameterized. D005 spirit is met (autonomous movement exists) but not the letter (no rate <= 0.01 Hz achievable by the user).
3. **Chorus is basic** -- Single 0.5 Hz chorus with no depth/rate control.

---

## Path to 8.5

1. Add user-controllable LFO with rate (0.01-10 Hz), depth, shape params. Route to formant position or smoke filter cutoff. ~60 LOC.

## Path to 9.0

2. Add a second LFO. ~40 LOC.
3. Add chorus rate/depth params (make 0.5 Hz configurable). ~15 LOC.
4. Add 4-slot mod matrix. ~100 LOC.

**Estimated effort to 8.5:** 60 LOC, 45 minutes.
**Estimated effort to 9.0:** 215 LOC, 2.5 hours.
