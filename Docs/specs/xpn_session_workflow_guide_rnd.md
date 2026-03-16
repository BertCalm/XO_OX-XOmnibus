<!-- rnd: session-workflow -->

# XO_OX Producer Session Workflow Guide

*From preset discovery to MPC performance to recorded stems — a practical playbook.*

---

## 1. Session Philosophy

XO_OX is built around one conviction: **character over features**. Every preset in the 2,550-preset fleet carries a Sonic DNA fingerprint — a set of traits (Mood, Energy, Texture, Role) that tells you *what it does in a mix* before you audition it.

The **feliX-Oscar axis** is the compass for every session decision:
- **feliX** (the X pole): melodic, harmonic, shimmering, generative — engines like OPAL, ORPHICA, OPENSKY
- **Oscar** (the O pole): percussive, dense, abyssal, rhythmic — engines like ONSET, OUTWIT, OCEANDEEP
- Most sessions want **tension between poles**, not dominance by one

**Coupling as composition**: XO_OX engines are designed to feed each other. OPAL → DUB, OVERWORLD → OPAL, DRIFT → OPAL are documented chains. When you pick your anchor preset, think immediately about what it couples *into* — that second voice is often where the session lives.

---

## 2. Discovery Flow

**2,550 presets across 31 engines** is navigable if you enter through the right door.

### By Mood Category
Seven moods structure the fleet: Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family. Start here when you know the session's emotional register.

```bash
# Browse presets filtered by mood
python3 Tools/xpn_preset_browser.py --mood Atmosphere --engine OPAL
```

### By Sonic DNA
Each preset carries Energy (1–5), Texture (Smooth/Gritty/Airy/Dense), and Role (Lead/Pad/Bass/Perc/FX). Use DNA browsing when you know the *function* before the sound.

```bash
python3 Tools/xpn_preset_search_engine.py --role Pad --energy 3 --texture Airy
python3 Tools/audit_sonic_dna.py --report-gaps   # find engines with thin coverage
```

### By Coupling Chain
If your session anchor is already chosen, let the coupling recommender surface compatible partners:

```bash
python3 Tools/xpn_coupling_pair_recommender.py --anchor OPAL --count 5
python3 Tools/xpn_coupling_recipes.py --source OVERWORLD  # shows downstream targets
```

Use `xpn_preset_emotion_wheel.py` for free-association browsing when you don't have a clear starting point.

---

## 3. XPN Pack Assembly

A focused session pack is typically **16–64 presets** organized around one narrative (mood arc, coupling chain, or instrument family).

### Step 1 — Curate with the Kit Builder Assistant
```bash
python3 Tools/xpn_kit_builder_assistant.py --theme "deep-water-bass" --count 32
```

### Step 2 — Validate DNA coverage
```bash
python3 Tools/xpn_kit_completeness.py --pack my_session_pack/
python3 Tools/xpn_dna_gap_finder.py --pack my_session_pack/
```

### Step 3 — Assign choke groups and Q-Links
```bash
python3 Tools/xpn_choke_group_designer.py --pack my_session_pack/
python3 Tools/xpn_macro_assignment_suggester.py --pack my_session_pack/
```

### Step 4 — Export to XPN
```bash
python3 Tools/xpn_batch_export.py --pack my_session_pack/ --output exports/
python3 Tools/xpn_packager.py --input exports/ --name "SESSION_2026_03" --version 1.0.0
```

### Step 5 — Final QA
```bash
python3 Tools/xpn_validator.py --xpn exports/SESSION_2026_03.xpn
python3 Tools/xpn_pack_integrity_check.py --xpn exports/SESSION_2026_03.xpn
```

---

## 4. MPC Live III Setup

### Loading the Expansion
1. Copy the `.xpn` file to MPC's `Expansions/` folder (USB or WiFi transfer)
2. On device: **MENU → Expansions → Browse** — locate your pack
3. Assign to a project: **Project → Expansion Slots → Slot 1**

### Pad Layout Conventions
XO_OX follows an **8×2 upper/lower split** on the 4×4 grid:
- **Row 1 (pads 13–16)**: feliX leads and arps — melodic, high-register
- **Row 2 (pads 9–12)**: atmospheric pads and textures
- **Row 3 (pads 5–8)**: Oscar rhythmic and percussive voices
- **Row 4 (pads 1–4)**: bass and sub foundations

### Q-Link Defaults
Standard Q-Link assignment (see `xpn_qlink_defaults_rnd.md` for full table):
- **Q1**: Filter cutoff (character sweep)
- **Q2**: Macro 1 — engine's primary identity knob (DRAMA, PUNCH, BOND, etc.)
- **Q3**: Reverb send
- **Q4**: Master volume / expression

```bash
python3 Tools/xpn_macro_performance_recorder.py --session my_session --record
```

### MPCe 3D Pad — The XO_OX Quad-Corner Strategy
The pressure and positional axes of MPCe pads map directly to the feliX-Oscar axis:

