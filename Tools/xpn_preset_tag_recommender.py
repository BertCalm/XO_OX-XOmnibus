#!/usr/bin/env python3
"""
XOlokun Preset Tag Recommender

Recommends tags for .xometa presets based on Sonic DNA, mood, and engine —
ensuring consistent, meaningful tagging across the fleet.

Usage:
    python xpn_preset_tag_recommender.py <preset_or_dir> [--fix] [--batch] [--output report.txt]
"""

import argparse
import json
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Tag vocabulary (canonical — no free-form tags outside these sets)
# ---------------------------------------------------------------------------

VOCAB = {
    "character": {"bright", "dark", "warm", "cold", "organic", "metallic", "glassy", "wooden", "silky", "gritty"},
    "movement":  {"static", "pulsing", "evolving", "chaotic", "rhythmic", "breathing", "sweeping", "trembling"},
    "density":   {"sparse", "layered", "dense", "full", "minimal", "thick", "thin"},
    "space":     {"intimate", "roomy", "cavernous", "dry", "wet", "spatial", "close", "vast"},
    "genre":     {"ambient", "cinematic", "hip-hop", "electronic", "jazz", "classical", "world", "lo-fi", "trap", "indie"},
    "use_case":  {"lead", "pad", "bass", "texture", "percussion", "fx", "atmosphere", "drone", "chord", "melody"},
    "intensity": {"aggressive", "gentle", "intense", "soft", "punchy", "subtle", "powerful", "delicate"},
}

ALL_VALID = set().union(*VOCAB.values())

TAG_TARGET_MIN = 4
TAG_TARGET_MAX = 8

# ---------------------------------------------------------------------------
# Engine → genre + use case hints
# (covers all 34 registered engines + V1 concepts)
# ---------------------------------------------------------------------------

ENGINE_HINTS: dict[str, dict] = {
    # Dual-engine family
    "OddfeliX":   {"genre": ["electronic", "ambient"],          "use_case": ["lead", "texture"]},
    "OddOscar":   {"genre": ["electronic", "cinematic"],        "use_case": ["pad", "atmosphere"]},
    # Character synths
    "Overdub":    {"genre": ["hip-hop", "lo-fi", "electronic"], "use_case": ["pad", "atmosphere"]},
    "Odyssey":    {"genre": ["electronic", "cinematic"],        "use_case": ["lead", "pad"]},
    "Oblong":     {"genre": ["electronic", "indie"],            "use_case": ["lead", "chord"]},
    "Obese":      {"genre": ["hip-hop", "trap", "electronic"],  "use_case": ["bass", "lead"]},
    "Onset":      {"genre": ["hip-hop", "electronic", "trap"],  "use_case": ["percussion"]},
    "Overworld":  {"genre": ["electronic", "cinematic"],        "use_case": ["lead", "melody"]},
    "Opal":       {"genre": ["ambient", "cinematic"],           "use_case": ["texture", "atmosphere", "pad"]},
    "Orbital":    {"genre": ["electronic", "ambient"],          "use_case": ["pad", "melody"]},
    "Organon":    {"genre": ["ambient", "cinematic"],           "use_case": ["atmosphere", "drone", "texture"]},
    "Ouroboros":  {"genre": ["electronic", "cinematic"],        "use_case": ["texture", "atmosphere", "fx"]},
    "Obsidian":   {"genre": ["cinematic", "ambient"],           "use_case": ["pad", "atmosphere"]},
    "Origami":    {"genre": ["electronic", "indie"],            "use_case": ["lead", "melody"]},
    "Oracle":     {"genre": ["cinematic", "world", "ambient"],  "use_case": ["melody", "atmosphere", "drone"]},
    "Obscura":    {"genre": ["cinematic", "ambient"],           "use_case": ["texture", "atmosphere"]},
    "Oceanic":    {"genre": ["ambient", "cinematic"],           "use_case": ["atmosphere", "pad", "texture"]},
    "Ocelot":     {"genre": ["world", "ambient"],               "use_case": ["texture", "atmosphere"]},
    "Overbite":   {"genre": ["electronic", "hip-hop"],          "use_case": ["bass", "lead"]},
    "Optic":      {"genre": ["electronic", "ambient"],          "use_case": ["fx", "texture"]},
    "Oblique":    {"genre": ["electronic", "hip-hop"],          "use_case": ["lead", "chord"]},
    "Osprey":     {"genre": ["world", "ambient", "cinematic"],  "use_case": ["melody", "texture"]},
    "Osteria":    {"genre": ["jazz", "world", "cinematic"],     "use_case": ["melody", "chord"]},
    "Owlfish":    {"genre": ["cinematic", "ambient"],           "use_case": ["drone", "atmosphere", "texture"]},
    "Ohm":        {"genre": ["ambient", "world"],               "use_case": ["pad", "drone", "atmosphere"]},
    "Orphica":    {"genre": ["ambient", "cinematic"],           "use_case": ["texture", "atmosphere", "melody"]},
    "Obbligato":  {"genre": ["classical", "cinematic"],         "use_case": ["melody", "pad"]},
    "Ottoni":     {"genre": ["classical", "cinematic", "jazz"], "use_case": ["melody", "chord"]},
    "Ole":        {"genre": ["world", "jazz"],                  "use_case": ["melody", "chord", "percussion"]},
    "Ombre":      {"genre": ["ambient", "cinematic"],           "use_case": ["pad", "atmosphere", "texture"]},
    "Orca":       {"genre": ["cinematic", "electronic"],        "use_case": ["bass", "atmosphere"]},
    "Octopus":    {"genre": ["electronic", "cinematic"],        "use_case": ["texture", "fx", "atmosphere"]},
    # V1 concepts
    "Ostinato":   {"genre": ["world", "electronic"],            "use_case": ["percussion", "texture"]},
    "OpenSky":    {"genre": ["ambient", "cinematic"],           "use_case": ["pad", "atmosphere"]},
    "OceanDeep":  {"genre": ["ambient", "cinematic"],           "use_case": ["bass", "drone", "atmosphere"]},
    "Ouie":       {"genre": ["electronic", "cinematic"],        "use_case": ["lead", "melody"]},
}

