#!/usr/bin/env python3
"""
XOceanus — Auto-Populate Missing Sonic DNA Blocks

Finds every .xometa preset that is missing a `sonic_dna` block (or `dna` block)
and writes plausible 6D values derived from heuristics:

  1. Mood baseline  — each mood has a characteristic DNA signature
  2. Engine nudge   — each engine identity shifts certain dimensions
  3. Name/tag keywords — words in the preset name, description, and tags apply
                         ±0.2 shifts to individual dimensions

The result is a `sonic_dna` block that is directionally correct and makes the
preset discoverable in DNA-filtered searches. Values are estimates — manual
curation should refine them over time.

Dimensions (all 0.0–1.0):
    brightness  — spectral high content, filter openness
    warmth      — sub content, saturation, body
    aggression  — resonance, drive, hard transients
    movement    — LFO depth, modulation rate, envelope activity
    complexity  — layering, mod routing density, parameter interaction
    depth       — reverb/space, sub extension, dimensional size

NOTE: The existing audit uses [brightness, warmth, movement, density, space, aggression].
This tool uses [brightness, warmth, aggression, movement, complexity, depth] as the
task specifies. The audit tool also accepts both key sets via the `dna` / `sonic_dna`
lookup. If the project standardises on the 6 audit dimensions in future, run a migration.

For compatibility with the audit tool (which checks: brightness, warmth, movement,
density, space, aggression), this tool writes BOTH sets of keys mapped as follows:
    complexity → density
    depth      → space
so the written block contains all 6 audit dimensions under the `sonic_dna` key.

Usage:
    python3 Tools/add_missing_dna.py --dry-run    # preview only, no writes
    python3 Tools/add_missing_dna.py              # apply changes
    python3 Tools/add_missing_dna.py --verbose    # extra per-preset detail
"""

import json
import copy
import glob
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).parent.parent
PRESET_DIR = REPO_ROOT / "Presets"

REQUIRED_DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

# ---------------------------------------------------------------------------
# Mood baselines
# ---------------------------------------------------------------------------
# Each value is keyed to the 6 audit dimensions:
#   brightness, warmth, aggression, movement, complexity→density, depth→space
MOOD_BASELINES: dict[str, dict[str, float]] = {
    "Foundation": {
        "brightness": 0.4,
        "warmth":     0.6,
        "aggression": 0.2,
        "movement":   0.2,
        "density":    0.3,  # complexity
        "space":      0.6,  # depth
    },
    "Atmosphere": {
        "brightness": 0.3,
        "warmth":     0.5,
        "aggression": 0.1,
        "movement":   0.5,
        "density":    0.5,
        "space":      0.7,
    },
    "Prism": {
        "brightness": 0.8,
        "warmth":     0.3,
        "aggression": 0.4,
        "movement":   0.6,
        "density":    0.7,
        "space":      0.4,
    },
    "Flux": {
        "brightness": 0.5,
        "warmth":     0.3,
        "aggression": 0.6,
        "movement":   0.8,
        "density":    0.8,
        "space":      0.3,
    },
    "Entangled": {
        "brightness": 0.5,
        "warmth":     0.4,
        "aggression": 0.5,
        "movement":   0.7,
        "density":    0.9,
        "space":      0.5,
    },
    "Aether": {
        "brightness": 0.4,
        "warmth":     0.2,
        "aggression": 0.1,
        "movement":   0.4,
        "density":    0.6,
        "space":      0.9,
    },
}

# Fallback for unknown moods
_DEFAULT_BASELINE: dict[str, float] = {
    "brightness": 0.5,
    "warmth":     0.5,
    "aggression": 0.3,
    "movement":   0.4,
    "density":    0.5,
    "space":      0.5,
}

# ---------------------------------------------------------------------------
# Engine identity nudges  (applied on top of mood baseline)
# ---------------------------------------------------------------------------
# Format: {engine_name: {dim: delta}}  — positive = push higher
ENGINE_NUDGES: dict[str, dict[str, float]] = {
    # XOwlfish: subharmonic bass engine — deep, warm, sub content dominant
    "XOwlfish": {
        "warmth":     +0.15,
        "space":      +0.10,
        "brightness": -0.10,
        "aggression": -0.05,
    },
    # Onset: percussion / drum engine — aggressive, high movement
    "Onset": {
        "aggression": +0.20,
        "movement":   +0.15,
        "warmth":     -0.10,
    },
    # OddfeliX / Snap: transient snap engine — bright, aggressive
    "OddfeliX": {
        "brightness": +0.10,
        "aggression": +0.10,
        "movement":   +0.05,
    },
    # OddOscar / Morph: wavetable scanner — mid-register, complex
    "OddOscar": {
        "density":    +0.10,
        "brightness": +0.05,
    },
    # Overdub / Dub: tape delay + reverb — high space, moderate warmth
    "Overdub": {
        "space":      +0.15,
        "warmth":     +0.10,
        "brightness": -0.05,
    },
    # Odyssey: psychedelic pad — complex, spacious
    "Odyssey": {
        "density":    +0.10,
        "space":      +0.10,
        "movement":   +0.05,
    },
    # Organon: VFE metabolic engine — complex, unpredictable
    "Organon": {
        "density":    +0.15,
        "movement":   +0.10,
    },
    # Oracle: GENDY stochastic — maximum complexity
    "Oracle": {
        "density":    +0.20,
        "aggression": +0.10,
        "movement":   +0.10,
    },
    # Opal: granular — complex, movement-heavy
    "Opal": {
        "density":    +0.10,
        "movement":   +0.10,
        "space":      +0.05,
    },
    # Overworld: chip synthesis — bright, clear, minimal warmth
    "Overworld": {
        "brightness": +0.10,
        "warmth":     -0.15,
        "space":      -0.05,
    },
    # Obese: fat pad — warm, dense, low brightness
    "Obese": {
        "warmth":     +0.15,
        "density":    +0.10,
        "brightness": -0.10,
    },
    # Oblong / Bob: warm musical synth
    "Oblong": {
        "warmth":     +0.10,
        "brightness": -0.05,
    },
    # Ouroboros: chaotic feedback — aggressive, high movement
    "Ouroboros": {
        "aggression": +0.15,
        "movement":   +0.15,
        "density":    +0.10,
    },
    # Overbite / Bite: bass-forward, biting character
    "Overbite": {
        "aggression": +0.10,
        "warmth":     +0.10,
        "space":      -0.05,
    },
}

