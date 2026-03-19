#!/usr/bin/env python3
"""
XOmnibus Preset Generator
  OVERLAP : 106 remaining (to reach 150 total)
  OUTWIT  : 50 Block 1

Usage:
  python3 generate_presets_overlap_outwit.py          # write files
  python3 generate_presets_overlap_outwit.py --dry-run # preview only
"""

import json, sys
from pathlib import Path

BASE   = Path.home() / "Documents/GitHub/XO_OX-XOmnibus/Presets/XOmnibus"
AUTHOR = "XO_OX Designs"
DATE   = "2026-03-18"
DRY    = "--dry-run" in sys.argv
_written = _skipped = 0


def write_preset(data: dict, engine_folder: str) -> None:
    global _written, _skipped
    p = BASE / data["mood"] / engine_folder / f"{data['name']}.xometa"
    p.parent.mkdir(parents=True, exist_ok=True)
    if p.exists():
        print(f"  SKIP  {data['mood']}/{engine_folder}/{data['name']}")
        _skipped += 1
        return
    if not DRY:
        p.write_text(json.dumps(data, indent=2))
    print(f"  WRITE {data['mood']}/{engine_folder}/{data['name']}")
    _written += 1


# ═══════════════════════════════════════════════════════
#  OVERLAP HELPERS
# ═══════════════════════════════════════════════════════

_OB = dict(
    olap_knot=0, olap_tangleDepth=0.3, olap_torusP=3, olap_torusQ=2,
    olap_pulseRate=0.3, olap_entrain=0.2, olap_spread=0.75,
    olap_voiceMode=0, olap_glide=0.0,
    olap_delayBase=25.0, olap_dampening=0.55, olap_feedback=0.88,
    olap_brightness=0.35, olap_bioluminescence=0.12, olap_current=0.08, olap_currentRate=0.04,
    olap_attack=1.0, olap_decay=2.0, olap_sustain=0.85, olap_release=4.0,
    olap_filterCutoff=8000.0, olap_filterRes=0.15, olap_filterEnvAmt=0.15, olap_filterEnvDecay=1.5,
    olap_lfo1Rate=0.15, olap_lfo1Shape=0, olap_lfo1Depth=0.18, olap_lfo1Dest=4,
    olap_lfo2Rate=0.08, olap_lfo2Shape=0, olap_lfo2Depth=0.12, olap_lfo2Dest=2,
    olap_chorusMix=0.1, olap_chorusRate=0.04, olap_diffusion=0.45,
    olap_macroKnot=0.3, olap_macroPulse=0.25, olap_macroEntrain=0.2, olap_macroBloom=0.2,
    olap_modWheelDest=0, olap_modWheelDepth=0.3, olap_atPressureDest=1, olap_atPressureDepth=0.2,
)


def olap(name, mood, tags, desc, d, **p):
    params = dict(_OB)
    for k, v in p.items():
        params[k if k.startswith("olap_") else f"olap_{k}"] = v
    return {
        "author": AUTHOR, "coupling": {"pairs": []}, "couplingIntensity": "None",
        "created": DATE, "description": desc,
        "dna": {"aggression": d[0], "brightness": d[1], "density": d[2],
                "movement": d[3], "space": d[4], "warmth": d[5]},
        "engines": ["Overlap"], "macroLabels": ["KNOT", "PULSE", "ENTRAIN", "BLOOM"],
        "mood": mood, "name": name,
        "parameters": {"Overlap": params},
        "schema_version": 1, "sequencer": None, "tags": tags, "tempo": None, "version": "1.0.0",
    }


# Mood-base shorthands — merge mood defaults with preset-specific overrides, then call olap()
def _atm(name, tags, desc, d, **p):
    base = dict(feedback=0.91, spread=0.85, attack=1.5, release=7.0, pulseRate=0.12,
                entrain=0.1, delayBase=35.0, dampening=0.62, diffusion=0.6,
                bioluminescence=0.08, lfo1Depth=0.12, lfo2Depth=0.08, chorusMix=0.12)
    return olap(name, "Atmosphere", tags, desc, d, **{**base, **p})

def _ent(name, tags, desc, d, **p):
    base = dict(feedback=0.86, spread=0.65, attack=0.1, release=2.5, pulseRate=1.2,
                entrain=0.45, delayBase=15.0, dampening=0.4, diffusion=0.35,
                bioluminescence=0.25, lfo1Dest=4, lfo2Dest=2)
    return olap(name, "Entangled", tags, desc, d, **{**base, **p})

def _flx(name, tags, desc, d, **p):
    base = dict(feedback=0.91, tangleDepth=0.8, spread=0.58, attack=0.02, release=0.8,
                pulseRate=3.0, entrain=0.2, delayBase=8.0, dampening=0.28, diffusion=0.12,
                brightness=0.75, bioluminescence=0.5, filterCutoff=14000.0, filterRes=0.58,
                filterEnvAmt=0.78, filterEnvDecay=0.16,
                lfo1Rate=2.5, lfo1Shape=4, lfo1Depth=0.6, lfo1Dest=4,
                lfo2Rate=1.2, lfo2Shape=3, lfo2Depth=0.48, lfo2Dest=0,
                macroKnot=0.65, macroPulse=0.72, macroEntrain=0.22, macroBloom=0.65)
    return olap(name, "Flux", tags, desc, d, **{**base, **p})

def _aet(name, tags, desc, d, **p):
    base = dict(feedback=0.93, tangleDepth=0.05, spread=0.9, attack=2.0, release=9.0,
                pulseRate=0.06, entrain=0.05, delayBase=42.0, dampening=0.68, diffusion=0.65,
                brightness=0.22, bioluminescence=0.02, current=0.04, currentRate=0.02,
                filterCutoff=10000.0, lfo1Depth=0.07, lfo2Depth=0.05,
                macroKnot=0.15, macroPulse=0.1, macroEntrain=0.08, macroBloom=0.1)
    return olap(name, "Aether", tags, desc, d, **{**base, **p})

def _prs(name, tags, desc, d, **p):
    base = dict(feedback=0.88, tangleDepth=0.35, spread=0.72, attack=0.3, release=3.5,
                pulseRate=0.6, entrain=0.25, delayBase=18.0, dampening=0.42, diffusion=0.4,
                brightness=0.72, bioluminescence=0.45, filterCutoff=15000.0, filterRes=0.3,
                lfo1Depth=0.35, lfo2Depth=0.25, lfo1Dest=4, lfo2Dest=0,
                macroKnot=0.5, macroPulse=0.45, macroEntrain=0.3, macroBloom=0.55)
    return olap(name, "Prism", tags, desc, d, **{**base, **p})

def _fdn(name, tags, desc, d, **p):
    base = dict(feedback=0.82, tangleDepth=0.25, spread=0.45, attack=0.3, release=2.5,
                pulseRate=0.4, entrain=0.35, delayBase=15.0, dampening=0.68, diffusion=0.35,
                brightness=0.28, bioluminescence=0.18, filterCutoff=4000.0,
                lfo1Depth=0.15, lfo2Depth=0.1)
    return olap(name, "Foundation", tags, desc, d, **{**base, **p})


# ═══════════════════════════════════════════════════════
#  OVERLAP PRESETS  (106 total)
# ═══════════════════════════════════════════════════════

