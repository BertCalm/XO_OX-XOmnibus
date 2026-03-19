#!/usr/bin/env python3
"""
Universal C++ → .xometa Preset Extractor

Extracts factory presets from XOverdub, XOdyssey, and XOblong C++ source
files and converts them to the unified .xometa format for XOmnibus.

Each engine uses a different C++ pattern:
  - XOverdub:   setParam(a, paramId, value) inside lambdas
  - XOdyssey:   {"param_id", value} pairs in vectors
  - XOblong: ParamOverride { "param_id", value } in static array

Usage:
    python3 extract_cpp_presets.py [--engine xoverdub|xodyssey|xoblongbob|all] [--dry-run]
"""

import json
import re
import sys
from pathlib import Path
from datetime import date

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------

REPO_ROOT = Path(__file__).parent.parent
OUTPUT_DIR = REPO_ROOT / "Presets" / "XOmnibus"
TODAY = date.today().isoformat()

ENGINE_SOURCES = {
    "XOverdub": Path.home() / "Documents" / "GitHub" / "XOverdub" / "src" / "PluginProcessor.cpp",
    "XOdyssey": Path.home() / "Documents" / "GitHub" / "XOdyssey" / "src" / "preset",
    "XOblong": Path.home() / "Documents" / "GitHub" / "XOblong" / "src" / "FactoryPresets.h",
}

# Engine-specific macro labels
MACRO_LABELS = {
    "XOverdub":   ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
    "XOdyssey":   ["JOURNEY", "BREATHE", "BLOOM", "FRACTURE"],
    "XOblong": ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"],
}

# Category → XOmnibus mood mapping (from mega_tool_preset_philosophy.md)
MOOD_MAP = {
    # XOverdub
    "Dub Bass":         "Foundation",
    "Log Drums":        "Foundation",
    "Dub Leads":        "Prism",
    "Dub Chords":       "Prism",
    "Echo FX":          "Flux",
    "Tape Chaos":       "Flux",
    "Dub Atmospheres":  "Atmosphere",
    "Drone Machines":   "Aether",
    # XOdyssey
    "Pads":      "Atmosphere",
    "Leads":     "Prism",
    "Keys":      "Prism",
    "Bass":      "Foundation",
    "Textures":  "Atmosphere",
    # XOblong
    "WarmPads":        "Atmosphere",
    "DreamPads":       "Aether",
    "CuriousMotion":   "Entangled",
    "SoftLeads":       "Prism",
    "WarmBass":        "Foundation",
    "Textures_bob":    "Atmosphere",  # special key to avoid collision
    "LoFiAtmospheres": "Flux",
    "Oddball":         "Flux",
    "Plucks":          "Prism",
    "KeysAndBells":    "Prism",
    "Living":          "Entangled",
}

# XOblong PresetCategory enum → string name
BOB_CATEGORY_ENUM = {
    "PresetCategory::WarmPads":        "WarmPads",
    "PresetCategory::DreamPads":       "DreamPads",
    "PresetCategory::CuriousMotion":   "CuriousMotion",
    "PresetCategory::SoftLeads":       "SoftLeads",
    "PresetCategory::WarmBass":        "WarmBass",
    "PresetCategory::Textures":        "Textures_bob",
    "PresetCategory::LoFiAtmospheres": "LoFiAtmospheres",
    "PresetCategory::Oddball":         "Oddball",
    "PresetCategory::Plucks":          "Plucks",
    "PresetCategory::KeysAndBells":    "KeysAndBells",
    "PresetCategory::Living":          "Living",
}

# Map category keys back to display names for legacy metadata
CATEGORY_DISPLAY = {
    "WarmPads": "Warm Pads",
    "DreamPads": "Dream Pads",
    "CuriousMotion": "Curious Motion",
    "SoftLeads": "Soft Leads",
    "WarmBass": "Warm Bass",
    "Textures_bob": "Textures",
    "LoFiAtmospheres": "Lo-Fi Atmospheres",
    "Oddball": "Oddball",
    "Plucks": "Plucks",
    "KeysAndBells": "Keys & Bells",
    "Living": "Living",
}

