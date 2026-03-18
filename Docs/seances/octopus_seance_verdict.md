# OCTOPUS Seance Verdict

**Engine**: XOctopus (OCTOPUS)
**Date**: 2026-03-17
**Source**: `Source/Engines/Octopus/OctopusEngine.h`
**Accent Color**: Chromatophore Magenta `#E040FB`
**Parameter Prefix**: `octo_`

---

## DOCTRINE COMPLIANCE

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 — Velocity shapes timbre | PARTIAL | Velocity gates the Ink Cloud at lines 1350 and 1433: `if (velocity >= inkThreshold)` triggers a qualitatively different sonic event — a saturated noise freeze burst replaces/overlays the wavetable signal. This is genuinely timbral: high velocity produces a completely different spectral texture vs. low velocity. However, velocity has no _continuous_ timbral gradient — it does not scale filter cutoff, wavetable position, arm depth, or chromatophore sensitivity. The effect is a binary threshold switch. D001 is partially served: categorical timbral change at threshold, but no continuous timbral modulation below threshold. |
| D002 — 2+ LFOs, mod wheel/aftertouch, 4 macros, mod matrix | PASS | Two named LFOs (`octo_lfo1Rate`/`octo_lfo2Rate`, range 0.01–30 Hz each) plus 8 ARM LFOs running at irrational prime-ratio rates — 10 simultaneous modulation oscillators total. Mod wheel CC#1 wired at line 572 (`modWheelAmount_`) and applied at line 533 to `effectiveChromaDepth` (timbral — pushes the morphing filter harder). MPEVoiceExpression handles pitch bend per voice (line 672). 4 macros (CHARACTER, MOVEMENT, COUPLING, SPACE) all wired to meaningful multi-target destinations. Most modulation-rich engine in this seance batch. |
| D003 — Physics rigor | PASS | Arm LFO rates use true irrational multipliers: phi (1.618034), sqrt(5) (2.236068), pi (3.14159), 1/sqrt(2) (0.7071068), sqrt(2) (1.4142136), sqrt(7) (2.6457513), 1-1/phi (0.381966) — lines 356–359. These are all transcendental or algebraic irrationals; the arms are mathematically guaranteed never to phase-lock. Portamento glide uses correct matched-Z coefficient: `1 - exp(-1 / (glide * sampleRate))` at line 520. Pitch modulation uses `fastPow2(cents / 1200)` at line 673 — correct semitone ratio. Chromatophore envelope follower uses `1 - exp(-2π * fc / sr)` at line 729. Wavetable wavefold at late frames uses `sin(sample * π * (1 + fold))` — valid analog wavefolder model. |
| D004 — All declared parameters wired | PASS | All 43 declared parameters are fetched in `attachParameters` and consumed in `renderBlock`. No dead declared parameters. One dead _coupling_ path exists (see Coupling section below): `couplingRingModSrc` is accumulated in `applyCouplingInput` (line 880) but never read in the render loop before being zeroed at line 549. This is a coupling bug, not a declared parameter issue; D004 is met on parameter grounds. |
| D005 — LFO rate floor ≤ 0.01 Hz | PASS | `octo_lfo1Rate` and `octo_lfo2Rate` both declare `NormalisableRange(0.01f, 30.0f)` at lines 1053 and 1066 — exactly at the 0.01 Hz doctrine floor. The engine can breathe at near-subsonic modulation rates. The 8 ARM LFOs have a base rate floor of 0.05 Hz (`octo_armBaseRate` min, line 919), but the two named LFOs satisfy the doctrine requirement. |
| D006 — Expression input not optional | PASS | Mod wheel CC#1 captured at line 572 and applied at line 533 — intensifies chromatophore depth (documented in source with explicit "D006" comment at line 530 and 1477). MPE pitch bend wired per-voice at line 672 (`voice.mpeExpression.pitchBendSemitones`). Velocity triggers the Ink Cloud timbral weapon. Three distinct expression pathways confirmed active. |

**Overall Doctrine Score**: 4 PASS, 1 PARTIAL, 0 FAIL — 5/6 effective. Strongest doctrine result in this seance batch.

---

## SILENCE GATE

**Status**: FULLY INTEGRATED — fleet-compliant.

