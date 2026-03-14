#!/usr/bin/env python3
"""
Generate 530 .xometa presets for the Family Constellation engines.
100 per engine (Ohm, Orphica, Obbligato, Ottoni, Ole) + 30 Family Dinner.
Distribution: ~16 per mood + 4 hero presets per engine.

Usage: python3 generate_constellation_presets.py
Output: Presets/XOmnibus/{mood}/*.xometa
"""

import json, os, random, math
from datetime import date

random.seed(42)  # reproducible

PRESET_DIR = os.path.join(os.path.dirname(__file__), '..', 'Presets', 'XOmnibus')
MOODS = ['Foundation', 'Atmosphere', 'Entangled', 'Prism', 'Flux', 'Aether']
TODAY = str(date.today())

# ─── Naming pools per engine (evocative, 2-3 words, ≤30 chars) ───────────────

OHM_NAMES = {
    'Foundation': ['Campfire Circle', 'Banjo Porch', 'Mandolin Dawn', 'Dobro Slide', 'Fiddle Creek',
                   'Guitar Meadow', 'Kalimba Rain', 'Sitar Dusk', 'Djembe Roots', 'Harmonica Dust',
                   'Peaceful Valley', 'Folk Ember', 'Porch Swing', 'String Cottage', 'Gentle Strum', 'Morning Jam'],
    'Atmosphere': ['Meadow Mist', 'Commune Haze', 'Drift Pasture', 'Fog Valley', 'Still Pond',
                   'Evening Grass', 'Manatee Float', 'Warm Current', 'Surface Calm', 'Tide Rest',
                   'Amber Glow', 'Horizon Breath', 'Slow River', 'Moss Blanket', 'Dew Field', 'Twilight Hum'],
    'Entangled': ['Family Supper', 'Marriage Duet', 'Shared Hearth', 'Resonance Wed', 'Groove Legacy',
                  'Father Son Jam', 'Pentagon Pulse', 'Commune Bridge', 'Coupled Folk', 'Tangled Roots',
                  'Woven String', 'Thread Circle', 'Bound Melody', 'Joint Venture', 'Pair Dance', 'Unity Ring'],
    'Prism': ['Crystal Banjo', 'Glass Mandolin', 'Prism Fiddle', 'Light Kalimba', 'Shimmer Sitar',
              'Bright Meadow', 'Sun Harmonica', 'Clear Stream', 'Diamond Strum', 'Facet Folk',
              'Refracted Dawn', 'Spectrum Pick', 'Rainbow Drift', 'Glint String', 'Sparkle Porch', 'Lucid Jam'],
    'Flux': ['Meddling Tide', 'In-Law Storm', 'Obed Arrives', 'Wedge Drive', 'Commune Shift',
             'FM Folk Clash', 'Theremin Creep', 'Spectral Drift', 'Grain Scatter', 'Phase Dad',
             'Pulse Meadow', 'Moving Roots', 'Shifting Ground', 'Morph Folk', 'Evolve Jam', 'Flow State'],
    'Aether': ['The Miracle', 'Nuclear Jam', 'Full Pentagon', 'Obed Transcends', 'Glass Cathedral',
               'Spectral Freeze', 'Quantum Folk', 'Atomic Ratio', 'Plutonium Haze', 'Manatee Dream',
               'Beyond Commune', 'Infinite Meadow', 'Cosmos Folk', 'Ethereal Dad', 'Void Harmony', 'Spirit Drift'],
}

