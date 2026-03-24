# /benchmark — Performance Measurement & Regression Detection

Measure engine performance, compare against baselines, and flag regressions.

## Usage

- `/benchmark` — Run all engine benchmarks
- `/benchmark {engine}` — Benchmark a specific engine (e.g., `/benchmark organon`)
- `/benchmark --save` — Run and save results as new baseline
- `/benchmark --compare` — Run and compare against saved baseline

## Process

### 1. Build benchmark binary

If `Tools/benchmark_organon.cpp` exists as a template, use it. Otherwise, build and run
the test binary with profiling enabled.

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target XOlokunTests
```

### 2. Run engine profiling

For each engine (or specified engine):
- Instantiate engine via EngineRegistry
- Process 1000 blocks of 512 samples at 44100 Hz
- Measure: avg CPU time per block, peak CPU time, P95 percentile
- Record memory: RSS before and after processing

Use EngineProfiler.h metrics when available (lock-free ring buffer, P95 stats).

### 3. Compare against baseline (if --compare or baseline exists)

Baseline file: `.claude/benchmarks/baseline.json`

```json
{
  "timestamp": "2026-03-12T00:00:00Z",
  "sample_rate": 44100,
  "block_size": 512,
  "iterations": 1000,
  "engines": {
    "Organon": { "avg_us": 142, "peak_us": 210, "p95_us": 180 },
    "Ouroboros": { "avg_us": 98, "peak_us": 155, "p95_us": 130 }
  }
}
```

Regression thresholds:
- **Warning:** > 10% slower than baseline
- **Failure:** > 25% slower than baseline
- **Improvement:** > 10% faster than baseline (worth noting)

### 4. Save baseline (if --save)

Write current results to `.claude/benchmarks/baseline.json`.

### 5. Report

```markdown
## Benchmark Report
- **Sample Rate:** 44100 Hz
- **Block Size:** 512 samples
- **Iterations:** 1000

### Engine Performance
| Engine | Avg (μs) | Peak (μs) | P95 (μs) | vs Baseline | Status |
|--------|----------|-----------|----------|-------------|--------|
| Organon | 142 | 210 | 180 | +5% | ✓ |
| Ouroboros | 98 | 155 | 130 | -12% | ⚠ REGRESSION |

### Budget Analysis
- Block budget at 44100/512: {budget_us}μs
- Engines within budget: {count}/{total}
- Engines exceeding budget: {list}

### Regressions (if any)
- **{Engine}:** {current}μs vs {baseline}μs (+{percent}%)
  - Investigate: {suggestion based on recent changes}
```

## Primitives Used
- `chain-pipeline` — build → profile → compare → report
- `validate-fix-loop` — if regression detected, investigate and suggest

## Notes
- Always run benchmarks in Release mode — Debug numbers are meaningless
- Run on a quiet system (minimize background processes) for stable measurements
- The baseline file is checked into the repo so regressions are tracked across commits
- EngineProfiler.h budget alarm threshold is 22% for Organon — use as reference
