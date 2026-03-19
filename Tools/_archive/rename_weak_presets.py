#!/usr/bin/env python3
"""
rename_weak_presets.py — Fleet-wide preset name elevation for XO_OX-XOmnibus

Elevates generic/functional preset names to evocative, atmospheric alternatives
aligned with the XO_OX brand identity and aquatic mythology.

Usage:
    python3 rename_weak_presets.py --dry-run    # preview all changes
    python3 rename_weak_presets.py              # apply changes
    python3 rename_weak_presets.py --engine Odyssey  # filter by engine

Naming philosophy:
  - Aquatic/oceanic environments (thermocline, hadal zone, kelp bed, bioluminescence)
  - Emotional states (dissolution, yearning, emergence, vertigo)
  - Physical phenomena (crystallization, pressure wave, thermal inversion)
  - Mythological/historical (Sargasso, Leviathan, Nereid, Abyssal Plain)
  - Engine identity matched: Bob=warm/fuzzy, Drift=psychedelic/shimmer,
    Fat=massive/abyssal, Morph=liminal/transitional, Dub=echo/pressure,
    Overbite=bite/crunch, Overdub=dub/echo, Overworld=chip/retro
"""

import json
import os
import sys
import glob
import argparse
import shutil
from datetime import datetime

PRESETS_BASE = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..", "Presets"
)

