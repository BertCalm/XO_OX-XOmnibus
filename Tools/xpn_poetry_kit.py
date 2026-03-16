#!/usr/bin/env python3
"""
XPN Poetry Kit — XO_OX Designs
CMU Pronouncing Dictionary stress patterns → drum rhythms.

Each syllable = one 16th-note step. Stress level determines voice and velocity:
  1 (PRIMARY stress)   → Kick or Snare (alternating), vel 100-127
  2 (SECONDARY stress) → Closed Hat or Clap, vel 60-80
  0 (UNSTRESSED)       → Ghost note (any voice), vel 1-30

3 hardcoded poem fragments:
  shakespeare  "Shall I compare thee to a summer's day" (Sonnet 18)
  dickinson    "Because I could not stop for Death" (poem 712)
  ginsberg     "I saw the best minds of my generation" (Howl, Part I)

Custom text: --text "your phrase here"
  Words not in the hardcoded CMU data fall back to a simple stress heuristic.

Output (XPN ZIP + scansion report):
  poetry_kit_{poem}.xpn       — MPC drum program (XPN ZIP)
  scansion_report.txt          — stress pattern with DUM/dum/Dum notation

CLI:
  python xpn_poetry_kit.py --poem shakespeare --output ./out/
  python xpn_poetry_kit.py --poem dickinson   --output ./out/
  python xpn_poetry_kit.py --poem ginsberg    --output ./out/
  python xpn_poetry_kit.py --text "Because I could not stop" --output ./out/

XPN golden rules (always applied):
  KeyTrack  = True
  RootNote  = 0
  VelStart  = 0 on empty layers
"""

import argparse
import json
import re
import struct
import sys
import zipfile
from pathlib import Path
from xml.sax.saxutils import escape as xml_escape

# =============================================================================
# CMU PRONOUNCING DICTIONARY — HARDCODED SUBSET (50 common words)
# Format: word -> list of syllable stress values (0=unstressed, 1=primary, 2=secondary)
# Stress codes follow CMU ARPAbet convention.
# =============================================================================
CMU_STRESS = {
    # Pronouns / articles / prepositions
    "i":        [1],
    "you":      [1],
    "he":       [1],
    "she":      [1],
    "we":       [1],
    "they":     [1],
    "me":       [1],
    "him":      [1],
    "her":      [1],
    "it":       [1],
    "a":        [0],
    "an":       [0],
    "the":      [0],
    "to":       [0],
    "of":       [0],
    "in":       [0],
    "for":      [0],
    "not":      [1],
    "with":     [0],
    "by":       [0],
    "at":       [0],
    "on":       [0],
    "and":      [0],
    "or":       [0],
    "but":      [0],
    "so":       [0],
    # Common verbs
    "shall":    [1],
    "could":    [1],
    "stop":     [1],
    "saw":      [1],
    "compare":  [0, 1],
    "see":      [1],
    # Nouns / adjectives
    "summer":   [1, 0],
    "day":      [1],
    "death":    [1],
    "mind":     [1],
    "minds":    [1],
    "best":     [1],
    "thee":     [1],
    "thy":      [1],
    "my":       [1],
    "own":      [1],
    "love":     [1],
    "life":     [1],
    "time":     [1],
    "night":    [1],
    "light":    [1],
    "dark":     [1],
    "world":    [1],
    "heart":    [1],
    "soul":     [1],
    "sweet":    [1],
    "deep":     [1],
    "long":     [1],
    "old":      [1],
    "new":      [1],
    "great":    [1],
    "last":     [1],
    "first":    [1],
    "own":      [1],
    "true":     [1],
    "far":      [1],
    "sun":      [1],
    "sky":      [1],
    "eye":      [1],
    "eyes":     [1],
    "hand":     [1],
    "hands":    [1],
    "dead":     [1],
    "free":     [1],
    "man":      [1],
    "men":      [1],
    "god":      [1],
    "earth":    [1],
    "stone":    [1],
    "fire":     [1, 0],
    "water":    [1, 0],
    "because":  [0, 1],
    "generation": [2, 0, 1, 0],
    "immortal":   [0, 1, 0],
    "beautiful":  [1, 0, 0],
    "remember":   [0, 1, 0],
    "together":   [0, 1, 0],
    "another":    [0, 1, 0],
    "forever":    [0, 1, 0],
    "whatever":   [0, 1, 0],
    "however":    [0, 1, 0],
    "beneath":    [0, 1],
    "beyond":     [0, 1],
    "between":    [0, 1],
    "before":     [0, 1],
    "behind":     [0, 1],
    "beside":     [0, 1],
    "within":     [0, 1],
    "without":    [0, 1],
    "compare":    [0, 1],
    "desire":     [0, 1],
    "divine":     [0, 1],
    "alive":      [0, 1],
    "alone":      [0, 1],
    "along":      [0, 1],
    "among":      [0, 1],
    "above":      [0, 1],
    "across":     [0, 1],
    "around":     [0, 1],
    "below":      [0, 1],
    # Poem-specific
    "starving":   [1, 0],
    "hysterical": [0, 1, 0, 0],
    "naked":      [1, 0],
    "dragging":   [1, 0],
    "through":    [0],
    "negro":      [1, 0],
    "streets":    [1],
    "dawn":       [1],
    "looking":    [1, 0],
    "angry":      [1, 0],
    "fix":        [1],
    "machinery":  [0, 1, 0, 0],
}

