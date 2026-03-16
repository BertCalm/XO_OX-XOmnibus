#!/usr/bin/env python3
"""
XPN Preset Recipe Book — XO_OX Designs
Generates a "recipe book" document showing how to recreate signature XO_OX sounds
using specific parameter combinations. Useful for learning, teaching, and marketing.

Each recipe documents: engine, name, description, key parameters + values,
sound design notes, and macro guidance.

Output: Markdown document with clear per-engine sections.
Optional: generate actual .xometa preset files from recipes.

Usage:
    python xpn_preset_recipe_book.py
    python xpn_preset_recipe_book.py --engine OPAL
    python xpn_preset_recipe_book.py --as-presets --output-dir recipes/
    python xpn_preset_recipe_book.py --output recipe_book.md
    python xpn_preset_recipe_book.py --engine ONSET --as-presets --output-dir recipes/onset/
"""

import argparse
import json
from datetime import date
from pathlib import Path

# ---------------------------------------------------------------------------
# Recipe Library
# ---------------------------------------------------------------------------

RECIPES = [
    # ── ONSET ────────────────────────────────────────────────────────────────
    {
        "engine": "ONSET",
        "name": "808 Sub Kick",
        "tags": ["kick", "808", "sub", "bass", "punch"],
        "mood": "Foundation",
        "description": (
            "A deep, wobbly 808-style sub kick. The pitch envelope drops fast from "
            "a high transient tone down into a sustained sub rumble. Low end sits at "
            "90–100 Hz for maximum chest impact."
        ),
        "parameters": {
            "Onset": {
                "perc_kickTone": 0.85,
                "perc_kickPunch": 0.92,
                "perc_kickDecay": 0.68,
                "perc_kickPitchEnv": 0.95,
                "perc_kickSub": 0.78,
                "perc_kickDistortion": 0.18,
                "perc_kickTune": 0.38,
                "perc_masterVol": 0.82,
            }
        },
        "dna": {"brightness": 0.2, "warmth": 0.85, "movement": 0.3,
                "density": 0.6, "space": 0.15, "aggression": 0.55},
        "macroLabels": ["MACHINE", "PUNCH", "SPACE", "MUTATE"],
        "macro_notes": [
            ("PUNCH", "up", "Increases attack transient — dial in until the beater is felt, not just heard."),
            ("MACHINE", "down", "Strips digital artifacts, pushing the character toward an analog circuit sim."),
            ("SPACE", "10–30%", "Add just a breath of room — too much blurs the sub."),
        ],
        "sound_design_notes": (
            "Start with PUNCH at 70% and sweep MACHINE to taste. The pitch envelope "
            "speed (perc_kickPitchEnv) is the single most important parameter — higher "
            "values give a shorter 'dip', lower values create the classic long 808 tail. "
            "Keep distortion below 0.25 to avoid sub mud."
        ),
        "couplingIntensity": "None",
    },
    {
        "engine": "ONSET",
        "name": "Crispy Snare",
        "tags": ["snare", "crispy", "bright", "crack", "rimshot"],
        "mood": "Prism",
        "description": (
            "A tight, bright snare with sharp transient crack and a short, controlled "
            "noise burst. Snappy rimshot character — works well in trap and lo-fi contexts."
        ),
        "parameters": {
            "Onset": {
                "perc_snareTone": 0.55,
                "perc_snareSnap": 0.88,
                "perc_snareNoise": 0.62,
                "perc_snareDecay": 0.35,
                "perc_snareTune": 0.52,
                "perc_snareDistortion": 0.28,
                "perc_masterVol": 0.78,
            }
        },
        "dna": {"brightness": 0.82, "warmth": 0.2, "movement": 0.45,
                "density": 0.5, "space": 0.3, "aggression": 0.72},
        "macroLabels": ["MACHINE", "PUNCH", "SPACE", "MUTATE"],
        "macro_notes": [
            ("PUNCH", "up", "Tightens the snap — past 80% it becomes a rim crack."),
            ("MUTATE", "slight nudge", "Introduces pitch variation for a less programmed feel."),
            ("SPACE", "down", "Keep reverb minimal — this sound lives in the dry present."),
        ],
        "sound_design_notes": (
            "The ratio between perc_snareSnap and perc_snareNoise determines character. "
            "High snap / low noise = rimshot; equal values = classic snare; "
            "low snap / high noise = brushed or liner snare. The MACHINE macro "
            "subtly colors the noise layer — position it between 40–60% for optimal texture."
        ),
        "couplingIntensity": "None",
    },
    {
        "engine": "ONSET",
        "name": "Hi-Hat Cloud",
        "tags": ["hi-hat", "open", "shimmer", "texture", "cloud"],
        "mood": "Atmosphere",
        "description": (
            "An open hi-hat with an extended shimmer tail that diffuses into a "
            "metallic cloud. Ideal for filling space between downbeats or as a "
            "sustained textural element in ambient productions."
        ),
        "parameters": {
            "Onset": {
                "perc_hatTone": 0.72,
                "perc_hatDecay": 0.88,
                "perc_hatOpen": 1.0,
                "perc_hatSizzle": 0.65,
                "perc_hatTune": 0.50,
                "perc_masterVol": 0.70,
            }
        },
        "dna": {"brightness": 0.88, "warmth": 0.15, "movement": 0.7,
                "density": 0.45, "space": 0.75, "aggression": 0.2},
        "macroLabels": ["MACHINE", "PUNCH", "SPACE", "MUTATE"],
        "macro_notes": [
            ("SPACE", "up", "Pushes the shimmer into a wide stereo field — essential for the 'cloud' effect."),
            ("MUTATE", "low", "Small amounts add organic flutter to the decay tail."),
            ("MACHINE", "mid", "50% keeps the hat in the sweet spot between analog warmth and digital sheen."),
        ],
        "sound_design_notes": (
            "The key is the interplay between perc_hatDecay and SPACE. A long decay "
            "with generous reverb creates the cloud; pulling SPACE back gives a "
            "more grounded, dry shimmer suitable for lo-fi. Layer this with a closed "
            "hat at low velocity for a humanized pattern."
        ),
        "couplingIntensity": "None",
    },

    # ── OPAL ─────────────────────────────────────────────────────────────────
    {
        "engine": "OPAL",
        "name": "Frozen Shimmer",
        "tags": ["granular", "freeze", "shimmer", "ambient", "pad"],
        "mood": "Aether",
        "description": (
            "A granular freeze effect where incoming audio crystallizes into an "
            "endless shimmer. Grain position is static, spray is tight, and pitch "
            "randomization adds a delicate sparkle without destabilizing the pitch."
        ),
        "parameters": {
            "Opal": {
                "opal_grainSize": 0.85,
                "opal_grainDensity": 0.92,
                "opal_grainPosition": 0.50,
                "opal_grainSpray": 0.08,
                "opal_pitchRandom": 0.12,
                "opal_pitchShift": 0.50,
                "opal_freeze": 1.0,
                "opal_space": 0.82,
                "opal_masterVol": 0.72,
            }
        },
        "dna": {"brightness": 0.72, "warmth": 0.45, "movement": 0.15,
                "density": 0.9, "space": 0.88, "aggression": 0.05},
        "macroLabels": ["GRAIN", "SCATTER", "DRIFT", "SPACE"],
        "macro_notes": [
            ("GRAIN", "high", "Large grains sustain the frozen texture — pull down for crunchier ice."),
            ("SCATTER", "minimal", "Keep low to maintain the frozen illusion; increase for shimmer movement."),
            ("SPACE", "high", "Wide reverb is the backbone of this sound — do not pull below 60%."),
        ],
        "sound_design_notes": (
            "Set opal_freeze to 1.0 before anything else — this is the defining parameter. "
            "Then sculpt with grain size (shimmer vs. crystalline) and spray (tight vs. diffuse). "
            "Coupling OPAL → DUB adds a subtle tape warmth that prevents the freeze from feeling sterile."
        ),
        "couplingIntensity": "Light",
    },
    {
        "engine": "OPAL",
        "name": "Granular Fog",
        "tags": ["granular", "fog", "texture", "noise", "atmospheric"],
        "mood": "Atmosphere",
        "description": (
            "Dense overlapping micro-grains with high spray create an amorphous, "
            "evolving fog of sound. Pitch tracking is loose — this is texture, not tone."
        ),
        "parameters": {
            "Opal": {
                "opal_grainSize": 0.22,
                "opal_grainDensity": 0.98,
                "opal_grainPosition": 0.35,
                "opal_grainSpray": 0.78,
                "opal_pitchRandom": 0.55,
                "opal_pitchShift": 0.50,
                "opal_freeze": 0.0,
                "opal_space": 0.65,
                "opal_masterVol": 0.65,
            }
        },
        "dna": {"brightness": 0.35, "warmth": 0.55, "movement": 0.85,
                "density": 0.95, "space": 0.65, "aggression": 0.25},
        "macroLabels": ["GRAIN", "SCATTER", "DRIFT", "SPACE"],
        "macro_notes": [
            ("SCATTER", "high", "The core of the fog — maximum spray breaks melodic identity."),
            ("DRIFT", "slow sweep", "Slowly evolving grain position keeps the fog alive and non-repetitive."),
            ("GRAIN", "down", "Small grain sizes are essential — larger grains start forming recognizable phrases."),
        ],
        "sound_design_notes": (
            "The fog lives in the collision between high density and high spray. "
            "Any reduction in density (below 0.80) will create audible gaps that break the illusion. "
            "Use automation on opal_grainPosition over 8–16 bars for organic movement."
        ),
        "couplingIntensity": "None",
    },
    {
        "engine": "OPAL",
        "name": "Glass Melt",
        "tags": ["granular", "glass", "melt", "pitched", "ethereal"],
        "mood": "Prism",
        "description": (
            "Pitched granular synthesis where medium-size grains overlap to create "
            "the sensation of glass slowly melting. Slight pitch randomization and a "
            "long, glassy reverb tail complete the effect."
        ),
        "parameters": {
            "Opal": {
                "opal_grainSize": 0.55,
                "opal_grainDensity": 0.75,
                "opal_grainPosition": 0.62,
                "opal_grainSpray": 0.30,
                "opal_pitchRandom": 0.22,
                "opal_pitchShift": 0.50,
                "opal_freeze": 0.0,
                "opal_space": 0.78,
                "opal_masterVol": 0.68,
            }
        },
        "dna": {"brightness": 0.78, "warmth": 0.3, "movement": 0.5,
                "density": 0.75, "space": 0.82, "aggression": 0.08},
        "macroLabels": ["GRAIN", "SCATTER", "DRIFT", "SPACE"],
        "macro_notes": [
            ("GRAIN", "mid", "Medium grain size is critical — too small = noise, too large = chords."),
            ("SPACE", "high", "A long, bright reverb IS the glass surface."),
            ("DRIFT", "gentle", "Slow position drift makes the melt feel continuous."),
        ],
        "sound_design_notes": (
            "The melt illusion depends on the overlap between grains — opal_grainDensity "
            "must stay above 0.65. Use opal_pitchRandom between 0.15–0.30: "
            "lower is glassy and pure, higher introduces a blurred, melting quality. "
            "Feed this into a plate reverb with a 4–6 second tail for full effect."
        ),
        "couplingIntensity": "None",
    },

    # ── ODYSSEY ───────────────────────────────────────────────────────────────
    {
        "engine": "ODYSSEY",
        "name": "Acid Bass",
        "tags": ["acid", "bass", "resonant", "TB-303", "squelch"],
        "mood": "Flux",
        "description": (
            "Classic 303-style acid bass. High resonance filter with fast envelope "
            "produces the iconic squelch. Slight portamento ties notes into a "
            "sliding sequence."
        ),
        "parameters": {
            "Odyssey": {
                "drift_filterCutoff": 0.38,
                "drift_filterRes": 0.88,
                "drift_filterEnvAmt": 0.82,
                "drift_filterDecay": 0.22,
                "drift_oscWave": 0.0,
                "drift_portamento": 0.18,
                "drift_accent": 0.72,
                "drift_masterVol": 0.80,
            }
        },
        "dna": {"brightness": 0.6, "warmth": 0.45, "movement": 0.8,
                "density": 0.5, "space": 0.2, "aggression": 0.82},
        "macroLabels": ["DRIFT", "ACID", "MOTION", "SPACE"],
        "macro_notes": [
            ("ACID", "high", "The core macro — dial between 60–100% to taste the squelch intensity."),
            ("MOTION", "up", "Increases portamento and modulation depth simultaneously."),
            ("DRIFT", "subtle", "Small amounts of pitch drift humanize the sequence."),
        ],
        "sound_design_notes": (
            "The filter envelope decay is the most expressive parameter — fast decay (< 0.25) "
            "gives classic 303 blip; slower decay creates a more vocal, talking quality. "
            "Pair with a saturator or overdrive after the synth for authentic analog bite. "
            "Note: drift_oscWave at 0.0 = sawtooth. Set to 1.0 for square wave acid."
        ),
        "couplingIntensity": "None",
    },
    {
        "engine": "ODYSSEY",
        "name": "Sub Pulse",
        "tags": ["sub", "bass", "pulse", "deep", "minimal"],
        "mood": "Foundation",
        "description": (
            "A clean, sub-dominant pulse bass. Square wave with minimal filter movement "
            "sits below 80 Hz. Works as a low-end foundation under melodic or harmonic content."
        ),
        "parameters": {
            "Odyssey": {
                "drift_filterCutoff": 0.28,
                "drift_filterRes": 0.15,
                "drift_filterEnvAmt": 0.12,
                "drift_oscWave": 1.0,
                "drift_portamento": 0.05,
                "drift_subOsc": 0.85,
                "drift_masterVol": 0.85,
            }
        },
        "dna": {"brightness": 0.12, "warmth": 0.8, "movement": 0.15,
                "density": 0.55, "space": 0.1, "aggression": 0.35},
        "macroLabels": ["DRIFT", "ACID", "MOTION", "SPACE"],
        "macro_notes": [
            ("ACID", "low", "Keep the filter closed — this sound is about weight, not movement."),
            ("SPACE", "off", "Zero reverb keeps the sub tight and punchy in a mix."),
            ("MOTION", "gentle", "A small amount softens the attack from clinical to musical."),
        ],
        "sound_design_notes": (
            "High pass the rest of your mix at 80 Hz when using this — the sub pulse "
            "owns everything below that. The square wave (drift_oscWave = 1.0) has a "
            "hollow quality in the low register that pure sine lacks. "
            "Use drift_subOsc to blend in the octave-down sub for extra weight."
        ),
        "couplingIntensity": "None",
    },
    {
        "engine": "ODYSSEY",
        "name": "Sync Sweep",
        "tags": ["sync", "lead", "sweep", "aggressive", "bright"],
        "mood": "Prism",
        "description": (
            "Hard-sync lead with dramatic filter sweep. The sync ratio modulated by "
            "an LFO creates the classic tearing, harmonic-rich sweep sound synonymous "
            "with late 90s / early 2000s electronic music."
        ),
        "parameters": {
            "Odyssey": {
                "drift_filterCutoff": 0.55,
                "drift_filterRes": 0.55,
                "drift_filterEnvAmt": 0.65,
                "drift_syncRatio": 0.72,
                "drift_lfoRate": 0.35,
                "drift_lfoAmount": 0.68,
                "drift_lfoTarget": 2,
                "drift_masterVol": 0.75,
            }
        },
        "dna": {"brightness": 0.9, "warmth": 0.2, "movement": 0.88,
                "density": 0.6, "space": 0.4, "aggression": 0.75},
        "macroLabels": ["DRIFT", "ACID", "MOTION", "SPACE"],
        "macro_notes": [
            ("MOTION", "high", "LFO speed and sync modulation both increase — the sweep becomes frantic."),
            ("ACID", "mid", "Opens the filter to reveal the full harmonic content of the sync."),
            ("DRIFT", "subtle", "Slight oscillator drift prevents the sync from sounding too rigid."),
        ],
        "sound_design_notes": (
            "drift_syncRatio is the character parameter: below 0.5 = subtle harmonic coloring; "
            "above 0.7 = aggressive tearing. The LFO targeting the sync ratio (drift_lfoTarget = 2) "
            "creates the animated sweep — set LFO to a free-running triangle wave for the "
            "most natural-feeling movement."
        ),
        "couplingIntensity": "None",
    },

    # ── OVERDUB ───────────────────────────────────────────────────────────────
    {
        "engine": "OVERDUB",
        "name": "Classic Dub Echo",
        "tags": ["dub", "echo", "delay", "reggae", "vintage"],
        "mood": "Entangled",
        "description": (
            "A warm, regenerating tape delay in the dub tradition. High feedback "
            "with gentle tape flutter creates self-oscillating echo trails that "
            "drift in and out of pitch. Essential for dub, reggae, and psychedelic production."
        ),
        "parameters": {
            "Overdub": {
                "dub_delayTime": 0.50,
                "dub_feedback": 0.82,
                "dub_tapeFlutter": 0.45,
                "dub_tapeSaturation": 0.62,
                "dub_filterCutoff": 0.55,
                "dub_filterRes": 0.38,
                "dub_reverbSend": 0.35,
                "dub_masterVol": 0.75,
            }
        },
        "dna": {"brightness": 0.35, "warmth": 0.82, "movement": 0.72,
                "density": 0.65, "space": 0.78, "aggression": 0.15},
        "macroLabels": ["SEND", "ECHO", "TAPE", "SPACE"],
        "macro_notes": [
            ("ECHO", "high", "Increases feedback toward self-oscillation — the classic dub spiral."),
            ("TAPE", "mid", "Controls the warmth and flutter character simultaneously."),
            ("SEND", "live", "Use SEND as a performance control — pulling it in a mix creates the iconic dub drop."),
        ],
        "sound_design_notes": (
            "The dub echo is a performance instrument. dub_feedback at 0.82 is near the edge "
            "of self-oscillation — increase cautiously. The tape flutter (dub_tapeFlutter) "
            "is what separates this from a digital delay; keep it above 0.35 for authenticity. "
            "Use ECHO CUT pad to kill feedback instantly — a classic dub engineer move."
        ),
        "couplingIntensity": "Light",
    },
    {
        "engine": "OVERDUB",
        "name": "Tape Warmth",
        "tags": ["tape", "warmth", "saturation", "vintage", "analog"],
        "mood": "Foundation",
        "description": (
            "Pure tape machine character with minimal delay. Saturation and gentle "
            "high-frequency rolloff add warmth and organic imperfection to any sound "
            "passing through the engine."
        ),
        "parameters": {
            "Overdub": {
                "dub_delayTime": 0.05,
                "dub_feedback": 0.05,
                "dub_tapeFlutter": 0.28,
                "dub_tapeSaturation": 0.78,
                "dub_filterCutoff": 0.72,
                "dub_filterRes": 0.12,
                "dub_reverbSend": 0.12,
                "dub_masterVol": 0.80,
            }
        },
        "dna": {"brightness": 0.35, "warmth": 0.9, "movement": 0.25,
                "density": 0.55, "space": 0.2, "aggression": 0.12},
        "macroLabels": ["SEND", "ECHO", "TAPE", "SPACE"],
        "macro_notes": [
            ("TAPE", "high", "More saturation and flutter — push toward 80% for heavy tape color."),
            ("ECHO", "low", "Keep near zero — this recipe is about warmth, not echo."),
            ("SEND", "minimal", "A faint VCA send at 10–20% can add subtle harmonic richness."),
        ],
        "sound_design_notes": (
            "This is OVERDUB used as a color processor, not an echo. The minimal delay time "
            "(0.05) provides just enough smear to soften transients without creating audible repeats. "
            "Pair as a coupling target for any engine to add vintage character — particularly "
            "effective receiving from OPAL for a granular-through-tape aesthetic."
        ),
        "couplingIntensity": "None",
    },

    # ── OBLONG ────────────────────────────────────────────────────────────────
    {
        "engine": "OBLONG",
        "name": "Punchy 808",
        "tags": ["808", "bass", "punch", "hip-hop", "modern"],
        "mood": "Foundation",
        "description": (
            "A modern hip-hop 808 with chest-filling punch and a long, clean sub tail. "
            "Distinct click attack followed by a deep pitched tone. Works across all "
            "tempos and sits naturally in dense mixes."
        ),
        "parameters": {
            "Oblong": {
                "bob_oscFreq": 0.28,
                "bob_punch": 0.88,
                "bob_subLevel": 0.82,
                "bob_pitchDecay": 0.72,
                "bob_distortion": 0.22,
                "bob_bodyTone": 0.55,
                "bob_masterVol": 0.82,
            }
        },
        "dna": {"brightness": 0.2, "warmth": 0.88, "movement": 0.25,
                "density": 0.6, "space": 0.1, "aggression": 0.6},
        "macroLabels": ["BODY", "PUNCH", "TONE", "SPACE"],
        "macro_notes": [
            ("PUNCH", "high", "The defining macro — 808 punch without PUNCH is just a sine wave."),
            ("BODY", "mid", "Controls the mid-body resonance; too high hollows out the sub."),
            ("TONE", "low", "Keep tone control minimal — the 808 character lives in the sub, not the mids."),
        ],
        "sound_design_notes": (
            "bob_pitchDecay is the 808 identity parameter — how fast the pitch drops from "
            "the high initial transient to the sustained sub note. Shorter = punchy trap 808; "
            "longer = classic boom bap. Tune bob_oscFreq to your song key "
            "(0.28 ≈ C2) for clean harmonic interaction with chord stabs."
        ),
        "couplingIntensity": "None",
    },
    {
        "engine": "OBLONG",
        "name": "Hollow Body",
        "tags": ["body", "resonant", "woody", "acoustic", "hollow"],
        "mood": "Atmosphere",
        "description": (
            "A resonant hollow-body character that mimics the tonal signature of "
            "acoustic instruments — guitars, kora, bass clarinet. The body resonance "
            "is the sound, not a wrapper around it."
        ),
        "parameters": {
            "Oblong": {
                "bob_oscFreq": 0.45,
                "bob_punch": 0.35,
                "bob_subLevel": 0.28,
                "bob_pitchDecay": 0.25,
                "bob_distortion": 0.05,
                "bob_bodyTone": 0.88,
                "bob_bodyResonance": 0.72,
                "bob_masterVol": 0.72,
            }
        },
        "dna": {"brightness": 0.45, "warmth": 0.78, "movement": 0.35,
                "density": 0.45, "space": 0.5, "aggression": 0.12},
        "macroLabels": ["BODY", "PUNCH", "TONE", "SPACE"],
        "macro_notes": [
            ("BODY", "high", "The resonance cavity is the instrument — this macro IS the sound."),
            ("TONE", "sweep", "Sweep TONE to find the air pocket — each position creates a different 'wood'."),
            ("SPACE", "30–50%", "Moderate room reverb reinforces the acoustic space illusion."),
        ],
        "sound_design_notes": (
            "Reduce punch and sub significantly — this recipe is about mid-body resonance, "
            "not low-end weight. The hollow character emerges from bob_bodyTone above 0.75 "
            "in combination with bob_bodyResonance. Velocity sensitivity is critical here: "
            "soft notes should sound muted and woody; hard notes should ring and sustain."
        ),
        "couplingIntensity": "None",
    },

    # ── OBESE ─────────────────────────────────────────────────────────────────
    {
        "engine": "OBESE",
        "name": "Warm Saturation",
        "tags": ["saturation", "warm", "tube", "harmonic", "vintage"],
        "mood": "Foundation",
        "description": (
            "Tube-style even-harmonic saturation that adds weight and presence without "
            "harshness. The classic 'glue' treatment — makes everything sit in a mix "
            "as though recorded through expensive analog hardware."
        ),
        "parameters": {
            "Obese": {
                "fat_drive": 0.42,
                "fat_tone": 0.58,
                "fat_warmth": 0.78,
                "fat_harmonics": 0.55,
                "fat_evenOdd": 0.25,
                "fat_outputGain": 0.75,
            }
        },
        "dna": {"brightness": 0.45, "warmth": 0.88, "movement": 0.2,
                "density": 0.65, "space": 0.25, "aggression": 0.25},
        "macroLabels": ["FAT", "BITE", "COLOR", "OUTPUT"],
        "macro_notes": [
            ("FAT", "mid", "The main saturation depth control — 40–60% is the sweet spot for glue."),
            ("COLOR", "warm side", "Tilts the harmonic profile toward even harmonics for tube character."),
            ("BITE", "low", "Keep minimal — this recipe is warmth, not aggression."),
        ],
        "sound_design_notes": (
            "fat_evenOdd is the character control: 0.0 = pure even harmonics (warm, musical); "
            "1.0 = odd harmonics dominant (gritty, transformer-like). The recipe targets 0.25 "
            "for a subtle tube bias. Push fat_drive above 0.65 only if you want "
            "the saturation to become audibly part of the sound design."
        ),
        "couplingIntensity": "None",
    },
    {
        "engine": "OBESE",
        "name": "Fuzz Stack",
        "tags": ["fuzz", "distortion", "aggressive", "guitar", "noise"],
        "mood": "Flux",
        "description": (
            "Maximum fuzz stacking — multiple saturation stages combine to create "
            "a crushed, harmonically dense wall of distortion. Guitar pedal meets "
            "oscillator feedback. Controlled chaos."
        ),
        "parameters": {
            "Obese": {
                "fat_drive": 0.92,
                "fat_tone": 0.65,
                "fat_warmth": 0.35,
                "fat_harmonics": 0.88,
                "fat_evenOdd": 0.82,
                "fat_stages": 3,
                "fat_outputGain": 0.55,
            }
        },
        "dna": {"brightness": 0.78, "warmth": 0.35, "movement": 0.5,
                "density": 0.9, "space": 0.3, "aggression": 0.95},
        "macroLabels": ["FAT", "BITE", "COLOR", "OUTPUT"],
        "macro_notes": [
            ("BITE", "high", "Unleashes the upper-harmonic aggression — this is the fuzz's personality."),
            ("FAT", "max", "All the way up — this recipe has no subtlety."),
            ("OUTPUT", "down", "Compensate for the gain increase — keep output gain below 0.60."),
        ],
        "sound_design_notes": (
            "Three saturation stages (fat_stages = 3) compound the distortion — each stage "
            "clips and re-clips the signal. fat_evenOdd at 0.82 pushes hard into odd harmonics "
            "for maximum edge. Use a high-pass filter post-OBESE at 200 Hz to prevent "
            "the fuzz from consuming the sub register. Best used on percussion, noise, or "
            "as a parallel distortion chain on synths."
        ),
        "couplingIntensity": "None",
    },

    # ── ODDFELIX + ODDOSCAR ───────────────────────────────────────────────────
    {
        "engine": "ODDFELIX+ODDOSCAR",
        "name": "The Coupling",
        "tags": ["coupling", "dual", "tension", "polarity", "XO"],
        "mood": "Entangled",
        "description": (
            "The signature XO_OX sound: feliX and Oscar in bidirectional tension. "
            "SNAP (feliX) generates bright, attack-forward material that feeds into "
            "MORPH (Oscar), which transforms and reflects energy back. "
            "The result is a living, breathing interaction between opposing forces."
        ),
        "parameters": {
            "OddFelix": {
                "snap_brightness": 0.78,
                "snap_attack": 0.15,
                "snap_transient": 0.82,
                "snap_couplingOut": 0.72,
                "snap_masterVol": 0.72,
            },
            "OddOscar": {
                "morph_depth": 0.65,
                "morph_rate": 0.38,
                "morph_response": 0.55,
                "morph_couplingIn": 0.68,
                "morph_feedback": 0.25,
                "morph_masterVol": 0.75,
            }
        },
        "dna": {"brightness": 0.65, "warmth": 0.55, "movement": 0.75,
                "density": 0.7, "space": 0.55, "aggression": 0.5},
        "macroLabels": ["SNAP", "MORPH", "TENSION", "SPACE"],
        "macro_notes": [
            ("TENSION", "mid", "The coupling balance — center is equal exchange; push toward SNAP or MORPH to favor one side."),
            ("SNAP", "up", "Increases feliX output and transient strength — more aggressive top-end material feeding Oscar."),
            ("MORPH", "slow sweep", "Oscar's transformation depth — sweep for evolving textural interactions."),
        ],
        "sound_design_notes": (
            "This is a coupled system, not two independent engines. The magic is in the "
            "feedback loop between snap_couplingOut and morph_couplingIn — setting both "
            "above 0.65 creates a self-reinforcing system that evolves over time. "
            "Start with TENSION at center, then slowly move it during performance. "
            "The sweet spot is when you can no longer tell which engine is driving. "
            "This is the XO_OX ethos made audible."
        ),
        "couplingIntensity": "Strong",
    },
]

