#!/usr/bin/env python3
"""
Oxport — XO_OX Expansion Pack Pipeline Orchestrator
Chains the full XPN export pipeline into a single command.

Pipeline stages:
  1. render_spec       — Generate render specifications from .xometa presets
  2. categorize        — Classify WAV samples into voice categories
  3. expand            — Expand flat kits into velocity/cycle/smart WAV sets
  4. qa                — Perceptual QA check on rendered WAV files
  5. smart_trim        — Auto-trim silence tails and add fade-out on rendered WAVs
  6. export            — Generate .xpm programs (drum or keygroup, per engine)
  7. cover_art         — Generate branded procedural cover art
  8. complement_chain  — Generate primary+complement XPM variant pairs (Artwork collection)
  9. preview           — Generate 15-second preview audio for each program
 10. listen            — [GATE] Generate preview playlist; require human approval before packaging
 11. package           — Package everything into a .xpn archive

Human listen gate (build pipeline):
    # After render completes, build pauses at LISTEN and prints:
    #   Run `oxport approve <output_dir>` to continue to PACKAGE.
    python3 oxport.py approve builds/mpce-perc-001/
    python3 oxport.py build spec.oxbuild --resume

    # Bypass the gate (CI/automated builds):
    python3 oxport.py build spec.oxbuild --skip-listen

Usage:
    # Full pipeline for an engine
    python3 oxport.py run --engine Onset --wavs-dir /path/to/wavs \\
        --output-dir /path/to/out

    # Dry run — show what would happen
    python3 oxport.py run --engine Onset --output-dir /tmp/test --dry-run

    # Skip specific stages
    python3 oxport.py run --engine Onset --wavs-dir /path/to/wavs \\
        --output-dir /path/to/out --skip cover_art,categorize

    # Run for a single preset
    python3 oxport.py run --engine Onset --preset "808 Reborn" \\
        --wavs-dir /path/to/wavs --output-dir /path/to/out

    # Show pipeline status
    python3 oxport.py status --output-dir /path/to/out

    # Validate pipeline output (structural checks)
    python3 oxport.py validate --output-dir /path/to/out

    # Validate .xometa presets (10-point quality check)
    python3 oxport.py validate --presets
    python3 oxport.py validate --presets --fix --strict

    # Both at once
    python3 oxport.py validate --output-dir /path/to/out --presets

    # Batch export multiple engines from a config file
    python3 oxport.py batch --config batch.json
    python3 oxport.py batch --config batch.json --parallel 4 --skip-failed
    python3 oxport.py batch --config batch.json --dry-run

    # Batch with cross-engine loudness normalization (LUFS via ITU-R BS.1770-4)
    python3 oxport.py batch --config batch.json --normalize
    python3 oxport.py batch --config batch.json --normalize --normalize-target -16.0
"""

from __future__ import annotations

import argparse
import json
import math
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time
import traceback
import uuid
from datetime import datetime
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Resolve Tools/ directory so sibling imports work when invoked from anywhere
# ---------------------------------------------------------------------------
TOOLS_DIR = Path(__file__).parent.resolve()
REPO_ROOT = TOOLS_DIR.parent
PRESETS_DIR = REPO_ROOT / "Presets" / "XOceanus"

if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

# ---------------------------------------------------------------------------
# Optional numpy acceleration for fleet-scale audio processing
# ---------------------------------------------------------------------------
try:
    import numpy as np
    _NUMPY_AVAILABLE = True
except ImportError:
    _NUMPY_AVAILABLE = False

# ---------------------------------------------------------------------------
# Lazy imports — each stage imports its tool only when needed.
# This keeps startup fast and lets --dry-run work without heavy deps.
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# Engine name canonicalization
# ---------------------------------------------------------------------------

# ENGINE_ALIASES is built from engine_registry so it covers all 76 engines.
# Upper-case and lower-case variants of every canonical name are pre-populated.
# Additional hand-authored entries handle multi-word casing quirks and the
# "OnsetEngine" legacy alias that appears in older render specs.
from engine_registry import get_all_engines as _get_all_engines

ENGINE_ALIASES: dict[str, str] = {}
for _n in _get_all_engines():
    ENGINE_ALIASES[_n.upper()] = _n
    ENGINE_ALIASES[_n.lower()] = _n
# Extra hand-authored aliases not derivable by simple case-fold
ENGINE_ALIASES.update({
    "OnsetEngine": "Onset",
    "Opensky":     "OpenSky",
    "Oceandeep":   "OceanDeep",
})
del _get_all_engines, _n

# ---------------------------------------------------------------------------
# Tier configurations — SURFACE / DEEP / TRENCH
# QDD locked decision: tiers control velocity layers, round-robin policy,
# and maximum XPM slot budget.  First ONSET pack ships at TRENCH.
# ---------------------------------------------------------------------------

# Lazy-import so oxport.py stays fast when these modules are not needed.
def _build_tier_configs() -> dict:
    from xpn_velocity_standard import RENDER_MIDPOINTS, NUM_LAYERS
    return {
        "SURFACE": {
            "vel_layers":  1,
            "vel_values":  [RENDER_MIDPOINTS[3]],   # Hard only (vel=109)
            "round_robin": False,
            "max_slots":   16,
        },
        "DEEP": {
            "vel_layers":  NUM_LAYERS,               # 4
            "vel_values":  list(RENDER_MIDPOINTS),   # [10, 38, 73, 109]
            "round_robin": False,
            "max_slots":   64,
        },
        "TRENCH": {
            "vel_layers":  NUM_LAYERS,               # 4
            "vel_values":  list(RENDER_MIDPOINTS),   # [10, 38, 73, 109]
            "round_robin": True,                     # per-voice RR from VOICE_RR_COUNTS
            "max_slots":   96,
        },
    }

TIER_CONFIGS: dict = {}   # populated on first use via _get_tier_config()

def _get_tier_config(tier: str) -> dict:
    """Return the config dict for *tier*, building TIER_CONFIGS on first call."""
    global TIER_CONFIGS
    if not TIER_CONFIGS:
        TIER_CONFIGS = _build_tier_configs()
    tier_upper = tier.upper()
    if tier_upper not in TIER_CONFIGS:
        valid = ", ".join(TIER_CONFIGS)
        raise ValueError(f"Unknown tier {tier!r}. Valid values: {valid}")
    return TIER_CONFIGS[tier_upper]


def resolve_engine_name(raw: str) -> str:
    """Normalize engine name to canonical spelling."""
    return ENGINE_ALIASES.get(raw, raw)


# Engines whose presets produce drum programs (everything else is keygroup)
DRUM_ENGINES = {"Onset"}  # Single canonical spelling

BANNER = r"""
   ____                       _
  / __ \_  ___ __  ___  _ __ | |_
 | |  | \ \/ / '_ \/ _ \| '__| __|
 | |__| |>  <| |_) |(_) | |  | |_
  \____/_/\_\ .__/\___/|_|   \__|
    XO_OX   |_| Expansion Pipeline
"""

# ---------------------------------------------------------------------------
# Pipeline stage definitions
# ---------------------------------------------------------------------------

STAGES = [
    "render_spec",
    "categorize",
    "expand",
    "qa",
    "smart_trim",
    "export",
    "cover_art",
    "complement_chain",
    "preview",
    "package",
]

STAGE_DESCRIPTIONS = {
    "render_spec":      "Generate render specifications from .xometa presets",
    "categorize":       "Classify WAV samples into voice categories",
    "expand":           "Expand flat kits into velocity/cycle/smart WAV sets",
    "qa":               "Perceptual QA check on rendered WAV files",
    "smart_trim":       "Auto-trim silence tails and add fade-out on rendered WAVs",
    "export":           "Generate .xpm programs (drum or keygroup)",
    "cover_art":        "Generate branded procedural cover art",
    "complement_chain": "Generate primary+complement variant XPM pairs (Artwork/Color collection)",
    "preview":          "Generate 15-second preview audio for each program",
    "package":          "Package into .xpn archive",
}

# Artwork/Color collection engine keys — complement_chain stage is active only for these.
# This set is the canonical gating check; the full database lives in xpn_complement_renderer.py.
#
# IMPORTANT: NONE of these 24 engines exist in engine_registry.py or as Source/Engines/
# directories yet.  They are PLANNED future engines for the Artwork/Color Collection.
# Invoking the pipeline with any of these names will produce no preset output because
# no .xometa files or WAV renders exist for them.  The complement_chain stage is the
# only stage that legitimately references these names.  All other pipeline stages will
# silently produce empty output for Artwork engine names — issue #823.
ARTWORK_ENGINES = {
    "XOxblood", "XOnyx", "XOchre", "XOrchid",
    "XOttanio", "XOmaomao", "XOstrum", "XOni",
    "XOpulent", "XOccult", "XOvation", "XOverdrive",
    "XOrnament", "XOblation", "XObsession", "XOther",
    "XOrdeal",  "XOutpour", "XOctavo", "XObjet",
    "XOkami",   "XOmni",    "XOdama",  "XOffer",
}


# ---------------------------------------------------------------------------
# Pipeline context — passed between stages, accumulates outputs
# ---------------------------------------------------------------------------

class PipelineContext:
    """Mutable state bag that flows through the pipeline."""

    def __init__(self, engine: str, output_dir: Path,
                 wavs_dir: Optional[Path] = None,
                 preset_filter: Optional[str] = None,
                 kit_mode: str = "smart",
                 version: str = "1.0",
                 pack_name: Optional[str] = None,
                 dry_run: bool = False,
                 strict_qa: bool = False,
                 auto_fix: bool = False,
                 tuning: Optional[str] = None,
                 choke_preset: str = "none",
                 round_robin: bool = True):
        self.engine = resolve_engine_name(engine)
        self.output_dir = output_dir
        self.wavs_dir = wavs_dir
        self.preset_filter = preset_filter
        self.kit_mode = kit_mode
        self.version = version
        self.pack_name = pack_name or f"XO_OX {engine}"
        self.dry_run = dry_run
        self.strict_qa = strict_qa
        self.auto_fix = auto_fix  # Apply safe auto-fixes (DC_OFFSET, ATTACK_PRESENCE, SILENCE_TAIL)
        self.tuning = tuning  # Optional tuning system name (from xpn_tuning_systems)
        self.choke_preset = choke_preset  # "onset" | "standard" | "none"
        self.round_robin = round_robin  # Enable round-robin layer support (default: on)

        # Accumulated outputs
        self.render_specs: list[dict] = []
        self.categories: dict = {}
        # Per-sample 6D Sonic DNA cache: {wav_stem_lower: dna_dict}
        # Populated by categorize stage, consumed by export stage for
        # Legend Feature #1 — Sonic DNA Velocity Sculpting.
        self.sample_dna_cache: dict = {}
        self.expanded_files: list[str] = []
        self.xpm_paths: list[Path] = []
        self.cover_paths: dict = {}
        self.xpn_path: Optional[Path] = None

        # Stage timing
        self.stage_times: dict[str, float] = {}

        # Derived paths
        self.build_dir = output_dir / engine.replace(" ", "_")
        self.specs_dir = self.build_dir / "specs"
        self.samples_dir = self.build_dir / "Samples"
        self.programs_dir = self.build_dir / "Programs"

    @property
    def is_drum_engine(self) -> bool:
        return self.engine in DRUM_ENGINES

    @property
    def preset_slug(self) -> str:
        if self.preset_filter:
            return self.preset_filter.replace(" ", "_")
        return self.engine.replace(" ", "_")

    def ensure_dirs(self):
        """Create the build directory tree (no-op on dry run)."""
        if self.dry_run:
            return
        self.build_dir.mkdir(parents=True, exist_ok=True)
        self.specs_dir.mkdir(exist_ok=True)
        self.samples_dir.mkdir(exist_ok=True)
        self.programs_dir.mkdir(exist_ok=True)


# ---------------------------------------------------------------------------
# Stage runners
# ---------------------------------------------------------------------------

def _stage_render_spec(ctx: PipelineContext) -> None:
    """Stage 1: Generate render specs from .xometa presets."""
    from xpn_render_spec import generate_render_spec

    # Find presets for this engine
    presets_found = []
    for mood_dir in sorted(PRESETS_DIR.iterdir()):
        if not mood_dir.is_dir():
            continue
        # Check engine subdirectories
        for engine_dir in sorted(mood_dir.iterdir()):
            if not engine_dir.is_dir():
                continue
            if engine_dir.name.lower() != ctx.engine.lower():
                continue
            for xmeta in sorted(engine_dir.glob("*.xometa")):
                presets_found.append(xmeta)
        # Also check flat presets in mood dir
        for xmeta in sorted(mood_dir.glob("*.xometa")):
            try:
                with open(xmeta, encoding='utf-8', errors='replace') as f:
                    data = json.load(f)
                engines = data.get("engines", [])
                if isinstance(engines, str):
                    engines = [engines]
                if any(e.lower() == ctx.engine.lower() for e in engines):
                    presets_found.append(xmeta)
            except (json.JSONDecodeError, OSError):
                continue

    if ctx.preset_filter:
        filter_lower = ctx.preset_filter.lower()
        presets_found = [
            p for p in presets_found
            if filter_lower in p.stem.lower()
            or filter_lower in p.stem.lower().replace("_", " ")
        ]

    if not presets_found:
        print(f"    No .xometa presets found for engine '{ctx.engine}'")
        return

    print(f"    Found {len(presets_found)} presets")
    for xmeta in presets_found:
        try:
            spec = generate_render_spec(xmeta)

            # Check for Entangled mood or coupling data — coupling is lost in XPN export
            _preset_data = {}
            try:
                with open(xmeta, encoding='utf-8', errors='replace') as _f:
                    _preset_data = json.load(_f)
            except (json.JSONDecodeError, OSError):
                pass
            _mood = _preset_data.get("mood", "")
            _coupling = _preset_data.get("coupling", _preset_data.get("coupling_data", None))
            if _mood == "Entangled" or _coupling:
                _pname = _preset_data.get("name", xmeta.stem)
                print(f"      [WARN] Entangled preset '{_pname}' — coupling will be "
                      f"lost in XPN export. Consider rendering dry variant.")
                spec["coupling_warning"] = True
            else:
                spec["coupling_warning"] = False

            # Sanitize slug derived from external preset name (#563 — path traversal)
            spec['preset_slug'] = re.sub(
                r'[^a-zA-Z0-9_.-]', '_', spec['preset_slug']
            ).strip('.')

            ctx.render_specs.append(spec)

            if not ctx.dry_run:
                spec_path = ctx.specs_dir / f"{spec['preset_slug']}_render_spec.json"
                # Path containment check before write
                resolved = spec_path.resolve()
                if not resolved.is_relative_to(ctx.specs_dir.resolve()):
                    print(f'[WARN] Path escape blocked: {spec_path}', file=sys.stderr)
                else:
                    with open(spec_path, "w", encoding='utf-8') as f:
                        json.dump(spec, f, indent=2)

            print(f"      {spec['preset_name']}  ({spec['wav_count']} WAVs, {spec['program_type']})")
        except Exception as e:
            print(f"      [WARN] {xmeta.name}: {e}")

    print(f"    Generated {len(ctx.render_specs)} render specs")


def _stage_categorize(ctx: PipelineContext) -> None:
    """Stage 2: Classify WAV samples into voice categories."""
    if not ctx.wavs_dir or not ctx.wavs_dir.exists():
        print("    [SKIP] No --wavs-dir provided or directory does not exist")
        return

    # DNA pre-flight + full per-sample DNA cache (Legend Feature #1 — Sonic DNA Velocity Sculpting)
    try:
        import xpn_auto_dna as _dna  # lazy import
        all_wavs = sorted(ctx.wavs_dir.glob("*.wav")) + sorted(ctx.wavs_dir.glob("*.WAV"))
        wav_sample = all_wavs[:5]
        if wav_sample:
            # Pre-flight summary (first 5 WAVs)
            results = [_dna.compute_dna(str(p)) for p in wav_sample]
            brightness  = sum(r.get("brightness",  0.0) for r in results) / len(results)
            warmth      = sum(r.get("warmth",      0.0) for r in results) / len(results)
            aggression  = sum(r.get("aggression",  0.0) for r in results) / len(results)
            print(f"    DNA pre-flight: brightness={brightness:.2f}, "
                  f"warmth={warmth:.2f}, aggression={aggression:.2f}")
            if aggression > 0.8:
                print("    High aggression pack — consider tagging as 'aggressive'")
            if brightness < 0.3:
                print("    Dark/deep pack — consider tagging as 'dark'")

        # Full DNA scan: populate ctx.sample_dna_cache for every WAV so the
        # export stage can use per-sample DNA for velocity curve sculpting.
        n_dna = 0
        for wav_path in all_wavs:
            try:
                dna = _dna.compute_dna(str(wav_path))
                ctx.sample_dna_cache[wav_path.stem.lower()] = dna
                n_dna += 1
            except Exception as e:
                print(f'[WARN] DNA cache failed: {e}', file=sys.stderr)
        if n_dna:
            print(f"    DNA cache: {n_dna} sample(s) analyzed (velocity sculpting enabled)")
    except ImportError as _imp_err:
        # #821: Warn when the optional module is missing so the user knows
        # Legend Feature #1 (DNA velocity sculpting) is unavailable and why.
        print(f"    [WARN] xpn_auto_dna not available — DNA velocity sculpting disabled "
              f"({_imp_err}). Install with: pip install xpn_auto_dna or check Tools/ path.",
              file=sys.stderr)
    except Exception as _dna_err:
        print(f"    [WARN] DNA pre-flight failed: {_dna_err}", file=sys.stderr)

    from xpn_sample_categorizer import categorize_folder

    ctx.categories = categorize_folder(ctx.wavs_dir, use_audio_analysis=False)
    total = sum(len(v) for v in ctx.categories.values())
    print(f"    Classified {total} samples:")
    for voice in ["kick", "snare", "chat", "ohat", "clap", "tom", "perc", "fx", "unknown"]:
        samples = ctx.categories.get(voice, [])
        if samples:
            print(f"      {voice:8s}: {len(samples)}")


def _stage_expand(ctx: PipelineContext) -> None:
    """Stage 3: Expand flat kits into velocity/cycle/smart WAV sets."""
    if not ctx.wavs_dir or not ctx.wavs_dir.exists():
        print("    [SKIP] No --wavs-dir provided or directory does not exist")
        return

    if not ctx.is_drum_engine:
        print("    [SKIP] Kit expansion only applies to drum engines (Onset)")
        return

    from xpn_kit_expander import expand_kit

    expand_out = ctx.samples_dir / ctx.preset_slug
    if not ctx.dry_run:
        expand_out.mkdir(parents=True, exist_ok=True)

    print(f"    Mode: {ctx.kit_mode}")
    preset_name = ctx.preset_filter or ctx.engine
    summary = expand_kit(
        kit_dir=ctx.wavs_dir,
        preset_name=preset_name,
        expand_mode=ctx.kit_mode,
        output_dir=expand_out,
        dry_run=ctx.dry_run,
    )
    ctx.expanded_files = summary.get("files_written", [])
    print(f"    Expanded: {len(ctx.expanded_files)} files")


def _stage_qa(ctx: PipelineContext) -> None:
    """Stage QA: Perceptual quality check on rendered WAV files."""
    try:
        import xpn_qa_checker
    except ImportError:
        print("    [SKIP] xpn_qa_checker not found")
        return

    # Determine where to look for WAVs: prefer expanded samples dir, fall back to wavs_dir
    search_dirs: list[Path] = []
    if ctx.samples_dir.exists():
        search_dirs.append(ctx.samples_dir)
    if ctx.wavs_dir and ctx.wavs_dir.exists():
        search_dirs.append(ctx.wavs_dir)

    if not search_dirs:
        print("    [SKIP] No WAV directories available for QA")
        return

    # Collect all unique WAV files
    seen: set[str] = set()
    wav_paths: list[Path] = []
    for d in search_dirs:
        for wav in sorted(d.rglob("*.wav")) + sorted(d.rglob("*.WAV")):
            key = str(wav).lower()
            if key not in seen:
                seen.add(key)
                wav_paths.append(wav)

    if not wav_paths:
        print("    [SKIP] No WAV files found for QA")
        return

    if ctx.dry_run:
        print(f"    [DRY] Would run QA on {len(wav_paths)} WAV file(s)")
        return

    print(f"    Checking {len(wav_paths)} WAV file(s)..."
          + (" (auto-fix enabled)" if ctx.auto_fix else ""))

    n_pass   = 0
    n_fail   = 0
    n_fixed  = 0
    warnings: list[str] = []   # issue descriptions for blocking/non-blocking issues
    abort_issues = {"CLIPPING", "PHASE_CANCELLED"}

    for wav_path in wav_paths:
        result = xpn_qa_checker.check_file(wav_path)
        overall = result.get("overall", "ERROR")
        issues  = result.get("issues", [])

        # Auto-remediation: apply safe fixes and re-check before counting
        if ctx.auto_fix and issues:
            fixable = [i for i in issues if i in xpn_qa_checker.AUTO_FIX_SAFE]
            if fixable:
                fixed = xpn_qa_checker.auto_remediate(wav_path, fixable)
                for fix in fixed:
                    n_fixed += 1
                    print(f"      [QA] Auto-fixed {fix} in {wav_path.name}")
                if fixed:
                    # Re-run QA on the now-modified file
                    result = xpn_qa_checker.check_file(wav_path)
                    overall = result.get("overall", "ERROR")
                    issues  = result.get("issues", [])

        if overall == "PASS":
            n_pass += 1
        else:
            n_fail += 1
            for issue in issues:
                desc = f"{issue} in {wav_path.name}"
                warnings.append(desc)
                print(f"      [WARN] {desc}")

    total = n_pass + n_fail
    fix_str = f", {n_fixed} auto-fix{'es' if n_fixed != 1 else ''} applied" if n_fixed else ""
    if warnings:
        summary = f"QA: {n_pass}/{total} passed{fix_str} ({len(warnings)} warning(s): {'; '.join(warnings[:3])}" + \
                  (" ..." if len(warnings) > 3 else "") + ")"
    else:
        summary = f"QA: {n_pass}/{total} passed{fix_str}"
    print(f"    {summary}")

    # Abort logic
    if n_fail > 0 and ctx.strict_qa:
        raise RuntimeError(
            f"QA strict mode: {n_fail} file(s) failed — "
            + ", ".join(warnings[:5])
        )

    # Non-blocking warning for CLIPPING / PHASE_CANCELLED even without --strict-qa
    blocking_found = [w for w in warnings if any(a in w for a in abort_issues)]
    if blocking_found and not ctx.strict_qa:
        print(f"    [WARN] Blocking issues detected (pass --strict-qa to abort): "
              + ", ".join(blocking_found[:3]))

    # --- Musical QA: velocity layer balance and tonal consistency ---
    # Group WAV files by voice (note) to find velocity layer sets.
    # Naming convention: {preset}__{NOTE}__{vel}.WAV — group by note, order by vel tag.
    import re as _re
    _vel_re = _re.compile(r'^(.+)__([A-Ga-g][#bs]?\d+)__(v\d+)\.wav$', _re.IGNORECASE)
    voice_groups: dict[str, list[tuple[int, Path]]] = {}
    for wp in wav_paths:
        m = _vel_re.match(wp.name)
        if m:
            voice_key = f"{m.group(1)}__{m.group(2)}"
            vel_num = int(m.group(3)[1:])
            voice_groups.setdefault(voice_key, []).append((vel_num, wp))

    # Only check groups with 2+ velocity layers
    layer_groups = {k: v for k, v in voice_groups.items() if len(v) >= 2}
    if layer_groups:
        try:
            n_layer_pass = 0
            n_layer_fail = 0
            for voice_key, vel_list in layer_groups.items():
                ordered_paths = [p for _, p in sorted(vel_list)]
                layer_result = xpn_qa_checker.check_velocity_layers(ordered_paths)
                layer_overall = layer_result.get("overall", "ERROR")
                layer_issues = layer_result.get("issues", [])

                if layer_overall == "PASS":
                    n_layer_pass += 1
                else:
                    n_layer_fail += 1
                    for issue in layer_issues:
                        desc = f"{issue} in {voice_key} layers"
                        warnings.append(desc)
                        print(f"      [WARN] {desc}")

            print(f"    Musical QA: {n_layer_pass}/{n_layer_pass + n_layer_fail} "
                  f"voice groups passed layer checks")
        except AttributeError:
            # check_velocity_layers not available in older xpn_qa_checker
            pass


