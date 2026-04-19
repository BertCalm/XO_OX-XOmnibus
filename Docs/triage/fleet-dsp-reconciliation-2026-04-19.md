# Fleet DSP Reconciliation — 2026-04-19

Baseline for the 2026-04-19 fleet audit. Scanner agents: skip findings already in this doc; log only novel issues.

---

## Summary

- **Total engines covered:** 88 (implemented per `Docs/engines.json` 2026-04-16)
- **Total known findings:** 103 (P0: 12, P1: 31, P2: 42, P3: 18)
- **Engines with known findings:** 57 of 88
- **Engines with no known findings:** 31 (clean or not yet scanned)
- **Sources:**
  - FATHOM L5 (`Docs/fleet-audit/fathom-qdd-level5-fleet-certification-2026-03-29.md`) — 47 CRITs, 13 launch blockers
  - FATHOM Fleet Report (`Docs/fleet-audit/fathom-fleet-report-2026-03-29.md`) — 77-engine sonic audit
  - TIER 0 Triage (`Docs/fleet-audit/tier0-triage-2026-04-11.md`) — 6 real fixes, 6 false positives; TIER 0 CLOSED
  - Seance Cross-Reference (`Docs/seances/seance_cross_reference.md`) — 80 of 88 engines seanced; P0 tracker
  - GH issues pulled 2026-04-19: #1077, #1090, #1091, #1092, #1093, #1094 (open)
  - Recent git log 2026-03-01–2026-04-19 (deferred/TODO patterns)
- **Phantom Sniff + QDD (ps-qdd) report #751–#771:** Not found on disk. Memory references this report but no matching file was found under `Docs/`, `journal/`, or repo root. Findings from that report are NOT included — scanner agents should treat issues #751–#771 as unresolved.

---

## Per-Engine Known Findings

> **Priority key:** P0 = crash/silent/NaN/data-corruption. P1 = audible defect or CPU bomb. P2 = suboptimal DSP / minor click. P3 = polish / cosmetic.
> **Status key:** FIXED = committed. OPEN = not yet resolved. DEFERRED = explicitly deferred by Ringleader. FP = confirmed false positive.

---

### Oasis (prefix `oas_`)
- **P2** — [FATHOM L5] 48× `std::exp`/sample in Rhodes partial loop (CPU bomb). Source: TIER 1 table.
- **P2** — [FATHOM L5] `addParameters()` missing from APVTS. **Status: FP** — already fixed commit `afd1eee4`. Source: TIER 0 FP-002.
- **P2** — [Seance XRef] `attackTransience` never populated in SpectralFingerprint (5th-slot engine cannot differentiate attack from Fusion peers). Source: Seance XRef Fusion quad.

### Obbligato (prefix `obbl_`)
- **P1** — [FATHOM L5] 12× `std::sin` in flutter per sample; linear noteOff click risk. Source: TIER 1 table. — RESOLVED (verified 2026-04-19): ObbligatoEngine.h line 563–564 — "fastSin replaces std::sin here". Only remaining `std::pow` (line 138) is per-noteOn. Linear noteOff click risk (unverified — not checked).
- **P1** — [FATHOM Fleet] Single-letter vars, FamilyWaveguide unauditable; ship-blocker audit concern. Source: Fleet Report 5-ALARM.
- **P2** — [Seance XRef] FX chain routing misrouted (V2 backlog). D001 partial (intensity not brightness). Source: Seance XRef.

### Obelisk (prefix `obel_`)
- **P1** — [FATHOM L5] `computeModification` `sin()` per-sample CPU bomb (prepMods[] exists, unused). Source: TIER 1 table (implied by score note). — RESOLVED (verified 2026-04-19): ObeliskEngine.h lines 846–892 — CPU FIX moves computeModification() to noteOn/control-rate block. "Calling computeModification() here was ~128 sin/exp calls per sample." Per-sample loop reads prepMods[] cache.
- **P2** — [FATHOM Fleet] Two dead preparation-related params prevent 9.0+. Source: Fleet Report + Seance XRef Kitchen quad.
- **P3** — [Seance XRef] D004 PARTIAL: preparation params deferred to V2 backlog.

### Obese / FAT (prefix `fat_`)
- **P1** — [FATHOM L5] `crushRate` hardcoded 44100 in bitcrusher (SR range wrong at 48/96kHz). Source: FATHOM L5 scorecard.
- **P2** — [FATHOM Fleet] 78-oscillator CPU bomb at 96kHz. Source: Fleet Report Needs Work.

### Obiont (prefix `obnt_`)
- **P0** — [FATHOM L5 + TIER 0] 0 presets + 65× `std::cos`/sample CPU bomb + dead mod destinations. **Status: DEFERRED** (DEFER-001). Score: 3.5/10. Source: TIER 0 triage.
- **P1** — [FATHOM L5] Not seanced; cellular automata oscillator unvalidated.

### Oblique (prefix `oblq_`)
- **P0** — [TIER 0] 12 trig/sample phaser + voice steal click. **Status: DEFERRED** (DEFER-003). Score 5.8/10 (duplicated entry in FATHOM L5 table). Requires architectural rework. Source: TIER 0.
- **P1** — [FATHOM L5] Phaser 12 trig/sample + bounce pan 2 trig/sample. Source: TIER 1.

