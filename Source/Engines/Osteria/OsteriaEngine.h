#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/ShoreSystem/ShoreSystem.h"
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>

namespace xomnibus {

//==============================================================================
// OsteriaEngine — Ensemble Synthesis with Elastic Coupling & Timbral Memory.
//
// A jazz quartet stretches across coastal cultures, absorbing folk instrument
// character through elastic rubber-band coupling. Four independent voice
// channels (bass, harmony, melody, rhythm) each with their own shore position,
// connected by spring forces. Cross-pollination memory means borrowed
// influences persist — the quartet accumulates a living history of everywhere
// it's been.
//
// Features:
//   - 4 quartet channels per MIDI voice (Bass, Harmony, Melody, Rhythm)
//   - Per-channel shore position (0-4) with continuous morphing
//   - Elastic coupling: spring-force model between channels
//   - Timbral memory: circular buffer of shore history per channel
//   - Tavern room model: FDN reverb per-shore acoustic character
//   - Murmur generator: crowd/conversation texture
//   - Character stages: Patina, Porto, Smoke
//   - Session delay, chorus, hall, tape FX
//   - 8-voice polyphony with LRU stealing + 5ms crossfade
//
// Coupling:
//   - Output: post-room stereo audio via getSampleForCoupling
//   - Input: AudioToWavetable (shapes quartet character),
//            AmpToFilter (modulates elastic tightness),
//            AudioToFM (excites tavern room), EnvToMorph (drives shore drift)
//
//==============================================================================

//==============================================================================
// Constants
//==============================================================================
static constexpr int kOsteriaMaxVoices = 8;
static constexpr float kOsteriaPI = 3.14159265358979323846f;
static constexpr float kOsteriaTwoPi = 6.28318530717958647692f;
static constexpr int kMemoryBufferSize = 32;

//==============================================================================
// Quartet roles
//==============================================================================
enum class QuartetRole : int { Bass = 0, Harmony = 1, Melody = 2, Rhythm = 3 };

//==============================================================================
// OsteriaADSR — lightweight inline ADSR envelope.
//==============================================================================
struct OsteriaADSR
{
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    Stage stage = Stage::Idle;
    float level = 0.0f;
    float attackRate  = 0.0f;
    float decayRate   = 0.0f;
    float sustainLevel = 1.0f;
    float releaseRate = 0.0f;

    void setParams (float attackSec, float decaySec, float sustain, float releaseSec,
                    float sampleRate) noexcept
    {
        float sr = std::max (1.0f, sampleRate);
        attackRate  = (attackSec  > 0.001f) ? (1.0f / (attackSec  * sr)) : 1.0f;
        decayRate   = (decaySec   > 0.001f) ? (1.0f / (decaySec   * sr)) : 1.0f;
        sustainLevel = sustain;
        releaseRate = (releaseSec > 0.001f) ? (1.0f / (releaseSec * sr)) : 1.0f;
    }

    void noteOn() noexcept { stage = Stage::Attack; }

    void noteOff() noexcept
    {
        if (stage != Stage::Idle)
            stage = Stage::Release;
    }

    float process() noexcept
    {
        switch (stage)
        {
            case Stage::Idle:    return 0.0f;
            case Stage::Attack:
                level += attackRate;
                if (level >= 1.0f) { level = 1.0f; stage = Stage::Decay; }
                return level;
            case Stage::Decay:
                level -= decayRate * (level - sustainLevel + 0.0001f);
                if (level <= sustainLevel + 0.0001f) { level = sustainLevel; stage = Stage::Sustain; }
                return level;
            case Stage::Sustain: return level;
            case Stage::Release:
                level -= releaseRate * (level + 0.0001f);
                if (level <= 0.0001f) { level = 0.0f; stage = Stage::Idle; }
                return level;
        }
        return 0.0f;
    }

    bool isActive() const noexcept { return stage != Stage::Idle; }
    void reset() noexcept { stage = Stage::Idle; level = 0.0f; }
};

//==============================================================================
// QuartetChannel — one of four ensemble voices with shore-morphed formants.
//==============================================================================
struct QuartetChannel
{
    QuartetRole role = QuartetRole::Bass;
    float shorePos = 0.0f;
    float targetShorePos = 0.0f;
    float shoreVelocity = 0.0f;

    // 4 formant bandpass filters
    CytomicSVF formants[4];
    float formantFreqs[4] = { 300.0f, 1200.0f, 2800.0f, 5500.0f };
    float formantGains[4] = { 1.0f, 0.7f, 0.5f, 0.3f };
    float formantBandwidths[4] = { 80.0f, 150.0f, 200.0f, 300.0f };

    // Timbral memory
    float memoryBuffer[kMemoryBufferSize] = {};
    int memoryWritePos = 0;
    float memoryCachedFormantFreqs[4] = { 300.0f, 1200.0f, 2800.0f, 5500.0f };

    // Per-channel oscillator phase
    float oscPhase = 0.0f;
    float oscPhase2 = 0.0f; // second oscillator for shimmer/detune
    float noiseState = 0.0f;

    // Rhythm transient envelope
    float transientEnv = 0.0f;
    float transientPhase = 0.0f;

    // Level and pan
    float level = 1.0f;
    float pan = 0.0f;

    // Per-channel output (for sympathy crossfeed)
    float lastOutputL = 0.0f;
    float lastOutputR = 0.0f;

    uint32_t rng = 22222u;

    float nextRandom() noexcept
    {
        rng = rng * 1664525u + 1013904223u;
        return static_cast<float> (rng & 0xFFFF) / 32768.0f - 1.0f;
    }

    void recordShorePosition() noexcept
    {
        memoryBuffer[memoryWritePos] = shorePos;
        memoryWritePos = (memoryWritePos + 1) % kMemoryBufferSize;
    }

    void applyMemory (float memoryAmount, float sampleRate) noexcept
    {
        if (memoryAmount < 0.001f) return;

        // Compute average historical shore position
        float avgShore = 0.0f;
        for (int i = 0; i < kMemoryBufferSize; ++i)
            avgShore += memoryBuffer[i];
        avgShore /= static_cast<float> (kMemoryBufferSize);

        // Morph memory shore's resonator into cached formants
        ShoreMorphState memMorph = decomposeShore (avgShore);
        int slot = static_cast<int> (role);
        if (slot > 2) slot = 0; // rhythm uses bass slot
        ResonatorProfile memProfile = morphResonator (memMorph, slot);

        for (int i = 0; i < 4; ++i)
        {
            memoryCachedFormantFreqs[i] = lerp (formantFreqs[i],
                                                memProfile.formantFreqs[i],
                                                memoryAmount);
        }
    }

