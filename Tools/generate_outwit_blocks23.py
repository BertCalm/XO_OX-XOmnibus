#!/usr/bin/env python3
"""
OUTWIT Preset Generator — Blocks 2 & 3 (100 presets)
Adds: Atmosphere+22, Entangled+20, Flux+15, Aether+11, Prism×20, Foundation×12

Usage:
  python3 generate_outwit_blocks23.py          # write files
  python3 generate_outwit_blocks23.py --dry-run
"""

import json, sys
from pathlib import Path

BASE   = Path(__file__).parent.parent / "Presets" / "XOlokun"
AUTHOR = "XO_OX Designs"
DATE   = "2026-03-18"
DRY    = "--dry-run" in sys.argv
_w = _s = 0


def write_preset(data):
    global _w, _s
    p = BASE / data["mood"] / "Outwit" / f"{data['name']}.xometa"
    p.parent.mkdir(parents=True, exist_ok=True)
    if p.exists():
        print(f"  SKIP  {data['mood']}/Outwit/{data['name']}")
        _s += 1; return
    if not DRY:
        p.write_text(json.dumps(data, indent=2))
    print(f"  WRITE {data['mood']}/Outwit/{data['name']}")
    _w += 1


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
         lengths=None, levels=None, pitches=None, filters=None, waves=None, pans=None, **g):
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

    params.update({**_OWG, **g})

    return {
        "author": AUTHOR, "coupling": {"pairs": []}, "couplingIntensity": "None",
        "created": DATE, "description": desc,
        "dna": {"aggression": d[0], "brightness": d[1], "density": d[2],
                "movement": d[3], "space": d[4], "warmth": d[5]},
        "engines": ["Outwit"], "macroLabels": ["SOLVE", "SYNAPSE", "CHROMATOPHORE", "DEN"],
        "mood": mood, "name": name, "parameters": {"Outwit": params},
        "schema_version": 1, "sequencer": None, "tags": tags, "tempo": None, "version": "1.0.0",
    }


# Rule class reference:
#   Class 1 (stable):  0,8,32,40,128,136,160,200
#   Class 2 (periodic):4,5,24,25,50,51,52,72,73,77,78,108,130,132,152,156,184,204
#   Class 3 (chaotic): 18,22,30,45,73,89,90,105,150
#   Class 4 (complex): 54,106,110,124,137,193

