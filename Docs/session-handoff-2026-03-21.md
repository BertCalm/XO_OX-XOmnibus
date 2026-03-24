# Session Handoff — 2026-03-21
**From:** Extended overnight + morning session (2026-03-20 evening → 2026-03-21)
**To:** Next session (2026-03-22 or later)
**Repo:** `~/Documents/GitHub/XO_OX-XOlokun/`
**Branch:** `main`

---

## 1. Session Summary

This session extended the previous arc with infrastructure work, a new engine, Guru Bin retreats,
and a large quantity of preset generation for the three newest engines (ORBWEAVE, OVERTONE, ORGANISM).
The Coupling Performance System — a category-defining live performance architecture — was also
designed and partially implemented. The session closed with a substantial number of uncommitted
changes that represent the "overnight work" dispatched as parallel agents.

### High-Level Output

| Category | Count / Status |
|----------|---------------|
| New shared DSP utilities (Source/DSP/) | 7 total (6 from prev session + StandardADSR added) |
| New engine: OWARE | Engine #44, 911 lines, 7 pillars, 9.8 target |
| Engine migrations to shared utilities | 22 engines (see Section 4) |
| New Guru Bin retreats | 4: OWARE, ORBWEAVE, OVERTONE, ORGANISM |
| OWARE seance | 8.4/10 current → 9.2 with 2 fixes |
| New presets (untracked) | ~129 new .xometa files across 5 moods |
| New docs (untracked) | 10+ new files |
| New core/UI files | CouplingCrossfader.h, PerformanceViewPanel.h |
| Committed changes (2026-03-21) | 2 commits: Coupling Performance System + skill-friction-detective |
| Uncommitted changes | 36 modified tracked files + 129 untracked files |

---

## 2. New Infrastructure: 7 Shared DSP Utilities in Source/DSP/

All 7 utilities are committed. They live in `/Source/DSP/` under the `xolokun` namespace.
All are allocation-free, noexcept, and real-time safe.

| Header | Purpose | Fleet Coverage |
|--------|---------|----------------|
| `StandardLFO.h` | 5-shape LFO, D005-compliant (floor 0.005 Hz), deterministic S&H via Knuth TAOCP LCG, phase offset for ensemble staggering | Consolidates pattern from 21+ engines |
| `PitchBendUtil.h` | 14-bit MIDI → bipolar → semitones → freq ratio pipeline. Inline/constexpr. One-liner: `PitchBendUtil::freqRatio(msg.getPitchWheelValue(), 2.0f)` | Consolidates pattern from 10+ engines |
| `FilterEnvelope.h` | Independent ADSR for filter/pitch modulation. Linear attack, exponential decay/release. The primary seance finding across the fleet: bass programming requires a filter envelope separate from amp ADSR | Newly available to all engines |
| `VoiceAllocator.h` | LRU voice stealing + ReleasePriority strategy. Works with any voice array that has `bool active` + `uint64_t startTime` fields | Consolidates pattern from 18+ engines |
| `GlideProcessor.h` | Portamento/glide in Hz space. Analog synth convention (upward glides feel naturally slower). Time-constant-based coefficient | Consolidates pattern from 35+ engines |
| `ParameterSmoother.h` | One-pole smoothing, 5ms default. Zipper-free automation. `smoothed += (target - smoothed) * coeff` pattern | Consolidates pattern from 13+ engines |
| `StandardADSR.h` | Shared amplitude envelope. Three shapes: AD, AHD, ADSR. Linear attack, exponential decay/release. Based on OnsetEnvelope (most battle-tested in fleet). Drop-in replacement for OrcaADSR, OctoADSR, OracleADSR, ObscuraADSR, OuieADSR, and 15+ others | Targets 20+ engines |

**Key design notes:**
- Utilities are `struct`s, not base classes (Seance ruling: opt-in, not inherited hierarchy)
- `StandardADSR` was the last utility added this session (commit `ec7b52b`)
- `FilterEnvelope` is distinct from `StandardADSR` — it is for filter/pitch modulation targets, not amp

