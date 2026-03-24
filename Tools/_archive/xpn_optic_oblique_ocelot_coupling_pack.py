#!/usr/bin/env python3
"""
xpn_optic_oblique_ocelot_coupling_pack.py
Generates Entangled preset stubs for OPTIC, OBLIQUE, and OCELOT partner coverage.

Output: Presets/XOlokun/Entangled/
Skips existing files.
"""

import json
import os
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
PRESETS_DIR = SCRIPT_DIR.parent / "Presets" / "XOlokun" / "Entangled"
PRESETS_DIR.mkdir(parents=True, exist_ok=True)

TODAY = "2026-03-16"
AUTHOR = "XPN Tool — Optic/Oblique/Ocelot Coupling Pack"

# ── DNA baselines ────────────────────────────────────────────────────────────
DNA = {
    "OPTIC":   {"brightness": 0.9,  "warmth": 0.3,  "movement": 0.7,
                "density": 0.4,  "space": 0.8,  "aggression": 0.2},
    "OBLIQUE": {"brightness": 0.75, "warmth": 0.45, "movement": 0.8,
                "density": 0.6,  "space": 0.6,  "aggression": 0.55},
    "OCELOT":  {"brightness": 0.6,  "warmth": 0.65, "movement": 0.6,
                "density": 0.65, "space": 0.5,  "aggression": 0.5},
}

DNA_PARTNERS = {
    "ONSET":      {"brightness": 0.55, "warmth": 0.5,  "movement": 0.75,
                   "density": 0.7,  "space": 0.4,  "aggression": 0.7},
    "OBLONG":     {"brightness": 0.5,  "warmth": 0.7,  "movement": 0.5,
                   "density": 0.75, "space": 0.5,  "aggression": 0.4},
    "OVERWORLD":  {"brightness": 0.65, "warmth": 0.4,  "movement": 0.65,
                   "density": 0.55, "space": 0.7,  "aggression": 0.45},
    "OVERDUB":    {"brightness": 0.45, "warmth": 0.6,  "movement": 0.55,
                   "density": 0.6,  "space": 0.75, "aggression": 0.35},
    "OPAL":       {"brightness": 0.7,  "warmth": 0.4,  "movement": 0.7,
                   "density": 0.5,  "space": 0.8,  "aggression": 0.25},
    "ORBITAL":    {"brightness": 0.6,  "warmth": 0.35, "movement": 0.8,
                   "density": 0.45, "space": 0.85, "aggression": 0.3},
    "ORGANON":    {"brightness": 0.55, "warmth": 0.55, "movement": 0.6,
                   "density": 0.65, "space": 0.6,  "aggression": 0.4},
    "OUROBOROS":  {"brightness": 0.5,  "warmth": 0.5,  "movement": 0.75,
                   "density": 0.7,  "space": 0.55, "aggression": 0.6},
    "OBSIDIAN":   {"brightness": 0.35, "warmth": 0.55, "movement": 0.5,
                   "density": 0.8,  "space": 0.45, "aggression": 0.65},
    "ORACLE":     {"brightness": 0.65, "warmth": 0.45, "movement": 0.65,
                   "density": 0.5,  "space": 0.75, "aggression": 0.35},
    "OSPREY":     {"brightness": 0.7,  "warmth": 0.35, "movement": 0.75,
                   "density": 0.4,  "space": 0.8,  "aggression": 0.5},
    "OSTERIA":    {"brightness": 0.6,  "warmth": 0.7,  "movement": 0.5,
                   "density": 0.7,  "space": 0.5,  "aggression": 0.35},
    "OWLFISH":    {"brightness": 0.55, "warmth": 0.5,  "movement": 0.65,
                   "density": 0.55, "space": 0.7,  "aggression": 0.4},
    "OHM":        {"brightness": 0.5,  "warmth": 0.6,  "movement": 0.55,
                   "density": 0.65, "space": 0.65, "aggression": 0.3},
    "ORPHICA":    {"brightness": 0.75, "warmth": 0.4,  "movement": 0.8,
                   "density": 0.45, "space": 0.85, "aggression": 0.25},
    "OBBLIGATO":  {"brightness": 0.6,  "warmth": 0.5,  "movement": 0.7,
                   "density": 0.55, "space": 0.65, "aggression": 0.4},
    "OTTONI":     {"brightness": 0.65, "warmth": 0.5,  "movement": 0.65,
                   "density": 0.6,  "space": 0.6,  "aggression": 0.55},
    "OLE":        {"brightness": 0.7,  "warmth": 0.6,  "movement": 0.85,
                   "density": 0.55, "space": 0.55, "aggression": 0.6},
}

