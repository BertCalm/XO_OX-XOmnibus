#!/usr/bin/env python3
"""
XPN Keygroup Exporter — XO_OX Designs
Generates Akai MPC Keygroup XPM programs from XO_OX WAV sample sets.

WAV naming convention expected:
    {preset_slug}__{NOTE}__{vel}.WAV
    e.g. Deep_Drift__C2__v1.WAV
         Deep_Drift__C2__v2.WAV   (velocity layer 2)
         Deep_Drift__Eb3__v1.WAV

NOTE format: note name + octave, e.g. C2, F#3, Bb4, Eb2
vel: v1–v4 (velocity layers, ascending velocity)

Velocity layer ranges (4-layer standard):
    v1 → 0–31    (pp)
    v2 → 32–63   (mp)
    v3 → 64–95   (mf)
    v4 → 96–127  (ff)

XPM Rules (from CLAUDE.md — never break these):
    - KeyTrack  = True   (samples transpose across zones)
    - RootNote  = 0      (MPC auto-detect convention)
    - VelStart  = 0      on empty/unused layers (prevents ghost triggering)

Usage (standalone):
    python3 xpn_keygroup_export.py --wavs-dir /path/to/wavs \\
        --preset "Deep Drift" --engine Odyssey --output /path/to/out

Usage (library):
    from xpn_keygroup_export import build_keygroup_wav_map, generate_keygroup_xpm
"""

import argparse
import re
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from xml.sax.saxutils import escape as xml_escape


# ---------------------------------------------------------------------------
# Note name → MIDI number
# ---------------------------------------------------------------------------

_NOTE_NAMES = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}
_SHARP_FLAT = {"#": 1, "b": -1, "s": 1}  # "s" as fallback for Fs, Bs etc.

_NOTE_RE = re.compile(r'^([A-Ga-g])([#bs]?)(-?\d+)$')


def note_name_to_midi(note_str: str) -> Optional[int]:
    """Convert a note name like 'C2', 'F#3', 'Bb4' to MIDI number (C-1=0)."""
    m = _NOTE_RE.match(note_str.strip())
    if not m:
        return None
    letter, acc, octave = m.group(1).upper(), m.group(2), int(m.group(3))
    semitone = _NOTE_NAMES[letter] + _SHARP_FLAT.get(acc, 0)
    return (octave + 1) * 12 + semitone


