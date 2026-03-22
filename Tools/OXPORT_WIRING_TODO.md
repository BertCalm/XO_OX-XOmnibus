# Oxport Pipeline Wiring — Status & TODO

## Fixed (2026-03-22)

### SELECT → RENDER wiring (cmd_build, Stage 2 → Stage 3)
- `selected_presets` list (populated by SELECT) is now consumed by RENDER.
- Each selected preset is assigned `program = 0-based index` in the bank.
- Note-layout entries from the render spec are cloned once per selected preset,
  with `program` injected and slug prefixed by preset name.
- Total jobs = `N_presets × N_voice_entries × velocity_layers`.
- A `render_manifest.json` is written to the build dir for auditability.

### program numbers in render spec (`mpce_perc_001_render.json`)
- All 64 note-layout entries now have `"program": 0` (smoke-test default).
- When building from .oxbuild with SELECT enabled, RENDER overrides these
  with per-preset program numbers at runtime.
- `"_program_wiring_note"` field documents the design.

---

## Remaining TODOs

### 1. Bank ordering must match XOmnibus preset browser order
- MIDI program change `program=N` loads the Nth preset in the currently loaded bank.
- XOmnibus must be configured with ONSET presets loaded in the same order as
  `selected_presets.json` produces them (sorted by distance_from_center, then name).
- **Action needed**: verify the XOmnibus preset browser sort order matches the
  selector's output order, OR use bank_msb/bank_lsb to address presets by absolute
  position in the XOmnibus bank file.

### 2. ASSEMBLE stage does not consume render_manifest.json
- The ASSEMBLE stage (Stage 4) calls `xpn_mpce_quad_builder.load_presets()` independently,
  re-scanning the preset directory. It does not use `selected_presets.json` to know
  which presets were actually rendered.
- **Action needed**: pass `selected_presets` into ASSEMBLE so the XPM only references
  WAVs that were rendered, in the same order.

### 3. renders/ WAV staging for multi-preset packs
- With SELECT wired, renders/ now contains `N × 256` WAVs organized as
  `{preset_name}__{slug}__{note}__{vel}.WAV`.
- The ASSEMBLE stage's WAV staging loop (copies renders/ → Samples/) needs to
  handle subdirectory organization per preset.

### 4. Profile validator not called from RENDER
- `xpn_profile_validator.py` should run after render to verify the rendered WAVs
  match the profile phenotype assertions (e.g., kick_has_sub).
- Currently the VALIDATE stage runs at the end but does not check audio content,
  only XPM/XPN structure.

### 5. Preset load wait time
- `oxport_render.render_jobs()` uses `preset_load_ms=200` (hardcoded default).
- For XOmnibus with 50+ ONSET presets, preset switching may need longer settle time.
- **Action needed**: expose `preset_load_ms` in .oxbuild spec under `rendering:`.

---

## Architecture Reference

```
.oxbuild spec
    │
    ├── [1] PARSE     → BuildManifest (profile loaded)
    ├── [2] SELECT    → selected_presets[] + selected_presets.json + pack_dna.json
    │                   ↓ (wired 2026-03-22)
    ├── [3] RENDER    → injects program=N per preset into note-layout clone
    │                   → render_manifest.json
    │                   → renders/{preset}__{voice}__{note}__{vel}.WAV
    ├── [4] ASSEMBLE  → Programs/*.xpm  (TODO: consume selected_presets.json)
    ├── [5] FALLBACK  → standard XPM
    ├── [6] INTENT    → xpn_intent.json
    ├── [7] DOCS      → MPCE_SETUP.md
    ├── [8] ART       → artwork.png
    ├── [9] PACKAGE   → .xpn archive
    └── [10] VALIDATE → XPN integrity + phenotype check
```

## Smoke Test

```bash
# Dry run — verify SELECT→RENDER wiring prints correctly
python3 Tools/oxport.py build packs/mpce-perc-001.oxbuild --dry-run

# Single-preset render test (uses program=0 from render spec, no SELECT)
python3 Tools/oxport.py build packs/mpce-perc-001.oxbuild \
    --skip select,assemble,fallback,intent,docs,art,package,validate \
    --midi-port "XOmnibus" --audio-device "BlackHole"

# Full build with SELECT wired to RENDER
python3 Tools/oxport.py build packs/mpce-perc-001.oxbuild \
    --audio-device BlackHole
```
