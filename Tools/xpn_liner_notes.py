#!/usr/bin/env python3
"""
XPN Liner Notes Engine — XO_OX Designs
Auto-generates rich cultural/educational metadata for XPN expansion packs.

Liner notes capture the lore, history, and sonic philosophy embedded in each
XO_OX Collection. The output JSON is placed in the build directory where
xpn_packager.py discovers and bundles it into the XPN archive.

Supported collections:
  - Kitchen Essentials  (6 quads: Chef, Kitchen, Choir, Garden, Cellar, Broth)
  - Travel / Water      (4 sets: Sail, Industrial, Leisure, Historical + Sable Island)
  - Artwork / Color     (5 quads: Color A, Color B, Showmen, Aesthetes, Magic/Water)

Section types:
  collection_intro    — overarching design philosophy and architecture
  quad_intro          — one quad or set within a collection
  engine_profile      — per-engine identity, instrument history, sound design notes
  artist_bio          — for Artwork visionary quads
  color_science       — for Artwork color quads
  vessel_history      — for Travel vessel sets
  genre_timeline      — for Travel genre-era wildcards
  instrument_history  — standalone deep-dive on a world instrument
  fx_philosophy       — explanation of the FX chain design logic
  coupling_note       — how engines relate to each other in coupling mode

Output JSON schema:
  {
    "schema_version": "1.0",
    "pack_name": str,
    "collection": str,
    "quad": str | null,
    "generated": str (ISO date),
    "sections": [ { "type": str, ... } ]
  }

Packager integration:
  Place liner_notes.json in the build directory root. xpn_packager.py will
  automatically detect and bundle it alongside bundle_manifest.json.

Usage:
    # Full collection liner notes (all quads)
    python3 xpn_liner_notes.py --collection "Kitchen Essentials" --output ./liner_notes/

    # Single quad
    python3 xpn_liner_notes.py --collection "Kitchen Essentials" --quad "Chef" \\
        --output ./liner_notes/

    # Single engine (any collection)
    python3 xpn_liner_notes.py --engine ONSET --output ./liner_notes/

    # All collections (batch)
    python3 xpn_liner_notes.py --all --output ./liner_notes/

    # Validate an existing liner notes file
    python3 xpn_liner_notes.py --validate ./liner_notes/kitchen_chef.json
"""

import argparse
import json
import sys
from dataclasses import dataclass, field
from datetime import date
from pathlib import Path
from typing import Optional


# =============================================================================
# Schema helpers
# =============================================================================

def _section(section_type: str, **kwargs) -> dict:
    """Build a section dict with type guaranteed first."""
    return {"type": section_type, **kwargs}


def _stub(description: str) -> str:
    """
    Mark a content stub. Stubs describe WHAT should be written, not the
    final text. Authors replace stubs with real prose before publishing.
    """
    return f"[STUB: {description}]"


# =============================================================================
# COLLECTION: KITCHEN ESSENTIALS
# =============================================================================

def kitchen_collection_intro() -> dict:
    return _section(
        "collection_intro",
        title="Kitchen Essentials",
        tagline="Six quads. Twenty-four engines. Every essential instrument group, reframed through the kitchen.",
        body=(
            "Every synth workstation ships the same instrument categories: organ, piano, "
            "electric piano, strings, bass, pads. They're the kitchen essentials — the tools "
            "no studio can function without. "
            "The Culinary Collection takes these six essential instrument groups and rebuilds "
            "each as an XO_OX quad — four engines per group, each shaped by a culinary metaphor "
            "that determines how the instruments are modeled, how they couple, and what new sounds "
            "emerge when the kitchen comes alive. "
            "This isn't a sample library with categories. Each quad is a synthesis philosophy "
            "applied to an instrument family. The culinary metaphor isn't decoration — it's the "
            "DSP architecture."
        ),
        architecture={
            "quads": [
                {"name": "Chef",    "instrument_group": "Organs",         "metaphor": "The Cooks (adversarial)"},
                {"name": "Kitchen", "instrument_group": "Pianos",         "metaphor": "The Surfaces (resonant)"},
                {"name": "Choir",   "instrument_group": "Voices",         "metaphor": "The Table (conversational)"},
                {"name": "Garden",  "instrument_group": "Strings",        "metaphor": "The Ingredients (evolutionary)"},
                {"name": "Cellar",  "instrument_group": "Bass",           "metaphor": "The Foundation (gravitational)"},
                {"name": "Broth",   "instrument_group": "Pads",           "metaphor": "The Medium (cooperative)"},
            ],
            "secret_slot": {
                "name": "Fusion",
                "instrument_group": "Electric Pianos",
                "unlock": "Load all 4 Kitchen (piano) engines simultaneously",
                "metaphor": "The Crossroads (migratory)",
            },
        },
        design_philosophy=(
            "The culinary metaphor runs all the way down: coupling modes are named after the "
            "physical relationship between cooking processes (adversarial, resonant, cooperative). "
            "Parameter vocabularies borrow culinary language (temperature, density, seasoning). "
            "Even the unlock mechanic is a culinary thesis — you cannot do fusion cooking without "
            "a fully equipped kitchen."
        ),
    )