---

## 3. New Engine: OWARE (#44) — The Resonant Board

**Source:** `Source/Engines/Oware/OwareEngine.h` (911 lines)
**Accent:** Akan Goldweight `#B5883E`
**Parameter prefix:** `owr_` (frozen, 22 parameters)
**Polyphony:** 8 voices
**Seance score:** 8.4/10 current → 9.2/10 with two targeted fixes (see Section 8)

### The 7 Pillars

1. **Material Continuum** — 4 material tables (wood/metal/bell/bowl) interpolated through a
   tri-segment morph. Material exponent alpha from beam dispersion theory (Fletcher & Rossing §2.3)
   controls per-mode differential decay. Wood upper modes die fast; metal rings equally; bowl ratios
   leave the harmonic series entirely.

2. **Mallet Physics** — Full Chaigne contact model (cited: Chaigne & Doutaut 1997). Contact duration
   as function of hardness (0.5 ms hard, 5 ms soft), sinusoidal force pulse, noise mix via hardness²,
   spectral lowpass via matched-Z IIR. Physical mallet bounce at 15-25 ms for soft strikes.
   Ghost council called it "Chaigne with footnotes."

3. **Sympathetic Resonance Network** — Per-mode frequency-selective coupling. Each active voice checks
   every mode of every other voice. If any mode of voice B is within 50 Hz of a mode of voice A,
   voice B's output feeds into voice A's mode input weighted by `(1 - dist/50) * sympathy * 0.03`.
   Spectrum-based, not amplitude-based. "Rossing-style sympathetic resonance, not a global reverb
   approximation." (Pearlman)

4. **Resonator Body** — Tube/frame/bowl/open body types with Gaussian proximity decay boost.
   Body-membrane coupling varies by type.

5. **Buzz Membrane** — Balafon spider-silk mirliton: BPF extraction (200-800 Hz band) + tanh
   nonlinearity + re-injection. Body-type-specific buzz frequency (gourd = 300 Hz, frame = 150 Hz,
   metal = 500 Hz). Ethnographically grounded. Currently defaults to 0.0 — users never hear it
   without dialing it in.

6. **Breathing Gamelan** — Balinese beat-frequency shimmer via shadow voice detuning. Rate in Hz
   (not ratio-based, matching gamelan tuning practice). Currently hardcoded at 0.3 Hz per voice
   in renderBlock — the declared `owr_shimmerRate` parameter is not connected.

7. **Thermal Drift** — Shared slow tuning drift toward random targets every ~4 seconds (max ±8 cents).
   Per-voice personality seed from PRNG at prepare-time — each of the 8 voices has stable individual
   tuning character. 100-second time constant at 48 kHz. "A genuine long-form temporal process,
   not vibrato." (Schulze)

### Registration Status
- Registered in `Source/XOlokunProcessor.cpp` (PASS)
- CLAUDE.md updated: count, module list, engine table, parameter prefix table, key files (PASS)
- 20 factory presets committed across 5 moods
- Post-completion checklist: `Docs/post-completion/oware-completion-2026-03-21.md` (PASS with 2 blocking findings)

### Blessing Candidates (from seance)
- **BC-OWARE-01: Mallet Articulation Stack** — Chaigne contact model with 3 parallel timbre paths from one gesture
- **BC-OWARE-02: Living Tuning Grid** — Per-voice thermal personality + shared drift; engine feels alive when nobody plays
- **BC-OWARE-03: Per-Mode Sympathetic Network** — Spectrum-based resonance coupling, not amplitude-based

---

## 4. Engine Migrations: Which Engines Now Use Shared Utilities

The following engines have been updated to use one or more shared DSP utilities.
This is confirmed by `#include` grep across `Source/Engines/`.

**Engines confirmed using shared DSP utilities (22 total):**

