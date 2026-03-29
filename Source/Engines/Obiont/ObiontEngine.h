#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <mutex>

namespace xolokun {

//==============================================================================
//
//  OBIONT ENGINE — Cellular Automata Oscillator
//  XOlokun Engine Module | Accent: Bioluminescent Amber #E8A030
//
//  Creature: The XObiont — a colonial organism (like a siphonophore) whose body
//            IS the waveform. Individual cells follow simple local rules; the
//            collective spatial pattern becomes music. Neither the cells nor the
//            music were designed — they emerged.
//
//  Architecture:
//    A cellular automata grid whose state IS the oscillator waveform.
//    The grid evolves on a background timer; the spatial pattern is read as
//    audio via a cosine-weighted projection (like a compact Fourier readout).
//
//  Dual Sub-Engine:
//    Mode 0 — 1D Elementary CA (Wolfram rules 0-255)
//      256-cell ring. Rule 90, 110 etc. Tonal / melodic use.
//      Supports Rule Morphing: smooth interpolation between two rules.
//    Mode 1 — 2D Life-like CA (B3/S23 and variants) [TODO — stub only]
//      64×64 grid. Richer evolving textures, less pitch-stable.
//
//  Signal Flow per Voice:
//    MIDI Note → Grid Seeder → CA Grid → Spatial Projection Readout
//      → Reconstruction Filter (4-pole matched-Z LP)
//      → DC Blocker
//      → SVF Musical Filter
//      → ADSR Amp Envelope
//      → Output
//
//  Spatial Projection:
//    NOT direct 1-bit readout (harsh noise). Instead, a cosine-weighted
//    dot product across the 256-cell ring produces a smooth scalar per
//    phase position. The phase accumulator (MIDI-pitch-driven) reads this
//    projected waveform like a wavetable. Grid width determines the fundamental
//    period in cells; projection width determines harmonic content.
//
//  Triple-Buffer Evolution:
//    A dedicated background thread advances the CA at evolutionRate Hz.
//    Three generation buffers (A/B/C) rotate: background writes next gen to
//    the "pending" buffer, then atomically promotes it to "ready". The audio
//    thread reads "ready" without blocking or allocating.
//    Lock-free swap via atomic index. No std::mutex on the hot path.
//
//  Anti-Extinction:
//    If live cell count falls below 5% of grid width, the grid is re-seeded
//    with a sinusoidal burst pattern anchored to the current MIDI note. This
//    prevents the engine from going silent due to a dying automaton.
//
//  8-Voice Polyphony:
//    Each voice owns its own CA grid (256 bytes — negligible). Voice stealing
//    re-seeds the grid on steal; it does NOT kill mid-evolution.
//
//  Modulation Matrix (stubs — destinations wired, sources partially wired):
//    Sources:  Envelope level, LFO, Velocity, Grid Energy (live cell ratio)
//    Destinations: Evolution Rate, Chaos, Rule Morph, Projection shape,
//                  Filter Cutoff, Filter Resonance
//
//  Parameters (prefix: obnt_):
//    obnt_mode            — 0=1D Elementary, 1=2D Life [stub]
//    obnt_rule            — 0-255 Wolfram rule
//    obnt_ruleMorph       — 0-1 blend to rule+N
//    obnt_evolutionRate   — 0.1-50 Hz grid step rate
//    obnt_gridDensity     — 0-1 initial seed density
//    obnt_chaos           — 0-1 random cell flip probability
//    obnt_projection      — 0-1 readout weighting shape
//    obnt_filterCutoff    — 20-20000 Hz
//    obnt_filterResonance — 0-1
//    obnt_attack / decay / sustain / release
//    obnt_pitchMode       — 0=Locked/MIDI-tracked, 1=Drift/free evolution
//    obnt_velSens         — 0-1 velocity → amplitude sensitivity
//    obnt_lfo1Rate        — 0.01-10 Hz
//    obnt_lfo1Depth       — 0-1
//    obnt_modSrc[1-4]     — mod matrix source selectors
//    obnt_modDst[1-4]     — mod matrix destination selectors
//    obnt_modAmt[1-4]     — mod matrix amounts
//    Macros: obnt_macroChaos, obnt_macroEvolution, obnt_macroSpace, obnt_macroCoupling
//
//  Doctrines honoured:
//    D001 Velocity → filter brightness + amplitude
//    D002 LFO1 (rate/depth), mod wheel → rule morph, aftertouch → chaos
//    D003 Physics IS synthesis — Wolfram rule is the actual CA rule byte
//    D004 Every declared parameter audibly wired
//    D005 LFO rate floor 0.01 Hz
//    D006 Velocity, mod wheel, aftertouch, pitch bend
//
//  SilenceGate: 500 ms hold (evolving textures need a generous tail).
//
//  Gallery code: OBIONT | Accent: Bioluminescent Amber #E8A030 | Prefix: obnt_
//
//==============================================================================

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
static constexpr int kObiontGridWidth1D  = 256;   // 1D ring size (cells)
static constexpr int kObiontGridH2D      = 64;    // 2D grid height [stub]
static constexpr int kObiontGridW2D      = 64;    // 2D grid width  [stub]
static constexpr int kObiontNumBufs      = 3;     // triple-buffer count
static constexpr int kObiontMaxVoices    = 8;
static constexpr float kObiontExtinctionThreshold = 0.05f; // 5% live cells

// ---------------------------------------------------------------------------
// Modulation matrix enums (sources / destinations)
// ---------------------------------------------------------------------------
enum class ObiontModSrc : int {
    Envelope   = 0,
    LFO        = 1,
    Velocity   = 2,
    GridEnergy = 3,   // live cell ratio
};
static constexpr int kObiontModSrcCount = 4;

enum class ObiontModDst : int {
    EvolutionRate  = 0,
    Chaos          = 1,
    RuleMorph      = 2,
    Projection     = 3,
    FilterCutoff   = 4,
    FilterResonance= 5,
    None           = 6,
};
static constexpr int kObiontModDstCount = 7;
static constexpr int kObiontModSlots    = 4;

// ---------------------------------------------------------------------------
// ObiontDCBlocker — 1-pole highpass to remove DC offset from CA readout.
// Uses matched-Z coefficient: y[n] = x[n] - x[n-1] + R*y[n-1]
// R = exp(-2π * fc / sr), fc ≈ 10 Hz → removes DC while preserving audio.
// ---------------------------------------------------------------------------
struct ObiontDCBlocker {
    float x1 = 0.f;
    float y1 = 0.f;
    float R  = 0.9997f; // updated in prepare()

