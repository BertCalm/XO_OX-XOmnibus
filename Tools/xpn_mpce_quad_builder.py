#!/usr/bin/env python3
"""
XPN MPCe Quad Builder — XO_OX Designs
Rex + Hex flagship tool for MPCe-native feliX-Oscar quad-corner drum kits.

Each pad carries 4 sample variants mapped to velocity zones (Tier 1, all MPC hardware)
with a commented-out native MPCe corner XML stub (Tier 2, pending Akai docs).

feliX-Oscar Corner Architecture:
  NW = feliX/Dry  — clinical, bright, unprocessed     (vel  1-31)
  NE = feliX/Wet  — bright + FX chain                 (vel 32-63)
  SW = Oscar/Dry  — warm, organic, unprocessed        (vel 64-95)
  SE = Oscar/Wet  — warm + FX chain                   (vel 96-127)

Usage — Single pad:
    python xpn_mpce_quad_builder.py \\
        --felix-dry  samples/kick_felix_dry/ \\
        --felix-wet  samples/kick_felix_wet/ \\
        --oscar-dry  samples/kick_oscar_dry/ \\
        --oscar-wet  samples/kick_oscar_wet/ \\
        --output     programs/mpce_kick_quad/ \\
        --pad-name   "Kick_QuadCorner"

Usage — Full kit from structured directory:
    python xpn_mpce_quad_builder.py \\
        --kit-dir samples/full_kit/ \\
        --output  programs/mpce_full_kit/
    # expects: samples/full_kit/{pad_name}/{felix_dry,felix_wet,oscar_dry,oscar_wet}/

Usage — Auto-generate tone variants from standard kit:
    python xpn_mpce_quad_builder.py \\
        --from-standard-kit existing_kit.xpm \\
        --auto-tone-variants \\
        --output programs/mpce_auto/
"""

import argparse
import json
import re
import sys
import xml.etree.ElementTree as ET
from datetime import date
from pathlib import Path
from xml.sax.saxutils import escape as xml_escape

# Optional scipy for shelf filters (--auto-tone-variants)
try:
    import numpy as np
    import scipy.signal as signal
    import scipy.io.wavfile as wavfile
    SCIPY_AVAILABLE = True
except ImportError:
    SCIPY_AVAILABLE = False

REPO_ROOT = Path(__file__).parent.parent
TODAY = date.today().isoformat()

# =============================================================================
# CORNER DEFINITIONS
# =============================================================================

CORNERS = ["felix_dry", "felix_wet", "oscar_dry", "oscar_wet"]

CORNER_META = {
    "felix_dry": {
        "position":    "NW",
        "vel_start":   1,
        "vel_end":     31,
        "volume":      0.707946,
        "label":       "feliX/Dry",
        "description": "clinical, bright, unprocessed",
    },
    "felix_wet": {
        "position":    "NE",
        "vel_start":   32,
        "vel_end":     63,
        "volume":      0.707946,
        "label":       "feliX/Wet",
        "description": "bright + FX chain",
    },
    "oscar_dry": {
        "position":    "SW",
        "vel_start":   64,
        "vel_end":     95,
        "volume":      0.707946,
        "label":       "Oscar/Dry",
        "description": "warm, organic, unprocessed",
    },
    "oscar_wet": {
        "position":    "SE",
        "vel_start":   96,
        "vel_end":     127,
        "volume":      0.707946,
        "label":       "Oscar/Wet",
        "description": "warm + FX chain",
    },
}

# GM-convention pad layout matching xpn_drum_export.py
PAD_MAP = [
    (36, "kick",  0, [0, 0, 0, 0]),
    (38, "snare", 0, [0, 0, 0, 0]),
    (39, "clap",  0, [0, 0, 0, 0]),
    (42, "chat",  1, [46, 0, 0, 0]),
    (46, "ohat",  1, [0, 0, 0, 0]),
    (41, "tom",   0, [0, 0, 0, 0]),
    (43, "perc",  0, [0, 0, 0, 0]),
    (49, "fx",    0, [0, 0, 0, 0]),
]

# Kit subdirectory names for --kit-dir mode (canonical)
KIT_CORNER_DIRS = ["felix_dry", "felix_wet", "oscar_dry", "oscar_wet"]

# =============================================================================
# TONE VARIANT DSP  (--auto-tone-variants)
# =============================================================================