OVERLAP_PRESETS = [

    # ── ATMOSPHERE · UNKNOT (8) ──────────────────────────────────────────────
    _atm("Luminous Void",   ["atmosphere","unknot","crystalline","space","sparse"],
         "Near-zero tangle (0.03), feedback 0.95. Pure self-resonance — 6 FDN lines humming in empty space. Maximum spread, crystalline shimmer.",
         (0.02, 0.30, 0.18, 0.04, 0.95, 0.42),
         knot=0, tangleDepth=0.03, feedback=0.95, spread=0.93, delayBase=44.0, dampening=0.65, brightness=0.28,
         bioluminescence=0.0, lfo1Depth=0.08, lfo2Depth=0.05),

    _atm("Resonant Field",  ["atmosphere","unknot","pad","open","resonant"],
         "Pure self-resonance at moderate pulse rate 0.2 Hz. The field breathes slowly, undisturbed by topology.",
         (0.03, 0.32, 0.22, 0.07, 0.90, 0.48),
         knot=0, tangleDepth=0.05, feedback=0.92, spread=0.87, delayBase=38.0, pulseRate=0.2),

    _atm("Empty Shimmer",   ["atmosphere","unknot","bright","shimmer","airy"],
         "Higher brightness (0.48) reveals inharmonic partials glinting above the resonant body. Gentle bioluminescent flashes.",
         (0.03, 0.50, 0.20, 0.10, 0.88, 0.35),
         knot=0, tangleDepth=0.08, feedback=0.91, spread=0.88, brightness=0.48, bioluminescence=0.2,
         filterCutoff=11000.0),

    _atm("Quiet Expanse",   ["atmosphere","unknot","vast","minimal","long"],
         "Maximum space, minimum movement. 2-second attack, 10-second release, pulse rate 0.04. The slowest breath in the library.",
         (0.01, 0.22, 0.15, 0.02, 0.97, 0.44),
         knot=0, tangleDepth=0.02, feedback=0.94, spread=0.95, attack=2.0, release=10.0, pulseRate=0.04,
         bioluminescence=0.0, lfo1Depth=0.05, lfo2Depth=0.03),

    _atm("Open Field",      ["atmosphere","unknot","pad","natural","open"],
         "Standard unknot atmosphere. Clean self-resonance at moderate width. The reliable foundation of the Overlap library.",
         (0.03, 0.33, 0.23, 0.06, 0.88, 0.45),
         knot=0, tangleDepth=0.06, feedback=0.90, spread=0.85, delayBase=32.0, pulseRate=0.18),

    _atm("Unknot Rain",     ["atmosphere","unknot","rain","organic","movement"],
         "Gentle bioluminescent onset flashes and slow current create the impression of soft rain on still water.",
         (0.04, 0.38, 0.25, 0.12, 0.85, 0.46),
         knot=0, tangleDepth=0.10, feedback=0.89, bioluminescence=0.28, pulseRate=0.22,
         current=0.1, currentRate=0.05),

    _atm("Soft Resonance",  ["atmosphere","unknot","warm","dark","smooth"],
         "Warmth-biased unknot: dampening 0.72, filter at 5500 Hz, low brightness. The resonance becomes a gentle rumble.",
         (0.02, 0.20, 0.20, 0.05, 0.90, 0.62),
         knot=0, tangleDepth=0.04, feedback=0.92, dampening=0.72, filterCutoff=5500.0, brightness=0.18),

    _atm("Hollow Chamber",  ["atmosphere","unknot","cavernous","reverb","diffuse"],
         "High diffusion (0.78) and long delay base (48ms). The FDN becomes a cathedral — sound arrives from everywhere.",
         (0.02, 0.28, 0.18, 0.05, 0.93, 0.40),
         knot=0, tangleDepth=0.05, feedback=0.93, diffusion=0.78, delayBase=48.0, dampening=0.72,
         chorusMix=0.15),

    # ── ATMOSPHERE · TREFOIL (8) ─────────────────────────────────────────────
    _atm("Three Rivers",    ["atmosphere","trefoil","flowing","harmonic","three"],
         "Trefoil T(3) at tangle 0.38. Three resonant streams merge and diverge in slow motion.",
         (0.04, 0.35, 0.28, 0.08, 0.86, 0.45),
         knot=1, tangleDepth=0.38, feedback=0.90, spread=0.82, delayBase=30.0, pulseRate=0.15),

    _atm("Clover Space",    ["atmosphere","trefoil","wide","open","spacious"],
         "Wide spread (0.9) with trefoil crossing. The three paths separate into a panoramic resonant field.",
         (0.04, 0.35, 0.30, 0.09, 0.89, 0.42),
         knot=1, tangleDepth=0.42, feedback=0.88, spread=0.9, diffusion=0.65),

    _atm("Trinity Pad",     ["atmosphere","trefoil","warm","entrained","pad"],
         "Entrain 0.3 pulls the three trefoil voices toward phase coherence — a warm, stable harmonic cluster.",
         (0.05, 0.32, 0.32, 0.10, 0.82, 0.52),
         knot=1, tangleDepth=0.45, feedback=0.89, entrain=0.3, filterCutoff=6500.0, brightness=0.3),

    _atm("Trefoil Rain",    ["atmosphere","trefoil","rain","bioluminescent","organic"],
         "Bioluminescence 0.3 and 0.25 Hz pulse create a shower of onset sparks over the trefoil bed.",
         (0.05, 0.40, 0.30, 0.15, 0.82, 0.40),
         knot=1, tangleDepth=0.4, bioluminescence=0.3, pulseRate=0.25, currentRate=0.06),

    _atm("Three Moons",     ["atmosphere","trefoil","crystalline","bright","space"],
         "High brightness (0.52) with trefoil rotation makes three shimmering orbital bodies — cold light.",
         (0.04, 0.55, 0.25, 0.08, 0.86, 0.32),
         knot=1, tangleDepth=0.32, feedback=0.91, brightness=0.52, filterCutoff=12000.0),

    _atm("Triple Haze",     ["atmosphere","trefoil","dark","warm","haze"],
         "Dampening 0.75 and low filterCutoff (4500 Hz) sink the trefoil rotation into a thick harmonic haze.",
         (0.04, 0.20, 0.35, 0.07, 0.83, 0.58),
         knot=1, tangleDepth=0.5, dampening=0.75, filterCutoff=4500.0, brightness=0.18),

    _atm("Harmonic Web",    ["atmosphere","trefoil","woven","entrained","delicate"],
         "Light entrainment (0.2) and moderate diffusion weave the three crossings into a suspended web.",
         (0.04, 0.38, 0.32, 0.10, 0.84, 0.47),
         knot=1, tangleDepth=0.43, entrain=0.2, feedback=0.89, lfo1Depth=0.15, diffusion=0.55),

    _atm("Clover Bloom",    ["atmosphere","trefoil","evolving","bloom","macro"],
         "Macro-ready trefoil. macroBloom 0.4 opens bioluminescent onset bursts. macroKnot shifts tangle depth.",
         (0.05, 0.42, 0.33, 0.13, 0.82, 0.44),
         knot=1, tangleDepth=0.48, macroKnot=0.5, macroBloom=0.4, bioluminescence=0.2,
         filterEnvAmt=0.25),

    _atm("Three Voices",    ["atmosphere","trefoil","harmonic","trio","balanced"],
         "Balanced trefoil at tangle 0.36 — three voices at equal weight, spread 0.84, almost vocal.",
         (0.04, 0.37, 0.30, 0.09, 0.86, 0.46),
         knot=1, tangleDepth=0.36, feedback=0.90, spread=0.84, entrain=0.15, delayBase=28.0),

    # ── ATMOSPHERE · FIGURE-EIGHT (8) ────────────────────────────────────────
    _atm("Lemniscate Wash", ["atmosphere","figure-eight","symmetric","wide","clean"],
         "Figure-eight at tangle 0.25. Amphichiral symmetry: left and right resonate as perfect mirrors.",
         (0.03, 0.33, 0.25, 0.07, 0.90, 0.44),
         knot=2, tangleDepth=0.25, feedback=0.92, spread=0.9, diffusion=0.65, delayBase=38.0),

    _atm("Infinity Pad",    ["atmosphere","figure-eight","endless","long","vast"],
         "2-second attack, 10-second release, 0.05 Hz pulse. The figure-eight loops endlessly, never resolving.",
         (0.02, 0.30, 0.22, 0.04, 0.92, 0.46),
         knot=2, tangleDepth=0.28, feedback=0.93, spread=0.88, attack=2.0, release=10.0, pulseRate=0.05),

    _atm("Double Mirror",   ["atmosphere","figure-eight","symmetric","mirror","wide"],
         "Near-perfect symmetry: diffusion 0.7, dampening 0.65, spread 0.93. Every partial mirrored.",
         (0.03, 0.32, 0.24, 0.06, 0.91, 0.43),
         knot=2, tangleDepth=0.3, feedback=0.91, spread=0.93, diffusion=0.7, dampening=0.65),

    _atm("Looped Horizon",  ["atmosphere","figure-eight","warm","gentle","horizon"],
         "Warm-leaning figure-eight: filterCutoff 7000 Hz, lower brightness. A soft loop on the edge of hearing.",
         (0.04, 0.30, 0.27, 0.08, 0.87, 0.50),
         knot=2, tangleDepth=0.22, feedback=0.89, delayBase=32.0, filterCutoff=7000.0, brightness=0.28),

    _atm("Symmetric Space", ["atmosphere","figure-eight","pure","balanced","open"],
         "Pure amphichiral character: tangle 0.2, no modulation bias. The cleanest figure-eight in the set.",
         (0.03, 0.33, 0.22, 0.07, 0.88, 0.43),
         knot=2, tangleDepth=0.2, feedback=0.92, spread=0.85, diffusion=0.62),

    _atm("Endless Field",   ["atmosphere","figure-eight","vast","release","space"],
         "Release 9.0s, high dampening (0.68) — the figure-eight decay is a journey, not an ending.",
         (0.02, 0.28, 0.18, 0.05, 0.93, 0.44),
         knot=2, tangleDepth=0.18, feedback=0.94, spread=0.9, release=9.0, dampening=0.68),

    _atm("Figure Rain",     ["atmosphere","figure-eight","rain","movement","organic"],
         "LFO modulation and 0.2 Hz pulse breathe life into the figure-eight bed. Gentle onset shimmer.",
         (0.04, 0.40, 0.28, 0.15, 0.84, 0.42),
         knot=2, tangleDepth=0.32, bioluminescence=0.22, lfo1Depth=0.2, pulseRate=0.2, currentRate=0.06),

    _atm("Two Paths",       ["atmosphere","figure-eight","dual","balanced","crossing"],
         "Tangle 0.35 makes the two paths feel distinct: slight pulse, gentle entrain. A crossing of equals.",
         (0.04, 0.36, 0.28, 0.12, 0.85, 0.44),
         knot=2, tangleDepth=0.35, feedback=0.88, spread=0.82, pulseRate=0.25, entrain=0.18,
         bioluminescence=0.15),

    # ── ATMOSPHERE · TORUS (7) ───────────────────────────────────────────────
    _atm("Ring Nebula",     ["atmosphere","torus","fifth","harmonic","space"],
         "T(3,2) at tangle 0.45 — the perfect fifth ratio locks six delay lines into a resonant nebula.",
         (0.04, 0.38, 0.32, 0.08, 0.85, 0.45),
         knot=3, torusP=3, torusQ=2, tangleDepth=0.45, feedback=0.90, spread=0.82, delayBase=28.0),

    _atm("Donut Haze",      ["atmosphere","torus","rich","dense","overtone"],
         "T(4,3) ratio creates a denser harmonic lock — more overtones than T(3,2), warmer quality.",
         (0.05, 0.38, 0.35, 0.10, 0.83, 0.46),
         knot=3, torusP=4, torusQ=3, tangleDepth=0.5, feedback=0.88, spread=0.80, brightness=0.35),

    _atm("Toroidal Sky",    ["atmosphere","torus","open","fifth","airy"],
         "T(5,2) — higher prime in the p direction opens the torus. The lock feels more distant, airy.",
         (0.04, 0.37, 0.30, 0.09, 0.87, 0.44),
         knot=3, torusP=5, torusQ=2, tangleDepth=0.42, feedback=0.91, spread=0.85, delayBase=25.0),

    _atm("Orbital Wash",    ["atmosphere","torus","orbital","motion","complex"],
         "T(4,3) with slow pulse (0.18 Hz). The orbital motion of the torus becomes a wash of slow revolution.",
         (0.05, 0.40, 0.34, 0.11, 0.82, 0.44),
         knot=3, torusP=4, torusQ=3, tangleDepth=0.55, feedback=0.89, spread=0.78, pulseRate=0.18),

    _atm("Winding Haze",    ["atmosphere","torus","complex","high-prime","winding"],
         "T(7,3) — seven windings in one direction, three in the other. Complex ratio creates interference clusters.",
         (0.05, 0.40, 0.35, 0.10, 0.84, 0.42),
         knot=3, torusP=7, torusQ=3, tangleDepth=0.48, feedback=0.90, spread=0.80, diffusion=0.58),

    _atm("Solar Ring",      ["atmosphere","torus","warm","golden","fifth"],
         "T(3,2) with warm filter (6000 Hz) and high dampening. The fifth harmonic locks into a golden ring.",
         (0.04, 0.30, 0.33, 0.08, 0.83, 0.56),
         knot=3, torusP=3, torusQ=2, tangleDepth=0.5, feedback=0.88, filterCutoff=6000.0,
         brightness=0.30, dampening=0.68),

    _atm("Fifth Wash",      ["atmosphere","torus","fifth","bright","harmonic"],
         "T(3,2) brighter variant: brightness 0.4, filterCutoff 12000. The fifth harmonic glints.",
         (0.04, 0.42, 0.28, 0.07, 0.88, 0.43),
         knot=3, torusP=3, torusQ=2, tangleDepth=0.38, feedback=0.92, spread=0.88, brightness=0.4),

    # ── ENTANGLED · UNKNOT (6) ──────────────────────────────────────────────
    _ent("Pulse Scatter",   ["entangled","unknot","pulse","scatter","rhythmic"],
         "Fast scatter pulse at 2.0 Hz, low entrain (0.15) lets voices scatter independently.",
         (0.20, 0.45, 0.42, 0.55, 0.55, 0.38),
         knot=0, tangleDepth=0.3, pulseRate=2.0, entrain=0.15, spread=0.70, release=2.0),

    _ent("Open Rhythm",     ["entangled","unknot","rhythm","clean","pulse"],
         "Clean rhythmic pulse at 1.5 Hz, entrain 0.4. Structured without topology — pure Kuramoto oscillation.",
         (0.18, 0.40, 0.40, 0.50, 0.58, 0.40),
         knot=0, tangleDepth=0.25, pulseRate=1.5, entrain=0.4, feedback=0.84, spread=0.62),

    _ent("Unknot Groove",   ["entangled","unknot","groove","driving","square"],
         "Square LFO (shape 3) on filter and S&H on pulse rate. Groovy, driving without topology.",
         (0.25, 0.48, 0.45, 0.60, 0.52, 0.38),
         knot=0, tangleDepth=0.32, pulseRate=2.5, entrain=0.55,
         lfo1Shape=3, lfo1Dest=4, lfo1Depth=0.35, lfo2Dest=2),

    _ent("Resonant Trigger",["entangled","unknot","trigger","bioluminescent","organic"],
         "Short attack (0.05s) makes each pulse feel triggered. Bioluminescence 0.4 flashes on each onset.",
         (0.22, 0.50, 0.40, 0.52, 0.55, 0.38),
         knot=0, tangleDepth=0.28, feedback=0.88, pulseRate=1.8, entrain=0.3,
         bioluminescence=0.4, attack=0.05),

    _ent("Free Pulse",      ["entangled","unknot","free","loose","movement"],
         "Low entrain (0.1) lets voices pulse at independent rates. LFO sweeps pulse rate for organic drift.",
         (0.18, 0.42, 0.38, 0.48, 0.60, 0.40),
         knot=0, tangleDepth=0.35, pulseRate=1.0, entrain=0.1, spread=0.70,
         lfo2Rate=0.5, lfo2Depth=0.3, lfo2Dest=2),

    _ent("Entrain Open",    ["entangled","unknot","phase-lock","coherent","sync"],
         "High entrain (0.8) phase-locks all six voices to a single pulse — organized, martial.",
         (0.20, 0.40, 0.42, 0.48, 0.56, 0.42),
         knot=0, tangleDepth=0.2, pulseRate=1.5, entrain=0.8, feedback=0.85, spread=0.65),

    # ── ENTANGLED · TREFOIL (6) ──────────────────────────────────────────────
    _ent("Three Phase",     ["entangled","trefoil","phase","three","pulse"],
         "Trefoil with three distinct pulse phases at low entrain (0.25). Each crossing rotates separately.",
         (0.25, 0.48, 0.45, 0.55, 0.52, 0.38),
         knot=1, tangleDepth=0.5, pulseRate=1.5, entrain=0.25, spread=0.68),

    _ent("Trefoil Gate",    ["entangled","trefoil","gated","square","rhythmic"],
         "Square LFO gates the filter at pulse rate. The trefoil crossing adds harmonic complexity to a hard gate.",
         (0.28, 0.52, 0.48, 0.62, 0.48, 0.35),
         knot=1, tangleDepth=0.55, pulseRate=2.0, entrain=0.4,
         lfo1Shape=3, lfo1Depth=0.45, lfo1Dest=4),

    _ent("Rotating Sequence",["entangled","trefoil","rotating","sequence","motion"],
         "S&H LFO on tangle depth creates stepped tangle variations. Trefoil crossing rotates between states.",
         (0.25, 0.50, 0.45, 0.58, 0.50, 0.37),
         knot=1, tangleDepth=0.48, pulseRate=1.8, entrain=0.35, spread=0.70,
         lfo2Rate=0.8, lfo2Shape=4, lfo2Dest=0),

    _ent("Triple Pulse",    ["entangled","trefoil","three","pulse","bioluminescent"],
         "Three-way pulsing at 2.5 Hz. Bioluminescence 0.35 fires on each triple-crossing onset.",
         (0.30, 0.52, 0.48, 0.65, 0.45, 0.35),
         knot=1, tangleDepth=0.52, pulseRate=2.5, entrain=0.5, bioluminescence=0.35, spread=0.65),

    _ent("Clover Rhythm",   ["entangled","trefoil","rhythmic","filter","groove"],
         "Filter resonance (0.25) at 10kHz adds bite to trefoil rhythm. Slightly brighter character.",
         (0.22, 0.48, 0.43, 0.52, 0.52, 0.40),
         knot=1, tangleDepth=0.45, pulseRate=1.2, entrain=0.42,
         filterCutoff=10000.0, filterRes=0.25),

    _ent("Triadic Loop",    ["entangled","trefoil","loop","triadic","driven"],
         "LFO1 on filter at 1.0 Hz creates looped triadic filtering over continuous trefoil rotation.",
         (0.28, 0.50, 0.47, 0.60, 0.48, 0.38),
         knot=1, tangleDepth=0.5, pulseRate=2.2, entrain=0.55, feedback=0.85,
         spread=0.62, lfo1Rate=1.0, lfo1Dest=4),

    # ── ENTANGLED · FIGURE-EIGHT (6) ────────────────────────────────────────
    _ent("Symmetric Gate",  ["entangled","figure-eight","gated","symmetric","rhythmic"],
         "Gated figure-eight: square LFO at pulse rate gates the filter. Symmetry makes L and R gate equally.",
         (0.25, 0.50, 0.44, 0.58, 0.50, 0.36),
         knot=2, tangleDepth=0.55, pulseRate=2.0, entrain=0.35,
         lfo1Shape=3, lfo1Depth=0.4, lfo1Dest=4),

    _ent("Looped Rhythm",   ["entangled","figure-eight","loop","rhythmic","clean"],
         "Rhythmic figure-eight at moderate pulse. Feedback 0.84 keeps the loop tight.",
         (0.22, 0.47, 0.42, 0.55, 0.52, 0.38),
         knot=2, tangleDepth=0.5, pulseRate=1.5, entrain=0.4, feedback=0.84, spread=0.65),

    _ent("Double Eight",    ["entangled","figure-eight","double","bioluminescent","crossing"],
         "Two crossing rhythms: pulseRate 2.2, bioluminescence 0.3 marks each crossing. Wide panoramic.",
         (0.27, 0.52, 0.46, 0.60, 0.50, 0.35),
         knot=2, tangleDepth=0.58, pulseRate=2.2, entrain=0.28, spread=0.72, bioluminescence=0.3),

    _ent("Mirror Pulse",    ["entangled","figure-eight","mirror","pulse","sync"],
         "Entrain 0.5 half-locks the mirror — each reflection pulses slightly differently, creating phasing.",
         (0.25, 0.48, 0.44, 0.57, 0.52, 0.38),
         knot=2, tangleDepth=0.48, pulseRate=1.8, entrain=0.5, diffusion=0.4, spread=0.68),

    _ent("Amphichiral Beat",["entangled","figure-eight","beat","s&h","unstable"],
         "S&H LFO on pulse rate creates random beat variations while the amphichiral matrix stays symmetric.",
         (0.30, 0.55, 0.48, 0.65, 0.45, 0.33),
         knot=2, tangleDepth=0.52, pulseRate=3.0, entrain=0.38,
         lfo1Shape=4, lfo1Rate=2.0, lfo1Dest=2),

    _ent("Figure Loop",     ["entangled","figure-eight","tight","loop","driving"],
         "Short delayBase (12ms), tight loop. Attack 0.08, release 1.8 for punchy figure-eight groove.",
         (0.28, 0.50, 0.47, 0.62, 0.48, 0.37),
         knot=2, tangleDepth=0.55, pulseRate=2.5, entrain=0.45,
         attack=0.08, release=1.8, delayBase=12.0),

    # ── ENTANGLED · TORUS (7) ───────────────────────────────────────────────
    _ent("Harmonic Step",   ["entangled","torus","harmonic","stepping","fifth"],
         "T(3,2) with 1.5 Hz pulse and entrain 0.45. Fifth harmonic lock provides a musical stepping point.",
         (0.25, 0.50, 0.45, 0.55, 0.52, 0.40),
         knot=3, torusP=3, torusQ=2, tangleDepth=0.55, pulseRate=1.5, entrain=0.45),

    _ent("Fifth Engine",    ["entangled","torus","fifth","engine","driven"],
         "T(3,2) at 2.0 Hz — the fifth harmonic ratio as a rhythmic engine. Feedback 0.87 keeps energy high.",
         (0.28, 0.52, 0.48, 0.62, 0.48, 0.42),
         knot=3, torusP=3, torusQ=2, tangleDepth=0.6, pulseRate=2.0, entrain=0.5, feedback=0.87),

    _ent("Locked Groove",   ["entangled","torus","locked","groove","phase"],
         "T(5,2) with entrain 0.7 — near-full phase lock with 5:2 ratio. The groove cannot escape the lock.",
         (0.25, 0.50, 0.50, 0.58, 0.48, 0.42),
         knot=3, torusP=5, torusQ=2, tangleDepth=0.58, pulseRate=1.8, entrain=0.7, spread=0.62),

    _ent("Torus Arp",       ["entangled","torus","arp","T(4,3)","fast"],
         "T(4,3) at 3.5 Hz with short attack (0.05). The ratio complexity becomes arpeggio-like movement.",
         (0.30, 0.55, 0.48, 0.68, 0.45, 0.38),
         knot=3, torusP=4, torusQ=3, tangleDepth=0.52, pulseRate=3.5, entrain=0.35, attack=0.05),

    _ent("Ratio Sequence",  ["entangled","torus","ratio","sequence","complex"],
         "T(7,2) — high prime creates complex ratio. The 7:2 sequencing is irregular but musical.",
         (0.28, 0.52, 0.47, 0.62, 0.50, 0.40),
         knot=3, torusP=7, torusQ=2, tangleDepth=0.5, pulseRate=2.5, entrain=0.55),

    _ent("Winding Beat",    ["entangled","torus","winding","beat","complex"],
         "T(5,3) with LFO1 rate 1.5 Hz on filter. The winding ratio becomes a complex rhythmic texture.",
         (0.25, 0.50, 0.48, 0.60, 0.50, 0.40),
         knot=3, torusP=5, torusQ=3, tangleDepth=0.55, pulseRate=2.0, entrain=0.45,
         lfo1Rate=1.5, lfo1Dest=4),

    _ent("Fifth Loop",      ["entangled","torus","fifth","loop","harmonic"],
         "T(3,2) looping fifth: entrain 0.6 creates a near-locked loop within the torus ratio.",
         (0.25, 0.50, 0.46, 0.55, 0.52, 0.42),
         knot=3, torusP=3, torusQ=2, tangleDepth=0.45, pulseRate=1.5, entrain=0.6,
         feedback=0.86, spread=0.65),

    # ── FLUX · UNKNOT (3) ───────────────────────────────────────────────────
    _flx("Unknot Storm",    ["flux","unknot","storm","chaotic","high-feedback"],
         "Unknot at feedback 0.95 and tangle 0.62. Self-resonance becomes an unstable weather system.",
         (0.75, 0.70, 0.42, 0.82, 0.32, 0.22),
         knot=0, tangleDepth=0.62, feedback=0.95, pulseRate=5.0, entrain=0.1, spread=0.55),

    _flx("Wild Ring",       ["flux","unknot","wild","s&h","chaotic"],
         "S&H LFO at 4.0 Hz on filter and pulse rate. The unknot resonance has no memory — pure wild chaos.",
         (0.78, 0.75, 0.40, 0.85, 0.35, 0.25),
         knot=0, tangleDepth=0.7, feedback=0.94, pulseRate=4.5,
         lfo1Shape=4, lfo1Rate=4.0, lfo1Depth=0.7, lfo1Dest=4,
         lfo2Shape=4, lfo2Rate=2.5, lfo2Dest=2),

    _flx("Feedback Ring",   ["flux","unknot","extreme","feedback","limit"],
         "Feedback 0.96 at delayBase 5ms — the FDN rings at near-infinite resonance. Minimal dampening.",
         (0.82, 0.72, 0.38, 0.88, 0.28, 0.18),
         knot=0, tangleDepth=0.65, feedback=0.96, pulseRate=6.0,
         dampening=0.22, delayBase=5.0, attack=0.01, release=0.6),

    # ── FLUX · TREFOIL (4) ──────────────────────────────────────────────────
    _flx("Trefoil Break",   ["flux","trefoil","breaking","chaos","unstable"],
         "T(trefoil) at tangle 0.85, feedback 0.92. The three crossings cannot maintain order at this energy.",
         (0.80, 0.75, 0.45, 0.85, 0.30, 0.22),
         knot=1, tangleDepth=0.85, feedback=0.92, pulseRate=3.5, entrain=0.15, spread=0.52),

    _flx("Rotating Chaos",  ["flux","trefoil","rotating","s&h","chaotic"],
         "S&H LFO on tangle depth randomly rotates the trefoil matrix, breaking all harmonic locks.",
         (0.82, 0.78, 0.45, 0.88, 0.30, 0.20),
         knot=1, tangleDepth=0.82, feedback=0.90, pulseRate=4.0,
         lfo2Shape=4, lfo2Rate=2.0, lfo2Dest=0, lfo2Depth=0.55),

    _flx("Triple Surge",    ["flux","trefoil","surge","bioluminescent","intense"],
         "Feedback 0.93, tangle 0.88, bioluminescence 0.6. Each triple crossing fires an intense burst.",
         (0.85, 0.80, 0.48, 0.90, 0.28, 0.18),
         knot=1, tangleDepth=0.88, feedback=0.93, pulseRate=4.5,
         bioluminescence=0.6, filterRes=0.65),

    _flx("Trefoil Storm",   ["flux","trefoil","extreme","storm","maximum"],
         "Maximum trefoil chaos: tangle 0.9, feedback 0.94, brightness 0.85. The three crossings are white noise.",
         (0.88, 0.88, 0.50, 0.92, 0.25, 0.16),
         knot=1, tangleDepth=0.9, feedback=0.94, pulseRate=5.0, brightness=0.85, entrain=0.12),

    # ── FLUX · FIGURE-EIGHT (4) ─────────────────────────────────────────────
    _flx("Eight Collapse",  ["flux","figure-eight","collapse","symmetric","chaos"],
         "Figure-eight at tangle 0.85. The symmetry collapses — left and right lose their mirror relationship.",
         (0.80, 0.75, 0.44, 0.84, 0.32, 0.22),
         knot=2, tangleDepth=0.85, feedback=0.91, pulseRate=3.5, entrain=0.18, spread=0.55),

    _flx("Loop Dissolution",["flux","figure-eight","dissolving","dampening","chaos"],
         "Low diffusion (0.08) and dampening 0.22 strip away smoothness. The loop is unraveling.",
         (0.78, 0.72, 0.42, 0.85, 0.30, 0.20),
         knot=2, tangleDepth=0.82, feedback=0.92, pulseRate=4.0,
         diffusion=0.08, dampening=0.22),

    _flx("Mirror Break",    ["flux","figure-eight","broken-mirror","s&h","unstable"],
         "S&H LFO at 3.5 Hz randomly shatter the filter. The mirror is cracked — L and R diverge.",
         (0.82, 0.78, 0.46, 0.87, 0.30, 0.20),
         knot=2, tangleDepth=0.88, feedback=0.90, pulseRate=3.8,
         lfo1Shape=4, lfo1Rate=3.5, lfo1Dest=4),

    _flx("Symmetric Storm", ["flux","figure-eight","symmetric","extreme","storm"],
         "Maximum figure-eight chaos while preserving amphichiral symmetry. Both sides equally destroyed.",
         (0.85, 0.82, 0.46, 0.90, 0.27, 0.18),
         knot=2, tangleDepth=0.85, feedback=0.93, pulseRate=4.5,
         entrain=0.1, bioluminescence=0.65),

    # ── FLUX · TORUS (4) ────────────────────────────────────────────────────
    _flx("Torus Shatter",   ["flux","torus","shatter","T(5,2)","chaos"],
         "T(5,2) at tangle 0.85 — the 5:2 harmonic lock is shattering. Order and chaos coexist briefly.",
         (0.82, 0.78, 0.48, 0.88, 0.30, 0.22),
         knot=3, torusP=5, torusQ=2, tangleDepth=0.85, feedback=0.92, pulseRate=4.0),

    _flx("Fifth Chaos",     ["flux","torus","T(3,2)","chaos","fifth"],
         "T(3,2) in full chaos: square LFO periodically restores and destroys the fifth lock.",
         (0.80, 0.75, 0.46, 0.88, 0.30, 0.22),
         knot=3, torusP=3, torusQ=2, tangleDepth=0.88, feedback=0.91, pulseRate=4.5,
         lfo2Shape=3, lfo2Rate=1.5, lfo2Dest=0, lfo2Depth=0.5),

    _flx("Harmonic Collapse",["flux","torus","collapse","T(7,3)","extreme"],
         "T(7,3): the most complex harmonic ratio collapsing. Seven wrappings unwinding into noise.",
         (0.84, 0.82, 0.50, 0.88, 0.28, 0.18),
         knot=3, torusP=7, torusQ=3, tangleDepth=0.82, feedback=0.93, pulseRate=3.8, brightness=0.82),

    _flx("Torus Storm",     ["flux","torus","extreme","T(5,3)","maximum"],
         "T(5,3): feedback 0.95, tangle 0.9, pulse 5.5 Hz. Maximum torus chaos before self-oscillation.",
         (0.88, 0.85, 0.50, 0.93, 0.25, 0.15),
         knot=3, torusP=5, torusQ=3, tangleDepth=0.9, feedback=0.95, pulseRate=5.5, entrain=0.08),

    # ── AETHER · UNKNOT (5) ─────────────────────────────────────────────────
    _aet("Crystal Void",    ["aether","unknot","crystal","sparse","pure"],
         "Near-zero tangle (0.03), feedback 0.95, zero bioluminescence. Pure self-resonance in void.",
         (0.01, 0.30, 0.12, 0.02, 0.96, 0.38),
         knot=0, tangleDepth=0.03, feedback=0.95, spread=0.92, brightness=0.28, bioluminescence=0.0),

    _aet("Empty Ring",      ["aether","unknot","empty","void","infinite"],
         "Feedback 0.96, delayBase 46ms, dampening 0.70. Maximum delay → maximum perceived space.",
         (0.01, 0.25, 0.10, 0.02, 0.97, 0.47),
         knot=0, tangleDepth=0.02, feedback=0.96, spread=0.94, delayBase=46.0, dampening=0.70),

    _aet("Minimal Pulse",   ["aether","unknot","minimal","sparse","pulse"],
         "Pulse rate 0.03 Hz — one pulse every 33 seconds. The FDN barely breathes.",
         (0.01, 0.18, 0.12, 0.03, 0.95, 0.42),
         knot=0, tangleDepth=0.05, feedback=0.94, pulseRate=0.03, spread=0.90, brightness=0.15),

    _aet("Sparse Field",    ["aether","unknot","sparse","clean","void"],
         "No bioluminescence, minimal current (0.02). The FDN resonates untouched in open space.",
         (0.01, 0.22, 0.12, 0.03, 0.94, 0.40),
         knot=0, tangleDepth=0.04, feedback=0.93, spread=0.88, bioluminescence=0.0, current=0.02),

    _aet("Ghost Signal",    ["aether","unknot","ghost","faint","ethereal"],
         "Release 10s, brightness 0.2. The signal is almost gone — a ghost frequency in empty space.",
         (0.01, 0.22, 0.12, 0.03, 0.96, 0.38),
         knot=0, tangleDepth=0.06, feedback=0.95, spread=0.90, release=10.0, brightness=0.2,
         dampening=0.72),

    # ── AETHER · TREFOIL (4) ────────────────────────────────────────────────
    _aet("Trefoil Crystal", ["aether","trefoil","crystal","sparse","delicate"],
         "Trefoil at minimal tangle (0.12), near-zero bio. Three faint crossing points in the void.",
         (0.02, 0.28, 0.15, 0.04, 0.92, 0.38),
         knot=1, tangleDepth=0.12, feedback=0.93, spread=0.88, brightness=0.25, bioluminescence=0.0),

    _aet("Three Whispers",  ["aether","trefoil","whisper","quiet","sparse"],
         "High dampening (0.72), attack 2.0. The three crossings whisper rather than resonate.",
         (0.02, 0.22, 0.14, 0.03, 0.93, 0.40),
         knot=1, tangleDepth=0.08, feedback=0.94, spread=0.90, dampening=0.72, attack=2.0),

    _aet("Sparse Trefoil",  ["aether","trefoil","sparse","minimal","clean"],
         "Tangle 0.1, filterCutoff 9000. Three crossing paths barely disturb the resonant field.",
         (0.02, 0.25, 0.15, 0.04, 0.90, 0.38),
         knot=1, tangleDepth=0.1, feedback=0.92, spread=0.87, filterCutoff=9000.0),

    _aet("Crystal Web",     ["aether","trefoil","crystal","woven","diffuse"],
         "High diffusion (0.70) smears the trefoil crossings into a crystalline web of resonance.",
         (0.02, 0.30, 0.14, 0.04, 0.93, 0.36),
         knot=1, tangleDepth=0.12, feedback=0.93, diffusion=0.70, brightness=0.28, spread=0.90),

    # ── AETHER · FIGURE-EIGHT (4) ───────────────────────────────────────────
    _aet("Eight Echo",      ["aether","figure-eight","echo","faint","long"],
         "Release 9.5s, brightness 0.2. The figure-eight echo decays over nearly ten seconds.",
         (0.01, 0.22, 0.13, 0.03, 0.94, 0.40),
         knot=2, tangleDepth=0.08, feedback=0.94, spread=0.91, release=9.5, brightness=0.2),

    _aet("Lemniscate Crystal",["aether","figure-eight","crystal","pure","diffuse"],
         "Pure crystal lemniscate: diffusion 0.70, brightness 0.25, near-zero tangle.",
         (0.01, 0.27, 0.12, 0.03, 0.95, 0.38),
         knot=2, tangleDepth=0.06, feedback=0.95, spread=0.92, diffusion=0.70, brightness=0.25),

    _aet("Ghost Eight",     ["aether","figure-eight","ghost","attack","slow"],
         "2-second attack, dampening 0.72. The figure-eight takes two seconds to materialize.",
         (0.02, 0.22, 0.13, 0.03, 0.92, 0.40),
         knot=2, tangleDepth=0.1, feedback=0.93, spread=0.88, attack=2.0, dampening=0.72),

    _aet("Sparse Loop",     ["aether","figure-eight","sparse","loop","minimal"],
         "Zero bioluminescence, filterCutoff 9500. The loop is almost silent — presence felt not heard.",
         (0.01, 0.23, 0.12, 0.03, 0.94, 0.38),
         knot=2, tangleDepth=0.07, feedback=0.94, spread=0.90, bioluminescence=0.0,
         filterCutoff=9500.0),

    # ── AETHER · TORUS (3) ──────────────────────────────────────────────────
    _aet("Torus Whisper",   ["aether","torus","whisper","T(3,2)","sparse"],
         "T(3,2) at minimal tangle (0.10). The fifth harmonic lock barely holds — a whisper of a ratio.",
         (0.02, 0.28, 0.15, 0.04, 0.92, 0.40),
         knot=3, torusP=3, torusQ=2, tangleDepth=0.1, feedback=0.93, spread=0.88, release=9.0),

    _aet("Fifth Crystal",   ["aether","torus","crystal","fifth","bright"],
         "T(3,2) crystalline variant: brightness 0.28 gives the fifth harmonic a glinting quality.",
         (0.02, 0.30, 0.14, 0.04, 0.93, 0.38),
         knot=3, torusP=3, torusQ=2, tangleDepth=0.08, feedback=0.94, spread=0.90, brightness=0.28),

    _aet("Ring Echo",       ["aether","torus","echo","T(5,3)","sparse"],
         "T(5,3) ring echo: dampening 0.70, tangle 0.10. The complex ratio is barely audible.",
         (0.02, 0.25, 0.15, 0.04, 0.91, 0.40),
         knot=3, torusP=5, torusQ=3, tangleDepth=0.1, feedback=0.93, spread=0.87, dampening=0.70),

    # ── PRISM · UNKNOT (3) ──────────────────────────────────────────────────
    _prs("Spectral Ring",   ["prism","unknot","spectral","bright","shimmer"],
         "High brightness (0.75), filterCutoff 16kHz. Self-resonance as pure spectral shimmer.",
         (0.10, 0.78, 0.28, 0.25, 0.68, 0.28),
         knot=0, tangleDepth=0.3, feedback=0.88, brightness=0.75, filterCutoff=16000.0, spread=0.75),

    _prs("Prismatic Void",  ["prism","unknot","prismatic","airy","ultra-bright"],
         "Brightness 0.82, filterCutoff 18kHz. The void becomes a prism: pure light, no warmth.",
         (0.08, 0.85, 0.22, 0.20, 0.72, 0.22),
         knot=0, tangleDepth=0.2, feedback=0.90, brightness=0.82, filterCutoff=18000.0, spread=0.80),

    _prs("Rainbow Field",   ["prism","unknot","rainbow","bioluminescent","colorful"],
         "Bioluminescence 0.55 creates colorful onset bursts across the bright spectral field.",
         (0.10, 0.82, 0.28, 0.30, 0.70, 0.25),
         knot=0, tangleDepth=0.28, feedback=0.87, brightness=0.78, bioluminescence=0.55,
         filterCutoff=17000.0),

    # ── PRISM · TREFOIL (4) ─────────────────────────────────────────────────
    _prs("Spectral Trefoil",["prism","trefoil","spectral","bright","harmonic"],
         "Trefoil T(3) at brightness 0.78. The three crossings refract into spectral bands.",
         (0.12, 0.80, 0.32, 0.28, 0.65, 0.25),
         knot=1, tangleDepth=0.38, feedback=0.87, brightness=0.78,
         filterCutoff=16000.0, bioluminescence=0.5),

    _prs("Color Web",       ["prism","trefoil","colorful","web","wide"],
         "Wide spread (0.72), brightness 0.82. The trefoil web splits into visible color bands.",
         (0.12, 0.85, 0.33, 0.30, 0.65, 0.22),
         knot=1, tangleDepth=0.42, feedback=0.86, brightness=0.82,
         filterCutoff=17500.0, spread=0.72),

    _prs("Rainbow Trinity", ["prism","trefoil","rainbow","three","bioluminescent"],
         "Three-color trefoil: bioluminescence 0.6 fires spectral bursts at each crossing.",
         (0.12, 0.78, 0.33, 0.32, 0.64, 0.25),
         knot=1, tangleDepth=0.45, feedback=0.88, brightness=0.75,
         bioluminescence=0.6, filterCutoff=16500.0),

    _prs("Prism Knot",      ["prism","trefoil","knotted","resonant","spectral"],
         "FilterRes 0.35 adds resonance to the bright spectrum. The knot becomes a resonant filter bank.",
         (0.12, 0.82, 0.32, 0.28, 0.65, 0.23),
         knot=1, tangleDepth=0.4, feedback=0.87, brightness=0.80,
         filterCutoff=17000.0, filterRes=0.35),

    # ── PRISM · FIGURE-EIGHT (4) ────────────────────────────────────────────
    _prs("Spectral Eight",  ["prism","figure-eight","spectral","wide","symmetric"],
         "Figure-eight at brightness 0.78, filterCutoff 16kHz. Symmetric spectral refraction.",
         (0.10, 0.80, 0.30, 0.27, 0.67, 0.25),
         knot=2, tangleDepth=0.35, feedback=0.88, brightness=0.78,
         filterCutoff=16000.0, spread=0.78),

    _prs("Lemniscate Prism",["prism","figure-eight","prismatic","diffuse","airy"],
         "High diffusion (0.45) spreads the lemniscate spectrum into a prismatic wash.",
         (0.10, 0.85, 0.28, 0.25, 0.70, 0.22),
         knot=2, tangleDepth=0.32, feedback=0.89, brightness=0.82,
         filterCutoff=17000.0, diffusion=0.45),

    _prs("Color Loop",      ["prism","figure-eight","colorful","bioluminescent","loop"],
         "Bioluminescence 0.55 fires color bursts as the figure-eight loops. LFO modulates the spectrum.",
         (0.12, 0.78, 0.30, 0.30, 0.66, 0.25),
         knot=2, tangleDepth=0.38, feedback=0.87, brightness=0.75,
         bioluminescence=0.55, spread=0.75),

    _prs("Mirror Prism",    ["prism","figure-eight","mirror","spectral","entrained"],
         "Entrain 0.3 partially locks the mirror, creating synchronized spectral pulses.",
         (0.12, 0.82, 0.32, 0.30, 0.65, 0.23),
         knot=2, tangleDepth=0.4, feedback=0.88, brightness=0.80,
         filterCutoff=16500.0, entrain=0.3),

    # ── PRISM · TORUS (4) ───────────────────────────────────────────────────
    _prs("Harmonic Prism",  ["prism","torus","T(3,2)","harmonic","spectral"],
         "T(3,2) fifth ratio as a prism: brightness 0.80 makes the harmonic lock visible in spectrum.",
         (0.12, 0.82, 0.33, 0.28, 0.64, 0.28),
         knot=3, torusP=3, torusQ=2, tangleDepth=0.42, feedback=0.87,
         brightness=0.80, filterCutoff=16000.0),

    _prs("Fifth Spectrum",  ["prism","torus","fifth","spectrum","bright"],
         "T(3,2) at highest brightness (0.82). The fifth harmonic creates a comb of bright spectral peaks.",
         (0.13, 0.85, 0.33, 0.30, 0.63, 0.25),
         knot=3, torusP=3, torusQ=2, tangleDepth=0.45, feedback=0.86,
         brightness=0.82, filterCutoff=17000.0),

    _prs("Torus Rainbow",   ["prism","torus","T(5,2)","rainbow","bioluminescent"],
         "T(5,2) with bioluminescence 0.55 — five-way harmonic lock with onset color bursts.",
         (0.12, 0.80, 0.32, 0.28, 0.65, 0.25),
         knot=3, torusP=5, torusQ=2, tangleDepth=0.4, feedback=0.88,
         brightness=0.78, bioluminescence=0.55),

    _prs("Toroidal Prism",  ["prism","torus","T(4,3)","spectral","rich"],
         "T(4,3) prismatic: richer harmonic ratio makes the prism more complex. filterRes 0.3 resonates.",
         (0.13, 0.80, 0.35, 0.30, 0.63, 0.26),
         knot=3, torusP=4, torusQ=3, tangleDepth=0.45, feedback=0.87,
         brightness=0.78, filterCutoff=16500.0, filterRes=0.3),

    # ── FOUNDATION (4) ──────────────────────────────────────────────────────
    _fdn("Ground State",    ["foundation","unknot","warm","grounded","root"],
         "Unknot at feedback 0.80, filterCutoff 3000 Hz, spread 0.40. The most grounded state in the library.",
         (0.18, 0.22, 0.60, 0.12, 0.22, 0.75),
         knot=0, tangleDepth=0.2, feedback=0.80, filterCutoff=3000.0,
         brightness=0.22, spread=0.40, dampening=0.72),

    _fdn("Warm Root",       ["foundation","trefoil","warm","three","root"],
         "Trefoil foundation: filterCutoff 3500, spread 0.42, high dampening. Three warm root voices.",
         (0.20, 0.25, 0.58, 0.15, 0.25, 0.72),
         knot=1, tangleDepth=0.3, feedback=0.82, filterCutoff=3500.0,
         brightness=0.25, spread=0.42, dampening=0.68),

    _fdn("Solid Eight",     ["foundation","figure-eight","solid","stable","balanced"],
         "Figure-eight foundation: symmetric, stable. filterCutoff 4000, spread 0.45.",
         (0.20, 0.28, 0.55, 0.15, 0.28, 0.68),
         knot=2, tangleDepth=0.28, feedback=0.82, filterCutoff=4000.0,
         brightness=0.28, spread=0.45, dampening=0.65),

    _fdn("Torus Base",      ["foundation","torus","T(3,2)","harmonic","root"],
         "T(3,2) foundation: fifth harmonic lock provides musical root content. filterCutoff 4500.",
         (0.22, 0.30, 0.55, 0.15, 0.28, 0.65),
         knot=3, torusP=3, torusQ=2, tangleDepth=0.35, feedback=0.83,
         filterCutoff=4500.0, brightness=0.30, spread=0.45),
]


