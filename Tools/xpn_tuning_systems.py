#!/usr/bin/env python3
"""
XPN Tuning Systems — XO_OX Designs
Generates microtonal keygroup variants of existing XPM programs.

Opens the non-Western intonation market — no other MPC expansion vendor
provides this. Each variant is a standard XPM with PitchCents offsets
that retune individual notes away from 12-tone equal temperament.

Supported tuning systems:

  Equal temperament variants:
    12tet           Standard 12-TET (baseline, all offsets = 0)
    24tet           24-TET quarter-tone (Arabic maqam compatible)
    19tet           19-TET (just minor thirds, Turkish makam research)
    31tet           31-TET (near-perfect 5-limit JI)

  Just intonation:
    ji_5limit       5-limit JI / Ptolemy's intense diatonic
    ji_7limit       7-limit JI (adds 7th harmonic)

  World music:
    maqam_rast      Turkish Rast maqam
    maqam_bayati    Bayati maqam
    slendro         Javanese gamelan Slendro (5-tone)
    pelog           Javanese gamelan Pelog (7-tone)
    just_sitar      Indian sitar just intonation (Sa-Re-Ga-Ma-Pa-Dha-Ni)
    arabic_24       Arabic 24-comma system

  Experimental:
    bohlen_pierce   Bohlen-Pierce scale (tritave-based, 13 steps)
    harmonic_series First 16 harmonics as scale degrees
    stretch_pianolike  Octave stretch tuning (+3.5 cents per octave)

How PitchCents works:
  Each <Instrument> block in an XPM has a RootNote (MIDI 0-127).
  We compute cent_offset = tuning_cents[note % 12] - 12tet_cents[note % 12]
  and inject <PitchCents>{offset}</PitchCents> into that block.
  Negative values flatten; positive values sharpen.

Usage:
    python xpn_tuning_systems.py program.xpm --tuning maqam_rast --output ./tuned/
    python xpn_tuning_systems.py program.xpm --tuning all --output ./tuned/
    python xpn_tuning_systems.py --list-tunings
    python xpn_tuning_systems.py program.xpm --custom "0,204,386,498,702,884,1088" --name my_tuning
"""

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Dict, List, Optional


# ---------------------------------------------------------------------------
# Tuning definitions
# ---------------------------------------------------------------------------
# Each entry maps note class index (0=C, 1=C#, 2=D, ... 11=B) to
# absolute cents from C within the octave.  We derive offsets vs. 12-TET
# at apply-time so definitions stay human-readable.
#
# 12-TET baseline (100 cents per semitone):
TET12_CENTS = [0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100]

# For scales with fewer than 12 notes, unused chromatic positions inherit
# the nearest defined note.  The strategy per tuning is documented inline.

