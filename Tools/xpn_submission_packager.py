#!/usr/bin/env python3
"""
XPN Submission Packager — XO_OX Designs
Intake normalizer for community pack submissions.

Reads a loose submission directory, normalizes files to XO_OX conventions,
generates missing required artifacts, and outputs a clean ZIP ready for QA.

Normalization steps:
  - WAV: 32-bit float → 24-bit PCM (stdlib wave + struct, no external deps)
  - Cover art: validate presence; generate SVG placeholder if missing
  - pack.yaml: generate from directory structure if missing
  - LICENSE.txt: generate CC BY-NC 4.0 template if missing
  - Output: {pack_name}.submission.zip

Usage:
    python xpn_submission_packager.py \\
        --input ./raw_submission/ \\
        --output ./normalized/ \\
        --pack-name "My Pack"

    python xpn_submission_packager.py \\
        --input ./raw_submission/ \\
        --output ./normalized/ \\
        --pack-name "My Pack" \\
        --contributor "beatmaker_x"
"""

import argparse
import hashlib
import json
import os
import shutil
import struct
import sys
import wave
import zipfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

# ── Brand constants ────────────────────────────────────────────────────────────
ACCENT_COLOR = "#E9C46A"   # XO Gold — used for placeholder cover art
PLACEHOLDER_SVG_W = 1400
PLACEHOLDER_SVG_H = 1400

# WAV format codes
WAVE_FORMAT_PCM        = 0x0001
WAVE_FORMAT_IEEE_FLOAT = 0x0003
WAVE_FORMAT_EXTENSIBLE = 0xFFFE

# Submission spec constants
COVER_ART_MIN_PX = 1400
COVER_ART_MAX_BYTES = 2 * 1024 * 1024  # 2 MB

VALID_ENGINES = {
    # Mascots
    "ODDFELIX", "ODDOSCAR",
    # Original fleet
    "OVERDUB", "ODYSSEY", "OBLONG", "OBESE", "ONSET",
    "OVERWORLD", "OPAL", "ORBITAL",
    "ORGANON", "OUROBOROS",
    "OBSIDIAN", "OVERBITE", "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC",
    "OCELOT", "OSPREY", "OSTERIA", "OWLFISH",
    "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE",
    "OPTIC", "OBLIQUE", "OMBRE", "ORCA", "OCTOPUS",
    # Phase 4 engines
    "OVERLAP", "OUTWIT",
    # V1 concept engines
    "OPENSKY", "OSTINATO", "OCEANDEEP", "OUIE",
    # Flagship
    "OBRIX",
    # V2 theorem engines
    "ORBWEAVE", "OVERTONE", "ORGANISM",
    # Singularity engines
    "OXBOW", "OWARE",
    # Kuramoto vocal synthesis
    "OPERA",
    # Psychology-driven boom bap drums
    "OFFERING",
    # Chef Quad Collection
    "OTO", "OCTAVE", "OLEG", "OTIS",
    # Kitchen Quad Collection
    "OVEN", "OCHRE", "OBELISK", "OPALINE",
    # Cellar Quad Collection
    "OGRE", "OLATE", "OAKEN", "OMEGA",
    # Garden Quad Collection
    "ORCHARD", "OVERGROW", "OSIER", "OXALIS",
    # Broth Quad Collection
    "OVERWASH", "OVERWORN", "OVERFLOW", "OVERCAST",
    # Fusion Quad Collection
    "OKEANOS", "ODDFELLOW", "ONKOLO", "OPCODE",
    # Membrane Collection
    "OSMOSIS",
    # Love Triangle Circuit Synth
    "OXYTOCIN",
    # Panoramic visionary synth
    "OUTLOOK",
    # Dual Engine Integration
    "OASIS", "OUTFLOW",
    # Cellular Automata Oscillator
    "OBIONT",
}

# All 15 canonical mood categories — source of truth is PresetManager / CLAUDE.md
VALID_MOODS = {
    "Foundation", "Atmosphere", "Entangled", "Prism", "Flux",
    "Aether", "Family", "Submerged", "Coupling", "Crystalline",
    "Deep", "Ethereal", "Kinetic", "Luminous", "Organic",
}


# ── Data classes ───────────────────────────────────────────────────────────────

@dataclass
class FileResult:
    path: str          # relative path inside output dir / zip
    action: str        # "copied", "normalized", "generated", "skipped", "warning"
    detail: str = ""


