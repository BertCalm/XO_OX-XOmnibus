#!/usr/bin/env python3
"""
xoceanus_preset_migration.py — Two-pass preset migration for XOceanus .xometa files.

Pass 1 (structural fixes):
  - Bare-list coupling → {"pairs": [...]}
  - Wrong pair keys: source→engineA, target/destination→engineB
  - Missing/null couplingIntensity → "Moderate" (when pairs exist)
  - Invalid couplingIntensity strings → mapped canonical values

Pass 2 (coupling type remapping):
  - Maps all non-canonical type strings to valid CamelCase forms
  - Arrow-notation aliases are already valid; only truly invalid strings are mapped

Usage:
  python3 Tools/xoceanus_preset_migration.py --preset-dir Presets/XOceanus --dry-run --all
  python3 Tools/xoceanus_preset_migration.py --preset-dir Presets/XOceanus --execute --all
  python3 Tools/xoceanus_preset_migration.py --preset-dir Presets/XOceanus --execute --pass 1
  python3 Tools/xoceanus_preset_migration.py --preset-dir Presets/XOceanus --execute --pass 2
"""

import argparse
import glob
import json
import os
import shutil
import sys
from collections import Counter, defaultdict
from typing import Any


# ---------------------------------------------------------------------------
# Canonical coupling types (from Source/Core/PresetManager.h validCouplingTypes)
# ---------------------------------------------------------------------------
CANONICAL_TYPES = {
    # CamelCase forms
    "AmpToFilter", "AmpToPitch", "LFOToPitch", "EnvToMorph",
    "AudioToFM", "AudioToRing", "FilterToFilter", "AmpToChoke",
    "RhythmToBlend", "EnvToDecay", "PitchToPitch", "AudioToWavetable",
    "AudioToBuffer", "KnotTopology", "TriangularCoupling",
    # Arrow-notation aliases (accepted by the parser)
    "Amp->Filter", "Amp->Pitch", "LFO->Pitch", "Env->Morph",
    "Audio->FM", "Audio->Ring", "Filter->Filter", "Amp->Choke",
    "Rhythm->Blend", "Env->Decay", "Pitch->Pitch", "Audio->Wavetable",
    "Audio->Buffer", "Knot->Topology", "Triangular->Coupling",
}

