<!-- ARCHITECT'S REVIEW — OWARE ENGINE -->
<!-- Generated: 2026-03-21 | Scope: Single Engine | Risk: Medium -->

# Architect's Review: OWARE Engine
**XOware "The Resonant Board" — Governance Review Against the Five Provinces**
Date: 2026-03-21 | Scope: Single Engine | Risk: Medium (two blocking findings)

---

## Proactive Analysis

**Rework Risk:** AT RISK — Two categories of rework identified (LFO dead params, per-sample trig cost). Both are localized to OwareEngine.h and do not require touching other engines or shared headers.

**Inconsistency Risk:** AT RISK — The shimmer LFO rate is hardcoded to `0.3f` at line 592 inside the per-sample loop, ignoring the declared `owr_lfo1Rate` and `owr_lfo2Rate` parameters entirely. This creates a mismatch between declared capability and actual behavior that will confuse preset authors.

**Conflict Risk:** CLEAR — No recent overlapping commits to this file. OWARE was registered 2026-03-20; no other engines share `owr_` prefix.

**Fleet Pattern Survey — how the fleet handles dedicated LFOs:**

| Engine | LFO declaration | LFO used in render? |
|--------|----------------|---------------------|
| OXBOW (OxbowEngine.h) | `StandardLFO lfo1, lfo2` engine-level members | Yes — driven from paramLfo1Rate/Depth in renderBlock |
| ORBWEAVE (OrbweaveEngine.h) | `StandardLFO lfo1, lfo2` engine-level members | Yes — driven from paramLfo1Rate/Depth in renderBlock |
| OWARE (OwareEngine.h) | `StandardLFO shimmerLFO` per-voice; engine-level LFO objects absent | **No** — paramLfo1/2 attached but never read |

OWARE is the only post-2026-03-20 engine to declare LFO parameters without engine-level LFO objects to back them. The correct fleet pattern (two engine-level `StandardLFO` members driven from their params) is missing.

**Full Scope:** OwareEngine.h only. No shared headers affected. 20 existing .xometa presets should be verified after fix (LFO depth defaults to 0.0 so no audible change to existing presets — backward safe).

---

## Province I — DOCTRINE

### D001: Velocity Must Shape Timbre

**PASS.**

Velocity drives `hardness` computation at note-on (line 728–730):
```
float hardness = std::clamp(
    (paramMalletHardness->load() : 0.3f) + vel * 0.5f + aftertouchAmount * 0.3f,
    0.0f, 1.0f);
```
`hardness` directly controls:
- Mallet contact time (line 94: `contactMs = 5.0f - hardness * 4.5f`)
- Spectral lowpass cutoff (line 102: `malletCutoff = baseFreq * (1.5f + hardness * 18.5f)`)
- Mallet bounce activation threshold (line 109: `bounceActive = hardness < 0.4f && vel < 0.7f`)
- Upper mode amplitude rolloff (line 653: `modeAmp = 1.0f / (1.0f + m * (1.5f - malletNow * 1.2f))`)

Velocity shapes contact spectrum, bounce behavior, and mode balance — not only amplitude. D001 is unambiguously satisfied.

---

### D002: Modulation is the Lifeblood

**CONCERN — LFO parameters declared but dead.**

The engine declares 6 LFO parameters (`owr_lfo1Rate/Depth/Shape`, `owr_lfo2Rate/Depth/Shape`, lines 822–831) and attaches them via `attachParameters` (lines 855–860). However, `paramLfo1Rate`, `paramLfo1Depth`, `paramLfo1Shape`, `paramLfo2Rate`, `paramLfo2Depth`, `paramLfo2Shape` are **never read anywhere in `renderBlock`** or any other method.

The only LFO in use is `voice.shimmerLFO`, which is a per-voice internal LFO hardcoded at `0.3 Hz` (line 592). It is not driven from any of the declared LFO parameters.

Minimum D002 requirement is: "Min: 2 LFOs, mod wheel/aftertouch, 4 working macros, 4+ mod matrix slots." The mod wheel (line 527: material blend), aftertouch (line 528: mallet hardness), and 4 macros are all wired. But the "2 LFOs" requirement is met in form only — the parameters exist but the DSP objects do not.

This is a **D004 co-violation** (see below). The LFO parameters are declared but produce zero audio effect.

**Line references:** Parameters declared at lines 822–831. Attached at lines 855–860. `renderBlock` (lines 481–705): no reference to `paramLfo1*` or `paramLfo2*`. Dead parameter verdict confirmed.

