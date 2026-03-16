# XPN Kit Design Patterns — R&D Spec

**Date**: 2026-03-16
**Scope**: Reusable architectural patterns for organizing XPN expansion pack programs on MPC. Each pattern is a repeatable layout + UX contract, not a one-off.

---

## Pattern 1: The Spectrum Kit

**Concept**: 16 pads in a single bank, ordered from driest/brightest (pad 1) to wettest/darkest (pad 16). Sweeping across pads = traversing a sonic continuum.

**User experience**: Playing pads left-to-right or bottom-to-top (MPC pad layout) moves from feliX territory into Oscar territory. The journey is the instrument. Works for drums (snappy→boomy), textures (airy→submerged), and melodic one-shots (bright→dark).

**XO_OX axis mapping**: feliX (bright, dry, high-clarity) occupies pads 1–4; feliX-leaning center at pads 5–8; Oscar-leaning center at pads 9–12; Oscar (wet, dark, sub-weight) at pads 13–16. Use engines on the feliX pole (OPENSKY, ONYX, OVERWORLD NES era) for the bright end; Oscar pole engines (OCEANDEEP, OHM, OVERDUB) for the dark end.

**Implementation**:
1. Design or select 16 samples covering the full feliX→Oscar range — vary pre-FX brightness, reverb tail length, and low-end weight in a monotonic progression.
2. Use `xpn_drum_export.py` or `xpn_keygroup_export.py` to build the program, assigning samples in index order (pad 1 = index 0).
3. Run `xpn_pad_layout_visualizer.py` to confirm the visual gradient reads correctly before export.
4. Set Q-Link 1 to filter cutoff — this reinforces the continuum when performing.

**Gotcha**: Non-monotonic loudness breaks the illusion. Normalize all samples with `xpn_normalize.py` first, then apply a subtle volume taper (−0.5 dB per step) so pads get slightly quieter as they get darker, matching listener expectation.

---

## Pattern 2: The Quad-Color Kit (MPCe Native)

**Concept**: Every pad exposes all four MPCe quad corners — NW/NE/SW/SE — mapped to feliX dry / feliX wet / Oscar dry / Oscar wet. 16 pads × 4 corners = 64 addressable sounds in one program.

**User experience**: Each pad is an instrument with a full feliX/Oscar spectrum accessible via pressure + corner. The MPC becomes a 2D performance surface.

**Corner mapping**:
- NW = feliX dry (bright, no reverb — OPENSKY, OVERWORLD NES)
- NE = feliX wet (bright + space — OPAL bright presets, ORPHICA)
- SW = Oscar dry (dark, tight — ONYX, OCEANDEEP sub hits)
- SE = Oscar wet (dark + depth — OVERDUB, OHM, OVERLAP)

**Implementation**:
1. For each of the 16 instruments, render 4 variants matching the corner map above.
2. Use `xpn_mpce_quad_builder.py` to assemble the program — it handles the quad-slot XML structure automatically.
3. Use `xpn_kit_validator.py` to confirm all 64 slots are populated and no corner is silent.

**Gotcha**: Tonal center must be consistent across all four corners of a pad. If NW is a C3 hit and SE is a D3 hit, the quad feels broken when corners are explored mid-performance. Tune all four variants to the same root before export (`xpn_auto_root_detect.py` can flag mismatches).

---

## Pattern 3: The Full-Circle Kit

**Concept**: 4 banks (A/B/C/D), each covering one production role: A=drums, B=bass, C=melodic one-shots, D=atmospheric/FX. One program = one complete production toolkit.

**User experience**: A producer never leaves the program. Bank A lays the foundation; bank B locks the low end; bank C carries melody and chords; bank D adds air, transition FX, and texture. The kit tells a complete sonic story.

**Bank assignments**:
- Bank A: 16 percussive sounds — kick, snare, hats, claps, toms, percs. Primary tools: ONSET, OBBLIGATO (attack transients).
- Bank B: 16 bass hits — sub hits, plucked bass, bass stabs. Primary tools: OVERBITE, OCEANDEEP, OVERDUB sends.
- Bank C: 16 melodic one-shots — keys, pads, hits, chimes. Primary tools: OPAL, ORPHICA, OTTONI, ODYSSEY.
- Bank D: 16 textures/FX — risers, downlifters, sweeps, ambience. Primary tools: OVERLAP, OUTWIT, OHM, OLE transitions.

**Implementation**:
1. Build each bank as a separate export pass using `xpn_drum_export.py` (Bank A) and `xpn_keygroup_export.py` (Banks B–D).
2. Merge into a 4-bank program using `xpn_kit_expander.py` with the `--banks A,B,C,D` flag.
3. Validate completeness: `xpn_kit_completeness.py` — target is 64 unique samples, no duplicates across banks.

**Gotcha**: Banks B–D need consistent tuning reference. Melodic content in C and D must be pitched to the same root key (default C) or kits become melodically unusable. Run `xpn_tuning_coverage_checker.py` before final export.

---

## Pattern 4: The Mood Gradient Kit

**Concept**: 4 banks ordered by emotional intensity — Foundation → Atmosphere → Prism → Aether — mirroring XO_OX's 7-mood taxonomy. Playing up through banks = emotional escalation.

**User experience**: Bank A grounds the track; Bank B opens space; Bank C introduces prismatic complexity; Bank D dissolves into the abstract. The bank structure is a compositional arc.

