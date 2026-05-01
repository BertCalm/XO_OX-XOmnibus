// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

#include "../../Core/DNAModulationBus.h"
#include "../../Core/PartnerAudioBus.h"
#include "../ParameterSmoother.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <cmath>

namespace xoceanus
{

//==============================================================================
// OtriumChain — Triangular Coupling Pump (FX Pack 1, Sidechain Creative)
//
// Wildcard: 3 partner engines duck each other in a phase-staggered loop
// (the matrix demo). Reuses existing TriangularCoupling primitive — no
// MegaCouplingMatrix changes required.
//
// Parameter prefix: otrm_  (13 params, locked in Pack 1 spec §10 incl. A3)
//
// Routing: Stereo In → Stereo Out (no expansion stage)
// Accent: TBD — Pack 1 color table review
//
// DSP: 3 partner engines duck each other in a phase-staggered loop.
//   1. Triangular Input Router — partnerX_idx params select 3 engine slots;
//      partner mono audio is read from PartnerAudioBus per block.
//   2. Phase-Stagger Envelope Followers — one-pole attack/release per partner,
//      modulated by sine LFO at pumpRate with phaseSkew° offset between followers.
//   3. Cross-Triangle Mixer — A-env ducks the B-path, B-env ducks the C-path,
//      C-env ducks the A-path (the "phase-staggered loop" wildcard).
//   4. VCA Bank — per-path gain = 1 - depth × cross-routed envelope.
//   5. Output Mixer — averages the 3 paths, blends to dry via otrm_mix.
//
// couplingDepth blends autonomous LFO ducking ↔ partner-driven envelope ducking.
// dnaTilt scales depth by partner aggression DNA (read via DNAModulationBus).
//
// See: Docs/specs/2026-04-27-fx-pack-1-sidechain-creative.md §2
//==============================================================================
class OtriumChain
{
public:
    OtriumChain() = default;

    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr_ = sampleRate;
        pumpDepthSmoothed_.reset(sampleRate, 0.02);
        mixSmoothed_.reset(sampleRate, 0.02);
    }

    void reset()
    {
        pumpDepthSmoothed_.setCurrentAndTargetValue(0.5f);
        mixSmoothed_.setCurrentAndTargetValue(1.0f);
        envA_ = envB_ = envC_ = 0.0f;
        lfoPhase_ = 0.0f;
        chaoticOffsetB_ = juce::degreesToRadians(120.0f);
        chaoticOffsetC_ = juce::degreesToRadians(240.0f);
        chaoticRng_ = 0x9E3779B9u; // golden-ratio seed; deterministic per reset
    }

    // Inject DNA bus pointer (set once on message thread before audio starts).
    // Read lock-free per block to apply dnaTilt scaling to ducking depth.
    void setDNABus(const DNAModulationBus* bus) noexcept { dnaBus_ = bus; }

    // Inject partner audio bus pointer (set once on message thread before
    // audio starts). Read lock-free per block to pull partner mono mix for
    // each of the 3 envelope followers.
    void setPartnerAudioBus(const PartnerAudioBus* bus) noexcept { partnerBus_ = bus; }

