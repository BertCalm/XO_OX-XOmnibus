#!/usr/bin/env python3
"""
XPN Attractor Velocity Kit Generator — XO_OX Designs
Hex + Rex build: chaotic attractor outputs drive velocity sequences for XPM pads.

Lorenz and Henon map trajectories are projected onto MIDI velocity (0-127).
Velocity > 64 = hit; velocity <= 64 = rest.
The actual velocity value encodes the attractor's intensity at that moment —
loud hits vary dynamically in a way no sequencer can replicate.

3-voice Lorenz kit:
  Lorenz X axis -> Pads 1-4  (kick/bass)
  Lorenz Y axis -> Pads 5-8  (snare/mid)
  Lorenz Z axis -> Pads 9-12 (hat/high)
  Henon X axis  -> Pads 13-16 (accent)
All 4 voices driven from the SAME attractor run — causally linked.

Attractor presets:
  classic_lorenz  sigma=10  rho=28    beta=8/3   (standard chaos)
  subtle_lorenz   sigma=8   rho=26.5  beta=8/3   (gentle chaos)
  wild_lorenz     sigma=14  rho=38    beta=1.8   (full chaos)
  classic_henon   a=1.4     b=0.3
  gentle_henon    a=1.0     b=0.2

CLI:
  python xpn_attractor_kit.py --system lorenz --preset classic_lorenz --output ./kits/
  python xpn_attractor_kit.py --system henon --output ./kits/
  python xpn_attractor_kit.py --system lorenz --sigma 14 --rho 38 --output ./kits/
  python xpn_attractor_kit.py --visualize lorenz
  python xpn_attractor_kit.py --visualize henon --preset gentle_henon
"""

import argparse
import json
import math
import sys
from datetime import date
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from xml.sax.saxutils import escape as xml_escape


# =============================================================================
# ATTRACTOR PRESETS
# =============================================================================

ATTRACTOR_PRESETS = {
    "classic_lorenz": {
        "type":        "lorenz",
        "sigma":       10.0,
        "rho":         28.0,
        "beta":        8.0 / 3.0,
        "description": "Standard Lorenz attractor — canonical butterfly chaos",
    },
    "subtle_lorenz": {
        "type":        "lorenz",
        "sigma":       8.0,
        "rho":         26.5,
        "beta":        8.0 / 3.0,
        "description": "Near-margin Lorenz — gentle, semi-regular chaos",
    },
    "wild_lorenz": {
        "type":        "lorenz",
        "sigma":       14.0,
        "rho":         38.0,
        "beta":        1.8,
        "description": "Far-from-equilibrium Lorenz — full chaos, dense triggers",
    },
    "classic_henon": {
        "type":        "henon",
        "a":           1.4,
        "b":           0.3,
        "description": "Classic Henon map — sparse, spiky attractor",
    },
    "gentle_henon": {
        "type":        "henon",
        "a":           1.0,
        "b":           0.2,
        "description": "Lower-amplitude Henon — periodic islands in chaos",
    },
}

# Voice defaults (shared conventions with braille_rhythm_kit and lsystem_kit)
VOICE_DEFAULTS = {
    "kick":   {"mono": True,  "poly": 1, "one_shot": True,  "vtp": 0.05, "vtf": 0.00,
               "attack": 0.0, "decay": 0.30, "sustain": 0.0, "release": 0.05,
               "cutoff": 1.0, "resonance": 0.00, "mute_group": 0},
    "snare":  {"mono": True,  "poly": 1, "one_shot": True,  "vtp": 0.00, "vtf": 0.30,
               "attack": 0.0, "decay": 0.40, "sustain": 0.0, "release": 0.08,
               "cutoff": 0.9, "resonance": 0.05, "mute_group": 0},
    "hat":    {"mono": True,  "poly": 1, "one_shot": True,  "vtp": 0.00, "vtf": 0.00,
               "attack": 0.0, "decay": 0.15, "sustain": 0.0, "release": 0.02,
               "cutoff": 0.85,"resonance": 0.10, "mute_group": 1},
    "accent": {"mono": False, "poly": 2, "one_shot": True,  "vtp": 0.00, "vtf": 0.15,
               "attack": 0.0, "decay": 0.50, "sustain": 0.0, "release": 0.10,
               "cutoff": 0.95,"resonance": 0.00, "mute_group": 0},
    "perc":   {"mono": False, "poly": 2, "one_shot": True,  "vtp": 0.00, "vtf": 0.00,
               "attack": 0.0, "decay": 0.35, "sustain": 0.0, "release": 0.08,
               "cutoff": 1.0, "resonance": 0.00, "mute_group": 0},
}

