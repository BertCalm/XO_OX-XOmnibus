#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

namespace xolokun {

//==============================================================================
//
//  X O P T I C  —  T H E   B I O L U M I N E S C E N T   F L A S H
//
//  Creature:   The comb jelly — a bioluminescent organism in the sunlit
//              shallows. OPTIC doesn't sing; it *pulses*. It reads the
//              spectral light of other engines and flashes patterns back
//              into the water column, synchronizing the ecosystem's rhythm
//              through luminous modulation rather than sound.
//
//  Lineage:    feliX-leaning, third generation. Born from feliX's electric
//              surface energy, OPTIC evolved to sense and respond to the
//              ocean's spectral weather. Where OddfeliX *makes* transients,
//              OPTIC *watches* them and translates their energy into
//              modulation signals that shape every engine it touches.
//
//  Position:   Sunlit Shallows — near the surface where light refracts
//              through shallow water. Accent: Phosphor Green #00FF41
//              (CRT phosphor glow / bioluminescent flash).
//
//  Heritage:   Winamp / MilkDrop (Ryan Geiss, 2001) — the idea that audio
//              visualization IS a creative instrument. Sidechaining as art
//              (Daft Punk's pumping bass). Spectral analysis as modulation
//              source (Izotope Iris, Zynaptiq). The "audio-reactive
//              visualizer" tradition, weaponized as a synth coupling engine.
//
//  Role:       OPTIC generates zero audio. It is a pure modulation engine:
//              it analyzes incoming audio from coupled engines, extracts
//              spectral features (bass, mid, high energy, centroid, flux),
//              and outputs 8 lock-free modulation signals that other engines
//              read through the coupling matrix. The AutoPulse system adds
//              self-evolving rhythmic pulses that breathe with the spectrum.
//
//  Signal:     Coupled audio in --> Band analysis --> Mod output bus --> Out
//              (no voice engine, no audio output, pure modulation)
//
//  Param prefix: optic_
//
//==============================================================================

//==============================================================================
// OpticNoiseGen — XORShift32 pseudo-random number generator.
//
// Used for stochastic modulation: adding organic variation to pulse timing
// and accent levels. The xorshift32 algorithm (Marsaglia, 2003) provides
// fast, statistically adequate randomness without the overhead of
// std::mt19937 — critical since this runs per-sample on the audio thread.
//
// Shift constants (13, 17, 5) are from Marsaglia's original paper:
// "Xorshift RNGs", Journal of Statistical Software, Vol. 8, Issue 14.
// These specific triplets produce a full-period sequence of 2^32 - 1 values.
//==============================================================================
class OpticNoiseGen
{
public:
    void seed (uint32_t initialSeed) noexcept { state = initialSeed ? initialSeed : 1; }

    // Returns bipolar noise in [-1, 1].
    // Division by 2^31 (2147483648) normalizes the full int32 range.
    float process() noexcept
    {
        state ^= state << 13;  // Marsaglia shift constant 1
        state ^= state >> 17;  // Marsaglia shift constant 2
        state ^= state << 5;   // Marsaglia shift constant 3
        return static_cast<float> (static_cast<int32_t> (state)) / 2147483648.0f;  // 2^31 normalization
    }

private:
    uint32_t state = 1;
};

//==============================================================================
// OpticBandAnalyzer — 8-band spectral energy tracker.
//
// Extracts the spectral shape of incoming audio using cascaded Cytomic SVF
// bandpass filters — one per frequency band — followed by rectification and
// envelope-following lowpass filters. This gives us real-time spectral
// features (centroid, flux, per-band energy) without the latency, windowing
// artifacts, or CPU cost of an FFT.
//
// The technique descends from analog spectrum analyzers (e.g., the Klark
// Teknik DN60) that used parallel bandpass filter banks. The digital
// equivalent using Cytomic SVFs (Andrew Simper's topology-preserving
// transform) gives us zero-delay-feedback accuracy at audio rates.
//
// Band layout (approximate center frequencies via geometric mean):
//   0: Sub         20 -   80 Hz     4: Hi-Mid      1k -  4k Hz
//   1: Bass        80 -  200 Hz     5: Presence     4k -  8k Hz
//   2: Lo-Mid     200 -  500 Hz     6: Brilliance   8k - 16k Hz
//   3: Mid        500 - 1000 Hz     7: Air         16k - 20k Hz
//==============================================================================
class OpticBandAnalyzer
{
public:
    static constexpr int kNumBands = 8;

    void prepare (double sampleRate) noexcept
    {
        cachedSampleRate = sampleRate;

        // Crossover frequencies between adjacent bands (Hz).
        // Chosen to match standard audio engineering band divisions:
        // Sub/Bass at 80, Bass/Lo-Mid at 200, Lo-Mid/Mid at 500,
        // Mid/Hi-Mid at 1k, Hi-Mid/Presence at 4k, Presence/Brilliance
        // at 8k, Brilliance/Air at 16k.
        static constexpr float crossoverFrequencies[kNumBands - 1] = {
            80.0f, 200.0f, 500.0f, 1000.0f, 4000.0f, 8000.0f, 16000.0f
        };

        for (int bandIndex = 0; bandIndex < kNumBands; ++bandIndex)
        {
            // Compute band edges: first band starts at 20 Hz (sub-bass floor),
            // last band extends to 20 kHz (audible ceiling).
            float lowEdgeHz  = (bandIndex == 0) ? 20.0f : crossoverFrequencies[bandIndex - 1];
            float highEdgeHz = (bandIndex == kNumBands - 1) ? 20000.0f : crossoverFrequencies[bandIndex];

            // Geometric mean gives perceptually centered frequency for log-spaced bands
            float centerFrequencyHz = std::sqrt (lowEdgeHz * highEdgeHz);

            // Q = center / bandwidth. Guard against division by zero for
            // extremely narrow bands. The 0.5x scaling and [0.1, 0.9] clamp
            // keep Q moderate: too high would ring, too low would bleed between bands.
            float bandwidth = highEdgeHz - lowEdgeHz;
            if (bandwidth < 1.0f) bandwidth = 1.0f;
            float qualityFactor = centerFrequencyHz / bandwidth;

            bandpassFilters[bandIndex].setMode (CytomicSVF::Mode::BandPass);
            bandpassFilters[bandIndex].setCoefficients (
                centerFrequencyHz,
                clamp (qualityFactor * 0.5f, 0.1f, 0.9f),
                static_cast<float> (cachedSampleRate));

            // Envelope follower: a lowpass filter at 30 Hz smooths the rectified
            // bandpass output into a slowly-varying energy estimate. 30 Hz is fast
            // enough to track kick drum transients (~33ms response) but slow
            // enough to avoid ripple from bass frequencies.
            envelopeFollowers[bandIndex].setMode (CytomicSVF::Mode::LowPass);
            envelopeFollowers[bandIndex].setCoefficients (
                30.0f,   // 30 Hz envelope cutoff — tracks ~33ms transients
                0.0f,    // No resonance — pure smoothing
                static_cast<float> (cachedSampleRate));
        }
    }

