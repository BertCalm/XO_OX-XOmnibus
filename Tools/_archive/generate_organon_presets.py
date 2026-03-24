#!/usr/bin/env python3
"""Generate 120 XOrganon factory presets for XOlokun."""

import json
import os

PRESET_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "Presets", "XOlokun")
DATE = "2026-03-10"

# Parameter ranges for reference:
# organon_metabolicRate: 0.1-10.0 (Hz)
# organon_enzymeSelect: 20-20000 (Hz)
# organon_catalystDrive: 0.0-2.0
# organon_dampingCoeff: 0.01-0.99
# organon_signalFlux: 0.0-1.0
# organon_phasonShift: 0.0-1.0
# organon_isotopeBalance: 0.0-1.0
# organon_lockIn: 0.0-1.0
# organon_membrane: 0.0-1.0
# organon_noiseColor: 0.0-1.0


def make_preset(name, mood, desc, tags, params, dna, coupling_intensity="None",
                tempo=None, coupling_pairs=None, engines=None):
    """Create a single .xometa preset dict."""
    if engines is None:
        engines = ["XOrganon"]
    preset = {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": engines,
        "author": "XO_OX",
        "version": "1.0.0",
        "description": desc,
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": coupling_intensity,
        "tempo": tempo,
        "created": DATE,
        "legacy": {
            "sourceInstrument": None,
            "sourceCategory": None,
            "sourcePresetName": None
        },
        "parameters": {
            "XOrganon": params
        },
        "coupling": {"pairs": coupling_pairs} if coupling_pairs else None,
        "sequencer": None,
        "dna": dna
    }
    return preset


def write_preset(preset):
    """Write a preset to the appropriate mood directory."""
    mood = preset["mood"]
    mood_dir = os.path.join(PRESET_DIR, mood)
    os.makedirs(mood_dir, exist_ok=True)
    filename = preset["name"].replace(" ", "_").replace("/", "-") + ".xometa"
    filepath = os.path.join(mood_dir, filename)
    with open(filepath, "w") as f:
        json.dump(preset, f, indent=2)
    return filepath


def p(mr=1.0, es=1000.0, cd=0.5, dc=0.3, sf=0.5, ps=0.0, ib=0.5, li=0.0, mb=0.2, nc=0.5):
    """Shorthand for parameter dict."""
    return {
        "organon_metabolicRate": mr,
        "organon_enzymeSelect": es,
        "organon_catalystDrive": cd,
        "organon_dampingCoeff": dc,
        "organon_signalFlux": sf,
        "organon_phasonShift": ps,
        "organon_isotopeBalance": ib,
        "organon_lockIn": li,
        "organon_membrane": mb,
        "organon_noiseColor": nc,
    }


def dna(b, w, m, d, s, a):
    """Shorthand for DNA dict."""
    return {"brightness": b, "warmth": w, "movement": m, "density": d, "space": s, "aggression": a}


