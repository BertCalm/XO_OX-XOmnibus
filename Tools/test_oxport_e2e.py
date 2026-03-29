"""End-to-end structural tests for the oxport pipeline.

These tests exercise the full pipeline flow without audio hardware
(dry-run mode), verifying stage sequencing, context accumulation,
and subcommand behavior.

No pytest dependency — run directly:
    python3 Tools/test_oxport_e2e.py
"""
from __future__ import annotations

import argparse
import json
import os
import sys
import tempfile
import types
from pathlib import Path
from unittest.mock import patch

# Ensure Tools/ is on the path so oxport imports succeed
TOOLS_DIR = Path(__file__).parent.resolve()
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))


# ---------------------------------------------------------------------------
# Helper: build a minimal stub that satisfies _check_dependencies and each
# stage's lazy imports, so tests that call run_pipeline don't need real
# xpn_* modules to be importable.  We install them into sys.modules once
# before any test that needs them, and remove them in finally blocks.
# ---------------------------------------------------------------------------

_REQUIRED_STUBS = [
    "xpn_render_spec",
    "xpn_sample_categorizer",
    "xpn_qa_checker",
    "xpn_smart_trim",
    "xpn_drum_export",
    "xpn_packager",
]

_OPTIONAL_STUBS = [
    "xpn_cover_art",
    "xpn_complement_renderer",
    "xpn_manifest_generator",
    "xpn_choke_group_assigner",
    "xpn_tuning_systems",
    "xpn_adaptive_velocity",
    "xpn_kit_expander",
    "xpn_auto_dna",
]


def _install_stubs(extra: list[str] | None = None) -> list[str]:
    """Install minimal stub modules into sys.modules.

    Returns a list of names that were *newly* installed (so callers can
    remove only those in cleanup, leaving pre-existing real modules alone).
    """
    installed: list[str] = []
    all_names = _REQUIRED_STUBS + _OPTIONAL_STUBS + (extra or [])
    for name in all_names:
        if name not in sys.modules:
            stub = types.ModuleType(name)
            sys.modules[name] = stub
            installed.append(name)
    return installed


def _remove_stubs(names: list[str]) -> None:
    for name in names:
        sys.modules.pop(name, None)


# ---------------------------------------------------------------------------
# Test 1 — dry_run_completes_all_stages
# ---------------------------------------------------------------------------

def test_dry_run_completes_all_stages():
    """run_pipeline in dry_run mode should visit every stage and return 0."""
    from oxport import PipelineContext, run_pipeline, STAGES

    installed = _install_stubs()
    with tempfile.TemporaryDirectory() as tmp:
        ctx = PipelineContext(
            engine="Onset",
            output_dir=Path(tmp),
            dry_run=True,
        )

        # Patch each stage function to a no-op so we don't need real data
        stage_visited: list[str] = []

        def make_noop(name):
            def noop(c):
                stage_visited.append(name)
            return noop

        import oxport as _ox
        original_funcs = dict(_ox.STAGE_FUNCS)
        try:
            for s in STAGES:
                _ox.STAGE_FUNCS[s] = make_noop(s)

            rc = run_pipeline(ctx, skip_stages=set())
        finally:
            _ox.STAGE_FUNCS.update(original_funcs)
            _remove_stubs(installed)

    assert rc == 0, f"Expected return code 0, got {rc}"
    assert len(ctx.stage_times) == len(STAGES), (
        f"Expected {len(STAGES)} entries in stage_times, "
        f"got {len(ctx.stage_times)}: {list(ctx.stage_times.keys())}"
    )
    for s in STAGES:
        assert s in ctx.stage_times, f"Stage '{s}' missing from stage_times"
        assert s in stage_visited, f"Stage '{s}' was never executed"


# ---------------------------------------------------------------------------
# Test 2 — skip_stages_respected
# ---------------------------------------------------------------------------