# ---------------------------------------------------------------------------
# Coupling type remapping table (invalid → canonical)
# Ordered from most-specific to most-general; first match wins.
# ---------------------------------------------------------------------------
TYPE_MAP: dict[str, str] = {
    # AmpToFilter
    "FILTER_MOD":           "AmpToFilter",
    "AMP_MOD":              "AmpToFilter",
    "AMPLITUDE_MOD":        "AmpToFilter",
    "AMPLITUDE_MODULATION": "AmpToFilter",
    "AMP_TO_FILTER":        "AmpToFilter",
    "filter_modulation":    "AmpToFilter",
    "VELOCITY_COUPLE":      "AmpToFilter",
    "amplitude_sidechain":  "AmpToFilter",
    "SIDECHAIN_DUCK":       "AmpToFilter",
    "AMPLITUDE_SYNC":       "AmpToFilter",
    "AMPLITUDE_LOCK":       "AmpToFilter",
    "DYNAMICS_SHARE":       "AmpToFilter",
    "DYNAMIC_CROSS":        "AmpToFilter",
    "AmplitudeModulation":  "AmpToFilter",
    "AmpToAmp":             "AmpToFilter",   # no AmpToAmp canonical, closest
    "AMPLITUDE_MOD":        "AmpToFilter",
    "Amp->Amp":             "AmpToFilter",
    "Amp->Drive":           "AmpToFilter",
    "Amp->Space":           "AmpToFilter",
    "Amp->Pitch":           "AmpToPitch",    # valid arrow alias — remapped to CamelCase

    # AmpToPitch
    "PITCH_MOD":            "AmpToPitch",
    "PITCH_SYNC":           "PitchToPitch",
    "PitchSync":            "PitchToPitch",
    "pitch_follow":         "PitchToPitch",
    "PITCH_TRACKING":       "PitchToPitch",
    "PITCH_FOLLOW":         "PitchToPitch",
    "PITCH_TRACK":          "PitchToPitch",
    "PITCH_ENTANGLE":       "PitchToPitch",
    "PITCH_WARP":           "PitchToPitch",
    "PitchMod":             "PitchToPitch",
    "PhaseSync":            "PitchToPitch",
    "PhaseModulation":      "PitchToPitch",
    "PHASE_LOCK":           "PitchToPitch",
    "PHASE_COUPLE":         "PitchToPitch",
    "PHASE_MOD":            "PitchToPitch",
    "PHASE_SYNC":           "PitchToPitch",
    "Pitch->Pitch":         "PitchToPitch",  # valid arrow alias — CamelCase
    "Pitch->Character":     "PitchToPitch",
    "Pitch->Mod":           "PitchToPitch",
    "Pitch->Filter":        "AmpToFilter",
    "Pitch->Velocity":      "AmpToFilter",
    "Pitch->Rate":          "LFOToPitch",
    "PitchToFilter":        "AmpToFilter",
    "PitchToPosition":      "PitchToPitch",
    "Filter->Pitch":        "AmpToPitch",

    # LFOToPitch
    "LFO_SYNC":             "LFOToPitch",
    "SYNC_LFO":             "LFOToPitch",
    "LFOSync":              "LFOToPitch",
    "LFO->Pitch":           "LFOToPitch",   # valid arrow alias — CamelCase
    "LFO->Filter":          "AmpToFilter",
    "LFO->Amp":             "AmpToFilter",
    "LFO->Pan":             "LFOToPitch",
    "LFO->LFO":             "LFOToPitch",
    "LFO->ExcitePos":       "LFOToPitch",
    "LFOToFilter":          "AmpToFilter",
    "LFOToDelay":           "LFOToPitch",

    # EnvToMorph
    "TIMBRE_BLEND":         "EnvToMorph",
    "SPECTRAL_MORPH":       "EnvToMorph",
    "HARMONIC_BLEND":       "EnvToMorph",
    "TIMBRAL_WARP":         "EnvToMorph",
    "TIMBRAL_BLEED":        "EnvToMorph",
    "WAVETABLE_MORPH":      "EnvToMorph",
    "SPECTRAL_BRAID":       "EnvToMorph",
    "SPECTRAL_TRANSFER":    "EnvToMorph",
    "SPECTRAL_REFRACTION":  "EnvToMorph",
    "SPECTRAL_FOLD":        "EnvToMorph",
    "SPECTRAL_BLEND":       "EnvToMorph",
    "SPECTRAL_WEAVE":       "EnvToMorph",
    "SPECTRAL_SHIFT":       "EnvToMorph",
    "SPECTRAL_CROSS":       "EnvToMorph",
    "TEXTURE_MORPH":        "EnvToMorph",
    "TEXTURE_WEAVE":        "EnvToMorph",
    "HARMONIC_FOLD":        "EnvToMorph",
    "HARMONIC_LOCK":        "EnvToMorph",
    "harmonic_lock":        "FilterToFilter",
    "HARMONIC_MESH":        "EnvToMorph",
    "HARMONIC_STACK":       "EnvToMorph",
    "HARMONIC_SYNC":        "EnvToMorph",
    "SpectralMorph":        "EnvToMorph",
    "spectral_cross":       "EnvToMorph",
    "timbral_blend":        "EnvToMorph",
    "MODULATION_FLOW":      "EnvToMorph",
    "MODULATION_SHARE":     "EnvToMorph",
    "TimbreShare":          "EnvToMorph",
    "Spectral->Harmonic":   "EnvToMorph",
    "SpectralToFilter":     "AmpToFilter",
    "SpectralFeed":         "EnvToMorph",
    "FORMANT_BRIDGE":       "EnvToMorph",
    "FORMANT_BLEND":        "EnvToMorph",
    "DensityToMorph":       "EnvToMorph",
    "Spectral":             "EnvToMorph",
    "Harmonic":             "EnvToMorph",
    "Additive":             "EnvToMorph",
    "Multiplicative":       "EnvToMorph",

    # EnvToDecay
    "ENVELOPE_LINK":        "EnvToDecay",
    "ENVELOPE_MIRROR":      "EnvToDecay",
    "ENVELOPE_SHARING":     "EnvToDecay",
    "ENVELOPE_SHARE":       "EnvToDecay",
    "ENVELOPE_SLAVE":       "EnvToDecay",
    "ENVELOPE_FOLLOW":      "EnvToDecay",
    "envelope_share":       "EnvToDecay",
    "EnvToMod":             "EnvToDecay",
    "GrainToMod":           "EnvToDecay",
    "ModToMod":             "EnvToDecay",
    "Envelope->Env":        "EnvToDecay",
    "Envelope->Filter":     "EnvToDecay",
    "Env->Morph":           "EnvToMorph",   # valid arrow alias — CamelCase
    "Env->Morph":           "EnvToMorph",
    "Env->Decay":           "EnvToDecay",   # valid arrow alias — CamelCase
    "Env->Filter":          "AmpToFilter",
    "Env->Cutoff":          "AmpToFilter",
    "Env->Amp":             "AmpToFilter",
    "Env->Level":           "EnvToDecay",
    "Env->LFO Rate":        "LFOToPitch",
    "Env->Pitch":           "EnvToDecay",
    "Env->Character":       "EnvToMorph",
    "Gate->Env":            "EnvToDecay",

    # AudioToFM
    "CHAOS_INJECT":         "AudioToFM",
    "CHAOS_INJECTION":      "AudioToFM",
    "CHAOS_SYNC":           "AudioToFM",
    "CHAOS_LEASH":          "AudioToFM",
    "FREQUENCY_SHIFT":      "AudioToFM",
    "FREQUENCY_MODULATION": "AudioToFM",
    "FREQUENCY_SMEAR":      "AudioToFM",
    "FM_CROSS":             "AudioToFM",
    "FrequencyModulation":  "AudioToFM",
    "FreqToColor":          "AudioToFM",
    "ChaosModulation":      "AudioToFM",
    "ChaosToTrigger":       "AudioToFM",
    "Chaos->Timbre":        "AudioToFM",
    "PHASE_LOCK":           "AudioToFM",
    "DAGUERREOTYPE_BLUR":   "AudioToFM",
    "PREDATOR_STALK":       "AudioToFM",
    "Audio->FM":            "AudioToFM",    # valid arrow alias — CamelCase

    # AudioToRing
    "RING_MOD":             "AudioToRing",
    "FEEDBACK":             "AudioToRing",
    "FEEDBACK_RING":        "AudioToRing",
    "FeedbackCoupling":     "AudioToRing",
    "ORBITAL_RING":         "AudioToRing",
    "Audio->Ring":          "AudioToRing",  # valid arrow alias — CamelCase
    "Audio->AM":            "AudioToRing",
    "Feedback":             "AudioToRing",

    # FilterToFilter
    "RESONANCE_SHARE":      "FilterToFilter",
    "FILTER_COUPLING":      "FilterToFilter",
    "FILTER_LINK":          "FilterToFilter",
    "FILTER_SHARE":         "FilterToFilter",
    "ResonanceChain":       "FilterToFilter",
    "RESONANT_PIPE":        "FilterToFilter",
    "RESONANT_BRIDGE":      "FilterToFilter",
    "RESONANCE_CHAIN":      "FilterToFilter",
    "CRYSTAL_RESONANCE":    "FilterToFilter",
    "SYMPATHETIC_RESONANCE":"FilterToFilter",
    "BREATH_COUPLING":      "FilterToFilter",
    "ReverbShare":          "FilterToFilter",
    "FilterMod":            "FilterToFilter",
    "FilterEnvelope":       "FilterToFilter",
    "Filter->Filter":       "FilterToFilter",  # valid arrow alias — CamelCase
    "Filter->Drive":        "FilterToFilter",
    "Mod->Cutoff":          "FilterToFilter",
    "CV->Filter":           "FilterToFilter",
    "CV->Depth":            "FilterToFilter",
    "CV->Pitch":            "AmpToPitch",
    "SPATIAL_BIND":         "FilterToFilter",
    "ANGULAR_DEFLECT":      "FilterToFilter",
    "SpectralCoupling":     "FilterToFilter",

    # AmpToChoke
    "SIDECHAIN_DUCK":       "AmpToChoke",
    "amplitude_sidechain":  "AmpToChoke",
    "BITE_TRIGGER":         "AmpToChoke",
    "RhythmGate":           "AmpToChoke",
    "RHYTHM_GATE":          "AmpToChoke",
    "Gate->Env":            "AmpToChoke",
    "Amp->Choke":           "AmpToChoke",   # valid arrow alias — CamelCase
    "VelocityToTimbre":     "AmpToChoke",
    "VELOCITY_COUPLE":      "AmpToChoke",
    "MetabolicCoupling":    "AmpToChoke",
    "METABOLIC_SYNC":       "AmpToChoke",

    # RhythmToBlend
    "RHYTHM_LOCK":          "RhythmToBlend",
    "RHYTHMIC_SYNC":        "RhythmToBlend",
    "RHYTHMIC_LOCK":        "RhythmToBlend",
    "RHYTHM_SYNC":          "RhythmToBlend",
    "rhythm_sync":          "RhythmToBlend",
    "TIDAL_MODULATION":     "RhythmToBlend",
    "ECHOLOCATION_SYNC":    "RhythmToBlend",
    "PHOSPHOR_PULSE":       "RhythmToBlend",
    "RhythmicTrigger":      "RhythmToBlend",
    "Rhythm->Blend":        "RhythmToBlend",  # valid arrow alias — CamelCase
    "Rhythm->Gate":         "RhythmToBlend",
    "Rhythmic":             "RhythmToBlend",

    # AudioToWavetable
    "WAVETABLE_MORPH":      "AudioToWavetable",
    "WavetableSync":        "AudioToWavetable",
    "Audio->Wavetable":     "AudioToWavetable",  # valid arrow alias — CamelCase
    "SPATIAL_COUPLE":       "AudioToWavetable",
    "SPATIAL_BLEND":        "AudioToWavetable",
    "SPACE_BLEND":          "AudioToWavetable",
    "ERA_CROSSFADE":        "AudioToWavetable",
    "DriftLock":            "AudioToWavetable",
    "DRIFT_SYNC":           "AudioToWavetable",
    "Stoch->GroupMix":      "AudioToWavetable",
    "Sympathy->DriftDepth": "AudioToWavetable",

    # AudioToBuffer
    "GRANULAR_EXCHANGE":    "AudioToBuffer",
    "GRAIN_SCATTER":        "AudioToBuffer",
    "GRAIN_SYNC":           "AudioToBuffer",
    "GRANULAR_SYNC":        "AudioToBuffer",
    "GRANULAR_WEAVE":       "AudioToBuffer",
    "SUBHARMONIC_WEAVE":    "AudioToBuffer",
    "NEURAL_WEAVE":         "AudioToBuffer",
    "VelToGrain":           "AudioToBuffer",
    "DensityToScatter":     "AudioToBuffer",
    "FilterToGrain":        "AudioToBuffer",
    "AudioToDensity":       "AudioToBuffer",
    "AudioToShape":         "AudioToBuffer",
    "Audio->Buffer":        "AudioToBuffer",  # valid arrow alias — CamelCase
    "Granular":             "AudioToBuffer",

    # KnotTopology
    "HARMONIC_MESH":        "KnotTopology",
    "GRADIENT_DISSOLVE":    "KnotTopology",
    "TAPE_ECHO_LOCK":       "KnotTopology",
    "Knot->Topology":       "KnotTopology",  # valid arrow alias — CamelCase
    "PredictiveCoupling":   "KnotTopology",
    "EnzymaticCoupling":    "KnotTopology",

    # TriangularCoupling
    "MORPH_CHAIN":          "TriangularCoupling",
    "FOLD_MODULATE":        "TriangularCoupling",
    "WAVEFOLD":             "TriangularCoupling",
    "Triangular->Coupling": "TriangularCoupling",  # valid arrow alias — CamelCase
    "Organic":              "TriangularCoupling",

    # Misc low-count unknowns — best-effort mapping
    "SATURATION_DRIVE":     "AudioToFM",
    "SATURATION_FEED":      "AudioToFM",
    "COASTAL_DIVE":         "AudioToWavetable",
    "TIMBRAL_BLEED":        "EnvToMorph",
    "PITCH_ENTANGLE":       "PitchToPitch",
    "SendFeed":             "AudioToBuffer",
    "ModToMod":             "EnvToDecay",
    "GrainToMod":           "EnvToDecay",
    "Audio->Filter":        "AmpToFilter",
    "Macro->Param":         "LFOToPitch",
    "MODULATION_FLOW":      "EnvToMorph",
}

