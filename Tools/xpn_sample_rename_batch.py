#!/usr/bin/env python3
"""
XPN Sample Rename Batch — XO_OX Designs
Batch-rename WAV samples inside a .xpn ZIP and update all XPM XML references.

Naming convention: {ENGINE}_{VOICE}_{DESCRIPTOR}_{INDEX:02d}.wav
Examples:
  ONSET_KICK_Heavy_01.wav
  OPAL_PAD_Shimmer_01.wav
  OBLONG_BASS_Warm_03.wav

Usage:
  # Dry-run with auto-naming (engine inferred from pack metadata or --engine flag)
  python xpn_sample_rename_batch.py pack.xpn --engine ONSET --dry-run

  # Apply renames from an explicit map
  python xpn_sample_rename_batch.py pack.xpn --map rename_map.json --output renamed.xpn

  # Machine-readable output of proposed renames
  python xpn_sample_rename_batch.py pack.xpn --engine ONSET --dry-run --format json
"""

import argparse
import json
import os
import re
import shutil
import sys
import tempfile
import zipfile
import xml.etree.ElementTree as ET
from pathlib import Path
from collections import defaultdict
from typing import Optional


# ---------------------------------------------------------------------------
# Constants

VOICE_KEYWORDS: dict[str, list[str]] = {
    "KICK":  ["kick", "bd", "bassdrum", "bass_drum", "kik", "kck"],
    "SNARE": ["snare", "snr", "sd", "snaredrum", "rimshot", "rim"],
    "HAT":   ["hat", "hihat", "hi_hat", "hh", "chh", "ohh",
               "closed_hat", "open_hat", "closedhat", "openhat"],
    "CLAP":  ["clap", "clp", "handclap", "hand_clap"],
    "TOM":   ["tom", "ltom", "mtom", "htom", "floor_tom"],
    "PERC":  ["perc", "percussion", "shaker", "tambourine", "conga",
               "bongo", "clave", "cowbell", "woodblock", "guiro"],
    "FX":    ["fx", "sfx", "effect", "noise", "riser", "sweep", "impact",
               "crash", "cymbal", "ride", "reverse", "vinyl", "crackle",
               "glitch", "stutter", "tape"],
    "PAD":   ["pad", "drone", "texture", "atmos", "atmosphere", "shimmer",
               "wash", "evolv", "ambient"],
    "BASS":  ["bass", "sub", "low", "808"],
    "LEAD":  ["lead", "synth", "arp", "melody", "melodic"],
    "VOCAL": ["vox", "vocal", "voice", "choir", "choral", "sing"],
}

# Suffix letters for conflict resolution: _B, _C, _D …
CONFLICT_SUFFIXES = list("BCDEFGHIJKLMNOPQRSTUVWXYZ")


# ---------------------------------------------------------------------------
# Helpers

def warn(msg: str) -> None:
    print(f"  WARNING: {msg}", file=sys.stderr)


def error(msg: str) -> None:
    print(f"  ERROR: {msg}", file=sys.stderr)


# ---------------------------------------------------------------------------
# Auto-naming helpers

def _voice_from_filename(stem: str) -> str:
    """Guess a VOICE token from a sample filename stem."""
    lower = stem.lower().replace("-", "_").replace(" ", "_")
    for voice, keywords in VOICE_KEYWORDS.items():
        for kw in keywords:
            if kw in lower:
                return voice
    return "SAMPLE"


def _descriptor_from_filename(stem: str) -> str:
    """Extract a clean one-word descriptor from a filename stem.

    Strategy: strip known voice keywords, numeric suffixes, and common filler
    words, then title-case the longest remaining token.
    """
    lower = stem.lower()
    # Remove leading/trailing underscores/dashes/spaces
    cleaned = re.sub(r"[_\-\s]+", "_", lower).strip("_")

    # Remove known voice keywords
    all_kws = {kw for kws in VOICE_KEYWORDS.values() for kw in kws}
    parts = cleaned.split("_")
    filtered = [p for p in parts if p not in all_kws and not re.fullmatch(r"\d+", p)]

    if not filtered:
        # Fall back to first token that is not purely numeric
        for p in parts:
            if not re.fullmatch(r"\d+", p):
                return p.title()
        return "Sample"

    # Pick the longest remaining word (most descriptive)
    best = max(filtered, key=len)
    return best.title()