ORPHICA_NAMES = {
    'Foundation': ['Harp Awakens', 'Nylon Touch', 'Steel Pluck', 'Crystal Bell', 'Light Thread',
                   'Simple String', 'Clean Harp', 'Pure Tone', 'Bridge Tap', 'Nut Glide',
                   'Open Chord', 'Root Note', 'Bass Harp', 'Treble Wire', 'Init Goddess', 'Naked String'],
    'Atmosphere': ['Siren Mist', 'Deep Ocean', 'Surface Calm', 'Moonlit Harp', 'Tidal Breath',
                   'Siphon Drift', 'Colony Float', 'Zooid Haze', 'Gentle Fracture', 'Pluck Echo',
                   'Still Water', 'Night Harp', 'Fog String', 'Dew Shimmer', 'Mist Pluck', 'Cloud Harp'],
    'Entangled': ['Resonance Wed', 'Sister Chord', 'Mother Thread', 'Divine Gift', 'Surface Link',
                  'Coupled Harp', 'String Weave', 'Harmonic Lock', 'Zooid Chain', 'Twin Pluck',
                  'Mirror String', 'Pair Shimmer', 'Bond Harp', 'Woven Tone', 'Thread Merge', 'Linked Bell'],
    'Prism': ['Crystal Cascade', 'Light Scatter', 'Prism Harp', 'Diamond String', 'Facet Pluck',
              'Rainbow Wire', 'Sparkle Tone', 'Glint Bell', 'Bright Colony', 'Shimmer Burst',
              'Sun String', 'Clear Crystal', 'Glass Harp', 'Refracted Pluck', 'Gem Tone', 'Lucid Wire'],
    'Flux': ['Stutter Harp', 'Scatter Grain', 'Freeze Frame', 'Reverse Pluck', 'Fracture Pulse',
             'Micro Glitch', 'Grain Storm', 'Shatter String', 'Dissolve Wire', 'Morph Pluck',
             'Shift Tone', 'Evolve Harp', 'Pulse Crystal', 'Phase String', 'Flux Bell', 'Moving Light'],
    'Aether': ['Siren Song', 'Divine Light', 'Goddess Ascends', 'Cosmic Harp', 'Infinite String',
               'Void Shimmer', 'Spirit Pluck', 'Transcend Wire', 'Ethereal Bell', 'Beyond Surface',
               'Quantum Harp', 'Astral String', 'Nebula Pluck', 'Celestial Tone', 'Dream Cascade', 'Star Wire'],
}

OBBLIGATO_NAMES = {
    'Foundation': ['Flute Breath', 'Reed Whisper', 'Pan Melody', 'Clarinet Pure', 'Oboe Warm',
                   'Bansuri Song', 'Ney Prayer', 'Duduk Earth', 'Shakuhachi Zen', 'Brothers Meet',
                   'Simple Wind', 'Clean Reed', 'Open Pipe', 'Root Breath', 'Init Brothers', 'Naked Wind'],
    'Atmosphere': ['Wind Cavern', 'Reed Fog', 'Flute Cloud', 'Breath Mist', 'Air Current',
                   'Deep Breath', 'Night Reed', 'Dusk Flute', 'Fog Pipe', 'Still Air',
                   'Cavern Echo', 'Mountain Mist', 'Valley Breath', 'Ocean Breeze', 'Soft Wind', 'Haze Pipe'],
    'Entangled': ['Bond Harmony', 'Brothers Duo', 'Twin Breath', 'Paired Wind', 'Flute Reed Merge',
                  'Wind Weave', 'Coupled Air', 'Tangled Breath', 'Joint Song', 'United Pipe',
                  'Thread Wind', 'Mirror Breath', 'Linked Reed', 'Woven Air', 'Bond Bridge', 'Duo Flow'],
    'Prism': ['Bright Flute', 'Crystal Reed', 'Light Pipe', 'Sun Wind', 'Clear Breath',
              'Shimmer Flute', 'Sparkle Reed', 'Glass Pipe', 'Diamond Air', 'Facet Wind',
              'Prism Breath', 'Rainbow Reed', 'Glint Pipe', 'Refracted Air', 'Gem Flute', 'Lucid Wind'],
    'Flux': ['Bond Storm', 'Fight Phase', 'Cry Resolve', 'Dare Dance', 'Play Chase',
             'Mischief Pulse', 'Shifting Bond', 'Evolve Breath', 'Morph Wind', 'Phase Reed',
             'Moving Pipe', 'Flux Duo', 'Pulse Brothers', 'Drift Air', 'Wave Wind', 'Flow Breath'],
    'Aether': ['Brothers Cry', 'Transcend Bond', 'Beyond Wind', 'Cosmic Breath', 'Spirit Flute',
               'Ethereal Reed', 'Infinite Pipe', 'Void Air', 'Dream Duo', 'Quantum Wind',
               'Astral Breath', 'Nebula Reed', 'Celestial Pipe', 'Star Wind', 'Divine Duo', 'Soul Breath'],
}

