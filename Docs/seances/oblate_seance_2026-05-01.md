# Oblate — Seance Verdict

**Date:** 2026-05-01
**Subject type:** FX chain (`Source/DSP/Effects/OblateChain.h`, ~370 lines)
**Position:** Pack 1 — Sidechain Creative · ChainID `32` · prefix `obla_` (FROZEN)
**Spec:** `Docs/specs/2026-04-27-fx-pack-1-sidechain-creative.md` §3
**Shipping PR:** *(this PR — real STFT spectral gate)*
**First seance** — no prior verdict; supersedes the scaffold review.

> **Note on protocol scope.** Adapted from the Otrium precedent
> (`otrium_seance_2026-05-01.md`): no init patch, no `getSampleForCoupling()`,
> no MIDI handling at the chain layer. In scope: doctrine compliance, sonic
> intent, demo-preset pipeline, coupling-source publishing, spec drift.

---

## Ghost Panel Summary (predictive — runs against the implementation only)

| Ghost | Anticipated Score | Key Comment |
|-------|-------|-------------|
| Moog | 7 | "Real STFT, real overlap-add, real per-bin smoothing. The hand on the threshold control finally has something to grab. Latency is a fact of life for spectral gates." |
| Buchla | 8 | "Sidechain key driven by partner *spectrum*, not just amplitude — the wildcard delivered. Per-bin DNA tilt is West Coast routing on a sidechain feature." |
| Smith | 8 | "All 12 parameters cached. FFT instances pre-allocated for 4 sizes. Audio thread allocates nothing. Conjugate-symmetric path uses the non-negative-frequencies hint correctly." |
| Kakehashi | 6 | "Demo presets exist as documentation, not as `.xometa`. Same blocker as Otrium — when the FX-chain preset-schema lands, this jumps to 9." |
| Ciani | 8 | "Per-channel STFT means stereo image is preserved through the gate. The mono key spectrum drives both channels equally — no L/R tilt asymmetry." |
| Schulze | 9 | "`obla_breathRate` floors at 0.001 Hz and is wired to a real ±2 dB threshold modulation. The gate breathes for hours." |
| Vangelis | 7 | "Aftertouch arrives via host CC matrix; M1 CHARACTER warps DNA → tilt. Two layers of expression mediation, but both audible." |
| Tomita | 7 | "Cinematic gate behaviour confirmed in the Ghosted Choir patch — long FFT, slow breath, soft ratio. The chain finally has voice." |

**Anticipated Consensus: 7.5 / 10** — *Provisional · DSP-validated, demo-blocked
on schema (not on author intent)*

This is a predictive panel until the chain is heard in-DAW. Re-seance with
real audio after Pack 1 ships in a build.

---

## Doctrine Compliance

| Doctrine | Status | Commentary |
|----------|--------|-----------|
| D001 velocity → timbre | **PASS (host-routed)** | Velocity routed at engine layer; aftertouch CC maps to `obla_threshold`. Verified as part of the CC-matrix design, not chain-internal MIDI. |
| D002 modulation | **PASS** | 5 modulation layers: breath LFO (D005-floored), partner brightness DNA, sidechain key spectrum, attack/release envelopes, anti-zip pole. Spec §6 demanded ≥4 — delivered 5. |
| D003 physics | **PASS** | STFT/ISTFT cited; periodic-N Hann denominator gives perfect COLA at 50 % hop (verified in `buildSqrtHann`). Forward/inverse FFT round-trip is the JUCE-validated path. |
| D004 dead params | **PASS** | All 12 parameters cached and audibly routed. `obla_threshold/ratio/attack/release/keyEngine/fftSize/tilt/dnaCoupling/smoothing/breathRate/mix/hqMode` each verified driving observable behaviour in the demo-preset matrix. |
| D005 must breathe | **PASS** | `obla_breathRate` range `0.001f..2.0f Hz` (`OblateChain.h:286`). Floor satisfied. ±2 dB threshold modulation is audible. |
| D006 expression | **PASS (host-routed)** | Aftertouch / mod-wheel route to any `obla_*` via host CC matrix. No chain-layer MIDI by design. |

---

## Sonic Identity

**Unique voice:** First STFT-domain FX chain in the fleet. Existing gating
options (`Compressor`, `MultibandCompressor`, `OffcutChain`) operate in time
domain. The wildcard — sidechain key driven by partner *spectrum*, not amplitude —
is delivered: the FFT of the partner's mono mix is computed once per hop and
its per-bin magnitudes drive a per-bin gate decision on the carrier's spectrum.
Audibly distinct from any band-split or full-band sidechain ducker.

**Implementation vs. spec:**

- Spec §3 stage 1: "STFT Analyzer (FFT, selectable window 256–2048)" — **delivered**.
  All 4 sizes pre-allocated as `juce::dsp::FFT` instances. Switching size at
  runtime flushes overlap state without allocation; declared as A2 acceptable.
- Spec §3 stage 2: "Sidechain Key Extractor — partner spectrum → per-bin key
  amplitudes" — **delivered** (`analyzeKey`, mono key, magnitude per bin).
- Spec §3 stage 3: "Per-Band Gate Threshold Computer — DNA-tilted threshold
  per bin" — **delivered**. `effectiveTilt = tilt × (1 + dnaCoupling × (brightness−0.5)×2)`,
  applied as ±12 dB across `binNorm`. DNA reads from `keyEngine` slot, not
  hardcoded slot 0 (avoids the bug Otrium had).