def _stage_smart_trim(ctx: PipelineContext) -> None:
    """Stage: Auto-trim silence tails and add fade-out on rendered WAVs."""
    try:
        from xpn_smart_trim import process as smart_trim_process
    except ImportError:
        print("    [SKIP] xpn_smart_trim not available")
        return

    # Collect WAV files from samples dir and wavs_dir
    search_dirs: list[Path] = []
    if ctx.samples_dir.exists():
        search_dirs.append(ctx.samples_dir)
    if ctx.wavs_dir and ctx.wavs_dir.exists():
        search_dirs.append(ctx.wavs_dir)

    if not search_dirs:
        print("    [SKIP] No WAV directories available for smart trim")
        return

    seen: set[str] = set()
    wav_paths: list[Path] = []
    for d in search_dirs:
        for wav in sorted(d.rglob("*.wav")) + sorted(d.rglob("*.WAV")):
            key = str(wav).lower()
            if key not in seen:
                seen.add(key)
                wav_paths.append(wav)

    if not wav_paths:
        print("    [SKIP] No WAV files found for smart trim")
        return

    if ctx.dry_run:
        print(f"    [DRY] Would smart-trim {len(wav_paths)} WAV file(s)")
        return

    print(f"    Trimming {len(wav_paths)} WAV file(s)...")
    n_trimmed = 0
    n_skipped = 0

    for wav_path in wav_paths:
        try:
            # Trim in-place: write trimmed output to a temp file, then replace
            trimmed_path = wav_path.with_suffix(".trimmed.wav")
            result = smart_trim_process(
                str(wav_path), str(trimmed_path), mode='auto', verbose=False
            )
            # Replace original with trimmed version
            trimmed_path.replace(wav_path)
            n_trimmed += 1
        except Exception as e:
            print(f"      [WARN] {wav_path.name}: {e}")
            n_skipped += 1
            # Clean up temp file if it exists
            trimmed_tmp = wav_path.with_suffix(".trimmed.wav")
            if trimmed_tmp.exists():
                trimmed_tmp.unlink()

    print(f"    Trimmed: {n_trimmed}, Skipped: {n_skipped}")


def _apply_dna_velocity_sculpting(ctx: PipelineContext) -> None:
    """Apply per-sample Sonic DNA velocity curve morphing to generated XPMs.

    Legend Feature #1: velocity curves shaped by the actual sonic character
    of each sample, not just its instrument category.

    Requires xpn_adaptive_velocity.dna_to_velocity_curve and
    ctx.sample_dna_cache populated during the categorize stage.
    """
    try:
        import xml.etree.ElementTree as ET
        from xpn_adaptive_velocity import (
            dna_to_velocity_curve,
            get_instrument_wav,
            get_instrument_layers,
            set_layer_velocity_bounds,
            interpolate_boundaries,
            indent_tree,
            VELOCITY_CURVES,
        )
        from xpn_classify_instrument import classify_wav
    except ImportError as exc:
        print(f"    [WARN] DNA velocity sculpting unavailable: {exc}")
        return

    n_sculpted = 0
    for xpm_path in ctx.xpm_paths:
        if not xpm_path.exists():
            continue
        try:
            tree = ET.parse(xpm_path)
            root = tree.getroot()
            instruments = root.findall(".//Instrument")
            changed = False

            for instrument in instruments:
                wav_ref = get_instrument_wav(instrument)
                if not wav_ref:
                    continue

                # Resolve DNA: check cache by stem (case-insensitive)
                import os as _os
                stem = _os.path.splitext(_os.path.basename(wav_ref))[0].lower()
                sample_dna = ctx.sample_dna_cache.get(stem)
                if sample_dna is None:
                    continue  # no DNA for this sample — leave curve as-is

                # Classify to get instrument type for the base curve
                wav_path = None
                if ctx.samples_dir.exists():
                    candidates = list(ctx.samples_dir.rglob(_os.path.basename(wav_ref)))
                    if candidates:
                        wav_path = str(candidates[0])
                if wav_path is None and ctx.wavs_dir and ctx.wavs_dir.exists():
                    candidates = list(ctx.wavs_dir.rglob(_os.path.basename(wav_ref)))
                    if candidates:
                        wav_path = str(candidates[0])

                if wav_path:
                    try:
                        cls_result = classify_wav(wav_path)
                        instrument_type = cls_result.get("category", "unknown")
                    except Exception:
                        instrument_type = "unknown"
                else:
                    instrument_type = "unknown"

                curve_key = instrument_type if instrument_type in VELOCITY_CURVES else "unknown"
                layers = get_instrument_layers(instrument)
                n_layers = len(layers)
                if n_layers == 0:
                    continue

                sculpted_curve = dna_to_velocity_curve(sample_dna, curve_key, n_layers)
                new_starts = interpolate_boundaries([], sculpted_curve, n_layers)
                set_layer_velocity_bounds(layers, new_starts)
                changed = True

            if changed:
                indent_tree(root)
                tree.write(str(xpm_path), encoding="unicode", xml_declaration=True)
                n_sculpted += 1
        except Exception as exc:
            print(f"    [WARN] DNA sculpting failed for {xpm_path.name}: {exc}")

    if n_sculpted:
        print(f"    DNA velocity sculpting applied to {n_sculpted} XPM(s)")


def _stage_export(ctx: PipelineContext) -> None:
    """Stage 4: Generate .xpm programs."""
    if ctx.is_drum_engine:
        _export_drum(ctx)
    else:
        _export_keygroup(ctx)

    # Legend Feature #1 — Sonic DNA Velocity Sculpting
    # If DNA was gathered during the categorize stage, apply per-instrument
    # DNA-shaped velocity curves to every generated XPM now.
    if ctx.sample_dna_cache and ctx.xpm_paths and not ctx.dry_run:
        _apply_dna_velocity_sculpting(ctx)


def _export_drum(ctx: PipelineContext) -> None:
    """Generate drum .xpm programs."""
    from xpn_drum_export import generate_xpm, build_wav_map

    if not ctx.render_specs:
        print("    [WARN] No render specs — generating XPM with empty WAV map")
        specs = [{"preset_name": ctx.preset_filter or ctx.engine,
                  "preset_slug": ctx.preset_slug}]
    else:
        specs = ctx.render_specs

    for spec in specs:
        name = spec.get("preset_name", ctx.engine)
        slug = spec.get("preset_slug", name.replace(" ", "_"))

        # Look for WAVs in samples dir or wavs_dir
        wav_map = {}
        samples_sub = ctx.samples_dir / slug
        if samples_sub.exists():
            wav_map = build_wav_map(samples_sub, slug, ctx.kit_mode)
        elif ctx.wavs_dir and ctx.wavs_dir.exists():
            wav_map = build_wav_map(ctx.wavs_dir, slug, ctx.kit_mode)

        xpm_content = generate_xpm(name, wav_map, ctx.kit_mode)
        xpm_path = ctx.programs_dir / f"{slug}.xpm"

        if not ctx.dry_run:
            xpm_path.write_text(xpm_content, encoding="utf-8")

        ctx.xpm_paths.append(xpm_path)
        print(f"      {xpm_path.name}  (drum, mode={ctx.kit_mode}, {len(wav_map)} WAVs)")

        # Apply choke preset if requested
        if ctx.choke_preset != "none" and not ctx.dry_run and xpm_path.exists():
            try:
                import xpn_choke_group_assigner as _choke  # lazy import
                _choke.apply_choke_preset(xpm_path, ctx.choke_preset)
                print(f"      Applied choke preset {ctx.choke_preset} to {xpm_path.name}")
            except ImportError:
                print("      [WARN] xpn_choke_group_assigner not available — skipping choke preset")
            except Exception as e:
                print(f"      [WARN] Choke preset failed for {xpm_path.name}: {e}")

    print(f"    Generated {len(ctx.xpm_paths)} drum programs")


def _export_keygroup(ctx: PipelineContext) -> None:
    """Generate keygroup .xpm programs."""
    from xpn_keygroup_export import generate_keygroup_xpm, build_keygroup_wav_map

    if not ctx.render_specs:
        print("    [WARN] No render specs — generating XPM with empty WAV map")
        specs = [{"preset_name": ctx.preset_filter or ctx.engine,
                  "preset_slug": ctx.preset_slug}]
    else:
        specs = ctx.render_specs

    # Resolve tuning system once if requested
    tuning_module = None
    if ctx.tuning:
        try:
            import xpn_tuning_systems as _tun
            if ctx.tuning not in _tun.TUNING_SYSTEMS:
                print(f"    [WARN] Unknown tuning '{ctx.tuning}' — "
                      f"valid names: {', '.join(_tun.TUNING_SYSTEMS)}")
                print("    [WARN] Continuing without tuning.")
            else:
                tuning_module = _tun
                ts_info = _tun.TUNING_SYSTEMS[ctx.tuning]
                max_dev = max(abs(o) for o in _tun.compute_offsets(ctx.tuning))
                print(f"    Tuning:  {ctx.tuning}")
                print(f"             {ts_info.get('description', '')[:80]}")
                print(f"             Max deviation from 12-TET: ±{max_dev:.1f} cents")
                if ts_info.get("reference"):
                    print(f"             Ref: {ts_info['reference'][:80]}")
        except ImportError:
            print("    [WARN] xpn_tuning_systems not found — continuing without tuning.")

    for spec in specs:
        name = spec.get("preset_name", ctx.engine)
        slug = spec.get("preset_slug", name.replace(" ", "_"))

        wav_map = {}
        if ctx.wavs_dir and ctx.wavs_dir.exists():
            wav_map = build_keygroup_wav_map(ctx.wavs_dir, slug)

        xpm_content = generate_keygroup_xpm(name, ctx.engine, wav_map,
                                                  round_robin=ctx.round_robin)

        # Apply tuning: rewrite PitchCents in every Instrument block
        if tuning_module is not None and not ctx.dry_run:
            xpm_content = tuning_module.apply_tuning_to_xpm(xpm_content, ctx.tuning)

        xpm_path = ctx.programs_dir / f"{slug}.xpm"

        if not ctx.dry_run:
            xpm_path.write_text(xpm_content, encoding="utf-8")
        elif tuning_module is not None:
            print(f"      [DRY] Would apply tuning '{ctx.tuning}' to {slug}.xpm")

        ctx.xpm_paths.append(xpm_path)
        tuning_tag = f", tuning={ctx.tuning}" if tuning_module else ""
        print(f"      {xpm_path.name}  (keygroup, {len(wav_map)} WAVs{tuning_tag})")

        # Apply choke preset if requested
        if ctx.choke_preset != "none" and not ctx.dry_run and xpm_path.exists():
            try:
                import xpn_choke_group_assigner as _choke  # lazy import
                _choke.apply_choke_preset(xpm_path, ctx.choke_preset)
                print(f"      Applied choke preset {ctx.choke_preset} to {xpm_path.name}")
            except ImportError:
                print("      [WARN] xpn_choke_group_assigner not available — skipping choke preset")
            except Exception as e:
                print(f"      [WARN] Choke preset failed for {xpm_path.name}: {e}")

    print(f"    Generated {len(ctx.xpm_paths)} keygroup programs")


def _stage_cover_art(ctx: PipelineContext) -> None:
    """Stage 5: Generate branded procedural cover art."""
    try:
        from xpn_cover_art import generate_cover, PILLOW_AVAILABLE
    except ImportError:
        print("    [SKIP] xpn_cover_art not available")
        return

    if not PILLOW_AVAILABLE:
        print("    [SKIP] Pillow/numpy not installed (pip install Pillow numpy)")
        return

    if ctx.dry_run:
        print(f"    [DRY] Would generate cover art for '{ctx.pack_name}'")
        return

    preset_count = len(ctx.render_specs) or 1
    try:
        ctx.cover_paths = generate_cover(
            engine=ctx.engine.upper(),
            pack_name=ctx.pack_name,
            output_dir=str(ctx.build_dir),
            preset_count=preset_count,
            version=ctx.version,
            seed=hash(ctx.pack_name) % 10000,
        )
        print(f"    artwork.png + artwork_2000.png")
    except Exception as e:
        print(f"    [WARN] Cover art failed: {e}")


def _stage_package(ctx: PipelineContext) -> None:
    """Stage 6: Package everything into a .xpn archive."""
    from xpn_packager import package_xpn, XPNMetadata

    if not ctx.xpm_paths and not ctx.dry_run:
        # Check if programs dir has any .xpm files from previous runs
        existing = list(ctx.programs_dir.glob("*.xpm"))
        if not existing:
            print("    [SKIP] No .xpm programs to package")
            return

    pack_slug = ctx.pack_name.replace(" ", "_")
    xpn_path = ctx.output_dir / f"{pack_slug}.xpn"

    tuning_suffix = f" | Tuning: {ctx.tuning}" if ctx.tuning else ""
    meta = XPNMetadata(
        name=ctx.pack_name,
        version=ctx.version,
        author="XO_OX Designs",
        description=f"{ctx.pack_name} expansion pack — XO_OX Designs{tuning_suffix}",
        cover_engine=ctx.engine.upper(),
    )

    if ctx.dry_run:
        print(f"    [DRY] Would package to {xpn_path}")
        print(f"    [DRY] {len(ctx.xpm_paths)} programs, metadata: {meta.name} v{meta.version}")
        return

    # Generate expansion.json + bundle_manifest.json and write them into build_dir
    # so the packager picks them up when sealing the ZIP.
    try:
        from xpn_manifest_generator import generate_manifests
        pack_config = {
            "pack_name": ctx.pack_name,
            "engine":    ctx.engine.upper(),
            "mood":      getattr(ctx, "mood", "Foundation"),
            "version":   ctx.version,
            "bpm":       getattr(ctx, "bpm", None),
            "key":       getattr(ctx, "key", None),
            "tags":      getattr(ctx, "tags", []),
        }
        expansion_json, bundle_json = generate_manifests(pack_config)
        (ctx.build_dir / "expansion.json").write_text(
            json.dumps(expansion_json, indent=2), encoding="utf-8"
        )
        (ctx.build_dir / "bundle_manifest.json").write_text(
            json.dumps(bundle_json, indent=2), encoding="utf-8"
        )
        print("    Manifests: expansion.json + bundle_manifest.json written")
    except ImportError:
        print("    [WARN] xpn_manifest_generator not found — skipping manifests")

    # XPMs are written to ctx.programs_dir (build_dir/Programs/) by _stage_export.
    # _collect_programs() already scans build_dir/Programs/ before the root, so
    # we must NOT copy them into the root — doing so would produce duplicate ZIP
    # entries (one from the Programs/ scan and one from the root scan).
    ctx.xpn_path = package_xpn(
        build_dir=ctx.build_dir,
        output_path=xpn_path,
        metadata=meta,
        include_artwork=True,
        verbose=True,
    )
    print(f"    Output: {ctx.xpn_path}")


def _stage_complement_chain(ctx: PipelineContext) -> None:
    """Stage 5b: Generate primary+complement variant XPM pairs (Artwork collection only)."""
    # Only meaningful for Artwork/Color collection engines.
    if ctx.engine not in ARTWORK_ENGINES:
        print(f"    [SKIP] '{ctx.engine}' is not an Artwork/Color engine "
              f"— complement chain does not apply")
        return

    try:
        from xpn_complement_renderer import run_complement_stage
    except ImportError:
        print("    [SKIP] xpn_complement_renderer not found")
        return

    # Search for presets in the wavs_dir or the build Programs directory.
    presets_search = ctx.wavs_dir or ctx.programs_dir

    if ctx.dry_run:
        print(f"    [DRY] Would render complement chain for '{ctx.engine}' "
              f"from {presets_search}")
        return

    result = run_complement_stage(
        engine_key=ctx.engine,
        output_dir=ctx.output_dir,
        presets_dir=presets_search if (presets_search and presets_search.exists()) else None,
        version=ctx.version,
        skip_cover_art=False,
        dry_run=False,
        verbose=True,
    )

    if result.get("skipped"):
        print(f"    [SKIP] {result.get('reason', '')}")
        return

    n_presets  = result.get("preset_count", 0)
    n_variants = result.get("variant_count", 0)
    print(f"    {n_presets} preset(s) × 5 shades = {n_variants} variant programs")

    # C3: Apply tuning to all newly generated complement XPMs so they match
    # the primary XPMs that were tuned during the export stage.
    if hasattr(ctx, 'tuning') and ctx.tuning:
        try:
            from xpn_tuning_systems import apply_tuning_to_xpm
            complement_xpms = list(ctx.output_dir.glob("*_shade_*.xpm"))
            for xpm_path in complement_xpms:
                xpm_content = xpm_path.read_text(encoding="utf-8")
                tuned_content = apply_tuning_to_xpm(xpm_content, ctx.tuning)
                xpm_path.write_text(tuned_content, encoding="utf-8")
                print(f"      Applied {ctx.tuning} tuning to {xpm_path.name}")
        except ImportError:
            print(f"    [WARN] xpn_tuning_systems not available — complement XPMs not tuned")


# ---------------------------------------------------------------------------
# WAV reading/writing helpers (stdlib only — no numpy/scipy dependency)
# ---------------------------------------------------------------------------

def _read_wav_raw(path: Path) -> tuple[int, int, int, bytes]:
    """Read a WAV file and return (num_channels, sample_rate, bits_per_sample, raw_data).

    Only supports PCM (format tag 1). Raises ValueError for compressed formats.
    """
    with open(path, "rb") as f:
        riff = f.read(4)
        if riff != b"RIFF":
            raise ValueError(f"Not a RIFF file: {path}")
        f.read(4)  # file size
        wave = f.read(4)
        if wave != b"WAVE":
            raise ValueError(f"Not a WAVE file: {path}")

        fmt_found = False
        num_channels = sample_rate = bits_per_sample = 0
        data_bytes = b""

        while True:
            chunk_id = f.read(4)
            if len(chunk_id) < 4:
                break
            chunk_size = struct.unpack("<I", f.read(4))[0]
            if chunk_id == b"fmt ":
                fmt_data = f.read(chunk_size)
                audio_fmt = struct.unpack("<H", fmt_data[0:2])[0]
                if audio_fmt != 1:
                    raise ValueError(f"Unsupported audio format {audio_fmt} (only PCM supported)")
                num_channels = struct.unpack("<H", fmt_data[2:4])[0]
                sample_rate = struct.unpack("<I", fmt_data[4:8])[0]
                bits_per_sample = struct.unpack("<H", fmt_data[14:16])[0]
                fmt_found = True
            elif chunk_id == b"data":
                data_bytes = f.read(chunk_size)
            else:
                f.read(chunk_size)
            # WAV chunks are word-aligned
            if chunk_size % 2 != 0:
                f.read(1)

        if not fmt_found:
            raise ValueError(f"No fmt chunk in {path}")

        return num_channels, sample_rate, bits_per_sample, data_bytes


