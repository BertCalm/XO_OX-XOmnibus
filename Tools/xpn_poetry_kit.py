#!/usr/bin/env python3
"""
XPN Poetry Kit — XO_OX Designs
CMU Pronouncing Dictionary stress patterns → drum rhythms.

Each syllable = 1 sixteenth-note step.
Stress mapping:
  1 (primary)    → Kick or Snare (alternating), velocity 100-127
  2 (secondary)  → Closed Hat or Clap, velocity 60-80
  0 (unstressed) → Ghost Rim hit, velocity 1-30

4 hardcoded poem fragments with correct prosodic stress:
  shakespeare  — Sonnet 18: "Shall I Compare Thee"
  dickinson    — "Because I Could Not Stop for Death"
  ginsberg     — Howl: "Best Minds of My Generation"
  hughes       — "A Dream Deferred"

CLI:
  python xpn_poetry_kit.py --poem shakespeare --output ./out/
  python xpn_poetry_kit.py --poem all --output ./out/
  python xpn_poetry_kit.py --text "any phrase here" --output ./out/
"""

import argparse
import json
import re
import sys
import zipfile
from datetime import date
from pathlib import Path
from xml.sax.saxutils import escape as xml_escape

# =============================================================================
# CMU PRONOUNCING DICTIONARY — Hardcoded stress data (150+ words)
# Format: word (lowercase) → list of ints, 0=unstressed 1=primary 2=secondary
# One int per syllable.
# =============================================================================