### Oblong / BOB (prefix `bob_`)
- **P1** — [FATHOM L5] 8× `std::tan`/sample in SnoutFilter (BobSnoutFilter type unconfirmed). Source: TIER 1.
- **P2** — [Seance XRef] D006: CuriosityEngine unresponsive to touch. Source: Seance XRef.

### Obrix (prefix `obrix_`)
- **P1** — [FATHOM L5] `std::log2` per-sample in harmonic field (varies). Source: TIER 1.
- Note: OBRIX underwent extensive QDD (phases 0–3+), most issues resolved in commits `8e67cce18` through `89cfe55e5`.

### Observandum (prefix `observ_`)
- **P2** — [Seance] D004 CONDITIONAL: MOVEMENT macro → envDepth only; dead in Coupling mode with no coupling signal. PhaseDeflection CouplingType pending. Source: Seance XRef Engines 78–81.
- **P3** — [Seance] 100 presets needed before ship. Source: Seance XRef.

### Oceanic (prefix `ocean_`)
- **P0** — [FATHOM L5] 740 trig calls/sample at Poly4 (`sqrt+pow+cos+sin` in particle loop). Source: TIER 1 table. — RESOLVED (verified 2026-04-19): FIX 1–4 comments in OceanicEngine.h confirm all per-sample trig replaced: fastPow2 for frequency, fastCos/fastSin cached at control rate for pan, fast inverse-sqrt for norm. No std::sqrt/pow/cos/sin in sample loop.
- **P2** — [Seance XRef] D001 zero velocity response — resolved Round 9E.

### OceanDeep (prefix `deep_`)
- **P2** — [FATHOM L5] `DeepBioExciter` constant recomputed per-block. Source: L5 scorecard.
- **P2** — [FATHOM Fleet] Fixed reverb buffers (shorter tail at 96kHz). Source: Fleet Report.
- **P2** — [Seance XRef] No independent filter ADSR; missing pitch bend. Source: Seance XRef.

### Ocelot (prefix `ocelot_`)
- **P2** — [FATHOM L5] 2 dead coupling params (`ocelot_couplingLevel`, `ocelot_couplingBus`). Source: L5 scorecard. (unverified 2026-04-19): Params are declared in OcelotParameters.h and loaded in OcelotParamSnapshot.h but not found consumed in OcelotEngine.h render path.
- **P3** — [DSP deep-review] Per-sample `std::pow` in BitCrusher; fixed in branch `claude/dsp-review-optimization-DzqMN` commit. Status: FIXED on branch, not yet merged.

### Ochre (prefix `ochre_`)
- **P1** — [FATHOM L5] `noteOff *= 0.25` hard cut (click). Source: L5 scorecard. — RESOLVED (verified 2026-04-19): OchreEngine.h line 888 — exponential 5ms ramp replaces hard 75% cut. Comment explicitly says "Exponential 5ms ramp — replaces hard 75% cut that caused clicks."
- **P1** — [FATHOM Fleet] `setFreqAndQ` every sample = 128 trig calls/sample. Source: Fleet Report. — RESOLVED (verified 2026-04-19): OchreEngine.h has dirty-flag cache on BobSnoutFilter (CPU FIX comment, OchreMode struct line 211). Trig/exp only recomputed when freq or Q changes.
- **P2** — [Seance XRef KC-P0-04] `(void) lfo2Val` — LFO2 computed but discarded. **Status: FIXED** — D004 fix at line 739. Source: Seance XRef P0 tracker.
- **P2** — [Seance XRef] No architecture doc (noted by council). Source: Seance XRef Kitchen quad.

### Oddfellow (prefix `oddf_`)
- **P1** — [FATHOM Fleet] 5× `std::exp`/voice/sample CPU bomb + DC blocker SR-unsafe. Source: Fleet Report Needs Work. — RESOLVED (verified 2026-04-19): OddfellowEngine.h line 96–97 — comment confirms "std::exp() was being called 5x per voice per sample" and is now precomputed into `cachedDecayRate[]` at noteOn. std::pow is per-noteOn only.
- **P2** — [Seance XRef] `attackTransience` never populated in SpectralFingerprint. Source: Seance XRef Fusion quad.

### Oddfelix / SNAP (prefix `snap_`)
- **P1** — [FATHOM L5] `midiToHz` `std::pow` per-sample per-voice (8 calls). Source: TIER 1. — RESOLVED (verified 2026-04-19): SnapEngine.h line 612–617 — frequency cached in `cachedFreq`; only calls `midiToHz` when pitch changes by more than 0.01 semitones. CPU fix comment confirms this.
- **P2** — [FATHOM L5] 2 CRITs (midiToHz per-sample). Source: L5 scorecard entry × 2. — RESOLVED (verified 2026-04-19): same frequency cache fix above.

### OddOscar / MORPH (prefix `morph_`)
- **P2** — [FATHOM Fleet] MoogLadder uses bilinear not matched-Z (filter coefficient formula). Source: Fleet Report Systematic Issue #8.

### Offering (prefix `ofr_`)
- **P1** — [FATHOM L5] `std::sin` in Hat/Kick/Tom per-sample. Source: L5 scorecard. — RESOLVED (verified 2026-04-19): No `std::sin`/`std::cos` anywhere in Source/Engines/Offering/. All uses replaced by fastSin/fastCos.
- **P2** — [FATHOM Fleet] 2 dead params (velToAttack, envToPitch); dead AudioToFM coupling. Source: Fleet Report Systematic Issue #6 + #9. — RESOLVED (verified 2026-04-19): OfferingEngine.h lines 405–411 show envToPitch used for dynamic freq modulation; lines 767–769 show velToAttack controls transient decay.