def kitchen_quad_intro(quad_name: str) -> dict:
    quads = {
        "Chef": {
            "instrument_group": "Organs",
            "coupling_mode": "Adversarial — ego, stakes, regional competition",
            "concept": (
                "Four world-class seafood chefs in regional competition. Each chef brings a "
                "loadout of 4 regional organs + 4 boutique synth wildcards + 4 FX recipes = "
                "64 configurations per chef, 256 total. The organ is the most ego-laden "
                "instrument — it fills the entire acoustic space. Two organs in the same room "
                "fight. The Chef quad takes that fight seriously."
            ),
            "engines": [
                {"name": "XOto",    "chef": "Oto (音)",   "region": "East Asia",           "organs": "Shō, Sheng, Khene, Melodica"},
                {"name": "XOctave", "chef": "Octave",     "region": "Western Europe",       "organs": "Cavaillé-Coll pipe, Baroque positiv, Musette accordion, Farfisa"},
                {"name": "XOleg",   "chef": "Oleg",       "region": "Baltic/Eastern Europe","organs": "Bayan, Hurdy-gurdy, Bandoneon, Garmon"},
                {"name": "XOtis",   "chef": "Otis",       "region": "Americas",             "organs": "Hammond B3, Calliope, Blues harmonica, Zydeco accordion"},
            ],
        },
        "Kitchen": {
            "instrument_group": "Pianos",
            "coupling_mode": "Resonant — material physics, sympathetic vibration",
            "concept": (
                "The kitchen IS the instrument. Hammers hit strings inside resonant bodies — "
                "but the surface material determines everything about how energy transfers, "
                "how harmonics develop, how sound decays. Four materials. Four piano archetypes. "
                "Four completely different responses to the same gesture."
            ),
            "engines": [
                {"name": "XOven",    "material": "Cast Iron",      "archetype": "Concert Grand",       "character": "Massive thermal mass — dark, sustained, absorbs and radiates slowly"},
                {"name": "XOchre",   "material": "Copper",         "archetype": "Upright Piano",       "character": "Quick, responsive, warm conductivity. Thinner body, faster decay"},
                {"name": "XObelisk", "material": "Stone / Marble", "archetype": "Prepared Piano",      "character": "Cold, dense, unyielding. Objects placed on strings"},
                {"name": "XOpaline", "material": "Glass/Porcelain","archetype": "Toy Piano / Celesta", "character": "Fragile, crystalline, rings when struck"},
            ],
        },
        "Choir": {
            "instrument_group": "Voices",
            "coupling_mode": "Conversational — call and response, harmony, counterpoint",
            "concept": (
                "The table is where the food is served and the meal becomes an occasion. "
                "Voices are the most fundamental instrument — every culture sings before it "
                "builds a piano or stretches a string. Choir is the table where the Culinary "
                "Collection gathers."
            ),
            "engines": [
                {"name": "XOrison",   "role": "Grace / blessing",   "archetype": "Sacred Choir",       "character": "Reverent, blended, cathedral. Many voices becoming one"},
                {"name": "XOpera",    "role": "The toast",          "archetype": "Operatic Voice",     "character": "Dramatic, solo, projecting. One voice filling the room"},
                {"name": "XOrate",    "role": "Kitchen calls",      "archetype": "Beatbox / Vocal Perc","character": "Rhythmic vocal percussion — Order up! Behind! Yes, Chef!"},
                {"name": "XOvertone", "role": "The hum",            "archetype": "Throat Singing",     "character": "One voice containing multitudes — Tuvan khoomei, Tibetan chant"},
            ],
        },
        "Garden": {
            "instrument_group": "Strings",
            "coupling_mode": "Evolutionary — growth, cultivation, seasonal change",
            "concept": (
                "The garden is where raw ingredients grow. Strings are the most alive of "
                "instrument families — they vibrate, they breathe, they respond to touch with "
                "infinite gradation. A bowed string is a cultivated plant — sustained, nurtured, "
                "shaped over time. A plucked string is a harvest — one decisive gesture, then decay."
            ),
            "engines": [
                {"name": "XOrchard",  "zone": "Orchard (cultivated)", "archetype": "Orchestral Strings", "character": "Lush, blended, 60 players breathing together"},
                {"name": "XOvergrow", "zone": "Wild Garden",          "archetype": "Solo Strings",        "character": "Raw, exposed, every imperfection audible"},
                {"name": "XOsier",    "zone": "Herb Garden (woven)",  "archetype": "Chamber Strings",     "character": "Intimate, intertwined. 4–8 players each voice audible"},
                {"name": "XOxalis",   "zone": "Geometric Garden",     "archetype": "Synth Strings",       "character": "Filtered, phased, stacked. Solina, Elka, JP-8 pads"},
            ],
        },
        "Cellar": {
            "instrument_group": "Bass",
            "coupling_mode": "Gravitational — weight, pull, everything settles toward it",
            "concept": (
                "Every kitchen has a cellar. It's underneath. It's dark. It stores what "
                "everything else rests on. You don't see the cellar, but you'd notice instantly "
                "if it disappeared. Bass is the cellar of music — foundational, felt more than "
                "heard, the gravity that pulls everything else into coherence."
            ),
            "engines": [
                {"name": "XOgre",  "stock": "Root Vegetables",  "archetype": "Sub Bass",      "character": "Below hearing, felt in the body. Pure weight"},
                {"name": "XOxbow", "stock": "Aged Wine",        "archetype": "Analog Bass",   "character": "Warm, fat, developed — Moog, TB-303, SH-101 with terroir"},
                {"name": "XOaken", "stock": "Cured Wood",       "archetype": "Acoustic Bass", "character": "Wooden body, gut or steel strings, hands on an instrument"},
                {"name": "XOmega", "stock": "Distillation",     "archetype": "FM / Digital Bass","character": "Pure, concentrated, mathematically precise. DX bass, Reese"},
            ],
        },
        "Broth": {
            "instrument_group": "Pads / Atmosphere",
            "coupling_mode": "Cooperative — physics, no ego, transformation through immersion",
            "concept": (
                "The broth is the liquid medium everything else cooks in. It's not an ingredient "
                "— it's the environment. A good broth transforms everything immersed in it. "
                "A pad does the same thing in music — it's the atmospheric bed that changes the "
                "character of every sound played over it."
            ),
            "engines": [
                {"name": "XOverwash",  "process": "Infusion",         "archetype": "Diffusion Pad",      "timescale": "Seconds",      "character": "Slow spectral crossfade — one timbre bleeds into another like watercolor"},
                {"name": "XOverworn",  "process": "Reduction",        "archetype": "Erosion Pad",        "timescale": "Session-long", "character": "Imperceptible spectral theft — partials removed until the sound is fundamentally different"},
                {"name": "XOverflow",  "process": "Pressure Cooking", "archetype": "Pressure Pad",       "timescale": "Phrases",      "character": "Energy accumulates until it bursts — sealed harmonics compress then release"},
                {"name": "XOvercast",  "process": "Flash Freeze",     "archetype": "Crystallization Pad","timescale": "Instant",      "character": "One trigger transforms everything — liquid to solid, warm to frozen"},
            ],
        },
        "Fusion": {
            "instrument_group": "Electric Pianos",
            "coupling_mode": "Migratory — cultural collision, genre-crossing, hybrid identity",
            "concept": (
                "Electric pianos exist because genres collided. Jazz needed amplification. "
                "Funk needed percussive keys. Pop needed shimmer. R&B needed warmth-at-volume. "
                "Every EP is a fusion instrument. The Fusion slot unlocks when all 4 Kitchen "
                "engines are loaded — you cannot do fusion cooking without a fully equipped kitchen."
            ),
            "unlock": "Load XOven + XOchre + XObelisk + XOpaline simultaneously",
            "engines": [
                {"name": "XOasis",     "cuisine": "Spice Route",       "archetype": "Rhodes / Tine EP",  "character": "Warm bell-tones, smooth enough for jazz, gritty enough for neo-soul"},
                {"name": "XOddfellow", "cuisine": "Night Market",      "archetype": "Wurlitzer / Reed EP","character": "Reedy, gritty, intimate. Sounds best slightly broken"},
                {"name": "XOnkolo",    "cuisine": "West African diaspora","archetype": "Clavinet",        "character": "Funky, percussive, string-slap energy — Stevie Wonder, Bernie Worrell"},
                {"name": "XOpcode",    "cuisine": "Silicon Valley→Tokyo","archetype": "DX / FM EP",      "character": "Crystalline, digital, impossibly clean — the 1980s imagining the future"},
            ],
        },
    }
    data = quads.get(quad_name, {})
    if not data:
        return _section("quad_intro", title=f"[STUB: {quad_name} quad not yet defined]")
    return _section(
        "quad_intro",
        collection="Kitchen Essentials",
        quad=quad_name,
        instrument_group=data["instrument_group"],
        coupling_mode=data["coupling_mode"],
        concept=data["concept"],
        engines=data.get("engines", []),
        unlock=data.get("unlock"),
    )