STRESS_DICT = {
    # A
    "a":            [0],
    "about":        [0, 1],
    "above":        [0, 1],
    "again":        [0, 1],
    "age":          [1],
    "all":          [1],
    "am":           [0],
    "an":           [0],
    "and":          [0],
    "angel":        [1, 0],
    "angry":        [1, 0],
    "are":          [0],
    "as":           [0],
    "at":           [0],
    "away":         [0, 1],
    # B
    "be":           [0],
    "because":      [0, 1],
    "been":         [0],
    "best":         [1],
    "both":         [1],
    "bright":       [1],
    "buds":         [1],
    "burned":       [1],
    "burning":      [1, 0],
    "but":          [0],
    "by":           [0],
    # C
    "can":          [0],
    "carriage":     [1, 0],
    "civility":     [0, 1, 0, 0],
    "cold":         [1],
    "compare":      [0, 1],
    "complexion":   [0, 1, 0],
    "connections":  [0, 1, 0],
    "could":        [0],
    "course":       [1],
    "crust":        [1],
    "crusted":      [1, 0],
    # D
    "dare":         [1],
    "darling":      [1, 0],
    "date":         [1],
    "day":          [1],
    "dead":         [1],
    "dear":         [1],
    "death":        [1],
    "declined":     [0, 1],
    "declines":     [0, 1],
    "deferred":     [0, 1],
    "destroyed":    [0, 1],
    "dim":          [1],
    "dimmed":       [1],
    "do":           [0],
    "does":         [0],
    "dragging":     [1, 0],
    "dream":        [1],
    "dried":        [1],
    "drugs":        [1],
    "dry":          [1],
    "drying":       [1, 0],
    # E
    "eternal":      [0, 1, 0],
    "ever":         [1, 0],
    "every":        [1, 0],
    "explosive":    [0, 1, 0],
    "eyes":         [1],
    # F
    "fair":         [1],
    "fall":         [1],
    "farewell":     [0, 1],
    "fester":       [1, 0],
    "flesh":        [1],
    "for":          [0],
    "from":         [0],
    # G
    "generation":   [2, 0, 1, 0],
    "gold":         [1],
    "gone":         [1],
    # H
    "happen":       [1, 0],
    "happens":      [1, 0],
    "hath":         [0],
    "he":           [0],
    "heaven":       [1, 0],
    "heavy":        [1, 0],
    "his":          [0],
    "holy":         [1, 0],
    "hot":          [1],
    "how":          [1],
    "howled":       [1],
    "howling":      [1, 0],
    "hung":         [1],
    "hysterical":   [0, 1, 0, 0],
    # I
    "i":            [1],
    "if":           [0],
    "illuminated":  [0, 2, 0, 1, 0],
    "in":           [0],
    "intelligence": [0, 1, 0, 0],
    "is":           [0],
    "it":           [0],
    # J
    "jazz":         [1],
    # K
    "kindly":       [1, 0],
    "kindness":     [1, 0],
    "knew":         [1],
    # L
    "labor":        [1, 0],
    "lease":        [1],
    "leaving":      [1, 0],
    "leisure":      [1, 0],
    "like":         [1],
    "lines":        [1],
    "load":         [1],
    "long":         [1],
    "love":         [1],
    "lovely":       [1, 0],
    "lowly":        [1, 0],
    # M
    "machinery":    [0, 1, 0, 0],
    "madness":      [1, 0],
    "magnetic":     [0, 1, 0],
    "may":          [0],
    "me":           [0],
    "meat":         [1],
    "midnight":     [1, 0],
    "minds":        [1],
    "more":         [0],
    "mortality":    [0, 1, 0, 0],
    "mostly":       [1, 0],
    "my":           [0],
    "mystical":     [1, 0, 0],
    # N
    "naked":        [1, 0],
    "nature":       [1, 0],
    "negro":        [1, 0],
    "night":        [1],
    "nor":          [0],
    "not":          [1],
    # O
    "of":           [0],
    "often":        [1, 0],
    "on":           [0],
    "only":         [1, 0],
    "or":           [0],
    "our":          [0],
    "out":          [1],
    "over":         [1, 0],
    "owe":          [1],
    # P
    "passed":       [1],
    "pause":        [1],
    "possesses":    [0, 1, 0],
    "poverty":      [1, 0, 0],
    # R
    "raging":       [1, 0],
    "raisin":       [1, 0],
    "ran":          [1],
    "recess":       [0, 1],
    "repose":       [0, 1],
    "ring":         [1],
    "rot":          [1],
    "rough":        [1],
    "run":          [1],
    # S
    "sag":          [1],
    "saw":          [1],
    "scarce":       [1],
    "school":       [1],
    "see":          [1],
    "seeking":      [1, 0],
    "setting":      [1, 0],
    "shall":        [0],
    "shakes":       [1],
    "shine":        [1],
    "short":        [1],
    "since":        [0],
    "sky":          [1],
    "slowly":       [1, 0],
    "something":    [1, 0],
    "sometime":     [1, 0],
    "sometimes":    [1, 0],
    "sore":         [1],
    "star":         [1],
    "stars":        [1],
    "starving":     [1, 0],
    "stink":        [1],
    "stinking":     [1, 0],
    "stop":         [1],
    "stopped":      [1],
    "streets":      [1],
    "strife":       [1],
    "sugar":        [1, 0],
    "summer":       [1, 0],
    "sun":          [1],
    "surmised":     [0, 1],
    "sweet":        [1],
    "swelling":     [1, 0],
    "syrupy":       [1, 0, 0],
    # T
    "temperate":    [1, 0, 0],
    "the":          [0],
    "thee":         [0],
    "their":        [0],
    "then":         [0],
    "there":        [0],
    "they":         [0],
    "this":         [0],
    "time":         [1],
    "to":           [0],
    "too":          [0],
    "toward":       [0, 1],
    "trimmed":      [1],
    # U
    "under":        [1, 0],
    "untrimmed":    [0, 1],
    "up":           [1],
    # V
    "vision":       [1, 0],
    "visionary":    [0, 1, 0, 0],
    # W
    "wander":       [1, 0],
    "was":          [0],
    "we":           [0],
    "what":         [1],
    "when":         [0],
    "where":        [0],
    "who":          [0],
    "will":         [0],
    "winds":        [1],
    "with":         [0],
    # Extended
    "buds":         [1],
    "darling":      [1, 0],
    "dim":          [1],
    "explode":      [0, 1],
    "fair":         [1],
    "heaven's":    [1, 0],
    "nature's":    [1, 0],
    "ohat":         [1, 1],
}

# =============================================================================
# FOUR HARDCODED POEM FRAGMENTS
# Format: list of (word_label, [stress_per_syllable]) tuples
# Stresses are hand-scanned for prosodic accuracy.
# =============================================================================

