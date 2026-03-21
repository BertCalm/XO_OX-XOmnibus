#!/usr/bin/env python3
"""
xpn_profile_validator.py — Profile-Driven QA Validator for XO_OX Pack Builds

Loads a pack profile YAML from profiles/ and validates an actual pack's DNA
data against every phenotype assertion defined in that profile.

Assertion DSL (simple, no eval()):
  Supported patterns (parsed via regex):

  DNA checks:
    "{voice} DNA {dimension} {op} {value}"
    "pack average DNA {dimension} {op} {value}"

  Render/duration checks:
    "{voice} render duration {op} {value}ms"

  Velocity-layer checks:
    "velocity_layer_{n} peak {op} {value}dBFS"
    "velocity_layer_{n}_peak - velocity_layer_{m}_peak {op} {value}dB"

  Choke-group checks:
    "mute_group({voice_a}, {voice_b}) == same_group"

  Operators: > | < | >= | <= | == | !=

Pack DNA data dict format (passed to validate()):
  {
      "voices": {
          "kick":        {"warmth": 0.72, "brightness": 0.38, ...},
          "snare":       {"brightness": 0.55, "aggression": 0.60, ...},
          "closed_hat":  {"brightness": 0.70, ...},
      },
      "pack_average": {"warmth": 0.65, "space": 0.12, ...},
      "render_durations_ms": {"closed_hat": 180, "kick": 400, ...},
      "velocity_peaks_dbfs": {
          "layer_1": -28.0,
          "layer_2": -20.0,
          "layer_3": -12.0,
          "layer_4": -6.0,
      },
      "choke_groups": {
          "closed_hat": "group_1",
          "open_hat": "group_1",
      },
  }

Usage:
  python xpn_profile_validator.py --profile boom-bap-percussion --pack-dir builds/mpce-perc-001/
  python xpn_profile_validator.py --profile boom-bap-percussion --dry-run
  python xpn_profile_validator.py --profile boom-bap-percussion --pack-dir builds/mpce-perc-001/ --json
  python xpn_profile_validator.py --profile trap-drums --dry-run
"""

from __future__ import annotations

import argparse
import json
import re
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

from xpn_qa_decision_log import QADecisionLog

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------

_TOOLS_DIR = Path(__file__).parent
_PROFILES_DIR = _TOOLS_DIR.parent / "profiles"
_BUILDS_DIR = _TOOLS_DIR / "builds"

# ---------------------------------------------------------------------------
# DSL parsing
# ---------------------------------------------------------------------------

# Regex patterns for each assertion form.  Ordered from most-specific to
# least-specific so we try the right pattern first.

_OPS = r"(?P<op>[><!]=?|==)"

# "pack average DNA {dim} {op} {value}"
_RE_PACK_AVG = re.compile(
    r"^pack\s+average\s+DNA\s+(?P<dim>\w+)\s+" + _OPS + r"\s+(?P<value>[\d.]+)$",
    re.IGNORECASE,
)

# "{voice} DNA {dim} {op} {value}"
_RE_VOICE_DNA = re.compile(
    r"^(?P<voice>\w+)\s+DNA\s+(?P<dim>\w+)\s+" + _OPS + r"\s+(?P<value>[\d.]+)$",
    re.IGNORECASE,
)

# "{voice} render duration {op} {value}ms"
_RE_RENDER_DUR = re.compile(
    r"^(?P<voice>\w+)\s+render\s+duration\s+" + _OPS + r"\s+(?P<value>[\d.]+)ms$",
    re.IGNORECASE,
)

# "velocity_layer_{n} peak {op} {value}dBFS"
_RE_VEL_PEAK = re.compile(
    r"^velocity_layer_(?P<n>\d+)\s+peak\s+" + _OPS + r"\s+(?P<value>-?[\d.]+)dBFS$",
    re.IGNORECASE,
)

# "velocity_layer_{n}_peak - velocity_layer_{m}_peak {op} {value}dB"
_RE_VEL_RANGE = re.compile(
    r"^velocity_layer_(?P<n>\d+)_peak\s+-\s+velocity_layer_(?P<m>\d+)_peak\s+"
    + _OPS
    + r"\s+(?P<value>[\d.]+)dB$",
    re.IGNORECASE,
)

