#!/usr/bin/env python3
"""
xpn_tide_tables_builder.py

TIDE TABLES — free gateway pack for XO_OX
Showcases ONSET (drums), ODYSSEY (leads/pads), OPAL (textures/granular).

Outputs:
  Docs/packs/tide_tables/tide_tables_pack_spec.json
  Docs/packs/tide_tables/TIDE_TABLES_README.md
  Docs/packs/tide_tables/tide_tables_marketing.md
"""

import json
import os
import pathlib
from datetime import date

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT_DIR = REPO_ROOT / "Docs" / "packs" / "tide_tables"
TODAY = date.today().isoformat()


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def pad_assignments(start_note=36, count=16):
    """Return a list of MIDI notes C2–D#3 (36–51)."""
    return list(range(start_note, start_note + count))


def make_pad(number, name, preset, midi_note, vel_low=0, vel_high=127, layer_mode="single", notes=None):
    entry = {
        "pad": number,
        "name": name,
        "preset": preset,
        "midi_note": midi_note,
        "velocity_low": vel_low,
        "velocity_high": vel_high,
        "layer_mode": layer_mode,
    }
    if notes:
        entry["notes"] = notes
    return entry


def make_keygroup(root_note, vel_layers, envelope):
    return {
        "root_note": root_note,
        "velocity_layers": vel_layers,
        "envelope": envelope,
    }


def make_qlink(index, label, param, min_val, max_val, curve="linear"):
    return {
        "qlink": index,
        "label": label,
        "parameter": param,
        "min": min_val,
        "max": max_val,
        "curve": curve,
    }


def make_variation(name, mood_tags, dna_delta=None):
    return {
        "name": name,
        "mood_tags": mood_tags,
        "dna_delta": dna_delta or {},
    }


# ---------------------------------------------------------------------------
# Program 1 — Coastal Drums (ONSET)
# ---------------------------------------------------------------------------

COASTAL_DRUMS_PADS = [
    make_pad(1,  "Kick Punch",     "perc_kick_solid",      36),
    make_pad(2,  "Kick Deep",      "perc_kick_deep",       37),
    make_pad(3,  "Snare Crack",    "perc_snare_crack",     38),
    make_pad(4,  "Snare Brush",    "perc_snare_brush",     39),
    make_pad(5,  "Closed Hat",     "perc_chat_tight",      40),
    make_pad(6,  "Open Hat",       "perc_ohat_long",       41),
    make_pad(7,  "Clap Classic",   "perc_clap_classic",    42),
    make_pad(8,  "Clap Layer",     "perc_clap_layer",      43),
    make_pad(9,  "Tom Low",        "perc_tom_low",         44),
    make_pad(10, "Tom Mid",        "perc_tom_mid",         45),
    make_pad(11, "Perc Rim",       "perc_perc_rim",        46),
    make_pad(12, "Perc Shaker",    "perc_perc_shaker",     47),
    make_pad(13, "FX Crash",       "perc_fx_crash",        48),
    make_pad(14, "FX Rise",        "perc_fx_rise",         49),
    make_pad(15, "FX Sweep",       "perc_fx_sweep",        50),
    make_pad(16, "FX Glitch",      "perc_fx_glitch",       51),
]

COASTAL_DRUMS_KEYGROUP = make_keygroup(
    root_note=36,
    vel_layers=[
        {"low": 0,   "high": 63,  "label": "soft"},
        {"low": 64,  "high": 100, "label": "medium"},
        {"low": 101, "high": 127, "label": "hard"},
    ],
    envelope={"attack": 0.001, "decay": 0.15, "sustain": 0.0, "release": 0.08},
)

COASTAL_DRUMS_QLINKS = [
    make_qlink(1, "MACHINE", "perc_macroMachine", 0.0, 1.0),
    make_qlink(2, "PUNCH",   "perc_macroPunch",   0.0, 1.0),
    make_qlink(3, "SPACE",   "perc_macroSpace",   0.0, 1.0),
    make_qlink(4, "MUTATE",  "perc_macroMutate",  0.0, 1.0),
]

