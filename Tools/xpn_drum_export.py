#!/usr/bin/env python3
"""
XPN Drum Program Exporter — XO_OX Designs
Generates MPC-compatible drum expansion packs (.xpn) from XOnset presets.

For each XOnset preset, produces:
  - A <Program type="Drum"> XPM file (128 instruments, 8 active pads)

Pad layout (GM-convention MIDI notes):
  V1 Kick         → Note 36  (C2)
  V2 Snare        → Note 38  (D2)
  V3 Closed Hat   → Note 42  (F#2)   MuteGroup 1
  V4 Open Hat     → Note 46  (A#2)   MuteGroup 1 (muted by closed hat)
  V5 Clap         → Note 39  (D#2)
  V6 Tom          → Note 41  (F2)
  V7 Percussion   → Note 43  (G2)
  V8 FX/Cymbal    → Note 49  (C#3)

Kit Modes (--mode):
  velocity        4 layers by velocity (pp/mp/mf/ff). Default. Best for dynamics.
                  WAVs: {slug}_{voice}_v1.wav … v4.wav

  cycle           4 round-robin layers. Cycling through samples on successive
                  hits prevents the "machine gun" effect. Best for hats and snares.
                  WAVs: {slug}_{voice}_c1.wav … c4.wav

  random          4 layers picked randomly per hit. Best for organic fx/claps.
                  WAVs: {slug}_{voice}_c1.wav … c4.wav  (same naming as cycle)

  random-norepeat Random pick, never same sample twice in a row.
                  WAVs: {slug}_{voice}_c1.wav … c4.wav

  smart           Per-voice automatic mode assignment (recommended):
                    kick  → velocity  (dynamics matter)
                    snare → velocity  (dynamics matter)
                    chat  → cycle     (machine-gun prevention)
                    ohat  → cycle     (machine-gun prevention)
                    clap  → random    (organic feel)
                    tom   → velocity
                    perc  → cycle
                    fx    → random-norepeat

Per-Voice Smart Defaults (applied in all modes):
  kick:  VelocityToPitch=0.05 (slight pitch follow — classic bounce)
  snare: VelocityToFilter=0.30 (filter opens on hard hits)
  ohat:  OneShot=False, Polyphony=2 (rings until muted by closed hat)
  clap:  Polyphony=2 (stacked hits sound natural)
  fx:    OneShot=False, Polyphony=4 (sustained, overlapping)

WAV file naming:
  Velocity mode:  {preset_slug}_{voice}_v{1-4}.wav
  Cycle/Random:   {preset_slug}_{voice}_c{1-4}.wav
  Smart mode:     use the mode for that voice (see above)

Usage:
    # Export with velocity layers (default)
    python3 xpn_drum_export.py --preset "808 Reborn" \\
        --wavs-dir /path/to/wavs --output-dir /path/to/out

    # Export cycle kit (machine-gun prevention on every pad)
    python3 xpn_drum_export.py --preset "808 Reborn" --mode cycle \\
        --wavs-dir /path/to/wavs --output-dir /path/to/out

    # Export smart kit (per-voice optimal mode)
    python3 xpn_drum_export.py --preset "808 Reborn" --mode smart \\
        --wavs-dir /path/to/wavs --output-dir /path/to/out

    # Print WAV checklist for a preset + mode
    python3 xpn_drum_export.py --checklist "808 Reborn" --mode smart

    # Export all XOnset presets
    python3 xpn_drum_export.py --all-onset --mode smart \\
        --wavs-dir /path/to/wavs --output-dir /path/to/out
"""

import argparse
import json
import shutil
import sys
from datetime import date
from pathlib import Path
from xml.sax.saxutils import escape as xml_escape

try:
    from xpn_cover_art import generate_cover
    COVER_ART_AVAILABLE = True
except ImportError:
    COVER_ART_AVAILABLE = False

REPO_ROOT   = Path(__file__).parent.parent
PRESETS_DIR = REPO_ROOT / "Presets" / "XOlokun"


# =============================================================================
# ENGINE PAD COLORS — accent color per engine for MPC pad display
# =============================================================================

