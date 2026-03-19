"""
xpn_pack_readme_generator.py

Generates README.md for an XPN pack directory by reading:
  - expansion.json   — pack metadata (name, version, engines)
  - *.xpm            — program files (type, sample count)
  - *.xometa         — preset files (mood distribution)

CLI:
    python xpn_pack_readme_generator.py <pack_dir> [--output README.md]

Importable:
    from xpn_pack_readme_generator import generate_readme_for_pack
"""


import argparse
import json
import xml.etree.ElementTree as ET
from pathlib import Path

# ---------------------------------------------------------------------------
# Engine description library
# ---------------------------------------------------------------------------

ENGINE_DESCRIPTIONS: dict[str, str] = {
    "ONSET": (
        "ONSET is XO_OX's drum synthesis engine — eight dedicated synthesis voices "
        "(Kick, Snare, Closed Hat, Open Hat, Clap, Tom, Perc, FX) each built from "
        "physical models and layered noise circuits. The MACHINE macro shifts the "
        "character from acoustic-adjacent to full industrial, while PUNCH controls "
        "transient density. No samples required — every sound is synthesized in real time."
    ),
    "OPAL": (
        "OPAL is a granular synthesis engine inspired by the internal geometry of "
        "opal gemstones. It scatters audio grains across a probabilistic cloud, "
        "with controls for grain size, scatter, pitch spread, and density. The "
        "engine excels at evolving textures, spectral smears, and impossibly wide "
        "stereo fields. Every note is different — grain randomization is baked in."
    ),
    "OBLONG": (
        "OBLONG (XOblongBob) is a multi-mode synthesizer with 72 source files and "
        "167 factory presets. Its Apple Liquid Glass UI and PlaySurface chord/scale "
        "modes make it the most playable engine in the fleet. The engine covers "
        "everything from tight bass to sprawling pads, anchored by a flexible "
        "filter section and a four-macro performance surface."
    ),
    "OBESE": (
        "OBESE is the saturation and harmonic density engine — a feedback-saturated "
        "signal path built around the MOJO macro axis, which sweeps from clean analog "
        "character to full digital destruction. The Blessing B015 Mojo Control "
        "introduces an orthogonal analog/digital dimension that no other engine in the "
        "fleet replicates. Thick bass, crushed pads, and aggressive leads are its home."
    ),
    "OVERDUB": (
        "OVERDUB is a dub synthesizer and performance FX engine. Its signal chain "
        "mirrors the classic dub signal path: Voice → Send VCA → Drive → Tape Delay "
        "→ Spring Reverb → Master. The Spring Reverb received the Blessing B004 from "
        "Vangelis and Tomita for its metallic splash character. 38 parameters, 40 "
        "presets, and four performance pads (FIRE, XOSEND, ECHO CUT, PANIC) make it "
        "the most performance-oriented engine in XOmnibus."
    ),
    "ODDFELIX": (
        "ODDFELIX is the feliX pole engine — neon-bright, harmonically ordered, and "
        "forward in the mix. Named after feliX the neon tetra, it represents clarity, "
        "light, and tonal precision. Its accent color (Neon Tetra Blue #00A6D6) "
        "signals its role as the bright half of the feliX-Oscar polarity axis. "
        "Best paired with ODDOSCAR for full-spectrum coupling explorations."
    ),
    "ODDOSCAR": (
        "ODDOSCAR is the Oscar pole engine — dark, entropic, and recessed. Named after "
        "Oscar the axolotl, it embodies regeneration, noise, and deep sonic shadow. "
        "Its accent color (Axolotl Gill Pink #E8839B) marks its role as the textured, "
        "unpredictable half of the feliX-Oscar axis. Pairs naturally with ODDFELIX "
        "in Entangled and Family mood presets."
    ),
    "ODYSSEY": (
        "ODYSSEY is a wavetable and drift synthesis engine inspired by the sense of "
        "slow celestial travel. Wavetable oscillator mode is active (with a sine "
        "placeholder for v2 expansion), and the DRIFT modulation system creates "
        "organic pitch and timbre movement that never fully repeats. Ideal for "
        "evolving leads, textured chords, and sci-fi sound design."
    ),
    "OVERWORLD": (
        "OVERWORLD is a chip synthesis engine with three oscillator cores: the NES "
        "2A03, Sega Genesis YM2612, and SNES SPC700. The ERA crossfade macro sweeps "
        "across the entire 8-bit and 16-bit era in real time, and the CRT UI provides "
        "15 color themes and 13 glitch types. The Blessing B009 ERA Triangle — a 2D "
        "timbral crossfade inspired by Buchla, Schulze, Vangelis, and Pearlman — is "
        "the engine's defining innovation."
    ),
    "OHM": (
        "OHM is the sage-toned community instrument — a hippy-dad jam engine built "
        "around the MEDDLING/COMMUNE macro axis. Its Sage accent color (#87AE73) "
        "reflects its role as a grounding, harmonically generous voice in any ensemble. "
        "OHM rewards slow exploration and long sessions."
    ),
    "ORPHICA": (
        "ORPHICA is a microsound harp engine inspired by the siphonophore — a "
        "colonial organism where each unit is both individual and part of a whole. "
        "Its Siren Seafoam accent (#7FDBCA) reflects shimmering, multi-layered "
        "pluck textures that cascade across the stereo field."
    ),
}

