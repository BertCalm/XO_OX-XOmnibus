#!/usr/bin/env python3
"""
generate_descriptions.py

Generates evocative 1-2 sentence descriptions for XOlokun presets that are
missing them.

Usage:
    python3 generate_descriptions.py           # dry-run (default)
    python3 generate_descriptions.py --dry-run # same as above
    python3 generate_descriptions.py --apply   # write descriptions to files
"""

import argparse
import glob
import json
import pathlib
import sys

PRESETS_ROOT = pathlib.Path(__file__).parent.parent / "Presets" / "XOlokun"


def generate_description(name: str, mood: str, engines: list, dna: dict, tags: list) -> str:
    """Generate an evocative description from preset metadata."""
    engine = engines[0] if engines else "Unknown"

    # Determine sonic character from DNA
    brightness = dna.get("brightness", 0.5)
    warmth = dna.get("warmth", 0.5)
    movement = dna.get("movement", 0.5)
    density = dna.get("density", 0.5)
    space = dna.get("space", 0.5)
    aggression = dna.get("aggression", 0.5)

    # Build character phrases
    texture_words = []
    if brightness > 0.7:
        texture_words.append("shimmering")
    elif brightness < 0.3:
        texture_words.append("dark")

    if warmth > 0.7:
        texture_words.append("warm")
    elif warmth < 0.3:
        texture_words.append("crystalline")

    if density > 0.7:
        texture_words.append("dense")
    elif density < 0.3:
        texture_words.append("ethereal")

    if movement > 0.7:
        texture_words.append("evolving")
    elif movement < 0.3:
        texture_words.append("still")

    if space > 0.7:
        texture_words.append("expansive")
    elif space < 0.3:
        texture_words.append("intimate")

    if aggression > 0.7:
        texture_words.append("aggressive")
    elif aggression < 0.3:
        texture_words.append("gentle")

    # Pick top 2 most extreme textures
    texture = ", ".join(texture_words[:2]) if texture_words else "balanced"

    # Sound type from tags or DNA
    sound_type = "sound"
    if tags:
        tag_types = {
            "pad": "pad",
            "bass": "bass",
            "lead": "lead",
            "pluck": "pluck",
            "drone": "drone",
            "texture": "texture",
            "percussion": "percussion",
            "bell": "bell",
            "key": "keys",
            "string": "strings",
            "brass": "brass",
            "ambient": "atmosphere",
            "fx": "effect",
            "arp": "arpeggio",
        }
        for tag in tags:
            tag_lower = tag.lower()
            for key, val in tag_types.items():
                if key in tag_lower:
                    sound_type = val
                    break
            if sound_type != "sound":
                break

    # If no tag match, infer from DNA
    if sound_type == "sound":
        if density > 0.6 and movement < 0.4 and space > 0.5:
            sound_type = "pad"
        elif density > 0.7 and brightness < 0.4:
            sound_type = "bass"
        elif brightness > 0.6 and density < 0.4:
            sound_type = "lead"
        elif movement > 0.7:
            sound_type = "texture"
        elif space > 0.8:
            sound_type = "atmosphere"

    # Mood context
    mood_phrases = {
        "Foundation": "Anchored and grounding",
        "Atmosphere": "Spacious and ambient",
        "Prism": "Bright and refractive",
        "Flux": "Restless and kinetic",
        "Aether": "Vast and otherworldly",
        "Family": "Warm and familiar",
        "Submerged": "Deep and pressurized",
        "Entangled": "Interwoven and coupled",
        "Crystalline": "Precise and translucent",
        "Deep": "Subterranean and weighty",
        "Ethereal": "Weightless and dissolving",
        "Kinetic": "Propulsive and forward-moving",
        "Luminous": "Radiant and open",
        "Organic": "Living and breathing",
        "Coupling": "Resonant and interconnected",
    }
    mood_phrase = mood_phrases.get(mood, "Expressive")

    # Build the description
    # Pattern: "[Mood phrase], [texture] [sound type]. [Action phrase based on movement/space]."
    if movement > 0.6:
        action = "Macro sweeps reveal shifting timbral landscapes."
    elif space > 0.6:
        action = "Let it breathe — the space is the instrument."
    elif aggression > 0.6:
        action = "Drives through the mix with presence and bite."
    elif warmth > 0.6:
        action = "Sits warmly in any arrangement without fighting for space."
    else:
        action = "Responds expressively to velocity and modulation."

    description = f"{mood_phrase}, {texture} {sound_type}. {action}"
    return description


def is_description_missing(description) -> bool:
    """Return True if description needs to be generated."""
    if description is None:
        return True
    if not isinstance(description, str):
        return True
    if description.strip() == "":
        return True
    return False


def collect_presets():
    """Return list of (path, data) tuples for all non-quarantine presets."""
    pattern = str(PRESETS_ROOT / "**" / "*.xometa")
    files = glob.glob(pattern, recursive=True)
    presets = []
    for f in sorted(files):
        if "_quarantine" in f:
            continue
        p = pathlib.Path(f)
        try:
            data = json.loads(p.read_text(encoding="utf-8"))
            presets.append((p, data))
        except Exception as e:
            print(f"  [WARN] Could not parse {f}: {e}", file=sys.stderr)
    return presets


def run(apply: bool, sample_count: int = 15):
    presets = collect_presets()
    total = len(presets)

    needs_description = [(p, d) for p, d in presets if is_description_missing(d.get("description"))]
    count_missing = len(needs_description)

    print(f"Scanned {total} presets ({total - count_missing} have descriptions, {count_missing} need them)")
    print()

    if count_missing == 0:
        print("Nothing to do — all presets have descriptions.")
        return

    # Generate descriptions for all missing
    generated = []
    for path, data in needs_description:
        name = data.get("name", path.stem)
        mood = data.get("mood", "")
        engines = data.get("engines") or []
        dna = data.get("dna") or {}
        tags = data.get("tags") or []
        desc = generate_description(name, mood, engines, dna, tags)
        generated.append((path, data, desc))

    # Show samples
    sample_n = min(sample_count, len(generated))
    print(f"Sample descriptions ({sample_n} of {count_missing}):")
    print("-" * 70)
    for path, data, desc in generated[:sample_n]:
        name = data.get("name", path.stem)
        engine = (data.get("engines") or ["?"])[0]
        mood = data.get("mood", "?")
        print(f"  [{engine} / {mood}] {name}")
        print(f"    {desc}")
        print()

    if apply:
        updated = 0
        errors = 0
        for path, data, desc in generated:
            try:
                data["description"] = desc
                path.write_text(
                    json.dumps(data, indent=2, ensure_ascii=False) + "\n",
                    encoding="utf-8",
                )
                updated += 1
            except Exception as e:
                print(f"  [ERROR] Failed to write {path}: {e}", file=sys.stderr)
                errors += 1
        print(f"Summary: {updated} presets updated", end="")
        if errors:
            print(f", {errors} errors", end="")
        print()
    else:
        print(f"(dry-run) Would update {count_missing} presets. Pass --apply to write.")


def main():
    parser = argparse.ArgumentParser(
        description="Generate descriptions for XOlokun presets that are missing them."
    )
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "--dry-run",
        action="store_true",
        default=False,
        help="Preview results without writing files (default behaviour)",
    )
    mode.add_argument(
        "--apply",
        action="store_true",
        default=False,
        help="Write generated descriptions to .xometa files",
    )
    args = parser.parse_args()

    run(apply=args.apply)


if __name__ == "__main__":
    main()
