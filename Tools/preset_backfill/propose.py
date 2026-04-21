#!/usr/bin/env python3
"""
propose.py — scans every .xometa and proposes (category, timbre)
for each, with a confidence score.

Writes: Docs/fleet-audit/instrument-taxonomy-proposal.csv

Reviewer workflow:
    1. Run this script.
    2. Open the CSV in a spreadsheet.
    3. Spot-check low-confidence rows (confidence < 0.7).
    4. Edit the `approved_category` / `approved_timbre` columns as needed.
    5. Run apply.py against the approved CSV.

Heuristics (first match wins, confidence decreases down the list):
    - Explicit category-synonym tag (e.g. "pad","bass","lead") → 0.95
    - Name contains category keyword → 0.80
    - Description contains category keyword → 0.70
    - Engine-level default (e.g. Offering → drums) → 0.60
    - Fallback: textures → 0.20

Note: some presets may have a pre-existing category like "lesson" that
is NOT in our taxonomy. These are overridable — the proposer ignores
the existing category and proposes its own. The reviewer sees both the
proposal and (if they look) the existing file, and decides.
"""

import csv
import json
import re
import sys
from pathlib import Path

# Ordered for first-match-wins. Each entry maps a KEYWORD to a CATEGORY.
CATEGORY_KEYWORDS = [
    ("bass", "bass"),
    ("sub", "bass"),
    ("kick", "drums"),
    ("snare", "drums"),
    ("hat", "drums"),
    ("drum", "drums"),
    ("tom", "drums"),
    ("clap", "drums"),
    ("perc", "perc"),
    ("bell", "perc"),
    ("chime", "perc"),
    ("pad", "pads"),
    ("lead", "leads"),
    ("arp", "sequence"),
    ("seq", "sequence"),
    ("piano", "keys"),
    ("key", "keys"),
    ("organ", "keys"),
    ("rhodes", "keys"),
    ("vox", "vocal"),
    ("choir", "vocal"),
    ("vocal", "vocal"),
    ("voice", "vocal"),
    ("fx", "fx"),
    ("riser", "fx"),
    ("impact", "fx"),
    ("hit", "fx"),
    ("texture", "textures"),
    ("atmo", "textures"),
]

TIMBRE_KEYWORDS = [
    ("strings", "strings"),
    ("violin", "strings"),
    ("cello", "strings"),
    ("brass", "brass"),
    ("trumpet", "brass"),
    ("horn", "brass"),
    ("tuba", "brass"),
    ("flute", "wind"),
    ("clarinet", "wind"),
    ("oboe", "wind"),
    ("reed", "wind"),
    ("choir", "choir"),
    ("chant", "choir"),
    ("organ", "organ"),
    ("pipe", "organ"),
    ("guitar", "plucked"),
    ("harp", "plucked"),
    ("koto", "plucked"),
    ("sitar", "world"),
    ("kora", "world"),
    ("duduk", "world"),
    ("shaku", "world"),
    ("bell", "metallic"),
    ("tine", "metallic"),
    ("gong", "metallic"),
]

# Engine-level category fallbacks (if nothing else matches).
# Map engine name → (category, timbre) default. Populate as knowledge grows.
ENGINE_DEFAULTS = {
    "Offering": ("drums", None),
    "Oware": ("perc", None),
    "Onkolo": ("perc", None),
    "Opera": ("pads", "choir"),
    "Ondine": ("pads", None),
    "Organon": ("pads", None),
    "Orrery": ("textures", None),
}

def scan_text(text: str, keyword_list):
    """Return (matched_keyword, mapped_value) or (None, None)."""
    if not text:
        return None, None
    lower = text.lower()
    for keyword, value in keyword_list:
        if re.search(rf"\b{re.escape(keyword)}\b", lower):
            return keyword, value
    return None, None