# ============================================================================
# FOUNDATION (25 presets) — Bread-and-butter standalone pads
# ============================================================================
FOUNDATION = [
    ("Warm Start", "A warm, slowly evolving pad that breathes on its own. The first organism.",
     ["pad", "warm", "evolving", "organic", "starter"],
     p(mr=0.5, es=800.0, cd=0.4, dc=0.15, sf=0.6, ps=0.0, ib=0.35, li=0.0, mb=0.2, nc=0.4),
     dna(0.3, 0.7, 0.3, 0.3, 0.2, 0.1)),

    ("Deep Root", "Subterranean bass pad with slow metabolism and earth-heavy isotopes.",
     ["bass", "sub", "deep", "grounding"],
     p(mr=0.3, es=200.0, cd=0.6, dc=0.2, sf=0.7, ps=0.0, ib=0.1, li=0.0, mb=0.1, nc=0.2),
     dna(0.1, 0.8, 0.2, 0.5, 0.1, 0.2)),

    ("Morning Glow", "Gentle bright pad that wakes up slowly. Like sunrise through cells.",
     ["bright", "gentle", "morning", "pad"],
     p(mr=0.8, es=3000.0, cd=0.3, dc=0.12, sf=0.5, ps=0.0, ib=0.65, li=0.0, mb=0.3, nc=0.7),
     dna(0.65, 0.5, 0.25, 0.3, 0.3, 0.05)),

    ("Organ Bed", "Stable harmonic bed with organ-like sustain and warmth.",
     ["organ", "stable", "warm", "sustain"],
     p(mr=0.2, es=1200.0, cd=0.7, dc=0.08, sf=0.8, ps=0.0, ib=0.45, li=0.0, mb=0.15, nc=0.45),
     dna(0.4, 0.7, 0.1, 0.6, 0.15, 0.1)),

    ("Slow Pulse", "Barely perceptible breathing rhythm in a warm foundation.",
     ["pulse", "breath", "slow", "warm"],
     p(mr=1.2, es=900.0, cd=0.5, dc=0.18, sf=0.55, ps=0.3, ib=0.4, li=0.0, mb=0.2, nc=0.4),
     dna(0.35, 0.6, 0.4, 0.3, 0.2, 0.1)),

    ("Amber Membrane", "Rich mid-range pad with slight spectral shimmer.",
     ["amber", "mid", "shimmer", "pad"],
     p(mr=0.6, es=1500.0, cd=0.55, dc=0.14, sf=0.6, ps=0.1, ib=0.5, li=0.0, mb=0.25, nc=0.5),
     dna(0.45, 0.6, 0.3, 0.4, 0.25, 0.1)),

    ("Fossil Layer", "Ancient, sedimentary pad. Dense harmonics barely moving.",
     ["fossil", "ancient", "dense", "still"],
     p(mr=0.15, es=600.0, cd=0.8, dc=0.06, sf=0.9, ps=0.0, ib=0.25, li=0.0, mb=0.1, nc=0.3),
     dna(0.2, 0.8, 0.05, 0.8, 0.1, 0.15)),

    ("Cell Wall", "Firm, defined pad with clear harmonic edges.",
     ["firm", "defined", "clear", "structural"],
     p(mr=0.7, es=2000.0, cd=0.65, dc=0.25, sf=0.5, ps=0.0, ib=0.55, li=0.0, mb=0.15, nc=0.55),
     dna(0.5, 0.5, 0.15, 0.5, 0.15, 0.2)),

    ("Silt Bed", "Low, grainy foundation texture with organic movement.",
     ["grainy", "low", "texture", "organic"],
     p(mr=0.4, es=400.0, cd=0.5, dc=0.2, sf=0.65, ps=0.15, ib=0.2, li=0.0, mb=0.15, nc=0.35),
     dna(0.2, 0.65, 0.3, 0.45, 0.15, 0.2)),

    ("Photosynthesis", "Bright, energetic pad converting noise into harmonic light.",
     ["bright", "energetic", "converting", "living"],
     p(mr=1.5, es=4000.0, cd=0.6, dc=0.15, sf=0.7, ps=0.0, ib=0.7, li=0.0, mb=0.2, nc=0.75),
     dna(0.7, 0.4, 0.35, 0.4, 0.2, 0.15)),

    ("Humus", "Dark, fertile pad. Rich low-end with natural decay.",
     ["dark", "fertile", "rich", "bass"],
     p(mr=0.25, es=300.0, cd=0.7, dc=0.22, sf=0.75, ps=0.0, ib=0.12, li=0.0, mb=0.1, nc=0.2),
     dna(0.1, 0.85, 0.15, 0.6, 0.1, 0.2)),

    ("Quiet Colony", "Gentle colony of harmonics. Soft, communal texture.",
     ["soft", "gentle", "colony", "communal"],
     p(mr=0.5, es=1100.0, cd=0.35, dc=0.1, sf=0.5, ps=0.05, ib=0.45, li=0.0, mb=0.3, nc=0.45),
     dna(0.4, 0.6, 0.2, 0.35, 0.3, 0.05)),

    ("Bedrock", "Utterly stable sub-bass foundation. Immovable.",
     ["sub", "bass", "stable", "bedrock"],
     p(mr=0.1, es=150.0, cd=0.9, dc=0.05, sf=0.95, ps=0.0, ib=0.05, li=0.0, mb=0.05, nc=0.15),
     dna(0.05, 0.9, 0.02, 0.7, 0.05, 0.25)),

    ("Warm Broth", "Thick, nourishing mid-range. Comfort food for ears.",
     ["thick", "warm", "mid", "comfort"],
     p(mr=0.6, es=800.0, cd=0.55, dc=0.18, sf=0.65, ps=0.0, ib=0.38, li=0.0, mb=0.2, nc=0.4),
     dna(0.3, 0.75, 0.2, 0.5, 0.2, 0.1)),

    ("Mycelium Net", "Interconnected pad with subtle internal movement.",
     ["network", "connected", "subtle", "organic"],
     p(mr=0.8, es=1300.0, cd=0.45, dc=0.12, sf=0.55, ps=0.2, ib=0.48, li=0.0, mb=0.25, nc=0.5),
     dna(0.4, 0.55, 0.35, 0.4, 0.25, 0.08)),

    ("Pond Floor", "Still, deep pad with occasional harmonic bubbles.",
     ["still", "deep", "pond", "meditative"],
     p(mr=0.35, es=500.0, cd=0.5, dc=0.15, sf=0.6, ps=0.08, ib=0.22, li=0.0, mb=0.2, nc=0.3),
     dna(0.2, 0.7, 0.15, 0.4, 0.2, 0.05)),

    ("Bark Texture", "Rough, woody pad with natural grain.",
     ["rough", "woody", "texture", "natural"],
     p(mr=0.7, es=700.0, cd=0.65, dc=0.28, sf=0.6, ps=0.0, ib=0.3, li=0.0, mb=0.1, nc=0.35),
     dna(0.25, 0.65, 0.15, 0.55, 0.1, 0.3)),

    ("Placid Lake", "Mirror-smooth pad. Calm, reflective, spacious.",
     ["calm", "smooth", "reflective", "spacious"],
     p(mr=0.3, es=1800.0, cd=0.35, dc=0.08, sf=0.45, ps=0.0, ib=0.55, li=0.0, mb=0.45, nc=0.6),
     dna(0.5, 0.5, 0.1, 0.25, 0.45, 0.03)),

    ("Iron Core", "Heavy, metallic-tinged foundation with dense overtones.",
     ["heavy", "metallic", "dense", "industrial"],
     p(mr=1.0, es=2500.0, cd=0.85, dc=0.3, sf=0.7, ps=0.0, ib=0.6, li=0.0, mb=0.1, nc=0.6),
     dna(0.55, 0.4, 0.2, 0.7, 0.1, 0.45)),

    ("Lichen Pad", "Slow-growing texture that accumulates richness over time.",
     ["slow", "growing", "accumulating", "patient"],
     p(mr=0.2, es=1000.0, cd=0.4, dc=0.1, sf=0.5, ps=0.0, ib=0.4, li=0.0, mb=0.2, nc=0.45),
     dna(0.35, 0.6, 0.2, 0.35, 0.2, 0.05)),

    ("Thermal Vent", "Warm upwelling of harmonics from deep sub-bass.",
     ["thermal", "warm", "upwelling", "sub"],
     p(mr=0.5, es=350.0, cd=0.7, dc=0.15, sf=0.8, ps=0.1, ib=0.15, li=0.0, mb=0.15, nc=0.25),
     dna(0.15, 0.8, 0.3, 0.55, 0.15, 0.2)),

    ("Clean Signal", "Pure, unadorned harmonic pad. Crystal-clear foundation.",
     ["pure", "clean", "clear", "minimal"],
     p(mr=0.4, es=2200.0, cd=0.3, dc=0.1, sf=0.4, ps=0.0, ib=0.6, li=0.0, mb=0.1, nc=0.65),
     dna(0.6, 0.35, 0.1, 0.2, 0.1, 0.02)),

    ("Root Chord", "Musical foundation with clear harmonic identity.",
     ["musical", "harmonic", "chord", "tonal"],
     p(mr=0.6, es=1200.0, cd=0.55, dc=0.12, sf=0.6, ps=0.0, ib=0.5, li=0.0, mb=0.2, nc=0.5),
     dna(0.45, 0.6, 0.15, 0.45, 0.2, 0.1)),

    ("Slow Amber", "Thick, resinous pad that hardens over time.",
     ["amber", "thick", "resinous", "aging"],
     p(mr=0.15, es=700.0, cd=0.6, dc=0.08, sf=0.7, ps=0.0, ib=0.3, li=0.0, mb=0.15, nc=0.35),
     dna(0.25, 0.75, 0.1, 0.55, 0.15, 0.15)),

    ("Entropy Sink", "Enzyme selectivity low, high catalyst. Sub diet organism.",
     ["sub", "entropy", "dense", "consuming"],
     p(mr=0.8, es=250.0, cd=0.9, dc=0.25, sf=0.85, ps=0.0, ib=0.08, li=0.0, mb=0.1, nc=0.2),
     dna(0.1, 0.8, 0.15, 0.85, 0.1, 0.35)),
]