---

### D003: The Physics IS the Synthesis

**PASS — genuinely exemplary.**

Every claim is cited:
- Modal ratios sourced from Rossing (2000) and Fletcher & Rossing (1998) (lines 68–82): `kWoodRatios`, `kMetalRatios`, `kBellRatios`, `kBowlRatios` are all static constexpr tables.
- Mallet contact model cites Chaigne & Doutaut 1997 (header line 86, code comments line 100).
- Resonator bank uses second-order digital resonators with proper matched-Z coefficient derivation (line 173: `r = exp(-pi * bw / sr)`) — not Euler approximation.
- Material exponent alpha cites Fletcher & Rossing §2.3 (header line 43).
- Balinese beat-frequency shimmer cites Rossing 2000 with correct additive-Hz implementation (lines 595–596), not ratio-based detuning.

D003 is a standout — this is one of the most rigorously grounded physical models in the fleet.

---

### D004: Dead Parameters Are Broken Promises

**BLOCK — 6 parameters are dead.**

`owr_lfo1Rate`, `owr_lfo1Depth`, `owr_lfo1Shape`, `owr_lfo2Rate`, `owr_lfo2Depth`, `owr_lfo2Shape` are declared in `createParameterLayout` (lines 822–831), cached as member pointers (lines 903–908), and attached in `attachParameters` (lines 855–860). None are read in `renderBlock`, `noteOn`, `noteOff`, or any other method.

Additionally, there are no engine-level `StandardLFO` objects (the voice-level `shimmerLFO` is internal and unrelated to these parameters). The parameters will appear in the UI and preset files but will have no audible effect.

This is a D004 violation. **Status: BLOCK.**

Fix required: Add two engine-level `StandardLFO` objects and wire them to `owr_lfo1*` / `owr_lfo2*` params. Route LFO1 to material modulation or brightness (the most musically meaningful targets for this engine), LFO2 to shimmer rate or decay. Default depth of 0.0 means existing presets remain silent on these params — no backward compatibility risk.

---

### D005: An Engine That Cannot Breathe Is a Photograph

**CONCERN — conditional pass.**

The Thermal Drift system (lines 554–563) provides continuous, slow autonomous modulation with target update every ~4 seconds and approach rate of `0.00001f` per sample — genuinely slow breathing at a rate far below 0.01 Hz. This is the "alive when nobody's playing" design intent.

However, D005 specifies "at least one LFO with rate floor ≤ 0.01 Hz." The `owr_shimmerLFO` is hardcoded at 0.3 Hz (line 592) and cannot be set to sub-Hz rates by the user. The declared `owr_lfo1Rate` has a floor of 0.005 Hz (line 823) but is dead (see D004). Thermal drift qualifies as autonomous modulation but not as a user-controllable LFO.

Once D004 is resolved (LFO1/2 wired), D005 is satisfied. The LFO rate range `0.005–20 Hz` (line 823) meets the 0.01 Hz floor requirement.

**Status: CONCERN — becomes CLEAR when D004 is resolved.**

---

### D006: Expression Input Is Not Optional

**PASS.**

- Velocity → timbre: confirmed at D001 above.
- Aftertouch (CC channel pressure): captured at line 490, applied to `aftertouchAmount`, routed to mallet hardness (line 528) and filter brightness (line 532). Aftertouch creates immediate timbral change post-note-on.
- Mod wheel (CC1): captured at line 491–492, applied to material blend (line 527). Material blend continuously shifts the spectral character of all sounding voices.

D006 satisfied on both required axes.

---

## Province II — BLESSINGS

### B002: auval compliance
**CLEAR.** No allocation in audio thread, no blocking I/O, `prepare()` / `releaseResources()` / `reset()` all present. No auval risk identified.

### B003: Preset backward compatibility
**CLEAR with caveat.** All 20 existing presets (Foundation, Atmosphere, Prism, Flux, Entangled moods) use `owr_` prefixed keys. None include `owr_lfo1*` or `owr_lfo2*` values, confirming these were never in use. Adding the LFO DSP objects will not change existing preset sounds because default depth is 0.0.

The Balafon preset (inspected) does not store LFO params, confirming they will default to 0.0 depth on load — backward safe.

### B006: Voice stealing policy
**CLEAR.** Uses `VoiceAllocator::findFreeVoice()` (line 713) — fleet-standard oldest-note steal. Consistent with all other polyphonic engines.

