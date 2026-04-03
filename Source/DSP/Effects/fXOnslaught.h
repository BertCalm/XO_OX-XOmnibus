// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <array>
#include <vector>
#include "../FastMath.h"
#include "../CytomicSVF.h"
#include "../StandardLFO.h"

namespace xoceanus
{

//==============================================================================
// fXOnslaught — "Wave Function Collapse" boutique effect.
//
// A two-state machine: lush, liquid allpass chorus (Flow) that violently
// collapses into aggressive phase modulation (Collapse) on transient detection.
// The transient is the "Observer" — the quantum measurement that shatters the
// wave function.
//
// Signal flow:
//   input → capture buffer (continuous)
//         → transient detector (dual envelope, sidechain HP)
//         │
//         ├─ FLOW STATE: 4-stage CytomicSVF allpass cascade (chorus)
//         │   with LFO modulation + resonance feedback (Moog)
//         │
//         └─ COLLAPSE STATE: grain capture → phase modulation
//             with grain evolution (Buchla phase rotation)
//             and recovery shimmer (Tomita)
//         │
//         └─ crossfade via collapseEnvelope → output
//
// Character: Liquid beauty that shatters into metallic aggression on impact.
//   Smooth pads become jagged FM when a snare hits. Ambient chords collapse
//   into industrial texture on a kick. Then the chaos decays, and the flow
//   returns — with a brief shimmer of recovery.
//
// Ghost guidance baked in:
//   Buchla — grain evolution via phase rotation during collapse
//   Moog   — resonance feedback from APF cascade output to input
//   Tomita — recovery shimmer (APF depth boost on collapse relaxation)
//   Vangelis — sidechain HP on transient detector
//
// CPU budget: ~60 ops/sample (~0.3% @ 48kHz)
// Bricks: CytomicSVF ×5 (4 APF + 1 sidechain HP), StandardLFO ×1, FastMath
//
// Usage:
//   fXOnslaught fx;
//   fx.prepare(48000.0, 512);
//   fx.setFlowRate(0.8f);        // chorus LFO Hz
//   fx.setFlowDepth(0.5f);       // APF modulation depth
//   fx.setThreshold(4.0f);       // transient sensitivity
//   fx.setCollapseDepth(0.6f);   // PM intensity
//   fx.setCollapseRate(2.0f);    // grain read speed ratio
//   fx.setCollapseDecay(300.0f); // ms before flow returns
//   fx.setSidechainHP(200.0f);   // Hz — filter out bass from detector
//   fx.setMix(0.4f);
//   fx.processBlock(L, R, numSamples);
//==============================================================================
class fXOnslaught
{
public:
    fXOnslaught() = default;

    //--------------------------------------------------------------------------
    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;

        // 4-stage allpass cascade for Flow chorus
        for (int i = 0; i < kNumAPFs; ++i)
        {
            apfL[i].setMode(CytomicSVF::Mode::AllPass);
            apfR[i].setMode(CytomicSVF::Mode::AllPass);
            apfL[i].reset();
            apfR[i].reset();
        }

        // Sidechain HP filter on transient detector input (Vangelis)
        sidechainHP.setMode(CytomicSVF::Mode::HighPass);
        sidechainHP.setCoefficients(sidechainHPFreq, 0.5f, static_cast<float>(sr));
        sidechainHP.reset();

        // LFO for APF modulation — sine, sub-Hz
        lfo.setShape(StandardLFO::Sine);
        lfo.setRate(flowRate, static_cast<float>(sr));
        lfo.reset();

        // Capture buffer: 50ms circular buffer for grain source
        int captureLen = static_cast<int>(sr * 0.05) + 1;
        captureBuffer.assign(static_cast<size_t>(captureLen), 0.0f);
        captureSize = captureLen;
        captureWritePos = 0;