# ============================================================================
# ATMOSPHERE (25 presets) — Evolving ambient textures
# ============================================================================
ATMOSPHERE = [
    ("Living Cloud", "Continuously evolving atmospheric mass with gentle breathing.",
     ["evolving", "cloud", "atmospheric", "breathing"],
     p(mr=0.4, es=1500.0, cd=0.4, dc=0.08, sf=0.55, ps=0.15, ib=0.55, li=0.0, mb=0.5, nc=0.55),
     dna(0.5, 0.55, 0.4, 0.35, 0.5, 0.05)),

    ("Tidal Membrane", "Slow swelling pad that rises and falls like breathing ocean.",
     ["tidal", "swelling", "ocean", "pad"],
     p(mr=0.3, es=800.0, cd=0.5, dc=0.1, sf=0.6, ps=0.35, ib=0.4, li=0.0, mb=0.45, nc=0.4),
     dna(0.35, 0.6, 0.5, 0.4, 0.45, 0.08)),

    ("Spore Drift", "Delicate particles floating through harmonic space.",
     ["delicate", "floating", "particles", "ambient"],
     p(mr=1.2, es=5000.0, cd=0.25, dc=0.06, sf=0.35, ps=0.1, ib=0.75, li=0.0, mb=0.6, nc=0.8),
     dna(0.75, 0.3, 0.35, 0.2, 0.6, 0.02)),

    ("Deep Biome", "Vast subterranean atmosphere. Slow, dark, immersive.",
     ["deep", "vast", "dark", "immersive"],
     p(mr=0.2, es=400.0, cd=0.6, dc=0.1, sf=0.7, ps=0.2, ib=0.18, li=0.0, mb=0.4, nc=0.25),
     dna(0.15, 0.75, 0.35, 0.5, 0.4, 0.12)),

    ("Canopy Haze", "Dappled light filtering through dense harmonic foliage.",
     ["dappled", "filtered", "haze", "forest"],
     p(mr=0.7, es=2500.0, cd=0.4, dc=0.1, sf=0.5, ps=0.12, ib=0.6, li=0.0, mb=0.45, nc=0.65),
     dna(0.55, 0.5, 0.3, 0.35, 0.45, 0.05)),

    ("Plankton Bloom", "Shimmering upper-register atmosphere that multiplies slowly.",
     ["shimmer", "bright", "bloom", "multiplying"],
     p(mr=0.5, es=6000.0, cd=0.35, dc=0.07, sf=0.45, ps=0.08, ib=0.8, li=0.0, mb=0.55, nc=0.85),
     dna(0.8, 0.3, 0.3, 0.25, 0.55, 0.03)),

    ("Cavern Echo", "Resonant atmosphere with long reverberant decay.",
     ["cavern", "echo", "resonant", "reverberant"],
     p(mr=0.3, es=600.0, cd=0.55, dc=0.06, sf=0.6, ps=0.0, ib=0.3, li=0.0, mb=0.7, nc=0.35),
     dna(0.25, 0.65, 0.15, 0.45, 0.7, 0.1)),

    ("Thermal Layer", "Warm air mass with gradual spectral shifts.",
     ["thermal", "warm", "shifting", "gradual"],
     p(mr=0.6, es=1000.0, cd=0.45, dc=0.12, sf=0.55, ps=0.2, ib=0.45, li=0.0, mb=0.4, nc=0.45),
     dna(0.4, 0.65, 0.35, 0.35, 0.4, 0.08)),

    ("Fog Bank", "Dense, obscuring atmosphere. Sounds emerge and vanish.",
     ["fog", "dense", "obscuring", "mysterious"],
     p(mr=0.35, es=700.0, cd=0.5, dc=0.15, sf=0.7, ps=0.25, ib=0.3, li=0.0, mb=0.55, nc=0.35),
     dna(0.2, 0.7, 0.4, 0.5, 0.55, 0.08)),

    ("Aurora Wash", "Shimmering curtains of harmonic light overhead.",
     ["aurora", "shimmer", "curtains", "celestial"],
     p(mr=0.8, es=4000.0, cd=0.35, dc=0.08, sf=0.45, ps=0.15, ib=0.72, li=0.0, mb=0.6, nc=0.75),
     dna(0.7, 0.4, 0.4, 0.3, 0.6, 0.05)),

    ("Marsh Gas", "Bubbling, unstable low atmosphere with organic gurgle.",
     ["bubbling", "unstable", "gurgling", "organic"],
     p(mr=2.0, es=500.0, cd=0.6, dc=0.25, sf=0.75, ps=0.4, ib=0.2, li=0.0, mb=0.3, nc=0.3),
     dna(0.2, 0.6, 0.6, 0.5, 0.3, 0.25)),

    ("Stratosphere", "Thin, crystalline atmosphere at extreme altitude.",
     ["thin", "crystal", "high", "stratosphere"],
     p(mr=1.0, es=8000.0, cd=0.2, dc=0.05, sf=0.3, ps=0.0, ib=0.85, li=0.0, mb=0.65, nc=0.9),
     dna(0.85, 0.2, 0.2, 0.15, 0.65, 0.02)),

    ("Pollen Cloud", "Gentle, golden atmosphere with warm drifting motion.",
     ["golden", "pollen", "drifting", "warm"],
     p(mr=0.5, es=1800.0, cd=0.4, dc=0.1, sf=0.5, ps=0.18, ib=0.52, li=0.0, mb=0.4, nc=0.55),
     dna(0.5, 0.6, 0.35, 0.3, 0.4, 0.05)),

    ("Bioluminescence", "Glowing deep-sea atmosphere with cyan radiance.",
     ["glowing", "deep-sea", "bioluminescent", "radiant"],
     p(mr=0.6, es=2000.0, cd=0.5, dc=0.08, sf=0.55, ps=0.1, ib=0.58, li=0.0, mb=0.5, nc=0.6),
     dna(0.55, 0.5, 0.3, 0.4, 0.5, 0.08)),

    ("Dew Point", "Condensing atmosphere. Harmonics collect and drip.",
     ["condensing", "dripping", "delicate", "collecting"],
     p(mr=0.9, es=3500.0, cd=0.3, dc=0.15, sf=0.4, ps=0.05, ib=0.65, li=0.0, mb=0.45, nc=0.7),
     dna(0.6, 0.45, 0.25, 0.25, 0.45, 0.05)),

    ("Magma Chamber", "Slow, pressurized heat beneath the surface.",
     ["hot", "pressurized", "slow", "magma"],
     p(mr=0.2, es=300.0, cd=0.8, dc=0.08, sf=0.85, ps=0.1, ib=0.12, li=0.0, mb=0.2, nc=0.2),
     dna(0.1, 0.85, 0.25, 0.7, 0.2, 0.3)),

    ("Wind Organ", "Aeolian harmonics shaped by invisible breath.",
     ["wind", "aeolian", "breath", "harmonics"],
     p(mr=0.7, es=1600.0, cd=0.4, dc=0.12, sf=0.5, ps=0.3, ib=0.5, li=0.0, mb=0.4, nc=0.5),
     dna(0.45, 0.5, 0.45, 0.35, 0.4, 0.08)),

    ("Permafrost", "Frozen, crystallized atmosphere. Still but alive beneath.",
     ["frozen", "crystal", "still", "ice"],
     p(mr=0.15, es=3000.0, cd=0.35, dc=0.04, sf=0.4, ps=0.0, ib=0.7, li=0.0, mb=0.5, nc=0.7),
     dna(0.65, 0.3, 0.08, 0.3, 0.5, 0.03)),

    ("Oxygen Garden", "Bright, life-giving atmosphere. Clean and fresh.",
     ["bright", "fresh", "clean", "garden"],
     p(mr=0.8, es=3500.0, cd=0.4, dc=0.1, sf=0.55, ps=0.1, ib=0.68, li=0.0, mb=0.45, nc=0.7),
     dna(0.65, 0.45, 0.3, 0.3, 0.45, 0.05)),

    ("Abyssal Plain", "Total darkness. Harmonics emerge from nothing.",
     ["abyss", "dark", "emerging", "deep"],
     p(mr=0.15, es=200.0, cd=0.65, dc=0.08, sf=0.8, ps=0.15, ib=0.08, li=0.0, mb=0.35, nc=0.15),
     dna(0.05, 0.8, 0.3, 0.55, 0.35, 0.15)),

    ("Cirrus Veil", "Wispy, translucent atmosphere at extreme height.",
     ["wispy", "translucent", "high", "veil"],
     p(mr=0.9, es=7000.0, cd=0.2, dc=0.06, sf=0.3, ps=0.08, ib=0.82, li=0.0, mb=0.6, nc=0.85),
     dna(0.8, 0.25, 0.25, 0.15, 0.6, 0.02)),

    ("Compost Heat", "Decomposing warmth with organic microbial activity.",
     ["decomposing", "warm", "microbial", "organic"],
     p(mr=1.5, es=600.0, cd=0.6, dc=0.2, sf=0.7, ps=0.3, ib=0.25, li=0.0, mb=0.25, nc=0.3),
     dna(0.2, 0.7, 0.5, 0.55, 0.25, 0.2)),

    ("Night Bloom", "Flowers that open only in darkness. Slow harmonic unfurling.",
     ["night", "bloom", "unfurling", "dark"],
     p(mr=0.3, es=1200.0, cd=0.45, dc=0.08, sf=0.55, ps=0.12, ib=0.45, li=0.0, mb=0.45, nc=0.45),
     dna(0.4, 0.6, 0.3, 0.35, 0.45, 0.05)),

    ("Salt Flat", "Wide, flat, reflective atmosphere. Minimal but vast.",
     ["flat", "wide", "reflective", "minimal"],
     p(mr=0.4, es=2000.0, cd=0.3, dc=0.06, sf=0.4, ps=0.0, ib=0.55, li=0.0, mb=0.55, nc=0.6),
     dna(0.5, 0.4, 0.1, 0.2, 0.55, 0.03)),

    ("Monsoon Swell", "Building atmospheric pressure with rhythmic intensity.",
     ["building", "pressure", "monsoon", "rhythmic"],
     p(mr=1.0, es=1000.0, cd=0.6, dc=0.15, sf=0.7, ps=0.35, ib=0.4, li=0.0, mb=0.35, nc=0.4),
     dna(0.35, 0.6, 0.55, 0.5, 0.35, 0.18)),
]

