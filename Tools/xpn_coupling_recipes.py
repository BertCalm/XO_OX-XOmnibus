#!/usr/bin/env python3
"""
XPN Coupling Recipe Cards — XO_OX Designs
Bundles coupling metadata INTO XPN packs so producers know how to recreate
live engine coupling from XOceanus inside their DAW.

XPN format is static audio. When a coupled preset is exported, the XPN only
captures the rendered output — the live modulation relationship between engines
is lost. This tool extracts coupling data from .xometa files and writes
coupling_recipe.json alongside each .xpm, so the producer has a step-by-step
guide to loading both engines and reconnecting the coupling route.

What it generates per coupled preset:
  coupling_recipe.json  — machine-readable coupling spec + human setup instructions
  (Optional) coupling_snapshot.wav note in recipe — flag for future render tooling

Collection routing:
  Also supports pre-defined collection coupling matrices (Kitchen Fusion,
  Travel Trade Wind, Sable Island) for batch recipe generation.

Usage:
    # All coupled presets in a mood folder
    python xpn_coupling_recipes.py --presets ./Presets/XOceanus/Entangled/ \\
                                   --output ./coupling_recipes/

    # Single preset
    python xpn_coupling_recipes.py --preset "Beat_Drives_Fat.xometa" \\
                                   --output ./coupling_recipes/

    # Collection routing tables only (no preset scan)
    python xpn_coupling_recipes.py --collection kitchen \\
                                   --output ./coupling_recipes/

Integration with xpn_packager.py:
    from xpn_coupling_recipes import find_recipe_for_preset, attach_recipe_to_pack
    attach_recipe_to_pack(xpn_zip, preset_name, recipe_dir)
"""

import argparse
import json
import os
import sys
import zipfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional, Union


# =============================================================================
# Coupling type registry
# =============================================================================

# Human-readable descriptions for every CouplingType enum value.
# Keep in sync with Source/Core/SynthEngine.h :: CouplingType.
COUPLING_TYPE_DESCRIPTIONS: dict[str, str] = {
    "Amp->Filter":       "Source amplitude modulates target filter cutoff — classic sidechain pump effect",
    "AmpToFilter":       "Source amplitude modulates target filter cutoff — classic sidechain pump effect",
    "Amp->Pitch":        "Source amplitude bends target pitch — louder hits push the note up",
    "AmpToPitch":        "Source amplitude bends target pitch — louder hits push the note up",
    "LFO->Pitch":        "Source LFO sweeps target pitch — synchronized vibrato or detune drift",
    "LFOToPitch":        "Source LFO sweeps target pitch — synchronized vibrato or detune drift",
    "Env->Morph":        "Source envelope moves target wavetable/morph position — transient sculpts the timbre",
    "EnvToMorph":        "Source envelope moves target wavetable/morph position — transient sculpts the timbre",
    "Audio->FM":         "Source audio directly FM-modulates target oscillator — sidebands multiply",
    "AudioToFM":         "Source audio directly FM-modulates target oscillator — sidebands multiply",
    "Audio->Ring":       "Source audio ring-modulates target output — metallic combination tones",
    "AudioToRing":       "Source audio ring-modulates target output — metallic combination tones",
    "Filter->Filter":    "Source filter output feeds into target filter input — serial spectral shaping",
    "FilterToFilter":    "Source filter output feeds into target filter input — serial spectral shaping",
    "Amp->Choke":        "Source amplitude chokes target voice — staccato cut on beat",
    "AmpToChoke":        "Source amplitude chokes target voice — staccato cut on beat",
    "Rhythm->Blend":     "Source rhythm pattern shifts target blend parameter — groove cross-fades texture",
    "RhythmToBlend":     "Source rhythm pattern shifts target blend parameter — groove cross-fades texture",
    "Env->Decay":        "Source envelope stretches or compresses target decay time — sympathy release",
    "EnvToDecay":        "Source envelope stretches or compresses target decay time — sympathy release",
    "Pitch->Pitch":      "Source pitch tracks into target pitch — harmonic lock or micro-drift unison",
    "PitchToPitch":      "Source pitch tracks into target pitch — harmonic lock or micro-drift unison",
    "Audio->Wavetable":  "Source audio becomes target wavetable source — live spectral imprinting",
    "AudioToWavetable":  "Source audio becomes target wavetable source — live spectral imprinting",
    "Audio->Buffer":     "Source audio streams into target grain buffer — granular time-telescope effect",
    "AudioToBuffer":     "Source audio streams into target grain buffer — granular time-telescope effect",
}

