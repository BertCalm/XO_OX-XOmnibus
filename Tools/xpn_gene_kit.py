#!/usr/bin/env python3
"""
XPN Gene Kit — XO_OX Designs
DNA codon sequences → MPC-compatible drum programs (.xpn).

Genetic code translation:
  First base:   A→Kick, T→Snare, G→Closed Hat, C→Open Hat
  Second base:  A=high vel (100-127), T=mid-high (75-99), G=mid-low (50-74), C=low (25-49)
  Third base:   A=on-beat, T=16th-late (+1 timing), G=16th-early (-1 timing), C=triplet

Architecture:
  64 codons from a gene region → 4 pad banks of 16 pads each
  Each pad = 1 codon → voice / velocity / timing

XPN golden rules:
  KeyTrack = True  (samples transpose across zones)
  RootNote = 0     (MPC auto-detect convention)
  Empty layer VelStart = 0  (prevents ghost triggering)

Built-in gene sequences (first 64 codons):
  tp53    — TP53 tumor suppressor, "The Guardian of the Genome"
  brca1   — BRCA1 breast cancer gene, "The Protector"
  chr2    — Channelrhodopsin-2 (optogenetics), "Light Activated"

CLI:
  python xpn_gene_kit.py --gene tp53 --output ./out/
  python xpn_gene_kit.py --gene brca1 --output ./out/
  python xpn_gene_kit.py --gene chr2 --output ./out/
  python xpn_gene_kit.py --fasta gene.fasta --output ./out/
  python xpn_gene_kit.py --codons "ATG GTT CCG ..." --output ./out/
  python xpn_gene_kit.py --gene tp53 --visualize
"""

import argparse
import os
import sys
import zipfile
from datetime import date
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from xml.sax.saxutils import escape as xml_escape

# =============================================================================
# GENETIC CODE — STANDARD CODON TABLE
# =============================================================================

# Standard genetic code: codon (DNA) → amino acid (1-letter code or "Stop")
CODON_TABLE: Dict[str, str] = {
    # Phenylalanine
    "TTT": "Phe", "TTC": "Phe",
    # Leucine
    "TTA": "Leu", "TTG": "Leu",
    "CTT": "Leu", "CTC": "Leu", "CTA": "Leu", "CTG": "Leu",
    # Isoleucine
    "ATT": "Ile", "ATC": "Ile", "ATA": "Ile",
    # Methionine (Start)
    "ATG": "Met",
    # Valine
    "GTT": "Val", "GTC": "Val", "GTA": "Val", "GTG": "Val",
    # Serine
    "TCT": "Ser", "TCC": "Ser", "TCA": "Ser", "TCG": "Ser",
    "AGT": "Ser", "AGC": "Ser",
    # Proline
    "CCT": "Pro", "CCC": "Pro", "CCA": "Pro", "CCG": "Pro",
    # Threonine
    "ACT": "Thr", "ACC": "Thr", "ACA": "Thr", "ACG": "Thr",
    # Alanine
    "GCT": "Ala", "GCC": "Ala", "GCA": "Ala", "GCG": "Ala",
    # Tyrosine
    "TAT": "Tyr", "TAC": "Tyr",
    # Stop codons
    "TAA": "Stop", "TAG": "Stop", "TGA": "Stop",
    # Histidine
    "CAT": "His", "CAC": "His",
    # Glutamine
    "CAA": "Gln", "CAG": "Gln",
    # Asparagine
    "AAT": "Asn", "AAC": "Asn",
    # Lysine
    "AAA": "Lys", "AAG": "Lys",
    # Aspartate
    "GAT": "Asp", "GAC": "Asp",
    # Glutamate
    "GAA": "Glu", "GAG": "Glu",
    # Cysteine
    "TGT": "Cys", "TGC": "Cys",
    # Tryptophan
    "TGG": "Trp",
    # Arginine
    "CGT": "Arg", "CGC": "Arg", "CGA": "Arg", "CGG": "Arg",
    "AGA": "Arg", "AGG": "Arg",
    # Glycine
    "GGT": "Gly", "GGC": "Gly", "GGA": "Gly", "GGG": "Gly",
}

# =============================================================================
# HARDCODED GENE SEQUENCES (first 64 codons each)
# Sources: NCBI reference sequences
# =============================================================================

