#pragma once
#include "../../Core/SynthEngine.h"
#include "../../Core/AudioRingBuffer.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/FastMath.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cmath>
#include <cstdint>

namespace xomnibus {

//==============================================================================
// OPAL CONSTANTS
//==============================================================================

static constexpr int   kOpalMaxClouds       = 12;
static constexpr int   kOpalMaxGrains       = 32;
static constexpr float kOpalBufferSeconds   = 4.0f;
static constexpr int   kOpalModSlots        = 6;

//==============================================================================
// OPAL PARAMETER IDs — 86 frozen IDs, opal_ prefix
//==============================================================================

namespace OpalParam {
    // Grain Source (1-6)
    inline constexpr const char* SOURCE          = "opal_source";
    inline constexpr const char* OSC_SHAPE       = "opal_oscShape";
    inline constexpr const char* OSC2_SHAPE      = "opal_osc2Shape";
    inline constexpr const char* OSC2_MIX        = "opal_osc2Mix";
    inline constexpr const char* OSC2_DETUNE     = "opal_osc2Detune";
    inline constexpr const char* COUPLING_LEVEL  = "opal_couplingLevel";
    inline constexpr const char* EXTERNAL_MIX    = "opal_externalMix";  // 0=internal only, 1=external AudioToBuffer only
    // Grain Scheduler (7-14)
    inline constexpr const char* GRAIN_SIZE      = "opal_grainSize";
    inline constexpr const char* DENSITY         = "opal_density";
    inline constexpr const char* POSITION        = "opal_position";
    inline constexpr const char* POS_SCATTER     = "opal_posScatter";
    inline constexpr const char* PITCH_SHIFT     = "opal_pitchShift";
    inline constexpr const char* PITCH_SCATTER   = "opal_pitchScatter";
    inline constexpr const char* PAN_SCATTER     = "opal_panScatter";
    inline constexpr const char* WINDOW          = "opal_window";
    // Freeze (15-16)
    inline constexpr const char* FREEZE          = "opal_freeze";
    inline constexpr const char* FREEZE_SIZE     = "opal_freezeSize";
    // Filter (17-21)
    inline constexpr const char* FILTER_CUTOFF   = "opal_filterCutoff";
    inline constexpr const char* FILTER_RESO     = "opal_filterReso";
    inline constexpr const char* FILTER_MODE     = "opal_filterMode";
    inline constexpr const char* FILTER_KEY_TRACK = "opal_filterKeyTrack";
    inline constexpr const char* FILTER_DRIVE    = "opal_filterDrive";
    // Character (22-23)
    inline constexpr const char* SHIMMER         = "opal_shimmer";
    inline constexpr const char* FROST           = "opal_frost";
    // Amp Envelope (24-28)
    inline constexpr const char* AMP_ATTACK      = "opal_ampAttack";
    inline constexpr const char* AMP_DECAY       = "opal_ampDecay";
    inline constexpr const char* AMP_SUSTAIN     = "opal_ampSustain";
    inline constexpr const char* AMP_RELEASE     = "opal_ampRelease";
    inline constexpr const char* AMP_VEL_SENS    = "opal_ampVelSens";
    // Filter Envelope (29-33)
    inline constexpr const char* FILTER_ENV_AMT  = "opal_filterEnvAmt";
    inline constexpr const char* FILTER_ATTACK   = "opal_filterAttack";
    inline constexpr const char* FILTER_DECAY    = "opal_filterDecay";
    inline constexpr const char* FILTER_SUSTAIN  = "opal_filterSustain";
    inline constexpr const char* FILTER_RELEASE  = "opal_filterRelease";
    // LFO 1 (34-39)
    inline constexpr const char* LFO1_SHAPE      = "opal_lfo1Shape";
    inline constexpr const char* LFO1_RATE       = "opal_lfo1Rate";
    inline constexpr const char* LFO1_DEPTH      = "opal_lfo1Depth";
    inline constexpr const char* LFO1_SYNC       = "opal_lfo1Sync";
    inline constexpr const char* LFO1_RETRIGGER  = "opal_lfo1Retrigger";
    inline constexpr const char* LFO1_PHASE      = "opal_lfo1Phase";
    // LFO 2 (40-45)
    inline constexpr const char* LFO2_SHAPE      = "opal_lfo2Shape";
    inline constexpr const char* LFO2_RATE       = "opal_lfo2Rate";
    inline constexpr const char* LFO2_DEPTH      = "opal_lfo2Depth";
    inline constexpr const char* LFO2_SYNC       = "opal_lfo2Sync";
    inline constexpr const char* LFO2_RETRIGGER  = "opal_lfo2Retrigger";
    inline constexpr const char* LFO2_PHASE      = "opal_lfo2Phase";
    // Mod Matrix 6 slots (46-63)
    inline constexpr const char* MOD1_SRC = "opal_modSlot1Src";
    inline constexpr const char* MOD1_DST = "opal_modSlot1Dst";
    inline constexpr const char* MOD1_AMT = "opal_modSlot1Amt";
    inline constexpr const char* MOD2_SRC = "opal_modSlot2Src";
    inline constexpr const char* MOD2_DST = "opal_modSlot2Dst";
    inline constexpr const char* MOD2_AMT = "opal_modSlot2Amt";
    inline constexpr const char* MOD3_SRC = "opal_modSlot3Src";
    inline constexpr const char* MOD3_DST = "opal_modSlot3Dst";
    inline constexpr const char* MOD3_AMT = "opal_modSlot3Amt";
    inline constexpr const char* MOD4_SRC = "opal_modSlot4Src";
    inline constexpr const char* MOD4_DST = "opal_modSlot4Dst";
    inline constexpr const char* MOD4_AMT = "opal_modSlot4Amt";
    inline constexpr const char* MOD5_SRC = "opal_modSlot5Src";
    inline constexpr const char* MOD5_DST = "opal_modSlot5Dst";
    inline constexpr const char* MOD5_AMT = "opal_modSlot5Amt";
    inline constexpr const char* MOD6_SRC = "opal_modSlot6Src";
    inline constexpr const char* MOD6_DST = "opal_modSlot6Dst";
    inline constexpr const char* MOD6_AMT = "opal_modSlot6Amt";
    // Macros (64-67)
    inline constexpr const char* MACRO_SCATTER   = "opal_macroScatter";
    inline constexpr const char* MACRO_DRIFT     = "opal_macroDrift";
    inline constexpr const char* MACRO_COUPLING  = "opal_macroCoupling";
    inline constexpr const char* MACRO_SPACE     = "opal_macroSpace";
    // FX — Smear (68-69)
    inline constexpr const char* FX_SMEAR_AMT    = "opal_fxSmearAmount";
    inline constexpr const char* FX_SMEAR_MIX    = "opal_fxSmearMix";
    // FX — Scatter Reverb (70-73)
    inline constexpr const char* FX_REVERB_SIZE  = "opal_fxReverbSize";
    inline constexpr const char* FX_REVERB_DECAY = "opal_fxReverbDecay";
    inline constexpr const char* FX_REVERB_DAMP  = "opal_fxReverbDamping";
    inline constexpr const char* FX_REVERB_MIX   = "opal_fxReverbMix";
    // FX — Delay (74-78)
    inline constexpr const char* FX_DELAY_TIME   = "opal_fxDelayTime";
    inline constexpr const char* FX_DELAY_FB     = "opal_fxDelayFeedback";
    inline constexpr const char* FX_DELAY_MIX    = "opal_fxDelayMix";
    inline constexpr const char* FX_DELAY_SYNC   = "opal_fxDelaySync";
    inline constexpr const char* FX_DELAY_SPREAD = "opal_fxDelaySpread";
    // FX — Finish (79-81)
    inline constexpr const char* FX_FINISH_GLUE  = "opal_fxFinishGlue";
    inline constexpr const char* FX_FINISH_WIDTH = "opal_fxFinishWidth";
    inline constexpr const char* FX_FINISH_LEVEL = "opal_fxFinishLevel";
    // Voice (82-86)
    inline constexpr const char* VOICE_MODE      = "opal_voiceMode";
    inline constexpr const char* GLIDE_TIME      = "opal_glideTime";
    inline constexpr const char* GLIDE_MODE      = "opal_glideMode";
    inline constexpr const char* PAN             = "opal_pan";
    inline constexpr const char* LEVEL           = "opal_level";
} // namespace OpalParam

//==============================================================================
// OpalPRNG — xorshift32 deterministic PRNG for RT-safe grain randomization.
// Seeded per-voice from voice index + note number for reproducible patterns.
//==============================================================================
class OpalPRNG
{
public:
    void seed (uint32_t s) noexcept { state = s ? s : 1; }

    float next() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float> (static_cast<int32_t> (state)) / 2147483648.0f;
    }

    // Returns value in [lo, hi]
    float nextRange (float lo, float hi) noexcept
    {
        return lo + (next() + 1.0f) * 0.5f * (hi - lo);
    }

private:
    uint32_t state = 1;
};

//==============================================================================
// Window functions for grain envelopes.
//   0: Hann — smooth, no clicks
//   1: Gaussian — softer edges, overlaps well
//   2: Tukey — flat top, tapered edges, preserves transients
//   3: Rectangular — hard edges, deliberate clicks
//==============================================================================
inline float opalWindow (float phase, int shape) noexcept
{
    constexpr float pi = 3.14159265358979323846f;

    switch (shape)
    {
        case 0: // Hann
            return 0.5f * (1.0f - std::cos (2.0f * pi * phase));

        case 1: // Gaussian (sigma=0.15)
        {
            float x = (phase - 0.5f) / 0.15f;
            return fastExp (-0.5f * x * x);
        }

        case 2: // Tukey (alpha=0.5 — 50% taper)
        {
            constexpr float alpha = 0.5f;
            if (phase < alpha * 0.5f)
                return 0.5f * (1.0f - std::cos (2.0f * pi * phase / alpha));
            if (phase > 1.0f - alpha * 0.5f)
                return 0.5f * (1.0f - std::cos (2.0f * pi * (1.0f - phase) / alpha));
            return 1.0f;
        }

        case 3: // Rectangular
            return 1.0f;

        default:
            return 0.5f * (1.0f - std::cos (2.0f * pi * phase));
    }
}

//==============================================================================
// OpalGrainBuffer — 4-second mono ring buffer for grain source audio.
// Write head advances continuously; freeze stops writing.
//==============================================================================
class OpalGrainBuffer
{
public:
    void prepare (double sampleRate) noexcept
    {
        bufferSize = static_cast<int> (sampleRate * kOpalBufferSeconds);
        if (bufferSize > kMaxBufferSize) bufferSize = kMaxBufferSize;
        clear();
    }

    void clear() noexcept
    {
        for (int i = 0; i < kMaxBufferSize; ++i)
            buffer[i] = 0.0f;
        writeHead = 0;
    }

    void write (float sample, bool frozen) noexcept
    {
        if (!frozen)
        {
            buffer[writeHead] = sample;
            writeHead = (writeHead + 1) % bufferSize;
        }
    }

    float readInterpolated (float position) const noexcept
    {
        // position is in samples (fractional)
        while (position < 0.0f)
            position += static_cast<float> (bufferSize);

        int iLo = static_cast<int> (position) % bufferSize;
        int iHi = (iLo + 1) % bufferSize;
        float frac = position - std::floor (position);
        return buffer[iLo] + frac * (buffer[iHi] - buffer[iLo]);
    }

    int getWriteHead() const noexcept { return writeHead; }
    int getBufferSize() const noexcept { return bufferSize; }

private:
    // Max buffer: 4 seconds at 192kHz
    static constexpr int kMaxBufferSize = 192000 * 4;
    float buffer[kMaxBufferSize] = {};
    int writeHead = 0;
    int bufferSize = 176400; // default for 44.1k
};

//==============================================================================
// OpalADSR — Lightweight ADSR envelope for amp and filter modulation.
//==============================================================================
class OpalADSR
{
public:
    enum Stage { Off, Attack, Decay, Sustain, Release };

