# Oxport v2 Architecture — R&D Document

**Date**: 2026-03-16
**Status**: Planning / Pre-implementation
**Scope**: Architecture design for the Oxport tool suite as tool count scales past ~50

---

## 1. Current Architecture Assessment

The current suite is 49 `xpn_*.py` scripts plus `oxport.py` as an orchestrator, all living flat in `Tools/`. This worked well at 10–15 tools. At 49, and with more generators incoming, the limitations are becoming structural.

**Discovery problem.** There is no registry. `oxport.py` must hard-code which scripts exist and in what order they run. Every new tool requires editing the orchestrator. A contributor adding `xpn_new_thing.py` has no standard contract for how it gets discovered or invoked.

**Duplicate logic.** WAV loading, XPM XML parsing, sample-rate detection, instrument classification, and pad-note mapping appear in multiple scripts with minor variations. Each copy drifts independently. A bug fix in `xpn_normalize.py`'s WAV loader does not fix the same logic in `xpn_smart_trim.py`.

**No shared state between tools.** When `oxport build` runs a full pipeline, each script re-reads the same files from disk and re-parses the same XPM XML. There is no in-memory session object passed between stages. This is slow and means each tool sees a slightly different view of the project if files change mid-run.

**Import hell.** Scripts that want to call each other currently use `subprocess` or copy-paste code. There is no importable package. `from xpn_classify_instrument import classify` does not work without adding `Tools/` to `sys.path` manually.

**Testing surface.** Flat scripts with `if __name__ == "__main__"` blocks and hard-coded `argparse` setups are difficult to unit-test. Logic is tangled with CLI parsing. There is no test runner that can import and exercise individual functions.

**Namespace collisions incoming.** The `xpn_` prefix is doing all the organizational work. Once the kit generators (attractor, CA, braille, climate, entropy, gene, git, gravitational wave, lsystem, pendulum, poetry, seismograph, transit, turbulence) are joined by Artwork Collection and Travel Collection generators, distinguishing generator tools from analysis tools from export tools becomes impossible by prefix alone.

---

## 2. Proposed v2 Module Structure

```
Tools/oxport/
  __init__.py           # version, public API surface
  core/
    __init__.py
    wav_io.py           # read_wav, write_wav, detect_sample_rate
    xpm_parser.py       # parse_xpm, build_xpm, XPMProgram dataclass
    classify.py         # instrument_family, is_melodic, root_from_filename
    project.py          # OxportSession — shared in-memory project state
  analysis/
    __init__.py
    stems_checker.py    # from xpn_stems_checker
    kit_completeness.py # from xpn_kit_completeness
    auto_root.py        # from xpn_auto_root_detect
    optic_fingerprint.py
  transform/
    __init__.py
    normalize.py        # from xpn_normalize
    smart_trim.py       # from xpn_smart_trim
    adaptive_velocity.py
    tuning_systems.py
  generate/
    __init__.py
    variation_generator.py
    cover_art.py        # from xpn_cover_art + xpn_cover_art_batch
    evolution_builder.py
    liner_notes.py
    preview_generator.py
    # kit generators: attractor, braille, ca_presets, climate, entropy...
  export/
    __init__.py
    drum_export.py      # from xpn_drum_export
    keygroup_export.py
    bundle_builder.py
    packager.py
    submission_packager.py
    render_spec.py
  validate/
    __init__.py
    validator.py        # from xpn_validator
    qa_checker.py       # from xpn_qa_checker
  plugins/              # third-party / community extensions
    __init__.py         # plugin loader
  cli.py                # unified entry point
```