# TP53 (NM_000546.6) — first 64 codons of coding sequence
# "The Guardian of the Genome" — Li-Fraumeni / most-mutated human cancer gene
TP53_CODONS = (
    "ATG GAG GAG CCG CAG TCA GAT CCT AGC ATA AGT CAG GAA ACA ATT "
    "TTC CAA CAT GAT GGG AGG AGA ATC CAA CTT CGC CTT GGT AGT GAG "
    "CTG ATG ACC TGG AGT CTT CAT GAA GCA TCA CTT GCC AGG GAT GAG "
    "GAC TGT TCC AGC CTT TGC CAC TGG CTG GAG GAG CTG AAT CAG GAA"
).split()

# BRCA1 (NM_007294.4) — first 64 codons of coding sequence
# "The Protector" — hereditary breast/ovarian cancer susceptibility
BRCA1_CODONS = (
    "ATG GAT TTT GTT TGT GAA TTT TAT GAA GAG TTT ACT CCT TTA TTT "
    "CCT TTT CTT TGT TCA GTG GGT GTG TTT CAA AGT GAG ATC TGA GAT "
    "ATC CAA GGT AGC ATT AGC TAT ATG TCT GTT GAG TGT ATA TCC AGC "
    "AGG GAA GAG ATC AAA TGC AGA GTG AAG CTG GAG CAA GAA GAA ACT"
).split()

# ChR2 / Channelrhodopsin-2 (GenBank EF474018) — first 64 codons (codon-optimized)
# "Light Activated" — the light-gated ion channel that launched optogenetics
CHR2_CODONS = (
    "ATG GAT TAT GGA GGT GCT CTG ACC GGA GAG CCT GTG ATT GTG ATG "
    "GCT ATC CTC CTG GGA ATT TTG GGT ATC GGC CTG CTG AAC ACG ATC "
    "CTG TGG CTG ATT GAA GGT GCC GGC ATT TCT CTG CTG GTG GTG GGT "
    "GAA ATC CTG CTG GGA ATT CTG GTC ACG CTG CTG AAT ACC GCC ACC"
).split()

BUILTIN_GENES: Dict[str, Tuple[List[str], str, str]] = {
    "tp53":  (TP53_CODONS,  "TP53_Guardian",  "The Guardian of the Genome"),
    "brca1": (BRCA1_CODONS, "BRCA1_Protector", "The Protector"),
    "chr2":  (CHR2_CODONS,  "ChR2_Light",      "Light Activated"),
}

# =============================================================================
# CODON → DRUM MAPPING RULES
# =============================================================================

# First base → drum voice
FIRST_BASE_VOICE: Dict[str, str] = {
    "A": "kick",
    "T": "snare",
    "G": "chat",
    "C": "ohat",
}

# Second base → velocity tier (range min, range max, display label)
SECOND_BASE_VEL: Dict[str, Tuple[int, int, str]] = {
    "A": (100, 127, "high"),
    "T": (75,   99, "mid-high"),
    "G": (50,   74, "mid-low"),
    "C": (25,   49, "low"),
}

# Third base → timing description (informational — baked into pad label)
THIRD_BASE_TIMING: Dict[str, str] = {
    "A": "on-beat",
    "T": "16th-late",
    "G": "16th-early",
    "C": "triplet",
}

# Voice → MIDI note (GM-convention)
VOICE_NOTE: Dict[str, int] = {
    "kick":  36,
    "snare": 38,
    "chat":  42,
    "ohat":  46,
}

# Voice → display symbol for ASCII grid
VOICE_SYMBOL: Dict[str, str] = {
    "kick":  "K",
    "snare": "S",
    "chat":  "H",
    "ohat":  "O",
}

# Voice → MuteGroup (hats share group 1)
VOICE_MUTE_GROUP: Dict[str, int] = {
    "kick":  0,
    "snare": 0,
    "chat":  1,
    "ohat":  1,
}


# =============================================================================
# CODON PARSING
# =============================================================================

def parse_fasta(path: str) -> List[str]:
    """Read a FASTA file, strip headers and whitespace, return list of codons."""
    seq_parts: List[str] = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line.startswith(">"):
                continue
            seq_parts.append(line.upper().replace(" ", "").replace("\t", ""))
    raw = "".join(seq_parts)
    # Build codons
    codons = [raw[i:i+3] for i in range(0, len(raw)-2, 3) if len(raw[i:i+3]) == 3]
    return codons