# "all samples peak {op} {value}dBFS"
_RE_ALL_PEAK = re.compile(
    r"^all\s+samples\s+peak\s+" + _OPS + r"\s+(?P<value>-?[\d.]+)dBFS$",
    re.IGNORECASE,
)

# "mute_group({voice_a}, {voice_b}) == same_group"
_RE_CHOKE = re.compile(
    r"^mute_group\(\s*(?P<voice_a>\w+)\s*,\s*(?P<voice_b>\w+)\s*\)\s*==\s*same_group$",
    re.IGNORECASE,
)

# Compound "AND" assertions (e.g. "kick DNA warmth > 0.5 AND kick render has energy below 100Hz")
# We handle the DNA portion only; the spectral portion is marked as "needs_audio".
_RE_COMPOUND_AND = re.compile(r"\bAND\b", re.IGNORECASE)


def _apply_op(lhs: float, op: str, rhs: float) -> bool:
    """Evaluate a binary comparison operator."""
    if op in (">",):
        return lhs > rhs
    if op in ("<",):
        return lhs < rhs
    if op in (">=",):
        return lhs >= rhs
    if op in ("<=",):
        return lhs <= rhs
    if op in ("==",):
        return lhs == rhs
    if op in ("!=",):
        return lhs != rhs
    return False


class _ParseError(Exception):
    """Raised when an assertion check string cannot be parsed."""


def _parse_and_evaluate(check: str, pack_dna: dict) -> tuple[bool, str]:
    """
    Parse a single assertion ``check`` string and evaluate it against ``pack_dna``.

    Returns
    -------
    tuple[bool, str]
        ``(passed, detail_message)`` where ``detail_message`` describes the
        actual value found (or the reason it could not be evaluated).

    Raises
    ------
    _ParseError
        When the check string doesn't match any supported DSL pattern.
    """
    check = check.strip()

    # Compound AND — evaluate left clause only if it's a DNA clause;
    # the right clause often requires audio analysis.
    if _RE_COMPOUND_AND.search(check):
        left_clause = _RE_COMPOUND_AND.split(check)[0].strip()
        passed, detail = _parse_and_evaluate(left_clause, pack_dna)
        return passed, detail + " [compound AND — right clause requires audio analysis]"

    # --- pack average DNA ---
    m = _RE_PACK_AVG.match(check)
    if m:
        dim = m.group("dim")
        op = m.group("op")
        threshold = float(m.group("value"))
        avg_dna: dict = pack_dna.get("pack_average", {})
        if dim not in avg_dna:
            return False, f"pack_average.{dim} not present in DNA data"
        actual = float(avg_dna[dim])
        passed = _apply_op(actual, op, threshold)
        return passed, f"pack average {dim}={actual:.3f} {op} {threshold}"

    # --- voice DNA ---
    m = _RE_VOICE_DNA.match(check)
    if m:
        voice = m.group("voice")
        dim = m.group("dim")
        op = m.group("op")
        threshold = float(m.group("value"))
        voices: dict = pack_dna.get("voices", {})
        if voice not in voices:
            return False, f"voice '{voice}' not present in DNA data"
        voice_dna = voices[voice]
        if dim not in voice_dna:
            return False, f"voice '{voice}' has no DNA dimension '{dim}'"
        actual = float(voice_dna[dim])
        passed = _apply_op(actual, op, threshold)
        return passed, f"{voice} {dim}={actual:.3f} {op} {threshold}"

    # --- render duration ---
    m = _RE_RENDER_DUR.match(check)
    if m:
        voice = m.group("voice")
        op = m.group("op")
        threshold = float(m.group("value"))
        durations: dict = pack_dna.get("render_durations_ms", {})
        if voice not in durations:
            return False, f"render_durations_ms has no entry for '{voice}'"
        actual = float(durations[voice])
        passed = _apply_op(actual, op, threshold)
        return passed, f"{voice} render duration={actual:.0f}ms {op} {threshold:.0f}ms"

    # --- velocity layer peak ---
    m = _RE_VEL_PEAK.match(check)
    if m:
        n = m.group("n")
        op = m.group("op")
        threshold = float(m.group("value"))
        vel_peaks: dict = pack_dna.get("velocity_peaks_dbfs", {})
        key = f"layer_{n}"
        if key not in vel_peaks:
            return False, f"velocity_peaks_dbfs has no entry for '{key}'"
        actual = float(vel_peaks[key])
        passed = _apply_op(actual, op, threshold)
        return passed, f"velocity layer {n} peak={actual:.1f}dBFS {op} {threshold:.1f}dBFS"

    # --- velocity range (layer_n_peak - layer_m_peak) ---
    m = _RE_VEL_RANGE.match(check)
    if m:
        n = m.group("n")
        mm = m.group("m")
        op = m.group("op")
        threshold = float(m.group("value"))
        vel_peaks = pack_dna.get("velocity_peaks_dbfs", {})
        key_n = f"layer_{n}"
        key_m = f"layer_{mm}"
        if key_n not in vel_peaks:
            return False, f"velocity_peaks_dbfs has no entry for '{key_n}'"
        if key_m not in vel_peaks:
            return False, f"velocity_peaks_dbfs has no entry for '{key_m}'"
        actual = abs(float(vel_peaks[key_n]) - float(vel_peaks[key_m]))
        passed = _apply_op(actual, op, threshold)
        return passed, f"layer {n} - layer {mm} peak range={actual:.1f}dB {op} {threshold:.1f}dB"

    # --- all samples peak ---
    m = _RE_ALL_PEAK.match(check)
    if m:
        op = m.group("op")
        threshold = float(m.group("value"))
        vel_peaks = pack_dna.get("velocity_peaks_dbfs", {})
        if not vel_peaks:
            return False, "velocity_peaks_dbfs is empty — cannot evaluate all-samples peak"
        worst = max(float(v) for v in vel_peaks.values())
        passed = _apply_op(worst, op, threshold)
        return passed, f"all-samples worst peak={worst:.1f}dBFS {op} {threshold:.1f}dBFS"

    # --- choke group ---
    m = _RE_CHOKE.match(check)
    if m:
        va = m.group("voice_a")
        vb = m.group("voice_b")
        choke: dict = pack_dna.get("choke_groups", {})
        if va not in choke:
            return False, f"'{va}' not found in choke_groups"
        if vb not in choke:
            return False, f"'{vb}' not found in choke_groups"
        passed = choke[va] == choke[vb]
        return (
            passed,
            f"{va}={choke[va]} {'==' if passed else '!='} {vb}={choke[vb]}",
        )

    raise _ParseError(f"Unrecognised assertion pattern: {check!r}")