# Canonical display name for the coupling type (for recipe JSON output).
# Normalises both camelCase and arrow-format variants to a single string.
def _normalise_coupling_type(raw: str) -> str:
    """Return canonical arrow-format coupling type string."""
    mapping = {
        "AmpToFilter":      "Amp->Filter",
        "AmpToPitch":       "Amp->Pitch",
        "LFOToPitch":       "LFO->Pitch",
        "EnvToMorph":       "Env->Morph",
        "AudioToFM":        "Audio->FM",
        "AudioToRing":      "Audio->Ring",
        "FilterToFilter":   "Filter->Filter",
        "AmpToChoke":       "Amp->Choke",
        "RhythmToBlend":    "Rhythm->Blend",
        "EnvToDecay":       "Env->Decay",
        "PitchToPitch":     "Pitch->Pitch",
        "AudioToWavetable": "Audio->Wavetable",
        "AudioToBuffer":    "Audio->Buffer",
    }
    return mapping.get(raw, raw)


def _describe_coupling(coupling_type: str, source_engine: str, target_engine: str) -> str:
    """Generate a one-sentence description of what this coupling route does."""
    canonical = _normalise_coupling_type(coupling_type)
    base = COUPLING_TYPE_DESCRIPTIONS.get(canonical,
           COUPLING_TYPE_DESCRIPTIONS.get(coupling_type,
           f"{coupling_type} modulation from {source_engine} into {target_engine}"))
    return f"{source_engine} → {target_engine}: {base}"


# =============================================================================
# Engine name resolution
# =============================================================================

# Resolve legacy engine names to canonical IDs.
# Mirrors resolveEngineAlias() from Source/Core/PresetManager.h.
_ENGINE_ALIAS_MAP: dict[str, str] = {
    "Snap":   "OddfeliX",
    "Morph":  "OddOscar",
    "Dub":    "Overdub",
    "Drift":  "Odyssey",
    "Bob":    "Oblong",
    "Fat":    "Obese",
    "Bite":   "Overbite",
    # Fully spelled names pass through
}

def _resolve_engine(name: str) -> str:
    return _ENGINE_ALIAS_MAP.get(name, name)


# Macro label defaults per engine (M1–M4 labels from engine specs).
# Used to make setup instructions more actionable.
_ENGINE_MACRO_LABELS: dict[str, list[str]] = {
    "OddfeliX":  ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "OddOscar":  ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Overdub":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Odyssey":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Oblong":    ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Obese":     ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Onset":     ["MACHINE",   "PUNCH",    "SPACE",    "MUTATE"],
    "Overworld": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Opal":      ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Orbital":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Organon":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Ouroboros": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Obsidian":  ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Overbite":  ["BELLY",     "BITE",     "SCURRY",   "TRASH"],
    "Origami":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Oracle":    ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Obscura":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Oceanic":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Ocelot":    ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Optic":     ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Oblique":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Osprey":    ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Osteria":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Owlfish":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Ohm":       ["MEDDLING",  "COMMUNE",  "COUPLING", "SPACE"],
    "Orphica":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Obbligato": ["CHARACTER", "BOND",     "COUPLING", "SPACE"],
    "Ottoni":    ["CHARACTER", "GROW",     "COUPLING", "SPACE"],
    "Ole":       ["CHARACTER", "DRAMA",    "COUPLING", "SPACE"],
    "Ombre":     ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Orca":      ["HUNT",      "MOVEMENT", "COUPLING", "SPACE"],
    "Octopus":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Overlap":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "Outwit":    ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
}