ENGINE_PAD_COLORS = {
    "OddfeliX":  "#00A6D6",  # Neon Tetra Blue
    "OddOscar":  "#E8839B",  # Axolotl Gill Pink
    "Overdub":   "#6B7B3A",  # Olive
    "Odyssey":   "#7B2D8B",  # Violet
    "Oblong":    "#E9A84A",  # Amber
    "Obese":     "#FF1493",  # Hot Pink
    "Onset":     "#0066FF",  # Electric Blue
    "Overworld": "#39FF14",  # Neon Green
    "Opal":      "#A78BFA",  # Lavender
    "Orbital":   "#FF6B6B",  # Warm Red
    "Organon":   "#00CED1",  # Bioluminescent Cyan
    "Ouroboros":  "#FF2D2D",  # Strange Attractor Red
    "Obsidian":  "#E8E0D8",  # Crystal White
    "Overbite":  "#F0EDE8",  # Fang White
    "Origami":   "#E63946",  # Vermillion Fold
    "Oracle":    "#4B0082",  # Prophecy Indigo
    "Obscura":   "#8A9BA8",  # Daguerreotype Silver
    "Oceanic":   "#00B4A0",  # Phosphorescent Teal
    "Ocelot":    "#C5832B",  # Ocelot Tawny
    "Optic":     "#00FF41",  # Phosphor Green
    "Oblique":   "#BF40FF",  # Prism Violet
    "Osprey":    "#1B4F8A",  # Azulejo Blue
    "Osteria":   "#722F37",  # Porto Wine
    "Owlfish":   "#B8860B",  # Abyssal Gold
    "Ohm":       "#87AE73",  # Sage
    "Orphica":   "#7FDBCA",  # Siren Seafoam
    "Obbligato": "#FF8A7A",  # Rascal Coral
    "Ottoni":    "#5B8A72",  # Patina
    "Ole":       "#C9377A",  # Hibiscus
    "Overlap":   "#00FFB4",  # Bioluminescent Cyan-Green
    "Outwit":    "#CC6600",  # Chromatophore Amber
    "Ombre":     "#7B6B8A",  # Shadow Mauve
    "Orca":      "#1B2838",  # Deep Ocean
    "Octopus":   "#E040FB",  # Chromatophore Magenta
    # Wave 4-7 engines (added 2026-03-20 through 2026-03-23)
    "Ostinato":  "#E8701A",  # Firelight Orange
    "OpenSky":   "#FF8C00",  # Sunburst
    "OceanDeep": "#2D0A4E",  # Trench Violet
    "Ouie":      "#708090",  # Hammerhead Steel
    "Obrix":     "#1E8B7E",  # Reef Jade
    "Orbweave":  "#8E4585",  # Kelp Knot Purple
    "Overtone":  "#A8D8EA",  # Spectral Ice
    "Organism":  "#C6E377",  # Emergence Lime
    "Oxbow":     "#1A6B5A",  # Oxbow Teal
    "Oware":     "#B5883E",  # Akan Goldweight
    "Opera":     "#D4AF37",  # Aria Gold
    "Offering":  "#E5B80B",  # Crate Wax Yellow
    "Osmosis":   "#C0C0C0",  # Surface Tension Silver
    # Engine #48 (added 2026-03-23)
    "Oxytocin":  "#9B5DE5",  # Synapse Violet
    # Newer engines (added 2026-03-23 through 2026-03-31)
    "Outlook":   "#4169E1",  # Horizon Indigo
    "Obiont":    "#E8A030",  # Bioluminescent Amber
    "Okeanos":   "#C49B3F",  # Cardamom Gold
    "Outflow":   "#1A1A40",  # Deep Storm Indigo
    # Kitchen Collection — Chef quad (organs)
    "Oto":       "#F5F0E8",  # Pipe Organ Ivory
    "Octave":    "#8B6914",  # Hammond Teak
    "Oleg":      "#C0392B",  # Theatre Red
    "Otis":      "#D4A017",  # Gospel Gold
    # Kitchen Collection — Kitchen quad (pianos)
    "Oven":      "#1C1C1C",  # Steinway Ebony
    "Ochre":     "#CC7722",  # Ochre Pigment
    "Obelisk":   "#FFFFF0",  # Grand Ivory
    "Opaline":   "#B7410E",  # Prepared Rust
    # Kitchen Collection — Cellar quad (bass)
    "Ogre":      "#0D0D0D",  # Sub Bass Black
    "Olate":     "#5C3317",  # Fretless Walnut
    "Oaken":     "#9C6B30",  # Upright Oak
    "Omega":     "#003366",  # Synth Bass Blue
    # Kitchen Collection — Garden quad (strings)
    "Orchard":   "#FFB7C5",  # Orchard Blossom
    "Overgrow":  "#228B22",  # Forest Green
    "Osier":     "#C0C8C8",  # Willow Silver
    "Oxalis":    "#9B59B6",  # Wood Sorrel Lilac
    # Kitchen Collection — Broth quad (pads)
    "Overwash":  "#F0F8FF",  # Tide Foam White
    "Overworn":  "#808080",  # Worn Felt Grey
    "Overflow":  "#1A3A5C",  # Deep Current Blue
    "Overcast":  "#778899",  # Light Slate Gray
    # Kitchen Collection — Fusion quad (EPs)
    "Oasis":     "#00827F",  # Desert Spring Teal
    "Oddfellow": "#B87333",  # Fusion Copper
    "Onkolo":    "#FFBF00",  # Spectral Amber
    "Opcode":    "#5F9EA0",  # Cadet Blue
}


# =============================================================================
# KIT MODES
# =============================================================================

# ZonePlay values — per MPC XPM spec
ZONE_PLAY = {
    "velocity":        1,
    "cycle":           2,
    "random":          3,
    "random-norepeat": 4,
}

# Velocity layer definitions (mode=velocity)
# Even distribution — straightforward, equal-width velocity bands
VEL_LAYERS_EVEN = [
    (1,   31,  0.35),   # ghost   (pp) — VelStart=1 not 0 (Rex Rule #3)
    (32,  63,  0.55),   # soft    (mp)
    (64,  95,  0.75),   # medium  (mf)
    (96, 127,  0.95),   # hard    (ff)
]

# Vibe's musical curve — ghost range is narrow (barely touching the pad),
# mid range is wide (where most expressive playing lives), hard is responsive.
# Feels more natural under the fingers than even splits.
VEL_LAYERS_MUSICAL = [
    (1,   20,  0.30),   # ghost   (15% — barely touching)
    (21,  50,  0.55),   # light   (23% — gentle playing)
    (51,  90,  0.75),   # mid     (31% — expressive sweet spot)
    (91, 127,  0.95),   # hard    (29% — full force)
]

# Default: musical curve (Vibe-approved). Set --vel-curve even to use even.
VEL_LAYERS = VEL_LAYERS_MUSICAL

# Guru Bin: velocity layers at true input velocities — do not normalize layers to equal amplitude
# The 4 render trigger velocities are: pp=20, mp=50, mf=80, ff=120
# Record at these exact MIDI velocities. Do NOT boost quiet layers to match loud ones.
# Dynamic range between layers IS the expression — preserve it.


def _set_vel_curve(curve: str):
    """Set the module-level velocity curve. Safe for CLI use."""
    global VEL_LAYERS
    VEL_LAYERS = VEL_LAYERS_EVEN if curve == "even" else VEL_LAYERS_MUSICAL
VEL_SUFFIXES = ["v1", "v2", "v3", "v4"]

# Cycle/random variant suffixes — all layers span full velocity range
CYCLE_SUFFIXES = ["c1", "c2", "c3", "c4"]

# CycleType values for XPM XML (Rex's bible §6)
CYCLE_TYPE = {
    "cycle":           "RoundRobin",
    "random":          "Random",
    "random-norepeat": "RandomNoRepeat",
}

# Smart mode: best ZonePlay per voice
SMART_MODE = {
    "kick":  "velocity",
    "snare": "velocity",
    "chat":  "cycle",
    "ohat":  "cycle",
    "clap":  "random",
    "tom":   "velocity",
    "perc":  "cycle",
    "fx":    "random-norepeat",
}


# =============================================================================
# PAD / VOICE DEFINITIONS
# =============================================================================