@dataclass
class PackageResult:
    pack_name: str
    zip_path: Path
    files: list = field(default_factory=list)
    warnings: list = field(default_factory=list)
    errors: list = field(default_factory=list)


# ── WAV utilities ──────────────────────────────────────────────────────────────

def _read_wav_format(wav_path: Path):
    """Return (format_code, num_channels, sample_rate, bit_depth, num_frames)."""
    with open(wav_path, "rb") as f:
        riff = f.read(12)
        if len(riff) < 12 or riff[:4] != b"RIFF" or riff[8:12] != b"WAVE":
            raise ValueError(f"Not a valid WAVE file: {wav_path.name}")

        format_code = None
        num_channels = None
        sample_rate = None
        bit_depth = None
        data_size = None

        while True:
            chunk_header = f.read(8)
            if len(chunk_header) < 8:
                break
            chunk_id, chunk_size = struct.unpack_from("<4sI", chunk_header)

            if chunk_id == b"fmt ":
                fmt_data = f.read(chunk_size)
                fmt_code = struct.unpack_from("<H", fmt_data, 0)[0]
                if fmt_code == WAVE_FORMAT_EXTENSIBLE and len(fmt_data) >= 28:
                    # SubFormat GUID: bytes 24-27 give the actual format
                    sub_fmt = struct.unpack_from("<H", fmt_data, 24)[0]
                    format_code = sub_fmt
                else:
                    format_code = fmt_code
                num_channels  = struct.unpack_from("<H", fmt_data, 2)[0]
                sample_rate   = struct.unpack_from("<I", fmt_data, 4)[0]
                bit_depth     = struct.unpack_from("<H", fmt_data, 14)[0]
            elif chunk_id == b"data":
                data_size = chunk_size
                break
            else:
                f.seek(chunk_size + (chunk_size & 1), 1)  # skip + word-align

    return format_code, num_channels, sample_rate, bit_depth, data_size


def _float32_to_int24(f_val: float) -> int:
    """Clamp and convert a float32 sample to a signed 24-bit integer."""
    clamped = max(-1.0, min(1.0, f_val))
    i = int(round(clamped * 8388607.0))
    return max(-8388608, min(8388607, i))


def normalize_wav_to_24bit(src: Path, dst: Path) -> bool:
    """
    If src is 32-bit float WAV, convert to 24-bit PCM and write to dst.
    Returns True if conversion was performed, False if file was already PCM.
    Raises ValueError for unsupported formats.
    """
    format_code, num_channels, sample_rate, bit_depth, data_size = _read_wav_format(src)

    if format_code == WAVE_FORMAT_PCM:
        # Already PCM — just copy
        shutil.copy2(src, dst)
        return False

    if format_code != WAVE_FORMAT_IEEE_FLOAT or bit_depth != 32:
        # Non-float non-PCM (e.g. 64-bit float, MP3-in-WAV) — copy with warning
        shutil.copy2(src, dst)
        raise ValueError(
            f"Unsupported WAV format 0x{format_code:04X} {bit_depth}-bit — copied as-is"
        )

    # Read raw float32 data
    with open(src, "rb") as f:
        riff = f.read(12)
        while True:
            chunk_header = f.read(8)
            if len(chunk_header) < 8:
                break
            chunk_id, chunk_size = struct.unpack_from("<4sI", chunk_header)
            if chunk_id == b"data":
                raw_data = f.read(chunk_size)
                break
            f.seek(chunk_size + (chunk_size & 1), 1)

    num_frames = len(raw_data) // (4 * num_channels)
    float_samples = struct.unpack_from(f"<{num_frames * num_channels}f", raw_data)

    # Pack as 24-bit little-endian
    packed_24 = bytearray()
    for s in float_samples:
        i24 = _float32_to_int24(s)
        # little-endian 3 bytes
        packed_24 += struct.pack("<i", i24)[:3]

    # Write using stdlib wave module
    with wave.open(str(dst), "wb") as wf:
        wf.setnchannels(num_channels)
        wf.setsampwidth(3)   # 3 bytes = 24 bit
        wf.setframerate(sample_rate)
        wf.writeframes(bytes(packed_24))

    return True


# ── Cover art utilities ────────────────────────────────────────────────────────

def find_cover_art(directory: Path) -> Optional[Path]:
    """Return the first plausible cover art file found in directory (non-recursive)."""
    candidates = ["cover_art.png", "cover.png", "artwork.png", "cover_art.jpg",
                  "cover.jpg", "artwork.jpg"]
    for name in candidates:
        p = directory / name
        if p.exists():
            return p
    # Fallback: any PNG/JPG in root
    for ext in ("*.png", "*.jpg", "*.jpeg"):
        matches = list(directory.glob(ext))
        if matches:
            return matches[0]
    return None


