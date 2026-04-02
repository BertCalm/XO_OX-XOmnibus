// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/ShaperEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/CytomicSVF.h"
#include <array>
#include <cmath>

namespace xoceanus {

//==============================================================================
// ObserveShaper — The Mantis Shrimp
//
// 6-band parametric EQ with per-band feliX↔Oscar character axis.
// feliX = surgical, minimum phase, zero color.
// Oscar = transformer iron, tanh saturation, harmonic richness.
//
// Each band: freq, gain, Q, type, character, bypass, dynamic enable, tide enable
// Global: input/output gain, mix, phase mode, analyzer, oversampling, coupling
//
// Gallery code: OBSERVE | Accent: Spectral Amber #E8A020 | Prefix: obs_
//
// Coupling output:
//   ch0-5: per-band RMS energy (spectral intelligence signal)
//   Other engines can respond to what the EQ is "seeing"
//
//==============================================================================

//==============================================================================
// ObserveBand — one of 6 parametric EQ bands
//==============================================================================
struct ObserveBand
{
    CytomicSVF filter;
    CytomicSVF tiltHighShelf;  // Second filter used only in Tilt mode (type 11)
    float character = 0.0f;       // 0 = feliX (clinical), 1 = Oscar (warm)

    // Transformer iron emulation state (Oscar mode)
    float ironStateX = 0.0f;
    float ironStateY = 0.0f;

    // Tide LFO state
    float tidePhase = 0.0f;
    float tideOut = 0.0f;

    // RMS energy tracker (for coupling output)
    float rmsEnergy = 0.0f;
    float rmsSmooth = 0.0f;

    // Dynamic EQ state
    float dynEnvelope = 0.0f;