OTTONI_NAMES = {
    'Foundation': ['Conch Call', 'Shofar Dawn', 'Trumpet Pure', 'Horn Warm', 'Tuba Ground',
                   'Sax Root', 'Brass Clean', 'Toddler Bleat', 'Tween Bright', 'Teen Power',
                   'Simple Brass', 'Open Horn', 'Root Buzz', 'Init Cousins', 'Naked Brass', 'Bell Tone'],
    'Atmosphere': ['Lake Mist', 'Cold Current', 'Deep Brass', 'Fog Horn', 'Still Lake',
                   'Night Brass', 'Dusk Trumpet', 'Mountain Horn', 'Valley Tuba', 'Soft Buzz',
                   'Haze Brass', 'Cloud Horn', 'Mist Sax', 'Dew Trumpet', 'Calm Lake', 'Quiet Brass'],
    'Entangled': ['Cousin Reunion', 'Age Bridge', 'Growth Pair', 'Warm Exchange', 'Cold Meets Warm',
                  'Coupled Brass', 'Tangled Horn', 'Joint Buzz', 'United Sax', 'Thread Trumpet',
                  'Mirror Brass', 'Linked Horn', 'Woven Buzz', 'Bond Trumpet', 'Bridge Brass', 'Duo Horn'],
    'Prism': ['Crystal Horn', 'Light Brass', 'Sun Trumpet', 'Clear Buzz', 'Shimmer Sax',
              'Bright Tuba', 'Glass Horn', 'Diamond Brass', 'Facet Trumpet', 'Sparkle Buzz',
              'Prism Horn', 'Rainbow Brass', 'Glint Trumpet', 'Refracted Sax', 'Gem Horn', 'Lucid Brass'],
    'Flux': ['Grow Sweep', 'Age Morph', 'Foreign Drift', 'Alien Brass', 'Cold Shift',
             'Evolve Horn', 'Phase Trumpet', 'Pulse Buzz', 'Moving Sax', 'Wave Brass',
             'Flux Horn', 'Shift Trumpet', 'Morph Tuba', 'Drift Sax', 'Flow Brass', 'Tide Horn'],
    'Aether': ['Cousins Arrive', 'Teen Virtuoso', 'Beyond Grow', 'Cosmic Brass', 'Spirit Horn',
               'Ethereal Trumpet', 'Infinite Buzz', 'Void Sax', 'Dream Brass', 'Quantum Horn',
               'Astral Trumpet', 'Nebula Sax', 'Celestial Brass', 'Star Horn', 'Divine Buzz', 'Soul Brass'],
}

OLE_NAMES = {
    'Foundation': ['Tres Groove', 'Berimbau Wire', 'Charango Quick', 'Aunt One Lead', 'Aunt Two Bow',
                   'Aunt Three Trem', 'Simple Strum', 'Clean Latin', 'Root Chord', 'Open String',
                   'Init Sisters', 'Naked Strum', 'Bass Latin', 'Treble Wire', 'Warm Tres', 'Bright Char'],
    'Atmosphere': ['Isla Breeze', 'Tropical Mist', 'Reef Calm', 'Crystal Water', 'Coral Shade',
                   'Night Latin', 'Dusk Strum', 'Fog Wire', 'Still Reef', 'Gentle Tres',
                   'Soft Charango', 'Haze Latin', 'Cloud Strum', 'Mist Wire', 'Dew String', 'Calm Reef'],
    'Entangled': ['Alliance Bond', 'Sisters Unite', 'Drama Bridge', 'Coupled Latin', 'Tangled Wire',
                  'Joint Strum', 'United Tres', 'Thread Latin', 'Mirror Strum', 'Linked Wire',
                  'Woven Chord', 'Bond Strum', 'Bridge Wire', 'Duo Latin', 'Pair Strum', 'Twin Wire'],
    'Prism': ['Crystal Tres', 'Light Strum', 'Sun Latin', 'Clear Wire', 'Shimmer Chord',
              'Bright Charango', 'Glass Berimbau', 'Diamond Strum', 'Facet Wire', 'Sparkle Latin',
              'Prism Strum', 'Rainbow Wire', 'Glint Chord', 'Refracted Strum', 'Gem Wire', 'Lucid Latin'],
    'Flux': ['Drama Rising', 'Sides Rotate', 'Alliance Shift', 'Fuego Pulse', 'Husband Entry',
             'Oud Arrives', 'Bouzouki Wave', 'Pin Whisper', 'Evolve Latin', 'Phase Strum',
             'Moving Wire', 'Flux Sisters', 'Pulse Chord', 'Drift Latin', 'Wave Strum', 'Flow Wire'],
    'Aether': ['La Mesa', 'Full Drama', 'Six Voices', 'Cosmic Latin', 'Spirit Strum',
               'Ethereal Wire', 'Infinite Chord', 'Void Latin', 'Dream Sisters', 'Quantum Strum',
               'Astral Wire', 'Nebula Latin', 'Celestial Strum', 'Star Wire', 'Divine Sisters', 'Soul Latin'],
}

