// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/ModMatrix.h"
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <vector>

namespace xoceanus
{

//==============================================================================
//
//  O C T A N T   E N G I N E
//  Tensor Spectral Synthesis
//
//  XO_OX Aquatic Identity: Navigate a 3D ocean volume — depth × latitude ×
//  longitude. Each coordinate has a distinct acoustic character encoded in a
//  spectral tensor. Three rotation angles steer the projection through 256
//  pre-computed partial amplitudes.
//
//  Gallery code: Octant | Accent: Deep Ocean Teal #1A6B7C | Prefix: octn_
//
//  Inspired by (D003):
//    Tensor-organized additive synthesis — spectral amplitudes stored in a
//    3D array [harmonics × layers × morphs] with bilinear interpolation.
//    Concept draws on multi-modal data representation (Tucker 1966,
//    De Lathauwer et al. 2000) as an organizing principle for timbral space.
//
//  Tensor Structure:
//    tensor[16][4][4]: 16 harmonics × 4 depth layers × 4 morph states
//    Layer axis  (octn_depth):    bright→dark spectral character
//    Morph axis  (octn_morph):    saw→bell→string→vocal timbral character
//    Rotation    (octn_rotation): cross-fades between depth and morph axes
//
//  Signal Flow:
//    Tensor[16][4][4] → project(depth, morph, rotation) → 16 partial amplitudes
//        |
//    16x sine oscillators (harmonics 1-16 of fundamental)
//        |
//    Sum → CytomicSVF Output Filter → VCA → Output (stereo)
//
//  Coupling:
//    Input:  AudioToFM     -> FM modulates sine partial phases
//            AmpToFilter   -> output filter cutoff modulation
//            EnvToMorph    -> coupling envelope modulates morph position
//            RhythmToBlend -> coupling rhythm modulates rotation
//    Output: ch0=L, ch1=R, ch2=current tensor depth [0..1]
//
//==============================================================================

static constexpr int   kOctantMaxVoices  = 8;
static constexpr int   kOctantNumPartials = 16;
static constexpr float kOctantTwoPi      = 6.28318530717958647692f;

//==============================================================================
// Pre-built spectral tensor data: tensor[16 harmonics][4 layers][4 morphs]
//
// Layer axis (depth/brightness):
//   Layer 0 = bright (saw-like: all harmonics, 1/n rolloff)
//   Layer 1 = hollow (odd harmonics boosted, square-like)
//   Layer 2 = warm   (even harmonics boosted, octave-rich)
//   Layer 3 = dark   (fundamental + sparse upper partials)
//
// Morph axis (timbral character):
//   Morph 0 = Saw    spectrum (1/n rolloff)
//   Morph 1 = Bell   spectrum (enhanced at prime partials: 2,3,5,7,11)
//   Morph 2 = String spectrum (strong fundamental, formant bumps)
//   Morph 3 = Vocal  spectrum (F1/F2/F3 formant peaks encoded as amplitude bumps)
//==============================================================================
static const float kOctantTensor[kOctantNumPartials][4][4] = {
    // Harmonic 1 (fundamental)
    { {1.00f, 0.90f, 0.95f, 0.80f},   // Layer: bright, hollow, warm, dark  Morph 0 (Saw)
      {1.00f, 1.00f, 1.00f, 1.00f},   // Morph 1 (Bell) - strong fundamental
      {1.00f, 0.90f, 1.00f, 0.90f},   // Morph 2 (String)
      {0.90f, 0.85f, 0.90f, 0.80f} }, // Morph 3 (Vocal)
    // Harmonic 2
    { {0.50f, 0.20f, 0.80f, 0.10f},
      {0.80f, 0.20f, 0.70f, 0.10f},
      {0.60f, 0.15f, 0.75f, 0.05f},
      {0.40f, 0.10f, 0.50f, 0.05f} },
    // Harmonic 3
    { {0.33f, 0.80f, 0.15f, 0.08f},
      {0.70f, 0.80f, 0.15f, 0.08f},
      {0.45f, 0.70f, 0.20f, 0.05f},
      {0.60f, 0.75f, 0.30f, 0.10f} },
    // Harmonic 4
    { {0.25f, 0.10f, 0.60f, 0.05f},
      {0.30f, 0.10f, 0.50f, 0.05f},
      {0.35f, 0.08f, 0.55f, 0.03f},
      {0.20f, 0.05f, 0.35f, 0.03f} },
    // Harmonic 5
    { {0.20f, 0.60f, 0.10f, 0.04f},
      {0.65f, 0.60f, 0.12f, 0.04f},
      {0.25f, 0.50f, 0.40f, 0.08f},   // String: formant bump around h5
      {0.50f, 0.55f, 0.20f, 0.06f} }, // Vocal: F2 formant bump
    // Harmonic 6
    { {0.17f, 0.08f, 0.40f, 0.03f},
      {0.15f, 0.08f, 0.35f, 0.03f},
      {0.20f, 0.06f, 0.38f, 0.02f},
      {0.12f, 0.04f, 0.25f, 0.02f} },
    // Harmonic 7
    { {0.14f, 0.50f, 0.08f, 0.03f},
      {0.55f, 0.50f, 0.10f, 0.03f},
      {0.18f, 0.40f, 0.12f, 0.02f},
      {0.35f, 0.45f, 0.15f, 0.04f} },
    // Harmonic 8
    { {0.13f, 0.06f, 0.30f, 0.02f},
      {0.12f, 0.06f, 0.25f, 0.02f},
      {0.15f, 0.05f, 0.28f, 0.01f},
      {0.10f, 0.03f, 0.18f, 0.01f} },
    // Harmonic 9
    { {0.11f, 0.40f, 0.06f, 0.02f},
      {0.35f, 0.40f, 0.08f, 0.02f},
      {0.12f, 0.30f, 0.10f, 0.02f},
      {0.20f, 0.35f, 0.12f, 0.03f} },
    // Harmonic 10
    { {0.10f, 0.05f, 0.20f, 0.02f},
      {0.08f, 0.05f, 0.18f, 0.02f},
      {0.30f, 0.04f, 0.22f, 0.01f},   // String: body resonance bump around h10
      {0.08f, 0.03f, 0.12f, 0.01f} },
    // Harmonic 11
    { {0.09f, 0.30f, 0.05f, 0.01f},
      {0.40f, 0.30f, 0.06f, 0.01f},   // Bell: prime harmonic
      {0.10f, 0.25f, 0.08f, 0.01f},
      {0.15f, 0.28f, 0.10f, 0.02f} },
    // Harmonic 12
    { {0.08f, 0.04f, 0.15f, 0.01f},
      {0.07f, 0.04f, 0.12f, 0.01f},
      {0.09f, 0.03f, 0.14f, 0.01f},
      {0.06f, 0.02f, 0.08f, 0.01f} },
    // Harmonic 13
    { {0.08f, 0.20f, 0.04f, 0.01f},
      {0.18f, 0.20f, 0.05f, 0.01f},
      {0.08f, 0.15f, 0.06f, 0.01f},
      {0.10f, 0.18f, 0.08f, 0.01f} },
    // Harmonic 14
    { {0.07f, 0.03f, 0.10f, 0.01f},
      {0.06f, 0.03f, 0.09f, 0.01f},
      {0.08f, 0.02f, 0.10f, 0.00f},
      {0.05f, 0.02f, 0.06f, 0.00f} },
    // Harmonic 15
    { {0.07f, 0.15f, 0.03f, 0.01f},
      {0.12f, 0.15f, 0.04f, 0.01f},
      {0.07f, 0.10f, 0.05f, 0.00f},
      {0.08f, 0.12f, 0.06f, 0.01f} },
    // Harmonic 16
    { {0.06f, 0.02f, 0.08f, 0.00f},
      {0.05f, 0.02f, 0.07f, 0.00f},
      {0.06f, 0.02f, 0.08f, 0.00f},
      {0.04f, 0.01f, 0.05f, 0.00f} },
};

//==============================================================================
// projectTensor -- compute 16 partial amplitudes via bilinear interpolation
//
// depth [0,1] → layer axis (0..3)
// morph [0,1] → morph axis (0..3)
// rotation [0,360 degrees] → cross-fades the depth and morph axes
//
// After bilinear interpolation on the 4×4 inner grid, the rotation angle
// blends the depth-dominant projection with the morph-dominant projection.
//==============================================================================
inline void projectTensor(const float depth,
                          const float morph,
                          const float rotationDeg,
                          float outAmplitudes[kOctantNumPartials]) noexcept
{
    // Scale depth and morph into layer/morph index space [0..3]
    const float d   = std::clamp(depth, 0.0f, 1.0f) * 3.0f;
    const float m   = std::clamp(morph, 0.0f, 1.0f) * 3.0f;

    const int   d0  = static_cast<int>(d);
    const int   d1  = std::min(d0 + 1, 3);
    const int   m0  = static_cast<int>(m);
    const int   m1  = std::min(m0 + 1, 3);

    const float fd  = d - static_cast<float>(d0);  // fractional layer
    const float fm  = m - static_cast<float>(m0);  // fractional morph

    // Rotation blending: at 0° → pure depth-first, at 90° → equal mix,
    // at 180° → pure morph-first. Use cosine blend for smooth transition.
    const float rotRad = rotationDeg * (kOctantTwoPi / 360.0f);
    // rotBlend: 0=depth-dominant, 1=morph-dominant, cycles with 360 rotation
    const float rotBlend = 0.5f + 0.5f * fastSin(rotRad);

    // depth-dominant interpolation (fd varies, fm interpolates at fixed d)
    const float ddA = 1.0f - fd;
    const float ddB = fd;

    // morph-dominant interpolation (fm varies, fd interpolates at fixed m)
    const float mdA = 1.0f - fm;
    const float mdB = fm;

    for (int h = 0; h < kOctantNumPartials; ++h)
    {
        // Bilinear interpolation across the full [layer][morph] grid
        const float v00 = kOctantTensor[h][d0][m0];
        const float v10 = kOctantTensor[h][d1][m0];
        const float v01 = kOctantTensor[h][d0][m1];
        const float v11 = kOctantTensor[h][d1][m1];

        // Depth-dominant: interpolate along depth axis first
        const float depthInterp = (v00 * ddA + v10 * ddB) * mdA
                                + (v01 * ddA + v11 * ddB) * mdB;

        // Morph-dominant: interpolate along morph axis first
        const float morphInterp = (v00 * mdA + v01 * mdB) * ddA
                                + (v10 * mdA + v11 * mdB) * ddB;

        // Blend between depth-dominant and morph-dominant via rotation
        outAmplitudes[h] = lerp(depthInterp, morphInterp, rotBlend);
    }
}

//==============================================================================
// OctantVoice -- one polyphonic voice
//==============================================================================
struct OctantVoice
{
    bool  active    = false;
    bool  releasing = false;
    int   note      = -1;
    float velocity  = 0.0f;
    float keyTrack  = 0.0f;