# ============================================================================
# ENTANGLED (20 presets) — Coupling-focused
# ============================================================================
ENTANGLED = [
    ("Symbiotic Drone", "Pure reconstruction of input. The organism becomes what it eats.",
     ["symbiotic", "drone", "reconstruction", "coupled"],
     p(mr=0.3, es=1000.0, cd=0.3, dc=0.1, sf=1.0, ps=0.0, ib=0.5, li=0.0, mb=0.3, nc=0.5),
     dna(0.4, 0.5, 0.3, 0.6, 0.3, 0.1)),

    ("Hibernation Cycle", "Near-dormant organism that blooms only when fed.",
     ["hibernation", "dormant", "bloom", "coupled"],
     p(mr=0.15, es=800.0, cd=0.5, dc=0.08, sf=0.1, ps=0.0, ib=0.4, li=0.0, mb=0.4, nc=0.4),
     dna(0.3, 0.6, 0.2, 0.3, 0.4, 0.05)),

    ("Parasitic Bloom", "Aggressively feeds on input. High catalyst, high intake.",
     ["parasitic", "aggressive", "bloom", "feeding"],
     p(mr=2.0, es=1500.0, cd=1.5, dc=0.2, sf=1.0, ps=0.0, ib=0.55, li=0.0, mb=0.15, nc=0.5),
     dna(0.5, 0.4, 0.4, 0.7, 0.15, 0.5)),

    ("Echo Digestion", "Feeds on DUB delay tails. Recycled nutrient ecology.",
     ["echo", "recycled", "dub", "delay"],
     p(mr=0.6, es=1200.0, cd=0.5, dc=0.12, sf=0.8, ps=0.15, ib=0.45, li=0.0, mb=0.35, nc=0.45),
     dna(0.4, 0.55, 0.35, 0.45, 0.35, 0.1)),

    ("Impact Feeder", "Converts SNAP percussion into harmonic bloom bursts.",
     ["percussion", "impact", "burst", "snap"],
     p(mr=3.0, es=3000.0, cd=0.8, dc=0.3, sf=0.9, ps=0.0, ib=0.6, li=0.0, mb=0.2, nc=0.6),
     dna(0.55, 0.35, 0.5, 0.5, 0.2, 0.35)),

    ("Tidal Lock", "Rhythm-locked organism synced to partner's pulse.",
     ["locked", "rhythm", "sync", "tidal"],
     p(mr=1.0, es=1000.0, cd=0.5, dc=0.15, sf=0.7, ps=0.0, ib=0.45, li=0.8, mb=0.25, nc=0.45),
     dna(0.4, 0.5, 0.6, 0.45, 0.25, 0.15)),

    ("Spooky Action", "Entangled at distance. Subtle mutual influence.",
     ["entangled", "quantum", "subtle", "mutual"],
     p(mr=0.5, es=1500.0, cd=0.35, dc=0.1, sf=0.5, ps=0.1, ib=0.5, li=0.3, mb=0.4, nc=0.5),
     dna(0.45, 0.5, 0.3, 0.35, 0.4, 0.08)),

    ("Coral Reef", "Multiple small organisms responding to tidal coupling.",
     ["coral", "responsive", "tidal", "communal"],
     p(mr=0.8, es=2000.0, cd=0.45, dc=0.12, sf=0.7, ps=0.2, ib=0.55, li=0.5, mb=0.35, nc=0.55),
     dna(0.5, 0.5, 0.45, 0.45, 0.35, 0.1)),

    ("Metabolic Mirror", "Reflects the spectral character of its coupling partner.",
     ["mirror", "reflecting", "spectral", "adaptive"],
     p(mr=0.6, es=1200.0, cd=0.4, dc=0.1, sf=0.9, ps=0.0, ib=0.5, li=0.0, mb=0.3, nc=0.5),
     dna(0.45, 0.5, 0.25, 0.5, 0.3, 0.1)),

    ("Enzyme Cascade", "Chain reaction when fed. Rapid harmonic multiplication.",
     ["cascade", "chain", "rapid", "multiplying"],
     p(mr=4.0, es=2500.0, cd=1.2, dc=0.25, sf=0.85, ps=0.0, ib=0.65, li=0.0, mb=0.2, nc=0.6),
     dna(0.6, 0.35, 0.55, 0.65, 0.2, 0.4)),

    ("Gentle Mutualism", "Both engines benefit. Warm, cooperative texture.",
     ["mutual", "cooperative", "warm", "gentle"],
     p(mr=0.4, es=900.0, cd=0.4, dc=0.1, sf=0.6, ps=0.1, ib=0.42, li=0.2, mb=0.35, nc=0.42),
     dna(0.38, 0.6, 0.3, 0.4, 0.35, 0.05)),

    ("Predator Pulse", "Aggressive rhythmic feeding. Staccato energy extraction.",
     ["predator", "aggressive", "staccato", "rhythmic"],
     p(mr=5.0, es=2000.0, cd=1.0, dc=0.35, sf=0.95, ps=0.0, ib=0.5, li=0.7, mb=0.1, nc=0.5),
     dna(0.45, 0.35, 0.7, 0.55, 0.1, 0.55)),

    ("Lichen Bond", "Slow, patient coupling. Grows imperceptibly richer.",
     ["lichen", "patient", "slow", "growing"],
     p(mr=0.15, es=800.0, cd=0.35, dc=0.06, sf=0.5, ps=0.05, ib=0.4, li=0.1, mb=0.3, nc=0.4),
     dna(0.35, 0.6, 0.15, 0.35, 0.3, 0.05)),

    ("Osmotic Flow", "Gentle diffusion of energy between coupled engines.",
     ["osmotic", "diffusion", "gentle", "flow"],
     p(mr=0.5, es=1100.0, cd=0.4, dc=0.1, sf=0.65, ps=0.08, ib=0.48, li=0.15, mb=0.35, nc=0.48),
     dna(0.42, 0.55, 0.3, 0.4, 0.35, 0.08)),

    ("Nerve Impulse", "Fast-twitch coupling response. Immediate harmonic reaction.",
     ["nerve", "fast", "impulse", "reactive"],
     p(mr=8.0, es=4000.0, cd=0.9, dc=0.4, sf=0.9, ps=0.0, ib=0.7, li=0.0, mb=0.1, nc=0.7),
     dna(0.65, 0.25, 0.7, 0.5, 0.1, 0.45)),

    ("Commensal Hum", "Organism that lives alongside. Present but not intrusive.",
     ["commensal", "alongside", "subtle", "hum"],
     p(mr=0.4, es=700.0, cd=0.3, dc=0.1, sf=0.45, ps=0.0, ib=0.35, li=0.1, mb=0.25, nc=0.4),
     dna(0.3, 0.6, 0.2, 0.3, 0.25, 0.05)),

    ("Feedback Garden", "Self-reinforcing coupling loop. Harmonics bloom endlessly.",
     ["feedback", "loop", "blooming", "garden"],
     p(mr=0.8, es=1500.0, cd=0.6, dc=0.08, sf=0.8, ps=0.15, ib=0.55, li=0.0, mb=0.4, nc=0.55),
     dna(0.5, 0.5, 0.4, 0.55, 0.4, 0.15)),

    ("Cross Pollination", "Two engines exchanging spectral material.",
     ["cross", "exchange", "spectral", "pollination"],
     p(mr=0.6, es=1800.0, cd=0.5, dc=0.12, sf=0.7, ps=0.1, ib=0.55, li=0.2, mb=0.3, nc=0.55),
     dna(0.5, 0.5, 0.35, 0.45, 0.3, 0.1)),

    ("Host Response", "The organism's immune response to foreign audio input.",
     ["immune", "response", "foreign", "reactive"],
     p(mr=3.0, es=2000.0, cd=0.8, dc=0.25, sf=0.85, ps=0.2, ib=0.55, li=0.0, mb=0.2, nc=0.55),
     dna(0.5, 0.4, 0.5, 0.55, 0.2, 0.35)),

    ("Endosymbiont", "Lives inside another engine. Deeply integrated coupling.",
     ["endo", "deep", "integrated", "symbiont"],
     p(mr=0.5, es=1000.0, cd=0.5, dc=0.1, sf=0.9, ps=0.05, ib=0.48, li=0.4, mb=0.3, nc=0.48),
     dna(0.42, 0.55, 0.35, 0.55, 0.3, 0.12)),
]

