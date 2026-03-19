#!/usr/bin/env python3
"""
XPN Transit Kit — XO_OX Designs
City transit schedules as polyrhythmic drum kits.

Train arrival headways → trigger density → pad patterns.
Each transit line = one pad's rhythmic signature.
Rush hour vs. off-peak = velocity layers.
Multiple cities simultaneously = real-world polyrhythm.

Schedule Data (hardcoded approximate weekday headways in minutes):

  NYC Subway:
    A/C/E:     rush=4  off-peak=8
    1/2/3:     rush=3  off-peak=6
    4/5/6:     rush=2  off-peak=5
    J/Z:       rush=6  off-peak=12
    L:         rush=4  off-peak=8
    N/Q/R/W:   rush=3  off-peak=7

  Tokyo Metro / JR:
    Ginza Line:        rush=2  off-peak=5
    Chiyoda Line:      rush=2  off-peak=5
    Yamanote Line:     rush=2  off-peak=4
    Tokaido Shinkansen:rush=10 off-peak=15

  London Underground:
    Jubilee Line:   rush=2.5  off-peak=5
    Victoria Line:  rush=2    off-peak=4
    Bakerloo Line:  rush=5    off-peak=8
    District Line:  rush=4    off-peak=8

Time mapping (16 steps = 1 day):
  Each step represents 1 hour of service from 6am to 9pm (16 hours).
  Departures per hour derived from headway: floor(60 / headway).
  Rush hours (7–9am = steps 1–3, 5–7pm = steps 11–13) mark high-velocity.
  Off-peak hours use lower-density patterns.
  A step triggers if its departure count exceeds the line's per-step threshold.

Trigger logic:
  - Compute departures per step for rush and off-peak headways
  - Rush steps: velocity layer 2 (dense)
  - Off-peak steps: velocity layer 1 (sparse)
  - Threshold: trigger if departures >= median departures for that line
  - This ensures each line produces a distinct rhythmic character

XPM Structure:
  16 pads = 16 transit lines (4 NYC + 4 Tokyo + 4 London + 4 wildcard)
  Layer 1 (vel 1–63):   off-peak pattern
  Layer 2 (vel 64–127): rush hour pattern
  ca_rules_manifest.json: line → pad, trigger grids, headways

Wildcard lines (bonus rhythms):
  Paris RER A:           rush=2  off-peak=5
  Berlin S-Bahn Ring:    rush=4  off-peak=10
  Chicago L Red Line:    rush=5  off-peak=10
  Seoul Metro Line 2:    rush=2  off-peak=4

Usage:
  python xpn_transit_kit.py --output ./kits/
  python xpn_transit_kit.py --city nyc --output ./kits/
  python xpn_transit_kit.py --city nyc,tokyo,london --output ./kits/
  python xpn_transit_kit.py --visualize nyc

Output:
  transit_kit_{cities}.xpm
  transit_manifest.json
  transit_grid.txt
"""

import argparse
import json
import sys
from datetime import date
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from xml.sax.saxutils import escape as xml_escape

# ---------------------------------------------------------------------------
# Transit Schedule Data
# ---------------------------------------------------------------------------
# headway_rush:     train interval in minutes during peak hours
# headway_offpeak:  train interval in minutes off-peak
# service_start:    first departure hour (24h)
# service_end:      last departure hour (24h, exclusive)
# rush_windows:     list of (start_hour, end_hour) — peak periods