### B007: Denormal safety
**CLEAR.** `flushDenormal()` called at line 183 (OwareMode resonator loop), line 332 (bowl resonator body), line 675 (voice ampLevel). All feedback paths are guarded. `OwareBuzzMembrane` uses CytomicSVF which has its own internal denormal protection.

### B008: Sample rate independence
**CLEAR.** All time-domain and frequency calculations use `srf` (derived from `sampleRate` argument to `prepare()`). No hardcoded 44100 or 48000 in any DSP path. `OwareBodyResonator` stores `sr` member and uses it throughout. Mallet contact samples computed as `contactMs * 0.001f * sampleRate` (line 95).

### B009: Thread safety
**CLEAR.** All parameter reads use `p->load(std::memory_order_relaxed)` (lines 501–522). `activeVoiceCount` is `std::atomic<int>` (line 869). No unsafe cross-thread writes detected.

### B013: Engine prefix uniqueness
**CLEAR.** `owr_` prefix is unique to OWARE. No other engine uses this prefix (verified against CLAUDE.md parameter prefix table).

---

## Province III — DEBATES

### DB001: Mutual exclusivity vs. effect chaining
**SURFACES weakly.** The Buzz Membrane (OwareBuzzMembrane) feeds into the Body Resonator in a fixed signal chain (buzz → body, lines 668–671). The user cannot reorder these stages or bypass body while keeping buzz. A strict Buchla-school reading would say this is a fixed series chain that limits exploration. The field-school reading would say it models the physical signal path correctly. This engine takes the field position implicitly. **No resolution required — surface for awareness.**

### DB003: Preset count floor
**SURFACES.** OWARE has 20 presets. The fleet floor is 150 per engine. OWARE is well below floor at launch. This is a quantitative gap rather than a design tension. **Action: expand to 150+ in Round 13 preset expansion.**

### DB004: Expression vs. Evolution — gesture vs. temporal depth
**SURFACES directly.** Thermal Drift is the engine's central "temporal depth" feature — it makes patches feel alive over long timescales even without gesture. Simultaneously, aftertouch-to-mallet hardness provides immediate gestural expression. OWARE takes a clear position: **both coexist**. The engine does not resolve DB004 but demonstrates a successful synthesis of both poles. This is worth noting in the seance record.

---

## Province IV — ARCHITECTURE

### Per-sample allocations
**PASS.** No `new`, `delete`, `std::vector`, or dynamic container usage in `renderBlock`. `sympInputPerMode[OwareVoice::kMaxModes]` at line 601 is a stack array with compile-time size — not a heap allocation.

### FastMath usage in audio loop
**VIOLATION — 4 instances identified.**

The per-sample loop (lines 568–699) contains the following transcendental function calls that violate Province IV:

**1. Line 588 — `std::pow` for thermal drift pitch calculation:**
```cpp
freq *= std::pow(2.0f, totalThermalCents / 1200.0f);
```
Called every sample per active voice. `totalThermalCents` changes very slowly (thermal drift, ≤±8 cents). Should use `PitchBendUtil::semitonesToFreqRatio` or a FastMath approximation.

**2. Line 657 — `std::pow` for material alpha mode scaling:**
```cpp
float modeDecayScale = std::pow(static_cast<float>(m + 1), -materialAlpha);
```
Called 8 times per active voice per sample (inner mode loop). `materialAlpha` is derived from a smoothed parameter. This is the highest-cost violation — up to 64 `std::pow` calls per sample at full 8-voice polyphony.

**3. Lines 689–690 — `std::cos`/`std::sin` for panning:**
```cpp
float gainL = std::cos((pan + 1.0f) * 0.25f * 3.14159265f);
float gainR = std::sin((pan + 1.0f) * 0.25f * 3.14159265f);
```
Pan is based on `voice.currentNote` which never changes during a note. These values are constants for the life of the voice and should be cached at `noteOn` (line 711), not recomputed every sample.

**4. `OwareMode::setFreqAndQ` called per-sample (line 660), which contains `std::exp`, `std::cos`, `std::sin` (lines 172–176).**
```cpp
float r = std::exp(-3.14159265f * bw / sampleRate);
a1 = 2.0f * r * std::cos(w);
b0 = (1.0f - r * r) * std::sin(w);
```
This is called inside the inner mode loop (`for (int m = 0; m < kMaxModes; ++m)`) inside the voice loop inside the sample loop. At 8 voices × 8 modes = 64 calls per sample, each running `std::exp + std::cos + std::sin`. This is the most severe performance violation.

