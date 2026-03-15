# DSP Patterns Catalog

Common DSP patterns used across XOmnibus engines. Reference this when implementing or reviewing DSP code.

## Audio Thread Rules (Non-Negotiable)

1. **No memory allocation** — no `new`, `malloc`, `std::vector::push_back`, `std::string` construction
2. **No blocking I/O** — no file reads, network calls, or mutex locks that could block
3. **No system calls** — no `printf`, `NSLog`, or OS-level calls
4. **No exceptions** — use return codes, not try/catch
5. **No virtual dispatch in inner loops** — resolve vtables outside the sample loop

## ParamSnapshot Pattern

Cache all parameter pointers once per block, not per sample:

```cpp
// CORRECT: Cache once per processBlock()
void processBlock(AudioBuffer& buffer, int numSamples) {
    // Snapshot all params at block start
    const float cutoff = *cutoffParam;
    const float resonance = *resonanceParam;
    const float drive = *driveParam;

    for (int i = 0; i < numSamples; ++i) {
        // Use cached values — no pointer derefs in the loop
        processSample(buffer[i], cutoff, resonance, drive);
    }
}

// WRONG: Deref pointer every sample
for (int i = 0; i < numSamples; ++i) {
    float cutoff = *cutoffParam;  // pointer deref per sample — wasteful
    processSample(buffer[i], cutoff);
}
```

## Denormal Protection

Required in ALL feedback paths, recursive filters, and decay envelopes:

```cpp
// Option 1: Add DC offset (simplest)
static constexpr float kAntiDenormal = 1.0e-25f;
output = filterProcess(input) + kAntiDenormal;

// Option 2: Flush to zero check
inline float flushDenormal(float x) {
    return (std::abs(x) < 1.0e-15f) ? 0.0f : x;
}

// Option 3: Platform-level (set at plugin init, not per-sample)
// Set FTZ/DAZ flags on the audio thread at initialization
```

Places that MUST have denormal protection:
- IIR filter feedback paths (all biquads, SVF, ladder filters)
- Delay line feedback
- Envelope release tails
- Reverb decay networks
- Any recursive algorithm

## Smoothing / Zipper Prevention

Parameter changes must be smoothed to prevent zipper noise:

```cpp
// One-pole smoother — use for all continuous parameters
class ParamSmoother {
    float current = 0.0f;
    float coeff = 0.999f;  // Adjust for speed: 0.99 = fast, 0.9999 = slow
public:
    float process(float target) {
        current += coeff * (target - current);
        return current;
    }
};
```

When to smooth:
- Filter cutoff/resonance — always
- Amplitude/gain — always
- Oscillator pitch — usually (unless portamento is separate)
- Discrete switches — use crossfade instead of smoothing

## Engine Hot-Swap Crossfade

50ms crossfade when swapping engines to prevent clicks:

```cpp
static constexpr int kCrossfadeSamples = 2205;  // ~50ms at 44.1kHz

// During swap:
// 1. Start rendering new engine alongside old
// 2. Fade old engine out (linear or equal-power)
// 3. Fade new engine in
// 4. After crossfade complete, release old engine
```

## Common Filter Topologies

| Filter | Character | Used In |
|--------|-----------|---------|
| SVF (State Variable) | Clean, flexible, self-oscillates | Most engines |
| Moog Ladder | Warm, fat, classic low-pass | Obese, OddOscar |
| Biquad cascade | CPU-efficient, neutral | Utility filtering |
| Comb filter | Metallic, resonant, Karplus-Strong | Onset, Overworld |
| Allpass cascade | Phase manipulation, diffusion | Reverbs, phasers |

## Oscillator Patterns

```cpp
// Anti-aliased naive waveforms: use PolyBLEP
float polyBlep(float t, float dt) {
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0f;
    } else if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

// Wavetable: pre-compute band-limited tables per octave
// Granular: use Hann window for smooth grain envelopes
// FM: use phase accumulator, not frequency-modulated sin()
```

## Buffer Processing Conventions

```cpp
// Standard processBlock signature
void processBlock(float** outputBuffers, int numChannels, int numSamples);

// Stereo convention: channel 0 = left, channel 1 = right
// Mono engines: write same signal to both channels
// Always clear output buffer before writing (don't assume it's zeroed)
```

## Modulation Conventions

```cpp
// Modulation depth: 0.0 = none, 1.0 = full range
// Bipolar modulation: -1.0 to +1.0
// Unipolar modulation: 0.0 to 1.0

// Apply modulation:
float modulatedValue = baseValue + (modSource * modDepth * modRange);
modulatedValue = std::clamp(modulatedValue, minValue, maxValue);
```

## Coupling Integration Points

When an engine accepts coupling modulation:
1. Expose a `setCouplingInput(CouplingType type, float value)` method
2. Apply coupling modulation AFTER parameter smoothing, BEFORE DSP
3. Respect the coupling type — frequency mod and amplitude mod behave differently
4. Clamp coupled values to safe ranges (prevent NaN, inf, extreme values)

## Testing DSP in Isolation

Every DSP module must be testable without UI:
```cpp
// Test pattern: create module, feed impulse, verify output
MyFilter filter;
filter.setSampleRate(44100.0);
filter.setCutoff(1000.0f);
filter.setResonance(0.5f);

float output = filter.processSample(1.0f);  // impulse
assert(!std::isnan(output));
assert(!std::isinf(output));
assert(std::abs(output) < 100.0f);  // sanity bound
```
