#!/usr/bin/env python3
"""
xpn_tide_tables_pack_builder.py
Builds the TIDE TABLES free gateway pack spec for XO_OX Patreon.

Generates:
  Docs/packs/tide_tables/tide_tables_spec.json
  Docs/packs/tide_tables/TIDE_TABLES_README.md
"""

import json
import os
import pathlib
import textwrap
from datetime import date

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT_DIR = REPO_ROOT / "Docs" / "packs" / "tide_tables"

TODAY = date.today().isoformat()

# ---------------------------------------------------------------------------
# Pad helpers
# ---------------------------------------------------------------------------

def make_pad(number, name, preset, note, velocity_low=0, velocity_high=127,
             layer_mode="single", notes=None):
    """Return a single pad entry dict."""
    entry = {
        "pad": number,
        "name": name,
        "preset": preset,
        "midi_note": note,
        "velocity_low": velocity_low,
        "velocity_high": velocity_high,
        "layer_mode": layer_mode,
    }
    if notes:
        entry["notes"] = notes
    return entry


# ---------------------------------------------------------------------------
# Program definitions
# ---------------------------------------------------------------------------

def program_incoming_tide():
    """Program 1: ONSET showcase — drums and percussion."""
    pads = [
        make_pad(1,  "Kick Deep",        "ONSET:Kick > Trench Kick",         36, notes="Sub-heavy kick, MACHINE macro pushed high"),
        make_pad(2,  "Kick Snap",        "ONSET:Kick > Shore Break",         36, notes="Punchy mid-forward kick, PUNCH macro at 70%"),
        make_pad(3,  "Snare Crack",      "ONSET:Snare > Brine Snap",         38, notes="Sharp transient snare, PUNCH at max"),
        make_pad(4,  "Snare Brush",      "ONSET:Snare > Foam Roll",          38, notes="Brushed snare, lower PUNCH, SPACE open"),
        make_pad(5,  "Closed Hat",       "ONSET:CHat > Ripple Hat",          42, notes="Tight hi-hat, MACHINE at 50%"),
        make_pad(6,  "Open Hat",         "ONSET:OHat > Crest Hat",           46, notes="Open hat with long decay, SPACE at 60%"),
        make_pad(7,  "Clap",             "ONSET:Clap > Tidal Clap",          39, notes="Wide stereo clap, SPACE adds room"),
        make_pad(8,  "Rim Shot",         "ONSET:Snare > Reef Rim",           37, notes="Bright rimshot, MUTATE adds grit"),
        make_pad(9,  "Tom High",         "ONSET:Tom > High Water Tom",       48, notes="High pitched tom, PITCH up"),
        make_pad(10, "Tom Low",          "ONSET:Tom > Deep Current Tom",     45, notes="Low tuned tom, sub resonance"),
        make_pad(11, "Perc Shaker",      "ONSET:Perc > Drift Shaker",        56, notes="Continuous shaker texture, MUTATE varies tone"),
        make_pad(12, "Perc Clave",       "ONSET:Perc > Sandbar Clave",       75, notes="Dry wood clave, tight SPACE"),
        make_pad(13, "FX Swell",         "ONSET:FX > Incoming Surge",        37, notes="Rising ocean FX swell, full SPACE"),
        make_pad(14, "FX Crash",         "ONSET:FX > Whitecap Crash",        49, notes="Cymbal-like crash built from ONSET noise"),
        make_pad(15, "Full Kit A",       "ONSET:Kick > Trench Kick",         36, notes="Performance pad — kick + snare layered via MUTATE"),
        make_pad(16, "Full Kit B",       "ONSET:Perc > Drift Shaker",        56, notes="Performance pad — perc texture fill"),
    ]
    return {
        "name": "Incoming Tide",
        "type": "DrumProgram",
        "engine": "ONSET",
        "description": (
            "Pure percussion. ONSET's eight synthesis voices map across the kit: "
            "kick, snare, hats, clap, toms, perc, and two FX slots. "
            "MACHINE macro dials between acoustic-adjacent and machine-rigid. "
            "PUNCH shapes transient aggression. SPACE opens the reverb field. "
            "MUTATE introduces timbral randomness — nudge it for organic variation."
        ),
        "macro_map": {
            "Q1": "MACHINE — acoustic ↔ machine character",
            "Q2": "PUNCH — transient attack across all voices",
            "Q3": "SPACE — global reverb field depth",
            "Q4": "MUTATE — timbral randomness / controlled chaos",
        },
        "pads": pads,
    }


