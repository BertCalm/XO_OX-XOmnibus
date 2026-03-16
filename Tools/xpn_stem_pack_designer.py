#!/usr/bin/env python3
"""
xpn_stem_pack_designer.py — XO_OX Stem Pack Design Assistant

Designs XPN expansion packs where each pad contains an individual stem
(kick, bass, melody, etc.) from a full track, enabling MPC remix/rearrange.

Usage:
    python xpn_stem_pack_designer.py --config stems.yaml --output stem_design.json

Config format (stems.yaml):
    pack_name: My Stem Pack
    artist: Artist Name
    bpm: 120
    tracks:
      - name: Track A
        stems: [kick, snare, hihat, bass, melody, pad, fx]
      - name: Track B
        stems: [kick, snare, bass, chords, lead]

Uses only Python stdlib: json, argparse, pathlib.
"""

import argparse
import json
import sys
from pathlib import Path


# ---------------------------------------------------------------------------
# YAML-like config parser (stdlib only — no PyYAML dependency)
# ---------------------------------------------------------------------------

def parse_simple_yaml(text: str) -> dict:
    """
    Parse a restricted YAML subset:
      - top-level key: value
      - list blocks under a key (items starting with '  - ')
      - nested mappings under a list item (indented key: value after '  - name:')
    Returns a plain dict/list structure.
    """
    result: dict = {}
    lines = text.splitlines()
    i = 0
    n = len(lines)

    def strip_comment(line: str) -> str:
        # Remove inline # comments outside of quoted values (simple heuristic)
        if "#" in line and '"' not in line and "'" not in line:
            line = line[:line.index("#")]
        return line.rstrip()

    def parse_value(raw: str):
        raw = raw.strip()
        if raw.startswith('"') and raw.endswith('"'):
            return raw[1:-1]
        if raw.startswith("'") and raw.endswith("'"):
            return raw[1:-1]
        if raw.lower() in ("true", "yes"):
            return True
        if raw.lower() in ("false", "no"):
            return False
        try:
            if "." in raw:
                return float(raw)
            return int(raw)
        except ValueError:
            return raw

    while i < n:
        line = strip_comment(lines[i])
        if not line or line.lstrip().startswith("#"):
            i += 1
            continue

        # Top-level key: value  OR  key: (block follows)
        if ":" in line and not line.startswith(" ") and not line.startswith("-"):
            key, _, rest = line.partition(":")
            key = key.strip()
            rest = rest.strip()
            if rest:
                result[key] = parse_value(rest)
                i += 1
            else:
                # Block value — collect indented lines
                i += 1
                block_lines = []
                while i < n:
                    bl = strip_comment(lines[i])
                    if bl and not bl.startswith(" ") and not bl.startswith("-"):
                        break  # next top-level key
                    block_lines.append(lines[i])
                    i += 1

                # Parse block as list of items
                items = []
                j = 0
                bn = len(block_lines)
                while j < bn:
                    bl = strip_comment(block_lines[j])
                    if not bl:
                        j += 1
                        continue
                    if bl.lstrip().startswith("- "):
                        # List item
                        item_val = bl.lstrip()[2:].strip()
                        # Check if next lines are sub-keys (nested mapping)
                        sub = {}
                        if item_val and ":" in item_val:
                            sk, _, sv = item_val.partition(":")
                            sub[sk.strip()] = parse_value(sv.strip())
                            item_val = ""
                        elif item_val:
                            # Inline list values like [a, b, c]
                            if item_val.startswith("[") and item_val.endswith("]"):
                                items.append([v.strip() for v in item_val[1:-1].split(",")])
                                j += 1
                                continue
                            # Simple scalar list item
                            j += 1
                            items.append(parse_value(item_val))
                            continue

                        j += 1
                        # Collect sub-key lines
                        while j < bn:
                            sbl = strip_comment(block_lines[j])
                            if not sbl:
                                j += 1
                                continue
                            indent = len(sbl) - len(sbl.lstrip())
                            if indent >= 4 and ":" in sbl:
                                sk2, _, sv2 = sbl.strip().partition(":")
                                sv2 = sv2.strip()
                                if sv2.startswith("[") and sv2.endswith("]"):
                                    sub[sk2.strip()] = [
                                        v.strip() for v in sv2[1:-1].split(",")
                                    ]
                                else:
                                    sub[sk2.strip()] = parse_value(sv2)
                                j += 1
                            else:
                                break
                        items.append(sub if sub else parse_value(item_val))
                    else:
                        j += 1

                result[key] = items
        else:
            i += 1

    return result


# ---------------------------------------------------------------------------
# Stem metadata helpers
# ---------------------------------------------------------------------------

DRUM_STEMS = {"kick", "snare", "hihat", "clap", "tom", "cymbal", "perc", "drum"}
EXPECTED_DRUM_PARTNERS = {
    "kick": "snare",
    "snare": "kick",
}

