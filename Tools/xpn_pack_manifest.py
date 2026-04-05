#!/usr/bin/env python3
"""
XPN Pack Manifest — Curated preset collections for Series-of-Packs architecture.
Dave Smith amendment (QDD Ghost Council, 2026-04-04).

.oxpack files are YAML manifests that describe a named, editorially-curated preset
collection.  They are lightweight complements to .oxbuild (the build configuration
format) — a manifest says *which* presets are in a pack, not *how* to build it.

Usage:
    from xpn_pack_manifest import load_manifest, validate_manifest, estimate_pack_size

    manifest = load_manifest("packs/onset-deep-water.oxpack")
    errors, warnings = validate_manifest(manifest, preset_dir="/path/to/presets")
    size_bytes = estimate_pack_size(manifest)

CLI:
    python3 xpn_pack_manifest.py packs/onset-deep-water.oxpack
    python3 xpn_pack_manifest.py packs/onset-deep-water.oxpack --validate-presets /path/to/presets
"""

from __future__ import annotations

import os
import sys
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# PyYAML — soft dependency with graceful fallback to a minimal parser
# ---------------------------------------------------------------------------

try:
    import yaml as _yaml
    _YAML_AVAILABLE = True
except ImportError:
    _yaml = None  # type: ignore[assignment]
    _YAML_AVAILABLE = False


def _parse_yaml(text: str) -> dict:
    """Parse YAML text, using PyYAML if available, otherwise a minimal fallback.

    The fallback handles simple key: value pairs and list items (- item) only.
    It is sufficient for .oxpack manifests but not a general YAML parser.
    Install PyYAML for full support: pip install pyyaml
    """
    if _YAML_AVAILABLE:
        return _yaml.safe_load(text)

    # ── Minimal YAML fallback ──────────────────────────────────────────────
    # Supports:
    #   key: value          (string, int, float, bool)
    #   key:                (starts a list on the next lines)
    #     - item            (list items)
    #   # comments
    result: dict = {}
    current_key: Optional[str] = None
    current_list: Optional[list] = None

    for raw_line in text.splitlines():
        # Strip inline comments (not inside quotes — good enough for manifest files)
        line = raw_line.split("#")[0].rstrip()
        if not line.strip():
            continue

        if line.startswith("  - ") or line.startswith("- "):
            # List item
            item = line.strip().lstrip("- ").strip().strip('"').strip("'")
            if current_list is not None:
                current_list.append(item)
            elif current_key is not None:
                result[current_key] = [item]
                current_list = result[current_key]
            continue

        if ": " in line or line.rstrip().endswith(":"):
            current_list = None  # leaving a list context
            if ": " in line:
                key, _, raw_val = line.partition(": ")
                key = key.strip()
                raw_val = raw_val.strip().strip('"').strip("'")
                # Type coercion
                if raw_val.lower() in ("true", "yes"):
                    val: object = True
                elif raw_val.lower() in ("false", "no"):
                    val = False
                else:
                    try:
                        val = int(raw_val)
                    except ValueError:
                        try:
                            val = float(raw_val)
                        except ValueError:
                            val = raw_val
                result[key] = val
                current_key = key
            else:
                # key: with no value — starts a list
                current_key = line.rstrip(":").strip()
                result[current_key] = []
                current_list = result[current_key]

    return result


# ---------------------------------------------------------------------------
# Schema
# ---------------------------------------------------------------------------

MANIFEST_SCHEMA: dict = {
    "required": ["name", "engine", "presets"],
    "optional": ["description", "tier", "mood", "tags", "max_size_gb"],
}

VALID_TIERS = {"SURFACE", "DEEP", "TRENCH"}

# Recommended preset count bounds (warnings, not errors)
RECOMMENDED_MIN_PRESETS = 10
RECOMMENDED_MAX_PRESETS = 50


# ---------------------------------------------------------------------------
# Core API
# ---------------------------------------------------------------------------

