#!/usr/bin/env python3
"""
XPN Evolution Builder — XO_OX Designs
Interpolates between two XPM programs across N steps, producing a series of
XPN programs that smoothly transition from a Start kit state to an End kit
state. Use case: load 8 progressively evolved versions of a kit during a live
set.

Evolution interpolation strategy:
  - Matching instruments are found by MIDI note number.
  - VelocityToVolume: linear interpolation across steps.
  - PitchSemitones (TuneCoarse): linear interpolation across steps.
  - FilterCutoff (Cutoff): linear interpolation if present in both XPMs.
  - Sample assignment: start sample for steps 1..N//2, end sample for
    steps N//2+1..N.
  - Steps closer to start are "dry/raw"; steps closer to end are "evolved".

XPN golden rules (never violate):
  - KeyTrack = True
  - RootNote = 0
  - Empty layer VelStart = 0

Usage:
    python xpn_evolution_builder.py \\
        --start kit_a.xpm \\
        --end   kit_b.xpm \\
        --steps 8 \\
        --output ./out/

Output:
    ./out/
        Evolution_Step1.xpm  ..  Evolution_StepN.xpm
        evolution_notes.txt
    (These are then zipped into Evolution_Kit.xpn by the tool.)
"""

import argparse
import os
import sys
import zipfile
from datetime import date
from pathlib import Path
from xml.etree import ElementTree as ET
from xml.sax.saxutils import escape as xml_escape


# =============================================================================
# XPM PARSING
# =============================================================================

def _text(node: ET.Element, tag: str, default: str = "") -> str:
    """Return text content of first child with given tag, or default."""
    child = node.find(tag)
    return child.text.strip() if (child is not None and child.text) else default


def _float(node: ET.Element, tag: str, default: float = 0.0) -> float:
    """Return float value of a child element."""
    try:
        return float(_text(node, tag, str(default)))
    except ValueError:
        return default


def _int(node: ET.Element, tag: str, default: int = 0) -> int:
    """Return int value of a child element."""
    try:
        return int(_text(node, tag, str(default)))
    except ValueError:
        return default


class InstrumentState:
    """Parsed state of a single drum instrument (one MIDI note slot)."""

    def __init__(self, number: int):
        self.number: int = number
        self.volume: float = 0.707946           # instrument-level volume
        self.pan: float = 0.500000
        self.tune_coarse: int = 0               # semitones
        self.tune_fine: int = 0
        self.cutoff: float = 1.0                # filter cutoff (0-1)
        self.resonance: float = 0.0
        self.velocity_to_pitch: float = 0.0
        self.velocity_to_filter: float = 0.0
        self.velocity_sensitivity: float = 1.0
        self.one_shot: bool = True
        self.mono: bool = True
        self.polyphony: int = 1
        self.mute_group: int = 0
        self.mute_targets: list = [0, 0, 0, 0]
        self.zone_play: int = 1
        self.filter_type: int = 2
        self.filter_env_amt: float = 0.0
        self.vol_attack: float = 0.0
        self.vol_hold: float = 0.0
        self.vol_decay: float = 0.0
        self.vol_sustain: float = 1.0
        self.vol_release: float = 0.0
        # Layer data: list of dicts with keys:
        #   number, active, vel_start, vel_end, volume, pan, pitch,
        #   tune_coarse, tune_fine, sample_name, sample_file, root_note,
        #   key_track, loop, loop_start, loop_end
        self.layers: list = []