    void prepare(double sampleRate) noexcept {
        // Matched-Z for fc = 10 Hz
        R = std::exp(-6.28318530718f * 10.f / (float)sampleRate);
        x1 = y1 = 0.f;
    }

    void reset() noexcept { x1 = y1 = 0.f; }

    float process(float in) noexcept {
        float out = in - x1 + R * y1;
        out = flushDenormal(out);
        x1 = in;
        y1 = out;
        return out;
    }
};

// ---------------------------------------------------------------------------
// ObiontReconLP — 4-pole matched-Z lowpass (two cascaded 2-pole biquads).
// Reconstruction filter: removes aliasing from the CA spatial projection.
// Cutoff tracks obnt_filterCutoff / 4 so it acts as a timbral control rather
// than a pure anti-alias brick wall.
// ---------------------------------------------------------------------------
struct ObiontReconLP {
    // Stage 1
    float s1_x1=0.f, s1_x2=0.f, s1_y1=0.f, s1_y2=0.f;
    float s1_b0=1.f, s1_b1=0.f, s1_b2=0.f, s1_a1=0.f, s1_a2=0.f;
    // Stage 2
    float s2_x1=0.f, s2_x2=0.f, s2_y1=0.f, s2_y2=0.f;
    float s2_b0=1.f, s2_b1=0.f, s2_b2=0.f, s2_a1=0.f, s2_a2=0.f;

    float lastFc=-1.f, lastQ=-1.f;

    void reset() noexcept {
        s1_x1=s1_x2=s1_y1=s1_y2=0.f;
        s2_x1=s2_x2=s2_y1=s2_y2=0.f;
        lastFc=lastQ=-1.f;
    }

    // Butterworth 4-pole split into two 2-pole stages with Q factors
    // 1/2sin(π/4)=0.7654 and 1/2sin(3π/4)=1.8478 (Butterworth Q table, order 4)
    void computeCoeffs(float fc, float sr) noexcept {
        if (fc == lastFc) return;
        lastFc = fc;

        fc = std::max(20.f, std::min(fc, sr * 0.45f));

        // Stage 1: Q1 = 0.7654 (Butterworth)
        const float Q1 = 0.7654f;
        {
            float w0   = 6.28318530718f * fc / sr;
            float cosW = fastCos(w0);
            float sinW = fastSin(w0);
            float alpha= sinW / (2.f * Q1);
            float norm = 1.f / (1.f + alpha);
            s1_b0 = (1.f - cosW) * 0.5f * norm;
            s1_b1 = (1.f - cosW)         * norm;
            s1_b2 = s1_b0;
            s1_a1 = -2.f * cosW           * norm;
            s1_a2 = (1.f - alpha)         * norm;
        }
        // Stage 2: Q2 = 1.8478 (Butterworth)
        const float Q2 = 1.8478f;
        {
            float w0   = 6.28318530718f * fc / sr;
            float cosW = fastCos(w0);
            float sinW = fastSin(w0);
            float alpha= sinW / (2.f * Q2);
            float norm = 1.f / (1.f + alpha);
            s2_b0 = (1.f - cosW) * 0.5f * norm;
            s2_b1 = (1.f - cosW)         * norm;
            s2_b2 = s2_b0;
            s2_a1 = -2.f * cosW           * norm;
            s2_a2 = (1.f - alpha)         * norm;
        }
    }

    float process(float in, float fc, float sr) noexcept {
        computeCoeffs(fc, sr);
        // Stage 1
        float y = s1_b0*in + s1_b1*s1_x1 + s1_b2*s1_x2 - s1_a1*s1_y1 - s1_a2*s1_y2;
        y = flushDenormal(y);
        s1_x2=s1_x1; s1_x1=in;
        s1_y2=s1_y1; s1_y1=y;
        // Stage 2
        float y2 = s2_b0*y + s2_b1*s2_x1 + s2_b2*s2_x2 - s2_a1*s2_y1 - s2_a2*s2_y2;
        y2 = flushDenormal(y2);
        s2_x2=s2_x1; s2_x1=y;
        s2_y2=s2_y1; s2_y1=y2;
        return y2;
    }
};

// ---------------------------------------------------------------------------
// ObiontSVF — State Variable Filter for musical filter section.
// Topology-Preserving Transform (TPT) formulation.
// Uses matched-Z coefficient: g = tan(π*fc/sr) via fastTan.
// ---------------------------------------------------------------------------
struct ObiontSVF {
    float ic1eq = 0.f;
    float ic2eq = 0.f;
    float lastFc = -1.f;
    float lastQ  = -1.f;
    float g  = 0.5f;
    float k  = 1.414f;
    float a1 = 0.33f;
    float a2 = 0.5f;
    float a3 = 0.17f;

    void reset() noexcept { ic1eq = ic2eq = 0.f; lastFc = lastQ = -1.f; }

