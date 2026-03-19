#!/usr/bin/env python3
"""
Atmosphere Expansion Pack — 60 .xometa presets for the Atmosphere mood.

4 tonal families × 15 presets each:
  - Dawn Atmosphere   : OPAL, ORPHICA, OHM, OSPREY, OCEANIC, OLE, ORACLE
  - Deep Atmosphere   : OVERDUB, OMBRE, OBSIDIAN, ODYSSEY, ORGANON, ORACLE, OWLFISH
  - Electric Atmosphere: OPTIC, ODDFELIX, OVERWORLD, OBLIQUE, ORIGAMI, OSTERIA, ONSET
  - Warm Drone        : ODDOSCAR, OSTERIA, OBLONG, OVERDUB, OHM, OTTONI, OBBLIGATO

Usage: python3 xpn_atmosphere_expansion_pack.py
Output: Presets/XOmnibus/Atmosphere/*.xometa  (skips existing files)
"""

import json
import os
import random

random.seed(2026)

PRESET_DIR = os.path.join(os.path.dirname(__file__), '..', 'Presets', 'XOmnibus', 'Atmosphere')

# ─── Name pools (2-3 words, ≤30 chars, evocative) ────────────────────────────

DAWN_NAMES = [
    'First Light', 'Gilt Horizon', 'Pale Shore', 'Amber Solstice',
    'Morning Veil', 'Saffron Drift', 'Waking Tide', 'Rose Canopy',
    'Dew Thread', 'Copper Mist', 'Lavender Gate', 'Solstice Calm',
    'Orchid Haze', 'Warm Arrival', 'Chorus of Herons',
]

DEEP_NAMES = [
    'Abyssal Calm', 'Trench Whisper', 'Midnight Column', 'Pressure Field',
    'Below Surface', 'Void Current', 'Dark Meridian', 'Benthic Drift',
    'Cold Descent', 'Fathom Pulse', 'Hadal Dream', 'Depth Mantra',
    'Silent Basin', 'Gravity Well', 'Oceanic Mass',
]

ELECTRIC_NAMES = [
    'Phosphor Bloom', 'Scan Line', 'Cathode Fog', 'Neon Passage',
    'CRT Haze', 'Voltage Shimmer', 'UV Lattice', 'Grid Glow',
    'Plasma Veil', 'Signal Burn', 'Static Canopy', 'Pixel Drift',
    'Electron Cloud', 'Charged Mist', 'Spark Array',
]

WARM_DRONE_NAMES = [
    'Amber Column', 'Hearth Sustain', 'Wool Blanket', 'Molasses Hum',
    'Resin Drone', 'Sunbaked Earth', 'Thick Horizon', 'Tallow Pad',
    'Ochre Field', 'Ember Still', 'Honey Sustain', 'Cork Resonance',
    'Terra Mass', 'Bronze Swell', 'Warm Monolith',
]

# ─── Engine pools per family ─────────────────────────────────────────────────

DAWN_ENGINES = ['OPAL', 'ORPHICA', 'OHM', 'OSPREY', 'OCEANIC', 'OLE', 'ORACLE']
DEEP_ENGINES = ['OVERDUB', 'OMBRE', 'OBSIDIAN', 'ODYSSEY', 'ORGANON', 'ORACLE', 'OWLFISH']
ELECTRIC_ENGINES = ['OPTIC', 'ODDFELIX', 'OVERWORLD', 'OBLIQUE', 'ORIGAMI', 'OSTERIA', 'ONSET']
WARM_DRONE_ENGINES = ['ODDOSCAR', 'OSTERIA', 'OBLONG', 'OVERDUB', 'OHM', 'OTTONI', 'OBBLIGATO']

# ─── Tag pools per family ────────────────────────────────────────────────────

DAWN_TAGS = ['atmosphere', 'pad', 'ambient', 'dawn', 'gentle']
DEEP_TAGS = ['atmosphere', 'pad', 'ambient', 'deep', 'vast']
ELECTRIC_TAGS = ['atmosphere', 'pad', 'ambient', 'electric', 'phosphor']
WARM_TAGS = ['atmosphere', 'drone', 'ambient', 'warm', 'thick']

# ─── Description pools per family ────────────────────────────────────────────

DAWN_DESCS = [
    "Soft light breaks across still water. Warmth builds slowly, movement a gentle tide.",
    "Morning haze lifts through reeds. A rising brightness, nothing urgent.",
    "The sky becomes gold before you notice. Space opens like held breath released.",
    "First birdsong over dew-covered grass. Bright, tender, unhurried.",
    "Copper warmth spreads across the canopy. The day arrives without announcement.",
    "Pale horizon giving way to saffron. A pad that breathes like morning itself.",
    "Lavender into amber. The transition between night and day held in sustained tone.",
    "Something golden and weightless. A chord built from early light.",
    "Orchid mist over a still lake. Warmth accumulates with no sharp edges.",
    "A slow opening — like watching a flower find the sun. Patient and luminous.",
    "The world before it fully wakes. Gilded texture, low movement, enormous space.",
    "Dew on spider silk. Bright but fragile, trembling with first warmth.",
    "Rose light on stone. Ancient and gentle. A pad with geological patience.",
    "Where the river meets the open sky. Warmth and space in perfect equilibrium.",
    "Herons calling across flooded fields at dawn. Tonal, spacious, alive.",
]

