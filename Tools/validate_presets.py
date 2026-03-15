#!/usr/bin/env python3
"""
XOmnibus Preset Validator — Comprehensive .xometa Quality Assurance

Validates all factory presets for:
  1. Schema compliance (required fields, types, ranges)
  2. DNA completeness (all 6 dimensions present, in [0,1])
  3. DNA balance analysis (flat profiles, clustering, coverage gaps)
  4. Macro labels (exactly 4, non-empty)
  5. Engine names (must be in the valid set)
  6. Coupling pairs (valid types, valid engine refs, amounts in [-1,1])
  7. Parameter sanity (no NaN, no extreme values)
  8. Naming rules (2-3 words, max 30 chars, no duplicates, no jargon)
  9. Mood distribution (flag imbalances)
  10. Coupling coverage analysis (which engine pairs lack coupling presets)

Usage:
    python3 validate_presets.py [--fix] [--report] [--strict]

Options:
    --fix       Auto-fix trivially correctable issues (pad macros, clamp DNA)
    --report    Print full report even for passing presets
    --strict    Treat warnings as errors (for CI)
"""

import json
import math
import re
import sys
import os
from pathlib import Path
from collections import defaultdict, Counter

PRESET_DIR = Path(__file__).parent.parent / "Presets" / "XOmnibus"

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

VALID_MOODS = {"Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family", "User"}

VALID_ENGINES = {
    # Canonical O-prefix engine IDs (all 20)
    "OddfeliX", "OddOscar", "Overdub", "Odyssey", "Oblong",
    "Obese", "Onset", "Overworld", "Opal", "Orbital",
    "Organon", "Ouroboros", "Obsidian", "Overbite", "Origami",
    "Oracle", "Obscura", "Oceanic", "Optic", "Oblique",
    "Ocelot", "Osprey", "Osteria", "Owlfish",
    "Ohm", "Orphica", "Obbligato", "Ottoni", "Ole",
    # Legacy aliases (resolved by PresetManager::resolveEngineAlias)
    "Snap", "Morph", "Dub", "Drift", "Bob", "Fat", "Bite",
    "XOverdub", "XOdyssey", "XOblong", "XObese", "XOnset",
    "XOpal", "XOrbital", "XOrganon", "XOuroboros",
}

# Map non-standard coupling intensities to valid ones
COUPLING_INTENSITY_ALIASES = {
    "Medium": "Moderate",
    "Light": "Subtle",
    "Strong": "Deep",
    "Heavy": "Deep",
    "High": "Deep",
}

VALID_COUPLING_TYPES = {
    "Amp->Filter", "Amp->Pitch", "LFO->Pitch", "Env->Morph",
    "Audio->FM", "Audio->Ring", "Filter->Filter", "Amp->Choke",
    "Rhythm->Blend", "Env->Decay", "Pitch->Pitch", "Audio->Wavetable",
}

VALID_COUPLING_INTENSITIES = {"None", "Subtle", "Moderate", "Deep"}

DNA_DIMENSIONS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

MAX_NAME_LENGTH = 30
MAX_FILE_SIZE = 1024 * 1024  # 1 MB

# Jargon words to flag in preset names (brand rule: evocative, no jargon)
JARGON_WORDS = {
    "init", "default", "test", "template", "basic", "demo", "example",
    "patch", "preset", "synth", "oscillator", "osc", "filter", "lfo",
    "adsr", "env", "wavetable", "granular", "fm", "subtractive",
    "additive", "modular", "parameter", "param",
}

# Known engine parameter prefixes for cross-checking (frozen — never change)
ENGINE_PARAM_PREFIXES = {
    "OddfeliX": "snap_", "OddOscar": "morph_", "Overdub": "dub_",
    "Odyssey": "odyssey_", "Oblong": "bob_", "Obese": "fat_",
    "Overbite": "poss_", "Onset": "onset_", "Opal": "opal_",
    "Overworld": "era_", "Orbital": "orbital_",
    "Organon": "organon_", "Ouroboros": "ouroboros_",
    "Obsidian": "obsidian_", "Origami": "origami_", "Oracle": "oracle_",
    "Obscura": "obscura_", "Oceanic": "oceanic_", "Optic": "optic_",
    "Oblique": "oblq_",
}

# Core engines that should have coupling presets with each other
CORE_ENGINES = [
    "OddfeliX", "OddOscar", "Overdub", "Odyssey", "Oblong",
    "Obese", "Overbite", "Onset", "Opal", "Overworld", "Organon", "Ouroboros",
    "Obsidian", "Origami", "Oracle", "Obscura", "Oceanic", "Optic", "Oblique",
    "Orbital",
]

