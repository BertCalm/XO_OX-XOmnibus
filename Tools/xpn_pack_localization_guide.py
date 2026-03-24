"""
xpn_pack_localization_guide.py — XO_OX XOlokun Pack Localization Checker

Checks pack names, descriptions, and .xometa preset names for potential
localization issues: non-ASCII characters, false-friend words, display-length
limits, untranslatable jargon, and cultural sensitivity flags.

Usage:
    python xpn_pack_localization_guide.py --name "Pack Name" --description "..."
    python xpn_pack_localization_guide.py --name "Pack Name" --preset-dir ./Presets
    python xpn_pack_localization_guide.py --preset-dir ./Presets --output guide.txt
"""

import argparse
import json
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Reference data
# ---------------------------------------------------------------------------

# Words that mean something problematic in another language.
# Format: word (lowercase) -> list of (language, meaning) tuples
FALSE_FRIENDS: dict[str, list[tuple[str, str]]] = {
    "gift":     [("German", "poison"), ("Swedish/Norwegian/Danish", "poison/married")],
    "bald":     [("German", "soon / shortly")],
    "mist":     [("German", "manure/dung"), ("Dutch", "manure")],
    "angel":    [("German", "fishing rod")],
    "blank":    [("German", "shiny/polished"), ("French", "white")],
    "kind":     [("German", "child")],
    "fast":     [("German", "almost/nearly"), ("Swedish", "firm/fixed")],
    "chef":     [("German", "boss/head")],
    "slip":     [("German", "underpants/slip"), ("French", "briefs")],
    "fat":      [("Welsh", "pace/tempo")],
    "pain":     [("French", "bread")],
    "sale":     [("French", "dirty"), ("Italian", "salt")],
    "coin":     [("French", "corner")],
    "car":      [("Welsh", "love"), ("French informal", "because")],
    "pray":     [("French slang", "meadow/pasture — archaic overlap")],
    "nova":     [("Spanish/Portuguese", "doesn't go — as in car brand joke Nova")],
    "phat":     [("regional slang caution", "body-shaming connotation in some contexts")],
    "trap":     [("French", "trapdoor; also slang term with varying regional connotations")],
    "murk":     [("Scandinavian", "dark/murky — fine but may carry heavy connotation")],
    "odd":      [("Norwegian", "proper given name — not offensive, but may confuse")],
}

# Jargon acronyms that are safe vs. ones that may not translate.
SAFE_ACRONYMS = {"MPC", "BPM", "DAW", "MIDI", "XPN", "LFO", "VCA", "VCF", "OSC",
                 "VST", "AU", "AAX", "EQ", "FX", "LP", "HP", "BP", "XO", "OX"}

# Terms flagged for cultural sensitivity in public-facing product names.
# Kept minimal and factual.
SENSITIVE_TERMS: set[str] = {
    "voodoo", "savage", "tribal", "primitive", "exotic", "native",
    "gypsy", "oriental", "ghetto", "thug", "gangsta",
}

# MPC display constraints
PACK_NAME_MAX = 20
PROGRAM_NAME_MAX = 30
DESCRIPTION_WORD_SOFT_CAP = 80  # words — beyond this, MPC browser truncates

# ---------------------------------------------------------------------------
# Analysis helpers
# ---------------------------------------------------------------------------

def check_ascii(text: str) -> list[str]:
    """Return list of non-ASCII characters found."""
    return [ch for ch in text if ord(ch) > 127]


def check_false_friends(text: str) -> list[dict]:
    """Find false-friend words in text. Returns list of finding dicts."""
    findings = []
    words = re.findall(r"[a-zA-Z]+", text)
    seen = set()
    for word in words:
        lower = word.lower()
        if lower in FALSE_FRIENDS and lower not in seen:
            seen.add(lower)
            findings.append({
                "word": word,
                "issues": FALSE_FRIENDS[lower],
            })
    return findings


def check_jargon_acronyms(text: str) -> list[str]:
    """Find ALL-CAPS tokens that are not in the safe acronym list."""
    tokens = re.findall(r"\b[A-Z]{2,}\b", text)
    return [t for t in tokens if t not in SAFE_ACRONYMS]


def check_sensitive_terms(text: str) -> list[str]:
    """Return sensitive terms found in text."""
    words = re.findall(r"[a-zA-Z]+", text.lower())
    return [w for w in words if w in SENSITIVE_TERMS]


def check_length(name: str, max_chars: int) -> bool:
    """Return True if name exceeds max_chars."""
    return len(name) > max_chars


def description_word_count(description: str) -> int:
    return len(description.split())


# ---------------------------------------------------------------------------
# Scoring
# ---------------------------------------------------------------------------

