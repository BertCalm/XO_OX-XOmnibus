#!/usr/bin/env python3
"""
XPN Braille Rhythm Kit Generator — XO_OX Designs
Hex + Rex build: braille dots as drum patterns.

Each Braille letter is a 2×3 grid of 6 dot positions.
Active dots = drum hits on a 6-step rhythmic grid.
Spell words to generate beats.

Braille position layout (standard Grade 1):
  Col 1 (left)   Col 2 (right)
  Pos 1  Pos 4   ← row 1 (top)
  Pos 2  Pos 5   ← row 2 (middle)
  Pos 3  Pos 6   ← row 3 (bottom)

Position → drum voice mapping:
  Pos 1 (top-left)     = Kick         MIDI 36
  Pos 2 (mid-left)     = Snare        MIDI 38
  Pos 3 (bottom-left)  = Closed Hat   MIDI 42
  Pos 4 (top-right)    = Open Hat     MIDI 46
  Pos 5 (mid-right)    = Clap         MIDI 39
  Pos 6 (bottom-right) = Rim/Perc     MIDI 43

CLI:
  python xpn_braille_rhythm_kit.py "hello world" --output ./kits/
  python xpn_braille_rhythm_kit.py "xo ox" --output ./kits/ --step-size 8th
  python xpn_braille_rhythm_kit.py "love" --polyrhythm "hate" --output ./kits/
  python xpn_braille_rhythm_kit.py --alphabet
"""

import argparse
import json
import sys
from datetime import date
from pathlib import Path
from xml.sax.saxutils import escape as xml_escape


# =============================================================================
# BRAILLE ALPHABET (Grade 1 + common Grade 2 contractions)
# Dot sets — numbers refer to positions 1-6 as defined above.
# =============================================================================

BRAILLE_ALPHA = {
    # Letters
    'a': [1],
    'b': [1, 2],
    'c': [1, 4],
    'd': [1, 4, 5],
    'e': [1, 5],
    'f': [1, 2, 4],
    'g': [1, 2, 4, 5],
    'h': [1, 2, 5],
    'i': [2, 4],
    'j': [2, 4, 5],
    'k': [1, 3],
    'l': [1, 2, 3],
    'm': [1, 3, 4],
    'n': [1, 3, 4, 5],
    'o': [1, 3, 5],
    'p': [1, 2, 3, 4],
    'q': [1, 2, 3, 4, 5],
    'r': [1, 2, 3, 5],
    's': [2, 3, 4],
    't': [2, 3, 4, 5],
    'u': [1, 3, 6],
    'v': [1, 2, 3, 6],
    'w': [2, 4, 5, 6],
    'x': [1, 3, 4, 6],
    'y': [1, 3, 4, 5, 6],
    'z': [1, 3, 5, 6],

    # Digits (when preceded by number indicator — standalone use)
    '0': [3, 4, 5, 6],
    '1': [1],          # same as 'a' in number context
    '2': [1, 2],
    '3': [1, 4],
    '4': [1, 4, 5],
    '5': [1, 5],
    '6': [1, 2, 4],
    '7': [1, 2, 4, 5],
    '8': [1, 2, 5],
    '9': [2, 4],

    # Punctuation
    '.': [2, 5, 6],
    ',': [2],
    '!': [2, 3, 5],
    '?': [2, 3, 6],
    "'": [3],
    '-': [3, 6],
    ' ': [],           # space = silence (all rests)

    # Grade 2 contractions — common words as single cells
    # These produce characteristic rhythmic signatures
    'THE':  [2, 3, 4, 6],   # most common English word → distinctive 4-dot cell
    'AND':  [1, 2, 3, 4, 6],
    'FOR':  [1, 2, 3, 4, 5, 6],  # all six dots = full grid hit
    'OF':   [1, 2],
    'WITH': [2, 3, 4, 5, 6],
    'IN':   [3, 5],
    'IS':   [3, 4],
    'IT':   [1, 3, 4, 5],
    'YOU':  [1, 3, 4, 5, 6],
    'NOT':  [1, 3, 4, 5],
    'BE':   [2, 3],
    'HIS':  [2, 3, 6],
    'BY':   [2, 3, 5, 6],
    'AR':   [3, 4, 5],
    'ER':   [1, 2, 4, 5, 6],
    'ST':   [3, 4],
    'CH':   [1, 6],
    'GH':   [1, 2, 6],
    'OW':   [2, 4, 6],
    'WH':   [1, 5, 6],
    'SH':   [1, 4, 6],
    'TH':   [1, 4, 5, 6],
    'ED':   [1, 2, 4, 6],
    'ING':  [3, 4, 6],
    'AR':   [3, 4, 5],
}

