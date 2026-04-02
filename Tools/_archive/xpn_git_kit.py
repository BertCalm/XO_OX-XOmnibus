#!/usr/bin/env python3
"""
XPN Git Kit — XO_OX Designs
Git commit history → MPC drum patterns.

Parses any local git repository via subprocess (pure stdlib, no external git libs).

Mapping:
  Inter-arrival time → which of 16 pads gets hit:
    Pad  1:  0-1 hr         (rapid fire)
    Pad  2:  1-4 hrs        (focused session)
    Pad  3:  4-8 hrs        (half-day gap)
    Pad  4:  8-24 hrs       (daily cadence)
    Pad  5:  1-3 days
    Pad  6:  3-7 days       (weekly)
    Pad  7:  1-2 weeks
    Pad  8:  2-4 weeks
    Pad  9:  1-2 months
    Pad 10:  2-3 months
    Pad 11:  3-6 months
    Pad 12:  6-12 months
    Pad 13: 12-24 months    (also burst accent)
    Pad 14: 24-36 months    (also burst accent)
    Pad 15: 36-60 months    (also burst accent)
    Pad 16: 60+ months      (also burst accent)

  Files changed → base velocity:
    1-5   files → 30
    6-20  files → 60
    21-100 files → 90
    100+  files → 127

  Lines added (insertions) → velocity boost: +min(20, insertions // 50)
  Lines deleted (deletions) → velocity reduction: -min(15, deletions // 100)
  Commit message length → msg weight (short=0.3, medium=0.6, long=1.0)
  Burst sessions (3+ commits within 2 hours) → accent hits on pads 13-16

Output: XPN Drum Program ZIP + commit_report.txt
CLI:
  python xpn_git_kit.py --repo . --output ./out/ --max-commits 200
  python xpn_git_kit.py --repo /path/to/repo --output ./out/
  python xpn_git_kit.py --repo XO_OX-XOceanus --output ./out/
"""

import argparse
import json
import re
import subprocess
import sys
import zipfile
from datetime import date, datetime, timezone
from pathlib import Path
from xml.sax.saxutils import escape as xml_escape

# =============================================================================
# INTER-ARRIVAL TIME BINS → PAD NUMBER (1-16), all values in seconds
# =============================================================================

IAT_BINS = [
    (0,            3_600,           1),    # 0-1 hr
    (3_600,       14_400,           2),    # 1-4 hrs
    (14_400,      28_800,           3),    # 4-8 hrs
    (28_800,      86_400,           4),    # 8-24 hrs
    (86_400,     259_200,           5),    # 1-3 days
    (259_200,    604_800,           6),    # 3-7 days
    (604_800,  1_209_600,           7),    # 1-2 weeks
    (1_209_600, 2_419_200,          8),    # 2-4 weeks
    (2_419_200, 5_184_000,          9),    # 1-2 months
    (5_184_000, 7_776_000,         10),    # 2-3 months
    (7_776_000, 15_552_000,        11),    # 3-6 months
    (15_552_000, 31_104_000,       12),    # 6-12 months
    (31_104_000, 63_072_000,       13),    # 1-2 years
    (63_072_000, 94_608_000,       14),    # 2-3 years
    (94_608_000, 157_680_000,      15),    # 3-5 years
    (157_680_000, float("inf"),    16),    # 5+ years
]

BURST_WINDOW_SECS = 7_200   # 2 hours
BURST_THRESHOLD   = 3       # 3+ commits in window = burst session

# =============================================================================
# PAD → MIDI NOTE + VOICE + LABEL
# =============================================================================

PAD_MIDI = {
    1:  36,   # Kick
    2:  38,   # Snare
    3:  42,   # Closed Hat
    4:  46,   # Open Hat
    5:  39,   # Clap
    6:  41,   # Low Tom
    7:  43,   # Mid Tom
    8:  45,   # High Tom
    9:  49,   # Crash Cymbal
    10: 51,   # Ride Cymbal
    11: 56,   # Cowbell
    12: 37,   # Side Stick
    13: 50,   # High Floor Tom (burst accent 1)
    14: 55,   # Splash Cymbal  (burst accent 2)
    15: 57,   # Crash 2        (burst accent 3)
    16: 58,   # Vibraslap      (burst accent 4)
}