PARTNER_ENGINES = list(DNA_PARTNERS.keys())

# ── Prefix map ───────────────────────────────────────────────────────────────
PREFIX = {
    "OPTIC":     "optic_",
    "OBLIQUE":   "oblq_",
    "OCELOT":    "ocelot_",
    "ONSET":     "onset_",
    "OBLONG":    "bob_",
    "OVERWORLD": "ow_",
    "OVERDUB":   "dub_",
    "OPAL":      "opal_",
    "ORBITAL":   "orb_",
    "ORGANON":   "org_",
    "OUROBOROS": "ouro_",
    "OBSIDIAN":  "obs_",
    "ORACLE":    "orac_",
    "OSPREY":    "osp_",
    "OSTERIA":   "ost_",
    "OWLFISH":   "owlf_",
    "OHM":       "ohm_",
    "ORPHICA":   "orph_",
    "OBBLIGATO": "obbl_",
    "OTTONI":    "otto_",
    "OLE":       "ole_",
}

COUPLING_TYPES = [
    "AmpToFilter", "AudioToFM", "EnvToMod", "PitchSync",
    "HARMONIC_BLEND", "SpectralMorph", "GrainToMod", "RhythmGate",
]

# ── Evocative name tables ────────────────────────────────────────────────────
# 3-way marquee names (OPTIC × OBLIQUE × OCELOT)
MARQUEE_NAMES = [
    ("Phosphor Prism Hunt",      "Neon light diffracting through jungle canopy — OPTIC's pulse refracts in OBLIQUE's prism while OCELOT stalks the resulting shadow."),
    ("Tawny Glass Circuit",      "Burnt-orange circuits behind frosted violet — biome-shift and prismatic bounce locked to visual modulation in a single moving frame."),
    ("Spotted Frequency Arc",    "Each spot on the coat is a frequency node — OCELOT's harmonic body mapped across OBLIQUE's angular ricochet and OPTIC's reactive grid."),
    ("Three-Body Chromatic",     "Three gravitational bodies in color space: green urgency, violet slant, tawny drift — orbiting a shared rhythmic center of mass."),
    ("Refraction Biome",         "OPTIC refracts OBLIQUE's angular energy through OCELOT's warm ecosystem — a landscape that bends light and shifts terrain simultaneously."),
    ("Zero Apex Prism",          "Zero-Audio Identity blessing meets apex predator instinct meets prismatic bounce — three distinct wills resolving into one iridescent strike."),
]