    void noteOn (float attack, float decay, float sustain, float release,
                 double sampleRate) noexcept
    {
        sr = sampleRate;
        sustainLevel = sustain;
        releaseTime = release;
        stage = Attack;
        if (attack < 0.001f)
        {
            level = 1.0f;
            setDecay (decay);
        }
        else
        {
            attackRate = 1.0f / (static_cast<float> (sr) * attack);
        }
        decayTime = decay;
    }

    void noteOff() noexcept
    {
        if (stage != Off)
        {
            stage = Release;
            if (releaseTime < 0.001f)
                releaseRate = 100.0f;
            else
                releaseRate = level / (static_cast<float> (sr) * releaseTime);
        }
    }

    float process() noexcept
    {
        switch (stage)
        {
            case Attack:
                level += attackRate;
                if (level >= 1.0f)
                {
                    level = 1.0f;
                    setDecay (decayTime);
                }
                break;

            case Decay:
                level -= decayRate;
                if (level <= sustainLevel)
                {
                    level = sustainLevel;
                    stage = Sustain;
                }
                break;

            case Sustain:
                level = sustainLevel;
                break;

            case Release:
                level -= releaseRate;
                if (level <= 0.0f)
                {
                    level = 0.0f;
                    stage = Off;
                }
                break;

            case Off:
                level = 0.0f;
                break;
        }
        return flushDenormal (level);
    }

    bool isActive() const noexcept { return stage != Off; }
    float getLevel() const noexcept { return level; }
    Stage getStage() const noexcept { return stage; }

    void reset() noexcept
    {
        level = 0.0f;
        stage = Off;
    }

    // Retrigger from current level (for legato filter envelope)
    void retriggerFromCurrent (float attack, float decay, float sustain, float release,
                               double sampleRate) noexcept
    {
        sr = sampleRate;
        sustainLevel = sustain;
        releaseTime = release;
        decayTime = decay;
        stage = Attack;
        if (attack < 0.001f)
        {
            level = std::max (level, 0.5f);
            setDecay (decay);
        }
        else
        {
            attackRate = (1.0f - level) / (static_cast<float> (sr) * attack);
        }
    }

private:
    void setDecay (float decay) noexcept
    {
        stage = Decay;
        if (decay < 0.001f)
            decayRate = 100.0f;
        else
            decayRate = (1.0f - sustainLevel) / (static_cast<float> (sr) * decay);
    }

    double sr = 44100.0;
    float level = 0.0f;
    float attackRate = 0.0f;
    float decayRate = 0.0f;
    float decayTime = 0.5f;
    float sustainLevel = 0.8f;
    float releaseRate = 0.0f;
    float releaseTime = 1.0f;
    Stage stage = Off;
};

//==============================================================================
// OpalLFO — 6-shape LFO with retrigger and phase control.
//   0: Sine, 1: Triangle, 2: Saw (down), 3: Square, 4: Random (S&H), 5: Stepped
//==============================================================================
class OpalLFO
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; }

    void retrigger (float startPhase) noexcept
    {
        phase = startPhase;
        randomValue = 0.0f;
    }

    void reset() noexcept
    {
        phase = 0.0f;
        randomValue = 0.0f;
    }

    // Returns bipolar [-1, 1]
    float process (float rateHz, int shape, OpalPRNG& rng) noexcept
    {
        float inc = rateHz / static_cast<float> (sr);
        float prevPhase = phase;
        phase += inc;

        bool wrapped = false;
        if (phase >= 1.0f)
        {
            phase -= 1.0f;
            wrapped = true;
        }

        float out = 0.0f;
        switch (shape)
        {
            case 0: // Sine
                out = fastSin (phase * 6.28318530718f);
                break;
            case 1: // Triangle
                out = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
                break;
            case 2: // Saw (down)
                out = 1.0f - 2.0f * phase;
                break;
            case 3: // Square
                out = (phase < 0.5f) ? 1.0f : -1.0f;
                break;
            case 4: // Random (S&H) — new value on wrap
                if (wrapped) randomValue = rng.next();
                out = randomValue;
                break;
            case 5: // Stepped — quantized sine
            {
                float raw = fastSin (phase * 6.28318530718f);
                out = std::round (raw * 4.0f) * 0.25f;
                break;
            }
        }
        return out;
    }

private:
    double sr = 44100.0;
    float phase = 0.0f;
    float randomValue = 0.0f;
};

//==============================================================================
// OpalGrain — A single grain: reads from the grain buffer with pitch shift,
// window envelope, and stereo pan. Pre-allocated, no dynamic memory.
//==============================================================================
struct OpalGrain
{
    float readPosition  = 0.0f;   // Current read position in buffer (samples, fractional)
    float readIncrement = 1.0f;   // Playback rate (pitch shift: 2^(semitones/12))
    float windowPhase   = 0.0f;   // 0.0 → 1.0 over grain lifetime
    float windowInc     = 0.001f; // 1.0 / grainSizeSamples
    float panL          = 0.707f; // Equal-power pan left coefficient
    float panR          = 0.707f; // Equal-power pan right coefficient
    int   windowShape   = 0;      // Hann/Gaussian/Tukey/Rect
    int   ownerVoice    = -1;     // Which cloud voice spawned this grain
    bool  active        = false;
    uint32_t birthOrder = 0;      // For LRU stealing

    float process (const OpalGrainBuffer& buf) noexcept
    {
        if (!active) return 0.0f;

        float sample = buf.readInterpolated (readPosition);
        float window = opalWindow (windowPhase, windowShape);
        sample *= window;

        readPosition += readIncrement;
        windowPhase  += windowInc;

        if (windowPhase >= 1.0f)
            active = false;

        return sample;
    }
};

//==============================================================================
// OpalGrainPool — Fixed pool of 32 grains shared across all cloud voices.
//==============================================================================
class OpalGrainPool
{
public:
    OpalGrain& spawn (int voiceIdx, float readPos, float pitchSemitones,
                      float grainSizeMs, float panPos, int windowShape,
                      double sampleRate) noexcept
    {
        int idx = findFreeSlot (voiceIdx);
        auto& g = grains[idx];
        g.active = true;
        g.ownerVoice = voiceIdx;
        g.readPosition = readPos;
        g.readIncrement = std::pow (2.0f, pitchSemitones / 12.0f);

        float sizeSamples = grainSizeMs * 0.001f * static_cast<float> (sampleRate);
        if (sizeSamples < 1.0f) sizeSamples = 1.0f;
        g.windowPhase = 0.0f;
        g.windowInc = 1.0f / sizeSamples;
        g.windowShape = windowShape;

        // Equal-power pan
        constexpr float pi4 = 0.78539816339745f; // pi/4
        float p = clamp (panPos, -1.0f, 1.0f);
        g.panL = std::cos ((p + 1.0f) * 0.5f * pi4 * 2.0f);
        g.panR = std::sin ((p + 1.0f) * 0.5f * pi4 * 2.0f);

        g.birthOrder = ++globalBirthCounter;
        return g;
    }

    void processAll (const OpalGrainBuffer& buf, float* outL, float* outR,
                     int numSamples) noexcept
    {
        for (int n = 0; n < numSamples; ++n)
        {
            float sumL = 0.0f, sumR = 0.0f;
            int count = 0;

            for (auto& g : grains)
            {
                if (!g.active) continue;
                float s = g.process (buf);
                sumL += s * g.panL;
                sumR += s * g.panR;
                ++count;
            }

            // Energy-preserving normalization
            if (count > 0)
            {
                float norm = 1.0f / std::sqrt (static_cast<float> (count));
                sumL *= norm;
                sumR *= norm;
            }

            outL[n] += sumL;
            outR[n] += sumR;
        }
    }

    void killVoiceGrains (int voiceIdx) noexcept
    {
        for (auto& g : grains)
            if (g.ownerVoice == voiceIdx)
                g.active = false;
    }

    void killAll() noexcept
    {
        for (auto& g : grains)
            g.active = false;
    }

    int countActiveForVoice (int voiceIdx) const noexcept
    {
        int c = 0;
        for (const auto& g : grains)
            if (g.active && g.ownerVoice == voiceIdx)
                ++c;
        return c;
    }

    int countActive() const noexcept
    {
        int c = 0;
        for (const auto& g : grains)
            if (g.active) ++c;
        return c;
    }

private:
    int findFreeSlot (int voiceIdx) noexcept
    {
        // Prefer free slot
        for (int i = 0; i < kOpalMaxGrains; ++i)
            if (!grains[i].active) return i;

        // Steal oldest grain belonging to this voice
        int oldest = -1;
        uint32_t oldestBirth = UINT32_MAX;
        for (int i = 0; i < kOpalMaxGrains; ++i)
        {
            if (grains[i].ownerVoice == voiceIdx && grains[i].birthOrder < oldestBirth)
            {
                oldest = i;
                oldestBirth = grains[i].birthOrder;
            }
        }
        if (oldest >= 0) return oldest;

        // Steal globally oldest
        oldest = 0;
        oldestBirth = grains[0].birthOrder;
        for (int i = 1; i < kOpalMaxGrains; ++i)
        {
            if (grains[i].birthOrder < oldestBirth)
            {
                oldest = i;
                oldestBirth = grains[i].birthOrder;
            }
        }
        return oldest;
    }

    std::array<OpalGrain, kOpalMaxGrains> grains {};
    uint32_t globalBirthCounter = 0;
};

//==============================================================================
// OpalCloudVoice — Per-MIDI-note voice: owns a grain scheduler, envelopes,
// filter, and character processing. 12 of these exist (pre-allocated).
//==============================================================================
struct OpalCloudVoice
{
    bool  active         = false;
    int   noteNumber     = -1;
    float velocity       = 1.0f;
    int   voiceIndex     = 0;
    float frequency      = 261.63f;    // MIDI note frequency
    float glideFreq      = 261.63f;    // Current glide frequency
    float glideTarget    = 261.63f;    // Target glide frequency
    float glideRate      = 0.0f;       // Samples to reach target
    bool  hasPlayedBefore = false;

    // Grain scheduling state
    float triggerAccum   = 0.0f;
    OpalPRNG rng;

    // Envelopes
    OpalADSR ampEnv;
    OpalADSR filterEnv;

    // Per-voice filter (Cytomic SVF)
    CytomicSVF filterL;
    CytomicSVF filterR;

    // LFOs (per-voice for retrigger behavior)
    OpalLFO lfo1;
    OpalLFO lfo2;

    // Oscillator for grain source
    PolyBLEP osc1;
    PolyBLEP osc2;

    // Age counter for voice stealing
    uint32_t birthOrder = 0;

    void noteOn (int note, float vel, double sr, float attack, float decay,
                 float sustain, float release, float filterA, float filterD,
                 float filterS, float filterR_,
                 float prevFreq, float glideTime, bool legatoActive,
                 bool lfo1Retrig, float lfo1StartPhase,
                 bool lfo2Retrig, float lfo2StartPhase) noexcept
    {
        active = true;
        noteNumber = note;
        velocity = vel;
        frequency = midiToFreq (note);

        if (glideTime > 0.001f && hasPlayedBefore)
        {
            glideTarget = frequency;
            glideFreq = legatoActive ? glideFreq : prevFreq;
            glideRate = 1.0f / (static_cast<float> (sr) * glideTime);
        }
        else
        {
            glideFreq = frequency;
            glideTarget = frequency;
            glideRate = 0.0f;
        }

        if (legatoActive)
        {
            // Legato: don't retrigger amp, retrigger filter from current
            filterEnv.retriggerFromCurrent (filterA, filterD, filterS, filterR_, sr);
        }
        else
        {
            ampEnv.noteOn (attack, decay, sustain, release, sr);
            filterEnv.noteOn (filterA, filterD, filterS, filterR_, sr);
            triggerAccum = 1000.0f; // Force first grain immediately
            filterL.reset();
            filterR.reset();
        }

        // LFO retrigger on noteOn (per-voice phase reset)
        if (lfo1Retrig) lfo1.retrigger (lfo1StartPhase);
        if (lfo2Retrig) lfo2.retrigger (lfo2StartPhase);

        rng.seed (static_cast<uint32_t> (note * 137 + voiceIndex * 31 + 42));
        osc1.reset();
        osc2.reset();
        hasPlayedBefore = true;
    }