# ============================================================================
# PRISM (20 presets) — Bright, crystalline, hyper-metabolic
# ============================================================================
PRISM = [
    ("Hyper-Metabolic Lead", "Blazing fast metabolism. Bright, aggressive, responsive.",
     ["lead", "bright", "fast", "aggressive"],
     p(mr=8.0, es=6000.0, cd=1.5, dc=0.4, sf=0.8, ps=0.0, ib=0.85, li=0.0, mb=0.1, nc=0.85),
     dna(0.85, 0.15, 0.6, 0.6, 0.1, 0.7)),

    ("Crystal Lattice", "Precise, geometric harmonics. Mathematical beauty.",
     ["crystal", "geometric", "precise", "mathematical"],
     p(mr=1.5, es=4000.0, cd=0.6, dc=0.2, sf=0.5, ps=0.0, ib=0.75, li=0.0, mb=0.2, nc=0.75),
     dna(0.75, 0.25, 0.2, 0.5, 0.2, 0.2)),

    ("Bioluminescent Key", "Melodic organism that glows when played.",
     ["melodic", "glowing", "key", "bioluminescent"],
     p(mr=2.0, es=3000.0, cd=0.5, dc=0.25, sf=0.6, ps=0.0, ib=0.65, li=0.0, mb=0.25, nc=0.65),
     dna(0.6, 0.4, 0.3, 0.4, 0.25, 0.2)),

    ("Nerve Synapse", "Fast-firing melodic lead with sharp attack.",
     ["synapse", "fast", "sharp", "lead"],
     p(mr=10.0, es=5000.0, cd=1.2, dc=0.5, sf=0.7, ps=0.0, ib=0.8, li=0.0, mb=0.05, nc=0.8),
     dna(0.8, 0.15, 0.5, 0.45, 0.05, 0.65)),

    ("Prism Split", "White noise split into spectral rainbow components.",
     ["prism", "spectral", "rainbow", "split"],
     p(mr=2.5, es=8000.0, cd=0.45, dc=0.15, sf=0.5, ps=0.0, ib=0.9, li=0.0, mb=0.3, nc=0.9),
     dna(0.9, 0.2, 0.3, 0.3, 0.3, 0.1)),

    ("Electric Jellyfish", "Pulsing, translucent lead with stinging overtones.",
     ["pulsing", "jellyfish", "electric", "stinging"],
     p(mr=3.0, es=3500.0, cd=0.7, dc=0.3, sf=0.65, ps=0.25, ib=0.7, li=0.0, mb=0.2, nc=0.7),
     dna(0.65, 0.3, 0.5, 0.45, 0.2, 0.35)),

    ("Diamond Core", "Hard, brilliant lead with extreme clarity.",
     ["diamond", "hard", "brilliant", "clear"],
     p(mr=5.0, es=7000.0, cd=0.9, dc=0.35, sf=0.6, ps=0.0, ib=0.88, li=0.0, mb=0.1, nc=0.88),
     dna(0.88, 0.1, 0.3, 0.4, 0.1, 0.45)),

    ("Firefly Swarm", "Rapid tiny harmonic bursts like a cloud of fireflies.",
     ["firefly", "swarm", "rapid", "bursts"],
     p(mr=6.0, es=4500.0, cd=0.5, dc=0.35, sf=0.55, ps=0.4, ib=0.72, li=0.0, mb=0.25, nc=0.72),
     dna(0.7, 0.3, 0.65, 0.35, 0.25, 0.2)),

    ("Catalyst Peak", "Maximum energy conversion. Pure harmonic intensity.",
     ["catalyst", "maximum", "intense", "peak"],
     p(mr=4.0, es=3000.0, cd=2.0, dc=0.3, sf=0.9, ps=0.0, ib=0.65, li=0.0, mb=0.1, nc=0.65),
     dna(0.6, 0.3, 0.4, 0.8, 0.1, 0.55)),

    ("Solar Flare", "Explosive harmonic eruption. Bright, violent, beautiful.",
     ["solar", "explosive", "bright", "eruption"],
     p(mr=9.0, es=5000.0, cd=1.8, dc=0.45, sf=0.85, ps=0.0, ib=0.82, li=0.0, mb=0.1, nc=0.82),
     dna(0.82, 0.15, 0.55, 0.65, 0.1, 0.7)),

    ("Mitosis Bell", "Bell-like tone that divides into overtone harmonics.",
     ["bell", "dividing", "overtone", "metallic"],
     p(mr=2.0, es=4000.0, cd=0.6, dc=0.2, sf=0.5, ps=0.0, ib=0.7, li=0.0, mb=0.3, nc=0.7),
     dna(0.7, 0.3, 0.2, 0.45, 0.3, 0.15)),

    ("Enzyme Blade", "Cutting lead with surgical precision.",
     ["cutting", "surgical", "blade", "lead"],
     p(mr=7.0, es=6000.0, cd=1.0, dc=0.45, sf=0.7, ps=0.0, ib=0.8, li=0.0, mb=0.05, nc=0.8),
     dna(0.8, 0.1, 0.4, 0.4, 0.05, 0.6)),

    ("Quicksilver", "Liquid, mercurial lead that shifts shape constantly.",
     ["liquid", "mercurial", "shifting", "lead"],
     p(mr=5.0, es=3500.0, cd=0.7, dc=0.3, sf=0.6, ps=0.3, ib=0.68, li=0.0, mb=0.15, nc=0.68),
     dna(0.65, 0.3, 0.55, 0.45, 0.15, 0.3)),

    ("Photon Burst", "Ultra-bright staccato hits with crystalline decay.",
     ["photon", "staccato", "bright", "crystalline"],
     p(mr=10.0, es=8000.0, cd=1.0, dc=0.55, sf=0.6, ps=0.0, ib=0.92, li=0.0, mb=0.1, nc=0.92),
     dna(0.92, 0.08, 0.3, 0.3, 0.1, 0.4)),

    ("Coral Polyp", "Bright, living texture with gentle metabolic pulse.",
     ["coral", "bright", "living", "gentle"],
     p(mr=1.5, es=2500.0, cd=0.45, dc=0.15, sf=0.55, ps=0.15, ib=0.62, li=0.0, mb=0.3, nc=0.62),
     dna(0.58, 0.45, 0.35, 0.35, 0.3, 0.1)),

    ("Spectrum Scan", "Sweeping through the full isotope range. Spectral journey.",
     ["sweep", "scanning", "spectral", "journey"],
     p(mr=1.0, es=2000.0, cd=0.5, dc=0.15, sf=0.55, ps=0.0, ib=0.5, li=0.0, mb=0.25, nc=0.5),
     dna(0.5, 0.4, 0.45, 0.4, 0.25, 0.15)),

    ("Gamma Ray", "High-energy burst with extreme upper partial content.",
     ["gamma", "high-energy", "extreme", "burst"],
     p(mr=10.0, es=10000.0, cd=1.5, dc=0.5, sf=0.75, ps=0.0, ib=0.95, li=0.0, mb=0.05, nc=0.95),
     dna(0.95, 0.05, 0.4, 0.5, 0.05, 0.65)),

    ("Opal Sheen", "Iridescent surface shimmer. Changes color with angle.",
     ["opal", "iridescent", "shimmer", "surface"],
     p(mr=1.2, es=3000.0, cd=0.4, dc=0.1, sf=0.5, ps=0.1, ib=0.65, li=0.0, mb=0.35, nc=0.65),
     dna(0.6, 0.4, 0.3, 0.3, 0.35, 0.05)),

    ("Anabolic Surge", "Muscle-building harmonic growth. Power lead.",
     ["power", "surge", "growing", "lead"],
     p(mr=6.0, es=2500.0, cd=1.5, dc=0.3, sf=0.8, ps=0.0, ib=0.6, li=0.0, mb=0.1, nc=0.6),
     dna(0.55, 0.35, 0.45, 0.7, 0.1, 0.55)),

    ("Refraction", "Sound bent through harmonic prism. Split and recombined.",
     ["refracted", "bent", "split", "recombined"],
     p(mr=2.0, es=3500.0, cd=0.55, dc=0.18, sf=0.55, ps=0.05, ib=0.7, li=0.0, mb=0.3, nc=0.7),
     dna(0.65, 0.35, 0.25, 0.4, 0.3, 0.15)),
]