# =============================================================================
# Data structures
# =============================================================================

@dataclass
class CouplingRoute:
    source_engine: str
    target_engine: str
    coupling_type: str          # canonical arrow-format
    amount: float               # 0.0–1.0
    description: str = ""


@dataclass
class CouplingRecipe:
    preset_name: str
    mood: str
    engines: list[str]
    coupling_routes: list[CouplingRoute]
    macro_positions: dict[str, float]   # {"M1": 0.0, ...} pulled from parameters
    setup_instructions: list[str]
    xpn_note: str = (
        "This XPN pack captures the static audio output of the coupled preset. "
        "Load XOceanus with this coupling configuration to hear the live version."
    )
    source_file: str = ""               # relative path to .xometa
    coupling_intensity: str = ""


# =============================================================================
# Parser
# =============================================================================

def _extract_macro_positions(preset: dict) -> dict[str, float]:
    """
    Pull M1–M4 values from a preset's parameters block.
    XOceanus stores macros under each engine's parameter dict as
    '{prefix}_macro_{name}'. We scan all engines and collect the first
    four distinct macro values we find.
    """
    macros: dict[str, float] = {}
    params = preset.get("parameters", {})
    for engine_name, engine_params in params.items():
        if not isinstance(engine_params, dict):
            continue
        for key, val in engine_params.items():
            lower = key.lower()
            if "macro" in lower and isinstance(val, (int, float)):
                # Try to assign to M1–M4 slots in order of discovery
                slot = f"M{len(macros) + 1}"
                if slot not in macros and len(macros) < 4:
                    macros[slot] = round(float(val), 3)
    # Fill missing slots with 0.5 (neutral)
    for i in range(1, 5):
        macros.setdefault(f"M{i}", 0.5)
    return macros


def _build_setup_instructions(
    preset_name: str,
    engines: list[str],
    routes: list[CouplingRoute],
    macro_positions: dict[str, float],
) -> list[str]:
    """
    Produce numbered plain-English setup instructions a producer can follow
    inside XOceanus to recreate the coupling.
    """
    steps: list[str] = []
    step = 1

    # Load engines into slots
    for i, engine in enumerate(engines, start=1):
        steps.append(f"{step}. Load {engine.upper()} in Slot {i}")
        step += 1

    # Connect coupling routes
    for route in routes:
        source_slot = engines.index(route.source_engine) + 1 if route.source_engine in engines else "?"
        target_slot = engines.index(route.target_engine) + 1 if route.target_engine in engines else "?"
        amount_pct = int(round(route.amount * 100))
        steps.append(
            f"{step}. In the Coupling Matrix, connect Slot {source_slot} → Slot {target_slot} "
            f"using coupling type \"{route.coupling_type}\" at {amount_pct}%"
        )
        step += 1

    # Macro positions
    for label, value in sorted(macro_positions.items()):
        pct = int(round(value * 100))
        steps.append(f"{step}. Set macro {label} to {pct}%")
        step += 1

    steps.append(
        f"{step}. Play a note — the XPN pack captures the same sonic state rendered at these settings"
    )

    return steps


