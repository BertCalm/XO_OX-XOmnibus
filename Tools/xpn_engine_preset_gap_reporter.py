#!/usr/bin/env python3
"""
xpn_engine_preset_gap_reporter.py

Gap report tool for XO_OX XOceanus engine presets.
Analyzes mood/DNA coverage for a specific engine and generates
prescription suggestions for missing zones.

Usage:
    python xpn_engine_preset_gap_reporter.py --engine OPAL --preset-dir Presets/XOceanus
    python xpn_engine_preset_gap_reporter.py --engine OPAL --preset-dir Presets/XOceanus --output gap_report.md
"""

import argparse
import json
from pathlib import Path
from collections import defaultdict

# ── Constants ────────────────────────────────────────────────────────────────

MOODS = ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family",
         "Submerged", "Coupling", "Crystalline", "Deep", "Ethereal", "Kinetic", "Luminous",
         "Organic", "Shadow"]

DNA_AXES = ["brightness", "warmth", "movement"]  # primary 3 for 3×3×3 zones
ZONE_LABELS = ["low", "mid", "high"]

UNDER_REPRESENTED_THRESHOLD = 3

# Word bank for prescription name generation
# Keys roughly correspond to (brightness_zone, mood_character)
WORD_BANK = {
    "bright": ["Shimmer", "Glint", "Spark", "Flare", "Gleam", "Radiance", "Prism", "Crystal",
               "Aurora", "Blaze", "Luminance", "Sheen"],
    "mid": ["Dusk", "Haze", "Drift", "Veil", "Mist", "Echo", "Bloom", "Pulse",
            "Current", "Flow", "Gradient", "Contour"],
    "dark": ["Hollow", "Abyss", "Shadow", "Depth", "Murk", "Obsidian", "Void", "Undertow",
             "Cavern", "Gloom", "Cinder", "Undertone"],
    "warm": ["Ember", "Amber", "Ochre", "Honey", "Copper", "Rust", "Glow", "Smolder"],
    "cool": ["Tundra", "Glacier", "Slate", "Frost", "Indigo", "Cobalt", "Steel", "Chrome"],
    "active": ["Storm", "Rush", "Surge", "Flux", "Torrent", "Cascade", "Frenzy", "Churn"],
    "still": ["Stasis", "Stillness", "Suspension", "Pause", "Lull", "Calm", "Plateau", "Equilibrium"],
    "foundation": ["Root", "Anchor", "Ground", "Base", "Core", "Bedrock", "Foundation", "Grain"],
    "atmosphere": ["Cloud", "Canopy", "Layer", "Blanket", "Veil", "Wash", "Envelope", "Expanse"],
    "entangled": ["Web", "Knot", "Thread", "Lattice", "Tangle", "Braid", "Mesh", "Weave"],
    "prism": ["Refraction", "Spectrum", "Band", "Split", "Angle", "Facet", "Lens", "Scatter"],
    "flux": ["Shift", "Morph", "Warp", "Bend", "Fold", "Phase", "Churn", "Transition"],
    "aether": ["Nebula", "Void", "Cosmos", "Orbit", "Stellar", "Quantum", "Entropy", "Singularity"],
    "family": ["Sibling", "Kin", "Lineage", "Bond", "Pair", "Echo", "Mirror", "Resonance"],
}

