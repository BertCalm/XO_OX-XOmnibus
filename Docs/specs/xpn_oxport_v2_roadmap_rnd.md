# Oxport v2 — Unified Session Pipeline: R&D Spec

**Date**: 2026-03-16
**Status**: R&D / Pre-Build
**Owner**: XO_OX Tools
**Audience**: Internal — Tools team, future automation work

---

## Overview

Oxport (`Tools/oxport.py`) is the XPN export orchestrator. It works. It has shipped every pack. But it was designed as a script runner, not a pipeline — and the seams are starting to show as the engine count grows and collection workflows diverge.

This spec designs Oxport v2: a session-based, stage-pluggable, resumable export pipeline with a clean Python API and Rich terminal output. v2 is not an emergency. It is the right architecture to build before the current model becomes a bottleneck.

---

## 1. Current Limitations

The trigger for v2 is when any of these costs become unacceptable:

| Limitation | Current Behavior | Cost |
|-----------|-----------------|------|
| Filesystem state | Temp dirs passed between ~65 scripts | Race conditions possible in parallel runs; debugging requires inspecting tmp/ |
| No session persistence | Interrupted export = full restart | Painful for 150-preset packs with render steps |
| No Python API | CLI only — C++ automation requires subprocess with flag parsing | Brittle; breaks on flag changes |
| No stage plugins | Custom collection stages require forking oxport.py | Artwork ComplementChain, Kitchen RecipeMap, etc. need custom logic |
| No progress UI | `print()` statements only | No timing data; hard to know which stage is slow |
| No parallel stages | Sequential only | cover_art + manifest_generator have no dependency on each other |

None of these are bugs. They are design debts that v2 pays off.

---

## 2. OxportSession Design

The core class. Holds all pipeline state, resolves configuration, and exposes both programmatic and CLI entry points.

```python
class OxportSession:
    def __init__(self, session_id: str | None = None):
        self.session_id: str          # UUID, generated if not provided
        self.engine: str              # e.g. "OPAL", "ONSET"
        self.wavs_dir: Path
        self.output_dir: Path
        self.stages: list[Stage]      # ordered stage list
        self.results: dict[str, StageResult]
        self.config: SessionConfig    # dry_run, skip_stages, parallel_groups
        self._state_path: Path        # output_dir/.oxport_session.json

    def configure(self, engine: str, wavs_dir: str | Path, output_dir: str | Path,
                  *, dry_run: bool = False, skip: list[str] | None = None) -> None: ...

    def add_stage(self, stage: Stage, *, after: str | None = None) -> None: ...

    def run_stage(self, stage_name: str) -> StageResult: ...

    def run_all(self) -> SessionSummary: ...

    def resume(self) -> SessionSummary: ...

    def export_report(self, fmt: str = "text") -> str: ...

    def save_state(self) -> None: ...

    @classmethod
    def load(cls, state_path: str | Path) -> "OxportSession": ...
```

### SessionConfig

```python
@dataclass
class SessionConfig:
    dry_run: bool = False
    skip_stages: list[str] = field(default_factory=list)
    parallel_groups: bool = True      # allow concurrent independent stages
    verbosity: int = 1                # 0=silent, 1=default, 2=verbose
    samplerate: int = 44100
    bit_depth: int = 24
```

### Key design decisions

- `configure()` validates paths and raises `OxportConfigError` before any stage runs. No silent failures.
- `run_stage()` is idempotent when the stage supports it (cover_art, manifest). Export stages are not idempotent by default — they skip if output already exists unless `--force` is set.
- `export_report()` returns structured text or JSON. JSON mode enables C++ subprocess parsing.

---

## 3. Stage Plugin Architecture

Every pipeline step becomes a `Stage` subclass. Stages are self-describing, registerable, and composable.

```python
@dataclass
class StageResult:
    stage_name: str
    success: bool
    elapsed_s: float
    files_written: list[Path]
    warnings: list[str]
    errors: list[str]
    metadata: dict          # stage-specific output (e.g. xpm_count, cover_hash)

class Stage:
    name: str               # registry key, e.g. "export", "cover_art"
    description: str
    depends_on: list[str]   # stage names that must complete first

    def run(self, ctx: StageContext) -> StageResult: ...
    def can_skip(self, ctx: StageContext) -> bool: ...    # idempotency check
```

### StageContext

Replaces the temp-dir filesystem handoff. All inter-stage data lives here.

```python
@dataclass
class StageContext:
    session: OxportSession
    wavs: list[Path]
    xpm_paths: list[Path]
    manifest: dict | None
    cover_art_path: Path | None
    scratch_dir: Path          # still exists for tools that genuinely need disk
    logger: OxportLogger
```

