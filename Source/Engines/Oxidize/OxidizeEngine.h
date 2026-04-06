// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OxidizeEngine.h — XOxidize | Main engine (voices + sediment + MIDI)
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  Engine #77 — Degradation as synthesis. The Second Law of Thermodynamics
//  as a performance interface. 8 polyphonic voices, per-voice age accumulator,
//  shared sediment FDN reverb, 3-tap sends per voice.
//
//  Render loop (processBlock):
//    1. Update snapshot from APVTS (updateFrom)
//    2. Apply macro modulations to snapshot values (block-rate)
//    3. Process MIDI events (note on/off, pitch bend, mod wheel, aftertouch)
//    4. Clear sediment send buffers
//    5. For each active voice: updateAge, then renderSamples
//    6. Sum voice outputs to main buffer
//    7. setParameters on sediment, processBlock
//    8. Mix sediment wet into main buffer
//    9. Update outputCacheL/R for coupling reads
//
//  CPU budget (Release, arm64): ~8-10% at 8 voices, 44100 Hz, 128 block
//==============================================================================

#include "OxidizeParamSnapshot.h"
#include "OxidizeVoice.h"
#include "OxidizeSediment.h"
#include "OxidizeLookupTables.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
// OxidizeEngine — manages 8 OxidizeVoice instances + shared OxidizeSediment.
//
// This class owns all DSP state for OXIDIZE and is accessed exclusively on
// the audio thread.  The OxidizeAdapter (thin SynthEngine wrapper) delegates
// all audio work here.
//==============================================================================
class OxidizeEngine
{
public:
    static constexpr int MaxVoices = 8;

    //==========================================================================
    // prepare — allocate buffers, init voices, sediment, LUTs.
    // Must be called before processBlock().
    //==========================================================================
    void prepare(double sampleRate, int maxBlockSize) noexcept
    {
        sampleRate_   = sampleRate;
        maxBlockSize_ = maxBlockSize;

        // Initialise lookup tables (62KB, static shape — computed once)
        luts_.initialize();

        // Initialise all voices
        for (auto& v : voices_)
            v.prepare(sampleRate, maxBlockSize);

        // Initialise shared sediment reverb
        sediment_.prepare(sampleRate, maxBlockSize);

        // Allocate sediment send + reverb output scratch buffers
        sedimentSendL_.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        sedimentSendR_.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        sedimentOutL_.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        sedimentOutR_.assign(static_cast<size_t>(maxBlockSize), 0.0f);

        // Allocate per-voice voice output scratch buffers (reused each block)
        voiceBufL_.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        voiceBufR_.assign(static_cast<size_t>(maxBlockSize), 0.0f);

        // Reset coupling state
        couplingAgeBoost_   = 0.0f;
        couplingWobbleMod_  = 0.0f;
        couplingFilterMod_  = 0.0f;
        couplingRingAmount_ = 0.0f;
        couplingRingBuffer_.assign(static_cast<size_t>(maxBlockSize), 0.0f);

        outputCacheL_.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheR_.assign(static_cast<size_t>(maxBlockSize), 0.0f);

        sampleClock_ = 0;
        pitchBendPosition_ = 0.0f;
        modWheelPosition_  = 0.0f;
        aftertouchValue_   = 0.0f;
    }

    //==========================================================================
    // reset — clear all voice/sediment/coupling state without reallocating.
    //==========================================================================
    void reset() noexcept
    {
        for (auto& v : voices_)
        {
            v.active = false;
            v.resetState();
        }

        sediment_.reset();

        couplingAgeBoost_   = 0.0f;
        couplingWobbleMod_  = 0.0f;
        couplingFilterMod_  = 0.0f;
        couplingRingAmount_ = 0.0f;
        if (!couplingRingBuffer_.empty())
            std::fill(couplingRingBuffer_.begin(), couplingRingBuffer_.end(), 0.0f);

        pitchBendPosition_ = 0.0f;
        modWheelPosition_  = 0.0f;
        aftertouchValue_   = 0.0f;
        sampleClock_       = 0;
    }

    //==========================================================================
    // getActiveVoiceCount — safe to call from message thread (reads atomic).
    //==========================================================================
    int getActiveVoiceCount() const noexcept
    {
        int count = 0;
        for (const auto& v : voices_)
            if (v.active) ++count;
        return count;
    }

    //==========================================================================
    // Coupling accumulator setters — called by OxidizeAdapter before processBlock.
    //==========================================================================
    void setCouplingAgeBoost(float v)   noexcept { couplingAgeBoost_   = v; }
    void setCouplingWobbleMod(float v)  noexcept { couplingWobbleMod_  = v; }
    void setCouplingFilterMod(float v)  noexcept { couplingFilterMod_  = v; }

