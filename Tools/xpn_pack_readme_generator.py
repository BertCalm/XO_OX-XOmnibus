"""
xpn_pack_readme_generator.py

Generates README.md and optional MPCE_SETUP.md for XPN packs.

CLI usage:
    python xpn_pack_readme_generator.py \
        --pack-name "ONSET Ritual Drums" \
        --engine ONSET \
        --mood Foundation \
        --presets 20 \
        --version 1.0.0 \
        [--mpce] \
        [--output ./]

Importable:
    from xpn_pack_readme_generator import generate_readme, generate_mpce_setup
"""

import argparse
import os
import sys

# ---------------------------------------------------------------------------
# Mood descriptions
# ---------------------------------------------------------------------------

MOOD_DESCRIPTIONS = {
    "Foundation": (
        "Foundation presets are the bedrock — steady, grounded, and production-ready. "
        "These kits anchor a mix without drawing attention to themselves."
    ),
    "Atmosphere": (
        "Atmosphere presets breathe and shimmer. They are designed for texture, "
        "space, and slow movement — ideal for intros, beds, and ambient passages."
    ),
    "Entangled": (
        "Entangled presets blur the line between rhythm and melody. "
        "Elements interact, interlock, and create emergent patterns."
    ),
    "Prism": (
        "Prism presets fracture sound into colour. Harmonic, spectral, "
        "and chromatic — these kits reward close listening."
    ),
    "Flux": (
        "Flux presets are in motion. Modulation, drift, and instability "
        "are features, not flaws. Nothing stays the same twice."
    ),
    "Aether": (
        "Aether presets exist at the edge of perception — delicate, "
        "spatial, and otherworldly. Best used with restraint."
    ),
    "Family": (
        "Family presets bridge engines. They are designed to work "
        "alongside other XO_OX instruments in a shared sonic world."
    ),
}

MOOD_FALLBACK = "A curated collection tuned for production use across a range of styles and contexts."

# ---------------------------------------------------------------------------
# Engine format hints
# ---------------------------------------------------------------------------

ENGINE_FORMAT = {
    "ONSET": "drum kit (.xpm)",
    "OBLONG": "keygroup program (.xpm)",
    "OVERBITE": "keygroup program (.xpm)",
    "OVERDUB": "keygroup program (.xpm)",
    "ODYSSEY": "keygroup program (.xpm)",
    "OPAL": "keygroup program (.xpm)",
    "OVERWORLD": "keygroup program (.xpm)",
    "OHM": "keygroup program (.xpm)",
    "ORPHICA": "keygroup program (.xpm)",
    "OBBLIGATO": "keygroup program (.xpm)",
    "OTTONI": "keygroup program (.xpm)",
    "OLE": "keygroup program (.xpm)",
    "OVERLAP": "keygroup program (.xpm)",
    "OUTWIT": "keygroup program (.xpm)",
}

ENGINE_FORMAT_FALLBACK = "keygroup program (.xpm)"


def _engine_format(engine: str) -> str:
    return ENGINE_FORMAT.get(engine.upper(), ENGINE_FORMAT_FALLBACK)


def _mood_description(mood: str) -> str:
    return MOOD_DESCRIPTIONS.get(mood, MOOD_FALLBACK)


# ---------------------------------------------------------------------------
# README generator
# ---------------------------------------------------------------------------

README_TEMPLATE = """\
# {pack_name}

> **XO_OX** — {engine} engine | v{version}
> [xo-ox.org](https://xo-ox.org)

---

## What's Inside

| Detail        | Value                          |
|---------------|--------------------------------|
| Pack Name     | {pack_name}                    |
| Engine        | {engine}                       |
| Format        | {format}                       |
| Preset Count  | {presets}                      |
| Mood          | {mood}                         |
| Version       | {version}                      |

**{mood}** — {mood_description}

---

## Loading on MPC

1. Connect your MPC (or open MPC Software / MPC Beats).
2. Press **Menu** and navigate to **Expansions**.
3. Select **Browse** and locate **{pack_name}**.
4. Open the pack folder and tap the `.xpm` file you want to load.
5. The program loads into the current track. Hit a pad to play.

> If the pack does not appear, confirm the expansion folder is set in
> **Preferences → Expansions Directory** and that the `.xpn` file has
> been installed (drag to the MPC or use MPC Software's expansion installer).

---

## Macro Guide

XOmnibus programs expose four performance macros mapped to Q-Links 1–4.

| Macro       | Q-Link | Description                                              |
|-------------|--------|----------------------------------------------------------|
| CHARACTER   | Q1     | Shifts the core tonal character of the engine.           |
|             |        | Low = raw/dry; High = processed/shaped.                  |
| MOVEMENT    | Q2     | Controls rhythmic animation and internal motion.         |
|             |        | Low = static; High = active/evolving.                    |
| COUPLING    | Q3     | Blends feliX and Oscar poles of the engine.              |
|             |        | Low = pure feliX; High = pure Oscar.                     |
| SPACE       | Q4     | Controls reverb, delay, and spatial depth.               |
|             |        | Low = dry/close; High = deep/ambient.                    |

---

## feliX-Oscar Axis

Every XO_OX instrument lives on a polarity axis between two characters:

- **feliX** — bright, forward, harmonic, ordered, light
- **Oscar** — dark, recessed, noisy, entropic, heavy

The COUPLING macro sweeps between these poles in real time. Most presets
in this pack are balanced, but explore the extremes to find the version
that fits your track.

---

## Made with XOmnibus

**{pack_name}** is part of the XO_OX instrument ecosystem.

- Full instrument suite: [xo-ox.org](https://xo-ox.org)
- Field Guide (tutorials + philosophy): [xo-ox.org/guide](https://xo-ox.org/guide)
- Aquarium (engine deep dives): [xo-ox.org/aquarium](https://xo-ox.org/aquarium)

---

*XO_OX | {pack_name} v{version}*
"""