# ─── Parameter templates per engine ──────────────────────────────────────────

def clamp(v, lo=0.0, hi=1.0):
    return max(lo, min(hi, v))

def vary(base, spread=0.15):
    return clamp(base + random.uniform(-spread, spread))

def mood_dna(mood):
    """Base DNA profile per mood, then varied per preset."""
    profiles = {
        'Foundation': {'brightness':0.4,'warmth':0.7,'movement':0.15,'density':0.4,'space':0.2,'aggression':0.1},
        'Atmosphere': {'brightness':0.3,'warmth':0.6,'movement':0.25,'density':0.3,'space':0.8,'aggression':0.05},
        'Entangled':  {'brightness':0.5,'warmth':0.5,'movement':0.5,'density':0.6,'space':0.4,'aggression':0.2},
        'Prism':      {'brightness':0.8,'warmth':0.3,'movement':0.35,'density':0.35,'space':0.5,'aggression':0.1},
        'Flux':       {'brightness':0.5,'warmth':0.4,'movement':0.8,'density':0.5,'space':0.3,'aggression':0.4},
        'Aether':     {'brightness':0.6,'warmth':0.5,'movement':0.6,'density':0.7,'space':0.7,'aggression':0.3},
    }
    base = profiles[mood]
    return {k: round(vary(v, 0.12), 2) for k, v in base.items()}

def ohm_params(mood, idx):
    is_atmo = mood in ('Atmosphere', 'Aether')
    is_flux = mood in ('Flux', 'Aether')
    is_prism = mood == 'Prism'
    inst = idx % 9
    return {
        'ohm_dadInstrument': inst, 'ohm_dadLevel': vary(0.8, 0.1),
        'ohm_pluckBrightness': vary(0.6 if is_prism else 0.4, 0.15),
        'ohm_bowPressure': vary(0.6, 0.2) if inst == 4 else 0.0,
        'ohm_bowSpeed': vary(0.5, 0.2) if inst == 4 else 0.5,
        'ohm_bodyMaterial': idx % 4,
        'ohm_sympatheticAmt': vary(0.4 if is_atmo else 0.25, 0.1),
        'ohm_driftRate': vary(0.12, 0.05), 'ohm_driftDepth': vary(3.0 if is_atmo else 2.0, 1.5),
        'ohm_damping': vary(0.993, 0.003),
        'ohm_inlawLevel': vary(0.5 if is_flux else 0.0, 0.2),
        'ohm_thereminScale': vary(1.0, 0.3), 'ohm_thereminWobble': vary(0.3, 0.15),
        'ohm_glassBrightness': vary(0.5, 0.2),
        'ohm_spectralFreeze': vary(0.4 if is_atmo else 0.0, 0.15),
        'ohm_grainSize': vary(80, 40), 'ohm_grainDensity': vary(12, 6), 'ohm_grainScatter': vary(0.3, 0.15),
        'ohm_obedLevel': vary(0.4 if is_flux else 0.0, 0.2),
        'ohm_fmRatioPreset': idx % 8, 'ohm_fmIndex': vary(2.0, 1.5),
        'ohm_fmAttack': vary(0.01, 0.005), 'ohm_fmDecay': vary(0.3, 0.15),
        'ohm_meddlingThresh': vary(0.5, 0.15), 'ohm_communeAbsorb': vary(0.5, 0.2),
        'ohm_reverbMix': vary(0.4 if is_atmo else 0.15, 0.1),
        'ohm_delayTime': vary(0.4, 0.2), 'ohm_delayFeedback': vary(0.35, 0.15),
        'ohm_macroJam': vary(0.5, 0.2), 'ohm_macroMeddling': vary(0.5 if is_flux else 0.1, 0.2),
        'ohm_macroCommune': vary(0.5 if is_atmo else 0.2, 0.2), 'ohm_macroMeadow': vary(0.5 if is_atmo else 0.2, 0.15),
    }

