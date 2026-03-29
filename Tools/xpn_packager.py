#!/usr/bin/env python3
"""
XPN Packager — XO_OX Designs
Creates MPC-loadable .xpn expansion archives from built program directories.

An XPN file is a ZIP archive with a specific internal structure that MPC
expects. Get the structure wrong and nothing loads — no error, just silence.

XPN ZIP Structure (Rex's Rule #6):
    MyPack.xpn (ZIP archive)
    ├── Expansions/
    │   └── manifest              (plain text: Name=, Version=, Author=)
    ├── Programs/
    │   ├── MyDrumKit.xpm         (drum or keygroup program XML)
    │   └── MyDrumKit.mp3         (preview audio — same base name)
    ├── Samples/
    │   └── MyDrumKit/            (subfolder per program)
    │       ├── Kick_v1.wav
    │       └── Snare_v2.wav
    └── (artwork.png)             (optional — in root or per Akai convention)

Usage:
    from xpn_packager import package_xpn, XPNMetadata

    meta = XPNMetadata(
        name="XOnset: 808 Collection",
        version="1.0",
        author="XO_OX Designs",
        description="808 drum kits from XOnset",
    )
    package_xpn(build_dir="/path/to/built/pack",
                output_path="/path/to/808_Collection.xpn",
                metadata=meta)

CLI:
    python3 xpn_packager.py --build-dir /path/to/pack \\
        --output MyPack.xpn --name "My Pack" --author "XO_OX Designs"
"""

import argparse
import json
import os
import shutil
import sys
import zipfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional, Union
from xml.sax.saxutils import escape as xml_escape


@dataclass
class XPNMetadata:
    """Expansion pack metadata for the manifest."""
    name: str
    version: str = "1.0"
    author: str = "XO_OX Designs"
    description: str = ""
    pack_id: str = ""
    cover_engine: str = ""
    pack_type: str = "instrument"


def generate_manifest(meta: XPNMetadata) -> str:
    """
    Generate the XPN manifest file content.

    MPC supports both plain-text key=value and XML expansion manifests.
    We generate BOTH — the plain-text `manifest` in Expansions/ (widest
    firmware compatibility) and an `Expansion.xml` (newer firmware features
    like artwork references).
    """
    return (
        f"Name={meta.name}\n"
        f"Version={meta.version}\n"
        f"Author={meta.author}\n"
        f"Description={meta.description}\n"
    )


def generate_expansion_xml(meta: XPNMetadata) -> str:
    """XML-format expansion manifest for newer MPC firmware."""
    pack_id = meta.pack_id or f"com.xo-ox.pack.{meta.name.lower().replace(' ', '-')}"
    return (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<expansion version="2.0.0.0" buildVersion="2.10.0.0">\n'
        '  <local/>\n'
        f'  <identifier>{xml_escape(pack_id)}</identifier>\n'
        f'  <title>{xml_escape(meta.name)}</title>\n'
        f'  <manufacturer>{xml_escape(meta.author)}</manufacturer>\n'
        f'  <version>{meta.version}.0</version>\n'
        f'  <type>{xml_escape(meta.pack_type)}</type>\n'
        '  <priority>50</priority>\n'
        '  <img>artwork.png</img>\n'
        f'  <description>{xml_escape(meta.description)}</description>\n'
        '  <separator>-</separator>\n'
        '</expansion>\n'
    )


def _collect_programs(build_dir: Path) -> list[tuple[str, Path]]:
    """
    Find all .xpm files in the build directory.
    Returns [(program_name, xpm_path), ...].

    Subdirectories are checked first (drums/, keygroups/, Programs/), then the
    root of build_dir.  A program slug is only added once — subdirectory copies
    win over root copies — so XPMs that exist in both locations are never
    double-added to the ZIP.
    """
    programs = []
    seen_stems: set = set()
    # Check subdirectories first (drums/, keygroups/)
    for subdir in ["drums", "keygroups", "Programs"]:
        sub = build_dir / subdir
        if sub.exists():
            for xpm in sorted(sub.glob("*.xpm")):
                if xpm.stem not in seen_stems:
                    programs.append((xpm.stem, xpm))
                    seen_stems.add(xpm.stem)
    # Also check flat .xpm files in build_dir root (only add if not already seen)
    for xpm in sorted(build_dir.glob("*.xpm")):
        if xpm.stem not in seen_stems:
            programs.append((xpm.stem, xpm))
            seen_stems.add(xpm.stem)
    return programs


