#pragma once
#include <cmath>
#include <algorithm>
#include <array>

namespace xoceanus {

//==============================================================================
// WavetableScanMode — PPG Wave-inspired harsh-digital wavetable scanning.
//
// The PPG Wave (1981) defined wavetable synthesis with its distinctive
// digital aliasing and steppy transitions between waveforms. Modern synths
// clean this up, but the PPG's character IS the aliasing. This module
// recreates that specific harsh-digital-but-musical quality.
//
// Modes:
//   PPG Classic — 8-bit quantized scanning with inter-wave aliasing
//   PPG Smooth  — Same waves but with interpolation (modern interpretation)
//   Bitcrush    — Variable bit depth on the wavetable readout
//
// The module processes an existing audio signal through wavetable
// remapping, using the signal amplitude as a lookup index into
// a PPG-style wave sequence.
//
// Controls:
//   scanPosition: 0..1 — Manual position in wavetable (overridable by mod)
//   scanSpeed:    0..1 — Rate of automatic scanning
//   quantize:     0..1 — Bit depth reduction on readout (0 = clean, 1 = 8-bit)
//   interpolation: 0..1 — Inter-wave smoothing (0 = PPG steppy, 1 = modern smooth)
//   mix:          0..1 — Wet/dry blend
//
// Inspired by: PPG Wave 2.2/2.3 (1981), Waldorf Microwave, Prophet VS
//==============================================================================
class WavetableScanMode
{
public:
    WavetableScanMode() = default;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        generatePPGWaves();
        reset();
    }

    void setScanPosition (float p)     { scanPos       = std::clamp (p, 0.0f, 1.0f); }
    void setScanSpeed (float s)        { scanSpeed     = std::clamp (s, 0.0f, 1.0f); }
    void setQuantize (float q)         { quantize      = std::clamp (q, 0.0f, 1.0f); }
    void setInterpolation (float i)    { interpolation = std::clamp (i, 0.0f, 1.0f); }
    void setMix (float m)              { mix           = std::clamp (m, 0.0f, 1.0f); }

    void processBlock (float* left, float* right, int numSamples)
    {
        if (mix < 0.001f)
            return;

        for (int i = 0; i < numSamples; ++i)
        {
            // Advance scan position (auto-scanning)
            if (scanSpeed > 0.001f)
            {
                autoScanPhase += scanSpeed * 0.5f / static_cast<float> (sr);
                if (autoScanPhase >= 1.0f) autoScanPhase -= 1.0f;
            }

            float effectivePos = std::fmod (scanPos + autoScanPhase, 1.0f);

            // Map position to wavetable index
            float tableIdxF = effectivePos * static_cast<float> (kNumWaves - 1);
            int tableIdx0 = static_cast<int> (tableIdxF);
            int tableIdx1 = std::min (tableIdx0 + 1, kNumWaves - 1);
            float tableFrac = tableIdxF - static_cast<float> (tableIdx0);

            // PPG mode: step between waves (no interpolation at interp=0)
            if (interpolation < 0.999f)
                tableFrac = tableFrac * interpolation + (tableFrac > 0.5f ? 1.0f : 0.0f) * (1.0f - interpolation);

            float dryL = left[i];
            float dryR = right[i];

            // Use input signal phase (extracted from zero crossings) as wave index
            float phaseL = signalToPhase (left[i], phasePrevL);
            float phaseR = signalToPhase (right[i], phasePrevR);
            phasePrevL = left[i];
            phasePrevR = right[i];

            // Look up in wavetable
            float wetL = lookupWave (tableIdx0, tableIdx1, tableFrac, phaseL);
            float wetR = lookupWave (tableIdx0, tableIdx1, tableFrac, phaseR);

            // Preserve amplitude envelope from original signal
            float envL = std::abs (left[i]);
            float envR = std::abs (right[i]);
            wetL *= envL;
            wetR *= envR;

            // Quantize (bit depth reduction for PPG character)
            if (quantize > 0.001f)
            {
                float levels = 256.0f - quantize * 240.0f; // 256 → 16 levels
                wetL = std::round (wetL * levels) / levels;
                wetR = std::round (wetR * levels) / levels;
            }

            left[i]  = dryL + mix * (wetL - dryL);
            right[i] = dryR + mix * (wetR - dryR);
        }
    }

