# Oxport R&D Session Handoff — March 16, 2026 (Part 3)

**Date**: 2026-03-16
**Author**: Ruby (Session Recorder)
**Session type**: Overnight R&D continuation (Part 3 of 3)
**Precursors**: See `overnight_session_2026_03_16.md` (Part 1) and `overnight_session_2026_03_16_part2.md` (Part 2)

This document covers the third and final leg of the overnight R&D session. Part 1 laid the foundation: original 8-tool suite, oxport.py orchestrator, and first engine spec passes. Part 2 pushed waves 8–21, the MPC eBook, XObserve/XOxide deep dives, and the bulk of the R&D spec library. Part 3 covers waves 22–current: tool suite expansion to 57 tools, the choke/mute group fix, pack tier naming, distribution strategy, and all deferred Opus work catalogued below.

---

## 1. Complete Tool Suite — 57 Tools

The full `xpn_*.py` inventory as of session end:

### Original Suite (8 tools — pre-session)

| Tool | Purpose |
|------|---------|
| `xpn_drum_export.py` | Renders ONSET engine presets to XPN drum programs; 5 kit modes, Vibe velocity curve |
| `xpn_keygroup_export.py` | Renders melodic engine presets to XPN keygroup programs; per-octave zone mapping |
| `xpn_kit_expander.py` | Expands a base kit by velocity/cycle/random/smart variation; round-robin support |
| `xpn_bundle_builder.py` | Assembles multiple XPN programs into a named expansion ZIP |
| `xpn_cover_art.py` | Generates XPN-compliant cover art PNG from engine color + name |
| `xpn_packager.py` | Final ZIP packaging with manifest.json + preview.mp3 slot |
| `xpn_sample_categorizer.py` | Auto-tags WAV files by spectral content before export |
| `xpn_render_spec.py` | Emits render specs (note, velocity, length) for external render pipelines |

### Wave 6 — Pipeline Infrastructure

| Tool | Purpose |
|------|---------|
| `xpn_validator.py` | XPN/XPM format linter; checks manifest, program XML, velocity zone math, ZIP layout; machine-readable exit codes (0/1/2) |
| `xpn_preview_generator.py` | Generates .mp3 preview audio (stitches pad one-shots into 30s showcase clip) matching Akai commercial expansion encoding spec |
| `xpn_qa_checker.py` | Perceptual WAV quality checker: clipping, silence, DC offset, peak normalisation, bit depth — integrates as oxport pre-export stage |

### Wave 7 — Intelligence Layer

| Tool | Purpose |
|------|---------|
| `xpn_optic_fingerprint.py` | Offline 8-band spectral analyser; produces JSON fingerprint per WAV (centroid, per-band energy, timbral balance) |
| `xpn_auto_dna.py` | Infers 6D Sonic DNA scores from WAV spectral fingerprint; outputs JSON DNA block for preset .xometa injection |
| `xpn_manifest_generator.py` | Generates XPN expansion manifest.json from a staged output directory; pulls engine color, version, sample count |
| `xpn_dedup_checker.py` | Detects duplicate or near-duplicate WAV files by waveform fingerprint before bundle packaging |
| `xpn_normalize.py` | Batch LUFS/peak normalisation for render outputs; configurable target level and true-peak ceiling |
| `xpn_smart_trim.py` | Silence-detection trim for rendered WAVs; preserves attack transients while removing dead tail |

### Wave 8 — Kit Architecture

| Tool | Purpose |
|------|---------|
| `xpn_choke_group_assigner.py` | Assigns choke/mute group IDs to drum program pads based on instrument type; auto-maps hi-hats to shared choke group |
| `xpn_auto_root_detect.py` | Detects the musical root note of a pitched sample via YIN/autocorrelation; injects RootNote into XPN zone XML |
| `xpn_articulation_builder.py` | Builds multi-articulation keygroup programs (legato/staccato/pizzicato layers) from a labelled sample directory |
| `xpn_classify_instrument.py` | ML-lite instrument family classifier (pitched/drum/noise/texture) from spectral features |
| `xpn_mpce_quad_builder.py` | Builds a 4-engine MPC-E native quad-program from four rendered engine outputs |

### Wave 9 — Quality & Scoring