def load_manifest(path: str | Path) -> dict:
    """Load and validate a .oxpack YAML manifest.

    Args:
        path: Path to the .oxpack file.

    Returns:
        Validated manifest dict with defaults filled in.

    Raises:
        FileNotFoundError: if path does not exist.
        ValueError: if required fields are missing or presets list is empty.
    """
    path = Path(path)
    if not path.exists():
        raise FileNotFoundError(f"Manifest not found: {path}")

    text = path.read_text(encoding="utf-8", errors="replace")
    manifest = _parse_yaml(text)

    if not isinstance(manifest, dict):
        raise ValueError(f"Manifest must be a YAML mapping (got {type(manifest).__name__})")

    # Validate required fields
    for field in MANIFEST_SCHEMA["required"]:
        if field not in manifest:
            raise ValueError(f"Manifest missing required field: '{field}'")

    # Validate presets is a non-empty list
    presets = manifest.get("presets")
    if not isinstance(presets, list) or len(presets) == 0:
        raise ValueError("Manifest 'presets' must be a non-empty list")

    # Fill in defaults
    manifest.setdefault("tier", "DEEP")
    manifest.setdefault("max_size_gb", 1.6)
    manifest.setdefault("tags", [])

    # Normalize tier to uppercase
    manifest["tier"] = str(manifest["tier"]).upper()

    return manifest


def validate_manifest(manifest: dict, preset_dir: str | Path | None = None) -> tuple[list[str], list[str]]:
    """Validate a loaded manifest, optionally checking that presets exist on disk.

    Args:
        manifest: dict returned by load_manifest().
        preset_dir: Optional path to the preset directory to check preset existence.
                    When provided, warns for any preset name that cannot be found.

    Returns:
        (errors, warnings) — both are lists of strings.
        errors: validation failures that should block the build.
        warnings: advisory issues that do not block the build.
    """
    errors: list[str] = []
    warnings: list[str] = []

    presets = manifest.get("presets", [])

    # Tier validation
    tier = manifest.get("tier", "DEEP")
    if tier not in VALID_TIERS:
        errors.append(f"Unknown tier '{tier}'. Valid values: {', '.join(sorted(VALID_TIERS))}")

    # Preset count checks
    n = len(presets)
    if n > RECOMMENDED_MAX_PRESETS:
        warnings.append(
            f"Pack has {n} presets (recommended max: {RECOMMENDED_MAX_PRESETS}). "
            f"Consider splitting into multiple packs."
        )
    if n < RECOMMENDED_MIN_PRESETS:
        warnings.append(
            f"Pack has only {n} presets (recommended min: {RECOMMENDED_MIN_PRESETS}). "
            f"Consider adding more variety."
        )

    # Duplicate preset names
    seen: set[str] = set()
    for p in presets:
        p_lower = str(p).lower()
        if p_lower in seen:
            warnings.append(f"Duplicate preset name in manifest: '{p}'")
        seen.add(p_lower)

    # Size constraint
    max_gb = manifest.get("max_size_gb", 1.6)
    estimated_bytes = estimate_pack_size(manifest)
    estimated_gb = estimated_bytes / 1_073_741_824
    if estimated_gb > max_gb:
        warnings.append(
            f"Estimated pack size ({estimated_gb:.2f} GB) exceeds max_size_gb ({max_gb} GB). "
            f"Consider reducing preset count or using a lower tier."
        )

    # Filesystem preset existence check
    if preset_dir is not None:
        preset_dir = Path(preset_dir)
        if preset_dir.exists():
            missing: list[str] = []
            for preset_name in presets:
                # Search for the preset by name (stem match, case-insensitive)
                name_lower = str(preset_name).lower().replace(" ", "_")
                found = False
                for candidate in preset_dir.rglob("*.xometa"):
                    if candidate.stem.lower() == name_lower or \
                       candidate.stem.lower().replace("_", " ") == str(preset_name).lower():
                        found = True
                        break
                if not found:
                    missing.append(str(preset_name))

            if missing:
                for m in missing[:5]:
                    warnings.append(f"Preset not found in preset_dir: '{m}'")
                if len(missing) > 5:
                    warnings.append(f"... and {len(missing) - 5} more preset(s) not found")
        else:
            warnings.append(f"preset_dir does not exist: {preset_dir}")

    return errors, warnings