# ============================================================================
# FLUX (15 presets) — Rhythmic, tempo-locked, unstable
# ============================================================================
FLUX = [
    ("Pace of Life", "Metabolic rate locked to host tempo. Living groove.",
     ["groove", "tempo", "living", "locked"],
     p(mr=2.0, es=1200.0, cd=0.6, dc=0.2, sf=0.65, ps=0.4, ib=0.48, li=0.7, mb=0.2, nc=0.48),
     dna(0.42, 0.5, 0.7, 0.45, 0.2, 0.2)),

    ("Cellular Rhythm", "Biological pulse at the molecular level.",
     ["cellular", "pulse", "molecular", "biological"],
     p(mr=3.0, es=1500.0, cd=0.55, dc=0.25, sf=0.6, ps=0.5, ib=0.5, li=0.6, mb=0.15, nc=0.5),
     dna(0.45, 0.45, 0.65, 0.4, 0.15, 0.2)),

    ("Fever Dream", "Overheated metabolism. Unstable, hallucinatory pulse.",
     ["fever", "unstable", "hallucination", "hot"],
     p(mr=8.0, es=2000.0, cd=1.2, dc=0.35, sf=0.8, ps=0.6, ib=0.6, li=0.5, mb=0.15, nc=0.6),
     dna(0.55, 0.35, 0.8, 0.55, 0.15, 0.45)),

    ("Cardiac Sync", "Heartbeat-rate metabolic pulse. Warm and steady.",
     ["heartbeat", "steady", "warm", "cardiac"],
     p(mr=1.2, es=800.0, cd=0.5, dc=0.15, sf=0.6, ps=0.3, ib=0.4, li=0.8, mb=0.2, nc=0.4),
     dna(0.35, 0.6, 0.55, 0.4, 0.2, 0.1)),

    ("Circadian Wave", "24-bar cycle of harmonic rise and fall.",
     ["circadian", "cycle", "wave", "long"],
     p(mr=0.3, es=1000.0, cd=0.5, dc=0.1, sf=0.55, ps=0.8, ib=0.45, li=0.4, mb=0.3, nc=0.45),
     dna(0.4, 0.55, 0.5, 0.4, 0.3, 0.08)),

    ("Seizure Grid", "Rapid-fire metabolic spasm. Chaotic but musical.",
     ["seizure", "rapid", "chaotic", "glitch"],
     p(mr=10.0, es=3000.0, cd=1.5, dc=0.5, sf=0.9, ps=0.7, ib=0.65, li=0.3, mb=0.05, nc=0.65),
     dna(0.6, 0.2, 0.9, 0.55, 0.05, 0.6)),

    ("Peristalsis", "Slow muscular wave pushing harmonics through the system.",
     ["wave", "muscular", "pushing", "slow"],
     p(mr=0.5, es=600.0, cd=0.6, dc=0.15, sf=0.7, ps=0.45, ib=0.3, li=0.5, mb=0.2, nc=0.3),
     dna(0.25, 0.65, 0.55, 0.5, 0.2, 0.15)),

    ("Mitotic Clock", "Cell division timing. Regular splits creating new harmonics.",
     ["mitotic", "clock", "regular", "dividing"],
     p(mr=2.5, es=2000.0, cd=0.6, dc=0.25, sf=0.6, ps=0.35, ib=0.55, li=0.65, mb=0.2, nc=0.55),
     dna(0.5, 0.4, 0.6, 0.5, 0.2, 0.2)),

    ("Twitching Nerve", "Fast, irregular metabolic bursts. Nervous energy.",
     ["twitching", "nervous", "irregular", "fast"],
     p(mr=7.0, es=3500.0, cd=0.8, dc=0.4, sf=0.75, ps=0.55, ib=0.65, li=0.2, mb=0.1, nc=0.65),
     dna(0.6, 0.25, 0.75, 0.45, 0.1, 0.4)),

    ("Biorhythm Lock", "Tight tempo sync with warm metabolic foundation.",
     ["biorhythm", "sync", "warm", "tight"],
     p(mr=1.5, es=1000.0, cd=0.55, dc=0.18, sf=0.6, ps=0.3, ib=0.45, li=0.9, mb=0.2, nc=0.45),
     dna(0.4, 0.55, 0.6, 0.45, 0.2, 0.15)),

    ("Metabolic Jitter", "Slightly unstable metabolism creating organic swing.",
     ["jitter", "unstable", "swing", "organic"],
     p(mr=3.5, es=1500.0, cd=0.6, dc=0.22, sf=0.65, ps=0.45, ib=0.5, li=0.4, mb=0.15, nc=0.5),
     dna(0.45, 0.45, 0.6, 0.45, 0.15, 0.2)),

    ("Pulse Colony", "Many small pulses creating complex polyrhythmic texture.",
     ["colony", "polyrhythm", "complex", "many"],
     p(mr=4.0, es=2000.0, cd=0.5, dc=0.3, sf=0.6, ps=0.6, ib=0.55, li=0.5, mb=0.2, nc=0.55),
     dna(0.5, 0.4, 0.7, 0.5, 0.2, 0.25)),

    ("Adrenaline Pump", "Fight-or-flight metabolic surge. Intense rhythmic drive.",
     ["adrenaline", "intense", "drive", "surge"],
     p(mr=9.0, es=2500.0, cd=1.5, dc=0.4, sf=0.85, ps=0.3, ib=0.6, li=0.7, mb=0.05, nc=0.6),
     dna(0.55, 0.3, 0.85, 0.6, 0.05, 0.6)),

    ("Tidal Pulse", "Ocean-like rhythmic swell tied to tempo.",
     ["tidal", "ocean", "swell", "rhythmic"],
     p(mr=0.8, es=800.0, cd=0.5, dc=0.12, sf=0.6, ps=0.5, ib=0.4, li=0.6, mb=0.3, nc=0.4),
     dna(0.35, 0.6, 0.55, 0.4, 0.3, 0.1)),

    ("Entropy Flux", "Chaotic entropy fluctuations creating glitchy rhythm.",
     ["entropy", "chaotic", "glitchy", "fluctuating"],
     p(mr=6.0, es=4000.0, cd=0.9, dc=0.35, sf=0.8, ps=0.7, ib=0.7, li=0.3, mb=0.1, nc=0.7),
     dna(0.65, 0.25, 0.8, 0.5, 0.1, 0.45)),
]