# ═══════════════════════════════════════════════════════
#  OUTWIT HELPERS
# ═══════════════════════════════════════════════════════

_OWIT_PANS = [-0.86, -0.57, -0.29, 0.0, 0.29, 0.57, 0.86, 0.0]

_OWG = dict(
    owit_stepRate=4.0, owit_stepSync=0, owit_stepDiv=4,
    owit_synapse=0.2, owit_chromAmount=0.5,
    owit_solve=0.0, owit_huntRate=0.3,
    owit_targetBrightness=0.5, owit_targetWarmth=0.5, owit_targetMovement=0.5,
    owit_targetDensity=0.5, owit_targetSpace=0.5, owit_targetAggression=0.5,
    owit_inkCloud=0.0, owit_inkDecay=0.1, owit_triggerThresh=0.3, owit_masterLevel=0.8,
    owit_ampAttack=0.01, owit_ampDecay=0.2, owit_ampSustain=0.8, owit_ampRelease=0.3,
    owit_filterRes=0.2, owit_filterType=0,
    owit_denSize=0.4, owit_denDecay=0.4, owit_denMix=0.2,
    owit_lfo1Rate=0.5, owit_lfo1Depth=0.2, owit_lfo1Shape=0, owit_lfo1Dest=1,
    owit_lfo2Rate=0.2, owit_lfo2Depth=0.15, owit_lfo2Shape=0, owit_lfo2Dest=1,
    owit_voiceMode=0, owit_glide=0.0,
    owit_macroSolve=0.0, owit_macroSynapse=0.2, owit_macroChromatophore=0.3, owit_macroDen=0.2,
)