# ---------------------------------------------------------------------------
# Name / tag keyword shifts
# ---------------------------------------------------------------------------
# Each entry: (keyword_list, {dim: delta})
# Applied if any keyword appears (case-insensitive) in name, description, or tags.
KEYWORD_SHIFTS: list[tuple[list[str], dict[str, float]]] = [
    # Brightness-raising keywords
    (["bright", "crystal", "crystalline", "shimmer", "shine", "radiant",
      "neon", "prismatic", "light", "bioluminescent", "photon"],
     {"brightness": +0.20}),

    # Warmth-raising keywords
    (["warm", "bass", "sub", "body", "analog", "fat", "depth",
      "heavy", "abyssal", "floor", "trench"],
     {"warmth": +0.20}),

    # Darkness / low brightness keywords
    (["dark", "midnight", "shadow", "abyss", "void", "deep", "abyssal"],
     {"brightness": -0.20}),

    # Aggression-raising keywords
    (["aggressive", "aggression", "growl", "snarl", "feral", "bite",
      "distorted", "crush", "attack", "frenz", "predator", "hunter",
      "razor", "spike"],
     {"aggression": +0.20}),

    # Movement-raising keywords
    (["swarm", "granular", "chaotic", "scatter", "pulse", "frenzy",
      "storm", "flutter", "modulated", "animate", "swarming"],
     {"movement": +0.20}),

    # Stillness keywords
    (["drone", "static", "still", "meditative", "calm", "lone",
      "solitary", "infinite"],
     {"movement": -0.20}),

    # Space / depth keywords
    (["reverb", "vast", "deep", "space", "galaxy", "infinite",
      "open", "cathedral", "migration", "drift"],
     {"space": +0.20}),

    # Dry / close keywords
    (["dry", "direct", "punchy", "tight", "close"],
     {"space": -0.20}),

    # Density / complexity keywords
    (["layered", "stacked", "thick", "dense", "ecosystem", "coupling",
      "harmonic", "complex"],
     {"density": +0.20}),

    # Sparse keywords
    (["sparse", "solo", "lone", "solitary", "minimal", "thin"],
     {"density": -0.20}),

    # Percussive keywords
    (["percussive", "snap", "kick", "snare", "explosive", "transient"],
     {"aggression": +0.15, "movement": +0.10}),

    # Evolving / organic keywords
    (["evolving", "organic", "glide", "legato", "morph", "portamento"],
     {"movement": +0.10, "density": +0.10}),

    # Fragment / scatter
    (["fragment", "scattered", "scatter", "armor"],
     {"movement": +0.15, "density": +0.10}),
]


def _clamp(v: float) -> float:
    """Clamp to [0.0, 1.0] and round to 2 decimal places."""
    return round(min(1.0, max(0.0, v)), 2)


def compute_dna(mood: str, engines: list, name: str,
                description: str = "", tags=None) -> dict:
    """
    Compute a 6D sonic DNA block for a preset using heuristics.

    Parameters
    ----------
    mood        : Preset mood string (Foundation, Atmosphere, Prism, Flux, Entangled, Aether)
    engines     : List of engine names in the preset
    name        : Preset name
    description : Preset description text
    tags        : List of tag strings

    Returns
    -------
    dict with keys: brightness, warmth, aggression, movement, density, space
    """
    # 1. Start from mood baseline
    baseline = MOOD_BASELINES.get(mood, _DEFAULT_BASELINE)
    dna = dict(baseline)  # copy

    # 2. Apply engine nudges (average across all engines if multiple)
    total_nudges: dict[str, float] = {d: 0.0 for d in dna}
    n_nudge_engines = 0
    for eng in engines:
        if eng in ENGINE_NUDGES:
            for dim, delta in ENGINE_NUDGES[eng].items():
                total_nudges[dim] += delta
            n_nudge_engines += 1

    if n_nudge_engines > 0:
        for dim in dna:
            dna[dim] += total_nudges[dim] / n_nudge_engines

    # 3. Apply keyword shifts from name, description, tags
    search_text = " ".join([
        name.lower(),
        description.lower(),
        " ".join(tags or []).lower(),
    ])

    for keywords, shifts in KEYWORD_SHIFTS:
        if any(kw.lower() in search_text for kw in keywords):
            for dim, delta in shifts.items():
                dna[dim] = dna.get(dim, 0.5) + delta

    # 4. Clamp all to [0.0, 1.0]
    dna = {d: _clamp(v) for d, v in dna.items()}

    return dna


