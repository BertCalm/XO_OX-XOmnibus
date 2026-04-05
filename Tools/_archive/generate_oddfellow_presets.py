#!/usr/bin/env python3
"""
generate_oddfellow_presets.py
Generate 102 factory presets for the Oddfellow Wurlitzer 200 engine.

Mood distribution:
  Foundation   25   classic Wurlitzer tones
  Luminous     20   light tremolo, clean warm attack
  Crystalline  15   tremolo off, clean precise reed
  Kinetic      15   punchy, short decay, driven
  Deep         10   dark, heavy drive, low brightness
  Flux         10   tremolo variations, LFO modulation
  Organic       7   natural reed character, warm

Constraint: 20+ presets must have aggression >= 0.7
Engine: Oddfellow (prefix oddf_)
  oddf_reed          0.0 – 1.0   (default 0.5)
  oddf_drive         0.0 – 1.0   (default 0.3)   min 1.5x architectural = no hard floor in 0-1 range
  oddf_brightness    200 – 16000 (skewed 0.3)
  oddf_tremRate      0.5 – 12.0
  oddf_tremDepth     0.0 – 1.0
  oddf_attack        0.001 – 0.5
  oddf_decay         0.05 – 5.0
  oddf_sustain       0.0 – 1.0
  oddf_release       0.01 – 5.0
  oddf_filterEnvAmt  0.0 – 1.0
  oddf_migration     0.0 – 1.0
  oddf_bendRange     1 – 24 (int)
  oddf_lfo1Rate      0.005 – 20.0
  oddf_lfo1Depth     0.0 – 1.0
  oddf_lfo1Shape     0 – 4 (int)
  oddf_lfo2Rate      0.005 – 20.0
  oddf_lfo2Depth     0.0 – 1.0
  oddf_lfo2Shape     0 – 4 (int)
  oddf_macroCharacter 0.0 – 1.0
  oddf_macroMovement  0.0 – 1.0
  oddf_macroCoupling  0.0 – 1.0
  oddf_macroSpace     0.0 – 1.0
"""

import json
import os
import sys

BASE = os.path.join(
    os.path.dirname(__file__), "..", "Presets", "XOceanus"
)

def preset(
    name, mood, description, tags, dna, params,
    coupling_intensity="None"
):
    """Build a canonical .xometa dict."""
    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": ["Oddfellow"],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": coupling_intensity,
        "tempo": None,
        "dna": dna,
        "parameters": {"Oddfellow": params},
        "coupling": {"pairs": []},
        "sequencer": None,
    }


def p(reed, drive, brightness, trem_rate, trem_depth,
      attack, decay, sustain, release, filter_env,
      migration=0.0, bend=2,
      lfo1_rate=0.5, lfo1_depth=0.0, lfo1_shape=0,
      lfo2_rate=1.0, lfo2_depth=0.0, lfo2_shape=0,
      macro_char=0.5, macro_move=0.4,
      macro_coupling=0.0, macro_space=0.3):
    """Compact parameter builder."""
    return {
        "oddf_reed": reed,
        "oddf_drive": drive,
        "oddf_brightness": brightness,
        "oddf_tremRate": trem_rate,
        "oddf_tremDepth": trem_depth,
        "oddf_attack": attack,
        "oddf_decay": decay,
        "oddf_sustain": sustain,
        "oddf_release": release,
        "oddf_filterEnvAmt": filter_env,
        "oddf_migration": migration,
        "oddf_bendRange": bend,
        "oddf_lfo1Rate": lfo1_rate,
        "oddf_lfo1Depth": lfo1_depth,
        "oddf_lfo1Shape": lfo1_shape,
        "oddf_lfo2Rate": lfo2_rate,
        "oddf_lfo2Depth": lfo2_depth,
        "oddf_lfo2Shape": lfo2_shape,
        "oddf_macroCharacter": macro_char,
        "oddf_macroMovement": macro_move,
        "oddf_macroCoupling": macro_coupling,
        "oddf_macroSpace": macro_space,
    }


def dna(brightness, warmth, movement, density, space, aggression):
    return {
        "brightness": brightness,
        "warmth": warmth,
        "movement": movement,
        "density": density,
        "space": space,
        "aggression": aggression,
    }


# ---------------------------------------------------------------------------
# PRESETS
# ---------------------------------------------------------------------------

PRESETS = []

# ===========================================================================
# FOUNDATION (25) — classic Wurlitzer tones
# ===========================================================================