def orphica_params(mood, idx):
    is_atmo = mood in ('Atmosphere', 'Aether')
    is_flux = mood in ('Flux', 'Aether')
    is_prism = mood == 'Prism'
    return {
        'orph_stringMaterial': idx % 4, 'orph_pluckBrightness': vary(0.7 if is_prism else 0.45, 0.15),
        'orph_pluckPosition': vary(0.5, 0.2), 'orph_stringCount': min(6, max(1, int(vary(4, 1.5)))),
        'orph_bodySize': vary(0.5, 0.2), 'orph_sympatheticAmt': vary(0.4 if is_atmo else 0.25, 0.1),
        'orph_damping': vary(0.994, 0.003), 'orph_driftRate': vary(0.1, 0.04), 'orph_driftDepth': vary(3, 1.5),
        'orph_microMode': idx % 4, 'orph_microRate': vary(5 if is_flux else 2, 3),
        'orph_microSize': vary(50, 30), 'orph_microDensity': vary(4, 3),
        'orph_microScatter': vary(0.4 if is_flux else 0.2, 0.15),
        'orph_microMix': vary(0.5 if is_flux else 0.0, 0.2),
        'orph_crossoverNote': int(vary(60, 8)), 'orph_crossoverBlend': int(vary(6, 3)),
        'orph_lowLevel': vary(0.8, 0.1), 'orph_highLevel': vary(0.8, 0.1),
        'orph_subAmount': vary(0.2 if mood == 'Foundation' else 0.0, 0.1),
        'orph_tapeSat': vary(0.15, 0.1), 'orph_darkDelayTime': vary(0.4, 0.2),
        'orph_darkDelayFb': vary(0.3, 0.15), 'orph_deepPlateMix': vary(0.3 if is_atmo else 0.1, 0.1),
        'orph_shimmerMix': vary(0.3 if is_prism else 0.1, 0.15),
        'orph_microDelayTime': vary(5, 4), 'orph_spectralSmear': vary(0.3 if is_atmo else 0.0, 0.15),
        'orph_crystalChorusRate': vary(1.0, 0.5), 'orph_crystalChorusDpth': vary(0.3, 0.15),
        'orph_macroPluck': vary(0.5, 0.2), 'orph_macroFracture': vary(0.5 if is_flux else 0.0, 0.2),
        'orph_macroSurface': vary(0.5, 0.2), 'orph_macroDivine': vary(0.5 if is_atmo else 0.15, 0.2),
    }