def parse_xometa(xometa_path: Union[str, Path]) -> Optional[CouplingRecipe]:
    """
    Parse a single .xometa preset file and return a CouplingRecipe if the
    preset has at least one coupling route. Returns None for uncoupled presets.
    """
    path = Path(xometa_path)
    try:
        with open(path, "r", encoding="utf-8") as fh:
            preset = json.load(fh)
    except (OSError, json.JSONDecodeError) as exc:
        print(f"  [WARN] Could not parse {path.name}: {exc}")
        return None

    coupling_block = preset.get("coupling", {})
    pairs = coupling_block.get("pairs", []) if isinstance(coupling_block, dict) else []

    if not pairs:
        return None  # Not a coupled preset — skip

    preset_name = preset.get("name", path.stem)
    mood = preset.get("mood", "Entangled")
    engines_raw = preset.get("engines", [])
    engines = [_resolve_engine(e) for e in engines_raw]

    routes: list[CouplingRoute] = []
    for pair in pairs:
        if not isinstance(pair, dict):
            continue
        source = _resolve_engine(pair.get("engineA", ""))
        target = _resolve_engine(pair.get("engineB", ""))
        raw_type = pair.get("type", "")
        amount = float(pair.get("amount", 0.5))
        canonical_type = _normalise_coupling_type(raw_type)
        description = _describe_coupling(canonical_type, source, target)
        routes.append(CouplingRoute(
            source_engine=source,
            target_engine=target,
            coupling_type=canonical_type,
            amount=round(amount, 4),
            description=description,
        ))

    if not routes:
        return None

    macro_positions = _extract_macro_positions(preset)
    setup = _build_setup_instructions(preset_name, engines, routes, macro_positions)

    return CouplingRecipe(
        preset_name=preset_name,
        mood=mood,
        engines=engines,
        coupling_routes=routes,
        macro_positions=macro_positions,
        setup_instructions=setup,
        coupling_intensity=preset.get("couplingIntensity", ""),
        source_file=path.name,
    )


# =============================================================================
# Serialisation
# =============================================================================

def recipe_to_dict(recipe: CouplingRecipe) -> dict:
    """Convert a CouplingRecipe to a JSON-serialisable dict."""
    return {
        "preset_name": recipe.preset_name,
        "mood": recipe.mood,
        "engines": recipe.engines,
        "coupling_intensity": recipe.coupling_intensity,
        "coupling_routes": [
            {
                "source_engine": r.source_engine,
                "target_engine": r.target_engine,
                "coupling_type": r.coupling_type,
                "amount": r.amount,
                "description": r.description,
            }
            for r in recipe.coupling_routes
        ],
        "macro_positions": recipe.macro_positions,
        "setup_instructions": recipe.setup_instructions,
        "xpn_note": recipe.xpn_note,
        "source_file": recipe.source_file,
    }


def write_recipe(recipe: CouplingRecipe, output_dir: Union[str, Path]) -> Path:
    """
    Write coupling_recipe.json for a single preset into output_dir.
    File is named {preset_slug}_coupling_recipe.json.
    """
    out = Path(output_dir)
    out.mkdir(parents=True, exist_ok=True)
    slug = recipe.preset_name.replace(" ", "_").replace("/", "-")
    dest = out / f"{slug}_coupling_recipe.json"
    with open(dest, "w", encoding="utf-8") as fh:
        json.dump(recipe_to_dict(recipe), fh, indent=2)
    return dest


# =============================================================================
# Batch scanning
# =============================================================================

def scan_preset_folder(
    presets_dir: Union[str, Path],
    output_dir: Union[str, Path],
    verbose: bool = True,
) -> list[Path]:
    """
    Scan a folder of .xometa files, write a coupling_recipe.json for every
    preset that has at least one coupling route.

    Returns list of written recipe file paths.
    """
    presets_dir = Path(presets_dir)
    written: list[Path] = []

    xometa_files = sorted(presets_dir.rglob("*.xometa"))
    if not xometa_files:
        print(f"  [WARN] No .xometa files found in {presets_dir}")
        return written

    if verbose:
        print(f"\n  Scanning {len(xometa_files)} presets in {presets_dir.name}/")

    coupled = 0
    for xm in xometa_files:
        recipe = parse_xometa(xm)
        if recipe is None:
            continue
        dest = write_recipe(recipe, output_dir)
        written.append(dest)
        coupled += 1
        if verbose:
            route_summary = ", ".join(
                f"{r.source_engine}→{r.target_engine} ({r.coupling_type})"
                for r in recipe.coupling_routes
            )
            print(f"  + {recipe.preset_name}  [{route_summary}]  → {dest.name}")

    if verbose:
        skipped = len(xometa_files) - coupled
        print(f"\n  Results: {coupled} coupled presets, {skipped} uncoupled (skipped)")

    return written