# ---------------------------------------------------------------------------
# couplingIntensity remapping (invalid → canonical)
# Valid values: None | Subtle | Moderate | Deep
# ---------------------------------------------------------------------------
CI_MAP: dict[str, str] = {
    # Near-canonical synonyms
    "Medium":       "Moderate",
    "Extreme":      "Deep",
    "Strong":       "Deep",
    "High":         "Deep",
    "Low":          "Subtle",
    "Light":        "Subtle",
    "Whisper":      "Subtle",
    "Minimal":      "Subtle",
    "Tight":        "Moderate",
    "Possession":   "Deep",
    "Collision":    "Deep",
    # Thematic strings that describe coupling character, not intensity
    "Timbral":      "Moderate",
    "Sync":         "Moderate",
    "Spectral":     "Moderate",
    "Spatial":      "Moderate",
    "Rhythmic":     "Moderate",
    "Harmonic":     "Moderate",
    "Dynamic":      "Moderate",
    "Dialogue":     "Moderate",
    "Temporal":     "Moderate",
    "Resonant":     "Moderate",
    "Modulation":   "Moderate",
    "Chaotic":      "Deep",
}

VALID_CI = {"None", "Subtle", "Moderate", "Deep"}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_preset(path: str) -> tuple[dict, str]:
    """Load a .xometa file, strip UTF-8 BOM if present. Returns (data, raw_text)."""
    with open(path, encoding="utf-8-sig") as f:
        raw = f.read()
    data = json.loads(raw)
    return data, raw


