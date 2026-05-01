// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
// OperaAdapter.h — XOceanus adapter for opera::OperaEngine
//
// Engine: XOpera — Kuramoto-coupled additive-vocal synthesis
// Accent: Aria Gold #D4AF37 | Prefix: opera_ | Voices: 8
// Creature: The Humpback Whale (Mesopelagic/SOFAR)
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "OperaEngine.h"

#include <array>
#include <cstring>

// Forward-declare processor so setProcessorPtr() can store the pointer without
// pulling in the full XOceanusProcessor.h here (that would create a circular
// include because XOceanusProcessor.h includes OperaAdapter.h transitively).
class XOceanusProcessor;

namespace xoceanus
{

class OperaAdapter : public SynthEngine
{
public:
    OperaAdapter() = default;

    juce::String getEngineId() const override { return engine_.getEngineId(); }
    juce::Colour getAccentColour() const override { return engine_.getAccentColour(); }
    int getMaxVoices() const override { return engine_.getMaxVoices(); }
    int getActiveVoiceCount() const override { return engine_.getActiveVoiceCount(); }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        engine_.prepare(sampleRate, maxBlockSize);
        prepareSilenceGate(sampleRate, maxBlockSize, 500.0f);
    }

    void releaseResources() override { engine_.releaseResources(); }
    void reset() override { engine_.reset(); }

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;

        // Compute average voice velocity once per block for velocity-scaled routes.
        float avgVel = 0.0f;
        {
            int count = 0;
            for (const auto& metadata : midi)
            {
                auto msg = metadata.getMessage();
                if (msg.isNoteOn())
                {
                    avgVel += static_cast<float>(msg.getVelocity()) / 127.0f;
                    ++count;
                    wakeSilenceGate();
                }
            }
            if (count > 0)
                avgVel /= static_cast<float>(count);
        }

        if (isSilenceGateBypassed() && midi.isEmpty())
            return;

        // T6: Apply global mod-route offsets BEFORE delegating to the inner
        // OperaEngine.  The engine reads these via its own ModOffsets path;
        // we inject them directly into the engine's external-mod-offsets
        // API so they accumulate on top of LFO/MW/AT modulation.
        applyGlobalModRoutes(avgVel);