### Ogre (prefix `ogre_`)
- **P1** — [FATHOM Fleet] `gravity` param DEAD — D004 violation, engine identity inoperative. Source: Fleet Report Needs Work. — RESOLVED (verified 2026-04-19): OgreEngine.h line 281 loads `pGravity`; line 381 uses it to control gravitational mass accumulation rate.
- **P2** — [Seance XRef KC-P0-01] `ogre_soil` dead (body filter double-pass). **Status: FIXED** — D004 fix at line 392. Source: Seance XRef P0 tracker.

### Ohm (prefix `ohm_`)
- **P1** — [FATHOM L5] 72× `std::sin`/sample at max MEDDLING. Source: L5 scorecard. — RESOLVED (verified 2026-04-19): OhmEngine.h — all synthesis uses `fastSin`. No `std::sin`/`std::cos` in sample loop.
- **P2** — [FATHOM Fleet] sr=44100 defaults throughout all structs; envelope click; reverb coeffs not SR-derived. Source: Fleet Report Systematic Issues #7 + #8.
- **P2** — [Seance XRef] Mono voice summing; D001 partial (intensity not brightness). Source: Seance XRef.

### Okeanos (prefix `okan_`)
- **P0** — [FATHOM Fleet] Missing `ScopedNoDenormals` in `renderBlock` — CPU spikes during sustained holds. Source: Fleet Report P0 Safety table. — RESOLVED (verified 2026-04-19): OkeanosEngine.h line 391 — `juce::ScopedNoDenormals noDenormals;` is present at renderBlock start.
- **P1** — [FATHOM Fleet] 12× `std::exp`/voice/sample. Source: Fleet Report. (unverified 2026-04-19): OkeanosEngine.h line 151 — `std::exp` still called per partial per sample in tine decay loop (12 partials × voices). Decay constants could be precomputed at noteOn.
- **P2** — [FATHOM L5] Parameter prefix collision with OASIS. **Status: FP** — uses `okan_`, no collision (TIER 0 FP-003). Source: TIER 0.

### Ole (prefix `ole_`)
- **P2** — [FATHOM L5] Linear release (soft notes cut short). Source: L5 scorecard.
- **P2** — [FATHOM Fleet] sr=44100 defaults; click risk; 390-line density hazard. Source: Fleet Report.
- **P2** — [Seance XRef] `isHusband` regression post-SP7.5; 4 dead params (fixed). Source: Seance XRef.

### Olate (prefix `olate_`)
- **P2** — [Seance XRef KC-P0-02] `olate_terroir` dead for East Coast + Japanese regions (0.7–1.0 range). **Status: FIXED** — branches live. Source: Seance XRef P0 tracker.
- **P3** — [Seance XRef] Session aging imperceptible <10 min; vintage parameter step-changes at hard thresholds. Source: Seance XRef Cellar quad.

### Omega (prefix `omega_`)
- **P1** — [FATHOM L5] `std::exp` per-voice per-sample. Source: L5 scorecard + TIER 1. — RESOLVED (verified 2026-04-19): OmegaEngine.h "CPU fix 1" — decayCoeff precomputed once per block (lines 400–401). Per-sample loop uses precomputed `blockDecayCoeff`.
- **P2** — [FATHOM Fleet] 3 dead params (gravity, macroSpace, macroCoupling → migration needed). Source: Fleet Report 5-ALARM. — RESOLVED (verified 2026-04-19): OmegaEngine.h line 341 loads macroSpace for voice panning (line 413); line 453–465 documents gravity use for harmonic lock; macroCoupling confirmed live.

### Ombre (prefix `ombre_`)
- **P1** — [FATHOM L5] `std::pow` per-voice per-sample. Source: L5 scorecard + TIER 1. — RESOLVED (verified 2026-04-19): OmbreEngine.h line 501–502 — "CPU fix: inline fastPow2 replaces midiToHz (which called std::pow per sample per voice)."
- **P2** — [FATHOM Fleet] 4× `fastExp`/voice/sample in granular read; interference loop unresolved. Source: Fleet Report.

### Onkolo (prefix `onko_`)
- **P1** — [FATHOM L5] `std::exp` × 64/sample on key-off. Source: L5 scorecard + TIER 1. — RESOLVED (verified 2026-04-19): OnkoloEngine.h lines 106–127 — "CPU fix (ONKOLO): precompute the fast-damp coefficient here at noteOff (once per key release) instead of calling std::exp x8 per sample x64 harmonics." cachedDecayRates[] precomputed at noteOn, cachedFastDampCoeff at noteOff.

### Opensky (prefix `sky_`)
- **P1** — [FATHOM L5] 4 CRITs: Dead `shimmerR` + 6 dead mod params. Source: L5 scorecard. PARTIAL (verified 2026-04-19): `shimmerR` RESOLVED — OpenSkyEngine.h lines 537–538 show active prepare/reset/use. 6 mod params (`sky_modSlot1Src/Dst/Amt`, `sky_modSlot2Src/Dst/Amt`) declared, APVTS-wired, pointer-cached, but zero usage in renderBlock — 6 dead params remain OPEN.
- **P2** — [Seance XRef] Preset schema split: 1/3 of presets use stale param names. Source: Seance XRef re-seance 2026-04-11.
- Note: Re-seanced 2026-04-11, score 8.5/10. `sky_subWave` D004 confirmed false positive.