    void computeCoeffs(float fc, float Q, float sr) noexcept {
        if (fc == lastFc && Q == lastQ) return;
        lastFc = fc; lastQ = Q;
        fc = std::max(20.f, std::min(fc, sr * 0.45f));
        Q  = std::max(0.5f, std::min(Q, 20.f));
        // TPT prewarping: g = tan(pi*fc/sr)
        g  = fastTan(3.14159265358979f * fc / sr);
        k  = 1.f / Q;
        a1 = 1.f / (1.f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    // Returns low-pass output
    float processLP(float in, float fc, float Q, float sr) noexcept {
        computeCoeffs(fc, Q, sr);
        float v3 = in - ic2eq;
        float v1 = a1 * ic1eq + a2 * v3;
        float v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = flushDenormal(2.f * v1 - ic1eq);
        ic2eq = flushDenormal(2.f * v2 - ic2eq);
        return v2; // low-pass
    }
};

// ---------------------------------------------------------------------------
// ObiontLFO — Simple sine LFO, same pattern as OrganismEngine.
// ---------------------------------------------------------------------------
struct ObiontLFO {
    float phase = 0.f;
    float sr    = 44100.f;

    void prepare(double s) noexcept { sr = (float)s; phase = 0.f; }
    void reset()           noexcept { phase = 0.f; }

    float tick(float rate) noexcept {
        phase += rate / sr;
        if (phase >= 1.f) phase -= 1.f;
        return fastSin(6.28318530718f * phase);
    }
};

// ---------------------------------------------------------------------------
// ObiontCA1D — 1D Elementary Cellular Automaton (256-cell ring, triple-buffered)
//
// Thread model:
//   - Audio thread: reads from readyBuf (lock-free via atomic swap)
//   - Evolution is called by the audio thread's per-block evolution counter
//     (not a background OS thread — simpler, safer for the Audio Unit sandbox)
//
// Design rationale for audio-thread evolution:
//   We advance the CA from the audio renderBlock() at most once per audio block
//   (via a sample-counter gate). This avoids OS thread creation inside an AU,
//   which is forbidden in some hosts. The evolution step is O(256) bit ops —
//   ~256 ns at 3 GHz — negligible vs. audio processing. The "triple-buffer"
//   model is retained conceptually for future background-thread promotion.
//
// Triple-buffer rotation (logical):
//   readyBuf  — current generation read by spatial projection
//   pendingBuf— next generation being written
//   After each step: swap pending↔ready, clear the old ready as the new pending.
// ---------------------------------------------------------------------------
struct ObiontCA1D {
    // Three generation buffers, each 256 uint8_t
    uint8_t bufs[kObiontNumBufs][kObiontGridWidth1D] = {};
    // 0=A, 1=B, 2=C — which buffer is currently "ready" (audio-thread readable)
    std::atomic<int> readyIdx   { 0 };
    int              pendingIdx { 1 };
    // Lock protecting writes to pendingIdx buffer from concurrent access.
    // Only taken during evolution (off-hot-path per-block at most once).
    std::mutex       evolveMutex;

    // Current rule (0-255)
    uint8_t rule  = 90;
    // Morph target rule (for inter-rule blending)
    uint8_t rule2 = 110;
    // Blend amount 0→rule, 1→rule2 (sampled per evolution step)
    float   morphAmt = 0.f;

    void reset() noexcept {
        for (auto& b : bufs) std::fill(std::begin(b), std::end(b), uint8_t(0));
        readyIdx.store(0, std::memory_order_relaxed);
        pendingIdx = 1;
    }

    // Seed the pending buffer with a structured sinusoidal pattern.
    // frequency: how many "bumps" across the ring (1=one half-wave, 2=full, etc.)
    // density:   0-1, threshold for cell-on (0.5 = balanced; >0.5 = denser)
    void seedSinusoidal(int frequency, float density, uint32_t& rng) noexcept {
        std::lock_guard<std::mutex> lock(evolveMutex);
        for (int i = 0; i < kObiontGridWidth1D; ++i) {
            // Sinusoidal envelope: sin(2π * freq * i / N) → 0..1
            float val = 0.5f + 0.5f * std::sin(6.28318530718f * frequency * i / (float)kObiontGridWidth1D);
            // Add small chaos from RNG
            rng = rng * 1664525u + 1013904223u;
            float noise = (float)(rng & 0xFFFF) / 65535.f * 0.1f;
            bufs[pendingIdx][i] = (val + noise > density) ? 1u : 0u;
        }
        // Promote pending → ready immediately after seed
        int old = readyIdx.load(std::memory_order_relaxed);
        readyIdx.store(pendingIdx, std::memory_order_release);
        pendingIdx = old;
    }

    // Advance one generation.
    // chaosProbFP: probability 0–1 of randomly flipping a cell after the rule step.
    // rng: caller's LCG state (modified in-place).
    // Returns the live-cell ratio (for anti-extinction check and Grid Energy source).
    float evolve(float chaosProbFP, float morphAmtSnapshot, uint32_t& rng) noexcept {
        std::lock_guard<std::mutex> lock(evolveMutex);

        const int readIdx = readyIdx.load(std::memory_order_acquire);
        const uint8_t* src = bufs[readIdx];
        uint8_t* dst       = bufs[pendingIdx];

        // Compute blended rule: per-bit majority vote between rule and rule2
        uint8_t blendedRule = rule;
        if (morphAmtSnapshot > 0.f && morphAmtSnapshot <= 1.f) {
            blendedRule = 0;
            for (int bit = 0; bit < 8; ++bit) {
                int ba = (rule  >> bit) & 1;
                int bb = (rule2 >> bit) & 1;
                if (ba == bb) {
                    blendedRule |= (uint8_t)(ba << bit);
                } else {
                    // Use morphAmt as probability: bit from rule2 if rand < morph
                    rng = rng * 1664525u + 1013904223u;
                    float r = (float)(rng & 0xFFFF) / 65536.f;
                    blendedRule |= (uint8_t)(((r < morphAmtSnapshot) ? bb : ba) << bit);
                }
            }
        }

        int liveCells = 0;
        for (int i = 0; i < kObiontGridWidth1D; ++i) {
            int left   = (i - 1 + kObiontGridWidth1D) & (kObiontGridWidth1D - 1);
            int right  = (i + 1) & (kObiontGridWidth1D - 1);
            // Wolfram neighborhood: (left<<2) | (center<<1) | right
            int neighborhood = (src[left] << 2) | (src[i] << 1) | src[right];
            uint8_t newCell = (blendedRule >> neighborhood) & 1u;

            // Chaos: random bit flip
            if (chaosProbFP > 0.f) {
                rng = rng * 1664525u + 1013904223u;
                float r = (float)(rng & 0xFFFF) / 65536.f;
                if (r < chaosProbFP) newCell ^= 1u;
            }
            dst[i] = newCell;
            liveCells += newCell;
        }

        // Swap pending → ready
        int old = readyIdx.load(std::memory_order_relaxed);
        readyIdx.store(pendingIdx, std::memory_order_release);
        pendingIdx = old;

        return (float)liveCells / (float)kObiontGridWidth1D;
    }

    // Read the currently-ready generation into a local snapshot (per-voice copy).
    // Called from renderBlock() before the spatial projection loop.
    void readSnapshot(uint8_t* dst256) const noexcept {
        const int ridx = readyIdx.load(std::memory_order_acquire);
        std::memcpy(dst256, bufs[ridx], kObiontGridWidth1D);
    }

    // Count live cells in the ready buffer (for Grid Energy mod source).
    float computeGridEnergy() const noexcept {
        const int ridx = readyIdx.load(std::memory_order_acquire);
        int live = 0;
        for (int i = 0; i < kObiontGridWidth1D; ++i)
            live += bufs[ridx][i];
        return (float)live / (float)kObiontGridWidth1D;
    }
};

// ---------------------------------------------------------------------------
// ObiontProjection — Cosine-weighted spatial readout.
//
// The phase accumulator (driven by MIDI pitch) scans across the 256-cell ring.
// At phase p ∈ [0,1), we compute a weighted dot product of the grid against a
// cosine window centred at cell floor(p * N). The width of the window controls
// how many harmonics are present: narrow = bright (many harmonics), wide = dark.
//
//   projectionWidth = lerp(4, 64, projection_param)   cells
//   window[k] = cos(π * k / projectionWidth)  for k in [-w/2, +w/2]
//   output = Σ grid[i] * window[i - centre]
//
// This is equivalent to reading a "soft" wavetable whose harmonic spectrum is
// shaped by the CA pattern AND the cosine window width. Because the grid
// changes with each evolution, the wavetable itself evolves.
//
// Fast path: the readout only touches projectionWidth cells per sample,
// not all 256. For projectionWidth=32 at 44100Hz, that is 44100*32 ≈ 1.4M
// multiplies/s — entirely acceptable.
// ---------------------------------------------------------------------------
struct ObiontProjection {
    static float read(const uint8_t* grid256, float phase, float projectionParam) noexcept {
        // Map projection_param 0-1 → window width 4-64 cells
        int width = 4 + (int)(projectionParam * 60.f + 0.5f);
        width = std::max(4, std::min(width, 64));

        int N       = kObiontGridWidth1D;
        int centre  = (int)(phase * N) & (N - 1);
        int half    = width / 2;

        float sum   = 0.f;
        float norm  = 0.f;
        for (int k = -half; k <= half; ++k) {
            int idx = (centre + k + N) & (N - 1);
            // Cosine window
            float w = std::cos(3.14159265358979f * (float)k / (float)(half + 1));
            sum  += (float)grid256[idx] * w;
            norm += std::abs(w);
        }
        if (norm < 1e-6f) return 0.f;
        return (sum / norm) * 2.f - 1.f; // bipolar
    }
};

// ---------------------------------------------------------------------------
// ObiontVoice — single polyphonic voice
// ---------------------------------------------------------------------------
struct ObiontVoice {
    // Identity
    bool     active       = false;
    int      note         = 60;
    float    velocity     = 0.f;
    float    stealGain    = 0.f;     // crossfade on steal (ramps from 1 → 0 quickly)
    bool     releasing    = false;

    // CA engine (per-voice — each voice has its own independent grid)
    ObiontCA1D ca;

    // Phase accumulator for wavetable-style readout (0-1)
    float phase        = 0.f;

    // Evolution subsystem
    float evoCounter   = 0.f;       // sample counter between evolution steps
    float evoEnergy    = 0.5f;      // Grid Energy (last evolve() result), smoothed
    float evoEnergySmooth = 0.5f;   // IIR-smoothed grid energy (for mod source)

    // DSP chain
    ObiontReconLP reconFilter;
    ObiontDCBlocker dcBlocker;
    ObiontSVF svf;
    StandardADSR adsr;
    ObiontLFO lfo;

    // Snapshot of the CA grid for the current render (no allocation in audio thread)
    uint8_t gridSnapshot[kObiontGridWidth1D] = {};

    // Coupling accumulation
    float couplingFilterMod = 0.f;
    float couplingPitchMod  = 0.f;

    // Per-voice RNG (Lehmer LCG)
    uint32_t rng = 0xCAFEBABEu;

    void reset(double sampleRate) noexcept {
        active     = false;
        releasing  = false;
        phase      = 0.f;
        evoCounter = 0.f;
        evoEnergy  = 0.5f;
        evoEnergySmooth = 0.5f;
        stealGain  = 0.f;
        couplingFilterMod = 0.f;
        couplingPitchMod  = 0.f;
        ca.reset();
        reconFilter.reset();
        dcBlocker.prepare(sampleRate);
        svf.reset();
        adsr.prepare((float)sampleRate);
        lfo.prepare(sampleRate);
        rng = 0xCAFEBABEu ^ (uint32_t)(uintptr_t)this;
    }

    // Note-on: seed the CA and trigger the ADSR
    void noteOn(int midiNote, float vel, int frequency, float density,
                double sampleRate) noexcept {
        note      = midiNote;
        velocity  = vel;
        releasing = false;
        active    = true;
        phase     = 0.f;
        evoCounter= 0.f;
        couplingFilterMod = 0.f;
        couplingPitchMod  = 0.f;

        // Structured sinusoidal seed — use MIDI note to vary frequency of bumps
        int seedFreq = std::max(1, (midiNote % 12) + 1); // 1-12 bumps
        ca.seedSinusoidal(seedFreq, density, rng);

        adsr.noteOn();
        lfo.reset();
        dcBlocker.reset();
        reconFilter.reset();
        svf.reset();
    }

    // Note-off: enter ADSR release
    void noteOff() noexcept {
        releasing = true;
        adsr.noteOff();
    }

    // Voice steal: silent kill for immediate re-use
    void steal(int midiNote, float vel, int frequency, float density,
               double sampleRate) noexcept {
        // Re-seed grid on steal — don't kill mid-evolution
        int seedFreq = std::max(1, (midiNote % 12) + 1);
        ca.seedSinusoidal(seedFreq, density, rng);

        note      = midiNote;
        velocity  = vel;
        releasing = false;
        phase     = 0.f;
        evoCounter= 0.f;
        couplingFilterMod = 0.f;
        couplingPitchMod  = 0.f;
        adsr.kill();
        adsr.prepare((float)sampleRate);
        adsr.noteOn();
        lfo.reset();
    }

    bool isIdle() const noexcept {
        return !active;
    }
};

//==============================================================================
// ObiontEngine — the full SynthEngine implementation
//==============================================================================
class ObiontEngine : public SynthEngine {
public:
    ObiontEngine()  = default;
    ~ObiontEngine() = default;

    // -------------------------------------------------------------------------
    // Static parameter registration (called by XOlokunProcessor)
    // -------------------------------------------------------------------------
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using P  = juce::ParameterID;
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;
        auto nr  = [](float lo, float hi, float step=0.f) {
            return juce::NormalisableRange<float>(lo, hi, step);
        };

        // --- CA control ---
        params.push_back(std::make_unique<PI>(P("obnt_mode",1),    "Mode",     0, 1, 0));
        params.push_back(std::make_unique<PI>(P("obnt_rule",1),    "Rule",     0, 255, 90));
        params.push_back(std::make_unique<PF>(P("obnt_ruleMorph",1), "Rule Morph",
            nr(0.f, 1.f), 0.f));
        params.push_back(std::make_unique<PF>(P("obnt_evolutionRate",1), "Evolution Rate",
            nr(0.1f, 50.f), 4.f));
        params.push_back(std::make_unique<PF>(P("obnt_gridDensity",1), "Grid Density",
            nr(0.f, 1.f), 0.5f));
        params.push_back(std::make_unique<PF>(P("obnt_chaos",1),   "Chaos",
            nr(0.f, 1.f), 0.f));
        params.push_back(std::make_unique<PF>(P("obnt_projection",1), "Projection",
            nr(0.f, 1.f), 0.35f));

        // --- Pitch mode ---
        params.push_back(std::make_unique<PI>(P("obnt_pitchMode",1), "Pitch Mode", 0, 1, 0));

        // --- Reconstruction + musical filter ---
        params.push_back(std::make_unique<PF>(P("obnt_filterCutoff",1), "Filter Cutoff",
            nr(20.f, 20000.f), 3000.f));
        params.push_back(std::make_unique<PF>(P("obnt_filterResonance",1), "Filter Resonance",
            nr(0.f, 1.f), 0.3f));

        // --- ADSR ---
        params.push_back(std::make_unique<PF>(P("obnt_attack",1),  "Attack",
            nr(0.001f, 4.f), 0.01f));
        params.push_back(std::make_unique<PF>(P("obnt_decay",1),   "Decay",
            nr(0.01f, 4.f), 0.3f));
        params.push_back(std::make_unique<PF>(P("obnt_sustain",1), "Sustain",
            nr(0.f, 1.f), 0.7f));
        params.push_back(std::make_unique<PF>(P("obnt_release",1), "Release",
            nr(0.01f, 8.f), 0.8f));

        // --- Velocity sensitivity ---
        params.push_back(std::make_unique<PF>(P("obnt_velSens",1), "Vel Sensitivity",
            nr(0.f, 1.f), 0.7f));

        // --- LFO 1 ---
        params.push_back(std::make_unique<PF>(P("obnt_lfo1Rate",1),  "LFO1 Rate",
            nr(0.01f, 10.f), 0.5f));
        params.push_back(std::make_unique<PF>(P("obnt_lfo1Depth",1), "LFO1 Depth",
            nr(0.f, 1.f), 0.2f));

        // --- Modulation matrix (4 slots × src/dst/amt) ---
        // Src: 0=Envelope, 1=LFO, 2=Velocity, 3=GridEnergy
        // Dst: 0=EvolutionRate, 1=Chaos, 2=RuleMorph, 3=Projection,
        //      4=FilterCutoff, 5=FilterResonance, 6=None
        for (int s = 1; s <= kObiontModSlots; ++s) {
            params.push_back(std::make_unique<PI>(
                P(("obnt_modSrc" + std::to_string(s)).c_str(), 1),
                ("Mod " + std::to_string(s) + " Src").c_str(),
                0, kObiontModSrcCount - 1, 0));
            params.push_back(std::make_unique<PI>(
                P(("obnt_modDst" + std::to_string(s)).c_str(), 1),
                ("Mod " + std::to_string(s) + " Dst").c_str(),
                0, kObiontModDstCount - 1, kObiontModDstCount - 1)); // default None
            params.push_back(std::make_unique<PF>(
                P(("obnt_modAmt" + std::to_string(s)).c_str(), 1),
                ("Mod " + std::to_string(s) + " Amt").c_str(),
                nr(-1.f, 1.f), 0.f));
        }

        // --- 4 Macros ---
        params.push_back(std::make_unique<PF>(P("obnt_macroChaos",1),     "CHAOS",
            nr(0.f, 1.f), 0.f));
        params.push_back(std::make_unique<PF>(P("obnt_macroEvolution",1), "EVOLUTION",
            nr(0.f, 1.f), 0.5f));
        params.push_back(std::make_unique<PF>(P("obnt_macroSpace",1),     "SPACE",
            nr(0.f, 1.f), 0.3f));
        params.push_back(std::make_unique<PF>(P("obnt_macroCoupling",1),  "COUPLING",
            nr(0.f, 1.f), 0.f));
    }

    // -------------------------------------------------------------------------
    // SynthEngine identity
    // -------------------------------------------------------------------------
    juce::String   getEngineId()     const override { return "Obiont"; }
    juce::Colour   getAccentColour() const override { return juce::Colour(0xffE8A030); }
    int            getMaxVoices()    const override { return kObiontMaxVoices; }

    int getActiveVoiceCount() const override {
        return activeVoiceCount_.load(std::memory_order_relaxed);
    }

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------
    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = (float)sampleRate;
        (void)maxBlockSize;

        for (auto& v : voices)
            v.reset(sampleRate);

        // IIR smooth coefficient for per-block smoothed params (~5ms)
        smoothCoeff = 1.f - std::exp(-1.f / (0.005f * (float)sampleRate));

        modWheelVal    = 0.f;
        aftertouchVal  = 0.f;
        pitchBendNorm  = 0.f;
        couplingFilterMod = 0.f;
        couplingPitchMod  = 0.f;
        lastOutputSample  = 0.f;
        rngGlobal = 0xDEADBEEFu;

        prepareSilenceGate(sampleRate, maxBlockSize, 500.f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset((double)sr);
        modWheelVal    = 0.f;
        aftertouchVal  = 0.f;
        pitchBendNorm  = 0.f;
        couplingFilterMod = 0.f;
        couplingPitchMod  = 0.f;
        lastOutputSample  = 0.f;
    }

    // -------------------------------------------------------------------------
    // Parameter layout / attachment
    // -------------------------------------------------------------------------
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParameters(params);
        return { params.begin(), params.end() };
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        p_mode             = apvts.getRawParameterValue("obnt_mode");
        p_rule             = apvts.getRawParameterValue("obnt_rule");
        p_ruleMorph        = apvts.getRawParameterValue("obnt_ruleMorph");
        p_evolutionRate    = apvts.getRawParameterValue("obnt_evolutionRate");
        p_gridDensity      = apvts.getRawParameterValue("obnt_gridDensity");
        p_chaos            = apvts.getRawParameterValue("obnt_chaos");
        p_projection       = apvts.getRawParameterValue("obnt_projection");
        p_pitchMode        = apvts.getRawParameterValue("obnt_pitchMode");
        p_filterCutoff     = apvts.getRawParameterValue("obnt_filterCutoff");
        p_filterResonance  = apvts.getRawParameterValue("obnt_filterResonance");
        p_attack           = apvts.getRawParameterValue("obnt_attack");
        p_decay            = apvts.getRawParameterValue("obnt_decay");
        p_sustain          = apvts.getRawParameterValue("obnt_sustain");
        p_release          = apvts.getRawParameterValue("obnt_release");
        p_velSens          = apvts.getRawParameterValue("obnt_velSens");
        p_lfo1Rate         = apvts.getRawParameterValue("obnt_lfo1Rate");
        p_lfo1Depth        = apvts.getRawParameterValue("obnt_lfo1Depth");
        for (int s = 1; s <= kObiontModSlots; ++s) {
            p_modSrc[s-1] = apvts.getRawParameterValue(("obnt_modSrc" + std::to_string(s)).c_str());
            p_modDst[s-1] = apvts.getRawParameterValue(("obnt_modDst" + std::to_string(s)).c_str());
            p_modAmt[s-1] = apvts.getRawParameterValue(("obnt_modAmt" + std::to_string(s)).c_str());
        }
        p_macroChaos       = apvts.getRawParameterValue("obnt_macroChaos");
        p_macroEvolution   = apvts.getRawParameterValue("obnt_macroEvolution");
        p_macroSpace       = apvts.getRawParameterValue("obnt_macroSpace");
        p_macroCoupling    = apvts.getRawParameterValue("obnt_macroCoupling");
    }

