# FX Engine Multi-Session Execution Plan

**Date:** 2026-05-01
**Author:** Ringleader (RAC session)
**Source plan:** `Docs/specs/2026-04-27-fx-engine-build-plan.md`
**Status:** Active — sessions A, B, C ready to launch in parallel

---

## How to use this doc

Each numbered session below is a single Claude Code session. Sessions are grouped into **waves**:

- **Within a wave**, sessions are independent — start them in parallel from main.
- **Between waves**, sessions wait for the previous wave's merge.

Each session has:
- A **branch name** (deterministic, so the agent knows where to push)
- A **paste-ready prompt** (drop into a fresh session)
- An **estimated effort**
- An explicit **completion criterion** (so you know when to chain the next)

When a session completes, paste the prompt for the next session in the next wave (or another session in the same wave that hasn't been triggered yet).

---

## Wave 0 — Pack 1 Closure (in flight)

**Goal:** finish FX Pack 1 — Sidechain Creative — to "implemented" status.

| # | Session | Branch | Status | Effort |
|---|---|---|---|---|
| 0.1 | Otrium D004 fix + Copilot review | `claude/fx-engine-gap-analysis-2MXFG` | **PR #1474 open** | done |
| 0.2 | Otrium presets + re-seance | `claude/otrium-presets-and-reseance` | **ready to launch** | ~2 hr |
| 0.3 | Real Oligo DSP | `claude/oligo-real-dsp` | **ready to launch** (parallel with 0.2) | ~3-4 hr |
| 0.4 | Real Oblate DSP | `claude/oblate-real-dsp` | **ready to launch** (parallel with 0.2 + 0.3) | ~6-8 hr |

**Wave 0 parallelism:** 0.2 + 0.3 + 0.4 are independent — they touch different files. Start all three in parallel.

### Session 0.2 — Otrium presets + re-seance

```
Working branch: claude/otrium-presets-and-reseance (off main)
Parent reference: PR #1474 (must be merged before starting)

Goal: close out Otrium seance items 5 and 7.

Item 5 — author 5 demo presets per spec §8 of
Docs/specs/2026-04-27-fx-pack-1-sidechain-creative.md, in
Presets/XOceanus/Coupling/:
  - Triangular Throb       (Equilateral / Free, fast pump)
  - Three-Way Pad          (Equilateral / Sync, slow LFO)
  - Bass Triangle          (Isosceles / Free, narrow skew)
  - Chaos Topology         (Chaotic / Free)
  - Stereo Triangle        (Cyclical / Sync, slow drift)

Each preset must:
  - Demonstrate one topology + sync state combination so the audition
    is legible
  - Use 4 macros (CHARACTER, MOVEMENT, COUPLING, SPACE) producing
    audible change
  - Include 6D Sonic DNA
  - Use the .xometa format (see Skills/preset-architect/SKILL.md)

Use /preset-architect to author. Use /preset-auditor as the quality
gate before commit.

Item 7 — invoke /synth-seance on Otrium with the new presets in place.
Target verdict ≥ 8.0/10. If below, fix doctrine violations and re-run.
Persist verdict to Docs/seances/otrium_seance_<date>.md and update
seance_cross_reference.md row.

Push the branch. Open a PR with title "feat(otrium): 5 demo presets +
re-seance to <score>/10".

Completion = PR open + CI green + verdict ≥ 8.0.
```

### Session 0.3 — Real Oligo DSP

```
Working branch: claude/oligo-real-dsp (off main)
Parent reference: Source/DSP/Effects/OligoChain.h (current scaffold)

Goal: replace OligoChain scaffold with real frequency-selective
ducking DSP.

Spec: Docs/specs/2026-04-27-fx-pack-1-sidechain-creative.md §4
- Concept: 4-band Linkwitz-Riley split with per-band ducking
- Signal flow: 4-band LR split → per-band envelope followers from
  sidechain key → per-band VCAs (depth/atk/rel) → DNA-aware release
  scaler → band recombine
- 13 parameters declared (olig_lowDepth, ..., olig_keyEngine, etc.)
- Coupling: reads PartnerAudioBus for sidechain key (already plumbed
  for Otrium)

Implementation rules (CLAUDE.md):
  - All DSP inline in OligoChain.h
  - No audio-thread allocation
  - Denormal protection in feedback paths
  - ParamSnapshot pattern (cache pointers once per block)
  - Use PartnerAudioBus exactly like Otrium does

After DSP lands:
  1. Build (cmake --build build) — must compile clean
  2. /validate-engine — D001-D006 pass
  3. /synth-seance — capture verdict
  4. Author 3-5 demo presets (Pack 1 spec lists e.g. "Sidechain Pump",
     "Bass Carve", "Multiband Pump")

Push branch. Open PR "feat(oligo): real DSP — 4-band LR sidechain
ducker". Completion = PR open + CI green + seance verdict captured.
```

### Session 0.4 — Real Oblate DSP

```
Working branch: claude/oblate-real-dsp (off main)
Parent reference: Source/DSP/Effects/OblateChain.h (current scaffold)

Goal: replace OblateChain scaffold with real STFT spectral gate DSP.

Spec: Docs/specs/2026-04-27-fx-pack-1-sidechain-creative.md §3
- Concept: STFT gate where each FFT bin is gated by partner spectrum
- Signal flow: STFT analyzer → sidechain key extractor → per-band
  gate threshold computer (DNA-tilted) → anti-zip smoothing → ISTFT
- 11 parameters (obla_threshold, obla_fftSize, obla_dnaCoupling, etc.)
- Coupling: reads PartnerAudioBus for sidechain key (key engine)

Implementation rules (CLAUDE.md):
  - All DSP inline in OblateChain.h
  - JUCE FFT (juce::dsp::FFT) — fixed-size buffers allocated in
    prepare(), never on audio thread
  - Window function (Hann, square-root for COLA at 50% overlap)
  - 50% overlap-add (50% hop) standard
  - No audio-thread allocation, no blocking I/O
  - Denormal protection
  - obla_fftSize is enum (256/512/1024/2048) — pre-allocate at the
    largest size or rebuild on parameter change in prepare() (not
    audio thread)

After DSP lands:
  1. Build clean
  2. /validate-engine — D001-D006 pass
  3. /synth-seance
  4. Author 3-5 demo presets ("Spectral Gate", "Vocal Carve",
     "DNA-Tilted Gate")

Push branch. Open PR "feat(oblate): real DSP — STFT spectral gate".
Completion = PR open + CI green + seance verdict captured.

NOTE: STFT is the most complex of the three. If hopping into a fresh
session, consider reading Source/DSP/SpectralFreeze.h or any existing
STFT pattern in the codebase first to understand the project's FFT
conventions.
```

---

## Wave 1 — Phase 0 Wildcard Infrastructure

**Goal:** ship the three reusable primitives that Phase 2 packs depend on.

**Status:** `DNAModulationBus` already exists. `MoodModulationBus` and `AgeCouplingTarget` do not.

| # | Session | Branch | Effort | Depends on |
|---|---|---|---|---|
| 1.1 | MoodModulationBus | `claude/phase0-mood-mod-bus` | ~3-4 hr | Wave 0 merged |
| 1.2 | AgeCouplingTarget | `claude/phase0-age-coupling` | ~2-3 hr | Wave 0 merged |

Wave 1 sessions are independent — run in parallel.

### Session 1.1 — MoodModulationBus

```
Working branch: claude/phase0-mood-mod-bus (off main)

Goal: implement MoodModulationBus per
Docs/specs/2026-04-27-fx-engine-build-plan.md §3.1.

Mirror the structure of Source/Core/DNAModulationBus.h:
  - 16-mood one-hot + soft-blend continuous output
  - Default to current preset's mood tag
  - Opt-in target on every chain (default OFF per locked decision D3)
  - ≤0.01 Hz drift LFO mode (D005)

Files to create:
  - Source/Core/MoodModulationBus.h
  - Wire it from XOceanusProcessor.cpp like DNAModulationBus

No consumers yet — Pack 8 (Mastering) is the first consumer in
Phase 2. This session ships the bus + a smoke test only.

Build clean, /architect approval before merge.

Push branch. Open PR. Completion = PR open + CI green.
```

### Session 1.2 — AgeCouplingTarget

```
Working branch: claude/phase0-age-coupling (off main)

Goal: implement AgeCouplingTarget per
Docs/specs/2026-04-27-fx-engine-build-plan.md §3.1.

Locked decision D5: AGE = 0 default (factory-fresh). User explicitly
couples AGE to time / partner LFO / partner MOVEMENT macro.

Files to create:
  - Source/DSP/Effects/Aging.h — standard 0-1 AGE scalar convention
  - Wire as a coupling target in MegaCouplingMatrix (additive only —
    do not modify existing target IDs)

No consumers yet — Pack 2 (Analog Warmth) and Pack 6 (Lo-Fi Physical)
are first consumers. Ship the target type + matrix wiring only.

Build clean, /architect approval before merge.

Push branch. Open PR. Completion = PR open + CI green.
```

---

## Wave 2 — Phase 1 Wave 2 Validation (Big parallel pool)

**Goal:** flip 20 Wave 2 chains from `designed` → `implemented` in `engines.json`.

**Strategy:** start with `/master-audit` to surface bulk issues. Then per-chain
seances in parallel. Each chain seance can run in its own session.

| # | Session | Branch | Effort | Depends on |
|---|---|---|---|---|
| 2.0 | Master audit (sets baseline) | `claude/wave2-master-audit` | ~2 hr | Wave 0 merged |
| 2.1-2.20 | Per-chain seance + fixes (one session each) | `claude/wave2-seance-<chain>` | ~4-6 hr each | 2.0 complete |

The 20 chains: Oubliette, Osmium, Orogen, Oculus, Outage, Override, Occlusion,
Obdurate, Orison, Overshoot, Obverse, Oxymoron, Ornate, Oration, Offcut, Omen,
Opus, Outlaw, Outbreak, Orrery.

**Parallelism:** unlimited — every per-chain session is independent. You can
run 2-5 in parallel comfortably. Total elapsed time depends on parallel
session bandwidth.

### Session 2.0 — Master audit

```
Working branch: claude/wave2-master-audit (off main; doc-only)

Goal: run /master-audit across all 20 Wave 2 chains and produce a
prioritized seance queue.

Output: Docs/fleet-audit/wave2-master-audit-<date>.md ranking the 20
chains by:
  1. Severity of doctrine violations (D004 dead params first, then
     D002 weak modulation, then D005 floor)
  2. Implementation completeness (does the DSP exist, or is it stub?)
  3. Spec drift (does code match spec?)

This becomes the queue order for Wave 2 sessions 2.1-2.20.

Push branch with the audit doc. PR is doc-only.
```

### Session 2.x — Per-chain seance template

```
Working branch: claude/wave2-seance-<chain> (off main; chain in lower-case)
Replace <CHAIN> below.

Goal: validate <CHAIN>Chain (Source/DSP/Effects/<CHAIN>Chain.h) and
flip its engines.json status from "designed" to "implemented".

Workflow:
  1. /validate-engine on <CHAIN>
  2. /synth-seance on <CHAIN>
  3. For every doctrine violation surfaced, fix it inline in the chain
     header (no audio-thread alloc, additive params only — existing
     IDs are FROZEN)
  4. Re-run /synth-seance until verdict ≥ 8.0
  5. Update Docs/engines.json status to "implemented"
  6. Run python Tools/sync_engine_sources.py
  7. Update Docs/seances/seance_cross_reference.md row
  8. Update Docs/reference/engine-color-table.md if score changes
     blessing/debate state

Push branch. Open PR "feat(<chain>): seance to <score>/10 + flip
implemented". Completion = PR open + CI green + verdict ≥ 8.0.
```

---

## Wave 3 — Phase 2 New Packs (8 packs × 3 chains)

**Goal:** ship Packs 2-9 from `Docs/specs/2026-04-27-fx-engine-build-plan.md` §5.

**Per-pack effort:** ~3 chains × 4 hr DSP + 4 hr seance + 4 hr presets + 4 hr
retrofits = ~30-40 hr per pack. Many sessions per pack.

**Strategy:** one pack at a time across waves; within a pack, the 3 chains can
run in parallel.

### Pack 2 — Analog Warmth (Wave 3.A)
- Session 3.A.1: TapeEcho (`claude/pack2-tape-echo`)
- Session 3.A.2: SpringReverb (`claude/pack2-spring-reverb`)
- Session 3.A.3: TubeDrive (`claude/pack2-tube-drive`)
- Session 3.A.4: Outage + Offcut AGE retrofit (`claude/pack2-retrofits`)
- **Depends on:** Wave 1.2 (AgeCouplingTarget) merged

### Pack 3 — Vintage Modulation
- 3.B.1: Flanger · 3.B.2: Univibe · 3.B.3: Rotary
- 3.B.4: Oration / Oxymoron / Outlaw retrofit
- **Depends on:** Wave 0 + Wave 1 merged

### Pack 4 — Reverb Classics
- 3.C.1: Plate · 3.C.2: Hall · 3.C.3: IRConvolver
- 3.C.4: Orison / Orogen / Occlusion / Orrery retrofit
- **Depends on:** Wave 0 + Wave 1 merged

### Pack 5 — Multiband Creative
- 3.D.1: MBSaturator · 3.D.2: MBReverb · 3.D.3: MBDelay
- 3.D.4: Obdurate / Ornate retrofit
- **Depends on:** Wave 0 + Wave 1 (DNA bus already exists) merged

### Pack 6 — Lo-Fi Physical
- 3.E.1: Vinyl · 3.E.2: Cassette · 3.E.3: Shortwave
- 3.E.4: Outage / Offcut second-pass retrofit
- **Depends on:** Wave 1.2 (AgeCouplingTarget) merged

### Pack 7 — Pitch / Time
- 3.F.1: PitchProcessor · 3.F.2: TimeStretcher · 3.F.3: FormantShifter
- 3.F.4: Oubliette / Osmium retrofit
- **Depends on:** Wave 0 + Wave 1 merged

### Pack 8 — Mastering Pipeline
- 3.G.1: TransparentComp · 3.G.2: LinearPhaseEQ · 3.G.3: LoudnessMaximizer
- 3.G.4: Opus / Omen retrofit
- **Depends on:** Wave 1.1 (MoodModulationBus) merged

### Pack 9 — Aquatic Spatial
- 3.H.1: BinauralField · 3.H.2: BathyscapeSpatializer · 3.H.3: DepthDoppler
- 3.H.4: Oculus retrofit
- **Depends on:** Wave 0 + Wave 1 merged · HRTF research session (3.H.0) ahead

### Per-pack session prompt template

```
Working branch: claude/<pack-id>-<chain-name> (off main)
Spec: Docs/specs/2026-04-27-fx-engine-build-plan.md §5 Pack <N>

Goal: implement <CHAIN_NAME>Chain end-to-end (DSP + presets + seance).

Steps:
  1. /new-xo-engine to scaffold the chain header (or directly write
     the header following Source/DSP/Effects/OtriumChain.h pattern)
  2. Implement DSP per spec §<pack-section>
  3. Wire into EpicChainSlotController.h (chain enum + factory)
  4. Add to Source/Core/PresetManager.h validEngineNames + frozenPrefix
  5. Update Docs/engines.json (status: implemented)
  6. python Tools/sync_engine_sources.py
  7. /validate-engine + /synth-seance until verdict ≥ 8.0
  8. Author 5 demo presets in Presets/XOceanus/<mood>/ via
     /preset-architect + /preset-auditor
  9. Update CLAUDE.md (engine count, parameter prefix table row)
 10. Update Docs/reference/engine-color-table.md (color row + blessing
     if applicable)

Push branch. Open PR. Completion = PR open + CI green + verdict ≥ 8.0
+ 5 presets exist.
```

---

## Wave 4 — Phase 3 Meta-Wildcards

**Goal:** ship Knot-Topology slot routing + Mood-as-modulator fleet promotion.

| # | Session | Branch | Effort | Depends on |
|---|---|---|---|---|
| 4.1 | Knot-Topology slot routing | `claude/phase3-knot-routing` | ~2 weeks | Wave 3 merged |
| 4.2 | Mood-as-mod fleet promotion | `claude/phase3-mood-fleet` | ~1 week | Wave 1.1 + Wave 3 merged |

These are major architectural lifts. Recommend dedicated multi-session campaigns
each, not single sessions. Will be re-planned when Wave 3 closes.

---

## Parallelism summary

At any given moment, you can have running:

- **Now (Wave 0):** 3 parallel sessions (0.2 Otrium presets, 0.3 Oligo DSP, 0.4 Oblate DSP)
- **After Wave 0 merges:** up to 22 parallel sessions (Wave 1.1 + 1.2 + Wave 2.1-2.20)
- **After Wave 2 merges:** up to 8 packs × 4 sessions/pack = ~30 parallel sessions through Wave 3

The throughput limit becomes how many sessions you can monitor and merge, not
how much work is available to dispatch.

## Suggested triggering cadence

If you can run **2 parallel sessions** at a time:

1. Trigger 0.2 + 0.3. Let them complete.
2. Merge both PRs. Trigger 0.4 + 1.1.
3. Merge. Trigger 1.2 + 2.0.
4. Merge. Trigger 2.1 + 2.2 (top of master-audit queue).
5. Continue down the master-audit queue, two at a time, until 2.x exhausted.
6. Move to Wave 3 (Pack 2 first — Analog Warmth — since it has the highest
   market legibility).

If you can run **5+ parallel sessions** at a time, run the entire Wave 0 in
parallel, then the entire Wave 1 + 2.0 in parallel, then 5 Wave 2 sessions
at a time.

---

## Branch hygiene rules

- Every session works on its own branch off main
- Branches are named `claude/<descriptive-slug>` for one-shot work or
  `claude/<scope>-<chain>` for chain-specific work
- Never push directly to main
- Every session opens a PR (not a draft)
- Sessions never touch each other's branches
- Merge order: PRs land in dependency order (Wave 0 before Wave 1 before
  Wave 2 etc.); within a wave, any order is fine

---

## Status tracking

This doc is the source of truth. As sessions complete:
- Tick the session in the wave table
- Note the merged PR number
- Move blocked-on dependencies forward as they unblock

A future session can read this doc and immediately know what's next to dispatch.
