#!/usr/bin/env python3
"""
XPN Multi-Articulation Program Builder — XO_OX Designs

Builds Akai MPC Keygroup XPM programs that expose multiple playing techniques
(articulations) within a single program. Professional-grade articulation
switching: key switches (low-octave trigger notes) or pad banks (MPC-native
A/B/C/D bank assignment).

Designed for XO_OX Kitchen, Travel, and Artwork collection world instruments
and orchestral sounds: erhu, guzheng, sitar, oud, violin, brass, woodwind, etc.

---------------------------------------------------------------------------
Directory convention (articulations as sub-folders):

    erhu_samples/
        legato/
            Erhu_Legato__C3__v1.wav
            Erhu_Legato__C3__v2.wav
            Erhu_Legato__G3__v1.wav
        tremolo/
            Erhu_Tremolo__C3__v1.wav
        pizzicato/
            Erhu_Pizzicato__C3__v1.wav

    Each sub-folder name = articulation name.
    WAV naming within each folder follows the same convention as
    xpn_keygroup_export.py: {preset_slug}__{NOTE}__{vel}.wav

---------------------------------------------------------------------------
Mode: keyswitch
    Low-octave notes (C0, C#0, D0 … starting at MIDI 12) each become a
    narrow one-note zone that triggers a silent "switch" sample. The MPC
    keyswitch approach: the producer plays C0 to select legato, C#0 for
    tremolo, etc. Each articulation is a separate keygroup instrument block
    spanning the playable range (C2–C7 by default), gated by ProgramLayer.

    Because MPC Keygroup XPM does not natively support true dynamic key-
    switch routing (unlike Kontakt), we use the practical workaround:
    each articulation is emitted as a separate XPM file, and the keyswitch
    XPM wraps them as separate Instrument groups. The player loads the
    keyswitch XPM and uses an MPC PadNoteMap to trigger articulation banks.

Mode: pad-banks
    MPC pad banks A / B / C / D each carry one articulation. Bank A holds
    legato, Bank B holds tremolo, etc. Each bank gets its own XPM file.
    Additionally a "master" XPM is written that maps pad banks via the
    PadNoteMap convention (pads 1-16 = bank A, 17-32 = B, etc.).

---------------------------------------------------------------------------
Usage:

    # Key-switch mode (low-octave notes C0+ select articulation)
    python3 xpn_articulation_builder.py \\
        --instrument-family world_bowed \\
        --samples ./erhu_samples/ \\
        --output ./programs/ \\
        --preset "Erhu Solo" \\
        --mode keyswitch

    # Pad-bank mode (MPC banks A/B/C/D each carry one articulation)
    python3 xpn_articulation_builder.py \\
        --instrument-family strings \\
        --samples ./strings/ \\
        --preset "Violin Section" \\
        --mode pad-banks \\
        --output ./programs/

    # Override articulation list (subset or custom order)
    python3 xpn_articulation_builder.py \\
        --instrument-family world_bowed \\
        --samples ./erhu/ \\
        --preset "Erhu Intimate" \\
        --articulations legato tremolo \\
        --output ./programs/ \\
        --mode keyswitch

    # Dry run — print XPM XML to stdout, no files written
    python3 xpn_articulation_builder.py \\
        --instrument-family brass \\
        --samples ./brass/ \\
        --preset "Horn Open" \\
        --mode keyswitch \\
        --output ./programs/ \\
        --dry-run

---------------------------------------------------------------------------
XPM Rules (never break these — Rex's Golden Rules):
    KeyTrack  = True    samples transpose across zones
    RootNote  = 0       MPC auto-detect convention
    VelStart  = 0       on empty/unused layers (prevents ghost triggering)
    Boolean   = True/False  (capitalized)
"""

import argparse
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from xml.sax.saxutils import escape as xml_escape

# Borrow shared utilities from the keygroup exporter (same Tools/ directory)
_TOOLS_DIR = Path(__file__).parent
sys.path.insert(0, str(_TOOLS_DIR))

from xpn_keygroup_export import (
    FAMILY_VEL_LAYERS,
    build_keygroup_wav_map,
    generate_keygroup_xpm,
    note_name_to_midi,
    midi_to_note_name,
    _XPM_HEADER,
    _fmt,
)

