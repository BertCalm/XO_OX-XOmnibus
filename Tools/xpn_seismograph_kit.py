#!/usr/bin/env python3
"""
XPN Seismograph Kit — XO_OX Designs
USGS earthquake data → MPC drum kit.

Each earthquake event maps to one pad hit:
  Magnitude  → MIDI velocity  (M5.0→40, M7.0→90, M9.0→127, linear interpolation)
  Depth      → voice / pad row:
                 0–10 km   = Kick   (shallow, surface-rupturing)
                10–35 km   = Snare  (crustal mid-range)
                35–70 km   = Tom    (deep crustal / upper mantle)
                70+ km     = Sub    (subduction zone deep slab)
  Time delta → pad column (which of 16 pads receives the hit)
               Temporal gaps between events are normalized across the 16-pad grid.

3 Hardcoded datasets (USGS format, plausible real values):
  tohoku     2011 Tohoku M9.0 — mainshock + 20 aftershock events
  haiti      2010 Haiti M7.0  — mainshock + 20 aftershock events
  northridge 1994 Northridge M6.7 — 20 events from the LA Basin sequence

CSV input (--csv FILE):
  Columns: time,latitude,longitude,depth,mag
  time must be ISO-8601 (e.g. 2011-03-11T05:46:24Z)

Output (XPN ZIP + text report):
  seismograph_{dataset}.xpn   — MPC drum program
  earthquake_report.txt        — event list with pad assignments

CLI:
  python xpn_seismograph_kit.py --dataset tohoku --output ./out/
  python xpn_seismograph_kit.py --dataset haiti  --output ./out/
  python xpn_seismograph_kit.py --dataset northridge --output ./out/
  python xpn_seismograph_kit.py --csv my_quakes.csv --output ./out/

XPN golden rules (always applied):
  KeyTrack  = True
  RootNote  = 0
  VelStart  = 0 on empty layers
"""

import argparse
import csv
import json
import struct
import sys
import zipfile
from datetime import datetime, timezone
from pathlib import Path
from xml.sax.saxutils import escape as xml_escape