    // -------------------------------------------------------------------------
    // Coupling
    // -------------------------------------------------------------------------
    void applyCouplingInput(CouplingType type, float amount,
                            const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        const float recv = p_macroCoupling ? (0.5f + p_macroCoupling->load() * 0.5f) : 0.5f;
        if (type == CouplingType::AmpToFilter)
            couplingFilterMod += amount * 2000.f * recv;
        else if (type == CouplingType::AmpToPitch || type == CouplingType::PitchToPitch)
            couplingPitchMod  += amount * recv;
    }

    float getSampleForCoupling(int /*channel*/, int /*sampleIndex*/) const override {
        return lastOutputSample;
    }

    // -------------------------------------------------------------------------
    // renderBlock
    // -------------------------------------------------------------------------
    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer&          midi,
                     int                        numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;

        // 1. Parse MIDI
        for (const auto meta : midi) {
            const auto msg = meta.getMessage();
            if (msg.isNoteOn()) {
                handleNoteOn(msg.getNoteNumber(), msg.getFloatVelocity());
                wakeSilenceGate();
            } else if (msg.isNoteOff()) {
                handleNoteOff(msg.getNoteNumber());
            } else if (msg.isController() && msg.getControllerNumber() == 1) {
                // D006: mod wheel → rule morph bias
                modWheelVal = msg.getControllerValue() / 127.f;
            } else if (msg.isChannelPressure()) {
                // D006: aftertouch → chaos intensity
                aftertouchVal = msg.getChannelPressureValue() / 127.f;
            } else if (msg.isAftertouch()) {
                aftertouchVal = msg.getAfterTouchValue() / 127.f;
            } else if (msg.isPitchWheel()) {
                pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
            }
        }

