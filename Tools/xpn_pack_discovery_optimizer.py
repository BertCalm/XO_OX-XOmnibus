#!/usr/bin/env python3
"""
xpn_pack_discovery_optimizer.py — XO_OX Designs
Audits and improves MPC expansion pack metadata for marketplace discoverability.

Reads expansion.json, scores 8 discoverability factors, suggests fixes,
and optionally auto-applies safe corrections.

Usage:
    python xpn_pack_discovery_optimizer.py <pack_dir> [--fix] [--output audit.txt]

Exit codes:
    0  — score >= 80 (good)
    1  — score 50-79 (needs work)
    2  — score < 50 (poor discoverability)
"""

import argparse
import json
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# CONSTANTS
# ---------------------------------------------------------------------------

SEMVER_RE = re.compile(r"^\d+\.\d+\.\d+$")

GENRE_TAXONOMY = {
    "hip-hop", "hip hop", "trap", "lo-fi", "lofi", "lo fi",
    "r&b", "rnb", "soul", "funk", "jazz", "blues",
    "electronic", "edm", "house", "techno", "drum and bass", "dnb",
    "ambient", "cinematic", "orchestral", "classical",
    "pop", "rock", "indie", "alternative",
    "afrobeat", "latin", "reggae", "world",
    "experimental", "industrial", "noise",
    "gospel", "neo-soul", "boom bap",
}

INSTRUMENT_TAGS = {
    "synth", "synthesizer", "bass", "drums", "drum machine", "percussion",
    "keys", "piano", "organ", "strings", "pads", "leads", "plucks",
    "brass", "woodwind", "guitar", "harp", "bells", "marimba",
    "808", "909", "rhodes", "moog", "fm", "wavetable", "granular",
    "sampler", "acoustic", "electric",
}

MOOD_TAGS = {
    "dark", "bright", "warm", "cold", "ethereal", "gritty", "smooth",
    "aggressive", "mellow", "dreamy", "energetic", "melancholic",
    "uplifting", "tense", "mysterious", "hypnotic", "raw", "lush",
    "minimal", "dense", "sparse", "evolving", "static", "emotional",
    "cinematic", "atmospheric", "punchy", "airy", "heavy",
}

SEARCH_KEYWORDS = {
    "mpc", "expansion", "samples", "kit", "preset", "program",
    "production", "producer", "beat", "beatmaking",
}

GENERIC_AUTHORS = {"user", "artist", "unknown", "default", "admin", "author", "none", "n/a", ""}

XO_OX_ENGINES = {
    "OVERDUB", "ODYSSEY", "OBLONG", "OBESE", "ONSET", "OVERWORLD",
    "OPAL", "ORBITAL", "ORGANON", "OUROBOROS", "OBSIDIAN", "OVERBITE",
    "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC", "OCELOT", "OPTIC",
    "OBLIQUE", "OSPREY", "OHM", "ORPHICA", "OBBLIGATO", "OTTONI",
    "OLE", "OVERLAP", "OUTWIT", "OSTINATO", "OPENSKY", "OCEANDEEP", "OUIE",
}

DISTINCTIVE_TECHNIQUES = {
    "granular", "wavetable", "fm synthesis", "cellular automata", "convolution",
    "spectral", "physical modeling", "additive", "subtractive", "karplus",
    "tape saturation", "ring modulation", "frequency shifting", "formant",
    "topology", "knot", "chaos", "fractal", "evolutionary",
}

# ---------------------------------------------------------------------------
# FACTOR WEIGHTS (must sum to 1.0)
# ---------------------------------------------------------------------------

FACTOR_WEIGHTS = {
    "name_length":       0.10,
    "description":       0.25,
    "tags":              0.20,
    "version_format":    0.05,
    "author_field":      0.08,
    "genre_field":       0.10,
    "unique_selling":    0.12,
    "search_keywords":   0.10,
}

# ---------------------------------------------------------------------------
# AUDIT ENGINE
# ---------------------------------------------------------------------------

def audit_name_length(data: dict) -> tuple[int, list[str]]:
    """Factor 1: Name should be 2-4 words for marketplace display."""
    name = data.get("name", "").strip()
    if not name:
        return 0, ["No 'name' field found — add a concise 2-4 word pack name."]
    words = name.split()
    count = len(words)
    issues = []
    if count < 2:
        score = 40
        issues.append(f"Name '{name}' is only 1 word — 2-4 words display better on marketplace.")
    elif count <= 4:
        score = 100
    elif count <= 6:
        score = 60
        issues.append(f"Name '{name}' is {count} words — trim to 2-4 words for cleaner display.")
    else:
        score = 20
        issues.append(f"Name '{name}' is {count} words — far too long; shorten to 2-4 words.")
    return score, issues