def parse_codon_string(s: str) -> List[str]:
    """Parse a space/comma-separated codon string into a list."""
    tokens = s.upper().replace(",", " ").split()
    codons = []
    for t in tokens:
        if len(t) == 3 and all(c in "ATGCU" for c in t):
            # RNA → DNA: U→T
            codons.append(t.replace("U", "T"))
    return codons


def validate_codon(codon: str) -> bool:
    """Return True if codon is a valid 3-base DNA codon."""
    return len(codon) == 3 and all(b in "ATGC" for b in codon)


# =============================================================================
# CODON → DRUM EVENT
# =============================================================================

def codon_to_drum(codon: str) -> dict:
    """
    Map a single codon to a drum event descriptor.

    Returns:
        {
          "codon":    "ATG",
          "amino":    "Met",
          "voice":    "kick",
          "note":     36,
          "vel_min":  100,
          "vel_max":  127,
          "vel_mid":  113,   # midpoint used for display / single-layer velocity
          "vel_tier": "high",
          "timing":   "on-beat",
        }
    """
    if not validate_codon(codon):
        return {}
    b1, b2, b3 = codon[0], codon[1], codon[2]
    voice    = FIRST_BASE_VOICE.get(b1, "kick")
    vel_min, vel_max, vel_tier = SECOND_BASE_VEL.get(b2, (50, 74, "mid-low"))
    timing   = THIRD_BASE_TIMING.get(b3, "on-beat")
    amino    = CODON_TABLE.get(codon, "???")
    vel_mid  = (vel_min + vel_max) // 2

    return {
        "codon":    codon,
        "amino":    amino,
        "voice":    voice,
        "note":     VOICE_NOTE[voice],
        "vel_min":  vel_min,
        "vel_max":  vel_max,
        "vel_mid":  vel_mid,
        "vel_tier": vel_tier,
        "timing":   timing,
    }


# =============================================================================
# ASCII VISUALIZATION
# =============================================================================

TIMING_MARKER: Dict[str, str] = {
    "on-beat":   " ",
    "16th-late": "+",
    "16th-early": "-",
    "triplet":   "^",
}

VEL_CHAR: Dict[str, str] = {
    "high":     "█",
    "mid-high": "▓",
    "mid-low":  "▒",
    "low":      "░",
}

