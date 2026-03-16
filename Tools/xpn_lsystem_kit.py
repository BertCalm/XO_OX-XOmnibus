#!/usr/bin/env python3
"""
XPN L-System Rhythm Kit Generator — XO_OX Designs
Hex + Rex build: fractal string rewriting produces drum patterns.

L-System (Lindenmayer System) string rewriting rules produce fractal trigger
sequences. Each expansion generates a different rhythmic personality.

Symbol interpretation (turtle-style):
  F or f  = trigger pad (hit)
  +       = pitch up one semitone
  -       = pitch down one semitone
  [       = push state (accent / louder)
  ]       = pop state (return to normal)
  X,Y,Z   = ignored structural growth symbols

Hardcoded L-systems:
  koch       F -> F+F-F-F+F          (Koch snowflake rhythm)
  sierpinski A -> B-A-B / B -> A+B+A (Sierpinski triangle)
  dragon     FX / X -> X+YF+ etc.    (Dragon curve)
  plant      X -> F+[[X]-X] etc.     (Plant branching)
  algae      A -> AB / B -> A         (Fibonacci / Lindenmayer original)
  penrose    [N]++[N]... / M/N/O/P    (Penrose tiling)

Multi-voice polyrhythm mode:
  Voice 1 (Koch)       -> Pads 1-4  (kick zone)
  Voice 2 (Sierpinski) -> Pads 5-8  (snare zone)
  Voice 3 (Dragon)     -> Pads 9-12 (hat zone)
  Voice 4 (Plant)      -> Pads 13-16 (accent zone)

CLI:
  python xpn_lsystem_kit.py --system koch --iterations 3 --output ./kits/
  python xpn_lsystem_kit.py --system all --output ./kits/
  python xpn_lsystem_kit.py --axiom "F" --rules "F:F+F-F" --name custom --output ./kits/
  python xpn_lsystem_kit.py --polyrhythm --output ./kits/
  python xpn_lsystem_kit.py --visualize koch --iterations 4
"""

import argparse
import json
import sys
from datetime import date
from pathlib import Path
from typing import Optional
from xml.sax.saxutils import escape as xml_escape


# =============================================================================
# L-SYSTEM DEFINITIONS
# =============================================================================

L_SYSTEMS = {
    "koch": {
        "axiom":    "F",
        "rules":    {"F": "F+F-F-F+F"},
        "triggers": "Ff",
        "angle":    90,
        "description": "Koch snowflake curve — dense, fractal subdivision",
    },
    "sierpinski": {
        "axiom":    "A",
        "rules":    {"A": "B-A-B", "B": "A+B+A"},
        "triggers": "AB",   # A and B are terminal hits in this system
        "angle":    60,
        "description": "Sierpinski triangle — alternating 3-beat subdivision",
    },
    "dragon": {
        "axiom":    "FX",
        "rules":    {"X": "X+YF+", "Y": "-FX-Y"},
        "triggers": "Ff",
        "angle":    90,
        "description": "Dragon curve — asymmetric folding pattern",
    },
    "plant": {
        "axiom":    "X",
        "rules":    {"X": "F+[[X]-X]-F[-FX]+X", "F": "FF"},
        "triggers": "Ff",
        "angle":    25,
        "description": "Lindenmayer plant — branching with accent pockets",
    },
    "algae": {
        "axiom":    "A",
        "rules":    {"A": "AB", "B": "A"},
        "triggers": "AB",   # both symbols = hits; Fibonacci density pattern
        "angle":    0,
        "description": "Fibonacci algae — grows at golden-ratio density",
    },
    "penrose": {
        "axiom":    "[N]++[N]++[N]++[N]++[N]",
        "rules":    {
            "M": "OA++PA----NA[-OA----MA]++",
            "N": "+OA--PA[---MA--NA]+",
            "O": "-MA++NA[+++OA++PA]-",
            "P": "--OA++++MA[+PA++++NA]--NA",
        },
        "triggers": "MNOP",  # all production symbols are terminal hits
        "angle":    36,
        "description": "Penrose tiling — 5-fold quasi-crystalline rhythm",
    },
}