def owit(name, mood, tags, desc, d, rules,
         lengths=None, levels=None, pitches=None, filters=None, waves=None, pans=None,
         **g):
    if lengths is None: lengths = [16]*8
    if levels  is None: levels  = [0.7]*8
    if pitches is None: pitches = [0]*8
    if filters is None: filters = [8000.0]*8
    if waves   is None: waves   = [0]*8
    if pans    is None: pans    = list(_OWIT_PANS)

    params = {}
    for n in range(8):
        params[f"owit_arm{n}Rule"]   = rules[n]
        params[f"owit_arm{n}Length"] = lengths[n]
        params[f"owit_arm{n}Level"]  = levels[n]
        params[f"owit_arm{n}Pitch"]  = pitches[n]
        params[f"owit_arm{n}Filter"] = filters[n]
        params[f"owit_arm{n}Wave"]   = waves[n]
        params[f"owit_arm{n}Pan"]    = pans[n]

    gparams = {**_OWG, **g}
    params.update(gparams)

    return {
        "author": AUTHOR, "coupling": {"pairs": []}, "couplingIntensity": "None",
        "created": DATE, "description": desc,
        "dna": {"aggression": d[0], "brightness": d[1], "density": d[2],
                "movement": d[3], "space": d[4], "warmth": d[5]},
        "engines": ["Outwit"], "macroLabels": ["SOLVE", "SYNAPSE", "CHROMATOPHORE", "DEN"],
        "mood": mood, "name": name,
        "parameters": {"Outwit": params},
        "schema_version": 1, "sequencer": None, "tags": tags, "tempo": None, "version": "1.0.0",
    }