PRESETS += [
    preset(
        "Copper Reed",
        "Foundation",
        "The quintessential Wurlitzer 200 — medium reed, low drive, classic tremolo. The instrument in its factory state.",
        ["wurlitzer", "classic", "foundation", "reed", "copper"],
        dna(0.42, 0.68, 0.38, 0.42, 0.38, 0.22),
        p(0.5, 0.3, 4000.0, 5.5, 0.38, 0.005, 0.7, 0.5, 0.4, 0.5,
          macro_char=0.5, macro_move=0.38),
    ),
    preset(
        "Sunset Strip",
        "Foundation",
        "Mid-era Wurlitzer with classic drive and soft tremolo. The warm side of the 70s.",
        ["wurlitzer", "70s", "warm", "sunset", "soul"],
        dna(0.44, 0.72, 0.35, 0.44, 0.4, 0.28),
        p(0.48, 0.32, 3800.0, 4.5, 0.32, 0.006, 0.75, 0.52, 0.42, 0.48,
          macro_char=0.48, macro_move=0.35),
    ),
    preset(
        "Golden Hour",
        "Foundation",
        "Soft drive, high warmth, tremolo barely breathing. The Wurlitzer at dusk, everything amber.",
        ["wurlitzer", "golden", "warm", "dusk", "classic"],
        dna(0.36, 0.78, 0.28, 0.4, 0.45, 0.18),
        p(0.44, 0.22, 3400.0, 3.5, 0.25, 0.007, 0.9, 0.55, 0.5, 0.42,
          macro_char=0.42, macro_move=0.28),
    ),
    preset(
        "Reed Riot",
        "Foundation",
        "High reed stiffness, medium-high drive. The Wurlitzer buzzing with argument. Angry but melodic.",
        ["wurlitzer", "reed", "driven", "gritty", "buzz"],
        dna(0.52, 0.52, 0.45, 0.5, 0.38, 0.72),
        p(0.82, 0.62, 5000.0, 6.0, 0.45, 0.004, 0.6, 0.48, 0.35, 0.6,
          macro_char=0.7, macro_move=0.5),
    ),
    preset(
        "Copper Haze",
        "Foundation",
        "Low reed stiffness, medium drive, slow tremolo. Vintage warmth, slightly blurred at the edges.",
        ["wurlitzer", "vintage", "warm", "haze", "copper"],
        dna(0.38, 0.75, 0.32, 0.42, 0.44, 0.2),
        p(0.28, 0.28, 3600.0, 4.0, 0.3, 0.006, 1.0, 0.52, 0.55, 0.4,
          macro_char=0.38, macro_move=0.3),
    ),
    preset(
        "Overdriven Soul",
        "Foundation",
        "Heavy drive pushing the reed into saturation. The Wurlitzer as rhythm instrument — gospel, soul, sweat.",
        ["wurlitzer", "overdrive", "soul", "driven", "gospel"],
        dna(0.55, 0.5, 0.5, 0.55, 0.38, 0.78),
        p(0.55, 0.75, 5500.0, 5.5, 0.5, 0.004, 0.65, 0.46, 0.38, 0.62,
          macro_char=0.72, macro_move=0.5),
    ),
    preset(
        "Classic Body",
        "Foundation",
        "Medium everything. A dependable starting point for any session involving a Wurlitzer.",
        ["wurlitzer", "neutral", "classic", "session", "foundation"],
        dna(0.4, 0.65, 0.4, 0.45, 0.42, 0.25),
        p(0.5, 0.3, 4000.0, 5.5, 0.4, 0.005, 0.6, 0.5, 0.4, 0.5,
          macro_char=0.5, macro_move=0.4),
    ),
    preset(
        "Smoke and Honey",
        "Foundation",
        "Warm brightness, low-medium drive, gentle tremolo. A lounge Wurlitzer with good manners and bad intentions.",
        ["wurlitzer", "lounge", "warm", "honey", "soul"],
        dna(0.35, 0.76, 0.3, 0.42, 0.5, 0.2),
        p(0.4, 0.25, 3200.0, 4.2, 0.28, 0.006, 1.1, 0.54, 0.52, 0.38,
          macro_char=0.38, macro_move=0.3),
    ),
    preset(
        "Bell Reed",
        "Foundation",
        "Higher brightness, low drive, fast attack. The Wurlitzer at its most bell-like without losing the reed character.",
        ["wurlitzer", "bell", "bright", "clean", "attack"],
        dna(0.6, 0.55, 0.38, 0.45, 0.42, 0.28),
        p(0.6, 0.22, 6500.0, 5.0, 0.3, 0.003, 0.5, 0.48, 0.38, 0.52,
          macro_char=0.55, macro_move=0.38),
    ),
    preset(
        "Rhodes Cousin",
        "Foundation",
        "Bridging the Wurlitzer toward Rhodes territory — slightly less reed bite, more sustain, warmer attack.",
        ["wurlitzer", "rhodes", "warm", "sustain", "soul"],
        dna(0.38, 0.72, 0.32, 0.44, 0.48, 0.2),
        p(0.35, 0.2, 3800.0, 4.5, 0.28, 0.008, 1.5, 0.58, 0.6, 0.38,
          macro_char=0.38, macro_move=0.28),
    ),
    preset(
        "Studio Brick",
        "Foundation",
        "Session-ready Wurlitzer. Tight attack, controlled tremolo, medium drive. Works in any mix.",
        ["wurlitzer", "studio", "session", "tight", "foundation"],
        dna(0.45, 0.6, 0.42, 0.48, 0.38, 0.3),
        p(0.5, 0.35, 4500.0, 5.5, 0.35, 0.004, 0.55, 0.5, 0.38, 0.52,
          macro_char=0.5, macro_move=0.42),
    ),
    preset(
        "Copper Fire",
        "Foundation",
        "Reed stiffness high, drive elevated, tremolo fast and present. The Wurlitzer in confrontational mode.",
        ["wurlitzer", "driven", "fire", "confrontational", "reed"],
        dna(0.55, 0.5, 0.52, 0.52, 0.38, 0.75),
        p(0.75, 0.65, 5200.0, 7.0, 0.48, 0.003, 0.6, 0.46, 0.36, 0.62,
          macro_char=0.7, macro_move=0.52),
    ),
    preset(
        "Midnight Copper",
        "Foundation",
        "Dark warmth, slow tremolo, medium drive. The Wurlitzer alone in a room at 2am.",
        ["wurlitzer", "midnight", "dark", "warm", "moody"],
        dna(0.32, 0.78, 0.28, 0.42, 0.5, 0.2),
        p(0.42, 0.28, 2800.0, 3.0, 0.22, 0.008, 1.2, 0.56, 0.6, 0.38,
          macro_char=0.38, macro_move=0.25),
    ),
    preset(
        "Soulprint",
        "Foundation",
        "The fingerprint of the Wurlitzer in R&B — medium reed, confident drive, perfect tremolo for the pocket.",
        ["wurlitzer", "soul", "r&b", "pocket", "driven"],
        dna(0.45, 0.65, 0.42, 0.48, 0.4, 0.35),
        p(0.52, 0.38, 4200.0, 5.8, 0.42, 0.005, 0.65, 0.5, 0.4, 0.52,
          macro_char=0.52, macro_move=0.42),
    ),
    preset(
        "Raw Copper",
        "Foundation",
        "Zero migration, raw reed physics, drive at the threshold of grit. Unprocessed and proud of it.",
        ["wurlitzer", "raw", "reed", "unprocessed", "grit"],
        dna(0.46, 0.62, 0.38, 0.46, 0.35, 0.38),
        p(0.62, 0.42, 4600.0, 5.0, 0.38, 0.004, 0.7, 0.5, 0.42, 0.55,
          macro_char=0.55, macro_move=0.38),
    ),
    preset(
        "Warm Pocket",
        "Foundation",
        "Slightly reduced brightness, slow tremolo, warm sustain. The Wurlitzer in a gospel choir — supportive and sure.",
        ["wurlitzer", "warm", "pocket", "gospel", "sustain"],
        dna(0.35, 0.75, 0.3, 0.44, 0.48, 0.2),
        p(0.42, 0.24, 3500.0, 3.8, 0.28, 0.007, 1.0, 0.56, 0.55, 0.38,
          macro_char=0.38, macro_move=0.3),
    ),
    preset(
        "Drive Theory",
        "Foundation",
        "Drive pushed to 0.7. Every note carries the reed's opinion about distortion.",
        ["wurlitzer", "drive", "distortion", "reed", "grit"],
        dna(0.52, 0.48, 0.48, 0.52, 0.38, 0.72),
        p(0.58, 0.7, 5400.0, 6.0, 0.45, 0.004, 0.62, 0.46, 0.38, 0.6,
          macro_char=0.68, macro_move=0.48),
    ),
    preset(
        "Tremolo Dusk",
        "Foundation",
        "Classic 5.5 Hz tremolo, warm brightness, comfortable drive. The Wurlitzer in the last hour of light.",
        ["wurlitzer", "tremolo", "dusk", "warm", "classic"],
        dna(0.4, 0.7, 0.42, 0.44, 0.42, 0.22),
        p(0.5, 0.3, 3900.0, 5.5, 0.42, 0.005, 0.7, 0.52, 0.42, 0.48,
          macro_char=0.48, macro_move=0.42),
    ),
    preset(
        "Reed Gospel",
        "Foundation",
        "Gospel-register Wurlitzer — high sustain, expressive release, medium drive with full tremolo.",
        ["wurlitzer", "gospel", "sustain", "expressive", "soul"],
        dna(0.42, 0.7, 0.4, 0.46, 0.46, 0.28),
        p(0.5, 0.32, 4100.0, 5.5, 0.4, 0.005, 0.8, 0.55, 0.5, 0.5,
          macro_char=0.5, macro_move=0.4),
    ),
    preset(
        "Copper Sustain",
        "Foundation",
        "Long decay and release, low tremolo, medium drive. Notes bloom instead of attack.",
        ["wurlitzer", "sustain", "long", "bloom", "expressive"],
        dna(0.38, 0.72, 0.28, 0.44, 0.52, 0.18),
        p(0.45, 0.25, 3700.0, 4.0, 0.22, 0.006, 2.0, 0.58, 1.2, 0.38,
          macro_char=0.4, macro_move=0.28),
    ),
    preset(
        "Snap Reed",
        "Foundation",
        "Fast attack, short decay, medium drive. The Wurlitzer's most percussive personality.",
        ["wurlitzer", "snap", "percussive", "attack", "short"],
        dna(0.5, 0.55, 0.5, 0.52, 0.38, 0.45),
        p(0.6, 0.42, 4800.0, 5.5, 0.35, 0.002, 0.35, 0.42, 0.25, 0.58,
          macro_char=0.58, macro_move=0.5),
    ),
    preset(
        "American Copper",
        "Foundation",
        "Classic American soul-era tone. Drive balanced against sustain, tremolo well-behaved.",
        ["wurlitzer", "american", "soul", "classic", "balanced"],
        dna(0.43, 0.67, 0.4, 0.46, 0.42, 0.3),
        p(0.5, 0.34, 4100.0, 5.5, 0.38, 0.005, 0.72, 0.52, 0.42, 0.5,
          macro_char=0.5, macro_move=0.4),
    ),
    preset(
        "Grit Standard",
        "Foundation",
        "The Wurlitzer refusing to be polished. Drive confident, reed stiff, tone authoritative.",
        ["wurlitzer", "grit", "authority", "reed", "driven"],
        dna(0.5, 0.55, 0.45, 0.52, 0.38, 0.55),
        p(0.65, 0.52, 4700.0, 5.5, 0.42, 0.004, 0.65, 0.48, 0.38, 0.56,
          macro_char=0.6, macro_move=0.45),
    ),
    preset(
        "Factory Copper",
        "Foundation",
        "Replicating the original Wurlitzer 200 factory settings as faithfully as possible.",
        ["wurlitzer", "factory", "stock", "200", "classic"],
        dna(0.42, 0.65, 0.4, 0.44, 0.4, 0.22),
        p(0.5, 0.3, 4000.0, 5.5, 0.4, 0.005, 0.6, 0.5, 0.4, 0.5,
          macro_char=0.5, macro_move=0.4),
    ),
    preset(
        "Burnished Tone",
        "Foundation",
        "Low migration, warm brightness, slow deep tremolo. A Wurlitzer that has been played every night for 30 years.",
        ["wurlitzer", "vintage", "worn", "warm", "burnished"],
        dna(0.36, 0.76, 0.3, 0.42, 0.48, 0.2),
        p(0.42, 0.26, 3400.0, 3.8, 0.3, 0.007, 1.1, 0.54, 0.55, 0.4,
          macro_char=0.4, macro_move=0.3),
    ),
]

