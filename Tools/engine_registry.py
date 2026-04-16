#!/usr/bin/env python3
"""
engine_registry.py — Single source of truth for all 76 XOceanus engine names
and their frozen parameter prefixes.

Synced to Source/Core/PresetManager.h: validEngineNames + frozenPrefixForEngine.

Any Python tool that needs the engine list should import from here:

    from engine_registry import get_all_engines, get_prefix, is_valid_engine

Do NOT maintain local engine lists in individual tools.  When a new engine is
added, update PresetManager.h and this file only — all tools stay current
automatically.
"""

from __future__ import annotations

# ---------------------------------------------------------------------------
# Canonical engine names (76 total) — order matches PresetManager.h
# ---------------------------------------------------------------------------
# Each entry is (canonical_name, frozen_prefix, accent_color, category).
# frozen_prefix matches PresetManager.h's frozenPrefixForEngine() exactly.
# accent_color matches CLAUDE.md's Engine Modules table.
# category is one of: mascot, original, constellation, phase4, v1concept,
#   flagship, v2theorem, singularity, vocal, drums, chef, kitchen, cellar,
#   garden, broth, fusion, membrane, circuit, visionary, dual, cellular.

_ENGINE_DATA: list[tuple[str, str, str, str]] = [
    # Mascots
    ("OddfeliX",   "snap",     "#00A6D6", "mascot"),
    ("OddOscar",   "morph",    "#E8839B", "mascot"),
    # Original fleet
    ("Overdub",    "dub",      "#6B7B3A", "original"),
    ("Odyssey",    "drift",    "#7B2D8B", "original"),
    ("Oblong",     "bob",      "#E9A84A", "original"),
    ("Obese",      "fat",      "#FF1493", "original"),
    ("Onset",      "perc",     "#0066FF", "original"),
    ("Overworld",  "ow",       "#39FF14", "original"),
    ("Opal",       "opal",     "#A78BFA", "original"),
    ("Orbital",    "orb",      "#FF6B6B", "original"),
    ("Organon",    "organon",  "#00CED1", "original"),
    ("Ouroboros",  "ouro",     "#FF2D2D", "original"),
    ("Obsidian",   "obsidian", "#E8E0D8", "original"),
    ("Overbite",   "poss",     "#F0EDE8", "original"),
    ("Origami",    "origami",  "#E63946", "original"),
    ("Oracle",     "oracle",   "#4B0082", "original"),
    ("Obscura",    "obscura",  "#8A9BA8", "original"),
    ("Oceanic",    "ocean",    "#00B4A0", "original"),
    ("Ocelot",     "ocelot",   "#C5832B", "original"),
    ("Osprey",     "osprey",   "#1B4F8A", "original"),
    ("Osteria",    "osteria",  "#722F37", "original"),
    ("Owlfish",    "owl",      "#B8860B", "original"),
    ("Optic",      "optic",    "#00FF41", "original"),
    ("Oblique",    "oblq",     "#BF40FF", "original"),
    # Constellation engines
    ("Ohm",        "ohm",      "#87AE73", "constellation"),
    ("Orphica",    "orph",     "#7FDBCA", "constellation"),
    ("Obbligato",  "obbl",     "#FF8A7A", "constellation"),
    ("Ottoni",     "otto",     "#5B8A72", "constellation"),
    ("Ole",        "ole",      "#C9377A", "constellation"),
    # Phase 3/4 engines
    ("Ombre",      "ombre",    "#7B6B8A", "phase4"),
    ("Orca",       "orca",     "#1B2838", "phase4"),
    ("Octopus",    "octo",     "#E040FB", "phase4"),
    ("Overlap",    "olap",     "#00FFB4", "phase4"),
    ("Outwit",     "owit",     "#CC6600", "phase4"),
    # Concept engines
    ("OpenSky",    "sky",      "#FF8C00", "v1concept"),
    ("Ostinato",   "osti",     "#E8701A", "v1concept"),
    ("OceanDeep",  "deep",     "#2D0A4E", "v1concept"),
    ("Ouie",       "ouie",     "#708090", "v1concept"),
    # Flagship
    ("Obrix",      "obrix",    "#1E8B7E", "flagship"),
    # V2 theorem engines
    ("Orbweave",   "weave",    "#8E4585", "v2theorem"),
    ("Overtone",   "over",     "#A8D8EA", "v2theorem"),
    ("Organism",   "org",      "#C6E377", "v2theorem"),
    # Singularity engines
    ("Oxbow",      "oxb",      "#1A6B5A", "singularity"),
    ("Oware",      "owr",      "#B5883E", "singularity"),
    # Kuramoto vocal synthesis
    ("Opera",      "opera",    "#D4AF37", "vocal"),
    # Psychology-driven boom bap drums
    ("Offering",   "ofr",      "#E5B80B", "drums"),
    # Chef Quad Collection
    ("Oto",        "oto",      "#F5F0E8", "chef"),
    ("Octave",     "oct",      "#8B6914", "chef"),
    ("Oleg",       "oleg",     "#C0392B", "chef"),
    ("Otis",       "otis",     "#D4A017", "chef"),
    # Kitchen Quad Collection
    ("Oven",       "oven_",    "#1C1C1C", "kitchen"),
    ("Ochre",      "ochre_",   "#CC7722", "kitchen"),
    ("Obelisk",    "obel_",    "#FFFFF0", "kitchen"),
    ("Opaline",    "opal2_",   "#B7410E", "kitchen"),
    # Cellar Quad Collection
    ("Ogre",       "ogre_",    "#0D0D0D", "cellar"),
    ("Olate",      "olate_",   "#5C3317", "cellar"),
    ("Oaken",      "oaken_",   "#9C6B30", "cellar"),
    ("Omega",      "omega_",   "#003366", "cellar"),
    # Garden Quad Collection
    ("Orchard",    "orch_",    "#FFB7C5", "garden"),
    ("Overgrow",   "grow_",    "#228B22", "garden"),
    ("Osier",      "osier_",   "#C0C8C8", "garden"),
    ("Oxalis",     "oxal_",    "#9B59B6", "garden"),
    # Broth Quad Collection
    ("Overwash",   "wash_",    "#F0F8FF", "broth"),
    ("Overworn",   "worn_",    "#808080", "broth"),
    ("Overflow",   "flow_",    "#1A3A5C", "broth"),
    ("Overcast",   "cast_",    "#778899", "broth"),
    # Fusion Quad Collection
    ("Okeanos",    "okan_",    "#C49B3F", "fusion"),
    ("Oddfellow",  "oddf_",    "#B87333", "fusion"),
    ("Onkolo",     "onko_",    "#FFBF00", "fusion"),
    ("Opcode",     "opco_",    "#5F9EA0", "fusion"),
    # Membrane Collection
    ("Osmosis",    "osmo_",    "#C0C0C0", "membrane"),
    # Love Triangle Circuit Synth
    ("Oxytocin",   "oxy_",     "#9B5DE5", "circuit"),
    # Panoramic Visionary Synth
    ("Outlook",    "look_",    "#4169E1", "visionary"),
    # Dual Engine Integration
    ("Oasis",      "oas_",     "#00827F", "dual"),
    ("Outflow",    "out_",     "#1A1A40", "dual"),
    # Cellular Automata Oscillator
    ("Obiont",     "obnt_",    "#E8A030", "cellular"),
]