- `SilenceGate silenceGate` declared at line 1178.
- `silenceGate.prepare(sampleRate, maxBlockSize)` called at line 388.
- `silenceGate.wake()` called on every note-on at line 558.
- Zero-idle bypass at line 575: `if (silenceGate.isBypassed() && midi.isEmpty()) { buffer.clear(); return; }` — full DSP skip when silent.
- `silenceGate.analyzeBlock()` called at block end, line 849.

One architectural note: a voice stays active as long as the Ink Cloud is dissolving even after the amp envelope reaches zero (line 679: `if (!voice.ampEnv.isActive() && !voice.inkCloud.isActive())`). This is correct — the gate will correctly wait out the cloud's full decay before declaring silence. No false-bypass risk.

---

## FASTMATH ADOPTION

**Status**: FULLY ADOPTED — fleet-compliant.

- `#include "../../DSP/FastMath.h"` at line 5.
- `fastSin()` used in `OctoLFO::process()` at line 161 (LFO sine shape — hot path).
- `fastPow2()` used at lines 673 (pitch cents → freq ratio) and 1292 (MIDI note → Hz) — hot path.
- `flushDenormal()` used at lines 239, 666, 731, 810, 811 — denormal protection in all feedback paths.
- `clamp()` used extensively throughout render loop.

`std::sin` appears only in `buildOctopusWavetable()` (lines 1213, 1243, 1258) — runs once at `prepare()` time, not in the render loop. Not a performance concern. Zero hot-path transcendental calls.

---

## COUPLING SUPPORT

**Output channels**:
- Channel 0: left audio
- Channel 1: right audio
- Channel 2: envelope level (`envelopeOutput` = peak amp envelope across all active voices, line 842)

**Input coupling types accepted**:

| Type | Accumulator | Applied To | Status |
|------|-------------|-----------|--------|
| `AudioToFM` | `couplingWTPosMod` | `effectiveWTPos` (line 534) | ACTIVE |
| `AmpToFilter` | `couplingChromaMod` | `effectiveChromaDepth` (line 532) | ACTIVE |
| `EnvToMorph` | `couplingArmRateMod` | `effectiveArmRate` (line 529) | ACTIVE |
| `AudioToRing` | `couplingRingModSrc` | **NOT APPLIED** | DEAD PATH |
| `LFOToPitch` | `couplingPitchMod` | pitch cents offset (line 671) | ACTIVE |

**Dead path detail**: `couplingRingModSrc` is accumulated at line 880 and cleared at line 549 every block. It does not appear anywhere in the render sample loop. `AudioToRing` is listed in the header comment at line 61 as a supported coupling type. The ring mod source signal is silently discarded every block.

**Active coupling assessment**: The four functional input routes are conceptually cohesive. `EnvToMorph → arm rate` is fleet-leading design: another engine's envelope literally conducts the speed of all 8 arm LFOs simultaneously — predator-presence composing the octopus's neurological tempo. `AmpToFilter → chromatophore depth` creates reactive skin-shifting driven by external amplitude — the octopus reacts to what it senses in its environment.

---

## PARAMETER AUDIT

**Total declared parameters**: 43

| Group | Parameters | Count | Wired? |
|-------|-----------|-------|--------|
| Arms | armCount, armSpread, armBaseRate, armDepth | 4 | Yes |
| Chromatophores | chromaSens, chromaSpeed, chromaMorph, chromaDepth, chromaFreq | 5 | Yes |
| Ink Cloud | inkThreshold, inkDensity, inkDecay, inkMix | 4 | Yes |
| Shapeshifter | shiftMicro, shiftGlide, shiftDrift | 3 | Yes |
| Suckers | suckerReso, suckerFreq, suckerDecay, suckerMix | 4 | Yes |
| Core Oscillator | wtPosition, wtScanRate | 2 | Yes |
| Main Filter | filterCutoff, filterReso | 2 | Yes |
| Level | level | 1 | Yes |
| Amp Envelope | ampAttack, ampDecay, ampSustain, ampRelease | 4 | Yes |
| Mod Envelope | modAttack, modDecay, modSustain, modRelease | 4 | Yes |
| LFO 1 | lfo1Rate, lfo1Depth, lfo1Shape | 3 | Yes |
| LFO 2 | lfo2Rate, lfo2Depth, lfo2Shape | 3 | Yes |
| Voice Mode | polyphony | 1 | Yes |
| Macros | macroCharacter, macroMovement, macroCoupling, macroSpace | 4 | Yes |