DEEP_DESCS = [
    "Three hundred meters below. No light, only pressure and resonance.",
    "The deep ocean at rest. Vast space, mineral warmth, absolute stillness.",
    "A trench that holds its own weather system. Dark and enormously patient.",
    "Below the thermocline. Where sound travels differently and time slows.",
    "Abyssal plain stretching past comprehension. A drone built from fathoms.",
    "Cold dark water moving in geological time. Low, warm, unreachable.",
    "Midnight zone drift. Bioluminescence too faint to count. Enormous space.",
    "Benthic hum carried upward through sediment layers. Dense and ancient.",
    "The sound of water above water, heard from below. Refracted, slow, still.",
    "Pressure at depth makes everything resonant. The hull groans beautifully.",
    "A cave system beneath the ocean floor. Darkness with its own harmonics.",
    "Fathoms of silence, broken only by current. Warm low frequencies endure.",
    "Down past the mesopelagic, into true dark. A pad with gravitational mass.",
    "Hadal zone gravity. Sound compresses. Everything hums at a lower register.",
    "The basin as instrument. Bottomless resonance, infinite sustain.",
]

ELECTRIC_DESCS = [
    "Phosphor coating on a CRT screen. Bright, buzzing, slightly toxic.",
    "Scan lines refreshing faster than perception. Electric texture, cool warmth.",
    "Neon tubes humming in an empty corridor. The sound of artificial light.",
    "Plasma field shimmering above stadium grass. High brightness, high movement.",
    "UV lattice spreading across a dark room. Electric, cool, textural.",
    "Voltage shimmer from a transformer bank. Industrial brightness, controlled drift.",
    "Static electricity before the lightning. Highly charged ambient tension.",
    "Grid interference mapped as tone. Every frequency a data artifact.",
    "Cathode glow warming nothing but the eye. Cool brightness, restless movement.",
    "Electron cloud above a supercooled magnet. Eerie, bright, faintly alive.",
    "The hum of a particle accelerator at rest. Waiting energy, vast and electric.",
    "Pixel decay in slow motion. A signal losing integrity in beautiful increments.",
    "Signal burn across a dark monitor. Phosphor memory of sound already gone.",
    "Charged atmosphere before a plasma storm. Everything bright, nothing warm.",
    "A rack of synthesizers nobody is playing. Electric presence without gesture.",
]

WARM_DRONE_DESCS = [
    "Amber resin hardened around a sustained note. Dense, immovable, warm.",
    "A fireplace turned to tone. Thick, radiant, absolutely still.",
    "Wool blanket of low harmonics. Heavy warmth, near-zero movement.",
    "Molasses at room temperature. Sweet, viscous, impossible to rush.",
    "An earthen vessel resonating in a warm room. Ancient and comfortable.",
    "Sunbaked stone holding the heat of the afternoon. Dense mineral warmth.",
    "Bronze singing bowl at rest. Rich overtones decaying over geological time.",
    "Tallow and beeswax — organic warmth that refuses to move.",
    "Ochre clay walls absorbing sound. A room that breathes as one drone.",
    "The warmest possible pad. Density without aggression. Complete stillness.",
    "Honey over a low note. Slow, thick, impossible to separate from comfort.",
    "A cork floor in a warm room. Soft density, perfect absorption.",
    "Terra cotta resonance. Earthy and rich. A drone that smells like afternoon.",
    "Thick horizon before the rain. Warm pressure, heavy and patient.",
    "Monolith of warm frequency. Singular, dense, and completely still.",
]


# ─── Helpers ─────────────────────────────────────────────────────────────────

def clamp(v, lo, hi):
    return max(lo, min(hi, v))

def rng(lo, hi):
    return round(random.uniform(lo, hi), 3)

def pick(seq):
    return random.choice(seq)


def dawn_dna(i):
    """brightness 0.6-0.8, warmth 0.6-0.8, movement 0.1-0.3"""
    return {
        'brightness': rng(0.6, 0.8),
        'warmth':     rng(0.6, 0.8),
        'movement':   rng(0.1, 0.3),
        'density':    rng(0.3, 0.6),
        'space':      rng(0.6, 0.9),
        'aggression': rng(0.0, 0.15),
    }

