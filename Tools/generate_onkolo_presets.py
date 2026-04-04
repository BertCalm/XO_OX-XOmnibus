#!/usr/bin/env python3
"""
generate_onkolo_presets.py
--------------------------
Generates 104 factory presets for the Onkolo Clavinet D6 physical model engine.

Onkolo is a percussive, funky Clavinet D6 — its sweet spot is Kinetic and Organic.
25+ presets have aggression >= 0.7 (Clavinet is naturally aggressive: percussive
attack, auto-wah, pickup snap).

Engine parameters (prefix: onko_):
  onko_funk          [0.0–1.0]        Wah depth / auto-wah
  onko_pickup        [0.0–1.0]        Pickup position (neck=0 → bridge=1)
  onko_brightness    [200–20000 Hz]   High shelf / tone
  onko_clunk         [0.0–1.0]        Key-off mechanical clunk
  onko_attack        [0.001–0.2 s]    Amp envelope attack
  onko_decay         [0.05–3.0 s]     Amp envelope decay
  onko_sustain       [0.0–1.0]        Amp envelope sustain
  onko_release       [0.01–2.0 s]     Amp envelope release
  onko_filterEnvAmt  [0.0–1.0]        Filter envelope amount
  onko_migration     [0.0–1.0]        FUSION (cross-engine blend)
  onko_bendRange     [1–24 semitones] Pitch bend range (integer)
  onko_macroCharacter [0.0–1.0]       Macro CHARACTER
  onko_macroMovement  [0.0–1.0]       Macro MOVEMENT
  onko_macroCoupling  [0.0–1.0]       Macro COUPLING
  onko_macroSpace     [0.0–1.0]       Macro SPACE
  onko_lfo1Rate      [0.005–20.0 Hz]  LFO1 rate
  onko_lfo1Depth     [0.0–1.0]        LFO1 depth
  onko_lfo1Shape     [0–4 int]        LFO1 shape (0=Sine,1=Tri,2=Saw,3=Square,4=SH)
  onko_lfo2Rate      [0.005–20.0 Hz]  LFO2 rate
  onko_lfo2Depth     [0.0–1.0]        LFO2 depth
  onko_lfo2Shape     [0–4 int]        LFO2 shape

Mood distribution (104 presets):
  Kinetic:      25  (funky staccato, percussive, slap)
  Organic:      20  (diaspora character, natural wood)
  Foundation:   20  (classic Clavinet patches)
  Deep:         15  (sub-frequency emphasis, dark wah)
  Flux:         10  (auto-wah sweeps, LFO modulation)
  Coupling:      8  (paired membrane interaction)
  Crystalline:   6  (bright clean, no wah, bell-like clav)

Output: Presets/XOceanus/{mood}/Onkolo/{name}.xometa
"""

import json
import os

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
PRESET_BASE = os.path.join(REPO_ROOT, "Presets", "XOceanus")