# OPTIC paired names: (partner, preset_name, description)
OPTIC_PAIRS = [
    ("ONSET",     "Phosphor Transient Grid",    "OPTIC's visual modulation gates every ONSET transient — the kick lands only when the pixel fires."),
    ("OBLONG",    "Green Screen Detuned",        "Phosphor green smeared across OBLONG's fat detuned stack — a chroma-key hallucination of analog warmth."),
    ("OVERWORLD", "CRT Chip Scan",               "OPTIC's scan-line feedback synced to OVERWORLD's retro oscillators — both engines live in the era of phosphorescent screens."),
    ("OVERDUB",   "Tape Pixel",                  "Visual modulation rate-locked to OVERDUB's tape flutter — the image shakes in time with the delay head."),
    ("OPAL",      "Grain Luminance",             "OPTIC brightness envelope feeding OPAL's grain scatter — each cloud of particles lit from within by reactive phosphor."),
    ("ORBITAL",   "Sidereal Pulse",              "OPTIC pulse subdivisions tracing ORBITAL's planetary arc — a clock that ticks in light-years."),
    ("ORGANON",   "Logic Gate Glow",             "OPTIC vizMode feeding ORGANON's logical sequencer — the display IS the score."),
    ("OUROBOROS", "Feedback Phosphor Loop",      "OPTIC vizFeedback and OUROBOROS recursion feeding each other — two self-consuming signals chasing phosphor tails."),
    ("OBSIDIAN",  "Dark Monitor",                "Phosphor green on obsidian black — OPTIC's high-contrast reactivity carving shapes from OBSIDIAN's dense low-end."),
    ("ORACLE",    "Divination Display",          "ORACLE's prediction engine outputs routed as OPTIC input signals — prophecy rendered as light."),
    ("OSPREY",    "Thermal Vision Strike",       "OPTIC thermal-palette mode locking OSPREY's attack transient — the raptor sees in heat before it dives."),
    ("OSTERIA",   "Warm Backlight Menu",         "OPTIC's cool reactive light warming against OSTERIA's tavern textures — a menu glowing in candlelit silicon."),
    ("OWLFISH",   "Bioluminescent Grid",         "OWLFISH's deep-sea bioluminescence as OPTIC input signal — pressure-triggered phosphor in absolute dark."),
    ("OHM",       "Commune Frequency Display",   "The commune tunes the dial until OPTIC confirms resonance — resistance visualized as glowing filament."),
    ("ORPHICA",   "Spectral Harp Screen",        "ORPHICA's microsound partials mapped to OPTIC screen columns — every harp string a scanline."),
    ("OBBLIGATO", "Wind Display Bond",           "OBBLIGATO's obligatory wind line written in light — OPTIC traces each breath as a vector on the screen."),
    ("OTTONI",    "Brass Harmonic Scope",        "OTTONI's triple brass overtones scoped on OPTIC's display — a live spectral painting of growing brass chords."),
    ("OLE",       "Drama Phosphor Stage",        "OPTIC lights the stage for OLE's drama — each intensity spike a spotlight moment."),
]

# OBLIQUE paired names
OBLIQUE_PAIRS = [
    ("ONSET",     "Ricochet Floor",              "OBLIQUE's angular bounce redirecting ONSET's drum floor — the kick arrives from an unexpected angle."),
    ("OBLONG",    "Prism Fat Stack",             "RTJ energy smearing OBLONG's detuned warmth through a violet prism — big bass with angular attitude."),
    ("OVERWORLD", "Tame Chip Bounce",            "Tame Impala-style prism shimmer over OVERWORLD's chip oscillators — psychedelic NES."),
    ("OVERDUB",   "Dub Angle",                   "OBLIQUE's funk slant applied to OVERDUB's tape delay — the echo comes back at 37 degrees."),
    ("OPAL",      "Prismatic Grain Field",       "OPAL granular clouds refracting through OBLIQUE's angular modulation — light scattered in a thousand directions."),
    ("ORBITAL",   "Elliptical Refraction",       "ORBITAL's planetary orbit tilted through OBLIQUE's prism — an ellipse that bends in color."),
    ("ORGANON",   "Logic Slant",                 "OBLIQUE redirecting ORGANON's Boolean logic outputs — the rule is to never go straight."),
    ("OUROBOROS", "Recursive Angle",             "OUROBOROS loops feeding into OBLIQUE's bounce — each revolution exits at a different angle than it entered."),
    ("OBSIDIAN",  "Black Prism",                 "OBLIQUE's violet prism refracting light that OBSIDIAN's mass refuses to emit — dark matter diffraction."),
    ("ORACLE",    "Slanted Prophecy",            "ORACLE's output arriving at an oblique angle — the prediction is correct but the path is never direct."),
    ("OSPREY",    "Dive Vector",                 "OSPREY's dive recalculated through OBLIQUE's angular engine — the strike arrives from the unexpected quadrant."),
    ("OSTERIA",   "Funk Tavern",                 "RTJ × Funk DNA landing in OSTERIA's warm communal space — the groove is served alongside the meal."),
    ("OWLFISH",   "Pressure Angle",              "OWLFISH's deep-sea pressure waves deflected by OBLIQUE — sonar pings returning from oblique surfaces."),
    ("OHM",       "Commune Slant",               "OHM's communal resonance played on OBLIQUE's angled grid — the jam circle is a rhombus."),
    ("ORPHICA",   "Micro-Prism Harp",            "ORPHICA's microsound plucks refracted through OBLIQUE — each particle emerges at a different spectral angle."),
    ("OBBLIGATO", "Obligate Bounce",             "OBBLIGATO's necessary wind line angled through OBLIQUE — the obligatory phrase arrives from the side."),
    ("OTTONI",    "Brass Prism Fanfare",         "OTTONI's growing brass chords spread through OBLIQUE's prism — the fanfare explodes into colored vectors."),
    ("OLE",       "Drama Slant Drama",           "OLE's Afro-Latin drama amplified by OBLIQUE's funk angle — the performance arrives sideways and hits harder."),
]

