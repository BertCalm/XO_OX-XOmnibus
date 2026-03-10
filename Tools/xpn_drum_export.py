#!/usr/bin/env python3
"""
XPN Drum Program Exporter — XO_OX Designs
Generates MPC-compatible drum expansion packs (.xpn) from XOnset presets.

For each XOnset preset, produces:
  - A <Program type="Drum"> XPM file (128 instruments, 8 active pads)
  - Expects WAV files named: {preset}_{voice}_{vel}.wav
    e.g. 808_Reborn_kick_v1.wav, 808_Reborn_kick_v2.wav

Pad layout (GM-convention MIDI notes):
  V1 Kick         → Note 36  (C2)
  V2 Snare        → Note 38  (D2)
  V3 Closed Hat   → Note 42  (F#2)   MuteGroup 1
  V4 Open Hat     → Note 46  (A#2)   MuteGroup 1 (muted by closed hat)
  V5 Clap         → Note 39  (D#2)
  V6 Tom          → Note 41  (F2)
  V7 Percussion   → Note 43  (G2)
  V8 FX/Cymbal    → Note 49  (C#3)

Velocity layers per pad (render from XOnset at 4 intensities):
  Layer 1: vel 0–31   (ghost)
  Layer 2: vel 32–63  (soft)
  Layer 3: vel 64–95  (medium)
  Layer 4: vel 96–127 (hard)

WAV file naming convention:
  {preset_slug}_{voice_name}_v{1-4}.wav
  e.g.:  808_Reborn_kick_v1.wav
         808_Reborn_kick_v4.wav
         808_Reborn_snare_v2.wav

Usage:
    # Export a single preset
    python3 xpn_drum_export.py \\
        --preset "808 Reborn" \\
        --wavs-dir /path/to/rendered/wavs \\
        --output-dir /path/to/output

    # Export all XOnset presets
    python3 xpn_drum_export.py \\
        --all-onset \\
        --wavs-dir /path/to/rendered/wavs \\
        --output-dir /path/to/output

    # Generate XPM structure only (no WAVs yet — for planning)
    python3 xpn_drum_export.py --preset "808 Reborn" --output-dir /tmp/test --dry-run
"""

import argparse
import json
import os
import shutil
import sys
from datetime import date
from pathlib import Path
from textwrap import indent

# Import cover art generator if available
try:
    from xpn_cover_art import generate_cover
    COVER_ART_AVAILABLE = True
except ImportError:
    COVER_ART_AVAILABLE = False

REPO_ROOT = Path(__file__).parent.parent
PRESETS_DIR = REPO_ROOT / "Presets" / "XOmnibus"

# =============================================================================
# PAD / VOICE DEFINITIONS
# =============================================================================

# (midi_note, voice_name, mute_group, mute_targets)
# MuteGroup 0 = no choke. MuteGroup 1 = hi-hat group.
# MuteTarget: instrument number that THIS pad mutes (0 = none)
PAD_MAP = [
    (36, "kick",   0, [0, 0, 0, 0]),   # V1
    (38, "snare",  0, [0, 0, 0, 0]),   # V2
    (42, "chat",   1, [46, 0, 0, 0]),  # V3 Closed hat → mutes open hat (46)
    (46, "ohat",   1, [0, 0, 0, 0]),   # V4 Open hat
    (39, "clap",   0, [0, 0, 0, 0]),   # V5
    (41, "tom",    0, [0, 0, 0, 0]),   # V6
    (43, "perc",   0, [0, 0, 0, 0]),   # V7
    (49, "fx",     0, [0, 0, 0, 0]),   # V8
]

# Velocity layer ranges for the 4 layers
VEL_LAYERS = [
    (0,  31,  0.35),   # ghost — lower volume
    (32, 63,  0.55),   # soft
    (64, 95,  0.75),   # medium
    (96, 127, 0.95),   # hard
]

# Sample file suffix per velocity layer
VEL_SUFFIXES = ["v1", "v2", "v3", "v4"]


# =============================================================================
# XPM GENERATION
# =============================================================================