def program_open_water():
    """Program 2: ODYSSEY showcase — melodic / synth, keygroup style on pads."""
    # Chromatic layout across two octaves starting at C3 (midi 48)
    chromatic_notes = [48, 50, 52, 53, 55, 57, 59, 60, 62, 64, 65, 67, 69, 71, 72, 74]
    note_names = ["C3","D3","E3","F3","G3","A3","B3","C4","D4","E4","F4","G4","A4","B4","C5","D5"]

    preset_cycle = [
        "ODYSSEY:Lead > Oceanic Saw",
        "ODYSSEY:Pad > Pelagic Wash",
        "ODYSSEY:Bass > Abyssal Sub",
        "ODYSSEY:Arp > Current Sequence",
    ]

    pads = []
    for i in range(16):
        preset = preset_cycle[i % len(preset_cycle)]
        pads.append(make_pad(
            i + 1,
            note_names[i],
            preset,
            chromatic_notes[i],
            notes="Chromatic keygroup layout — two octaves C3–D5",
        ))

    return {
        "name": "Open Water",
        "type": "DrumProgram",
        "description": (
            "ODYSSEY's wavetable/subtractive engine laid out chromatically across two octaves. "
            "Pads 1–4 cycle through four tonal characters: Lead, Pad, Bass, Arp. "
            "The layout mirrors a keygroup so producers can play melodies directly off the pads. "
            "DRIFT macro introduces pitch instability — from studio-clean to open-sea detuned. "
            "DEPTH controls filter evolution. WAVE morphs between wavetable frames. "
            "ECHO sends into the built-in tape-style delay."
        ),
        "engine": "ODYSSEY",
        "macro_map": {
            "Q1": "DRIFT — pitch instability / detuning width",
            "Q2": "DEPTH — filter sweep depth",
            "Q3": "WAVE — wavetable morph position",
            "Q4": "ECHO — tape delay send level",
        },
        "pads": pads,
    }


def program_surface_tension():
    """Program 3: OPAL showcase — granular textures, ambient."""
    pad_specs = [
        (1,  "Grain Cloud A",  "OPAL:Texture > Bioluminescence",   60, "Dense slow grain cloud, high density"),
        (2,  "Grain Cloud B",  "OPAL:Texture > Thermal Plume",     60, "Rising warm grain mass, position sweeps up"),
        (3,  "Scatter A",      "OPAL:Scatter > Foam Scatter",      62, "High scatter, short grain size, bright"),
        (4,  "Scatter B",      "OPAL:Scatter > Depth Scatter",     57, "Low scatter, long grains, dark and slow"),
        (5,  "Freeze A",       "OPAL:Freeze > Ice Shelf",          64, "Frozen grain loop, sustained shimmer"),
        (6,  "Freeze B",       "OPAL:Freeze > Permafrost",         59, "Dense freeze, subtle pitch modulation"),
        (7,  "Pitch Grain A",  "OPAL:Pitched > Coral Chord",       65, "Pitched grains forming a chord cluster"),
        (8,  "Pitch Grain B",  "OPAL:Pitched > Kelp Drone",        55, "Deep pitched drone grains, slow attack"),
        (9,  "Motion A",       "OPAL:Motion > Tidal Drift",        67, "Position LFO creates sweeping motion"),
        (10, "Motion B",       "OPAL:Motion > Undertow",           53, "Reverse-sweep position motion, dark pull"),
        (11, "Stutter A",      "OPAL:Stutter > Chop Break",        69, "Rapid grain stutter, rhythmic texture"),
        (12, "Stutter B",      "OPAL:Stutter > Tide Pool Drip",    48, "Sparse stutter, organic drop pattern"),
        (13, "Atmos A",        "OPAL:Atmos > Open Sea Breath",     72, "Wide stereo atmosphere, all SPACE"),
        (14, "Atmos B",        "OPAL:Atmos > Cave Echo",           45, "Narrow dark atmosphere, heavy reverb"),
        (15, "FX Sweep",       "OPAL:FX > Current Sweep",          74, "Filter sweep FX grain, performance use"),
        (16, "FX Burst",       "OPAL:FX > Surge Burst",            36, "One-shot grain burst, full transient"),
    ]

    pads = [make_pad(n, name, preset, note, notes=notes)
            for n, name, preset, note, notes in pad_specs]

    return {
        "name": "Surface Tension",
        "type": "DrumProgram",
        "engine": "OPAL",
        "description": (
            "OPAL's granular synthesis engine across six texture categories: "
            "Cloud, Scatter, Freeze, Pitched, Motion, Stutter — plus two Atmos and two FX slots. "
            "Each pair presents the same category in contrasting characters (bright/dark, fast/slow). "
            "DENSITY controls grain population. SIZE sets grain duration. "
            "POSITION scrubs the source position. SCATTER randomizes grain placement. "
            "Push all four macros for maximum granular chaos; pull back for intimate, breath-like textures."
        ),
        "macro_map": {
            "Q1": "DENSITY — grain population (sparse to cloud)",
            "Q2": "SIZE — grain duration (micro to long)",
            "Q3": "POSITION — source scan position",
            "Q4": "SCATTER — spatial randomization",
        },
        "pads": pads,
    }


