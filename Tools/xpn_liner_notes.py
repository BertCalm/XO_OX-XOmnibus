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
                "\u2018In my work, I have always tried to maintain a dialogue with the forces of "
                "Nature, and especially with silence. Silence is something that architects have "
                "forgotten.\u2019"
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
        # ------------------------------------------------------------------
        # ONSET — XOnset (Electric Blue #0066FF)
        # ------------------------------------------------------------------
        "ONSET": {
            "engine": "ONSET",
            "full_name": "XOnset",
            "creature": "Electric Eel — Bio-electric percussion architect",
            "accent": "#0066FF",
            "param_prefix": "perc_",
            "title": "XOnset — Percussive Synthesis Engine",
            "summary": (
                "XOnset models 8 percussion voices (Kick, Snare, Closed Hat, Open Hat, Clap, "
                "Tom, Percussion, FX) through physical synthesis — not samples. Each voice is "
                "a synthesis model of the physical mechanism that produces that sound: resonant "
                "membranes, spring reverbs, metallic plate modes, noise through waveguides."
            ),
            "historical_homage": (
                "Inspired by the Roland TR-808 and TR-909 — synthesized drum machines that "
                "redefined pop, hip hop, and electronic music worldwide between 1980 and 1985. "
                "XOnset extends their philosophy: physical models replace sample lookup, "
                "producing infinite timbral variation from first principles."
            ),
            "cultural_context": (
                "Synthesized percussion has shaped every genre from Miami bass to techno to "
                "drill. XOnset carries the torch of those drum machines while adding the "
                "cross-voice coupling intelligence (XVC) that the originals never had — "
                "your kick now shapes your hi-hat, your snare modulates your FX voice."
            ),
            "use_cases": [
                "Building fully synthesized drum kits that breathe and evolve in real time",
                "Cross-voice percussion design — kick sidechain into hat filter, clap into reverb tail",
                "Drum synthesis export to MPC via XPN — 8-voice physical kits for sampler playback",
            ],
            "coupling_recommendations": [
                "OPAL: route ONSET transients into grain trigger — percussive granular textures",
                "ORBITAL: ONSET MACHINE macro drives ORBITAL GROUP envelope macro",
                "OVERDUB: ONSET FX voice feeds OVERDUB spring reverb send",
            ],
            "doctrine_compliance": "B002 — XVC Cross-Voice Coupling (Blessing 2): all 8 voices can modulate each other, 3–5 years ahead of the field.",
            "sound_design_notes": _stub(
                "Write 150 words on XOnset's dual-layer blend architecture (Circuit layer = "
                "vintage analog, Algorithm layer = modern digital, BLEND macro morphs between). "
                "Explain the MACHINE/PUNCH/SPACE/MUTATE macros."
            ),
            "sonic_dna": {"brightness": 0.55, "warmth": 0.35, "movement": 0.60, "density": 0.70, "space": 0.45, "aggression": 0.65},
            "credit": "XO_OX Designs — xo-ox.org",
        },

        # ------------------------------------------------------------------
        # OPAL — XOpal (Lavender #A78BFA)
        # ------------------------------------------------------------------
        "OPAL": {
            "engine": "OPAL",
            "full_name": "XOpal",
            "creature": "Jellyfish — Cloud of luminescent particles in perpetual drift",
            "accent": "#A78BFA",
            "param_prefix": "opal_",
            "title": "XOpal — Granular Synthesis Engine",
            "summary": (
                "XOpal performs granular synthesis — decomposing audio into microscopic grains "
                "(typically 10–500ms) and reassembling them in transformed configurations. "
                "The engine draws from Curtis Roads' granular theory, building sound from clouds "
                "of particles rather than continuous waveforms."
            ),
            "historical_homage": (
                "Homage to Iannis Xenakis and his 1971 granular theory, plus Barry Truax's "
                "early real-time granular synthesis work in the 1980s. The grain cloud model "
                "was a speculative DSP concept for decades before CPU power made it practical. "
                "XOpal treats each grain as a living organism contributing to a collective."
            ),
            "cultural_context": (
                "Granular synthesis gave composers tools to work with time itself — stretching, "
                "compressing, and randomizing audio at the sub-note level. XOpal inherits this "
                "philosophy and adds the XO_OX coupling dimension: grains can be driven by "
                "external engine outputs, creating layered cloud textures no single engine could produce."
            ),
            "use_cases": [
                "Atmospheric pads and textures from single sustained notes stretched into landscapes",
                "Glitch and stutter effects by manipulating grain position and scatter in real time",
                "Layered granular coupling with ODYSSEY for spectral drift clouds",
            ],
            "coupling_recommendations": [
                "ODYSSEY (DRIFT): DRIFT→OPAL coupling route — pitch modulates grain position",
                "OVERDUB (DUB): OPAL→DUB route — granular output into OVERDUB tape delay",
                "ONSET: ONSET triggers drive grain density bursts",
            ],
            "doctrine_compliance": "D005 — autonomous modulation with rate floor ≤ 0.01 Hz (grain scatter LFO).",
            "sound_design_notes": _stub(
                "Write 150 words on XOpal's grain parameters: grain size, scatter, density, "
                "position freeze/spray, and how COUPLING interaction with DRIFT (XOdyssey) "
                "enables the DRIFT→OPAL and OPAL→DUB coupling routes."
            ),
            "sonic_dna": {"brightness": 0.50, "warmth": 0.55, "movement": 0.75, "density": 0.65, "space": 0.80, "aggression": 0.20},
            "credit": "XO_OX Designs — xo-ox.org",
        },

        # ------------------------------------------------------------------
        # OVERDUB — XOverdub (Olive #6B7B3A)
        # ------------------------------------------------------------------
        "OVERDUB": {
            "engine": "OVERDUB",
            "full_name": "XOverdub",
            "creature": "Sea Turtle — Patient, ancient, layered in accumulated time",
            "accent": "#6B7B3A",
            "param_prefix": "dub_",
            "title": "XOverdub — Dub Synth & Performance FX Engine",
            "summary": (
                "XOverdub is a dub synth and performance FX platform built around the "
                "Jamaican sound system tradition. Its signal chain runs Voice → Send VCA → "
                "Drive → Tape Delay → Spring Reverb → Master, placing classic dub effects "
                "as first-class synthesis elements rather than afterthoughts."
            ),
            "historical_homage": (
                "Homage to King Tubby, Lee 'Scratch' Perry, and the Jamaican dub tradition "
                "of the 1960s–70s — engineers who turned the mixing board into an instrument. "
                "The spring reverb model is specifically inspired by the AKG BX20, a staple "
                "of classic reggae and dub productions that Vangelis and Tomita also praised."
            ),
            "cultural_context": (
                "Dub is the genre that invented remix culture — the idea that a recording's "
                "raw tracks are raw material for a new performance. XOverdub channels that "
                "philosophy into a synthesizer: the FX chain is the composition. "
                "Every preset is a live sound system waiting for your performance."
            ),
            "use_cases": [
                "Dub and reggae bass lines with authentic tape echo and spring reverb textures",
                "Live dub mixing — real-time send VCA throws and tape delay feedback swells",
                "Ambient and experimental soundscapes via the spring reverb reverb tail",
            ],
            "coupling_recommendations": [
                "OPAL: OPAL→DUB coupling route — granular clouds into the dub FX chain",
                "OBLONG: OBLONG bassline feeds OVERDUB tape delay for deep dub bass",
                "ODYSSEY: DRIFT pitch source drives OVERDUB vibrato depth",
            ],
            "doctrine_compliance": "B004 — Spring Reverb (Blessing 4): metallic splash praised by Vangelis + Tomita.",
            "sound_design_notes": _stub(
                "Write 150 words on XOverdub's signal path: Voice oscillator quality, "
                "Send VCA performance throws, Drive saturation character, Tape Delay "
                "feedback behavior at high settings, and the AKG-inspired spring reverb model."
            ),
            "sonic_dna": {"brightness": 0.40, "warmth": 0.75, "movement": 0.55, "density": 0.50, "space": 0.80, "aggression": 0.35},
            "credit": "XO_OX Designs — xo-ox.org",
        },

        # ------------------------------------------------------------------
        # OBLONG — XOblong (Amber #E9A84A)
        # ------------------------------------------------------------------
        "OBLONG": {
            "engine": "OBLONG",
            "full_name": "XOblong",
            "creature": "Mudskipper — Amphibious, low-end explorer crossing genres",
            "accent": "#E9A84A",
            "param_prefix": "bob_",
            "title": "XOblong — Character Bass Synthesizer",
            "summary": (
                "XOblong is a character bass synthesizer with a 72-source file architecture, "
                "167 factory presets, and an Apple Liquid Glass UI. It excels at warm analog "
                "bass tones, chord/scale performance via PlaySurface, and MPC XPN export for "
                "bass keygroup programs. The engine name internally uses the 'bob_' prefix, "
                "a legacy identifier frozen for preset compatibility."
            ),
            "historical_homage": (
                "Inspired by the Minimoog Model D and its descendants — the synthesis standard "
                "for bass from 1970 to the present. XOblong's filter model draws from the "
                "ladder filter topology, adding modern character controls that push the tonal "
                "palette beyond the analog originals without erasing their warmth."
            ),
            "cultural_context": (
                "Bass synthesis is the engine of popular music — from Motown to hip hop to "
                "techno, the bass synth defines the groove. XOblong gives producers a bass "
                "engine that handles everything from vintage Moog tones to modern reese basses, "
                "with PlaySurface chord modes that turn bass lines into melodic statements."
            ),
            "use_cases": [
                "Deep analog-style bass lines with expressive filter envelope articulation",
                "Chord-mode bass comping via PlaySurface scale/chord selection",
                "MPC export of multi-velocity bass keygroup programs for live performance",
            ],
            "coupling_recommendations": [
                "OVERDUB: OBLONG bass feeds OVERDUB dub echo chain",
                "ONSET: ONSET kick modulates OBLONG filter cutoff for sidechain-style pumping",
                "OUROBOROS: OBLONG bass root feeds OUROBOROS chaotic oscillator pitch",
            ],
            "doctrine_compliance": "D001 — velocity drives filter brightness (hammer weight model).",
            "sound_design_notes": _stub(
                "Write 150 words on XOblong's filter character, the PlaySurface chord/scale "
                "modes, and what makes OBLONG presets distinct from generic bass synth presets."
            ),
            "sonic_dna": {"brightness": 0.35, "warmth": 0.80, "movement": 0.45, "density": 0.60, "space": 0.30, "aggression": 0.55},
            "credit": "XO_OX Designs — xo-ox.org",
        },

        # ------------------------------------------------------------------
        # ORBITAL — XOrbital (Warm Red #FF6B6B)
        # ------------------------------------------------------------------
        "ORBITAL": {
            "engine": "ORBITAL",
            "full_name": "XOrbital",
            "creature": "Nautilus — Chambered, orbital, ancient logarithmic architecture",
            "accent": "#FF6B6B",
            "param_prefix": "orb_",
            "title": "XOrbital — Group Envelope Performance Synthesizer",
            "summary": (
                "XOrbital is a performance synthesizer built around a Group Envelope System — "
                "macros that govern entire sections of the synthesis chain in coordinated "
                "sweeps. The engine excels at cinematic pads, evolving leads, and "
                "orchestral-style dynamics where multiple parameters move as one organism."
            ),
            "historical_homage": (
                "Homage to Bob Moog and Dave Smith — the engineers who gave synthesizers "
                "the ADSR envelope as a musical instrument parameter. XOrbital's Group "
                "Envelope System is crowned by their legacy: the envelope is no longer a "
                "utility function but the primary compositional tool."
            ),
            "cultural_context": (
                "Orbital (the band) proved that electronic music could be performed live with "
                "as much drama as a rock concert — real-time parameter automation as "
                "composition. XOrbital channels that performance-first spirit into a "
                "synthesis engine where every macro arc tells a sonic story."
            ),
            "use_cases": [
                "Cinematic pad sweeps with coordinated filter, volume, and pitch arcs",
                "Live performance leads with macro GROUP sweeps for dramatic dynamic shifts",
                "Evolving ambient textures driven by autonomous GROUP envelope automation",
            ],
            "coupling_recommendations": [
                "ONSET: ONSET macro output drives ORBITAL GROUP envelope trigger",
                "OPAL: ORBITAL GROUP drives OPAL grain density in coordinated swells",
                "ORGANON: ORGANON metabolic rate drives ORBITAL envelope speed",
            ],
            "doctrine_compliance": "B001 — Group Envelope System (Blessing 1): crowned by Moog + Smith.",
            "sound_design_notes": _stub(
                "Write 150 words on XOrbital's GROUP envelope macros, how they differ from "
                "standard ADSR per-voice envelopes, and what kinds of sounds benefit most "
                "from the coordinated GROUP sweep approach."
            ),
            "sonic_dna": {"brightness": 0.60, "warmth": 0.65, "movement": 0.70, "density": 0.55, "space": 0.75, "aggression": 0.40},
            "credit": "XO_OX Designs — xo-ox.org",
        },

        # ------------------------------------------------------------------
        # OBESE — XObese (Hot Pink #FF1493)
        # ------------------------------------------------------------------
        "OBESE": {
            "engine": "OBESE",
            "full_name": "XObese",
            "creature": "Pufferfish — Expands under pressure, saturates on contact",
            "accent": "#FF1493",
            "param_prefix": "fat_",
            "title": "XObese — Saturation & Character Synthesis Engine",
            "summary": (
                "XObese is a saturation-forward character synthesis engine with a MOJO control "
                "as its defining feature — an orthogonal analog/digital axis that determines "
                "the fundamental tonal character before any effects are applied. "
                "The 'fat_' parameter prefix reflects its core philosophy: fullness first."
            ),
            "historical_homage": (
                "Inspired by the Roland Juno-106 and Oberheim OB-X — synthesizers whose "
                "character came from harmonic saturation in the signal path, not just "
                "filter cutoff. XObese takes that analog-warmth tradition and makes saturation "
                "a primary synthesis parameter, not a post-processing tool."
            ),
            "cultural_context": (
                "Saturation and harmonic density define the difference between thin and full "
                "in music production. XObese is built for producers who want presence before "
                "they start mixing — sounds that sit in the mix without fighting for space. "
                "The MOJO axis gives producers language for what was previously only instinct."
            ),
            "use_cases": [
                "Fat pads and chords that fill frequency spectrum without post-processing",
                "Saturated leads with MOJO analog character dialed to taste",
                "Layered textures using the MOJO axis to differentiate harmonic density across layers",
            ],
            "coupling_recommendations": [
                "OBLONG: OBLONG bass routed into OBESE for saturation before the mix",
                "OVERDUB: OBESE MOJO output feeds OVERDUB Drive input parameter",
                "ORBITAL: OBESE density drives ORBITAL GROUP sweep depth",
            ],
            "doctrine_compliance": "B015 — Mojo Control (Blessing 15): orthogonal analog/digital axis.",
            "sound_design_notes": _stub(
                "Write 150 words on XObese's MOJO control — what the analog vs. digital poles "
                "mean sonically, how saturation interacts with the filter, and why OBESE "
                "presets sound different from the same patch without the fat_ prefix engines."
            ),
            "sonic_dna": {"brightness": 0.55, "warmth": 0.85, "movement": 0.40, "density": 0.90, "space": 0.35, "aggression": 0.70},
            "credit": "XO_OX Designs — xo-ox.org",
        },

        # ------------------------------------------------------------------
        # ORACLE — XOracle (Prophecy Indigo #4B0082)
        # ------------------------------------------------------------------
        "ORACLE": {
            "engine": "ORACLE",
            "full_name": "XOracle",
            "creature": "Cuttlefish — Stochastic pattern painter with intelligent memory",
            "accent": "#4B0082",
            "param_prefix": "oracle_",
            "title": "XOracle — Stochastic & Generative Synthesis Engine",
            "summary": (
                "XOracle generates sound through GENDY stochastic synthesis and Maqam-informed "
                "pitch organization — two systems that introduce controlled randomness and "
                "microtonal intelligence into synthesis. Each note is a prediction, each "
                "phrase is a probability field shaped by the oracle's breakpoints."
            ),
            "historical_homage": (
                "Homage to Iannis Xenakis and his GENDY stochastic synthesis system (1991), "
                "and to the Maqam tradition of Arabic and Turkish classical music — a "
                "microtonal framework developed across 1,000 years of musical culture. "
                "ORACLE scores 8.6/10 from the ghost council; Buchla gave it a 10/10."
            ),
            "cultural_context": (
                "Stochastic synthesis was Xenakis's application of probability theory to "
                "sound generation — chaos as compositional material. XOracle pairs this "
                "Western avant-garde approach with Maqam's deeply human microtonal grammar, "
                "creating a generative engine that sounds both ancient and alien simultaneously."
            ),
            "use_cases": [
                "Generative melodic phrases in Arabic and Turkish Maqam scales",
                "GENDY-style stochastic timbral evolution for unpredictable pads and leads",
                "Microtonal composition with controlled randomness via breakpoint probability fields",
            ],
            "coupling_recommendations": [
                "ORGANON: ORACLE stochastic output feeds ORGANON metabolic modulation",
                "OPAL: ORACLE pitch field drives OPAL grain position scatter",
                "ODYSSEY: ORACLE melodic output routes into ODYSSEY wavetable position",
            ],
            "doctrine_compliance": "B010 — GENDY Stochastic Synthesis + Maqam (Blessing 10): Buchla gave 10/10.",
            "sound_design_notes": _stub(
                "Write 150 words on XOracle's GENDY breakpoint system, how Maqam scale "
                "selection interacts with stochastic pitch generation, and what the "
                "oracle_breakpoints parameter actually controls."
            ),
            "sonic_dna": {"brightness": 0.60, "warmth": 0.40, "movement": 0.85, "density": 0.55, "space": 0.70, "aggression": 0.45},
            "credit": "XO_OX Designs — xo-ox.org",
        },

        # ------------------------------------------------------------------
        # ORGANON — XOrganon (Bioluminescent Cyan #00CED1)
        # ------------------------------------------------------------------
        "ORGANON": {
            "engine": "ORGANON",
            "full_name": "XOrganon",
            "creature": "Bioluminescent Anglerfish — Metabolic deep-sea signal processor",
            "accent": "#00CED1",
            "param_prefix": "organon_",
            "title": "XOrganon — Metabolic Synthesis Engine",
            "summary": (
                "XOrganon models synthesis as a biological metabolism — parameters like "
                "metabolicRate, energy, and homeostasis drive a system that breathes, "
                "self-regulates, and responds to stimulus as a living organism. The Variational "
                "Free Energy metabolism model is so rigorous it earned unanimous approval from "
                "the ghost council as a publishable academic paper."
            ),
            "historical_homage": (
                "Inspired by Karl Friston's Variational Free Energy framework and the tradition "
                "of biologically-inspired synthesis from composer and theorist Herbert Brün. "
                "XOrganon extends this into an actual synthesis engine where metabolic "
                "parameters govern DSP states, not just metaphorically but mathematically."
            ),
            "cultural_context": (
                "Biological modeling in synthesis has always been aspirational — most bio-inspired "
                "synths use the metaphor without the math. XOrganon uses the math. The engine "
                "is the first in XOmnibus to earn a Blessing for publishable academic novelty, "
                "and it still makes music you can dance to."
            ),
            "use_cases": [
                "Evolving ambient textures that self-regulate and breathe autonomously",
                "Coupling hub: metabolicRate drives multiple connected engine parameters",
                "Scientific sound design — sonification of biological and mathematical processes",
            ],
            "coupling_recommendations": [
                "ORACLE: ORGANON metabolic rate drives ORACLE stochastic probability field",
                "ORBITAL: ORGANON energy level drives ORBITAL GROUP envelope depth",
                "OUROBOROS: ORGANON homeostasis signal feeds OUROBOROS leash parameter",
            ],
            "doctrine_compliance": "B011 — Variational Free Energy Metabolism (Blessing 11): unanimous; publishable as paper.",
            "sound_design_notes": _stub(
                "Write 150 words on XOrganon's metabolic parameters: metabolicRate, energy, "
                "homeostasis, and how the Variational Free Energy model produces synthesis "
                "behavior that standard ADSR+LFO systems cannot replicate."
            ),
            "sonic_dna": {"brightness": 0.55, "warmth": 0.60, "movement": 0.90, "density": 0.65, "space": 0.70, "aggression": 0.30},
            "credit": "XO_OX Designs — xo-ox.org",
        },

        # ------------------------------------------------------------------
        # OUROBOROS — XOuroboros (Strange Attractor Red #FF2D2D)
        # ------------------------------------------------------------------
        "OUROBOROS": {
            "engine": "OUROBOROS",
            "full_name": "XOuroboros",
            "creature": "Ouroboros Serpent — Chaos contained by a leash, always almost escaping",
            "accent": "#FF2D2D",
            "param_prefix": "ouro_",
            "title": "XOuroboros — Chaotic Attractor Synthesis Engine",
            "summary": (
                "XOuroboros is built on the mathematics of chaotic systems — strange attractors, "
                "Lorenz equations, and feedback topologies that produce deterministic but "
                "unpredictable sonic behavior. The Leash Mechanism is its defining feature: "
                "a parameter that keeps the chaos musical by constraining how far it can wander."
            ),
            "historical_homage": (
                "Homage to Edward Lorenz and his 1963 strange attractor equations that founded "
                "chaos theory, and to the electronic music tradition of feedback synthesis "
                "from Pauline Oliveros and early Buchla experimentalists. The Leash mechanism "
                "is XOuroboros's invention: a musical solution to the control problem of chaos."
            ),
            "cultural_context": (
                "Chaos in music has always been in tension with repetition — the organizing "
                "principle of all groove. XOuroboros resolves this tension with the Leash, "
                "which lets chaos inform timbre without destroying meter. The result sounds "
                "like something alive: never exactly repeating, never completely lost."
            ),
            "use_cases": [
                "Chaotic leads and pads that evolve without repetition yet remain musical",
                "Velocity coupling source — ouro_topology outputs feed other engine mod targets",
                "Feedback synthesis experiments with the Leash as a safety parameter",
            ],
            "coupling_recommendations": [
                "ORGANON: OUROBOROS chaos signal feeds ORGANON homeostasis input",
                "OPAL: OUROBOROS attractor position drives OPAL grain scatter",
                "ORACLE: OUROBOROS velocity coupling into ORACLE stochastic probability",
            ],
            "doctrine_compliance": "B003 — Leash Mechanism (Blessing 3); B007 — Velocity Coupling Outputs (Blessing 7).",
            "sound_design_notes": _stub(
                "Write 150 words on XOuroboros's Leash Mechanism, how ouro_topology "
                "selects different strange attractor geometries, and how velocity coupling "
                "outputs make OUROBOROS a unique modulation source engine."
            ),
            "sonic_dna": {"brightness": 0.65, "warmth": 0.30, "movement": 0.95, "density": 0.70, "space": 0.50, "aggression": 0.85},
            "credit": "XO_OX Designs — xo-ox.org",
        },

        # ------------------------------------------------------------------
        # OVERBITE — XOverbite (Fang White #F0EDE8)
        # ------------------------------------------------------------------
        "OVERBITE": {
            "engine": "OVERBITE",
            "full_name": "XOverbite",
            "creature": "Opossum — Bass-forward, plays dead, bites when ready",
            "accent": "#F0EDE8",
            "param_prefix": "poss_",
            "title": "XOverbite — Bass-Forward Character Synthesizer",
            "summary": (
                "XOverbite is a bass-forward character synthesizer with 342 factory presets "
                "and five performance macros: BELLY, BITE, SCURRY, TRASH, and PLAY DEAD. "
                "The 'poss_' parameter prefix is frozen for backward compatibility with all "
                "342 presets. The Gallery code is OVERBITE; the instrument accent is Fang White."
            ),
            "historical_homage": (
                "Homage to the Korg MS-20 and its aggressive, distinctive bass character — "
                "the synthesizer that defined 'gnarly' before that word existed in synth circles. "
                "XOverbite's five-macro system earns Blessing 8 from the ghost council, "
                "specifically cited by all 8 legacy synthesizer ghosts."
            ),
            "cultural_context": (
                "Bass-forward synthesis defines hip hop, trap, and any genre where the low "
                "end carries emotional weight. XOverbite's PLAY DEAD macro is a performance "
                "innovation — a macro that kills the signal and waits, creating silence as "
                "a deliberate compositional element rather than an absence."
            ),
            "use_cases": [
                "Bass leads with the BITE macro for aggressive filter articulation",
                "BELLY macro for warm low-end emphasis in pad and chord contexts",
                "PLAY DEAD performance trick — silence as composition, drop as statement",
            ],
            "coupling_recommendations": [
                "OVERDUB: OVERBITE bass into OVERDUB dub echo chain",
                "OBLONG: OVERBITE and OBLONG bass layers — BITE vs. warmth character contrast",
                "ONSET: OVERBITE SCURRY macro synced with ONSET kick for locked groove",
            ],
            "doctrine_compliance": "B008 — Five-Macro System BELLY/BITE/SCURRY/TRASH/PLAY DEAD (Blessing 8): all 8 ghosts.",
            "sound_design_notes": _stub(
                "Write 150 words on XOverbite's five-macro system and how BELLY/BITE/SCURRY/"
                "TRASH/PLAY DEAD each address a different expressive dimension of bass synthesis."
            ),
            "sonic_dna": {"brightness": 0.40, "warmth": 0.70, "movement": 0.50, "density": 0.75, "space": 0.30, "aggression": 0.80},
            "credit": "XO_OX Designs — xo-ox.org",
        },

        # ------------------------------------------------------------------
        # OVERWORLD — XOverworld (Neon Green #39FF14)
        # ------------------------------------------------------------------
        "OVERWORLD": {
            "engine": "OVERWORLD",
            "full_name": "XOverworld",
            "creature": "Neon Tetra School — Chip-sync collective moving as one",
            "accent": "#39FF14",
            "param_prefix": "ow_",
            "title": "XOverworld — Chip Synthesis & ERA Crossfade Engine",
            "summary": (
                "XOverworld is a chip synthesizer modeled on three classic game console "
                "sound chips: NES 2A03, Sega Genesis YM2612, and SNES SPC700. Its ERA "
                "triangle gives producers a 2D crossfade between the timbral character of "
                "each console era, with the ow_era parameter as the defining control."
            ),
            "historical_homage": (
                "Homage to Koji Kondo (Nintendo), Yuzo Koshiro (Sega), and Nobuo Uematsu "
                "(Square) — the composers who built entire musical universes within the "
                "constraints of 4-bit and 8-bit sound hardware. XOverworld honors their "
                "craft by modeling the physics of each chip, not just its output character."
            ),
            "cultural_context": (
                "Chiptune is simultaneously the most constrained and most beloved synthesis "
                "tradition — music made from hardware limitations becomes its own aesthetic. "
                "XOverworld's ERA triangle lets producers blend between console eras as "
                "timbral dimensions, making the constraint a compositional variable."
            ),
            "use_cases": [
                "Chiptune leads and arpeggios with authentic NES/Genesis/SNES chip textures",
                "ERA crossfade sweeps — from 8-bit NES purity to FM-rich YM2612 complexity",
                "CRT-filtered ambient textures using the 15 color themes and glitch types",
            ],
            "coupling_recommendations": [
                "OPAL: OVERWORLD chip patterns feed OPAL grain scatter for chip-granular textures",
                "ONSET: OVERWORLD arp triggers ONSET percussion for chip+drums locked sync",
                "OBLONG: OVERWORLD leads layered with OBLONG bass for full chiptune production",
            ],
            "doctrine_compliance": "B009 — ERA Triangle (Blessing 9): 2D timbral crossfade, praised by Buchla/Schulze/Vangelis/Pearlman.",
            "sound_design_notes": _stub(
                "Write 150 words on XOverworld's ERA triangle — how NES, YM2612, and SNES "
                "SPC700 differ sonically, what the ow_era parameter controls, and why "
                "the 2D crossfade approach produces sounds no single chip engine can."
            ),
            "sonic_dna": {"brightness": 0.80, "warmth": 0.25, "movement": 0.65, "density": 0.45, "space": 0.40, "aggression": 0.60},
            "credit": "XO_OX Designs — xo-ox.org",
        },

        # ------------------------------------------------------------------
        # OHM — XOhm (Sage #87AE73)
        # ------------------------------------------------------------------
        "OHM": {
            "engine": "OHM",
            "full_name": "XOhm",
            "creature": "Sea Anemone — Commune of waving tendrils, ohm-resonant",
            "accent": "#87AE73",
            "param_prefix": "ohm_",
            "title": "XOhm — Communal Meditation & Drone Synthesizer",
            "summary": (
                "XOhm is a drone and meditation synthesizer designed around the MEDDLING/COMMUNE "
                "axis — a macro that moves from intrusive harmonic intervention to total "
                "communal resonance. Its character is the Hippy Dad jam: warm, patient, "
                "seeking harmonic convergence rather than timbral aggression."
            ),
            "historical_homage": (
                "Homage to La Monte Young and his drone music investigations from 1960 onward, "
                "and to the tambura tradition of Indian classical music — an instrument whose "
                "entire purpose is to sustain a harmonic foundation for other voices. "
                "XOhm makes the drone expressive, not static."
            ),
            "cultural_context": (
                "Drone music is meditation music, healing music, the music of sustained "
                "resonance — a tradition spanning Indian raga, Tibetan singing bowls, and "
                "Western minimalism. XOhm gives producers a synthesizer engine where "
                "patience and stillness are first-class sound design values."
            ),
            "use_cases": [
                "Sustained drone pads for meditation, ambient, and ceremonial contexts",
                "COMMUNE macro — full harmonic lock-in for lush, beating resonance clouds",
                "MEDDLING macro — harmonic disturbance as compositional tension and release",
            ],
            "coupling_recommendations": [
                "ORGANON: OHM drone feeds ORGANON metabolic modulation as sustained carrier",
                "OPAL: OHM drone sustain into OPAL grain cloud for layered resonance",
                "ORACLE: OHM harmonic field sets tonal center for ORACLE Maqam generation",
            ],
            "doctrine_compliance": "D005 — LFO rate floor ≤ 0.01 Hz; D006 — mod wheel controls COMMUNE depth.",
            "sound_design_notes": _stub(
                "Write 150 words on XOhm's MEDDLING/COMMUNE axis, what each pole sounds "
                "like in practice, and how the drone architecture differs from a standard "
                "pad synthesizer."
            ),
            "sonic_dna": {"brightness": 0.30, "warmth": 0.85, "movement": 0.20, "density": 0.60, "space": 0.90, "aggression": 0.10},
            "credit": "XO_OX Designs — xo-ox.org",
        },

        # ------------------------------------------------------------------
        # ORPHICA — XOrphica (Siren Seafoam #7FDBCA)
        # ------------------------------------------------------------------
        "ORPHICA": {
            "engine": "ORPHICA",
            "full_name": "XOrphica",
            "creature": "Siphonophore — Colony instrument, each member a microsound harp string",
            "accent": "#7FDBCA",
            "param_prefix": "orph_",
            "title": "XOrphica — Microsound Harp Synthesis Engine",
            "summary": (
                "XOrphica is a microsound harp synthesizer that treats each partial of a "
                "plucked string as an independent microsound particle. The pluck brightness "
                "parameter (orph_pluckBrightness) governs transient spectral character; "
                "the siphonophore model means the instrument is a colony of voices "
                "coordinating as a single sonic organism."
            ),
            "historical_homage": (
                "Homage to Orpheus and the mythological harp tradition, and to Curtis Roads' "
                "microsound theory applied to plucked string synthesis. The siphonophore "
                "biology metaphor — a colony that is simultaneously individual and collective "
                "— maps directly onto how harp partials cooperate in physical reality."
            ),
            "cultural_context": (
                "The harp is one of the oldest instruments in human music, appearing across "
                "every major culture independently. XOrphica uses this universal instrument "
                "as the basis for a microsound exploration that honors the harp's mythology "
                "while extending its timbral vocabulary into electronic territory."
            ),
            "use_cases": [
                "Ethereal plucked textures with microsound grain-level articulation",
                "Colony-voice pads where each voice is an independent pluck partial",
                "Coupling source: pluck transient brightness drives other engine filter attacks",
            ],
            "coupling_recommendations": [
                "OVERDUB: ORPHICA pluck feeds OVERDUB spring reverb for natural resonance bloom",
                "OPAL: ORPHICA partials route into OPAL grain cloud for string-granular fusion",
                "OHM: ORPHICA melodic plucks over OHM drone — ancient harp + drone meditation",
            ],
            "doctrine_compliance": "D001 — velocity drives pluck brightness; D006 — aftertouch controls sustain bloom.",
            "sound_design_notes": _stub(
                "Write 150 words on XOrphica's microsound harp model, what orph_pluckBrightness "
                "controls at different values, and how the siphonophore colony voice architecture "
                "produces harp textures that single-voice models cannot."
            ),
            "sonic_dna": {"brightness": 0.70, "warmth": 0.55, "movement": 0.50, "density": 0.40, "space": 0.75, "aggression": 0.20},
            "credit": "XO_OX Designs — xo-ox.org",
        },

        # ------------------------------------------------------------------
        # OBBLIGATO — XObbligato (Rascal Coral #FF8A7A)
        # ------------------------------------------------------------------
        "OBBLIGATO": {
            "engine": "OBBLIGATO",
            "full_name": "XObbligato",
            "creature": "Mantis Shrimp — Dual-punching mandatory presence",
            "accent": "#FF8A7A",
            "param_prefix": "obbl_",
            "title": "XObbligato — Dual Wind Instrument Synthesis Engine",
            "summary": (
                "XObbligato is a dual wind instrument synthesizer built around two breath "
                "models (obbl_breathA and obbl_breathB) that can blend, counterpoint, or "
                "lock together via the BOND macro. The name means 'mandatory' — this engine "
                "insists on being heard, its two voices demanding presence in the mix."
            ),
            "historical_homage": (
                "Homage to the oboe d'amore and bassoon duet tradition in Baroque music, "
                "and to the Yamaha WX11 MIDI wind controller that brought physical breath "
                "to electronic synthesis. OBBLIGATO's dual breath architecture honors the "
                "human requirement for air as the driver of musical expression."
            ),
            "cultural_context": (
                "Wind instruments are the most directly human of all instrument families — "
                "breath is life, and putting breath into a synthesizer means modeling the "
                "most intimate expressive gesture. OBBLIGATO's dual breath model lets "
                "two breath sources counterpoint, harmonize, or chase each other."
            ),
            "use_cases": [
                "Woodwind-style leads with authentic breath pressure articulation",
                "BOND macro — lock both breath voices into harmonic unison for thick wind chords",
                "Counterpoint textures using independent breathA and breathB modulation",
            ],
            "coupling_recommendations": [
                "OHM: OBBLIGATO wind voices over OHM drone foundation",
                "ORPHICA: OBBLIGATO breath over ORPHICA harp plucks — woodwind + harp ensemble",
                "ORBITAL: OBBLIGATO BOND macro drives ORBITAL GROUP envelope for coordinated swells",
            ],
            "doctrine_compliance": "D006 — breath pressure CC controls breathA/breathB directly.",
            "sound_design_notes": _stub(
                "Write 150 words on XObbligato's dual breath model — how breathA and breathB "
                "differ in character, what the BOND macro does at different settings, and "
                "what genres benefit most from the mandatory-presence wind texture."
            ),
            "sonic_dna": {"brightness": 0.65, "warmth": 0.60, "movement": 0.55, "density": 0.50, "space": 0.55, "aggression": 0.45},
            "credit": "XO_OX Designs — xo-ox.org",
        },

        # ------------------------------------------------------------------
        # OTTONI — XOttoni (Patina #5B8A72)
        # ------------------------------------------------------------------
        "OTTONI": {
            "engine": "OTTONI",
            "full_name": "XOttoni",
            "creature": "Chambered Nautilus Trio — Three brass bells resonating in formation",
            "accent": "#5B8A72",
            "param_prefix": "otto_",
            "title": "XOttoni — Triple Brass Synthesis Engine",
            "summary": (
                "XOttoni models three simultaneous brass voices — trumpet, trombone, and "
                "French horn — synthesized through physical brass bore and bell models. "
                "The GROW macro drives coordinated dynamic expansion across all three "
                "voices, producing orchestral brass swell behavior from a single gesture."
            ),
            "historical_homage": (
                "Homage to the Italian Renaissance brass ensemble tradition (ottoni = 'brasses' "
                "in Italian), and to the orchestral brass writing of Gustav Mahler — a "
                "composer who treated the brass section as a unified organism capable of "
                "both whisper and thunder. XOttoni's GROW macro is Mahler's crescendo in code."
            ),
            "cultural_context": (
                "Brass instruments developed from animal horns to metal tubes to valved "
                "instruments over 5,000 years. XOttoni compresses this entire lineage into "
                "three physical models that can be played simultaneously, giving solo "
                "producers access to the full harmonic weight of a brass section."
            ),
            "use_cases": [
                "Orchestral brass stabs and swells with GROW macro for coordinated crescendo",
                "Soul and funk horn sections using all three voices in tight voicings",
                "Cinematic brass textures with the physical bore models for authentic air-column resonance",
            ],
            "coupling_recommendations": [
                "OBBLIGATO: OTTONI brass and OBBLIGATO wind — full wind ensemble simulation",
                "ORBITAL: OTTONI GROW macro drives ORBITAL GROUP envelope for synchronized swell",
                "OVERDUB: OTTONI brass into OVERDUB spring reverb for vintage brass hall sound",
            ],
            "doctrine_compliance": "D001 — velocity drives brass lip tension across all three voice models.",
            "sound_design_notes": _stub(
                "Write 150 words on XOttoni's three brass models — what distinguishes trumpet, "
                "trombone, and horn synthesis in the physical model, what otto_macroGrow "
                "controls, and what musical contexts the triple brass architecture serves best."
            ),
            "sonic_dna": {"brightness": 0.70, "warmth": 0.55, "movement": 0.55, "density": 0.65, "space": 0.60, "aggression": 0.65},
            "credit": "XO_OX Designs — xo-ox.org",
        },

        # ------------------------------------------------------------------
        # OLE — XOlé (Hibiscus #C9377A)
        # ------------------------------------------------------------------
        "OLE": {
            "engine": "OLE",
            "full_name": "XOlé",
            "creature": "Flamingo Colony — Afro-Latin percussive elegance",
            "accent": "#C9377A",
            "param_prefix": "ole_",
            "title": "XOlé — Afro-Latin Trio Percussion Synthesizer",
            "summary": (
                "XOlé is an Afro-Latin trio synthesizer — three percussion voices (congas, "
                "claves, and cowbell) modeled through physical membrane and shell resonance. "
                "The DRAMA macro drives collective intensity across the trio, moving from "
                "gentle groove to full percussive drama in a single gesture."
            ),
            "historical_homage": (
                "Homage to the Cuban son and Afro-Cuban rumba traditions — the root music "
                "that gave Latin jazz, salsa, and bossa nova their rhythmic vocabulary. "
                "Also honors Tito Puente and Mongo Santamaria — percussionists who brought "
                "Afro-Cuban rhythm to international audiences. XOlé synthesizes the physics "
                "of their instruments, not just their samples."
            ),
            "cultural_context": (
                "Afro-Latin percussion is the rhythmic foundation of half the popular music "
                "on earth — the clave rhythm pattern underlies reggaeton, salsa, jazz, and "
                "hip hop in ways producers often don't consciously recognize. XOlé gives "
                "these rhythmic traditions physical synthesis models: not samples of the past, "
                "but living models of the instruments themselves."
            ),
            "use_cases": [
                "Afro-Cuban percussion synthesis with authentic conga, clave, and cowbell physical models",
                "DRAMA macro for live percussive intensity sweeps in DJ and live contexts",
                "Coupling source: OLE rhythm output drives other engine groove parameters",
            ],
            "coupling_recommendations": [
                "ONSET: OLE percussion alongside ONSET drum synthesis — full rhythm section",
                "OHM: OLE groove over OHM drone — Afro-Latin meditation fusion",
                "OVERDUB: OLE percussion into OVERDUB dub echo for reggaeton flavor",
            ],
            "doctrine_compliance": "D001 — velocity drives membrane strike intensity across trio voices.",
            "sound_design_notes": _stub(
                "Write 150 words on XOlé's three physical percussion models — how conga, "
                "clave, and cowbell synthesis differs, what ole_macroDrama controls at "
                "different settings, and the cultural significance of preserving physical "
                "modeling over sampling for these instruments."
            ),
            "sonic_dna": {"brightness": 0.65, "warmth": 0.60, "movement": 0.80, "density": 0.55, "space": 0.40, "aggression": 0.65},
            "credit": "XO_OX Designs — xo-ox.org",
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
            import re
            slug = self.pack_name.lower()
            slug = re.sub(r"[^a-z0-9]+", "_", slug).strip("_")
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
            if not any(k in sec for k in ("title", "engine", "name", "quad", "collection")):
                errors.append(f"Section {i} (type={sec_type!r}): no identifying field (title/engine/name/quad/collection)")

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