# 4-voice zone layout for the full kit
ZONE_LAYOUT = [
    {"voice": "kick",   "axis": "lorenz_x", "base_midi": 36, "pad_start": 1},
    {"voice": "snare",  "axis": "lorenz_y", "base_midi": 40, "pad_start": 5},
    {"voice": "hat",    "axis": "lorenz_z", "base_midi": 44, "pad_start": 9},
    {"voice": "accent", "axis": "henon_x",  "base_midi": 48, "pad_start": 13},
]


# =============================================================================
# ATTRACTOR SYSTEMS
# =============================================================================

def lorenz_sequence(sigma: float = 10.0, rho: float = 28.0,
                    beta: float = 8.0 / 3.0,
                    dt: float = 0.01, n: int = 1000,
                    warmup: int = 200) -> List[Tuple[float, float, float]]:
    """
    Runge-Kutta 4 integration of the Lorenz system.
    Discards warmup steps so trajectory settles onto the attractor.
    Returns list of (x, y, z) tuples.
    """
    x, y, z = 0.1, 0.0, 0.0

    def deriv(x_, y_, z_):
        dx = sigma * (y_ - x_)
        dy = x_ * (rho - z_) - y_
        dz = x_ * y_ - beta * z_
        return dx, dy, dz

    seq = []
    for i in range(n + warmup):
        # RK4
        k1x, k1y, k1z = deriv(x, y, z)
        k2x, k2y, k2z = deriv(x + dt/2*k1x, y + dt/2*k1y, z + dt/2*k1z)
        k3x, k3y, k3z = deriv(x + dt/2*k2x, y + dt/2*k2y, z + dt/2*k2z)
        k4x, k4y, k4z = deriv(x + dt*k3x,   y + dt*k3y,   z + dt*k3z)
        x += dt/6 * (k1x + 2*k2x + 2*k3x + k4x)
        y += dt/6 * (k1y + 2*k2y + 2*k3y + k4y)
        z += dt/6 * (k1z + 2*k2z + 2*k3z + k4z)
        if i >= warmup:
            seq.append((x, y, z))

    return seq


def henon_sequence(a: float = 1.4, b: float = 0.3,
                   n: int = 1000, warmup: int = 200) -> List[Tuple[float, float]]:
    """
    Henon map iteration.
    Discards warmup steps. Returns list of (x, y) tuples.
    On divergence (|x|>1e6), resets to a nearby perturbed start.
    """
    x, y = 0.1, 0.1
    seq = []
    restarts = 0
    i = 0
    while len(seq) < n:
        if abs(x) > 1e6 or math.isnan(x):
            # Diverged — restart from slightly different IC
            restarts += 1
            x = 0.1 + restarts * 0.03
            y = 0.1 - restarts * 0.01
            i = 0
            continue
        x_new = 1.0 - a * x * x + y
        y_new = b * x
        x, y = x_new, y_new
        if i >= warmup:
            seq.append((x, y))
        i += 1
    return seq


def normalize_to_midi(values: List[float]) -> List[int]:
    """Normalize a list of floats to integer MIDI velocities 1-127."""
    if not values:
        return []
    lo = min(values)
    hi = max(values)
    span = hi - lo
    if span < 1e-10:
        return [64] * len(values)
    return [max(1, min(127, int(1 + (v - lo) / span * 126))) for v in values]