    void setCouplingRingBuffer(const float* src, int numSamples, float amount) noexcept
    {
        couplingRingAmount_ = amount;
        const int toCopy = std::min(numSamples, maxBlockSize_);
        if (src != nullptr && !couplingRingBuffer_.empty())
            std::copy(src, src + toCopy, couplingRingBuffer_.begin());
    }

    //==========================================================================
    // getSampleForCoupling — O(1) cached read (called by MegaCouplingMatrix).
    //==========================================================================
    float getSampleForCoupling(int channel, int sampleIndex) const noexcept
    {
        const int idx = std::clamp(sampleIndex, 0,
                                   static_cast<int>(outputCacheL_.size()) - 1);
        return (channel == 0) ? outputCacheL_[static_cast<size_t>(idx)]
                              : outputCacheR_[static_cast<size_t>(idx)];
    }

    //==========================================================================
    // processBlock — main render entry point.
    //
    // hostBPM:     DAW tempo in BPM. 0 = unavailable → dropout falls back to free mode.
    // hostBeatPos: DAW beat position in PPQ (quarter notes since session start).
    // hostIsPlaying: true when DAW transport is rolling.
    //==========================================================================
    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer&          midi,
                      xooxidize::OxidizeParamSnapshot& snap,
                      int numSamples,
                      float  hostBPM      = 0.0f,
                      double hostBeatPos  = 0.0,
                      bool   hostIsPlaying = false) noexcept
    {
        jassert(numSamples <= maxBlockSize_);

        float* outL = buffer.getWritePointer(0);
        float* outR = (buffer.getNumChannels() > 1) ? buffer.getWritePointer(1) : outL;

        // ── Step 3: Process MIDI events ──────────────────────────────────────
        // (Snapshot already updated + macros applied by the adapter before this call)
        processMidi(midi, snap, numSamples);

        // ── Step 4: Clear sediment send buffers ───────────────────────────────
        std::fill(sedimentSendL_.begin(), sedimentSendL_.begin() + numSamples, 0.0f);
        std::fill(sedimentSendR_.begin(), sedimentSendR_.begin() + numSamples, 0.0f);

        // ── Step 5: Update age + render each active voice ─────────────────────
        // Pre-compute block dt (block-rate age update)
        const float dt = static_cast<float>(numSamples) / static_cast<float>(sampleRate_);

        // Apply coupling-driven wobble mod to snapshot (block-rate)
        float snapWowRate     = snap.wowRate     * (1.0f + couplingWobbleMod_);
        float snapFlutterRate = snap.flutterRate * (1.0f + couplingWobbleMod_);
        snapWowRate     = std::clamp(snapWowRate,     0.01f, 4.0f);
        snapFlutterRate = std::clamp(snapFlutterRate, 6.0f, 20.0f);

        // Apply coupling filter modulation to erosion floor (block-rate)
        // SpectralShaping: coupling spectral centroid shifts erosion cutoff
        if (couplingFilterMod_ != 0.0f)
        {
            // couplingFilterMod_ is normalised spectral centroid (0-1) * amount
            // Map to ±8000 Hz shift on erosionFloor
            float shift = couplingFilterMod_ * 8000.0f;
            snap.erosionFloor = std::clamp(snap.erosionFloor + shift, 20.0f, 18000.0f);
        }

        // Apply adjusted wobble rates back into snap for voice use
        float savedWowRate     = snap.wowRate;
        float savedFlutterRate = snap.flutterRate;
        snap.wowRate     = snapWowRate;
        snap.flutterRate = snapFlutterRate;

        // Clear main output accumulation area
        std::fill(outL, outL + numSamples, 0.0f);
        if (buffer.getNumChannels() > 1)
            std::fill(outR, outR + numSamples, 0.0f);

        for (auto& voice : voices_)
        {
            if (!voice.isActive())
                continue;

            // Clear per-voice scratch buffers
            std::fill(voiceBufL_.begin(), voiceBufL_.begin() + numSamples, 0.0f);
            std::fill(voiceBufR_.begin(), voiceBufR_.begin() + numSamples, 0.0f);

            // Block-rate: update age accumulator + derive age-dependent state
            voice.updateAge(dt, snap, luts_, couplingAgeBoost_);

            // Per-sample: render voice DSP (adds into voiceBufL/R + sediment sends)
            voice.renderSamples(voiceBufL_.data(), voiceBufR_.data(),
                                numSamples, snap, luts_,
                                sedimentSendL_.data(), sedimentSendR_.data(),
                                hostBPM, hostBeatPos, hostIsPlaying);

            // Apply ring modulation if AudioToRing coupling is active
            if (couplingRingAmount_ > 0.001f && !couplingRingBuffer_.empty())
            {
                for (int i = 0; i < numSamples; ++i)
                {
                    float ring = couplingRingBuffer_[static_cast<size_t>(i)];
                    float dryWet = couplingRingAmount_;
                    voiceBufL_[static_cast<size_t>(i)] = voiceBufL_[static_cast<size_t>(i)] * (1.0f - dryWet)
                                                         + voiceBufL_[static_cast<size_t>(i)] * ring * dryWet;
                    voiceBufR_[static_cast<size_t>(i)] = voiceBufR_[static_cast<size_t>(i)] * (1.0f - dryWet)
                                                         + voiceBufR_[static_cast<size_t>(i)] * ring * dryWet;
                }
            }

            // ── Step 6: Sum voice outputs to main buffer ──────────────────────
            for (int i = 0; i < numSamples; ++i)
            {
                outL[i] += voiceBufL_[static_cast<size_t>(i)];
                outR[i] += voiceBufR_[static_cast<size_t>(i)];
            }
        }

        // Restore snap wobble rates (snap is passed in by ref; adapter re-reads next block)
        snap.wowRate     = savedWowRate;
        snap.flutterRate = savedFlutterRate;

        // ── Step 7: Process shared sediment reverb ────────────────────────────
        sediment_.setParameters(snap.sedimentTail, snap.sedimentTone, sampleRate_);
        sediment_.processBlock(sedimentSendL_.data(), sedimentSendR_.data(),
                               sedimentOutL_.data(),  sedimentOutR_.data(),
                               numSamples);

        // ── Step 8: Mix sediment wet output into main buffer ──────────────────
        for (int i = 0; i < numSamples; ++i)
        {
            outL[i] += sedimentOutL_[static_cast<size_t>(i)];
            outR[i] += sedimentOutR_[static_cast<size_t>(i)];
        }

        // ── Step 9: Cache output samples for coupling reads ───────────────────
        for (int i = 0; i < numSamples; ++i)
        {
            outputCacheL_[static_cast<size_t>(i)] = outL[i];
            outputCacheR_[static_cast<size_t>(i)] = outR[i];
        }

        // Advance sample clock (used for LRU voice stealing)
        sampleClock_ += static_cast<uint64_t>(numSamples);

        // Reset per-block coupling accumulators (adapter re-sets from next
        // applyCouplingInput call before the next processBlock)
        couplingAgeBoost_   = 0.0f;
        couplingWobbleMod_  = 0.0f;
        couplingFilterMod_  = 0.0f;
        couplingRingAmount_ = 0.0f;
    }