COASTAL_DRUMS_DNA = {
    "brightness": 0.55,
    "warmth":     0.45,
    "movement":   0.70,
    "density":    0.60,
    "space":      0.40,
    "aggression": 0.65,
}

COASTAL_DRUMS_VARIATIONS = [
    make_variation("Shoreline Kit",    ["Foundation", "Prism"],      {"movement": 0.75}),
    make_variation("Lagoon Breaks",    ["Atmosphere"],               {"space": 0.60}),
    make_variation("Reef Stomp",       ["Flux"],                     {"aggression": 0.80}),
    make_variation("Tide Pool Snap",   ["Prism"],                    {"brightness": 0.70}),
    make_variation("Rip Current",      ["Flux", "Entangled"],        {"movement": 0.90, "aggression": 0.75}),
    make_variation("Dawn Patrol",      ["Foundation", "Atmosphere"], {"warmth": 0.65}),
    make_variation("Surge Pocket",     ["Entangled"],                {"density": 0.80}),
    make_variation("Wash Out",         ["Aether"],                   {"space": 0.80, "aggression": 0.30}),
]

COASTAL_DRUMS = {
    "program_name":  "Coastal Drums",
    "engine":        "ONSET",
    "engine_prefix": "perc_",
    "description":   "Eight synthesis voices — kick, snare, hats, clap, toms, perc, and two FX slots. ONSET's full kit in a single program. Start here.",
    "pads":          COASTAL_DRUMS_PADS,
    "keygroup":      COASTAL_DRUMS_KEYGROUP,
    "qlinks":        COASTAL_DRUMS_QLINKS,
    "dna_vector":    COASTAL_DRUMS_DNA,
    "variations":    COASTAL_DRUMS_VARIATIONS,
}


# ---------------------------------------------------------------------------
# Program 2 — Tide Lead (ODYSSEY)
# ---------------------------------------------------------------------------

TIDE_LEAD_PAD_NAMES = [
    "Lead C3",  "Lead D3",  "Lead E3",  "Lead F3",
    "Lead G3",  "Lead A3",  "Lead B3",  "Pad C4",
    "Pad D4",   "Pad E4",   "Pad F4",   "Bass C3",
    "Bass G2",  "Arp C4",   "Arp G4",   "Arp E4",
]

TIDE_LEAD_PRESETS = [
    "drift_lead_bright", "drift_lead_mid",   "drift_lead_warm",  "drift_lead_edge",
    "drift_lead_hollow", "drift_lead_glass",  "drift_lead_drive", "drift_pad_wide",
    "drift_pad_shimmer", "drift_pad_choir",  "drift_pad_string", "drift_bass_deep",
    "drift_bass_sub",    "drift_arp_fast",   "drift_arp_gate",   "drift_arp_rise",
]

TIDE_LEAD_PADS = [
    make_pad(i + 1, TIDE_LEAD_PAD_NAMES[i], TIDE_LEAD_PRESETS[i], 36 + i)
    for i in range(16)
]

TIDE_LEAD_KEYGROUP = make_keygroup(
    root_note=60,
    vel_layers=[
        {"low": 0,   "high": 89,  "label": "soft"},
        {"low": 90,  "high": 127, "label": "hard"},
    ],
    envelope={"attack": 0.008, "decay": 0.30, "sustain": 0.75, "release": 0.35},
)

TIDE_LEAD_QLINKS = [
    make_qlink(1, "DRIFT", "drift_macroDrift", 0.0, 1.0),
    make_qlink(2, "DEPTH", "drift_macroDepth", 0.0, 1.0),
    make_qlink(3, "WAVE",  "drift_macroWave",  0.0, 1.0),
    make_qlink(4, "ECHO",  "drift_macroEcho",  0.0, 1.0),
]

TIDE_LEAD_DNA = {
    "brightness": 0.65,
    "warmth":     0.50,
    "movement":   0.55,
    "density":    0.40,
    "space":      0.55,
    "aggression": 0.35,
}

