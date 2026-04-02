#!/usr/bin/env python3
"""
xpn_pack_marketing_copy.py — Generate marketing copy for XO_OX XPN packs.

Reads expansion.json + .xometa presets from a pack directory, detects engines,
computes DNA centroid and mood distribution, then produces 5 types of copy:
  1. Product page description  (150-200 words)
  2. Short description         (50 words)
  3. Feature bullets           (5 bullets)
  4. Social post               (280 chars, Twitter/X compatible)
  5. Email subject line        (50 chars max)

Usage:
  python xpn_pack_marketing_copy.py <pack_dir> [--pack-name override] [--output copy.md]
"""

import argparse
import json
import random
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Engine descriptions — short evocative one-liners for product copy
# ---------------------------------------------------------------------------
ENGINE_LINES = {
    "Oblong":     "warm, characterful melodics from the OBLONG engine",
    "Overdub":    "dub-soaked tape warmth via OVERDUB",
    "Odyssey":    "drifting spectral landscapes from ODYSSEY",
    "Obese":      "thick, saturated analog grit from OBESE",
    "Onset":      "physically-modelled percussion via ONSET",
    "Overworld":  "chip-era nostalgia and ERA crossfade from OVERWORLD",
    "Opal":       "cloud-grain textures from the OPAL granular engine",
    "Organon":    "metabolic, bioluminescent tones from ORGANON",
    "Ouroboros":  "self-feeding chaotic energy from OUROBOROS",
    "Obsidian":   "crystalline stasis from OBSIDIAN",
    "Origami":    "folded harmonic geometry from ORIGAMI",
    "Oracle":     "stochastic maqam-tuned voices from ORACLE",
    "Obscura":    "daguerreotype smear and analog drift from OBSCURA",
    "Oceanic":    "phosphorescent tidal motion from OCEANIC",
    "Ocelot":     "biome-shifting timbral prowl from OCELOT",
    "Overbite":   "fanged bass character from OVERBITE",
    "Orbital":    "warm-red melodic arcs from ORBITAL",
    "Optic":      "visual-modulation rhythm from OPTIC",
    "Oblique":    "prismatic bounce and refracted funk from OBLIQUE",
    "Osprey":     "five-coastline cultural resonance from OSPREY",
    "Osteria":    "Porto-wine warmth and shore textures from OSTERIA",
    "Owlfish":    "abyssal Mixtur-Trautonium tones from OWLFISH",
    "OddfeliX":   "neon-tetra bright leads from ODDFELIX",
    "OddOscar":   "axolotl-gill bass and morph from ODDOSCAR",
    "Ohm":        "sage communal jams from OHM",
    "Orphica":    "microsound harp shimmer from ORPHICA",
    "Obbligato":  "dual wind breath and bond from OBBLIGATO",
    "Ottoni":     "triple brass growth arcs from OTTONI",
    "Ole":        "Afro-Latin rhythm and drama from OLE",
    "Overlap":    "knot-topology FDN diffusion from OVERLAP",
    "Outwit":     "cellular automata evolution from OUTWIT",
    "Ombre":      "shadow-to-light timbral gradient from OMBRE",
    "Orca":       "apex predator wavetable drive from ORCA",
    "Octopus":    "eight-arm decentralised intelligence from OCTOPUS",
}