def propose_row(path: Path, data: dict):
    name = data.get("name", "")
    desc = data.get("description", "")
    tags = data.get("tags", []) or []
    engines = data.get("engines", []) or []
    primary_engine = engines[0] if engines else ""

    # Category pipeline
    cat_source, cat_keyword, category, confidence = None, None, None, 0.0

    for tag in tags:
        kw, mapped = scan_text(tag, CATEGORY_KEYWORDS)
        if kw:
            cat_source = "tag"; cat_keyword = kw; category = mapped; confidence = 0.95
            break

    if not category:
        kw, mapped = scan_text(name, CATEGORY_KEYWORDS)
        if kw:
            cat_source = "name"; cat_keyword = kw; category = mapped; confidence = 0.80

    if not category:
        kw, mapped = scan_text(desc, CATEGORY_KEYWORDS)
        if kw:
            cat_source = "desc"; cat_keyword = kw; category = mapped; confidence = 0.70

    if not category and primary_engine in ENGINE_DEFAULTS:
        default_cat, default_tim = ENGINE_DEFAULTS[primary_engine]
        if default_cat:
            cat_source = "engine"; cat_keyword = primary_engine; category = default_cat; confidence = 0.60

    if not category:
        cat_source = "fallback"; cat_keyword = "-"; category = "textures"; confidence = 0.20

    # Timbre pipeline (independent of category)
    tim_source, tim_keyword, timbre, tim_confidence = None, None, None, 0.0

    for tag in tags:
        kw, mapped = scan_text(tag, TIMBRE_KEYWORDS)
        if kw:
            tim_source = "tag"; tim_keyword = kw; timbre = mapped; tim_confidence = 0.90
            break

    if not timbre:
        kw, mapped = scan_text(name, TIMBRE_KEYWORDS)
        if kw:
            tim_source = "name"; tim_keyword = kw; timbre = mapped; tim_confidence = 0.75

    if not timbre:
        kw, mapped = scan_text(desc, TIMBRE_KEYWORDS)
        if kw:
            tim_source = "desc"; tim_keyword = kw; timbre = mapped; tim_confidence = 0.65

    if not timbre and primary_engine in ENGINE_DEFAULTS:
        _, default_tim = ENGINE_DEFAULTS[primary_engine]
        if default_tim:
            tim_source = "engine"; tim_keyword = primary_engine; timbre = default_tim; tim_confidence = 0.50

    # Otherwise timbre stays None — default for electronic presets

    return {
        "path": str(path),
        "name": name,
        "engine": primary_engine,
        "proposed_category": category,
        "category_source": cat_source,
        "category_keyword": cat_keyword,
        "category_confidence": round(confidence, 2),
        "approved_category": category,
        "proposed_timbre": timbre or "",
        "timbre_source": tim_source or "",
        "timbre_keyword": tim_keyword or "",
        "timbre_confidence": round(tim_confidence, 2),
        "approved_timbre": timbre or "",
        "reviewer_notes": "",
    }

def main():
    repo_root = Path(__file__).resolve().parent.parent.parent
    preset_root = Path(sys.argv[1]) if len(sys.argv) > 1 else repo_root
    out_path = repo_root / "Docs" / "fleet-audit" / "instrument-taxonomy-proposal.csv"
    out_path.parent.mkdir(parents=True, exist_ok=True)

    rows = []
    total = 0
    failed = 0
    skipped_fixtures = 0
    for path in preset_root.rglob("*.xometa"):
        # Skip test fixtures
        if "Tests/PresetTests/fixtures" in str(path):
            skipped_fixtures += 1
            continue
        total += 1
        try:
            with path.open("r", encoding="utf-8") as f:
                data = json.load(f)
        except (json.JSONDecodeError, UnicodeDecodeError) as e:
            failed += 1
            print(f"WARN: failed to parse {path}: {e}", file=sys.stderr)
            continue
        rows.append(propose_row(path, data))

    columns = [
        "path", "name", "engine",
        "proposed_category", "category_source", "category_keyword", "category_confidence",
        "approved_category",
        "proposed_timbre", "timbre_source", "timbre_keyword", "timbre_confidence",
        "approved_timbre",
        "reviewer_notes",
    ]
    with out_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=columns)
        writer.writeheader()
        writer.writerows(rows)

    print(f"Scanned {total} presets ({failed} failed to parse, {skipped_fixtures} fixtures skipped).")
    print(f"Proposal written to: {out_path}")
    print(f"Next step: review low-confidence rows, edit 'approved_*' columns, then run apply.py.")

    # Quick summary of confidence distribution
    buckets = {"high (>=0.9)": 0, "med (0.6-0.89)": 0, "low (<0.6)": 0}
    for row in rows:
        c = row["category_confidence"]
        if c >= 0.9: buckets["high (>=0.9)"] += 1
        elif c >= 0.6: buckets["med (0.6-0.89)"] += 1
        else: buckets["low (<0.6)"] += 1
    print("\nCategory-confidence distribution:")
    for k, v in buckets.items():
        pct = (v * 100.0 / len(rows)) if rows else 0
        print(f"  {k:20s} {v:6d} ({pct:.1f}%)")

if __name__ == "__main__":
    main()