PAD_VOICE = {
    1: "kick",  2: "snare", 3: "chat",  4: "ohat",
    5: "clap",  6: "tom",   7: "tom",   8: "tom",
    9: "fx",   10: "fx",   11: "fx",   12: "snare",
    13: "fx",  14: "fx",   15: "fx",   16: "fx",
}

PAD_LABELS = {
    1:  "Kick — 0-1hr",            2:  "Snare — 1-4hr",
    3:  "CHat — 4-8hr",            4:  "OHat — 8-24hr",
    5:  "Clap — 1-3day",           6:  "LowTom — 3-7day",
    7:  "MidTom — 1-2wk",          8:  "HiTom — 2-4wk",
    9:  "Crash — 1-2mo",           10: "Ride — 2-3mo",
    11: "Cowbell — 3-6mo",         12: "Stick — 6-12mo",
    13: "HiFloor — 1-2yr (burst)", 14: "Splash — 2-3yr (burst)",
    15: "Crash2 — 3-5yr (burst)",  16: "Vibra — 5+yr (burst)",
}

VOICE_DEFAULTS = {
    "kick":  {"mono": True,  "poly": 1, "one_shot": True,  "vtp": 0.05, "vtf": 0.0,
               "attack": 0.0, "decay": 0.3, "sustain": 0.0, "release": 0.05,
               "cutoff": 1.0, "resonance": 0.0, "mute_group": 0},
    "snare": {"mono": True,  "poly": 1, "one_shot": True,  "vtp": 0.0,  "vtf": 0.30,
               "attack": 0.0, "decay": 0.4, "sustain": 0.0, "release": 0.08,
               "cutoff": 0.9, "resonance": 0.05, "mute_group": 0},
    "chat":  {"mono": True,  "poly": 1, "one_shot": True,  "vtp": 0.0,  "vtf": 0.0,
               "attack": 0.0, "decay": 0.15, "sustain": 0.0, "release": 0.02,
               "cutoff": 0.85, "resonance": 0.1, "mute_group": 1},
    "ohat":  {"mono": False, "poly": 2, "one_shot": False, "vtp": 0.0,  "vtf": 0.0,
               "attack": 0.0, "decay": 0.8, "sustain": 0.38, "release": 0.3,
               "cutoff": 0.95, "resonance": 0.0, "mute_group": 1},
    "clap":  {"mono": False, "poly": 2, "one_shot": True,  "vtp": 0.0,  "vtf": 0.15,
               "attack": 0.0, "decay": 0.5, "sustain": 0.0, "release": 0.1,
               "cutoff": 0.95, "resonance": 0.0, "mute_group": 0},
    "tom":   {"mono": True,  "poly": 2, "one_shot": True,  "vtp": 0.02, "vtf": 0.1,
               "attack": 0.0, "decay": 0.5, "sustain": 0.0, "release": 0.1,
               "cutoff": 0.9, "resonance": 0.0, "mute_group": 0},
    "fx":    {"mono": False, "poly": 4, "one_shot": False, "vtp": 0.0,  "vtf": 0.0,
               "attack": 0.01, "decay": 0.6, "sustain": 0.1, "release": 0.3,
               "cutoff": 1.0, "resonance": 0.0, "mute_group": 0},
}


# =============================================================================
# GIT LOG PARSING
# =============================================================================

def find_repo(repo_arg: str) -> Path:
    """Resolve --repo argument to an absolute path."""
    if repo_arg == ".":
        return Path.cwd()
    p = Path(repo_arg)
    if p.is_absolute() and p.exists():
        return p
    # Shorthand: search ~/Documents/GitHub/ then cwd
    for candidate in [
        Path.home() / "Documents" / "GitHub" / repo_arg,
        Path.cwd() / repo_arg,
    ]:
        if candidate.exists():
            return candidate
    return p   # let git fail with a useful error


def run_git(args: list, cwd: Path) -> str:
    """Run a git command in cwd, return stdout. Raises RuntimeError on failure."""
    try:
        result = subprocess.run(
            ["git"] + args,
            cwd=str(cwd),
            capture_output=True,
            text=True,
            timeout=60,
        )
    except FileNotFoundError:
        raise RuntimeError("git not found in PATH")
    except subprocess.TimeoutExpired:
        raise RuntimeError("git command timed out after 60s")
    if result.returncode != 0:
        raise RuntimeError(f"git error: {result.stderr.strip()}")
    return result.stdout