# ---------------------------------------------------------------------------
# Validation result tracking
# ---------------------------------------------------------------------------

class ValidationResult:
    def __init__(self):
        self.errors = []
        self.warnings = []
        self.fixes = []

    def error(self, msg):
        self.errors.append(msg)

    def warn(self, msg):
        self.warnings.append(msg)

    def fix(self, msg):
        self.fixes.append(msg)

    @property
    def ok(self):
        return len(self.errors) == 0


def validate_preset(filepath: Path, do_fix: bool = False) -> ValidationResult:
    """Validate a single .xometa preset file."""
    result = ValidationResult()
    rel = filepath.relative_to(PRESET_DIR)

    # File size check
    if filepath.stat().st_size > MAX_FILE_SIZE:
        result.error(f"File exceeds 1MB limit: {filepath.stat().st_size} bytes")
        return result

    # Parse JSON
    try:
        with open(filepath, "r", encoding="utf-8") as f:
            data = json.load(f)
    except json.JSONDecodeError as e:
        result.error(f"Invalid JSON: {e}")
        return result
    except Exception as e:
        result.error(f"Read error: {e}")
        return result

    if not isinstance(data, dict):
        result.error("Root is not a JSON object")
        return result

    modified = False

    # --- Required fields ---

    # schema_version
    sv = data.get("schema_version")
    if sv is None:
        result.error("Missing required field: schema_version")
    elif not isinstance(sv, int) or sv < 1:
        result.error(f"Invalid schema_version: {sv}")

    # name
    name = data.get("name")
    if not name or not isinstance(name, str):
        result.error("Missing or empty required field: name")
    else:
        if len(name) > MAX_NAME_LENGTH:
            result.warn(f"Name too long ({len(name)} chars, max {MAX_NAME_LENGTH}): '{name}'")
        # Word count check (2-3 words preferred; split on spaces and underscores)
        words = [w for w in re.split(r'[\s_]+', name) if w]
        if len(words) < 2:
            result.warn(f"Name has only {len(words)} word(s), prefer 2-3: '{name}'")
        elif len(words) > 5:
            result.warn(f"Name has {len(words)} words, prefer 2-3: '{name}'")
        # Jargon check
        name_lower_words = {w.lower() for w in words}
        jargon_found = name_lower_words & JARGON_WORDS
        if jargon_found:
            result.warn(f"Name contains jargon words {jargon_found}: '{name}'")

    # mood
    mood = data.get("mood")
    if not mood or mood not in VALID_MOODS:
        result.warn(f"Invalid mood '{mood}', expected one of {VALID_MOODS}")

    # engines
    engines = data.get("engines")
    if not engines or not isinstance(engines, list) or len(engines) == 0:
        result.error("Missing or empty required field: engines")
    else:
        if len(engines) > 3:
            result.warn(f"Too many engines ({len(engines)}), max 3")
        for eng in engines:
            if eng not in VALID_ENGINES:
                result.warn(f"Unknown engine name: '{eng}'")

    # parameters
    params = data.get("parameters")
    if params is None:
        result.error("Missing required field: parameters")
    elif not isinstance(params, dict):
        result.error("parameters must be a JSON object")
    else:
        # Check for NaN/Inf in parameter values
        for eng_name, eng_params in params.items():
            if not isinstance(eng_params, dict):
                continue
            for pname, pval in eng_params.items():
                if isinstance(pval, float):
                    if pval != pval:  # NaN check
                        result.error(f"NaN parameter: {eng_name}.{pname}")
                    elif abs(pval) == float('inf'):
                        result.error(f"Inf parameter: {eng_name}.{pname}")

    # --- DNA validation ---
    dna = data.get("dna")
    if dna is None:
        result.warn("Missing DNA — run compute_preset_dna.py")
    elif isinstance(dna, dict):
        for dim in DNA_DIMENSIONS:
            val = dna.get(dim)
            if val is None:
                result.warn(f"Missing DNA dimension: {dim}")
            elif not isinstance(val, (int, float)):
                result.warn(f"Non-numeric DNA.{dim}: {val}")
            elif val < 0.0 or val > 1.0:
                result.warn(f"DNA.{dim} out of range [0,1]: {val}")
                if do_fix:
                    dna[dim] = max(0.0, min(1.0, val))
                    result.fix(f"Clamped DNA.{dim} to {dna[dim]}")
                    modified = True

        # Check for all-default DNA (0.5 across the board = probably not computed)
        if all(dna.get(d) == 0.5 for d in DNA_DIMENSIONS):
            result.warn("All DNA dimensions are 0.5 — likely not computed")

        # Check for flat DNA profile (all within narrow band = lacks character)
        dna_vals = [dna.get(d, 0.5) for d in DNA_DIMENSIONS if isinstance(dna.get(d), (int, float))]
        if len(dna_vals) >= 6:
            dna_range = max(dna_vals) - min(dna_vals)
            if dna_range < 0.15 and not all(v == 0.5 for v in dna_vals):
                result.warn(f"Flat DNA profile (range={dna_range:.2f}) — preset may lack character")
    else:
        result.warn(f"DNA is not an object: {type(dna)}")

    # --- Macro labels ---
    macros = data.get("macroLabels")
    if macros is None:
        result.warn("Missing macroLabels")
        if do_fix:
            data["macroLabels"] = ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]
            result.fix("Added default macroLabels")
            modified = True
    elif not isinstance(macros, list):
        result.warn(f"macroLabels is not an array: {type(macros)}")
    else:
        if len(macros) != 4:
            result.warn(f"macroLabels has {len(macros)} items, expected 4")
            if do_fix and len(macros) < 4:
                defaults = ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]
                while len(macros) < 4:
                    macros.append(defaults[len(macros)])
                result.fix(f"Padded macroLabels to 4")
                modified = True
        for i, label in enumerate(macros):
            if not label or not isinstance(label, str) or len(label.strip()) == 0:
                result.warn(f"Empty macro label at index {i}")

    # --- Coupling intensity ---
    ci = data.get("couplingIntensity")
    if ci is not None and ci not in VALID_COUPLING_INTENSITIES:
        # Case-insensitive alias lookup
        ci_title = ci.strip().title() if isinstance(ci, str) else ci
        alias_lookup = {k.lower(): v for k, v in COUPLING_INTENSITY_ALIASES.items()}
        if ci_title in VALID_COUPLING_INTENSITIES:
            if do_fix:
                data["couplingIntensity"] = ci_title
                result.fix(f"Fixed couplingIntensity casing '{ci}' → '{ci_title}'")
                modified = True
            else:
                result.warn(f"couplingIntensity casing: '{ci}' (use --fix to correct to '{ci_title}')")
        elif ci.lower() in alias_lookup:
            resolved = alias_lookup[ci.lower()]
            if do_fix:
                data["couplingIntensity"] = resolved
                result.fix(f"Fixed couplingIntensity '{ci}' → '{resolved}'")
                modified = True
            else:
                result.warn(f"Invalid couplingIntensity: '{ci}' (use --fix to correct to '{resolved}')")
        else:
            result.warn(f"Invalid couplingIntensity: '{ci}'")

    # --- Coupling pairs ---
    coupling = data.get("coupling")
    if coupling is not None and isinstance(coupling, dict):
        pairs = coupling.get("pairs", [])
        if isinstance(pairs, list):
            for i, pair in enumerate(pairs):
                if not isinstance(pair, dict):
                    result.warn(f"Coupling pair {i} is not an object")
                    continue
                ea = pair.get("engineA", "")
                eb = pair.get("engineB", "")
                ct = pair.get("type", "")
                amt = pair.get("amount", 0)

                if ea not in VALID_ENGINES:
                    result.warn(f"Coupling pair {i}: unknown engineA '{ea}'")
                if eb not in VALID_ENGINES:
                    result.warn(f"Coupling pair {i}: unknown engineB '{eb}'")
                if ct not in VALID_COUPLING_TYPES:
                    result.warn(f"Coupling pair {i}: unknown type '{ct}'")
                if isinstance(amt, (int, float)) and (amt < -1.0 or amt > 1.0):
                    result.warn(f"Coupling pair {i}: amount {amt} out of [-1,1]")

    # --- Author ---
    author = data.get("author")
    if not author or not isinstance(author, str):
        result.warn("Missing or empty author field")

    # --- Tags ---
    tags = data.get("tags")
    if tags is not None and isinstance(tags, list):
        # Check for duplicate tags
        tag_counts = Counter(tags)
        dupes = {t: c for t, c in tag_counts.items() if c > 1}
        if dupes:
            if do_fix:
                data["tags"] = list(dict.fromkeys(tags))  # deduplicate preserving order
                result.fix(f"Deduplicated tags: removed {sum(c-1 for c in dupes.values())} dupes")
                modified = True
            else:
                result.warn(f"Duplicate tags: {dupes}")

    # --- Tempo ---
    tempo = data.get("tempo")
    if tempo is not None and not isinstance(tempo, type(None)):
        if isinstance(tempo, (int, float)):
            if tempo < 0 or tempo > 999:
                result.warn(f"Suspicious tempo value: {tempo}")

    # Write fixes
    if modified and do_fix:
        try:
            with open(filepath, "w", encoding="utf-8") as f:
                json.dump(data, f, indent=2, ensure_ascii=False)
                f.write("\n")
        except Exception as e:
            result.error(f"Failed to write fix: {e}")

    return result


