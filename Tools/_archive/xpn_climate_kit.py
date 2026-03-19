#!/usr/bin/env python3
"""
XPN Climate Kit Generator — XO_OX Designs
Hex + Rex build: climate measurement data as velocity layers.

Velocity = time. The harder you hit the pad, the further we've drifted.
Six indicators. Seventy-three years of atmospheric memory.
The most politically charged drum kit ever made.

This kit was built from 73 years of atmospheric measurements.
It doesn't judge. It just plays what the numbers say.

CLI:
  python xpn_climate_kit.py --output ./kits/
  python xpn_climate_kit.py --indicator co2 --output ./kits/
  python xpn_climate_kit.py --year 1970 --output ./kits/
  python xpn_climate_kit.py --year-range 1970:2023 --step 10 --output ./kits/
"""

import argparse
import json
import math
import sys
from datetime import date
from pathlib import Path
from xml.sax.saxutils import escape as xml_escape


# =============================================================================
# CLIMATE DATA — hardcoded from published scientific records
# Sources: NOAA, NASA GISS, NSIDC, HOT (Hawaii Ocean Time-series),
#          IMBIE, PRODES/INPE
# =============================================================================

CLIMATE_DATA = {
    "co2": {
        "name":        "CO2 Concentration",
        "source":      "Mauna Loa Observatory (NOAA/Scripps)",
        "unit":        "ppm (parts per million)",
        "pad_voice":   "kick",
        "midi_note":   36,
        "direction":   "ascending",   # higher value = more change = harder hit
        "baseline":    315.97,        # 1958 first measurement
        "description": (
            "Atmospheric CO2 measured at Mauna Loa since 1958. "
            "Low velocity = 1958 (315 ppm, pre-industrial edge). "
            "High velocity = 2023 (421 ppm, highest in 3 million years). "
            "DSP character: low vel = clean/bright (full spectrum), "
            "high vel = low-pass filtered + subtle saturation (atmospheric opacity)."
        ),
        "data": {
            1958: 315.97,
            1960: 316.91,
            1965: 320.04,
            1970: 325.68,
            1975: 331.11,
            1980: 338.68,
            1985: 345.87,
            1990: 354.35,
            1995: 360.62,
            2000: 369.52,
            2005: 379.80,
            2010: 389.90,
            2015: 400.83,
            2020: 412.50,
            2022: 418.56,
            2023: 421.08,
        },
    },

    "temperature": {
        "name":        "Global Mean Temperature Anomaly",
        "source":      "NASA GISS Surface Temperature Analysis (GISTEMP v4)",
        "unit":        "°C anomaly vs. 1951-1980 baseline",
        "pad_voice":   "snare",
        "midi_note":   38,
        "direction":   "ascending",
        "baseline":    -0.16,         # 1950 anomaly
        "description": (
            "Global surface temperature anomaly relative to 1951-1980 mean. "
            "Low velocity = 1950 (-0.16°C, cooler than baseline). "
            "High velocity = 2023 (+1.17°C, hottest year on record at time of writing). "
            "DSP character: low vel = warm analog tone, "
            "high vel = harsh/bright + subtle pitch-up (thermal expansion metaphor)."
        ),
        "data": {
            1950: -0.16,
            1955: -0.14,
            1960: -0.03,
            1965: -0.01,
            1970:  0.03,
            1975:  0.01,
            1980:  0.26,
            1985:  0.12,
            1990:  0.44,
            1995:  0.38,
            2000:  0.42,
            2005:  0.68,
            2010:  0.72,
            2015:  0.87,
            2020:  1.02,
            2021:  0.85,
            2022:  0.89,
            2023:  1.17,
        },
    },

    "sea_ice": {
        "name":        "Arctic Sea Ice Extent (September minimum)",
        "source":      "NSIDC Sea Ice Index, Version 3",
        "unit":        "million km²",
        "pad_voice":   "chat",
        "midi_note":   42,
        "direction":   "descending",  # lower value = more change = harder hit
        "baseline":    7.20,          # 1979 September minimum
        "description": (
            "Arctic sea ice September minimum extent since satellite records began (1979). "
            "Low velocity = 1979 (7.2M km², full Arctic coverage). "
            "High velocity = 2012 (3.41M km², record minimum). "
            "INVERTED mapping: less ice = harder hit. "
            "DSP character: low vel = sparse/open (wide stereo), "
            "high vel = compressed/narrow (the sound of a shrinking world)."
        ),
        "data": {
            1979: 7.20,
            1980: 7.83,
            1985: 6.93,
            1990: 6.24,
            1995: 6.14,
            2000: 6.32,
            2005: 5.57,
            2007: 4.17,
            2010: 4.90,
            2012: 3.41,   # record minimum
            2015: 4.41,
            2019: 4.14,
            2020: 3.74,
            2021: 4.72,
            2022: 4.67,
            2023: 4.23,
        },
    },

    "ocean_ph": {
        "name":        "Ocean pH (Hawaii Ocean Time-series)",
        "source":      "HOT Station ALOHA (University of Hawaii)",
        "unit":        "pH units",
        "pad_voice":   "ohat",
        "midi_note":   46,
        "direction":   "descending",  # lower pH = more acidification = harder hit
        "baseline":    8.12,          # estimated pre-industrial baseline
        "description": (
            "Ocean pH measured at HOT station ALOHA, Hawaii, since 1988. "
            "pH has dropped ~0.1 units since pre-industrial times — "
            "a 26% increase in hydrogen ion concentration (ocean acidification). "
            "INVERTED mapping: lower pH = harder hit. "
            "DSP character: low vel = pure clean tone, "
            "high vel = corrosive distortion + high-pass (acid metaphor)."
        ),
        "data": {
            1988: 8.110,
            1990: 8.107,
            1993: 8.104,
            1995: 8.101,
            1998: 8.098,
            2000: 8.090,
            2003: 8.086,
            2005: 8.082,
            2008: 8.078,
            2010: 8.075,
            2013: 8.071,
            2015: 8.065,
            2018: 8.059,
            2020: 8.053,
            2022: 8.047,
            2023: 8.044,
        },
    },

    "ice_mass": {
        "name":        "Greenland Ice Sheet Mass Loss",
        "source":      "IMBIE (Ice sheet Mass Balance Inter-comparison Exercise)",
        "unit":        "Gt/year (gigatonnes per year)",
        "pad_voice":   "clap",
        "midi_note":   39,
        "direction":   "ascending_abs",  # larger absolute loss = harder hit
        "baseline":    0.0,              # ~1972 near-zero baseline
        "description": (
            "Greenland ice sheet mass loss rate. Negative = mass loss. "
            "Near zero in 1970s, accelerating from 1990s, "
            "record loss in 2019 (-532 Gt in a single year). "
            "DSP character: low vel = deep/resonant (solid ice), "
            "high vel = cracking/transient-heavy (calving event)."
        ),
        "data": {
            1972:    0,
            1980:   -5,
            1985:   -8,
            1990:  -10,
            1995:  -30,
            2000:  -50,
            2003: -100,
            2007: -200,
            2010: -250,
            2012: -300,
            2015: -280,
            2019: -532,  # record single-year loss
            2020: -480,
            2021: -380,
            2022: -450,
            2023: -480,
        },
    },

    "deforestation": {
        "name":        "Amazon Deforestation Rate",
        "source":      "INPE PRODES (Brazil National Institute for Space Research)",
        "unit":        "km²/year",
        "pad_voice":   "rim",
        "midi_note":   43,
        "direction":   "ascending",  # higher deforestation = harder hit
        "baseline":    0.0,
        "description": (
            "Annual Amazon deforestation rate from Brazilian PRODES satellite monitoring. "
            "Peak in 1988 (21,050 km²/yr) and 2004 (27,772 km²/yr), "
            "reduced after policies, then rebounded 2019-2022. "
            "DSP character: low vel = lush/reverberant (forest acoustic), "
            "high vel = dry/no reverb + noise (bare ground, chainsaw texture)."
        ),
        "data": {
            1988: 21050,
            1991: 11030,
            1994: 14896,
            1995: 29059,
            2000: 18165,
            2002: 21651,
            2004: 27772,  # record peak
            2006: 14286,
            2008: 12911,
            2010:  7000,
            2012:  4656,  # lowest (policy enforcement peak)
            2014:  5012,
            2016:  7989,
            2019: 11088,  # post-policy rebound
            2021: 13235,
            2022: 11568,
            2023:  9001,
        },
    },
}