# =============================================================================
# Collection routing tables
# =============================================================================

# Pre-defined coupling matrices for XO_OX collections.
# These represent the canonical coupling architecture for each collection set.
# Producers loading a collection XPN can use these as reference when building
# their own XOceanus session.

COLLECTION_ROUTING_TABLES: dict[str, dict] = {
    "kitchen": {
        "name": "Kitchen Essentials — Fusion Coupling Matrix",
        "description": (
            "6 quads × 4 engines + Fusion 5th slot. "
            "Within each quad, engines couple on the flavour axis. "
            "The Fusion slot receives from all four quad slots via AudioToBuffer."
        ),
        "sets": [
            {
                "set_name": "Kitchen Fusion Quad A",
                "engines": ["EngineA1", "EngineA2", "EngineA3", "EngineA4"],
                "routes": [
                    {"source": "EngineA1", "target": "EngineA2",
                     "type": "Amp->Filter", "amount": 0.5,
                     "note": "Primary flavour feed — adjust amount to taste"},
                    {"source": "EngineA3", "target": "EngineA4",
                     "type": "Env->Morph", "amount": 0.4,
                     "note": "Texture morph on transient — secondary layer"},
                ],
            },
            {
                "set_name": "Fusion 5th Slot",
                "engines": ["Slot1", "Slot2", "Slot3", "Slot4", "FusionSlot"],
                "routes": [
                    {"source": "Slot1", "target": "FusionSlot",
                     "type": "Audio->Buffer", "amount": 0.3,
                     "note": "All four quad outputs stream into Fusion grain buffer"},
                    {"source": "Slot2", "target": "FusionSlot",
                     "type": "Audio->Buffer", "amount": 0.3,
                     "note": ""},
                    {"source": "Slot3", "target": "FusionSlot",
                     "type": "Audio->Buffer", "amount": 0.3,
                     "note": ""},
                    {"source": "Slot4", "target": "FusionSlot",
                     "type": "Audio->Buffer", "amount": 0.3,
                     "note": ""},
                ],
            },
        ],
        "xpn_note": (
            "Kitchen collection XPNs contain rendered output from each engine slot. "
            "Load all engines in XOceanus and apply the Fusion routing to hear "
            "the live interactive version."
        ),
    },

    "travel": {
        "name": "Travel/Water/Vessels — Trade Wind Coupling Matrix",
        "description": (
            "4 sets × 4 engines + Sable Island 5th slot. "
            "Sail × Industrial cross-modulation. "
            "Historical percussion drives Synth Genres envelope morph."
        ),
        "sets": [
            {
                "set_name": "Sail × Industrial (Trade Wind)",
                "engines": ["Sail", "Industrial"],
                "routes": [
                    {"source": "Sail", "target": "Industrial",
                     "type": "LFO->Pitch", "amount": 0.25,
                     "note": "Sail woodwind LFO drifts Industrial brass pitch — wind-over-hull effect"},
                    {"source": "Industrial", "target": "Sail",
                     "type": "Amp->Filter", "amount": 0.35,
                     "note": "Industrial attack pumps Sail filter — engine room breathing into rigging"},
                ],
            },
            {
                "set_name": "Leisure × Historical (Island Crossfade)",
                "engines": ["Leisure", "Historical"],
                "routes": [
                    {"source": "Historical", "target": "Leisure",
                     "type": "Rhythm->Blend", "amount": 0.4,
                     "note": "Historical percussion rhythm shifts Leisure blend — era cross-fade"},
                ],
            },
            {
                "set_name": "Sable Island (Cross-set Fusion)",
                "engines": ["Sail", "Industrial", "Leisure", "Historical", "SableIsland"],
                "routes": [
                    {"source": "Sail", "target": "SableIsland",
                     "type": "Audio->Buffer", "amount": 0.2,
                     "note": "Sable Island receives all four set outputs as grain sources"},
                    {"source": "Industrial", "target": "SableIsland",
                     "type": "Audio->Buffer", "amount": 0.2,
                     "note": ""},
                    {"source": "Leisure", "target": "SableIsland",
                     "type": "Audio->Buffer", "amount": 0.2,
                     "note": ""},
                    {"source": "Historical", "target": "SableIsland",
                     "type": "Audio->Buffer", "amount": 0.2,
                     "note": ""},
                ],
            },
        ],
        "xpn_note": (
            "Travel collection XPNs are rendered per-engine. "
            "Load all four Travel sets plus Sable Island in XOceanus and apply "
            "Trade Wind routing for the full live cross-modulation experience."
        ),
    },

    "artwork": {
        "name": "Artwork/Color — Complementary Color Coupling Matrix",
        "description": (
            "5 quads × 4 + Arcade fusion = 24 engines. "
            "Color Quad A: Oxblood/Onyx/Ochre/Orchid. "
            "Complementary color wildcard modulates via FilterToFilter across each quad."
        ),
        "sets": [
            {
                "set_name": "Color Quad A (Erhu/Didgeridoo/Oud/Guzheng)",
                "engines": ["Oxblood", "Onyx", "Ochre", "Orchid"],
                "routes": [
                    {"source": "Oxblood", "target": "Orchid",
                     "type": "Filter->Filter", "amount": 0.3,
                     "note": "Complementary pair — Oxblood filter bleeds into Orchid timbral space"},
                    {"source": "Onyx", "target": "Ochre",
                     "type": "Env->Morph", "amount": 0.35,
                     "note": "Complementary pair — Onyx envelope morphs Ochre wavetable position"},
                ],
            },
            {
                "set_name": "Arcade Fusion (Suda/Mizuguchi/Takahashi/Massive Monster)",
                "engines": ["Suda", "Mizuguchi", "Takahashi", "MassiveMonster"],
                "routes": [
                    {"source": "Suda", "target": "Mizuguchi",
                     "type": "Audio->Ring", "amount": 0.4,
                     "note": "Suda 51 chaos ring-modulates Mizuguchi synesthesia — collision tones"},
                    {"source": "Takahashi", "target": "MassiveMonster",
                     "type": "Rhythm->Blend", "amount": 0.45,
                     "note": "Takahashi rhythm shifts Monster blend — playful cross-fade"},
                ],
            },
        ],
        "xpn_note": (
            "Artwork collection XPNs capture each engine's voice independently. "
            "Load the full color quad in XOceanus and apply Complementary Color routing "
            "for the live painting-in-motion experience."
        ),
    },
}


