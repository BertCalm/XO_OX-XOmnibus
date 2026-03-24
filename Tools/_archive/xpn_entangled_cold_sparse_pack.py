#!/usr/bin/env python3
"""
xpn_entangled_cold_sparse_pack.py

Generates 80 Entangled mood presets targeting the cold-sparse corner:
  brightness <= 0.18  AND  density <= 0.20

16 pairs × 5 presets each = 80 total.

Cold + sparse = crystalline, void-like, spectral, architectural emptiness.
Single frequencies in vast space. Bone-dry and cold.

DNA constraints:
  brightness: 0.04–0.17
  density:    0.04–0.18
  movement:   0.10–0.80 (free)
  warmth:     0.00–0.30 (free)
  space:      0.65–1.00 (free)
  aggression: 0.00–0.25 (free)

Writes to Presets/XOlokun/Entangled/. Skips existing files.
"""

import json
import os

PRESET_DIR = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..", "Presets", "XOlokun", "Entangled"
)

# ---------------------------------------------------------------------------
# Preset data:
# (name, [engineA, engineB], dna_dict, macros_dict, coupling_type,
#  coupling_source, coupling_target, coupling_amount, tags, description)
# ---------------------------------------------------------------------------

PRESETS = [
    # ── ODYSSEY × OBSIDIAN ──────────────────────────────────────────────
    (
        "Void Interval",
        ["ODYSSEY", "OBSIDIAN"],
        dict(brightness=0.06, warmth=0.08, movement=0.22, density=0.07, space=0.94, aggression=0.05),
        dict(CHARACTER=0.15, MOVEMENT=0.22, COUPLING=0.72, SPACE=0.94),
        "SPECTRAL_MORPH", "ODYSSEY", "OBSIDIAN", 0.72,
        ["entangled", "cold-sparse", "void", "crystalline"],
        "Odyssey wavetable drifts through a single spectral node — Obsidian crystallises the silence around it."
    ),
    (
        "Crystal Epoch",
        ["ODYSSEY", "OBSIDIAN"],
        dict(brightness=0.09, warmth=0.05, movement=0.45, density=0.10, space=0.91, aggression=0.08),
        dict(CHARACTER=0.2, MOVEMENT=0.45, COUPLING=0.65, SPACE=0.91),
        "TIMBRE_BLEND", "ODYSSEY", "OBSIDIAN", 0.65,
        ["entangled", "cold-sparse", "crystalline", "spectral"],
        "Two glacial architectures — Odyssey slow-drifts through Obsidian's faceted harmonic lattice."
    ),
    (
        "Bone Frequency",
        ["ODYSSEY", "OBSIDIAN"],
        dict(brightness=0.12, warmth=0.03, movement=0.18, density=0.06, space=0.88, aggression=0.12),
        dict(CHARACTER=0.25, MOVEMENT=0.18, COUPLING=0.78, SPACE=0.88),
        "PITCH_SYNC", "OBSIDIAN", "ODYSSEY", 0.78,
        ["entangled", "cold-sparse", "void", "drone"],
        "A single harmonic bone — Obsidian locks Odyssey's pitch to sub-zero stillness."
    ),
    (
        "Tundra Partial",
        ["ODYSSEY", "OBSIDIAN"],
        dict(brightness=0.07, warmth=0.10, movement=0.35, density=0.14, space=0.85, aggression=0.06),
        dict(CHARACTER=0.18, MOVEMENT=0.35, COUPLING=0.60, SPACE=0.85),
        "SPATIAL_COUPLE", "ODYSSEY", "OBSIDIAN", 0.60,
        ["entangled", "cold-sparse", "tundra", "spectral"],
        "Two partials suspended above frozen ground — spatially coupled, impossibly sparse."
    ),
    (
        "Absolute Zero",
        ["ODYSSEY", "OBSIDIAN"],
        dict(brightness=0.05, warmth=0.02, movement=0.12, density=0.05, space=0.97, aggression=0.03),
        dict(CHARACTER=0.08, MOVEMENT=0.12, COUPLING=0.82, SPACE=0.97),
        "ENVELOPE_LINK", "OBSIDIAN", "ODYSSEY", 0.82,
        ["entangled", "cold-sparse", "void", "silence"],
        "Temperature floor: Obsidian envelope controls Odyssey's emergence into near-silence."
    ),

    # ── OPAL × OBSCURA ──────────────────────────────────────────────────
    (
        "Grain Solitude",
        ["OPAL", "OBSCURA"],
        dict(brightness=0.08, warmth=0.12, movement=0.55, density=0.09, space=0.92, aggression=0.07),
        dict(CHARACTER=0.15, MOVEMENT=0.55, COUPLING=0.68, SPACE=0.92),
        "SPECTRAL_MORPH", "OPAL", "OBSCURA", 0.68,
        ["entangled", "cold-sparse", "granular", "spectral"],
        "A single grain drifts through Obscura's daguerreotype haze — spectral loneliness."
    ),
    (
        "Silver Dispersal",
        ["OPAL", "OBSCURA"],
        dict(brightness=0.11, warmth=0.07, movement=0.40, density=0.12, space=0.89, aggression=0.09),
        dict(CHARACTER=0.2, MOVEMENT=0.40, COUPLING=0.74, SPACE=0.89),
        "TIMBRE_BLEND", "OBSCURA", "OPAL", 0.74,
        ["entangled", "cold-sparse", "silver", "dispersal"],
        "Obscura silver plate blends into Opal granular spray — both vanish into the mix."
    ),
    (
        "Fading Emulsion",
        ["OPAL", "OBSCURA"],
        dict(brightness=0.06, warmth=0.15, movement=0.28, density=0.08, space=0.93, aggression=0.04),
        dict(CHARACTER=0.12, MOVEMENT=0.28, COUPLING=0.80, SPACE=0.93),
        "ENVELOPE_LINK", "OPAL", "OBSCURA", 0.80,
        ["entangled", "cold-sparse", "emulsion", "fading"],
        "Grain envelope shapes Obscura's decay — both dissolve before the eye can fix them."
    ),
    (
        "Cryogenic Cloud",
        ["OPAL", "OBSCURA"],
        dict(brightness=0.14, warmth=0.06, movement=0.62, density=0.15, space=0.87, aggression=0.11),
        dict(CHARACTER=0.22, MOVEMENT=0.62, COUPLING=0.62, SPACE=0.87),
        "FREQUENCY_SHIFT", "OBSCURA", "OPAL", 0.62,
        ["entangled", "cold-sparse", "cryogenic", "cloud"],
        "Obscura frequency-shifts Opal's grain cloud into subzero spectral territory."
    ),
    (
        "Plate Erosion",
        ["OPAL", "OBSCURA"],
        dict(brightness=0.09, warmth=0.04, movement=0.20, density=0.07, space=0.95, aggression=0.05),
        dict(CHARACTER=0.10, MOVEMENT=0.20, COUPLING=0.77, SPACE=0.95),
        "SPATIAL_COUPLE", "OPAL", "OBSCURA", 0.77,
        ["entangled", "cold-sparse", "erosion", "spectral"],
        "A daguerreotype plate dissolving in cold space — grains and silver both erase."
    ),

    # ── ORACLE × ODYSSEY ────────────────────────────────────────────────
    (
        "Prophecy Void",
        ["ORACLE", "ODYSSEY"],
        dict(brightness=0.07, warmth=0.06, movement=0.30, density=0.09, space=0.96, aggression=0.06),
        dict(CHARACTER=0.12, MOVEMENT=0.30, COUPLING=0.76, SPACE=0.96),
        "SPECTRAL_MORPH", "ORACLE", "ODYSSEY", 0.76,
        ["entangled", "cold-sparse", "prophecy", "void"],
        "Oracle stochastic breakpoints scatter into Odyssey's cold drift — a prophecy with no body."
    ),
    (
        "Maqam Zero",
        ["ORACLE", "ODYSSEY"],
        dict(brightness=0.10, warmth=0.08, movement=0.50, density=0.13, space=0.91, aggression=0.08),
        dict(CHARACTER=0.18, MOVEMENT=0.50, COUPLING=0.70, SPACE=0.91),
        "PITCH_SYNC", "ORACLE", "ODYSSEY", 0.70,
        ["entangled", "cold-sparse", "maqam", "spectral"],
        "A single maqam interval floats in void — Oracle pitch-syncs Odyssey to ancient cold."
    ),
    (
        "Threshold Lattice",
        ["ORACLE", "ODYSSEY"],
        dict(brightness=0.05, warmth=0.10, movement=0.18, density=0.07, space=0.97, aggression=0.03),
        dict(CHARACTER=0.09, MOVEMENT=0.18, COUPLING=0.84, SPACE=0.97),
        "ENVELOPE_LINK", "ODYSSEY", "ORACLE", 0.84,
        ["entangled", "cold-sparse", "lattice", "void"],
        "Odyssey envelope gates Oracle's breakpoints — a threshold crossed only in deepest space."
    ),
    (
        "Indigo Dispersal",
        ["ORACLE", "ODYSSEY"],
        dict(brightness=0.13, warmth=0.05, movement=0.42, density=0.16, space=0.88, aggression=0.10),
        dict(CHARACTER=0.20, MOVEMENT=0.42, COUPLING=0.66, SPACE=0.88),
        "TIMBRE_BLEND", "ORACLE", "ODYSSEY", 0.66,
        ["entangled", "cold-sparse", "indigo", "dispersal"],
        "Oracle's prophecy indigo bleeds into Odyssey violet — two cold purples dissolving together."
    ),
    (
        "Stochastic Void",
        ["ORACLE", "ODYSSEY"],
        dict(brightness=0.08, warmth=0.03, movement=0.25, density=0.06, space=0.94, aggression=0.04),
        dict(CHARACTER=0.10, MOVEMENT=0.25, COUPLING=0.80, SPACE=0.94),
        "FREQUENCY_SHIFT", "ORACLE", "ODYSSEY", 0.80,
        ["entangled", "cold-sparse", "stochastic", "void"],
        "GENDY breakpoints shift Odyssey's wavetable frequency — erratic cold silence."
    ),

    # ── ORIGAMI × OBSIDIAN ──────────────────────────────────────────────
    (
        "Folded Crystal",
        ["ORIGAMI", "OBSIDIAN"],
        dict(brightness=0.07, warmth=0.07, movement=0.22, density=0.08, space=0.93, aggression=0.06),
        dict(CHARACTER=0.14, MOVEMENT=0.22, COUPLING=0.73, SPACE=0.93),
        "SPECTRAL_MORPH", "ORIGAMI", "OBSIDIAN", 0.73,
        ["entangled", "cold-sparse", "origami", "crystalline"],
        "A paper fold frozen mid-crease — Origami's vermillion edge meets Obsidian's white silence."
    ),
    (
        "Score of Ice",
        ["ORIGAMI", "OBSIDIAN"],
        dict(brightness=0.11, warmth=0.04, movement=0.38, density=0.12, space=0.90, aggression=0.07),
        dict(CHARACTER=0.18, MOVEMENT=0.38, COUPLING=0.68, SPACE=0.90),
        "TIMBRE_BLEND", "OBSIDIAN", "ORIGAMI", 0.68,
        ["entangled", "cold-sparse", "score", "ice"],
        "Obsidian crystal timbres blend into Origami's paper harmonics — cold scored silence."
    ),
    (
        "Crease Lattice",
        ["ORIGAMI", "OBSIDIAN"],
        dict(brightness=0.06, warmth=0.09, movement=0.15, density=0.06, space=0.95, aggression=0.04),
        dict(CHARACTER=0.09, MOVEMENT=0.15, COUPLING=0.82, SPACE=0.95),
        "ENVELOPE_LINK", "ORIGAMI", "OBSIDIAN", 0.82,
        ["entangled", "cold-sparse", "crease", "void"],
        "Paper envelope shapes crystal decay — a crease becomes a lattice of cold air."
    ),
    (
        "White Fold",
        ["ORIGAMI", "OBSIDIAN"],
        dict(brightness=0.14, warmth=0.06, movement=0.55, density=0.17, space=0.86, aggression=0.12),
        dict(CHARACTER=0.22, MOVEMENT=0.55, COUPLING=0.62, SPACE=0.86),
        "PITCH_SYNC", "OBSIDIAN", "ORIGAMI", 0.62,
        ["entangled", "cold-sparse", "white", "fold"],
        "Obsidian locks pitch to a paper crease — white on white on white."
    ),
    (
        "Angular Silence",
        ["ORIGAMI", "OBSIDIAN"],
        dict(brightness=0.09, warmth=0.02, movement=0.28, density=0.09, space=0.97, aggression=0.05),
        dict(CHARACTER=0.12, MOVEMENT=0.28, COUPLING=0.78, SPACE=0.97),
        "SPATIAL_COUPLE", "ORIGAMI", "OBSIDIAN", 0.78,
        ["entangled", "cold-sparse", "angular", "silence"],
        "Geometric silence — Origami fold angles couple spatially with Obsidian's empty room."
    ),

    # ── ORACLE × OBSCURA ────────────────────────────────────────────────
    (
        "Daguerreotype Oracle",
        ["ORACLE", "OBSCURA"],
        dict(brightness=0.08, warmth=0.13, movement=0.32, density=0.10, space=0.91, aggression=0.07),
        dict(CHARACTER=0.16, MOVEMENT=0.32, COUPLING=0.75, SPACE=0.91),
        "SPECTRAL_MORPH", "ORACLE", "OBSCURA", 0.75,
        ["entangled", "cold-sparse", "daguerreotype", "prophecy"],
        "Prophecy fixed in silver plate — Oracle's voice captured as a fading spectral image."
    ),
    (
        "Stiff Prophecy",
        ["ORACLE", "OBSCURA"],
        dict(brightness=0.11, warmth=0.05, movement=0.45, density=0.13, space=0.89, aggression=0.09),
        dict(CHARACTER=0.20, MOVEMENT=0.45, COUPLING=0.70, SPACE=0.89),
        "TIMBRE_BLEND", "OBSCURA", "ORACLE", 0.70,
        ["entangled", "cold-sparse", "stiff", "prophecy"],
        "Obscura string stiffness timbres blend with Oracle's GENDY voice — prophecy in metal strings."
    ),
    (
        "Silver Void",
        ["ORACLE", "OBSCURA"],
        dict(brightness=0.05, warmth=0.03, movement=0.18, density=0.06, space=0.96, aggression=0.03),
        dict(CHARACTER=0.08, MOVEMENT=0.18, COUPLING=0.85, SPACE=0.96),
        "ENVELOPE_LINK", "OBSCURA", "ORACLE", 0.85,
        ["entangled", "cold-sparse", "silver", "void"],
        "Obscura's decay envelope silences Oracle — what the oracle foretold was silence."
    ),
    (
        "Cold Augur",
        ["ORACLE", "OBSCURA"],
        dict(brightness=0.14, warmth=0.08, movement=0.58, density=0.16, space=0.87, aggression=0.11),
        dict(CHARACTER=0.22, MOVEMENT=0.58, COUPLING=0.64, SPACE=0.87),
        "FREQUENCY_SHIFT", "ORACLE", "OBSCURA", 0.64,
        ["entangled", "cold-sparse", "augur", "spectral"],
        "Oracle shifts Obscura's resonant frequencies into cold augury — reading the stiffness of ice."
    ),
    (
        "Indigo Plate",
        ["ORACLE", "OBSCURA"],
        dict(brightness=0.09, warmth=0.06, movement=0.25, density=0.08, space=0.94, aggression=0.06),
        dict(CHARACTER=0.13, MOVEMENT=0.25, COUPLING=0.79, SPACE=0.94),
        "PITCH_SYNC", "ORACLE", "OBSCURA", 0.79,
        ["entangled", "cold-sparse", "indigo", "plate"],
        "Oracle's indigo pitch anchors Obscura's silverpoint resonance — cold tonality fixed."
    ),

    # ── OPAL × ODYSSEY ──────────────────────────────────────────────────
    (
        "Grain Drift",
        ["OPAL", "ODYSSEY"],
        dict(brightness=0.07, warmth=0.09, movement=0.48, density=0.11, space=0.92, aggression=0.06),
        dict(CHARACTER=0.15, MOVEMENT=0.48, COUPLING=0.71, SPACE=0.92),
        "SPECTRAL_MORPH", "OPAL", "ODYSSEY", 0.71,
        ["entangled", "cold-sparse", "grain", "drift"],
        "Single grains drift through Odyssey wavetable space — cloud becomes singular trajectory."
    ),
    (
        "Lavender Void",
        ["OPAL", "ODYSSEY"],
        dict(brightness=0.10, warmth=0.06, movement=0.35, density=0.09, space=0.93, aggression=0.07),
        dict(CHARACTER=0.18, MOVEMENT=0.35, COUPLING=0.67, SPACE=0.93),
        "TIMBRE_BLEND", "ODYSSEY", "OPAL", 0.67,
        ["entangled", "cold-sparse", "lavender", "void"],
        "Violet and lavender bleed into each other across a vast empty spectrum."
    ),
    (
        "Cold Granularity",
        ["OPAL", "ODYSSEY"],
        dict(brightness=0.06, warmth=0.04, movement=0.20, density=0.07, space=0.96, aggression=0.04),
        dict(CHARACTER=0.10, MOVEMENT=0.20, COUPLING=0.82, SPACE=0.96),
        "ENVELOPE_LINK", "ODYSSEY", "OPAL", 0.82,
        ["entangled", "cold-sparse", "cold", "granular"],
        "Odyssey envelope gates grain density — few grains emerge; most remain frozen."
    ),
    (
        "Spectral Suspension",
        ["OPAL", "ODYSSEY"],
        dict(brightness=0.13, warmth=0.07, movement=0.60, density=0.15, space=0.88, aggression=0.10),
        dict(CHARACTER=0.20, MOVEMENT=0.60, COUPLING=0.63, SPACE=0.88),
        "PITCH_SYNC", "OPAL", "ODYSSEY", 0.63,
        ["entangled", "cold-sparse", "spectral", "suspension"],
        "Grain position synced to wavetable cycle — two sparse voices locked in cold suspension."
    ),
    (
        "Frozen Scatter",
        ["OPAL", "ODYSSEY"],
        dict(brightness=0.08, warmth=0.02, movement=0.27, density=0.08, space=0.95, aggression=0.05),
        dict(CHARACTER=0.11, MOVEMENT=0.27, COUPLING=0.76, SPACE=0.95),
        "SPATIAL_COUPLE", "OPAL", "ODYSSEY", 0.76,
        ["entangled", "cold-sparse", "frozen", "scatter"],
        "Spatially coupled grains and wavetable — each lives at its own frozen coordinate."
    ),

    # ── ORIGAMI × ODYSSEY ───────────────────────────────────────────────
    (
        "Paper Drift",
        ["ORIGAMI", "ODYSSEY"],
        dict(brightness=0.09, warmth=0.08, movement=0.42, density=0.12, space=0.91, aggression=0.07),
        dict(CHARACTER=0.17, MOVEMENT=0.42, COUPLING=0.69, SPACE=0.91),
        "SPECTRAL_MORPH", "ORIGAMI", "ODYSSEY", 0.69,
        ["entangled", "cold-sparse", "paper", "drift"],
        "A paper fold drifts across wavetable space — folded harmonics dispersing on cold air."
    ),
    (
        "Vermillion Void",
        ["ORIGAMI", "ODYSSEY"],
        dict(brightness=0.07, warmth=0.05, movement=0.22, density=0.07, space=0.94, aggression=0.04),
        dict(CHARACTER=0.11, MOVEMENT=0.22, COUPLING=0.77, SPACE=0.94),
        "TIMBRE_BLEND", "ODYSSEY", "ORIGAMI", 0.77,
        ["entangled", "cold-sparse", "vermillion", "void"],
        "Odyssey's violet cools Origami's vermillion — two bright tones become cold grey."
    ),
    (
        "Crease Epoch",
        ["ORIGAMI", "ODYSSEY"],
        dict(brightness=0.05, warmth=0.03, movement=0.15, density=0.05, space=0.97, aggression=0.03),
        dict(CHARACTER=0.08, MOVEMENT=0.15, COUPLING=0.85, SPACE=0.97),
        "ENVELOPE_LINK", "ORIGAMI", "ODYSSEY", 0.85,
        ["entangled", "cold-sparse", "crease", "epoch"],
        "A fold that takes forever — envelope-linked to geological drift, near-static cold."
    ),
    (
        "Fold Frequency",
        ["ORIGAMI", "ODYSSEY"],
        dict(brightness=0.12, warmth=0.07, movement=0.52, density=0.14, space=0.89, aggression=0.09),
        dict(CHARACTER=0.19, MOVEMENT=0.52, COUPLING=0.65, SPACE=0.89),
        "FREQUENCY_SHIFT", "ORIGAMI", "ODYSSEY", 0.65,
        ["entangled", "cold-sparse", "fold", "frequency"],
        "Origami shifts Odyssey's frequency at each crease — partials scattered by paper logic."
    ),
    (
        "Angular Drift",
        ["ORIGAMI", "ODYSSEY"],
        dict(brightness=0.10, warmth=0.06, movement=0.38, density=0.10, space=0.92, aggression=0.06),
        dict(CHARACTER=0.16, MOVEMENT=0.38, COUPLING=0.72, SPACE=0.92),
        "PITCH_SYNC", "ODYSSEY", "ORIGAMI", 0.72,
        ["entangled", "cold-sparse", "angular", "drift"],
        "Odyssey pitch-syncs Origami's fold angles — geometry and frequency align in cold space."
    ),

    # ── ORACLE × ORIGAMI ────────────────────────────────────────────────
    (
        "Prophetic Fold",
        ["ORACLE", "ORIGAMI"],
        dict(brightness=0.08, warmth=0.10, movement=0.30, density=0.10, space=0.93, aggression=0.06),
        dict(CHARACTER=0.15, MOVEMENT=0.30, COUPLING=0.74, SPACE=0.93),
        "SPECTRAL_MORPH", "ORACLE", "ORIGAMI", 0.74,
        ["entangled", "cold-sparse", "prophecy", "fold"],
        "A prophecy folded into paper — GENDY breakpoints and paper harmonics share one cold voice."
    ),
    (
        "Maqam Crease",
        ["ORACLE", "ORIGAMI"],
        dict(brightness=0.11, warmth=0.05, movement=0.44, density=0.13, space=0.90, aggression=0.08),
        dict(CHARACTER=0.19, MOVEMENT=0.44, COUPLING=0.68, SPACE=0.90),
        "TIMBRE_BLEND", "ORIGAMI", "ORACLE", 0.68,
        ["entangled", "cold-sparse", "maqam", "crease"],
        "Maqam intervals folded along Origami creases — microtonal architecture in paper."
    ),
    (
        "Written in Ice",
        ["ORACLE", "ORIGAMI"],
        dict(brightness=0.05, warmth=0.04, movement=0.16, density=0.05, space=0.97, aggression=0.03),
        dict(CHARACTER=0.08, MOVEMENT=0.16, COUPLING=0.86, SPACE=0.97),
        "ENVELOPE_LINK", "ORACLE", "ORIGAMI", 0.86,
        ["entangled", "cold-sparse", "ice", "prophecy"],
        "Oracle controls Origami's envelope — the prophecy is written in ice that melts slowly."
    ),
    (
        "Indigo Fold",
        ["ORACLE", "ORIGAMI"],
        dict(brightness=0.14, warmth=0.07, movement=0.56, density=0.16, space=0.87, aggression=0.11),
        dict(CHARACTER=0.21, MOVEMENT=0.56, COUPLING=0.63, SPACE=0.87),
        "FREQUENCY_SHIFT", "ORIGAMI", "ORACLE", 0.63,
        ["entangled", "cold-sparse", "indigo", "fold"],
        "Origami shifts Oracle's prophetic frequencies at each crease — the oracle speaks in folds."
    ),
    (
        "Oracle of Paper",
        ["ORACLE", "ORIGAMI"],
        dict(brightness=0.09, warmth=0.06, movement=0.28, density=0.08, space=0.95, aggression=0.05),
        dict(CHARACTER=0.13, MOVEMENT=0.28, COUPLING=0.79, SPACE=0.95),
        "PITCH_SYNC", "ORIGAMI", "ORACLE", 0.79,
        ["entangled", "cold-sparse", "oracle", "paper"],
        "Paper geometry anchors prophecy pitch — the oracle cannot deviate from the crease."
    ),

    # ── ODYSSEY × OBSCURA ───────────────────────────────────────────────
    (
        "Drift Plate",
        ["ODYSSEY", "OBSCURA"],
        dict(brightness=0.07, warmth=0.11, movement=0.40, density=0.11, space=0.92, aggression=0.07),
        dict(CHARACTER=0.16, MOVEMENT=0.40, COUPLING=0.70, SPACE=0.92),
        "SPECTRAL_MORPH", "ODYSSEY", "OBSCURA", 0.70,
        ["entangled", "cold-sparse", "drift", "plate"],
        "Odyssey drift morphs through Obscura's silver resonance — a memory of motion on glass."
    ),
    (
        "Slow Exposure",
        ["ODYSSEY", "OBSCURA"],
        dict(brightness=0.10, warmth=0.06, movement=0.24, density=0.09, space=0.94, aggression=0.06),
        dict(CHARACTER=0.14, MOVEMENT=0.24, COUPLING=0.76, SPACE=0.94),
        "TIMBRE_BLEND", "OBSCURA", "ODYSSEY", 0.76,
        ["entangled", "cold-sparse", "exposure", "slow"],
        "A long-exposure photograph of a cold drift — blurred with silver chemistry."
    ),
    (
        "Tonal Emulsion",
        ["ODYSSEY", "OBSCURA"],
        dict(brightness=0.06, warmth=0.04, movement=0.18, density=0.07, space=0.96, aggression=0.04),
        dict(CHARACTER=0.10, MOVEMENT=0.18, COUPLING=0.82, SPACE=0.96),
        "ENVELOPE_LINK", "OBSCURA", "ODYSSEY", 0.82,
        ["entangled", "cold-sparse", "tonal", "emulsion"],
        "Obscura's decay controls Odyssey's envelope — the image fades before it can form."
    ),
    (
        "Violet Silver",
        ["ODYSSEY", "OBSCURA"],
        dict(brightness=0.13, warmth=0.07, movement=0.58, density=0.15, space=0.88, aggression=0.10),
        dict(CHARACTER=0.20, MOVEMENT=0.58, COUPLING=0.64, SPACE=0.88),
        "FREQUENCY_SHIFT", "OBSCURA", "ODYSSEY", 0.64,
        ["entangled", "cold-sparse", "violet", "silver"],
        "Obscura silver chemistry shifts Odyssey's violet frequencies — cold tonal distortion."
    ),
    (
        "Frozen Plate",
        ["ODYSSEY", "OBSCURA"],
        dict(brightness=0.08, warmth=0.03, movement=0.22, density=0.08, space=0.95, aggression=0.05),
        dict(CHARACTER=0.11, MOVEMENT=0.22, COUPLING=0.79, SPACE=0.95),
        "SPATIAL_COUPLE", "ODYSSEY", "OBSCURA", 0.79,
        ["entangled", "cold-sparse", "frozen", "plate"],
        "Spatially coupled wavetable and plate resonance — two planes in frozen co-existence."
    ),

    # ── OPAL × ORACLE ───────────────────────────────────────────────────
    (
        "Prophetic Grain",
        ["OPAL", "ORACLE"],
        dict(brightness=0.09, warmth=0.10, movement=0.45, density=0.12, space=0.91, aggression=0.07),
        dict(CHARACTER=0.17, MOVEMENT=0.45, COUPLING=0.69, SPACE=0.91),
        "SPECTRAL_MORPH", "ORACLE", "OPAL", 0.69,
        ["entangled", "cold-sparse", "prophecy", "grain"],
        "Oracle breakpoints scatter Opal grains — each grain lands on a prophesied frequency."
    ),
    (
        "Maqam Grain",
        ["OPAL", "ORACLE"],
        dict(brightness=0.07, warmth=0.07, movement=0.28, density=0.09, space=0.93, aggression=0.05),
        dict(CHARACTER=0.13, MOVEMENT=0.28, COUPLING=0.75, SPACE=0.93),
        "PITCH_SYNC", "ORACLE", "OPAL", 0.75,
        ["entangled", "cold-sparse", "maqam", "grain"],
        "Maqam intervals as grain positions — ancient intervals suspend in cold space."
    ),
    (
        "Granular Prophecy",
        ["OPAL", "ORACLE"],
        dict(brightness=0.05, warmth=0.05, movement=0.14, density=0.06, space=0.96, aggression=0.03),
        dict(CHARACTER=0.09, MOVEMENT=0.14, COUPLING=0.83, SPACE=0.96),
        "ENVELOPE_LINK", "OPAL", "ORACLE", 0.83,
        ["entangled", "cold-sparse", "granular", "prophecy"],
        "Grain envelope controls Oracle's articulation — the prophecy breathes in grain time."
    ),
    (
        "Cold Oracle",
        ["OPAL", "ORACLE"],
        dict(brightness=0.12, warmth=0.06, movement=0.52, density=0.14, space=0.89, aggression=0.09),
        dict(CHARACTER=0.19, MOVEMENT=0.52, COUPLING=0.66, SPACE=0.89),
        "TIMBRE_BLEND", "OPAL", "ORACLE", 0.66,
        ["entangled", "cold-sparse", "cold", "oracle"],
        "Lavender grain and prophecy indigo blend — a cold oracle speaking in scattered light."
    ),
    (
        "Sparse Divination",
        ["OPAL", "ORACLE"],
        dict(brightness=0.08, warmth=0.03, movement=0.32, density=0.08, space=0.95, aggression=0.05),
        dict(CHARACTER=0.12, MOVEMENT=0.32, COUPLING=0.78, SPACE=0.95),
        "FREQUENCY_SHIFT", "ORACLE", "OPAL", 0.78,
        ["entangled", "cold-sparse", "divination", "sparse"],
        "Oracle frequency-shifts grain positions — divination through spectral scarcity."
    ),

    # ── ORIGAMI × ORACLE ────────────────────────────────────────────────
    (
        "Folded Prophecy",
        ["ORIGAMI", "ORACLE"],
        dict(brightness=0.08, warmth=0.09, movement=0.34, density=0.10, space=0.92, aggression=0.06),
        dict(CHARACTER=0.15, MOVEMENT=0.34, COUPLING=0.72, SPACE=0.92),
        "SPECTRAL_MORPH", "ORIGAMI", "ORACLE", 0.72,
        ["entangled", "cold-sparse", "fold", "prophecy"],
        "Each crease reveals a new prophetic partial — Origami and Oracle unfold together in cold air."
    ),
    (
        "Paper Oracle",
        ["ORIGAMI", "ORACLE"],
        dict(brightness=0.11, warmth=0.05, movement=0.46, density=0.13, space=0.90, aggression=0.08),
        dict(CHARACTER=0.18, MOVEMENT=0.46, COUPLING=0.67, SPACE=0.90),
        "TIMBRE_BLEND", "ORACLE", "ORIGAMI", 0.67,
        ["entangled", "cold-sparse", "paper", "oracle"],
        "Oracle's timbre blends with Origami paper harmonics — prophecy written in folded sheets."
    ),
    (
        "Cold Scripture",
        ["ORIGAMI", "ORACLE"],
        dict(brightness=0.05, warmth=0.04, movement=0.12, density=0.05, space=0.97, aggression=0.03),
        dict(CHARACTER=0.08, MOVEMENT=0.12, COUPLING=0.86, SPACE=0.97),
        "ENVELOPE_LINK", "ORACLE", "ORIGAMI", 0.86,
        ["entangled", "cold-sparse", "scripture", "cold"],
        "Oracle's envelope is the scripture — Origami folds along prescribed rhythms of silence."
    ),
    (
        "Geometric Prophecy",
        ["ORIGAMI", "ORACLE"],
        dict(brightness=0.14, warmth=0.07, movement=0.58, density=0.16, space=0.87, aggression=0.11),
        dict(CHARACTER=0.21, MOVEMENT=0.58, COUPLING=0.63, SPACE=0.87),
        "FREQUENCY_SHIFT", "ORIGAMI", "ORACLE", 0.63,
        ["entangled", "cold-sparse", "geometric", "prophecy"],
        "Origami fold geometry shifts Oracle's prophecy frequencies — the shape of cold knowledge."
    ),
    (
        "Crease Divination",
        ["ORIGAMI", "ORACLE"],
        dict(brightness=0.09, warmth=0.06, movement=0.26, density=0.08, space=0.95, aggression=0.05),
        dict(CHARACTER=0.12, MOVEMENT=0.26, COUPLING=0.79, SPACE=0.95),
        "PITCH_SYNC", "ORACLE", "ORIGAMI", 0.79,
        ["entangled", "cold-sparse", "crease", "divination"],
        "Oracle pitch-locks Origami's crease angles — divination through paper geometry."
    ),

    # ── OBSIDIAN × ORACLE ───────────────────────────────────────────────
    (
        "Crystal Prophecy",
        ["OBSIDIAN", "ORACLE"],
        dict(brightness=0.07, warmth=0.07, movement=0.28, density=0.09, space=0.93, aggression=0.05),
        dict(CHARACTER=0.13, MOVEMENT=0.28, COUPLING=0.74, SPACE=0.93),
        "SPECTRAL_MORPH", "OBSIDIAN", "ORACLE", 0.74,
        ["entangled", "cold-sparse", "crystal", "prophecy"],
        "Crystal harmonics morph into prophetic breakpoints — cold clarity distilled to augury."
    ),
    (
        "White Oracle",
        ["OBSIDIAN", "ORACLE"],
        dict(brightness=0.10, warmth=0.05, movement=0.42, density=0.12, space=0.91, aggression=0.07),
        dict(CHARACTER=0.17, MOVEMENT=0.42, COUPLING=0.69, SPACE=0.91),
        "TIMBRE_BLEND", "ORACLE", "OBSIDIAN", 0.69,
        ["entangled", "cold-sparse", "white", "oracle"],
        "Oracle indigo bleeds into Obsidian white — the prophecy crystallises as it speaks."
    ),
    (
        "Frozen Oracle",
        ["OBSIDIAN", "ORACLE"],
        dict(brightness=0.05, warmth=0.03, movement=0.14, density=0.05, space=0.97, aggression=0.03),
        dict(CHARACTER=0.08, MOVEMENT=0.14, COUPLING=0.85, SPACE=0.97),
        "ENVELOPE_LINK", "OBSIDIAN", "ORACLE", 0.85,
        ["entangled", "cold-sparse", "frozen", "oracle"],
        "Obsidian's envelope freezes Oracle's timing — the prophecy delivered once, then silent."
    ),
    (
        "Lattice Oracle",
        ["OBSIDIAN", "ORACLE"],
        dict(brightness=0.13, warmth=0.08, movement=0.55, density=0.15, space=0.88, aggression=0.10),
        dict(CHARACTER=0.20, MOVEMENT=0.55, COUPLING=0.65, SPACE=0.88),
        "FREQUENCY_SHIFT", "ORACLE", "OBSIDIAN", 0.65,
        ["entangled", "cold-sparse", "lattice", "oracle"],
        "Oracle shifts Obsidian's crystal lattice frequencies — prophecy encoded in facets."
    ),
    (
        "Void Oracle",
        ["OBSIDIAN", "ORACLE"],
        dict(brightness=0.08, warmth=0.04, movement=0.24, density=0.08, space=0.95, aggression=0.05),
        dict(CHARACTER=0.11, MOVEMENT=0.24, COUPLING=0.79, SPACE=0.95),
        "PITCH_SYNC", "OBSIDIAN", "ORACLE", 0.79,
        ["entangled", "cold-sparse", "void", "oracle"],
        "Obsidian locks Oracle's pitch into crystalline void — absolute cold prophecy."
    ),

    # ── OBSCURA × ORIGAMI ───────────────────────────────────────────────
    (
        "Silver Fold",
        ["OBSCURA", "ORIGAMI"],
        dict(brightness=0.08, warmth=0.10, movement=0.36, density=0.11, space=0.92, aggression=0.07),
        dict(CHARACTER=0.16, MOVEMENT=0.36, COUPLING=0.71, SPACE=0.92),
        "SPECTRAL_MORPH", "OBSCURA", "ORIGAMI", 0.71,
        ["entangled", "cold-sparse", "silver", "fold"],
        "Silver plate resonance morphs into paper harmonics — photography and origami fuse cold."
    ),
    (
        "Emulsion Crease",
        ["OBSCURA", "ORIGAMI"],
        dict(brightness=0.11, warmth=0.06, movement=0.44, density=0.13, space=0.90, aggression=0.08),
        dict(CHARACTER=0.18, MOVEMENT=0.44, COUPLING=0.67, SPACE=0.90),
        "TIMBRE_BLEND", "ORIGAMI", "OBSCURA", 0.67,
        ["entangled", "cold-sparse", "emulsion", "crease"],
        "Origami paper timbres blend into Obscura's emulsion — both surfaces dissolve in cold light."
    ),
    (
        "Fold Exposure",
        ["OBSCURA", "ORIGAMI"],
        dict(brightness=0.06, warmth=0.04, movement=0.18, density=0.06, space=0.96, aggression=0.04),
        dict(CHARACTER=0.09, MOVEMENT=0.18, COUPLING=0.83, SPACE=0.96),
        "ENVELOPE_LINK", "OBSCURA", "ORIGAMI", 0.83,
        ["entangled", "cold-sparse", "fold", "exposure"],
        "Obscura's exposure duration controls Origami's fold speed — slow chemistry, cold geometry."
    ),
    (
        "Stiff Paper",
        ["OBSCURA", "ORIGAMI"],
        dict(brightness=0.14, warmth=0.07, movement=0.56, density=0.17, space=0.87, aggression=0.11),
        dict(CHARACTER=0.21, MOVEMENT=0.56, COUPLING=0.63, SPACE=0.87),
        "FREQUENCY_SHIFT", "ORIGAMI", "OBSCURA", 0.63,
        ["entangled", "cold-sparse", "stiff", "paper"],
        "Origami fold angles frequency-shift Obscura's string stiffness — paper becomes wire."
    ),
    (
        "Cyanotype Fold",
        ["OBSCURA", "ORIGAMI"],
        dict(brightness=0.09, warmth=0.05, movement=0.27, density=0.08, space=0.95, aggression=0.05),
        dict(CHARACTER=0.12, MOVEMENT=0.27, COUPLING=0.78, SPACE=0.95),
        "PITCH_SYNC", "OBSCURA", "ORIGAMI", 0.78,
        ["entangled", "cold-sparse", "cyanotype", "fold"],
        "Obscura pitch-locks Origami's crease angle — blue cold chemistry etched in paper."
    ),

    # ── OPAL × ORIGAMI ──────────────────────────────────────────────────
    (
        "Grain Fold",
        ["OPAL", "ORIGAMI"],
        dict(brightness=0.07, warmth=0.09, movement=0.38, density=0.10, space=0.93, aggression=0.06),
        dict(CHARACTER=0.15, MOVEMENT=0.38, COUPLING=0.72, SPACE=0.93),
        "SPECTRAL_MORPH", "OPAL", "ORIGAMI", 0.72,
        ["entangled", "cold-sparse", "grain", "fold"],
        "Grain clouds morphing through paper crease shapes — granular geometry in cold suspension."
    ),
    (
        "Lavender Crease",
        ["OPAL", "ORIGAMI"],
        dict(brightness=0.10, warmth=0.06, movement=0.46, density=0.13, space=0.90, aggression=0.08),
        dict(CHARACTER=0.18, MOVEMENT=0.46, COUPLING=0.68, SPACE=0.90),
        "TIMBRE_BLEND", "ORIGAMI", "OPAL", 0.68,
        ["entangled", "cold-sparse", "lavender", "crease"],
        "Origami paper timbres blend into Opal lavender grain — both cold and precise."
    ),
    (
        "Folded Cloud",
        ["OPAL", "ORIGAMI"],
        dict(brightness=0.05, warmth=0.03, movement=0.14, density=0.05, space=0.97, aggression=0.03),
        dict(CHARACTER=0.08, MOVEMENT=0.14, COUPLING=0.85, SPACE=0.97),
        "ENVELOPE_LINK", "ORIGAMI", "OPAL", 0.85,
        ["entangled", "cold-sparse", "folded", "cloud"],
        "Origami envelope shapes grain cloud density — the cloud folds into almost nothing."
    ),
    (
        "Crease Dispersion",
        ["OPAL", "ORIGAMI"],
        dict(brightness=0.13, warmth=0.07, movement=0.58, density=0.16, space=0.88, aggression=0.10),
        dict(CHARACTER=0.20, MOVEMENT=0.58, COUPLING=0.64, SPACE=0.88),
        "FREQUENCY_SHIFT", "OPAL", "ORIGAMI", 0.64,
        ["entangled", "cold-sparse", "crease", "dispersion"],
        "Opal frequency-shifts Origami partials along grain scatter — crease becomes spectrum."
    ),
    (
        "Paper Cloud",
        ["OPAL", "ORIGAMI"],
        dict(brightness=0.08, warmth=0.04, movement=0.25, density=0.08, space=0.95, aggression=0.05),
        dict(CHARACTER=0.11, MOVEMENT=0.25, COUPLING=0.77, SPACE=0.95),
        "SPATIAL_COUPLE", "ORIGAMI", "OPAL", 0.77,
        ["entangled", "cold-sparse", "paper", "cloud"],
        "Paper and grain cloud occupy spatially separated cold zones — neither intrudes."
    ),

    # ── ODYSSEY × ORIGAMI ───────────────────────────────────────────────
    (
        "Wavetable Fold",
        ["ODYSSEY", "ORIGAMI"],
        dict(brightness=0.08, warmth=0.09, movement=0.42, density=0.12, space=0.91, aggression=0.07),
        dict(CHARACTER=0.16, MOVEMENT=0.42, COUPLING=0.70, SPACE=0.91),
        "SPECTRAL_MORPH", "ODYSSEY", "ORIGAMI", 0.70,
        ["entangled", "cold-sparse", "wavetable", "fold"],
        "Odyssey wavetable morphs along Origami crease paths — spectral architecture in motion."
    ),
    (
        "Drift Crease",
        ["ODYSSEY", "ORIGAMI"],
        dict(brightness=0.06, warmth=0.05, movement=0.22, density=0.07, space=0.95, aggression=0.04),
        dict(CHARACTER=0.10, MOVEMENT=0.22, COUPLING=0.80, SPACE=0.95),
        "TIMBRE_BLEND", "ORIGAMI", "ODYSSEY", 0.80,
        ["entangled", "cold-sparse", "drift", "crease"],
        "Origami paper timbres blend into cold Odyssey drift — paper dissolves into wavetable space."
    ),
    (
        "Folded Epoch",
        ["ODYSSEY", "ORIGAMI"],
        dict(brightness=0.11, warmth=0.06, movement=0.50, density=0.14, space=0.89, aggression=0.09),
        dict(CHARACTER=0.18, MOVEMENT=0.50, COUPLING=0.66, SPACE=0.89),
        "ENVELOPE_LINK", "ODYSSEY", "ORIGAMI", 0.66,
        ["entangled", "cold-sparse", "folded", "epoch"],
        "Odyssey's drift envelope controls Origami fold rate — geological paper folding."
    ),
    (
        "Cold Geometry",
        ["ODYSSEY", "ORIGAMI"],
        dict(brightness=0.09, warmth=0.04, movement=0.30, density=0.09, space=0.94, aggression=0.06),
        dict(CHARACTER=0.14, MOVEMENT=0.30, COUPLING=0.73, SPACE=0.94),
        "PITCH_SYNC", "ORIGAMI", "ODYSSEY", 0.73,
        ["entangled", "cold-sparse", "cold", "geometry"],
        "Origami locks Odyssey's pitch to fold angles — synth geometry becomes paper geometry."
    ),
    (
        "Violet Crease",
        ["ODYSSEY", "ORIGAMI"],
        dict(brightness=0.05, warmth=0.03, movement=0.15, density=0.05, space=0.97, aggression=0.03),
        dict(CHARACTER=0.08, MOVEMENT=0.15, COUPLING=0.84, SPACE=0.97),
        "FREQUENCY_SHIFT", "ORIGAMI", "ODYSSEY", 0.84,
        ["entangled", "cold-sparse", "violet", "crease"],
        "Origami crease angles frequency-shift Odyssey's violet wavetable — cold angular drift."
    ),

    # ── OBSIDIAN × OPAL ─────────────────────────────────────────────────
    (
        "Crystal Grain",
        ["OBSIDIAN", "OPAL"],
        dict(brightness=0.07, warmth=0.08, movement=0.38, density=0.11, space=0.92, aggression=0.06),
        dict(CHARACTER=0.14, MOVEMENT=0.38, COUPLING=0.71, SPACE=0.92),
        "SPECTRAL_MORPH", "OBSIDIAN", "OPAL", 0.71,
        ["entangled", "cold-sparse", "crystal", "grain"],
        "Crystal harmonics morph into granular clouds — facets dissolving into suspended particles."
    ),
    (
        "White Grain",
        ["OBSIDIAN", "OPAL"],
        dict(brightness=0.10, warmth=0.05, movement=0.48, density=0.14, space=0.89, aggression=0.08),
        dict(CHARACTER=0.19, MOVEMENT=0.48, COUPLING=0.67, SPACE=0.89),
        "TIMBRE_BLEND", "OPAL", "OBSIDIAN", 0.67,
        ["entangled", "cold-sparse", "white", "grain"],
        "Opal lavender blends into Obsidian white — two pale cold timbres unite in vast space."
    ),
    (
        "Lattice Grain",
        ["OBSIDIAN", "OPAL"],
        dict(brightness=0.05, warmth=0.03, movement=0.16, density=0.05, space=0.97, aggression=0.03),
        dict(CHARACTER=0.08, MOVEMENT=0.16, COUPLING=0.84, SPACE=0.97),
        "ENVELOPE_LINK", "OBSIDIAN", "OPAL", 0.84,
        ["entangled", "cold-sparse", "lattice", "grain"],
        "Obsidian's crystal envelope gates grain emergence — grains born from lattice vacancies."
    ),
    (
        "Facet Dispersion",
        ["OBSIDIAN", "OPAL"],
        dict(brightness=0.13, warmth=0.07, movement=0.58, density=0.16, space=0.87, aggression=0.10),
        dict(CHARACTER=0.20, MOVEMENT=0.58, COUPLING=0.63, SPACE=0.87),
        "FREQUENCY_SHIFT", "OPAL", "OBSIDIAN", 0.63,
        ["entangled", "cold-sparse", "facet", "dispersion"],
        "Opal frequency-shifts Obsidian's crystal facets — each facet refracts a different grain."
    ),
    (
        "Cold Crystal",
        ["OBSIDIAN", "OPAL"],
        dict(brightness=0.08, warmth=0.04, movement=0.26, density=0.08, space=0.95, aggression=0.05),
        dict(CHARACTER=0.11, MOVEMENT=0.26, COUPLING=0.78, SPACE=0.95),
        "PITCH_SYNC", "OBSIDIAN", "OPAL", 0.78,
        ["entangled", "cold-sparse", "cold", "crystal"],
        "Obsidian pitch-syncs grain positions — crystal geometry determines grain coordinates."
    ),
]


