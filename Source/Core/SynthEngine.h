#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "MPEManager.h"
#include "../DSP/SRO/SilenceGate.h"

namespace xomnibus {

//==============================================================================
// Coupling types supported by the MegaCouplingMatrix.
// Each defines how one engine's output modulates another engine's parameter.
enum class CouplingType {
    AmpToFilter,       // Engine A amplitude → Engine B filter cutoff
    AmpToPitch,        // Engine A amplitude → Engine B pitch
    LFOToPitch,        // Engine A LFO → Engine B pitch
    EnvToMorph,        // Engine A envelope → Engine B wavetable/morph position
    AudioToFM,         // Engine A audio → Engine B FM input
    AudioToRing,       // Engine A audio × Engine B audio
    FilterToFilter,    // Engine A filter output → Engine B filter input
    AmpToChoke,        // Engine A amplitude chokes Engine B
    RhythmToBlend,     // Engine A rhythm pattern → Engine B blend parameter
    EnvToDecay,        // Engine A envelope → Engine B decay time
    PitchToPitch,      // Engine A pitch → Engine B pitch (harmony)
    AudioToWavetable,  // Engine A audio → Engine B wavetable source
    AudioToBuffer,     // Engine A audio → Engine B ring buffer (continuous stereo streaming)
                       // Designed for OPAL's grain buffer — the "Time Telescope" coupling type.
                       // Unlike AudioToWavetable (periodic snapshots), AudioToBuffer streams
                       // every block into a pre-allocated circular buffer with freeze support.
                       // See Docs/xopal_phase1_architecture.md §15 for the full design.
    KnotTopology       // Bidirectional irreducible coupling (KNOT type, V2 Theorem feature).
                       // Both engines modulate each other's parameters simultaneously.
                       // Linking number (encoded in `amount` param, 1-5) sets entanglement depth:
                       //   linkingNum = juce::roundToInt(amount * 4.0f) + 1  →  range [1, 5]
                       // Unlike all other CouplingType values (which are sender→receiver),
                       // KnotTopology creates mutual, co-evolving entanglement: neither engine
                       // can be decoupled from the other without destroying the patch.
                       // See Docs/specs/knot_coupling_spec.md for full design.
};

//==============================================================================
// The SynthEngine interface.
//
// Every engine module (ODDFELIX, ODDOSCAR, OVERDUB, ODYSSEY, OBLONG, OBESE, ONSET, etc.) implements
// this interface. The XOmnibusProcessor holds up to 4 active engines and
// connects them through the MegaCouplingMatrix.
//
// Design contract:
//   - No memory allocation in renderBlock()
//   - No blocking I/O in renderBlock()
//   - getSampleForCoupling() must be O(1) — just return a cached sample
//   - applyCouplingInput() accumulates modulation; renderBlock() consumes it
//   - createParameterLayout() returns engine-namespaced parameter IDs
//
class SynthEngine {
public:
    virtual ~SynthEngine() = default;

    //-- Lifecycle -------------------------------------------------------------

    // Called before audio starts. Allocate buffers, initialize state.
    virtual void prepare(double sampleRate, int maxBlockSize) = 0;

    // Called when audio stops. Free non-essential resources.
    virtual void releaseResources() = 0;

    // Reset all engine state (oscillator phases, filter state, envelopes, etc.)
    // without reallocating buffers. Called on preset change and transport reset.
    virtual void reset() = 0;

    //-- Audio -----------------------------------------------------------------

    // Render a block of audio into `buffer`. Process MIDI from `midi`.
    // Must be real-time safe: no allocation, no blocking, no exceptions.
    virtual void renderBlock(juce::AudioBuffer<float>& buffer,
                            juce::MidiBuffer& midi,
                            int numSamples) = 0;

    //-- Coupling (the XOmnibus differentiator) --------------------------------

    // Return the most recent output sample for coupling reads.
    // Called per-sample by the MegaCouplingMatrix during tight coupling.
    // Must be O(1) — return from a cached member, not a computation.
    virtual float getSampleForCoupling(int channel, int sampleIndex) const = 0;

    // Receive modulation from another engine via the coupling matrix.
    // Called before renderBlock() for block-level coupling,
    // or per-sample for tight coupling types (AudioToFM, AudioToRing).
    virtual void applyCouplingInput(CouplingType type,
                                   float amount,
                                   const float* sourceBuffer,
                                   int numSamples) = 0;