def p(
    funk, pickup, brightness, clunk,
    attack, decay, sustain, release,
    filter_env_amt, migration, bend_range,
    macro_character, macro_movement, macro_coupling, macro_space,
    lfo1_rate, lfo1_depth, lfo1_shape,
    lfo2_rate, lfo2_depth, lfo2_shape,
):
    """Return a fully-specified Onkolo parameter dict (all 21 params)."""
    return {
        "onko_funk": funk,
        "onko_pickup": pickup,
        "onko_brightness": brightness,
        "onko_clunk": clunk,
        "onko_attack": attack,
        "onko_decay": decay,
        "onko_sustain": sustain,
        "onko_release": release,
        "onko_filterEnvAmt": filter_env_amt,
        "onko_migration": migration,
        "onko_bendRange": bend_range,
        "onko_macroCharacter": macro_character,
        "onko_macroMovement": macro_movement,
        "onko_macroCoupling": macro_coupling,
        "onko_macroSpace": macro_space,
        "onko_lfo1Rate": lfo1_rate,
        "onko_lfo1Depth": lfo1_depth,
        "onko_lfo1Shape": lfo1_shape,
        "onko_lfo2Rate": lfo2_rate,
        "onko_lfo2Depth": lfo2_depth,
        "onko_lfo2Shape": lfo2_shape,
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


def make_preset(
    name, mood, description, tags, params, sonic_dna,
    coupling_intensity="None", coupling_pairs=None,
):
    return {
        "schema_version": 1,
        "name": name,
        "mood": mood,
        "engines": ["Onkolo"],
        "author": "XO_OX Designs",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
        "couplingIntensity": coupling_intensity,
        "tempo": None,
        "dna": sonic_dna,
        "parameters": {"Onkolo": params},
        "coupling": {"pairs": coupling_pairs or []},
        "sequencer": None,
    }


# =============================================================================
# PRESET DEFINITIONS
# =============================================================================

PRESETS = []

# ---------------------------------------------------------------------------
# KINETIC (25) — funky staccato, percussive, slap; many high-aggression
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Superstition",
        "Kinetic",
        "Stevie Wonder's clavinet riff incarnate — bridge pickup, heavy wah, punchy attack, aggressive filter envelope. The quintessential Clavinet D6 sound.",
        ["clavinet", "stevie-wonder", "funk", "wah", "kinetic", "classic"],
        p(0.88, 0.92, 11000.0, 0.65, 0.001, 0.25, 0.3, 0.08, 0.88, 0.0, 2,
          0.85, 0.75, 0.0, 0.3, 0.08, 0.04, 0, 4.0, 0.06, 0),
        dna(0.82, 0.35, 0.75, 0.58, 0.28, 0.92),
    ),
    make_preset(
        "Parliament Funk",
        "Kinetic",
        "P-Funk rhythmic clavinet — wide wah sweep, neck-to-bridge pickup blend, syncopated attack. The groove machine.",
        ["clavinet", "parliament", "pfunk", "groove", "kinetic", "funky"],
        p(0.82, 0.75, 9500.0, 0.58, 0.001, 0.3, 0.28, 0.1, 0.82, 0.0, 2,
          0.78, 0.82, 0.0, 0.35, 0.1, 0.0, 0, 3.0, 0.08, 0),
        dna(0.75, 0.42, 0.78, 0.55, 0.32, 0.88),
    ),
    make_preset(
        "Rooftop Slap",
        "Kinetic",
        "Percussive muted clavinet — high clunk, very short decay, slap articulation. Rhythmic and tight.",
        ["clavinet", "slap", "percussive", "muted", "kinetic", "tight"],
        p(0.72, 0.85, 9000.0, 0.85, 0.001, 0.12, 0.1, 0.06, 0.75, 0.0, 2,
          0.72, 0.88, 0.0, 0.22, 0.05, 0.0, 0, 8.0, 0.05, 3),
        dna(0.72, 0.38, 0.82, 0.45, 0.22, 0.85),
    ),
    make_preset(
        "Cactus Funk",
        "Kinetic",
        "Dry, spiky clavinet with an exaggerated attack transient. No sustain, all percussive snap. James Brown session material.",
        ["clavinet", "funk", "dry", "percussive", "attack", "kinetic"],
        p(0.65, 0.88, 10500.0, 0.72, 0.001, 0.18, 0.08, 0.05, 0.7, 0.0, 2,
          0.65, 0.85, 0.0, 0.2, 0.06, 0.0, 0, 6.0, 0.04, 0),
        dna(0.78, 0.32, 0.85, 0.42, 0.2, 0.87),
    ),
    make_preset(
        "Hendrix Clav",
        "Kinetic",
        "Band of Gypsys clavinet energy — aggressive wah, raw tonal character, dirty midrange bite. Psychedelic funk.",
        ["clavinet", "hendrix", "psychedelic", "wah", "aggressive", "kinetic"],
        p(0.92, 0.8, 7500.0, 0.6, 0.001, 0.22, 0.2, 0.1, 0.85, 0.0, 4,
          0.88, 0.72, 0.0, 0.32, 0.08, 0.06, 0, 3.5, 0.08, 1),
        dna(0.65, 0.48, 0.72, 0.52, 0.3, 0.91),
    ),
    make_preset(
        "Shaft Driver",
        "Kinetic",
        "Isaac Hayes / David Porter groove clavinet — driving wah, medium sustain, authoritative presence. Classic soul-funk session.",
        ["clavinet", "shaft", "soul", "funk", "groove", "kinetic"],
        p(0.78, 0.82, 9800.0, 0.55, 0.001, 0.28, 0.32, 0.12, 0.78, 0.0, 2,
          0.72, 0.78, 0.0, 0.38, 0.12, 0.04, 0, 2.5, 0.06, 0),
        dna(0.72, 0.42, 0.72, 0.58, 0.38, 0.82),
    ),
    make_preset(
        "Keyboard Snake",
        "Kinetic",
        "Sneaky, low pickup position clavinet with tight wah and a rhythmic LFO bite. Urban funk.",
        ["clavinet", "snake", "urban", "funky", "rhythmic", "kinetic"],
        p(0.75, 0.65, 8500.0, 0.62, 0.001, 0.2, 0.18, 0.08, 0.72, 0.0, 2,
          0.68, 0.82, 0.0, 0.28, 0.16, 0.05, 0, 4.0, 0.07, 0),
        dna(0.68, 0.45, 0.78, 0.52, 0.28, 0.82),
    ),
    make_preset(
        "Chaka Force",
        "Kinetic",
        "Dance-floor clavinet in the Chaka Khan tradition — bright tone, tight attack, the filter envelope makes every note an event.",
        ["clavinet", "chaka-khan", "dance", "bright", "kinetic", "filter"],
        p(0.8, 0.9, 12000.0, 0.58, 0.001, 0.22, 0.25, 0.09, 0.82, 0.0, 2,
          0.8, 0.75, 0.0, 0.42, 0.1, 0.0, 0, 5.0, 0.05, 0),
        dna(0.82, 0.32, 0.75, 0.5, 0.32, 0.85),
    ),
    make_preset(
        "Slick Mute",
        "Kinetic",
        "Half-muted clavinet stroke — the palm damping of the physical D6 translated to clunk and short decay. Pure rhythm instrument.",
        ["clavinet", "muted", "slick", "rhythm", "staccato", "kinetic"],
        p(0.55, 0.88, 8000.0, 0.9, 0.001, 0.1, 0.06, 0.05, 0.65, 0.0, 2,
          0.6, 0.9, 0.0, 0.2, 0.04, 0.0, 0, 8.0, 0.03, 3),
        dna(0.65, 0.38, 0.88, 0.38, 0.18, 0.78),
    ),
    make_preset(
        "Voltage Stab",
        "Kinetic",
        "High-energy clavinet stab — extremely fast attack and decay, maximum wah depth, bridge pickup snap. For stabs in dense mixes.",
        ["clavinet", "stab", "voltage", "wah", "bridge", "kinetic"],
        p(0.9, 0.95, 13000.0, 0.5, 0.001, 0.08, 0.05, 0.04, 0.9, 0.0, 2,
          0.9, 0.88, 0.0, 0.22, 0.08, 0.0, 0, 6.0, 0.04, 0),
        dna(0.88, 0.28, 0.9, 0.4, 0.2, 0.95),
    ),
    make_preset(
        "Bootsy Thumb",
        "Kinetic",
        "Clavinet as bass-adjacent instrument — mid pickup, extended decay, thick low-end presence. Bootsy-flavored rhythm clavinet.",
        ["clavinet", "bootsy", "bass", "thick", "groove", "kinetic"],
        p(0.68, 0.55, 6000.0, 0.7, 0.001, 0.45, 0.35, 0.15, 0.72, 0.0, 2,
          0.65, 0.7, 0.0, 0.42, 0.14, 0.04, 0, 2.0, 0.08, 0),
        dna(0.52, 0.58, 0.68, 0.58, 0.38, 0.78),
    ),
    make_preset(
        "Detroit Pluck",
        "Kinetic",
        "Motown clavinet snap — tight, precise, forward in the mix. The backbone of classic Detroit recordings.",
        ["clavinet", "motown", "detroit", "snap", "classic", "kinetic"],
        p(0.62, 0.85, 10000.0, 0.65, 0.001, 0.2, 0.22, 0.08, 0.68, 0.0, 2,
          0.62, 0.72, 0.0, 0.35, 0.1, 0.0, 0, 4.0, 0.04, 0),
        dna(0.72, 0.38, 0.72, 0.48, 0.32, 0.75),
    ),
    make_preset(
        "Acid Clav",
        "Kinetic",
        "Clavinet meets acid filter — auto-wah deep, resonance pushed to the edge, rapid LFO. The D6 through a mutated filter.",
        ["clavinet", "acid", "filter", "wah", "lfo", "kinetic"],
        p(0.95, 0.78, 8000.0, 0.5, 0.001, 0.2, 0.22, 0.1, 0.92, 0.0, 4,
          0.88, 0.8, 0.0, 0.35, 0.2, 0.12, 0, 5.0, 0.1, 1),
        dna(0.68, 0.38, 0.82, 0.52, 0.3, 0.9),
    ),
    make_preset(
        "Hard Hits",
        "Kinetic",
        "Maximum velocity clavinet — high clunk, bridge pickup, full filter envelope attack. Playing hard makes this instrument speak.",
        ["clavinet", "velocity", "hard", "dynamic", "percussive", "kinetic"],
        p(0.72, 0.92, 11000.0, 0.78, 0.001, 0.15, 0.12, 0.06, 0.78, 0.0, 2,
          0.78, 0.85, 0.0, 0.25, 0.07, 0.0, 0, 7.0, 0.04, 0),
        dna(0.78, 0.32, 0.85, 0.42, 0.22, 0.88),
    ),
    make_preset(
        "Swamp Riff",
        "Kinetic",
        "Murky, southern-fried clavinet — mid wah, warm pickup position, elongated decay for a loose swampy feel.",
        ["clavinet", "swamp", "southern", "wah", "warm", "kinetic"],
        p(0.7, 0.6, 7000.0, 0.62, 0.001, 0.38, 0.28, 0.14, 0.72, 0.0, 2,
          0.65, 0.62, 0.0, 0.45, 0.16, 0.06, 0, 1.5, 0.08, 0),
        dna(0.6, 0.52, 0.65, 0.55, 0.42, 0.72),
    ),
    make_preset(
        "Scratch Clav",
        "Kinetic",
        "DJ scratch-like clavinet stabs — extreme filter envelope, very short notes, high clunk. Sounds like vinyl manipulation.",
        ["clavinet", "scratch", "dj", "stabs", "vinyl", "kinetic"],
        p(0.88, 0.88, 9500.0, 0.82, 0.001, 0.06, 0.04, 0.04, 0.88, 0.0, 2,
          0.85, 0.92, 0.0, 0.22, 0.05, 0.0, 0, 9.0, 0.04, 3),
        dna(0.75, 0.3, 0.92, 0.38, 0.18, 0.9),
    ),
    make_preset(
        "Cuban Montuno",
        "Kinetic",
        "Latin clavinet for montuno patterns — bright, articulate, mid-neck pickup, moderate wah. The salsa-funk crossover.",
        ["clavinet", "cuba", "montuno", "latin", "salsa", "kinetic"],
        p(0.58, 0.72, 10500.0, 0.5, 0.001, 0.25, 0.3, 0.1, 0.65, 0.0, 2,
          0.62, 0.68, 0.0, 0.38, 0.12, 0.0, 0, 3.0, 0.04, 0),
        dna(0.75, 0.38, 0.72, 0.5, 0.35, 0.72),
    ),
    make_preset(
        "Anvil Groove",
        "Kinetic",
        "Heavy-hitting rhythmic clavinet with iron-hard attack transients. Industrial funk — all weight and forward momentum.",
        ["clavinet", "heavy", "industrial", "groove", "attack", "kinetic"],
        p(0.78, 0.9, 9000.0, 0.75, 0.001, 0.18, 0.15, 0.07, 0.8, 0.0, 2,
          0.82, 0.88, 0.0, 0.28, 0.08, 0.0, 0, 5.0, 0.05, 0),
        dna(0.7, 0.35, 0.82, 0.48, 0.25, 0.88),
    ),
    make_preset(
        "Keyboard Stomp",
        "Kinetic",
        "Staccato clavinet attack with high mechanical clunk — you feel the key mechanism as much as the tone. Maximum kinetic energy.",
        ["clavinet", "stomp", "staccato", "clunk", "mechanical", "kinetic"],
        p(0.65, 0.85, 9500.0, 0.88, 0.001, 0.14, 0.1, 0.05, 0.72, 0.0, 2,
          0.72, 0.88, 0.0, 0.22, 0.06, 0.0, 0, 7.5, 0.04, 3),
        dna(0.7, 0.35, 0.88, 0.42, 0.2, 0.82),
    ),
    make_preset(
        "Wah Machine",
        "Kinetic",
        "Pure auto-wah clavinet — maximum funk depth, every note a wah sweep. The most extreme filter-envelope preset in the collection.",
        ["clavinet", "wah", "auto-wah", "filter", "extreme", "kinetic"],
        p(1.0, 0.85, 10000.0, 0.55, 0.001, 0.22, 0.22, 0.1, 0.95, 0.0, 2,
          0.95, 0.8, 0.0, 0.32, 0.12, 0.08, 0, 4.0, 0.08, 1),
        dna(0.75, 0.35, 0.82, 0.52, 0.3, 0.95),
    ),
    make_preset(
        "Night Caller",
        "Kinetic",
        "Late-night funk clavinet with a smoky edge — moderate wah, warm pickup blend, persistent groove. City streets at 2am.",
        ["clavinet", "night", "smoky", "funk", "groove", "kinetic"],
        p(0.72, 0.68, 8000.0, 0.58, 0.001, 0.32, 0.25, 0.12, 0.72, 0.0, 2,
          0.68, 0.72, 0.0, 0.42, 0.14, 0.05, 0, 2.0, 0.07, 0),
        dna(0.62, 0.48, 0.68, 0.55, 0.38, 0.75),
    ),
    make_preset(
        "Funk Injection",
        "Kinetic",
        "The funk injection moment — aggressive wah hits, bright bridge pickup, all forward motion. One-bar riff loops forever.",
        ["clavinet", "injection", "funk", "aggressive", "wah", "kinetic"],
        p(0.85, 0.88, 11500.0, 0.62, 0.001, 0.2, 0.2, 0.08, 0.85, 0.0, 2,
          0.85, 0.82, 0.0, 0.3, 0.1, 0.06, 0, 4.5, 0.07, 0),
        dna(0.8, 0.35, 0.8, 0.52, 0.28, 0.88),
    ),
    make_preset(
        "Slap Shot",
        "Kinetic",
        "Instant attack, zero sustain — the clavinet as pure percussive slap. Staccato playing only. Enormous rhythmic potential.",
        ["clavinet", "slap", "staccato", "percussive", "zero-sustain", "kinetic"],
        p(0.7, 0.88, 10000.0, 0.78, 0.001, 0.07, 0.0, 0.04, 0.7, 0.0, 2,
          0.72, 0.92, 0.0, 0.2, 0.04, 0.0, 0, 9.0, 0.03, 3),
        dna(0.72, 0.3, 0.92, 0.35, 0.18, 0.85),
    ),
    make_preset(
        "Jam Session",
        "Kinetic",
        "Live jam clavinet feel — moderate wah, LFO movement adds human imperfection, medium clunk keeps the groove alive.",
        ["clavinet", "jam", "live", "groove", "session", "kinetic"],
        p(0.62, 0.72, 8500.0, 0.6, 0.001, 0.28, 0.28, 0.12, 0.68, 0.0, 2,
          0.65, 0.68, 0.0, 0.4, 0.18, 0.06, 1, 2.5, 0.06, 0),
        dna(0.68, 0.42, 0.65, 0.52, 0.38, 0.72),
    ),
    make_preset(
        "Street Level",
        "Kinetic",
        "Street-level urban clavinet — aggressive, upfront, zero pretension. The sound of the block.",
        ["clavinet", "street", "urban", "aggressive", "upfront", "kinetic"],
        p(0.8, 0.9, 10000.0, 0.68, 0.001, 0.18, 0.15, 0.07, 0.82, 0.0, 2,
          0.8, 0.85, 0.0, 0.25, 0.09, 0.0, 0, 5.0, 0.05, 0),
        dna(0.75, 0.35, 0.78, 0.48, 0.25, 0.85),
    ),
]