TIDE_LEAD_VARIATIONS = [
    make_variation("Biolume Lead",      ["Atmosphere"],               {"brightness": 0.80}),
    make_variation("Salt Flat Pulse",   ["Prism"],                    {"movement": 0.75}),
    make_variation("Kelp Forest Pad",   ["Atmosphere", "Aether"],     {"space": 0.75, "density": 0.55}),
    make_variation("Surge Bass",        ["Foundation"],               {"warmth": 0.70, "aggression": 0.55}),
    make_variation("Coral Arp",         ["Flux"],                     {"movement": 0.85}),
    make_variation("Drift Tone",        ["Aether"],                   {"brightness": 0.50, "space": 0.70}),
    make_variation("Undertow Dark",     ["Entangled"],                {"warmth": 0.30, "aggression": 0.60}),
    make_variation("Open Horizon",      ["Foundation", "Prism"],      {"brightness": 0.70, "space": 0.60}),
]

TIDE_LEAD = {
    "program_name":  "Tide Lead",
    "engine":        "ODYSSEY",
    "engine_prefix": "drift_",
    "description":   "ODYSSEY's wavetable-subtractive engine laid chromatically across 16 pads. Four tonal characters: Lead, Pad, Bass, Arp. Play melodies straight off the pads.",
    "pads":          TIDE_LEAD_PADS,
    "keygroup":      TIDE_LEAD_KEYGROUP,
    "qlinks":        TIDE_LEAD_QLINKS,
    "dna_vector":    TIDE_LEAD_DNA,
    "variations":    TIDE_LEAD_VARIATIONS,
}


# ---------------------------------------------------------------------------
# Program 3 — Shore Texture (OPAL)
# ---------------------------------------------------------------------------

SHORE_TEXTURE_PAD_NAMES = [
    "Cloud Bright", "Cloud Dark",  "Scatter Fast",  "Scatter Slow",
    "Freeze High",  "Freeze Low",  "Pitched Rise",  "Pitched Fall",
    "Motion Fast",  "Motion Slow", "Stutter Hard",  "Stutter Soft",
    "Foam",         "Surge",       "Undertow",      "Spray",
]

SHORE_TEXTURE_PRESETS = [
    "opal_cloud_bright",   "opal_cloud_dark",    "opal_scatter_fast",  "opal_scatter_slow",
    "opal_freeze_high",    "opal_freeze_low",    "opal_pitched_rise",  "opal_pitched_fall",
    "opal_motion_fast",    "opal_motion_slow",   "opal_stutter_hard",  "opal_stutter_soft",
    "opal_texture_foam",   "opal_texture_surge", "opal_texture_under", "opal_texture_spray",
]

SHORE_TEXTURE_PADS = [
    make_pad(i + 1, SHORE_TEXTURE_PAD_NAMES[i], SHORE_TEXTURE_PRESETS[i], 36 + i)
    for i in range(16)
]

SHORE_TEXTURE_KEYGROUP = make_keygroup(
    root_note=60,
    vel_layers=[
        {"low": 0,   "high": 70,  "label": "breath"},
        {"low": 71,  "high": 127, "label": "full"},
    ],
    envelope={"attack": 0.120, "decay": 0.80, "sustain": 0.60, "release": 1.20},
)

SHORE_TEXTURE_QLINKS = [
    make_qlink(1, "DENSITY",  "opal_macroDensity",  0.0, 1.0),
    make_qlink(2, "SIZE",     "opal_macroSize",     0.0, 1.0),
    make_qlink(3, "POSITION", "opal_macroPosition", 0.0, 1.0),
    make_qlink(4, "SCATTER",  "opal_macroScatter",  0.0, 1.0),
]

SHORE_TEXTURE_DNA = {
    "brightness": 0.50,
    "warmth":     0.55,
    "movement":   0.60,
    "density":    0.70,
    "space":      0.75,
    "aggression": 0.20,
}