def _design_shelf(shelf_type: str, fc: float, sr: float,
                  gain_db: float) -> tuple:
    """
    Design a 2nd-order shelf filter using the Audio EQ Cookbook formulation
    (Robert Bristow-Johnson) with bilinear pre-warping for correct fc placement.
    shelf_type: 'low' or 'high'
    Returns (b, a) direct-form IIR coefficients.
    """
    if not SCIPY_AVAILABLE:
        raise RuntimeError("scipy is required for --auto-tone-variants")

    A = 10 ** (gain_db / 40.0)   # sqrt of linear amplitude gain
    # Bilinear pre-warp: maps analogue fc to digital domain correctly
    w0 = 2.0 * np.pi * fc / sr
    cos_w0 = np.cos(w0)
    alpha = np.sin(w0) / (2.0 * 0.707)   # Q = 0.707 (Butterworth slope)

    if shelf_type == "high":
        b0 =      A * ((A + 1) + (A - 1) * cos_w0 + 2 * np.sqrt(A) * alpha)
        b1 = -2 * A * ((A - 1) + (A + 1) * cos_w0)
        b2 =      A * ((A + 1) + (A - 1) * cos_w0 - 2 * np.sqrt(A) * alpha)
        a0 =           (A + 1) - (A - 1) * cos_w0 + 2 * np.sqrt(A) * alpha
        a1 =  2 *     ((A - 1) - (A + 1) * cos_w0)
        a2 =           (A + 1) - (A - 1) * cos_w0 - 2 * np.sqrt(A) * alpha
    else:
        # Low shelf
        b0 =      A * ((A + 1) - (A - 1) * cos_w0 + 2 * np.sqrt(A) * alpha)
        b1 =  2 * A * ((A - 1) - (A + 1) * cos_w0)
        b2 =      A * ((A + 1) - (A - 1) * cos_w0 - 2 * np.sqrt(A) * alpha)
        a0 =           (A + 1) + (A - 1) * cos_w0 + 2 * np.sqrt(A) * alpha
        a1 = -2 *     ((A - 1) + (A + 1) * cos_w0)
        a2 =           (A + 1) + (A - 1) * cos_w0 - 2 * np.sqrt(A) * alpha

    b = np.array([b0, b1, b2]) / a0
    a = np.array([a0, a1, a2]) / a0
    return b, a


def _apply_shelf(samples: np.ndarray, sr: int,
                 shelf_type: str, fc: float, gain_db: float) -> np.ndarray:
    """Apply shelf filter to audio samples array (float64)."""
    b, a = _design_shelf(shelf_type, fc, sr, gain_db)
    if samples.ndim == 1:
        return signal.lfilter(b, a, samples)
    # Stereo: filter each channel
    out = np.stack([signal.lfilter(b, a, samples[:, ch])
                    for ch in range(samples.shape[1])], axis=1)
    return out


def _normalize_peak(samples: np.ndarray, target_db: float = -0.5) -> np.ndarray:
    """Normalise peak to target_db (default -0.5 dBFS)."""
    peak = np.max(np.abs(samples))
    if peak < 1e-10:
        return samples
    target_linear = 10 ** (target_db / 20.0)
    return samples * (target_linear / peak)


def _read_wav(path: Path) -> tuple:
    """Return (sr, float64 samples) from a WAV file."""
    sr, data = wavfile.read(str(path))
    if data.dtype == np.int16:
        samples = data.astype(np.float64) / 32768.0
    elif data.dtype == np.int32:
        samples = data.astype(np.float64) / 2147483648.0
    elif data.dtype == np.float32:
        samples = data.astype(np.float64)
    else:
        samples = data.astype(np.float64)
    return sr, samples


def _write_wav(path: Path, sr: int, samples: np.ndarray) -> None:
    """Write float64 samples to 16-bit WAV."""
    clipped = np.clip(samples, -1.0, 1.0)
    int16_data = (clipped * 32767).astype(np.int16)
    wavfile.write(str(path), sr, int16_data)


def generate_tone_variants(source_wav: Path, output_dir: Path,
                            stem: str) -> dict:
    """
    Generate 4 feliX-Oscar tone variants from a single source WAV.

    Returns dict mapping corner key -> output Path.

    Tonal recipes:
      felix_dry: source normalised bright (high-shelf +1.5 dB @ 6kHz)
      felix_wet: source + high-shelf +3 dB @ 8kHz
      oscar_dry: source + low-shelf +3 dB @ 300Hz
      oscar_wet: source + high-shelf -2 dB @ 4kHz + low-shelf +4 dB @ 300Hz
    """
    if not SCIPY_AVAILABLE:
        raise RuntimeError(
            "scipy + numpy are required for --auto-tone-variants.\n"
            "Install with: pip install scipy numpy"
        )
    sr, samples = _read_wav(source_wav)
    output_dir.mkdir(parents=True, exist_ok=True)
    out_paths = {}

    recipes = {
        "felix_dry": [("high", 6000.0, +1.5)],
        "felix_wet": [("high", 8000.0, +3.0)],
        "oscar_dry": [("low",  300.0,  +3.0)],
        "oscar_wet": [("high", 4000.0, -2.0), ("low", 300.0, +4.0)],
    }

    for corner, shelves in recipes.items():
        processed = samples.copy()
        for shelf_type, fc, gain_db in shelves:
            processed = _apply_shelf(processed, sr, shelf_type, fc, gain_db)
        processed = _normalize_peak(processed, target_db=-0.5)
        out_name = f"{stem}_{corner}.wav"
        out_path = output_dir / out_name
        _write_wav(out_path, sr, processed)
        out_paths[corner] = out_path
        print(f"  [tone] {corner}: {out_path.name}")

    return out_paths


