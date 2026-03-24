# /debug-audio — DSP Health & Safety Checks

Diagnose audio issues: silence, clipping, denormals, NaN/Inf, DC offset, instability.
The audio equivalent of a debugger — for when something sounds wrong or the CPU spikes.

## Usage

- `/debug-audio` — Full health check on all engines
- `/debug-audio {engine}` — Check a specific engine
- `/debug-audio silence` — Diagnose why an engine produces no output
- `/debug-audio clip` — Find sources of clipping/distortion
- `/debug-audio cpu` — Diagnose CPU spikes
- `/debug-audio denormal` — Check denormal protection coverage

## Process

### 1. Static Analysis (code inspection)

#### Denormal Check
Scan all engine render paths for:
- IIR filters without `flushDenormals()` or `JUCE_SNAP_TO_ZERO`
- Feedback loops without denormal protection
- Recursive calculations that decay toward zero
- One-pole smoothers without floor clamping

Report each unprotected path with file:line.

#### NaN/Inf Risk
Scan for:
- Division where denominator could be zero
- `sqrt()` of potentially negative values
- `log()` of potentially zero/negative values
- `pow()` with extreme exponents
- Uninitialized variables in DSP path

#### Clipping Risk
Scan for:
- Gain chains without limiting (gain × gain × gain)
- Feedback loops with gain > 1.0
- Missing output clamp before final buffer write
- Resonant filters at high Q without output limiting

#### DC Offset Risk
Scan for:
- Asymmetric waveform generation without DC blocking
- Rectification without highpass
- Unipolar modulation sources feeding audio output

### 2. Runtime Diagnostics (if test binary available)

Build and run a diagnostic pass:

```bash
cmake --build build --target XOlokunTests
```

For the target engine:
- Render 10 seconds of audio with default preset parameters
- Analyze output buffer:
  - **Silence:** All samples < 1e-10? → Dead engine
  - **NaN/Inf:** Any non-finite samples? → Numerical instability
  - **Denormals:** Samples in denormal range (1e-38 to 1e-45)? → Missing protection
  - **DC Offset:** Mean of all samples > 0.01? → DC bias
  - **Clipping:** Samples > 1.0? → Unbounded output
  - **Peak Level:** Max absolute sample value
  - **RMS Level:** Root mean square energy

### 3. CPU Profiling (if cpu mode)

Using EngineProfiler.h:
- Check each engine's avg/peak/P95 CPU time
- Flag engines exceeding their budget (default: 22% of block time)
- Check for CPU spikes (peak >> avg indicates intermittent expensive operations)
- Look for: memory allocation in audio path, lock contention, cache misses (large data structures)

### 4. Report

```markdown
## Audio Health Report
- **Scope:** {all engines / specific engine}
- **Status:** HEALTHY / {N} issues

### Static Analysis
| Check | Files Scanned | Issues | Severity |
|-------|--------------|--------|----------|
| Denormal Protection | {n} | {n} | {high/ok} |
| NaN/Inf Risk | {n} | {n} | {critical/ok} |
| Clipping Risk | {n} | {n} | {medium/ok} |
| DC Offset Risk | {n} | {n} | {low/ok} |

### Runtime Diagnostics (if available)
| Engine | Silent | NaN | Denormal | DC Offset | Clipping | Peak | RMS |
|--------|--------|-----|----------|-----------|----------|------|-----|
| {name} | ✓/✗ | ✓/✗ | ✓/✗ | ✓/✗ | ✓/✗ | {dB} | {dB} |

### CPU Profile (if cpu mode)
| Engine | Avg (μs) | Peak (μs) | P95 (μs) | Budget | Status |
|--------|----------|-----------|----------|--------|--------|
| {name} | {n} | {n} | {n} | {n}μs | ✓/⚠ |

### Issues Found
1. **[SEVERITY]** {file}:{line} — {description}
   - Risk: {what could go wrong in practice}
   - Fix: {specific code change needed}

### Silence Diagnosis (if silence mode)
If engine produces silence, check in order:
1. Is the oscillator running? (frequency > 0, amplitude > 0)
2. Is the envelope opening? (attack/sustain configured)
3. Is the filter passing signal? (cutoff not at 0)
4. Is the output gain > 0?
5. Are note events reaching the engine? (MIDI routing)
```

## Severity Levels
- **CRITICAL** — NaN/Inf in output (will corrupt downstream audio)
- **HIGH** — Denormals in output (will cause CPU spikes on some hardware)
- **MEDIUM** — Clipping without limiting (distortion, potential speaker damage)
- **LOW** — DC offset (wastes headroom, pops on start/stop)

## Notes
- Static analysis runs without building — useful for quick checks
- Runtime diagnostics require the test binary
- This skill is diagnostic only — it reports issues but doesn't auto-fix DSP code
- DSP fixes require human judgment about the intended behavior
- For CPU issues, also check if ParamSnapshot pattern is being used (cache per block, not per sample)
