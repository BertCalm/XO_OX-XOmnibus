#!/usr/bin/env python3
"""
XPN Evolution Builder — XO_OX Designs
======================================
Generates "evolution packs" — a series of XPM programs that evolve a single
preset through a DNA journey, producing a narrative arc from one timbral state
to another. Use case: load 8 progressively evolved versions of a kit during a
live set (pad 1 = raw, pad 16 = transformed).

Two source modes:
  1. XPM-to-XPM: supply --start and --end (full MPC Drum program files)
  2. DNA target: supply --start and --dna-target "brightness=0.9,warmth=0.2"
     — end state is synthesised by nudging numeric instrument parameters
     proportionally from their start values toward the DNA targets.

Interpolation:
  - ProgramParameters numeric attributes: linearly interpolated at each step.
  - Instrument-level floats (Volume, Pan, Cutoff, Resonance, envelope times):
    interpolated with the selected easing curve.
  - Integer params (TuneCoarse): lerp rounded to nearest int.
  - Boolean / string params: snap at midpoint (step < N/2 → start, else end).
  - Sample assignment: start samples for first half, end samples for second half.

Easing curves (--ease):
  linear         — uniform t each step
  ease-in        — t² (slow start)
  ease-out       — t*(2-t) (slow end)
  ease-in-out    — smoothstep: 3t²-2t³

Output file naming: {basename}_evo_01.xpm … {basename}_evo_N.xpm
Also writes:
  {basename}_evolution_notes.txt  — human-readable arc description
  {basename}_Evolution.xpn        — ZIP bundle for MPC expansion manager

CLI:
    python xpn_evolution_builder.py \\
        --start path/to/start.xpm \\
        --end path/to/end.xpm \\
        --steps 8 \\
        --output-dir ./evolved/ \\
        --ease ease-in-out

    python xpn_evolution_builder.py \\
        --start path/to/start.xpm \\
        --dna-target "brightness=0.9,warmth=0.2,aggression=0.8" \\
        --steps 8 \\
        --output-dir ./evolved/

    python xpn_evolution_builder.py --start a.xpm --end b.xpm --dry-run

XPN golden rules (never violated):
  - KeyTrack = True
  - RootNote = 0
  - Empty layer VelStart = 0
"""

import argparse
import sys
import zipfile
from datetime import date
from pathlib import Path
from xml.etree import ElementTree as ET
from xml.sax.saxutils import escape as xml_escape


# ---------------------------------------------------------------------------
# DNA parameter map — maps DNA axis names to InstrumentState field names.
# Brightness ↔ Cutoff, Warmth ↔ (inverse of Cutoff), Aggression ↔ Resonance,
# Movement ↔ vol_release, Space ↔ vol_decay, Density ↔ polyphony (discrete).
# ---------------------------------------------------------------------------
_DNA_FIELD_MAP = {
    "brightness":  ("cutoff",        0.0, 1.0),
    "warmth":      ("cutoff",        1.0, 0.0),   # inverted
    "aggression":  ("resonance",     0.0, 1.0),
    "movement":    ("vol_release",   0.0, 4.0),
    "space":       ("vol_decay",     0.0, 4.0),
    "density":     ("vol_sustain",   0.0, 1.0),
}

# Key instrument params tracked in the summary table
_TABLE_PARAMS = ["cutoff", "resonance", "vol_attack", "vol_release"]
_TABLE_LABELS = {"cutoff": "Cutoff", "resonance": "Resonance",
                 "vol_attack": "Attack", "vol_release": "Release"}


# =============================================================================
# EASING
# =============================================================================

def apply_ease(t: float, mode: str) -> float:
    """Map normalised t∈[0,1] through the requested easing curve."""
    t = max(0.0, min(1.0, t))
    if mode == "ease-in":
        return t * t
    if mode == "ease-out":
        return t * (2.0 - t)
    if mode == "ease-in-out":
        return t * t * (3.0 - 2.0 * t)
    return t  # linear


# =============================================================================
# XPM PARSING
# =============================================================================

def _text(node: ET.Element, tag: str, default: str = "") -> str:
    child = node.find(tag)
    return child.text.strip() if (child is not None and child.text) else default