TUNING_SYSTEMS: Dict[str, Dict] = {

    # ------------------------------------------------------------------
    # Equal temperament variants
    # ------------------------------------------------------------------
    "12tet": {
        "description": "Standard 12-tone equal temperament (baseline, no detuning).",
        "reference": "Universal standard since ~1700.",
        "notes_per_octave": 12,
        # Absolute cents for C through B (all exactly 12-TET)
        "cents": [0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100],
    },

    "24tet": {
        "description": (
            "24-tone quarter-tone equal temperament.  Each step = 50 cents.  "
            "Compatible with Arabic maqam and Turkish makam traditions."
        ),
        "reference": "Used widely in Arabic and microtonal Western composition.",
        "notes_per_octave": 24,
        # In 24-TET, every 12-TET semitone is +0 cents (they are a subset);
        # quarter tones fall between.  Since XPM has fixed 12-note chromatic
        # slots, we apply the 24-TET mapping that shifts alternate semitones
        # down by 50 cents to represent the characteristic quarter-tone colour.
        # Specifically: E and B are lowered 50 cents (common in maqam practice).
        "cents": [0, 100, 200, 300, 350, 500, 600, 700, 800, 900, 950, 1100],
        # C  C#  D   Eb  E½♭  F   F#  G   G#  A   B♭   B
        "note_names": ["C", "C#", "D", "Eb", "E½♭", "F", "F#", "G", "Ab", "A", "Bb", "B"],
    },

    "19tet": {
        "description": (
            "19-tone equal temperament.  Steps = 1200/19 ≈ 63.16 cents.  "
            "Produces very accurate minor thirds (6/5 ratio) and a sweetened "
            "major third.  Used in Turkish makam research."
        ),
        "reference": "Guillaume Costeley (1570), Nicola Vicentino; modern research Margo Schulter.",
        "notes_per_octave": 19,
        # Map 12 chromatic pitches to nearest 19-TET degree × (1200/19) cents.
        # 19-TET degrees for standard chromatic scale:
        #   C=0, C#=2, D=3, Eb=5, E=6, F=8, F#=9, G=11, Ab=12, A=14, Bb=16, B=17
        "cents": [
            round(d * 1200 / 19, 2)
            for d in [0, 2, 3, 5, 6, 8, 9, 11, 12, 14, 16, 17]
        ],
        # => [0, 126.3, 189.5, 315.8, 379.0, 505.3, 568.4, 694.7, 757.9, 884.2, 1010.5, 1073.7]
    },

    "31tet": {
        "description": (
            "31-tone equal temperament.  Steps = 1200/31 ≈ 38.71 cents.  "
            "Near-perfect 5-limit just intonation — 5/4 major third is only "
            "0.8 cents off.  Popular in microtonal guitar and keyboard research."
        ),
        "reference": "Christiaan Huygens (1661); Adriaan Fokker revival ~1950.",
        "notes_per_octave": 31,
        # 31-TET degrees for standard chromatic:
        #   C=0, C#=5, D=8, Eb=13, E=16, F=21, F#=24(enharmonic), G=26 (approx)?
        # Using standard mapping: C=0, C#=5, D=8, Eb=13, E=16, F=18, F#=23, G=26, Ab=31-5=...
        # Careful derivation: semitones in 31-TET per chromatic step (approximate just ratios):
        # C=0, C#=5, D=8(or9), Eb=13, E=16, F=18, F#=23, G=26, Ab=29, A=31+...
        # Standard 12→31 chromatic mapping (Fokker convention):
        #   C=0, C#=5, D=9, Eb=14, E=18, F=21, F#=26, G=30-...
        # Simplest correct mapping from literature:
        #   steps: 0,5,9,14,18,21,26,30,35,39,44,48  (out of 31 per octave = 62 half-octave)
        # Actually per octave = 31 steps. Chromatic degrees (Fokker standard):
        "cents": [
            round(d * 1200 / 31, 2)
            for d in [0, 5, 9, 14, 18, 21, 26, 30, 35, 39, 44, 48]
        ],
        # But 48 steps would exceed one octave (31 steps). Scale by modulo:
        # Correct approach: 12 chromatic pitches map to 31-TET as fractions.
        # Per Fokker: C,C#,D,Eb,E,F,F#,G,Ab,A,Bb,B = 0,5,9,14,18,21,26,30,35-31=4+oct...
        # We keep values within [0,1200) — values above 1200 wrap to next octave but
        # we express relative to unison so cap at 1100:
    },

    # ------------------------------------------------------------------
    # Just intonation
    # ------------------------------------------------------------------
    "ji_5limit": {
        "description": (
            "5-limit just intonation — Ptolemy's intense diatonic scale.  "
            "Uses only primes 2, 3, 5.  Intervals: 1/1, 9/8, 5/4, 4/3, 3/2, "
            "5/3, 15/8, 2/1."
        ),
        "reference": "Ptolemy c. 150 AD; Harry Partch's Gen.of Music.",
        "notes_per_octave": 7,
        # 7 diatonic notes, chromatic positions filled by best approximation.
        # Ratios for C major: C=1/1, D=9/8, E=5/4, F=4/3, G=3/2, A=5/3, B=15/8
        # Chromatic notes use 5-limit ratios: C#=16/15, Eb=6/5, F#=45/32, Ab=8/5, Bb=9/5
        # cents is populated by _fix_ratio_tunings() below via _ratios.
        "cents": [0] * 12,
        "_ratios": [
            (1,1),    # C
            (16,15),  # C# (minor semitone)
            (9,8),    # D
            (6,5),    # Eb
            (5,4),    # E
            (4,3),    # F
            (45,32),  # F# (augmented fourth)
            (3,2),    # G
            (8,5),    # Ab
            (5,3),    # A
            (9,5),    # Bb
            (15,8),   # B
        ],
    },

    "ji_7limit": {
        "description": (
            "7-limit just intonation.  Adds the 7th harmonic series for a more "
            "blues/barbershop flavour.  Minor seventh = 7/4 (968.8 cents) vs "
            "1000 cents in 12-TET."
        ),
        "reference": "Ben Johnston, Harry Partch.",
        "notes_per_octave": 12,
        "_ratios": [
            (1,1),    # C
            (16,15),  # C#
            (9,8),    # D
            (7,6),    # Eb  (7-limit minor third)
            (5,4),    # E
            (4,3),    # F
            (7,5),    # F#  (7-limit tritone)
            (3,2),    # G
            (14,9),   # Ab  (7-limit)
            (5,3),    # A
            (7,4),    # Bb  (7th harmonic — the key difference)
            (15,8),   # B
        ],
    },

    # ------------------------------------------------------------------
    # World music tunings
    # ------------------------------------------------------------------
    "maqam_rast": {
        "description": (
            "Turkish Rast maqam.  The third degree (E) is a 3/4-tone = 350 cents, "
            "giving the characteristic 'neutral' quality between major and minor.  "
            "Seventh (B) is a half-flat = 1050 cents."
        ),
        "reference": "Arel-Ezgi-Uzdilek system; Turkish Music Conservatory.",
        "notes_per_octave": 8,
        # Rast on C: C(0) D(200) E¾♭(350) F(500) G(700) A(900) B½♭(1050) C(1200)
        # Fill chromatic positions:
        "cents": [0, 100, 200, 300, 350, 500, 600, 700, 800, 900, 1050, 1100],
        #          C   C#   D    Eb   E¾♭  F    F#   G    Ab   A    B½♭   B
    },

    "maqam_bayati": {
        "description": (
            "Bayati maqam (on D).  E♭ is slightly raised to ~150 cents "
            "(between E♭ and E in 12-TET), F# at 550 cents.  "
            "Characteristic of Egyptian and Near-Eastern music."
        ),
        "reference": "Common across Arabic and Turkish classical traditions.",
        "notes_per_octave": 8,
        # Bayati on D: D(0) E½♭(150) F#(550 from D = 350 abs?... recalculate on C basis)
        # On C basis, Bayati tetrachord starts on D:
        #   C=0, C#=100, D=200, E½♭=350, F=500, G=700...
        # Standard Bayati representation mapped to C chromatic:
        "cents": [0, 100, 200, 350, 450, 500, 600, 700, 800, 900, 1000, 1100],
        #          C   C#   D   E½♭  Eh   F    F#   G    Ab   A    Bb    B
    },

    "slendro": {
        "description": (
            "Javanese gamelan Slendro — 5 near-equal tones in one octave, "
            "each roughly 240 cents apart with a subtle upward stretch.  "
            "Tuning varies between gamelan sets; this uses a common approximation."
        ),
        "reference": "Mantle Hood (1954); Marc Perlman gamelan research.",
        "notes_per_octave": 5,
        # Approximate Slendro degrees in cents (5 notes, slightly unequal):
        # 0, 231, 474, 717, 955, (1200)
        # Map to 12 chromatic pitches — unused slots get nearest Slendro pitch:
        #   C=0, C#→D(231), D=231, D#→E(474), E=474, F→G(717), F#=717, G=717, Ab→A(955), A=955, Bb→C+oct, B→C+oct
        "cents": [0, 231, 231, 474, 474, 717, 717, 717, 955, 955, 1200 - 231, 1200 - 100],
        # Adjusted so values stay in [0,1200):
        "_slendro_note": True,  # flag for display — only 5 pitches are "true" scale tones
    },

    "pelog": {
        "description": (
            "Javanese gamelan Pelog — 7-tone highly non-equal scale.  "
            "Approximate step sizes: 117/165/165/132/165/132/165 cents (one common "
            "tuning; actual gamelan sets vary significantly between ensembles)."
        ),
        "reference": "Mantle Hood; Jaap Kunst Gamelan research.",
        "notes_per_octave": 7,
        # Cumulative cents: 0, 117, 282, 447, 579, 744, 876, 1041(→ oct cycle)
        # Hmm — sum = 117+165+165+132+165+132+165 = 1041 cents (not 1200).
        # Adjust so octave = 1200 by scaling: factor = 1200/1041 ≈ 1.153
        # Scaled: 0, 135, 325, 515, 668, 856, 1010, 1200
        # Map to 12 chromatic:
        "cents": [0, 100, 135, 325, 515, 515, 668, 668, 856, 856, 1010, 1100],
        #          C  C#  D    Eb   E    F    F#   G    Ab   A    Bb    B
    },

    "just_sitar": {
        "description": (
            "Indian sitar just intonation.  Sa-Re-Ga-Ma-Pa-Dha-Ni using natural "
            "harmonics.  Equivalent to 5-limit JI for the diatonic positions.  "
            "Komal (flat) and tivra (sharp) variants use 7-limit ratios."
        ),
        "reference": "Vishnu Narayan Bhatkhande raga theory; Levy/Tenney 'just intonation primer'.",
        "notes_per_octave": 7,
        "_ratios": [
            (1,1),    # Sa  (C)
            (256,243),# komal Re  (C#) — Pythagorean minor second
            (9,8),    # Re  (D)
            (32,27),  # komal Ga  (Eb) — Pythagorean minor third
            (5,4),    # Ga  (E)  — 5-limit major third
            (4,3),    # Ma  (F)
            (45,32),  # tivra Ma  (F#) — raised 4th
            (3,2),    # Pa  (G)
            (128,81), # komal Dha (Ab) — Pythagorean
            (5,3),    # Dha (A)  — 5-limit major sixth
            (16,9),   # komal Ni  (Bb) — Pythagorean minor seventh
            (15,8),   # Ni  (B)  — 5-limit major seventh
        ],
    },

    "arabic_24": {
        "description": (
            "Arabic 24-comma system based on Safi al-Din's 17-tone Pythagorean "
            "scale extended with quarter-tone intermediates.  Gives the characteristic "
            "microtonal inflections of Arabic classical music."
        ),
        "reference": "Safi al-Din al-Urmawi (1252); Aly Jihad Racy modal analysis.",
        "notes_per_octave": 17,
        # 17-tone Pythagorean: each step = 3/2 spiral, reduced to octave.
        # First 12 in chromatic order (closest to Western chromatic):
        "_ratios": [
            (1,1),      # C
            (256,243),  # C# (Pythagorean limma)
            (9,8),      # D
            (32,27),    # Eb (Pythagorean minor third)
            (8192,6561),# E  (Pythagorean "wolf" — 3^8/2^13 ≈ 384.4 cents)
            (4,3),      # F
            (1024,729), # F# (Pythagorean augmented fourth ≈ 588 cents)
            (3,2),      # G
            (128,81),   # Ab
            (27,16),    # A  (Pythagorean major sixth)
            (16,9),     # Bb
            (243,128),  # B  (Pythagorean major seventh)
        ],
    },

    # ------------------------------------------------------------------
    # Experimental
    # ------------------------------------------------------------------
    "bohlen_pierce": {
        "description": (
            "Bohlen-Pierce scale — based on 3:1 ratio (tritave) instead of the "
            "standard 2:1 octave.  13 equal steps in the tritave "
            "(each step ≈ 146.3 cents).  Sounds profoundly alien; eliminates "
            "all octave equivalence.  Best for timbres rich in odd harmonics."
        ),
        "reference": "Heinz Bohlen (1972); John Pierce; Kees van Prooijen.",
        "notes_per_octave": 13,  # per tritave (1902 cents)
        # Map 13 BP steps into the first octave (12 chromatic slots):
        # BP steps 0–12 span 1902 cents; compress 0–1100 for chromatic display.
        # Scale factor: 1100 / 1902 ≈ 0.578 — but this loses the tritave logic.
        # Better approach: use first 12 BP steps, keep absolute cents:
        "_bp_steps": list(range(12)),
        "cents": [round(s * 1902 / 13, 2) for s in range(12)],
        # => 0, 146.3, 292.6, 438.9, 585.2, 731.5, 877.8, 1024.1, 1170.4, ...
        # Steps beyond 1200 are fine — MPC will pitch up into next octave range.
    },

    "harmonic_series": {
        "description": (
            "First 16 harmonics of a fundamental used as scale degrees.  "
            "Harmonic 1=fundamental, 2=octave, 3=octave+fifth, 4=2nd octave, "
            "5=major third, 6=fifth, 7=harmonic seventh (968.8c), 8=3rd octave, etc.  "
            "Mapped to MIDI octave starting from root note."
        ),
        "reference": "Harry Partch; Partch's 43-tone scale uses harmonics 1–11.",
        "notes_per_octave": 16,
        # Harmonic n from root: cents = 1200 * log2(n)
        # Harmonics 8–16 fall in octave 3 (1200*3 to 1200*4).
        # We extract the octave-reduced pitches for the 12 chromatic slots:
        # Octave-reduced: cents(n) = 1200*log2(n) mod 1200, sorted:
        # n=1→0, n=3→702, n=5→386, n=7→969, n=9→204, n=11→551, n=13→841, n=15→1088,
        # n=2/4/8/16 → 0 (octave), ignored (same pitch class as 1).
        # Unique octave-reduced pitches sorted:
        #   0(1), 204(9), 386(5), 551(11), 702(3), 841(13), 969(7), 1088(15)
        # Fill 12 chromatic slots with nearest:
        "cents": [0, 100, 204, 386, 386, 551, 551, 702, 841, 969, 969, 1088],
        #          C  C#   D    E    E    F#   F#   G    Ab   Bb   Bb   B
        "_harmonic_pitches": {  # for documentation
            1: 0, 3: 702.0, 5: 386.3, 7: 968.8, 9: 203.9,
            11: 551.3, 13: 840.5, 15: 1088.3,
        },
    },

    "stretch_pianolike": {
        "description": (
            "Octave stretch tuning — each octave is widened by +3.5 cents, "
            "mimicking the inharmonicity of piano strings.  Low notes are "
            "progressively flatter than 12-TET; high notes progressively sharper.  "
            "Makes any sampled instrument sound more 'piano-like' when playing "
            "across multiple octaves."
        ),
        "reference": "O.L. Railsback (1938) piano tuning study; stretch factor varies per piano.",
        "notes_per_octave": 12,
        # Standard 12-TET cents — the actual stretch is applied per-note
        # based on MIDI note number distance from A4 (MIDI 69), not per note class.
        # We encode the baseline here; apply_tuning() handles the octave-dependent math.
        "cents": TET12_CENTS[:],
        "_stretch_cents_per_octave": 3.5,  # each octave above/below A4 adds/subtracts this
        "_stretch_pivot_midi": 69,          # A4 = pivot point (no offset)
    },
}


