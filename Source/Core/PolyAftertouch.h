#pragma once
#include <array>
#include <cmath>
#include <algorithm>
#include "../DSP/FastMath.h"

namespace xoceanus {

//==============================================================================
// PolyAftertouch — CS-80-inspired per-voice aftertouch modulation.
//
// The Yamaha CS-80 (1977) was legendary for its per-note aftertouch:
// each key independently controlled its own voice's filter and amplitude
// based on finger pressure. This gave performers unprecedented expression.
//
// This module applies per-voice pressure modulation to any set of
// target parameters, with smoothing and configurable response curves.
//
// Targets:
//   - Filter cutoff (per voice)
//   - Amplitude / level (per voice)
//   - Vibrato depth (per voice)
//   - Waveshape / morph position (per voice)
//   - Pitch bend (per voice micro-tuning)
//
// Features:
//   - Independent smoothing per voice (no zipper noise)
//   - Configurable response curve (linear, exponential, S-curve)
//   - Sensitivity scaling per target
//   - Mono aftertouch fallback (channel pressure → all voices)
//
// Integration: PlaySurface provides per-pad pressure via MPE or
//              polyphonic aftertouch MIDI messages.
//
// Inspired by: Yamaha CS-80 (1977), Expressive E Osmose, ROLI Seaboard
//==============================================================================
class PolyAftertouch
{
public:
    static constexpr int kMaxVoices = 16;

    enum class ResponseCurve
    {
        Linear = 0,       // 1:1 mapping
        Exponential,      // Gentle start, aggressive end
        SCurve,           // Gentle at extremes, steep in middle
        Logarithmic,      // Aggressive start, gentle end
        NumCurves
    };

    enum class Target
    {
        None = 0,
        FilterCutoff,     // Pressure → filter cutoff offset
        Amplitude,        // Pressure → level scaling
        VibratoDepth,     // Pressure → vibrato intensity
        MorphPosition,    // Pressure → wavetable/morph sweep
        PitchBend,        // Pressure → micro pitch offset
        NumTargets
    };

    struct VoiceModulation
    {
        float filterOffset   = 0.0f;  // Hz offset for filter cutoff
        float amplitudeScale = 1.0f;  // Multiplier for voice amplitude
        float vibratoDepth   = 0.0f;  // Vibrato depth (0-1)
        float morphOffset    = 0.0f;  // Morph position offset (0-1)
        float pitchBend      = 0.0f;  // Semitones offset
    };

    struct TargetConfig
    {
        Target target       = Target::None;
        float sensitivity   = 0.5f;    // 0-1 scaling
        float minValue      = 0.0f;    // Minimum output
        float maxValue      = 1.0f;    // Maximum output
        ResponseCurve curve = ResponseCurve::Linear;
    };

    PolyAftertouch() = default;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        // Smoothing coefficient: ~5ms attack, ~20ms release
        // SRO: fastExp replaces std::exp (prepare-time coefficient calc)
        smoothAttack  = 1.0f - fastExp (-1.0f / (0.005f * static_cast<float> (sr)));
        smoothRelease = 1.0f - fastExp (-1.0f / (0.020f * static_cast<float> (sr)));
        reset();
    }

    //--------------------------------------------------------------------------
    // Configuration

    void setTarget1 (TargetConfig config) { targets[0] = config; }
    void setTarget2 (TargetConfig config) { targets[1] = config; }
    void setTarget3 (TargetConfig config) { targets[2] = config; }

    void setGlobalSensitivity (float s) { globalSensitivity = std::clamp (s, 0.0f, 2.0f); }

    //--------------------------------------------------------------------------
    // Input: receive pressure values from PlaySurface or MIDI

    /// Set pressure for a specific voice (polyphonic aftertouch / MPE)
    void setVoicePressure (int voiceIndex, float pressure)
    {
        if (voiceIndex >= 0 && voiceIndex < kMaxVoices)
            rawPressure[static_cast<size_t> (voiceIndex)] = std::clamp (pressure, 0.0f, 1.0f);
    }

    /// Set channel pressure (mono aftertouch → applied to all voices)
    void setChannelPressure (float pressure)
    {
        float p = std::clamp (pressure, 0.0f, 1.0f);
        for (auto& rp : rawPressure) rp = p;
    }