def parse_git_log(repo_path: Path, max_commits: int) -> list:
    """
    Parse git log into a list of commit dicts:
      {hash, timestamp, subject, files_changed, insertions, deletions}
    Sorted oldest-first.
    """
    # Get hashes + timestamps + subjects
    log_out = run_git(
        ["log", f"--max-count={max_commits}", "--format=%H %at %s"],
        repo_path,
    )
    commits_raw = []
    for line in log_out.splitlines():
        line = line.strip()
        if not line:
            continue
        parts = line.split(" ", 2)
        if len(parts) < 2:
            continue
        try:
            timestamp = int(parts[1])
        except ValueError:
            continue
        commits_raw.append({
            "hash":          parts[0],
            "timestamp":     timestamp,
            "subject":       parts[2] if len(parts) > 2 else "",
            "files_changed": 0,
            "insertions":    0,
            "deletions":     0,
        })

    if not commits_raw:
        return []

    # Get numstat in one pass
    numstat_out = run_git(
        ["log", f"--max-count={max_commits}", "--numstat", "--format=COMMIT:%H"],
        repo_path,
    )
    stats: dict = {}
    current_hash = None
    for line in numstat_out.splitlines():
        if line.startswith("COMMIT:"):
            current_hash = line[7:].strip()
            stats[current_hash] = {"files": 0, "ins": 0, "dels": 0}
        elif current_hash and line.strip():
            cols = line.split("\t")
            if len(cols) >= 3:
                try:
                    ins  = int(cols[0]) if cols[0] != "-" else 0
                    dels = int(cols[1]) if cols[1] != "-" else 0
                    stats[current_hash]["files"] += 1
                    stats[current_hash]["ins"]   += ins
                    stats[current_hash]["dels"]  += dels
                except ValueError:
                    pass

    for c in commits_raw:
        s = stats.get(c["hash"], {})
        c["files_changed"] = s.get("files", 0)
        c["insertions"]    = s.get("ins",   0)
        c["deletions"]     = s.get("dels",  0)

    # Sort oldest-first for inter-arrival calculation
    commits_raw.sort(key=lambda c: c["timestamp"])
    return commits_raw


# =============================================================================
# COMMIT → PAD + VELOCITY MAPPING
# =============================================================================

def iat_to_pad(iat_seconds: float) -> int:
    """Map inter-arrival time (seconds) to pad number 1-16."""
    for lo, hi, pad in IAT_BINS:
        if lo <= iat_seconds < hi:
            return pad
    return 16


def files_to_base_velocity(files: int) -> int:
    if files >= 100:
        return 127
    elif files >= 21:
        return 90
    elif files >= 6:
        return 60
    return 30