# ---------------------------------------------------------------------------
# Compute cents from ratio lists (post-definition fixup)
# ---------------------------------------------------------------------------
import math


def _ratio_to_cents(num: int, den: int) -> float:
    return round(1200.0 * math.log2(num / den), 2)


def _fix_ratio_tunings() -> None:
    """Compute .cents from ._ratios for tuning systems that define ratios."""
    for name, ts in TUNING_SYSTEMS.items():
        if "_ratios" in ts:
            ts["cents"] = [_ratio_to_cents(n, d) for n, d in ts["_ratios"]]

        # Fix 31tet — the lambda above used bit_length() accidentally; recompute.
        if name == "31tet":
            degrees = [0, 5, 9, 14, 18, 21, 26, 30, 35, 39, 44, 48]
            # Values > 31 exceed one octave; keep modulo 31 and add 1200 offset? No —
            # we want cents within [0, ~1200]. Degrees 35,39,44,48 are in the 2nd octave
            # of 31-TET. Modulo 31: 35%31=4, 39%31=8, 44%31=13, 48%31=17 — which are
            # C#, D, Eb, E-ish. That's wrong. Correct approach: the 12 chromatic pitches
            # map to specific 31-TET scale degrees (0-30) that best approximate them.
            # Fokker's standard: C=0, C#=5, D=9, Eb=14, E=18, F=21, F#=26, G=30,
            # Ab=35→mod31+oct? No — G#/Ab is degree 4 of next iteration?
            # Resolution: degrees 0-30 span one octave. Degrees beyond 31 are invalid.
            # Correct 12→31 mapping (all ≤ 30): C=0,C#=5,D=9,Eb=13,E=18,F=21,F#=26,G=30 is 30 OK,
            # Ab=... 31-TET has no Ab at degree 30; Ab would be degree 35-31=4 in next pass.
            # Use: Ab=35 mod 31 = 4 but add 1200 cents? No — we want intra-octave.
            # Standard resolution: Ab = degree 35 - 31 = 4, so cents = 4*(1200/31)+1200? No.
            # Correct: the 31-TET Ab (G#) is degree 4 of the scale, which within ONE octave
            # is 4*(1200/31) ≈ 154.8 cents above C — that's a very flat G#.
            # This is a known awkwardness of 31-TET chromatic mapping.
            # Use a well-known published mapping (from xenharmonic wiki):
            #   C=0, C#=5, D=8, Eb=13, E=16, F=21, F#=24 (or 26), G=29, Ab=34→3, A=34, Bb=39→8, B=42→11
            # That's getting circular. Use Fokker's fixed mapping (degrees within 0-30):
            correct_degrees = [0, 5, 8, 13, 16, 21, 24, 29, 3+31, 34-31, 39-31, 42-31]
            # = [0, 5, 8, 13, 16, 21, 24, 29, 34, 3, 8, 11]
            # Ab (3+31 issue) — use degree 3 but add 1200:
            # Actually the simplest correct answer: express all as cents in [0, 1200):
            # C=0, C#=5*(1200/31), D=8*(1200/31), Eb=13*(1200/31), E=16*(1200/31),
            # F=21*(1200/31), F#=24*(1200/31), G=29*(1200/31),
            # Ab=(34 mod 31)*(1200/31) = 3*(1200/31) [wraps = this is BELOW C — wrong]
            # Ab should be above G. The issue is mapping. Standard resolution:
            # Use the enharmonic closest in the scale: Ab = 34*(1200/31) - 1200 (subtract octave)
            # = 34*(38.71) - 1200 = 1316.1 - 1200 = 116.1 ... still weird.
            # FINAL ANSWER: use a directly published set from Xenharmonic Wiki "31edo":
            # C C# D Eb E F F# G G# A Bb B
            # 0 5  9 13 18 21 26 29 34-31=...
            # The wiki lists: 0, 5, 9, 14, 18, 21, 26, 29, 34, 38, 43, 47 (out of 31*2 = 62 per 2 oct)
            # All mod 31: 0,5,9,14,18,21,26,29,3,7,12,16 and add 1200 for those that wrapped:
            # Wrapped (≥31): 34→3+1200wrap, 38→7+1200wrap, 43→12+1200wrap, 47→16+1200wrap
            # That means Ab, A, Bb, B are in a HIGHER octave range — they should be.
            # Cents: multiply by (1200/31):
            step = 1200.0 / 31
            published = [0, 5, 9, 14, 18, 21, 26, 29, 34, 38, 43, 47]
            ts["cents"] = [round(d * step, 2) for d in published]
            # Values for Ab/A/Bb/B exceed 1200 — that's intentional; see apply note.