def audit_description(data: dict) -> tuple[int, list[str]]:
    """Factor 2: Description ≥50 words, mentions instrument + genre, no excessive repetition."""
    desc = data.get("description", "").strip()
    issues = []
    score = 0

    if not desc:
        return 0, ["No 'description' field — add a rich description (50+ words) to improve discovery."]

    words = desc.lower().split()
    word_count = len(words)
    points = 0

    # Length check (40 pts)
    if word_count >= 50:
        points += 40
    elif word_count >= 30:
        points += 20
        issues.append(f"Description is {word_count} words — aim for 50+ to rank better in searches.")
    else:
        points += 5
        issues.append(f"Description is only {word_count} words — needs at least 50 for good discoverability.")

    # Instrument mention (25 pts)
    desc_lower = desc.lower()
    has_instrument = any(inst in desc_lower for inst in INSTRUMENT_TAGS)
    if has_instrument:
        points += 25
    else:
        issues.append("Description doesn't mention instrument type (e.g., 'synth', 'bass', 'drums') — producers filter by this.")

    # Genre/use-case mention (25 pts)
    has_genre = any(g in desc_lower for g in GENRE_TAXONOMY)
    if has_genre:
        points += 25
    else:
        issues.append("Description doesn't mention genre or use-case (e.g., 'hip-hop', 'cinematic') — add at least one.")

    # Repetition check (10 pts)
    if word_count > 10:
        from collections import Counter
        freq = Counter(w for w in words if len(w) > 4)
        most_common_count = freq.most_common(1)[0][1] if freq else 0
        repetition_ratio = most_common_count / word_count if word_count else 0
        if repetition_ratio < 0.07:
            points += 10
        else:
            issues.append("Description has excessive word repetition — vary your language for better SEO.")
    else:
        points += 10

    return min(100, points), issues


def audit_tags(data: dict) -> tuple[int, list[str]]:
    """Factor 3: 5-15 tags including at least 1 genre, 1 instrument, 1 mood tag."""
    tags_raw = data.get("tags", [])
    issues = []

    if not tags_raw:
        return 0, ["No 'tags' field — add 5-15 tags covering genre, instrument type, and mood."]

    tags = [str(t).lower().strip() for t in tags_raw]
    count = len(tags)
    points = 0

    # Count range (40 pts)
    if 5 <= count <= 15:
        points += 40
    elif count < 5:
        points += 15
        issues.append(f"Only {count} tag(s) — add more to reach 5-15 (genre, instrument, mood, technique, era).")
    else:
        points += 25
        issues.append(f"{count} tags is above the ideal 15 — excess tags can hurt relevance ranking.")

    # Genre tag (20 pts)
    has_genre_tag = any(g in tags or any(g in t for t in tags) for g in GENRE_TAXONOMY)
    if has_genre_tag:
        points += 20
    else:
        issues.append("No genre tag found — add one (e.g., 'hip-hop', 'ambient', 'electronic').")

    # Instrument tag (20 pts)
    has_instrument_tag = any(any(inst in t for inst in INSTRUMENT_TAGS) for t in tags)
    if has_instrument_tag:
        points += 20
    else:
        issues.append("No instrument type tag found — add one (e.g., 'synth', 'bass', 'pads').")

    # Mood tag (20 pts)
    has_mood_tag = any(any(mood in t for mood in MOOD_TAGS) for t in tags)
    if has_mood_tag:
        points += 20
    else:
        issues.append("No mood tag found — add one (e.g., 'dark', 'warm', 'ethereal', 'gritty').")

    return min(100, points), issues


def audit_version_format(data: dict) -> tuple[int, list[str]]:
    """Factor 4: Version should be semantic (X.Y.Z)."""
    version = str(data.get("version", "")).strip()
    if not version:
        return 0, ["No 'version' field — add a semantic version like '1.0.0'."]
    if SEMVER_RE.match(version):
        return 100, []
    # Check for partial semver
    if re.match(r"^\d+\.\d+$", version):
        return 60, [f"Version '{version}' is X.Y format — use X.Y.Z semantic versioning (e.g., '{version}.0'). Auto-fixable with --fix."]
    if re.match(r"^\d+$", version):
        return 40, [f"Version '{version}' is bare integer — use X.Y.Z format (e.g., '{version}.0.0'). Auto-fixable with --fix."]
    return 20, [f"Version '{version}' is non-standard — use semantic versioning X.Y.Z (e.g., '1.0.0'). Auto-fixable with --fix."]