| Engine | Utilities Used |
|--------|---------------|
| Bite | StandardLFO, VoiceAllocator |
| Bob | StandardLFO, GlideProcessor, VoiceAllocator |
| Drift | StandardLFO, PitchBendUtil, GlideProcessor, ParameterSmoother |
| Dub | StandardLFO, ParameterSmoother |
| Fat | StandardLFO, VoiceAllocator |
| Morph | StandardLFO, GlideProcessor |
| Oblique | PitchBendUtil |
| Obscura | StandardLFO, FilterEnvelope |
| Obsidian | StandardLFO |
| Oceanic | StandardLFO |
| Octopus | StandardLFO, VoiceAllocator, GlideProcessor |
| OpenSky | StandardLFO, VoiceAllocator |
| Oracle | StandardLFO |
| Orca | StandardLFO, StandardADSR |
| Origami | StandardLFO, VoiceAllocator, GlideProcessor, ParameterSmoother (proof-of-concept migration) |
| Osprey | StandardLFO, FilterEnvelope |
| Osteria | StandardLFO, FilterEnvelope |
| Ostinato | StandardLFO |
| Ouie | StandardLFO, VoiceAllocator |
| Oware | StandardLFO, FilterEnvelope, GlideProcessor, ParameterSmoother, VoiceAllocator, PitchBendUtil (flagship — uses all 6 applicable utilities) |
| Oxbow | StandardLFO |
| Snap | StandardLFO |

**CRITICAL:** These migrations are all in the `36 modified tracked files` category — they have NOT
been committed. They represent the primary build risk for next session.

---

## 5. Fleet Status: 44 Engines

### Engine Count
**44 registered engines** as of 2026-03-20/21.

The 5 newest: ORBWEAVE (2026-03-20), OVERTONE (2026-03-20), ORGANISM (2026-03-20),
OXBOW (2026-03-20), OWARE (2026-03-20).

### Preset Census (as of 2026-03-21)
Source: `Docs/preset-census-2026-03-21.md`

| Metric | Value |
|--------|-------|
| Total .xometa files | 15,680 (committed) + ~129 (untracked) |
| Engine appearances | 22,407 |
| Distinct engines | 44 |
| Average appearances per engine | 509.2 |
| Engines below 150-preset target | **2** |

**Engines below 150-preset target:**
| Engine | Count | Status |
|--------|-------|--------|
| OWARE | 20 | CRITICAL — needs immediate expansion |
| OXBOW | 150 | AT TARGET (exactly) |

**Bottom 5 by appearances:**
1. Oware — 20 (CRITICAL)
2. Oxbow — 150 (at target)
3. Ostinato — 197
4. OceanDeep — 296
5. Overtone — 326

**Note:** The 129 untracked preset files (ORBWEAVE, OVERTONE, ORGANISM, OBRIX, OWARE
Awakening/Foundation/Prism/Atmosphere/Flux presets) are NOT counted in the 15,680 figure.
When committed, these will significantly lift ORBWEAVE, OVERTONE, and ORGANISM counts.

### Mood Distribution (committed presets)
Entangled dominates at 28.7% (4,502 files). Submerged is weakest at 1.4% (223 files).

### Fleet Seance Summary (post-2026-03-20 fixes)
- Fleet average: ~8.6/10 (up from ~7.2 at start of previous session)
- Top tier: OVERBITE 9.2, OBSCURA 9.1, OUROBOROS 9.0
- OWARE: 8.4/10 current → 9.2 projected
- Full scores: `Docs/fleet-seance-scores-2026-03-20.md`

### Build Status
Last verified build + auval PASS: 2026-03-20 (4× verified, 42 engines).
**Engine #44 OWARE has not been through a fresh build + auval cycle.**
Migration changes (36 modified engine files) have NOT been build-tested.

---

## 6. Overnight Work: What Was Dispatched

The following work was produced by agents overnight (all uncommitted):

