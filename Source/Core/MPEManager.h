#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <cmath>

namespace xolokun {

//==============================================================================
// Per-voice MPE expression state.
// Engines embed one of these per voice to receive per-note expression data.
// All values are normalized or in musically useful units.
//==============================================================================
struct MPEVoiceExpression
{
    int midiChannel = 0;            // MIDI channel this voice was triggered on
    float pitchBendSemitones = 0.0f; // Per-note pitch offset in semitones
    float pressure = 0.0f;           // Per-note pressure / aftertouch (0..1)
    float slide = 0.0f;             // Per-note slide / CC74 / brightness (0..1)
    bool sustainHeld = false;        // Sustain pedal state for this channel

    void reset() noexcept
    {
        midiChannel = 0;
        pitchBendSemitones = 0.0f;
        pressure = 0.0f;
        slide = 0.0f;
        sustainHeld = false;
    }
};

//==============================================================================
// MPE Zone configuration.
//==============================================================================
enum class MPEZoneLayout : int
{
    Off = 0,    // Standard MIDI (no MPE — all channels treated equally)
    Lower,      // Lower zone: master = ch1, members = ch2-16
    Upper,      // Upper zone: master = ch16, members = ch1-15
    Both        // Both zones (split keyboard)
};

//==============================================================================
// MPEManager — Central MPE state manager for XOlokun.
//
// Sits between raw MIDI input and engine voice allocation.
// Parses per-channel pitch bend, pressure, and CC74 into per-note
// expression data that engines can query by MIDI channel.
//
// Design:
//   - No allocation in processBlock() — all arrays are fixed-size.
//   - Per-channel expression stored in flat array (16 channels).
//   - Engines look up expression by voice's MIDI channel after note-on.
//   - Thread-safe zone config via atomics (UI thread writes, audio reads).
//
// Roli Seaboard / Linnstrument / Sensel workflow:
//   - Controller sends MCM (MPE Configuration Message) on connect.
//   - Each finger gets its own MIDI channel.
//   - Pitch bend on that channel = per-note pitch glide.
//   - Channel pressure = per-note aftertouch.
//   - CC74 = per-note slide (Y-axis on Seaboard).
//
//==============================================================================
class MPEManager
{
public:
    static constexpr int kNumChannels = 16;

    //-- Configuration (message thread) ----------------------------------------

    void setZoneLayout(MPEZoneLayout layout) noexcept { zoneLayout.store(static_cast<int>(layout)); }
    MPEZoneLayout getZoneLayout() const noexcept { return static_cast<MPEZoneLayout>(zoneLayout.load()); }

    void setPitchBendRange(int semitones) noexcept { pitchBendRange.store(juce::jlimit(1, 96, semitones)); }
    int getPitchBendRange() const noexcept { return pitchBendRange.load(); }

    void setMPEEnabled(bool enabled) noexcept { mpeEnabled.store(enabled ? 1 : 0); }
    bool isMPEEnabled() const noexcept { return mpeEnabled.load() != 0; }

    // Pressure and slide routing targets
    enum class ExpressionTarget : int
    {
        FilterCutoff = 0,
        Volume,
        WavetablePosition,
        FXSend,
        MacroCharacter,
        MacroMovement,
        NumTargets
    };

    void setPressureTarget(ExpressionTarget target) noexcept { pressureTarget.store(static_cast<int>(target)); }
    ExpressionTarget getPressureTarget() const noexcept { return static_cast<ExpressionTarget>(pressureTarget.load()); }

    void setSlideTarget(ExpressionTarget target) noexcept { slideTarget.store(static_cast<int>(target)); }
    ExpressionTarget getSlideTarget() const noexcept { return static_cast<ExpressionTarget>(slideTarget.load()); }

    //-- Lifecycle -------------------------------------------------------------

    void prepare(double /*sampleRate*/, int /*maxBlockSize*/)
    {
        resetAllChannels();
    }

    void reset()
    {
        resetAllChannels();
    }

    //-- Audio-thread processing -----------------------------------------------