    // Glide-smoothed note frequency
    float glideFreq = 440.0f;

    // 16 sine phase accumulators (0..1, wrap at 1.0)
    float partialPhase[kOctantNumPartials] = {};

    // Per-partial amplitude smoothers (one-pole, prevents zipper on tensor change)
    float smoothAmp[kOctantNumPartials] = {};

    // Stereo output filter
    CytomicSVF outputFilterL;
    CytomicSVF outputFilterR;

    // Envelopes
    StandardADSR ampEnv;
    StandardADSR filterEnv;

    // Per-voice LFOs
    StandardLFO lfo1;
    StandardLFO lfo2;

    // Last LFO output values (for mod matrix source population)
    float lastLfo1Val = 0.0f;
    float lastLfo2Val = 0.0f;

    // Coupling AudioToFM accumulator (per-sample tight coupling)
    float couplingFmAccum = 0.0f;

    // Filter coefficient update counter (updates every 16 samples)
    int filterUpdateCounter = 0;

    void reset() noexcept
    {
        active    = false;
        releasing = false;
        note      = -1;
        velocity  = 0.0f;
        keyTrack  = 0.0f;
        glideFreq = 440.0f;

        for (int i = 0; i < kOctantNumPartials; ++i)
        {
            partialPhase[i] = 0.0f;
            smoothAmp[i]    = 0.0f;
        }

        outputFilterL.reset();
        outputFilterR.reset();
        ampEnv.reset();
        filterEnv.reset();
        lfo1.reset();
        lfo2.reset();
        lastLfo1Val         = 0.0f;
        lastLfo2Val         = 0.0f;
        couplingFmAccum     = 0.0f;
        filterUpdateCounter = 0;
    }
};

//==============================================================================
//
//  OctantEngine -- Tensor Spectral Synthesis
//
//  Implements SynthEngine for XOceanus integration.
//  All DSP is inline per project convention -- .cpp is a one-line stub.
//
//  Parameter prefix: octn_
//  Gallery accent:   Deep Ocean Teal #1A6B7C
//
//==============================================================================
class OctantEngine : public SynthEngine
{
public:

    //==========================================================================
    //  P A R A M E T E R   R E G I S T R A T I O N
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using AP  = juce::AudioParameterFloat;
        using APC = juce::AudioParameterChoice;
        using PID = juce::ParameterID;
        using NR  = juce::NormalisableRange<float>;

        // ---- A: Tensor Navigation (6 params) ----
        params.push_back(std::make_unique<AP>(PID{"octn_depth",1}, "Octant Depth",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"octn_morph",1}, "Octant Morph",
            NR{0.0f, 1.0f, 0.001f}, 0.25f));

        params.push_back(std::make_unique<AP>(PID{"octn_rotation",1}, "Octant Rotation",
            NR{0.0f, 360.0f, 0.1f}, 0.0f));

        params.push_back(std::make_unique<AP>(PID{"octn_spread",1}, "Octant Spread",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));

        params.push_back(std::make_unique<AP>(PID{"octn_harmonicShift",1}, "Octant Pitch Offset",
            NR{-12.0f, 12.0f, 0.01f}, 0.0f));

        params.push_back(std::make_unique<APC>(PID{"octn_partialCount",1}, "Octant Partial Count",
            juce::StringArray{"4","8","12","16"}, 3));

        // ---- B: Tensor Character (4 params) ----
        params.push_back(std::make_unique<AP>(PID{"octn_tensorWarp",1}, "Octant Spectrum Contrast",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));

        params.push_back(std::make_unique<AP>(PID{"octn_spectralTilt",1}, "Octant Spectral Tilt",
            NR{-1.0f, 1.0f, 0.001f}, 0.0f));

