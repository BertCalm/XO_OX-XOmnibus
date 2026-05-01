// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
// OxbowAdapter.h — XOceanus mod-matrix opt-in adapter for OxbowEngine
//
// Engine: Oxbow (Entangled Reverb) — Engine #43
// Concept: Chiasmus FDN reverb. Sound enters as rushing water; the Oxbow cuts
//          the current, leaving a suspended pool of resonance that slowly erases
//          itself. Golden standing waves remain.
// Accent: Oxbow Teal #1A6B5A | Prefix: oxb_ | Voices: 1 (monophonic reverb)
//
// T6: Pattern B mod-matrix opt-in.
// Wraps OxbowEngine and adds the global-mod-route consumption path so the mod
// matrix can target Oxbow's 5 most performance-expressive parameters without any
// per-sample strcmp on the audio thread.
//
// Silence gate: managed by OxbowEngine internally. This adapter calls
// engine_.prepareSilenceGate() in prepare() so the engine's gate is ready, and
// delegates all SRO gate queries to the engine via analyzeForSilenceGate() at
// the end of renderBlock(). The adapter's own gate (inherited from SynthEngine)
// is kept in sync so the processor's isSilenceGateBypassed() check works correctly.
//
// Targets (5):
//   0 = oxb_decay       — Decay Time (0.1..60s). Velocity → longer tail.
//   1 = oxb_damping     — Damping Hz (200..16000). D001: velocity → brighter timbre.
//   2 = oxb_entangle    — Entanglement (0..1). Velocity → denser L/R cross-coupling.
//   3 = oxb_dryWet      — Dry/Wet (0..1). Velocity → deeper immersion.
//   4 = oxb_size        — Space Size (0..1). Velocity → larger virtual room.
//
// Threading: setProcessorPtr() called once on message thread from loadEngine();
// cacheGlobalModRoutes() refreshed from flushModRoutesSnapshot(). Audio thread
// reads cached indices read-only with one-block lag tolerance.
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "OxbowEngine.h"
#include <array>
#include <cstring>

namespace xoceanus
{

// T6: Forward-declare so OxbowAdapter can cache a processor pointer for the
// global mod-route path. Full type is included in OxbowAdapter.cpp only.
class XOceanusProcessor;

// Number of global mod-route slots cached at load time.
static constexpr int kOxbowGlobalModTargets = 5;

class OxbowAdapter : public SynthEngine
{
public:
    OxbowAdapter() = default;

    //-- Identity ---------------------------------------------------------------

    juce::String getEngineId()    const override { return engine_.getEngineId(); }
    juce::Colour getAccentColour() const override { return engine_.getAccentColour(); }
    int          getMaxVoices()   const override { return engine_.getMaxVoices(); }

    //-- Lifecycle ---------------------------------------------------------------

    void prepare(double sampleRate, int maxBlockSize) override
    {
        engine_.prepare(sampleRate, maxBlockSize);
        // Prepare the adapter's own silence gate so isSilenceGateBypassed() returns
        // a valid result when called by the processor. The engine's internal gate is
        // also set up here (engine_.prepareSilenceGate) so OxbowEngine's renderBlock()
        // internal bypass check works correctly.
        // Note: the processor calls prepareSilenceGate() separately after prepare() —
        // that second call sets the hold time from silenceGateHoldMs(). Both calls are
        // needed: this one ensures the engine's gate is prepared before the first
        // renderBlock(), the processor's call overrides hold time on both gates.
        const float holdMs = 500.0f; // Chiasmus FDN + pre-delay reverb tail
        prepareSilenceGate(sampleRate, maxBlockSize, holdMs);
        engine_.prepareSilenceGate(sampleRate, maxBlockSize, holdMs);
    }

    void releaseResources() override { engine_.releaseResources(); }

    void reset() override { engine_.reset(); }

    //-- Audio ------------------------------------------------------------------

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;

        // Wake both gates on note-on so neither gate silences the reverb tail.
        for (const auto& metadata : midi)
        {
            if (metadata.getMessage().isNoteOn())
            {
                wakeSilenceGate();          // adapter's gate
                engine_.wakeSilenceGate();  // engine's internal gate
                break;
            }
        }

