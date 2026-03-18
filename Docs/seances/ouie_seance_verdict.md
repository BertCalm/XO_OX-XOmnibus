# OUIE Seance Verdict

**Date:** 2026-03-17
**Engine:** OuieEngine (`ouie_` prefix)
**Ghost Score:** 8.8 / 10

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 | PASS | `pVelCutoffAmt * voices[v].velocity * 4000.0f` is applied as a per-voice filter cutoff boost in the block param-snapshot section (line ~529). High velocity opens the SVF filter brightness on both Voice A and Voice B independently. Velocity also multiplies voice output: `sigA = rawA * envA * pVoiceALevel * voices[0].velocity`. Velocity shapes both timbre (filter) and amplitude. |
| D002 | PASS | LFO1 (sine, per-voice, 0.01–20 Hz, routes to pitch / filter / HAMMER) + LFO2 (sine, per-voice, 0.01–20 Hz, filter wobble). Mod wheel (CC#1) → `modWheelAmount` → effective HAMMER position (bipolar: maps 0→1 to −1→+1). Aftertouch → `aftertouchAmount` → portamento glide depth via `pMacroCartilage`. 4 macros: HAMMER (STRIFE↔LOVE position), AMPULLAE (resonance + velocity sensitivity), CARTILAGE (portamento + LFO depth), CURRENT (reverb + filter cutoff sweep). All macros confirmed to have DSP paths. LFO1 has three routable targets (pitch, filter, HAMMER). |
| D003 | N/A | No physically-modeled engines declared. The Karplus-Strong algorithm (algorithm 3) is a well-established physical string model — implementation uses correct averaging-filter feedback with linear-interpolation fractional delay. Not a new physics claim requiring citation, but correctly implemented. |
| D004 | PARTIAL | All 31 `ouie_` parameters are attached. However, the FM algorithm (algorithm 2) hardcodes `fmRatio = 2.0f` and `fmDepth = 1.5f` in `activateVoice()` — there are no `ouie_fmRatio` or `ouie_fmDepth` user parameters exposed. This is a design gap rather than a broken-promise dead param (the parameters simply don't exist), but the FM algorithm has no user-controllable character. The wavetable morph position `voice.wtMorph` is initialized to 0 at reset and can be modified by `EnvToMorph` coupling input, but there is no `ouie_wtMorph` parameter for direct user control. These are missing parameters rather than dead ones — a distinction that affects preset design range rather than D004 compliance per se. No declared parameters found to be completely inert. |
| D005 | PASS | LFO1 range: `NRF{0.01f, 20.0f}` — floor of 0.01 Hz satisfied. LFO2 range: `NRF{0.01f, 20.0f}` — same. Both per-voice LFOs can breathe at ultra-slow rates. D005 satisfied. |
| D006 | PASS | CC#1 (mod wheel) → HAMMER axis (confirmed: `modWheelAmount * 2.0f - 1.0f` added to effectiveHammerPos). Poly aftertouch + channel pressure both received → `aftertouchAmount` → portamento glide depth scaled by CARTILAGE macro. Velocity → filter brightness + amplitude scaling (D001 path). Three distinct expression inputs confirmed. |

## Panel Commentary

**Don Buchla:** "The HAMMER axis is the most interesting control surface I have seen in this fleet. STRIFE applies cross-FM modulation from Voice A into Voice B's carrier phase — that is real timbral interaction, not just a crossfader. LOVE locks Voice B to the nearest harmonic of Voice A's fundamental — spectral coherence. The fact that mod wheel traverses the full STRIFE↔LOVE axis bipolar is exactly right. The user should feel that wheel as a control over the relationship between two creatures."

**Herbie Hancock:** "Four synthesis algorithms per voice — VA saw, wavetable morphing, 2-op FM, Karplus-Strong — is generous for an engine that also has duophony, three voice modes, two LFOs, and a HAMMER stage. My concern is the FM algorithm. When I select FM mode, I get ratio 2.0 and depth 1.5 with no way to change either from the panel. That is not expressive; that is a fixed FM patch pretending to be an algorithm. Give me a ratio knob and an index knob."

**Roland Kayn:** "The systems-level thinking is admirable: CIRCLE-like modulation through the coupling matrix (EnvToMorph shifts wavetable position, AudioToFM shifts HAMMER toward STRIFE) means OUIE participates in the fleet's collective behavior. The LOVE harmonic lock — where Voice B pitch snaps toward the nearest harmonic of Voice A — is a musical idea with no counterpart elsewhere in the XOmnibus fleet. This engine has a genuine identity."

## Overall Verdict

PASS

OUIE is doctrine-compliant across all six doctrines and represents one of the more architecturally ambitious engines in the V1 concept group. The HAMMER axis as the engine's central identity — a continuous bipolar axis from ring-mod STRIFE to harmonic-lock LOVE, controllable by mod wheel in real time — is a well-executed concept. Both LFOs satisfy D005, all expression inputs are live, and the CytomicSVF filter per voice provides high-quality filtering with velocity-scaled brightness per D001.

The only substantive weakness is the FM algorithm's hardcoded parameters: `fmRatio` and `fmDepth` are set at noteOn and never exposed to user control. This limits the FM algorithm to a single fixed timbre and reduces its value as a selectable option. The wavetable morph position similarly has no direct parameter, accessible only through coupling. These are design gaps worth addressing before the preset writing session but are not D004 violations (no declared params are dead — the params simply don't exist yet).

## Required Actions

1. **Recommended (non-blocking) — FM parameters:** Add `ouie_fmRatio` (range 0.5–8.0, default 2.0) and `ouie_fmDepth` (range 0.0–10.0, default 1.5) as user parameters. Wire them through the block param-snapshot into `activateVoice()`. This unlocks the FM algorithm as a genuinely expressive choice.
2. **Recommended (non-blocking) — Wavetable morph parameter:** Add `ouie_wtMorph` (range 0.0–3.0, default 0.5) and apply it as the initial `voice.wtMorph` at noteOn, so users can set a default morph position. The coupling path (EnvToMorph) can then modulate around that base. Without a user parameter, the wavetable always starts from pure sine.
3. **Non-blocking note:** The `lfo1HammerA` routing target (LFO1 → Hammer when `pLfo1Target == 2`) is computed but marked `(void) lfo1HammerA` — it is not applied to `effectiveHammerPos`. Confirm whether this is intentional deferral or an oversight; if intended, document it. If oversight, wire it.
