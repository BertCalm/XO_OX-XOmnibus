#!/usr/bin/env python3
"""
XPN Complement Chain Renderer — XO_OX Designs
Generates primary + complement variant XPM program pairs for Artwork/Color
collection engines.

Every Artwork engine has a primary color and a complementary color (opposite
on the wheel).  The Wildcard axis sweeps through 4 shades of the complement:

    Primary   — engine at full primary color character
    Tint      — 25% complement shift
    Tone      — 50/50 blend
    Shade     — 75% complement dominant
    Pure      — 100% complement (the inverted engine)

For each input preset this tool writes 5 paired XPM programs and 5 cover-art
images that visualise the primary→complement gradient at each blend position.

Usage:
    # Single engine, discover presets in a directory
    python3 xpn_complement_renderer.py \\
        --engine XOxblood --presets ./presets/ --output ./complement_packs/

    # Full Artwork collection — all 24 engines
    python3 xpn_complement_renderer.py \\
        --collection artwork --all-engines --output ./complement_packs/

    # Dry run (no files written)
    python3 xpn_complement_renderer.py \\
        --engine XOxblood --presets ./presets/ --output ./out/ --dry-run

    # Skip cover art (faster)
    python3 xpn_complement_renderer.py \\
        --engine XOxblood --presets ./presets/ --output ./out/ --skip-cover-art

Dependencies: Pillow, numpy (only required for cover-art generation)
    pip install Pillow numpy
"""

import argparse
import json
import math
import os
import random
import sys
import time
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Resolve Tools/ directory so sibling imports work from any cwd
# ---------------------------------------------------------------------------
TOOLS_DIR = Path(__file__).parent.resolve()
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

# ---------------------------------------------------------------------------
# Banner
# ---------------------------------------------------------------------------
BANNER = r"""
   _____                    _                            _
  / ____|                  | |                          | |
 | |     ___  _ __ ___  ___| |_ ___ _ __ _ __ ___  ___ | |_
 | |    / _ \| '_ ` _ \|  __| __/ _ \ '__| '_ ` _ \/ _ \| __|
 | |___| (_) | | | | | | |__| ||  __/ |  | | | | | (_) | |_
  \_____\___/|_| |_| |_|\___|\__\___|_|  |_| |_| |_\___/ \__|
   ___  _           _
  / __|| |__   __ _(_)_ __
 | |   | '_ \ / _` | | '_ \
 | |___| | | | (_| | | | | |
  \___ |_| |_|\__,_|_|_| |_|
   ____                _
  |  _ \ ___ _ __   __| | ___ _ __ ___ _ __
  | |_) / _ \ '_ \ / _` |/ _ \ '__/ _ \ '__|
  |  _ <  __/ | | | (_| |  __/ | |  __/ |
  |_| \_\___|_| |_|\__,_|\___|_|  \___|_|

  XO_OX — Artwork/Color Collection  ·  Complement Chain Renderer
"""

# ---------------------------------------------------------------------------
# Shade definitions
# ---------------------------------------------------------------------------
# Each preset generates these 5 variants.  Blend value = fraction of complement
# mixed into the primary.  0.0 = pure primary, 1.0 = pure complement.
SHADE_DEFS = [
    {"name": "Primary", "blend": 0.00, "suffix": "Primary"},
    {"name": "Tint",    "blend": 0.25, "suffix": "Tint"},
    {"name": "Tone",    "blend": 0.50, "suffix": "Tone"},
    {"name": "Shade",   "blend": 0.75, "suffix": "Shade"},
    {"name": "Pure",    "blend": 1.00, "suffix": "Pure"},
]

# ---------------------------------------------------------------------------
# Complement colour database
# ---------------------------------------------------------------------------
# Sourced directly from Docs/concepts/artwork_collection_overview.md.
# Each entry carries the 4 complement shade variants mirroring the doc tables.
#
# Keys are the XO + O-word engine names exactly as used in the CLI.
# Each shade entry: (hex, display_name, sonic_shift)
# ---------------------------------------------------------------------------