| Tool | Purpose |
|------|---------|
| `xpn_pack_score.py` | Scores a completed pack on 5 axes (coverage, velocity depth, round-robin, DNA accuracy, QA pass rate); emits 0–100 score |
| `xpn_kit_completeness.py` | Checks a drum kit for missing canonical voices (kick/snare/hat/clap/perc/FX); flags gaps before packaging |
| `xpn_stems_checker.py` | Verifies stem sample sets have consistent sample rate, bit depth, and phase alignment |
| `xpn_preset_variation_generator.py` | Generates N preset variations from a seed preset by perturbing DNA dimensions within genre constraints |

### Wave 10 — Documentation & Delivery

| Tool | Purpose |
|------|---------|
| `xpn_liner_notes.py` | Generates markdown liner notes for a pack from manifest + engine identity cards + preset DNA data |
| `xpn_pack_readme_generator.py` | Generates the README.txt included in the expansion ZIP (installation, credits, legal) |
| `xpn_tutorial_builder.py` | Generates a structured tutorial document for a pack, covering pad layout, velocity behavior, and performance tips |
| `xpn_submission_packager.py` | Prepares a pack for third-party marketplace submission (Splice, Sounds.com); adds watermark, ISRC stub, submission manifest |
| `xpn_cover_art_batch.py` | Batch cover art generation for an entire fleet of engines; outputs one PNG per engine at correct XPN resolution |

### Wave 11 — Generative & Experimental Kits

| Tool | Purpose |
|------|---------|
| `xpn_ca_presets_kit.py` | Builds drum kits whose pad assignments follow a 1D cellular automaton rhythm pattern (Wolfram rules) |
| `xpn_lsystem_kit.py` | Builds melodic keygroup programs using L-system fractal note sequences for sample zone layout |
| `xpn_braille_rhythm_kit.py` | Maps Braille dot patterns to drum pad triggers; generative rhythmic structure from tactile language |
| `xpn_gravity_wave_kit.py` | Generates kit sample timing derived from gravitational wave chirp data (LIGO open dataset) |
| `xpn_seismograph_kit.py` | Maps seismograph amplitude data to velocity layers; geological event → dynamic kit |
| `xpn_gene_kit.py` | Derives drum kit structure from DNA base-pair sequences; ACGT → pitch/velocity/duration mapping |
| `xpn_pendulum_kit.py` | Builds polyrhythmic kits where pad triggers follow coupled pendulum physics simulation |
| `xpn_turbulence_kit.py` | Generates velocity layer contours from fluid turbulence simulation data |
| `xpn_climate_kit.py` | Maps climate/weather data (temperature, pressure, precipitation) to kit dynamics and articulation density |
| `xpn_transit_kit.py` | Generates kit structures from public transit schedule data (departure times → rhythm patterns) |
| `xpn_entropy_kit.py` | Builds kits where velocity and round-robin selection are governed by Shannon entropy of a seed signal |
| `xpn_attractor_kit.py` | Maps strange attractor trajectories (Lorenz, Rössler) to pad velocity curves and sample selection |

### Wave 12 — Coupling, Performance & Community

| Tool | Purpose |
|------|---------|
| `xpn_coupling_recipes.py` | Generates XPN coupling-aware keygroup programs where sample selection follows XOceanus coupling route logic |
| `xpn_coupling_docs_generator.py` | Emits per-engine coupling documentation (which targets are compatible, what modulation types are accepted) |
| `xpn_setlist_builder.py` | Assembles a live-performance setlist document from a fleet of packs, ordering by energy curve and key compatibility |
| `xpn_community_qa.py` | Community-facing QA checklist generator; produces a fillable checklist for beta testers and forum reviewers |
| `xpn_variation_generator.py` | High-level variation generator: wraps xpn_preset_variation_generator with fleet-level batch mode and DNA drift bounds |
| `xpn_adaptive_velocity.py` | Generates velocity curves adapted to instrument physics (linear/exponential/logarithmic) per voice type |
| `xpn_complement_renderer.py` | Renders harmonic complement samples for a given root note set; fills spectral gaps in melodic packs |
| `xpn_deconstruction_builder.py` | Builds deconstructed kit programs (individual component layers per hit) for sound design packs |
| `xpn_evolution_builder.py` | Generates a temporal evolution program: same kit across 4 versions showing sonic DNA drift over time |
| `xpn_monster_rancher.py` | Cross-breeds two engine preset DNAs to produce a hybrid preset; genetic recombination for sound design |
| `xpn_curiosity_engine.py` | Generates speculative "what if" preset variants by intentionally violating one DNA constraint per iteration |
| `xpn_poetry_kit.py` | Maps prosodic stress patterns from a poem (iambic, trochaic, etc.) to drum kit velocity accent maps |
| `xpn_git_kit.py` | Generates kit structure from git commit history (commit frequency → rhythm density, file count → velocity) |
| `xpn_tuning_systems.py` | Builds keygroup programs using non-12TET tuning systems (just intonation, meantone, gamelan pelog/slendro) |