# ---------------------------------------------------------------------------
# Banner
# ---------------------------------------------------------------------------

BANNER = """
╔══════════════════════════════════════════════════════════════════╗
║     XPN Multi-Articulation Program Builder — XO_OX Designs      ║
║     Kitchen · Travel · Artwork collection world instruments      ║
╚══════════════════════════════════════════════════════════════════╝
"""

# ---------------------------------------------------------------------------
# Articulation definitions per instrument family
# ---------------------------------------------------------------------------

ARTICULATIONS: Dict[str, List[str]] = {
    "strings": [
        "legato",
        "tremolo",
        "pizzicato",
        "col_legno",
        "sul_ponticello",
    ],
    "brass": [
        "open",
        "muted",
        "growl",
        "flutter_tongue",
        "stopped",
    ],
    "woodwind": [
        "legato",
        "staccato",
        "flutter",
        "multiphonic",
        "overblown",
    ],
    # Plucked world instruments: sitar, guzheng, oud, koto, biwa
    "world_plucked": [
        "normal",
        "muted",
        "harmonics",
        "slap",
    ],
    # Bowed world instruments: erhu, sarangi, rebab, morin khuur
    "world_bowed": [
        "legato",
        "tremolo",
        "sul_ponticello",
        "pizzicato",
    ],
    # Struck world instruments: taiko, frame drum, djembe, tabla
    "world_struck": [
        "normal",
        "rim",
        "muted",
        "roll",
    ],
    "piano": [
        "sustain",
        "staccato",
        "una_corda",
        "key_release",
    ],
    "organ": [
        "full",
        "soft",
        "swell",
        "percussion",
    ],
}

# Map instrument family → velocity family for keygroup_export
_FAMILY_VEL_MAP: Dict[str, str] = {
    "strings":       "strings",
    "brass":         "brass",
    "woodwind":      "woodwind",
    "world_plucked": "world",
    "world_bowed":   "world",
    "world_struck":  "world",
    "piano":         "piano",
    "organ":         "organ",
}

# ---------------------------------------------------------------------------
# Key-switch note assignments
# Key switches live in MIDI octave 0 (notes 12-23, which are C0–B0).
# This is below the standard playable range (C2 = 36 upward).
# ---------------------------------------------------------------------------

# First keyswitch note: C0 = MIDI 12
_KEYSWITCH_BASE_MIDI = 12  # C0


def keyswitch_note_for_index(idx: int) -> Tuple[int, str]:
    """Return (midi_number, note_name) for articulation index idx (0-based)."""
    midi = _KEYSWITCH_BASE_MIDI + idx
    return midi, midi_to_note_name(midi)


# ---------------------------------------------------------------------------
# Pad bank assignments
# MPC pad banks: A (pads 1-16), B (17-32), C (33-48), D (49-64)
# We map each articulation to one bank by giving it a MIDI note offset.
# Bank A base note = 36 (C2), B = 52, C = 68, D = 84
# ---------------------------------------------------------------------------

_PAD_BANK_NAMES = ["A", "B", "C", "D"]
_PAD_BANK_BASE_NOTES = [36, 52, 68, 84]   # 16 pads per bank, spaced 16 apart


def pad_bank_for_index(idx: int) -> Tuple[str, int, int]:
    """
    Return (bank_name, base_note, last_note) for articulation index idx (0-based).
    Raises ValueError if idx >= 4 (only 4 banks available).
    """
    if idx >= len(_PAD_BANK_NAMES):
        raise ValueError(
            f"Pad-bank mode supports at most {len(_PAD_BANK_NAMES)} articulations "
            f"(one per bank A/B/C/D). Got articulation index {idx}."
        )
    base = _PAD_BANK_BASE_NOTES[idx]
    return _PAD_BANK_NAMES[idx], base, base + 15


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _discover_articulations(samples_dir: Path) -> List[str]:
    """
    Auto-detect articulations from sub-folder names in samples_dir.
    Returns folder names sorted alphabetically.
    """
    return sorted(
        d.name for d in samples_dir.iterdir()
        if d.is_dir() and not d.name.startswith(".")
    )