def _layer_block(number: int, vel_start: int, vel_end: int,
                 sample_name: str, sample_file: str, volume: float) -> str:
    """Generate a single <Layer> XML block."""
    active = "True" if sample_name else "True"  # always True (empty = VelStart 0)
    sfile = sample_file if sample_file else ""
    return f"""\
          <Layer number="{number}">
            <Active>{active}</Active>
            <Volume>{volume:.6f}</Volume>
            <Pan>0.500000</Pan>
            <Pitch>0.000000</Pitch>
            <TuneCoarse>0</TuneCoarse>
            <TuneFine>0</TuneFine>
            <VelStart>{vel_start}</VelStart>
            <VelEnd>{vel_end}</VelEnd>
            <SampleStart>0</SampleStart>
            <SampleEnd>0</SampleEnd>
            <Loop>False</Loop>
            <LoopStart>0</LoopStart>
            <LoopEnd>0</LoopEnd>
            <LoopTune>0</LoopTune>
            <Mute>False</Mute>
            <RootNote>0</RootNote>
            <KeyTrack>False</KeyTrack>
            <SampleName>{sample_name}</SampleName>
            <SampleFile>{sfile}</SampleFile>
            <SliceIndex>128</SliceIndex>
            <Direction>0</Direction>
            <Offset>0</Offset>
            <SliceStart>0</SliceStart>
            <SliceEnd>0</SliceEnd>
            <SliceLoopStart>0</SliceLoopStart>
            <SliceLoop>0</SliceLoop>
          </Layer>"""


def _instrument_block(instrument_num: int, note: int, voice_name: str,
                      mute_group: int, mute_targets: list,
                      wav_map: dict, preset_slug: str) -> str:
    """Generate a single <Instrument> XML block (0-indexed, empty = inactive)."""
    # Check if this is an active pad (has samples)
    active_pad = instrument_num == note

    # Build layer blocks
    layers = []
    if active_pad:
        for i, (vel_start, vel_end, vol) in enumerate(VEL_LAYERS):
            suffix = VEL_SUFFIXES[i]
            wav_key = f"{preset_slug}_{voice_name}_{suffix}"
            sample_name = wav_map.get(wav_key, "")
            sample_file = sample_name  # same name (MPC looks up by name)
            layers.append(_layer_block(i + 1, vel_start, vel_end,
                                       sample_name, sample_file, vol))
    else:
        # Inactive instrument: 4 empty layers, VelStart=0
        for i in range(4):
            layers.append(_layer_block(i + 1, 0, 127, "", "", 0.707946))

    mute_xml = "\n".join(
        f"        <MuteTarget{i+1}>{t}</MuteTarget{i+1}>"
        for i, t in enumerate(mute_targets)
    )
    simult_xml = "\n".join(
        f"        <SimultTarget{i+1}>0</SimultTarget{i+1}>"
        for i in range(4)
    )
    layers_xml = "\n".join(layers)

    return f"""\
      <Instrument number="{instrument_num}">
        <AudioRoute>
          <AudioRoute>0</AudioRoute>
          <AudioRouteSubIndex>0</AudioRouteSubIndex>
          <InsertsEnabled>False</InsertsEnabled>
        </AudioRoute>
        <Send1>0.000000</Send1>
        <Send2>0.000000</Send2>
        <Send3>0.000000</Send3>
        <Send4>0.000000</Send4>
        <Volume>0.707946</Volume>
        <Mute>False</Mute>
        <Pan>0.500000</Pan>
        <TuneCoarse>0</TuneCoarse>
        <TuneFine>0</TuneFine>
        <Mono>True</Mono>
        <Polyphony>1</Polyphony>
        <FilterKeytrack>0.000000</FilterKeytrack>
        <LowNote>0</LowNote>
        <HighNote>127</HighNote>
        <IgnoreBaseNote>False</IgnoreBaseNote>
        <ZonePlay>1</ZonePlay>
        <MuteGroup>{mute_group}</MuteGroup>
{mute_xml}
{simult_xml}
        <LfoPitch>0.000000</LfoPitch>
        <LfoCutoff>0.000000</LfoCutoff>
        <LfoVolume>0.000000</LfoVolume>
        <LfoPan>0.000000</LfoPan>
        <OneShot>True</OneShot>
        <FilterType>2</FilterType>
        <Cutoff>1.000000</Cutoff>
        <Resonance>0.000000</Resonance>
        <FilterEnvAmt>0.000000</FilterEnvAmt>
        <AfterTouchToFilter>0.000000</AfterTouchToFilter>
        <VelocityToStart>0.000000</VelocityToStart>
        <VelocityToFilterAttack>0.000000</VelocityToFilterAttack>
        <VelocityToFilter>0.000000</VelocityToFilter>
        <VelocityToFilterEnvelope>0.000000</VelocityToFilterEnvelope>
        <FilterAttack>0.000000</FilterAttack>
        <FilterDecay>0.000000</FilterDecay>
        <FilterSustain>1.000000</FilterSustain>
        <FilterRelease>0.000000</FilterRelease>
        <FilterHold>0.000000</FilterHold>
        <FilterDecayType>True</FilterDecayType>
        <FilterADEnvelope>True</FilterADEnvelope>
        <VolumeHold>0.000000</VolumeHold>
        <VolumeDecayType>True</VolumeDecayType>
        <VolumeADEnvelope>True</VolumeADEnvelope>
        <VolumeAttack>0.000000</VolumeAttack>
        <VolumeDecay>0.000000</VolumeDecay>
        <VolumeSustain>1.000000</VolumeSustain>
        <VolumeRelease>0.000000</VolumeRelease>
        <VelocityToPitch>0.000000</VelocityToPitch>
        <VelocityToVolumeAttack>0.000000</VelocityToVolumeAttack>
        <VelocitySensitivity>1.000000</VelocitySensitivity>
        <VelocityToPan>0.000000</VelocityToPan>
        <LFO>
          <Speed>0.000000</Speed>
          <Amount>0.000000</Amount>
          <Type>0</Type>
          <Sync>False</Sync>
          <Retrigger>True</Retrigger>
        </LFO>
        <Layers>
{layers_xml}
        </Layers>
      </Instrument>"""


