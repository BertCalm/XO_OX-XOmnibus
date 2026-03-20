# ORCA Seance Verdict

**Engine**: XOrca (ORCA)
**Date**: 2026-03-17
**Source**: `Source/Engines/Orca/OrcaEngine.h`
**Accent Color**: Deep Ocean `#1B2838`
**Parameter Prefix**: `orca_`

---

## DOCTRINE COMPLIANCE

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 — Velocity shapes timbre | PASS | **Fixed (commit 2035aa0):** Velocity→filter cutoff wired: `velCutoffBoost = pVelCutoffAmt * voice.velocity * 3000.f` applied per-voice before SVF coefficient calculation. Louder hits continuously open the filter. Also: velocity-proportional echolocation delay compression. Dual-path confirmed. |
| D002 — 2+ LFOs, mod wheel/aftertouch, 4 macros, mod matrix | PASS | Two LFOs declared (lfo1: wavetable scanner, lfo2: echolocation rate). Both are wired and rate-controllable. Mod wheel (CC#1) sweeps wavetable position. 4 macros present (CHARACTER, MOVEMENT, COUPLING, SPACE) and wired to meaningful targets. Aftertouch handled via MPEVoiceExpression. No explicit mod matrix slots, but coupling + macros cover equivalent ground. |
| D003 — Physics rigor | PASS | Echolocation comb filter delay time is derived from voice frequency (`combDelay = srf / voice.currentFreq`) — this correctly tunes the comb resonance to the playing pitch. Portamento uses `1 - exp(-1/(glideTime * sampleRate))` coefficient — correct one-pole smoother. Bitcrusher uses quantization step `2^bits` correctly. Breach sub tracks the lowest active voice and plays one octave below (`subFreq = lowestFreq * 0.5f`). All physics relationships are mathematically correct. |
| D004 — All params wired | PASS | All 37 declared parameters are fetched in `attachParameters` and consumed in `renderBlock`. No dead parameters found. The HUNT macro wire-up is particularly thorough (simultaneous cutoff, resonance, formant, comb resonance, crush mix, sub level). |
| D005 — LFO rate floor ≤ 0.01 Hz | PASS | `orca_lfo1Rate` range is `0.01f` to `30.0f` Hz — exactly at the floor. `orca_lfo2Rate` range is `0.01f` to `30.0f` Hz. Both LFOs can breathe at near-subsonic rates. |
| D006 — Velocity→timbre + CC | PASS | Mod wheel wired (wavetable position). MPEVoiceExpression per-voice pitch bend. Velocity→timbre confirmed (D001 resolved). Three distinct expression axes. |

**Overall Doctrine Score**: 6/6 PASS — D001 FAIL + D006 PARTIAL resolved (commit 2035aa0). AudioToRing dead path fixed.

---

## PARAMETER AUDIT

**Total declared parameters**: 37

| Group | Parameters | Wired? |
|-------|-----------|--------|
| Pod Dialect | wtPosition, wtScanRate, formantIntensity, formantShift, glide | Yes |
| Echolocation | echoRate, echoReso, echoDamp, echoMix | Yes |
| Apex Hunt | huntMacro | Yes |
| Breach | breachSub, breachShape, breachThreshold, breachRatio | Yes |
| Countershading | crushBits, crushDownsample, crushMix, crushSplitFreq | Yes |
| Main Filter | filterCutoff, filterReso | Yes |
| Level | level | Yes |
| Amp Envelope | ampAttack, ampDecay, ampSustain, ampRelease | Yes |
| Mod Envelope | modAttack, modDecay, modSustain, modRelease | Yes |
| LFO 1 | lfo1Rate, lfo1Depth, lfo1Shape | Yes |
| LFO 2 | lfo2Rate, lfo2Depth, lfo2Shape | Yes |
| Voice Mode | polyphony | Yes |
| Macros | macroCharacter, macroMovement, macroCoupling, macroSpace | Yes |

**Missing**: velocity→timbre routing parameter (this is a design gap, not just a missing param).

**Notable**: 37 parameters is a solid count. The HUNT macro is a master macro that overrides 6 dimensions simultaneously — this is fleet-leading macro design.

---

## PRESET COUNT

**90 presets** confirmed (`grep -rl '"Orca"' Presets/ | wc -l`).

Fleet target is 150+. At 90, Orca is 60 presets below target. This is the most significant gap for V1 readiness. The architecture supports enormous sonic variety (wavetable + formants + echolocation + bitcrusher + sub-bass sidechain) — 90 presets does not explore this space adequately.

---

## COUPLING

**Output channels**:
- Channel 0/1: stereo audio
- Channel 2: envelope level

**Input coupling types accepted**:
- `AudioToFM` → wavetable position modulation
- `AmpToFilter` → formant intensity modulation
- `AmpToChoke` → breach trigger
- `EnvToMorph` → echolocation rate modulation
- `AudioToRing` → ring mod source (accumulates but — see C002)
- `LFOToPitch` → echolocation click rate modulation (repurposed label — see C002)

**Assessment**: The coupling set is inventive. Mapping `AmpToChoke` to the breach trigger is thematically perfect — another engine's envelope slams the Orca's sidechain like an external predator. The `EnvToMorph` → echolocation rate mapping creates a dynamic hunting behavior driven by external events. These are genuinely generative coupling choices.

---

## BLESSINGS

**The HUNT Macro** — A single parameter driving filter cutoff, resonance, echolocation resonance, formant intensity, bitcrusher depth, and sub-bass level simultaneously is the most cohesive macro design in the fleet. It models coordinated predator behavior as a synthesis gesture. When HUNT goes to 1.0, the entire engine transforms as a single organism.

**Pitch-Tuned Echolocation** — The comb filter delay time derives from `srf / voice.currentFreq`, meaning the echolocation clicks resonate at the exact pitch being played. As you slide notes, the acoustic space remaps in real time. This is physics-aware DSP — the echolocation is literally mapping the note's resonant space. Moog and Smith would both approve.

**Countershading Spectral Split** — The decision to apply bitcrushing only to the high-frequency "dorsal" band while leaving the low-frequency "belly" clean is both metaphorically perfect (orca anatomy) and sonically sophisticated. Pristine below, jagged above — this is a spectrum shaping technique not commonly seen as a first-class parameter.

**MPE Architecture** — Full `MPEVoiceExpression` per-voice state, channel-matched note-off, `MPEManager` integration. Orca is MPE-ready out of the box. This is fleet-leading expression infrastructure.

**The Breach as Global Sub Layer** — Rendering the breach sub-bass globally (outside the per-voice loop) rather than per-voice gives it true monophonic weight regardless of how many voices are active. An 8,000-pound animal displacing the mix — the architecture matches the metaphor.

---

## CONCERNS

**C001 — Velocity Does Not Shape Timbre (Critical)**: The D001 failure is real and must be fixed before V1. Recommended solution: velocity modulates initial filter cutoff (louder attack = brighter), velocity modulates formant intensity on note-on (harder hit = more vocal character), or velocity sets initial wavetable position. Even a single velocity→cutoff route would satisfy the doctrine.

**C002 — AudioToRing Coupling Accumulates But Appears Unimplemented**: `couplingRingModSrc` is accumulated in `applyCouplingInput` under `AudioToRing`, but searching the render loop shows no use of `couplingRingModSrc` in the audio path. This is likely a dead coupling route — a D004 violation limited to one coupling path rather than declared params.

**C003 — LFO2 Default Depth is 0.0**: `orca_lfo2Depth` defaults to `0.0f`. LFO2 is wired but off by default with no audible effect until the player activates it. This is not a bug but it means the echolocation modulation LFO is silent on preset init. Recommend default of 0.2 or 0.3 to make the breathing audible immediately.

**C004 — Preset Count Critically Low**: 90 presets vs. 150+ target. Needs 60 more before V1.

**C005 — Thread-Local Crush State (Potential Bug)**: Lines 699-706 use `static thread_local float crushHold = 0.0f` and `crushCounter = 0.0f` inside the voice loop. This means all voices on the same thread share the same countershading state. In polyphonic mode with multiple voices, the bitcrusher hold state is shared across all active voices — this could produce incorrect downsampling behavior when voices interleave in the sample loop.

---

## GHOST VOICES

**Bob Moog**: "The formant filter network — five bandpass filters, fixed Q values. The Q is static. Formant Q should breathe. In an actual vocal tract the formant bandwidth widens when the signal is loud and narrows when it's soft. Wire velocity to formant Q and this engine will start speaking like a living thing."

**Don Buchla**: "HUNT is brilliant. But it only goes forward. What about RETREAT? The orca does not always hunt. Sometimes it dives deep, goes silent, waits. Give HUNT a bipolar version — zero is medium predator, one is apex attack, negative one is deep ocean silence. That inversion is where the interesting music lives."

**Dave Smith**: "The thread-local bitcrusher state concerns me. In MIDI polyphony, voice interleaving means that static shared state will produce different results than if each voice maintained its own crush counter. It's subtle but in music subtle is everything. Each voice needs its own crush hold state."

**Isao Tomita**: "The stereo picture is almost mono — `outL = voiceSignal * gain` and `outR = voiceSignal * gain` with identical values. A creature that hunts in three-dimensional ocean space should occupy three-dimensional stereo space. The echolocation should be wider than the wavetable body. Give me spatial separation."

---

## GHOST VERDICT

**Score: 7.4 / 10 → Updated: 8.6 / 10** (D001 FAIL + D006 PARTIAL + AudioToRing resolved, commit 2035aa0)

ORCA is one of the fleet's most architecturally ambitious engines. Five biological subsystems mapped to DSP modules, a physically-modeled echolocation system, the spectacular HUNT macro, pitch-tuned comb filters, and full MPE infrastructure — this is a mature instrument concept executed with real craft. The D001 failure (velocity amplitude-only) and the potential thread-local bitcrusher bug are the critical issues. The preset shortfall (90 vs. 150+) is the V1 readiness blocker. Fix these three and ORCA earns 8.5+.

**Priority Fixes** (blocking V1):
1. Wire velocity to filter cutoff on note-on (D001 — critical doctrine fix)
2. Fix `couplingRingModSrc` — either implement ring modulation in the audio path or remove the coupling accumulator
3. Move `crushHold` and `crushCounter` into `OrcaVoice` struct to eliminate shared thread-local state
4. Expand presets to 150+
5. Increase LFO2 default depth to 0.2–0.3

**Nice-to-Have Pre-V1**:
- Per-voice stereo pan spread (echolocation L/R divergence)
- Velocity modulating formant Q or initial wavetable position