def audit_author_field(data: dict) -> tuple[int, list[str]]:
    """Factor 5: Author must be present and non-generic."""
    author = str(data.get("author", "")).strip()
    if not author or author.lower() in GENERIC_AUTHORS:
        return 0, [f"Author field is missing or generic ('{author}') — set to 'XO_OX' or your brand name. Auto-fixable with --fix."]
    if len(author) < 2:
        return 30, ["Author name is too short — use your full brand/artist name."]
    return 100, []


def audit_genre_field(data: dict) -> tuple[int, list[str]]:
    """Factor 6: Genre field present and matches taxonomy."""
    genre = str(data.get("genre", "")).strip().lower()
    if not genre:
        return 0, ["No 'genre' field — add a primary genre (e.g., 'hip-hop', 'electronic', 'cinematic')."]
    # Exact or substring match in taxonomy
    if genre in GENRE_TAXONOMY or any(genre in g or g in genre for g in GENRE_TAXONOMY):
        return 100, []
    return 50, [f"Genre '{genre}' not in standard taxonomy — consider using: hip-hop, trap, lo-fi, r&b, electronic, ambient, cinematic, pop, jazz."]


def audit_unique_selling(data: dict) -> tuple[int, list[str]]:
    """Factor 7: Description must reference engine name, technique, or unique attribute."""
    desc = (data.get("description", "") + " " + data.get("name", "")).lower()
    issues = []

    # Check for XO_OX engine names
    has_engine = any(engine.lower() in desc for engine in XO_OX_ENGINES)
    if has_engine:
        return 100, []

    # Check for distinctive technique
    has_technique = any(tech in desc for tech in DISTINCTIVE_TECHNIQUES)
    if has_technique:
        return 85, []

    # Check for general unique descriptors
    unique_words = {"exclusive", "original", "custom", "signature", "unique", "proprietary",
                    "hand-crafted", "handcrafted", "designed", "crafted", "curated",
                    "bespoke", "boutique", "limited"}
    has_unique = any(w in desc for w in unique_words)
    if has_unique:
        return 60, ["Unique attribute mentioned but vague — name the specific engine (e.g., 'OPAL granular engine') or technique for stronger USP."]

    issues.append(
        "No unique selling point found — mention the engine name (e.g., 'OVERDUB', 'OPAL') "
        "or a distinctive technique (granular, FM, cellular automata) in the description."
    )
    return 0, issues


def audit_search_keywords(data: dict) -> tuple[int, list[str]]:
    """Factor 8: Description should contain words producers actually search for."""
    desc = data.get("description", "").lower()
    name = data.get("name", "").lower()
    combined = desc + " " + name

    found = [kw for kw in SEARCH_KEYWORDS if kw in combined]
    count = len(found)
    missing = SEARCH_KEYWORDS - set(found)

    if count >= 4:
        return 100, []
    elif count >= 2:
        score = 60
    elif count == 1:
        score = 30
    else:
        score = 0

    issues = [
        f"Only {count}/8 producer search keywords found ({', '.join(sorted(found)) or 'none'}). "
        f"Add some of: {', '.join(sorted(list(missing)[:5]))}."
    ]
    return score, issues


# ---------------------------------------------------------------------------
# AUTO-FIX
# ---------------------------------------------------------------------------

def apply_fixes(data: dict) -> tuple[dict, list[str]]:
    """Apply safe auto-fixes: version format and author field."""
    applied = []

    # Fix version
    version = str(data.get("version", "")).strip()
    if version and not SEMVER_RE.match(version):
        if re.match(r"^\d+\.\d+$", version):
            data["version"] = version + ".0"
            applied.append(f"version: '{version}' → '{data['version']}'")
        elif re.match(r"^\d+$", version):
            data["version"] = version + ".0.0"
            applied.append(f"version: '{version}' → '{data['version']}'")
        else:
            data["version"] = "1.0.0"
            applied.append(f"version: '{version}' → '1.0.0' (non-standard, reset to 1.0.0)")
    elif not version:
        data["version"] = "1.0.0"
        applied.append("version: (missing) → '1.0.0'")

    # Fix author
    author = str(data.get("author", "")).strip()
    if not author or author.lower() in GENERIC_AUTHORS:
        data["author"] = "XO_OX"
        applied.append(f"author: '{author}' → 'XO_OX'")

    return data, applied


# ---------------------------------------------------------------------------
# REPORT
# ---------------------------------------------------------------------------

