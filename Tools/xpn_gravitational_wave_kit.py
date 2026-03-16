#!/usr/bin/env python3
"""
XPN Gravitational Wave Kit — XO_OX Designs
GW150914 chirp waveform → Akai MPC KeygroupProgram XPN packages.

GW150914 (2015-09-14): First direct detection of gravitational waves by LIGO.
Two black holes (~29 and ~36 solar masses) merging 1.3 billion light-years away.
Total energy radiated: ~3 solar masses converted to gravitational wave energy in ~0.2s.

The signal has three distinct phases:
  inspiral  — two black holes spiraling inward, frequency sweeping 35→150 Hz
  merger    — the moment of collision, maximum strain amplitude
  ringdown  — the newly-formed black hole settling into a Kerr geometry, f≈250 Hz

Waveform model (simplified but physically motivated):
  h(t) = A(t) * cos(2π * f(t) * t + φ)

  Inspiral (t < tc, tc = 0.2s):
    f(t) = f0 * (1 - t/tc)^(-3/8)          f0 = 35 Hz
    A(t) = A0 * (1 - t/tc)^(-1/4)

  Ringdown (t >= tc):
    h(t) = Apeak * exp(-(t-tc)/τ) * cos(2π * f_ring * (t-tc))
    τ = 0.01s, f_ring = 250 Hz

Pad layout per phase (16 pads, 4 banks of 4):
  inspiral  — pads  1-8: early inspiral  |  pads  9-16: late inspiral
  merger    — all 16 pads: angular samples across the ±0.02s merger window
  ringdown  — pads  1-8: early ringdown  |  pads  9-16: late ringdown

Velocity = normalized |h(t)| → 1-127 (amplitude encodes gravitational wave strain).

Optional contrast kit:
  LVT-150012 — a LIGO non-detection candidate (likely noise). Generates a
  "silence kit" using Gaussian noise envelope — the universe saying nothing.

XPM Golden Rules:
  KeyTrack  = True
  RootNote  = 0
  VelStart  = 0  (empty layers)

Usage:
  python xpn_gravitational_wave_kit.py --event GW150914 --phase inspiral --output ./out/
  python xpn_gravitational_wave_kit.py --event GW150914 --phase merger   --output ./out/
  python xpn_gravitational_wave_kit.py --event GW150914 --phase ringdown --output ./out/
  python xpn_gravitational_wave_kit.py --event GW150914 --phase all      --output ./out/
  python xpn_gravitational_wave_kit.py --event all       --output ./out/
  python xpn_gravitational_wave_kit.py --event LVT-150012 --output ./out/
"""

import argparse
import io
import math
import struct
import zipfile
from pathlib import Path
from typing import List, Tuple
from xml.sax.saxutils import escape as xml_escape

# ---------------------------------------------------------------------------
# Physical constants for GW150914 simplified chirp model
# ---------------------------------------------------------------------------

GW150914_F0 = 35.0       # Hz — starting frequency at t=0
GW150914_TC = 0.2        # s  — coalescence time (merger moment)
GW150914_A0 = 1.0        # normalized strain amplitude at t=0
GW150914_F_RING = 250.0  # Hz — ringdown quasi-normal mode frequency
GW150914_TAU = 0.01      # s  — ringdown damping time

SAMPLE_RATE = 44100      # Hz — standard audio sample rate


# ---------------------------------------------------------------------------
# GW150914 waveform synthesis
# ---------------------------------------------------------------------------

def gw150914_strain(t: float) -> float:
    """
    Compute h(t) for GW150914 simplified chirp.

    Inspiral phase (t < tc):
      f(t) = f0 * (1 - t/tc)^(-3/8)
      A(t) = A0 * (1 - t/tc)^(-1/4)
      h(t) = A(t) * cos(2π * f(t) * t)

    Ringdown phase (t >= tc):
      h(t) = Apeak * exp(-(t-tc)/τ) * cos(2π * f_ring * (t-tc))
    """
    tc = GW150914_TC
    if t < 0.0:
        return 0.0
    if t < tc:
        u = 1.0 - t / tc
        # Guard against t approaching tc exactly (u → 0)
        if u < 1e-6:
            u = 1e-6
        freq = GW150914_F0 * u ** (-3.0 / 8.0)
        amp = GW150914_A0 * u ** (-1.0 / 4.0)
        return amp * math.cos(2.0 * math.pi * freq * t)
    else:
        # Peak amplitude at merger: A0 * (epsilon)^(-1/4) — cap at a finite value
        amp_peak = GW150914_A0 * (1e-6) ** (-1.0 / 4.0)
        dt = t - tc
        return amp_peak * math.exp(-dt / GW150914_TAU) * math.cos(2.0 * math.pi * GW150914_F_RING * dt)