    void reset() noexcept
    {
        for (int bandIndex = 0; bandIndex < kNumBands; ++bandIndex)
        {
            bandpassFilters[bandIndex].reset();
            envelopeFollowers[bandIndex].reset();
            bandEnergy[bandIndex] = 0.0f;
        }
        spectralCentroid = 0.0f;
        spectralFlux = 0.0f;
        totalEnergy = 0.0f;
    }

    // Process a single sample through the full filter bank.
    // Call once per sample inside renderBlock's inner loop.
    void processSample (float input) noexcept
    {
        float previousTotalEnergy = totalEnergy;
        totalEnergy = 0.0f;

        for (int bandIndex = 0; bandIndex < kNumBands; ++bandIndex)
        {
            float bandpassOutput = bandpassFilters[bandIndex].processSample (input);
            float rectifiedLevel = std::fabs (bandpassOutput);
            float smoothedEnergy = envelopeFollowers[bandIndex].processSample (rectifiedLevel);

            // Flush denormals: envelope followers with exponential decay can produce
            // subnormal floats (< 1.18e-38) that cause massive CPU spikes on x86
            // when the FPU switches to microcode handling. Flushing to zero prevents
            // this while the energy is inaudibly small anyway.
            bandEnergy[bandIndex] = flushDenormal (smoothedEnergy);
            totalEnergy += bandEnergy[bandIndex];
        }

        // ---- Spectral centroid ----
        // The energy-weighted average band index, normalized to [0, 1].
        // A centroid near 0 means energy concentrated in sub/bass (dark timbre),
        // near 1 means energy in brilliance/air (bright timbre).
        // Threshold of 0.0001 prevents division by zero during silence.
        if (totalEnergy > 0.0001f)
        {
            float weightedBandSum = 0.0f;
            for (int bandIndex = 0; bandIndex < kNumBands; ++bandIndex)
                weightedBandSum += bandEnergy[bandIndex] * static_cast<float> (bandIndex);

            // Divide by (totalEnergy * 7) to normalize: max possible centroid
            // is band index 7, so dividing by (kNumBands - 1) maps to [0, 1].
            spectralCentroid = flushDenormal (
                weightedBandSum / (totalEnergy * static_cast<float> (kNumBands - 1)));
        }
        else
        {
            spectralCentroid = 0.0f;
        }

        // ---- Spectral flux ----
        // The frame-to-frame change in total energy. Positive spikes indicate
        // transient onsets (attacks). Negative values indicate decay.
        // Flush denormals for the same CPU protection reason as above.
        spectralFlux = flushDenormal (totalEnergy - previousTotalEnergy);
    }

    //--------------------------------------------------------------------------
    // Accessors
    //--------------------------------------------------------------------------

    float getBandEnergy (int bandIndex) const noexcept { return bandEnergy[bandIndex]; }
    float getCentroid()                 const noexcept { return spectralCentroid; }
    float getFlux()                     const noexcept { return spectralFlux; }
    float getTotalEnergy()              const noexcept { return totalEnergy; }

private:
    double cachedSampleRate = 44100.0;

    CytomicSVF bandpassFilters[kNumBands];    // One bandpass per frequency band
    CytomicSVF envelopeFollowers[kNumBands];  // One lowpass envelope follower per band

    float bandEnergy[kNumBands] = {};   // Smoothed energy per band [0, ~1]
    float spectralCentroid = 0.0f;      // Energy-weighted band center [0, 1]
    float spectralFlux = 0.0f;          // Frame-to-frame energy delta
    float totalEnergy = 0.0f;           // Sum of all band energies
};

//==============================================================================
// OpticAutoPulse — Self-evolving rhythmic pulse generator.
//
// Generates a thumping, breathing rhythmic modulation signal without MIDI
// input — the "trance heartbeat" of the OPTIC engine. The pulse shape
// models a kick drum's amplitude envelope: instant attack, exponential
// decay. But unlike a static drum machine, the AutoPulse *listens* to the
// spectral state of the analyzer and lets it slowly morph the rhythm:
//
//   - Spectral centroid shifts the pulse rate (brighter = slightly faster)
//   - Total energy extends decay time (louder = longer ring)
//   - Spectral flux adds stochastic accent variation (transients = surprise)
//
// This creates emergent trance patterns that evolve organically — the
// bioluminescent comb jelly pulsing in response to the ocean's currents.
//
// Output is a modulation signal [0, 1], not audio. It drives other engines
// through the coupling matrix (e.g., OPTIC pulse --> OBESE filter cutoff).
//
// Heritage: TR-808/909 accent circuits + Aphex Twin's generative rhythms
// + MilkDrop's audio-reactive feedback loops.
//==============================================================================
class OpticAutoPulse
{
public:
    void prepare (double sampleRate) noexcept
    {
        cachedSampleRate = sampleRate;
        phase = 0.0;
        pulseLevel = 0.0f;
        noiseGen.seed (42);  // Deterministic seed for reproducible patterns
    }