        // Grain buffer: fixed 2048 samples (Architect condition: std::array, not vector)
        grainBuffer.fill(0.0f);
        grainReadPos = 0.0f;
        grainRotationPhase = 0.0f;

        // Envelope followers for transient detection
        shortEnv = longEnv = 0.0f;

        // Collapse state
        collapseEnv = 0.0f;
        recoveryShimmerEnv = 0.0f;

        // Resonance feedback state (Moog)
        resFeedbackL = resFeedbackR = 0.0f;
    }

    //--------------------------------------------------------------------------
    /// Flow LFO rate in Hz (0.1-5.0).
    void setFlowRate(float hz)
    {
        flowRate = clamp(hz, 0.1f, 5.0f);
        lfo.setRate(flowRate, static_cast<float>(sr));
    }

    /// Flow APF modulation depth (0-1).
    void setFlowDepth(float d) { flowDepth = clamp(d, 0.0f, 1.0f); }

    /// Transient detection sensitivity — ratio threshold (1.0-20.0).
    void setThreshold(float t) { threshold = clamp(t, 1.0f, 20.0f); }

    /// Collapse PM intensity (0-1).
    void setCollapseDepth(float d) { collapseDepth = clamp(d, 0.0f, 1.0f); }

    /// Grain read speed ratio (0.25-8.0). 1.0 = original speed, 2.0 = double.
    void setCollapseRate(float r) { collapseRate = clamp(r, 0.25f, 8.0f); }

    /// Collapse decay time in ms (10-2000).
    void setCollapseDecay(float ms) { collapseDecayMs = clamp(ms, 10.0f, 2000.0f); }

    /// Sidechain HP frequency for transient detector (20-2000 Hz).
    /// Filters bass from the detector — lets you trigger only on high transients.
    void setSidechainHP(float hz)
    {
        sidechainHPFreq = clamp(hz, 20.0f, 2000.0f);
        sidechainHP.setCoefficients(sidechainHPFreq, 0.5f, static_cast<float>(sr));
    }

    /// Dry/wet mix (0-1).
    void setMix(float m) { mix = clamp(m, 0.0f, 1.0f); }

    /// Read the current collapse envelope value (for UI metering).
    float getCollapseLevel() const noexcept { return collapseEnv; }

