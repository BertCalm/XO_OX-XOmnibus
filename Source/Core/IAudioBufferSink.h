// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "AudioRingBuffer.h"

namespace xoceanus
{

//==============================================================================
// IAudioBufferSink — implemented by any engine that can receive an
// AudioToBuffer coupling route.
//
// MegaCouplingMatrix::processAudioRoute() queries the destination engine
// for this interface via dynamic_cast<IAudioBufferSink*>(dest). If the cast
// returns nullptr, the route is silently skipped — the dest engine does not
// support streaming audio input.
//
// Design constraints (all must hold for a conforming implementation):
//   - getGrainBuffer() must be callable on the audio thread with no allocation.
//   - getNumInputSlots() must return a value >= 1 and <= MegaCouplingMatrix::MaxSlots.
//   - receiveAudioBuffer() must be real-time safe: no allocation, no I/O, no locks.
//
// Thread safety note: all three methods are called on the audio thread by
// MegaCouplingMatrix. The implementing class is responsible for ensuring
// that prepare() (which calls AudioRingBuffer::prepare()) has completed on
// the non-audio thread before processBlock() is first called.
//
class IAudioBufferSink
{
public:
    virtual ~IAudioBufferSink() = default;

    // Return a pointer to the ring buffer for the given source slot index.
    // Slot indices correspond to MegaCouplingMatrix source slot positions (0-3).
    // Returns nullptr if slot is out of range -- callers must check.
    virtual AudioRingBuffer* getGrainBuffer(int slot) noexcept = 0;

    // Return the number of input slots this engine supports.
    // Used by MegaCouplingMatrix to validate slot assignments.
    // Must return a value in [1, MegaCouplingMatrix::MaxSlots].
    virtual int getNumInputSlots() const noexcept = 0;

    // Receive and blend a block of audio from a ring buffer source.
    // Called by OpalEngine (and future engines) during renderBlock().
    // `src`        -- the ring buffer to read from
    // `numSamples` -- number of samples in the current block
    // `mix`        -- blend ratio (0.0 = internal only, 1.0 = external only)
    // `frozen`     -- current FREEZE state; implementor may hold blend cache
    //                 rather than advancing through the ring when true
    virtual void receiveAudioBuffer(const AudioRingBuffer& src, int numSamples, float mix, bool frozen) noexcept = 0;
};

} // namespace xoceanus
