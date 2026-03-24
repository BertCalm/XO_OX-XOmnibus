#pragma once

#include <array>
#include <algorithm>
#include <limits>
#include "OcelotVoice.h"
#include "OcelotParamSnapshot.h"
#include "../../DSP/FastMath.h"

namespace xocelot {

// OcelotVoicePool — 8-voice polyphonic pool with quietest-voice stealing.
//
// Stealing strategy: when all voices are active, steal the voice with the
// smallest lastAmplitude (the quietest one). This preserves the loud, ringing
// voices while silently reusing decayed ones — correct for textural/percussive content.
//
// Thread safety: all methods called from the audio thread only.
// Biome changes are signalled from the message thread via setBiomeTarget()
// which must be called via JUCE's asyncUpdater pattern in the processor.

class OcelotVoicePool
{
public:
    static constexpr int kMaxVoices = 8;

    void prepare(double sampleRate)
    {
        lastSampleRate = sampleRate;
        for (auto& v : voices)
            v.prepare(sampleRate);
    }

    // noteOn: allocate or steal a voice. Returns the voice index used.
    int noteOn(int note, float velocity, const OcelotParamSnapshot& snap)
    {
        // 1. Try to find a free (inactive) voice
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (!voices[i].isActive())
            {
                voices[i].noteOn(note, velocity, snap);
                return i;
            }
        }

        // 2. All voices active — steal the quietest
        int   stealIdx = 0;
        float minAmp   = std::numeric_limits<float>::max();
        for (int i = 0; i < kMaxVoices; ++i)
        {
            float amp = voices[i].getLastAmplitude();
            if (amp < minAmp)
            {
                minAmp   = amp;
                stealIdx = i;
            }
        }
        voices[stealIdx].noteOff();
        voices[stealIdx].noteOn(note, velocity, snap);
        return stealIdx;
    }

    // noteOff: silence the first voice matching this note number.
    void noteOff(int note)
    {
        for (auto& v : voices)
        {
            if (v.getNoteNumber() == note && v.isActive())
            {
                v.noteOff();
                return; // only release one voice per note
            }
        }
    }

    // allNotesOff: release all active voices (e.g., transport stop, panic).
    void allNotesOff()
    {
        for (auto& v : voices)
            if (v.isActive()) v.noteOff();
    }

    // reset: hard-reset all voices — clear all state, filter memory, delay lines.
    // Called by adapter's reset() method.
    void reset()
    {
        allNotesOff();
        for (auto& v : voices)
            v.prepare(lastSampleRate);
    }

    // setBiomeTarget: called from message thread when biome param changes.
    // Each voice smoothly crossfades to the new biome independently.
    void setBiomeTarget(int biomeIndex)
    {
        for (auto& v : voices)
            v.setBiomeTarget(biomeIndex);
    }

    // renderBlock: accumulate all active voices into outL/outR.
    // Caller must clear outL/outR before calling.
    // Returns the post-mix mono RMS (for coupling getSampleForCoupling).
    float renderBlock(float* outL, float* outR, int numSamples,
                      const OcelotParamSnapshot& snap)
    {
        float totalSumSq = 0.0f;

        for (auto& v : voices)
        {
            if (v.isActive())
            {
                float amp = v.renderBlock(outL, outR, numSamples, snap);
                totalSumSq += amp * amp;
            }
        }

        // Soft-clip the mix to prevent hard distortion when multiple voices sum
        for (int i = 0; i < numSamples; ++i)
        {
            outL[i] = xolokun::fastTanh(outL[i]);
            outR[i] = xolokun::fastTanh(outR[i]);
        }

        return std::sqrt(totalSumSq / static_cast<float>(kMaxVoices));
    }

    int  activeVoiceCount() const
    {
        int n = 0;
        for (const auto& v : voices) if (v.isActive()) ++n;
        return n;
    }

private:
    double lastSampleRate = 44100.0;
    std::array<OcelotVoice, kMaxVoices> voices;
};

} // namespace xocelot