def _parse_layer(layer_el: ET.Element) -> dict:
    """Parse a <Layer> element into a dict."""
    layer = {
        "number":      int(layer_el.get("number", "1")),
        "active":      _text(layer_el, "Active", "False"),
        "vel_start":   _int(layer_el, "VelStart", 0),
        "vel_end":     _int(layer_el, "VelEnd", 127),
        "volume":      _float(layer_el, "Volume", 0.707946),
        "pan":         _float(layer_el, "Pan", 0.5),
        "pitch":       _float(layer_el, "Pitch", 0.0),
        "tune_coarse": _int(layer_el, "TuneCoarse", 0),
        "tune_fine":   _int(layer_el, "TuneFine", 0),
        "sample_name": _text(layer_el, "SampleName", ""),
        "sample_file": _text(layer_el, "SampleFile", ""),
        "file":        _text(layer_el, "File", ""),
        "root_note":   _int(layer_el, "RootNote", 0),
        "key_track":   _text(layer_el, "KeyTrack", "False"),
        "loop":        _text(layer_el, "Loop", "False"),
        "loop_start":  _int(layer_el, "LoopStart", 0),
        "loop_end":    _int(layer_el, "LoopEnd", 0),
        "loop_tune":   _int(layer_el, "LoopTune", 0),
        "mute":        _text(layer_el, "Mute", "False"),
        "sample_start": _int(layer_el, "SampleStart", 0),
        "sample_end":   _int(layer_el, "SampleEnd", 0),
        "slice_index":  _int(layer_el, "SliceIndex", 128),
        "direction":    _int(layer_el, "Direction", 0),
        "offset":       _int(layer_el, "Offset", 0),
        "slice_start":  _int(layer_el, "SliceStart", 0),
        "slice_end":    _int(layer_el, "SliceEnd", 0),
        "slice_loop_start": _int(layer_el, "SliceLoopStart", 0),
        "slice_loop":   _int(layer_el, "SliceLoop", 0),
        "cycle_type":   _text(layer_el, "CycleType", ""),
        "cycle_group":  _int(layer_el, "CycleGroup", 0),
    }
    return layer


def parse_xpm(path: Path) -> dict:
    """
    Parse an XPM file into a dict: {midi_note: InstrumentState}.
    Returns empty dict on parse failure with a warning printed to stderr.
    """
    try:
        tree = ET.parse(str(path))
    except ET.ParseError as exc:
        print(f"[ERROR] Cannot parse XPM {path}: {exc}", file=sys.stderr)
        return {}

    root = tree.getroot()
    program = root.find("Program")
    if program is None:
        print(f"[ERROR] No <Program> element in {path}", file=sys.stderr)
        return {}

    instruments = {}
    instruments_el = program.find("Instruments")
    if instruments_el is None:
        return instruments

    for inst_el in instruments_el.findall("Instrument"):
        num = int(inst_el.get("number", "0"))
        inst = InstrumentState(num)
        inst.volume = _float(inst_el, "Volume", 0.707946)
        inst.pan = _float(inst_el, "Pan", 0.5)
        inst.tune_coarse = _int(inst_el, "TuneCoarse", 0)
        inst.tune_fine = _int(inst_el, "TuneFine", 0)
        inst.cutoff = _float(inst_el, "Cutoff", 1.0)
        inst.resonance = _float(inst_el, "Resonance", 0.0)
        inst.velocity_to_pitch = _float(inst_el, "VelocityToPitch", 0.0)
        inst.velocity_to_filter = _float(inst_el, "VelocityToFilter", 0.0)
        inst.velocity_sensitivity = _float(inst_el, "VelocitySensitivity", 1.0)
        inst.one_shot = _text(inst_el, "OneShot", "True") == "True"
        inst.mono = _text(inst_el, "Mono", "True") == "True"
        inst.polyphony = _int(inst_el, "Polyphony", 1)
        inst.mute_group = _int(inst_el, "MuteGroup", 0)
        inst.mute_targets = [
            _int(inst_el, f"MuteTarget{i+1}", 0) for i in range(4)
        ]
        inst.zone_play = _int(inst_el, "ZonePlay", 1)
        inst.filter_type = _int(inst_el, "FilterType", 2)
        inst.filter_env_amt = _float(inst_el, "FilterEnvAmt", 0.0)
        inst.vol_attack = _float(inst_el, "VolumeAttack", 0.0)
        inst.vol_hold = _float(inst_el, "VolumeHold", 0.0)
        inst.vol_decay = _float(inst_el, "VolumeDecay", 0.0)
        inst.vol_sustain = _float(inst_el, "VolumeSustain", 1.0)
        inst.vol_release = _float(inst_el, "VolumeRelease", 0.0)

        layers_el = inst_el.find("Layers")
        if layers_el is not None:
            for layer_el in layers_el.findall("Layer"):
                inst.layers.append(_parse_layer(layer_el))

        instruments[num] = inst

    return instruments