# ---------------------------------------------------------------------------
# Writer
# ---------------------------------------------------------------------------

def make_preset(name, engines, dna, macros, coupling_type, source, target, amount, tags, description):
    engine_a, engine_b = engines
    # Build per-engine parameters using macros as proxy for all 4 macro keys
    def engine_params(char, movement, coupling, space):
        return {
            "macro_character": round(char, 3),
            "macro_movement":  round(movement, 3),
            "macro_coupling":  round(coupling, 3),
            "macro_space":     round(space, 3),
        }

    # Slight variation between engines to avoid identical macros
    params_a = engine_params(
        macros["CHARACTER"],
        macros["MOVEMENT"],
        macros["COUPLING"],
        macros["SPACE"],
    )
    params_b = engine_params(
        round(max(0.0, macros["CHARACTER"] - 0.03), 3),
        round(max(0.0, macros["MOVEMENT"] - 0.05), 3),
        round(min(1.0, macros["COUPLING"] + 0.05), 3),
        round(min(1.0, macros["SPACE"] + 0.03), 3),
    )

    return {
        "name": name,
        "version": "1.0",
        "mood": "Entangled",
        "engines": engines,
        "parameters": {
            engine_a: params_a,
            engine_b: params_b,
        },
        "coupling": {
            "type": coupling_type,
            "source": source,
            "target": target,
            "amount": amount,
        },
        "dna": {k: round(v, 3) for k, v in dna.items()},
        "macros": {k: round(v, 3) for k, v in macros.items()},
        "tags": tags,
        "description": description,
    }


def run():
    os.makedirs(PRESET_DIR, exist_ok=True)
    written = 0
    skipped = 0

    for entry in PRESETS:
        name, engines, dna, macros, coupling_type, source, target, amount, tags, description = entry
        preset = make_preset(name, engines, dna, macros, coupling_type, source, target, amount, tags, description)

        filename = name.replace(" ", "_").replace("/", "-") + ".xometa"
        filepath = os.path.join(PRESET_DIR, filename)

        if os.path.exists(filepath):
            print(f"  skip  {filename}")
            skipped += 1
            continue

        with open(filepath, "w") as f:
            json.dump(preset, f, indent=2)
        print(f"  write {filename}")
        written += 1

    print(f"\nDone: {written} written, {skipped} skipped.")
    print(f"Total presets attempted: {len(PRESETS)}")


if __name__ == "__main__":
    run()