def test_skip_stages_respected():
    """Stages listed in skip_stages must not appear in ctx.stage_times."""
    from oxport import PipelineContext, run_pipeline, STAGES

    skip = {"cover_art", "preview", "package"}
    expected_run = [s for s in STAGES if s not in skip]

    installed = _install_stubs()
    with tempfile.TemporaryDirectory() as tmp:
        ctx = PipelineContext(
            engine="Onset",
            output_dir=Path(tmp),
            dry_run=True,
        )

        stage_visited: list[str] = []

        def make_noop(name):
            def noop(c):
                stage_visited.append(name)
            return noop

        import oxport as _ox
        original_funcs = dict(_ox.STAGE_FUNCS)
        try:
            for s in STAGES:
                _ox.STAGE_FUNCS[s] = make_noop(s)

            rc = run_pipeline(ctx, skip_stages=skip)
        finally:
            _ox.STAGE_FUNCS.update(original_funcs)
            _remove_stubs(installed)

    assert rc == 0, f"Expected return code 0, got {rc}"

    for s in skip:
        assert s not in ctx.stage_times, (
            f"Skipped stage '{s}' should NOT be in stage_times"
        )
        assert s not in stage_visited, (
            f"Skipped stage '{s}' should not have been executed"
        )

    for s in expected_run:
        assert s in ctx.stage_times, (
            f"Non-skipped stage '{s}' should be in stage_times"
        )
        assert s in stage_visited, (
            f"Non-skipped stage '{s}' should have been executed"
        )


# ---------------------------------------------------------------------------
# Test 3 — pipeline_context_defaults
# ---------------------------------------------------------------------------

def test_pipeline_context_defaults():
    """PipelineContext must expose correct derived properties and create dirs."""
    from oxport import PipelineContext

    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)

        # Drum engine detection
        ctx_drum = PipelineContext(engine="Onset", output_dir=tmp_path / "drum")
        assert ctx_drum.is_drum_engine is True, (
            "Onset should be flagged as a drum engine"
        )

        ctx_key = PipelineContext(engine="Odyssey", output_dir=tmp_path / "key")
        assert ctx_key.is_drum_engine is False, (
            "Odyssey should NOT be flagged as a drum engine"
        )

        # preset_slug without filter
        assert ctx_drum.preset_slug == "Onset", (
            f"Expected preset_slug 'Onset', got '{ctx_drum.preset_slug}'"
        )

        # preset_slug with filter
        ctx_filtered = PipelineContext(
            engine="Onset",
            output_dir=tmp_path / "filtered",
            preset_filter="808 Reborn",
        )
        assert ctx_filtered.preset_slug == "808_Reborn", (
            f"Expected '808_Reborn', got '{ctx_filtered.preset_slug}'"
        )

        # build_dir is a Path under output_dir
        assert ctx_drum.build_dir == tmp_path / "drum" / "Onset", (
            f"Unexpected build_dir: {ctx_drum.build_dir}"
        )
        assert isinstance(ctx_drum.build_dir, Path)

        # ensure_dirs creates directories when not dry_run
        ctx_real = PipelineContext(engine="Onset", output_dir=tmp_path / "real", dry_run=False)
        ctx_real.ensure_dirs()
        assert ctx_real.build_dir.exists(), "build_dir should be created by ensure_dirs"
        assert ctx_real.specs_dir.exists(), "specs_dir should be created by ensure_dirs"
        assert ctx_real.samples_dir.exists(), "samples_dir should be created by ensure_dirs"
        assert ctx_real.programs_dir.exists(), "programs_dir should be created by ensure_dirs"

        # ensure_dirs is a no-op when dry_run=True
        ctx_dry = PipelineContext(engine="Onset", output_dir=tmp_path / "dryonly", dry_run=True)
        ctx_dry.ensure_dirs()
        assert not ctx_dry.build_dir.exists(), (
            "ensure_dirs must not create directories in dry_run mode"
        )


# ---------------------------------------------------------------------------
# Test 4 — pipeline_context_resolves_engine_aliases
# ---------------------------------------------------------------------------