def sample_attractor_16(seq: List[tuple], axis_idx: int = 0) -> List[int]:
    """
    Sample a single axis of an attractor trajectory at 16 evenly-spaced points.
    axis_idx: 0=x, 1=y, 2=z
    Returns list of 16 MIDI velocity values (1-127).
    """
    n = len(seq)
    if n < 16:
        return [64] * 16
    indices = [int(i * (n - 1) / 15) for i in range(16)]
    raw = [seq[idx][axis_idx] for idx in indices]
    return normalize_to_midi(raw)


def velocity_to_step(vel: int) -> dict:
    """Convert a MIDI velocity value to a bar-step dict."""
    hit = vel > 64
    return {
        "hit":          hit,
        "velocity":     vel if hit else 0,
        "accented":     vel >= 100,
        "pitch_offset": 0,
    }


# =============================================================================
# XPM GENERATION  (shared layer/instrument builders)
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


def _attractor_layers(sample_name: str, sample_file: str, velocity: int) -> str:
    """
    Build 4 velocity layers centred on the attractor velocity value.
    The actual velocity band containing `velocity` uses a higher volume.
    All bands span the full 1-127 range so any incoming velocity triggers,
    but the MPC selects the layer whose VelStart/VelEnd bracket the note velocity.
    """
    if not sample_name:
        return _empty_layers()

    # Derive a volume for each quarter based on proximity to attractor velocity
    # Layer 1: vel 1-31   Layer 2: 32-63   Layer 3: 64-95   Layer 4: 96-127
    layers = [
        (1,   1,  31),
        (2,  32,  63),
        (3,  64,  95),
        (4,  96, 127),
    ]
    blocks = []
    for num, vs, ve in layers:
        # Distance of layer midpoint from attractor velocity
        midpoint = (vs + ve) / 2.0
        dist = abs(velocity - midpoint)
        # Volume 1.0 at exact match, scaling down with distance
        vol = max(0.3, 1.0 - dist / 127.0)
        blocks.append(_layer_block(num, vs, ve, sample_name, sample_file, vol))

    return "\n".join(blocks)