VELOCITY_MAP = {
    "kick": "high",
    "snare": "high",
    "clap": "high",
    "hihat": "medium",
    "tom": "medium",
    "cymbal": "medium",
    "perc": "medium",
    "bass": "high",
    "sub": "high",
    "melody": "medium",
    "lead": "medium",
    "chords": "low",
    "pad": "low",
    "strings": "low",
    "atmo": "low",
    "atmosphere": "low",
    "fx": "low",
    "riser": "low",
    "sweep": "low",
    "vocal": "medium",
    "vox": "medium",
    "sample": "medium",
}

# XO_OX quad-corner strategy: feliX (dry/wet) NW/NE, Oscar (dry/wet) SW/SE
CORNER_LABELS = {
    "NW": "feliX Dry",
    "NE": "feliX Wet",
    "SW": "Oscar Dry",
    "SE": "Oscar Wet",
}

def corner_for_stem(stem_name: str, stem_index: int) -> dict:
    """
    Assign quad-corner based on stem character:
      - Bright/harmonic/high-frequency stems → feliX corners (NW/NE)
      - Dark/rhythmic/low-frequency stems → Oscar corners (SW/SE)
      - Within each pair: even index → Dry, odd index → Wet
    """
    stem_lower = stem_name.lower()
    felix_stems = {"melody", "lead", "chords", "strings", "pad", "atmo", "atmosphere",
                   "vocal", "vox", "riser", "sweep", "fx", "hihat", "cymbal"}
    oscar_stems = {"kick", "snare", "bass", "sub", "tom", "perc", "clap", "drum", "sample"}

    if stem_lower in felix_stems:
        primary = "feliX"
        corner = "NW" if stem_index % 2 == 0 else "NE"
    elif stem_lower in oscar_stems:
        primary = "Oscar"
        corner = "SW" if stem_index % 2 == 0 else "SE"
    else:
        # Default: alternate feliX/Oscar by index
        if stem_index % 2 == 0:
            primary = "feliX"
            corner = "NW"
        else:
            primary = "Oscar"
            corner = "SW"

    return {
        "corner": corner,
        "label": CORNER_LABELS[corner],
        "polarity": primary,
    }


def velocity_for_stem(stem_name: str) -> str:
    return VELOCITY_MAP.get(stem_name.lower(), "medium")


# ---------------------------------------------------------------------------
# Pad layout designer
# ---------------------------------------------------------------------------

PADS_PER_BANK = 16
MAX_BANKS = 8
MAX_PADS = PADS_PER_BANK * MAX_BANKS  # 128