# ---------------------------------------------------------------------------
# Pre-built lookup structures (computed once at import time)
# ---------------------------------------------------------------------------

# Set of canonical names for O(1) membership tests
_NAME_SET: set[str] = {row[0] for row in _ENGINE_DATA}

# Canonical name → frozen prefix
_PREFIX_MAP: dict[str, str] = {row[0]: row[1] for row in _ENGINE_DATA}

# Canonical name → accent color
_COLOR_MAP: dict[str, str] = {row[0]: row[2] for row in _ENGINE_DATA}

# Canonical name → category
_CATEGORY_MAP: dict[str, str] = {row[0]: row[3] for row in _ENGINE_DATA}

# UPPER-CASE name → canonical name  (used by tools that store ALLCAPS names)
_UPPER_MAP: dict[str, str] = {row[0].upper(): row[0] for row in _ENGINE_DATA}


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def get_all_engines() -> list[str]:
    """Return all 76 canonical engine names in registry order."""
    return [row[0] for row in _ENGINE_DATA]


def get_prefix(engine_name: str) -> str:
    """Return the frozen parameter prefix for engine_name.

    The prefix is returned exactly as stored (some include a trailing
    underscore, e.g. "oven_"; others do not, e.g. "snap").
    Raises KeyError if engine_name is not a canonical engine name.
    """
    try:
        return _PREFIX_MAP[engine_name]
    except KeyError:
        raise KeyError(
            f"Unknown engine: {engine_name!r}.  "
            "Use is_valid_engine() to check before calling get_prefix()."
        ) from None


def get_color(engine_name: str) -> str:
    """Return the hex accent color for engine_name.

    Raises KeyError if engine_name is not a canonical engine name.
    """
    try:
        return _COLOR_MAP[engine_name]
    except KeyError:
        raise KeyError(f"Unknown engine: {engine_name!r}") from None


def get_category(engine_name: str) -> str:
    """Return the collection/category string for engine_name."""
    try:
        return _CATEGORY_MAP[engine_name]
    except KeyError:
        raise KeyError(f"Unknown engine: {engine_name!r}") from None


def is_valid_engine(name: str) -> bool:
    """Return True if name is a canonical engine name (exact case)."""
    return name in _NAME_SET


def canonical_from_upper(upper_name: str) -> str | None:
    """Convert an ALL-CAPS engine name to its canonical mixed-case form.

    Returns None if the name is not recognised.
    This is the inverse of name.upper() for engine IDs stored in uppercase
    (e.g. the VALID_ENGINES set in xpn_submission_packager.py).
    """
    return _UPPER_MAP.get(upper_name)


def valid_engines_upper() -> set[str]:
    """Return the set of all engine names in ALL-CAPS form.

    Useful as a drop-in replacement for a hardcoded VALID_ENGINES set that
    stores engine IDs in upper-case (e.g. xpn_submission_packager.py).
    """
    return set(_UPPER_MAP.keys())


# ---------------------------------------------------------------------------
# Self-check (run as script: python3 engine_registry.py)
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    engines = get_all_engines()
    print(f"Total engines: {len(engines)}")
    for name in engines:
        prefix = get_prefix(name)
        color  = get_color(name)
        cat    = get_category(name)
        print(f"  {name:<14}  prefix={prefix:<10}  color={color}  category={cat}")