PRESETS = [

    # ═══════════════════════════════════════════════════════
    #  ATMOSPHERE  (+22)
    # ═══════════════════════════════════════════════════════
    owit("Living Tide", "Atmosphere",
         ["atmosphere","class-2","tide","periodic","flowing"],
         "Rules 156/152 (class 2) alternating. The pattern travels in one direction like a tidal current.",
         (0.06, 0.33, 0.37, 0.13, 0.63, 0.47),
         rules=[156,152,156,152,156,152,156,152],
         owit_stepRate=2.5, owit_synapse=0.10, owit_chromAmount=0.28),

    owit("Pulse Garden", "Atmosphere",
         ["atmosphere","class-2","garden","ink","organic"],
         "Rule 108 (deep class 2) with gentle ink cloud. A garden of cellular pulses, each leaving ink traces.",
         (0.07, 0.35, 0.38, 0.14, 0.62, 0.46),
         rules=[108,108,108,108,108,108,108,108],
         owit_stepRate=3.5, owit_synapse=0.12, owit_chromAmount=0.25,
         owit_inkCloud=0.20),

    owit("Harmonic Shore", "Atmosphere",
         ["atmosphere","class-2","harmonic","pitched","fifth"],
         "Rule 50 with arms tuned to fifth intervals (0,7,0,7 semitones). Periodic CA as harmonic pad.",
         (0.06, 0.38, 0.36, 0.12, 0.64, 0.46),
         rules=[50,50,50,50,50,50,50,50],
         pitches=[0,7,0,7,0,7,0,7],
         owit_stepRate=2.0, owit_synapse=0.08, owit_chromAmount=0.22),

    owit("Tidal Breath", "Atmosphere",
         ["atmosphere","class-2","breath","tidal","slow"],
         "Rule 184 (traffic rule) with long amp release (0.8s). Each step is a slow tidal breath.",
         (0.06, 0.32, 0.36, 0.11, 0.64, 0.48),
         rules=[184,184,184,184,184,184,184,184],
         owit_stepRate=1.5, owit_synapse=0.10, owit_chromAmount=0.22,
         owit_ampAttack=0.10, owit_ampRelease=0.80),

    owit("Pattern Mist", "Atmosphere",
         ["atmosphere","class-2","mist","organic","ink"],
         "Rules 152/130 alternating with ink cloud (0.22). The periodic pattern dissolves into mist.",
         (0.06, 0.33, 0.37, 0.12, 0.63, 0.47),
         rules=[152,130,152,130,152,130,152,130],
         owit_stepRate=2.0, owit_synapse=0.08, owit_chromAmount=0.20,
         owit_inkCloud=0.22),

    owit("Sea Memory", "Atmosphere",
         ["atmosphere","class-2","memory","periodic","warm"],
         "Rules 77/78 (class 2 complementary pair) cycling across arms. The sea remembers its pattern.",
         (0.06, 0.33, 0.37, 0.12, 0.63, 0.47),
         rules=[77,78,78,77,77,78,78,77],
         owit_stepRate=2.5, owit_synapse=0.10, owit_chromAmount=0.25),

    owit("Gentle Wave", "Atmosphere",
         ["atmosphere","class-2","wave","lfo","arm-levels"],
         "Rule 4 (minimal class 2) with LFO sweeping arm levels — creates a visible wave across all arms.",
         (0.06, 0.35, 0.35, 0.13, 0.63, 0.46),
         rules=[4,4,4,4,4,4,4,4],
         owit_stepRate=1.5, owit_synapse=0.06, owit_chromAmount=0.18,
         owit_lfo1Rate=0.4, owit_lfo1Dest=3, owit_lfo1Depth=0.30),

    owit("Shore Pattern", "Atmosphere",
         ["atmosphere","class-2","shore","stable","gentle"],
         "Rule 25 (class 2) — a slightly irregular periodic rule. Creates a shoreline of recurring patterns.",
         (0.05, 0.30, 0.35, 0.10, 0.65, 0.48),
         rules=[25,25,25,25,25,25,25,25],
         owit_stepRate=3.0, owit_synapse=0.10, owit_chromAmount=0.22),

    owit("Class Two Rain", "Atmosphere",
         ["atmosphere","class-2","rain","ink","varied"],
         "Rules 51/77 alternating with inkCloud (0.20). Class 2 periodicity falling like soft rain.",
         (0.07, 0.37, 0.38, 0.14, 0.62, 0.45),
         rules=[51,77,51,77,51,77,51,77],
         owit_stepRate=4.0, owit_synapse=0.12, owit_chromAmount=0.28,
         owit_inkCloud=0.20),

    owit("Periodic Shore", "Atmosphere",
         ["atmosphere","class-2","periodic","warm","low-filter"],
         "Rule 130 (class 2) with warm filterCutoff (6000 Hz). The periodic pattern is muffled by sand.",
         (0.06, 0.28, 0.37, 0.11, 0.64, 0.50),
         rules=[130,130,130,130,130,130,130,130],
         filters=[6000.0]*8,
         owit_stepRate=2.5, owit_synapse=0.10, owit_chromAmount=0.20),

    owit("Garden Arms", "Atmosphere",
         ["atmosphere","class-2","garden","varied","organic"],
         "Varied class 2 rules across arms (4,8,50,51,77,108,152,204). Each arm a different plant.",
         (0.07, 0.35, 0.40, 0.14, 0.62, 0.46),
         rules=[4,8,50,51,77,108,152,204],
         owit_stepRate=2.0, owit_synapse=0.08, owit_chromAmount=0.22),

    owit("Quiet Shore", "Atmosphere",
         ["atmosphere","class-1","quiet","minimal","calm"],
         "Rule 40 (class 1 stable) at lower master level. A very quiet shore with barely any movement.",
         (0.04, 0.25, 0.32, 0.08, 0.68, 0.52),
         rules=[40,40,40,40,40,40,40,40],
         owit_stepRate=1.0, owit_synapse=0.04, owit_chromAmount=0.12,
         owit_masterLevel=0.65),

    owit("Deep Current", "Atmosphere",
         ["atmosphere","class-2","deep","den","current"],
         "Rules 108/50 alternating with den reverb (0.35). The current runs through a resonant chamber.",
         (0.07, 0.33, 0.42, 0.13, 0.62, 0.48),
         rules=[108,50,108,50,108,50,108,50],
         owit_stepRate=2.5, owit_synapse=0.14, owit_chromAmount=0.28,
         owit_denMix=0.35, owit_denSize=0.55),

    owit("Slow Grid", "Atmosphere",
         ["atmosphere","class-2","slow","grid","drone"],
         "Rules 200/184 (identity and traffic) at 0.8 Hz. A barely-moving grid — nearly a drone.",
         (0.05, 0.28, 0.33, 0.08, 0.66, 0.50),
         rules=[200,184,200,184,200,184,200,184],
         owit_stepRate=0.8, owit_synapse=0.05, owit_chromAmount=0.15),

    owit("Rule Haze", "Atmosphere",
         ["atmosphere","class-2","haze","ink","smooth"],
         "Rule 52 (class 2) with inkCloud 0.25 — the periodic pattern is rendered as a haze.",
         (0.06, 0.33, 0.36, 0.12, 0.63, 0.47),
         rules=[52,52,52,52,52,52,52,52],
         owit_stepRate=2.0, owit_synapse=0.08, owit_chromAmount=0.20,
         owit_inkCloud=0.25),

    owit("Silent Wave", "Atmosphere",
         ["atmosphere","class-2","silent","quiet","low"],
         "Rule 132 (class 2) at reduced master level (0.60). The wave is present but barely heard.",
         (0.04, 0.27, 0.30, 0.08, 0.67, 0.52),
         rules=[132,132,132,132,132,132,132,132],
         owit_stepRate=1.0, owit_synapse=0.05, owit_chromAmount=0.15,
         owit_masterLevel=0.60),

    owit("Ink Shore", "Atmosphere",
         ["atmosphere","class-2","ink","shore","diffusion"],
         "Rule 24 with high inkCloud (0.35). The ink from each step diffuses into the next, shoreline-like.",
         (0.06, 0.33, 0.36, 0.12, 0.63, 0.47),
         rules=[24,24,24,24,24,24,24,24],
         owit_stepRate=2.0, owit_synapse=0.08, owit_chromAmount=0.20,
         owit_inkCloud=0.35, owit_inkDecay=0.10),

    owit("Dormant Garden", "Atmosphere",
         ["atmosphere","class-1","dormant","stable","quiet"],
         "Rules 160/128 (stable class 1) alternating at slow rate. The garden is dormant — alive but still.",
         (0.04, 0.25, 0.30, 0.07, 0.68, 0.52),
         rules=[160,128,160,128,160,128,160,128],
         owit_stepRate=0.8, owit_synapse=0.04, owit_chromAmount=0.12),

    owit("Wave Pattern", "Atmosphere",
         ["atmosphere","class-2","wave","periodic","smooth"],
         "Rule 156 on all arms — a smooth traveling-wave class 2 rule. Pure periodic propagation.",
         (0.06, 0.33, 0.37, 0.12, 0.63, 0.47),
         rules=[156,156,156,156,156,156,156,156],
         owit_stepRate=3.0, owit_synapse=0.10, owit_chromAmount=0.25),

    owit("Arm Whisper", "Atmosphere",
         ["atmosphere","class-1","class-2","whisper","low-level"],
         "Rules 8/32 alternating at low arm levels (0.4). Eight whispered voices barely above silence.",
         (0.04, 0.25, 0.30, 0.07, 0.68, 0.52),
         rules=[8,32,8,32,8,32,8,32],
         levels=[0.4]*8,
         owit_stepRate=1.5, owit_synapse=0.05, owit_chromAmount=0.15),

    owit("Ocean Breath", "Atmosphere",
         ["atmosphere","class-2","breath","organic","smooth"],
         "Rules 50/51 with longer amp release (0.9s). Each step breathes in, holds, releases slowly.",
         (0.06, 0.33, 0.37, 0.12, 0.64, 0.47),
         rules=[50,51,50,51,50,51,50,51],
         owit_stepRate=1.8, owit_synapse=0.09, owit_chromAmount=0.22,
         owit_ampAttack=0.08, owit_ampRelease=0.90),

    owit("Rule Dream", "Atmosphere",
         ["atmosphere","class-2","dream","slow","hazy"],
         "Rules 204/152 (identity and class 2) at 1.0 Hz with ink cloud. The automaton dreams.",
         (0.05, 0.28, 0.33, 0.08, 0.66, 0.50),
         rules=[204,152,204,152,204,152,204,152],
         owit_stepRate=1.0, owit_synapse=0.06, owit_chromAmount=0.18,
         owit_inkCloud=0.15),

    # ═══════════════════════════════════════════════════════
    #  ENTANGLED  (+20)
    # ═══════════════════════════════════════════════════════
    owit("Wolfram Grid", "Entangled",
         ["entangled","class-4","class-4","grid","complex"],
         "Rules 110/54 alternating — two class 4 rules creating complex interference patterns.",
         (0.33, 0.52, 0.52, 0.62, 0.38, 0.32),
         rules=[110,54,110,54,110,54,110,54],
         owit_stepRate=10.0, owit_synapse=0.30, owit_chromAmount=0.48),

    owit("Cross Synapse", "Entangled",
         ["entangled","class-3","class-4","synapse","crossing"],
         "Rules 90 (class 3) and 110 (class 4) with synapse 0.40. Classes cross-fertilize through synapse.",
         (0.35, 0.52, 0.50, 0.63, 0.38, 0.32),
         rules=[90,110,90,110,90,110,90,110],
         owit_stepRate=8.0, owit_synapse=0.40, owit_chromAmount=0.45),

    owit("Pattern Engine", "Entangled",
         ["entangled","class-4","engine","complex","varied"],
         "Class 4 rules 54/106 alternating — two different complex engines competing in parallel.",
         (0.33, 0.52, 0.52, 0.62, 0.38, 0.32),
         rules=[54,106,54,106,54,106,54,106],
         owit_stepRate=8.0, owit_synapse=0.32, owit_chromAmount=0.45),

    owit("Solve Rhythm", "Entangled",
         ["entangled","solve","ga","rhythm","class-4"],
         "Rule 110 with SOLVE (0.50) hunting for high-movement targets. GA creates rhythmic variation.",
         (0.32, 0.52, 0.50, 0.65, 0.38, 0.32),
         rules=[110,110,110,110,110,110,110,110],
         owit_stepRate=8.0, owit_synapse=0.25, owit_chromAmount=0.42,
         owit_solve=0.50, owit_huntRate=0.40, owit_targetMovement=0.70,
         owit_macroSolve=0.5),

    owit("Synapse Web", "Entangled",
         ["entangled","class-3","synapse","web","bandpass"],
         "Rule 90 with high synapse (0.50) and bandpass filter. Arms form a webbed network.",
         (0.33, 0.52, 0.52, 0.60, 0.38, 0.32),
         rules=[90,90,90,90,90,90,90,90],
         owit_stepRate=6.0, owit_synapse=0.50, owit_chromAmount=0.42,
         owit_filterType=1),

    owit("Arm Dance", "Entangled",
         ["entangled","class-4","class-3","dance","lfo-levels"],
         "Rules 110/30 with LFO sweeping arm levels — arms rise and fall in a chaotic dance.",
         (0.32, 0.52, 0.50, 0.65, 0.38, 0.32),
         rules=[110,30,110,30,110,30,110,30],
         owit_stepRate=10.0, owit_synapse=0.28, owit_chromAmount=0.48,
         owit_lfo1Rate=2.0, owit_lfo1Dest=3, owit_lfo1Depth=0.35),

    owit("CA Beat II", "Entangled",
         ["entangled","class-3","class-4","beat","varied"],
         "Four-class mix (30,50,90,110) cycling — class 3/2/3/4 creates irregular but groovy rhythm.",
         (0.30, 0.50, 0.50, 0.62, 0.40, 0.33),
         rules=[30,50,90,110,30,50,90,110],
         owit_stepRate=8.0, owit_synapse=0.32, owit_chromAmount=0.45),

    owit("Wolfram Step II", "Entangled",
         ["entangled","class-4","class-4","stepped","complex"],
         "Rules 124/110 (two class 4 variants) at 10 Hz. Complex localized structures at every step.",
         (0.33, 0.52, 0.52, 0.63, 0.38, 0.32),
         rules=[124,110,124,110,124,110,124,110],
         owit_stepRate=10.0, owit_synapse=0.28, owit_chromAmount=0.45),

    owit("Eight Groove", "Entangled",
         ["entangled","class-4","class-3","groove","complex"],
         "Four distinct rules cycling (54,30,110,45,54,30,110,45). Maximum timbral variety in a groove.",
         (0.33, 0.52, 0.53, 0.62, 0.38, 0.32),
         rules=[54,30,110,45,54,30,110,45],
         owit_stepRate=8.0, owit_synapse=0.35, owit_chromAmount=0.48),

    owit("Rule Loop", "Entangled",
         ["entangled","class-2","class-4","loop","alternating"],
         "Rule 50 (class 2) and 110 (class 4) alternating. Order and complexity trading turns.",
         (0.28, 0.50, 0.50, 0.60, 0.40, 0.35),
         rules=[50,110,50,110,50,110,50,110],
         owit_stepRate=6.0, owit_synapse=0.28, owit_chromAmount=0.42),

    owit("Wolfram Dance", "Entangled",
         ["entangled","class-4","dance","complex","organic"],
         "Rules 106/110 alternating — two similar class 4 rules creates subtle interference dance.",
         (0.33, 0.52, 0.52, 0.63, 0.38, 0.32),
         rules=[106,110,106,110,106,110,106,110],
         owit_stepRate=10.0, owit_synapse=0.30, owit_chromAmount=0.45),

    owit("CA Flow", "Entangled",
         ["entangled","class-3","flow","alternating","chaotic"],
         "Rules 90/150 alternating — XOR and XOR-symmetric class 3 rules flowing together.",
         (0.32, 0.50, 0.50, 0.62, 0.40, 0.32),
         rules=[90,150,90,150,90,150,90,150],
         owit_stepRate=8.0, owit_synapse=0.32, owit_chromAmount=0.45),

    owit("Synapse Beat", "Entangled",
         ["entangled","class-3","class-4","synapse","beat"],
         "Rules 45/110 with synapse 0.38. Synapse makes the class 3/4 interaction rhythmic.",
         (0.33, 0.52, 0.50, 0.63, 0.38, 0.32),
         rules=[45,110,45,110,45,110,45,110],
         owit_stepRate=10.0, owit_synapse=0.38, owit_chromAmount=0.45),

    owit("Arm Lock", "Entangled",
         ["entangled","class-4","locked","synapse","coherent"],
         "Rule 110 with very high synapse (0.65). Arms strongly lock to each other — complex but coherent.",
         (0.33, 0.52, 0.55, 0.60, 0.38, 0.32),
         rules=[110,110,110,110,110,110,110,110],
         owit_stepRate=8.0, owit_synapse=0.65, owit_chromAmount=0.40),

    owit("Cellular Flow", "Entangled",
         ["entangled","class-3","flow","alternating","chaotic"],
         "Rules 30/90 alternating — canonical class 3 and XOR rule trading control.",
         (0.30, 0.50, 0.50, 0.62, 0.40, 0.33),
         rules=[30,90,30,90,30,90,30,90],
         owit_stepRate=6.0, owit_synapse=0.30, owit_chromAmount=0.45),

    owit("Eight Engine", "Entangled",
         ["entangled","class-4","engine","complex","varied"],
         "Four-class-4 mix (110,54,106,30) cycling. Maximum complexity at 12 Hz step rate.",
         (0.35, 0.52, 0.53, 0.65, 0.38, 0.32),
         rules=[110,54,106,30,110,54,106,30],
         owit_stepRate=12.0, owit_synapse=0.30, owit_chromAmount=0.50),

    owit("Pattern Step", "Entangled",
         ["entangled","class-4","step","synced","rhythmic"],
         "Rule 193 (class 4) / 110 alternating, step-synced (owit_stepSync=1). Complex CA on the grid.",
         (0.33, 0.52, 0.52, 0.63, 0.38, 0.32),
         rules=[193,110,193,110,193,110,193,110],
         owit_stepRate=8.0, owit_synapse=0.28, owit_chromAmount=0.45,
         owit_stepSync=1, owit_stepDiv=4),

    owit("Trigger Array", "Entangled",
         ["entangled","class-4","trigger","fast","threshold"],
         "Rule 137 (class 4) at high trigger threshold (0.40) and fast step (12 Hz). Selective firing.",
         (0.32, 0.52, 0.50, 0.63, 0.38, 0.32),
         rules=[137,137,137,137,137,137,137,137],
         owit_stepRate=12.0, owit_synapse=0.25, owit_chromAmount=0.42,
         owit_triggerThresh=0.40),

    owit("Rule Wave", "Entangled",
         ["entangled","class-4","wave","complex","uniform"],
         "Rule 54 (class 4) on all arms — uniform complex CA creates standing wave patterns.",
         (0.30, 0.50, 0.50, 0.62, 0.40, 0.33),
         rules=[54,54,54,54,54,54,54,54],
         owit_stepRate=6.0, owit_synapse=0.30, owit_chromAmount=0.45),

    owit("Arm Sequence", "Entangled",
         ["entangled","varied","sequence","class-mix","organic"],
         "Eight different rules (110,30,90,45,184,110,30,90) — each arm evolving by its own rule.",
         (0.30, 0.50, 0.52, 0.62, 0.40, 0.33),
         rules=[110,30,90,45,184,110,30,90],
         owit_stepRate=8.0, owit_synapse=0.28, owit_chromAmount=0.45,
         owit_stepDiv=5),

    # ═══════════════════════════════════════════════════════
    #  FLUX  (+15)
    # ═══════════════════════════════════════════════════════
    owit("Rule Flood", "Flux",
         ["flux","class-3","flood","extreme","chaos"],
         "Rules 30/22 alternating at 25 Hz — two of the most chaotic rules flooding simultaneously.",
         (0.78, 0.70, 0.52, 0.85, 0.28, 0.20),
         rules=[30,22,30,22,30,22,30,22],
         owit_stepRate=25.0, owit_synapse=0.68, owit_chromAmount=0.72),

    owit("Full Chaos", "Flux",
         ["flux","class-3","rule-22","extreme","maximum"],
         "Rule 22 (class 3) at 30 Hz — the fastest, most chaotic preset in the OUTWIT library.",
         (0.85, 0.75, 0.52, 0.92, 0.25, 0.18),
         rules=[22,22,22,22,22,22,22,22],
         owit_stepRate=30.0, owit_synapse=0.72, owit_chromAmount=0.78),

    owit("Synapse Storm", "Flux",
         ["flux","class-3","synapse","storm","extreme"],
         "Rules 90/150 at 20 Hz with synapse 0.75. XOR rules amplified through maximum arm coupling.",
         (0.80, 0.72, 0.52, 0.87, 0.27, 0.20),
         rules=[90,150,90,150,90,150,90,150],
         owit_stepRate=20.0, owit_synapse=0.75, owit_chromAmount=0.72),

    owit("Arm Break", "Flux",
         ["flux","class-3","break","high-level","extreme"],
         "Rule 30 at high arm levels (0.95) and 24 Hz. All 8 arms at near-maximum gain in chaos.",
         (0.83, 0.73, 0.53, 0.89, 0.26, 0.18),
         rules=[30,30,30,30,30,30,30,30],
         levels=[0.95]*8,
         owit_stepRate=24.0, owit_synapse=0.65, owit_chromAmount=0.70),

    owit("Rule Storm", "Flux",
         ["flux","class-3","storm","varied","chaos"],
         "Four class 3 variants (45,22,90,150) at 20 Hz. Each pair of arms runs a different chaos rule.",
         (0.78, 0.70, 0.52, 0.85, 0.28, 0.20),
         rules=[45,22,90,150,45,22,90,150],
         owit_stepRate=20.0, owit_synapse=0.62, owit_chromAmount=0.70),

    owit("Solve Frenzy", "Flux",
         ["flux","solve","ga","frenzy","extreme"],
         "SOLVE (0.90) hunting for maximum aggression while class 3/4 rules rage at 18 Hz.",
         (0.85, 0.77, 0.53, 0.90, 0.26, 0.18),
         rules=[110,30,90,22,110,30,90,22],
         owit_stepRate=18.0, owit_synapse=0.60, owit_chromAmount=0.65,
         owit_solve=0.90, owit_huntRate=0.80, owit_targetAggression=0.95,
         owit_macroSolve=0.9),

    owit("Rule Destroy", "Flux",
         ["flux","class-3","destruction","alternating","extreme"],
         "Rules 150/22 alternating — symmetric XOR and asymmetric chaos destroying each other.",
         (0.80, 0.72, 0.52, 0.87, 0.27, 0.20),
         rules=[150,22,150,22,150,22,150,22],
         owit_stepRate=22.0, owit_synapse=0.65, owit_chromAmount=0.72),

    owit("Eight Storm", "Flux",
         ["flux","class-3","storm","eight","varied"],
         "Four class 3 rules (30,45,90,22) cycling — maximum rule variety at 20 Hz.",
         (0.80, 0.70, 0.52, 0.87, 0.27, 0.20),
         rules=[30,45,90,22,30,45,90,22],
         owit_stepRate=20.0, owit_synapse=0.68, owit_chromAmount=0.70),

    owit("Cellular Break", "Flux",
         ["flux","class-3","rule-18","break","extreme"],
         "Rule 18 (class 3, Pascal's triangle mod 2) at 24 Hz. Fractal chaos at extreme speed.",
         (0.83, 0.73, 0.50, 0.90, 0.26, 0.18),
         rules=[18,18,18,18,18,18,18,18],
         owit_stepRate=24.0, owit_synapse=0.65, owit_chromAmount=0.75),

    owit("Pattern Flood", "Flux",
         ["flux","class-3","flood","pattern","chaos"],
         "Rules 73/89 (class 3 minor variants) at 20 Hz. Less explored chaos rules in full flood.",
         (0.78, 0.68, 0.52, 0.85, 0.28, 0.20),
         rules=[73,89,73,89,73,89,73,89],
         owit_stepRate=20.0, owit_synapse=0.62, owit_chromAmount=0.68),

    owit("Chaos Bloom", "Flux",
         ["flux","class-3","class-4","bloom","ink"],
         "Rules 30/110 with inkCloud (0.40). Class 3 chaos blooms through the class 4 structure.",
         (0.78, 0.70, 0.52, 0.85, 0.28, 0.20),
         rules=[30,110,30,110,30,110,30,110],
         owit_stepRate=18.0, owit_synapse=0.60, owit_chromAmount=0.65,
         owit_inkCloud=0.40),

    owit("Surge Pattern", "Flux",
         ["flux","class-3","surge","alternating","extreme"],
         "Rules 22/150 at 22 Hz — the two most extreme class 3 rules surging against each other.",
         (0.80, 0.72, 0.52, 0.87, 0.27, 0.20),
         rules=[22,150,22,150,22,150,22,150],
         owit_stepRate=22.0, owit_synapse=0.68, owit_chromAmount=0.72),

    owit("Maximum Entropy", "Flux",
         ["flux","class-3","entropy","maximum","varied"],
         "All 8 arms on different class 3 rules (30,22,90,150,18,45,73,89). Maximum possible entropy.",
         (0.85, 0.75, 0.52, 0.92, 0.25, 0.18),
         rules=[30,22,90,150,18,45,73,89],
         owit_stepRate=25.0, owit_synapse=0.72, owit_chromAmount=0.78),

    owit("System Break", "Flux",
         ["flux","class-3","system","extreme","synapse"],
         "Rule 90 with near-max synapse (0.78) — extreme coupling creates cascade system failure.",
         (0.82, 0.73, 0.53, 0.88, 0.27, 0.18),
         rules=[90,90,90,90,90,90,90,90],
         owit_stepRate=24.0, owit_synapse=0.78, owit_chromAmount=0.72,
         owit_triggerThresh=0.15),

    owit("Solve Maximum", "Flux",
         ["flux","solve","ga","maximum","extreme"],
         "SOLVE at 1.0 with GA hunting maximum aggression+movement. The octopus evolves into its most chaotic form.",
         (0.88, 0.78, 0.55, 0.92, 0.25, 0.18),
         rules=[110,110,110,110,110,110,110,110],
         owit_stepRate=20.0, owit_synapse=0.65, owit_chromAmount=0.68,
         owit_solve=1.0, owit_huntRate=0.90,
         owit_targetAggression=1.0, owit_targetMovement=1.0,
         owit_macroSolve=1.0),

    # ═══════════════════════════════════════════════════════
    #  AETHER  (+11)
    # ═══════════════════════════════════════════════════════
    owit("Silent Grid", "Aether",
         ["aether","rule-0","silent","minimal","void"],
         "Rule 0 at very low arm levels (0.20) and near-zero step rate. The grid approaches silence.",
         (0.01, 0.12, 0.10, 0.02, 0.85, 0.50),
         rules=[0,0,0,0,0,0,0,0],
         levels=[0.20]*8,
         owit_stepRate=0.20, owit_synapse=0.01, owit_chromAmount=0.05,
         owit_masterLevel=0.35),

    owit("Ghost Arms", "Aether",
         ["aether","class-1","ghost","sparse","minimal"],
         "Rule 8 (class 1 minimal) at 0.25 arm levels. Eight ghost arms barely touching the substrate.",
         (0.01, 0.14, 0.12, 0.02, 0.83, 0.50),
         rules=[8,8,8,8,8,8,8,8],
         levels=[0.25]*8,
         owit_stepRate=0.40, owit_synapse=0.02, owit_chromAmount=0.08,
         owit_masterLevel=0.40),

    owit("Void Rule", "Aether",
         ["aether","class-1","void","alternating","minimal"],
         "Rules 0/32 alternating — dead and near-dead cells coexist in a near-void.",
         (0.01, 0.13, 0.11, 0.02, 0.83, 0.50),
         rules=[0,32,0,32,0,32,0,32],
         owit_stepRate=0.30, owit_synapse=0.02, owit_chromAmount=0.07,
         owit_masterLevel=0.35),

    owit("Trace Pattern", "Aether",
         ["aether","class-1","trace","sparse","alternating"],
         "Rules 136/0 — stable and dead cells alternating at low levels. A trace of a pattern.",
         (0.01, 0.15, 0.12, 0.02, 0.82, 0.50),
         rules=[136,0,136,0,136,0,136,0],
         levels=[0.35,0.10,0.35,0.10,0.35,0.10,0.35,0.10],
         owit_stepRate=0.50, owit_synapse=0.02, owit_chromAmount=0.08),

    owit("Near Silence", "Aether",
         ["aether","class-1","silence","minimal","void"],
         "Rule 40 (class 1) at near-minimum master level (0.30). The threshold of audibility.",
         (0.01, 0.12, 0.10, 0.02, 0.85, 0.50),
         rules=[40,40,40,40,40,40,40,40],
         owit_stepRate=0.20, owit_synapse=0.01, owit_chromAmount=0.05,
         owit_masterLevel=0.30),

    owit("Empty Grid", "Aether",
         ["aether","rule-0","empty","ink","minimal"],
         "Rule 0 with faint ink cloud (0.08). The dead grid leaves only traces — almost nothing.",
         (0.01, 0.12, 0.11, 0.02, 0.85, 0.50),
         rules=[0,0,0,0,0,0,0,0],
         owit_stepRate=0.50, owit_synapse=0.01, owit_chromAmount=0.06,
         owit_masterLevel=0.30, owit_inkCloud=0.08),

    owit("Minimal Current", "Aether",
         ["aether","class-2","minimal","quiet","slow"],
         "Rule 200 (identity copy) at lowest step rate. The current carries nothing — barely a ripple.",
         (0.01, 0.13, 0.11, 0.02, 0.84, 0.50),
         rules=[200,200,200,200,200,200,200,200],
         owit_stepRate=0.30, owit_synapse=0.02, owit_chromAmount=0.08,
         owit_masterLevel=0.38),

    owit("Dormant State", "Aether",
         ["aether","class-1","dormant","minimal","stable"],
         "Rule 128 (class 1) at low arm levels (0.25). The octopus in complete dormancy.",
         (0.01, 0.13, 0.11, 0.02, 0.83, 0.50),
         rules=[128,128,128,128,128,128,128,128],
         levels=[0.25]*8,
         owit_stepRate=0.50, owit_synapse=0.02, owit_chromAmount=0.08),

    owit("Ghost Current", "Aether",
         ["aether","class-1","ghost","alternating","trace"],
         "Rules 160/0 alternating at very low levels. A ghost current leaving no wake.",
         (0.01, 0.13, 0.11, 0.02, 0.82, 0.50),
         rules=[160,0,160,0,160,0,160,0],
         levels=[0.30,0.10,0.30,0.10,0.30,0.10,0.30,0.10],
         owit_stepRate=0.40, owit_synapse=0.02, owit_chromAmount=0.07),

    owit("Sparse Rule", "Aether",
         ["aether","class-1","sparse","stable","quiet"],
         "Rule 32 (class 1 stable) at minimal levels. Sparse energy scattered across the substrate.",
         (0.01, 0.14, 0.11, 0.02, 0.83, 0.50),
         rules=[32,32,32,32,32,32,32,32],
         owit_stepRate=0.50, owit_synapse=0.02, owit_chromAmount=0.08,
         owit_masterLevel=0.40),

    owit("Rule Whisper", "Aether",
         ["aether","class-1","whisper","stable","near-zero"],
         "Rule 136 at lowest possible arm levels (0.20) and near-zero step rate. A whisper in the void.",
         (0.01, 0.12, 0.10, 0.02, 0.85, 0.50),
         rules=[136,136,136,136,136,136,136,136],
         levels=[0.20]*8,
         owit_stepRate=0.30, owit_synapse=0.01, owit_chromAmount=0.06,
         owit_masterLevel=0.35),

    # ═══════════════════════════════════════════════════════
    #  PRISM  (×20 — new mood for OUTWIT)
    #  High chromAmount (0.65-0.85), high filterCutoff (12k-18k), bright/spectral
    # ═══════════════════════════════════════════════════════
    owit("Chromatic Web", "Prism",
         ["prism","class-4","chromatic","bright","spectral"],
         "Rule 110 at high chromAmount (0.75) and filterCutoff 14kHz. CA complexity becomes chromatic spectrum.",
         (0.15, 0.78, 0.40, 0.35, 0.58, 0.22),
         rules=[110,110,110,110,110,110,110,110],
         filters=[14000.0]*8,
         owit_stepRate=6.0, owit_synapse=0.18, owit_chromAmount=0.75),

    owit("Spectral Arms", "Prism",
         ["prism","class-2","spectral","periodic","bright"],
         "Rules 50/77 alternating with filterCutoff 15kHz and chromAmount 0.72. Periodic CA as spectral filter.",
         (0.12, 0.75, 0.38, 0.28, 0.60, 0.25),
         rules=[50,77,50,77,50,77,50,77],
         filters=[15000.0]*8,
         owit_stepRate=5.0, owit_synapse=0.15, owit_chromAmount=0.72),

    owit("Color Garden", "Prism",
         ["prism","class-2","colorful","garden","bright"],
         "Varied class 2 rules (4,50,108,184) with filterCutoff 13kHz. Each arm a different spectral color.",
         (0.12, 0.73, 0.40, 0.28, 0.60, 0.25),
         rules=[4,50,108,184,4,50,108,184],
         filters=[13000.0]*8,
         owit_stepRate=4.0, owit_synapse=0.12, owit_chromAmount=0.70),

    owit("Prismatic Rule", "Prism",
         ["prism","class-mix","prismatic","bright","varied"],
         "Class 3/2/3/4 mix (30,50,90,110) at 16kHz. Chaos and order refracting through a prism.",
         (0.18, 0.80, 0.40, 0.38, 0.58, 0.22),
         rules=[30,50,90,110,30,50,90,110],
         filters=[16000.0]*8,
         owit_stepRate=8.0, owit_synapse=0.20, owit_chromAmount=0.78),

    owit("Rainbow CA", "Prism",
         ["prism","class-4","class-3","rainbow","chromatic"],
         "Rules 110/45 alternating at chromAmount 0.80 and 15kHz. The CA spectrum becomes a rainbow.",
         (0.15, 0.82, 0.40, 0.35, 0.58, 0.22),
         rules=[110,45,110,45,110,45,110,45],
         filters=[15000.0]*8,
         owit_stepRate=6.0, owit_synapse=0.18, owit_chromAmount=0.80),

    owit("Bright Synapse", "Prism",
         ["prism","class-3","bright","synapse","bandpass"],
         "Rule 90 with synapse 0.35, bandpass filter at 14kHz, chromAmount 0.75. Bright class 3 through BP.",
         (0.22, 0.78, 0.40, 0.40, 0.55, 0.22),
         rules=[90,90,90,90,90,90,90,90],
         filters=[14000.0]*8,
         owit_stepRate=8.0, owit_synapse=0.35, owit_chromAmount=0.75,
         owit_filterType=1),

    owit("Spectral Drift", "Prism",
         ["prism","class-2","spectral","drift","lfo"],
         "Rule 50 with LFO sweeping chromAmount at 16kHz. The spectrum drifts slowly through color.",
         (0.10, 0.75, 0.35, 0.25, 0.62, 0.25),
         rules=[50,50,50,50,50,50,50,50],
         filters=[16000.0]*8,
         owit_stepRate=3.0, owit_synapse=0.10, owit_chromAmount=0.72,
         owit_lfo1Dest=2, owit_lfo1Rate=0.5, owit_lfo1Depth=0.35),

    owit("Color Current", "Prism",
         ["prism","class-2","color","sine","bright"],
         "Rules 152/130 with Sine wave (wave=2) at 13kHz. Sine arms give the color current a smooth tone.",
         (0.12, 0.73, 0.38, 0.28, 0.60, 0.25),
         rules=[152,130,152,130,152,130,152,130],
         filters=[13000.0]*8,
         waves=[2]*8,
         owit_stepRate=4.0, owit_synapse=0.12, owit_chromAmount=0.70),

    owit("Prismatic Wave", "Prism",
         ["prism","class-2","wave","bright","alternating"],
         "Rules 108/77 alternating at 14kHz and chromAmount 0.75. Two class 2 waves refracting.",
         (0.13, 0.78, 0.38, 0.30, 0.60, 0.24),
         rules=[108,77,108,77,108,77,108,77],
         filters=[14000.0]*8,
         owit_stepRate=5.0, owit_synapse=0.15, owit_chromAmount=0.75),

    owit("Chromatic Garden", "Prism",
         ["prism","class-2","chromatic","garden","varied"],
         "Eight distinct class 2 rules (4,8,24,50,77,108,152,184) at 15kHz. The full class 2 spectrum.",
         (0.12, 0.80, 0.42, 0.28, 0.60, 0.24),
         rules=[4,8,24,50,77,108,152,184],
         filters=[15000.0]*8,
         owit_stepRate=3.0, owit_synapse=0.10, owit_chromAmount=0.78),

    owit("Spectral Beat", "Prism",
         ["prism","class-4","class-3","spectral","beat"],
         "Rules 110/30 at 10 Hz, 16kHz. Complex CA becomes a bright spectral beat.",
         (0.20, 0.80, 0.42, 0.40, 0.55, 0.22),
         rules=[110,30,110,30,110,30,110,30],
         filters=[16000.0]*8,
         owit_stepRate=10.0, owit_synapse=0.25, owit_chromAmount=0.75),

    owit("Color Engine", "Prism",
         ["prism","class-3","color","engine","alternating"],
         "Rules 90/45 at 8 Hz, 14kHz and chromAmount 0.78. Class 3 engines generating color.",
         (0.18, 0.80, 0.42, 0.38, 0.55, 0.22),
         rules=[90,45,90,45,90,45,90,45],
         filters=[14000.0]*8,
         owit_stepRate=8.0, owit_synapse=0.28, owit_chromAmount=0.78),

    owit("Bright Automaton", "Prism",
         ["prism","class-4","bright","slow","chromAmount"],
         "Rule 110 at slow rate (4 Hz) with very high chromAmount (0.82) and 17kHz. Bright slow CA.",
         (0.12, 0.85, 0.38, 0.28, 0.60, 0.22),
         rules=[110,110,110,110,110,110,110,110],
         filters=[17000.0]*8,
         owit_stepRate=4.0, owit_synapse=0.12, owit_chromAmount=0.82),

    owit("Prismatic Bloom", "Prism",
         ["prism","class-4","class-2","bloom","ink"],
         "Rules 110/50 with inkCloud (0.25) at 15kHz. Ink blooms through the prismatic spectrum.",
         (0.13, 0.82, 0.40, 0.30, 0.58, 0.24),
         rules=[110,50,110,50,110,50,110,50],
         filters=[15000.0]*8,
         owit_stepRate=5.0, owit_synapse=0.15, owit_chromAmount=0.80,
         owit_inkCloud=0.25),

    owit("Chromatic Storm", "Prism",
         ["prism","class-3","storm","bright","chromatic"],
         "Rules 30/22 at 12 Hz with chromAmount 0.82 and 16kHz. Class 3 chaos through bright prism.",
         (0.28, 0.85, 0.45, 0.48, 0.52, 0.20),
         rules=[30,22,30,22,30,22,30,22],
         filters=[16000.0]*8,
         owit_stepRate=12.0, owit_synapse=0.38, owit_chromAmount=0.82),

    owit("Spectral Lock", "Prism",
         ["prism","class-4","locked","bright","synapse"],
         "Rule 110 with high synapse (0.45) and 15kHz. Phase-locked complex CA in bright spectral space.",
         (0.18, 0.80, 0.42, 0.38, 0.55, 0.22),
         rules=[110,110,110,110,110,110,110,110],
         filters=[15000.0]*8,
         owit_stepRate=6.0, owit_synapse=0.45, owit_chromAmount=0.78),

    owit("Color Cascade", "Prism",
         ["prism","class-mix","cascade","bright","varied"],
         "Rules cascade from class 2 to class 3 to class 4 (30,50,90,110,150,90,50,30) at 14kHz.",
         (0.18, 0.78, 0.42, 0.38, 0.55, 0.22),
         rules=[30,50,90,110,150,90,50,30],
         filters=[14000.0]*8,
         owit_stepRate=8.0, owit_synapse=0.22, owit_chromAmount=0.75),

    owit("Rainbow Rules", "Prism",
         ["prism","class-mix","rainbow","varied","bright"],
         "Eight different rules across all classes (4,51,90,110,152,30,77,200) at 16kHz. Full spectrum.",
         (0.15, 0.82, 0.45, 0.35, 0.55, 0.24),
         rules=[4,51,90,110,152,30,77,200],
         filters=[16000.0]*8,
         owit_stepRate=5.0, owit_synapse=0.15, owit_chromAmount=0.80),

    owit("Bright Solve", "Prism",
         ["prism","solve","ga","bright","hunting"],
         "SOLVE (0.60) hunting for high brightness while class 4/3 rules run at 15kHz. GA seeks the light.",
         (0.15, 0.82, 0.42, 0.38, 0.55, 0.22),
         rules=[110,30,90,50,110,30,90,50],
         filters=[15000.0]*8,
         owit_stepRate=8.0, owit_synapse=0.20, owit_chromAmount=0.75,
         owit_solve=0.60, owit_huntRate=0.50, owit_targetBrightness=0.90,
         owit_macroSolve=0.6),

    owit("Spectral Storm", "Prism",
         ["prism","class-3","storm","spectral","extreme"],
         "Class 3 storm (30,90,150,22) at 15 Hz and 16kHz. Chaos rendered as pure spectral energy.",
         (0.28, 0.85, 0.45, 0.48, 0.52, 0.20),
         rules=[30,90,150,22,30,90,150,22],
         filters=[16000.0]*8,
         owit_stepRate=15.0, owit_synapse=0.40, owit_chromAmount=0.82),

    # ═══════════════════════════════════════════════════════
    #  FOUNDATION  (×12 — new mood for OUTWIT)
    #  Warm, stable, grounded. Low filterCutoff (1k-4k), class 1/2, slow.
    # ═══════════════════════════════════════════════════════
    owit("Root Pattern", "Foundation",
         ["foundation","class-2","warm","root","stable"],
         "Rule 200 (identity) with low filterCutoff (2500 Hz). The automaton copies itself — a warm root.",
         (0.12, 0.22, 0.45, 0.10, 0.30, 0.70),
         rules=[200,200,200,200,200,200,200,200],
         filters=[2500.0]*8,
         owit_stepRate=1.5, owit_synapse=0.05, owit_chromAmount=0.12,
         owit_denMix=0.28, owit_denSize=0.45),

    owit("Stable Ground", "Foundation",
         ["foundation","class-2","stable","warm","grounded"],
         "Rule 184 (traffic rule) at 2.0 Hz with 3000 Hz filter. A stable traveling-wave ground tone.",
         (0.12, 0.25, 0.45, 0.10, 0.28, 0.68),
         rules=[184,184,184,184,184,184,184,184],
         filters=[3000.0]*8,
         owit_stepRate=2.0, owit_synapse=0.08, owit_chromAmount=0.15),

    owit("Warm Automaton", "Foundation",
         ["foundation","class-2","warm","pad","grounded"],
         "Rule 50 (class 2) at 3500 Hz filter. The periodic automaton as a warm pad foundation.",
         (0.12, 0.25, 0.45, 0.10, 0.28, 0.68),
         rules=[50,50,50,50,50,50,50,50],
         filters=[3500.0]*8,
         owit_stepRate=2.0, owit_synapse=0.08, owit_chromAmount=0.15),

    owit("Foundation Rule", "Foundation",
         ["foundation","class-1","stable","deep","drone"],
         "Rule 136 (class 1 stable) at 2000 Hz filter — a deep stable automaton drone.",
         (0.10, 0.20, 0.42, 0.08, 0.30, 0.70),
         rules=[136,136,136,136,136,136,136,136],
         filters=[2000.0]*8,
         owit_stepRate=1.5, owit_synapse=0.06, owit_chromAmount=0.12),

    owit("Deep Root", "Foundation",
         ["foundation","class-2","deep","den","warm"],
         "Rule 204 (identity) with 1500 Hz filter and den reverb (0.35). A deep root with resonance.",
         (0.10, 0.18, 0.48, 0.08, 0.28, 0.72),
         rules=[204,204,204,204,204,204,204,204],
         filters=[1500.0]*8,
         owit_stepRate=1.0, owit_synapse=0.05, owit_chromAmount=0.10,
         owit_denMix=0.35, owit_denSize=0.55),

    owit("Grounded Pattern", "Foundation",
         ["foundation","class-2","grounded","warm","steady"],
         "Rule 108 at 3000 Hz and step rate 2.5. Deep class 2 periodicity as a grounded texture.",
         (0.13, 0.25, 0.45, 0.12, 0.28, 0.68),
         rules=[108,108,108,108,108,108,108,108],
         filters=[3000.0]*8,
         owit_stepRate=2.5, owit_synapse=0.10, owit_chromAmount=0.18),

    owit("Warm Grid", "Foundation",
         ["foundation","class-2","warm","grid","paired"],
         "Rules 77/78 (class 2 pair) at 2500 Hz. The paired rules create a warm, slightly varied grid.",
         (0.12, 0.23, 0.44, 0.10, 0.30, 0.70),
         rules=[77,78,77,78,77,78,77,78],
         filters=[2500.0]*8,
         owit_stepRate=2.0, owit_synapse=0.08, owit_chromAmount=0.15),

    owit("Base Pattern", "Foundation",
         ["foundation","class-2","base","steady","warm"],
         "Rule 152 (class 2) at 3500 Hz and step rate 2.5. A reliable, warm base pattern.",
         (0.13, 0.25, 0.44, 0.12, 0.30, 0.68),
         rules=[152,152,152,152,152,152,152,152],
         filters=[3500.0]*8,
         owit_stepRate=2.5, owit_synapse=0.10, owit_chromAmount=0.18),

    owit("Root Current", "Foundation",
         ["foundation","class-2","root","ink","current"],
         "Rules 184/50 with inkCloud (0.15) at 2500 Hz. A warm root tone with gentle ink traces.",
         (0.12, 0.23, 0.45, 0.10, 0.30, 0.68),
         rules=[184,50,184,50,184,50,184,50],
         filters=[2500.0]*8,
         owit_stepRate=2.0, owit_synapse=0.08, owit_chromAmount=0.15,
         owit_inkCloud=0.15),

    owit("Stable Sea", "Foundation",
         ["foundation","class-1","class-2","stable","warm"],
         "Rules 200/136 (identity and stable class 1) at 3000 Hz. Maximum stability as foundation.",
         (0.10, 0.22, 0.42, 0.08, 0.30, 0.70),
         rules=[200,136,200,136,200,136,200,136],
         filters=[3000.0]*8,
         owit_stepRate=1.5, owit_synapse=0.05, owit_chromAmount=0.12),

    owit("Warm Shore", "Foundation",
         ["foundation","class-2","warm","bass","pitched"],
         "Rules 24/25 with all arms pitched down an octave (-12 semitones). A warm bass shore.",
         (0.13, 0.25, 0.45, 0.12, 0.28, 0.68),
         rules=[24,25,24,25,24,25,24,25],
         filters=[4000.0]*8,
         pitches=[-12,-12,-12,-12,-12,-12,-12,-12],
         owit_stepRate=2.5, owit_synapse=0.10, owit_chromAmount=0.18),

    owit("Ground State II", "Foundation",
         ["foundation","class-2","ground","den","stable"],
         "Rule 130 (class 2) with den reverb (0.30) at 2000 Hz. Stability with resonant depth.",
         (0.11, 0.22, 0.48, 0.09, 0.28, 0.70),
         rules=[130,130,130,130,130,130,130,130],
         filters=[2000.0]*8,
         owit_stepRate=1.5, owit_synapse=0.06, owit_chromAmount=0.12,
         owit_denSize=0.50, owit_denMix=0.30),
]


def main():
    print(f"\n{'='*60}")
    print(f"  OUTWIT Blocks 2 & 3 Generator {'(DRY RUN) ' if DRY else ''}")
    print(f"  Total presets: {len(PRESETS)}")
    print(f"{'='*60}\n")

    for p in PRESETS:
        write_preset(p)

    print(f"\n{'='*60}")
    print(f"  Written: {_w}  |  Skipped (exists): {_s}")
    print(f"{'='*60}\n")

    from collections import Counter
    moods = Counter(p["mood"] for p in PRESETS)
    print("Mood distribution (this batch):")
    for m, n in sorted(moods.items()):
        print(f"  {m:12s}: {n}")

    total = sum(1 for _ in
                Path(BASE).glob("*/Outwit/*.xometa"))
    print(f"\nOUTWIT total on disk after run: {total}")


if __name__ == "__main__":
    main()
