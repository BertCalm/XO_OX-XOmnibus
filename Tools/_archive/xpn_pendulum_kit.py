#!/usr/bin/env python3
"""
XPN Pendulum Kit Generator — XO_OX Designs
Huygens synchronization: two coupled pendulums spontaneously phase-lock.

Physics: θ1'' = -(g/L)sin(θ1) + k(θ2 - θ1)
         θ2'' = -(g/L)sin(θ2) + k(θ1 - θ2)
Euler integration, dt=0.01, 5000 steps.

3 presets:
  free         k=0.0,  L1=1.0, L2=1.1  (mismatched periods, no sync)
  weak         k=0.05, L1=1.0, L2=1.1  (partial sync)
  synchronized k=0.3,  L1=1.0, L2=1.1  (full Huygens sync)

Pad mapping:
  Pads 1-8:   θ1 zero-crossings (positive slope) → velocity = |ω1| normalized 1-127
  Pads 9-16:  θ2 zero-crossings (positive slope) → velocity = |ω2| normalized 1-127
  VelocityToVolume driven by mean phase difference (small diff = louder)

CLI:
  python xpn_pendulum_kit.py --preset synchronized --output ./out/
  python xpn_pendulum_kit.py --preset all --output ./out/
"""

import argparse
import json
import math
import struct
import zipfile
from datetime import date
from pathlib import Path
from typing import List, Tuple
from xml.sax.saxutils import escape as xml_escape

# =============================================================================
# PHYSICS PRESETS
# =============================================================================

PENDULUM_PRESETS = {
    "free": {
        "k": 0.0,
        "L1": 1.0,
        "L2": 1.1,
        "g": 9.81,
        "description": "No coupling — mismatched periods drift freely",
    },
    "weak": {
        "k": 0.05,
        "L1": 1.0,
        "L2": 1.1,
        "g": 9.81,
        "description": "Weak coupling — partial synchronization, phase wander",
    },
    "synchronized": {
        "k": 0.3,
        "L1": 1.0,
        "L2": 1.1,
        "g": 9.81,
        "description": "Strong coupling — full Huygens phase-lock",
    },
}

DT = 0.01
N_STEPS = 5000

# Initial conditions: θ1=0.3 rad, θ2=-0.3 rad (anti-phase start — classic Huygens experiment)
THETA1_0 = 0.3
THETA2_0 = -0.3
OMEGA1_0 = 0.0
OMEGA2_0 = 0.0


# =============================================================================
# PENDULUM SIMULATION
# =============================================================================

def simulate_pendulums(k: float, L1: float, L2: float, g: float = 9.81,
                       dt: float = DT, n_steps: int = N_STEPS
                       ) -> Tuple[List[float], List[float], List[float], List[float]]:
    """
    Euler integration of two coupled pendulums.
    Returns (theta1_series, omega1_series, theta2_series, omega2_series).
    """
    theta1, omega1 = THETA1_0, OMEGA1_0
    theta2, omega2 = THETA2_0, OMEGA2_0

    t1_list: List[float] = []
    o1_list: List[float] = []
    t2_list: List[float] = []
    o2_list: List[float] = []

    for _ in range(n_steps):
        # Euler step
        alpha1 = -(g / L1) * math.sin(theta1) + k * (theta2 - theta1)
        alpha2 = -(g / L2) * math.sin(theta2) + k * (theta1 - theta2)

        omega1 += alpha1 * dt
        omega2 += alpha2 * dt
        theta1 += omega1 * dt
        theta2 += omega2 * dt

        t1_list.append(theta1)
        o1_list.append(omega1)
        t2_list.append(theta2)
        o2_list.append(omega2)

    return t1_list, o1_list, t2_list, o2_list


def find_positive_zero_crossings(theta: List[float], omega: List[float],
                                 count: int = 8) -> List[Tuple[int, float]]:
    """
    Find up to `count` indices where theta crosses zero with positive velocity.
    Returns list of (index, |omega_at_crossing|).
    """
    crossings: List[Tuple[int, float]] = []
    for i in range(1, len(theta)):
        if theta[i - 1] < 0.0 and theta[i] >= 0.0:
            crossings.append((i, abs(omega[i])))
            if len(crossings) >= count:
                break
    # Pad if fewer than `count` crossings found (flat signal or very slow)
    while len(crossings) < count:
        crossings.append((0, 0.0))
    return crossings[:count]


def normalize_velocities(raw: List[float], lo_midi: int = 1, hi_midi: int = 127) -> List[int]:
    """Normalize a list of floats to MIDI velocity range lo_midi-hi_midi."""
    if not raw:
        return []
    max_val = max(raw)
    min_val = min(raw)
    span = max_val - min_val
    if span < 1e-12:
        return [64] * len(raw)
    return [
        max(lo_midi, min(hi_midi, int(lo_midi + (v - min_val) / span * (hi_midi - lo_midi))))
        for v in raw
    ]