# ═══════════════════════════════════════════════════════
#  OUTWIT PRESETS  (50 total)
# ═══════════════════════════════════════════════════════
# Rule class reference:
#   Class 1 (stable):   0, 8, 32, 128, 136, 160, 200
#   Class 2 (periodic): 4, 24, 50, 51, 77, 78, 108, 130, 152, 184, 204
#   Class 3 (chaotic):  18, 22, 30, 45, 73, 89, 90, 105, 150
#   Class 4 (complex):  54, 106, 110, 124, 137, 193

OUTWIT_PRESETS = [

    # ── ATMOSPHERE (18) ─────────────────────────────────────────────────────
    owit("Slow Automaton", "Atmosphere",
         ["atmosphere","class-2","periodic","gentle","drone"],
         "Rule 204 (identity) on all arms — the CA copies itself each step. Slow step rate, minimal synapse. The octopus barely stirs.",
         (0.04, 0.30, 0.35, 0.08, 0.70, 0.52),
         rules=[204,204,204,204,204,204,204,204],
         owit_stepRate=1.0, owit_synapse=0.05, owit_chromAmount=0.2,
         owit_denMix=0.35, owit_denSize=0.5, owit_ampSustain=0.9, owit_ampRelease=1.0,
         owit_macroSynapse=0.1, owit_macroChromatophore=0.2, owit_macroDen=0.4),

    owit("Cellular Sea", "Atmosphere",
         ["atmosphere","class-2","periodic","sea","drift"],
         "Rule 50/51 (periodic class 2) alternating across arms. Gentle periodicity — a calm sea floor.",
         (0.06, 0.35, 0.38, 0.12, 0.65, 0.48),
         rules=[50,50,51,51,50,50,51,50],
         owit_stepRate=2.0, owit_synapse=0.10, owit_chromAmount=0.25,
         owit_denMix=0.3, owit_denSize=0.45),

    owit("Arm Wave", "Atmosphere",
         ["atmosphere","wave","class-1","class-2","motion"],
         "Alternating class 2 (rule 4) and class 1 (rule 8) arms create a traveling wave motion.",
         (0.07, 0.38, 0.38, 0.15, 0.62, 0.45),
         rules=[4,8,4,8,4,8,4,8],
         owit_stepRate=3.0, owit_synapse=0.08, owit_chromAmount=0.3,
         owit_inkCloud=0.15, owit_inkDecay=0.08),

    owit("Periodic Drift", "Atmosphere",
         ["atmosphere","class-2","periodic","drift","smooth"],
         "Rule 184 (traffic rule — class 2) on all arms. Particles drift in one direction. Smooth, continuous.",
         (0.06, 0.32, 0.38, 0.12, 0.64, 0.48),
         rules=[184,184,184,184,184,184,184,184],
         owit_stepRate=2.5, owit_synapse=0.12, owit_chromAmount=0.28,
         owit_ampAttack=0.05),

    owit("Rule Zero Wash", "Atmosphere",
         ["atmosphere","class-1","minimal","void","static"],
         "Rule 0 (all cells die) — static after first step. Low step rate, ink cloud for movement.",
         (0.02, 0.22, 0.20, 0.04, 0.75, 0.55),
         rules=[0,0,0,0,0,0,0,0],
         owit_stepRate=0.5, owit_synapse=0.02, owit_chromAmount=0.15,
         owit_inkCloud=0.30, owit_inkDecay=0.05, owit_masterLevel=0.65),

    owit("Class Two Pad", "Atmosphere",
         ["atmosphere","class-2","periodic","pad","stable"],
         "Rules 77/78 (class 2) alternating. Periodic oscillation becomes a stable ambient pad.",
         (0.06, 0.33, 0.37, 0.12, 0.63, 0.47),
         rules=[77,78,77,78,77,78,77,78],
         owit_stepRate=3.0, owit_synapse=0.10, owit_chromAmount=0.25,
         owit_denMix=0.30),

    owit("Soft Synapse", "Atmosphere",
         ["atmosphere","synapse","gentle","chromatic","evolving"],
         "Rule 50/51 with chromAmount 0.35 and gentle LFO sweep. Chromatophore shifts create color drift.",
         (0.05, 0.35, 0.36, 0.12, 0.65, 0.46),
         rules=[50,51,50,51,50,51,50,51],
         owit_stepRate=2.0, owit_synapse=0.05, owit_chromAmount=0.35,
         owit_lfo1Rate=0.3, owit_lfo1Depth=0.15, owit_lfo1Dest=2),

    owit("Octopus Dream", "Atmosphere",
         ["atmosphere","octopus","dream","symmetric","organic"],
         "Symmetric arm distribution: outer arms stable (class 1), inner arms periodic (class 2). Ink cloud creates depth.",
         (0.04, 0.30, 0.32, 0.08, 0.68, 0.52),
         rules=[200,160,136,128,128,136,160,200],
         owit_stepRate=1.5, owit_synapse=0.08, owit_chromAmount=0.20,
         owit_inkCloud=0.20, owit_inkDecay=0.08, owit_denSize=0.55),

    owit("Stable Arms", "Atmosphere",
         ["atmosphere","class-1","stable","drone","quiet"],
         "Rule 136 (class 1 stable pattern) on all arms. Very low energy — a drone from stable CA.",
         (0.04, 0.28, 0.33, 0.08, 0.68, 0.52),
         rules=[136,136,136,136,136,136,136,136],
         owit_stepRate=2.0, owit_synapse=0.06, owit_chromAmount=0.20),

    owit("Chromo Haze", "Atmosphere",
         ["atmosphere","chromatophore","haze","colorful","evolving"],
         "High chromAmount (0.55) with LFO sweep on chrom dest. Chromatophore patterns color the sound continuously.",
         (0.06, 0.40, 0.38, 0.15, 0.62, 0.42),
         rules=[51,51,50,50,51,51,50,50],
         owit_stepRate=3.0, owit_synapse=0.10, owit_chromAmount=0.55,
         owit_lfo1Dest=2, owit_lfo1Rate=0.5, owit_lfo1Depth=0.25),

    owit("Deep Synapse", "Atmosphere",
         ["atmosphere","synapse","deep","den","connected"],
         "Moderate synapse (0.25) with high den settings. Connection between arms creates depth.",
         (0.07, 0.33, 0.42, 0.12, 0.62, 0.48),
         rules=[108,108,108,108,108,108,108,108],
         owit_stepRate=2.5, owit_synapse=0.25, owit_chromAmount=0.30,
         owit_denSize=0.55, owit_denDecay=0.5, owit_denMix=0.35),

    owit("Gentle Rules", "Atmosphere",
         ["atmosphere","class-2","gentle","varied","organic"],
         "Rules 24/25 (gentle class 2) alternating. Step rate 2.0, low energy. A quiet garden of automata.",
         (0.05, 0.30, 0.35, 0.10, 0.65, 0.50),
         rules=[24,25,24,25,24,25,24,24],
         owit_stepRate=2.0, owit_synapse=0.08, owit_chromAmount=0.20,
         owit_masterLevel=0.70),

    owit("Automaton Rain", "Atmosphere",
         ["atmosphere","class-2","rain","varied","organic"],
         "Varied class 2 rules (50,51,77,78) with ink cloud. Rain-like onset pattern from rule variation.",
         (0.08, 0.38, 0.40, 0.18, 0.60, 0.44),
         rules=[50,51,77,78,51,50,78,77],
         owit_stepRate=4.0, owit_synapse=0.12, owit_chromAmount=0.30,
         owit_inkCloud=0.25),

    owit("Periodic Wave", "Atmosphere",
         ["atmosphere","class-2","wave","periodic","lfo"],
         "Rule 152 (class 2) with LFO sweeping arm levels. Creates a wave motion across all eight arms.",
         (0.07, 0.35, 0.38, 0.15, 0.62, 0.46),
         rules=[152,152,152,152,152,152,152,152],
         owit_stepRate=3.5, owit_synapse=0.10, owit_chromAmount=0.25,
         owit_lfo1Rate=0.8, owit_lfo1Depth=0.20, owit_lfo1Dest=3),

    owit("Diffuse Ink", "Atmosphere",
         ["atmosphere","ink","diffuse","class-2","organic"],
         "Rule 130 (class 2) with high inkCloud (0.45) and slow decay. Ink diffuses through the automaton.",
         (0.05, 0.32, 0.35, 0.10, 0.65, 0.48),
         rules=[130,130,130,130,130,130,130,130],
         owit_stepRate=2.0, owit_synapse=0.08, owit_chromAmount=0.20,
         owit_inkCloud=0.45, owit_inkDecay=0.12),

    owit("Arm Chorus", "Atmosphere",
         ["atmosphere","chorus","pitched","harmonic","spread"],
         "Rule 4 with pitched arms (0,2,4,7 semitone intervals). Chromatophore tuning creates a chord.",
         (0.07, 0.40, 0.38, 0.15, 0.62, 0.44),
         rules=[4,4,4,4,4,4,4,4],
         pitches=[0,2,4,7,0,2,4,7],
         owit_stepRate=2.5, owit_synapse=0.10, owit_chromAmount=0.40,
         owit_lfo1Rate=0.5, owit_lfo1Depth=0.25),

    owit("Rule Garden", "Atmosphere",
         ["atmosphere","varied","class-2","garden","organic"],
         "One class 2 rule per arm (4,8,24,50,77,108,152,184). Eight different automata sharing a substrate.",
         (0.08, 0.38, 0.42, 0.18, 0.60, 0.44),
         rules=[4,8,24,50,77,108,152,184],
         owit_stepRate=3.0, owit_synapse=0.12, owit_chromAmount=0.32),

    owit("Silent Evolution", "Atmosphere",
         ["atmosphere","class-1","silent","minimal","slow"],
         "All class 1/2 stable rules at low master level (0.60). The octopus evolves in near-silence.",
         (0.03, 0.25, 0.28, 0.06, 0.72, 0.52),
         rules=[160,136,200,136,160,200,136,160],
         owit_stepRate=0.8, owit_synapse=0.04, owit_chromAmount=0.15,
         owit_masterLevel=0.60, owit_inkCloud=0.15),

    # ── ENTANGLED (15) ──────────────────────────────────────────────────────
    owit("CA Sequence", "Entangled",
         ["entangled","class-4","class-3","sequence","rhythmic"],
         "Mix of class 4 (110), class 3 (30), and class 2 (50) cycling across three arms. 8 Hz step rate.",
         (0.28, 0.48, 0.52, 0.55, 0.42, 0.35),
         rules=[110,30,50,110,30,50,110,30],
         owit_stepRate=8.0, owit_synapse=0.25, owit_chromAmount=0.45),

    owit("Synapse Gate", "Entangled",
         ["entangled","class-3","synapse","gate","rule-90"],
         "Rule 90 (XOR class 3) on all arms with medium-high synapse (0.35). Arms talk to each other in chaos.",
         (0.32, 0.50, 0.50, 0.60, 0.40, 0.32),
         rules=[90,90,90,90,90,90,90,90],
         owit_stepRate=6.0, owit_synapse=0.35, owit_chromAmount=0.40,
         owit_filterType=1),

    owit("Rule 110 Engine", "Entangled",
         ["entangled","class-4","turing","complex","rule-110"],
         "Rule 110 — Turing complete cellular automaton. Universal computation in 8 parallel arms at 10 Hz.",
         (0.35, 0.52, 0.52, 0.65, 0.38, 0.32),
         rules=[110,110,110,110,110,110,110,110],
         owit_stepRate=10.0, owit_synapse=0.28, owit_chromAmount=0.42),

    owit("Class Four Rhythm", "Entangled",
         ["entangled","class-4","rhythm","complex","alternating"],
         "Alternating class 4 rules (54/110). Complex localized structures create irregular rhythm.",
         (0.33, 0.52, 0.52, 0.62, 0.38, 0.32),
         rules=[54,110,54,110,54,110,54,110],
         owit_stepRate=8.0, owit_synapse=0.30, owit_chromAmount=0.45,
         owit_stepDiv=5),

    owit("Wolfram Arp", "Entangled",
         ["entangled","class-4","class-3","arp","pitched"],
         "Cycling class 4/3/3 rules with pitched arms (0,4,7,12 semitones). CA rules become an arpeggio.",
         (0.30, 0.55, 0.50, 0.68, 0.38, 0.35),
         rules=[110,30,90,110,30,90,110,30],
         pitches=[0,4,7,12,0,4,7,12],
         owit_stepRate=12.0, owit_synapse=0.20, owit_chromAmount=0.50,
         owit_ampAttack=0.01, owit_ampDecay=0.15, owit_ampSustain=0.50),

    owit("Eight Arm Groove", "Entangled",
         ["entangled","groove","varied","class-mix","organic"],
         "All different rule classes across 8 arms (110,30,45,90,110,30,45,90). Maximum variety in a groove.",
         (0.35, 0.52, 0.55, 0.62, 0.38, 0.32),
         rules=[110,30,45,90,110,30,45,90],
         owit_stepRate=8.0, owit_synapse=0.32, owit_chromAmount=0.45),

    owit("Octopus Pulse", "Entangled",
         ["entangled","class-3","pulse","ink","organic"],
         "Rule 90/45 alternating with inkCloud (0.25). Eight arms pulse chaotically with visible ink traces.",
         (0.30, 0.50, 0.50, 0.58, 0.40, 0.33),
         rules=[90,45,90,45,90,45,90,45],
         owit_stepRate=6.0, owit_synapse=0.28, owit_chromAmount=0.40,
         owit_inkCloud=0.25),

    owit("Rule Trigger", "Entangled",
         ["entangled","class-2","trigger","fast","rhythmic"],
         "Rule 184 at very fast step rate (16 Hz) with trigger threshold 0.5. Rhythmic CA trigger machine.",
         (0.25, 0.45, 0.48, 0.58, 0.42, 0.35),
         rules=[184,184,184,184,184,184,184,184],
         owit_stepRate=16.0, owit_synapse=0.20, owit_chromAmount=0.35,
         owit_triggerThresh=0.50),

    owit("Cellular Beat", "Entangled",
         ["entangled","class-3","class-2","beat","alternating"],
         "Rule 30 (class 3) and 50 (class 2) alternating at 10 Hz. Chaos and order trading the downbeat.",
         (0.30, 0.50, 0.50, 0.62, 0.40, 0.33),
         rules=[30,50,30,50,30,50,30,50],
         owit_stepRate=10.0, owit_synapse=0.25, owit_chromAmount=0.42,
         owit_stepDiv=6),

    owit("Arm Weave", "Entangled",
         ["entangled","woven","class-mix","lfo","motion"],
         "Four different rules weaving: 110,45,184,30. LFO sweeps step rate for organic rhythm variation.",
         (0.33, 0.52, 0.52, 0.62, 0.38, 0.32),
         rules=[110,45,184,30,110,45,184,30],
         owit_stepRate=8.0, owit_synapse=0.30, owit_chromAmount=0.45,
         owit_lfo1Rate=2.0, owit_lfo1Dest=0, owit_lfo1Depth=0.30),

    owit("Synapse Lock", "Entangled",
         ["entangled","synapse","locked","class-4","connected"],
         "Rule 110 with high synapse (0.55). Arms strongly influence each other — complex locked behavior.",
         (0.32, 0.50, 0.55, 0.60, 0.38, 0.32),
         rules=[110,110,110,110,110,110,110,110],
         owit_stepRate=8.0, owit_synapse=0.55, owit_chromAmount=0.40,
         owit_lfo2Rate=1.0, owit_lfo2Dest=1),

    owit("CA Matrix", "Entangled",
         ["entangled","class-3","class-4","matrix","complex"],
         "Class 3/4 matrix: rules 90,106,110,124 cycling. Complex interactions at every step.",
         (0.33, 0.52, 0.52, 0.60, 0.40, 0.32),
         rules=[90,106,110,124,90,106,110,124],
         owit_stepRate=6.0, owit_synapse=0.28, owit_chromAmount=0.42),

    owit("Wolfram Step", "Entangled",
         ["entangled","class-3","stepped","sync","rhythmic"],
         "Rule 45/90 alternating, step-synced to host tempo. Chaotic CA locked to the grid.",
         (0.28, 0.50, 0.50, 0.62, 0.40, 0.35),
         rules=[45,90,45,90,45,90,45,90],
         owit_stepRate=10.0, owit_synapse=0.22, owit_chromAmount=0.38,
         owit_stepSync=1, owit_stepDiv=5),

    owit("Coupled Rules", "Entangled",
         ["entangled","coupled","class-4","class-3","lfo"],
         "Rule 110/30 paired with LFO sweeping arm levels. Coupling turns the class 4/3 mix musical.",
         (0.35, 0.52, 0.52, 0.65, 0.38, 0.32),
         rules=[110,30,110,30,110,30,110,30],
         owit_stepRate=8.0, owit_synapse=0.40, owit_chromAmount=0.45,
         owit_lfo1Rate=1.5, owit_lfo1Dest=3),

    owit("Arm Entangle", "Entangled",
         ["entangled","class-3","entangled","ink","organic"],
         "Rule 90 paired in groups (90,90,45,45) with inkCloud (0.2). Arms entangled in pairs.",
         (0.32, 0.50, 0.52, 0.60, 0.40, 0.33),
         rules=[90,90,45,45,90,90,45,45],
         owit_stepRate=6.0, owit_synapse=0.35, owit_chromAmount=0.48,
         owit_inkCloud=0.20, owit_denMix=0.30),

    # ── FLUX (10) ───────────────────────────────────────────────────────────
    owit("Rule 30 Chaos", "Flux",
         ["flux","class-3","rule-30","chaos","extreme"],
         "Rule 30 on all arms — the canonical chaotic elementary CA. 20 Hz step, high synapse. Maximum entropy.",
         (0.72, 0.65, 0.52, 0.82, 0.30, 0.22),
         rules=[30,30,30,30,30,30,30,30],
         owit_stepRate=20.0, owit_synapse=0.55, owit_chromAmount=0.65),

    owit("Wolfram Storm", "Flux",
         ["flux","class-3","storm","varied","chaotic"],
         "Four different class 3 rules (30,22,90,150) cycling. Maximum rule diversity in a chaos storm.",
         (0.78, 0.70, 0.52, 0.85, 0.28, 0.20),
         rules=[30,22,90,150,30,22,90,150],
         owit_stepRate=16.0, owit_synapse=0.60, owit_chromAmount=0.70),

    owit("CA Dissolution", "Flux",
         ["flux","class-3","rule-150","symmetric","dissolving"],
         "Rule 150 (symmetric XOR class 3) with inkCloud 0.4. The pattern dissolves into ink diffusion.",
         (0.75, 0.68, 0.50, 0.82, 0.30, 0.22),
         rules=[150,150,150,150,150,150,150,150],
         owit_stepRate=18.0, owit_synapse=0.50, owit_chromAmount=0.60,
         owit_inkCloud=0.40, owit_inkDecay=0.30),

    owit("Class Three Break", "Flux",
         ["flux","class-3","chaotic","varied","breaking"],
         "All four class 3 rules (18,22,30,45) on successive arms. The class 3 spectrum fully represented.",
         (0.78, 0.70, 0.52, 0.85, 0.28, 0.20),
         rules=[18,22,30,45,18,22,30,45],
         owit_stepRate=20.0, owit_synapse=0.58, owit_chromAmount=0.65),

    owit("Synapse Surge", "Flux",
         ["flux","synapse","class-4","extreme","surge"],
         "Rule 110 (class 4) with extreme synapse (0.80). Arms strongly disturb each other — cascade chaos.",
         (0.80, 0.72, 0.55, 0.85, 0.27, 0.20),
         rules=[110,110,110,110,110,110,110,110],
         owit_stepRate=16.0, owit_synapse=0.80, owit_chromAmount=0.58,
         owit_filterRes=0.55),

    owit("Arm Collapse", "Flux",
         ["flux","class-3","collapse","high-level","extreme"],
         "Class 3 rules (90,90,22,22) with high arm levels (0.9). The collapse of all eight arms simultaneously.",
         (0.82, 0.72, 0.52, 0.88, 0.27, 0.18),
         rules=[90,90,22,22,90,90,22,22],
         levels=[0.9,0.9,0.85,0.85,0.9,0.9,0.85,0.85],
         owit_stepRate=20.0, owit_synapse=0.65, owit_chromAmount=0.72),

    owit("Rule Cascade", "Flux",
         ["flux","class-3","cascade","varied","chaos"],
         "Cascade from simple (rule 30) to complex (rule 150): each arm one step further into chaos.",
         (0.78, 0.70, 0.52, 0.85, 0.28, 0.20),
         rules=[30,45,90,150,30,45,90,150],
         owit_stepRate=18.0, owit_synapse=0.55, owit_chromAmount=0.68),

    owit("Cellular Chaos", "Flux",
         ["flux","class-3","rule-22","extreme","noise"],
         "Rule 22 (class 3) on all arms at 24 Hz. The fastest pure chaos in the OUTWIT library.",
         (0.85, 0.75, 0.50, 0.90, 0.25, 0.18),
         rules=[22,22,22,22,22,22,22,22],
         owit_stepRate=24.0, owit_synapse=0.62, owit_chromAmount=0.75,
         owit_ampAttack=0.001, owit_ampDecay=0.10, owit_ampSustain=0.40),

    owit("Octopus Frenzy", "Flux",
         ["flux","class-3","frenzy","ink","chaotic"],
         "Four class 3 rules (30,90,150,22) cycling with inkCloud 0.35. The octopus in full frenzy.",
         (0.82, 0.72, 0.52, 0.88, 0.27, 0.18),
         rules=[30,90,150,22,30,90,150,22],
         owit_stepRate=20.0, owit_synapse=0.70, owit_chromAmount=0.70,
         owit_inkCloud=0.35, owit_triggerThresh=0.20),

    owit("Solve Storm", "Flux",
         ["flux","solve","ga","hunting","extreme"],
         "SOLVE macro active (0.8): GA hunting for chaos targets while class 3/4 rules rage at 16 Hz.",
         (0.82, 0.75, 0.52, 0.88, 0.27, 0.18),
         rules=[110,30,90,150,110,30,90,150],
         owit_stepRate=16.0, owit_synapse=0.60, owit_chromAmount=0.65,
         owit_solve=0.80, owit_huntRate=0.70,
         owit_targetAggression=0.90, owit_targetMovement=0.90, owit_targetBrightness=0.80,
         owit_macroSolve=0.8),

    # ── AETHER (7) ──────────────────────────────────────────────────────────
    owit("Rule Zero Space", "Aether",
         ["aether","rule-0","minimal","void","sparse"],
         "Rule 0 (all cells die) at lowest step rate (0.3). The automaton reaches silence immediately.",
         (0.02, 0.15, 0.12, 0.03, 0.82, 0.48),
         rules=[0,0,0,0,0,0,0,0],
         levels=[0.4,0.3,0.3,0.25,0.25,0.3,0.3,0.4],
         owit_stepRate=0.3, owit_synapse=0.02, owit_chromAmount=0.10,
         owit_masterLevel=0.50),

    owit("Dead Cell Haze", "Aether",
         ["aether","class-1","sparse","ink","minimal"],
         "Alternating rule 0 and rule 8 (minimal class 1) with slow ink cloud. Near-dead arms, ink traces.",
         (0.02, 0.18, 0.14, 0.04, 0.80, 0.45),
         rules=[0,8,0,8,0,8,0,8],
         owit_stepRate=0.5, owit_synapse=0.03, owit_chromAmount=0.12,
         owit_inkCloud=0.20, owit_inkDecay=0.05),

    owit("Sparse CA", "Aether",
         ["aether","class-1","sparse","alternating","minimal"],
         "Rule 136 (stable) alternating with rule 0 (dead). Half the arms are silent — extreme sparsity.",
         (0.02, 0.20, 0.15, 0.04, 0.78, 0.46),
         rules=[136,0,136,0,136,0,136,0],
         levels=[0.5,0.2,0.5,0.2,0.5,0.2,0.5,0.2],
         owit_stepRate=0.8, owit_synapse=0.04, owit_chromAmount=0.15),

    owit("Minimal Arms", "Aether",
         ["aether","class-1","minimal","quiet","static"],
         "Rule 8 (class 1 minimal) at 0.3 level per arm. The quietest operational OUTWIT preset.",
         (0.01, 0.15, 0.12, 0.02, 0.82, 0.50),
         rules=[8,8,8,8,8,8,8,8],
         levels=[0.3,0.3,0.3,0.3,0.3,0.3,0.3,0.3],
         owit_stepRate=0.5, owit_synapse=0.02, owit_chromAmount=0.10,
         owit_masterLevel=0.45),

    owit("Ghost Rule", "Aether",
         ["aether","class-1","ghost","stable","ink"],
         "Rule 160 (class 1 stable) with gentle ink cloud release. A ghost haunting a stable automaton.",
         (0.02, 0.18, 0.14, 0.03, 0.80, 0.48),
         rules=[160,160,160,160,160,160,160,160],
         owit_stepRate=0.8, owit_synapse=0.03, owit_chromAmount=0.12,
         owit_inkCloud=0.15, owit_ampRelease=2.0),

    owit("Empty Automaton", "Aether",
         ["aether","class-2","identity","void","drone"],
         "Rule 200 (copy rule — identity) at very low master level (0.40). The automaton copies silence.",
         (0.01, 0.15, 0.12, 0.02, 0.85, 0.50),
         rules=[200,200,200,200,200,200,200,200],
         owit_stepRate=0.5, owit_synapse=0.02, owit_chromAmount=0.10,
         owit_masterLevel=0.40),

    owit("Void Pattern", "Aether",
         ["aether","rule-0","void","absolute","minimal"],
         "Rule 0 at absolute minimum. The pattern generator at zero energy — just the faint hiss of potential.",
         (0.01, 0.12, 0.10, 0.02, 0.88, 0.48),
         rules=[0,0,0,0,0,0,0,0],
         levels=[0.25,0.2,0.2,0.15,0.15,0.2,0.2,0.25],
         owit_stepRate=0.3, owit_synapse=0.01, owit_chromAmount=0.08,
         owit_masterLevel=0.35, owit_inkCloud=0.10),
]