    //--------------------------------------------------------------------------
    /// Process stereo audio in-place.
    void processBlock(float* L, float* R, int numSamples)
    {
        if (mix < 0.001f)
            return;

        // Pre-compute per-block constants
        const float srF = static_cast<float>(sr);

        // Envelope follower coefficients
        const float shortAttack = smoothCoeffFromTime(0.001f, srF); // 1ms
        const float shortRelease = smoothCoeffFromTime(0.01f, srF); // 10ms
        const float longAttack = smoothCoeffFromTime(0.05f, srF);   // 50ms
        const float longRelease = smoothCoeffFromTime(0.15f, srF);  // 150ms

        // Collapse decay coefficient (exponential per-sample)
        const float decaySamples = collapseDecayMs * 0.001f * srF;
        const float decayCoeff = (decaySamples > 1.0f) ? fastExp(-6.9078f / decaySamples) // -60dB over decay time
                                                       : 0.0f;

        // Recovery shimmer decay (~50ms)
        const float shimmerDecay = smoothCoeffFromTime(0.05f, srF);

        // Grain rotation rate (Buchla evolution): slow phase rotation
        const float grainRotationRate = 0.0003f; // ~0.3 rad/sample evolution

        // APF base frequencies spanning the spectrum
        static constexpr float apfBaseFreqs[kNumAPFs] = {200.0f, 800.0f, 2400.0f, 6000.0f};

        // Resonance feedback amount (Moog): subtle, prevents runaway
        static constexpr float resFeedbackAmount = 0.15f;

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = L[i];
            float inR = R[i];

            // === Write to capture buffer (always running) ===
            float monoIn = (inL + inR) * 0.5f;
            captureBuffer[static_cast<size_t>(captureWritePos)] = monoIn;
            captureWritePos = (captureWritePos + 1) % captureSize;

            // === Transient detection (dual envelope follower) ===
            // Sidechain HP removes bass from detector (Vangelis)
            float detectorIn = sidechainHP.processSample(monoIn);
            float absDetector = std::fabs(detectorIn);

            float shortCoeff = (absDetector > shortEnv) ? shortAttack : shortRelease;
            shortEnv = flushDenormal(shortEnv + shortCoeff * (absDetector - shortEnv));

            float longCoeff = (absDetector > longEnv) ? longAttack : longRelease;
            longEnv = flushDenormal(longEnv + longCoeff * (absDetector - longEnv));

            // Transient ratio: short spike relative to ambient level
            float transientRatio = (longEnv > 1e-6f) ? (shortEnv / longEnv) : 0.0f;

            // === Trigger collapse on transient ===
            if (transientRatio > threshold && collapseEnv < 0.01f)
            {
                // Capture grain: copy last kGrainSize samples from circular buffer
                for (int g = 0; g < kGrainSize; ++g)
                {
                    int idx = (captureWritePos - kGrainSize + g + captureSize) % captureSize;
                    grainBuffer[static_cast<size_t>(g)] = captureBuffer[static_cast<size_t>(idx)];
                }
                collapseEnv = 1.0f;
                grainReadPos = 0.0f;
                grainRotationPhase = 0.0f;
            }

            // === Recovery shimmer trigger (Tomita) ===
            // When collapse crosses below 0.1, brief shimmer burst
            if (collapseEnv < 0.1f && collapseEnv > 0.001f && recoveryShimmerEnv < 0.01f)
            {
                recoveryShimmerEnv = 0.5f;
            }
            recoveryShimmerEnv = flushDenormal(recoveryShimmerEnv * (1.0f - shimmerDecay));

            // === LFO for flow chorus ===
            float lfoVal = lfo.process();

            // Effective flow depth: boosted during recovery shimmer
            float effectiveFlowDepth = flowDepth + recoveryShimmerEnv * 0.3f;
            effectiveFlowDepth = clamp(effectiveFlowDepth, 0.0f, 1.0f);

            // === FLOW STATE: 4-stage allpass cascade ===
            // Add resonance feedback from cascade output (Moog)
            float flowInL = inL + resFeedbackL * resFeedbackAmount;
            float flowInR = inR + resFeedbackR * resFeedbackAmount;

            float flowL = flowInL;
            float flowR = flowInR;

            for (int a = 0; a < kNumAPFs; ++a)
            {
                // Stagger LFO phase per stage for richer chorus
                float stagger = static_cast<float>(a) * 0.25f;
                float modVal = fastSin((lfoVal * 0.5f + 0.5f + stagger) * 6.28318530718f);
                float freq = apfBaseFreqs[a] * (1.0f + modVal * effectiveFlowDepth * 0.5f);
                freq = clamp(freq, 20.0f, srF * 0.49f);

                apfL[a].setCoefficients_fast(freq, 0.5f, srF);
                apfR[a].setCoefficients_fast(freq, 0.5f, srF);

                flowL = apfL[a].processSample(flowL);
                flowR = apfR[a].processSample(flowR);
            }

            // Store cascade output for resonance feedback (Moog)
            resFeedbackL = flushDenormal(flowL);
            resFeedbackR = flushDenormal(flowR);

            // === COLLAPSE STATE: phase modulation from grain buffer ===
            float collapseL = inL;
            float collapseR = inR;

            if (collapseEnv > 0.001f)
            {
                // Read grain with linear interpolation (Architect condition #2)
                int grainIdx0 = static_cast<int>(grainReadPos) % kGrainSize;
                int grainIdx1 = (grainIdx0 + 1) % kGrainSize;
                float frac = grainReadPos - static_cast<float>(static_cast<int>(grainReadPos));
                float grainRaw = lerp(grainBuffer[static_cast<size_t>(grainIdx0)],
                                      grainBuffer[static_cast<size_t>(grainIdx1)], frac);

                // Grain evolution: phase rotation (Buchla)
                // Slowly rotates the grain's effective waveform during collapse
                float evolvedGrain = grainRaw * fastCos(grainRotationPhase) +
                                     grainBuffer[static_cast<size_t>((grainIdx0 + kGrainSize / 4) % kGrainSize)] *
                                         fastSin(grainRotationPhase);
                grainRotationPhase += grainRotationRate;

                // Phase modulation: grain modulates the phase of the input
                float modPhase = evolvedGrain * collapseDepth * collapseEnv;
                float pmCos = fastCos(modPhase * 6.28318530718f);

                collapseL = inL * pmCos;
                collapseR = inR * pmCos;

                // Advance grain read position
                grainReadPos += collapseRate;
                if (grainReadPos >= static_cast<float>(kGrainSize))
                    grainReadPos -= static_cast<float>(kGrainSize);

                // Decay the collapse envelope
                collapseEnv = flushDenormal(collapseEnv * decayCoeff);
            }

            // === Crossfade: flow ←→ collapse ===
            float wetL = flowL * (1.0f - collapseEnv) + collapseL * collapseEnv;
            float wetR = flowR * (1.0f - collapseEnv) + collapseR * collapseEnv;

            // === Mix dry/wet ===
            L[i] = inL * (1.0f - mix) + wetL * mix;
            R[i] = inR * (1.0f - mix) + wetR * mix;
        }
    }

    //--------------------------------------------------------------------------
    void reset()
    {
        for (int i = 0; i < kNumAPFs; ++i)
        {
            apfL[i].reset();
            apfR[i].reset();
        }
        sidechainHP.reset();
        lfo.reset();

        if (!captureBuffer.empty())
            std::fill(captureBuffer.begin(), captureBuffer.end(), 0.0f);
        grainBuffer.fill(0.0f);

        captureWritePos = 0;
        grainReadPos = 0.0f;
        grainRotationPhase = 0.0f;
        shortEnv = longEnv = 0.0f;
        collapseEnv = 0.0f;
        recoveryShimmerEnv = 0.0f;
        resFeedbackL = resFeedbackR = 0.0f;
    }

