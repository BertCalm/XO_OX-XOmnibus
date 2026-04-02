// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <cmath>

namespace xoceanus {

//==============================================================================
// SharedTransport — Unified timing/transport for all engines.
//
// Provides a single clock that engines read during renderBlock().
// Engines with sequencers (ONSET) and tempo-synced LFOs (ODYSSEY, OBLONG)
// consume beat position and phase from this shared source.
//
// Thread safety:
//   - processBlock() is called on the audio thread only.
//   - Getters use atomics so the UI thread can safely read transport state.
//   - Setters for internal clock state are called from the message thread.
//
// Real-time contract:
//   - processBlock() performs no allocation, no blocking, no exceptions.
//   - All getters return cached values in O(1).
//
class SharedTransport
{
public:
    //-- Sync mode -------------------------------------------------------------

    enum class SyncMode { Internal, Host, Auto };

    //-- Construction ----------------------------------------------------------

    SharedTransport()
    {
        bpm.store (120.0);
        playing.store (false);
        beatPosition.store (0.0);
        barPosition.store (0.0);
        timeSigNumerator.store (4);
        timeSigDenominator.store (4);
        samplesPerBeat.store (44100.0 / 2.0); // 120 BPM at 44.1 kHz
        syncMode.store (SyncMode::Auto);
        cachedSampleRate.store (44100.0);
    }

    //==========================================================================
    // Called by the processor at the start of each audio block.
    // Reads host position info if available and sync mode permits,
    // otherwise advances the internal clock.
    //
    void processBlock (int numSamples, double sampleRate,
                       const juce::AudioPlayHead::PositionInfo* hostPosition)
    {
        cachedSampleRate.store (sampleRate, std::memory_order_release);

        const auto mode = syncMode.load (std::memory_order_acquire);
        bool useHost = false;

        //-- Determine whether to follow the host --------------------------
        if (hostPosition != nullptr && mode != SyncMode::Internal)
        {
            // Host info is considered valid if it provides a BPM.
            // Some hosts always fill the struct but leave BPM at 0 when
            // there is no timeline — treat that as unavailable.
            if (hostPosition->getBpm().hasValue() && *hostPosition->getBpm() > 0.0)
                useHost = true;
        }

        if (useHost)
            processHostSync (*hostPosition, sampleRate);
        else
            processInternalClock (numSamples, sampleRate);
    }

    //==========================================================================
    // Transport state — read by engines during their renderBlock().
    // All getters are real-time safe (atomic loads only).

    bool   isPlaying()            const { return playing.load (std::memory_order_acquire); }
    double getBPM()               const { return bpm.load (std::memory_order_acquire); }
    double getBeatPosition()      const { return beatPosition.load (std::memory_order_acquire); }
    double getBarPosition()       const { return barPosition.load (std::memory_order_acquire); }
    int    getTimeSigNumerator()  const { return timeSigNumerator.load (std::memory_order_acquire); }
    int    getTimeSigDenominator() const { return timeSigDenominator.load (std::memory_order_acquire); }
    double getSamplesPerBeat()    const { return samplesPerBeat.load (std::memory_order_acquire); }

    //--------------------------------------------------------------------------
    // Returns a 0.0–1.0 phase for a given note division relative to the
    // current beat position. The division is expressed as a fraction of a beat:
    //   0.25  = 16th note  (4 cycles per beat)
    //   0.5   = 8th note   (2 cycles per beat)
    //   1.0   = quarter    (1 cycle per beat)
    //   2.0   = half note  (cycle every 2 beats)
    //   4.0   = whole note (cycle every 4 beats)
    //
    double getPhaseForDivision (double division) const
    {
        if (division <= 0.0)
            return 0.0;

        const double beat = beatPosition.load (std::memory_order_acquire);
        // How many full division-cycles fit in beat?
        const double cyclePos = beat / division;
        // Fractional part is the phase within the current cycle.
        return cyclePos - std::floor (cyclePos);
    }

    //==========================================================================
    // Internal clock controls — called from the message thread.
    // These affect the internal clock only; host-synced mode ignores them.

    void setBPM (double newBPM)
    {
        if (newBPM > 0.0)
        {
            bpm.store (newBPM, std::memory_order_release);
            updateSamplesPerBeat (newBPM, cachedSampleRate.load (std::memory_order_acquire));
        }
    }

    void setPlaying (bool shouldPlay)
    {
        playing.store (shouldPlay, std::memory_order_release);
    }