# ─────────────────────────────────────────────────────────────────────────────
# NAME MAP: old_name -> new_name
# Keyed by the "name" field in the JSON (not the filename).
# Each entry is a tuple of (new_name, rationale).
# ─────────────────────────────────────────────────────────────────────────────
NAME_MAP = {

    # ─── BOB / Oblong (warm, fuzzy, analog, character-driven) ────────────────
    "Bob Warm Pad":      ("Hearthside Current",      "The analog warmth of home — slow, sustained, like current through warm kelp"),
    "Bob_Warm_Pad":      ("Hearthside Current",      "The analog warmth of home — slow, sustained, like current through warm kelp"),
    "Bob_Pad_Lush":      ("Sargasso Drift",           "Lush suspended mass — wide, detuned, drifting in the Sargasso's windless heart"),
    "Bob_Slow_Pad":      ("Tide Margin",              "Barely-there presence at the water's edge — presence without intrusion"),
    "Bob_Vapor_Pad":     ("Evaporation Zone",         "Sound evaporating at the surface — vaporous, dusty, almost gone"),
    "Bob_Lead_Classic":  ("Coral Antenna",            "The analog signal reaching up through the reef — bright saw, alive"),
    "Bob_Stab_Chord":    ("Pressure Burst",           "Short, dense chord impact like a hydrothermal vent release"),
    "Bob_Duo_Chord":     ("Two Currents Meeting",     "Dual oscillators in the convergence zone — warm harmonic interference"),
    "Bob_Poly_Chord":    ("Reef Chorus",              "All voices active — the full reef colony speaking at once"),

    # ─── DRIFT / Odyssey (psychedelic, shimmer, familiar↔alien) ─────────────
    "Drift_Warm_Pad":    ("Haze Thermocline",         "DRIFT's warmest mode — the boundary layer where hot and cold meet and shimmer"),
    "Drift_Haze_Pad":    ("Saturation Bloom",         "Maximum haze saturation — warm density like bioluminescent bloom in still water"),
    "Drift_Pad_Wide":    ("Superposed Horizon",       "Heavy supersaw stereo — width like standing at the edge of the continental shelf"),
    "Drift_Lead_Classic":("Voyager Signal",            "Classic analog lead on the long journey out — simple, musical, traveling"),
    "Drift_Keys_Bright": ("Littoral Flash",            "Bright keys at the shoreline — high filter, short decay, salt-bright clarity"),
    "Drift_Keys_Bright": ("Littoral Flash",            "Bright keys at the shoreline — high filter, short decay, salt-bright clarity"),

    # ─── FAT / Obese (massive, sub-bass, abyssal, maximum density) ───────────
    "Fat Bass":          ("Hadal Pressure",           "Maximum low-end density — the weight of the hadal zone pressing from below"),
    "Fat_Bass":          ("Hadal Pressure",           "Maximum low-end density — the weight of the hadal zone pressing from below"),
    "Fat_Pad_Warm":      ("Mesopelagic Bloom",        "Thick, detuned warmth at mid-depth — the quintessential abyssal pad"),
    "Fat_Wide_Pad":      ("Abyssal Horizon",          "Maximum stereo width — the widest pad achievable, like the ocean floor's expanse"),
    "Fat_Stab":          ("Vent Pulse",               "Short, maximum-density impact — hydrothermal vent percussive burst"),
    "Fat_Sub_Only":      ("Midnight Zone",            "Filter nearly closed, sub dominant — pure sub-bass from the midnight zone"),

    # ─── MORPH / OddOscar (liminal, transitional, wavetable scanning) ────────
    "Morph_Pad_Warm":    ("Transitional Membrane",   "MORPH at low position — fundamental-heavy tone, slow bloom through states"),
    "Morph_Resonant_Pad":("Eigenfrequency",           "Wavetable pad with self-resonance — the frequency at which a thing vibrates alone"),
    "Morph_Mono_Lead":   ("Littoral Migration",       "MORPH mono lead with portamento — the seasonal journey between states"),
    "Morph_Bright_Lead": ("Spectral Apex",            "MORPH at maximum position — brightest harmonic summit, all overtones exposed"),
    "Morph_Bright_Pluck":("Dissolution Strike",       "Bright spectral pluck — a moment of crystallization then fade"),
    "Morph_Full_Scan":   ("Tidal Traverse",           "Fast bloom, mid-morph — actively scanning the full tidal cycle of states"),
    "Morph_Dub_Classic_Bass":("Nereid's Bassline",    "MORPH keys with DUB below — the ocean nymph's song over deep current"),

    # ─── ODYSSEY / Drift (bass voices — grounding, traveling) ───────────────
    "Ambient Bass":      ("Benthic Presence",         "A bass that simply exists, grounding without intrusion — benthic floor tone"),
    "Boomy Sub":         ("Deep Resonance",           "Sub-bass bloom from the deep — pressure wave expanding upward"),
    "Detune Bass":       ("Thermocline Beating",      "Detuned oscillators creating beats — the interference pattern of two water masses"),
    "Fifth Bass":        ("Interval Drop",            "The perfect fifth below the root — the bass note that implies the whole"),
    "FM Thump":          ("Operator Descent",         "FM-generated floor punch — carrier sinking below the filter cutoff"),
    "Formant Bass":      ("Vowel Depth",              "Formant-filtered bass — the singing tube of the ocean trench"),
    "Glide Bass":        ("Pelagic Glide",            "Portamento bass voice — movement through the open water column"),
    "Legato Smooth":     ("Laminar Flow",             "Smooth, connected legato — water flowing without turbulence"),
    "Muted Bass":        ("Sediment Filter",          "Bass muted by the weight of accumulated sediment above"),
    "Octave Bass":       ("Sub-Octave Anchor",        "Octave below the root — the gravitational pull of the ocean floor"),
    "Pluck Bass":        ("Surface Strike",           "Short attack pluck — the moment something breaks the water's surface"),
    "Pulse Width Bass":  ("Tidal Compression",        "Pulse width modulated bass — the squeeze and release of tidal breathing"),
    "Pure Sub":          ("Origin Point",             "The sine wave below all else — the very beginning of the sound column"),
    "Rubber Bass":       ("Elastic Depth",            "Flexible bass with soft transient — the pressure-elastic quality of the deep"),
    "Square Thump":      ("Pressure Front",           "Square wave punch — the flat face of a pressure front arriving"),
    "Supersaw Sub":      ("Neritic Mass",             "Supersaw wave at sub frequency — shallow water density meeting the surface"),
    "Wide Bass":         ("Shelf Extension",          "Maximum-width bass — the continental shelf's broadest reach"),
    "String Pluck":      ("Karplus Resonance",        "Acoustic pluck via Karplus-Strong — the struck string still ringing"),

    # ─── OVERBITE (bite, crunch, saw, noise) ────────────────────────────────
    "Noise Bed":         ("Static Plankton",          "Sustained noise with crackle — the ocean's background radiation, alive"),
    "Hollow Pad":        ("Notch Filter Grotto",      "Notch-carved hollow pad — a cave in the frequency spectrum"),
    "Saw Growl":         ("Benthic Snarl",            "Saw bass with fur warmth — the predator sound of the ocean floor"),
    "FM Pad":            ("Carrier Overtones",        "FM interaction at low ratio — complex harmonics, evolutionary complexity"),
    "Chew Lead":         ("Predator Signal",          "Heavy chew contour lead — compressed, forward, hunting"),
    "Velvet Sub":        ("Triangle Abyss",           "Triangle belly sub — the softest possible weight from the deepest point"),

    # ─── OVERDUB (dub echo, pressure, spatial, delay) ────────────────────────
    "Cosmic Drone":      ("Infinite Delay Horizon",   "Dub drone in vast reverb — the echo arriving from beyond the event horizon"),
    "Glide Lead":        ("Dub Glissando",            "Dub lead with portamento — a melody descending through the pressure drop"),

    # ─── OVERWORLD / chip / retro ────────────────────────────────────────────
    "Ring Lead":         ("Inharmonic Shimmer",       "Ring modulation makes inharmonic partials — metallic, alien-adjacent shimmer"),

    # ─── ODDFELIX / OddfeliX (Moog ladder, character, feliX identity) ────────
    "Warm Pad":          ("Moog Shoreline",           "MORPH at its most inviting — Moog ladder warmth, the familiar coast"),
    "FM Bell":           ("Metallic Tide",            "FM bell attack over sustained tone — the bell buoy in choppy water"),

    # ─── PRISM CATEGORY (lead voices, bright, spectral) ─────────────────────
    "Arp Lead":          ("Sequential Phosphorescence","Arpeggiated lead — each note lights up and fades like bioluminescence in sequence"),
    "Bitcrushed Lead":   ("Fracture Signal",          "Bitcrushed lead — the digital coral reef fragmenting under pressure"),
    "FM Bell":           ("Operator Bell",            "FM-generated bell partial — the overtone series in a single strike"),
    "FM Razor":          ("Harmonic Blade",           "FM razor — the thin edge where carrier meets modulator at high ratio"),
    "Mono Glide":        ("Pelagic Solo",             "Mono portamento lead — the solo voice migrating through open water"),
    "Mono Growl":        ("Thermocline Snarl",        "Mono filter-driven growl — the sound of warm water pressing against cold"),
    "Notch Lead":        ("Spectral Absence",         "Notch-filtered lead — defined by what is removed, not what remains"),
    "Overdrive Mono":    ("Surge Lead",               "Saturated mono voice — the surge tide breaking over the seawall"),
    "Pluck Keys":        ("Trigger Particle",         "Short-decay pluck — the bioluminescent organism triggered by movement"),
    "Portamento Glide":  ("Cetacean Arc",             "Portamento lead — the long glide of a whale's descent through water"),
    "Pulse Width Lead":  ("PWM Tide",                 "Pulse width modulated lead — expanding and contracting like tidal breathing"),
    "Saw Stack Unison":  ("Upwelling Stack",          "Stacked unison saws — the upwelling of nutrients rising from the deep"),
    "Soft Supersaw":     ("Diffuse Shimmer",          "Soft supersaw — the psychedelic shimmer of DRIFT in its gentlest mode"),
    "Supersaw Chord":    ("Harmonic Upwelling",       "Supersaw chord voice — all partials rising from depth at once"),

    # ─── ATMOSPHERE / AETHER category generic names ──────────────────────────
    "Harmonic Drone":    ("Overtone Column",          "Floating harmonic drone — the water column's own resonant frequency"),
    "Low Drone":         ("Abyssal Fundamental",      "Ethereal low drone with voyager drift — the lowest audible frequency in open ocean"),
    "Analog Warmth":     ("Temperate Drift",          "Warm analog pad floating — the temperate zone where cold and warm currents meet"),

    # ─── DUAL ENGINE names (Morph + Dub) ─────────────────────────────────────
    "Morph_Bright_Lead": ("Spectral Crest",          "MORPH at full spectral position — the crest of the breaking harmonic wave"),
}