ENGINE_DESCRIPTION_FALLBACK = (
    "A character synthesis engine in the XO_OX fleet. "
    "Load a program and use the four Q-Link macros (CHARACTER, MOVEMENT, COUPLING, SPACE) "
    "to explore the engine's sonic range."
)

# ---------------------------------------------------------------------------
# Mood emoji and labels
# ---------------------------------------------------------------------------

MOOD_EMOJI: dict[str, str] = {
    "Foundation": "🟦",
    "Atmosphere": "🟩",
    "Entangled":  "🔴",
    "Prism":      "🔮",
    "Flux":       "🌀",
    "Aether":     "⚡",
    "Family":     "🏠",
}

MOOD_ORDER = ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"]

# ---------------------------------------------------------------------------
# Producer tips by engine
# ---------------------------------------------------------------------------

ENGINE_TIPS: dict[str, list[str]] = {
    "ONSET": [
        "Use Q-Link 1 (MACHINE) to morph the entire kit character while looping a pattern.",
        "Layer ONSET pads with short OPAL grain clouds for hybrid acoustic-electronic drum sounds.",
        "Set PUNCH (Q2) fully clockwise for punchy club kicks; roll it back for vintage room tones.",
        "The FX voice responds well to extreme SPACE values — try it as a textural tail layer.",
    ],
    "OPAL": [
        "Set grain size small (≤ 20ms) for glassy shimmer; larger (>100ms) for lush pads.",
        "Automate grain scatter via Q2 (MOVEMENT) to build tension across an 8-bar loop.",
        "Pair OPAL with OVERDUB's Spring Reverb via COUPLING for infinite, smeared soundscapes.",
        "Run two OPAL programs in tandem — one feliX-biased, one Oscar-biased — for wide stereo width.",
    ],
    "OVERDUB": [
        "Hit the ECHO CUT pad on the MPC to clear delay tails for a classic dub drop effect.",
        "XOSEND routes signal into the spring reverb send; ride it manually for performance depth.",
        "Use FIRE to trigger the drive circuit hard — instant transient saturation on any sound.",
        "Keep SPACE (Q4) moderate and use PANIC to kill all tails cleanly before a section change.",
    ],
    "OBESE": [
        "MOJO at noon is your safe zone; push past 75% only for intentional overdrive moments.",
        "Layer a clean ODDFELIX patch under an OBESE bass for sub weight with tonal clarity.",
        "OBESE's saturation responds to MIDI velocity — play harder for more harmonic bite.",
    ],
}

GENERIC_TIPS = [
    "Use Q-Link 3 (COUPLING) to blend feliX (bright/ordered) and Oscar (dark/noisy) poles in real time.",
    "Automate Q-Link 4 (SPACE) from 0→127 over 16 bars to build depth gradually into a mix.",
    "Record a Q-Link performance pass before committing to MIDI note patterns — movement first.",
    "Try loading two programs from this pack into adjacent tracks and panning them 40L / 40R.",
    "Start from the Foundation presets when roughing out a track; switch to Flux or Aether in the outro.",
]

# ---------------------------------------------------------------------------
# Data readers
# ---------------------------------------------------------------------------