# (midi_note, voice_name, mute_group, mute_targets)
# Guru Bin canonical pad layout: A1=Kick, A2=Snare, A3=Clap, A4=CHat, B1=OHat, B2=Tom, B3=Perc, B4=FX
PAD_MAP = [
    (36, "kick",  0, [0, 0, 0, 0]),   # A1 (pad 0)
    (38, "snare", 2, [0, 0, 0, 0]),   # A2 (pad 1) MuteGroup 2
    (39, "clap",  3, [0, 0, 0, 0]),   # A3 (pad 2) MuteGroup 3
    (42, "chat",  1, [46, 0, 0, 0]),  # A4 (pad 3) Closed hat → mutes open hat (46)
    (46, "ohat",  1, [0, 0, 0, 0]),   # B1 (pad 4) Open hat
    (41, "tom",   0, [0, 0, 0, 0]),   # B2 (pad 5)
    (43, "perc",  0, [0, 0, 0, 0]),   # B3 (pad 6)
    (49, "fx",    0, [0, 0, 0, 0]),   # B4 (pad 7)
]

# Per-voice physical behavior overrides — these match the acoustic reality
# of each drum type and apply regardless of kit mode.
VOICE_DEFAULTS = {
    "kick": {
        "mono":               True,
        "polyphony":          1,
        "one_shot":           True,
        "velocity_to_pitch":  0.05,   # slight pitch follow on hard hits
        "velocity_to_filter": 0.0,
        "velocity_sensitivity": 1.0,
        # Envelope: punchy attack, short decay, no sustain
        "attack": 0.0, "hold": 0.0, "decay": 0.3, "sustain": 0.0, "release": 0.05,
        # Filter: open, slight velocity tracking
        "filter_type": 2, "cutoff": 1.0, "resonance": 0.0, "filter_env_amt": 0.0,
        # Q-Link targets (MPC knob assignments)
        "qlink_1": ("FilterCutoff", 0.3, 1.0),   # Q1: Tone
        "qlink_2": ("TuneCoarse",  -12, 12),      # Q2: Pitch
    },
    "snare": {
        "mono":               True,
        "polyphony":          1,
        "one_shot":           True,
        "velocity_to_pitch":  0.0,
        "velocity_to_filter": 0.30,   # filter opens on hard snare hits
        "velocity_sensitivity": 1.0,
        "attack": 0.0, "hold": 0.0, "decay": 0.4, "sustain": 0.0, "release": 0.08,
        "filter_type": 2, "cutoff": 0.9, "resonance": 0.05, "filter_env_amt": 0.1,
        "qlink_1": ("FilterCutoff", 0.2, 1.0),
        "qlink_2": ("TuneCoarse",  -5, 5),
    },
    "chat": {
        "mono":               True,
        "polyphony":          1,
        "one_shot":           True,
        "velocity_to_pitch":  0.0,
        "velocity_to_filter": 0.0,
        "velocity_sensitivity": 0.85,
        "attack": 0.0, "hold": 0.0, "decay": 0.15, "sustain": 0.0, "release": 0.02,
        "filter_type": 2, "cutoff": 0.85, "resonance": 0.1, "filter_env_amt": 0.0,
        "qlink_1": ("FilterCutoff", 0.3, 1.0),
        "qlink_2": ("Volume", 0.0, 1.0),
    },
    "ohat": {
        "mono":               False,
        "polyphony":          2,      # overlapping rings sound natural
        "one_shot":           False,  # rings until muted by closed hat
        "velocity_to_pitch":  0.0,
        "velocity_to_filter": 0.0,
        "velocity_sensitivity": 0.80,
        "attack": 0.0, "hold": 0.0, "decay": 0.8, "sustain": 0.38, "release": 0.3,
        "filter_type": 2, "cutoff": 0.95, "resonance": 0.0, "filter_env_amt": 0.0,
        "qlink_1": ("FilterCutoff", 0.3, 1.0),
        "qlink_2": ("Volume", 0.0, 1.0),
    },
    "clap": {
        "mono":               False,
        "polyphony":          2,      # stacked for that layer-clap texture
        "one_shot":           True,
        "velocity_to_pitch":  0.0,
        "velocity_to_filter": 0.15,
        "velocity_sensitivity": 0.90,
        "attack": 0.0, "hold": 0.0, "decay": 0.5, "sustain": 0.0, "release": 0.1,
        "filter_type": 2, "cutoff": 0.95, "resonance": 0.0, "filter_env_amt": 0.05,
        "qlink_1": ("FilterCutoff", 0.3, 1.0),
        "qlink_2": ("Pan", 0.0, 1.0),
    },
    "tom": {
        "mono":               False,   # toms sound better with slight overlap
        "polyphony":          2,
        "one_shot":           True,
        "velocity_to_pitch":  0.03,
        "velocity_to_filter": 0.10,
        "velocity_sensitivity": 1.0,
        "attack": 0.0, "hold": 0.0, "decay": 0.5, "sustain": 0.0, "release": 0.1,
        "filter_type": 2, "cutoff": 1.0, "resonance": 0.0, "filter_env_amt": 0.0,
        "qlink_1": ("TuneCoarse", -12, 12),
        "qlink_2": ("FilterCutoff", 0.3, 1.0),
    },
    "perc": {
        "mono":               False,
        "polyphony":          2,
        "one_shot":           True,
        "velocity_to_pitch":  0.0,
        "velocity_to_filter": 0.0,
        "velocity_sensitivity": 0.90,
        "attack": 0.0, "hold": 0.0, "decay": 0.35, "sustain": 0.0, "release": 0.08,
        "filter_type": 2, "cutoff": 1.0, "resonance": 0.0, "filter_env_amt": 0.0,
        "qlink_1": ("FilterCutoff", 0.3, 1.0),
        "qlink_2": ("TuneCoarse", -5, 5),
    },
    "fx": {
        "mono":               False,
        "polyphony":          4,      # sustained + layerable
        "one_shot":           False,  # let it decay naturally
        "velocity_to_pitch":  0.0,
        "velocity_to_filter": 0.0,
        "velocity_sensitivity": 0.75,
        "attack": 0.01, "hold": 0.0, "decay": 1.0, "sustain": 0.3, "release": 0.5,
        "filter_type": 2, "cutoff": 1.0, "resonance": 0.0, "filter_env_amt": 0.0,
        "qlink_1": ("FilterCutoff", 0.2, 1.0),
        "qlink_2": ("Volume", 0.0, 1.0),
    },
}