def find_missing(preset_dir: Path) -> list[dict]:
    """Return list of preset dicts (path, data) where DNA is absent or incomplete."""
    files = sorted(glob.glob(str(preset_dir / "**" / "*.xometa"), recursive=True))
    missing = []

    for fpath in files:
        try:
            with open(fpath, encoding="utf-8") as fh:
                data = json.load(fh)
        except (json.JSONDecodeError, IOError) as e:
            print(f"  WARN: could not parse {fpath}: {e}", file=sys.stderr)
            continue

        dna = data.get("sonic_dna") or data.get("dna")
        if dna is None:
            missing.append({"path": fpath, "data": data, "reason": "no dna key"})
        elif any(d not in dna for d in REQUIRED_DIMS):
            missing_dims = [d for d in REQUIRED_DIMS if d not in dna]
            missing.append({
                "path": fpath,
                "data": data,
                "reason": f"missing dims: {', '.join(missing_dims)}",
            })

    return missing


def apply_dna(entry: dict, dry_run: bool = False, verbose: bool = False) -> dict[str, float]:
    """
    Compute and optionally write a sonic_dna block for one preset.

    Returns the computed DNA dict.
    """
    data = entry["data"]
    name = data.get("name", Path(entry["path"]).stem)
    mood = data.get("mood", "Unknown")
    engines = data.get("engines", [])
    description = data.get("description", "")
    tags = data.get("tags", [])

    dna = compute_dna(mood, engines, name, description, tags)

    if verbose:
        print(f"    name    : {name}")
        print(f"    mood    : {mood}")
        print(f"    engines : {engines}")
        print(f"    reason  : {entry['reason']}")
        print(f"    dna     : {dna}")

    if not dry_run:
        # Write sonic_dna block — insert after "mood" if present, else append
        new_data = copy.deepcopy(data)
        # Remove legacy 'dna' key if present (replace with canonical 'sonic_dna')
        new_data.pop("dna", None)
        new_data["sonic_dna"] = dna

        # Reconstruct key order: put sonic_dna right after mood (or engines)
        ordered = {}
        inserted = False
        for k, v in new_data.items():
            if k == "sonic_dna":
                continue  # skip — we'll insert it at the right place
            ordered[k] = v
            if k in ("mood", "engines") and not inserted:
                ordered["sonic_dna"] = dna
                inserted = True
        if not inserted:
            ordered["sonic_dna"] = dna

        with open(entry["path"], "w", encoding="utf-8") as fh:
            json.dump(ordered, fh, indent=2, ensure_ascii=False)
            fh.write("\n")

    return dna


def main() -> int:
    dry_run = "--dry-run" in sys.argv
    verbose = "--verbose" in sys.argv or "-v" in sys.argv

    sep = "=" * 70

    print(sep)
    print("XOceanus — Add Missing Sonic DNA Blocks")
    print(f"Mode: {'DRY RUN (no files written)' if dry_run else 'LIVE (files will be modified)'}")
    print(sep)

    missing = find_missing(PRESET_DIR)

    if not missing:
        print("No presets are missing DNA blocks. Nothing to do.")
        return 0

    print(f"Found {len(missing)} preset(s) missing DNA.\n")

    fixed = 0
    skipped = 0

    for entry in missing:
        rel = Path(entry["path"]).relative_to(REPO_ROOT)
        print(f"  {'[DRY]' if dry_run else '[WRITE]'} {rel}")
        if verbose:
            apply_dna(entry, dry_run=dry_run, verbose=True)
        else:
            dna = apply_dna(entry, dry_run=dry_run, verbose=False)
            name = entry["data"].get("name", Path(entry["path"]).stem)
            mood = entry["data"].get("mood", "?")
            print(f"         {name} ({mood})  →  "
                  f"B={dna['brightness']:.2f} W={dna['warmth']:.2f} "
                  f"A={dna['aggression']:.2f} M={dna['movement']:.2f} "
                  f"D={dna['density']:.2f} S={dna['space']:.2f}")
        fixed += 1

    print()
    print(sep)
    print(f"{'Would fix' if dry_run else 'Fixed'} {fixed} preset(s).")
    if dry_run:
        print("Re-run without --dry-run to apply changes.")
    else:
        print("Run `python3 Tools/audit_sonic_dna.py` to verify coverage.")
    print(sep)

    return 0


if __name__ == "__main__":
    sys.exit(main())