# OCELOT paired names
OCELOT_PAIRS = [
    ("ONSET",     "Percussion Biome",            "OCELOT's ecosystem-shift applied to ONSET's drum palette — the kit evolves as the habitat changes."),
    ("OBLONG",    "Jungle Fat",                  "OCELOT's warm tawny texture layered over OBLONG's detuned mass — dense foliage over deep earth."),
    ("OVERWORLD", "Chip Savanna",                "OVERWORLD's retro chip tones inhabiting OCELOT's shifting biome — NES cartridges found in the undergrowth."),
    ("OVERDUB",   "Tape Habitat",                "OCELOT's biome-shift pacing OVERDUB's tape flutter — the environment breathes in the time of delay."),
    ("OPAL",      "Canopy Grain",                "OPAL's granular canopy filtered through OCELOT's tawny warmth — light through leaves, dappled and moving."),
    ("ORBITAL",   "Celestial Territory",         "OCELOT marking territory across ORBITAL's planetary arc — each orbit claiming a new biome."),
    ("ORGANON",   "Ecosystem Logic",             "ORGANON's logical structures shaped by OCELOT's habitat rules — Boolean gates decided by the biome."),
    ("OUROBOROS", "Predator Loop",               "OCELOT circling OUROBOROS's recursive loop — apex predator and self-consuming serpent in the same territory."),
    ("OBSIDIAN",  "Shadow Biome",                "OCELOT hunting through OBSIDIAN's dense shadow territory — spotted coat invisible in black volcanic field."),
    ("ORACLE",    "Feral Foresight",             "ORACLE's predictions filtered through OCELOT's animal instinct — the prophecy arrives via scent and sound."),
    ("OSPREY",    "Apex Convergence",            "Two apex predators in a shared biome — OCELOT marks the ground as OSPREY controls the sky above."),
    ("OSTERIA",   "Wild Warmth Table",           "OCELOT's warm tawny palette at OSTERIA's table — the feast is organic, the spice is feral."),
    ("OWLFISH",   "Pressure Habitat",            "OCELOT's surface biome pressed down by OWLFISH's deep pressure — where ecosystems meet at the thermocline."),
    ("OHM",       "Commune Habitat",             "OHM's communal resonance rooted in OCELOT's shifting biome — the commune moves with the seasons."),
    ("ORPHICA",   "Microsound Undergrowth",      "ORPHICA's microsound particles hiding in OCELOT's undergrowth — the microsounds ARE the rustling leaves."),
    ("OBBLIGATO", "Wind Through Canopy",         "OBBLIGATO's obligatory wind line threading through OCELOT's canopy — the note the jungle requires."),
    ("OTTONI",    "Brass Biome Shift",           "OTTONI's triple brass calling across OCELOT's shifting biome — a fanfare announcing the ecosystem change."),
    ("OLE",       "Savanna Drama",               "OLE's Afro-Latin drama rooted in OCELOT's warm savanna — the dance belongs to this landscape."),
]


def blend_dna(dna_a, dna_b, weight_a=0.5):
    """Blend two DNA dicts."""
    w_b = 1.0 - weight_a
    return {k: round(dna_a[k] * weight_a + dna_b[k] * w_b, 3) for k in dna_a}


