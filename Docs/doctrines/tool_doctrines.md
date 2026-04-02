# Tool Doctrines (T001–T006)

> Ratified 2026-03-18 by The Architect (Raj). Applies to all Python tools in `Tools/`.
> Analogous to the 6 Engine Doctrines (D001–D006) that govern synth engine quality.

## The 6 Tool Doctrines

| ID | Doctrine | Summary |
|----|----------|---------|
| **T001** | **Input Validation Is the Contract** | Every tool validates all inputs before processing. Missing files, malformed JSON, out-of-range values fail early with clear error messages — never silently proceed. |
| **T002** | **Idempotency or Explicit Mutation** | A tool is either idempotent (safe to run 1000×, same result) OR explicitly destructive (named `*_fixer`/`*_rebuild`, requires `--confirm`). No silent partial mutations. |
| **T003** | **Output Format Is Part of the Product** | Tools produce output that downstream tools can consume. Format must be schema-validated. Format drift breaks the pipeline. |
| **T004** | **Composability: Tools Chain Into Pipelines** | No hardcoded paths, engine lists, or mood categories. All must accept `--input-dir`, `--output-dir`, `--filter` flags. Useful standalone AND in oxport pipelines. |
| **T005** | **Fleet Scale: Handle 11,000+ Presets** | <1 min for fleet-wide analysis, <5 min for fleet-wide mutation. Memory scales linearly, not quadratically. No loading entire fleet into memory unless necessary. |
| **T006** | **Failure Is Observable** | All errors logged with context (file path, error type, expected vs actual). No swallowed exceptions. Exit codes: 0=success, 1=warning, 2=error. Pipeline failures identify *which* tool failed. |

## Enforcement

- **T001**: `--strict` flag required for CI runs; malformed inputs must produce specific error messages
- **T002**: Destructive tools require `--confirm` flag; dry-run mode available for all mutators
- **T003**: Output validation against JSON schema; pipeline integration tests verify chaining
- **T004**: Oxport integration tests; tool metadata (inputs/outputs) in TOOL_REGISTRY
- **T005**: Benchmark suite: wall-clock time + peak memory for 11K presets; flag tools exceeding thresholds
- **T006**: Required `--log` output; error messages include input file + expected + actual; stack traces in `--verbose`

## Initial Tool Blessings

| ID | Tool | Recognition |
|----|------|-------------|
| **B_T001** | `xpn_fleet_dna_diversity_report.py` | **Diagnostic Excellence** — asks *why* diversity is low, delivers actionable statistical analysis |
| **B_T002** | `compute_preset_dna.py` | **Core Intelligence** — 6D Sonic DNA fingerprinting with 40+ engine-specific extractors |
| **B_T003** | `breed_presets.py` | **Genetic Sound Design** — crossover + mutation on DNA vectors. No other synth has this. |
| **B_T004** | `validate_presets.py` | **Fleet Integrity Guardian** — 10-point validation, auto-fix, CI-ready |
| **B_T005** | `oxport.py` | **Pipeline Orchestrator** — 8-stage chain, lazy imports, context-aware routing |

## Tool Seance Process

Adapted from the Synth Seance model:

1. **Registry & Baseline** — create `TOOL_REGISTRY.json`, document all tool inputs/outputs
2. **Doctrine Audit** — review cohort of 30 high-impact tools against T001–T006
3. **Blessing Voting** — tools achieving all 6 doctrines nominated for blessings
4. **Repair Sprint** — fix doctrine violations, prioritize pipeline-blocking tools
5. **CI Integration** — automatic `oxport validate` on preset PRs; new tools auto-scored at merge