# ===========================================================================
# LUMINOUS (20) — light tremolo, clean warm attack — fills 59→79 gap
# ===========================================================================

PRESETS += [
    preset(
        "Morning Light",
        "Luminous",
        "Warm brightness, very light tremolo, clean attack. A Wurlitzer at sunrise.",
        ["wurlitzer", "morning", "light", "clean", "warm"],
        dna(0.55, 0.7, 0.3, 0.42, 0.5, 0.18),
        p(0.42, 0.2, 5000.0, 3.5, 0.15, 0.004, 0.8, 0.55, 0.5, 0.38,
          macro_char=0.4, macro_move=0.28),
    ),
    preset(
        "Amber Veil",
        "Luminous",
        "Soft brightness, very gentle tremolo, expressive sustain. Diffuse light through copper.",
        ["wurlitzer", "amber", "soft", "luminous", "veil"],
        dna(0.5, 0.72, 0.25, 0.4, 0.55, 0.16),
        p(0.38, 0.18, 4600.0, 3.0, 0.12, 0.005, 1.0, 0.58, 0.6, 0.35,
          macro_char=0.38, macro_move=0.25),
    ),
    preset(
        "Shimmer Reed",
        "Luminous",
        "Light migration, open brightness, gentle tremolo. The Wurlitzer as a bell instrument.",
        ["wurlitzer", "shimmer", "bright", "luminous", "bell"],
        dna(0.62, 0.62, 0.32, 0.42, 0.52, 0.2),
        p(0.45, 0.18, 6000.0, 4.0, 0.18, 0.003, 0.7, 0.54, 0.48, 0.42,
          migration=0.08, macro_char=0.42, macro_move=0.3),
    ),
    preset(
        "Warm Canopy",
        "Luminous",
        "High warmth, low drive, tremolo at a natural sway. The Wurlitzer under green light.",
        ["wurlitzer", "warm", "canopy", "natural", "luminous"],
        dna(0.44, 0.76, 0.28, 0.42, 0.52, 0.15),
        p(0.38, 0.16, 4000.0, 3.5, 0.14, 0.006, 1.1, 0.56, 0.58, 0.35,
          macro_char=0.36, macro_move=0.25),
    ),
    preset(
        "Soap Bubble",
        "Luminous",
        "Very low drive, high brightness, light tremolo. Delicate — almost too beautiful to play.",
        ["wurlitzer", "delicate", "bright", "soap", "luminous"],
        dna(0.65, 0.62, 0.3, 0.38, 0.55, 0.14),
        p(0.3, 0.14, 7000.0, 3.0, 0.1, 0.004, 0.8, 0.52, 0.5, 0.35,
          macro_char=0.35, macro_move=0.28),
    ),
    preset(
        "Spring Copper",
        "Luminous",
        "Seasonal warmth — light drive, gentle sway. The Wurlitzer waking up after winter.",
        ["wurlitzer", "spring", "warm", "light", "luminous"],
        dna(0.48, 0.7, 0.3, 0.42, 0.5, 0.18),
        p(0.42, 0.2, 4400.0, 3.8, 0.16, 0.005, 0.9, 0.54, 0.52, 0.38,
          macro_char=0.4, macro_move=0.28),
    ),
    preset(
        "Pale Light",
        "Luminous",
        "Minimal drive, soft brightness, barely perceptible tremolo. Presence without pressure.",
        ["wurlitzer", "pale", "soft", "minimal", "luminous"],
        dna(0.42, 0.74, 0.2, 0.36, 0.58, 0.12),
        p(0.28, 0.12, 4200.0, 2.5, 0.08, 0.007, 1.2, 0.58, 0.65, 0.3,
          macro_char=0.32, macro_move=0.2),
    ),
    preset(
        "Honey Glass",
        "Luminous",
        "Warm attack, very clean decay, light tremolo. Like watching amber set in glass.",
        ["wurlitzer", "honey", "warm", "glass", "luminous"],
        dna(0.46, 0.72, 0.26, 0.4, 0.52, 0.15),
        p(0.38, 0.17, 4300.0, 3.2, 0.12, 0.005, 1.0, 0.55, 0.55, 0.35,
          macro_char=0.37, macro_move=0.25),
    ),
    preset(
        "Cathedral Reed",
        "Luminous",
        "Long decay, very low drive, slow tremolo. The Wurlitzer in a reverberant space.",
        ["wurlitzer", "cathedral", "long", "luminous", "space"],
        dna(0.44, 0.7, 0.22, 0.38, 0.68, 0.12),
        p(0.35, 0.15, 4000.0, 2.5, 0.1, 0.006, 2.5, 0.56, 1.5, 0.3,
          macro_char=0.35, macro_move=0.2, macro_space=0.65),
    ),
    preset(
        "Bright Copper",
        "Luminous",
        "Elevated brightness, low-medium drive, classic tremolo. Clean and confident.",
        ["wurlitzer", "bright", "clean", "luminous", "confident"],
        dna(0.62, 0.58, 0.38, 0.44, 0.44, 0.22),
        p(0.48, 0.22, 6500.0, 4.5, 0.25, 0.004, 0.7, 0.52, 0.45, 0.45,
          macro_char=0.45, macro_move=0.35),
    ),
    preset(
        "Noon Sun",
        "Luminous",
        "Maximum brightness available to Oddfellow while remaining warm. The reed blazes.",
        ["wurlitzer", "noon", "bright", "blazing", "luminous"],
        dna(0.72, 0.55, 0.35, 0.44, 0.42, 0.22),
        p(0.45, 0.2, 8000.0, 4.0, 0.2, 0.003, 0.6, 0.5, 0.42, 0.42,
          macro_char=0.45, macro_move=0.32),
    ),
    preset(
        "Open Copper",
        "Luminous",
        "Open migration, warm brightness, light tremolo. Notes drift outward and glow.",
        ["wurlitzer", "open", "luminous", "drift", "warm"],
        dna(0.48, 0.68, 0.32, 0.42, 0.52, 0.16),
        p(0.4, 0.18, 4500.0, 3.5, 0.15, 0.005, 0.9, 0.54, 0.5, 0.35,
          migration=0.12, macro_char=0.38, macro_move=0.28),
    ),
    preset(
        "Quiet Storm",
        "Luminous",
        "Low drive, warm brightness, gentle LFO rate modulation. Tension held lightly.",
        ["wurlitzer", "quiet", "tension", "luminous", "warm"],
        dna(0.44, 0.7, 0.3, 0.42, 0.5, 0.2),
        p(0.4, 0.2, 4000.0, 3.8, 0.15, 0.005, 1.0, 0.55, 0.52, 0.38,
          lfo1_rate=0.08, lfo1_depth=0.06,
          macro_char=0.38, macro_move=0.28),
    ),
    preset(
        "Warm Horizon",
        "Luminous",
        "Moderate brightness, extremely gentle tremolo, full body. The Wurlitzer gazing outward.",
        ["wurlitzer", "horizon", "warm", "luminous", "expansive"],
        dna(0.46, 0.72, 0.24, 0.42, 0.56, 0.15),
        p(0.42, 0.18, 4600.0, 2.8, 0.1, 0.006, 1.2, 0.56, 0.6, 0.35,
          macro_char=0.38, macro_move=0.22, macro_space=0.55),
    ),
    preset(
        "Halo Reed",
        "Luminous",
        "Very light migration, high brightness, barely-there tremolo. A sound with a halo around it.",
        ["wurlitzer", "halo", "bright", "luminous", "airy"],
        dna(0.65, 0.6, 0.26, 0.38, 0.58, 0.14),
        p(0.32, 0.15, 7500.0, 2.5, 0.08, 0.004, 0.85, 0.52, 0.55, 0.32,
          migration=0.06, macro_char=0.35, macro_move=0.22),
    ),
    preset(
        "Soft Copper Arc",
        "Luminous",
        "Warm and high enough in brightness to feel light. LFO1 creates a subtle undulation.",
        ["wurlitzer", "arc", "luminous", "undulation", "warm"],
        dna(0.5, 0.68, 0.32, 0.42, 0.5, 0.16),
        p(0.4, 0.18, 5200.0, 3.5, 0.14, 0.005, 0.9, 0.54, 0.5, 0.36,
          lfo1_rate=0.05, lfo1_depth=0.07,
          macro_char=0.4, macro_move=0.3),
    ),
    preset(
        "Glasswork Reed",
        "Luminous",
        "Inspired by minimalist composition — precise, clean, luminous. No decay, just presence.",
        ["wurlitzer", "minimalist", "glass", "luminous", "precise"],
        dna(0.6, 0.6, 0.22, 0.38, 0.54, 0.14),
        p(0.35, 0.14, 6200.0, 2.0, 0.07, 0.003, 1.5, 0.56, 0.7, 0.3,
          macro_char=0.36, macro_move=0.2),
    ),
    preset(
        "Copper Lantern",
        "Luminous",
        "Warm, amber glow. Low drive, medium brightness, tremolo like a lantern flame.",
        ["wurlitzer", "lantern", "amber", "warm", "luminous"],
        dna(0.44, 0.74, 0.28, 0.4, 0.5, 0.15),
        p(0.38, 0.17, 4200.0, 3.8, 0.18, 0.005, 1.0, 0.55, 0.55, 0.36,
          macro_char=0.38, macro_move=0.26),
    ),
    preset(
        "Gilded Touch",
        "Luminous",
        "Short decay, light drive, high warmth. Each note gilded, then gone.",
        ["wurlitzer", "gilded", "short", "warmth", "luminous"],
        dna(0.48, 0.68, 0.35, 0.44, 0.44, 0.18),
        p(0.4, 0.18, 4600.0, 4.0, 0.18, 0.004, 0.55, 0.5, 0.38, 0.38,
          macro_char=0.4, macro_move=0.32),
    ),
    preset(
        "Luminous Standard",
        "Luminous",
        "The Luminous reference preset — optimized for the 59→79 brightness gap with warmth intact.",
        ["wurlitzer", "luminous", "reference", "warm", "bright"],
        dna(0.6, 0.66, 0.3, 0.42, 0.5, 0.18),
        p(0.42, 0.19, 5600.0, 3.8, 0.16, 0.004, 0.8, 0.54, 0.5, 0.38,
          macro_char=0.42, macro_move=0.28),
    ),
]