def generate_xpm(preset_name: str, wav_map: dict) -> str:
    """Generate complete drum program XPM XML string."""
    preset_slug = preset_name.replace(" ", "_")
    prog_name = f"XO_OX-{preset_name}"

    # Build a lookup: note → pad info
    note_to_pad = {note: (voice, mg, mt) for note, voice, mg, mt in PAD_MAP}

    # Generate all 128 instrument blocks
    instruments_xml_parts = []
    for i in range(128):
        note = i  # instrument number == MIDI note
        if note in note_to_pad:
            voice, mg, mt = note_to_pad[note]
        else:
            voice, mg, mt = "", 0, [0, 0, 0, 0]

        block = _instrument_block(
            instrument_num=note,
            note=note,
            voice_name=voice,
            mute_group=mg,
            mute_targets=mt,
            wav_map=wav_map,
            preset_slug=preset_slug,
        )
        instruments_xml_parts.append(block)

    instruments_xml = "\n".join(instruments_xml_parts)

    # ProgramPads JSON blob (minimal default — MPC populates from pad layout)
    pad_json = json.dumps({"ProgramPads": {"Universal": {"value0": False},
                                            "Type": {"value0": 5},
                                            "universalPad": 32512}},
                          separators=(",", ":"))

    return f"""<?xml version="1.0" encoding="UTF-8"?>

<MPCVObject>
  <Version>
    <File_Version>1.7</File_Version>
    <Application>MPC-V</Application>
    <Application_Version>2.10.0.0</Application_Version>
    <Platform>OSX</Platform>
  </Version>
  <Program type="Drum">
    <Name>{prog_name}</Name>
    <ProgramPads>{pad_json}</ProgramPads>
    <PadNoteMap></PadNoteMap>
    <PadGroupMap></PadGroupMap>
    <Instruments>
{instruments_xml}
    </Instruments>
  </Program>
</MPCVObject>
"""