# Position → drum voice label + MIDI note
POSITION_MAP = {
    1: ("kick",  36),
    2: ("snare", 38),
    3: ("chat",  42),
    4: ("ohat",  46),
    5: ("clap",  39),
    6: ("rim",   43),
}

VOICE_NAMES = {1: "Kick", 2: "Snare", 3: "ClosedHat", 4: "OpenHat", 5: "Clap", 6: "Rim"}


# =============================================================================
# TEXT → BRAILLE CELLS
# =============================================================================

def tokenize_phrase(phrase: str) -> list[tuple[str, list[int]]]:
    """
    Convert a phrase into a list of (token_label, dot_list) pairs.
    Grade 2 contractions are matched first (longest match wins).
    Single characters fall back to BRAILLE_ALPHA.
    Unrecognised characters use an empty dot pattern (rest).
    """
    phrase_upper = phrase.upper()
    tokens = []
    i = 0
    while i < len(phrase):
        matched = False
        # Try Grade 2 contractions (multi-char keys, uppercase)
        for length in range(5, 1, -1):
            chunk = phrase_upper[i:i + length]
            if chunk in BRAILLE_ALPHA:
                tokens.append((chunk, BRAILLE_ALPHA[chunk]))
                i += length
                matched = True
                break
        if not matched:
            ch = phrase[i].lower()
            dots = BRAILLE_ALPHA.get(ch, [])
            tokens.append((ch if ch != ' ' else 'SPC', dots))
            i += 1
    return tokens


def phrase_to_grid(tokens: list[tuple[str, list[int]]]) -> list[dict]:
    """
    Convert token list to a list of cell dicts:
      { 'label': str, 'dots': [int,...], 'hits': {pos: bool} }
    """
    cells = []
    for label, dots in tokens:
        hits = {p: (p in dots) for p in range(1, 7)}
        cells.append({'label': label, 'dots': dots, 'hits': hits})
    return cells


# =============================================================================
# ASCII GRID VISUALISATION
# =============================================================================

STEP_LABELS = {
    '16th':    '1/16',
    '8th':     '1/8',
    'triplet': '1/8T',
}


def render_grid_txt(phrase: str, cells: list[dict], step_size: str,
                    polyrhythm_phrase: str = None,
                    poly_cells: list[dict] = None) -> str:
    """
    Render an ASCII grid showing the rhythmic pattern for each drum voice.
    Each letter = one column group of 6 steps.
    """
    step_label = STEP_LABELS.get(step_size, step_size)
    lines = []
    lines.append("=" * 72)
    lines.append("XO_OX BRAILLE RHYTHM KIT — ASCII GRID")
    lines.append(f"Phrase  : \"{phrase}\"")
    lines.append(f"Step    : {step_label} per dot position")
    lines.append(f"Cells   : {len(cells)} letters × 6 steps = {len(cells) * 6} total steps")
    lines.append("=" * 72)
    lines.append("")
    lines.append("Braille position layout:")
    lines.append("  [1][4]  Pos 1=Kick  Pos 4=OpenHat")
    lines.append("  [2][5]  Pos 2=Snare Pos 5=Clap")
    lines.append("  [3][6]  Pos 3=CHat  Pos 6=Rim")
    lines.append("")

    def render_phrase_grid(phrase_label: str, grid_cells: list[dict]) -> list[str]:
        out = []
        # Header: letter labels
        header = f"{'':10s}"
        for cell in grid_cells:
            lbl = cell['label'].upper()[:3].center(7)
            header += lbl
        out.append(f"  [{phrase_label}]")
        out.append(header)

        # Separator row: show letter in braille dots
        sep = f"{'LETTER':10s}"
        for cell in grid_cells:
            dot_str = "".join(str(d) for d in sorted(cell['dots'])) if cell['dots'] else "---"
            sep += dot_str.center(7)
        out.append(sep)
        out.append("")

        # Per-voice rows
        for pos in range(1, 7):
            voice_name, midi = POSITION_MAP[pos]
            row_label = f"  Pos{pos} {VOICE_NAMES[pos]:10s}"
            row = row_label
            for cell in grid_cells:
                hit = cell['hits'][pos]
                row += (" X " if hit else " . ").center(7)
            out.append(row)
        return out

    lines += render_phrase_grid(phrase, cells)

    if polyrhythm_phrase and poly_cells:
        lines.append("")
        lines.append("─" * 72)
        lines.append(f"POLYRHYTHM LAYER: \"{polyrhythm_phrase}\"")
        lines.append("")
        lines += render_phrase_grid(polyrhythm_phrase, poly_cells)

    lines.append("")
    lines.append("─" * 72)
    lines.append("Braille → Drum mapping legend:")
    for pos in range(1, 7):
        voice_name, midi = POSITION_MAP[pos]
        lines.append(f"  Position {pos} ({VOICE_NAMES[pos]:10s}) → MIDI note {midi}")
    lines.append("")
    lines.append("Grade 2 contractions used (if any):")
    used = [c['label'].upper() for c in cells if c['label'].upper() in BRAILLE_ALPHA
            and len(c['label']) > 1]
    if used:
        for u in used:
            lines.append(f"  \"{u}\" → dots {BRAILLE_ALPHA[u]}")
    else:
        lines.append("  (none — all single-character Grade 1 cells)")
    lines.append("=" * 72)
    return "\n".join(lines)