# ---------------------------------------------------------------------------
# Markdown generation
# ---------------------------------------------------------------------------

def recipe_to_markdown(recipe: dict) -> str:
    lines = []
    engine_label = recipe["engine"]
    lines.append(f"### {recipe['name']}")
    lines.append(f"**Engine:** {engine_label}  ")
    lines.append(f"**Mood:** {recipe['mood']}  ")
    lines.append(f"**Tags:** {', '.join(recipe['tags'])}  ")
    lines.append(f"**Coupling:** {recipe['couplingIntensity']}")
    lines.append("")
    lines.append(recipe["description"])
    lines.append("")

    # Parameter tables — one per engine namespace
    for engine_ns, params in recipe["parameters"].items():
        lines.append(f"**Parameters — `{engine_ns}`**")
        lines.append("")
        lines.append("| Parameter | Value |")
        lines.append("|-----------|-------|")
        for k, v in params.items():
            lines.append(f"| `{k}` | `{v}` |")
        lines.append("")

    # Macro guidance
    lines.append("**Macro Guide**")
    lines.append("")
    lines.append("| Macro | Direction | What It Does |")
    lines.append("|-------|-----------|--------------|")
    for macro, direction, note in recipe["macro_notes"]:
        lines.append(f"| {macro} | {direction} | {note} |")
    lines.append("")

    # Sound design notes
    lines.append("**Sound Design Notes**")
    lines.append("")
    lines.append(recipe["sound_design_notes"])
    lines.append("")
    lines.append("---")
    lines.append("")
    return "\n".join(lines)


