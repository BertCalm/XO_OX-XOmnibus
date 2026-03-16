# XPN Oxport v2 Architecture — R&D Spec

**Date:** 2026-03-16
**Status:** Proposal
**Author:** XO_OX Designs

---

## 1. What's Wrong with Oxport v1

Oxport v1 ships as `Tools/oxport.py` — a single-file orchestrator that runs eight pipeline stages
in sequence via a `PipelineContext` object. It works, but it has structural problems that are
becoming harder to ignore as the pack catalog grows.

**PipelineContext is a mutable bag.** The class holds `render_specs`, `categories`,
`expanded_files`, `xpm_paths`, `cover_paths`, and `xpn_path` all in one flat namespace.
Any stage can write any field at any time. Stage 3 (`expand`) can accidentally overwrite
a key that stage 2 (`categorize`) set, and there is no contract preventing it. As stages
multiply — complement chains, tuning system overlays, Optic integration — the bag grows
and the implicit dependencies between stages become harder to track.

**Stage failures leave partial build artifacts.** When stage 6 (`cover_art`) throws an
exception, the `build_dir` tree already contains stage 1–5 outputs. There is no cleanup
pass. The next run either collides with stale artifacts or the user manually wipes the
directory. Neither is acceptable at production volume.

**No resume capability.** A crash at stage 7 (`complement_chain`) means rerunning from
stage 1. Stages 1–3 can take several minutes when rendering large preset sets. Forcing a
full restart every time a downstream step fails is a real workflow tax.

**Difficult to test stages in isolation.** Because stages are private functions
(`_stage_render_spec`, `_stage_categorize`, etc.) that mutate a shared `PipelineContext`,
there is no clean way to unit-test a single stage. You must construct a full context and
run the surrounding pipeline to observe outputs.

**No structured logging.** Every stage communicates via `print()`. Machine-readable output
for CI — exit codes, timing, artifact paths — requires post-processing the terminal stream.
There is no log file written by default.

**Config is CLI-only.** Oxport v1 accepts `--engine`, `--wavs-dir`, `--output-dir`,
`--preset`, `--kit-mode`, `--version`, and several more flags. Reproducing a specific build
requires re-typing or scripting all flags. A declarative config file would make builds
repeatable and version-controllable alongside the presets they describe.

---

## 2. OxportSession Model

The core v2 abstraction is `OxportSession` — a serializable, session-scoped object that
travels through the pipeline instead of a mutable bag.

```python
@dataclass
class OxportSession:
    session_id: str          # UUID for this build
    pack_config: PackConfig  # frozen dataclass — immutable once created
    build_dir: Path          # session-scoped temp directory
    stage_results: dict      # stage_name → StageResult
    created_at: datetime
```

`PackConfig` is a frozen dataclass so stages cannot mutate the build specification:

```python
@dataclass(frozen=True)
class PackConfig:
    pack_name: str
    engine: str
    mood: str
    version: str
    preset_names: list[str]
    tier: str               # signal | form | doctrine
```

Separation of concerns is explicit: `PackConfig` holds what to build; `OxportSession` tracks
how the build is going. Stages receive a session, read what they need from `pack_config`,
and write results only to `stage_results[their_name]`. There is no shared namespace to
accidentally write into.

---

## 3. Stage Protocol

Each stage is a pure function with a typed signature:

```python
def stage_render(session: OxportSession) -> StageResult:
    t0 = time.monotonic()
    # ... do work, read from session.pack_config ...
    return StageResult(
        stage="render",
        status="pass" | "warn" | "fail",
        artifacts=[paths],
        warnings=[],
        errors=[],
        duration_s=time.monotonic() - t0,
    )
```

`StageResult` is a frozen dataclass. Stages do not mutate session state directly — the
runner does, writing `session.stage_results[result.stage] = result` after each call.
This means every stage is independently invokable in a test:

```python
session = make_test_session(engine="ONSET", preset_names=["808 Reborn"])
result = stage_render(session)
assert result.status == "pass"
assert len(result.artifacts) > 0
```

No surrounding pipeline required.

---

## 4. Resume Capability