# Macro direction hints per mood
MACRO_HINTS = {
    "Foundation": {
        "CHARACTER": "set low-mid (0.2–0.5) for grounded character",
        "MOVEMENT": "set low (0.0–0.3) for stable, minimal motion",
        "COUPLING": "set low (0.0–0.2) — standalone voice",
        "SPACE": "set mid (0.3–0.6) for intimate but present space",
    },
    "Atmosphere": {
        "CHARACTER": "set mid (0.3–0.6) for ambient texture",
        "MOVEMENT": "set low-mid (0.1–0.4) for gentle drift",
        "COUPLING": "set mid (0.3–0.6) — soft coupling encouraged",
        "SPACE": "set high (0.6–1.0) for expansive air",
    },
    "Entangled": {
        "CHARACTER": "set mid-high (0.4–0.8) to expose coupling character",
        "MOVEMENT": "set mid (0.3–0.6) — motion serves the entanglement",
        "COUPLING": "set high (0.6–1.0) — coupling IS the sound",
        "SPACE": "set mid (0.3–0.6) — tight space reinforces coupling",
    },
    "Prism": {
        "CHARACTER": "set high (0.6–1.0) for spectral brightness",
        "MOVEMENT": "set mid-high (0.4–0.8) for prismatic shimmer",
        "COUPLING": "set low-mid (0.2–0.5) — let the timbre lead",
        "SPACE": "set mid-high (0.5–0.8) for crystalline depth",
    },
    "Flux": {
        "CHARACTER": "set mid-high (0.5–0.9) — character in motion",
        "MOVEMENT": "set high (0.7–1.0) for maximum flux",
        "COUPLING": "set mid (0.3–0.6) — dynamic coupling",
        "SPACE": "set mid (0.3–0.6) — space shifts with movement",
    },
    "Aether": {
        "CHARACTER": "set low (0.0–0.3) for ethereal, formless quality",
        "MOVEMENT": "set low (0.0–0.3) — suspended, near-static",
        "COUPLING": "set low (0.0–0.2) — self-contained",
        "SPACE": "set high (0.7–1.0) for cosmic scale",
    },
    "Family": {
        "CHARACTER": "set mid (0.4–0.6) — balanced, welcoming character",
        "MOVEMENT": "set mid (0.3–0.5) — accessible motion",
        "COUPLING": "set mid-high (0.5–0.8) — designed for coupling",
        "SPACE": "set mid (0.4–0.6) — comfortable room",
    },
}

# ── Helpers ───────────────────────────────────────────────────────────────────

def dna_zone(value: float) -> int:
    """Map a 0–1 DNA value to a zone index: 0=low, 1=mid, 2=high."""
    if value < 0.33:
        return 0
    elif value < 0.67:
        return 1
    else:
        return 2


def zone_center(zone: int) -> float:
    """Return a representative DNA value for a zone."""
    return [0.2, 0.5, 0.8][zone]


def zone_label(zone: int) -> str:
    return ZONE_LABELS[zone]


def is_felix(brightness: float) -> bool:
    """feliX polarity: bright (>=0.5). Oscar: dark (<0.5)."""
    return brightness >= 0.5


def generate_name(b_zone: int, w_zone: int, m_zone: int, mood: str) -> str:
    """Generate a suggestive preset name from zone coordinates and mood."""
    import random
    random.seed(b_zone * 100 + w_zone * 10 + m_zone + hash(mood) % 97)

    b_key = ["dark", "mid", "bright"][b_zone]
    w_key = ["cool", "mid", "warm"][w_zone]
    m_key = ["still", "mid", "active"][m_zone]
    mood_key = mood.lower()

    pool_a = WORD_BANK.get(b_key, WORD_BANK["mid"])
    pool_b = WORD_BANK.get(mood_key, WORD_BANK.get(w_key, WORD_BANK["warm"]))

    word_a = pool_a[random.randint(0, len(pool_a) - 1)]
    word_b = pool_b[random.randint(0, len(pool_b) - 1)]

    # Avoid same word twice
    attempts = 0
    while word_a == word_b and attempts < 10:
        word_b = pool_b[random.randint(0, len(pool_b) - 1)]
        attempts += 1

    return f"{word_a} {word_b}"


def load_presets(preset_dir: Path, engine: str) -> list[dict]:
    """Load all .xometa files that include the given engine."""
    engine_upper = engine.upper()
    presets = []
    missing_dna = 0

    for xometa in preset_dir.rglob("*.xometa"):
        try:
            with xometa.open() as f:
                data = json.load(f)
        except (json.JSONDecodeError, OSError):
            continue

        engines_in_preset = [e.upper() for e in data.get("engines", [])]
        if engine_upper not in engines_in_preset:
            continue

        dna = data.get("dna")
        if not dna:
            missing_dna += 1
            continue

        presets.append(data)

    return presets, missing_dna


# ── Analysis ──────────────────────────────────────────────────────────────────