    void reset() noexcept
    {
        phase = 0.0;
        pulseLevel = 0.0f;
        swingAccumulator = 0.0f;
        driftOscillatorPhase = 0.0;
        subdivisionAccumulator = 0.0;
    }

    struct Params {
        float rate;         // Base pulse rate in Hz (0.5-16, typically 1-4 Hz for trance)
        float shape;        // Pulse shape: 0 = sharp kick, 1 = soft swell
        float swing;        // Swing amount [0, 1] — offsets even subdivisions
        float evolve;       // Visual feedback influence [0, 1] — how much the spectrum morphs the pulse
        float subdivision;  // Rhythmic subdivision: 0=whole, 0.25=quarter, 0.5=8th, 1=16th
        float accent;       // Accent strength on downbeats [0, 1]

        // Feedback FROM the band analyzer (lock-free atomic handoff)
        float visualCentroid;   // Spectral brightness [0, 1]
        float visualEnergy;     // Total spectral energy [0, 1]
        float visualFlux;       // Spectral rate-of-change
    };

    float process (const Params& params) noexcept
    {
        //----------------------------------------------------------------------
        // Rate modulation — the spectrum bends the tempo
        //----------------------------------------------------------------------

        float effectiveRate = params.rate;
        if (params.evolve > 0.0f)
        {
            // Spectral centroid shifts rate by +/-20%: bright sounds speed up,
            // dark sounds slow down. The 0.4 coefficient maps centroid's
            // [-0.5, +0.5] deviation to a [-20%, +20%] rate multiplier.
            float centroidRateShift = (params.visualCentroid - 0.5f) * 0.4f * params.evolve;

            // Energy surges speed up by up to 15%: louder passages push tempo.
            // The 0.15 coefficient keeps this subtle — a gentle acceleration.
            float energyRateBoost = params.visualEnergy * 0.15f * params.evolve;

            effectiveRate *= (1.0f + centroidRateShift + energyRateBoost);
        }

        // Drift oscillator: a very slow sine LFO (0.07 Hz ~ 14 second cycle)
        // adds organic rate wandering of +/-3%. This prevents the pulse from
        // sounding perfectly mechanical even without spectral input.
        driftOscillatorPhase += 0.07 / cachedSampleRate;    // 0.07 Hz drift rate
        if (driftOscillatorPhase > 1.0) driftOscillatorPhase -= 1.0;
        float driftModulation = fastSin (static_cast<float> (driftOscillatorPhase * kTwoPi))
                                * 0.03f     // +/-3% maximum rate deviation
                                * params.evolve;
        effectiveRate *= (1.0f + driftModulation);

        effectiveRate = clamp (effectiveRate, 0.1f, 30.0f);  // Safety bounds

        //----------------------------------------------------------------------
        // Phase accumulation — the pulse clock
        //----------------------------------------------------------------------

        double phaseIncrement = effectiveRate / cachedSampleRate;
        phase += phaseIncrement;

        // Map subdivision parameter to discrete rate multipliers.
        // The thresholds (0.15, 0.35, 0.6) divide the [0, 1] knob range
        // into four zones: whole note, quarter, 8th, 16th.
        float subdivisionMultiplier = 1.0f;
        if (params.subdivision > 0.6f)        subdivisionMultiplier = 4.0f;  // 16th notes
        else if (params.subdivision > 0.35f)  subdivisionMultiplier = 2.0f;  // 8th notes
        else if (params.subdivision > 0.15f)  subdivisionMultiplier = 1.0f;  // Quarter notes

        subdivisionAccumulator += phaseIncrement * subdivisionMultiplier;

        //----------------------------------------------------------------------
        // Trigger detection — fire the pulse on phase wrap
        //----------------------------------------------------------------------

        bool mainBeatTrigger = (phase >= 1.0);
        bool subdivisionTrigger = (subdivisionAccumulator >= 1.0);

        if (mainBeatTrigger)
        {
            phase -= 1.0;
            subdivisionAccumulator = 0.0;
            triggerPulse (1.0f, params);  // Full accent on downbeat
        }
        else if (subdivisionTrigger && subdivisionMultiplier > 1.0f)
        {
            subdivisionAccumulator -= 1.0;

            // Subdivision accent: base level 0.4 + up to 0.3 from inverse accent.
            // Higher accent param = stronger downbeat contrast = quieter subdivisions.
            float subdivisionAccent = 0.4f + (1.0f - params.accent) * 0.3f;

            // Swing: odd-numbered subdivisions are attenuated by up to 30%,
            // creating the "lazy" triplet feel common in house/techno.
            if (swingBeatCounter & 1)
                subdivisionAccent *= (1.0f - params.swing * 0.3f);

            // Mask with 0x7FFFFFFF to prevent signed integer overflow
            swingBeatCounter = (swingBeatCounter + 1) & 0x7FFFFFFF;

            triggerPulse (subdivisionAccent, params);
        }

        //----------------------------------------------------------------------
        // Decay — the pulse fades between triggers
        //----------------------------------------------------------------------

        float perSampleDecayCoefficient = computeDecayCoefficient (params);
        pulseLevel *= perSampleDecayCoefficient;

        // Flush denormals: the exponential decay multiplication produces
        // subnormal floats as pulseLevel approaches zero. Without flushing,
        // these tiny values cause x86 FPU microcode traps that spike CPU
        // usage by 10-100x. Flushing to zero is inaudible and safe.
        pulseLevel = flushDenormal (pulseLevel);

        return pulseLevel;
    }

