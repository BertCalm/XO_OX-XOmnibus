// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cstring>
#include "../DSP/Effects/AquaticFXSuite.h"
#include "../DSP/Effects/MathFXChain.h"
#include "../DSP/Effects/BoutiqueFXChain.h"
#include "../DSP/Effects/fXOnslaught.h"
#include "../DSP/Effects/fXObscura.h"
#include "../DSP/Effects/fXOratory.h"
#include "../DSP/Effects/OnrushChain.h"
#include "../DSP/Effects/OmnistereoChain.h"
#include "../DSP/Effects/ObliterateChain.h"
#include "../DSP/Effects/ObscurityChain.h"
// Wave 2 — 20 new Epic chains
#include "../DSP/Effects/OublietteChain.h"
#include "../DSP/Effects/OsmiumChain.h"
#include "../DSP/Effects/OrogenChain.h"
#include "../DSP/Effects/OculusChain.h"
#include "../DSP/Effects/OutageChain.h"
#include "../DSP/Effects/OverrideChain.h"
#include "../DSP/Effects/OcclusionChain.h"
#include "../DSP/Effects/ObdurateChain.h"
#include "../DSP/Effects/OrisonChain.h"
#include "../DSP/Effects/OvershootChain.h"
#include "../DSP/Effects/ObverseChain.h"
#include "../DSP/Effects/OxymoronChain.h"
#include "../DSP/Effects/OrnateChain.h"
#include "../DSP/Effects/OrationChain.h"
#include "../DSP/Effects/OffcutChain.h"
#include "../DSP/Effects/OmenChain.h"
#include "../DSP/Effects/OpusChain.h"
#include "../DSP/Effects/OutlawChain.h"
#include "../DSP/Effects/OutbreakChain.h"
#include "../DSP/Effects/OrreryChain.h"
// FX Pack 1 — Sidechain Creative
#include "../DSP/Effects/OtriumChain.h"
#include "../DSP/Effects/OblateChain.h"
#include "../DSP/Effects/OligoChain.h"

namespace xoceanus
{

//==============================================================================
// EpicChainSlotController — 3 fully-assignable FX chain slots.
//
// Replaces the fixed AquaticFXSuite / MathFXChain / BoutiqueFXChain /
// Singularity FX positions in MasterFXChain with a unified assignable system.
//
// Each slot holds all 10 chain instances (pre-allocated in prepare()).
// Only the active chain per slot runs processBlock() per audio block.
// Chain swaps trigger a 50ms linear crossfade.
//
// Parameter convention: slot{N}_{chainPrefix}_{paramName}
//   Control:  slot1_chain, slot1_mix, slot1_bypass  (× 3 slots = 9 params)
//   Chains:   slot1_aqua_fathom_mix, slot2_onr_swellThresh, etc.
//
// See Docs/specs/2026-04-05-epic-chains-fx-design.md for full spec.
//==============================================================================
class EpicChainSlotController
{
public:
    static constexpr int kNumSlots   = 3;
    static constexpr int kNumChains  = 33;
    static constexpr int kMaxChainID = 33;  // == static_cast<int>(Oligo)

    enum ChainID : int
    {
        Off        = 0,
        Aquatic    = 1,
        Math       = 2,
        Boutique   = 3,
        Onslaught  = 4,
        Obscura    = 5,
        Oratory    = 6,
        Onrush     = 7,
        Omnistereo = 8,
        Obliterate = 9,
        Obscurity  = 10,
        // Wave 2 — Monsterous
        Oubliette  = 11,
        Osmium     = 12,
        Orogen     = 13,
        Oculus     = 14,
        // Wave 2 — Sunken Treasure
        Outage     = 15,
        Override   = 16,
        Occlusion  = 17,
        Obdurate   = 18,
        // Wave 2 — Anomalous
        Orison     = 19,
        Overshoot  = 20,
        Obverse    = 21,
        Oxymoron   = 22,
        // Wave 2 — AHA
        Ornate     = 23,
        Oration    = 24,
        Offcut     = 25,
        Omen       = 26,
        // Wave 2 — Alt Universe
        Opus       = 27,
        Outlaw     = 28,
        Outbreak   = 29,
        Orrery     = 30,
        // FX Pack 1 — Sidechain Creative (scaffold; Pack 1 ships full DSP)
        Otrium     = 31,
        Oblate     = 32,
        Oligo      = 33
    };

