#!/usr/bin/env python3
"""
XPN Attractor Kit — XO_OX Designs
Chaotic attractor velocity curves for Akai MPC KeygroupProgram XPN packages.

Generates XPN ZIP archives whose velocity-layer boundaries are shaped by
chaotic attractor trajectories — Lorenz (3 presets) and Henon map.

Attractor → Pad Mapping:
  Lorenz X-axis  → pads 1–4   (Bank A)
  Lorenz Y-axis  → pads 5–8   (Bank B)
  Lorenz Z-axis  → pads 9–12  (Bank C)
  Henon X-axis   → pads 13–16 (Bank D)

Each pad gets 4 velocity layers whose breakpoints are derived from the
corresponding attractor trajectory sampled at evenly-spaced indices.

Lorenz presets:
  classic  σ=10,  ρ=28,  β=8/3   — the canonical butterfly
  subtle   σ=5,   ρ=15,  β=2     — slow drift, gentler transitions
  wild     σ=16,  ρ=45,  β=4     — hyper-chaotic, wide swings

Henon map:
  a=1.4, b=0.3   — classic parameters

XPM Rules (golden):
  KeyTrack  = True   (samples transpose across zones)
  RootNote  = 0      (MPC auto-detect convention)
  Empty layer VelStart = 0  (prevents ghost triggering)

Usage:
  python xpn_attractor_kit.py --attractor lorenz --preset classic --output ./out/
  python xpn_attractor_kit.py --attractor lorenz --preset subtle  --output ./out/
  python xpn_attractor_kit.py --attractor lorenz --preset wild    --output ./out/
  python xpn_attractor_kit.py --attractor henon  --output ./out/
  python xpn_attractor_kit.py --attractor all    --output ./out/
"""

import argparse
import io
import math
import struct
import zipfile
from datetime import date
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from xml.sax.saxutils import escape as xml_escape

# ---------------------------------------------------------------------------
# Lorenz attractor — Euler integration
# ---------------------------------------------------------------------------

def lorenz_trajectory(
    sigma: float, rho: float, beta: float,
    dt: float = 0.01, steps: int = 10_000,
    x0: float = 0.1, y0: float = 0.0, z0: float = 0.0,
) -> Tuple[List[float], List[float], List[float]]:
    """
    Integrate Lorenz system:
        dx/dt = σ(y - x)
        dy/dt = x(ρ - z) - y
        dz/dt = xy - βz
    Returns (xs, ys, zs) — lists of length `steps`.
    """
    xs, ys, zs = [], [], []
    x, y, z = x0, y0, z0
    for _ in range(steps):
        dx = sigma * (y - x)
        dy = x * (rho - z) - y
        dz = x * y - beta * z
        x += dx * dt
        y += dy * dt
        z += dz * dt
        xs.append(x)
        ys.append(y)
        zs.append(z)
    return xs, ys, zs


# ---------------------------------------------------------------------------
# Henon map
# ---------------------------------------------------------------------------

def henon_trajectory(
    a: float = 1.4, b: float = 0.3,
    steps: int = 10_000,
    x0: float = 0.1, y0: float = 0.0,
) -> Tuple[List[float], List[float]]:
    """
    Iterate Henon map:
        x_{n+1} = 1 - a * x_n^2 + y_n
        y_{n+1} = b * x_n
    Returns (xs, ys) — lists of length `steps`.
    Divergent trajectories (|x| > 1e10) are clamped to previous value.
    """
    xs, ys = [], []
    x, y = x0, y0
    for _ in range(steps):
        x_new = 1.0 - a * x * x + y
        y_new = b * x
        # Guard against divergence
        if abs(x_new) > 1e10 or math.isnan(x_new):
            x_new = x
            y_new = y
        x, y = x_new, y_new
        xs.append(x)
        ys.append(y)
    return xs, ys


# ---------------------------------------------------------------------------
# Normalization — map arbitrary float list → 1..127 velocity range
# ---------------------------------------------------------------------------