| Corner | Axis | Character |
|--------|------|-----------|
| **NW** | feliX / dry | Clean melodic voice, no effects |
| **NE** | feliX / wet | Melodic + full send chain |
| **SW** | Oscar / dry | Raw percussive hit |
| **SE** | Oscar / wet | Processed, effected percussion |

Build pads so velocity + corner position naturally moves you along both axes simultaneously. Use `xpn_mpce_quad_builder.py` to generate pad assignments that honor this geometry.

```bash
python3 Tools/xpn_mpce_quad_builder.py --engine OPAL --partner ONSET --output session_quad.xpm
```

---

## 5. Performance Techniques

### Live Coupling Manipulation
Coupling parameters respond well to Q-Link sweeps mid-performance. Identify coupling send levels in your engine pairs and assign them to a dedicated Q-Link bank before the session.

```bash
python3 Tools/xpn_macro_curve_designer.py --param coupling_send --shape exponential
```

### Macro Performance Scripts
Pre-record macro movement arcs for hands-free automation:
```bash
python3 Tools/xpn_macro_performance_recorder.py --session arc_01 --playback looped
```

### Choke Group Strategy
- Group all Oscar percussive voices into choke groups by type (kick family, hat family)
- Keep feliX voices unchoked — allow harmonic stacking
- Use `xpn_choke_group_assigner.py` to batch-apply groups across the pack

```bash
python3 Tools/xpn_choke_group_assigner.py --pack session_pack/ --mode smart
```

### Setlist Building
For longer sessions with mood arc movement, pre-plan program changes:
```bash
python3 Tools/xpn_setlist_builder.py --arc "Foundation → Flux → Aether" --programs 8
```

---

## 6. Recording Pipeline

### Stem Export from MPC
1. On MPC: **MENU → Export → Stems** — route each program to its own track
2. Export at 24-bit / 48kHz (match XO_OX's native sample rate)
3. Label stems by engine name for DAW re-import clarity

```bash
# Verify stems after export
python3 Tools/xpn_stems_checker.py --stems-dir ./session_stems/
python3 Tools/xpn_sample_rate_auditor.py --dir ./session_stems/
```

### Re-import to DAW
- Import stems as audio clips — no pitch or time correction at this stage
- Keep XOmnibus plugin loaded on a dedicated insert channel for live processing
- Route the XOmnibus send through the same coupling chain used during performance

### XOmnibus Plugin Processing
Use XOmnibus in the DAW to apply additional engine processing to recorded stems:
- Load matching presets so processing character is continuous from performance to mix
- Coupling chains (e.g., OPAL → DUB) can be reproduced in-plugin for deeper saturation on stems

---

## 7. Pack Maintenance

### Version Bumping
```bash
python3 Tools/xpn_pack_version_bumper.py --pack SESSION_2026_03.xpn --bump minor
```

### Changelog Generation
```bash
python3 Tools/xpn_changelog_generator.py --pack SESSION_2026_03.xpn --since v1.0.0
python3 Tools/xpn_pack_changelog_tracker.py --log changes.md
```

### Adding Presets to Existing Packs
1. Run `xpn_preset_duplicate_detector.py` before adding — prevent DNA redundancy
2. Add new programs to the pack's source directory
3. Re-run `xpn_kit_completeness.py` to confirm improved coverage
4. Bump patch version, regenerate manifest, repackage

```bash
python3 Tools/xpn_preset_duplicate_detector.py --pack SESSION_2026_03/ --new new_presets/
python3 Tools/xpn_manifest_generator.py --pack SESSION_2026_03/
python3 Tools/xpn_packager.py --input SESSION_2026_03/ --name "SESSION_2026_03" --version 1.0.1
```

---

## 8. Quick Reference — 12 Most-Used Tools

| Tool | What It Does |
|------|-------------|
| `xpn_preset_browser.py` | Browse presets by mood, engine, or energy level |
| `xpn_preset_search_engine.py` | DNA-filtered search (role, texture, energy) |
| `xpn_coupling_pair_recommender.py` | Suggest coupling partners for a chosen anchor engine |
| `xpn_kit_builder_assistant.py` | Build a themed session pack with curated preset selection |
| `xpn_choke_group_designer.py` | Design choke groups interactively before export |
| `xpn_macro_assignment_suggester.py` | Suggest Q-Link / macro assignments per engine |
| `xpn_mpce_quad_builder.py` | Generate feliX/Oscar quad-corner pad assignments for MPCe |
| `xpn_batch_export.py` | Export multiple presets to XPN format in one pass |
| `xpn_packager.py` | Bundle programs + samples + metadata into a distributable `.xpn` |
| `xpn_validator.py` | Validate XPN file structure and metadata completeness |
| `xpn_pack_version_bumper.py` | Bump semantic version on an existing pack |
| `xpn_stems_checker.py` | Verify exported MPC stems (sample rate, bit depth, naming) |

---

*For coupling architecture details see `Docs/specs/coupling_dna_pack_design_rnd.md`. For MPCe-native design patterns see `Docs/specs/xpn_mpce_native_design_guide_rnd.md`. For Q-Link default table see `Docs/specs/xpn_qlink_defaults_rnd.md`.*