# ---------------------------------------------------------------------------
# DNA → copy adjectives
# ---------------------------------------------------------------------------
DNA_ADJECTIVES = {
    "brightness": {
        "high":   ("bright, air-kissed",    "crystalline highs and shimmering presence"),
        "mid":    ("balanced, clear",        "a balanced tonal centre"),
        "low":    ("shadowed, murky",        "deep sub-tonal darkness"),
    },
    "warmth": {
        "high":   ("organic, intimate",     "the kind of warmth you feel before you hear"),
        "mid":    ("natural, grounded",     "grounded, natural character"),
        "low":    ("cold, clinical",        "icy precision and digital clarity"),
    },
    "movement": {
        "high":   ("alive, constantly evolving", "patches that breathe, shift and surprise"),
        "mid":    ("gently animated",           "subtle motion that rewards attention"),
        "low":    ("still, meditative",          "patient, static soundscapes"),
    },
    "density": {
        "high":   ("rich, layered",          "dense harmonic stacks built for the mix"),
        "mid":    ("full but focused",       "full-bodied yet mix-ready"),
        "low":    ("sparse, minimal",        "space-conscious single-voice purity"),
    },
    "space":  {
        "high":   ("cavernous, expansive",   "panoramic depth that opens a room"),
        "mid":    ("room-sized, natural",    "natural room ambience"),
        "low":    ("intimate, close",        "up-close, dry presence"),
    },
    "aggression": {
        "high":   ("cutting-edge, fierce",   "aggressive transients that cut through anything"),
        "mid":    ("assertive, forward",     "forward-pushing character"),
        "low":    ("gentle, yielding",       "soft-spoken tonal restraint"),
    },
}

# ---------------------------------------------------------------------------
# Mood descriptor phrases
# ---------------------------------------------------------------------------
MOOD_PHRASES = {
    "Foundation": "solid, production-ready building blocks",
    "Atmosphere": "immersive ambient and textural sound design",
    "Entangled":  "cross-engine coupled complexity",
    "Prism":      "multi-faceted harmonic colour",
    "Flux":       "dynamic, ever-shifting motion",
    "Aether":     "cosmic, weightless space design",
    "Family":     "intimate character voices",
}

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def dna_tier(value: float) -> str:
    if value >= 0.65:
        return "high"
    if value >= 0.35:
        return "mid"
    return "low"


def load_xometa_files(pack_dir: Path) -> list[dict]:
    presets = []
    for p in pack_dir.rglob("*.xometa"):
        try:
            data = json.loads(p.read_text(encoding="utf-8"))
            presets.append(data)
        except (json.JSONDecodeError, OSError):
            pass
    return presets


def load_expansion_json(pack_dir: Path) -> dict:
    candidate = pack_dir / "expansion.json"
    if candidate.exists():
        try:
            return json.loads(candidate.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError):
            pass
    return {}


def compute_centroid(presets: list[dict]) -> dict[str, float]:
    keys = ["brightness", "warmth", "movement", "density", "space", "aggression"]
    totals = {k: 0.0 for k in keys}
    count = 0
    for p in presets:
        dna = p.get("dna") or {}
        if not dna:
            continue
        for k in keys:
            totals[k] += float(dna.get(k, 0.5))
        count += 1
    if count == 0:
        return {k: 0.5 for k in keys}
    return {k: totals[k] / count for k in keys}


def collect_engines(presets: list[dict]) -> list[str]:
    seen: dict[str, int] = {}
    for p in presets:
        for eng in p.get("engines") or []:
            seen[eng] = seen.get(eng, 0) + 1
    return sorted(seen, key=lambda e: seen[e], reverse=True)


def collect_moods(presets: list[dict]) -> dict[str, int]:
    moods: dict[str, int] = {}
    for p in presets:
        m = p.get("mood")
        if m:
            moods[m] = moods.get(m, 0) + 1
    return moods


def dominant_mood(moods: dict[str, int]) -> str:
    if not moods:
        return "Atmosphere"
    return max(moods, key=lambda m: moods[m])


def dna_adjective_pair(centroid: dict[str, float]) -> tuple[str, str]:
    """Return (short_adj, long_phrase) for the most distinctive DNA dimension."""
    # pick the dimension furthest from 0.5
    dim = max(centroid, key=lambda k: abs(centroid[k] - 0.5))
    tier = dna_tier(centroid[dim])
    return DNA_ADJECTIVES[dim][tier]


def engine_sentence(engines: list[str]) -> str:
    if not engines:
        return "XO_OX synthesis technology"
    lines = [ENGINE_LINES.get(e, f"the {e.upper()} engine") for e in engines[:3]]
    # Capitalise just the first character of the full sentence without lowercasing the rest
    if len(lines) == 1:
        s = lines[0]
        return s[0].upper() + s[1:] if s else s
    joined = ", ".join(lines[:-1]) + ", and " + lines[-1]
    return joined[0].upper() + joined[1:] if joined else joined