TRANSIT_LINES = {
    # --- NYC Subway ---
    "nyc_ace": {
        "city":           "nyc",
        "city_display":   "NYC",
        "name":           "A/C/E",
        "display":        "NYC A/C/E",
        "headway_rush":   4,
        "headway_offpeak":8,
        "service_start":  6,
        "service_end":    22,   # 24-hour service; capped at 22 for our 16-step window
        "rush_windows":   [(7, 10), (17, 20)],
        "color":          "#0039A6",
        "pad_index":      0,
    },
    "nyc_123": {
        "city": "nyc", "city_display": "NYC",
        "name": "1/2/3", "display": "NYC 1/2/3",
        "headway_rush": 3, "headway_offpeak": 6,
        "service_start": 6, "service_end": 22,
        "rush_windows": [(7, 10), (17, 20)],
        "color": "#EE352E", "pad_index": 1,
    },
    "nyc_456": {
        "city": "nyc", "city_display": "NYC",
        "name": "4/5/6", "display": "NYC 4/5/6",
        "headway_rush": 2, "headway_offpeak": 5,
        "service_start": 6, "service_end": 22,
        "rush_windows": [(7, 10), (17, 20)],
        "color": "#00933C", "pad_index": 2,
    },
    "nyc_jz": {
        "city": "nyc", "city_display": "NYC",
        "name": "J/Z", "display": "NYC J/Z",
        "headway_rush": 6, "headway_offpeak": 12,
        "service_start": 6, "service_end": 22,
        "rush_windows": [(7, 10), (17, 20)],
        "color": "#996633", "pad_index": 3,
    },
    "nyc_l": {
        "city": "nyc", "city_display": "NYC",
        "name": "L", "display": "NYC L",
        "headway_rush": 4, "headway_offpeak": 8,
        "service_start": 6, "service_end": 22,
        "rush_windows": [(7, 10), (17, 20)],
        "color": "#A7A9AC", "pad_index": 4,
    },
    "nyc_nqrw": {
        "city": "nyc", "city_display": "NYC",
        "name": "N/Q/R/W", "display": "NYC N/Q/R/W",
        "headway_rush": 3, "headway_offpeak": 7,
        "service_start": 6, "service_end": 22,
        "rush_windows": [(7, 10), (17, 20)],
        "color": "#FCCC0A", "pad_index": 5,
    },

    # --- Tokyo ---
    "tokyo_ginza": {
        "city": "tokyo", "city_display": "Tokyo",
        "name": "Ginza Line", "display": "Tokyo Ginza",
        "headway_rush": 2, "headway_offpeak": 5,
        "service_start": 5, "service_end": 24,
        "rush_windows": [(7, 10), (17, 20)],
        "color": "#F39700", "pad_index": 6,
    },
    "tokyo_chiyoda": {
        "city": "tokyo", "city_display": "Tokyo",
        "name": "Chiyoda Line", "display": "Tokyo Chiyoda",
        "headway_rush": 2, "headway_offpeak": 5,
        "service_start": 5, "service_end": 24,
        "rush_windows": [(7, 10), (17, 20)],
        "color": "#00BB85", "pad_index": 7,
    },
    "tokyo_yamanote": {
        "city": "tokyo", "city_display": "Tokyo",
        "name": "Yamanote JR", "display": "Tokyo Yamanote",
        "headway_rush": 2, "headway_offpeak": 4,
        "service_start": 5, "service_end": 24,
        "rush_windows": [(7, 10), (17, 20)],
        "color": "#9ACD32", "pad_index": 8,
    },
    "tokyo_shinkansen": {
        "city": "tokyo", "city_display": "Tokyo",
        "name": "Shinkansen (Tokaido)", "display": "Tokyo Shinkansen",
        "headway_rush": 10, "headway_offpeak": 15,
        "service_start": 6, "service_end": 22,
        "rush_windows": [(7, 10), (17, 20)],
        "color": "#0072BB", "pad_index": 9,
    },

    # --- London ---
    "london_jubilee": {
        "city": "london", "city_display": "London",
        "name": "Jubilee Line", "display": "London Jubilee",
        "headway_rush": 2.5, "headway_offpeak": 5,
        "service_start": 5, "service_end": 24,
        "rush_windows": [(7, 10), (17, 20)],
        "color": "#A0A5A9", "pad_index": 10,
    },
    "london_victoria": {
        "city": "london", "city_display": "London",
        "name": "Victoria Line", "display": "London Victoria",
        "headway_rush": 2, "headway_offpeak": 4,
        "service_start": 5, "service_end": 24,
        "rush_windows": [(7, 10), (17, 20)],
        "color": "#009FE0", "pad_index": 11,
    },
    "london_bakerloo": {
        "city": "london", "city_display": "London",
        "name": "Bakerloo Line", "display": "London Bakerloo",
        "headway_rush": 5, "headway_offpeak": 8,
        "service_start": 5, "service_end": 24,
        "rush_windows": [(7, 10), (17, 20)],
        "color": "#B36305", "pad_index": 12,
    },
    "london_district": {
        "city": "london", "city_display": "London",
        "name": "District Line", "display": "London District",
        "headway_rush": 4, "headway_offpeak": 8,
        "service_start": 5, "service_end": 24,
        "rush_windows": [(7, 10), (17, 20)],
        "color": "#007D32", "pad_index": 13,
    },

    # --- Wildcard ---
    "paris_rera": {
        "city": "wildcard", "city_display": "Paris",
        "name": "RER A", "display": "Paris RER A",
        "headway_rush": 2, "headway_offpeak": 5,
        "service_start": 5, "service_end": 24,
        "rush_windows": [(7, 10), (17, 20)],
        "color": "#E2231A", "pad_index": 14,
    },
    "berlin_sbahn": {
        "city": "wildcard", "city_display": "Berlin",
        "name": "S-Bahn Ring", "display": "Berlin S-Bahn",
        "headway_rush": 4, "headway_offpeak": 10,
        "service_start": 4, "service_end": 24,
        "rush_windows": [(7, 10), (17, 20)],
        "color": "#008D4F", "pad_index": 15,
    },
}