SHORE_TEXTURE_VARIATIONS = [
    make_variation("Sea Glass Cloud",    ["Atmosphere", "Aether"],  {"brightness": 0.65, "density": 0.55}),
    make_variation("Tidal Scatter",      ["Flux"],                  {"movement": 0.80}),
    make_variation("Ice Freeze",         ["Atmosphere"],            {"brightness": 0.75, "warmth": 0.30}),
    make_variation("Warm Grain Pitch",   ["Foundation"],            {"warmth": 0.70}),
    make_variation("Storm Motion",       ["Entangled", "Flux"],     {"movement": 0.90, "aggression": 0.50}),
    make_variation("Drift Stutter",      ["Prism"],                 {"brightness": 0.60, "movement": 0.70}),
    make_variation("Shore Mist",         ["Aether"],                {"density": 0.45, "space": 0.85}),
    make_variation("Abyssal Freeze",     ["Entangled"],             {"warmth": 0.25, "space": 0.90}),
]

SHORE_TEXTURE = {
    "program_name":  "Shore Texture",
    "engine":        "OPAL",
    "engine_prefix": "opal_",
    "description":   "OPAL's granular engine across six texture categories — Cloud, Scatter, Freeze, Pitched, Motion, Stutter. Each in two contrasting flavors. Atmosphere in 16 pads.",
    "pads":          SHORE_TEXTURE_PADS,
    "keygroup":      SHORE_TEXTURE_KEYGROUP,
    "qlinks":        SHORE_TEXTURE_QLINKS,
    "dna_vector":    SHORE_TEXTURE_DNA,
    "variations":    SHORE_TEXTURE_VARIATIONS,
}


# ---------------------------------------------------------------------------
# Program 4 — Deep Water (ONSET + ODYSSEY + OPAL, Entangled)
# ---------------------------------------------------------------------------

DEEP_WATER_PADS = [
    # Pads 1–4: ONSET rhythm anchors
    make_pad(1,  "DW Kick",       "perc_kick_deep",       36),
    make_pad(2,  "DW Snare",      "perc_snare_crack",     37),
    make_pad(3,  "DW Hat",        "perc_chat_tight",      38),
    make_pad(4,  "DW Clap",       "perc_clap_classic",    39),
    # Pads 5–8: ODYSSEY harmonic tones
    make_pad(5,  "DW Lead C",     "drift_lead_warm",      48),
    make_pad(6,  "DW Lead F",     "drift_lead_hollow",    53),
    make_pad(7,  "DW Pad C",      "drift_pad_wide",       60),
    make_pad(8,  "DW Bass C",     "drift_bass_deep",      36),
    # Pads 9–12: OPAL texture layers
    make_pad(9,  "DW Cloud",      "opal_cloud_dark",      60),
    make_pad(10, "DW Freeze",     "opal_freeze_low",      60),
    make_pad(11, "DW Motion",     "opal_motion_slow",     60),
    make_pad(12, "DW Scatter",    "opal_scatter_slow",    60),
    # Pads 13–16: coupled performance pads
    make_pad(13, "DW Surge",      "entangled_surge",      60, notes="ONSET→OPAL coupling; drum hits scatter grains"),
    make_pad(14, "DW Entangle",   "entangled_mesh",       62, notes="ODYSSEY→ONSET coupling; pitch gates rhythm"),
    make_pad(15, "DW Tide",       "entangled_tide",       64, notes="Tempo-synced LFO sweeps all three engines"),
    make_pad(16, "DW Release",    "entangled_release",    65, notes="Full open — all engines breathe together"),
]

DEEP_WATER_KEYGROUP = make_keygroup(
    root_note=60,
    vel_layers=[
        {"low": 0,   "high": 80,  "label": "submerged"},
        {"low": 81,  "high": 127, "label": "surfacing"},
    ],
    envelope={"attack": 0.050, "decay": 0.50, "sustain": 0.70, "release": 0.80},
)