        params.push_back(std::make_unique<AP>(PID{"octn_oddEvenBalance",1}, "Octant Harmonic Balance",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"octn_inharmonicity",1}, "Octant Inharmonicity",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));

        // ---- C: Filter + Filter Envelope (9 params) ----
        {
            NR r{20.0f, 20000.0f, 0.1f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"octn_fltCutoff",1}, "Octant Flt Cutoff", r, 10000.0f));
        }
        params.push_back(std::make_unique<AP>(PID{"octn_fltReso",1}, "Octant Flt Reso",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<APC>(PID{"octn_fltType",1}, "Octant Flt Type",
            juce::StringArray{"LP","HP","BP","Notch"}, 0));
        params.push_back(std::make_unique<AP>(PID{"octn_fltEnvAmt",1}, "Octant Flt Env Amt",
            NR{-1.0f, 1.0f, 0.001f}, 0.2f));
        params.push_back(std::make_unique<AP>(PID{"octn_fltKeyTrack",1}, "Octant Flt Key Track",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"octn_fltAtk",1}, "Octant Flt Atk", r, 0.01f));
        }
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"octn_fltDec",1}, "Octant Flt Dec", r, 0.3f));
        }
        params.push_back(std::make_unique<AP>(PID{"octn_fltSus",1}, "Octant Flt Sus",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"octn_fltRel",1}, "Octant Flt Rel", r, 0.4f));
        }

        // ---- D: Amp Envelope (5 params) ----
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"octn_ampAtk",1}, "Octant Amp Atk", r, 0.01f));
        }
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"octn_ampDec",1}, "Octant Amp Dec", r, 0.4f));
        }
        params.push_back(std::make_unique<AP>(PID{"octn_ampSus",1}, "Octant Amp Sus",
            NR{0.0f, 1.0f, 0.001f}, 0.7f));
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"octn_ampRel",1}, "Octant Amp Rel", r, 0.5f));
        }
        // D001: velocity shifts depth toward bright layer
        params.push_back(std::make_unique<AP>(PID{"octn_velTimbre",1}, "Octant Vel Timbre",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        // ---- E: LFOs (8 params) ----
        static const juce::StringArray kLFOShapes   {"Sine","Tri","Saw","Square","S&H"};
        static const juce::StringArray kLFO1Targets {"Depth","Morph","Rotation","Filter Cutoff"};
        static const juce::StringArray kLFO2Targets {"Depth","Morph","Rotation","Filter Cutoff"};

        {
            NR r{0.01f, 20.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"octn_lfo1Rate",1}, "Octant LFO1 Rate", r, 0.4f));
        }
        params.push_back(std::make_unique<AP>(PID{"octn_lfo1Depth",1}, "Octant LFO1 Depth",
            NR{0.0f, 1.0f, 0.001f}, 0.2f));
        params.push_back(std::make_unique<APC>(PID{"octn_lfo1Shape",1}, "Octant LFO1 Shape",
            kLFOShapes, 0));
        params.push_back(std::make_unique<APC>(PID{"octn_lfo1Target",1}, "Octant LFO1 Target",
            kLFO1Targets, 0));  // Depth

        {
            NR r{0.01f, 20.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"octn_lfo2Rate",1}, "Octant LFO2 Rate", r, 0.12f));
        }
        params.push_back(std::make_unique<AP>(PID{"octn_lfo2Depth",1}, "Octant LFO2 Depth",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<APC>(PID{"octn_lfo2Shape",1}, "Octant LFO2 Shape",
            kLFOShapes, 1));  // Triangle
        params.push_back(std::make_unique<APC>(PID{"octn_lfo2Target",1}, "Octant LFO2 Target",
            kLFO2Targets, 1)); // Morph

        // ---- F: Mod Matrix (4 slots × 3 params = 12 params) ----
        static const juce::StringArray kOctantModDests {
            "Off", "Filter Cutoff", "Depth", "Morph", "Rotation", "Spread", "Amp Level"
        };
        ModMatrix<4>::addParameters(params, "octn_", "Octant", kOctantModDests);

        // ---- G: Macros + Voice (7 params) ----
        // M1=DEPTH: tensor depth navigation
        params.push_back(std::make_unique<AP>(PID{"octn_macro1",1}, "Octant Macro1",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));
        // M2=MORPH: tensor morph navigation
        params.push_back(std::make_unique<AP>(PID{"octn_macro2",1}, "Octant Macro2",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        // M3=ROTATION: tensor rotation
        params.push_back(std::make_unique<AP>(PID{"octn_macro3",1}, "Octant Macro3",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        // M4=SPACE: filter cutoff + spread
        params.push_back(std::make_unique<AP>(PID{"octn_macro4",1}, "Octant Macro4",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<APC>(PID{"octn_voiceMode",1}, "Octant Voice Mode",
            juce::StringArray{"Mono","Legato","Poly4","Poly8"}, 2));

        {
            NR r{0.0f, 2.0f, 0.001f};
            r.setSkewForCentre(0.5f);
            params.push_back(std::make_unique<AP>(PID{"octn_glide",1}, "Octant Glide", r, 0.0f));
        }

        params.push_back(std::make_unique<APC>(PID{"octn_glideMode",1}, "Octant Glide Mode",
            juce::StringArray{"Legato","Always"}, 0));
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParameters(params);
        return {params.begin(), params.end()};
    }

    //==========================================================================
    //  L I F E C Y C L E
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sampleRateFloat = (sampleRate > 0.0) ? static_cast<float>(sampleRate) : 44100.0f;
        maxBlock        = maxBlockSize;

        for (int i = 0; i < kOctantMaxVoices; ++i)
            voices[i].reset();

        couplingFmBuf.assign(static_cast<size_t>(std::max(maxBlockSize, 1)), 0.0f);
        couplingAmpFilter = 0.0f;
        couplingEnvMorph  = 0.0f;
        couplingRhythm    = 0.0f;

        modWheelValue   = 0.0f;
        aftertouchValue = 0.0f;

        lastSampleL  = 0.0f;
        lastSampleR  = 0.0f;
        lastDepth    = 0.5f;

        activeVoiceCount_.store(0, std::memory_order_relaxed);

        // Additive tails can linger — use standard 200ms hold
        prepareSilenceGate(sampleRate, maxBlockSize, 200.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();

        std::fill(couplingFmBuf.begin(), couplingFmBuf.end(), 0.0f);
        couplingAmpFilter = 0.0f;
        couplingEnvMorph  = 0.0f;
        couplingRhythm    = 0.0f;
        modWheelValue     = 0.0f;
        aftertouchValue   = 0.0f;
        pitchBendValue    = 0.0f;
        lastSampleL       = 0.0f;
        lastSampleR       = 0.0f;
        lastDepth         = 0.5f;
        activeVoiceCount_.store(0, std::memory_order_relaxed);
    }

    //==========================================================================
    //  C O U P L I N G
    //==========================================================================

    void applyCouplingInput(CouplingType type, float amount,
                            const float* sourceBuffer, int numSamples) override
    {
        if (sourceBuffer == nullptr || numSamples <= 0)
            return;

        switch (type)
        {
        case CouplingType::AudioToFM:
        {
            // Copy into FM buffer -- applied per-sample to partial phases
            const int copyLen = std::min(numSamples, maxBlock);
            for (int i = 0; i < copyLen; ++i)
                couplingFmBuf[static_cast<size_t>(i)] = sourceBuffer[i] * amount;
            // Update per-voice accumulator with the last sample
            for (auto& v : voices)
                if (v.active)
                    v.couplingFmAccum = sourceBuffer[numSamples - 1] * amount;
            break;
        }
        case CouplingType::AmpToFilter:
        {
            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += std::fabs(sourceBuffer[i]);
            rms /= static_cast<float>(numSamples);
            couplingAmpFilter = rms * amount;
            break;
        }
        case CouplingType::EnvToMorph:
        {
            // Last sample of the envelope modulates morph position
            couplingEnvMorph = sourceBuffer[numSamples - 1] * amount;
            break;
        }
        case CouplingType::RhythmToBlend:
        {
            // Rhythm modulates rotation position
            couplingRhythm = sourceBuffer[numSamples - 1] * amount;
            break;
        }
        default:
        {
            // Generic fallback: AmpToFilter
            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += std::fabs(sourceBuffer[i]);
            rms /= static_cast<float>(numSamples);
            couplingAmpFilter += rms * amount;
            break;
        }
        }
    }

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        if (channel == 0) return lastSampleL;
        if (channel == 1) return lastSampleR;
        if (channel == 2) return lastDepth;  // current tensor depth [0..1]
        return 0.0f;
    }

    //==========================================================================
    //  P A R A M E T E R   A T T A C H M E N T
    //==========================================================================

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        // Group A — Tensor Navigation
        pDepth          = apvts.getRawParameterValue("octn_depth");
        pMorph          = apvts.getRawParameterValue("octn_morph");
        pRotation       = apvts.getRawParameterValue("octn_rotation");
        pSpread         = apvts.getRawParameterValue("octn_spread");
        pHarmonicShift  = apvts.getRawParameterValue("octn_harmonicShift");
        pPartialCount   = apvts.getRawParameterValue("octn_partialCount");

        // Group B — Tensor Character
        pTensorWarp      = apvts.getRawParameterValue("octn_tensorWarp");
        pSpectralTilt    = apvts.getRawParameterValue("octn_spectralTilt");
        pOddEvenBalance  = apvts.getRawParameterValue("octn_oddEvenBalance");
        pInharmonicity   = apvts.getRawParameterValue("octn_inharmonicity");

        // Group C — Filter
        pFltCutoff   = apvts.getRawParameterValue("octn_fltCutoff");
        pFltReso     = apvts.getRawParameterValue("octn_fltReso");
        pFltType     = apvts.getRawParameterValue("octn_fltType");
        pFltEnvAmt   = apvts.getRawParameterValue("octn_fltEnvAmt");
        pFltKeyTrack = apvts.getRawParameterValue("octn_fltKeyTrack");
        pFltAtk      = apvts.getRawParameterValue("octn_fltAtk");
        pFltDec      = apvts.getRawParameterValue("octn_fltDec");
        pFltSus      = apvts.getRawParameterValue("octn_fltSus");
        pFltRel      = apvts.getRawParameterValue("octn_fltRel");

        // Group D — Amp Envelope
        pAmpAtk    = apvts.getRawParameterValue("octn_ampAtk");
        pAmpDec    = apvts.getRawParameterValue("octn_ampDec");
        pAmpSus    = apvts.getRawParameterValue("octn_ampSus");
        pAmpRel    = apvts.getRawParameterValue("octn_ampRel");
        pVelTimbre = apvts.getRawParameterValue("octn_velTimbre");

        // Group E — LFOs
        pLfo1Rate   = apvts.getRawParameterValue("octn_lfo1Rate");
        pLfo1Depth  = apvts.getRawParameterValue("octn_lfo1Depth");
        pLfo1Shape  = apvts.getRawParameterValue("octn_lfo1Shape");
        pLfo1Target = apvts.getRawParameterValue("octn_lfo1Target");
        pLfo2Rate   = apvts.getRawParameterValue("octn_lfo2Rate");
        pLfo2Depth  = apvts.getRawParameterValue("octn_lfo2Depth");
        pLfo2Shape  = apvts.getRawParameterValue("octn_lfo2Shape");
        pLfo2Target = apvts.getRawParameterValue("octn_lfo2Target");

        // Group F — Mod Matrix
        modMatrix.attachParameters(apvts, "octn_");

        // Group G — Macros + Voice
        pMacro1    = apvts.getRawParameterValue("octn_macro1");
        pMacro2    = apvts.getRawParameterValue("octn_macro2");
        pMacro3    = apvts.getRawParameterValue("octn_macro3");
        pMacro4    = apvts.getRawParameterValue("octn_macro4");
        pVoiceMode = apvts.getRawParameterValue("octn_voiceMode");
        pGlide     = apvts.getRawParameterValue("octn_glide");
        pGlideMode = apvts.getRawParameterValue("octn_glideMode");
    }

    //==========================================================================
    //  I D E N T I T Y
    //==========================================================================

    juce::String  getEngineId()     const override { return "Octant"; }
    juce::Colour  getAccentColour() const override { return juce::Colour(0xFF1A6B7C); } // Deep Ocean Teal
    int           getMaxVoices()    const override { return kOctantMaxVoices; }

    //==========================================================================
    //  R E N D E R   B L O C K
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (sampleRateFloat <= 0.0f) return;
        if (numSamples <= 0) return;

        // ---- SilenceGate: wake on note-on, bail if truly silent ----
        for (const auto& md : midi)
        {
            if (md.getMessage().isNoteOn())
            {
                wakeSilenceGate();
                break;
            }
        }
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // ---- Snapshot all parameters once per block ----
        const float macro1 = pMacro1 ? pMacro1->load() : 0.5f;
        const float macro2 = pMacro2 ? pMacro2->load() : 0.0f;
        const float macro3 = pMacro3 ? pMacro3->load() : 0.0f;
        const float macro4 = pMacro4 ? pMacro4->load() : 0.5f;

        // Group A — Tensor Navigation
        // M1=DEPTH: macro shifts depth toward bright (layer 0)
        float baseDepth  = pDepth     ? pDepth->load()     : 0.5f;
        float baseMorph  = pMorph     ? pMorph->load()     : 0.0f;
        float baseRotation = pRotation ? pRotation->load() : 0.0f;

        // M1=DEPTH: bipolar — below 0.5 darkens, above 0.5 brightens; at 0.5 no change
        baseDepth    = std::clamp(baseDepth + (macro1 - 0.5f) * 0.8f, 0.0f, 1.0f);
        // M2 adds morph
        baseMorph    = std::clamp(baseMorph  + macro2 * 0.8f, 0.0f, 1.0f);
        // M3 adds rotation
        baseRotation = std::fmod(baseRotation + macro3 * 180.0f, 360.0f);

        // Apply coupling EnvToMorph
        baseMorph = std::clamp(baseMorph + couplingEnvMorph, 0.0f, 1.0f);
        // Apply coupling RhythmToBlend → rotation
        baseRotation = std::fmod(baseRotation + couplingRhythm * 90.0f, 360.0f);

        const float spread         = pSpread        ? pSpread->load()        : 0.0f;
        // M4=SPACE: macro widens spread
        const float effectiveSpread = std::clamp(spread + macro4 * 0.3f, 0.0f, 1.0f);

        const float harmonicShift  = pHarmonicShift ? pHarmonicShift->load() : 0.0f;
        // partialCount choice: 0=4, 1=8, 2=12, 3=16
        const int   partialCountIdx = pPartialCount ? static_cast<int>(pPartialCount->load()) : 3;
        static const int kPartialCounts[4] = {4, 8, 12, 16};
        const int   numPartials = kPartialCounts[std::clamp(partialCountIdx, 0, 3)];

        // Group B — Tensor Character
        const float tensorWarp     = pTensorWarp     ? pTensorWarp->load()     : 0.0f;
        const float spectralTilt   = pSpectralTilt   ? pSpectralTilt->load()   : 0.0f;
        const float oddEvenBalance = pOddEvenBalance ? pOddEvenBalance->load() : 0.5f;
        const float inharmonicity  = pInharmonicity  ? pInharmonicity->load()  : 0.0f;

        // Group C — Filter
        float baseCutoff  = pFltCutoff  ? pFltCutoff->load()  : 10000.0f;
        baseCutoff += couplingAmpFilter * 8000.0f;  // AmpToFilter coupling
        // M4=SPACE: macro opens filter
        baseCutoff = std::clamp(baseCutoff * (0.5f + macro4), 20.0f, 20000.0f);

        const float fltReso     = pFltReso     ? pFltReso->load()     : 0.0f;
        const int   fltType     = pFltType     ? static_cast<int>(pFltType->load())  : 0;
        const float fltEnvAmt   = pFltEnvAmt   ? pFltEnvAmt->load()   : 0.2f;
        const float fltKeyTrack = pFltKeyTrack ? pFltKeyTrack->load() : 0.5f;
        const float fltAtk      = pFltAtk      ? pFltAtk->load()      : 0.01f;
        const float fltDec      = pFltDec      ? pFltDec->load()      : 0.3f;
        const float fltSus      = pFltSus      ? pFltSus->load()      : 0.0f;
        const float fltRel      = pFltRel      ? pFltRel->load()      : 0.4f;

        // Group D — Amp Envelope
        const float ampAtk    = pAmpAtk    ? pAmpAtk->load()    : 0.01f;
        const float ampDec    = pAmpDec    ? pAmpDec->load()    : 0.4f;
        const float ampSus    = pAmpSus    ? pAmpSus->load()    : 0.7f;
        const float ampRel    = pAmpRel    ? pAmpRel->load()    : 0.5f;
        const float velTimbre = pVelTimbre ? pVelTimbre->load() : 0.5f;

        // Group E — LFOs (enforce 0.01 Hz floor — D005)
        const float lfo1Rate  = std::max(0.01f, pLfo1Rate  ? pLfo1Rate->load()  : 0.4f);
        const float lfo1Depth = pLfo1Depth ? pLfo1Depth->load() : 0.0f;
        const int   lfo1Shape = pLfo1Shape ? static_cast<int>(pLfo1Shape->load())  : 0;
        const int   lfo1Tgt   = pLfo1Target? static_cast<int>(pLfo1Target->load()) : 0;

        const float lfo2Rate  = std::max(0.01f, pLfo2Rate  ? pLfo2Rate->load()  : 0.12f);
        const float lfo2Depth = pLfo2Depth ? pLfo2Depth->load() : 0.0f;
        const int   lfo2Shape = pLfo2Shape ? static_cast<int>(pLfo2Shape->load())  : 1;
        const int   lfo2Tgt   = pLfo2Target? static_cast<int>(pLfo2Target->load()) : 1;

        // Group G — Voice
        const int   voiceMode = pVoiceMode ? static_cast<int>(pVoiceMode->load()) : 2;
        const float glideTime = pGlide     ? pGlide->load() : 0.0f;
        const int   glideMode = pGlideMode ? static_cast<int>(pGlideMode->load()) : 0;

        // Glide coefficient
        const bool  glideActive = (glideTime > 0.0001f) && (glideMode == 1 || voiceMode == 1);
        const float glideCoeff  = glideActive
            ? fastExp(-1.0f / (glideTime * sampleRateFloat))
            : 0.0f;

        // ---- Build mod matrix sources from active voices ----
        {
            float lfo1Sum = 0.0f, lfo2Sum = 0.0f, envSum = 0.0f;
            float velSum  = 0.0f, ktSum   = 0.0f;
            int count = 0;
            for (auto& v : voices)
            {
                if (v.active)
                {
                    lfo1Sum += v.lastLfo1Val;
                    lfo2Sum += v.lastLfo2Val;
                    envSum  += v.ampEnv.getLevel();
                    velSum  += v.velocity;
                    ktSum   += v.keyTrack;
                    ++count;
                }
            }
            if (count > 0)
            {
                const float inv = 1.0f / static_cast<float>(count);
                blockModSrc.lfo1     = lfo1Sum * inv;
                blockModSrc.lfo2     = lfo2Sum * inv;
                blockModSrc.env      = envSum  * inv;
                blockModSrc.velocity = velSum  * inv;
                blockModSrc.keyTrack = ktSum   * inv;
            }
            blockModSrc.modWheel   = modWheelValue;
            blockModSrc.aftertouch = aftertouchValue;
        }

        // ---- Apply mod matrix ----
        // Destinations: 0=Off, 1=Filter Cutoff, 2=Depth, 3=Morph,
        //               4=Rotation, 5=Spread, 6=Amp Level
        float modDestOffsets[7] = {};
        modMatrix.apply(blockModSrc, modDestOffsets);

        baseCutoff  = std::clamp(baseCutoff + modDestOffsets[1] * 8000.0f, 20.0f, 20000.0f);
        baseDepth   = std::clamp(baseDepth  + modDestOffsets[2] * 0.5f,    0.0f,  1.0f);
        baseMorph   = std::clamp(baseMorph  + modDestOffsets[3] * 0.5f,    0.0f,  1.0f);
        baseRotation = std::fmod(baseRotation + modDestOffsets[4] * 90.0f + 720.0f, 360.0f);
        const float modSpreadAdd = modDestOffsets[5] * 0.3f;
        const float modAmpLevel  = modDestOffsets[6];

        const float finalSpread = std::clamp(effectiveSpread + modSpreadAdd, 0.0f, 1.0f);

        // ---- D006: CC1 modulates rotation (timbral cross-axis change) ----
        baseRotation = std::fmod(baseRotation + modWheelValue * 90.0f, 360.0f);

        // ---- D006: Aftertouch modulates morph position ----
        baseMorph = std::clamp(baseMorph + aftertouchValue * 0.5f, 0.0f, 1.0f);

        // ---- Apply nonlinear warp to depth and morph ----
        // tensorWarp bends the interpolation response nonlinearly
        float warpedDepth = baseDepth;
        float warpedMorph = baseMorph;
        if (tensorWarp > 0.0001f)
        {
            // Warp: push values toward extremes at high warp
            warpedDepth = baseDepth < 0.5f
                ? baseDepth * (1.0f - tensorWarp * 0.5f * baseDepth)
                : 1.0f - (1.0f - baseDepth) * (1.0f - tensorWarp * 0.5f * (1.0f - baseDepth));
            warpedMorph = baseMorph < 0.5f
                ? baseMorph * (1.0f - tensorWarp * 0.5f * baseMorph)
                : 1.0f - (1.0f - baseMorph) * (1.0f - tensorWarp * 0.5f * (1.0f - baseMorph));
        }

        // ---- Project tensor to get 16 partial target amplitudes ----
        float targetAmps[kOctantNumPartials] = {};
        projectTensor(warpedDepth, warpedMorph, baseRotation, targetAmps);

        // Apply spectral tilt: amplify bright partials (positive) or darken (negative)
        for (int h = 0; h < numPartials; ++h)
        {
            // tilt > 0 = brighter (high partials boosted), tilt < 0 = darker
            const float tiltFactor = 1.0f + spectralTilt * (static_cast<float>(h) / 15.0f - 0.5f) * 2.0f;
            targetAmps[h] *= std::max(0.0f, tiltFactor);
        }

        // Apply odd/even balance: 0=odd only, 0.5=balanced, 1=even only
        // Linear crossfade across full range — normalization below handles level
        for (int h = 0; h < numPartials; ++h)
        {
            const bool isOdd = ((h + 1) % 2 == 1);  // h+1 is harmonic number
            const float balance = isOdd ? (1.0f - oddEvenBalance) : oddEvenBalance;
            targetAmps[h] *= balance;
        }

        // Normalize so loudest partial is 1.0 (prevents clipping)
        {
            float maxAmp = 0.0f;
            for (int h = 0; h < numPartials; ++h)
                maxAmp = std::max(maxAmp, targetAmps[h]);
            if (maxAmp > 1e-6f)
            {
                const float invMax = 1.0f / maxAmp;
                for (int h = 0; h < numPartials; ++h)
                    targetAmps[h] *= invMax;
            }
        }

        // ---- Output buffers ----
        auto* writeL = buffer.getWritePointer(0);
        auto* writeR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : writeL;
        buffer.clear();

        // ---- MIDI + render interleaved ----
        int midiSamplePos = 0;

        for (const auto& midiEvent : midi)
        {
            const auto& msg    = midiEvent.getMessage();
            const int   msgPos = std::min(midiEvent.samplePosition, numSamples - 1);

            renderVoicesRange(writeL, writeR, midiSamplePos, msgPos,
                              targetAmps, numPartials,
                              ampAtk, ampDec, ampSus, ampRel,
                              baseCutoff, fltReso, fltType, fltEnvAmt, fltKeyTrack,
                              fltAtk, fltDec, fltSus, fltRel,
                              lfo1Rate, lfo1Depth, lfo1Shape, lfo1Tgt,
                              lfo2Rate, lfo2Depth, lfo2Shape, lfo2Tgt,
                              velTimbre, glideCoeff, modAmpLevel,
                              finalSpread, harmonicShift, inharmonicity);
            midiSamplePos = msgPos;

            if (msg.isNoteOn())
            {
                handleNoteOn(msg.getNoteNumber(), msg.getVelocity(),
                             ampAtk, ampDec, ampSus, ampRel,
                             fltAtk, fltDec, fltSus, fltRel,
                             voiceMode, glideTime);
            }
            else if (msg.isNoteOff())
            {
                handleNoteOff(msg.getNoteNumber());
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                modWheelValue = msg.getControllerValue() / 127.0f;
            }
            else if (msg.isChannelPressure())
            {
                aftertouchValue = msg.getChannelPressureValue() / 127.0f;
            }
            else if (msg.isAftertouch())
            {
                aftertouchValue = msg.getAfterTouchValue() / 127.0f;
            }
            else if (msg.isPitchWheel())
            {
                pitchBendValue = (msg.getPitchWheelValue() - 8192) / 8192.0f;
            }
        }

        // Render remaining samples after last MIDI event
        renderVoicesRange(writeL, writeR, midiSamplePos, numSamples,
                          targetAmps, numPartials,
                          ampAtk, ampDec, ampSus, ampRel,
                          baseCutoff, fltReso, fltType, fltEnvAmt, fltKeyTrack,
                          fltAtk, fltDec, fltSus, fltRel,
                          lfo1Rate, lfo1Depth, lfo1Shape, lfo1Tgt,
                          lfo2Rate, lfo2Depth, lfo2Shape, lfo2Tgt,
                          velTimbre, glideCoeff, modAmpLevel,
                          finalSpread, harmonicShift, inharmonicity);

        // ---- Cache last output samples for coupling ----
        if (numSamples > 0)
        {
            lastSampleL = writeL[numSamples - 1];
            lastSampleR = writeR[numSamples - 1];
            lastDepth   = baseDepth;
        }

        // Coupling accumulator decay — sample-rate and block-size independent (~1s time constant)
        const float couplingDecay = std::exp(-static_cast<float>(numSamples) / (sampleRateFloat * 1.0f));
        couplingAmpFilter *= couplingDecay;
        couplingEnvMorph  *= couplingDecay;
        couplingRhythm    *= couplingDecay;

        // ---- Count active voices ----
        {
            int count = 0;
            for (auto& v : voices)
                if (v.active) ++count;
            activeVoiceCount_.store(count, std::memory_order_relaxed);
        }

        // ---- SilenceGate: analyze output ----
        analyzeForSilenceGate(buffer, numSamples);
    }

private:

    //==========================================================================
    //  V O I C E   R E N D E R
    //==========================================================================

    void renderVoicesRange(float* writeL, float* writeR,
                           int startSample, int endSample,
                           const float targetAmps[kOctantNumPartials],
                           int numPartials,
                           float ampAtk, float ampDec, float ampSus, float ampRel,
                           float baseCutoff, float fltReso, int fltType,
                           float fltEnvAmt, float fltKeyTrack,
                           float fltAtk, float fltDec, float fltSus, float fltRel,
                           float lfo1Rate, float lfo1Depth, int lfo1Shape, int lfo1Tgt,
                           float lfo2Rate, float lfo2Depth, int lfo2Shape, int lfo2Tgt,
                           float velTimbre, float glideCoeff, float modAmpLevel,
                           float spread, float harmonicShift, float inharmonicity) noexcept
    {
        if (startSample >= endSample) return;

        for (auto& v : voices)
        {
            if (!v.active) continue;

            // Update LFO rates and shapes once per block
            v.lfo1.setRate(lfo1Rate, sampleRateFloat);
            v.lfo1.setShape(lfo1Shape);
            v.lfo2.setRate(lfo2Rate, sampleRateFloat);
            v.lfo2.setShape(lfo2Shape);

            // Update envelopes once per block
            v.ampEnv.setADSR(ampAtk, ampDec, ampSus, ampRel);
            v.filterEnv.setADSR(fltAtk, fltDec, fltSus, fltRel);

            // D001: velocity shifts depth toward bright (layer 0) more
            // velTimbre=1.0 means full velocity-to-brightness mapping
            const float velBrightOffset = velTimbre * v.velocity * 0.5f;

            // Amplitude smoother coefficient: ~20ms smoothing for partial amp changes
            // Prevents zipper when tensor position changes between blocks
            const float ampSmoothCoeff = smoothCoeffFromTime(0.02f, sampleRateFloat);

            // One-pole smoother for partial amplitudes toward target
            for (int h = 0; h < kOctantNumPartials; ++h)
            {
                float target = (h < numPartials) ? targetAmps[h] : 0.0f;

                // Apply velocity timbre: brighter at high velocity (more high partials)
                if (velBrightOffset > 0.0001f && h > 0)
                {
                    // Higher harmonics get a velocity boost
                    const float velBoost = velBrightOffset * (static_cast<float>(h) / 15.0f);
                    target = std::min(1.0f, target + velBoost * target);
                }

                v.smoothAmp[h] += (target - v.smoothAmp[h]) * ampSmoothCoeff;
                v.smoothAmp[h] = flushDenormal(v.smoothAmp[h]);
            }

            // Per-sample render
            for (int s = startSample; s < endSample; ++s)
            {
                // ---- LFOs ----
                const float lfo1Val = v.lfo1.process();
                const float lfo2Val = v.lfo2.process();
                v.lastLfo1Val = lfo1Val;
                v.lastLfo2Val = lfo2Val;

                // ---- Envelopes ----
                const float ampLevel    = v.ampEnv.process();
                const float filterLevel = v.filterEnv.process();

                // ---- Glide-smooth frequency ----
                {
                    const float targetFreq = midiToFreqTune(v.note, harmonicShift)
                        * fastPow2(pitchBendValue * 2.0f / 12.0f);  // ±2 semitones
                    if (glideCoeff > 0.0001f)
                        v.glideFreq = v.glideFreq + (targetFreq - v.glideFreq) * (1.0f - glideCoeff);
                    else
                        v.glideFreq = targetFreq;
                    v.glideFreq = flushDenormal(v.glideFreq);
                }

                // ---- LFO targets ----
                // Targets: 0=Depth, 1=Morph, 2=Rotation, 3=FilterCutoff
                float lfo1DepthOffset  = 0.0f, lfo1MorphOffset   = 0.0f;
                float lfo1RotOffset    = 0.0f, lfo1FilterOffset   = 0.0f;
                float lfo2DepthOffset  = 0.0f, lfo2MorphOffset    = 0.0f;
                float lfo2RotOffset    = 0.0f, lfo2FilterOffset   = 0.0f;

                const float lfo1Scaled = lfo1Val * lfo1Depth;
                const float lfo2Scaled = lfo2Val * lfo2Depth;

                switch (lfo1Tgt)
                {
                case 0: lfo1DepthOffset  = lfo1Scaled * 0.5f;    break;
                case 1: lfo1MorphOffset  = lfo1Scaled * 0.5f;    break;
                case 2: lfo1RotOffset    = lfo1Scaled * 90.0f;   break;
                case 3: lfo1FilterOffset = lfo1Scaled * 4000.0f; break;
                default: break;
                }
                switch (lfo2Tgt)
                {
                case 0: lfo2DepthOffset  = lfo2Scaled * 0.5f;    break;
                case 1: lfo2MorphOffset  = lfo2Scaled * 0.5f;    break;
                case 2: lfo2RotOffset    = lfo2Scaled * 90.0f;   break;
                case 3: lfo2FilterOffset = lfo2Scaled * 4000.0f; break;
                default: break;
                }

                // LFO modulation doesn't re-project the tensor per-sample (too expensive)
                // Instead, LFO modulates the smoothed amplitudes directly via depth proxy
                // This is a valid approximation since changes are slow-rate
                const float totalDepthMod   = lfo1DepthOffset + lfo2DepthOffset;
                const float totalMorphMod   = lfo1MorphOffset + lfo2MorphOffset;
                const float totalFilterMod  = lfo1FilterOffset + lfo2FilterOffset;
                // rotation mod cross-fades between depth-dominant (low-partial) and
                // morph-dominant (high-partial) amplitude weighting — applied per-partial below
                const float totalRotMod     = lfo1RotOffset + lfo2RotOffset;

                // ---- Filter cutoff with key tracking, env, and LFO ----
                float effectiveCutoff = baseCutoff
                    + totalFilterMod
                    + fltKeyTrack * (static_cast<float>(v.note - 60)) * 50.0f;

                // Bipolar filter env amount: !0 check for negative sweeps (D001 lesson)
                if (fltEnvAmt != 0.0f)
                    effectiveCutoff += fltEnvAmt * filterLevel * 8000.0f;

                effectiveCutoff = std::clamp(effectiveCutoff, 20.0f, 20000.0f);

                // ---- Update filter coefficients every 16 samples ----
                if (v.filterUpdateCounter <= 0)
                {
                    const auto mode = static_cast<CytomicSVF::Mode>(
                        std::clamp(fltType, 0, 3));
                    v.outputFilterL.setMode(mode);
                    v.outputFilterR.setMode(mode);
                    v.outputFilterL.setCoefficients(effectiveCutoff, fltReso, sampleRateFloat);
                    v.outputFilterR.setCoefficients(effectiveCutoff, fltReso, sampleRateFloat);
                    v.filterUpdateCounter = 16;
                }
                --v.filterUpdateCounter;

                // ---- Build 16-partial additive sum ----
                float sumL = 0.0f, sumR = 0.0f;

                for (int h = 0; h < numPartials; ++h)
                {
                    // Harmonic number (1-indexed) with inharmonicity stretch
                    // B-coefficient inharmonicity: fn = n * f0 * sqrt(1 + B*n^2)
                    // Use B = inharmonicity * 0.001 for subtle stretching
                    const float n = static_cast<float>(h + 1);
                    const float inhFactor = std::sqrt(1.0f + inharmonicity * 0.001f * n * n);
                    const float partialFreq = v.glideFreq * n * inhFactor;
                    if (partialFreq >= sampleRateFloat * 0.499f) continue; // Nyquist guard

                    // Phase increment per sample
                    const float phaseInc = partialFreq / sampleRateFloat;

                    // FM coupling modulates the phase increment (true FM)
                    const float fmMod = couplingFmBuf[std::min(static_cast<size_t>(s), static_cast<size_t>(maxBlock - 1))];
                    const float fmPhaseAdd = fmMod * 0.01f;

                    // Advance phase accumulator with FM baked into the increment
                    v.partialPhase[h] += phaseInc + fmPhaseAdd;
                    if (v.partialPhase[h] >= 1.0f)
                        v.partialPhase[h] -= 1.0f;

                    // Spread: add a small stereo phase offset per partial
                    // Alternating sign creates L/R decorrelation
                    const float sign = ((h % 2) == 0) ? 1.0f : -1.0f;
                    const float spreadOffset = sign * spread * 0.02f * static_cast<float>(h + 1);

                    // Compute sine
                    const float sineL = fastSin(v.partialPhase[h] * kOctantTwoPi);
                    const float sineR = fastSin((v.partialPhase[h] + spreadOffset) * kOctantTwoPi);

                    // LFO depth/morph modulation on amplitude: crude but effective
                    // Scale the smoothed amplitude by a small LFO-driven envelope
                    float ampMod = 1.0f;
                    if (std::fabs(totalDepthMod) > 0.0001f)
                    {
                        // deeper depth = darker (reduce high partials)
                        const float brightWeight = static_cast<float>(h) / 15.0f;
                        ampMod += totalDepthMod * brightWeight * 0.5f;
                    }
                    if (std::fabs(totalMorphMod) > 0.0001f)
                    {
                        // morph modulation: odd partials respond more
                        const bool isOdd = ((h + 1) % 2 == 1);
                        ampMod += totalMorphMod * (isOdd ? 0.3f : -0.1f);
                    }
                    if (std::fabs(totalRotMod) > 0.0001f)
                    {
                        // Rotation modulates the balance between low and high partials:
                        // positive rotation boosts high partials (morph-projection dominant),
                        // negative rotation boosts low partials (depth-projection dominant).
                        // rotWeight runs [-1, 1] across the partial stack.
                        const float rotWeight = (static_cast<float>(h) / 15.0f) * 2.0f - 1.0f;
                        ampMod += totalRotMod * rotWeight * 0.3f;
                    }
                    ampMod = std::clamp(ampMod, 0.0f, 2.0f);

                    const float amp = v.smoothAmp[h] * ampMod;
                    sumL += sineL * amp;
                    sumR += sineR * amp;
                }

                // ---- VCA ----
                const float finalAmp = ampLevel * v.velocity * (1.0f + modAmpLevel * 0.5f);
                sumL *= finalAmp;
                sumR *= finalAmp;

                // ---- Soft clip to prevent hard clipping ----
                sumL = softClip(sumL * 0.5f) * 2.0f;
                sumR = softClip(sumR * 0.5f) * 2.0f;

                // ---- Output filter ----
                const float outL = v.outputFilterL.processSample(sumL);
                const float outR = v.outputFilterR.processSample(sumR);

                writeL[s] += outL;
                writeR[s] += outR;

                // ---- Deactivate voice when amp env is idle ----
                if (!v.ampEnv.isActive())
                {
                    v.active = false;
                    break;
                }
            }
        }
    }

    //==========================================================================
    //  N O T E   H A N D L E R S
    //==========================================================================

    void handleNoteOn(int noteNumber, int velocity,
                      float ampAtk, float ampDec, float ampSus, float ampRel,
                      float fltAtk, float fltDec, float fltSus, float fltRel,
                      int voiceMode, float glideTime) noexcept
    {
        const float vel01 = static_cast<float>(velocity) / 127.0f;

        // Find a voice to use
        OctantVoice* target = nullptr;

        if (voiceMode == 0 || voiceMode == 1)
        {
            // Mono / Legato: always use voice 0
            target = &voices[0];
        }
        else
        {
            // Poly4 or Poly8: find free voice, then steal
            const int maxV = (voiceMode == 2) ? 4 : kOctantMaxVoices;

            // First: find an inactive voice
            for (int i = 0; i < maxV; ++i)
            {
                if (!voices[i].active)
                {
                    target = &voices[i];
                    break;
                }
            }
            // Steal: pass 1 — prefer the quietest RELEASING voice
            if (target == nullptr)
            {
                float lowestAmp = 2.0f;
                for (int i = 0; i < maxV; ++i)
                {
                    if (voices[i].releasing && voices[i].ampEnv.getLevel() < lowestAmp)
                    {
                        lowestAmp = voices[i].ampEnv.getLevel();
                        target = &voices[i];
                    }
                }
            }
            // Steal: pass 2 — fall back to the quietest voice regardless of state
            if (target == nullptr)
            {
                float lowestAmp = 2.0f;
                for (int i = 0; i < maxV; ++i)
                {
                    if (voices[i].ampEnv.getLevel() < lowestAmp)
                    {
                        lowestAmp = voices[i].ampEnv.getLevel();
                        target = &voices[i];
                    }
                }
            }
        }

        if (target == nullptr) return;

        const bool wasActive = target->active;
        const float prevGlideFreq = target->glideFreq;

        target->active    = true;
        target->releasing = false;
        target->note      = noteNumber;
        target->velocity  = vel01;
        target->keyTrack  = (static_cast<float>(noteNumber) - 60.0f) / 60.0f;

        // Glide: set target freq but keep current freq for smoothing
        if (wasActive && (voiceMode == 1 || glideTime > 0.0001f))
            target->glideFreq = prevGlideFreq;
        else
            target->glideFreq = midiToFreq(noteNumber);

        // Initialize partial amplitudes to 0 on fresh voice
        if (!wasActive)
        {
            for (int h = 0; h < kOctantNumPartials; ++h)
                target->smoothAmp[h] = 0.0f;
            target->outputFilterL.reset();
            target->outputFilterR.reset();
        }

        // Legato retrigger
        if (wasActive && voiceMode == 1)
        {
            target->ampEnv.retriggerFrom(target->ampEnv.getLevel(),
                                         ampAtk, ampDec, ampSus, ampRel);
            target->filterEnv.retriggerFrom(target->filterEnv.getLevel(),
                                            fltAtk, fltDec, fltSus, fltRel);
        }
        else
        {
            target->ampEnv.setADSR(ampAtk, ampDec, ampSus, ampRel);
            target->filterEnv.setADSR(fltAtk, fltDec, fltSus, fltRel);
            target->ampEnv.noteOn();
            target->filterEnv.noteOn();
        }

        // Reset phases for fresh voices
        if (!wasActive)
        {
            for (int h = 0; h < kOctantNumPartials; ++h)
                target->partialPhase[h] = 0.0f;
            target->lfo1.reset();
            target->lfo2.reset();
            target->lfo2.reseed(static_cast<uint32_t>(noteNumber) * 7919u);
        }

        target->filterUpdateCounter = 0;
    }

    void handleNoteOff(int noteNumber) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && !v.releasing && v.note == noteNumber)
            {
                v.releasing = true;
                v.ampEnv.noteOff();
                v.filterEnv.noteOff();
            }
        }
    }

    //==========================================================================
    //  M E M B E R S
    //==========================================================================

    // Do not default-init — must be set by prepare() on the live sample rate.
    // Sentinel 0.0 makes misuse before prepare() a crash instead of silent wrong-rate DSP.
    float sampleRateFloat = 0.0f;
    int   maxBlock        = 512;

    std::array<OctantVoice, kOctantMaxVoices> voices;

    // Coupling state
    std::vector<float> couplingFmBuf;
    float couplingAmpFilter = 0.0f;
    float couplingEnvMorph  = 0.0f;
    float couplingRhythm    = 0.0f;

    // Expression state
    float modWheelValue   = 0.0f;
    float aftertouchValue = 0.0f;
    float pitchBendValue  = 0.0f;  // [-1, +1] range, ±2 semitones

    // Coupling output cache
    float lastSampleL = 0.0f;
    float lastSampleR = 0.0f;
    float lastDepth   = 0.5f;

    // Mod matrix
    ModMatrix<4> modMatrix;
    ModMatrix<4>::Sources blockModSrc;

    // ---- Cached parameter pointers ----
    // Group A — Tensor Navigation
    std::atomic<float>* pDepth         = nullptr;
    std::atomic<float>* pMorph         = nullptr;
    std::atomic<float>* pRotation      = nullptr;
    std::atomic<float>* pSpread        = nullptr;
    std::atomic<float>* pHarmonicShift = nullptr;
    std::atomic<float>* pPartialCount  = nullptr;

    // Group B — Tensor Character
    std::atomic<float>* pTensorWarp     = nullptr;
    std::atomic<float>* pSpectralTilt   = nullptr;
    std::atomic<float>* pOddEvenBalance = nullptr;
    std::atomic<float>* pInharmonicity  = nullptr;

    // Group C — Filter
    std::atomic<float>* pFltCutoff   = nullptr;
    std::atomic<float>* pFltReso     = nullptr;
    std::atomic<float>* pFltType     = nullptr;
    std::atomic<float>* pFltEnvAmt   = nullptr;
    std::atomic<float>* pFltKeyTrack = nullptr;
    std::atomic<float>* pFltAtk      = nullptr;
    std::atomic<float>* pFltDec      = nullptr;
    std::atomic<float>* pFltSus      = nullptr;
    std::atomic<float>* pFltRel      = nullptr;

    // Group D — Amp Envelope
    std::atomic<float>* pAmpAtk    = nullptr;
    std::atomic<float>* pAmpDec    = nullptr;
    std::atomic<float>* pAmpSus    = nullptr;
    std::atomic<float>* pAmpRel    = nullptr;
    std::atomic<float>* pVelTimbre = nullptr;

    // Group E — LFOs
    std::atomic<float>* pLfo1Rate   = nullptr;
    std::atomic<float>* pLfo1Depth  = nullptr;
    std::atomic<float>* pLfo1Shape  = nullptr;
    std::atomic<float>* pLfo1Target = nullptr;
    std::atomic<float>* pLfo2Rate   = nullptr;
    std::atomic<float>* pLfo2Depth  = nullptr;
    std::atomic<float>* pLfo2Shape  = nullptr;
    std::atomic<float>* pLfo2Target = nullptr;

    // Group G — Macros + Voice
    std::atomic<float>* pMacro1    = nullptr;
    std::atomic<float>* pMacro2    = nullptr;
    std::atomic<float>* pMacro3    = nullptr;
    std::atomic<float>* pMacro4    = nullptr;
    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlide     = nullptr;
    std::atomic<float>* pGlideMode = nullptr;
};

} // namespace xoceanus