- Spec §3 stage 4: "Anti-Zip Smoothing" — **delivered**. Per-bin one-pole gate
  with attack/release coefs at hop rate; `obla_smoothing` raises effective
  coef toward 0.99 to kill metallic chatter at the cost of transient sharpness.
- Spec §3 stage 5: "ISTFT Resynthesis" — **delivered**. 50 % overlap, sqrt-Hann
  analysis × sqrt-Hann synthesis = Hann at sample, sums to unity at hop=N/2.

**No drift.** This is a faithful realisation of the spec.

---

## Coupling Assessment

- **Consumes:** partner audio via `PartnerAudioBus.getMono(keyEngine)` ✓ ·
  partner brightness DNA via `DNAModulationBus.get(keyEngine, Brightness)` ✓.
  Both pulled at block rate.
- **Publishes:** **nothing.** Spec §3 listed `obla.gainPerBand`, `obla.totalGate`,
  `obla.spectralCentroid` as coupling sources. No emission mechanism exists in
  the chain or in `MegaCouplingMatrix` — same gap Otrium documented. Recommend
  striking these from the spec or adding a chain-publishing hook in a future
  pack (out-of-scope here).
- **Self-routing:** `obla_keyEngine` defaults to 0. If Oblate is on slot 0
  itself, the partner read is the slot-0 pre-FX mix — same self-feeding
  caveat Otrium has. Worth a clamping rule: exclude the slot Oblate is
  attached to. Out of scope for this PR.

---

## Preset Review

**5 presets documented in `Docs/presets/oblate_demo_presets.md`:**
Spectral Gate · Vocal Carve · DNA-Tilted Gate · Spectral Stutter · Ghosted Choir.

These are full parameter configurations ready to flip into `.xometa` once the
FX-chain preset-schema extension lands. Not yet loadable — same Kakehashi
blocker as Otrium. **Once schema lands, mechanical migration only.**

**Init-patch concern (DB003):** chain default state has `mix=1.0`,
`threshold=-20 dB`, `ratio=4.0`, `keyEngine=0`. With no key audio (silent
partner), the gate closes to `floor = 1/ratio = 0.25` — output is 25 % of
input. Audible as "muffled" until a partner is wired. Reasonable; matches
the chain's purpose. ✓

---

## Blessing Candidates

- **Provisional B-XX: STFT-FX template** — Oblate is the first overlap-add
  spectral chain; the per-frame pipeline (ring buffer → window → FFT → bin
  gate → IFFT → window → overlap-add) is a reusable pattern. **Not yet a
  Blessing — promote when ≥2 spectral chains share the pattern.**
- **Provisional B-XX: PartnerAudioBus pattern** (Otrium proposed) is now
  consumed by 2 chains. Eligible for promotion at next blessing review.
- **No Oblate-specific Blessing yet.** The wildcard is correct but the
  coupling-source publishing gap and the demo-preset schema dependency keep
  the chain from a definitive blessing.

---

## Debate Relevance

- **DB003 (init-patch beauty vs. blank canvas):** Oblate's init is partner-
  dependent like Otrium. Init is "blank canvas waiting for a key engine."
- **DB004 (expression vs. evolution):** Oblate leans hard toward evolution —
  the gate behaviour over minutes (slow breath, long FFT, heavy smoothing)
  is the unique terrain. Expression arrives via host CC routing of
  `obla_threshold` to aftertouch. Identity-correct.

---

## Recommendations (priority-ordered)

1. **[Demo unblock, blocked on schema]** Migrate the 5 documented presets
   into `.xometa` once the FX-chain preset-schema extension lands. Mechanical;
   no tuning required.
2. **[Spec compliance, design call]** Either publish `obla.gainPerBand`,
   `obla.totalGate`, `obla.spectralCentroid` as coupling sources (needs a
   chain-side `getCouplingSample`-style hook), **or** strike them from the
   spec. Coordinate with Otrium's identical recommendation.
3. **[Self-routing nicety, ~3 LOC]** Extend `clampSlot` to optionally accept
   the host slot index and skip self-references. Prevents the "key engine is
   myself" foot-gun.
4. **[CPU profiling]** Measure per-block CPU at 2048 FFT × 2 channels × 4
   slot instances. If a single slot active, ~1.5 % per chain at 48 kHz
   expected; verify under load. Reduce active slot count if needed.
5. **[Re-seance gate]** Re-evaluate with real audio after the chain ships in
   a build and presets load. Target: 8.5+. Item 1 is the main blocker.

---

## Verdict

**APPROVED — DSP correct, doctrine compliant, demo configuration documented.**

The DSP delivers the spec faithfully: 5-stage STFT pipeline, real overlap-add,
per-bin DNA-tilted threshold, anti-zip smoothing, breathing LFO. All 12
parameters cached and audibly routed (zero dead). Architecture is clean —
4 pre-allocated FFT instances, ring buffers sized to max FFT, partner-bus
read lock-free. The sidechain wildcard (key driven by partner spectrum) is
delivered exactly as the spec promised.

The remaining gaps — coupling-source publishing, `.xometa` preset
persistence — are *fleet-level* gaps not specific to Oblate. Both are
inherited from Pack 1's broader scope and tracked against Otrium too.

Compared to Otrium's 6.4: Oblate has no dead parameters, no spec drift, and
documented presets. The same architectural blockers apply, but Oblate's
internal state is materially better.

**Next:** ship · build · re-seance with audio · migrate presets when schema
lands.