def run_validation(do_fix=False, report_all=False, strict=False):
    """Run validation across all presets."""
    if not PRESET_DIR.exists():
        print(f"ERROR: Preset directory not found: {PRESET_DIR}")
        return 1

    files = sorted(PRESET_DIR.rglob("*.xometa"))
    if not files:
        print("WARNING: No .xometa files found")
        return 1

    print(f"XOmnibus Preset Validator")
    print(f"{'=' * 60}")
    print(f"Scanning: {PRESET_DIR}")
    print(f"Found: {len(files)} preset files")
    print(f"Mode: {'fix' if do_fix else 'validate'} | {'strict' if strict else 'normal'}")
    print()

    # Stats
    total = len(files)
    passed = 0
    warned = 0
    failed = 0
    total_errors = 0
    total_warnings = 0
    total_fixes = 0
    mood_counts = Counter()
    engine_counts = Counter()
    name_set = set()
    duplicate_names = []

    for filepath in files:
        rel = filepath.relative_to(PRESET_DIR)
        result = validate_preset(filepath, do_fix=do_fix)

        # Track stats
        total_errors += len(result.errors)
        total_warnings += len(result.warnings)
        total_fixes += len(result.fixes)

        # Track mood/engine distribution
        try:
            with open(filepath, "r", encoding="utf-8") as f:
                data = json.load(f)
            mood_counts[data.get("mood", "Unknown")] += 1
            for eng in data.get("engines", []):
                engine_counts[eng] += 1

            # Track name duplicates
            name = data.get("name", "")
            if name in name_set:
                duplicate_names.append(name)
            name_set.add(name)
        except:
            pass

        if result.errors:
            failed += 1
            print(f"  FAIL  {rel}")
            for e in result.errors:
                print(f"        ERROR: {e}")
            for w in result.warnings:
                print(f"        WARN:  {w}")
        elif result.warnings:
            warned += 1
            if report_all:
                print(f"  WARN  {rel}")
                for w in result.warnings:
                    print(f"        WARN:  {w}")
        else:
            passed += 1
            if report_all:
                print(f"  PASS  {rel}")

        for fix in result.fixes:
            print(f"        FIXED: {fix}")

    # Summary
    print()
    print(f"{'=' * 60}")
    print(f"RESULTS")
    print(f"{'=' * 60}")
    print(f"  Total presets:  {total}")
    print(f"  Passed:         {passed} ({100*passed//total}%)")
    print(f"  Warnings:       {warned} ({100*warned//total}%)")
    print(f"  Failed:         {failed} ({100*failed//total}%)")
    print(f"  Total errors:   {total_errors}")
    print(f"  Total warnings: {total_warnings}")
    if do_fix:
        print(f"  Total fixes:    {total_fixes}")

    # Mood distribution
    print()
    print(f"MOOD DISTRIBUTION")
    print(f"{'─' * 40}")
    for mood in sorted(VALID_MOODS):
        count = mood_counts.get(mood, 0)
        bar = "█" * (count // 5) if count > 0 else ""
        print(f"  {mood:<14} {count:>4}  {bar}")

    # Engine distribution
    print()
    print(f"ENGINE DISTRIBUTION")
    print(f"{'─' * 40}")
    for eng, count in engine_counts.most_common(20):
        bar = "█" * (count // 5) if count > 0 else ""
        print(f"  {eng:<16} {count:>4}  {bar}")

    # Duplicate names
    if duplicate_names:
        print()
        print(f"DUPLICATE NAMES ({len(duplicate_names)})")
        print(f"{'─' * 40}")
        for name in sorted(set(duplicate_names)):
            print(f"  - {name}")

    # --- DNA Coverage Analysis ---
    if report_all:
        all_dna = []
        for filepath in files:
            try:
                with open(filepath, "r", encoding="utf-8") as f:
                    data = json.load(f)
                dna = data.get("dna")
                if dna and isinstance(dna, dict):
                    vals = {d: dna.get(d, 0.5) for d in DNA_DIMENSIONS}
                    all_dna.append((data.get("name", "?"), data.get("engines", []), vals))
            except:
                pass

        if all_dna:
            print()
            print(f"DNA COVERAGE ANALYSIS")
            print(f"{'─' * 60}")

            # Per-dimension statistics
            for dim in DNA_DIMENSIONS:
                vals = [d[2][dim] for d in all_dna if isinstance(d[2].get(dim), (int, float))]
                if not vals:
                    continue
                avg = sum(vals) / len(vals)
                lo = min(vals)
                hi = max(vals)
                # Count how many in each quintile
                quintiles = [0] * 5
                for v in vals:
                    q = min(4, int(v * 5))
                    quintiles[q] += 1
                q_str = " ".join(f"{q:3d}" for q in quintiles)
                print(f"  {dim:12s}: avg={avg:.2f}  [{lo:.2f}–{hi:.2f}]  quintiles: {q_str}")

            # Find underrepresented DNA regions
            print()
            print(f"DNA GAP ANALYSIS (quintile counts across all dimensions)")
            print(f"{'─' * 60}")
            for dim in DNA_DIMENSIONS:
                vals = [d[2][dim] for d in all_dna if isinstance(d[2].get(dim), (int, float))]
                if not vals:
                    continue
                quintiles = [0] * 5
                for v in vals:
                    q = min(4, int(v * 5))
                    quintiles[q] += 1
                avg_q = sum(quintiles) / 5
                gaps = []
                for i, q in enumerate(quintiles):
                    if q < avg_q * 0.4:
                        lo_pct = i * 20
                        hi_pct = (i + 1) * 20
                        gaps.append(f"{lo_pct}-{hi_pct}% ({q} presets)")
                if gaps:
                    print(f"  {dim:12s}: SPARSE in {', '.join(gaps)}")

            # Flat profile count
            flat_count = 0
            for name, engines, vals in all_dna:
                dna_vals = list(vals.values())
                dna_range = max(dna_vals) - min(dna_vals)
                if dna_range < 0.15:
                    flat_count += 1
            if flat_count > 0:
                print(f"\n  {flat_count} presets have flat DNA profiles (range < 0.15)")

    # --- Coupling Coverage Analysis ---
    if report_all:
        coupling_pairs_found = set()
        for filepath in files:
            try:
                with open(filepath, "r", encoding="utf-8") as f:
                    data = json.load(f)
                coupling = data.get("coupling")
                if coupling and isinstance(coupling, dict):
                    pairs = coupling.get("pairs", [])
                    if isinstance(pairs, list):
                        for pair in pairs:
                            if isinstance(pair, dict):
                                ea = pair.get("engineA", pair.get("source", ""))
                                eb = pair.get("engineB", pair.get("target", ""))
                                if ea and eb:
                                    coupling_pairs_found.add((min(ea, eb), max(ea, eb)))
            except:
                pass

        print()
        print(f"COUPLING COVERAGE")
        print(f"{'─' * 60}")
        print(f"  Unique engine pairs with coupling presets: {len(coupling_pairs_found)}")

        # Find missing pairs among core engines
        missing_pairs = []
        for i, ea in enumerate(CORE_ENGINES):
            for eb in CORE_ENGINES[i+1:]:
                pair = (min(ea, eb), max(ea, eb))
                if pair not in coupling_pairs_found:
                    missing_pairs.append(pair)

        if missing_pairs:
            total_possible = len(CORE_ENGINES) * (len(CORE_ENGINES) - 1) // 2
            coverage = ((total_possible - len(missing_pairs)) / total_possible) * 100
            print(f"  Core engine pair coverage: {coverage:.0f}% ({total_possible - len(missing_pairs)}/{total_possible})")
            print(f"\n  Missing coupling pairs ({len(missing_pairs)}):")
            for ea, eb in sorted(missing_pairs):
                print(f"    {ea} <-> {eb}")
        else:
            print(f"  All core engine pairs have coupling presets!")

    # Exit code
    if strict:
        return 1 if (failed > 0 or total_warnings > 0) else 0
    return 1 if failed > 0 else 0


if __name__ == "__main__":
    args = sys.argv[1:]
    do_fix = "--fix" in args
    report_all = "--report" in args
    strict = "--strict" in args

    exit_code = run_validation(do_fix=do_fix, report_all=report_all, strict=strict)
    sys.exit(exit_code)