def _infer_engine_from_xpn(zf: zipfile.ZipFile) -> Optional[str]:
    """Try to infer engine name from bundle_manifest.json or expansion manifest."""
    # bundle_manifest.json
    for name in zf.namelist():
        if name.lower().endswith("bundle_manifest.json"):
            try:
                data = json.loads(zf.read(name).decode("utf-8", errors="replace"))
                engine = data.get("engine") or data.get("Engine")
                if engine:
                    return str(engine).upper()
            except Exception:
                pass
        # Expansions/manifest plain-text key=value
        if re.search(r"[/\\]?manifest$", name, re.IGNORECASE):
            try:
                text = zf.read(name).decode("utf-8", errors="replace")
                m = re.search(r"(?i)^engine\s*=\s*(\S+)", text, re.MULTILINE)
                if m:
                    return m.group(1).upper()
            except Exception:
                pass
    return None


# ---------------------------------------------------------------------------
# XPM scanning

def get_xpm_paths(zf: zipfile.ZipFile) -> list[str]:
    return [n for n in zf.namelist() if n.lower().endswith(".xpm")]


def get_wav_paths(zf: zipfile.ZipFile) -> list[str]:
    return [n for n in zf.namelist() if n.lower().endswith(".wav")]


def scan_sample_refs(xml_bytes: bytes) -> list[str]:
    """Return all <SampleFile> values from an XPM XML document."""
    refs: list[str] = []
    try:
        root = ET.fromstring(xml_bytes)
        for elem in root.iter("SampleFile"):
            if elem.text and elem.text.strip():
                refs.append(elem.text.strip())
    except ET.ParseError:
        # Fallback: regex
        for m in re.finditer(rb"<SampleFile[^>]*>([^<]+)</SampleFile>", xml_bytes):
            refs.append(m.group(1).decode("utf-8", errors="replace").strip())
    return refs


def rewrite_sample_refs(xml_bytes: bytes, rename_map: dict[str, str]) -> tuple[bytes, int]:
    """Replace <SampleFile> values according to rename_map.

    Matches on the basename only (last path component) so paths like
    Samples/kick_001.wav are handled correctly.

    Returns (updated_bytes, change_count).
    """
    changes = 0

    def replace_ref(m: re.Match) -> bytes:
        nonlocal changes
        tag_open = m.group(1)
        value = m.group(2).decode("utf-8", errors="replace")
        tag_close = m.group(3)

        # Normalise path separators for lookup
        basename = Path(value.replace("\\", "/")).name
        if basename in rename_map:
            # Rebuild path with new basename, preserving directory component
            dir_part = value.replace("\\", "/")
            if "/" in dir_part:
                new_value = dir_part.rsplit("/", 1)[0] + "/" + rename_map[basename]
            else:
                new_value = rename_map[basename]
            changes += 1
            return tag_open + new_value.encode("utf-8") + tag_close
        return m.group(0)

    pattern = re.compile(rb"(<SampleFile[^>]*>)([^<]+)(</SampleFile>)", re.DOTALL)
    result = pattern.sub(replace_ref, xml_bytes)
    return result, changes


# ---------------------------------------------------------------------------
# Build rename plan