# ─────────────────────────────────────────────────────────────────────────────
# File rename map: when the name changes AND the filename should also update,
# list old_filename_stem -> new_filename_stem here.
# We only rename the file when there's no ambiguity (one preset per name).
# ─────────────────────────────────────────────────────────────────────────────
# For safety we DON'T rename files — only update the "name" field inside JSON.
# This keeps paths stable and avoids breaking any references.


def find_all_presets(base):
    """Return list of all .xometa file paths under base."""
    return sorted(glob.glob(os.path.join(base, "**", "*.xometa"), recursive=True))


def load_preset(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def save_preset(path, data):
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
        f.write("\n")


def engine_filter_matches(engines, engine_filter):
    if not engine_filter:
        return True
    return any(engine_filter.lower() in e.lower() for e in engines)


def main():
    parser = argparse.ArgumentParser(description="Elevate weak preset names fleet-wide")
    parser.add_argument("--dry-run", action="store_true",
                        help="Preview changes without writing files")
    parser.add_argument("--engine", type=str, default="",
                        help="Filter to a specific engine name (e.g. Odyssey, Oblong)")
    parser.add_argument("--verbose", action="store_true",
                        help="Show all files inspected, not just changes")
    args = parser.parse_args()

    presets = find_all_presets(PRESETS_BASE)
    print(f"Scanning {len(presets)} preset files in {PRESETS_BASE}")
    print()

    changes = []
    skipped = []

    for path in presets:
        try:
            data = load_preset(path)
        except Exception as e:
            print(f"  PARSE ERROR: {path}: {e}")
            continue

        name = data.get("name", "")
        engines = data.get("engines", [])

        if not engine_filter_matches(engines, args.engine):
            continue

        if name in NAME_MAP:
            new_name, rationale = NAME_MAP[name]
            rel_path = os.path.relpath(path, PRESETS_BASE)
            changes.append({
                "path": path,
                "rel": rel_path,
                "old_name": name,
                "new_name": new_name,
                "rationale": rationale,
                "engines": engines,
                "mood": data.get("mood", ""),
                "data": data,
            })
        elif args.verbose:
            rel_path = os.path.relpath(path, PRESETS_BASE)
            skipped.append(f"  OK  [{name}] {rel_path}")

    # Report
    print(f"Found {len(changes)} names to elevate")
    print()

    if not changes:
        print("Nothing to do.")
        return

    # Group by engine for readability
    from collections import defaultdict
    by_engine = defaultdict(list)
    for c in changes:
        eng = c["engines"][0] if c["engines"] else "Unknown"
        by_engine[eng].append(c)

    for eng in sorted(by_engine.keys()):
        group = by_engine[eng]
        print(f"  {eng} ({len(group)} changes):")
        for c in group:
            print(f"    [{c['old_name']}]")
            print(f"      → [{c['new_name']}]")
            print(f"      Rationale: {c['rationale']}")
            print(f"      File: {c['rel']}")
            print()

    if args.dry_run:
        print("=" * 60)
        print(f"DRY RUN — {len(changes)} names would be elevated.")
        print("Run without --dry-run to apply changes.")
        return

    # Apply changes
    print("=" * 60)
    print(f"Applying {len(changes)} name elevations...")
    applied = 0
    for c in changes:
        data = c["data"]
        data["name"] = c["new_name"]
        try:
            save_preset(c["path"], data)
            print(f"  OK [{c['old_name']}] → [{c['new_name']}]  ({c['rel']})")
            applied += 1
        except Exception as e:
            print(f"  ERROR writing {c['path']}: {e}")

    print()
    print(f"Done. {applied}/{len(changes)} names elevated.")


if __name__ == "__main__":
    main()