# =============================================================================
# HARDCODED POEM FRAGMENTS
# =============================================================================
POEMS = {
    "shakespeare": {
        "title": 'Shakespeare Sonnet 18: "Shall I compare thee to a summer\'s day"',
        "author": "William Shakespeare (1609)",
        "text": "Shall I compare thee to a summer's day",
        "notes": "Iambic pentameter — unstressed/stressed pairs throughout.",
    },
    "dickinson": {
        "title": 'Dickinson: "Because I could not stop for Death"',
        "author": "Emily Dickinson (c. 1863)",
        "text": "Because I could not stop for Death",
        "notes": "Common meter (8-6-8-6). Alternating iambic tetrameter/trimeter.",
    },
    "ginsberg": {
        "title": 'Ginsberg: "I saw the best minds of my generation"',
        "author": "Allen Ginsberg, Howl Part I (1956)",
        "text": "I saw the best minds of my generation",
        "notes": "Long-breath line. Variable stress creates polyrhythmic feel.",
    },
}

# =============================================================================
# PAD / VOICE DEFINITIONS
# =============================================================================

# 16 pads, GM-convention MIDI notes
PAD_MIDI_NOTES = [36, 38, 42, 46, 39, 41, 43, 49,
                  50, 52, 54, 56, 58, 60, 62, 64]

# Voice names for display
VOICE_DISPLAY = {
    "kick":  "Kick",
    "snare": "Snare",
    "chat":  "Closed Hat",
    "clap":  "Clap",
    "ghost": "Ghost",
}

# Pad index for each voice
VOICE_PAD = {
    "kick":  0,   # MIDI 36
    "snare": 1,   # MIDI 38
    "chat":  2,   # MIDI 42
    "clap":  4,   # MIDI 39
    "ghost": 5,   # MIDI 41 (low velocity ghost)
}


# =============================================================================
# TEXT / STRESS PARSING
# =============================================================================

def clean_word(w: str) -> str:
    """Lowercase and strip punctuation."""
    return re.sub(r"[^a-z']", "", w.lower()).strip("'")


def count_vowel_groups(word: str) -> int:
    """Rough syllable count by vowel groups."""
    groups = re.findall(r"[aeiouy]+", word.lower())
    return max(1, len(groups))


def heuristic_stress(word: str):
    """
    Simple heuristic stress for words not in CMU_STRESS.
    Mono-syllabic content words → [1]
    Multi-syllabic → first syllable stressed, rest unstressed.
    Very common function words → all [0].
    """
    function_words = {
        "a", "an", "the", "to", "of", "in", "for", "on", "at",
        "by", "as", "is", "was", "are", "were", "be", "been",
        "and", "or", "but", "nor", "so", "yet", "if", "then",
        "that", "this", "these", "those", "with", "from",
    }
    if word in function_words:
        return [0]
    n = count_vowel_groups(word)
    if n == 1:
        return [1]
    # first syllable primary, rest unstressed
    return [1] + [0] * (n - 1)