        // Zero-idle bypass: check adapter's gate (kept in sync via analyzeForSilenceGate).
        if (isSilenceGateBypassed() && midi.isEmpty())
            return;

        // T6: Apply accumulated global mod-route offsets to the 5 target param
        // atomics BEFORE the engine reads them in its renderBlock().
        // avgVelocity = 1.0f (monophonic reverb; OxbowEngine stores currentVelocity
        // internally but does not expose it). Velocity-scaled routes therefore
        // express at full depth — use non-velocity-scaled routes for fixed offsets.
        applyGlobalModRoutes(1.0f);

        // Delegate all DSP, MIDI parsing, and internal gate management to engine.
        engine_.renderBlock(buffer, midi, numSamples);

        // Keep the adapter's gate in sync with the engine's rendered output.
        analyzeForSilenceGate(buffer, numSamples);
    }

    //-- Coupling ---------------------------------------------------------------

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        return engine_.getSampleForCoupling(channel, sampleIndex);
    }

    void applyCouplingInput(CouplingType type, float amount, const float* sourceBuffer, int numSamples) override
    {
        engine_.applyCouplingInput(type, amount, sourceBuffer, numSamples);
    }

    //-- Parameters -------------------------------------------------------------

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        OxbowEngine::addParameters(params);
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        engine_.attachParameters(apvts);

        // Cache pointers for the 5 mod-target params so applyGlobalModRoutes()
        // can add offsets to the live atomic values without string lookups.
        pDecaySnap_    = apvts.getRawParameterValue("oxb_decay");
        pDampingSnap_  = apvts.getRawParameterValue("oxb_damping");
        pEntangleSnap_ = apvts.getRawParameterValue("oxb_entangle");
        pDryWetSnap_   = apvts.getRawParameterValue("oxb_dryWet");
        pSizeSnap_     = apvts.getRawParameterValue("oxb_size");
    }

    //-- T6: Global mod-route opt-in -------------------------------------------
    //
    // setProcessorPtr() — called once from XOceanusProcessor::loadEngine() on the
    // message thread after attachParameters(). Stores the processor pointer and
    // runs an initial cacheGlobalModRoutes() scan so cached indices are ready
    // before the first renderBlock().
    //
    // cacheGlobalModRoutes() — scans the current global-mod-route snapshot for
    // routes targeting any of Oxbow's 5 modulated parameters and stores matching
    // route indices in globalModRouteIdx_[]. -1 = no active route for that target.
    // Called whenever the snapshot changes (on load + on route model flush).

    void setProcessorPtr(XOceanusProcessor* proc) noexcept
    {
        processorPtr_ = proc;
        cacheGlobalModRoutes();
    }

    void cacheGlobalModRoutes() noexcept; // implemented in OxbowAdapter.cpp