# Shakespeare Sonnet 18 — iambic pentameter
# "Shall I com-PARE thee TO a SUM-mer's DAY"
#   0    1   0   1    0   1  0   1    0    1
POEM_SHAKESPEARE = [
    ("Shall",   [0]),
    ("I",       [1]),
    ("com",     [0]),
    ("pare",    [1]),
    ("thee",    [0]),
    ("to",      [1]),
    ("a",       [0]),
    ("sum",     [1]),
    ("mer's",  [0]),
    ("day",     [1]),
]

# Emily Dickinson — "Because I could not stop for Death —
#                    He kindly stopped for me"
# BE-cause  I  could  not  STOP  for  DEATH —  He  KIND-ly  STOPPED  for  me
#  0   1    0    0     1    1     0    1         0   1    0     1       0   0
POEM_DICKINSON = [
    ("Be",      [0]),
    ("cause",   [1]),
    ("I",       [0]),
    ("could",   [0]),
    ("not",     [1]),
    ("stop",    [1]),
    ("for",     [0]),
    ("Death",   [1]),
    ("He",      [0]),
    ("kind",    [1]),
    ("ly",      [0]),
    ("stopped", [1]),
    ("for",     [0]),
    ("me",      [0]),
]

# Allen Ginsberg Howl — "I saw the best minds of my generation
#                         destroyed by madness"
# I  SAW  the  BEST  MINDS  of  my  GEN-er-A-tion  de-STROYED  by  MAD-ness
# 1   1    0    1     1      0   0   2   0   1   0    0    1     0   1    0
POEM_GINSBERG = [
    ("I",       [1]),
    ("saw",     [1]),
    ("the",     [0]),
    ("best",    [1]),
    ("minds",   [1]),
    ("of",      [0]),
    ("my",      [0]),
    ("gen",     [2]),
    ("er",      [0]),
    ("a",       [1]),
    ("tion",    [0]),
    ("de",      [0]),
    ("stroyed", [1]),
    ("by",      [0]),
    ("mad",     [1]),
    ("ness",    [0]),
]

# Langston Hughes — "What happens to a dream deferred —
#                    Does it dry up like a raisin in the sun"
# WHAT  HAP-pens  to  a  DREAM  de-FERRED —
#  1     1    0    0  0    1     0    1
# DOES  it  DRY  UP  like  a  RAI-sin  in  the  SUN
#  0    0    1    1    0    0    1   0   0    0    1
POEM_HUGHES = [
    ("What",    [1]),
    ("hap",     [1]),
    ("pens",    [0]),
    ("to",      [0]),
    ("a",       [0]),
    ("dream",   [1]),
    ("de",      [0]),
    ("ferred",  [1]),
    ("Does",    [0]),
    ("it",      [0]),
    ("dry",     [1]),
    ("up",      [1]),
    ("like",    [0]),
    ("a",       [0]),
    ("rai",     [1]),
    ("sin",     [0]),
    ("in",      [0]),
    ("the",     [0]),
    ("sun",     [1]),
]

POEMS = {
    "shakespeare": POEM_SHAKESPEARE,
    "dickinson":   POEM_DICKINSON,
    "ginsberg":    POEM_GINSBERG,
    "hughes":      POEM_HUGHES,
}

POEM_TITLES = {
    "shakespeare": "Sonnet 18 — Shall I Compare Thee",
    "dickinson":   "Because I Could Not Stop for Death",
    "ginsberg":    "Howl — Best Minds of My Generation",
    "hughes":      "A Dream Deferred",
}

# =============================================================================
# DRUM VOICE DEFINITIONS
# =============================================================================