COMPLEMENT_PAIRS = {
    # ── QUAD 1: COLOR QUAD A ─────────────────────────────────────────────────
    "XOxblood": {
        "primary":          "#6A0D0D",
        "primary_name":     "Oxblood",
        "complement":       "#0D6A6A",
        "complement_name":  "Teal",
        "instrument":       "Erhu",
        "prefix":           "oxblood_",
        "shades": [
            ("#7FDBCA", "Seafoam",     "Light aquatic wash — the erhu heard through shallow water"),
            ("#008080", "True Teal",   "Equal tension — warm blood meeting cold ocean"),
            ("#005F5F", "Deep Teal",   "Underwater — the erhu submerged, frequencies absorbed by depth"),
            ("#003D3D", "Abyssal Teal","Full inversion — the erhu has become a submarine instrument"),
        ],
    },
    "XOnyx": {
        "primary":          "#0A0A0A",
        "primary_name":     "Onyx Black",
        "complement":       "#F5F5F5",
        "complement_name":  "Shock White",
        "instrument":       "Didgeridoo",
        "prefix":           "onyx_",
        "shades": [
            ("#FFFFF0", "Ivory",      "Warmth entering the black — overtone brightness emerging"),
            ("#FFFFFF", "True White", "Maximum contrast — drone and silence in equal measure"),
            ("#F8F8FF", "Glare",      "Overexposed — pushed into blinding clarity"),
            ("#FDFDFD", "Bleach",     "Full inversion — pure overtone, no fundamental"),
        ],
    },
    "XOchre": {
        "primary":          "#CC7722",
        "primary_name":     "Earthy Yellow",
        "complement":       "#6A7B8B",
        "complement_name":  "Slate Blue",
        "instrument":       "Oud",
        "prefix":           "ochre_",
        "shades": [
            ("#B0C4DE", "Powder Blue", "Cool mist over warm earth — the oud heard at dawn"),
            ("#6A7B8B", "True Slate",  "Sky meeting desert — equal temperature"),
            ("#4A5D6E", "Storm Blue",  "Overcast — warmth suppressed by cold light"),
            ("#2C3E50", "Gunmetal",    "Full inversion — the oud has become cold steel"),
        ],
    },
    "XOrchid": {
        "primary":          "#DA70D6",
        "primary_name":     "Purple-Pink",
        "complement":       "#7FFF00",
        "complement_name":  "Chartreuse",
        "instrument":       "Guzheng",
        "prefix":           "orchid_",
        "shades": [
            ("#BFFF00", "Lime Mist",    "Electric brightness entering purple luxury"),
            ("#7FFF00", "True Chartreuse","Silk and poison in equal measure"),
            ("#40BF00", "Toxic Green",  "Delicacy attacked by neon"),
            ("#00FF00", "Radioactive",  "Full inversion — the guzheng has become a digital weapon"),
        ],
    },

    # ── QUAD 2: COLOR QUAD B ─────────────────────────────────────────────────
    "XOttanio": {
        "primary":          "#005F5F",
        "primary_name":     "Petrol Teal",
        "complement":       "#CC5500",
        "complement_name":  "Arancione Bruciato",
        "instrument":       "Handpan",
        "prefix":           "ottanio_",
        "shades": [
            ("#E07A3A", "Terracotta Wash", "Warm glow — copper warming at sunset"),
            ("#CC5500", "True Bruciato",   "Cool metal and hot clay in balance"),
            ("#8B4513", "Deep Sienna",     "Earth dominant — muffled, resonating through clay"),
            ("#B87333", "Molten Copper",   "Oxidation reversed — raw copper before time touched it"),
        ],
    },
    "XOmaomao": {
        "primary":          "#228B22",
        "primary_name":     "Verdant Green",
        "complement":       "#8B2222",
        "complement_name":  "Poni-ʻulaʻula",
        "instrument":       "Angklung",
        "prefix":           "omaomao_",
        "shades": [
            ("#CD5C5C", "Hibiscus Blush", "Flower-red touching green — red bloom against the leaf"),
            ("#8B2222", "True Poni",      "Iron soil meeting bamboo — earth that feeds the plant"),
            ("#6B1010", "Volcanic Red",   "Lava that enriches the soil that grows the bamboo"),
            ("#4A0000", "Pele's Fire",    "Full inversion — bamboo consumed by fire. Ash becoming soil again."),
        ],
    },
    "XOstrum": {
        "primary":          "#66023C",
        "primary_name":     "Tyrian Purple",
        "complement":       "#9ACD32",
        "complement_name":  "Flavovirens",
        "instrument":       "Sitar",
        "prefix":           "ostrum_",
        "shades": [
            ("#BFFF00", "Spring Chartreuse", "Acid brightness entering imperial richness"),
            ("#9ACD32", "True Flavovirens",  "Court meets field — emperor through crop rows"),
            ("#6B8E23", "Olive Virens",      "Military green — the army protecting the empire"),
            ("#556B2F", "Peasant Green",     "Full inversion — stripped of sympathetic strings. One voice, working the land."),
        ],
    },
    "XOni": {
        "primary":          "#E34234",
        "primary_name":     "Demon Vermillion",
        "complement":       "#48929B",
        "complement_name":  "Asagi-iro",
        "instrument":       "Tsugaru Shamisen",
        "prefix":           "oni_",
        "shades": [
            ("#7BC8D0", "Morning Asagi", "Cool mist entering fire — heard across a lake at dawn"),
            ("#48929B", "True Asagi",    "Demon and water in balance — oni reflected in the still pond"),
            ("#2F6B72", "Deep Asagi",    "Water winning — attack softened by depth"),
            ("#1A4A50", "Abyssal Asagi", "Full inversion — the demon drowned. The shamisen as lullaby."),
        ],
    },

    # ── QUAD 3: SHOWMEN ───────────────────────────────────────────────────────
    # Showmen engines use "Their Rival" wildcard rather than a colour complement.
    # We map the Primary → complement as before but shades follow thematic
    # descriptions appropriate to each visionary.
    "XOpulent": {
        "primary":          "#CC4400",  # Mothership Orange (psychedelic max)
        "primary_name":     "Mothership Orange",
        "complement":       "#0044CC",
        "complement_name":  "Cosmic Blue",
        "instrument":       "Parliament/Funkadelic",
        "prefix":           "opulent_",
        "shades": [
            ("#4488FF", "Sky Flash",    "One colour of the spectrum — the funk before the stack"),
            ("#0044CC", "True Cosmic",  "Orange and blue in maximum psychedelic tension"),
            ("#002288", "Deep Space",   "The Mothership ascending — stage lights dimming"),
            ("#000033", "Void",         "Full inversion — the silhouette after the lights go out"),
        ],
    },
    "XOccult": {
        "primary":          "#FF6EC7",  # Henning tie-dye pink
        "primary_name":     "Henning Pink",
        "complement":       "#11912F",
        "complement_name":  "Illusion Green",
        "instrument":       "Stage Magic",
        "prefix":           "occult_",
        "shades": [
            ("#66CC88", "Reveal Mist",    "The cloth lifting — first glimpse of green"),
            ("#11912F", "True Illusion",  "Equal wonder — the trick balanced on both sides"),
            ("#0A5C1E", "Vanishing Green","The assistant gone — only forest remaining"),
            ("#042E0F", "The Dark Stage", "Full inversion — lights out. The magic complete."),
        ],
    },
    "XOvation": {
        "primary":          "#1A1A1A",  # Fosse black
        "primary_name":     "Fosse Black",
        "complement":       "#E5E5E5",
        "complement_name":  "Spotlight White",
        "instrument":       "Broadway Jazz Dance",
        "prefix":           "ovation_",
        "shades": [
            ("#CCCCCC", "Footlight",     "First brightness — the follow-spot finding the dancer"),
            ("#E5E5E5", "True Spotlight","Maximum contrast — black and white, a single frame of Cabaret"),
            ("#F0F0F0", "Bleached Stage","Overexposed — the costume designer's nightmare"),
            ("#FDFDFD", "Pure Light",    "Full inversion — the theatre flooded. No darkness left for drama."),
        ],
    },
    "XOverdrive": {
        "primary":          "#CC1100",  # Frankenstrat red
        "primary_name":     "Frankenstrat Red",
        "complement":       "#00CCAA",
        "complement_name":  "Eruption Cyan",
        "instrument":       "Electric Guitar",
        "prefix":           "odrive_",
        "shades": [
            ("#55EEDD", "Pick Scrape",   "Harmonic wash entering the overdrive"),
            ("#00CCAA", "True Eruption", "Red and cyan — the stripe and the sky"),
            ("#008870", "Brown Sound",   "The Variac cranked — power tubes sagging into warmth"),
            ("#004438", "Buried Low",    "Full inversion — the riff compressed into a drone"),
        ],
    },

    # ── QUAD 4: AESTHETES ────────────────────────────────────────────────────
    "XOrnament": {
        "primary":          "#4CAF50",  # Wiley botanical green
        "primary_name":     "Botanical Green",
        "complement":       "#F44336",
        "complement_name":  "Portrait Red",
        "instrument":       "Visual Art / Portraiture",
        "prefix":           "ornament_",
        "shades": [
            ("#FF7777", "Blossom Red",   "Warmth entering botanical luxury"),
            ("#F44336", "True Portrait", "Subject and background in equal command"),
            ("#B71C1C", "Heritage Red",  "Old Master palette — the figure emerging from history"),
            ("#7F0000", "Crown Red",     "Full inversion — the background has taken the foreground"),
        ],
    },
    "XOblation": {
        "primary":          "#FF69B4",  # Barragán hot pink wall
        "primary_name":     "Barragán Pink",
        "complement":       "#69FFB4",
        "complement_name":  "Pool Celadon",
        "instrument":       "Architecture",
        "prefix":           "oblation_",
        "shades": [
            ("#AAFFD4", "Light Reflection", "Cool water entering warm masonry"),
            ("#69FFB4", "True Celadon",     "Wall and pool — hot pink and blue-green in balance"),
            ("#33CC7A", "Volcanic Stone",   "The pool dominant — pink receding into the garden"),
            ("#008844", "Deep Courtyard",   "Full inversion — the pink wall gone. Only the water remains."),
        ],
    },
    "XObsession": {
        "primary":          "#CC1100",  # Almodóvar red
        "primary_name":     "Almodóvar Red",
        "complement":       "#00CCCC",
        "complement_name":  "Melodrama Cyan",
        "instrument":       "Film",
        "prefix":           "obsess_",
        "shades": [
            ("#55EEEE", "Cool Frame",     "Cyan entering Almodóvar's fire — the cold scene"),
            ("#00CCCC", "True Melodrama", "Desire and distance in equal measure"),
            ("#008888", "Distant Frame",  "The camera pulling back — red receding"),
            ("#004444", "Cold Ending",    "Full inversion — the credits roll in silence"),
        ],
    },
    "XOther": {
        "primary":          "#D4A017",  # Vinyl amber
        "primary_name":     "Vinyl Amber",
        "complement":       "#1749D4",
        "complement_name":  "Sample Blue",
        "instrument":       "Production / Collage",
        "prefix":           "other_",
        "shades": [
            ("#5577EE", "Crate Dust",    "Cool light entering warm vinyl — the dig beginning"),
            ("#1749D4", "True Sample",   "Amber and blue — old source material and new context"),
            ("#0D2E8A", "Deepened Crate","The sample dominant — the original barely audible"),
            ("#060F33", "Vault",         "Full inversion — pure sample. No source audible. Only the idea."),
        ],
    },

    # ── QUAD 5: COLOR / MAGIC / WATER ────────────────────────────────────────
    "XOrdeal": {
        "primary":          "#7FFFD4",
        "primary_name":     "Stage Aquamarine",
        "complement":       "#FF00FF",
        "complement_name":  "Theatrical Magenta",
        "instrument":       "Waterphone",
        "prefix":           "ordeal_",
        "shades": [
            ("#FF77FF", "Blush Curtain", "Light showmanship — theatrical warmth"),
            ("#FF00FF", "True Magenta",  "Water and curtain, reveal and conceal"),
            ("#8B008B", "Deep Curtain",  "The curtain closing — muffled, the trick ending"),
            ("#4B004B", "Blackout",      "Stage goes dark. Only the memory of drowning."),
        ],
    },
    "XOutpour": {
        "primary":          "#7F1734",
        "primary_name":     "Claret",
        "complement":       "#ACE1AF",
        "complement_name":  "Celadon",
        "instrument":       "Theremin",
        "prefix":           "outpour_",
        "shades": [
            ("#D0F0C0", "Jade Mist",   "Coolness entering wine — green transparency"),
            ("#ACE1AF", "True Celadon","Wine and jade in balance — pour and vessel"),
            ("#5F8A6E", "Deep Jade",   "Vessel dominant — warmth cooled, wine becoming water"),
            ("#3D6B4F", "Stone Celadon","Full inversion — empty cup. The trick un-performed."),
        ],
    },
    "XOctavo": {
        "primary":          "#8E8E8E",
        "primary_name":     "Newsprint Grey",
        "complement":       "#FDEE00",
        "complement_name":  "Aureolin",
        "instrument":       "Harmonium / Shruti Box",
        "prefix":           "octavo_",
        "shades": [
            ("#FFF44F", "Lemon Wash",   "Brightness entering grey — suddenly present"),
            ("#FDEE00", "True Aureolin","Grey and yellow — newspaper and headline"),
            ("#DAA520", "Goldenrod",    "Yellow aging — old newsprint turning golden"),
            ("#FFBF00", "Raw Amber",    "Full inversion — no medium, no hiding. The water poured out."),
        ],
    },
    "XObjet": {
        "primary":          "#FFD700",
        "primary_name":     "Goldfish Gold",
        "complement":       "#120A8F",
        "complement_name":  "Ultramarine",
        "instrument":       "Bullroarer",
        "prefix":           "objet_",
        "shades": [
            ("#6495ED", "Cornflower",    "Light water entering gold — aquatic shimmer"),
            ("#120A8F", "True Ultramarine","Gold and deep blue — fish in the bowl, visible through water"),
            ("#0D0D6B", "Midnight Blue", "The deep — roar submerged, fish descending"),
            ("#06063D", "Abyssal Blue",  "Full inversion — only water. The bowl without the fish."),
        ],
    },

    # ── THE ARCADE (hidden 5th slot) ─────────────────────────────────────────
    "XOkami": {
        "primary":          "#CC1155",  # Suda punk neon
        "primary_name":     "Punk Neon",
        "complement":       "#11CC88",
        "complement_name":  "Blood Lime",
        "instrument":       "Video Game (Suda 51)",
        "prefix":           "okami_",
        "shades": [
            ("#55FFCC", "Neon Splash",  "Style entering substance"),
            ("#11CC88", "True Blood",   "Punk and jungle in equal weight"),
            ("#0A8860", "Deep Jungle",  "The beat underlying the style"),
            ("#054430", "Grave",        "Full inversion — the punk retired. The music remains."),
        ],
    },
    "XOmni": {
        "primary":          "#0044FF",  # Rez / Mizuguchi synesthesia blue
        "primary_name":     "Synesthesia Blue",
        "complement":       "#FF4400",
        "complement_name":  "Area X Orange",
        "instrument":       "Video Game (Mizuguchi)",
        "prefix":           "omni_",
        "shades": [
            ("#FF8844", "Warm Pulse",   "Light entering synesthesia — heat on the retina"),
            ("#FF4400", "True Area X",  "Blue and orange — every colour at once"),
            ("#BB2200", "Deep Rez",     "Approaching Eden — the audio dominant"),
            ("#550F00", "Boss Room",    "Full inversion — all senses collapsed to one frequency"),
        ],
    },
    "XOdama": {
        "primary":          "#FFD700",  # Katamari gold/primary yellow
        "primary_name":     "Katamari Gold",
        "complement":       "#6B35C0",
        "complement_name":  "Prince Purple",
        "instrument":       "Video Game (Takahashi)",
        "prefix":           "odama_",
        "shades": [
            ("#AA88EE", "Playground Lavender","Delight entering gold — the cousin found"),
            ("#6B35C0", "True Prince",        "Gold and purple — the Katamari and its maker"),
            ("#4A2088", "Deep Royal",         "The King of All Cosmos looming"),
            ("#1E0A44", "Cosmos Night",       "Full inversion — the stars rolled up. Silence after collection."),
        ],
    },
    "XOffer": {
        "primary":          "#FFB7C5",  # Cult of the Lamb pastel
        "primary_name":     "Pastel Lamb",
        "complement":       "#4A182A",
        "complement_name":  "Cult Shadow",
        "instrument":       "Video Game (Massive Monster)",
        "prefix":           "offer_",
        "shades": [
            ("#884466", "Sermon Pink",    "Warmth entering the shadow — the sermon beginning"),
            ("#4A182A", "True Cult",      "Pastel and dark — the lamb on the altar"),
            ("#2A0B18", "Inner Sanctum",  "Ritual dominant — the cute surface eroded"),
            ("#100409", "The Void Below", "Full inversion — no lamb. Only the cult. Only the void."),
        ],
    },
}