def build_waveform(t_start: float, t_end: float, n_points: int = 16) -> List[Tuple[float, float]]:
    """
    Sample the GW150914 waveform at n_points evenly-spaced times in [t_start, t_end].
    Returns list of (t, h(t)) tuples.
    """
    points = []
    for i in range(n_points):
        if n_points == 1:
            t = t_start
        else:
            t = t_start + i * (t_end - t_start) / (n_points - 1)
        h = gw150914_strain(t)
        points.append((t, h))
    return points


# ---------------------------------------------------------------------------
# Phase windows
# ---------------------------------------------------------------------------

PHASE_WINDOWS = {
    "inspiral": (0.0, 0.18),          # t: 0 → 0.18s (just before merger)
    "merger":   (0.18, 0.22),         # t: 0.18 → 0.22s (peak ± 0.02s)
    "ringdown": (0.20, 0.50),         # t: 0.20 → 0.50s (decay)
}

PHASE_DESCRIPTIONS = {
    "inspiral": "Two black holes spiraling inward — 35 Hz rising to 150 Hz over 0.18 seconds",
    "merger":   "The moment of collision — maximum gravitational wave strain, all 16 pads",
    "ringdown": "Kerr black hole settling — 250 Hz damped oscillation, τ=10ms",
}


# ---------------------------------------------------------------------------
# Velocity normalization
# ---------------------------------------------------------------------------

def normalize_to_velocity(values: List[float], v_min: int = 1, v_max: int = 127) -> List[int]:
    """Normalize a list of floats (absolute values) to integer velocities [v_min, v_max]."""
    abs_vals = [abs(v) for v in values]
    lo = min(abs_vals)
    hi = max(abs_vals)
    span = hi - lo
    if span < 1e-12:
        return [v_min] * len(abs_vals)
    scale = (v_max - v_min) / span
    return [max(v_min, min(v_max, int(round(v_min + (v - lo) * scale)))) for v in abs_vals]


# ---------------------------------------------------------------------------
# WAV writer — encodes the actual chirp segment as audible audio
# ---------------------------------------------------------------------------

def chirp_wav_bytes(t_center: float, duration_sec: float = 0.5,
                    sample_rate: int = SAMPLE_RATE) -> bytes:
    """
    Render a short WAV containing the GW150914 waveform centered at t_center.
    The waveform is time-shifted so t_center aligns to the middle of the clip.
    16-bit PCM mono. Amplitude is normalized to prevent clipping.
    """
    n_samples = int(sample_rate * duration_sec)
    half_dur = duration_sec / 2.0
    t_offset = t_center - half_dur

    # Gather raw samples
    raw = []
    for i in range(n_samples):
        t = t_offset + i / sample_rate
        raw.append(gw150914_strain(max(0.0, t)))

    # Normalize to 16-bit range (±30000 to leave headroom)
    peak = max(abs(v) for v in raw)
    if peak < 1e-12:
        peak = 1.0
    scale = 30000.0 / peak
    pcm_samples = [max(-32767, min(32767, int(v * scale))) for v in raw]

    data_size = n_samples * 2  # 16-bit mono
    header = struct.pack(
        "<4sI4s4sIHHIIHH4sI",
        b"RIFF", 36 + data_size, b"WAVE",
        b"fmt ", 16, 1, 1,
        sample_rate, sample_rate * 2, 2, 16,
        b"data", data_size,
    )
    audio_data = struct.pack(f"<{n_samples}h", *pcm_samples)
    return header + audio_data