def program_full_spectrum():
    """Program 4: all three engines coupled — Entangled mood."""
    pad_specs = [
        # Rhythmic foundation — ONSET
        (1,  "Kick",          "ONSET:Kick > Trench Kick",          36, "ONSET"),
        (2,  "Snare",         "ONSET:Snare > Brine Snap",          38, "ONSET"),
        (3,  "Hat",           "ONSET:CHat > Ripple Hat",           42, "ONSET"),
        (4,  "Perc",          "ONSET:Perc > Drift Shaker",         56, "ONSET"),
        # Harmonic layer — ODYSSEY
        (5,  "Bass",          "ODYSSEY:Bass > Abyssal Sub",        36, "ODYSSEY"),
        (6,  "Lead",          "ODYSSEY:Lead > Oceanic Saw",        60, "ODYSSEY"),
        (7,  "Pad",           "ODYSSEY:Pad > Pelagic Wash",        65, "ODYSSEY"),
        (8,  "Arp",           "ODYSSEY:Arp > Current Sequence",    67, "ODYSSEY"),
        # Texture layer — OPAL
        (9,  "Grain Atmos",   "OPAL:Atmos > Open Sea Breath",      72, "OPAL"),
        (10, "Grain Motion",  "OPAL:Motion > Tidal Drift",         67, "OPAL"),
        (11, "Grain Freeze",  "OPAL:Freeze > Ice Shelf",           64, "OPAL"),
        (12, "Grain Scatter", "OPAL:Scatter > Foam Scatter",       62, "OPAL"),
        # Coupled performance pads
        (13, "Entangled 1",   "COUPLED:ONSET+OPAL > Tide Break",   36, "ONSET+OPAL"),
        (14, "Entangled 2",   "COUPLED:ODYSSEY+OPAL > Deep Bloom", 60, "ODYSSEY+OPAL"),
        (15, "Full Surge",    "COUPLED:ALL > Full Spectrum Surge",  48, "ALL THREE"),
        (16, "Reset",         "COUPLED:ALL > Return to Shore",      36, "ALL THREE — soft version"),
    ]

    pads = []
    for n, name, preset, note, engine_tag in pad_specs:
        pads.append(make_pad(n, name, preset, note, notes=f"Engine: {engine_tag}"))

    return {
        "name": "Full Spectrum",
        "type": "DrumProgram",
        "engine": "ONSET + ODYSSEY + OPAL",
        "mood": "Entangled",
        "description": (
            "The full XO_OX water column in one program. "
            "Pads 1–4: ONSET rhythmic core. Pads 5–8: ODYSSEY harmonic layer. "
            "Pads 9–12: OPAL granular textures. Pads 13–16: coupled performance pads "
            "where two or three engines are entangled — their parameters cross-modulate. "
            "This is the Entangled mood: no engine operates alone. "
            "Use as a compositional sketch pad or a live performance set."
        ),
        "coupling_notes": (
            "Entangled mood couples ONSET transient data → OPAL grain trigger density, "
            "ODYSSEY filter cutoff → OPAL scatter amount, "
            "OPAL density → ODYSSEY reverb send. "
            "All three share a common SPACE macro for unified room feel."
        ),
        "macro_map": {
            "Q1": "DRIVE — saturation across all three engines",
            "Q2": "SPACE — unified reverb field",
            "Q3": "ENTANGLE — cross-modulation depth",
            "Q4": "TIDE — tempo-synced global LFO",
        },
        "pads": pads,
    }