INDICATOR_KEYS = list(CLIMATE_DATA.keys())


# =============================================================================
# DATA NORMALISATION
# =============================================================================

def get_years_sorted(data: dict) -> list[int]:
    return sorted(data.keys())


def normalize_value(value: float, min_val: float, max_val: float,
                    direction: str) -> float:
    """
    Normalize a raw measurement to 0.0-1.0 where 1.0 = most change from baseline.
    direction:
      'ascending'      → higher value = more change → 1.0
      'descending'     → lower value = more change → 1.0
      'ascending_abs'  → larger absolute value = more change → 1.0
    """
    if direction == "ascending_abs":
        val = abs(value)
        mn  = abs(min_val)
        mx  = abs(max_val)
    elif direction == "descending":
        val = max_val - value
        mn  = 0.0
        mx  = max_val - min_val
    else:  # ascending
        val = value - min_val
        mn  = 0.0
        mx  = max_val - min_val

    if mx == 0:
        return 0.0
    return max(0.0, min(1.0, (val - mn) / mx))


def interpolate_year(year: int, data: dict) -> float:
    """Linear interpolation for years between data points."""
    years = get_years_sorted(data)
    if year <= years[0]:
        return data[years[0]]
    if year >= years[-1]:
        return data[years[-1]]
    for i in range(len(years) - 1):
        if years[i] <= year <= years[i + 1]:
            t = (year - years[i]) / (years[i + 1] - years[i])
            return data[years[i]] + t * (data[years[i + 1]] - data[years[i]])
    return data[years[-1]]