    void reset()
    {
        filter.reset();
        tiltHighShelf.reset();
        ironStateX = ironStateY = 0.0f;
        tidePhase = 0.0f;
        tideOut = 0.0f;
        rmsEnergy = rmsSmooth = 0.0f;
        dynEnvelope = 0.0f;
    }
};

//==============================================================================
// ObserveShaper
//==============================================================================
class ObserveShaper : public ShaperEngine
{
public:
    static constexpr int kNumBands = 6;

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int /*maxBlockSize*/) override
    {
        sr = static_cast<float> (sampleRate);
        for (auto& b : bands) b.reset();
        silenceGate.prepare (sr, 100); // 100ms hold — no tail
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& b : bands) b.reset();
    }

    //==========================================================================
    // Audio — The Spectral Sculptor
    //==========================================================================

    void processBlock (juce::AudioBuffer<float>& buffer,
                       juce::MidiBuffer& /*midi*/,
                       int numSamples) override
    {
        if (numSamples <= 0 || isBypassed()) return;

        // Silence gate: skip all DSP when input has been silent long enough
        if (silenceGate.isBypassed())
        {
            buffer.clear();
            return;
        }

        // ParamSnapshot
        const float inGain  = loadP (pInputGain, 0.0f);
        const float outGain = loadP (pOutputGain, 0.0f);
        const float mix     = loadP (pMix, 1.0f);
        const float charGlobal = loadP (pCharacterGlobal, 0.0f);

        // dB to linear
        const float inLin  = dbToGain (inGain);
        const float outLin = dbToGain (outGain);

        // Per-band params
        struct BandSnap {
            float freq, gain, q, character;
            int type;
            bool bypass, tideEnable;
            float tideRate, tideDepth;
            int tideTarget;
        };

        BandSnap snap[kNumBands];
        for (int b = 0; b < kNumBands; ++b)
        {
            snap[b].freq      = loadP (pBandFreq[b], kDefaultFreqs[b]);
            snap[b].gain      = loadP (pBandGain[b], 0.0f);
            snap[b].q         = loadP (pBandQ[b], 0.707f);
            snap[b].type      = static_cast<int> (loadP (pBandType[b], 0.0f));
            snap[b].character = clamp (loadP (pBandCharacter[b], 0.0f) + charGlobal, 0.0f, 1.0f);
            snap[b].bypass    = loadP (pBandBypass[b], 0.0f) > 0.5f;
            snap[b].tideEnable = loadP (pBandTideEnable[b], 0.0f) > 0.5f;
            snap[b].tideRate  = loadP (pBandTideRate[b], 0.5f);
            snap[b].tideDepth = loadP (pBandTideDepth[b], 0.0f);
            snap[b].tideTarget = static_cast<int> (loadP (pBandTideTarget[b], 1.0f));
        }

        float* L = buffer.getWritePointer (0);
        float* R = buffer.getNumChannels() >= 2 ? buffer.getWritePointer (1) : nullptr;

        for (int s = 0; s < numSamples; ++s)
        {
            // Input gain
            float sampleL = L[s] * inLin;
            float sampleR = R ? R[s] * inLin : sampleL;
            float dryL = sampleL, dryR = sampleR;

            // Process each band
            for (int b = 0; b < kNumBands; ++b)
            {
                if (snap[b].bypass) continue;

                auto& band = bands[b];
                band.character = snap[b].character;

                // Tide LFO modulation
                float freqMod = 0.0f, gainMod = 0.0f, qMod = 0.0f, charMod = 0.0f;
                if (snap[b].tideEnable && snap[b].tideDepth > 0.001f)
                {
                    band.tidePhase += snap[b].tideRate / sr;
                    if (band.tidePhase >= 1.0f) band.tidePhase -= 1.0f;
                    band.tideOut = fastSin (band.tidePhase * 6.28318f) * snap[b].tideDepth;

                    switch (snap[b].tideTarget)
                    {
                        case 0: freqMod = band.tideOut * snap[b].freq * 0.5f; break; // Freq
                        case 1: gainMod = band.tideOut * 6.0f; break;                 // Gain (±6dB)
                        case 2: qMod   = band.tideOut * 2.0f; break;                  // Q
                        case 3: charMod = band.tideOut * 0.3f; break;                 // Character
                        default: break;
                    }
                }

                // Apply coupling modulation
                freqMod += couplingFreqMod[b];
                gainMod += couplingGainMod[b];

                float effFreq = clamp (snap[b].freq + freqMod, 20.0f, 22000.0f);
                float effGain = snap[b].gain + gainMod;
                float effQ    = clamp (snap[b].q + qMod, 0.1f, 20.0f);
                float effChar = clamp (snap[b].character + charMod, 0.0f, 1.0f);

                // Set filter coefficients.
                // Tilt (type 11) uses two shelf filters; all others use a single filter.
                const bool isTilt = (snap[b].type == 11);
                if (isTilt)
                {
                    // Tilt EQ: LowShelf at +effGain/2 dB and HighShelf at -effGain/2 dB,
                    // both centred on effFreq. Combined they tilt the spectrum around the
                    // pivot frequency — boosting lows while cutting highs (or vice versa).
                    band.filter.setMode (CytomicSVF::Mode::LowShelf);
                    band.filter.setCoefficients (effFreq, effQ, sr, effGain * 0.5f);
                    band.tiltHighShelf.setMode (CytomicSVF::Mode::HighShelf);
                    band.tiltHighShelf.setCoefficients (effFreq, effQ, sr, -effGain * 0.5f);
                }
                else
                {
                    setFilterType (band.filter, snap[b].type);
                    band.filter.setCoefficients (effFreq, effQ, sr, effGain);
                }

                // Apply gain as a post-filter scale for non-shelf, non-tilt types
                // (shelf/tilt bake gain into coefficients; LP/HP/BP/Notch/Peak scale output).
                float gainLin = (isTilt
                                 || snap[b].type == 2 || snap[b].type == 3)  // Low/High Shelf
                                ? 1.0f
                                : dbToGain (effGain);

                // Process L
                float bandL;
                if (isTilt)
                    bandL = band.tiltHighShelf.processSample (band.filter.processSample (sampleL));
                else
                    bandL = band.filter.processSample (sampleL) * gainLin;

                // Process R (filter is stereo-linked for now)
                float bandR;
                if (R)
                {
                    if (isTilt)
                        bandR = band.tiltHighShelf.processSample (band.filter.processSample (sampleR));
                    else
                        bandR = band.filter.processSample (sampleR) * gainLin;
                }
                else
                {
                    bandR = bandL;
                }

                // Oscar character: tanh saturation on the band output
                if (effChar > 0.05f)
                {
                    float drive = 1.0f + effChar * 4.0f;
                    bandL = fastTanh (bandL * drive) / drive;
                    bandR = fastTanh (bandR * drive) / drive;

                    // Transformer iron HP emulation (matched-Z, 3 Hz)
                    if (effChar > 0.3f)
                    {
                        float ironCoeff = std::exp (-6.28318f * 3.0f / sr);
                        float ironOutL = bandL - ironCoeff * band.ironStateX + ironCoeff * band.ironStateY;
                        band.ironStateX = bandL;
                        band.ironStateY = ironOutL;
                        bandL = ironOutL;
                    }
                }

                // Serial EQ topology: each band processes the output of the previous band.
                // This is the standard parametric EQ model — bands are chained, not summed.
                sampleL = bandL;
                sampleR = bandR;

                // RMS energy tracking for coupling output
                float energy = bandL * bandL + (R ? bandR * bandR : bandL * bandL);
                band.rmsSmooth += (energy - band.rmsSmooth) * 0.001f;
                band.rmsEnergy = flushDenormal (band.rmsSmooth);
            }

            // Output gain
            sampleL *= outLin;
            sampleR *= outLin;

            // Dry/wet mix
            L[s] = dryL * (1.0f - mix) + sampleL * mix;
            if (R) R[s] = dryR * (1.0f - mix) + sampleR * mix;
        }

        // Update silence gate — analyzes output to detect sustained silence
        silenceGate.analyzeBlock (buffer.getReadPointer (0),
                                  buffer.getNumChannels() >= 2 ? buffer.getReadPointer (1) : nullptr,
                                  numSamples);

        // Store coupling output (per-band energy)
        for (int b = 0; b < kNumBands; ++b)
            bandEnergyCoupling[b] = bands[b].rmsEnergy;

        // Reset coupling accumulators
        for (int b = 0; b < kNumBands; ++b)
        {
            couplingFreqMod[b] = 0.0f;
            couplingGainMod[b] = 0.0f;
        }
    }

    //==========================================================================
    // Coupling — spectral intelligence output
    //==========================================================================

    float getSampleForCoupling (int channel, int /*sampleIndex*/) const override
    {
        // Channels 0-5: per-band RMS energy
        if (channel >= 0 && channel < kNumBands)
            return bandEnergyCoupling[channel];
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float*, int) override
    {
        // Distribute coupling across bands based on type
        switch (type)
        {
            case CouplingType::AmpToFilter:
                for (int b = 0; b < kNumBands; ++b)
                    couplingFreqMod[b] += amount * 500.0f; // Shift all bands
                break;
            case CouplingType::LFOToPitch:
                for (int b = 0; b < kNumBands; ++b)
                    couplingFreqMod[b] += amount * 200.0f;
                break;
            default: break;
        }
    }

    //==========================================================================
    // Parameters — 6 bands × 9 params + globals
    //==========================================================================

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        using PF = juce::AudioParameterFloat;
        using PC = juce::AudioParameterChoice;
        using PB = juce::AudioParameterBool;

        auto bandTypes = juce::StringArray { "Peak", "Notch", "Low Shelf", "High Shelf",
                                              "LP6", "LP12", "LP24", "HP6", "HP12", "HP24",
                                              "Band Pass", "Tilt" };
        auto tideShapes = juce::StringArray { "Sine", "Triangle", "Saw", "Rev Saw",
                                               "Square", "S&H", "Drift" };
        auto tideTargets = juce::StringArray { "Frequency", "Gain", "Q", "Character" };

        // Per-band parameters (6 × 9 = 54)
        for (int b = 0; b < kNumBands; ++b)
        {
            auto id = [&](const char* suffix) { return juce::String ("obs_b") + juce::String (b + 1) + "_" + suffix; };
            auto nm = [&](const char* suffix) { return juce::String ("Observe Band ") + juce::String (b + 1) + " " + suffix; };

            params.push_back (std::make_unique<PF> (juce::ParameterID { id ("freq"), 1 }, nm ("Freq"),
                juce::NormalisableRange<float> (20.0f, 22000.0f, 0.1f, 0.3f), kDefaultFreqs[b]));
            params.push_back (std::make_unique<PF> (juce::ParameterID { id ("gain"), 1 }, nm ("Gain"),
                juce::NormalisableRange<float> (-18.0f, 18.0f, 0.1f), 0.0f));
            params.push_back (std::make_unique<PF> (juce::ParameterID { id ("q"), 1 }, nm ("Q"),
                juce::NormalisableRange<float> (0.1f, 20.0f, 0.01f, 0.3f), 0.707f));
            params.push_back (std::make_unique<PC> (juce::ParameterID { id ("type"), 1 }, nm ("Type"),
                bandTypes, 0));
            params.push_back (std::make_unique<PF> (juce::ParameterID { id ("character"), 1 }, nm ("Character"),
                juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
            params.push_back (std::make_unique<PF> (juce::ParameterID { id ("bypass"), 1 }, nm ("Bypass"),
                juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f), 0.0f));
            params.push_back (std::make_unique<PF> (juce::ParameterID { id ("tide_enable"), 1 }, nm ("Tide Enable"),
                juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f), 0.0f));
            params.push_back (std::make_unique<PF> (juce::ParameterID { id ("tide_rate"), 1 }, nm ("Tide Rate"),
                juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.3f), 0.5f));
            params.push_back (std::make_unique<PF> (juce::ParameterID { id ("tide_depth"), 1 }, nm ("Tide Depth"),
                juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
            params.push_back (std::make_unique<PC> (juce::ParameterID { id ("tide_target"), 1 }, nm ("Tide Target"),
                tideTargets, 1));
        }

        // Global parameters (10)
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obs_input_gain", 1 }, "Observe Input Gain",
            juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obs_output_gain", 1 }, "Observe Output Gain",
            juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obs_mix", 1 }, "Observe Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obs_character_global", 1 }, "Observe Character Global",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        return { params.begin(), params.end() };
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pInputGain = apvts.getRawParameterValue ("obs_input_gain");
        pOutputGain = apvts.getRawParameterValue ("obs_output_gain");
        pMix = apvts.getRawParameterValue ("obs_mix");
        pCharacterGlobal = apvts.getRawParameterValue ("obs_character_global");

        for (int b = 0; b < kNumBands; ++b)
        {
            auto id = [&](const char* suffix) { return juce::String ("obs_b") + juce::String (b + 1) + "_" + suffix; };
            pBandFreq[b]       = apvts.getRawParameterValue (id ("freq"));
            pBandGain[b]       = apvts.getRawParameterValue (id ("gain"));
            pBandQ[b]          = apvts.getRawParameterValue (id ("q"));
            pBandType[b]       = apvts.getRawParameterValue (id ("type"));
            pBandCharacter[b]  = apvts.getRawParameterValue (id ("character"));
            pBandBypass[b]     = apvts.getRawParameterValue (id ("bypass"));
            pBandTideEnable[b] = apvts.getRawParameterValue (id ("tide_enable"));
            pBandTideRate[b]   = apvts.getRawParameterValue (id ("tide_rate"));
            pBandTideDepth[b]  = apvts.getRawParameterValue (id ("tide_depth"));
            pBandTideTarget[b] = apvts.getRawParameterValue (id ("tide_target"));
        }
    }

    //==========================================================================
    // Identity
    //==========================================================================

    juce::String getShaperId() const override { return "Observe"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFFE8A020); }