    EpicChainSlotController() = default;

    //--------------------------------------------------------------------------
    /// Prepare all slots and all chain instances.
    /// Must be called before processBlock(). No heap allocation in processBlock.
    void prepare(double sampleRate, int maxBlockSize);

    //--------------------------------------------------------------------------
    /// Process 3 slots in series. buffer must be stereo (2 channels).
    /// Reads slot{N}_chain, slot{N}_mix, slot{N}_bypass from cached param pointers.
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples,
                      double bpm = 0.0, double ppqPosition = -1.0);

    //--------------------------------------------------------------------------
    /// Register all slot control parameters + all chain parameters (all 3 slots).
    /// Called from XOceanusProcessor::createParameterLayout().
    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout);

    //--------------------------------------------------------------------------
    /// Cache all parameter pointers once after prepareToPlay().
    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts);

    //--------------------------------------------------------------------------
    /// Reset all chain states (called from MasterFXChain::reset()).
    void reset();

    //--------------------------------------------------------------------------
    /// Inject the DNAModulationBus pointer into every Pack 1 chain instance
    /// (Otrium, Oblate, Oligo across all 3 slots). Call once on the message
    /// thread after construction, before audio starts. The pointer is read
    /// lock-free on the audio thread by the chains' DSP.
    void setDNABus(const DNAModulationBus* bus) noexcept;