# ---------------------------------------------------------------------------
# ORGANIC (20) — diaspora character, natural wood, acoustic reference
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Thumb Piano",
        "Organic",
        "The clavinet heard as an electronic kalimba — soft attack, natural decay, neck pickup warmth. African diaspora resonance.",
        ["clavinet", "kalimba", "thumb-piano", "africa", "diaspora", "organic"],
        p(0.12, 0.25, 4000.0, 0.25, 0.003, 0.8, 0.35, 0.45, 0.2, 0.0, 2,
          0.3, 0.15, 0.0, 0.55, 0.05, 0.04, 1, 0.08, 0.06, 0),
        dna(0.32, 0.72, 0.22, 0.45, 0.55, 0.2),
    ),
    make_preset(
        "Kora Strings",
        "Organic",
        "West African kora influence on the clavinet — light touch, harp-like decay, warm pickup position, organic tremolo.",
        ["clavinet", "kora", "west-africa", "harp", "diaspora", "organic"],
        p(0.08, 0.2, 3500.0, 0.15, 0.002, 1.2, 0.4, 0.6, 0.12, 0.0, 2,
          0.25, 0.12, 0.0, 0.6, 0.06, 0.08, 1, 0.05, 0.05, 0),
        dna(0.28, 0.78, 0.2, 0.4, 0.62, 0.12),
    ),
    make_preset(
        "Wood Body",
        "Organic",
        "The natural wood resonance of the D6 body — no wah, warm tone, mechanical clunk preserved. The instrument unadorned.",
        ["clavinet", "wood", "natural", "acoustic", "unprocessed", "organic"],
        p(0.0, 0.45, 5000.0, 0.42, 0.001, 0.55, 0.4, 0.3, 0.0, 0.0, 2,
          0.35, 0.0, 0.0, 0.48, 0.08, 0.05, 1, 0.04, 0.03, 0),
        dna(0.42, 0.72, 0.08, 0.5, 0.48, 0.25),
    ),
    make_preset(
        "Soul Village",
        "Organic",
        "Village soul clavinet — gentle wah, mid-neck pickup, generous decay. Community music, not performance.",
        ["clavinet", "soul", "village", "gentle", "warm", "organic"],
        p(0.22, 0.4, 6000.0, 0.32, 0.002, 0.65, 0.38, 0.28, 0.28, 0.0, 2,
          0.35, 0.2, 0.0, 0.5, 0.1, 0.04, 0, 0.06, 0.05, 0),
        dna(0.45, 0.65, 0.18, 0.48, 0.5, 0.32),
    ),
    make_preset(
        "Night Market",
        "Organic",
        "Clavinet heard across a night market — natural decay, light touch, warm atmosphere. The everyday music of diaspora life.",
        ["clavinet", "night-market", "diaspora", "warm", "atmospheric", "organic"],
        p(0.18, 0.35, 5500.0, 0.28, 0.002, 0.72, 0.42, 0.35, 0.2, 0.0, 2,
          0.3, 0.12, 0.0, 0.52, 0.08, 0.04, 1, 0.05, 0.04, 0),
        dna(0.4, 0.68, 0.15, 0.45, 0.52, 0.22),
    ),
    make_preset(
        "Nkosi Groove",
        "Organic",
        "South African township clavinet — moderate wah, natural warmth, mbaqanga influence. The clavinet in a Soweto setting.",
        ["clavinet", "south-africa", "township", "mbaqanga", "diaspora", "organic"],
        p(0.42, 0.52, 7000.0, 0.38, 0.001, 0.45, 0.35, 0.2, 0.45, 0.0, 2,
          0.45, 0.38, 0.0, 0.45, 0.12, 0.04, 0, 0.08, 0.05, 0),
        dna(0.52, 0.62, 0.35, 0.5, 0.45, 0.45),
    ),
    make_preset(
        "Amber Key",
        "Organic",
        "Warm amber clavinet tone — minimal wah, neck pickup dominant, long natural decay. Like a vintage key under warm light.",
        ["clavinet", "amber", "warm", "vintage", "neck", "organic"],
        p(0.05, 0.15, 4500.0, 0.3, 0.002, 0.9, 0.45, 0.5, 0.08, 0.0, 2,
          0.28, 0.0, 0.0, 0.55, 0.06, 0.04, 1, 0.04, 0.03, 0),
        dna(0.35, 0.78, 0.1, 0.42, 0.55, 0.15),
    ),
    make_preset(
        "Roots Strum",
        "Organic",
        "Reggae-roots clavinet strum — relaxed tempo, mid wah, warm pickup. The upstroke complement to a roots bassline.",
        ["clavinet", "reggae", "roots", "strum", "warm", "organic"],
        p(0.38, 0.48, 6500.0, 0.4, 0.001, 0.38, 0.32, 0.18, 0.42, 0.0, 2,
          0.4, 0.32, 0.0, 0.48, 0.14, 0.04, 0, 0.1, 0.05, 0),
        dna(0.5, 0.6, 0.32, 0.48, 0.45, 0.42),
    ),
    make_preset(
        "Balafon Echo",
        "Organic",
        "Balafon-influenced clavinet decay — marimba-like tone envelope, warm resonance, light wah. West African resonator percussion as clavinet.",
        ["clavinet", "balafon", "marimba", "west-africa", "resonator", "organic"],
        p(0.15, 0.32, 5000.0, 0.22, 0.002, 0.7, 0.28, 0.4, 0.18, 0.0, 2,
          0.28, 0.1, 0.0, 0.55, 0.07, 0.06, 1, 0.06, 0.05, 0),
        dna(0.38, 0.65, 0.15, 0.45, 0.52, 0.22),
    ),
    make_preset(
        "String Garden",
        "Organic",
        "Clavinet as stringed instrument — bow-like decay, gentle wah arc, natural warmth. The plant kingdom as D6.",
        ["clavinet", "strings", "garden", "natural", "decay", "organic"],
        p(0.1, 0.28, 4800.0, 0.18, 0.004, 1.1, 0.48, 0.55, 0.12, 0.0, 2,
          0.22, 0.05, 0.0, 0.58, 0.07, 0.05, 1, 0.04, 0.04, 0),
        dna(0.3, 0.75, 0.12, 0.42, 0.58, 0.12),
    ),
    make_preset(
        "Desert Call",
        "Organic",
        "North African gnawa-influenced clavinet — pentatonic feel implied by clunk rhythm, warm tone, minimal wah. The Sahara edge.",
        ["clavinet", "gnawa", "north-africa", "desert", "pentatonic", "organic"],
        p(0.2, 0.38, 5800.0, 0.45, 0.001, 0.55, 0.35, 0.25, 0.22, 0.0, 2,
          0.35, 0.18, 0.0, 0.48, 0.1, 0.04, 0, 0.06, 0.04, 0),
        dna(0.45, 0.62, 0.2, 0.48, 0.48, 0.35),
    ),
    make_preset(
        "Heartwood",
        "Organic",
        "Deep within the wood — neck pickup, zero wah, slow decay like tree rings. The oldest clavinet possible.",
        ["clavinet", "wood", "neck", "deep", "natural", "organic"],
        p(0.0, 0.08, 3200.0, 0.35, 0.003, 1.5, 0.5, 0.7, 0.0, 0.0, 2,
          0.2, 0.0, 0.0, 0.6, 0.05, 0.04, 1, 0.03, 0.03, 0),
        dna(0.22, 0.85, 0.08, 0.38, 0.62, 0.1),
    ),
    make_preset(
        "Street Piano",
        "Organic",
        "Open-air busker clavinet — outdoor character, natural dynamics, light wah, generous decay. The public instrument.",
        ["clavinet", "busker", "street", "outdoor", "natural", "organic"],
        p(0.25, 0.42, 6200.0, 0.35, 0.002, 0.6, 0.38, 0.32, 0.28, 0.0, 2,
          0.38, 0.2, 0.0, 0.5, 0.1, 0.04, 0, 0.07, 0.04, 0),
        dna(0.48, 0.62, 0.18, 0.45, 0.5, 0.32),
    ),
    make_preset(
        "Juju Touch",
        "Organic",
        "Nigerian juju music clavinet influence — King Sunny Ade territory. Bright but warm, light syncopation implied, talking drum sibling.",
        ["clavinet", "juju", "nigeria", "africa", "king-sunny-ade", "organic"],
        p(0.35, 0.55, 7500.0, 0.42, 0.001, 0.42, 0.32, 0.2, 0.38, 0.0, 2,
          0.42, 0.28, 0.0, 0.42, 0.12, 0.04, 0, 0.08, 0.05, 0),
        dna(0.55, 0.58, 0.3, 0.5, 0.42, 0.45),
    ),
    make_preset(
        "Smoke and Grain",
        "Organic",
        "Vintage clavinet through an old tape path — warm grain, light wah flutter, neck-dominant. The aged instrument.",
        ["clavinet", "vintage", "tape", "grain", "warm", "organic"],
        p(0.15, 0.22, 4200.0, 0.28, 0.003, 0.85, 0.42, 0.45, 0.15, 0.0, 2,
          0.25, 0.08, 0.0, 0.55, 0.09, 0.06, 1, 0.05, 0.05, 0),
        dna(0.3, 0.78, 0.12, 0.45, 0.52, 0.15),
    ),
    make_preset(
        "Afro Jazz",
        "Organic",
        "Hugh Masekela / Dollar Brand intersection — clavinet as jazz-African crossover. Relaxed wah, open tone, conversational.",
        ["clavinet", "afro-jazz", "masekela", "dollar-brand", "africa", "organic"],
        p(0.3, 0.48, 6800.0, 0.38, 0.002, 0.5, 0.38, 0.25, 0.32, 0.0, 2,
          0.38, 0.22, 0.0, 0.48, 0.12, 0.04, 0, 0.08, 0.05, 0),
        dna(0.5, 0.62, 0.25, 0.48, 0.48, 0.38),
    ),
    make_preset(
        "Cane Field",
        "Organic",
        "Caribbean cane-field rhythms in clavinet form — light wah, mid pickup, calypso-reggae feel. Tropical diaspora.",
        ["clavinet", "caribbean", "calypso", "tropical", "diaspora", "organic"],
        p(0.28, 0.45, 6500.0, 0.35, 0.001, 0.42, 0.3, 0.18, 0.3, 0.0, 2,
          0.38, 0.28, 0.0, 0.48, 0.12, 0.04, 0, 0.1, 0.05, 0),
        dna(0.52, 0.58, 0.28, 0.48, 0.48, 0.38),
    ),
    make_preset(
        "Bamboo Key",
        "Organic",
        "Imagining the clavinet built from bamboo instead of wood — lighter, resonant, a touch percussive, Southeast Asian spirit.",
        ["clavinet", "bamboo", "asian", "resonant", "light", "organic"],
        p(0.08, 0.3, 5200.0, 0.32, 0.002, 0.65, 0.35, 0.38, 0.1, 0.0, 2,
          0.28, 0.08, 0.0, 0.52, 0.08, 0.05, 1, 0.05, 0.04, 0),
        dna(0.42, 0.65, 0.12, 0.42, 0.52, 0.18),
    ),
    make_preset(
        "Field Recording",
        "Organic",
        "Clavinet like a field recording of an acoustic instrument — honest, unprocessed character, natural dynamics.",
        ["clavinet", "field-recording", "honest", "natural", "acoustic", "organic"],
        p(0.02, 0.38, 5800.0, 0.3, 0.002, 0.7, 0.4, 0.38, 0.04, 0.0, 2,
          0.25, 0.0, 0.0, 0.5, 0.07, 0.03, 1, 0.04, 0.03, 0),
        dna(0.42, 0.72, 0.08, 0.42, 0.5, 0.15),
    ),
    make_preset(
        "Griot's Voice",
        "Organic",
        "West African griot storytelling through clavinet — moderate wah like a talking drum, warm register, narrative decay.",
        ["clavinet", "griot", "west-africa", "talking-drum", "narrative", "organic"],
        p(0.42, 0.4, 6200.0, 0.42, 0.002, 0.55, 0.35, 0.28, 0.45, 0.0, 2,
          0.4, 0.3, 0.0, 0.48, 0.12, 0.05, 1, 0.08, 0.05, 0),
        dna(0.48, 0.62, 0.28, 0.5, 0.48, 0.42),
    ),
]