# =============================================================================
# SAMPLE FILE DISCOVERY
# =============================================================================

WAV_EXTENSIONS = {".wav", ".WAV"}


def _find_samples_in_dir(directory: Path) -> list[Path]:
    """Return sorted list of WAV files in a directory (non-recursive)."""
    if not directory.is_dir():
        return []
    return sorted(p for p in directory.iterdir()
                  if p.suffix in WAV_EXTENSIONS)


def _find_samples_recursive(directory: Path) -> list[Path]:
    """Return sorted list of WAV files anywhere under directory."""
    if not directory.is_dir():
        return []
    return sorted(directory.rglob("*.wav"))


def _pick_best_sample(candidates: list) -> "Path | None":
    """
    From a list of candidate WAV files, return the 'best' one.
    Strategy: prefer files whose names don't start with '_' or '.';
    among ties, return the first alphabetically.
    """
    valid = [p for p in candidates if not p.name.startswith(("_", "."))]
    return (valid or candidates or [None])[0]


# =============================================================================
# XPM XML GENERATION
# =============================================================================

def _tier2_corner_comment(pad_num: int, stem: str,
                           sample_paths: dict) -> str:
    """
    Generate the Tier 2 commented-out MPCe native corner assignment block.
    pad_num is 1-based instrument index.
    sample_paths: dict corner_key -> filename (just the basename).
    """
    lines = [
        f'<!-- MPCe Native Corner Assignment (pending Akai documentation confirmation)',
        f'<PadCornerAssignment pad="{pad_num}">',
    ]
    for corner, meta in CORNER_META.items():
        fname = sample_paths.get(corner, "")
        pos = meta["position"]
        lines.append(f'  <Corner position="{pos}" sample="{xml_escape(fname)}" />')
    lines.append('</PadCornerAssignment>')
    lines.append('-->')
    return "\n".join(lines)


def _layer_block_quad(number: int, corner: str,
                       sample_name: str, sample_file: str,
                       program_slug: str) -> str:
    """Build one <Layer> XML block for a quad corner velocity zone."""
    meta = CORNER_META[corner]
    vel_start = meta["vel_start"]
    vel_end   = meta["vel_end"]
    volume    = meta["volume"]
    active    = "True" if sample_name else "False"
    file_path = (f"Samples/{program_slug}/{sample_file}"
                 if (sample_file and program_slug) else sample_file)
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


def _empty_layer_block(number: int) -> str:
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


def _instrument_block_quad(instrument_num: int,
                            midi_note: int,
                            pad_name: str,
                            mute_group: int,
                            mute_targets: list,
                            sample_paths: dict,
                            program_slug: str,
                            tier2_comment: bool = True) -> str:
    """
    Build one full <Instrument> block with 4 quad-corner velocity layers.
    sample_paths: dict corner_key -> Path or None
    """
    is_active = any(sample_paths.get(c) for c in CORNERS)

    # Prepare layer XML
    layers_xml_parts = []
    sample_basenames = {}
    for i, corner in enumerate(CORNERS):
        sp = sample_paths.get(corner)
        if sp and Path(sp).exists():
            fname = Path(sp).name
            sample_basenames[corner] = fname
            layers_xml_parts.append(
                _layer_block_quad(i + 1, corner, fname, fname, program_slug)
            )
        else:
            sample_basenames[corner] = ""
            layers_xml_parts.append(_empty_layer_block(i + 1))

    layers_xml = "\n".join(layers_xml_parts)

    # Mute target XML
    mute_xml = "\n".join(
        f"        <MuteTarget{i+1}>{t}</MuteTarget{i+1}>"
        for i, t in enumerate(mute_targets)
    )
    simult_xml = "\n".join(
        f"        <SimultTarget{i+1}>0</SimultTarget{i+1}>"
        for i in range(4)
    )

    # Tier 2 stub (commented out)
    tier2_block = ""
    if tier2_comment and is_active:
        comment = _tier2_corner_comment(instrument_num, pad_name, sample_basenames)
        # Indent the comment inside the instrument block
        indented = "\n".join(f"        {line}" for line in comment.splitlines())
        tier2_block = f"\n{indented}\n"

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
        f'        <Mono>True</Mono>\n'
        f'        <Polyphony>1</Polyphony>\n'
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
        f'        <OneShot>True</OneShot>\n'
        f'        <FilterType>2</FilterType>\n'
        f'        <Cutoff>1.000000</Cutoff>\n'
        f'        <Resonance>0.000000</Resonance>\n'
        f'        <FilterEnvAmt>0.000000</FilterEnvAmt>\n'
        f'        <AfterTouchToFilter>0.000000</AfterTouchToFilter>\n'
        f'        <VelocityToStart>0.000000</VelocityToStart>\n'
        f'        <VelocityToFilterAttack>0.000000</VelocityToFilterAttack>\n'
        f'        <VelocityToFilter>0.000000</VelocityToFilter>\n'
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
        f'        <VolumeAttack>0.000000</VolumeAttack>\n'
        f'        <VolumeDecay>0.300000</VolumeDecay>\n'
        f'        <VolumeSustain>0.000000</VolumeSustain>\n'
        f'        <VolumeRelease>0.050000</VolumeRelease>\n'
        f'        <VelocityToPitch>0.000000</VelocityToPitch>\n'
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
        f'{tier2_block}'
        f'        <Layers>\n'
        f'{layers_xml}\n'
        f'        </Layers>\n'
        f'      </Instrument>'
    )