# ===========================================================================
# CRYSTALLINE (15) — tremolo off, clean precise reed — fills 36→51 gap
# ===========================================================================

PRESETS += [
    preset(
        "Clear Reed",
        "Crystalline",
        "Tremolo completely off. Clean precise reed physics with no modulation. The Wurlitzer as a tone object.",
        ["wurlitzer", "clean", "precise", "crystalline", "no-tremolo"],
        dna(0.55, 0.6, 0.0, 0.42, 0.42, 0.18),
        p(0.5, 0.22, 5200.0, 5.5, 0.0, 0.003, 0.65, 0.5, 0.42, 0.45,
          macro_char=0.48, macro_move=0.0),
    ),
    preset(
        "Crystal Bite",
        "Crystalline",
        "Reed stiffness elevated, no tremolo, bright clarity. Each note cuts cleanly.",
        ["wurlitzer", "crystal", "bite", "precise", "bright"],
        dna(0.62, 0.55, 0.0, 0.44, 0.4, 0.25),
        p(0.65, 0.25, 6000.0, 5.5, 0.0, 0.003, 0.55, 0.48, 0.38, 0.5,
          macro_char=0.52, macro_move=0.0),
    ),
    preset(
        "Still Reed",
        "Crystalline",
        "No tremolo, low drive, warm brightness. The Wurlitzer completely at rest.",
        ["wurlitzer", "still", "warm", "crystalline", "quiet"],
        dna(0.44, 0.68, 0.0, 0.4, 0.46, 0.15),
        p(0.42, 0.18, 4200.0, 5.5, 0.0, 0.005, 0.9, 0.54, 0.5, 0.38,
          macro_char=0.4, macro_move=0.0),
    ),
    preset(
        "Diamond Reed",
        "Crystalline",
        "Maximum clarity — high brightness, precise attack, no tremolo. Hard and beautiful.",
        ["wurlitzer", "diamond", "hard", "precise", "bright"],
        dna(0.7, 0.5, 0.0, 0.42, 0.4, 0.28),
        p(0.55, 0.22, 7500.0, 5.5, 0.0, 0.002, 0.5, 0.48, 0.38, 0.48,
          macro_char=0.52, macro_move=0.0),
    ),
    preset(
        "Copper Prism",
        "Crystalline",
        "Warm brightness, no tremolo, medium reed. The instrument decomposed into pure tone.",
        ["wurlitzer", "prism", "warm", "crystalline", "tone"],
        dna(0.48, 0.65, 0.0, 0.42, 0.44, 0.18),
        p(0.5, 0.2, 4600.0, 5.5, 0.0, 0.004, 0.7, 0.52, 0.44, 0.4,
          macro_char=0.46, macro_move=0.0),
    ),
    preset(
        "Reed Jewel",
        "Crystalline",
        "Low drive, no tremolo, high reed stiffness. Precious and exact.",
        ["wurlitzer", "jewel", "precise", "crystalline", "reed"],
        dna(0.56, 0.6, 0.0, 0.44, 0.42, 0.2),
        p(0.68, 0.18, 5600.0, 5.5, 0.0, 0.003, 0.6, 0.5, 0.4, 0.45,
          macro_char=0.5, macro_move=0.0),
    ),
    preset(
        "Quartz Reed",
        "Crystalline",
        "Tremolo off. Precise oscillation, medium warmth, quartz-like sustain.",
        ["wurlitzer", "quartz", "precise", "crystalline", "sustain"],
        dna(0.52, 0.64, 0.0, 0.42, 0.48, 0.16),
        p(0.5, 0.2, 5000.0, 5.5, 0.0, 0.004, 1.0, 0.55, 0.55, 0.4,
          macro_char=0.46, macro_move=0.0),
    ),
    preset(
        "Frost Reed",
        "Crystalline",
        "High brightness, zero tremolo, cool attack. The Wurlitzer in winter, precise and cold.",
        ["wurlitzer", "frost", "cold", "bright", "crystalline"],
        dna(0.66, 0.52, 0.0, 0.4, 0.42, 0.18),
        p(0.42, 0.16, 7000.0, 5.5, 0.0, 0.003, 0.62, 0.5, 0.42, 0.4,
          macro_char=0.44, macro_move=0.0),
    ),
    preset(
        "Facet",
        "Crystalline",
        "No tremolo, varied reed stiffness, moderate drive. Each note has a facet.",
        ["wurlitzer", "facet", "varied", "crystalline", "driven"],
        dna(0.5, 0.58, 0.0, 0.44, 0.4, 0.28),
        p(0.58, 0.3, 5200.0, 5.5, 0.0, 0.003, 0.6, 0.48, 0.38, 0.5,
          macro_char=0.52, macro_move=0.0),
    ),
    preset(
        "Copper Lens",
        "Crystalline",
        "Wide brightness, no tremolo, low migration. Notes focused without dispersion.",
        ["wurlitzer", "lens", "focused", "crystalline", "copper"],
        dna(0.54, 0.62, 0.0, 0.42, 0.42, 0.2),
        p(0.48, 0.2, 5400.0, 5.5, 0.0, 0.004, 0.65, 0.52, 0.42, 0.42,
          macro_char=0.48, macro_move=0.0),
    ),
    preset(
        "Bone Reed",
        "Crystalline",
        "High reed stiffness, elevated drive, no tremolo. The Wurlitzer with architectural intent.",
        ["wurlitzer", "bone", "hard", "crystalline", "driven"],
        dna(0.56, 0.52, 0.0, 0.46, 0.38, 0.38),
        p(0.75, 0.42, 5500.0, 5.5, 0.0, 0.003, 0.55, 0.46, 0.35, 0.55,
          macro_char=0.6, macro_move=0.0),
    ),
    preset(
        "Clean Machine",
        "Crystalline",
        "Session-ready, no tremolo, medium everything. The cleanest possible Wurlitzer.",
        ["wurlitzer", "clean", "session", "crystalline", "precise"],
        dna(0.5, 0.6, 0.0, 0.42, 0.4, 0.18),
        p(0.5, 0.18, 5000.0, 5.5, 0.0, 0.004, 0.65, 0.5, 0.4, 0.4,
          macro_char=0.46, macro_move=0.0),
    ),
    preset(
        "Spire Reed",
        "Crystalline",
        "No tremolo, high brightness, long sustain. The Wurlitzer reaching upward.",
        ["wurlitzer", "spire", "bright", "crystalline", "tall"],
        dna(0.65, 0.58, 0.0, 0.4, 0.52, 0.16),
        p(0.44, 0.16, 8000.0, 5.5, 0.0, 0.003, 1.5, 0.56, 0.8, 0.35,
          macro_char=0.42, macro_move=0.0, macro_space=0.5),
    ),
    preset(
        "Ice Reed",
        "Crystalline",
        "Zero drive, zero tremolo, high brightness, short decay. Crystalline in the truest sense.",
        ["wurlitzer", "ice", "cold", "crystalline", "short"],
        dna(0.68, 0.5, 0.0, 0.38, 0.4, 0.14),
        p(0.38, 0.12, 8000.0, 5.5, 0.0, 0.002, 0.4, 0.44, 0.32, 0.35,
          macro_char=0.4, macro_move=0.0),
    ),
    preset(
        "Crystalline Standard",
        "Crystalline",
        "Reference preset for the Crystalline mood — optimized for the 36→51 brightness gap.",
        ["wurlitzer", "reference", "crystalline", "standard", "bright"],
        dna(0.56, 0.58, 0.0, 0.42, 0.42, 0.18),
        p(0.5, 0.2, 5800.0, 5.5, 0.0, 0.003, 0.6, 0.5, 0.4, 0.42,
          macro_char=0.48, macro_move=0.0),
    ),
]