def _collect_samples(build_dir: Path, program_name: str) -> list[Path]:
    """
    Find WAV files that belong to a program.
    Looks for files matching the program name pattern in build_dir and subdirs.
    """
    slug = program_name.replace(" ", "_")
    wavs = []

    # Check Samples/{program}/ subdirectory
    samples_sub = build_dir / "Samples" / slug
    if samples_sub.exists():
        wavs.extend(sorted(samples_sub.glob("*.wav")))
        wavs.extend(sorted(samples_sub.glob("*.WAV")))

    # Check flat WAVs in build_dir matching program slug
    for ext in ["*.wav", "*.WAV"]:
        for wf in sorted(build_dir.glob(ext)):
            if wf.stem.startswith(slug) and wf not in wavs:
                wavs.append(wf)

    return wavs


def _collect_previews(build_dir: Path, program_name: str) -> Optional[Path]:
    """Find a preview audio file (.mp3 or .wav) matching the program name."""
    slug = program_name.replace(" ", "_")
    for ext in [".mp3", ".wav"]:
        preview = build_dir / f"{slug}{ext}"
        if preview.exists():
            return preview
        # Check Programs/ subdir
        preview = build_dir / "Programs" / f"{slug}{ext}"
        if preview.exists():
            return preview
    return None


def package_xpn(
    build_dir: Union[str, Path],
    output_path: Union[str, Path],
    metadata: XPNMetadata,
    include_artwork: bool = True,
    verbose: bool = True,
) -> Path:
    """
    Package a build directory into a proper .xpn expansion archive.

    Args:
        build_dir: Directory containing .xpm programs, WAV samples, artwork
        output_path: Where to write the .xpn file
        metadata: Pack metadata for the manifest
        include_artwork: Whether to include artwork.png/artwork_2000.png
        verbose: Print progress

    Returns:
        Path to the created .xpn file
    """
    build_dir = Path(build_dir)
    output_path = Path(output_path)

    if not output_path.suffix:
        output_path = output_path.with_suffix(".xpn")

    output_path.parent.mkdir(parents=True, exist_ok=True)

    programs = _collect_programs(build_dir)
    if not programs:
        print(f"  [ERROR] No .xpm programs found in {build_dir}")
        return output_path

    if verbose:
        print(f"\n  Packaging: {metadata.name}")
        print(f"  Programs: {len(programs)}")

    with zipfile.ZipFile(str(output_path), "w", zipfile.ZIP_DEFLATED) as zf:

        # --- Expansions/manifest ---
        manifest_content = generate_manifest(metadata)
        zf.writestr("Expansions/manifest", manifest_content)
        if verbose:
            print("  + Expansions/manifest")

        # --- Expansions/Expansion.xml (newer firmware) ---
        expansion_xml = generate_expansion_xml(metadata)
        zf.writestr("Expansions/Expansion.xml", expansion_xml)

        # --- Programs/ ---
        total_samples = 0
        for prog_name, xpm_path in programs:
            # Rewrite SampleFile paths to XPN-relative before writing into ZIP
            prog_slug = prog_name.replace(" ", "_")
            raw_xpm = xpm_path.read_text(encoding="utf-8")
            rewritten_xpm = rewrite_sample_paths(raw_xpm, prog_slug)
            # Write the program file (rewritten content)
            arc_path = f"Programs/{xpm_path.name}"
            zf.writestr(arc_path, rewritten_xpm)
            if verbose:
                print(f"  + {arc_path}")

            # Write preview file if it exists
            preview = _collect_previews(build_dir, prog_name)
            if preview:
                preview_arc = f"Programs/{preview.name}"
                zf.write(str(preview), preview_arc)
                if verbose:
                    print(f"  + {preview_arc}")

            # Write samples for this program
            wavs = _collect_samples(build_dir, prog_name)
            for wav in wavs:
                sample_arc = f"Samples/{prog_name}/{wav.name}"
                zf.write(str(wav), sample_arc)
                total_samples += 1

        if verbose and total_samples:
            print(f"  + Samples/ ({total_samples} files)")

        # --- Artwork ---
        if include_artwork:
            for art_name in ["artwork.png", "artwork_2000.png"]:
                art_path = build_dir / art_name
                if art_path.exists():
                    zf.write(str(art_path), art_name)
                    if verbose:
                        print(f"  + {art_name}")

        # --- DISCLAIMER.txt ---
        # Include in every .xpn so installers understand this is NOT an Akai
        # official expansion and know how to install it correctly.
        disclaimer_path = Path(__file__).parent / "DISCLAIMER.txt"
        if disclaimer_path.exists():
            zf.write(str(disclaimer_path), "DISCLAIMER.txt")
            if verbose:
                print("  + DISCLAIMER.txt")

        # --- Bundle manifest (JSON, for our own tooling reference) ---
        bundle_manifest = build_dir / "bundle_manifest.json"
        if bundle_manifest.exists():
            zf.write(str(bundle_manifest), "bundle_manifest.json")

        # --- Liner notes (generated by xpn_liner_notes.py) ---
        # Matches liner_notes.json or liner_notes_*.json placed in the build dir.
        for ln_path in sorted(build_dir.glob("liner_notes*.json")):
            zf.write(str(ln_path), ln_path.name)
            if verbose:
                print(f"  + {ln_path.name}")

        # --- Coupling recipes (if a recipe_dir was provided or auto-discovered) ---
        # xpn_coupling_recipes.py generates {preset_slug}_coupling_recipe.json files.
        # We include any recipes found for the pack's programs so producers know how
        # to recreate live coupling in XOlokun from the static XPN audio.
        coupling_recipes_dir = build_dir / "coupling_recipes"
        if coupling_recipes_dir.exists():
            recipe_jsons = sorted(coupling_recipes_dir.glob("*_coupling_recipe.json"))
            if recipe_jsons:
                for recipe_json in recipe_jsons:
                    arc_path = f"CouplingRecipes/{recipe_json.name}"
                    zf.write(str(recipe_json), arc_path)
                    if verbose:
                        print(f"  + {arc_path}")

    if verbose:
        size_kb = output_path.stat().st_size / 1024
        unit = "KB" if size_kb < 1024 else "MB"
        size_val = size_kb if size_kb < 1024 else size_kb / 1024
        print(f"\n  Output: {output_path}  ({size_val:.1f} {unit})")

    return output_path