# Auto-generate description templates by mood
DESCRIPTION_TEMPLATES = {
    "Foundation": [
        "Solid, grounding {category} tone with {character}.",
        "Deep {category} presence with {character}.",
        "Rooted {category} sound with {character}.",
    ],
    "Atmosphere": [
        "Floating {category} texture with {character}.",
        "Expansive {category} atmosphere with {character}.",
        "Ethereal {category} wash with {character}.",
    ],
    "Entangled": [
        "Reactive {category} motion with {character}.",
        "Cross-modulated {category} energy with {character}.",
        "Living {category} movement with {character}.",
    ],
    "Prism": [
        "Expressive {category} voice with {character}.",
        "Bright {category} character with {character}.",
        "Focused {category} tone with {character}.",
    ],
    "Flux": [
        "Evolving {category} texture with {character}.",
        "Unpredictable {category} motion with {character}.",
        "Glitchy {category} character with {character}.",
    ],
    "Aether": [
        "Vast {category} space with {character}.",
        "Cinematic {category} depth with {character}.",
        "Transcendent {category} atmosphere with {character}.",
    ],
}

# ---------------------------------------------------------------------------
# Parameter analysis for auto-tagging
# ---------------------------------------------------------------------------

def analyze_params_for_tags(params: dict, engine: str) -> list:
    """Generate tags from parameter values."""
    tags = []

    if engine == "XOverdub":
        if params.get("delay_feedback", 0) > 0.8:
            tags.append("self-oscillating")
        if params.get("delay_wow", 0) > 0.3:
            tags.append("wobbly")
        if params.get("delay_wear", 0) > 0.4:
            tags.append("worn")
        if params.get("send_level", 0) > 0.5:
            tags.append("wet")
        if params.get("drive_amount", 1) > 3:
            tags.append("driven")
        if params.get("osc_drift", 0) > 0.2:
            tags.append("drifty")
        if params.get("osc_sub_level", 0) > 0.4:
            tags.append("subby")
        if params.get("reverb_size", 0) > 0.7:
            tags.append("spacious")
        if params.get("pitch_env_depth", 0) > 10:
            tags.append("pitched")
        wave = params.get("osc_wave", 2)
        wave_tags = {0: "sine", 1: "triangle", 2: "saw", 3: "square"}
        tags.append(wave_tags.get(int(wave), "saw"))

    elif engine == "XOdyssey":
        if params.get("drift_depth", 0) > 0.2:
            tags.append("drifty")
        if params.get("haze_amount", 0) > 0.2:
            tags.append("saturated")
        if params.get("shimmer_amount", 0) > 0.2:
            tags.append("shimmery")
        if params.get("fracture_enable", 0) > 0:
            tags.append("fractured")
        if params.get("tidal_depth", 0) > 0.2:
            tags.append("tidal")
        if params.get("reverb_mix", 0) > 0.3:
            tags.append("spacious")
        if params.get("chorus_enable", 0) > 0:
            tags.append("chorus")
        if params.get("osc_a_detune", 0) > 0.2:
            tags.append("supersaw")
        if params.get("filt_b_mix", 0) > 0.1:
            tags.append("formant")
        if params.get("env_amp_attack", 0) > 200:
            tags.append("slow-attack")

    elif engine == "XOblong":
        if params.get("oscA_drift", 0) > 0.2:
            tags.append("drifty")
        if params.get("space_mix", 0) > 0.3:
            tags.append("spacious")
        if params.get("bob_mode", 0) > 0.3:
            tags.append("bob-heavy")
        if params.get("env_attack", 0) < 0.01:
            tags.append("snappy")
        if params.get("env_attack", 0) > 0.5:
            tags.append("slow-attack")
        if params.get("cur_mode", -1) >= 0:
            tags.append("curious")
        if params.get("tex_level", 0) > 0:
            tags.append("textured")
        if params.get("dust_amount", 0) > 0:
            tags.append("dusty")
        if params.get("glide", 0) > 0:
            tags.append("glide")

    # Ensure minimum 3 tags
    while len(tags) < 3:
        tags.append(engine.lower().replace("xo", ""))

    return tags[:8]  # Cap at 8 tags