### Committed Today (2026-03-21)
1. `3efc16c` — Coupling Performance System: `CouplingCrossfader.h` + `PerformanceViewPanel.h` +
   `Docs/specs/coupling_performance_spec.md`
2. `576f56360` — `/skill-friction-detective` meta-skill

### Untracked Files (produced overnight, not committed)

**Docs — Analysis and Research:**
- `Docs/architect-review-oware-2026-03-21.md` — Five Provinces review of OWARE, 2 blocking findings
- `Docs/dsp-profiler-heavy-engines-2026-03-21.md` — Static analysis of ORGANON, OSTINATO, OWARE
- `Docs/engine-comparator-percussion-trio-2026-03-21.md` — ONSET × OSTINATO × OWARE side-by-side
- `Docs/field-guide-editorial-plan-2026-03-21.md` — Status of all 44 engines in Field Guide (14 published, 30 planned)
- `Docs/preset-census-2026-03-21.md` — Authoritative fleet count (15,680 presets, 44 engines)
- `Docs/xoxbow-sound-design-guide.md` — OXBOW sound design reference

**Docs — Seances:**
- `Docs/seances/oware-seance-2026-03-21.md` — OWARE seance verdict: 8.4/10, 3 blessing candidates,
  2 critical bugs (D004 dead LFOs, hardcoded shimmer rate)

**Docs — Guru Bin Retreats:**
- `Docs/guru-bin-retreats/oware-retreat-2026-03-21.md`
- `Docs/guru-bin-retreats/orbweave-retreat-2026-03-21.md`
- `Docs/guru-bin-retreats/overtone-retreat-2026-03-21.md`
- `Docs/guru-bin-retreats/organism-retreat-2026-03-21.md`

**Docs — Post-Completion:**
- `Docs/post-completion/oware-completion-2026-03-21.md` — OWARE registration checklist (PASS with 2 findings)
- `Docs/post-completion/oxbow-completion-2026-03-21.md` — OXBOW registration checklist (PASS)

**Docs — Specs and Recipes:**
- `Docs/specs/coupling_performance_spec.md` — Real-time performable coupling architecture
- `Docs/coupling-recipes/oware-coupling-cookbook-2026-03-21.md`

**Scripture:**
- `scripture/retreats/oware-retreat-2026-03-21.md`

**New Core/UI:**
- `Source/Core/CouplingCrossfader.h` — Real-time coupling overlay crossfader
- `Source/UI/PerformanceViewPanel.h` — Performance view UI panel

**Presets (untracked, ~129 files):**
- ORBWEAVE Awakening × 6 (Foundation, Atmosphere, Prism moods)
- OVERTONE Awakening × 8 (Foundation, Atmosphere, Prism moods)
- ORGANISM Awakening × 8 (Foundation, Atmosphere, Flux moods)
- OBRIX expansion × 16 (Aether, Atmosphere, Entangled, Family, Flux, Foundation, Prism, Submerged)
- OWARE coupled × 3 (Entangled: Oware×Onset, Oware×Osprey, Oware×Ostinato)
- Atmospheric pads × 12 (generic Atmosphere/Foundation presets)

---

## 7. What to Check in Next Session

Run these checks in order before any new work begins.

### 7.1 Git Status — Uncommitted Changes

```bash
git status --short
git log --oneline -5
```

**Expected:** 36 modified tracked files + ~129 untracked files.
The untracked files are all safe to stage. The modified tracked files carry the migration changes
and need build verification before committing.

**Modified tracked files that matter most:**
- `Source/Engines/*/` (22 engines with shared utility migrations) — BUILD TEST FIRST
- `Source/XOlokunProcessor.cpp` / `.h` — OWARE registration (likely committed in CLAUDE.md update)
- `CLAUDE.md` — B016 amendment applied by a linter this session
- `Presets/XOlokun/Atmosphere/Tidal_Membrane.xometa` — a single preset was modified

