#!/usr/bin/env python3
"""
XPN Turbulence Kit Generator — XO_OX Designs
Kolmogorov -5/3 turbulence: energy cascades from large eddies to small.

Physics:
  For N frequency bins, synthesize a 1D turbulent velocity field:
    A(f) ∝ f^(-5/6)   so that  Power(f) ∝ f^(-5/3)  (Kolmogorov inertial range)
    φ(f) ~ Uniform[0, 2π]  (random phase)
  Inverse DFT computed from scratch using math.cos/math.sin (no FFT, 64 bins).

3 scales:
  integral     large eddies — f_min=1, f_max=4    (smooth, long wavelength)
  inertial     cascade range — f_min=4, f_max=20  (Kolmogorov -5/3 chaos)
  dissipation  small eddies — f_min=20, f_max=64  (high-freq noise, energy drain)

Pad mapping:
  16 pads = 16 evenly-spaced time samples from the turbulent signal
  Velocity = signal amplitude normalized to 1-127
  Scale determines which frequency band is active

CLI:
  python xpn_turbulence_kit.py --scale inertial --output ./out/
  python xpn_turbulence_kit.py --scale all --output ./out/
  python xpn_turbulence_kit.py --scale inertial --seed 42 --output ./out/
"""

import argparse
import json
import math
import random
import struct
import zipfile
from datetime import date
from pathlib import Path
from typing import List
from xml.sax.saxutils import escape as xml_escape

# =============================================================================
# TURBULENCE SCALE DEFINITIONS
# =============================================================================

TURBULENCE_SCALES = {
    "integral": {
        "f_min": 1,
        "f_max": 4,
        "n_signal": 512,
        "description": "Large eddies — slow, smooth, long-wavelength structures",
    },
    "inertial": {
        "f_min": 4,
        "f_max": 20,
        "n_signal": 512,
        "description": "Inertial cascade — Kolmogorov -5/3 range, chaotic mid-scale",
    },
    "dissipation": {
        "f_min": 20,
        "f_max": 64,
        "n_signal": 512,
        "description": "Dissipation range — small eddies, energy converted to heat",
    },
}

N_BINS = 64       # frequency bins for IDFT synthesis
N_PADS = 16       # MPC pads


# =============================================================================
# TURBULENCE SIGNAL SYNTHESIS
# =============================================================================

def kolmogorov_spectrum(f: float) -> float:
    """
    Kolmogorov -5/3 power law.
    Power ∝ f^(-5/3), so amplitude ∝ f^(-5/6).
    Returns amplitude weight for frequency bin f.
    Guard: f=0 returns 0 (DC has no turbulent energy).
    """
    if f <= 0:
        return 0.0
    return f ** (-5.0 / 6.0)


def synthesize_turbulent_field(n_points: int, f_min: int, f_max: int,
                               seed: int = 0) -> List[float]:
    """
    Synthesize a 1D turbulent velocity field using Kolmogorov -5/3 spectrum.

    Steps:
      1. For bins f = f_min..f_max, compute amplitude A(f) = f^(-5/6)
      2. Draw random phase φ(f) ~ U[0, 2π]
      3. IDFT: u(x) = Σ A(f) * cos(2π f x / N + φ(f))
      4. Normalize to [-1, 1]

    Returns list of n_points float values.
    """
    rng = random.Random(seed)

    # Build (amplitude, phase) for each active frequency bin
    freq_components: List[tuple] = []
    for f in range(f_min, f_max + 1):
        amp = kolmogorov_spectrum(f)
        phase = rng.uniform(0.0, 2.0 * math.pi)
        freq_components.append((f, amp, phase))

    if not freq_components:
        return [0.0] * n_points

    # IDFT (manual, no FFT)
    signal: List[float] = []
    two_pi = 2.0 * math.pi
    for x in range(n_points):
        val = 0.0
        for f, amp, phase in freq_components:
            val += amp * math.cos(two_pi * f * x / n_points + phase)
        signal.append(val)

    # Normalize to [-1, 1]
    max_abs = max(abs(v) for v in signal)
    if max_abs < 1e-12:
        return [0.0] * n_points
    return [v / max_abs for v in signal]


def sample_signal_16(signal: List[float]) -> List[float]:
    """Sample 16 evenly-spaced points from the signal."""
    n = len(signal)
    if n < 16:
        return signal + [0.0] * (16 - n)
    indices = [int(i * (n - 1) / (N_PADS - 1)) for i in range(N_PADS)]
    return [signal[idx] for idx in indices]