    void reset()
    {
        autoScanPhase = 0.0f;
        phasePrevL = phasePrevR = 0.0f;
        phaseAccumL = phaseAccumR = 0.0f;
    }

private:
    static constexpr int kNumWaves = 8;
    static constexpr int kWaveSize = 256;

    double sr = 44100.0;
    float scanPos       = 0.0f;
    float scanSpeed     = 0.0f;
    float quantize      = 0.3f;
    float interpolation = 0.0f;
    float mix           = 1.0f;

    float autoScanPhase = 0.0f;
    float phasePrevL = 0.0f, phasePrevR = 0.0f;
    float phaseAccumL = 0.0f, phaseAccumR = 0.0f;

    // PPG-style wavetable: 8 waves × 256 samples
    std::array<std::array<float, kWaveSize>, kNumWaves> waves;

    void generatePPGWaves()
    {
        // PPG-inspired wave sequence: from pure sine through increasingly
        // complex/harsh shapes, mimicking the PPG Wave 2.2's wave sequence
        for (int w = 0; w < kNumWaves; ++w)
        {
            float complexity = static_cast<float> (w) / static_cast<float> (kNumWaves - 1);
            int numHarmonics = 1 + static_cast<int> (complexity * 31.0f);

            for (int s = 0; s < kWaveSize; ++s)
            {
                float phase = static_cast<float> (s) / static_cast<float> (kWaveSize) * 2.0f * kPi;
                float sample = 0.0f;

                for (int h = 1; h <= numHarmonics; ++h)
                {
                    // PPG-style harmonic weighting: odd harmonics stronger,
                    // with increasing harshness at higher wave indices
                    float harmGain;
                    if (h % 2 == 1)
                        harmGain = 1.0f / static_cast<float> (h);
                    else
                        harmGain = complexity / (static_cast<float> (h) * 1.5f);

                    sample += harmGain * std::sin (phase * static_cast<float> (h));
                }

                // Normalize
                waves[static_cast<size_t> (w)][static_cast<size_t> (s)] = sample;
            }

            // Normalize each wave to ±1
            float maxVal = 0.001f;
            for (auto& s : waves[static_cast<size_t> (w)])
                maxVal = std::max (maxVal, std::abs (s));
            for (auto& s : waves[static_cast<size_t> (w)])
                s /= maxVal;
        }
    }

    float lookupWave (int idx0, int idx1, float frac, float phase) const
    {
        // Map phase (0-1) to wave sample index
        float posF = phase * static_cast<float> (kWaveSize);
        int pos0 = static_cast<int> (posF) & (kWaveSize - 1);
        int pos1 = (pos0 + 1) & (kWaveSize - 1);
        float posFrac = posF - std::floor (posF);

        // Read from both waves
        float s0 = waves[static_cast<size_t> (idx0)][static_cast<size_t> (pos0)] * (1.0f - posFrac)
                  + waves[static_cast<size_t> (idx0)][static_cast<size_t> (pos1)] * posFrac;
        float s1 = waves[static_cast<size_t> (idx1)][static_cast<size_t> (pos0)] * (1.0f - posFrac)
                  + waves[static_cast<size_t> (idx1)][static_cast<size_t> (pos1)] * posFrac;

        // Crossfade between waves
        return s0 * (1.0f - frac) + s1 * frac;
    }

    float signalToPhase (float sample, float prev)
    {
        // Simple phase tracking via zero-crossing detection + accumulator
        // This is an approximation — works well for tonal signals
        bool zeroCross = (prev < 0.0f && sample >= 0.0f);

        if (zeroCross)
            phaseAccumL = 0.0f;  // Reset on positive zero crossing

        // Estimate frequency from signal characteristics
        // Advance phase at a reasonable rate
        float freqEstimate = 440.0f;  // Fallback; could be improved with pitch detection
        phaseAccumL += freqEstimate / static_cast<float> (sr);
        if (phaseAccumL > 1.0f) phaseAccumL -= 1.0f;

        return phaseAccumL;
    }

    static constexpr float kPi = 3.14159265358979f;
};

} // namespace xoceanus