def rewrite_sample_paths(xpm_content: str, program_slug: str) -> str:
    """
    Rewrite SampleFile values in XPM XML to use XPN-relative paths.

    Transforms: <SampleFile>808_Reborn_kick_v1.wav</SampleFile>
    Into:       <SampleFile>Samples/808_Reborn/808_Reborn_kick_v1.wav</SampleFile>

    Also adds <File> element if not present (for firmware compatibility).
    """
    import re

    def _replace_sample_file(m):
        filename = m.group(1)
        if not filename:
            return m.group(0)
        # Don't double-prefix
        if filename.startswith("Samples/"):
            return m.group(0)
        return f"<SampleFile>Samples/{program_slug}/{filename}</SampleFile>"

    result = re.sub(
        r"<SampleFile>(.*?)</SampleFile>",
        _replace_sample_file,
        xpm_content,
    )
    return result


# =============================================================================
# CLI
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Packager — create MPC expansion archives",
    )
    parser.add_argument("--build-dir", required=True,
                        help="Directory containing built programs and samples")
    parser.add_argument("--output",    required=True,
                        help="Output .xpn file path")
    parser.add_argument("--name",      required=True,
                        help="Pack name")
    parser.add_argument("--author",    default="XO_OX Designs")
    parser.add_argument("--version",   default="1.0")
    parser.add_argument("--description", default="")
    parser.add_argument("--no-artwork", action="store_true")
    parser.add_argument(
        "--coupling-recipes",
        metavar="DIR",
        default=None,
        help=(
            "Directory of *_coupling_recipe.json files generated by xpn_coupling_recipes.py. "
            "If provided, matching recipes are attached to the XPN under CouplingRecipes/. "
            "If omitted, the packager checks build_dir/coupling_recipes/ automatically."
        ),
    )
    args = parser.parse_args()

    meta = XPNMetadata(
        name=args.name,
        version=args.version,
        author=args.author,
        description=args.description or f"{args.name} — XO_OX Designs",
    )

    xpn_path = package_xpn(
        build_dir=args.build_dir,
        output_path=args.output,
        metadata=meta,
        include_artwork=not args.no_artwork,
    )

    # Attach coupling recipes from an explicit --coupling-recipes directory.
    # (Recipes in build_dir/coupling_recipes/ are already included by package_xpn.)
    if args.coupling_recipes:
        try:
            from xpn_coupling_recipes import attach_recipe_to_pack
            # Attach every recipe found in the directory to the pack.
            from pathlib import Path as _Path
            recipes_dir = _Path(args.coupling_recipes)
            attached = 0
            for recipe_json in sorted(recipes_dir.glob("*_coupling_recipe.json")):
                preset_name = recipe_json.stem.replace("_coupling_recipe", "").replace("_", " ")
                if attach_recipe_to_pack(xpn_path, preset_name, recipes_dir):
                    attached += 1
            if attached:
                print(f"  Attached {attached} coupling recipe(s) from {recipes_dir}")
        except ImportError:
            print("  [WARN] xpn_coupling_recipes.py not found — skipping recipe attachment")


if __name__ == "__main__":
    main()