# Voice zone assignments for polyrhythm mode
POLY_ZONES = [
    ("koch",       "kick",  [36, 37, 38, 39],  "Kick zone   (Koch)"),
    ("sierpinski", "snare", [40, 41, 42, 43],  "Snare zone  (Sierpinski)"),
    ("dragon",     "hat",   [44, 45, 46, 47],  "Hat zone    (Dragon)"),
    ("plant",      "accent",[48, 49, 50, 51],  "Accent zone (Plant)"),
]

# Drum voice defaults (shared with braille_rhythm_kit conventions)
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


# =============================================================================
# L-SYSTEM ENGINE
# =============================================================================

def expand(axiom: str, rules: dict, iterations: int, max_len: int = 4096) -> str:
    """Expand an L-system string for the given number of iterations."""
    s = axiom
    for _ in range(iterations):
        s = "".join(rules.get(c, c) for c in s)
        if len(s) > max_len:
            s = s[:max_len]
            break
    return s


def extract_triggers(lstring: str, max_steps: int = 256,
                     trigger_chars: str = "Ff") -> list[dict]:
    """
    Walk the L-system string and extract trigger events.
    Returns list of dicts: {step, accented, pitch_offset}

    State machine:
      - trigger_chars  -> trigger at current step, advance step counter
      - [              -> push accent state (accented=True)
      - ]              -> pop accent state
      - +              -> increment pending pitch offset
      - -              -> decrement pending pitch offset
      - everything else -> skip (structural/growth symbols)

    trigger_chars defaults to "Ff" for standard L-systems;
    Sierpinski/Algae pass "AB" since those are their terminal symbols.
    """
    trigger_set = set(trigger_chars)
    triggers = []
    step = 0
    accent_stack = [False]   # stack of accent booleans
    pitch_offset = 0

    for ch in lstring:
        if step >= max_steps:
            break
        if ch in trigger_set:
            accented = accent_stack[-1]
            triggers.append({
                "step":         step % 16,   # wrap into 16-step bar
                "raw_step":     step,
                "accented":     accented,
                "pitch_offset": pitch_offset,
            })
            step += 1
            pitch_offset = 0           # pitch modifier consumed by this hit
        elif ch == "[":
            accent_stack.append(True)
        elif ch == "]":
            if len(accent_stack) > 1:
                accent_stack.pop()
        elif ch == "+":
            pitch_offset += 1
        elif ch == "-":
            pitch_offset -= 1
        # X, Y, Z, A, B, M, N, O, P — structural only, no step advance

    return triggers


def triggers_to_bar(triggers: list[dict]) -> list[dict]:
    """
    Collapse raw trigger list into a 16-step bar.
    Each step: {hit: bool, accented: bool, pitch_offset: int, velocity: int}
    Multiple triggers on the same step: take the loudest (accented wins).
    """
    bar = [{"hit": False, "accented": False, "pitch_offset": 0, "velocity": 0}
           for _ in range(16)]

    for t in triggers:
        s = t["step"]  # already mod 16
        if t["accented"] or not bar[s]["hit"]:
            bar[s]["hit"]          = True
            bar[s]["accented"]     = t["accented"]
            bar[s]["pitch_offset"] = t["pitch_offset"]
            bar[s]["velocity"]     = 100 if t["accented"] else 80

    return bar


# =============================================================================
# XPM GENERATION
# =============================================================================