def midi_to_note_name(midi: int) -> str:
    """Convert MIDI number to note name (C-1=0 convention)."""
    names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
    octave = (midi // 12) - 1
    note = names[midi % 12]
    return f"{note}{octave}"


# ---------------------------------------------------------------------------
# Velocity layer helpers
# ---------------------------------------------------------------------------

# Standard 4-layer velocity map
VEL_LAYERS = {
    "v1": (0,   31),
    "v2": (32,  63),
    "v3": (64,  95),
    "v4": (96, 127),
}

# Single-layer fallback (no velocity switching)
VEL_SINGLE = {"v1": (1, 127)}  # VelStart=1 not 0 (Rex Rule #3)


def _vel_range(vel_tag: str, all_vel_tags: List[str]) -> Tuple[int, int]:
    """Return (vel_start, vel_end) for a velocity tag given its peers."""
    if vel_tag in VEL_LAYERS and len(all_vel_tags) > 1:
        return VEL_LAYERS[vel_tag]
    # If only one velocity layer, use full range
    return (0, 127)


# ---------------------------------------------------------------------------
# WAV map builder
# ---------------------------------------------------------------------------

def build_keygroup_wav_map(wavs_dir: Path, preset_slug: str) -> Dict[str, str]:
    """
    Scan wavs_dir for files matching {preset_slug}__*__*.WAV.
    Returns {stem: filename} mapping.

    stem format: {preset_slug}__{NOTE}__{vel}
    """
    wav_map: Dict[str, str] = {}
    pattern = f"{preset_slug}__*.WAV"
    for wf in sorted(wavs_dir.glob(pattern)):
        wav_map[wf.stem] = wf.name
    # Also accept lowercase .wav
    for wf in sorted(wavs_dir.glob(f"{preset_slug}__*.wav")):
        if wf.stem not in wav_map:
            wav_map[wf.stem] = wf.name
    return wav_map


def _parse_stem(stem: str) -> Optional[Tuple[str, int, str]]:
    """
    Parse a WAV stem like 'Deep_Drift__C2__v1'.
    Returns (note_str, midi_num, vel_tag) or None if unparseable.
    """
    parts = stem.split("__")
    if len(parts) < 3:
        return None
    note_str = parts[-2]
    vel_tag = parts[-1]
    midi = note_name_to_midi(note_str)
    if midi is None:
        # Try interpreting as a raw MIDI number
        try:
            midi = int(note_str)
            note_str = midi_to_note_name(midi)
        except ValueError:
            return None
    return (note_str, midi, vel_tag)


# ---------------------------------------------------------------------------
# Zone layout
# ---------------------------------------------------------------------------

def _compute_zones(
    root_midi_notes: List[int],
    full_range: Tuple[int, int] = (0, 127),
) -> List[Tuple[int, int, int]]:
    """
    Given a sorted list of root MIDI notes, compute (low, root, high) zone
    triples that span [full_range[0], full_range[1]] with no gaps.

    Each zone's boundary is the midpoint between adjacent roots.
    """
    if not root_midi_notes:
        return []

    notes = sorted(set(root_midi_notes))
    zones = []
    lo = full_range[0]

    for i, root in enumerate(notes):
        if i < len(notes) - 1:
            next_root = notes[i + 1]
            hi = (root + next_root) // 2  # midpoint
        else:
            hi = full_range[1]
        zones.append((lo, root, hi))
        lo = hi + 1

    return zones


# ---------------------------------------------------------------------------
# XPM XML generation
# ---------------------------------------------------------------------------

_XPM_HEADER = """<?xml version="1.0" encoding="UTF-8"?>
<MPCVObject>
  <Version>
    <File_Version>1.7</File_Version>
    <Application>MPC-V</Application>
    <Application_Version>2.10.0.0</Application_Version>
    <Platform>OSX</Platform>
  </Version>"""


def _fmt(v: float) -> str:
    return f"{v:.6f}"


def generate_keygroup_xpm(
    preset_name: str,
    engine: str,
    wav_map: Dict[str, str],
) -> str:
    """
    Generate a complete Keygroup XPM from a wav_map.

    wav_map: {stem: filename} as returned by build_keygroup_wav_map()

    If wav_map is empty, returns a single-instrument program with no layers
    that MPC can load (though it will be silent until WAVs are present).
    """
    prog_name = xml_escape(f"XO_OX-{engine}-{preset_name}"[:32])  # MPC 32-char limit

    # --- Parse all stems ---------------------------------------------------
    # Group by MIDI note: {midi: {vel_tag: (stem, filename)}}
    by_note: Dict[int, Dict[str, Tuple[str, str]]] = {}
    for stem, filename in wav_map.items():
        parsed = _parse_stem(stem)
        if not parsed:
            continue
        _, midi, vel_tag = parsed
        if midi not in by_note:
            by_note[midi] = {}
        by_note[midi][vel_tag] = (stem, filename)

    # --- Compute zones -----------------------------------------------------
    root_notes = sorted(by_note.keys())
    zones = _compute_zones(root_notes) if root_notes else [(0, 60, 127)]

    # --- Build instrument XML ----------------------------------------------
    instruments_xml = ""
    num_instruments = len(zones)

    for inst_idx, (low, root, high) in enumerate(zones):
        vel_layers = by_note.get(root, {})
        all_vel_tags = sorted(vel_layers.keys())
        has_samples = bool(vel_layers)

        layers_xml = ""
        if vel_layers:
            for layer_num, vel_tag in enumerate(all_vel_tags, start=1):
                stem, filename = vel_layers[vel_tag]
                vel_start, vel_end = _vel_range(vel_tag, all_vel_tags)
                # File path: XPN-relative (Rex Rule #5)
                file_path = f"Samples/{prog_name}/{filename}" if filename else ""
                layers_xml += (
                    f"            <Layer number=\"{layer_num}\">\n"
                    f"              <Active>True</Active>\n"
                    f"              <Volume>1.000000</Volume>\n"
                    f"              <Pan>0.500000</Pan>\n"
                    f"              <Pitch>0.000000</Pitch>\n"
                    f"              <TuneCoarse>0</TuneCoarse>\n"
                    f"              <TuneFine>0</TuneFine>\n"
                    f"              <SampleName>{xml_escape(stem)}</SampleName>\n"
                    f"              <SampleFile>{xml_escape(filename)}</SampleFile>\n"
                    f"              <File>{xml_escape(file_path)}</File>\n"
                    f"              <RootNote>0</RootNote>\n"
                    f"              <KeyTrack>True</KeyTrack>\n"
                    f"              <OneShot>False</OneShot>\n"
                    f"              <Loop>False</Loop>\n"
                    f"              <LoopStart>0</LoopStart>\n"
                    f"              <LoopEnd>0</LoopEnd>\n"
                    f"              <LoopXFade>0</LoopXFade>\n"
                    f"              <VolumeAttack>{_fmt(0)}</VolumeAttack>\n"
                    f"              <VolumeDecay>{_fmt(0)}</VolumeDecay>\n"
                    f"              <VolumeSustain>{_fmt(1)}</VolumeSustain>\n"
                    f"              <VolumeRelease>{_fmt(0.5)}</VolumeRelease>\n"
                    f"              <VelStart>{vel_start}</VelStart>\n"
                    f"              <VelEnd>{vel_end}</VelEnd>\n"
                    f"            </Layer>\n"
                )
        else:
            # Empty placeholder — Rex Rule #3: VelStart=0, Active=False
            layers_xml = (
                "            <Layer number=\"1\">\n"
                "              <Active>False</Active>\n"
                "              <SampleName></SampleName>\n"
                "              <SampleFile></SampleFile>\n"
                "              <File></File>\n"
                "              <VelStart>0</VelStart>\n"
                "              <VelEnd>0</VelEnd>\n"
                "            </Layer>\n"
            )

        active_str = "True" if has_samples else "False"
        instruments_xml += (
            f"      <Instrument number=\"{inst_idx}\">\n"
            f"        <Active>{active_str}</Active>\n"
            f"        <Volume>1.000000</Volume>\n"
            f"        <Pan>0.500000</Pan>\n"
            f"        <Tune>0</Tune>\n"
            f"        <Transpose>0</Transpose>\n"
            f"        <VolumeAttack>{_fmt(0)}</VolumeAttack>\n"
            f"        <VolumeHold>{_fmt(0)}</VolumeHold>\n"
            f"        <VolumeDecay>{_fmt(0)}</VolumeDecay>\n"
            f"        <VolumeSustain>{_fmt(1)}</VolumeSustain>\n"
            f"        <VolumeRelease>{_fmt(0.5)}</VolumeRelease>\n"
            f"        <FilterType>2</FilterType>\n"
            f"        <Cutoff>1.000000</Cutoff>\n"
            f"        <Resonance>0.000000</Resonance>\n"
            f"        <FilterEnvAmt>0.000000</FilterEnvAmt>\n"
            f"        <LowNote>{low}</LowNote>\n"
            f"        <HighNote>{high}</HighNote>\n"
            f"        <RootNote>0</RootNote>\n"
            f"        <KeyTrack>True</KeyTrack>\n"
            f"        <OneShot>False</OneShot>\n"
            f"        <Layers>\n"
            f"{layers_xml}"
            f"        </Layers>\n"
            f"      </Instrument>\n"
        )

    # Q-Link assignments for keygroup programs (Atlas bridge §9)
    qlink_xml = (
        '    <QLinks>\n'
        '      <QLink number="1">\n'
        '        <Name>TONE</Name>\n'
        '        <Parameter>FilterCutoff</Parameter>\n'
        '        <Min>0.200000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="2">\n'
        '        <Name>ATTACK</Name>\n'
        '        <Parameter>VolumeAttack</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="3">\n'
        '        <Name>RELEASE</Name>\n'
        '        <Parameter>VolumeRelease</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>2.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="4">\n'
        '        <Name>SPACE</Name>\n'
        '        <Parameter>Send1</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>0.700000</Max>\n'
        '      </QLink>\n'
        '    </QLinks>\n'
    )

    xpm = (
        f"{_XPM_HEADER}\n"
        f"  <Program type=\"Keygroup\">\n"
        f"    <Name>{prog_name}</Name>\n"
        f"    <KeygroupNumKeygroups>{num_instruments}</KeygroupNumKeygroups>\n"
        f"    <KeygroupPitchBendRange>2</KeygroupPitchBendRange>\n"
        f"    <KeygroupWheelToLfo>0.000000</KeygroupWheelToLfo>\n"
        f"{qlink_xml}"
        f"    <Instruments>\n"
        f"{instruments_xml}"
        f"    </Instruments>\n"
        f"  </Program>\n"
        f"</MPCVObject>\n"
    )
    return xpm


# ---------------------------------------------------------------------------
# Standalone CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Generate Akai MPC Keygroup XPM from XO_OX WAV samples"
    )
    parser.add_argument("--wavs-dir",  required=True,
                        help="Directory containing WAV files")
    parser.add_argument("--preset",    required=True,
                        help="Preset name (e.g. 'Deep Drift')")
    parser.add_argument("--engine",    default="Unknown",
                        help="Engine name (e.g. Odyssey)")
    parser.add_argument("--output",    required=True,
                        help="Output directory for .xpm file")
    parser.add_argument("--dry-run",   action="store_true",
                        help="Print XPM to stdout, do not write file")
    args = parser.parse_args()

    wavs_dir   = Path(args.wavs_dir)
    output_dir = Path(args.output)
    preset_slug = args.preset.replace(" ", "_")

    wav_map = build_keygroup_wav_map(wavs_dir, preset_slug)
    print(f"Found {len(wav_map)} WAV files for '{args.preset}'")

    xpm = generate_keygroup_xpm(args.preset, args.engine, wav_map)

    if args.dry_run:
        print(xpm)
        return

    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / f"{preset_slug}.xpm"
    out_path.write_text(xpm, encoding="utf-8")
    print(f"Written: {out_path}")


if __name__ == "__main__":
    main()