**Mood-to-bank mapping**:
- Bank A (Foundation): grounded, anchoring sounds — low transients, stable tones
- Bank B (Atmosphere): sustained, breath-like — reverb tails, filtered pads
- Bank C (Prism): harmonically rich, refracted — overtones, chord stabs, bell tones
- Bank D (Aether): sparse, dissolving — granular textures, long decays, near-silence moments

**Implementation**:
1. Use `xpn_mood_arc_designer.py` to scaffold the 4-bank mood progression and generate sample slot targets.
2. Curate samples using `xpn_preset_emotion_wheel.py` to score emotional placement and flag mis-sorted sounds.
3. Assemble via `xpn_kit_expander.py`.

**Gotcha**: Avoid mapping mood to loudness — a common mistake where Foundation = loud and Aether = quiet. Mood is about harmonic density and decay, not volume. Normalize all samples equally; let texture carry the arc.

---

## Pattern 5: The Conversation Kit

**Concept**: Pads are arranged in dialogue pairs — pad 1 and 2 are call/response, pad 3 and 4, and so on. Each pair draws from complementary engines per XO_OX coupling theory.

**User experience**: Playing pairs creates instant musical dialogue. The kit rewards intuitive two-finger play. Works for drum patterns (kick/response-snare), melodic lines (question phrase/answer phrase), and textural exchange.

**Coupling pair examples** (from XO_OX coupling doctrine):
- OVERWORLD ↔ OPAL (spectral call, granular response)
- ONSET ↔ OBBLIGATO (percussive hit, wind answer)
- OVERDUB ↔ ORPHICA (dub delay, microsound reply)
- OHM ↔ OLE (drone sustain, rhythmic punctuation)

**Implementation**:
1. Use `xpn_coupling_pair_recommender.py` to generate engine pairing suggestions based on feliX/Oscar polarity distance.
2. Use `xpn_coupling_recipes.py` to get preset pairs that are already tuned to work together.
3. Lay pairs into pad slots manually or via `xpn_performance_map_builder.py` with `--layout conversation`.

**Gotcha**: Pairs that are too similar (same engine, minor variation) produce a "stuttering" effect rather than dialogue. Enforce minimum polarity distance: feliX↔Oscar or at minimum two different engine families. `xpn_coupling_preset_auditor.py` can flag same-engine pairs.

---

## Pattern 6: The Era Kit

**Concept**: 4 banks = 4 musical eras. Bank A: acoustic/folk (pre-electric). Bank B: analog/electric (1960s–70s). Bank C: digital/early sampling (1980s–90s). Bank D: contemporary/hybrid (2000s–present). Maps directly to XOverworld's ERA triangle parameter.

**User experience**: Dialing through banks is a history lesson and a genre toolkit simultaneously. Each bank has internally consistent timbral character — no anachronistic samples.

**Engine-to-era mapping**:
- Bank A (Acoustic): OBBLIGATO (winds), OTTONI (brass), ONSET acoustic voices, OLE (folk percussion)
- Bank B (Analog): OVERWORLD analog/NES voices, OHM (analog drone), OVERDUB (tape), OVERBITE (analog bass)
- Bank C (Digital): ODYSSEY (digital synthesis), OUTWIT (Wolfram CA — algorithmic 90s), ONSET digital voices
- Bank D (Contemporary): OVERLAP (FDN reverb), OPAL (granular), ORPHICA (microsound), ORGANISM (generative)

**Implementation**:
1. In `xpn_drum_export.py` or `xpn_keygroup_export.py`, tag samples with era metadata using the `--era` flag so `xpn_pack_analytics.py` can audit distribution.
2. Use `xpn_kit_theme_analyzer.py` to verify each bank's timbral coherence scores within the era target.
3. Run `xpn_era_coverage_checker.py` (or `xpn_tuning_systems.py` for pitch era accuracy) before packaging.

**Gotcha**: Digital-era samples (Bank C) are often over-bright relative to analog samples (Bank B), creating a loudness cliff between banks. Apply era-appropriate EQ curves during render — Bank B gets gentle high roll-off above 12kHz; Bank C gets the full spectrum. Use `xpn_render_spec_generator.py` to bake era EQ into the render spec.

---

## Tool Reference Summary

| Pattern | Primary Tools |
|---|---|
| Spectrum Kit | `xpn_normalize.py`, `xpn_drum_export.py`, `xpn_pad_layout_visualizer.py` |
| Quad-Color Kit | `xpn_mpce_quad_builder.py`, `xpn_kit_validator.py`, `xpn_auto_root_detect.py` |
| Full-Circle Kit | `xpn_drum_export.py`, `xpn_keygroup_export.py`, `xpn_kit_expander.py`, `xpn_kit_completeness.py` |
| Mood Gradient Kit | `xpn_mood_arc_designer.py`, `xpn_preset_emotion_wheel.py`, `xpn_kit_expander.py` |
| Conversation Kit | `xpn_coupling_pair_recommender.py`, `xpn_coupling_recipes.py`, `xpn_performance_map_builder.py` |
| Era Kit | `xpn_kit_theme_analyzer.py`, `xpn_render_spec_generator.py`, `xpn_pack_analytics.py` |

---

*These patterns are composable. A Full-Circle Kit (Pattern 3) can have each bank ordered as a Spectrum (Pattern 1). A Conversation Kit (Pattern 5) can be tiered by Mood Gradient (Pattern 4). Use `xpn_kit_builder_assistant.py` to scaffold combined-pattern programs.*