# =============================================================================
# INTERPOLATION
# =============================================================================

def _lerp(a: float, b: float, t: float) -> float:
    """Linear interpolation: a + t*(b-a), t in [0,1]."""
    return a + t * (b - a)


def _lerp_int(a: int, b: int, t: float) -> int:
    """Linear interpolation rounded to nearest int."""
    return int(round(a + t * (b - a)))


def interpolate_instrument(
    start_inst: InstrumentState,
    end_inst: InstrumentState,
    t: float,
    n_steps: int,
    step_idx: int,          # 0-based
) -> InstrumentState:
    """
    Produce an interpolated InstrumentState between start and end.
    t = 0.0 → pure start, t = 1.0 → pure end.
    Sample selection: start samples for first half, end samples for second half.
    """
    out = InstrumentState(start_inst.number)

    # Linear interpolation of continuous parameters
    out.volume = _lerp(start_inst.volume, end_inst.volume, t)
    out.pan = _lerp(start_inst.pan, end_inst.pan, t)
    out.tune_coarse = _lerp_int(start_inst.tune_coarse, end_inst.tune_coarse, t)
    out.tune_fine = _lerp_int(start_inst.tune_fine, end_inst.tune_fine, t)
    out.cutoff = max(0.0, min(1.0, _lerp(start_inst.cutoff, end_inst.cutoff, t)))
    out.resonance = max(0.0, min(1.0, _lerp(start_inst.resonance, end_inst.resonance, t)))
    out.velocity_to_pitch = _lerp(start_inst.velocity_to_pitch, end_inst.velocity_to_pitch, t)
    out.velocity_to_filter = _lerp(start_inst.velocity_to_filter, end_inst.velocity_to_filter, t)
    out.velocity_sensitivity = max(0.0, min(1.0,
        _lerp(start_inst.velocity_sensitivity, end_inst.velocity_sensitivity, t)))
    out.filter_env_amt = _lerp(start_inst.filter_env_amt, end_inst.filter_env_amt, t)
    out.vol_attack = max(0.0, _lerp(start_inst.vol_attack, end_inst.vol_attack, t))
    out.vol_hold = max(0.0, _lerp(start_inst.vol_hold, end_inst.vol_hold, t))
    out.vol_decay = max(0.0, _lerp(start_inst.vol_decay, end_inst.vol_decay, t))
    out.vol_sustain = max(0.0, min(1.0, _lerp(start_inst.vol_sustain, end_inst.vol_sustain, t)))
    out.vol_release = max(0.0, _lerp(start_inst.vol_release, end_inst.vol_release, t))

    # Boolean / discrete: take from whichever source dominates (start for first half)
    use_start = (step_idx < n_steps // 2)
    src = start_inst if use_start else end_inst
    out.one_shot = src.one_shot
    out.mono = src.mono
    out.polyphony = src.polyphony
    out.mute_group = src.mute_group
    out.mute_targets = list(src.mute_targets)
    out.zone_play = src.zone_play
    out.filter_type = src.filter_type

    # Layers: interpolate layer volumes; swap samples at midpoint
    start_layers = start_inst.layers
    end_layers = end_inst.layers

    # Use start layers as template structure; interpolate volumes
    out_layers = []
    for i, sl in enumerate(start_layers):
        el = end_layers[i] if i < len(end_layers) else sl
        layer = dict(sl)  # shallow copy

        # Volume: linear interpolation per layer
        layer["volume"] = _lerp(sl["volume"], el["volume"], t)

        # Pitch (fine cents in layer): interpolate
        layer["pitch"] = _lerp(sl["pitch"], el["pitch"], t)
        layer["tune_coarse"] = _lerp_int(sl["tune_coarse"], el["tune_coarse"], t)
        layer["tune_fine"] = _lerp_int(sl["tune_fine"], el["tune_fine"], t)

        # Sample assignment: first half uses start, second half uses end
        if use_start:
            layer["sample_name"] = sl["sample_name"]
            layer["sample_file"] = sl["sample_file"]
            layer["file"] = sl["file"]
            layer["active"] = sl["active"]
        else:
            layer["sample_name"] = el["sample_name"]
            layer["sample_file"] = el["sample_file"]
            layer["file"] = el["file"]
            layer["active"] = el["active"]

        # Golden rules: RootNote=0, KeyTrack=True for active layers
        layer["root_note"] = 0
        if layer["active"] == "True":
            layer["key_track"] = "True"

        out_layers.append(layer)

    out.layers = out_layers
    return out


# =============================================================================
# XPM GENERATION
# =============================================================================

def _layer_xml(layer: dict) -> str:
    num = layer["number"]
    active = layer["active"]
    vol = layer["volume"]
    cycle_xml = ""
    if layer.get("cycle_type") and layer["active"] == "True":
        cycle_xml = (
            f"            <CycleType>{xml_escape(layer['cycle_type'])}</CycleType>\n"
            f"            <CycleGroup>{layer['cycle_group']}</CycleGroup>\n"
        )
    return (
        f'          <Layer number="{num}">\n'
        f'            <Active>{active}</Active>\n'
        f'            <Volume>{vol:.6f}</Volume>\n'
        f'            <Pan>{layer["pan"]:.6f}</Pan>\n'
        f'            <Pitch>{layer["pitch"]:.6f}</Pitch>\n'
        f'            <TuneCoarse>{layer["tune_coarse"]}</TuneCoarse>\n'
        f'            <TuneFine>{layer["tune_fine"]}</TuneFine>\n'
        f'            <VelStart>{layer["vel_start"]}</VelStart>\n'
        f'            <VelEnd>{layer["vel_end"]}</VelEnd>\n'
        f'            <SampleStart>{layer["sample_start"]}</SampleStart>\n'
        f'            <SampleEnd>{layer["sample_end"]}</SampleEnd>\n'
        f'            <Loop>{layer["loop"]}</Loop>\n'
        f'            <LoopStart>{layer["loop_start"]}</LoopStart>\n'
        f'            <LoopEnd>{layer["loop_end"]}</LoopEnd>\n'
        f'            <LoopTune>{layer["loop_tune"]}</LoopTune>\n'
        f'            <Mute>{layer["mute"]}</Mute>\n'
        f'            <RootNote>0</RootNote>\n'
        f'            <KeyTrack>{layer["key_track"]}</KeyTrack>\n'
        f'            <SampleName>{xml_escape(layer["sample_name"])}</SampleName>\n'
        f'            <SampleFile>{xml_escape(layer["sample_file"])}</SampleFile>\n'
        f'            <File>{xml_escape(layer["file"])}</File>\n'
        f'{cycle_xml}'
        f'            <SliceIndex>{layer["slice_index"]}</SliceIndex>\n'
        f'            <Direction>{layer["direction"]}</Direction>\n'
        f'            <Offset>{layer["offset"]}</Offset>\n'
        f'            <SliceStart>{layer["slice_start"]}</SliceStart>\n'
        f'            <SliceEnd>{layer["slice_end"]}</SliceEnd>\n'
        f'            <SliceLoopStart>{layer["slice_loop_start"]}</SliceLoopStart>\n'
        f'            <SliceLoop>{layer["slice_loop"]}</SliceLoop>\n'
        f'          </Layer>'
    )


def _instrument_xml(inst: InstrumentState) -> str:
    mono_str = "True" if inst.mono else "False"
    oneshot_str = "True" if inst.one_shot else "False"
    mute_xml = "\n".join(
        f"        <MuteTarget{i+1}>{t}</MuteTarget{i+1}>"
        for i, t in enumerate(inst.mute_targets)
    )
    simult_xml = "\n".join(
        f"        <SimultTarget{i+1}>0</SimultTarget{i+1}>"
        for i in range(4)
    )
    layers_xml = "\n".join(_layer_xml(l) for l in inst.layers)

    return (
        f'      <Instrument number="{inst.number}">\n'
        f'        <AudioRoute>\n'
        f'          <AudioRoute>0</AudioRoute>\n'
        f'          <AudioRouteSubIndex>0</AudioRouteSubIndex>\n'
        f'          <InsertsEnabled>False</InsertsEnabled>\n'
        f'        </AudioRoute>\n'
        f'        <Send1>0.000000</Send1>\n'
        f'        <Send2>0.000000</Send2>\n'
        f'        <Send3>0.000000</Send3>\n'
        f'        <Send4>0.000000</Send4>\n'
        f'        <Volume>{inst.volume:.6f}</Volume>\n'
        f'        <Mute>False</Mute>\n'
        f'        <Pan>{inst.pan:.6f}</Pan>\n'
        f'        <TuneCoarse>{inst.tune_coarse}</TuneCoarse>\n'
        f'        <TuneFine>{inst.tune_fine}</TuneFine>\n'
        f'        <Mono>{mono_str}</Mono>\n'
        f'        <Polyphony>{inst.polyphony}</Polyphony>\n'
        f'        <FilterKeytrack>0.000000</FilterKeytrack>\n'
        f'        <LowNote>0</LowNote>\n'
        f'        <HighNote>127</HighNote>\n'
        f'        <IgnoreBaseNote>False</IgnoreBaseNote>\n'
        f'        <ZonePlay>{inst.zone_play}</ZonePlay>\n'
        f'        <MuteGroup>{inst.mute_group}</MuteGroup>\n'
        f'{mute_xml}\n'
        f'{simult_xml}\n'
        f'        <LfoPitch>0.000000</LfoPitch>\n'
        f'        <LfoCutoff>0.000000</LfoCutoff>\n'
        f'        <LfoVolume>0.000000</LfoVolume>\n'
        f'        <LfoPan>0.000000</LfoPan>\n'
        f'        <OneShot>{oneshot_str}</OneShot>\n'
        f'        <FilterType>{inst.filter_type}</FilterType>\n'
        f'        <Cutoff>{inst.cutoff:.6f}</Cutoff>\n'
        f'        <Resonance>{inst.resonance:.6f}</Resonance>\n'
        f'        <FilterEnvAmt>{inst.filter_env_amt:.6f}</FilterEnvAmt>\n'
        f'        <AfterTouchToFilter>0.000000</AfterTouchToFilter>\n'
        f'        <VelocityToStart>0.000000</VelocityToStart>\n'
        f'        <VelocityToFilterAttack>0.000000</VelocityToFilterAttack>\n'
        f'        <VelocityToFilter>{inst.velocity_to_filter:.6f}</VelocityToFilter>\n'
        f'        <VelocityToFilterEnvelope>0.000000</VelocityToFilterEnvelope>\n'
        f'        <FilterAttack>0.000000</FilterAttack>\n'
        f'        <FilterDecay>0.000000</FilterDecay>\n'
        f'        <FilterSustain>1.000000</FilterSustain>\n'
        f'        <FilterRelease>0.000000</FilterRelease>\n'
        f'        <FilterHold>0.000000</FilterHold>\n'
        f'        <FilterDecayType>True</FilterDecayType>\n'
        f'        <FilterADEnvelope>True</FilterADEnvelope>\n'
        f'        <VolumeHold>{inst.vol_hold:.6f}</VolumeHold>\n'
        f'        <VolumeDecayType>True</VolumeDecayType>\n'
        f'        <VolumeADEnvelope>True</VolumeADEnvelope>\n'
        f'        <VolumeAttack>{inst.vol_attack:.6f}</VolumeAttack>\n'
        f'        <VolumeDecay>{inst.vol_decay:.6f}</VolumeDecay>\n'
        f'        <VolumeSustain>{inst.vol_sustain:.6f}</VolumeSustain>\n'
        f'        <VolumeRelease>{inst.vol_release:.6f}</VolumeRelease>\n'
        f'        <VelocityToPitch>{inst.velocity_to_pitch:.6f}</VelocityToPitch>\n'
        f'        <VelocityToVolumeAttack>0.000000</VelocityToVolumeAttack>\n'
        f'        <VelocitySensitivity>{inst.velocity_sensitivity:.6f}</VelocitySensitivity>\n'
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


def generate_step_xpm(
    step_num: int,
    all_notes: list,
    start_map: dict,
    end_map: dict,
    n_steps: int,
) -> str:
    """
    Generate XPM XML for a single evolution step.
    step_num: 1-based step number.
    all_notes: sorted list of all MIDI note numbers present in either XPM.
    """
    # t = 0.0 for step 1, 1.0 for step N
    if n_steps == 1:
        t = 0.0
    else:
        t = (step_num - 1) / (n_steps - 1)

    step_idx = step_num - 1  # 0-based for midpoint sample-swap logic

    prog_name = xml_escape(f"Evolution Step {step_num:02d}")

    instruments_parts = []
    # Emit all 128 slots to satisfy MPC XPM spec
    for note_num in range(128):
        if note_num in start_map and note_num in end_map:
            inst = interpolate_instrument(
                start_map[note_num],
                end_map[note_num],
                t,
                n_steps,
                step_idx,
            )
        elif note_num in start_map:
            # Fade out instruments only in start kit as we evolve
            inst = start_map[note_num]
            # Gradually reduce volume toward end
            faded = InstrumentState(note_num)
            faded.__dict__.update(inst.__dict__)
            faded.layers = [dict(l) for l in inst.layers]
            fade_vol = _lerp(inst.volume, 0.0, t)
            faded.volume = fade_vol
            inst = faded
        elif note_num in end_map:
            # Fade in instruments only in end kit as we evolve
            inst = end_map[note_num]
            faded = InstrumentState(note_num)
            faded.__dict__.update(inst.__dict__)
            faded.layers = [dict(l) for l in inst.layers]
            fade_vol = _lerp(0.0, inst.volume, t)
            faded.volume = fade_vol
            inst = faded
        else:
            # Empty slot — minimal instrument block
            inst = InstrumentState(note_num)
            inst.layers = [
                {
                    "number": i + 1, "active": "False", "vel_start": 0,
                    "vel_end": 0, "volume": 0.707946, "pan": 0.5,
                    "pitch": 0.0, "tune_coarse": 0, "tune_fine": 0,
                    "sample_name": "", "sample_file": "", "file": "",
                    "root_note": 0, "key_track": "False", "loop": "False",
                    "loop_start": 0, "loop_end": 0, "loop_tune": 0,
                    "mute": "False", "sample_start": 0, "sample_end": 0,
                    "slice_index": 128, "direction": 0, "offset": 0,
                    "slice_start": 0, "slice_end": 0, "slice_loop_start": 0,
                    "slice_loop": 0, "cycle_type": "", "cycle_group": 0,
                }
                for i in range(4)
            ]

        instruments_parts.append(_instrument_xml(inst))

    instruments_xml = "\n".join(instruments_parts)

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
        '    <Instruments>\n'
        f'{instruments_xml}\n'
        '    </Instruments>\n'
        '  </Program>\n'
        '</MPCVObject>\n'
    )


# =============================================================================
# EVOLUTION NOTES
# =============================================================================

def generate_evolution_notes(
    start_path: Path,
    end_path: Path,
    n_steps: int,
    start_map: dict,
    end_map: dict,
) -> str:
    """Generate a human-readable evolution_notes.txt describing each step."""
    shared_notes = sorted(set(start_map.keys()) & set(end_map.keys()))
    start_only = sorted(set(start_map.keys()) - set(end_map.keys()))
    end_only = sorted(set(end_map.keys()) - set(start_map.keys()))

    lines = [
        "XPN Evolution Builder — XO_OX Designs",
        f"Generated: {date.today()}",
        "",
        f"Start kit : {start_path.name}",
        f"End kit   : {end_path.name}",
        f"Steps     : {n_steps}",
        "",
        "Interpolation rules:",
        "  - VelocityToVolume   : linear interpolation",
        "  - PitchSemitones     : linear interpolation (rounded to nearest semitone)",
        "  - FilterCutoff       : linear interpolation (0.0–1.0 range)",
        f"  - Sample assignment  : start samples for steps 1–{n_steps // 2},"
        f" end samples for steps {n_steps // 2 + 1}–{n_steps}",
        "",
        f"Shared instruments (MIDI notes): {shared_notes if shared_notes else 'none'}",
        f"Start-only instruments         : {start_only if start_only else 'none'} (fade out)",
        f"End-only instruments           : {end_only if end_only else 'none'} (fade in)",
        "",
    ]

    if n_steps == 1:
        t_values = [0.0]
    else:
        t_values = [(i) / (n_steps - 1) for i in range(n_steps)]

    for step_idx, t in enumerate(t_values):
        step_num = step_idx + 1
        phase = "START (dry/raw)" if t < 0.25 else (
            "TRANSITIONING" if t < 0.75 else "END (evolved/processed)"
        )
        use_start_samples = (step_idx < n_steps // 2)
        sample_src = f"start kit ({start_path.stem})" if use_start_samples else f"end kit ({end_path.stem})"

        lines.append(f"Step {step_num:02d}  [t={t:.3f}]  {phase}")
        lines.append(f"  Samples from : {sample_src}")

        if shared_notes:
            # Show parameter values at this step for the first active note
            ref_note = shared_notes[0]
            si = start_map[ref_note]
            ei = end_map[ref_note]
            vol_val = _lerp(si.volume, ei.volume, t)
            pitch_val = _lerp_int(si.tune_coarse, ei.tune_coarse, t)
            cutoff_val = _lerp(si.cutoff, ei.cutoff, t)
            lines.append(
                f"  Ref note {ref_note}: Volume={vol_val:.3f}  "
                f"Pitch={pitch_val:+d}st  FilterCutoff={cutoff_val:.3f}"
            )
        lines.append("")

    lines.append("Live set tip:")
    lines.append("  Load Step 01 at set start, swap programs across the set arc.")
    lines.append("  Steps 1-4 are raw/dry, Steps 5-8 are processed/evolved.")

    return "\n".join(lines) + "\n"


# =============================================================================
# XPN (ZIP) PACKAGING
# =============================================================================

def generate_expansion_xml(pack_name: str, pack_id: str, n_steps: int) -> str:
    today = str(date.today())
    return (
        '<?xml version="1.0" encoding="UTF-8"?>\n\n'
        f'<expansion version="2.0.0.0" buildVersion="2.10.0.0">\n'
        f'  <local/>\n'
        f'  <identifier>{xml_escape(pack_id)}</identifier>\n'
        f'  <title>{xml_escape(pack_name)}</title>\n'
        f'  <manufacturer>XO_OX Designs</manufacturer>\n'
        f'  <version>1.0.0</version>\n'
        f'  <type>drum</type>\n'
        f'  <priority>50</priority>\n'
        f'  <img>artwork.png</img>\n'
        f'  <description>Kit evolution arc — {n_steps} steps from raw to processed. '
        f'XO_OX Designs.</description>\n'
        f'  <separator>-</separator>\n'
        f'</expansion>\n'
    )


def build_xpn(
    xpm_files: list,      # list of (filename, content_str)
    notes_content: str,
    expansion_xml: str,
    output_path: Path,
):
    """Write XPM files + notes + expansion manifest into an XPN ZIP."""
    with zipfile.ZipFile(str(output_path), "w", zipfile.ZIP_DEFLATED) as zf:
        zf.writestr("Expansion.xml", expansion_xml)
        zf.writestr("evolution_notes.txt", notes_content)
        for filename, content in xpm_files:
            zf.writestr(filename, content)
    print(f"  XPN: {output_path}  ({len(xpm_files)} programs)")


# =============================================================================
# MAIN
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Evolution Builder — interpolate between two XPM kits across N steps.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--start", required=True, metavar="START.xpm",
                        help="Path to the start/initial XPM program file.")
    parser.add_argument("--end", required=True, metavar="END.xpm",
                        help="Path to the target/evolved XPM program file.")
    parser.add_argument("--steps", type=int, default=8, metavar="N",
                        help="Number of evolution steps to generate (default: 8).")
    parser.add_argument("--output", default="./out", metavar="DIR",
                        help="Output directory (default: ./out).")
    parser.add_argument("--pack-name", default="", metavar="NAME",
                        help="Override the XPN pack name. Default: 'Evolution: <start> → <end>'.")
    parser.add_argument("--pack-id", default="", metavar="ID",
                        help="Override the XPN pack identifier. Default: auto-generated.")
    parser.add_argument("--dry-run", action="store_true",
                        help="Parse and report without writing any files.")
    args = parser.parse_args()

    start_path = Path(args.start)
    end_path = Path(args.end)
    n_steps = max(1, args.steps)
    output_dir = Path(args.output)

    # Validate inputs
    for p, label in [(start_path, "--start"), (end_path, "--end")]:
        if not p.exists():
            print(f"[ERROR] {label} file not found: {p}", file=sys.stderr)
            sys.exit(1)
        if p.suffix.lower() != ".xpm":
            print(f"[WARN] {label} file does not have .xpm extension: {p}", file=sys.stderr)

    if n_steps < 1:
        print("[ERROR] --steps must be >= 1", file=sys.stderr)
        sys.exit(1)

    print(f"XPN Evolution Builder — XO_OX Designs")
    print(f"  Start : {start_path}")
    print(f"  End   : {end_path}")
    print(f"  Steps : {n_steps}")
    print(f"  Output: {output_dir}")
    print()

    # Parse both XPMs
    print("Parsing XPMs...")
    start_map = parse_xpm(start_path)
    end_map = parse_xpm(end_path)

    if not start_map:
        print("[ERROR] Start XPM yielded no instruments — check file.", file=sys.stderr)
        sys.exit(1)
    if not end_map:
        print("[ERROR] End XPM yielded no instruments — check file.", file=sys.stderr)
        sys.exit(1)

    shared = set(start_map.keys()) & set(end_map.keys())
    print(f"  Start instruments: {len(start_map)}")
    print(f"  End instruments  : {len(end_map)}")
    print(f"  Shared (matched) : {len(shared)}")
    print()

    if not shared:
        print("[WARN] No shared MIDI notes — no interpolation will occur. "
              "Start and end kits use different note assignments.", file=sys.stderr)

    all_notes = sorted(set(start_map.keys()) | set(end_map.keys()))

    if args.dry_run:
        print("[DRY RUN] Would generate:")
        for s in range(1, n_steps + 1):
            print(f"  Evolution_Step{s:02d}.xpm")
        print("  evolution_notes.txt")
        print("  Evolution_Kit.xpn")
        return

    # Create output directory
    output_dir.mkdir(parents=True, exist_ok=True)

    # Generate all step XPMs
    xpm_files = []
    print("Generating evolution steps...")
    for step_num in range(1, n_steps + 1):
        filename = f"Evolution_Step{step_num:02d}.xpm"
        content = generate_step_xpm(step_num, all_notes, start_map, end_map, n_steps)
        xpm_files.append((filename, content))

        # Also write individual .xpm to output dir for easy MPC drag-and-drop
        xpm_path = output_dir / filename
        xpm_path.write_text(content, encoding="utf-8")
        t = 0.0 if n_steps == 1 else (step_num - 1) / (n_steps - 1)
        print(f"  {filename}  [t={t:.2f}]")

    # Generate evolution notes
    notes_content = generate_evolution_notes(start_path, end_path, n_steps, start_map, end_map)
    notes_path = output_dir / "evolution_notes.txt"
    notes_path.write_text(notes_content, encoding="utf-8")
    print(f"  evolution_notes.txt")

    # Pack name / ID
    pack_name = args.pack_name or f"Evolution: {start_path.stem} → {end_path.stem}"
    pack_id = args.pack_id or (
        f"com.xo-ox.evolution.{start_path.stem.lower().replace(' ', '_')}"
        f"_to_{end_path.stem.lower().replace(' ', '_')}"
    )

    expansion_xml = generate_expansion_xml(pack_name, pack_id, n_steps)

    # Build XPN ZIP
    xpn_path = output_dir / "Evolution_Kit.xpn"
    build_xpn(xpm_files, notes_content, expansion_xml, xpn_path)

    print()
    print(f"Done. {n_steps} evolution steps written to {output_dir}/")
    print(f"XPN pack: {xpn_path}")


if __name__ == "__main__":
    main()