def generate_placeholder_svg(pack_name: str, dst: Path) -> None:
    """Generate a minimal SVG placeholder with pack name centered on accent color."""
    # Escape pack name for SVG
    safe_name = (pack_name
                 .replace("&", "&amp;")
                 .replace("<", "&lt;")
                 .replace(">", "&gt;")
                 .replace('"', "&quot;"))

    svg = f"""<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg"
     width="{PLACEHOLDER_SVG_W}" height="{PLACEHOLDER_SVG_H}"
     viewBox="0 0 {PLACEHOLDER_SVG_W} {PLACEHOLDER_SVG_H}">
  <rect width="{PLACEHOLDER_SVG_W}" height="{PLACEHOLDER_SVG_H}" fill="{ACCENT_COLOR}"/>
  <text
    x="{PLACEHOLDER_SVG_W // 2}"
    y="{PLACEHOLDER_SVG_H // 2}"
    font-family="Space Grotesk, Inter, sans-serif"
    font-size="96"
    font-weight="700"
    fill="#1A1A1A"
    text-anchor="middle"
    dominant-baseline="middle">{safe_name}</text>
  <text
    x="{PLACEHOLDER_SVG_W // 2}"
    y="{PLACEHOLDER_SVG_H // 2 + 120}"
    font-family="Inter, sans-serif"
    font-size="48"
    fill="#3A3A3A"
    text-anchor="middle"
    dominant-baseline="middle">XO_OX Designs — PLACEHOLDER ART</text>
</svg>
"""
    dst.write_text(svg, encoding="utf-8")


# ── Pack manifest utilities ────────────────────────────────────────────────────

def _infer_pack_yaml(directory: Path, pack_name: str) -> dict:
    """
    Build a pack.yaml dict inferred from directory structure.
    Scans presets/*.xometa for engine references.
    """
    engines_found = set()
    moods_found = set()
    preset_dir = directory / "presets"
    if preset_dir.is_dir():
        for xometa in preset_dir.glob("*.xometa"):
            try:
                data = json.loads(xometa.read_text(encoding="utf-8"))
                # Engine may appear as "engine" string or in "engines" list
                if "engine" in data:
                    engines_found.add(str(data["engine"]).upper())
                if "engines" in data and isinstance(data["engines"], list):
                    for e in data["engines"]:
                        engines_found.add(str(e).upper())
                # Mood may appear as "mood" string or in "mood_tags" list
                if "mood" in data:
                    moods_found.add(str(data["mood"]))
                if "mood_tags" in data and isinstance(data["mood_tags"], list):
                    for m in data["mood_tags"]:
                        moods_found.add(str(m))
            except Exception:
                pass

    valid_found = sorted(engines_found & VALID_ENGINES)
    # Filter scanned moods against VALID_MOODS; fall back to ["Foundation"] if none recognized
    valid_moods_found = sorted(moods_found & VALID_MOODS) or ["Foundation"]

    return {
        "pack_name": pack_name,
        "contributor_handle": "CONTRIBUTOR_HANDLE",
        "contributor_email": "contact@example.com",
        "target_engines": valid_found or ["UNKNOWN"],
        "mood_tags": valid_moods_found,
        "xo_ox_version": "1.0",
        "art_origin": "original",
        "_generated": True,
        "_note": "Auto-generated by xpn_submission_packager.py — please fill in all fields",
    }


def _serialize_yaml_simple(data: dict, indent: int = 0) -> str:
    """
    Minimal YAML serializer for the small dicts we produce.
    Handles str, int, float, bool, list-of-str/int, dict (nested 1 level).
    No external deps — pure stdlib.
    """
    lines = []
    prefix = "  " * indent
    for k, v in data.items():
        if isinstance(v, bool):
            lines.append(f"{prefix}{k}: {'true' if v else 'false'}")
        elif isinstance(v, (int, float)):
            lines.append(f"{prefix}{k}: {v}")
        elif isinstance(v, str):
            # Quote if contains special chars
            if any(c in v for c in ":#{}[]|>&*!,"):
                lines.append(f'{prefix}{k}: "{v}"')
            else:
                lines.append(f"{prefix}{k}: {v}")
        elif isinstance(v, list):
            if not v:
                lines.append(f"{prefix}{k}: []")
            else:
                items = ", ".join(
                    f'"{x}"' if isinstance(x, str) and any(c in x for c in ":#{}[]|>&*!,")
                    else (f'"{x}"' if isinstance(x, str) else str(x))
                    for x in v
                )
                lines.append(f"{prefix}{k}: [{items}]")
        elif isinstance(v, dict):
            lines.append(f"{prefix}{k}:")
            lines.append(_serialize_yaml_simple(v, indent + 1))
        else:
            lines.append(f"{prefix}{k}: {v}")
    return "\n".join(lines)