def analyze_presets(presets: list[dict]) -> dict:
    mood_counts = defaultdict(int)
    dna_grid = defaultdict(list)  # (b_zone, w_zone, m_zone) -> [preset names]
    felix_count = 0
    oscar_count = 0
    family_count = 0

    for p in presets:
        mood = p.get("mood", "Unknown")
        mood_counts[mood] += 1

        if "family" in [t.lower() for t in p.get("tags", [])]:
            family_count += 1

        dna = p.get("dna", {})
        b = dna.get("brightness", 0.5)
        w = dna.get("warmth", 0.5)
        m = dna.get("movement", 0.5)

        bz, wz, mz = dna_zone(b), dna_zone(w), dna_zone(m)
        dna_grid[(bz, wz, mz)].append(p.get("name", "Unnamed"))

        if is_felix(b):
            felix_count += 1
        else:
            oscar_count += 1

    return {
        "mood_counts": dict(mood_counts),
        "dna_grid": dict(dna_grid),
        "felix_count": felix_count,
        "oscar_count": oscar_count,
        "family_count": family_count,
        "total": len(presets),
    }


def find_gaps(analysis: dict) -> dict:
    mood_counts = analysis["mood_counts"]
    dna_grid = analysis["dna_grid"]

    missing_moods = [m for m in MOODS if mood_counts.get(m, 0) == 0]
    thin_moods = [m for m in MOODS if 0 < mood_counts.get(m, 0) < UNDER_REPRESENTED_THRESHOLD]

    empty_zones = []
    for bz in range(3):
        for wz in range(3):
            for mz in range(3):
                key = (bz, wz, mz)
                if key not in dna_grid or len(dna_grid[key]) == 0:
                    empty_zones.append(key)

    return {
        "missing_moods": missing_moods,
        "thin_moods": thin_moods,
        "empty_zones": empty_zones,
    }


# ── Prescription generation ───────────────────────────────────────────────────

def prescriptions_for_mood_gaps(missing_moods: list[str], engine: str) -> list[dict]:
    prescriptions = []
    for mood in missing_moods:
        # Pick a neutral DNA zone for the mood as default
        b_zone = 1  # mid brightness by default
        if mood in ("Prism", "Foundation"):
            b_zone = 2  # bright
        elif mood == "Aether":
            b_zone = 0  # dark
        w_zone = 1
        m_zone = 1
        if mood == "Flux":
            m_zone = 2
        elif mood == "Aether":
            m_zone = 0

        prescriptions.append({
            "type": "missing_mood",
            "mood": mood,
            "name": generate_name(b_zone, w_zone, m_zone, mood),
            "dna": {
                "brightness": zone_center(b_zone),
                "warmth": zone_center(w_zone),
                "movement": zone_center(m_zone),
            },
            "macros": MACRO_HINTS.get(mood, {}),
            "note": f"Engine {engine} has 0 presets in the {mood} mood — this is the minimum viable prescription.",
        })
    return prescriptions


def prescriptions_for_zone_gaps(empty_zones: list[tuple], engine: str, existing_moods: list[str]) -> list[dict]:
    prescriptions = []
    for (bz, wz, mz) in empty_zones:
        # Guess the best mood for this zone
        if bz == 0 and mz == 0:
            mood = "Aether"
        elif bz == 2 and mz == 0:
            mood = "Foundation"
        elif bz == 2 and mz == 2:
            mood = "Prism"
        elif mz == 2:
            mood = "Flux"
        elif bz == 1 and wz == 1 and mz == 1:
            mood = "Atmosphere"
        else:
            mood = "Atmosphere"

        # Prefer moods that exist in the collection
        if mood not in existing_moods and existing_moods:
            mood = existing_moods[0]

        prescriptions.append({
            "type": "empty_dna_zone",
            "zone": (bz, wz, mz),
            "zone_label": f"brightness={zone_label(bz)}, warmth={zone_label(wz)}, movement={zone_label(mz)}",
            "mood": mood,
            "name": generate_name(bz, wz, mz, mood),
            "dna": {
                "brightness": zone_center(bz),
                "warmth": zone_center(wz),
                "movement": zone_center(mz),
            },
            "macros": MACRO_HINTS.get(mood, {}),
        })
    return prescriptions


# ── Markdown rendering ────────────────────────────────────────────────────────