def build_auto_rename_plan(
    wav_paths: list[str],
    xpm_paths: list[str],
    zf: zipfile.ZipFile,
    engine: str,
) -> dict[str, str]:
    """Auto-generate {old_basename: new_basename} using XPM voice context and position."""

    # Gather sample refs from all XPMs with their order of appearance
    # order: list of (voice_hint, basename)
    ordered: list[tuple[str, str]] = []
    seen: set[str] = set()

    for xpm_path in sorted(xpm_paths):
        xml_bytes = zf.read(xpm_path)
        refs = scan_sample_refs(xml_bytes)
        for ref in refs:
            basename = Path(ref.replace("\\", "/")).name
            if basename not in seen:
                seen.add(basename)
                voice = _voice_from_filename(Path(basename).stem)
                ordered.append((voice, basename))

    # Add any WAV files not referenced by any XPM (orphan samples)
    wav_basenames = {Path(w).name for w in wav_paths}
    for basename in sorted(wav_basenames - seen):
        voice = _voice_from_filename(Path(basename).stem)
        ordered.append((voice, basename))

    # Assign sequential index per voice
    voice_counters: dict[str, int] = defaultdict(int)
    rename_map: dict[str, str] = {}

    for voice, basename in ordered:
        stem = Path(basename).stem
        descriptor = _descriptor_from_filename(stem)
        voice_counters[voice] += 1
        idx = voice_counters[voice]
        new_name = f"{engine}_{voice}_{descriptor}_{idx:02d}.wav"
        if basename != new_name:
            rename_map[basename] = new_name

    return rename_map


def resolve_conflicts(rename_map: dict[str, str]) -> dict[str, str]:
    """If two source files map to the same target, append _B, _C suffixes."""
    # target -> list of source basenames
    target_to_sources: dict[str, list[str]] = defaultdict(list)
    for src, tgt in rename_map.items():
        target_to_sources[tgt].append(src)

    resolved: dict[str, str] = {}
    for tgt, srcs in target_to_sources.items():
        if len(srcs) == 1:
            resolved[srcs[0]] = tgt
        else:
            # First source keeps original name, subsequent get _B, _C…
            resolved[srcs[0]] = tgt
            for i, src in enumerate(srcs[1:]):
                suffix = CONFLICT_SUFFIXES[i] if i < len(CONFLICT_SUFFIXES) else f"_{i+2}"
                stem, ext = os.path.splitext(tgt)
                resolved[src] = f"{stem}_{suffix}{ext}"

    return resolved


# ---------------------------------------------------------------------------
# Output formatting

def print_rename_table(
    rename_map: dict[str, str],
    all_wav_basenames: list[str],
    xpn_name: str,
) -> None:
    conflicts = _count_conflicts(rename_map)
    unchanged = [b for b in all_wav_basenames if b not in rename_map]

    print(f"\nRENAME PLAN — {xpn_name}")
    col_w = max((len(src) for src in rename_map), default=20) + 2
    for src, tgt in sorted(rename_map.items()):
        print(f"  {src:<{col_w}} → {tgt}")

    print(f"\nTotal: {len(rename_map)} rename(s) | {conflicts} conflict(s) | {len(unchanged)} unchanged")


def _count_conflicts(rename_map: dict[str, str]) -> int:
    targets = list(rename_map.values())
    return len(targets) - len(set(targets))


# ---------------------------------------------------------------------------
# Core write logic

def write_renamed_xpn(
    xpn_path: Path,
    rename_map: dict[str, str],
    output_path: Path,
) -> None:
    """Write a new .xpn where WAV files are renamed and XPM refs updated."""
    tmp_fd, tmp_path_str = tempfile.mkstemp(suffix=".xpn")
    tmp_path = Path(tmp_path_str)
    total_xml_changes = 0

    try:
        with zipfile.ZipFile(xpn_path, "r") as zf_in, \
             zipfile.ZipFile(tmp_path, "w", compression=zipfile.ZIP_DEFLATED) as zf_out:

            all_members = zf_in.namelist()

            for member in all_members:
                data = zf_in.read(member)
                info = zf_in.getinfo(member)
                basename = Path(member).name

                if member.lower().endswith(".wav") and basename in rename_map:
                    # Rename: reconstruct the full path with new basename
                    dir_part = member.replace("\\", "/")
                    if "/" in dir_part:
                        new_member = dir_part.rsplit("/", 1)[0] + "/" + rename_map[basename]
                    else:
                        new_member = rename_map[basename]
                    new_info = zipfile.ZipInfo(new_member)
                    new_info.compress_type = zipfile.ZIP_DEFLATED
                    zf_out.writestr(new_info, data)

                elif member.lower().endswith(".xpm"):
                    updated, n = rewrite_sample_refs(data, rename_map)
                    total_xml_changes += n
                    zf_out.writestr(info, updated)

                else:
                    zf_out.writestr(info, data)

        os.close(tmp_fd)
        shutil.move(str(tmp_path), str(output_path))
        print(f"\n  XPM references updated: {total_xml_changes}")
        print(f"  Written to: {output_path}")

    except Exception:
        try:
            os.close(tmp_fd)
        except Exception:
            pass
        tmp_path.unlink(missing_ok=True)
        raise


