# The Verdict -- ORGANISM (Re-Seance)
**Seance Date**: 2026-04-03
**Engine**: ORGANISM | Cellular Automata Generative Synthesizer
**Identity**: The Coral Colony | Emergence Lime `#C6E377`
**Param prefix**: `org_` | 24 parameters | Monophonic
**Prior Score**: 8.1/10 (2026-03-19) → 7.2/10 (re-seance, CRITICAL filter bug) → **8.7/10** (post-fix)
**DSP Fixes Since Prior Seance**: CytomicSVF (TPT) replaces biquad bilinear LPF; reverb SR-scaling; sample-rate-correct cell-smooth coefficient; caState XOR seed per note; pitch bend wired; PolyBLEP on all oscillators.

---

## What Changed Since March 19

| Fix | Commit | Impact |
|-----|--------|--------|
| Replaced biquad with CytomicSVF (matched-Z) | Phantom Sniff pass 3 | Eliminates cutoff instability at 96 kHz; no more 3200 Hz jump |
| Reverb delay lengths SR-scaled at runtime (`kRefSampleRate / sampleRate`) | DSP fixes wave | Room size correct at 44.1/48/96 kHz |
| `cellSmoothCoeff = 1 - exp(-1/(0.003 * sr))` — sample-rate-correct | DSP fixes wave | Smooth coefficient was hardcoded; now scales with host SR |
| `caState ^= (note * 257u)` — per-note seed XOR | Phantom Sniff pass | Different MIDI notes now produce distinct evolution trajectories |
| Pitch bend now parsed and applied | Phantom Sniff pass 3 | D006 completion |
| PolyBLEP anti-aliasing on saw, square, sub oscillators | PolyBLEP & SR fixes | Clean alias-free tone up to Nyquist |
| 397 factory presets across moods | Preset forge sessions | D004 preset coverage resolved |

---

## The Council Has Spoken

| Ghost | Core Observation |
|-------|-----------------|
| **Moog** | The CytomicSVF is the correct fix. The bilinear-transform biquad was producing 3200 Hz cutoff spikes when the automaton flipped cells rapidly — the TPT topology is unconditionally stable and tracks the cutoff changes without the aliasing artifact. This is how a filter should behave in a dynamic-modulation context. |
| **Buchla** | The per-note XOR seed is a quiet genius move: `caState = seed ^ (note * 257u)`. Every MIDI pitch now initializes a different automaton trajectory, so a C4 and a G4 evolve differently from the same preset. The monophonic voice constraint is transformed from a limitation into a feature — you are always hearing one specific colony, chosen by the note you play. |
| **Smith** | The coupling architecture is now fully bidirectional: `AmpToFilter` drives ±1000 Hz modulation on the cell-derived cutoff; `AmpToPitch` drives pitch offset. `macroCoupling` scales receive sensitivity (0.5 + macro * 0.5). The autonomous mutation boost from `macroCoupling * 0.01` is the right level — audible on close listening, never dominant. |
| **Kakehashi** | 397 factory presets changes everything. The engine that was a fascinating algorithm is now a playable instrument. Rule 110 with scope=4 at medium step rate is the new Init Patch, and it sounds alive on the first keypress. The preset library demonstrates the full behavioral range: from Rule 30 (chaotic, percussive) to Rule 184 (traffic flow, rhythmic) to Rule 90 (fractal, slow). |
| **Pearlman** | The PolyBLEP correction on the saw and square oscillators is felt, not heard. At low step rates (0.5 Hz) the tonal quality is substantially cleaner. The sub oscillator's half-frequency square also has proper PolyBLEP on both the fundamental and duty crossings. The oscillator section is now honest. |
| **Tomita** | The reverb fix is consequential. At 96 kHz, the old hardcoded comb lengths created a room half the intended size — the new SR-scaling keeps the reverb spatial character consistent across all host sample rates. The 4-comb + 4-allpass Schroeder topology (both stereo-L and stereo-R) was always competent; now it is also portable. |
| **Vangelis** | The OrgScopeHistory system is still the engine's quiet masterpiece, and 397 presets now demonstrate its full range. At scope=1 it is raw and percussive — individual cell flips create hard timbral transients. At scope=16 it is glacial — the automaton's decisions emerge as long, breathing contours. The scope parameter is one of the most expressive controls in the fleet once you understand it. |
| **Schulze** | Pitch bend is now wired: `rootFreq * PitchBendUtil::semitonesToFreqRatio(bend * 2.0f)`. This matters for Berlin School technique — gliding between automaton states via pitch bend creates morphological transitions that feel like watching a colony evolve under changing conditions. The step rate range 0.5–32 Hz spans three orders of magnitude of temporal behavior. |

---

## Doctrine Compliance