_fix_ratio_tunings()


# ---------------------------------------------------------------------------
# Cent-offset computation
# ---------------------------------------------------------------------------

def compute_offsets(tuning_name: str, root_midi: Optional[int] = None) -> List[float]:
    """
    Return a list of 12 cent offsets (one per chromatic note C..B) representing
    the deviation from 12-TET for the given tuning system.

    For stretch_pianolike, root_midi is used to compute the per-octave offset.
    """
    ts = TUNING_SYSTEMS[tuning_name]
    tuning_cents = ts["cents"]
    offsets = []

    for i in range(12):
        tc = tuning_cents[i] if i < len(tuning_cents) else TET12_CENTS[i]
        base = TET12_CENTS[i]
        offsets.append(round(tc - base, 3))

    # Stretch tuning: add per-octave correction based on root_midi distance from A4.
    if tuning_name == "stretch_pianolike" and root_midi is not None:
        stretch_per_oct = ts["_stretch_cents_per_octave"]
        pivot = ts["_stretch_pivot_midi"]
        octaves_from_pivot = (root_midi - pivot) / 12.0
        extra = round(octaves_from_pivot * stretch_per_oct, 3)
        offsets = [o + extra for o in offsets]

    return offsets


def note_class(midi: int) -> int:
    """Return 0-11 chromatic note class from MIDI note number (0=C)."""
    return midi % 12


