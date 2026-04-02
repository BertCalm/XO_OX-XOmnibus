# .oxbuild Schema — v1.0

**Origin:** Vision Quest 005 ("The One-Command Pack")

A `.oxbuild` file is the source code for a musical instrument pack. It declares
what to build, not how to build it. The `oxport --build` compiler reads the
declaration and orchestrates the full pipeline: preset selection, rendering,
program assembly, intent generation, documentation, cover art, packaging, validation.

---

## Minimal Schema (4 required fields)

```json
{
  "pack_id": "mpce-perc-001",
  "engine": "ONSET",
  "platform": "mpce",
  "archetype": "percussion"
}
```

That's it. Four fields. The compiler derives everything else from defaults,
DNA analysis, and the existing tool configurations.

---

## Full Schema

```json
{
  "$schema": "https://xo-ox.org/schemas/oxbuild/v1.json",
  "schema_version": "1.0",

  "pack_id": "mpce-perc-001",
  "pack_name": "XO_OX MPCe Percussion Vol. 1",
  "version": "1.0.0",
  "author": "XO_OX Designs",

  "engine": "ONSET",
  "platform": "mpce",
  "archetype": "percussion",

  "corner_strategy": "dynamic_expression",
  "pad_count": 16,

  "preset_selection": {
    "mode": "auto_dna",
    "mood_filter": ["Foundation", "Kinetic"],
    "min_presets": 64,
    "dna_diversity_target": 0.7
  },

  "rendering": {
    "sample_rate": 44100,
    "bit_depth": 24,
    "velocity_layers": 4,
    "velocity_values": [20, 50, 90, 127],
    "render_spec_override": null
  },

  "intent": {
    "summary": "Finger drumming kit for MPCe 3D pads with four articulation corners per voice.",
    "target_player": "Live finger drummers and beat performers",
    "target_genre": ["hip-hop", "neo-soul", "boom-bap", "electronic"],
    "not_for": "Step-sequenced programming where articulation variety isn't needed"
  },

  "output": {
    "include_standard_version": true,
    "include_mpce_setup": true,
    "include_cover_art": true,
    "include_intent": true,
    "cover_art_badge": "MPCe Exclusive"
  },

  "experiment": {
    "embed_id": true,
    "tags": ["first-mpce-pack", "competitive-window"]
  }
}
```

---

## Field Reference

### Required Fields

| Field | Type | Description |
|-------|------|-------------|
| `pack_id` | string | Unique identifier. Convention: `{platform}-{archetype}-{edition}` |
| `engine` | string | Primary XOceanus engine name (ONSET, OPAL, etc.) |
| `platform` | string | Target: `mpce`, `standard`, `both` |
| `archetype` | string | Content type: `percussion`, `melodic`, `texture`, `coupled` |

### Optional Fields (with defaults)

| Field | Default | Description |
|-------|---------|-------------|
| `pack_name` | Auto-generated from pack_id | Display name |
| `version` | `"1.0.0"` | Semantic version |
| `author` | `"XO_OX Designs"` | Pack author |
| `corner_strategy` | `"dynamic_expression"` | Corner pattern (see xpn_intent_schema.md) |
| `pad_count` | `16` | Number of pads (4, 8, or 16) |
| `preset_selection.mode` | `"auto_dna"` | `auto_dna` (maximize diversity) or `manual` (list specific presets) |
| `preset_selection.mood_filter` | `null` (all moods) | Restrict to specific moods |
| `preset_selection.min_presets` | `pad_count * 4` | Minimum presets needed (4 per pad for quad-corner) |
| `preset_selection.dna_diversity_target` | `0.7` | Target DNA spread (0-1, higher = more diverse) |
| `rendering.sample_rate` | `44100` | Sample rate in Hz |
| `rendering.bit_depth` | `24` | Bit depth |
| `rendering.velocity_layers` | `4` | Number of velocity layers |
| `rendering.velocity_values` | `[20, 50, 90, 127]` | Vibe's musical curve |
| `rendering.render_spec_override` | `null` | Path to custom render spec JSON (overrides auto-generation) |
| `intent.summary` | Auto-generated | One-line pack summary |
| `intent.target_player` | Auto from archetype | Who this pack is for |
| `intent.target_genre` | `[]` | Target genres |
| `output.include_standard_version` | `true` | Generate non-MPCe fallback |
| `output.include_mpce_setup` | `true` | Generate MPCE_SETUP.md |
| `output.include_cover_art` | `true` | Generate cover art |
| `output.include_intent` | `true` | Generate .xpn_intent sidecar |
| `output.cover_art_badge` | `null` | Badge text on cover art |
| `experiment.embed_id` | `true` | Embed UUID experiment_id in outputs |
| `experiment.tags` | `[]` | Searchable tags for experiment tracking |

---

## Compilation Pipeline

When `oxport --build packs/mpce-perc-001.oxbuild` runs, the compiler executes:

```
1. PARSE      .oxbuild → BuildManifest
2. SELECT     Presets by DNA diversity (or manual list)
3. RENDER     oxport_render.py → 256 WAVs via BlackHole
4. ASSEMBLE   xpn_mpce_quad_builder.py → MPCe XPM
5. FALLBACK   xpn_drum_export.py → Standard XPM (if include_standard_version)
6. INTENT     xpn_intent_generator.py → xpn_intent.json
7. DOCS       MPCE_SETUP.md from template
8. ART        xpn_cover_art_generator_v2.py → cover art
9. PACKAGE    xpn_packager.py → .xpn ZIP
10. VALIDATE  xpn_validator.py → pass/fail report
```

Each stage can be skipped with `--skip {stage}` for partial builds:
- `--skip render` — use pre-rendered WAVs from a previous build
- `--skip art` — skip cover art generation
- `--dry-run` — parse and validate without executing

---

## Experiment ID

Every build embeds a UUID `experiment_id` in:
- `xpn_intent.json` → `provenance.experiment_id`
- `MPCE_SETUP.md` → footer comment
- Build log → `builds/{pack_id}/{timestamp}/build.log`

This enables passive behavioral tracking: when downloads/engagement data
exists in the future, it can be correlated back to specific build configurations.
Near-zero cost now, high analytical value later.