def design_pad_layout(tracks: list) -> tuple[list, list]:
    """
    Given tracks = [{"name": str, "stems": [str, ...]}, ...],
    return (pad_assignments, warnings).

    Pad assignment: list of dicts with bank, pad, track, stem, corner, velocity.
    Banks are 1-indexed; pads within a bank are 1-16 (MPC convention).
    Strategy: one bank per track (if ≤16 stems), otherwise multi-bank.
    Unused pads within a track's bank(s) are noted as empty.
    """
    assignments = []
    warnings = []
    global_pad_index = 0  # 0-based absolute pad index

    for track_idx, track in enumerate(tracks):
        track_name = track.get("name", f"Track {track_idx + 1}")
        stems = track.get("stems", [])

        if not stems:
            warnings.append(f"Track '{track_name}' has no stems defined — skipped.")
            continue

        # Validate drum stem completeness
        stem_set = {s.lower() for s in stems}
        for drum, partner in EXPECTED_DRUM_PARTNERS.items():
            if drum in stem_set and partner not in stem_set:
                warnings.append(
                    f"Track '{track_name}': has '{drum}' but missing '{partner}' "
                    f"— incomplete drum stem set."
                )

        # Check pad capacity
        if global_pad_index + len(stems) > MAX_PADS:
            warnings.append(
                f"Track '{track_name}': adding {len(stems)} stems would exceed "
                f"128-pad limit. Stems beyond pad 128 are omitted."
            )

        for stem_idx, stem in enumerate(stems):
            if global_pad_index >= MAX_PADS:
                break

            bank_number = (global_pad_index // PADS_PER_BANK) + 1
            pad_in_bank = (global_pad_index % PADS_PER_BANK) + 1
            corner_info = corner_for_stem(stem, stem_idx)

            assignments.append({
                "bank": bank_number,
                "pad": pad_in_bank,
                "absolute_pad": global_pad_index + 1,
                "track": track_name,
                "stem": stem,
                "corner": corner_info["corner"],
                "corner_label": corner_info["label"],
                "polarity": corner_info["polarity"],
                "velocity_sensitivity": velocity_for_stem(stem),
            })
            global_pad_index += 1

    total_pads = global_pad_index
    if total_pads > MAX_PADS:
        warnings.append(
            f"Pack exceeds 128-pad limit ({total_pads} pads requested). "
            f"Only first 128 pads included."
        )
    elif total_pads == 0:
        warnings.append("No pads assigned — check config for valid tracks and stems.")

    return assignments, warnings


# ---------------------------------------------------------------------------
# Bank summary helper
# ---------------------------------------------------------------------------

def summarize_banks(assignments: list) -> list:
    banks: dict[int, list] = {}
    for a in assignments:
        banks.setdefault(a["bank"], []).append(a)

    summary = []
    for bank_num in sorted(banks.keys()):
        pads = banks[bank_num]
        tracks_in_bank = list(dict.fromkeys(p["track"] for p in pads))
        summary.append({
            "bank": bank_num,
            "pad_count": len(pads),
            "empty_pads": PADS_PER_BANK - len(pads),
            "tracks": tracks_in_bank,
            "stems": [p["stem"] for p in pads],
        })
    return summary


# ---------------------------------------------------------------------------
# Main design function
# ---------------------------------------------------------------------------

def design_stem_pack(config: dict) -> dict:
    pack_name = config.get("pack_name", "Untitled Stem Pack")
    artist = config.get("artist", "Unknown Artist")
    bpm = config.get("bpm", 120)
    tracks = config.get("tracks", [])

    if not tracks:
        return {
            "error": "No tracks defined in config.",
            "pack_name": pack_name,
        }

    assignments, warnings = design_pad_layout(tracks)
    bank_summary = summarize_banks(assignments)

    total_stems = sum(len(t.get("stems", [])) for t in tracks)
    assigned_pads = len(assignments)
    total_banks = max((a["bank"] for a in assignments), default=0)

    design = {
        "pack_name": pack_name,
        "artist": artist,
        "bpm": bpm,
        "summary": {
            "tracks": len(tracks),
            "total_stems": total_stems,
            "assigned_pads": assigned_pads,
            "banks_used": total_banks,
            "max_pads": MAX_PADS,
            "capacity_pct": round(assigned_pads / MAX_PADS * 100, 1),
        },
        "warnings": warnings,
        "banks": bank_summary,
        "pads": assignments,
        "quad_corner_strategy": {
            "NW": "feliX Dry — bright/harmonic stems, unprocessed",
            "NE": "feliX Wet — bright/harmonic stems, effected",
            "SW": "Oscar Dry — dark/rhythmic stems, unprocessed",
            "SE": "Oscar Wet — dark/rhythmic stems, effected",
        },
    }

    return design


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="XO_OX Stem Pack Designer — design XPN stem expansion packs",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Example config (stems.yaml):
  pack_name: Summer Sessions
  artist: DJ Algonaut
  bpm: 95
  tracks:
    - name: Poolside
      stems: [kick, snare, hihat, bass, chords, melody, fx]
    - name: Midnight Drift
      stems: [kick, bass, pad, lead, vox]
        """,
    )
    parser.add_argument(
        "--config",
        type=Path,
        required=True,
        help="Path to YAML config file",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Path for JSON output (default: print to stdout)",
    )
    parser.add_argument(
        "--pretty",
        action="store_true",
        default=True,
        help="Pretty-print JSON output (default: True)",
    )
    args = parser.parse_args()

    # Read config
    config_path = args.config
    if not config_path.exists():
        print(f"ERROR: Config file not found: {config_path}", file=sys.stderr)
        sys.exit(1)

    config_text = config_path.read_text(encoding="utf-8")
    try:
        config = parse_simple_yaml(config_text)
    except Exception as exc:
        print(f"ERROR: Failed to parse config: {exc}", file=sys.stderr)
        sys.exit(1)

    # Design
    design = design_stem_pack(config)

    # Output
    indent = 2 if args.pretty else None
    json_out = json.dumps(design, indent=indent)

    if args.output:
        args.output.write_text(json_out, encoding="utf-8")
        print(f"Stem pack design written to: {args.output}")

        # Print human-readable summary to stdout
        s = design.get("summary", {})
        print(f"\nPack: {design['pack_name']} — {design.get('artist', '')} @ {design.get('bpm', '?')} BPM")
        print(f"Tracks: {s.get('tracks', 0)} | Pads: {s.get('assigned_pads', 0)}/{s.get('max_pads', 128)} "
              f"({s.get('capacity_pct', 0)}%) | Banks: {s.get('banks_used', 0)}")
        if design.get("warnings"):
            print(f"\nWarnings ({len(design['warnings'])}):")
            for w in design["warnings"]:
                print(f"  ! {w}")
        else:
            print("No warnings.")
    else:
        print(json_out)


if __name__ == "__main__":
    main()