def _float(node: ET.Element, tag: str, default: float = 0.0) -> float:
    try:
        return float(_text(node, tag, str(default)))
    except ValueError:
        return default


def _int(node: ET.Element, tag: str, default: int = 0) -> int:
    try:
        return int(_text(node, tag, str(default)))
    except ValueError:
        return default


class InstrumentState:
    """Parsed state of a single drum instrument slot (one MIDI note)."""

    def __init__(self, number: int):
        self.number: int = number
        self.volume: float = 0.707946
        self.pan: float = 0.500000
        self.tune_coarse: int = 0
        self.tune_fine: int = 0
        self.cutoff: float = 1.0
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
        self.layers: list = []
        # Raw ProgramParameters attributes captured for round-trip / DNA use
        self.program_params: dict = {}


def _parse_program_parameters(program_el: ET.Element) -> dict:
    """
    Parse a <ProgramParameters> child (if present) into a flat dict of
    attribute-name → value strings.  All child element text values are
    collected so they can be round-tripped and interpolated where numeric.
    """
    params = {}
    pp_el = program_el.find("ProgramParameters")
    if pp_el is None:
        return params
    # Attributes on the element itself
    params.update(pp_el.attrib)
    # Child elements (text content)
    for child in pp_el:
        if child.text and child.text.strip():
            params[child.tag] = child.text.strip()
    return params