    void updateFormants (float sampleRate, float memoryAmount) noexcept
    {
        for (int i = 0; i < 4; ++i)
        {
            float freq = (memoryAmount > 0.001f) ? memoryCachedFormantFreqs[i] : formantFreqs[i];
            float bw = formantBandwidths[i];
            float resonance = clamp (1.0f - (bw / std::max (freq, 20.0f)), 0.0f, 0.95f);
            formants[i].setMode (CytomicSVF::Mode::BandPass);
            formants[i].setCoefficients (freq, resonance, sampleRate);
        }
    }

    void reset() noexcept
    {
        shorePos = 0.0f;
        targetShorePos = 0.0f;
        shoreVelocity = 0.0f;
        oscPhase = 0.0f;
        oscPhase2 = 0.0f;
        noiseState = 0.0f;
        transientEnv = 0.0f;
        transientPhase = 0.0f;
        lastOutputL = 0.0f;
        lastOutputR = 0.0f;
        memoryWritePos = 0;
        for (int i = 0; i < kMemoryBufferSize; ++i) memoryBuffer[i] = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            formants[i].reset();
            memoryCachedFormantFreqs[i] = formantFreqs[i];
        }
    }
};

//==============================================================================
// TavernRoom — FDN reverb modeling tavern acoustics.
//==============================================================================
struct TavernRoom
{
    static constexpr int kMaxDelay = 4096;
    float delayBuf[4][kMaxDelay] = {};
    int delayWritePos = 0;
    int delayLengths[4] = { 337, 509, 677, 883 };
    float feedback = 0.3f;
    CytomicSVF absorptionFilter;
    float warmthGain = 1.0f;

    void setCharacter (const TavernCharacter& tc, float mix, float sampleRate) noexcept
    {
        // Scale delay lengths by room size
        float roomScale = tc.roomSizeMs / 15.0f;
        delayLengths[0] = std::max (1, std::min (kMaxDelay - 1, static_cast<int> (337.0f * roomScale)));
        delayLengths[1] = std::max (1, std::min (kMaxDelay - 1, static_cast<int> (509.0f * roomScale)));
        delayLengths[2] = std::max (1, std::min (kMaxDelay - 1, static_cast<int> (677.0f * roomScale)));
        delayLengths[3] = std::max (1, std::min (kMaxDelay - 1, static_cast<int> (883.0f * roomScale)));

        // Feedback from RT60
        float rt60Samples = tc.decayMs * 0.001f * sampleRate;
        feedback = (rt60Samples > 0.0f) ? std::pow (0.001f, static_cast<float> (delayLengths[0]) / rt60Samples) : 0.0f;
        feedback = clamp (feedback, 0.0f, 0.85f);

        // Absorption filter (LP)
        float cutoff = lerp (2000.0f, 12000.0f, 1.0f - tc.absorption);
        absorptionFilter.setMode (CytomicSVF::Mode::LowPass);
        absorptionFilter.setCoefficients (cutoff, 0.0f, sampleRate);

        warmthGain = 1.0f + tc.warmth * 0.5f;
    }

    void processSample (float& inL, float& inR, float mix) noexcept
    {
        if (mix < 0.001f) return;

        float input = (inL + inR) * 0.5f;

        // Read from delays
        float d0 = readDelay (0);
        float d1 = readDelay (1);
        float d2 = readDelay (2);
        float d3 = readDelay (3);

        // Householder-like mixing
        float sum = (d0 + d1 + d2 + d3) * 0.25f;
        float fb0 = sum - d0 * 0.5f;
        float fb1 = sum - d1 * 0.5f;
        float fb2 = sum - d2 * 0.5f;
        float fb3 = sum - d3 * 0.5f;

        // Apply absorption
        float filtered = absorptionFilter.processSample (sum);

        // Write to delays with feedback
        writeDelay (0, input + flushDenormal (fb0 * feedback));
        writeDelay (1, input * 0.7f + flushDenormal (fb1 * feedback));
        writeDelay (2, input * 0.5f + flushDenormal (fb2 * feedback));
        writeDelay (3, input * 0.3f + flushDenormal (fb3 * feedback));

        delayWritePos = (delayWritePos + 1) % kMaxDelay;

        // Mix wet signal
        float wetL = (d0 + d2) * 0.5f * warmthGain;
        float wetR = (d1 + d3) * 0.5f * warmthGain;

        inL = lerp (inL, inL + wetL, mix);
        inR = lerp (inR, inR + wetR, mix);
    }

    float readDelay (int line) const noexcept
    {
        int readPos = delayWritePos - delayLengths[line];
        if (readPos < 0) readPos += kMaxDelay;
        return delayBuf[line][readPos];
    }

    void writeDelay (int line, float value) noexcept
    {
        delayBuf[line][delayWritePos] = value;
    }

    void reset() noexcept
    {
        delayWritePos = 0;
        for (int l = 0; l < 4; ++l)
            for (int i = 0; i < kMaxDelay; ++i)
                delayBuf[l][i] = 0.0f;
        absorptionFilter.reset();
    }
};

//==============================================================================
// MurmurGenerator — crowd/conversation texture.
//==============================================================================
struct MurmurGenerator
{
    uint32_t rng = 77777u;
    CytomicSVF formant1;
    CytomicSVF formant2;
    float modPhase = 0.0f;

    void prepare (float sampleRate) noexcept
    {
        formant1.setMode (CytomicSVF::Mode::BandPass);
        formant1.setCoefficients (350.0f, 0.4f, sampleRate);
        formant2.setMode (CytomicSVF::Mode::BandPass);
        formant2.setCoefficients (2500.0f, 0.3f, sampleRate);
    }

    float process (float brightness, float sampleRate) noexcept
    {
        // Generate noise
        rng = rng * 1664525u + 1013904223u;
        float noise = static_cast<float> (rng & 0xFFFF) / 32768.0f - 1.0f;

        // Slow modulation of formant positions
        modPhase += 0.5f / std::max (1.0f, sampleRate);
        if (modPhase >= 1.0f) modPhase -= 1.0f;
        float mod = fastSin (modPhase * kOsteriaTwoPi);

        float f1Freq = 350.0f + mod * 50.0f;
        float f2Freq = lerp (2000.0f, 4000.0f, brightness) + mod * 200.0f;
        formant1.setCoefficients (f1Freq, 0.4f, sampleRate);
        formant2.setCoefficients (f2Freq, 0.3f, sampleRate);

        float out = formant1.processSample (noise) * 0.6f
                  + formant2.processSample (noise) * 0.4f;

        return out * 0.15f;
    }