def _qlink_xml() -> str:
    """Q-Link assignments: TONE / PITCH / CHARACTER / SPACE."""
    return (
        '    <QLinks>\n'
        '      <QLink number="1">\n'
        '        <Name>TONE</Name>\n'
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
        '        <Name>CHARACTER</Name>\n'
        '        <Parameter>Resonance</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>0.600000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="4">\n'
        '        <Name>SPACE</Name>\n'
        '        <Parameter>Send1</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>0.700000</Max>\n'
        '      </QLink>\n'
        '    </QLinks>\n'
    )


def build_xpm(program_name: str,
              program_slug: str,
              pads: list[dict],
              tier2_comment: bool = True) -> str:
    """
    Build a complete XPM drum program XML string.

    pads: list of dicts, each with keys:
        instrument_num (int, 1-based)
        midi_note      (int)
        pad_name       (str)
        mute_group     (int)
        mute_targets   (list of 4 ints)
        sample_paths   (dict corner_key -> Path or None)

    Inactive pads (127 empty slots) are filled automatically.
    """
    active_nums = {p["instrument_num"] for p in pads}
    pad_lookup = {p["instrument_num"]: p for p in pads}

    instruments_xml_parts = []

    for i in range(1, 129):
        if i in pad_lookup:
            p = pad_lookup[i]
            instruments_xml_parts.append(
                _instrument_block_quad(
                    instrument_num=i,
                    midi_note=p["midi_note"],
                    pad_name=p["pad_name"],
                    mute_group=p.get("mute_group", 0),
                    mute_targets=p.get("mute_targets", [0, 0, 0, 0]),
                    sample_paths=p["sample_paths"],
                    program_slug=program_slug,
                    tier2_comment=tier2_comment,
                )
            )
        else:
            # Empty inactive instrument
            empty_sample_paths = {c: None for c in CORNERS}
            instruments_xml_parts.append(
                _instrument_block_quad(
                    instrument_num=i,
                    midi_note=i - 1,
                    pad_name="",
                    mute_group=0,
                    mute_targets=[0, 0, 0, 0],
                    sample_paths=empty_sample_paths,
                    program_slug=program_slug,
                    tier2_comment=False,
                )
            )

    instruments_xml = "\n".join(instruments_xml_parts)
    qlinks = _qlink_xml()

    return (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        f'<!-- XPN MPCe Quad Kit — {program_name} -->\n'
        f'<!-- Generated by xpn_mpce_quad_builder.py — XO_OX Designs {TODAY} -->\n'
        f'<!-- feliX-Oscar quad-corner architecture -->\n'
        f'<!--   NW (vel 1-31):  feliX/Dry — clinical, bright, unprocessed -->\n'
        f'<!--   NE (vel 32-63): feliX/Wet — bright + FX chain -->\n'
        f'<!--   SW (vel 64-95): Oscar/Dry — warm, organic, unprocessed -->\n'
        f'<!--   SE (vel 96-127):Oscar/Wet — warm + FX chain -->\n'
        '<MPCVObject>\n'
        '  <Version>2.4</Version>\n'
        '  <Type>Program</Type>\n'
        '  <Program>\n'
        '    <Name>' + xml_escape(program_name) + '</Name>\n'
        '    <Type>Drum</Type>\n'
        f'{qlinks}'
        '    <Instruments>\n'
        f'{instruments_xml}\n'
        '    </Instruments>\n'
        '  </Program>\n'
        '</MPCVObject>\n'
    )


# =============================================================================
# MANIFEST GENERATION
# =============================================================================