# ===========================================================================
# KINETIC (15) — punchy, short decay, driven (many high aggression)
# ===========================================================================

PRESETS += [
    preset(
        "Punch Clock",
        "Kinetic",
        "Maximum punch — near-zero attack, short decay, high drive. The Wurlitzer as a percussion instrument.",
        ["wurlitzer", "punch", "percussive", "kinetic", "short"],
        dna(0.55, 0.5, 0.52, 0.56, 0.38, 0.75),
        p(0.65, 0.72, 5200.0, 6.0, 0.48, 0.002, 0.35, 0.42, 0.25, 0.62,
          macro_char=0.72, macro_move=0.5),
    ),
    preset(
        "Riot Reed",
        "Kinetic",
        "High drive, fast tremolo, short decay. No patience, maximum energy.",
        ["wurlitzer", "riot", "fast", "kinetic", "driven"],
        dna(0.58, 0.48, 0.88, 0.55, 0.38, 0.82),
        p(0.72, 0.78, 5800.0, 9.0, 0.62, 0.002, 0.35, 0.42, 0.25, 0.65,
          macro_char=0.8, macro_move=0.88),
    ),
    preset(
        "Freight Train",
        "Kinetic",
        "Relentless drive, medium tremolo, unstoppable forward motion.",
        ["wurlitzer", "freight", "powerful", "kinetic", "drive"],
        dna(0.52, 0.48, 0.65, 0.58, 0.38, 0.8),
        p(0.68, 0.8, 5500.0, 7.5, 0.55, 0.003, 0.4, 0.44, 0.3, 0.65,
          macro_char=0.78, macro_move=0.65),
    ),
    preset(
        "Snap Attack",
        "Kinetic",
        "Instantaneous attack, elevated drive, punchy decay. The single-note weapon.",
        ["wurlitzer", "snap", "attack", "kinetic", "punch"],
        dna(0.52, 0.52, 0.5, 0.54, 0.38, 0.72),
        p(0.62, 0.68, 5000.0, 5.5, 0.42, 0.001, 0.3, 0.4, 0.22, 0.6,
          macro_char=0.7, macro_move=0.5),
    ),
    preset(
        "Electric Surge",
        "Kinetic",
        "Drive near maximum, fast tremolo. The Wurlitzer electrical system under stress.",
        ["wurlitzer", "electric", "surge", "kinetic", "driven"],
        dna(0.58, 0.45, 0.78, 0.56, 0.38, 0.85),
        p(0.65, 0.85, 6000.0, 8.5, 0.65, 0.003, 0.4, 0.44, 0.28, 0.68,
          macro_char=0.82, macro_move=0.78),
    ),
    preset(
        "Drive Spike",
        "Kinetic",
        "Short attack, very high drive, minimal release. One spike of copper energy per note.",
        ["wurlitzer", "drive", "spike", "kinetic", "sharp"],
        dna(0.55, 0.48, 0.55, 0.56, 0.35, 0.78),
        p(0.68, 0.78, 5400.0, 6.5, 0.5, 0.002, 0.32, 0.4, 0.2, 0.65,
          macro_char=0.75, macro_move=0.55),
    ),
    preset(
        "Hammer Reed",
        "Kinetic",
        "Short decay, high reed stiffness, high drive. The Wurlitzer hitting back.",
        ["wurlitzer", "hammer", "hard", "kinetic", "reed"],
        dna(0.56, 0.48, 0.55, 0.58, 0.36, 0.78),
        p(0.8, 0.72, 5600.0, 6.5, 0.5, 0.002, 0.36, 0.42, 0.24, 0.65,
          macro_char=0.75, macro_move=0.55),
    ),
    preset(
        "Aggressive Copper",
        "Kinetic",
        "Full aggression — high reed, high drive, fast tremolo, short decay. The instrument at war.",
        ["wurlitzer", "aggressive", "kinetic", "war", "driven"],
        dna(0.6, 0.45, 0.62, 0.6, 0.35, 0.88),
        p(0.78, 0.85, 5800.0, 8.0, 0.6, 0.002, 0.35, 0.42, 0.22, 0.68,
          macro_char=0.85, macro_move=0.65),
    ),
    preset(
        "Fast Lane",
        "Kinetic",
        "Fast tremolo, punchy attack, driven. The Wurlitzer at highway speed.",
        ["wurlitzer", "fast", "highway", "kinetic", "driven"],
        dna(0.56, 0.48, 0.75, 0.54, 0.38, 0.78),
        p(0.6, 0.75, 5500.0, 9.5, 0.58, 0.003, 0.38, 0.42, 0.25, 0.62,
          macro_char=0.78, macro_move=0.75),
    ),
    preset(
        "Reed Thrust",
        "Kinetic",
        "Maximum reed stiffness, high drive, very short decay. A sound like a piston.",
        ["wurlitzer", "thrust", "reed", "kinetic", "mechanical"],
        dna(0.55, 0.48, 0.58, 0.58, 0.36, 0.8),
        p(0.9, 0.74, 5600.0, 7.0, 0.5, 0.002, 0.3, 0.4, 0.2, 0.65,
          macro_char=0.78, macro_move=0.58),
    ),
    preset(
        "Copper Blade",
        "Kinetic",
        "Sharp, precise, high drive, no hesitation. The Wurlitzer as a cutting instrument.",
        ["wurlitzer", "blade", "sharp", "kinetic", "precise"],
        dna(0.58, 0.48, 0.52, 0.56, 0.36, 0.75),
        p(0.65, 0.7, 5600.0, 6.0, 0.45, 0.002, 0.33, 0.4, 0.22, 0.62,
          macro_char=0.72, macro_move=0.52),
    ),
    preset(
        "Impact Zone",
        "Kinetic",
        "High reed stiffness, near-max drive, very short decay. Built for impact.",
        ["wurlitzer", "impact", "hard", "kinetic", "percussive"],
        dna(0.56, 0.46, 0.6, 0.58, 0.36, 0.82),
        p(0.82, 0.82, 5800.0, 7.5, 0.55, 0.002, 0.3, 0.4, 0.2, 0.67,
          macro_char=0.8, macro_move=0.62),
    ),
    preset(
        "Copper Fist",
        "Kinetic",
        "The most aggressive single-note tone in the Oddfellow library. Drive and reed maxed.",
        ["wurlitzer", "fist", "maximum", "kinetic", "aggressive"],
        dna(0.58, 0.44, 0.65, 0.62, 0.34, 0.9),
        p(0.92, 0.9, 6000.0, 8.0, 0.6, 0.001, 0.28, 0.38, 0.18, 0.7,
          macro_char=0.88, macro_move=0.65),
    ),
    preset(
        "Short Circuit",
        "Kinetic",
        "Very short decay, fast tremolo, elevated drive. The Wurlitzer short-circuiting beautifully.",
        ["wurlitzer", "short", "circuit", "kinetic", "fast"],
        dna(0.55, 0.48, 0.7, 0.55, 0.36, 0.75),
        p(0.62, 0.68, 5400.0, 8.5, 0.55, 0.003, 0.32, 0.42, 0.22, 0.62,
          macro_char=0.72, macro_move=0.7),
    ),
    preset(
        "Copper Kick",
        "Kinetic",
        "The Wurlitzer's kick drum. Near-zero attack, short decay, high drive. Purely kinetic.",
        ["wurlitzer", "kick", "percussive", "kinetic", "driven"],
        dna(0.52, 0.5, 0.62, 0.58, 0.36, 0.78),
        p(0.65, 0.72, 5000.0, 7.0, 0.5, 0.002, 0.3, 0.4, 0.2, 0.62,
          macro_char=0.75, macro_move=0.62),
    ),
]