---

## 2. R&D Specs Produced (Post-Wave 22 / Post-Part-2)

All specs below were created after `overnight_session_2026_03_16_part2.md` was written (after ~01:28).

### Pack Operations & Strategy

| Spec | Description |
|------|-------------|
| `xpn_pack_tier_system_rnd.md` | Pack tier naming research; landed on SIGNAL/FORM/DOCTRINE (not Basic/Pro/Premium) |
| `xpn_distribution_channels_rnd.md` | Year 1 distribution stack analysis: Gumroad + Patreon + MPC-Forums free seeding |
| `xpn_sample_normalization_rnd.md` | 44100Hz as XO_OX render standard; LUFS targets, true-peak ceilings, headroom doctrine |
| `velocity_science_rnd.md` | Velocity render midpoint documentation (not boundaries); musical velocity curve math |
| `xpn_render_quality_settings_rnd.md` | Render quality settings reference: bit depth, sample rate, dithering, fade envelope specs |
| `xpn_choke_mute_groups_rnd.md` | Choke/mute group architecture; bug finding: mg=0 was no-group, correct is mg=2 (snare) / mg=3 (clap) |
| `pack_roadmap_2026_rnd.md` | Fleet pack release roadmap through 2026; sequencing by engine readiness and market timing |
| `xpn_quality_scoring_rnd.md` | Quality scoring rubric for packs (5-axis, 0–100); minimum ship threshold documented |
| `pack_launch_playbook_rnd.md` | Step-by-step pack launch playbook: pre-launch, launch day, follow-through sequence |

### XPN Format & Technical

| Spec | Description |
|------|-------------|
| `xpn_round_robin_strategies_rnd.md` | Round-robin implementation strategies: CycleType/CycleGroup, true random, weighted random |
| `xpn_qlink_defaults_rnd.md` | Q-Link default assignment doctrine per instrument family; canonical per-voice physical defaults |
| `xpn_format_edge_cases_rnd.md` | Edge case catalogue for XPN format: empty layers, conflicting zone ranges, missing samples |
| `xpn_smart_trim_rnd.md` | Smart trim algorithm design: silence threshold, attack preservation, tail detection heuristics |
| `xpn_sample_library_vs_keygroup_rnd.md` | Decision matrix: when to ship as sample library vs. keygroup program vs. drum program |
| `xpn_preset_variation_rnd.md` | Preset variation generation strategy: DNA perturbation bounds, genre coherence constraints |

### Intelligence & Automation

| Spec | Description |
|------|-------------|
| `xpn_sonic_dna_automation_rnd.md` | Auto-DNA calibration design: 50-preset human-labeled dataset requirement, spectral feature mapping |
| `oxport_v2_architecture_rnd.md` | Oxport v2 package architecture proposal; OxportSession as shared state object across stages |

### Community & Market

| Spec | Description |
|------|-------------|
| `xpn_pack_feedback_loops_rnd.md` | Community feedback loop design: beta channels, forum seeding, revision cadence |
| `xpn_pack_naming_brand_rnd.md` | Pack naming conventions and brand voice guidelines for the XO_OX catalog |
| `xpn_social_proof_community_rnd.md` | Social proof strategy: MPC-Forums presence, Patreon exclusives, YouTube demo seeding |
| `xpn_catalog_longevity_rnd.md` | Catalog longevity doctrine: evergreen vs. topical packs, versioning, format migration path |
| `xpn_accessibility_rnd.md` | Accessibility considerations for XPN packs: documentation standards, labeling, tactile kit designs |
| `xpn_live_performance_rnd.md` | Live performance use cases for XPN packs; setlist design, pad layout conventions, energy mapping |
| `xpn_educational_pack_rnd.md` | Educational pack format: tutorial-first pack design, beginner/intermediate/advanced tiers |
| `mpc_os_35_format_innovations_rnd.md` | MPC OS 3.5 format capabilities research: new XPN fields, controller integration, plugin program slots |
| `juce_to_mpc_workflow_rnd.md` | End-to-end JUCE → MPC workflow documentation: render → export → test → ship |