def normalize_to_midi(values: List[float], lo: int = 1, hi: int = 127) -> List[int]:
    """Map float values (possibly negative) to MIDI velocity range."""
    abs_vals = [abs(v) for v in values]
    max_val = max(abs_vals) if abs_vals else 1.0
    if max_val < 1e-12:
        return [64] * len(values)
    return [
        max(lo, min(hi, int(lo + abs(v) / max_val * (hi - lo))))
        for v in values
    ]


# =============================================================================
# WAV GENERATION (stdlib struct only)
# =============================================================================

def _make_wav_mono(samples: List[float], sample_rate: int = 44100) -> bytes:
    """Generate minimal mono 16-bit PCM WAV from normalized float samples [-1, 1]."""
    n = len(samples)
    int_samples = [max(-32768, min(32767, int(s * 32767.0))) for s in samples]
    data_bytes = struct.pack(f"<{n}h", *int_samples)
    data_size = len(data_bytes)
    byte_rate = sample_rate * 2
    header = struct.pack(
        "<4sI4s4sIHHIIHH4sI",
        b"RIFF",
        36 + data_size,
        b"WAVE",
        b"fmt ",
        16,
        1,            # PCM
        1,            # mono
        sample_rate,
        byte_rate,
        2,            # block align
        16,           # bits per sample
        b"data",
        data_size,
    )
    return header + data_bytes


def make_turbulence_wav(pad_index: int, amplitude: float, scale: str,
                        f_min: int, f_max: int,
                        sample_rate: int = 44100) -> bytes:
    """
    Generate a short percussive WAV that encodes turbulent energy at this pad point.
    - Frequency band maps to pitch: integral=low, dissipation=high
    - Amplitude from the turbulent signal controls volume envelope peak
    - Duration: 0.12s
    """
    duration = 0.12
    n = int(duration * sample_rate)

    # Map scale to a base frequency
    scale_freqs = {"integral": 80.0, "inertial": 200.0, "dissipation": 600.0}
    base_freq = scale_freqs.get(scale, 200.0)
    # Each pad gets slight frequency variation across the eddy band
    freq_spread = (f_max - f_min) / N_PADS * 20.0
    freq = base_freq + pad_index * freq_spread

    # Decay: faster for small eddies (dissipation)
    decay_rate = {"integral": 15.0, "inertial": 30.0, "dissipation": 60.0}.get(scale, 30.0)
    amp = max(0.05, min(1.0, abs(amplitude)))

    samples = []
    for i in range(n):
        t = i / sample_rate
        env = amp * math.exp(-decay_rate * t)
        # Slight noise component to evoke turbulence texture
        noise = (random.Random(pad_index * 1000 + i).random() * 2.0 - 1.0) * 0.15
        sig = env * (math.sin(2.0 * math.pi * freq * t) + noise)
        samples.append(max(-1.0, min(1.0, sig)))

    return _make_wav_mono(samples, sample_rate)


# =============================================================================
# XPM XML GENERATION
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


def _empty_layer(number: int) -> str:
    return _layer_block(number, 0, 0, "", "", 0.707946)


def _empty_layers() -> str:
    return "\n".join(_empty_layer(i) for i in range(1, 5))