def build_markdown(recipes: list[dict]) -> str:
    lines = []
    lines.append("# XO_OX Preset Recipe Book")
    lines.append("")
    lines.append(
        "> A practical guide to recreating signature XO_OX sounds. "
        "Each recipe specifies exact parameter values, macro directions, "
        "and sound design principles so you can learn the engines from the inside out."
    )
    lines.append("")
    lines.append(f"*Generated: {date.today().isoformat()} — {len(recipes)} recipes*")
    lines.append("")
    lines.append("---")
    lines.append("")

    # Group by engine
    engine_order = []
    seen = set()
    for r in recipes:
        e = r["engine"]
        if e not in seen:
            engine_order.append(e)
            seen.add(e)

    lines.append("## Table of Contents")
    lines.append("")
    for engine in engine_order:
        count = sum(1 for r in recipes if r["engine"] == engine)
        anchor = engine.lower().replace("+", "").replace(" ", "-")
        lines.append(f"- [{engine}](#{anchor}) — {count} recipe{'s' if count > 1 else ''}")
    lines.append("")
    lines.append("---")
    lines.append("")

    for engine in engine_order:
        anchor = engine.lower().replace("+", "").replace(" ", "-")
        lines.append(f"## {engine}")
        lines.append("")
        engine_recipes = [r for r in recipes if r["engine"] == engine]
        for recipe in engine_recipes:
            lines.append(recipe_to_markdown(recipe))

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# .xometa generation
# ---------------------------------------------------------------------------