# =============================================================================
# HARDCODED EARTHQUAKE DATASETS
# =============================================================================
# Fields: time (ISO-8601 UTC), latitude, longitude, depth_km, magnitude
# Tohoku sequence: mainshock 2011-03-11, 20 significant aftershocks
DATASETS = {
    "tohoku": {
        "name": "2011 Tohoku (M9.0)",
        "region": "Honshu, Japan",
        "description": "Megathrust event off the Pacific coast of Tohoku. "
                       "Triggered a devastating tsunami and the Fukushima nuclear crisis.",
        "events": [
            # time                    lat      lon      depth  mag
            ("2011-03-11T05:46:24Z",  38.297,  142.373,  29.0, 9.0),  # mainshock
            ("2011-03-11T06:15:40Z",  36.281,  141.111,  42.0, 7.7),
            ("2011-03-11T06:25:50Z",  38.058,  144.590,  18.0, 7.6),
            ("2011-03-11T06:47:46Z",  37.786,  141.649,  40.0, 7.5),
            ("2011-03-11T07:15:31Z",  36.407,  141.457,  37.0, 7.3),
            ("2011-03-11T08:19:24Z",  37.221,  141.476,  35.0, 6.8),
            ("2011-03-11T09:36:07Z",  38.905,  142.335,  25.0, 6.6),
            ("2011-03-11T10:02:59Z",  39.168,  142.588,  30.0, 6.4),
            ("2011-03-11T11:36:01Z",  37.552,  143.290,  22.0, 6.5),
            ("2011-03-12T02:46:44Z",  36.979,  138.596,  10.0, 6.7),
            ("2011-03-12T04:48:27Z",  40.408,  139.240,  14.0, 6.4),
            ("2011-03-13T07:24:45Z",  38.120,  143.640,  20.0, 6.2),
            ("2011-03-14T06:13:50Z",  37.970,  142.620,  18.0, 6.3),
            ("2011-03-15T22:31:00Z",  34.988,  138.526,  14.0, 6.4),
            ("2011-03-19T18:56:01Z",  36.748,  140.636,  11.0, 6.1),
            ("2011-03-22T07:19:44Z",  37.040,  138.498,  12.0, 5.8),
            ("2011-03-23T06:54:04Z",  37.302,  141.220,  40.0, 5.9),
            ("2011-04-07T14:32:42Z",  38.276,  141.586,  42.0, 7.1),
            ("2011-04-11T08:08:00Z",  36.945,  140.672,  13.0, 6.7),
            ("2011-04-12T14:07:42Z",  37.051,  140.624,  11.0, 6.0),
            ("2011-07-10T00:57:16Z",  38.043,  143.275,  33.0, 7.0),  # M7 aftershock
        ],
    },
    "haiti": {
        "name": "2010 Haiti (M7.0)",
        "region": "Port-au-Prince, Haiti",
        "description": "Blind strike-slip rupture on the Enriquillo-Plantain Garden fault. "
                       "One of the deadliest urban earthquakes in modern history.",
        "events": [
            ("2010-01-12T21:53:10Z",  18.443, -72.571,  13.0, 7.0),  # mainshock
            ("2010-01-12T22:00:28Z",  18.382, -72.695,  10.0, 5.9),
            ("2010-01-12T22:15:44Z",  18.397, -72.579,  12.0, 5.5),
            ("2010-01-12T22:26:44Z",  18.360, -72.543,  10.0, 5.7),
            ("2010-01-13T01:42:29Z",  18.476, -72.654,  10.0, 5.5),
            ("2010-01-13T07:23:54Z",  18.472, -72.634,  10.0, 5.6),
            ("2010-01-13T21:49:18Z",  18.440, -72.616,  10.0, 5.5),
            ("2010-01-14T06:29:02Z",  18.396, -72.543,  10.0, 5.5),
            ("2010-01-15T05:23:15Z",  18.447, -72.585,  10.0, 5.5),
            ("2010-01-16T10:31:53Z",  18.463, -72.592,  10.0, 5.5),
            ("2010-01-17T03:57:28Z",  18.387, -72.617,  10.0, 5.6),
            ("2010-01-20T11:04:00Z",  18.442, -72.594,  10.0, 5.9),
            ("2010-01-20T11:52:42Z",  18.450, -72.601,  10.0, 5.5),
            ("2010-02-09T05:42:26Z",  18.467, -72.554,  10.0, 5.8),
            ("2010-02-17T21:22:54Z",  18.458, -72.566,  10.0, 5.5),
            ("2010-03-20T06:51:17Z",  18.511, -72.582,  10.0, 5.6),
            ("2010-06-05T02:26:43Z",  18.425, -72.544,  10.0, 5.7),
            ("2010-11-30T08:25:25Z",  18.409, -72.551,  10.0, 5.6),
            ("2011-01-12T11:12:37Z",  18.439, -72.569,  10.0, 5.5),
            ("2011-05-21T03:40:12Z",  18.418, -72.573,  10.0, 5.7),
            ("2011-08-23T10:02:19Z",  18.467, -72.573,  10.0, 5.5),
        ],
    },
    "northridge": {
        "name": "1994 Northridge (M6.7)",
        "region": "Los Angeles Basin, California",
        "description": "Blind thrust fault rupture beneath the San Fernando Valley. "
                       "The costliest earthquake in US history at the time.",
        "events": [
            ("1994-01-17T12:30:55Z",  34.213, -118.537,  17.5, 6.7),  # mainshock
            ("1994-01-17T12:40:55Z",  34.244, -118.576,  19.0, 5.6),
            ("1994-01-17T13:06:28Z",  34.262, -118.543,  17.0, 5.5),
            ("1994-01-17T14:46:47Z",  34.198, -118.519,  18.0, 5.9),
            ("1994-01-17T17:34:48Z",  34.280, -118.591,  16.5, 5.6),
            ("1994-01-17T20:58:49Z",  34.214, -118.528,  17.0, 5.5),
            ("1994-01-18T01:51:56Z",  34.233, -118.561,  15.0, 5.5),
            ("1994-01-18T21:03:41Z",  34.156, -118.464,  14.0, 5.5),
            ("1994-01-19T21:59:38Z",  34.269, -118.565,  18.5, 5.8),
            ("1994-01-20T17:29:59Z",  34.195, -118.543,  16.0, 5.7),
            ("1994-01-22T04:00:18Z",  34.202, -118.530,  17.0, 5.5),
            ("1994-01-24T09:29:08Z",  34.230, -118.540,  17.5, 5.6),
            ("1994-01-27T04:18:50Z",  34.207, -118.555,  15.5, 5.5),
            ("1994-01-29T03:09:25Z",  34.262, -118.553,  18.0, 5.5),
            ("1994-02-01T01:12:16Z",  34.196, -118.519,  16.0, 5.6),
            ("1994-02-06T05:30:38Z",  34.217, -118.527,  17.0, 5.5),
            ("1994-02-13T08:41:51Z",  34.237, -118.558,  16.5, 5.5),
            ("1994-02-15T04:32:44Z",  34.209, -118.543,  17.0, 5.6),
            ("1994-03-08T07:31:22Z",  34.200, -118.531,  16.0, 5.5),
            ("1994-04-09T07:17:56Z",  34.225, -118.569,  15.5, 5.6),
            ("1994-12-06T01:19:56Z",  34.154, -118.462,  15.0, 5.9),
        ],
    },
}