def _instrument_block(instrument_num: int, sample_name: str, sample_file: str,
                      velocity: int) -> str:
    """Build one Instrument element."""
    if sample_name:
        layer1 = _layer_block(1, 1, 127, sample_name, sample_file, 0.707946)
        layers_xml = layer1 + "\n" + "\n".join(_empty_layer(i) for i in range(2, 5))
    else:
        layers_xml = _empty_layers()

    mute_xml = "\n".join(f"        <MuteTarget{i+1}>0</MuteTarget{i+1}>" for i in range(4))
    simult_xml = "\n".join(f"        <SimultTarget{i+1}>0</SimultTarget{i+1}>" for i in range(4))

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
        f'        <MuteGroup>0</MuteGroup>\n'
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
        f'        <VelocityToFilter>0.200000</VelocityToFilter>\n'
        f'        <VelocityToFilterEnvelope>0.000000</VelocityToFilterEnvelope>\n'
        f'        <FilterAttack>0.000000</FilterAttack>\n'
        f'        <FilterDecay>0.200000</FilterDecay>\n'
        f'        <FilterSustain>0.500000</FilterSustain>\n'
        f'        <FilterRelease>0.100000</FilterRelease>\n'
        f'        <FilterHold>0.000000</FilterHold>\n'
        f'        <FilterDecayType>True</FilterDecayType>\n'
        f'        <FilterADEnvelope>True</FilterADEnvelope>\n'
        f'        <VolumeHold>0.000000</VolumeHold>\n'
        f'        <VolumeDecayType>True</VolumeDecayType>\n'
        f'        <VolumeADEnvelope>True</VolumeADEnvelope>\n'
        f'        <VolumeAttack>0.000000</VolumeAttack>\n'
        f'        <VolumeDecay>0.250000</VolumeDecay>\n'
        f'        <VolumeSustain>0.000000</VolumeSustain>\n'
        f'        <VolumeRelease>0.060000</VolumeRelease>\n'
        f'        <VelocityToPitch>0.000000</VelocityToPitch>\n'
        f'        <VelocityToVolumeAttack>0.000000</VelocityToVolumeAttack>\n'
        f'        <VelocitySensitivity>1.000000</VelocitySensitivity>\n'
        f'        <VelocityToVolume>0.800000</VelocityToVolume>\n'
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


def generate_xpm(scale_name: str, slug: str, pad_data: List[dict],
                 f_min: int, f_max: int, seed: int) -> str:
    """Generate full Drum Program XPM for a turbulence kit."""
    prog_name = xml_escape(f"XO-TURB-{scale_name.upper()}")

    pad_note_xml = "\n".join(
        f'        <Pad number="{p["pad"]}" note="{p["midi"]}"/>'
        for p in pad_data
    )

    midi_map = {p["midi"]: p for p in pad_data}
    instrument_parts = []
    for i in range(128):
        if i in midi_map:
            p = midi_map[i]
            instrument_parts.append(
                _instrument_block(i, p["sample_name"], p["sample_file"], p["velocity"])
            )
        else:
            instrument_parts.append(_instrument_block(i, "", "", 0))

    instruments_xml = "\n".join(instrument_parts)

    pad_json = json.dumps(
        {"ProgramPads": {"Universal": {"value0": False},
                         "Type": {"value0": 5},
                         "universalPad": 32512}},
        separators=(",", ":"),
    )

    today = date.today().isoformat()
    comment = (f"Kolmogorov -5/3 turbulence kit | scale={scale_name} | "
               f"f_min={f_min} f_max={f_max} | seed={seed} | generated={today}")

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
        f'    <!-- {xml_escape(comment)} -->\n'
        f'    <ProgramPads>{pad_json}</ProgramPads>\n'
        '    <PadNoteMap>\n'
        f'{pad_note_xml}\n'
        '    </PadNoteMap>\n'
        '    <PadGroupMap>\n'
        '    </PadGroupMap>\n'
        '    <QLinks>\n'
        '      <QLink number="1">\n'
        '        <Name>CASCADE</Name>\n'
        '        <Parameter>FilterCutoff</Parameter>\n'
        '        <Min>0.100000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="2">\n'
        '        <Name>EDDY</Name>\n'
        '        <Parameter>TuneCoarse</Parameter>\n'
        '        <Min>-12</Min>\n'
        '        <Max>12</Max>\n'
        '      </QLink>\n'
        '      <QLink number="3">\n'
        '        <Name>VISCOSITY</Name>\n'
        '        <Parameter>VolumeDecay</Parameter>\n'
        '        <Min>0.050000</Min>\n'
        '        <Max>2.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="4">\n'
        '        <Name>DIFFUSION</Name>\n'
        '        <Parameter>Send1</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>0.800000</Max>\n'
        '      </QLink>\n'
        '    </QLinks>\n'
        '    <Instruments>\n'
        f'{instruments_xml}\n'
        '    </Instruments>\n'
        '  </Program>\n'
        '</MPCVObject>\n'
    )


# =============================================================================
# KIT BUILDER
# =============================================================================