def render_report(engine: str, presets: list[dict], analysis: dict, gaps: dict,
                  mood_prescriptions: list[dict], zone_prescriptions: list[dict]) -> str:
    total = analysis["total"]
    felix = analysis["felix_count"]
    oscar = analysis["oscar_count"]
    family = analysis["family_count"]
    felix_pct = round(felix / total * 100) if total else 0
    oscar_pct = round(oscar / total * 100) if total else 0

    lines = []
    lines.append(f"# {engine} Preset Gap Report")
    lines.append("")
    lines.append(f"> Generated for engine **{engine}** | {total} presets analyzed")
    lines.append("")

    # ── Summary ──
    lines.append("## Summary")
    lines.append("")
    lines.append(f"- **Total presets:** {total}")
    lines.append(f"- **feliX (bright):** {felix} ({felix_pct}%)")
    lines.append(f"- **Oscar (dark):** {oscar} ({oscar_pct}%)")
    lines.append(f"- **Family-tagged:** {family}")
    lines.append(f"- **Missing moods:** {len(gaps['missing_moods'])} — {', '.join(gaps['missing_moods']) if gaps['missing_moods'] else 'none'}")
    lines.append(f"- **Under-represented moods (<{UNDER_REPRESENTED_THRESHOLD}):** {len(gaps['thin_moods'])} — {', '.join(gaps['thin_moods']) if gaps['thin_moods'] else 'none'}")
    lines.append(f"- **Empty DNA zones (3×3×3 grid):** {len(gaps['empty_zones'])} of 27")
    lines.append("")

    # ── Mood coverage table ──
    lines.append("## Mood Coverage")
    lines.append("")
    lines.append("| Mood | Count | Status |")
    lines.append("|------|-------|--------|")
    for mood in MOODS:
        count = analysis["mood_counts"].get(mood, 0)
        if count == 0:
            status = "MISSING"
        elif count < UNDER_REPRESENTED_THRESHOLD:
            status = f"THIN ({count})"
        else:
            status = f"OK ({count})"
        lines.append(f"| {mood} | {count} | {status} |")
    lines.append("")

    # ── DNA grid ──
    lines.append("## DNA Coverage (3×3×3 Brightness × Warmth × Movement Zones)")
    lines.append("")
    lines.append("Zones show preset counts. Empty (0) zones are gaps.")
    lines.append("")
    lines.append("| Brightness | Warmth | Movement | Count | Sample Names |")
    lines.append("|-----------|--------|----------|-------|-------------|")
    for bz in range(3):
        for wz in range(3):
            for mz in range(3):
                key = (bz, wz, mz)
                names = analysis["dna_grid"].get(key, [])
                count = len(names)
                sample = ", ".join(names[:3]) + ("…" if len(names) > 3 else "")
                marker = " **EMPTY**" if count == 0 else ""
                lines.append(f"| {zone_label(bz)} | {zone_label(wz)} | {zone_label(mz)} | {count}{marker} | {sample} |")
    lines.append("")

    # ── feliX / Oscar balance ──
    lines.append("## feliX / Oscar Balance")
    lines.append("")
    bar_width = 30
    felix_bar = round(felix_pct / 100 * bar_width)
    oscar_bar = bar_width - felix_bar
    lines.append(f"```")
    lines.append(f"feliX (bright)  {'█' * felix_bar}{'░' * oscar_bar}  {felix_pct}%  ({felix} presets)")
    lines.append(f"Oscar  (dark)   {'░' * felix_bar}{'█' * oscar_bar}  {oscar_pct}%  ({oscar} presets)")
    lines.append(f"```")
    if abs(felix_pct - oscar_pct) > 30:
        dominant = "feliX" if felix > oscar else "Oscar"
        lines.append(f"> **Imbalance detected** — {dominant} polarity dominates. Consider adding more {('Oscar' if felix > oscar else 'feliX')} presets.")
    else:
        lines.append("> Balance is within acceptable range.")
    lines.append("")

    # ── Prescriptions ──
    if mood_prescriptions or zone_prescriptions:
        lines.append("## Prescription Suggestions")
        lines.append("")
        lines.append(f"The following presets are prescribed to close the gaps for engine **{engine}**.")
        lines.append("")

        if mood_prescriptions:
            lines.append("### Mood Gap Prescriptions")
            lines.append("")
            for i, p in enumerate(mood_prescriptions, 1):
                lines.append(f"#### {i}. {p['name']}  *(mood: {p['mood']})*")
                lines.append("")
                lines.append(f"**Gap type:** Missing mood — {p['mood']} has 0 presets for {engine}")
                lines.append("")
                dna = p["dna"]
                lines.append(f"**Suggested DNA:**")
                lines.append(f"- `brightness`: {dna['brightness']:.2f}")
                lines.append(f"- `warmth`: {dna['warmth']:.2f}")
                lines.append(f"- `movement`: {dna['movement']:.2f}")
                lines.append("")
                lines.append(f"**Macro directions:**")
                for macro, hint in p["macros"].items():
                    lines.append(f"- **{macro}**: {hint}")
                lines.append("")
                if "note" in p:
                    lines.append(f"*{p['note']}*")
                    lines.append("")
                lines.append("---")
                lines.append("")

        if zone_prescriptions:
            lines.append("### Empty DNA Zone Prescriptions")
            lines.append("")
            for i, p in enumerate(zone_prescriptions, 1):
                bz, wz, mz = p["zone"]
                lines.append(f"#### {i}. {p['name']}  *(zone: {p['zone_label']})*")
                lines.append("")
                lines.append(f"**Gap type:** Empty 3D DNA zone — no presets occupy `brightness={zone_label(bz)} / warmth={zone_label(wz)} / movement={zone_label(mz)}`")
                lines.append(f"**Suggested mood:** {p['mood']}")
                lines.append("")
                dna = p["dna"]
                lines.append(f"**Suggested DNA:**")
                lines.append(f"- `brightness`: {dna['brightness']:.2f}")
                lines.append(f"- `warmth`: {dna['warmth']:.2f}")
                lines.append(f"- `movement`: {dna['movement']:.2f}")
                lines.append("")
                lines.append(f"**Macro directions:**")
                for macro, hint in p["macros"].items():
                    lines.append(f"- **{macro}**: {hint}")
                lines.append("")
                lines.append("---")
                lines.append("")
    else:
        lines.append("## Prescriptions")
        lines.append("")
        lines.append("No gaps found — all moods and DNA zones are covered.")
        lines.append("")

    lines.append("---")
    lines.append("*Generated by `xpn_engine_preset_gap_reporter.py` — XO_OX Designs*")

    return "\n".join(lines)