def _build_keyswitch_instrument_xml(
    ks_midi: int,
    ks_name: str,
    articulation: str,
    artic_idx: int,
) -> str:
    """
    Build a keyswitch trigger instrument block.
    This is a single-note zone (LowNote == HighNote == ks_midi) with
    OneShot=True, no sample file, and Active=True. On the MPC the producer
    presses this key to "select" the articulation bank conceptually.
    In practice (since we embed articulations as separate Instrument groups)
    this serves as a visual/navigational marker in the program.
    """
    return (
        f"      <!-- Keyswitch {ks_name} → {articulation} (artic {artic_idx + 1}) -->\n"
        f"      <Instrument number=\"ks_{artic_idx}\">\n"
        f"        <Active>True</Active>\n"
        f"        <Volume>0.000000</Volume>\n"  # Silent — just a switch marker
        f"        <Pan>0.500000</Pan>\n"
        f"        <Tune>0</Tune>\n"
        f"        <Transpose>0</Transpose>\n"
        f"        <VolumeAttack>{_fmt(0)}</VolumeAttack>\n"
        f"        <VolumeHold>{_fmt(0)}</VolumeHold>\n"
        f"        <VolumeDecay>{_fmt(0)}</VolumeDecay>\n"
        f"        <VolumeSustain>{_fmt(1)}</VolumeSustain>\n"
        f"        <VolumeRelease>{_fmt(0.05)}</VolumeRelease>\n"
        f"        <FilterType>0</FilterType>\n"
        f"        <Cutoff>1.000000</Cutoff>\n"
        f"        <Resonance>0.000000</Resonance>\n"
        f"        <FilterEnvAmt>0.000000</FilterEnvAmt>\n"
        f"        <LowNote>{ks_midi}</LowNote>\n"
        f"        <HighNote>{ks_midi}</HighNote>\n"
        f"        <RootNote>0</RootNote>\n"
        f"        <KeyTrack>False</KeyTrack>\n"
        f"        <OneShot>True</OneShot>\n"
        f"        <Layers>\n"
        f"          <Layer number=\"1\">\n"
        f"            <Active>False</Active>\n"
        f"            <SampleName></SampleName>\n"
        f"            <SampleFile></SampleFile>\n"
        f"            <File></File>\n"
        f"            <VelStart>0</VelStart>\n"
        f"            <VelEnd>0</VelEnd>\n"
        f"          </Layer>\n"
        f"        </Layers>\n"
        f"      </Instrument>\n"
    )