def mean_phase_difference(theta1: List[float], theta2: List[float]) -> float:
    """Compute mean |θ1 - θ2| over the full trajectory (normalized 0-1)."""
    if not theta1:
        return 0.5
    diffs = [abs(a - b) for a, b in zip(theta1, theta2)]
    mean_diff = sum(diffs) / len(diffs)
    # Typical range ~0 (synced) to ~1+ (free). Clamp and normalize.
    return min(1.0, mean_diff / 1.0)


# =============================================================================
# WAV GENERATION (stdlib struct only)
# =============================================================================

def _make_wav_mono(samples: List[float], sample_rate: int = 44100) -> bytes:
    """
    Generate a minimal mono 16-bit PCM WAV from normalized float samples [-1, 1].
    Returns raw bytes.
    """
    n = len(samples)
    int_samples = [max(-32768, min(32767, int(s * 32767.0))) for s in samples]

    data_bytes = struct.pack(f"<{n}h", *int_samples)
    data_size = len(data_bytes)
    byte_rate = sample_rate * 2  # 16-bit mono
    header = struct.pack(
        "<4sI4s4sIHHIIHH4sI",
        b"RIFF",
        36 + data_size,
        b"WAVE",
        b"fmt ",
        16,           # chunk size
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


def make_pendulum_wav(crossing_idx: int, omega_mag: float, preset_name: str,
                      phase_diff: float, sample_rate: int = 44100) -> bytes:
    """
    Generate a short percussive WAV click that encodes pendulum crossing data.
    Amplitude = omega magnitude. Duration = 0.1s. Phase diff sculpts decay.
    """
    duration = 0.10
    n = int(duration * sample_rate)
    # Envelope: sharp attack, exponential decay — decay rate influenced by phase_diff
    decay_rate = 30.0 + phase_diff * 60.0  # faster decay when out-of-phase
    amp = min(1.0, omega_mag / 3.0) if omega_mag > 0 else 0.05
    freq = 220.0 + crossing_idx * 40.0  # each crossing gets a slightly different pitch

    samples = []
    for i in range(n):
        t = i / sample_rate
        env = math.exp(-decay_rate * t)
        sig = amp * env * math.sin(2.0 * math.pi * freq * t)
        samples.append(sig)

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
                      velocity: int, vtv: float = 0.5) -> str:
    """Build one Instrument element. vtv = VelocityToVolume (0-1)."""
    if sample_name:
        # Single active layer spanning full velocity range
        layer1 = _layer_block(1, 1, 127, sample_name, sample_file, 0.707946)
        layers_2_4 = "\n".join(_empty_layer(i) for i in range(2, 5))
        layers_xml = layer1 + "\n" + layers_2_4
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
        f'        <VelocityToVolume>{vtv:.6f}</VelocityToVolume>\n'
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


def generate_xpm(preset_name: str, slug: str,
                 pad_data: List[dict],
                 phase_diff_mean: float) -> str:
    """
    Generate full Drum Program XPM.
    pad_data: list of 16 dicts with keys: pad (1-16), midi, sample_name, sample_file, velocity
    phase_diff_mean: 0=synced, 1=chaotic — controls VelocityToVolume
    """
    prog_name = xml_escape(f"XO-PEND-{preset_name.upper()}")

    # VelocityToVolume: synced = 1.0 (phase locks loud), chaotic = 0.3 (flatter dynamic)
    vtv = max(0.3, 1.0 - phase_diff_mean * 0.7)

    pad_note_xml = "\n".join(
        f'        <Pad number="{p["pad"]}" note="{p["midi"]}"/>'
        for p in pad_data
    )

    # Build 128-slot instruments
    midi_map = {p["midi"]: p for p in pad_data}
    instrument_parts = []
    for i in range(128):
        if i in midi_map:
            p = midi_map[i]
            instrument_parts.append(
                _instrument_block(i, p["sample_name"], p["sample_file"], p["velocity"], vtv)
            )
        else:
            instrument_parts.append(_instrument_block(i, "", "", 0, 0.5))

    instruments_xml = "\n".join(instrument_parts)

    pad_json = json.dumps(
        {"ProgramPads": {"Universal": {"value0": False},
                         "Type": {"value0": 5},
                         "universalPad": 32512}},
        separators=(",", ":"),
    )

    today = date.today().isoformat()
    comment = (f"Huygens pendulum kit | preset={preset_name} | "
               f"mean_phase_diff={phase_diff_mean:.4f} | generated={today}")

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
        '        <Name>COUPLING</Name>\n'
        '        <Parameter>FilterCutoff</Parameter>\n'
        '        <Min>0.200000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="2">\n'
        '        <Name>SWING</Name>\n'
        '        <Parameter>TuneCoarse</Parameter>\n'
        '        <Min>-12</Min>\n'
        '        <Max>12</Max>\n'
        '      </QLink>\n'
        '      <QLink number="3">\n'
        '        <Name>DECAY</Name>\n'
        '        <Parameter>VolumeDecay</Parameter>\n'
        '        <Min>0.050000</Min>\n'
        '        <Max>1.500000</Max>\n'
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
# KIT BUILDER
# =============================================================================