    /// Clear pressure for a voice (note off)
    void releaseVoice (int voiceIndex)
    {
        if (voiceIndex >= 0 && voiceIndex < kMaxVoices)
            rawPressure[static_cast<size_t> (voiceIndex)] = 0.0f;
    }

    //--------------------------------------------------------------------------
    // Processing: call once per block to update smoothed modulation values

    void updateBlock (int numSamples)
    {
        for (int v = 0; v < kMaxVoices; ++v)
        {
            float target = rawPressure[static_cast<size_t> (v)] * globalSensitivity;
            target = std::clamp (target, 0.0f, 1.0f);

            // Smooth pressure
            float coeff = (target > smoothedPressure[static_cast<size_t> (v)])
                        ? smoothAttack : smoothRelease;

            // Block-rate approximation
            for (int i = 0; i < numSamples; ++i)
                smoothedPressure[static_cast<size_t> (v)] +=
                    coeff * (target - smoothedPressure[static_cast<size_t> (v)]);

            // Flush denormals
            if (std::abs (smoothedPressure[static_cast<size_t> (v)]) < 1e-10f)
                smoothedPressure[static_cast<size_t> (v)] = 0.0f;

            // Compute modulation outputs for this voice
            computeVoiceMod (v);
        }
    }

    //--------------------------------------------------------------------------
    // Output: get modulation values per voice

    const VoiceModulation& getVoiceMod (int voiceIndex) const
    {
        return voiceMods[static_cast<size_t> (
            std::clamp (voiceIndex, 0, kMaxVoices - 1))];
    }

    float getSmoothedPressure (int voiceIndex) const
    {
        return smoothedPressure[static_cast<size_t> (
            std::clamp (voiceIndex, 0, kMaxVoices - 1))];
    }

    void reset()
    {
        rawPressure.fill (0.0f);
        smoothedPressure.fill (0.0f);
        for (auto& vm : voiceMods) vm = {};
    }

private:
    double sr = 44100.0;
    float globalSensitivity = 1.0f;
    float smoothAttack  = 0.1f;
    float smoothRelease = 0.02f;

    std::array<float, kMaxVoices> rawPressure {};
    std::array<float, kMaxVoices> smoothedPressure {};
    std::array<VoiceModulation, kMaxVoices> voiceMods {};
    std::array<TargetConfig, 3> targets {};

    void computeVoiceMod (int v)
    {
        auto& mod = voiceMods[static_cast<size_t> (v)];
        float pressure = smoothedPressure[static_cast<size_t> (v)];

        // Reset to defaults
        mod = {};

        for (const auto& tc : targets)
        {
            if (tc.target == Target::None || tc.sensitivity < 0.001f)
                continue;

            // Apply response curve
            float shaped = applyResponseCurve (pressure, tc.curve);

            // Scale by sensitivity and map to range
            float value = tc.minValue + shaped * tc.sensitivity * (tc.maxValue - tc.minValue);

            switch (tc.target)
            {
                case Target::FilterCutoff:
                    mod.filterOffset = value * 8000.0f;  // Up to 8kHz offset
                    break;
                case Target::Amplitude:
                    mod.amplitudeScale = 1.0f + (value - 0.5f) * 0.5f;  // 0.75x to 1.25x
                    break;
                case Target::VibratoDepth:
                    mod.vibratoDepth = value;
                    break;
                case Target::MorphPosition:
                    mod.morphOffset = value;
                    break;
                case Target::PitchBend:
                    mod.pitchBend = value * 2.0f;  // Up to ±2 semitones
                    break;
                default:
                    break;
            }
        }
    }

    static float applyResponseCurve (float x, ResponseCurve curve)
    {
        switch (curve)
        {
            case ResponseCurve::Linear:
                return x;
            case ResponseCurve::Exponential:
                return x * x;
            case ResponseCurve::SCurve:
                return x * x * (3.0f - 2.0f * x);  // Hermite S-curve
            case ResponseCurve::Logarithmic:
                // SRO: fast sqrt via fastPow2/fastLog2 (per-voice response curve)
                return (x > 0.0f) ? fastPow2 (0.5f * fastLog2 (x)) : 0.0f;
            default:
                return x;
        }
    }
};

} // namespace xoceanus