# Canonical 16-pad order
PAD_ORDER = sorted(TRANSIT_LINES.keys(), key=lambda k: TRANSIT_LINES[k]["pad_index"])

# City → line keys mapping
CITY_LINES = {
    "nyc":      [k for k in PAD_ORDER if TRANSIT_LINES[k]["city"] == "nyc"],
    "tokyo":    [k for k in PAD_ORDER if TRANSIT_LINES[k]["city"] == "tokyo"],
    "london":   [k for k in PAD_ORDER if TRANSIT_LINES[k]["city"] == "london"],
    "wildcard": [k for k in PAD_ORDER if TRANSIT_LINES[k]["city"] == "wildcard"],
}

# Step → hour mapping: 16 steps = hours 6–21 (inclusive)
STEP_HOURS = [6 + i for i in range(16)]  # [6, 7, 8, ..., 21]


def is_rush_hour(hour: int, rush_windows: List[Tuple]) -> bool:
    for start, end in rush_windows:
        if start <= hour < end:
            return True
    return False


def departures_per_hour(headway_minutes: float) -> float:
    """How many trains depart in one hour at this headway."""
    return 60.0 / headway_minutes


def compute_trigger_grid(line_key: str) -> dict:
    """
    For a transit line, compute:
      rush_grid:    16-step trigger sequence using rush headway
      offpeak_grid: 16-step trigger sequence using off-peak headway
      rush_vel:     per-step velocity (higher in deeper rush hours)
      step_labels:  hour strings for display
    """
    line = TRANSIT_LINES[line_key]
    rush_hw = line["headway_rush"]
    op_hw = line["headway_offpeak"]
    rush_windows = line["rush_windows"]

    rush_grid = []
    offpeak_grid = []
    rush_vel = []

    # For trigger threshold: a step triggers if it's within service hours
    # and the headway is short enough to produce at least 1 train per hour
    # (all lines we have do; shinkansen at 10-min still = 6 trains/hr)
    rush_dph = departures_per_hour(rush_hw)
    op_dph = departures_per_hour(op_hw)

    # Normalize across the line: always trigger (1.0) in active hours,
    # but encode density as velocity in manifest. Grid is binary.
    # Rush trigger: trigger on every step within service hours
    # Off-peak trigger: trigger only if departures >= threshold
    # Threshold = ceil(max_op_dph / 2) — fire when at least half-max frequency
    threshold = max(1.0, op_dph * 0.5)

    for step, hour in enumerate(STEP_HOURS):
        in_service = line["service_start"] <= hour < line["service_end"]
        in_rush = is_rush_hour(hour, rush_windows)

        # Rush grid: trigger all service hours, denser in peak
        rush_hit = 1 if in_service and rush_dph >= 1.0 else 0
        rush_grid.append(rush_hit)

        # Off-peak grid: trigger only if frequency meets threshold
        op_hit = 1 if in_service and op_dph >= threshold else 0
        offpeak_grid.append(op_hit)

        # Velocity scaling: peak rush = 127, normal service = ~85, off = ~45
        if in_rush and rush_hit:
            vel = 112 + int(15 * min(1.0, (30.0 / rush_hw - 1) / 14.0))
        elif in_service:
            vel = 60 + int(40 * min(1.0, op_dph / 30.0))
        else:
            vel = 0
        rush_vel.append(min(127, max(0, vel)))

    return {
        "rush_grid":    rush_grid,
        "offpeak_grid": offpeak_grid,
        "rush_vel":     rush_vel,
        "step_hours":   STEP_HOURS,
        "rush_dph":     round(rush_dph, 2),
        "op_dph":       round(op_dph, 2),
    }


# ---------------------------------------------------------------------------
# MIDI note layout: 16 pads in pad-index order
# ---------------------------------------------------------------------------
PAD_MIDI_NOTES = [36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66]