def guess_character(params: dict, engine: str) -> str:
    """Generate a brief character description from parameters."""
    traits = []

    if engine == "XOverdub":
        if params.get("delay_feedback", 0) > 0.8:
            traits.append("self-oscillating delay")
        elif params.get("send_level", 0) > 0.4:
            traits.append("deep dub echo")
        if params.get("drive_amount", 1) > 3:
            traits.append("driven saturation")
        if params.get("reverb_size", 0) > 0.7:
            traits.append("vast reverb space")
        if params.get("osc_drift", 0) > 0.15:
            traits.append("analog drift")

    elif engine == "XOdyssey":
        if params.get("drift_depth", 0) > 0.2:
            traits.append("voyager drift")
        if params.get("shimmer_amount", 0) > 0.2:
            traits.append("prism shimmer")
        if params.get("haze_amount", 0) > 0.3:
            traits.append("warm haze")
        if params.get("tidal_depth", 0) > 0.3:
            traits.append("tidal pulse")
        if params.get("fracture_enable", 0) > 0:
            traits.append("glitch fracture")

    elif engine == "XOblong":
        if params.get("bob_mode", 0) > 0.2:
            traits.append("bob character")
        if params.get("oscA_drift", 0) > 0.2:
            traits.append("analog drift")
        if params.get("space_mix", 0) > 0.3:
            traits.append("reverb depth")
        if params.get("cur_amount", 0) > 0.3:
            traits.append("curious motion")

    return " and ".join(traits[:2]) if traits else "warm character"


def guess_coupling_intensity(params: dict, engine: str) -> str:
    """Estimate coupling intensity from parameter analysis."""
    if engine == "XOverdub":
        send = params.get("send_level", 0)
        fb = params.get("delay_feedback", 0)
        if send > 0.6 and fb > 0.7:
            return "Deep"
        if send > 0.3:
            return "Moderate"
        return "Light"
    return "None"


# ---------------------------------------------------------------------------
# Engine-specific extractors
# ---------------------------------------------------------------------------

def extract_xoverdub(source_path: Path) -> list:
    """Extract presets from XOverdub PluginProcessor.cpp."""
    text = source_path.read_text()

    presets = []
    # Match each push_back block
    # Pattern: presets.push_back({ "Name", "Category", [](auto& a) { ... }});
    pattern = re.compile(
        r'presets\.push_back\(\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*'
        r'\[.*?\]\s*\(.*?\)\s*\{(.*?)\}'   # lambda: [capture](params){ body }
        r'\s*\}\s*\)\s*;',                  # close struct + push_back
        re.DOTALL
    )

    for match in pattern.finditer(text):
        name = match.group(1)
        category = match.group(2)
        body = match.group(3)

        # Extract setParam calls
        params = {}
        param_pattern = re.compile(r'setParam\s*\(\s*a\s*,\s*(\w+)\s*,\s*([0-9eE.+-]+f?)\s*\)')
        for pm in param_pattern.finditer(body):
            param_name = pm.group(1)
            param_value = float(pm.group(2).rstrip('f'))
            # Map C++ const names to string IDs
            param_id = CPP_TO_ID_XOVERDUB.get(param_name, param_name)
            params[param_id] = param_value

        presets.append({
            "name": name,
            "category": category,
            "params": params,
        })

    return presets


