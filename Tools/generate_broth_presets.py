#!/usr/bin/env python3
"""Generate 40 BROTH quad awakening preset .xometa files.

Engines: Overwash, Overworn, Overflow, Overcast
10 presets each = 40 total.
"""

import json
import os

PRESET_DIR = os.path.join(os.path.dirname(__file__), "..", "Presets", "XOlokun")

def write_preset(mood, filename, data):
    mood_dir = os.path.join(PRESET_DIR, mood)
    os.makedirs(mood_dir, exist_ok=True)
    path = os.path.join(mood_dir, filename)
    with open(path, "w") as f:
        json.dump(data, f, indent=2)
    print(f"  Wrote: {mood}/{filename}")


# ---------------------------------------------------------------------------
# ENGINE I: OVERWASH
# ---------------------------------------------------------------------------

overwash_presets = [
    {
        "filename": "Overwash_First_Drop.xometa",
        "mood": "Foundation",
        "data": {
            "name": "First Drop",
            "engines": ["Overwash"],
            "author": "XO_OX Designs",
            "description": "Single drop in perfectly still water — the reference diffusion patch. Six harmonics spreading very slowly through a viscous medium.",
            "mood": "Foundation",
            "tags": ["pad", "ambient", "diffusion", "warm"],
            "macros": {
                "CHARACTER": 0.3,
                "MOVEMENT": 0.2,
                "COUPLING": 0.5,
                "SPACE": 0.5
            },
            "dna": {
                "warmth": 0.72,
                "complexity": 0.3,
                "movement": 0.15,
                "brightness": 0.3,
                "aggression": 0.05
            },
            "params": {
                "wash_diffusionRate": 0.15,
                "wash_viscosity": 0.72,
                "wash_harmonics": 6,
                "wash_diffusionTime": 20.0,
                "wash_spreadMax": 80.0,
                "wash_brightness": 0.48,
                "wash_warmth": 0.72,
                "wash_ampAttack": 1.5,
                "wash_ampDecay": 1.0,
                "wash_ampSustain": 0.85,
                "wash_ampRelease": 6.0,
                "wash_filterCutoff": 3200.0,
                "wash_filterRes": 0.12,
                "wash_filtEnvAmount": 0.25,
                "wash_lfo1Rate": 0.04,
                "wash_lfo1Depth": 0.12,
                "wash_lfo2Rate": 0.015,
                "wash_lfo2Depth": 0.08,
                "wash_stereoWidth": 0.5,
                "wash_level": 0.8
            }
        }
    },
    {
        "filename": "Overwash_Chamomile_Hour.xometa",
        "mood": "Atmosphere",
        "data": {
            "name": "Chamomile Hour",
            "engines": ["Overwash"],
            "author": "XO_OX Designs",
            "description": "Warm tea dissolving — golden warmth, unhurried pace. Tea Amber made audible.",
            "mood": "Atmosphere",
            "tags": ["pad", "warm", "ambient", "diffusion"],
            "macros": {
                "CHARACTER": 0.45,
                "MOVEMENT": 0.35,
                "COUPLING": 0.5,
                "SPACE": 0.62
            },
            "dna": {
                "warmth": 0.82,
                "complexity": 0.45,
                "movement": 0.25,
                "brightness": 0.45,
                "aggression": 0.05
            },
            "params": {
                "wash_diffusionRate": 0.38,
                "wash_viscosity": 0.52,
                "wash_harmonics": 8,
                "wash_diffusionTime": 12.0,
                "wash_spreadMax": 140.0,
                "wash_brightness": 0.58,
                "wash_warmth": 0.82,
                "wash_ampAttack": 0.9,
                "wash_ampDecay": 1.0,
                "wash_ampSustain": 0.80,
                "wash_ampRelease": 5.0,
                "wash_filterCutoff": 4500.0,
                "wash_filterRes": 0.15,
                "wash_filtEnvAmount": 0.28,
                "wash_lfo1Rate": 0.06,
                "wash_lfo1Depth": 0.18,
                "wash_lfo2Rate": 0.022,
                "wash_lfo2Depth": 0.10,
                "wash_stereoWidth": 0.62,
                "wash_level": 0.8
            }
        }
    },
    {
        "filename": "Overwash_Slow_Tide.xometa",
        "mood": "Aether",
        "data": {
            "name": "Slow Tide",
            "engines": ["Overwash"],
            "author": "XO_OX Designs",
            "description": "Geological diffusion — tidal forces measured in minutes. One breath every 100 seconds.",
            "mood": "Aether",
            "tags": ["pad", "ambient", "drone", "evolving"],
            "macros": {
                "CHARACTER": 0.55,
                "MOVEMENT": 0.1,
                "COUPLING": 0.5,
                "SPACE": 0.7
            },
            "dna": {
                "warmth": 0.78,
                "complexity": 0.55,
                "movement": 0.08,
                "brightness": 0.2,
                "aggression": 0.02
            },
            "params": {
                "wash_diffusionRate": 0.08,
                "wash_viscosity": 0.88,
                "wash_harmonics": 10,
                "wash_diffusionTime": 30.0,
                "wash_spreadMax": 110.0,
                "wash_brightness": 0.32,
                "wash_warmth": 0.78,
                "wash_ampAttack": 3.2,
                "wash_ampDecay": 2.0,
                "wash_ampSustain": 0.90,
                "wash_ampRelease": 12.0,
                "wash_filterCutoff": 2200.0,
                "wash_filterRes": 0.10,
                "wash_filtEnvAmount": 0.15,
                "wash_lfo1Rate": 0.01,
                "wash_lfo1Depth": 0.25,
                "wash_lfo2Rate": 0.008,
                "wash_lfo2Depth": 0.15,
                "wash_stereoWidth": 0.70,
                "wash_level": 0.78
            }
        }
    },
    {
        "filename": "Overwash_Ink_Bloom.xometa",
        "mood": "Deep",
        "data": {
            "name": "Ink Bloom",
            "engines": ["Overwash"],
            "author": "XO_OX Designs",
            "description": "Chinese ink in cold water — concentrated drop, dark, patient. Five harmonics in darkness.",
            "mood": "Deep",
            "tags": ["pad", "dark", "ambient", "diffusion"],
            "macros": {
                "CHARACTER": 0.25,
                "MOVEMENT": 0.2,
                "COUPLING": 0.5,
                "SPACE": 0.44
            },
            "dna": {
                "warmth": 0.68,
                "complexity": 0.25,
                "movement": 0.12,
                "brightness": 0.12,
                "aggression": 0.05
            },
            "params": {
                "wash_diffusionRate": 0.22,
                "wash_viscosity": 0.62,
                "wash_harmonics": 5,
                "wash_diffusionTime": 18.0,
                "wash_spreadMax": 200.0,
                "wash_brightness": 0.22,
                "wash_warmth": 0.68,
                "wash_ampAttack": 2.2,
                "wash_ampDecay": 1.5,
                "wash_ampSustain": 0.85,
                "wash_ampRelease": 9.0,
                "wash_filterCutoff": 1800.0,
                "wash_filterRes": 0.12,
                "wash_filtEnvAmount": 0.20,
                "wash_lfo1Rate": 0.02,
                "wash_lfo1Depth": 0.20,
                "wash_lfo2Rate": 0.009,
                "wash_lfo2Depth": 0.10,
                "wash_stereoWidth": 0.44,
                "wash_level": 0.80
            }
        }
    },
    {
        "filename": "Overwash_Underwater_Drift.xometa",
        "mood": "Submerged",
        "data": {
            "name": "Underwater Drift",
            "engines": ["Overwash"],
            "author": "XO_OX Designs",
            "description": "Coral pigment dissolving at 30 meters — diffusion under pressure. Cinematic tension territory.",
            "mood": "Submerged",
            "tags": ["pad", "cinematic", "pressure", "dark", "ambient"],
            "macros": {
                "CHARACTER": 0.5,
                "MOVEMENT": 0.25,
                "COUPLING": 0.5,
                "SPACE": 0.58
            },
            "dna": {
                "warmth": 0.75,
                "complexity": 0.5,
                "movement": 0.18,
                "brightness": 0.25,
                "aggression": 0.12
            },
            "params": {
                "wash_diffusionRate": 0.28,
                "wash_viscosity": 0.78,
                "wash_harmonics": 9,
                "wash_diffusionTime": 25.0,
                "wash_spreadMax": 90.0,
                "wash_brightness": 0.38,
                "wash_warmth": 0.75,
                "wash_ampAttack": 2.8,
                "wash_ampDecay": 2.0,
                "wash_ampSustain": 0.88,
                "wash_ampRelease": 10.0,
                "wash_filterCutoff": 2600.0,
                "wash_filterRes": 0.18,
                "wash_filtEnvAmount": 0.30,
                "wash_filtAttack": 0.8,
                "wash_filtDecay": 3.0,
                "wash_filtSustain": 0.5,
                "wash_lfo1Rate": 0.03,
                "wash_lfo1Depth": 0.22,
                "wash_lfo2Rate": 0.012,
                "wash_lfo2Depth": 0.12,
                "wash_stereoWidth": 0.58,
                "wash_level": 0.80
            }
        }
    },
    {
        "filename": "Overwash_Saffron_Bloom.xometa",
        "mood": "Prism",
        "data": {
            "name": "Saffron Bloom",
            "engines": ["Overwash"],
            "author": "XO_OX Designs",
            "description": "Saffron releasing pigment in hot water — fast, bright, spectacular. The entire spectrum blurs into mist within 5 seconds.",
            "mood": "Prism",
            "tags": ["pad", "bright", "evolving", "diffusion"],
            "macros": {
                "CHARACTER": 0.75,
                "MOVEMENT": 0.7,
                "COUPLING": 0.5,
                "SPACE": 0.82
            },
            "dna": {
                "warmth": 0.48,
                "complexity": 0.8,
                "movement": 0.75,
                "brightness": 0.78,
                "aggression": 0.2
            },
            "params": {
                "wash_diffusionRate": 0.78,
                "wash_viscosity": 0.18,
                "wash_harmonics": 14,
                "wash_diffusionTime": 5.0,
                "wash_spreadMax": 320.0,
                "wash_brightness": 0.78,
                "wash_warmth": 0.48,
                "wash_ampAttack": 0.05,
                "wash_ampDecay": 1.2,
                "wash_ampSustain": 0.70,
                "wash_ampRelease": 3.5,
                "wash_filterCutoff": 7500.0,
                "wash_filterRes": 0.22,
                "wash_filtEnvAmount": 0.40,
                "wash_lfo1Rate": 0.28,
                "wash_lfo1Depth": 0.32,
                "wash_lfo2Rate": 0.14,
                "wash_lfo2Depth": 0.22,
                "wash_stereoWidth": 0.82,
                "wash_level": 0.78
            }
        }
    },
    {
        "filename": "Overwash_Viscosity_Study.xometa",
        "mood": "Foundation",
        "data": {
            "name": "Viscosity Study",
            "engines": ["Overwash"],
            "author": "XO_OX Designs",
            "description": "Honey-thick resistance — near-maximum viscosity, low filter. The diffusion is barely perceptible. A thick, dark, barely-breathing pad.",
            "mood": "Foundation",
            "tags": ["pad", "dark", "warm", "drone"],
            "macros": {
                "CHARACTER": 0.5,
                "MOVEMENT": 0.15,
                "COUPLING": 0.5,
                "SPACE": 0.38
            },
            "dna": {
                "warmth": 0.82,
                "complexity": 0.4,
                "movement": 0.06,
                "brightness": 0.15,
                "aggression": 0.05
            },
            "params": {
                "wash_diffusionRate": 0.5,
                "wash_viscosity": 0.94,
                "wash_harmonics": 8,
                "wash_diffusionTime": 28.0,
                "wash_spreadMax": 250.0,
                "wash_brightness": 0.42,
                "wash_warmth": 0.82,
                "wash_ampAttack": 1.2,
                "wash_ampDecay": 1.5,
                "wash_ampSustain": 0.90,
                "wash_ampRelease": 10.0,
                "wash_filterCutoff": 1600.0,
                "wash_filterRes": 0.28,
                "wash_lfo1Rate": 0.03,
                "wash_lfo1Depth": 0.14,
                "wash_lfo2Rate": 0.012,
                "wash_lfo2Depth": 0.08,
                "wash_stereoWidth": 0.38,
                "wash_level": 0.80
            }
        }
    },
    {
        "filename": "Overwash_Cinematic_Tension.xometa",
        "mood": "Kinetic",
        "data": {
            "name": "Cinematic Tension",
            "engines": ["Overwash"],
            "author": "XO_OX Designs",
            "description": "Diffusion under emotional pressure. Filter envelope creates weight arriving — opens tightly and spreads into darkness.",
            "mood": "Kinetic",
            "tags": ["pad", "cinematic", "dark", "evolving", "pressure"],
            "macros": {
                "CHARACTER": 0.6,
                "MOVEMENT": 0.4,
                "COUPLING": 0.5,
                "SPACE": 0.75
            },
            "dna": {
                "warmth": 0.55,
                "complexity": 0.65,
                "movement": 0.35,
                "brightness": 0.38,
                "aggression": 0.25
            },
            "params": {
                "wash_diffusionRate": 0.42,
                "wash_viscosity": 0.65,
                "wash_harmonics": 12,
                "wash_diffusionTime": 20.0,
                "wash_spreadMax": 180.0,
                "wash_brightness": 0.52,
                "wash_warmth": 0.55,
                "wash_ampAttack": 0.4,
                "wash_ampDecay": 3.0,
                "wash_ampSustain": 0.75,
                "wash_ampRelease": 6.0,
                "wash_filterCutoff": 3800.0,
                "wash_filterRes": 0.30,
                "wash_filtEnvAmount": 0.45,
                "wash_filtAttack": 0.25,
                "wash_filtDecay": 2.5,
                "wash_filtSustain": 0.42,
                "wash_lfo1Rate": 0.08,
                "wash_lfo1Depth": 0.25,
                "wash_lfo2Rate": 0.04,
                "wash_lfo2Depth": 0.18,
                "wash_stereoWidth": 0.75,
                "wash_level": 0.82
            }
        }
    },
    {
        "filename": "Overwash_Watercolor_Wet.xometa",
        "mood": "Atmosphere",
        "data": {
            "name": "Watercolor Wet",
            "engines": ["Overwash"],
            "author": "XO_OX Designs",
            "description": "Wet-on-wet watercolor — each note bleeds into the previous one's wake. Interference parameter raised; future-ready for BROTH coupling.",
            "mood": "Atmosphere",
            "tags": ["pad", "ambient", "diffusion", "evolving", "coupling"],
            "macros": {
                "CHARACTER": 0.5,
                "MOVEMENT": 0.45,
                "COUPLING": 0.7,
                "SPACE": 0.72
            },
            "dna": {
                "warmth": 0.6,
                "complexity": 0.55,
                "movement": 0.4,
                "brightness": 0.48,
                "aggression": 0.1
            },
            "params": {
                "wash_diffusionRate": 0.48,
                "wash_viscosity": 0.38,
                "wash_harmonics": 10,
                "wash_diffusionTime": 10.0,
                "wash_spreadMax": 175.0,
                "wash_interference": 0.70,
                "wash_brightness": 0.62,
                "wash_warmth": 0.60,
                "wash_ampAttack": 0.32,
                "wash_ampDecay": 1.0,
                "wash_ampSustain": 0.78,
                "wash_ampRelease": 4.0,
                "wash_lfo1Rate": 0.09,
                "wash_lfo1Depth": 0.24,
                "wash_lfo2Rate": 0.038,
                "wash_lfo2Depth": 0.16,
                "wash_stereoWidth": 0.72,
                "wash_level": 0.80
            }
        }
    },
    {
        "filename": "Overwash_BROTH_Opening.xometa",
        "mood": "Foundation",
        "data": {
            "name": "BROTH Opening",
            "engines": ["Overwash"],
            "author": "XO_OX Designs",
            "description": "The inaugural state — fresh water, before any reduction. Balanced, warm, patient. The ideal starting state for BROTH system play.",
            "mood": "Foundation",
            "tags": ["pad", "warm", "ambient", "diffusion"],
            "macros": {
                "CHARACTER": 0.5,
                "MOVEMENT": 0.3,
                "COUPLING": 0.5,
                "SPACE": 0.55
            },
            "dna": {
                "warmth": 0.66,
                "complexity": 0.5,
                "movement": 0.22,
                "brightness": 0.42,
                "aggression": 0.05
            },
            "params": {
                "wash_diffusionRate": 0.30,
                "wash_viscosity": 0.45,
                "wash_harmonics": 10,
                "wash_diffusionTime": 15.0,
                "wash_spreadMax": 155.0,
                "wash_brightness": 0.55,
                "wash_warmth": 0.66,
                "wash_ampAttack": 1.0,
                "wash_ampDecay": 1.2,
                "wash_ampSustain": 0.85,
                "wash_ampRelease": 6.5,
                "wash_filterCutoff": 4000.0,
                "wash_filterRes": 0.15,
                "wash_filtEnvAmount": 0.28,
                "wash_lfo1Rate": 0.05,
                "wash_lfo1Depth": 0.18,
                "wash_lfo2Rate": 0.02,
                "wash_lfo2Depth": 0.12,
                "wash_stereoWidth": 0.55,
                "wash_level": 0.80
            }
        }
    },
]