# ---------------------------------------------------------------------------
# FOUNDATION (20) — classic Clavinet D6 patches, reference tones
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "D6 Reference",
        "Foundation",
        "The stock Clavinet D6 with no augmentation — middle pickup position, moderate wah, factory voice. The benchmark.",
        ["clavinet", "reference", "stock", "classic", "d6", "foundation"],
        p(0.5, 0.7, 8000.0, 0.5, 0.001, 0.3, 0.4, 0.15, 0.6, 0.0, 2,
          0.5, 0.5, 0.0, 0.5, 0.1, 0.0, 0, 1.0, 0.0, 0),
        dna(0.62, 0.48, 0.35, 0.5, 0.42, 0.55),
    ),
    make_preset(
        "Clav Noir",
        "Foundation",
        "Noir clavinet for late-night recordings — dark brightness, low wah, extended sustain. The sophisticated side of the D6.",
        ["clavinet", "noir", "dark", "sophisticated", "late-night", "foundation"],
        p(0.25, 0.35, 3800.0, 0.4, 0.002, 0.6, 0.5, 0.3, 0.28, 0.0, 2,
          0.35, 0.12, 0.0, 0.55, 0.08, 0.04, 1, 0.06, 0.04, 0),
        dna(0.28, 0.65, 0.15, 0.5, 0.52, 0.32),
    ),
    make_preset(
        "Neck Pure",
        "Foundation",
        "Pure neck pickup clavinet — maximum warmth, minimum brightness, the low-end character of the instrument.",
        ["clavinet", "neck", "warm", "pure", "low-end", "foundation"],
        p(0.3, 0.0, 5000.0, 0.35, 0.001, 0.45, 0.42, 0.22, 0.32, 0.0, 2,
          0.35, 0.15, 0.0, 0.52, 0.1, 0.0, 0, 0.8, 0.0, 0),
        dna(0.38, 0.72, 0.12, 0.48, 0.5, 0.32),
    ),
    make_preset(
        "Bridge Snap",
        "Foundation",
        "Pure bridge pickup clavinet — maximum brightness and attack, minimum warmth. The other end of the D6 tonal spectrum.",
        ["clavinet", "bridge", "bright", "snap", "attack", "foundation"],
        p(0.45, 1.0, 14000.0, 0.6, 0.001, 0.18, 0.2, 0.08, 0.55, 0.0, 2,
          0.65, 0.35, 0.0, 0.35, 0.08, 0.0, 0, 1.5, 0.0, 0),
        dna(0.88, 0.22, 0.35, 0.45, 0.32, 0.62),
    ),
    make_preset(
        "Vintage Sixties",
        "Foundation",
        "1960s session clavinet sound — before heavy wah was fashionable. Clean, percussive, authentic early D6 recordings.",
        ["clavinet", "vintage", "sixties", "session", "classic", "foundation"],
        p(0.18, 0.6, 9000.0, 0.55, 0.001, 0.28, 0.35, 0.14, 0.2, 0.0, 2,
          0.4, 0.15, 0.0, 0.45, 0.08, 0.0, 0, 0.5, 0.0, 0),
        dna(0.65, 0.5, 0.2, 0.48, 0.42, 0.5),
    ),
    make_preset(
        "Seventies Session",
        "Foundation",
        "Peak 1970s studio clavinet — the wah fully embraced, bridge pickup, clean production. Records from that era.",
        ["clavinet", "seventies", "studio", "wah", "session", "foundation"],
        p(0.65, 0.82, 10000.0, 0.52, 0.001, 0.25, 0.3, 0.1, 0.7, 0.0, 2,
          0.62, 0.55, 0.0, 0.42, 0.08, 0.0, 0, 1.0, 0.0, 0),
        dna(0.72, 0.4, 0.35, 0.5, 0.38, 0.68),
    ),
    make_preset(
        "Dry Room",
        "Foundation",
        "Clavinet in a dry recording room — no reverb coloring, direct signal, the pure D6 in studio daylight.",
        ["clavinet", "dry", "studio", "direct", "pure", "foundation"],
        p(0.4, 0.65, 8500.0, 0.48, 0.001, 0.3, 0.38, 0.12, 0.45, 0.0, 2,
          0.45, 0.25, 0.0, 0.42, 0.08, 0.0, 0, 0.8, 0.0, 0),
        dna(0.62, 0.45, 0.2, 0.48, 0.35, 0.55),
    ),
    make_preset(
        "Gospel Foundation",
        "Foundation",
        "Gospel keyboard in the soul-funk tradition — clavinet as church instrument, moderate wah, spiritual presence.",
        ["clavinet", "gospel", "church", "soul", "spiritual", "foundation"],
        p(0.45, 0.58, 7500.0, 0.42, 0.001, 0.38, 0.42, 0.18, 0.48, 0.0, 2,
          0.45, 0.35, 0.0, 0.5, 0.1, 0.04, 0, 0.5, 0.04, 0),
        dna(0.55, 0.55, 0.25, 0.52, 0.45, 0.52),
    ),
    make_preset(
        "Session Double",
        "Foundation",
        "The classic double-tracked clavinet studio technique — slightly different pickup positions create natural width.",
        ["clavinet", "double", "session", "studio", "wide", "foundation"],
        p(0.55, 0.62, 8800.0, 0.5, 0.001, 0.28, 0.35, 0.12, 0.6, 0.0, 2,
          0.55, 0.4, 0.0, 0.45, 0.08, 0.05, 0, 0.6, 0.03, 0),
        dna(0.65, 0.45, 0.28, 0.52, 0.42, 0.58),
    ),
    make_preset(
        "Felt Damper",
        "Foundation",
        "Felt-damped clavinet variation — softer transient, reduced brightness, intimate character. The quieter D6.",
        ["clavinet", "felt", "damped", "soft", "intimate", "foundation"],
        p(0.22, 0.4, 5500.0, 0.62, 0.002, 0.5, 0.4, 0.25, 0.25, 0.0, 2,
          0.32, 0.1, 0.0, 0.52, 0.08, 0.04, 1, 0.05, 0.03, 0),
        dna(0.4, 0.62, 0.15, 0.5, 0.48, 0.3),
    ),
    make_preset(
        "Two Pickup Blend",
        "Foundation",
        "Equal blend of neck and bridge pickups — the center position on the D6. Balanced tonal character, versatile.",
        ["clavinet", "blend", "balanced", "versatile", "center", "foundation"],
        p(0.4, 0.5, 7500.0, 0.45, 0.001, 0.32, 0.38, 0.14, 0.45, 0.0, 2,
          0.48, 0.28, 0.0, 0.48, 0.09, 0.0, 0, 0.8, 0.0, 0),
        dna(0.58, 0.5, 0.18, 0.5, 0.42, 0.5),
    ),
    make_preset(
        "Producer Ready",
        "Foundation",
        "The preset a session player loads and keeps — moderate wah, medium pickup, everything in the right proportion.",
        ["clavinet", "session", "producer", "balanced", "ready", "foundation"],
        p(0.52, 0.68, 8200.0, 0.5, 0.001, 0.28, 0.35, 0.12, 0.58, 0.0, 2,
          0.55, 0.42, 0.0, 0.45, 0.08, 0.0, 0, 0.8, 0.0, 0),
        dna(0.62, 0.45, 0.22, 0.5, 0.42, 0.58),
    ),
    make_preset(
        "Rhodes Cousin",
        "Foundation",
        "The clavinet and Rhodes are cousins — this patch leans toward the family resemblance. Warmer tone, reduced wah, longer tail.",
        ["clavinet", "rhodes", "ep", "warm", "family", "foundation"],
        p(0.18, 0.32, 6000.0, 0.3, 0.002, 0.75, 0.48, 0.38, 0.22, 0.0, 2,
          0.38, 0.12, 0.0, 0.55, 0.08, 0.04, 0, 0.6, 0.04, 0),
        dna(0.45, 0.65, 0.15, 0.48, 0.52, 0.28),
    ),
    make_preset(
        "Daylight Classic",
        "Foundation",
        "Open, bright daytime clavinet — the cheerful side of the D6 without being aggressive.",
        ["clavinet", "bright", "open", "daylight", "cheerful", "foundation"],
        p(0.38, 0.72, 10000.0, 0.42, 0.001, 0.25, 0.32, 0.12, 0.42, 0.0, 2,
          0.52, 0.32, 0.0, 0.42, 0.08, 0.0, 0, 0.8, 0.0, 0),
        dna(0.72, 0.42, 0.22, 0.48, 0.38, 0.52),
    ),
    make_preset(
        "Chicago Clav",
        "Foundation",
        "Chicago blues-influenced clavinet — the D6 in the hands of Buddy Guy's keyboard player. Gritty warmth, controlled wah.",
        ["clavinet", "chicago", "blues", "gritty", "warm", "foundation"],
        p(0.42, 0.55, 7000.0, 0.52, 0.001, 0.38, 0.38, 0.18, 0.45, 0.0, 2,
          0.45, 0.32, 0.0, 0.48, 0.1, 0.04, 0, 0.5, 0.04, 0),
        dna(0.52, 0.58, 0.25, 0.52, 0.45, 0.52),
    ),
    make_preset(
        "British Clav",
        "Foundation",
        "The British R&B approach to clavinet — more restrained wah, clean attack, sophisticated tonal balance.",
        ["clavinet", "british", "rb", "clean", "restrained", "foundation"],
        p(0.32, 0.65, 8500.0, 0.42, 0.001, 0.3, 0.38, 0.14, 0.35, 0.0, 2,
          0.45, 0.28, 0.0, 0.45, 0.08, 0.0, 0, 0.8, 0.0, 0),
        dna(0.65, 0.48, 0.22, 0.5, 0.42, 0.45),
    ),
    make_preset(
        "New York Clean",
        "Foundation",
        "New York session clavinet — clean, professional, no fuss. The sound of 70s and 80s Manhattan recording studios.",
        ["clavinet", "new-york", "session", "clean", "manhattan", "foundation"],
        p(0.48, 0.75, 9000.0, 0.48, 0.001, 0.24, 0.32, 0.12, 0.52, 0.0, 2,
          0.52, 0.38, 0.0, 0.42, 0.08, 0.0, 0, 0.8, 0.0, 0),
        dna(0.68, 0.42, 0.22, 0.5, 0.4, 0.58),
    ),
    make_preset(
        "Funk Convention",
        "Foundation",
        "The conventional funk clavinet preset — moderate-high wah, bridge pickup lean, the building block of all funk keyboard sounds.",
        ["clavinet", "funk", "conventional", "standard", "building-block", "foundation"],
        p(0.7, 0.8, 10000.0, 0.52, 0.001, 0.22, 0.28, 0.1, 0.72, 0.0, 2,
          0.68, 0.6, 0.0, 0.38, 0.08, 0.0, 0, 1.0, 0.0, 0),
        dna(0.72, 0.38, 0.35, 0.5, 0.35, 0.72),
    ),
    make_preset(
        "Lean Machine",
        "Foundation",
        "Lean, stripped-down clavinet — nothing superfluous. Pickup positioned for maximum efficiency, wah controlled, clunk present.",
        ["clavinet", "lean", "stripped", "efficient", "clean", "foundation"],
        p(0.48, 0.72, 9000.0, 0.55, 0.001, 0.22, 0.28, 0.1, 0.52, 0.0, 2,
          0.5, 0.42, 0.0, 0.4, 0.07, 0.0, 0, 0.8, 0.0, 0),
        dna(0.65, 0.42, 0.22, 0.48, 0.38, 0.58),
    ),
    make_preset(
        "Parlour Clav",
        "Foundation",
        "Victorian parlour re-imagined as D6 — no wah, warm neck pickup, gentle clunk. Charming and slightly anachronistic.",
        ["clavinet", "parlour", "victorian", "warm", "gentle", "foundation"],
        p(0.05, 0.18, 4500.0, 0.32, 0.003, 0.75, 0.48, 0.4, 0.08, 0.0, 2,
          0.28, 0.05, 0.0, 0.58, 0.07, 0.04, 1, 0.04, 0.03, 0),
        dna(0.3, 0.72, 0.1, 0.45, 0.55, 0.18),
    ),
]