def extract_xodyssey(preset_dir: Path) -> list:
    """Extract presets from XOdyssey preset header files."""
    presets = []
    preset_files = sorted(preset_dir.glob("*Presets.h"))

    for fpath in preset_files:
        text = fpath.read_text()

        # Match: p.push_back({ "Name", "Category", { {pairs...} }});
        # The pairs section contains {"param_id", value} entries
        pattern = re.compile(
            r'p\.push_back\(\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*\{(.*?)\}\s*\}\);',
            re.DOTALL
        )

        for match in pattern.finditer(text):
            name = match.group(1)
            category = match.group(2)
            pairs_text = match.group(3)

            params = {}
            pair_pattern = re.compile(r'\{\s*"([^"]+)"\s*,\s*([0-9eE.+-]+f?)\s*\}')
            for pm in pair_pattern.finditer(pairs_text):
                param_id = pm.group(1)
                param_value = float(pm.group(2).rstrip('f'))
                params[param_id] = param_value

            presets.append({
                "name": name,
                "category": category,
                "params": params,
            })

    return presets


def extract_xoblongbob(source_path: Path) -> list:
    """Extract presets from XOblong FactoryPresets.h."""
    text = source_path.read_text()

    presets = []
    # Match each preset block in the kFactoryPresets array
    # Pattern: { "Name", PresetCategory::CatName, "tags", { { overrides } } }
    pattern = re.compile(
        r'\{\s*"([^"]+)"\s*,\s*(PresetCategory::\w+)\s*,\s*"([^"]*)"\s*,\s*\{(.*?)\}\s*\}(?=\s*[,}])',
        re.DOTALL
    )

    for match in pattern.finditer(text):
        name = match.group(1)
        cat_enum = match.group(2)
        tags_str = match.group(3)
        params_text = match.group(4)

        category = BOB_CATEGORY_ENUM.get(cat_enum, "Oddball")
        tags = [t.strip() for t in tags_str.split(",") if t.strip()]

        params = {}
        param_pattern = re.compile(r'\{\s*"([^"]+)"\s*,\s*([0-9eE.+-]+f?)\s*\}')
        for pm in param_pattern.finditer(params_text):
            param_id = pm.group(1)
            param_value = float(pm.group(2).rstrip('f'))
            params[param_id] = param_value

        presets.append({
            "name": name,
            "category": category,
            "params": params,
            "tags": tags,
        })

    return presets


# XOverdub C++ const name → parameter string ID mapping
CPP_TO_ID_XOVERDUB = {
    "oscWave":        "osc_wave",
    "oscOctave":      "osc_octave",
    "oscTune":        "osc_tune",
    "oscPwm":         "osc_pwm",
    "oscSubLevel":    "osc_sub_level",
    "oscNoiseLevel":  "osc_noise_level",
    "oscDrift":       "osc_drift",
    "oscLevel":       "osc_level",
    "filterMode":     "filter_mode",
    "filterCutoff":   "filter_cutoff",
    "filterResonance":"filter_resonance",
    "filterEnvAmt":   "filter_env_amt",
    "envAttack":      "env_attack",
    "envDecay":       "env_decay",
    "envSustain":     "env_sustain",
    "envRelease":     "env_release",
    "pitchEnvDepth":  "pitch_env_depth",
    "pitchEnvDecay":  "pitch_env_decay",
    "lfoRate":        "lfo_rate",
    "lfoDepth":       "lfo_depth",
    "lfoDest":        "lfo_dest",
    "sendLevel":      "send_level",
    "returnLevel":    "return_level",
    "dryLevel":       "dry_level",
    "driveAmount":    "drive_amount",
    "delayTime":      "delay_time",
    "delaySync":      "delay_sync",
    "delayFeedback":  "delay_feedback",
    "delayWear":      "delay_wear",
    "delayWow":       "delay_wow",
    "delayMix":       "delay_mix",
    "reverbSize":     "reverb_size",
    "reverbDamp":     "reverb_damp",
    "reverbMix":      "reverb_mix",
    "voiceMode":      "voice_mode",
    "voiceGlide":     "voice_glide",
    "masterVolume":   "master_volume",
}