    void reset() noexcept
    {
        formant1.reset();
        formant2.reset();
        modPhase = 0.0f;
        rng = 77777u;
    }
};

//==============================================================================
// OsteriaVoice — per-MIDI-note state with quartet channels.
//==============================================================================
struct OsteriaVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;
    float targetFreq = 440.0f;
    float currentTargetFreq = 440.0f;
    float glideCoeff = 1.0f;

    std::array<QuartetChannel, 4> quartet;

    OsteriaADSR ampEnv;

    float fadeGain = 1.0f;
    bool fadingOut = false;
    int controlCounter = 0;
    uint32_t rng = 12345u;

    float dcPrevInL = 0.0f, dcPrevOutL = 0.0f;
    float dcPrevInR = 0.0f, dcPrevOutR = 0.0f;

    float nextRandom() noexcept
    {
        rng = rng * 1664525u + 1013904223u;
        return static_cast<float> (rng & 0xFFFF) / 32768.0f - 1.0f;
    }

    float nextRandomUni() noexcept
    {
        rng = rng * 1664525u + 1013904223u;
        return static_cast<float> (rng & 0xFFFF) / 65536.0f;
    }

    void reset() noexcept
    {
        active = false;
        noteNumber = -1;
        velocity = 0.0f;
        targetFreq = 440.0f;
        currentTargetFreq = 440.0f;
        fadeGain = 1.0f;
        fadingOut = false;
        controlCounter = 0;
        rng = 12345u;
        dcPrevInL = 0.0f; dcPrevOutL = 0.0f;
        dcPrevInR = 0.0f; dcPrevOutR = 0.0f;
        ampEnv.reset();
        for (auto& ch : quartet)
            ch.reset();
    }
};