    // Real DSP — 3-partner phase-staggered cross-ducking.
    void processBlock(float* L, float* R, int numSamples,
                      double bpm = 0.0, double /*ppqPosition*/ = -1.0)
    {
        if (! pPumpDepth_ || ! pMix_) return;

        // ---- Block-rate parameter loads ----
        pumpDepthSmoothed_.setTargetValue(pPumpDepth_->load(std::memory_order_relaxed));
        mixSmoothed_.setTargetValue(pMix_->load(std::memory_order_relaxed));

        const float pumpRate     = pPumpRate_      ? pPumpRate_->load(std::memory_order_relaxed)      : 1.0f;
        const float atkMs        = pAttack_        ? pAttack_->load(std::memory_order_relaxed)        : 5.0f;
        const float relMs        = pRelease_       ? pRelease_->load(std::memory_order_relaxed)       : 200.0f;
        const float phaseSkewDeg = pPhaseSkew_     ? pPhaseSkew_->load(std::memory_order_relaxed)     : 120.0f;
        const float couplingAmt  = pCouplingDepth_ ? pCouplingDepth_->load(std::memory_order_relaxed) : 0.5f;
        const float dnaTilt      = pDnaTilt_       ? pDnaTilt_->load(std::memory_order_relaxed)       : 0.0f;
        const int   idxA         = pPartnerA_      ? clampSlot(pPartnerA_->load(std::memory_order_relaxed)) : 0;
        const int   idxB         = pPartnerB_      ? clampSlot(pPartnerB_->load(std::memory_order_relaxed)) : 1;
        const int   idxC         = pPartnerC_      ? clampSlot(pPartnerC_->load(std::memory_order_relaxed)) : 2;
        const int   topology     = pTopology_      ? static_cast<int>(pTopology_->load(std::memory_order_relaxed) + 0.5f) : 0;
        const bool  syncOn       = pSyncMode_      ? pSyncMode_->load(std::memory_order_relaxed) >= 0.5f                  : false;

        // ---- Resolve partner audio (nullptr when slot silent / no bus) ----
        const float* pA = partnerBus_ ? partnerBus_->getMono(idxA) : nullptr;
        const float* pB = partnerBus_ ? partnerBus_->getMono(idxB) : nullptr;
        const float* pC = partnerBus_ ? partnerBus_->getMono(idxC) : nullptr;

        // ---- Envelope-follower coefficients (one-pole, exact ms → coef) ----
        const float atkCoef = (sr_ > 0.0 && atkMs > 0.0f)
                                  ? std::exp(-1.0f / (atkMs * 0.001f * static_cast<float>(sr_)))
                                  : 0.0f;
        const float relCoef = (sr_ > 0.0 && relMs > 0.0f)
                                  ? std::exp(-1.0f / (relMs * 0.001f * static_cast<float>(sr_)))
                                  : 0.0f;

        // ---- Sync mode: reinterpret pumpRate as cycles-per-beat when locked ----
        // Free  → pumpRate is Hz directly (D005 floor at 0.001 Hz applies).
        // Sync  → pumpRate is cycles per beat; effective Hz = pumpRate * bpm / 60.
        //          When bpm is unknown (offline render, no transport), fall back
        //          to Free behaviour so the chain never falls silent.
        const float effectiveRateHz = (syncOn && bpm > 0.0)
                                          ? pumpRate * static_cast<float>(bpm) * (1.0f / 60.0f)
                                          : pumpRate;

        // ---- LFO step (radians per sample, with D005 0.001 Hz floor) ----
        const float twoPi   = juce::MathConstants<float>::twoPi;
        const float lfoInc  = (sr_ > 0.0)
                                  ? twoPi * effectiveRateHz / static_cast<float>(sr_)
                                  : 0.0f;

        // ---- Topology: per-partner phase offset routing ----
        //   Equilateral (0): fixed 120° / 240° — phaseSkew ignored, perfect triangle
        //   Isoceles    (1): user phaseSkew between A↔B and B↔C (current behaviour)
        //   Chaotic     (2): per-partner offsets re-randomised on every LFO wrap
        //                    (deterministic LCG seed, audible but reproducible)
        //   Cyclical    (3): user skew, but offsets rotate slowly (skew + driftAngle)
        //                    creating a continuous through-zero phase walk
        constexpr int kTopoEq = 0, kTopoIso = 1, kTopoChaos = 2, kTopoCyc = 3;
        const float skewRad = (topology == kTopoEq)
                                  ? juce::degreesToRadians(120.0f)
                                  : juce::degreesToRadians(phaseSkewDeg);

        // ---- DNA tilt: scale depth by partner aggression on slot 0 ----
        // Partner DNA (per spec) modulates duck spectrum; scaffold uses depth.
        // Real implementation may route this into a tilt EQ on the duck path.
        float aggression = 0.5f;
        if (dnaBus_)
            aggression = dnaBus_->get(0, DNAModulationBus::Axis::Aggression);
        const float dnaScale = 1.0f + dnaTilt * (aggression - 0.5f) * 2.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float depth = pumpDepthSmoothed_.getNextValue() * dnaScale;
            const float mix   = mixSmoothed_.getNextValue();

            // Envelope followers (silent when partner slot null)
            const float aIn = pA ? std::abs(pA[i]) : 0.0f;
            const float bIn = pB ? std::abs(pB[i]) : 0.0f;
            const float cIn = pC ? std::abs(pC[i]) : 0.0f;
            envA_ = (aIn > envA_) ? atkCoef * envA_ + (1.0f - atkCoef) * aIn : relCoef * envA_;
            envB_ = (bIn > envB_) ? atkCoef * envB_ + (1.0f - atkCoef) * bIn : relCoef * envB_;
            envC_ = (cIn > envC_) ? atkCoef * envC_ + (1.0f - atkCoef) * cIn : relCoef * envC_;

            // Per-partner phase offsets driven by topology.
            float offsetB, offsetC;
            switch (topology)
            {
                case kTopoChaos:
                    offsetB = chaoticOffsetB_;
                    offsetC = chaoticOffsetC_;
                    break;
                case kTopoCyc:
                    // Slow drift: per-partner phase walks at ¹⁄₈ of the LFO rate,
                    // so the triangle continuously rotates without ever phase-locking.
                    offsetB = skewRad           + lfoPhase_ * 0.125f;
                    offsetC = 2.0f * skewRad    + lfoPhase_ * 0.250f;
                    break;
                case kTopoEq:
                case kTopoIso:
                default:
                    offsetB = skewRad;
                    offsetC = 2.0f * skewRad;
                    break;
            }

            // 3 LFO outputs (unipolar 0..1)
            const float lfoA = 0.5f + 0.5f * std::sin(lfoPhase_);
            const float lfoB = 0.5f + 0.5f * std::sin(lfoPhase_ + offsetB);
            const float lfoC = 0.5f + 0.5f * std::sin(lfoPhase_ + offsetC);

            lfoPhase_ += lfoInc;
            if (lfoPhase_ > twoPi)
            {
                lfoPhase_ -= twoPi;
                // LFO wrap → re-seed Chaotic offsets via deterministic LCG
                // (Numerical Recipes constants: a=1664525, c=1013904223, m=2^32).
                if (topology == kTopoChaos)
                {
                    chaoticRng_ = chaoticRng_ * 1664525u + 1013904223u;
                    chaoticOffsetB_ = static_cast<float>((chaoticRng_ >> 8) & 0xFFFFFF)
                                          * (twoPi / 16777216.0f);
                    chaoticRng_ = chaoticRng_ * 1664525u + 1013904223u;
                    chaoticOffsetC_ = static_cast<float>((chaoticRng_ >> 8) & 0xFFFFFF)
                                          * (twoPi / 16777216.0f);
                }
            }

            // Cross-rotate: A-path ducked by env C, B by env A, C by env B.
            // couplingAmt blends autonomous LFO (0) ↔ partner envelope (1).
            const float duckA = depth * (couplingAmt * envC_ * lfoC + (1.0f - couplingAmt) * lfoC);
            const float duckB = depth * (couplingAmt * envA_ * lfoA + (1.0f - couplingAmt) * lfoA);
            const float duckC = depth * (couplingAmt * envB_ * lfoB + (1.0f - couplingAmt) * lfoB);

            const float gainA = std::max(0.0f, 1.0f - duckA);
            const float gainB = std::max(0.0f, 1.0f - duckB);
            const float gainC = std::max(0.0f, 1.0f - duckC);
            const float avgGain = (gainA + gainB + gainC) * (1.0f / 3.0f);

            const float dryL = L[i];
            const float dryR = R[i];
            L[i] = dryL * (1.0f - mix) + dryL * avgGain * mix;
            R[i] = dryR * (1.0f - mix) + dryR * avgGain * mix;
        }
    }

    //--------------------------------------------------------------------------
    // APVTS integration — 13 parameters per Pack 1 spec §2.
    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& slotPrefix = "")
    {
        const juce::String p = slotPrefix + "otrm_";

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "pumpDepth", 1), "OTRM Pump Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "pumpRate", 1), "OTRM Pump Rate",
            juce::NormalisableRange<float>(0.001f, 40.0f, 0.0f, 0.3f), 1.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "attack", 1), "OTRM Attack",
            juce::NormalisableRange<float>(0.1f, 200.0f, 0.0f, 0.5f), 5.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "release", 1), "OTRM Release",
            juce::NormalisableRange<float>(10.0f, 2000.0f, 0.0f, 0.5f), 200.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "phaseSkew", 1), "OTRM Phase Skew",
            juce::NormalisableRange<float>(0.0f, 360.0f), 120.0f));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID(p + "topology", 1), "OTRM Topology",
            juce::StringArray{"Equilateral", "Isoceles", "Chaotic", "Cyclical"}, 0));
        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID(p + "partnerA_idx", 1), "OTRM Partner A", 0, 3, 0));
        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID(p + "partnerB_idx", 1), "OTRM Partner B", 0, 3, 1));
        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID(p + "partnerC_idx", 1), "OTRM Partner C", 0, 3, 2));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "couplingDepth", 1), "OTRM Coupling Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "dnaTilt", 1), "OTRM DNA Tilt",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "mix", 1), "OTRM Mix",
            juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));
        // A3 (locked 2026-04-27): tempo-sync mode for Cyclical topology.
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID(p + "syncMode", 1), "OTRM Sync Mode",
            juce::StringArray{"Free", "Sync"}, 0));
    }

    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "")
    {
        const juce::String p = slotPrefix + "otrm_";
        pPumpDepth_     = apvts.getRawParameterValue(p + "pumpDepth");
        pPumpRate_      = apvts.getRawParameterValue(p + "pumpRate");
        pAttack_        = apvts.getRawParameterValue(p + "attack");
        pRelease_       = apvts.getRawParameterValue(p + "release");
        pPhaseSkew_     = apvts.getRawParameterValue(p + "phaseSkew");
        pPartnerA_      = apvts.getRawParameterValue(p + "partnerA_idx");
        pPartnerB_      = apvts.getRawParameterValue(p + "partnerB_idx");
        pPartnerC_      = apvts.getRawParameterValue(p + "partnerC_idx");
        pCouplingDepth_ = apvts.getRawParameterValue(p + "couplingDepth");
        pDnaTilt_       = apvts.getRawParameterValue(p + "dnaTilt");
        pMix_           = apvts.getRawParameterValue(p + "mix");
        pTopology_      = apvts.getRawParameterValue(p + "topology");
        pSyncMode_      = apvts.getRawParameterValue(p + "syncMode");
    }