VOICE_DEFAULTS = {
    "kick":  {"mono": True,  "poly": 1, "one_shot": True,  "vtp": 0.05, "vtf": 0.0,
               "attack": 0.0, "decay": 0.3, "sustain": 0.0, "release": 0.05,
               "cutoff": 1.0, "resonance": 0.0, "mute_group": 0},
    "snare": {"mono": True,  "poly": 1, "one_shot": True,  "vtp": 0.0,  "vtf": 0.30,
               "attack": 0.0, "decay": 0.4, "sustain": 0.0, "release": 0.08,
               "cutoff": 0.9, "resonance": 0.05, "mute_group": 0},
    "chat":  {"mono": True,  "poly": 1, "one_shot": True,  "vtp": 0.0,  "vtf": 0.0,
               "attack": 0.0, "decay": 0.15, "sustain": 0.0, "release": 0.02,
               "cutoff": 0.85, "resonance": 0.1, "mute_group": 1},
    "clap":  {"mono": False, "poly": 2, "one_shot": True,  "vtp": 0.0,  "vtf": 0.15,
               "attack": 0.0, "decay": 0.5, "sustain": 0.0, "release": 0.1,
               "cutoff": 0.95, "resonance": 0.0, "mute_group": 0},
    "rim":   {"mono": False, "poly": 2, "one_shot": True,  "vtp": 0.0,  "vtf": 0.0,
               "attack": 0.0, "decay": 0.35, "sustain": 0.0, "release": 0.08,
               "cutoff": 1.0, "resonance": 0.0, "mute_group": 0},
}

VOICE_MIDI = {"kick": 36, "snare": 38, "chat": 42, "clap": 39, "rim": 43}


# =============================================================================
# STRESS → DRUM HIT MAPPING
# =============================================================================

def stress_to_hit(stress: int, primary_counter: int) -> tuple:
    """
    Convert syllable stress level to (voice_name, midi_note, vel_lo, vel_hi).
    primary_counter tracks how many primary-stress syllables have been seen so far;
    even = kick, odd = snare. Same alternation for secondary (chat/clap).
    """
    if stress == 1:
        if primary_counter % 2 == 0:
            return ("kick",  36, 100, 127)
        else:
            return ("snare", 38, 100, 127)
    elif stress == 2:
        if primary_counter % 2 == 0:
            return ("chat", 42, 60, 80)
        else:
            return ("clap", 39, 60, 80)
    else:
        return ("rim", 43, 1, 30)


def text_to_syllables(text: str) -> list:
    """
    Convert free text to list of (syllable_label, stress_int) pairs.
    Looks up each word in STRESS_DICT; falls back to 2-syllable even split if unknown.
    """
    syllables = []
    words = re.split(r"\s+", text.strip())
    for word in words:
        if not word:
            continue
        clean = re.sub(r"[^a-zA-Z'-]", "", word).lower()
        # Try the word, then strip trailing possessive/plural markers
        for variant in [clean, clean.rstrip("s"), clean.rstrip("'s"), clean + "s"]:
            if variant in STRESS_DICT:
                stresses = STRESS_DICT[variant]
                if len(stresses) == 1:
                    syllables.append((clean, stresses[0]))
                else:
                    for i, s in enumerate(stresses):
                        syllables.append((f"{clean}-{i+1}", s))
                break
        else:
            # Fallback: guess 2-syllable split for longer words, monosyllable for short
            if len(clean) > 3:
                syllables.append((clean + "-1", 1))
                syllables.append((clean + "-2", 0))
            else:
                syllables.append((clean, 1))
    return syllables


def poem_to_syllables(poem_data: list) -> list:
    """Convert poem data (list of (label, [stresses])) to flat syllable list."""
    syllables = []
    for word_label, stresses in poem_data:
        if len(stresses) == 1:
            syllables.append((word_label, stresses[0]))
        else:
            for i, s in enumerate(stresses):
                syllables.append((f"{word_label}-{i+1}", s))
    return syllables


# =============================================================================
# SCANSION REPORT
# =============================================================================