# ---------------------------------------------------------------------------
# Copy generators
# ---------------------------------------------------------------------------

def gen_product_description(
    pack_name: str,
    preset_count: int,
    engines: list[str],
    centroid: dict[str, float],
    moods: dict[str, int],
    expansion: dict,
) -> str:
    short_adj, long_phrase = dna_adjective_pair(centroid)
    eng_sent = engine_sentence(engines)
    dom_mood = dominant_mood(moods)
    mood_phrase = MOOD_PHRASES.get(dom_mood, "expressive sound design")
    engine_list = ", ".join(e.upper() for e in engines[:4])
    if not engine_list:
        engine_list = "XO_OX engines"
    mood_list = ", ".join(
        m for m, _ in sorted(moods.items(), key=lambda x: x[1], reverse=True)[:3]
    )
    if not mood_list:
        mood_list = "multiple mood categories"

    desc = (
        f"{pack_name} is an XO_OX expansion built for producers who demand {short_phrase(short_adj)} "
        f"sound design that holds up in any mix. {preset_count} hand-crafted presets draw from "
        f"{eng_sent} — each one shaped for {mood_phrase}.\n\n"
        f"The collection centres on {long_phrase}, with presets spanning {mood_list}. "
        f"Every patch carries a full 6D Sonic DNA profile, four mapped macros, and deep "
        f"velocity sensitivity, so expression comes without effort.\n\n"
        f"Engines: {engine_list}. Designed for Akai MPC Live and MPC X, and the XOceanus AU/VST3 plugin. "
        f"Drop it in and play — no menu-diving, no compromise."
    )
    return desc


def short_phrase(adj: str) -> str:
    """Ensure adjective phrase reads naturally inline."""
    return adj


def gen_short_description(
    pack_name: str,
    preset_count: int,
    engines: list[str],
    centroid: dict[str, float],
) -> str:
    short_adj, _ = dna_adjective_pair(centroid)
    engine_list = " + ".join(e.upper() for e in engines[:2]) or "XO_OX engines"
    return (
        f"{pack_name}: {preset_count} {short_adj} presets across {engine_list}. "
        f"Four macros, full DNA, velocity-expressive. "
        f"Ready for MPC and XOceanus. No compromise."
    )


def gen_feature_bullets(
    pack_name: str,
    preset_count: int,
    engines: list[str],
    moods: dict[str, int],
    centroid: dict[str, float],
) -> list[str]:
    engine_str = (
        ", ".join(e.upper() for e in engines[:3])
        if engines else "XO_OX engines"
    )
    engine_count = len(engines)
    mood_count = len(moods)
    short_adj, long_phrase = dna_adjective_pair(centroid)

    bullets = [
        f"✦ {preset_count} hand-crafted presets across {engine_count} XO_OX engine{'s' if engine_count != 1 else ''} ({engine_str})",
        f"✦ {mood_count} mood categor{'ies' if mood_count != 1 else 'y'} — {', '.join(list(moods.keys())[:3])}",
        f"✦ 6D Sonic DNA on every preset — {long_phrase}",
        f"✦ Four mapped macros (CHARACTER, MOVEMENT, COUPLING, SPACE) on every patch",
        f"✦ Velocity-expressive, MPC-ready, XOceanus AU/VST3 compatible",
    ]
    return bullets


def gen_social_post(
    pack_name: str,
    preset_count: int,
    engines: list[str],
    centroid: dict[str, float],
) -> str:
    short_adj, _ = dna_adjective_pair(centroid)
    engine_list = " + ".join(e.upper() for e in engines[:2]) or "XO_OX"
    tags = "#MPCLive #MPC #SoundDesign #XO_OX"

    templates = [
        f"{pack_name} is live — {preset_count} {short_adj} presets from {engine_list}. "
        f"Four macros, full DNA, velocity-expressive. Free on XO-OX.org. {tags}",

        f"New XO_OX pack: {pack_name}. {preset_count} presets, {engine_list} engines, "
        f"{short_adj} character throughout. Drop it on your MPC and go. {tags}",

        f"{pack_name} drops today. {preset_count} patches, {short_adj} sound design, "
        f"{engine_list} under the hood. Built for producers who care. {tags}",
    ]
    post = random.choice(templates)  # noqa: S311
    # Trim to 280 chars if needed
    if len(post) > 280:
        # shorten engine list to first engine only and retry
        engine_short = engines[0].upper() if engines else "XO_OX"
        post = (
            f"{pack_name} — {preset_count} {short_adj} presets from {engine_short}. "
            f"Free on XO-OX.org. {tags}"
        )
    return post[:280]