# =============================================================================
# EXPANSION.XML
# =============================================================================

def generate_expansion_xml(pack_name: str, pack_id: str,
                            description: str, version: str = "1.0.0") -> str:
    """Generate the MPC expansion manifest XML."""
    today = str(date.today())
    return f"""<?xml version="1.0" encoding="UTF-8"?>

<expansion version="2.0.0.0" buildVersion="2.10.0.0">
  <local/>
  <identifier>{pack_id}</identifier>
  <title>{pack_name}</title>
  <manufacturer>XO_OX Designs</manufacturer>
  <version>{version}.0</version>
  <type>drum</type>
  <priority>50</priority>
  <img>artwork.png</img>
  <description>{description}</description>
  <separator>-</separator>
</expansion>
"""


# =============================================================================
# WAV MANIFEST HELPER
# =============================================================================

def build_wav_map(wavs_dir: Path, preset_slug: str) -> dict:
    """
    Scan wavs_dir for WAV files matching this preset and build a lookup map.
    Keys: "{preset_slug}_{voice}_{vel_suffix}"
    Values: filename (without path, MPC uses name-based lookup)
    """
    wav_map = {}
    if not wavs_dir or not wavs_dir.exists():
        return wav_map

    for wav_file in wavs_dir.glob("*.wav"):
        name = wav_file.stem  # e.g. "808_Reborn_kick_v1"
        if name.startswith(preset_slug):
            wav_map[name] = wav_file.name

    return wav_map


def print_wav_checklist(preset_name: str):
    """Print the list of WAV files needed for a preset."""
    slug = preset_name.replace(" ", "_")
    print(f"\nWAV files needed for '{preset_name}':")
    print(f"  Render each voice at 4 velocity levels from XOnset.")
    print(f"  Name them exactly as follows:\n")
    for note, voice, _, _ in PAD_MAP:
        for i, (vel_start, vel_end, _) in enumerate(VEL_LAYERS):
            suffix = VEL_SUFFIXES[i]
            fname = f"{slug}_{voice}_{suffix}.wav"
            print(f"    {fname}  (vel {vel_start}-{vel_end})")
    print()


# =============================================================================
# PACK BUILDER
# =============================================================================

def build_drum_pack(preset_name: str, wavs_dir: Path, output_dir: Path,
                    pack_id: str = None, version: str = "1.0",
                    description: str = "", dry_run: bool = False,
                    generate_art: bool = True):
    """
    Build a complete drum XPN pack structure for one XOnset preset.

    output_dir/
      {preset_slug}/
        Expansion.xml
        artwork.png          (1000×1000)
        artwork_2000.png     (2000×2000 master)
        {preset_slug}.xpm
        [WAV files copied here if wavs_dir provided]
    """
    preset_slug = preset_name.replace(" ", "_")
    pack_id = pack_id or f"com.xo-ox.onset.{preset_slug.lower()}"
    pack_dir = output_dir / preset_slug
    pack_dir.mkdir(parents=True, exist_ok=True)

    # Build WAV map
    wav_map = build_wav_map(wavs_dir, preset_slug) if wavs_dir else {}
    missing = []
    for note, voice, _, _ in PAD_MAP:
        for suf in VEL_SUFFIXES:
            key = f"{preset_slug}_{voice}_{suf}"
            if key not in wav_map:
                missing.append(f"{preset_slug}_{voice}_{suf}.wav")

    # Generate XPM
    xpm_content = generate_xpm(preset_name, wav_map)
    xpm_path = pack_dir / f"{preset_slug}.xpm"
    if not dry_run:
        xpm_path.write_text(xpm_content, encoding="utf-8")
        print(f"  XPM:  {xpm_path.name}")

    # Generate Expansion.xml
    if not description:
        description = f"XOnset drum kit — {preset_name}. Dual-layer synthesis percussion by XO_OX Designs."
    exp_content = generate_expansion_xml(
        pack_name=f"XOnset: {preset_name}",
        pack_id=pack_id,
        description=description,
        version=version,
    )
    exp_path = pack_dir / "Expansion.xml"
    if not dry_run:
        exp_path.write_text(exp_content, encoding="utf-8")
        print(f"  Manifest: {exp_path.name}")

    # Cover art
    if generate_art and not dry_run:
        if COVER_ART_AVAILABLE:
            try:
                generate_cover(
                    engine="ONSET",
                    pack_name=preset_name,
                    output_dir=str(pack_dir),
                    preset_count=1,
                    version=version,
                    seed=hash(preset_name) % 10000,
                )
            except Exception as e:
                print(f"  [WARN] Cover art failed: {e}")
        else:
            print("  [SKIP] Cover art: xpn_cover_art.py not found or Pillow missing")

    # Copy WAVs
    if wavs_dir and not dry_run:
        copied = 0
        for wav_name in wav_map.values():
            src = wavs_dir / wav_name
            dst = pack_dir / wav_name
            if src.exists() and not dst.exists():
                shutil.copy2(src, dst)
                copied += 1
        if copied:
            print(f"  WAVs: {copied} files copied")

    # Report missing WAVs
    if missing:
        print(f"  [MISSING] {len(missing)} WAV files not found:")
        for m in missing[:6]:
            print(f"    {m}")
        if len(missing) > 6:
            print(f"    ... and {len(missing) - 6} more")
        print(f"  Run with --checklist to see all required WAV names.")

    return {"pack_dir": str(pack_dir), "missing_wavs": missing}


