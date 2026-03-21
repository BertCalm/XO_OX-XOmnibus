#!/usr/bin/env python3
"""
xpn_qa_decision_log.py — QA Substitution Decision Log for XO_OX Pack Builds

Records every accept/reject/substitute decision made during a pack build.
Generates the training dataset for the adaptive QA learning system (VQ 007).

Each log entry captures 8 fields:
  timestamp    — ISO 8601 UTC (e.g. "2026-03-21T14:30:00Z")
  pack_id      — build identifier matching the builds/ directory (e.g. "mpce-perc-001")
  profile_id   — the profile YAML driving this build (e.g. "boom-bap-percussion")
  stage        — pipeline stage: "select" | "validate" | "substitute"
  preset_name  — human-readable preset name being evaluated
  action       — outcome: "accepted" | "rejected" | "substituted"
  reason       — plain-language explanation (must reference the DNA dimension or rule)
  substitution — null when accepted/rejected; dict with original/replacement/dna_delta when substituted

Log files are saved to:
  builds/{pack_id}/qa_decisions.json

Usage (as library):
    from xpn_qa_decision_log import QADecisionLog

    log = QADecisionLog(pack_id="mpce-perc-001", profile_id="boom-bap-percussion")
    log.log_decision(
        stage="select",
        preset_name="808 Deep Sub",
        action="accepted",
        reason="All DNA dimensions within boom-bap-percussion ranges",
    )
    log.log_decision(
        stage="validate",
        preset_name="Bright Rim Click",
        action="substituted",
        reason="warmth 0.42 below profile min 0.50 — substituting warmer variant",
        substitution={
            "original": "Bright Rim Click",
            "replacement": "Warm Rim Knock",
            "dna_delta": {"warmth": +0.21, "brightness": -0.08},
        },
    )
    log.save()
    print(log.summary())

Usage (CLI):
    python xpn_qa_decision_log.py --pack-id mpce-perc-001 --summary
    python xpn_qa_decision_log.py --pack-id mpce-perc-001 --dump
"""

from __future__ import annotations

import argparse
import json
import sys
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

VALID_STAGES = {"select", "validate", "substitute"}
VALID_ACTIONS = {"accepted", "rejected", "substituted"}

# Default root for builds — relative to this file's parent (Tools/)
_DEFAULT_BUILDS_ROOT = Path(__file__).parent / "builds"


# ---------------------------------------------------------------------------
# QADecisionLog
# ---------------------------------------------------------------------------