# ---------------------------------------------------------------------------
# Main

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Batch-rename WAV samples inside a .xpn ZIP and update XPM references.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("xpn", metavar="PACK.xpn", help="Input .xpn archive.")
    parser.add_argument(
        "--output", metavar="OUTPUT.xpn",
        help="Output path (default: <input>_renamed.xpn).",
    )
    parser.add_argument(
        "--map", metavar="rename_map.json",
        help="JSON file mapping old filenames to new filenames.",
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Print proposed renames without writing any files.",
    )
    parser.add_argument(
        "--engine", metavar="ENGINE",
        help="Engine prefix override for auto-naming (e.g. ONSET, OPAL).",
    )
    parser.add_argument(
        "--format", choices=["text", "json"], default="text",
        help="Output format for dry-run (default: text).",
    )
    args = parser.parse_args()

    xpn_path = Path(args.xpn)
    if not xpn_path.exists():
        error(f"File not found: {xpn_path}")
        sys.exit(1)

    output_path = Path(args.output) if args.output else \
        xpn_path.with_name(xpn_path.stem + "_renamed.xpn")

    with zipfile.ZipFile(xpn_path, "r") as zf:
        wav_paths = get_wav_paths(zf)
        xpm_paths = get_xpm_paths(zf)
        wav_basenames = [Path(w).name for w in wav_paths]

        if not wav_paths:
            print("No WAV files found in archive.")
            sys.exit(0)

        # ---- Build rename map ------------------------------------------------

        if args.map:
            map_path = Path(args.map)
            if not map_path.exists():
                error(f"Rename map not found: {map_path}")
                sys.exit(1)
            with open(map_path, "r", encoding="utf-8") as f:
                rename_map: dict[str, str] = json.load(f)
            if not isinstance(rename_map, dict):
                error("Rename map must be a JSON object.")
                sys.exit(1)
            # Validate: only basenames
            rename_map = {Path(k).name: Path(v).name for k, v in rename_map.items()}
        else:
            # Auto-naming
            engine = args.engine
            if not engine:
                engine = _infer_engine_from_xpn(zf)
            if not engine:
                error(
                    "Cannot determine engine prefix. "
                    "Pass --engine ENGINE or provide a --map file."
                )
                sys.exit(1)
            engine = engine.upper()
            rename_map = build_auto_rename_plan(wav_paths, xpm_paths, zf, engine)

        # ---- Resolve conflicts -----------------------------------------------
        n_conflicts_before = _count_conflicts(rename_map)
        rename_map = resolve_conflicts(rename_map)

        # ---- Output ----------------------------------------------------------
        if args.dry_run or args.format == "json":
            if args.format == "json":
                print(json.dumps(rename_map, indent=2, ensure_ascii=False))
            else:
                print_rename_table(rename_map, wav_basenames, xpn_path.name)
            print("\n[DRY RUN] No files written.")
            return

        # ---- Write -----------------------------------------------------------
        if not rename_map:
            print("Nothing to rename.")
            sys.exit(0)

        print_rename_table(rename_map, wav_basenames, xpn_path.name)
        write_renamed_xpn(xpn_path, rename_map, output_path)


if __name__ == "__main__":
    main()