def compute_velocity(files: int, insertions: int, deletions: int) -> int:
    v = files_to_base_velocity(files)
    v += min(20, insertions // 50)
    v -= min(15, deletions // 100)
    return max(1, min(127, v))


def msg_weight(subject: str) -> float:
    n = len(subject)
    if n < 20:
        return 0.3
    elif n < 80:
        return 0.6
    return 1.0


def detect_bursts(commits: list) -> set:
    """Return set of commit hashes that are part of burst sessions."""
    burst_hashes: set = set()
    n = len(commits)
    for i in range(n):
        window = [commits[i]]
        for j in range(i + 1, n):
            if commits[j]["timestamp"] - commits[i]["timestamp"] <= BURST_WINDOW_SECS:
                window.append(commits[j])
            else:
                break
        if len(window) >= BURST_THRESHOLD:
            for c in window:
                burst_hashes.add(c["hash"])
    return burst_hashes


def map_commits_to_hits(commits: list) -> list:
    """
    Convert parsed commits to hit dicts.
    First commit has IAT=0 → pad 1.
    Burst commits get remapped to pads 13-16.
    """
    if not commits:
        return []

    burst_hashes = detect_bursts(commits)
    hits = []

    for i, commit in enumerate(commits):
        iat = 0.0 if i == 0 else float(commit["timestamp"] - commits[i - 1]["timestamp"])
        pad = iat_to_pad(iat)

        is_burst = commit["hash"] in burst_hashes
        if is_burst:
            pad = 13 + (i % 4)

        vel     = compute_velocity(commit["files_changed"], commit["insertions"], commit["deletions"])
        mweight = msg_weight(commit["subject"])

        hits.append({
            "hash":          commit["hash"][:8],
            "full_hash":     commit["hash"],
            "timestamp":     commit["timestamp"],
            "subject":       commit["subject"],
            "pad":           pad,
            "midi_note":     PAD_MIDI[pad],
            "velocity":      vel,
            "is_burst":      is_burst,
            "files_changed": commit["files_changed"],
            "insertions":    commit["insertions"],
            "deletions":     commit["deletions"],
            "msg_weight":    mweight,
            "iat_seconds":   iat,
        })

    return hits


# =============================================================================
# COMMIT REPORT
# =============================================================================

_IAT_FMTS = [
    (3600,          lambda s: f"{int(s//60)}m"),
    (86400,         lambda s: f"{s/3600:.1f}hr"),
    (604800,        lambda s: f"{s/86400:.1f}d"),
    (2592000,       lambda s: f"{s/604800:.1f}wk"),
    (31536000,      lambda s: f"{s/2592000:.1f}mo"),
    (float("inf"),  lambda s: f"{s/31536000:.1f}yr"),
]


def iat_human(seconds: float) -> str:
    if seconds == 0:
        return "start"
    for threshold, fmt in _IAT_FMTS:
        if seconds < threshold:
            return fmt(seconds)
    return f"{seconds/31536000:.1f}yr"


def build_commit_report(repo_path: Path, hits: list, repo_name: str) -> str:
    lines = []
    lines.append("=" * 78)
    lines.append("XO_OX GIT KIT — COMMIT REPORT")
    lines.append(f"Repo     : {repo_path}")
    lines.append(f"Name     : {repo_name}")
    lines.append(f"Date     : {date.today()}")
    lines.append(f"Commits  : {len(hits)}")
    lines.append("=" * 78)
    lines.append("")

    # Pad usage summary
    pad_counts: dict = {}
    for h in hits:
        pad_counts[h["pad"]] = pad_counts.get(h["pad"], 0) + 1

    lines.append("Pad usage (inter-arrival time distribution):")
    for pad_num in sorted(pad_counts):
        count = pad_counts[pad_num]
        label = PAD_LABELS.get(pad_num, f"Pad {pad_num}")
        bar   = "#" * min(40, count)
        lines.append(f"  Pad {pad_num:>2}  ({label:<32})  {count:>4}x  {bar}")
    lines.append("")

    # Burst summary
    burst_count = sum(1 for h in hits if h["is_burst"])
    if burst_count:
        lines.append(f"Burst sessions: {burst_count} commits in rapid succession")
        lines.append("  (3+ commits within 2 hours — mapped to accent pads 13-16)")
        lines.append("")

    # Per-commit table
    lines.append(
        f"{'#':>4}  {'Hash':>8}  {'Date':>10}  {'IAT':>6}  "
        f"{'Pad':>3}  {'Vel':>3}  {'Files':>5}  {'+Ins':>5}  {'-Del':>5}  "
        f"{'Wt':>4}  {'Brst':>4}  Subject"
    )
    lines.append("-" * 110)
    for i, h in enumerate(hits):
        dt      = datetime.fromtimestamp(h["timestamp"], tz=timezone.utc).strftime("%Y-%m-%d")
        subj    = h["subject"][:40]
        burst   = "YES" if h["is_burst"] else ""
        iat_str = iat_human(h["iat_seconds"])
        wt_str  = f"{h['msg_weight']:.1f}"
        lines.append(
            f"{i+1:>4}  {h['hash']:>8}  {dt:>10}  {iat_str:>6}  "
            f"{h['pad']:>3}  {h['velocity']:>3}  {h['files_changed']:>5}  "
            f"{h['insertions']:>5}  {h['deletions']:>5}  {wt_str:>4}  {burst:>4}  {subj}"
        )

    lines.append("")
    lines.append("Mapping key:")
    lines.append("  Inter-arrival time  → pad number (1=fastest, 16=slowest)")
    lines.append("  Files changed       → base velocity (1-5=30, 6-20=60, 21-100=90, 100+=127)")
    lines.append("  Insertions          → +velocity boost (up to +20)")
    lines.append("  Deletions           → -velocity reduction (up to -15)")
    lines.append("  Msg weight (Wt)     → VelocityToVolume (short=0.3, med=0.6, long=1.0)")
    lines.append("  Burst (3+ in 2hr)   → accent pads 13-16")
    lines.append("")
    lines.append("Pad → MIDI note reference:")
    for pad_num in range(1, 17):
        label = PAD_LABELS[pad_num]
        lines.append(f"  Pad {pad_num:>2} → MIDI {PAD_MIDI[pad_num]:>3}  ({label})")
    lines.append("=" * 78)
    return "\n".join(lines)


# =============================================================================
# XPM GENERATION
# =============================================================================

def _layer_block(number: int, vel_start: int, vel_end: int,
                 sample_name: str, sample_file: str,
                 volume: float = 0.707946) -> str:
    active = "True" if sample_name else "False"
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
        f'            <SampleFile>{xml_escape(sample_file)}</SampleFile>\n'
        f'            <File>{xml_escape(sample_file)}</File>\n'
        f'            <SliceIndex>128</SliceIndex>\n'
        f'            <Direction>0</Direction>\n'
        f'            <Offset>0</Offset>\n'
        f'            <SliceStart>0</SliceStart>\n'
        f'            <SliceEnd>0</SliceEnd>\n'
        f'            <SliceLoopStart>0</SliceLoopStart>\n'
        f'            <SliceLoop>0</SliceLoop>\n'
        f'          </Layer>'
    )


def _empty_layers() -> str:
    return "\n".join(_layer_block(i, 0, 0, "", "", 0.707946) for i in range(1, 5))


def _instrument_block(instrument_num: int, voice_name: str,
                      sample_name: str, sample_file: str,
                      mute_group: int = 0) -> str:
    is_active = bool(voice_name and sample_name)
    cfg = VOICE_DEFAULTS.get(voice_name, VOICE_DEFAULTS["fx"])

    if is_active:
        layers_xml = "\n".join([
            _layer_block(1, 1,  31,  sample_name, sample_file, 0.35),
            _layer_block(2, 32, 63,  sample_name, sample_file, 0.55),
            _layer_block(3, 64, 95,  sample_name, sample_file, 0.75),
            _layer_block(4, 96, 127, sample_name, sample_file, 0.95),
        ])
    else:
        layers_xml = _empty_layers()

    mono_str    = "True" if cfg["mono"] else "False"
    oneshot_str = "True" if cfg["one_shot"] else "False"

    mute_xml = "\n".join(
        f"        <MuteTarget{i+1}>0</MuteTarget{i+1}>" for i in range(4)
    )
    if voice_name == "chat":
        mute_xml = (
            f"        <MuteTarget1>{PAD_MIDI[4]}</MuteTarget1>\n"
            + "\n".join(f"        <MuteTarget{i+1}>0</MuteTarget{i+1}>" for i in range(1, 4))
        )
    simult_xml = "\n".join(
        f"        <SimultTarget{i+1}>0</SimultTarget{i+1}>" for i in range(4)
    )

    return (
        f'      <Instrument number="{instrument_num}">\n'
        f'        <AudioRoute>\n'
        f'          <AudioRoute>0</AudioRoute>\n'
        f'          <AudioRouteSubIndex>0</AudioRouteSubIndex>\n'
        f'          <InsertsEnabled>False</InsertsEnabled>\n'
        f'        </AudioRoute>\n'
        f'        <Send1>0.000000</Send1>\n'
        f'        <Send2>0.000000</Send2>\n'
        f'        <Send3>0.000000</Send3>\n'
        f'        <Send4>0.000000</Send4>\n'
        f'        <Volume>0.707946</Volume>\n'
        f'        <Mute>False</Mute>\n'
        f'        <Pan>0.500000</Pan>\n'
        f'        <TuneCoarse>0</TuneCoarse>\n'
        f'        <TuneFine>0</TuneFine>\n'
        f'        <Mono>{mono_str}</Mono>\n'
        f'        <Polyphony>{cfg["poly"]}</Polyphony>\n'
        f'        <FilterKeytrack>0.000000</FilterKeytrack>\n'
        f'        <LowNote>0</LowNote>\n'
        f'        <HighNote>127</HighNote>\n'
        f'        <IgnoreBaseNote>False</IgnoreBaseNote>\n'
        f'        <ZonePlay>1</ZonePlay>\n'
        f'        <MuteGroup>{mute_group}</MuteGroup>\n'
        f'{mute_xml}\n'
        f'{simult_xml}\n'
        f'        <LfoPitch>0.000000</LfoPitch>\n'
        f'        <LfoCutoff>0.000000</LfoCutoff>\n'
        f'        <LfoVolume>0.000000</LfoVolume>\n'
        f'        <LfoPan>0.000000</LfoPan>\n'
        f'        <OneShot>{oneshot_str}</OneShot>\n'
        f'        <FilterType>2</FilterType>\n'
        f'        <Cutoff>{cfg["cutoff"]:.6f}</Cutoff>\n'
        f'        <Resonance>{cfg["resonance"]:.6f}</Resonance>\n'
        f'        <FilterEnvAmt>0.000000</FilterEnvAmt>\n'
        f'        <AfterTouchToFilter>0.000000</AfterTouchToFilter>\n'
        f'        <VelocityToStart>0.000000</VelocityToStart>\n'
        f'        <VelocityToFilterAttack>0.000000</VelocityToFilterAttack>\n'
        f'        <VelocityToFilter>{cfg["vtf"]:.6f}</VelocityToFilter>\n'
        f'        <VelocityToFilterEnvelope>0.000000</VelocityToFilterEnvelope>\n'
        f'        <FilterAttack>0.000000</FilterAttack>\n'
        f'        <FilterDecay>0.000000</FilterDecay>\n'
        f'        <FilterSustain>1.000000</FilterSustain>\n'
        f'        <FilterRelease>0.000000</FilterRelease>\n'
        f'        <FilterHold>0.000000</FilterHold>\n'
        f'        <FilterDecayType>True</FilterDecayType>\n'
        f'        <FilterADEnvelope>True</FilterADEnvelope>\n'
        f'        <VolumeHold>0.000000</VolumeHold>\n'
        f'        <VolumeDecayType>True</VolumeDecayType>\n'
        f'        <VolumeADEnvelope>True</VolumeADEnvelope>\n'
        f'        <VolumeAttack>{cfg["attack"]:.6f}</VolumeAttack>\n'
        f'        <VolumeDecay>{cfg["decay"]:.6f}</VolumeDecay>\n'
        f'        <VolumeSustain>{cfg["sustain"]:.6f}</VolumeSustain>\n'
        f'        <VolumeRelease>{cfg["release"]:.6f}</VolumeRelease>\n'
        f'        <VelocityToPitch>{cfg["vtp"]:.6f}</VelocityToPitch>\n'
        f'        <VelocityToVolumeAttack>0.000000</VelocityToVolumeAttack>\n'
        f'        <VelocitySensitivity>1.000000</VelocitySensitivity>\n'
        f'        <VelocityToPan>0.000000</VelocityToPan>\n'
        f'        <LFO>\n'
        f'          <Speed>0.000000</Speed>\n'
        f'          <Amount>0.000000</Amount>\n'
        f'          <Type>0</Type>\n'
        f'          <Sync>False</Sync>\n'
        f'          <Retrigger>True</Retrigger>\n'
        f'        </LFO>\n'
        f'        <Layers>\n'
        f'{layers_xml}\n'
        f'        </Layers>\n'
        f'      </Instrument>'
    )


def build_xpm(slug: str, repo_name: str, hits: list) -> str:
    """Build complete drum program XPM XML. Used pads get active instruments."""
    prog_name  = xml_escape(f"XO_OX-GIT-{slug.upper()}")
    used_pads  = set(h["pad"] for h in hits)

    instrument_parts = []
    for i in range(128):
        pad_for_note = None
        for pad_num, midi in PAD_MIDI.items():
            if midi == i and pad_num in used_pads:
                pad_for_note = pad_num
                break
        if pad_for_note is not None:
            v_name = PAD_VOICE[pad_for_note]
            s_name = f"{slug}_pad{pad_for_note}"
            s_file = f"{s_name}.wav"
            mg     = VOICE_DEFAULTS.get(v_name, VOICE_DEFAULTS["fx"])["mute_group"]
            instrument_parts.append(
                _instrument_block(i, v_name, s_name, s_file, mute_group=mg)
            )
        else:
            instrument_parts.append(_instrument_block(i, "", "", "", mute_group=0))

    instruments_xml = "\n".join(instrument_parts)

    pad_note_entries = [
        f'        <Pad number="{pad_num}" note="{PAD_MIDI[pad_num]}"/>  '
        f'<!-- {xml_escape(PAD_LABELS[pad_num])} -->'
        for pad_num in sorted(used_pads)
    ]
    pad_note_xml = "\n".join(pad_note_entries)

    pad_group_entries = []
    for pad_num in sorted(used_pads):
        mg = VOICE_DEFAULTS.get(PAD_VOICE[pad_num], VOICE_DEFAULTS["fx"])["mute_group"]
        if mg > 0:
            pad_group_entries.append(f'        <Pad number="{pad_num}" group="{mg}"/>')
    pad_group_xml = "\n".join(pad_group_entries)

    pad_json = json.dumps(
        {"ProgramPads": {"Universal": {"value0": False},
                         "Type":      {"value0": 5},
                         "universalPad": 32512}},
        separators=(",", ":"),
    )

    return (
        '<?xml version="1.0" encoding="UTF-8"?>\n\n'
        '<MPCVObject>\n'
        '  <Version>\n'
        '    <File_Version>1.7</File_Version>\n'
        '    <Application>MPC-V</Application>\n'
        '    <Application_Version>2.10.0.0</Application_Version>\n'
        '    <Platform>OSX</Platform>\n'
        '  </Version>\n'
        '  <Program type="Drum">\n'
        f'    <Name>{prog_name}</Name>\n'
        f'    <!-- Git Kit: {xml_escape(repo_name)} | {len(hits)} commits -->\n'
        f'    <!-- Pad = inter-arrival time bin | Velocity = commit size -->\n'
        f'    <ProgramPads>{pad_json}</ProgramPads>\n'
        '    <PadNoteMap>\n'
        f'{pad_note_xml}\n'
        '    </PadNoteMap>\n'
        '    <PadGroupMap>\n'
        f'{pad_group_xml}\n'
        '    </PadGroupMap>\n'
        '    <QLinks>\n'
        '      <QLink number="1">\n'
        '        <Name>DENSITY</Name>\n'
        '        <Parameter>FilterCutoff</Parameter>\n'
        '        <Min>0.200000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="2">\n'
        '        <Name>PITCH</Name>\n'
        '        <Parameter>TuneCoarse</Parameter>\n'
        '        <Min>-12</Min>\n'
        '        <Max>12</Max>\n'
        '      </QLink>\n'
        '      <QLink number="3">\n'
        '        <Name>BURST</Name>\n'
        '        <Parameter>VelocitySensitivity</Parameter>\n'
        '        <Min>0.500000</Min>\n'
        '        <Max>2.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="4">\n'
        '        <Name>SPACE</Name>\n'
        '        <Parameter>Send1</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>0.700000</Max>\n'
        '      </QLink>\n'
        '    </QLinks>\n'
        '    <Instruments>\n'
        f'{instruments_xml}\n'
        '    </Instruments>\n'
        '  </Program>\n'
        '</MPCVObject>\n'
    )


# =============================================================================
# MANIFEST
# =============================================================================

def build_manifest(slug: str, repo_name: str, repo_path: Path, hits: list) -> dict:
    pad_usage: dict = {}
    for h in hits:
        p = str(h["pad"])
        pad_usage[p] = pad_usage.get(p, 0) + 1

    return {
        "tool":        "xpn_git_kit",
        "version":     "1.0",
        "date":        str(date.today()),
        "slug":        slug,
        "repo_name":   repo_name,
        "repo_path":   str(repo_path),
        "commits":     len(hits),
        "burst_commits": sum(1 for h in hits if h["is_burst"]),
        "total_files_changed": sum(h["files_changed"] for h in hits),
        "total_insertions":    sum(h["insertions"] for h in hits),
        "total_deletions":     sum(h["deletions"] for h in hits),
        "pad_usage": pad_usage,
        "golden_rules": {"KeyTrack": True, "RootNote": 0, "empty_layer_VelStart": 0},
        "pad_map": {
            str(pad): {"midi": PAD_MIDI[pad], "voice": PAD_VOICE[pad], "label": PAD_LABELS[pad]}
            for pad in range(1, 17)
        },
        "commit_hits": [
            {
                "index":         i + 1,
                "hash":          h["hash"],
                "date":          datetime.fromtimestamp(h["timestamp"], tz=timezone.utc).strftime("%Y-%m-%d"),
                "subject":       h["subject"][:80],
                "pad":           h["pad"],
                "midi_note":     h["midi_note"],
                "velocity":      h["velocity"],
                "is_burst":      h["is_burst"],
                "files_changed": h["files_changed"],
                "insertions":    h["insertions"],
                "deletions":     h["deletions"],
                "iat_seconds":   h["iat_seconds"],
            }
            for i, h in enumerate(hits)
        ],
    }


# =============================================================================
# ZIP PACKAGING
# =============================================================================

def write_kit_zip(output_dir: Path, slug: str, repo_name: str,
                  repo_path: Path, hits: list) -> Path:
    """Package XPM + commit_report.txt + manifest.json + README into a ZIP."""
    output_dir.mkdir(parents=True, exist_ok=True)
    zip_path = output_dir / f"XO_OX-GIT-{slug.upper()}.zip"

    xpm_xml  = build_xpm(slug, repo_name, hits)
    report   = build_commit_report(repo_path, hits, repo_name)
    manifest = build_manifest(slug, repo_name, repo_path, hits)

    used_pads = sorted(set(h["pad"] for h in hits))
    readme_lines = [
        f"XO_OX GIT KIT — {repo_name}",
        "=" * 60,
        "",
        "Place WAV samples next to the .xpm file.",
        "Only pads with commits in the analyzed history are active.",
        "",
        "Active pad → WAV file mapping:",
    ]
    for pad_num in used_pads:
        readme_lines.append(
            f"  Pad {pad_num:>2} (MIDI {PAD_MIDI[pad_num]:>3}) → {slug}_pad{pad_num}.wav"
            f"  [{PAD_LABELS[pad_num]}]"
        )
    readme_lines += [
        "",
        "Velocity mapping:",
        "  1-5 files     → vel  30  (quiet tap)",
        "  6-20 files    → vel  60  (medium hit)",
        "  21-100 files  → vel  90  (strong hit)",
        "  100+ files    → vel 127  (maximum impact)",
        "  +insertions   → up to +20 velocity boost",
        "  -deletions    → up to -15 velocity reduction",
        "",
        "Burst sessions (3+ commits in 2hrs) → pads 13-16 accent layer.",
        "",
        "See commit_report.txt for the full timeline.",
        "See manifest.json for machine-readable data.",
    ]

    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
        zf.writestr(f"{slug}/{slug}.xpm",          xpm_xml)
        zf.writestr(f"{slug}/commit_report.txt",   report)
        zf.writestr(f"{slug}/manifest.json",       json.dumps(manifest, indent=2))
        zf.writestr(f"{slug}/README.txt",          "\n".join(readme_lines) + "\n")

    return zip_path


# =============================================================================
# CLI
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Git Kit — git commit history → MPC drum programs"
    )
    parser.add_argument(
        "--repo",
        default=".",
        help=(
            "Git repo path. '.' = current dir; absolute path; or shorthand name "
            "(auto-resolved from ~/Documents/GitHub/). Default: current directory."
        ),
    )
    parser.add_argument(
        "--output", "-o",
        default="./out",
        help="Output directory (default: ./out)",
    )
    parser.add_argument(
        "--max-commits",
        type=int,
        default=200,
        help="Maximum commits to analyze (default: 200)",
    )
    args = parser.parse_args()

    repo_path = find_repo(args.repo)
    if not repo_path.exists():
        print(f"ERROR: Repository path not found: {repo_path}", file=sys.stderr)
        sys.exit(1)

    repo_name = repo_path.name
    slug      = re.sub(r"[^a-zA-Z0-9_-]", "_", repo_name.lower())[:30]

    print(f"Analyzing : {repo_path}")
    print(f"Max commits: {args.max_commits}")

    try:
        commits = parse_git_log(repo_path, args.max_commits)
    except RuntimeError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(1)

    if not commits:
        print("ERROR: No commits found.", file=sys.stderr)
        sys.exit(1)

    print(f"Found {len(commits)} commits")

    hits = map_commits_to_hits(commits)

    burst_count = sum(1 for h in hits if h["is_burst"])
    pads_used   = len(set(h["pad"] for h in hits))
    print(f"Mapped to {pads_used} active pads  |  {burst_count} burst commits (pads 13-16)")

    output_dir = Path(args.output)
    zip_path   = write_kit_zip(output_dir, slug, repo_name, repo_path, hits)
    print(f"Written   : {zip_path}")


if __name__ == "__main__":
    main()