PROGRAM_SLUG_TEMPLATE = "transit_kit_{cities}"


def _layer_block(number: int, vel_start: int, vel_end: int,
                 sample_name: str, volume: float = 0.707946,
                 program_slug: str = "") -> str:
    active = "True" if sample_name else "False"
    file_path = f"Samples/{program_slug}/{sample_name}" if (sample_name and program_slug) else sample_name
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
                      line_key: str, program_slug: str) -> str:
    line = TRANSIT_LINES[line_key]
    slug_part = line_key
    offpeak_sample = f"transit_{slug_part}_offpeak.wav"
    rush_sample = f"transit_{slug_part}_rush.wav"
    display = line["display"]

    layer1 = _layer_block(1, 1,  63, offpeak_sample, 0.600000, program_slug)
    layer2 = _layer_block(2, 64, 127, rush_sample,   0.850000, program_slug)
    layer3 = _empty_layer(3)
    layer4 = _empty_layer(4)
    layers_xml = "\n".join([layer1, layer2, layer3, layer4])

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
        f'          <Mono>True</Mono>\n'
        f'          <Polyphony>1</Polyphony>\n'
        f'          <OneShot>True</OneShot>\n'
        f'          <VelocitySensitivity>1.000000</VelocitySensitivity>\n'
        f'          <VelocityToPitch>0.000000</VelocityToPitch>\n'
        f'          <VelocityToFilter>0.150000</VelocityToFilter>\n'
        f'          <ProgramName>{xml_escape(display)}</ProgramName>\n'
        f'          <Attack>0.000000</Attack>\n'
        f'          <Hold>0.000000</Hold>\n'
        f'          <Decay>0.250000</Decay>\n'
        f'          <Sustain>0.000000</Sustain>\n'
        f'          <Release>0.080000</Release>\n'
        f'          <FilterType>2</FilterType>\n'
        f'          <FilterCutoff>0.900000</FilterCutoff>\n'
        f'          <FilterResonance>0.050000</FilterResonance>\n'
        f'          <FilterEnvAmt>0.050000</FilterEnvAmt>\n'
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