def save_preset(path: str, data: dict) -> None:
    """Write preset with indent=2, trailing newline, no trailing whitespace."""
    lines = json.dumps(data, indent=2, ensure_ascii=False).splitlines()
    cleaned = "\n".join(line.rstrip() for line in lines) + "\n"
    with open(path, "w", encoding="utf-8") as f:
        f.write(cleaned)


def get_pairs(coupling: Any) -> list[dict]:
    """Extract pair list regardless of structure (bare list or dict)."""
    if isinstance(coupling, list):
        return coupling
    if isinstance(coupling, dict):
        return coupling.get("pairs", [])
    return []


def has_valid_pairs(coupling: Any) -> bool:
    return bool(get_pairs(coupling))


# ---------------------------------------------------------------------------
# Pass 1: Structural fixes
# ---------------------------------------------------------------------------

def pass1_fix_structure(data: dict) -> tuple[dict, list[str]]:
    """
    Apply structural fixes to a single preset's data dict.
    Returns (modified_data, list_of_change_descriptions).
    Does NOT modify data in-place; returns a shallow-to-deep copy as needed.
    """
    changes: list[str] = []
    import copy
    d = copy.deepcopy(data)

    # --- Fix couplingIntensity ---
    ci = d.get("couplingIntensity")
    coupling_raw = d.get("coupling")
    pairs = get_pairs(coupling_raw)

    if "couplingIntensity" not in d or ci is None:
        if pairs:
            d["couplingIntensity"] = "Moderate"
            changes.append(f"couplingIntensity: missing/null → 'Moderate'")
    elif ci not in VALID_CI:
        if ci in CI_MAP:
            new_ci = CI_MAP[ci]
            d["couplingIntensity"] = new_ci
            changes.append(f"couplingIntensity: '{ci}' → '{new_ci}'")
        else:
            # Unknown value — default to Moderate when pairs exist, else None
            new_ci = "Moderate" if pairs else "None"
            d["couplingIntensity"] = new_ci
            changes.append(f"couplingIntensity: unknown '{ci}' → '{new_ci}'")

    # --- Fix coupling structure ---
    if coupling_raw is None:
        pass  # Nothing to fix
    elif isinstance(coupling_raw, list):
        # Bare list → wrap in {"pairs": [...]}
        d["coupling"] = {"pairs": coupling_raw}
        changes.append(f"coupling: bare list → {{\"pairs\": [...]}} ({len(coupling_raw)} pairs)")
        coupling_raw = d["coupling"]

    # --- Fix pair keys ---
    coupling_obj = d.get("coupling")
    if isinstance(coupling_obj, dict):
        new_pairs = []
        for i, pair in enumerate(coupling_obj.get("pairs", [])):
            new_pair = dict(pair)
            modified = False

            # source → engineA
            if "source" in new_pair and "engineA" not in new_pair:
                new_pair["engineA"] = new_pair.pop("source")
                modified = True

            # target → engineB
            if "target" in new_pair and "engineB" not in new_pair:
                new_pair["engineB"] = new_pair.pop("target")
                modified = True

            # destination → engineB (fallback)
            if "destination" in new_pair and "engineB" not in new_pair:
                new_pair["engineB"] = new_pair.pop("destination")
                modified = True

            if modified:
                changes.append(f"coupling.pairs[{i}]: renamed source/target/destination keys")

            new_pairs.append(new_pair)

        if new_pairs != list(coupling_obj.get("pairs", [])):
            d["coupling"] = dict(coupling_obj)
            d["coupling"]["pairs"] = new_pairs

    return d, changes


