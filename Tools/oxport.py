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
  9. package           — Package everything into a .xpn archive

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
"""

import argparse
import json
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import Optional

if sys.version_info < (3, 9):
    sys.exit("Error: XOmnibus tools require Python 3.9+")

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
<<<<<<< HEAD
    """Validate pipeline output AND/OR .xometa presets."""
=======
    """Validate a pipeline output using xpn_validator.py."""
>>>>>>> origin/v1-launch-prep
    print(BANNER)

<<<<<<< HEAD
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
=======
    # Find .xpn files to validate
    xpns = list(output_dir.glob("*.xpn"))
    if not xpns:
        # Also search one level deep (e.g. output_dir/EngineName/Pack.xpn)
        xpns = list(output_dir.glob("**/*.xpn"))
    if not xpns:
        print(f"  [ERROR] No .xpn files found in {output_dir}")
        print("  Run `oxport.py run` first to generate a .xpn archive.")
        return 1

    try:
        from xpn_validator import validate_xpn, CRITICAL
    except ImportError:
        print("  [ERROR] xpn_validator.py not found in Tools directory")
        return 1

    any_critical = False
    for xpn_path in sorted(xpns):
        print(f"  Validating: {xpn_path.name}")
        result = validate_xpn(str(xpn_path))
        result.print_report()
        counts = result.counts
        if counts[CRITICAL] > 0:
            any_critical = True
>>>>>>> origin/v1-launch-prep

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

    if not args.output_dir and not args.presets:
        print("  Nothing to validate. Use --output-dir and/or --presets.")
        print()
        return 1

    print()
<<<<<<< HEAD
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
=======
    if any_critical:
        print("  RESULT: CRITICAL findings detected — fix before distribution.")
        return 1
    else:
        print("  RESULT: No CRITICAL findings.")
        return 0
>>>>>>> origin/v1-launch-prep


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

    args = parser.parse_args()
    if not args.command:
        parser.print_help()
        return 1

    dispatch = {
        "run":      cmd_run,
        "status":   cmd_status,
        "validate": cmd_validate,
        "batch":    cmd_batch,
    }
    return dispatch[args.command](args)


if __name__ == "__main__":
    sys.exit(main())