def noise_wav_bytes(seed: int = 42, duration_sec: float = 0.5,
                    sample_rate: int = SAMPLE_RATE) -> bytes:
    """
    Render a WAV of Gaussian-envelope noise for LVT-150012 (the non-detection kit).
    Uses a simple LCG so no external dependencies are needed.
    """
    n_samples = int(sample_rate * duration_sec)
    # LCG random (stdlib math only)
    state = seed
    def lcg_float():
        nonlocal state
        state = (state * 1664525 + 1013904223) & 0xFFFFFFFF
        return (state / 0xFFFFFFFF) * 2.0 - 1.0

    # Gaussian-ish envelope (bell curve centered at midpoint)
    mid = n_samples / 2.0
    sigma = n_samples / 6.0
    pcm_samples = []
    for i in range(n_samples):
        envelope = math.exp(-0.5 * ((i - mid) / sigma) ** 2)
        pcm_samples.append(max(-32767, min(32767, int(lcg_float() * envelope * 8000))))

    data_size = n_samples * 2
    header = struct.pack(
        "<4sI4s4sIHHIIHH4sI",
        b"RIFF", 36 + data_size, b"WAVE",
        b"fmt ", 16, 1, 1,
        sample_rate, sample_rate * 2, 2, 16,
        b"data", data_size,
    )
    audio_data = struct.pack(f"<{n_samples}h", *pcm_samples)
    return header + audio_data


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
    """Inactive layer placeholder — VelStart=0 per golden rule."""
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
                      pad_label: str, sample_name: str,
                      velocity: int, program_slug: str) -> str:
    """
    Build one <Instrument> block with a single active velocity layer.
    The velocity from the waveform amplitude sets VelEnd (range 1-velocity).
    Pads 1-16 map to MIDI notes 36-51 (MPC Bank A/B/C/D).
    """
    vel_start = 1
    vel_end = velocity

    layers_xml = _layer_block(1, vel_start, vel_end, sample_name, program_slug)
    for n in range(2, 5):
        layers_xml += "\n" + _empty_layer(n)

    return (
        f'      <Instrument number="{instrument_num}">\n'
        f'        <MidiNote>{midi_note}</MidiNote>\n'
        f'        <MidiChannel>0</MidiChannel>\n'
        f'        <MuteGroup>0</MuteGroup>\n'
        f'        <PadNote>{pad_label}</PadNote>\n'
        f'        <LowNote>0</LowNote>\n'
        f'        <HighNote>127</HighNote>\n'
        f'        <PitchBendLow>-2</PitchBendLow>\n'
        f'        <PitchBendHigh>2</PitchBendHigh>\n'
        f'        <Volume>1.000000</Volume>\n'
        f'        <Pan>0.500000</Pan>\n'
        f'        <Detune>0</Detune>\n'
        f'        <GlideTime>0.500000</GlideTime>\n'
        f'        <CutOff>1.000000</CutOff>\n'
        f'        <Resonance>0.000000</Resonance>\n'
        f'        <FilterEnvAmount>0.000000</FilterEnvAmount>\n'
        f'        <FilterAtk>0.000000</FilterAtk>\n'
        f'        <FilterDcy>0.630957</FilterDcy>\n'
        f'        <FilterSus>1.000000</FilterSus>\n'
        f'        <FilterRel>0.630957</FilterRel>\n'
        f'        <AmpAtk>0.000000</AmpAtk>\n'
        f'        <AmpDcy>0.630957</AmpDcy>\n'
        f'        <AmpSus>1.000000</AmpSus>\n'
        f'        <AmpRel>0.630957</AmpRel>\n'
        f'        <VelSens>1.000000</VelSens>\n'
        f'        <Layers>\n'
        f'{layers_xml}\n'
        f'        </Layers>\n'
        f'      </Instrument>'
    )


# MPC pad note labels (Bank A: A01-A04, Bank B: B01-B04, etc.)
PAD_LABELS = [
    "A01", "A02", "A03", "A04",
    "B01", "B02", "B03", "B04",
    "C01", "C02", "C03", "C04",
    "D01", "D02", "D03", "D04",
]
# MIDI note numbers for pads 1-16 (MPC standard: pad 1 = note 37, ascending)
MIDI_NOTES = list(range(37, 53))


def build_xpm_xml(program_name: str, program_slug: str,
                  sample_names: List[str], velocities: List[int]) -> str:
    """
    Build a complete KeygroupProgram XPM XML document.
    sample_names: list of 16 filenames (one per pad).
    velocities:   list of 16 velocity values (1-127).
    """
    instruments_xml = []
    for i in range(16):
        instr = _instrument_block(
            instrument_num=i + 1,
            midi_note=MIDI_NOTES[i],
            pad_label=PAD_LABELS[i],
            sample_name=sample_names[i],
            velocity=velocities[i],
            program_slug=program_slug,
        )
        instruments_xml.append(instr)

    return (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<MPCVObject>\n'
        '  <Version>\n'
        '    <File_Version>2.1</File_Version>\n'
        '    <Application>MPC</Application>\n'
        '    <Application_Version>2.10</Application_Version>\n'
        '  </Version>\n'
        '  <KeygroupProgram>\n'
        f'    <ProgramName>{xml_escape(program_name)}</ProgramName>\n'
        '    <Instruments>\n'
        + "\n".join(instruments_xml) + "\n"
        '    </Instruments>\n'
        '  </KeygroupProgram>\n'
        '</MPCVObject>\n'
    )


