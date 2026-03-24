#!/usr/bin/env python3
"""
XPN CA Presets Kit — XO_OX Designs
Wolfram's 256 elementary cellular automaton rules as chromatic keygroup zones.

Maps all 256 CA rules to pad triggers via behavioral class. The OUTWIT engine
(XOctopus, 8-arm CA engine) implements Wolfram CA internally — this tool
packages that rule-space as a portable XPN keygroup for non-XOlokun users.

Wolfram Classes (behavioral taxonomy):
  Class I   (fixed point): Rules that converge to uniform state — silence/solid
                           e.g., Rule 0, Rule 255, Rule 8, Rule 32
  Class II  (periodic):    Rules producing stable repetitive patterns
                           e.g., Rule 4, Rule 108, Rule 51
  Class III (chaotic):     Rules producing pseudo-random, aperiodic evolution
                           e.g., Rule 30, Rule 45, Rule 86, Rule 135
  Class IV  (complex):     Turing-complete emergent behavior, localized structures
                           e.g., Rule 110, Rule 124, Rule 137

Pad Assignment:
  Bank A (pads 1–4):   Class I   — velocity layers 1–31
  Bank B (pads 5–8):   Class II  — velocity layers 32–63
  Bank C (pads 9–12):  Class III — velocity layers 64–95
  Bank D (pads 13–16): Class IV  — velocity layers 96–127

For each rule, a 16-step trigger sequence is derived from:
  1. Single center-cell seed: ...00010000...
  2. Evolve 16 generations under the rule
  3. Extract center 16 cells of generation 16
  4. Binary → trigger sequence (1=hit, 0=rest)

XPM Structure:
  16 pads × 4 velocity layers = 64 instrument slots
  Each pad = one Wolfram class (4 representative rules per layer)
  ca_rules_manifest.json: rule → pad, trigger sequence, class
  ca_evolution_viz.txt: ASCII art of rule evolutions

Usage:
  python xpn_ca_presets_kit.py --output ./kits/
  python xpn_ca_presets_kit.py --rules 30,90,110,184 --output ./kits/
  python xpn_ca_presets_kit.py --class chaotic --output ./kits/
  python xpn_ca_presets_kit.py --visualize 110

Output:
  ca_rules_kit.xpm
  ca_rules_manifest.json
  ca_evolution_viz.txt
"""

import argparse
import json
import sys
from datetime import date
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from xml.sax.saxutils import escape as xml_escape

# ---------------------------------------------------------------------------
# Wolfram Class Assignments
# ---------------------------------------------------------------------------
# Scientifically derived classifications from Wolfram's "A New Kind of Science"
# and subsequent computational studies. Rules 0–255 are partitioned into
# 4 behavioral classes based on long-run evolution from random initial conditions.

# Class I: Fixed-point attractors — evolution halts to uniform state
CLASS_I = {
    0, 2, 4, 8, 10, 12, 13, 14, 16, 19, 24, 26, 32, 34, 36, 40, 42,
    44, 48, 50, 56, 58, 72, 74, 76, 78, 104, 128, 130, 132, 136, 138,
    140, 144, 152, 160, 162, 164, 168, 170, 172, 176, 200, 204, 232,
    240, 248, 255
}

# Class II: Periodic — stable repeating structures
CLASS_II = {
    1, 3, 5, 6, 7, 9, 11, 15, 17, 18, 20, 21, 22, 23, 25, 27, 28, 29,
    31, 33, 35, 37, 38, 39, 41, 43, 46, 47, 49, 51, 53, 55, 57, 59, 61,
    63, 65, 67, 69, 71, 73, 77, 79, 94, 108, 109, 111, 127, 131, 133,
    137, 139, 143, 145, 153, 155, 161, 163, 165, 171, 173, 177, 178,
    179, 181, 191, 201, 205, 223, 233
}

# Class III: Chaotic — pseudo-random, aperiodic evolution
CLASS_III = {
    6, 22, 30, 45, 54, 60, 86, 90, 102, 105, 106, 107, 110, 120, 121,
    122, 126, 135, 146, 150, 151, 182, 183, 184, 188, 189, 190, 219,
    249, 252, 253, 254
}

# Class IV: Complex — Turing-complete emergent structures (gliders, etc.)
CLASS_IV = {
    20, 41, 54, 62, 103, 110, 124, 137, 193, 195
}

# Resolve overlaps: IV > III > II > I (more complex class wins)
def _assign_class(rule: int) -> int:
    """Return 4/3/2/1 for Wolfram class of this rule."""
    if rule in CLASS_IV:
        return 4
    if rule in CLASS_III:
        return 3
    if rule in CLASS_II:
        return 2
    return 1  # Class I by default