# ===========================================================================
# DEEP (10) — dark, heavy drive, low brightness
# ===========================================================================

PRESETS += [
    preset(
        "Abyssal Reed",
        "Deep",
        "Lowest brightness, maximum drive, long decay. The Wurlitzer below the thermocline.",
        ["wurlitzer", "dark", "deep", "low", "abyssal"],
        dna(0.15, 0.75, 0.35, 0.52, 0.55, 0.75),
        p(0.6, 0.85, 500.0, 5.5, 0.35, 0.01, 2.0, 0.6, 1.0, 0.65,
          macro_char=0.72, macro_move=0.35),
    ),
    preset(
        "Tar Pit",
        "Deep",
        "Maximum drive, dark brightness, slow heavy tremolo. Dense, stuck, relentless.",
        ["wurlitzer", "tar", "dark", "heavy", "deep"],
        dna(0.18, 0.72, 0.42, 0.6, 0.5, 0.85),
        p(0.65, 0.9, 600.0, 4.5, 0.42, 0.01, 1.5, 0.58, 0.8, 0.68,
          macro_char=0.82, macro_move=0.42),
    ),
    preset(
        "Deep Copper",
        "Deep",
        "Very dark tone, high reed stiffness, heavy drive. Copper at depth.",
        ["wurlitzer", "deep", "dark", "copper", "heavy"],
        dna(0.2, 0.72, 0.38, 0.55, 0.52, 0.78),
        p(0.72, 0.82, 700.0, 5.0, 0.38, 0.008, 1.8, 0.58, 0.9, 0.65,
          macro_char=0.78, macro_move=0.38),
    ),
    preset(
        "Basement Rhodes",
        "Deep",
        "A Wurlitzer stored for 20 years and just turned on. Dark, heavy, uncompromising.",
        ["wurlitzer", "basement", "vintage", "dark", "heavy"],
        dna(0.22, 0.7, 0.35, 0.54, 0.5, 0.7),
        p(0.58, 0.78, 800.0, 4.5, 0.35, 0.01, 1.6, 0.6, 0.85, 0.62,
          macro_char=0.72, macro_move=0.35),
    ),
    preset(
        "Iron Reed",
        "Deep",
        "Reed stiffness maximum, extreme drive, dark brightness. Weight without mercy.",
        ["wurlitzer", "iron", "heavy", "dark", "deep"],
        dna(0.2, 0.68, 0.4, 0.6, 0.48, 0.82),
        p(0.92, 0.88, 600.0, 5.5, 0.4, 0.008, 1.5, 0.56, 0.75, 0.68,
          macro_char=0.82, macro_move=0.4),
    ),
    preset(
        "Dark Current",
        "Deep",
        "Low brightness, high drive, slow deep tremolo, long release. Dark water flowing.",
        ["wurlitzer", "dark", "current", "deep", "heavy"],
        dna(0.18, 0.74, 0.3, 0.55, 0.55, 0.75),
        p(0.6, 0.82, 650.0, 3.5, 0.4, 0.01, 2.2, 0.6, 1.2, 0.65,
          macro_char=0.72, macro_move=0.3, macro_space=0.5),
    ),
    preset(
        "Copper Trench",
        "Deep",
        "Maximum bass register feel — dark tone, high reed, heavy drive, sub-sustain.",
        ["wurlitzer", "trench", "bass", "deep", "dark"],
        dna(0.15, 0.76, 0.38, 0.6, 0.52, 0.8),
        p(0.78, 0.85, 450.0, 5.0, 0.38, 0.01, 2.0, 0.62, 1.0, 0.68,
          macro_char=0.8, macro_move=0.38),
    ),
    preset(
        "Thermal Vent",
        "Deep",
        "Dense, hot, dark. Drive and brightness set for maximum psychoacoustic weight.",
        ["wurlitzer", "thermal", "dense", "dark", "deep"],
        dna(0.2, 0.7, 0.42, 0.62, 0.5, 0.78),
        p(0.68, 0.84, 700.0, 6.0, 0.42, 0.008, 1.5, 0.58, 0.8, 0.66,
          macro_char=0.78, macro_move=0.42),
    ),
    preset(
        "Pressure Copper",
        "Deep",
        "Maximum drive with darkness dialled in. The reed under incredible pressure.",
        ["wurlitzer", "pressure", "dark", "deep", "dense"],
        dna(0.18, 0.72, 0.4, 0.62, 0.5, 0.82),
        p(0.7, 0.9, 550.0, 5.5, 0.4, 0.008, 1.8, 0.58, 0.9, 0.68,
          macro_char=0.82, macro_move=0.4),
    ),
    preset(
        "Deep Standard",
        "Deep",
        "Reference for the Deep mood — dark, driven, fully Oddfellow character.",
        ["wurlitzer", "deep", "reference", "standard", "dark"],
        dna(0.2, 0.72, 0.38, 0.56, 0.52, 0.75),
        p(0.65, 0.82, 650.0, 5.0, 0.38, 0.01, 1.8, 0.6, 0.9, 0.65,
          macro_char=0.75, macro_move=0.38),
    ),
]