def build_manifest(program_name: str,
                   program_slug: str,
                   pads: list[dict],
                   accent_color: str = "#00A6D6") -> dict:
    """Build mpce_kit_manifest.json data."""
    pad_records = []
    for p in pads:
        corner_files = {}
        for corner in CORNERS:
            sp = p["sample_paths"].get(corner)
            corner_files[corner] = str(sp) if sp else ""

        pad_records.append({
            "instrument_num": p["instrument_num"],
            "pad_name":       p["pad_name"],
            "midi_note":      p["midi_note"],
            "mute_group":     p.get("mute_group", 0),
            "corners": {
                corner: {
                    "file":        corner_files[corner],
                    "position":    CORNER_META[corner]["position"],
                    "vel_start":   CORNER_META[corner]["vel_start"],
                    "vel_end":     CORNER_META[corner]["vel_end"],
                    "label":       CORNER_META[corner]["label"],
                    "description": CORNER_META[corner]["description"],
                }
                for corner in CORNERS
            },
        })

    return {
        "program_name":    program_name,
        "program_slug":    program_slug,
        "generated_by":    "xpn_mpce_quad_builder.py",
        "generated_date":  TODAY,
        "engine":          "XO_OX feliX-Oscar Quad Architecture",
        "accent_color":    accent_color,
        "tier1_mode":      "velocity_proxy",
        "tier2_mode":      "mpce_native_corner (stub — pending Akai docs)",
        "corner_architecture": {
            corner: {
                "position":    CORNER_META[corner]["position"],
                "vel_start":   CORNER_META[corner]["vel_start"],
                "vel_end":     CORNER_META[corner]["vel_end"],
                "label":       CORNER_META[corner]["label"],
                "description": CORNER_META[corner]["description"],
            }
            for corner in CORNERS
        },
        "pad_count": len(pads),
        "pads": pad_records,
        "xpn_rules": {
            "KeyTrack":  "True — samples transpose across zones",
            "RootNote":  "0 — MPC auto-detect convention",
            "VelStart":  "1 minimum (not 0) for active layers",
        },
    }


# =============================================================================
# MPCE_SETUP.md GENERATION
# =============================================================================

MPCE_SETUP_MD = """\
# MPCe 3D Pad Setup Guide

Generated by `xpn_mpce_quad_builder.py` — XO_OX Designs

---

## The feliX-Oscar Quad Architecture

Each pad in this kit carries **4 sample variants** — one for each corner
of the feliX-Oscar polarity axis. feliX is clinical and bright; Oscar is
warm and organic. Dry is unprocessed; Wet has the full FX chain applied.

| Corner | Velocity     | Character       | Description                    |
|--------|-------------|-----------------|-------------------------------|
| NW     | vel   1–31  | feliX / Dry     | Clinical, bright, unprocessed |
| NE     | vel  32–63  | feliX / Wet     | Bright with FX chain          |
| SW     | vel  64–95  | Oscar / Dry     | Warm, organic, unprocessed    |
| SE     | vel  96–127 | Oscar / Wet     | Warm with FX chain            |

---

## Velocity Proxy — Works on All MPC Hardware (Tier 1)

This kit uses **velocity zones** to access the 4 character variants per pad.
No special MPCe hardware required — works on any MPC.

| Touch intensity | Velocity range | Corner you're accessing |
|-----------------|---------------|-------------------------|
| Light press     | vel   1–31    | NW — feliX/Dry          |
| Medium-light    | vel  32–63    | NE — feliX/Wet          |
| Medium-hard     | vel  64–95    | SW — Oscar/Dry          |
| Hard press      | vel  96–127   | SE — Oscar/Wet          |

**Tip:** Adjust pad sensitivity in MPC Settings → Hardware to shift where
your natural playing lands on the velocity curve.

---

## For MPCe Owners — 3D Pad Physical Corners

The velocity zones map directly to MPCe's physical pad corner positions:

```
NW ─────── NE
 │         │
 │   Pad   │
 │         │
SW ─────── SE
```

- **NW corner** (light touch) → feliX/Dry
- **NE corner** (medium-light) → feliX/Wet
- **SW corner** (medium-hard) → Oscar/Dry
- **SE corner** (hard press) → Oscar/Wet

Press closer to a corner + adjust pressure to blend between character voices.

> **Note:** A native MPCe `<PadCornerAssignment>` XML stub is included (commented
> out) inside the XPM file. When Akai releases official 3D pad documentation,
> that block gets un-commented and the tool is updated to write it live.

---

## Session Tips

1. **Lock a single corner** — automate the channel's MIDI output velocity to
   a fixed range to commit to one character (e.g., vel 1–31 for all-feliX/Dry).

2. **Sweep the corner axis live** — map a MIDI CC to velocity (check your
   controller's velocity-CC routing or use a DAW MIDI transform) and sweep
   from feliX to Oscar across a performance.

3. **Use both dry and wet on separate tracks** — route the same MIDI note to
   two MIDI channels, one at vel 1–31 (dry) and one at 32–63 (wet), for
   parallel processing control.

4. **Q-Links on this kit:**
   - Q1: TONE (filter cutoff)
   - Q2: PITCH (coarse tune)
   - Q3: CHARACTER (resonance)
   - Q4: SPACE (send 1 / reverb)

---

## About feliX and Oscar

feliX is the neon tetra — darting, neon-lit, clinical precision.
Oscar is the axolotl — warm-blooded, regenerative, organic depth.

Together they form the XO_OX polarity axis. Every sample in this kit
lives somewhere on that spectrum, and this quad architecture lets you
access all four quadrants with a single finger.

*XO_OX Designs — for all*
"""