# ---------------------------------------------------------------------------
# Pass 2: Coupling type remapping
# ---------------------------------------------------------------------------

def pass2_fix_types(data: dict) -> tuple[dict, list[str]]:
    """
    Remap invalid coupling type strings to canonical values.
    Returns (modified_data, list_of_change_descriptions).
    """
    changes: list[str] = []
    import copy
    d = copy.deepcopy(data)

    coupling_obj = d.get("coupling")
    if not isinstance(coupling_obj, dict):
        return d, changes

    pairs = coupling_obj.get("pairs", [])
    new_pairs = []

    for i, pair in enumerate(pairs):
        new_pair = dict(pair)
        t = new_pair.get("type", "")

        if t in CANONICAL_TYPES:
            pass  # Already valid
        elif t in TYPE_MAP:
            new_t = TYPE_MAP[t]
            new_pair["type"] = new_t
            changes.append(f"coupling.pairs[{i}].type: '{t}' → '{new_t}'")
        else:
            # Unknown unmapped type — quarantine to AmpToFilter as safe fallback
            if t:  # non-empty
                new_pair["type"] = "AmpToFilter"
                changes.append(f"coupling.pairs[{i}].type: UNKNOWN '{t}' → 'AmpToFilter' (fallback)")

        new_pairs.append(new_pair)

    d["coupling"] = dict(coupling_obj)
    d["coupling"]["pairs"] = new_pairs
    return d, changes