# ---------------------------------------------------------------------------
# DEEP (15) — sub-frequency emphasis, dark wah, heavy character
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Wah Cathedral",
        "Deep",
        "Deep, resonant clavinet in a cathedral space — slow wah arc, bass-forward tone, cavernous register. Sub-bass presence.",
        ["clavinet", "cathedral", "wah", "deep", "bass", "reverb"],
        p(0.55, 0.18, 2500.0, 0.38, 0.003, 1.2, 0.5, 0.6, 0.58, 0.0, 2,
          0.35, 0.08, 0.0, 0.7, 0.08, 0.06, 0, 0.05, 0.04, 0),
        dna(0.18, 0.78, 0.12, 0.55, 0.72, 0.35),
    ),
    make_preset(
        "Subterranean",
        "Deep",
        "Underground clavinet frequencies — heavy low-end, dark filter, mechanical resonance. The D6 in the earth.",
        ["clavinet", "sub", "underground", "dark", "filter", "deep"],
        p(0.42, 0.15, 2000.0, 0.55, 0.002, 0.9, 0.45, 0.45, 0.45, 0.0, 2,
          0.28, 0.08, 0.0, 0.62, 0.07, 0.05, 0, 0.06, 0.04, 0),
        dna(0.15, 0.82, 0.12, 0.58, 0.65, 0.3),
    ),
    make_preset(
        "Dark Wah",
        "Deep",
        "Dark auto-wah clavinet — the filter envelope moves through bass frequencies, not the usual treble range.",
        ["clavinet", "dark-wah", "bass", "filter", "deep", "heavy"],
        p(0.72, 0.12, 1800.0, 0.48, 0.002, 0.8, 0.42, 0.38, 0.72, 0.0, 2,
          0.32, 0.12, 0.0, 0.62, 0.08, 0.06, 0, 0.08, 0.05, 0),
        dna(0.12, 0.78, 0.18, 0.55, 0.65, 0.42),
    ),
    make_preset(
        "Bass Clav",
        "Deep",
        "Clavinet tuned and voiced as a bass instrument — long decay, heavy low-end, minimal treble. Sub territory.",
        ["clavinet", "bass", "sub", "heavy", "low-end", "deep"],
        p(0.28, 0.08, 2200.0, 0.42, 0.002, 1.4, 0.55, 0.65, 0.3, 0.0, 2,
          0.22, 0.04, 0.0, 0.65, 0.06, 0.04, 0, 0.04, 0.03, 0),
        dna(0.1, 0.85, 0.1, 0.62, 0.68, 0.22),
    ),
    make_preset(
        "Iron Gate",
        "Deep",
        "Heavy iron-gate clavinet — slow filter movement, dark tone, clunk resonates into a cavernous sub-frequency.",
        ["clavinet", "iron", "heavy", "gate", "dark", "deep"],
        p(0.38, 0.2, 2800.0, 0.75, 0.002, 1.0, 0.45, 0.5, 0.4, 0.0, 2,
          0.3, 0.06, 0.0, 0.65, 0.07, 0.05, 0, 0.06, 0.04, 0),
        dna(0.2, 0.8, 0.12, 0.6, 0.68, 0.35),
    ),
    make_preset(
        "Trench Mode",
        "Deep",
        "Maximum depth clavinet — positioned at the lowest tonal register, heavy wah movement in the sub-bass range.",
        ["clavinet", "trench", "depth", "sub", "wah", "deep"],
        p(0.65, 0.1, 1500.0, 0.45, 0.003, 1.2, 0.48, 0.55, 0.65, 0.0, 2,
          0.25, 0.08, 0.0, 0.68, 0.07, 0.06, 0, 0.05, 0.04, 0),
        dna(0.08, 0.82, 0.15, 0.6, 0.72, 0.35),
    ),
    make_preset(
        "Pressure Zone",
        "Deep",
        "The clavinet in a high-pressure acoustic environment — compressed tone, dark filter ceiling, dense presence.",
        ["clavinet", "pressure", "compressed", "dark", "dense", "deep"],
        p(0.32, 0.25, 3200.0, 0.5, 0.002, 0.85, 0.5, 0.42, 0.35, 0.0, 2,
          0.28, 0.08, 0.0, 0.62, 0.07, 0.05, 0, 0.06, 0.04, 0),
        dna(0.22, 0.78, 0.12, 0.6, 0.65, 0.3),
    ),
    make_preset(
        "Dub Foundation",
        "Deep",
        "Clavinet as dub music foundation — deep wah, extended reverb decay, bass-frequency emphasis. Kingston influence.",
        ["clavinet", "dub", "kingston", "bass", "foundation", "deep"],
        p(0.48, 0.2, 2800.0, 0.4, 0.002, 1.0, 0.48, 0.55, 0.52, 0.0, 2,
          0.3, 0.08, 0.0, 0.65, 0.08, 0.05, 0, 0.05, 0.04, 0),
        dna(0.15, 0.8, 0.15, 0.58, 0.7, 0.35),
    ),
    make_preset(
        "Low Rider",
        "Deep",
        "Low-riding clavinet — slow groove, sub emphasis, moderate wah opening slowly. The hydraulic bounce of the instrument.",
        ["clavinet", "low-rider", "sub", "groove", "slow", "deep"],
        p(0.42, 0.22, 3000.0, 0.45, 0.002, 0.95, 0.45, 0.48, 0.45, 0.0, 2,
          0.28, 0.08, 0.0, 0.62, 0.08, 0.04, 0, 0.06, 0.04, 0),
        dna(0.2, 0.78, 0.15, 0.58, 0.65, 0.32),
    ),
    make_preset(
        "Cavern Resonance",
        "Deep",
        "Clavinet resonating in a cave — the D6 body becomes the cave walls. Long mechanical decay, dark overtones.",
        ["clavinet", "cavern", "resonance", "cave", "dark", "deep"],
        p(0.15, 0.15, 2500.0, 0.65, 0.003, 1.5, 0.48, 0.65, 0.18, 0.0, 2,
          0.22, 0.05, 0.0, 0.68, 0.06, 0.06, 0, 0.04, 0.04, 0),
        dna(0.15, 0.82, 0.1, 0.55, 0.72, 0.2),
    ),
    make_preset(
        "Silt Current",
        "Deep",
        "Underwater current clavinet — slow wah like deep-water sediment movement, heavy and unhurried.",
        ["clavinet", "silt", "underwater", "slow", "heavy", "deep"],
        p(0.38, 0.18, 2200.0, 0.42, 0.003, 1.1, 0.45, 0.52, 0.4, 0.0, 2,
          0.25, 0.06, 0.0, 0.65, 0.07, 0.05, 0, 0.05, 0.04, 0),
        dna(0.12, 0.8, 0.12, 0.58, 0.68, 0.28),
    ),
    make_preset(
        "Thunder Root",
        "Deep",
        "Thunderous root-position clavinet — extreme sub emphasis, heavy filter, the earth-shaking side of the D6.",
        ["clavinet", "thunder", "root", "sub", "extreme", "deep"],
        p(0.52, 0.12, 1600.0, 0.55, 0.002, 1.3, 0.5, 0.58, 0.55, 0.0, 2,
          0.28, 0.08, 0.0, 0.65, 0.07, 0.06, 0, 0.05, 0.04, 0),
        dna(0.08, 0.85, 0.12, 0.65, 0.7, 0.38),
    ),
    make_preset(
        "Midnight Wah",
        "Deep",
        "The wah opens slowly in the dark — deep filter sweep, low brightness, late-night sub rumble. Dark funk.",
        ["clavinet", "midnight", "wah", "dark-funk", "slow", "deep"],
        p(0.62, 0.28, 3500.0, 0.45, 0.002, 0.75, 0.42, 0.38, 0.65, 0.0, 2,
          0.32, 0.1, 0.0, 0.6, 0.08, 0.05, 0, 0.06, 0.05, 0),
        dna(0.25, 0.72, 0.18, 0.55, 0.62, 0.42),
    ),
    make_preset(
        "Deep Current",
        "Deep",
        "The D6 as a deep-ocean current — constant, massive, low-frequency movement. Slow oscillation in the depths.",
        ["clavinet", "current", "ocean", "deep", "movement", "bass"],
        p(0.25, 0.15, 2000.0, 0.38, 0.003, 1.2, 0.5, 0.6, 0.28, 0.0, 2,
          0.22, 0.06, 0.0, 0.68, 0.06, 0.04, 0, 0.04, 0.03, 0),
        dna(0.1, 0.82, 0.1, 0.58, 0.72, 0.2),
    ),
    make_preset(
        "Gravity Well",
        "Deep",
        "Clavinet approaching a gravity well — progressively darker, longer decay, the pull toward sub-frequency singularity.",
        ["clavinet", "gravity", "dark", "sub", "decay", "deep"],
        p(0.35, 0.1, 1800.0, 0.48, 0.003, 1.4, 0.52, 0.68, 0.38, 0.0, 2,
          0.2, 0.05, 0.0, 0.68, 0.06, 0.05, 0, 0.04, 0.04, 0),
        dna(0.08, 0.85, 0.1, 0.62, 0.75, 0.22),
    ),
]