def test_pipeline_context_resolves_engine_aliases():
    """PipelineContext must canonicalize engine names via resolve_engine_name."""
    from oxport import PipelineContext

    with tempfile.TemporaryDirectory() as tmp:
        for raw in ["onset", "ONSET", "OnsetEngine"]:
            ctx = PipelineContext(engine=raw, output_dir=Path(tmp))
            assert ctx.engine == "Onset", (
                f"'{raw}' should resolve to 'Onset', got '{ctx.engine}'"
            )

        # An unknown name is passed through unchanged
        ctx_unknown = PipelineContext(engine="MyCustomEngine", output_dir=Path(tmp))
        assert ctx_unknown.engine == "MyCustomEngine", (
            "Unknown engine name should pass through unchanged"
        )


# ---------------------------------------------------------------------------
# Test 5 — validate_presets_subcommand_no_crash
# ---------------------------------------------------------------------------

def test_validate_presets_subcommand_no_crash():
    """cmd_validate with --presets must not raise an unhandled exception.

    validate_presets.py may or may not be importable in the test environment.
    Either outcome (ImportError printed as [ERROR] or successful run) is
    acceptable — what matters is that cmd_validate returns an int and does not
    propagate an exception.
    """
    from oxport import cmd_validate

    args = argparse.Namespace(
        output_dir=None,
        presets=True,
        fix=False,
        strict=False,
        xpn=None,
    )
    try:
        rc = cmd_validate(args)
    except SystemExit as e:
        # SystemExit is acceptable (e.g. from validate_presets calling sys.exit)
        rc = e.code if e.code is not None else 0

    assert isinstance(rc, int), (
        f"cmd_validate must return an int, got {type(rc).__name__}"
    )


# ---------------------------------------------------------------------------
# Test 6 — stage_order_is_correct (golden test)
# ---------------------------------------------------------------------------

