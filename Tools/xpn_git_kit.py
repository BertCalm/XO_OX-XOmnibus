#!/usr/bin/env python3
"""
XPN Git Kit — XO_OX Designs
Git commit history → MPC drum program.

Each commit becomes a drum hit. The rhythm of your codebase, played back.

Mappings:
  Inter-commit interval (time since previous commit) → pad selection (1–16)
  Lines changed (insertions + deletions)             → velocity (20–127)
  Commit message length                              → VelocityToVolume (0.3–1.0)
  Burst sessions (3+ commits within 2 hours)        → accent pad (13–16) at vel 127

Pad / Interval Bins (16 temporal bins):
  Pad  1:  0–30 min
  Pad  2:  30 min–1 hr
  Pad  3:  1–2 hr
  Pad  4:  2–4 hr
  Pad  5:  4–8 hr
  Pad  6:  8–12 hr
  Pad  7:  12–24 hr
  Pad  8:  1–2 days
  Pad  9:  2–3 days
  Pad 10:  3–5 days
  Pad 11:  5–7 days
  Pad 12:  1–2 weeks
  Pad 13:  2–4 weeks
  Pad 14:  1–3 months
  Pad 15:  3–6 months
  Pad 16:  6 months+

Velocity Layers (4 per pad):
  Layer 1: 1–50 lines changed    → VelStart=1,   VelEnd=50
  Layer 2: 51–200 lines changed  → VelStart=51,  VelEnd=200
  Layer 3: 201–1000 lines        → VelStart=201, VelEnd=1000 (capped at 127 for MPC)
  Layer 4: 1000+ lines           → VelStart=101, VelEnd=127

Velocity values by lines changed:
  1–10 lines    → 20
  11–50 lines   → 50
  51–200 lines  → 80
  201–500 lines → 100
  500+ lines    → 127

VelocityToVolume from commit message length:
  0–20 chars   → 0.3
  20–50 chars  → 0.6
  50–100 chars → 0.9
  100+ chars   → 1.0

Output:
  {output}/git_kit_{slug}.xpn   — MPC-compatible ZIP with XPM + placeholder WAVs
  {output}/commit_report.txt    — per-commit breakdown
  ASCII histogram printed to stdout

CLI:
  python xpn_git_kit.py --repo .
  python xpn_git_kit.py --repo /path/to/repo --output ./out/ --max-commits 200
  python xpn_git_kit.py --repo xomnibus --author you@email.com --since 2026-01-01
  python xpn_git_kit.py --repo . --output ./out/ --max-commits 100 --author me@email.com

XPN golden rules (no exceptions):
  KeyTrack  = True
  RootNote  = 0
  VelStart  = 0 for empty layers
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import zipfile
from datetime import date, datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from xml.sax.saxutils import escape as xml_escape

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

XOMNIBUS_REPO_PATH = Path.home() / "Documents" / "GitHub" / "XO_OX-XOmnibus"

# 16 temporal bins: (lower_seconds, upper_seconds, label, pad_index 0-based)
# upper_seconds = None means infinity
INTERVAL_BINS: List[Tuple[int, Optional[int], str]] = [
    (0,          30 * 60,             "0–30 min"),
    (30 * 60,    60 * 60,             "30 min–1 hr"),
    (60 * 60,    2 * 60 * 60,         "1–2 hr"),
    (2 * 60 * 60, 4 * 60 * 60,        "2–4 hr"),
    (4 * 60 * 60, 8 * 60 * 60,        "4–8 hr"),
    (8 * 60 * 60, 12 * 60 * 60,       "8–12 hr"),
    (12 * 60 * 60, 24 * 60 * 60,      "12–24 hr"),
    (24 * 60 * 60, 2 * 24 * 60 * 60,  "1–2 days"),
    (2 * 24 * 60 * 60, 3 * 24 * 60 * 60, "2–3 days"),
    (3 * 24 * 60 * 60, 5 * 24 * 60 * 60, "3–5 days"),
    (5 * 24 * 60 * 60, 7 * 24 * 60 * 60, "5–7 days"),
    (7 * 24 * 60 * 60, 14 * 24 * 60 * 60, "1–2 weeks"),
    (14 * 24 * 60 * 60, 28 * 24 * 60 * 60, "2–4 weeks"),
    (28 * 24 * 60 * 60, 90 * 24 * 60 * 60, "1–3 months"),
    (90 * 24 * 60 * 60, 180 * 24 * 60 * 60, "3–6 months"),
    (180 * 24 * 60 * 60, None,          "6 months+"),
]

# MIDI notes for 16 pads: C2 (36) through D#3 (51)
PAD_MIDI_NOTES = list(range(36, 52))

# Velocity layer split ranges (VelStart, VelEnd for each layer)
# Layer boundaries chosen for lines changed encoding
LAYER_VEL_RANGES = [
    (1,  50),    # Layer 1 — 1-50 lines (light changes)
    (51, 75),    # Layer 2 — 51-200 lines (medium changes)
    (76, 100),   # Layer 3 — 201-1000 lines (heavy changes)
    (101, 127),  # Layer 4 — 1000+ lines (massive changes)
]

PAD_LABELS = [
    "0-30min", "30m-1h", "1-2hr", "2-4hr", "4-8hr", "8-12hr", "12-24hr", "1-2day",
    "2-3day", "3-5day", "5-7day", "1-2wk", "2-4wk", "1-3mo", "3-6mo", "6mo+"
]


# ---------------------------------------------------------------------------
# Git parsing
# ---------------------------------------------------------------------------

def resolve_repo(repo_arg: str) -> Path:
    """Resolve --repo argument to an absolute path."""
    if repo_arg == "xomnibus":
        return XOMNIBUS_REPO_PATH
    p = Path(repo_arg).expanduser().resolve()
    if not p.exists():
        print(f"ERROR: repo path does not exist: {p}", file=sys.stderr)
        sys.exit(1)
    return p


def run_git(repo: Path, args: List[str], extra_env: Optional[Dict] = None) -> str:
    """Run a git command in repo, return stdout as string."""
    env = os.environ.copy()
    if extra_env:
        env.update(extra_env)
    result = subprocess.run(
        ["git", "-C", str(repo)] + args,
        capture_output=True,
        text=True,
        env=env,
    )
    if result.returncode != 0:
        print(f"ERROR running git: {result.stderr.strip()}", file=sys.stderr)
        sys.exit(1)
    return result.stdout


def fetch_commits(
    repo: Path,
    max_commits: int,
    author: Optional[str],
    since: Optional[str],
) -> List[Dict]:
    """
    Parse git log output into a list of commit dicts.
    Uses --numstat to get per-file line counts.

    Returns list of:
      {hash, timestamp, subject, insertions, deletions, lines_changed}
    sorted oldest-first.
    """
    git_args = [
        "log",
        f"--max-count={max_commits}",
        "--format=%H %at %s",
        "--numstat",
    ]
    if author:
        git_args.append(f"--author={author}")
    if since:
        git_args.append(f"--since={since}")

    raw = run_git(repo, git_args)
    commits: List[Dict] = []
    current: Optional[Dict] = None

    for line in raw.splitlines():
        line_stripped = line.strip()
        if not line_stripped:
            continue

        # Detect header lines: 40-char hex hash, space, unix timestamp, space, subject
        parts = line_stripped.split(" ", 2)
        if len(parts) >= 2 and len(parts[0]) == 40 and all(c in "0123456789abcdef" for c in parts[0]):
            if current is not None:
                commits.append(current)
            subject = parts[2] if len(parts) == 3 else ""
            current = {
                "hash": parts[0],
                "timestamp": int(parts[1]),
                "subject": subject,
                "insertions": 0,
                "deletions": 0,
            }
            continue

        # numstat line: insertions<tab>deletions<tab>filename
        # Binary files show "-" for counts
        if current is not None and "\t" in line_stripped:
            tab_parts = line_stripped.split("\t", 2)
            if len(tab_parts) >= 2:
                ins_str, del_str = tab_parts[0], tab_parts[1]
                if ins_str.isdigit():
                    current["insertions"] += int(ins_str)
                if del_str.isdigit():
                    current["deletions"] += int(del_str)

    if current is not None:
        commits.append(current)

    # Add computed field
    for c in commits:
        c["lines_changed"] = c["insertions"] + c["deletions"]

    # git log returns newest-first; reverse to oldest-first for interval calc
    commits.reverse()
    return commits


# ---------------------------------------------------------------------------
# Mapping functions
# ---------------------------------------------------------------------------

def interval_to_pad(seconds: float) -> int:
    """Map inter-commit interval in seconds to pad index (0-based, 0–15)."""
    for idx, (lo, hi, _label) in enumerate(INTERVAL_BINS):
        if hi is None or seconds < hi:
            return idx
    return 15


def lines_to_velocity(lines: int) -> int:
    """Map lines changed to MIDI velocity (20–127)."""
    if lines <= 10:
        return 20
    if lines <= 50:
        return 50
    if lines <= 200:
        return 80
    if lines <= 500:
        return 100
    return 127


def lines_to_layer(lines: int) -> int:
    """Return layer index (0-based 0–3) based on lines changed."""
    if lines <= 50:
        return 0
    if lines <= 200:
        return 1
    if lines <= 1000:
        return 2
    return 3


def msg_len_to_vtv(length: int) -> float:
    """Map commit message length to VelocityToVolume float (0.3–1.0)."""
    if length < 20:
        return 0.3
    if length < 50:
        return 0.6
    if length < 100:
        return 0.9
    return 1.0


def detect_burst_sessions(commits: List[Dict]) -> set:
    """
    Return a set of commit hashes that are part of burst sessions.
    Burst = 3 or more commits within any 2-hour window.
    """
    burst_hashes: set = set()
    n = len(commits)
    two_hours = 2 * 60 * 60

    for i in range(n):
        window = [commits[i]]
        for j in range(i + 1, n):
            if commits[j]["timestamp"] - commits[i]["timestamp"] <= two_hours:
                window.append(commits[j])
            else:
                break
        if len(window) >= 3:
            for c in window:
                burst_hashes.add(c["hash"])

    return burst_hashes


def assign_commit(commit: Dict, prev_ts: Optional[int], burst_hashes: set) -> Dict:
    """
    Compute pad, velocity, layer, and vtv for a single commit.
    Returns augmented commit dict with these fields.
    """
    # Interval
    if prev_ts is None:
        interval = 180 * 24 * 60 * 60  # first commit → pad 15 (6 months+)
    else:
        interval = max(0, commit["timestamp"] - prev_ts)

    pad_idx = interval_to_pad(interval)

    # Burst override: reassign to accent pads 12–15 (0-based)
    is_burst = commit["hash"] in burst_hashes
    if is_burst:
        # Map to pads 12–15 based on lines changed
        lines = commit["lines_changed"]
        if lines <= 50:
            pad_idx = 12
        elif lines <= 200:
            pad_idx = 13
        elif lines <= 1000:
            pad_idx = 14
        else:
            pad_idx = 15

    velocity = 127 if is_burst else lines_to_velocity(commit["lines_changed"])
    layer = lines_to_layer(commit["lines_changed"])
    vtv = msg_len_to_vtv(len(commit["subject"]))

    return {
        **commit,
        "interval_seconds": interval,
        "pad_idx": pad_idx,          # 0-based
        "pad_num": pad_idx + 1,      # 1-based for display
        "velocity": velocity,
        "layer_idx": layer,          # 0-based
        "layer_num": layer + 1,      # 1-based for display
        "vtv": vtv,
        "is_burst": is_burst,
    }


def process_commits(commits: List[Dict]) -> List[Dict]:
    """Assign pad/velocity/layer to all commits in chronological order."""
    burst_hashes = detect_burst_sessions(commits)
    processed = []
    prev_ts = None
    for c in commits:
        p = assign_commit(c, prev_ts, burst_hashes)
        processed.append(p)
        prev_ts = c["timestamp"]
    return processed


# ---------------------------------------------------------------------------
# Pad hit accumulator → per-pad velocity layers
# ---------------------------------------------------------------------------

def build_pad_layers(processed: List[Dict]) -> Dict[int, Dict]:
    """
    For each pad (0-based 0–15), build 4 layer sample file names.
    Returns {pad_idx: {layer_idx: sample_filename, ...}}

    Sample filename format: git_pad{N:02d}_layer{L}.wav
    (N = 1-based pad, L = 1-based layer)
    """
    # Track which (pad, layer) combos have hits
    hits: Dict[Tuple[int, int], int] = {}  # (pad_idx, layer_idx) → count
    for c in processed:
        key = (c["pad_idx"], c["layer_idx"])
        hits[key] = hits.get(key, 0) + 1

    pad_layers: Dict[int, Dict] = {}
    for pad_idx in range(16):
        pad_layers[pad_idx] = {}
        for layer_idx in range(4):
            if (pad_idx, layer_idx) in hits:
                pad_layers[pad_idx][layer_idx] = f"git_pad{pad_idx + 1:02d}_layer{layer_idx + 1}.wav"
            else:
                pad_layers[pad_idx][layer_idx] = ""  # empty — no hits

    return pad_layers


# ---------------------------------------------------------------------------
# XPM XML builder
# ---------------------------------------------------------------------------

def _layer_block(number: int, vel_start: int, vel_end: int,
                 sample_name: str, volume: float, program_slug: str) -> str:
    active = "True" if sample_name else "False"
    file_path = f"Samples/{program_slug}/{sample_name}" if sample_name else ""
    return (
        f'          <Layer number="{number}">\n'
        f'            <Active>{active}</Active>\n'
        f'            <Volume>{volume:.6f}</Volume>\n'
        f'            <Pan>0.500000</Pan>\n'
        f'            <Pitch>0.000000</Pitch>\n'
        f'            <TuneCoarse>0</TuneCoarse>\n'
        f'            <TuneFine>0</TuneFine>\n'
        f'            <VelStart>{vel_start}</VelStart>\n'
        f'            <VelEnd>{vel_end}</VelEnd>\n'
        f'            <SampleStart>0</SampleStart>\n'
        f'            <SampleEnd>0</SampleEnd>\n'
        f'            <Loop>False</Loop>\n'
        f'            <LoopStart>0</LoopStart>\n'
        f'            <LoopEnd>0</LoopEnd>\n'
        f'            <LoopTune>0</LoopTune>\n'
        f'            <Mute>False</Mute>\n'
        f'            <RootNote>0</RootNote>\n'
        f'            <KeyTrack>True</KeyTrack>\n'
        f'            <SampleName>{xml_escape(sample_name)}</SampleName>\n'
        f'            <SampleFile>{xml_escape(sample_name)}</SampleFile>\n'
        f'            <File>{xml_escape(file_path)}</File>\n'
        f'            <SliceIndex>128</SliceIndex>\n'
        f'            <Direction>0</Direction>\n'
        f'            <Offset>0</Offset>\n'
        f'            <SliceStart>0</SliceStart>\n'
        f'            <SliceEnd>0</SliceEnd>\n'
        f'            <SliceLoopStart>0</SliceLoopStart>\n'
        f'            <SliceLoop>0</SliceLoop>\n'
        f'          </Layer>'
    )


def _empty_layer(number: int) -> str:
    return (
        f'          <Layer number="{number}">\n'
        f'            <Active>False</Active>\n'
        f'            <Volume>0.707946</Volume>\n'
        f'            <Pan>0.500000</Pan>\n'
        f'            <Pitch>0.000000</Pitch>\n'
        f'            <TuneCoarse>0</TuneCoarse>\n'
        f'            <TuneFine>0</TuneFine>\n'
        f'            <VelStart>0</VelStart>\n'
        f'            <VelEnd>0</VelEnd>\n'
        f'            <SampleStart>0</SampleStart>\n'
        f'            <SampleEnd>0</SampleEnd>\n'
        f'            <Loop>False</Loop>\n'
        f'            <LoopStart>0</LoopStart>\n'
        f'            <LoopEnd>0</LoopEnd>\n'
        f'            <LoopTune>0</LoopTune>\n'
        f'            <Mute>False</Mute>\n'
        f'            <RootNote>0</RootNote>\n'
        f'            <KeyTrack>True</KeyTrack>\n'
        f'            <SampleName></SampleName>\n'
        f'            <SampleFile></SampleFile>\n'
        f'            <File></File>\n'
        f'            <SliceIndex>128</SliceIndex>\n'
        f'            <Direction>0</Direction>\n'
        f'            <Offset>0</Offset>\n'
        f'            <SliceStart>0</SliceStart>\n'
        f'            <SliceEnd>0</SliceEnd>\n'
        f'            <SliceLoopStart>0</SliceLoopStart>\n'
        f'            <SliceLoop>0</SliceLoop>\n'
        f'          </Layer>'
    )


def _instrument_block(
    instrument_num: int,
    pad_idx: int,
    pad_layers: Dict[int, str],
    pad_label: str,
    program_slug: str,
    vtv: float,
    active: bool,
) -> str:
    midi_note = PAD_MIDI_NOTES[pad_idx]

    # Build up to 4 layer blocks
    layer_xml_parts = []
    for layer_idx in range(4):
        sample_name = pad_layers.get(layer_idx, "")
        if sample_name:
            vel_start, vel_end = LAYER_VEL_RANGES[layer_idx]
            volume = 0.6 + 0.4 * (layer_idx / 3.0)  # 0.6 → 1.0 across layers
            layer_xml_parts.append(
                _layer_block(layer_idx + 1, vel_start, vel_end, sample_name, volume, program_slug)
            )
        else:
            layer_xml_parts.append(_empty_layer(layer_idx + 1))

    layers_xml = "\n".join(layer_xml_parts)
    active_str = "True" if active else "False"

    return (
        f'        <Instrument number="{instrument_num}">\n'
        f'          <Active>{active_str}</Active>\n'
        f'          <Volume>1.000000</Volume>\n'
        f'          <Pan>0.500000</Pan>\n'
        f'          <Pitch>0.000000</Pitch>\n'
        f'          <MidiNote>{midi_note}</MidiNote>\n'
        f'          <ZonePlay>1</ZonePlay>\n'
        f'          <MuteGroup>0</MuteGroup>\n'
        f'          <MuteTarget>0</MuteTarget>\n'
        f'          <VoiceOverlap>0</VoiceOverlap>\n'
        f'          <Mono>True</Mono>\n'
        f'          <Polyphony>1</Polyphony>\n'
        f'          <OneShot>True</OneShot>\n'
        f'          <VelocitySensitivity>1.000000</VelocitySensitivity>\n'
        f'          <VelocityToPitch>0.000000</VelocityToPitch>\n'
        f'          <VelocityToFilter>0.150000</VelocityToFilter>\n'
        f'          <VelocityToVolume>{vtv:.6f}</VelocityToVolume>\n'
        f'          <ProgramName>{xml_escape(pad_label)}</ProgramName>\n'
        f'          <Attack>0.000000</Attack>\n'
        f'          <Hold>0.000000</Hold>\n'
        f'          <Decay>0.350000</Decay>\n'
        f'          <Sustain>0.000000</Sustain>\n'
        f'          <Release>0.100000</Release>\n'
        f'          <FilterType>2</FilterType>\n'
        f'          <FilterCutoff>0.900000</FilterCutoff>\n'
        f'          <FilterResonance>0.050000</FilterResonance>\n'
        f'          <FilterEnvAmt>0.050000</FilterEnvAmt>\n'
        f'{layers_xml}\n'
        f'        </Instrument>'
    )


def _inactive_instrument(number: int, midi_note: int) -> str:
    layers = "\n".join(_empty_layer(i + 1) for i in range(4))
    return (
        f'        <Instrument number="{number}">\n'
        f'          <Active>False</Active>\n'
        f'          <Volume>0.707946</Volume>\n'
        f'          <Pan>0.500000</Pan>\n'
        f'          <Pitch>0.000000</Pitch>\n'
        f'          <MidiNote>{midi_note}</MidiNote>\n'
        f'          <ZonePlay>1</ZonePlay>\n'
        f'          <MuteGroup>0</MuteGroup>\n'
        f'          <MuteTarget>0</MuteTarget>\n'
        f'          <VoiceOverlap>0</VoiceOverlap>\n'
        f'          <Mono>True</Mono>\n'
        f'          <Polyphony>1</Polyphony>\n'
        f'          <OneShot>True</OneShot>\n'
        f'          <VelocitySensitivity>1.000000</VelocitySensitivity>\n'
        f'          <VelocityToPitch>0.000000</VelocityToPitch>\n'
        f'          <VelocityToFilter>0.000000</VelocityToFilter>\n'
        f'          <VelocityToVolume>0.707946</VelocityToVolume>\n'
        f'          <ProgramName></ProgramName>\n'
        f'          <Attack>0.000000</Attack>\n'
        f'          <Hold>0.000000</Hold>\n'
        f'          <Decay>0.000000</Decay>\n'
        f'          <Sustain>1.000000</Sustain>\n'
        f'          <Release>0.000000</Release>\n'
        f'          <FilterType>2</FilterType>\n'
        f'          <FilterCutoff>1.000000</FilterCutoff>\n'
        f'          <FilterResonance>0.000000</FilterResonance>\n'
        f'          <FilterEnvAmt>0.000000</FilterEnvAmt>\n'
        f'{layers}\n'
        f'        </Instrument>'
    )


def build_xpm(
    pad_layers_map: Dict[int, Dict],
    pad_vtv: Dict[int, float],
    program_slug: str,
    repo_name: str,
    commit_count: int,
    date_range: str,
) -> str:
    today = date.today().isoformat()
    instruments_xml_parts = []

    # 16 active pads
    for pad_idx in range(16):
        pad_layers = pad_layers_map[pad_idx]
        has_any_sample = any(pad_layers.get(li, "") for li in range(4))
        vtv = pad_vtv.get(pad_idx, 0.707946)
        label = f"{PAD_LABELS[pad_idx]} — {INTERVAL_BINS[pad_idx][2]}"
        instruments_xml_parts.append(
            _instrument_block(
                instrument_num=pad_idx + 1,
                pad_idx=pad_idx,
                pad_layers=pad_layers,
                pad_label=label,
                program_slug=program_slug,
                vtv=vtv,
                active=has_any_sample,
            )
        )

    # Fill remaining 112 slots (17–128) as inactive
    for instrument_num in range(17, 129):
        midi_note = min(PAD_MIDI_NOTES[15] + (instrument_num - 16), 127)
        instruments_xml_parts.append(_inactive_instrument(instrument_num, midi_note))

    instruments_xml = "\n".join(instruments_xml_parts)

    return f"""<?xml version="1.0" encoding="UTF-8"?>