def normalize_to_velocity(values: List[float], v_min: int = 1, v_max: int = 127) -> List[int]:
    """
    Normalize a list of floats to integer velocities in [v_min, v_max].
    If all values are identical, returns all v_min.
    """
    lo = min(values)
    hi = max(values)
    span = hi - lo
    if span == 0.0:
        return [v_min] * len(values)
    scale = (v_max - v_min) / span
    return [int(round(v_min + (v - lo) * scale)) for v in values]


# ---------------------------------------------------------------------------
# Velocity breakpoint extraction
#
# For 4 velocity layers per pad we need 3 interior breakpoints + edges:
#   Layer 1: [1,      bp1]
#   Layer 2: [bp1+1,  bp2]
#   Layer 3: [bp2+1,  bp3]
#   Layer 4: [bp3+1,  127]
#
# We sample the normalized trajectory at evenly-spaced indices, extract
# 3 quartile-like breakpoints, then sort and deduplicate them.
# ---------------------------------------------------------------------------

def velocity_breakpoints(norm_traj: List[int], n_samples: int = 64) -> Tuple[int, int, int]:
    """
    Sample `n_samples` evenly-spaced values from norm_traj and derive
    three sorted interior breakpoints for a 4-layer velocity split.
    Returns (bp1, bp2, bp3) with 1 <= bp1 < bp2 < bp3 <= 126.
    """
    step = max(1, len(norm_traj) // n_samples)
    samples = sorted(norm_traj[i] for i in range(0, len(norm_traj), step))
    n = len(samples)
    # Take the 25th, 50th, and 75th percentile positions
    q1 = samples[n // 4]
    q2 = samples[n // 2]
    q3 = samples[(3 * n) // 4]
    # Ensure strict ordering and valid range
    q1 = max(2, min(q1, 124))
    q2 = max(q1 + 1, min(q2, 125))
    q3 = max(q2 + 1, min(q3, 126))
    return q1, q2, q3


# ---------------------------------------------------------------------------
# Minimal 1-second silent WAV writer (pure struct — no wave module dependency)
# ---------------------------------------------------------------------------

def silent_wav_bytes(sample_rate: int = 44100, channels: int = 1,
                     duration_sec: float = 1.0) -> bytes:
    """
    Build a minimal 16-bit PCM WAV containing silence.
    Used as placeholder samples inside the XPN ZIP.
    """
    n_samples = int(sample_rate * duration_sec)
    data_size = n_samples * channels * 2  # 16-bit = 2 bytes/sample
    header = struct.pack(
        "<4sI4s4sIHHIIHH4sI",
        b"RIFF",
        36 + data_size,          # ChunkSize
        b"WAVE",
        b"fmt ",
        16,                      # Subchunk1Size (PCM)
        1,                       # AudioFormat (PCM)
        channels,
        sample_rate,
        sample_rate * channels * 2,  # ByteRate
        channels * 2,                # BlockAlign
        16,                          # BitsPerSample
        b"data",
        data_size,
    )
    return header + bytes(data_size)


# ---------------------------------------------------------------------------
# XPM XML builders
# ---------------------------------------------------------------------------

def _layer_block(number: int, vel_start: int, vel_end: int,
                 sample_name: str, program_slug: str,
                 volume: float = 0.707946) -> str:
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
    """Empty/inactive layer — VelStart=0 per golden rule."""
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


def _instrument_block(instrument_num: int, midi_note: int,
                      pad_label: str, axis_label: str,
                      sample_names: List[str],
                      breakpoints: Tuple[int, int, int],
                      program_slug: str) -> str:
    """
    Build one <Instrument> block with 4 velocity layers.
    sample_names: list of up to 4 sample filenames (one per velocity layer).
    breakpoints: (bp1, bp2, bp3) interior velocity boundaries.
    """
    bp1, bp2, bp3 = breakpoints
    # Layer velocity ranges: [1,bp1], [bp1+1,bp2], [bp2+1,bp3], [bp3+1,127]
    ranges = [(1, bp1), (bp1 + 1, bp2), (bp2 + 1, bp3), (bp3 + 1, 127)]

    layers_parts = []
    for i in range(4):
        if i < len(sample_names) and sample_names[i]:
            vs, ve = ranges[i]
            layers_parts.append(
                _layer_block(i + 1, vs, ve, sample_names[i], program_slug)
            )
        else:
            layers_parts.append(_empty_layer(i + 1))

    layers_xml = "\n".join(layers_parts)

    return (
        f'        <Instrument number="{instrument_num}">\n'
        f'          <Active>True</Active>\n'
        f'          <Volume>1.000000</Volume>\n'
        f'          <Pan>0.500000</Pan>\n'
        f'          <Pitch>0.000000</Pitch>\n'
        f'          <MidiNote>{midi_note}</MidiNote>\n'
        f'          <ZonePlay>1</ZonePlay>\n'
        f'          <MuteGroup>0</MuteGroup>\n'
        f'          <MuteTarget>0</MuteTarget>\n'
        f'          <VoiceOverlap>0</VoiceOverlap>\n'
        f'          <Mono>False</Mono>\n'
        f'          <Polyphony>4</Polyphony>\n'
        f'          <OneShot>True</OneShot>\n'
        f'          <VelocitySensitivity>1.000000</VelocitySensitivity>\n'
        f'          <VelocityToPitch>0.000000</VelocityToPitch>\n'
        f'          <VelocityToFilter>0.000000</VelocityToFilter>\n'
        f'          <ProgramName>{xml_escape(pad_label)} — {xml_escape(axis_label)}</ProgramName>\n'
        f'          <Attack>0.000000</Attack>\n'
        f'          <Hold>0.000000</Hold>\n'
        f'          <Decay>0.350000</Decay>\n'
        f'          <Sustain>0.000000</Sustain>\n'
        f'          <Release>0.100000</Release>\n'
        f'          <FilterType>2</FilterType>\n'
        f'          <FilterCutoff>1.000000</FilterCutoff>\n'
        f'          <FilterResonance>0.000000</FilterResonance>\n'
        f'          <FilterEnvAmt>0.000000</FilterEnvAmt>\n'
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


# ---------------------------------------------------------------------------
# Pad layout — 16 pads across 4 banks
# MIDI notes follow Akai MPC convention starting at 36 (C2)
# ---------------------------------------------------------------------------

# (pad_num_1based, midi_note, axis_label, pad_label)
PAD_LAYOUT = [
    # Bank A — Lorenz X
    (1,  36, "Lorenz X", "Pad 1"),
    (2,  37, "Lorenz X", "Pad 2"),
    (3,  38, "Lorenz X", "Pad 3"),
    (4,  39, "Lorenz X", "Pad 4"),
    # Bank B — Lorenz Y
    (5,  40, "Lorenz Y", "Pad 5"),
    (6,  41, "Lorenz Y", "Pad 6"),
    (7,  42, "Lorenz Y", "Pad 7"),
    (8,  43, "Lorenz Y", "Pad 8"),
    # Bank C — Lorenz Z
    (9,  44, "Lorenz Z", "Pad 9"),
    (10, 45, "Lorenz Z", "Pad 10"),
    (11, 46, "Lorenz Z", "Pad 11"),
    (12, 47, "Lorenz Z", "Pad 12"),
    # Bank D — Henon X
    (13, 48, "Henon X",  "Pad 13"),
    (14, 49, "Henon X",  "Pad 14"),
    (15, 50, "Henon X",  "Pad 15"),
    (16, 51, "Henon X",  "Pad 16"),
]


def build_xpm(program_name: str, program_slug: str,
              axis_trajectories: Dict[str, List[int]]) -> str:
    """
    Build complete XPM XML string.
    axis_trajectories: {"Lorenz X": [norm_vel, ...], "Lorenz Y": ..., ...}
    """
    today = date.today().isoformat()
    instruments_parts = []
    instrument_num = 1

    for pad_num, midi_note, axis_label, pad_label in PAD_LAYOUT:
        # Sample names for 4 velocity layers
        sample_names = [
            f"{program_slug}_pad{pad_num:02d}_v{layer+1}.wav"
            for layer in range(4)
        ]
        # Derive breakpoints from the corresponding attractor trajectory
        traj = axis_trajectories.get(axis_label, [])
        if traj:
            # Each of the 4 pads in a bank gets a different quarter of the trajectory
            pad_in_bank = (pad_num - 1) % 4
            chunk_size = len(traj) // 4
            chunk = traj[pad_in_bank * chunk_size: (pad_in_bank + 1) * chunk_size]
            bps = velocity_breakpoints(chunk)
        else:
            bps = (32, 64, 96)

        instruments_parts.append(
            _instrument_block(
                instrument_num, midi_note, pad_label, axis_label,
                sample_names, bps, program_slug
            )
        )
        instrument_num += 1

    # Fill remaining slots — XPM requires 128 instruments total
    next_note = 52
    while instrument_num <= 128:
        instruments_parts.append(_inactive_instrument(instrument_num, next_note))
        instrument_num += 1
        next_note += 1

    instruments_xml = "\n".join(instruments_parts)

    return (
        f'<?xml version="1.0" encoding="UTF-8"?>\n'
        f'<MPCVObject>\n'
        f'  <Version>\n'
        f'    <File_Version>2.1</File_Version>\n'
        f'    <Application>MPC</Application>\n'
        f'    <Application_Version>2.11</Application_Version>\n'
        f'  </Version>\n'
        f'  <KeygroupProgram>\n'
        f'    <ProgramName>{xml_escape(program_name)}</ProgramName>\n'
        f'    <Audition>False</Audition>\n'
        f'    <NumKeygroupsShown>0</NumKeygroupsShown>\n'
        f'    <InstrumentPluginName></InstrumentPluginName>\n'
        f'    <InstrumentPluginPreset></InstrumentPluginPreset>\n'
        f'    <ProgramAudioBusName></ProgramAudioBusName>\n'
        f'    <ProgramOutput>0</ProgramOutput>\n'
        f'    <ProgramLevel>1.000000</ProgramLevel>\n'
        f'    <ProgramPan>0.500000</ProgramPan>\n'
        f'    <ProgramTranspose>0</ProgramTranspose>\n'
        f'    <ProgramTune>0.000000</ProgramTune>\n'
        f'    <PitchBendRange>2</PitchBendRange>\n'
        f'    <WheelToLFO>0.000000</WheelToLFO>\n'
        f'    <AfterTouchToFilter>0.000000</AfterTouchToFilter>\n'
        f'    <MonoGlide>False</MonoGlide>\n'
        f'    <GlideTime>0.000000</GlideTime>\n'
        f'    <LegaTo>False</LegaTo>\n'
        f'    <GlobalLFORate>1.000000</GlobalLFORate>\n'
        f'    <GlobalLFODepth>0.000000</GlobalLFODepth>\n'
        f'    <NumKeygroups>128</NumKeygroups>\n'
        f'    <!-- XO_OX Designs — Attractor Kit — {today} -->\n'
        f'    <!-- Velocity curves shaped by chaotic attractor trajectories -->\n'
        f'      <Instruments>\n'
        f'{instruments_xml}\n'
        f'      </Instruments>\n'
        f'  </KeygroupProgram>\n'
        f'</MPCVObject>\n'
    )


# ---------------------------------------------------------------------------
# XPN ZIP packager
# ---------------------------------------------------------------------------

def build_xpn_zip(program_name: str, program_slug: str,
                  xpm_xml: str, sample_rate: int = 44100) -> bytes:
    """
    Pack XPM + 64 placeholder silent WAV files into an XPN ZIP.
    Returns raw ZIP bytes.
    """
    buf = io.BytesIO()
    wav_data = silent_wav_bytes(sample_rate=sample_rate, channels=1, duration_sec=1.0)

    with zipfile.ZipFile(buf, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        # XPM program file
        xpm_path = f"{program_slug}/{program_slug}.xpm"
        zf.writestr(xpm_path, xpm_xml.encode("utf-8"))

        # 16 pads × 4 velocity layers = 64 WAV placeholders
        for pad_num in range(1, 17):
            for layer in range(1, 5):
                sample_name = f"{program_slug}_pad{pad_num:02d}_v{layer}.wav"
                wav_path = f"{program_slug}/Samples/{program_slug}/{sample_name}"
                zf.writestr(wav_path, wav_data)

    return buf.getvalue()


# ---------------------------------------------------------------------------
# Velocity distribution summary
# ---------------------------------------------------------------------------

def print_velocity_summary(program_name: str,
                           axis_trajectories: Dict[str, List[int]]) -> None:
    """Print a summary table of velocity breakpoints per pad."""
    print(f"\n{'='*72}")
    print(f"  Velocity Distribution — {program_name}")
    print(f"{'='*72}")
    print(f"  {'Pad':<8} {'Axis':<12} {'L1 range':<14} {'L2 range':<14} "
          f"{'L3 range':<14} {'L4 range'}")
    print(f"  {'-'*8} {'-'*12} {'-'*14} {'-'*14} {'-'*14} {'-'*14}")

    for pad_num, _midi, axis_label, pad_label in PAD_LAYOUT:
        traj = axis_trajectories.get(axis_label, [])
        if traj:
            pad_in_bank = (pad_num - 1) % 4
            chunk_size = len(traj) // 4
            chunk = traj[pad_in_bank * chunk_size: (pad_in_bank + 1) * chunk_size]
            bp1, bp2, bp3 = velocity_breakpoints(chunk)
        else:
            bp1, bp2, bp3 = 32, 64, 96

        r1 = f"1–{bp1}"
        r2 = f"{bp1+1}–{bp2}"
        r3 = f"{bp2+1}–{bp3}"
        r4 = f"{bp3+1}–127"
        print(f"  {pad_label:<8} {axis_label:<12} {r1:<14} {r2:<14} {r3:<14} {r4}")

    print()

    # Per-axis trajectory statistics
    print(f"  Axis Statistics:")
    print(f"  {'-'*68}")
    for axis_label, traj in axis_trajectories.items():
        lo, hi = min(traj), max(traj)
        mean = sum(traj) / len(traj)
        mid_count = sum(1 for v in traj if 40 <= v <= 90)
        extreme_count = sum(1 for v in traj if v <= 10 or v >= 117)
        print(f"  {axis_label:<12} min={lo:3d}  max={hi:3d}  mean={mean:5.1f}  "
              f"mid(40-90)={mid_count/len(traj)*100:4.1f}%  "
              f"extreme={extreme_count/len(traj)*100:4.1f}%")
    print()


# ---------------------------------------------------------------------------
# Preset definitions
# ---------------------------------------------------------------------------

LORENZ_PRESETS = {
    "classic": dict(sigma=10.0,  rho=28.0,  beta=8.0 / 3.0),
    "subtle":  dict(sigma=5.0,   rho=15.0,  beta=2.0),
    "wild":    dict(sigma=16.0,  rho=45.0,  beta=4.0),
}


def generate_lorenz_kit(preset_name: str, output_dir: Path) -> None:
    """Generate and write a Lorenz-attractor XPN kit to output_dir."""
    params = LORENZ_PRESETS[preset_name]
    print(f"\nGenerating Lorenz '{preset_name}' "
          f"(sigma={params['sigma']}, rho={params['rho']}, beta={params['beta']:.4f}) ...")

    xs, ys, zs = lorenz_trajectory(**params)
    norm_x = normalize_to_velocity(xs)
    norm_y = normalize_to_velocity(ys)
    norm_z = normalize_to_velocity(zs)

    # Henon trajectory always fills Bank D (pads 13-16)
    henon_xs, _ = henon_trajectory()
    norm_hx = normalize_to_velocity(henon_xs)

    axis_traj: Dict[str, List[int]] = {
        "Lorenz X": norm_x,
        "Lorenz Y": norm_y,
        "Lorenz Z": norm_z,
        "Henon X":  norm_hx,
    }

    program_name = f"Lorenz {preset_name.capitalize()}"
    program_slug = f"lorenz_{preset_name}"

    print_velocity_summary(program_name, axis_traj)

    xpm_xml = build_xpm(program_name, program_slug, axis_traj)
    zip_bytes = build_xpn_zip(program_name, program_slug, xpm_xml)

    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / f"{program_slug}.xpn"
    out_path.write_bytes(zip_bytes)
    print(f"  Written: {out_path}  ({len(zip_bytes) // 1024} KB)")


def generate_henon_kit(output_dir: Path) -> None:
    """Generate and write a Henon map XPN kit to output_dir."""
    print(f"\nGenerating Henon map kit (a=1.4, b=0.3) ...")

    henon_xs, henon_ys = henon_trajectory()
    norm_hx = normalize_to_velocity(henon_xs)
    norm_hy = normalize_to_velocity(henon_ys)

    # For a standalone Henon kit: alternate X and Y across banks
    axis_traj: Dict[str, List[int]] = {
        "Lorenz X": norm_hx,   # Bank A — Henon X
        "Lorenz Y": norm_hy,   # Bank B — Henon Y
        "Lorenz Z": norm_hx,   # Bank C — Henon X (second quarter window)
        "Henon X":  norm_hy,   # Bank D — Henon Y (second quarter window)
    }

    program_name = "Henon Map"
    program_slug = "henon_map"

    print_velocity_summary(program_name, axis_traj)

    xpm_xml = build_xpm(program_name, program_slug, axis_traj)
    zip_bytes = build_xpn_zip(program_name, program_slug, xpm_xml)

    output_dir.mkdir(parents=True, exist_ok=True)
    out_path = output_dir / f"{program_slug}.xpn"
    out_path.write_bytes(zip_bytes)
    print(f"  Written: {out_path}  ({len(zip_bytes) // 1024} KB)")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="XPN Attractor Kit — chaotic attractor velocity curves for MPC KeygroupPrograms",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python xpn_attractor_kit.py --attractor lorenz --preset classic --output ./out/
  python xpn_attractor_kit.py --attractor lorenz --preset subtle  --output ./out/
  python xpn_attractor_kit.py --attractor lorenz --preset wild    --output ./out/
  python xpn_attractor_kit.py --attractor henon  --output ./out/
  python xpn_attractor_kit.py --attractor all    --output ./out/
        """,
    )
    parser.add_argument(
        "--attractor",
        choices=["lorenz", "henon", "all"],
        required=True,
        help="Which attractor to use: lorenz / henon / all",
    )
    parser.add_argument(
        "--preset",
        choices=["classic", "subtle", "wild"],
        default="classic",
        help="Lorenz preset (classic / subtle / wild) — ignored for henon/all",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("./out"),
        help="Output directory for XPN ZIPs (default: ./out/)",
    )
    args = parser.parse_args()

    output_dir = args.output

    if args.attractor == "lorenz":
        generate_lorenz_kit(args.preset, output_dir)
    elif args.attractor == "henon":
        generate_henon_kit(output_dir)
    elif args.attractor == "all":
        for preset_name in LORENZ_PRESETS:
            generate_lorenz_kit(preset_name, output_dir)
        generate_henon_kit(output_dir)

    print("Done.")


if __name__ == "__main__":
    main()
