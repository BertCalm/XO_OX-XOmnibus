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

### 2. ASSEMBLE stage — RESOLVED (2026-03-22)
- When `selected_presets` is populated (SELECT ran), ASSEMBLE now builds `qb.PresetInfo`
  objects directly from the selected_presets dicts instead of calling `qb.load_presets()`.
- Fallback: when SELECT did not run, still scans full presets tree (backward compatible).
- Dry-run output shows the source: "N selected presets" or "all {engine} presets".

### 3. WAV staging for multi-preset packs — RESOLVED (2026-03-22)
- When `selected_presets` is populated, WAVs are staged per preset into
  `Samples/{program_slug}/{preset_name}/` (using the `{preset_name}__` prefix in WAV names).
- When SELECT did not run, flat staging into `Samples/{program_slug}/` is preserved.

### 4. WAV content audit in VALIDATE — RESOLVED (2026-03-22)
- VALIDATE Phase 2 now audits renders/ WAVs: count, total size, zero-byte check.
- Compares actual WAV count against `render_manifest.json` total_jobs.
- Zero-byte WAVs cause VALIDATE failure (render likely silently failed).
- Gated behind `continue_on_error` like other critical checks.

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