def _instrument_block(instrument_num: int, voice_key: str,
                      sample_name: str, sample_file: str,
                      step: Optional[dict] = None,
                      mute_group: int = 0) -> str:
    cfg = VOICE_DEFAULTS.get(voice_key, VOICE_DEFAULTS["perc"])
    is_active = bool(sample_name and step and step.get("hit"))

    if is_active:
        layers_xml = _attractor_layers(sample_name, sample_file, step["velocity"])
    else:
        layers_xml = _empty_layers()

    mono_str    = "True" if cfg["mono"] else "False"
    oneshot_str = "True" if cfg["one_shot"] else "False"

    mute_xml = "\n".join(
        f"        <MuteTarget{i+1}>0</MuteTarget{i+1}>" for i in range(4)
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


def generate_attractor_xpm(preset_name: str, slug: str,
                            zone_steps: list[dict],
                            comment: str = "") -> str:
    """
    Generate full Drum Program XPM for an attractor kit.

    zone_steps: list of {voice, base_midi, pad_start, steps: list[16 step dicts]}
    """
    prog_name = xml_escape(f"XO_OX-ATTR-{preset_name.upper().replace(' ', '-')}")

    # Map MIDI note -> (voice_key, step)
    instrument_map: dict[int, tuple[str, dict]] = {}
    pad_note_entries = []
    pad_group_entries = []

    for zone in zone_steps:
        vkey = zone["voice"]
        for i, step in enumerate(zone["steps"]):
            midi = zone["base_midi"] + i
            instrument_map[midi] = (vkey, step)
            pad_num = zone["pad_start"] + i
            if pad_num <= 16:
                pad_note_entries.append(
                    f'        <Pad number="{pad_num}" note="{midi}"/>'
                    f'  <!-- {zone["axis"]} step {i+1} vel={step["velocity"]} hit={step["hit"]} -->'
                )
                mg = VOICE_DEFAULTS.get(vkey, VOICE_DEFAULTS["perc"])["mute_group"]
                if mg > 0:
                    pad_group_entries.append(
                        f'        <Pad number="{pad_num}" group="{mg}"/>'
                    )

    # Build 128-slot instruments
    instrument_parts = []
    for i in range(128):
        if i in instrument_map:
            vkey, step = instrument_map[i]
            s_idx = i - list(instrument_map.keys())[
                [k for k in instrument_map].index(i)
            ] if False else i
            # Build sample name from zone + step
            # Find which zone this MIDI belongs to
            s_name = ""
            s_file = ""
            for zone in zone_steps:
                idx = i - zone["base_midi"]
                if 0 <= idx < 16:
                    s_name = f"{slug}_{zone['axis']}_s{idx+1:02d}"
                    s_file = f"{s_name}.wav"
                    break
            mg = VOICE_DEFAULTS.get(vkey, VOICE_DEFAULTS["perc"])["mute_group"]
            instrument_parts.append(
                _instrument_block(i, vkey, s_name if step["hit"] else "",
                                  s_file if step["hit"] else "", step, mute_group=mg)
            )
        else:
            instrument_parts.append(_instrument_block(i, "", "", "", None, 0))

    instruments_xml = "\n".join(instrument_parts)
    pad_note_xml    = "\n".join(pad_note_entries)
    pad_group_xml   = "\n".join(pad_group_entries)

    pad_json = json.dumps(
        {"ProgramPads": {"Universal": {"value0": False},
                         "Type": {"value0": 5},
                         "universalPad": 32512}},
        separators=(",", ":"),
    )

    comment_line = f'\n    <!-- {comment} -->' if comment else ""

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
        f'    <Name>{prog_name}</Name>{comment_line}\n'
        f'    <ProgramPads>{pad_json}</ProgramPads>\n'
        '    <PadNoteMap>\n'
        f'{pad_note_xml}\n'
        '    </PadNoteMap>\n'
        '    <PadGroupMap>\n'
        f'{pad_group_xml}\n'
        '    </PadGroupMap>\n'
        '    <QLinks>\n'
        '      <QLink number="1">\n'
        '        <Name>CHAOS</Name>\n'
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
        '        <Name>DECAY</Name>\n'
        '        <Parameter>VolumeDecay</Parameter>\n'
        '        <Min>0.050000</Min>\n'
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
# ASCII PHASE PORTRAIT VISUALIZER
# =============================================================================

def render_phase_portrait_lorenz(seq: List[tuple], width: int = 60,
                                 height: int = 24,
                                 axes: tuple = (0, 2)) -> str:
    """
    Render Lorenz phase portrait as ASCII art.
    Default: X-Z plane (the classic butterfly view).
    """
    ax1, ax2 = axes
    axis_names = ["X", "Y", "Z"]
    name1, name2 = axis_names[ax1], axis_names[ax2]

    xs = [p[ax1] for p in seq]
    ys = [p[ax2] for p in seq]

    x_min, x_max = min(xs), max(xs)
    y_min, y_max = min(ys), max(ys)

    grid = [["·"] * width for _ in range(height)]

    for x, y in zip(xs, ys):
        col = int((x - x_min) / (x_max - x_min + 1e-10) * (width - 1))
        row = int((1.0 - (y - y_min) / (y_max - y_min + 1e-10)) * (height - 1))
        col = max(0, min(width - 1, col))
        row = max(0, min(height - 1, row))
        grid[row][col] = "●"

    lines = []
    lines.append(f"  {name2}↑")
    for i, row in enumerate(grid):
        prefix = "  |" if i < height - 1 else "  +"
        lines.append(prefix + "".join(row))
    lines.append("   " + "─" * width + f"→ {name1}")
    lines.append(f"   [{x_min:.1f} .. {x_max:.1f}]  {name2}:[{y_min:.1f} .. {y_max:.1f}]")
    return "\n".join(lines)


def render_phase_portrait_henon(seq: List[tuple], width: int = 60,
                                height: int = 24) -> str:
    """Render Henon attractor X-Y phase portrait as ASCII art."""
    xs = [p[0] for p in seq]
    ys = [p[1] for p in seq]

    x_min, x_max = min(xs), max(xs)
    y_min, y_max = min(ys), max(ys)

    grid = [["·"] * width for _ in range(height)]

    for x, y in zip(xs, ys):
        col = int((x - x_min) / (x_max - x_min + 1e-10) * (width - 1))
        row = int((1.0 - (y - y_min) / (y_max - y_min + 1e-10)) * (height - 1))
        col = max(0, min(width - 1, col))
        row = max(0, min(height - 1, row))
        grid[row][col] = "●"

    lines = []
    lines.append("  Y↑")
    for i, row in enumerate(grid):
        prefix = "  |" if i < height - 1 else "  +"
        lines.append(prefix + "".join(row))
    lines.append("   " + "─" * width + "→ X")
    lines.append(f"   X:[{x_min:.3f}..{x_max:.3f}]  Y:[{y_min:.3f}..{y_max:.3f}]")
    return "\n".join(lines)


def render_attractor_viz(system_type: str, params: dict,
                         lorenz_seq: Optional[List[tuple]],
                         henon_seq: Optional[List[tuple]],
                         vel_sequences: dict,
                         preset_name: str) -> str:
    """Full attractor visualization: phase portrait + velocity bars."""
    lines = []
    lines.append("=" * 72)
    lines.append("XO_OX ATTRACTOR VELOCITY KIT — PHASE PORTRAIT + VELOCITY BARS")
    lines.append(f"Preset    : {preset_name}")
    lines.append(f"System    : {system_type}")

    if system_type == "lorenz":
        lines.append(f"Parameters: σ={params['sigma']}  ρ={params['rho']}  β={params['beta']:.4f}")
        lines.append(f"Description: {params.get('description', '')}")
        lines.append("")
        lines.append("Phase Portrait (X-Z plane — classic butterfly view):")
        lines.append("")
        if lorenz_seq:
            lines.append(render_phase_portrait_lorenz(lorenz_seq, axes=(0, 2)))
        lines.append("")
        lines.append("Phase Portrait (X-Y plane):")
        lines.append("")
        if lorenz_seq:
            lines.append(render_phase_portrait_lorenz(lorenz_seq, axes=(0, 1)))
    else:
        lines.append(f"Parameters: a={params['a']}  b={params['b']}")
        lines.append(f"Description: {params.get('description', '')}")
        lines.append("")
        lines.append("Phase Portrait (X-Y plane — Henon attractor):")
        lines.append("")
        if henon_seq:
            lines.append(render_phase_portrait_henon(henon_seq))

    lines.append("")
    lines.append("=" * 72)
    lines.append("Velocity Sequences (16 steps sampled from attractor trajectory)")
    lines.append("")

    axis_labels = {
        "lorenz_x": "Lorenz X → Pads 1-4  (kick/bass)",
        "lorenz_y": "Lorenz Y → Pads 5-8  (snare/mid)",
        "lorenz_z": "Lorenz Z → Pads 9-12 (hat/high)",
        "henon_x":  "Henon X  → Pads 13-16 (accent)",
    }

    for axis, label in axis_labels.items():
        if axis not in vel_sequences:
            continue
        vels = vel_sequences[axis]
        lines.append(f"  {label}")
        # Velocity bar chart
        bar_line = "  Step : " + "".join(f"{i+1:>4}" for i in range(16))
        lines.append(bar_line)
        vel_line = "  Vel  : " + "".join(f"{v:>4}" for v in vels)
        lines.append(vel_line)
        hit_line = "  Hit  : " + "".join("   X" if v > 64 else "   ." for v in vels)
        lines.append(hit_line)
        # Horizontal bar chart
        lines.append("")
        for i, v in enumerate(vels):
            bar_len = int(v / 127 * 40)
            bar = "█" * bar_len
            hit_mark = "←HIT" if v > 64 else "    "
            lines.append(f"  {i+1:>2} [{v:>3}] {bar:<40} {hit_mark}")
        lines.append("")

    # Summary stats
    lines.append("─" * 72)
    lines.append("Summary:")
    for axis, label in axis_labels.items():
        if axis not in vel_sequences:
            continue
        vels = vel_sequences[axis]
        hits   = sum(1 for v in vels if v > 64)
        avg    = sum(vels) / len(vels)
        peak   = max(vels)
        trough = min(vels)
        binary = "".join("1" if v > 64 else "0" for v in vels)
        lines.append(f"  {axis:12s}: {hits}/16 hits  avg={avg:.0f}  "
                     f"range=[{trough},{peak}]  pattern={binary}")

    lines.append("=" * 72)
    return "\n".join(lines)


# =============================================================================
# MANIFEST
# =============================================================================

def build_manifest(preset_name: str, system_type: str, params: dict,
                   vel_sequences: dict, slug: str) -> dict:
    zone_data = []
    for zone in ZONE_LAYOUT:
        axis = zone["axis"]
        if axis not in vel_sequences:
            continue
        vels   = vel_sequences[axis]
        binary = "".join("1" if v > 64 else "0" for v in vels)
        hits   = binary.count("1")
        zone_data.append({
            "voice":      zone["voice"],
            "axis":       axis,
            "base_midi":  zone["base_midi"],
            "pad_start":  zone["pad_start"],
            "velocities": vels,
            "binary":     binary,
            "hit_count":  hits,
            "density_pct": round(hits / 16 * 100, 1),
            "avg_velocity": round(sum(vels) / len(vels), 1),
            "peak_velocity": max(vels),
        })

    params_clean = {k: (round(v, 6) if isinstance(v, float) else v)
                    for k, v in params.items() if k != "type" and k != "description"}

    return {
        "tool":        "xpn_attractor_kit",
        "version":     "1.0.0",
        "generated":   str(date.today()),
        "preset_name": preset_name,
        "system_type": system_type,
        "parameters":  params_clean,
        "description": params.get("description", ""),
        "slug":        slug,
        "zones":       zone_data,
        "phase_portrait_description": (
            "Lorenz: butterfly-shaped double-lobe in X-Z plane; "
            "sensitive to initial conditions." if system_type == "lorenz" else
            "Henon: folded crescent attractor in X-Y plane; "
            "sparse and spiky compared to Lorenz."
        ),
    }


# =============================================================================
# MAIN PIPELINE
# =============================================================================

def process_attractor(system_type: str, params: dict, preset_name: str,
                      output_dir: Path) -> None:
    """Run attractor, generate XPM + manifest + viz."""
    slug = f"attr_{preset_name.lower().replace(' ', '_').replace('-', '_')}"

    print(f"[Hex+Rex] Attractor: {preset_name} ({system_type})")
    if system_type == "lorenz":
        sigma = params["sigma"]
        rho   = params["rho"]
        beta  = params["beta"]
        print(f"          σ={sigma}  ρ={rho}  β={beta:.4f}")
        lorenz_seq = lorenz_sequence(sigma=sigma, rho=rho, beta=beta, n=1000)
        henon_seq  = henon_sequence(a=1.4, b=0.3, n=1000)  # always run Henon for pad 13-16
        print(f"          Lorenz: {len(lorenz_seq)} steps computed")
        vel_lorenz_x = sample_attractor_16(lorenz_seq, axis_idx=0)
        vel_lorenz_y = sample_attractor_16(lorenz_seq, axis_idx=1)
        vel_lorenz_z = sample_attractor_16(lorenz_seq, axis_idx=2)
        vel_henon_x  = sample_attractor_16(henon_seq,  axis_idx=0)
    else:
        a = params["a"]
        b = params["b"]
        print(f"          a={a}  b={b}")
        lorenz_seq = lorenz_sequence(n=1000)  # run Lorenz for pads 1-12
        henon_seq  = henon_sequence(a=a, b=b, n=1000)
        print(f"          Henon: {len(henon_seq)} steps computed")
        vel_lorenz_x = sample_attractor_16(lorenz_seq, axis_idx=0)
        vel_lorenz_y = sample_attractor_16(lorenz_seq, axis_idx=1)
        vel_lorenz_z = sample_attractor_16(lorenz_seq, axis_idx=2)
        vel_henon_x  = sample_attractor_16(henon_seq,  axis_idx=0)

    vel_sequences = {
        "lorenz_x": vel_lorenz_x,
        "lorenz_y": vel_lorenz_y,
        "lorenz_z": vel_lorenz_z,
        "henon_x":  vel_henon_x,
    }

    # Build zone_steps for XPM generator
    zone_steps = []
    for zone in ZONE_LAYOUT:
        axis = zone["axis"]
        vels = vel_sequences[axis]
        steps = [velocity_to_step(v) for v in vels]
        zone_steps.append({
            "voice":      zone["voice"],
            "axis":       axis,
            "base_midi":  zone["base_midi"],
            "pad_start":  zone["pad_start"],
            "steps":      steps,
        })

    # Print quick pattern summary
    for zone in zone_steps:
        binary = "".join("1" if s["hit"] else "0" for s in zone["steps"])
        print(f"          {zone['axis']:12s}: {binary}  ({binary.count('1')}/16)")

    # 1. XPM
    comment = (f"Attractor: {system_type} preset={preset_name} "
               f"lorenz(σ={lorenz_seq and params.get('sigma', 10)}) "
               f"henon(a={params.get('a', 1.4)})")
    xpm_xml  = generate_attractor_xpm(preset_name, slug, zone_steps, comment=comment)
    xpm_path = output_dir / f"{slug}_kit.xpm"
    xpm_path.write_text(xpm_xml, encoding="utf-8")
    print(f"[Hex+Rex] XPM      -> {xpm_path}")

    # 2. Manifest
    manifest = build_manifest(preset_name, system_type, params, vel_sequences, slug)
    manifest_path = output_dir / f"{slug}_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"[Hex+Rex] Manifest -> {manifest_path}")

    # 3. Viz
    viz_txt  = render_attractor_viz(system_type, params,
                                    lorenz_seq if system_type == "lorenz" else lorenz_seq,
                                    henon_seq,
                                    vel_sequences, preset_name)
    viz_path = output_dir / f"{slug}_viz.txt"
    viz_path.write_text(viz_txt, encoding="utf-8")
    print(f"[Hex+Rex] Viz      -> {viz_path}")


# =============================================================================
# MAIN
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Attractor Velocity Kit Generator — XO_OX / Hex+Rex",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--system", default="lorenz",
                        choices=["lorenz", "henon"],
                        help="Attractor system (lorenz|henon)")
    parser.add_argument("--preset", default=None,
                        help="Preset name from ATTRACTOR_PRESETS "
                             "(classic_lorenz|subtle_lorenz|wild_lorenz|classic_henon|gentle_henon)")
    parser.add_argument("--output", "-o", default="./kits",
                        help="Output directory (default: ./kits)")
    parser.add_argument("--visualize", metavar="SYSTEM",
                        help="Print phase portrait for system and exit (lorenz|henon)")
    # Lorenz overrides
    parser.add_argument("--sigma", type=float, help="Lorenz σ (default 10.0)")
    parser.add_argument("--rho",   type=float, help="Lorenz ρ (default 28.0)")
    parser.add_argument("--beta",  type=float, help="Lorenz β (default 2.667)")
    # Henon overrides
    parser.add_argument("--a", type=float, help="Henon a parameter (default 1.4)")
    parser.add_argument("--b", type=float, help="Henon b parameter (default 0.3)")
    # List presets
    parser.add_argument("--list-presets", action="store_true",
                        help="List all available presets and exit")

    args = parser.parse_args()

    if args.list_presets:
        print("\nAvailable attractor presets:")
        print("-" * 60)
        for name, p in ATTRACTOR_PRESETS.items():
            print(f"  {name:20s} ({p['type']:6s}) — {p['description']}")
        print()
        sys.exit(0)

    # --visualize
    if args.visualize:
        stype = args.visualize.lower()
        preset_name = args.preset or (
            "classic_lorenz" if stype == "lorenz" else "classic_henon"
        )
        if preset_name in ATTRACTOR_PRESETS:
            params = ATTRACTOR_PRESETS[preset_name]
        else:
            params = ATTRACTOR_PRESETS["classic_lorenz"] if stype == "lorenz" \
                else ATTRACTOR_PRESETS["classic_henon"]

        if stype == "lorenz":
            seq = lorenz_sequence(sigma=params["sigma"],
                                  rho=params["rho"],
                                  beta=params["beta"],
                                  n=1000)
            vel_seqs = {
                "lorenz_x": sample_attractor_16(seq, 0),
                "lorenz_y": sample_attractor_16(seq, 1),
                "lorenz_z": sample_attractor_16(seq, 2),
                "henon_x":  sample_attractor_16(henon_sequence(n=1000), 0),
            }
            print(render_attractor_viz("lorenz", params, seq, None, vel_seqs, preset_name))
        else:
            seq = henon_sequence(a=params.get("a", 1.4), b=params.get("b", 0.3), n=1000)
            lseq = lorenz_sequence(n=1000)
            vel_seqs = {
                "lorenz_x": sample_attractor_16(lseq, 0),
                "lorenz_y": sample_attractor_16(lseq, 1),
                "lorenz_z": sample_attractor_16(lseq, 2),
                "henon_x":  sample_attractor_16(seq, 0),
            }
            print(render_attractor_viz("henon", params, None, seq, vel_seqs, preset_name))
        sys.exit(0)

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    # Resolve params
    system_type = args.system.lower()

    if args.preset and args.preset in ATTRACTOR_PRESETS:
        params = dict(ATTRACTOR_PRESETS[args.preset])
        preset_name = args.preset
    else:
        # Build from system defaults + overrides
        if system_type == "lorenz":
            params = {
                "type":        "lorenz",
                "sigma":       args.sigma or 10.0,
                "rho":         args.rho   or 28.0,
                "beta":        args.beta  or (8.0 / 3.0),
                "description": "Custom Lorenz parameters",
            }
            preset_name = (f"lorenz_s{params['sigma']}_r{params['rho']}"
                           .replace(".", "p"))
        else:
            params = {
                "type":        "henon",
                "a":           args.a or 1.4,
                "b":           args.b or 0.3,
                "description": "Custom Henon parameters",
            }
            preset_name = (f"henon_a{params['a']}_b{params['b']}"
                           .replace(".", "p"))

    # Apply CLI overrides on top of preset
    if system_type == "lorenz":
        if args.sigma: params["sigma"] = args.sigma
        if args.rho:   params["rho"]   = args.rho
        if args.beta:  params["beta"]  = args.beta
    else:
        if args.a: params["a"] = args.a
        if args.b: params["b"] = args.b

    process_attractor(system_type, params, preset_name, output_dir)

    print(f"\n[Hex+Rex] Done. Load {preset_name} kit into your MPC.")
    print(f"          Output: {output_dir}/")
    print()
    print("  Pad layout:")
    print("    Pads  1-4 : Lorenz X axis → kick/bass")
    print("    Pads  5-8 : Lorenz Y axis → snare/mid")
    print("    Pads  9-12: Lorenz Z axis → hat/high")
    print("    Pads 13-16: Henon X axis  → accent")
    print()
    print("  Velocity encodes attractor intensity — no two bars will sound the same.")


if __name__ == "__main__":
    main()