def test_stage_order_is_correct():
    """STAGES must match the documented pipeline order exactly."""
    from oxport import STAGES

    expected = [
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

    assert STAGES == expected, (
        f"Stage order mismatch.\n"
        f"  Expected: {expected}\n"
        f"  Got:      {STAGES}"
    )


# ---------------------------------------------------------------------------
# Test 7 — dry_run_writes_no_files
# ---------------------------------------------------------------------------

def test_dry_run_writes_no_files():
    """run_pipeline in dry_run mode must write no files to the output directory."""
    from oxport import PipelineContext, run_pipeline, STAGES

    installed = _install_stubs()
    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        ctx = PipelineContext(
            engine="Onset",
            output_dir=tmp_path,
            dry_run=True,
        )

        import oxport as _ox
        original_funcs = dict(_ox.STAGE_FUNCS)
        try:
            for s in STAGES:
                _ox.STAGE_FUNCS[s] = lambda c: None  # no-op

            run_pipeline(ctx, skip_stages=set())
        finally:
            _ox.STAGE_FUNCS.update(original_funcs)
            _remove_stubs(installed)

        # Collect every file under the temp dir
        all_files = list(tmp_path.rglob("*"))
        written_files = [p for p in all_files if p.is_file()]

        assert len(written_files) == 0, (
            f"dry_run should write no files, but found: "
            f"{[str(p) for p in written_files]}"
        )


# ---------------------------------------------------------------------------
# Test 8 — cmd_status on a directory with a mock state file
# ---------------------------------------------------------------------------

def test_cmd_status_reads_state_file():
    """cmd_status must parse a .oxport_state.json and return 0."""
    from oxport import cmd_status, STAGES

    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        build_dir = tmp_path / "Onset"
        build_dir.mkdir()

        # Write a minimal state file matching _write_state output format
        state = {
            "engine": "Onset",
            "pack_name": "XO_OX Onset",
            "version": "1.0",
            "timestamp": "2026-03-29T10:00:00",
            "render_specs": 5,
            "xpm_count": 3,
            "expanded_files": 42,
            "xpn_path": None,
            "stage_times": {s: 0.01 for s in STAGES},
            "failed_stage": None,
            "stages": {s: "ok" for s in STAGES},
        }
        state_path = build_dir / ".oxport_state.json"
        with open(state_path, "w") as f:
            json.dump(state, f)

        args = argparse.Namespace(output_dir=str(tmp_path))
        rc = cmd_status(args)

    assert rc == 0, f"cmd_status should return 0 when state file exists, got {rc}"


def test_cmd_status_missing_directory_returns_nonzero():
    """cmd_status on a non-existent directory must return a non-zero exit code."""
    from oxport import cmd_status

    args = argparse.Namespace(output_dir="/tmp/oxport_e2e_nonexistent_12345")
    rc = cmd_status(args)
    assert rc != 0, (
        f"cmd_status on a missing directory should return non-zero, got {rc}"
    )


def test_cmd_status_no_state_returns_zero():
    """cmd_status on an existing-but-empty directory should return 0 (not an error)."""
    from oxport import cmd_status

    with tempfile.TemporaryDirectory() as tmp:
        args = argparse.Namespace(output_dir=tmp)
        rc = cmd_status(args)

    assert rc == 0, (
        f"cmd_status with no state files should return 0 (informational), got {rc}"
    )


# ---------------------------------------------------------------------------
# Test 9 — cmd_validate on a mock output directory
# ---------------------------------------------------------------------------

def test_cmd_validate_empty_dir_reports_failures():
    """cmd_validate on an empty directory should report no .xpm found (checks_failed > 0)."""
    from oxport import cmd_validate

    with tempfile.TemporaryDirectory() as tmp:
        args = argparse.Namespace(
            output_dir=tmp,
            presets=False,
            fix=False,
            strict=False,
            xpn=None,
        )
        rc = cmd_validate(args)

    # Empty dir has no .xpm files — validation should fail with non-zero
    assert rc != 0, (
        f"cmd_validate on an empty directory should return non-zero, got {rc}"
    )


def test_cmd_validate_nothing_to_validate():
    """cmd_validate with no --output-dir, --presets, or --xpn should return 1."""
    from oxport import cmd_validate

    args = argparse.Namespace(
        output_dir=None,
        presets=False,
        fix=False,
        strict=False,
        xpn=None,
    )
    rc = cmd_validate(args)
    assert rc == 1, (
        f"cmd_validate with nothing to validate should return 1, got {rc}"
    )


# ---------------------------------------------------------------------------
# Test 10 — context accumulates stage_times as floats
# ---------------------------------------------------------------------------

def test_stage_times_are_floats():
    """stage_times values must be non-negative floats after a pipeline run."""
    from oxport import PipelineContext, run_pipeline, STAGES

    installed = _install_stubs()
    with tempfile.TemporaryDirectory() as tmp:
        ctx = PipelineContext(engine="Onset", output_dir=Path(tmp), dry_run=True)

        import oxport as _ox
        original_funcs = dict(_ox.STAGE_FUNCS)
        try:
            for s in STAGES:
                _ox.STAGE_FUNCS[s] = lambda c: None

            run_pipeline(ctx, skip_stages=set())
        finally:
            _ox.STAGE_FUNCS.update(original_funcs)
            _remove_stubs(installed)

    for stage, t in ctx.stage_times.items():
        assert isinstance(t, float), (
            f"stage_times['{stage}'] should be float, got {type(t).__name__}"
        )
        assert t >= 0.0, (
            f"stage_times['{stage}'] should be >= 0, got {t}"
        )


# ---------------------------------------------------------------------------
# Test runner
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    tests = [
        test_dry_run_completes_all_stages,
        test_skip_stages_respected,
        test_pipeline_context_defaults,
        test_pipeline_context_resolves_engine_aliases,
        test_validate_presets_subcommand_no_crash,
        test_stage_order_is_correct,
        test_dry_run_writes_no_files,
        test_cmd_status_reads_state_file,
        test_cmd_status_missing_directory_returns_nonzero,
        test_cmd_status_no_state_returns_zero,
        test_cmd_validate_empty_dir_reports_failures,
        test_cmd_validate_nothing_to_validate,
        test_stage_times_are_floats,
    ]

    passed = 0
    failed = 0
    for t in tests:
        try:
            t()
            print(f"  PASS  {t.__name__}")
            passed += 1
        except Exception as e:
            import traceback
            print(f"  FAIL  {t.__name__}: {e}")
            traceback.print_exc()
            failed += 1

    print(f"\n{passed} passed, {failed} failed")
    sys.exit(0 if failed == 0 else 1)