# ===========================================================================
# FLUX (10) — tremolo variations, LFO modulation
# ===========================================================================

PRESETS += [
    preset(
        "Warble Engine",
        "Flux",
        "Slow deep tremolo with LFO1 rate modulation. The tremolo is breathing, not static.",
        ["wurlitzer", "warble", "flux", "lfo", "tremolo"],
        dna(0.44, 0.65, 0.72, 0.48, 0.44, 0.3),
        p(0.5, 0.32, 4200.0, 4.5, 0.62, 0.005, 0.8, 0.52, 0.45, 0.48,
          lfo1_rate=0.12, lfo1_depth=0.15, lfo1_shape=1,
          macro_char=0.48, macro_move=0.72),
    ),
    preset(
        "Flutter Field",
        "Flux",
        "Very fast tremolo depth modulation via LFO2. The tremolo rate mutates.",
        ["wurlitzer", "flutter", "flux", "fast", "lfo"],
        dna(0.46, 0.6, 0.88, 0.5, 0.42, 0.35),
        p(0.48, 0.38, 4500.0, 10.0, 0.72, 0.004, 0.7, 0.5, 0.4, 0.5,
          lfo2_rate=4.5, lfo2_depth=0.28, lfo2_shape=2,
          macro_char=0.52, macro_move=0.88),
    ),
    preset(
        "Phase Trem",
        "Flux",
        "LFO1 at triangle wave modulating tremolo depth. Phase offset between rate and depth.",
        ["wurlitzer", "phase", "flux", "triangle", "lfo"],
        dna(0.44, 0.62, 0.75, 0.48, 0.44, 0.28),
        p(0.5, 0.3, 4300.0, 6.0, 0.55, 0.005, 0.75, 0.52, 0.42, 0.46,
          lfo1_rate=0.3, lfo1_depth=0.18, lfo1_shape=2,
          macro_char=0.48, macro_move=0.75),
    ),
    preset(
        "Sine Flux",
        "Flux",
        "Both LFOs active on sine waves at different rates. The tremolo breathes against itself.",
        ["wurlitzer", "sine", "flux", "dual-lfo", "tremolo"],
        dna(0.44, 0.62, 0.8, 0.5, 0.44, 0.32),
        p(0.5, 0.34, 4400.0, 6.5, 0.6, 0.005, 0.75, 0.52, 0.42, 0.48,
          lfo1_rate=0.25, lfo1_depth=0.14, lfo1_shape=0,
          lfo2_rate=1.8, lfo2_depth=0.18, lfo2_shape=0,
          macro_char=0.5, macro_move=0.8),
    ),
    preset(
        "Creep Rate",
        "Flux",
        "Extremely slow LFO1 modulates filter env. The tone slowly changes over time.",
        ["wurlitzer", "slow", "flux", "creep", "lfo"],
        dna(0.44, 0.65, 0.45, 0.46, 0.46, 0.25),
        p(0.48, 0.28, 4200.0, 5.0, 0.38, 0.005, 0.85, 0.54, 0.48, 0.45,
          lfo1_rate=0.008, lfo1_depth=0.25, lfo1_shape=0,
          macro_char=0.46, macro_move=0.42),
    ),
    preset(
        "Tremolo Surge",
        "Flux",
        "Tremolo depth modulated by LFO2 at square wave. Depth surges in and out.",
        ["wurlitzer", "surge", "flux", "square", "tremolo"],
        dna(0.46, 0.6, 0.82, 0.52, 0.42, 0.38),
        p(0.52, 0.38, 4600.0, 7.5, 0.6, 0.004, 0.7, 0.5, 0.38, 0.5,
          lfo2_rate=0.6, lfo2_depth=0.32, lfo2_shape=3,
          macro_char=0.52, macro_move=0.82),
    ),
    preset(
        "Drift Copper",
        "Flux",
        "Migration enabled with LFO1 pitch drift. Notes wander slowly.",
        ["wurlitzer", "drift", "flux", "migration", "lfo"],
        dna(0.44, 0.65, 0.42, 0.46, 0.46, 0.25),
        p(0.5, 0.28, 4100.0, 5.0, 0.3, 0.005, 0.85, 0.54, 0.5, 0.42,
          migration=0.25, lfo1_rate=0.05, lfo1_depth=0.12, lfo1_shape=1,
          macro_char=0.45, macro_move=0.4),
    ),
    preset(
        "Chaos Trem",
        "Flux",
        "LFO1 and LFO2 at inharmonic rates. The tremolo is no longer regular.",
        ["wurlitzer", "chaos", "flux", "inharmonic", "lfo"],
        dna(0.46, 0.6, 0.85, 0.5, 0.42, 0.35),
        p(0.52, 0.36, 4400.0, 6.8, 0.58, 0.004, 0.72, 0.5, 0.4, 0.48,
          lfo1_rate=0.17, lfo1_depth=0.2, lfo1_shape=4,
          lfo2_rate=2.7, lfo2_depth=0.22, lfo2_shape=4,
          macro_char=0.52, macro_move=0.85),
    ),
    preset(
        "Slow Warble",
        "Flux",
        "Very slow tremolo, deep depth, LFO1 at ultra-low rate. The Wurlitzer in slow motion.",
        ["wurlitzer", "slow", "warble", "flux", "deep"],
        dna(0.42, 0.68, 0.55, 0.46, 0.46, 0.22),
        p(0.48, 0.28, 4000.0, 1.2, 0.72, 0.006, 1.0, 0.54, 0.55, 0.42,
          lfo1_rate=0.01, lfo1_depth=0.1, lfo1_shape=0,
          macro_char=0.44, macro_move=0.55),
    ),
    preset(
        "Wobble Copper",
        "Flux",
        "Medium tremolo, LFO1 at sawtooth modulating migration. Notes wobble on their axis.",
        ["wurlitzer", "wobble", "flux", "sawtooth", "migration"],
        dna(0.44, 0.64, 0.65, 0.48, 0.44, 0.28),
        p(0.5, 0.3, 4300.0, 5.5, 0.5, 0.005, 0.8, 0.52, 0.45, 0.46,
          migration=0.15, lfo1_rate=0.45, lfo1_depth=0.18, lfo1_shape=3,
          macro_char=0.48, macro_move=0.65),
    ),
]