private:
    static int clampSlot(float v) noexcept
    {
        const int n = static_cast<int>(v + 0.5f);
        return n < 0 ? 0 : (n > 3 ? 3 : n);
    }

    double sr_ = 0.0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> pumpDepthSmoothed_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed_;

    // Envelope-follower state per partner (one-pole, mono).
    float envA_ = 0.0f, envB_ = 0.0f, envC_ = 0.0f;
    // Master LFO phase shared across A/B/C; per-partner phase = phase + offset.
    float lfoPhase_ = 0.0f;
    // Chaotic-topology state: per-partner randomised phase offsets, reseeded
    // on every LFO wrap. LCG state is deterministic (reset via reset()).
    float chaoticOffsetB_ = 0.0f, chaoticOffsetC_ = 0.0f;
    juce::uint32 chaoticRng_ = 0x9E3779B9u;

    std::atomic<float>* pPumpDepth_     = nullptr;
    std::atomic<float>* pPumpRate_      = nullptr;
    std::atomic<float>* pAttack_        = nullptr;
    std::atomic<float>* pRelease_       = nullptr;
    std::atomic<float>* pPhaseSkew_     = nullptr;
    std::atomic<float>* pPartnerA_      = nullptr;
    std::atomic<float>* pPartnerB_      = nullptr;
    std::atomic<float>* pPartnerC_      = nullptr;
    std::atomic<float>* pCouplingDepth_ = nullptr;
    std::atomic<float>* pDnaTilt_       = nullptr;
    std::atomic<float>* pMix_           = nullptr;
    std::atomic<float>* pTopology_      = nullptr;
    std::atomic<float>* pSyncMode_      = nullptr;

    // Set once on message thread before audio starts; read lock-free on the
    // audio thread (single-writer / single-reader before-after pattern).
    const DNAModulationBus*  dnaBus_     = nullptr;
    const PartnerAudioBus*   partnerBus_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OtriumChain)
};

} // namespace xoceanus
