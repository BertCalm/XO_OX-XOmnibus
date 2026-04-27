// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <cmath>
#include <functional>
#include <vector>

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

//==============================================================================
// CurveShape — per-route transfer function applied before the amount multiply.
// All curves operate on a bipolar input x ∈ [-1, +1] and preserve sign.
//
//   Linear: y = x                         (default, no curve)
//   Exp:    y = sign(x) * x²              (concave-up, slow start → fast end)
//   Log:    y = sign(x) * √|x|            (concave-down, fast start → slow end)
//   S:      y = sign(x) * smoothstep(|x|) (S-curve, slow at both ends)
//
// APVTS parameter name pattern: {prefix}modSlot{N}Curve (AudioParameterChoice,
// 4 entries matching this enum order).
enum class CurveShape : int { Linear = 0, Exp, Log, S, Count };

inline const juce::StringArray& curveShapeChoices()
{
    static const juce::StringArray kCurves {"Linear", "Exp", "Log", "S"};
    return kCurves;
}

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

            // D9 E3: per-route curve shape (Linear/Exp/Log/S)
            params.push_back(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID {prefix + "modSlot" + idx + "Curve", 1},
                slotLabel + " Curve",
                curveShapeChoices(),
                static_cast<int>(CurveShape::Linear)));

            // D9 E3: per-route quantize-to-scale toggle
            params.push_back(std::make_unique<juce::AudioParameterBool>(
                juce::ParameterID {prefix + "modSlot" + idx + "Quant", 1},
                slotLabel + " Quantize",
                false));
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
            pSrc  [i] = apvts.getRawParameterValue (prefix + "modSlot" + idx + "Src");
            pDst  [i] = apvts.getRawParameterValue (prefix + "modSlot" + idx + "Dst");
            pAmt  [i] = apvts.getRawParameterValue (prefix + "modSlot" + idx + "Amt");
            pCurve[i] = apvts.getRawParameterValue (prefix + "modSlot" + idx + "Curve");
            pQuant[i] = apvts.getRawParameterValue (prefix + "modSlot" + idx + "Quant");
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

            // D9 E3: apply per-route curve shape BEFORE amount multiply
            const auto curve = (pCurve[i] != nullptr)
                                    ? static_cast<CurveShape>(static_cast<int>(pCurve[i]->load()))
                                    : CurveShape::Linear;
            srcVal = applyCurve(srcVal, curve);

            // D9 E3: apply quantize-to-scale if toggled + quantizer_ is wired
            const bool quantOn = (pQuant[i] != nullptr) && (pQuant[i]->load() >= 0.5f);
            if (quantOn && quantizer_)
                srcVal = quantizer_(srcVal);

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

    //--------------------------------------------------------------------------
    // D9 E3: applyCurve — bipolar transfer function, sign-preserving.
    //
    //   x is bipolar [-1, +1].  All curves preserve sign so that a negative
    //   modulation amount can sweep downward (see CLAUDE.md "Bipolar modulation").
    //
    //   CurveShape::Exp  source=0.5  → output = sign(0.5) * 0.5² = 0.25
    //                                  i.e. route(Exp, amt=0.5, src=0.5) → 0.125
    //   CurveShape::Log  source=0.5  → output = sign(0.5) * √0.5   ≈ 0.707
    //   CurveShape::S    source=0.5  → output = sign(0.5) * smoothstep(0.5) = 0.5
    //--------------------------------------------------------------------------
    static float applyCurve(float x, CurveShape c) noexcept
    {
        const float sign = (x < 0.0f) ? -1.0f : 1.0f;
        const float absX = std::abs(x);
        switch (c)
        {
            case CurveShape::Linear:
                return x;
            case CurveShape::Exp:
                return sign * absX * absX;                                         // x²
            case CurveShape::Log:
                return sign * std::sqrt(absX);                                     // √|x|
            case CurveShape::S:
                return sign * (3.0f * absX * absX - 2.0f * absX * absX * absX);   // smoothstep
            default:
                return x;
        }
    }

    //--------------------------------------------------------------------------
    // D9 E3: setScaleQuantizer — inject a scale-quantization function.
    //
    // The quantizer maps a normalised pitch offset (0..1 ≈ 0..127 MIDI notes)
    // to the nearest scale degree.  It does NOT need to know about global key/
    // scale: the processor wires that in via a lambda.  Provide a no-op (or
    // leave unset) to disable quantization across all routes.
    //
    //   void MyProcessor::prepareToPlay(...) {
    //       auto& scale = globalScaleModule.getCurrentScale();
    //       modMatrix.setScaleQuantizer([&scale](float x) {
    //           return scale.quantise(x);
    //       });
    //   }
    //
    // Called on the message thread before audio is running — not RT-safe
    // itself, but the stored function is only read on the audio thread after
    // assignment is complete.
    //--------------------------------------------------------------------------
    void setScaleQuantizer(std::function<float(float)> q)
    {
        quantizer_ = std::move(q);
    }

    //--------------------------------------------------------------------------
    // D9 F4: getRoutesTargeting — return info for all active routes whose
    // destination index matches destIdx.  Used by knob paint code to collect
    // the per-route amount values for the badge ring overlay.
    //
    //   destIdx — the destination index as read from pDst (integer choice index).
    //   Returns a vector of {amount, curveShape} pairs for each active route.
    //
    // O(N) over slots — N ≤ 16 so this is negligible.  Call from paint()
    // (message thread) only — does NOT lock.
    //--------------------------------------------------------------------------
    struct RouteInfo { float amount; CurveShape curve; };

    std::vector<RouteInfo> getRoutesTargeting(int destIdx) const
    {
        std::vector<RouteInfo> result;
        if (destIdx <= 0)
            return result;
        result.reserve(N);

        for (int i = 0; i < N; ++i)
        {
            if (pSrc[i] == nullptr || pDst[i] == nullptr || pAmt[i] == nullptr)
                continue;

            const int src = static_cast<int>(pSrc[i]->load());
            const int dst = static_cast<int>(pDst[i]->load());
            const float amt = pAmt[i]->load();

            if (src == 0 || dst != destIdx || std::fabs(amt) < 1e-5f)
                continue;

            const auto curve = (pCurve[i] != nullptr)
                                    ? static_cast<CurveShape>(static_cast<int>(pCurve[i]->load()))
                                    : CurveShape::Linear;
            result.push_back({amt, curve});
        }
        return result;
    }

private:
    std::atomic<float>* pSrc  [N] = {};
    std::atomic<float>* pDst  [N] = {};
    std::atomic<float>* pAmt  [N] = {};
    std::atomic<float>* pCurve[N] = {};
    std::atomic<float>* pQuant[N] = {};

    // Scale quantizer function — injected by the processor, null by default.
    // Applied after curve transform, before amount multiply, only when the
    // per-route Quantize toggle is on.
    std::function<float(float)> quantizer_;
};

} // namespace xoceanus