    void noteOff() noexcept
    {
        ampEnv.noteOff();
        filterEnv.noteOff();
    }

    void kill() noexcept
    {
        active = false;
        ampEnv.reset();
        filterEnv.reset();
    }
};

//==============================================================================
// OpalScatterReverb — Schroeder-style 4-comb + 2-allpass for grain textures.
// Pre-allocated delay lines, no dynamic memory.
//==============================================================================
class OpalScatterReverb
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        for (auto& c : combL) c.clear();
        for (auto& c : combR) c.clear();
        for (auto& a : apL) a.clear();
        for (auto& a : apR) a.clear();
    }

    void setParams (float size, float decay, float damping) noexcept
    {
        // Scale comb delay times by size
        float scale = 0.5f + size * 1.5f;
        float fbBase = 0.7f + decay * 0.025f;
        if (fbBase > 0.98f) fbBase = 0.98f;

        static constexpr float combTimesMs[4] = { 29.7f, 37.1f, 41.1f, 43.7f };
        static constexpr float apTimesMs[2]   = { 5.0f, 1.7f };

        for (int i = 0; i < 4; ++i)
        {
            int samples = static_cast<int> (combTimesMs[i] * scale * 0.001f * sr);
            combL[i].setDelay (samples);
            combR[i].setDelay (samples + static_cast<int> (1.5f * (i + 1)));
            combL[i].feedback = fbBase;
            combR[i].feedback = fbBase;
            combL[i].damping = damping;
            combR[i].damping = damping;
        }
        for (int i = 0; i < 2; ++i)
        {
            int samples = static_cast<int> (apTimesMs[i] * scale * 0.001f * sr);
            apL[i].setDelay (samples);
            apR[i].setDelay (samples + 3);
        }
    }

    void process (float& inL, float& inR) noexcept
    {
        float sumL = 0.0f, sumR = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            sumL += combL[i].process (inL);
            sumR += combR[i].process (inR);
        }
        sumL *= 0.25f;
        sumR *= 0.25f;

        for (int i = 0; i < 2; ++i)
        {
            sumL = apL[i].process (sumL);
            sumR = apR[i].process (sumR);
        }

        inL = sumL;
        inR = sumR;
    }

private:
    struct CombFilter
    {
        static constexpr int kMaxDelay = 8192;
        float buffer[kMaxDelay] = {};
        int writePos = 0;
        int delaySamples = 1000;
        float feedback = 0.85f;
        float damping = 0.5f;
        float lastOut = 0.0f;

        void setDelay (int samples) noexcept
        {
            delaySamples = (samples < kMaxDelay) ? samples : kMaxDelay - 1;
            if (delaySamples < 1) delaySamples = 1;
        }

        float process (float in) noexcept
        {
            int readPos = writePos - delaySamples;
            if (readPos < 0) readPos += kMaxDelay;
            float delayed = buffer[readPos];

            // LP damping on feedback
            lastOut = flushDenormal (delayed * (1.0f - damping) + lastOut * damping);
            buffer[writePos] = in + lastOut * feedback;
            writePos = (writePos + 1) % kMaxDelay;
            return delayed;
        }

        void clear() noexcept
        {
            for (auto& s : buffer) s = 0.0f;
            writePos = 0;
            lastOut = 0.0f;
        }
    };

    struct AllpassFilter
    {
        static constexpr int kMaxDelay = 2048;
        float buffer[kMaxDelay] = {};
        int writePos = 0;
        int delaySamples = 200;
        static constexpr float gain = 0.5f;

        void setDelay (int samples) noexcept
        {
            delaySamples = (samples < kMaxDelay) ? samples : kMaxDelay - 1;
            if (delaySamples < 1) delaySamples = 1;
        }

        float process (float in) noexcept
        {
            int readPos = writePos - delaySamples;
            if (readPos < 0) readPos += kMaxDelay;
            float delayed = buffer[readPos];
            float out = -in * gain + delayed;
            buffer[writePos] = flushDenormal (in + delayed * gain);
            writePos = (writePos + 1) % kMaxDelay;
            return out;
        }

        void clear() noexcept
        {
            for (auto& s : buffer) s = 0.0f;
            writePos = 0;
        }
    };

    double sr = 44100.0;
    CombFilter combL[4], combR[4];
    AllpassFilter apL[2], apR[2];
};

//==============================================================================
// OpalStereoDelay — Simple stereo delay with feedback and spread.
//==============================================================================
class OpalStereoDelay
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        for (auto& s : bufL) s = 0.0f;
        for (auto& s : bufR) s = 0.0f;
        writePos = 0;
    }

    void process (float& inL, float& inR, float timeSec, float feedback,
                  float spread) noexcept
    {
        int delaySamplesL = static_cast<int> (timeSec * sr);
        int delaySamplesR = static_cast<int> (timeSec * (1.0f + spread * 0.3f) * sr);
        delaySamplesL = clampI (delaySamplesL, 1, kMaxDelay - 1);
        delaySamplesR = clampI (delaySamplesR, 1, kMaxDelay - 1);

        int readL = writePos - delaySamplesL;
        if (readL < 0) readL += kMaxDelay;
        int readR = writePos - delaySamplesR;
        if (readR < 0) readR += kMaxDelay;

        float delayedL = bufL[readL];
        float delayedR = bufR[readR];

        bufL[writePos] = flushDenormal (inL + delayedL * feedback);
        bufR[writePos] = flushDenormal (inR + delayedR * feedback);
        writePos = (writePos + 1) % kMaxDelay;

        inL = delayedL;
        inR = delayedR;
    }

private:
    static int clampI (int v, int lo, int hi) noexcept { return v < lo ? lo : (v > hi ? hi : v); }
    static constexpr int kMaxDelay = 192000 * 2; // 2 sec at 192k
    float bufL[kMaxDelay] = {};
    float bufR[kMaxDelay] = {};
    int writePos = 0;
    double sr = 44100.0;
};

//==============================================================================
// OpalFinish — Glue compressor + stereo width.
//==============================================================================
class OpalFinish
{
public:
    void reset() noexcept { envL = 0.0f; envR = 0.0f; }

    void process (float& L, float& R, float glue, float width) noexcept
    {
        // Soft-knee glue compressor (2:1 ratio)
        if (glue > 0.001f)
        {
            float threshold = 1.0f - glue * 0.6f;
            L = compress (L, threshold, envL);
            R = compress (R, threshold, envR);
        }

        // Stereo width (M/S)
        float mid  = (L + R) * 0.5f;
        float side = (L - R) * 0.5f;
        side *= width;
        L = mid + side;
        R = mid - side;
    }

private:
    float compress (float in, float threshold, float& env) noexcept
    {
        float absIn = std::fabs (in);
        // Envelope follower (attack 0.1ms, release 50ms approx)
        float target = absIn;
        float coeff = (target > env) ? 0.01f : 0.001f;
        env = flushDenormal (env + coeff * (target - env));

        if (env > threshold)
        {
            float excess = env - threshold;
            float gain = threshold + excess * 0.5f; // 2:1 ratio
            gain /= env;
            return in * gain;
        }
        return in;
    }

    float envL = 0.0f, envR = 0.0f;
};

//==============================================================================
// OpalEngine — XOpal granular synthesis engine for XOmnibus.
//
// Signal chain:
//   OSC / AudioToWavetable → GrainBuffer (4s ring)
//   MIDI → CloudPool (12) → GrainPool (32) → Cloud Mix
//   Cloud Mix → SVFilter → Shimmer/Frost → Amp → FX Chain → Out
//
// Coupling inputs:  AudioToWavetable, AmpToFilter, EnvToMorph, LFOToPitch,
//                   RhythmToBlend, EnvToDecay
// Coupling output:  Post-filter cloud stereo, normalized ±1
//==============================================================================
class OpalEngine : public SynthEngine
{
public:
    OpalEngine() = default;
    ~OpalEngine() = default;

    //-- Identity --------------------------------------------------------------