def kitchen_engine_profile(engine_name: str) -> dict:
    profiles = {
        "XOto": {
            "engine": "XOto",
            "chef": "Oto (音)",
            "title": "XOto — East Asian Free-Reed Organs",
            "instrument_history": _stub(
                "Write 200–300 words on the East Asian free-reed organ family: the shō (Japanese "
                "court gagaku), the sheng (Chinese, 3,000 year history, ancestor of the harmonica "
                "and accordion), the Lao khene (UNESCO intangible heritage 2017), and the melodica "
                "as its modern descendant. Emphasize the free-reed physics shared across all four "
                "and the cultural journeys from the steppes into imperial courts."
            ),
            "sound_design_notes": _stub(
                "Explain how XOto models cluster density (shō chords use tone clusters not found "
                "in Western harmony), chiff transients (breath onset), and pressure instability. "
                "Note how the wildcard boutique synth company reshapes the timbral character."
            ),
            "sonic_dna": {"brightness": 0.45, "warmth": 0.60, "movement": 0.35, "density": 0.55, "space": 0.40, "aggression": 0.25},
            "recommended_couplings": [
                "XOrchard (Garden) — reed organ over orchestral strings: the gagaku ensemble",
                "XOverwash (Broth) — free-reed immersed in slow spectral diffusion",
            ],
            "param_prefix": "oto_",
        },
        "XOtis": {
            "engine": "XOtis",
            "chef": "Otis",
            "title": "XOtis — The Americas Organ Kitchen",
            "instrument_history": _stub(
                "Write 200–300 words on the American organ family: the Hammond B3 and its "
                "percussion feature, Leslie rotary cabinet physics, the steam calliope (showboat "
                "history, Mississippi River), blues harmonica (African American tradition, "
                "country blues to Chicago electric), and Creole zydeco accordion. Each instrument "
                "is a product of cultural collision and migration. The Americas don't have a "
                "native organ tradition — they borrowed, broke, and rebuilt everything."
            ),
            "sound_design_notes": _stub(
                "Describe how XOtis models the Hammond's tonewheel leakage and drawbar crosstalk, "
                "the calliope's steam pressure instability, and how the Leslie speed modulation "
                "is implemented as a coupling output (XOtis can modulate adjacent engines with "
                "its rotary LFO)."
            ),
            "sonic_dna": {"brightness": 0.55, "warmth": 0.75, "movement": 0.65, "density": 0.60, "space": 0.45, "aggression": 0.50},
            "recommended_couplings": [
                "XOgre (Cellar) — Hammond over sub bass: the gospel church foundation",
                "XOrchid (Artwork, Color A) — zydeco accordion meets guzheng: impossible geography",
            ],
            "param_prefix": "otis_",
        },
    }
    data = profiles.get(engine_name, {})
    if not data:
        return _section(
            "engine_profile",
            engine=engine_name,
            title=f"XO_OX — {engine_name}",
            instrument_history=_stub(f"Write instrument history for {engine_name}"),
            sound_design_notes=_stub(f"Write sound design notes for {engine_name}"),
            sonic_dna=_stub("Fill 6D Sonic DNA: brightness, warmth, movement, density, space, aggression"),
            recommended_couplings=[],
            param_prefix=_stub("Fill parameter prefix from CLAUDE.md engine table"),
        )
    return _section("engine_profile", **data)


# =============================================================================
# COLLECTION: TRAVEL / WATER
# =============================================================================

def travel_collection_intro() -> dict:
    return _section(
        "collection_intro",
        title="Travel / Water",
        tagline="Four vessel sets. Twenty engines. Every instrument family reframed through boats, water, and the genres that sailed with them.",
        body=(
            "Travel is not tourism. Tourism is looking. Travel is being changed by a place. "
            "Boats are the oldest form of long-distance travel. Every vessel type has a sonic "
            "personality that maps — almost inevitably — to a family of acoustic instruments. "
            "Sails catch wind the same way reeds do. A steamboat valve and a brass bell share "
            "the same pressure logic. An island pontoon afternoon and a steel pan have the same "
            "sun in them."
        ),
        architecture={
            "sets": [
                {"name": "Sail",       "vessel": "Wind-powered sailing vessels", "instrument": "Woodwinds",            "genre": "Hip Hop",       "coupling": "Breath"},
                {"name": "Industrial", "vessel": "Steam/engine-powered ships",   "instrument": "Brass",                "genre": "Dance Music",   "coupling": "Pressure"},
                {"name": "Leisure",    "vessel": "Pleasure craft",               "instrument": "Island Cultural",      "genre": "Island Music",  "coupling": "Warmth"},
                {"name": "Historical", "vessel": "Ships of history",             "instrument": "Historical Percussion","genre": "Synth Genres",  "coupling": "Impact"},
            ],
            "secret_slot": {
                "name": "Sable Island",
                "unlock": "Load one engine from each of the four vessel sets",
                "mythology": "Graveyard of the Atlantic — 350 shipwrecks, 400 wild horses, 100,000 grey seals, one tree",
            },
        },
        cross_set_couplings=[
            {"name": "Trade Wind",  "sets": ["Sail", "Industrial"],  "description": "Woodwind meets brass. The 19th-century transition from sail to steam."},
            {"name": "Regatta",     "sets": ["Sail", "Leisure"],     "description": "Woodwind meets island instruments. Competition becoming celebration."},
            {"name": "Expedition",  "sets": ["Sail", "Historical"],  "description": "Woodwind meets ancient percussion. The voyage that might not return."},
            {"name": "Port Call",   "sets": ["Industrial", "Leisure"],"description": "Brass meets island instruments. Heavy industry meets afternoon sun."},
            {"name": "Convoy",      "sets": ["Industrial","Historical"],"description": "Brass meets ancient percussion. Organized force across water."},
            {"name": "Pilgrimage",  "sets": ["Leisure", "Historical"],"description": "Island instruments meet ancient percussion. Comfort encountering consequence."},
        ],
    )