def _write_wav(path: Path, num_channels: int, sample_rate: int,
               bits_per_sample: int, raw_data: bytes) -> None:
    """Write a PCM WAV file from raw sample bytes."""
    byte_rate = sample_rate * num_channels * (bits_per_sample // 8)
    block_align = num_channels * (bits_per_sample // 8)
    data_size = len(raw_data)
    file_size = 36 + data_size  # RIFF header (minus 8) + fmt (24) + data header (8) + data

    with open(path, "wb") as f:
        # RIFF header
        f.write(b"RIFF")
        f.write(struct.pack("<I", file_size))
        f.write(b"WAVE")
        # fmt chunk
        f.write(b"fmt ")
        f.write(struct.pack("<I", 16))  # chunk size
        f.write(struct.pack("<H", 1))   # PCM
        f.write(struct.pack("<H", num_channels))
        f.write(struct.pack("<I", sample_rate))
        f.write(struct.pack("<I", byte_rate))
        f.write(struct.pack("<H", block_align))
        f.write(struct.pack("<H", bits_per_sample))
        # data chunk
        f.write(b"data")
        f.write(struct.pack("<I", data_size))
        f.write(raw_data)


def _make_silence_bytes(sample_rate: int, num_channels: int,
                        bits_per_sample: int, duration_s: float) -> bytes:
    """Return raw PCM silence bytes for the given duration."""
    n_samples = int(sample_rate * duration_s)
    bytes_per_sample = bits_per_sample // 8
    return b"\x00" * (n_samples * num_channels * bytes_per_sample)


def _generate_preview_wav(wav_paths: list[Path], output_path: Path,
                          max_duration_s: float = 15.0,
                          gap_s: float = 0.5) -> bool:
    """Concatenate the first few WAV samples with silence gaps into a preview file.

    All input WAVs are assumed to share the same format (channels, sample rate,
    bit depth). If they differ, the first file's format is used and mismatched
    files are skipped.

    Returns True if the preview was written, False otherwise.
    """
    if not wav_paths:
        return False

    # Read the first WAV to establish format
    try:
        ref_ch, ref_sr, ref_bps, first_data = _read_wav_raw(wav_paths[0])
    except (ValueError, OSError) as e:
        print(f"      [WARN] Cannot read {wav_paths[0].name}: {e}")
        return False

    bytes_per_sample = ref_bps // 8
    bytes_per_frame = ref_ch * bytes_per_sample
    max_bytes = int(max_duration_s * ref_sr) * bytes_per_frame
    gap_bytes = _make_silence_bytes(ref_sr, ref_ch, ref_bps, gap_s)

    accumulated = bytearray()
    files_used = 0

    for wav_path in wav_paths:
        if len(accumulated) >= max_bytes:
            break

        try:
            ch, sr, bps, data = _read_wav_raw(wav_path)
        except (ValueError, OSError):
            continue

        if ch != ref_ch or sr != ref_sr or bps != ref_bps:
            continue  # format mismatch — skip

        # Add gap before second and subsequent clips
        if files_used > 0:
            accumulated.extend(gap_bytes)

        # Truncate this clip if it would exceed the budget
        remaining = max_bytes - len(accumulated)
        if len(data) > remaining:
            # Truncate to whole frame boundary
            remaining = (remaining // bytes_per_frame) * bytes_per_frame
            data = data[:remaining]

        accumulated.extend(data)
        files_used += 1

    if files_used == 0:
        return False

    # Trim to max duration (whole frame boundary)
    if len(accumulated) > max_bytes:
        max_bytes = (max_bytes // bytes_per_frame) * bytes_per_frame
        accumulated = accumulated[:max_bytes]

    _write_wav(output_path, ref_ch, ref_sr, ref_bps, bytes(accumulated))
    return True


def _stage_preview(ctx: PipelineContext) -> None:
    """Stage: Generate 15-second preview audio for each XPM program."""
    if not ctx.xpm_paths:
        # Check for existing XPMs from a prior run
        existing = list(ctx.programs_dir.glob("*.xpm")) if ctx.programs_dir.exists() else []
        if not existing:
            print("    [SKIP] No .xpm programs — nothing to preview")
            return
        xpm_list = existing
    else:
        xpm_list = ctx.xpm_paths

    # Determine WAV source directories
    search_dirs: list[Path] = []
    if ctx.samples_dir.exists():
        search_dirs.append(ctx.samples_dir)
    if ctx.wavs_dir and ctx.wavs_dir.exists():
        search_dirs.append(ctx.wavs_dir)

    if not search_dirs:
        print("    [SKIP] No WAV directories available for preview generation")
        return

    # Collect all WAV files, grouped by subdirectory slug if possible
    all_wavs: list[Path] = []
    for d in search_dirs:
        all_wavs.extend(sorted(d.rglob("*.wav")))
        all_wavs.extend(sorted(d.rglob("*.WAV")))

    if not all_wavs:
        print("    [SKIP] No WAV files found for preview generation")
        return

    if ctx.dry_run:
        print(f"    [DRY] Would generate preview WAV for {len(xpm_list)} program(s)")
        return

    n_generated = 0
    for xpm_path in xpm_list:
        slug = xpm_path.stem
        preview_path = ctx.programs_dir / f"{slug}.wav"

        # Try to find WAVs specific to this program slug first
        slug_wavs = [w for w in all_wavs if slug.lower() in str(w.parent).lower()
                     or slug.lower() in w.stem.lower()]

        if ctx.is_drum_engine:
            # For drums: use first velocity layer of pads 1-4
            candidates = slug_wavs[:4] if slug_wavs else all_wavs[:4]
        else:
            # For keygroups: use first 4 notes
            candidates = slug_wavs[:4] if slug_wavs else all_wavs[:4]

        if _generate_preview_wav(candidates, preview_path, max_duration_s=15.0, gap_s=0.5):
            size_kb = preview_path.stat().st_size / 1024
            print(f"      {preview_path.name}  ({size_kb:.1f} KB, {len(candidates)} clips)")
            n_generated += 1
        else:
            print(f"      [WARN] Could not generate preview for {slug}")

    print(f"    Generated {n_generated} preview(s)")


# ---------------------------------------------------------------------------
# Loudness normalization helpers (RMS-based)
# Dispatchers (_compute_rms_db / _apply_gain_db) use numpy when available,
# falling back to pure-stdlib implementations.
# ---------------------------------------------------------------------------

def _compute_rms_db_numpy(wav_path: Path) -> float:
    """Numpy-accelerated RMS computation. ~50-100x faster than stdlib loop."""
    try:
        num_ch, sr, bps, data = _read_wav_raw(wav_path)
    except (ValueError, OSError):
        return float("-inf")

    if bps == 16:
        samples = np.frombuffer(data, dtype=np.int16).astype(np.float64) / 32768.0
    elif bps == 24:
        # 24-bit: unpack 3 bytes per sample
        n_samples = len(data) // 3
        raw = np.frombuffer(data[:n_samples * 3], dtype=np.uint8).reshape(-1, 3)
        samples = (raw[:, 0].astype(np.int32) |
                   (raw[:, 1].astype(np.int32) << 8) |
                   (raw[:, 2].astype(np.int32) << 16))
        # Sign extend
        samples = np.where(samples >= 0x800000, samples - 0x1000000, samples)
        samples = samples.astype(np.float64) / 8388608.0
    elif bps == 32:
        samples = np.frombuffer(data, dtype=np.int32).astype(np.float64) / 2147483648.0
    else:
        return float("-inf")

    if len(samples) == 0:
        return float("-inf")

    rms = np.sqrt(np.mean(samples ** 2))
    if rms < 1e-10:
        return float("-inf")
    return 20.0 * np.log10(rms)


def _apply_gain_db_numpy(wav_path: Path, gain_db: float) -> None:
    """Numpy-accelerated gain application with TPDF dither. ~50-100x faster."""
    if abs(gain_db) < 0.01:
        return

    if not np.isfinite(gain_db):
        raise ValueError(f"gain_db must be finite, got {gain_db}")

    num_ch, sr, bps, data = _read_wav_raw(wav_path)
    gain_linear = 10.0 ** (gain_db / 20.0)

    if bps == 16:
        samples = np.frombuffer(data, dtype=np.int16).astype(np.float64)
        # TPDF dither: sum of two uniform [-0.5, 0.5]
        dither = np.random.uniform(-0.5, 0.5, len(samples)) + np.random.uniform(-0.5, 0.5, len(samples))
        result = np.round(samples * gain_linear + dither).astype(np.int64)
        result = np.clip(result, -32768, 32767).astype(np.int16)
        new_data = result.tobytes()
    elif bps == 24:
        n_samples = len(data) // 3
        raw = np.frombuffer(data[:n_samples * 3], dtype=np.uint8).reshape(-1, 3)
        samples = (raw[:, 0].astype(np.int32) |
                   (raw[:, 1].astype(np.int32) << 8) |
                   (raw[:, 2].astype(np.int32) << 16))
        samples = np.where(samples >= 0x800000, samples - 0x1000000, samples).astype(np.float64)
        dither = np.random.uniform(-0.5, 0.5, len(samples)) + np.random.uniform(-0.5, 0.5, len(samples))
        result = np.round(samples * gain_linear + dither).astype(np.int64)
        result = np.clip(result, -8388608, 8388607)
        # Convert back to unsigned for byte packing
        unsigned = np.where(result < 0, result + 0x1000000, result).astype(np.uint32)
        out = np.zeros((len(unsigned), 3), dtype=np.uint8)
        out[:, 0] = unsigned & 0xFF
        out[:, 1] = (unsigned >> 8) & 0xFF
        out[:, 2] = (unsigned >> 16) & 0xFF
        new_data = out.tobytes()
    elif bps == 32:
        samples = np.frombuffer(data, dtype=np.int32).astype(np.float64)
        result = np.round(samples * gain_linear).astype(np.int64)  # No dither at 32-bit
        result = np.clip(result, -2147483648, 2147483647).astype(np.int32)
        new_data = result.tobytes()
    else:
        raise ValueError(f"Unsupported bit depth {bps} in {wav_path} — only 16/24/32-bit PCM supported")

    # Atomic write
    temp_fd, temp_path = tempfile.mkstemp(suffix=".wav", dir=wav_path.parent)
    os.close(temp_fd)
    try:
        _write_wav(Path(temp_path), num_ch, sr, bps, new_data)
        os.replace(temp_path, wav_path)
    except Exception:
        try:
            os.unlink(temp_path)
        except OSError:
            pass
        raise


def _compute_rms_db_stdlib(wav_path: Path) -> float:
    """Compute the RMS level in dB for a WAV file. Returns -inf for silence."""
    try:
        num_ch, sr, bps, data = _read_wav_raw(wav_path)
    except (ValueError, OSError):
        return float("-inf")

    bytes_per_sample = bps // 8
    n_samples = len(data) // bytes_per_sample
    if n_samples == 0:
        return float("-inf")

    # Decode samples to float [-1, 1]
    if bps == 16:
        fmt = f"<{n_samples}h"
        samples = struct.unpack(fmt, data[:n_samples * 2])
        scale = 1.0 / 32768.0
    elif bps == 24:
        samples = []
        for i in range(0, n_samples * 3, 3):
            b = data[i:i + 3]
            if len(b) < 3:
                break
            val = b[0] | (b[1] << 8) | (b[2] << 16)
            if val >= 0x800000:
                val -= 0x1000000
            samples.append(val)
        scale = 1.0 / 8388608.0
    elif bps == 32:
        fmt = f"<{n_samples}i"
        samples = struct.unpack(fmt, data[:n_samples * 4])
        scale = 1.0 / 2147483648.0
    elif bps == 8:
        # 8-bit WAV is unsigned
        samples = [b - 128 for b in data[:n_samples]]
        scale = 1.0 / 128.0
    else:
        return float("-inf")

    sum_sq = 0.0
    for s in samples:
        v = s * scale
        sum_sq += v * v

    rms = math.sqrt(sum_sq / n_samples) if n_samples > 0 else 0.0
    if rms < 1e-10:
        return float("-inf")
    return 20.0 * math.log10(rms)


def _apply_gain_db_stdlib(wav_path: Path, gain_db: float) -> None:
    """Apply a gain offset (in dB) to a WAV file in-place."""
    import random
    if abs(gain_db) < 0.01:
        return  # negligible — skip

    if not math.isfinite(gain_db):
        raise ValueError(f"gain_db must be finite, got {gain_db}")

    num_ch, sr, bps, data = _read_wav_raw(wav_path)
    bytes_per_sample = bps // 8
    n_samples = len(data) // bytes_per_sample
    if n_samples == 0:
        return

    gain_linear = 10.0 ** (gain_db / 20.0)

    if bps == 16:
        fmt = f"<{n_samples}h"
        samples = list(struct.unpack(fmt, data[:n_samples * 2]))
        for i in range(n_samples):
            v = round(samples[i] * gain_linear + random.uniform(-0.5, 0.5) + random.uniform(-0.5, 0.5))  # TPDF dither
            samples[i] = max(-32768, min(32767, v))
        new_data = struct.pack(fmt, *samples)
    elif bps == 24:
        out = bytearray()
        for i in range(0, min(n_samples * 3, len(data)), 3):
            b = data[i:i + 3]
            if len(b) < 3:
                break
            val = b[0] | (b[1] << 8) | (b[2] << 16)
            if val >= 0x800000:
                val -= 0x1000000
            val = round(val * gain_linear + random.uniform(-0.5, 0.5) + random.uniform(-0.5, 0.5))  # TPDF dither
            val = max(-8388608, min(8388607, val))
            if val < 0:
                val += 0x1000000
            out.append(val & 0xFF)
            out.append((val >> 8) & 0xFF)
            out.append((val >> 16) & 0xFF)
        new_data = bytes(out)
    elif bps == 32:
        fmt = f"<{n_samples}i"
        samples = list(struct.unpack(fmt, data[:n_samples * 4]))
        for i in range(n_samples):
            v = round(samples[i] * gain_linear)  # No dither needed at 32-bit (-192 dBFS noise floor)
            samples[i] = max(-2147483648, min(2147483647, v))
        new_data = struct.pack(fmt, *samples)
    else:
        raise ValueError(f"Unsupported bit depth {bps} in {wav_path} — only 16/24/32-bit PCM supported")

    # B3: Atomic write — write to temp file then replace, so a crash mid-write
    # never leaves a corrupted or empty source WAV.
    temp_fd, temp_path = tempfile.mkstemp(suffix=".wav", dir=wav_path.parent)
    os.close(temp_fd)
    try:
        _write_wav(Path(temp_path), num_ch, sr, bps, new_data)
        os.replace(temp_path, wav_path)  # atomic on POSIX
    except Exception:
        os.unlink(temp_path)  # clean up on failure
        raise


def _compute_rms_db(wav_path: Path) -> float:
    """Compute RMS dB — uses numpy if available, falls back to stdlib."""
    if _NUMPY_AVAILABLE:
        return _compute_rms_db_numpy(wav_path)
    return _compute_rms_db_stdlib(wav_path)


def _apply_gain_db(wav_path: Path, gain_db: float) -> None:
    """Apply gain in dB — uses numpy if available, falls back to stdlib."""
    if _NUMPY_AVAILABLE:
        return _apply_gain_db_numpy(wav_path, gain_db)
    return _apply_gain_db_stdlib(wav_path, gain_db)


def _compute_lufs_for_wav(wav_path: Path) -> float:
    """Compute integrated LUFS for a WAV file using xpn_normalize.compute_lufs.

    #817: Batch normalization now uses the same ITU-R BS.1770-4 LUFS
    measurement as per-sample normalization (xpn_normalize.normalize_file_lufs).
    Falls back to RMS if xpn_normalize is unavailable, and warns so the user
    knows which measurement is in use.

    Returns the LUFS value, or float("-inf") for silence/errors.
    """
    try:
        from xpn_normalize import read_wav as _read_wav_xpn, compute_lufs
        frames, sample_rate, n_channels, _bit_depth, _n_frames = _read_wav_xpn(str(wav_path))
        lufs = compute_lufs(frames, sample_rate, n_channels)
        return lufs
    except ImportError:
        # Fallback: use RMS — callers log a one-time warning.
        return _compute_rms_db(wav_path)
    except Exception:
        return float("-inf")


def normalize_batch_loudness(output_base: Path, engine_dirs: list[Path],
                             target_rms_db: float = -14.0,
                             dry_run: bool = False,
                             pack_name: Optional[str] = None) -> dict[str, float]:
    """Normalize loudness across engine output directories.

    #817: Computes integrated LUFS per engine (ITU-R BS.1770-4, via
    xpn_normalize.compute_lufs — the same implementation used by per-sample
    normalization) then applies per-engine gain to reach the target.
    Falls back to RMS if xpn_normalize is unavailable, with a one-time warning.

    Returns a dict of engine_name -> gain_db applied.

    If target_rms_db is None, uses the median loudness across engines as the
    target.

    After normalization, each sample's loudness, true peak, and crest factor
    are recorded to the Cross-Pack Loudness Ledger (Legend Feature #6).
    """
    if _NUMPY_AVAILABLE:
        print("    [INFO] Normalization: using numpy path (~50-100x faster)")
    else:
        print("    [INFO] Normalization: using stdlib path (pip install numpy for acceleration)")

    # #817: Confirm which measurement method is active.
    try:
        from xpn_normalize import compute_lufs as _lufs_check  # noqa: F401
        _lufs_measurement = True
        print("    [INFO] Loudness measurement: LUFS (ITU-R BS.1770-4 via xpn_normalize)")
    except ImportError:
        _lufs_measurement = False
        print("    [WARN] xpn_normalize not available — falling back to RMS approximation. "
              "This produces results inconsistent with per-sample LUFS normalization. "
              "Add xpn_normalize.py to Tools/ to resolve this (#817).")

    # Lazy import — ledger is only needed when normalization actually runs.
    try:
        from xpn_loudness_ledger import record_sample as _ledger_record
        from xpn_loudness_ledger import measure_wav as _ledger_measure
        _ledger_available = True
    except ImportError:
        _ledger_available = False

    _pack_name = pack_name or output_base.name

    engine_loudness: dict[str, list[float]] = {}

    for edir in engine_dirs:
        if not edir.exists():
            continue
        engine_name = edir.name
        wavs = list(edir.rglob("*.wav")) + list(edir.rglob("*.WAV"))
        if not wavs:
            continue

        loudness_values = []
        for wav in wavs:
            # Use LUFS when xpn_normalize is available, RMS otherwise (#817).
            val = _compute_lufs_for_wav(wav) if _lufs_measurement else _compute_rms_db(wav)
            if val > float("-inf"):
                loudness_values.append(val)

        if loudness_values:
            engine_loudness[engine_name] = loudness_values

    if not engine_loudness:
        print("    [SKIP] No WAV files found across engine directories")
        return {}

    # Compute per-engine average loudness
    engine_avg: dict[str, float] = {}
    for name, values in engine_loudness.items():
        engine_avg[name] = sum(values) / len(values)

    # Determine target
    if target_rms_db is None:
        sorted_avgs = sorted(engine_avg.values())
        mid = len(sorted_avgs) // 2
        target = sorted_avgs[mid] if len(sorted_avgs) % 2 == 1 else \
            (sorted_avgs[mid - 1] + sorted_avgs[mid]) / 2.0
    else:
        target = target_rms_db

    # Compute and apply gain offsets
    adjustments: dict[str, float] = {}
    for name, avg in engine_avg.items():
        gain = target - avg
        adjustments[name] = round(gain, 1)

    if dry_run:
        return adjustments

    for edir in engine_dirs:
        engine_name = edir.name
        if engine_name not in adjustments:
            continue
        gain = adjustments[engine_name]
        if abs(gain) < 0.1:
            continue

        wavs = list(edir.rglob("*.wav")) + list(edir.rglob("*.WAV"))
        for wav in wavs:
            try:
                _apply_gain_db(wav, gain)
            except (ValueError, OSError) as e:
                print(f"      [WARN] {wav.name}: {e}")
                continue

            # --- Legend Feature #6: Cross-Pack Loudness Ledger ---
            # Measure the post-normalization sample and record to the ledger.
            # category = direct parent dir name (e.g. "kick", "snare", engine slug).
            if _ledger_available:
                try:
                    metrics = _ledger_measure(wav)
                    if metrics is not None:
                        _ledger_record(
                            pack_name=_pack_name,
                            sample_name=wav.name,
                            loudness_db=metrics["rms_db_proxy"],
                            true_peak=metrics["true_peak"],
                            crest_factor=metrics["crest_factor"],
                            category=wav.parent.name,
                            engine=engine_name,
                        )
                except Exception as exc:
                    print(f"      [WARN] Ledger write failed for {wav.name}: {exc}")

    return adjustments


# ---------------------------------------------------------------------------
# Dependency check (C4) — runs in BOTH dry-run and real mode
# ---------------------------------------------------------------------------

def _check_dependencies(ctx: PipelineContext) -> None:
    """Verify all required modules are importable.

    Raises ImportError if any required module is missing.
    Prints an info line for each missing optional module.
    Runs in both dry-run and real mode so --dry-run gives accurate feedback.
    """
    required = [
        "xpn_render_spec", "xpn_sample_categorizer", "xpn_qa_checker",
        "xpn_smart_trim", "xpn_drum_export", "xpn_packager",
    ]
    optional = [
        "xpn_cover_art", "xpn_complement_renderer", "xpn_manifest_generator",
        "xpn_choke_group_assigner", "xpn_tuning_systems", "xpn_adaptive_velocity",
    ]

    missing_required = []
    missing_optional = []
    for mod in required:
        try:
            __import__(mod)
        except ImportError:
            missing_required.append(mod)
    for mod in optional:
        try:
            __import__(mod)
        except ImportError:
            missing_optional.append(mod)

    if missing_required:
        print(f"  [FAIL] Missing required modules: {', '.join(missing_required)}")
        raise ImportError(f"Required modules not found: {missing_required}")
    if missing_optional:
        print(f"  [INFO] Optional modules not installed: {', '.join(missing_optional)}")
    else:
        print(f"  [OK] All {len(required) + len(optional)} modules available")


# Stage function dispatch
STAGE_FUNCS = {
    "render_spec":      _stage_render_spec,
    "categorize":       _stage_categorize,
    "expand":           _stage_expand,
    "qa":               _stage_qa,
    "smart_trim":       _stage_smart_trim,
    "export":           _stage_export,
    "cover_art":        _stage_cover_art,
    "complement_chain": _stage_complement_chain,
    "preview":          _stage_preview,
    "package":          _stage_package,
}


# ---------------------------------------------------------------------------
# Stage failure hint helper (#822)
# ---------------------------------------------------------------------------

def _stage_failure_hint(stage: str, exc: Exception) -> str:
    """Return a human-readable fix hint for a stage failure.

    Inspects the exception type and message to suggest the most likely remedy.
    Returns an empty string when no specific hint is available.
    """
    msg = str(exc).lower()

    # Missing dependency
    if isinstance(exc, ImportError) or "no module named" in msg:
        mod = getattr(exc, "name", None) or msg.replace("no module named ", "").strip("'")
        return (f"A required Python module is missing ({mod}). "
                f"Try: pip install {mod}  or verify it exists in Tools/")

    # File not found
    if isinstance(exc, FileNotFoundError) or "no such file or directory" in msg:
        return ("A required file or directory was not found. "
                "Check that --wavs-dir and --output-dir paths exist and are accessible.")

    # Permission denied
    if isinstance(exc, PermissionError) or "permission denied" in msg:
        return ("Permission denied while reading or writing a file. "
                "Check file ownership and permissions on the output directory.")

    # Stage-specific hints
    if stage == "render_spec":
        return ("Render spec generation failed. Verify that .xometa preset files exist "
                "under Presets/XOceanus/ for the requested engine.")
    if stage == "export":
        return ("XPM export failed. Ensure xpn_drum_export / xpn_keygroup_export modules "
                "are present in Tools/ and that WAV files were produced by the expand stage.")
    if stage == "package":
        return ("Packaging failed. Check that xpn_packager.py exists in Tools/ and that "
                "at least one .xpm program was produced by the export stage.")
    if stage == "qa":
        return ("QA check failed. Verify that WAV files exist and are valid PCM audio. "
                "Re-run with --skip qa to bypass QA if this is a known-good render.")

    return ""


# ---------------------------------------------------------------------------
# Stage input validation (#820)
# ---------------------------------------------------------------------------

def _validate_stage_inputs(stage: str, ctx: PipelineContext,
                           skip_stages: set[str]) -> str:
    """Check that required inputs for *stage* are present before it executes.

    Returns an error message string if validation fails, or an empty string
    on success.  This is a lightweight check — not a full DAG — that catches
    the most common failure mode of later stages running on missing upstream
    output.
    """
    # expand/qa/smart_trim need WAVs in the samples or wavs directory.
    if stage in ("expand", "qa", "smart_trim"):
        if ctx.wavs_dir and ctx.wavs_dir.exists():
            wavs = list(ctx.wavs_dir.glob("*.wav")) + list(ctx.wavs_dir.glob("*.WAV"))
            if not wavs and stage == "expand":
                return (f"No WAV files found in --wavs-dir {ctx.wavs_dir} — "
                        f"cannot expand empty kit")
        # qa and smart_trim also accept samples_dir output from expand
        return ""

    # export needs render_specs (populated by render_spec stage) or WAVs.
    if stage == "export":
        if "render_spec" not in skip_stages and not ctx.render_specs:
            return ("No render specs were generated — export stage has nothing to process. "
                    "Check that .xometa preset files exist for this engine.")
        return ""

    # package needs at least one XPM program.
    if stage == "package":
        if not ctx.dry_run:
            # Check in-memory list first, then fall back to disk scan
            if not ctx.xpm_paths:
                existing = list(ctx.programs_dir.glob("*.xpm"))
                if not existing:
                    return ("No .xpm programs found — package stage requires at least one "
                            "program generated by the export stage.")
            # #818: Abort package if no WAVs were produced (empty render).
            # Scan samples_dir and wavs_dir.
            wav_count = 0
            if ctx.samples_dir.exists():
                wav_count += len(list(ctx.samples_dir.rglob("*.wav"))) + \
                             len(list(ctx.samples_dir.rglob("*.WAV")))
            if ctx.wavs_dir and ctx.wavs_dir.exists():
                wav_count += len(list(ctx.wavs_dir.rglob("*.wav"))) + \
                             len(list(ctx.wavs_dir.rglob("*.WAV")))
            if wav_count == 0 and "expand" not in skip_stages:
                return ("Render produced zero WAV files — packaging an empty archive "
                        "would create a broken .xpn. Re-run the render stage first.")
        return ""

    return ""


# ---------------------------------------------------------------------------
# Pipeline runner
# ---------------------------------------------------------------------------

def run_pipeline(ctx: PipelineContext, skip_stages: set[str]) -> int:
    """Execute the pipeline stages in order. Returns 0 on success, 1 on failure."""
    print(BANNER)
    print(f"  Engine:     {ctx.engine}")
    print(f"  Output:     {ctx.output_dir}")
    print(f"  WAVs:       {ctx.wavs_dir or '(none)'}")
    print(f"  Pack name:  {ctx.pack_name}")
    print(f"  Kit mode:   {ctx.kit_mode}")
    print(f"  Version:    {ctx.version}")
    if ctx.preset_filter:
        print(f"  Filter:     {ctx.preset_filter}")
    if ctx.tuning:
        print(f"  Tuning:     {ctx.tuning}")
    if ctx.dry_run:
        print(f"  Mode:       DRY RUN")
    if skip_stages:
        print(f"  Skipping:   {', '.join(sorted(skip_stages))}")
    print()

    # Guard: Artwork/Color Collection engines are planned but not yet built.
    # They have no registered presets, no engine directory, and no WAV renders.
    # Running the full pipeline for one will silently produce empty output — issue #823.
    if ctx.engine in ARTWORK_ENGINES:
        print(f"  [WARN] '{ctx.engine}' is an Artwork/Color Collection engine that does not")
        print(f"         yet exist in engine_registry or as a Source/Engines/ directory.")
        print(f"         Only the complement_chain stage is meaningful for this engine.")
        print(f"         All other stages will produce no output (no presets, no WAVs).")
        print()

    ctx.ensure_dirs()

    # Provenance chain — tracks artifact hashes at each stage
    try:
        from xpn_provenance import ProvenanceChain
        provenance = ProvenanceChain(ctx.engine, ctx.pack_name, ctx.version)
    except ImportError:
        provenance = None

    # C4: Dependency check — runs in both dry-run and real mode so --dry-run
    # surfaces missing modules before any stage executes.
    print(f"  [deps] Checking module availability...")
    try:
        _check_dependencies(ctx)
    except ImportError as e:
        print(f"  [FAIL] Dependency check failed: {e}")
        return 1
    print()

    pipeline_start = time.monotonic()
    failed_stage = None

    for stage in STAGES:
        if stage in skip_stages:
            print(f"  [{stage}] SKIPPED")
            continue

        # #820: Validate that required inputs from upstream stages are present
        # before executing this stage.  Skips validation in dry-run mode since
        # no files are actually written.
        if not ctx.dry_run:
            _validation_error = _validate_stage_inputs(stage, ctx, skip_stages)
            if _validation_error:
                print(f"  [{stage}] {STAGE_DESCRIPTIONS[stage]}")
                print(f"    [FAIL] Stage input validation failed: {_validation_error}")
                failed_stage = stage
                break

        print(f"  [{stage}] {STAGE_DESCRIPTIONS[stage]}")
        stage_start = time.monotonic()

        try:
            STAGE_FUNCS[stage](ctx)
        except Exception as e:
            elapsed = time.monotonic() - stage_start
            ctx.stage_times[stage] = elapsed
            # #822: Print full traceback and actionable fix hint, not just the bare message.
            print(f"    [FAIL] {e}")
            print(f"    [TRACEBACK]", file=sys.stderr)
            traceback.print_exc(file=sys.stderr)
            _hint = _stage_failure_hint(stage, e)
            if _hint:
                print(f"    [HINT] {_hint}")
            failed_stage = stage
            break

        elapsed = time.monotonic() - stage_start
        ctx.stage_times[stage] = elapsed
        print(f"    Done ({elapsed:.2f}s)")

        # Record provenance for key stages
        if provenance is not None and not ctx.dry_run:
            if stage == "export" and ctx.xpm_paths:
                for xpm in ctx.xpm_paths:
                    if xpm.exists():
                        provenance.record("export", xpm, f"XPM program: {xpm.name}")
            elif stage == "package" and ctx.xpn_path and ctx.xpn_path.exists():
                provenance.record("package", ctx.xpn_path, "Final .xpn archive")

        print()

    total_time = time.monotonic() - pipeline_start

    # Summary
    print("=" * 60)
    if failed_stage:
        print(f"  PIPELINE FAILED at stage: {failed_stage}")
    else:
        print("  PIPELINE COMPLETE")
    print(f"  Total time: {total_time:.2f}s")
    print()
    print("  Stage timings:")
    for stage in STAGES:
        if stage in skip_stages:
            print(f"    {stage:<14s}  --skipped--")
        elif stage in ctx.stage_times:
            t = ctx.stage_times[stage]
            status = "FAIL" if stage == failed_stage else "OK"
            print(f"    {stage:<14s}  {t:6.2f}s  {status}")
        else:
            print(f"    {stage:<14s}  --not reached--")
    print()

    if ctx.xpn_path and ctx.xpn_path.exists():
        size_kb = ctx.xpn_path.stat().st_size / 1024
        unit = "KB" if size_kb < 1024 else "MB"
        size_val = size_kb if size_kb < 1024 else size_kb / 1024
        print(f"  Output: {ctx.xpn_path} ({size_val:.1f} {unit})")
    elif ctx.dry_run:
        print("  (Dry run — no files written)")

    # Write provenance chain before pipeline state
    if not ctx.dry_run and not failed_stage and provenance is not None:
        try:
            prov_path = ctx.build_dir / "provenance.json"
            prov_path.write_text(provenance.to_json(), encoding="utf-8")
            print(f"  Provenance: {prov_path}")
        except Exception as exc:
            print(f"  [WARN] Provenance write failed: {exc}")

    # Write pipeline state for status command
    if not ctx.dry_run:
        _write_state(ctx, failed_stage)

    return 1 if failed_stage else 0


def _write_state(ctx: PipelineContext, failed_stage: Optional[str] = None):
    """Persist pipeline state to JSON for the status command."""
    state = {
        "engine":        ctx.engine,
        "pack_name":     ctx.pack_name,
        "version":       ctx.version,
        "timestamp":     datetime.now().isoformat(),
        "render_specs":  len(ctx.render_specs),
        "xpm_count":     len(ctx.xpm_paths),
        "expanded_files": len(ctx.expanded_files),
        "xpn_path":      str(ctx.xpn_path) if ctx.xpn_path else None,
        "stage_times":   ctx.stage_times,
        "failed_stage":  failed_stage,
        "stages":        {
            stage: ("skipped" if stage not in ctx.stage_times and stage != failed_stage
                    else "failed" if stage == failed_stage
                    else "ok" if stage in ctx.stage_times
                    else "not_reached")
            for stage in STAGES
        },
    }
    state_path = ctx.build_dir / ".oxport_state.json"
    try:
        ctx.build_dir.mkdir(parents=True, exist_ok=True)
        with open(state_path, "w", encoding='utf-8') as f:
            json.dump(state, f, indent=2)
    except OSError:
        pass  # non-critical


# ---------------------------------------------------------------------------
# Subcommands
# ---------------------------------------------------------------------------

def cmd_run(args) -> int:
    """Execute the full pipeline."""
    skip = set()
    if args.skip:
        skip = {s.strip() for s in args.skip.split(",")}
        invalid = skip - set(STAGES)
        if invalid:
            print(f"ERROR: Unknown stages to skip: {invalid}")
            print(f"Valid stages: {', '.join(STAGES)}")
            return 1

    # Auto-skip complement_chain unless --collection artwork is specified and the
    # engine is an Artwork/Color collection engine.
    collection = getattr(args, "collection", None)
    if "complement_chain" not in skip:
        is_artwork = (collection == "artwork") or (args.engine in ARTWORK_ENGINES)
        if not is_artwork:
            skip.add("complement_chain")

    ctx = PipelineContext(
        engine=args.engine,
        output_dir=Path(args.output_dir),
        wavs_dir=Path(args.wavs_dir) if args.wavs_dir else None,
        preset_filter=args.preset,
        kit_mode=args.kit_mode,
        version=args.version,
        pack_name=args.pack_name,
        dry_run=args.dry_run,
        strict_qa=getattr(args, "strict_qa", False),
        auto_fix=getattr(args, "auto_fix", False),
        tuning=getattr(args, "tuning", None),
        choke_preset=getattr(args, "choke_preset", "none"),
        round_robin=getattr(args, "round_robin", True),
    )
    return run_pipeline(ctx, skip)


def cmd_status(args) -> int:
    """Show pipeline state for an output directory."""
    output_dir = Path(args.output_dir)
    if not output_dir.exists():
        print(f"Output directory does not exist: {output_dir}")
        return 1

    # Find all .oxport_state.json files
    states = sorted(output_dir.glob("**/.oxport_state.json"))
    if not states:
        print(f"No pipeline state found in {output_dir}")
        print("Run `oxport.py run` first to create a pipeline state.")
        return 0

    for state_path in states:
        try:
            with open(state_path, encoding='utf-8', errors='replace') as f:
                state = json.load(f)
        except (json.JSONDecodeError, OSError) as e:
            print(f"  [WARN] Could not read {state_path}: {e}")
            continue

        engine = state.get("engine", "?")
        pack = state.get("pack_name", "?")
        ts = state.get("timestamp", "?")
        failed = state.get("failed_stage")

        status_icon = "FAILED" if failed else "COMPLETE"
        print(f"  {pack} ({engine}) — {status_icon}")
        print(f"    Timestamp: {ts}")
        print(f"    Render specs: {state.get('render_specs', 0)}")
        print(f"    Programs: {state.get('xpm_count', 0)}")
        print(f"    Expanded files: {state.get('expanded_files', 0)}")
        if state.get("xpn_path"):
            print(f"    XPN: {state['xpn_path']}")

        stages = state.get("stages", {})
        times = state.get("stage_times", {})
        print("    Stages:")
        for stage in STAGES:
            s = stages.get(stage, "?")
            t = times.get(stage)
            time_str = f" ({t:.2f}s)" if t is not None else ""
            print(f"      {stage:<14s}  {s}{time_str}")

        if failed:
            print(f"    FAILED at: {failed}")
        print()

    return 0


def cmd_validate(args) -> int:
    """Validate pipeline output AND/OR .xometa presets."""
    exit_code = 0

    # --- Mode 1: Validate pipeline output directory (structural checks) ---
    if args.output_dir:
        output_dir = Path(args.output_dir)
        print(f"  Validate pipeline output: {output_dir}")
        print()

        checks_passed = 0
        checks_failed = 0

        xpms = list(output_dir.rglob("*.xpm"))
        if xpms:
            print(f"    [OK] Found {len(xpms)} .xpm program(s)")
            checks_passed += 1
        else:
            print(f"    [FAIL] No .xpm programs found")
            checks_failed += 1

        xpns = list(output_dir.glob("*.xpn"))
        if xpns:
            for xpn in xpns:
                size_kb = xpn.stat().st_size / 1024
                print(f"    [OK] {xpn.name} ({size_kb:.1f} KB)")
                checks_passed += 1
        else:
            print(f"    [INFO] No .xpn archives found (run full pipeline to generate)")

        artwork = list(output_dir.rglob("artwork.png"))
        if artwork:
            print(f"    [OK] Cover art found ({len(artwork)} file(s))")
            checks_passed += 1
        else:
            print(f"    [INFO] No cover art found")

        wavs = list(output_dir.rglob("*.wav")) + list(output_dir.rglob("*.WAV"))
        if wavs:
            print(f"    [OK] {len(wavs)} WAV sample(s)")
            checks_passed += 1
        else:
            print(f"    [INFO] No WAV samples found")

        for xpm in xpms[:5]:
            try:
                content = xpm.read_text(encoding="utf-8")
                if "<MPCVObject>" in content and "</MPCVObject>" in content:
                    print(f"    [OK] {xpm.name} — valid XML structure")
                    checks_passed += 1
                else:
                    print(f"    [FAIL] {xpm.name} — missing MPCVObject tags")
                    checks_failed += 1
            except Exception as e:
                print(f"    [FAIL] {xpm.name} — {e}")
                checks_failed += 1

        # Q-Link validation on all XPM files
        if xpms:
            try:
                from xpn_qa_checker import check_xpm_qlinks
                qlink_issues = 0
                for xpm in xpms:
                    qr = check_xpm_qlinks(xpm)
                    if qr["overall"] == "PASS":
                        qlinks_str = ", ".join(qr.get("qlinks", []))
                        print(f"    [OK] {xpm.name} — Q-Links: {qlinks_str}")
                        checks_passed += 1
                    else:
                        for issue in qr.get("issues", []):
                            if issue == "QLINK_NAME_TOO_LONG":
                                long_names = [n for n in qr.get("qlinks", []) if len(n) > 10]
                                print(f"    [FAIL] {xpm.name} — Q-Link name too long: {long_names}")
                            elif issue == "QLINK_MISSING":
                                print(f"    [FAIL] {xpm.name} — No Q-Link assignments")
                            checks_failed += 1
                            qlink_issues += 1
                if qlink_issues == 0:
                    print(f"    Q-Link check: all {len(xpms)} programs have valid assignments")
            except ImportError:
                print("    [WARN] xpn_qa_checker not available — skipping Q-Link validation")

        print()
        print(f"  Pipeline checks — Passed: {checks_passed}  Failed: {checks_failed}")
        if checks_failed > 0:
            exit_code = 1

    # --- Mode 2: Validate .xometa presets (delegates to validate_presets.py) ---
    if args.presets:
        print()
        print(f"  Validate .xometa presets …")
        print()
        try:
            tools_dir = Path(__file__).resolve().parent
            vp_cmd = [sys.executable, str(tools_dir / "validate_presets.py")]
            if args.fix:
                vp_cmd.append("--fix")
            if args.strict:
                vp_cmd.append("--strict")
            vp_cmd.append("--report")

            result = subprocess.run(vp_cmd, timeout=300)
            vp_rc = result.returncode

            if vp_rc != 0:
                exit_code = max(exit_code, vp_rc)
        except FileNotFoundError:
            print("    [ERROR] validate_presets.py not found in Tools/")
            exit_code = 2
        except Exception as e:
            print(f"    [ERROR] Preset validation failed: {e}")
            exit_code = 2


    # --- Mode 3: Validate .xpn archive(s) via xpn_validator.py ---
    if getattr(args, "xpn", None):
        print()
        print(f"  Validate .xpn archive(s): {args.xpn}")
        print()
        try:
            tools_dir = Path(__file__).resolve().parent
            xv_cmd = [sys.executable, str(tools_dir / "xpn_validator.py"), args.xpn]
            if args.strict:
                xv_cmd.append("--strict")

            result = subprocess.run(xv_cmd, timeout=300)
            xv_rc = result.returncode

            if xv_rc != 0:
                exit_code = max(exit_code, xv_rc)
        except FileNotFoundError:
            print("    [ERROR] xpn_validator.py not found in Tools/")
            exit_code = 2
        except Exception as e:
            print(f"    [ERROR] .xpn validation failed: {e}")
            exit_code = 2

    if not args.output_dir and not args.presets and not getattr(args, "xpn", None):
        print("  Nothing to validate. Use --output-dir, --presets, and/or --xpn.")
        print()
        return 1

    print()
    return exit_code


def cmd_batch(args) -> int:
    """Run the pipeline for multiple engines from a JSON config file."""
    print(BANNER)

    config_path = Path(args.config)
    if not config_path.exists():
        print(f"  [ERROR] Config file not found: {config_path}")
        return 2

    try:
        config = json.loads(config_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as e:
        print(f"  [ERROR] Invalid JSON in config: {e}")
        return 2

    batch_name = config.get("batch_name", config_path.stem)
    output_base = Path(config.get("output_base_dir", "./dist/"))
    jobs = config.get("jobs", [])

    if not jobs:
        print(f"  [ERROR] No jobs found in config")
        return 2

    print(f"  Batch: {batch_name}")
    print(f"  Output: {output_base}")
    print(f"  Jobs: {len(jobs)}")
    print()

    try:
        import xpn_batch_export as batcher
    except ImportError:
        print("  [ERROR] xpn_batch_export.py not found in Tools/")
        return 2

    parallel = getattr(args, "parallel", 1)
    dry_run = getattr(args, "dry_run", False)
    skip_failed = getattr(args, "skip_failed", False)

    if parallel > 1:
        results = batcher.run_jobs_parallel(
            jobs, output_base, dry_run, parallel, skip_failed)
    else:
        results = []
        for job in jobs:
            result = batcher.run_job(job, output_base, dry_run)
            results.append(result)
            if not result.passed and not skip_failed:
                print(f"  [ABORT] {result.engine} failed — stopping batch")
                break

    # --- Post-batch loudness normalization ---
    do_normalize = getattr(args, "normalize", False)
    if do_normalize:
        print()
        print(f"  LOUDNESS NORMALIZATION")
        print(f"  {'─'*50}")

        # Collect engine output directories
        engine_dirs: list[Path] = []
        for job in jobs:
            engine = job.get("engine", "unknown")
            edir = output_base / engine.replace(" ", "_")
            if edir.exists():
                engine_dirs.append(edir)

        target = getattr(args, "normalize_target", -14.0)
        dry_run_norm = getattr(args, "dry_run", False)

        adjustments = normalize_batch_loudness(
            output_base, engine_dirs,
            target_rms_db=target,
            dry_run=dry_run_norm,
        )

        if adjustments:
            prefix = "[DRY] " if dry_run_norm else ""
            for name, gain in sorted(adjustments.items()):
                sign = "+" if gain >= 0 else ""
                print(f"    {prefix}{name}: {sign}{gain:.1f} dB")
            print()
        else:
            print("    No adjustments needed (or no WAVs found)")
            print()

    # Summary
    print()
    print(f"  {'='*50}")
    print(f"  BATCH SUMMARY: {batch_name}")
    print(f"  {'='*50}")
    passed = sum(1 for r in results if r.passed)
    failed = len(results) - passed
    for r in results:
        duration = f"{r.duration:.1f}s" if r.duration > 0 else "—"
        xpn = r.output_xpn.name if r.output_xpn else "—"
        print(f"    {r.status_label:6s}  {r.engine:<20s}  {duration:>8s}  {xpn}")
    print()
    print(f"  Passed: {passed}  Failed: {failed}  Total: {len(results)}")
    return 1 if failed > 0 else 0


# ---------------------------------------------------------------------------
# BUILD command — .oxbuild compiler (VQ 005)
# ---------------------------------------------------------------------------

BUILD_STAGES = [
    "parse", "select", "render", "assemble",
    "fallback", "intent", "docs", "art", "listen", "package", "validate",
]


def _estimate_disk_usage(total_jobs: int, render_duration_ms: int, render_release_ms: int,
                         sample_rate: int, bit_depth: int = 24, channels: int = 2) -> int:
    """Return estimated total bytes for a render run.

    Uses the same formula as the dry-run display so the two are consistent.
    Formula: jobs × (hold_ms + release_ms) / 1000 × sample_rate × channels × bytes_per_sample
    """
    bytes_per_sample = (bit_depth + 7) // 8  # 24-bit → 3
    avg_wav_bytes = (
        (render_duration_ms + render_release_ms) / 1000.0
        * sample_rate * channels * bytes_per_sample
    )
    return int(total_jobs * avg_wav_bytes)


def _check_disk_space(output_dir: Path, estimated_bytes: int, margin: float = 1.20) -> bool:
    """Check that output_dir has enough free space for the render.

    Returns True if sufficient space is available.
    Prints an [ABORT] message and returns False if not.
    The check requires margin × estimated_bytes free (default 20% headroom).
    """
    try:
        free_bytes = shutil.disk_usage(output_dir).free
    except OSError:
        # Can't stat the directory — warn but don't block
        print(f"         [WARN] Could not check disk space for {output_dir}")
        return True

    needed_bytes = estimated_bytes * margin
    if free_bytes < needed_bytes:
        need_gb = needed_bytes / 1_073_741_824
        free_gb = free_bytes / 1_073_741_824
        print(f"[ABORT] Insufficient disk space. "
              f"Need ~{need_gb:.1f}GB (with 20% margin), "
              f"only {free_gb:.1f}GB available.")
        return False
    return True


# ---------------------------------------------------------------------------
# Build log helpers (Feature #734 — --resume support)
# ---------------------------------------------------------------------------

def _update_build_log(log_path: Path, stage_name: str, status: str = "completed") -> None:
    """Append a completed stage to build.log.json after each stage finishes.

    Idempotent: calling with the same stage_name multiple times does not
    produce duplicate entries.  Also writes a ``last_updated`` timestamp so
    callers can see when the last successful stage completed.
    """
    try:
        if log_path.exists():
            with open(log_path, encoding='utf-8', errors='replace') as _f:
                log = json.load(_f)
        else:
            log = {"completed_stages": []}

        if status == "completed" and stage_name not in log.get("completed_stages", []):
            log.setdefault("completed_stages", []).append(stage_name)

        log["last_updated"] = datetime.now().isoformat()

        with open(log_path, "w", encoding='utf-8') as _f:
            json.dump(log, _f, indent=2)
    except Exception:
        pass  # build log is best-effort — never abort the pipeline for a log write failure


def _load_resume_state(log_path: Path) -> list[str]:
    """Return list of completed stage names from an existing build.log.json.

    Returns an empty list if the file does not exist or cannot be parsed.
    """
    if not log_path.exists():
        return []
    try:
        with open(log_path, encoding='utf-8', errors='replace') as _f:
            log = json.load(_f)
        return log.get("completed_stages", [])
    except Exception:
        return []


# ---------------------------------------------------------------------------
# Doctor — environment validation (QDD issue #736)
# ---------------------------------------------------------------------------

_DOCTOR_REQUIRED_MODULES = [
    ("sounddevice", "sounddevice"),
    ("mido",        "mido"),
    ("numpy",       "numpy"),
    ("json",        "json"),
]

# XPN file-size guard: MPC hardware chokes on .xpn archives above ~2 GB.
_XPN_SIZE_LIMIT_BYTES = 2_147_483_648  # 2 GiB

# Bytes/sample estimate used for disk/XPN-size checks (24-bit stereo).
_BYTES_PER_SAMPLE_24BIT_STEREO = 6   # 3 bytes × 2 channels


def _doctor_line(status: str, message: str) -> str:
    """Format a single doctor check line."""
    icons = {"PASS": "PASS", "FAIL": "FAIL", "WARN": "WARN", "SKIP": "SKIP"}
    tag = icons.get(status, status)
    return f"  │  [{tag}] {message}"


def _quick_doctor_check(args) -> bool:
    """Lightweight gate run at the start of cmd_build (no smoke test).

    Checks: (1) required Python modules importable, (2) enough disk space.
    Prints warnings/errors but does NOT abort — returns False if any check
    fails so the caller can decide whether to proceed.

    Respects ``--skip-doctor`` flag on *args* if present.
    """
    if getattr(args, "skip_doctor", False):
        return True

    all_ok = True

    # --- Python dependencies ---
    missing_mods = []
    for mod_name, _ in _DOCTOR_REQUIRED_MODULES:
        try:
            __import__(mod_name)
        except ImportError:
            missing_mods.append(mod_name)

    if missing_mods:
        print(f"[DOCTOR] Missing Python modules: {', '.join(missing_mods)}")
        print(f"         Run: pip install {' '.join(missing_mods)}")
        all_ok = False

    # --- Disk space (quick estimate from spec if available) ---
    oxbuild_path = Path(getattr(args, "oxbuild", "") or "")
    check_dir = Path.home()
    if oxbuild_path.exists():
        output_dir = Path(args.output_dir) if getattr(args, "output_dir", None) else REPO_ROOT / "builds"
        check_dir = output_dir
    try:
        free_bytes = shutil.disk_usage(check_dir).free
        free_gb = free_bytes / 1_073_741_824
        if free_gb < 5.0:
            print(f"[DOCTOR] Low disk space: {free_gb:.1f} GB free on {check_dir}")
            print(f"         Large renders need 20–100 GB. Free up space before building.")
            all_ok = False
    except OSError:
        pass  # Can't stat — skip silently in quick mode

    return all_ok


def cmd_doctor(args) -> int:
    """oxport doctor — validate environment before rendering."""
    import math as _math

    ci_mode: bool = getattr(args, "ci", False)
    audio_device_name: str = getattr(args, "audio_device", "BlackHole")
    midi_port_name: str = getattr(args, "midi_port", "IAC Driver Bus 1")
    spec_path_raw: str | None = getattr(args, "spec", None)

    pass_count = 0
    warn_count = 0
    skip_count = 0
    fail_count = 0
    lines: list[str] = []

    def _emit(status: str, msg: str) -> None:
        nonlocal pass_count, warn_count, skip_count, fail_count
        lines.append(_doctor_line(status, msg))
        if status == "PASS":
            pass_count += 1
        elif status == "WARN":
            warn_count += 1
        elif status == "SKIP":
            skip_count += 1
        elif status == "FAIL":
            fail_count += 1

    def _section(title: str) -> None:
        lines.append("")
        lines.append(f"  {title}")

    # ── Print header ─────────────────────────────────────────────────────────
    print()
    print("  ╔══════════════════════════════════════")
    print("    OXPORT DOCTOR")
    print("  ╔══════════════════════════════════════")

    # ── 1. Python Dependencies ───────────────────────────────────────────────
    _section("Python Dependencies")
    all_modules_ok = True
    for mod_name, import_as in _DOCTOR_REQUIRED_MODULES:
        try:
            mod = __import__(mod_name)
            # Prefer __version__; fall back to version attribute only if it's a string.
            _v = getattr(mod, "__version__", None)
            if _v is None:
                _candidate = getattr(mod, "version", None)
                if isinstance(_candidate, str):
                    _v = _candidate
            ver_str = f" {_v}" if _v else ""
            _emit("PASS", f"{mod_name}{ver_str}")
        except ImportError:
            _emit("FAIL", f"{mod_name} NOT FOUND  (pip install {mod_name})")
            all_modules_ok = False

    if all_modules_ok:
        _emit("PASS", "All required modules importable")
    else:
        _emit("FAIL", "One or more required modules missing — install before rendering")

    # ── 2. Audio Chain ───────────────────────────────────────────────────────
    _section("Audio Chain")
    sd_module = None
    mido_module = None
    found_device_idx: int | None = None
    found_device_sr: int | None = None
    found_midi_port: bool = False

    if ci_mode:
        _emit("SKIP", f"Audio device check ({audio_device_name}) — CI mode")
        _emit("SKIP", f"MIDI port check ({midi_port_name}) — CI mode")
    else:
        # Audio device
        try:
            import sounddevice as _sd
            sd_module = _sd
            devices = _sd.query_devices()
            needle = audio_device_name.lower()
            for idx, d in enumerate(devices):
                if needle in d["name"].lower() and d["max_input_channels"] > 0:
                    found_device_idx = idx
                    found_device_sr = int(d["default_samplerate"])
                    _emit("PASS", f"{audio_device_name} found (device index {idx})")
                    _emit("PASS", f"Device sample rate: {found_device_sr} Hz")
                    break
            else:
                _emit("FAIL", f"{audio_device_name} NOT found in audio input devices")
                _emit("INFO", "Available input devices:")
                for idx, d in enumerate(devices):
                    if d["max_input_channels"] > 0:
                        lines.append(f"          [{idx}] {d['name']}")
        except ImportError:
            _emit("FAIL", "sounddevice not installed — cannot check audio devices")
        except Exception as e:
            _emit("FAIL", f"Audio device query failed: {e}")

        # MIDI port
        try:
            import mido as _mido
            mido_module = _mido
            ports = _mido.get_output_names()
            needle = midi_port_name.lower()
            for p in ports:
                if needle in p.lower():
                    found_midi_port = True
                    _emit("PASS", f"{midi_port_name} found")
                    break
            else:
                _emit("FAIL", f"MIDI port '{midi_port_name}' NOT found")
                if ports:
                    lines.append(f"         Available MIDI ports: {', '.join(ports)}")
                else:
                    lines.append("         No MIDI output ports found at all")
        except ImportError:
            _emit("FAIL", "mido not installed — cannot check MIDI ports")
        except Exception as e:
            _emit("FAIL", f"MIDI port query failed: {e}")

    # ── 3. Disk Space ────────────────────────────────────────────────────────
    _section("Disk Space")
    check_dir = Path.home()
    spec_data: dict | None = None
    spec_path: Path | None = None

    if spec_path_raw:
        sp = Path(spec_path_raw)
        if sp.exists():
            spec_path = sp
            try:
                with open(sp, encoding="utf-8", errors="replace") as f:
                    spec_data = json.load(f)
                output_dir_for_disk = REPO_ROOT / "builds" / spec_data.get("pack_id", "unknown")
                check_dir = output_dir_for_disk.parent
            except Exception:
                pass

    try:
        usage = shutil.disk_usage(check_dir)
        free_gb = usage.free / 1_073_741_824
        _emit("PASS", f"{free_gb:.1f} GB free on {check_dir}")

        # Estimate disk requirement if spec is available
        if spec_data:
            rendering = spec_data.get("rendering", {})
            sr = rendering.get("sample_rate", 44100)
            dur_ms = rendering.get("duration_ms", 3000) + rendering.get("release_tail_ms", 500)
            n_presets = spec_data.get("preset_selection", {}).get("min_presets", 16)
            try:
                from xpn_voice_taxonomy import ONSET_VOICES, rr_count as _rr_count
                from xpn_velocity_standard import NUM_LAYERS
                n_jobs = n_presets * sum(max(1, _rr_count(v)) for v in ONSET_VOICES) * NUM_LAYERS
            except ImportError:
                n_jobs = n_presets * 16 * 4  # rough fallback
            estimated_bytes = _estimate_disk_usage(n_jobs, dur_ms - 500, 500, sr)
            needed_gb = estimated_bytes * 1.2 / 1_073_741_824
            if usage.free >= estimated_bytes * 1.2:
                _emit("PASS", f"Sufficient for estimated render (~{needed_gb:.1f} GB needed)")
            else:
                _emit("FAIL",
                      f"Insufficient disk: need ~{needed_gb:.1f} GB, only {free_gb:.1f} GB free")
        else:
            _emit("PASS", "Sufficient for estimated render (no spec — skipping size estimate)")
    except OSError as e:
        _emit("WARN", f"Could not check disk space: {e}")

    # ── 4. Spec Validation ───────────────────────────────────────────────────
    if spec_data is not None:
        _section(f"Spec Validation ({spec_path.name if spec_path else 'unknown'})")

        _emit("PASS", "File parses correctly")

        # Required fields
        required_fields = ["pack_id", "engine", "platform", "archetype"]
        missing_fields = [k for k in required_fields if k not in spec_data]
        if missing_fields:
            _emit("FAIL", f"Missing required fields: {', '.join(missing_fields)}")
        else:
            _emit("PASS", f"All required fields present")

        # Tier + slot budget
        tier_raw = spec_data.get("tier", "DEEP").upper()
        try:
            t_cfg = _get_tier_config(tier_raw)
            max_slots = t_cfg["max_slots"]
            try:
                from xpn_voice_taxonomy import ONSET_VOICES, compute_slot_budget
                from xpn_velocity_standard import NUM_LAYERS
                slot_budget = compute_slot_budget(ONSET_VOICES, t_cfg["vel_layers"])
                _emit("PASS", f"Tier: {tier_raw} ({slot_budget}/{max_slots} slots)")
            except ImportError:
                _emit("PASS", f"Tier: {tier_raw} (max_slots: {max_slots})")
        except ValueError as e:
            _emit("FAIL", str(e))

        # Sample rate vs device
        sr_spec = spec_data.get("rendering", {}).get("sample_rate")
        if sr_spec:
            if found_device_sr is not None:
                if sr_spec == found_device_sr:
                    _emit("PASS", f"sample_rate: {sr_spec} matches device")
                else:
                    _emit("WARN",
                          f"sample_rate mismatch: spec={sr_spec}, device={found_device_sr}")
            else:
                _emit("SKIP", f"sample_rate: {sr_spec} (device not checked — CI mode or device missing)")
        else:
            _emit("WARN", "No sample_rate in rendering block")

        # Preset count
        n_presets = spec_data.get("preset_selection", {}).get("min_presets")
        if n_presets is not None:
            _emit("PASS", f"{n_presets} presets configured")
        else:
            _emit("WARN", "preset_selection.min_presets not set")

        # corner_strategy
        cs = spec_data.get("corner_strategy", "none")
        if cs in ("none", None, ""):
            _emit("WARN", "corner_strategy: none (disabled)")
        else:
            _emit("PASS", f"corner_strategy: {cs}")

        # voice_map.json existence (if renders dir exists)
        if spec_path:
            vm_path = REPO_ROOT / "builds" / spec_data.get("pack_id", "") / "renders" / "voice_map.json"
            if vm_path.exists():
                _emit("PASS", f"voice_map.json found ({vm_path.name})")
            else:
                _emit("WARN", "voice_map.json not found (will be generated during build)")

    elif spec_path_raw:
        _section("Spec Validation")
        _emit("FAIL", f"Spec file not found: {spec_path_raw}")

    # ── 5. Smoke Test ────────────────────────────────────────────────────────
    _section("Smoke Test")
    if ci_mode:
        _emit("SKIP", "Smoke test (--ci mode)")
    elif found_device_idx is None or not found_midi_port:
        _emit("SKIP", "Smoke test (audio device or MIDI port unavailable)")
    else:
        try:
            import sounddevice as _sd
            import mido as _mido
            import numpy as _np

            sr = found_device_sr or 44100
            duration_s = 0.3
            note = 36
            velocity = 80
            midi_ch = 0

            # Find exact port name
            port_names = _mido.get_output_names()
            exact_port = next(
                (p for p in port_names if midi_port_name.lower() in p.lower()), None
            )

            with _mido.open_output(exact_port) as midi_out:
                # Start recording first
                recording = _sd.rec(
                    int(sr * duration_s),
                    samplerate=sr, channels=2, dtype="float32",
                    device=found_device_idx,
                )
                # Small settle time before note
                import time as _time_sm
                _time_sm.sleep(0.02)

                midi_out.send(_mido.Message("note_on",  channel=midi_ch, note=note, velocity=velocity))
                _emit("PASS", f"Sent MIDI note {note} (kick) at vel={velocity}")

                _time_sm.sleep(duration_s - 0.05)
                midi_out.send(_mido.Message("note_off", channel=midi_ch, note=note, velocity=0))

                _sd.wait()

            peak = float(_np.max(_np.abs(recording)))
            peak_db = 20.0 * _math.log10(max(peak, 1e-10))
            _emit("PASS", f"Recorded {duration_s}s audio, peak: {peak_db:.1f} dBFS")

            if peak > 10 ** (-40.0 / 20.0):          # > -40 dBFS
                _emit("PASS", "Signal detected — audio chain is live")
            elif peak < 10 ** (-60.0 / 20.0):         # < -60 dBFS
                _emit("FAIL", "No signal detected — check: XOceanus running? BlackHole routed? MIDI connected?")
            else:
                _emit("WARN", f"Weak signal ({peak_db:.1f} dBFS) — check BlackHole routing or XOceanus volume")

        except Exception as e:
            _emit("FAIL", f"Smoke test error: {e}")

    # ── Print all collected lines ─────────────────────────────────────────────
    for ln in lines:
        print(ln)

    # ── Summary ──────────────────────────────────────────────────────────────
    total = pass_count + warn_count + skip_count + fail_count
    print()
    print("  ─────────────────────────────────────")
    result_str = f"RESULT: {pass_count} PASS, {warn_count} WARN, {skip_count} SKIP, {fail_count} FAIL"
    if fail_count == 0:
        verdict = "Environment is ready for rendering."
    else:
        verdict = "Fix FAIL items before starting a render."
    print(f"  {result_str}")
    print(f"  {verdict}")
    print("  ╚══════════════════════════════════════")
    print()

    return 0 if fail_count == 0 else 1


def cmd_build(args) -> int:
    """Compile a .oxbuild spec into a complete .xpn pack."""
    # Quick pre-build gate: verify Python deps + disk space (no smoke test).
    # Catches the most common first-run failure modes before 12,800 jobs start.
    # Skip with --skip-doctor if you have already run `oxport doctor`.
    if not getattr(args, "skip_doctor", False):
        _quick_doctor_check(args)

    oxbuild_path = Path(args.oxbuild)
    if not oxbuild_path.exists():
        print(f"ERROR: .oxbuild/.oxpack file not found: {oxbuild_path}", file=sys.stderr)
        return 1

    # ── Feature 3: .oxpack YAML manifest support ─────────────────────────────
    # If a .oxpack file is provided instead of .oxbuild, load it as a manifest
    # and synthesize a minimal spec dict that cmd_build can work with.
    # The manifest's preset list becomes the SELECT stage's explicit filter and
    # its tier overrides the .oxbuild default.
    _oxpack_manifest: dict | None = None
    if oxbuild_path.suffix.lower() == ".oxpack":
        try:
            from xpn_pack_manifest import load_manifest as _load_oxpack, apply_manifest_to_build_args
        except ImportError:
            print("ERROR: xpn_pack_manifest.py not found — cannot load .oxpack files",
                  file=sys.stderr)
            return 1
        try:
            _oxpack_manifest = _load_oxpack(oxbuild_path)
        except (FileNotFoundError, ValueError) as _e:
            print(f"ERROR: Invalid .oxpack manifest: {_e}", file=sys.stderr)
            return 1

        # Build a minimal .oxbuild-compatible spec from the manifest.
        # Fields like pack_id, platform, archetype get sensible defaults.
        _pack_slug = re.sub(r"[^a-z0-9-]", "-",
                            _oxpack_manifest["name"].lower().replace(" ", "-"))
        spec: dict = {
            "pack_id":   f"{_oxpack_manifest['engine'].lower()}-{_pack_slug}",
            "engine":    _oxpack_manifest["engine"],
            "platform":  "mpce",
            "archetype": "percussion",
            "pack_name": f"XO_OX {_oxpack_manifest['engine']} — {_oxpack_manifest['name']}",
            "tier":      _oxpack_manifest.get("tier", "DEEP"),
        }
        apply_manifest_to_build_args(_oxpack_manifest, spec)
        print(f"  [OXPACK] Loaded manifest: {oxbuild_path.name}")
        print(f"  [OXPACK] Pack: {spec['pack_name']} | "
              f"{len(_oxpack_manifest['presets'])} presets | Tier: {spec['tier']}")
    else:
        with open(oxbuild_path, encoding='utf-8', errors='replace') as f:
            spec = json.load(f)

    # Validate required fields
    required = ["pack_id", "engine", "platform", "archetype"]
    missing = [k for k in required if k not in spec]
    if missing:
        print(f"ERROR: Missing required fields: {', '.join(missing)}", file=sys.stderr)
        return 1

    pack_id = spec["pack_id"]
    engine = spec["engine"]
    platform = spec.get("platform", "mpce")
    archetype = spec.get("archetype", "percussion")
    pack_name = spec.get("pack_name", f"XO_OX {engine} {archetype.title()}")
    version = spec.get("version", "1.0.0")

    # Determine output directory
    output_dir = Path(args.output_dir) if args.output_dir else REPO_ROOT / "builds" / pack_id
    output_dir.mkdir(parents=True, exist_ok=True)

    # Generate experiment ID
    experiment_id = str(uuid.uuid4()) if spec.get("experiment", {}).get("embed_id", True) else None
    experiment_tags = spec.get("experiment", {}).get("tags", [])

    # Parse skip list
    skip = set()
    if args.skip:
        skip = set(args.skip.split(","))

    # ── Feature #734: --resume support ──────────────────────────────────────
    # If --resume is set, load the prior build log and skip all stages that
    # already completed.  Stage-level skips merge with any --skip list.
    log_path = output_dir / "build.log.json"
    _resume_completed: list[str] = []
    if getattr(args, "resume", False):
        _resume_completed = _load_resume_state(log_path)
        if _resume_completed:
            _last = _resume_completed[-1]
            _last_n = BUILD_STAGES.index(_last) + 1 if _last in BUILD_STAGES else "?"
            print(f"[RESUME] Resuming from Stage {_last_n + 1 if isinstance(_last_n, int) else '?'} "
                  f"({_last.upper()} was last completed). "
                  f"Stages already done: {', '.join(_resume_completed)}")
            # Merge completed stages into the skip set
            skip.update(_resume_completed)
        else:
            print("[RESUME] No prior build.log.json found — starting fresh.")

    # Build timestamp
    build_time = datetime.now().isoformat()

    print(BANNER)
    print(f"  .oxbuild Compiler v1.0")
    print(f"  ─────────────────────────────────")
    print(f"  Pack:         {pack_name}")
    print(f"  Pack ID:      {pack_id}")
    print(f"  Engine:       {engine}")
    print(f"  Platform:     {platform}")
    print(f"  Archetype:    {archetype}")
    print(f"  Version:      {version}")
    if experiment_id:
        print(f"  Experiment:   {experiment_id}")
    if experiment_tags:
        print(f"  Tags:         {', '.join(experiment_tags)}")
    print(f"  Output:       {output_dir}")
    print(f"  Build time:   {build_time}")
    print()

    if args.dry_run:
        print("  DRY RUN — showing compilation plan:")
        print()

    # Rendering config
    rendering = spec.get("rendering", {})
    sample_rate = rendering.get("sample_rate")
    if sample_rate is None:
        sample_rate = 44100
        print(f"  [WARN] No sample_rate in .oxbuild rendering block — defaulting to {sample_rate}Hz")
    bit_depth = rendering.get("bit_depth", 24)
    render_spec_override = rendering.get("render_spec_override")
    # preset_load_ms: how long to wait after MIDI program change before recording.
    # 200ms default is too tight for XOceanus with 50+ presets — use 400ms or set
    # rendering.preset_load_ms in the .oxbuild spec.
    preset_load_ms = rendering.get("preset_load_ms", 400)
    # Average per-voice duration for disk estimate (matches oxport_render defaults).
    render_duration_ms = rendering.get("duration_ms", 3000)
    render_release_ms = rendering.get("release_tail_ms", 500)

    # Corner config
    # Quad-corner velocity workaround removed (QDD 2026-04-04: causes MPC hardware state corruption)
    # "none" or missing value means no corner expansion; "dynamic_expression" is kept in schema
    # for backwards compatibility with older .oxbuild files but is now effectively inert.
    corner_strategy = spec.get("corner_strategy", "none")
    corner_expansion_active = corner_strategy not in ("none", None, "")
    pad_count = spec.get("pad_count", 16)

    # ── Tier resolution ──────────────────────────────────────────────────────
    # Priority: --tier CLI flag > .oxbuild "tier" field > default "DEEP"
    _raw_tier = (getattr(args, "tier", None) or spec.get("tier", "DEEP")).upper()
    try:
        tier_cfg = _get_tier_config(_raw_tier)
    except ValueError as _e:
        print(f"ERROR: {_e}", file=sys.stderr)
        return 1
    tier = _raw_tier

    # Tier-derived rendering parameters (override any legacy .oxbuild values)
    velocity_layers = tier_cfg["vel_layers"]
    velocity_values = tier_cfg["vel_values"]
    round_robin     = tier_cfg["round_robin"]
    max_slots       = tier_cfg["max_slots"]

    # --preset-count overrides preset_selection.min_presets from .oxbuild
    if getattr(args, "preset_count", None) is not None:
        spec.setdefault("preset_selection", {})
        spec["preset_selection"]["min_presets"] = args.preset_count

    # Output config
    output_cfg = spec.get("output", {})
    include_standard = output_cfg.get("include_standard_version", True)
    include_mpce_setup = output_cfg.get("include_mpce_setup", True)
    include_cover_art = output_cfg.get("include_cover_art", True)
    include_intent = output_cfg.get("include_intent", True)
    cover_art_badge = output_cfg.get("cover_art_badge")

    # Intent config
    intent_cfg = spec.get("intent", {})

    # Profile loading
    profile_id = spec.get("profile")
    profile = None
    profile_velocity_curve: str | None = None  # issue #825 — "hot", "gentle", "musical", etc.
    if profile_id:
        profile_path = REPO_ROOT / "profiles" / f"{profile_id}.yaml"
        if profile_path.exists():
            try:
                # Try yaml first, fall back to json-compatible parsing
                import yaml
                with open(profile_path, encoding='utf-8', errors='replace') as pf:
                    profile = yaml.safe_load(pf)
            except ImportError:
                # Fallback: read as text for display, skip structured access
                profile = {"_raw": profile_path.read_text(), "profile_id": profile_id}
            except Exception as e:
                print(f"  [WARN] Profile load failed: {e}")
        else:
            print(f"  [WARN] Profile not found: {profile_path}")

    # Extract and validate profile velocity_curve (issue #825).
    # The curve name overrides per-instrument DNA sculpting when the DNA sculpting
    # stage runs.  Valid names are any key in xpn_adaptive_velocity.VELOCITY_CURVES
    # plus the standard musical curve aliases.
    if profile and isinstance(profile.get("genotype"), dict):
        _raw_vel_curve = profile["genotype"].get("velocity_curve")
        if _raw_vel_curve:
            _KNOWN_PROFILE_CURVES = {"hot", "gentle", "musical", "kick", "snare",
                                     "hihat", "cymbal", "pad", "atmosphere", "lead",
                                     "melodic", "bass", "perc", "fx", "unknown"}
            if _raw_vel_curve in _KNOWN_PROFILE_CURVES:
                profile_velocity_curve = _raw_vel_curve
                print(f"  [Profile] velocity_curve override: {profile_velocity_curve!r}")
            else:
                print(f"  [WARN] Profile velocity_curve {_raw_vel_curve!r} is not a known curve — ignored")

    total_samples = pad_count * 4 * velocity_layers  # pads * corners * vel layers
    stages_run = 0
    stages_skipped = 0
    stages_failed = 0

    # ── Tier summary print ───────────────────────────────────────────────────
    # Compute slot budget and estimated job count using voice taxonomy.
    # These are informational — actual job count is set during RENDER stage.
    try:
        from xpn_voice_taxonomy import ONSET_VOICES, compute_slot_budget, rr_count
        _onset_voices = ONSET_VOICES
        _slot_budget = compute_slot_budget(_onset_voices, velocity_layers) if round_robin else (len(_onset_voices) * velocity_layers)
        _preset_count_hint = spec.get("preset_selection", {}).get("min_presets", pad_count)
        if round_robin:
            _rr_jobs = sum(
                velocity_layers * max(1, rr_count(v))
                for v in _onset_voices
            )
            _est_jobs = _preset_count_hint * _rr_jobs
        else:
            _est_jobs = _preset_count_hint * len(_onset_voices) * velocity_layers
        _rr_str = "enabled" if round_robin else "disabled"
        print(f"[BUILD] Pack: {pack_name} | Tier: {tier} | Vel layers: {velocity_layers} | RR: {_rr_str}")
        print(f"        Slot budget: {_slot_budget}/{max_slots} | Estimated jobs: {_est_jobs}")
        print()
    except ImportError:
        # voice taxonomy not available (non-Onset engine path) — minimal tier line
        _rr_str = "enabled" if round_robin else "disabled"
        print(f"[BUILD] Pack: {pack_name} | Tier: {tier} | Vel layers: {velocity_layers} | RR: {_rr_str}")
        print()

    # ── STAGE 1: PARSE ──
    print(f"  [1/10] PARSE        .oxbuild → BuildManifest")
    if profile:
        pid = profile.get("profile_id", profile_id)
        pname = profile.get("display_name", pid)
        print(f"         Profile: {pname} ({pid})")
        genotype = profile.get("genotype", {})
        if genotype:
            dna = genotype.get("dna_ranges", {})
            if dna:
                centers = {k: v.get("center", "?") for k, v in dna.items() if isinstance(v, dict)}
                print(f"         DNA center: {centers}")
            if profile_velocity_curve:
                print(f"         Velocity curve: {profile_velocity_curve!r}")
    if args.dry_run:
        print(f"         [OK] Parsed {oxbuild_path}")
    stages_run += 1
    _update_build_log(log_path, "parse")

    # ── STAGE 2: SELECT ──
    selected_presets: list[dict] = []  # populated by SELECT, consumed by RENDER
    if "select" not in skip:
        preset_cfg = spec.get("preset_selection", {})
        mode = preset_cfg.get("mode", "auto_dna")
        mood_filter = preset_cfg.get("mood_filter")
        min_presets = preset_cfg.get("min_presets", pad_count * 4)
        dna_target = preset_cfg.get("dna_diversity_target", 0.7)

        print(f"  [2/10] SELECT       {mode} mode, {min_presets}+ presets, DNA diversity {dna_target}")
        if args.dry_run:
            print(f"         → Would scan {PRESETS_DIR} for {engine} presets")
            if mood_filter:
                print(f"         → Mood filter: {', '.join(mood_filter)}")
            stages_run += 1
        else:
            try:
                import xpn_profile_preset_selector as selector

                if profile_id:
                    # Set up QA log for accept/reject tracking
                    qa_log = None
                    if experiment_id and selector._QA_LOG_AVAILABLE:
                        qa_log = selector.QADecisionLog(
                            pack_id=pack_id,
                            profile_id=profile_id,
                        )

                    selected_presets = selector.select_presets(
                        profile_id=profile_id,
                        engine=engine,
                        count=min_presets,
                        mood_filter=mood_filter,
                        qa_log=qa_log,
                        verbose=True,
                    )

                    # ── BANK ORDERING FIX ──────────────────────────────────────
                    # select_presets() returns presets in farthest-point-sampling
                    # (diversity) order.  MIDI program change N must load the Nth
                    # preset in XOceanus's preset browser.  XOceanus loads presets
                    # via JUCE File::findChildFiles (alphabetical path order on
                    # macOS HFS+/APFS).  The selector's scan_presets() also uses
                    # sorted(Path.rglob()), which sorts by Path parts (part-wise
                    # lexicographic, NOT raw string comparison) — matching JUCE.
                    # Re-sort here using the same Path.__lt__ key so program N in
                    # the render exactly matches browser position N.
                    # Authoritative reference: Docs/render_specs/mpce_perc_001_bank_index.json
                    selected_presets = sorted(
                        selected_presets,
                        key=lambda p: Path(p.get("path", "")),
                    )
                    print(
                        f"         [OK] Bank order aligned to XOceanus browser "
                        f"(Path-part sort, matches selector + JUCE findChildFiles)"
                    )

                    # Save selection manifest
                    manifest_path = output_dir / "selected_presets.json"
                    with open(manifest_path, "w", encoding='utf-8') as f:
                        json.dump({
                            "profile": profile_id,
                            "engine": engine,
                            "count_requested": min_presets,
                            "count_selected": len(selected_presets),
                            "dna_diversity_target": dna_target,
                            "mood_filter": mood_filter,
                            "presets": selected_presets,
                        }, f, indent=2)

                    # Generate preliminary pack_dna.json (pack_average from selected presets)
                    if selected_presets:
                        dna_dims = ("brightness", "warmth", "movement", "density", "space", "aggression")
                        avg_dna = {}
                        for dim in dna_dims:
                            vals = [p["dna"][dim] for p in selected_presets if dim in p.get("dna", {})]
                            avg_dna[dim] = round(sum(vals) / len(vals), 4) if vals else 0.0

                        pack_dna_data = {
                            "pack_average": avg_dna,
                            "voices": {},
                            "render_durations_ms": {},
                            "velocity_peaks_dbfs": {},
                            "choke_groups": {},
                        }
                        dna_path = output_dir / "pack_dna.json"
                        with open(dna_path, "w", encoding='utf-8') as f:
                            json.dump(pack_dna_data, f, indent=2)
                        print(f"         [OK] pack_dna.json written (pack average from {len(selected_presets)} presets)")

                    # Save QA log if populated
                    if qa_log and len(qa_log):
                        qa_log.save()

                    print(f"         [OK] Selected {len(selected_presets)} presets (target: {min_presets})")
                    print(f"         [OK] Manifest: {manifest_path}")

                    if len(selected_presets) < min_presets:
                        print(f"         [WARN] Only {len(selected_presets)} presets matched — "
                              f"below target of {min_presets}")
                else:
                    print(f"         [WARN] No profile specified — skipping DNA-based selection")
            except ImportError:
                print(f"         [SKIP] xpn_profile_preset_selector not available")
                stages_skipped += 1
            except Exception as e:
                print(f"         [FAIL] Selection failed: {e}")
                stages_failed += 1
                if not args.continue_on_error:
                    return 1
            else:
                stages_run += 1
                _update_build_log(log_path, "select")
    else:
        print(f"  [2/10] SELECT       SKIPPED")
        stages_skipped += 1

    # ── STAGE 0: VOICE PROBE ──
    # QDD D2 (unanimous Ghost Council decision): two-pass probe before any rendering.
    # Prevents committing 12,800+ jobs when no audio is flowing from the engine.
    # voice_map is passed into the RENDER stage to skip known-dead voices.
    voice_map: dict | None = None
    renders_dir_for_probe = output_dir / "renders"
    voice_map_path = renders_dir_for_probe / "voice_map.json"

    if "render" not in skip and not args.dry_run and not args.no_render and not args.skip_probe:
        print(f"\n{'─'*60}")
        print(f"  STAGE 0: VOICE PROBE")
        print(f"{'─'*60}")

        # If voice_map.json already exists from a prior probe run, load it rather
        # than running the full probe again (saves significant time on resume).
        if voice_map_path.exists():
            print(f"  [PROBE] Found existing voice_map.json — loading (use --skip-probe to bypass probe)")
            try:
                import oxport_render as _or_probe
                voice_map = _or_probe.load_voice_map(str(renders_dir_for_probe))
                if voice_map is not None:
                    _vm_presets = len(voice_map)
                    _vm_alive = sum(
                        sum(1 for v in pv.values() if v)
                        for pv in voice_map.values()
                    )
                    _vm_total = sum(len(pv) for pv in voice_map.values())
                    print(f"  [PROBE] Loaded voice_map: {_vm_presets} presets, "
                          f"{_vm_alive}/{_vm_total} voices alive")
                else:
                    print(f"  [PROBE] voice_map.json unreadable — will run fresh probe")
            except Exception as _e:
                print(f"  [PROBE] Failed to load voice_map.json: {_e} — will run fresh probe")
                voice_map = None

        if voice_map is None and render_spec_override:
            # Build engine voice list from render spec for the probe
            try:
                import oxport_render as _or_probe
                import mido as _mido_probe
                import sounddevice as _sd_probe

                _probe_spec_path = REPO_ROOT / render_spec_override
                if _probe_spec_path.exists():
                    _probe_spec = _or_probe.load_spec(str(_probe_spec_path))
                    _probe_layout = _or_probe.resolve_presets(_probe_spec)

                    # Extract unique (voice_name, note) pairs from the note layout.
                    # Each entry in note_layout has a "slug"/"name" (voice) and "notes".
                    _seen_voices: set[str] = set()
                    _engine_voices: list[dict] = []
                    for entry in _probe_layout:
                        _vname = entry.get("slug", entry.get("name", ""))
                        _vnotes = entry.get("notes", [entry.get("note")])
                        _vnote = _vnotes[0] if _vnotes else 60
                        if _vname and _vname not in _seen_voices:
                            _seen_voices.add(_vname)
                            _engine_voices.append({"name": _vname, "note": _vnote})

                    # Build probe preset list from selected_presets (or a stub if SELECT was skipped)
                    if selected_presets:
                        _probe_presets = [
                            {"name": p["name"], "index": i,
                             "engine": engine,
                             "channel": p.get("channel", 0),
                             "bank_msb": p.get("bank_msb"),
                             "bank_lsb": p.get("bank_lsb")}
                            for i, p in enumerate(selected_presets)
                        ]
                    else:
                        # No SELECT data — probe with a single stub preset at program 0
                        _probe_presets = [{"name": f"{engine}_default", "index": 0,
                                           "engine": engine, "channel": 0,
                                           "bank_msb": None, "bank_lsb": None}]

                    if _engine_voices and _probe_presets:
                        # Open MIDI port and resolve audio device for the probe
                        _midi_ports = _mido_probe.get_output_names()
                        _probe_midi_port_name = args.midi_port or (_midi_ports[0] if _midi_ports else None)
                        if _probe_midi_port_name is None:
                            print(f"  [PROBE SKIP] No MIDI port available — skipping probe")
                        else:
                            _probe_port = _mido_probe.open_output(_probe_midi_port_name)
                            _probe_audio_device = None
                            if args.audio_device is not None:
                                _ad = args.audio_device
                                if str(_ad).isdigit():
                                    _probe_audio_device = int(_ad)
                                else:
                                    _all_devs = _sd_probe.query_devices()
                                    for _di, _dd in enumerate(_all_devs):
                                        if _ad.lower() in _dd["name"].lower() and _dd["max_input_channels"] > 0:
                                            _probe_audio_device = _di
                                            break

                            renders_dir_for_probe.mkdir(parents=True, exist_ok=True)
                            _or_probe._require_sounddevice()  # ensure sd/numpy loaded
                            voice_map = _or_probe.probe_voices(
                                voices=_engine_voices,
                                presets=_probe_presets,
                                midi_port=_probe_port,
                                audio_device=_probe_audio_device,
                                sample_rate=sample_rate,
                                output_dir=str(renders_dir_for_probe),
                                preset_load_ms=preset_load_ms,
                            )
                            _probe_port.close()

                            if not voice_map:
                                print(f"  [ABORT] Probe failed — no voice map generated. "
                                      f"Check BlackHole routing and XOceanus engine state.")
                                return 1
                    else:
                        print(f"  [PROBE SKIP] No voices or presets to probe — skipping Stage 0")

                else:
                    print(f"  [PROBE SKIP] Render spec not found at {_probe_spec_path} — skipping Stage 0")

            except ImportError as _e:
                print(f"  [PROBE SKIP] Missing dependency for probe: {_e}")
            except Exception as _e:
                print(f"  [PROBE WARN] Probe failed with error: {_e}")
                print(f"  [PROBE WARN] Continuing without voice map — all voices will be rendered")
                voice_map = None
        elif voice_map is None and not render_spec_override:
            print(f"  [PROBE SKIP] No render_spec_override in .oxbuild — cannot build voice list for probe")

    elif args.skip_probe:
        # --skip-probe: try to load existing voice_map.json silently
        if voice_map_path.exists():
            try:
                import oxport_render as _or_vm
                voice_map = _or_vm.load_voice_map(str(renders_dir_for_probe))
                if voice_map:
                    print(f"  [STAGE 0] --skip-probe: loaded existing voice_map.json "
                          f"({len(voice_map)} presets)")
            except Exception:
                pass

    # ── STAGE 3: RENDER ──
    if "render" not in skip:
        print(f"  [3/10] RENDER       {total_samples} WAVs ({pad_count} pads × 4 corners × {velocity_layers} vel)")
        if render_spec_override:
            print(f"         → Using render spec: {render_spec_override}")
        else:
            print(f"         → Auto-generating render spec from .oxbuild")
        print(f"         → {sample_rate}Hz / {bit_depth}-bit / via BlackHole loopback")

        # SELECT → RENDER wiring: inject program numbers from selected_presets.
        # When SELECT produced a preset list, each preset is rendered as a separate program:
        # program number = 0-based index of that preset in the XOceanus bank as loaded.
        # The render spec defines the note-layout (which MIDI note → which drum voice).
        # We clone that layout once per selected preset, patching in the program number
        # so oxport_render sends the correct MIDI PC before recording each preset's voices.
        if selected_presets:
            print(f"         → SELECT wired: {len(selected_presets)} preset(s) → program 0..{len(selected_presets)-1}")
        else:
            print(f"         → SELECT not run (or skipped) — will render with spec program numbers as-is")

        if args.dry_run:
            if selected_presets:
                total_jobs = len(selected_presets) * pad_count * 4 * velocity_layers
                print(f"         → Would render {len(selected_presets)} presets × {pad_count} pads × 4 corners "
                      f"× {velocity_layers} vel = {total_jobs} WAVs total")
                print(f"         → Would call: oxport_render.py --spec <spec> --output-dir {output_dir / 'renders'}")
            else:
                total_jobs = pad_count * 4 * velocity_layers
                print(f"         → Would call: oxport_render.py --spec <spec> --output-dir {output_dir / 'renders'}")
            # Disk space estimate for final Samples/ output.
            # renders/ is cleaned up after ASSEMBLE, so only one copy is retained.
            # Formula: jobs × (duration + release) × sample_rate × channels × bytes_per_sample
            _bytes_per_sample = (bit_depth + 7) // 8  # rounds up; 24-bit → 3
            _channels = 2  # oxport_render always records stereo
            _avg_wav_bytes = (
                (render_duration_ms + render_release_ms) / 1000.0
                * sample_rate * _channels * _bytes_per_sample
            )
            _total_bytes = total_jobs * _avg_wav_bytes
            if _total_bytes >= 1_073_741_824:
                _disk_str = f"{_total_bytes / 1_073_741_824:.1f} GB"
            else:
                _disk_str = f"{_total_bytes / 1_048_576:.0f} MB"
            print(f"         → Estimated final output size: {_disk_str} "
                  f"({total_jobs} WAVs × "
                  f"{(render_duration_ms + render_release_ms) / 1000.0:.1f}s × "
                  f"{sample_rate}Hz × stereo × {_bytes_per_sample * 8}-bit)")
            stages_run += 1
        else:
            renders_dir = output_dir / "renders"
            renders_dir.mkdir(exist_ok=True)
            if render_spec_override:
                spec_path = REPO_ROOT / render_spec_override
            else:
                spec_path = output_dir / f"{pack_id}_render_spec.json"
                # Auto-generate render spec would go here
                print(f"         [WARN] Auto render spec generation not yet implemented")
                print(f"            Use rendering.render_spec_override in .oxbuild to point to a spec")

            if spec_path.exists():
                print(f"         → Launching render: {spec_path}")
                # Import and run the render engine
                try:
                    import oxport_render
                    render_spec_data = oxport_render.load_spec(str(spec_path))
                    note_layout = oxport_render.resolve_presets(render_spec_data)

                    # A2: Validate that render spec sample_rate matches .oxbuild rendering.sample_rate.
                    spec_sr = render_spec_data.get("rendering", {}).get("sample_rate")
                    if spec_sr is not None and spec_sr != sample_rate:
                        print(f"         [ERROR] Sample rate mismatch: .oxbuild says {sample_rate}Hz "
                              f"but render spec says {spec_sr}Hz. "
                              f"Update rendering.sample_rate in one of the two files to match.")
                        return 1

                    if selected_presets:
                        # SELECT → RENDER: expand note-layout × selected presets.
                        # Each selected preset gets program number = its 0-based index.
                        # Slug prefix = preset name (slugified) so WAVs are organized per preset.
                        all_jobs: list[dict] = []
                        for prog_num, sel_preset in enumerate(selected_presets):
                            preset_slug_prefix = re.sub(
                                r'[^a-zA-Z0-9_.-]', '_',
                                sel_preset["name"].replace(" ", "_")
                            ).strip('.')
                            # Clone note-layout entries with the correct program number injected
                            patched_layout = []
                            for entry in note_layout:
                                patched = dict(entry)
                                patched["program"] = prog_num
                                # Preserve the original voice slug for voice_map filtering
                                _orig_voice_slug = entry.get("slug", entry.get("name", "voice"))
                                patched["_voice_name"] = _orig_voice_slug  # probe filter key
                                # Prefix slug so WAV filenames are unique across presets
                                patched["slug"] = f"{preset_slug_prefix}__{_orig_voice_slug}"
                                patched_layout.append(patched)
                            preset_jobs = oxport_render.build_render_jobs(
                                render_spec_data, patched_layout,
                                tier=tier,
                                vel_values=velocity_values,
                                round_robin=round_robin,
                            )
                            # Annotate each job with preset_name + voice_name for
                            # Stage 0 voice_map filtering in _render_jobs_locked.
                            _preset_name_for_jobs = sel_preset["name"]
                            for _pj in preset_jobs:
                                _pj["preset_name"] = _preset_name_for_jobs
                                # voice_name: strip the preset prefix from the slug
                                _full_slug = _pj.get("slug", "")
                                _sep = f"{preset_slug_prefix}__"
                                if _full_slug.startswith(_sep):
                                    _pj["voice_name"] = _full_slug[len(_sep):]
                                else:
                                    _pj["voice_name"] = _full_slug
                            all_jobs.extend(preset_jobs)

                        # Save the wired render manifest for auditability
                        wired_manifest_path = output_dir / "render_manifest.json"
                        with open(wired_manifest_path, "w", encoding='utf-8') as _wm:
                            json.dump({
                                "source_spec": str(spec_path),
                                "selected_presets_count": len(selected_presets),
                                "total_jobs": len(all_jobs),
                                "tier": tier,
                                "round_robin": round_robin,
                                "vel_values": velocity_values,
                                "presets": [
                                    {"name": p["name"], "program": i, "path": p.get("path", "")}
                                    for i, p in enumerate(selected_presets)
                                ],
                            }, _wm, indent=2)
                        print(f"         [OK] Render manifest: {wired_manifest_path}")
                        print(f"         → {len(all_jobs)} total render jobs "
                              f"({len(selected_presets)} presets × {len(note_layout)} voices × {velocity_layers} vel"
                              + (" × RR" if round_robin else "") + ")")
                        jobs = all_jobs
                    else:
                        # No SELECT data — render spec as-is (program numbers from the spec file)
                        jobs = oxport_render.build_render_jobs(
                            render_spec_data, note_layout,
                            tier=tier,
                            vel_values=velocity_values,
                            round_robin=round_robin,
                        )
                        print(f"         → {len(jobs)} render jobs queued (no SELECT override)")

                    if not args.no_render:
                        # Pre-render disk space check — abort early rather than
                        # filling the disk during a multi-hour render session.
                        _est_bytes = _estimate_disk_usage(
                            len(jobs), render_duration_ms, render_release_ms,
                            sample_rate, bit_depth
                        )
                        renders_dir.mkdir(exist_ok=True)
                        if not _check_disk_space(renders_dir, _est_bytes):
                            return 1
                        oxport_render.render_jobs(
                            jobs, str(renders_dir),
                            midi_port_name=args.midi_port,
                            audio_device=args.audio_device,
                            sample_rate=sample_rate,
                            preset_load_ms=preset_load_ms,
                            voice_map=voice_map,
                        )
                    else:
                        print(f"         → --no-render flag: skipping actual audio capture")
                    stages_run += 1
                    _update_build_log(log_path, "render")
                except ImportError:
                    print(f"         [SKIP] oxport_render not available (pip install mido sounddevice numpy)")
                    stages_skipped += 1
                except Exception as e:
                    print(f"         [FAIL] Render failed: {e}")
                    stages_failed += 1
                    if not args.continue_on_error:
                        return 1
            else:
                stages_skipped += 1
    else:
        print(f"  [3/10] RENDER       SKIPPED")
        stages_skipped += 1

    # ── STAGE 4: ASSEMBLE ──
    assembled_xpm_paths: list[Path] = []  # populated here, consumed by PACKAGE
    if "assemble" not in skip:
        print(f"  [4/10] ASSEMBLE     MPCe XPM (corner_strategy={corner_strategy}, expansion={'on' if corner_expansion_active else 'off'})")
        if args.dry_run:
            src = f"{len(selected_presets)} selected presets" if selected_presets else f"all {engine} presets"
            print(f"         → Would call: xpn_mpce_quad_builder.py --engine {engine} "
                  f"--pad-count {pad_count} (source: {src})")
            stages_run += 1
        else:
            try:
                import xpn_mpce_quad_builder as qb
                import shutil as _shutil

                # When SELECT ran, build PresetInfo from selected_presets
                # dicts rather than re-scanning the entire presets tree. This guarantees
                # the XPM only references presets that actually got rendered.
                if selected_presets:
                    presets = []
                    for sp in selected_presets:
                        dna = sp.get("dna", {})
                        macros = sp.get("macros", {})
                        presets.append(qb.PresetInfo(
                            name=sp.get("name", ""),
                            filepath=sp.get("path", ""),
                            engine=engine,
                            brightness=float(dna.get("brightness", 0.5)),
                            warmth=float(dna.get("warmth", 0.5)),
                            movement=float(dna.get("movement", 0.5)),
                            density=float(dna.get("density", 0.5)),
                            space=float(dna.get("space", 0.5)),
                            aggression=float(dna.get("aggression", 0.5)),
                            character=float(macros.get("character", 0.5)),
                            movement_macro=float(macros.get("movement", 0.5)),
                            coupling=float(macros.get("coupling", 0.5)),
                            space_macro=float(macros.get("space", 0.5)),
                            mood=sp.get("mood", ""),
                            tags=sp.get("tags", []),
                        ))
                    print(f"         → SELECT wired: using {len(presets)} preset(s) from SELECT stage")
                else:
                    # Fallback: no SELECT ran — scan the full presets tree.
                    presets = qb.load_presets(str(PRESETS_DIR), engine)

                if not presets:
                    print(f"         [WARN] No presets found for engine '{engine}'")
                else:
                    assignments = qb.assign_corners(presets, pad_count)
                    programs_dir = output_dir / "Programs"
                    programs_dir.mkdir(exist_ok=True)
                    xpm_path = qb.generate_xpm(engine, assignments, str(programs_dir))
                    assembled_xpm_paths.append(Path(xpm_path))
                    print(f"         [OK] {len(assignments)} pad(s) assigned "
                          f"({len(presets)} presets)")
                    print(f"         [OK] Written: {xpm_path}")

                    # Stage WAVs from renders/ into Samples/.
                    # For multi-preset packs (SELECT wired), WAVs are named
                    # {preset_name}__{slug}__{note}__{vel}.WAV — group them per
                    # preset name into Samples/{program_slug}/{safe_preset_name}/
                    # so the packager can include them without collision.
                    renders_dir = output_dir / "renders"
                    if renders_dir.exists():
                        program_slug = Path(xpm_path).stem
                        samples_root = output_dir / "Samples" / program_slug
                        wav_count = 0
                        copy_ok = 0
                        if selected_presets:
                            # Multi-preset: each preset has its own sample subdir.
                            # WAV names: {preset_name}__{rest}.WAV
                            for wav in sorted(renders_dir.glob("*.WAV")) + \
                                       sorted(renders_dir.glob("*.wav")):
                                parts = wav.stem.split("__", maxsplit=1)
                                if len(parts) == 2:
                                    # Sanitize slug extracted from WAV filename (#563 — path traversal)
                                    preset_slug = re.sub(
                                        r'[^a-zA-Z0-9_.-]', '_', parts[0]
                                    ).strip('.')
                                    dest_dir = samples_root / preset_slug
                                else:
                                    dest_dir = samples_root
                                # Path containment check before any file write
                                if not dest_dir.resolve().is_relative_to(samples_root.resolve()):
                                    print(f'[WARN] Path escape blocked: {dest_dir}', file=sys.stderr)
                                    continue
                                dest_dir.mkdir(parents=True, exist_ok=True)
                                dest_wav = dest_dir / wav.name
                                if not dest_wav.exists():
                                    _shutil.copy2(wav, dest_wav)
                                wav_count += 1
                                copy_ok += 1
                            print(f"         [OK] {wav_count} WAV(s) staged → "
                                  f"Samples/{program_slug}/{{preset}}/")
                        else:
                            # Single-preset: flat staging.
                            samples_root.mkdir(parents=True, exist_ok=True)
                            for wav in sorted(renders_dir.glob("*.WAV")) + \
                                       sorted(renders_dir.glob("*.wav")):
                                dest_wav = samples_root / wav.name
                                if not dest_wav.exists():
                                    _shutil.copy2(wav, dest_wav)
                                wav_count += 1
                                copy_ok += 1
                            print(f"         [OK] {wav_count} WAV(s) staged → "
                                  f"Samples/{program_slug}/")

                        # Clean up renders/ only when every file was copied
                        # successfully — avoids data loss on partial failures.
                        if copy_ok == wav_count and wav_count > 0:
                            try:
                                _shutil.rmtree(renders_dir)
                                print(f"         [OK] Cleaned up {renders_dir} "
                                      f"({wav_count} intermediate file(s) removed)")
                            except Exception as _cleanup_err:
                                print(f"         [WARN] renders/ cleanup failed (non-fatal): "
                                      f"{_cleanup_err}")

            except ImportError:
                print(f"         [SKIP] xpn_mpce_quad_builder not available")
                stages_skipped += 1
            except Exception as e:
                print(f"         [FAIL] Assemble failed: {e}")
                stages_failed += 1
                if not args.continue_on_error:
                    return 1
            else:
                stages_run += 1
                _update_build_log(log_path, "assemble")
    else:
        print(f"  [4/10] ASSEMBLE     SKIPPED")
        stages_skipped += 1

    # ── STAGE 5: FALLBACK ──
    # NOT YET IMPLEMENTED — issue #819.
    # This stage is intended to produce a standard (non-MPCe) velocity-layered XPM
    # by calling xpn_drum_export.py --mode smart. The integration is not yet written.
    # It is skipped in all modes (including dry_run) so the build log stays accurate.
    if "fallback" not in skip and include_standard:
        print(f"  [5/10] FALLBACK     [NOT IMPLEMENTED] Standard XPM fallback is not yet implemented")
        print(f"         Skipping — set skip: [fallback] in .oxbuild to suppress this warning")
        stages_skipped += 1
    else:
        print(f"  [5/10] FALLBACK     SKIPPED")
        stages_skipped += 1

    # ── STAGE 6: INTENT ──
    if "intent" not in skip and include_intent:
        print(f"  [6/10] INTENT       xpn_intent.json sidecar")
        if args.dry_run:
            print(f"         → Would call: xpn_intent_generator.py --corner-pattern {corner_strategy}")
            stages_run += 1
        else:
            try:
                import xpn_intent_generator as ig
                intent_data = ig.generate_intent(
                    pack_name=pack_name,
                    engine=engine,
                    corner_pattern=corner_strategy,
                    summary=intent_cfg.get("summary", ""),
                    target_player=intent_cfg.get("target_player", ""),
                    target_genre=intent_cfg.get("target_genre"),
                    presets_dir=str(PRESETS_DIR),
                    mpce_native=platform in ("mpce", "both"),
                    pad_count=pad_count,
                )
                # Embed experiment ID
                if experiment_id:
                    intent_data.setdefault("provenance", {})["experiment_id"] = experiment_id
                    intent_data["provenance"]["experiment_tags"] = experiment_tags
                    intent_data["provenance"]["build_time"] = build_time

                intent_path = output_dir / "xpn_intent.json"
                with open(intent_path, "w", encoding='utf-8') as f:
                    json.dump(intent_data, f, indent=2)
                print(f"         [OK] Written: {intent_path}")
                stages_run += 1
                _update_build_log(log_path, "intent")
            except ImportError:
                print(f"         [SKIP] xpn_intent_generator not available")
                stages_skipped += 1
            except Exception as e:
                print(f"         [FAIL] Intent generation failed: {e}")
                stages_failed += 1
                if not args.continue_on_error:
                    return 1
    else:
        print(f"  [6/10] INTENT       SKIPPED")
        stages_skipped += 1

    # ── STAGE 7: DOCS ──
    if "docs" not in skip and include_mpce_setup:
        print(f"  [7/10] DOCS         MPCE_SETUP.md")
        if args.dry_run:
            print(f"         → Would copy template from Docs/templates/MPCE_SETUP.md")
        else:
            template_path = REPO_ROOT / "Docs" / "templates" / "MPCE_SETUP.md"
            docs_dir = output_dir / "Docs"
            docs_dir.mkdir(exist_ok=True)
            if template_path.exists():
                import shutil
                dest = docs_dir / "MPCE_SETUP.md"
                shutil.copy2(template_path, dest)
                # Embed experiment ID as footer comment
                if experiment_id:
                    with open(dest, "a", encoding='utf-8') as f:
                        f.write(f"\n\n<!-- experiment_id: {experiment_id} -->\n")
                        f.write(f"<!-- build_time: {build_time} -->\n")
                print(f"         [OK] Written: {dest}")
            else:
                print(f"         [WARN] Template not found: {template_path}")
        stages_run += 1
        _update_build_log(log_path, "docs")
    else:
        print(f"  [7/10] DOCS         SKIPPED")
        stages_skipped += 1

    # ── STAGE 8: ART ──
    if "art" not in skip and include_cover_art:
        print(f"  [8/10] ART          Cover art{f' [{cover_art_badge}]' if cover_art_badge else ''}")
        if args.dry_run:
            print(f"         → Would call: xpn_cover_art_generator_v2.py --engine {engine}")
            stages_run += 1
        else:
            try:
                import xpn_cover_art_generator_v2 as art_gen

                # Build a best-effort sonic DNA from pack_dna.json if available,
                # otherwise use neutral defaults.
                sonic_dna: dict = {
                    "brightness": 0.5, "warmth": 0.5, "movement": 0.5,
                    "density": 0.5, "space": 0.5, "aggression": 0.5,
                }
                pack_dna_path = output_dir / "pack_dna.json"
                if pack_dna_path.exists():
                    try:
                        with open(pack_dna_path, encoding='utf-8', errors='replace') as _f:
                            _pd = json.load(_f)
                        _avg = _pd.get("pack_average", {})
                        sonic_dna.update({k: v for k, v in _avg.items()
                                         if k in sonic_dna})
                    except Exception as e:
                        print(f'[WARN] pack_dna.json parse error: {e}', file=sys.stderr)

                png_bytes = art_gen.generate_cover_art(
                    engine=engine,
                    pack_name=pack_name,
                    sonic_dna=sonic_dna,
                    size=400,
                )
                art_path = output_dir / "artwork.png"
                art_path.write_bytes(png_bytes)
                print(f"         [OK] artwork.png written ({len(png_bytes):,} bytes)")

                # Also write 2000×2000 hi-res variant
                try:
                    png_2000 = art_gen.generate_cover_art(
                        engine=engine,
                        pack_name=pack_name,
                        sonic_dna=sonic_dna,
                        size=2000,
                    )
                    art_2000_path = output_dir / "artwork_2000.png"
                    art_2000_path.write_bytes(png_2000)
                    print(f"         [OK] artwork_2000.png written ({len(png_2000):,} bytes)")
                except Exception:
                    pass  # hi-res is optional

                stages_run += 1
                _update_build_log(log_path, "art")
            except ImportError:
                print(f"         [SKIP] xpn_cover_art_generator_v2 not available")
                stages_skipped += 1
            except Exception as e:
                print(f"         [FAIL] Art generation failed: {e}")
                stages_failed += 1
    else:
        print(f"  [8/10] ART          SKIPPED")
        stages_skipped += 1

    # ── STAGE 9: LISTEN ──
    if "listen" not in skip:
        renders_dir_listen = output_dir / "renders"
        listen_wavs: list[Path] = []
        if renders_dir_listen.exists():
            listen_wavs = (sorted(renders_dir_listen.glob("*.WAV")) +
                           sorted(renders_dir_listen.glob("*.wav")))

        print(f"  [9/11] LISTEN       Preview gate ({len(listen_wavs)} rendered WAV(s))")

        if args.dry_run:
            print(f"         -> Would generate: {output_dir}/_preview_playlist.wav")
            print(f"         -> Would pause for human listening approval")
            stages_run += 1
        elif getattr(args, "skip_listen", False):
            print(f"         [SKIP] Listen gate bypassed (--skip-listen)")
            stages_run += 1
            _update_build_log(log_path, "listen")
        else:
            # Generate preview playlist WAV (first 2s of each sample, 0.5s silence gaps,
            # capped at 60 seconds total / ~20 samples).
            playlist_path = output_dir / "_preview_playlist.wav"
            playlist_ok = False
            if listen_wavs:
                try:
                    MAX_PLAYLIST_SECONDS = 60.0
                    SNIPPET_SECONDS = 2.0
                    GAP_SECONDS = 0.5
                    MAX_SAMPLES = 20

                    # Read the first WAV to establish target format
                    _ref_channels, _ref_sr, _ref_bits, _ = _read_wav_raw(listen_wavs[0])
                    out_channels = _ref_channels
                    out_sr = _ref_sr
                    out_bits = _ref_bits

                    playlist_chunks: list[bytes] = []
                    total_seconds = 0.0
                    n_included = 0
                    bytes_per_sample_pl = out_bits // 8
                    samples_per_snippet = int(out_sr * SNIPPET_SECONDS)
                    bytes_per_snippet = samples_per_snippet * out_channels * bytes_per_sample_pl
                    silence_gap = _make_silence_bytes(out_sr, out_channels, out_bits, GAP_SECONDS)

                    for wav_path in listen_wavs[:MAX_SAMPLES]:
                        if total_seconds >= MAX_PLAYLIST_SECONDS:
                            break
                        try:
                            nc, sr, bits, raw = _read_wav_raw(wav_path)
                            # Skip format mismatches rather than resample
                            if nc != out_channels or sr != out_sr or bits != out_bits:
                                print(f"         [WARN] Format mismatch in {wav_path.name}"
                                      f" ({nc}ch/{sr}Hz/{bits}bit vs"
                                      f" {out_channels}ch/{out_sr}Hz/{out_bits}bit) -- skipping")
                                continue
                            snippet = raw[:bytes_per_snippet]
                            playlist_chunks.append(snippet)
                            playlist_chunks.append(silence_gap)
                            total_seconds += SNIPPET_SECONDS + GAP_SECONDS
                            n_included += 1
                        except Exception as _wav_err:
                            print(f"         [WARN] Could not read {wav_path.name}: {_wav_err}")

                    if playlist_chunks:
                        _write_wav(playlist_path, out_channels, out_sr, out_bits,
                                   b"".join(playlist_chunks))
                        size_kb = playlist_path.stat().st_size // 1024
                        print(f"         Preview playlist: {playlist_path}")
                        print(f"           {n_included} snippet(s), ~{total_seconds:.0f}s, {size_kb} KB")
                        playlist_ok = True
                    else:
                        print(f"         [WARN] No readable WAVs -- playlist not generated")

                except Exception as _pl_err:
                    print(f"         [WARN] Playlist generation failed: {_pl_err}")
            else:
                print(f"         [WARN] No rendered WAVs found -- playlist not generated")

            # Count presets from selected_presets.json if available
            _sp_path = output_dir / "selected_presets.json"
            _n_presets_display = 0
            try:
                if _sp_path.exists():
                    with open(_sp_path, encoding="utf-8", errors="replace") as _spf:
                        _sp_data = json.load(_spf)
                    _n_presets_display = _sp_data.get(
                        "count_selected", len(_sp_data.get("presets", [])))
            except Exception:
                pass

            # Check for existing approval marker (supports --resume runs)
            _approval_marker = output_dir / ".listen_approved"
            if _approval_marker.exists():
                try:
                    _approval_ts = _approval_marker.read_text(encoding="utf-8").strip()
                except Exception:
                    _approval_ts = "(unknown time)"
                print(f"         [APPROVED] Listen approval found ({_approval_ts})")
                stages_run += 1
                _update_build_log(log_path, "listen")
            else:
                # Gate: print the action-required banner and stop the build.
                print()
                print(f"  -- STAGE: LISTEN -----------------------------------------------")
                if playlist_ok:
                    print(f"    Preview playlist: {playlist_path}")
                print(f"    {len(listen_wavs)} samples rendered across {_n_presets_display} preset(s).")
                print()
                print(f"    [ACTION REQUIRED] Listen to the preview before packaging.")
                print(f"    Run `oxport approve {output_dir}` to continue to PACKAGE.")
                print(f"    Or use --skip-listen to bypass this gate.")
                print()
                return 0
    else:
        print(f"  [9/11] LISTEN       SKIPPED")
        stages_skipped += 1

        # ── STAGE 10: PACKAGE ──
    if "package" not in skip:
        # Gate: require listen approval unless --skip-listen is set
        _listen_marker = output_dir / ".listen_approved"
        if not getattr(args, "skip_listen", False) and not _listen_marker.exists():
            print(f"  [10/11] PACKAGE     BLOCKED -- listen approval required")
            print(f"         Run `oxport approve {output_dir}` then `oxport build --resume`")
            print(f"         or pass --skip-listen to bypass this gate.")
            return 1

        # #818: Guard — abort if render produced zero WAV files so we never
        # package a broken/empty .xpn.  Check renders/ (primary render output)
        # and Samples/ (post-assemble staging location).
        if not args.dry_run:
            _renders_dir = output_dir / "renders"
            _samples_dir = output_dir / "Samples"
            _wav_count = 0
            if _renders_dir.exists():
                _wav_count += len(list(_renders_dir.glob("*.wav"))) + \
                              len(list(_renders_dir.glob("*.WAV")))
            if _samples_dir.exists():
                _wav_count += len(list(_samples_dir.rglob("*.wav"))) + \
                              len(list(_samples_dir.rglob("*.WAV")))
            if _wav_count == 0 and "render" not in skip and "assemble" not in skip:
                print(f"  [10/11] PACKAGE     ABORTED — zero WAV files found")
                print(f"         Render produced no audio output. Re-run the RENDER stage "
                      f"before attempting to package.")
                print(f"         Use --skip-listen --skip render,assemble to package "
                      f"from an existing Samples/ directory.")
                stages_failed += 1
                return 1

        xpn_name = f"XO_OX_{pack_id.replace('-', '_')}_v{version}.xpn"
        print(f"  [10/11] PACKAGE     {xpn_name}")
        if args.dry_run:
            print(f"         → Would call: xpn_packager.py --output {output_dir / xpn_name}")
            stages_run += 1
        else:
            try:
                from xpn_packager import package_xpn, XPNMetadata

                # Verify we have at least one .xpm to package.
                existing_xpms = list((output_dir / "Programs").glob("*.xpm")) \
                    if (output_dir / "Programs").exists() else []
                existing_xpms += list(output_dir.glob("*.xpm"))
                # Also count assembled paths from STAGE 4 that weren't moved yet
                all_xpms = existing_xpms + [p for p in assembled_xpm_paths
                                            if p not in existing_xpms]

                if not all_xpms:
                    print(f"         [WARN] No .xpm programs found — skipping package")
                else:
                    xpn_path_out = output_dir / xpn_name
                    meta = XPNMetadata(
                        name=pack_name,
                        version=version,
                        author=spec.get("author", "XO_OX Designs"),
                        description=(
                            f"{pack_name} — {archetype.title()} pack for "
                            f"{platform.upper()} — XO_OX Designs"
                        ),
                        pack_id=pack_id,
                        cover_engine=engine.upper(),
                    )
                    result_path = package_xpn(
                        build_dir=output_dir,
                        output_path=xpn_path_out,
                        metadata=meta,
                        include_artwork=include_cover_art,
                        verbose=True,
                    )
                    size_mb = result_path.stat().st_size / (1024 * 1024) \
                        if result_path.exists() else 0
                    print(f"         [OK] {result_path.name} ({size_mb:.2f} MB)")
                stages_run += 1
                _update_build_log(log_path, "package")
            except ImportError:
                print(f"         [SKIP] xpn_packager not available")
                stages_skipped += 1
            except Exception as e:
                print(f"         [FAIL] Package failed: {e}")
                stages_failed += 1
                if not args.continue_on_error:
                    return 1
    else:
        print(f"  [10/11] PACKAGE     SKIPPED")
        stages_skipped += 1

    # ── STAGE 11: VALIDATE ──
    if "validate" not in skip:
        print(f"  [11/11] VALIDATE    XPN integrity + profile phenotype check")
        if args.dry_run:
            if profile_id:
                try:
                    import xpn_profile_validator as validator_mod
                    v = validator_mod.ProfileValidator(profile_id=profile_id)
                    v.dry_run()
                except Exception:
                    print(f"         → Would call: xpn_profile_validator.py --profile {profile_id}")
            else:
                print(f"         → Would call: xpn_validator.py")
        else:
            # Phase 1: Profile phenotype validation (if profile is set)
            if profile_id:
                try:
                    import xpn_profile_validator as validator_mod

                    v = validator_mod.ProfileValidator(
                        profile_id=profile_id,
                        pack_id=pack_id,
                        builds_root=output_dir.parent,
                    )

                    # Load pack_dna.json from build output
                    pack_dna = validator_mod._load_pack_dna(output_dir)
                    report = v.validate(pack_dna)

                    # Print report
                    validator_mod._print_report(report)

                    # Save report as JSON
                    report_path = output_dir / "validation_report.json"
                    with open(report_path, "w", encoding='utf-8') as f:
                        json.dump(report, f, indent=2)
                    print(f"         [OK] Report: {report_path}")

                    if report.get("overall") == "FAIL":
                        print(f"         [FAIL] Profile validation FAILED — "
                              f"{len(report.get('failed', []))} critical assertion(s)")
                        if not args.continue_on_error:
                            return 1
                    else:
                        print(f"         [OK] Profile validation PASSED — "
                              f"score {report.get('score', 0)}%")
                except ImportError:
                    print(f"         [SKIP] xpn_profile_validator not available")
                except Exception as e:
                    print(f"         [FAIL] Profile validation error: {e}")
                    if not args.continue_on_error:
                        return 1
            else:
                print(f"         [WARN] No profile — skipping phenotype validation")

            # Phase 2: WAV content audit
            # Check that rendered WAVs exist, are non-empty, and count matches expectation.
            renders_dir = output_dir / "renders"
            if renders_dir.exists():
                all_wavs = sorted(renders_dir.glob("*.WAV")) + \
                           sorted(renders_dir.glob("*.wav"))
                empty_wavs = [w for w in all_wavs if w.stat().st_size == 0]
                total_bytes = sum(w.stat().st_size for w in all_wavs)
                print(f"         → WAV audit: {len(all_wavs)} file(s), "
                      f"{total_bytes / 1024 / 1024:.1f} MB total")
                if empty_wavs:
                    print(f"         [FAIL] {len(empty_wavs)} zero-byte WAV(s) — render likely failed:")
                    for w in empty_wavs[:5]:
                        print(f"              {w.name}")
                    if len(empty_wavs) > 5:
                        print(f"              ... and {len(empty_wavs) - 5} more")
                    if not args.continue_on_error:
                        return 1
                else:
                    print(f"         [OK] All {len(all_wavs)} WAV(s) non-empty")
                    # Check expected count when selected_presets count is known.
                    manifest_path = output_dir / "render_manifest.json"
                    if manifest_path.exists():
                        try:
                            with open(manifest_path, encoding='utf-8', errors='replace') as _mf:
                                _manifest = json.load(_mf)
                            expected = _manifest.get("total_jobs", 0)
                            if expected and len(all_wavs) != expected:
                                print(f"         [WARN] Expected {expected} WAV(s) per manifest, "
                                      f"got {len(all_wavs)}")
                            elif expected:
                                print(f"         [OK] WAV count matches manifest ({expected})")
                        except Exception:
                            pass
            else:
                print(f"         → No renders/ directory — WAV audit skipped "
                      f"(run RENDER stage first)")

            # Phase 3: Structural XPN integrity check (existing validator)
            xpn_path = output_dir / f"XO_OX_{pack_id.replace('-', '_')}_v{version}.xpn"
            if xpn_path.exists():
                print(f"         → XPN file exists: {xpn_path.name} ({xpn_path.stat().st_size:,} bytes)")
            else:
                print(f"         → XPN not yet built — structural check deferred")
        stages_run += 1
        _update_build_log(log_path, "validate")
    else:
        print(f"  [11/11] VALIDATE    SKIPPED")
        stages_skipped += 1

    # Write final build log — merge with the incrementally-written completed_stages
    # so that --resume reads the canonical set of completed stage names.
    _existing_log: dict = {}
    if log_path.exists():
        try:
            with open(log_path, encoding='utf-8', errors='replace') as _lf:
                _existing_log = json.load(_lf)
        except Exception:
            pass
    build_log = {
        "pack_id": pack_id,
        "pack_name": pack_name,
        "engine": engine,
        "version": version,
        "experiment_id": experiment_id,
        "experiment_tags": experiment_tags,
        "build_time": build_time,
        "oxbuild_source": str(oxbuild_path),
        "stages_run": stages_run,
        "stages_skipped": stages_skipped,
        "stages_failed": stages_failed,
        "dry_run": args.dry_run,
        "output_dir": str(output_dir),
        "completed_stages": _existing_log.get("completed_stages", []),
        "last_updated": datetime.now().isoformat(),
    }
    with open(log_path, "w", encoding='utf-8') as f:
        json.dump(build_log, f, indent=2)

    print()
    print(f"  ─────────────────────────────────")
    _label = 'plan' if args.dry_run else 'complete'
    _dry = '(dry)' if args.dry_run else ''
    _failed_str = f", {stages_failed} failed" if stages_failed else ""
    print(f"  Build {_label}: {stages_run} completed{_dry}, {stages_skipped} skipped{_failed_str}")
    if experiment_id:
        print(f"  Experiment ID: {experiment_id}")
    print(f"  Output: {output_dir}")
    print(f"  Log: {log_path}")
    return 0


# ---------------------------------------------------------------------------
# Verify subcommand
# ---------------------------------------------------------------------------

def cmd_verify(args) -> int:
    """Verify provenance chain."""
    from xpn_provenance import verify_provenance
    target = Path(args.path)
    if target.is_dir():
        prov_file = target / "provenance.json"
    else:
        prov_file = target
    if not prov_file.exists():
        print(f"ERROR: {prov_file} not found")
        return 1
    result = verify_provenance(prov_file.read_text(), prov_file.parent)
    if result["valid"]:
        print(f"  VERIFIED: {result['verified']}/{result['total']} artifacts checked, chain intact")
        return 0
    else:
        print(f"  FAILED: {len(result['errors'])} error(s):")
        for err in result["errors"]:
            print(f"    - {err}")
        return 1


# ---------------------------------------------------------------------------
# Approve subcommand — human listen gate
# ---------------------------------------------------------------------------

def cmd_approve(args) -> int:
    """Write a .listen_approved marker so the next build --resume can continue to PACKAGE."""
    output_dir = Path(args.output_dir)
    if not output_dir.exists():
        print(f"ERROR: Output directory not found: {output_dir}", file=sys.stderr)
        return 1
    marker = output_dir / ".listen_approved"
    timestamp = datetime.now().isoformat()
    marker.write_text(timestamp, encoding="utf-8")
    print(f"[APPROVED] Pack approved for packaging. Run build --resume to continue.")
    print(f"           Marker: {marker}")
    return 0


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Oxport — XO_OX Expansion Pack Pipeline Orchestrator. "
                    "See also: oxport_render.py for standalone render operations.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__ + """
Entry Points:
  oxport.py build    Full 10-stage pipeline (recommended)
  oxport.py doctor   Environment validation
  oxport.py run      Execute the full export pipeline
  oxport_render.py   Standalone render tool (used internally by oxport.py)
""",
    )
    sub = parser.add_subparsers(dest="command")

    # --- run ---
    p_run = sub.add_parser("run", help="Execute the full export pipeline")
    p_run.add_argument("--engine",     required=True,
                       help="Engine name (e.g. Onset, Obsidian, Opal; legacy aliases like Odyssey/Oblong/Obese also accepted)")
    p_run.add_argument("--preset",     metavar="NAME",
                       help="Filter to a single preset name (partial match)")
    p_run.add_argument("--wavs-dir",   metavar="DIR",
                       help="Directory containing rendered WAV files")
    p_run.add_argument("--output-dir", required=True,
                       help="Output directory for the built pack")
    p_run.add_argument("--pack-name",  metavar="NAME",
                       help="Display name for the pack (default: 'XO_OX {engine}')")
    p_run.add_argument("--kit-mode",   default="smart",
                       choices=["velocity", "cycle", "random",
                                "random-norepeat", "smart"],
                       help="Kit expansion mode for drum engines (default: smart)")
    p_run.add_argument("--version",    default="1.0",
                       help="Pack version string (default: 1.0)")
    p_run.add_argument("--collection", metavar="NAME",
                       choices=["artwork"],
                       help="Collection name — enables collection-specific stages "
                            "(e.g. 'artwork' enables complement_chain)")
    p_run.add_argument("--skip",       metavar="STAGES",
                       help="Comma-separated stages to skip "
                            f"({', '.join(STAGES)})")
    p_run.add_argument("--dry-run",    action="store_true",
                       help="Show what would be executed without running")
    p_run.add_argument("--strict-qa",  action="store_true", dest="strict_qa",
                       help="Abort the pipeline on any QA failure (default: warn and continue)")
    p_run.add_argument("--auto-fix",   action="store_true", dest="auto_fix",
                       help="Auto-fix safe QA issues in-place (DC_OFFSET, ATTACK_PRESENCE, "
                            "SILENCE_TAIL) before re-running QA. No artistic impact.")
    p_run.add_argument("--tuning",     metavar="SYSTEM", default=None,
                       help="Apply a microtonal tuning system to keygroup XPM root note assignments "
                            "(e.g. just_intonation, 19tet, pythagorean, maqam_rast). "
                            "Run xpn_tuning_systems.py --list-tunings to see all options. "
                            "Tuning name is also appended to the expansion manifest description.")
    p_run.add_argument("--no-round-robin", action="store_false", dest="round_robin",
                       help="Disable round-robin layer support for keygroup programs "
                            "(round-robin is enabled by default)")
    p_run.add_argument("--choke-preset", metavar="PRESET", default="none",
                       choices=["onset", "standard", "none"], dest="choke_preset",
                       help="Apply a choke group preset to each generated XPM after export. "
                            "'onset' = Onset drum voice choke groups, "
                            "'standard' = generic hi-hat choke groups, "
                            "'none' = no choke assignment (default). "
                            "Requires xpn_choke_group_assigner module.")

    # --- status ---
    p_status = sub.add_parser("status", help="Show pipeline state")
    p_status.add_argument("--output-dir", required=True,
                          help="Output directory to inspect")

    # --- validate ---
    p_validate = sub.add_parser("validate",
                                help="Validate pipeline output and/or .xometa presets")
    p_validate.add_argument("--output-dir", default=None,
                            help="Output directory to validate (structural XPM/XPN checks)")
    p_validate.add_argument("--presets",    action="store_true",
                            help="Run 10-point .xometa preset validation (schema, DNA, naming, "
                                 "coupling, parameters, mood balance)")
    p_validate.add_argument("--fix",        action="store_true",
                            help="Auto-fix trivially correctable preset issues")
    p_validate.add_argument("--strict",     action="store_true",
                            help="Treat warnings as errors (for CI)")
    p_validate.add_argument("--xpn",        default=None, metavar="FILE_OR_DIR",
                            help="Validate .xpn archive(s) against Rex's XPN Bible "
                                 "(CRITICAL/WARNING/INFO severity levels)")

    # --- batch ---
    p_batch = sub.add_parser("batch",
                             help="Run the pipeline for multiple engines from a JSON config")
    p_batch.add_argument("--config",       required=True, metavar="FILE",
                         help="JSON config file with batch job definitions")
    p_batch.add_argument("--parallel",     type=int, default=1, metavar="N",
                         help="Number of parallel jobs (default: 1 = sequential)")
    p_batch.add_argument("--dry-run",      action="store_true", dest="dry_run",
                         help="Show commands without executing")
    p_batch.add_argument("--skip-failed",  action="store_true", dest="skip_failed",
                         help="Continue batch even if a job fails")
    p_batch.add_argument("--normalize",    action="store_true",
                         help="After all jobs complete, normalize loudness across engines "
                              "(LUFS via ITU-R BS.1770-4; falls back to RMS if xpn_normalize "
                              "is unavailable; target -14 LUFS)")
    p_batch.add_argument("--normalize-target", type=float, default=-14.0,
                         metavar="LUFS", dest="normalize_target",
                         help="Target loudness in LUFS (default: -14.0)")

    # --- build (.oxbuild compiler) ---
    p_build = sub.add_parser("build",
                             help="Compile a .oxbuild spec into a complete .xpn pack")
    p_build.add_argument("oxbuild", metavar="SPEC",
                         help="Path to .oxbuild JSON spec file or .oxpack YAML manifest")
    p_build.add_argument("--output-dir", default=None,
                         help="Output directory (default: builds/{pack_id}/)")
    p_build.add_argument("--skip", metavar="STAGES",
                         help="Comma-separated stages to skip "
                              f"({', '.join(BUILD_STAGES)})")
    p_build.add_argument("--dry-run", action="store_true", dest="dry_run",
                         help="Show compilation plan without executing")
    p_build.add_argument("--no-render", action="store_true", dest="no_render",
                         help="Parse render spec but skip actual audio capture "
                              "(use pre-rendered WAVs)")
    p_build.add_argument("--midi-port", default=None, metavar="PORT",
                         help="MIDI output port name for rendering")
    p_build.add_argument("--audio-device", default=None, metavar="DEVICE",
                         help="Audio input device for recording (e.g. 'BlackHole')")
    p_build.add_argument("--continue-on-error", action="store_true",
                         dest="continue_on_error",
                         help="Continue build even if a stage fails")
    p_build.add_argument("--auto-fix", action="store_true", dest="auto_fix",
                         help="Auto-fix safe QA issues in-place (DC_OFFSET, ATTACK_PRESENCE, "
                              "SILENCE_TAIL) when WAV QA is run during the validate stage.")
    p_build.add_argument("--skip-probe", action="store_true", dest="skip_probe",
                         help="Skip Stage 0 voice probe (load existing voice_map.json if present, "
                              "or render all voices). Use when voice_map.json already exists "
                              "from a prior probe run.")
    p_build.add_argument("--tier", choices=["SURFACE", "DEEP", "TRENCH"], default=None,
                         metavar="TIER",
                         help="Override tier from .oxbuild (SURFACE | DEEP | TRENCH). "
                              "Controls velocity layers, round-robin policy, and max slot budget.")
    p_build.add_argument("--preset-count", type=int, default=None, dest="preset_count",
                         metavar="N",
                         help="Override preset_selection.min_presets from .oxbuild. "
                              "Useful for quick test builds without editing the spec file.")
    p_build.add_argument("--resume", action="store_true",
                         help="Resume a previously interrupted build from the last completed stage. "
                              "Reads build.log.json from the output directory and skips all stages "
                              "that already completed successfully.")
    p_build.add_argument("--skip-listen", action="store_true", dest="skip_listen",
                         help="Bypass the LISTEN gate — skip human listening approval before "
                              "packaging. Useful for automated/CI builds.")

    # --- verify ---
    p_verify = sub.add_parser("verify", help="Verify .xpn provenance chain")
    p_verify.add_argument("path", help="Path to provenance.json or build directory")

    # --- doctor ---
    p_doctor = sub.add_parser("doctor", help="Validate environment before rendering")
    p_doctor.add_argument("spec", nargs="?",
                          help="Optional .oxbuild file to validate against")
    p_doctor.add_argument("--audio-device", default="BlackHole",
                          dest="audio_device",
                          help="Audio input device name to look for (default: BlackHole)")
    p_doctor.add_argument("--midi-port", default="IAC Driver Bus 1",
                          dest="midi_port",
                          help="MIDI output port name to look for (default: IAC Driver Bus 1)")
    p_doctor.add_argument("--ci", action="store_true",
                          help="CI mode: skip all audio hardware checks and smoke test")
    p_doctor.set_defaults(func=cmd_doctor)

    # --- build: add --skip-doctor flag ---
    p_build.add_argument("--skip-doctor", action="store_true", dest="skip_doctor",
                         help="Skip the lightweight pre-build dependency/disk check")

    # --- approve ---
    p_approve = sub.add_parser("approve",
                               help="Mark a build output as listen-approved to ungate PACKAGE")
    p_approve.add_argument("output_dir", metavar="OUTPUT_DIR",
                           help="Build output directory to approve (contains rendered WAVs)")

    args = parser.parse_args()
    if not args.command:
        parser.print_help()
        return 1

    dispatch = {
        "run":      cmd_run,
        "status":   cmd_status,
        "validate": cmd_validate,
        "batch":    cmd_batch,
        "build":    cmd_build,
        "verify":   cmd_verify,
        "doctor":   cmd_doctor,
        "approve":  cmd_approve,
    }
    return dispatch[args.command](args)


if __name__ == "__main__":
    sys.exit(main())