### STAGE_REGISTRY

```python
STAGE_REGISTRY: dict[str, type[Stage]] = {}

def register_stage(cls: type[Stage]) -> type[Stage]:
    STAGE_REGISTRY[cls.name] = cls
    return cls

@register_stage
class ExportStage(Stage):
    name = "export"
    depends_on = []
    ...

@register_stage
class ManifestStage(Stage):
    name = "manifest"
    depends_on = ["export"]
    ...

@register_stage
class CoverArtStage(Stage):
    name = "cover_art"
    depends_on = []          # independent — can run in parallel with export
    ...

@register_stage
class PackagerStage(Stage):
    name = "packager"
    depends_on = ["export", "manifest", "cover_art"]
    ...
```

### Custom collection stages

Collection-specific workflows register custom stages without forking oxport.py:

```python
# In Tools/collections/artwork/complement_chain.py
@register_stage
class ComplementChainStage(Stage):
    name = "complement_chain"
    description = "Build color-complement velocity crossfade chains for Artwork engines"
    depends_on = ["export"]

    def run(self, ctx: StageContext) -> StageResult:
        ...
```

The session loads custom stages on demand:

```python
session.configure("OXBLOOD", wavs_dir, output_dir)
session.add_stage(ComplementChainStage(), after="export")
session.run_all()
```

---

## 4. Python API for Programmatic Use

The first-class use case for v2 is import-and-call, not just CLI invocation.

```python
from oxport import OxportSession

session = OxportSession()
session.configure(
    engine="OPAL",
    wavs_dir="/path/to/wavs",
    output_dir="/path/to/out",
    dry_run=False,
    skip=["cover_art"]
)
summary = session.run_all()
print(summary.total_files_written)
print(summary.elapsed_s)
report = session.export_report(fmt="json")
```

### C++ automation via subprocess

XOlokun render automation can call Oxport programmatically without parsing print output:

```cpp
// In a worker thread — never the audio thread
auto result = juce::ChildProcess();
result.start("python3 -m oxport --engine OPAL --wavs /tmp/render --out /tmp/packs --report json");
auto json = juce::JSON::parse(result.readAllProcessOutput());
```

The `--report json` flag outputs a single JSON blob on stdout (all other output goes to stderr). This is a stable contract — flag and schema are versioned.

---

## 5. Parallel Stage Execution

Stages declare their dependencies via `depends_on`. The session runner builds a DAG and executes independent stages concurrently.

```
export ─────────────────────────┬── manifest ──┬── packager
                                │              │
cover_art (independent) ────────┘              │
                                               │
sample_categorizer (independent) ──────────────┘
```

Implementation uses `concurrent.futures.ThreadPoolExecutor`. Stage count is small (< 12), so process overhead from `ProcessPoolExecutor` is unnecessary. IO-bound stages (file writes, subprocess calls) benefit from threading.

```python
def _run_parallel_group(self, stages: list[Stage], ctx: StageContext) -> list[StageResult]:
    with ThreadPoolExecutor(max_workers=min(len(stages), 4)) as pool:
        futures = {pool.submit(stage.run, ctx): stage for stage in stages}
        return [f.result() for f in as_completed(futures)]
```

Stages that share `scratch_dir` writes must declare a mutex key in their metadata. The runner enforces this at registration time, not at runtime — fail early.

---

## 6. Progress UI (stdlib-only)

No `rich` dependency. ANSI escape codes via `sys.stdout` only. Rationale: oxport runs in CI, in subprocess from C++, and on machines that may not have `rich` installed. Stdlib is always available.

```
Oxport v2 — OPAL  ·  2026-03-16 14:23
─────────────────────────────────────────────────────
  [✓]  export          3.2s   147 XPM files written
  [✓]  cover_art       1.1s   1 PNG generated
  [✓]  manifest        0.4s   manifest.json written
  [~]  complement_chain  running…  (12/147 processed)
  [ ]  packager        waiting
─────────────────────────────────────────────────────
  Total elapsed: 4.7s  |  ETA: ~18s
```

The spinner and file count update in place using `\r` writes. The logger class:

```python
class OxportLogger:
    def stage_start(self, name: str) -> None: ...
    def stage_progress(self, name: str, current: int, total: int) -> None: ...
    def stage_complete(self, name: str, result: StageResult) -> None: ...
    def summary_table(self, summary: SessionSummary) -> None: ...
    def set_verbosity(self, level: int) -> None: ...
```