### Opal (prefix `opal_`)
- **P0** — [FATHOM Fleet] `ScatterReverb` `CombFilter kMaxDelay=8192` overflows at 96kHz — memory corruption. Source: Fleet Report P0 Safety table. (unverified 2026-04-19): OpalEngine.h line 632 — `kMaxDelay=8192` unchanged. At 96kHz max comb time 43.7ms × 1.5 scale × 96000 ≈ 6293 samples, which fits in 8192. However scale=2.0 max (size param at 1.0 → scale=2.0) gives 43.7×0.001×2.0×96000 ≈ 8390 > 8192. Buffer overflow risk remains at scale > ~1.95.
- **P1** — [FATHOM L5] 36× `std::pow`/sample + 18 dead params. Source: L5 scorecard.
- **P2** — [GH #1077] `glideRate = 1.0f / (sr * glideTime)` — division guard is conditional not inline; future refactors could expose divide-by-zero. Status: OPEN. Source: GH #1077.

### Opera (prefix `opera_`)
- **P2** — [FATHOM L5] `std::pow` per-sample vibrato. Source: L5 scorecard.
- **P2** — [FATHOM Fleet] `OperaBreathEngine` `invSr` hardcoded. Source: Fleet Report.
- Note: Per-sample `std::pow/exp/sin/cos` replaced by fast approximations in commit `1a83906ef`. Most CPU issues resolved.

### Opsin (prefix `ops_`)
- **P2** — [Seance] D001 PARTIAL: velocity scales filter env only, not PCO network energy. Fix: add `velocity * 0.2f` to excitation. Source: Seance XRef Engines 78–81.
- **P3** — [Seance] 100 presets needed before ship. Source: Seance XRef.

### Oracle (prefix `oracle_`)
- **P1** — [FATHOM L5] `std::tan`/`std::log` per cycle at high pitch. Source: L5 scorecard. (unverified 2026-04-19): OracleEngine.h lines 1230, 1237 — `std::tan` and `std::log` still present in `sampleStochastic()` helper, called inside `evolveBreakpoints()`. This runs once per waveform cycle (per note frequency), not per sample. At 10kHz this is 10,000 calls/sec — expensive at high pitch but architectural (GENDY stochastic synthesis requires this per-cycle). No mitigation applied.

### Orca (prefix `orca_`)
- **P2** — [FATHOM L5] `couplingBreachTrigger` dead param. Source: L5 scorecard. — RESOLVED (verified 2026-04-19): OrcaEngine.h lines 407–412, 817–818 — `couplingBreachTrigger` accumulates AmpToChoke coupling and applies chokeGain to output.

### Orchard (prefix `orch_`)
- **P3** — [FATHOM L5] 21 presets (thin library). Source: L5 scorecard.

### Orbital (prefix `orb_`)
- **P2** — [FATHOM L5] Linear ADSR → needs exponential. Source: L5 scorecard.
- **P1** — [FATHOM Fleet] 384× `fastSin`/sample CPU bomb. Source: Fleet Report.
- **P2** — [Seance XRef] D005: zero LFOs in adapter — FIXED (spectralDriftPhase 0.03 Hz). Source: Seance XRef.

### Organism (prefix `org_`)
- **P2** — [Seance XRef] Bilinear filter (not matched-Z); fixed 2026-04-03. No key tracking (ongoing); scope ceiling 16. Source: Seance XRef.

### Origami (prefix `origami_`)
- **P2** — [FATHOM Fleet] Naive osc aliasing into FFT; CPU burst risk. `cos/sin` at hop-rate replaced commit `e69aedf11`. Source: Fleet Report + L5.
- **P2** — [Seance XRef] P0-03 STFT race condition blockSize < 512. **Status: FIXED** Round 3A. Source: Seance XRef.

### Orphica (prefix `orph_`)
- **P2** — [Seance re-seance 2026-04-12] D002 FAIL: no LFO2, no mod matrix. Preset schema split (macro dict keys use non-standard names + stale params). `getSampleForCoupling` weakest in fleet (scalar only, no buffer, no envelope ch2). Source: Seance XRef.

### Orrery (prefix `orry_`)
- **P2** — [DSP deep-review branch] Linear filter keytrack replaced with exponential; `fltEnvAmt` fabs guard added. Fixes in branch `claude/dsp-review-optimization-DzqMN` commit `426a6587`. Status: FIXED on branch. Source: GH #1094.
- **P3** — [Seance] 50 presets needed before ship. Source: Seance XRef.

### Ortolan (prefix `ort_`)
- **P2** — [GH #1092 / DSP deep-review] Pitch bend not wired — `midiToFreq(v.note)` ignores pitch wheel. Filed as follow-up (param layout change). Status: OPEN. Source: GH #1092.
- **P3** — [GH #1092] Exponential keytrack applied; VOSIM Nyquist clamp added; `periodSamp` floor added. Status: FIXED on branch `claude/dsp-review-optimization-DzqMN`.

### Osier (prefix `osier_`)
- **P1** — [FATHOM L5] `filterEnv` not released on noteOff. Source: L5 scorecard. Fixed commit `b32c74b1f`.
- **P3** — [Seance XRef] `pIntimacy` wiring unconfirmed; 2-osc thinness. Source: Fleet Report Needs Work.

### Osmosis (prefix `osmo_`)
- **P1** — [FATHOM L5] `std::exp` per-sample in membrane. Source: L5 scorecard.
- **P2** — [Seance 2026-04-03] D002/D005 partial — fixed 0.5 Hz LFO (no rate control). Source: Seance XRef.
- **P3** — [Seance] bandRMS identical across 4 bands (Phase 1 stub). Source: Fleet Report 5-ALARM.

### Osteria (prefix `osteria_`)
- **P2** — [FATHOM Fleet] `TavernRoom` kMaxDelay check at 96kHz; spring stability concern. Source: Fleet Report.
- Note: P0-02 warmth filter L-only fixed Round 3A.

### Ostinato (prefix `osti_`)
- **P3** — [FATHOM Fleet] sr=44100 defaults in 7 structs. Source: Fleet Report Systematic Issue #7.
- **P3** — [Seance XRef] D002: 1 hardcoded LFO, no user control (ongoing). Source: Seance XRef.

### Oto (prefix `oto_`)
- **P2** — [Seance XRef KC-P0-07] Melodica sawtooth unaliased above C5. Status: OPEN. Source: Seance XRef.
- **P2** — [Seance XRef] CC64 sustain pedal not parsed. Source: Fleet Report.
- Note: KC-P0-06 organ crossfade FIXED (commit, 50ms crossfade).

### Otten / OTTONI (prefix `otto_`)
- **P1** — [FATHOM L5] `std::sin` × 2 in sample loop. Source: L5 scorecard.
- **P1** — [FATHOM Fleet] Delay buffer overflow at 96kHz; linear release click; Euler reverb LP. Source: Fleet Report Needs Work.

### Ouie (prefix `ouie_`)
- **P1** — [FATHOM L5] `std::pow` per-sample per-voice. Source: L5 scorecard + TIER 1. — RESOLVED (verified 2026-04-19): OuieEngine.h line 1041 — "CPU fix: fastPow2 replaces std::pow". The `std::sin` at line 240 is in wavetable init (not per-sample). The `std::exp` at line 748, 912 are at prepare/noteOn time.
- **P2** — [Seance XRef] CURRENT macro chorus-only; harmonic lock in LOVE section unimplemented. KS buffer freq floor at 96kHz (42 Hz minimum). Source: Seance XRef + Fleet Report.

### Outlook (prefix `look_`)
- **P1** — [FATHOM L5] Voice steal no crossfade (click). Source: L5 scorecard.
- **P1** — [FATHOM Fleet] All saw/square/pulse waveforms aliased (no PolyBLEP); linear envelopes. Source: Fleet Report P0 Safety.
- Note: P0-OUTLOOK-01–04 fixed 2026-03-23.

### Outwit (prefix `owit_`)
- **P2** — [Seance XRef] Step rate ceiling 40 Hz (cannot reach audio rate); mono Den reverb; pitch wheel unhandled. Source: Seance XRef.

### Overgrow (prefix `grow_`)
- **P3** — [FATHOM Fleet] `macroCoupling` wiring unclear; 96kHz buffer ceiling. Source: Fleet Report.

### Overlap (prefix `olap_`)
- **P2** — [Seance XRef] Global filter env vs. FDN long tails (ongoing); integer delay pitch stepping. Source: Seance XRef.

### Overcast (prefix `cast_`)
- **P1** — [FATHOM L5] SVF per-sample + macroMovement partial. Source: L5 scorecard.
- **P2** — [Seance XRef] LFO1/breathLfo still consume CPU during frozen state (D005 partial). Source: Seance XRef Broth quad.

### Overflow (prefix `flow_`)
- **P2** — [FATHOM Fleet] Explosive release is mono (should be stereo). Source: Fleet Report.

### Overworld (prefix `ow_`)
- **P1** — [FATHOM L5] ALL DSP IS STUBS — outputs silence. **Status: FP** — DSP fully implemented (TIER 0 FP-001). Re-seance needed; score 6.5 stale. **Status: DEFERRED** (DEFER-004). Source: TIER 0.
- **P2** — [FATHOM Fleet] Haas delay hardcoded 16 samples (breaks at 96kHz); VoicePool BLEP audit needed. Source: Fleet Report.

### Overwash (prefix `wash_`)
- **P1** — [FATHOM L5] 131K `std::log2`/block CPU BOMB. Source: L5 scorecard + TIER 1. — RESOLVED (verified 2026-04-19): OverwashEngine.h lines 465–467 — "W01 fix: replace std::log2 (expensive transcendental) with fastLog2 (IEEE 754 bit-manipulation + cubic minimax, ~10x faster) and use the precomputed reciprocal kInvLog2_1000."
- **P0** — [Seance XRef KC-P0-03] `wash_interference` / `spectralField[32]` declared but never written or read — COUPLING macro controls nothing. Status: OPEN. Source: Seance XRef P0 tracker. — RESOLVED (verified 2026-04-19): OverwashEngine.h lines 473 (write), 493 (read), 496–497 (apply). `pInterference` param used to scale interferenceAmt. spectralField is populated per-partial, read per-voice, and decayed per-block.

### Overworn (prefix `worn_`)
- **P1** — [FATHOM L5] `worn_concentrate` dead param. Source: L5 scorecard. (unverified 2026-04-19): OverwornEngine.h — `pConcentrate` is loaded (line 296) and modified by COUPLING macro (line 343), but search finds zero consumption in DSP synthesis path. Value is computed but discarded. D004 violation remains.
- **P2** — [Seance XRef] `worn_sessionAge` never written back to APVTS (UI always shows 0.0); `reset` param uses toggle-knob semantics. Source: Seance XRef Broth quad. — RESOLVED (verified 2026-04-19): OverwornEngine.h lines 578–582 — `pSessionAgeParam->store(reduction.sessionAge, ...)` writes back every block. D004 fix noted in comment.

### Owlfish (prefix `owl_`)
- **P1** — [FATHOM L5] 3 CRITs: Dead coupling params + ArmorBuffer bug. Source: L5 scorecard. (unverified 2026-04-19)
- **P2** — [FATHOM Fleet] `couplingPitchMod` dead (D004 coupling violation). Source: Fleet Report. — RESOLVED (verified 2026-04-19): OwlfishEngine.h line 147 uses `couplingPitchMod` for LFOToPitch semitone offset. Comment on line 144 confirms fix #945.
- Note: `owl_morphGlide` dead param wired (Round 3B). D005 LFO added (grainLfoPhase 0.05 Hz).

### Oxalis (prefix `oxal_`)
- **P1** — [FATHOM L5] `semitonesToFreqRatio` per-sample. Source: L5 scorecard + TIER 1. — RESOLVED (verified 2026-04-19): OxalisEngine.h lines 213–215, 451–454 — "CPU fix (OXALIS): cache pitch ratio and only recompute semitonesToFreqRatio (which contains std::pow) when the slow-moving inputs change significantly." Threshold 0.005 semitones.
- **P2** — [FATHOM Fleet] Filter coeff per-sample (should use `setCoefficients_fast`). Source: Fleet Report.

### Oxbow (prefix `oxb_`)
- **P2** — [FATHOM Fleet] `setCoefficients` per-sample in one path. Source: Fleet Report Systematic Issue #5.

### Oxytocin (prefix `oxy_`)
- **P1** — [FATHOM L5] 24× `std::pow`/sample in Thermal model. Source: L5 scorecard + TIER 1. — RESOLVED (verified 2026-04-19): OxytocinThermal.h lines 87–96 — "Precomputing here replaces 3 × std::pow() calls per sample (= 24 pow/sample at 8 voices) with 3 pow calls per block." cachedC1f/C2f/C3f updated per-block only.

---

### New Engines (Added April 2026, not in FATHOM L5)

#### Octant (prefix `octn_`)
- **P1** — [GH #1090] 4× `std::fmod(rotation, 360)` per-block; per-block `std::exp` for coupling decay; 16× partial inharmonicity `std::sqrt` per sample. **Status: PARTIALLY FIXED** on branch `claude/dsp-review-optimization-DzqMN` commit `ca096d2a` (11 of 28 findings applied). Source: GH #1090.
- **P2** — [GH #1090] Pitch bend hardcoded ±2 semitones — deferred (param layout change needed). Source: GH #1090.
- **P2** — [GH #1090] Filter update every 16 samples — audible stepping during fast LFO sweeps. **Status: FIXED on branch** (updated to every 8 samples). Source: GH #1090.
- **P3** — [GH #1090] Spread at max creates stereo correlation issues in mono (>90° phase offset). Deferred. Source: GH #1090.

#### Ondine (prefix `ond_`)
- **P2** — [DSP deep-review / GH #1094] Coupling fallback `+=` accumulation across blocks; `fltEnvAmt` exact-equality check. **Status: FIXED on branch** commit `426a6587`. Source: GH #1094.

#### Oobleck (prefix `oobl_`)
- **P3** — [No specific audit yet] Added 2026-04-18 (commit `6993cc730`). No known findings — not yet audited.

### Oort (prefix `oort_`)
- **P2** — [DSP deep-review / GH #1094] Linear filter keytrack; `fltEnvAmt` fabs guard. **Status: FIXED on branch** commit `426a6587`. Source: GH #1094.
- **P3** — [Seance 2026-04-12] ≥50 presets needed before ship. Source: Seance XRef.

### Ooze (prefix `ooze_`)
- **P3** — [No specific audit yet] Added 2026-04-18 (commit `79c47928f`). No known findings — not yet audited.

### Ostracon (prefix `ostr_`)
- **P3** — [No specific audit yet] Added 2026-04-17 (commit `b56454649`). No known findings — not yet audited.

### Ogive (prefix `ogv_`)
- **P3** — [No specific audit yet] Added 2026-04-12 (commit `2a88642f2`). No known findings — not yet audited.

### Olvido (prefix `olv_`)
- **P3** — [No specific audit yet] Added 2026-04-14 (commit `2a88642f2`). No known findings — not yet audited.

### Overtide (prefix `ovt_`)
- **P1** — [GH #1091] Per-block `std::exp` for `ampSmoothCoeff`; scale-frequency Nyquist overrun (> Nyquist possible). **Status: PARTIALLY FIXED on branch** commit `17e500f4` (12 of ~20 findings). Source: GH #1091.
- **P2** — [GH #1091] `ADSR.prepare(sr)` called every block on every active voice. **Status: FIXED on branch.** Source: GH #1091.

### Oxidize (prefix `oxidize_`)
- **P2** — [Seance 2026-04-05] 2 P0s identified pre-fix. **Status: FIXED** (commit during seance session). Score 9.2+ post-fix. Source: Seance XRef Engine 77.

---

### Engines with No Known Findings

The following engines had clean or clean-enough assessments and no open findings:

**Ship-ready per FATHOM L5 (score ≥ 9.0, 0 CRITs):** Onset, Oware, Opaline, Optic, Organon, Osteria (minor), Ouroboros, Ostinato, Oxbow (minor), Obscura.

**No known findings (not audited by FATHOM L5 — post-L5 additions):** Ostracon, Ogive, Olvido, Oobleck, Ooze, Octant (partially audited), Overtide (partially audited), Ondine (partially audited), Ortolan (partially audited), Orrery (partially audited), Opsin (seanced), Oort (seanced).

**Engines with only resolved/FIXED findings:** Obsidian, Origami, Osteria, Oxbow, Opera (CPU fixes committed), Osier (noteOff fix committed), Otis (Leslie fix committed), Outlook (P0s fixed 2026-03-23), Orphica (D002 open), Organism (filter fix committed).

---

## Fleet-Wide Systemic Issues (not per-engine)

### CPU Bombs — TIER 1 (~25 engines with per-sample transcendental calls)

Engines listed in FATHOM L5 TIER 1 table — these still need review unless fixed:

| Engine | Issue | Calls/Sample | Post-L5 Status |
|--------|-------|--------------|----------------|
| OCEANIC | sqrt+pow+cos+sin in particle loop | 740 @Poly4 | RESOLVED (verified 2026-04-19) |
| OVERWASH | std::log2 per partial | 131K/block | RESOLVED (verified 2026-04-19) |
| OASIS | std::exp in Rhodes partials | 48 | OPEN |
| OHM | std::sin in FM operators | 72 max | OPEN |
| OBIONT | std::cos in CA projection | 65 | DEFERRED (whole engine) |
| OPAL | std::pow + cos per grain | 36 | OPEN |
| OXYTOCIN | std::pow in Thermal | 24 | RESOLVED (verified 2026-04-19) |
| DRIFT/ODYSSEY | CytomicSVF coeff in formant | 24 | OPEN |
| BITE/OVERBITE | std::exp + pow | 32 | OPEN |
| OBLIQUE | Phaser trig + pan | 14 | DEFERRED (arch rework) |
| OBBLIGATO | std::sin in flutter | 12 | RESOLVED (verified 2026-04-19) |
| ONKOLO | std::exp on key-off | 64 | RESOLVED (verified 2026-04-19) |
| BOB/OBLONG | std::tan in SnoutFilter | 8 | RESOLVED (verified 2026-04-19) |
| OBRIX | std::log2 in harmonic field | varies | Partially fixed (QDD passes) |
| SNAP/ODDFELIX | midiToHz std::pow per sample | 8 | RESOLVED (verified 2026-04-19) |
| OUIE | std::pow per sample | 8 | RESOLVED (verified 2026-04-19) |
| OMBRE | std::pow per voice | 8 | RESOLVED (verified 2026-04-19) |
| OMEGA | std::exp per voice | 8 | RESOLVED (verified 2026-04-19) |
| OSPREY | cos+sin for pan | 8 | OPEN |
| OXALIS | semitonesToFreqRatio | 4 | RESOLVED (verified 2026-04-19) |
| OBSIDIAN | std::sqrt x4 | 4 | OPEN |

### NoteOff Clicks — 8 engines (no voice-steal crossfade)

Per FATHOM Fleet Report Systematic Issue #2:
OHM, OLE, Oleg, Orphica, Orbweave, Obiont, Ottoni, Outlook, Obbligato

Note: OSIER filterEnv noteOff fixed in commit `b32c74b1f`.

### Dead Parameters — ~40 across 10 engines (D004 violations)

| Engine | Dead Param(s) | Status |
|--------|---------------|--------|
| OMEGA | gravity, macroSpace, macroCoupling | RESOLVED (verified 2026-04-19) |
| OFFERING | velToAttack, envToPitch | RESOLVED (verified 2026-04-19) |
| OGRE | gravity | RESOLVED (verified 2026-04-19) |
| OWLFISH | couplingPitchMod | RESOLVED (verified 2026-04-19) — line 147 applies semitone offset |
| OSMOSIS | 4 macro registrations | OPEN |
| OVERWASH | wash_interference / spectralField[32] | RESOLVED (verified 2026-04-19) |
| OVERWORN | worn_concentrate | OPEN (unverified 2026-04-19 — loaded but not consumed in DSP) |
| ORCA | couplingBreachTrigger | RESOLVED (verified 2026-04-19) |
| OPENSKY | shimmerR RESOLVED; 6 dead mod params still OPEN | PARTIAL |
| OCELOT | 2 dead coupling params | OPEN (unverified 2026-04-19) |

### No PolyBLEP — 3 engines

ONSET MetallicOsc (6 naive squares), OUTLOOK (all saws/squares/pulses), ORIGAMI (naive osc into STFT). Source: Fleet Report Systematic Issue #3 + P0 Safety.

### Buffer Overflow at 96kHz — 7 engines

OPAL (memory corruption), Bite/Overbite (echo/chorus), Drift/Odyssey (stutter), OceanDeep (reverb), Ottoni (delay), Ouie (KS floor), Overworld (Haas 16-sample hardcode). Source: Fleet Report Systematic Issue #4.

### SR=44100 Hardcoded — 4 engines

OHM (all structs), OLE (voice struct), Ottoni (voice struct), Ostinato (7 structs). Source: Fleet Report Systematic Issue #7.

### Bilinear/Euler Filters (not matched-Z) — 3 engines

Morph/OddOscar (MoogLadder), Organism (OrgFilter), Ottoni (reverb LP). Source: Fleet Report Systematic Issue #8.

### Dead Coupling Routes — 4 engines

Owlfish (LFOToPitch discarded), Offering (AudioToFM TODO), Outflow (EnvToDecay noop). Orca (couplingBreachTrigger dead). Source: Fleet Report Systematic Issue #9.

### Kitchen Collection — BROTH Coordinator Missing

BROTH cooperative coupling (`setBrothSessionAge`, `getSessionAge`, `getConcentrate`, `getSpectralMass`) is never called from `XOceanusProcessor.cpp`. Affects OVERWASH, OVERWORN, OVERFLOW, OVERCAST. Source: Seance XRef KC-P0-05. Status: OPEN. — RESOLVED (verified 2026-04-19): XOceanusProcessor.h line 665 declares `BrothCoordinator brothCoordinator_`. XOceanusProcessor.cpp lines 2213–2248 show full coordinator — engine discovery, `getSessionAge()`, `getConcentrateDark()`, `getTotalSpectralMass()`, and broadcasts to all 4 BROTH engines via `setBrothSessionAge`, `setBrothConcentrateDark`, `setBrothSpectralMass`.

### SpectralFingerprint Propagation (Fusion quad)

`attackTransience` never populated in OASIS and ODDFELLOW fingerprints. 5th-slot engine cannot differentiate attack character. Source: Seance XRef Fusion quad.

### ComplexOscillator DSP Library

`Source/DSP/ComplexOscillator.h` — Claims bidirectional FM but does phase rotation. Architecturally misrepresented. Score 6.5/10. Should be renamed or reimplemented. Source: Fleet Report DSP Library + Systematic Issue #10.

### MegaCouplingMatrix Infrastructure

100-item improvement roadmap filed as GH #1093. Key P1 items:
- Audio-rate and control-rate not separated on audio thread
- `std::vector` / `shared_ptr` atomic in audio path (should be flat array)
- CouplingRoute config/state not split (audio/message thread safety)
- AudioToBuffer ring buffer no crossfade on wrap (click at low freq content)
- SRO linear interp has no jitter rejection (stair-step in harmonics)
Source: GH #1093. Status: OPEN (roadmap only, no fixes landed).

---

## Launch Blockers — Consolidated

From FATHOM L5 TIER 0 (original 13 items) with TIER 0 Triage 2026-04-11 resolution status:

| # | Issue | Status |
|---|-------|--------|
| 1 | OVERWORLD: All DSP files are stubs | **FP** — DSP implemented (TIER 0 FP-001); re-seance needed |
| 2 | OBIONT: 0 presets + CPU bomb + dead mod destinations | **DEFERRED** (DEFER-001) |
| 3 | OUTFLOW: Zero presets | **DEFERRED** (DEFER-002); params wired in `afd1eee4` |
| 4 | OKEANOS: Prefix collision with OASIS | **FP** — uses `okan_`, no collision (TIER 0 FP-003) |
| 5 | ShaperRegistry: Shapers never called | **FIXED** commit `0b307df5` (FIX-003) |
| 6 | ObserveShaper: EQ band accumulation broken | **FP** — serial topology is correct by design (TIER 0 FP-004) |
| 7 | OASIS `addParameters()` missing from APVTS | **FP** — fixed in `afd1eee4` (TIER 0 FP-002) |
| 8 | OUTFLOW `addParameters()` missing from APVTS | **FP** — fixed in `afd1eee4` (TIER 0 FP-002) |
| 9 | MasterFXChain: 6 null param loads → crash | **FIXED** commit `0b307df5` (FIX-001) |
| 10 | OxideShaper: Lorenz NaN propagation | **FIXED** commit `c86e37a5` (FIX-005) |
| 11 | OxideShaper: Tape mode HF rolloff no-op | **FP** — working one-pole LP (TIER 0 FP-005) |
| 12 | ObserveShaper: Iron emulation mono-only | **FP** — both channels processed (TIER 0 FP-006) |
| 13 | XOceanusProcessor: `currentSampleRate` data race | **FIXED** commit `0b307df5` (FIX-002) |

**Net result: 6 real fixes landed; 6 confirmed false positives. TIER 0 CLOSED 2026-04-11.**

**Remaining active blockers (from systemic issues above):**
- ~~OVERWASH coupling macro controls nothing (KC-P0-03)~~ — RESOLVED (verified 2026-04-19)
- ~~BROTH coordinator never called (KC-P0-05)~~ — RESOLVED (verified 2026-04-19)
- OTO Melodica sawtooth unaliased (KC-P0-07)
- OBLIQUE 5.8/10 — DEFERRED arch rework
- OBIONT 3.5/10 — DEFERRED full rework

---

## Missing Sources — Note for Scanner Agents

**Phantom Sniff + QDD report (issues #751–#771):** Referenced in user memory as a 35-finding, 21-issue report from 2026-04-04. No matching file found under `Docs/`, `journal/`, or repo root using patterns `*phantom-sniff*`, `*ps-qdd*`, `*2026-04-04*`. The 21 issues are NOT reflected in this baseline. Scanner agents MUST treat issues #751–#771 as potentially live and verify them independently.

---

*Generated 2026-04-19 | Sources: FATHOM L5 (2026-03-29), FATHOM Fleet (2026-03-29), TIER 0 Triage (2026-04-11), Seance XRef (2026-04-15), GH issues #1077 #1090–#1094 (2026-04-19), git log 2026-03-01–2026-04-19*
*Scanner agents: log only findings NOT present in this document.*