# =============================================================================
# VOICE DEFAULTS (shared with drum_export conventions)
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
    "ohat":  {"mono": False, "poly": 2, "one_shot": False, "vtp": 0.0,  "vtf": 0.0,
               "attack": 0.0, "decay": 0.8, "sustain": 0.38, "release": 0.3,
               "cutoff": 0.95, "resonance": 0.0, "mute_group": 1},
    "clap":  {"mono": False, "poly": 2, "one_shot": True,  "vtp": 0.0,  "vtf": 0.15,
               "attack": 0.0, "decay": 0.5, "sustain": 0.0, "release": 0.1,
               "cutoff": 0.95, "resonance": 0.0, "mute_group": 0},
    "rim":   {"mono": False, "poly": 2, "one_shot": True,  "vtp": 0.0,  "vtf": 0.0,
               "attack": 0.0, "decay": 0.35, "sustain": 0.0, "release": 0.08,
               "cutoff": 1.0, "resonance": 0.0, "mute_group": 0},
}

# MIDI note for each voice key (matches POSITION_MAP)
VOICE_MIDI = {
    "kick": 36, "snare": 38, "chat": 42,
    "ohat": 46, "clap": 39,  "rim":  43,
}


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
    blocks = []
    for i in range(1, 5):
        blocks.append(_layer_block(i, 0, 0, "", "", 0.707946))
    return "\n".join(blocks)