**B016 AMENDMENT NOTE:** The Seance amended Blessing B016 (OBRIX Brick Independence) during this
session. The new text clarifies that MIDI-layer voice independence is inviolable, but synthesis-layer
interdependence (shared JI attractor field, cross-voice amplitude ecology, environmental globals)
is explicitly permitted. The CLAUDE.md change is already in the working tree. Do not revert it.

### 7.2 Build Verification After Migrations

```bash
eval "$(fnm env)" && fnm use 20
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
# Expect: 0 errors, 0 warnings on new shared DSP includes
```

**Primary risk:** The 22 engine migrations replace inline patterns with `#include` directives.
The replacement types must match the old struct layouts. Most likely issues:
- Stage enum name collision (if an engine defined its own `Stage` enum matching `StandardADSR::Stage`)
- `VoiceAllocator::findFreeVoice()` template instantiation failures if voice structs have non-standard
  field names

After build passes, run auval:
```bash
auval -v aumu XOMn Xa_X
```

### 7.3 Preset QA Results

After committing the ~129 untracked presets, run the census script to get the updated count:
```bash
# Count all .xometa files
find Presets/XOlokun -name "*.xometa" | wc -l
```

Check for the two critical preset coverage gaps:
- **OWARE: 20 presets** — CRITICAL. Needs expansion to 150 minimum. The retreat and seance
  both generated recipe material. Plan at least 130 new OWARE presets next session.
- **OXBOW: 150 presets** — at target exactly. OK for now.

Check ORBWEAVE, OVERTONE, ORGANISM after the new untracked presets are committed — they should
each gain 6-8 Awakening presets from the overnight work.

### 7.4 Guru Bin Retreat Findings

Four retreats were written overnight. Summary of actionable findings:

**OWARE Retreat** (`Docs/guru-bin-retreats/oware-retreat-2026-03-21.md`):
- Engine-level LFO objects are absent. Two `StandardLFO` members at engine scope are needed.
- Shimmer rate hardcoded at 0.3 Hz — must read `owr_shimmerRate` and `owr_lfo1Rate`/`owr_lfo2Rate`
- Buzz membrane defaults to 0.0 — consider changing default to 0.15 to demonstrate balafon character
- These are blocking for the 9.2 target score

**ORBWEAVE Retreat** (`Docs/guru-bin-retreats/orbweave-retreat-2026-03-21.md`):
- "The engine is not being explored. It is being decorated." — the Default Trap in motion.
  braidDepth defaulting to 0.5 means neither coupled nor decoupled character is heard.
- KNOT macro sat at 0.0 in half the library — the flagship feature is not being exercised.
- Recommended fix: lower braidDepth default to 0.2 (decoupled character first, coupling as reward).
- Integer FDN delay lengths cause audible pitch stepping when sweeping delayBase — flagged at seance.
  Still unaddressed. V2 backlog unless it becomes a complaint.

**OVERTONE Retreat** (`Docs/guru-bin-retreats/overtone-retreat-2026-03-21.md`):
- Four ratio families (phi/pi/e/sqrt2) produce fundamentally different timbres. The retreat maps
  each family's character clearly. Phi (Fibonacci) is the most musical; sqrt(2) (Pell) is the
  most inharmonic.
- Seance re-scored it 7.6/10 (down from 8.1) due to Pi table spectral collapse at low depth
  and 1-voice-not-8-voice implementation. Patches were applied 2026-03-20.
  Verify these patches are in the working tree.

**ORGANISM Retreat** (`Docs/guru-bin-retreats/organism-retreat-2026-03-21.md`):
- Rule selection guide now documented: Rule 110 (Turing-complete, perpetual movement), Rule 90
  (Sierpinski, fractal periodicity), Rule 30 (chaotic, noise-like), Rule 184 (traffic), Rule 150
  (additive, musical), Rule 18 (localized, sparse), Rule 54 (gliders), Rule 22 (medium chaos).