def _layer_block(number: int, vel_start: int, vel_end: int,
                 sample_name: str, sample_file: str,
                 volume: float = 0.707946,
                 pitch: float = 0.0) -> str:
    active = "True" if sample_name else "False"
    return (
        f'          <Layer number="{number}">\n'
        f'            <Active>{active}</Active>\n'
        f'            <Volume>{volume:.6f}</Volume>\n'
        f'            <Pan>0.500000</Pan>\n'
        f'            <Pitch>{pitch:.6f}</Pitch>\n'
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


def _instrument_block(instrument_num: int, voice_key: str,
                      sample_name: str, sample_file: str,
                      bar_step: Optional[dict] = None,
                      mute_group: int = 0) -> str:
    """One <Instrument> XML block. bar_step drives velocity layers if given."""
    cfg = VOICE_DEFAULTS.get(voice_key, VOICE_DEFAULTS["perc"])
    is_active = bool(sample_name and bar_step and bar_step["hit"])

    if is_active:
        vel = bar_step["velocity"]  # 80 normal, 100 accented
        # Pitch offset in semitones clamped to ±12
        p_offset = max(-12, min(12, bar_step.get("pitch_offset", 0)))
        pitch_val = float(p_offset)   # MPC TuneCoarse is integer semitones

        if bar_step["accented"]:
            # Accented: one wide-spanning velocity layer (loud)
            layers_xml = "\n".join([
                _layer_block(1, 1,   40,  sample_name, sample_file, 0.50),
                _layer_block(2, 41,  80,  sample_name, sample_file, 0.70),
                _layer_block(3, 81,  110, sample_name, sample_file, 0.90),
                _layer_block(4, 111, 127, sample_name, sample_file, 1.00),
            ])
        else:
            # Normal hit: softer velocity range
            layers_xml = "\n".join([
                _layer_block(1, 1,   31,  sample_name, sample_file, 0.35),
                _layer_block(2, 32,  63,  sample_name, sample_file, 0.55),
                _layer_block(3, 64,  95,  sample_name, sample_file, 0.75),
                _layer_block(4, 96,  127, sample_name, sample_file, 0.85),
            ])
    else:
        layers_xml = _empty_layers()
        pitch_val = 0.0

    mono_str    = "True" if cfg["mono"] else "False"
    oneshot_str = "True" if cfg["one_shot"] else "False"

    mute_xml = "\n".join(
        f"        <MuteTarget{i+1}>0</MuteTarget{i+1}>" for i in range(4)
    )
    simult_xml = "\n".join(
        f"        <SimultTarget{i+1}>0</SimultTarget{i+1}>" for i in range(4)
    )

    # Pitch offset from L-system (TuneCoarse = semitones)
    tune_coarse = int(pitch_val) if is_active else 0

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
        f'        <TuneCoarse>{tune_coarse}</TuneCoarse>\n'
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


def generate_xpm(system_name: str, bar: list[dict], slug: str,
                 voice_key: str = "perc",
                 pad_start: int = 1,
                 base_midi: int = 36,
                 comment: str = "") -> str:
    """
    Generate a complete Drum Program XPM for a single L-system bar.

    bar: 16-step list from triggers_to_bar()
    pad_start: first pad number (1 for single kit, zone offset for polyrhythm)
    base_midi: MIDI note for first pad
    """
    prog_name = xml_escape(f"XO_OX-LSYS-{system_name.upper()}")

    # Build 128-slot instrument list
    # Active pads occupy base_midi .. base_midi+15
    instrument_parts = []
    pad_note_entries = []
    pad_group_entries = []
    mg = VOICE_DEFAULTS.get(voice_key, VOICE_DEFAULTS["perc"])["mute_group"]

    for i in range(128):
        pad_idx = i - base_midi   # 0-based index into the bar
        if 0 <= pad_idx < 16:
            step = bar[pad_idx]
            if step["hit"]:
                s_name = f"{slug}_step{pad_idx + 1:02d}"
                s_file = f"{s_name}.wav"
            else:
                s_name = ""
                s_file = ""
            instrument_parts.append(
                _instrument_block(i, voice_key, s_name, s_file, step, mute_group=mg)
            )
            # PadNoteMap
            pad_num = pad_start + pad_idx
            if pad_num <= 16:
                pad_note_entries.append(
                    f'        <Pad number="{pad_num}" note="{i}"/>'
                    f'  <!-- step {pad_idx+1} hit={step["hit"]} acc={step["accented"]} -->'
                )
                if mg > 0:
                    pad_group_entries.append(
                        f'        <Pad number="{pad_num}" group="{mg}"/>'
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
        '        <Name>DENSITY</Name>\n'
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


def generate_polyrhythm_xpm(zone_bars: list[dict]) -> str:
    """
    Generate a single XPM with all 4 zone bars merged.
    zone_bars: list of {system_name, voice_key, bar, base_midi, pad_start, slug}
    """
    prog_name = xml_escape("XO_OX-LSYS-POLYRHYTHM")

    # Build 128-slot instrument array
    # Each zone claims 4 MIDI notes (base_midi .. base_midi+3)
    instrument_map: dict[int, dict] = {}
    for zb in zone_bars:
        for pad_idx in range(4):
            midi = zb["base_midi"] + pad_idx
            step = zb["bar"][pad_idx]
            s_name = f"{zb['slug']}_step{pad_idx+1:02d}" if step["hit"] else ""
            s_file = f"{s_name}.wav" if s_name else ""
            instrument_map[midi] = {
                "voice_key": zb["voice_key"],
                "sample_name": s_name,
                "sample_file": s_file,
                "bar_step": step,
            }

    instrument_parts = []
    pad_note_entries = []
    pad_group_entries = []

    for i in range(128):
        if i in instrument_map:
            d = instrument_map[i]
            mg = VOICE_DEFAULTS.get(d["voice_key"], VOICE_DEFAULTS["perc"])["mute_group"]
            instrument_parts.append(
                _instrument_block(i, d["voice_key"],
                                  d["sample_name"], d["sample_file"],
                                  d["bar_step"], mute_group=mg)
            )
        else:
            instrument_parts.append(_instrument_block(i, "", "", "", None, 0))

    # Assign pads sequentially: zone1=1-4, zone2=5-8, zone3=9-12, zone4=13-16
    pad_num = 1
    for zb in zone_bars:
        for pad_idx in range(4):
            midi = zb["base_midi"] + pad_idx
            step = zb["bar"][pad_idx]
            pad_note_entries.append(
                f'        <Pad number="{pad_num}" note="{midi}"/>'
                f'  <!-- {zb["system_name"]} step {pad_idx+1} hit={step["hit"]} -->'
            )
            mg = VOICE_DEFAULTS.get(zb["voice_key"], VOICE_DEFAULTS["perc"])["mute_group"]
            if mg > 0:
                pad_group_entries.append(
                    f'        <Pad number="{pad_num}" group="{mg}"/>'
                )
            pad_num += 1

    instruments_xml = "\n".join(instrument_parts)
    pad_note_xml    = "\n".join(pad_note_entries)
    pad_group_xml   = "\n".join(pad_group_entries)

    pad_json = json.dumps(
        {"ProgramPads": {"Universal": {"value0": False},
                         "Type": {"value0": 5},
                         "universalPad": 32512}},
        separators=(",", ":"),
    )

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
        '    <!-- Polyrhythm: Koch/Sierpinski/Dragon/Plant — 4-voice L-system kit -->\n'
        f'    <ProgramPads>{pad_json}</ProgramPads>\n'
        '    <PadNoteMap>\n'
        f'{pad_note_xml}\n'
        '    </PadNoteMap>\n'
        '    <PadGroupMap>\n'
        f'{pad_group_xml}\n'
        '    </PadGroupMap>\n'
        '    <QLinks>\n'
        '      <QLink number="1">\n'
        '        <Name>DENSITY</Name>\n'
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
# ASCII VISUALIZER
# =============================================================================

def render_trigger_grid(system_name: str, bar: list[dict],
                        lstring: str, iterations: int,
                        ls_def: dict) -> str:
    """Render a 16-step trigger grid as ASCII art."""
    lines = []
    lines.append("=" * 72)
    lines.append("XO_OX L-SYSTEM RHYTHM KIT — TRIGGER GRID")
    lines.append(f"System    : {system_name}")
    lines.append(f"Axiom     : {ls_def['axiom']!r}")
    rules_str = "  ".join(f"{k}->{v}" for k, v in ls_def["rules"].items())
    lines.append(f"Rules     : {rules_str}")
    lines.append(f"Iterations: {iterations}")
    lines.append(f"Expanded  : {len(lstring)} chars (truncated at 4096)")
    lines.append(f"Desc      : {ls_def.get('description', '')}")
    lines.append("=" * 72)
    lines.append("")

    # Step header
    step_header = "Step   : " + "".join(f"{i+1:>3}" for i in range(16))
    lines.append(step_header)
    lines.append("-" * 57)

    # Hit row
    hit_row = "Hit    : " + "".join("  X" if s["hit"] else "  ." for s in bar)
    lines.append(hit_row)

    # Accent row
    acc_row = "Accent : " + "".join("  A" if s["accented"] else "  ." for s in bar)
    lines.append(acc_row)

    # Pitch row
    pitch_row = "Pitch  : " + "".join(f"{s['pitch_offset']:>3}" for s in bar)
    lines.append(pitch_row)

    # Velocity row
    vel_row = "Vel    : " + "".join(f"{s['velocity']:>3}" if s["hit"] else "  ." for s in bar)
    lines.append(vel_row)

    lines.append("")

    # Binary representation
    binary = "".join("1" if s["hit"] else "0" for s in bar)
    hit_count = binary.count("1")
    lines.append(f"Binary : {binary}")
    lines.append(f"Density: {hit_count}/16 steps ({hit_count/16*100:.1f}%)")
    lines.append(f"Accents: {sum(1 for s in bar if s['accented'])} accented hits")

    lines.append("")
    lines.append("Bar visualisation (each step = 1/16th note):")
    lines.append("")

    # Visual bar: 4 groups of 4 beats
    viz_lines = []
    for beat in range(4):
        group = bar[beat*4:(beat+1)*4]
        cells = ""
        for step in group:
            if step["hit"]:
                cells += "[A]" if step["accented"] else "[X]"
            else:
                cells += "[ ]"
        viz_lines.append(f"  Beat {beat+1}: {cells}")
    lines += viz_lines
    lines.append("")
    lines.append("  Legend: [X]=hit  [A]=accented  [ ]=rest")
    lines.append("=" * 72)
    return "\n".join(lines)


# =============================================================================
# MANIFEST
# =============================================================================

def build_manifest(system_name: str, ls_def: dict, iterations: int,
                   lstring: str, bar: list[dict], slug: str) -> dict:
    binary = "".join("1" if s["hit"] else "0" for s in bar)
    return {
        "tool":        "xpn_lsystem_kit",
        "version":     "1.0.0",
        "generated":   str(date.today()),
        "system_name": system_name,
        "axiom":       ls_def["axiom"],
        "rules":       ls_def["rules"],
        "angle":       ls_def.get("angle", 0),
        "description": ls_def.get("description", ""),
        "iterations":  iterations,
        "expanded_len": len(lstring),
        "slug":        slug,
        "trigger_sequence_binary": binary,
        "hit_count":   binary.count("1"),
        "density_pct": round(binary.count("1") / 16 * 100, 1),
        "steps": [
            {
                "step":         i + 1,
                "hit":          bar[i]["hit"],
                "accented":     bar[i]["accented"],
                "pitch_offset": bar[i]["pitch_offset"],
                "velocity":     bar[i]["velocity"],
            }
            for i in range(16)
        ],
    }


# =============================================================================
# SINGLE SYSTEM PIPELINE
# =============================================================================

def process_system(system_name: str, ls_def: dict, iterations: int,
                   output_dir: Path, voice_key: str = "perc",
                   base_midi: int = 36) -> tuple[list[dict], str, dict]:
    """Expand, extract, render, write files. Returns (bar, lstring, manifest)."""
    print(f"[Hex+Rex] L-System: {system_name} | iter={iterations}")

    lstring = expand(ls_def["axiom"], ls_def["rules"], iterations, max_len=4096)
    print(f"          Expanded: {len(lstring)} chars")

    trig_chars = ls_def.get("triggers", "Ff")
    triggers = extract_triggers(lstring, max_steps=256, trigger_chars=trig_chars)
    bar      = triggers_to_bar(triggers)
    binary   = "".join("1" if s["hit"] else "0" for s in bar)
    print(f"          Pattern : {binary}  ({binary.count('1')}/16 hits)")

    slug = f"lsys_{system_name}"

    # 1. XPM
    comment = (f"L-System: {system_name} axiom={ls_def['axiom']!r} "
               f"iter={iterations} density={binary.count('1')}/16")
    xpm_xml = generate_xpm(system_name, bar, slug, voice_key=voice_key,
                           base_midi=base_midi, comment=comment)
    xpm_path = output_dir / f"{slug}_kit.xpm"
    xpm_path.write_text(xpm_xml, encoding="utf-8")
    print(f"          XPM     -> {xpm_path}")

    # 2. Manifest JSON
    manifest = build_manifest(system_name, ls_def, iterations, lstring, bar, slug)
    manifest_path = output_dir / f"{slug}_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"          Manifest-> {manifest_path}")

    # 3. ASCII grid
    grid_txt  = render_trigger_grid(system_name, bar, lstring, iterations, ls_def)
    grid_path = output_dir / f"{slug}_viz.txt"
    grid_path.write_text(grid_txt, encoding="utf-8")
    print(f"          Viz     -> {grid_path}")

    return bar, lstring, manifest


# =============================================================================
# MAIN
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN L-System Rhythm Kit Generator — XO_OX / Hex+Rex",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--system", default="koch",
                        help="L-system name (koch|sierpinski|dragon|plant|algae|penrose|all)")
    parser.add_argument("--iterations", "-i", type=int, default=3,
                        help="Expansion iterations (default: 3)")
    parser.add_argument("--output", "-o", default="./kits",
                        help="Output directory (default: ./kits)")
    parser.add_argument("--polyrhythm", action="store_true",
                        help="Generate 4-voice polyrhythm kit (Koch+Sierpinski+Dragon+Plant)")
    parser.add_argument("--visualize", metavar="SYSTEM",
                        help="Print trigger grid for system and exit")
    # Custom L-system
    parser.add_argument("--axiom",  help="Custom axiom string")
    parser.add_argument("--rules",  help="Custom rules e.g. 'F:F+F-F,X:FX'")
    parser.add_argument("--name",   default="custom", help="Name for custom system")

    args = parser.parse_args()

    # --visualize
    if args.visualize:
        sname = args.visualize.lower()
        if sname not in L_SYSTEMS:
            print(f"[Hex+Rex] Unknown system '{sname}'. Available: {', '.join(L_SYSTEMS)}")
            sys.exit(1)
        ls_def   = L_SYSTEMS[sname]
        iters    = args.iterations
        lstring  = expand(ls_def["axiom"], ls_def["rules"], iters)
        trig_chars = ls_def.get("triggers", "Ff")
        triggers = extract_triggers(lstring, trigger_chars=trig_chars)
        bar      = triggers_to_bar(triggers)
        print(render_trigger_grid(sname, bar, lstring, iters, ls_def))
        sys.exit(0)

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    # Custom L-system overrides
    if args.axiom:
        rules = {}
        if args.rules:
            for pair in args.rules.split(","):
                if ":" in pair:
                    k, v = pair.split(":", 1)
                    rules[k.strip()] = v.strip()
        custom_def = {
            "axiom":       args.axiom,
            "rules":       rules,
            "angle":       0,
            "description": f"Custom L-system: {args.name}",
        }
        process_system(args.name, custom_def, args.iterations, output_dir)
        print(f"\n[Hex+Rex] Done. Output in {output_dir}/")
        return

    # --polyrhythm
    if args.polyrhythm:
        print("[Hex+Rex] Polyrhythm mode: Koch/Sierpinski/Dragon/Plant × 4 voices")
        zone_bars = []
        midi_bases = [36, 40, 44, 48]   # kick/snare/hat/accent zones
        for idx, (sname, vkey, midi_notes, desc) in enumerate(POLY_ZONES):
            ls_def     = L_SYSTEMS[sname]
            lstring    = expand(ls_def["axiom"], ls_def["rules"], args.iterations)
            trig_chars = ls_def.get("triggers", "Ff")
            triggers   = extract_triggers(lstring, trigger_chars=trig_chars)
            bar        = triggers_to_bar(triggers)
            binary   = "".join("1" if s["hit"] else "0" for s in bar)
            print(f"  {desc}: {binary[:4]} ({binary[:4].count('1')}/4 hits in pads)")
            zone_bars.append({
                "system_name": sname,
                "voice_key":   vkey,
                "bar":         bar,
                "base_midi":   midi_bases[idx],
                "pad_start":   1 + idx * 4,
                "slug":        f"poly_{sname}",
                "lstring":     lstring,
            })

        # Single polyrhythm XPM
        xpm_xml  = generate_polyrhythm_xpm(zone_bars)
        xpm_path = output_dir / "lsys_polyrhythm_kit.xpm"
        xpm_path.write_text(xpm_xml, encoding="utf-8")
        print(f"[Hex+Rex] Wrote polyrhythm XPM  -> {xpm_path}")

        # Manifest per zone + combined
        combined_manifest = {
            "tool":      "xpn_lsystem_kit",
            "mode":      "polyrhythm",
            "generated": str(date.today()),
            "zones": []
        }
        for zb in zone_bars:
            ls_def = L_SYSTEMS[zb["system_name"]]
            m = build_manifest(zb["system_name"], ls_def, args.iterations,
                               zb["lstring"], zb["bar"], zb["slug"])
            m["voice_key"]  = zb["voice_key"]
            m["base_midi"]  = zb["base_midi"]
            m["pad_start"]  = zb["pad_start"]
            combined_manifest["zones"].append(m)

            # Per-zone viz
            grid_txt  = render_trigger_grid(zb["system_name"], zb["bar"],
                                            zb["lstring"], args.iterations,
                                            L_SYSTEMS[zb["system_name"]])
            grid_path = output_dir / f"lsys_poly_{zb['system_name']}_viz.txt"
            grid_path.write_text(grid_txt, encoding="utf-8")
            print(f"[Hex+Rex] Wrote zone viz        -> {grid_path}")

        manifest_path = output_dir / "lsys_polyrhythm_manifest.json"
        manifest_path.write_text(json.dumps(combined_manifest, indent=2), encoding="utf-8")
        print(f"[Hex+Rex] Wrote polyrhythm manifest -> {manifest_path}")
        print(f"\n[Hex+Rex] Done. Load lsys_polyrhythm_kit.xpm into your MPC.")
        return

    # --system all
    if args.system == "all":
        print("[Hex+Rex] Generating all 6 L-system kits...")
        for sname, ls_def in L_SYSTEMS.items():
            process_system(sname, ls_def, args.iterations, output_dir)
            print()
        print(f"[Hex+Rex] Done. {len(L_SYSTEMS)} kits written to {output_dir}/")
        return

    # Single system
    sname = args.system.lower()
    if sname not in L_SYSTEMS:
        print(f"[Hex+Rex] Unknown system '{sname}'.")
        print(f"          Available: {', '.join(L_SYSTEMS.keys())}")
        sys.exit(1)

    ls_def = L_SYSTEMS[sname]
    bar, lstring, manifest = process_system(sname, ls_def, args.iterations, output_dir)

    # Print grid to terminal
    print()
    print(render_trigger_grid(sname, bar, lstring, args.iterations, ls_def))
    print(f"\n[Hex+Rex] Done. Load lsys_{sname}_kit.xpm into your MPC.")


if __name__ == "__main__":
    main()