# ---------------------------------------------------------------------------
# Profile loader
# ---------------------------------------------------------------------------


def load_profile(profile_id: str, profiles_dir: Optional[Path] = None) -> dict:
    """
    Load a pack profile YAML by its ``profile_id``.

    Parameters
    ----------
    profile_id:
        The ``profile_id`` value from the YAML header, which also forms the
        filename stem (e.g. ``"boom-bap-percussion"`` → ``boom-bap-percussion.yaml``).
    profiles_dir:
        Override the default ``profiles/`` directory.

    Returns
    -------
    dict
        Parsed YAML content.

    Raises
    ------
    FileNotFoundError
        When the profile YAML does not exist.
    """
    root = Path(profiles_dir) if profiles_dir else _PROFILES_DIR
    path = root / f"{profile_id}.yaml"
    if not path.exists():
        available = sorted(p.stem for p in root.glob("*.yaml"))
        raise FileNotFoundError(
            f"Profile '{profile_id}' not found at {path}.\n"
            f"Available profiles: {available}"
        )
    with path.open(encoding="utf-8") as fh:
        return yaml.safe_load(fh)


# ---------------------------------------------------------------------------
# Validator
# ---------------------------------------------------------------------------


class ProfileValidator:
    """
    Validates pack DNA data against the ``phenotype.assertions`` in a profile.

    Parameters
    ----------
    profile_id:
        The profile to load (e.g. ``"boom-bap-percussion"``).
    pack_id:
        The build identifier for logging (e.g. ``"mpce-perc-001"``).
        If ``None``, decision logging is skipped.
    profiles_dir:
        Override the default ``profiles/`` directory.
    builds_root:
        Override the default ``Tools/builds/`` root for decision logs.
    """

    def __init__(
        self,
        profile_id: str,
        pack_id: Optional[str] = None,
        profiles_dir: Optional[Path] = None,
        builds_root: Optional[Path] = None,
    ) -> None:
        self.profile_id = profile_id
        self.pack_id = pack_id
        self.profile = load_profile(profile_id, profiles_dir=profiles_dir)
        self._log: Optional[QADecisionLog] = None

        if pack_id:
            self._log = QADecisionLog(
                pack_id=pack_id,
                profile_id=profile_id,
                builds_root=builds_root,
            )

    @property
    def assertions(self) -> list[dict]:
        """Return the raw assertion list from the profile YAML."""
        return self.profile.get("phenotype", {}).get("assertions", [])

    def dry_run(self) -> None:
        """
        Print all assertions for this profile without evaluating any data.

        Useful to preview what will be checked before running a build.
        """
        print(
            f"Profile: {self.profile.get('display_name', self.profile_id)} "
            f"(v{self.profile.get('profile_version', '?')})"
        )
        print(f"  {len(self.assertions)} phenotype assertion(s):")
        for i, assertion in enumerate(self.assertions, start=1):
            sev = assertion.get("severity", "info").upper()
            name = assertion.get("name", f"assertion_{i}")
            check = assertion.get("check", "(no check)")
            desc = assertion.get("description", "")
            print(f"  [{i:2d}] [{sev:8s}] {name}")
            print(f"        check: {check}")
            if desc:
                print(f"        desc:  {desc}")

    def validate(self, pack_dna: dict) -> dict:
        """
        Evaluate all phenotype assertions against ``pack_dna``.

        Parameters
        ----------
        pack_dna:
            Dictionary with actual pack measurements.  See module docstring for
            the expected shape.

        Returns
        -------
        dict
            Validation report with keys:

            ``passed``
                List of assertion result dicts that passed.
            ``failed``
                List of assertion result dicts that failed (critical severity).
            ``warnings``
                List of assertion result dicts for non-critical failures.
            ``errors``
                List of assertion result dicts that could not be evaluated
                (e.g. missing data, unsupported DSL pattern).
            ``overall``
                ``"PASS"`` if no critical failures; ``"FAIL"`` otherwise.
            ``score``
                Simple score: (passed / total) × 100, rounded to 1 dp.
        """
        passed: list[dict] = []
        failed: list[dict] = []
        warnings: list[dict] = []
        errors: list[dict] = []

        for assertion in self.assertions:
            name = assertion.get("name", "unnamed")
            check = assertion.get("check", "")
            severity = assertion.get("severity", "warning").lower()
            description = assertion.get("description", "")

            result: dict[str, Any] = {
                "name": name,
                "check": check,
                "severity": severity,
                "description": description,
            }

            if not check:
                result["outcome"] = "error"
                result["detail"] = "Empty check string — skipped"
                errors.append(result)
                continue

            try:
                ok, detail = _parse_and_evaluate(check, pack_dna)
                result["outcome"] = "passed" if ok else "failed"
                result["detail"] = detail

                if ok:
                    passed.append(result)
                    if self._log is not None:
                        self._log.log_decision(
                            stage="validate",
                            preset_name=f"[{name}]",
                            action="accepted",
                            reason=f"Assertion passed: {detail}",
                        )
                else:
                    if severity == "critical":
                        failed.append(result)
                    else:
                        warnings.append(result)

                    if self._log is not None:
                        self._log.log_decision(
                            stage="validate",
                            preset_name=f"[{name}]",
                            action="rejected",
                            reason=f"Assertion failed ({severity}): {detail}",
                        )

            except _ParseError as exc:
                result["outcome"] = "error"
                result["detail"] = str(exc)
                errors.append(result)
                if self._log is not None:
                    self._log.log_decision(
                        stage="validate",
                        preset_name=f"[{name}]",
                        action="rejected",
                        reason=f"DSL parse error — {exc}",
                    )

        total = len(passed) + len(failed) + len(warnings) + len(errors)
        score = round((len(passed) / total * 100) if total > 0 else 0.0, 1)
        overall = "FAIL" if failed else "PASS"

        if self._log is not None:
            self._log.save()

        return {
            "profile_id": self.profile_id,
            "pack_id": self.pack_id,
            "passed": passed,
            "failed": failed,
            "warnings": warnings,
            "errors": errors,
            "overall": overall,
            "score": score,
        }