# Mapping from engine key → XPM program description field prefix
ARTWORK_ENGINES = sorted(COMPLEMENT_PAIRS.keys())

# ---------------------------------------------------------------------------
# Helper: hex → RGB tuple
# ---------------------------------------------------------------------------

def _hex_to_rgb(hex_str: str) -> tuple[int, int, int]:
    h = hex_str.lstrip("#")
    return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16))


def _blend_hex(primary_hex: str, complement_hex: str, t: float) -> str:
    """Linear blend between primary (t=0) and complement (t=1). Returns hex."""
    pr, pg, pb = _hex_to_rgb(primary_hex)
    cr, cg, cb = _hex_to_rgb(complement_hex)
    r = int(pr + (cr - pr) * t)
    g = int(pg + (cg - pg) * t)
    b = int(pb + (cb - pb) * t)
    return f"#{r:02X}{g:02X}{b:02X}"


# ---------------------------------------------------------------------------
# XPM template
# ---------------------------------------------------------------------------
# We generate a minimal but spec-compliant keygroup XPM.
# The critical rules from CLAUDE.md / xpn_keygroup_export.py:
#   KeyTrack  = True
#   RootNote  = 0
#   VelStart  = 0 on empty layers
# ---------------------------------------------------------------------------

XPM_TEMPLATE = """\
<?xml version="1.0" ?>
<MPCVObject>
  <Version>2.6</Version>
  <Program type="Keygroup">
    <ProgramName>{program_name}</ProgramName>
    <ProgramDescription>{description}</ProgramDescription>
    <Instrument number="1">
      <InstrumentName>{program_name}</InstrumentName>
      <LowNote>0</LowNote>
      <HighNote>127</HighNote>
      <Octave>0</Octave>
      <Layer number="1">
        <SampleName>{sample_name}</SampleName>
        <SampleFile>{sample_file}</SampleFile>
        <RootNote>0</RootNote>
        <KeyTrack>True</KeyTrack>
        <VelStart>1</VelStart>
        <VelEnd>127</VelEnd>
        <Volume>1.0</Volume>
        <Pan>0.5</Pan>
        <Pitch>0.0</Pitch>
      </Layer>
      <Layer number="2">
        <SampleName></SampleName>
        <SampleFile></SampleFile>
        <RootNote>0</RootNote>
        <KeyTrack>True</KeyTrack>
        <VelStart>0</VelStart>
        <VelEnd>0</VelEnd>
        <Volume>1.0</Volume>
        <Pan>0.5</Pan>
        <Pitch>0.0</Pitch>
      </Layer>
      <Layer number="3">
        <SampleName></SampleName>
        <SampleFile></SampleFile>
        <RootNote>0</RootNote>
        <KeyTrack>True</KeyTrack>
        <VelStart>0</VelStart>
        <VelEnd>0</VelEnd>
        <Volume>1.0</Volume>
        <Pan>0.5</Pan>
        <Pitch>0.0</Pitch>
      </Layer>
      <Layer number="4">
        <SampleName></SampleName>
        <SampleFile></SampleFile>
        <RootNote>0</RootNote>
        <KeyTrack>True</KeyTrack>
        <VelStart>0</VelStart>
        <VelEnd>0</VelEnd>
        <Volume>1.0</Volume>
        <Pan>0.5</Pan>
        <Pitch>0.0</Pitch>
      </Layer>
    </Instrument>
  </Program>
</MPCVObject>
"""