def build_scansion_report(title: str, syllables: list, hits: list) -> str:
    """Build human-readable scansion report with DUM/dum visual pattern."""
    DUM_MAP = {1: "DUM", 2: "dum", 0: " . "}

    lines = []
    lines.append("=" * 72)
    lines.append("XO_OX POETRY KIT — SCANSION REPORT")
    lines.append(f"Poem     : {title}")
    lines.append(f"Date     : {date.today()}")
    lines.append(f"Syllables: {len(syllables)}")
    lines.append("=" * 72)
    lines.append("")

    # Visual pattern line
    pattern = "  ".join(DUM_MAP[s] for _, s in syllables)
    lines.append("Stress pattern:")
    lines.append("  " + pattern)
    lines.append("")

    # Step table
    lines.append(f"{'Step':>4}  {'Syllable':<14}  {'Stress':<10}  {'Voice':<8}  {'Vel Range':>10}  DUM")
    lines.append("-" * 72)
    stress_labels = {0: "unstressed", 1: "PRIMARY", 2: "secondary"}
    for i, ((label, stress), hit) in enumerate(zip(syllables, hits)):
        if hit:
            v_name, midi, vel_lo, vel_hi = hit
            vel_str = f"{vel_lo}-{vel_hi}"
        else:
            v_name, vel_str = "—", "—"
        dum = DUM_MAP[stress]
        lines.append(
            f"{i+1:>4}  {label:<14}  {stress_labels[stress]:<10}  {v_name:<8}  {vel_str:>10}  {dum}"
        )

    lines.append("")
    lines.append("Voice legend:")
    lines.append("  DUM = PRIMARY stress    → Kick (even) / Snare (odd),  vel 100-127")
    lines.append("  dum = secondary stress  → CHat (even) / Clap  (odd),  vel  60-80")
    lines.append("   .  = unstressed        → Ghost Rim,                   vel   1-30")
    lines.append("")
    lines.append("XPM golden rules applied:")
    lines.append("  KeyTrack=True  |  RootNote=0  |  empty-layer VelStart=0")
    lines.append("=" * 72)
    return "\n".join(lines)


# =============================================================================
# XPM GENERATION
# =============================================================================

def _layer_block(number: int, vel_start: int, vel_end: int,
                 sample_name: str, sample_file: str,
                 volume: float = 0.707946) -> str:
    active = "True" if sample_name else "False"
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