def deep_dna(i):
    """brightness 0.1-0.3, warmth 0.4-0.6, space 0.8-1.0"""
    return {
        'brightness': rng(0.1, 0.3),
        'warmth':     rng(0.4, 0.6),
        'movement':   rng(0.05, 0.25),
        'density':    rng(0.5, 0.8),
        'space':      rng(0.8, 1.0),
        'aggression': rng(0.0, 0.1),
    }

def electric_dna(i):
    """brightness 0.8-1.0, warmth 0.1-0.3, movement 0.5-0.7"""
    return {
        'brightness': rng(0.8, 1.0),
        'warmth':     rng(0.1, 0.3),
        'movement':   rng(0.5, 0.7),
        'density':    rng(0.3, 0.6),
        'space':      rng(0.4, 0.7),
        'aggression': rng(0.05, 0.25),
    }

def warm_drone_dna(i):
    """warmth 0.8-1.0, density 0.6-0.9, movement 0.0-0.15"""
    return {
        'brightness': rng(0.2, 0.5),
        'warmth':     rng(0.8, 1.0),
        'movement':   rng(0.0, 0.15),
        'density':    rng(0.6, 0.9),
        'space':      rng(0.4, 0.7),
        'aggression': rng(0.0, 0.1),
    }


def macros_from_dna(dna, coupling=0.0):
    """MOVEMENT macro mirrors movement DNA; CHARACTER ~ brightness/warmth blend."""
    character = round((dna['brightness'] * 0.5 + dna['warmth'] * 0.5), 3)
    movement  = round(dna['movement'], 3)
    space     = round(dna['space'], 3)
    return {
        'CHARACTER': character,
        'MOVEMENT':  movement,
        'COUPLING':  round(coupling, 3),
        'SPACE':     space,
    }


def make_preset(name, engine, dna_fn, tags, descs, idx):
    dna = dna_fn(idx)
    macros = macros_from_dna(dna)
    desc = descs[idx % len(descs)]
    return {
        'name':       name,
        'version':    '1.0',
        'engines':    [engine],
        'mood':       'Atmosphere',
        'macros':     macros,
        'sonic_dna':  dna,
        'parameters': {},
        'tags':       tags[:],
        'description': desc,
    }


def preset_filename(name):
    safe = name.replace('/', '-').replace('\\', '-')
    return safe + '.xometa'


def existing_names(directory):
    names = set()
    if not os.path.isdir(directory):
        return names
    for fn in os.listdir(directory):
        if fn.endswith('.xometa'):
            names.add(fn[:-len('.xometa')])
    return names


# ─── Families ────────────────────────────────────────────────────────────────

FAMILIES = [
    {
        'label':   'Dawn Atmosphere',
        'count':   15,
        'names':   DAWN_NAMES,
        'engines': DAWN_ENGINES,
        'dna_fn':  dawn_dna,
        'tags':    DAWN_TAGS,
        'descs':   DAWN_DESCS,
    },
    {
        'label':   'Deep Atmosphere',
        'count':   15,
        'names':   DEEP_NAMES,
        'engines': DEEP_ENGINES,
        'dna_fn':  deep_dna,
        'tags':    DEEP_TAGS,
        'descs':   DEEP_DESCS,
    },
    {
        'label':   'Electric Atmosphere',
        'count':   15,
        'names':   ELECTRIC_NAMES,
        'engines': ELECTRIC_ENGINES,
        'dna_fn':  electric_dna,
        'tags':    ELECTRIC_TAGS,
        'descs':   ELECTRIC_DESCS,
    },
    {
        'label':   'Warm Drone',
        'count':   15,
        'names':   WARM_DRONE_NAMES,
        'engines': WARM_DRONE_ENGINES,
        'dna_fn':  warm_drone_dna,
        'tags':    WARM_TAGS,
        'descs':   WARM_DRONE_DESCS,
    },
]


# ─── Main ─────────────────────────────────────────────────────────────────────

def main():
    os.makedirs(PRESET_DIR, exist_ok=True)
    existing = existing_names(PRESET_DIR)

    written  = 0
    skipped  = 0

    for family in FAMILIES:
        label   = family['label']
        count   = family['count']
        names   = family['names']
        engines = family['engines']
        dna_fn  = family['dna_fn']
        tags    = family['tags']
        descs   = family['descs']

        print(f'\n{label} ({count} presets):')

        for i in range(count):
            name   = names[i % len(names)]
            engine = engines[i % len(engines)]
            preset = make_preset(name, engine, dna_fn, tags, descs, i)

            fn = preset_filename(name)
            if name in existing or fn[:-len('.xometa')] in existing:
                print(f'  skip  {name}')
                skipped += 1
                continue

            out_path = os.path.join(PRESET_DIR, fn)
            with open(out_path, 'w') as f:
                json.dump(preset, f, indent=2)
                f.write('\n')

            existing.add(name)
            print(f'  write {name}  [{engine}]')
            written += 1

    print(f'\nDone — {written} written, {skipped} skipped.')


if __name__ == '__main__':
    main()