def _parse_layer(layer_el: ET.Element) -> dict:
    return {
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


def parse_xpm(path: Path) -> dict:
    """
    Parse an XPM file into {midi_note: InstrumentState}.
    Also attaches raw ProgramParameters to each InstrumentState.
    Returns empty dict on failure.
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

    prog_params = _parse_program_parameters(program)
    instruments: dict = {}
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
        inst.mute_targets = [_int(inst_el, f"MuteTarget{i+1}", 0) for i in range(4)]
        inst.zone_play = _int(inst_el, "ZonePlay", 1)
        inst.filter_type = _int(inst_el, "FilterType", 2)
        inst.filter_env_amt = _float(inst_el, "FilterEnvAmt", 0.0)
        inst.vol_attack = _float(inst_el, "VolumeAttack", 0.0)
        inst.vol_hold = _float(inst_el, "VolumeHold", 0.0)
        inst.vol_decay = _float(inst_el, "VolumeDecay", 0.0)
        inst.vol_sustain = _float(inst_el, "VolumeSustain", 1.0)
        inst.vol_release = _float(inst_el, "VolumeRelease", 0.0)
        inst.program_params = prog_params  # shared reference — read-only

        layers_el = inst_el.find("Layers")
        if layers_el is not None:
            for layer_el in layers_el.findall("Layer"):
                inst.layers.append(_parse_layer(layer_el))

        instruments[num] = inst

    return instruments


# =============================================================================
# DNA TARGET → SYNTHETIC END MAP
# =============================================================================

def parse_dna_target(raw: str) -> dict:
    """
    Parse a DNA target string like "brightness=0.9,warmth=0.2" into a dict
    {axis_name: float_value}.  Raises ValueError on malformed input.
    """
    result = {}
    for token in raw.split(","):
        token = token.strip()
        if not token:
            continue
        if "=" not in token:
            raise ValueError(f"Expected 'key=value' in DNA target, got: {token!r}")
        key, val = token.split("=", 1)
        key = key.strip().lower()
        if key not in _DNA_FIELD_MAP:
            raise ValueError(
                f"Unknown DNA axis {key!r}. Valid: {sorted(_DNA_FIELD_MAP)}"
            )
        result[key] = float(val.strip())
    return result


def synthesise_end_map(start_map: dict, dna_targets: dict) -> dict:
    """
    Build a synthetic end InstrumentState map by copying start states and
    applying DNA axis targets.  Each axis drives one field toward its target.
    """
    import copy
    end_map: dict = {}
    for note, inst in start_map.items():
        end_inst = copy.deepcopy(inst)
        for axis, target_val in dna_targets.items():
            field, lo, hi = _DNA_FIELD_MAP[axis]
            # Map target_val (0-1 DNA space) to the field's native range
            native_target = lo + target_val * (hi - lo)
            setattr(end_inst, field, native_target)
        end_map[note] = end_inst
    return end_map


# =============================================================================
# INTERPOLATION
# =============================================================================

def _lerp(a: float, b: float, t: float) -> float:
    return a + t * (b - a)


def _lerp_int(a: int, b: int, t: float) -> int:
    return int(round(a + t * (b - a)))


def _lerp_prog_params(start_pp: dict, end_pp: dict, t: float) -> dict:
    """
    Interpolate ProgramParameters dicts.  Numeric values are lerped;
    non-numeric values snap at midpoint.
    """
    all_keys = set(start_pp) | set(end_pp)
    out = {}
    for k in all_keys:
        sv = start_pp.get(k, "")
        ev = end_pp.get(k, sv)
        try:
            out[k] = str(_lerp(float(sv), float(ev), t))
        except (ValueError, TypeError):
            # Boolean / string: snap at midpoint
            out[k] = sv if t < 0.5 else ev
    return out


def interpolate_instrument(
    start_inst: InstrumentState,
    end_inst: InstrumentState,
    t: float,
    n_steps: int,
    step_idx: int,
) -> InstrumentState:
    """
    Produce an interpolated InstrumentState.
    t = 0.0 → pure start, t = 1.0 → pure end (already eased by caller).
    """
    out = InstrumentState(start_inst.number)
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

    # Discrete / boolean: snap at midpoint
    use_start = step_idx < n_steps // 2
    src = start_inst if use_start else end_inst
    out.one_shot = src.one_shot
    out.mono = src.mono
    out.polyphony = src.polyphony
    out.mute_group = src.mute_group
    out.mute_targets = list(src.mute_targets)
    out.zone_play = src.zone_play
    out.filter_type = src.filter_type

    # ProgramParameters interpolation
    out.program_params = _lerp_prog_params(
        start_inst.program_params, end_inst.program_params, t
    )

    # Layers: interpolate continuous values; snap samples at midpoint
    start_layers = start_inst.layers
    end_layers = end_inst.layers
    out_layers = []
    for i, sl in enumerate(start_layers):
        el = end_layers[i] if i < len(end_layers) else sl
        layer = dict(sl)
        layer["volume"] = _lerp(sl["volume"], el["volume"], t)
        layer["pitch"] = _lerp(sl["pitch"], el["pitch"], t)
        layer["tune_coarse"] = _lerp_int(sl["tune_coarse"], el["tune_coarse"], t)
        layer["tune_fine"] = _lerp_int(sl["tune_fine"], el["tune_fine"], t)
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
        # Golden rules
        layer["root_note"] = 0
        if layer["active"] == "True":
            layer["key_track"] = "True"
        out_layers.append(layer)
    out.layers = out_layers
    return out


# =============================================================================
# XPM XML GENERATION
# =============================================================================

def _layer_xml(layer: dict) -> str:
    num = layer["number"]
    active = layer["active"]
    cycle_xml = ""
    if layer.get("cycle_type") and layer["active"] == "True":
        cycle_xml = (
            f"            <CycleType>{xml_escape(layer['cycle_type'])}</CycleType>\n"
            f"            <CycleGroup>{layer['cycle_group']}</CycleGroup>\n"
        )
    return (
        f'          <Layer number="{num}">\n'
        f'            <Active>{active}</Active>\n'
        f'            <Volume>{layer["volume"]:.6f}</Volume>\n'
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
        f"        <SimultTarget{i+1}>0</SimultTarget{i+1}>" for i in range(4)
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


_EMPTY_LAYER = {
    "number": 1, "active": "False", "vel_start": 0, "vel_end": 0,
    "volume": 0.707946, "pan": 0.5, "pitch": 0.0, "tune_coarse": 0,
    "tune_fine": 0, "sample_name": "", "sample_file": "", "file": "",
    "root_note": 0, "key_track": "False", "loop": "False",
    "loop_start": 0, "loop_end": 0, "loop_tune": 0, "mute": "False",
    "sample_start": 0, "sample_end": 0, "slice_index": 128,
    "direction": 0, "offset": 0, "slice_start": 0, "slice_end": 0,
    "slice_loop_start": 0, "slice_loop": 0, "cycle_type": "", "cycle_group": 0,
}


def _empty_instrument(note_num: int) -> InstrumentState:
    inst = InstrumentState(note_num)
    inst.layers = [dict(_EMPTY_LAYER, number=i + 1) for i in range(4)]
    return inst


def generate_step_xpm(
    step_num: int,
    start_map: dict,
    end_map: dict,
    n_steps: int,
    ease: str,
    prog_name_prefix: str,
) -> str:
    """Build XPM XML for one evolution step."""
    raw_t = 0.0 if n_steps == 1 else (step_num - 1) / (n_steps - 1)
    t = apply_ease(raw_t, ease)
    step_idx = step_num - 1

    prog_name = xml_escape(f"{prog_name_prefix} {step_num:02d}")
    instruments_parts = []

    for note_num in range(128):
        in_start = note_num in start_map
        in_end = note_num in end_map

        if in_start and in_end:
            inst = interpolate_instrument(
                start_map[note_num], end_map[note_num], t, n_steps, step_idx
            )
        elif in_start:
            # Fade out — only in start
            import copy
            inst = copy.deepcopy(start_map[note_num])
            inst.volume = _lerp(start_map[note_num].volume, 0.0, t)
        elif in_end:
            # Fade in — only in end
            import copy
            inst = copy.deepcopy(end_map[note_num])
            inst.volume = _lerp(0.0, end_map[note_num].volume, t)
        else:
            inst = _empty_instrument(note_num)

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
# SUMMARY TABLE
# =============================================================================

def print_summary_table(start_map: dict, end_map: dict, n_steps: int, ease: str):
    """
    Print a formatted table showing key parameter values at each step,
    computed from the first shared MIDI note (or first start note).
    """
    shared = sorted(set(start_map.keys()) & set(end_map.keys()))
    if not shared:
        print("  (no shared MIDI notes — no interpolation table available)\n")
        return

    ref = shared[0]
    si = start_map[ref]
    ei = end_map[ref]

    col_w = 9
    header_parts = ["Step", "t_raw", "t_eased"] + [_TABLE_LABELS[p] for p in _TABLE_PARAMS]
    separator = "-" * (6 + 7 + 9 + col_w * len(_TABLE_PARAMS) + 3 * (len(header_parts) - 1))

    print(f"  Ref instrument: MIDI note {ref}")
    print(f"  Ease curve    : {ease}")
    print()

    row_fmt = "  {:<5} {:>6} {:>8}  " + ("  {:>8}" * len(_TABLE_PARAMS))
    print(row_fmt.format(*header_parts))
    print("  " + separator)

    for step_num in range(1, n_steps + 1):
        raw_t = 0.0 if n_steps == 1 else (step_num - 1) / (n_steps - 1)
        t = apply_ease(raw_t, ease)
        vals = [
            f"{_lerp(getattr(si, p), getattr(ei, p), t):.4f}"
            for p in _TABLE_PARAMS
        ]
        print(row_fmt.format(step_num, f"{raw_t:.3f}", f"{t:.3f}", *vals))
    print()


# =============================================================================
# EVOLUTION NOTES
# =============================================================================

def generate_evolution_notes(
    start_label: str,
    end_label: str,
    n_steps: int,
    ease: str,
    start_map: dict,
    end_map: dict,
) -> str:
    shared = sorted(set(start_map.keys()) & set(end_map.keys()))
    start_only = sorted(set(start_map.keys()) - set(end_map.keys()))
    end_only = sorted(set(end_map.keys()) - set(start_map.keys()))

    lines = [
        "XPN Evolution Builder — XO_OX Designs",
        f"Generated : {date.today()}",
        "",
        f"Start     : {start_label}",
        f"End       : {end_label}",
        f"Steps     : {n_steps}",
        f"Ease      : {ease}",
        "",
        "Interpolation rules:",
        "  - All numeric ProgramParameters attributes: linearly interpolated",
        "  - Instrument floats (Volume, Pan, Cutoff, Resonance, envelopes): eased lerp",
        "  - TuneCoarse (semitones): lerp → round to nearest int",
        f"  - Sample assignment: start kit steps 1–{n_steps // 2},"
        f" end kit steps {n_steps // 2 + 1}–{n_steps}",
        "  - Boolean / string values: snap at midpoint",
        "",
        f"Shared instruments (MIDI notes): {shared or 'none'}",
        f"Start-only (fade out)           : {start_only or 'none'}",
        f"End-only   (fade in)            : {end_only or 'none'}",
        "",
    ]

    if n_steps == 1:
        t_raw_values = [0.0]
    else:
        t_raw_values = [i / (n_steps - 1) for i in range(n_steps)]

    for step_idx, raw_t in enumerate(t_raw_values):
        t = apply_ease(raw_t, ease)
        step_num = step_idx + 1
        phase = (
            "START (raw)" if raw_t < 0.25 else
            "TRANSITIONING" if raw_t < 0.75 else
            "END (evolved)"
        )
        use_start = step_idx < n_steps // 2
        sample_src = f"start ({start_label})" if use_start else f"end ({end_label})"

        lines.append(f"Step {step_num:02d}  [t_raw={raw_t:.3f}  t_eased={t:.3f}]  {phase}")
        lines.append(f"  Samples from : {sample_src}")

        if shared:
            ref = shared[0]
            si = start_map[ref]
            ei = end_map[ref]
            param_summary = "  ".join(
                f"{_TABLE_LABELS[p]}={_lerp(getattr(si, p), getattr(ei, p), t):.3f}"
                for p in _TABLE_PARAMS
            )
            lines.append(f"  Ref note {ref:3d} : {param_summary}")
        lines.append("")

    lines += [
        "Live set tip:",
        "  Load Step 01 at set start, swap programs across the set arc.",
        f"  Steps 1–{n_steps // 2} carry start-kit samples (raw/dry).",
        f"  Steps {n_steps // 2 + 1}–{n_steps} carry end-kit samples (evolved).",
    ]
    return "\n".join(lines) + "\n"


# =============================================================================
# XPN PACKAGING
# =============================================================================

def generate_expansion_xml(pack_name: str, pack_id: str, n_steps: int, ease: str) -> str:
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
        f'  <description>Evolution arc — {n_steps} steps, {ease} curve. '
        f'XO_OX Designs.</description>\n'
        f'  <separator>-</separator>\n'
        f'</expansion>\n'
    )


def build_xpn(xpm_files: list, notes_content: str, expansion_xml: str, output_path: Path):
    with zipfile.ZipFile(str(output_path), "w", zipfile.ZIP_DEFLATED) as zf:
        zf.writestr("Expansion.xml", expansion_xml)
        zf.writestr("evolution_notes.txt", notes_content)
        for filename, content in xpm_files:
            zf.writestr(filename, content)
    print(f"  XPN bundle : {output_path}  ({len(xpm_files)} programs)")


# =============================================================================
# MAIN
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Evolution Builder — morphs two XPM kits across N steps.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--start", required=True, metavar="START.xpm",
                        help="Path to the start XPM program file.")
    parser.add_argument("--end", default="", metavar="END.xpm",
                        help="Path to the target XPM program file (omit when using --dna-target).")
    parser.add_argument("--dna-target", default="", metavar="AXES",
                        help='DNA end state, e.g. "brightness=0.9,warmth=0.2,aggression=0.8". '
                             'Axes: brightness, warmth, aggression, movement, space, density.')
    parser.add_argument("--steps", type=int, default=8, metavar="N",
                        help="Number of evolution steps to generate (default: 8).")
    parser.add_argument("--output-dir", default="./evolved", metavar="DIR",
                        help="Output directory (default: ./evolved).")
    parser.add_argument("--ease", default="linear",
                        choices=["linear", "ease-in", "ease-out", "ease-in-out"],
                        help="Interpolation easing curve (default: linear).")
    parser.add_argument("--pack-name", default="", metavar="NAME",
                        help="Override XPN pack name.")
    parser.add_argument("--pack-id", default="", metavar="ID",
                        help="Override XPN pack identifier.")
    parser.add_argument("--dry-run", action="store_true",
                        help="Show interpolation plan without writing files.")
    args = parser.parse_args()

    # Validate mutual exclusivity of --end vs --dna-target
    if not args.end and not args.dna_target:
        parser.error("Provide either --end <file.xpm> or --dna-target <axes>.")
    if args.end and args.dna_target:
        parser.error("--end and --dna-target are mutually exclusive.")

    start_path = Path(args.start)
    n_steps = max(1, args.steps)
    output_dir = Path(args.output_dir)

    if not start_path.exists():
        print(f"[ERROR] --start file not found: {start_path}", file=sys.stderr)
        sys.exit(1)
    if start_path.suffix.lower() != ".xpm":
        print(f"[WARN] --start does not have .xpm extension: {start_path}", file=sys.stderr)

    # Determine basename for output files (start stem)
    basename = start_path.stem

    print("XPN Evolution Builder — XO_OX Designs")
    print(f"  Start     : {start_path}")

    # Parse start
    print("Parsing start XPM...")
    start_map = parse_xpm(start_path)
    if not start_map:
        print("[ERROR] Start XPM yielded no instruments.", file=sys.stderr)
        sys.exit(1)

    # Resolve end map
    if args.end:
        end_path = Path(args.end)
        if not end_path.exists():
            print(f"[ERROR] --end file not found: {end_path}", file=sys.stderr)
            sys.exit(1)
        if end_path.suffix.lower() != ".xpm":
            print(f"[WARN] --end does not have .xpm extension: {end_path}", file=sys.stderr)
        print("Parsing end XPM...")
        end_map = parse_xpm(end_path)
        if not end_map:
            print("[ERROR] End XPM yielded no instruments.", file=sys.stderr)
            sys.exit(1)
        end_label = end_path.stem
        print(f"  End       : {end_path}")
    else:
        try:
            dna_targets = parse_dna_target(args.dna_target)
        except ValueError as exc:
            print(f"[ERROR] --dna-target: {exc}", file=sys.stderr)
            sys.exit(1)
        end_map = synthesise_end_map(start_map, dna_targets)
        end_label = f"DNA({args.dna_target})"
        print(f"  End       : {end_label}")

    shared = set(start_map.keys()) & set(end_map.keys())
    print(f"  Steps     : {n_steps}  |  Ease: {args.ease}")
    print(f"  Shared instruments: {len(shared)}")
    print(f"  Output dir: {output_dir}")
    print()

    if not shared:
        print("[WARN] No shared MIDI notes — only volume fade in/out will occur.", file=sys.stderr)

    # Summary table
    print("Parameter interpolation table:")
    print_summary_table(start_map, end_map, n_steps, args.ease)

    if args.dry_run:
        print("[DRY RUN] Would write:")
        for s in range(1, n_steps + 1):
            print(f"  {basename}_evo_{s:02d}.xpm")
        print(f"  {basename}_evolution_notes.txt")
        print(f"  {basename}_Evolution.xpn")
        return

    # Write files
    output_dir.mkdir(parents=True, exist_ok=True)
    xpm_files = []

    prog_name_prefix = basename[:20] if len(basename) > 20 else basename

    print("Writing evolution steps...")
    for step_num in range(1, n_steps + 1):
        filename = f"{basename}_evo_{step_num:02d}.xpm"
        content = generate_step_xpm(
            step_num, start_map, end_map, n_steps, args.ease, prog_name_prefix
        )
        xpm_files.append((filename, content))
        (output_dir / filename).write_text(content, encoding="utf-8")
        raw_t = 0.0 if n_steps == 1 else (step_num - 1) / (n_steps - 1)
        eased_t = apply_ease(raw_t, args.ease)
        print(f"  {filename}  [t={raw_t:.2f} → eased={eased_t:.2f}]")

    notes = generate_evolution_notes(
        basename, end_label, n_steps, args.ease, start_map, end_map
    )
    notes_path = output_dir / f"{basename}_evolution_notes.txt"
    notes_path.write_text(notes, encoding="utf-8")
    print(f"  {notes_path.name}")

    pack_name = args.pack_name or f"Evolution: {basename}"
    pack_id = args.pack_id or (
        f"com.xo-ox.evolution.{basename.lower().replace(' ', '_')}"
    )
    expansion_xml = generate_expansion_xml(pack_name, pack_id, n_steps, args.ease)
    xpn_path = output_dir / f"{basename}_Evolution.xpn"
    build_xpn(xpm_files, notes, expansion_xml, xpn_path)

    print()
    print(f"Done. {n_steps} steps → {output_dir}/")


if __name__ == "__main__":
    main()