def _read_expansion_json(pack_dir: Path) -> dict:
    """Read expansion.json if present; return empty dict otherwise."""
    path = pack_dir / "expansion.json"
    if path.exists():
        try:
            return json.loads(path.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError):
            pass
    return {}


def _read_xpm_programs(pack_dir: Path) -> list[dict]:
    """
    Scan all .xpm files and return a list of dicts:
      { name, type, sample_count }
    """
    programs = []
    for xpm_path in sorted(pack_dir.rglob("*.xpm")):
        entry = {"name": xpm_path.stem, "type": "unknown", "sample_count": 0}
        try:
            tree = ET.parse(xpm_path)
            root = tree.getroot()
            # Program type lives on <Program type="Drum"> or <Program type="Keygroup">
            prog_elem = root if root.tag == "Program" else root.find(".//Program")
            if prog_elem is not None:
                raw_type = prog_elem.get("type", "").lower()
                if "drum" in raw_type:
                    entry["type"] = "Drum Kit"
                elif "keygroup" in raw_type:
                    entry["type"] = "Keygroup"
                else:
                    entry["type"] = raw_type.title() or "Program"
            # Count sample file references
            entry["sample_count"] = len(root.findall(".//SampleFile"))
        except ET.ParseError:
            pass
        programs.append(entry)
    return programs


def _read_xometa_presets(pack_dir: Path) -> dict[str, int]:
    """
    Scan all .xometa files and count presets per mood.
    Returns { mood: count }.
    """
    mood_counts: dict[str, int] = {}
    for meta_path in sorted(pack_dir.rglob("*.xometa")):
        try:
            data = json.loads(meta_path.read_text(encoding="utf-8"))
            mood = data.get("mood", "").strip()
            if mood:
                mood_counts[mood] = mood_counts.get(mood, 0) + 1
        except (json.JSONDecodeError, OSError):
            pass
    return mood_counts

# ---------------------------------------------------------------------------
# Emoji bar chart
# ---------------------------------------------------------------------------

BAR_WIDTH = 10


def _emoji_bar(count: int, total: int) -> str:
    if total == 0:
        return "░" * BAR_WIDTH
    filled = round(BAR_WIDTH * count / total)
    return "█" * filled + "░" * (BAR_WIDTH - filled)


def _render_mood_chart(mood_counts: dict[str, int]) -> str:
    if not mood_counts:
        return "_No .xometa preset files found in this pack._\n"
    total = sum(mood_counts.values())
    lines = ["| Mood | Count | Distribution |", "|------|-------|--------------|"]
    for mood in MOOD_ORDER:
        if mood not in mood_counts:
            continue
        count = mood_counts[mood]
        emoji = MOOD_EMOJI.get(mood, "▪")
        bar = _emoji_bar(count, total)
        lines.append(f"| {emoji} {mood} | {count} | `{bar}` |")
    # Any moods not in MOOD_ORDER
    for mood, count in sorted(mood_counts.items()):
        if mood not in MOOD_ORDER:
            bar = _emoji_bar(count, total)
            lines.append(f"| ▪ {mood} | {count} | `{bar}` |")
    lines.append(f"\n_Total presets: {total}_")
    return "\n".join(lines) + "\n"

# ---------------------------------------------------------------------------
# Tip selection
# ---------------------------------------------------------------------------

def _pick_tips(engines: list[str]) -> list[str]:
    tips: list[str] = []
    for eng in engines:
        tips.extend(ENGINE_TIPS.get(eng.upper(), []))
    # Pad with generics until we have 3–5
    for tip in GENERIC_TIPS:
        if len(tips) >= 5:
            break
        if tip not in tips:
            tips.append(tip)
    return tips[:5]

# ---------------------------------------------------------------------------
# Engine guide section
# ---------------------------------------------------------------------------

def _render_engine_guide(engines: list[str]) -> str:
    if not engines:
        return "_No engine information found in expansion.json._\n"
    sections = []
    for eng in engines:
        key = eng.upper()
        desc = ENGINE_DESCRIPTIONS.get(key, ENGINE_DESCRIPTION_FALLBACK)
        sections.append(f"### {key}\n\n{desc}\n")
    return "\n".join(sections)

# ---------------------------------------------------------------------------
# Program listing
# ---------------------------------------------------------------------------