def get_stress_pattern(word: str):
    """Return stress list for a word, using CMU data or heuristic fallback."""
    w = clean_word(word)
    if not w:
        return []
    if w in CMU_STRESS:
        return list(CMU_STRESS[w])
    return heuristic_stress(w)


def parse_phrase(text: str):
    """
    Parse a phrase into (word, syllable_index, stress_value) tuples.
    Returns list of dicts per syllable.
    """
    words = text.split()
    syllables = []
    step = 0
    for word in words:
        clean = clean_word(word)
        if not clean:
            continue
        pattern = get_stress_pattern(clean)
        for syl_i, stress in enumerate(pattern):
            syllables.append({
                "word":      clean,
                "word_raw":  word,
                "syl_index": syl_i,
                "stress":    stress,
                "step":      step,
            })
            step += 1
    return syllables


# =============================================================================
# STRESS → DRUM HIT MAPPING
# =============================================================================

def assign_hits(syllables):
    """
    Map each syllable to a drum hit.
    Primary stress (1): alternates Kick / Snare, vel 100-127
    Secondary stress (2): alternates Closed Hat / Clap, vel 60-80
    Unstressed (0): Ghost note, vel 1-30

    Velocity within range scales with position (adds slight groove feel).
    """
    primary_count = 0
    secondary_count = 0
    hits = []

    for syl in syllables:
        s = syl["stress"]
        step = syl["step"]
        pos_frac = (step % 16) / 15.0  # 0.0–1.0 within 16-step grid

        if s == 1:
            # Primary stress
            if primary_count % 2 == 0:
                voice = "kick"
            else:
                voice = "snare"
            # Velocity: 100-127, slight swell by position
            vel = int(100 + pos_frac * 27)
            vel = min(vel, 127)
            primary_count += 1

        elif s == 2:
            # Secondary stress
            if secondary_count % 2 == 0:
                voice = "chat"
            else:
                voice = "clap"
            # Velocity: 60-80
            vel = int(60 + pos_frac * 20)
            vel = min(vel, 80)
            secondary_count += 1

        else:
            # Unstressed — ghost note
            voice = "ghost"
            # Velocity: 1-30, even softer toward end of phrase
            vel = max(1, int(30 - pos_frac * 20))

        pad_index = VOICE_PAD[voice]
        midi_note = PAD_MIDI_NOTES[pad_index]

        hits.append({
            **syl,
            "voice":     voice,
            "velocity":  vel,
            "pad_index": pad_index + 1,
            "midi_note": midi_note,
        })

    return hits


# =============================================================================
# XPM GENERATION
# =============================================================================

def _silent_wav() -> bytes:
    """Minimal valid 44100Hz mono 16-bit WAV, ~0.1s silence (placeholder)."""
    sample_rate = 44100
    num_samples = 4410
    num_channels = 1
    bits = 16
    byte_rate = sample_rate * num_channels * bits // 8
    block_align = num_channels * bits // 8
    data_size = num_samples * block_align
    header = struct.pack(
        "<4sI4s4sIHHIIHH4sI",
        b"RIFF", 36 + data_size, b"WAVE", b"fmt ",
        16, 1, num_channels, sample_rate, byte_rate,
        block_align, bits, b"data", data_size,
    )
    return header + b"\x00" * data_size


def _layer_block(number: int, vel_start: int, vel_end: int,
                 sample_name: str, volume: float) -> str:
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


VOICE_SAMPLE_MAP = {
    # voice: (low_sample, mid_sample, hi_sample)
    "kick":  ("poetry_kick_soft.wav",  "poetry_kick_mid.wav",  "poetry_kick_hard.wav"),
    "snare": ("poetry_snare_soft.wav", "poetry_snare_mid.wav", "poetry_snare_hard.wav"),
    "chat":  ("poetry_chat_soft.wav",  "poetry_chat_mid.wav",  "poetry_chat_hard.wav"),
    "clap":  ("poetry_clap_soft.wav",  "poetry_clap_mid.wav",  "poetry_clap_hard.wav"),
    "ghost": ("poetry_ghost_soft.wav", "poetry_ghost_mid.wav", "poetry_ghost_hard.wav"),
}

