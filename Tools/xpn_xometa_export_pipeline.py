#!/usr/bin/env python3
"""
xpn_xometa_export_pipeline.py — XO_OX Designs
Complete pipeline: .xometa preset collection → distributable XPN ZIP pack.

Stages:
  1. Collect  — scan input dir for .xometa files, validate required fields
  2. Build    — synthesize expansion.json from metadata + CLI args
  3. Programs — generate minimal .xpm stub per preset (full-range keygroup)
  4. Samples  — copy WAV files referenced in .xometa parameters (if --samples-dir)
  5. Manifest — generate bundle_manifest.json
  6. Package  — ZIP everything into {name}_{version}.xpn
  7. Validate — check ZIP structure: expansion.json, ≥1 .xpm, file count reasonable

CLI:
    python xpn_xometa_export_pipeline.py \\
        --presets-dir Presets/XOmnibus/Foundation \\
        --output-dir /tmp/builds \\
        --name "TIDE TABLES" \\
        --version 1.0.0 \\
        --description "Entry-level XO_OX expansion" \\
        [--samples-dir /path/to/samples] \\
        [--dry-run]

Exit codes: 0 = success, 1 = failure
"""

import argparse
import json
import shutil
import sys
import zipfile
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Optional

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

MANUFACTURER = "XO_OX Designs"
SCHEMA_VERSION = 1
OXPORT_VERSION = "2.0"

# Required fields in every .xometa file
REQUIRED_META_FIELDS = ("name", "engines")

# XPN ZIP structure
EXPANSION_JSON = "expansion.json"
PROGRAMS_DIR = "Programs"
SAMPLES_DIR_NAME = "Samples"
MANIFEST_FILE = "bundle_manifest.json"

# XPM keygroup full-range defaults (CLAUDE.md: RootNote=0, KeyTrack=True, VelStart=0)
KEYGROUP_LOW = 0
KEYGROUP_HIGH = 127
ROOT_NOTE = 0


# ---------------------------------------------------------------------------
# Stage 1: Collect
# ---------------------------------------------------------------------------

def stage_collect(presets_dir: Path) -> List[dict]:
    """Scan presets_dir recursively for .xometa files and validate required fields."""
    print("Stage 1/7: Collect ...", end=" ", flush=True)

    if not presets_dir.exists():
        print(f"\n  [ERROR] Presets dir not found: {presets_dir}", file=sys.stderr)
        sys.exit(1)

    files = sorted(presets_dir.rglob("*.xometa"))
    if not files:
        print(f"\n  [ERROR] No .xometa files found in {presets_dir}", file=sys.stderr)
        sys.exit(1)

    collected = []
    errors = []

    for path in files:
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError) as exc:
            errors.append(f"  [WARN] Cannot parse {path.name}: {exc}")
            continue

        missing = [f for f in REQUIRED_META_FIELDS if not data.get(f)]
        if missing:
            errors.append(f"  [WARN] {path.name} missing required fields: {missing}")
            continue

        data["_path"] = path
        collected.append(data)

    for msg in errors:
        print(f"\n{msg}", file=sys.stderr)

    if not collected:
        print(f"\n  [ERROR] No valid .xometa files after validation.", file=sys.stderr)
        sys.exit(1)

    print(f"done ({len(collected)} presets, {len(errors)} skipped)")
    return collected


# ---------------------------------------------------------------------------
# Stage 2: Build expansion.json
# ---------------------------------------------------------------------------

def stage_build_expansion(
    presets: List[dict],
    name: str,
    version: str,
    description: str,
) -> dict:
    """Synthesize expansion.json dict from .xometa metadata and CLI args."""
    print("Stage 2/7: Build expansion.json ...", end=" ", flush=True)

    # Collect unique engine names across all presets
    engines_seen: set[str] = set()
    moods_seen: set[str] = set()
    for p in presets:
        for eng in p.get("engines", []):
            engines_seen.add(eng)
        mood = p.get("mood") or p.get("category") or ""
        if mood:
            moods_seen.add(mood)

    expansion = {
        "name": name,
        "version": version,
        "description": description,
        "type": "Expansion",
        "manufacturer": MANUFACTURER,
        "schemaVersion": SCHEMA_VERSION,
        "oxportVersion": OXPORT_VERSION,
        "engines": sorted(engines_seen),
        "moods": sorted(moods_seen),
        "presetCount": len(presets),
        "createdAt": datetime.now(timezone.utc).isoformat(),
    }

    print(f"done (engines: {', '.join(sorted(engines_seen)) or 'none'})")
    return expansion


# ---------------------------------------------------------------------------
# Stage 3: Build Programs directory
# ---------------------------------------------------------------------------