# Mood → genre boost
MOOD_GENRE_HINTS: dict[str, list[str]] = {
    "Foundation":  [],
    "Atmosphere":  ["ambient", "cinematic"],
    "Entangled":   ["electronic", "cinematic"],
    "Prism":       ["electronic", "indie"],
    "Flux":        ["electronic", "ambient"],
    "Aether":      ["ambient", "cinematic", "classical"],
    "Family":      [],
}

# ---------------------------------------------------------------------------
# Tag derivation logic
# ---------------------------------------------------------------------------

def derive_tags(preset: dict) -> list[str]:
    """Return a sorted list of recommended vocabulary tags for a preset."""
    dna = preset.get("dna", {})
    mood = preset.get("mood", "")
    engines = preset.get("engines", [])

    brightness  = float(dna.get("brightness",  0.5))
    warmth      = float(dna.get("warmth",      0.5))
    movement    = float(dna.get("movement",    0.5))
    density     = float(dna.get("density",     0.5))
    space       = float(dna.get("space",       0.5))
    aggression  = float(dna.get("aggression",  0.5))

    tags: set[str] = set()

    # --- Character (brightness + warmth) ---
    if brightness >= 0.65:
        tags.add("bright")
    elif brightness <= 0.35:
        tags.add("dark")

    if warmth >= 0.65:
        tags.add("warm")
    elif warmth <= 0.35:
        tags.add("cold")

    # Texture character derived from combined DNA
    if warmth >= 0.6 and brightness <= 0.55:
        tags.add("wooden")
    elif warmth <= 0.4 and brightness >= 0.6:
        tags.add("glassy")
    elif warmth >= 0.55 and movement >= 0.5:
        tags.add("organic")
    elif warmth <= 0.4 and aggression >= 0.55:
        tags.add("metallic")

    if aggression <= 0.25 and brightness >= 0.55:
        tags.add("silky")
    elif aggression >= 0.65:
        tags.add("gritty")

    # --- Movement ---
    if movement >= 0.7:
        if aggression >= 0.6:
            tags.add("chaotic")
        else:
            tags.add("evolving")
    elif movement >= 0.5:
        if density >= 0.55:
            tags.add("pulsing")
        else:
            tags.add("breathing")
    elif movement >= 0.35:
        tags.add("sweeping")
    elif movement <= 0.2:
        tags.add("static")

    if movement >= 0.45 and aggression <= 0.4:
        tags.add("trembling")

    # --- Density ---
    if density >= 0.7:
        if movement >= 0.5:
            tags.add("dense")
        else:
            tags.add("thick")
    elif density >= 0.5:
        tags.add("layered")
    elif density <= 0.3:
        if movement <= 0.3:
            tags.add("sparse")
        else:
            tags.add("minimal")
    else:
        tags.add("thin")

    # Avoid thin + dense simultaneously (pick dominant)
    if "thin" in tags and "dense" in tags:
        tags.discard("thin")
    if "thin" in tags and "thick" in tags:
        tags.discard("thin")

    # --- Space ---
    if space >= 0.7:
        if density >= 0.5:
            tags.add("cavernous")
        else:
            tags.add("vast")
    elif space >= 0.5:
        tags.add("roomy")
        tags.add("wet")
    elif space <= 0.25:
        tags.add("dry")
        tags.add("close")
    else:
        tags.add("spatial")
        tags.add("intimate")

    # --- Intensity ---
    if aggression >= 0.7:
        tags.add("aggressive")
        tags.add("intense")
    elif aggression >= 0.5:
        tags.add("powerful")
        tags.add("punchy")
    elif aggression <= 0.2:
        tags.add("gentle")
        tags.add("delicate")
        tags.add("soft")
    elif aggression <= 0.35:
        tags.add("subtle")

    # --- Genre + use case: engine hints first ---
    genre_pool: list[str] = []
    use_pool: list[str] = []

    for eng in engines:
        hints = ENGINE_HINTS.get(eng, {})
        genre_pool.extend(hints.get("genre", []))
        use_pool.extend(hints.get("use_case", []))

    # Mood boost
    genre_pool.extend(MOOD_GENRE_HINTS.get(mood, []))

    # Deduplicate while preserving vote-order priority
    seen_genre: set[str] = set()
    for g in genre_pool:
        if g not in seen_genre:
            seen_genre.add(g)
            tags.add(g)
            if len(seen_genre) >= 2:
                break

    seen_use: set[str] = set()
    for u in use_pool:
        if u not in seen_use:
            seen_use.add(u)
            tags.add(u)
            if len(seen_use) >= 2:
                break

    # Ensure result stays within vocabulary
    tags &= ALL_VALID

    # Trim to max 8 by priority: keep intensity + character first, then trim movement/space extras
    tag_list = sorted(tags)
    if len(tag_list) > TAG_TARGET_MAX:
        priority = (
            [t for t in tag_list if t in VOCAB["use_case"]] +
            [t for t in tag_list if t in VOCAB["character"]] +
            [t for t in tag_list if t in VOCAB["intensity"]] +
            [t for t in tag_list if t in VOCAB["movement"]] +
            [t for t in tag_list if t in VOCAB["density"]] +
            [t for t in tag_list if t in VOCAB["genre"]] +
            [t for t in tag_list if t in VOCAB["space"]]
        )
        seen: set[str] = set()
        deduped = [t for t in priority if not (t in seen or seen.add(t))]  # type: ignore[func-returns-value]
        tag_list = deduped[:TAG_TARGET_MAX]

    return sorted(tag_list)


