#pragma once

#include <cmath>
#include <algorithm>
#include <array>
#include "OcelotParamSnapshot.h"
#include "BiomeMorph.h"

namespace xocelot {

// OcelotCanopy — Spectral Additive Pad Synth
//
// Core: complex oscillator with Buchla-style wavefold + 8 partials + shimmer feedback.
// Shimmer feedback clamped at 0.88 max to prevent runaway regardless of user value.
//
// Biome transforms: Underwater = deep/dark (low partials dominant, slow tidal breathe)
//                   Winter = sparse/wide (high partials, wind gust breathe, cold detuning)
//
// TODO Phase 2: Full additive partial bank + wavefolder.
// Stub: sine bank with wavefold approximation.

class OcelotCanopy
{
public:
    static constexpr int kMaxPartials = 8;
    static constexpr float kMaxShimmerFeedback = 0.88f; // hard cap, no matter what user sets

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        partialPhases.fill(0.0f);
        shimmerBuffer.fill(0.0f);
        shimmerReadHead = 0.0f;
        shimmerWriteHead = 0;
        breathePhase = 0.0f;
        lastAmplitude = 0.0f;
        lastSpectral  = 0.0f;
    }

    void noteOn(int midiNote, float velocity)
    {
        baseNote     = midiNote;
        baseVelocity = velocity;
        active       = true;
    }

    void noteOff() { active = false; }

    float renderBlock(float* outL, float* outR, int numSamples,
                      const OcelotParamSnapshot& snap,
                      const BiomeProfile& biome,
                      const struct StrataModulation& mod)
    {
        if (!active)
        {
            std::fill(outL, outL + numSamples, 0.0f);
            std::fill(outR, outR + numSamples, 0.0f);
            return 0.0f;
        }

        // Base frequency from MIDI note
        float pitchOffset = (snap.canopyPitch - 0.5f) * 48.0f; // ±24 cents
        float baseFreq = 440.0f * std::pow(2.0f, (baseNote - 69 + pitchOffset / 100.0f) / 12.0f);

        // Active partials (biome can tilt balance)
        int numPartials = std::clamp(snap.canopyPartials, 1, kMaxPartials);

        // Shimmer: clamp user value with biome additive, hard cap at kMaxShimmerFeedback
        float shimmerAmt = std::clamp(snap.canopyShimmer + mod.canopyShimmerMod, 0.0f, 1.0f);
        float shimmerFB  = shimmerAmt * kMaxShimmerFeedback;

        // Spectral filter position (modulated by floor via matrix)
        float spectralPos = std::clamp(snap.canopySpectralFilter + mod.canopyFilterMod, 0.0f, 1.0f);
        float spectralCutoff = 200.0f * std::pow(100.0f, spectralPos); // 200Hz–20kHz log

        // Wavefold depth
        float wavefold = std::clamp(snap.canopyWavefold + mod.canopyMorphMod, 0.0f, 1.0f);

        // Breathe rate (modulated by biome period multiplier)
        float breatheRate = 0.3f / biome.canopyBreathePeriod;

        float detune     = std::clamp(snap.canopyDetune + mod.canopyMorphMod * 0.3f, 0.0f, 1.0f);
        float sumSq      = 0.0f;
        float spectralSum= 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            // Breathe LFO
            breathePhase += breatheRate / static_cast<float>(sr);
            if (breathePhase > 1.0f) breathePhase -= 1.0f;
            float breathe = 0.5f + 0.5f * std::sin(breathePhase * juce::MathConstants<float>::twoPi);

            // Sum partials
            float sample = 0.0f;
            for (int p = 0; p < numPartials; ++p)
            {
                float harmonic = static_cast<float>(p + 1);
                float partialFreq = baseFreq * harmonic
                                  * (1.0f + detune * (p % 2 == 0 ? 0.001f : -0.001f));

                // Apply biome partial tilt (negative = boost lows, positive = boost highs)
                float partialLevel = 1.0f / harmonic;
                partialLevel *= (1.0f + biome.canopyPartialTilt * ((harmonic - 1.0f) / kMaxPartials));
                partialLevel = std::max(0.0f, partialLevel);

                // Spectral filter: soft LP proportional to partialFreq vs cutoff
                float filterAtten = 1.0f / (1.0f + (partialFreq / spectralCutoff) * (partialFreq / spectralCutoff));
                partialLevel *= filterAtten;

                partialPhases[static_cast<size_t>(p)] += partialFreq / static_cast<float>(sr);
                if (partialPhases[static_cast<size_t>(p)] > 1.0f)
                    partialPhases[static_cast<size_t>(p)] -= 1.0f;

                float s = std::sin(partialPhases[static_cast<size_t>(p)]
                                   * juce::MathConstants<float>::twoPi);
                sample += s * partialLevel;
                spectralSum += partialFreq * partialLevel;
            }

            // Normalize by partial count
            sample /= static_cast<float>(numPartials);

            // Wavefold (soft Buchla-style — fold back above threshold)
            if (wavefold > 0.01f)
            {
                float threshold = 1.0f - wavefold * 0.85f;
                while (sample > threshold)  sample = threshold * 2.0f - sample;
                while (sample < -threshold) sample = -threshold * 2.0f - sample;
            }

            // Shimmer feedback (pitch-shifted +1 octave, read from delay buffer)
            int sWriteInt = shimmerWriteHead % static_cast<int>(shimmerBuffer.size());
            shimmerBuffer[static_cast<size_t>(sWriteInt)] = sample;
            shimmerWriteHead = (shimmerWriteHead + 1) % static_cast<int>(shimmerBuffer.size());

            // Shimmer reads at half-speed (doubles frequency = +1 oct) with feedback
            shimmerReadHead += 0.5f;
            if (shimmerReadHead >= static_cast<float>(shimmerBuffer.size()))
                shimmerReadHead -= static_cast<float>(shimmerBuffer.size());
            float shimmerSample = shimmerBuffer[static_cast<size_t>(
                static_cast<int>(shimmerReadHead) % static_cast<int>(shimmerBuffer.size()))];

            sample += shimmerSample * shimmerFB;

            // Breathe + level
            sample *= breathe * snap.canopyLevel * baseVelocity;

            outL[i] = sample;
            outR[i] = sample * (1.0f + detune * 0.1f); // slight stereo spread via detune
            sumSq  += sample * sample;
        }

        lastAmplitude = std::sqrt(sumSq / static_cast<float>(numSamples));
        lastSpectral  = (numSamples > 0 && lastAmplitude > 0.001f)
                       ? std::clamp(spectralSum / (static_cast<float>(numSamples) * 10000.0f), 0.0f, 1.0f)
                       : 0.0f;
        return lastAmplitude;
    }

    bool  isActive() const          { return active; }
    float getLastAmplitude() const  { return lastAmplitude; }
    float getLastSpectral() const   { return lastSpectral; }

private:
    double sr = 44100.0;
    std::array<float, kMaxPartials> partialPhases;
    std::array<float, 4096> shimmerBuffer; // ~93ms at 44.1k
    float shimmerReadHead   = 0.0f;
    int   shimmerWriteHead  = 0;
    float breathePhase      = 0.0f;
    float lastAmplitude     = 0.0f;
    float lastSpectral      = 0.0f;
    bool  active            = false;
    int   baseNote          = 60;
    float baseVelocity      = 0.8f;
};

} // namespace xocelot