# ---------------------------------------------------------------------------
# CLI helpers
# ---------------------------------------------------------------------------


def _print_report(report: dict) -> None:
    """Print a human-readable validation report to stdout."""
    overall = report.get("overall", "UNKNOWN")
    score = report.get("score", 0.0)
    passed = report.get("passed", [])
    failed = report.get("failed", [])
    warnings = report.get("warnings", [])
    errors = report.get("errors", [])

    print(
        f"Profile Validation Report — {report.get('profile_id', '?')} "
        f"on pack {report.get('pack_id', 'n/a')}"
    )
    print(f"  Overall : {overall}   Score : {score}%")
    print(
        f"  Passed  : {len(passed)}  |  Failed (critical) : {len(failed)}  "
        f"|  Warnings : {len(warnings)}  |  Errors : {len(errors)}"
    )

    if failed:
        print("\n  FAILED (critical):")
        for r in failed:
            print(f"    [FAIL] {r['name']} — {r['detail']}")
            if r.get("description"):
                print(f"           {r['description']}")

    if warnings:
        print("\n  WARNINGS:")
        for r in warnings:
            print(f"    [WARN] {r['name']} — {r['detail']}")
            if r.get("description"):
                print(f"           {r['description']}")

    if errors:
        print("\n  ERRORS (could not evaluate):")
        for r in errors:
            print(f"    [ERR ] {r['name']} — {r['detail']}")

    if passed:
        print("\n  Passed:")
        for r in passed:
            print(f"    [OK  ] {r['name']} — {r['detail']}")