def write_collection_routing(
    collection_key: str,
    output_dir: Union[str, Path],
    verbose: bool = True,
) -> Optional[Path]:
    """
    Write a collection coupling matrix JSON to output_dir.
    collection_key: one of "kitchen", "travel", "artwork".
    """
    if collection_key not in COLLECTION_ROUTING_TABLES:
        print(f"  [ERROR] Unknown collection '{collection_key}'. "
              f"Valid: {', '.join(COLLECTION_ROUTING_TABLES.keys())}")
        return None

    table = COLLECTION_ROUTING_TABLES[collection_key]
    out = Path(output_dir)
    out.mkdir(parents=True, exist_ok=True)
    dest = out / f"{collection_key}_collection_coupling_matrix.json"

    with open(dest, "w", encoding="utf-8") as fh:
        json.dump(table, fh, indent=2)

    if verbose:
        print(f"  + Collection matrix: {dest.name}  ({len(table['sets'])} sets)")

    return dest


# =============================================================================
# XPN packager integration
# =============================================================================

def find_recipe_for_preset(preset_name: str, recipe_dir: Union[str, Path]) -> Optional[Path]:
    """
    Search recipe_dir for a coupling_recipe.json matching preset_name.
    Returns the Path if found, None otherwise.

    Called by xpn_packager.py to attach recipes to packs.
    """
    recipe_dir = Path(recipe_dir)
    if not recipe_dir.exists():
        return None
    slug = preset_name.replace(" ", "_").replace("/", "-")
    candidate = recipe_dir / f"{slug}_coupling_recipe.json"
    if candidate.exists():
        return candidate
    # Fuzzy fallback: case-insensitive stem match
    slug_lower = slug.lower()
    for f in recipe_dir.glob("*_coupling_recipe.json"):
        if f.stem.lower().startswith(slug_lower):
            return f
    return None