private:
    double sr_         = 0.0;  // Sentinel: must be set by prepare() before use
    int    blockSize_  = 512;

    //==========================================================================
    // FXSlot — one assignable slot holding all 10 chain instances.
    //==========================================================================
    struct FXSlot
    {
        ChainID activeChain   = Off;
        ChainID pendingChain  = Off;
        float crossfadeProgress = 1.0f; // 1.0 = stable; 0.0 = crossfade just started

        // Crossfade scratch buffer (allocated in prepare)
        juce::AudioBuffer<float> crossfadeBuf;

        // All 30 chain instances (pre-allocated, never on audio-thread heap)
        AquaticFXSuite    aquatic;
        MathFXChain       math;
        BoutiqueFXChain   boutique;
        fXOnslaught       onslaught;
        fXObscura         obscura;
        fXOratory         oratory;

        // Epic chains (Tasks 6-9)
        OnrushChain     onrush;
        OmnistereoChain omnistereo;
        ObliterateChain obliterate;
        ObscurityChain  obscurity;

        // Wave 2 — Monsterous
        OublietteChain  oubliette;
        OsmiumChain     osmium;
        OrogenChain     orogen;
        OculusChain     oculus;
        // Wave 2 — Sunken Treasure
        OutageChain     outage;
        OverrideChain   override_;   // avoids collision with C++ keyword
        OcclusionChain  occlusion;
        ObdurateChain   obdurate;
        // Wave 2 — Anomalous
        OrisonChain     orison;
        OvershootChain  overshoot;
        ObverseChain    obverse;
        OxymoronChain   oxymoron;
        // Wave 2 — AHA
        OrnateChain     ornate;
        OrationChain    oration;
        OffcutChain     offcut;
        OmenChain       omen;
        // Wave 2 — Alt Universe
        OpusChain       opus;
        OutlawChain     outlaw;
        OutbreakChain   outbreak;
        OrreryChain     orrery;
        // FX Pack 1 — Sidechain Creative
        OtriumChain     otrium;
        OblateChain     oblate;
        OligoChain      oligo;

        // Mono scratch buffer for Mono-In chains (19 Wave 2 + Onrush, Obliterate, Obscurity)
        std::vector<float> monoScratch;

        // ---- Cached slot control param pointers (added per slot in cacheParameterPointers) ----
        std::atomic<float>* pChain  = nullptr;
        std::atomic<float>* pMix    = nullptr;
        std::atomic<float>* pBypass = nullptr;
    };

    std::array<FXSlot, kNumSlots> slots_;

    //--------------------------------------------------------------------------
    // Helper: dispatch processBlock to the correct chain instance.
    // Task 3 fills in the chain cases; for now all cases are passthrough.
    void dispatchChain(FXSlot& slot, ChainID chain,
                       float* L, float* R, int numSamples,
                       double bpm, double ppqPosition);

    //--------------------------------------------------------------------------
    // Helper: apply per-slot wet/dry mix.
    static void applyMix(float* dryL, float* dryR,
                         float* wetL, float* wetR,
                         int numSamples, float mix);

    //--------------------------------------------------------------------------
    // Compute crossfade increment per sample (50ms ramp).
    float crossfadeIncrement() const
    {
        // Clamp to avoid division by zero
        float xfSamples = static_cast<float>(sr_) * 0.05f; // 50ms
        return 1.0f / std::max(xfSamples, 1.0f);
    }
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void EpicChainSlotController::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;

    for (auto& slot : slots_)
    {
        slot.crossfadeBuf.setSize(2, maxBlockSize, false, true, true);
        slot.crossfadeProgress = 1.0f;
        slot.activeChain       = Off;
        slot.pendingChain      = Off;

        slot.aquatic.prepare(sampleRate, maxBlockSize);
        slot.math.prepare(sampleRate);
        slot.boutique.prepare(sampleRate);
        slot.onslaught.prepare(sampleRate, maxBlockSize);
        slot.obscura.prepare(sampleRate);
        slot.oratory.prepare(sampleRate, maxBlockSize);
        // Epic chains
        slot.onrush.prepare(sampleRate, maxBlockSize);
        slot.omnistereo.prepare(sampleRate, maxBlockSize);
        slot.obliterate.prepare(sampleRate, maxBlockSize);
        slot.obscurity.prepare(sampleRate, maxBlockSize);
        // Wave 2 chains
        slot.oubliette.prepare(sampleRate, maxBlockSize);
        slot.osmium.prepare(sampleRate, maxBlockSize);
        slot.orogen.prepare(sampleRate, maxBlockSize);
        slot.oculus.prepare(sampleRate, maxBlockSize);
        slot.outage.prepare(sampleRate, maxBlockSize);
        slot.override_.prepare(sampleRate, maxBlockSize);
        slot.occlusion.prepare(sampleRate, maxBlockSize);
        slot.obdurate.prepare(sampleRate, maxBlockSize);
        slot.orison.prepare(sampleRate, maxBlockSize);
        slot.overshoot.prepare(sampleRate, maxBlockSize);
        slot.obverse.prepare(sampleRate, maxBlockSize);
        slot.oxymoron.prepare(sampleRate, maxBlockSize);
        slot.ornate.prepare(sampleRate, maxBlockSize);
        slot.oration.prepare(sampleRate, maxBlockSize);
        slot.offcut.prepare(sampleRate, maxBlockSize);
        slot.omen.prepare(sampleRate, maxBlockSize);
        slot.opus.prepare(sampleRate, maxBlockSize);
        slot.outlaw.prepare(sampleRate, maxBlockSize);
        slot.outbreak.prepare(sampleRate, maxBlockSize);
        slot.orrery.prepare(sampleRate, maxBlockSize);
        slot.otrium.prepare(sampleRate, maxBlockSize); // FX Pack 1 scaffold
        slot.oblate.prepare(sampleRate, maxBlockSize); // FX Pack 1 scaffold
        slot.oligo.prepare(sampleRate, maxBlockSize);  // FX Pack 1 scaffold
        // Mono scratch buffer for mono-in epic chains
        slot.monoScratch.assign(maxBlockSize, 0.0f);
    }
}

inline void EpicChainSlotController::reset()
{
    for (auto& slot : slots_)
    {
        slot.crossfadeProgress = 1.0f;
        slot.aquatic.reset();
        slot.math.reset();
        slot.boutique.reset();
        slot.onslaught.reset();
        slot.obscura.reset();
        slot.oratory.reset();
        // Epic chains
        slot.onrush.reset();
        slot.omnistereo.reset();
        slot.obliterate.reset();
        slot.obscurity.reset();
        // Wave 2 chains
        slot.oubliette.reset();
        slot.osmium.reset();
        slot.orogen.reset();
        slot.oculus.reset();
        slot.outage.reset();
        slot.override_.reset();
        slot.occlusion.reset();
        slot.obdurate.reset();
        slot.orison.reset();
        slot.overshoot.reset();
        slot.obverse.reset();
        slot.oxymoron.reset();
        slot.ornate.reset();
        slot.oration.reset();
        slot.offcut.reset();
        slot.omen.reset();
        slot.opus.reset();
        slot.outlaw.reset();
        slot.outbreak.reset();
        slot.orrery.reset();
        slot.otrium.reset(); // FX Pack 1 scaffold
        slot.oblate.reset(); // FX Pack 1 scaffold
        slot.oligo.reset();  // FX Pack 1 scaffold
    }
}