def generate_pack_yaml(data: dict, dst: Path) -> None:
    content = "# XO_OX Community Pack Manifest\n"
    content += "# Fill in all CONTRIBUTOR_HANDLE / contact@example.com fields before submitting.\n\n"
    content += _serialize_yaml_simple(data)
    content += "\n"
    dst.write_text(content, encoding="utf-8")


# ── LICENSE template ───────────────────────────────────────────────────────────

LICENSE_TEMPLATE = """\
Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0)

Pack Name    : {pack_name}
Contributor  : [YOUR NAME / HANDLE HERE]
Contact      : [YOUR EMAIL HERE]
Date         : {date}

-----------------------------------------------------------------------
GRANT OF LICENSE

I, [YOUR NAME / HANDLE HERE], grant XO_OX Designs a non-exclusive,
royalty-free, worldwide license to distribute this pack and confirm
all included material is original or cleared for commercial use.

This work is licensed to end users under the Creative Commons
Attribution-NonCommercial 4.0 International License.

You are free to:
  Share — copy and redistribute the material in any medium or format
  Adapt — remix, transform, and build upon the material

Under the following terms:
  Attribution   — You must give appropriate credit to XO_OX Designs
                  and the original contributor.
  NonCommercial — You may not use the material for commercial purposes
                  without explicit written permission from the contributor
                  and XO_OX Designs.

Full license text: https://creativecommons.org/licenses/by-nc/4.0/legalcode

-----------------------------------------------------------------------
CONTRIBUTOR SIGNATURE

By submitting this pack, I confirm that:
1. All samples are original recordings or cleared for commercial distribution.
2. No third-party copyrighted material is included without explicit clearance.
3. AI-generated samples, if any, are declared in pack.yaml (ai_samples: true).
4. I have read and agree to the XO_OX Community Submission Guidelines.

Signed: ___________________________________  Date: ___________________
        [YOUR NAME / HANDLE]
"""


def generate_license(pack_name: str, dst: Path) -> None:
    import datetime
    date_str = datetime.date.today().isoformat()
    content = LICENSE_TEMPLATE.format(pack_name=pack_name, date=date_str)
    dst.write_text(content, encoding="utf-8")


# ── SHA-256 fingerprint ────────────────────────────────────────────────────────

def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


# ── Main packager ──────────────────────────────────────────────────────────────