# =============================================================================
# PAD / VOICE DEFINITIONS
# =============================================================================

# 16 pads, GM-convention MIDI notes
PAD_MIDI_NOTES = [36, 38, 42, 46, 39, 41, 43, 49,
                  50, 52, 54, 56, 58, 60, 62, 64]

# Voice assignment by depth
VOICE_NAMES = {
    "kick":  "Kick",
    "snare": "Snare",
    "tom":   "Tom",
    "sub":   "Sub",
}

VOICE_ORDER = ["kick", "snare", "tom", "sub"]   # pad rows 0-3; 4 pads per voice
VOICE_MIDI_OFFSETS = {                           # base MIDI note per voice row
    "kick":  36,
    "snare": 38,
    "tom":   41,
    "sub":   43,
}


def depth_to_voice(depth_km: float) -> str:
    if depth_km < 10.0:
        return "kick"
    elif depth_km < 35.0:
        return "snare"
    elif depth_km < 70.0:
        return "tom"
    else:
        return "sub"


def mag_to_velocity(magnitude: float) -> int:
    """Linear interpolation: M5.0→40, M7.0→90, M9.0→127, clamp 1-127."""
    if magnitude <= 5.0:
        return 40
    elif magnitude <= 7.0:
        # 40 at 5.0 → 90 at 7.0
        return int(40 + (magnitude - 5.0) / 2.0 * 50)
    else:
        # 90 at 7.0 → 127 at 9.0
        v = int(90 + (magnitude - 7.0) / 2.0 * 37)
        return min(v, 127)


def parse_iso_time(s: str) -> datetime:
    """Parse ISO-8601 UTC timestamp. Handles trailing 'Z'."""
    s = s.strip().rstrip("Z")
    for fmt in ("%Y-%m-%dT%H:%M:%S", "%Y-%m-%dT%H:%M:%S.%f", "%Y-%m-%d %H:%M:%S"):
        try:
            return datetime.strptime(s, fmt).replace(tzinfo=timezone.utc)
        except ValueError:
            pass
    raise ValueError(f"Cannot parse timestamp: {s!r}")


# =============================================================================
# EVENT PROCESSING
# =============================================================================