- CA filter cutoff jump (audible click) was patched 2026-03-20. Verify patch is committed.
- LCG seed space only 16 bits — degenerate states possible. V2 backlog.

### 7.5 Shared DSP Migration Status

**Priority for next session:** OWARE LFO fix is the highest-value single change.
The OWARE seance gives a clear roadmap to 9.2:
1. Add two `StandardLFO lfo1, lfo2` at engine scope (lines ~880)
2. In renderBlock: read `paramLfo1Rate/Depth/Shape` and apply LFO1 to pitch, LFO2 to filter brightness
3. Connect `owr_shimmerRate` to the per-voice `shimmerLFO.setRate()` instead of hardcoding 0.3f
4. Consider changing `owr_buzzAmount` default from 0.0 to 0.15

Full architect review: `Docs/architect-review-oware-2026-03-21.md`

---

## 8. Decision Points for Next Session

### 8.1 Which Guru Bin Findings to Act On

| Finding | Engine | Effort | Impact | Recommendation |
|---------|--------|--------|--------|----------------|
| LFO engine objects missing (D004 + D002) | OWARE | Low (30 min) | Score 8.4 → 9.2 | **DO FIRST** |
| Shimmer rate hardcoded | OWARE | Low (10 min) | D005 compliance | **DO WITH LFO FIX** |
| Buzz membrane default 0.0 | OWARE | Trivial (5 min) | Character discovery | **DO WITH ABOVE** |
| braidDepth default 0.5 trap | ORBWEAVE | Low (preset audit) | Preset quality | Do this session |
| KNOT macro underexercised | ORBWEAVE | Medium (preset batch) | Library coverage | Do this session |
| Integer FDN delay pitch stepping | ORBWEAVE | High (DSP rewrite) | Seance finding | V2 backlog |

### 8.2 Whether to Merge Engine Migrations (Build Test First)

**Decision:** Do NOT commit the 22 engine migration changes until a clean build is confirmed.

Order of operations:
1. Commit untracked docs and presets first (safe — no DSP changes)
2. Run build with current modified engine files
3. If build passes: commit engine migrations as a single atomic commit
4. If build fails: identify which engines have conflicts and fix before committing

**Origami** was the proof-of-concept migration (committed at `41b72ac`). If Origami compiles in
the current tree, the migration pattern is valid. The remaining 21 engines follow the same pattern.

### 8.3 Preset Expansion Priorities Based on Census

Priority order from census data:
1. **OWARE** — 130 presets needed to reach floor. Use the retreat's 5 recipe categories
   (metallic balafon, wood marimba, glass bowl, bronze bell, hybrid/morph) × 26 each.
2. **OXBOW** — at exactly 150. A Guru Bin retreat has not been written yet. Write retreat first,
   then expand to 200+ presets.
3. **OSTINATO** — 197 appearances. Needs ~53 more for comfortable cushion. The 8.7 re-seance
   score justifies a full expansion pass.
4. **OCEANDEEP** — 296 appearances. Lowest of the established-engine tier.
5. **OVERTONE** — 326 appearances. Awakening presets from overnight should help; check after commit.

### 8.4 Coupling Performance System — Next Steps

The architecture is designed and partially scaffolded:
- `Source/Core/CouplingCrossfader.h` — real-time overlay crossfader (uncommitted)
- `Source/UI/PerformanceViewPanel.h` — UI panel (uncommitted)
- `Docs/specs/coupling_performance_spec.md` — approved architecture spec

**Decision needed:** Is this a V1.2 feature or V2? The spec says "V1.2 → V2 (phased delivery)."
The coupling crossfader can be merged as an opt-in component without disrupting existing presets.
The PerformanceViewPanel requires UI integration work. This should be a standalone agenda item
before any implementation continues.

### 8.5 Field Guide Coverage — 30 Engines Without Individual Posts