DEEP_WATER_QLINKS = [
    make_qlink(1, "DRIVE",    "global_macroDrive",    0.0, 1.0, curve="exponential"),
    make_qlink(2, "SPACE",    "global_macroSpace",    0.0, 1.0),
    make_qlink(3, "ENTANGLE", "global_macroEntangle", 0.0, 1.0),
    make_qlink(4, "TIDE",     "global_macroTide",     0.0, 1.0),
]

DEEP_WATER_DNA = {
    "brightness": 0.45,
    "warmth":     0.50,
    "movement":   0.65,
    "density":    0.75,
    "space":      0.80,
    "aggression": 0.40,
}

DEEP_WATER_VARIATIONS = [
    make_variation("Midnight Trench",   ["Entangled", "Aether"],   {"brightness": 0.20, "space": 0.95}),
    make_variation("Shallow Reef",      ["Foundation"],            {"brightness": 0.65, "density": 0.55}),
    make_variation("Current Break",     ["Flux"],                  {"movement": 0.85, "aggression": 0.60}),
    make_variation("Still Water",       ["Atmosphere", "Aether"],  {"movement": 0.30, "space": 0.85}),
    make_variation("Thermal Rise",      ["Prism"],                 {"warmth": 0.75, "brightness": 0.60}),
    make_variation("Storm Front",       ["Flux", "Entangled"],     {"aggression": 0.80, "movement": 0.90}),
    make_variation("Bioluminescent",    ["Prism", "Atmosphere"],   {"brightness": 0.80, "density": 0.60}),
    make_variation("The Deep",          ["Family"],                {"warmth": 0.45, "space": 1.0, "aggression": 0.15}),
]

DEEP_WATER = {
    "program_name":  "Deep Water",
    "engine":        "ONSET + ODYSSEY + OPAL",
    "engine_prefix": "multi",
    "mood":          "Entangled",
    "description":   "All three engines in one program. Pads 1–4 are ONSET rhythm. Pads 5–8 are ODYSSEY harmony. Pads 9–12 are OPAL texture. Pads 13–16 are coupled performance pads where engines cross-modulate. No engine operates alone.",
    "pads":          DEEP_WATER_PADS,
    "keygroup":      DEEP_WATER_KEYGROUP,
    "qlinks":        DEEP_WATER_QLINKS,
    "dna_vector":    DEEP_WATER_DNA,
    "variations":    DEEP_WATER_VARIATIONS,
}


# ---------------------------------------------------------------------------
# Full pack spec
# ---------------------------------------------------------------------------

PACK_SPEC = {
    "pack_name":    "TIDE TABLES",
    "subtitle":     "Three Engines. One Free Pack.",
    "version":      "1.0.0",
    "release_date": TODAY,
    "format":       "Akai MPC Expansion (.xpn)",
    "license":      "Free for personal and commercial use",
    "engines":      ["ONSET", "ODYSSEY", "OPAL"],
    "engine_colors": {
        "ONSET":   "#0066FF",
        "ODYSSEY": "#7B2D8B",
        "OPAL":    "#A78BFA",
    },
    "programs": [
        COASTAL_DRUMS,
        TIDE_LEAD,
        SHORE_TEXTURE,
        DEEP_WATER,
    ],
    "metadata": {
        "description": (
            "TIDE TABLES is a free gateway pack for the XO_OX synthesizer library. "
            "Three flagship engines — ONSET (drums), ODYSSEY (leads/pads), OPAL (textures/granular) "
            "— assembled into four programs that work together or alone. "
            "No samples. Every sound is synthesized in real time."
        ),
        "tags": ["free", "gateway", "drums", "leads", "pads", "granular", "synthesis", "XO_OX"],
        "url":  "https://xo-ox.org",
    },
}


# ---------------------------------------------------------------------------
# README content
# ---------------------------------------------------------------------------