def travel_set_intro(set_name: str) -> dict:
    sets = {
        "Sail": {
            "vessel_type": "Wind-powered sailing vessels",
            "instrument_family": "Woodwinds",
            "genre_axis": "Hip Hop",
            "coupling_mode": "Breath — air as shared medium",
            "concept": (
                "Wind fills sails. Breath fills reeds. MCs breathe bars over chopped woodwind "
                "samples. The connection isn't metaphor — it's air."
            ),
            "engines": [
                {"name": "XOxygen",  "vessel": "Dhow (lateen sail)",    "instrument": "Flute / Bansuri / Ney",   "character": "Open, ancient, the breath that crosses the Sahara and the Indian Ocean"},
                {"name": "XObeam",   "vessel": "Clipper (speed sailing)","instrument": "Clarinet / Chalumeau",    "character": "Focused, fast, agile. Both reward precision — the clipper and the clarinet"},
                {"name": "XOarlock", "vessel": "Junk (Chinese battened)","instrument": "Oboe / Duduk / Suona",    "character": "Nasal, penetrating, double-reed intensity. Self-correcting, survives what others can't"},
                {"name": "XOssia",   "vessel": "Catamaran (twin hull)",  "instrument": "Saxophone family",        "character": "Twin hulls = twin registers. Jazz, soul, pop, R&B — the sax crosses every boundary"},
            ],
            "era_wildcards": [
                {"era": "Old School",          "period": "1979–1986", "signature": "808/Linn drum machine stiffness, sparse arrangement, DMX snap, vocal sample chops"},
                {"era": "Golden Age / Boom Bap","period": "1986–1996", "signature": "SP-1200 grit, MPC60 swing, chopped soul/jazz loops, vinyl crackle, Premo scratches"},
                {"era": "New Jack Swing",       "period": "1987–1995", "signature": "Teddy Riley's R&B-meets-hip-hop. Synth bass stabs, swingbeat, gated reverb, FM bells"},
                {"era": "Trap",                 "period": "2012–present","signature": "808 sub-bass sustain, hi-hat rolls, dark minor keys, Metro Boomin cinematic approach"},
            ],
            "fx_chains": [
                {"name": "Boom Bap FX",   "path": "Vinyl saturation → lo-pass EQ → sidechain compression → short room verb"},
                {"name": "Trap FX",       "path": "Hard-clip distortion → aggressive hi-pass → hall reverb → stereo widener"},
                {"name": "New Jack FX",   "path": "Gate reverb → FM chorus → warm compression → bright shelf EQ"},
                {"name": "Chopped Soul FX","path": "Time-stretch artifacts → vinyl wow+flutter → tape saturation → mono-sum below 200Hz"},
            ],
        },
        "Industrial": {
            "vessel_type": "Steam/engine-powered ships",
            "instrument_family": "Brass",
            "genre_axis": "Dance Music",
            "coupling_mode": "Pressure — steam through valves, air through brass bells, bass through speaker cones",
            "concept": (
                "Steam pressure through valves. Air pressure through brass bells. Both move "
                "bodies — one moves cargo ships, the other moves dance floors."
            ),
            "engines": [
                {"name": "XOxide",    "vessel": "Steamship (coal/steam)",  "instrument": "Trombone / Tuba",       "character": "Heavy, industrial, the brass section's foundation. Steam and trombone share the same physics"},
                {"name": "XOrdnance", "vessel": "Battleship / Warship",    "instrument": "Trumpet / Cornet",      "character": "Sharp, commanding, the brass that orders and announces. The call to attention"},
                {"name": "XOilrig",   "vessel": "Tanker / Oil Platform",   "instrument": "French Horn",           "character": "Remote, reverberant, the horn that carries across distances. Both fill vast spaces"},
                {"name": "XOutremer", "vessel": "Container Ship (freight)","instrument": "Flugelhorn / Muted Trumpet","character": "Smooth, muted, the quiet efficiency of modern logistics"},
            ],
            "era_wildcards": [
                {"era": "Disco",        "period": "1974–1982", "signature": "Four-on-the-floor, Nile Rodgers funk, Moroder sequencer, Studio 54 mirror ball"},
                {"era": "House",        "period": "1984–1995", "signature": "Roland TR-909 + TB-303 acid, Chicago warehouse reverb, Frankie Knuckles edits"},
                {"era": "Jungle / DnB", "period": "1991–1998", "signature": "Timestretched Amen breaks, sub-bass pressure, reggae vocal chops, 170bpm+"},
                {"era": "Trance",       "period": "1998–2006", "signature": "Supersaw unison, side-chain pumping, long builds, van Dyk/ATB/Tiësto melodic structures"},
            ],
            "fx_chains": [
                {"name": "Disco FX",   "path": "Phaser → studio plate reverb → light tape compression → stereo auto-pan"},
                {"name": "House FX",   "path": "Sidechain compression → warm saturation → spring reverb → high-pass filter"},
                {"name": "Jungle FX",  "path": "Timestretching artifacts → distortion → sub-bass layering → rapid auto-pan"},
                {"name": "Trance FX",  "path": "Supersaw chorus → sidechain pump → shimmer reverb (long tail, modulated) → high-shelf boost"},
            ],
        },
        "Leisure": {
            "vessel_type": "Pleasure craft",
            "instrument_family": "Island cultural instruments",
            "genre_axis": "Island Music",
            "coupling_mode": "Warmth — sun on water, heat on metal, the temperature of an afternoon that becomes a memory",
            "concept": (
                "The pontoon afternoon. The charter yacht. The harbor bar. Every island in the "
                "world has a music that sounds like sunset on water."
            ),
            "engines": [
                {"name": "XOshun",     "vessel": "Pontoon Boat",         "instrument": "Steel Pan (Trinidad)",  "character": "Open, ringing, the sun in metallic form. Industrial waste becoming the most joyful sound in the Caribbean"},
                {"name": "XOutrigger", "vessel": "Outrigger Canoe",      "instrument": "Ukulele / Slack-Key",   "character": "Polynesian, intimate. The oldest ocean vessel design still in active use"},
                {"name": "XOriole",    "vessel": "Sailing Yacht (luxury)","instrument": "Kalimba / Mbira",      "character": "African thumb piano — portable, meditative, circular melodies matching ocean swells"},
                {"name": "XOkavango", "vessel": "Glass-Bottom Boat",    "instrument": "Marimba / Balafon",     "character": "The underwater perspective. Wooden resonators floating above hollow depth"},
            ],
            "era_wildcards": [
                {"era": "Roots Reggae / Dub", "period": "1968–1985", "signature": "One Drop rhythm, King Tubby echo chamber, Lee Perry alien production, the deep"},
                {"era": "Dancehall / Riddim",  "period": "1985–present","signature": "Digital Casio riddims (Sleng Teng), sound system culture, drum machine patterns"},
                {"era": "Reggaetón",           "period": "2004–present","signature": "Dem Bow riddim, Bad Bunny genre fluidity, Latin trap bass, Caribbean meets urban"},
                {"era": "Pacific / Polynesian","period": "Traditional–present","signature": "Slack-key tunings, log drum rhythms, Maori haka percussion, Hawaiian falsetto"},
            ],
            "fx_chains": [
                {"name": "The Scientist's FX","path": "Spring reverb (long dark) → tape delay (modulated) → high-cut sweep → sub-bass boost"},
                {"name": "Sound System FX",   "path": "Sub-harmonic generator → hard limiter → room reverb (concrete) → bass boost"},
                {"name": "Tropical FX",       "path": "Chorus (warm wide) → plate reverb (bright) → stereo ping-pong delay → high-shelf sparkle"},
                {"name": "Roots FX",          "path": "Heavy one-knob compression → spring reverb (short) → lo-fi saturation → mono sum"},
            ],
        },
        "Historical": {
            "vessel_type": "Ships of history",
            "instrument_family": "Historical percussion",
            "genre_axis": "Synth-enabled genres",
            "coupling_mode": "Impact — the moment of contact that organized empires and machines that organized genres",
            "concept": (
                "The ships that carried empires. The machines that carried genres. Both changed "
                "the world and became relics."
            ),
            "engines": [
                {"name": "XOrage",   "vessel": "Trireme / Galley",         "instrument": "Taiko / War Drum",     "character": "170 rowers pulling in time to a drum. The rhythm that synchronized the oars"},
                {"name": "XOrmada", "vessel": "Galleon / Ship of the Line","instrument": "Frame Drum / Bodhran", "character": "The Age of Sail. Armadas, trade routes, the drum that marched soldiers aboard"},
                {"name": "XOrigin",  "vessel": "Viking Longship",          "instrument": "Ship's Bell / Gong",   "character": "Norse exploration — longships went everywhere. Metal percussion that resonates for minutes"},
                {"name": "XObelus",  "vessel": "Chinese Treasure Fleet",   "instrument": "Gamelan / Bonang",     "character": "Zheng He's fleet reached East Africa 60 years before Columbus. Gamelan: impossible alone"},
            ],
            "era_wildcards": [
                {"era": "Synthpop",   "period": "1978–1986", "signature": "Depeche Mode austerity, OMD melodic sequencing, Human League directness, Vince Clarke hooks"},
                {"era": "Chiptune",   "period": "1983–present","signature": "NES 2A03 pulse waves, SID chip arpeggios, Game Boy WAV channel, 4-channel limitations as aesthetic"},
                {"era": "Industrial", "period": "1976–1995", "signature": "Throbbing Gristle anti-music, Neubauten metal percussion, NIN studio precision, destruction as composition"},
                {"era": "Electro",    "period": "1982–1988", "signature": "TR-808 as lead instrument, Kraftwerk precision, Afrika Bambaataa, machine funk as identity"},
            ],
            "fx_chains": [
                {"name": "Juno Chorus FX",    "path": "BBD chorus (Juno-60 stereo mode) → warm saturation → gentle high-cut → short room reverb"},
                {"name": "Moogerfooger FX",   "path": "MF-101 LP filter (env follower) → MF-104M analog delay (modulated) → MF-102 ring mod"},
                {"name": "BOSS/Roland FX",    "path": "CE-1 chorus ensemble → RE-201 Space Echo → RV-6 shimmer reverb"},
                {"name": "Buchla FX",         "path": "296e spectral processor (16-band, env/band) → 227e delay → 206e mixer (feedback path)"},
            ],
        },
        "Sable Island": {
            "vessel_type": "The island where all journeys end",
            "instrument_family": "Hybrid — fuses all four parent sets",
            "genre_axis": "Cross-genre fusion eras",
            "coupling_mode": "Elemental — sand, fog, salt, gale",
            "concept": (
                "Sable Island. 44°N 60°W. A sandbar 42km long. 350 shipwrecks (the Graveyard "
                "of the Atlantic). 400 wild horses. 100,000 grey seals. One tree — a single "
                "Scotch Pine, three feet tall, surviving against all physical reason. "
                "Sable Island is not a travel destination. It is what travel reveals when you "
                "go far enough."
            ),
            "unlock": "Load one engine from each of Sail, Industrial, Leisure, and Historical",
            "engines": [
                {"name": "XOutlier",  "phenomenon": "Wild Horses (400 feral)", "archetype": "Hybrid wind/percussion/string", "character": "Galloping, feral, unpredictable. Hooves=percussion, mane=woodwind, sinew=strings, breath=brass"},
                {"name": "XOssuary",  "phenomenon": "Graveyard (350+ wrecks)", "archetype": "Submerged/corroded hybrid",   "character": "Metal becoming sand. Hull groaning. The sound of every vessel type — after it went down"},
                {"name": "XOutcry",   "phenomenon": "Grey Seal Colony (100k+)","archetype": "Layered voices/texture",      "character": "100,000 individual voices creating texture, not harmony. Between speech and music"},
                {"name": "XOneTree",  "phenomenon": "One Tree (single Scotch Pine)","archetype": "Minimal/solo",           "character": "One oscillator. Three feet tall. Surviving against all physical reason"},
            ],
            "fx_chains": [
                {"name": "Sand FX", "path": "Granular diffusion → slow LP sweep → room verb (sand absorption)", "element": "Sand"},
                {"name": "Fog FX",  "path": "Dense reverb (infinite tail, heavy LP) → chorus (slow wide) → noise floor", "element": "Fog"},
                {"name": "Salt FX", "path": "Corrosion distortion (asymmetric) → band-pass (narrow swept) → metallic resonance", "element": "Salt"},
                {"name": "Gale FX", "path": "Wind noise modulation → aggressive high-pass (gusting) → tremolo (irregular) → hard limiter", "element": "Gale"},
            ],
        },
    }
    data = sets.get(set_name, {})
    if not data:
        return _section("quad_intro", title=f"[STUB: {set_name} set not yet defined]")
    return _section(
        "quad_intro",
        collection="Travel / Water",
        quad=set_name,
        vessel_type=data["vessel_type"],
        instrument_family=data["instrument_family"],
        genre_axis=data["genre_axis"],
        coupling_mode=data["coupling_mode"],
        concept=data["concept"],
        engines=data.get("engines", []),
        unlock=data.get("unlock"),
        era_wildcards=data.get("era_wildcards", []),
        fx_chains=data.get("fx_chains", []),
    )