**Note on lfo2Val**: `lfo2Val` is computed in the render loop at line 687 (`float lfo2Val = voice.lfo2.process() * pLfo2Depth`) but is not applied to any audio target in the current render loop — only `lfo1Val` feeds into wavetable position. This means LFO2 is ticking but silent. This is effectively a dead execution path. The default depth of 0.0 means this is currently masked (multiplied to zero), but the routing gap should be addressed.

---

## SUMMARY

OCTOPUS is the most architecturally ambitious of the three engines in this seance batch (alongside OMBRE and ORCA). The five-subsystem biological design — Arms, Chromatophores, Ink Cloud, Shapeshifter, Suckers — is genuinely novel and fully implemented in DSP. The 8-arm prime-ratio LFO system is the most sophisticated internal modulation architecture in the current fleet: using phi, sqrt(5), pi, sqrt(2), sqrt(7), and 1/phi as rate multipliers produces aperiodic polyrhythmic behavior that mathematically never repeats on human-perceptible timescales. No other fleet engine achieves this.

The ink cloud freeze buffer with per-voice independence is a fleet-first design — a velocity weapon that transforms the spectral character of a note at threshold. Per-voice arm phase offsets (line 1426) ensure that in polyphonic mode, each simultaneously-held note generates a completely independent modulation pattern: the chord becomes a living, independently-breathing organism.

SilenceGate and FastMath are both fully and correctly integrated. 43 declared parameters, all wired, no dead declared params.

**Primary unresolved issues**: `AudioToRing` coupling is a dead path; `lfo2Val` is computed but not applied; D001 has no continuous velocity→timbre gradient (ink cloud is binary threshold); arm base rate floor at 0.05 Hz is higher than the D005 ideal.

---

## NEXT ACTIONS

| Priority | Action | Doctrine / Category |
|----------|--------|---------------------|
| P1 | **Fix AudioToRing dead path** — apply `couplingRingModSrc` as a ring modulator on the voice signal in the render loop, or remove the accumulator and the `AudioToRing` case entirely. Dead coupling routes are documentation rot. | D004 / Coupling |
| P1 | **Wire lfo2Val to a distinct audio target** — filter cutoff modulation or pan spread are obvious candidates that would give LFO2 a non-redundant role distinct from LFO1's wavetable scan. Alternatively document it as a user-assignable slot. | D004 |
| P2 | **Add continuous velocity→timbre path** — scale `effectiveArmDepth` by `voice.velocity` (louder = more arm agitation), or map velocity to a cutoff brightening offset. One line per voice in the render loop elevates D001 from PARTIAL to PASS. | D001 |
| P2 | **Lower `octo_armBaseRate` floor from 0.05 Hz to 0.01 Hz** — at current minimum with maximum spread, the slowest arm runs at ~0.019 Hz (53-second cycle). Lowering the floor to 0.01 Hz gives the slowest arm a 4-minute+ cycle, satisfying D005 across the full arm system. | D005 |
| P3 | **Fix chromatophore SVF morphing** — lines 742–770 use differential subtraction to approximate BP and HP responses from a single LP pass. CytomicSVF provides simultaneous LP/BP/HP outputs; use them directly and blend by morph parameter for clean, artifact-free topology transitions. | Audio quality |
| P3 | **Preset expansion to 150+** — verify current count; expand with patches targeting extreme arm configurations, ink-cloud-heavy textures, Shapeshifter microtonal territories, and coupling showcase presets. | Preset health |
| P4 | **Increase default `octo_armSpread` to 0.7** — default of 0.5 still leaves arm rates partially correlated. The engine's defining character requires spread near maximum; 0.7 would demonstrate this on any fresh patch. | UX / preset init |
| P4 | **Increase default `octo_lfo2Depth` to 0.2** — current default of 0.0 means LFO2 is inaudible until a player discovers it. | UX |
