#pragma once
// MinimalEngine — XOlokun SDK template
// A sine oscillator with one parameter (pitch). Start here.
//
// To build:
//   1. Copy this directory
//   2. Rename class + files to your engine name
//   3. Add your DSP in renderBlock()
//   4. Compile as shared library
//   5. Run validate-engine on your .dylib

#include <xolokun/SynthEngine.h>
#include <xolokun/EngineModule.h>
#include <cmath>
#include <array>

class MinimalEngine : public xolokun::SynthEngine
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/) override
    {
        sr = static_cast<float> (sampleRate);
        phase = 0.0f;
        freq = 440.0f;
    }

    void reset() override
    {
        phase = 0.0f;
        activeNote = -1;
    }

    void renderBlock (xolokun::StereoBuffer& buffer, const xolokun::MidiEventList& midi) override
    {
        // Process MIDI
        for (int i = 0; i < midi.numEvents; ++i)
        {
            const auto& ev = midi.events[i];
            if (ev.isNoteOn())
            {
                activeNote = ev.getNoteNumber();
                velocity = ev.getFloatVelocity();
                freq = 440.0f * std::pow (2.0f, (static_cast<float> (activeNote) - 69.0f) / 12.0f);
            }
            else if (ev.isNoteOff() && ev.getNoteNumber() == activeNote)
            {
                activeNote = -1;
            }
        }

        // Render sine wave
        if (activeNote < 0 || !buffer.left || !buffer.right) return;

        float phaseInc = freq / sr;
        for (int s = 0; s < buffer.numSamples; ++s)
        {
            float sample = std::sin (phase * 6.28318530718f) * velocity * 0.5f;
            buffer.left[s]  += sample;
            buffer.right[s] += sample;
            phase += phaseInc;
            if (phase >= 1.0f) phase -= 1.0f;
        }
    }

    std::vector<xolokun::ParameterDef> getParameterDefs() const override
    {
        return {
            { "min_pitch", "Pitch", 20.0f, 20000.0f, 440.0f, 0.1f, 0.3f }
        };
    }

    std::string getEngineId() const override { return "Minimal"; }

    xolokun::Colour getAccentColour() const override
    {
        return { 100, 200, 150 };
    }

    int getMaxVoices() const override { return 1; }
    int getActiveVoiceCount() const override { return (activeNote >= 0) ? 1 : 0; }

private:
    float sr = 44100.0f;
    float phase = 0.0f;
    float freq = 440.0f;
    float velocity = 0.0f;
    int activeNote = -1;
};

// Export the engine for dynamic loading
XOLOKUN_EXPORT_ENGINE(MinimalEngine, "Minimal", "Minimal Sine Engine",
                        "min_", 100, 200, 150, "1.0.0", "XO_OX SDK Template")