# ---------------------------------------------------------------------------
# XPN ZIP builder
# ---------------------------------------------------------------------------

def build_xpn_zip(program_name: str, program_slug: str,
                  sample_names: List[str], velocities: List[int],
                  wav_getter) -> bytes:
    """
    Assemble an XPN ZIP containing:
      {program_slug}.xpm
      Samples/{program_slug}/{sample_name}.wav  (16 files)

    wav_getter: callable(pad_index: int) -> bytes
    """
    xpm_xml = build_xpm_xml(program_name, program_slug, sample_names, velocities)
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED) as zf:
        zf.writestr(f"{program_slug}.xpm", xpm_xml.encode("utf-8"))
        for i, sname in enumerate(sample_names):
            wav_data = wav_getter(i)
            zf.writestr(f"Samples/{program_slug}/{sname}", wav_data)
    return buf.getvalue()


# ---------------------------------------------------------------------------
# Phase kit generators
# ---------------------------------------------------------------------------

def generate_inspiral_kit(output_dir: Path) -> Path:
    """
    Inspiral phase kit.
    Pads 1-8: early inspiral (t=0.00 → 0.09s)
    Pads 9-16: late inspiral (t=0.09 → 0.18s)
    """
    t_start, t_end = PHASE_WINDOWS["inspiral"]
    waveform = build_waveform(t_start, t_end, n_points=16)
    h_values = [h for _, h in waveform]
    velocities = normalize_to_velocity(h_values)
    times = [t for t, _ in waveform]

    program_name = "GW150914 Inspiral"
    program_slug = "GW150914_Inspiral"

    sample_names = []
    for i, (t, _) in enumerate(waveform):
        sname = f"gw150914_inspiral_pad{i+1:02d}_t{t:.4f}s.wav"
        sample_names.append(sname)

    def wav_getter(pad_idx: int) -> bytes:
        t_center = times[pad_idx]
        return chirp_wav_bytes(t_center, duration_sec=0.4)

    zip_bytes = build_xpn_zip(program_name, program_slug, sample_names, velocities, wav_getter)
    out_path = output_dir / f"{program_slug}.xpn"
    out_path.write_bytes(zip_bytes)
    return out_path


def generate_merger_kit(output_dir: Path) -> Path:
    """
    Merger phase kit.
    All 16 pads = angular samples across the ±0.02s merger window.
    """
    t_start, t_end = PHASE_WINDOWS["merger"]
    waveform = build_waveform(t_start, t_end, n_points=16)
    h_values = [h for _, h in waveform]
    velocities = normalize_to_velocity(h_values)
    times = [t for t, _ in waveform]

    program_name = "GW150914 Merger"
    program_slug = "GW150914_Merger"

    sample_names = []
    for i, (t, _) in enumerate(waveform):
        sname = f"gw150914_merger_pad{i+1:02d}_t{t:.4f}s.wav"
        sample_names.append(sname)

    def wav_getter(pad_idx: int) -> bytes:
        t_center = times[pad_idx]
        return chirp_wav_bytes(t_center, duration_sec=0.15)

    zip_bytes = build_xpn_zip(program_name, program_slug, sample_names, velocities, wav_getter)
    out_path = output_dir / f"{program_slug}.xpn"
    out_path.write_bytes(zip_bytes)
    return out_path


def generate_ringdown_kit(output_dir: Path) -> Path:
    """
    Ringdown phase kit.
    Pads 1-8: early ringdown (t=0.20 → 0.35s)
    Pads 9-16: late ringdown (t=0.35 → 0.50s)
    """
    t_start, t_end = PHASE_WINDOWS["ringdown"]
    waveform = build_waveform(t_start, t_end, n_points=16)
    h_values = [h for _, h in waveform]
    velocities = normalize_to_velocity(h_values)
    times = [t for t, _ in waveform]

    program_name = "GW150914 Ringdown"
    program_slug = "GW150914_Ringdown"

    sample_names = []
    for i, (t, _) in enumerate(waveform):
        sname = f"gw150914_ringdown_pad{i+1:02d}_t{t:.4f}s.wav"
        sample_names.append(sname)

    def wav_getter(pad_idx: int) -> bytes:
        t_center = times[pad_idx]
        return chirp_wav_bytes(t_center, duration_sec=0.4)

    zip_bytes = build_xpn_zip(program_name, program_slug, sample_names, velocities, wav_getter)
    out_path = output_dir / f"{program_slug}.xpn"
    out_path.write_bytes(zip_bytes)
    return out_path