def _safe_xml_text(value: object) -> str:
    """Return a safe XML text value from any Python object."""
    if value is None:
        return ""
    return str(value).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace('"', "&quot;")


def _build_xpm(preset: dict) -> str:
    """
    Build a minimal KeygroupProgram XPM XML string for the given preset.
    Rules from CLAUDE.md: KeyTrack=True, RootNote=0, VelStart=0.
    """
    preset_name = _safe_xml_text(preset.get("name", "Unnamed"))
    engines_str = _safe_xml_text(", ".join(preset.get("engines", [])))
    mood = _safe_xml_text(preset.get("mood") or preset.get("category") or "")

    # Encode first 4 macro labels into ProgramPads Q-Link names
    macro_labels: List[str] = (preset.get("macroLabels") or ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"])[:4]
    macro_labels = (macro_labels + ["", "", "", ""])[:4]

    qlinks = "\n".join(
        f'      <Pad number="{i+1}" name="{_safe_xml_text(macro_labels[i])}" />'
        for i in range(4)
    )

    xpm = f"""<?xml version="1.0" encoding="UTF-8"?>
<MPCVObject version="2.1">
  <Program type="KeygroupProgram">
    <ProgramName>{preset_name}</ProgramName>
    <Tags>
      <Tag>{engines_str}</Tag>
      <Tag>{mood}</Tag>
    </Tags>
    <Keygroups>
      <Keygroup number="1">
        <LowNote>{KEYGROUP_LOW}</LowNote>
        <HighNote>{KEYGROUP_HIGH}</HighNote>
        <RootNote>{ROOT_NOTE}</RootNote>
        <KeyTrack>True</KeyTrack>
        <Layer number="1">
          <VelStart>0</VelStart>
          <VelEnd>127</VelEnd>
          <SampleName></SampleName>
        </Layer>
      </Keygroup>
    </Keygroups>
    <MacroAssignments>
{qlinks}
    </MacroAssignments>
  </Program>
</MPCVObject>
"""
    return xpm


def stage_build_programs(presets: List[dict], build_dir: Path) -> List[Path]:
    """Write one .xpm stub per preset into build_dir/Programs/."""
    print("Stage 3/7: Build Programs dir ...", end=" ", flush=True)

    programs_dir = build_dir / PROGRAMS_DIR
    programs_dir.mkdir(parents=True, exist_ok=True)

    written: List[Path] = []
    seen_names: dict[str, int] = {}

    for preset in presets:
        raw_name = preset.get("name") or "Unnamed"
        # Sanitize filename
        safe_name = "".join(c if c.isalnum() or c in " _-" else "_" for c in raw_name).strip()
        if not safe_name:
            safe_name = "Unnamed"

        # Deduplicate filenames
        if safe_name in seen_names:
            seen_names[safe_name] += 1
            safe_name = f"{safe_name}_{seen_names[safe_name]}"
        else:
            seen_names[safe_name] = 0

        xpm_path = programs_dir / f"{safe_name}.xpm"
        xpm_content = _build_xpm(preset)
        xpm_path.write_text(xpm_content, encoding="utf-8")
        written.append(xpm_path)

    print(f"done ({len(written)} .xpm files)")
    return written


# ---------------------------------------------------------------------------
# Stage 4: Copy samples
# ---------------------------------------------------------------------------

def _find_wav_references(preset: dict) -> list[str]:
    """
    Walk the parameters dict (nested or flat) and collect any string values
    that look like WAV file references (end with .wav, case-insensitive).
    """
    wavs: List[str] = []

    def _walk(obj: object) -> None:
        if isinstance(obj, str):
            if obj.lower().endswith(".wav"):
                wavs.append(obj)
        elif isinstance(obj, dict):
            for v in obj.values():
                _walk(v)
        elif isinstance(obj, list):
            for item in obj:
                _walk(item)

    _walk(preset.get("parameters"))
    return wavs


def stage_copy_samples(
    presets: List[dict],
    samples_src_dir: Optional[Path],
    build_dir: Path,
) -> int:
    """Copy WAV files referenced in presets from samples_src_dir into build_dir/Samples/."""
    print("Stage 4/7: Copy samples ...", end=" ", flush=True)

    if samples_src_dir is None:
        print("skipped (no --samples-dir)")
        return 0

    if not samples_src_dir.exists():
        print(f"\n  [ERROR] --samples-dir not found: {samples_src_dir}", file=sys.stderr)
        sys.exit(1)

    samples_out_dir = build_dir / SAMPLES_DIR_NAME
    samples_out_dir.mkdir(parents=True, exist_ok=True)

    copied = 0
    missing = 0

    for preset in presets:
        for wav_ref in _find_wav_references(preset):
            wav_name = Path(wav_ref).name
            src = samples_src_dir / wav_name
            if not src.exists():
                # Try recursive search
                matches = list(samples_src_dir.rglob(wav_name))
                src = matches[0] if matches else src

            if src.exists():
                dest = samples_out_dir / wav_name
                if not dest.exists():
                    shutil.copy2(src, dest)
                    copied += 1
            else:
                print(f"\n  [WARN] Sample not found: {wav_name}", file=sys.stderr)
                missing += 1

    print(f"done ({copied} copied, {missing} missing)")
    return copied


# ---------------------------------------------------------------------------
# Stage 5: Generate bundle_manifest.json
# ---------------------------------------------------------------------------

def stage_generate_manifest(
    presets: List[dict],
    xpm_files: List[Path],
    expansion: dict,
    build_dir: Path,
    copied_samples: int,
) -> dict:
    """Write bundle_manifest.json into build_dir."""
    print("Stage 5/7: Generate bundle_manifest.json ...", end=" ", flush=True)

    programs_list = []
    for preset, xpm_path in zip(presets, xpm_files):
        programs_list.append({
            "name": preset.get("name", "Unnamed"),
            "xpmFile": f"{PROGRAMS_DIR}/{xpm_path.name}",
            "engines": preset.get("engines", []),
            "mood": preset.get("mood") or preset.get("category") or "",
            "sonicDNA": preset.get("sonicDNA") or preset.get("sonic_dna") or {},
            "macroLabels": (preset.get("macroLabels") or ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"])[:4],
        })

    manifest = {
        "packName": expansion["name"],
        "packVersion": expansion["version"],
        "manufacturer": MANUFACTURER,
        "generatedAt": datetime.now(timezone.utc).isoformat(),
        "stats": {
            "totalPrograms": len(programs_list),
            "totalSamples": copied_samples,
            "engines": expansion["engines"],
            "moods": expansion["moods"],
        },
        "programs": programs_list,
    }

    manifest_path = build_dir / MANIFEST_FILE
    manifest_path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False), encoding="utf-8")

    print(f"done ({len(programs_list)} programs)")
    return manifest