README_MD = """\
# TIDE TABLES
### A free MPC expansion from XO_OX
**"Three Engines. One Free Pack."**

---

## What Is This?

TIDE TABLES is the gateway pack for the XO_OX synthesizer library.
It brings three of XOlokun's flagship engines — **ONSET**, **ODYSSEY**, and **OPAL** —
into a single, free MPC expansion you can load today.

No samples. No loops. Every sound is synthesized in real time by the engines themselves.

---

## Engines Showcased

| Engine  | Character | Color |
|---------|-----------|-------|
| **ONSET**   | Percussive synthesis — 8 voices across kick, snare, hats, clap, toms, perc, FX | Electric Blue `#0066FF` |
| **ODYSSEY** | Wavetable-subtractive leads, pads, bass, and arpeggios | Violet `#7B2D8B` |
| **OPAL**    | Granular textures — cloud, scatter, freeze, pitched, motion, stutter | Lavender `#A78BFA` |

---

## Four Programs

### 1. Coastal Drums — *ONSET*
Pure percussion. ONSET's eight synthesis voices cover the full kit.
Three velocity layers per pad. Macro control over character, transient, space, and mutation.

| Q-Link | Function |
|--------|----------|
| Q1 MACHINE | Acoustic ↔ machine character |
| Q2 PUNCH   | Transient attack across all voices |
| Q3 SPACE   | Global reverb field depth |
| Q4 MUTATE  | Timbral randomness |

---

### 2. Tide Lead — *ODYSSEY*
ODYSSEY's engine laid chromatically across 16 pads.
Four tonal characters — Lead, Pad, Bass, Arp — organized so you can play melodies and harmonics
straight off the pads without ever touching keys.

| Q-Link | Function |
|--------|----------|
| Q1 DRIFT | Pitch instability / detuning width |
| Q2 DEPTH | Filter sweep depth |
| Q3 WAVE  | Wavetable morph position |
| Q4 ECHO  | Tape delay send level |

---

### 3. Shore Texture — *OPAL*
OPAL's granular engine across six texture categories, each in two contrasting flavors
(bright/dark, fast/slow, hard/soft). Sixteen pads of atmosphere you can trigger,
layer, and sweep in real time.

| Q-Link | Function |
|--------|----------|
| Q1 DENSITY  | Grain population (sparse to cloud) |
| Q2 SIZE     | Grain duration (micro to long) |
| Q3 POSITION | Source scan position |
| Q4 SCATTER  | Spatial randomization |

---

### 4. Deep Water — *ONSET + ODYSSEY + OPAL* (Entangled)
All three engines in one program.

- Pads 1–4 → ONSET rhythm anchors
- Pads 5–8 → ODYSSEY harmonic tones
- Pads 9–12 → OPAL texture layers
- Pads 13–16 → Coupled performance pads: engines cross-modulate each other

This is the **Entangled mood**. No engine operates alone.

| Q-Link   | Function |
|----------|----------|
| Q1 DRIVE    | Saturation across all three engines |
| Q2 SPACE    | Unified reverb field |
| Q3 ENTANGLE | Cross-modulation depth |
| Q4 TIDE     | Tempo-synced global LFO |

---

## How to Load

1. Open MPC Software or boot your MPC hardware.
2. Go to **Menu → Browse → Expansions**.
3. Navigate to the **TIDE TABLES .xpn** file and double-click to install.
4. Find "TIDE TABLES" in your expansion library.
5. Load any of the four programs and start playing.

---

## What Makes It Special

TIDE TABLES is fully synthesized — no sample content, no licensing restrictions.
Every pad responds to velocity, mod wheel, and aftertouch exactly as it would
inside XOlokun on a desktop. The engines are the same code running on the same DSP.

The pack is also a map. Each program is a door into a different engine.
Play Coastal Drums, then open Deep Water and hear what happens when all three
engines lock together. That's what XOlokun is built for.

---

## Want More?

TIDE TABLES is a taste. The full XOlokun library contains **34+ engines**,
2,550+ factory presets, and the complete XO_OX water column mythology.

Visit **[xo-ox.org](https://xo-ox.org)** to learn more, or join Patreon at the Signal tier
to receive new packs as they're released.

---

## Credits

All sounds: XO_OX
Engines: ONSET, ODYSSEY, OPAL (XOlokun)
Format: Akai MPC Expansion (.xpn)
Released: {TODAY}
License: Free for personal and commercial use

---

*"Every tide starts somewhere."*
""".format(TODAY=TODAY)