After each stage, the runner serializes the session to `.oxport_session.json` in
`build_dir`. Serialization uses `dataclasses.asdict()` with `Path` → `str` conversion;
deserialization reconstructs `Path` objects from strings.

On restart with `--resume SESSION_ID`, the runner:

1. Locates `build_dir/SESSION_ID/.oxport_session.json`
2. Deserializes the session, restoring `stage_results`
3. Skips any stage whose result already has `status == "pass"`
4. Resumes from the first non-passing stage

Partial artifacts in `build_dir` are intentionally retained — the resume mechanism relies
on them. Final cleanup (moving artifacts to `output_dir` and deleting `build_dir`) happens
only on full pipeline success or explicit `oxport_v2.py cleanup SESSION_ID`.

This means a crash at stage 7 costs only the time to rerun stages 7–8, not stages 1–8.
At 200-preset ONSET builds, that difference is significant.

---

## 5. Structured Logging

`StageLogger` replaces all `print()` calls. It writes two streams simultaneously:

**JSON lines** to `build_dir/oxport.log`:
```json
{"ts": "2026-03-16T14:22:01Z", "stage": "render", "level": "info", "msg": "Found 45 presets"}
{"ts": "2026-03-16T14:22:01Z", "stage": "render", "level": "warn", "msg": "Missing tuning: Tide Kit 1"}
```

**Human-readable** to terminal, controlled by `--verbosity`:
- `quiet` — errors only
- `normal` (default) — stage headers + warnings + final summary
- `verbose` — every log line

CI environments detect `--verbosity=quiet` and parse the JSON log for structured results.
The log file always captures everything regardless of terminal verbosity.

`StageLogger` is injected into stages as a second argument:

```python
def stage_render(session: OxportSession, log: StageLogger) -> StageResult:
    log.info("Found %d presets", len(presets))
```

This keeps stages testable — pass a `NullLogger` in tests, a real `StageLogger` in
production.

---

## 6. Declarative Config File

Instead of re-typing CLI flags, pack builders define a `pack.yml` alongside their presets:

```yaml
name: "Machine Gun Reef"
engine: ONSET
mood: Foundation
tier: form
version: 1.0.0
presets:
  - "Tide Kit 1"
  - "Pressure Break"
  - "808 Reborn"
  - "Snare City"
kit_mode: smart
tuning: null
choke_preset: onset
```

Running `oxport_v2.py run pack.yml` is fully reproducible. The same `pack.yml` committed
to the repo produces the same `.xpn` output on any machine and on any CI runner. No shell
scripts to maintain, no flag re-typing, no "what version did I use last time?" archaeology.

`PackConfig` is constructed by parsing `pack.yml` via `PyYAML` and validating required
fields. Unknown keys are rejected with a clear error rather than silently ignored.

---

## 7. Migration Path

Oxport v1 (`Tools/oxport.py`) continues to work without modification. No breaking changes
are introduced to the v1 CLI.

Oxport v2 ships as `Tools/oxport_v2.py` with an additive interface. The `OxportSession`
and `StageResult` types are importable from `Tools/oxport_session.py` — usable by other
tools in the suite without importing the full runner.

During the parallel period (6 months from the v2 release date), v1 runs will print a
single deprecation notice:

```
[oxport] oxport.py v1 is deprecated. Use oxport_v2.py with a pack.yml config.
         See Tools/MIGRATION.md for the upgrade guide.
```

The notice does not block execution. After the parallel period, v1 will be archived to
`Tools/archive/oxport_v1.py` and removed from the default `PATH` alias in the project's
`Makefile`.

The migration for an existing pack is mechanical: create a `pack.yml` from the flags in
the last `oxport.py run ...` invocation. `Tools/migrate_to_v2.py` will automate this
by parsing a shell history file or a v1 `--dry-run` trace.

---

## Summary

Oxport v2 solves four concrete problems: stage isolation via typed `StageResult` returns,
failure recovery via serialized session state, reproducibility via `pack.yml`, and
observability via `StageLogger`. It is not a rewrite — it is a layering of structure
on top of the same eight stage functions. The v1 path stays live through the transition,
and the migration for any given pack is a ten-minute task.