# ---------------------------------------------------------------------------
# Main migration runner
# ---------------------------------------------------------------------------

def run_migration(
    preset_dir: str,
    dry_run: bool,
    run_pass1: bool,
    run_pass2: bool,
) -> None:
    pattern = os.path.join(preset_dir, "**", "*.xometa")
    files = sorted(glob.glob(pattern, recursive=True))

    if not files:
        print(f"ERROR: No .xometa files found under {preset_dir}", file=sys.stderr)
        sys.exit(1)

    total = len(files)
    changed = 0
    errors = 0
    unchanged = 0

    # Detailed counters
    p1_counter: Counter = Counter()
    p2_counter: Counter = Counter()
    unmapped_types: Counter = Counter()

    mode_label = "DRY RUN" if dry_run else "EXECUTE"
    passes = []
    if run_pass1:
        passes.append("1 (structural)")
    if run_pass2:
        passes.append("2 (type remap)")
    print(f"\n[{mode_label}] Passes: {', '.join(passes)}")
    print(f"Scanning {total} files under {preset_dir}\n")

    for path in files:
        try:
            data, raw = load_preset(path)
        except Exception as e:
            print(f"  ERROR loading {path}: {e}")
            errors += 1
            continue

        current = data
        all_changes: list[str] = []

        if run_pass1:
            current, c1 = pass1_fix_structure(current)
            all_changes.extend(c1)
            for c in c1:
                p1_counter[c.split(":")[0].strip()] += 1

        if run_pass2:
            current, c2 = pass2_fix_types(current)
            all_changes.extend(c2)
            for c in c2:
                if "UNKNOWN" in c:
                    # Extract the original type for reporting
                    try:
                        orig = c.split("'")[1]
                        unmapped_types[orig] += 1
                    except IndexError:
                        pass
                p2_counter["type remapped"] += 1

        if not all_changes:
            unchanged += 1
            continue

        changed += 1

        rel = os.path.relpath(path, preset_dir)
        print(f"  {'[WOULD CHANGE]' if dry_run else '[CHANGED]'} {rel}")
        for c in all_changes[:8]:  # cap per-file output to 8 lines
            print(f"    • {c}")
        if len(all_changes) > 8:
            print(f"    … and {len(all_changes) - 8} more change(s)")

        if not dry_run:
            # Backup original
            bak_path = path + ".bak"
            shutil.copy2(path, bak_path)
            save_preset(path, current)

    # ---------------------------------------------------------------------------
    # Summary
    # ---------------------------------------------------------------------------
    print("\n" + "=" * 60)
    print(f"SUMMARY ({mode_label})")
    print("=" * 60)
    print(f"  Files scanned : {total}")
    print(f"  Files changed : {changed}")
    print(f"  Files unchanged: {unchanged}")
    print(f"  Errors        : {errors}")

    if run_pass1 and p1_counter:
        print(f"\nPass 1 — structural fixes breakdown:")
        for k, v in p1_counter.most_common():
            print(f"  {v:5d}  {k}")

    if run_pass2 and p2_counter:
        print(f"\nPass 2 — type remaps: {p2_counter['type remapped']} total")

    if unmapped_types:
        print(f"\nWARNING: {len(unmapped_types)} distinct unknown types fell back to 'AmpToFilter':")
        for t, cnt in unmapped_types.most_common(20):
            print(f"  {cnt:4d}  '{t}'")

    if dry_run:
        print("\nDry run complete. No files were modified.")
        print("Re-run with --execute to apply changes (backups written as .bak files).")
    else:
        print("\nMigration complete. Backups written as <filename>.xometa.bak")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="XOceanus preset migration: structural fixes + coupling type remapping."
    )
    parser.add_argument(
        "--preset-dir",
        default="Presets/XOceanus",
        help="Root directory of .xometa presets (default: Presets/XOceanus)",
    )

    mode_group = parser.add_mutually_exclusive_group(required=True)
    mode_group.add_argument(
        "--dry-run",
        action="store_true",
        help="Report changes without writing files (default safe mode)",
    )
    mode_group.add_argument(
        "--execute",
        action="store_true",
        help="Apply changes and write .bak backups",
    )

    pass_group = parser.add_mutually_exclusive_group(required=True)
    pass_group.add_argument(
        "--pass",
        dest="pass_num",
        choices=["1", "2"],
        help="Run only pass 1 (structural) or pass 2 (type remap)",
    )
    pass_group.add_argument(
        "--all",
        action="store_true",
        help="Run both passes (pass 1 then pass 2)",
    )

    args = parser.parse_args()

    # Resolve preset_dir relative to CWD or absolute
    preset_dir = args.preset_dir
    if not os.path.isabs(preset_dir):
        preset_dir = os.path.join(os.getcwd(), preset_dir)

    if not os.path.isdir(preset_dir):
        print(f"ERROR: preset-dir not found: {preset_dir}", file=sys.stderr)
        sys.exit(1)

    run_pass1 = args.all or args.pass_num == "1"
    run_pass2 = args.all or args.pass_num == "2"

    run_migration(
        preset_dir=preset_dir,
        dry_run=args.dry_run,
        run_pass1=run_pass1,
        run_pass2=run_pass2,
    )


if __name__ == "__main__":
    main()
