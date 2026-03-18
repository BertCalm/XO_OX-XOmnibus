#pragma once
#include <cmath>
#include <algorithm>
#include <cstring>

namespace xoverworld {

//==============================================================================
// GlitchEngine — Sample-domain glitch artefact generator
//
// Models digital glitch types found in retro chip hardware:
//   Type 0 — Stutter (buffer freeze / loop)
//   Type 1 — Bit-shift scramble (register read corruption)
//   Type 2 — Buffer reversal (DMA read-back error)
//   Type 3 — Sample-rate aliasing burst (clock mismatch artefact)
//
// Parameters:
//   amount  [0,1] — probability / intensity of glitch trigger per sample
//   type    [0,3] — glitch algorithm select
//   rate    [0,1] — how often glitch events are re-triggered (normalised)
//   depth   [0,1] — severity of the effect when active
//   mix     [0,1] — wet/dry blend (0 = full dry, 1 = full wet)
//==============================================================================
class GlitchEngine
{
public:
    static constexpr int kBufLen = 4096;

    GlitchEngine()
    {
        std::memset(buf, 0, sizeof(buf));
    }

    void prepare(float sampleRate)
    {
        sr = sampleRate;
        writePos = 0;
        readPos  = 0;
        glitchCounter = 0;
        active = false;
        std::memset(buf, 0, sizeof(buf));
    }

    void setAmount(float a)  { amount = std::max(0.0f, std::min(1.0f, a)); }
    void setType  (int   t)  { glitchType = std::max(0, std::min(3, t)); }
    void setRate  (float r)  { rate  = std::max(0.0f, std::min(1.0f, r)); }
    void setDepth (float d)  { depth = std::max(0.0f, std::min(1.0f, d)); }
    void setMix   (float m)  { mix   = std::max(0.0f, std::min(1.0f, m)); }

    float process(float x)
    {
        // Write into circular buffer
        buf[writePos] = x;
        writePos = (writePos + 1) & (kBufLen - 1);

        // Decide whether to trigger / sustain a glitch event
        // Rate controls the period between new glitch events (in samples)
        const float periodSamples = sr * (0.02f + (1.0f - rate) * 1.98f); // 20ms–2s
        ++glitchCounter;
        if (!active && glitchCounter >= static_cast<int>(periodSamples))
        {
            // Trigger new glitch with probability proportional to amount
            // Use a lightweight deterministic pseudo-random (xorshift)
            rng ^= rng << 13;
            rng ^= rng >> 17;
            rng ^= rng << 5;
            float prob = static_cast<float>(rng & 0xFFFF) / 65535.0f;
            if (prob < amount)
            {
                active = true;
                glitchLen = static_cast<int>(sr * 0.005f + depth * sr * 0.1f); // 5–100ms
                glitchSampleCount = 0;

                // Freeze read position at a past point
                int offset = static_cast<int>(depth * kBufLen * 0.5f);
                readPos = (writePos - 1 - offset + kBufLen) & (kBufLen - 1);
            }
            glitchCounter = 0;
        }

        float wet = x;

        if (active)
        {
            switch (glitchType)
            {
                case 0: // Stutter — loop current readPos
                    wet = buf[readPos];
                    break;

                case 1: // Bit-shift scramble — XOR with shifted version
                {
                    int idx = (readPos + 1) & (kBufLen - 1);
                    wet = buf[readPos] + buf[idx] * depth * 0.5f;
                    readPos = (readPos + 1) & (kBufLen - 1);
                    break;
                }

                case 2: // Buffer reversal — walk backwards
                    wet = buf[readPos];
                    readPos = (readPos - 1 + kBufLen) & (kBufLen - 1);
                    break;

                case 3: // Aliasing burst — skip every N samples
                {
                    int skip = 1 + static_cast<int>(depth * 3.0f);
                    wet = buf[readPos];
                    readPos = (readPos + skip) & (kBufLen - 1);
                    break;
                }

                default:
                    wet = buf[readPos];
                    break;
            }

            ++glitchSampleCount;
            if (glitchSampleCount >= glitchLen)
                active = false;
        }

        return x + (wet - x) * mix;
    }

private:
    float sr        = 44100.0f;
    float amount    = 0.0f;
    int   glitchType = 0;
    float rate      = 0.5f;
    float depth     = 0.5f;
    float mix       = 0.0f;

    float buf[kBufLen];
    int   writePos  = 0;
    int   readPos   = 0;

    bool  active    = false;
    int   glitchLen = 0;
    int   glitchSampleCount = 0;
    int   glitchCounter = 0;

    // Xorshift32 PRNG (deterministic, cheap)
    unsigned int rng = 2463534242u;
};

} // namespace xoverworld