# ---------------------------------------------------------------------------
# Representative rules per class — 4 rules × 4 banks = 16 pads
# Each tuple is (rule_number, nickname)
# ---------------------------------------------------------------------------
CLASS_REPRESENTATIVES = {
    1: [
        (0,   "Void"),          # All cells → 0, complete silence
        (255, "Solid"),         # All cells → 1, constant
        (8,   "Decay"),         # Single bits absorbed
        (32,  "Fade"),          # Uniform convergence
    ],
    2: [
        (4,   "Checker"),       # Checkerboard static
        (51,  "Stripes"),       # Alternating bands
        (108, "Weave"),         # Complex periodic lattice
        (94,  "Ripple"),        # Wave-like propagation
    ],
    3: [
        (30,  "Chaos"),         # The canonical chaotic rule (used in Mathematica RNG)
        (45,  "Turbulence"),    # Chaotic with irregular structures
        (86,  "Static"),        # Dense random-looking evolution
        (90,  "XOR"),           # Sierpinski-triangle XOR rule
    ],
    4: [
        (110, "Universal"),     # Proven Turing-complete by Wolfram/Cook
        (124, "Glider"),        # Produces glider-like structures
        (137, "Entangle"),      # Complex emergent coupling
        (193, "Cascade"),       # Cascading localized structures
    ],
}

CLASS_NAMES = {1: "fixed", 2: "periodic", 3: "chaotic", 4: "complex"}
CLASS_CLI_NAMES = {"fixed": 1, "periodic": 2, "chaotic": 3, "complex": 4}

# Velocity bands per class (pad bank)
CLASS_VEL = {
    1: (1,  31),   # Bank A
    2: (32, 63),   # Bank B
    3: (64, 95),   # Bank C
    4: (96, 127),  # Bank D
}

# ---------------------------------------------------------------------------
# Cellular Automaton Engine
# ---------------------------------------------------------------------------

def apply_rule(rule: int, cells: List[int]) -> List[int]:
    """Apply Wolfram elementary CA rule to one generation of cells."""
    n = len(cells)
    new = [0] * n
    for i in range(n):
        left   = cells[(i - 1) % n]
        center = cells[i]
        right  = cells[(i + 1) % n]
        index  = (left << 2) | (center << 1) | right
        new[i] = (rule >> index) & 1
    return new


def evolve(rule: int, seed: List[int], generations: int) -> List[List[int]]:
    """Evolve a CA from seed for N generations. Returns all generations."""
    history = [seed[:]]
    current = seed[:]
    for _ in range(generations):
        current = apply_rule(rule, current)
        history.append(current[:])
    return history