**core/** is the dependency-free foundation. Everything else imports from here; nothing here imports from siblings. `OxportSession` in `project.py` is the shared state object passed between pipeline stages — it holds parsed XPM programs, loaded WAV buffers, and metadata accumulated during a run.

**analysis/** tools are read-only: they inspect and report. They return structured data, not side effects.

**transform/** tools mutate audio or XPM data in memory. They operate on an `OxportSession` and return a modified session, never write to disk directly.

**generate/** tools produce new content — XPM programs, artwork, liner notes, preview audio. Kit generators (attractor, braille, CA, etc.) live here as submodules.

**export/** tools write final artifacts to disk: ZIP bundles, XPN packages, render specs.

**validate/** tools check correctness and report issues. Always the last stage before export.

---

## 3. Unified CLI

Replace 49 separate `python xpn_*.py` invocations with one entry point:

```
oxport [command] [options] [path]
```

Core commands:

```
oxport build    [path]   # full pipeline: analyze → transform → generate → validate → export
oxport analyze  [path]   # stems_checker + kit_completeness + auto_root
oxport transform [path]  # normalize + smart_trim + adaptive_velocity
oxport validate [path]   # validator + qa_checker
oxport generate [path]   # variation_generator, cover_art, liner_notes
oxport art      [path]   # cover art only (fast iteration)
oxport list             # show all registered commands including plugins
```

`cli.py` uses `argparse` with subparsers. Each submodule exposes a `register_subcommand(subparsers)` function that adds its own subcommand, flags, and help text. `cli.py` loops over all submodules and plugins calling `register_subcommand`. No hard-coding of command names in the orchestrator.

Common flags live at the top level: `--dry-run`, `--verbose`, `--output-dir`, `--config`.

---

## 4. Plugin Architecture

Community contributors should be able to add new generators or analyzers without touching core files.

Scan `Tools/oxport/plugins/` for `*.py` files at startup. For each module found, call `register(session, subparsers)` if the function exists. The `register` function adds CLI subcommands and registers any pipeline hooks.

A minimal plugin looks like:

```python
# Tools/oxport/plugins/my_generator.py
def register(session_class, subparsers):
    p = subparsers.add_parser("my-gen", help="Generate X from Y")
    p.set_defaults(func=run)

def run(args, session):
    ...
```

This is the simplest viable contract: one file, one function, zero framework dependency. Plugins import from `oxport.core` for shared utilities. They do not modify any file outside their own module.

---

## 5. Migration Path

No existing workflow should break during migration. The strategy: thin wrapper scripts that delegate to the package.

Each existing `xpn_*.py` becomes a three-line shim:

```python
# Tools/xpn_normalize.py  (legacy shim)
from oxport.transform.normalize import main
if __name__ == "__main__": main()
```

`oxport.py` (the current orchestrator) becomes a shim that calls `oxport.cli.main()`. All existing shell scripts, Makefile targets, and documented workflows continue to work unchanged.

Migration is incremental: move one tool at a time into the package, update its shim, verify nothing broke. No flag-day cutover required.

---

## 6. Sonnet vs Opus Work Split

**Mechanical refactoring (Sonnet + Medium):**
- Moving script code into submodules without changing logic
- Writing wrapper shims for backward compatibility
- Wiring `register_subcommand()` boilerplate
- Writing the plugin loader (simple directory scan)
- Extracting duplicate WAV/XPM parsing into `core/`

**Architectural decisions requiring judgment (Opus + High):**
- Designing the `OxportSession` dataclass — what state lives there, what is lazily loaded, how transforms chain
- Deciding the contract between analysis/transform/generate/export stages (what each returns, what errors look like)
- Plugin API design — any complexity beyond the minimal `register()` pattern
- Determining whether `core/` should be a separate installable package for community use
- Performance: which stages benefit from parallel execution, how to avoid re-parsing shared data

The refactoring is large but mostly mechanical. The session object and stage contracts are the load-bearing architectural decisions — get those wrong and the package will be as brittle as the flat scripts.

---

## Next Steps

1. Design `OxportSession` dataclass (Opus decision)
2. Extract `core/wav_io.py` and `core/xpm_parser.py` from most-used existing tools
3. Migrate `xpn_validator.py` and `xpn_qa_checker.py` into `validate/` as first full submodule
4. Wire `cli.py` with the `list` and `validate` commands as proof of concept
5. Migrate remaining tools incrementally, one submodule at a time
