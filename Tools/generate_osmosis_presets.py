#!/usr/bin/env python3
"""
generate_osmosis_presets.py
Generates 15 additional Osmosis membrane configuration presets.

Categories:
  Coupling  (8) — membrane configs optimised for coupling with specific partner types
  Organic   (4) — natural/acoustic source analysis
  Deep      (3) — sub-bass focused membrane settings

Parameter semantics (from OsmosisEngine.h):
  osmo_permeability  [0-1]  wet/dry of LP membrane vs dry signal
  osmo_selectivity   [0-1]  membrane LP cutoff 200 Hz → 18.2 kHz
  osmo_reactivity    [0-1]  envelope attack 1–50 ms (high = fast, low = slow)
  osmo_memory        [0-1]  envelope release 10–1000 ms (high = long hold)

Output: ~/Documents/GitHub/XO_OX-XOmnibus/Presets/XOceanus/{Coupling,Organic,Deep}/Osmosis/
"""

import json
import os
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
PRESETS_ROOT = REPO_ROOT / "Presets" / "XOceanus"
TODAY = "2026-04-01"
AUTHOR = "XO_OX Designs"

# ---------------------------------------------------------------------------
# Preset definitions
# ---------------------------------------------------------------------------

PRESETS = [
    # ---- COUPLING (8) -------------------------------------------------------
    {
        "name": "Vocal Membrane",
        "mood": "Coupling",
        "description": (
            "Tuned for vocal coupling sources: high selectivity keeps the analysis band "
            "in the 1–4 kHz presence zone where vowel formants live. Medium reactivity "
            "tracks syllabic rhythm without smearing consonants. Memory long enough to "
            "bridge inter-syllable gaps. Use as Osmosis input when coupling voice to any "
            "melodic engine."
        ),
        "tags": ["coupling", "vocal", "formant", "syllabic", "coupling-source", "presence"],
        "couplingIntensity": "Moderate",
        "parameters": {
            "osmo_permeability": 0.62,
            "osmo_selectivity": 0.72,   # ~13 kHz LP — lets formant region dominate
            "osmo_reactivity": 0.65,    # attack ~18.5 ms — tracks syllables
            "osmo_memory": 0.42,        # release ~426 ms — bridges inter-syllable
        },
        "dna": {
            "brightness": 0.72, "warmth": 0.48, "movement": 0.62,
            "density": 0.45, "space": 0.55, "aggression": 0.28,
        },
    },
    {
        "name": "Drum Sieve",
        "mood": "Coupling",
        "description": (
            "Short attack, short memory — designed to track drum transients cleanly. "
            "High reactivity snaps to kick and snare hits; low memory drops quickly between "
            "beats so the coupling signal reflects rhythmic pulse rather than sustained "
            "envelope. Selectivity kept low to capture the body of the drum, not the air. "
            "Pairs with any engine where you want rhythmic amplitude modulation."
        ),
        "tags": ["coupling", "drums", "transient", "rhythmic", "percussive", "coupling-source"],
        "couplingIntensity": "Strong",
        "parameters": {
            "osmo_permeability": 0.78,
            "osmo_selectivity": 0.18,   # ~3.4 kHz — low-mid body of drums
            "osmo_reactivity": 0.90,    # attack ~5.5 ms — snaps to transients
            "osmo_memory": 0.08,        # release ~89 ms — drops fast between beats
        },
        "dna": {
            "brightness": 0.25, "warmth": 0.55, "movement": 0.88,
            "density": 0.65, "space": 0.25, "aggression": 0.78,
        },
    },
    {
        "name": "Spectral Gate",
        "mood": "Coupling",
        "description": (
            "Maximum selectivity pushes the membrane cutoff to near 18 kHz, letting only "
            "the brightest upper partials trigger coupling events. Use this when you need "
            "the coupled engine to respond exclusively to shimmer, air, and upper harmonic "
            "content — cymbal rides, string harmonics, vocal sibilance."
        ),
        "tags": ["coupling", "air", "shimmer", "high-frequency", "coupling-source", "spectral"],
        "couplingIntensity": "Moderate",
        "parameters": {
            "osmo_permeability": 0.55,
            "osmo_selectivity": 0.95,   # ~17.3 kHz — near-air band
            "osmo_reactivity": 0.75,    # attack ~13.5 ms — brisk
            "osmo_memory": 0.22,        # release ~228 ms — medium tail
        },
        "dna": {
            "brightness": 0.95, "warmth": 0.18, "movement": 0.72,
            "density": 0.28, "space": 0.75, "aggression": 0.32,
        },
    },
    {
        "name": "Synth Follower",
        "mood": "Coupling",
        "description": (
            "Designed to track synthesiser pads and sustained tones. Moderate selectivity "
            "covers the midrange swell. Slow reactivity smooths over filter sweeps and "
            "vibrato, extracting only the overall amplitude contour. Long memory sustains "
            "the coupling signal across the body of each note, creating stable sidechain "
            "pressure from held chords."
        ),
        "tags": ["coupling", "synth", "pads", "sustained", "smooth", "coupling-source"],
        "couplingIntensity": "Subtle",
        "parameters": {
            "osmo_permeability": 0.50,
            "osmo_selectivity": 0.52,   # ~9.6 kHz — mid-upper coverage
            "osmo_reactivity": 0.28,    # attack ~36 ms — smoothed over vibrato
            "osmo_memory": 0.72,        # release ~724 ms — holds through sustained notes
        },
        "dna": {
            "brightness": 0.52, "warmth": 0.60, "movement": 0.28,
            "density": 0.55, "space": 0.65, "aggression": 0.12,
        },
    },
    {
        "name": "Bass Coupling",
        "mood": "Coupling",
        "description": (
            "Low selectivity narrows the analysis window to the low-mid bass region "
            "(200–3 kHz effective). Tracks bass guitar and sub-synth groove patterns. "
            "Medium reactivity catches note attacks while low memory resets between "
            "rhythmic bass hits. The ideal Osmosis setting when coupling a bass line to "
            "a melodic or harmonic engine for rhythmic entrainment."
        ),
        "tags": ["coupling", "bass", "low-mid", "groove", "rhythmic", "coupling-source"],
        "couplingIntensity": "Moderate",
        "parameters": {
            "osmo_permeability": 0.70,
            "osmo_selectivity": 0.08,   # ~1.6 kHz — bass body band
            "osmo_reactivity": 0.55,    # attack ~23.5 ms — catches note attacks
            "osmo_memory": 0.15,        # release ~159 ms — resets between hits
        },
        "dna": {
            "brightness": 0.18, "warmth": 0.78, "movement": 0.58,
            "density": 0.72, "space": 0.35, "aggression": 0.55,
        },
    },
    {
        "name": "Guitar Tracking",
        "mood": "Coupling",
        "description": (
            "Tuned for electric guitar: selectivity centred around the upper-mid guitar "
            "presence peak (approx. 2–6 kHz effective). Reactivity fast enough to catch "
            "pick attacks and bends without over-tracking string noise. Memory moderate — "
            "sustains coupling between picked notes but resets on rests. Natural feel for "
            "guitar-driven generative coupling."
        ),
        "tags": ["coupling", "guitar", "electric", "pick-attack", "mid-presence", "coupling-source"],
        "couplingIntensity": "Moderate",
        "parameters": {
            "osmo_permeability": 0.60,
            "osmo_selectivity": 0.32,   # ~5.9 kHz — guitar presence zone
            "osmo_reactivity": 0.72,    # attack ~15 ms — tracks pick attacks
            "osmo_memory": 0.38,        # release ~385 ms — bridges sustain
        },
        "dna": {
            "brightness": 0.45, "warmth": 0.52, "movement": 0.70,
            "density": 0.48, "space": 0.42, "aggression": 0.52,
        },
    },
    {
        "name": "Piano Resonance",
        "mood": "Coupling",
        "description": (
            "Captures the characteristic resonance tail of piano: medium selectivity "
            "focuses on the 2–8 kHz string tone, moderate reactivity for clean note "
            "detection, long memory mimics the natural sustain decay. The coupled engine "
            "feels the piano's full resonant shape — not just the hammer transient."
        ),
        "tags": ["coupling", "piano", "acoustic", "resonance", "sustain", "coupling-source"],
        "couplingIntensity": "Subtle",
        "parameters": {
            "osmo_permeability": 0.45,
            "osmo_selectivity": 0.45,   # ~8.3 kHz — string tone region
            "osmo_reactivity": 0.60,    # attack ~21 ms — note detection
            "osmo_memory": 0.82,        # release ~820 ms — mimics sustain tail
        },
        "dna": {
            "brightness": 0.55, "warmth": 0.58, "movement": 0.40,
            "density": 0.45, "space": 0.70, "aggression": 0.08,
        },
    },
    {
        "name": "Crowd Pulse",
        "mood": "Coupling",
        "description": (
            "Designed for dense, complex audio sources — crowd noise, dense ensemble "
            "recordings, full mixes. Low selectivity captures the full-spectrum energy "
            "mass. Low reactivity smooths over micro-variations, while maximum memory "
            "tracks the global energy arc across bars rather than individual events. "
            "Use when you want a coupled engine to ride the overall intensity envelope "
            "of a complete scene."
        ),
        "tags": ["coupling", "crowd", "full-mix", "dense", "macro-envelope", "coupling-source"],
        "couplingIntensity": "Subtle",
        "parameters": {
            "osmo_permeability": 0.35,
            "osmo_selectivity": 0.25,   # ~4.7 kHz — broad coverage
            "osmo_reactivity": 0.15,    # attack ~43.5 ms — smoothed
            "osmo_memory": 0.98,        # release ~990 ms — global arc
        },
        "dna": {
            "brightness": 0.38, "warmth": 0.62, "movement": 0.18,
            "density": 0.88, "space": 0.55, "aggression": 0.12,
        },
    },

    # ---- ORGANIC (4) --------------------------------------------------------
    {
        "name": "Breath Trace",
        "mood": "Organic",
        "description": (
            "Extracts the biological rhythm of breath from wind instruments and voice. "
            "Slow reactivity smooths over pitch wobble and vibrato, focusing on the "
            "inhale/exhale cycle. Long memory sustains the measured breath arc. Moderate "
            "selectivity targets the fundamental and lower partials. The coupled signal "
            "rises and falls with the performer's lungs."
        ),
        "tags": ["organic", "breath", "wind", "voice", "biological", "natural"],
        "couplingIntensity": "Subtle",
        "parameters": {
            "osmo_permeability": 0.52,
            "osmo_selectivity": 0.35,   # ~6.5 kHz — fundamental + lower harmonics
            "osmo_reactivity": 0.20,    # attack ~41 ms — ignores vibrato fluctuation
            "osmo_memory": 0.88,        # release ~882 ms — holds breath arc
        },
        "dna": {
            "brightness": 0.38, "warmth": 0.72, "movement": 0.22,
            "density": 0.38, "space": 0.75, "aggression": 0.05,
        },
    },
    {
        "name": "Acoustic Room",
        "mood": "Organic",
        "description": (
            "Captures the ambient energy of a naturally reverberant acoustic space. "
            "High selectivity attenuates direct signal, favouring upper-frequency room "
            "reflections. Very slow reactivity and maximum memory mean the membrane "
            "holds the room's collective decay energy long after individual transients "
            "have passed. Use with room recordings, orchestral sources, or drum rooms."
        ),
        "tags": ["organic", "room", "reverb", "acoustic", "ambience", "decay"],
        "couplingIntensity": "Subtle",
        "parameters": {
            "osmo_permeability": 0.40,
            "osmo_selectivity": 0.80,   # ~14.6 kHz — room reflections band
            "osmo_reactivity": 0.10,    # attack ~46 ms — ignores direct transients
            "osmo_memory": 0.92,        # release ~921 ms — holds room decay
        },
        "dna": {
            "brightness": 0.65, "warmth": 0.55, "movement": 0.12,
            "density": 0.42, "space": 0.92, "aggression": 0.05,
        },
    },
    {
        "name": "String Grain",
        "mood": "Organic",
        "description": (
            "Reads the micro-texture of bowed string instruments — the rosin grain, "
            "bow pressure fluctuations, and harmonic shimmer. Medium-high selectivity "
            "targets the upper partials where string character lives. Fast reactivity "
            "captures bow-change transients. Medium memory bridges the arc between "
            "bowing strokes without losing the detail of each."
        ),
        "tags": ["organic", "strings", "bowed", "rosin", "grain", "texture", "natural"],
        "couplingIntensity": "Moderate",
        "parameters": {
            "osmo_permeability": 0.58,
            "osmo_selectivity": 0.65,   # ~11.9 kHz — upper string partials
            "osmo_reactivity": 0.80,    # attack ~11 ms — bow-change detection
            "osmo_memory": 0.50,        # release ~505 ms — bridges bowing strokes
        },
        "dna": {
            "brightness": 0.68, "warmth": 0.52, "movement": 0.62,
            "density": 0.42, "space": 0.60, "aggression": 0.30,
        },
    },
    {
        "name": "Field Recording",
        "mood": "Organic",
        "description": (
            "Designed for raw field recordings and environmental audio: wind, water, "
            "insects, traffic. Very low selectivity broadens the analysis band to capture "
            "the full spectral mass of complex natural soundscapes. Slow reactivity and "
            "long memory extract large-scale amplitude movement — the way a forest swells "
            "in wind or a rain shower peaks and decays."
        ),
        "tags": ["organic", "field-recording", "nature", "environment", "ambient", "spectral-mass"],
        "couplingIntensity": "Subtle",
        "parameters": {
            "osmo_permeability": 0.38,
            "osmo_selectivity": 0.12,   # ~2.2 kHz — broad spectral coverage
            "osmo_reactivity": 0.18,    # attack ~42 ms — smooth over micro-events
            "osmo_memory": 0.78,        # release ~782 ms — large-scale envelope
        },
        "dna": {
            "brightness": 0.30, "warmth": 0.68, "movement": 0.25,
            "density": 0.60, "space": 0.85, "aggression": 0.08,
        },
    },

    # ---- DEEP (3) -----------------------------------------------------------
    {
        "name": "Bass Osmosis",
        "mood": "Deep",
        "description": (
            "Sub-bass membrane locked to the lowest octave. Minimum selectivity keeps "
            "the analysis window centred just above the 200 Hz lower bound — capturing "
            "only fundamental sub energy. High permeability maximises the influence of "
            "the filtered signal. Medium reactivity catches bass note attacks; medium "
            "memory smooths between them. The engine at its most gravitational."
        ),
        "tags": ["deep", "sub-bass", "low-end", "fundamental", "gravitational", "sub"],
        "couplingIntensity": "Strong",
        "parameters": {
            "osmo_permeability": 0.85,
            "osmo_selectivity": 0.02,   # ~360 Hz — sub-bass window
            "osmo_reactivity": 0.55,    # attack ~23.5 ms — bass note attack
            "osmo_memory": 0.45,        # release ~456 ms — smooths between notes
        },
        "dna": {
            "brightness": 0.08, "warmth": 0.88, "movement": 0.45,
            "density": 0.85, "space": 0.38, "aggression": 0.60,
        },
    },
    {
        "name": "Infrasonic Hold",
        "mood": "Deep",
        "description": (
            "Extremely long memory combined with minimum selectivity creates a membrane "
            "that locks onto sub-bass energy and holds it as a sustained pressure for up "
            "to a full second. Slow reactivity prevents individual transients from "
            "disrupting the held mass. Use to generate a slow, tectonic coupling signal "
            "from any low-frequency source — perfect for ambient and experimental pairings "
            "where you want the sub-world to move in geological time."
        ),
        "tags": ["deep", "sub-bass", "infrasonic", "tectonic", "ambient", "slow", "pressure"],
        "couplingIntensity": "Subtle",
        "parameters": {
            "osmo_permeability": 0.72,
            "osmo_selectivity": 0.05,   # ~1.1 kHz — extended low coverage
            "osmo_reactivity": 0.12,    # attack ~45 ms — ignores transients
            "osmo_memory": 0.97,        # release ~971 ms — near-maximum hold
        },
        "dna": {
            "brightness": 0.05, "warmth": 0.85, "movement": 0.10,
            "density": 0.90, "space": 0.45, "aggression": 0.18,
        },
    },
    {
        "name": "Submerged Pulse",
        "mood": "Deep",
        "description": (
            "A rhythmic deep membrane: fast reactivity to snap to sub-bass kicks, "
            "but selectivity and memory tuned so that each hit reads as a distinct "
            "pressure event rather than a sustained tone. Medium permeability keeps "
            "the coupling signal punchy. The membrane surfaces briefly with each "
            "low-frequency transient, then sinks back. Like pressure waves felt "
            "through water."
        ),
        "tags": ["deep", "sub-bass", "rhythmic", "kick", "pulse", "pressure-wave"],
        "couplingIntensity": "Strong",
        "parameters": {
            "osmo_permeability": 0.75,
            "osmo_selectivity": 0.04,   # ~920 Hz — sub through low-mid
            "osmo_reactivity": 0.85,    # attack ~8.5 ms — snaps to kick transients
            "osmo_memory": 0.12,        # release ~129 ms — drops fast between hits
        },
        "dna": {
            "brightness": 0.12, "warmth": 0.72, "movement": 0.82,
            "density": 0.78, "space": 0.28, "aggression": 0.75,
        },
    },
]