def _make_xpm(program_name: str, description: str,
              sample_name: str = "", sample_file: str = "") -> str:
    """Return XPM XML string for a single-layer keygroup program."""
    return XPM_TEMPLATE.format(
        program_name=program_name,
        description=description,
        sample_name=sample_name or program_name,
        sample_file=sample_file or "",
    )


# ---------------------------------------------------------------------------
# Cover art — complement gradient
# ---------------------------------------------------------------------------

def _generate_complement_cover(
    engine_key: str,
    pack_name: str,
    shade_name: str,
    blend: float,
    output_dir: Path,
    primary_hex: str,
    complement_hex: str,
    preset_count: int = 1,
    version: str = "1.0",
    seed: int = 42,
    dry_run: bool = False,
) -> dict:
    """
    Generate complement cover art for one shade variant.

    The cover art blends between the engine's primary accent and its complement
    colour at the specified blend ratio, producing a gradient that visually
    communicates where on the primary→complement axis this preset sits.

    Falls back gracefully if Pillow/numpy are not installed.
    """
    if dry_run:
        return {"cover_1000": str(output_dir / "artwork.png"),
                "cover_2000": str(output_dir / "artwork_2000.png")}

    try:
        import numpy as np
        from PIL import Image, ImageDraw, ImageFilter, ImageFont
    except ImportError:
        print("    [SKIP] Cover art requires: pip install Pillow numpy")
        return {}

    SIZE = 2000
    rng_state = random.Random(seed + int(blend * 100))

    primary_rgb  = _hex_to_rgb(primary_hex)
    comp_rgb     = _hex_to_rgb(complement_hex)
    blend_hex    = _blend_hex(primary_hex, complement_hex, blend)
    blend_rgb    = _hex_to_rgb(blend_hex)

    # ── Background ──────────────────────────────────────────────────────────
    # Vertical gradient: primary at top, complement at bottom, blended by t.
    # At blend=0   → almost solid primary across both ends
    # At blend=1   → almost solid complement across both ends
    # At blend=0.5 → primary top, complement bottom
    arr = np.zeros((SIZE, SIZE, 3), dtype=np.float32)
    for row in range(SIZE):
        t_row = row / (SIZE - 1)   # 0 = top, 1 = bottom
        # top colour = primary weighted by (1-blend), complement weighted by blend
        # bottom colour = complement weighted by (1-blend), primary weighted by blend
        top_t    = blend
        bot_t    = 1.0 - blend
        row_t    = top_t + (bot_t - top_t) * t_row

        r = (primary_rgb[0] / 255.0) * (1.0 - row_t) + (comp_rgb[0] / 255.0) * row_t
        g = (primary_rgb[1] / 255.0) * (1.0 - row_t) + (comp_rgb[1] / 255.0) * row_t
        b = (primary_rgb[2] / 255.0) * (1.0 - row_t) + (comp_rgb[2] / 255.0) * row_t
        arr[row, :, 0] = r * 0.4   # Darkened for legibility
        arr[row, :, 1] = g * 0.4
        arr[row, :, 2] = b * 0.4

    # ── Centre circle: the blended colour at this shade position ────────────
    cx, cy = SIZE // 2, SIZE // 2
    Y, X = np.mgrid[0:SIZE, 0:SIZE]
    dist = np.sqrt((X - cx) ** 2 + (Y - cy) ** 2)
    circle_r = int(SIZE * 0.32)
    mask = dist <= circle_r
    fade = np.clip(1.0 - (dist - circle_r * 0.7) / (circle_r * 0.3), 0.0, 1.0)
    fade[~mask] = 0.0

    br, bg, bb = [c / 255.0 for c in blend_rgb]
    arr[:, :, 0] = np.clip(arr[:, :, 0] + br * fade * 0.85, 0.0, 1.0)
    arr[:, :, 1] = np.clip(arr[:, :, 1] + bg * fade * 0.85, 0.0, 1.0)
    arr[:, :, 2] = np.clip(arr[:, :, 2] + bb * fade * 0.85, 0.0, 1.0)

    # ── Complement chain bar at the bottom ──────────────────────────────────
    # 5 equal-width swatches showing the full chain (Primary→Tint→Tone→Shade→Pure)
    swatch_w = SIZE // 5
    swatch_h = int(SIZE * 0.06)
    y0 = SIZE - swatch_h
    for i, sd in enumerate(SHADE_DEFS):
        sw_hex = _blend_hex(primary_hex, complement_hex, sd["blend"])
        sw_rgb = _hex_to_rgb(sw_hex)
        x0, x1 = i * swatch_w, (i + 1) * swatch_w
        # Highlight the current swatch
        brightness = 1.0 if abs(sd["blend"] - blend) < 0.01 else 0.6
        arr[y0:SIZE, x0:x1, 0] = (sw_rgb[0] / 255.0) * brightness
        arr[y0:SIZE, x0:x1, 1] = (sw_rgb[1] / 255.0) * brightness
        arr[y0:SIZE, x0:x1, 2] = (sw_rgb[2] / 255.0) * brightness

    # ── PIL text overlay ─────────────────────────────────────────────────────
    img = Image.fromarray((np.clip(arr, 0, 1) * 255).astype("uint8"))
    img = img.filter(ImageFilter.GaussianBlur(radius=0.8))
    img = img.convert("RGBA")
    draw = ImageDraw.Draw(img)

    XO_GOLD   = (233, 196, 106)
    WARM_WHITE = (248, 246, 243)

    def try_font(size_pt):
        candidates = [
            "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
            "/System/Library/Fonts/Helvetica.ttc",
            "/Library/Fonts/Arial Bold.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        ]
        for p in candidates:
            if os.path.exists(p):
                try:
                    return ImageFont.truetype(p, size_pt)
                except Exception as e:
                    print(f'[WARN] Font load failed: {e}', file=sys.stderr)
        return ImageFont.load_default()

    font_large  = try_font(int(SIZE * 0.055))
    font_medium = try_font(int(SIZE * 0.032))
    font_small  = try_font(int(SIZE * 0.022))
    font_wm     = try_font(int(SIZE * 0.028))
    pad = int(SIZE * 0.04)

    # Accent bar (left edge) — blended colour
    bar_w = int(SIZE * 0.012)
    draw.rectangle([(pad, pad), (pad + bar_w, SIZE - pad - swatch_h)],
                   fill=blend_rgb + (200,))

    text_x = pad + bar_w + int(SIZE * 0.02)

    # Pack name
    draw.text((text_x + 3, int(SIZE * 0.06) + 3), pack_name, font=font_large,
              fill=(0, 0, 0, 140))
    draw.text((text_x, int(SIZE * 0.06)), pack_name, font=font_large, fill=WARM_WHITE)

    # Engine + shade pill
    pill_label = f"{engine_key.upper()}  ·  {shade_name.upper()}"
    pill_y = int(SIZE * 0.06) + int(SIZE * 0.075)
    pill_px, pill_py = int(SIZE * 0.015), int(SIZE * 0.008)
    bbox = draw.textbbox((text_x, pill_y), pill_label, font=font_medium)
    rect = (bbox[0] - pill_px, bbox[1] - pill_py,
            bbox[2] + pill_px, bbox[3] + pill_py)
    draw.rounded_rectangle(rect, radius=int(SIZE * 0.01), fill=blend_rgb)
    luma = 0.299 * blend_rgb[0] + 0.587 * blend_rgb[1] + 0.114 * blend_rgb[2]
    pill_fg = (0, 0, 0) if luma > 128 else (255, 255, 255)
    draw.text((text_x, pill_y), pill_label, font=font_medium, fill=pill_fg)

    # Blend percentage
    blend_text = f"{int(blend * 100)}% complement"
    meta_y = SIZE - pad - swatch_h - int(SIZE * 0.09)
    draw.text((text_x + 1, meta_y + 1), blend_text, font=font_small, fill=(0, 0, 0, 120))
    draw.text((text_x, meta_y), blend_text, font=font_small, fill=XO_GOLD)

    count_text = f"{preset_count} PRESETS  ·  v{version}"
    draw.text((text_x, meta_y + int(SIZE * 0.03)), count_text, font=font_small,
              fill=(160, 155, 150))

    # Shade labels for the 5-swatch bar
    for i, sd in enumerate(SHADE_DEFS):
        sx = i * swatch_w + int(swatch_w * 0.1)
        sy = SIZE - swatch_h + int(swatch_h * 0.2)
        sw_rgb = _hex_to_rgb(_blend_hex(primary_hex, complement_hex, sd["blend"]))
        luma_s = 0.299 * sw_rgb[0] + 0.587 * sw_rgb[1] + 0.114 * sw_rgb[2]
        fg = (0, 0, 0) if luma_s > 100 else (255, 255, 255)
        draw.text((sx, sy), sd["suffix"], font=try_font(int(SIZE * 0.016)), fill=fg)

    # Watermark
    wm_text = "XO_OX"
    wm_bbox = draw.textbbox((0, 0), wm_text, font=font_wm)
    wm_w, wm_h = wm_bbox[2] - wm_bbox[0], wm_bbox[3] - wm_bbox[1]
    draw.text((SIZE - pad - wm_w, SIZE - pad - swatch_h - wm_h - int(SIZE * 0.01)),
              wm_text, font=font_wm, fill=(255, 255, 255, 55))

    img = img.convert("RGB")

    output_dir.mkdir(parents=True, exist_ok=True)
    path_2000 = output_dir / "artwork_2000.png"
    path_1000 = output_dir / "artwork.png"
    img.save(str(path_2000), "PNG", optimize=True)
    img.resize((1000, 1000), Image.LANCZOS).save(str(path_1000), "PNG", optimize=True)

    return {"cover_1000": str(path_1000), "cover_2000": str(path_2000)}


# ---------------------------------------------------------------------------
# Core rendering logic
# ---------------------------------------------------------------------------

def render_complement_chain(
    engine_key: str,
    preset_name: str,
    preset_slug: str,
    output_dir: Path,
    engine_data: dict,
    sample_file: str = "",
    version: str = "1.0",
    skip_cover_art: bool = False,
    dry_run: bool = False,
    verbose: bool = True,
) -> list[dict]:
    """
    Render 5 XPM + cover art pairs for one preset across the complement chain.

    Returns a list of dicts, one per shade, each with keys:
        shade_name, blend, xpm_path, cover_paths, description
    """
    results = []

    primary_hex    = engine_data["primary"]
    complement_hex = engine_data["complement"]
    primary_name   = engine_data["primary_name"]
    comp_name      = engine_data["complement_name"]
    instrument     = engine_data.get("instrument", engine_key)
    shades         = engine_data.get("shades", [])

    # Build shade colour / name lookup from the doc-sourced table
    # shades list: [(hex, name, sonic_shift), ...] for indices 0-3 (Tint→Pure)
    shade_hex_map = {}
    shade_sonic_map = {}
    for i, sd in enumerate(shades):
        shade_hex_map[i]   = sd[0]  # hex for this shade step
        shade_sonic_map[i] = sd[2]  # sonic shift description

    for i, shade_def in enumerate(SHADE_DEFS):
        shade_name   = shade_def["name"]
        shade_suffix = shade_def["suffix"]
        blend        = shade_def["blend"]

        # Variant name and slug
        variant_name = f"{preset_name} {shade_suffix}"
        variant_slug = f"{preset_slug}_{shade_suffix}"

        # Blended colour for this step
        if blend == 0.0:
            blend_hex  = primary_hex
            blend_name = primary_name
            sonic_note = f"Engine at full {primary_name} character — {instrument}"
        elif blend == 1.0:
            blend_hex  = complement_hex
            blend_name = comp_name
            sonic_note = shade_sonic_map.get(3, f"Full complement — {comp_name}")
        else:
            shade_idx  = i - 1   # index into the 4-shade table (0–3)
            blend_hex  = shade_hex_map.get(shade_idx, _blend_hex(primary_hex, complement_hex, blend))
            blend_name = shades[shade_idx][1] if shade_idx < len(shades) else shade_name
            sonic_note = shade_sonic_map.get(shade_idx, "")

        description = (
            f"XO_OX {engine_key} | {shade_name} ({int(blend * 100)}% {comp_name}) | "
            f"{blend_name} {blend_hex} | {sonic_note}"
        )

        # Output directory for this variant
        variant_dir = output_dir / variant_slug
        xpm_path    = variant_dir / f"{variant_slug}.xpm"

        if verbose:
            print(f"    {shade_name:<8s}  {blend_hex}  {blend_name}")

        if not dry_run:
            variant_dir.mkdir(parents=True, exist_ok=True)
            xpm_content = _make_xpm(variant_name, description,
                                    sample_name=variant_name,
                                    sample_file=sample_file)
            xpm_path.write_text(xpm_content, encoding="utf-8")

        # Cover art
        cover_paths = {}
        if not skip_cover_art:
            cover_paths = _generate_complement_cover(
                engine_key=engine_key,
                pack_name=variant_name,
                shade_name=shade_name,
                blend=blend,
                output_dir=variant_dir,
                primary_hex=primary_hex,
                complement_hex=complement_hex,
                preset_count=1,
                version=version,
                seed=hash(variant_slug) % 100_000,
                dry_run=dry_run,
            )

        results.append({
            "shade_name":   shade_name,
            "shade_suffix": shade_suffix,
            "blend":        blend,
            "blend_hex":    blend_hex,
            "blend_name":   blend_name,
            "xpm_path":     str(xpm_path),
            "cover_paths":  cover_paths,
            "description":  description,
        })

    return results


def process_presets_dir(
    engine_key: str,
    presets_dir: Path,
    output_dir: Path,
    version: str = "1.0",
    skip_cover_art: bool = False,
    dry_run: bool = False,
    verbose: bool = True,
) -> dict:
    """
    Process all presets found in a directory for one engine.

    Accepts:
      - .xometa files (reads preset_name / preset_slug fields)
      - .xpm files (uses stem as preset name)
      - Directory itself (uses engine_key as a single preset name)

    Returns summary dict.
    """
    engine_data = COMPLEMENT_PAIRS.get(engine_key)
    if engine_data is None:
        raise ValueError(
            f"Engine '{engine_key}' not found in COMPLEMENT_PAIRS.\n"
            f"Available: {', '.join(sorted(COMPLEMENT_PAIRS.keys()))}"
        )

    presets = []

    if presets_dir.is_dir():
        # Gather .xometa files
        for xmeta in sorted(presets_dir.glob("**/*.xometa")):
            try:
                with open(xmeta) as f:
                    data = json.load(f)
                name = data.get("name", xmeta.stem)
                slug = data.get("preset_slug") or name.replace(" ", "_")
                presets.append({"name": name, "slug": slug, "source": str(xmeta)})
            except (json.JSONDecodeError, OSError) as e:
                if verbose:
                    print(f"    [WARN] Cannot parse {xmeta.name}: {e}")

        # Fallback: .xpm files
        if not presets:
            for xpm in sorted(presets_dir.glob("**/*.xpm")):
                name = xpm.stem.replace("_", " ")
                presets.append({"name": name, "slug": xpm.stem, "source": str(xpm)})

        # Final fallback: treat the dir as a single unnamed preset
        if not presets:
            slug = engine_key.replace(" ", "_")
            presets.append({"name": engine_key, "slug": slug, "source": str(presets_dir)})
    else:
        # presets_dir is actually a single file
        if presets_dir.suffix == ".xometa":
            try:
                with open(presets_dir) as f:
                    data = json.load(f)
                name = data.get("name", presets_dir.stem)
                slug = data.get("preset_slug") or name.replace(" ", "_")
                presets.append({"name": name, "slug": slug, "source": str(presets_dir)})
            except (json.JSONDecodeError, OSError) as e:
                raise RuntimeError(f"Cannot parse preset file: {e}") from e
        else:
            slug = presets_dir.stem
            presets.append({"name": slug.replace("_", " "), "slug": slug,
                             "source": str(presets_dir)})

    if verbose:
        print(f"  Engine:   {engine_key}")
        print(f"  Primary:  {engine_data['primary_name']} {engine_data['primary']}")
        print(f"  Complement: {engine_data['complement_name']} {engine_data['complement']}")
        print(f"  Presets:  {len(presets)}")
        print()

    all_variants = []
    for preset in presets:
        if verbose:
            print(f"  Preset: {preset['name']}")
        variants = render_complement_chain(
            engine_key=engine_key,
            preset_name=preset["name"],
            preset_slug=preset["slug"],
            output_dir=output_dir / engine_key,
            engine_data=engine_data,
            version=version,
            skip_cover_art=skip_cover_art,
            dry_run=dry_run,
            verbose=verbose,
        )
        all_variants.extend(variants)
        if verbose:
            print()

    return {
        "engine":       engine_key,
        "preset_count": len(presets),
        "variant_count": len(all_variants),
        "output_dir":   str(output_dir / engine_key),
        "variants":     all_variants,
    }


# ---------------------------------------------------------------------------
# oxport.py integration hook
# ---------------------------------------------------------------------------

def run_complement_stage(
    engine_key: str,
    output_dir: Path,
    presets_dir: Optional[Path] = None,
    version: str = "1.0",
    skip_cover_art: bool = False,
    dry_run: bool = False,
    verbose: bool = True,
) -> dict:
    """
    Integration point for oxport.py when --collection artwork is specified.

    This function is imported and called by oxport's complement_chain stage.
    It mirrors the signature style of other oxport stage helpers.

    Args:
        engine_key:     Artwork engine key (e.g., "XOxblood").
        output_dir:     Root output directory (same as PipelineContext.output_dir).
        presets_dir:    Optional directory of .xometa / .xpm presets. When None
                        the function generates a placeholder Primary program only.
        version:        Pack version string.
        skip_cover_art: If True, skip the cover art generation step.
        dry_run:        If True, print what would happen but write nothing.
        verbose:        Print progress information.

    Returns:
        Summary dict compatible with PipelineContext state tracking.
    """
    if engine_key not in COMPLEMENT_PAIRS:
        if verbose:
            print(f"    [SKIP] {engine_key} not in Artwork collection "
                  f"— complement stage does not apply")
        return {"skipped": True, "reason": "not_artwork_engine"}

    search_dir = presets_dir or output_dir
    return process_presets_dir(
        engine_key=engine_key,
        presets_dir=search_dir,
        output_dir=output_dir,
        version=version,
        skip_cover_art=skip_cover_art,
        dry_run=dry_run,
        verbose=verbose,
    )


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="xpn_complement_renderer",
        description=(
            "XPN Complement Chain Renderer — XO_OX Designs\n"
            "Generates primary + complement variant XPM pairs for Artwork/Color engines."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )

    src = p.add_mutually_exclusive_group(required=False)
    src.add_argument(
        "--engine", metavar="NAME",
        help="Single engine key, e.g. XOxblood",
    )
    src.add_argument(
        "--all-engines", action="store_true",
        help="Process all 24 Artwork engines (use with --collection artwork)",
    )

    p.add_argument(
        "--collection", metavar="NAME",
        help="Collection name (currently only 'artwork' supported). "
             "Used with --all-engines to scope the run.",
    )
    p.add_argument(
        "--presets", metavar="DIR",
        help="Directory of .xometa or .xpm preset files.",
    )
    p.add_argument(
        "--output", required=True, metavar="DIR",
        help="Root output directory. Variants are written to OUTPUT/ENGINE/PRESET_shade/",
    )
    p.add_argument(
        "--version", default="1.0", metavar="VER",
        help="Pack version string (default: 1.0)",
    )
    p.add_argument(
        "--skip-cover-art", action="store_true",
        help="Skip cover art generation (faster; useful for pipeline testing)",
    )
    p.add_argument(
        "--dry-run", action="store_true",
        help="Show what would be generated without writing any files",
    )
    p.add_argument(
        "--list-engines", action="store_true",
        help="Print all supported Artwork engine keys and exit",
    )
    return p


def main(argv=None):
    parser = _build_parser()
    args   = parser.parse_args(argv)

    print(BANNER)

    if args.list_engines:
        print("Supported Artwork engine keys:")
        for k in sorted(COMPLEMENT_PAIRS.keys()):
            ed = COMPLEMENT_PAIRS[k]
            print(f"  {k:<20s}  {ed['primary_name']:<22s} → {ed['complement_name']}")
        return 0

    if not args.engine and not args.all_engines:
        parser.error("one of the arguments --engine --all-engines is required")

    output_dir   = Path(args.output)
    presets_dir  = Path(args.presets) if args.presets else None

    if args.dry_run:
        print("  Mode: DRY RUN — no files will be written\n")

    t0 = time.monotonic()

    if args.all_engines:
        if args.collection and args.collection.lower() != "artwork":
            print(f"ERROR: --collection '{args.collection}' is not supported. "
                  f"Only 'artwork' is valid.")
            return 1

        print(f"  Processing all {len(ARTWORK_ENGINES)} Artwork engines\n")
        total_presets  = 0
        total_variants = 0

        for eng in ARTWORK_ENGINES:
            print(f"─── {eng} " + "─" * max(0, 50 - len(eng)))
            eng_presets = presets_dir / eng if (presets_dir and presets_dir.is_dir()) else presets_dir
            try:
                result = process_presets_dir(
                    engine_key=eng,
                    presets_dir=eng_presets or output_dir / eng,
                    output_dir=output_dir,
                    version=args.version,
                    skip_cover_art=args.skip_cover_art,
                    dry_run=args.dry_run,
                    verbose=True,
                )
                total_presets  += result["preset_count"]
                total_variants += result["variant_count"]
            except Exception as e:
                print(f"  [WARN] {eng}: {e}\n")

        elapsed = time.monotonic() - t0
        print("=" * 60)
        print(f"  COMPLETE — {len(ARTWORK_ENGINES)} engines, "
              f"{total_presets} presets, {total_variants} variants")
        print(f"  Output:  {output_dir}")
        print(f"  Time:    {elapsed:.2f}s")

    else:
        engine_key = args.engine
        if engine_key not in COMPLEMENT_PAIRS:
            print(f"ERROR: '{engine_key}' is not a known Artwork engine.\n"
                  f"Run with --list-engines to see available engines.")
            return 1

        src_dir = presets_dir or output_dir
        try:
            result = process_presets_dir(
                engine_key=engine_key,
                presets_dir=src_dir,
                output_dir=output_dir,
                version=args.version,
                skip_cover_art=args.skip_cover_art,
                dry_run=args.dry_run,
                verbose=True,
            )
        except Exception as e:
            print(f"ERROR: {e}")
            return 1

        elapsed = time.monotonic() - t0
        print("=" * 60)
        print(f"  COMPLETE — {result['preset_count']} presets × "
              f"{len(SHADE_DEFS)} shades = {result['variant_count']} variants")
        print(f"  Output:  {result['output_dir']}")
        print(f"  Time:    {elapsed:.2f}s")
        if args.dry_run:
            print("  (Dry run — no files written)")

    return 0


if __name__ == "__main__":
    sys.exit(main())