inline void EpicChainSlotController::setDNABus(const DNAModulationBus* bus) noexcept
{
    for (auto& slot : slots_)
    {
        slot.otrium.setDNABus(bus);
        slot.oblate.setDNABus(bus);
        slot.oligo.setDNABus(bus);
    }
}

inline void EpicChainSlotController::processBlock(juce::AudioBuffer<float>& buffer,
                                                   int numSamples,
                                                   double bpm,
                                                   double ppqPosition)
{
    jassert(sr_ > 0.0);  // sr=0.0 sentinel: prepare() must be called before processBlock()
    if (buffer.getNumChannels() < 2 || numSamples <= 0)
        return;

    const float xfInc = crossfadeIncrement();

    for (int slotIdx = 0; slotIdx < kNumSlots; ++slotIdx)
    {
        FXSlot& slot = slots_[slotIdx];

        // Read control params
        const float chainVal  = slot.pChain  ? slot.pChain->load(std::memory_order_relaxed)  : 0.0f;
        const float slotMix   = slot.pMix    ? slot.pMix->load(std::memory_order_relaxed)    : 1.0f;
        const float bypass    = slot.pBypass ? slot.pBypass->load(std::memory_order_relaxed) : 0.0f;

        if (bypass > 0.5f)
            continue; // slot bypassed — passthrough

        // Detect chain change
        ChainID requestedChain = static_cast<ChainID>(static_cast<int>(chainVal + 0.5f));
        requestedChain = static_cast<ChainID>(
            juce::jlimit(static_cast<int>(Off), kMaxChainID,
                         static_cast<int>(requestedChain)));

        if (requestedChain != slot.pendingChain && slot.crossfadeProgress >= 1.0f)
        {
            // New chain requested — initiate crossfade
            slot.pendingChain      = requestedChain;
            slot.crossfadeProgress = 0.0f;
        }

        if (slot.activeChain == Off && slot.pendingChain == Off)
            continue; // both Off — zero-cost passthrough

        float* L = buffer.getWritePointer(0);
        float* R = buffer.getWritePointer(1);

        if (slot.crossfadeProgress >= 1.0f)
        {
            // ---- Stable: process active chain ----
            if (slot.activeChain == Off)
                continue;

            // Save dry copy for wet/dry mix
            if (slotMix < 0.999f)
            {
                const int safeSamples = std::min(numSamples, slot.crossfadeBuf.getNumSamples());
                slot.crossfadeBuf.copyFrom(0, 0, buffer, 0, 0, safeSamples);
                slot.crossfadeBuf.copyFrom(1, 0, buffer, 1, 0, safeSamples);
            }

            dispatchChain(slot, slot.activeChain, L, R, numSamples, bpm, ppqPosition);

            if (slotMix < 0.999f)
            {
                applyMix(slot.crossfadeBuf.getWritePointer(0),
                         slot.crossfadeBuf.getWritePointer(1),
                         L, R, numSamples, slotMix);
            }
        }
        else
        {
            // ---- Crossfade active ----
            // outgoing = activeChain, incoming = pendingChain
            // crossfadeBuf = outgoing wet result
            // buffer = incoming wet result

            const int safeSamples = std::min(numSamples, slot.crossfadeBuf.getNumSamples());

            // Copy input into crossfadeBuf for outgoing chain
            slot.crossfadeBuf.copyFrom(0, 0, buffer, 0, 0, safeSamples);
            slot.crossfadeBuf.copyFrom(1, 0, buffer, 1, 0, safeSamples);

            // Outgoing chain processes crossfadeBuf
            if (slot.activeChain != Off)
                dispatchChain(slot, slot.activeChain,
                              slot.crossfadeBuf.getWritePointer(0),
                              slot.crossfadeBuf.getWritePointer(1),
                              safeSamples, bpm, ppqPosition);

            // Incoming chain processes buffer
            if (slot.pendingChain != Off)
                dispatchChain(slot, slot.pendingChain, L, R, safeSamples, bpm, ppqPosition);
            // else buffer stays as dry input (crossfading out to dry)

            // Equal-power crossfade prevents -3dB energy dip at midpoint (fix from QDD audit).
            // Linear law dips ~3dB at 50% progress — audible on sustained pads.
            // sqrt() is called once per sample only during the 50ms transition window
            // (~2400 samples at 48kHz), so the cost is negligible vs. a permanent hot path.
            float* xfL = slot.crossfadeBuf.getWritePointer(0);
            float* xfR = slot.crossfadeBuf.getWritePointer(1);
            float progress = slot.crossfadeProgress;
            for (int i = 0; i < safeSamples; ++i)
            {
                const float gainNew = std::sqrt(progress);
                const float gainOld = std::sqrt(1.0f - progress);
                L[i] = L[i] * gainNew + xfL[i] * gainOld;
                R[i] = R[i] * gainNew + xfR[i] * gainOld;
                progress = std::min(1.0f, progress + xfInc);
            }
            slot.crossfadeProgress = progress;

            if (slot.crossfadeProgress >= 1.0f)
            {
                slot.activeChain      = slot.pendingChain;
                slot.crossfadeProgress = 1.0f;
            }
        }
    }
}