    void setTimeSignature (int numerator, int denominator)
    {
        if (numerator > 0 && denominator > 0)
        {
            timeSigNumerator.store (numerator, std::memory_order_release);
            timeSigDenominator.store (denominator, std::memory_order_release);
        }
    }

    void resetPosition()
    {
        beatPosition.store (0.0, std::memory_order_release);
        barPosition.store (0.0, std::memory_order_release);
    }

    //-- Sync mode control -----------------------------------------------------

    void setSyncMode (SyncMode mode)
    {
        syncMode.store (mode, std::memory_order_release);
    }

    SyncMode getSyncMode() const
    {
        return syncMode.load (std::memory_order_acquire);
    }

private:
    //==========================================================================
    // Process using host-provided position info.
    //
    void processHostSync (const juce::AudioPlayHead::PositionInfo& pos,
                          double sampleRate)
    {
        // BPM
        if (pos.getBpm().hasValue())
        {
            const double hostBPM = *pos.getBpm();
            bpm.store (hostBPM, std::memory_order_release);
            updateSamplesPerBeat (hostBPM, sampleRate);
        }

        // Playing state
        playing.store (pos.getIsPlaying(), std::memory_order_release);

        // Time signature
        if (pos.getTimeSignature().hasValue())
        {
            const auto sig = *pos.getTimeSignature();
            timeSigNumerator.store (sig.numerator, std::memory_order_release);
            timeSigDenominator.store (sig.denominator, std::memory_order_release);
        }

        // Beat and bar position
        if (pos.getPpqPosition().hasValue())
        {
            const double ppq = *pos.getPpqPosition();
            beatPosition.store (ppq, std::memory_order_release);

            // Compute bar position from ppq and time signature.
            const int num = timeSigNumerator.load (std::memory_order_acquire);
            const double beatsPerBar = (num > 0) ? static_cast<double> (num) : 4.0;
            barPosition.store (ppq / beatsPerBar, std::memory_order_release);
        }
        else if (pos.getPpqPositionOfLastBarStart().hasValue())
        {
            // Some hosts provide bar start but not absolute ppq — do our best.
            const double barStart = *pos.getPpqPositionOfLastBarStart();
            const int num = timeSigNumerator.load (std::memory_order_acquire);
            const double beatsPerBar = (num > 0) ? static_cast<double> (num) : 4.0;
            barPosition.store (barStart / beatsPerBar, std::memory_order_release);
        }
    }

    //==========================================================================
    // Advance the internal free-running clock.
    //
    void processInternalClock (int numSamples, double sampleRate)
    {
        const double currentBPM = bpm.load (std::memory_order_acquire);
        updateSamplesPerBeat (currentBPM, sampleRate);

        if (! playing.load (std::memory_order_acquire))
            return;

        const double spb = samplesPerBeat.load (std::memory_order_acquire);
        if (spb <= 0.0)
            return;

        const double beatIncrement = static_cast<double> (numSamples) / spb;
        double beat = beatPosition.load (std::memory_order_acquire) + beatIncrement;

        // Wrap at bar boundary
        const int num = timeSigNumerator.load (std::memory_order_acquire);
        const double beatsPerBar = (num > 0) ? static_cast<double> (num) : 4.0;

        // Allow beat position to grow indefinitely (engines may want
        // absolute position) but also compute bar-relative position.
        beatPosition.store (beat, std::memory_order_release);
        barPosition.store (beat / beatsPerBar, std::memory_order_release);
    }

    //--------------------------------------------------------------------------
    void updateSamplesPerBeat (double currentBPM, double sampleRate)
    {
        if (currentBPM > 0.0 && sampleRate > 0.0)
            samplesPerBeat.store (sampleRate * 60.0 / currentBPM, std::memory_order_release);
    }

    //==========================================================================
    // Atomic state — safe for cross-thread reads.
    //
    // Using std::atomic<double> is lock-free on all modern 64-bit platforms
    // for aligned doubles (guaranteed by the standard on arm64 and x86-64).
    //
    std::atomic<double> bpm            { 120.0 };
    std::atomic<bool>   playing        { false };
    std::atomic<double> beatPosition   { 0.0 };
    std::atomic<double> barPosition    { 0.0 };
    std::atomic<int>    timeSigNumerator   { 4 };
    std::atomic<int>    timeSigDenominator { 4 };
    std::atomic<double> samplesPerBeat { 22050.0 }; // 120 BPM @ 44100 Hz

    std::atomic<SyncMode> syncMode { SyncMode::Auto };

    // Atomic — written on audio thread in processBlock(), read by setBPM() on message thread.
    std::atomic<double> cachedSampleRate { 44100.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SharedTransport)
};

} // namespace xoceanus