        engine_.renderBlock(buffer, midi, numSamples);
        analyzeForSilenceGate(buffer, numSamples);
    }

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        return engine_.getSampleForCoupling(channel, sampleIndex);
    }

    void applyCouplingInput(CouplingType type, float amount, const float* sourceBuffer, int numSamples) override
    {
        engine_.applyCouplingInput(static_cast<int>(type), amount, sourceBuffer, numSamples);
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override { engine_.attachParameters(apvts); }

    // #1257: Forward MPE manager to the inner OperaEngine so it can do per-voice
    // pitch-bend lookups. The base SynthEngine::mpeManager is also set by the
    // default impl (which OperaAdapter inherits), but OperaEngine is a plain class
    // that doesn't have access to it — hence the explicit delegation here.
    void setMPEManager(MPEManager* m) override
    {
        SynthEngine::setMPEManager(m); // keep base-class pointer in sync
        engine_.setMPEManager(m);
    }

    //-- T6: Global mod-route opt-in (Pattern B) --------------------------------
    //
    // setProcessorPtr() — called once from XOceanusProcessor::loadEngine() on the
    // message thread after attachParameters().  Stores the processor pointer so
    // cacheGlobalModRoutes() can call the public route accessors.
    //
    // cacheGlobalModRoutes() — scans the current snapshot for routes that target
    // any of Opera's 5 modulated parameters and stores the matching route indices
    // in globalModRouteIdx_[].  -1 means no active route for that target.
    // Called whenever the snapshot changes (on load + on route model flush).
    //
    // Target → index mapping (fixed):
    //   0 = opera_drama       (Kuramoto coupling strength / timbral intensity — D001)
    //   1 = opera_filterCutoff (filter brightness — D001: velocity→timbre)
    //   2 = opera_breath      (breath noise level — expressiveness via mod wheel)
    //   3 = opera_vibDepth    (vibrato depth — aftertouch expressiveness)
    //   4 = opera_ampR        (amp release — tail length extension via aftertouch)

    static constexpr int kOperaGlobalModTargets = 5;

    void setProcessorPtr(XOceanusProcessor* p) noexcept
    {
        processorPtr_ = p;
        cacheGlobalModRoutes();
    }

    void cacheGlobalModRoutes() noexcept; // implemented in OperaAdapter.cpp (needs full XOceanusProcessor type)

private:
    opera::OperaEngine engine_;

    // T6: Global mod-route opt-in state.
    // processorPtr_: set by setProcessorPtr() on the message thread; read-only on
    //   the audio thread after that.  Plain pointer — no atomic needed because
    //   assignment happens before the first renderBlock() call.
    XOceanusProcessor* processorPtr_ = nullptr;

    // Cached route indices for the 5 target params (kOperaGlobalModTargets).
    // -1 = no active global route for that target.
    // Written by cacheGlobalModRoutes() (message thread), read by renderBlock()
    // (audio thread).  One-block lag on route add/remove is safe.
    std::array<int, kOperaGlobalModTargets> globalModRouteIdx_{ -1, -1, -1, -1, -1 };

    // velocityScaled flag for each cached route slot.
    std::array<bool, kOperaGlobalModTargets> globalModVelScaled_{};

    // Pre-cached range span for each target param so applyGlobalModRoutes() can
    // scale the normalised accumulator to param units without calling juce:: methods.
    std::array<float, kOperaGlobalModTargets> globalModRangeSpan_{};

    // Raw pointer to the processor's routeModAccum_ array.  Set by setProcessorPtr()
    // alongside processorPtr_.  Stored separately so applyGlobalModRoutes() can read
    // accumulators without needing the full XOceanusProcessor type (forward-decl safe).
    const float* modAccumPtr_ = nullptr;

    // Param IDs for the 5 modulated targets (index-matched to globalModRouteIdx_).
    static constexpr const char* kGlobalModTargetIds[kOperaGlobalModTargets] = {
        "opera_drama",        // 0: Kuramoto coupling / timbral intensity (D001)
        "opera_filterCutoff", // 1: filter brightness (D001 velocity→timbre)
        "opera_breath",       // 2: breath noise level (expressiveness)
        "opera_vibDepth",     // 3: vibrato depth (aftertouch expressiveness)
        "opera_ampR",         // 4: amp release (tail length)
    };

    // T6: Apply accumulated global mod-route offsets to the 5 target params via
    // the engine's external mod-offset API.  Implemented inline here (all data
    // comes from cached arrays — no full processor type needed).
    // Must be called BEFORE engine_.renderBlock() each block.
    void applyGlobalModRoutes(float avgVel) noexcept
    {
        if (modAccumPtr_ == nullptr)
            return;

        // Target 0: opera_drama (0..1)
        // High drama = stronger Kuramoto coupling → richer harmonic blend.
        {
            int ri = globalModRouteIdx_[0];
            if (ri >= 0)
            {
                float raw   = modAccumPtr_[static_cast<size_t>(ri)];
                float depth = globalModVelScaled_[0] ? raw * avgVel : raw;
                float span  = globalModRangeSpan_[0]; // 1.0f
                engine_.setExternalModOffset_Drama(depth * span);
            }
        }

        // Target 1: opera_filterCutoff (20..20000 Hz) — D001 compliance.
        // Velocity route: high velocity → brighter filter.
        {
            int ri = globalModRouteIdx_[1];
            if (ri >= 0)
            {
                float raw   = modAccumPtr_[static_cast<size_t>(ri)];
                float depth = globalModVelScaled_[1] ? raw * avgVel : raw;
                float span  = globalModRangeSpan_[1]; // 19980.0f
                engine_.setExternalModOffset_FilterCutoff(depth * span);
            }
        }

        // Target 2: opera_breath (0..1)
        // Mod-wheel→breath brings in breath noise for organic, live feel.
        {
            int ri = globalModRouteIdx_[2];
            if (ri >= 0)
            {
                float raw   = modAccumPtr_[static_cast<size_t>(ri)];
                float depth = globalModVelScaled_[2] ? raw * avgVel : raw;
                float span  = globalModRangeSpan_[2]; // 1.0f
                engine_.setExternalModOffset_Breath(depth * span);
            }
        }

        // Target 3: opera_vibDepth (0..1)
        // Aftertouch→vibrato depth for expressive pitch modulation.
        {
            int ri = globalModRouteIdx_[3];
            if (ri >= 0)
            {
                float raw   = modAccumPtr_[static_cast<size_t>(ri)];
                float depth = globalModVelScaled_[3] ? raw * avgVel : raw;
                float span  = globalModRangeSpan_[3]; // 1.0f
                engine_.setExternalModOffset_VibDepth(depth * span);
            }
        }

        // Target 4: opera_ampR (0..2 s normalised 0..1 in snap_)
        // Aftertouch extends release tails for sustained, evolving chords.
        {
            int ri = globalModRouteIdx_[4];
            if (ri >= 0)
            {
                float raw   = modAccumPtr_[static_cast<size_t>(ri)];
                float depth = globalModVelScaled_[4] ? raw * avgVel : raw;
                float span  = globalModRangeSpan_[4]; // 1.0f (snap_.ampR is 0..1)
                engine_.setExternalModOffset_AmpR(depth * span);
            }
        }
    }
};

// T6: cacheGlobalModRoutes() is implemented in OperaAdapter.cpp where
// XOceanusProcessor.h can be included for the full type without a circular
// dependency (OperaAdapter.h only forward-declares XOceanusProcessor).

} // namespace xoceanus