def obbligato_params(mood, idx):
    is_atmo = mood in ('Atmosphere', 'Aether')
    is_flux = mood in ('Flux', 'Aether')
    return {
        'obbl_breathA': vary(0.7, 0.15), 'obbl_embouchureA': vary(0.5, 0.2),
        'obbl_airFlutterA': vary(0.2, 0.1), 'obbl_instrumentA': idx % 8,
        'obbl_bodySizeA': vary(0.5, 0.2),
        'obbl_breathB': vary(0.7, 0.15), 'obbl_reedStiffness': vary(0.5, 0.2),
        'obbl_reedBite': vary(0.3, 0.15), 'obbl_instrumentB': idx % 8,
        'obbl_bodySizeB': vary(0.5, 0.2),
        'obbl_voiceRouting': idx % 5, 'obbl_damping': vary(0.994, 0.003),
        'obbl_sympatheticAmt': vary(0.35, 0.1), 'obbl_driftRate': vary(0.1, 0.04),
        'obbl_driftDepth': vary(3, 1.5),
        'obbl_bondStage': vary(0.5 if is_flux else 0.1, 0.3),
        'obbl_bondIntensity': vary(0.5, 0.2), 'obbl_bondRate': vary(0.2, 0.1),
        'obbl_fxAChorus': vary(0.3, 0.15), 'obbl_fxABrightDelay': vary(0.2, 0.1),
        'obbl_fxAPlate': vary(0.3 if is_atmo else 0.15, 0.1), 'obbl_fxAExciter': vary(0.1, 0.08),
        'obbl_fxBPhaser': vary(0.2, 0.1), 'obbl_fxBDarkDelay': vary(0.3, 0.15),
        'obbl_fxBSpring': vary(0.2, 0.1), 'obbl_fxBTapeSat': vary(0.1, 0.08),
        'obbl_macroBreath': vary(0.5, 0.2), 'obbl_macroBond': vary(0.5 if is_flux else 0.1, 0.3),
        'obbl_macroMischief': vary(0.3 if is_flux else 0.0, 0.15), 'obbl_macroWind': vary(0.4 if is_atmo else 0.2, 0.15),
    }

def ottoni_params(mood, idx):
    is_atmo = mood in ('Atmosphere', 'Aether')
    is_flux = mood in ('Flux', 'Aether')
    is_prism = mood == 'Prism'
    return {
        'otto_toddlerLevel': vary(0.5, 0.2), 'otto_toddlerPressure': vary(0.3, 0.15),
        'otto_toddlerInst': idx % 6,
        'otto_tweenLevel': vary(0.5, 0.2), 'otto_tweenEmbouchure': vary(0.5, 0.2),
        'otto_tweenValve': vary(0.5, 0.2), 'otto_tweenInst': idx % 6,
        'otto_teenLevel': vary(0.5, 0.2), 'otto_teenEmbouchure': vary(0.7, 0.15),
        'otto_teenBore': vary(0.5, 0.2), 'otto_teenInst': idx % 10,
        'otto_teenVibratoRate': vary(5, 1.5), 'otto_teenVibratoDepth': vary(0.3, 0.15),
        'otto_damping': vary(0.994, 0.003), 'otto_sympatheticAmt': vary(0.3, 0.1),
        'otto_driftRate': vary(0.1, 0.04), 'otto_driftDepth': vary(3, 1.5),
        'otto_foreignStretch': vary(0.3 if is_flux else 0.0, 0.15),
        'otto_foreignDrift': vary(0.2 if is_flux else 0.0, 0.1),
        'otto_foreignCold': vary(0.3 if is_flux else 0.0, 0.15),
        'otto_reverbSize': vary(0.4 if is_atmo else 0.2, 0.15),
        'otto_chorusRate': vary(1.0, 0.5), 'otto_driveAmount': vary(0.15 if is_flux else 0.0, 0.1),
        'otto_delayMix': vary(0.25, 0.15),
        'otto_macroEmbouchure': vary(0.5, 0.2), 'otto_macroGrow': vary(0.5, 0.3),
        'otto_macroForeign': vary(0.4 if is_flux else 0.0, 0.2), 'otto_macroLake': vary(0.4 if is_atmo else 0.2, 0.15),
    }