# ---------------------------------------------------------------------------
# Pack spec assembly
# ---------------------------------------------------------------------------

def build_pack_spec():
    return {
        "pack_name": "TIDE TABLES",
        "tagline": "Where the water meets the shore",
        "version": "1.0.0",
        "release_date": TODAY,
        "price": "free",
        "patreon_tier": "Signal",
        "format": "MPC Expansion (.xpn)",
        "engines": ["ONSET", "ODYSSEY", "OPAL"],
        "mood": "Entangled (primary) / Foundation (Program 1) / Atmosphere (Programs 2–3)",
        "target_audience": (
            "New listeners discovering XO_OX for the first time. "
            "Producers who want a taste of all three flagship engines before committing to the full XOlokun library."
        ),
        "description": (
            "TIDE TABLES is the free gateway pack for XO_OX. "
            "Four programs trace a journey from the shore to the open sea: "
            "ONSET builds the rhythmic foundation (Incoming Tide), "
            "ODYSSEY opens the harmonic layer (Open Water), "
            "OPAL dissolves into granular texture (Surface Tension), "
            "and Full Spectrum couples all three in the Entangled mood. "
            "Everything here is synthesized — no samples. "
            "Every sound was born in the engine."
        ),
        "programs": [
            program_incoming_tide(),
            program_open_water(),
            program_surface_tension(),
            program_full_spectrum(),
        ],
        "marketing_hooks": [
            "100% free — no catch, no email wall. Just download and make something.",
            "Three flagship XO_OX engines in one pack: ONSET (drums), ODYSSEY (synth), OPAL (granular).",
            "64 pads across 4 programs, all synthesized from scratch — zero sample playback.",
            "Entangled program shows XOlokun engine coupling: rhythms that breathe, pads that pulse.",
            "Gateway to the full XOlokun library — every engine here has a deeper version in the main release.",
            "MPC-native format: drop it in MPC Software or MPC Live/One/X and play immediately.",
        ],
        "release_checklist": [
            "[ ] Record 30-second audio demo of each program (4 clips total)",
            "[ ] Export .xpn bundle using xpn_pack_builder.py or MPC Software export",
            "[ ] Upload to XO-OX.org downloads page",
            "[ ] Post to Patreon Signal tier as free public post",
            "[ ] Cross-post to Instagram with audio demo clip (Program 4 preferred)",
            "[ ] Pin to top of Patreon page",
            "[ ] Link from XO-OX.org index hero section",
            "[ ] Add to XO-OX.org /packs page as featured free pack",
            "[ ] Update www.patreon.com/cw/XO_OX bio link to point to TIDE TABLES landing page",
        ],
        "build_notes": {
            "preset_references": (
                "Preset strings use the format ENGINE:Category > Preset Name. "
                "These are reference labels for the sound designer — map them to actual "
                "XOlokun preset IDs during the XPN build step."
            ),
            "xpn_build_command": "python3 Tools/xpn_pack_builder.py Docs/packs/tide_tables/tide_tables_spec.json",
            "next_steps": [
                "Run xpn_pack_builder.py to generate .xpn bundle",
                "Validate in MPC Software",
                "Record audio demos",
                "Release",
            ],
        },
    }


# ---------------------------------------------------------------------------
# README
# ---------------------------------------------------------------------------