# ---------------------------------------------------------------------------
# Analysis helpers
# ---------------------------------------------------------------------------

def analyse_preset(path: Path) -> dict:
    """Load a .xometa file and return a comparison report dict."""
    try:
        preset = json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        return {"path": str(path), "error": str(exc)}

    current_raw = preset.get("tags", [])
    current = [t for t in current_raw if isinstance(t, str)]
    valid_current = [t for t in current if t in ALL_VALID]
    invalid_current = [t for t in current if t not in ALL_VALID]

    suggested = derive_tags(preset)

    current_set = set(valid_current)
    suggested_set = set(suggested)

    missing = sorted(suggested_set - current_set)
    extra   = sorted(current_set - suggested_set)  # valid but not suggested

    status = "ok"
    if not current:
        status = "empty"
    elif len(current) < TAG_TARGET_MIN:
        status = "thin"
    elif len(current) > TAG_TARGET_MAX:
        status = "bloated"
    elif missing:
        status = "missing"

    return {
        "path":            str(path),
        "name":            preset.get("name", path.stem),
        "engine":          ", ".join(preset.get("engines", [])),
        "mood":            preset.get("mood", ""),
        "current":         current,
        "invalid":         invalid_current,
        "suggested":       suggested,
        "missing":         missing,
        "extra":           extra,
        "status":          status,
        "preset":          preset,
    }