def build_kit(scale_name: str, cfg: dict, seed: int = 0) -> dict:
    """
    Synthesize turbulent field, sample 16 points, build pad data and WAVs.
    Returns dict with xpm_xml and wav_files {filename: bytes}.
    """
    f_min = cfg["f_min"]
    f_max = cfg["f_max"]
    n_signal = cfg["n_signal"]

    print(f"\n{'='*60}")
    print(f"  Scale: {scale_name}")
    print(f"  {cfg['description']}")
    print(f"  f_min={f_min}  f_max={f_max}  N_bins={N_BINS}  seed={seed}")

    # Synthesize turbulent field
    signal = synthesize_turbulent_field(n_signal, f_min, f_max, seed=seed)
    samples_16 = sample_signal_16(signal)
    velocities = normalize_to_midi(samples_16)

    # Physics summary
    energy = sum(v ** 2 for v in signal) / len(signal)
    rms = math.sqrt(energy)
    peak = max(abs(v) for v in signal)
    mean_vel = sum(velocities) / len(velocities)
    vel_range = max(velocities) - min(velocities)

    print(f"  Signal RMS: {rms:.4f}  Peak: {peak:.4f}")
    print(f"  Mean MIDI velocity: {mean_vel:.1f}  Range: {vel_range}")
    print(f"  Pad velocities: {velocities}")

    # Print per-bin amplitude (show power law in action)
    print(f"  Kolmogorov spectrum (f → A(f)):")
    step = max(1, (f_max - f_min) // 8)
    for f in range(f_min, f_max + 1, step):
        amp = kolmogorov_spectrum(f)
        bar = "█" * int(amp * 20 / kolmogorov_spectrum(f_min))
        print(f"    f={f:3d}  A={amp:.4f}  {bar}")

    slug = f"turb_{scale_name}"
    pad_data = []
    wav_files = {}

    for i, (raw_amp, vel) in enumerate(zip(samples_16, velocities)):
        midi_note = 36 + i
        pad_num = i + 1
        s_name = f"{slug}_p{pad_num:02d}"
        s_file = f"{s_name}.wav"
        pad_data.append({
            "pad": pad_num,
            "midi": midi_note,
            "sample_name": s_name,
            "sample_file": s_file,
            "velocity": vel,
        })
        wav_files[s_file] = make_turbulence_wav(i, raw_amp, scale_name, f_min, f_max)

    xpm_xml = generate_xpm(scale_name, slug, pad_data, f_min, f_max, seed)
    xpm_name = f"XO-TURB-{scale_name.upper()}.xpm"

    return {
        "xpm_name": xpm_name,
        "xpm_xml": xpm_xml,
        "wav_files": wav_files,
        "slug": slug,
        "rms": rms,
        "mean_vel": mean_vel,
    }


# =============================================================================
# ZIP OUTPUT
# =============================================================================

def write_zip(kit: dict, output_dir: Path) -> Path:
    """Package kit as XPN ZIP."""
    output_dir.mkdir(parents=True, exist_ok=True)
    zip_path = output_dir / f"{kit['slug']}.xpn"

    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
        zf.writestr(kit["xpm_name"], kit["xpm_xml"])
        for fname, wav_bytes in kit["wav_files"].items():
            zf.writestr(fname, wav_bytes)

    print(f"  => Wrote {zip_path}  "
          f"({len(kit['wav_files'])} WAVs + 1 XPM, "
          f"{zip_path.stat().st_size // 1024}KB)")
    return zip_path


# =============================================================================
# CLI
# =============================================================================

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate Kolmogorov turbulence kit XPN for MPC"
    )
    parser.add_argument(
        "--scale",
        choices=list(TURBULENCE_SCALES.keys()) + ["all"],
        default="inertial",
        help="Turbulence scale to generate (default: inertial)",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=0,
        help="Random seed for phase noise (default: 0)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("./out"),
        help="Output directory for XPN ZIPs (default: ./out)",
    )
    args = parser.parse_args()

    scales_to_run = (
        list(TURBULENCE_SCALES.keys()) if args.scale == "all" else [args.scale]
    )

    print("XPN Turbulence Kit Generator — Kolmogorov -5/3 Energy Cascade")
    print(f"Output: {args.output.resolve()}")
    print(f"N_BINS={N_BINS}  IDFT synthesizer (no FFT, pure stdlib)")

    for sname in scales_to_run:
        cfg = TURBULENCE_SCALES[sname]
        kit = build_kit(sname, cfg, seed=args.seed)
        write_zip(kit, args.output)

    print("\nDone.")


if __name__ == "__main__":
    main()