# ---------------------------------------------------------------------------
# FLUX (10) — auto-wah sweeps, LFO modulation, movement as identity
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Wah Cascade",
        "Flux",
        "Auto-wah in continuous cascade — LFO modulates filter envelope amount, creating a sweeping, hypnotic wah current.",
        ["clavinet", "wah", "cascade", "lfo", "sweep", "flux"],
        p(0.85, 0.78, 9000.0, 0.52, 0.001, 0.25, 0.25, 0.12, 0.85, 0.0, 2,
          0.78, 0.85, 0.0, 0.38, 0.35, 0.45, 0, 0.12, 0.28, 1),
        dna(0.68, 0.38, 0.82, 0.5, 0.42, 0.72),
    ),
    make_preset(
        "LFO Funk",
        "Flux",
        "Funk clavinet with LFO modulation on filter — the wah pulses at a rhythmic rate, creating automatic groove movement.",
        ["clavinet", "lfo", "funk", "filter", "rhythmic", "flux"],
        p(0.75, 0.82, 9500.0, 0.55, 0.001, 0.22, 0.22, 0.1, 0.78, 0.0, 2,
          0.75, 0.8, 0.0, 0.35, 0.22, 0.35, 0, 4.0, 0.25, 0),
        dna(0.72, 0.38, 0.78, 0.52, 0.38, 0.78),
    ),
    make_preset(
        "Resonance Dance",
        "Flux",
        "The filter resonance dances — LFO2 modulates brightness while LFO1 moves wah depth. The clavinet in perpetual motion.",
        ["clavinet", "resonance", "dance", "lfo", "motion", "flux"],
        p(0.72, 0.72, 8500.0, 0.5, 0.001, 0.28, 0.28, 0.12, 0.75, 0.0, 2,
          0.72, 0.78, 0.0, 0.4, 0.28, 0.38, 1, 3.5, 0.3, 0),
        dna(0.68, 0.42, 0.82, 0.52, 0.42, 0.72),
    ),
    make_preset(
        "Phase Wah",
        "Flux",
        "Phasing wah modulation — the filter envelope and LFO are slightly out of phase, creating organic flux character.",
        ["clavinet", "phase", "wah", "modulation", "organic", "flux"],
        p(0.8, 0.75, 8800.0, 0.52, 0.001, 0.24, 0.24, 0.1, 0.8, 0.0, 2,
          0.78, 0.82, 0.0, 0.38, 0.18, 0.25, 1, 0.25, 0.22, 0),
        dna(0.65, 0.42, 0.78, 0.52, 0.45, 0.72),
    ),
    make_preset(
        "Tremolo Clav",
        "Flux",
        "Classic tremolo-wah clavinet — tremolo amplitude modulation layered with auto-wah filter sweep. Vintage effect combination.",
        ["clavinet", "tremolo", "wah", "vintage", "effect", "flux"],
        p(0.65, 0.68, 8000.0, 0.48, 0.001, 0.3, 0.32, 0.15, 0.68, 0.0, 2,
          0.65, 0.72, 0.0, 0.45, 0.25, 0.55, 1, 5.5, 0.4, 1),
        dna(0.62, 0.45, 0.78, 0.52, 0.45, 0.65),
    ),
    make_preset(
        "Auto Motion",
        "Flux",
        "Automatic motion clavinet — dual LFOs create opposing movements, never settling. The restless D6.",
        ["clavinet", "auto", "motion", "dual-lfo", "restless", "flux"],
        p(0.7, 0.72, 9000.0, 0.5, 0.001, 0.25, 0.25, 0.12, 0.72, 0.0, 2,
          0.72, 0.82, 0.0, 0.4, 0.15, 0.22, 2, 0.37, 0.28, 3),
        dna(0.68, 0.42, 0.85, 0.5, 0.42, 0.68),
    ),
    make_preset(
        "Sine Wah",
        "Flux",
        "Pure sine LFO wah — the cleanest possible filter modulation, perfectly smooth wah oscillation. Mathematical and musical.",
        ["clavinet", "sine", "wah", "lfo", "smooth", "flux"],
        p(0.75, 0.78, 9500.0, 0.5, 0.001, 0.22, 0.22, 0.1, 0.78, 0.0, 2,
          0.75, 0.78, 0.0, 0.38, 0.12, 0.32, 0, 0.18, 0.18, 0),
        dna(0.7, 0.38, 0.72, 0.52, 0.38, 0.7),
    ),
    make_preset(
        "Envelope River",
        "Flux",
        "The filter envelope as a flowing river — continuous, directional, never the same twice. Organic flux.",
        ["clavinet", "envelope", "river", "flow", "organic", "flux"],
        p(0.78, 0.72, 9000.0, 0.52, 0.001, 0.28, 0.28, 0.12, 0.82, 0.0, 2,
          0.75, 0.8, 0.0, 0.42, 0.2, 0.28, 1, 0.08, 0.15, 0),
        dna(0.65, 0.42, 0.75, 0.52, 0.42, 0.68),
    ),
    make_preset(
        "Square Pulse",
        "Flux",
        "Square LFO wah for rhythmic gating — the filter snaps open and shut in time. Percussive flux character.",
        ["clavinet", "square", "pulse", "rhythmic", "gated", "flux"],
        p(0.82, 0.82, 9500.0, 0.55, 0.001, 0.2, 0.2, 0.1, 0.82, 0.0, 2,
          0.8, 0.85, 0.0, 0.35, 0.18, 0.45, 3, 4.5, 0.35, 3),
        dna(0.72, 0.35, 0.88, 0.5, 0.35, 0.8),
    ),
    make_preset(
        "Drift Current",
        "Flux",
        "Random sample-and-hold wah drift — the filter follows an unpredictable path, like a current finding its own way.",
        ["clavinet", "drift", "random", "sample-hold", "unpredictable", "flux"],
        p(0.72, 0.72, 8500.0, 0.5, 0.001, 0.25, 0.25, 0.12, 0.75, 0.0, 2,
          0.72, 0.78, 0.0, 0.42, 0.15, 0.35, 4, 0.22, 0.25, 4),
        dna(0.65, 0.42, 0.78, 0.5, 0.45, 0.68),
    ),
]

