# ONSET Seance Verdict -- 2026-03-20

**Engine:** XOnset | **Gallery Code:** ONSET | **Accent:** Electric Blue #0066FF
**Creature:** The Pistol Shrimp | **Param Prefix:** `perc_`

---

## Ghost Council Score: 8.8 / 10

**Previous verdict:** Ahead of industry (non-numeric)

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 Velocity->Timbre | PASS | D001 velocity opens filter -- harder hits are brighter. Snap transient scaled by velocity. Body tone + noise mix both velocity-responsive. |
| D002 Modulation Depth | PASS | 4 macros (MACHINE/PUNCH/SPACE/MUTATE). Aftertouch + mod wheel. Breathing LFO (0.08 Hz). XVC cross-voice coupling matrix provides internal modulation. No user-controllable LFO with rate param, but XVC + macros + breathing LFO provide deep modulation. |
| D003 Physics Rigor | N/A | Drum synthesis, not physical modeling (hybrid circuit/algorithm blend). |
| D004 No Dead Params | PASS | All params wired. 8 voice types (Kick/Snare/CHat/OHat/Clap/Tom/Perc/FX) each use all relevant params. |
| D005 Must Breathe | PASS | BreathingLFO at 0.08 Hz modulates filter cutoff continuously. 12.5-second cycle. |
| D006 Expression | PASS | Aftertouch -> PUNCH macro (+0.3). Mod wheel CC1 -> MUTATE macro depth scale. Both smoothed via PolyAftertouch. |

---

## DSP Quality Assessment

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Anti-aliasing | GOOD | Noise-based synthesis + sine-based body tones. No waveform aliasing concerns. |
| Denormal protection | GOOD | Standard flushDenormal in filter paths. |
| Sample rate independence | GOOD | All timing derived from sampleRate. |
| Parameter smoothing | ADEQUATE | Per-block parameter reads. |
| Voice architecture | EXCELLENT | 8 independent voice types with XVC cross-voice coupling. 111 parameters. |

---

## Strengths

1. **B002 XVC Cross-Voice Coupling** -- 3-5 years ahead of commercial drum machines. 8 voices can modulate each other's parameters.
2. **B006 Dual-Layer Blend** -- Circuit + Algorithm crossfade per voice. Analog warmth meets digital precision.
3. **8 synthesis voices** -- Full drum kit in one engine (Kick/Snare/CHat/OHat/Clap/Tom/Perc/FX).
4. **115 factory presets** -- Strong preset library covering diverse drum styles.
5. **4 macros** -- MACHINE (circuit/algorithm balance), PUNCH (transient aggression), SPACE (reverb/room), MUTATE (randomization).

## Issues Preventing Higher Score

1. **No user-controllable LFO** -- The breathing LFO is hardcoded at 0.08 Hz. No user rate/depth/shape control. A user LFO modulating filter cutoff or pitch would add movement to sustained drum patterns.
2. **No mod matrix** -- XVC provides cross-voice modulation, but there is no traditional mod matrix for LFO/envelope routing.
3. **111 params** -- Massive parameter count makes the engine complex. Many params are per-voice-type, so the effective exposed count is manageable, but UI organization matters.

---

## Path to 9.0

1. Make breathing LFO rate user-controllable (add perc_breathRate param, 0.01-2 Hz). ~15 LOC.
2. Add a second LFO with rate/depth/shape for filter/pitch modulation. ~60 LOC.

## Path to 9.5

3. Add 4-slot mod matrix per voice type. ~120 LOC.

**Estimated effort to 9.0:** 75 LOC, 1 hour.