def travel_vessel_history(engine_name: str) -> dict:
    histories = {
        "XOshun": {
            "engine": "XOshun",
            "vessel": "Pontoon Boat",
            "instrument": "Steel Pan (Trinidad)",
            "vessel_history": _stub(
                "Write 150 words on the pontoon boat: flat-platform design, aluminum pontoons "
                "providing buoyancy, no pretension — the most democratic pleasure craft. Popular "
                "on American lakes and rivers. The vessel that says: we are not going anywhere "
                "important, and that is the point."
            ),
            "instrument_history": (
                "The steel pan was invented in Trinidad and Tobago in the 1930s–1940s from oil "
                "drums — industrial waste repurposed into one of the only acoustic instruments "
                "invented in the 20th century. Winston 'Spree' Simon and Ellie Mannette are "
                "credited with key developments. When British colonial authorities banned the "
                "tamboo bamboo drums, Trinidadians began experimenting with metal containers. "
                "The oil barrel's curved bottom, when beaten into a concave dish and marked with "
                "tuned sections, produced clear, ringing pitches. Industrial waste had become the "
                "most joyful sound in the Caribbean. The pan's overtone structure is genuinely "
                "unusual — it produces strong even harmonics that give it an almost bell-like "
                "sheen. Oshun is the Yoruba/Orisha goddess of rivers, love, and sweet water — "
                "worshipped in Trinidad through the Orisha tradition brought via the transatlantic "
                "slave trade, now inseparable from the island's spiritual identity."
            ),
            "xoox_connection": (
                "XOshun models the steel pan's unusual harmonic structure — the way individual "
                "notes contain strong partials from adjacent notes due to physical proximity on "
                "the pan surface. The Warmth coupling mode outputs temperature-modulated signals "
                "that other Leisure engines can receive as timbral color shifts."
            ),
        },
        "XObelus": {
            "engine": "XObelus",
            "vessel": "Chinese Treasure Fleet (Zheng He)",
            "instrument": "Gamelan / Bonang / Gender",
            "vessel_history": (
                "Admiral Zheng He's treasure fleet (1405–1433) was the largest wooden maritime "
                "expedition in history — ships up to 120 meters long, 317 vessels at peak, "
                "voyaging to Southeast Asia, India, Arabia, and East Africa. These ships reached "
                "East Africa 60 years before Columbus reached the Caribbean. The fleet was built "
                "on the orders of the Yongle Emperor and abruptly ended when he died; his "
                "successor demolished the ships and banned deep-sea voyages. The fleet that "
                "could have colonized the world chose not to. Obelus (÷): the division sign — "
                "the symbol that divides history into before and after."
            ),
            "instrument_history": _stub(
                "Write 200 words on Javanese/Balinese gamelan: the interlocking metallophones "
                "(bonang, gender, saron), gongs and kempul, communal playing structure where no "
                "single player can produce the music alone, Javanese tuning systems (pélog and "
                "sléndro) that don't correspond to Western equal temperament, and the tradition's "
                "spread through court culture and UNESCO intangible heritage status."
            ),
            "xoox_connection": _stub(
                "Explain how XObelus models gamelan's interlocking interdependence — the engine "
                "generates incomplete patterns that only make musical sense when coupling with "
                "other Historical engines, mirroring how gamelan requires collective performance."
            ),
        },
    }
    data = histories.get(engine_name, {})
    if not data:
        return _section(
            "vessel_history",
            engine=engine_name,
            vessel_history=_stub(f"Write vessel history for {engine_name}"),
            instrument_history=_stub(f"Write instrument history for {engine_name}"),
            xoox_connection=_stub(f"Explain XO_OX connection for {engine_name}"),
        )
    return _section("vessel_history", **data)


# =============================================================================
# COLLECTION: ARTWORK / COLOR
# =============================================================================

def artwork_collection_intro() -> dict:
    return _section(
        "collection_intro",
        title="Artwork / Color",
        tagline="Five quads. Twenty-four engines. Color as frequency. Sound as pigment. Vision as synthesis.",
        body=(
            "Color is frequency. Sound is frequency. The Artwork collection takes this literally. "
            "Every engine is organized around a color — not as decoration, but as architecture. "
            "The color determines the world instrument, the complementary color determines the "
            "wildcard, and the FX chains are drawn from color science and water physics. "
            "This is synesthesia as a design framework."
        ),
        architecture={
            "quads": [
                {"name": "Color Quad A", "theme": "The Intuitive Palette",    "engines": ["XOxblood", "XOnyx", "XOchre", "XOrchid"]},
                {"name": "Color Quad B", "theme": "The Deeper Spectrum",      "engines": ["XOttanio", "XOma'oma'o", "XOstrum", "XŌni"]},
                {"name": "Showmen",      "theme": "Performance Visionaries",  "engines": ["XOpulent (Clinton)", "XOccult (Henning)", "XOvation (Fosse)", "XOverdrive (Van Halen)"]},
                {"name": "Aesthetes",    "theme": "Visual/Spatial Visionaries","engines": ["XOrnament (Wiley)", "XOblation (Barragán)", "XObsession (Almodóvar)", "XOther (Madlib)"]},
                {"name": "Magic/Water",  "theme": "The Impossible Liquid",    "engines": ["XOrdeal (Houdini/Waterphone)", "XOutpour (Cohen/Theremin)", "XOctavo (Anderson/Harmonium)", "XObjet (Chung Ling Soo/Bullroarer)"]},
            ],
            "secret_slot": {
                "name": "The Arcade",
                "engines": ["XOkami (Suda 51)", "XOmni (Mizuguchi)", "XOdama (Takahashi)", "XOffer (Massive Monster)"],
                "unlock": _stub("Unlock mechanic for Arcade quad — TBD in design phase"),
            },
        },
        complementary_color_system=(
            "Every color engine has a complementary color — directly opposite on the color wheel. "
            "The wildcard axis IS the complement. Loading the complement transforms the engine "
            "into its own opposite, progressing through four shades: Tint (25% shift), Tone "
            "(equal tension), Shade (complement dominant), Pure (full inversion — the engine "
            "has become its own negative)."
        ),
        fx_design=(
            "FX chains in the Artwork collection are drawn from two domains: color science "
            "(dispersion, refraction, absorption, fluorescence) and water physics "
            "(surface tension, laminar flow, turbulence, crystallization). These are not "
            "metaphors — they are design constraints. Each FX chain models a specific physical "
            "phenomenon applied to audio."
        ),
    )


