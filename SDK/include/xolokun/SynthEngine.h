#pragma once
// xolokun-engine-sdk — SynthEngine.h
// Standalone interface — no JUCE dependency at the boundary.
// Community engine developers implement this interface.
//
// All JUCE types are replaced with plain C++ equivalents:
//   juce::AudioBuffer<float>  →  StereoBuffer (float** + counts)
//   juce::MidiBuffer          →  MidiEventList (struct array)
//   juce::String              →  std::string
//   juce::Colour              →  Colour (uint8 RGBA)
//
// When building inside XOlokun, the adapter layer converts between
// JUCE types and these SDK types. When building standalone, developers
// work with these types directly.

#include "CouplingTypes.h"
#include <string>
#include <cstdint>
#include <vector>
#include <memory>

namespace xolokun {

//==============================================================================
// Plain types — no JUCE dependency
//==============================================================================

/// Colour as RGBA bytes.
struct Colour {
    uint8_t r = 0, g = 0, b = 0, a = 255;

    constexpr Colour() = default;
    constexpr Colour (uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255)
        : r(r_), g(g_), b(b_), a(a_) {}

    /// Construct from 0xAARRGGBB packed integer (same layout as juce::Colour).
    static constexpr Colour fromARGB (uint32_t argb)
    {
        return { static_cast<uint8_t> ((argb >> 16) & 0xFF),
                 static_cast<uint8_t> ((argb >>  8) & 0xFF),
                 static_cast<uint8_t> ((argb      ) & 0xFF),
                 static_cast<uint8_t> ((argb >> 24) & 0xFF) };
    }
};

/// Non-owning view of a stereo audio buffer.
struct StereoBuffer {
    float* left  = nullptr;
    float* right = nullptr;
    int numSamples = 0;
};

/// A single MIDI message.
struct MidiEvent {
    int sampleOffset = 0;   ///< Sample position within block
    uint8_t data[3] {};     ///< Raw MIDI bytes (status, data1, data2)
    int numBytes = 0;       ///< 1, 2, or 3

    bool isNoteOn()        const { return numBytes >= 3 && (data[0] & 0xF0) == 0x90 && data[2] > 0; }
    bool isNoteOff()       const { return numBytes >= 3 && ((data[0] & 0xF0) == 0x80 || ((data[0] & 0xF0) == 0x90 && data[2] == 0)); }
    bool isController()    const { return numBytes >= 3 && (data[0] & 0xF0) == 0xB0; }
    bool isChannelPressure() const { return numBytes >= 2 && (data[0] & 0xF0) == 0xD0; }
    bool isPitchBend()     const { return numBytes >= 3 && (data[0] & 0xF0) == 0xE0; }

    int getNoteNumber()       const { return data[1]; }
    int getVelocity()         const { return data[2]; }
    float getFloatVelocity()  const { return static_cast<float>(data[2]) / 127.0f; }
    int getChannel()          const { return (data[0] & 0x0F) + 1; }
    int getControllerNumber() const { return data[1]; }
    int getControllerValue()  const { return data[2]; }
    int getChannelPressureValue() const { return data[1]; }
};

/// Non-owning view of a MIDI event list for one block.
struct MidiEventList {
    const MidiEvent* events = nullptr;
    int numEvents = 0;
};

/// Parameter definition — describes one automatable parameter.
struct ParameterDef {
    std::string id;         ///< Unique ID: "{prefix}_{name}" e.g. "snap_filterCutoff"
    std::string name;       ///< Display name
    float minVal = 0.0f;
    float maxVal = 1.0f;
    float defaultVal = 0.0f;
    float step = 0.001f;    ///< Quantization step (0 = continuous)
    float skew = 1.0f;      ///< UI skew factor (1.0 = linear, <1 = log-like)
    bool isChoice = false;  ///< If true, value is integer index
    std::vector<std::string> choices; ///< Choice labels (when isChoice = true)
};

//==============================================================================
// SynthEngine — the interface every XOlokun engine implements.
//==============================================================================
class SynthEngine
{
public:
    virtual ~SynthEngine() = default;

    //--- Lifecycle ------------------------------------------------------------

    /// Called once before audio starts. Allocate buffers, initialize state.
    virtual void prepare (double sampleRate, int maxBlockSize) = 0;

    /// Called when audio stops. Free non-essential resources.
    virtual void releaseResources() {}

    /// Reset all oscillator phases, filter state, envelopes — without reallocating.
    virtual void reset() = 0;

    //--- Audio ----------------------------------------------------------------

    /// Render audio into the buffer. Process MIDI events. Real-time safe.
    /// @param buffer  Stereo buffer (write audio here, additive to existing content).
    /// @param midi    MIDI events for this block.
    virtual void renderBlock (StereoBuffer& buffer, const MidiEventList& midi) = 0;

    //--- Coupling (Cross-Engine Modulation) -----------------------------------

    /// Return a cached output sample for coupling reads. O(1).
    /// @param channel  0=left, 1=right, 2=envelope level.
    /// @param sampleIndex  Sample offset within the last rendered block.
    virtual float getSampleForCoupling (int channel, int sampleIndex) const { (void)channel; (void)sampleIndex; return 0.0f; }

    /// Receive modulation from another engine.
    virtual void applyCouplingInput (CouplingType type, float amount,
                                     const float* sourceBuffer, int numSamples)
    { (void)type; (void)amount; (void)sourceBuffer; (void)numSamples; }

    //--- Parameters -----------------------------------------------------------

    /// Return all parameter definitions for this engine.
    virtual std::vector<ParameterDef> getParameterDefs() const = 0;

    /// Set a parameter value by ID. Called from non-audio thread.
    virtual void setParameter (const std::string& id, float value) { (void)id; (void)value; }

    /// Get a parameter value by ID.
    virtual float getParameter (const std::string& id) const { (void)id; return 0.0f; }

    //--- Identity -------------------------------------------------------------

    /// Unique engine identifier (e.g. "OddfeliX", "Obrix").
    virtual std::string getEngineId() const = 0;

    /// Accent colour for UI theming.
    virtual Colour getAccentColour() const { return { 128, 128, 128 }; }

    /// Maximum polyphony.
    virtual int getMaxVoices() const { return 1; }

    /// Currently sounding voices.
    virtual int getActiveVoiceCount() const { return 0; }
};

} // namespace xolokun