_VOICE_DEFAULTS_FALLBACK = {
    "mono": True, "polyphony": 1, "one_shot": True,
    "velocity_to_pitch": 0.0, "velocity_to_filter": 0.0,
    "velocity_sensitivity": 1.0,
    "attack": 0.0, "hold": 0.0, "decay": 0.0, "sustain": 1.0, "release": 0.0,
    "filter_type": 2, "cutoff": 1.0, "resonance": 0.0, "filter_env_amt": 0.0,
}


def _voice_cfg(voice_name: str) -> dict:
    return VOICE_DEFAULTS.get(voice_name, _VOICE_DEFAULTS_FALLBACK)


def _resolve_mode(kit_mode: str, voice_name: str) -> str:
    """Return the effective ZonePlay mode string for a voice given kit mode."""
    if kit_mode == "smart":
        return SMART_MODE.get(voice_name, "velocity")
    return kit_mode


# =============================================================================
# XPM LAYER BUILDERS
# =============================================================================

def _layers_for_voice(voice_name: str, kit_mode: str,
                      wav_map: dict, preset_slug: str,
                      dna: dict = None) -> list[tuple]:
    """
    Return a list of (vel_start, vel_end, volume, sample_name, sample_file)
    tuples for each layer of this voice in the given kit mode.

    When dna is provided and mode is velocity, the velocity splits and volumes
    are adapted based on the preset's Sonic DNA profile.
    """
    effective_mode = _resolve_mode(kit_mode, voice_name)

    if effective_mode == "velocity":
        # Use DNA-adaptive curve when DNA is available, otherwise fall back
        # to the module-level VEL_LAYERS (musical or even, per CLI flag).
        if dna is not None:
            vel_layers = _dna_adapt_velocity_layers(dna)
        else:
            vel_layers = VEL_LAYERS
        layers = []
        for i, (vel_start, vel_end, vol) in enumerate(vel_layers):
            key = f"{preset_slug}_{voice_name}_{VEL_SUFFIXES[i]}"
            name = wav_map.get(key, "")
            layers.append((vel_start, vel_end, vol, name, name))
        return layers

    # cycle / random / random-norepeat — all variants span full vel range
    layers = []
    for suffix in CYCLE_SUFFIXES:
        key = f"{preset_slug}_{voice_name}_{suffix}"
        name = wav_map.get(key, "")
        layers.append((1, 127, 0.707946, name, name))  # VelStart=1 (Rex Rule #3)
    return layers


def _layer_block(number: int, vel_start: int, vel_end: int,
                 sample_name: str, sample_file: str, volume: float,
                 program_slug: str = "",
                 cycle_type: str = "", cycle_group: int = 0) -> str:
    active = "True" if sample_name else "False"
    if active == "False":
        vel_start, vel_end = 0, 0
    # File path: relative from XPN root (Rex's Rule #5)
    file_path = f"Samples/{program_slug}/{sample_file}" if (sample_file and program_slug) else sample_file
    # CycleType/CycleGroup for round-robin (Rex's bible §6)
    cycle_xml = ""
    if cycle_type and sample_name:
        cycle_xml = (
            f'            <CycleType>{cycle_type}</CycleType>\n'
            f'            <CycleGroup>{cycle_group}</CycleGroup>\n'
        )
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
        f'            <KeyTrack>False</KeyTrack>\n'
        f'            <SampleName>{xml_escape(sample_name)}</SampleName>\n'
        f'            <SampleFile>{xml_escape(sample_file)}</SampleFile>\n'
        f'            <File>{xml_escape(file_path)}</File>\n'
        f'{cycle_xml}'
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
    """Four silent placeholder layers for inactive instruments (VelStart=0)."""
    blocks = []
    for i in range(1, 5):
        blocks.append(_layer_block(i, 0, 0, "", "", 0.707946))
    return "\n".join(blocks)


# =============================================================================
# INSTRUMENT BLOCK
# =============================================================================