def apply_fix(report: dict) -> bool:
    """Write suggested tags back to the .xometa file. Returns True on success."""
    path = Path(report["path"])
    preset = report["preset"]

    # Preserve valid existing tags + fill gaps with suggestions
    current_valid = set(t for t in preset.get("tags", []) if t in ALL_VALID)
    merged = sorted(current_valid | set(report["suggested"]))
    if len(merged) > TAG_TARGET_MAX:
        # Re-run derive to get priority-ordered list, then fill remaining from valid current
        priority = report["suggested"]
        extras = sorted(current_valid - set(priority))
        merged = (priority + extras)[:TAG_TARGET_MAX]

    preset["tags"] = merged
    try:
        path.write_text(json.dumps(preset, indent=2) + "\n", encoding="utf-8")
        return True
    except Exception as exc:
        print(f"  ERROR writing {path}: {exc}", file=sys.stderr)
        return False


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def format_report(reports: list[dict], verbose: bool = False) -> str:
    lines: list[str] = []
    total = len(reports)
    errors = [r for r in reports if "error" in r]
    ok     = [r for r in reports if r.get("status") == "ok"]
    empty  = [r for r in reports if r.get("status") == "empty"]
    thin   = [r for r in reports if r.get("status") == "thin"]
    bloated= [r for r in reports if r.get("status") == "bloated"]
    missing= [r for r in reports if r.get("status") == "missing"]

    lines.append("=" * 70)
    lines.append("XOlokun Preset Tag Recommender — Report")
    lines.append("=" * 70)
    lines.append(f"Presets scanned : {total}")
    lines.append(f"  Errors        : {len(errors)}")
    lines.append(f"  OK            : {len(ok)}")
    lines.append(f"  Empty tags    : {len(empty)}")
    lines.append(f"  Thin (<{TAG_TARGET_MIN} tags): {len(thin)}")
    lines.append(f"  Bloated (>{TAG_TARGET_MAX}): {len(bloated)}")
    lines.append(f"  Missing tags  : {len(missing)}")
    lines.append("")

    needs_attention = empty + thin + bloated + missing + errors
    if not needs_attention:
        lines.append("All presets have healthy tag coverage.")
        return "\n".join(lines)

    for r in needs_attention:
        if "error" in r:
            lines.append(f"[ERROR] {r['path']}")
            lines.append(f"        {r['error']}")
            lines.append("")
            continue

        lines.append(f"[{r['status'].upper():8}] {r['name']}")
        lines.append(f"  Engine : {r['engine']}  |  Mood: {r['mood']}")
        lines.append(f"  Current  ({len(r['current']):2}): {r['current']}")
        if r["invalid"]:
            lines.append(f"  Invalid      : {r['invalid']}  <-- not in vocabulary")
        lines.append(f"  Suggested({len(r['suggested']):2}): {r['suggested']}")
        if r["missing"]:
            lines.append(f"  Missing      : {r['missing']}")
        if r["extra"]:
            lines.append(f"  Extra (kept) : {r['extra']}")
        lines.append("")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def collect_presets(target: Path, batch: bool) -> list[Path]:
    if target.is_file():
        return [target] if target.suffix == ".xometa" else []
    if batch or target.is_dir():
        return sorted(target.rglob("*.xometa"))
    return []


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Recommend or apply tags for .xometa presets based on Sonic DNA."
    )
    parser.add_argument("target", help="Path to a .xometa file or directory")
    parser.add_argument("--fix",    action="store_true", help="Apply suggested tags (preserves valid existing tags)")
    parser.add_argument("--batch",  action="store_true", help="Recursively process a directory")
    parser.add_argument("--output", metavar="FILE",      help="Write report to file instead of stdout")
    args = parser.parse_args()

    target = Path(args.target)
    if not target.exists():
        print(f"ERROR: path not found: {target}", file=sys.stderr)
        sys.exit(1)

    preset_paths = collect_presets(target, args.batch)
    if not preset_paths:
        print("No .xometa files found.", file=sys.stderr)
        sys.exit(1)

    reports = [analyse_preset(p) for p in preset_paths]

    if args.fix:
        fixed = 0
        skipped = 0
        for r in reports:
            if "error" in r:
                skipped += 1
                continue
            if r["status"] == "ok":
                skipped += 1
                continue
            if apply_fix(r):
                fixed += 1
            else:
                skipped += 1
        print(f"Fixed {fixed} preset(s). Skipped {skipped}.")

    report_text = format_report(reports)

    if args.output:
        out_path = Path(args.output)
        out_path.write_text(report_text + "\n", encoding="utf-8")
        print(f"Report written to {out_path}")
    else:
        print(report_text)


if __name__ == "__main__":
    main()