private:
    //==========================================================================
    // processMidi — handle note on/off, pitch bend, mod wheel, aftertouch.
    // Events within the block are processed at their sample-accurate positions
    // for note on/off; continuous controllers are treated as block-rate.
    //==========================================================================
    void processMidi(juce::MidiBuffer& midi,
                     xooxidize::OxidizeParamSnapshot& snap,
                     int /*numSamples*/) noexcept
    {
        for (const auto& meta : midi)
        {
            const auto msg = meta.getMessage();

            if (msg.isNoteOn())
            {
                const int   note = msg.getNoteNumber();
                const float vel  = msg.getFloatVelocity();
                noteOn(note, vel, snap);
            }
            else if (msg.isNoteOff())
            {
                const int note = msg.getNoteNumber();
                noteOff(note);
            }
            else if (msg.isPitchWheel())
            {
                // Normalise pitch wheel to [-1, +1]
                pitchBendPosition_ = static_cast<float>(msg.getPitchWheelValue() - 8192)
                                     / 8192.0f;
            }
            else if (msg.isControllerOfType(1)) // CC1 = Mod Wheel
            {
                modWheelPosition_ = static_cast<float>(msg.getControllerValue()) / 127.0f;
            }
            else if (msg.isAftertouch() || msg.isChannelPressure())
            {
                if (msg.isChannelPressure())
                    aftertouchValue_ = static_cast<float>(msg.getChannelPressureValue()) / 127.0f;
                else
                    aftertouchValue_ = static_cast<float>(msg.getAfterTouchValue()) / 127.0f;
            }
        }

        // Write live controller values into snapshot for voice processing.
        // pitchBend in snap is the actual bend in semitones (range * position).
        snap.pitchBend  = snap.pitchBend * pitchBendPosition_;  // = bendRange * bendPos
        snap.aftertouch = aftertouchValue_ * snap.aftertouch;   // scale by user depth param
        snap.modWheel   = modWheelPosition_ * snap.modWheel;    // scale by user depth param
    }

    //==========================================================================
    // noteOn — find a free voice (or steal the oldest), initialise it.
    //==========================================================================
    void noteOn(int note, float velocity,
                const xooxidize::OxidizeParamSnapshot& snap) noexcept
    {
        // Re-trigger any existing voice on the same note (legato / retrigger)
        for (auto& v : voices_)
        {
            if (v.active && v.note == note)
            {
                const float snapshotAge = computeSnapshotAge(velocity, snap);
                v.noteOn(note, velocity, snapshotAge, snap.reverseAge != 0, snap, sampleClock_);
                return;
            }
        }

        // Find a free voice
        OxidizeVoice* target = nullptr;
        for (auto& v : voices_)
        {
            if (!v.active)
            {
                target = &v;
                break;
            }
        }

        // LRU steal: find the oldest active voice
        if (target == nullptr)
        {
            uint64_t oldest = UINT64_MAX;
            for (auto& v : voices_)
            {
                if (v.startTime < oldest)
                {
                    oldest = v.startTime;
                    target = &v;
                }
            }
        }

        if (target != nullptr)
        {
            // Detect steal: target was active before reset.
            const bool isSteal = target->active;
            target->resetState();
            const float snapshotAge = computeSnapshotAge(velocity, snap);
            target->noteOn(note, velocity, snapshotAge, snap.reverseAge != 0, snap, sampleClock_);
            // P0-02 fix: stolen voice starts at gain=0 and ramps to 1 over 5ms,
            // preventing a click from the abrupt DSP state reset in resetState().
            if (isSteal)
                target->stealFadeGain = 0.0f;
        }
    }

    //==========================================================================
    // noteOff — release the voice matching the given note.
    //==========================================================================
    void noteOff(int note) noexcept
    {
        for (auto& v : voices_)
        {
            if (v.active && v.note == note)
            {
                v.noteOff();
                // Note: voice stays active until envelope completes
                return;
            }
        }
    }

    //==========================================================================
    // computeSnapshotAge — derive initial age from preset ageOffset + velocity.
    // Smith design: preset stores the initial age position; velocity adds depth.
    //==========================================================================
    static float computeSnapshotAge(float velocity,
                                    const xooxidize::OxidizeParamSnapshot& snap) noexcept
    {
        float age = snap.ageOffset + velocity * snap.ageVelSens * 0.4f;
        return std::clamp(age, 0.0f, 1.0f);
    }

    //==========================================================================
    // Members
    //==========================================================================

    // Voice pool
    std::array<OxidizeVoice, MaxVoices> voices_;

    // Shared sediment reverb
    OxidizeSediment sediment_;

    // Shared lookup tables (62KB, initialized in prepare())
    oxidize::LookupTables luts_;

    // Audio engine state
    double   sampleRate_   = 0.0;  // Sentinel: must be set by prepare() before use
    int      maxBlockSize_ = 512;
    uint64_t sampleClock_  = 0;

    // MIDI expression state (block-rate)
    float pitchBendPosition_ = 0.0f; // [-1, +1] normalised
    float modWheelPosition_  = 0.0f; // [0, 1]
    float aftertouchValue_   = 0.0f; // [0, 1]

    // Scratch buffers — allocated in prepare(), reused each block
    std::vector<float> sedimentSendL_;
    std::vector<float> sedimentSendR_;
    std::vector<float> sedimentOutL_;
    std::vector<float> sedimentOutR_;
    std::vector<float> voiceBufL_;
    std::vector<float> voiceBufR_;

    // Coupling output cache (full block, for per-sample coupling reads)
    std::vector<float> outputCacheL_;
    std::vector<float> outputCacheR_;

    // Coupling input accumulators — set by adapter, consumed + reset in processBlock
    float couplingAgeBoost_   = 0.0f; // AmplitudeModulation → age rate boost
    float couplingWobbleMod_  = 0.0f; // FrequencyModulation → wobble rate multiplier
    float couplingFilterMod_  = 0.0f; // SpectralShaping → erosion floor shift (normalised)
    float couplingRingAmount_ = 0.0f; // AudioToRing → ring mod depth
    std::vector<float> couplingRingBuffer_;   // AudioToRing source audio
};

} // namespace xoceanus