| Doctrine | Status | Evidence |
|----------|--------|----------|
| D001 | PASS | `velCutoffBoost = currentVel * velCutoff * 3000.f` — velocity adds up to +3000 Hz to CytomicSVF cutoff, scaled by `org_velCutoff` depth parameter. Timbre change is pronounced and stable. |
| D002 | PASS | LFO1 (0.01–10 Hz) sweeps step rate (CA evolution speed); LFO2 (0.01–10 Hz) offsets filter cutoff. Mod wheel wired (CC1 morphs rule index +/- 2 positions). Aftertouch wired (mutation rate override ×0.3). 4 working macros: RULE, SEED, COUPLING, MUTATE. CA quadrants function as 4 mod matrix channels. |
| D003 | PASS | 1D elementary CA: Wolfram neighborhood index `(left<<2)|(center<<1)|right`, rule byte lookup, 16-cell circular wrap. CytomicSVF (TPT) is correct matched-Z design. PolyBLEP anti-aliasing. LCG RNG for mutation. All DSP mathematically grounded. |
| D004 | PASS | All 24 `org_` parameters live in `renderBlock()`. `org_macroCoupling`: scales `recvScale = 0.5f + macro * 0.5f` and adds `macro * 0.01f` autonomous mutation. `org_macroSeed`: latch-re-seeds via LCG. No dead parameters. |
| D005 | PASS | Both LFOs: `nr(0.01f, 10.f)` — 100-second minimum cycle. Step rate floor 0.5 Hz. |
| D006 | PASS | CC1 → rule morph (+/- 2 rule indices). Aftertouch → mutation rate. Velocity → timbre (D001). Pitch bend → root frequency. Four independent expression axes. |

---

## Points of Agreement (3+ ghosts converged)

1. **The CytomicSVF fix resolves the CRITICAL** (Moog, Tomita, Smith, Pearlman) — The bilinear biquad was collapsing under rapid CA-driven cutoff changes because discrete-time biquad coefficients require smooth parameter changes; the TPT SVF handles discontinuous cutoff updates without instability. This was the primary reason the engine re-seanced at 7.2/10.

2. **Per-note XOR seed is semantically perfect** (Buchla, Vangelis, Schulze, Kakehashi) — A generative engine that produces the same evolution regardless of note choice is a one-trick pony. The XOR with `note * 257u` ensures every pitch launches a different colony. Combined with the curated rule set, the 88 MIDI notes each become entry points into distinct generative landscapes.

3. **397 presets transform the instrument** (Kakehashi, Pearlman, Vangelis) — The original 8.1/10 score included a 0/10 for presets. With 397 presets across Foundation, Atmosphere, Kinetic, Organic, and Coupling moods, new users can immediately access the engine's behavioral range without expertise in cellular automata.

4. **Reverb SR-scaling is the correct fix** (Tomita, Moog, Schulze) — The Schroeder reverb now maintains consistent room geometry at all sample rates. The stereo implementation (separate L and R delay buffers with slightly offset comb lengths) provides natural stereo diffusion without artificial processing.

---

## Points of Contention

**Buchla vs. Pearlman — Oscillator Scope**
- Buchla: The engine now has PolyBLEP oscillators but still only three waveforms (saw/square/tri). A noise waveform would give the CA percussive material to shape; the CA acting on noise produces crackling, breathing textures unavailable from tonal waveforms.
- Pearlman: Noise is for a future wave. The current three waveforms cover tonal, square, and triangular harmonic profiles — each responds differently to the CA's timbral shaping. Adding noise increases oscillator design complexity without resolving any current doctrine violation.
- Resolution: Pearlman is right for current scope. Noise waveform is a valid V2 addition.

**Moog vs. Schulze — Key Tracking**
- Moog: The CytomicSVF fix is correct but does not address the original key tracking gap. A bright preset at C4 is still disproportionately dark at C6 because the CA-derived cutoff range (200–8000 Hz) does not scale with note frequency.
- Schulze: Key tracking in a generative engine is philosophically wrong. The automaton's timbral decisions should not be overridden by the performer's note choice. The per-note XOR seed is the correct idiom — different notes produce different behaviors, not scaled brightness.
- Resolution: Schulze makes the stronger argument for this engine. Key tracking belongs to conventional subtractive synthesis; the CA already provides per-note timbral variation via seed XOR.

---

## The Prophecy

ORGANISM at 8.7/10 is now a genuine instrument, not merely a fascinating algorithm. The three critical changes — CytomicSVF replacing the unstable biquad, per-note XOR seeding, and 397 factory presets — have collectively transformed the engine from an ambitious experiment into a playable, expressive, doctrine-compliant voice in the fleet.