# ---------------------------------------------------------------------------
# Conversion to .xometa
# ---------------------------------------------------------------------------

def to_xometa(preset: dict, engine: str) -> dict:
    """Convert an extracted preset to .xometa format."""
    category = preset["category"]
    mood = MOOD_MAP.get(category, "Entangled")

    params = preset["params"]
    tags = preset.get("tags", analyze_params_for_tags(params, engine))
    character = guess_character(params, engine)
    coupling = guess_coupling_intensity(params, engine)

    # Build description
    templates = DESCRIPTION_TEMPLATES.get(mood, DESCRIPTION_TEMPLATES["Atmosphere"])
    display_cat = CATEGORY_DISPLAY.get(category, category)
    template = templates[hash(preset["name"]) % len(templates)]
    description = template.format(category=display_cat.lower(), character=character)

    xometa = {
        "schema_version": 1,
        "name": preset["name"],
        "mood": mood,
        "engines": [engine],
        "author": "XO_OX",
        "version": "1.0.0",
        "description": description,
        "tags": tags,
        "macroLabels": MACRO_LABELS.get(engine, ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]),
        "couplingIntensity": coupling,
        "tempo": None,
        "created": TODAY,
        "legacy": {
            "sourceInstrument": engine,
            "sourceCategory": CATEGORY_DISPLAY.get(category, category),
            "sourcePresetName": None,
        },
        "parameters": {
            engine: params,
        },
        "coupling": None,
        "sequencer": None,
    }

    return xometa


def validate_xometa(xometa: dict) -> list:
    """Basic validation — returns list of issues."""
    issues = []
    if not xometa.get("name"):
        issues.append("Missing name")
    if len(xometa.get("name", "")) > 30:
        issues.append(f"Name too long: {len(xometa['name'])} chars (max 30)")
    if xometa.get("mood") not in ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "User"]:
        issues.append(f"Invalid mood: {xometa.get('mood')}")
    if not xometa.get("engines"):
        issues.append("Missing engines")
    if len(xometa.get("tags", [])) < 3:
        issues.append(f"Too few tags: {len(xometa.get('tags', []))} (min 3)")
    if not xometa.get("description"):
        issues.append("Missing description")
    return issues


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def process_engine(engine: str, dry_run: bool) -> dict:
    """Extract and convert all presets for one engine."""
    source = ENGINE_SOURCES[engine]

    print(f"\n{'─' * 60}")
    print(f"  {engine}")
    print(f"{'─' * 60}")

    if not source.exists():
        print(f"  ERROR: Source not found: {source}")
        return {"total": 0, "migrated": 0, "warnings": 0, "errors": 1, "by_mood": {}}

    # Extract
    if engine == "XOverdub":
        raw_presets = extract_xoverdub(source)
    elif engine == "XOdyssey":
        raw_presets = extract_xodyssey(source)
    elif engine == "XOblong":
        raw_presets = extract_xoblongbob(source)
    else:
        print(f"  ERROR: Unknown engine: {engine}")
        return {"total": 0, "migrated": 0, "warnings": 0, "errors": 1, "by_mood": {}}

    print(f"  Found {len(raw_presets)} presets in C++ source")

    stats = {"total": len(raw_presets), "migrated": 0, "warnings": 0, "errors": 0, "by_mood": {}}
    all_issues = []
    all_names = set()

    for preset in raw_presets:
        xometa = to_xometa(preset, engine)

        # Validate
        issues = validate_xometa(xometa)
        if issues:
            for issue in issues:
                all_issues.append((xometa["name"], issue))
            stats["warnings"] += len(issues)

        # Check name collision
        name_key = xometa["name"].lower()
        if name_key in all_names:
            all_issues.append((xometa["name"], "DUPLICATE NAME within engine"))
            stats["errors"] += 1
        all_names.add(name_key)

        # Track mood distribution
        mood = xometa["mood"]
        stats["by_mood"][mood] = stats["by_mood"].get(mood, 0) + 1

        # Write
        if not dry_run:
            mood_dir = OUTPUT_DIR / mood
            mood_dir.mkdir(parents=True, exist_ok=True)
            out_path = mood_dir / f"{xometa['name']}.xometa"

            # Avoid overwriting OddfeliX presets
            if out_path.exists():
                # Check if it's from a different engine
                try:
                    existing = json.loads(out_path.read_text())
                    if engine not in existing.get("engines", []):
                        # Name collision with different engine — add suffix
                        out_path = mood_dir / f"{xometa['name']} ({engine}).xometa"
                        all_issues.append((xometa["name"], f"Name collision with {existing['engines']} — saved with suffix"))
                except json.JSONDecodeError:
                    pass

            with open(out_path, "w") as f:
                json.dump(xometa, f, indent=2)
            stats["migrated"] += 1
        else:
            print(f"  [DRY] {preset['category']}/{xometa['name']} → {mood}/{xometa['name']}.xometa")
            stats["migrated"] += 1

    # Report
    print(f"\n  Extracted: {stats['total']}")
    print(f"  Migrated:  {stats['migrated']}")
    print(f"  Warnings:  {stats['warnings']}")
    print(f"  Errors:    {stats['errors']}")
    print(f"  Mood distribution:")
    for mood, count in sorted(stats["by_mood"].items()):
        print(f"    {mood}: {count}")

    if all_issues:
        print(f"  Issues:")
        for name, issue in all_issues:
            print(f"    {name}: {issue}")

    return stats