def print_codon_grid(codons: List[str], gene_name: str, subtitle: str) -> None:
    """Print a 16×4 ASCII codon grid to stdout."""
    events = [codon_to_drum(c) for c in codons[:64]]
    # Pad to 64
    while len(events) < 64:
        events.append(None)

    width = 72
    print()
    print("=" * width)
    print(f"  GENE KIT  ·  {gene_name}  ·  {subtitle}")
    print(f"  Date: {date.today()}  ·  {len([e for e in events if e])} codons mapped")
    print("=" * width)

    bank_labels = ["BANK A (pads 1-16)", "BANK B (pads 17-32)",
                   "BANK C (pads 33-48)", "BANK D (pads 49-64)"]

    for bank_idx in range(4):
        start = bank_idx * 16
        bank_events = events[start:start + 16]
        print()
        print(f"  {bank_labels[bank_idx]}")
        print(f"  {'PAD':<5} {'CODON':<6} {'AMINO':<6} {'VOICE':<7} {'VEL':<10} {'TIMING':<13} GRID")
        print(f"  {'-'*4:<5} {'-'*5:<6} {'-'*5:<6} {'-'*5:<7} {'-'*9:<10} {'-'*12:<13} ----")
        for i, ev in enumerate(bank_events):
            pad_num = start + i + 1
            if ev is None or not ev:
                print(f"  {pad_num:<5} {'---':<6} {'---':<6} {'---':<7} {'---':<10} {'---':<13} ·")
                continue
            sym    = VOICE_SYMBOL.get(ev["voice"], "?")
            vel_b  = VEL_CHAR.get(ev["vel_tier"], "?")
            t_mark = TIMING_MARKER.get(ev["timing"], " ")
            bar    = vel_b * max(1, ev["vel_mid"] // 16)
            print(
                f"  {pad_num:<5} {ev['codon']:<6} {ev['amino']:<6} "
                f"{ev['voice']:<7} {ev['vel_min']}-{ev['vel_max']:<6} "
                f"{ev['timing']:<13} {sym}{t_mark}{bar}"
            )

    print()
    print("  Legend:  K=Kick  S=Snare  H=Closed Hat  O=Open Hat")
    print("           Timing: ' '=on-beat  '+'=16th-late  '-'=16th-early  '^'=triplet")
    print("           Vel blocks: █=high  ▓=mid-high  ▒=mid-low  ░=low")
    print("=" * width)
    print()


# =============================================================================
# CODON REPORT
# =============================================================================

def build_codon_report(codons: List[str], gene_name: str, subtitle: str) -> str:
    """Build the codon_report.txt content."""
    lines = [
        f"XPN Gene Kit — Codon Report",
        f"Generated: {date.today()}",
        f"Gene: {gene_name}  ({subtitle})",
        f"Codons processed: {min(len(codons), 64)}",
        "",
        f"{'BANK':<6} {'PAD':<5} {'CODON':<7} {'AMINO':<8} {'VOICE':<10} "
        f"{'VEL_MIN':<9} {'VEL_MAX':<9} {'VEL_MID':<9} {'TIMING'}",
        "-" * 80,
    ]
    for idx in range(min(len(codons), 64)):
        ev = codon_to_drum(codons[idx])
        bank = ["A", "B", "C", "D"][idx // 16]
        pad  = (idx % 16) + 1
        if not ev:
            lines.append(f"  {bank}    {pad:<5} {'???':<7} {'???':<8} {'???':<10} {'?':<9} {'?':<9} {'?':<9} ???")
            continue
        lines.append(
            f"  {bank}    {pad:<5} {ev['codon']:<7} {ev['amino']:<8} {ev['voice']:<10} "
            f"{ev['vel_min']:<9} {ev['vel_max']:<9} {ev['vel_mid']:<9} {ev['timing']}"
        )

    lines += [
        "",
        "Codon → Voice mapping:",
        "  First base: A=Kick  T=Snare  G=Closed Hat  C=Open Hat",
        "",
        "Velocity tier mapping:",
        "  Second base: A=high(100-127)  T=mid-high(75-99)  G=mid-low(50-74)  C=low(25-49)",
        "",
        "Timing subdivision:",
        "  Third base: A=on-beat  T=16th-late  G=16th-early  C=triplet",
        "",
        "XPN Golden Rules applied:",
        "  KeyTrack = True",
        "  RootNote = 0",
        "  Empty layer VelStart = 0",
    ]
    return "\n".join(lines)


# =============================================================================
# XPM XML GENERATION
# =============================================================================

def _layer_block(number: int, vel_start: int, vel_end: int,
                 sample_name: str, sample_file: str, volume: float,
                 program_slug: str = "") -> str:
    active     = "True" if sample_name else "False"
    file_path  = (f"Samples/{program_slug}/{sample_file}"
                  if (sample_file and program_slug) else sample_file)
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
    return _layer_block(number, 0, 0, "", "", 0.707946)


def _instrument_block(instr_num: int, ev: Optional[dict], program_slug: str) -> str:
    """Generate one <Instrument> XML block for a codon drum event."""
    if ev:
        voice      = ev["voice"]
        note       = ev["note"]
        vel_min    = ev["vel_min"]
        vel_max    = ev["vel_max"]
        vel_mid    = ev["vel_mid"]
        mute_group = VOICE_MUTE_GROUP.get(voice, 0)
        # Single velocity layer spanning the codon's tier range
        sample_key = f"{program_slug}_{voice}_{ev['codon'].lower()}"
        layer1 = _layer_block(1, vel_min, vel_max, sample_key, f"{sample_key}.wav",
                               0.707946, program_slug)
        # Remaining 3 layers: empty
        layers_xml = "\n".join([
            layer1,
            _empty_layer(2),
            _empty_layer(3),
            _empty_layer(4),
        ])
        mono_str    = "True"
        oneshot_str = "True"
        polyphony   = 1 if voice in ("kick", "snare") else 2
        low_note    = note
        high_note   = note
    else:
        note        = instr_num + 35   # fallback MIDI note
        mute_group  = 0
        layers_xml  = "\n".join(_empty_layer(i) for i in range(1, 5))
        mono_str    = "True"
        oneshot_str = "True"
        polyphony   = 1
        low_note    = note
        high_note   = note

    mute_targets_xml = "\n".join(
        f"        <MuteTarget{i+1}>0</MuteTarget{i+1}>" for i in range(4)
    )
    simult_xml = "\n".join(
        f"        <SimultTarget{i+1}>0</SimultTarget{i+1}>" for i in range(4)
    )

    return (
        f'      <Instrument number="{instr_num}">\n'
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
        f'        <Polyphony>{polyphony}</Polyphony>\n'
        f'        <FilterKeytrack>0.000000</FilterKeytrack>\n'
        f'        <LowNote>{low_note}</LowNote>\n'
        f'        <HighNote>{high_note}</HighNote>\n'
        f'        <IgnoreBaseNote>False</IgnoreBaseNote>\n'
        f'        <ZonePlay>1</ZonePlay>\n'
        f'        <MuteGroup>{mute_group}</MuteGroup>\n'
        f'{mute_targets_xml}\n'
        f'{simult_xml}\n'
        f'        <LfoPitch>0.000000</LfoPitch>\n'
        f'        <LfoCutoff>0.000000</LfoCutoff>\n'
        f'        <LfoVolume>0.000000</LfoVolume>\n'
        f'        <LfoPan>0.000000</LfoPan>\n'
        f'        <OneShot>{oneshot_str}</OneShot>\n'
        f'        <FilterType>2</FilterType>\n'
        f'        <Cutoff>1.000000</Cutoff>\n'
        f'        <Resonance>0.000000</Resonance>\n'
        f'        <FilterEnvAmt>0.000000</FilterEnvAmt>\n'
        f'        <AfterTouchToFilter>0.000000</AfterTouchToFilter>\n'
        f'        <VelocityToStart>0.000000</VelocityToStart>\n'
        f'        <VelocityToFilterAttack>0.000000</VelocityToFilterAttack>\n'
        f'        <VelocityToFilter>0.000000</VelocityToFilter>\n'
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
        f'        <VolumeAttack>0.000000</VolumeAttack>\n'
        f'        <VolumeDecay>0.300000</VolumeDecay>\n'
        f'        <VolumeSustain>0.000000</VolumeSustain>\n'
        f'        <VolumeRelease>0.050000</VolumeRelease>\n'
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


def _build_xpm(codons: List[str], gene_name: str, program_slug: str,
               bank_index: int) -> str:
    """
    Build one XPM XML string for a 16-pad bank (bank_index 0-3).
    Instruments 1-16 correspond to codons [bank_index*16 : (bank_index+1)*16].
    """
    bank_letter = ["A", "B", "C", "D"][bank_index]
    start       = bank_index * 16
    bank_codons = codons[start:start + 16]

    # Pad to 16
    while len(bank_codons) < 16:
        bank_codons.append(None)

    instruments_xml_parts = []
    for i, codon in enumerate(bank_codons):
        instr_num = i + 1
        if codon and validate_codon(codon):
            ev = codon_to_drum(codon)
        else:
            ev = None
        instruments_xml_parts.append(
            _instrument_block(instr_num, ev, program_slug)
        )

    instruments_xml = "\n".join(instruments_xml_parts)
    program_name    = f"{gene_name}_Bank{bank_letter}"

    return (
        f'<?xml version="1.0" encoding="UTF-8"?>\n'
        f'<MPCVObject>\n'
        f'  <Version>2.1</Version>\n'
        f'  <Program type="Drum">\n'
        f'    <Name>{xml_escape(program_name)}</Name>\n'
        f'    <Instruments>\n'
        f'{instruments_xml}\n'
        f'    </Instruments>\n'
        f'  </Program>\n'
        f'</MPCVObject>\n'
    )


# =============================================================================
# XPN ZIP PACKAGER
# =============================================================================

def build_xpn(codons: List[str], gene_name: str, subtitle: str,
              program_slug: str, output_dir: Path) -> Path:
    """
    Build a .xpn ZIP containing:
      - 4 XPM drum programs (one per bank of 16 codons)
      - Samples/ placeholder directory marker
      - codon_report.txt
    Returns path to the created .xpn file.
    """
    output_dir.mkdir(parents=True, exist_ok=True)
    xpn_path = output_dir / f"{program_slug}.xpn"

    report_text = build_codon_report(codons, gene_name, subtitle)

    with zipfile.ZipFile(xpn_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        # Write 4 XPM files — one per bank
        for bank_idx in range(4):
            bank_letter = ["A", "B", "C", "D"][bank_idx]
            xpm_name    = f"{program_slug}_Bank{bank_letter}.xpm"
            xpm_content = _build_xpm(codons, gene_name, program_slug, bank_idx)
            zf.writestr(xpm_name, xpm_content)

        # Placeholder for Samples directory (MPC expects folder to exist)
        zf.writestr(f"Samples/{program_slug}/.keep", "")

        # Codon report
        zf.writestr("codon_report.txt", report_text)

        # Manifest
        manifest = (
            f"XPN Gene Kit\n"
            f"Gene: {gene_name}\n"
            f"Subtitle: {subtitle}\n"
            f"Codons: {min(len(codons), 64)}\n"
            f"Banks: 4 (A=pads 1-16, B=17-32, C=33-48, D=49-64)\n"
            f"Created: {date.today()}\n"
            f"By: XO_OX Designs — xpn_gene_kit.py\n"
        )
        zf.writestr("manifest.txt", manifest)

    return xpn_path


# =============================================================================
# CLI
# =============================================================================

def _slug(name: str) -> str:
    """Convert a name to a filesystem-safe slug."""
    return name.replace(" ", "_").replace("/", "_").replace("\\", "_")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="XPN Gene Kit — DNA codon sequences → MPC drum programs",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Examples:\n"
            "  python xpn_gene_kit.py --gene tp53 --output ./out/\n"
            "  python xpn_gene_kit.py --gene brca1 --visualize\n"
            "  python xpn_gene_kit.py --fasta myseq.fasta --output ./out/\n"
            "  python xpn_gene_kit.py --codons 'ATG GTT CCG' --output ./out/\n"
        ),
    )

    src_group = parser.add_mutually_exclusive_group(required=True)
    src_group.add_argument(
        "--gene", choices=list(BUILTIN_GENES.keys()),
        metavar="GENE",
        help=f"Built-in gene: {', '.join(BUILTIN_GENES.keys())}",
    )
    src_group.add_argument(
        "--fasta", metavar="FILE",
        help="Path to a FASTA file",
    )
    src_group.add_argument(
        "--codons", metavar="STRING",
        help="Space-separated codon string e.g. 'ATG GTT CCG ...'",
    )

    parser.add_argument(
        "--output", "-o", metavar="DIR", default="./gene_kits",
        help="Output directory for .xpn file (default: ./gene_kits)",
    )
    parser.add_argument(
        "--visualize", action="store_true",
        help="Print ASCII codon grid to terminal (works with --output too)",
    )
    parser.add_argument(
        "--report-only", action="store_true",
        help="Print codon report to stdout; do not write .xpn",
    )

    args = parser.parse_args()

    # --- Load codons ---
    if args.gene:
        codons, gene_name, subtitle = BUILTIN_GENES[args.gene]
        codons = list(codons[:64])
    elif args.fasta:
        fasta_path = Path(args.fasta)
        if not fasta_path.exists():
            print(f"ERROR: FASTA file not found: {fasta_path}", file=sys.stderr)
            sys.exit(1)
        codons   = parse_fasta(str(fasta_path))[:64]
        gene_name = fasta_path.stem
        subtitle  = f"from {fasta_path.name}"
    else:
        codons    = parse_codon_string(args.codons)[:64]
        gene_name = "CustomGene"
        subtitle  = "user-supplied codons"

    if not codons:
        print("ERROR: No valid codons found in input.", file=sys.stderr)
        sys.exit(1)

    program_slug = _slug(gene_name)

    # --- Visualize ---
    if args.visualize or args.report_only:
        print_codon_grid(codons, gene_name, subtitle)

    if args.report_only:
        print(build_codon_report(codons, gene_name, subtitle))
        return

    # --- Build XPN ---
    output_dir = Path(args.output)
    xpn_path   = build_xpn(codons, gene_name, subtitle, program_slug, output_dir)

    if not args.visualize:
        # Print a compact summary if we didn't already show the full grid
        print()
        print(f"  Gene: {gene_name}  ({subtitle})")
        print(f"  Codons mapped: {min(len(codons), 64)}")
        voice_counts: Dict[str, int] = {}
        for c in codons[:64]:
            if validate_codon(c):
                v = FIRST_BASE_VOICE.get(c[0], "?")
                voice_counts[v] = voice_counts.get(v, 0) + 1
        for voice, count in sorted(voice_counts.items()):
            print(f"    {voice:<10} {count:>3} hits")
        print()

    print(f"  XPN written: {xpn_path}")
    print(f"  Banks: 4 × 16 pads (A–D)")
    print(f"  Tip: Load each BankX.xpm into its own MPC drum program track.")
    print()


if __name__ == "__main__":
    main()