---

## 3. Key Decisions Made This Session

### Bug Fix: Snare/Clap Mute Group (CRITICAL)
- **Problem**: `xpn_drum_export.py` was setting `mg=0` for snare and clap voices, which means "no mute group" in the XPN spec — snare and clap never choked each other.
- **Fix**: Corrected to `mg=2` for snare family, `mg=3` for clap family. Hi-hats retain their existing choke group logic.
- **Impact**: All drum programs exported before this fix should be re-exported for correct performance behavior.

### Tier Naming: SIGNAL / FORM / DOCTRINE
- Rejected: Basic/Pro/Premium (generic, does not carry XO_OX identity)
- Rejected: Sketch/Study/Score (too academic)
- **Adopted**: SIGNAL / FORM / DOCTRINE
  - SIGNAL: entry-level, single-engine, curated essentials
  - FORM: mid-tier, multi-program, full kit treatment
  - DOCTRINE: flagship, cross-engine coupling, Guru Bin–blessed, maximum preset depth

### Year 1 Distribution Stack
- **Gumroad**: Primary storefront for paid packs. Direct to consumer, low friction, good for single-pack sales.
- **Patreon**: Subscription tier gets early access + exclusive DOCTRINE packs. Builds recurring revenue base.
- **MPC-Forums free seeding**: 1–2 SIGNAL packs per engine released free on MPC-Forums to build community presence before monetization.
- **Not pursuing**: Splice (requires stems), Sounds.com (requires exclusivity), Akai Marketplace (approval pipeline too slow for V1 window).

### Oxport v2 — OxportSession Architecture
- Current oxport.py passes data between stages via filesystem (JSON files + WAV directories).
- Proposed v2: `OxportSession` dataclass holds all stage outputs in memory; stages receive and return the session object.
- Benefits: faster (no disk I/O between stages), testable (mock session injection), resumable (serialize session to disk on interrupt).
- **Status**: Architecture designed in `oxport_v2_architecture_rnd.md`. Implementation deferred to Sonnet-ready work (see below).

### Velocity Render Midpoints (Not Boundaries)
- Velocity layers are specified by their **midpoint**, not their boundary values.
- Example: 3-layer kit uses midpoints 40, 90, 115 — boundaries are computed automatically by the export tools.
- This matches how professional sample library designers think ("play at medium velocity") and produces musically coherent zone splits.
- Documented in `velocity_science_rnd.md`.

### 44100Hz as XO_OX Render Standard
- All XPN renders target 44100Hz / 24-bit.
- Rationale: MPC hardware is optimized for 44100Hz; 48kHz offers no perceptual benefit for sample playback; 24-bit gives adequate headroom over 16-bit without file size penalty.
- Exception: stems and mastering deliverables may use 48kHz / 32-bit float internally, but must be converted before XPN packaging.
- Documented in `xpn_sample_normalization_rnd.md`.

---

## 4. Opus-Deferred Work (Requires Opus Model to Build)

These items are explicitly deferred because they require novel DSP design, complex architecture decisions, or calibration work that Sonnet should not attempt without Opus oversight.

### P0 — CRITICAL

**Fleet Render Automation (`renderNoteToWav()` in `Source/Export/XPNExporter.h`)**
- The entire XPN export pipeline is currently a Python-side spec generator. The actual audio rendering (JUCE OfflineAudioContext → WAV file) does not exist yet.
- `renderNoteToWav(engineId, presetName, note, velocity, durationMs, outputPath)` is the missing primitive.
- Until this exists, all XPN tools produce render specs only — a human must render the audio manually.
- Full design is in `Docs/specs/fleet_render_automation_spec.md`.
- **Why Opus**: OfflineAudioContext threading in JUCE, worker thread lifecycle, denormal protection in offline render path, multi-engine parameter snapshot for render isolation.

### P1 — V1 Concept Engine DSP Builds

All four V1 concept engines have approved identity cards, design specs, and preset targets but zero source code:

