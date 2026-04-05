// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <cmath>

namespace xoceanus
{

//==============================================================================
// ModMatrix<N> — Fleet-wide shared modulation matrix (D002 compliance).
//
// Provides N source→destination routing slots, each with a bipolar amount.
// Designed for integration into any XOceanus engine adapter without allocation
// on the audio thread.
//
// Standard mod sources (index values, stable and serialised in presets):
//   0 = Off
//   1 = LFO1
//   2 = LFO2
//   3 = Envelope (filter/amp)
//   4 = Velocity (0..1)
//   5 = Key Track  (bipolar, centred on C4 = MIDI 60; ±1 = ±60 semitones)
//   6 = Mod Wheel  (CC1, 0..1)
//   7 = Aftertouch (channel pressure, 0..1)
//
// Destinations are engine-specific and passed in as a StringArray at parameter
// creation time. Index 0 is always "Off" (no-op).
//
// Usage (in engine addParametersImpl / attachParameters / renderBlock):
//   // 1. Declare (as a member):
//   ModMatrix<4> modMatrix;
//
//   // 2. Register parameters:
//   modMatrix.addParameters(params, "snap_", "Snap", engineSpecificDests);
//
//   // 3. Attach:
//   modMatrix.attachParameters(apvts, "snap_");
//
//   // 4. Apply (in renderBlock, after computing source values):
//   ModMatrix<4>::Sources src;
//   src.lfo1 = lfo1Value;
//   src.lfo2 = lfo2Value;
//   src.env  = envLevel;
//   src.velocity = voiceVelocity;
//   src.keyTrack = (midiNote - 60.0f) / 60.0f;
//   src.modWheel = modWheelValue;
//   src.aftertouch = atPressure;
//
//   float cutoffOffset = 0.0f, pitchOffset = 0.0f, ampOffset = 0.0f;
//   modMatrix.apply(src, {&cutoffOffset, &pitchOffset, &ampOffset});
//   // Then: effectiveCutoff += cutoffOffset * 10000.0f; etc.
//
// All methods are RT-safe (no allocation, no blocking).
//==============================================================================

template <int N>
struct ModMatrix
{
    static_assert(N >= 1 && N <= 16, "ModMatrix: slot count must be 1–16");

    //--------------------------------------------------------------------------
    // Source values snapshot — filled once per block by the engine.
    //--------------------------------------------------------------------------
    struct Sources
    {
        float lfo1      = 0.0f; // bipolar [-1, +1]
        float lfo2      = 0.0f; // bipolar [-1, +1]
        float env       = 0.0f; // unipolar [0, 1] — use filter env or amp env
        float velocity  = 0.0f; // unipolar [0, 1]
        float keyTrack  = 0.0f; // bipolar [-1, +1], (note-60)/60
        float modWheel  = 0.0f; // unipolar [0, 1]
        float aftertouch= 0.0f; // unipolar [0, 1]
    };

    //--------------------------------------------------------------------------
    // Parameter registration — call from addParametersImpl / createParameterLayout
    //
    //   prefix       — engine parameter prefix, e.g. "snap_"
    //   displayName  — human name prefix, e.g. "Snap"
    //   destinations — engine-specific destination StringArray (index 0 = Off)
    //--------------------------------------------------------------------------
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                              const juce::String& prefix,
                              const juce::String& displayName,
                              const juce::StringArray& destinations)
    {
        static const juce::StringArray kSources {"Off", "LFO1", "LFO2", "Envelope",
                                                  "Velocity", "Key Track", "Mod Wheel", "Aftertouch"};

        for (int i = 0; i < N; ++i)
        {
            const juce::String idx (i + 1);
            const juce::String slotLabel = displayName + " Mod " + idx;

            params.push_back(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID {prefix + "modSlot" + idx + "Src", 1},
                slotLabel + " Src",
                kSources, 0));

            params.push_back(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID {prefix + "modSlot" + idx + "Dst", 1},
                slotLabel + " Dst",
                destinations, 0));

            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID {prefix + "modSlot" + idx + "Amt", 1},
                slotLabel + " Amt",
                juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f),
                0.0f));
        }
    }

    //--------------------------------------------------------------------------
    // Parameter attachment — call from attachParameters
    //--------------------------------------------------------------------------
    void attachParameters(juce::AudioProcessorValueTreeState& apvts,
                          const juce::String& prefix)
    {
        for (int i = 0; i < N; ++i)
        {
            const juce::String idx (i + 1);
            pSrc[i] = apvts.getRawParameterValue (prefix + "modSlot" + idx + "Src");
            pDst[i] = apvts.getRawParameterValue (prefix + "modSlot" + idx + "Dst");
            pAmt[i] = apvts.getRawParameterValue (prefix + "modSlot" + idx + "Amt");
        }
    }

    //--------------------------------------------------------------------------
    // apply — accumulate mod matrix contributions into destination offsets.
    //
    //   sources — pre-computed source values for this block
    //   outputs — array of pointers to float, indexed by destination index.
    //             outputs[0] is ignored (Off = no-op). Must have numDests entries.
    //   numDests — total number of destinations (including Off at index 0)
    //
    // Caller is responsible for interpreting the accumulated offsets
    // (e.g. multiplying by a sensible range before adding to a base value).
    //--------------------------------------------------------------------------
    void apply(const Sources& sources,
               float* const* outputs,
               int numDests) const noexcept
    {
        for (int i = 0; i < N; ++i)
        {
            if (pSrc[i] == nullptr || pDst[i] == nullptr || pAmt[i] == nullptr)
                continue;

            const int src = static_cast<int> (pSrc[i]->load());
            const int dst = static_cast<int> (pDst[i]->load());
            const float amt = pAmt[i]->load();

            if (src == 0 || dst == 0 || dst >= numDests || std::fabs(amt) < 1e-5f)
                continue;

            float srcVal = 0.0f;
            switch (src)
            {
                case 1: srcVal = sources.lfo1;       break;
                case 2: srcVal = sources.lfo2;       break;
                case 3: srcVal = sources.env;        break;
                case 4: srcVal = sources.velocity;   break;
                case 5: srcVal = sources.keyTrack;   break;
                case 6: srcVal = sources.modWheel;   break;
                case 7: srcVal = sources.aftertouch; break;
                default: break;
            }

            if (outputs[dst] != nullptr)
                *outputs[dst] += srcVal * amt;
        }
    }

    //--------------------------------------------------------------------------
    // Convenience overload: apply with a fixed-size array of destination floats.
    //
    //   destOffsets — array of numDests floats. Caller zeros and reads these.
    //--------------------------------------------------------------------------
    template <int D>
    void apply(const Sources& sources,
               float (&destOffsets)[D]) const noexcept
    {
        float* ptrs[D];
        for (int i = 0; i < D; ++i)
            ptrs[i] = &destOffsets[i];
        apply(sources, ptrs, D);
    }

private:
    std::atomic<float>* pSrc[N] = {};
    std::atomic<float>* pDst[N] = {};
    std::atomic<float>* pAmt[N] = {};
};

} // namespace xoceanus