def estimate_pack_size(
    manifest: dict,
    avg_wav_bytes: int = 900_000,
    vel_layers: int = 4,
    voices: int = 8,
) -> int:
    """Estimate the total uncompressed .xpn size from a manifest.

    Args:
        manifest: loaded manifest dict.
        avg_wav_bytes: average WAV file size in bytes (default: ~900KB for 3.5s 24-bit stereo).
        vel_layers: number of velocity layers per voice (default: 4 for DEEP/TRENCH tier).
        voices: number of voices per preset (default: 8 — ONSET voice count).

    Returns:
        Estimated total size in bytes.

    Note:
        The actual size depends on voice taxonomy (round-robin counts vary per voice),
        sample duration, and bit depth.  This is a planning estimate, not a guarantee.
        For production accuracy use oxport_render.build_render_jobs() + _estimate_disk_usage().
    """
    tier = manifest.get("tier", "DEEP").upper()
    if tier == "SURFACE":
        vel_layers = 1
    elif tier in ("DEEP", "TRENCH"):
        vel_layers = 4

    n_presets = len(manifest.get("presets", []))
    samples_per_preset = voices * vel_layers
    total_bytes = n_presets * samples_per_preset * avg_wav_bytes
    return total_bytes


# ---------------------------------------------------------------------------
# .oxpack → .oxbuild preset filter integration (Feature 3 / oxport.py bridge)
# ---------------------------------------------------------------------------

def apply_manifest_to_build_args(manifest: dict, spec: dict) -> dict:
    """Inject a manifest's preset list and tier into an .oxbuild spec dict.

    This is called by oxport.py's cmd_build when a .oxpack file is passed
    as the build spec argument instead of a .oxbuild JSON file.

    The manifest's ``presets`` list is written into
    ``spec["preset_selection"]["explicit_presets"]`` and the manifest's
    ``tier`` overrides ``spec["tier"]``.

    Args:
        manifest: loaded manifest dict (from load_manifest()).
        spec: mutable .oxbuild spec dict to update.

    Returns:
        The mutated spec dict (same object, returned for chaining).
    """
    spec.setdefault("preset_selection", {})
    spec["preset_selection"]["explicit_presets"] = list(manifest["presets"])
    spec["preset_selection"]["mode"] = "explicit"

    if "tier" in manifest:
        spec["tier"] = manifest["tier"]

    if "engine" in manifest and "engine" not in spec:
        spec["engine"] = manifest["engine"]

    return spec


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _cli_main(argv: list[str] | None = None) -> int:
    """Validate and display a .oxpack manifest."""
    import argparse
    parser = argparse.ArgumentParser(
        description="XPN Pack Manifest validator — loads and validates a .oxpack YAML manifest.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("manifest", help="Path to .oxpack file")
    parser.add_argument("--validate-presets", metavar="DIR", default=None,
                        dest="preset_dir",
                        help="Check that each preset in the manifest exists in DIR")
    args = parser.parse_args(argv)

    try:
        manifest = load_manifest(args.manifest)
    except (FileNotFoundError, ValueError) as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 1

    print(f"Manifest: {args.manifest}")
    print(f"  Name:        {manifest.get('name', '?')}")
    print(f"  Engine:      {manifest.get('engine', '?')}")
    print(f"  Tier:        {manifest.get('tier', '?')}")
    print(f"  Description: {manifest.get('description', '(none)')}")
    print(f"  Presets:     {len(manifest.get('presets', []))}")
    print(f"  Max size:    {manifest.get('max_size_gb', 1.6)} GB")
    if manifest.get("tags"):
        print(f"  Tags:        {', '.join(manifest['tags'])}")

    est_bytes = estimate_pack_size(manifest)
    est_gb = est_bytes / 1_073_741_824
    print(f"  Est. size:   {est_gb:.2f} GB (simplified estimate)")

    errors, warnings = validate_manifest(manifest, preset_dir=args.preset_dir)

    if warnings:
        print()
        for w in warnings:
            print(f"  [WARN] {w}")

    if errors:
        print()
        for e in errors:
            print(f"  [ERROR] {e}")
        return 1

    if not errors and not warnings:
        print("\n  [PASS] Manifest is valid.")

    print()
    print("Presets:")
    for i, p in enumerate(manifest["presets"], 1):
        print(f"  {i:3d}. {p}")

    return 0


if __name__ == "__main__":
    sys.exit(_cli_main())