# ============================================================================
# AETHER (15 presets) — Ultra-slow, glacial, minutes-long evolution
# ============================================================================
AETHER = [
    ("Cellular Bloom", "Minutes-long harmonic accumulation. Ultra-patient organism.",
     ["bloom", "patient", "minutes", "accumulating"],
     p(mr=0.1, es=1000.0, cd=0.4, dc=0.04, sf=0.5, ps=0.05, ib=0.45, li=0.0, mb=0.5, nc=0.45),
     dna(0.4, 0.55, 0.15, 0.4, 0.5, 0.05)),

    ("Geological Time", "Harmonics that shift over continental timescales.",
     ["geological", "slow", "continental", "massive"],
     p(mr=0.1, es=400.0, cd=0.5, dc=0.03, sf=0.6, ps=0.02, ib=0.2, li=0.0, mb=0.45, nc=0.25),
     dna(0.15, 0.75, 0.08, 0.5, 0.45, 0.1)),

    ("Heat Death", "The universe's final sound. Maximum entropy, minimum energy.",
     ["heat-death", "entropy", "final", "cosmic"],
     p(mr=0.1, es=500.0, cd=0.2, dc=0.02, sf=0.3, ps=0.0, ib=0.5, li=0.0, mb=0.7, nc=0.5),
     dna(0.4, 0.4, 0.05, 0.2, 0.7, 0.02)),

    ("Primordial Soup", "The conditions before life. Raw potential as sound.",
     ["primordial", "raw", "potential", "origin"],
     p(mr=0.15, es=600.0, cd=0.6, dc=0.05, sf=0.7, ps=0.1, ib=0.3, li=0.0, mb=0.4, nc=0.3),
     dna(0.25, 0.65, 0.2, 0.5, 0.4, 0.15)),

    ("Stellar Nursery", "Where harmonic stars are born. Bright, vast, generative.",
     ["stellar", "birth", "vast", "generative"],
     p(mr=0.2, es=2000.0, cd=0.5, dc=0.05, sf=0.55, ps=0.08, ib=0.6, li=0.0, mb=0.6, nc=0.6),
     dna(0.55, 0.45, 0.2, 0.4, 0.6, 0.08)),

    ("Deep Hibernation", "Organism in suspended animation. Barely alive.",
     ["hibernation", "suspended", "minimal", "barely"],
     p(mr=0.1, es=800.0, cd=0.2, dc=0.03, sf=0.2, ps=0.0, ib=0.4, li=0.0, mb=0.5, nc=0.4),
     dna(0.35, 0.5, 0.05, 0.15, 0.5, 0.02)),

    ("Tectonic Shift", "Massive harmonic plates grinding past each other.",
     ["tectonic", "massive", "grinding", "slow"],
     p(mr=0.12, es=300.0, cd=0.7, dc=0.06, sf=0.8, ps=0.05, ib=0.15, li=0.0, mb=0.35, nc=0.2),
     dna(0.1, 0.8, 0.12, 0.65, 0.35, 0.25)),

    ("Cosmic Background", "The residual hum of creation. Omnipresent, barely there.",
     ["cosmic", "residual", "omnipresent", "hum"],
     p(mr=0.1, es=1500.0, cd=0.15, dc=0.02, sf=0.25, ps=0.0, ib=0.5, li=0.0, mb=0.65, nc=0.5),
     dna(0.45, 0.45, 0.03, 0.1, 0.65, 0.01)),

    ("Glacier Mind", "Ice-slow intelligence. Thoughts that take millennia.",
     ["glacier", "slow", "ice", "intelligence"],
     p(mr=0.1, es=2500.0, cd=0.35, dc=0.03, sf=0.4, ps=0.03, ib=0.65, li=0.0, mb=0.55, nc=0.65),
     dna(0.6, 0.35, 0.08, 0.3, 0.55, 0.03)),

    ("Pangaea", "All harmonics unified in one supercontinent of sound.",
     ["unified", "massive", "ancient", "whole"],
     p(mr=0.15, es=800.0, cd=0.6, dc=0.04, sf=0.7, ps=0.05, ib=0.4, li=0.0, mb=0.4, nc=0.4),
     dna(0.35, 0.65, 0.12, 0.6, 0.4, 0.1)),

    ("Abiogenesis", "The moment chemistry became biology. First harmonics emerge.",
     ["origin", "emergence", "first", "becoming"],
     p(mr=0.2, es=1200.0, cd=0.45, dc=0.05, sf=0.55, ps=0.08, ib=0.5, li=0.0, mb=0.45, nc=0.5),
     dna(0.45, 0.5, 0.18, 0.35, 0.45, 0.08)),

    ("Eternal Return", "Cyclic evolution that never resolves. Always becoming.",
     ["cyclic", "eternal", "becoming", "unresolved"],
     p(mr=0.2, es=1000.0, cd=0.4, dc=0.04, sf=0.5, ps=0.15, ib=0.45, li=0.0, mb=0.5, nc=0.45),
     dna(0.4, 0.5, 0.25, 0.35, 0.5, 0.05)),

    ("Dark Matter Pad", "Invisible harmonic mass shaping the audible.",
     ["dark-matter", "invisible", "shaping", "gravitational"],
     p(mr=0.12, es=500.0, cd=0.5, dc=0.04, sf=0.6, ps=0.03, ib=0.25, li=0.0, mb=0.45, nc=0.3),
     dna(0.2, 0.6, 0.1, 0.45, 0.45, 0.08)),

    ("Schwarzschild Gate", "The boundary where harmonics can no longer escape.",
     ["horizon", "boundary", "inescapable", "gravitational"],
     p(mr=0.15, es=700.0, cd=0.55, dc=0.05, sf=0.65, ps=0.05, ib=0.3, li=0.0, mb=0.4, nc=0.35),
     dna(0.25, 0.65, 0.12, 0.5, 0.4, 0.15)),

    ("Solaris Ocean", "An alien intelligence made of sound. Unknowable.",
     ["alien", "ocean", "intelligence", "unknowable"],
     p(mr=0.18, es=1200.0, cd=0.45, dc=0.04, sf=0.55, ps=0.1, ib=0.5, li=0.0, mb=0.55, nc=0.5),
     dna(0.45, 0.5, 0.2, 0.4, 0.55, 0.08)),
]