<MPCVObject>
  <Version>
    <File_Version>2.1</File_Version>
    <Application>MPC</Application>
    <Application_Version>2.10</Application_Version>
  </Version>
  <Program type="Drum">
    <Name>Git Kit — {xml_escape(repo_name)}</Name>
    <Slug>{program_slug}</Slug>
    <DateCreated>{today}</DateCreated>
    <Description>Git commit history as drum kit. {commit_count} commits from {xml_escape(date_range)}. Each pad = one temporal interval between commits. Velocity = lines changed. VelocityToVolume = commit message length. Pads 13-16 = burst session accents (3+ commits within 2 hours).</Description>
    <NumInstruments>128</NumInstruments>
    <ProgramVolume>1.000000</ProgramVolume>
    <ProgramPan>0.500000</ProgramPan>
    <ProgramTranspose>0</ProgramTranspose>
    <MemoryUsage>0</MemoryUsage>
    <LFO1Shape>0</LFO1Shape>
    <LFO1Rate>0.000000</LFO1Rate>
    <LFO1Depth>0.000000</LFO1Depth>
    <FilterQ>0.000000</FilterQ>
    <InstrumentList>
{instruments_xml}
    </InstrumentList>
  </Program>
</MPCVObject>
"""


# ---------------------------------------------------------------------------
# Commit report builder
# ---------------------------------------------------------------------------

def format_interval(seconds: float) -> str:
    """Human-readable interval string."""
    if seconds < 60:
        return f"{int(seconds)}s"
    if seconds < 3600:
        return f"{int(seconds / 60)}m"
    if seconds < 86400:
        h = seconds / 3600
        return f"{h:.1f}h"
    d = seconds / 86400
    return f"{d:.1f}d"


def build_commit_report(
    processed: List[Dict],
    repo_path: Path,
    author_filter: Optional[str],
    since_filter: Optional[str],
) -> str:
    lines = [
        "XPN Git Kit — Commit Report",
        "=" * 72,
        f"Repository : {repo_path}",
        f"Generated  : {date.today().isoformat()}",
        f"Commits    : {len(processed)}",
    ]
    if author_filter:
        lines.append(f"Author     : {author_filter}")
    if since_filter:
        lines.append(f"Since      : {since_filter}")
    lines.append("")
    lines.append(
        f"{'Hash':>10}  {'Date':>10}  {'Pad':>3}  {'Layer':>5}  {'Vel':>3}  {'VtV':>4}  {'Lines':>6}  {'Burst':>5}  Subject"
    )
    lines.append("-" * 100)

    for c in processed:
        ts_str = datetime.fromtimestamp(c["timestamp"]).strftime("%Y-%m-%d")
        subject = c["subject"][:50] + "…" if len(c["subject"]) > 50 else c["subject"]
        burst_flag = "YES" if c["is_burst"] else ""
        lines.append(
            f"{c['hash'][:10]}  {ts_str}  {c['pad_num']:>3}  L{c['layer_num']:>4}  {c['velocity']:>3}  "
            f"{c['vtv']:.2f}  {c['lines_changed']:>6}  {burst_flag:>5}  {subject}"
        )

    lines.append("")
    lines.append("Pad Mapping")
    lines.append("-" * 40)
    for i, (_lo, _hi, label) in enumerate(INTERVAL_BINS):
        lines.append(f"  Pad {i + 1:>2} (MIDI {PAD_MIDI_NOTES[i]:>3}) : {label}")

    lines.append("")
    lines.append("Velocity Layer Boundaries")
    lines.append("-" * 40)
    layer_labels = ["1–50 lines (light)", "51–200 lines (medium)", "201–1000 lines (heavy)", "1000+ lines (massive)"]
    for i, (vs, ve) in enumerate(LAYER_VEL_RANGES):
        lines.append(f"  Layer {i + 1}: vel {vs:>3}–{ve:>3} — {layer_labels[i]}")

    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------------------
# ASCII histogram
# ---------------------------------------------------------------------------

def print_histogram(processed: List[Dict]) -> None:
    pad_counts = [0] * 16
    for c in processed:
        pad_counts[c["pad_idx"]] += 1

    total = max(sum(pad_counts), 1)
    bar_width = 40

    print("\nCommit Frequency per Pad")
    print("=" * 72)
    print(f"  {'Pad':>3}  {'Interval':>14}  {'Count':>5}  {'Bar'}")
    print("-" * 72)
    for i in range(16):
        count = pad_counts[i]
        label = INTERVAL_BINS[i][2]
        bar_len = int(count / total * bar_width)
        bar = "#" * bar_len
        print(f"  {i + 1:>3}  {label:>14}  {count:>5}  {bar}")
    print()

    # Burst summary
    burst_count = sum(1 for c in processed if c["is_burst"])
    print(f"Burst session commits : {burst_count} / {len(processed)}")
    print(f"Pads with zero hits   : {sum(1 for c in pad_counts if c == 0)}")
    print()


# ---------------------------------------------------------------------------
# ZIP / XPN builder
# ---------------------------------------------------------------------------

def build_xpn_zip(
    xpm_xml: str,
    program_slug: str,
    pad_layers_map: Dict[int, Dict],
    output_dir: Path,
) -> Path:
    """
    Write the XPN ZIP file.
    Includes:
      - {program_slug}.xpm
      - Samples/{program_slug}/*.wav  (placeholder empty WAV stubs for referenced samples)
    """
    output_dir.mkdir(parents=True, exist_ok=True)
    zip_path = output_dir / f"{program_slug}.xpn"

    # Minimal valid WAV stub: 44-byte header, 0 frames
    def minimal_wav_stub() -> bytes:
        import struct
        channels = 1
        sample_rate = 44100
        bit_depth = 16
        byte_rate = sample_rate * channels * bit_depth // 8
        block_align = channels * bit_depth // 8
        data_size = 0
        chunk_size = 36 + data_size
        header = struct.pack(
            "<4sI4s4sIHHIIHH4sI",
            b"RIFF", chunk_size, b"WAVE",
            b"fmt ", 16,
            1,            # PCM
            channels,
            sample_rate,
            byte_rate,
            block_align,
            bit_depth,
            b"data", data_size,
        )
        return header

    wav_stub = minimal_wav_stub()

    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
        zf.writestr(f"{program_slug}.xpm", xpm_xml)

        for pad_idx in range(16):
            for layer_idx in range(4):
                sample_name = pad_layers_map[pad_idx].get(layer_idx, "")
                if sample_name:
                    zf.writestr(f"Samples/{program_slug}/{sample_name}", wav_stub)

    return zip_path


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="XPN Git Kit — git commit history → MPC drum program",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--repo",
        default=".",
        help='Repo path. Use "." for current dir, "xomnibus" for XO_OX-XOmnibus, or any path.',
    )
    parser.add_argument(
        "--output",
        default="./out/",
        help="Output directory for XPN ZIP and commit_report.txt (default: ./out/)",
    )
    parser.add_argument(
        "--max-commits",
        type=int,
        default=200,
        help="Maximum number of commits to parse (default: 200)",
    )
    parser.add_argument(
        "--author",
        default=None,
        help="Filter commits by author email or name (passed to git --author)",
    )
    parser.add_argument(
        "--since",
        default=None,
        help="Filter commits since date, e.g. 2026-01-01 (passed to git --since)",
    )
    args = parser.parse_args()

    repo_path = resolve_repo(args.repo)
    output_dir = Path(args.output).expanduser().resolve()

    print(f"XPN Git Kit")
    print(f"Repository : {repo_path}")
    print(f"Output     : {output_dir}")
    print(f"Max commits: {args.max_commits}")
    if args.author:
        print(f"Author     : {args.author}")
    if args.since:
        print(f"Since      : {args.since}")
    print()

    # Get repo name for slug
    repo_name = repo_path.name
    safe_name = "".join(c if c.isalnum() or c in "-_" else "_" for c in repo_name).lower()
    program_slug = f"git_kit_{safe_name}"

    # Fetch and process commits
    print("Fetching commits...")
    commits = fetch_commits(repo_path, args.max_commits, args.author, args.since)

    if not commits:
        print("ERROR: No commits found with the given filters.", file=sys.stderr)
        sys.exit(1)

    print(f"Found {len(commits)} commits. Processing...")
    processed = process_commits(commits)

    # Compute date range string
    first_ts = processed[0]["timestamp"]
    last_ts = processed[-1]["timestamp"]
    date_range = (
        f"{datetime.fromtimestamp(first_ts).strftime('%Y-%m-%d')} – "
        f"{datetime.fromtimestamp(last_ts).strftime('%Y-%m-%d')}"
    )

    # Build pad layers map
    pad_layers_map = build_pad_layers(processed)

    # Compute per-pad average VelocityToVolume (mean of all commits hitting that pad)
    pad_vtv_sums: Dict[int, float] = {}
    pad_vtv_counts: Dict[int, int] = {}
    for c in processed:
        pi = c["pad_idx"]
        pad_vtv_sums[pi] = pad_vtv_sums.get(pi, 0.0) + c["vtv"]
        pad_vtv_counts[pi] = pad_vtv_counts.get(pi, 0) + 1
    pad_vtv = {
        pi: pad_vtv_sums[pi] / pad_vtv_counts[pi]
        for pi in pad_vtv_sums
    }

    # Print histogram
    print_histogram(processed)

    # Build XPM XML
    xpm_xml = build_xpm(
        pad_layers_map=pad_layers_map,
        pad_vtv=pad_vtv,
        program_slug=program_slug,
        repo_name=repo_name,
        commit_count=len(processed),
        date_range=date_range,
    )

    # Build commit report
    report_text = build_commit_report(processed, repo_path, args.author, args.since)

    # Write XPN ZIP
    xpn_path = build_xpn_zip(xpm_xml, program_slug, pad_layers_map, output_dir)
    print(f"XPN written : {xpn_path}")

    # Write commit report
    report_path = output_dir / "commit_report.txt"
    report_path.write_text(report_text, encoding="utf-8")
    print(f"Report      : {report_path}")

    # Summary
    pad_counts = [0] * 16
    for c in processed:
        pad_counts[c["pad_idx"]] += 1
    active_pads = sum(1 for n in pad_counts if n > 0)
    burst_count = sum(1 for c in processed if c["is_burst"])
    print()
    print(f"Summary")
    print(f"  Commits processed : {len(processed)}")
    print(f"  Active pads       : {active_pads} / 16")
    print(f"  Burst commits     : {burst_count}")
    print(f"  Date range        : {date_range}")
    print(f"  Program slug      : {program_slug}")


if __name__ == "__main__":
    main()