def rule_trigger_sequence(rule: int, width: int = 32, generations: int = 16) -> List[int]:
    """
    Derive a 16-step trigger sequence from rule's CA evolution.
    1. Seed: single center cell active
    2. Evolve 16 generations
    3. Extract center 16 cells of final generation
    """
    seed = [0] * width
    seed[width // 2] = 1
    history = evolve(rule, seed, generations)
    final_gen = history[-1]
    # Center 16 cells
    start = (width - 16) // 2
    return final_gen[start:start + 16]


def ascii_evolution(rule: int, width: int = 33, generations: int = 20) -> str:
    """Return ASCII visualization of rule evolution."""
    seed = [0] * width
    seed[width // 2] = 1
    history = evolve(rule, seed, generations)
    lines = [f"Rule {rule:3d} — {CLASS_NAMES[_assign_class(rule)].upper()} (Class {_assign_class(rule)})"]
    lines.append("─" * width)
    for gen_idx, row in enumerate(history):
        bar = "".join("█" if c else "·" for c in row)
        lines.append(f"{gen_idx:2d} │{bar}│")
    trigger = rule_trigger_sequence(rule)
    trigger_str = "".join("X" if t else "." for t in trigger)
    lines.append("─" * width)
    lines.append(f"   Trigger: [{trigger_str}]")
    lines.append("")
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# XPM Layer / Instrument / Program builders
# ---------------------------------------------------------------------------

PROGRAM_SLUG = "ca_rules"


def _layer_block(number: int, vel_start: int, vel_end: int,
                 sample_name: str, volume: float = 0.707946) -> str:
    active = "True" if sample_name else "False"
    file_path = f"Samples/{PROGRAM_SLUG}/{sample_name}" if sample_name else ""
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
    return _layer_block(number, 0, 0, "", 0.707946)


def _instrument_block(instrument_num: int, midi_note: int,
                      cls: int, pad_idx: int,
                      rules_for_pad: List[Tuple]) -> str:
    """
    Build one <Instrument> XML block.
    rules_for_pad: list of (rule_num, nickname) — up to 4 entries.
    Each becomes a velocity layer within the class velocity band.
    """
    vel_lo, vel_hi = CLASS_VEL[cls]
    # Split class velocity band into 4 equal sub-layers
    band = vel_hi - vel_lo + 1
    layer_size = band // 4
    layers_xml_parts = []
    for i in range(4):
        if i < len(rules_for_pad):
            rule_num, nick = rules_for_pad[i]
            sub_lo = vel_lo + i * layer_size
            sub_hi = vel_lo + (i + 1) * layer_size - 1
            if i == 3:
                sub_hi = vel_hi  # absorb remainder in last layer
            sample_name = f"ca_rule_{rule_num:03d}_{nick.lower()}.wav"
            layers_xml_parts.append(
                _layer_block(i + 1, sub_lo, sub_hi, sample_name)
            )
        else:
            layers_xml_parts.append(_empty_layer(i + 1))

    layers_xml = "\n".join(layers_xml_parts)
    class_name = CLASS_NAMES[cls].upper()
    rule_nums = [r for r, _ in rules_for_pad]

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
        f'          <ProgramName>CA Class {cls} — {class_name} (Rules {rule_nums})</ProgramName>\n'
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


# Pad layout: 16 active pads covering 4 Wolfram classes × 4 pads each
# MIDI notes: Bank A = 36-39, Bank B = 40-43, Bank C = 44-47, Bank D = 48-51
PAD_LAYOUT = [
    # (pad_index, midi_note, class, pad_slot_within_class)
    # Bank A — Class I (fixed)
    (0,  36, 1, 0), (1,  37, 1, 1), (2,  38, 1, 2), (3,  39, 1, 3),
    # Bank B — Class II (periodic)
    (4,  40, 2, 0), (5,  41, 2, 1), (6,  42, 2, 2), (7,  43, 2, 3),
    # Bank C — Class III (chaotic)
    (8,  44, 3, 0), (9,  45, 3, 1), (10, 46, 3, 2), (11, 47, 3, 3),
    # Bank D — Class IV (complex)
    (12, 48, 4, 0), (13, 49, 4, 1), (14, 50, 4, 2), (15, 51, 4, 3),
]


def build_xpm(selected_rules: Optional[List[int]] = None) -> str:
    """Generate the full XPM XML string."""
    today = date.today().isoformat()
    instruments_xml_parts = []
    instrument_num = 1

    # Build instrument blocks for the 16 active pads
    for pad_idx, midi_note, cls, slot_in_class in PAD_LAYOUT:
        reps = CLASS_REPRESENTATIVES[cls]
        # Each of the 4 pads in a class gets 1 representative rule as its
        # primary layer, plus variants at sub-velocity bands.
        # Slot 0 → all 4 reps stacked as velocity layers on one pad
        # Slots 1–3 → single rule repeated across all 4 layers (for drill)
        if slot_in_class == 0:
            rules_for_pad = reps  # 4-rule velocity stack
        else:
            # Single rule, one per pad (for isolated trigger playback)
            rule_num, nick = reps[slot_in_class]
            rules_for_pad = [(rule_num, nick)] * 4
        instruments_xml_parts.append(
            _instrument_block(instrument_num, midi_note, cls, pad_idx, rules_for_pad)
        )
        instrument_num += 1

    # Fill remaining 112 instrument slots as inactive (XPM requires 128 total)
    # Remaining MIDI notes 52–127
    next_note = 52
    while instrument_num <= 128:
        instruments_xml_parts.append(
            _inactive_instrument(instrument_num, next_note)
        )
        instrument_num += 1
        next_note = min(next_note + 1, 127)

    instruments_xml = "\n".join(instruments_xml_parts)

    return f"""<?xml version="1.0" encoding="UTF-8"?>
<MPCVObject>
  <Version>
    <File_Version>2.1</File_Version>
    <Application>MPC</Application>
    <Application_Version>2.10</Application_Version>
  </Version>
  <Program type="Keygroup">
    <Name>CA Rules Kit</Name>
    <Slug>{PROGRAM_SLUG}</Slug>
    <DateCreated>{today}</DateCreated>
    <Description>Wolfram 256 Elementary CA Rules — Chromatic Keygroup Kit. OUTWIT engine rule-space packaged for universal MPC use. 4 banks × 4 pads = 16 pads covering Classes I–IV (fixed/periodic/chaotic/complex). Velocity layers select representative rules within each class.</Description>
    <NumInstruments>128</NumInstruments>
    <ProgramVolume>1.000000</ProgramVolume>
    <ProgramPan>0.500000</ProgramPan>
    <ProgramTranspose>0</ProgramTranspose>
    <MemoryUsage>0</MemoryUsage>
    <LFO1Shape>0</LFO1Shape>
    <LFO1Rate>0.000000</LFO1Rate>
    <LFO1Depth>0.000000</LFO1Depth>
    <FilterQ>0.000000</FilterQ>
    <InstrumentList>
{instruments_xml}
    </InstrumentList>
  </Program>
</MPCVObject>
"""


def build_manifest(selected_rules: Optional[List[int]] = None) -> dict:
    """Build the ca_rules_manifest.json content."""
    manifest = {
        "tool": "xpn_ca_presets_kit",
        "version": "1.0.0",
        "date": date.today().isoformat(),
        "description": "Wolfram elementary CA rules mapped to MPC pad triggers",
        "xolokun_engine": "OUTWIT (XOctopus — 8-arm Wolfram CA)",
        "pad_bank_map": {
            "bank_a": {"pads": "1–4",  "midi": "36–39", "class": 1, "name": "fixed",    "velocity": "1–31"},
            "bank_b": {"pads": "5–8",  "midi": "40–43", "class": 2, "name": "periodic", "velocity": "32–63"},
            "bank_c": {"pads": "9–12", "midi": "44–47", "class": 3, "name": "chaotic",  "velocity": "64–95"},
            "bank_d": {"pads": "13–16","midi": "48–51", "class": 4, "name": "complex",  "velocity": "96–127"},
        },
        "representative_rules": {},
        "all_rules": {},
    }

    # Representative rules with trigger sequences
    for cls, reps in CLASS_REPRESENTATIVES.items():
        manifest["representative_rules"][f"class_{cls}"] = []
        for rule_num, nick in reps:
            trigger = rule_trigger_sequence(rule_num)
            trigger_str = "".join(str(t) for t in trigger)
            manifest["representative_rules"][f"class_{cls}"].append({
                "rule": rule_num,
                "nickname": nick,
                "trigger_sequence": trigger_str,
                "trigger_readable": "".join("X" if t else "." for t in trigger),
                "density": sum(trigger) / len(trigger),
                "sample_name": f"ca_rule_{rule_num:03d}_{nick.lower()}.wav",
            })

    # All 256 rules
    rules_to_process = selected_rules if selected_rules else list(range(256))
    for rule in rules_to_process:
        cls = _assign_class(rule)
        trigger = rule_trigger_sequence(rule)
        trigger_str = "".join(str(t) for t in trigger)
        manifest["all_rules"][str(rule)] = {
            "rule": rule,
            "class": cls,
            "class_name": CLASS_NAMES[cls],
            "trigger_sequence": trigger_str,
            "trigger_readable": "".join("X" if t else "." for t in trigger),
            "density": sum(trigger) / len(trigger),
            "pad_bank": chr(ord("A") + cls - 1),
        }

    return manifest


def build_viz(rules: Optional[List[int]] = None) -> str:
    """Build the ASCII evolution visualization text."""
    lines = [
        "╔══════════════════════════════════════════════════════════════╗",
        "║         WOLFRAM ELEMENTARY CA RULES — XO_OX DESIGNS          ║",
        "║        OUTWIT Engine Rule-Space Visualization Atlas           ║",
        "╚══════════════════════════════════════════════════════════════╝",
        "",
        "Each rule evolves a single center-cell seed for 20 generations.",
        "█ = alive  · = dead  │ = boundary markers",
        "Trigger sequence extracted from center 16 cells of generation 16.",
        "X = trigger hit  . = rest",
        "",
    ]

    target_rules = rules if rules else []
    # If no specific rules, show all 4 class representatives
    if not target_rules:
        for cls in [1, 2, 3, 4]:
            class_name = CLASS_NAMES[cls].upper()
            lines.append(f"{'═' * 40}")
            lines.append(f"  CLASS {cls} — {class_name}")
            lines.append(f"{'═' * 40}")
            lines.append("")
            for rule_num, nick in CLASS_REPRESENTATIVES[cls]:
                lines.append(ascii_evolution(rule_num))
    else:
        for rule_num in target_rules:
            lines.append(ascii_evolution(rule_num))

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="XPN CA Presets Kit — Wolfram CA rules as MPC keygroup zones",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Build full kit (all 4 Wolfram classes, 16 pads)
  python xpn_ca_presets_kit.py --output ./kits/

  # Build kit for specific rules only
  python xpn_ca_presets_kit.py --rules 30,90,110,184 --output ./kits/

  # Build kit for one behavioral class
  python xpn_ca_presets_kit.py --class chaotic --output ./kits/

  # Visualize a specific rule's evolution
  python xpn_ca_presets_kit.py --visualize 110

  # List all rules with their class assignments
  python xpn_ca_presets_kit.py --list
        """
    )
    parser.add_argument("--output", "-o", metavar="DIR",
                        help="Output directory for kit files")
    parser.add_argument("--rules", metavar="30,90,110",
                        help="Comma-separated rule numbers (0–255)")
    parser.add_argument("--class", dest="cls", metavar="CLASS",
                        choices=["fixed", "periodic", "chaotic", "complex"],
                        help="Filter to one behavioral class")
    parser.add_argument("--visualize", metavar="RULE", type=int,
                        help="Print ASCII evolution for one rule and exit")
    parser.add_argument("--list", action="store_true",
                        help="List all 256 rules with class assignments and exit")
    args = parser.parse_args()

    # --visualize: print and exit
    if args.visualize is not None:
        rule = args.visualize
        if not 0 <= rule <= 255:
            print(f"ERROR: rule must be 0–255, got {rule}", file=sys.stderr)
            sys.exit(1)
        print(ascii_evolution(rule))
        trigger = rule_trigger_sequence(rule)
        print(f"Trigger density: {sum(trigger)}/16 hits ({100*sum(trigger)/16:.0f}%)")
        return

    # --list: dump class table and exit
    if args.list:
        header = f"{'Rule':>5}  {'Class':>5}  {'Name':<10}  Trigger"
        print(header)
        print("─" * 50)
        for rule in range(256):
            cls = _assign_class(rule)
            trigger = rule_trigger_sequence(rule)
            trig_str = "".join("X" if t else "." for t in trigger)
            print(f"{rule:5d}  {cls:5d}  {CLASS_NAMES[cls]:<10}  {trig_str}")
        return

    if not args.output:
        parser.print_help()
        sys.exit(1)

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    # Resolve rule subset
    selected_rules = None
    if args.rules:
        try:
            selected_rules = [int(r.strip()) for r in args.rules.split(",")]
            for r in selected_rules:
                if not 0 <= r <= 255:
                    raise ValueError(f"Rule {r} out of range 0–255")
        except ValueError as e:
            print(f"ERROR: {e}", file=sys.stderr)
            sys.exit(1)
    elif args.cls:
        cls_num = CLASS_CLI_NAMES[args.cls]
        selected_rules = [r for r in range(256) if _assign_class(r) == cls_num]

    # Build outputs
    print(f"Building CA Rules Kit → {output_dir}/")

    xpm_path = output_dir / "ca_rules_kit.xpm"
    xpm_content = build_xpm(selected_rules)
    xpm_path.write_text(xpm_content, encoding="utf-8")
    print(f"  ✓ {xpm_path.name}")

    manifest_path = output_dir / "ca_rules_manifest.json"
    manifest = build_manifest(selected_rules)
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"  ✓ {manifest_path.name}")

    viz_path = output_dir / "ca_evolution_viz.txt"
    viz_rules = selected_rules if selected_rules and len(selected_rules) <= 32 else None
    viz_content = build_viz(viz_rules)
    viz_path.write_text(viz_content, encoding="utf-8")
    print(f"  ✓ {viz_path.name}")

    # Summary
    rule_count = len(selected_rules) if selected_rules else 256
    print(f"\nKit summary:")
    print(f"  Rules covered: {rule_count}")
    print(f"  Active pads:   16 (4 classes × 4 pads)")
    print(f"  Velocity map:  Class I=1–31 | II=32–63 | III=64–95 | IV=96–127")
    print(f"  XPM format:    Keygroup, 128 instruments, 4 layers each")
    print(f"\nSample naming convention:")
    print(f"  ca_rule_{{num}}_{{nickname}}.wav  (place in Samples/ca_rules/)")
    print(f"\nClass representatives (suggested sample recordings):")
    for cls in [1, 2, 3, 4]:
        print(f"  Class {cls} ({CLASS_NAMES[cls]}):")
        for rule_num, nick in CLASS_REPRESENTATIVES[cls]:
            trigger = rule_trigger_sequence(rule_num)
            trig_str = "".join("X" if t else "." for t in trigger)
            print(f"    Rule {rule_num:3d} [{nick:<12}] trigger: {trig_str}")


if __name__ == "__main__":
    main()