    bool isActive() const noexcept { return pulseLevel > 0.001f; }

private:
    static constexpr double kTwoPi = 6.28318530718;

    double cachedSampleRate = 44100.0;
    double phase = 0.0;                     // Main pulse phase [0, 1)
    double driftOscillatorPhase = 0.0;      // Slow organic drift LFO phase [0, 1)
    double subdivisionAccumulator = 0.0;    // Subdivision phase accumulator
    float  pulseLevel = 0.0f;               // Current pulse amplitude [0, 1]
    float  swingAccumulator = 0.0f;         // Unused — kept for binary compat
    int    swingBeatCounter = 0;            // Counts subdivisions for swing alternation
    OpticNoiseGen noiseGen;                 // PRNG for stochastic accent variation

    void triggerPulse (float accentLevel, const Params& params) noexcept
    {
        // Shape-dependent attack: at shape=0 (sharp kick), the pulse snaps
        // to full level instantly. At shape=1 (soft swell), it blends with
        // the current level for a gentler onset.
        float attackLevel = lerp (1.0f, 0.5f + pulseLevel * 0.5f, params.shape);
        pulseLevel = clamp (attackLevel * accentLevel, 0.0f, 1.0f);

        // Spectral flux adds stochastic accent variation: sudden spectral
        // changes (transients in the analyzed audio) make some pulses louder
        // or quieter, preventing robotic repetition. Clamped to +/-30%.
        if (params.evolve > 0.0f)
        {
            float fluxAccentBoost = clamp (params.visualFlux * 2.0f, -0.3f, 0.3f) * params.evolve;
            pulseLevel = clamp (pulseLevel + fluxAccentBoost, 0.0f, 1.0f);
        }
    }

    float computeDecayCoefficient (const Params& params) const noexcept
    {
        // Decay time controlled by shape: 30ms at shape=0 (sharp kick snap)
        // to 500ms at shape=1 (slow pad swell). These values were tuned to
        // match the feel of an 808 kick (short) and a pad LFO (long).
        float decayTimeMs = lerp (30.0f, 500.0f, params.shape);

        // Visual energy extends decay: louder spectral input makes pulses
        // ring up to 50% longer, creating a "sympathetic resonance" feel
        // where the pulse breathes with the input level.
        if (params.evolve > 0.0f)
            decayTimeMs *= (1.0f + params.visualEnergy * 0.5f * params.evolve);

        float decayTimeSamples = decayTimeMs * 0.001f * static_cast<float> (cachedSampleRate);
        if (decayTimeSamples < 1.0f) return 0.0f;

        // Per-sample decay coefficient for -60 dB total attenuation.
        // -60 dB = amplitude ratio of 0.001 = 10^(-60/20).
        // To reach 0.001 in N samples: coefficient = 0.001^(1/N).
        // ln(0.001) = -6.9078, so coefficient = exp(-6.9078 / N).
        // Using fastExp for audio-thread performance.
        return flushDenormal (fastExp (-6.9078f / decayTimeSamples));
    }
};

//==============================================================================
// OpticModOutputs — Lock-free modulation signal bus.
//
// The nervous system of the comb jelly: 8 modulation channels written on
// the audio thread and read (lock-free, relaxed ordering) by both the UI
// thread (for CRT-style visualization) and the coupling matrix (for
// cross-engine modulation).
//
// std::memory_order_relaxed is safe here because:
//   - Each channel is an independent float (no inter-channel ordering needed)
//   - The UI reads are purely cosmetic (a stale frame is invisible)
//   - Coupling reads happen within the same audio callback (same thread)
//
// Channel map:
//   0: AutoPulse level         4: Spectral centroid (brightness)
//   1: Bass energy (sub+bass)  5: Spectral flux (rate of change)
//   2: Mid energy              6: Total energy (loudness)
//   3: High energy             7: Transient detect (flux > threshold)
//==============================================================================
struct OpticModOutputs
{
    static constexpr int kNumModChannels = 8;
    std::atomic<float> values[kNumModChannels] = {};

    void store (int channelIndex, float value) noexcept
    {
        values[channelIndex].store (value, std::memory_order_relaxed);
    }

    float load (int channelIndex) const noexcept
    {
        return values[channelIndex].load (std::memory_order_relaxed);
    }

    //--------------------------------------------------------------------------
    // Named writers — audio thread
    //--------------------------------------------------------------------------
    void setPulse     (float v) noexcept { store (0, v); }
    void setBass      (float v) noexcept { store (1, v); }
    void setMid       (float v) noexcept { store (2, v); }
    void setHigh      (float v) noexcept { store (3, v); }
    void setCentroid  (float v) noexcept { store (4, v); }
    void setFlux      (float v) noexcept { store (5, v); }
    void setEnergy    (float v) noexcept { store (6, v); }
    void setTransient (float v) noexcept { store (7, v); }