# ---------------------------------------------------------------------------
# ENGINE II: OVERWORN
# ---------------------------------------------------------------------------

overworn_presets = [
    {
        "filename": "Overworn_Fresh_Stock.xometa",
        "mood": "Foundation",
        "data": {
            "name": "Fresh Stock",
            "engines": ["Overworn"],
            "author": "XO_OX Designs",
            "description": "Session age 0.0 — the broth has just started. Full harmonics, open filter, patient reduction. The promise of the session arc.",
            "mood": "Foundation",
            "tags": ["pad", "warm", "ambient"],
            "macros": {
                "CHARACTER": 0.5,
                "MOVEMENT": 0.3,
                "COUPLING": 0.5,
                "SPACE": 0.45
            },
            "dna": {
                "warmth": 0.72,
                "complexity": 0.7,
                "movement": 0.2,
                "brightness": 0.55,
                "aggression": 0.05
            },
            "params": {
                "worn_reductionRate": 0.28,
                "worn_heat": 0.38,
                "worn_richness": 1.0,
                "worn_maillard": 0.15,
                "worn_umamiDepth": 0.65,
                "worn_sessionTarget": 30.0,
                "worn_filterCutoff": 7200.0,
                "worn_filterRes": 0.14,
                "worn_filtEnvAmount": 0.28,
                "worn_ampAttack": 0.70,
                "worn_ampDecay": 1.2,
                "worn_ampSustain": 0.90,
                "worn_ampRelease": 5.0,
                "worn_lfo1Rate": 0.06,
                "worn_lfo1Depth": 0.15,
                "worn_lfo2Rate": 0.015,
                "worn_lfo2Depth": 0.09,
                "worn_stereoWidth": 0.45,
                "worn_level": 0.80
            }
        }
    },
    {
        "filename": "Overworn_Early_Reduction.xometa",
        "mood": "Atmosphere",
        "data": {
            "name": "Early Reduction",
            "engines": ["Overworn"],
            "author": "XO_OX Designs",
            "description": "Twenty minutes in — the top notes have begun to leave. sessionAge ~0.25, bands 7-8 at 55% mass.",
            "mood": "Atmosphere",
            "tags": ["pad", "warm", "reduction", "evolving"],
            "macros": {
                "CHARACTER": 0.55,
                "MOVEMENT": 0.35,
                "COUPLING": 0.5,
                "SPACE": 0.44
            },
            "dna": {
                "warmth": 0.75,
                "complexity": 0.6,
                "movement": 0.2,
                "brightness": 0.45,
                "aggression": 0.08
            },
            "params": {
                "worn_reductionRate": 0.45,
                "worn_heat": 0.42,
                "worn_richness": 0.85,
                "worn_maillard": 0.22,
                "worn_umamiDepth": 0.65,
                "worn_sessionTarget": 30.0,
                "worn_filterCutoff": 6200.0,
                "worn_filterRes": 0.15,
                "worn_filtEnvAmount": 0.25,
                "worn_ampAttack": 0.80,
                "worn_ampSustain": 0.88,
                "worn_ampRelease": 5.5,
                "worn_lfo1Rate": 0.06,
                "worn_lfo1Depth": 0.15,
                "worn_stereoWidth": 0.44,
                "worn_level": 0.80,
                "worn_stateReset": 0.0
            }
        }
    },
    {
        "filename": "Overworn_Mid_Reduction.xometa",
        "mood": "Deep",
        "data": {
            "name": "Mid Reduction",
            "engines": ["Overworn"],
            "author": "XO_OX Designs",
            "description": "Halfway to concentration — the sauce is beginning to show its bones. Maillard saturation audible as mild warmth.",
            "mood": "Deep",
            "tags": ["pad", "dark", "warm", "reduction", "texture"],
            "macros": {
                "CHARACTER": 0.6,
                "MOVEMENT": 0.35,
                "COUPLING": 0.5,
                "SPACE": 0.4
            },
            "dna": {
                "warmth": 0.78,
                "complexity": 0.5,
                "movement": 0.18,
                "brightness": 0.35,
                "aggression": 0.12
            },
            "params": {
                "worn_reductionRate": 0.5,
                "worn_heat": 0.45,
                "worn_richness": 0.75,
                "worn_maillard": 0.35,
                "worn_umamiDepth": 0.68,
                "worn_sessionTarget": 30.0,
                "worn_filterCutoff": 5000.0,
                "worn_filterRes": 0.18,
                "worn_filtEnvAmount": 0.22,
                "worn_ampAttack": 0.90,
                "worn_ampSustain": 0.85,
                "worn_ampRelease": 6.0,
                "worn_lfo1Rate": 0.055,
                "worn_lfo1Depth": 0.15,
                "worn_stereoWidth": 0.40,
                "worn_level": 0.82
            }
        }
    },
    {
        "filename": "Overworn_Late_Reduction.xometa",
        "mood": "Crystalline",
        "data": {
            "name": "Late Reduction",
            "engines": ["Overworn"],
            "author": "XO_OX Designs",
            "description": "Nearly concentrated — only the fundamentals and their first shadows remain. Maillard saturation clearly audible, umami bed rising.",
            "mood": "Crystalline",
            "tags": ["pad", "dark", "reduction", "warm", "drone"],
            "macros": {
                "CHARACTER": 0.65,
                "MOVEMENT": 0.3,
                "COUPLING": 0.5,
                "SPACE": 0.36
            },
            "dna": {
                "warmth": 0.82,
                "complexity": 0.4,
                "movement": 0.15,
                "brightness": 0.25,
                "aggression": 0.18
            },
            "params": {
                "worn_reductionRate": 0.5,
                "worn_heat": 0.50,
                "worn_richness": 0.60,
                "worn_maillard": 0.55,
                "worn_umamiDepth": 0.72,
                "worn_sessionTarget": 30.0,
                "worn_filterCutoff": 3600.0,
                "worn_filterRes": 0.20,
                "worn_ampAttack": 1.0,
                "worn_ampSustain": 0.82,
                "worn_ampRelease": 7.0,
                "worn_lfo1Rate": 0.05,
                "worn_lfo1Depth": 0.14,
                "worn_stereoWidth": 0.36,
                "worn_level": 0.84
            }
        }
    },
    {
        "filename": "Overworn_Dark_Sauce.xometa",
        "mood": "Deep",
        "data": {
            "name": "Dark Sauce",
            "engines": ["Overworn"],
            "author": "XO_OX Designs",
            "description": "The furthest state — nearly fully reduced, dark, concentrated, irreversible. Fundamentals only, deeply saturated, narrow stereo field.",
            "mood": "Deep",
            "tags": ["pad", "dark", "reduction", "drone", "warm"],
            "macros": {
                "CHARACTER": 0.75,
                "MOVEMENT": 0.25,
                "COUPLING": 0.5,
                "SPACE": 0.32
            },
            "dna": {
                "warmth": 0.85,
                "complexity": 0.3,
                "movement": 0.1,
                "brightness": 0.15,
                "aggression": 0.25
            },
            "params": {
                "worn_reductionRate": 0.5,
                "worn_heat": 0.55,
                "worn_richness": 0.45,
                "worn_maillard": 0.75,
                "worn_umamiDepth": 0.80,
                "worn_concentrate": 0.85,
                "worn_sessionTarget": 30.0,
                "worn_filterCutoff": 2400.0,
                "worn_filterRes": 0.22,
                "worn_ampAttack": 1.2,
                "worn_ampSustain": 0.80,
                "worn_ampRelease": 8.0,
                "worn_lfo1Rate": 0.045,
                "worn_lfo1Depth": 0.13,
                "worn_stereoWidth": 0.32,
                "worn_level": 0.85
            }
        }
    },
    {
        "filename": "Overworn_Slow_Burn.xometa",
        "mood": "Kinetic",
        "data": {
            "name": "Slow Burn",
            "engines": ["Overworn"],
            "author": "XO_OX Designs",
            "description": "Controlled, intentional reduction — low heat, patient, methodical. Session target doubled to 60 minutes for hour-long sessions.",
            "mood": "Kinetic",
            "tags": ["pad", "warm", "ambient", "evolving", "reduction"],
            "macros": {
                "CHARACTER": 0.4,
                "MOVEMENT": 0.2,
                "COUPLING": 0.5,
                "SPACE": 0.48
            },
            "dna": {
                "warmth": 0.75,
                "complexity": 0.65,
                "movement": 0.12,
                "brightness": 0.55,
                "aggression": 0.05
            },
            "params": {
                "worn_reductionRate": 0.18,
                "worn_heat": 0.28,
                "worn_richness": 1.0,
                "worn_maillard": 0.18,
                "worn_umamiDepth": 0.60,
                "worn_sessionTarget": 60.0,
                "worn_filterCutoff": 7500.0,
                "worn_filterRes": 0.12,
                "worn_ampAttack": 0.65,
                "worn_ampSustain": 0.92,
                "worn_ampRelease": 5.5,
                "worn_lfo1Rate": 0.06,
                "worn_lfo1Depth": 0.14,
                "worn_stereoWidth": 0.48,
                "worn_level": 0.80
            }
        }
    },
    {
        "filename": "Overworn_Infusion_Technique.xometa",
        "mood": "Atmosphere",
        "data": {
            "name": "Infusion Technique",
            "engines": ["Overworn"],
            "author": "XO_OX Designs",
            "description": "The quiet long tone — adding character without accelerating reduction. Very low heat. Hold chords. Japanese dashi rather than French fond.",
            "mood": "Atmosphere",
            "tags": ["pad", "warm", "ambient", "evolving"],
            "macros": {
                "CHARACTER": 0.4,
                "MOVEMENT": 0.15,
                "COUPLING": 0.5,
                "SPACE": 0.5
            },
            "dna": {
                "warmth": 0.72,
                "complexity": 0.65,
                "movement": 0.1,
                "brightness": 0.52,
                "aggression": 0.03
            },
            "params": {
                "worn_reductionRate": 0.30,
                "worn_heat": 0.20,
                "worn_richness": 1.0,
                "worn_maillard": 0.12,
                "worn_umamiDepth": 0.70,
                "worn_sessionTarget": 45.0,
                "worn_filterCutoff": 6800.0,
                "worn_filterRes": 0.10,
                "worn_ampAttack": 0.50,
                "worn_ampSustain": 0.95,
                "worn_ampRelease": 6.0,
                "worn_lfo1Rate": 0.05,
                "worn_lfo1Depth": 0.10,
                "worn_stereoWidth": 0.50,
                "worn_level": 0.78
            }
        }
    },
    {
        "filename": "Overworn_Quick_Reduction.xometa",
        "mood": "Flux",
        "data": {
            "name": "Quick Reduction",
            "engines": ["Overworn"],
            "author": "XO_OX Designs",
            "description": "High heat, fast reduction — the sauce reaches concentration in minutes. Session target 10 minutes. The full arc within a single song.",
            "mood": "Flux",
            "tags": ["pad", "reduction", "evolving", "pressure"],
            "macros": {
                "CHARACTER": 0.7,
                "MOVEMENT": 0.65,
                "COUPLING": 0.5,
                "SPACE": 0.46
            },
            "dna": {
                "warmth": 0.65,
                "complexity": 0.7,
                "movement": 0.55,
                "brightness": 0.5,
                "aggression": 0.3
            },
            "params": {
                "worn_reductionRate": 0.85,
                "worn_heat": 0.75,
                "worn_richness": 1.0,
                "worn_maillard": 0.45,
                "worn_umamiDepth": 0.65,
                "worn_sessionTarget": 10.0,
                "worn_filterCutoff": 7000.0,
                "worn_filterRes": 0.15,
                "worn_ampAttack": 0.45,
                "worn_ampSustain": 0.88,
                "worn_ampRelease": 4.5,
                "worn_lfo1Rate": 0.08,
                "worn_lfo1Depth": 0.18,
                "worn_stereoWidth": 0.46,
                "worn_level": 0.80
            }
        }
    },
    {
        "filename": "Overworn_Morning_Stock.xometa",
        "mood": "Ethereal",
        "data": {
            "name": "Morning Stock",
            "engines": ["Overworn"],
            "author": "XO_OX Designs",
            "description": "A light, delicate broth — high filter cutoff, light Maillard, gentle reduction. Luminous opening bloom on each note.",
            "mood": "Ethereal",
            "tags": ["pad", "ambient", "bright", "warm", "evolving"],
            "macros": {
                "CHARACTER": 0.35,
                "MOVEMENT": 0.25,
                "COUPLING": 0.5,
                "SPACE": 0.55
            },
            "dna": {
                "warmth": 0.62,
                "complexity": 0.65,
                "movement": 0.2,
                "brightness": 0.72,
                "aggression": 0.03
            },
            "params": {
                "worn_reductionRate": 0.20,
                "worn_heat": 0.30,
                "worn_richness": 1.0,
                "worn_maillard": 0.08,
                "worn_umamiDepth": 0.40,
                "worn_sessionTarget": 45.0,
                "worn_filterCutoff": 8500.0,
                "worn_filterRes": 0.10,
                "worn_filtEnvAmount": 0.35,
                "worn_ampAttack": 0.60,
                "worn_ampSustain": 0.85,
                "worn_ampRelease": 5.0,
                "worn_lfo1Rate": 0.07,
                "worn_lfo1Depth": 0.20,
                "worn_lfo2Rate": 0.025,
                "worn_lfo2Depth": 0.12,
                "worn_stereoWidth": 0.55,
                "worn_level": 0.78
            }
        }
    },
    {
        "filename": "Overworn_The_Memory.xometa",
        "mood": "Organic",
        "data": {
            "name": "The Memory",
            "engines": ["Overworn"],
            "author": "XO_OX Designs",
            "description": "The session is the instrument — what the broth remembers. The default production workhorse. Begin, play, return — the engine remembers.",
            "mood": "Organic",
            "tags": ["pad", "warm", "ambient", "evolving", "reduction"],
            "macros": {
                "CHARACTER": 0.5,
                "MOVEMENT": 0.35,
                "COUPLING": 0.5,
                "SPACE": 0.44
            },
            "dna": {
                "warmth": 0.72,
                "complexity": 0.65,
                "movement": 0.22,
                "brightness": 0.5,
                "aggression": 0.1
            },
            "params": {
                "worn_reductionRate": 0.40,
                "worn_heat": 0.45,
                "worn_richness": 1.0,
                "worn_maillard": 0.28,
                "worn_umamiDepth": 0.70,
                "worn_sessionTarget": 30.0,
                "worn_filterCutoff": 7000.0,
                "worn_filterRes": 0.16,
                "worn_filtEnvAmount": 0.28,
                "worn_ampAttack": 0.72,
                "worn_ampDecay": 1.3,
                "worn_ampSustain": 0.88,
                "worn_ampRelease": 5.5,
                "worn_lfo1Rate": 0.06,
                "worn_lfo1Depth": 0.16,
                "worn_lfo2Rate": 0.015,
                "worn_lfo2Depth": 0.10,
                "worn_stereoWidth": 0.44,
                "worn_level": 0.80
            }
        }
    },
]