def package_submission(
    input_dir: Path,
    output_dir: Path,
    pack_name: str,
    contributor: Optional[str] = None,
) -> PackageResult:
    """
    Full normalization + packaging pipeline.
    Returns a PackageResult with per-file actions and warnings.
    """
    if not input_dir.is_dir():
        raise FileNotFoundError(f"Input directory not found: {input_dir}")

    output_dir.mkdir(parents=True, exist_ok=True)

    # Staging directory inside output
    safe_name = pack_name.replace(" ", "_").replace("/", "-")
    staging = output_dir / f"{safe_name}_staging"
    if staging.exists():
        shutil.rmtree(staging)
    staging.mkdir()

    result = PackageResult(
        pack_name=pack_name,
        zip_path=output_dir / f"{safe_name}.submission.zip",
    )

    # ── 1. Process presets/ ────────────────────────────────────────────────────
    presets_src = input_dir / "presets"
    preset_count = 0
    if presets_src.is_dir():
        presets_dst = staging / "presets"
        presets_dst.mkdir()
        for xometa in sorted(presets_src.glob("*.xometa")):
            dst = presets_dst / xometa.name
            shutil.copy2(xometa, dst)
            result.files.append(FileResult(f"presets/{xometa.name}", "copied"))
            preset_count += 1
        if preset_count == 0:
            result.warnings.append("presets/ directory is empty — no .xometa files found")
    else:
        result.warnings.append("No presets/ directory found — submission may be incomplete")

    # ── 2. Process samples/ ───────────────────────────────────────────────────
    samples_src = input_dir / "samples"
    wav_count = 0
    if samples_src.is_dir():
        samples_dst = staging / "samples"
        for wav_path in sorted(samples_src.rglob("*.wav")):
            rel = wav_path.relative_to(samples_src)
            dst = samples_dst / rel
            dst.parent.mkdir(parents=True, exist_ok=True)

            try:
                converted = normalize_wav_to_24bit(wav_path, dst)
                action = "normalized" if converted else "copied"
                result.files.append(FileResult(f"samples/{rel}", action,
                                               "32-bit float → 24-bit PCM" if converted else ""))
                wav_count += 1
            except ValueError as e:
                # Non-float WAV — copy as-is with warning
                shutil.copy2(wav_path, dst)
                result.files.append(FileResult(f"samples/{rel}", "warning", str(e)))
                result.warnings.append(f"samples/{rel}: {e}")
                wav_count += 1
            except Exception as e:
                result.errors.append(f"samples/{rel}: {e}")

    # ── 3. Cover art ──────────────────────────────────────────────────────────
    cover_src = find_cover_art(input_dir)
    if cover_src is not None:
        ext = cover_src.suffix.lower()
        cover_dst = staging / f"cover_art{ext}"
        shutil.copy2(cover_src, cover_dst)
        # Basic size check (dimensions require Pillow; skip here — flag for QA gate)
        file_size = cover_src.stat().st_size
        if file_size > COVER_ART_MAX_BYTES:
            result.warnings.append(
                f"cover art exceeds 2 MB ({file_size / 1024 / 1024:.1f} MB) — QA will flag"
            )
        result.files.append(FileResult(f"cover_art{ext}", "copied",
                                       f"{file_size // 1024} KB"))
    else:
        cover_dst = staging / "cover_art_PLACEHOLDER.svg"
        generate_placeholder_svg(pack_name, cover_dst)
        result.files.append(FileResult("cover_art_PLACEHOLDER.svg", "generated",
                                       "placeholder SVG — replace with real art before shipping"))
        result.warnings.append(
            "No cover art found — placeholder SVG generated. "
            "Replace with 1400×1400px PNG/JPG before final submission."
        )

    # ── 4. pack.yaml ──────────────────────────────────────────────────────────
    pack_yaml_src = input_dir / "pack.yaml"
    pack_yaml_dst = staging / "pack.yaml"
    if pack_yaml_src.exists():
        shutil.copy2(pack_yaml_src, pack_yaml_dst)
        result.files.append(FileResult("pack.yaml", "copied"))
    else:
        inferred = _infer_pack_yaml(input_dir, pack_name)
        if contributor:
            inferred["contributor_handle"] = contributor
        generate_pack_yaml(inferred, pack_yaml_dst)
        result.files.append(FileResult("pack.yaml", "generated",
                                       "inferred from directory — fill in all placeholder fields"))
        result.warnings.append(
            "pack.yaml missing — generated from directory structure. "
            "Contributor must fill in handle, email, target_engines, and mood_tags."
        )

    # ── 5. LICENSE.txt ────────────────────────────────────────────────────────
    license_src = input_dir / "LICENSE.txt"
    license_dst = staging / "LICENSE.txt"
    if license_src.exists():
        shutil.copy2(license_src, license_dst)
        result.files.append(FileResult("LICENSE.txt", "copied"))
    else:
        generate_license(pack_name, license_dst)
        result.files.append(FileResult("LICENSE.txt", "generated",
                                       "CC BY-NC 4.0 template — contributor must sign"))
        result.warnings.append(
            "LICENSE.txt missing — CC BY-NC 4.0 template generated. "
            "Contributor must sign before QA approval."
        )

    # ── 6. Passthrough any other files at root ─────────────────────────────────
    skip_names = {"pack.yaml", "LICENSE.txt"}
    skip_exts = {".xometa"}
    for item in sorted(input_dir.iterdir()):
        if item.is_dir():
            continue
        if item.name in skip_names:
            continue
        if item.suffix.lower() in skip_exts:
            continue
        if item.suffix.lower() in {".png", ".jpg", ".jpeg"}:
            continue  # Already handled above
        dst = staging / item.name
        shutil.copy2(item, dst)
        result.files.append(FileResult(item.name, "copied", "root file passthrough"))

    # ── 7. Generate SHA-256 manifest ──────────────────────────────────────────
    fingerprints = {}
    for f in sorted(staging.rglob("*")):
        if f.is_file():
            rel = str(f.relative_to(staging))
            fingerprints[rel] = sha256_file(f)

    manifest_path = staging / "sha256_manifest.txt"
    lines = [f"# SHA-256 fingerprints — {pack_name}\n"]
    for path, digest in sorted(fingerprints.items()):
        lines.append(f"{digest}  {path}\n")
    manifest_path.write_text("".join(lines), encoding="utf-8")
    result.files.append(FileResult("sha256_manifest.txt", "generated",
                                   f"{len(fingerprints)} files fingerprinted"))

    # ── 8. ZIP ────────────────────────────────────────────────────────────────
    zip_path = output_dir / f"{safe_name}.submission.zip"
    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for f in sorted(staging.rglob("*")):
            if f.is_file():
                arcname = str(f.relative_to(staging))
                zf.write(f, arcname)

    result.zip_path = zip_path

    # Cleanup staging
    shutil.rmtree(staging)

    return result


