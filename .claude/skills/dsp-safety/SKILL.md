---
name: dsp-safety
description: Audit engine or DSP code for audio thread safety violations. Use when modifying renderBlock, adding DSP, reviewing pull requests that touch audio code, or checking for real-time safety issues.
argument-hint: [file-or-engine e.g. Source/Engines/Onset/OnsetEngine.h]
---

# DSP Audio Thread Safety Audit

Audit **$ARGUMENTS** for real-time safety violations. These rules are non-negotiable — a single violation can cause audio glitches, dropouts, or crashes in a DAW host.

## What to Audit

If the argument is an engine name, read all `.h` files in `Source/Engines/{name}/`.
If the argument is a file path, read that file.
If no argument, audit all recently modified DSP files (check `git diff --name-only HEAD~5`).

Focus on `renderBlock()`, `processSample()`, `processBlock()`, and any function called from them.

## Rule 1: No Memory Allocation on Audio Thread

**Search for violations in renderBlock() and its callees:**
- `new` / `delete` / `malloc` / `free` / `realloc`
- `std::vector` resize, push_back, emplace_back (implicit allocation)
- `std::string` construction or concatenation (heap-allocated)
- `std::map` / `std::unordered_map` insertion
- `std::make_unique` / `std::make_shared`
- `juce::String` construction (heap-allocated)
- `juce::Array` / `juce::OwnedArray` add/insert
- `AudioBuffer` resize or `setSize()` on audio thread

**Allowed patterns:**
- `std::array` (stack-allocated, fixed size)
- Pre-allocated buffers set up in `prepare()`
- Raw float arrays / C-style fixed arrays
- `WavetableOscillator` internal 256x2048 fixed buffer (set at compile time)

## Rule 2: No Blocking I/O on Audio Thread

**Search for violations:**
- File operations: `fopen`, `fread`, `fwrite`, `std::ifstream`, `juce::File::loadFileAsString`
- Network: `socket`, `connect`, `send`, `recv`
- `std::mutex::lock()` (use lock-free or try_lock only)
- `std::condition_variable::wait()`
- `sleep`, `usleep`, `std::this_thread::sleep_for`
- `juce::MessageManager::callAsync` from audio thread
- `DBG()` / `Logger::writeToLog()` (can block on file I/O)

**Allowed patterns:**
- `std::atomic` load/store/compare_exchange
- Lock-free ring buffers (`AudioRingBuffer`)
- `SpinLock::ScopedTryLockType` (non-blocking attempt)
- Atomic double-buffering (as in `MegaCouplingMatrix`)

## Rule 3: Denormal Protection in Feedback/Filter Paths

**Every recursive filter and feedback path must flush denormals.**

Check for `flushDenormal()` calls (from `Source/DSP/FastMath.h`) on:
- SVF state variables (`ic1eq`, `ic2eq`) after each sample update
- Feedback delay line outputs
- Integrator accumulators (e.g., `triIntegrator` in PolyBLEP)
- Any variable that feeds back into itself across samples
- Oscillator phase accumulators (should use `fmod` wrapping)

**Pattern to verify:**
```cpp
// CORRECT — denormal protection after state update
ic1eq = flushDenormal(2.0f * v1 - ic1eq);
ic2eq = flushDenormal(2.0f * v2 - ic2eq);

// WRONG — no protection, will accumulate denormals
ic1eq = 2.0f * v1 - ic1eq;
```

**Also check for:**
- `if (std::abs(x) < 1e-15f) x = 0.0f;` — acceptable alternative
- Missing denormal guards after feedback multiplication

## Rule 4: ParamSnapshot Pattern (Coefficient Caching)

**Parameters must be read once per block, not per sample.**

Check that `renderBlock()` follows this pattern:
1. Read all `std::atomic<float>*` parameter pointers at the top of the block
2. Compute derived coefficients once (filter cutoff → g/k/a1/a2/a3, etc.)
3. Inner sample loop uses only cached values

**Violations:**
- `param->load()` inside the per-sample loop
- `std::pow()`, `std::exp()`, `std::tan()` called per-sample instead of per-block
- Filter coefficient recomputation inside the sample loop

## Rule 5: Coupling Output Safety

**`getSampleForCoupling()` must be O(1) — cached value only.**

Check that:
- The function returns a pre-computed cached sample, not a computed value
- No function calls, no branching, no array lookups beyond simple index
- Cache is populated during `renderBlock()`, read during coupling

**Violation:**
```cpp
// WRONG — computation in coupling getter
float getSampleForCoupling(int ch, int idx) {
    return oscillator.process() * envelope.getLevel();  // BAD
}

// CORRECT — cached output
float getSampleForCoupling(int ch, int idx) {
    return outputCache[ch][idx];  // O(1)
}
```

## Rule 6: Engine Hot-Swap Safety

**50ms crossfade required for engine switching.**

If the code touches engine slot management or switching:
- Verify crossfade duration is 50ms (not abrupt cut)
- Check that old engine continues rendering during fade-out
- Verify no shared state corruption between old and new engine

## Output Format

```
## DSP Safety Audit: {TARGET}

### Summary
| Rule | Status | Violations |
|------|--------|------------|
| No Allocation | PASS/FAIL | count |
| No Blocking I/O | PASS/FAIL | count |
| Denormal Protection | PASS/FAIL | count |
| ParamSnapshot | PASS/FAIL | count |
| Coupling O(1) | PASS/FAIL/N/A | count |
| Hot-Swap Safety | PASS/FAIL/N/A | count |

**Overall: SAFE / {N} VIOLATIONS FOUND**

### Violations
[Each violation with file:line, code snippet, and recommended fix]

### Recommendations
[Priority-ordered fixes]
```