    //--------------------------------------------------------------------------
    // Named readers — UI thread and coupling matrix
    //--------------------------------------------------------------------------
    float getPulse()     const noexcept { return load (0); }
    float getBass()      const noexcept { return load (1); }
    float getMid()       const noexcept { return load (2); }
    float getHigh()      const noexcept { return load (3); }
    float getCentroid()  const noexcept { return load (4); }
    float getFlux()      const noexcept { return load (5); }
    float getEnergy()    const noexcept { return load (6); }
    float getTransient() const noexcept { return load (7); }
};

//==============================================================================
//
// OpticEngine — The Bioluminescent Flash
//
// A modulation-only engine inspired by Winamp/MilkDrop audio visualizers
// (Ryan Geiss, 2001) and the sidechain-as-instrument philosophy of French
// house production (Daft Punk, Justice). OPTIC generates zero audio — it
// analyzes incoming sound from coupled engines, extracts 8 spectral
// features, and pulses those features back into the coupling matrix as
// modulation signals.
//
// The AutoPulse system adds a self-evolving rhythmic heartbeat whose
// character is shaped by the very spectrum it's analyzing — creating a
// feedback loop of light and rhythm, like a comb jelly synchronizing its
// bioluminescent flashes to the ocean's current.
//
// Unique among XOlokun engines: 0 voices, 0 audio output, 16 parameters.
// OPTIC's value is entirely in what it *does to other engines*.
//
// Key couplings:
//   OPTIC --> OBESE:    Pulse drives filter cutoff (pumping width)
//   OPTIC --> ODYSSEY:  Centroid drives JOURNEY macro (spectral journey)
//   OPTIC --> ONSET:    Transient detect triggers drum fills
//   Any   --> OPTIC:    Any engine's audio becomes the analysis source
//
// Accent: Phosphor Green #00FF41 (CRT phosphor glow)
// Param prefix: optic_
//
//==============================================================================
class OpticEngine : public SynthEngine
{
public:
    //==========================================================================
    //  L I F E C Y C L E
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        cachedSampleRate = sampleRate;

        // SRO SilenceGate: OPTIC is intentionally exempt from the zero-idle bypass
        // (B005: Zero-Audio Identity). It generates no audio — it is a pure modulation
        // engine that must run continuously to track spectral features even when
        // no notes are playing. The gate is prepared for contract completeness but
        // the bypass check is intentionally omitted from renderBlock(). This is the
        // same exemption as D006 aftertouch (Optic intentionally exempt — visual engine).
        prepareSilenceGate (sampleRate, maxBlockSize, 100.0f);

        analyzer.prepare (sampleRate);
        autoPulse.prepare (sampleRate);

        outputCacheLeft.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheRight.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        // Smoothing filters for modulation outputs.
        // 15 Hz lowpass prevents zipper noise (audible staircase artifacts)
        // when mod signals drive continuous parameters like filter cutoff.
        // 15 Hz was chosen as a compromise: fast enough to track kick transients
        // (~67ms response), slow enough to eliminate per-sample jitter.
        for (int i = 0; i < OpticModOutputs::kNumModChannels; ++i)
        {
            modulationSmoothingFilters[i].setMode (CytomicSVF::Mode::LowPass);
            modulationSmoothingFilters[i].setCoefficients (
                15.0f,   // 15 Hz smoothing cutoff
                0.0f,    // No resonance
                static_cast<float> (cachedSampleRate));
        }

        // Input smoother: 10 Hz lowpass on the coupling input.
        // Even lower than the mod smoothing because the coupling input is
        // an averaged RMS-like signal that shouldn't have fast transients.
        inputSmoothingFilter.setMode (CytomicSVF::Mode::LowPass);
        inputSmoothingFilter.setCoefficients (
            10.0f,   // 10 Hz input smoothing cutoff
            0.0f,    // No resonance
            static_cast<float> (cachedSampleRate));
    }

    void releaseResources() override
    {
        outputCacheLeft.clear();
        outputCacheRight.clear();
    }

    void reset() override
    {
        analyzer.reset();
        autoPulse.reset();
        for (int i = 0; i < OpticModOutputs::kNumModChannels; ++i)
            modulationSmoothingFilters[i].reset();
        inputSmoothingFilter.reset();

        couplingInputLevel = 0.0f;
        compositeEnvelopeOutput = 0.0f;
        std::fill (outputCacheLeft.begin(), outputCacheLeft.end(), 0.0f);
        std::fill (outputCacheRight.begin(), outputCacheRight.end(), 0.0f);
    }

    //==========================================================================
    //  A U D I O   R E N D E R I N G
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        // ---- D006: MIDI expression processing ----
        // OPTIC is a modulation engine with no voices, but it still responds to
        // MIDI expression for live performance control. Aftertouch boosts pulse
        // intensity and modulation depth; mod wheel scales reactivity and viz speed.
        for (const auto metadata : midi)
        {
            auto message = metadata.getMessage();
            if (message.isChannelPressure())
                midiAftertouchAmount = message.getChannelPressureValue() / 127.0f;
            else if (message.isController() && message.getControllerNumber() == 1)
                midiModWheelAmount = message.getControllerValue() / 127.0f;
        }

        // ---- ParamSnapshot: cache all parameter values once per block ----
        // This is the XOlokun standard pattern: read atomic parameter pointers
        // once at block start, then use local copies in the inner loop.
        // Avoids per-sample atomic loads (which defeat CPU branch prediction).

        const float reactivity       = (pReactivity  != nullptr) ? pReactivity->load()  : 0.7f;
        const float modulationDepth  = (pModDepth    != nullptr) ? pModDepth->load()    : 0.5f;
        const float pulseRate        = (pPulseRate   != nullptr) ? pPulseRate->load()   : 2.0f;
        const float pulseShape       = (pPulseShape  != nullptr) ? pPulseShape->load()  : 0.2f;
        const float pulseSwing       = (pPulseSwing  != nullptr) ? pPulseSwing->load()  : 0.0f;
        const float pulseEvolve      = (pPulseEvolve != nullptr) ? pPulseEvolve->load() : 0.5f;
        const float pulseSubdivision = (pPulseSubdiv != nullptr) ? pPulseSubdiv->load() : 0.0f;
        const float pulseAccent      = (pPulseAccent != nullptr) ? pPulseAccent->load() : 0.7f;
        const float autoPulseToggle  = (pAutoPulse   != nullptr) ? pAutoPulse->load()   : 0.0f;
        const float inputGain        = (pInputGain   != nullptr) ? pInputGain->load()   : 1.0f;
        const float modMixPulse      = (pModMixPulse != nullptr) ? pModMixPulse->load() : 0.5f;
        const float modMixSpectrum   = (pModMixSpec  != nullptr) ? pModMixSpec->load()  : 0.5f;