# ---------------------------------------------------------------------------
# Generator
# ---------------------------------------------------------------------------

def build_preset(p: dict) -> dict:
    """Return a complete .xometa dict from a compact preset definition."""
    return {
        "schema_version": 1,
        "name": p["name"],
        "mood": p["mood"],
        "engines": ["Osmosis"],
        "author": AUTHOR,
        "version": "1.0.0",
        "description": p["description"],
        "tags": p["tags"],
        "macroLabels": ["PERMEABILITY", "SELECTIVITY", "REACTIVITY", "MEMORY"],
        "couplingIntensity": p["couplingIntensity"],
        "tempo": None,
        "created": TODAY,
        "parameters": {
            "Osmosis": {
                "osmo_permeability": p["parameters"]["osmo_permeability"],
                "osmo_selectivity":  p["parameters"]["osmo_selectivity"],
                "osmo_reactivity":   p["parameters"]["osmo_reactivity"],
                "osmo_memory":       p["parameters"]["osmo_memory"],
            }
        },
        "coupling": {"pairs": []},
        "dna": p["dna"],
    }


def filename_from_name(name: str) -> str:
    """Convert 'Vocal Membrane' → 'Vocal_Membrane.xometa'."""
    return name.replace(" ", "_") + ".xometa"


def write_presets(presets: list, dry_run: bool = False) -> list[Path]:
    written = []
    for p in presets:
        mood_dir = PRESETS_ROOT / p["mood"] / "Osmosis"
        mood_dir.mkdir(parents=True, exist_ok=True)
        dest = mood_dir / filename_from_name(p["name"])
        data = build_preset(p)
        if dry_run:
            print(f"[DRY RUN] Would write: {dest}")
        else:
            dest.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
            print(f"  wrote  {dest.relative_to(REPO_ROOT)}")
        written.append(dest)
    return written


def count_existing() -> int:
    return len(list((REPO_ROOT / "Presets").rglob("*Osmosis*/*.xometa")))


if __name__ == "__main__":
    import sys
    dry = "--dry-run" in sys.argv

    existing = count_existing()
    new_count = len(PRESETS)
    print(f"Osmosis presets — existing: {existing}, adding: {new_count}, total after: {existing + new_count}")
    print()

    by_mood: dict[str, int] = {}
    for p in PRESETS:
        by_mood[p["mood"]] = by_mood.get(p["mood"], 0) + 1
    for mood, n in sorted(by_mood.items()):
        print(f"  {mood:12s}  {n} preset(s)")
    print()

    written = write_presets(PRESETS, dry_run=dry)

    if not dry:
        print(f"\nDone. {len(written)} preset(s) written.")
        new_total = count_existing()
        print(f"Osmosis total now: {new_total}")