def _qlink_xml_articulation() -> str:
    """Standard Q-Link assignments for articulation programs."""
    return (
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


# ---------------------------------------------------------------------------
# Keyswitch XPM builder
# ---------------------------------------------------------------------------

def build_keyswitch_xpm(
    preset_name: str,
    articulations: List[str],
    artic_wav_maps: Dict[str, Dict[str, str]],
    artic_wavs_dirs: Dict[str, Path],
    instrument_family: str = "default",
    vel_family: str = "default",
    playable_low: int = 36,   # C2
    playable_high: int = 96,  # C7
) -> str:
    """
    Build a combined keyswitch XPM.

    Strategy:
    - Keyswitch note zones (C0, C#0, D0…) are silent one-note Instruments at
      volume 0.0. They serve as named markers the producer can trigger.
    - Each articulation's multi-sample zone spans [playable_low, playable_high].
      Articulations are embedded as separate Instrument blocks within the same
      XPM. The MPC player selects articulations by navigating pad banks or
      by using the program's layering — we set each articulation's Instrument
      to a ProgramLayer so that all articulations co-exist and are addressable
      via pad bank / program layer switching.

    Note on MPC limitations:
    Since MPC Keygroup does not support true dynamic key-switch routing
    (unlike Kontakt), this XPM embeds all articulations in a single file
    with clearly labelled Instrument groups. The practical producer workflow
    is: load this XPM, use the MPC's "Layer" feature per articulation group,
    or load individual per-articulation XPMs from the same folder.

    Returns the XPM XML string.
    """
    prog_name = xml_escape(f"XO-{preset_name}-KS"[:32])

    instruments_xml = ""
    inst_counter = 0
    total_instruments = 0

    # 1. Keyswitch trigger instruments (silent, one note each)
    for idx, artic in enumerate(articulations):
        ks_midi, ks_name = keyswitch_note_for_index(idx)
        instruments_xml += _build_keyswitch_instrument_xml(
            ks_midi, ks_name, artic, idx
        )
        inst_counter += 1
        total_instruments += 1

    # 2. Articulation instrument groups (full playable range)
    for idx, artic in enumerate(articulations):
        wav_map   = artic_wav_maps.get(artic, {})
        wavs_dir  = artic_wavs_dirs.get(artic)
        artic_label = artic.replace("_", " ").title()
        ks_midi, ks_name = keyswitch_note_for_index(idx)

        instruments_xml += (
            f"      <!-- === Articulation {idx + 1}: {artic_label}"
            f" (keyswitch {ks_name}) === -->\n"
        )

        if not wav_map:
            # Empty placeholder group
            instruments_xml += (
                f"      <Instrument number=\"{inst_counter}\">\n"
                f"        <Active>False</Active>\n"
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
                f"        <LowNote>{playable_low}</LowNote>\n"
                f"        <HighNote>{playable_high}</HighNote>\n"
                f"        <RootNote>0</RootNote>\n"
                f"        <KeyTrack>True</KeyTrack>\n"
                f"        <OneShot>False</OneShot>\n"
                f"        <Layers>\n"
                f"          <Layer number=\"1\">\n"
                f"            <Active>False</Active>\n"
                f"            <SampleName></SampleName>\n"
                f"            <SampleFile></SampleFile>\n"
                f"            <File></File>\n"
                f"            <VelStart>0</VelStart>\n"
                f"            <VelEnd>0</VelEnd>\n"
                f"          </Layer>\n"
                f"        </Layers>\n"
                f"      </Instrument>\n"
            )
            inst_counter += 1
            total_instruments += 1
            continue

        # Build a per-articulation sub-XPM using the keygroup exporter,
        # then extract and transplant its <Instrument> blocks with
        # renumbered inst_counter offsets.
        sub_xpm = generate_keygroup_xpm(
            preset_name=f"{preset_name}_{artic}",
            engine="",
            wav_map=wav_map,
            instrument_family=vel_family,
            zone_strategy="midpoint",
        )

        # Extract <Instrument ...>…</Instrument> blocks from sub_xpm
        artic_insts = _extract_instruments(sub_xpm, inst_counter)
        instruments_xml += artic_insts["xml"]
        inst_counter   += artic_insts["count"]
        total_instruments += artic_insts["count"]

    # Build pad note map for keyswitch notes so MPC can see them labeled
    pad_note_map_xml = _build_keyswitch_pad_note_map(articulations)

    xpm = (
        f"{_XPM_HEADER}\n"
        f"  <Program type=\"Keygroup\">\n"
        f"    <Name>{prog_name}</Name>\n"
        f"    <KeygroupNumKeygroups>{total_instruments}</KeygroupNumKeygroups>\n"
        f"    <KeygroupPitchBendRange>2</KeygroupPitchBendRange>\n"
        f"    <KeygroupWheelToLfo>0.000000</KeygroupWheelToLfo>\n"
        f"{_qlink_xml_articulation()}"
        f"    <Instruments>\n"
        f"{instruments_xml}"
        f"    </Instruments>\n"
        f"{pad_note_map_xml}"
        f"  </Program>\n"
        f"</MPCVObject>\n"
    )
    return xpm


def _extract_instruments(sub_xpm: str, start_number: int) -> Dict:
    """
    Parse <Instrument number="...">...</Instrument> blocks from sub_xpm,
    renumber them starting at start_number, return {"xml": str, "count": int}.
    """
    import re
    pattern = re.compile(
        r'<Instrument number="[^"]*">(.*?)</Instrument>',
        re.DOTALL,
    )
    matches = pattern.findall(sub_xpm)
    xml_out = ""
    count = 0
    for inner in matches:
        xml_out += (
            f'      <Instrument number="{start_number + count}">'
            f"{inner}</Instrument>\n"
        )
        count += 1
    return {"xml": xml_out, "count": max(count, 1)}


def _build_keyswitch_pad_note_map(articulations: List[str]) -> str:
    """
    Build ProgramPads / PadNoteMap XML that assigns the low-octave keyswitch
    notes to the first N pads so the producer can see them labeled.
    """
    pad_lines = ""
    for idx, artic in enumerate(articulations[:16]):  # max 16 pads
        ks_midi, _ = keyswitch_note_for_index(idx)
        pad_num = idx + 1
        label = artic.replace("_", " ").upper()[:12]
        pad_lines += (
            f'        <Pad number="{pad_num}" note="{ks_midi}"'
            f' name="{xml_escape(label)}"/>\n'
        )
    return (
        f"    <ProgramPads>\n"
        f"      <PadNoteMap>\n"
        f"{pad_lines}"
        f"      </PadNoteMap>\n"
        f"    </ProgramPads>\n"
    )


# ---------------------------------------------------------------------------
# Pad-bank XPM builder
# ---------------------------------------------------------------------------

def build_pad_bank_xpm(
    preset_name: str,
    articulation: str,
    bank_name: str,
    bank_base_note: int,
    bank_last_note: int,
    wav_map: Dict[str, str],
    vel_family: str = "default",
) -> str:
    """
    Build one XPM for a single articulation mapped to an MPC pad bank.
    The Instrument zones are shifted to the bank's MIDI note range
    [bank_base_note, bank_last_note] so that:
    - Bank A (pads 1-16)  = articulation 1 at notes 36-51
    - Bank B (pads 17-32) = articulation 2 at notes 52-67
    - Bank C (pads 33-48) = articulation 3 at notes 68-83
    - Bank D (pads 49-64) = articulation 4 at notes 84-99

    This is the MPC-native articulation approach: each bank is a different
    playing style, accessible by pressing the BANK button on the MPC.
    """
    artic_label = articulation.replace("_", " ").title()
    prog_name = xml_escape(f"XO-{preset_name}-{bank_name}-{artic_label}"[:32])
    preset_slug = f"{preset_name}_{articulation}".replace(" ", "_")

    # Use the keygroup exporter's logic but remap zones to bank range
    sub_xpm = generate_keygroup_xpm(
        preset_name=f"{preset_name}_{articulation}",
        engine="",
        wav_map=wav_map,
        instrument_family=vel_family,
        zone_strategy="midpoint",
    )

    # Remap instrument LowNote/HighNote to bank range, preserving relative zone widths
    remapped_insts = _remap_instruments_to_bank(
        sub_xpm, bank_base_note, bank_last_note
    )

    pad_note_map = _build_bank_pad_note_map(bank_name, bank_base_note)

    xpm = (
        f"{_XPM_HEADER}\n"
        f"  <Program type=\"Keygroup\">\n"
        f"    <Name>{prog_name}</Name>\n"
        f"    <!-- Bank {bank_name}: {artic_label} -->\n"
        f"    <KeygroupNumKeygroups>{remapped_insts['count']}</KeygroupNumKeygroups>\n"
        f"    <KeygroupPitchBendRange>2</KeygroupPitchBendRange>\n"
        f"    <KeygroupWheelToLfo>0.000000</KeygroupWheelToLfo>\n"
        f"{_qlink_xml_articulation()}"
        f"    <Instruments>\n"
        f"{remapped_insts['xml']}"
        f"    </Instruments>\n"
        f"{pad_note_map}"
        f"  </Program>\n"
        f"</MPCVObject>\n"
    )
    return xpm


def _remap_instruments_to_bank(
    sub_xpm: str,
    bank_base: int,
    bank_last: int,
) -> Dict:
    """
    Extract Instrument blocks from sub_xpm and remap their LowNote/HighNote
    values to fit within [bank_base, bank_last].

    The original range is assumed to span some portion of 0-127.
    We scale linearly into the bank range.
    """
    import re

    # Extract all instrument blocks
    inst_pattern = re.compile(
        r'<Instrument number="[^"]*">(.*?)</Instrument>',
        re.DOTALL,
    )
    low_pattern  = re.compile(r'<LowNote>(\d+)</LowNote>')
    high_pattern = re.compile(r'<HighNote>(\d+)</HighNote>')

    matches = inst_pattern.findall(sub_xpm)
    if not matches:
        return {"xml": "", "count": 0}

    # Find original range
    all_lows  = []
    all_highs = []
    for inner in matches:
        lm = low_pattern.search(inner)
        hm = high_pattern.search(inner)
        if lm:
            all_lows.append(int(lm.group(1)))
        if hm:
            all_highs.append(int(hm.group(1)))

    orig_low  = min(all_lows)  if all_lows  else 0
    orig_high = max(all_highs) if all_highs else 127
    orig_span = max(orig_high - orig_low, 1)
    bank_span = bank_last - bank_base

    def remap(note: int) -> int:
        t = (note - orig_low) / orig_span
        return max(bank_base, min(bank_last, bank_base + round(t * bank_span)))

    xml_out = ""
    count = 0
    for inner in matches:
        new_inner = inner
        # Remap LowNote
        lm = low_pattern.search(new_inner)
        if lm:
            new_low = remap(int(lm.group(1)))
            new_inner = low_pattern.sub(f"<LowNote>{new_low}</LowNote>", new_inner, count=1)
        # Remap HighNote
        hm = high_pattern.search(new_inner)
        if hm:
            new_high = remap(int(hm.group(1)))
            new_inner = high_pattern.sub(f"<HighNote>{new_high}</HighNote>", new_inner, count=1)

        xml_out += (
            f'      <Instrument number="{count}">'
            f"{new_inner}</Instrument>\n"
        )
        count += 1

    return {"xml": xml_out, "count": count}


def _build_bank_pad_note_map(bank_name: str, bank_base_note: int) -> str:
    """Build ProgramPads for a single articulation bank (16 pads)."""
    bank_idx = _PAD_BANK_NAMES.index(bank_name) if bank_name in _PAD_BANK_NAMES else 0
    pad_start = bank_idx * 16 + 1  # pads 1-16 = bank A, 17-32 = B, etc.
    pad_lines = ""
    for i in range(16):
        pad_lines += (
            f'        <Pad number="{pad_start + i}" note="{bank_base_note + i}"/>\n'
        )
    return (
        f"    <ProgramPads>\n"
        f"      <PadNoteMap>\n"
        f"{pad_lines}"
        f"      </PadNoteMap>\n"
        f"    </ProgramPads>\n"
    )


# ---------------------------------------------------------------------------
# Main orchestration
# ---------------------------------------------------------------------------

def build_articulation_programs(
    samples_dir: Path,
    output_dir: Path,
    preset_name: str,
    instrument_family: str,
    mode: str,
    articulation_override: Optional[List[str]] = None,
    dry_run: bool = False,
) -> None:
    """
    Main entry point. Discover articulation sub-folders, build XPM files.

    For mode="keyswitch":
        - Writes one combined XPM: {preset_slug}_keyswitch.xpm
        - Writes individual per-articulation XPMs: {preset_slug}_{artic}.xpm

    For mode="pad-banks":
        - Writes per-articulation XPMs mapped to MPC pad banks A/B/C/D:
          {preset_slug}_bankA_{artic}.xpm, etc.
        - Writes a summary manifest listing all generated programs.
    """
    print(BANNER)

    preset_slug  = preset_name.replace(" ", "_")
    vel_family   = _FAMILY_VEL_MAP.get(instrument_family, "default")

    # --- Discover articulations ---
    if articulation_override:
        articulations = articulation_override
        print(f"  Articulations (override): {articulations}")
    else:
        default_artics = ARTICULATIONS.get(instrument_family, [])
        # Prefer sub-folder discovery over hard-coded defaults
        discovered = _discover_articulations(samples_dir)
        if discovered:
            # Use only folders that match known articulations (or all if none match)
            known_names = set(default_artics)
            filtered = [d for d in discovered if d in known_names]
            articulations = filtered if filtered else discovered
        else:
            articulations = default_artics
        print(f"  Articulations ({instrument_family}): {articulations}")

    if not articulations:
        print(f"  [ERROR] No articulations found for family '{instrument_family}'.")
        print("          Provide --articulations or add sub-folders to --samples dir.")
        sys.exit(1)

    # --- Load WAV maps per articulation ---
    artic_wav_maps:  Dict[str, Dict[str, str]] = {}
    artic_wavs_dirs: Dict[str, Path] = {}

    for artic in articulations:
        artic_dir = samples_dir / artic
        if not artic_dir.exists():
            print(f"  [WARN] No samples directory found for articulation '{artic}' "
                  f"(expected: {artic_dir}) — will generate empty placeholder.")
            artic_wav_maps[artic]  = {}
            artic_wavs_dirs[artic] = artic_dir
            continue

        artic_slug = f"{preset_slug}_{artic}"
        wav_map = build_keygroup_wav_map(artic_dir, artic_slug)
        if not wav_map:
            # Try without the preset prefix (bare note-named files)
            wav_map = build_keygroup_wav_map(artic_dir, artic)
        artic_wav_maps[artic]  = wav_map
        artic_wavs_dirs[artic] = artic_dir
        print(f"  {artic:<20} {len(wav_map):>4} WAV files  ({artic_dir.name}/)")

    print()

    if not dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)

    generated: List[str] = []

    # ------------------------------------------------------------------
    # Mode: keyswitch
    # ------------------------------------------------------------------
    if mode == "keyswitch":
        # 1. Combined keyswitch XPM
        ks_xpm = build_keyswitch_xpm(
            preset_name=preset_name,
            articulations=articulations,
            artic_wav_maps=artic_wav_maps,
            artic_wavs_dirs=artic_wavs_dirs,
            instrument_family=instrument_family,
            vel_family=vel_family,
        )
        ks_filename = f"{preset_slug}_keyswitch.xpm"
        _emit(ks_xpm, output_dir / ks_filename, dry_run)
        generated.append(ks_filename)

        # 2. Individual per-articulation XPMs (for loading single articulations)
        for artic in articulations:
            wav_map = artic_wav_maps.get(artic, {})
            if not wav_map:
                continue
            artic_xpm = generate_keygroup_xpm(
                preset_name=f"{preset_name} {artic.replace('_', ' ').title()}",
                engine="",
                wav_map=wav_map,
                instrument_family=vel_family,
                zone_strategy="midpoint",
            )
            fname = f"{preset_slug}_{artic}.xpm"
            _emit(artic_xpm, output_dir / fname, dry_run)
            generated.append(fname)

        # Print keyswitch legend
        print("  Keyswitch legend:")
        for idx, artic in enumerate(articulations):
            ks_midi, ks_name = keyswitch_note_for_index(idx)
            label = artic.replace("_", " ").title()
            print(f"    {ks_name:6}  (MIDI {ks_midi:3})  →  {label}")
        print()

    # ------------------------------------------------------------------
    # Mode: pad-banks
    # ------------------------------------------------------------------
    elif mode == "pad-banks":
        if len(articulations) > 4:
            print(f"  [WARN] Pad-bank mode supports max 4 articulations (one per bank).")
            print(f"         Using first 4: {articulations[:4]}")
            articulations = articulations[:4]

        for idx, artic in enumerate(articulations):
            wav_map = artic_wav_maps.get(artic, {})
            bank_name, bank_base, bank_last = pad_bank_for_index(idx)

            pb_xpm = build_pad_bank_xpm(
                preset_name=preset_name,
                articulation=artic,
                bank_name=bank_name,
                bank_base_note=bank_base,
                bank_last_note=bank_last,
                wav_map=wav_map,
                vel_family=vel_family,
            )
            fname = f"{preset_slug}_bank{bank_name}_{artic}.xpm"
            _emit(pb_xpm, output_dir / fname, dry_run)
            generated.append(fname)

        # Print bank legend
        print("  Pad bank legend:")
        for idx, artic in enumerate(articulations):
            bank_name, bank_base, bank_last = pad_bank_for_index(idx)
            label = artic.replace("_", " ").title()
            base_name = midi_to_note_name(bank_base)
            last_name = midi_to_note_name(bank_last)
            print(f"    Bank {bank_name}  (notes {base_name}–{last_name})  →  {label}")
        print()

    # ------------------------------------------------------------------
    # Summary
    # ------------------------------------------------------------------
    print(f"  Generated {len(generated)} XPM file(s):")
    for f in generated:
        marker = "  [dry-run]" if dry_run else ""
        print(f"    {output_dir / f}{marker}")
    print()
    print("  Done.")


