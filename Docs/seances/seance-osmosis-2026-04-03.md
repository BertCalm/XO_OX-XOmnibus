# The Verdict -- OSMOSIS (First Seance)
**Seance Date**: 2026-04-03
**Engine**: OSMOSIS | External Audio Membrane Engine
**Identity**: The Membrane | Surface Tension Silver `#C0C0C0`
**Param prefix**: `osmo_` | 4 parameters | Mono (analysis engine)
**Score**: **7.4/10**
**Notes**: First seance. OSMOSIS is an analysis engine — like OPTIC (B005), its primary output is coupling data rather than audio generation. Receives external audio, analyzes envelope/pitch/spectral content, and exposes coupling signals to the fleet. Passes external audio through a membrane filter with LFO-modulated coloring.

---

## Engine Architecture

OSMOSIS is the fleet's ear — the interface between XOceanus and the outside world:

- **Envelope follower**: Per-channel one-pole smoothing with attack/release derived from `reactivity` and `memory` parameters. Attack range 1–50 ms; release range 10–1000 ms. Both coefficients use `exp(-1/(sr * ms * 0.001f))` — matched-Z, not Euler approximation.
- **4-band spectral split**: CytomicSVF crossovers at 200 Hz, 1 kHz, 5 kHz. Four bands: sub (<200 Hz), lo-mid (200–1k), mid (1k–5k), high (>5k). Per-band RMS computed per block via exponential smoothing (`bandRMS = bandRMS * 0.8 + rms * 0.2`).
- **Pitch detection**: Autocorrelation on 2048-sample circular buffer. Range 50–2000 Hz. Runs every ~512 samples (every 2–4 blocks) to spread O(N²) work across blocks. Reliable on polyphonic material within the range.
- **Membrane filter**: One-pole LP (`1 - exp(-2π*cutoff/sr)`) for pass-through audio coloring. Cutoff derived from `selectivity` (200 Hz – 18.2 kHz). LFO modulates cutoff ±30% (`kLfoDepth = 0.3f`). Coefficient computed once per block (not per sample) at LFO block-rate phase — saves a `fastExp` per sample.
- **Coupling output**: `getSampleForCoupling()` returns the membrane-filtered output. `applyCouplingInput()` (fix #303) modulates base permeability by coupling RMS via `reactivity` attack coefficient — an engine coupled into OSMOSIS raises the membrane's wet/dry blend.
- **SilenceGate**: 500ms hold. Early-out when no external audio is present.

---

## The Council Has Spoken

| Ghost | Core Observation |
|-------|-----------------|
| **Moog** | The envelope follower coefficients are correct. `attackCoeff = exp(-1/(sr * attackMs * 0.001f))` is matched-Z, not Euler. The attack range 1–50 ms covers everything from transient detection to smooth RMS following. The release range 10–1000 ms covers everything from snappy drum response to glacial ambient decay. These are thoughtful design choices, not defaults. |
| **Buchla** | The 4-band spectral split is the engine's most underappreciated capability. Sub, lo-mid, mid, high RMS values are available as independent coupling outputs to the fleet. An engine that couples to OSMOSIS's sub band receives the kick drum and bass content independently from the high-frequency presence. This is a spectrally intelligent coupling source — more useful than raw amplitude. |
| **Smith** | Fix #303 — the coupling-input-to-permeability route — is architecturally coherent. An engine coupled INTO OSMOSIS raises the membrane's wet/dry blend, opening it to pass more external audio through. When nothing is coupled in, the membrane closes. This creates a call-and-response dynamic: the membrane opens when something is pushing against it from the synthesis side. |
| **Kakehashi** | Four macros — PERMEABILITY, SELECTIVITY, REACTIVITY, MEMORY — are exactly the right vocabulary for a membrane engine. A student who understands "how much passes through, what frequency it selects, how quickly it reacts, and how long it remembers" understands OSMOSIS completely. The macro naming is textbook. |
| **Pearlman** | The block-rate membrane coefficient computation is the right optimization. LFO rates are low (≤~5 Hz); the coefficient changes slowly. Computing `fastExp(-2π*cutoff/sr)` once per block (at LFO block-phase) vs. once per sample saves a transcendental evaluation per sample — a meaningful CPU reduction at 96 kHz. |
| **Tomita** | The pitch detection algorithm — autocorrelation on a 2048-sample ring buffer, running every 512 samples — is correctly amortized. Full O(N²) autocorrelation every block would be a CPU bomb at high sample rates. Spreading the work over 4 blocks gives detection latency of ~10ms at 48 kHz, which is acceptable for a coupling source (not a real-time pitch-tracking instrument). |
| **Vangelis** | OSMOSIS as a coupling source transforms every generative engine in the fleet. Route a vocalist into OSMOSIS, and the vocalist's breath envelope drives filter cutoffs, pitch, and envelope rates on 3 other engines simultaneously. This is not a sidechain — it is whole-body response to an external human performer. The fleet does not know the performer exists; OSMOSIS translates their presence into coupling language. |
| **Schulze** | The membrane metaphor is philosophically correct for synthesis. OSMOSIS does not analyze external audio to understand it — it passes through what it allows, filters what it selects, and remembers what it decides to remember. The `memory` parameter (10–1000ms release) determines how long the membrane holds the imprint of what passed through it. At maximum memory, the membrane holds the last loud moment for nearly a second after the sound has stopped. |

---

## Doctrine Compliance

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 | N/A (analysis engine) | OSMOSIS has no MIDI voice; "velocity" is external audio amplitude. Envelope follower tracks peak amplitude and drives coupling outputs. |
| D002 | PARTIAL | LFO modulates membrane filter cutoff (±30%). No user-facing LFO rate control — rate is fixed at default 0.5 Hz init. No mod wheel or aftertouch (no MIDI note processing). 4 working macros. Analysis engines have a modified doctrine interpretation; OPTIC (B005) sets the precedent for intentional MIDI exemption. |
| D003 | PASS | Matched-Z envelope follower coefficients. CytomicSVF for band-split crossovers. Autocorrelation pitch detection with power-of-2 buffer (2048-sample). Block-rate fastExp optimization with explicit correctness comment. |
| D004 | PASS | All 4 `osmo_` parameters live in `renderBlock()`. `osmo_permeability` → wet/dry. `osmo_selectivity` → membrane cutoff. `osmo_reactivity` → attack speed. `osmo_memory` → release time. |
| D005 | PARTIAL | LFO present and modulating membrane cutoff. No user-accessible LFO rate parameter — rate is fixed at `lfo_.setRate(0.5f, sr)` in `prepare()`. This is the D005 gap: the membrane breathes, but not at user-controlled pace. |
| D006 | PARTIAL | External audio amplitude is the expression input — the external performer IS the expression. No conventional MIDI CC/aftertouch (intentional). This is coherent with the engine's analysis role but creates a D006 gap for the doctrine's conventional interpretation. |

---

## Points of Agreement (3+ ghosts converged)

1. **The membrane metaphor is architecturally coherent** (Schulze, Kakehashi, Buchla, Vangelis) — OSMOSIS does not try to be a conventional synthesizer. It is a translation layer between acoustic performance and synthesis coupling. The four macros (permeability, selectivity, reactivity, memory) describe a membrane's physical properties with mechanical precision. The engine's identity is its function.

2. **4-band spectral split is the engine's most valuable capability** (Buchla, Smith, Tomita, Pearlman) — Raw envelope following gives one coupling signal. 4-band RMS gives four independent coupling signals simultaneously: sub energy, lo-mid energy, mid-range presence, high-frequency content. This transforms OSMOSIS from a sidechain into a spectral decomposer. A kick drum coupled through OSMOSIS sub band does something completely different from high-hat coupled through the high band.

3. **Fix #303 (coupling-to-permeability) is architecturally correct** (Smith, Moog, Schulze) — The membrane opens when something synthetic pushes against it. This creates interactive dynamics between the synthesis side and the external audio side: a loud synthline raises the membrane permeability, letting more external audio through; silence on the synthesis side closes the membrane. OSMOSIS becomes a synthesis-reactive channel strip.

4. **Block-rate membrane coefficient is the right optimization** (Pearlman, Tomita, Moog) — The comment in source explicitly documents why: "LFO rate is low; recomputing exp() every sample is the #1 CPU hotspot." This is the kind of thoughtful profiling comment that demonstrates engineering discipline.

---

## Points of Contention

**Vangelis vs. Tomita — LFO Rate User Control**
- Vangelis: The fixed 0.5 Hz LFO rate for membrane cutoff modulation is a missed expressive opportunity. A performer routing a vocalist through OSMOSIS should be able to set the membrane breathing rate to match the performer's phrase rhythm — slow for ballads, faster for up-tempo material.
- Tomita: Exposing the LFO rate adds a parameter to a 4-parameter engine that is designed for operational simplicity. The membrane should breathe automatically; the performer's focus should be on PERMEABILITY, SELECTIVITY, REACTIVITY, and MEMORY.
- Resolution: Vangelis wins on principle but Tomita wins on current scope. Adding LFO rate as a 5th parameter (osmo_lfoRate) would resolve the D005 gap without creating complexity. This is a P1 for the next patch.

**Buchla vs. Moog — Coupling Output Granularity**
- Buchla: `getSampleForCoupling()` returns only the membrane-filtered stereo output. The 4 band-RMS values are computed but not individually exposed as coupling outputs. An engine should be able to couple to OSMOSIS's sub band specifically, not just the full-range membrane output.
- Moog: Per-band coupling output would require the `CouplingType` enum to include band-specific types. That is a fleet-level API decision, not an OSMOSIS decision. OSMOSIS correctly exposes what the current API allows.
- Resolution: Moog is right about scope. Band-specific coupling output is a fleet architecture evolution (new `CouplingType` enum values), not an OSMOSIS bug.

---

## The Prophecy

OSMOSIS is the fleet's fourth paradigm-break after OPTIC (zero-audio synthesis), ONSET (percussion as primary voice), and ORGANON (metabolism as DSP). Where OPTIC generates synthesis without sound, OSMOSIS generates coupling from sound it did not make. It is the ear of the fleet — the membrane that translates acoustic reality into synthesis language.

The engine's score reflects its paradigm rather than its execution: 7.4/10 is high for an analysis engine. The 4-band spectral split, matched-Z envelope follower, block-rate-optimized membrane filter, and fix #303 coupling route are all correctly implemented. The D005/D006 doctrine gaps are inherent to the analysis paradigm rather than implementation failures — OSMOSIS has no MIDI note processing by design, just as OPTIC has no audio output by design.

The engine's next evolution requires two additions: an `osmo_lfoRate` parameter (resolves D005 gap with minimal complexity) and band-specific coupling outputs (requires fleet-level API work). With these, OSMOSIS becomes a 9.0+ engine that rivals OPTIC in paradigm originality.

The performer routing a vocalist, drum machine, or turntable through OSMOSIS does something no conventional synthesizer allows: they give the fleet a body to respond to. The fleet does not know the performer exists; OSMOSIS is how the fleet learns to breathe with them.

---

## Blessings & Warnings

| Ghost | Blessing | Warning |
|-------|----------|---------|
| Moog | Matched-Z envelope follower: `exp(-1/(sr*ms*0.001f))` — correct at all sample rates | Fixed LFO rate (0.5 Hz) is not user-configurable; D005 gap |
| Buchla | 4-band spectral split: sub/lo-mid/mid/high RMS exposed as independent coupling outputs | Band-RMS values not individually exposed via `getSampleForCoupling()` — lost coupling granularity |
| Smith | Fix #303: coupling-input-to-permeability route is architecturally coherent | Only 4 parameters total — deeper control requires osmo_lfoRate addition |
| Kakehashi | PERMEABILITY/SELECTIVITY/REACTIVITY/MEMORY — textbook macro naming for a membrane engine | No UI feedback for detected pitch or band RMS values; performers cannot see what OSMOSIS is hearing |
| Pearlman | Block-rate fastExp optimization correctly removes per-sample transcendental — explicit comment in source | Autocorrelation pitch detection runs mono (uses `(inL + inR) * 0.5f`); stereo pitch content not captured |
| Tomita | CytomicSVF for 4-band crossovers — correct, stable, sample-rate-independent | No user control over crossover frequencies (fixed 200/1k/5k Hz) |
| Vangelis | Fix #303 call-and-response dynamic: membrane opens when synthesis pushes against it | D006 intentional gap: no conventional MIDI CC expression routes (analysis engine exemption) |
| Schulze | Memory parameter 10–1000ms: membrane holds the imprint of what passed through it | Pitch detection range 50–2000 Hz may miss sub-bass content below 50 Hz |

---

## What the Ghosts Would Build Next

| Ghost | Next Addition |
|-------|--------------|
| Moog | `osmo_lfoRate` parameter (0.01–10 Hz) — resolve D005 gap, enable phrase-synchronous membrane breathing |
| Buchla | Per-band coupling output: expose sub/lo-mid/mid/high RMS individually via new CouplingType enums |
| Smith | UI visualizer: real-time band-RMS meter + detected pitch readout — performers need to see what OSMOSIS hears |
| Kakehashi | MIDI clock sync for LFO: membrane breathing synced to host BPM (1/4, 1/2, 1/1 note rates) |
| Pearlman | Stereo pitch detection: run autocorrelation on L and R independently, expose stereo pitch difference as coupling signal |
| Tomita | User-configurable crossover frequencies (low/mid/high): adapt spectral split to source material |
| Vangelis | Onset detector: trigger coupling burst on transients (kick, snare, pluck) independent of sustained envelope |
| Schulze | Extended memory range to 5–10 seconds: the membrane remembers long sonic events |

---

## Seance Score Breakdown

| Dimension | Score | Notes |
|-----------|-------|-------|
| Architecture originality | 9/10 | Analysis engine paradigm (like OPTIC B005): translates acoustic performance to coupling language |
| DSP implementation quality | 8/10 | Matched-Z envelope follower; CytomicSVF crossovers; block-rate membrane optimization; fix #303 |
| Parameter design | 8/10 | 4 macros perfectly named (PERMEABILITY, SELECTIVITY, REACTIVITY, MEMORY). Missing osmo_lfoRate (D005 gap) |
| Coupling architecture | 8/10 | 4-band RMS computed; fix #303 coupling-to-permeability; LFO-modulated membrane. Band-specific coupling not yet exposed |
| Doctrine compliance | 6/10 | D001/D006 intentionally N/A (analysis paradigm); D002/D005 partial (no LFO rate control). OPTIC precedent applies |
| First-take usability | 7/10 | 7 factory presets is thin; engine requires external audio source to be useful; routing setup non-trivial |
| Fleet integration potential | 9/10 | 4-band spectral split makes OSMOSIS the highest-quality coupling source for external audio in any fleet |
| Preset library | 5/10 | 7 presets covering basic membrane characters; needs 20+ covering different external source types |
| **TOTAL** | **7.4/10** | |

---

*First seance summoned by the Medium on 2026-04-03. Ghosts: Moog, Buchla, Smith, Kakehashi, Pearlman, Tomita, Vangelis, Schulze. Issue #118 resolved. D005/D006 gaps noted for osmo_lfoRate P1 addition.*