        const bool autoPulseEnabled = (autoPulseToggle >= 0.5f);

        // ---- D006: Apply MIDI expression to effective parameter values ----
        // Aftertouch: boosts mod depth by up to +0.3 and pulse rate by up to +4 Hz
        // (pressing harder makes the comb jelly pulse brighter and faster).
        // Mod wheel: boosts reactivity by up to +0.3 (wheel up = more sensitive analysis).
        const float effectiveModDepth    = clamp (modulationDepth + midiAftertouchAmount * 0.3f, 0.0f, 1.0f);
        const float effectivePulseRate   = clamp (pulseRate + midiAftertouchAmount * 4.0f, 0.5f, 16.0f);
        const float effectiveReactivity  = clamp (reactivity + midiModWheelAmount * 0.3f, 0.0f, 1.0f);

        // ---- Per-sample processing loop ----
        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            //------------------------------------------------------------------
            // Input acquisition: coupling bus + host sidechain
            //------------------------------------------------------------------

            float inputSample = couplingInputLevel;

            // Mix in any audio already present in the buffer (host sidechain routing)
            if (buffer.getNumChannels() > 0)
            {
                float leftChannel  = buffer.getSample (0, sampleIndex);
                float rightChannel = buffer.getNumChannels() > 1
                                     ? buffer.getSample (1, sampleIndex) : leftChannel;
                inputSample += (leftChannel + rightChannel) * 0.5f;  // Mono sum for analysis
            }

            inputSample *= inputGain * effectiveReactivity;
            inputSample = inputSmoothingFilter.processSample (inputSample);

            //------------------------------------------------------------------
            // Spectrum analysis — the comb jelly senses the water
            //------------------------------------------------------------------

            analyzer.processSample (inputSample);

            //------------------------------------------------------------------
            // AutoPulse — the bioluminescent heartbeat
            //------------------------------------------------------------------

            float pulseOutput = 0.0f;
            if (autoPulseEnabled)
            {
                OpticAutoPulse::Params pulseParams;
                pulseParams.rate            = effectivePulseRate;
                pulseParams.shape           = pulseShape;
                pulseParams.swing           = pulseSwing;
                pulseParams.evolve          = pulseEvolve;
                pulseParams.subdivision     = pulseSubdivision;
                pulseParams.accent          = pulseAccent;
                pulseParams.visualCentroid  = analyzer.getCentroid();

                // Scale total energy into [0, 1] range. The 5x multiplier was
                // tuned empirically: typical program material produces totalEnergy
                // around 0.05-0.2, so 5x maps this to usable 0.25-1.0 modulation range.
                pulseParams.visualEnergy = clamp (analyzer.getTotalEnergy() * 5.0f, 0.0f, 1.0f);
                pulseParams.visualFlux   = analyzer.getFlux();

                pulseOutput = autoPulse.process (pulseParams);
            }

            //------------------------------------------------------------------
            // Compute modulation outputs — 8 channels of spectral light
            //------------------------------------------------------------------

            // Band energy groupings with compensation gain.
            // Bass (bands 0+1) is scaled 4x because sub and bass frequencies
            // have inherently lower amplitude in most program material.
            // Mid (bands 2+3+4) is scaled 3x for similar level compensation.
            // High (bands 5+6+7) is scaled 5x because air/brilliance energy
            // is extremely low in level but perceptually significant.
            float bassEnergy = (analyzer.getBandEnergy (0) + analyzer.getBandEnergy (1)) * 4.0f;
            float midEnergy  = (analyzer.getBandEnergy (2) + analyzer.getBandEnergy (3)
                              + analyzer.getBandEnergy (4)) * 3.0f;
            float highEnergy = (analyzer.getBandEnergy (5) + analyzer.getBandEnergy (6)
                              + analyzer.getBandEnergy (7)) * 5.0f;

            float centroid   = analyzer.getCentroid();
            float flux       = analyzer.getFlux();
            float energy     = clamp (analyzer.getTotalEnergy() * 5.0f, 0.0f, 1.0f);

            // Transient detection: binary flag, fires when spectral flux exceeds
            // threshold. 0.05 was tuned to catch kick/snare onsets without
            // false-triggering on sustained pad movement.
            float transientDetect = (flux > 0.05f) ? 1.0f : 0.0f;

            // ---- Smooth modulation signals ----
            // Each mod channel gets its own lowpass to prevent zipper noise.
            // Note: pulse (ch 0), flux (ch 5), and transient (ch 7) are NOT
            // smoothed — they need fast response for rhythmic coupling.
            bassEnergy = modulationSmoothingFilters[1].processSample (bassEnergy);
            midEnergy  = modulationSmoothingFilters[2].processSample (midEnergy);
            highEnergy = modulationSmoothingFilters[3].processSample (highEnergy);
            centroid   = modulationSmoothingFilters[4].processSample (centroid);
            energy     = modulationSmoothingFilters[6].processSample (energy);

            // ---- Clamp to normalized [0, 1] range ----
            bassEnergy = clamp (bassEnergy, 0.0f, 1.0f);
            midEnergy  = clamp (midEnergy,  0.0f, 1.0f);
            highEnergy = clamp (highEnergy, 0.0f, 1.0f);
            centroid   = clamp (centroid,   0.0f, 1.0f);
            energy     = clamp (energy,     0.0f, 1.0f);