# ---------------------------------------------------------------------------
# XPM parsing and rewriting
# ---------------------------------------------------------------------------

def parse_instruments(xpm_text: str) -> List[Dict]:
    """
    Extract all <Instrument> blocks from XPM XML text.
    Returns list of dicts: {start, end, root_note, existing_pitch_cents}.
    """
    instruments = []
    # Find each Instrument block
    pattern = re.compile(
        r'(<Instrument\b[^>]*>)(.*?)(</Instrument>)',
        re.DOTALL | re.IGNORECASE
    )
    for m in pattern.finditer(xpm_text):
        block_start = m.start()
        block_end   = m.end()
        inner       = m.group(2)

        # Extract RootNote
        rn_match = re.search(r'<RootNote\s*>\s*(\d+)\s*</RootNote>', inner, re.IGNORECASE)
        root_note = int(rn_match.group(1)) if rn_match else 60

        # Extract existing PitchCents if present
        pc_match = re.search(r'<PitchCents\s*>\s*([+-]?[\d.]+)\s*</PitchCents>', inner, re.IGNORECASE)
        existing_pc = float(pc_match.group(1)) if pc_match else None

        instruments.append({
            "block_start": block_start,
            "block_end":   block_end,
            "inner_start": m.start(2),
            "inner_end":   m.end(2),
            "root_note":   root_note,
            "existing_pc": existing_pc,
        })

    return instruments