VOICE_SETTINGS = {
    # voice: (vel_to_pitch, vel_to_filter, mute_group, one_shot, polyphony, decay, release)
    "kick":  ("0.050000", "0.100000", "0", "True",  "1", "0.350000", "0.100000"),
    "snare": ("0.000000", "0.300000", "0", "True",  "1", "0.280000", "0.090000"),
    "chat":  ("0.000000", "0.050000", "1", "True",  "1", "0.150000", "0.060000"),
    "clap":  ("0.000000", "0.100000", "0", "True",  "2", "0.200000", "0.080000"),
    "ghost": ("0.000000", "0.000000", "0", "True",  "1", "0.150000", "0.050000"),
}


def _active_instrument(instr_num: int, midi_note: int, voice: str) -> str:
    samples = VOICE_SAMPLE_MAP[voice]
    settings = VOICE_SETTINGS[voice]
    vtp, vtf, mg, os, poly, decay, release = settings

    # 3 velocity layers (soft/mid/hard) + 1 empty
    layer1 = _layer_block(1,   1,  42, samples[0], 0.600000)
    layer2 = _layer_block(2,  43,  84, samples[1], 0.800000)
    layer3 = _layer_block(3,  85, 127, samples[2], 1.000000)
    layer4 = _empty_layer(4)
    layers_xml = "\n".join([layer1, layer2, layer3, layer4])

    return (
        f'        <Instrument number="{instr_num}">\n'
        f'          <Active>True</Active>\n'
        f'          <Volume>1.000000</Volume>\n'
        f'          <Pan>0.500000</Pan>\n'
        f'          <Pitch>0.000000</Pitch>\n'
        f'          <MidiNote>{midi_note}</MidiNote>\n'
        f'          <ZonePlay>1</ZonePlay>\n'
        f'          <MuteGroup>{mg}</MuteGroup>\n'
        f'          <MuteTarget>0</MuteTarget>\n'
        f'          <VoiceOverlap>0</VoiceOverlap>\n'
        f'          <Mono>True</Mono>\n'
        f'          <Polyphony>{poly}</Polyphony>\n'
        f'          <OneShot>{os}</OneShot>\n'
        f'          <VelocitySensitivity>1.000000</VelocitySensitivity>\n'
        f'          <VelocityToPitch>{vtp}</VelocityToPitch>\n'
        f'          <VelocityToFilter>{vtf}</VelocityToFilter>\n'
        f'          <ProgramName>{xml_escape(VOICE_DISPLAY[voice])}</ProgramName>\n'
        f'          <Attack>0.000000</Attack>\n'
        f'          <Hold>0.000000</Hold>\n'
        f'          <Decay>{decay}</Decay>\n'
        f'          <Sustain>0.000000</Sustain>\n'
        f'          <Release>{release}</Release>\n'
        f'          <FilterType>2</FilterType>\n'
        f'          <FilterCutoff>0.900000</FilterCutoff>\n'
        f'          <FilterResonance>0.050000</FilterResonance>\n'
        f'          <FilterEnvAmt>0.060000</FilterEnvAmt>\n'
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


def build_xpm(poem_title: str, slug: str, poem_text: str, poet_notes: str) -> str:
    from datetime import date
    today = date.today().isoformat()

    instruments_xml_parts = []
    instr_num = 1

    # Active pads: kick, snare, chat, ghost=pad5, clap
    # Pad 1=kick(36), 2=snare(38), 3=chat(42), 4=ohat(46), 5=clap(39),
    # 6=ghost(41), 7-16 inactive
    active_voices = [
        (0, "kick"),
        (1, "snare"),
        (2, "chat"),
        (4, "clap"),
        (5, "ghost"),
    ]
    active_set = {pad_i for pad_i, _ in active_voices}

    for pad_i, voice in active_voices:
        instr_num_here = pad_i + 1
        midi_note = PAD_MIDI_NOTES[pad_i]
        instruments_xml_parts.append(
            (pad_i, _active_instrument(instr_num_here, midi_note, voice))
        )

    # Build ordered list
    ordered_parts = []
    active_by_pad = dict(active_voices)
    for pad_i in range(16):
        midi_note = PAD_MIDI_NOTES[pad_i]
        if pad_i in active_set:
            ordered_parts.append(
                _active_instrument(pad_i + 1, midi_note, active_by_pad[pad_i])
            )
        else:
            ordered_parts.append(_inactive_instrument(pad_i + 1, midi_note))

    # Fill remaining 112
    next_note = 67
    for num in range(17, 129):
        ordered_parts.append(_inactive_instrument(num, min(next_note, 127)))
        next_note += 1

    instruments_xml = "\n".join(ordered_parts)

    return f"""<?xml version="1.0" encoding="UTF-8"?>
<MPCVObject>
  <Version>
    <File_Version>2.1</File_Version>
    <Application>MPC</Application>
    <Application_Version>2.10</Application_Version>
  </Version>
  <Program type="Drum">
    <Name>Poetry Kit — {xml_escape(slug)}</Name>
    <Slug>{xml_escape(slug)}</Slug>
    <DateCreated>{today}</DateCreated>
    <Description>{xml_escape(poem_title)} Stress mapping: primary (DUM) = Kick/Snare vel 100-127; secondary (Dum) = Hat/Clap vel 60-80; unstressed (dum) = Ghost vel 1-30. Each syllable = one 16th-note step. {xml_escape(poet_notes)}</Description>
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


# =============================================================================
# SCANSION REPORT
# =============================================================================

STRESS_SYMBOL = {1: "DUM", 2: "Dum", 0: "dum"}
STRESS_NAME   = {1: "primary",   2: "secondary", 0: "unstressed"}


def build_scansion_report(hits, poem_title: str, poem_text: str,
                          poet_notes: str, slug: str) -> str:
    from datetime import date

    lines = [
        "=" * 72,
        "  XPN POETRY KIT — SCANSION REPORT",
        f"  Poem:      {poem_title}",
        f"  Kit slug:  {slug}",
        f"  Text:      {poem_text}",
        f"  Notes:     {poet_notes}",
        f"  Generated: {date.today().isoformat()}",
        "=" * 72,
        "",
        "STRESS LEGEND",
        "  DUM  = primary stress (1)   → Kick / Snare   vel 100-127",
        "  Dum  = secondary stress (2) → Hat / Clap     vel  60-80",
        "  dum  = unstressed (0)        → Ghost           vel   1-30",
        "",
        "SCANSION LINE",
        "",
    ]

    # Build visual scansion
    current_word = None
    word_groups = []
    current_group = []
    for hit in hits:
        if hit["word"] != current_word:
            if current_group:
                word_groups.append((current_word_raw, current_group))
            current_group = []
            current_word = hit["word"]
            current_word_raw = hit["word_raw"]
        current_group.append(hit)
    if current_group:
        word_groups.append((current_word_raw, current_group))

    scansion_parts = []
    for word_raw, group in word_groups:
        symbols = "-".join(STRESS_SYMBOL[h["stress"]] for h in group)
        scansion_parts.append(f"{word_raw}({symbols})")

    lines.append("  " + "  ".join(scansion_parts))
    lines.append("")

    # Classic scansion marks
    classic_parts = []
    for word_raw, group in word_groups:
        marks = ""
        for h in group:
            if h["stress"] == 1:
                marks += "/"
            elif h["stress"] == 2:
                marks += "\\"
            else:
                marks += "u"
        classic_parts.append(f"{word_raw}[{marks}]")

    lines.append("CLASSIC NOTATION  (/ = primary stress, \\ = secondary, u = unstressed)")
    lines.append("  " + "  ".join(classic_parts))
    lines.append("")

    # Step table
    lines.append(f"STEP-BY-STEP HIT TABLE")
    lines.append(
        f"  {'Step':>4}  {'Word':<14}  {'Syl':>3}  {'Stress':<10}  "
        f"{'Symbol':>5}  {'Voice':<12}  {'Vel':>3}  {'Pad':>3}  {'MIDI':>4}"
    )
    lines.append("  " + "-" * 68)
    for hit in hits:
        lines.append(
            f"  {hit['step']+1:>4}  {hit['word_raw']:<14}  "
            f"{hit['syl_index']+1:>3}  {STRESS_NAME[hit['stress']]:<10}  "
            f"{STRESS_SYMBOL[hit['stress']]:>5}  "
            f"{VOICE_DISPLAY[hit['voice']]:<12}  "
            f"{hit['velocity']:>3}  P{hit['pad_index']:>2}  {hit['midi_note']:>4}"
        )

    # Metrics summary
    primary_hits  = [h for h in hits if h["stress"] == 1]
    secondary_hits = [h for h in hits if h["stress"] == 2]
    ghost_hits    = [h for h in hits if h["stress"] == 0]
    total_steps   = len(hits)

    lines += [
        "",
        "METRICS SUMMARY",
        f"  Total syllables / steps: {total_steps}",
        f"  Primary stress (DUM):    {len(primary_hits)} "
        f"({100*len(primary_hits)//max(total_steps,1)}%)",
        f"  Secondary stress (Dum):  {len(secondary_hits)} "
        f"({100*len(secondary_hits)//max(total_steps,1)}%)",
        f"  Unstressed (dum):        {len(ghost_hits)} "
        f"({100*len(ghost_hits)//max(total_steps,1)}%)",
        "",
        "PAD LAYOUT",
        "  Pad  1  (MIDI 36)  Kick       — primary stress, beats 1/3",
        "  Pad  2  (MIDI 38)  Snare      — primary stress, beats 2/4",
        "  Pad  3  (MIDI 42)  Closed Hat — secondary stress",
        "  Pad  5  (MIDI 39)  Clap       — secondary stress (alternating with hat)",
        "  Pad  6  (MIDI 41)  Ghost      — unstressed syllables",
        "",
        "SOUND DESIGN NOTES",
        "  -- Ghost hits should be very quiet (vel 1-30) — texture, not rhythm",
        "  -- Primary stress pads benefit from open filter at high velocity (D001)",
        "  -- Try tuning the ghost pad down a minor 3rd for a poetic undertone",
        "  -- Vary BPM to match the natural speech rate of the poem",
        "  -- Stack with a melody loop in the engine of your choice for full effect",
    ]

    return "\n".join(lines)


# =============================================================================
# ZIP BUILDER
# =============================================================================

def write_zip(slug: str, xpm_xml: str, report_txt: str,
              manifest: dict, output_dir: Path) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    zip_path = output_dir / f"{slug}.xpn"
    wav_bytes = _silent_wav()

    all_samples = set()
    for samples in VOICE_SAMPLE_MAP.values():
        all_samples.update(samples)

    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
        zf.writestr(f"Programs/{slug}.xpm", xpm_xml)
        for fname in all_samples:
            zf.writestr(f"Samples/{fname}", wav_bytes)
        zf.writestr("scansion_manifest.json", json.dumps(manifest, indent=2))
        zf.writestr("scansion_report.txt", report_txt)

    return zip_path


# =============================================================================
# CLI
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Poetry Kit — CMU stress patterns to MPC drum rhythms",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Poems:
  shakespeare  Sonnet 18: "Shall I compare thee to a summer's day"
  dickinson    "Because I could not stop for Death"
  ginsberg     "I saw the best minds of my generation" (Howl)

Examples:
  python xpn_poetry_kit.py --poem shakespeare --output ./out/
  python xpn_poetry_kit.py --poem dickinson --output ./out/
  python xpn_poetry_kit.py --poem ginsberg --output ./out/
  python xpn_poetry_kit.py --text "Because I could not stop" --output ./out/
  python xpn_poetry_kit.py --list
        """
    )
    parser.add_argument("--poem", metavar="NAME",
                        help="Hardcoded poem: shakespeare, dickinson, ginsberg")
    parser.add_argument("--text", metavar="TEXT",
                        help="Custom phrase to scan")
    parser.add_argument("--output", "-o", metavar="DIR",
                        help="Output directory")
    parser.add_argument("--list", action="store_true",
                        help="List available poems and exit")
    args = parser.parse_args()

    if args.list:
        print("Available hardcoded poems:")
        print(f"  {'Key':<14}  Title")
        print("  " + "-" * 60)
        for key, poem in POEMS.items():
            print(f"  {key:<14}  {poem['title']}")
        return

    if not args.output:
        parser.print_help()
        sys.exit(1)

    if args.poem and args.text:
        print("ERROR: specify --poem OR --text, not both", file=sys.stderr)
        sys.exit(1)

    if args.poem:
        poem_key = args.poem.lower()
        if poem_key not in POEMS:
            valid = ", ".join(POEMS.keys())
            print(f"ERROR: unknown poem '{poem_key}'. Valid: {valid}", file=sys.stderr)
            sys.exit(1)
        poem = POEMS[poem_key]
        poem_text  = poem["text"]
        poem_title = poem["title"]
        poet_notes = poem["notes"]
        slug = f"poetry_{poem_key}"
    elif args.text:
        poem_text  = args.text
        poem_title = f'Custom: "{poem_text[:40]}"'
        poet_notes = "Custom text — stress via heuristic fallback for unknown words."
        poem_key   = "custom"
        slug       = "poetry_custom"
    else:
        print("ERROR: specify --poem or --text", file=sys.stderr)
        parser.print_help()
        sys.exit(1)

    output_dir = Path(args.output)

    print(f"Building Poetry Kit: {poem_title}")
    print(f"  Text: {poem_text}")

    # Parse
    syllables = parse_phrase(poem_text)
    hits = assign_hits(syllables)

    print(f"  Syllables: {len(syllables)}")

    # Print quick scansion
    symbols = [STRESS_SYMBOL[h["stress"]] for h in hits]
    print(f"  Scansion:  {' '.join(symbols)}")

    primary   = sum(1 for h in hits if h["stress"] == 1)
    secondary = sum(1 for h in hits if h["stress"] == 2)
    ghost     = sum(1 for h in hits if h["stress"] == 0)
    print(f"  DUM(primary)={primary}  Dum(secondary)={secondary}  dum(ghost)={ghost}")

    # Build XPM
    xpm_xml = build_xpm(poem_title, slug, poem_text, poet_notes)

    # Build report
    report_txt = build_scansion_report(hits, poem_title, poem_text, poet_notes, slug)

    # Build manifest
    manifest = {
        "tool":    "xpn_poetry_kit",
        "version": "1.0.0",
        "date":    __import__("datetime").date.today().isoformat(),
        "poem_key":   poem_key,
        "poem_title": poem_title,
        "text":       poem_text,
        "notes":      poet_notes,
        "total_syllables": len(hits),
        "stress_counts": {
            "primary":   primary,
            "secondary": secondary,
            "unstressed": ghost,
        },
        "stress_mapping": {
            "primary_1":    "Kick / Snare (alternating), vel 100-127",
            "secondary_2":  "Closed Hat / Clap (alternating), vel 60-80",
            "unstressed_0": "Ghost, vel 1-30",
        },
        "pad_map": {
            str(k): VOICE_DISPLAY[v] for k, v in {
                1: "kick", 2: "snare", 3: "chat", 5: "clap", 6: "ghost"
            }.items()
        },
        "hits": [
            {k: v for k, v in h.items()}
            for h in hits
        ],
    }

    # Write ZIP
    zip_path = write_zip(slug, xpm_xml, report_txt, manifest, output_dir)
    print(f"  Written: {zip_path}")

    # Standalone report
    report_path = output_dir / "scansion_report.txt"
    report_path.write_text(report_txt, encoding="utf-8")
    print(f"  Written: {report_path}")

    print()
    print("Sample files needed (replace placeholder WAVs inside the ZIP):")
    for voice, samples in VOICE_SAMPLE_MAP.items():
        print(f"  {VOICE_DISPLAY[voice]:<14} — {', '.join(samples)}")


if __name__ == "__main__":
    main()