def gen_email_subject(pack_name: str, centroid: dict[str, float]) -> str:
    short_adj, _ = dna_adjective_pair(centroid)
    templates = [
        f"New pack: {pack_name} — {short_adj} presets",
        f"{pack_name} is here: {short_adj} sound design",
        f"XO_OX drop: {pack_name}",
        f"Fresh pack — {pack_name} ({short_adj})",
    ]
    for t in templates:
        if len(t) <= 50:
            return t
    # fallback hard truncate
    return f"New XO_OX pack: {pack_name}"[:50]


# ---------------------------------------------------------------------------
# Formatting
# ---------------------------------------------------------------------------

def format_output(sections: dict[str, str]) -> str:
    lines = []
    for title, body in sections.items():
        lines.append(f"## {title}")
        lines.append("")
        lines.append(body)
        lines.append("")
    return "\n".join(lines).strip()


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate marketing copy for an XO_OX XPN pack."
    )
    parser.add_argument("pack_dir", help="Path to the pack directory")
    parser.add_argument("--pack-name", dest="pack_name", default=None,
                        help="Override the pack name (default: inferred from expansion.json or directory name)")
    parser.add_argument("--output", default=None,
                        help="Write output to this file (default: stdout)")
    args = parser.parse_args()

    pack_dir = Path(args.pack_dir).expanduser().resolve()
    if not pack_dir.is_dir():
        print(f"Error: '{pack_dir}' is not a directory.", file=sys.stderr)
        sys.exit(1)

    expansion = load_expansion_json(pack_dir)
    presets = load_xometa_files(pack_dir)

    # Determine pack name
    if args.pack_name:
        pack_name = args.pack_name
    elif expansion.get("name"):
        pack_name = expansion["name"]
    else:
        pack_name = pack_dir.name

    preset_count = len(presets)
    if preset_count == 0:
        print("Warning: no .xometa presets found — copy will use placeholder counts.", file=sys.stderr)
        preset_count = expansion.get("presetCount", 0)

    engines = collect_engines(presets)
    if not engines and expansion.get("engines"):
        engines = expansion["engines"]

    moods = collect_moods(presets)
    centroid = compute_centroid(presets)

    # Generate all 5 types
    product_desc = gen_product_description(
        pack_name, preset_count, engines, centroid, moods, expansion
    )
    short_desc = gen_short_description(pack_name, preset_count, engines, centroid)
    bullets = gen_feature_bullets(pack_name, preset_count, engines, moods, centroid)
    social = gen_social_post(pack_name, preset_count, engines, centroid)
    email_subj = gen_email_subject(pack_name, centroid)

    # Build output sections
    sections = {
        "Product Page Description": product_desc,
        "Short Description": short_desc,
        "Feature Bullets": "\n".join(bullets),
        "Social Post (Twitter/X)": f"{social}\n\n[{len(social)} chars]",
        "Email Subject Line": f"{email_subj}\n\n[{len(email_subj)} chars]",
    }

    # DNA summary for reference
    dna_lines = [f"  {k}: {v:.2f} ({dna_tier(v)})" for k, v in centroid.items()]
    sections["DNA Centroid (reference)"] = "\n".join(dna_lines)

    output = format_output(sections)

    if args.output:
        out_path = Path(args.output)
        out_path.write_text(output + "\n", encoding="utf-8")
        print(f"Saved to: {out_path}")
    else:
        print(output)


if __name__ == "__main__":
    main()