            //------------------------------------------------------------------
            // Store to atomic mod bus — the light pulses outward
            //------------------------------------------------------------------
            // The UI thread reads these for the CRT visualizer.
            // The coupling matrix reads these for cross-engine modulation.

            modOutputs.setPulse     (pulseOutput);
            modOutputs.setBass      (bassEnergy);
            modOutputs.setMid       (midEnergy);
            modOutputs.setHigh      (highEnergy);
            modOutputs.setCentroid  (centroid);
            modOutputs.setFlux      (flux);
            modOutputs.setEnergy    (energy);
            modOutputs.setTransient (transientDetect);

            //------------------------------------------------------------------
            // Composite coupling output — blended modulation for simple routing
            //------------------------------------------------------------------

            float couplingOutput = 0.0f;
            if (autoPulseEnabled)
                couplingOutput += pulseOutput * modMixPulse;
            couplingOutput += energy * modMixSpectrum;
            couplingOutput *= effectiveModDepth;
            couplingOutput = clamp (couplingOutput, 0.0f, 1.0f);

            // Cache for per-sample coupling reads by other engines
            auto bufferIndex = static_cast<size_t> (sampleIndex);
            if (bufferIndex < outputCacheLeft.size())
            {
                outputCacheLeft[bufferIndex]  = couplingOutput;
                outputCacheRight[bufferIndex] = couplingOutput;
            }
            compositeEnvelopeOutput = couplingOutput;
        }

        // OPTIC is a modulation engine — it outputs no audio.
        // Clear the buffer so downstream processing sees silence.
        buffer.clear();

        // Reset coupling accumulator for next block.
        // applyCouplingInput() will rebuild this before the next renderBlock().
        couplingInputLevel = 0.0f;
    }

    //==========================================================================
    //  C O U P L I N G  —  T H E   S Y M B I O T I C   N E R V O U S   S Y S T E M
    //==========================================================================

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        auto bufferIndex = static_cast<size_t> (sampleIndex);

        // Channel 0: Left composite modulation signal
        if (channel == 0 && bufferIndex < outputCacheLeft.size())
            return outputCacheLeft[bufferIndex];

        // Channel 1: Right composite modulation signal (identical to left — mono mod)
        if (channel == 1 && bufferIndex < outputCacheRight.size())
            return outputCacheRight[bufferIndex];

        // Channel 2: Composite envelope — for AmpToFilter, AmpToPitch on other engines
        if (channel == 2) return compositeEnvelopeOutput;

        // Channels 3-10: Individual mod outputs for extended coupling.
        // Maps to the 8 OpticModOutputs channels (pulse, bass, mid, high,
        // centroid, flux, energy, transient) for fine-grained routing.
        if (channel >= 3 && channel < 3 + OpticModOutputs::kNumModChannels)
            return modOutputs.load (channel - 3);

        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* sourceBuffer, int numSamples) override
    {
        // OPTIC receives audio from other engines for spectrum analysis.
        // Unlike most engines that use coupling to modify their own sound,
        // OPTIC uses it purely as an analysis source — the comb jelly
        // sensing the ocean's acoustic environment.
        switch (type)
        {
            case CouplingType::AudioToFM:
            case CouplingType::AudioToRing:
            case CouplingType::AudioToWavetable:
            case CouplingType::FilterToFilter:
            {
                // Sum the incoming audio block into a single level for analysis.
                // Using absolute value (rectification) to measure signal amplitude
                // regardless of polarity. The block average prevents single-sample
                // spikes from dominating.
                if (numSamples > 0 && sourceBuffer != nullptr)
                {
                    float rectifiedSum = 0.0f;
                    for (int i = 0; i < numSamples; ++i)
                        rectifiedSum += std::fabs (sourceBuffer[i]);
                    couplingInputLevel += (rectifiedSum / static_cast<float> (numSamples)) * amount;
                }
                break;
            }

            case CouplingType::AmpToFilter:
            case CouplingType::AmpToPitch:
                // Amplitude coupling adds a fixed 50% of the coupling amount
                // to the input level — a gentler influence than full audio routing.
                couplingInputLevel += amount * 0.5f;
                break;

            case CouplingType::RhythmToBlend:
                // Rhythm coupling adds 30% of the amount.
                // Future: use this to tempo-sync the AutoPulse.
                couplingInputLevel += amount * 0.3f;
                break;

            default:
                break;
        }
    }

    //==========================================================================
    //  P A R A M E T E R S  —  p r e f i x :  o p t i c _
    //==========================================================================

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl (params);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl (params);
        return { params.begin(), params.end() };
    }

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        //----------------------------------------------------------------------
        // Analysis section — how sensitively OPTIC reads the spectrum
        //----------------------------------------------------------------------

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_reactivity", 1 }, "Optic Reactivity",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_inputGain", 1 }, "Optic Signal Level",
            juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f), 1.0f));

        //----------------------------------------------------------------------
        // AutoPulse section — the self-evolving rhythmic heartbeat
        //----------------------------------------------------------------------

        // Default to On (index 1): OPTIC's entire identity is the autonomous heartbeat.
        // Booting with AutoPulse off means first contact is a silent analysis engine —
        // exactly the wrong first impression. On by default; Steady Glow preset overrides to Off.
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "optic_autoPulse", 1 }, "Optic AutoPulse",
            juce::StringArray { "Off", "On" }, 1));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_pulseRate", 1 }, "Optic Pulse Rate",
            juce::NormalisableRange<float> (0.5f, 16.0f, 0.01f, 0.35f), 2.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_pulseShape", 1 }, "Optic Pulse Shape",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.2f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_pulseSwing", 1 }, "Optic Pulse Swing",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_pulseEvolve", 1 }, "Optic Pulse Evolve",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_pulseSubdiv", 1 }, "Optic Pulse Subdivision",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_pulseAccent", 1 }, "Optic Pulse Accent",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

        //----------------------------------------------------------------------
        // Mod Output section — how OPTIC's light reaches other engines
        //----------------------------------------------------------------------

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_modDepth", 1 }, "Optic Mod Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_modMixPulse", 1 }, "Optic Pulse Blend",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_modMixSpec", 1 }, "Optic Spectral Blend",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        //----------------------------------------------------------------------
        // Visualizer section — CRT-style phosphor rendering for the UI
        //----------------------------------------------------------------------

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "optic_vizMode", 1 }, "Optic Viz Mode",
            juce::StringArray { "Scope", "Spectrum", "Milkdrop", "Particles" }, 2));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_vizFeedback", 1 }, "Optic Viz Feedback",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_vizSpeed", 1 }, "Optic Viz Speed",
            juce::NormalisableRange<float> (0.1f, 4.0f, 0.01f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "optic_vizIntensity", 1 }, "Optic Viz Intensity",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
    }