def pick_8_timepoints(indicator_key: str) -> list[int]:
    """
    Select 8 representative time points for velocity layers.
    Distributed from earliest to most recent, weighted toward the recent.
    """
    data = CLIMATE_DATA[indicator_key]["data"]
    years = get_years_sorted(data)
    if len(years) <= 8:
        # Pad with interpolations
        first, last = years[0], years[-1]
        step = (last - first) / 7
        return [int(round(first + i * step)) for i in range(8)]
    # Pick 8 evenly spaced
    first, last = years[0], years[-1]
    step = (last - first) / 7
    return [int(round(first + i * step)) for i in range(8)]


def build_velocity_layers_for_indicator(indicator_key: str) -> list[dict]:
    """
    Build 8 velocity layers for one indicator.
    Returns list of dicts: {year, value, normalized, vel_start, vel_end, volume, dsp_note}
    """
    info       = CLIMATE_DATA[indicator_key]
    data       = info["data"]
    direction  = info["direction"]
    years_all  = get_years_sorted(data)

    # Value range across all years
    all_vals = list(data.values())
    if direction == "ascending_abs":
        min_val = min(abs(v) for v in all_vals)
        max_val = max(abs(v) for v in all_vals)
    elif direction == "descending":
        min_val = min(all_vals)
        max_val = max(all_vals)
    else:
        min_val = min(all_vals)
        max_val = max(all_vals)

    timepoints = pick_8_timepoints(indicator_key)

    # Velocity bands (8 layers, even distribution per Vibe)
    vel_bands = [
        (1,  15),
        (16, 30),
        (31, 47),
        (48, 63),
        (64, 79),
        (80, 95),
        (96, 111),
        (112, 127),
    ]

    layers = []
    for i, year in enumerate(timepoints):
        value     = interpolate_year(year, data)
        norm      = normalize_value(value, min_val, max_val, direction)
        vs, ve    = vel_bands[i]
        volume    = 0.30 + norm * 0.65    # 0.30 (low) → 0.95 (high)

        # DSP note: human-readable processing hint
        dsp_note  = _build_dsp_note(indicator_key, norm, year, value, info["unit"])

        layers.append({
            "layer_num":   i + 1,
            "year":        year,
            "value":       round(value, 4),
            "unit":        info["unit"],
            "normalized":  round(norm, 4),
            "vel_start":   vs,
            "vel_end":     ve,
            "volume":      volume,
            "dsp_note":    dsp_note,
            "sample_name": f"{indicator_key}_{year}",
            "sample_file": f"{indicator_key}_{year}.wav",
        })

    return layers


def _build_dsp_note(indicator_key: str, norm: float, year: int,
                    value: float, unit: str) -> str:
    """Generate DSP processing description for a layer."""
    intensity = "minimal" if norm < 0.25 else ("moderate" if norm < 0.60 else
                 ("severe" if norm < 0.85 else "extreme"))

    dsp_map = {
        "co2": {
            "filter": f"low-pass at {int(20000 * (1 - norm * 0.6))} Hz",
            "drive":  f"soft saturation {int(norm * 25)} dB",
            "note":   f"CO2 opacity metaphor: {value:.1f} ppm",
        },
        "temperature": {
            "pitch":  f"+{norm * 12:.1f} semitones (thermal expansion)",
            "bright": f"high-shelf +{norm * 6:.1f} dB at 8kHz",
            "note":   f"Temp anomaly: +{value:.2f}°C vs. 1951-1980 baseline",
        },
        "sea_ice": {
            "stereo": f"width {int(100 - norm * 60)}% (narrowing world)",
            "comp":   f"ratio {1 + norm * 5:.1f}:1 compression",
            "note":   f"Ice extent: {value:.2f} million km² (less = harder hit)",
        },
        "ocean_ph": {
            "drive":  f"harmonic distortion {int(norm * 30)}% (acid corrosion)",
            "high":   f"high-pass at {int(200 + norm * 2000)} Hz",
            "note":   f"pH {value:.3f} — {norm * 100:.0f}% acidification relative to 1988",
        },
        "ice_mass": {
            "trans":  f"attack sharpened by {norm * 80:.0f}% (calving impulse)",
            "reverb": f"room size {int(100 - norm * 80)}% (solid → hollow)",
            "note":   f"Mass loss: {abs(value):.0f} Gt/year",
        },
        "deforestation": {
            "reverb": f"reverb dry {int(norm * 100)}% (forest to bare ground)",
            "noise":  f"noise layer {norm * 20:.0f} dB (chainsaw texture)",
            "note":   f"Deforestation: {value:.0f} km²/year",
        },
    }

    dsp = dsp_map.get(indicator_key, {})
    hints = [f"{k}: {v}" for k, v in dsp.items() if k != "note"]
    data_note = dsp.get("note", "")
    return f"[{year}] {intensity.upper()} — {data_note} | Processing: {'; '.join(hints)}"


# =============================================================================
# XPM GENERATION
# =============================================================================