private:
    //--------------------------------------------------------------------------
    static constexpr int kNumAPFs = 4;
    static constexpr int kGrainSize = 2048;

    double sr = 44100.0;

    // 4-stage allpass cascade (Flow state)
    CytomicSVF apfL[kNumAPFs];
    CytomicSVF apfR[kNumAPFs];

    // Sidechain HP on transient detector (Vangelis)
    CytomicSVF sidechainHP;

    // Chorus LFO
    StandardLFO lfo;

    // Capture buffer: continuous circular buffer for grain source
    std::vector<float> captureBuffer;
    int captureSize = 0;
    int captureWritePos = 0;

    // Grain buffer: captured transient (std::array per Architect condition #1)
    std::array<float, kGrainSize> grainBuffer{};
    float grainReadPos = 0.0f;
    float grainRotationPhase = 0.0f; // Buchla grain evolution

    // Transient detection
    float shortEnv = 0.0f;
    float longEnv = 0.0f;

    // Collapse state
    float collapseEnv = 0.0f;
    float recoveryShimmerEnv = 0.0f; // Tomita recovery shimmer

    // Resonance feedback (Moog)
    float resFeedbackL = 0.0f;
    float resFeedbackR = 0.0f;

    // Parameters
    float flowRate = 0.8f;
    float flowDepth = 0.5f;
    float threshold = 4.0f;
    float collapseDepth = 0.6f;
    float collapseRate = 2.0f;
    float collapseDecayMs = 300.0f;
    float sidechainHPFreq = 200.0f;
    float mix = 0.0f;
};

} // namespace xoceanus