def ole_params(mood, idx):
    is_atmo = mood in ('Atmosphere', 'Aether')
    is_flux = mood in ('Flux', 'Aether')
    return {
        'ole_aunt1Level': vary(0.7, 0.15), 'ole_aunt1StrumRate': vary(8, 4),
        'ole_aunt1Brightness': vary(0.6, 0.15),
        'ole_aunt2Level': vary(0.7, 0.15), 'ole_aunt2CoinPress': vary(0.2 if is_flux else 0.0, 0.15),
        'ole_aunt2GourdSize': vary(0.5, 0.2),
        'ole_aunt3Level': vary(0.7, 0.15), 'ole_aunt3Tremolo': vary(12, 5),
        'ole_aunt3Brightness': vary(0.5, 0.15),
        'ole_damping': vary(0.994, 0.003), 'ole_sympatheticAmt': vary(0.3, 0.1),
        'ole_driftRate': vary(0.1, 0.04), 'ole_driftDepth': vary(3, 1.5),
        'ole_allianceConfig': idx % 3, 'ole_allianceBlend': vary(0.5, 0.2),
        'ole_husbandOudLevel': vary(0.4, 0.2) if is_flux else 0.0,
        'ole_husbandBouzLevel': vary(0.3, 0.2) if is_flux else 0.0,
        'ole_husbandPinLevel': vary(0.3, 0.2) if is_flux else 0.0,
        'ole_macroFuego': vary(0.5, 0.2), 'ole_macroDrama': vary(0.5 if is_flux else 0.1, 0.3),
        'ole_macroSides': vary(0.3 if is_flux else 0.0, 0.2), 'ole_macroIsla': vary(0.4 if is_atmo else 0.2, 0.15),
    }

# ─── Engine configs ──────────────────────────────────────────────────────────

ENGINES = {
    'Ohm': {'names': OHM_NAMES, 'params': ohm_params, 'macros': ['JAM','MEDDLING','COMMUNE','MEADOW'],
            'tags_base': ['physical-modeling','folk','waveguide']},
    'Orphica': {'names': ORPHICA_NAMES, 'params': orphica_params, 'macros': ['PLUCK','FRACTURE','SURFACE','DIVINE'],
                'tags_base': ['harp','microsound','granular','waveguide']},
    'Obbligato': {'names': OBBLIGATO_NAMES, 'params': obbligato_params, 'macros': ['BREATH','BOND','MISCHIEF','WIND'],
                  'tags_base': ['wind','flute','reed','waveguide']},
    'Ottoni': {'names': OTTONI_NAMES, 'params': ottoni_params, 'macros': ['EMBOUCHURE','GROW','FOREIGN','LAKE'],
               'tags_base': ['brass','sax','waveguide']},
    'Ole': {'names': OLE_NAMES, 'params': ole_params, 'macros': ['FUEGO','DRAMA','SIDES','ISLA'],
            'tags_base': ['latin','strummed','world','waveguide']},
}

# ─── Family Dinner preset names ──────────────────────────────────────────────

FAMILY_DINNER_NAMES = [
    'Wedding Toast', 'Thanksgiving Table', 'Family Reunion', 'Sunday Dinner', 'Birthday Song',
    'Holiday Gather', 'Backyard Grill', 'Kitchen Dance', 'Porch Stories', 'Campfire Ring',
    'Grace Before Meal', 'Cousin Visit', 'Aunt Arrival', 'Uncle Jam', 'Sister Harmony',
    'Father Blessing', 'Mother Prayer', 'Family Portrait', 'Generational', 'Heritage Blend',
    'Love Language', 'Shared Meal', 'Table Talk', 'Old Photos', 'New Year Toast',
    'First Dance', 'Last Song', 'Morning After', 'Road Trip', 'Coming Home',
]

def make_preset(name, mood, engine_id, params, macros, tags_base, idx):
    """Create a single .xometa preset dict."""
    dna = mood_dna(mood)
    mood_tags = {
        'Foundation': ['foundation','clean','simple'],
        'Atmosphere': ['atmosphere','pad','ambient'],
        'Entangled': ['entangled','coupled','multi'],
        'Prism': ['prism','bright','crystalline'],
        'Flux': ['flux','movement','evolving'],
        'Aether': ['aether','experimental','deep'],
    }
    tags = tags_base[:2] + mood_tags[mood][:2] + [mood.lower()]

    # Round all float params to 3 decimal places
    rounded = {}
    for k, v in params.items():
        rounded[k] = round(v, 3) if isinstance(v, float) else v

    return {
        'schema_version': 1,
        'name': name,
        'mood': mood,
        'engines': [engine_id],
        'author': 'Exo Meta — Family Constellation',
        'version': '1.0.0',
        'tags': tags,
        'macroLabels': macros,
        'couplingIntensity': 'None',
        'created': TODAY,
        'dna': dna,
        'parameters': {engine_id: rounded},
        'coupling': None,
    }

