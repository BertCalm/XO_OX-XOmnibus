# XPN Oxport v2 — Stage Protocol Spec

**Date:** 2026-03-16
**Status:** Proposal
**Depends on:** `xpn_oxport_v2_architecture_rnd.md`

---

## 1. Stage Interface Definition

Every v2 pipeline stage implements the `OxportStage` protocol. This is an abstract base class
rather than a `typing.Protocol` so that missing method implementations raise at import time, not
at first execution.

```python
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from pathlib import Path
from typing import Literal

class OxportStage(ABC):

    # Identity — must be overridden as class-level attributes
    name: str        # machine identifier, e.g. "render_spec"
    description: str # one-line human summary

    @abstractmethod
    def run(self, session: "OxportSession") -> "StageResult":
        """
        Pure function contract. Reads from session.config and
        session.completed_stages. Writes only to the returned
        StageResult. Must not mutate session directly.

        Artifacts (files produced) must be written under
        session.workspace or session.output_dir — never elsewhere.
        """
        ...

    def can_skip(self, session: "OxportSession") -> bool:
        """
        Return True if this stage's outputs already exist and are
        fresh (e.g., cache hit). Default is False — always run.
        Override in stages that support incremental builds.
        """
        return False

    @property
    def requires(self) -> list[str]:
        """
        Names of stages that must have status 'pass' or 'warn'
        before this stage is eligible to run. Default: empty list.
        The runner validates this list before calling run().
        """
        return []
```

**Why ABC over Protocol:** Protocol enables duck-typing, which lets a misconfigured class slip
past registration. ABC raises `TypeError` at instantiation if any abstract method is missing —
fail-fast behavior matches the pipeline's correctness requirements.

---

## 2. StageResult Type

```python
@dataclass
class StageResult:
    status:     Literal["pass", "warn", "fail", "skip"]
    message:    str
    artifacts:  list[Path]      = field(default_factory=list)
    warnings:   list[str]       = field(default_factory=list)
    elapsed_ms: int             = 0

    @property
    def ok(self) -> bool:
        return self.status in ("pass", "warn", "skip")
```

- `pass` — stage completed, all artifacts produced, no issues.
- `warn` — stage completed with non-fatal problems; downstream stages may proceed.
- `fail` — stage could not complete; downstream stages that `requires` this stage are blocked.
- `skip` — `can_skip()` returned True; artifacts from a prior run are treated as current.

`artifacts` is the authoritative list of files this stage produced. The runner logs all paths;
downstream stages that need them read from `session.completed_stages["stage_name"].artifacts`
rather than guessing paths themselves.

---

## 3. OxportSession Interface

Stages receive one argument: the session. The session is intentionally narrow — stages cannot
reach outside it to mutate global state.

```python
@dataclass
class OxportSession:
    session_id:       str               # UUID
    config:           PackConfig        # frozen — read-only by definition
    completed_stages: dict[str, StageResult]  # read-only from stage perspective
    logger:           "StageLogger"
    workspace:        Path              # session-scoped temp dir; cleaned on success
    output_dir:       Path              # final delivery location
```

**`config: PackConfig`** — frozen dataclass. Stages read `session.config.engine`,
`session.config.preset_names`, etc. They cannot write to it. This replaces v1's pattern
of stages appending to a shared `PipelineContext` dict.

**`completed_stages`** — stages look up prior results here. Example: `export` stage reads
`session.completed_stages["expand"].artifacts` to find the rendered WAV paths. The runner
populates this dict as each stage completes; it is read-only from inside `run()`.

**`logger: StageLogger`** — structured logging interface with `.info()`, `.warn()`, `.error()`.
Writes to both console and a session-scoped log file. Replaces bare `print()` calls from v1.

```python
class StageLogger(Protocol):
    def info(self, msg: str) -> None: ...
    def warn(self, msg: str) -> None: ...
    def error(self, msg: str) -> None: ...
```

**`workspace: Path`** — temp directory created per session. Intermediate artifacts live here.
The runner deletes it on successful completion; it survives on failure for post-mortem inspection.

**`output_dir: Path`** — where the final `.xpn` bundle lands. Stages that produce final
deliverables (package, manifest) write here.

---

## 4. The 10 Canonical Stages

Each maps to one v1 `_stage_*` function. The class name is the stable import target.

| Stage Class         | `name`             | `requires`       | Primary Artifact              |
|---------------------|--------------------|------------------|-------------------------------|
| `RenderSpecStage`   | `render_spec`      | —                | `render_spec.json`            |
| `CategorizeStage`   | `categorize`       | `render_spec`    | `categories.json`             |
| `ExpandStage`       | `expand`           | `categorize`     | rendered `.wav` files         |
| `QAStage`           | `qa`               | `expand`         | `qa_report.json`              |
| `ExportStage`       | `export`           | `qa`             | `.xpm` keygroup files         |
| `CoverArtStage`     | `cover_art`        | `render_spec`    | `cover.png`                   |
| `ComplementChain`   | `complement_chain` | `export`         | complement `.xpm` files       |
| `PackageStage`      | `package`          | `export`, `cover_art` | `.xpn` archive           |
| `ManifestStage`     | `manifest`         | `package`        | `manifest.json`               |
| `ValidateStage`     | `validate`         | `manifest`       | `validation_report.json`      |

**Key dependency notes:**

- `cover_art` depends only on `render_spec` (needs engine + mood metadata, not WAV files) so
  it can run in parallel with `expand` in a future parallel executor.
- `package` requires both `export` and `cover_art` — it assembles the final bundle.
- `validate` is the only stage that reads the finished `.xpn` — it confirms MPC format
  compliance (KeyTrack, RootNote, VelStart rules) on the delivered file, not the source XPMs.

---

## 5. Stage Registration and Execution Order

**Recommendation: ordered list, not a dependency DAG.**

The 10 stages have a near-linear dependency structure. A DAG executor adds real complexity
(topological sort, cycle detection, parallel scheduling) that the current stage count does not
justify. An ordered list where the runner validates `requires` before each execution gives
correctness without the overhead.

```python
PIPELINE: list[type[OxportStage]] = [
    RenderSpecStage,
    CategorizeStage,
    ExpandStage,
    QAStage,
    ExportStage,
    CoverArtStage,
    ComplementChain,
    PackageStage,
    ManifestStage,
    ValidateStage,
]
```

The runner iterates this list in order:

1. Check `stage.requires` — if any required stage has `status == "fail"`, skip this stage with
   `status = "fail"` and message `"blocked by failed dependency: {name}"`.
2. Call `stage.can_skip(session)` — if True, record a `skip` result and continue.
3. Call `stage.run(session)` inside a try/except — uncaught exceptions become `fail` results
   with the traceback in `message`.
4. Append result to `session.completed_stages`.

**When to graduate to a DAG:** If any two stages become genuinely independent (no shared
artifact path) and their combined runtime exceeds 30 seconds, a parallel DAG executor pays for
itself. The most likely candidate is `cover_art` running concurrently with `expand`. At that
point, replace the ordered list with a topological sort over the `requires` graph — the stage
interface does not change, only the runner.

**Stage discovery:** Stages are registered by direct import into `PIPELINE`. No plugin scanning,
no filesystem discovery. Adding a stage means editing one list in `oxport_pipeline.py`. This is
intentional — implicit discovery hides the execution order, which is the most important thing
to understand when debugging a failed build.