def build_kit(preset_name: str, cfg: dict) -> dict:
    """
    Run simulation, find crossings, build pad data and WAVs.
    Returns dict with xpm_xml and wav_files {filename: bytes}.
    """
    k = cfg["k"]
    L1 = cfg["L1"]
    L2 = cfg["L2"]
    g = cfg["g"]

    print(f"\n{'='*60}")
    print(f"  Preset: {preset_name}")
    print(f"  {cfg['description']}")
    print(f"  k={k}  L1={L1}  L2={L2}  g={g}")
    print(f"  Simulating {N_STEPS} steps @ dt={DT}  (T={N_STEPS*DT:.1f}s)")

    theta1, omega1, theta2, omega2 = simulate_pendulums(k, L1, L2, g)

    # Natural frequency estimates
    omega0_1 = math.sqrt(g / L1)
    omega0_2 = math.sqrt(g / L2)
    T1 = 2.0 * math.pi / omega0_1
    T2 = 2.0 * math.pi / omega0_2

    print(f"  Natural periods: T1={T1:.3f}s  T2={T2:.3f}s")

    # Find zero-crossings for each pendulum
    crossings1 = find_positive_zero_crossings(theta1, omega1, count=8)
    crossings2 = find_positive_zero_crossings(theta2, omega2, count=8)

    omega_mags1 = [c[1] for c in crossings1]
    omega_mags2 = [c[1] for c in crossings2]

    vel1 = normalize_velocities(omega_mags1)
    vel2 = normalize_velocities(omega_mags2)

    phase_diff = mean_phase_difference(theta1, theta2)
    print(f"  Mean phase difference |θ1-θ2|: {phase_diff:.4f}  "
          f"({'synced' if phase_diff < 0.1 else 'partial' if phase_diff < 0.4 else 'free'})")
    print(f"  θ1 crossings found: {sum(1 for c in crossings1 if c[1] > 0)} / 8")
    print(f"  θ2 crossings found: {sum(1 for c in crossings2 if c[1] > 0)} / 8")
    print(f"  Pendulum 1 velocities (MIDI): {vel1}")
    print(f"  Pendulum 2 velocities (MIDI): {vel2}")

    slug = f"pend_{preset_name}"
    pad_data = []
    wav_files = {}

    # Pads 1-8 → θ1 crossings, MIDI notes 36-43
    for i, (crossing, vel) in enumerate(zip(crossings1, vel1)):
        crossing_idx, omega_mag = crossing
        midi_note = 36 + i
        pad_num = i + 1
        s_name = f"{slug}_p1_c{i+1:02d}"
        s_file = f"{s_name}.wav"
        pad_data.append({
            "pad": pad_num,
            "midi": midi_note,
            "sample_name": s_name,
            "sample_file": s_file,
            "velocity": vel,
        })
        wav_files[s_file] = make_pendulum_wav(i, omega_mag, preset_name, phase_diff)

    # Pads 9-16 → θ2 crossings, MIDI notes 44-51
    for i, (crossing, vel) in enumerate(zip(crossings2, vel2)):
        crossing_idx, omega_mag = crossing
        midi_note = 44 + i
        pad_num = i + 9
        s_name = f"{slug}_p2_c{i+1:02d}"
        s_file = f"{s_name}.wav"
        pad_data.append({
            "pad": pad_num,
            "midi": midi_note,
            "sample_name": s_name,
            "sample_file": s_file,
            "velocity": vel,
        })
        wav_files[s_file] = make_pendulum_wav(i, omega_mag, preset_name, phase_diff)

    xpm_xml = generate_xpm(preset_name, slug, pad_data, phase_diff)
    xpm_name = f"XO-PEND-{preset_name.upper()}.xpm"

    return {
        "xpm_name": xpm_name,
        "xpm_xml": xpm_xml,
        "wav_files": wav_files,
        "slug": slug,
        "phase_diff": phase_diff,
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
        description="Generate Huygens pendulum kit XPN for MPC"
    )
    parser.add_argument(
        "--preset",
        choices=list(PENDULUM_PRESETS.keys()) + ["all"],
        default="synchronized",
        help="Physics preset to use (default: synchronized)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("./out"),
        help="Output directory for XPN ZIPs (default: ./out)",
    )
    args = parser.parse_args()

    presets_to_run = (
        list(PENDULUM_PRESETS.keys()) if args.preset == "all" else [args.preset]
    )

    print("XPN Pendulum Kit Generator — Huygens Synchronization")
    print(f"Output: {args.output.resolve()}")

    for pname in presets_to_run:
        cfg = PENDULUM_PRESETS[pname]
        kit = build_kit(pname, cfg)
        write_zip(kit, args.output)

    print("\nDone.")


if __name__ == "__main__":
    main()