private:
    //-- T6: Apply accumulated global mod offsets to engine param snaps ---------
    //
    // Called at the top of renderBlock() after macro/coupling, before DSP.
    // Adds range_span * accum * (velScaled ? avgVel : 1.0f) to each target's
    // live parameter value. OxbowEngine reads these atomics in its renderBlock().
    // juce::jlimit used to clamp the result within each parameter's valid range.

    void applyGlobalModRoutes(float avgVelocity) noexcept
    {
        if (modAccumPtr_ == nullptr)
            return;

        // Target 0: oxb_decay (0.1..60.0 s)
        {
            int ri = globalModRouteIdx_[0];
            if (ri >= 0 && pDecaySnap_ != nullptr)
            {
                float raw   = modAccumPtr_[static_cast<size_t>(ri)];
                float depth = globalModVelScaled_[0] ? raw * avgVelocity : raw;
                float span  = globalModRangeSpan_[0]; // 59.9f
                float cur   = pDecaySnap_->load(std::memory_order_relaxed);
                pDecaySnap_->store(juce::jlimit(0.1f, 60.0f, cur + depth * span),
                                   std::memory_order_relaxed);
            }
        }

        // Target 1: oxb_damping (200..16000 Hz) — D001: velocity → brighter timbre
        {
            int ri = globalModRouteIdx_[1];
            if (ri >= 0 && pDampingSnap_ != nullptr)
            {
                float raw   = modAccumPtr_[static_cast<size_t>(ri)];
                float depth = globalModVelScaled_[1] ? raw * avgVelocity : raw;
                float span  = globalModRangeSpan_[1]; // 15800.0f
                float cur   = pDampingSnap_->load(std::memory_order_relaxed);
                pDampingSnap_->store(juce::jlimit(200.0f, 16000.0f, cur + depth * span),
                                     std::memory_order_relaxed);
            }
        }

        // Target 2: oxb_entangle (0..1)
        {
            int ri = globalModRouteIdx_[2];
            if (ri >= 0 && pEntangleSnap_ != nullptr)
            {
                float raw   = modAccumPtr_[static_cast<size_t>(ri)];
                float depth = globalModVelScaled_[2] ? raw * avgVelocity : raw;
                float span  = globalModRangeSpan_[2]; // 1.0f
                float cur   = pEntangleSnap_->load(std::memory_order_relaxed);
                pEntangleSnap_->store(juce::jlimit(0.0f, 1.0f, cur + depth * span),
                                      std::memory_order_relaxed);
            }
        }

        // Target 3: oxb_dryWet (0..1)
        {
            int ri = globalModRouteIdx_[3];
            if (ri >= 0 && pDryWetSnap_ != nullptr)
            {
                float raw   = modAccumPtr_[static_cast<size_t>(ri)];
                float depth = globalModVelScaled_[3] ? raw * avgVelocity : raw;
                float span  = globalModRangeSpan_[3]; // 1.0f
                float cur   = pDryWetSnap_->load(std::memory_order_relaxed);
                pDryWetSnap_->store(juce::jlimit(0.0f, 1.0f, cur + depth * span),
                                    std::memory_order_relaxed);
            }
        }

        // Target 4: oxb_size (0..1)
        {
            int ri = globalModRouteIdx_[4];
            if (ri >= 0 && pSizeSnap_ != nullptr)
            {
                float raw   = modAccumPtr_[static_cast<size_t>(ri)];
                float depth = globalModVelScaled_[4] ? raw * avgVelocity : raw;
                float span  = globalModRangeSpan_[4]; // 1.0f
                float cur   = pSizeSnap_->load(std::memory_order_relaxed);
                pSizeSnap_->store(juce::jlimit(0.0f, 1.0f, cur + depth * span),
                                  std::memory_order_relaxed);
            }
        }
    }

    //-- Members ----------------------------------------------------------------

    OxbowEngine engine_;

    // Live parameter pointers for the 5 mod targets (set in attachParameters).
    std::atomic<float>* pDecaySnap_    = nullptr;
    std::atomic<float>* pDampingSnap_  = nullptr;
    std::atomic<float>* pEntangleSnap_ = nullptr;
    std::atomic<float>* pDryWetSnap_   = nullptr;
    std::atomic<float>* pSizeSnap_     = nullptr;

    // T6: Global mod-route opt-in state
    // processorPtr_: set by setProcessorPtr() on the message thread; read-only
    //   on the audio thread after that. Plain pointer — no atomic needed because
    //   assignment happens before the first renderBlock() call.
    XOceanusProcessor* processorPtr_ = nullptr;

    // Cached route indices for the 5 target params.
    // -1 = no active global route for that target.
    // Written by cacheGlobalModRoutes() (message thread), read by renderBlock()
    // (audio thread). One-block lag is safe.
    std::array<int,   kOxbowGlobalModTargets> globalModRouteIdx_{-1, -1, -1, -1, -1};
    std::array<bool,  kOxbowGlobalModTargets> globalModVelScaled_{};
    std::array<float, kOxbowGlobalModTargets> globalModRangeSpan_{};

    // Raw pointer to the processor's routeModAccum_ array.
    const float* modAccumPtr_ = nullptr;

    // Param IDs for the 5 modulated targets (index-matched to globalModRouteIdx_).
    static constexpr const char* kGlobalModTargetIds[kOxbowGlobalModTargets] = {
        "oxb_decay",
        "oxb_damping",
        "oxb_entangle",
        "oxb_dryWet",
        "oxb_size",
    };
};

// T6: cacheGlobalModRoutes() is implemented in OxbowAdapter.cpp where
// XOceanusProcessor.h can be included for the full type without a circular
// dependency (OxbowAdapter.h only forward-declares XOceanusProcessor).

} // namespace xoceanus