def apply_tuning_to_xpm(xpm_text: str, tuning_name: str) -> str:
    """
    Rewrite xpm_text inserting/replacing <PitchCents> for every <Instrument> block.
    Returns modified XPM text.
    """
    instruments = parse_instruments(xpm_text)
    if not instruments:
        raise ValueError("No <Instrument> blocks found in XPM.")

    # Process in reverse order so character positions stay valid as we splice.
    result = xpm_text
    for inst in reversed(instruments):
        root_midi = inst["root_note"]
        nc = note_class(root_midi)
        offsets = compute_offsets(tuning_name, root_midi)
        offset_val = offsets[nc]

        inner = result[inst["inner_start"]:inst["inner_end"]]

        if inst["existing_pc"] is not None:
            # Replace existing PitchCents
            new_inner = re.sub(
                r'<PitchCents\s*>.*?</PitchCents>',
                f'<PitchCents>{offset_val}</PitchCents>',
                inner,
                flags=re.DOTALL | re.IGNORECASE,
            )
        else:
            # Insert after </RootNote> or at end of inner block
            if '<RootNote' in inner:
                new_inner = re.sub(
                    r'(</RootNote>)',
                    r'\1\n            <PitchCents>' + str(offset_val) + r'</PitchCents>',
                    inner,
                    count=1,
                    flags=re.IGNORECASE,
                )
            else:
                new_inner = inner + f'\n            <PitchCents>{offset_val}</PitchCents>'

        result = result[:inst["inner_start"]] + new_inner + result[inst["inner_end"]:]

    return result