def _load_pack_dna(pack_dir: Path) -> dict:
    """
    Load pack DNA data from ``pack_dir/pack_dna.json``.

    Falls back gracefully if the file is missing, returning an empty dict so
    the validator can report which assertions could not be evaluated.
    """
    dna_path = pack_dir / "pack_dna.json"
    if not dna_path.exists():
        print(
            f"WARNING: {dna_path} not found — assertions will be evaluated "
            f"against an empty DNA dict (most will report missing data).",
            file=sys.stderr,
        )
        return {}
    return json.loads(dna_path.read_text(encoding="utf-8"))


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="xpn_profile_validator — Validate pack outputs against a profile's phenotype assertions",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--profile",
        required=True,
        metavar="PROFILE_ID",
        help="Profile ID to load from profiles/ (e.g. boom-bap-percussion)",
    )

    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument(
        "--pack-dir",
        metavar="DIR",
        help="Path to the built pack directory (must contain pack_dna.json)",
    )
    mode.add_argument(
        "--dry-run",
        action="store_true",
        help="Print all assertions without evaluating any data",
    )

    parser.add_argument(
        "--pack-id",
        default=None,
        metavar="ID",
        help="Override pack ID for decision log (defaults to pack-dir basename)",
    )
    parser.add_argument(
        "--profiles-dir",
        default=None,
        metavar="DIR",
        help="Override the default profiles/ directory",
    )
    parser.add_argument(
        "--builds-root",
        default=None,
        metavar="DIR",
        help="Override the default Tools/builds/ root for decision logs",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Emit the validation report as JSON",
    )
    return parser


def main(argv: Optional[list[str]] = None) -> int:
    parser = _build_parser()
    args = parser.parse_args(argv)

    profiles_dir = Path(args.profiles_dir) if args.profiles_dir else None
    builds_root = Path(args.builds_root) if args.builds_root else None

    # --- dry-run mode: no data needed ---
    if args.dry_run:
        try:
            validator = ProfileValidator(
                profile_id=args.profile,
                profiles_dir=profiles_dir,
            )
        except FileNotFoundError as exc:
            print(f"ERROR: {exc}", file=sys.stderr)
            return 1
        validator.dry_run()
        return 0

    # --- validation mode ---
    pack_dir = Path(args.pack_dir)
    if not pack_dir.is_dir():
        print(f"ERROR: pack-dir '{pack_dir}' is not a directory.", file=sys.stderr)
        return 1

    pack_id = args.pack_id or pack_dir.name

    try:
        validator = ProfileValidator(
            profile_id=args.profile,
            pack_id=pack_id,
            profiles_dir=profiles_dir,
            builds_root=builds_root,
        )
    except FileNotFoundError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    pack_dna = _load_pack_dna(pack_dir)
    report = validator.validate(pack_dna)

    if args.json:
        print(json.dumps(report, indent=2, ensure_ascii=False))
    else:
        _print_report(report)

    return 0 if report.get("overall") == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