def generate_lvt150012_kit(output_dir: Path) -> Path:
    """
    LVT-150012 — the non-detection contrast kit.
    A candidate event from LIGO's first observing run that did not meet
    detection threshold. Classified as background noise.

    This kit uses Gaussian-envelope noise to represent what the universe
    sounds like when it says nothing. A silence kit with character.

    All 16 pads get noise WAVs. Velocity follows a bell-curve distribution
    (low → high → low) to mirror the Gaussian envelope shape.
    """
    # Bell-curve velocities: peak at pad 8/9, taper to edges
    mid = 7.5
    sigma = 5.0
    raw_vels = [math.exp(-0.5 * ((i - mid) / sigma) ** 2) for i in range(16)]
    velocities = normalize_to_velocity(raw_vels)

    program_name = "LVT-150012 Silence"
    program_slug = "LVT150012_Silence"

    sample_names = [f"lvt150012_noise_pad{i+1:02d}.wav" for i in range(16)]

    def wav_getter(pad_idx: int) -> bytes:
        return noise_wav_bytes(seed=pad_idx * 137 + 42, duration_sec=0.5)

    zip_bytes = build_xpn_zip(program_name, program_slug, sample_names, velocities, wav_getter)
    out_path = output_dir / f"{program_slug}.xpn"
    out_path.write_bytes(zip_bytes)
    return out_path


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate GW150914 gravitational wave XPN kits for Akai MPC.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python xpn_gravitational_wave_kit.py --event GW150914 --phase inspiral --output ./out/
  python xpn_gravitational_wave_kit.py --event GW150914 --phase merger   --output ./out/
  python xpn_gravitational_wave_kit.py --event GW150914 --phase ringdown --output ./out/
  python xpn_gravitational_wave_kit.py --event GW150914 --phase all      --output ./out/
  python xpn_gravitational_wave_kit.py --event all       --output ./out/
  python xpn_gravitational_wave_kit.py --event LVT-150012 --output ./out/

Phases:
  inspiral  — 35→150 Hz rising chirp, pads 1-8 early / 9-16 late
  merger    — peak amplitude ±0.02s, all 16 pads
  ringdown  — 250 Hz damped oscillation, pads 1-8 early / 9-16 late
  all       — generates all three GW150914 phases

Events:
  GW150914  — first direct gravitational wave detection (2015-09-14)
  LVT-150012 — LIGO non-detection candidate (noise-only contrast kit)
  all       — generates GW150914 (all phases) + LVT-150012
        """,
    )
    parser.add_argument(
        "--event",
        choices=["GW150914", "LVT-150012", "all"],
        default="GW150914",
        help="Gravitational wave event to sonify (default: GW150914)",
    )
    parser.add_argument(
        "--phase",
        choices=["inspiral", "merger", "ringdown", "all"],
        default="all",
        help="GW150914 phase to generate (default: all; ignored for LVT-150012)",
    )
    parser.add_argument(
        "--output",
        default="./out/",
        help="Output directory for XPN ZIP files (default: ./out/)",
    )
    args = parser.parse_args()

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    generated = []

    if args.event in ("GW150914", "all"):
        phases_to_run = (
            ["inspiral", "merger", "ringdown"]
            if args.phase == "all"
            else [args.phase]
        )
        generators = {
            "inspiral": generate_inspiral_kit,
            "merger":   generate_merger_kit,
            "ringdown": generate_ringdown_kit,
        }
        for phase in phases_to_run:
            print(f"Generating GW150914 {phase} kit...")
            path = generators[phase](output_dir)
            generated.append(path)
            print(f"  -> {path}")
            print(f"     {PHASE_DESCRIPTIONS[phase]}")

    if args.event in ("LVT-150012", "all"):
        print("Generating LVT-150012 silence kit (non-detection contrast)...")
        path = generate_lvt150012_kit(output_dir)
        generated.append(path)
        print(f"  -> {path}")
        print("     Gaussian noise envelope — the universe saying nothing.")

    print(f"\n{len(generated)} kit(s) written to {output_dir.resolve()}")
    for p in generated:
        size_kb = p.stat().st_size / 1024
        print(f"  {p.name}  ({size_kb:.1f} KB)")


if __name__ == "__main__":
    main()