def _render_program_listing(programs: list[dict]) -> str:
    if not programs:
        return "_No .xpm program files found in this pack._\n"
    lines = ["| Program | Type | Samples |", "|---------|------|---------|"]
    for p in programs:
        lines.append(f"| {p['name']} | {p['type']} | {p['sample_count']} |")
    return "\n".join(lines) + "\n"

# ---------------------------------------------------------------------------
# Main README builder
# ---------------------------------------------------------------------------

def generate_readme_for_pack(pack_dir: Path) -> str:
    """Read pack_dir and return full README.md content as a string."""
    pack_dir = pack_dir.resolve()

    expansion = _read_expansion_json(pack_dir)
    programs = _read_xpm_programs(pack_dir)
    mood_counts = _read_xometa_presets(pack_dir)

    # Derive metadata
    pack_name: str = expansion.get("name") or pack_dir.name
    version: str = str(expansion.get("version") or "1.0.0")
    engines_raw = expansion.get("engines") or []
    if isinstance(engines_raw, str):
        engines_raw = [engines_raw]
    engines: list[str] = [str(e).upper() for e in engines_raw]

    # Counts
    engine_count = len(engines)
    preset_count = sum(mood_counts.values()) if mood_counts else len(programs)
    program_count = len(programs)

    # Build badge line
    engine_list_str = ", ".join(engines) if engines else "—"
    badge_line = (
        f"**{pack_name}** &nbsp;|&nbsp; "
        f"v{version} &nbsp;|&nbsp; "
        f"{engine_count} engine{'s' if engine_count != 1 else ''} &nbsp;|&nbsp; "
        f"{preset_count} preset{'s' if preset_count != 1 else ''}"
    )

    # Sections
    program_listing = _render_program_listing(programs)
    mood_chart = _render_mood_chart(mood_counts) if mood_counts else (
        "_No .xometa preset files found in this pack._\n"
    )
    engine_guide = _render_engine_guide(engines)
    tips_list = "\n".join(f"- {t}" for t in _pick_tips(engines))

    readme = f"""\
# {pack_name}

{badge_line}

> Designed by XO_OX | Part of the XOmnibus instrument ecosystem
> [xo-ox.org](https://xo-ox.org)

---

## Quick Start

1. **Load the pack** — Open MPC's **Expansion Browser** (Menu → Expansions) and locate **{pack_name}**.
2. **Navigate banks** — Each `.xpm` program is a self-contained bank. Tap a program name to load it; use the **Bank A–H** buttons to step through pad layouts within the program.
3. **Use the Q-Links** — Four macros are pre-assigned to Q-Links 1–4: **CHARACTER** (Q1) shapes the core timbre, **MOVEMENT** (Q2) drives animation and rhythm, **COUPLING** (Q3) blends feliX and Oscar poles, and **SPACE** (Q4) controls reverb and spatial depth.

---

## What's Inside

{program_count} program{'s' if program_count != 1 else ''} &nbsp;|&nbsp; Engines: {engine_list_str}

{program_listing}
---

## Sound Map

{mood_chart}
---

## Engine Guide

{engine_guide}---

## Tips

{tips_list}

---

## Credits + License

Designed by XO_OX | XOmnibus Pack v{version} | Free to use in commercial productions

- Site: [xo-ox.org](https://xo-ox.org)
- Field Guide: [xo-ox.org/guide](https://xo-ox.org/guide)
- Aquarium (engine deep dives): [xo-ox.org/aquarium](https://xo-ox.org/aquarium)
"""
    return readme

# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description="Generate README.md for an XPN pack directory."
    )
    parser.add_argument(
        "pack_dir",
        help="Path to the pack directory containing expansion.json, .xpm, and .xometa files.",
    )
    parser.add_argument(
        "--output",
        default="README.md",
        help="Output filename (default: README.md). Relative paths are resolved from pack_dir.",
    )
    return parser.parse_args(argv)


def main(argv=None):
    args = _parse_args(argv)
    pack_dir = Path(args.pack_dir).resolve()

    if not pack_dir.is_dir():
        print(f"Error: '{pack_dir}' is not a directory.", flush=True)
        raise SystemExit(1)

    readme_content = generate_readme_for_pack(pack_dir)

    output_path = Path(args.output)
    if not output_path.is_absolute():
        output_path = pack_dir / output_path

    output_path.write_text(readme_content, encoding="utf-8")
    print(f"Written: {output_path}")


if __name__ == "__main__":
    main()