private:
    static float loadP (std::atomic<float>* p, float fb) noexcept { return p ? p->load() : fb; }

    static float dbToGain (float db) noexcept { return std::pow (10.0f, db / 20.0f); }

    void setFilterType (CytomicSVF& filter, int type)
    {
        // type indices must match the bandTypes StringArray in createParameterLayout:
        //   0=Peak, 1=Notch, 2=Low Shelf, 3=High Shelf,
        //   4=LP6, 5=LP12, 6=LP24, 7=HP6, 8=HP12, 9=HP24,
        //   10=Band Pass, 11=Tilt (handled in processBlock — not routed here)
        switch (type)
        {
            case 0:  filter.setMode (CytomicSVF::Mode::Peak);      break; // Peak
            case 1:  filter.setMode (CytomicSVF::Mode::Notch);     break; // Notch
            case 2:  filter.setMode (CytomicSVF::Mode::LowShelf);  break; // Low Shelf
            case 3:  filter.setMode (CytomicSVF::Mode::HighShelf); break; // High Shelf
            case 4: case 5: case 6: filter.setMode (CytomicSVF::Mode::LowPass);  break; // LP6/12/24
            case 7: case 8: case 9: filter.setMode (CytomicSVF::Mode::HighPass); break; // HP6/12/24
            case 10: filter.setMode (CytomicSVF::Mode::BandPass); break; // Band Pass
            // case 11 (Tilt) is handled directly in processBlock using two shelf filters
            default: filter.setMode (CytomicSVF::Mode::Peak); break;
        }
    }

    static constexpr float kDefaultFreqs[kNumBands] = { 80.0f, 250.0f, 800.0f, 2500.0f, 6000.0f, 16000.0f };

    float sr = 48000.0f;
    std::array<ObserveBand, kNumBands> bands {};

    // Coupling
    float bandEnergyCoupling[kNumBands] {};
    float couplingFreqMod[kNumBands] {};
    float couplingGainMod[kNumBands] {};

    // Parameter pointers
    std::atomic<float>* pInputGain = nullptr;
    std::atomic<float>* pOutputGain = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pCharacterGlobal = nullptr;
    std::atomic<float>* pBandFreq[kNumBands] {};
    std::atomic<float>* pBandGain[kNumBands] {};
    std::atomic<float>* pBandQ[kNumBands] {};
    std::atomic<float>* pBandType[kNumBands] {};
    std::atomic<float>* pBandCharacter[kNumBands] {};
    std::atomic<float>* pBandBypass[kNumBands] {};
    std::atomic<float>* pBandTideEnable[kNumBands] {};
    std::atomic<float>* pBandTideRate[kNumBands] {};
    std::atomic<float>* pBandTideDepth[kNumBands] {};
    std::atomic<float>* pBandTideTarget[kNumBands] {};
};

} // namespace xoceanus