`Docs/field-guide-editorial-plan-2026-03-21.md` maps all 44 engines. 15 have been published;
30 are "Coming Soon." The three newest engines (ORBWEAVE, OVERTONE, ORGANISM) and OXBOW/OWARE
have no individual posts planned. Consider whether to batch them (like Post #15 Great Awakening)
or give each a dedicated post. The content backlog (`Docs/content-backlog-6-month-2026.md`)
should be checked against the editorial plan to avoid duplication.

---

## 9. Recommended Model for Next Session Tasks

| Task | Model | Reasoning |
|------|-------|-----------|
| OWARE LFO fix (DSP code) | Sonnet / Medium | Localized change, well-specified by seance and architect review |
| Build verification | Sonnet / Medium | Diagnostic work, low creativity required |
| OWARE preset expansion (130 presets) | Sonnet / High or parallel agents | Large batch — use feedback-parallel-preset-agents pattern |
| Guru Bin retreat for OXBOW | Opus / High | Retreat writing requires deep creative and analytical synthesis |
| ORBWEAVE preset pass (KNOT macro) | Sonnet / Medium | Targeted preset writing guided by retreat |
| Coupling Performance System integration | Opus / High | Architectural decisions with fleet-wide implications |
| Field Guide post (ORBWEAVE or OVERTONE) | Opus / High | Long-form writing with technical depth |
| Commit / git housekeeping | Sonnet / Low | Mechanical operations |

For parallel preset batches of 15+ presets per engine, use the pattern documented in
`~/.claude/projects/-Users-joshuacramblet/memory/feedback-parallel-preset-agents.md`.

---

## 10. Key Files Index for Next Session

| What | Where |
|------|-------|
| OWARE seance verdict (2 bugs, 3 blessings) | `Docs/seances/oware-seance-2026-03-21.md` |
| OWARE architect review (blocking findings) | `Docs/architect-review-oware-2026-03-21.md` |
| OWARE post-completion checklist | `Docs/post-completion/oware-completion-2026-03-21.md` |
| OWARE engine source | `Source/Engines/Oware/OwareEngine.h` (911 lines) |
| Guru Bin: OWARE | `Docs/guru-bin-retreats/oware-retreat-2026-03-21.md` |
| Guru Bin: ORBWEAVE | `Docs/guru-bin-retreats/orbweave-retreat-2026-03-21.md` |
| Guru Bin: OVERTONE | `Docs/guru-bin-retreats/overtone-retreat-2026-03-21.md` |
| Guru Bin: ORGANISM | `Docs/guru-bin-retreats/organism-retreat-2026-03-21.md` |
| Preset census | `Docs/preset-census-2026-03-21.md` |
| Fleet seance scores | `Docs/fleet-seance-scores-2026-03-20.md` |
| Coupling performance spec | `Docs/specs/coupling_performance_spec.md` |
| DSP profiler (ORGANON/OSTINATO/OWARE) | `Docs/dsp-profiler-heavy-engines-2026-03-21.md` |
| Percussion trio comparator | `Docs/engine-comparator-percussion-trio-2026-03-21.md` |
| Field Guide editorial plan | `Docs/field-guide-editorial-plan-2026-03-21.md` |
| Shared DSP utilities | `Source/DSP/` (7 headers) |
| Previous session handoff | `Docs/session-handoff-2026-03-20.md` |

---

## 11. Immediate Next Actions (Ordered)

1. `git status` — confirm count of untracked + modified files matches this document
2. Commit untracked docs, retreats, seances, post-completion files (no DSP risk)
3. Commit untracked presets (~129 .xometa files)
4. Run build — verify migrations compile cleanly
5. If build passes: commit 22 engine migration files
6. If build passes: run auval
7. Fix OWARE LFOs (3 items: engine-level LFOs, shimmer rate wiring, buzz default)
8. Re-seance OWARE after fix to confirm 9.2 target
9. Begin OWARE preset expansion (target: 150 presets)
10. Write OXBOW Guru Bin retreat (no retreat exists yet)