def compute_score(issues: dict) -> int:
    """
    Start at 100. Deduct per finding category.
    Returns 0-100.
    """
    score = 100
    score -= len(issues.get("non_ascii", [])) * 15
    score -= len(issues.get("false_friends", [])) * 10
    score -= len(issues.get("jargon_acronyms", [])) * 5
    score -= len(issues.get("sensitive_terms", [])) * 20
    if issues.get("name_too_long"):
        score -= 10
    if issues.get("description_too_long"):
        score -= 5
    return max(0, score)


# ---------------------------------------------------------------------------
# Suggestion helpers
# ---------------------------------------------------------------------------

def suggest_alternatives(word: str) -> list[str]:
    """Very lightweight suggestion map for common flagged words."""
    suggestions: dict[str, list[str]] = {
        "gift":   ["Offering", "Tribute", "Blessing", "Present"],
        "bald":   ["Bare", "Open", "Clear", "Raw"],
        "mist":   ["Haze", "Fog", "Veil", "Cloud"],
        "angel":  ["Spirit", "Celestial", "Radiant"],
        "blank":  ["Void", "Empty", "Clean"],
        "kind":   ["Type", "Form", "Style"],
        "fast":   ["Swift", "Rush", "Rapid"],
        "fat":    ["Rich", "Deep", "Dense", "Thick"],
        "pain":   ["Ache", "Edge", "Tension"],
        "sale":   ["Release", "Drop", "Launch"],
        "coin":   ["Token", "Chip", "Mark"],
        "trap":   ["Snare", "Catch", "Chamber"],
        "nova":   ["Burst", "Flare", "Surge"],
        "phat":   ["Full", "Wide", "Warm"],
        "voodoo": ["Ritual", "Ceremony", "Rite"],
        "savage": ["Wild", "Fierce", "Untamed"],
        "tribal": ["Communal", "Circle", "Ensemble"],
        "gypsy":  ["Nomad", "Wanderer", "Traveler"],
        "oriental": ["Eastern", "Far East", "Asian"],
        "exotic": ["Rare", "Foreign", "Distant"],
        "native": ["Local", "Regional", "Root"],
        "ghetto": ["Underground", "Street", "Urban"],
    }
    return suggestions.get(word.lower(), ["(no suggestion — rename manually)"])


# ---------------------------------------------------------------------------
# Core analysis
# ---------------------------------------------------------------------------

def analyze_name(name: str, context: str = "pack", max_chars: int = PACK_NAME_MAX) -> dict:
    issues: dict = {}
    issues["non_ascii"] = check_ascii(name)
    issues["false_friends"] = check_false_friends(name)
    issues["jargon_acronyms"] = check_jargon_acronyms(name)
    issues["sensitive_terms"] = check_sensitive_terms(name)
    issues["name_too_long"] = check_length(name, max_chars)
    issues["description_too_long"] = False
    issues["score"] = compute_score(issues)
    issues["suggestions"] = {}
    for ff in issues["false_friends"]:
        issues["suggestions"][ff["word"]] = suggest_alternatives(ff["word"])
    for st in issues["sensitive_terms"]:
        issues["suggestions"][st] = suggest_alternatives(st)
    return issues


def analyze_description(description: str) -> dict:
    issues: dict = {}
    issues["non_ascii"] = check_ascii(description)
    issues["false_friends"] = check_false_friends(description)
    issues["jargon_acronyms"] = check_jargon_acronyms(description)
    issues["sensitive_terms"] = check_sensitive_terms(description)
    issues["name_too_long"] = False
    issues["description_too_long"] = description_word_count(description) > DESCRIPTION_WORD_SOFT_CAP
    issues["score"] = compute_score(issues)
    issues["suggestions"] = {}
    for ff in issues["false_friends"]:
        issues["suggestions"][ff["word"]] = suggest_alternatives(ff["word"])
    for st in issues["sensitive_terms"]:
        issues["suggestions"][st] = suggest_alternatives(st)
    return issues


# ---------------------------------------------------------------------------
# Report formatting
# ---------------------------------------------------------------------------