def blend_dna3(dna_a, dna_b, dna_c, wa=0.4, wb=0.3):
    """Blend three DNA dicts."""
    wc = round(1.0 - wa - wb, 3)
    return {
        k: round(dna_a[k] * wa + dna_b[k] * wb + dna_c[k] * wc, 3)
        for k in dna_a
    }


def default_params(engine_key):
    """Return a minimal parameter stub for an engine."""
    p = PREFIX[engine_key]
    if engine_key == "OPTIC":
        return {
            f"{p}reactivity": 0.7,
            f"{p}inputGain": 0.85,
            f"{p}autoPulse": 0,
            f"{p}pulseRate": 0.45,
            f"{p}pulseShape": 0.5,
            f"{p}modDepth": 0.6,
            f"{p}vizMode": 1,
            f"{p}vizFeedback": 0.5,
            f"{p}vizSpeed": 0.55,
            f"{p}vizIntensity": 0.65,
        }
    if engine_key == "OBLIQUE":
        return {
            f"{p}prismAngle": 0.55,
            f"{p}bounce": 0.65,
            f"{p}refraction": 0.6,
            f"{p}funkDepth": 0.7,
            f"{p}modRate": 0.5,
            f"{p}modDepth": 0.6,
            f"{p}filterCutoff": 3200.0,
            f"{p}filterReso": 0.35,
            f"{p}ampAttack": 0.02,
            f"{p}ampRelease": 1.2,
        }
    if engine_key == "OCELOT":
        return {
            f"{p}biomeShift": 0.5,
            f"{p}warmth": 0.65,
            f"{p}movement": 0.6,
            f"{p}predatorDepth": 0.55,
            f"{p}filterCutoff": 2400.0,
            f"{p}filterReso": 0.3,
            f"{p}ampAttack": 0.04,
            f"{p}ampRelease": 1.8,
        }
    # Generic stub for partner engines
    return {
        f"{p}filterCutoff": 2800.0,
        f"{p}filterReso": 0.3,
        f"{p}ampAttack": 0.03,
        f"{p}ampRelease": 1.5,
    }


def coupling_pair(engine_a, engine_b, ctype_idx=0, amount=0.6):
    return {
        "engineA": engine_a.title() if len(engine_a) > 4 else engine_a,
        "engineB": engine_b.title() if len(engine_b) > 4 else engine_b,
        "type": COUPLING_TYPES[ctype_idx % len(COUPLING_TYPES)],
        "amount": round(amount, 2),
    }


def engine_display(e):
    """Return display-form engine name (Title-cased short names)."""
    special = {
        "OPTIC": "Optic", "OBLIQUE": "Oblique", "OCELOT": "Ocelot",
        "ONSET": "Onset", "OBLONG": "Oblong", "OVERWORLD": "Overworld",
        "OVERDUB": "Overdub", "OPAL": "Opal", "ORBITAL": "Orbital",
        "ORGANON": "Organon", "OUROBOROS": "Ouroboros", "OBSIDIAN": "Obsidian",
        "ORACLE": "Oracle", "OSPREY": "Osprey", "OSTERIA": "Osteria",
        "OWLFISH": "Owlfish", "OHM": "Ohm", "ORPHICA": "Orphica",
        "OBBLIGATO": "Obbligato", "OTTONI": "Ottoni", "OLE": "Ole",
    }
    return special.get(e, e.title())


def make_preset(name, engines, dna, params, coupling_pairs, description, tags):
    return {
        "schema_version": 1,
        "name": name,
        "mood": "Entangled",
        "engines": [engine_display(e) for e in engines],
        "author": AUTHOR,
        "version": "1.0.0",
        "description": description,
        "tags": tags + ["entangled", "coupling"],
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": "Medium",
        "tempo": None,
        "created": TODAY,
        "legacy": {"sourceInstrument": None, "sourceCategory": None, "sourcePresetName": None},
        "parameters": {engine_display(e): params.get(e, {}) for e in engines},
        "coupling": {"pairs": coupling_pairs},
        "sequencer": None,
        "dna": dna,
    }


def safe_filename(name):
    return name.replace("/", "-").replace("\\", "-").replace(":", "") + ".xometa"