# =============================================================================
# STANDARD KIT PARSER  (--from-standard-kit)
# =============================================================================

def parse_standard_xpm(xpm_path: Path) -> list[dict]:
    """
    Parse an existing standard XPM drum program.
    Returns list of dicts:
        instrument_num, midi_note (guessed from num-1), pad_name, sample_path
    Only returns instruments that have at least one active layer.
    """
    tree = ET.parse(str(xpm_path))
    root = tree.getroot()

    # XPM structure: MPCVObject/Program/Instruments/Instrument
    instruments = root.findall(".//Instrument")
    pads = []
    for inst in instruments:
        num_attr = inst.get("number")
        if num_attr is None:
            continue
        num = int(num_attr)
        layers = inst.findall(".//Layer")
        # Find first active layer with a sample file
        sample_file = ""
        for layer in layers:
            active = layer.findtext("Active", "False")
            if active.strip() == "True":
                sf = layer.findtext("SampleFile", "").strip()
                sn = layer.findtext("SampleName", "").strip()
                if sf or sn:
                    sample_file = sf or sn
                    break
        if not sample_file:
            continue

        # Try to derive the pad name from sample file
        stem = Path(sample_file).stem
        # Strip trailing _v1/_v2/_c1 etc.
        clean_stem = re.sub(r'[_-](v\d|c\d)$', '', stem, flags=re.IGNORECASE)

        pads.append({
            "instrument_num": num,
            "midi_note":      num - 1,
            "pad_name":       clean_stem,
            "sample_path":    sample_file,
        })

    return pads


# =============================================================================
# BUILD MODES
# =============================================================================

def _slugify(name: str) -> str:
    """Convert a name to a filesystem-safe slug."""
    slug = re.sub(r'[^\w\-]', '_', name.lower())
    slug = re.sub(r'_+', '_', slug).strip('_')
    return slug


def build_single_pad_kit(args) -> None:
    """
    Build a kit from 4 explicit corner directories (--felix-dry / ... flags).
    """
    corners_dirs = {
        "felix_dry": Path(args.felix_dry),
        "felix_wet": Path(args.felix_wet),
        "oscar_dry": Path(args.oscar_dry),
        "oscar_wet": Path(args.oscar_wet),
    }
    pad_name = args.pad_name or "Quad_Pad"
    program_name = args.program_name or f"{pad_name}_MPCe"
    program_slug = _slugify(program_name)
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    # Discover samples per corner
    sample_paths = {}
    for corner, cdir in corners_dirs.items():
        candidates = _find_samples_in_dir(cdir)
        if not candidates:
            print(f"  [warn] No WAV files found in {cdir} for corner {corner}")
            sample_paths[corner] = None
        else:
            # Use all found samples — for single-pad kit, pick best one
            sample_paths[corner] = _pick_best_sample(candidates)
            print(f"  [{corner}] {sample_paths[corner].name}")

    pads = [
        {
            "instrument_num": 1,
            "midi_note":      36,
            "pad_name":       pad_name,
            "mute_group":     0,
            "mute_targets":   [0, 0, 0, 0],
            "sample_paths":   sample_paths,
        }
    ]

    _write_kit_outputs(program_name, program_slug, pads, output_dir,
                       accent_color=args.accent_color)