inline void EpicChainSlotController::dispatchChain(FXSlot& slot, ChainID chain,
                                                    float* L, float* R,
                                                    int numSamples,
                                                    double bpm,
                                                    double ppqPosition)
{
    jassert(numSamples <= static_cast<int>(slot.monoScratch.size()));

    switch (chain)
    {
        case Off:
            break; // passthrough

        case Aquatic:
            slot.aquatic.processBlock(L, R, numSamples, bpm);
            break;

        case Math:
            slot.math.processBlockFromSlot(L, R, numSamples);
            break;

        case Boutique:
            slot.boutique.processBlockFromSlot(L, R, numSamples);
            break;

        case Onslaught:
            slot.onslaught.processBlockFromSlot(L, R, numSamples);
            break;

        case Obscura:
            slot.obscura.processBlockFromSlot(L, R, numSamples);
            break;

        case Oratory:
            slot.oratory.processBlockFromSlot(L, R, numSamples);
            break;

        // Epic chains — Mono-In chains mix L+R to mono first, then write stereo out
        case Onrush:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.onrush.processBlock(slot.monoScratch.data(), L, R, numSamples, bpm, ppqPosition);
            break;
        }

        case Omnistereo:
            slot.omnistereo.processBlock(L, R, numSamples);
            break;

        case Obliterate:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.obliterate.processBlock(slot.monoScratch.data(), L, R, numSamples);
            break;
        }

        case Obscurity:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.obscurity.processBlock(slot.monoScratch.data(), L, R, numSamples);
            break;
        }

        // ---- Wave 2 — Monsterous (Mono-In chains) ----
        case Oubliette:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.oubliette.processBlock(slot.monoScratch.data(), L, R, numSamples, bpm, ppqPosition);
            break;
        }

        case Osmium:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.osmium.processBlock(slot.monoScratch.data(), L, R, numSamples, bpm, ppqPosition);
            break;
        }

        case Orogen:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.orogen.processBlock(slot.monoScratch.data(), L, R, numSamples, bpm, ppqPosition);
            break;
        }

        case Oculus:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.oculus.processBlock(slot.monoScratch.data(), L, R, numSamples, bpm, ppqPosition);
            break;
        }

        // ---- Wave 2 — Sunken Treasure (Mono-In chains) ----
        case Outage:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.outage.processBlock(slot.monoScratch.data(), L, R, numSamples, bpm, ppqPosition);
            break;
        }

        case Override:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.override_.processBlock(slot.monoScratch.data(), L, R, numSamples, bpm, ppqPosition);
            break;
        }

        case Occlusion:
            // Stereo-In, Stereo-Out — pass L/R directly (in-place safe per header)
            slot.occlusion.processBlock(L, R, L, R, numSamples, bpm, ppqPosition);
            break;

        case Obdurate:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.obdurate.processBlock(slot.monoScratch.data(), L, R, numSamples, bpm, ppqPosition);
            break;
        }

        // ---- Wave 2 — Anomalous (Mono-In chains) ----
        case Orison:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.orison.processBlock(slot.monoScratch.data(), L, R, numSamples);
            break;
        }

        case Overshoot:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.overshoot.processBlock(slot.monoScratch.data(), L, R, numSamples);
            break;
        }

        case Obverse:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.obverse.processBlock(slot.monoScratch.data(), L, R, numSamples);
            break;
        }

        case Oxymoron:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.oxymoron.processBlock(slot.monoScratch.data(), L, R, numSamples);
            break;
        }

        // ---- Wave 2 — AHA (Mono-In chains) ----
        case Ornate:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.ornate.processBlock(slot.monoScratch.data(), L, R, numSamples, bpm, ppqPosition);
            break;
        }

        case Oration:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.oration.processBlock(slot.monoScratch.data(), L, R, numSamples, bpm, ppqPosition);
            break;
        }

        case Offcut:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.offcut.processBlock(slot.monoScratch.data(), L, R, numSamples, bpm, ppqPosition);
            break;
        }

        case Omen:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.omen.processBlock(slot.monoScratch.data(), L, R, numSamples, bpm, ppqPosition);
            break;
        }

        // ---- Wave 2 — Alt Universe (Mono-In chains) ----
        case Opus:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.opus.processBlock(slot.monoScratch.data(), L, R, numSamples, bpm, ppqPosition);
            break;
        }

        case Outlaw:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.outlaw.processBlock(slot.monoScratch.data(), L, R, numSamples, bpm, ppqPosition);
            break;
        }

        case Outbreak:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.outbreak.processBlock(slot.monoScratch.data(), L, R, numSamples, bpm, ppqPosition);
            break;
        }

        case Orrery:
        {
            for (int i = 0; i < numSamples; ++i)
                slot.monoScratch[i] = (L[i] + R[i]) * 0.5f;
            slot.orrery.processBlock(slot.monoScratch.data(), L, R, numSamples, bpm, ppqPosition);
            break;
        }

        // FX Pack 1 — Sidechain Creative (stereo-in/stereo-out, no mono fold).
        case Otrium:
        {
            slot.otrium.processBlock(L, R, numSamples, bpm, ppqPosition);
            break;
        }

        case Oblate:
        {
            slot.oblate.processBlock(L, R, numSamples, bpm, ppqPosition);
            break;
        }

        case Oligo:
        {
            slot.oligo.processBlock(L, R, numSamples, bpm, ppqPosition);
            break;
        }

        default: break;
    }
}