    juce::String getEngineId()     const override { return "Opal"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFFA78BFA); }
    int          getMaxVoices()    const override { return kOpalMaxClouds; }

    int getActiveVoiceCount() const override
    {
        int count = 0;
        for (const auto& v : voices)
            if (v.active) ++count;
        return count;
    }

    //-- Lifecycle -------------------------------------------------------------

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;

        grainBuffer.prepare (sampleRate);
        grainPool.killAll();

        for (int i = 0; i < kOpalMaxClouds; ++i)
        {
            voices[i].voiceIndex = i;
            voices[i].lfo1.prepare (sampleRate);
            voices[i].lfo2.prepare (sampleRate);
        }

        scatterReverb.prepare (sampleRate);
        stereoDelay.prepare (sampleRate);
        finish.reset();
        aftertouch.prepare (sampleRate);

        // Pre-allocate work buffers
        workBufL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        workBufR.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        couplingBufL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        couplingBufR.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheLeft.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheRight.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        for (auto& s : couplingBufL) s = 0.0f;
        for (auto& s : couplingBufR) s = 0.0f;

        // AudioToBuffer: 4 per-slot ring buffers (~186ms each at 44.1kHz).
        // Using kOpalExternalBufferSeconds (not kOpalBufferSeconds) so external
        // inputs get a smaller dedicated window separate from the 4s grain buffer.
        for (auto& rb : inputBuffers)
            rb.prepare (static_cast<int> (sampleRate), kOpalExternalBufferSeconds);

        // Per-block external audio cache (blended in renderBlock grain source path)
        extAudioBufL.assign (static_cast<size_t> (maxBlockSize), 0.0f);
        extAudioBufR.assign (static_cast<size_t> (maxBlockSize), 0.0f);
    }

    void releaseResources() override
    {
        for (auto& v : voices) v.kill();
        grainPool.killAll();
    }

    void reset() override
    {
        grainBuffer.clear();
        grainPool.killAll();
        for (auto& v : voices) v.kill();
        scatterReverb.reset();
        stereoDelay.reset();
        finish.reset();
        for (auto& s : couplingBufL) s = 0.0f;
        for (auto& s : couplingBufR) s = 0.0f;
        extFilterMod = 0.0f;
        extPosMod = 0.0f;
        extPitchMod = 0.0f;
        extDensityMod = 0.0f;
        extFreezeMod = 0.0f;
        lastSampleL = 0.0f;
        lastSampleR = 0.0f;
        for (auto& s : extAudioBufL) s = 0.0f;
        for (auto& s : extAudioBufR) s = 0.0f;
    }

    //-- Parameters ------------------------------------------------------------

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl (params);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl (params);
        return { params.begin(), params.end() };
    }

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using FloatParam  = juce::AudioParameterFloat;
        using ChoiceParam = juce::AudioParameterChoice;
        using PID = juce::ParameterID;
        using NR  = juce::NormalisableRange<float>;

        // Mod matrix source/dest arrays
        static const juce::StringArray modSources {
            "Off", "LFO1", "LFO2", "Filter Env", "Amp Env",
            "Velocity", "Key Track", "Mod Wheel"
        };
        static const juce::StringArray modDests {
            "Off", "Grain Size", "Density", "Position", "Pitch Scatter",
            "Pan Scatter", "Filter Cutoff", "Shimmer", "Frost", "Freeze",
            "Osc Shape", "Level"
        };

        //==== 1. GRAIN SOURCE ====
        params.push_back (std::make_unique<ChoiceParam> (
            PID { OpalParam::SOURCE, 1 }, "Opal Source",
            juce::StringArray { "Sine", "Saw", "Pulse", "Noise", "Two-Osc", "Coupling" }, 1));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::OSC_SHAPE, 1 }, "Opal Osc Shape",
            NR (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::OSC2_SHAPE, 1 }, "Opal Osc 2 Shape",
            NR (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::OSC2_MIX, 1 }, "Opal Osc 2 Mix",
            NR (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::OSC2_DETUNE, 1 }, "Opal Osc 2 Detune",
            NR (-24.0f, 24.0f, 0.01f), 0.1f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::COUPLING_LEVEL, 1 }, "Opal Coupling Level",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        // External mix: blend between internal grain source and AudioToBuffer input.
        // 0.0 = internal oscillator/coupling only; 1.0 = external ring buffer only.
        // Blends continuously so presets can automate the cross-fade.
        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::EXTERNAL_MIX, 1 }, "Opal External Mix",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        //==== 2. GRAIN SCHEDULER ====
        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::GRAIN_SIZE, 1 }, "Opal Grain Size",
            NR (10.0f, 800.0f, 0.1f, 0.4f), 120.0f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::DENSITY, 1 }, "Opal Density",
            NR (1.0f, 120.0f, 0.1f, 0.5f), 20.0f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::POSITION, 1 }, "Opal Position",
            NR (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::POS_SCATTER, 1 }, "Opal Pos Scatter",
            NR (0.0f, 1.0f, 0.01f), 0.1f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::PITCH_SHIFT, 1 }, "Opal Pitch Shift",
            NR (-24.0f, 24.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::PITCH_SCATTER, 1 }, "Opal Pitch Scatter",
            NR (0.0f, 24.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::PAN_SCATTER, 1 }, "Opal Pan Scatter",
            NR (0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<ChoiceParam> (
            PID { OpalParam::WINDOW, 1 }, "Opal Window",
            juce::StringArray { "Hann", "Gaussian", "Tukey", "Rectangular" }, 0));

        //==== 3. FREEZE ====
        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FREEZE, 1 }, "Opal Freeze",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FREEZE_SIZE, 1 }, "Opal Freeze Region",
            NR (0.01f, 1.0f, 0.01f), 0.25f));

        //==== 4. FILTER ====
        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FILTER_CUTOFF, 1 }, "Opal Filter Cutoff",
            NR (20.0f, 20000.0f, 0.1f, 0.3f), 8000.0f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FILTER_RESO, 1 }, "Opal Filter Reso",
            NR (0.0f, 1.0f, 0.01f), 0.15f));

        params.push_back (std::make_unique<ChoiceParam> (
            PID { OpalParam::FILTER_MODE, 1 }, "Opal Filter Mode",
            juce::StringArray { "Low Pass", "Band Pass", "High Pass", "Notch" }, 0));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FILTER_KEY_TRACK, 1 }, "Opal Key Track",
            NR (0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FILTER_DRIVE, 1 }, "Opal Filter Drive",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        //==== 5. CHARACTER ====
        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::SHIMMER, 1 }, "Opal Shimmer",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FROST, 1 }, "Opal Frost",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        //==== 6. AMP ENVELOPE ====
        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::AMP_ATTACK, 1 }, "Opal Amp Attack",
            NR (0.001f, 8.0f, 0.001f, 0.4f), 0.3f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::AMP_DECAY, 1 }, "Opal Amp Decay",
            NR (0.05f, 4.0f, 0.001f, 0.4f), 0.5f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::AMP_SUSTAIN, 1 }, "Opal Amp Sustain",
            NR (0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::AMP_RELEASE, 1 }, "Opal Amp Release",
            NR (0.05f, 8.0f, 0.001f, 0.4f), 1.5f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::AMP_VEL_SENS, 1 }, "Opal Velocity",
            NR (0.0f, 1.0f, 0.01f), 0.4f));

        //==== 7. FILTER ENVELOPE ====
        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FILTER_ENV_AMT, 1 }, "Opal Filter Env Amt",
            NR (-1.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FILTER_ATTACK, 1 }, "Opal Filter Attack",
            NR (0.001f, 8.0f, 0.001f, 0.4f), 0.5f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FILTER_DECAY, 1 }, "Opal Filter Decay",
            NR (0.05f, 4.0f, 0.001f, 0.4f), 0.8f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FILTER_SUSTAIN, 1 }, "Opal Filter Sustain",
            NR (0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FILTER_RELEASE, 1 }, "Opal Filter Release",
            NR (0.05f, 8.0f, 0.001f, 0.4f), 1.0f));

        //==== 8. LFO 1 ====
        params.push_back (std::make_unique<ChoiceParam> (
            PID { OpalParam::LFO1_SHAPE, 1 }, "Opal LFO1 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "Random", "Stepped" }, 0));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::LFO1_RATE, 1 }, "Opal LFO1 Rate",
            NR (0.01f, 20.0f, 0.01f, 0.4f), 0.5f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::LFO1_DEPTH, 1 }, "Opal LFO1 Depth",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<ChoiceParam> (
            PID { OpalParam::LFO1_SYNC, 1 }, "Opal LFO1 Sync",
            juce::StringArray { "Off", "On" }, 0));

        params.push_back (std::make_unique<ChoiceParam> (
            PID { OpalParam::LFO1_RETRIGGER, 1 }, "Opal LFO1 Retrig",
            juce::StringArray { "Off", "On" }, 0));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::LFO1_PHASE, 1 }, "Opal LFO1 Phase",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        //==== 9. LFO 2 ====
        params.push_back (std::make_unique<ChoiceParam> (
            PID { OpalParam::LFO2_SHAPE, 1 }, "Opal LFO2 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "Random", "Stepped" }, 0));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::LFO2_RATE, 1 }, "Opal LFO2 Rate",
            NR (0.01f, 20.0f, 0.01f, 0.4f), 2.0f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::LFO2_DEPTH, 1 }, "Opal LFO2 Depth",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<ChoiceParam> (
            PID { OpalParam::LFO2_SYNC, 1 }, "Opal LFO2 Sync",
            juce::StringArray { "Off", "On" }, 0));

        params.push_back (std::make_unique<ChoiceParam> (
            PID { OpalParam::LFO2_RETRIGGER, 1 }, "Opal LFO2 Retrig",
            juce::StringArray { "Off", "On" }, 0));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::LFO2_PHASE, 1 }, "Opal LFO2 Phase",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        //==== 10. MOD MATRIX (6 slots) ====
        const char* modSrcIds[] = { OpalParam::MOD1_SRC, OpalParam::MOD2_SRC,
            OpalParam::MOD3_SRC, OpalParam::MOD4_SRC, OpalParam::MOD5_SRC, OpalParam::MOD6_SRC };
        const char* modDstIds[] = { OpalParam::MOD1_DST, OpalParam::MOD2_DST,
            OpalParam::MOD3_DST, OpalParam::MOD4_DST, OpalParam::MOD5_DST, OpalParam::MOD6_DST };
        const char* modAmtIds[] = { OpalParam::MOD1_AMT, OpalParam::MOD2_AMT,
            OpalParam::MOD3_AMT, OpalParam::MOD4_AMT, OpalParam::MOD5_AMT, OpalParam::MOD6_AMT };

        for (int i = 0; i < kOpalModSlots; ++i)
        {
            juce::String num (i + 1);
            params.push_back (std::make_unique<ChoiceParam> (
                PID { modSrcIds[i], 1 }, "Opal Mod " + num + " Src", modSources, 0));
            params.push_back (std::make_unique<ChoiceParam> (
                PID { modDstIds[i], 1 }, "Opal Mod " + num + " Dst", modDests, 0));
            params.push_back (std::make_unique<FloatParam> (
                PID { modAmtIds[i], 1 }, "Opal Mod " + num + " Amt",
                NR (-1.0f, 1.0f, 0.01f), 0.0f));
        }

        //==== 11. MACROS ====
        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::MACRO_SCATTER, 1 }, "Opal SCATTER",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::MACRO_DRIFT, 1 }, "Opal DRIFT",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::MACRO_COUPLING, 1 }, "Opal COUPLING",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::MACRO_SPACE, 1 }, "Opal SPACE",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        //==== 12. FX — SMEAR ====
        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FX_SMEAR_AMT, 1 }, "Opal Smear",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FX_SMEAR_MIX, 1 }, "Opal Smear Mix",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        //==== 13. FX — SCATTER REVERB ====
        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FX_REVERB_SIZE, 1 }, "Opal Reverb Size",
            NR (0.0f, 1.0f, 0.01f), 0.4f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FX_REVERB_DECAY, 1 }, "Opal Reverb Decay",
            NR (0.1f, 10.0f, 0.01f, 0.4f), 2.0f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FX_REVERB_DAMP, 1 }, "Opal Reverb Damp",
            NR (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FX_REVERB_MIX, 1 }, "Opal Reverb Mix",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        //==== 14. FX — DELAY ====
        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FX_DELAY_TIME, 1 }, "Opal Delay Time",
            NR (0.01f, 2.0f, 0.001f, 0.4f), 0.35f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FX_DELAY_FB, 1 }, "Opal Delay FB",
            NR (0.0f, 0.95f, 0.01f), 0.3f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FX_DELAY_MIX, 1 }, "Opal Delay Mix",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<ChoiceParam> (
            PID { OpalParam::FX_DELAY_SYNC, 1 }, "Opal Delay Sync",
            juce::StringArray { "Off", "On" }, 0));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FX_DELAY_SPREAD, 1 }, "Opal Delay Spread",
            NR (0.0f, 1.0f, 0.01f), 0.3f));

        //==== 15. FX — FINISH ====
        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FX_FINISH_GLUE, 1 }, "Opal Glue",
            NR (0.0f, 1.0f, 0.01f), 0.1f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FX_FINISH_WIDTH, 1 }, "Opal Width",
            NR (0.0f, 2.0f, 0.01f), 1.0f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::FX_FINISH_LEVEL, 1 }, "Opal Finish Level",
            NR (0.0f, 1.0f, 0.01f), 0.75f));

        //==== 16. VOICE ====
        params.push_back (std::make_unique<ChoiceParam> (
            PID { OpalParam::VOICE_MODE, 1 }, "Opal Voice Mode",
            juce::StringArray { "Poly", "Mono" }, 0));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::GLIDE_TIME, 1 }, "Opal Glide",
            NR (0.0f, 2.0f, 0.001f, 0.4f), 0.0f));

        params.push_back (std::make_unique<ChoiceParam> (
            PID { OpalParam::GLIDE_MODE, 1 }, "Opal Glide Mode",
            juce::StringArray { "Always", "Legato" }, 0));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::PAN, 1 }, "Opal Pan",
            NR (-1.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<FloatParam> (
            PID { OpalParam::LEVEL, 1 }, "Opal Level",
            NR (0.0f, 1.0f, 0.01f), 0.75f));
    }

    //-- attachParameters ------------------------------------------------------

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        // Grain Source
        pSource         = apvts.getRawParameterValue (OpalParam::SOURCE);
        pOscShape       = apvts.getRawParameterValue (OpalParam::OSC_SHAPE);
        pOsc2Shape      = apvts.getRawParameterValue (OpalParam::OSC2_SHAPE);
        pOsc2Mix        = apvts.getRawParameterValue (OpalParam::OSC2_MIX);
        pOsc2Detune     = apvts.getRawParameterValue (OpalParam::OSC2_DETUNE);
        pCouplingLevel  = apvts.getRawParameterValue (OpalParam::COUPLING_LEVEL);
        pExternalMix    = apvts.getRawParameterValue (OpalParam::EXTERNAL_MIX);
        // Grain Scheduler
        pGrainSize      = apvts.getRawParameterValue (OpalParam::GRAIN_SIZE);
        pDensity        = apvts.getRawParameterValue (OpalParam::DENSITY);
        pPosition       = apvts.getRawParameterValue (OpalParam::POSITION);
        pPosScatter     = apvts.getRawParameterValue (OpalParam::POS_SCATTER);
        pPitchShift     = apvts.getRawParameterValue (OpalParam::PITCH_SHIFT);
        pPitchScatter   = apvts.getRawParameterValue (OpalParam::PITCH_SCATTER);
        pPanScatter     = apvts.getRawParameterValue (OpalParam::PAN_SCATTER);
        pWindow         = apvts.getRawParameterValue (OpalParam::WINDOW);
        // Freeze
        pFreeze         = apvts.getRawParameterValue (OpalParam::FREEZE);
        pFreezeSize     = apvts.getRawParameterValue (OpalParam::FREEZE_SIZE);
        // Filter
        pFilterCutoff   = apvts.getRawParameterValue (OpalParam::FILTER_CUTOFF);
        pFilterReso     = apvts.getRawParameterValue (OpalParam::FILTER_RESO);
        pFilterMode     = apvts.getRawParameterValue (OpalParam::FILTER_MODE);
        pFilterKeyTrack = apvts.getRawParameterValue (OpalParam::FILTER_KEY_TRACK);
        pFilterDrive    = apvts.getRawParameterValue (OpalParam::FILTER_DRIVE);
        // Character
        pShimmer        = apvts.getRawParameterValue (OpalParam::SHIMMER);
        pFrost          = apvts.getRawParameterValue (OpalParam::FROST);
        // Amp Envelope
        pAmpAttack      = apvts.getRawParameterValue (OpalParam::AMP_ATTACK);
        pAmpDecay       = apvts.getRawParameterValue (OpalParam::AMP_DECAY);
        pAmpSustain     = apvts.getRawParameterValue (OpalParam::AMP_SUSTAIN);
        pAmpRelease     = apvts.getRawParameterValue (OpalParam::AMP_RELEASE);
        pAmpVelSens     = apvts.getRawParameterValue (OpalParam::AMP_VEL_SENS);
        // Filter Envelope
        pFilterEnvAmt   = apvts.getRawParameterValue (OpalParam::FILTER_ENV_AMT);
        pFilterAttack   = apvts.getRawParameterValue (OpalParam::FILTER_ATTACK);
        pFilterDecay    = apvts.getRawParameterValue (OpalParam::FILTER_DECAY);
        pFilterSustain  = apvts.getRawParameterValue (OpalParam::FILTER_SUSTAIN);
        pFilterRelease  = apvts.getRawParameterValue (OpalParam::FILTER_RELEASE);
        // LFO 1
        pLfo1Shape      = apvts.getRawParameterValue (OpalParam::LFO1_SHAPE);
        pLfo1Rate       = apvts.getRawParameterValue (OpalParam::LFO1_RATE);
        pLfo1Depth      = apvts.getRawParameterValue (OpalParam::LFO1_DEPTH);
        pLfo1Sync       = apvts.getRawParameterValue (OpalParam::LFO1_SYNC);
        pLfo1Retrigger  = apvts.getRawParameterValue (OpalParam::LFO1_RETRIGGER);
        pLfo1Phase      = apvts.getRawParameterValue (OpalParam::LFO1_PHASE);
        // LFO 2
        pLfo2Shape      = apvts.getRawParameterValue (OpalParam::LFO2_SHAPE);
        pLfo2Rate       = apvts.getRawParameterValue (OpalParam::LFO2_RATE);
        pLfo2Depth      = apvts.getRawParameterValue (OpalParam::LFO2_DEPTH);
        pLfo2Sync       = apvts.getRawParameterValue (OpalParam::LFO2_SYNC);
        pLfo2Retrigger  = apvts.getRawParameterValue (OpalParam::LFO2_RETRIGGER);
        pLfo2Phase      = apvts.getRawParameterValue (OpalParam::LFO2_PHASE);
        // Mod Matrix
        pModSlotSrc[0]  = apvts.getRawParameterValue (OpalParam::MOD1_SRC);
        pModSlotDst[0]  = apvts.getRawParameterValue (OpalParam::MOD1_DST);
        pModSlotAmt[0]  = apvts.getRawParameterValue (OpalParam::MOD1_AMT);
        pModSlotSrc[1]  = apvts.getRawParameterValue (OpalParam::MOD2_SRC);
        pModSlotDst[1]  = apvts.getRawParameterValue (OpalParam::MOD2_DST);
        pModSlotAmt[1]  = apvts.getRawParameterValue (OpalParam::MOD2_AMT);
        pModSlotSrc[2]  = apvts.getRawParameterValue (OpalParam::MOD3_SRC);
        pModSlotDst[2]  = apvts.getRawParameterValue (OpalParam::MOD3_DST);
        pModSlotAmt[2]  = apvts.getRawParameterValue (OpalParam::MOD3_AMT);
        pModSlotSrc[3]  = apvts.getRawParameterValue (OpalParam::MOD4_SRC);
        pModSlotDst[3]  = apvts.getRawParameterValue (OpalParam::MOD4_DST);
        pModSlotAmt[3]  = apvts.getRawParameterValue (OpalParam::MOD4_AMT);
        pModSlotSrc[4]  = apvts.getRawParameterValue (OpalParam::MOD5_SRC);
        pModSlotDst[4]  = apvts.getRawParameterValue (OpalParam::MOD5_DST);
        pModSlotAmt[4]  = apvts.getRawParameterValue (OpalParam::MOD5_AMT);
        pModSlotSrc[5]  = apvts.getRawParameterValue (OpalParam::MOD6_SRC);
        pModSlotDst[5]  = apvts.getRawParameterValue (OpalParam::MOD6_DST);
        pModSlotAmt[5]  = apvts.getRawParameterValue (OpalParam::MOD6_AMT);
        // Macros
        pMacroScatter   = apvts.getRawParameterValue (OpalParam::MACRO_SCATTER);
        pMacroDrift     = apvts.getRawParameterValue (OpalParam::MACRO_DRIFT);
        pMacroCoupling  = apvts.getRawParameterValue (OpalParam::MACRO_COUPLING);
        pMacroSpace     = apvts.getRawParameterValue (OpalParam::MACRO_SPACE);
        // FX — Smear
        pFxSmearAmt     = apvts.getRawParameterValue (OpalParam::FX_SMEAR_AMT);
        pFxSmearMix     = apvts.getRawParameterValue (OpalParam::FX_SMEAR_MIX);
        // FX — Reverb
        pFxReverbSize   = apvts.getRawParameterValue (OpalParam::FX_REVERB_SIZE);
        pFxReverbDecay  = apvts.getRawParameterValue (OpalParam::FX_REVERB_DECAY);
        pFxReverbDamp   = apvts.getRawParameterValue (OpalParam::FX_REVERB_DAMP);
        pFxReverbMix    = apvts.getRawParameterValue (OpalParam::FX_REVERB_MIX);
        // FX — Delay
        pFxDelayTime    = apvts.getRawParameterValue (OpalParam::FX_DELAY_TIME);
        pFxDelayFB      = apvts.getRawParameterValue (OpalParam::FX_DELAY_FB);
        pFxDelayMix     = apvts.getRawParameterValue (OpalParam::FX_DELAY_MIX);
        pFxDelaySync    = apvts.getRawParameterValue (OpalParam::FX_DELAY_SYNC);
        pFxDelaySpread  = apvts.getRawParameterValue (OpalParam::FX_DELAY_SPREAD);
        // FX — Finish
        pFxFinishGlue   = apvts.getRawParameterValue (OpalParam::FX_FINISH_GLUE);
        pFxFinishWidth  = apvts.getRawParameterValue (OpalParam::FX_FINISH_WIDTH);
        pFxFinishLevel  = apvts.getRawParameterValue (OpalParam::FX_FINISH_LEVEL);
        // Voice
        pVoiceMode      = apvts.getRawParameterValue (OpalParam::VOICE_MODE);
        pGlideTime      = apvts.getRawParameterValue (OpalParam::GLIDE_TIME);
        pGlideMode      = apvts.getRawParameterValue (OpalParam::GLIDE_MODE);
        pPan            = apvts.getRawParameterValue (OpalParam::PAN);
        pLevel          = apvts.getRawParameterValue (OpalParam::LEVEL);
    }

    //-- Coupling --------------------------------------------------------------

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        // Return the per-sample cached output so tight coupling (AudioToFM,
        // AudioToRing, etc.) sees the correct per-position value, not a stale
        // end-of-block scalar.
        auto idx = static_cast<size_t> (sampleIndex);
        if (channel == 0 && idx < outputCacheLeft.size())
            return outputCacheLeft[idx];
        if (channel == 1 && idx < outputCacheRight.size())
            return outputCacheRight[idx];
        // Fallback: honest scalar — last rendered sample (e.g. block-level coupling)
        return (channel == 0) ? lastSampleL : lastSampleR;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* sourceBuffer, int numSamples) override
    {
        switch (type)
        {
            case CouplingType::AudioToWavetable:
                if (sourceBuffer != nullptr)
                {
                    for (int n = 0; n < numSamples; ++n)
                    {
                        couplingBufL[static_cast<size_t> (n)] = sourceBuffer[n] * amount;
                        couplingBufR[static_cast<size_t> (n)] = sourceBuffer[n] * amount;
                    }
                }
                break;

            case CouplingType::AmpToFilter:
                extFilterMod = amount * 8000.0f;
                break;

            case CouplingType::EnvToMorph:
                extPosMod = amount * 0.3f;
                break;

            case CouplingType::LFOToPitch:
                extPitchMod = amount * 12.0f;
                break;

            case CouplingType::RhythmToBlend:
                extDensityMod = amount * 60.0f;
                break;

            case CouplingType::EnvToDecay:
                extFreezeMod = amount;
                break;

            default:
                break;
        }
    }

    //-- AudioToBuffer public API ----------------------------------------------
    //
    // getGrainBuffer(slot) — called by MegaCouplingMatrix::processAudioRoute()
    // to obtain the per-slot ring buffer into which it will push audio.
    // slot 0–3 correspond to MegaCouplingMatrix::MaxSlots source positions.
    // Returns nullptr if slot is out of range (defensive guard).
    //
    AudioRingBuffer* getGrainBuffer (int slot) noexcept
    {
        if (slot < 0 || slot >= kOpalInputSlots) return nullptr;
        return &inputBuffers[static_cast<size_t>(slot)];
    }

    // receiveAudioBuffer — called by any subsystem that has already filled an
    // AudioRingBuffer and wants OPAL to adopt its content directly. Reads a full
    // block of stereo samples from `src` (most-recent numSamples samples) and
    // blends them into the internal grain source buffer at the externalMix ratio.
    //
    // Blending rule:
    //   grainBuffer ← (1 - mix) * internal + mix * external
    // where `internal` is the mono sample generated by generateOscSample() and
    // `external` is the L+R average read from the ring buffer.
    //
    // Called during renderBlock() — must be real-time safe.
    // `mix` is pre-read from pExternalMix by the caller to avoid double-load.
    //
    void receiveAudioBuffer (const AudioRingBuffer& src, int numSamples,
                             float mix, bool frozen) noexcept
    {
        if (mix < 0.001f) return;          // external mix off — fast path
        if (src.capacity <= 0)  return;

        for (int n = 0; n < numSamples; ++n)
        {
            // Sample age: n=0 is most recent, n=numSamples-1 is oldest in this block.
            // We map sample n to a fractionalOffset such that 0 = latest write, and
            // the step per sample is 1/capacity (one sample per capacity units).
            float frac = static_cast<float>(numSamples - 1 - n)
                       / static_cast<float>(src.capacity);
            float extL = src.readAt (0, frac);
            float extR = src.readAt (1, frac);
            float extMono = (extL + extR) * 0.5f;

            // Accumulate into the per-block cache. renderBlock() reads this
            // cache in the grain-source write loop and blends it with the
            // internal oscillator sample using the (1-mix)/mix cross-fade.
            // Use += so multiple simultaneous slots accumulate correctly.
            if (static_cast<size_t>(n) < extAudioBufL.size())
            {
                extAudioBufL[static_cast<size_t>(n)] += extMono * mix;
                extAudioBufR[static_cast<size_t>(n)] += extMono * mix;
            }
        }
    }

    //-- Audio Rendering -------------------------------------------------------

    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0 || pSource == nullptr) return;

        juce::ScopedNoDenormals noDenormals;
        buffer.clear();

        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : outL;

        // D006: smooth aftertouch pressure and compute modulation value
        aftertouch.updateBlock (numSamples);
        const float atPressure = aftertouch.getSmoothedPressure (0);

        // ---- Snapshot all parameters (once per block) ----
        int   sourceMode    = safeLoad (pSource, 1);
        float oscShape      = safeLoadF (pOscShape, 0.5f);
        float osc2Shape     = safeLoadF (pOsc2Shape, 0.5f);
        float osc2Mix       = safeLoadF (pOsc2Mix, 0.5f);
        float osc2Detune    = safeLoadF (pOsc2Detune, 0.1f);
        float couplingLevel = safeLoadF (pCouplingLevel, 0.0f);
        float externalMix   = safeLoadF (pExternalMix, 0.0f);
        float grainSizeMs   = safeLoadF (pGrainSize, 120.0f);
        float density       = safeLoadF (pDensity, 20.0f);
        float position      = safeLoadF (pPosition, 0.0f) + extPosMod;
        float posScatter    = safeLoadF (pPosScatter, 0.1f);
        // D006: aftertouch adds up to +0.3 grain scatter (sensitivity 0.3) — grains spread more under pressure
        posScatter = clamp (posScatter + atPressure * 0.3f, 0.0f, 1.0f);
        // D006: mod wheel adds up to +0.35 posScatter (sensitivity 0.35) — wheel opens time scatter
        posScatter = clamp (posScatter + modWheelAmount * 0.35f, 0.0f, 1.0f);
        float pitchShift    = safeLoadF (pPitchShift, 0.0f);
        float pitchScatter  = safeLoadF (pPitchScatter, 0.0f) + extPitchMod;
        // D006: mod wheel adds up to +0.25 pitchScatter (sensitivity 0.25) — wheel widens pitch cloud
        pitchScatter = clamp (pitchScatter + modWheelAmount * 0.25f, 0.0f, 1.0f);
        float panScatter    = safeLoadF (pPanScatter, 0.3f);
        int   windowShape   = safeLoad (pWindow, 0);
        float freezeAmt     = safeLoadF (pFreeze, 0.0f) + extFreezeMod;
        float filterCutoff  = clamp (safeLoadF (pFilterCutoff, 8000.0f) + extFilterMod,
                                     20.0f, 20000.0f);
        float filterReso    = safeLoadF (pFilterReso, 0.15f);
        int   filterMode    = safeLoad (pFilterMode, 0);
        float filterKeyTrack = safeLoadF (pFilterKeyTrack, 0.3f);
        float filterDrive   = safeLoadF (pFilterDrive, 0.0f);
        float shimmerAmt    = safeLoadF (pShimmer, 0.0f);
        float frostAmt      = safeLoadF (pFrost, 0.0f);
        float ampAttack     = safeLoadF (pAmpAttack, 0.3f);
        float ampDecay      = safeLoadF (pAmpDecay, 0.5f);
        float ampSustain    = safeLoadF (pAmpSustain, 0.8f);
        float ampRelease    = safeLoadF (pAmpRelease, 1.5f);
        float ampVelSens    = safeLoadF (pAmpVelSens, 0.4f);
        float filterEnvAmt  = safeLoadF (pFilterEnvAmt, 0.3f);
        float filterAttack  = safeLoadF (pFilterAttack, 0.5f);
        float filterDecay   = safeLoadF (pFilterDecay, 0.8f);
        float filterSustain = safeLoadF (pFilterSustain, 0.3f);
        float filterRelease = safeLoadF (pFilterRelease, 1.0f);
        bool  lfo1Retrig    = safeLoad (pLfo1Retrigger, 0) > 0;
        float lfo1StartPh  = safeLoadF (pLfo1Phase, 0.0f);
        bool  lfo2Retrig    = safeLoad (pLfo2Retrigger, 0) > 0;
        float lfo2StartPh  = safeLoadF (pLfo2Phase, 0.0f);
        float freezeRegion  = safeLoadF (pFreezeSize, 0.25f);
        int   lfo1Shape     = safeLoad (pLfo1Shape, 0);
        float lfo1Rate      = safeLoadF (pLfo1Rate, 0.5f);
        float lfo1Depth     = safeLoadF (pLfo1Depth, 0.0f);
        int   lfo2Shape     = safeLoad (pLfo2Shape, 0);
        float lfo2Rate      = safeLoadF (pLfo2Rate, 2.0f);
        float lfo2Depth     = safeLoadF (pLfo2Depth, 0.0f);
        float masterPan     = safeLoadF (pPan, 0.0f);
        float masterLevel   = safeLoadF (pLevel, 0.75f);
        int   voiceMode     = safeLoad (pVoiceMode, 0);
        float glideTime     = safeLoadF (pGlideTime, 0.0f);
        int   glideMode     = safeLoad (pGlideMode, 0);

        // FX params
        float fxSmearAmt    = safeLoadF (pFxSmearAmt, 0.0f);
        float fxSmearMix    = safeLoadF (pFxSmearMix, 0.0f);
        float fxReverbSize  = safeLoadF (pFxReverbSize, 0.4f);
        float fxReverbDecay = safeLoadF (pFxReverbDecay, 2.0f);
        float fxReverbDamp  = safeLoadF (pFxReverbDamp, 0.5f);
        float fxReverbMix   = safeLoadF (pFxReverbMix, 0.0f);
        float fxDelayTime   = safeLoadF (pFxDelayTime, 0.35f);
        float fxDelayFB     = safeLoadF (pFxDelayFB, 0.3f);
        float fxDelayMix    = safeLoadF (pFxDelayMix, 0.0f);
        float fxDelaySpread = safeLoadF (pFxDelaySpread, 0.3f);
        float fxFinishGlue  = safeLoadF (pFxFinishGlue, 0.1f);
        float fxFinishWidth = safeLoadF (pFxFinishWidth, 1.0f);
        float fxFinishLevel = safeLoadF (pFxFinishLevel, 0.75f);

        density += extDensityMod;

        // ---- Apply macros ----
        applyMacros (grainSizeMs, density, posScatter, pitchScatter, panScatter,
                     couplingLevel, freezeAmt, fxReverbMix, fxSmearMix, fxDelayMix);

        // Clamp after macro/external modulation
        position     = clamp (position, 0.0f, 1.0f);
        pitchScatter = clamp (pitchScatter, 0.0f, 24.0f);
        density      = clamp (density, 1.0f, 120.0f);
        freezeAmt    = clamp (freezeAmt, 0.0f, 1.0f);

        bool frozen = (freezeAmt > 0.5f);

        // ---- MIDI ----
        bool isMono = (voiceMode == 1);
        bool legatoGlide = (glideMode == 1);

        for (const auto& msg : midi)
        {
            auto m = msg.getMessage();
            if (m.isNoteOn())
            {
                float prevFreq = 261.63f;
                int voiceIdx;

                if (isMono)
                {
                    voiceIdx = 0;
                    if (voices[0].active)
                        prevFreq = voices[0].glideFreq;
                    bool legato = voices[0].active;
                    voices[0].noteOn (m.getNoteNumber(), m.getFloatVelocity(), sr,
                                      ampAttack, ampDecay, ampSustain, ampRelease,
                                      filterAttack, filterDecay, filterSustain, filterRelease,
                                      prevFreq, glideTime,
                                      legato && legatoGlide,
                                      lfo1Retrig, lfo1StartPh,
                                      lfo2Retrig, lfo2StartPh);
                    voices[0].birthOrder = ++voiceBirthCounter;
                }
                else
                {
                    voiceIdx = findFreeVoice();
                    auto& v = voices[voiceIdx];
                    if (v.active)
                    {
                        grainPool.killVoiceGrains (voiceIdx);
                        prevFreq = v.glideFreq;
                    }
                    v.noteOn (m.getNoteNumber(), m.getFloatVelocity(), sr,
                              ampAttack, ampDecay, ampSustain, ampRelease,
                              filterAttack, filterDecay, filterSustain, filterRelease,
                              prevFreq, glideTime, false,
                              lfo1Retrig, lfo1StartPh,
                              lfo2Retrig, lfo2StartPh);
                    v.birthOrder = ++voiceBirthCounter;
                }
            }
            else if (m.isNoteOff())
            {
                if (!sustainPedalDown)
                {
                    for (auto& v : voices)
                        if (v.active && v.noteNumber == m.getNoteNumber())
                            v.noteOff();
                }
            }
            else if (m.isAllNotesOff() || m.isAllSoundOff())
            {
                for (auto& v : voices) v.kill();
                grainPool.killAll();
                sustainPedalDown = false;
            }
            else if (m.isController() && m.getControllerNumber() == 64)
            {
                bool wasDown = sustainPedalDown;
                sustainPedalDown = (m.getControllerValue() >= 64);
                if (wasDown && !sustainPedalDown)
                    for (auto& v : voices)
                        if (v.active && v.ampEnv.getStage() == OpalADSR::Sustain)
                            v.noteOff();
            }
            // D006: channel pressure → aftertouch (applied to grain scatter below)
            else if (m.isChannelPressure())
                aftertouch.setChannelPressure (m.getChannelPressureValue() / 127.0f);
            // D006: mod wheel (CC#1) → grain density scatter
            // Wheel up = wider position + pitch scatter: grains spread further in
            // time (posScatter +0.35) and pitch (pitchScatter +0.25) at full wheel,
            // producing a diffuse, shimmering iridescent cloud.
            else if (m.isController() && m.getControllerNumber() == 1)
                modWheelAmount = m.getControllerValue() / 127.0f;
        }

        // ---- Per-sample: write grain source + tick schedulers ----
        bool isCouplingSource = (sourceMode == 5);

        // Build the external audio blend cache from all active AudioToBuffer slots.
        // inputBuffers[] were pushed by MegaCouplingMatrix::processAudioRoute() before
        // this renderBlock() call, so they hold the current block's source audio.
        if (externalMix > 0.001f)
        {
            for (auto& s : extAudioBufL) s = 0.0f;
            for (auto& s : extAudioBufR) s = 0.0f;
            for (int slot = 0; slot < kOpalInputSlots; ++slot)
            {
                const auto& rb = inputBuffers[static_cast<size_t>(slot)];
                if (rb.capacity <= 0) continue;
                receiveAudioBuffer (rb, numSamples,
                                    externalMix / static_cast<float>(kOpalInputSlots),
                                    frozen);
            }
        }

        for (int n = 0; n < numSamples; ++n)
        {
            // Write grain source to buffer
            float srcSample = 0.0f;
            if (isCouplingSource)
            {
                srcSample = couplingBufL[static_cast<size_t> (n)] * couplingLevel;
            }
            else
            {
                // Use a simple oscillator for grain source material
                // All active voices contribute to the buffer from the lowest note
                srcSample = generateOscSample (sourceMode, oscShape, osc2Shape,
                                               osc2Mix, osc2Detune);
            }

            // Blend in external AudioToBuffer content.
            // externalMix = 0 → pure internal; externalMix = 1 → pure external.
            // extAudioBufL cache was populated above; the per-slot contribution is
            // already pre-scaled by (externalMix / kOpalInputSlots) inside
            // receiveAudioBuffer(), so summing all slots gives the full externalMix level.
            if (externalMix > 0.001f && static_cast<size_t>(n) < extAudioBufL.size())
            {
                float extSample = extAudioBufL[static_cast<size_t>(n)];
                srcSample = srcSample * (1.0f - externalMix) + extSample;
            }

            grainBuffer.write (srcSample, frozen);

            // Tick grain schedulers for each active voice
            for (auto& v : voices)
            {
                if (!v.active) continue;

                // Glide
                if (v.glideRate > 0.0f)
                {
                    float diff = v.glideTarget - v.glideFreq;
                    v.glideFreq += diff * v.glideRate;
                    if (std::fabs (diff) < 0.01f)
                    {
                        v.glideFreq = v.glideTarget;
                        v.glideRate = 0.0f;
                    }
                }

                // Set oscillator frequencies for this voice
                v.osc1.setFrequency (v.glideFreq, static_cast<float> (sr));
                v.osc2.setFrequency (v.glideFreq * std::pow (2.0f, osc2Detune / 12.0f),
                                     static_cast<float> (sr));

                // Grain scheduling
                v.triggerAccum += 1.0f;
                float triggerInterval = static_cast<float> (sr) / density;

                while (v.triggerAccum >= triggerInterval)
                {
                    v.triggerAccum -= triggerInterval;
                    spawnGrainForVoice (v, position, posScatter, pitchShift,
                                        pitchScatter, panScatter, grainSizeMs,
                                        windowShape, masterPan, frozen, freezeRegion);
                }
            }
        }

        // Clear coupling buffer + external mods
        for (int n = 0; n < numSamples; ++n)
        {
            couplingBufL[static_cast<size_t> (n)] = 0.0f;
            couplingBufR[static_cast<size_t> (n)] = 0.0f;
        }
        extFilterMod = 0.0f;
        extPosMod = 0.0f;
        extPitchMod = 0.0f;
        extDensityMod = 0.0f;
        extFreezeMod = 0.0f;

        // ---- Render grains into work buffer ----
        for (int n = 0; n < numSamples; ++n)
        {
            workBufL[static_cast<size_t> (n)] = 0.0f;
            workBufR[static_cast<size_t> (n)] = 0.0f;
        }
        grainPool.processAll (grainBuffer, workBufL.data(), workBufR.data(), numSamples);

        // ---- Per-voice post-processing: filter, character, envelope ----
        // For simplicity, process filter/character/envelope on the mixed output
        // (per-voice filtering would require separate render passes)
        for (int n = 0; n < numSamples; ++n)
        {
            float L = workBufL[static_cast<size_t> (n)];
            float R = workBufR[static_cast<size_t> (n)];

            // Compute voice envelope mix (sum of all active voice envelopes)
            float envSum = 0.0f;
            float filterEnvSum = 0.0f;
            float lfo1Val = 0.0f, lfo2Val = 0.0f;
            int activeCount = 0;

            for (auto& v : voices)
            {
                if (!v.active) continue;

                float ampLevel = v.ampEnv.process();
                float filterLevel = v.filterEnv.process();

                // Process LFOs
                float l1 = v.lfo1.process (lfo1Rate, lfo1Shape, v.rng) * lfo1Depth;
                float l2 = v.lfo2.process (lfo2Rate, lfo2Shape, v.rng) * lfo2Depth;

                // Velocity scaling
                float velScale = (1.0f - ampVelSens) + ampVelSens * v.velocity;
                ampLevel *= velScale;

                envSum += ampLevel;
                filterEnvSum += filterLevel;
                lfo1Val += l1;
                lfo2Val += l2;
                ++activeCount;

                // Check if voice is done
                if (!v.ampEnv.isActive())
                {
                    grainPool.killVoiceGrains (v.voiceIndex);
                    v.active = false;
                }
            }

            if (activeCount > 0)
            {
                float norm = 1.0f / static_cast<float> (activeCount);
                filterEnvSum *= norm;
                lfo1Val *= norm;
                lfo2Val *= norm;
            }

            // Apply amp envelope
            L *= envSum;
            R *= envSum;

            // Apply mod matrix modulation (accumulated)
            float modOffsets[12] = {}; // indexed by mod dest
            applyModMatrix (modOffsets, lfo1Val, lfo2Val, filterEnvSum, envSum, activeCount);

            // Filter with key tracking + filter envelope + drive
            // Key tracking: shift cutoff based on average active note vs middle C (60)
            float keyTrackOffset = 0.0f;
            if (filterKeyTrack > 0.001f && activeCount > 0)
            {
                float avgNote = 0.0f;
                for (const auto& v : voices)
                    if (v.active) avgNote += static_cast<float> (v.noteNumber);
                avgNote /= static_cast<float> (activeCount);
                // Each semitone above middle C shifts cutoff up proportionally
                keyTrackOffset = filterKeyTrack * (avgNote - 60.0f) * (filterCutoff / 60.0f);
            }
            float effCutoff = filterCutoff + keyTrackOffset
                            + filterEnvAmt * filterEnvSum * 10000.0f
                            + modOffsets[6] * 5000.0f; // mod dest 6 = FilterCutoff
            effCutoff = clamp (effCutoff, 20.0f, 20000.0f);

            // Pre-filter drive
            if (filterDrive > 0.001f)
            {
                float driveGain = 1.0f + filterDrive * 5.0f;
                L = fastTanh (L * driveGain);
                R = fastTanh (R * driveGain);
            }

            // Set filter mode
            CytomicSVF::Mode fMode;
            switch (filterMode)
            {
                case 1:  fMode = CytomicSVF::Mode::BandPass; break;
                case 2:  fMode = CytomicSVF::Mode::HighPass; break;
                case 3:  fMode = CytomicSVF::Mode::Notch;    break;
                default: fMode = CytomicSVF::Mode::LowPass;  break;
            }
            globalFilterL.setMode (fMode);
            globalFilterR.setMode (fMode);
            globalFilterL.setCoefficients (effCutoff, filterReso, static_cast<float> (sr));
            globalFilterR.setCoefficients (effCutoff, filterReso, static_cast<float> (sr));
            L = globalFilterL.processSample (L);
            R = globalFilterR.processSample (R);

            // Character: Shimmer (harmonic fold — octave-up via half-wave rect)
            float effShimmer = shimmerAmt + modOffsets[7] * 0.5f; // mod dest 7
            if (effShimmer > 0.001f)
            {
                effShimmer = clamp (effShimmer, 0.0f, 1.0f);
                float shimL = std::fabs (L) * 2.0f - L; // half-wave rect
                float shimR = std::fabs (R) * 2.0f - R;
                L = L + effShimmer * 0.5f * (shimL - L);
                R = R + effShimmer * 0.5f * (shimR - R);
            }

            // Character: Frost (cold limiter — hard knee, no warmth)
            float effFrost = frostAmt + modOffsets[8] * 0.5f; // mod dest 8
            if (effFrost > 0.001f)
            {
                effFrost = clamp (effFrost, 0.0f, 1.0f);
                float ceiling = 1.0f - effFrost * 0.5f;
                if (std::fabs (L) > ceiling)
                    L = ceiling * (L > 0 ? 1.0f : -1.0f);
                if (std::fabs (R) > ceiling)
                    R = ceiling * (R > 0 ? 1.0f : -1.0f);
            }

            outL[n] = L;
            outR[n] = R;
        }

        // ---- FX Chain (post-voice mix, stereo) ----

        // Smear: time-stretch effect (simplified: secondary granular-style diffusion)
        if (fxSmearMix > 0.001f)
        {
            for (int n = 0; n < numSamples; ++n)
            {
                // Smear approximation: feedback low-pass smoothing
                smearStateL = flushDenormal (smearStateL + (outL[n] - smearStateL)
                              * (0.01f + 0.1f * (1.0f - fxSmearAmt)));
                smearStateR = flushDenormal (smearStateR + (outR[n] - smearStateR)
                              * (0.01f + 0.1f * (1.0f - fxSmearAmt)));
                outL[n] = outL[n] * (1.0f - fxSmearMix) + smearStateL * fxSmearMix;
                outR[n] = outR[n] * (1.0f - fxSmearMix) + smearStateR * fxSmearMix;
            }
        }

        // Scatter Reverb
        if (fxReverbMix > 0.001f)
        {
            scatterReverb.setParams (fxReverbSize, fxReverbDecay, fxReverbDamp);
            for (int n = 0; n < numSamples; ++n)
            {
                float wetL = outL[n], wetR = outR[n];
                scatterReverb.process (wetL, wetR);
                outL[n] = outL[n] * (1.0f - fxReverbMix) + wetL * fxReverbMix;
                outR[n] = outR[n] * (1.0f - fxReverbMix) + wetR * fxReverbMix;
            }
        }

        // Delay
        if (fxDelayMix > 0.001f)
        {
            for (int n = 0; n < numSamples; ++n)
            {
                float dL = outL[n], dR = outR[n];
                stereoDelay.process (dL, dR, fxDelayTime, fxDelayFB, fxDelaySpread);
                outL[n] = outL[n] * (1.0f - fxDelayMix) + dL * fxDelayMix;
                outR[n] = outR[n] * (1.0f - fxDelayMix) + dR * fxDelayMix;
            }
        }

        // Finish: glue + width + level
        for (int n = 0; n < numSamples; ++n)
        {
            finish.process (outL[n], outR[n], fxFinishGlue, fxFinishWidth);

            // Master pan
            if (masterPan != 0.0f)
            {
                float pL = clamp (1.0f - masterPan, 0.0f, 1.0f);
                float pR = clamp (1.0f + masterPan, 0.0f, 1.0f);
                outL[n] *= pL;
                outR[n] *= pR;
            }

            // Master level + finish level
            outL[n] *= masterLevel * fxFinishLevel;
            outR[n] *= masterLevel * fxFinishLevel;
        }

        // ---- Cache per-sample output for coupling reads ----
        // getSampleForCoupling() uses outputCacheLeft/Right so that tight coupling
        // (AudioToFM, AudioToRing) sees the correct value at each sample index
        // instead of a stale end-of-block scalar.
        for (int n = 0; n < numSamples; ++n)
        {
            auto idx = static_cast<size_t> (n);
            if (idx < outputCacheLeft.size())
            {
                outputCacheLeft[idx]  = outL[n];
                outputCacheRight[idx] = outR[n];
            }
        }
        if (numSamples > 0)
        {
            lastSampleL = outL[numSamples - 1];
            lastSampleR = outR[numSamples - 1];
        }
    }