def recipe_to_xometa(recipe: dict) -> dict:
    engines = []
    # Derive engine display names from parameters keys
    for ns in recipe["parameters"]:
        engines.append(ns)

    return {
        "schema_version": 1,
        "name": recipe["name"],
        "mood": recipe["mood"],
        "engines": engines,
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": recipe["description"],
        "tags": recipe["tags"],
        "macroLabels": recipe["macroLabels"],
        "couplingIntensity": recipe["couplingIntensity"],
        "tempo": None,
        "dna": recipe["dna"],
        "parameters": recipe["parameters"],
        "_recipeSource": "xpn_preset_recipe_book.py",
    }


def safe_filename(name: str) -> str:
    return name.replace(" ", "_").replace("/", "-").replace("+", "").lower() + ".xometa"


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args():
    parser = argparse.ArgumentParser(
        description="XPN Preset Recipe Book — generate signature XO_OX sound recipes.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--engine",
        metavar="ENGINE",
        help="Filter to a single engine (e.g. OPAL, ONSET, OVERDUB). Case-insensitive.",
    )
    parser.add_argument(
        "--output",
        metavar="FILE",
        default="recipe_book.md",
        help="Output Markdown file path (default: recipe_book.md).",
    )
    parser.add_argument(
        "--as-presets",
        action="store_true",
        help="Also generate .xometa preset files for each recipe.",
    )
    parser.add_argument(
        "--output-dir",
        metavar="DIR",
        default="recipes",
        help="Directory for .xometa files when --as-presets is used (default: recipes/).",
    )
    return parser.parse_args()


def main():
    args = parse_args()

    recipes = RECIPES
    if args.engine:
        filter_key = args.engine.upper()
        recipes = [r for r in recipes if filter_key in r["engine"].upper()]
        if not recipes:
            print(f"No recipes found for engine: {args.engine}")
            print(f"Available engines: {sorted({r['engine'] for r in RECIPES})}")
            return

    # Write Markdown
    md = build_markdown(recipes)
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(md, encoding="utf-8")
    print(f"Recipe book written: {output_path}  ({len(recipes)} recipes)")

    # Optionally write .xometa presets
    if args.as_presets:
        out_dir = Path(args.output_dir)
        out_dir.mkdir(parents=True, exist_ok=True)
        for recipe in recipes:
            xometa = recipe_to_xometa(recipe)
            fname = safe_filename(recipe["name"])
            fpath = out_dir / fname
            fpath.write_text(json.dumps(xometa, indent=2), encoding="utf-8")
            print(f"  Preset: {fpath}")
        print(f"{len(recipes)} .xometa files written to: {out_dir}")


if __name__ == "__main__":
    main()