def render_issues(issues: dict, label: str) -> list[str]:
    lines = []
    lines.append(f"  [{label}]  Localization Score: {issues['score']}/100")

    if issues.get("non_ascii"):
        chars = ", ".join(repr(c) for c in issues["non_ascii"])
        lines.append(f"  ⚠  Non-ASCII characters: {chars}")
        lines.append("     → May cause filesystem errors on some MPC firmware versions.")

    if issues.get("name_too_long"):
        lines.append(f"  ⚠  Name length exceeds display limit.")
        lines.append(f"     → MPC truncates names beyond {PACK_NAME_MAX} chars for packs "
                     f"/ {PROGRAM_NAME_MAX} chars for programs.")

    if issues.get("description_too_long"):
        lines.append(f"  ⚠  Description exceeds {DESCRIPTION_WORD_SOFT_CAP}-word soft cap.")
        lines.append("     → MPC browser may truncate long descriptions.")

    for ff in issues.get("false_friends", []):
        word = ff["word"]
        for lang, meaning in ff["issues"]:
            lines.append(f"  ⚠  False friend: \"{word}\" means \"{meaning}\" in {lang}.")
        alts = issues["suggestions"].get(word, [])
        lines.append(f"     → Alternatives: {', '.join(alts)}")

    for acronym in issues.get("jargon_acronyms", []):
        lines.append(f"  ℹ  Untranslated acronym: \"{acronym}\" — verify it's widely understood.")

    for term in issues.get("sensitive_terms", []):
        alts = issues["suggestions"].get(term, [])
        lines.append(f"  ✗  Sensitive term: \"{term}\"")
        lines.append(f"     → Alternatives: {', '.join(alts)}")

    if issues["score"] == 100:
        lines.append("  ✓  No localization issues detected.")

    return lines


def build_report(pack_name, description,
                 preset_results: list) -> list:
    lines = ["=" * 64, "XO_OX XPN Pack Localization Guide", "=" * 64, ""]

    if pack_name:
        issues = analyze_name(pack_name, context="pack", max_chars=PACK_NAME_MAX)
        lines.append(f"Pack Name: \"{pack_name}\"")
        lines.extend(render_issues(issues, "PACK NAME"))
        lines.append("")

    if description:
        issues = analyze_description(description)
        lines.append(f"Description: \"{description[:60]}{'...' if len(description) > 60 else ''}\"")
        lines.extend(render_issues(issues, "DESCRIPTION"))
        lines.append("")

    if preset_results:
        lines.append(f"Preset Names ({len(preset_results)} checked):")
        lines.append("-" * 48)
        flagged = 0
        for preset_name, issues in preset_results:
            if issues["score"] < 100:
                flagged += 1
                lines.append(f"  Preset: \"{preset_name}\"")
                lines.extend(render_issues(issues, "PRESET"))
                lines.append("")
        if flagged == 0:
            lines.append("  ✓  All preset names passed localization checks.")
        else:
            lines.append(f"  {flagged}/{len(preset_results)} presets have localization flags.")
        lines.append("")

    lines.append("=" * 64)
    lines.append("Reference: MPC pack name limit = 20 chars | program name limit = 30 chars")
    lines.append("Score 90-100: Ship ready | 70-89: Review suggested | <70: Rename recommended")
    lines.append("=" * 64)
    return lines


# ---------------------------------------------------------------------------
# .xometa loading
# ---------------------------------------------------------------------------

def load_preset_names_from_dir(preset_dir: Path) -> list[str]:
    """Read all .xometa files in a directory and extract preset name fields."""
    names = []
    for meta_file in sorted(preset_dir.glob("**/*.xometa")):
        try:
            data = json.loads(meta_file.read_text(encoding="utf-8", errors="replace"))
            name = data.get("name") or data.get("presetName") or meta_file.stem
            names.append(str(name))
        except (json.JSONDecodeError, OSError):
            names.append(meta_file.stem)
    return names


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="XO_OX XPN Pack Localization Checker",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--name", metavar="PACK_NAME",
                        help="Pack name to check")
    parser.add_argument("--description", metavar="TEXT",
                        help="Pack description to check")
    parser.add_argument("--preset-dir", metavar="DIR",
                        help="Directory of .xometa files to scan")
    parser.add_argument("--output", metavar="FILE",
                        help="Write report to this file instead of stdout")
    args = parser.parse_args()

    if not args.name and not args.description and not args.preset_dir:
        parser.print_help()
        sys.exit(0)

    preset_results: list[tuple[str, dict]] = []
    if args.preset_dir:
        preset_dir = Path(args.preset_dir)
        if not preset_dir.is_dir():
            print(f"ERROR: --preset-dir '{preset_dir}' is not a directory.", file=sys.stderr)
            sys.exit(1)
        names = load_preset_names_from_dir(preset_dir)
        for name in names:
            preset_results.append(
                (name, analyze_name(name, context="preset", max_chars=PROGRAM_NAME_MAX))
            )

    report_lines = build_report(
        pack_name=args.name,
        description=args.description,
        preset_results=preset_results,
    )
    report_text = "\n".join(report_lines) + "\n"

    if args.output:
        out_path = Path(args.output)
        out_path.write_text(report_text, encoding="utf-8")
        print(f"Report saved to: {out_path}")
    else:
        print(report_text)


if __name__ == "__main__":
    main()