def make_family_preset(name, idx):
    """Create a Family Dinner multi-engine preset."""
    mood = MOODS[idx % 6]
    dna = mood_dna(mood)
    # Pick 2-3 engines for the dinner
    engine_count = 2 + (idx % 2)
    engine_keys = list(ENGINES.keys())
    random.shuffle(engine_keys)
    selected = engine_keys[:engine_count]

    all_params = {}
    for eid in selected:
        cfg = ENGINES[eid]
        all_params[eid] = {k: (round(v, 3) if isinstance(v, float) else v)
                           for k, v in cfg['params'](mood, idx).items()}

    # Coupling between first two engines
    coupling_types = ['AmpToFilter','PitchToPitch','FilterToFilter','AudioToFM','EnvToMorph']
    coupling = [{
        'source': selected[0],
        'destination': selected[1],
        'type': coupling_types[idx % len(coupling_types)],
        'amount': round(vary(0.4, 0.15), 2),
    }]

    return {
        'schema_version': 1,
        'name': name,
        'mood': mood,
        'engines': selected,
        'author': 'Exo Meta — Family Dinner',
        'version': '1.0.0',
        'tags': ['family','dinner','constellation','multi-engine', mood.lower()],
        'macroLabels': ['CHARACTER','MOVEMENT','COUPLING','SPACE'],
        'couplingIntensity': 'Medium',
        'created': TODAY,
        'dna': dna,
        'parameters': all_params,
        'coupling': coupling,
    }

def sanitize_filename(name):
    return name.replace(' ', '_').replace("'", '').replace('/', '_').replace(':', '') + '.xometa'

def main():
    total = 0
    os.makedirs(os.path.join(PRESET_DIR, 'Family'), exist_ok=True)
    for mood in MOODS:
        os.makedirs(os.path.join(PRESET_DIR, mood), exist_ok=True)

    # Generate 100 per engine
    for engine_id, cfg in ENGINES.items():
        names_pool = cfg['names']
        param_fn = cfg['params']
        macros = cfg['macros']
        tags = cfg['tags_base']
        count = 0

        for mood in MOODS:
            mood_names = names_pool[mood]
            for i, name in enumerate(mood_names):
                full_name = f"{engine_id}_{name}"
                params = param_fn(mood, count)
                preset = make_preset(name, mood, engine_id, params, macros, tags, count)
                fname = sanitize_filename(f"{engine_id}_{name}")
                path = os.path.join(PRESET_DIR, mood, fname)
                with open(path, 'w') as f:
                    json.dump(preset, f, indent=2)
                count += 1
                total += 1

        # Fill to 100 with extras if needed (cycle through moods)
        extra_idx = 0
        while count < 100:
            mood = MOODS[extra_idx % 6]
            name = f"{engine_id} Var {count+1}"
            params = param_fn(mood, count)
            preset = make_preset(name, mood, engine_id, params, macros, tags, count)
            fname = sanitize_filename(f"{engine_id}_{name}")
            path = os.path.join(PRESET_DIR, mood, fname)
            with open(path, 'w') as f:
                json.dump(preset, f, indent=2)
            count += 1
            total += 1
            extra_idx += 1

        print(f"  {engine_id}: {count} presets")

    # Generate 30 Family Dinner presets
    for i, name in enumerate(FAMILY_DINNER_NAMES):
        preset = make_family_preset(name, i)
        fname = sanitize_filename(f"Family_{name}")
        path = os.path.join(PRESET_DIR, 'Family', fname)
        with open(path, 'w') as f:
            json.dump(preset, f, indent=2)
        total += 1

    print(f"  Family Dinner: {len(FAMILY_DINNER_NAMES)} presets")
    print(f"\nTotal: {total} presets generated")

if __name__ == '__main__':
    main()