def _instrument_block(instrument_num: int, voice_name: str,
                      mute_group: int, mute_targets: list,
                      wav_map: dict, preset_slug: str,
                      kit_mode: str, dna: dict = None) -> str:
    """Generate one <Instrument> XML block."""
    is_active = bool(voice_name)
    cfg = _voice_cfg(voice_name) if is_active else _VOICE_DEFAULTS_FALLBACK

    effective_mode = _resolve_mode(kit_mode, voice_name) if is_active else "velocity"
    zone_play = ZONE_PLAY.get(effective_mode, 1)

    # Determine CycleType/CycleGroup for round-robin modes
    cycle_type_str = CYCLE_TYPE.get(effective_mode, "")
    prog_slug = preset_slug

    if is_active:
        layer_data = _layers_for_voice(voice_name, kit_mode, wav_map, preset_slug, dna=dna)
        layers_xml = "\n".join(
            _layer_block(
                i + 1, vs, ve, sn, sf, vol,
                program_slug=prog_slug,
                cycle_type=cycle_type_str if effective_mode != "velocity" else "",
                cycle_group=instrument_num if cycle_type_str else 0,
            )
            for i, (vs, ve, vol, sn, sf) in enumerate(layer_data)
        )
    else:
        layers_xml = _empty_layers()

    mono_str     = "True" if cfg["mono"] else "False"
    oneshot_str  = "True" if cfg["one_shot"] else "False"

    # Per-voice envelope defaults (Vibe's tuning)
    vol_attack  = cfg.get("attack", 0.0)
    vol_hold    = cfg.get("hold", 0.0)
    vol_decay   = cfg.get("decay", 0.0)
    vol_sustain = cfg.get("sustain", 1.0)
    vol_release = cfg.get("release", 0.0)

    # Per-voice filter defaults
    flt_type    = cfg.get("filter_type", 2)
    flt_cutoff  = cfg.get("cutoff", 1.0)
    flt_reso    = cfg.get("resonance", 0.0)
    flt_env_amt = cfg.get("filter_env_amt", 0.0)

    mute_xml = "\n".join(
        f"        <MuteTarget{i+1}>{t}</MuteTarget{i+1}>"
        for i, t in enumerate(mute_targets)
    )
    simult_xml = "\n".join(
        f"        <SimultTarget{i+1}>0</SimultTarget{i+1}>"
        for i in range(4)
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
        f'        <Polyphony>{cfg["polyphony"]}</Polyphony>\n'
        f'        <FilterKeytrack>0.000000</FilterKeytrack>\n'
        f'        <LowNote>0</LowNote>\n'
        f'        <HighNote>127</HighNote>\n'
        f'        <IgnoreBaseNote>False</IgnoreBaseNote>\n'
        f'        <ZonePlay>{zone_play}</ZonePlay>\n'
        f'        <MuteGroup>{mute_group}</MuteGroup>\n'
        f'{mute_xml}\n'
        f'{simult_xml}\n'
        f'        <LfoPitch>0.000000</LfoPitch>\n'
        f'        <LfoCutoff>0.000000</LfoCutoff>\n'
        f'        <LfoVolume>0.000000</LfoVolume>\n'
        f'        <LfoPan>0.000000</LfoPan>\n'
        f'        <OneShot>{oneshot_str}</OneShot>\n'
        f'        <FilterType>{flt_type}</FilterType>\n'
        f'        <Cutoff>{flt_cutoff:.6f}</Cutoff>\n'
        f'        <Resonance>{flt_reso:.6f}</Resonance>\n'
        f'        <FilterEnvAmt>{flt_env_amt:.6f}</FilterEnvAmt>\n'
        f'        <AfterTouchToFilter>0.000000</AfterTouchToFilter>\n'
        f'        <VelocityToStart>0.000000</VelocityToStart>\n'
        f'        <VelocityToFilterAttack>0.000000</VelocityToFilterAttack>\n'
        f'        <VelocityToFilter>{cfg["velocity_to_filter"]:.6f}</VelocityToFilter>\n'
        f'        <VelocityToFilterEnvelope>0.000000</VelocityToFilterEnvelope>\n'
        f'        <FilterAttack>0.000000</FilterAttack>\n'
        f'        <FilterDecay>0.000000</FilterDecay>\n'
        f'        <FilterSustain>1.000000</FilterSustain>\n'
        f'        <FilterRelease>0.000000</FilterRelease>\n'
        f'        <FilterHold>0.000000</FilterHold>\n'
        f'        <FilterDecayType>True</FilterDecayType>\n'
        f'        <FilterADEnvelope>True</FilterADEnvelope>\n'
        f'        <VolumeHold>{vol_hold:.6f}</VolumeHold>\n'
        f'        <VolumeDecayType>True</VolumeDecayType>\n'
        f'        <VolumeADEnvelope>True</VolumeADEnvelope>\n'
        f'        <VolumeAttack>{vol_attack:.6f}</VolumeAttack>\n'
        f'        <VolumeDecay>{vol_decay:.6f}</VolumeDecay>\n'
        f'        <VolumeSustain>{vol_sustain:.6f}</VolumeSustain>\n'
        f'        <VolumeRelease>{vol_release:.6f}</VolumeRelease>\n'
        f'        <VelocityToPitch>{cfg["velocity_to_pitch"]:.6f}</VelocityToPitch>\n'
        f'        <VelocityToVolumeAttack>0.000000</VelocityToVolumeAttack>\n'
        f'        <VelocitySensitivity>{(_dna_adapt_voice_sensitivity(voice_name, dna or _DEFAULT_DNA) if is_active else cfg["velocity_sensitivity"]):.6f}</VelocitySensitivity>\n'
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


# =============================================================================
# XPM GENERATION
# =============================================================================

def _generate_qlink_xml() -> str:
    """
    Generate Q-Link knob assignments for the program level.

    Standardized XOlokun macro → MPC Q-Link mapping:
      Q1 → CHARACTER  (FilterCutoff — timbral character)
      Q2 → MOVEMENT   (LFO Rate — motion/modulation depth)
      Q3 → COUPLING   (Send2/AuxSend — cross-feed, closest MPC analog to coupling)
      Q4 → SPACE      (Send1/Reverb — spatial depth)

    Label names are ≤10 chars for MPC XL OLED display compatibility.
    """
    return (
        '    <QLinks>\n'
        '      <QLink number="1">\n'
        '        <Name>CHARACTER</Name>\n'            # ≤10 chars — XOlokun macro 1
        '        <Parameter>FilterCutoff</Parameter>\n'
        '        <Min>0.200000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="2">\n'
        '        <Name>MOVEMENT</Name>\n'             # ≤10 chars — XOlokun macro 2
        '        <Parameter>LFORate</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="3">\n'
        '        <Name>COUPLING</Name>\n'             # ≤10 chars — XOlokun macro 3
        '        <Parameter>Send2</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>0.700000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="4">\n'
        '        <Name>SPACE</Name>\n'                # ≤10 chars — XOlokun macro 4
        '        <Parameter>Send1</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>0.700000</Max>\n'
        '      </QLink>\n'
        '    </QLinks>\n'
    )


# =============================================================================
# DNA-ADAPTIVE VELOCITY CURVES
# =============================================================================
#
# Sonic DNA (6D) shapes the velocity response per preset. The base curve is
# Vibe's musical curve (VEL_LAYERS_MUSICAL). DNA dimensions modulate it:
#
#   aggression  → shifts velocity split points downward ("hotter" curve).
#                 High aggression: ghost range shrinks, hard range expands —
#                 louder sooner, ghost notes quieter. Musical rationale: aggressive
#                 presets reward harder playing and punish tentative touches.
#
#   warmth      → softens the crossover between adjacent layers. High warmth
#                 widens the overlap zone where two layers blend, producing
#                 gentler transitions. Low warmth → sharp, discrete jumps.
#                 Implemented as velocity band expansion toward neighbors.
#
#   brightness  → scales VelocitySensitivity for attack-responsive voices.
#                 Bright presets make velocity differences more audible in the
#                 transient — you hear the attack change with every dynamic level.
#
#   density     → adjusts layer volume scaling. Dense presets compress the
#                 volume difference between ghost and hard layers — everything
#                 stays fuller. Sparse presets widen the dynamic range.
#
# The remaining DNA dimensions (movement, space) don't meaningfully map to
# velocity curve behavior, so they are intentionally left unused here.
#
# All DNA modulation is bounded so that extreme values still produce valid
# MPC velocity splits (non-overlapping, 1-127 range, ascending).

# Default DNA (neutral — no curve modification)
_DEFAULT_DNA = {
    "brightness": 0.5, "warmth": 0.5, "movement": 0.5,
    "density": 0.5, "space": 0.5, "aggression": 0.5,
}


def _dna_adapt_velocity_layers(dna: dict) -> list[tuple]:
    """
    Return a DNA-modified velocity layer list: [(vel_start, vel_end, volume), ...].

    Starts from VEL_LAYERS_MUSICAL and applies DNA-driven adjustments.
    Returns a new list — never mutates the module-level constant.
    """
    aggression = dna.get("aggression", 0.5)
    warmth = dna.get("warmth", 0.5)
    density = dna.get("density", 0.5)

    # --- Aggression: shift split points downward (hotter curve) ---
    # At aggression=0.5 (neutral), no shift. At 1.0, ghost ceiling drops by ~8,
    # hard floor drops by ~12 — hard layer starts sooner.
    # The shift is applied as a bias that compresses lower layers and expands upper ones.
    aggr_bias = (aggression - 0.5) * 2.0  # range: -1.0 .. +1.0

    # Start from the musical curve boundaries
    #   ghost: 1-20, light: 21-50, mid: 51-90, hard: 91-127
    boundaries = [1, 20, 50, 90, 127]  # 5 boundaries → 4 bands

    # Shift the 3 internal boundaries downward for high aggression
    # (ghost shrinks, hard expands) or upward for low aggression.
    shift_amounts = [
        int(round(aggr_bias * -4)),   # boundary between ghost/light
        int(round(aggr_bias * -6)),   # boundary between light/mid
        int(round(aggr_bias * -8)),   # boundary between mid/hard
    ]
    adjusted = list(boundaries)
    for i, shift in enumerate(shift_amounts):
        adjusted[i + 1] = boundaries[i + 1] + shift

    # Clamp internal boundaries to maintain ordering with minimum 4-wide bands
    min_width = 4
    for i in range(1, 4):
        adjusted[i] = max(adjusted[i], adjusted[i - 1] + min_width)
    for i in range(3, 0, -1):
        adjusted[i] = min(adjusted[i], adjusted[i + 1] - min_width)

    # --- Warmth: widen crossover zones ---
    # High warmth overlaps adjacent layers by expanding each band slightly.
    # MPC velocity layers don't truly overlap, so we approximate by nudging
    # boundaries to create wider "transition" bands in the mid range.
    # At warmth=1.0, mid band expands ±3; at warmth=0.0, bands tighten ±2.
    warmth_nudge = int(round((warmth - 0.5) * 4))  # -2 .. +2
    # Only nudge the mid band (index 2-3) — it's the expressive sweet spot
    adjusted[2] = max(adjusted[1] + min_width,
                      min(adjusted[2] - warmth_nudge, adjusted[3] - min_width))
    adjusted[3] = max(adjusted[2] + min_width,
                      min(adjusted[3] + warmth_nudge, adjusted[4] - min_width))

    # Re-clamp after warmth nudge to maintain ascending invariant (bidirectional)
    for i in range(1, 4):
        adjusted[i] = max(adjusted[i], adjusted[i-1] + min_width)
    for i in range(3, 0, -1):
        adjusted[i] = min(adjusted[i], adjusted[i + 1] - min_width)
    adjusted[3] = min(adjusted[3], 126)

    # --- Density: compress/expand volume scaling ---
    # Neutral density (0.5) → original volumes. High density compresses toward
    # 0.7 (everything fuller). Low density widens toward extremes.
    base_vols = [0.30, 0.55, 0.75, 0.95]
    density_bias = (density - 0.5) * 2.0  # -1.0 .. +1.0
    center_vol = 0.65
    adapted_vols = []
    for v in base_vols:
        # Lerp toward center for high density, away for low density
        compressed = center_vol + (v - center_vol) * (1.0 - density_bias * 0.3)
        adapted_vols.append(max(0.10, min(0.99, compressed)))

    # Build final layers
    layers = []
    for i in range(4):
        vs = adjusted[i] if i == 0 else adjusted[i] + 1
        ve = adjusted[i + 1]
        layers.append((vs, ve, adapted_vols[i]))

    return layers


def _dna_adapt_voice_sensitivity(voice_name: str, dna: dict) -> float:
    """
    Return a DNA-adjusted VelocitySensitivity for a voice.

    Brightness increases sensitivity for attack-prominent voices (kick, snare,
    tom, clap) — brighter presets make velocity differences more audible.
    """
    base = _voice_cfg(voice_name).get("velocity_sensitivity", 1.0)
    brightness = dna.get("brightness", 0.5)

    # Only modulate attack-prominent voices
    attack_voices = {"kick", "snare", "tom", "clap"}
    if voice_name not in attack_voices:
        return base

    # At brightness=0.5, no change. At 1.0, +10% sensitivity. At 0.0, -10%.
    brightness_mod = (brightness - 0.5) * 0.2
    return max(0.3, min(1.0, base + brightness_mod))


def _load_preset_dna(preset_name: str) -> dict:
    """
    Load the 6D Sonic DNA from a preset's .xometa file.
    Returns _DEFAULT_DNA if the preset or DNA is not found.
    """
    import json as _json
    slug = preset_name.replace(" ", "_")
    # Search through all mood directories for a matching preset
    for xmeta in PRESETS_DIR.rglob("*.xometa"):
        try:
            with open(xmeta) as f:
                data = _json.load(f)
            if data.get("name") == preset_name:
                dna = data.get("dna", {})
                if dna:
                    return {**_DEFAULT_DNA, **dna}
        except (KeyError, _json.JSONDecodeError, OSError):
            continue
    return dict(_DEFAULT_DNA)


def generate_xpm(preset_name: str, wav_map: dict,
                 kit_mode: str = "velocity",
                 dna: dict = None,
                 engine: str = None) -> str:
    """Generate complete drum program XPM XML string.

    If dna is None, attempts to load it from the preset's .xometa file.
    DNA shapes velocity curves and voice sensitivity per preset.
    If engine is provided, pad colors are set from the engine's accent color.
    """
    preset_slug = preset_name.replace(" ", "_")
    prog_name   = xml_escape(f"XO_OX-{preset_name}")

    # Load DNA for adaptive velocity curves
    if dna is None:
        dna = _load_preset_dna(preset_name)

    note_to_pad = {note: (voice, mg, mt) for note, voice, mg, mt in PAD_MAP}

    parts = []
    for i in range(128):
        if i in note_to_pad:
            voice, mg, mt = note_to_pad[i]
        else:
            voice, mg, mt = "", 0, [0, 0, 0, 0]

        parts.append(_instrument_block(
            instrument_num=i,
            voice_name=voice,
            mute_group=mg,
            mute_targets=mt,
            wav_map=wav_map,
            preset_slug=preset_slug,
            kit_mode=kit_mode,
            dna=dna,
        ))

    instruments_xml = "\n".join(parts)

    pad_json = json.dumps(
        {"ProgramPads": {"Universal": {"value0": False},
                         "Type":      {"value0": 5},
                         "universalPad": 32512}},
        separators=(",", ":"),
    )

    # PadNoteMap — maps physical pad numbers to MIDI notes (Rex's bible §4)
    pad_note_entries = []
    for pad_idx, (note, voice, _, _) in enumerate(PAD_MAP):
        pad_note_entries.append(
            f'        <Pad number="{pad_idx + 1}" note="{note}"/>'
            f'  <!-- {voice} -->'
        )
    pad_note_xml = "\n".join(pad_note_entries)

    # PadGroupMap — mute group assignments for hat choke
    pad_group_entries = []
    for pad_idx, (_, voice, mg, _) in enumerate(PAD_MAP):
        if mg > 0:
            pad_group_entries.append(
                f'        <Pad number="{pad_idx + 1}" group="{mg}"/>'
                f'  <!-- {voice} -->'
            )
    pad_group_xml = "\n".join(pad_group_entries)

    # Pad colors — derived from engine accent color
    pad_color_xml = ""
    if engine:
        hex_color = ENGINE_PAD_COLORS.get(engine, "")
        if hex_color:
            pad_color_entries = []
            for pad_idx in range(len(PAD_MAP)):
                pad_color_entries.append(
                    f'        <Pad number="{pad_idx + 1}">'
                    f'<PadColor>{hex_color}</PadColor>'
                    f'</Pad>'
                )
            pad_color_xml = (
                '    <PadColors>\n'
                + "\n".join(pad_color_entries) + "\n"
                + '    </PadColors>\n'
            )

    # Q-Link assignments — 4 knobs per program for macro control
    qlink_xml = _generate_qlink_xml()

    # Expression mapping: AfterTouch → FilterCutoff (amount 30) for drums
    expression_xml = (
        '    <ExpressionMappings>\n'
        '      <ExpressionMapping number="1">\n'
        '        <Source>AfterTouch</Source>\n'
        '        <Destination>FilterCutoff</Destination>\n'
        '        <Amount>30</Amount>\n'
        '      </ExpressionMapping>\n'
        '    </ExpressionMappings>\n'
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
        f'    <ProgramPads>{pad_json}</ProgramPads>\n'
        '    <PadNoteMap>\n'
        f'{pad_note_xml}\n'
        '    </PadNoteMap>\n'
        '    <PadGroupMap>\n'
        f'{pad_group_xml}\n'
        '    </PadGroupMap>\n'
        f'{pad_color_xml}'
        f'{expression_xml}'
        f'{qlink_xml}'
        '    <Instruments>\n'
        f'{instruments_xml}\n'
        '    </Instruments>\n'
        '  </Program>\n'
        '</MPCVObject>\n'
    )


# =============================================================================
# EXPANSION.XML
# =============================================================================

def generate_expansion_xml(pack_name: str, pack_id: str,
                            description: str, version: str = "1.0.0") -> str:
    today = str(date.today())
    return (
        '<?xml version="1.0" encoding="UTF-8"?>\n\n'
        f'<expansion version="2.0.0.0" buildVersion="2.10.0.0">\n'
        f'  <local/>\n'
        f'  <identifier>{xml_escape(pack_id)}</identifier>\n'
        f'  <title>{xml_escape(pack_name)}</title>\n'
        f'  <manufacturer>XO_OX Designs</manufacturer>\n'
        f'  <version>{version}.0</version>\n'
        f'  <type>drum</type>\n'
        f'  <priority>50</priority>\n'
        f'  <img>artwork.png</img>\n'
        f'  <description>{xml_escape(description)}</description>\n'
        f'  <separator>-</separator>\n'
        f'</expansion>\n'
    )


# =============================================================================
# WAV MAP
# =============================================================================

def build_wav_map(wavs_dir: Path, preset_slug: str,
                  kit_mode: str = "velocity") -> dict:
    """
    Build {stem: filename} lookup for all WAVs that belong to this preset.
    Handles both velocity (v1-v4) and cycle/random (c1-c4) naming.
    """
    wav_map = {}
    if not wavs_dir or not wavs_dir.exists():
        return wav_map
    for wav_file in sorted(wavs_dir.glob("*.wav")):
        stem = wav_file.stem
        if stem.startswith(preset_slug):
            wav_map[stem] = wav_file.name
    return wav_map


def _required_wavs(preset_slug: str, kit_mode: str) -> list[str]:
    """Return the list of WAV filenames required for a given preset + mode."""
    files = []
    for _, voice, _, _ in PAD_MAP:
        effective = _resolve_mode(kit_mode, voice)
        if effective == "velocity":
            for suf in VEL_SUFFIXES:
                files.append(f"{preset_slug}_{voice}_{suf}.wav")
        else:
            for suf in CYCLE_SUFFIXES:
                files.append(f"{preset_slug}_{voice}_{suf}.wav")
    return files


def print_wav_checklist(preset_name: str, kit_mode: str = "velocity"):
    slug = preset_name.replace(" ", "_")
    print(f"\nWAV checklist for '{preset_name}' — mode: {kit_mode}\n")
    for note, voice, _, _ in PAD_MAP:
        effective = _resolve_mode(kit_mode, voice)
        cfg = _voice_cfg(voice)
        suffixes = VEL_SUFFIXES if effective == "velocity" else CYCLE_SUFFIXES
        note_desc = "vel layers (pp→ff)" if effective == "velocity" else f"variants ({effective})"
        print(f"  {voice:6s}  [{note_desc}]")
        for suf in suffixes:
            print(f"    {slug}_{voice}_{suf}.wav")
    print()


# =============================================================================
# PACK BUILDER
# =============================================================================

def build_drum_pack(preset_name: str, wavs_dir: Path, output_dir: Path,
                    pack_id: str = None, version: str = "1.0",
                    description: str = "", kit_mode: str = "velocity",
                    dry_run: bool = False, generate_art: bool = True) -> dict:
    """Build a complete drum XPN pack for one XOnset preset."""
    preset_slug = preset_name.replace(" ", "_")
    pack_id   = pack_id or f"com.xo-ox.onset.{preset_slug.lower()}"
    pack_dir  = output_dir / preset_slug
    if not dry_run:
        pack_dir.mkdir(parents=True, exist_ok=True)

    wav_map = build_wav_map(wavs_dir, preset_slug, kit_mode) if wavs_dir else {}
    required = _required_wavs(preset_slug, kit_mode)
    missing  = [f for f in required if f.replace(".wav", "") not in wav_map]

    # Load DNA for adaptive velocity curves
    dna = _load_preset_dna(preset_name)
    xpm_content = generate_xpm(preset_name, wav_map, kit_mode, dna=dna)
    xpm_path    = pack_dir / f"{preset_slug}.xpm"
    if not dry_run:
        xpm_path.write_text(xpm_content, encoding="utf-8")
        print(f"  XPM:  {xpm_path.name}  (mode={kit_mode})")

    if not description:
        description = (
            f"XOnset drum kit — {preset_name}. "
            f"Dual-layer synthesis percussion by XO_OX Designs."
        )
    exp_content = generate_expansion_xml(
        pack_name=f"XOnset: {preset_name}",
        pack_id=pack_id,
        description=description,
        version=version,
    )
    if not dry_run:
        (pack_dir / "Expansion.xml").write_text(exp_content, encoding="utf-8")
        print(f"  Manifest: Expansion.xml")

    if generate_art and not dry_run and COVER_ART_AVAILABLE:
        try:
            generate_cover(
                engine="ONSET", pack_name=preset_name,
                output_dir=str(pack_dir), preset_count=1,
                version=version, seed=hash(preset_name) % 10000,
            )
        except Exception as e:
            print(f"  [WARN] Cover art: {e}")

    if wavs_dir and not dry_run:
        import shutil
        # Rex audit fix: WAVs must go into Samples/{slug}/ to match XPM <File> paths
        samples_subdir = pack_dir / "Samples" / preset_slug
        samples_subdir.mkdir(parents=True, exist_ok=True)
        copied = 0
        for wav_name in wav_map.values():
            src = wavs_dir / wav_name
            dst = samples_subdir / wav_name
            if src.exists() and not dst.exists():
                shutil.copy2(src, dst)
                copied += 1
        if copied:
            print(f"  WAVs: {copied} copied to Samples/{preset_slug}/")

    if missing:
        print(f"  [MISSING] {len(missing)} WAVs — run --checklist to see full list")

    return {"pack_dir": str(pack_dir), "missing_wavs": missing}


_ONSET_ENGINE_IDS = {"Onset", "OnsetEngine"}


def build_all_onset_packs(wavs_dir: Path, output_dir: Path,
                          version: str = "1.0", kit_mode: str = "velocity",
                          dry_run: bool = False) -> list:
    onset_presets = []
    import json as _json
    for xmeta in sorted(PRESETS_DIR.rglob("*.xometa")):
        try:
            with open(xmeta) as f:
                data = _json.load(f)
            engines = data.get("engines", [])
            if isinstance(engines, str):
                engines = [engines]
            if not any(e in _ONSET_ENGINE_IDS for e in engines):
                continue
            onset_presets.append({
                "name":        data["name"],
                "mood":        data.get("mood", ""),
                "description": data.get("description", ""),
            })
        except (KeyError, _json.JSONDecodeError, OSError) as exc:
            print(f"  [WARN] Skipping {xmeta.name}: {exc}")
            continue

    print(f"Found {len(onset_presets)} XOnset presets")
    results = []
    for p in onset_presets:
        print(f"\nBuilding: {p['name']} ({p['mood']})")
        results.append(build_drum_pack(
            preset_name=p["name"],
            wavs_dir=wavs_dir,
            output_dir=output_dir,
            version=version,
            description=p["description"],
            kit_mode=kit_mode,
            dry_run=dry_run,
            generate_art=True,
        ))
    return results


# =============================================================================
# CLI
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Drum Program Exporter — XO_OX Designs",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--preset",     help="Single preset name to export")
    parser.add_argument("--all-onset",  action="store_true",
                        help="Export all XOnset presets")
    parser.add_argument("--mode",       default="velocity",
                        choices=["velocity", "cycle", "random",
                                 "random-norepeat", "smart"],
                        help="Layer selection mode (default: velocity)")
    parser.add_argument("--vel-curve",  default="musical",
                        choices=["musical", "even"],
                        help="Velocity split curve (default: musical/Vibe-approved)")
    parser.add_argument("--wavs-dir",   help="Directory containing WAV files")
    parser.add_argument("--output-dir", default=".", help="Output directory")
    parser.add_argument("--version",    default="1.0")
    parser.add_argument("--dry-run",    action="store_true")
    parser.add_argument("--checklist",  metavar="PRESET",
                        help="Print required WAV filenames for a preset + mode")
    parser.add_argument("--no-art",     action="store_true")
    args = parser.parse_args()

    if args.checklist:
        print_wav_checklist(args.checklist, args.mode)
        return 0

    output_dir = Path(args.output_dir)
    wavs_dir   = Path(args.wavs_dir) if args.wavs_dir else None

    # Apply velocity curve selection (set module-level default for this CLI run)
    _set_vel_curve(args.vel_curve)

    if args.dry_run:
        print(f"DRY RUN — mode: {args.mode}  vel-curve: {args.vel_curve}\n")

    if args.all_onset:
        build_all_onset_packs(wavs_dir, output_dir, args.version,
                              args.mode, args.dry_run)
    elif args.preset:
        print(f"Building drum pack: {args.preset}  mode={args.mode}")
        build_drum_pack(
            preset_name=args.preset,
            wavs_dir=wavs_dir,
            output_dir=output_dir,
            version=args.version,
            kit_mode=args.mode,
            dry_run=args.dry_run,
            generate_art=not args.no_art,
        )
    else:
        parser.print_help()
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