# ---------------------------------------------------------------------------
# COUPLING (8) — paired membrane interaction, cross-engine modulation
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Membrane Talk",
        "Coupling",
        "Clavinet in coupling dialogue — migration enabled, the D6 becomes a coupling source/destination. Membrane-to-membrane resonance.",
        ["clavinet", "coupling", "membrane", "dialogue", "resonance", "coupling"],
        p(0.55, 0.65, 8000.0, 0.52, 0.001, 0.3, 0.35, 0.14, 0.6, 0.35, 2,
          0.55, 0.45, 0.65, 0.45, 0.12, 0.08, 0, 1.0, 0.06, 0),
        dna(0.62, 0.48, 0.45, 0.55, 0.5, 0.55),
    ),
    make_preset(
        "Sympathetic String",
        "Coupling",
        "Sympathetically resonating clavinet — migration creates a feedback path that extends the string's natural resonance.",
        ["clavinet", "sympathetic", "resonance", "feedback", "coupling", "string"],
        p(0.3, 0.45, 6500.0, 0.35, 0.002, 0.65, 0.45, 0.38, 0.35, 0.45, 2,
          0.38, 0.22, 0.55, 0.55, 0.1, 0.06, 1, 0.06, 0.05, 0),
        dna(0.5, 0.58, 0.38, 0.55, 0.58, 0.38),
    ),
    make_preset(
        "Entangled Clav",
        "Coupling",
        "The clavinet entangled with another engine — moderate migration, coupling macro fully opened. Sonic DNA interchange.",
        ["clavinet", "entangled", "coupling", "dna", "migration", "flux"],
        p(0.52, 0.7, 8500.0, 0.5, 0.001, 0.28, 0.32, 0.14, 0.58, 0.55, 2,
          0.52, 0.5, 0.72, 0.45, 0.12, 0.06, 0, 0.8, 0.06, 0),
        dna(0.62, 0.48, 0.5, 0.55, 0.52, 0.58),
    ),
    make_preset(
        "Dual Membrane",
        "Coupling",
        "Two membranes in conversation — clunk and migration create a paired mechanical resonance. Physical model coupling.",
        ["clavinet", "dual", "membrane", "mechanical", "coupling", "physical"],
        p(0.45, 0.6, 7500.0, 0.65, 0.001, 0.35, 0.35, 0.16, 0.5, 0.4, 2,
          0.45, 0.42, 0.6, 0.48, 0.1, 0.06, 0, 0.6, 0.05, 0),
        dna(0.58, 0.52, 0.42, 0.58, 0.52, 0.52),
    ),
    make_preset(
        "Cross Wah",
        "Coupling",
        "Wah driven by coupling input — another engine's output modulates the filter envelope amount. Reactive wah.",
        ["clavinet", "cross", "wah", "reactive", "coupling", "modulation"],
        p(0.72, 0.75, 9000.0, 0.52, 0.001, 0.25, 0.28, 0.12, 0.72, 0.5, 2,
          0.68, 0.62, 0.72, 0.42, 0.14, 0.08, 0, 1.5, 0.07, 0),
        dna(0.68, 0.42, 0.58, 0.55, 0.45, 0.68),
    ),
    make_preset(
        "Phase Lock",
        "Coupling",
        "Phase-locked coupling — the clavinet's filter envelope locked in phase with a coupling source. Synchronized dynamics.",
        ["clavinet", "phase-lock", "coupling", "synchronized", "filter", "dynamics"],
        p(0.6, 0.72, 8500.0, 0.5, 0.001, 0.28, 0.3, 0.12, 0.65, 0.45, 2,
          0.6, 0.52, 0.65, 0.42, 0.12, 0.06, 0, 0.8, 0.05, 0),
        dna(0.65, 0.45, 0.52, 0.55, 0.48, 0.62),
    ),
    make_preset(
        "Resonant Pair",
        "Coupling",
        "Two coupled resonant bodies — the clavinet acts as a resonator for another engine's output. Passive acoustic coupling.",
        ["clavinet", "resonant", "pair", "passive", "acoustic", "coupling"],
        p(0.25, 0.48, 6800.0, 0.45, 0.002, 0.55, 0.42, 0.28, 0.28, 0.38, 2,
          0.35, 0.18, 0.52, 0.55, 0.1, 0.05, 0, 0.5, 0.04, 0),
        dna(0.52, 0.55, 0.38, 0.55, 0.55, 0.38),
    ),
    make_preset(
        "Migration Gate",
        "Coupling",
        "Coupling-controlled gate — migration at maximum creates a gate effect driven by the coupling signal. Dynamic coupling architecture.",
        ["clavinet", "migration", "gate", "dynamic", "coupling", "control"],
        p(0.62, 0.68, 8800.0, 0.55, 0.001, 0.24, 0.28, 0.12, 0.65, 0.8, 2,
          0.62, 0.58, 0.85, 0.38, 0.12, 0.06, 0, 1.0, 0.06, 0),
        dna(0.65, 0.42, 0.58, 0.55, 0.45, 0.65),
    ),
]

