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
 10. package           — Package everything into a .xpn archive

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

    # Batch with cross-engine loudness normalization
    python3 oxport.py batch --config batch.json --normalize
    python3 oxport.py batch --config batch.json --normalize --normalize-target -16.0
"""

from __future__ import annotations

import argparse
import json
import math
import struct
import sys
import time
import uuid
from datetime import datetime
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Resolve Tools/ directory so sibling imports work when invoked from anywhere
# ---------------------------------------------------------------------------
TOOLS_DIR = Path(__file__).parent.resolve()
REPO_ROOT = TOOLS_DIR.parent
PRESETS_DIR = REPO_ROOT / "Presets" / "XOmnibus"

if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

# ---------------------------------------------------------------------------
# Lazy imports — each stage imports its tool only when needed.
# This keeps startup fast and lets --dry-run work without heavy deps.
# ---------------------------------------------------------------------------

# Engines whose presets produce drum programs (everything else is keygroup)
DRUM_ENGINES = {"onset", "Onset", "ONSET", "OnsetEngine"}

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
                 tuning: Optional[str] = None,
                 choke_preset: str = "none",
                 round_robin: bool = True):
        self.engine = engine
        self.output_dir = output_dir
        self.wavs_dir = wavs_dir
        self.preset_filter = preset_filter
        self.kit_mode = kit_mode
        self.version = version
        self.pack_name = pack_name or f"XO_OX {engine}"
        self.dry_run = dry_run
        self.strict_qa = strict_qa
        self.tuning = tuning  # Optional tuning system name (from xpn_tuning_systems)
        self.choke_preset = choke_preset  # "onset" | "standard" | "none"
        self.round_robin = round_robin  # Enable round-robin layer support (default: on)

        # Accumulated outputs
        self.render_specs: list[dict] = []
        self.categories: dict = {}
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
                with open(xmeta) as f:
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
                with open(xmeta) as _f:
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

            ctx.render_specs.append(spec)

            if not ctx.dry_run:
                spec_path = ctx.specs_dir / f"{spec['preset_slug']}_render_spec.json"
                with open(spec_path, "w") as f:
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

    # DNA pre-flight: sample up to 5 WAVs and report sonic character
    try:
        import xpn_auto_dna as _dna  # lazy import
        wav_sample = sorted(ctx.wavs_dir.glob("*.wav"))[:5] + \
                     sorted(ctx.wavs_dir.glob("*.WAV"))[:5]
        wav_sample = wav_sample[:5]
        if wav_sample:
            results = [_dna.compute_dna(str(p)) for p in wav_sample]
            brightness  = sum(r.get("brightness",  0.0) for r in results) / len(results)
            warmth      = sum(r.get("warmth",      0.0) for r in results) / len(results)
            aggression  = sum(r.get("aggression",  0.0) for r in results) / len(results)
            print(f"    DNA pre-flight: brightness={brightness:.2f}, "
                  f"warmth={warmth:.2f}, aggression={aggression:.2f}")
            if aggression > 0.8:
                print("    ⚡ High aggression pack — consider tagging as 'aggressive'")
            if brightness < 0.3:
                print("    🌊 Dark/deep pack — consider tagging as 'dark'")
    except (ImportError, Exception):
        pass  # module not available or analysis failed — skip silently

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

    print(f"    Checking {len(wav_paths)} WAV file(s)...")

    n_pass   = 0
    n_fail   = 0
    warnings: list[str] = []   # issue descriptions for blocking/non-blocking issues
    abort_issues = {"CLIPPING", "PHASE_CANCELLED"}

    for wav_path in wav_paths:
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
    if warnings:
        summary = f"QA: {n_pass}/{total} passed ({len(warnings)} warning(s): {'; '.join(warnings[:3])}" + \
                  (" ..." if len(warnings) > 3 else "") + ")"
    else:
        summary = f"QA: {n_pass}/{total} passed"
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


def _stage_export(ctx: PipelineContext) -> None:
    """Stage 4: Generate .xpm programs."""
    if ctx.is_drum_engine:
        _export_drum(ctx)
    else:
        _export_keygroup(ctx)


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

    # The packager expects .xpm files in the build dir or Programs/ subdir
    # Move .xpm files into the expected location if needed
    for xpm in ctx.xpm_paths:
        if xpm.parent != ctx.build_dir:
            target = ctx.build_dir / xpm.name
            if not target.exists() and xpm.exists():
                import shutil
                shutil.copy2(xpm, target)

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
# Loudness normalization helpers (RMS-based, stdlib only)
# ---------------------------------------------------------------------------

def _compute_rms_db(wav_path: Path) -> float:
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


def _apply_gain_db(wav_path: Path, gain_db: float) -> None:
    """Apply a gain offset (in dB) to a WAV file in-place."""
    if abs(gain_db) < 0.01:
        return  # negligible — skip

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
            v = int(samples[i] * gain_linear)
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
            val = int(val * gain_linear)
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
            v = int(samples[i] * gain_linear)
            samples[i] = max(-2147483648, min(2147483647, v))
        new_data = struct.pack(fmt, *samples)
    else:
        return  # unsupported bit depth

    _write_wav(wav_path, num_ch, sr, bps, new_data)


def normalize_batch_loudness(output_base: Path, engine_dirs: list[Path],
                             target_lufs: float = -14.0,
                             dry_run: bool = False) -> dict[str, float]:
    """Normalize loudness across engine output directories.

    Computes RMS (as LUFS proxy) per engine, then applies per-engine gain to
    reach the target. Returns a dict of engine_name -> gain_db applied.

    If target_lufs is None, uses the median RMS across engines as the target.
    """
    engine_rms: dict[str, list[float]] = {}

    for edir in engine_dirs:
        if not edir.exists():
            continue
        engine_name = edir.name
        wavs = list(edir.rglob("*.wav")) + list(edir.rglob("*.WAV"))
        if not wavs:
            continue

        rms_values = []
        for wav in wavs:
            rms = _compute_rms_db(wav)
            if rms > float("-inf"):
                rms_values.append(rms)

        if rms_values:
            engine_rms[engine_name] = rms_values

    if not engine_rms:
        print("    [SKIP] No WAV files found across engine directories")
        return {}

    # Compute per-engine average RMS
    engine_avg: dict[str, float] = {}
    for name, values in engine_rms.items():
        engine_avg[name] = sum(values) / len(values)

    # Determine target
    if target_lufs is None:
        sorted_avgs = sorted(engine_avg.values())
        mid = len(sorted_avgs) // 2
        target = sorted_avgs[mid] if len(sorted_avgs) % 2 == 1 else \
            (sorted_avgs[mid - 1] + sorted_avgs[mid]) / 2.0
    else:
        target = target_lufs

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

    return adjustments


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

    ctx.ensure_dirs()

    pipeline_start = time.monotonic()
    failed_stage = None

    for stage in STAGES:
        if stage in skip_stages:
            print(f"  [{stage}] SKIPPED")
            continue

        print(f"  [{stage}] {STAGE_DESCRIPTIONS[stage]}")
        stage_start = time.monotonic()

        try:
            STAGE_FUNCS[stage](ctx)
        except Exception as e:
            elapsed = time.monotonic() - stage_start
            ctx.stage_times[stage] = elapsed
            print(f"    [FAIL] {e}")
            failed_stage = stage
            break

        elapsed = time.monotonic() - stage_start
        ctx.stage_times[stage] = elapsed
        print(f"    Done ({elapsed:.2f}s)")
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
        with open(state_path, "w") as f:
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

    print(BANNER)
    for state_path in states:
        try:
            with open(state_path) as f:
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
    print(BANNER)

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
            import validate_presets as vp
            # Build the argv that validate_presets expects
            vp_argv = []
            if args.fix:
                vp_argv.append("--fix")
            if args.strict:
                vp_argv.append("--strict")
            vp_argv.append("--report")

            # validate_presets uses sys.argv internally, so we patch it
            import sys as _sys
            old_argv = _sys.argv
            _sys.argv = ["validate_presets.py"] + vp_argv
            try:
                vp_rc = vp.main() if hasattr(vp, "main") else 0
            except SystemExit as e:
                vp_rc = e.code if e.code is not None else 0
            finally:
                _sys.argv = old_argv

            if vp_rc != 0:
                exit_code = max(exit_code, vp_rc)
        except ImportError:
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
            import xpn_validator as xv
            import sys as _sys
            old_argv = _sys.argv
            xv_argv = ["xpn_validator.py", args.xpn]
            if args.strict:
                xv_argv.append("--strict")
            _sys.argv = xv_argv
            try:
                xv_rc = xv.main() if hasattr(xv, "main") else 0
            except SystemExit as e:
                xv_rc = e.code if e.code is not None else 0
            finally:
                _sys.argv = old_argv
            if xv_rc != 0:
                exit_code = max(exit_code, xv_rc)
        except ImportError:
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
            target_lufs=target,
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
    "fallback", "intent", "docs", "art", "package", "validate",
]

def cmd_build(args) -> int:
    """Compile a .oxbuild spec into a complete .xpn pack."""
    oxbuild_path = Path(args.oxbuild)
    if not oxbuild_path.exists():
        print(f"ERROR: .oxbuild file not found: {oxbuild_path}", file=sys.stderr)
        return 1

    with open(oxbuild_path) as f:
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
    sample_rate = rendering.get("sample_rate", 44100)
    bit_depth = rendering.get("bit_depth", 24)
    velocity_layers = rendering.get("velocity_layers", 4)
    velocity_values = rendering.get("velocity_values", [20, 50, 90, 127])
    render_spec_override = rendering.get("render_spec_override")
    # preset_load_ms: how long to wait after MIDI program change before recording.
    # 200ms default is too tight for XOmnibus with 50+ presets — use 400ms or set
    # rendering.preset_load_ms in the .oxbuild spec.
    preset_load_ms = rendering.get("preset_load_ms", 400)

    # Corner config
    corner_strategy = spec.get("corner_strategy", "dynamic_expression")
    pad_count = spec.get("pad_count", 16)

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
    if profile_id:
        profile_path = REPO_ROOT / "profiles" / f"{profile_id}.yaml"
        if profile_path.exists():
            try:
                # Try yaml first, fall back to json-compatible parsing
                import yaml
                with open(profile_path) as pf:
                    profile = yaml.safe_load(pf)
            except ImportError:
                # Fallback: read as text for display, skip structured access
                profile = {"_raw": profile_path.read_text(), "profile_id": profile_id}
            except Exception as e:
                print(f"  ⚠  Profile load failed: {e}")
        else:
            print(f"  ⚠  Profile not found: {profile_path}")

    total_samples = pad_count * 4 * velocity_layers  # pads * corners * vel layers
    stages_run = 0
    stages_skipped = 0

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
    if args.dry_run:
        print(f"         ✓ Parsed {oxbuild_path}")
    stages_run += 1

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
                    # preset in XOmnibus's preset browser, which lists them in
                    # alphabetical-path order (same as Python sorted() on the
                    # .xometa file paths — matching JUCE File::findChildFiles).
                    # Re-sort selected_presets by path so program N == browser N.
                    # Mood folder order: Aether < Atmosphere < Entangled < Family
                    #   < Flux < Foundation < Prism < Submerged (alphabetical).
                    selected_presets = sorted(
                        selected_presets,
                        key=lambda p: p.get("path", ""),
                    )
                    print(
                        f"         ✓ Bank order aligned to XOmnibus browser "
                        f"(alphabetical by preset path)"
                    )

                    # Save selection manifest
                    manifest_path = output_dir / "selected_presets.json"
                    with open(manifest_path, "w") as f:
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
                        with open(dna_path, "w") as f:
                            json.dump(pack_dna_data, f, indent=2)
                        print(f"         ✓ pack_dna.json written (pack average from {len(selected_presets)} presets)")

                    # Save QA log if populated
                    if qa_log and len(qa_log):
                        qa_log.save()

                    print(f"         ✓ Selected {len(selected_presets)} presets (target: {min_presets})")
                    print(f"         ✓ Manifest: {manifest_path}")

                    if len(selected_presets) < min_presets:
                        print(f"         ⚠  Only {len(selected_presets)} presets matched — "
                              f"below target of {min_presets}")
                else:
                    print(f"         ⚠  No profile specified — skipping DNA-based selection")
            except ImportError:
                print(f"         ⚠  xpn_profile_preset_selector not available")
            except Exception as e:
                print(f"         ✗  Selection failed: {e}")
                if not args.continue_on_error:
                    return 1
        stages_run += 1
    else:
        print(f"  [2/10] SELECT       SKIPPED")
        stages_skipped += 1

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
        # program number = 0-based index of that preset in the XOmnibus bank as loaded.
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
                print(f"         → Would call: oxport_render.py --spec <spec> --output-dir {output_dir / 'renders'}")
        else:
            renders_dir = output_dir / "renders"
            renders_dir.mkdir(exist_ok=True)
            if render_spec_override:
                spec_path = REPO_ROOT / render_spec_override
            else:
                spec_path = output_dir / f"{pack_id}_render_spec.json"
                # Auto-generate render spec would go here
                print(f"         ⚠  Auto render spec generation not yet implemented")
                print(f"            Use rendering.render_spec_override in .oxbuild to point to a spec")

            if spec_path.exists():
                print(f"         → Launching render: {spec_path}")
                # Import and run the render engine
                try:
                    import oxport_render
                    render_spec_data = oxport_render.load_spec(str(spec_path))
                    note_layout = oxport_render.resolve_presets(render_spec_data)

                    if selected_presets:
                        # SELECT → RENDER: expand note-layout × selected presets.
                        # Each selected preset gets program number = its 0-based index.
                        # Slug prefix = preset name (slugified) so WAVs are organized per preset.
                        all_jobs: list[dict] = []
                        for prog_num, sel_preset in enumerate(selected_presets):
                            preset_slug_prefix = sel_preset["name"].replace(" ", "_")
                            # Clone note-layout entries with the correct program number injected
                            patched_layout = []
                            for entry in note_layout:
                                patched = dict(entry)
                                patched["program"] = prog_num
                                # Prefix slug so WAV filenames are unique across presets
                                patched["slug"] = f"{preset_slug_prefix}__{entry.get('slug', entry.get('name', 'voice'))}"
                                patched_layout.append(patched)
                            preset_jobs = oxport_render.build_render_jobs(render_spec_data, patched_layout)
                            all_jobs.extend(preset_jobs)

                        # Save the wired render manifest for auditability
                        wired_manifest_path = output_dir / "render_manifest.json"
                        with open(wired_manifest_path, "w") as _wm:
                            json.dump({
                                "source_spec": str(spec_path),
                                "selected_presets_count": len(selected_presets),
                                "total_jobs": len(all_jobs),
                                "presets": [
                                    {"name": p["name"], "program": i, "path": p.get("path", "")}
                                    for i, p in enumerate(selected_presets)
                                ],
                            }, _wm, indent=2)
                        print(f"         ✓ Render manifest: {wired_manifest_path}")
                        print(f"         → {len(all_jobs)} total render jobs "
                              f"({len(selected_presets)} presets × {len(note_layout)} voices × {velocity_layers} vel)")
                        jobs = all_jobs
                    else:
                        # No SELECT data — render spec as-is (program numbers from the spec file)
                        jobs = oxport_render.build_render_jobs(render_spec_data, note_layout)
                        print(f"         → {len(jobs)} render jobs queued (no SELECT override)")

                    if not args.no_render:
                        oxport_render.render_jobs(
                            jobs, str(renders_dir),
                            midi_port_name=args.midi_port,
                            audio_device=args.audio_device,
                            sample_rate=sample_rate,
                            preset_load_ms=preset_load_ms,
                        )
                    else:
                        print(f"         → --no-render flag: skipping actual audio capture")
                except ImportError:
                    print(f"         ⚠  oxport_render not available (pip install mido sounddevice numpy)")
                except Exception as e:
                    print(f"         ✗  Render failed: {e}")
                    if not args.continue_on_error:
                        return 1
        stages_run += 1
    else:
        print(f"  [3/10] RENDER       SKIPPED")
        stages_skipped += 1

    # ── STAGE 4: ASSEMBLE ──
    assembled_xpm_paths: list[Path] = []  # populated here, consumed by PACKAGE
    if "assemble" not in skip:
        print(f"  [4/10] ASSEMBLE     MPCe XPM ({corner_strategy} corners)")
        if args.dry_run:
            print(f"         → Would call: xpn_mpce_quad_builder.py --engine {engine} --pad-count {pad_count}")
        else:
            try:
                import xpn_mpce_quad_builder as qb

                # Determine where to load presets from.
                # Use the Presets/XOmnibus tree (search all mood subdirs for this engine).
                presets_search_dir = PRESETS_DIR
                presets = qb.load_presets(str(presets_search_dir), engine)

                if not presets:
                    print(f"         ⚠  No .xometa presets found for engine '{engine}' "
                          f"in {presets_search_dir}")
                else:
                    assignments = qb.assign_corners(presets, pad_count)
                    programs_dir = output_dir / "Programs"
                    programs_dir.mkdir(exist_ok=True)
                    xpm_path = qb.generate_xpm(engine, assignments, str(programs_dir))
                    assembled_xpm_paths.append(Path(xpm_path))
                    print(f"         ✓ {len(assignments)} pad(s) assigned "
                          f"({len(presets)} presets loaded)")
                    print(f"         ✓ Written: {xpm_path}")

                    # Also copy renders/ WAVs into the expected Samples/ location
                    # so the packager can include them in the archive.
                    renders_dir = output_dir / "renders"
                    if renders_dir.exists():
                        import shutil as _shutil
                        program_slug = Path(xpm_path).stem
                        samples_dest = output_dir / "Samples" / program_slug
                        samples_dest.mkdir(parents=True, exist_ok=True)
                        wav_count = 0
                        for wav in sorted(renders_dir.glob("*.WAV")) + \
                                   sorted(renders_dir.glob("*.wav")):
                            dest_wav = samples_dest / wav.name
                            if not dest_wav.exists():
                                _shutil.copy2(wav, dest_wav)
                            wav_count += 1
                        print(f"         ✓ {wav_count} WAV(s) staged → Samples/{program_slug}/")

            except ImportError:
                print(f"         ⚠  xpn_mpce_quad_builder not available")
            except Exception as e:
                print(f"         ✗  Assemble failed: {e}")
                if not args.continue_on_error:
                    return 1
        stages_run += 1
    else:
        print(f"  [4/10] ASSEMBLE     SKIPPED")
        stages_skipped += 1

    # ── STAGE 5: FALLBACK ──
    if "fallback" not in skip and include_standard:
        print(f"  [5/10] FALLBACK     Standard XPM (velocity-layered, non-MPCe)")
        if args.dry_run:
            print(f"         → Would call: xpn_drum_export.py --mode smart")
        else:
            print(f"         ⚠  Standard XPM fallback not yet implemented")
        stages_run += 1
    else:
        print(f"  [5/10] FALLBACK     SKIPPED")
        stages_skipped += 1

    # ── STAGE 6: INTENT ──
    if "intent" not in skip and include_intent:
        print(f"  [6/10] INTENT       xpn_intent.json sidecar")
        if args.dry_run:
            print(f"         → Would call: xpn_intent_generator.py --corner-pattern {corner_strategy}")
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
                with open(intent_path, "w") as f:
                    json.dump(intent_data, f, indent=2)
                print(f"         ✓ Written: {intent_path}")
            except ImportError:
                print(f"         ⚠  xpn_intent_generator not available")
            except Exception as e:
                print(f"         ✗  Intent generation failed: {e}")
                if not args.continue_on_error:
                    return 1
        stages_run += 1
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
                    with open(dest, "a") as f:
                        f.write(f"\n\n<!-- experiment_id: {experiment_id} -->\n")
                        f.write(f"<!-- build_time: {build_time} -->\n")
                print(f"         ✓ Written: {dest}")
            else:
                print(f"         ⚠  Template not found: {template_path}")
        stages_run += 1
    else:
        print(f"  [7/10] DOCS         SKIPPED")
        stages_skipped += 1

    # ── STAGE 8: ART ──
    if "art" not in skip and include_cover_art:
        print(f"  [8/10] ART          Cover art{f' [{cover_art_badge}]' if cover_art_badge else ''}")
        if args.dry_run:
            print(f"         → Would call: xpn_cover_art_generator_v2.py --engine {engine}")
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
                        with open(pack_dna_path) as _f:
                            _pd = json.load(_f)
                        _avg = _pd.get("pack_average", {})
                        sonic_dna.update({k: v for k, v in _avg.items()
                                         if k in sonic_dna})
                    except Exception:
                        pass

                png_bytes = art_gen.generate_cover_art(
                    engine=engine,
                    pack_name=pack_name,
                    sonic_dna=sonic_dna,
                    size=400,
                )
                art_path = output_dir / "artwork.png"
                art_path.write_bytes(png_bytes)
                print(f"         ✓ artwork.png written ({len(png_bytes):,} bytes)")

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
                    print(f"         ✓ artwork_2000.png written ({len(png_2000):,} bytes)")
                except Exception:
                    pass  # hi-res is optional

            except ImportError:
                print(f"         ⚠  xpn_cover_art_generator_v2 not available")
            except Exception as e:
                print(f"         ✗  Art generation failed: {e}")
        stages_run += 1
    else:
        print(f"  [8/10] ART          SKIPPED")
        stages_skipped += 1

    # ── STAGE 9: PACKAGE ──
    if "package" not in skip:
        xpn_name = f"XO_OX_{pack_id.replace('-', '_')}_v{version}.xpn"
        print(f"  [9/10] PACKAGE      {xpn_name}")
        if args.dry_run:
            print(f"         → Would call: xpn_packager.py --output {output_dir / xpn_name}")
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
                    print(f"         ⚠  No .xpm programs found — skipping package")
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
                    print(f"         ✓ {result_path.name} ({size_mb:.2f} MB)")

            except ImportError:
                print(f"         ⚠  xpn_packager not available")
            except Exception as e:
                print(f"         ✗  Package failed: {e}")
                if not args.continue_on_error:
                    return 1
        stages_run += 1
    else:
        print(f"  [9/10] PACKAGE      SKIPPED")
        stages_skipped += 1

    # ── STAGE 10: VALIDATE ──
    if "validate" not in skip:
        print(f"  [10/10] VALIDATE    XPN integrity + profile phenotype check")
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
                    with open(report_path, "w") as f:
                        json.dump(report, f, indent=2)
                    print(f"         ✓ Report: {report_path}")

                    if report.get("overall") == "FAIL":
                        print(f"         ✗  Profile validation FAILED — "
                              f"{len(report.get('failed', []))} critical assertion(s)")
                        if not args.continue_on_error:
                            return 1
                    else:
                        print(f"         ✓ Profile validation PASSED — "
                              f"score {report.get('score', 0)}%")
                except ImportError:
                    print(f"         ⚠  xpn_profile_validator not available")
                except Exception as e:
                    print(f"         ✗  Profile validation error: {e}")
                    if not args.continue_on_error:
                        return 1
            else:
                print(f"         ⚠  No profile — skipping phenotype validation")

            # Phase 2: Structural XPN integrity check (existing validator)
            xpn_path = output_dir / f"XO_OX_{pack_id.replace('-', '_')}_v{version}.xpn"
            if xpn_path.exists():
                print(f"         → XPN file exists: {xpn_path.name} ({xpn_path.stat().st_size:,} bytes)")
            else:
                print(f"         → XPN not yet built — structural check deferred")
        stages_run += 1
    else:
        print(f"  [10/10] VALIDATE    SKIPPED")
        stages_skipped += 1

    # Write build log
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
        "dry_run": args.dry_run,
        "output_dir": str(output_dir),
    }
    log_path = output_dir / "build.log.json"
    with open(log_path, "w") as f:
        json.dump(build_log, f, indent=2)

    print()
    print(f"  ─────────────────────────────────")
    print(f"  Build {'plan' if args.dry_run else 'complete'}: {stages_run} stages{'(dry)' if args.dry_run else ''}, {stages_skipped} skipped")
    if experiment_id:
        print(f"  Experiment ID: {experiment_id}")
    print(f"  Output: {output_dir}")
    print(f"  Log: {log_path}")
    return 0


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Oxport — XO_OX Expansion Pack Pipeline Orchestrator",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    sub = parser.add_subparsers(dest="command")

    # --- run ---
    p_run = sub.add_parser("run", help="Execute the full export pipeline")
    p_run.add_argument("--engine",     required=True,
                       help="Engine name (e.g. Onset, Odyssey, Opal)")
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
                              "(RMS-based, target -14 LUFS)")
    p_batch.add_argument("--normalize-target", type=float, default=-14.0,
                         metavar="LUFS", dest="normalize_target",
                         help="Target loudness in dB RMS (default: -14.0)")

    # --- build (.oxbuild compiler) ---
    p_build = sub.add_parser("build",
                             help="Compile a .oxbuild spec into a complete .xpn pack")
    p_build.add_argument("oxbuild", metavar="SPEC",
                         help="Path to .oxbuild JSON spec file")
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
    }
    return dispatch[args.command](args)


if __name__ == "__main__":
    sys.exit(main())
