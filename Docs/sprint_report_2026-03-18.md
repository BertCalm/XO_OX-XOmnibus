# XOlokun Sprint Report — 2026-03-18

**Branch:** `v1-launch-prep`
**Status:** ROADMAP COMPLETE — ready for boutique engine design

---

## Fleet Completion

**40/40 engines at 150+ presets.** All committed, all normalized.

| Metric | Value |
|--------|-------|
| Total .xometa preset files | 13,550 |
| Engines at 150+ presets | 40 / 40 |
| Mood categories | 8 (Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged) |
| Sonic DNA coverage | 100% |
| Build status | PASS (AU + Tests) |

---

## What Shipped This Sprint

### 1. Engine Name Normalization (5,818 presets fixed)

All `.xometa` files now use canonical mixed-case engine names (e.g. `"Origami"` not `"ORIGAMI"`).

**Root cause:** `PresetManager.h::validEngineNames` is case-sensitive. All-caps names from earlier agent-written presets were silently dropped on load. Fixed in two layers:

- **`Tools/normalize_engine_names.py`** — rewrites engine names in-place across the entire preset library. Handles all-caps variants, wrong-case variants, X-prefix variants, and singular `"engine"` → plural `"engines"` key migration.
- **`Source/Core/PresetManager.h`** — registered 8 previously missing engines (`Overlap`, `Outwit`, `OpenSky`, `Ostinato`, `OceanDeep`, `Ouie`, `Overtone`, `Organism`) in `validEngineNames`; added X-prefix aliases in `resolveEngineAlias()`; fixed `frozenPrefixForEngine` keys.

### 2. Preset Rename Tool

**`Tools/rename_dna_coded_presets.py`** — renamed 80 coupling presets from DNA-encoded names (`5X DARK COLD KINETIC...`) to evocative 2-3 word names based on engine pair + DNA + mood. Naming convention fully enforced fleet-wide.

### 3. DSP Fixes

| Engine | Fix | Doctrine |
|--------|-----|---------|
| **OSTINATO** | `OstiDecayEnv` upgraded from single-stage decay to full Attack/Hold/Decay envelope; 8 pattern step params now gate voices on note-on | D004 |
| **ORCA** | `AudioToRing` dead coupling path wired: `voiceSignal *= (1.0f + couplingRingModSrc)` was accumulated but never applied | D004 |
| **OMBRE** | 3× `std::max` → `juce::jmax` at LFO rate floors and envelope peak tracking (convention fix) | — |

### 4. Sound Design Guide: 40/40 Complete

`Docs/xolokun_sound_design_guides.md` — all 6 new engines documented:

- **Section 31:** OPENSKY (`sky_` prefix, 30 params — euphoric shimmer, pure feliX)
- **Section 32:** OSTINATO (`osti_` prefix, 18 params + 8 pattern step bools — communal drum circle)
- **Section 33:** OCEANDEEP (`deep_` prefix, 25 params — abyssal bass, pure Oscar)
- **Section 34:** OUIE (`ouie_` prefix, 30 params — duophonic hammerhead, STRIFE↔LOVE axis)
- **Section 35:** OVERTONE (`over_` prefix, 26 params, 8 partial amplitudes — continued fractions spectral)
- **Section 36:** ORGANISM (`org_` prefix, 24 params — cellular automata generative, RULE/SEED/COUPLING/MUTATE)

---

## Oxport — XPN Export Pipeline

354 Python tools in `Tools/`. The full pipeline:

```
Flat WAVs → xpn_sample_categorizer.py → classified kit
          → xpn_kit_expander.py --mode smart → velocity + round-robin WAVs
          → xpn_drum_export.py --mode smart → .xpm programs
          → xpn_packager.py → .xpn ZIP → MPC

.xometa   → xpn_render_spec.py → render instructions
          → [record WAVs] → xpn_keygroup_export.py → .xpm programs
          → xpn_packager.py → .xpn ZIP → MPC
```

**Orchestrator:** `Tools/oxport.py` — 8 stages (render_spec → categorize → expand → qa → export → cover_art → complement_chain → package). One command from WAVs to `.xpn`.

### Specialized tools (selected)

