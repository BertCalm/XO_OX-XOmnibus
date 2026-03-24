#pragma once
#include <cmath>
#include <algorithm>

/// OxytocinMemory — global (not per-voice) session memory accumulator.
///
/// Maintains running averages of I, P, C across the session.  When voices
/// are sounding the memory "records" the current love state.  When silent
/// the memory decays exponentially toward zero at the memory_decay rate.
///
/// The memory is fed back to the engine: next block's effective I/P/C values
/// get a gentle boost proportional to memory_depth.
///
/// Design note (Architect C3): ONE global instance, not per-voice.

class OxytocinMemory
{
public:
    OxytocinMemory() = default;

    void reset() noexcept
    {
        memI = memP = memC = 0.0f;
    }

    /// Update the memory accumulator once per block.
    ///
    /// \param avgI,avgP,avgC  Average effective I/P/C across active voices
    ///                        (pass 0,0,0 if no voices active)
    /// \param anyVoiceActive  True if at least one voice is sounding
    /// \param memoryDepth     0 = disabled, 1 = full accumulation
    /// \param memoryDecay     Decay time in seconds
    /// \param blockTime       Seconds per block (numSamples / sampleRate)
    void update (float avgI, float avgP, float avgC,
                 bool  anyVoiceActive,
                 float memoryDepth,
                 float memoryDecay,
                 float blockTime) noexcept
    {
        if (memoryDepth <= 0.0f)
        {
            // Studio mode — no memory
            memI = memP = memC = 0.0f;
            return;
        }

        const float decaySec = std::max (0.1f, memoryDecay);

        if (anyVoiceActive)
        {
            // Record: blend toward current love state
            // rate = memoryDepth * blockTime / memoryDecay
            float learnRate = memoryDepth * blockTime / decaySec;
            learnRate = std::min (learnRate, 1.0f);

            memI += (avgI - memI) * learnRate;
            memP += (avgP - memP) * learnRate;
            memC += (avgC - memC) * learnRate;
        }
        else
        {
            // Decay toward zero
            float coeff = std::exp (-blockTime / decaySec);
            memI *= coeff;
            memP *= coeff;
            memC *= coeff;
        }

        // Clamp
        memI = std::clamp (memI, 0.0f, 1.0f);
        memP = std::clamp (memP, 0.0f, 1.0f);
        memC = std::clamp (memC, 0.0f, 1.0f);
    }

    /// Compute boosted I/P/C values incorporating memory.
    /// Returns values clamped to [0..1].
    void applyBoost (float  inI,  float  inP,  float  inC,
                     float  memoryDepth,
                     float& outI, float& outP, float& outC) const noexcept
    {
        const float boost = memoryDepth * 0.5f;
        outI = std::clamp (inI + memI * boost, 0.0f, 1.0f);
        outP = std::clamp (inP + memP * boost, 0.0f, 1.0f);
        outC = std::clamp (inC + memC * boost, 0.0f, 1.0f);
    }

    float getMemI() const noexcept { return memI; }
    float getMemP() const noexcept { return memP; }
    float getMemC() const noexcept { return memC; }

private:
    float memI = 0.0f;
    float memP = 0.0f;
    float memC = 0.0f;
};