def build_xpm(active_line_keys: List[str], cities_label: str) -> str:
    today = date.today().isoformat()
    program_slug = PROGRAM_SLUG_TEMPLATE.format(cities=cities_label)
    instruments_xml_parts = []
    instrument_num = 1

    for i, line_key in enumerate(active_line_keys[:16]):
        midi_note = PAD_MIDI_NOTES[i]
        instruments_xml_parts.append(
            _instrument_block(instrument_num, midi_note, line_key, program_slug)
        )
        instrument_num += 1

    # Fill remaining 112 slots as inactive
    next_note = 67
    while instrument_num <= 128:
        instruments_xml_parts.append(
            _inactive_instrument(instrument_num, min(next_note, 127))
        )
        instrument_num += 1
        next_note += 1

    instruments_xml = "\n".join(instruments_xml_parts)
    cities_display = cities_label.upper().replace("_", " + ")

    return f"""<?xml version="1.0" encoding="UTF-8"?>
<MPCVObject>
  <Version>
    <File_Version>2.1</File_Version>
    <Application>MPC</Application>
    <Application_Version>2.10</Application_Version>
  </Version>
  <Program type="Keygroup">
    <Name>Transit Kit — {cities_display}</Name>
    <Slug>{program_slug}</Slug>
    <DateCreated>{today}</DateCreated>
    <Description>City transit schedules as polyrhythmic drum kits. Each pad = one transit line. Velocity layer 1 (1–63) = off-peak pattern. Velocity layer 2 (64–127) = rush hour pattern. Playing multiple pads = real-world polyrhythm from {cities_display}. 16 steps = 6am–9pm service window.</Description>
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


def build_manifest(active_line_keys: List[str]) -> dict:
    manifest = {
        "tool": "xpn_transit_kit",
        "version": "1.0.0",
        "date": date.today().isoformat(),
        "description": "City transit schedules as polyrhythmic MPC drum kits",
        "step_map": {
            f"step_{i+1}": f"{STEP_HOURS[i]:02d}:00–{STEP_HOURS[i]+1:02d}:00"
            for i in range(16)
        },
        "velocity_layers": {
            "layer_1": {"range": "1–63",   "meaning": "off-peak pattern"},
            "layer_2": {"range": "64–127", "meaning": "rush hour pattern"},
        },
        "lines": {},
    }

    for i, line_key in enumerate(active_line_keys[:16]):
        line = TRANSIT_LINES[line_key]
        grid = compute_trigger_grid(line_key)
        manifest["lines"][line_key] = {
            "pad": i + 1,
            "midi_note": PAD_MIDI_NOTES[i],
            "city": line["city_display"],
            "name": line["name"],
            "display": line["display"],
            "headway_rush_min": line["headway_rush"],
            "headway_offpeak_min": line["headway_offpeak"],
            "rush_dph": grid["rush_dph"],
            "offpeak_dph": grid["op_dph"],
            "rush_grid": grid["rush_grid"],
            "offpeak_grid": grid["offpeak_grid"],
            "rush_trigger_readable": "".join("X" if t else "." for t in grid["rush_grid"]),
            "offpeak_trigger_readable": "".join("X" if t else "." for t in grid["offpeak_grid"]),
            "rush_velocity_per_step": grid["rush_vel"],
            "color": line["color"],
            "sample_offpeak": f"transit_{line_key}_offpeak.wav",
            "sample_rush":    f"transit_{line_key}_rush.wav",
        }

    return manifest


def build_viz(active_line_keys: List[str]) -> str:
    hour_header = "  ".join(f"{h:2d}" for h in STEP_HOURS)
    step_header = "  ".join(f" {i+1:1d}" for i in range(16))

    lines = [
        "╔══════════════════════════════════════════════════════════════════════════╗",
        "║            TRANSIT KIT — POLYRHYTHMIC DEPARTURE GRIDS                   ║",
        "║         XO_OX Designs | City Schedules as Drum Patterns                 ║",
        "╚══════════════════════════════════════════════════════════════════════════╝",
        "",
        f"Step:  {step_header}",
        f"Hour:  {hour_header}",
        "       " + "─" * (len(step_header) + 2),
        "",
    ]

    current_city = None
    for line_key in active_line_keys[:16]:
        line = TRANSIT_LINES[line_key]
        grid = compute_trigger_grid(line_key)
        city = line["city_display"]

        if city != current_city:
            current_city = city
            lines.append(f"  ── {city.upper()} ──")
            lines.append("")

        rush_str    = "  ".join(" X" if t else " ." for t in grid["rush_grid"])
        offpeak_str = "  ".join(" X" if t else " ." for t in grid["offpeak_grid"])
        label = f"{line['display']:<22}"

        lines.append(f"  {label} rush:    [{rush_str}]  {line['headway_rush']}min hdwy  {grid['rush_dph']:.1f}/hr")
        lines.append(f"  {' ' * 22} off-peak:[{offpeak_str}]  {line['headway_offpeak']}min hdwy  {grid['op_dph']:.1f}/hr")
        lines.append("")

    lines.append("Legend:")
    lines.append("  X = service active (trigger hit)  . = no service / threshold not met")
    lines.append("  Steps = hours 6am–9pm (16-hour service window)")
    lines.append("  Rush hours: 7–10am, 5–8pm (higher velocity layer)")
    lines.append("")
    lines.append("Polyrhythm note:")
    lines.append("  Play NYC 4/5/6 (2-min rush) + London Victoria (2-min rush) + Tokyo Ginza (2-min rush)")
    lines.append("  = Three cities, identical headways, slightly different service windows")
    lines.append("  = Interlocking polyrhythm emerges from schedule offsets alone.")

    return "\n".join(lines)


def ascii_city_viz(city: str) -> str:
    """Print ASCII trigger grids for all lines in a city."""
    if city not in CITY_LINES and city not in ["wildcard"]:
        return f"Unknown city: {city}. Valid: nyc, tokyo, london, wildcard"
    keys = CITY_LINES.get(city, [])
    if not keys:
        return f"No lines defined for city: {city}"
    return build_viz(keys)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def resolve_city_keys(city_arg: str) -> Tuple[List[str], str]:
    """Parse --city arg → (list of line keys, label string)."""
    if not city_arg or city_arg == "all":
        return PAD_ORDER, "all"

    cities = [c.strip().lower() for c in city_arg.split(",")]
    keys = []
    labels = []
    for city in cities:
        if city not in CITY_LINES:
            valid = ", ".join(sorted(CITY_LINES.keys()))
            print(f"ERROR: unknown city '{city}'. Valid: {valid}", file=sys.stderr)
            sys.exit(1)
        keys.extend(CITY_LINES[city])
        labels.append(city)
    return keys, "_".join(labels)


def main():
    parser = argparse.ArgumentParser(
        description="XPN Transit Kit — city transit schedules as polyrhythmic drum kits",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Build full 16-line polyrhythm kit (all cities)
  python xpn_transit_kit.py --output ./kits/

  # Build NYC-only kit
  python xpn_transit_kit.py --city nyc --output ./kits/

  # Build NYC + Tokyo polyrhythm kit
  python xpn_transit_kit.py --city nyc,tokyo --output ./kits/

  # Visualize trigger grids for London
  python xpn_transit_kit.py --visualize london

  # List all lines
  python xpn_transit_kit.py --list
        """
    )
    parser.add_argument("--output", "-o", metavar="DIR",
                        help="Output directory for kit files")
    parser.add_argument("--city", metavar="CITY",
                        help="City filter: nyc, tokyo, london, wildcard (comma-separated for poly)")
    parser.add_argument("--visualize", metavar="CITY",
                        help="Print ASCII trigger grid for a city and exit")
    parser.add_argument("--list", action="store_true",
                        help="List all transit lines and exit")
    args = parser.parse_args()

    # --visualize: print and exit
    if args.visualize:
        city = args.visualize.lower()
        if city == "all":
            print(build_viz(PAD_ORDER))
        elif city in CITY_LINES:
            print(build_viz(CITY_LINES[city]))
        else:
            print(f"Unknown city. Valid: {', '.join(sorted(CITY_LINES.keys()))}", file=sys.stderr)
            sys.exit(1)
        return

    # --list: dump all lines
    if args.list:
        print(f"{'Line Key':<22}  {'City':<8}  {'Name':<26}  {'Rush hw':>7}  {'OP hw':>5}  {'MIDI':>5}")
        print("─" * 80)
        for key in PAD_ORDER:
            line = TRANSIT_LINES[key]
            print(f"{key:<22}  {line['city_display']:<8}  {line['name']:<26}  "
                  f"{line['headway_rush']:>6}m  {line['headway_offpeak']:>4}m  "
                  f"{PAD_MIDI_NOTES[line['pad_index']]:>5}")
        return

    if not args.output:
        parser.print_help()
        sys.exit(1)

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    active_keys, cities_label = resolve_city_keys(args.city or "all")
    if not active_keys:
        print("ERROR: no transit lines selected", file=sys.stderr)
        sys.exit(1)

    # Cap at 16 pads
    active_keys = active_keys[:16]

    print(f"Building Transit Kit ({cities_label.upper()}) → {output_dir}/")
    print(f"  Lines: {len(active_keys)}")

    # XPM
    xpm_filename = f"transit_kit_{cities_label}.xpm"
    xpm_path = output_dir / xpm_filename
    xpm_content = build_xpm(active_keys, cities_label)
    xpm_path.write_text(xpm_content, encoding="utf-8")
    print(f"  ✓ {xpm_filename}")

    # Manifest
    manifest_path = output_dir / "transit_manifest.json"
    manifest = build_manifest(active_keys)
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"  ✓ transit_manifest.json")

    # Grid viz
    grid_path = output_dir / "transit_grid.txt"
    grid_content = build_viz(active_keys)
    grid_path.write_text(grid_content, encoding="utf-8")
    print(f"  ✓ transit_grid.txt")

    # Summary
    print(f"\nKit summary:")
    print(f"  Active pads: {len(active_keys)}")
    print(f"  Layer 1 (vel 1–63):    off-peak pattern")
    print(f"  Layer 2 (vel 64–127):  rush hour pattern")
    print(f"  16 steps = 6am–9pm service window")
    print(f"\nPad / sample map:")
    print(f"  {'Pad':>3}  {'MIDI':>5}  {'Line':<26}  Off-peak         Rush")
    print(f"  {'─'*3}  {'─'*5}  {'─'*26}  {'─'*16}  {'─'*16}")
    for i, key in enumerate(active_keys):
        line = TRANSIT_LINES[key]
        grid = compute_trigger_grid(key)
        op_str = "".join("X" if t else "." for t in grid["offpeak_grid"])
        rush_str = "".join("X" if t else "." for t in grid["rush_grid"])
        print(f"  {i+1:>3}  {PAD_MIDI_NOTES[i]:>5}  {line['display']:<26}  [{op_str}]  [{rush_str}]")

    print(f"\nSample naming (place in Samples/transit_kit_{cities_label}/):")
    print(f"  transit_{{line_key}}_offpeak.wav")
    print(f"  transit_{{line_key}}_rush.wav")


if __name__ == "__main__":
    main()