//==============================================================================
// OsteriaEngine — the main engine class.
//==============================================================================
class OsteriaEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = kOsteriaMaxVoices;

    //==========================================================================
    // SynthEngine interface — Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        controlRateDiv = std::max (1, static_cast<int> (srf / 2000.0f));
        controlDt = static_cast<float> (controlRateDiv) / srf;

        crossfadeRate = 1.0f / (0.005f * srf);

        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        // Initialize voices
        for (auto& v : voices) v.reset();

        // Initialize quartet roles
        for (auto& v : voices)
        {
            v.quartet[0].role = QuartetRole::Bass;
            v.quartet[1].role = QuartetRole::Harmony;
            v.quartet[2].role = QuartetRole::Melody;
            v.quartet[3].role = QuartetRole::Rhythm;
        }

        // Initialize tavern room and murmur
        tavernRoom.reset();
        murmur.prepare (srf);

        // Initialize post filters
        smokeFilter.setMode (CytomicSVF::Mode::LowPass);
        smokeFilter.setCoefficients (8000.0f, 0.0f, srf);
        warmthFilter.setMode (CytomicSVF::Mode::LowShelf);
        warmthFilter.setCoefficients (300.0f, 0.0f, srf, 3.0f);

        // Session delay
        sessionDelayWritePos = 0;
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < kSessionDelayMax; ++i)
                sessionDelayBuf[c][i] = 0.0f;

        // Chorus
        chorusWritePos = 0;
        chorusPhase = 0.0f;
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < kChorusBufSize; ++i)
                chorusBuf[c][i] = 0.0f;

        // Hall (allpass)
        for (int i = 0; i < 4; ++i)
        {
            hallWritePos[i] = 0;
            for (int s = 0; s < kHallDelayMax; ++s)
                hallBuf[i][s] = 0.0f;
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices) v.reset();
        envelopeOutput = 0.0f;
        couplingExcitationMod = 0.0f;
        couplingElasticMod = 0.0f;
        couplingRoomExcitation = 0.0f;
        couplingShoreDrift = 0.0f;
        tavernRoom.reset();
        murmur.reset();
        smokeFilter.reset();
        warmthFilter.reset();

        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);

        sessionDelayWritePos = 0;
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < kSessionDelayMax; ++i)
                sessionDelayBuf[c][i] = 0.0f;

        chorusWritePos = 0;
        chorusPhase = 0.0f;
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < kChorusBufSize; ++i)
                chorusBuf[c][i] = 0.0f;

        for (int i = 0; i < 4; ++i)
        {
            hallWritePos[i] = 0;
            for (int s = 0; s < kHallDelayMax; ++s)
                hallBuf[i][s] = 0.0f;
        }
    }

    //==========================================================================
    // SynthEngine interface — Audio
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        // --- ParamSnapshot ---
        const float pBassShore    = loadParam (paramBassShore, 0.0f);
        const float pHarmShore    = loadParam (paramHarmShore, 0.0f);
        const float pMelShore     = loadParam (paramMelShore, 0.0f);
        const float pRhythmShore  = loadParam (paramRhythmShore, 0.0f);
        const float pElastic      = loadParam (paramElastic, 0.5f);
        const float pStretch      = loadParam (paramStretch, 0.5f);
        const float pMemory       = loadParam (paramMemory, 0.3f);
        const float pSympathy     = loadParam (paramSympathy, 0.3f);

        const float pBassLvl      = loadParam (paramBassLevel, 0.8f);
        const float pHarmLvl      = loadParam (paramHarmLevel, 0.7f);
        const float pMelLvl       = loadParam (paramMelLevel, 0.7f);
        const float pRhythmLvl    = loadParam (paramRhythmLevel, 0.6f);
        const float pEnsWidth     = loadParam (paramEnsWidth, 0.5f);
        const float pBlend        = loadParam (paramBlendMode, 0.0f);

        const float pTavernMix    = loadParam (paramTavernMix, 0.3f);
        const float pTavernShore  = loadParam (paramTavernShore, 0.0f);
        const float pMurmur       = loadParam (paramMurmur, 0.2f);
        const float pWarmth       = loadParam (paramWarmth, 0.5f);
        const float pOceanBleed   = loadParam (paramOceanBleed, 0.1f);

        const float pPatina       = loadParam (paramPatina, 0.2f);
        const float pPorto        = loadParam (paramPorto, 0.0f);
        const float pSmoke        = loadParam (paramSmoke, 0.1f);

        const float pAmpA         = loadParam (paramAttack, 0.05f);
        const float pAmpD         = loadParam (paramDecay, 0.3f);
        const float pAmpS         = loadParam (paramSustain, 0.7f);
        const float pAmpR         = loadParam (paramRelease, 1.0f);

        const float pDelay        = loadParam (paramSessionDelay, 0.2f);
        const float pHall         = loadParam (paramHall, 0.2f);
        const float pChorus       = loadParam (paramChorus, 0.1f);
        const float pTape         = loadParam (paramTape, 0.0f);

        const float macroChar     = loadParam (paramMacroCharacter, 0.0f);
        const float macroMove     = loadParam (paramMacroMovement, 0.0f);
        const float macroCoup     = loadParam (paramMacroCoupling, 0.0f);
        const float macroSpace    = loadParam (paramMacroSpace, 0.0f);

        // --- Apply macros ---
        float effectiveBlend   = clamp (pBlend + macroChar * 0.8f, 0.0f, 1.0f);
        float convergence      = macroChar;
        float effectiveElastic = clamp (pElastic - macroMove * 0.7f, 0.0f, 1.0f);
        float effectiveStretch = clamp (pStretch + macroMove * 0.4f, 0.01f, 1.0f);
        float effectiveSympathy = clamp (pSympathy + macroCoup * 0.5f, 0.0f, 1.0f);
        float effectiveMemory  = clamp (pMemory + macroCoup * 0.5f, 0.0f, 1.0f);
        float effectiveTavern  = clamp (pTavernMix + macroSpace * 0.6f, 0.0f, 1.0f);
        float effectiveHall    = clamp (pHall + macroSpace * 0.5f, 0.0f, 1.0f);
        float effectiveBleed   = clamp (pOceanBleed + macroSpace * 0.5f, 0.0f, 1.0f);

        // Channel levels
        const float channelLevels[4] = { pBassLvl, pHarmLvl, pMelLvl, pRhythmLvl };

        // Channel pans (spread by ensWidth)
        const float channelPans[4] = {
            -0.3f * pEnsWidth,
            -0.1f * pEnsWidth,
             0.2f * pEnsWidth,
             0.4f * pEnsWidth
        };

        // Shore targets per channel
        float shoreTargets[4] = { pBassShore, pHarmShore, pMelShore, pRhythmShore };

        // Apply convergence (M1): blend toward centroid
        if (convergence > 0.001f)
        {
            float centroid = (shoreTargets[0] + shoreTargets[1] + shoreTargets[2] + shoreTargets[3]) * 0.25f;
            for (int i = 0; i < 4; ++i)
                shoreTargets[i] = lerp (shoreTargets[i], centroid, convergence);
        }

        // Apply coupling shore drift
        float driftOffset = couplingShoreDrift;
        couplingShoreDrift = 0.0f;
        for (int i = 0; i < 4; ++i)
            shoreTargets[i] = clamp (shoreTargets[i] + driftOffset, 0.0f, 4.0f);

        // Setup tavern room
        ShoreMorphState tavernMorph = decomposeShore (pTavernShore);
        TavernCharacter tc = morphTavern (tavernMorph);
        tavernRoom.setCharacter (tc, effectiveTavern, srf);

        // Setup smoke filter
        float smokeCutoff = lerp (18000.0f, 3000.0f, pSmoke);
        smokeFilter.setCoefficients (smokeCutoff, 0.0f, srf);

        // Setup warmth filter
        float warmthDb = pWarmth * 8.0f;
        warmthFilter.setCoefficients (300.0f, 0.0f, srf, warmthDb);

        // Reset coupling accumulators
        float excitationMod = couplingExcitationMod;
        couplingExcitationMod = 0.0f;
        float elasticMod = couplingElasticMod;
        couplingElasticMod = 0.0f;
        float roomExcitation = couplingRoomExcitation;
        couplingRoomExcitation = 0.0f;

        // Effective elastic with coupling modulation
        effectiveElastic = clamp (effectiveElastic + elasticMod * 0.3f, 0.0f, 1.0f);

        // Glide coefficient
        float glideCoeff = 1.0f;

        // --- Process MIDI ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(),
                        shoreTargets, channelLevels, channelPans,
                        pAmpA, pAmpD, pAmpS, pAmpR);
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
        }

        float peakEnv = 0.0f;

        // --- Render sample loop ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                // Voice stealing crossfade
                if (voice.fadingOut)
                {
                    voice.fadeGain -= crossfadeRate;
                    if (voice.fadeGain <= 0.0f) { voice.fadeGain = 0.0f; voice.active = false; continue; }
                }

                // Glide
                voice.currentTargetFreq += (voice.targetFreq - voice.currentTargetFreq) * voice.glideCoeff;

                // Envelope
                float ampLevel = voice.ampEnv.process();
                if (!voice.ampEnv.isActive()) { voice.active = false; continue; }

                // --- Control-rate update ---
                voice.controlCounter++;
                if (voice.controlCounter >= controlRateDiv)
                {
                    voice.controlCounter = 0;

                    // Elastic coupling: compute centroid
                    float centroid = 0.0f;
                    for (int c = 0; c < 4; ++c)
                        centroid += voice.quartet[c].shorePos;
                    centroid *= 0.25f;

                    // Apply spring forces
                    float springK = effectiveElastic * 4.0f;
                    float stretchThreshold = effectiveStretch * 2.0f + 0.5f;

                    for (int c = 0; c < 4; ++c)
                    {
                        auto& ch = voice.quartet[c];
                        ch.targetShorePos = shoreTargets[c];

                        // Spring toward centroid
                        float dist = centroid - ch.shorePos;
                        float force = springK * dist;

                        // Nonlinear increase past stretch limit
                        float absDist = std::fabs (dist);
                        if (absDist > stretchThreshold)
                        {
                            float excess = absDist - stretchThreshold;
                            force += (dist > 0.0f ? 1.0f : -1.0f) * excess * excess * 2.0f;
                        }

                        // Also pull toward target position
                        float targetPull = (ch.targetShorePos - ch.shorePos) * 2.0f;
                        force += targetPull;

                        ch.shoreVelocity += force * controlDt;
                        ch.shoreVelocity *= 0.95f; // damping
                        ch.shorePos += ch.shoreVelocity * controlDt;
                        ch.shorePos = clamp (ch.shorePos, 0.0f, 4.0f);
                        ch.shoreVelocity = flushDenormal (ch.shoreVelocity);
                    }

                    // Update formants per channel based on current shore
                    for (int c = 0; c < 4; ++c)
                    {
                        auto& ch = voice.quartet[c];
                        ShoreMorphState morph = decomposeShore (ch.shorePos);
                        int slot = c;
                        if (slot > 2) slot = 0;
                        ResonatorProfile prof = morphResonator (morph, slot);

                        // Scale formant frequencies relative to the note
                        float noteRatio = voice.currentTargetFreq / 440.0f;
                        for (int f = 0; f < 4; ++f)
                        {
                            ch.formantFreqs[f] = prof.formantFreqs[f] * noteRatio;
                            ch.formantFreqs[f] = clamp (ch.formantFreqs[f], 20.0f, 18000.0f);
                            ch.formantGains[f] = prof.formantGains[f];
                            ch.formantBandwidths[f] = prof.formantBandwidths[f];
                        }

                        // Apply timbral memory
                        ch.applyMemory (effectiveMemory, srf);
                        ch.updateFormants (srf, effectiveMemory);

                        // Record shore position
                        ch.recordShorePosition();
                    }

                    // Apply blend mode: at blend=1, force all channels toward centroid shore
                    if (effectiveBlend > 0.001f)
                    {
                        float blendCentroid = 0.0f;
                        for (int c = 0; c < 4; ++c)
                            blendCentroid += voice.quartet[c].shorePos;
                        blendCentroid *= 0.25f;

                        for (int c = 0; c < 4; ++c)
                        {
                            auto& ch = voice.quartet[c];
                            ch.shorePos = lerp (ch.shorePos, blendCentroid, effectiveBlend * 0.3f);
                        }
                    }
                }

                // --- Audio-rate: render quartet ---
                float voiceL = 0.0f, voiceR = 0.0f;
                float freq = voice.currentTargetFreq;
                float phaseInc = freq / srf;

                for (int c = 0; c < 4; ++c)
                {
                    auto& ch = voice.quartet[c];
                    float excitation = 0.0f;

                    switch (static_cast<QuartetRole> (c))
                    {
                        case QuartetRole::Bass:
                        {
                            // Fundamental + sub
                            float sin1 = fastSin (ch.oscPhase * kOsteriaTwoPi);
                            float sin2 = fastSin (ch.oscPhase * 0.5f * kOsteriaTwoPi); // sub
                            excitation = sin1 * 0.7f + sin2 * 0.3f;
                            ch.oscPhase += phaseInc;
                            if (ch.oscPhase >= 1.0f) ch.oscPhase -= 1.0f;
                            break;
                        }
                        case QuartetRole::Harmony:
                        {
                            // Mid partials with slight detune
                            float sin1 = fastSin (ch.oscPhase * kOsteriaTwoPi * 1.5f);
                            float sin2 = fastSin (ch.oscPhase2 * kOsteriaTwoPi * 2.003f); // detuned octave
                            excitation = sin1 * 0.5f + sin2 * 0.5f;
                            ch.oscPhase += phaseInc;
                            ch.oscPhase2 += phaseInc;
                            if (ch.oscPhase >= 1.0f) ch.oscPhase -= 1.0f;
                            if (ch.oscPhase2 >= 1.0f) ch.oscPhase2 -= 1.0f;
                            break;
                        }
                        case QuartetRole::Melody:
                        {
                            // Upper partials with brightness
                            float sin1 = fastSin (ch.oscPhase * kOsteriaTwoPi * 2.0f);
                            float sin2 = fastSin (ch.oscPhase2 * kOsteriaTwoPi * 3.01f);
                            float sin3 = fastSin (ch.oscPhase * kOsteriaTwoPi * 4.02f);
                            excitation = sin1 * 0.4f + sin2 * 0.35f + sin3 * 0.25f;
                            ch.oscPhase += phaseInc;
                            ch.oscPhase2 += phaseInc;
                            if (ch.oscPhase >= 1.0f) ch.oscPhase -= 1.0f;
                            if (ch.oscPhase2 >= 1.0f) ch.oscPhase2 -= 1.0f;
                            break;
                        }
                        case QuartetRole::Rhythm:
                        {
                            // Noise + transient at pulse rate
                            ShoreMorphState rm = decomposeShore (ch.shorePos);
                            ShoreRhythm rhythm = morphRhythm (rm);

                            ch.transientPhase += rhythm.pulseRate / srf;
                            if (ch.transientPhase >= 1.0f)
                            {
                                ch.transientPhase -= 1.0f;
                                ch.transientEnv = 1.0f;
                            }
                            ch.transientEnv *= (1.0f - 8.0f / srf); // fast decay
                            ch.transientEnv = flushDenormal (ch.transientEnv);

                            float noise = ch.nextRandom();
                            excitation = noise * ch.transientEnv;
                            break;
                        }
                    }

                    // Add coupling excitation
                    excitation += excitationMod * 0.3f;

                    // Run through formant filters
                    float channelOut = 0.0f;
                    for (int f = 0; f < 4; ++f)
                    {
                        float filtered = ch.formants[f].processSample (excitation);
                        channelOut += filtered * ch.formantGains[f];
                    }

                    // Apply level
                    channelOut *= channelLevels[c];

                    // Stereo panning (constant power)
                    float panAngle = (channelPans[c] + 1.0f) * 0.25f * kOsteriaPI;
                    float panL = std::cos (panAngle);
                    float panR = std::sin (panAngle);

                    float chL = channelOut * panL;
                    float chR = channelOut * panR;

                    // Store for sympathy
                    ch.lastOutputL = chL;
                    ch.lastOutputR = chR;

                    voiceL += chL;
                    voiceR += chR;
                }

                // Apply sympathy crossfeed
                if (effectiveSympathy > 0.001f)
                {
                    float sympGain = effectiveSympathy * 0.15f;
                    for (int c = 0; c < 4; ++c)
                    {
                        auto& ch = voice.quartet[c];
                        float sympInput = 0.0f;
                        for (int other = 0; other < 4; ++other)
                        {
                            if (other == c) continue;
                            sympInput += (voice.quartet[other].lastOutputL + voice.quartet[other].lastOutputR) * 0.5f;
                        }
                        // Feed sympathy back through the first formant
                        float sympOut = ch.formants[0].processSample (sympInput * sympGain * 0.3f);
                        voiceL += sympOut * 0.3f;
                        voiceR += sympOut * 0.3f;
                    }
                }

                // DC Blocker
                constexpr float dcCoeff = 0.9975f;
                float dcOutL = voiceL - voice.dcPrevInL + dcCoeff * voice.dcPrevOutL;
                float dcOutR = voiceR - voice.dcPrevInR + dcCoeff * voice.dcPrevOutR;
                voice.dcPrevInL = voiceL;
                voice.dcPrevOutL = flushDenormal (dcOutL);
                voice.dcPrevInR = voiceR;
                voice.dcPrevOutR = flushDenormal (dcOutR);
                voiceL = dcOutL;
                voiceR = dcOutR;

                // Soft limiter
                voiceL = fastTanh (voiceL * 1.5f);
                voiceR = fastTanh (voiceR * 1.5f);

                // Apply envelope, velocity, crossfade
                float gain = ampLevel * voice.velocity * voice.fadeGain;
                voiceL *= gain;
                voiceR *= gain;

                voiceL = flushDenormal (voiceL);
                voiceR = flushDenormal (voiceR);

                mixL += voiceL;
                mixR += voiceR;

                peakEnv = std::max (peakEnv, ampLevel);
            }

            // --- Post-voice processing ---

            // Character: Patina (gentle harmonic fold)
            if (pPatina > 0.001f)
            {
                float driveP = 1.0f + pPatina * 3.0f;
                mixL = softClip (mixL * driveP) / driveP;
                mixR = softClip (mixR * driveP) / driveP;
            }

            // Character: Porto (warm saturation)
            if (pPorto > 0.001f)
            {
                float driveW = 1.0f + pPorto * 4.0f;
                mixL = fastTanh (mixL * driveW) / driveW;
                mixR = fastTanh (mixR * driveW) / driveW;
            }

            // Character: Smoke (HF haze)
            mixL = smokeFilter.processSample (mixL);
            mixR = smokeFilter.processSample (mixR);

            // Warmth
            mixL = warmthFilter.processSample (mixL);

            // Tavern room model
            tavernRoom.processSample (mixL, mixR, effectiveTavern);

            // Murmur
            if (pMurmur > 0.001f)
            {
                ShoreMorphState tmm = decomposeShore (pTavernShore);
                TavernCharacter tcc = morphTavern (tmm);
                float murmurSample = murmur.process (tcc.murmurBrightness, srf);
                mixL += murmurSample * pMurmur;
                mixR += murmurSample * pMurmur * 0.9f; // slight stereo
            }

            // Session Delay
            if (pDelay > 0.001f)
            {
                int delayTime = std::max (1, std::min (kSessionDelayMax - 1,
                    static_cast<int> (0.15f * srf))); // ~150ms
                int readPos = sessionDelayWritePos - delayTime;
                if (readPos < 0) readPos += kSessionDelayMax;

                float delL = sessionDelayBuf[0][readPos];
                float delR = sessionDelayBuf[1][readPos];

                sessionDelayBuf[0][sessionDelayWritePos] = mixL + delL * 0.35f;
                sessionDelayBuf[1][sessionDelayWritePos] = mixR + delR * 0.35f;
                sessionDelayWritePos = (sessionDelayWritePos + 1) % kSessionDelayMax;

                mixL += delL * pDelay;
                mixR += delR * pDelay;
            }

            // Hall (simple allpass chain)
            if (effectiveHall > 0.001f)
            {
                float hallIn = (mixL + mixR) * 0.5f;
                float hallOut = processHallAllpass (hallIn, effectiveHall);
                mixL += hallOut * effectiveHall * 0.5f;
                mixR += hallOut * effectiveHall * 0.5f;
            }

            // Chorus
            if (pChorus > 0.001f)
            {
                chorusPhase += 0.5f / srf;
                if (chorusPhase >= 1.0f) chorusPhase -= 1.0f;
                float modMs = 8.0f + fastSin (chorusPhase * kOsteriaTwoPi) * 3.0f;
                int modSamples = static_cast<int> (modMs * 0.001f * srf);
                modSamples = std::max (1, std::min (kChorusBufSize - 1, modSamples));

                int readPos = chorusWritePos - modSamples;
                if (readPos < 0) readPos += kChorusBufSize;

                float chorusL = chorusBuf[0][readPos];
                float chorusR = chorusBuf[1][readPos];

                chorusBuf[0][chorusWritePos] = mixL;
                chorusBuf[1][chorusWritePos] = mixR;
                chorusWritePos = (chorusWritePos + 1) % kChorusBufSize;

                mixL += chorusL * pChorus * 0.5f;
                mixR += chorusR * pChorus * 0.5f;
            }

            // Tape (gentle LP + noise)
            if (pTape > 0.001f)
            {
                tapeState[0] += (mixL - tapeState[0]) * 0.3f;
                tapeState[1] += (mixR - tapeState[1]) * 0.3f;
                tapeState[0] = flushDenormal (tapeState[0]);
                tapeState[1] = flushDenormal (tapeState[1]);

                murmur.rng = murmur.rng * 1664525u + 1013904223u;
                float tapeNoise = static_cast<float> (murmur.rng & 0xFFFF) / 65536.0f - 0.5f;

                mixL = lerp (mixL, tapeState[0] + tapeNoise * 0.003f, pTape);
                mixR = lerp (mixR, tapeState[1] + tapeNoise * 0.003f, pTape);
            }

            // Final soft limiter (prevents clipping on multi-voice sum)
            mixL = fastTanh (mixL);
            mixR = fastTanh (mixR);
            mixL = flushDenormal (mixL);
            mixR = flushDenormal (mixR);

            // Write output
            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample (0, sample, mixL);
                buffer.addSample (1, sample, mixR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample (0, sample, (mixL + mixR) * 0.5f);
            }

            // Coupling cache
            if (sample < static_cast<int> (outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t> (sample)] = mixL;
                outputCacheR[static_cast<size_t> (sample)] = mixR;
            }
        }

        envelopeOutput = peakEnv;

        int count = 0;
        for (const auto& v : voices)
            if (v.active) ++count;
        activeVoices = count;
    }

    //==========================================================================
    // SynthEngine interface — Coupling
    //==========================================================================

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        auto si = static_cast<size_t> (sampleIndex);
        if (channel == 0 && si < outputCacheL.size()) return outputCacheL[si];
        if (channel == 1 && si < outputCacheR.size()) return outputCacheR[si];
        if (channel == 2) return envelopeOutput;
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
            case CouplingType::AudioToWavetable:
                couplingExcitationMod += amount * 0.5f;
                break;
            case CouplingType::AmpToFilter:
                couplingElasticMod += amount * 0.3f;
                break;
            case CouplingType::AudioToFM:
                couplingRoomExcitation += amount * 0.4f;
                break;
            case CouplingType::EnvToMorph:
                couplingShoreDrift += amount * 0.5f;
                break;
            default:
                break;
        }
    }

    //==========================================================================
    // SynthEngine interface — Parameters
    //==========================================================================

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
        // --- Quartet ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_qBassShore", 1 }, "Osteria Bass Shore",
            juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_qHarmShore", 1 }, "Osteria Harmony Shore",
            juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_qMelShore", 1 }, "Osteria Melody Shore",
            juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_qRhythmShore", 1 }, "Osteria Rhythm Shore",
            juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_qElastic", 1 }, "Osteria Elastic",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_qStretch", 1 }, "Osteria Stretch",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_qMemory", 1 }, "Osteria Memory",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_qSympathy", 1 }, "Osteria Sympathy",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

        // --- Voice Balance ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_bassLevel", 1 }, "Osteria Bass Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_harmLevel", 1 }, "Osteria Harmony Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_melLevel", 1 }, "Osteria Melody Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_rhythmLevel", 1 }, "Osteria Rhythm Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_ensWidth", 1 }, "Osteria Ensemble Width",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_blendMode", 1 }, "Osteria Blend Mode",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        // --- Tavern ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_tavernMix", 1 }, "Osteria Tavern Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_tavernShore", 1 }, "Osteria Tavern Shore",
            juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_murmur", 1 }, "Osteria Murmur",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.2f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_warmth", 1 }, "Osteria Warmth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_oceanBleed", 1 }, "Osteria Ocean Bleed",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.1f));

        // --- Character ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_patina", 1 }, "Osteria Patina",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.2f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_porto", 1 }, "Osteria Porto",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_smoke", 1 }, "Osteria Smoke",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.1f));

        // --- Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_attack", 1 }, "Osteria Attack",
            juce::NormalisableRange<float> (0.001f, 4.0f, 0.001f, 0.3f), 0.05f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_decay", 1 }, "Osteria Decay",
            juce::NormalisableRange<float> (0.05f, 4.0f, 0.001f, 0.3f), 0.3f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_sustain", 1 }, "Osteria Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_release", 1 }, "Osteria Release",
            juce::NormalisableRange<float> (0.05f, 8.0f, 0.001f, 0.3f), 1.0f));

        // --- FX ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_sessionDelay", 1 }, "Osteria Session Delay",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.2f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_hall", 1 }, "Osteria Hall",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.2f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_chorus", 1 }, "Osteria Chorus",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.1f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_tape", 1 }, "Osteria Tape",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        // --- Macros ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_macroCharacter", 1 }, "Osteria Macro CHARACTER",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_macroMovement", 1 }, "Osteria Macro MOVEMENT",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_macroCoupling", 1 }, "Osteria Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_macroSpace", 1 }, "Osteria Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramBassShore      = apvts.getRawParameterValue ("osteria_qBassShore");
        paramHarmShore      = apvts.getRawParameterValue ("osteria_qHarmShore");
        paramMelShore       = apvts.getRawParameterValue ("osteria_qMelShore");
        paramRhythmShore    = apvts.getRawParameterValue ("osteria_qRhythmShore");
        paramElastic        = apvts.getRawParameterValue ("osteria_qElastic");
        paramStretch        = apvts.getRawParameterValue ("osteria_qStretch");
        paramMemory         = apvts.getRawParameterValue ("osteria_qMemory");
        paramSympathy       = apvts.getRawParameterValue ("osteria_qSympathy");

        paramBassLevel      = apvts.getRawParameterValue ("osteria_bassLevel");
        paramHarmLevel      = apvts.getRawParameterValue ("osteria_harmLevel");
        paramMelLevel       = apvts.getRawParameterValue ("osteria_melLevel");
        paramRhythmLevel    = apvts.getRawParameterValue ("osteria_rhythmLevel");
        paramEnsWidth       = apvts.getRawParameterValue ("osteria_ensWidth");
        paramBlendMode      = apvts.getRawParameterValue ("osteria_blendMode");

        paramTavernMix      = apvts.getRawParameterValue ("osteria_tavernMix");
        paramTavernShore    = apvts.getRawParameterValue ("osteria_tavernShore");
        paramMurmur         = apvts.getRawParameterValue ("osteria_murmur");
        paramWarmth         = apvts.getRawParameterValue ("osteria_warmth");
        paramOceanBleed     = apvts.getRawParameterValue ("osteria_oceanBleed");

        paramPatina         = apvts.getRawParameterValue ("osteria_patina");
        paramPorto          = apvts.getRawParameterValue ("osteria_porto");
        paramSmoke          = apvts.getRawParameterValue ("osteria_smoke");

        paramAttack         = apvts.getRawParameterValue ("osteria_attack");
        paramDecay          = apvts.getRawParameterValue ("osteria_decay");
        paramSustain        = apvts.getRawParameterValue ("osteria_sustain");
        paramRelease        = apvts.getRawParameterValue ("osteria_release");

        paramSessionDelay   = apvts.getRawParameterValue ("osteria_sessionDelay");
        paramHall           = apvts.getRawParameterValue ("osteria_hall");
        paramChorus         = apvts.getRawParameterValue ("osteria_chorus");
        paramTape           = apvts.getRawParameterValue ("osteria_tape");

        paramMacroCharacter = apvts.getRawParameterValue ("osteria_macroCharacter");
        paramMacroMovement  = apvts.getRawParameterValue ("osteria_macroMovement");
        paramMacroCoupling  = apvts.getRawParameterValue ("osteria_macroCoupling");
        paramMacroSpace     = apvts.getRawParameterValue ("osteria_macroSpace");
    }

    //==========================================================================
    // SynthEngine interface — Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Osteria"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF722F37); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoices; }