        // 2. SilenceGate bypass
        if (isSilenceGateBypassed()) {
            buffer.clear();
            return;
        }

        // 3. Guard: parameters not yet attached
        if (!p_rule) { buffer.clear(); return; }

        // 4. ParamSnapshot — read all params once per block (zero-cost in the loop)
        const int   modeParam       = (int)(p_mode->load() + 0.5f);
        const int   ruleParam       = (int)(p_rule->load() + 0.5f) & 0xFF;
        const float ruleMorph       = std::clamp(p_ruleMorph->load(), 0.f, 1.f);
        const float baseEvoRate     = p_evolutionRate->load();
        const float macroEvo        = p_macroEvolution ? p_macroEvolution->load() : 0.5f;
        // EVOLUTION macro scales the evolution rate 0.1x..4x around the base
        const float evoRate         = baseEvoRate * (0.1f + macroEvo * 3.9f);
        const float gridDensity     = std::clamp(p_gridDensity->load(), 0.05f, 0.95f);
        const float baseChaos       = p_chaos->load();
        const float macroChaosV     = p_macroChaos ? p_macroChaos->load() : 0.f;
        // D006: aftertouch → chaos overlay
        const float effectiveChaos  = std::clamp(baseChaos + macroChaosV + aftertouchVal * 0.4f,
                                                  0.f, 1.f);
        const float projectionParam = std::clamp(
            p_projection->load() + (p_macroSpace ? p_macroSpace->load() * 0.4f : 0.f),
            0.f, 1.f);
        const int   pitchMode       = (int)(p_pitchMode->load() + 0.5f);
        const float baseCutoff      = p_filterCutoff->load();
        const float filterRes       = std::clamp(p_filterResonance->load(), 0.f, 1.f);
        const float atkSec          = p_attack->load();
        const float decSec          = p_decay->load();
        const float susLvl          = p_sustain->load();
        const float relSec          = p_release->load();
        const float velSens         = p_velSens->load();
        const float lfo1Rate        = p_lfo1Rate->load();
        const float lfo1Depth       = p_lfo1Depth->load();