def write_preset(preset):
    fname = safe_filename(preset["name"])
    fpath = PRESETS_DIR / fname
    if fpath.exists():
        print(f"  SKIP (exists): {fname}")
        return False
    fpath.write_text(json.dumps(preset, indent=2) + "\n")
    print(f"  WRITE: {fname}")
    return True


# ── Generate presets ─────────────────────────────────────────────────────────

written = 0
skipped = 0

print("=== Generating 3-way OPTIC × OBLIQUE × OCELOT marquee presets ===")
for i, (name, desc) in enumerate(MARQUEE_NAMES):
    engines = ["OPTIC", "OBLIQUE", "OCELOT"]
    dna = blend_dna3(DNA["OPTIC"], DNA["OBLIQUE"], DNA["OCELOT"])
    params = {e: default_params(e) for e in engines}
    pairs = [
        coupling_pair("OPTIC", "OBLIQUE", ctype_idx=i,      amount=0.65),
        coupling_pair("OBLIQUE", "OCELOT", ctype_idx=i + 1, amount=0.60),
        coupling_pair("OCELOT", "OPTIC",   ctype_idx=i + 2, amount=0.55),
    ]
    tags = ["optic", "oblique", "ocelot", "3-way", "marquee",
            "phosphor", "prism", "biome"]
    p = make_preset(name, engines, dna, params, pairs, desc, tags)
    result = write_preset(p)
    written += result
    skipped += not result

print(f"\n=== Generating 18 OPTIC paired presets ===")
for i, (partner, name, desc) in enumerate(OPTIC_PAIRS):
    engines = ["OPTIC", partner]
    partner_dna = DNA_PARTNERS.get(partner, DNA["OPTIC"])
    dna = blend_dna(DNA["OPTIC"], partner_dna, weight_a=0.55)
    params = {e: default_params(e) for e in engines}
    pairs = [
        coupling_pair("OPTIC", partner, ctype_idx=i,     amount=0.6),
        coupling_pair(partner, "OPTIC", ctype_idx=i + 3, amount=0.4),
    ]
    tags = ["optic", partner.lower(), "phosphor", "visual-modulation"]
    p = make_preset(name, engines, dna, params, pairs, desc, tags)
    result = write_preset(p)
    written += result
    skipped += not result

print(f"\n=== Generating 18 OBLIQUE paired presets ===")
for i, (partner, name, desc) in enumerate(OBLIQUE_PAIRS):
    engines = ["OBLIQUE", partner]
    partner_dna = DNA_PARTNERS.get(partner, DNA["OBLIQUE"])
    dna = blend_dna(DNA["OBLIQUE"], partner_dna, weight_a=0.55)
    params = {e: default_params(e) for e in engines}
    pairs = [
        coupling_pair("OBLIQUE", partner, ctype_idx=i,     amount=0.65),
        coupling_pair(partner, "OBLIQUE", ctype_idx=i + 2, amount=0.45),
    ]
    tags = ["oblique", partner.lower(), "prism", "rtj", "funk", "tame-impala"]
    p = make_preset(name, engines, dna, params, pairs, desc, tags)
    result = write_preset(p)
    written += result
    skipped += not result

print(f"\n=== Generating 18 OCELOT paired presets ===")
for i, (partner, name, desc) in enumerate(OCELOT_PAIRS):
    engines = ["OCELOT", partner]
    partner_dna = DNA_PARTNERS.get(partner, DNA["OCELOT"])
    dna = blend_dna(DNA["OCELOT"], partner_dna, weight_a=0.55)
    params = {e: default_params(e) for e in engines}
    pairs = [
        coupling_pair("OCELOT", partner, ctype_idx=i,     amount=0.6),
        coupling_pair(partner, "OCELOT", ctype_idx=i + 4, amount=0.45),
    ]
    tags = ["ocelot", partner.lower(), "biome-shift", "tawny", "apex"]
    p = make_preset(name, engines, dna, params, pairs, desc, tags)
    result = write_preset(p)
    written += result
    skipped += not result

print(f"\n=== Done — {written} written, {skipped} skipped ===")