private:
    //==========================================================================
    // Helpers
    //==========================================================================

    static float loadParam (std::atomic<float>* p, float fallback) noexcept
    {
        return (p != nullptr) ? p->load() : fallback;
    }

    static float midiToHz (float midiNote) noexcept
    {
        return 440.0f * std::pow (2.0f, (midiNote - 69.0f) / 12.0f);
    }

    //==========================================================================
    // Hall allpass processing
    //==========================================================================

    static constexpr int kHallDelayMax = 4096;
    static constexpr int kHallDelayLengths[4] = { 1051, 1399, 1747, 2083 };

    float processHallAllpass (float input, float feedbackAmount) noexcept
    {
        float sig = input;
        float g = feedbackAmount * 0.5f;
        g = clamp (g, 0.0f, 0.7f);

        for (int i = 0; i < 4; ++i)
        {
            int readPos = hallWritePos[i] - kHallDelayLengths[i];
            if (readPos < 0) readPos += kHallDelayMax;

            float delayed = hallBuf[i][readPos];
            float writeVal = sig + delayed * g;
            hallBuf[i][hallWritePos[i]] = flushDenormal (writeVal);
            sig = delayed - sig * g;

            hallWritePos[i] = (hallWritePos[i] + 1) % kHallDelayMax;
        }

        return sig;
    }

    //==========================================================================
    // MIDI note handling
    //==========================================================================

    void noteOn (int noteNumber, float velocity,
                 const float shoreTargets[4],
                 const float channelLevels[4],
                 const float channelPans[4],
                 float ampA, float ampD, float ampS, float ampR)
    {
        float freq = midiToHz (static_cast<float> (noteNumber));

        int idx = findFreeVoice();
        auto& voice = voices[static_cast<size_t> (idx)];

        if (voice.active)
        {
            voice.fadingOut = true;
            voice.fadeGain = std::min (voice.fadeGain, 0.5f);
        }

        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.startTime = voiceCounter++;
        voice.targetFreq = freq;
        voice.currentTargetFreq = freq;
        voice.glideCoeff = 1.0f;
        voice.fadingOut = false;
        voice.fadeGain = 1.0f;
        voice.controlCounter = 0;
        voice.dcPrevInL = 0.0f; voice.dcPrevOutL = 0.0f;
        voice.dcPrevInR = 0.0f; voice.dcPrevOutR = 0.0f;

        voice.rng = static_cast<uint32_t> (noteNumber * 7919 + voiceCounter * 104729);

        voice.ampEnv.setParams (ampA, ampD, ampS, ampR, srf);
        voice.ampEnv.noteOn();

        // Initialize quartet channels
        for (int c = 0; c < 4; ++c)
        {
            auto& ch = voice.quartet[c];
            ch.role = static_cast<QuartetRole> (c);
            ch.shorePos = shoreTargets[c];
            ch.targetShorePos = shoreTargets[c];
            ch.shoreVelocity = 0.0f;
            ch.level = channelLevels[c];
            ch.pan = channelPans[c];
            ch.oscPhase = voice.nextRandomUni();
            ch.oscPhase2 = voice.nextRandomUni();
            ch.transientEnv = 0.0f;
            ch.transientPhase = voice.nextRandomUni();
            ch.rng = static_cast<uint32_t> (noteNumber * 7919 + c * 31337 + voiceCounter * 54321);
            ch.lastOutputL = 0.0f;
            ch.lastOutputR = 0.0f;

            // Initialize formants from current shore
            ShoreMorphState morph = decomposeShore (ch.shorePos);
            int slot = c;
            if (slot > 2) slot = 0;
            ResonatorProfile prof = morphResonator (morph, slot);
            float noteRatio = freq / 440.0f;

            for (int f = 0; f < 4; ++f)
            {
                ch.formantFreqs[f] = clamp (prof.formantFreqs[f] * noteRatio, 20.0f, 18000.0f);
                ch.formantGains[f] = prof.formantGains[f];
                ch.formantBandwidths[f] = prof.formantBandwidths[f];
                ch.formants[f].reset();
            }
            ch.updateFormants (srf, 0.0f);

            // Initialize memory buffer with current shore
            for (int m = 0; m < kMemoryBufferSize; ++m)
                ch.memoryBuffer[m] = ch.shorePos;
            ch.memoryWritePos = 0;
        }
    }

    void noteOff (int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber && !voice.fadingOut)
                voice.ampEnv.noteOff();
        }
    }

    int findFreeVoice() const
    {
        for (int i = 0; i < kMaxVoices; ++i)
            if (!voices[static_cast<size_t> (i)].active)
                return i;

        int oldest = 0;
        uint64_t oldestTime = UINT64_MAX;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (voices[static_cast<size_t> (i)].startTime < oldestTime)
            {
                oldestTime = voices[static_cast<size_t> (i)].startTime;
                oldest = i;
            }
        }
        return oldest;
    }

    //==========================================================================
    // Member data
    //==========================================================================

    double sr = 44100.0;
    float srf = 44100.0f;
    float crossfadeRate = 0.01f;

    int controlRateDiv = 22;
    float controlDt = 0.0005f;

    std::array<OsteriaVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoices { 0 };

    // Coupling accumulators
    float envelopeOutput = 0.0f;
    float couplingExcitationMod = 0.0f;
    float couplingElasticMod = 0.0f;
    float couplingRoomExcitation = 0.0f;
    float couplingShoreDrift = 0.0f;

    // Output cache
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Tavern + Murmur
    TavernRoom tavernRoom;
    MurmurGenerator murmur;

    // Post filters
    CytomicSVF smokeFilter;
    CytomicSVF warmthFilter;

    // Session delay
    static constexpr int kSessionDelayMax = 22050;
    float sessionDelayBuf[2][kSessionDelayMax] = {};
    int sessionDelayWritePos = 0;

    // Chorus
    static constexpr int kChorusBufSize = 2048;
    float chorusBuf[2][kChorusBufSize] = {};
    int chorusWritePos = 0;
    float chorusPhase = 0.0f;

    // Hall (allpass delays)
    float hallBuf[4][kHallDelayMax] = {};
    int hallWritePos[4] = {};

    // Tape state
    float tapeState[2] = {};

    // Cached APVTS parameter pointers
    std::atomic<float>* paramBassShore = nullptr;
    std::atomic<float>* paramHarmShore = nullptr;
    std::atomic<float>* paramMelShore = nullptr;
    std::atomic<float>* paramRhythmShore = nullptr;
    std::atomic<float>* paramElastic = nullptr;
    std::atomic<float>* paramStretch = nullptr;
    std::atomic<float>* paramMemory = nullptr;
    std::atomic<float>* paramSympathy = nullptr;

    std::atomic<float>* paramBassLevel = nullptr;
    std::atomic<float>* paramHarmLevel = nullptr;
    std::atomic<float>* paramMelLevel = nullptr;
    std::atomic<float>* paramRhythmLevel = nullptr;
    std::atomic<float>* paramEnsWidth = nullptr;
    std::atomic<float>* paramBlendMode = nullptr;

    std::atomic<float>* paramTavernMix = nullptr;
    std::atomic<float>* paramTavernShore = nullptr;
    std::atomic<float>* paramMurmur = nullptr;
    std::atomic<float>* paramWarmth = nullptr;
    std::atomic<float>* paramOceanBleed = nullptr;

    std::atomic<float>* paramPatina = nullptr;
    std::atomic<float>* paramPorto = nullptr;
    std::atomic<float>* paramSmoke = nullptr;

    std::atomic<float>* paramAttack = nullptr;
    std::atomic<float>* paramDecay = nullptr;
    std::atomic<float>* paramSustain = nullptr;
    std::atomic<float>* paramRelease = nullptr;

    std::atomic<float>* paramSessionDelay = nullptr;
    std::atomic<float>* paramHall = nullptr;
    std::atomic<float>* paramChorus = nullptr;
    std::atomic<float>* paramTape = nullptr;

    std::atomic<float>* paramMacroCharacter = nullptr;
    std::atomic<float>* paramMacroMovement = nullptr;
    std::atomic<float>* paramMacroCoupling = nullptr;
    std::atomic<float>* paramMacroSpace = nullptr;
};

} // namespace xomnibus