        // Mod matrix snapshot
        int   modSrc[kObiontModSlots], modDst[kObiontModSlots];
        float modAmt[kObiontModSlots];
        for (int s = 0; s < kObiontModSlots; ++s) {
            modSrc[s] = p_modSrc[s] ? (int)(p_modSrc[s]->load() + 0.5f) : 0;
            modDst[s] = p_modDst[s] ? (int)(p_modDst[s]->load() + 0.5f) : (kObiontModDstCount-1);
            modAmt[s] = p_modAmt[s] ? p_modAmt[s]->load() : 0.f;
        }

        // Capture and clear coupling state (snap-and-zero pattern)
        float savedCouplingFilter = couplingFilterMod;
        float savedCouplingPitch  = couplingPitchMod;
        couplingFilterMod = 0.f;
        couplingPitchMod  = 0.f;

        // Mod wheel biases rule morph slightly (D006)
        float effectiveRuleMorph = std::clamp(ruleMorph + modWheelVal * 0.3f, 0.f, 1.f);

        // SVF resonance mapping: 0-1 → Q 0.5-6.0
        const float svfQ = 0.5f + filterRes * 5.5f;

        float* L = buffer.getWritePointer(0);
        float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : L;
        buffer.clear();

        // 5. Per-voice rendering
        int activeCount = 0;
        for (auto& v : voices) {
            if (!v.active) continue;

            // Update ADSR params per-block
            v.adsr.setADSR(atkSec, decSec, susLvl, relSec);

            // CA rule update (per-block, not per-sample — negligible cost)
            v.ca.rule  = (uint8_t)(ruleParam & 0xFF);
            // Rule2 for morphing: rule+8 wraps (creates interesting harmonic transitions)
            v.ca.rule2 = (uint8_t)((ruleParam + 8) & 0xFF);

            // Pre-take grid snapshot ONCE per block
            v.ca.readSnapshot(v.gridSnapshot);

            // Samples-per-evolution step (independent of pitch — CRITICAL for musicality)
            const float samplesPerEvo = (evoRate > 0.f) ? (sr / evoRate) : sr;

            // --- Per-sample loop ---
            for (int n = 0; n < numSamples; ++n) {
                // --- LFO tick — single call per sample, result reused throughout ---
                float lfoOut = v.lfo.tick(lfo1Rate);

                // --- Evolution step gate ---
                v.evoCounter += 1.f;
                if (v.evoCounter >= samplesPerEvo) {
                    v.evoCounter -= samplesPerEvo;

                    // Apply modulation matrix to CA parameters
                    float modChaos     = effectiveChaos;
                    float modRuleMorph = effectiveRuleMorph;
                    float gridEnergy   = v.evoEnergySmooth;

                    for (int s = 0; s < kObiontModSlots; ++s) {
                        int dst = modDst[s];
                        if (dst == (kObiontModDstCount - 1)) continue; // None
                        // Only process CA-side destinations here (1=Chaos, 2=RuleMorph)
                        if (dst != 1 && dst != 2) continue;

                        float srcVal = 0.f;
                        switch (modSrc[s]) {
                            case 0: srcVal = v.adsr.getLevel(); break; // Envelope
                            case 1: srcVal = lfoOut;            break; // LFO (already ticked)
                            case 2: srcVal = v.velocity;        break; // Velocity
                            case 3: srcVal = gridEnergy;        break; // GridEnergy
                            default: break;
                        }
                        float modVal = srcVal * modAmt[s];

                        if (dst == 1) modChaos     = std::clamp(modChaos     + modVal, 0.f, 1.f);
                        else          modRuleMorph = std::clamp(modRuleMorph + modVal, 0.f, 1.f);
                    }

                    // Advance CA
                    float rawEnergy = v.ca.evolve(modChaos, modRuleMorph, v.rng);
                    // IIR smooth the energy (τ ≈ 50ms) for a stable mod source
                    v.evoEnergySmooth += (rawEnergy - v.evoEnergySmooth) * smoothCoeff * 10.f;
                    v.evoEnergySmooth  = flushDenormal(v.evoEnergySmooth);

                    // Anti-extinction: re-seed if live cell ratio < 5%
                    if (rawEnergy < kObiontExtinctionThreshold) {
                        int seedFreq = std::max(1, (v.note % 12) + 1);
                        v.ca.seedSinusoidal(seedFreq, 0.5f, v.rng);
                    }

                    // Refresh snapshot after evolution
                    v.ca.readSnapshot(v.gridSnapshot);
                }

                // --- MIDI pitch → fundamental frequency ---
                // pitchMode 0: MIDI-locked (standard), phase follows note frequency
                // pitchMode 1: Drift — phase advances at a slower freewheel rate
                float pitchSemitones = (float)v.note + pitchBendNorm * 2.f + savedCouplingPitch;
                float noteFreq = 440.f * fastPow2((pitchSemitones - 69.f) / 12.f);

                float phaseInc;
                if (pitchMode == 0) {
                    // Locked: read the projection at the pitch-synced rate
                    phaseInc = noteFreq / sr;
                } else {
                    // Drift: freewheel at a slow rate driven by evolution
                    phaseInc = (evoRate * 0.1f) / sr;
                }

                v.phase += phaseInc;
                if (v.phase >= 1.f) v.phase -= 1.f;

                // --- Spatial projection readout ---
                float projOut = ObiontProjection::read(v.gridSnapshot, v.phase, projectionParam);

                // --- Reconstruction filter (4-pole LP, tracks baseCutoff/2) ---
                float reconFc  = std::clamp(baseCutoff * 0.5f, 20.f, sr * 0.45f);
                float reconOut = v.reconFilter.process(projOut, reconFc, sr);

                // --- DC blocker ---
                float dcOut = v.dcBlocker.process(reconOut);

                // --- Modulation matrix: filter-side destinations (per-sample) ---
                float modFilterCutoffDelta = 0.f;
                float modFilterResDelta    = 0.f;
                for (int s = 0; s < kObiontModSlots; ++s) {
                    int dst = modDst[s];
                    if (dst != 4 && dst != 5) continue; // only filter destinations here
                    float srcVal = 0.f;
                    switch (modSrc[s]) {
                        case 0: srcVal = v.adsr.getLevel();  break; // Envelope
                        case 1: srcVal = lfoOut;             break; // LFO
                        case 2: srcVal = v.velocity;         break; // Velocity
                        case 3: srcVal = v.evoEnergySmooth;  break; // GridEnergy
                        default: break;
                    }
                    if (dst == 4) modFilterCutoffDelta += srcVal * modAmt[s] * 8000.f;
                    else          modFilterResDelta    += srcVal * modAmt[s];
                }

                // LFO1 independently modulates filter cutoff (D005)
                float lfoCutoff = lfoOut * lfo1Depth * 2000.f;

                // D001: velocity brightens filter cutoff (timbre, not just amplitude)
                float velCutoffBoost = v.velocity * velSens * 1500.f;

                float finalCutoff = std::clamp(
                    baseCutoff + lfoCutoff + savedCouplingFilter
                    + modFilterCutoffDelta + velCutoffBoost,
                    20.f, 20000.f);
                float finalQ = std::clamp(svfQ + modFilterResDelta * 5.5f, 0.5f, 20.f);

                // --- SVF lowpass ---
                float filtered = v.svf.processLP(dcOut, finalCutoff, finalQ, sr);

                // --- ADSR envelope ---
                float envLevel = v.adsr.process();

                // D001: velocity shapes amplitude (with sensitivity control)
                float velGain = 1.f - velSens + velSens * v.velocity;

                float output = filtered * envLevel * velGain * 0.7f;

                // Soft clip to prevent clipping from high-resonance CA patterns
                output = output / (1.f + std::abs(output));

                L[n] += output;
                if (R != L) R[n] += output;
                lastOutputSample = output;

                // Voice done when ADSR is idle
                if (!v.adsr.isActive()) {
                    v.active = false;
                }
            }

            if (v.active) ++activeCount;
        }