# ---------------------------------------------------------------------------
# ENGINE III: OVERFLOW
# ---------------------------------------------------------------------------

overflow_presets = [
    {
        "filename": "Overflow_Gentle_Simmer.xometa",
        "mood": "Foundation",
        "data": {
            "name": "Gentle Simmer",
            "engines": ["Overflow"],
            "author": "XO_OX Designs",
            "description": "Low heat, patient accumulation — pressure builds slowly, valve opens gradually. High threshold; the valve may never open during relaxed playing.",
            "mood": "Foundation",
            "tags": ["pad", "ambient", "pressure", "warm"],
            "macros": {
                "CHARACTER": 0.4,
                "MOVEMENT": 0.3,
                "COUPLING": 0.5,
                "SPACE": 0.58
            },
            "dna": {
                "warmth": 0.65,
                "complexity": 0.45,
                "movement": 0.25,
                "brightness": 0.45,
                "aggression": 0.08
            },
            "params": {
                "flow_threshold": 0.85,
                "flow_accumRate": 0.28,
                "flow_valveType": 0,
                "flow_vesselSize": 0.55,
                "flow_strainColor": 0.38,
                "flow_releaseTime": 0.80,
                "flow_ampAttack": 0.40,
                "flow_ampDecay": 0.8,
                "flow_ampSustain": 0.85,
                "flow_ampRelease": 3.0,
                "flow_filterCutoff": 5500.0,
                "flow_filterRes": 0.15,
                "flow_filtEnvAmount": 0.28,
                "flow_lfo1Rate": 0.09,
                "flow_lfo1Depth": 0.18,
                "flow_lfo2Rate": 0.04,
                "flow_lfo2Depth": 0.12,
                "flow_stereoWidth": 0.58,
                "flow_level": 0.80
            }
        }
    },
    {
        "filename": "Overflow_Pressure_Build.xometa",
        "mood": "Kinetic",
        "data": {
            "name": "Pressure Build",
            "engines": ["Overflow"],
            "author": "XO_OX Designs",
            "description": "The ascent — watching the gauge climb toward the red. Strain hardening prominent; triggers gradual valve within 10-15 notes.",
            "mood": "Kinetic",
            "tags": ["pad", "pressure", "evolving", "cinematic"],
            "macros": {
                "CHARACTER": 0.55,
                "MOVEMENT": 0.55,
                "COUPLING": 0.5,
                "SPACE": 0.62
            },
            "dna": {
                "warmth": 0.5,
                "complexity": 0.6,
                "movement": 0.5,
                "brightness": 0.55,
                "aggression": 0.35
            },
            "params": {
                "flow_threshold": 0.60,
                "flow_accumRate": 0.65,
                "flow_valveType": 0,
                "flow_vesselSize": 0.50,
                "flow_strainColor": 0.65,
                "flow_releaseTime": 0.60,
                "flow_ampAttack": 0.28,
                "flow_ampDecay": 0.6,
                "flow_ampSustain": 0.88,
                "flow_ampRelease": 2.5,
                "flow_filterCutoff": 6200.0,
                "flow_filterRes": 0.22,
                "flow_filtEnvAmount": 0.35,
                "flow_lfo1Rate": 0.12,
                "flow_lfo1Depth": 0.25,
                "flow_lfo2Rate": 0.055,
                "flow_lfo2Depth": 0.18,
                "flow_stereoWidth": 0.62,
                "flow_level": 0.82
            }
        }
    },
    {
        "filename": "Overflow_Explosive_Valve.xometa",
        "mood": "Flux",
        "data": {
            "name": "Explosive Valve",
            "engines": ["Overflow"],
            "author": "XO_OX Designs",
            "description": "The lid blowing — full catastrophic pressure release, silence, restart. Designed for monophonic use where the valve event is part of the musical grammar.",
            "mood": "Flux",
            "tags": ["pad", "pressure", "experimental", "evolving"],
            "macros": {
                "CHARACTER": 0.6,
                "MOVEMENT": 0.65,
                "COUPLING": 0.5,
                "SPACE": 0.72
            },
            "dna": {
                "warmth": 0.4,
                "complexity": 0.65,
                "movement": 0.7,
                "brightness": 0.65,
                "aggression": 0.55
            },
            "params": {
                "flow_threshold": 0.55,
                "flow_accumRate": 0.70,
                "flow_valveType": 1,
                "flow_vesselSize": 0.45,
                "flow_strainColor": 0.72,
                "flow_releaseTime": 0.35,
                "flow_ampAttack": 0.18,
                "flow_ampDecay": 0.5,
                "flow_ampSustain": 0.90,
                "flow_ampRelease": 2.0,
                "flow_filterCutoff": 7500.0,
                "flow_filterRes": 0.28,
                "flow_filtEnvAmount": 0.40,
                "flow_lfo1Rate": 0.15,
                "flow_lfo1Depth": 0.30,
                "flow_lfo2Rate": 0.07,
                "flow_lfo2Depth": 0.22,
                "flow_stereoWidth": 0.72,
                "flow_level": 0.80
            }
        }
    },
    {
        "filename": "Overflow_Steam_Whistle.xometa",
        "mood": "Prism",
        "data": {
            "name": "Steam Whistle",
            "engines": ["Overflow"],
            "author": "XO_OX Designs",
            "description": "The kettle's voice — pitched FM burst on pressure release at 3200Hz. Frequency rises as pressure equalizes.",
            "mood": "Prism",
            "tags": ["pad", "pressure", "experimental", "bright"],
            "macros": {
                "CHARACTER": 0.45,
                "MOVEMENT": 0.5,
                "COUPLING": 0.5,
                "SPACE": 0.65
            },
            "dna": {
                "warmth": 0.45,
                "complexity": 0.5,
                "movement": 0.45,
                "brightness": 0.7,
                "aggression": 0.3
            },
            "params": {
                "flow_threshold": 0.58,
                "flow_accumRate": 0.55,
                "flow_valveType": 2,
                "flow_vesselSize": 0.42,
                "flow_strainColor": 0.50,
                "flow_releaseTime": 0.75,
                "flow_whistlePitch": 3200.0,
                "flow_ampAttack": 0.30,
                "flow_ampDecay": 0.7,
                "flow_ampSustain": 0.85,
                "flow_ampRelease": 2.5,
                "flow_filterCutoff": 5800.0,
                "flow_filterRes": 0.20,
                "flow_filtEnvAmount": 0.32,
                "flow_lfo1Rate": 0.11,
                "flow_lfo1Depth": 0.22,
                "flow_stereoWidth": 0.65,
                "flow_level": 0.80
            }
        }
    },
    {
        "filename": "Overflow_Meditation_Vessel.xometa",
        "mood": "Aether",
        "data": {
            "name": "Meditation Vessel",
            "engines": ["Overflow"],
            "author": "XO_OX Designs",
            "description": "No valve needed — the gentle pressure pad. Threshold near maximum; OVERFLOW as a normal pad with subtle strain-hardening interest.",
            "mood": "Aether",
            "tags": ["pad", "ambient", "warm", "drone"],
            "macros": {
                "CHARACTER": 0.4,
                "MOVEMENT": 0.2,
                "COUPLING": 0.5,
                "SPACE": 0.55
            },
            "dna": {
                "warmth": 0.68,
                "complexity": 0.4,
                "movement": 0.15,
                "brightness": 0.38,
                "aggression": 0.05
            },
            "params": {
                "flow_threshold": 0.95,
                "flow_accumRate": 0.18,
                "flow_valveType": 0,
                "flow_vesselSize": 0.65,
                "flow_strainColor": 0.25,
                "flow_releaseTime": 1.5,
                "flow_ampAttack": 1.2,
                "flow_ampDecay": 1.5,
                "flow_ampSustain": 0.92,
                "flow_ampRelease": 5.0,
                "flow_filterCutoff": 4800.0,
                "flow_filterRes": 0.12,
                "flow_filtEnvAmount": 0.20,
                "flow_lfo1Rate": 0.06,
                "flow_lfo1Depth": 0.14,
                "flow_lfo2Rate": 0.025,
                "flow_lfo2Depth": 0.09,
                "flow_stereoWidth": 0.55,
                "flow_level": 0.78
            }
        }
    },
    {
        "filename": "Overflow_High_Pressure_Zone.xometa",
        "mood": "Deep",
        "data": {
            "name": "High Pressure Zone",
            "engines": ["Overflow"],
            "author": "XO_OX Designs",
            "description": "Constant near-threshold operation — always on the edge. Maximum strain color; even a single note triggers audible strain hardening.",
            "mood": "Deep",
            "tags": ["pad", "pressure", "dark", "cinematic", "experimental"],
            "macros": {
                "CHARACTER": 0.55,
                "MOVEMENT": 0.45,
                "COUPLING": 0.5,
                "SPACE": 0.7
            },
            "dna": {
                "warmth": 0.4,
                "complexity": 0.55,
                "movement": 0.4,
                "brightness": 0.35,
                "aggression": 0.55
            },
            "params": {
                "flow_threshold": 0.45,
                "flow_accumRate": 0.50,
                "flow_valveType": 0,
                "flow_vesselSize": 0.52,
                "flow_strainColor": 0.80,
                "flow_releaseTime": 0.55,
                "flow_ampAttack": 0.22,
                "flow_ampDecay": 0.6,
                "flow_ampSustain": 0.88,
                "flow_ampRelease": 2.0,
                "flow_filterCutoff": 4500.0,
                "flow_filterRes": 0.30,
                "flow_filtEnvAmount": 0.42,
                "flow_lfo1Rate": 0.14,
                "flow_lfo1Depth": 0.28,
                "flow_stereoWidth": 0.70,
                "flow_level": 0.84
            }
        }
    },
    {
        "filename": "Overflow_Interval_Dissonance.xometa",
        "mood": "Entangled",
        "data": {
            "name": "Interval Dissonance",
            "engines": ["Overflow"],
            "author": "XO_OX Designs",
            "description": "The physics of harmonic tension — dissonant intervals cost more. Play consonant intervals and pressure accumulates normally; tritones spike the gauge.",
            "mood": "Entangled",
            "tags": ["pad", "pressure", "experimental", "coupling"],
            "macros": {
                "CHARACTER": 0.5,
                "MOVEMENT": 0.5,
                "COUPLING": 0.6,
                "SPACE": 0.62
            },
            "dna": {
                "warmth": 0.5,
                "complexity": 0.55,
                "movement": 0.4,
                "brightness": 0.5,
                "aggression": 0.3
            },
            "params": {
                "flow_threshold": 0.62,
                "flow_accumRate": 0.55,
                "flow_valveType": 0,
                "flow_vesselSize": 0.50,
                "flow_strainColor": 0.58,
                "flow_releaseTime": 0.65,
                "flow_ampAttack": 0.28,
                "flow_ampDecay": 0.7,
                "flow_ampSustain": 0.88,
                "flow_ampRelease": 2.8,
                "flow_filterCutoff": 5800.0,
                "flow_filterRes": 0.22,
                "flow_filtEnvAmount": 0.35,
                "flow_lfo1Rate": 0.10,
                "flow_lfo1Depth": 0.22,
                "flow_stereoWidth": 0.62,
                "flow_level": 0.82
            }
        }
    },
    {
        "filename": "Overflow_Catastrophic_Mode.xometa",
        "mood": "Kinetic",
        "data": {
            "name": "Catastrophic Mode",
            "engines": ["Overflow"],
            "author": "XO_OX Designs",
            "description": "Over-pressure — what happens when the valve is never released. Sustained aggressive playing cycles through multiple valve events.",
            "mood": "Kinetic",
            "tags": ["pad", "pressure", "experimental", "dark", "evolving"],
            "macros": {
                "CHARACTER": 0.6,
                "MOVEMENT": 0.75,
                "COUPLING": 0.5,
                "SPACE": 0.8
            },
            "dna": {
                "warmth": 0.3,
                "complexity": 0.65,
                "movement": 0.8,
                "brightness": 0.72,
                "aggression": 0.75
            },
            "params": {
                "flow_threshold": 0.50,
                "flow_accumRate": 0.80,
                "flow_valveType": 1,
                "flow_vesselSize": 0.40,
                "flow_strainColor": 0.90,
                "flow_releaseTime": 0.25,
                "flow_ampAttack": 0.15,
                "flow_ampDecay": 0.4,
                "flow_ampSustain": 0.92,
                "flow_ampRelease": 1.8,
                "flow_filterCutoff": 8000.0,
                "flow_filterRes": 0.35,
                "flow_filtEnvAmount": 0.50,
                "flow_lfo1Rate": 0.20,
                "flow_lfo1Depth": 0.35,
                "flow_stereoWidth": 0.80,
                "flow_level": 0.82
            }
        }
    },
    {
        "filename": "Overflow_Controlled_Release.xometa",
        "mood": "Foundation",
        "data": {
            "name": "Controlled Release",
            "engines": ["Overflow"],
            "author": "XO_OX Designs",
            "description": "The engineer's pad — managing pressure for maximum musical use. Balanced accumulation; pressure mechanic perceptible but not dominant.",
            "mood": "Foundation",
            "tags": ["pad", "pressure", "warm", "ambient"],
            "macros": {
                "CHARACTER": 0.45,
                "MOVEMENT": 0.4,
                "COUPLING": 0.5,
                "SPACE": 0.6
            },
            "dna": {
                "warmth": 0.58,
                "complexity": 0.5,
                "movement": 0.3,
                "brightness": 0.45,
                "aggression": 0.2
            },
            "params": {
                "flow_threshold": 0.70,
                "flow_accumRate": 0.45,
                "flow_valveType": 0,
                "flow_vesselSize": 0.58,
                "flow_strainColor": 0.42,
                "flow_releaseTime": 1.0,
                "flow_ampAttack": 0.35,
                "flow_ampDecay": 0.8,
                "flow_ampSustain": 0.88,
                "flow_ampRelease": 2.8,
                "flow_filterCutoff": 5200.0,
                "flow_filterRes": 0.16,
                "flow_filtEnvAmount": 0.30,
                "flow_lfo1Rate": 0.09,
                "flow_lfo1Depth": 0.19,
                "flow_lfo2Rate": 0.038,
                "flow_lfo2Depth": 0.12,
                "flow_stereoWidth": 0.60,
                "flow_level": 0.80
            }
        }
    },
    {
        "filename": "Overflow_The_Consequence.xometa",
        "mood": "Organic",
        "data": {
            "name": "The Consequence",
            "engines": ["Overflow"],
            "author": "XO_OX Designs",
            "description": "The pad that remembers how you played. Play gently — a pad. Play harder — it tightens. Play a dense passage — the valve opens. Then silence. Then restart.",
            "mood": "Organic",
            "tags": ["pad", "pressure", "evolving", "ambient"],
            "macros": {
                "CHARACTER": 0.5,
                "MOVEMENT": 0.48,
                "COUPLING": 0.5,
                "SPACE": 0.62
            },
            "dna": {
                "warmth": 0.55,
                "complexity": 0.55,
                "movement": 0.4,
                "brightness": 0.48,
                "aggression": 0.25
            },
            "params": {
                "flow_threshold": 0.65,
                "flow_accumRate": 0.52,
                "flow_valveType": 0,
                "flow_vesselSize": 0.52,
                "flow_strainColor": 0.55,
                "flow_releaseTime": 0.70,
                "flow_ampAttack": 0.32,
                "flow_ampDecay": 0.7,
                "flow_ampSustain": 0.88,
                "flow_ampRelease": 2.5,
                "flow_filterCutoff": 5500.0,
                "flow_filterRes": 0.18,
                "flow_filtEnvAmount": 0.32,
                "flow_lfo1Rate": 0.10,
                "flow_lfo1Depth": 0.20,
                "flow_lfo2Rate": 0.045,
                "flow_lfo2Depth": 0.14,
                "flow_stereoWidth": 0.62,
                "flow_level": 0.80
            }
        }
    },
]