def artwork_artist_bio(artist_name: str) -> dict:
    bios = {
        "Kehinde Wiley": {
            "name": "Kehinde Wiley",
            "born": "1977",
            "birthplace": "Los Angeles, California",
            "known_for": "Ornamental oil portraiture placing Black subjects in poses drawn from European Old Masters, surrounded by ornate floral and textile patterns",
            "biography": (
                "Kehinde Wiley grew up in South Central Los Angeles and studied at the San "
                "Francisco Art Institute and the Yale School of Art. His practice began by "
                "stopping strangers on the streets of Harlem and asking them to pose in the "
                "manner of figures in Old Master paintings — Napoleons on horseback, Caesars "
                "enthroned, saints in ecstasy — surrounded by decorative backgrounds drawn from "
                "Islamic tile work, William Morris wallpaper, and Baroque ornament. The effect "
                "is a direct confrontation between art history's absence of Black subjects and "
                "the living presence of the people Wiley photographs. The ornament is not "
                "background. It is armor, throne, and crown simultaneously."
            ),
            "key_works": [
                "Napoleon Leading the Army Over the Alps (2005)",
                "Rumors of War (2019, Times Square + Virginia Museum of Fine Arts)",
                "Official portrait of Barack Obama, National Portrait Gallery (2018)",
            ],
            "color_palette": "Deep jewel tones — lapis, crimson, gold — against pattern-saturated backgrounds. The figure is always the most saturated element; the background serves the figure.",
            "xoox_engine": "XOrnament",
            "xoox_connection": (
                "XOrnament translates Wiley's layered ornamentation into additive synthesis — "
                "the engine's spectral layers mirror the relationship between foreground subject "
                "and decorative background. The Ornament macro parameter shifts the ratio of "
                "fundamental to overtone structure, moving from simple presence to complex "
                "pattern-saturation, the way Wiley's backgrounds move from quiet texture to "
                "explosive decoration."
            ),
            "accent_color": _stub("Assign accent color from XO_OX palette — suggest deep jewel tones"),
        },
        "Luis Barragán": {
            "name": "Luis Barragán",
            "born": "1902",
            "died": "1988",
            "birthplace": "Guadalajara, Mexico",
            "known_for": "Minimalist architecture using saturated color, light, water, and silence as primary building materials",
            "biography": (
                "Luis Barragán trained as an engineer in Guadalajara and became one of the 20th "
                "century's most influential architects without ever leaving Mexico's architectural "
                "mainstream to achieve international fame until late in his career. His work uses "
                "color not as surface treatment but as spatial element — a magenta wall doesn't "
                "just look pink, it turns the light in a courtyard pink, and that pink light falls "
                "on white walls and yellow floors and suddenly you are inside a color. He was "
                "awarded the Pritzker Prize in 1980. In his acceptance speech, he said: "
                "'In my work, I have always tried to maintain a dialogue with the forces of '
                "Nature, and especially with silence. Silence is something that architects have "
                "forgotten.'"
            ),
            "key_works": [
                "Casa Barragán, Mexico City (1948) — now UNESCO World Heritage Site",
                "Las Arboledas (1958–61) — horse trough with magenta wall and eucalyptus grove",
                "San Cristóbal Stables (1968) — water, pink, yellow, and horses",
            ],
            "color_palette": "Mexican folk color translated into architectural scale — magenta, ochre, cobalt, coral, violet. Colors that should not be architectural but become architectural through scale and light.",
            "xoox_engine": "XOblation",
            "xoox_connection": (
                "XOblation models Barragán's use of silence as structural element. The engine's "
                "SILENCE macro doesn't reduce volume — it restructures the sound around its "
                "absences, the way Barragán's courtyards use open sky and still water to make "
                "the surrounding walls more present. The color wildcard axis maps directly to "
                "Barragán's palette — each complement shade shifts the tonal character the way "
                "a color change shifts the light in a Barragán courtyard."
            ),
            "accent_color": _stub("Suggest accent color — magenta or ochre reference recommended"),
        },
        "George Clinton": {
            "name": "George Clinton",
            "born": "1941",
            "birthplace": "Kannapolis, North Carolina",
            "known_for": "Architect of P-Funk — the synthesis of soul, rock, psychedelia, and science fiction that produced Parliament-Funkadelic",
            "biography": _stub(
                "Write 200 words on George Clinton's P-Funk mythology: the Mothership, Dr. "
                "Funkenstein, Chocolate City, the One (the downbeat as gospel), flash suits and "
                "costumes as visual language, the influence on hip hop (Dr. Dre, Kendrick Lamar, "
                "Prince). Clinton as the architect of an entire musical cosmology — not just a "
                "musician but a world-builder."
            ),
            "xoox_engine": "XOpulent",
            "xoox_connection": _stub(
                "Explain how XOpulent models P-Funk's layered ensemble approach — the way "
                "Parliament-Funkadelic stacked 40+ musicians into a single recording. Describe "
                "the Mothership macro and how it triggers the One (downbeat emphasis)."
            ),
            "accent_color": _stub("Assign accent color — suggest electric purple or P-Funk gold"),
        },
    }
    data = bios.get(artist_name, {})
    if not data:
        return _section(
            "artist_bio",
            name=artist_name,
            biography=_stub(f"Write biography for {artist_name}"),
            xoox_connection=_stub(f"Explain XO_OX connection for {artist_name}"),
        )
    return _section("artist_bio", **data)