def main():
    dry_run = "--dry-run" in sys.argv

    # Parse engine filter
    engines_to_run = ["XOverdub", "XOdyssey", "XOblong"]
    if "--engine" in sys.argv:
        idx = sys.argv.index("--engine")
        if idx + 1 < len(sys.argv):
            engine_arg = sys.argv[idx + 1].lower()
            engine_map = {
                "xoverdub": ["XOverdub"],
                "xodyssey": ["XOdyssey"],
                "xoblongbob": ["XOblong"],
                "all": engines_to_run,
            }
            engines_to_run = engine_map.get(engine_arg, engines_to_run)

    if not dry_run:
        OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
        for mood in ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether"]:
            (OUTPUT_DIR / mood).mkdir(exist_ok=True)

    print("=" * 60)
    print("XOmnibus C++ Preset Extractor")
    print(f"Output: {OUTPUT_DIR}")
    print(f"Mode: {'DRY RUN' if dry_run else 'LIVE'}")
    print("=" * 60)

    total_stats = {"total": 0, "migrated": 0, "warnings": 0, "errors": 0, "by_mood": {}}

    for engine in engines_to_run:
        stats = process_engine(engine, dry_run)
        total_stats["total"] += stats["total"]
        total_stats["migrated"] += stats["migrated"]
        total_stats["warnings"] += stats["warnings"]
        total_stats["errors"] += stats["errors"]
        for mood, count in stats["by_mood"].items():
            total_stats["by_mood"][mood] = total_stats["by_mood"].get(mood, 0) + count

    print(f"\n{'=' * 60}")
    print("GRAND TOTAL")
    print(f"{'=' * 60}")
    print(f"Total presets found:   {total_stats['total']}")
    print(f"Successfully migrated: {total_stats['migrated']}")
    print(f"Warnings:              {total_stats['warnings']}")
    print(f"Errors:                {total_stats['errors']}")
    print(f"\nMood distribution (all engines):")
    for mood, count in sorted(total_stats["by_mood"].items()):
        print(f"  {mood}: {count}")

    if not dry_run:
        print(f"\nOutput: {OUTPUT_DIR}")

    return 0 if total_stats["errors"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