def _layer_block_climate(number: int, layer: dict) -> str:
    active = "True" if layer["sample_name"] else "False"
    return (
        f'          <Layer number="{number}">\n'
        f'            <Active>{active}</Active>\n'
        f'            <Volume>{layer["volume"]:.6f}</Volume>\n'
        f'            <Pan>0.500000</Pan>\n'
        f'            <Pitch>0.000000</Pitch>\n'
        f'            <TuneCoarse>0</TuneCoarse>\n'
        f'            <TuneFine>0</TuneFine>\n'
        f'            <VelStart>{layer["vel_start"]}</VelStart>\n'
        f'            <VelEnd>{layer["vel_end"]}</VelEnd>\n'
        f'            <SampleStart>0</SampleStart>\n'
        f'            <SampleEnd>0</SampleEnd>\n'
        f'            <Loop>False</Loop>\n'
        f'            <LoopStart>0</LoopStart>\n'
        f'            <LoopEnd>0</LoopEnd>\n'
        f'            <LoopTune>0</LoopTune>\n'
        f'            <Mute>False</Mute>\n'
        f'            <RootNote>0</RootNote>\n'
        f'            <KeyTrack>True</KeyTrack>\n'
        f'            <SampleName>{xml_escape(layer["sample_name"])}</SampleName>\n'
        f'            <SampleFile>{xml_escape(layer["sample_file"])}</SampleFile>\n'
        f'            <File>{xml_escape(layer["sample_file"])}</File>\n'
        f'            <!-- {xml_escape(layer["dsp_note"])} -->\n'
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


def _instrument_block_climate(instrument_num: int, indicator_key: str,
                               layers: list[dict]) -> str:
    info     = CLIMATE_DATA[indicator_key]
    pad_name = info["pad_voice"]

    # Voice physical defaults
    VOICE_PHYS = {
        "kick":  {"mono": True,  "poly": 1, "one_shot": True,  "vtf": 0.0,
                   "attack": 0.0, "decay": 0.3,  "sustain": 0.0, "release": 0.05,
                   "cutoff": 1.0, "resonance": 0.0},
        "snare": {"mono": True,  "poly": 1, "one_shot": True,  "vtf": 0.30,
                   "attack": 0.0, "decay": 0.4,  "sustain": 0.0, "release": 0.08,
                   "cutoff": 0.9, "resonance": 0.05},
        "chat":  {"mono": True,  "poly": 1, "one_shot": True,  "vtf": 0.0,
                   "attack": 0.0, "decay": 0.15, "sustain": 0.0, "release": 0.02,
                   "cutoff": 0.85, "resonance": 0.1},
        "ohat":  {"mono": False, "poly": 2, "one_shot": False, "vtf": 0.0,
                   "attack": 0.0, "decay": 0.8,  "sustain": 0.38, "release": 0.3,
                   "cutoff": 0.95, "resonance": 0.0},
        "clap":  {"mono": False, "poly": 2, "one_shot": True,  "vtf": 0.15,
                   "attack": 0.0, "decay": 0.5,  "sustain": 0.0, "release": 0.1,
                   "cutoff": 0.95, "resonance": 0.0},
        "rim":   {"mono": False, "poly": 2, "one_shot": True,  "vtf": 0.0,
                   "attack": 0.0, "decay": 0.35, "sustain": 0.0, "release": 0.08,
                   "cutoff": 1.0, "resonance": 0.0},
    }
    cfg = VOICE_PHYS.get(pad_name, VOICE_PHYS["rim"])

    layers_xml_parts = [_layer_block_climate(i + 1, layer) for i, layer in enumerate(layers)]
    # Fill remaining layer slots to 4 minimum
    for i in range(len(layers), 4):
        layers_xml_parts.append(_empty_layer(i + 1))
    layers_xml = "\n".join(layers_xml_parts)

    mute_xml   = "\n".join(f"        <MuteTarget{i+1}>0</MuteTarget{i+1}>" for i in range(4))
    simult_xml = "\n".join(f"        <SimultTarget{i+1}>0</SimultTarget{i+1}>" for i in range(4))

    mono_str    = "True" if cfg["mono"] else "False"
    oneshot_str = "True" if cfg["one_shot"] else "False"

    return (
        f'      <Instrument number="{instrument_num}">\n'
        f'        <!-- {xml_escape(info["name"])} | {xml_escape(info["source"])} -->\n'
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
        f'        <MuteGroup>0</MuteGroup>\n'
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
        f'        <VelocityToPitch>0.000000</VelocityToPitch>\n'
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


def generate_xpm_climate(indicator_keys: list[str], year_label: str = "full") -> str:
    """Generate a complete drum program XPM with climate velocity layers."""
    prog_name = xml_escape(f"XO_OX-CLIMATE-{year_label.upper()}")

    # Build PadNoteMap
    pad_note_entries = []
    for idx, key in enumerate(indicator_keys):
        info    = CLIMATE_DATA[key]
        midi    = info["midi_note"]
        pad_num = idx + 1
        pad_note_entries.append(
            f'        <Pad number="{pad_num}" note="{midi}"/>'
            f'  <!-- {info["name"]} -->'
        )
    pad_note_xml = "\n".join(pad_note_entries)

    # Build all 128 instruments
    midi_to_indicator = {CLIMATE_DATA[k]["midi_note"]: k for k in indicator_keys}
    instrument_parts  = []
    for i in range(128):
        if i in midi_to_indicator:
            key    = midi_to_indicator[i]
            layers = build_velocity_layers_for_indicator(key)
            instrument_parts.append(
                _instrument_block_climate(i, key, layers)
            )
        else:
            # Empty instrument
            empty_layers = "\n".join(_empty_layer(j + 1) for j in range(4))
            instrument_parts.append(
                f'      <Instrument number="{i}">\n'
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
                f'        <MuteTarget1>0</MuteTarget1>\n'
                f'        <MuteTarget2>0</MuteTarget2>\n'
                f'        <MuteTarget3>0</MuteTarget3>\n'
                f'        <MuteTarget4>0</MuteTarget4>\n'
                f'        <SimultTarget1>0</SimultTarget1>\n'
                f'        <SimultTarget2>0</SimultTarget2>\n'
                f'        <SimultTarget3>0</SimultTarget3>\n'
                f'        <SimultTarget4>0</SimultTarget4>\n'
                f'        <OneShot>True</OneShot>\n'
                f'        <FilterType>2</FilterType>\n'
                f'        <Cutoff>1.000000</Cutoff>\n'
                f'        <Resonance>0.000000</Resonance>\n'
                f'        <Layers>\n'
                f'{empty_layers}\n'
                f'        </Layers>\n'
                f'      </Instrument>'
            )

    instruments_xml = "\n".join(instrument_parts)

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
        '    <!-- XO_OX Climate Kit: velocity = time, harder = further we have drifted -->\n'
        '    <!-- 6 indicators: CO2/Snare Temp/Snare Sea-Ice/CHat Ocean-pH/OHat Ice-Mass/Clap Deforestation/Rim -->\n'
        f'    <ProgramPads>{pad_json}</ProgramPads>\n'
        '    <PadNoteMap>\n'
        f'{pad_note_xml}\n'
        '    </PadNoteMap>\n'
        '    <PadGroupMap>\n'
        '    </PadGroupMap>\n'
        '    <QLinks>\n'
        '      <QLink number="1">\n'
        '        <Name>TONE</Name>\n'
        '        <Parameter>FilterCutoff</Parameter>\n'
        '        <Min>0.200000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="2">\n'
        '        <Name>ERA</Name>\n'
        '        <Parameter>TuneCoarse</Parameter>\n'
        '        <Min>-12</Min>\n'
        '        <Max>12</Max>\n'
        '      </QLink>\n'
        '      <QLink number="3">\n'
        '        <Name>DRIVE</Name>\n'
        '        <Parameter>Resonance</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>0.600000</Max>\n'
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
# YEAR SNAPSHOT MODE
# =============================================================================

def build_year_snapshot(year: int) -> dict:
    """
    For a given year, return all 6 indicators' values at that time.
    """
    snapshot = {"year": year, "indicators": {}}
    for key, info in CLIMATE_DATA.items():
        data  = info["data"]
        years = get_years_sorted(data)
        if year < years[0]:
            value = data[years[0]]
            note  = f"(pre-record, using earliest: {years[0]})"
        elif year > years[-1]:
            value = data[years[-1]]
            note  = f"(post-record, using latest: {years[-1]})"
        else:
            value = interpolate_year(year, data)
            note  = "(interpolated)"

        all_vals = list(data.values())
        direction = info["direction"]
        if direction == "ascending_abs":
            min_val = min(abs(v) for v in all_vals)
            max_val = max(abs(v) for v in all_vals)
        elif direction == "descending":
            min_val = min(all_vals)
            max_val = max(all_vals)
        else:
            min_val = min(all_vals)
            max_val = max(all_vals)

        norm = normalize_value(value, min_val, max_val, direction)

        snapshot["indicators"][key] = {
            "name":       info["name"],
            "value":      round(value, 4),
            "unit":       info["unit"],
            "normalized": round(norm, 4),
            "note":       note,
            "vel_equivalent": int(round(norm * 126)) + 1,
        }
    return snapshot


# =============================================================================
# DATA JSON
# =============================================================================

def build_climate_data_json() -> dict:
    out = {
        "tool":      "xpn_climate_kit",
        "version":   "1.0.0",
        "generated": str(date.today()),
        "liner_note": (
            "Every time you hit this pad, you're hearing data. "
            "The harder you hit, the further we've drifted. "
            "This kit was built from 73 years of atmospheric measurements. "
            "It doesn't judge. It just plays what the numbers say."
        ),
        "indicators": {}
    }
    for key, info in CLIMATE_DATA.items():
        layers = build_velocity_layers_for_indicator(key)
        out["indicators"][key] = {
            "name":        info["name"],
            "source":      info["source"],
            "unit":        info["unit"],
            "direction":   info["direction"],
            "description": info["description"],
            "pad_voice":   info["pad_voice"],
            "midi_note":   info["midi_note"],
            "raw_data":    {str(y): v for y, v in info["data"].items()},
            "velocity_layers": [
                {
                    "layer":      layer["layer_num"],
                    "year":       layer["year"],
                    "value":      layer["value"],
                    "unit":       layer["unit"],
                    "normalized": layer["normalized"],
                    "vel_start":  layer["vel_start"],
                    "vel_end":    layer["vel_end"],
                    "volume":     round(layer["volume"], 6),
                    "dsp_note":   layer["dsp_note"],
                    "sample_name": layer["sample_name"],
                    "sample_file": layer["sample_file"],
                }
                for layer in layers
            ]
        }
    return out


# =============================================================================
# README GENERATION
# =============================================================================

def build_readme(year_label: str = "full") -> str:
    lines = []
    lines.append("# XO_OX Climate Kit")
    lines.append("")
    lines.append("> Every time you hit this pad, you're hearing data. The harder you hit,")
    lines.append("> the further we've drifted. This kit was built from 73 years of atmospheric")
    lines.append("> measurements. It doesn't judge. It just plays what the numbers say.")
    lines.append("")
    lines.append("---")
    lines.append("")
    lines.append("## Concept")
    lines.append("")
    lines.append(
        "Climate measurement data mapped to velocity layers. Velocity = time. "
        "The earliest measured values (1950s-1970s) live at the softest "
        "touch. The most recent values (2020s) live at the hardest hit. "
        "Playing this kit over time is hearing Earth's atmosphere change under your fingers."
    )
    lines.append("")
    lines.append("## Pad Layout")
    lines.append("")
    lines.append("| Pad | Voice | Indicator | Source |")
    lines.append("|-----|-------|-----------|--------|")
    for idx, key in enumerate(INDICATOR_KEYS):
        info = CLIMATE_DATA[key]
        lines.append(
            f"| {idx+1} | {info['pad_voice'].upper()} | {info['name']} | {info['source']} |"
        )
    lines.append("")
    lines.append("## Velocity = Time")
    lines.append("")
    lines.append(
        "Each pad has 8 velocity layers, each layer corresponds to a decade of measurement. "
        "Ghost notes (barely touching) = oldest data. Full force = 2020s data."
    )
    lines.append("")
    lines.append("| Velocity Range | Approximate Era |")
    lines.append("|----------------|-----------------|")
    era_map = [
        ("1-15",    "1950s-1960s (pre-acceleration)"),
        ("16-30",   "1960s-1970s (early monitoring)"),
        ("31-47",   "1970s-1980s (acceleration begins)"),
        ("48-63",   "1980s-1990s (alarm bells)"),
        ("64-79",   "1990s-2000s (policy debates)"),
        ("80-95",   "2000s-2010s (acceleration clear)"),
        ("96-111",  "2010s (record-breaking decade)"),
        ("112-127", "2020s (where we are now)"),
    ]
    for vel_range, era in era_map:
        lines.append(f"| {vel_range} | {era} |")
    lines.append("")
    lines.append("## Indicators")
    lines.append("")
    for key, info in CLIMATE_DATA.items():
        lines.append(f"### {info['name']}")
        lines.append(f"**Source:** {info['source']}")
        lines.append(f"**Unit:** {info['unit']}")
        lines.append("")
        lines.append(info['description'])
        lines.append("")
        lines.append("**Key data points:**")
        years = get_years_sorted(info["data"])
        for i in range(0, len(years), max(1, len(years) // 6)):
            y = years[i]
            v = info["data"][y]
            lines.append(f"- {y}: {v} {info['unit']}")
        lines.append("")
    lines.append("## DSP Processing Notes")
    lines.append("")
    lines.append(
        "Each indicator has a characteristic DSP treatment that metaphorically represents "
        "the physical process being measured. These are production guidelines for sound designers "
        "rendering the sample library:"
    )
    lines.append("")
    dsp_notes = {
        "co2":          "Low velocity = clean/bright (clear atmosphere). High velocity = low-pass filter + saturation (atmospheric opacity, heat trapping).",
        "temperature":  "Low velocity = warm analog tone. High velocity = pitch-up + bright shelf (thermal expansion, heat distortion).",
        "sea_ice":      "Low velocity = wide stereo (open arctic). High velocity = narrow/compressed (a shrinking world).",
        "ocean_ph":     "Low velocity = pure clean tone. High velocity = harmonic distortion + high-pass (acid corrosion).",
        "ice_mass":     "Low velocity = deep reverberant (solid ice mass). High velocity = short/dry + sharp transient (calving, collapse).",
        "deforestation":"Low velocity = lush reverb (forest acoustics). High velocity = dry + noise layer (bare ground, machinery).",
    }
    for key, note in dsp_notes.items():
        info = CLIMATE_DATA[key]
        lines.append(f"**{info['name']}:** {note}")
        lines.append("")
    lines.append("## Using the Year Snapshot Mode")
    lines.append("")
    lines.append(
        "The `--year` flag generates a kit where all 6 indicators reflect their values "
        "at a specific year. Use `--year-range` to generate a series of temporal snapshots."
    )
    lines.append("")
    lines.append("```bash")
    lines.append("# Single year snapshot")
    lines.append("python xpn_climate_kit.py --year 1970 --output ./kits/")
    lines.append("")
    lines.append("# Temporal series")
    lines.append("python xpn_climate_kit.py --year-range 1970:2023 --step 10 --output ./kits/")
    lines.append("```")
    lines.append("")
    lines.append("Loading the temporal kits in sequence and playing through them")
    lines.append("is hearing the climate change in real time under your hands.")
    lines.append("")
    lines.append("---")
    lines.append("")
    lines.append("*Generated by XO_OX Designs. Tool: xpn_climate_kit.py*")
    lines.append(f"*Generated: {date.today()}*")
    return "\n".join(lines)


# =============================================================================
# MAIN
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Climate Kit Generator — XO_OX / Hex+Rex",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--output", "-o", default="./kits",
                        help="Output directory (default: ./kits)")
    parser.add_argument("--indicator",
                        choices=INDICATOR_KEYS + ["all"], default="all",
                        help="Which indicator to generate (default: all 6)")
    parser.add_argument("--year", type=int, default=None,
                        help="Generate a snapshot kit for a specific year (1950-2023)")
    parser.add_argument("--year-range", metavar="START:END", default=None,
                        help="Generate temporal series (e.g. 1970:2023)")
    parser.add_argument("--step", type=int, default=10,
                        help="Year step for --year-range (default: 10)")
    args = parser.parse_args()

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    # Decide which indicators to include
    if args.indicator == "all":
        selected_keys = INDICATOR_KEYS
    else:
        selected_keys = [args.indicator]

    # Year-range mode: generate series of snapshot kits
    if args.year_range:
        parts = args.year_range.split(":")
        if len(parts) != 2:
            parser.error("--year-range must be START:END, e.g. 1970:2023")
        start_yr, end_yr = int(parts[0]), int(parts[1])
        years = list(range(start_yr, end_yr + 1, args.step))
        if end_yr not in years:
            years.append(end_yr)

        print(f"[Hex+Rex] Generating temporal series: {years}")
        for yr in years:
            _generate_for_year(yr, selected_keys, output_dir)
        print(f"\n[Hex+Rex] Done. {len(years)} kits in {output_dir}/")
        print(f"          Load them in sequence. Hear the decades pass.")
        return

    # Single year snapshot
    if args.year:
        _generate_for_year(args.year, selected_keys, output_dir)
        return

    # Full kit (all time points as velocity layers)
    print("[Hex+Rex] Generating full climate kit (all years as velocity layers)")
    _generate_full_kit(selected_keys, output_dir)


def _generate_full_kit(selected_keys: list[str], output_dir: Path):
    """Generate the full kit with velocity = temporal layers."""
    year_label = "full"
    xpm_path = output_dir / f"climate_kit_{year_label}.xpm"
    xpm_xml  = generate_xpm_climate(selected_keys, year_label)
    xpm_path.write_text(xpm_xml, encoding="utf-8")
    print(f"[Hex+Rex] Wrote XPM      → {xpm_path}")

    # Data JSON
    data_json = build_climate_data_json()
    data_path = output_dir / "climate_data.json"
    data_path.write_text(json.dumps(data_json, indent=2), encoding="utf-8")
    print(f"[Hex+Rex] Wrote data     → {data_path}")

    # README
    readme_path = output_dir / "climate_readme.md"
    readme_path.write_text(build_readme(year_label), encoding="utf-8")
    print(f"[Hex+Rex] Wrote README   → {readme_path}")

    # Print layer summary
    print()
    print("  6 climate indicators:")
    for key in selected_keys:
        info   = CLIMATE_DATA[key]
        layers = build_velocity_layers_for_indicator(key)
        print(f"\n  [{info['pad_voice'].upper()}] {info['name']} ({info['unit']})")
        print(f"      Source: {info['source']}")
        for layer in layers:
            bar_len = int(layer["normalized"] * 20)
            bar     = "#" * bar_len + "." * (20 - bar_len)
            print(f"      L{layer['layer_num']} vel {layer['vel_start']:3d}-{layer['vel_end']:3d}"
                  f"  [{bar}] {layer['year']}: {layer['value']} {info['unit']}")

    print(f"\n[Hex+Rex] Done. Load climate_kit_full.xpm into your MPC.")
    print(f"          Ghost note = the world we inherited.")
    print(f"          Full force = where we are now.")


def _generate_for_year(year: int, selected_keys: list[str], output_dir: Path):
    """Generate a single-year snapshot kit."""
    print(f"[Hex+Rex] Generating year snapshot: {year}")
    snapshot = build_year_snapshot(year)

    # For year mode: build a modified XPM where all 8 velocity layers
    # use the SAME year's sample (uniform across velocity = mono-temporal)
    # The kit sounds like that specific year
    prog_name  = xml_escape(f"XO_OX-CLIMATE-{year}")
    year_label = str(year)

    # Build instruments with year-specific single-layer pads
    midi_to_indicator = {CLIMATE_DATA[k]["midi_note"]: k for k in selected_keys}
    instrument_parts  = []

    for i in range(128):
        if i in midi_to_indicator:
            key     = midi_to_indicator[i]
            info    = CLIMATE_DATA[key]
            ind_val = snapshot["indicators"][key]

            # Single sample at this year, 4 identical velocity layers
            s_name = f"{key}_{year}"
            s_file = f"{key}_{year}.wav"
            vol    = 0.30 + ind_val["normalized"] * 0.65
            dsp    = _build_dsp_note(key, ind_val["normalized"], year,
                                     ind_val["value"], info["unit"])

            # Make 8 identical layers (all same year)
            year_layers = []
            vel_bands = [(1,15),(16,30),(31,47),(48,63),(64,79),(80,95),(96,111),(112,127)]
            for j, (vs, ve) in enumerate(vel_bands):
                year_layers.append({
                    "layer_num":   j + 1,
                    "year":        year,
                    "value":       ind_val["value"],
                    "unit":        info["unit"],
                    "normalized":  ind_val["normalized"],
                    "vel_start":   vs,
                    "vel_end":     ve,
                    "volume":      vol,
                    "dsp_note":    dsp,
                    "sample_name": s_name,
                    "sample_file": s_file,
                })

            instrument_parts.append(
                _instrument_block_climate(i, key, year_layers)
            )
        else:
            empty_layers = "\n".join(_empty_layer(j + 1) for j in range(4))
            instrument_parts.append(
                f'      <Instrument number="{i}">\n'
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
                f'        <MuteTarget1>0</MuteTarget1>\n'
                f'        <MuteTarget2>0</MuteTarget2>\n'
                f'        <MuteTarget3>0</MuteTarget3>\n'
                f'        <MuteTarget4>0</MuteTarget4>\n'
                f'        <SimultTarget1>0</SimultTarget1>\n'
                f'        <SimultTarget2>0</SimultTarget2>\n'
                f'        <SimultTarget3>0</SimultTarget3>\n'
                f'        <SimultTarget4>0</SimultTarget4>\n'
                f'        <OneShot>True</OneShot>\n'
                f'        <FilterType>2</FilterType>\n'
                f'        <Cutoff>1.000000</Cutoff>\n'
                f'        <Resonance>0.000000</Resonance>\n'
                f'        <Layers>\n'
                f'{empty_layers}\n'
                f'        </Layers>\n'
                f'      </Instrument>'
            )

    instruments_xml = "\n".join(instrument_parts)

    pad_note_entries = []
    for idx, key in enumerate(selected_keys):
        info = CLIMATE_DATA[key]
        pad_note_entries.append(
            f'        <Pad number="{idx+1}" note="{info["midi_note"]}"/>'
            f'  <!-- {info["name"]} -->'
        )
    pad_note_xml = "\n".join(pad_note_entries)

    pad_json = json.dumps(
        {"ProgramPads": {"Universal": {"value0": False},
                         "Type": {"value0": 5},
                         "universalPad": 32512}},
        separators=(",", ":"),
    )

    xpm_xml = (
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
        f'    <!-- Year snapshot: {year} — all 6 indicators at their {year} values -->\n'
        f'    <ProgramPads>{pad_json}</ProgramPads>\n'
        '    <PadNoteMap>\n'
        f'{pad_note_xml}\n'
        '    </PadNoteMap>\n'
        '    <PadGroupMap>\n'
        '    </PadGroupMap>\n'
        '    <QLinks>\n'
        '      <QLink number="1">\n'
        '        <Name>TONE</Name>\n'
        '        <Parameter>FilterCutoff</Parameter>\n'
        '        <Min>0.200000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="2">\n'
        '        <Name>ERA</Name>\n'
        '        <Parameter>TuneCoarse</Parameter>\n'
        '        <Min>-12</Min>\n'
        '        <Max>12</Max>\n'
        '      </QLink>\n'
        '      <QLink number="3">\n'
        '        <Name>DRIVE</Name>\n'
        '        <Parameter>Resonance</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>0.600000</Max>\n'
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

    xpm_path = output_dir / f"climate_kit_{year}.xpm"
    xpm_path.write_text(xpm_xml, encoding="utf-8")
    print(f"[Hex+Rex] Wrote XPM      → {xpm_path}")

    # Snapshot JSON
    snap_path = output_dir / f"climate_snapshot_{year}.json"
    snap_path.write_text(json.dumps(snapshot, indent=2), encoding="utf-8")
    print(f"[Hex+Rex] Wrote snapshot → {snap_path}")

    # Print snapshot table
    print(f"\n  Year {year} — indicator values:")
    for key, val in snapshot["indicators"].items():
        bar_len = int(val["normalized"] * 20)
        bar     = "#" * bar_len + "." * (20 - bar_len)
        print(f"  [{key:15s}] [{bar}] {val['value']} {val['unit']}")


if __name__ == "__main__":
    main()