def _instrument_block(instrument_num: int, voice_name: str,
                      sample_name: str, sample_file: str,
                      vel_lo: int = 1, vel_hi: int = 127,
                      mute_group: int = 0) -> str:
    is_active = bool(voice_name and sample_name)
    cfg = VOICE_DEFAULTS.get(voice_name, VOICE_DEFAULTS["rim"])

    if is_active:
        volume = max(0.1, vel_hi / 127.0)
        layers_xml = _layer_block(1, vel_lo, vel_hi, sample_name, sample_file, volume)
        for i in range(2, 5):
            layers_xml += "\n" + _layer_block(i, 0, 0, "", "", 0.707946)
    else:
        layers_xml = _empty_layers()

    mono_str    = "True" if cfg["mono"] else "False"
    oneshot_str = "True" if cfg["one_shot"] else "False"

    mute_xml = "\n".join(
        f"        <MuteTarget{i+1}>0</MuteTarget{i+1}>" for i in range(4)
    )
    if voice_name == "chat":
        mute_xml = (
            "        <MuteTarget1>39</MuteTarget1>\n"
            + "\n".join(f"        <MuteTarget{i+1}>0</MuteTarget{i+1}>" for i in range(1, 4))
        )
    simult_xml = "\n".join(
        f"        <SimultTarget{i+1}>0</SimultTarget{i+1}>" for i in range(4)
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


def build_xpm(slug: str, title: str, syllables: list, hits: list) -> str:
    """
    Build XPM XML for the poetry drum program.
    5 active pads: kick(36), snare(38), chat(42), clap(39), rim(43).
    """
    prog_name = xml_escape(f"XO_OX-POETRY-{slug.upper()}")

    # The 5 drum voice slots: MIDI note → (voice_key, vel_lo, vel_hi)
    # We use fixed velocity ranges per stress level
    active_voices = [
        (36, "kick",  100, 127),
        (38, "snare", 100, 127),
        (42, "chat",   60,  80),
        (39, "clap",   60,  80),
        (43, "rim",     1,  30),
    ]

    instrument_parts = []
    for i in range(128):
        matched = None
        for midi, v_name, vel_lo, vel_hi in active_voices:
            if i == midi:
                matched = (v_name, vel_lo, vel_hi)
                break
        if matched:
            v_name, vel_lo, vel_hi = matched
            s_name = f"{slug}_{v_name}"
            s_file = f"{s_name}.wav"
            mg = VOICE_DEFAULTS[v_name]["mute_group"]
            instrument_parts.append(
                _instrument_block(i, v_name, s_name, s_file,
                                  vel_lo=vel_lo, vel_hi=vel_hi, mute_group=mg)
            )
        else:
            instrument_parts.append(_instrument_block(i, "", "", "", mute_group=0))

    instruments_xml = "\n".join(instrument_parts)

    pad_note_xml = (
        '        <Pad number="1" note="36"/>  <!-- Kick: primary stress (even) -->\n'
        '        <Pad number="2" note="38"/>  <!-- Snare: primary stress (odd) -->\n'
        '        <Pad number="3" note="42"/>  <!-- ClosedHat: secondary (even) -->\n'
        '        <Pad number="4" note="39"/>  <!-- Clap: secondary stress (odd) -->\n'
        '        <Pad number="5" note="43"/>  <!-- Rim: unstressed / ghost -->'
    )
    pad_group_xml = '        <Pad number="3" group="1"/>'

    pad_json = json.dumps(
        {"ProgramPads": {"Universal": {"value0": False},
                         "Type":      {"value0": 5},
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
        f'    <!-- Poetry Kit: {xml_escape(title)} -->\n'
        f'    <!-- Syllable count: {len(syllables)} | Each = 1 sixteenth note -->\n'
        f'    <ProgramPads>{pad_json}</ProgramPads>\n'
        '    <PadNoteMap>\n'
        f'{pad_note_xml}\n'
        '    </PadNoteMap>\n'
        '    <PadGroupMap>\n'
        f'{pad_group_xml}\n'
        '    </PadGroupMap>\n'
        '    <QLinks>\n'
        '      <QLink number="1">\n'
        '        <Name>TONE</Name>\n'
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
        '        <Name>ACCENT</Name>\n'
        '        <Parameter>VelocitySensitivity</Parameter>\n'
        '        <Min>0.500000</Min>\n'
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
# MANIFEST
# =============================================================================

def build_manifest(slug: str, title: str, syllables: list, hits: list) -> dict:
    stress_counts = {0: 0, 1: 0, 2: 0}
    for _, s in syllables:
        stress_counts[s] = stress_counts.get(s, 0) + 1

    return {
        "tool":    "xpn_poetry_kit",
        "version": "1.0",
        "date":    str(date.today()),
        "slug":    slug,
        "title":   title,
        "syllables": len(syllables),
        "stress_breakdown": {
            "primary":    stress_counts[1],
            "secondary":  stress_counts[2],
            "unstressed": stress_counts[0],
        },
        "pads": [
            {"pad": 1, "note": 36, "voice": "kick",  "stress": "primary (even)",    "vel": "100-127"},
            {"pad": 2, "note": 38, "voice": "snare", "stress": "primary (odd)",     "vel": "100-127"},
            {"pad": 3, "note": 42, "voice": "chat",  "stress": "secondary (even)",  "vel": "60-80"},
            {"pad": 4, "note": 39, "voice": "clap",  "stress": "secondary (odd)",   "vel": "60-80"},
            {"pad": 5, "note": 43, "voice": "rim",   "stress": "unstressed/ghost",  "vel": "1-30"},
        ],
        "golden_rules": {"KeyTrack": True, "RootNote": 0, "empty_layer_VelStart": 0},
        "syllable_map": [
            {
                "step":   i + 1,
                "label":  label,
                "stress": stress,
                "voice":  hits[i][0] if hits[i] else None,
                "vel_lo": hits[i][2] if hits[i] else 0,
                "vel_hi": hits[i][3] if hits[i] else 0,
            }
            for i, (label, stress) in enumerate(syllables)
        ],
    }


# =============================================================================
# ZIP PACKAGING
# =============================================================================

def write_kit_zip(output_dir: Path, slug: str, title: str,
                  syllables: list, hits: list) -> Path:
    """Package XPM + scansion_report.txt + manifest.json + README into a ZIP."""
    output_dir.mkdir(parents=True, exist_ok=True)
    zip_path = output_dir / f"XO_OX-POETRY-{slug.upper()}.zip"

    xpm_xml  = build_xpm(slug, title, syllables, hits)
    report   = build_scansion_report(title, syllables, hits)
    manifest = build_manifest(slug, title, syllables, hits)

    readme = (
        f"XO_OX POETRY KIT — {title}\n"
        "=" * 60 + "\n\n"
        "Place your WAV samples in this folder next to the .xpm:\n\n"
        f"  {slug}_kick.wav    — Kick drum  (primary stress, even syllables)\n"
        f"  {slug}_snare.wav   — Snare      (primary stress, odd syllables)\n"
        f"  {slug}_chat.wav    — Closed Hat (secondary stress, even syllables)\n"
        f"  {slug}_clap.wav    — Clap       (secondary stress, odd syllables)\n"
        f"  {slug}_rim.wav     — Rim/Ghost  (unstressed syllables)\n\n"
        "Pad → MIDI note mapping:\n"
        "  Pad 1 → 36 (Kick)   vel 100-127\n"
        "  Pad 2 → 38 (Snare)  vel 100-127\n"
        "  Pad 3 → 42 (CHat)   vel  60-80\n"
        "  Pad 4 → 39 (Clap)   vel  60-80\n"
        "  Pad 5 → 43 (Rim)    vel   1-30\n\n"
        "See scansion_report.txt for the full step-by-step stress map.\n"
        "See manifest.json for machine-readable syllable data.\n"
    )

    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
        zf.writestr(f"{slug}/{slug}.xpm",           xpm_xml)
        zf.writestr(f"{slug}/scansion_report.txt",  report)
        zf.writestr(f"{slug}/manifest.json",        json.dumps(manifest, indent=2))
        zf.writestr(f"{slug}/README.txt",           readme)

    return zip_path


# =============================================================================
# CLI
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Poetry Kit — stress patterns → MPC drum programs"
    )
    src = parser.add_mutually_exclusive_group(required=True)
    src.add_argument(
        "--poem",
        choices=["shakespeare", "dickinson", "ginsberg", "hughes", "all"],
        help="Use a hardcoded poem fragment",
    )
    src.add_argument(
        "--text",
        metavar="PHRASE",
        help='Any phrase, e.g. --text "hello world this is a test"',
    )
    parser.add_argument(
        "--output", "-o",
        default="./out",
        help="Output directory (default: ./out)",
    )
    args = parser.parse_args()
    output_dir = Path(args.output)

    if args.text:
        syllables = text_to_syllables(args.text)
        if not syllables:
            print("ERROR: No syllables extracted from text.", file=sys.stderr)
            sys.exit(1)
        primary_counter = 0
        hits = []
        for _, stress in syllables:
            hits.append(stress_to_hit(stress, primary_counter))
            if stress == 1:
                primary_counter += 1
        slug  = "custom"
        title = f"Custom: {args.text[:50]}"
        zip_path = write_kit_zip(output_dir, slug, title, syllables, hits)
        print(f"Written: {zip_path}")
        _print_stats(slug, syllables)

    else:
        keys = list(POEMS.keys()) if args.poem == "all" else [args.poem]
        for key in keys:
            title     = POEM_TITLES[key]
            syllables = poem_to_syllables(POEMS[key])
            primary_counter = 0
            hits = []
            for _, stress in syllables:
                hits.append(stress_to_hit(stress, primary_counter))
                if stress == 1:
                    primary_counter += 1
            zip_path = write_kit_zip(output_dir, key, title, syllables, hits)
            print(f"Written: {zip_path}")
            _print_stats(key, syllables)


def _print_stats(slug: str, syllables: list) -> None:
    n_primary   = sum(1 for _, s in syllables if s == 1)
    n_secondary = sum(1 for _, s in syllables if s == 2)
    n_ghost     = sum(1 for _, s in syllables if s == 0)
    print(f"  {len(syllables)} syllables  |  "
          f"{n_primary} primary (kick/snare)  "
          f"{n_secondary} secondary (chat/clap)  "
          f"{n_ghost} ghost (rim)")


if __name__ == "__main__":
    main()