# ===========================================================================
# ORGANIC (7) — natural reed character, warm
# ===========================================================================

PRESETS += [
    preset(
        "Breath Reed",
        "Organic",
        "The Wurlitzer breathing. Very low migration, natural reed, warm decay.",
        ["wurlitzer", "breath", "natural", "organic", "warm"],
        dna(0.35, 0.78, 0.28, 0.38, 0.52, 0.15),
        p(0.32, 0.2, 3200.0, 4.5, 0.22, 0.008, 1.4, 0.56, 0.65, 0.35,
          lfo1_rate=0.03, lfo1_depth=0.04, lfo1_shape=0,
          macro_char=0.35, macro_move=0.25),
    ),
    preset(
        "Reed Song",
        "Organic",
        "Natural reed physics foregrounded. Warm body, minimal drive, expressive.",
        ["wurlitzer", "natural", "song", "organic", "expressive"],
        dna(0.36, 0.76, 0.25, 0.38, 0.54, 0.14),
        p(0.28, 0.18, 3400.0, 4.0, 0.2, 0.007, 1.2, 0.56, 0.62, 0.32,
          lfo1_rate=0.02, lfo1_depth=0.04,
          macro_char=0.34, macro_move=0.22),
    ),
    preset(
        "Humid Copper",
        "Organic",
        "Slightly increased migration, warm brightness. The Wurlitzer in a humid room.",
        ["wurlitzer", "humid", "warm", "organic", "room"],
        dna(0.34, 0.78, 0.28, 0.4, 0.52, 0.14),
        p(0.35, 0.18, 3200.0, 4.5, 0.2, 0.008, 1.3, 0.56, 0.65, 0.32,
          migration=0.1, lfo1_rate=0.025, lfo1_depth=0.04,
          macro_char=0.34, macro_move=0.25),
    ),
    preset(
        "Living Reed",
        "Organic",
        "Reed stiffness gently varying, warm sustain, low drive. The reed as a living thing.",
        ["wurlitzer", "living", "organic", "reed", "warm"],
        dna(0.36, 0.76, 0.22, 0.38, 0.54, 0.12),
        p(0.3, 0.15, 3300.0, 3.5, 0.15, 0.007, 1.5, 0.58, 0.7, 0.3,
          lfo1_rate=0.018, lfo1_depth=0.05, lfo1_shape=0,
          macro_char=0.32, macro_move=0.2),
    ),
    preset(
        "Morning Reed",
        "Organic",
        "Warm, gentle, barely awake. Low drive, low tremolo, long release. The first sound of the day.",
        ["wurlitzer", "morning", "gentle", "organic", "warm"],
        dna(0.34, 0.8, 0.2, 0.36, 0.56, 0.1),
        p(0.25, 0.14, 3000.0, 3.0, 0.14, 0.009, 1.6, 0.58, 0.8, 0.28,
          lfo1_rate=0.015, lfo1_depth=0.04,
          macro_char=0.3, macro_move=0.18),
    ),
    preset(
        "Copper Root",
        "Organic",
        "The Wurlitzer rooted in its acoustic origins — natural reed, organic warmth, nothing added.",
        ["wurlitzer", "root", "natural", "organic", "acoustic"],
        dna(0.36, 0.76, 0.24, 0.38, 0.5, 0.14),
        p(0.3, 0.17, 3400.0, 4.0, 0.18, 0.007, 1.2, 0.55, 0.6, 0.32,
          macro_char=0.34, macro_move=0.22),
    ),
    preset(
        "Warm Organism",
        "Organic",
        "Every parameter tuned for organic naturalness. The Wurlitzer as a biological instrument.",
        ["wurlitzer", "warm", "organism", "organic", "natural"],
        dna(0.35, 0.78, 0.22, 0.38, 0.52, 0.12),
        p(0.28, 0.16, 3200.0, 3.8, 0.16, 0.008, 1.4, 0.56, 0.68, 0.3,
          lfo1_rate=0.02, lfo1_depth=0.04, lfo1_shape=0,
          lfo2_rate=0.008, lfo2_depth=0.02,
          macro_char=0.32, macro_move=0.2),
    ),
]


# ---------------------------------------------------------------------------
# WRITE PRESETS
# ---------------------------------------------------------------------------

def sanitize_filename(name):
    """Convert preset name to safe filename."""
    safe = name.replace(" ", "_").replace("/", "-").replace("\\", "-")
    safe = "".join(c for c in safe if c.isalnum() or c in "_-.")
    return safe + ".xometa"


def write_presets(presets):
    written = 0
    skipped = 0
    mood_counts = {}

    for pst in presets:
        mood = pst["mood"]
        mood_dir = os.path.join(BASE, mood, "Oddfellow")
        os.makedirs(mood_dir, exist_ok=True)

        filename = sanitize_filename(pst["name"])
        filepath = os.path.join(mood_dir, filename)

        if os.path.exists(filepath):
            # Read existing and compare names to avoid duplicates
            with open(filepath, "r") as f:
                existing = json.load(f)
            if existing.get("name") == pst["name"]:
                skipped += 1
                continue

        with open(filepath, "w") as f:
            json.dump(pst, f, indent=2)
            f.write("\n")

        written += 1
        mood_counts[mood] = mood_counts.get(mood, 0) + 1

    return written, skipped, mood_counts


def validate_presets(presets):
    """Check aggression constraint and count."""
    high_aggression = sum(
        1 for p in presets if p["dna"]["aggression"] >= 0.7
    )
    mood_dist = {}
    for p in presets:
        mood_dist[p["mood"]] = mood_dist.get(p["mood"], 0) + 1

    print(f"\n--- Validation ---")
    print(f"Total presets: {len(presets)}")
    print(f"High aggression (>= 0.7): {high_aggression}  (need 20+)")
    print(f"\nMood distribution:")
    for mood, count in sorted(mood_dist.items()):
        target = {
            "Foundation": 25, "Luminous": 20, "Crystalline": 15,
            "Kinetic": 15, "Deep": 10, "Flux": 10, "Organic": 7
        }.get(mood, "?")
        status = "OK" if count == target else f"MISMATCH (target {target})"
        print(f"  {mood:15s} {count:3d}  {status}")

    # Check for duplicate names
    names = [p["name"] for p in presets]
    dupes = [n for n in set(names) if names.count(n) > 1]
    if dupes:
        print(f"\nDUPLICATE NAMES: {dupes}")
    else:
        print(f"\nNo duplicate names.")

    if high_aggression < 20:
        print(f"WARNING: Only {high_aggression} presets with aggression >= 0.7 (need 20+)")
        return False
    if len(presets) != 102:
        print(f"WARNING: Expected 102 presets, got {len(presets)}")
        return False
    return True


if __name__ == "__main__":
    ok = validate_presets(PRESETS)
    if not ok:
        print("\nValidation FAILED — aborting write.")
        sys.exit(1)

    written, skipped, mood_counts = write_presets(PRESETS)

    print(f"\n--- Write Results ---")
    print(f"Written: {written}")
    print(f"Skipped (already exist): {skipped}")
    print(f"By mood:")
    for mood, count in sorted(mood_counts.items()):
        print(f"  {mood:15s} {count}")

    base_abs = os.path.abspath(BASE)
    print(f"\nPresets written to: {base_abs}/{{mood}}/Oddfellow/")
    print("Done.")