def load_csv(csv_path: str):
    """Load events from CSV file. Expected columns: time,latitude,longitude,depth,mag"""
    events = []
    with open(csv_path, newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                events.append((
                    row["time"].strip(),
                    float(row["latitude"]),
                    float(row["longitude"]),
                    float(row["depth"]),
                    float(row["mag"]),
                ))
            except (KeyError, ValueError) as e:
                print(f"  WARNING: skipping malformed row {row}: {e}", file=sys.stderr)
    if not events:
        print("ERROR: no valid events found in CSV", file=sys.stderr)
        sys.exit(1)
    return events, "CSV", csv_path, "Custom CSV dataset"


def process_events(raw_events):
    """
    Convert raw (time_str, lat, lon, depth, mag) tuples into pad assignments.

    Returns list of dicts:
      index, time_str, lat, lon, depth_km, magnitude,
      voice, velocity, pad_col (0-3), pad_index (0-15),
      midi_note, time_seconds_from_start
    """
    # Parse timestamps and compute seconds from first event
    parsed = []
    for (time_str, lat, lon, depth, mag) in raw_events:
        t = parse_iso_time(time_str)
        parsed.append((t, time_str, lat, lon, depth, mag))

    # Sort chronologically
    parsed.sort(key=lambda x: x[0])

    t0 = parsed[0][0]
    max_delta = (parsed[-1][0] - t0).total_seconds()
    if max_delta == 0:
        max_delta = 1.0

    results = []
    for i, (t, time_str, lat, lon, depth, mag) in enumerate(parsed):
        dt_seconds = (t - t0).total_seconds()
        # Normalize time to 0.0–1.0, map to 4 columns (0-3) within voice's pad group
        normalized = dt_seconds / max_delta
        pad_col = min(int(normalized * 4), 3)   # column 0-3 within voice group

        voice = depth_to_voice(depth)
        velocity = mag_to_velocity(mag)

        # Pad index: voice determines row (0=kick,1=snare,2=tom,3=sub)
        # Each voice occupies 4 pads in the 16-pad grid
        voice_row = VOICE_ORDER.index(voice)
        pad_index = voice_row * 4 + pad_col
        midi_note = PAD_MIDI_NOTES[pad_index]

        results.append({
            "index":           i + 1,
            "time_str":        time_str,
            "lat":             lat,
            "lon":             lon,
            "depth_km":        depth,
            "magnitude":       mag,
            "voice":           voice,
            "velocity":        velocity,
            "pad_col":         pad_col + 1,
            "pad_index":       pad_index + 1,
            "midi_note":       midi_note,
            "dt_seconds":      dt_seconds,
        })

    return results


# =============================================================================
# XPN / XPM GENERATION
# =============================================================================

def _silent_wav() -> bytes:
    """Minimal valid 44100Hz mono 16-bit WAV, ~0.1s silence (placeholder)."""
    sample_rate = 44100
    num_samples = 4410          # 0.1s
    num_channels = 1
    bits = 16
    byte_rate = sample_rate * num_channels * bits // 8
    block_align = num_channels * bits // 8
    data_size = num_samples * block_align

    header = struct.pack(
        "<4sI4s4sIHHIIHH4sI",
        b"RIFF",
        36 + data_size,
        b"WAVE",
        b"fmt ",
        16,             # chunk size
        1,              # PCM
        num_channels,
        sample_rate,
        byte_rate,
        block_align,
        bits,
        b"data",
        data_size,
    )
    return header + b"\x00" * data_size


def _layer_block(number: int, vel_start: int, vel_end: int,
                 sample_name: str, volume: float, program_slug: str = "") -> str:
    if not sample_name:
        return (
            f'            <Layer number="{number}">\n'
            f'              <Active>False</Active>\n'
            f'              <Volume>{volume:.6f}</Volume>\n'
            f'              <Pan>0.500000</Pan>\n'
            f'              <Pitch>0.000000</Pitch>\n'
            f'              <VelStart>0</VelStart>\n'
            f'              <VelEnd>0</VelEnd>\n'
            f'              <RootNote>0</RootNote>\n'
            f'              <KeyTrack>True</KeyTrack>\n'
            f'              <SampleName></SampleName>\n'
            f'              <SampleFile></SampleFile>\n'
            f'              <File></File>\n'
            f'              <SliceIndex>128</SliceIndex>\n'
            f'              <Direction>0</Direction>\n'
            f'              <Offset>0</Offset>\n'
            f'              <SliceStart>0</SliceStart>\n'
            f'              <SliceEnd>0</SliceEnd>\n'
            f'              <SliceLoopStart>0</SliceLoopStart>\n'
            f'              <SliceLoop>0</SliceLoop>\n'
            f'            </Layer>'
        )
    file_path = f"Samples/{xml_escape(sample_name)}"
    return (
        f'            <Layer number="{number}">\n'
        f'              <Active>True</Active>\n'
        f'              <Volume>{volume:.6f}</Volume>\n'
        f'              <Pan>0.500000</Pan>\n'
        f'              <Pitch>0.000000</Pitch>\n'
        f'              <VelStart>{vel_start}</VelStart>\n'
        f'              <VelEnd>{vel_end}</VelEnd>\n'
        f'              <RootNote>0</RootNote>\n'
        f'              <KeyTrack>True</KeyTrack>\n'
        f'              <SampleName>{xml_escape(sample_name)}</SampleName>\n'
        f'              <SampleFile>{xml_escape(sample_name)}</SampleFile>\n'
        f'              <File>{xml_escape(file_path)}</File>\n'
        f'              <SliceIndex>128</SliceIndex>\n'
        f'              <Direction>0</Direction>\n'
        f'              <Offset>0</Offset>\n'
        f'              <SliceStart>0</SliceStart>\n'
        f'              <SliceEnd>0</SliceEnd>\n'
        f'              <SliceLoopStart>0</SliceLoopStart>\n'
        f'              <SliceLoop>0</SliceLoop>\n'
        f'            </Layer>'
    )


def _empty_layer(number: int) -> str:
    return _layer_block(number, 0, 0, "", 0.707946)


def _instrument_block(instr_num: int, midi_note: int, voice: str,
                      col: int, slug: str) -> str:
    """
    Build one instrument block. Voice groups determine 4 velocity layers
    based on sub-group labels (shallow/intermediate/deep/very-deep).
    Each layer uses the same sample (placeholder WAV) but differs in
    VelStart/VelEnd so producers can swap in real samples per-layer.
    """
    label = VOICE_NAMES[voice]
    layer_labels = ["shallow", "intermediate", "deep", "very-deep"]
    sample_name = f"seismograph_{voice}_col{col}_{layer_labels[0]}.wav"

    # 4 velocity layers across the full range — each sub-group occupies a quarter
    layer1 = _layer_block(1,   1,  31, f"seismograph_{voice}_col{col}_{layer_labels[0]}.wav",
                          0.600000, slug)
    layer2 = _layer_block(2,  32,  63, f"seismograph_{voice}_col{col}_{layer_labels[1]}.wav",
                          0.707946, slug)
    layer3 = _layer_block(3,  64,  95, f"seismograph_{voice}_col{col}_{layer_labels[2]}.wav",
                          0.850000, slug)
    layer4 = _layer_block(4,  96, 127, f"seismograph_{voice}_col{col}_{layer_labels[3]}.wav",
                          1.000000, slug)
    layers_xml = "\n".join([layer1, layer2, layer3, layer4])

    # Voice-specific settings (per-voice smart defaults)
    if voice == "kick":
        vel_to_pitch  = "0.050000"
        vel_to_filter = "0.100000"
        mute_group    = "0"
        one_shot      = "True"
        polyphony     = "1"
    elif voice == "snare":
        vel_to_pitch  = "0.000000"
        vel_to_filter = "0.300000"
        mute_group    = "0"
        one_shot      = "True"
        polyphony     = "1"
    elif voice == "tom":
        vel_to_pitch  = "0.020000"
        vel_to_filter = "0.150000"
        mute_group    = "0"
        one_shot      = "True"
        polyphony     = "1"
    else:  # sub
        vel_to_pitch  = "0.030000"
        vel_to_filter = "0.050000"
        mute_group    = "0"
        one_shot      = "False"
        polyphony     = "2"

    return (
        f'        <Instrument number="{instr_num}">\n'
        f'          <Active>True</Active>\n'
        f'          <Volume>1.000000</Volume>\n'
        f'          <Pan>0.500000</Pan>\n'
        f'          <Pitch>0.000000</Pitch>\n'
        f'          <MidiNote>{midi_note}</MidiNote>\n'
        f'          <ZonePlay>1</ZonePlay>\n'
        f'          <MuteGroup>{mute_group}</MuteGroup>\n'
        f'          <MuteTarget>0</MuteTarget>\n'
        f'          <VoiceOverlap>0</VoiceOverlap>\n'
        f'          <Mono>True</Mono>\n'
        f'          <Polyphony>{polyphony}</Polyphony>\n'
        f'          <OneShot>{one_shot}</OneShot>\n'
        f'          <VelocitySensitivity>1.000000</VelocitySensitivity>\n'
        f'          <VelocityToPitch>{vel_to_pitch}</VelocityToPitch>\n'
        f'          <VelocityToFilter>{vel_to_filter}</VelocityToFilter>\n'
        f'          <ProgramName>{xml_escape(label)} Col {col}</ProgramName>\n'
        f'          <Attack>0.000000</Attack>\n'
        f'          <Hold>0.000000</Hold>\n'
        f'          <Decay>0.350000</Decay>\n'
        f'          <Sustain>0.000000</Sustain>\n'
        f'          <Release>0.100000</Release>\n'
        f'          <FilterType>2</FilterType>\n'
        f'          <FilterCutoff>0.880000</FilterCutoff>\n'
        f'          <FilterResonance>0.050000</FilterResonance>\n'
        f'          <FilterEnvAmt>0.080000</FilterEnvAmt>\n'
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


def build_xpm(dataset_name: str, slug: str, description: str) -> str:
    from datetime import date
    today = date.today().isoformat()
    instruments_xml_parts = []

    # 16 active instruments: 4 voices × 4 columns
    instr_num = 1
    for voice_idx, voice in enumerate(VOICE_ORDER):
        for col in range(1, 5):
            pad_index = voice_idx * 4 + (col - 1)
            midi_note = PAD_MIDI_NOTES[pad_index]
            instruments_xml_parts.append(
                _instrument_block(instr_num, midi_note, voice, col, slug)
            )
            instr_num += 1

    # Fill remaining 112 slots as inactive
    next_note = 67
    while instr_num <= 128:
        instruments_xml_parts.append(
            _inactive_instrument(instr_num, min(next_note, 127))
        )
        instr_num += 1
        next_note += 1

    instruments_xml = "\n".join(instruments_xml_parts)

    return f"""<?xml version="1.0" encoding="UTF-8"?>
<MPCVObject>
  <Version>
    <File_Version>2.1</File_Version>
    <Application>MPC</Application>
    <Application_Version>2.10</Application_Version>
  </Version>
  <Program type="Drum">
    <Name>Seismograph — {xml_escape(dataset_name)}</Name>
    <Slug>{xml_escape(slug)}</Slug>
    <DateCreated>{today}</DateCreated>
    <Description>{xml_escape(description)} Pad layout: rows = depth zones (Kick=0-10km / Snare=10-35km / Tom=35-70km / Sub=70km+), columns = temporal spread (Col1=early / Col4=late). Velocity = magnitude (M5.0=40 / M7.0=90 / M9.0=127). 4 layers per pad: shallow / intermediate / deep / very-deep sub-groups.</Description>
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


def build_report(events_processed, dataset_name: str, region: str,
                 dataset_description: str, slug: str) -> str:
    lines = [
        "=" * 72,
        f"  XPN SEISMOGRAPH KIT — EARTHQUAKE REPORT",
        f"  Dataset:     {dataset_name}",
        f"  Region:      {region}",
        f"  Description: {dataset_description}",
        f"  Kit slug:    {slug}",
        f"  Generated:   {__import__('datetime').date.today().isoformat()}",
        "=" * 72,
        "",
        "PAD LAYOUT",
        "  Rows  = Depth zones:",
        "    Row 1: Kick  — 0–10 km    (shallow / surface rupture)",
        "    Row 2: Snare — 10–35 km   (mid-crustal)",
        "    Row 3: Tom   — 35–70 km   (deep crustal / upper mantle)",
        "    Row 4: Sub   — 70+ km     (subduction deep slab)",
        "  Cols  = Temporal spread (1=early in sequence, 4=late)",
        "  Vel   = Magnitude (M5.0→40, M7.0→90, M9.0→127)",
        "  Layers = Sub-group depth within each voice:",
        "    L1 vel 1–31   shallow     (top of depth range for that voice)",
        "    L2 vel 32–63  intermediate",
        "    L3 vel 64–95  deep",
        "    L4 vel 96–127 very-deep   (bottom of depth range for that voice)",
        "",
        f"{'#':>4}  {'Time (UTC)':<22}  {'Mag':>4}  {'Depth':>6}  {'Voice':<6}  "
        f"{'Vel':>3}  {'Pad':>3}  {'MIDI':>4}  Notes",
        "-" * 72,
    ]

    for ev in events_processed:
        notes = ""
        if ev["magnitude"] >= 7.0:
            notes = "<-- MAJOR"
        elif ev["magnitude"] >= 6.0:
            notes = "<-- strong"
        lines.append(
            f"{ev['index']:>4}  {ev['time_str']:<22}  "
            f"M{ev['magnitude']:4.1f}  {ev['depth_km']:5.1f}km  "
            f"{ev['voice']:<6}  {ev['velocity']:>3}  "
            f"P{ev['pad_index']:>2}  {ev['midi_note']:>4}  {notes}"
        )

    lines += [
        "",
        "SAMPLE NAMING CONVENTION",
        "  Place WAV files in Samples/ directory of the ZIP.",
        "  Each pad has 4 velocity layers × 4 depth sub-groups:",
        "",
        "  seismograph_{voice}_col{1-4}_{sublabel}.wav",
        "  Subgroup labels: shallow, intermediate, deep, very-deep",
        "",
        "  Example — kick voice, column 1:",
        "    seismograph_kick_col1_shallow.wav      (vel 1–31)",
        "    seismograph_kick_col1_intermediate.wav (vel 32–63)",
        "    seismograph_kick_col1_deep.wav         (vel 64–95)",
        "    seismograph_kick_col1_very-deep.wav    (vel 96–127)",
        "",
        "  Total: 4 voices × 4 cols × 4 layers = 64 WAV files",
        "",
        "SOUND DESIGN NOTES",
        "  -- Kick (shallow 0-10km): hard transient attack, full rumble body",
        "  -- Snare (10-35km): crack + rattle, mid presence",
        "  -- Tom (35-70km): deep resonant thud, mantle weight",
        "  -- Sub (70km+): sub-bass drone, tectonic pressure",
        "  -- Very-deep layers (vel 96-127) should have maximum body and sub-content",
        "  -- Shallow layers (vel 1-31) can be ghost hits or texture",
    ]

    return "\n".join(lines)


# =============================================================================
# ZIP BUILDER
# =============================================================================

def write_zip(slug: str, xpm_xml: str, report_txt: str,
              manifest: dict, output_dir: Path) -> Path:
    """Package as XPN ZIP."""
    output_dir.mkdir(parents=True, exist_ok=True)
    zip_path = output_dir / f"{slug}.xpn"
    wav_bytes = _silent_wav()

    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
        # XPM program
        zf.writestr(f"Programs/{slug}.xpm", xpm_xml)

        # Placeholder WAVs for all 64 sample slots
        layer_labels = ["shallow", "intermediate", "deep", "very-deep"]
        for voice in VOICE_ORDER:
            for col in range(1, 5):
                for label in layer_labels:
                    fname = f"Samples/seismograph_{voice}_col{col}_{label}.wav"
                    zf.writestr(fname, wav_bytes)

        # Manifest
        zf.writestr("earthquake_manifest.json", json.dumps(manifest, indent=2))

        # Report
        zf.writestr("earthquake_report.txt", report_txt)

    return zip_path


# =============================================================================
# CLI
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Seismograph Kit — USGS earthquake data to MPC drum kit",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Datasets:
  tohoku      2011 Tohoku M9.0 — megathrust off Honshu (21 events)
  haiti       2010 Haiti M7.0  — Port-au-Prince sequence (21 events)
  northridge  1994 Northridge M6.7 — LA Basin (21 events)

Examples:
  python xpn_seismograph_kit.py --dataset tohoku --output ./out/
  python xpn_seismograph_kit.py --dataset haiti --output ./out/
  python xpn_seismograph_kit.py --dataset northridge --output ./out/
  python xpn_seismograph_kit.py --csv my_quakes.csv --output ./out/
  python xpn_seismograph_kit.py --list
        """
    )
    parser.add_argument("--dataset", metavar="NAME",
                        help="Hardcoded dataset: tohoku, haiti, northridge")
    parser.add_argument("--csv", metavar="FILE",
                        help="CSV file with columns: time,latitude,longitude,depth,mag")
    parser.add_argument("--output", "-o", metavar="DIR",
                        help="Output directory")
    parser.add_argument("--list", action="store_true",
                        help="List available datasets and exit")
    args = parser.parse_args()

    if args.list:
        print("Available hardcoded datasets:")
        print(f"  {'Name':<14}  {'Events':>6}  Description")
        print("  " + "-" * 60)
        for key, ds in DATASETS.items():
            n = len(ds["events"])
            print(f"  {key:<14}  {n:>6}  {ds['name']} — {ds['region']}")
        return

    if not args.output:
        parser.print_help()
        sys.exit(1)

    if args.csv and args.dataset:
        print("ERROR: specify --dataset OR --csv, not both", file=sys.stderr)
        sys.exit(1)

    if args.csv:
        raw_events, ds_key, ds_name, ds_description = load_csv(args.csv)
        region = "Custom"
        ds_name_display = f"Custom CSV ({args.csv})"
    elif args.dataset:
        ds_key = args.dataset.lower()
        if ds_key not in DATASETS:
            valid = ", ".join(DATASETS.keys())
            print(f"ERROR: unknown dataset '{ds_key}'. Valid: {valid}", file=sys.stderr)
            sys.exit(1)
        ds = DATASETS[ds_key]
        raw_events = ds["events"]
        region = ds["region"]
        ds_name_display = ds["name"]
        ds_description = ds["description"]
    else:
        print("ERROR: specify --dataset or --csv", file=sys.stderr)
        parser.print_help()
        sys.exit(1)

    slug = f"seismograph_{ds_key}"
    output_dir = Path(args.output)

    print(f"Building Seismograph Kit: {ds_name_display}")
    print(f"  Events: {len(raw_events)}")

    # Process events
    events_processed = process_events(raw_events)

    # Summary stats
    voices_used = set(ev["voice"] for ev in events_processed)
    vel_min = min(ev["velocity"] for ev in events_processed)
    vel_max = max(ev["velocity"] for ev in events_processed)
    mags = [ev["magnitude"] for ev in events_processed]
    print(f"  Magnitude range: M{min(mags):.1f}–M{max(mags):.1f}")
    print(f"  Velocity range:  {vel_min}–{vel_max}")
    print(f"  Voices active:   {', '.join(sorted(voices_used))}")

    # Build XPM
    xpm_xml = build_xpm(ds_name_display, slug, ds_description)

    # Build report
    report_txt = build_report(events_processed, ds_name_display, region,
                               ds_description, slug)

    # Build manifest
    manifest = {
        "tool":        "xpn_seismograph_kit",
        "version":     "1.0.0",
        "date":        __import__("datetime").date.today().isoformat(),
        "dataset":     ds_key,
        "dataset_name": ds_name_display,
        "region":      region if args.csv is None else "Custom",
        "total_events": len(events_processed),
        "magnitude_range": {
            "min": min(mags),
            "max": max(mags),
        },
        "velocity_range": {"min": vel_min, "max": vel_max},
        "pad_layout": {
            "rows": {
                "kick":  "0–10 km    (shallow, surface-rupturing)",
                "snare": "10–35 km   (mid-crustal)",
                "tom":   "35–70 km   (deep crustal / upper mantle)",
                "sub":   "70+ km     (subduction zone deep slab)",
            },
            "cols": "1=early in sequence, 4=late",
        },
        "velocity_mapping": {
            "M5.0": 40,
            "M7.0": 90,
            "M9.0": 127,
            "interpolation": "linear",
        },
        "events": events_processed,
    }

    # Write ZIP
    zip_path = write_zip(slug, xpm_xml, report_txt, manifest, output_dir)
    print(f"  Written: {zip_path}")

    # Also write standalone report
    report_path = output_dir / "earthquake_report.txt"
    report_path.write_text(report_txt, encoding="utf-8")
    print(f"  Written: {report_path}")

    print()
    print("Pad grid:")
    print(f"  {'Pad':>3}  {'MIDI':>4}  {'Voice':<6}  {'Col':>4}  Vel layers")
    print("  " + "-" * 45)
    for voice_idx, voice in enumerate(VOICE_ORDER):
        for col in range(1, 5):
            pad_i = voice_idx * 4 + (col - 1)
            midi = PAD_MIDI_NOTES[pad_i]
            # Which events land here?
            evs = [e for e in events_processed
                   if e["voice"] == voice and e["pad_col"] == col]
            hits = f"{len(evs)} hit{'s' if len(evs) != 1 else ''}"
            print(f"  P{pad_i+1:>2}  {midi:>4}  {voice:<6}  Col{col}  "
                  f"[vel 1-127, 4 layers]  {hits}")

    print()
    print("Sample files needed (place in Samples/ inside the ZIP):")
    print("  seismograph_{voice}_col{1-4}_{shallow|intermediate|deep|very-deep}.wav")
    print("  Total: 4 voices x 4 cols x 4 layers = 64 WAV files")
    print("  (Placeholder silent WAVs are included — replace with real renders.)")


if __name__ == "__main__":
    main()
