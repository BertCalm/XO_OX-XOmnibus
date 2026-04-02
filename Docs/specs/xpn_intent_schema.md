# `.xpn_intent` — Pack Design Intent Schema

## Version 1.0 — March 2026

**Origin:** Vision Quest 004 ("The Pipeline That Speaks First")

A `.xpn_intent` JSON sidecar ships alongside every XPN pack. It captures the **design
philosophy** behind the pack — not what the samples are, but what they're *for*. This is
the semantic layer the entire MPC pack ecosystem is missing: machine-readable design intent
that enables intent-based search, AI-assisted discovery, and community curation.

---

## Why This Exists

Every XPN pack ships with samples, programs, and a manifest. None of these capture *why*
the pack was designed the way it was. A producer browsing packs sees names and categories
but has no way to search by intent ("I want a kit designed for finger drumming with
expressive ghost notes") or compare design philosophies across packs.

`.xpn_intent` fills this gap. It answers: what did the designer intend you to *feel* and
*do* with this content?

---

## Schema

```json
{
  "$schema": "https://xo-ox.org/schemas/xpn-intent/v1.json",
  "schema_version": "1.0",

  "pack": {
    "name": "XO_OX MPCe Percussion Vol. 1",
    "id": "xoox-mpce-perc-001",
    "version": "1.0.0",
    "author": "XO_OX Designs",
    "date": "2026-03-25",
    "engine": "ONSET",
    "engine_family": ["ONSET"],
    "mpce_native": true
  },

  "intent": {
    "summary": "Finger drumming kit designed for MPCe 3D pads. Four articulation corners per drum voice. Designed for expressive performance, not programming.",
    "target_player": "Live finger drummers and beat performers on MPC Live III / MPC XL",
    "target_genre": ["hip-hop", "neo-soul", "boom-bap", "electronic"],
    "design_philosophy": "Each pad is a complete instrument with four articulations. Strike position determines timbre. Designed for single-take performance recording.",
    "creative_direction": "Warm analog character with modern transient clarity. Ghost notes that whisper, accents that bite.",
    "not_for": "Step-sequenced programming where articulation variety isn't needed"
  },

  "pad_architecture": {
    "type": "mpce_quad_corner",
    "corner_pattern": "dynamic_expression",
    "corner_assignments": {
      "NW": {
        "role": "ghost_note",
        "description": "Quiet, muted, brush-adjacent. The whisper.",
        "sonic_character": "Low velocity, filtered, short decay"
      },
      "NE": {
        "role": "accent",
        "description": "Alternate strike — rimshot, edge hit, bright transient.",
        "sonic_character": "Bright, sharp attack, full body"
      },
      "SW": {
        "role": "standard",
        "description": "Center hit. The default voice of the instrument.",
        "sonic_character": "Balanced, natural, full range"
      },
      "SE": {
        "role": "effect",
        "description": "Processed variant — flam, buzz roll, or FX treatment.",
        "sonic_character": "Textured, layered, characterful"
      }
    },
    "xy_modulation": {
      "x_axis": {
        "target": "pan_or_reverb_send",
        "description": "Horizontal = spatial width. Left = dry/mono, Right = wet/wide."
      },
      "y_axis": {
        "target": "decay_or_release",
        "description": "Vertical = sustain. Down = tight/choked, Up = long/open."
      }
    },
    "z_axis": {
      "target": "filter_cutoff",
      "description": "Pressure after strike opens filter. Light touch = dark, heavy = bright."
    }
  },

  "experience_arc": {
    "description": "How the 16 pads tell a story when played together.",
    "pad_groups": [
      {
        "pads": ["A01", "A02", "A03", "A04"],
        "role": "core_kit",
        "description": "Kick, snare, closed hat, open hat. The foundation."
      },
      {
        "pads": ["A05", "A06", "A07", "A08"],
        "role": "color",
        "description": "Clap, toms, rimshot, crash. Fills and accents."
      },
      {
        "pads": ["A09", "A10", "A11", "A12"],
        "role": "texture",
        "description": "Percussion, shakers, woodblocks, bells. Rhythmic detail."
      },
      {
        "pads": ["A13", "A14", "A15", "A16"],
        "role": "atmosphere",
        "description": "FX hits, noise sweeps, reversed elements. Space and transition."
      }
    ]
  },

  "sonic_dna_range": {
    "description": "The DNA space this pack inhabits. Min/max across all presets used.",
    "brightness": { "min": 0.3, "max": 0.9 },
    "warmth": { "min": 0.4, "max": 0.8 },
    "movement": { "min": 0.1, "max": 0.6 },
    "density": { "min": 0.3, "max": 0.8 },
    "space": { "min": 0.1, "max": 0.5 },
    "aggression": { "min": 0.2, "max": 0.9 }
  },

  "rendering": {
    "sample_rate": 44100,
    "bit_depth": 24,
    "velocity_layers": 4,
    "velocity_splits": [
      { "layer": 1, "vel_start": 1, "vel_end": 20, "character": "ghost" },
      { "layer": 2, "vel_start": 21, "vel_end": 50, "character": "light" },
      { "layer": 3, "vel_start": 51, "vel_end": 90, "character": "medium" },
      { "layer": 4, "vel_start": 91, "vel_end": 127, "character": "hard" }
    ],
    "corner_variants": 4,
    "total_samples_per_pad": 16,
    "total_samples": 256
  },

  "compatibility": {
    "mpce_required": false,
    "mpce_enhanced": true,
    "standard_version_included": true,
    "minimum_firmware": "3.7",
    "hardware": ["MPC Live III", "MPC XL"],
    "fallback_hardware": ["MPC Live II", "MPC One", "MPC Key 61"],
    "fallback_description": "Standard version (1 sample per pad) included for non-MPCe hardware."
  },

  "provenance": {
    "xoceanus_version": "1.0",
    "source_presets": [],
    "quad_builder_version": "1.0",
    "render_spec": "render_specs/mpce_perc_001_render.json",
    "vision_quest": "VQ 003 + VQ 004"
  }
}
```

---

## Field Definitions

### `pack` — Pack identity
Standard metadata. `mpce_native: true` flags this as MPCe-designed content.

### `intent` — Design philosophy
The human-readable "why" behind this pack. `target_player` and `not_for` help
producers self-select — reducing returns and bad reviews.

### `pad_architecture` — How the pads are designed
- `corner_pattern`: one of `feliX_oscar_polarity`, `dynamic_expression`, `era_corners`,
  `coupling_state`, `instrument_articulation`, or `custom`
- `corner_assignments`: what each corner means semantically
- `xy_modulation` / `z_axis`: what the continuous tracking does

### `experience_arc` — The story the pads tell
Groups pads into functional roles. This is the "level design" concept from VQ 004 —
the pack is a navigable space with intentional emotional progression.

### `sonic_dna_range` — DNA envelope
The min/max DNA values across all source presets. Enables DNA-based pack search:
"find me a pack with high aggression and low space."

### `rendering` — Technical render parameters
How the WAVs were produced. Enables quality verification and re-rendering.

### `compatibility` — Hardware support
`mpce_enhanced: true` + `standard_version_included: true` = ships both versions.
Non-MPCe users get the standard pack; MPCe users get the quad-corner experience.

### `provenance` — Where this came from
Links back to XOceanus source presets, the render spec, and which Vision Quests
informed the design. The audit trail.

---

## File Placement

```
MyPack.xpn (ZIP archive)
├── Expansions/
│   └── manifest
├── Programs/
│   ├── MyKit_Standard.xpm
│   └── MyKit_MPCe.xpm
├── Samples/
│   └── ...
├── Docs/
│   └── MPCE_SETUP.md
└── xpn_intent.json          ← HERE (pack root)
```

The `.xpn_intent` file lives at the pack root, alongside the manifest.
It is NOT required for MPC to load the pack — it's metadata for XO_OX's
toolchain, discovery systems, and community curation.

---

## Future Extensions (v2)

- `gesture_recordings`: paired .xygesture files (from VQ 003)
- `coupling_recipes`: for coupled engine packs
- `community_tags`: user-contributed semantic tags
- `ai_embeddings`: vector embeddings for semantic search
