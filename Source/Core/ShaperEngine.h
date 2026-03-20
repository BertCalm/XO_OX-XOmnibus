#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/SRO/SilenceGate.h"

namespace xomnibus {

//==============================================================================
// ShaperEngine — The Utility Engine Interface
//
// Shapers process audio (in-place) rather than synthesize it. They live in
// insert slots (one per engine, pre-mix) and bus slots (post-mix, pre-master).
//
// Design contract (same real-time safety as SynthEngine):
//   - No memory allocation in processBlock()
//   - No blocking I/O in processBlock()
//   - getSampleForCoupling() must be O(1) — return cached sample
//   - applyCouplingInput() accumulates; processBlock() consumes
//   - createParameterLayout() returns shaper-namespaced parameter IDs
//
// Unlike SynthEngine: no voices, no polyphony, no MPE. Audio in → audio out.
// MIDI is available for sidechain triggers, key-tracking, and rhythm sync.
//
class ShaperEngine
{
public:
    virtual ~ShaperEngine() = default;

    //-- Lifecycle -------------------------------------------------------------

    virtual void prepare (double sampleRate, int maxBlockSize) = 0;
    virtual void releaseResources() = 0;
    virtual void reset() = 0;

    //-- Audio -----------------------------------------------------------------

    // Process audio in-place. Buffer contains the engine's output (insert slot)
    // or the mixed bus signal (bus slot). MIDI is forwarded for sidechain,
    // key-tracking, and rhythm-sync use cases.
    // Must be real-time safe: no allocation, no blocking, no exceptions.
    virtual void processBlock (juce::AudioBuffer<float>& buffer,
                               juce::MidiBuffer& midi,
                               int numSamples) = 0;

    //-- Coupling (bidirectional — shapers sense AND modulate) -----------------

    // Return the most recent analysis/output sample for coupling reads.
    // Channel semantics are shaper-specific:
    //   XObserve: ch0-5 = per-band energy (6 channels of spectral intelligence)
    //   XOxide:   ch0 = harmonic density, ch1 = saturation amount
    //   XOpress:  ch0 = gain reduction amount
    // Must be O(1) — return from a cached member.
    virtual float getSampleForCoupling (int channel, int sampleIndex) const = 0;

    // Receive modulation from engines or other shapers via coupling matrix.
    virtual void applyCouplingInput (CouplingType type,
                                    float amount,
                                    const float* sourceBuffer,
                                    int numSamples) = 0;

    //-- Parameters ------------------------------------------------------------

    virtual juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout() = 0;

    virtual void attachParameters (juce::AudioProcessorValueTreeState& apvts) = 0;

    //-- Identity --------------------------------------------------------------

    virtual juce::String getShaperId() const = 0;
    virtual juce::Colour getAccentColour() const = 0;

    //-- SRO: SilenceGate — zero CPU when input is silent ----------------------
    //
    // Shapers use the same SilenceGate as engines. When the input signal
    // falls below -90 dB for the hold duration, processBlock() is bypassed
    // entirely. Hold times:
    //   Tone (EQ, saturation):     100ms (no tail)
    //   Dynamics (comp, gate):     200ms (release tail)
    //   Space (reverb, panner):    500ms (reverb tail)
    //   Modulation (LFO, gate):    100ms
    //   Creative (tape, convo):    500ms (decay tail)
    //   Utility (feedback, harm):  200ms
    //
    SilenceGate silenceGate;

    //-- Bypass ----------------------------------------------------------------

    bool isBypassed() const { return bypassed; }
    void setBypassed (bool b) { bypassed = b; }

private:
    bool bypassed = false;
};

} // namespace xomnibus