inline void EpicChainSlotController::applyMix(float* dryL, float* dryR,
                                               float* wetL, float* wetR,
                                               int numSamples, float mix)
{
    const float dry = 1.0f - mix;
    for (int i = 0; i < numSamples; ++i)
    {
        wetL[i] = wetL[i] * mix + dryL[i] * dry;
        wetR[i] = wetR[i] * mix + dryR[i] * dry;
    }
}

inline void EpicChainSlotController::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    using AP  = juce::AudioParameterFloat;
    using NR  = juce::NormalisableRange<float>;

    // ---- Slot control parameters (9 total) ----
    for (int n = 1; n <= kNumSlots; ++n)
    {
        juce::String prefix = "slot" + juce::String(n) + "_";
        layout.add(std::make_unique<AP>(
            juce::ParameterID{prefix + "chain", 1},
            prefix + "Chain",
            NR(0.0f, static_cast<float>(kMaxChainID), 1.0f),
            0.0f));
        layout.add(std::make_unique<AP>(
            juce::ParameterID{prefix + "mix", 1},
            prefix + "Mix",
            NR(0.0f, 1.0f, 0.001f),
            1.0f));
        layout.add(std::make_unique<AP>(
            juce::ParameterID{prefix + "bypass", 1},
            prefix + "Bypass",
            NR(0.0f, 1.0f, 1.0f),
            0.0f));
    }

    // ---- Legacy chain parameters are registered by XOceanusProcessor::createParameterLayout()
    // (aqua_* via AquaticFXSuite::addParameters(layout), mfx_* and bfx_* via vector API,
    //  master_onsl*/master_obs*/master_ora* are registered by fXOnslaught/fXObscura/fXOratory
    //  addParameters below — these were previously un-registered, so this is the first registration).
    fXOnslaught::addParameters(layout);
    fXObscura::addParameters(layout);
    fXOratory::addParameters(layout);
    // ---- Epic chain parameters (new, not registered anywhere else) ----
    OnrushChain::addParameters(layout);
    OmnistereoChain::addParameters(layout);
    ObliterateChain::addParameters(layout);
    ObscurityChain::addParameters(layout);
    // Wave 2 chain parameters
    OublietteChain::addParameters(layout);
    OsmiumChain::addParameters(layout);
    OrogenChain::addParameters(layout);
    OculusChain::addParameters(layout);
    OutageChain::addParameters(layout);
    OverrideChain::addParameters(layout);
    OcclusionChain::addParameters(layout);
    ObdurateChain::addParameters(layout);
    OrisonChain::addParameters(layout);
    OvershootChain::addParameters(layout);
    ObverseChain::addParameters(layout);
    OxymoronChain::addParameters(layout);
    OrnateChain::addParameters(layout);
    OrationChain::addParameters(layout);
    OffcutChain::addParameters(layout);
    OmenChain::addParameters(layout);
    OpusChain::addParameters(layout);
    OutlawChain::addParameters(layout);
    OutbreakChain::addParameters(layout);
    OrreryChain::addParameters(layout);
    OtriumChain::addParameters(layout); // FX Pack 1 scaffold
    OblateChain::addParameters(layout); // FX Pack 1 scaffold
    OligoChain::addParameters(layout);  // FX Pack 1 scaffold
}