def _instrument_block(instrument_num: int, voice_name: str,
                      sample_name: str, sample_file: str,
                      mute_group: int = 0) -> str:
    """One <Instrument> XML block for a single pad."""
    is_active = bool(voice_name and sample_name)
    cfg = VOICE_DEFAULTS.get(voice_name, VOICE_DEFAULTS["rim"])

    if is_active:
        layers_xml = "\n".join([
            _layer_block(1, 1,  31,  sample_name, sample_file, 0.35),
            _layer_block(2, 32, 63,  sample_name, sample_file, 0.55),
            _layer_block(3, 64, 95,  sample_name, sample_file, 0.75),
            _layer_block(4, 96, 127, sample_name, sample_file, 0.95),
        ])
    else:
        layers_xml = _empty_layers()

    mono_str    = "True" if cfg["mono"] else "False"
    oneshot_str = "True" if cfg["one_shot"] else "False"

    mute_xml = "\n".join(
        f"        <MuteTarget{i+1}>0</MuteTarget{i+1}>" for i in range(4)
    )
    # Closed hat mutes open hat
    if voice_name == "chat":
        mute_xml = (
            f"        <MuteTarget1>{VOICE_MIDI['ohat']}</MuteTarget1>\n"
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


def build_pad_assignments(cells: list[dict], slug: str,
                          poly_cells: list[dict] = None) -> list[dict]:
    """
    Assign cells to pads (up to 16).
    In polyrhythm mode: phrase 1 → pads 1-8, phrase 2 → pads 9-16.
    Each pad covers one letter's 6-step pattern.
    Returns list of pad dicts: {pad_num, letter, dots, voice_name, midi_note,
                                 sample_name, sample_file}.
    """
    pads = []
    all_series = [(cells, "A", slug)]
    if poly_cells:
        all_series.append((poly_cells, "B", slug + "_poly"))

    pad_num = 1
    for series_cells, series_tag, series_slug in all_series:
        limit = 8 if poly_cells else 16
        for cell in series_cells[:limit]:
            label = cell['label']
            # One pad per letter: voice selected by most-active column
            # Left column (pos 1-3) majority → kick/snare/chat
            # Right column (pos 4-6) majority → ohat/clap/rim
            left_hits  = sum(1 for p in [1, 2, 3] if cell['hits'][p])
            right_hits = sum(1 for p in [4, 5, 6] if cell['hits'][p])
            total_hits = left_hits + right_hits

            # Primary voice: highest position number that is active
            # (gives the pad a representative identity)
            active_pos = [p for p in range(1, 7) if cell['hits'][p]]
            if not active_pos:
                # Silent cell (space): rest pad — no sample
                pads.append({
                    "pad_num": pad_num,
                    "letter":  label,
                    "dots":    cell['dots'],
                    "voice":   "",
                    "midi":    0,
                    "sample_name": "",
                    "sample_file": "",
                    "active_voices": [],
                    "series": series_tag,
                })
            else:
                # Primary voice = first active position
                primary_pos = active_pos[0]
                v_name, v_midi = POSITION_MAP[primary_pos]
                all_voices = [(POSITION_MAP[p][0], POSITION_MAP[p][1]) for p in active_pos]
                safe_label = label.replace(" ", "_").replace("/", "-")
                s_name = f"{series_slug}_{safe_label}_pad{pad_num}"
                s_file = f"{s_name}.wav"
                pads.append({
                    "pad_num": pad_num,
                    "letter":  label,
                    "dots":    cell['dots'],
                    "voice":   v_name,
                    "midi":    v_midi,
                    "sample_name": s_name,
                    "sample_file": s_file,
                    "active_voices": all_voices,
                    "series": series_tag,
                })
            pad_num += 1
            if pad_num > 16:
                break

    return pads


def generate_xpm(phrase: str, cells: list[dict], slug: str,
                 step_size: str = "16th",
                 poly_phrase: str = None,
                 poly_cells: list[dict] = None) -> str:
    """Generate complete drum program XPM XML string."""
    prog_name = xml_escape(f"XO_OX-BRAILLE-{slug.upper()}")
    pads = build_pad_assignments(cells, slug, poly_cells)

    # Build MIDI note → pad mapping
    # Pads map to their primary voice MIDI note; use sequential notes if conflict
    note_to_pad = {}
    used_notes = set()

    for pad in pads:
        if pad['midi'] and pad['midi'] not in used_notes:
            note_to_pad[pad['midi']] = pad
            used_notes.add(pad['midi'])
        elif pad['midi']:
            # Find the next available note near this voice's base
            fallback = pad['midi']
            while fallback in used_notes and fallback < 127:
                fallback += 1
            note_to_pad[fallback] = pad
            used_notes.add(fallback)
            pad['midi'] = fallback

    # Build 128-slot instrument list
    instrument_parts = []
    for i in range(128):
        if i in note_to_pad:
            p = note_to_pad[i]
            instrument_parts.append(
                _instrument_block(i, p['voice'], p['sample_name'], p['sample_file'],
                                  mute_group=VOICE_DEFAULTS.get(p['voice'], {}).get('mute_group', 0))
            )
        else:
            instrument_parts.append(_instrument_block(i, "", "", "", mute_group=0))

    instruments_xml = "\n".join(instrument_parts)

    # PadNoteMap
    pad_note_entries = []
    for p in pads:
        if p['midi']:
            pad_note_entries.append(
                f'        <Pad number="{p["pad_num"]}" note="{p["midi"]}"/>'
                f'  <!-- {p["letter"]} ({p["voice"]}) dots:{p["dots"]} -->'
            )
    pad_note_xml = "\n".join(pad_note_entries)

    # PadGroupMap (hat choke)
    pad_group_entries = []
    for p in pads:
        mg = VOICE_DEFAULTS.get(p['voice'], {}).get('mute_group', 0)
        if mg > 0:
            pad_group_entries.append(f'        <Pad number="{p["pad_num"]}" group="{mg}"/>')
    pad_group_xml = "\n".join(pad_group_entries)

    import json as _json
    pad_json = _json.dumps(
        {"ProgramPads": {"Universal": {"value0": False},
                         "Type": {"value0": 5},
                         "universalPad": 32512}},
        separators=(",", ":"),
    )

    poly_note = (
        f'\n    <!-- Polyrhythm phrase: "{poly_phrase}" in pads 9-16 -->'
        if poly_phrase else ""
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
        f'    <!-- Braille phrase: "{phrase}" -->{poly_note}\n'
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
        '        <Name>BITE</Name>\n'
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
# MANIFEST JSON
# =============================================================================

def build_manifest(phrase: str, cells: list[dict], slug: str,
                   step_size: str, pads: list[dict],
                   poly_phrase: str = None,
                   poly_cells: list[dict] = None) -> dict:
    step_label = STEP_LABELS.get(step_size, step_size)
    total_steps = len(cells) * 6
    cell_data = []
    for c in cells:
        cell_data.append({
            "letter":       c['label'],
            "dots":         c['dots'],
            "braille_repr": "".join(["⠿"[0]] if c['dots'] else []),
            "hits": {
                f"pos{p}_{VOICE_NAMES[p]}": c['hits'][p]
                for p in range(1, 7)
            }
        })

    manifest = {
        "tool":        "xpn_braille_rhythm_kit",
        "version":     "1.0.0",
        "generated":   str(date.today()),
        "phrase":      phrase,
        "slug":        slug,
        "step_size":   step_label,
        "total_steps": total_steps,
        "letter_count": len(cells),
        "cells":       cell_data,
        "pad_map": [
            {
                "pad_num":       p['pad_num'],
                "letter":        p['letter'],
                "dots":          p['dots'],
                "primary_voice": p['voice'],
                "midi_note":     p['midi'],
                "active_voices": [{"voice": v, "midi": m} for v, m in p.get('active_voices', [])],
                "series":        p['series'],
            }
            for p in pads
        ],
        "position_voice_map": {
            str(pos): {"voice": VOICE_NAMES[pos], "midi": POSITION_MAP[pos][1]}
            for pos in range(1, 7)
        },
        "grade2_contractions_used": [
            c['label'] for c in cells
            if c['label'].upper() in BRAILLE_ALPHA and len(c['label']) > 1
        ],
    }

    if poly_phrase and poly_cells:
        manifest["polyrhythm"] = {
            "phrase": poly_phrase,
            "cells": [
                {"letter": c['label'], "dots": c['dots']}
                for c in poly_cells
            ]
        }

    return manifest


# =============================================================================
# --alphabet DISPLAY
# =============================================================================

def print_alphabet():
    """Print all Braille patterns with ASCII dot grid."""
    print("=" * 60)
    print("XO_OX BRAILLE ALPHABET — Grade 1 + Grade 2 contractions")
    print("Position layout:  [1][4]  [2][5]  [3][6]")
    print("Voice mapping:    Kick/OHat  Snare/Clap  CHat/Rim")
    print("=" * 60)

    def cell_to_ascii(dots: list[int]) -> list[str]:
        rows = []
        for row in range(3):
            left_pos  = row + 1      # positions 1, 2, 3
            right_pos = row + 4      # positions 4, 5, 6
            l = "●" if left_pos  in dots else "○"
            r = "●" if right_pos in dots else "○"
            rows.append(f"  {l} {r}")
        return rows

    # Print letters first
    print("\n--- Letters ---")
    for ch in "abcdefghijklmnopqrstuvwxyz":
        dots = BRAILLE_ALPHA.get(ch, [])
        grid = cell_to_ascii(dots)
        print(f"\n  '{ch.upper()}' dots={dots}")
        for row in grid:
            print(row)

    print("\n--- Grade 2 Contractions ---")
    grade2_keys = [k for k in BRAILLE_ALPHA if len(k) > 1]
    for key in sorted(grade2_keys):
        dots = BRAILLE_ALPHA[key]
        grid = cell_to_ascii(dots)
        voices = [VOICE_NAMES[p] for p in dots]
        print(f"\n  \"{key}\" dots={dots}  voices={voices}")
        for row in grid:
            print(row)

    print("\n--- Punctuation ---")
    for ch in '.,!?\'-':
        dots = BRAILLE_ALPHA.get(ch, [])
        grid = cell_to_ascii(dots)
        print(f"\n  '{ch}' dots={dots}")
        for row in grid:
            print(row)

    print("\n" + "=" * 60)


# =============================================================================
# MAIN
# =============================================================================

def slugify(phrase: str) -> str:
    import re
    s = phrase.lower().strip()
    s = re.sub(r"[^a-z0-9]+", "_", s)
    s = s.strip("_")
    return s[:40] or "braille"


def main():
    parser = argparse.ArgumentParser(
        description="XPN Braille Rhythm Kit Generator — XO_OX / Hex+Rex",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("phrase", nargs="?", default=None,
                        help="Word or phrase to encode as Braille rhythm (quote multi-word phrases)")
    parser.add_argument("--output", "-o", default="./kits",
                        help="Output directory (default: ./kits)")
    parser.add_argument("--step-size", choices=["16th", "8th", "triplet"], default="16th",
                        help="Rhythmic grid step size per dot (default: 16th)")
    parser.add_argument("--polyrhythm", metavar="PHRASE2",
                        help="Second phrase for polyrhythm mode (stacked in pads 9-16)")
    parser.add_argument("--alphabet", action="store_true",
                        help="Print the full Braille alphabet and exit")
    args = parser.parse_args()

    if args.alphabet:
        print_alphabet()
        sys.exit(0)

    if not args.phrase:
        parser.error("A phrase is required (or use --alphabet to see patterns).")

    phrase = args.phrase
    slug = slugify(phrase)
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    print(f"[Hex+Rex] Encoding phrase: \"{phrase}\"")
    tokens = tokenize_phrase(phrase)
    cells  = phrase_to_grid(tokens)
    print(f"          {len(cells)} braille cells  × 6 steps = {len(cells) * 6} total steps")

    poly_phrase = args.polyrhythm
    poly_cells  = None
    if poly_phrase:
        poly_tokens = tokenize_phrase(poly_phrase)
        poly_cells  = phrase_to_grid(poly_tokens)
        print(f"[Hex+Rex] Polyrhythm phrase: \"{poly_phrase}\"")
        print(f"          {len(poly_cells)} braille cells  × 6 steps = {len(poly_cells) * 6} total steps")

    # Build pad list (needed by both XPM and manifest)
    pads = build_pad_assignments(cells, slug, poly_cells)

    # 1. XPM
    xpm_path = output_dir / f"braille_{slug}_kit.xpm"
    xpm_xml  = generate_xpm(phrase, cells, slug, args.step_size, poly_phrase, poly_cells)
    xpm_path.write_text(xpm_xml, encoding="utf-8")
    print(f"[Hex+Rex] Wrote XPM    → {xpm_path}")

    # 2. Manifest JSON
    manifest = build_manifest(phrase, cells, slug, args.step_size, pads,
                               poly_phrase, poly_cells)
    manifest_path = output_dir / f"braille_{slug}_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"[Hex+Rex] Wrote manifest → {manifest_path}")

    # 3. ASCII grid
    grid_txt  = render_grid_txt(phrase, cells, args.step_size, poly_phrase, poly_cells)
    grid_path = output_dir / f"braille_{slug}_grid.txt"
    grid_path.write_text(grid_txt, encoding="utf-8")
    print(f"[Hex+Rex] Wrote grid   → {grid_path}")

    # Print summary to terminal
    print()
    print(grid_txt)

    # Active pads summary
    print(f"\n[Hex+Rex] Pad summary ({len(pads)} pads):")
    for p in pads:
        tag = f"[{p['series']}]"
        if p['voice']:
            av = ", ".join(v for v, _ in p.get('active_voices', []))
            print(f"  Pad {p['pad_num']:2d} {tag} '{p['letter']}' dots={p['dots']!s:20s} → {av}")
        else:
            print(f"  Pad {p['pad_num']:2d} {tag} '{p['letter']}' (rest — no hits)")

    print(f"\n[Hex+Rex] Done. Output in {output_dir}/")
    print(f"          Load braille_{slug}_kit.xpm into your MPC.")
    print(f"          Each pad spells one letter's rhythmic cell.")


if __name__ == "__main__":
    main()