README_TEMPLATE = """\
# TIDE TABLES
### A free MPC expansion from XO_OX
**"Where the water meets the shore"**

---

## What is this?

TIDE TABLES is a free gateway pack for the XO_OX synthesizer library.
It showcases three core engines from XOlokun — ONSET, ODYSSEY, and OPAL — in a single
MPC expansion you can load today at no cost.

No samples. No loops. Every sound is synthesized in real time.

---

## Four Programs

### 1. Incoming Tide — *ONSET*
Pure percussion. ONSET's eight synthesis voices cover the full kit:
kick, snare, hats, clap, toms, perc, and two FX slots.

| Macro | Function |
|-------|----------|
| Q1 MACHINE | Acoustic ↔ machine character |
| Q2 PUNCH   | Transient attack across all voices |
| Q3 SPACE   | Global reverb field depth |
| Q4 MUTATE  | Timbral randomness |

Start here. Build the groove.

---

### 2. Open Water — *ODYSSEY*
ODYSSEY's wavetable/subtractive engine laid out chromatically across two octaves (C3–D5).
Pads cycle through four tonal characters: Lead, Pad, Bass, Arp.
Play melodies straight off the pads.

| Macro | Function |
|-------|----------|
| Q1 DRIFT | Pitch instability / detuning width |
| Q2 DEPTH | Filter sweep depth |
| Q3 WAVE  | Wavetable morph position |
| Q4 ECHO  | Tape delay send level |

---

### 3. Surface Tension — *OPAL*
OPAL's granular engine across six texture categories:
Cloud, Scatter, Freeze, Pitched, Motion, Stutter.
Each category comes in two contrasting flavors — bright/dark, fast/slow.

| Macro | Function |
|-------|----------|
| Q1 DENSITY  | Grain population (sparse to cloud) |
| Q2 SIZE     | Grain duration (micro to long) |
| Q3 POSITION | Source scan position |
| Q4 SCATTER  | Spatial randomization |

---

### 4. Full Spectrum — *ONSET + ODYSSEY + OPAL* (Entangled)
All three engines in one program. Pads 1–4 are ONSET rhythm.
Pads 5–8 are ODYSSEY harmony. Pads 9–12 are OPAL texture.
Pads 13–16 are coupled performance pads where engines cross-modulate.

This is the **Entangled mood**: no engine operates alone.

| Macro | Function |
|-------|----------|
| Q1 DRIVE    | Saturation across all three engines |
| Q2 SPACE    | Unified reverb field |
| Q3 ENTANGLE | Cross-modulation depth |
| Q4 TIDE     | Tempo-synced global LFO |

---

## How to Load

1. Open MPC Software or boot your MPC hardware.
2. Go to **Menu → Browse → Expansions**.
3. Navigate to the TIDE TABLES .xpn file and double-click to install.
4. Find "TIDE TABLES" in your expansion library.
5. Load any of the four programs and start playing.

---

## Want More?

TIDE TABLES is a taste. The full XOlokun library contains **31+ engines**,
2,369+ factory presets, and the complete XO_OX water column mythology.

Visit **xo-ox.org** to learn more, or join Patreon at the Signal tier
to receive new packs as they're released.

---

## Credits

All sounds: XO_OX
Engines: ONSET, ODYSSEY, OPAL (XOlokun)
Format: Akai MPC Expansion (.xpn)
Released: {release_date}
License: Free for personal and commercial use

---

*"Every tide starts somewhere."*
"""


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    # 1. Build spec
    spec = build_pack_spec()
    spec_path = OUT_DIR / "tide_tables_spec.json"
    with open(spec_path, "w", encoding="utf-8") as f:
        json.dump(spec, f, indent=2, ensure_ascii=False)

    # 2. Write README
    readme_path = OUT_DIR / "TIDE_TABLES_README.md"
    with open(readme_path, "w", encoding="utf-8") as f:
        f.write(README_TEMPLATE.format(release_date=TODAY))

    # 3. Summary
    total_pads = sum(len(p["pads"]) for p in spec["programs"])
    print("=" * 60)
    print("TIDE TABLES pack builder — build complete")
    print("=" * 60)
    print(f"  Output dir  : {OUT_DIR}")
    print(f"  Spec JSON   : {spec_path.name}")
    print(f"  README      : {readme_path.name}")
    print()
    print(f"  Pack name   : {spec['pack_name']}")
    print(f"  Tagline     : {spec['tagline']}")
    print(f"  Price       : {spec['price']}")
    print(f"  Tier        : {spec['patreon_tier']}")
    print(f"  Engines     : {', '.join(spec['engines'])}")
    print(f"  Programs    : {len(spec['programs'])}")
    print(f"  Total pads  : {total_pads}")
    print()
    print("  Programs:")
    for i, prog in enumerate(spec["programs"], 1):
        print(f"    {i}. {prog['name']} ({prog['engine']}) — {len(prog['pads'])} pads")
    print()
    print("  Release checklist items:", len(spec["release_checklist"]))
    print("  Marketing hooks        :", len(spec["marketing_hooks"]))
    print()
    print("  Next step:")
    print(f"    {spec['build_notes']['xpn_build_command']}")
    print("=" * 60)


if __name__ == "__main__":
    main()