private:
    //-- Helpers ----------------------------------------------------------------

    static int safeLoad (std::atomic<float>* p, int fallback) noexcept
    {
        return (p != nullptr) ? static_cast<int> (p->load()) : fallback;
    }

    static float safeLoadF (std::atomic<float>* p, float fallback) noexcept
    {
        return (p != nullptr) ? p->load() : fallback;
    }

    int findFreeVoice() noexcept
    {
        // Prefer inactive voice
        for (int i = 0; i < kOpalMaxClouds; ++i)
            if (!voices[i].active) return i;

        // Steal oldest (LRU)
        int oldest = 0;
        uint32_t oldestBirth = voices[0].birthOrder;
        for (int i = 1; i < kOpalMaxClouds; ++i)
        {
            if (voices[i].birthOrder < oldestBirth)
            {
                oldest = i;
                oldestBirth = voices[i].birthOrder;
            }
        }
        return oldest;
    }

    void spawnGrainForVoice (OpalCloudVoice& v, float position, float posScatter,
                              float pitchShift, float pitchScatter, float panScatter,
                              float grainSizeMs, int windowShape, float basePan,
                              bool frozen, float freezeRegion) noexcept
    {
        int bufSize = grainBuffer.getBufferSize();
        int writeHead = grainBuffer.getWriteHead();

        // Read position relative to write head
        float basePos = static_cast<float> (writeHead) - position * static_cast<float> (bufSize);
        float scatterRange = frozen ? (freezeRegion * static_cast<float> (bufSize))
                                    : static_cast<float> (bufSize);
        float scatter = v.rng.nextRange (-1.0f, 1.0f) * posScatter * scatterRange;
        float readPos = basePos + scatter;

        // Pitch
        float pShift = pitchShift + v.rng.nextRange (-1.0f, 1.0f) * pitchScatter;

        // Pan
        float panPos = basePan + v.rng.nextRange (-1.0f, 1.0f) * panScatter;

        grainPool.spawn (v.voiceIndex, readPos, pShift, grainSizeMs,
                         panPos, windowShape, sr);
    }

    // Built-in oscillator for grain source material
    float generateOscSample (int sourceMode, float shape, float shape2,
                             float osc2MixVal, float osc2Det) noexcept
    {
        // Use the first active voice's oscillator, or a default frequency
        float freq = 261.63f; // Middle C default
        for (const auto& v : voices)
        {
            if (v.active)
            {
                freq = v.glideFreq;
                break;
            }
        }

        // Configure oscillator waveforms based on source mode
        switch (sourceMode)
        {
            case 0: // Sine
                srcOsc1.setWaveform (PolyBLEP::Waveform::Sine);
                srcOsc1.setFrequency (freq, static_cast<float> (sr));
                return srcOsc1.processSample() * 0.5f;

            case 1: // Saw
                srcOsc1.setWaveform (PolyBLEP::Waveform::Saw);
                srcOsc1.setFrequency (freq, static_cast<float> (sr));
                return srcOsc1.processSample() * 0.5f;

            case 2: // Pulse
                srcOsc1.setWaveform (PolyBLEP::Waveform::Pulse);
                srcOsc1.setPulseWidth (0.1f + shape * 0.8f);
                srcOsc1.setFrequency (freq, static_cast<float> (sr));
                return srcOsc1.processSample() * 0.5f;

            case 3: // Noise
                return srcPrng.next() * 0.3f;

            case 4: // Two-Osc
            {
                srcOsc1.setWaveform (PolyBLEP::Waveform::Saw);
                srcOsc1.setFrequency (freq, static_cast<float> (sr));
                srcOsc2.setWaveform (PolyBLEP::Waveform::Saw);
                srcOsc2.setFrequency (freq * std::pow (2.0f, osc2Det / 12.0f),
                                      static_cast<float> (sr));
                float s1 = srcOsc1.processSample();
                float s2 = srcOsc2.processSample();
                return (s1 * (1.0f - osc2MixVal) + s2 * osc2MixVal) * 0.5f;
            }

            default: // Coupling (handled elsewhere)
                return 0.0f;
        }
    }

    void applyMacros (float& grainSize, float& density, float& posScatter,
                      float& pitchScatter, float& panScatter,
                      float& couplingLevel, float& freeze,
                      float& reverbMix, float& smearMix, float& delayMix) noexcept
    {
        float scatter  = safeLoadF (pMacroScatter, 0.0f);
        float drift    = safeLoadF (pMacroDrift, 0.0f);
        float coupling = safeLoadF (pMacroCoupling, 0.0f);
        float space    = safeLoadF (pMacroSpace, 0.0f);

        // M1 SCATTER: inverse exponential — small/dense to large/sparse
        if (scatter > 0.001f)
        {
            float invScatter = 1.0f - scatter;
            grainSize = 10.0f + scatter * 790.0f; // 10ms → 800ms
            density = 120.0f * invScatter * invScatter + 1.0f; // exponential inverse
        }

        // M2 DRIFT: linear dissolution
        if (drift > 0.001f)
        {
            posScatter   = std::max (posScatter,   drift);
            pitchScatter = std::max (pitchScatter, drift * 12.0f);
            panScatter   = std::max (panScatter,   drift);
        }

        // M3 COUPLING: square root response (fast initial)
        if (coupling > 0.001f)
        {
            float sqrtCoupling = std::sqrt (coupling);
            couplingLevel = std::max (couplingLevel, sqrtCoupling);
            if (coupling > 0.2f)
                freeze = std::max (freeze, (coupling - 0.2f) * 1.0f);
        }

        // M4 SPACE: all spatial FX open together
        if (space > 0.001f)
        {
            reverbMix = std::max (reverbMix, space * 0.6f);
            smearMix  = std::max (smearMix,  space * 0.5f);
            delayMix  = std::max (delayMix,  space * 0.3f);
        }
    }

    void applyModMatrix (float* offsets, float lfo1Val, float lfo2Val,
                         float filterEnvVal, float ampEnvVal, int activeCount) noexcept
    {
        if (activeCount == 0) return;

        for (int slot = 0; slot < kOpalModSlots; ++slot)
        {
            int src = safeLoad (pModSlotSrc[slot], 0);
            int dst = safeLoad (pModSlotDst[slot], 0);
            float amt = safeLoadF (pModSlotAmt[slot], 0.0f);

            if (src == 0 || dst == 0 || std::fabs (amt) < 0.001f) continue;

            float srcVal = 0.0f;
            switch (src)
            {
                case 1: srcVal = lfo1Val;      break;
                case 2: srcVal = lfo2Val;      break;
                case 3: srcVal = filterEnvVal; break;
                case 4: srcVal = ampEnvVal;    break;
                case 5: srcVal = 0.5f;         break; // Velocity (avg placeholder)
                case 6: srcVal = 0.0f;         break; // KeyTrack (placeholder)
                case 7: srcVal = 0.0f;         break; // ModWheel (placeholder)
            }

            offsets[dst] += srcVal * amt;
        }
    }

    //-- State -----------------------------------------------------------------

    double sr = 44100.0;

    // Voice pool
    std::array<OpalCloudVoice, kOpalMaxClouds> voices {};
    uint32_t voiceBirthCounter = 0;
    bool sustainPedalDown = false;

    // DSP
    OpalGrainBuffer grainBuffer;
    OpalGrainPool grainPool;
    OpalScatterReverb scatterReverb;
    OpalStereoDelay stereoDelay;
    OpalFinish finish;

    // Source oscillators (shared, write into grain buffer)
    PolyBLEP srcOsc1;
    PolyBLEP srcOsc2;
    OpalPRNG srcPrng;

    // Global filter (post-grain-mix)
    CytomicSVF globalFilterL;
    CytomicSVF globalFilterR;

    // Smear state (simplified)
    float smearStateL = 0.0f;
    float smearStateR = 0.0f;

    // Work buffers (pre-allocated in prepare)
    std::vector<float> workBufL, workBufR;
    std::vector<float> couplingBufL, couplingBufR;

    // Per-sample output cache for coupling reads (getSampleForCoupling uses these)
    std::vector<float> outputCacheLeft;
    std::vector<float> outputCacheRight;

    // Coupling modulation accumulators (reset each block)
    float extFilterMod  = 0.0f;
    float extPosMod     = 0.0f;
    float extPitchMod   = 0.0f;
    float extDensityMod = 0.0f;
    float extFreezeMod  = 0.0f;

    // ---- D006 Aftertouch — pressure increases grain position scatter ----
    PolyAftertouch aftertouch;

    // Cached coupling output (scalar fallback for block-level coupling)
    float lastSampleL = 0.0f;
    float lastSampleR = 0.0f;

    // ---- AudioToBuffer Phase 2 ---------------------------------------------------
    //
    // 4 per-slot stereo ring buffers. MegaCouplingMatrix::processAudioRoute() holds a
    // pointer to the slot-matching buffer and calls pushBlock() before renderBlock().
    // Each buffer is ~186ms at 44.1kHz (kOpalExternalBufferSeconds = 8192 / 44100).
    //
    // Slot assignment mirrors MegaCouplingMatrix source slot indices (0–3).
    // A source in slot 2 pushes into inputBuffers[2]; OpalEngine blends [0..3] in
    // receiveAudioBuffer(), summing contributions before writing to grainBuffer.
    //
    static constexpr int   kOpalInputSlots          = 4;
    static constexpr float kOpalExternalBufferSeconds = 8192.0f / 44100.0f; // ~186ms

    std::array<AudioRingBuffer, kOpalInputSlots> inputBuffers;

    // Per-block mono blend cache for external audio (populated in renderBlock
    // from inputBuffers[], consumed in the grain-source write loop).
    std::vector<float> extAudioBufL;
    std::vector<float> extAudioBufR;

    //-- Parameter pointers (87 total, opal_externalMix added Phase 2) ----------

    // Grain Source
    std::atomic<float>* pSource         = nullptr;
    std::atomic<float>* pOscShape       = nullptr;
    std::atomic<float>* pOsc2Shape      = nullptr;
    std::atomic<float>* pOsc2Mix        = nullptr;
    std::atomic<float>* pOsc2Detune     = nullptr;
    std::atomic<float>* pCouplingLevel  = nullptr;
    std::atomic<float>* pExternalMix    = nullptr;  // AudioToBuffer blend (Phase 2)
    // Grain Scheduler
    std::atomic<float>* pGrainSize      = nullptr;
    std::atomic<float>* pDensity        = nullptr;
    std::atomic<float>* pPosition       = nullptr;
    std::atomic<float>* pPosScatter     = nullptr;
    std::atomic<float>* pPitchShift     = nullptr;
    std::atomic<float>* pPitchScatter   = nullptr;
    std::atomic<float>* pPanScatter     = nullptr;
    std::atomic<float>* pWindow         = nullptr;
    // Freeze
    std::atomic<float>* pFreeze         = nullptr;
    std::atomic<float>* pFreezeSize     = nullptr;
    // Filter
    std::atomic<float>* pFilterCutoff   = nullptr;
    std::atomic<float>* pFilterReso     = nullptr;
    std::atomic<float>* pFilterMode     = nullptr;
    std::atomic<float>* pFilterKeyTrack = nullptr;
    std::atomic<float>* pFilterDrive    = nullptr;
    // Character
    std::atomic<float>* pShimmer        = nullptr;
    std::atomic<float>* pFrost          = nullptr;
    // Amp Envelope
    std::atomic<float>* pAmpAttack      = nullptr;
    std::atomic<float>* pAmpDecay       = nullptr;
    std::atomic<float>* pAmpSustain     = nullptr;
    std::atomic<float>* pAmpRelease     = nullptr;
    std::atomic<float>* pAmpVelSens     = nullptr;
    // Filter Envelope
    std::atomic<float>* pFilterEnvAmt   = nullptr;
    std::atomic<float>* pFilterAttack   = nullptr;
    std::atomic<float>* pFilterDecay    = nullptr;
    std::atomic<float>* pFilterSustain  = nullptr;
    std::atomic<float>* pFilterRelease  = nullptr;
    // LFO 1
    std::atomic<float>* pLfo1Shape      = nullptr;
    std::atomic<float>* pLfo1Rate       = nullptr;
    std::atomic<float>* pLfo1Depth      = nullptr;
    std::atomic<float>* pLfo1Sync       = nullptr;
    std::atomic<float>* pLfo1Retrigger  = nullptr;
    std::atomic<float>* pLfo1Phase      = nullptr;
    // LFO 2
    std::atomic<float>* pLfo2Shape      = nullptr;
    std::atomic<float>* pLfo2Rate       = nullptr;
    std::atomic<float>* pLfo2Depth      = nullptr;
    std::atomic<float>* pLfo2Sync       = nullptr;
    std::atomic<float>* pLfo2Retrigger  = nullptr;
    std::atomic<float>* pLfo2Phase      = nullptr;
    // Mod Matrix
    std::atomic<float>* pModSlotSrc[kOpalModSlots] = {};
    std::atomic<float>* pModSlotDst[kOpalModSlots] = {};
    std::atomic<float>* pModSlotAmt[kOpalModSlots] = {};
    // Macros
    std::atomic<float>* pMacroScatter   = nullptr;
    std::atomic<float>* pMacroDrift     = nullptr;
    std::atomic<float>* pMacroCoupling  = nullptr;
    std::atomic<float>* pMacroSpace     = nullptr;
    // FX
    std::atomic<float>* pFxSmearAmt     = nullptr;
    std::atomic<float>* pFxSmearMix     = nullptr;
    std::atomic<float>* pFxReverbSize   = nullptr;
    std::atomic<float>* pFxReverbDecay  = nullptr;
    std::atomic<float>* pFxReverbDamp   = nullptr;
    std::atomic<float>* pFxReverbMix    = nullptr;
    std::atomic<float>* pFxDelayTime    = nullptr;
    std::atomic<float>* pFxDelayFB      = nullptr;
    std::atomic<float>* pFxDelayMix     = nullptr;
    std::atomic<float>* pFxDelaySync    = nullptr;
    std::atomic<float>* pFxDelaySpread  = nullptr;
    std::atomic<float>* pFxFinishGlue   = nullptr;
    std::atomic<float>* pFxFinishWidth  = nullptr;
    std::atomic<float>* pFxFinishLevel  = nullptr;
    // Voice
    std::atomic<float>* pVoiceMode      = nullptr;
    std::atomic<float>* pGlideTime      = nullptr;
    std::atomic<float>* pGlideMode      = nullptr;
    std::atomic<float>* pPan            = nullptr;
    std::atomic<float>* pLevel          = nullptr;
};

} // namespace xomnibus