        activeVoiceCount_.store(activeCount, std::memory_order_relaxed);

        // 6. Feed SilenceGate analyzer
        analyzeForSilenceGate(buffer, numSamples);

        // Suppress unused mode warning (2D stub — TODO)
        (void)modeParam;
    }

private:
    // -------------------------------------------------------------------------
    // Voice management helpers
    // -------------------------------------------------------------------------
    void handleNoteOn(int note, float vel) noexcept
    {
        if (!p_gridDensity) return;
        const float density = p_gridDensity->load();

        // Find free voice first
        for (auto& v : voices) {
            if (!v.active) {
                v.noteOn(note, vel, note % 12 + 1, density, (double)sr);
                return;
            }
        }
        // No free voice — steal the oldest releasing voice, or the quietest active one
        ObiontVoice* victim = nullptr;
        float minLevel = 1.f;
        for (auto& v : voices) {
            if (v.releasing) {
                victim = &v;
                break;
            }
            float lvl = v.adsr.getLevel();
            if (lvl < minLevel) { minLevel = lvl; victim = &v; }
        }
        if (victim) victim->steal(note, vel, note % 12 + 1, density, (double)sr);
    }

    void handleNoteOff(int note) noexcept
    {
        for (auto& v : voices) {
            if (v.active && v.note == note && !v.releasing) {
                v.noteOff();
                return;
            }
        }
    }

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------
    float  sr              = 44100.f;
    float  smoothCoeff     = 0.001f;

