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

### 1. Bank ordering — RESOLVED (2026-03-22)
- `select_presets()` was returning presets in farthest-point-sampling (diversity)
  order, not XOmnibus browser order.
- **Fix**: After `selector.select_presets()` returns, `oxport.py` now re-sorts
  `selected_presets` by `path` (ascending) — identical to Python `sorted()` on
  `.xometa` paths, which matches JUCE `File::findChildFiles` alphabetical ordering.
- This guarantees MIDI program change `N` loads the same preset that the XOmnibus
  browser shows at position `N`.
- Bank authoritative reference: `Docs/render_specs/mpce_perc_001_bank_index.json`
  (633 ONSET presets, programs 0–632, alphabetical path order).

### 5. Preset load wait time — RESOLVED (2026-03-22)
- `preset_load_ms` is now read from `rendering.preset_load_ms` in the `.oxbuild`
  spec and passed through to `oxport_render.render_jobs()`.
- Default in `oxport.py` changed from 200 to 400ms.
- `packs/mpce-perc-001.oxbuild` now sets `"preset_load_ms": 400` explicitly.

---

## Remaining TODOs

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