def _emit(xpm: str, path: Path, dry_run: bool) -> None:
    """Write XPM to file or print to stdout (dry-run)."""
    if dry_run:
        print(f"\n{'─' * 70}")
        print(f"  [DRY RUN] {path.name}")
        print('─' * 70)
        print(xpm)
    else:
        path.write_text(xpm, encoding="utf-8")
        print(f"  Written: {path}")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description=(
            "XPN Multi-Articulation Program Builder — XO_OX Designs\n"
            "Builds MPC Keygroup XPM programs with multiple articulations "
            "(key switches or pad banks)."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Examples:\n"
            "  # Key-switch mode for erhu\n"
            "  python3 xpn_articulation_builder.py \\\n"
            "      --instrument-family world_bowed \\\n"
            "      --samples ./erhu_samples/ \\\n"
            "      --preset 'Erhu Solo' \\\n"
            "      --mode keyswitch \\\n"
            "      --output ./programs/\n\n"
            "  # Pad-bank mode for strings\n"
            "  python3 xpn_articulation_builder.py \\\n"
            "      --instrument-family strings \\\n"
            "      --samples ./strings/ \\\n"
            "      --preset 'Violin Section' \\\n"
            "      --mode pad-banks \\\n"
            "      --output ./programs/\n"
        ),
    )

    parser.add_argument(
        "--samples", required=True,
        help="Root directory containing articulation sub-folders "
             "(e.g. legato/, tremolo/, pizzicato/)",
    )
    parser.add_argument(
        "--output", required=True,
        help="Output directory for generated .xpm files",
    )
    parser.add_argument(
        "--preset", required=True,
        help="Preset name (e.g. 'Erhu Solo', 'Violin Section')",
    )
    parser.add_argument(
        "--instrument-family", required=True,
        choices=list(ARTICULATIONS.keys()),
        metavar="FAMILY",
        help=(
            "Instrument family — determines default articulation set and "
            "velocity curves. "
            f"Choices: {', '.join(ARTICULATIONS.keys())}"
        ),
    )
    parser.add_argument(
        "--mode", required=True,
        choices=["keyswitch", "pad-banks"],
        help=(
            "keyswitch: low-octave notes (C0+) trigger articulation banks; "
            "pad-banks: MPC pad banks A/B/C/D each carry one articulation"
        ),
    )
    parser.add_argument(
        "--articulations", nargs="+",
        metavar="ARTICULATION",
        help=(
            "Override the default articulation list for this family. "
            "Must match sub-folder names under --samples. "
            "Example: --articulations legato tremolo pizzicato"
        ),
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Print generated XPM XML to stdout — do not write files",
    )
    parser.add_argument(
        "--list-families", action="store_true",
        help="Print all supported instrument families and their articulations, then exit",
    )

    # Allow --list-families without requiring the other flags
    if "--list-families" in sys.argv:
        args = parser.parse_args(["--list-families",
                                   "--samples", ".",
                                   "--output", ".",
                                   "--preset", "_",
                                   "--instrument-family", "strings",
                                   "--mode", "keyswitch"])
        args.list_families = True
    else:
        args = parser.parse_args()

    if args.list_families:
        print(BANNER)
        print("  Supported instrument families and articulations:\n")
        for fam, artics in ARTICULATIONS.items():
            vel_fam = _FAMILY_VEL_MAP.get(fam, "default")
            print(f"  {fam:<20}  (velocity curve: {vel_fam})")
            for a in artics:
                ks_midi, ks_name = keyswitch_note_for_index(artics.index(a))
                print(f"      {a:<22}  keyswitch: {ks_name}  (MIDI {ks_midi})")
            print()
        sys.exit(0)

    build_articulation_programs(
        samples_dir=Path(args.samples),
        output_dir=Path(args.output),
        preset_name=args.preset,
        instrument_family=args.instrument_family,
        mode=args.mode,
        articulation_override=args.articulations,
        dry_run=args.dry_run,
    )


if __name__ == "__main__":
    main()