# ── CLI output ─────────────────────────────────────────────────────────────────

def _action_color(action: str) -> str:
    colors = {
        "copied":     "\033[32m",   # green
        "normalized": "\033[33m",   # yellow
        "generated":  "\033[36m",   # cyan
        "skipped":    "\033[90m",   # gray
        "warning":    "\033[31m",   # red
    }
    reset = "\033[0m"
    return colors.get(action, "") + action + reset


def print_summary(result: PackageResult, verbose: bool = True) -> None:
    print()
    print("=" * 70)
    print(f"  XPN Submission Packager — {result.pack_name}")
    print("=" * 70)
    print()

    # Per-file table
    col_w = 46
    print(f"  {'FILE':<{col_w}}  {'ACTION':<12}  DETAIL")
    print(f"  {'-' * col_w}  {'-' * 12}  {'-' * 24}")
    for fr in result.files:
        colored = _action_color(fr.action)
        detail = fr.detail[:40] if fr.detail else ""
        print(f"  {fr.path:<{col_w}}  {colored:<22}  {detail}")

    print()

    # Stats
    actions = [fr.action for fr in result.files]
    print(f"  Files: {len(result.files)} total  |  "
          f"copied: {actions.count('copied')}  |  "
          f"normalized: {actions.count('normalized')}  |  "
          f"generated: {actions.count('generated')}")

    if result.warnings:
        print()
        print(f"  \033[33mWarnings ({len(result.warnings)}):\033[0m")
        for w in result.warnings:
            print(f"    • {w}")

    if result.errors:
        print()
        print(f"  \033[31mErrors ({len(result.errors)}):\033[0m")
        for e in result.errors:
            print(f"    • {e}")

    print()
    zip_size_kb = result.zip_path.stat().st_size // 1024
    print(f"  Output: {result.zip_path}")
    print(f"  ZIP size: {zip_size_kb:,} KB")
    print()

    if result.errors:
        print("  \033[31mPACKAGING COMPLETED WITH ERRORS — review above\033[0m")
    elif result.warnings:
        print("  \033[33mPACKAGING COMPLETE — warnings require curator review\033[0m")
    else:
        print("  \033[32mPACKAGING COMPLETE — ready for QA\033[0m")
    print()


# ── Entry point ────────────────────────────────────────────────────────────────

def main() -> int:
    parser = argparse.ArgumentParser(
        description="XPN Submission Packager — normalize and ZIP community pack submissions",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--input",  "-i", required=True,
                        help="Path to raw submission directory")
    parser.add_argument("--output", "-o", required=True,
                        help="Path to output directory")
    parser.add_argument("--pack-name", "-n", required=True,
                        help='Pack display name, e.g. "Submerged Vol. 1"')
    parser.add_argument("--contributor", "-c", default=None,
                        help="Contributor handle (injected into generated pack.yaml)")
    parser.add_argument("--quiet", "-q", action="store_true",
                        help="Suppress per-file table; only print summary line")

    args = parser.parse_args()

    input_dir  = Path(args.input).resolve()
    output_dir = Path(args.output).resolve()

    try:
        result = package_submission(
            input_dir=input_dir,
            output_dir=output_dir,
            pack_name=args.pack_name,
            contributor=args.contributor,
        )
    except FileNotFoundError as e:
        print(f"\n  \033[31mError: {e}\033[0m\n")
        return 1
    except Exception as e:
        print(f"\n  \033[31mUnexpected error: {e}\033[0m\n")
        raise

    print_summary(result, verbose=not args.quiet)

    return 1 if result.errors else 0


if __name__ == "__main__":
    sys.exit(main())