FACTOR_LABELS = {
    "name_length":    "Name Length",
    "description":    "Description Quality",
    "tags":           "Tags",
    "version_format": "Version Format",
    "author_field":   "Author Field",
    "genre_field":    "Genre Field",
    "unique_selling": "Unique Selling Point",
    "search_keywords":"Search Keywords",
}

AUDITORS = {
    "name_length":    audit_name_length,
    "description":    audit_description,
    "tags":           audit_tags,
    "version_format": audit_version_format,
    "author_field":   audit_author_field,
    "genre_field":    audit_genre_field,
    "unique_selling": audit_unique_selling,
    "search_keywords":audit_search_keywords,
}


def run_audit(data: dict) -> dict:
    """Run all 8 auditors, return results dict."""
    results = {}
    for key, auditor in AUDITORS.items():
        score, issues = auditor(data)
        results[key] = {"score": score, "issues": issues, "weight": FACTOR_WEIGHTS[key]}
    return results


def compute_total_score(results: dict) -> float:
    total = sum(r["score"] * r["weight"] for r in results.values())
    return round(total, 1)


def grade(score: float) -> str:
    if score >= 90: return "A  (Excellent)"
    if score >= 80: return "B  (Good)"
    if score >= 65: return "C  (Needs Work)"
    if score >= 50: return "D  (Poor)"
    return "F  (Critical)"


def format_report(data: dict, results: dict, applied_fixes: list[str]) -> str:
    lines = []
    pack_name = data.get("name", "(unnamed)")
    total = compute_total_score(results)
    lines.append("=" * 62)
    lines.append(f"  XPN Pack Discovery Optimizer — XO_OX Designs")
    lines.append("=" * 62)
    lines.append(f"  Pack:  {pack_name}")
    lines.append(f"  Score: {total:.1f}/100  {grade(total)}")
    lines.append("=" * 62)
    lines.append("")

    for key, r in results.items():
        label = FACTOR_LABELS[key]
        score = r["score"]
        weight_pct = int(r["weight"] * 100)
        bar_len = score // 5
        bar = "█" * bar_len + "░" * (20 - bar_len)
        status = "PASS" if score >= 70 else ("WARN" if score >= 40 else "FAIL")
        lines.append(f"  [{status}] {label:<22} {bar} {score:>3}/100  (weight {weight_pct}%)")
        for issue in r["issues"]:
            lines.append(f"         → {issue}")
        lines.append("")

    if applied_fixes:
        lines.append("-" * 62)
        lines.append("  Auto-fixes applied:")
        for fix in applied_fixes:
            lines.append(f"    • {fix}")
        lines.append("")

    lines.append("-" * 62)
    total_issues = sum(len(r["issues"]) for r in results.values())
    lines.append(f"  Total issues: {total_issues}")
    lines.append(f"  Discoverability score: {total:.1f}/100  {grade(total)}")
    lines.append("=" * 62)
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# MAIN
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Audit and optimize MPC expansion pack metadata for discoverability."
    )
    parser.add_argument("pack_dir", help="Path to expansion pack directory (containing expansion.json)")
    parser.add_argument("--fix", action="store_true", help="Auto-fix safe issues (version format, author) in expansion.json")
    parser.add_argument("--output", metavar="FILE", help="Write audit report to file (default: stdout)")
    args = parser.parse_args()

    pack_path = Path(args.pack_dir)
    if not pack_path.exists():
        print(f"ERROR: Pack directory not found: {pack_path}", file=sys.stderr)
        sys.exit(2)

    expansion_json = pack_path / "expansion.json"
    if not expansion_json.exists():
        print(f"ERROR: No expansion.json found in: {pack_path}", file=sys.stderr)
        sys.exit(2)

    try:
        with open(expansion_json, "r", encoding="utf-8") as f:
            data = json.load(f)
    except json.JSONDecodeError as e:
        print(f"ERROR: expansion.json is not valid JSON: {e}", file=sys.stderr)
        sys.exit(2)

    applied_fixes = []
    if args.fix:
        data, applied_fixes = apply_fixes(data)
        with open(expansion_json, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
            f.write("\n")

    results = run_audit(data)
    report = format_report(data, results, applied_fixes)

    if args.output:
        out_path = Path(args.output)
        out_path.write_text(report, encoding="utf-8")
        print(f"Audit report written to: {out_path}")
    else:
        print(report)

    total = compute_total_score(results)
    if total >= 80:
        sys.exit(0)
    elif total >= 50:
        sys.exit(1)
    else:
        sys.exit(2)


if __name__ == "__main__":
    main()