def build_kit_from_dir(args) -> None:
    """
    Build a full multi-pad kit from a structured directory.
    Expects: kit_dir/{pad_name}/{felix_dry,felix_wet,oscar_dry,oscar_wet}/
    """
    kit_dir = Path(args.kit_dir)
    if not kit_dir.is_dir():
        print(f"ERROR: --kit-dir {kit_dir} does not exist.", file=sys.stderr)
        sys.exit(1)

    program_name = args.program_name or (kit_dir.name + "_MPCe_Quad")
    program_slug = _slugify(program_name)
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    # Discover pad subdirectories
    pad_dirs = sorted(p for p in kit_dir.iterdir() if p.is_dir())
    if not pad_dirs:
        print(f"ERROR: No subdirectories found in {kit_dir}.", file=sys.stderr)
        sys.exit(1)

    # Use GM pad map if pad count matches, otherwise number sequentially
    gm_notes = {name: (note, mg, mt) for (note, name, mg, mt) in PAD_MAP}

    pads = []
    for pad_idx, pad_dir in enumerate(pad_dirs):
        pad_name = pad_dir.name
        # Look up GM note if pad_name matches a GM voice
        if pad_name.lower() in gm_notes:
            note, mg, mt = gm_notes[pad_name.lower()]
            instr_num = [i for i, (n, nm, g, t) in enumerate(PAD_MAP)
                         if nm == pad_name.lower()][0] + 1
        else:
            note = 36 + pad_idx
            mg = 0
            mt = [0, 0, 0, 0]
            instr_num = pad_idx + 1

        sample_paths = {}
        for corner in CORNERS:
            corner_dir = pad_dir / corner
            if corner_dir.is_dir():
                candidates = _find_samples_in_dir(corner_dir)
                sample_paths[corner] = _pick_best_sample(candidates) if candidates else None
            else:
                # Also try files directly in pad_dir named *_corner*
                matches = [p for p in _find_samples_in_dir(pad_dir)
                           if corner in p.stem.lower()]
                sample_paths[corner] = _pick_best_sample(matches) if matches else None

        has_any = any(v for v in sample_paths.values())
        if not has_any:
            print(f"  [skip] {pad_name}: no samples found")
            continue

        pads.append({
            "instrument_num": instr_num,
            "midi_note":      note,
            "pad_name":       pad_name,
            "mute_group":     mg,
            "mute_targets":   list(mt),
            "sample_paths":   sample_paths,
        })
        print(f"  [pad] {pad_name} → MIDI {note} | "
              + " ".join(f"{c}={'Y' if sample_paths[c] else 'N'}"
                         for c in CORNERS))

    if not pads:
        print("ERROR: No valid pads found.", file=sys.stderr)
        sys.exit(1)

    _write_kit_outputs(program_name, program_slug, pads, output_dir,
                       accent_color=args.accent_color)


def build_from_standard_kit(args) -> None:
    """
    Parse existing standard XPM, auto-generate tone variants,
    and rebuild as quad kit.
    """
    if not SCIPY_AVAILABLE and args.auto_tone_variants:
        print("ERROR: scipy + numpy required for --auto-tone-variants.\n"
              "Install: pip install scipy numpy", file=sys.stderr)
        sys.exit(1)

    xpm_path = Path(args.from_standard_kit)
    if not xpm_path.exists():
        print(f"ERROR: {xpm_path} does not exist.", file=sys.stderr)
        sys.exit(1)

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)
    variants_dir = output_dir / "Samples"

    parsed_pads = parse_standard_xpm(xpm_path)
    if not parsed_pads:
        print("ERROR: No active instruments found in XPM.", file=sys.stderr)
        sys.exit(1)

    program_name = args.program_name or (xpm_path.stem + "_MPCe_Quad")
    program_slug = _slugify(program_name)

    # Resolve sample paths relative to the XPM directory
    xpm_dir = xpm_path.parent
    pads = []

    for p in parsed_pads:
        pad_name = p["pad_name"]
        source_wav_name = p["sample_path"]
        # Resolve: try relative to XPM, then check Samples/ sibling
        source_wav = xpm_dir / source_wav_name
        if not source_wav.exists():
            source_wav = xpm_dir / "Samples" / source_wav_name
        if not source_wav.exists():
            # Search recursively
            candidates = list(xpm_dir.rglob(Path(source_wav_name).name))
            source_wav = candidates[0] if candidates else source_wav

        if args.auto_tone_variants and source_wav.exists():
            stem = _slugify(pad_name)
            out_subdir = variants_dir / program_slug / stem
            print(f"  [auto-tone] {pad_name} ← {source_wav.name}")
            variant_paths = generate_tone_variants(source_wav, out_subdir, stem)
            sample_paths = {c: variant_paths.get(c) for c in CORNERS}
        else:
            if not source_wav.exists():
                print(f"  [warn] Source WAV not found for {pad_name}: {source_wav_name}")
            # No auto-tone: use source for all corners (producer fills the gaps)
            sample_paths = {c: source_wav if source_wav.exists() else None
                            for c in CORNERS}

        gm_map = {name: (note, mg, mt) for (note, name, mg, mt) in PAD_MAP}
        pad_lower = pad_name.lower()
        if pad_lower in gm_map:
            note, mg, mt = gm_map[pad_lower]
        else:
            note = p["midi_note"]
            mg = 0
            mt = [0, 0, 0, 0]

        pads.append({
            "instrument_num": p["instrument_num"],
            "midi_note":      note,
            "pad_name":       pad_name,
            "mute_group":     mg,
            "mute_targets":   list(mt),
            "sample_paths":   sample_paths,
        })

    _write_kit_outputs(program_name, program_slug, pads, output_dir,
                       accent_color=args.accent_color)