The mode frequencies change each sample because `freqWithShimmer` includes the shimmer LFO output. But the shimmer LFO moves at 0.3 Hz. Coefficient recalculation at audio rate for a 0.3 Hz modulation is waste. The coefficients should be recomputed at a reduced rate (control-rate reduction) or the shimmer should be moved to a coefficient-update tick rather than the per-sample path.

**Summary of Architecture violations:**
- `std::pow` × 2 (lines 588, 657): VIOLATION
- `std::cos`/`std::sin` for pan (lines 689–690): VIOLATION — cache at noteOn
- `OwareMode::setFreqAndQ` per sample with internal `std::exp`/`std::cos`/`std::sin` (line 660): VIOLATION — worst offender

### IIR coefficient method
**PASS.** `OwareMode::setFreqAndQ` uses matched-Z transform: `r = exp(-pi * bw / sr)` (line 173). Not Euler approximation. Compliant.

### Denormal guards
**PASS** (see B007 above).

### Sample rate independence
**PASS** (see B008 above).

---

## Province V — BRAND

### XO + O-word naming
**PASS.** Engine is registered as `XOware` / gallery code `OWARE`. "Oware" is the correct O-word (mancala game from the Akan people). Follows XO___ pattern.

### Accent color uniqueness
**PASS.** `#B5883E` (Akan Goldweight brass) — verified unique in CLAUDE.md engine color table. No other engine uses this color. `getAccentColour()` at line 413 returns `juce::Colour(0xFFB5883E)` — hex matches the registered value exactly.

### Parameter prefix
**PASS.** All parameters use `owr_` prefix (lines 780–831). Matches CLAUDE.md Oware entry: `owr_`. `getEngineId()` returns `"Oware"` (line 412) which matches the registration name.

### owr_ prefix conflicts
**PASS.** Checked against full CLAUDE.md prefix table. `owr_` is assigned solely to OWARE. Nearest neighbors (`over_` for OVERTONE, `owit_` for OUTWIT) are distinct.

### Creature identity
**PASS — exceptional.** The creature identity (lines 6–13) is one of the most developed in the fleet: sunken oware board, coral-encrusted, bronze barnacles, metal/glass/stone/wood seeds in the cups. The mythological grounding directly motivates each pillar — the "board that remembers every impact" maps to sympathetic resonance, the seed materials map to the material continuum, the ocean floor setting maps to the thermal drift. Identity is fully coherent with the Aquatic Mythology framework.

---

## Technical Check Summary

| Check | Status | Notes |
|-------|--------|-------|
| No per-sample allocations | PASS | Stack array at line 601 is fine |
| FastMath used in audio loop | FAIL | 4 violations: std::pow ×2, std::cos/sin pan, setFreqAndQ per-sample |
| Denormal guards present | PASS | Lines 183, 332, 675 |
| Sample rate independent | PASS | All paths use `srf` |
| Param IDs unchanged | N/A | New engine, no prior shipped IDs |
| Presets backward compatible | PASS | LFO params absent from all 20 presets — silent default |

---

## SCIONS Alignment

| Principle | Score | Note |
|-----------|-------|------|
| Simplify | PARTIAL | Material Continuum and physics are clean; dead LFO params add noise |
| Continuous | PASS | Thermal drift + voice shimmer provide ongoing life |
| Improve | PASS | 7 pillars genuinely advance the percussion synthesis vocabulary |
| Organic | PASS | All features flow from the creature identity; nothing feels grafted |
| Natural | CONCERN | Per-sample setFreqAndQ will cause CPU spikes — may surface as audio dropout at 8-voice polyphony on slower machines |
| Sustain | CONCERN | Dead LFO params will confuse preset authors; they will waste time setting LFO rate/depth expecting results |

---

## Verdict