def build_all_onset_packs(wavs_dir: Path, output_dir: Path,
                          version: str = "1.0", dry_run: bool = False):
    """Build drum packs for all XOnset presets found in the preset library."""
    onset_presets = []
    for mood_dir in PRESETS_DIR.iterdir():
        if mood_dir.is_dir():
            onset_dir = mood_dir / "Onset"
            if onset_dir.exists():
                for xmeta in onset_dir.glob("*.xometa"):
                    with open(xmeta) as f:
                        data = json.load(f)
                    onset_presets.append({
                        "name": data["name"],
                        "mood": data["mood"],
                        "description": data.get("description", ""),
                    })

    print(f"Found {len(onset_presets)} XOnset presets")
    results = []
    for p in onset_presets:
        print(f"\nBuilding: {p['name']} ({p['mood']})")
        r = build_drum_pack(
            preset_name=p["name"],
            wavs_dir=wavs_dir,
            output_dir=output_dir,
            version=version,
            description=p["description"],
            dry_run=dry_run,
            generate_art=True,
        )
        results.append(r)

    return results


# =============================================================================
# CLI
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Drum Program Exporter — XO_OX Designs",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--preset",     help="Single preset name to export")
    parser.add_argument("--all-onset",  action="store_true",
                        help="Export all XOnset presets")
    parser.add_argument("--wavs-dir",   help="Directory containing rendered WAV files")
    parser.add_argument("--output-dir", default=".", help="Output directory for packs")
    parser.add_argument("--version",    default="1.0", help="Pack version string")
    parser.add_argument("--dry-run",    action="store_true",
                        help="Show what would be generated without writing files")
    parser.add_argument("--checklist",  help="Print WAV checklist for a preset name")
    parser.add_argument("--no-art",     action="store_true",
                        help="Skip cover art generation")
    args = parser.parse_args()

    if args.checklist:
        print_wav_checklist(args.checklist)
        return 0

    output_dir = Path(args.output_dir)
    wavs_dir = Path(args.wavs_dir) if args.wavs_dir else None

    if args.dry_run:
        print("DRY RUN — no files will be written\n")

    if args.all_onset:
        build_all_onset_packs(wavs_dir, output_dir, args.version, args.dry_run)
    elif args.preset:
        print(f"Building drum pack: {args.preset}")
        build_drum_pack(
            preset_name=args.preset,
            wavs_dir=wavs_dir,
            output_dir=output_dir,
            version=args.version,
            dry_run=args.dry_run,
            generate_art=not args.no_art,
        )
    else:
        parser.print_help()
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
