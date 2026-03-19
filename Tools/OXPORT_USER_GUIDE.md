# Oxport User Guide

**Oxport** is the XO_OX expansion pack pipeline — it chains all 9 XPN export stages into a single CLI.

## Quick Start

```bash
cd Tools/

# Full pipeline for an engine
python3 oxport.py run --engine Onset --wavs-dir /path/to/wavs --output-dir ./dist/onset/

# Dry run — see what would happen without executing
python3 oxport.py run --engine Onset --wavs-dir /path/to/wavs --output-dir ./dist/ --dry-run

# Validate finished .xpn archive (Rex's XPN Bible compliance)
python3 oxport.py validate --xpn dist/Onset_v1.0.xpn

# Validate .xometa presets (10-point quality check)
python3 oxport.py validate --presets

# Batch export multiple engines
python3 oxport.py batch --config batch.json --parallel 4
```

---

## Pipeline Stages

| Stage | Tool | What it does |
|-------|------|-------------|
| 1 `render_spec` | `xpn_render_spec.py` | Generates WAV render specs from .xometa presets |
| 2 `categorize` | `xpn_sample_categorizer.py` | Classifies WAV files into voice categories |
| 3 `expand` | `xpn_kit_expander.py` | Expands flat kits to velocity/cycle/smart layers |
| 4 `qa` | `xpn_qa_checker.py` | Perceptual QA on rendered WAVs |
| 5 `smart_trim` | `xpn_smart_trim.py` | Auto-trims silence tails + fade-out |
| 6 `export` | `xpn_drum_export.py` / `xpn_keygroup_export.py` | Generates .xpm programs |
| 7 `cover_art` | `xpn_cover_art.py` | Procedural branded cover art |
| 8 `complement_chain` | (Artwork collection only) | Primary + complement XPM pairs |
| 9 `package` | `xpn_packager.py` | Final .xpn ZIP archive |

---

## Commands

### `oxport.py run`
Full pipeline execution.

```
--engine NAME           Engine name (Onset, Odyssey, Opal, …)
--wavs-dir PATH         Directory containing rendered WAV samples
--output-dir PATH       Where to write .xpm, artwork, and .xpn
--preset NAME           Filter to a single preset (partial match)
--skip STAGES           Comma-separated stages to skip (e.g. cover_art,qa)
--dry-run               Show commands without executing
```

**Examples:**
```bash
# Full export
python3 oxport.py run --engine Onset --wavs-dir ~/renders/onset/ --output-dir ~/dist/

# Skip cover art and QA for a quick iteration
python3 oxport.py run --engine Onset --wavs-dir ~/renders/ --output-dir ~/dist/ \
    --skip cover_art,qa

# Single preset
python3 oxport.py run --engine Onset --preset "808 Reborn" \
    --wavs-dir ~/renders/ --output-dir ~/dist/
```

---

### `oxport.py validate`
Validate pipeline output and/or preset quality.

```
--output-dir PATH       Check XPM/XPN structure in output directory
--presets               Run 10-point .xometa preset validation
--xpn FILE_OR_DIR       Validate .xpn archive(s) against Rex's XPN Bible
--fix                   Auto-fix trivially correctable issues
--strict                Treat warnings as errors (for CI)
```

**Examples:**
```bash
# Validate a finished pack
python3 oxport.py validate --xpn dist/Onset_IronMachines_v1.0.xpn

# Validate all .xpn files in a directory
python3 oxport.py validate --xpn dist/

# Full preset health check
python3 oxport.py validate --presets --strict

# Everything at once
python3 oxport.py validate --output-dir dist/ --presets --xpn dist/
```

---

### `oxport.py batch`
Run the pipeline for multiple engines from a JSON config.

```
--config FILE           JSON config with batch job definitions
--parallel N            Number of parallel jobs (default: 1)
--dry-run               Preview without executing
--skip-failed           Continue batch even if a job fails
```

**batch.json format:**
```json
{
  "batch_name": "V1 Launch Export",
  "output_base_dir": "./dist/",
  "jobs": [
    {
      "engine": "Onset",
      "wavs_dir": "/path/to/onset/wavs",
      "skip": []
    },
    {
      "engine": "Opal",
      "wavs_dir": "/path/to/opal/wavs",
      "skip": ["cover_art"]
    }
  ]
}
```

---

### `oxport.py status`
Show pipeline completion status for an output directory.

```bash
python3 oxport.py status --output-dir dist/onset/
```

---

## Supporting Tools

These tools complement the main pipeline:

| Tool | Purpose |
|------|---------|
| `xpn_dry_variant_duplicator.py` | Create FX-off dry variants of all presets |
| `xpn_engine_distinctness_analyzer.py` | Check that engines maintain distinct sonic identities |
| `xpn_preset_batch_generator.py` | Generate preset JSON from parameter templates |
| `xpn_kit_validator.py` | Validate kit structure before export |
| `xpn_pack_health_dashboard.py` | Fleet-wide pack health metrics |
| `xpn_fleet_health_dashboard.py` | Cross-engine DNA health overview |
| `xpn_coupling_recipes.py` | Coupling preset design and generation |
| `preset_audit.py` | Deep preset quality audit |
| `validate_presets.py` | Schema + DNA + naming validation |
| `xpn_validator.py` | .xpn archive structural validation (Rex's Bible) |

---

## Critical XPM Rules

Three rules — no exceptions:
- `KeyTrack = True` — samples must transpose across zones
- `RootNote = 0` — MPC auto-detect convention
- `VelStart = 0` on empty layers — prevents ghost triggering

`xpn_validator.py` checks all three with `[CRITICAL]` severity.

---

## Archive

One-time sprint scripts are in `Tools/_archive/`. They remain in git history if needed.