def attach_recipe_to_pack(
    xpn_path: Union[str, Path],
    preset_name: str,
    recipe_dir: Union[str, Path],
    verbose: bool = True,
) -> bool:
    """
    Append a coupling_recipe.json into an existing .xpn archive if one exists.
    Returns True if a recipe was attached, False if none was found.

    Call this from xpn_packager.py after package_xpn() completes.
    """
    recipe_path = find_recipe_for_preset(preset_name, recipe_dir)
    if recipe_path is None:
        return False

    xpn_path = Path(xpn_path)
    if not xpn_path.exists():
        print(f"  [WARN] Cannot attach recipe — XPN not found: {xpn_path}")
        return False

    try:
        with zipfile.ZipFile(str(xpn_path), "a", zipfile.ZIP_DEFLATED) as zf:
            zf.write(str(recipe_path), f"CouplingRecipes/{recipe_path.name}")
        if verbose:
            print(f"  + CouplingRecipes/{recipe_path.name}")
        return True
    except (OSError, zipfile.BadZipFile) as exc:
        print(f"  [WARN] Could not attach recipe to {xpn_path.name}: {exc}")
        return False


# =============================================================================
# CLI
# =============================================================================

def main() -> None:
    parser = argparse.ArgumentParser(
        description="XPN Coupling Recipe Cards — bundle coupling metadata into XPN packs",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument(
        "--presets",
        metavar="DIR",
        help="Scan a folder of .xometa files and generate recipes for all coupled presets",
    )
    group.add_argument(
        "--preset",
        metavar="FILE",
        help="Generate a recipe for a single .xometa file",
    )
    group.add_argument(
        "--collection",
        metavar="NAME",
        choices=list(COLLECTION_ROUTING_TABLES.keys()),
        help=(
            "Write a collection coupling matrix JSON. "
            f"Choices: {', '.join(COLLECTION_ROUTING_TABLES.keys())}"
        ),
    )
    parser.add_argument(
        "--output",
        required=True,
        metavar="DIR",
        help="Output directory for recipe JSON files",
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="Suppress progress output",
    )
    args = parser.parse_args()

    verbose = not args.quiet

    if args.presets:
        presets_dir = Path(args.presets)
        if not presets_dir.exists():
            print(f"[ERROR] Presets directory not found: {presets_dir}", file=sys.stderr)
            sys.exit(1)
        written = scan_preset_folder(presets_dir, args.output, verbose=verbose)
        if not written:
            print("  No coupled presets found.")
        else:
            print(f"\n  {len(written)} recipe(s) written to {args.output}")

    elif args.preset:
        xm = Path(args.preset)
        if not xm.exists():
            print(f"[ERROR] Preset file not found: {xm}", file=sys.stderr)
            sys.exit(1)
        recipe = parse_xometa(xm)
        if recipe is None:
            print(f"  {xm.name} has no coupling routes — nothing to generate.")
        else:
            dest = write_recipe(recipe, args.output)
            if verbose:
                print(f"\n  Written: {dest}")
                print(f"  Engines: {', '.join(recipe.engines)}")
                print(f"  Routes:  {len(recipe.coupling_routes)}")
                for r in recipe.coupling_routes:
                    print(f"    {r.source_engine} → {r.target_engine}  "
                          f"{r.coupling_type}  {int(r.amount*100)}%")

    elif args.collection:
        dest = write_collection_routing(args.collection, args.output, verbose=verbose)
        if dest:
            print(f"\n  Written: {dest}")


if __name__ == "__main__":
    main()