| Engine | Spec Doc | DSP Challenge |
|--------|----------|--------------|
| OSTINATO | `xostinato-engine.md` (memory) | Communal drum circle — polyrhythmic synthesis, humanization algorithms |
| OPENSKY | `xopensky-engine.md` (memory) | Euphoric shimmer — spectral shimmer synthesis, feliX pole expression |
| OCEANDEEP | `xoceandeep-engine.md` (memory) | Abyssal bass — sub-bass physical modeling, pressure wave simulation |
| OUIE | `xouie-engine.md` (memory) | Duophonic hammerhead — 2-voice independent synthesis, STRIFE↔LOVE macro axis |

### P1 — Utility Engine DSP Builds

10 utility engines have concepts but no source code (from `utility_engine_concepts.md` and related docs). These are not registered in XOceanus yet. Exact list in `utility_engine_concepts.md` and `utility_engine_rapper_bundle.md`.

### P2 — Auto DNA Calibration

`xpn_auto_dna.py` uses heuristic spectral-to-DNA mapping that needs calibration against human-labeled data.
- **Requirement**: 50-preset human-labeled dataset (each preset scored on all 6 DNA dimensions by a sound designer).
- **Process**: Run `xpn_auto_dna.py` against the same 50 presets, compute correlation, tune multiplier constants.
- **Why Opus**: Requires interpreting perceptual audio features, defining calibration methodology, validating that spectral proxies correctly predict human-assigned DNA values.
- Full design in `xpn_sonic_dna_automation_rnd.md`.

### P2 — Oxport v2 Package Architecture

`OxportSession` dataclass design needs Opus to finalize the interface contract between stages before Sonnet implements it.
- Key open question: how does the session handle partial failure (one stage errors) — rollback, skip, or continue?
- Key open question: session serialization format for interrupt/resume (JSON vs. pickle vs. SQLite).
- Architecture research in `oxport_v2_architecture_rnd.md`.

---

## 5. Sonnet-Ready Next Steps

These items are well-scoped and can be implemented by Sonnet without novel design decisions.

### Wire --choke-preset flag into oxport.py
- `xpn_choke_group_assigner.py` exists and is tested.
- oxport.py `run` command does not yet have a `--choke-preset` flag to invoke it as a pipeline stage.
- Implementation: add flag to argparse, insert stage between `export` and `validate` in the stage graph.
- Reference: `xpn_choke_group_assigner.py` CLI signature.

### Wire xpn_auto_dna.py into oxport.py pre-flight
- `xpn_auto_dna.py` should run as the first stage of `oxport run`, injecting inferred DNA into the preset metadata before export stages consume it.
- Implementation: add `--auto-dna` flag (default off until calibrated), invoke as stage 0 in pipeline.

### Wire xpn_manifest_generator.py into oxport.py --stage-package
- `xpn_manifest_generator.py` is standalone; it should be called automatically by `oxport package` (or `oxport run --stage package`).
- Implementation: `package` subcommand calls manifest_generator, then cover_art_batch, then packager in sequence.

### Calibrate auto DNA tool against human-labeled presets
- Once the 50-preset human-labeled dataset exists (Opus work above), Sonnet can run the regression, compute scale factors, and update the constants in `xpn_auto_dna.py`.
- This is a data-pipeline task, not a design task — Sonnet-appropriate once the dataset is ready.

---

## 6. Session Health Summary

| Metric | Count |
|--------|-------|
| Total xpn_*.py tools | 57 |
| R&D specs created this session (Part 3) | ~20 |
| R&D specs total (all 3 parts) | 75+ |
| Bugs fixed | 1 critical (snare/clap mute group mg=0→2/3) |
| Key decisions finalized | 5 (tier naming, distribution, oxport v2 direction, velocity midpoints, sample rate standard) |
| Opus-deferred items | 6 major work blocks |
| Sonnet-ready items | 4 discrete tasks |

---

## 7. Handoff Checklist for Next Session

- [ ] Confirm snare/clap mute group fix is committed to `xpn_drum_export.py`
- [ ] Begin fleet render automation (`renderNoteToWav()`) — P0 CRITICAL, Opus required
- [ ] Resolve OxportSession serialization strategy (Opus) before Sonnet wires it
- [ ] Human-label 50 presets for DNA calibration dataset
- [ ] Wire --choke-preset into oxport.py (Sonnet)
- [ ] Decide DOCTRINE pack candidate: ONSET or OPAL for first flagship release?
- [ ] MPC-Forums free seed pack: which engine goes first?

---

*End of Part 3 handoff. Resume from fleet render automation.*
