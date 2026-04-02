#pragma once
// MinimalEngine — XOceanus SDK starting template
//
// The smallest possible engine: one mono sine voice, one pitch parameter.
// Every line here is intentional — start by reading it, then replace things.
//
// How to use this template:
//   1. Copy this directory: cp -r templates/MinimalEngine SDK/examples/MyEngine
//   2. Rename the class and files to your engine name
//   3. Add parameters in getParameterDefs(), wire them in setParameter()
//   4. Build your DSP in renderBlock()
//   5. Compile: clang++ -std=c++17 -I SDK/include -shared -fPIC -o MyEngine.dylib MyEngine.h
//   6. Validate: python3 SDK/tools/validate_engine.py MyEngine.h
//
// For a complete worked example with coupling, LFOs, macros, and all 6 Doctrines:
//   SDK/examples/HelloEngine/HelloEngine.h

#include <xoceanus/SynthEngine.h>
#include <xoceanus/EngineModule.h>
#include <cmath>
#include <array>

class MinimalEngine : public xoceanus::SynthEngine
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

    void renderBlock (xoceanus::StereoBuffer& buffer, const xoceanus::MidiEventList& midi) override
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

    std::vector<xoceanus::ParameterDef> getParameterDefs() const override
    {
        // Add one entry per parameter. Format:
        //   { "prefix_name", "Display Name", minVal, maxVal, defaultVal, step, skew }
        // skew < 1.0 gives more resolution at low values (useful for frequency/time).
        return {
            { "min_pitch", "Pitch", 20.0f, 20000.0f, 440.0f, 0.1f, 0.3f }
        };
    }

    // Called from non-audio thread when a parameter changes.
    // Every ID declared in getParameterDefs() must be handled here (D004).
    void setParameter (const std::string& id, float value) override
    {
        if (id == "min_pitch") freq = value;
    }

    float getParameter (const std::string& id) const override
    {
        if (id == "min_pitch") return freq;
        return 0.0f;
    }

    std::string getEngineId() const override { return "Minimal"; }

    xoceanus::Colour getAccentColour() const override
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

//==============================================================================
// Export — XOceanus loads this engine by finding these two C symbols.
// Change the strings to match your engine; keep the class name in sync.
//==============================================================================
XOCEANUS_EXPORT_ENGINE (MinimalEngine,
                        "Minimal",
                        "Minimal Sine Engine",
                        "min_",
                        100, 200, 150,
                        "1.0.0",
                        "XO_OX SDK Template")