    //-- Parameters ------------------------------------------------------------

    // Return the engine's parameter layout with namespaced IDs.
    // Example: ODDFELIX engine returns "snap_filterCutoff", "snap_resonance", etc.
    virtual juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout() = 0;

    // Cache raw parameter pointers from the shared APVTS.
    // Called once after the APVTS is constructed with all engine layouts merged.
    // Engines use these cached pointers in renderBlock() for zero-cost reads.
    virtual void attachParameters(juce::AudioProcessorValueTreeState& apvts) = 0;

    //-- Identity --------------------------------------------------------------

    // Return the engine's unique identifier (e.g., "OddfeliX", "OddOscar", "Overdub").
    virtual juce::String getEngineId() const = 0;

    // Return the engine's accent colour for UI theming.
    virtual juce::Colour getAccentColour() const = 0;

    // Return the maximum polyphony for this engine.
    virtual int getMaxVoices() const = 0;

    // Return the number of currently active (sounding) voices.
    // Default returns 0; engines with polyphony override this.
    // Safe to call from the message thread — implementations must use an atomic
    // or just return a cached counter updated at the end of each renderBlock().
    virtual int getActiveVoiceCount() const { return 0; }

    //-- MPE Expression --------------------------------------------------------

    // Set the MPE manager reference for per-note expression queries.
    // Called once during engine setup. Engines use this to look up per-channel
    // pitch bend, pressure, and slide values in their renderBlock().
    virtual void setMPEManager(MPEManager* manager) { mpeManager = manager; }

    //-- SRO: SilenceGate — Zero-Idle Bypass -----------------------------------
    //
    // Every engine has a SilenceGate that monitors output and bypasses
    // DSP processing when the signal falls below -90 dB. This eliminates
    // idle CPU cost fleet-wide. Engines configure hold time in prepare()
    // based on their tail characteristics:
    //
    //   Percussive (ONSET, OVERBITE, ODDFELIX):     100ms (default)
    //   Standard (OBLONG, ODYSSEY, OBLIQUE):         200ms
    //   Reverb-tail (OVERDUB, OPAL, OCEANIC):        500ms
    //   Infinite-sustain (ORGANON, OUROBOROS):       1000ms
    //
    // CRITICAL INTEGRATION ORDER in renderBlock():
    //   1. Parse MIDI → call wakeSilenceGate() on note-on
    //   2. Check isSilenceGateBypassed()
    //   3. If bypassed: clear buffer, return early (zero CPU)
    //   4. If active: run DSP, then call analyzeForSilenceGate()
    //

    /// Prepare the silence gate. Called by the processor after engine prepare().
    /// holdMs: gate hold time (100=percussive, 200=standard, 500=reverb, 1000=infinite).
    void prepareSilenceGate(double sampleRate, int maxBlockSize, float holdMs = 100.0f)
    {
        silenceGate.prepare(sampleRate, maxBlockSize);
        silenceGate.setHoldTime(holdMs);
    }

    /// Returns true if the engine's output has been silent for longer than
    /// the hold time. Safe to call from audio thread (lock-free atomic read).
    bool isSilenceGateBypassed() const { return silenceGate.isBypassed(); }

    /// Force the gate open. Call on note-on, preset change, or any event
    /// that will produce audio. Must be called BEFORE checking isBypassed().
    void wakeSilenceGate() { silenceGate.wake(); }

    /// Get the current peak level (for UI display). Thread-safe.
    float getSilenceGatePeakLevel() const { return silenceGate.getPeakLevel(); }

protected:
    // Engines access this to query per-channel expression state.
    // nullptr when MPE is disabled (engines should check before use).
    MPEManager* mpeManager = nullptr;

    // SRO: SilenceGate instance — engines configure hold time in prepare().
    // Call silenceGate.prepare(sampleRate, maxBlockSize) in engine's prepare().
    // Call silenceGate.analyzeBlock(L, R, numSamples) at end of renderBlock().
    SilenceGate silenceGate;

public:
    /// Helper: call from XOmnibusProcessor to feed the silence gate after renderBlock().
    void analyzeForSilenceGate(const juce::AudioBuffer<float>& buffer, int numSamples)
    {
        const float* L = buffer.getReadPointer(0);
        const float* R = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : L;
        silenceGate.analyzeBlock(L, R, numSamples);
    }
};

} // namespace xomnibus