# ═══════════════════════════════════════════════════════
#  MAIN
# ═══════════════════════════════════════════════════════

def main():
    print(f"\n{'='*60}")
    print(f"  XOmnibus Preset Generator {'(DRY RUN) ' if DRY else ''}")
    print(f"  OVERLAP: {len(OVERLAP_PRESETS)} presets")
    print(f"  OUTWIT:  {len(OUTWIT_PRESETS)} presets")
    print(f"{'='*60}\n")

    print("── OVERLAP ──────────────────────────────────────────────")
    for p in OVERLAP_PRESETS:
        write_preset(p, "Overlap")

    print("\n── OUTWIT ───────────────────────────────────────────────")
    for p in OUTWIT_PRESETS:
        write_preset(p, "Outwit")

    print(f"\n{'='*60}")
    print(f"  Written: {_written}  |  Skipped (exists): {_skipped}")
    print(f"{'='*60}\n")

    # Mood summary
    from collections import Counter
    o_moods = Counter(p["mood"] for p in OVERLAP_PRESETS)
    w_moods = Counter(p["mood"] for p in OUTWIT_PRESETS)
    print("OVERLAP mood distribution (this batch):")
    for m, n in sorted(o_moods.items()): print(f"  {m:12s}: {n}")
    print("\nOUTWIT mood distribution (this batch):")
    for m, n in sorted(w_moods.items()): print(f"  {m:12s}: {n}")


if __name__ == "__main__":
    main()