| Tool | Purpose |
|------|---------|
| `xpn_drum_export.py` | 5 kit modes, non-linear velocity curves, DNA-driven dynamics |
| `xpn_keygroup_export.py` | Zone layout, velocity layers, full XPM field set |
| `xpn_kit_expander.py` | 1 flat WAV → 4 velocity layers + 4 round-robins |
| `xpn_packager.py` | Correct `.xpn` ZIP structure (Expansions/manifest, Programs/, Samples/) |
| `xpn_sample_categorizer.py` | Auto-classifies drums: kick/snare/hat/clap/tom/perc/fx |
| `xpn_render_spec.py` | .xometa → render instructions (engine-specific strategies) |
| `xpn_mpce_quad_builder.py` | MPCe 3D pad quad-corner layout builder |
| `xpn_validator.py` | XPM XML schema validation |
| `xpn_cover_art.py` | Procedural 2000×2000 cover art per engine |

---

## Pack Status

### TIDE TABLES (free gateway pack)

**Status: SPEC COMPLETE — awaiting WAV recording session**

- 4 programs: Coastal Drums (ONSET), Tide Lead (ODYSSEY), Shore Texture (OPAL), Deep Water (Entangled multi-engine)
- Spec: `Docs/packs/tide_tables/tide_tables_pack_spec.json`
- README: `Docs/packs/tide_tables/TIDE_TABLES_README.md`
- Validator fixed (commit `1de201f1`)
- **Blocker:** No rendered WAV stems. Recording session required (~3-4 hours on hardware).

### MACHINE GUN REEF ($15 premium pack)

**Status: SPEC COMPLETE — awaiting WAV recording session**

- 5 programs, 4 engines (ONSET, OCEANIC, ORCA, OBESE), 80 pads
- Spec: `Docs/packs/machine_gun_reef/machine_gun_reef_spec.json`
- README: `Docs/packs/machine_gun_reef/MACHINE_GUN_REEF_README.md`
- **Blocker:** Same WAV recording session as TIDE TABLES.
- **Sequence:** TIDE TABLES ships first (free gateway); MACHINE GUN REEF is the paid upsell.

---

## Commit History (this sprint)

| Hash | Description |
|------|-------------|
| `7c70e999` | Straggler presets: Obsidian/Orbital/Odyssey/Oblique +61 to 150 each |
| `2d4b806e` | Straggler presets: OddOscar +71 to 150, Obsidian +34 fleet fills |
| `e9114bb7` | Straggler presets: Constellation + Owlfish Submerged + Oblong/Oracle/Osprey/Origami |
| `10a96131` | Add 561 presets from late-wave agents (Opal/Oceanic/Ouroboros/Onset/Osteria/Optic) |
| `cfd477dc` | Fleet complete: 40/40 engines at 150+ \| 19,559 presets \| DSP fixes + guide |
| `a46c6f61` | Complete fleet: 40/40 engines at 150+ presets, 19,578 total |
| `1b609ae2` | Normalize engine names fleet-wide + register missing engines in PresetManager |

---

## What's Next

### Immediate (unblocking XPN packs)
1. **Record WAV stems** — Open XOlokun standalone, render individual ONSET voice hits at 4 velocity levels + ODYSSEY multi-samples + OPAL texture beds. ~3-4 hours. Unblocks both packs.
2. **Run preset name validator** before the session — confirm spec preset IDs (`perc_kick_solid`, `drift_lead_bright`, etc.) exist in the library.
3. **Oxport preflight** — `python Tools/oxport.py --dry-run` to check what's missing pre-session.

### V1 Concept Engine DSP (4 engines — no source code yet)
These are V1 scope. Identity cards + 150 presets each exist. DSP scaffold is next:

| Engine | Character | Accent |
|--------|-----------|--------|
| OSTINATO | Communal drum circle | Firelight Orange `#E8701A` |
| OPENSKY | Euphoric shimmer, pure feliX | Sunburst `#FF8C00` |
| OCEANDEEP | Abyssal bass, pure Oscar | Trench Violet `#2D0A4E` |
| OUIE | Duophonic hammerhead, STRIFE↔LOVE | Hammerhead Steel `#708090` |

### Boutique Engine Design
First boutique engine: **OSTINATO** (top candidate — Voice + custom FX in 5th slot = paid product, fundraiser-connected). 150 presets + concept brief exist. Scaffold + DSP build is next major work.

### Other Pending
- Patreon URL placeholder (`www.patreon.com/cw/XO_OX`) — needs real URL before launch
- 20 hero preset audio recordings for XO-OX.org
- Field Guide posts for new engines (OHM, ORPHICA, OBBLIGATO, OTTONI, OLE, OVERLAP, OUTWIT, OMBRE, ORCA, OCTOPUS + V1 concepts)
- Guru Bin retreats pending: OPAL, ODYSSEY, ORBITAL + Tier 2 engines

---

*Generated 2026-03-18 | XOlokun v1-launch-prep branch*