# ============================================================================
# Generate all presets
# ============================================================================
def main():
    all_presets = []

    for name, desc, tags, params, d in FOUNDATION:
        all_presets.append(make_preset(name, "Foundation", desc, tags, params, d))

    for name, desc, tags, params, d in ATMOSPHERE:
        all_presets.append(make_preset(name, "Atmosphere", desc, tags, params, d))

    for name, desc, tags, params, d in ENTANGLED:
        all_presets.append(make_preset(name, "Entangled", desc, tags, params, d))

    for name, desc, tags, params, d in PRISM:
        all_presets.append(make_preset(name, "Prism", desc, tags, params, d))

    for name, desc, tags, params, d in FLUX:
        all_presets.append(make_preset(name, "Flux", desc, tags, params, d))

    for name, desc, tags, params, d in AETHER:
        all_presets.append(make_preset(name, "Aether", desc, tags, params, d))

    # Check for duplicate names
    names = [p["name"] for p in all_presets]
    dupes = [n for n in names if names.count(n) > 1]
    if dupes:
        print(f"WARNING: Duplicate preset names: {set(dupes)}")

    # Write all presets
    count_by_mood = {}
    for preset in all_presets:
        path = write_preset(preset)
        mood = preset["mood"]
        count_by_mood[mood] = count_by_mood.get(mood, 0) + 1

    print(f"Generated {len(all_presets)} XOrganon presets:")
    for mood, count in sorted(count_by_mood.items()):
        print(f"  {mood}: {count}")


if __name__ == "__main__":
    main()