# ---------------------------------------------------------------------------
# Stage 6: Package into XPN ZIP
# ---------------------------------------------------------------------------

def _xpn_filename(name: str, version: str) -> str:
    safe = "".join(c if c.isalnum() or c in "_-" else "_" for c in name).strip("_")
    ver = version.replace(".", "_")
    return f"{safe}_{ver}.xpn"


def stage_package(
    build_dir: Path,
    expansion: dict,
    output_dir: Path,
    name: str,
    version: str,
) -> Path:
    """ZIP build_dir contents into {name}_{version}.xpn in output_dir."""
    print("Stage 6/7: Package ...", end=" ", flush=True)

    output_dir.mkdir(parents=True, exist_ok=True)
    xpn_name = _xpn_filename(name, version)
    xpn_path = output_dir / xpn_name

    # Write expansion.json into build_dir first
    exp_path = build_dir / EXPANSION_JSON
    exp_path.write_text(json.dumps(expansion, indent=2, ensure_ascii=False), encoding="utf-8")

    file_count = 0
    with zipfile.ZipFile(xpn_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for file_path in sorted(build_dir.rglob("*")):
            if file_path.is_file():
                arcname = file_path.relative_to(build_dir)
                zf.write(file_path, arcname)
                file_count += 1

    print(f"done ({file_count} files → {xpn_path.name})")
    return xpn_path


# ---------------------------------------------------------------------------
# Stage 7: Validate
# ---------------------------------------------------------------------------

def stage_validate(xpn_path: Path) -> None:
    """Check ZIP structure: expansion.json present, ≥1 .xpm, file count reasonable."""
    print("Stage 7/7: Validate ...", end=" ", flush=True)

    if not xpn_path.exists():
        print(f"\n  [ERROR] Output file not found: {xpn_path}", file=sys.stderr)
        sys.exit(1)

    errors: List[str] = []
    warnings: List[str] = []

    with zipfile.ZipFile(xpn_path, "r") as zf:
        names = zf.namelist()

    # Check expansion.json
    if EXPANSION_JSON not in names:
        errors.append(f"Missing {EXPANSION_JSON} in ZIP")

    # Check ≥1 .xpm
    xpm_files = [n for n in names if n.lower().endswith(".xpm")]
    if not xpm_files:
        errors.append("No .xpm files found in ZIP")

    # Check bundle_manifest.json
    if MANIFEST_FILE not in names:
        warnings.append(f"Missing {MANIFEST_FILE} (non-fatal)")

    # File count sanity
    if len(names) > 10_000:
        warnings.append(f"Unusually large pack: {len(names)} files")
    elif len(names) < 2:
        errors.append(f"ZIP appears empty: only {len(names)} entries")

    if warnings:
        for w in warnings:
            print(f"\n  [WARN] {w}", file=sys.stderr)

    if errors:
        for e in errors:
            print(f"\n  [ERROR] {e}", file=sys.stderr)
        sys.exit(1)

    size_kb = xpn_path.stat().st_size // 1024
    print(f"done ({len(names)} files, {len(xpm_files)} .xpm, {size_kb} KB)")


# ---------------------------------------------------------------------------
# Dry-run preview
# ---------------------------------------------------------------------------

def dry_run_preview(
    presets: List[dict],
    name: str,
    version: str,
    description: str,
    samples_dir: Optional[Path],
    output_dir: Path,
) -> None:
    """Print the planned ZIP structure without building anything."""
    xpn_name = _xpn_filename(name, version)
    engines: set[str] = set()
    moods: set[str] = set()
    for p in presets:
        engines.update(p.get("engines", []))
        m = p.get("mood") or p.get("category") or ""
        if m:
            moods.add(m)

    print(f"\n[DRY RUN] Planned output: {output_dir / xpn_name}")
    print(f"  Pack name   : {name}")
    print(f"  Version     : {version}")
    print(f"  Description : {description}")
    print(f"  Engines     : {', '.join(sorted(engines)) or '(none)'}")
    print(f"  Moods       : {', '.join(sorted(moods)) or '(none)'}")
    print(f"  Presets     : {len(presets)}")
    print(f"  Samples dir : {samples_dir or '(not provided)'}")
    print(f"\n  ZIP contents would include:")
    print(f"    {EXPANSION_JSON}")
    print(f"    {MANIFEST_FILE}")
    print(f"    {PROGRAMS_DIR}/ ({len(presets)} × .xpm)")
    if samples_dir:
        total_refs = sum(len(_find_wav_references(p)) for p in presets)
        print(f"    {SAMPLES_DIR_NAME}/ (~{total_refs} WAV references)")
    print()


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="XPN XOmeta Export Pipeline — build a distributable .xpn pack from .xometa presets",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--presets-dir", required=True, type=Path,
        help="Directory to scan recursively for .xometa files",
    )
    parser.add_argument(
        "--output-dir", required=True, type=Path,
        help="Directory where the final .xpn file is written",
    )
    parser.add_argument(
        "--name", required=True,
        help="Pack name (e.g. 'TIDE TABLES')",
    )
    parser.add_argument(
        "--version", default="1.0.0",
        help="Pack version string (default: 1.0.0)",
    )
    parser.add_argument(
        "--description", default="",
        help="Human-readable pack description",
    )
    parser.add_argument(
        "--samples-dir", type=Path, default=None,
        help="Optional: directory containing WAV samples referenced by presets",
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Show planned structure without building anything",
    )
    return parser.parse_args()


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    args = parse_args()

    print(f"\nXPN XOmeta Export Pipeline — {args.name} v{args.version}")
    print("=" * 60)

    # Stage 1: Collect
    presets = stage_collect(args.presets_dir)

    # Dry-run exits here after preview
    if args.dry_run:
        dry_run_preview(
            presets,
            name=args.name,
            version=args.version,
            description=args.description,
            samples_dir=args.samples_dir,
            output_dir=args.output_dir,
        )
        print("[DRY RUN] No files written.")
        return

    # Build temp directory
    safe_name = "".join(c if c.isalnum() or c in "_-" else "_" for c in args.name).strip("_")
    build_dir = args.output_dir / f"_build_{safe_name}_{args.version}"
    if build_dir.exists():
        shutil.rmtree(build_dir)
    build_dir.mkdir(parents=True)

    try:
        # Stage 2: Build expansion.json
        expansion = stage_build_expansion(
            presets,
            name=args.name,
            version=args.version,
            description=args.description,
        )

        # Stage 3: Build Programs dir
        xpm_files = stage_build_programs(presets, build_dir)

        # Stage 4: Copy samples
        copied_samples = stage_copy_samples(presets, args.samples_dir, build_dir)

        # Stage 5: Generate manifest
        stage_generate_manifest(presets, xpm_files, expansion, build_dir, copied_samples)

        # Stage 6: Package
        xpn_path = stage_package(build_dir, expansion, args.output_dir, args.name, args.version)

        # Stage 7: Validate
        stage_validate(xpn_path)

    finally:
        # Clean up temp build dir
        if build_dir.exists():
            shutil.rmtree(build_dir)

    print("=" * 60)
    print(f"SUCCESS: {xpn_path}")
    print()


if __name__ == "__main__":
    main()