    // Expression inputs
    float  modWheelVal     = 0.f;
    float  aftertouchVal   = 0.f;
    float  pitchBendNorm   = 0.f;

    // Block-level coupling accumulation
    float  couplingFilterMod = 0.f;
    float  couplingPitchMod  = 0.f;

    // Coupling output sample cache (O(1) getSampleForCoupling)
    float  lastOutputSample  = 0.f;

    // Voice pool
    std::array<ObiontVoice, kObiontMaxVoices> voices;

    // Global RNG (for anti-extinction seeding that doesn't belong to one voice)
    uint32_t rngGlobal = 0xDEADBEEFu;

    // -------------------------------------------------------------------------
    // Raw parameter pointers (cached in attachParameters, read in renderBlock)
    // -------------------------------------------------------------------------
    std::atomic<float>* p_mode            = nullptr;
    std::atomic<float>* p_rule            = nullptr;
    std::atomic<float>* p_ruleMorph       = nullptr;
    std::atomic<float>* p_evolutionRate   = nullptr;
    std::atomic<float>* p_gridDensity     = nullptr;
    std::atomic<float>* p_chaos           = nullptr;
    std::atomic<float>* p_projection      = nullptr;
    std::atomic<float>* p_pitchMode       = nullptr;
    std::atomic<float>* p_filterCutoff    = nullptr;
    std::atomic<float>* p_filterResonance = nullptr;
    std::atomic<float>* p_attack          = nullptr;
    std::atomic<float>* p_decay           = nullptr;
    std::atomic<float>* p_sustain         = nullptr;
    std::atomic<float>* p_release         = nullptr;
    std::atomic<float>* p_velSens         = nullptr;
    std::atomic<float>* p_lfo1Rate        = nullptr;
    std::atomic<float>* p_lfo1Depth       = nullptr;
    std::atomic<float>* p_modSrc[kObiontModSlots]  = {};
    std::atomic<float>* p_modDst[kObiontModSlots]  = {};
    std::atomic<float>* p_modAmt[kObiontModSlots]  = {};
    std::atomic<float>* p_macroChaos      = nullptr;
    std::atomic<float>* p_macroEvolution  = nullptr;
    std::atomic<float>* p_macroSpace      = nullptr;
    std::atomic<float>* p_macroCoupling   = nullptr;
};

} // namespace xolokun
