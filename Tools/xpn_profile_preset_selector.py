#!/usr/bin/env python3
"""
xpn_profile_preset_selector.py — SELECT stage of the .oxbuild compiler.

Given a pack profile YAML and a preset directory, selects the best-matching
presets for the profile's DNA genotype using:

  1. DNA range filtering  — reject presets outside profile dna_ranges
  2. Mood filtering       — optionally restrict to specific mood directories
  3. Engine matching      — only presets for the requested engine
  4. Distance ranking     — Euclidean distance in 6D DNA space from the
                            profile's center point
  5. Diversity maximization — farthest-point sampling to spread the selection
                              across DNA space

DNA dimensions (all 0.0-1.0):
    brightness, warmth, movement, density, space, aggression

Output is a list of dicts::

    {
        "name": str,
        "path": str,           # absolute path to .xometa file
        "dna": dict,           # {brightness: float, ...}
        "distance_from_center": float,  # Euclidean distance from profile center
    }

Usage:
    # Select presets matching boom-bap profile for ONSET engine
    python xpn_profile_preset_selector.py \\
        --profile boom-bap-percussion \\
        --engine ONSET \\
        --count 64 \\
        --presets-dir ../Presets/XOlokun/

    # With mood filter
    python xpn_profile_preset_selector.py \\
        --profile boom-bap-percussion \\
        --engine ONSET \\
        --mood Foundation Kinetic \\
        --count 64

    # Output as JSON for piping to other tools
    python xpn_profile_preset_selector.py \\
        --profile boom-bap-percussion \\
        --engine ONSET \\
        --json
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path
from typing import Any, Optional

try:
    import yaml
except ImportError:
    print(
        "ERROR: PyYAML is required. Install with: pip install pyyaml",
        file=sys.stderr,
    )
    sys.exit(1)

# ---------------------------------------------------------------------------
# Optional QA decision log integration
# ---------------------------------------------------------------------------

try:
    # Import from the same Tools/ directory
    _tools_dir = Path(__file__).parent
    sys.path.insert(0, str(_tools_dir))
    from xpn_qa_decision_log import QADecisionLog

    _QA_LOG_AVAILABLE = True
except ImportError:
    _QA_LOG_AVAILABLE = False
    QADecisionLog = None  # type: ignore[assignment,misc]

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DNA_DIMENSIONS = ("brightness", "warmth", "movement", "density", "space", "aggression")

# Default profiles directory — sibling of the repo root Tools/ folder
_DEFAULT_PROFILES_DIR = Path(__file__).parent.parent / "profiles"

# Default presets directory
_DEFAULT_PRESETS_DIR = Path(__file__).parent.parent / "Presets" / "XOlokun"


# ---------------------------------------------------------------------------
# Data classes (plain dicts with helpers to avoid external deps)
# ---------------------------------------------------------------------------


class PresetCandidate:
    """Holds a parsed preset ready for selection decisions."""

    __slots__ = ("name", "path", "dna", "mood", "engines", "distance_from_center")

    def __init__(
        self,
        name: str,
        path: Path,
        dna: dict[str, float],
        mood: str,
        engines: list[str],
    ) -> None:
        self.name = name
        self.path = path
        self.dna = dna
        self.mood = mood
        self.engines = engines
        self.distance_from_center: float = 0.0

    def to_dict(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "path": str(self.path),
            "dna": self.dna,
            "distance_from_center": round(self.distance_from_center, 6),
        }

    def __repr__(self) -> str:
        return (
            f"PresetCandidate(name={self.name!r}, "
            f"mood={self.mood!r}, "
            f"dist={self.distance_from_center:.4f})"
        )


# ---------------------------------------------------------------------------
# Profile loading
# ---------------------------------------------------------------------------


def load_profile(profile_id: str, profiles_dir: Path) -> dict[str, Any]:
    """
    Load a pack profile YAML by its profile_id.

    Looks for ``{profile_id}.yaml`` inside *profiles_dir*.

    Parameters
    ----------
    profile_id:
        Profile identifier without extension (e.g. ``"boom-bap-percussion"``).
    profiles_dir:
        Directory containing ``*.yaml`` profile files.

    Returns
    -------
    dict
        Parsed YAML content.

    Raises
    ------
    FileNotFoundError
        If the expected YAML file does not exist.
    ValueError
        If the profile is missing required ``genotype.dna_ranges`` data.
    """
    profile_path = profiles_dir / f"{profile_id}.yaml"
    if not profile_path.exists():
        raise FileNotFoundError(
            f"Profile not found: {profile_path}\n"
            f"Available profiles: "
            + ", ".join(p.stem for p in profiles_dir.glob("*.yaml"))
        )

    with profile_path.open("r", encoding="utf-8") as fh:
        profile = yaml.safe_load(fh)

    # Validate required fields
    genotype = profile.get("genotype") or {}
    dna_ranges = genotype.get("dna_ranges") or {}
    if not dna_ranges:
        raise ValueError(
            f"Profile '{profile_id}' is missing genotype.dna_ranges — "
            "cannot perform DNA-based preset selection."
        )

    return profile


def extract_dna_ranges(profile: dict[str, Any]) -> dict[str, dict[str, float]]:
    """
    Extract and validate dna_ranges from a loaded profile.

    Returns a dict mapping each dimension name to its ``{min, max, center}`` spec.
    Missing dimensions are silently skipped (no filter applied for that dimension).
    """
    raw_ranges = profile["genotype"]["dna_ranges"]
    validated: dict[str, dict[str, float]] = {}
    for dim in DNA_DIMENSIONS:
        spec = raw_ranges.get(dim)
        if spec is None:
            continue  # dimension not constrained by this profile
        validated[dim] = {
            "min": float(spec.get("min", 0.0)),
            "max": float(spec.get("max", 1.0)),
            "center": float(spec.get("center", 0.5)),
        }
    return validated


def extract_dna_center(dna_ranges: dict[str, dict[str, float]]) -> dict[str, float]:
    """Return a dict of dimension -> center value for all constrained dimensions."""
    return {dim: spec["center"] for dim, spec in dna_ranges.items()}


# ---------------------------------------------------------------------------
# Preset scanning
# ---------------------------------------------------------------------------


def _parse_engines(raw: dict[str, Any]) -> list[str]:
    """
    Extract engine names from an xometa dict, checking both ``"engine"``
    (scalar) and ``"engines"`` (array) fields.  Returns lowercased names.
    """
    engines: list[str] = []
    if "engines" in raw and isinstance(raw["engines"], list):
        engines.extend(str(e).lower() for e in raw["engines"])
    if "engine" in raw and raw["engine"]:
        engines.append(str(raw["engine"]).lower())
    return list(dict.fromkeys(engines))  # deduplicate, preserve order


def _parse_dna(raw: dict[str, Any]) -> Optional[dict[str, float]]:
    """
    Extract the 6D DNA dict from an xometa file.

    Supports both ``"sonic_dna"`` and ``"dna"`` top-level keys.
    Returns ``None`` if neither key is present or if the value is not a dict.
    """
    dna_raw = raw.get("sonic_dna") or raw.get("dna")
    if not isinstance(dna_raw, dict):
        return None
    try:
        return {dim: float(dna_raw[dim]) for dim in DNA_DIMENSIONS if dim in dna_raw}
    except (TypeError, ValueError):
        return None


def scan_presets(
    presets_dir: Path,
    engine: str,
    mood_filter: Optional[list[str]] = None,
) -> list[PresetCandidate]:
    """
    Recursively scan *presets_dir* for ``.xometa`` files matching the engine.

    Parameters
    ----------
    presets_dir:
        Root of the XOlokun preset library (e.g. ``Presets/XOlokun/``).
        Subdirectory names are treated as mood names.
    engine:
        Engine name to match (case-insensitive, e.g. ``"ONSET"``).
    mood_filter:
        If provided, only scan subdirectories whose names are in this list
        (case-insensitive match).  Pass ``None`` to scan all moods.

    Returns
    -------
    list[PresetCandidate]
        All parseable presets that match the engine (and mood, if filtered).
    """
    engine_lower = engine.lower()
    mood_set: Optional[set[str]] = (
        {m.lower() for m in mood_filter} if mood_filter else None
    )

    candidates: list[PresetCandidate] = []
    seen_paths: set[Path] = set()

    for xometa_path in sorted(presets_dir.rglob("*.xometa")):
        if xometa_path in seen_paths:
            continue
        seen_paths.add(xometa_path)

        # Derive mood from the nearest parent directory under presets_dir
        try:
            rel = xometa_path.relative_to(presets_dir)
        except ValueError:
            continue

        # The first path component (after presets_dir) is the mood folder
        mood_dir = rel.parts[0] if len(rel.parts) > 1 else ""

        # Apply mood filter
        if mood_set is not None and mood_dir.lower() not in mood_set:
            continue

        # Parse JSON
        try:
            raw = json.loads(xometa_path.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError):
            continue

        # Engine match
        preset_engines = _parse_engines(raw)
        if engine_lower not in preset_engines:
            continue

        # DNA
        dna = _parse_dna(raw)
        if not dna:
            continue

        # Prefer the mood from the JSON; fall back to directory name
        mood = raw.get("mood") or mood_dir or "Unknown"
        name = raw.get("name") or xometa_path.stem

        candidates.append(
            PresetCandidate(
                name=name,
                path=xometa_path.resolve(),
                dna=dna,
                mood=mood,
                engines=preset_engines,
            )
        )

    return candidates


# ---------------------------------------------------------------------------
# DNA filtering and distance
# ---------------------------------------------------------------------------


def _euclidean_distance(
    dna: dict[str, float],
    center: dict[str, float],
) -> float:
    """
    Compute Euclidean distance in 6D DNA space between a preset's DNA and the
    profile's center point.

    Only dimensions present in *center* are included.  Dimensions absent from
    *dna* are treated as 0.0 (which will penalise presets with missing DNA).
    """
    sq_sum = 0.0
    for dim, c_val in center.items():
        p_val = dna.get(dim, 0.0)
        sq_sum += (p_val - c_val) ** 2
    return math.sqrt(sq_sum)


def filter_by_dna_ranges(
    candidates: list[PresetCandidate],
    dna_ranges: dict[str, dict[str, float]],
    qa_log: Optional["QADecisionLog"] = None,  # noqa: F821
    pack_id: str = "unknown",
    profile_id: str = "unknown",
) -> list[PresetCandidate]:
    """
    Reject presets whose DNA falls outside any of the profile's dna_ranges.

    Parameters
    ----------
    candidates:
        Full list of engine-matched presets.
    dna_ranges:
        Validated ranges dict from :func:`extract_dna_ranges`.
    qa_log:
        Optional ``QADecisionLog`` instance. When provided, every reject and
        accept decision is recorded to the log.
    pack_id / profile_id:
        Used only for the QA log reason strings.

    Returns
    -------
    list[PresetCandidate]
        Only the presets whose DNA is within all constrained ranges.
    """
    accepted: list[PresetCandidate] = []

    for preset in candidates:
        reject_reason: Optional[str] = None

        for dim, spec in dna_ranges.items():
            val = preset.dna.get(dim)
            if val is None:
                reject_reason = f"DNA dimension '{dim}' missing from preset"
                break
            if val < spec["min"]:
                reject_reason = (
                    f"DNA {dim} {val:.3f} below profile min {spec['min']:.3f}"
                )
                break
            if val > spec["max"]:
                reject_reason = (
                    f"DNA {dim} {val:.3f} above profile max {spec['max']:.3f}"
                )
                break

        if reject_reason:
            if qa_log is not None:
                qa_log.log_decision(
                    stage="select",
                    preset_name=preset.name,
                    action="rejected",
                    reason=reject_reason,
                )
        else:
            accepted.append(preset)
            if qa_log is not None:
                qa_log.log_decision(
                    stage="select",
                    preset_name=preset.name,
                    action="accepted",
                    reason="All DNA dimensions within profile ranges",
                )

    return accepted


def rank_by_distance(
    candidates: list[PresetCandidate],
    center: dict[str, float],
) -> list[PresetCandidate]:
    """
    Compute each candidate's Euclidean distance from *center* and return the
    list sorted from nearest to farthest.

    Mutates ``candidate.distance_from_center`` in place.
    """
    for preset in candidates:
        preset.distance_from_center = _euclidean_distance(preset.dna, center)
    return sorted(candidates, key=lambda p: p.distance_from_center)


# ---------------------------------------------------------------------------
# Diversity maximization — greedy farthest-point sampling
# ---------------------------------------------------------------------------


def farthest_point_sample(
    candidates: list[PresetCandidate],
    n: int,
    seed_index: int = 0,
) -> list[PresetCandidate]:
    """
    Select *n* presets from *candidates* using greedy farthest-point sampling
    to maximize diversity in 6D DNA space.

    The algorithm:
      1. Start with the preset at *seed_index* (default: the one closest to the
         profile center, i.e. index 0 of a distance-ranked list).
      2. Repeatedly pick the candidate farthest from *any* already-selected
         preset.

    This guarantees the selected set is as spread out as possible in DNA space,
    avoiding clusters of sonically similar presets.

    Parameters
    ----------
    candidates:
        Filtered and distance-ranked candidates.  Must not be empty.
    n:
        Number of presets to select.  Clamped to len(candidates).
    seed_index:
        Index in *candidates* to use as the first selected preset.

    Returns
    -------
    list[PresetCandidate]
        Selected presets in the order they were picked (seed first).
    """
    if not candidates:
        return []
    n = min(n, len(candidates))
    if n == len(candidates):
        return list(candidates)

    # Use only dimensions present in all candidates for distance computation
    all_dims = list(DNA_DIMENSIONS)

    def dist(a: PresetCandidate, b: PresetCandidate) -> float:
        return math.sqrt(
            sum((a.dna.get(d, 0.0) - b.dna.get(d, 0.0)) ** 2 for d in all_dims)
        )

    seed_index = max(0, min(seed_index, len(candidates) - 1))
    selected: list[PresetCandidate] = [candidates[seed_index]]
    remaining = [c for i, c in enumerate(candidates) if i != seed_index]

    # Track minimum distance from each remaining candidate to any selected one
    min_dists: list[float] = [dist(c, selected[0]) for c in remaining]

    while len(selected) < n and remaining:
        # Pick the candidate with the largest min-distance to the selected set
        farthest_idx = max(range(len(remaining)), key=lambda i: min_dists[i])
        chosen = remaining.pop(farthest_idx)
        min_dists.pop(farthest_idx)
        selected.append(chosen)

        # Update min_dists for the remaining candidates
        for i, candidate in enumerate(remaining):
            d = dist(candidate, chosen)
            if d < min_dists[i]:
                min_dists[i] = d

    return selected


# ---------------------------------------------------------------------------
# Top-level selector
# ---------------------------------------------------------------------------


def select_presets(
    profile_id: str,
    engine: str,
    count: int = 64,
    mood_filter: Optional[list[str]] = None,
    profiles_dir: Optional[Path] = None,
    presets_dir: Optional[Path] = None,
    qa_log: Optional["QADecisionLog"] = None,  # noqa: F821
    verbose: bool = False,
) -> list[dict[str, Any]]:
    """
    Full SELECT pipeline: load profile → scan presets → filter → rank → diversify.

    Parameters
    ----------
    profile_id:
        Profile YAML stem (e.g. ``"boom-bap-percussion"``).
    engine:
        Engine name to match (case-insensitive, e.g. ``"ONSET"``).
    count:
        Target number of presets to return.  The actual count may be lower if
        fewer presets pass the DNA filter.
    mood_filter:
        Optional list of mood names to restrict the search to.
    profiles_dir:
        Override the default ``profiles/`` directory.
    presets_dir:
        Override the default ``Presets/XOlokun/`` directory.
    qa_log:
        Optional :class:`~xpn_qa_decision_log.QADecisionLog` instance for
        recording accept/reject decisions.
    verbose:
        When ``True``, print progress information to stderr.

    Returns
    -------
    list[dict]
        Each dict has keys: ``name``, ``path``, ``dna``, ``distance_from_center``.
        Ordered by diversity (farthest-point sampling order), not by distance.
    """
    profiles_dir = profiles_dir or _DEFAULT_PROFILES_DIR
    presets_dir = presets_dir or _DEFAULT_PRESETS_DIR

    def _log(msg: str) -> None:
        if verbose:
            print(f"[selector] {msg}", file=sys.stderr)

    # 1. Load profile
    _log(f"Loading profile '{profile_id}' from {profiles_dir}")
    profile = load_profile(profile_id, Path(profiles_dir))
    dna_ranges = extract_dna_ranges(profile)
    center = extract_dna_center(dna_ranges)

    _log(f"Profile center: {center}")
    _log(f"DNA ranges: {json.dumps(dna_ranges, indent=2)}")

    # 2. Scan presets
    _log(
        f"Scanning presets in {presets_dir} "
        f"(engine={engine}, mood_filter={mood_filter})"
    )
    all_candidates = scan_presets(
        presets_dir=Path(presets_dir),
        engine=engine,
        mood_filter=mood_filter,
    )
    _log(f"Found {len(all_candidates)} {engine} presets matching mood filter")

    # 3. Filter by DNA ranges
    filtered = filter_by_dna_ranges(
        candidates=all_candidates,
        dna_ranges=dna_ranges,
        qa_log=qa_log,
        pack_id=profile.get("profile_id", "unknown"),
        profile_id=profile_id,
    )
    _log(
        f"{len(filtered)} presets passed DNA filter "
        f"({len(all_candidates) - len(filtered)} rejected)"
    )

    if not filtered:
        _log("WARNING: No presets passed the DNA filter — returning empty list")
        return []

    # 4. Rank by distance from center (nearest first)
    ranked = rank_by_distance(filtered, center)
    _log(
        f"Top 5 nearest: "
        + ", ".join(f"{p.name} ({p.distance_from_center:.3f})" for p in ranked[:5])
    )

    # 5. Greedy farthest-point sampling for diversity
    # Seed with the nearest preset to the center (index 0 of ranked list)
    n_select = min(count, len(ranked))
    selected = farthest_point_sample(candidates=ranked, n=n_select, seed_index=0)
    _log(
        f"Selected {len(selected)} presets via farthest-point sampling "
        f"(target={count})"
    )

    return [p.to_dict() for p in selected]


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="xpn_profile_preset_selector",
        description=(
            "SELECT stage of the .oxbuild compiler.\n"
            "Selects XOlokun presets that match a pack profile's DNA genotype."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )

    parser.add_argument(
        "--profile",
        required=True,
        metavar="PROFILE_ID",
        help="Profile YAML stem (e.g. 'boom-bap-percussion'). "
        "Loaded from --profiles-dir.",
    )
    parser.add_argument(
        "--engine",
        required=True,
        metavar="ENGINE",
        help="Engine name to match (case-insensitive, e.g. 'ONSET').",
    )
    parser.add_argument(
        "--count",
        type=int,
        default=64,
        metavar="N",
        help="Target number of presets to select (default: 64).",
    )
    parser.add_argument(
        "--mood",
        nargs="+",
        metavar="MOOD",
        default=None,
        help="Restrict search to these mood directories (e.g. 'Foundation Kinetic'). "
        "Case-insensitive. Default: all moods.",
    )
    parser.add_argument(
        "--profiles-dir",
        default=None,
        metavar="DIR",
        help=f"Directory containing profile YAML files. Default: {_DEFAULT_PROFILES_DIR}",
    )
    parser.add_argument(
        "--presets-dir",
        default=None,
        metavar="DIR",
        help=f"Root of the XOlokun preset library. Default: {_DEFAULT_PRESETS_DIR}",
    )
    parser.add_argument(
        "--pack-id",
        default=None,
        metavar="ID",
        help="Pack build ID for the QA decision log (e.g. 'mpce-perc-001'). "
        "If omitted, QA logging is skipped.",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        dest="output_json",
        help="Output results as JSON (suitable for piping to other tools).",
    )
    parser.add_argument(
        "--verbose",
        "-v",
        action="store_true",
        help="Print progress information to stderr.",
    )

    return parser


def main(argv: Optional[list[str]] = None) -> int:
    parser = _build_parser()
    args = parser.parse_args(argv)

    profiles_dir = Path(args.profiles_dir) if args.profiles_dir else None
    presets_dir = Path(args.presets_dir) if args.presets_dir else None

    # Set up optional QA log
    qa_log = None
    if args.pack_id and _QA_LOG_AVAILABLE:
        qa_log = QADecisionLog(  # type: ignore[call-arg]
            pack_id=args.pack_id,
            profile_id=args.profile,
        )
    elif args.pack_id and not _QA_LOG_AVAILABLE:
        print(
            "WARNING: --pack-id provided but xpn_qa_decision_log.py not found. "
            "QA logging disabled.",
            file=sys.stderr,
        )

    try:
        results = select_presets(
            profile_id=args.profile,
            engine=args.engine,
            count=args.count,
            mood_filter=args.mood,
            profiles_dir=profiles_dir,
            presets_dir=presets_dir,
            qa_log=qa_log,
            verbose=args.verbose,
        )
    except (FileNotFoundError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    # Save QA log if populated
    if qa_log and len(qa_log):
        log_path = qa_log.save()
        if args.verbose:
            print(f"[selector] QA decision log saved to {log_path}", file=sys.stderr)
            qa_log.print_summary()

    if args.output_json:
        print(
            json.dumps(
                {
                    "profile": args.profile,
                    "engine": args.engine,
                    "count_requested": args.count,
                    "count_selected": len(results),
                    "mood_filter": args.mood,
                    "presets": results,
                },
                indent=2,
                ensure_ascii=False,
            )
        )
    else:
        print(
            f"Selected {len(results)} presets "
            f"(profile: {args.profile}, engine: {args.engine})"
        )
        for i, p in enumerate(results, 1):
            dna = p["dna"]
            dna_str = ", ".join(
                f"{d[0]}={dna.get(d, 0.0):.2f}" for d in DNA_DIMENSIONS
            )
            print(
                f"  {i:3d}. [{p['distance_from_center']:.4f}] "
                f"{p['name']!r:<35s} {dna_str}"
            )

    return 0


if __name__ == "__main__":
    sys.exit(main())