public:
    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pReactivity  = apvts.getRawParameterValue ("optic_reactivity");
        pInputGain   = apvts.getRawParameterValue ("optic_inputGain");
        pAutoPulse   = apvts.getRawParameterValue ("optic_autoPulse");
        pPulseRate   = apvts.getRawParameterValue ("optic_pulseRate");
        pPulseShape  = apvts.getRawParameterValue ("optic_pulseShape");
        pPulseSwing  = apvts.getRawParameterValue ("optic_pulseSwing");
        pPulseEvolve = apvts.getRawParameterValue ("optic_pulseEvolve");
        pPulseSubdiv = apvts.getRawParameterValue ("optic_pulseSubdiv");
        pPulseAccent = apvts.getRawParameterValue ("optic_pulseAccent");
        pModDepth    = apvts.getRawParameterValue ("optic_modDepth");
        pModMixPulse = apvts.getRawParameterValue ("optic_modMixPulse");
        pModMixSpec  = apvts.getRawParameterValue ("optic_modMixSpec");
        pVizMode     = apvts.getRawParameterValue ("optic_vizMode");
        pVizFeedback = apvts.getRawParameterValue ("optic_vizFeedback");
        pVizSpeed    = apvts.getRawParameterValue ("optic_vizSpeed");
        pVizIntensity = apvts.getRawParameterValue ("optic_vizIntensity");
    }

    //==========================================================================
    //  I D E N T I T Y
    //==========================================================================

    juce::String getEngineId() const override { return "Optic"; }

    // Phosphor Green #00FF41 — the color of CRT phosphor glow and
    // bioluminescent comb jellies in the sunlit shallows.
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF00FF41); }

    // OPTIC is a modulation engine — it has no voice engine, no oscillators,
    // no audio output. Zero voices reflects its pure-analysis identity.
    int getMaxVoices() const override { return 0; }

    //==========================================================================
    //  P U B L I C   A C C E S S  —  U I   V I S U A L I Z E R
    //==========================================================================

    const OpticModOutputs& getModOutputs() const noexcept { return modOutputs; }
    const OpticBandAnalyzer& getAnalyzer() const noexcept { return analyzer; }

private:
    //==========================================================================
    //  S T A T E
    //==========================================================================

    double cachedSampleRate = 44100.0;

    // ---- DSP modules ----
    OpticBandAnalyzer analyzer;                                        // 8-band spectral decomposition
    OpticAutoPulse    autoPulse;                                       // Self-evolving rhythm generator
    OpticModOutputs   modOutputs;                                      // Lock-free mod signal bus (8 channels)
    CytomicSVF        modulationSmoothingFilters[OpticModOutputs::kNumModChannels];  // Anti-zipper lowpass per mod channel
    CytomicSVF        inputSmoothingFilter;                            // Coupling input smoother

    // ---- Coupling state ----
    float couplingInputLevel = 0.0f;       // Accumulated input from other engines (reset per block)
    float compositeEnvelopeOutput = 0.0f;  // Last composite mod value (for envelope coupling reads)

    // ---- D006: MIDI expression state ----
    // Aftertouch boosts pulse intensity/rate; mod wheel boosts analysis reactivity.
    float midiAftertouchAmount = 0.0f;     // Channel pressure [0, 1]
    float midiModWheelAmount   = 0.0f;     // CC#1 mod wheel [0, 1]

    // ---- Output cache for per-sample coupling reads ----
    std::vector<float> outputCacheLeft;    // Composite mod signal, left channel
    std::vector<float> outputCacheRight;   // Composite mod signal, right channel (identical — mono mod)

    // ---- Parameter pointers ----
    // Cached from APVTS for zero-cost audio-thread reads.
    // The ParamSnapshot pattern: these are read once per block in renderBlock(),
    // stored in local variables, and used throughout the inner loop without
    // touching the atomic pointers again.
    std::atomic<float>* pReactivity   = nullptr;
    std::atomic<float>* pInputGain    = nullptr;
    std::atomic<float>* pAutoPulse    = nullptr;
    std::atomic<float>* pPulseRate    = nullptr;
    std::atomic<float>* pPulseShape   = nullptr;
    std::atomic<float>* pPulseSwing   = nullptr;
    std::atomic<float>* pPulseEvolve  = nullptr;
    std::atomic<float>* pPulseSubdiv  = nullptr;
    std::atomic<float>* pPulseAccent  = nullptr;
    std::atomic<float>* pModDepth     = nullptr;
    std::atomic<float>* pModMixPulse  = nullptr;
    std::atomic<float>* pModMixSpec   = nullptr;
    std::atomic<float>* pVizMode      = nullptr;
    std::atomic<float>* pVizFeedback  = nullptr;
    std::atomic<float>* pVizSpeed     = nullptr;
    std::atomic<float>* pVizIntensity = nullptr;
};

} // namespace xolokun