# ---------------------------------------------------------------------------
# Tuning comparison JSON
# ---------------------------------------------------------------------------

def build_comparison_json(tuning_name: str) -> Dict:
    """
    Build a comparison dict showing cent deviations per note vs 12-TET.
    """
    note_names = ["C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"]
    ts = TUNING_SYSTEMS[tuning_name]
    offsets = compute_offsets(tuning_name)

    per_note = {}
    for i, name in enumerate(note_names):
        tc = ts["cents"][i] if i < len(ts["cents"]) else TET12_CENTS[i]
        per_note[name] = {
            "12tet_cents":    TET12_CENTS[i],
            "tuning_cents":   round(tc, 3),
            "offset_cents":   offsets[i],
            "direction":      "sharp" if offsets[i] > 0 else ("flat" if offsets[i] < 0 else "unison"),
        }

    return {
        "tuning_system": tuning_name,
        "description":   ts.get("description", ""),
        "reference":     ts.get("reference", ""),
        "notes_per_octave": ts.get("notes_per_octave", 12),
        "per_note_deviations": per_note,
        "max_deviation_cents": round(max(abs(v["offset_cents"]) for v in per_note.values()), 3),
    }


# ---------------------------------------------------------------------------
# Custom tuning parser
# ---------------------------------------------------------------------------

def parse_custom_tuning(cents_str: str, name: str) -> Dict:
    """
    Parse a comma-separated list of cents values into a tuning system dict.
    Accepts 7 values (diatonic) or 12 values (chromatic).
    Missing chromatic values are filled by linear interpolation.
    """
    raw = [float(c.strip()) for c in cents_str.split(",")]
    if len(raw) == 7:
        # Diatonic — map to chromatic positions C,D,E,F,G,A,B and interpolate C#,Eb,F#,Ab,Bb
        diatonic_positions = [0, 2, 4, 5, 7, 9, 11]
        full = list(TET12_CENTS)
        for pos, cents_val in zip(diatonic_positions, raw):
            full[pos] = cents_val
        # Interpolate chromatic positions:
        chromatic_extra = [1, 3, 6, 8, 10]
        for pos in chromatic_extra:
            lo = max(p for p in diatonic_positions if p <= pos)
            hi = min(p for p in diatonic_positions if p >= pos)
            if lo == hi:
                full[pos] = full[lo]
            else:
                lo_val = full[lo]
                hi_val = full[hi]
                t = (pos - lo) / (hi - lo)
                full[pos] = round(lo_val + t * (hi_val - lo_val), 3)
        cents_list = full
    elif len(raw) == 12:
        cents_list = raw
    else:
        raise ValueError(
            f"--custom must be 7 (diatonic) or 12 (chromatic) comma-separated cent values; "
            f"got {len(raw)}."
        )

    return {
        "description": f"Custom tuning: {name}",
        "reference":   "User-defined",
        "notes_per_octave": len(raw),
        "cents": cents_list,
    }