def artwork_color_science(engine_name: str) -> dict:
    entries = {
        "XOxblood": {
            "engine": "XOxblood",
            "color": "Oxblood",
            "hex": "#6A0D0D",
            "cultural_context": (
                "Oxblood is the color of things that have been alive — iron oxide, the chemical "
                "composition of dried blood. Used in architecture (Oxblood brick facades in "
                "London and New York), leather goods (oxblood leather achieves its color through "
                "iron-rich tanning compounds, not dye), and Chinese lacquerware. The color "
                "carries weight — it is not cheerful, not aggressive, not neutral. It is the "
                "color of endurance."
            ),
            "complement": "Teal",
            "complement_hex": "#0D6A6A",
            "complement_wildcard_progression": [
                {"shade": "Tint",  "variant": "Seafoam #7FDBCA", "sonic_shift": "Light aquatic wash — erhu heard through shallow water"},
                {"shade": "Tone",  "variant": "True Teal #008080","sonic_shift": "Equal tension — warm blood meeting cold ocean"},
                {"shade": "Shade", "variant": "Deep Teal #005F5F", "sonic_shift": "Underwater — erhu submerged, frequencies absorbed by depth"},
                {"shade": "Pure",  "variant": "Abyssal Teal #003D3D","sonic_shift": "Full inversion — erhu has become a submarine instrument"},
            ],
            "world_instrument": "Erhu (Chinese two-string fiddle)",
            "instrument_connection": (
                "The erhu is the reddest instrument in world music. Two strings, one bow, "
                "a resonance that sounds like a human crying or laughing. The outsider instrument "
                "— it entered China from the northern steppes, was dismissed as crude, and "
                "took 1,000 years to become the lead voice of Chinese orchestras. A-Bing "
                "(1893–1950), a blind street musician, recorded 'Erquan Yingyue' (Moon Reflected "
                "on Second Spring) months before his death. He was homeless. It is considered "
                "one of the most beautiful recordings ever made on any instrument."
            ),
            "physics_note": (
                "Oxblood red is in the long-wavelength end of visible light (~620–700nm), "
                "adjacent to infrared. In audio terms, this maps to the low-mid range — "
                "the frequencies (300–800Hz) where warmth lives. The erhu's fundamental "
                "resonance is centered in this same region."
            ),
        },
        "XOnyx": {
            "engine": "XOnyx",
            "color": "Onyx Black",
            "hex": "#0A0A0A",
            "cultural_context": (
                "Onyx comes from the Greek word for 'claw' or 'fingernail.' According to myth, "
                "Cupid used an arrowhead to cut Venus's divine fingernails while she slept; the "
                "clippings fell to earth and became onyx. Even the darkest gemstone began as "
                "part of a body. Onyx absorbs all light — its blackness is definitional, not "
                "decorative. In architecture, black absorbs heat and demands attention through "
                "contrast. In sound, silence demands attention the same way."
            ),
            "complement": "Shock White",
            "complement_hex": "#F5F5F5",
            "complement_wildcard_progression": [
                {"shade": "Tint",  "variant": "Ivory #FFFFF0",   "sonic_shift": "Warmth entering the black — overtone brightness emerging"},
                {"shade": "Tone",  "variant": "True White #FFFFFF","sonic_shift": "Maximum contrast — drone and silence in equal measure"},
                {"shade": "Shade", "variant": "Glare #F8F8FF",   "sonic_shift": "Overexposed — pushed into blinding clarity"},
                {"shade": "Pure",  "variant": "Bleach #FDFDFD",  "sonic_shift": "Full inversion — pure overtone, no fundamental"},
            ],
            "world_instrument": "Didgeridoo (Yidaki — Aboriginal Australian drone)",
            "instrument_connection": (
                "The didgeridoo is the blackest sound in world music. A continuous drone from "
                "a hollow eucalyptus trunk — literally a hole in the dark. At least 1,500 years "
                "old, possibly 40,000 — a candidate for the oldest wind instrument still in "
                "continuous use. The technique of circular breathing allows the drone to continue "
                "indefinitely. In Yolngu culture, the yidaki is not just music — it is a "
                "communication device, specific drone patterns carrying meaning across distances. "
                "The instrument as language."
            ),
            "physics_note": (
                "Black absorbs all visible wavelengths. The didgeridoo's drone occupies the "
                "lowest audible frequencies — 60–120Hz fundamental — absorbing the full spectrum "
                "of musical harmony into a single continuous point. The circular breathing "
                "technique creates a true continuous signal: no attack, no decay, no silence."
            ),
        },
    }
    data = entries.get(engine_name, {})
    if not data:
        return _section(
            "color_science",
            engine=engine_name,
            color=_stub(f"Color name for {engine_name}"),
            hex=_stub("Hex value"),
            cultural_context=_stub(f"Cultural/historical context for the color"),
            complement=_stub("Complementary color name"),
            complement_hex=_stub("Complementary color hex"),
            world_instrument=_stub("World instrument assigned to this engine"),
            instrument_connection=_stub("Connection between color and instrument"),
            physics_note=_stub("Physics of the color mapped to audio frequency"),
        )
    return _section("color_science", **data)


# =============================================================================
# ENGINE PROFILES (standalone — any collection)
# =============================================================================

def engine_standalone_profile(engine_id: str) -> dict:
    """
    Generate a liner notes profile for an existing XOmnibus engine.
    References the engine table in CLAUDE.md for accent colors and prefixes.
    """
    known = {
        "ONSET": {
            "engine": "ONSET",
            "full_name": "XOnset",
            "accent": "#0066FF",
            "param_prefix": "perc_",
            "title": "XOnset — Percussive Synthesis Engine",
            "summary": (
                "XOnset models 8 percussion voices (Kick, Snare, Closed Hat, Open Hat, Clap, "
                "Tom, Percussion, FX) through physical synthesis — not samples. Each voice is "
                "a synthesis model of the physical mechanism that produces that sound: resonant "
                "membranes, spring reverbs, metallic plate modes, noise through waveguides."
            ),
            "doctrine_compliance": "B002 — XVC Cross-Voice Coupling (Blessing 2): all 8 voices can modulate each other, 3–5 years ahead of the field.",
            "sound_design_notes": _stub(
                "Write 150 words on XOnset's dual-layer blend architecture (Circuit layer = "
                "vintage analog, Algorithm layer = modern digital, BLEND macro morphs between). "
                "Explain the MACHINE/PUNCH/SPACE/MUTATE macros."
            ),
            "sonic_dna": {"brightness": 0.55, "warmth": 0.35, "movement": 0.60, "density": 0.70, "space": 0.45, "aggression": 0.65},
        },
        "OPAL": {
            "engine": "OPAL",
            "full_name": "XOpal",
            "accent": "#A78BFA",
            "param_prefix": "opal_",
            "title": "XOpal — Granular Synthesis Engine",
            "summary": (
                "XOpal performs granular synthesis — decomposing audio into microscopic grains "
                "(typically 10–500ms) and reassembling them in transformed configurations. "
                "The engine draws from Curtis Roads' granular theory, building sound from clouds "
                "of particles rather than continuous waveforms."
            ),
            "doctrine_compliance": "D005 — autonomous modulation with rate floor ≤ 0.01 Hz (grain scatter LFO).",
            "sound_design_notes": _stub(
                "Write 150 words on XOpal's grain parameters: grain size, scatter, density, "
                "position freeze/spray, and how COUPLING interaction with DRIFT (XOdyssey) "
                "enables the DRIFT→OPAL and OPAL→DUB coupling routes."
            ),
            "sonic_dna": {"brightness": 0.50, "warmth": 0.55, "movement": 0.75, "density": 0.65, "space": 0.80, "aggression": 0.20},
        },
    }
    data = known.get(engine_id.upper(), {})
    if not data:
        return _section(
            "engine_profile",
            engine=engine_id,
            title=f"XO_OX — {engine_id}",
            summary=_stub(f"Write engine summary for {engine_id}"),
            sound_design_notes=_stub(f"Write sound design notes for {engine_id}"),
            sonic_dna=_stub("Fill 6D Sonic DNA"),
            param_prefix=_stub("Fill prefix from CLAUDE.md"),
            accent=_stub("Fill accent hex from CLAUDE.md engine table"),
        )
    return _section("engine_profile", **data)


# =============================================================================
# Document builders
# =============================================================================

def build_kitchen_notes(quad: Optional[str] = None) -> dict:
    sections = [kitchen_collection_intro()]
    quads_all = ["Chef", "Kitchen", "Choir", "Garden", "Cellar", "Broth", "Fusion"]
    targets = [quad] if quad else quads_all
    for q in targets:
        sections.append(kitchen_quad_intro(q))
    return sections


def build_travel_notes(set_name: Optional[str] = None) -> list:
    sections = [travel_collection_intro()]
    sets_all = ["Sail", "Industrial", "Leisure", "Historical", "Sable Island"]
    targets = [set_name] if set_name else sets_all
    for s in targets:
        sections.append(travel_set_intro(s))
    return sections