def _write_kit_outputs(program_name: str, program_slug: str,
                        pads: list[dict], output_dir: Path,
                        accent_color: str = "#00A6D6") -> None:
    """Write XPM, manifest JSON, and MPCE_SETUP.md to output_dir."""
    # XPM file
    xpm_content = build_xpm(program_name, program_slug, pads, tier2_comment=True)
    xpm_path = output_dir / f"{program_slug}.xpm"
    xpm_path.write_text(xpm_content, encoding="utf-8")
    print(f"\n[output] XPM: {xpm_path}")

    # Manifest JSON
    manifest = build_manifest(program_name, program_slug, pads,
                               accent_color=accent_color)
    manifest_path = output_dir / "mpce_kit_manifest.json"
    manifest_path.write_text(
        json.dumps(manifest, indent=2, ensure_ascii=False),
        encoding="utf-8"
    )
    print(f"[output] Manifest: {manifest_path}")

    # MPCE_SETUP.md
    setup_path = output_dir / "MPCE_SETUP.md"
    setup_path.write_text(MPCE_SETUP_MD, encoding="utf-8")
    print(f"[output] Setup guide: {setup_path}")

    print(f"\n[done] {len(pads)} pads | program: {program_name}")
    print(f"       {program_slug}.xpm written to {output_dir}")


# =============================================================================
# CLI
# =============================================================================

def _build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="xpn_mpce_quad_builder",
        description=(
            "XO_OX MPCe feliX-Oscar Quad Corner Kit Builder.\n"
            "Builds XPM drum programs with 4 corner velocity zones per pad.\n"
            "Generates XPM + mpce_kit_manifest.json + MPCE_SETUP.md."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )

    # --- Mode: single pad ---
    single = p.add_argument_group("Single-pad mode (one pad, 4 explicit corner dirs)")
    single.add_argument("--felix-dry",  metavar="DIR",
                        help="Directory of feliX/Dry WAVs (vel 1-31)")
    single.add_argument("--felix-wet",  metavar="DIR",
                        help="Directory of feliX/Wet WAVs (vel 32-63)")
    single.add_argument("--oscar-dry",  metavar="DIR",
                        help="Directory of Oscar/Dry WAVs (vel 64-95)")
    single.add_argument("--oscar-wet",  metavar="DIR",
                        help="Directory of Oscar/Wet WAVs (vel 96-127)")
    single.add_argument("--pad-name",   metavar="NAME",
                        help="Name for the single pad (default: Quad_Pad)")

    # --- Mode: full kit from directory ---
    kit = p.add_argument_group("Kit-dir mode (full kit from structured directory)")
    kit.add_argument("--kit-dir", metavar="DIR",
                     help=(
                         "Root directory containing {pad_name}/"
                         "{felix_dry,felix_wet,oscar_dry,oscar_wet}/ subdirs"
                     ))

    # --- Mode: from standard XPM ---
    std = p.add_argument_group("Standard-kit mode (convert existing XPM)")
    std.add_argument("--from-standard-kit", metavar="XPM",
                     help="Path to existing standard MPC XPM drum program")
    std.add_argument("--auto-tone-variants", action="store_true",
                     help=(
                         "Auto-generate feliX/Oscar tone variants using shelf "
                         "filters (requires scipy+numpy). Writes WAV variants "
                         "alongside the XPM."
                     ))

    # --- Common ---
    p.add_argument("--output", "-o", metavar="DIR", required=True,
                   help="Output directory for XPM + manifest + setup guide")
    p.add_argument("--program-name", metavar="NAME",
                   help="Override the XPM program name (default: derived from input)")
    p.add_argument("--accent-color", metavar="HEX", default="#00A6D6",
                   help="Engine accent color for manifest (default: neon tetra blue)")
    p.add_argument("--no-tier2-stub", action="store_true",
                   help="Omit the commented-out MPCe native corner XML stub")

    return p


def main() -> None:
    parser = _build_parser()
    args = parser.parse_args()

    # Determine mode
    has_single = any([args.felix_dry, args.felix_wet,
                      args.oscar_dry, args.oscar_wet])
    has_kit_dir = bool(args.kit_dir)
    has_standard = bool(getattr(args, "from_standard_kit", None))

    modes_active = sum([has_single, has_kit_dir, has_standard])
    if modes_active == 0:
        parser.error(
            "Specify one mode:\n"
            "  --felix-dry/--felix-wet/--oscar-dry/--oscar-wet  (single pad)\n"
            "  --kit-dir                                          (full kit)\n"
            "  --from-standard-kit                                (convert XPM)"
        )
    if modes_active > 1:
        parser.error("Specify only one mode at a time.")

    if has_single:
        missing = [f"--{c.replace('_','-')}" for c in
                   ["felix_dry", "felix_wet", "oscar_dry", "oscar_wet"]
                   if not getattr(args, c.replace("-", "_"), None)]
        if missing:
            parser.error(f"Single-pad mode requires all 4 corner dirs: {missing}")
        build_single_pad_kit(args)

    elif has_kit_dir:
        build_kit_from_dir(args)

    elif has_standard:
        build_from_standard_kit(args)


if __name__ == "__main__":
    main()