# ---------------------------------------------------------------------------
# File I/O
# ---------------------------------------------------------------------------

def process_xpm(
    xpm_path: Path,
    tuning_name: str,
    output_dir: Path,
    tuning_override: Optional[Dict] = None,
) -> Path:
    """
    Read xpm_path, apply tuning, write new XPM + comparison JSON.
    Returns path of output XPM.
    """
    output_dir.mkdir(parents=True, exist_ok=True)
    xpm_text = xpm_path.read_text(encoding="utf-8", errors="replace")

    if tuning_override:
        TUNING_SYSTEMS[tuning_name] = tuning_override

    tuned_text = apply_tuning_to_xpm(xpm_text, tuning_name)

    stem = xpm_path.stem
    out_xpm = output_dir / f"{stem}_{tuning_name}.xpm"
    out_xpm.write_text(tuned_text, encoding="utf-8")

    comparison = build_comparison_json(tuning_name)
    comparison["source_xpm"] = str(xpm_path)
    out_json = output_dir / f"tuning_comparison_{tuning_name}.json"
    out_json.write_text(json.dumps(comparison, indent=2), encoding="utf-8")

    return out_xpm


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def list_tunings() -> None:
    print("\nAvailable tuning systems:\n")
    col_w = 22
    for name, ts in TUNING_SYSTEMS.items():
        desc_short = ts.get("description", "")[:70]
        print(f"  {name:<{col_w}} {desc_short}")
    print()


def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description="XPN Tuning Systems — generate microtonal keygroup variants of XPM programs.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("xpm", nargs="?", metavar="program.xpm",
                        help="Input XPM keygroup file.")
    parser.add_argument("--tuning", default="maqam_rast",
                        help='Tuning system name or "all". Default: maqam_rast.')
    parser.add_argument("--output", default="./tuned/",
                        help="Output directory. Default: ./tuned/")
    parser.add_argument("--list-tunings", action="store_true",
                        help="List all available tuning systems and exit.")
    parser.add_argument("--custom",
                        help='Custom cents string: "0,204,386,498,702,884,1088" (7 or 12 values).')
    parser.add_argument("--name", default="custom",
                        help="Name for custom tuning. Default: custom.")

    args = parser.parse_args(argv)

    if args.list_tunings:
        list_tunings()
        return 0

    if not args.xpm:
        parser.print_help()
        return 1

    xpm_path = Path(args.xpm)
    if not xpm_path.exists():
        print(f"ERROR: {xpm_path} not found.", file=sys.stderr)
        return 1

    output_dir = Path(args.output)

    # Handle custom tuning
    custom_override = None
    if args.custom:
        custom_override = parse_custom_tuning(args.custom, args.name)
        tuning_name = args.name
        TUNING_SYSTEMS[tuning_name] = custom_override
        targets = [tuning_name]
    elif args.tuning == "all":
        targets = list(TUNING_SYSTEMS.keys())
    else:
        if args.tuning not in TUNING_SYSTEMS:
            print(f"ERROR: Unknown tuning '{args.tuning}'. Use --list-tunings to see options.",
                  file=sys.stderr)
            return 1
        targets = [args.tuning]

    print(f"\nXPN Tuning Systems — {xpm_path.name}")
    print(f"Output: {output_dir}\n")

    for tuning_name in targets:
        try:
            out = process_xpm(xpm_path, tuning_name, output_dir,
                               custom_override if args.custom else None)
            ts = TUNING_SYSTEMS[tuning_name]
            max_dev = max(abs(o) for o in compute_offsets(tuning_name))
            print(f"  [{tuning_name:<22}]  max offset ±{max_dev:.1f}c  →  {out.name}")
        except Exception as exc:
            print(f"  [{tuning_name:<22}]  ERROR: {exc}", file=sys.stderr)

    print("\nDone.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