# ---------------------------------------------------------------------------
# Marketing blurb content
# ---------------------------------------------------------------------------

MARKETING_MD = """\
# TIDE TABLES — Marketing Copy

## Pack: TIDE TABLES
**Tagline:** Three Engines. One Free Pack.

---

### Hook

You hear the ocean before you see it.
That's how synthesis works when it's done right — the sound arrives first,
and the explanation follows later, if at all.
TIDE TABLES is a free pack built for that moment:
the moment you load something new, hit the first pad, and already know
you're going to use this.

Three of XOlokun's most distinctive engines — ONSET, ODYSSEY, OPAL —
assembled into four programs. No demos. No watermarks. No strings.
Just load it and play.

---

### The Sounds

**Coastal Drums** is ONSET: percussive synthesis across eight dedicated voices.
Kick, snare, hats, clap, toms, perc, and two FX slots — all synthesized,
all velocity-sensitive, all controllable from four macros that shape character,
punch, space, and mutation simultaneously.

**Tide Lead** is ODYSSEY: wavetable-subtractive synthesis arranged chromatically
across 16 pads so you can run leads, pads, bass, and arps without ever switching programs.
DRIFT detunes. DEPTH sweeps the filter. WAVE morphs the wavetable.
ECHO sends you somewhere else entirely.

**Shore Texture** is OPAL: granular. Six texture categories — Cloud, Scatter, Freeze,
Pitched, Motion, Stutter — each in two contrasting flavors. Sixteen pads of atmosphere
you can trigger in a beat, layer across a phrase, or hold open until the room fills up.

**Deep Water** is what happens when all three engines run together.
Pads 1–4 are rhythm. Pads 5–8 are harmony. Pads 9–12 are texture.
Pads 13–16 are where the engines start talking to each other —
drum hits that scatter grains, pitch that gates rhythm, a tempo-locked LFO
that moves everything at once. This is the Entangled mood.
No engine operates alone.

---

### Call to Action

TIDE TABLES is free because everyone deserves to hear what XOlokun can do
before they decide whether to go deeper.

If these four programs feel like enough, keep them. Play them. Build with them.
If you want 30 more engines, 2,550 factory presets, and a complete sonic mythology
mapped across the water column from surface to trench —
visit **xo-ox.org** and see what's waiting.

The tide comes in either way.
The question is how far in you want to go.
"""


# ---------------------------------------------------------------------------
# Write outputs
# ---------------------------------------------------------------------------

def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    # 1. Pack spec JSON
    spec_path = OUT_DIR / "tide_tables_pack_spec.json"
    with open(spec_path, "w", encoding="utf-8") as f:
        json.dump(PACK_SPEC, f, indent=2, ensure_ascii=False)
    print(f"  [OK] {spec_path}")

    # 2. Producer README
    readme_path = OUT_DIR / "TIDE_TABLES_README.md"
    with open(readme_path, "w", encoding="utf-8") as f:
        f.write(README_MD)
    print(f"  [OK] {readme_path}")

    # 3. Marketing blurb
    marketing_path = OUT_DIR / "tide_tables_marketing.md"
    with open(marketing_path, "w", encoding="utf-8") as f:
        f.write(MARKETING_MD)
    print(f"  [OK] {marketing_path}")

    print()
    print("TIDE TABLES pack spec complete.")
    print(f"  Programs   : {len(PACK_SPEC['programs'])}")
    print(f"  Engines    : {', '.join(PACK_SPEC['engines'])}")
    total_pads = sum(len(p['pads']) for p in PACK_SPEC['programs'])
    total_variations = sum(len(p['variations']) for p in PACK_SPEC['programs'])
    print(f"  Total pads : {total_pads}")
    print(f"  Variations : {total_variations}")
    print(f"  Output dir : {OUT_DIR}")


if __name__ == "__main__":
    main()