    // Call this once per block BEFORE distributing MIDI to engines.
    // Extracts per-channel expression data from pitch bend, pressure, and CC74 messages.
    // Note-on/off messages pass through unchanged.
    void processBlock(const juce::MidiBuffer& input, juce::MidiBuffer& output)
    {
        output.clear();

        const bool mpe = isMPEEnabled();
        const float bendRange = static_cast<float>(pitchBendRange.load());

        for (const auto metadata : input)
        {
            const auto msg = metadata.getMessage();
            const int ch = msg.getChannel(); // 1-based

            if (ch >= 1 && ch <= kNumChannels)
            {
                const int idx = ch - 1;

                // --- Pitch bend → per-channel pitch offset ---
                if (msg.isPitchWheel())
                {
                    // MIDI pitch bend: 0..16383, center = 8192
                    const int bendValue = msg.getPitchWheelValue();
                    const float normalized = (static_cast<float>(bendValue) - 8192.0f) / 8192.0f; // -1..1
                    channelExpression[idx].pitchBendSemitones = normalized * bendRange;

                    if (!mpe)
                    {
                        // In non-MPE mode, apply master pitch bend to ALL channels
                        for (int i = 0; i < kNumChannels; ++i)
                            channelExpression[i].pitchBendSemitones = channelExpression[idx].pitchBendSemitones;
                    }
                    continue; // Don't pass pitch bend to engines — they read expression state
                }

                // --- Channel pressure → per-channel aftertouch ---
                if (msg.isChannelPressure())
                {
                    channelExpression[idx].pressure = static_cast<float>(msg.getChannelPressureValue()) / 127.0f;
                    if (!mpe)
                    {
                        for (int i = 0; i < kNumChannels; ++i)
                            channelExpression[i].pressure = channelExpression[idx].pressure;
                    }
                    continue;
                }

                // --- Polyphonic aftertouch → per-channel pressure (MPE mode) ---
                if (msg.isAftertouch())
                {
                    channelExpression[idx].pressure = static_cast<float>(msg.getAfterTouchValue()) / 127.0f;
                    continue;
                }

                // --- CC messages ---
                if (msg.isController())
                {
                    const int cc = msg.getControllerNumber();
                    const float value = static_cast<float>(msg.getControllerValue()) / 127.0f;

                    // CC74 (Brightness) = MPE slide / Y-axis
                    if (cc == 74)
                    {
                        channelExpression[idx].slide = value;
                        if (!mpe)
                        {
                            for (int i = 0; i < kNumChannels; ++i)
                                channelExpression[i].slide = value;
                        }
                        continue;
                    }

                    // CC64 (Sustain pedal)
                    if (cc == 64)
                    {
                        const bool held = value >= 0.5f;
                        if (mpe)
                        {
                            channelExpression[idx].sustainHeld = held;
                        }
                        else
                        {
                            for (int i = 0; i < kNumChannels; ++i)
                                channelExpression[i].sustainHeld = held;
                        }
                        // Pass sustain through to engines too (they may handle it directly)
                    }

                    // CC1 (Mod wheel) — pass through, engines may map it
                    // CC11 (Expression) — pass through
                    // All other CCs — pass through
                }
            }

            // Pass all messages through (note-on, note-off, CCs, etc.)
            output.addEvent(msg, metadata.samplePosition);
        }
    }

    //-- Expression queries (audio thread, per-voice) --------------------------

    // Engines call this after note-on to get the expression state for a channel.
    // Returns a snapshot — engine stores it in voice state and updates each block.
    const MPEVoiceExpression& getChannelExpression(int midiChannel) const noexcept
    {
        const int idx = juce::jlimit(0, kNumChannels - 1, midiChannel - 1);
        return channelExpression[idx];
    }

    // Update a voice's expression from the current channel state.
    // Call this once per block per active voice in renderBlock().
    void updateVoiceExpression(MPEVoiceExpression& voice) const noexcept
    {
        if (voice.midiChannel >= 1 && voice.midiChannel <= kNumChannels)
        {
            const auto& ch = channelExpression[voice.midiChannel - 1];
            voice.pitchBendSemitones = ch.pitchBendSemitones;
            voice.pressure = ch.pressure;
            voice.slide = ch.slide;
            voice.sustainHeld = ch.sustainHeld;
        }
    }

    //-- Master channel detection (for zone routing) ---------------------------

    bool isMasterChannel(int midiChannel) const noexcept
    {
        auto layout = getZoneLayout();
        if (layout == MPEZoneLayout::Off) return false;
        if (layout == MPEZoneLayout::Lower && midiChannel == 1) return true;
        if (layout == MPEZoneLayout::Upper && midiChannel == 16) return true;
        if (layout == MPEZoneLayout::Both && (midiChannel == 1 || midiChannel == 16)) return true;
        return false;
    }

    bool isMemberChannel(int midiChannel) const noexcept
    {
        if (!isMPEEnabled()) return false;
        return !isMasterChannel(midiChannel);
    }

private:
    void resetAllChannels()
    {
        for (auto& ch : channelExpression)
            ch.reset();
    }

    // Per-channel expression state (indexed 0-15 for channels 1-16)
    std::array<MPEVoiceExpression, kNumChannels> channelExpression;

    // Configuration (atomics for cross-thread access)
    std::atomic<int> zoneLayout { static_cast<int>(MPEZoneLayout::Off) };
    std::atomic<int> pitchBendRange { 48 };  // Default: 48 semitones (Roli Seaboard default)
    std::atomic<int> mpeEnabled { 0 };
    std::atomic<int> pressureTarget { static_cast<int>(ExpressionTarget::FilterCutoff) };
    std::atomic<int> slideTarget { static_cast<int>(ExpressionTarget::FilterCutoff) };
};

} // namespace xolokun