Verbosity 0 suppresses all output except errors (for CI/subprocess use). Verbosity 2 prints per-file lines.

---

## 7. Session Persistence and Resume

On `session.run_all()`, state is written to `{output_dir}/.oxport_session.json` after each stage completes.

```json
{
  "session_id": "a3f9c12e",
  "engine": "OPAL",
  "wavs_dir": "/path/to/wavs",
  "output_dir": "/path/to/out",
  "config": { "dry_run": false, "skip_stages": [] },
  "completed_stages": ["export", "cover_art"],
  "results": {
    "export": { "success": true, "elapsed_s": 3.2, "files_written": [...] }
  },
  "created_at": "2026-03-16T14:23:00Z",
  "updated_at": "2026-03-16T14:23:04Z"
}
```

Resume from CLI:

```bash
python3 -m oxport --resume /path/to/out/.oxport_session.json
```

Resume from API:

```python
session = OxportSession.load("/path/to/out/.oxport_session.json")
session.resume()
```

Resume skips completed stages (by checking `completed_stages` list) and picks up from the first incomplete stage. If a stage's output files are missing despite being in `completed_stages`, it is re-run automatically and a warning is emitted.

---

## 8. CLI Backward Compatibility

v2 CLI flags must be a superset of v1 flags. No breaking changes at the command line.

| v1 Flag | v2 Equivalent | Notes |
|---------|--------------|-------|
| `--engine ENGINE` | `--engine ENGINE` | Unchanged |
| `--wavs WAVS_DIR` | `--wavs WAVS_DIR` | Unchanged |
| `--out OUT_DIR` | `--out OUT_DIR` | Unchanged |
| `--skip STAGE` | `--skip STAGE` | Unchanged; can be repeated |
| `--dry-run` | `--dry-run` | Unchanged |
| `--samplerate N` | `--samplerate N` | Unchanged |
| _(new)_ | `--resume PATH` | Resume interrupted session |
| _(new)_ | `--report json\|text` | Output format for programmatic use |
| _(new)_ | `--parallel / --no-parallel` | Control concurrent stage execution |
| _(new)_ | `--add-stage MODULE.CLASS` | Register a custom stage at runtime |

The v1 entry point (`Tools/oxport.py`) is preserved as a thin shim that constructs an `OxportSession` and calls `run_all()`. Existing shell scripts and Makefile invocations continue to work.

---

## 9. Migration Path

### Phase 1 — No user-facing changes

Refactor `oxport.py` internals to use `OxportSession` and `Stage` classes while keeping the CLI identical. All ~65 scripts become Stage subclasses. Tests confirm output parity.

### Phase 2 — Python API stabilizes

Publish `from oxport import OxportSession` as the primary interface. Add `--report json` flag. C++ automation can start using it.

### Phase 3 — Parallel execution + resume

Enable parallel stage groups. Add session persistence. Custom stage registration exposed.

### Phase 4 — Collection integrations

Artwork `ComplementChainStage`, Kitchen `RecipeMapStage`, Travel `FXChainStage` registered from their respective collection modules. Oxport becomes the shared pipeline substrate.

---

## 10. When to Build

Do not build v2 until current oxport.py is the bottleneck. Specific triggers:

1. **An interrupted export causes a full re-run** that takes > 10 minutes — resume is now worth the build time.
2. **A collection requires a custom stage** that cannot be cleanly expressed as a script called from oxport.py — plugin architecture is now worth the build time.
3. **C++ render automation needs to call Oxport** and filesystem handoff becomes error-prone — Python API is now worth the build time.
4. **A parallel stage would save > 30 seconds per pack** and packs are being built daily — parallel execution is now worth the build time.

Until at least two of these triggers are true, oxport.py is the right tool.

---

## 11. Effort Estimate

| Phase | Effort | Model |
|-------|--------|-------|
| Phase 1: Refactor to Stage classes | 4–6 hours | Sonnet + Medium |
| Phase 2: Python API + JSON report | 2–3 hours | Sonnet + Medium |
| Phase 3: Parallel + resume | 3–4 hours | Sonnet + Medium |
| Phase 4: Collection integrations | 1–2 hours per collection | Sonnet + Low |

Total: ~12–16 hours of implementation. Design (this doc) is the Opus-level investment. Implementation is Sonnet work.

---

## References

- `Tools/oxport.py` — current orchestrator (CLI entry point)
- `Tools/` — ~65 Python scripts that become Stage subclasses in v2
- `Docs/specs/xpn_tools_architecture.md` — current XPN tool suite overview
- `Source/Export/` — C++ export hooks (future consumer of Python API)