inline void EpicChainSlotController::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts)
{
    for (int n = 0; n < kNumSlots; ++n)
    {
        juce::String prefix = "slot" + juce::String(n + 1) + "_";
        slots_[n].pChain  = apvts.getRawParameterValue(prefix + "chain");
        slots_[n].pMix    = apvts.getRawParameterValue(prefix + "mix");
        slots_[n].pBypass = apvts.getRawParameterValue(prefix + "bypass");

        // Cache param pointers — all slot instances share the same fixed IDs.
        // All 3 slot instances point to the same APVTS params; whichever is active uses them.
        slots_[n].aquatic.cacheParameterPointers(apvts);
        slots_[n].math.cacheParameterPointers(apvts);
        slots_[n].boutique.cacheParameterPointers(apvts);
        slots_[n].onslaught.cacheParameterPointers(apvts);
        slots_[n].obscura.cacheParameterPointers(apvts);
        slots_[n].oratory.cacheParameterPointers(apvts);
        // Epic chains
        slots_[n].onrush.cacheParameterPointers(apvts);
        slots_[n].omnistereo.cacheParameterPointers(apvts);
        slots_[n].obliterate.cacheParameterPointers(apvts);
        slots_[n].obscurity.cacheParameterPointers(apvts);
        // Wave 2 chains
        slots_[n].oubliette.cacheParameterPointers(apvts);
        slots_[n].osmium.cacheParameterPointers(apvts);
        slots_[n].orogen.cacheParameterPointers(apvts);
        slots_[n].oculus.cacheParameterPointers(apvts);
        slots_[n].outage.cacheParameterPointers(apvts);
        slots_[n].override_.cacheParameterPointers(apvts);
        slots_[n].occlusion.cacheParameterPointers(apvts);
        slots_[n].obdurate.cacheParameterPointers(apvts);
        slots_[n].orison.cacheParameterPointers(apvts);
        slots_[n].overshoot.cacheParameterPointers(apvts);
        slots_[n].obverse.cacheParameterPointers(apvts);
        slots_[n].oxymoron.cacheParameterPointers(apvts);
        slots_[n].ornate.cacheParameterPointers(apvts);
        slots_[n].oration.cacheParameterPointers(apvts);
        slots_[n].offcut.cacheParameterPointers(apvts);
        slots_[n].omen.cacheParameterPointers(apvts);
        slots_[n].opus.cacheParameterPointers(apvts);
        slots_[n].outlaw.cacheParameterPointers(apvts);
        slots_[n].outbreak.cacheParameterPointers(apvts);
        slots_[n].orrery.cacheParameterPointers(apvts);
        slots_[n].otrium.cacheParameterPointers(apvts); // FX Pack 1 scaffold
        slots_[n].oblate.cacheParameterPointers(apvts); // FX Pack 1 scaffold
        slots_[n].oligo.cacheParameterPointers(apvts);  // FX Pack 1 scaffold
    }
}

} // namespace xoceanus