```
╔══════════════════════════════════════════════════
  ARCHITECT'S REVIEW: OwareEngine.h — OWARE
  Scope: Single Engine | Risk: Medium
╔══════════════════════════════════════════════════

PROACTIVE ANALYSIS
──────────────────
Rework Risk:   AT RISK — Dead LFO params require DSP object addition and wiring
Inconsistency: AT RISK — shimmerLFO hardcoded at 0.3 Hz, ignores owr_lfo1* params
Conflict:      CLEAR — no overlapping changes

GOVERNANCE CHECK
────────────────
Doctrine:     VIOLATION — D004: 6 dead LFO parameters (lines 822–831, 855–860, 903–908)
              D002: LFO requirement met in form but not DSP; resolves when D004 fixed
              D005: Conditional CLEAR — thermal drift qualifies; full compliance after D004 fix
              D001, D003, D006: CLEAR
Blessings:    CLEAR — B002, B003, B006, B007, B008, B009, B013 all pass
Debates:      SURFACES DB001 (fixed buzz→body chain), DB003 (20 presets vs 150 floor),
              DB004 (thermal drift + aftertouch as synthesis of both poles)
Architecture: VIOLATION — 4 per-sample transcendental math violations:
              - std::pow for thermal drift at line 588
              - std::pow for material alpha scaling at line 657 (worst: 64×/sample)
              - std::cos/sin for pan at lines 689–690 (should cache at noteOn)
              - OwareMode::setFreqAndQ with std::exp/cos/sin called per-sample at line 660
Brand:        CLEAR — naming, color, prefix, creature identity all pass

TECHNICAL CHECK
───────────────
[✗] No per-sample allocation     PASS (stack array only)
[✗] FastMath used in audio loop  FAIL — 4 violations listed above
[✓] Denormal guards present      PASS
[✓] Sample rate independent      PASS
[N/A] Param IDs unchanged        N/A (new engine)
[✓] Presets backward compatible  PASS

SCIONS: S[~] C[✓] I[✓] O[✓] N[~] S[~]

VERDICT
───────
REQUEST CHANGES

Required before merge:

1. [D004 — BLOCK] Wire owr_lfo1* and owr_lfo2* to actual DSP.
   Add two engine-level StandardLFO members (lfo1, lfo2). In renderBlock,
   read paramLfo1Rate/Depth/Shape and drive lfo1 accordingly; same for lfo2.
   Route LFO1 output to brightness modulation (most natural target for this
   engine — shimmer the whole spectrum). Route LFO2 to material blend for
   slow morphing. Default depth 0.0 means no backward compat risk.

2. [Province IV — BLOCK] Move OwareMode::setFreqAndQ out of the per-sample loop.
   The shimmer LFO moves at 0.3 Hz. Recalculating IIR coefficients at audio
   rate for sub-Hz modulation is unjustified cost. Options:
   a) Move coefficient update to a block-rate tick (every 64 samples),
      with linear interpolation of the resonator frequency target.
   b) Cache modeFreq per voice at noteOn; only recompute when
      material or pitch changes beyond a threshold.
   Either approach eliminates 64× std::exp/cos/sin per sample at 8-voice polyphony.

3. [Province IV — CONCERN] Cache pan gains at noteOn.
   Lines 689–690 recompute std::cos/sin every sample for a value that only
   changes with currentNote. Move to noteOn (line 687 in current form):
   cache as v.gainL and v.gainR on the voice struct.

4. [Province IV — CONCERN] Replace inline std::pow calls in audio loop.
   Line 588: use PitchBendUtil::semitonesToFreqRatio(totalThermalCents / 100.0f)
   or a FastMath::fastPow2 approximation.
   Line 657: precompute mode decay scales at block boundary when materialAlpha
   changes beyond epsilon. These 8 values are stable between material changes.

5. [DB003 awareness] OWARE ships with 20 presets. Fleet floor is 150.
   Schedule Round 13 preset expansion: target 150 presets across all 8 moods.
   Priority: Submerged mood (zero presets currently), Family mood (zero).

Raj logs: OWARE — REQUEST CHANGES. D004 (6 dead LFO params) + Province IV
(4 per-sample trig violations). Brand/Blessings/D001/D003/D006 all clear.
Exceptional D003 compliance. Blast radius: OwareEngine.h only. Fix order:
D004 LFO wiring first (audit), then Province IV math fixes (performance).
╚══════════════════════════════════════════════════
```

---

## Blessing Nomination

Upon resolution of the two blocking findings, OWARE should be considered for a new Blessing:

**Candidate B017 — Material Continuum:** The four-material continuous spectral morphing system (wood → bell → metal → bowl via table interpolation with per-mode alpha exponents) is genuinely novel. No other engine in the fleet or known synthesizer ecosystem implements continuous material morphing at the per-mode decay level with physically-cited ratio tables. The Chaigne mallet contact model + material exponent + sympathetic resonance network is a publishable combination. Nomination pending D004+IV clearance.

---

*Architect's review complete. Raj: log as REQUEST CHANGES. Resubmit after items 1–4 resolved.*