class QADecisionLog:
    """
    Accumulates per-decision QA records during a pack build and persists them
    to ``builds/{pack_id}/qa_decisions.json``.

    Parameters
    ----------
    pack_id:
        Short build identifier used for the output directory name.
        Example: ``"mpce-perc-001"``.
    profile_id:
        The profile YAML that drove this build.
        Example: ``"boom-bap-percussion"``.
    builds_root:
        Parent directory that contains per-pack build folders.
        Defaults to ``Tools/builds/`` (sibling of this file).
    """

    def __init__(
        self,
        pack_id: str,
        profile_id: str,
        builds_root: Optional[Path] = None,
    ) -> None:
        self.pack_id = pack_id
        self.profile_id = profile_id
        self.builds_root = Path(builds_root) if builds_root else _DEFAULT_BUILDS_ROOT
        self._entries: list[dict] = []

    # ------------------------------------------------------------------
    # Core mutation
    # ------------------------------------------------------------------

    def log_decision(
        self,
        stage: str,
        preset_name: str,
        action: str,
        reason: str,
        substitution: Optional[dict] = None,
    ) -> dict:
        """
        Record one QA decision and append it to the in-memory log.

        Parameters
        ----------
        stage:
            Pipeline stage where the decision was made.
            Must be one of: ``"select"``, ``"validate"``, ``"substitute"``.
        preset_name:
            Human-readable name of the preset being evaluated.
        action:
            Outcome of the evaluation.
            Must be one of: ``"accepted"``, ``"rejected"``, ``"substituted"``.
        reason:
            Plain-language explanation.  Should reference the DNA dimension or
            rule that drove the decision.
            Example: ``"DNA warmth 0.42 below profile min 0.50"``.
        substitution:
            Required when ``action == "substituted"``; ``None`` otherwise.
            Expected shape::

                {
                    "original": "<original preset name>",
                    "replacement": "<replacement preset name>",
                    "dna_delta": {"warmth": +0.21, "brightness": -0.08},
                }

        Returns
        -------
        dict
            The full log entry as stored (including generated timestamp).

        Raises
        ------
        ValueError
            When ``stage`` or ``action`` is not a recognised value, or when
            ``substitution`` is missing for a ``"substituted"`` action.
        """
        if stage not in VALID_STAGES:
            raise ValueError(
                f"Invalid stage {stage!r}. Must be one of: {sorted(VALID_STAGES)}"
            )
        if action not in VALID_ACTIONS:
            raise ValueError(
                f"Invalid action {action!r}. Must be one of: {sorted(VALID_ACTIONS)}"
            )
        if action == "substituted" and not substitution:
            raise ValueError(
                "substitution dict is required when action == 'substituted'"
            )

        entry: dict = {
            "timestamp": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
            "pack_id": self.pack_id,
            "profile_id": self.profile_id,
            "stage": stage,
            "preset_name": preset_name,
            "action": action,
            "reason": reason,
            "substitution": substitution,
        }
        self._entries.append(entry)
        return entry

    # ------------------------------------------------------------------
    # Persistence
    # ------------------------------------------------------------------

    @property
    def log_path(self) -> Path:
        """Absolute path to the JSON log file for this pack."""
        return self.builds_root / self.pack_id / "qa_decisions.json"

    def save(self) -> Path:
        """
        Write all accumulated entries to ``builds/{pack_id}/qa_decisions.json``.

        Creates the output directory if it does not already exist.
        Overwrites any existing log for this pack.

        Returns
        -------
        Path
            The absolute path of the written file.
        """
        self.log_path.parent.mkdir(parents=True, exist_ok=True)
        payload = {
            "pack_id": self.pack_id,
            "profile_id": self.profile_id,
            "entry_count": len(self._entries),
            "entries": self._entries,
        }
        self.log_path.write_text(
            json.dumps(payload, indent=2, ensure_ascii=False),
            encoding="utf-8",
        )
        return self.log_path

    @classmethod
    def load(cls, pack_id: str, builds_root: Optional[Path] = None) -> "QADecisionLog":
        """
        Reconstruct a ``QADecisionLog`` from an existing JSON file.

        Parameters
        ----------
        pack_id:
            Build identifier used to locate ``builds/{pack_id}/qa_decisions.json``.
        builds_root:
            Override the default ``Tools/builds/`` root.

        Returns
        -------
        QADecisionLog
            A log instance pre-populated with the stored entries.

        Raises
        ------
        FileNotFoundError
            When the expected JSON file does not exist.
        """
        root = Path(builds_root) if builds_root else _DEFAULT_BUILDS_ROOT
        log_path = root / pack_id / "qa_decisions.json"
        if not log_path.exists():
            raise FileNotFoundError(
                f"No decision log found at {log_path}. "
                f"Has a build for pack '{pack_id}' been saved yet?"
            )
        raw = json.loads(log_path.read_text(encoding="utf-8"))
        profile_id = raw.get("profile_id", "unknown")
        instance = cls(pack_id=pack_id, profile_id=profile_id, builds_root=root)
        instance._entries = raw.get("entries", [])
        return instance

    # ------------------------------------------------------------------
    # Reporting
    # ------------------------------------------------------------------

    def summary(self) -> dict:
        """
        Return aggregate statistics over all logged decisions.

        Returns
        -------
        dict
            Keys:

            ``total``
                Total number of decisions logged.
            ``accepted``
                Count of ``"accepted"`` entries.
            ``rejected``
                Count of ``"rejected"`` entries.
            ``substituted``
                Count of ``"substituted"`` entries.
            ``by_stage``
                Dict mapping each stage name to a sub-dict of action counts.
            ``top_rejection_reasons``
                List of ``(reason, count)`` tuples for the 5 most frequent
                rejection reasons (entries where ``action == "rejected"``).
            ``top_substitution_reasons``
                List of ``(reason, count)`` tuples for the 5 most frequent
                substitution reasons.
        """
        total = len(self._entries)
        action_counts: Counter = Counter(e["action"] for e in self._entries)
        stage_action: dict[str, Counter] = {}
        for entry in self._entries:
            stage_action.setdefault(entry["stage"], Counter())[entry["action"]] += 1

        rejection_reasons = Counter(
            e["reason"] for e in self._entries if e["action"] == "rejected"
        )
        substitution_reasons = Counter(
            e["reason"] for e in self._entries if e["action"] == "substituted"
        )

        return {
            "total": total,
            "accepted": action_counts.get("accepted", 0),
            "rejected": action_counts.get("rejected", 0),
            "substituted": action_counts.get("substituted", 0),
            "by_stage": {
                stage: dict(counts)
                for stage, counts in stage_action.items()
            },
            "top_rejection_reasons": rejection_reasons.most_common(5),
            "top_substitution_reasons": substitution_reasons.most_common(5),
        }

    def print_summary(self) -> None:
        """Print a formatted summary to stdout."""
        s = self.summary()
        print(f"QA Decision Log — {self.pack_id} (profile: {self.profile_id})")
        print(f"  Total decisions : {s['total']}")
        print(f"  Accepted        : {s['accepted']}")
        print(f"  Rejected        : {s['rejected']}")
        print(f"  Substituted     : {s['substituted']}")
        if s["by_stage"]:
            print("  By stage:")
            for stage, counts in s["by_stage"].items():
                print(f"    {stage}: {counts}")
        if s["top_rejection_reasons"]:
            print("  Top rejection reasons:")
            for reason, count in s["top_rejection_reasons"]:
                print(f"    [{count}x] {reason}")
        if s["top_substitution_reasons"]:
            print("  Top substitution reasons:")
            for reason, count in s["top_substitution_reasons"]:
                print(f"    [{count}x] {reason}")

    def __len__(self) -> int:
        return len(self._entries)

    def __repr__(self) -> str:
        return (
            f"QADecisionLog(pack_id={self.pack_id!r}, "
            f"profile_id={self.profile_id!r}, "
            f"entries={len(self._entries)})"
        )


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="xpn_qa_decision_log — View or dump a saved QA decision log",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--pack-id",
        required=True,
        metavar="ID",
        help="Pack build ID (e.g. mpce-perc-001). Locates builds/{pack_id}/qa_decisions.json",
    )
    parser.add_argument(
        "--builds-root",
        default=None,
        metavar="DIR",
        help="Override the default builds/ root directory",
    )

    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument(
        "--summary",
        action="store_true",
        help="Print aggregate statistics for the log",
    )
    mode.add_argument(
        "--dump",
        action="store_true",
        help="Dump all log entries as JSON to stdout",
    )
    return parser


def main(argv: Optional[list[str]] = None) -> int:
    parser = _build_parser()
    args = parser.parse_args(argv)

    builds_root = Path(args.builds_root) if args.builds_root else None

    try:
        log = QADecisionLog.load(pack_id=args.pack_id, builds_root=builds_root)
    except FileNotFoundError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    if args.summary:
        log.print_summary()
    elif args.dump:
        print(
            json.dumps(
                {"pack_id": log.pack_id, "entries": log._entries},
                indent=2,
                ensure_ascii=False,
            )
        )

    return 0


if __name__ == "__main__":
    sys.exit(main())