# ---------------------------------------------------------------------------
# ENGINE IV: OVERCAST
# ---------------------------------------------------------------------------

overcast_presets = [
    {
        "filename": "Overcast_First_Freeze.xometa",
        "mood": "Foundation",
        "data": {
            "name": "First Freeze",
            "engines": ["Overcast"],
            "author": "XO_OX Designs",
            "description": "Clean crystallization at medium velocity — the reference crystal. The default OVERCAST experience.",
            "mood": "Foundation",
            "tags": ["pad", "crystalline", "frozen", "ambient"],
            "macros": {
                "CHARACTER": 0.45,
                "MOVEMENT": 0.35,
                "COUPLING": 0.5,
                "SPACE": 0.6
            },
            "dna": {
                "warmth": 0.35,
                "complexity": 0.45,
                "movement": 0.1,
                "brightness": 0.55,
                "aggression": 0.05
            },
            "params": {
                "cast_freezeRate": 0.08,
                "cast_crystalSize": 0.45,
                "cast_numPeaks": 7,
                "cast_transition": 1,
                "cast_latticeSnap": 0.35,
                "cast_purity": 0.45,
                "cast_crackle": 0.50,
                "cast_shatterGap": 0.10,
                "cast_stereoWidth": 0.60,
                "cast_filterCutoff": 6500.0,
                "cast_filterRes": 0.16,
                "cast_ampAttack": 0.005,
                "cast_ampDecay": 0.50,
                "cast_ampSustain": 1.0,
                "cast_ampRelease": 2.0,
                "cast_lfo1Rate": 0.15,
                "cast_lfo1Depth": 0.15,
                "cast_lfo2Rate": 0.5,
                "cast_lfo2Depth": 0.10,
                "cast_level": 0.80
            }
        }
    },
    {
        "filename": "Overcast_Flash_Freeze.xometa",
        "mood": "Crystalline",
        "data": {
            "name": "Flash Freeze",
            "engines": ["Overcast"],
            "author": "XO_OX Designs",
            "description": "Instant mode — no transition, just crystal. High lattice snap produces very clean, tonally pure crystals. An organ stop locked in place.",
            "mood": "Crystalline",
            "tags": ["pad", "crystalline", "frozen", "bright"],
            "macros": {
                "CHARACTER": 0.55,
                "MOVEMENT": 0.1,
                "COUPLING": 0.5,
                "SPACE": 0.65
            },
            "dna": {
                "warmth": 0.25,
                "complexity": 0.5,
                "movement": 0.02,
                "brightness": 0.65,
                "aggression": 0.05
            },
            "params": {
                "cast_freezeRate": 0.02,
                "cast_crystalSize": 0.50,
                "cast_numPeaks": 8,
                "cast_transition": 0,
                "cast_latticeSnap": 0.60,
                "cast_purity": 0.55,
                "cast_crackle": 0.10,
                "cast_stereoWidth": 0.65,
                "cast_filterCutoff": 7000.0,
                "cast_filterRes": 0.14,
                "cast_ampAttack": 0.001,
                "cast_ampDecay": 0.20,
                "cast_ampSustain": 1.0,
                "cast_ampRelease": 2.5,
                "cast_lfo1Rate": 0.12,
                "cast_lfo1Depth": 0.08,
                "cast_level": 0.80
            }
        }
    },
    {
        "filename": "Overcast_Shatter_and_Reform.xometa",
        "mood": "Flux",
        "data": {
            "name": "Shatter and Reform",
            "engines": ["Overcast"],
            "author": "XO_OX Designs",
            "description": "Shatter mode — old crystal breaks, 200ms silence, new crystal forms. Play arpeggios for a sequence of shatters and reformations.",
            "mood": "Flux",
            "tags": ["pad", "crystalline", "experimental", "evolving"],
            "macros": {
                "CHARACTER": 0.4,
                "MOVEMENT": 0.65,
                "COUPLING": 0.5,
                "SPACE": 0.7
            },
            "dna": {
                "warmth": 0.3,
                "complexity": 0.5,
                "movement": 0.6,
                "brightness": 0.5,
                "aggression": 0.4
            },
            "params": {
                "cast_freezeRate": 0.12,
                "cast_crystalSize": 0.52,
                "cast_numPeaks": 8,
                "cast_transition": 2,
                "cast_latticeSnap": 0.25,
                "cast_purity": 0.40,
                "cast_crackle": 0.65,
                "cast_shatterGap": 0.20,
                "cast_stereoWidth": 0.70,
                "cast_filterCutoff": 6000.0,
                "cast_filterRes": 0.20,
                "cast_ampAttack": 0.005,
                "cast_ampDecay": 0.40,
                "cast_ampSustain": 1.0,
                "cast_ampRelease": 1.8,
                "cast_lfo1Rate": 0.18,
                "cast_lfo1Depth": 0.20,
                "cast_level": 0.80
            }
        }
    },
    {
        "filename": "Overcast_Dark_Ice.xometa",
        "mood": "Deep",
        "data": {
            "name": "Dark Ice",
            "engines": ["Overcast"],
            "author": "XO_OX Designs",
            "description": "Low velocity, few peaks, dark filter — ice in deep water. Slightly inharmonic frozen crystal. Future BROTH coupling state.",
            "mood": "Deep",
            "tags": ["pad", "dark", "crystalline", "frozen", "ambient"],
            "macros": {
                "CHARACTER": 0.3,
                "MOVEMENT": 0.2,
                "COUPLING": 0.5,
                "SPACE": 0.5
            },
            "dna": {
                "warmth": 0.3,
                "complexity": 0.3,
                "movement": 0.05,
                "brightness": 0.2,
                "aggression": 0.08
            },
            "params": {
                "cast_freezeRate": 0.10,
                "cast_crystalSize": 0.35,
                "cast_numPeaks": 5,
                "cast_transition": 1,
                "cast_latticeSnap": 0.20,
                "cast_purity": 0.35,
                "cast_crackle": 0.45,
                "cast_stereoWidth": 0.50,
                "cast_filterCutoff": 3200.0,
                "cast_filterRes": 0.18,
                "cast_ampAttack": 0.008,
                "cast_ampDecay": 0.60,
                "cast_ampSustain": 1.0,
                "cast_ampRelease": 3.0,
                "cast_lfo1Rate": 0.10,
                "cast_lfo1Depth": 0.10,
                "cast_level": 0.80
            }
        }
    },
    {
        "filename": "Overcast_Frozen_Texture.xometa",
        "mood": "Crystalline",
        "data": {
            "name": "Frozen Texture",
            "engines": ["Overcast"],
            "author": "XO_OX Designs",
            "description": "Many peaks, high purity — a complex crystal locked in place. 150ms crystallization window lets you hear the full formation process.",
            "mood": "Crystalline",
            "tags": ["pad", "crystalline", "frozen", "texture", "ambient"],
            "macros": {
                "CHARACTER": 0.65,
                "MOVEMENT": 0.4,
                "COUPLING": 0.5,
                "SPACE": 0.78
            },
            "dna": {
                "warmth": 0.3,
                "complexity": 0.75,
                "movement": 0.08,
                "brightness": 0.68,
                "aggression": 0.1
            },
            "params": {
                "cast_freezeRate": 0.15,
                "cast_crystalSize": 0.62,
                "cast_numPeaks": 14,
                "cast_transition": 1,
                "cast_latticeSnap": 0.45,
                "cast_purity": 0.62,
                "cast_crackle": 0.55,
                "cast_stereoWidth": 0.78,
                "cast_filterCutoff": 7500.0,
                "cast_filterRes": 0.20,
                "cast_ampAttack": 0.005,
                "cast_ampDecay": 0.45,
                "cast_ampSustain": 1.0,
                "cast_ampRelease": 2.5,
                "cast_lfo1Rate": 0.20,
                "cast_lfo1Depth": 0.18,
                "cast_level": 0.80
            }
        }
    },
    {
        "filename": "Overcast_Crystal_Choir.xometa",
        "mood": "Ethereal",
        "data": {
            "name": "Crystal Choir",
            "engines": ["Overcast"],
            "author": "XO_OX Designs",
            "description": "Low purity, many peaks — all voices audible in the ice. Every partial locked, none filtered. Wide stereo frozen chord.",
            "mood": "Ethereal",
            "tags": ["pad", "crystalline", "frozen", "ambient", "bright"],
            "macros": {
                "CHARACTER": 0.45,
                "MOVEMENT": 0.25,
                "COUPLING": 0.5,
                "SPACE": 0.82
            },
            "dna": {
                "warmth": 0.35,
                "complexity": 0.68,
                "movement": 0.06,
                "brightness": 0.58,
                "aggression": 0.05
            },
            "params": {
                "cast_freezeRate": 0.09,
                "cast_crystalSize": 0.40,
                "cast_numPeaks": 12,
                "cast_transition": 1,
                "cast_latticeSnap": 0.55,
                "cast_purity": 0.20,
                "cast_crackle": 0.42,
                "cast_stereoWidth": 0.82,
                "cast_filterCutoff": 5800.0,
                "cast_filterRes": 0.12,
                "cast_ampAttack": 0.005,
                "cast_ampDecay": 0.55,
                "cast_ampSustain": 1.0,
                "cast_ampRelease": 3.5,
                "cast_lfo1Rate": 0.14,
                "cast_lfo1Depth": 0.12,
                "cast_lfo2Rate": 0.35,
                "cast_lfo2Depth": 0.08,
                "cast_level": 0.78
            }
        }
    },
    {
        "filename": "Overcast_Crackling_Glass.xometa",
        "mood": "Kinetic",
        "data": {
            "name": "Crackling Glass",
            "engines": ["Overcast"],
            "author": "XO_OX Designs",
            "description": "Maximum crackle, medium freeze rate — the formation itself is the feature. Play repeated notes to hear crackling texture generation.",
            "mood": "Kinetic",
            "tags": ["pad", "crystalline", "texture", "experimental", "evolving"],
            "macros": {
                "CHARACTER": 0.5,
                "MOVEMENT": 0.75,
                "COUPLING": 0.5,
                "SPACE": 0.72
            },
            "dna": {
                "warmth": 0.3,
                "complexity": 0.55,
                "movement": 0.65,
                "brightness": 0.6,
                "aggression": 0.45
            },
            "params": {
                "cast_freezeRate": 0.18,
                "cast_crystalSize": 0.50,
                "cast_numPeaks": 9,
                "cast_transition": 1,
                "cast_latticeSnap": 0.15,
                "cast_purity": 0.38,
                "cast_crackle": 0.90,
                "cast_stereoWidth": 0.72,
                "cast_filterCutoff": 6200.0,
                "cast_filterRes": 0.25,
                "cast_ampAttack": 0.005,
                "cast_ampDecay": 0.35,
                "cast_ampSustain": 1.0,
                "cast_ampRelease": 2.0,
                "cast_lfo1Rate": 0.22,
                "cast_lfo1Depth": 0.25,
                "cast_lfo2Rate": 0.60,
                "cast_lfo2Depth": 0.15,
                "cast_level": 0.80
            }
        }
    },
    {
        "filename": "Overcast_Anti_Pad.xometa",
        "mood": "Foundation",
        "data": {
            "name": "Anti-Pad",
            "engines": ["Overcast"],
            "author": "XO_OX Designs",
            "description": "The philosophical preset — stasis as the entire aesthetic. Near-sinusoidal frozen crystal. Time, defeated. A note that does not move.",
            "mood": "Foundation",
            "tags": ["pad", "crystalline", "frozen", "drone", "ambient"],
            "macros": {
                "CHARACTER": 0.5,
                "MOVEMENT": 0.05,
                "COUPLING": 0.5,
                "SPACE": 0.55
            },
            "dna": {
                "warmth": 0.4,
                "complexity": 0.2,
                "movement": 0.0,
                "brightness": 0.5,
                "aggression": 0.02
            },
            "params": {
                "cast_freezeRate": 0.03,
                "cast_crystalSize": 0.48,
                "cast_numPeaks": 7,
                "cast_transition": 0,
                "cast_latticeSnap": 0.80,
                "cast_purity": 0.70,
                "cast_crackle": 0.05,
                "cast_stereoWidth": 0.55,
                "cast_filterCutoff": 6000.0,
                "cast_filterRes": 0.10,
                "cast_ampAttack": 0.001,
                "cast_ampDecay": 0.15,
                "cast_ampSustain": 1.0,
                "cast_ampRelease": 1.5,
                "cast_lfo1Rate": 0.10,
                "cast_lfo1Depth": 0.05,
                "cast_level": 0.80
            }
        }
    },
    {
        "filename": "Overcast_Velocity_Crystal.xometa",
        "mood": "Luminous",
        "data": {
            "name": "Velocity Crystal",
            "engines": ["Overcast"],
            "author": "XO_OX Designs",
            "description": "Velocity equals crystal complexity — demonstrates D001 through density. Soft notes produce sparse crystals; hard notes produce complex ones.",
            "mood": "Luminous",
            "tags": ["pad", "crystalline", "frozen", "bright", "evolving"],
            "macros": {
                "CHARACTER": 0.45,
                "MOVEMENT": 0.45,
                "COUPLING": 0.5,
                "SPACE": 0.68
            },
            "dna": {
                "warmth": 0.35,
                "complexity": 0.6,
                "movement": 0.15,
                "brightness": 0.65,
                "aggression": 0.1
            },
            "params": {
                "cast_freezeRate": 0.10,
                "cast_crystalSize": 0.45,
                "cast_numPeaks": 16,
                "cast_transition": 1,
                "cast_latticeSnap": 0.30,
                "cast_purity": 0.35,
                "cast_crackle": 0.55,
                "cast_stereoWidth": 0.68,
                "cast_filterCutoff": 7000.0,
                "cast_filterRes": 0.18,
                "cast_ampAttack": 0.005,
                "cast_ampDecay": 0.50,
                "cast_ampSustain": 1.0,
                "cast_ampRelease": 2.2,
                "cast_lfo1Rate": 0.16,
                "cast_lfo1Depth": 0.16,
                "cast_level": 0.80
            }
        }
    },
    {
        "filename": "Overcast_Waiting_for_Spring.xometa",
        "mood": "Organic",
        "data": {
            "name": "Waiting for Spring",
            "engines": ["Overcast"],
            "author": "XO_OX Designs",
            "description": "The crystal that knows it will eventually melt. Long release contemplative freeze. The crystal holds, then the slow melt.",
            "mood": "Organic",
            "tags": ["pad", "crystalline", "frozen", "ambient", "evolving"],
            "macros": {
                "CHARACTER": 0.42,
                "MOVEMENT": 0.3,
                "COUPLING": 0.5,
                "SPACE": 0.62
            },
            "dna": {
                "warmth": 0.42,
                "complexity": 0.35,
                "movement": 0.05,
                "brightness": 0.45,
                "aggression": 0.03
            },
            "params": {
                "cast_freezeRate": 0.07,
                "cast_crystalSize": 0.42,
                "cast_numPeaks": 6,
                "cast_transition": 1,
                "cast_latticeSnap": 0.45,
                "cast_purity": 0.50,
                "cast_crackle": 0.48,
                "cast_stereoWidth": 0.62,
                "cast_filterCutoff": 5500.0,
                "cast_filterRes": 0.14,
                "cast_ampAttack": 0.006,
                "cast_ampDecay": 0.80,
                "cast_ampSustain": 0.95,
                "cast_ampRelease": 5.0,
                "cast_lfo1Rate": 0.12,
                "cast_lfo1Depth": 0.12,
                "cast_level": 0.78
            }
        }
    },
]

# ---------------------------------------------------------------------------
# Write all presets
# ---------------------------------------------------------------------------

all_groups = [
    ("OVERWASH", overwash_presets),
    ("OVERWORN", overworn_presets),
    ("OVERFLOW", overflow_presets),
    ("OVERCAST", overcast_presets),
]

total = 0
for engine_name, presets in all_groups:
    print(f"\n{engine_name} ({len(presets)} presets):")
    for p in presets:
        write_preset(p["mood"], p["filename"], p["data"])
        total += 1

print(f"\nDone. {total} preset files written.")