def build_artwork_notes(quad: Optional[str] = None) -> list:
    sections = [artwork_collection_intro()]
    # Artist bios for Aesthetes quad
    if quad in (None, "Aesthetes"):
        for artist in ["Kehinde Wiley", "Luis Barragán", "George Clinton"]:
            sections.append(artwork_artist_bio(artist))
    # Color science for Color Quad A
    if quad in (None, "Color Quad A"):
        for engine in ["XOxblood", "XOnyx"]:
            sections.append(artwork_color_science(engine))
            sections.append(_section(
                "instrument_history",
                engine=engine,
                title=_stub(f"Instrument history title for {engine}"),
                body=_stub(f"Full instrument history for {engine} world instrument — 300+ words"),
            ))
    return sections


def build_engine_notes(engine_id: str) -> list:
    return [engine_standalone_profile(engine_id)]


# =============================================================================
# Top-level document
# =============================================================================

@dataclass
class LinerNotesDoc:
    pack_name: str
    collection: str
    quad: Optional[str] = None
    sections: list = field(default_factory=list)

    def to_dict(self) -> dict:
        return {
            "schema_version": "1.0",
            "pack_name": self.pack_name,
            "collection": self.collection,
            "quad": self.quad,
            "generated": date.today().isoformat(),
            "sections": self.sections,
        }

    def write(self, output_dir: Path, filename: Optional[str] = None) -> Path:
        output_dir.mkdir(parents=True, exist_ok=True)
        if not filename:
            slug = self.pack_name.lower().replace(" ", "_").replace("/", "_")
            filename = f"liner_notes_{slug}.json"
        out_path = output_dir / filename
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(self.to_dict(), f, indent=2, ensure_ascii=False)
        return out_path


# =============================================================================
# Validation
# =============================================================================

VALID_SECTION_TYPES = {
    "collection_intro", "quad_intro", "engine_profile", "artist_bio",
    "color_science", "vessel_history", "genre_timeline", "instrument_history",
    "fx_philosophy", "coupling_note",
}


def validate_liner_notes(path: Path) -> bool:
    """
    Validate a liner notes JSON file against the schema.

    Checks:
      - Required top-level keys present
      - schema_version is "1.0"
      - All sections have a 'type' field with a recognized value
      - No section is missing both 'title' and 'engine'
    """
    errors = []
    try:
        with open(path, encoding="utf-8") as f:
            doc = json.load(f)
    except (json.JSONDecodeError, OSError) as exc:
        print(f"  [ERROR] Cannot read {path}: {exc}")
        return False

    for key in ("schema_version", "pack_name", "collection", "sections"):
        if key not in doc:
            errors.append(f"Missing required key: '{key}'")

    if doc.get("schema_version") != "1.0":
        errors.append(f"Unexpected schema_version: {doc.get('schema_version')!r}")

    sections = doc.get("sections", [])
    if not isinstance(sections, list):
        errors.append("'sections' must be a list")
    else:
        for i, sec in enumerate(sections):
            sec_type = sec.get("type")
            if not sec_type:
                errors.append(f"Section {i}: missing 'type' field")
            elif sec_type not in VALID_SECTION_TYPES:
                errors.append(f"Section {i}: unknown type {sec_type!r} — valid: {sorted(VALID_SECTION_TYPES)}")
            if "title" not in sec and "engine" not in sec and "name" not in sec:
                errors.append(f"Section {i} (type={sec_type!r}): no 'title', 'engine', or 'name' field")

    if errors:
        print(f"  [FAIL] {path}")
        for e in errors:
            print(f"         - {e}")
        return False

    stub_count = sum(
        1 for sec in sections
        for v in sec.values()
        if isinstance(v, str) and v.startswith("[STUB:")
    )
    status = "PASS" if not errors else "FAIL"
    print(f"  [{status}] {path.name}  ({len(sections)} sections, {stub_count} stubs remaining)")
    return True


# =============================================================================
# CLI
# =============================================================================

COLLECTION_ALIASES = {
    "kitchen": "Kitchen Essentials",
    "kitchen essentials": "Kitchen Essentials",
    "travel": "Travel / Water",
    "travel / water": "Travel / Water",
    "travel/water": "Travel / Water",
    "artwork": "Artwork / Color",
    "artwork / color": "Artwork / Color",
    "artwork/color": "Artwork / Color",
}


def main():
    parser = argparse.ArgumentParser(
        description="XPN Liner Notes Engine — XO_OX Designs",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--collection", metavar="NAME",
                       help='Collection name: "Kitchen Essentials", "Travel / Water", "Artwork / Color"')
    group.add_argument("--engine", metavar="ENGINE_ID",
                       help="Standalone engine ID (e.g. ONSET, OPAL)")
    group.add_argument("--all", action="store_true",
                       help="Generate liner notes for all three collections")
    group.add_argument("--validate", metavar="FILE",
                       help="Validate an existing liner notes JSON file")

    parser.add_argument("--quad", metavar="QUAD",
                        help="Restrict to a single quad or set within the collection")
    parser.add_argument("--output", metavar="DIR", default="./liner_notes",
                        help="Output directory (default: ./liner_notes)")
    parser.add_argument("--filename", metavar="FILE",
                        help="Override output filename (default: auto-generated)")

    args = parser.parse_args()

    # --- Validate mode ---
    if args.validate:
        ok = validate_liner_notes(Path(args.validate))
        sys.exit(0 if ok else 1)

    output_dir = Path(args.output)

    # --- Single engine ---
    if args.engine:
        engine_id = args.engine.upper()
        doc = LinerNotesDoc(
            pack_name=f"XO_OX — {engine_id} Engine",
            collection="XOmnibus",
            quad=None,
            sections=build_engine_notes(engine_id),
        )
        out = doc.write(output_dir, args.filename)
        print(f"  Liner notes: {out}  ({len(doc.sections)} sections)")
        return

    # --- All collections ---
    if args.all:
        for collection_name, builder in [
            ("Kitchen Essentials", lambda: build_kitchen_notes()),
            ("Travel / Water",     lambda: build_travel_notes()),
            ("Artwork / Color",    lambda: build_artwork_notes()),
        ]:
            sections = builder()
            slug = collection_name.lower().replace(" ", "_").replace("/", "_")
            doc = LinerNotesDoc(
                pack_name=f"XO_OX {collection_name}",
                collection=collection_name,
                sections=sections,
            )
            out = doc.write(output_dir)
            print(f"  {collection_name}: {out}  ({len(sections)} sections)")
        return

    # --- Single collection ---
    collection_norm = COLLECTION_ALIASES.get(args.collection.lower(), args.collection)

    if "Kitchen" in collection_norm:
        sections = build_kitchen_notes(args.quad)
        pack_name = f"XO_OX Kitchen Essentials{': ' + args.quad if args.quad else ''}"
    elif "Travel" in collection_norm:
        sections = build_travel_notes(args.quad)
        pack_name = f"XO_OX Travel / Water{': ' + args.quad if args.quad else ''}"
    elif "Artwork" in collection_norm:
        sections = build_artwork_notes(args.quad)
        pack_name = f"XO_OX Artwork / Color{': ' + args.quad if args.quad else ''}"
    else:
        print(f"  [ERROR] Unknown collection: {args.collection!r}")
        print("  Valid: 'Kitchen Essentials', 'Travel / Water', 'Artwork / Color'")
        sys.exit(1)

    doc = LinerNotesDoc(
        pack_name=pack_name,
        collection=collection_norm,
        quad=args.quad,
        sections=sections,
    )
    out = doc.write(output_dir, args.filename)
    print(f"  Liner notes: {out}  ({len(sections)} sections)")


if __name__ == "__main__":
    main()