The cellular automaton is still the synthesis, not a modulation source. The reverb fix and PolyBLEP corrections have removed the remaining artifacts that interrupted the automaton's behavioral expression. A performer can now explore the full range: Rule 30 (Wolfram Class III chaos) at high mutation rates produces unpredictable, clicking textures; Rule 110 (Turing-complete) at medium step rate and scope=8 produces slow, breathing organic movement; Rule 184 (traffic flow) at 8 Hz step rate with scope=2 produces rhythmic timbral patterns.

The path to 9.0 is narrow: it requires polyphonic voice allocation (two independent automata in chord mode) and an extended scope range (1–32 generations). The current implementation is maximally polished for its monophonic paradigm.

---

## Blessings & Warnings

| Ghost | Blessing | Warning |
|-------|----------|---------|
| Moog | CytomicSVF (TPT) correctly handles rapid cutoff changes — unconditional stability | Still no key tracking; high-register notes remain proportionally dark |
| Buchla | Per-note XOR seed: every MIDI pitch launches a distinct colony trajectory | Rule-blend per-bit majority vote has discontinuity at 0.5 threshold (by design) |
| Smith | Coupling receive sensitivity scales with COUPLING macro — musical responsiveness | Only AmpToFilter and AmpToPitch receive types wired; EnvToMorph not used |
| Kakehashi | 397 presets: the engine is now demonstrable to non-expert users | RULE/SEED macro labels remain engine-specific jargon; UI tooltips needed |
| Pearlman | PolyBLEP on all oscillators — clean, alias-free tone throughout the MIDI range | Only 3 waveforms (saw/square/tri); noise waveform would expand generative territory |
| Tomita | Reverb SR-scaled: room size stable at 44.1/48/88.2/96 kHz | Reverb decay is fixed (no user control); cells 12–15 modulate send level only |
| Vangelis | OrgScopeHistory scope range 1–16: from raw cellular transients to glacial evolution | Scope ceiling 16 is still conservative; 32–64 would enable ultra-slow morphology |
| Schulze | Step rate 0.5–32 Hz + LFO 0.01 Hz floor + pitch bend = full temporal range | Pitch bend range not user-configurable (hardcoded ±2 semitones) |

---

## What the Ghosts Would Build Next

| Ghost | Next Addition |
|-------|--------------|
| Moog | Filter key tracking parameter — scaling CA cutoff range with MIDI note |
| Buchla | Noise oscillator option — CA acting on noise for percussive texture generation |
| Smith | EnvToMorph coupling receive — another engine's envelope sweeps the scope parameter |
| Kakehashi | UI tooltip: "RULE = which law of nature governs the colony" |
| Pearlman | Configurable pitch bend range parameter (±1 to ±12 semitones) |
| Tomita | Reverb decay parameter (0.3–8s) mapped per cells 12–15 average |
| Vangelis | Extended scope range (1–32 or 1–64 generations) for ultra-smooth glacial evolution |
| Schulze | Step rate audio-rate extension (32–500 Hz) — CA at audio rate |

---

## Seance Score Breakdown

| Dimension | Prior Score (03-19) | Current Score | Notes |
|-----------|---------------------|---------------|-------|
| Architecture originality | 9/10 | 9/10 | Cellular automaton as synthesis core — unchanged, still genuinely novel |
| Filter quality | 7/10 | 9/10 | CytomicSVF (TPT) — correct matched-Z, unconditionally stable under rapid modulation |
| Source originality | 6/10 | 7/10 | PolyBLEP now clean; still 3 waveforms only |
| Expressiveness | 9/10 | 9/10 | Pitch bend now wired; expression complete |
| Spatial depth | 5/10 | 7/10 | SR-scaled reverb, stereo L/R offset — correct at all sample rates |
| Preset library | 0/10 | 9/10 | 397 presets across 5 moods |
| Temporal depth | 9/10 | 9/10 | Step rate 0.5–32 Hz + LFO 0.01 Hz + scope 1–16 + freeze |
| Coupling architecture | 7/10 | 8/10 | AmpToFilter ±1000 Hz; AmpToPitch; macroCoupling scales sensitivity |
| Generative integrity | 10/10 | 10/10 | Wolfram CA correctly implemented; curated rules; XOR per-note seeding; scope smoothing |
| First-take accessibility | 8/10 | 9/10 | 397 presets + alive Init patch (Rule 110, XOR seed 42 ^ note) |
| **TOTAL** | **8.1/10** | **8.7/10** | |

---

*Re-summoned by the Medium on 2026-04-03. Ghosts: Moog, Buchla, Smith, Kakehashi, Pearlman, Tomita, Vangelis, Schulze. Prior verdicts: 2026-03-17 (first seance, Conway/Wolfram panel), 2026-03-19 (8.1/10), CRITICAL re-seance 7.2/10 (filter bug). All superseded by this verdict.*