# ---------------------------------------------------------------------------
# CRYSTALLINE (6) — bright clean, no wah, bell-like, pure tone
# ---------------------------------------------------------------------------

PRESETS += [
    make_preset(
        "Bell Clav",
        "Crystalline",
        "Clavinet voiced as a bell — minimal wah, maximum brightness, long clean decay. The crystalline side of the D6.",
        ["clavinet", "bell", "bright", "clean", "crystalline", "pure"],
        p(0.0, 0.85, 16000.0, 0.2, 0.001, 1.2, 0.3, 0.8, 0.0, 0.0, 2,
          0.65, 0.05, 0.0, 0.55, 0.08, 0.04, 0, 0.04, 0.03, 0),
        dna(0.92, 0.2, 0.08, 0.35, 0.65, 0.15),
    ),
    make_preset(
        "Crystal Touch",
        "Crystalline",
        "Touch-sensitive crystalline clavinet — velocity controls the brightness arc, almost no wah. Pure velocity expressivity.",
        ["clavinet", "crystal", "velocity", "clean", "bright", "crystalline"],
        p(0.02, 0.9, 15000.0, 0.18, 0.001, 0.9, 0.25, 0.65, 0.02, 0.0, 2,
          0.75, 0.08, 0.0, 0.6, 0.07, 0.04, 0, 0.04, 0.03, 0),
        dna(0.88, 0.22, 0.1, 0.38, 0.62, 0.18),
    ),
    make_preset(
        "Glass Membrane",
        "Crystalline",
        "Clavinet membrane as glass — extreme brightness, zero wah, the transparent side of the physical model. Fragile clarity.",
        ["clavinet", "glass", "membrane", "transparent", "bright", "crystalline"],
        p(0.0, 0.95, 18000.0, 0.15, 0.001, 1.0, 0.2, 0.72, 0.0, 0.0, 2,
          0.7, 0.04, 0.0, 0.62, 0.07, 0.03, 0, 0.03, 0.02, 0),
        dna(0.95, 0.15, 0.08, 0.32, 0.65, 0.12),
    ),
    make_preset(
        "Ice Clav",
        "Crystalline",
        "Frozen clavinet tone — ultra-bright, no wah, long clean crystalline decay. Cold and precise.",
        ["clavinet", "ice", "frozen", "bright", "cold", "crystalline"],
        p(0.0, 0.88, 17000.0, 0.12, 0.001, 1.1, 0.25, 0.75, 0.0, 0.0, 2,
          0.68, 0.04, 0.0, 0.6, 0.06, 0.03, 0, 0.03, 0.02, 0),
        dna(0.92, 0.15, 0.08, 0.32, 0.65, 0.1),
    ),
    make_preset(
        "Diamond String",
        "Crystalline",
        "Clavinet string made of diamond — maximum hardness, maximum brightness, the hardest possible articulation. Crystalline perfection.",
        ["clavinet", "diamond", "string", "hard", "bright", "crystalline"],
        p(0.05, 0.92, 16500.0, 0.22, 0.001, 0.85, 0.22, 0.68, 0.05, 0.0, 2,
          0.72, 0.05, 0.0, 0.58, 0.07, 0.03, 0, 0.04, 0.02, 0),
        dna(0.9, 0.18, 0.1, 0.32, 0.62, 0.18),
    ),
    make_preset(
        "Pure Quartz",
        "Crystalline",
        "Quartz-pure clavinet — no coloration, no wah, maximum tonal transparency. The instrument heard without any character processing.",
        ["clavinet", "quartz", "pure", "transparent", "clean", "crystalline"],
        p(0.0, 0.88, 15000.0, 0.15, 0.001, 1.0, 0.28, 0.72, 0.0, 0.0, 2,
          0.65, 0.04, 0.0, 0.58, 0.06, 0.03, 0, 0.03, 0.02, 0),
        dna(0.88, 0.18, 0.08, 0.3, 0.65, 0.1),
    ),
]

# =============================================================================
# VALIDATION
# =============================================================================

def validate_preset(preset):
    errors = []

    params = preset["parameters"].get("Onkolo", {})

    # Parameter range checks
    def check_range(key, lo, hi):
        v = params.get(key)
        if v is None:
            errors.append(f"Missing param: {key}")
        elif not (lo <= v <= hi):
            errors.append(f"{key}={v} outside [{lo}, {hi}]")

    check_range("onko_funk", 0.0, 1.0)
    check_range("onko_pickup", 0.0, 1.0)
    check_range("onko_brightness", 200.0, 20000.0)
    check_range("onko_clunk", 0.0, 1.0)
    check_range("onko_attack", 0.001, 0.2)
    check_range("onko_decay", 0.05, 3.0)
    check_range("onko_sustain", 0.0, 1.0)
    check_range("onko_release", 0.01, 2.0)
    check_range("onko_filterEnvAmt", 0.0, 1.0)
    check_range("onko_migration", 0.0, 1.0)
    check_range("onko_bendRange", 1.0, 24.0)
    check_range("onko_macroCharacter", 0.0, 1.0)
    check_range("onko_macroMovement", 0.0, 1.0)
    check_range("onko_macroCoupling", 0.0, 1.0)
    check_range("onko_macroSpace", 0.0, 1.0)
    check_range("onko_lfo1Rate", 0.005, 20.0)
    check_range("onko_lfo1Depth", 0.0, 1.0)
    check_range("onko_lfo1Shape", 0, 4)
    check_range("onko_lfo2Rate", 0.005, 20.0)
    check_range("onko_lfo2Depth", 0.0, 1.0)
    check_range("onko_lfo2Shape", 0, 4)

    # DNA range checks
    for dna_key in ["brightness", "warmth", "movement", "density", "space", "aggression"]:
        v = preset["dna"].get(dna_key)
        if v is None:
            errors.append(f"Missing DNA: {dna_key}")
        elif not (0.0 <= v <= 1.0):
            errors.append(f"DNA {dna_key}={v} out of [0,1]")

    # Name length
    if len(preset["name"]) > 30:
        errors.append(f"Name too long ({len(preset['name'])} chars): {preset['name']!r}")

    return errors


# =============================================================================
# WRITE
# =============================================================================

def safe_filename(name):
    return name.replace(" ", "_").replace("/", "-").replace("\\", "-")


def write_presets(presets):
    # Count presets per mood
    mood_counts = {}
    for preset in presets:
        mood = preset["mood"]
        mood_counts[mood] = mood_counts.get(mood, 0) + 1

    print(f"Writing {len(presets)} Onkolo presets...")
    print("Mood distribution:")
    for mood, count in sorted(mood_counts.items()):
        print(f"  {mood:15s}: {count}")

    # Validate
    all_names = [p["name"] for p in presets]
    duplicate_names = [n for n in all_names if all_names.count(n) > 1]
    if duplicate_names:
        print(f"\nERROR: Duplicate names: {set(duplicate_names)}")
        return False

    errors_found = False
    for preset in presets:
        errs = validate_preset(preset)
        if errs:
            print(f"\nVALIDATION ERRORS in '{preset['name']}':")
            for e in errs:
                print(f"  - {e}")
            errors_found = True

    if errors_found:
        print("\nAborting due to validation errors.")
        return False

    # Check aggression >= 0.7 count
    high_aggression = [p for p in presets if p["dna"]["aggression"] >= 0.7]
    print(f"\nHigh-aggression presets (aggression >= 0.7): {len(high_aggression)}")
    if len(high_aggression) < 25:
        print(f"WARNING: Only {len(high_aggression)} high-aggression presets (need ≥ 25)")

    # Write files
    written = 0
    for preset in presets:
        mood = preset["mood"]
        folder = os.path.join(PRESET_BASE, mood, "Onkolo")
        os.makedirs(folder, exist_ok=True)

        filename = safe_filename(preset["name"]) + ".xometa"
        filepath = os.path.join(folder, filename)

        with open(filepath, "w") as f:
            json.dump(preset, f, indent=2)
            f.write("\n")

        written += 1

    print(f"\nWrote {written} presets to Presets/XOceanus/{{mood}}/Onkolo/")
    return True


if __name__ == "__main__":
    success = write_presets(PRESETS)
    if not success:
        import sys
        sys.exit(1)
    print("Done.")