# ── CLI ───────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Generate a DNA/mood gap report for an XO_OX XOceanus engine.",
    )
    parser.add_argument("--engine", required=True, help="Engine name, e.g. OPAL")
    parser.add_argument(
        "--preset-dir",
        default="Presets/XOceanus",
        help="Path to XOceanus preset directory (default: Presets/XOceanus)",
    )
    parser.add_argument(
        "--output",
        default=None,
        help="Output Markdown file path (default: print to stdout)",
    )
    args = parser.parse_args()

    engine = args.engine.upper()
    preset_dir = Path(args.preset_dir).expanduser().resolve()

    if not preset_dir.exists():
        print(f"ERROR: Preset directory not found: {preset_dir}")
        raise SystemExit(1)

    print(f"Loading presets for engine '{engine}' from {preset_dir} …")
    presets, missing_dna = load_presets(preset_dir, engine)
    print(f"  Found {len(presets)} presets with DNA | {missing_dna} skipped (no DNA field)")

    if not presets:
        print(f"WARNING: No presets found for engine '{engine}'. Check engine name spelling.")

    analysis = analyze_presets(presets)
    gaps = find_gaps(analysis)

    existing_moods = list(analysis["mood_counts"].keys())
    mood_prescriptions = prescriptions_for_mood_gaps(gaps["missing_moods"], engine)
    zone_prescriptions = prescriptions_for_zone_gaps(gaps["empty_zones"], engine, existing_moods)

    report = render_report(engine, presets, analysis, gaps, mood_prescriptions, zone_prescriptions)

    if args.output:
        out_path = Path(args.output).expanduser()
        out_path.write_text(report, encoding="utf-8")
        print(f"Report written to {out_path}")
    else:
        print()
        print(report)


if __name__ == "__main__":
    main()