def generate_readme(
    pack_name: str,
    engine: str,
    mood: str,
    presets: int,
    version: str,
) -> str:
    """Return the README.md content as a string."""
    return README_TEMPLATE.format(
        pack_name=pack_name,
        engine=engine.upper(),
        mood=mood,
        mood_description=_mood_description(mood),
        format=_engine_format(engine),
        presets=presets,
        version=version,
    )


# ---------------------------------------------------------------------------
# MPCE_SETUP generator
# ---------------------------------------------------------------------------

MPCE_TEMPLATE = """\
# {pack_name} — MPCe Quad-Corner Setup

> XO_OX MPCe format | {engine} engine | v{version}

---

## What is the MPCe Quad-Corner Format?

MPCe (MPC Expression) programs use the four corners of each pad to expose
four distinct timbral positions at once. Each corner is assigned by physical
position on the pad surface:

| Corner | Position    | Character         |
|--------|-------------|-------------------|
| NW     | Top-left    | feliX / Dry       |
| NE     | Top-right   | feliX / Wet       |
| SW     | Bottom-left | Oscar / Dry       |
| SE     | Bottom-right| Oscar / Wet       |

Striking the centre of a pad gives a blended mid-point reading across
all four corners.

---

## Loading the MPCe Variant

1. In the pack folder, locate the subfolder named `_MPCE`.
2. Open that folder and select the `.xpm` file ending in `_MPCE.xpm`.
3. Load it into a track as you would any standard program.
4. Each pad now responds to where on the pad surface you strike.

> Standard `.xpm` files (without `_MPCE`) load as single-corner programs.
> Use those if your workflow does not require quad-corner expression.

---

## Corner Assignment Reference

```
+------------------+------------------+
|                  |                  |
|   NW             |   NE             |
|   feliX / Dry    |   feliX / Wet    |
|                  |                  |
+------------------+------------------+
|                  |                  |
|   SW             |   SE             |
|   Oscar / Dry    |   Oscar / Wet    |
|                  |                  |
+------------------+------------------+
```

- **feliX axis** (top): bright, harmonic, forward character
- **Oscar axis** (bottom): dark, noisy, recessed character
- **Dry axis** (left): minimal processing, direct signal
- **Wet axis** (right): full reverb/delay, spatial depth

---

## Z-Axis (Pressure) Tips

If your MPC model supports aftertouch / pressure (Z-axis):

- Pressure is routed to **SPACE** (reverb depth) by default.
  Strike and hold to push sounds into the room.
- For sustained pads, use pressure to open or close the space gradually
  rather than striking a new note.
- Combine corner position (feliX/Oscar) with pressure (dry/wet) for a
  fully continuous 3-dimensional performance surface.

---

## feliX-Oscar Quick Reference

| Parameter  | feliX (low COUPLING) | Oscar (high COUPLING) |
|------------|---------------------|-----------------------|
| Character  | Bright, harmonic    | Dark, noisy           |
| Movement   | Ordered, rhythmic   | Entropic, drifting    |
| Space      | Intimate, close     | Deep, cavernous       |

---

*XO_OX | {pack_name} v{version} | MPCe Setup Guide*
"""


def generate_mpce_setup(
    pack_name: str,
    engine: str,
    version: str,
) -> str:
    """Return the MPCE_SETUP.md content as a string."""
    return MPCE_TEMPLATE.format(
        pack_name=pack_name,
        engine=engine.upper(),
        version=version,
    )


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description="Generate README.md (and optionally MPCE_SETUP.md) for an XPN pack."
    )
    parser.add_argument("--pack-name", required=True, help='Pack name, e.g. "ONSET Ritual Drums"')
    parser.add_argument("--engine", required=True, help="Engine name, e.g. ONSET")
    parser.add_argument("--mood", required=True, help="Mood tag, e.g. Foundation")
    parser.add_argument("--presets", required=True, type=int, help="Number of presets")
    parser.add_argument("--version", required=True, help="Version string, e.g. 1.0.0")
    parser.add_argument("--mpce", action="store_true", help="Also generate MPCE_SETUP.md")
    parser.add_argument("--output", default="./", help="Output directory (default: ./)")
    return parser.parse_args(argv)


def main(argv=None):
    args = _parse_args(argv)

    output_dir = os.path.abspath(args.output)
    os.makedirs(output_dir, exist_ok=True)

    readme_content = generate_readme(
        pack_name=args.pack_name,
        engine=args.engine,
        mood=args.mood,
        presets=args.presets,
        version=args.version,
    )
    readme_path = os.path.join(output_dir, "README.md")
    with open(readme_path, "w", encoding="utf-8") as f:
        f.write(readme_content)
    print(f"Written: {readme_path}")

    if args.mpce:
        mpce_content = generate_mpce_setup(
            pack_name=args.pack_name,
            engine=args.engine,
            version=args.version,
        )
        mpce_path = os.path.join(output_dir, "MPCE_SETUP.md")
        with open(mpce_path, "w", encoding="utf-8") as f:
            f.write(mpce_content)
        print(f"Written: {mpce_path}")


if __name__ == "__main__":
    main()
