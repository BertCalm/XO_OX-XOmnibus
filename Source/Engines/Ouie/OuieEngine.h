#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace xomnibus {

//==============================================================================
// OuieEngine — Duophonic Hammerhead Shark Synthesis.
//
// Maps the hammerhead shark's unique cephalofoil anatomy into a duophonic
// synthesizer architecture. Two synthesis voices (A and B) represent the
// shark's lateralized sensory organs — independently capable, but most
// powerful when operating in concert.
//
// ARCHITECTURE:
//
//   Voice A + Voice B
//     Each voice selects one of 4 algorithms:
//       0 = VA      — anti-aliased sawtooth (PolyBLEP-style)
//       1 = Wavetable — 4-frame morphing: sine → tri → saw → square
//       2 = FM      — 2-operator FM (carrier + modulator)
//       3 = KS      — Karplus-Strong pluck (delay-line synthesis)
//     Per-voice: pitch offset (±12 st), dedicated ADSR, dedicated SVF filter
//
//   HAMMER Interaction Stage — the cephalofoil:
//     STRIFE (-1.0): Voice A cross-FM-modulates Voice B + ring mod blend
//     Independent (0.0): voices run in parallel, summed at output
//     LOVE (+1.0): spectral crossfade + harmonic lock (Voice B pitch snaps
//                  to nearest harmonic of Voice A fundamental)
//
//   Voice Modes:
//     Duo   (0) — both respond to MIDI; A takes lower notes, B takes higher
//     Layer (1) — both play the same note simultaneously
//     Split (2) — A below splitPoint, B above
//
//   Shared master amp envelope scales both voices after HAMMER stage.
//   2 LFOs: LFO1 = pitch vibrato, LFO2 = filter wobble.
//   Mod wheel (CC#1) controls HAMMER position (D006).
//   Aftertouch controls portamento glide depth (D006).
//
// Accent Color: Hammerhead Steel #708090
// Gallery code: OUIE | Prefix: ouie_ | Engine ID: "Ouie"
//
//==============================================================================

//==============================================================================
// ADSR envelope.
//==============================================================================
struct OuieADSR
{
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    Stage stage   = Stage::Idle;
    float level   = 0.0f;
    float attackRate  = 0.0f;
    float decayRate   = 0.0f;
    float sustainLevel = 1.0f;
    float releaseRate = 0.0f;

    void setParams (float attackSec, float decaySec, float sustain,
                    float releaseSec, float sampleRate) noexcept
    {
        float sr = std::max (1.0f, sampleRate);
        attackRate  = (attackSec  > 0.001f) ? (1.0f / (attackSec  * sr)) : 1.0f;
        decayRate   = (decaySec   > 0.001f) ? (1.0f / (decaySec   * sr)) : 1.0f;
        sustainLevel = clamp (sustain, 0.0f, 1.0f);
        releaseRate = (releaseSec > 0.001f) ? (1.0f / (releaseSec * sr)) : 1.0f;
    }

    void noteOn()  noexcept { stage = Stage::Attack; }

    void noteOff() noexcept
    {
        if (stage != Stage::Idle)
            stage = Stage::Release;
    }

    float process() noexcept
    {
        switch (stage)
        {
            case Stage::Idle:
                return 0.0f;
            case Stage::Attack:
                level += attackRate;
                if (level >= 1.0f) { level = 1.0f; stage = Stage::Decay; }
                return level;
            case Stage::Decay:
                level -= decayRate * (level - sustainLevel + 0.0001f);
                if (level <= sustainLevel + 0.0001f) { level = sustainLevel; stage = Stage::Sustain; }
                return level;
            case Stage::Sustain:
                return level;
            case Stage::Release:
                level -= releaseRate * (level + 0.0001f);
                if (level <= 0.0001f) { level = 0.0f; stage = Stage::Idle; }
                return level;
        }
        return 0.0f;
    }

    bool isActive() const noexcept { return stage != Stage::Idle; }
    void reset()    noexcept { stage = Stage::Idle; level = 0.0f; }
};

//==============================================================================
// Simple sine LFO.
//==============================================================================
struct OuieLFO
{
    float phase    = 0.0f;
    float phaseInc = 0.0f;

    void setRate (float hz, float sampleRate) noexcept
    {
        phaseInc = hz / std::max (1.0f, sampleRate);
    }

    float process() noexcept
    {
        float out = fastSin (phase * 6.28318530718f);
        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;
        return out;
    }

    void reset() noexcept { phase = 0.0f; }
};

//==============================================================================
// Karplus-Strong delay line for KS algorithm.
//==============================================================================
struct OuieKSDelay
{
    static constexpr int kMaxDelay = 4096;
    float buffer[kMaxDelay] {};
    int writePos = 0;
    float delaySamples = 100.0f;
    float dampingCoeff = 0.5f;
    float prevSample   = 0.0f;

    void setFrequency (float freq, float sampleRate) noexcept
    {
        delaySamples = clamp (sampleRate / std::max (1.0f, freq), 2.0f,
                              static_cast<float> (kMaxDelay - 1));
    }

    void excite (float noiseLevel, uint32_t& rng) noexcept
    {
        // Fill delay line with band-limited noise
        for (int i = 0; i < kMaxDelay; ++i)
        {
            rng = rng * 1664525u + 1013904223u;
            buffer[i] = (static_cast<float> (rng & 0xFFFF) / 32768.0f - 1.0f) * noiseLevel;
        }
        prevSample = 0.0f;
        writePos   = 0;
    }

    float process() noexcept
    {
        // Read with linear interpolation
        float readPos = static_cast<float> (writePos) - delaySamples;
        if (readPos < 0.0f) readPos += static_cast<float> (kMaxDelay);

        int idx0 = static_cast<int> (readPos) % kMaxDelay;
        int idx1 = (idx0 + 1) % kMaxDelay;
        float frac = readPos - static_cast<float> (static_cast<int> (readPos));
        if (frac < 0.0f) frac += 1.0f;

        float delayed = buffer[idx0] + frac * (buffer[idx1] - buffer[idx0]);

        // Averaging low-pass filter in feedback path (Karplus-Strong damping)
        float filtered = 0.5f * (delayed + prevSample);
        filtered       = flushDenormal (filtered * dampingCoeff);
        prevSample     = filtered;

        buffer[writePos] = filtered;
        writePos = (writePos + 1) % kMaxDelay;

        return delayed;
    }

    void reset() noexcept
    {
        std::memset (buffer, 0, sizeof (buffer));
        writePos   = 0;
        prevSample = 0.0f;
    }
};

//==============================================================================
// Per-voice DSP state for one synthesis voice (A or B).
//==============================================================================
struct OuieVoice
{
    // Voice identity
    bool  active     = false;
    int   noteNumber = -1;
    float velocity   = 0.0f;

    // Portamento
    float currentFreq = 440.0f;
    float targetFreq  = 440.0f;
    float portaCoeff  = 1.0f;   // 1.0 = instant

    // VA oscillator state
    float vaPhase     = 0.0f;
    float vaPhaseInc  = 0.0f;

    // Wavetable oscillator state (4-wave morphing)
    float wtPhase    = 0.0f;
    float wtPhaseInc = 0.0f;
    float wtMorph    = 0.0f;   // 0=sine, 1=tri, 2=saw, 3=square (continuous 0–3)

    // FM oscillator state (2-op: carrier + modulator)
    float fmCarrierPhase = 0.0f;
    float fmModPhase     = 0.0f;
    float fmCarrierInc   = 0.0f;
    float fmModInc       = 0.0f;
    float fmRatio        = 2.0f;   // modulator:carrier ratio
    float fmDepth        = 1.0f;   // FM index

    // KS state
    OuieKSDelay ksDelay;
    uint32_t    ksRng     = 12345u;
    bool        ksActive  = false;

    // Voice envelope (per-voice A or B ADSR)
    OuieADSR voiceEnv;

    // LFOs (per-voice, so they aren't locked in phase across notes)
    OuieLFO lfo1;  // pitch vibrato
    OuieLFO lfo2;  // filter wobble

    // Per-voice SVF filter
    CytomicSVF filter;

    void reset() noexcept
    {
        active     = false;
        noteNumber = -1;
        velocity   = 0.0f;
        currentFreq = 440.0f;
        targetFreq  = 440.0f;
        portaCoeff  = 1.0f;
        vaPhase     = 0.0f;
        vaPhaseInc  = 0.0f;
        wtPhase     = 0.0f;
        wtPhaseInc  = 0.0f;
        wtMorph     = 0.0f;
        fmCarrierPhase = 0.0f;
        fmModPhase     = 0.0f;
        fmCarrierInc   = 0.0f;
        fmModInc       = 0.0f;
        ksActive = false;
        voiceEnv.reset();
        lfo1.reset();
        lfo2.reset();
        ksDelay.reset();
        filter.reset();
    }
};

//==============================================================================
// Allpass-based Schroeder reverb tail for ouie_reverbMix.
//==============================================================================
struct OuieSimpleReverb
{
    // 4 allpass stages — small, allocation-free
    static constexpr int kAP0 = 347;
    static constexpr int kAP1 = 523;
    static constexpr int kAP2 = 889;
    static constexpr int kAP3 = 1367;

    float ap0L[kAP0]{}, ap1L[kAP1]{}, ap2L[kAP2]{}, ap3L[kAP3]{};
    float ap0R[kAP0]{}, ap1R[kAP1]{}, ap2R[kAP2]{}, ap3R[kAP3]{};
    int   p0 = 0, p1 = 0, p2 = 0, p3 = 0;

    void processStereo (float inL, float inR, float& outL, float& outR) noexcept
    {
        constexpr float g = 0.5f;

        auto allpass = [](float* buf, int& pos, int len, float x, float gain) -> float {
            float delayed = buf[pos];
            float w = x + gain * delayed;
            w = flushDenormal (w);
            buf[pos] = w;
            pos = (pos + 1 >= len) ? 0 : pos + 1;
            return delayed - gain * w;
        };

        outL = allpass (ap0L, p0, kAP0, inL, g);
        outL = allpass (ap1L, p1, kAP1, outL, g);
        outL = allpass (ap2L, p2, kAP2, outL, g);
        outL = allpass (ap3L, p3, kAP3, outL, g);

        outR = allpass (ap0R, p0, kAP0, inR, g);
        outR = allpass (ap1R, p1, kAP1, outR, g);
        outR = allpass (ap2R, p2, kAP2, outR, g);
        outR = allpass (ap3R, p3, kAP3, outR, g);
    }

    void reset() noexcept
    {
        std::memset (ap0L, 0, sizeof(ap0L)); std::memset (ap0R, 0, sizeof(ap0R));
        std::memset (ap1L, 0, sizeof(ap1L)); std::memset (ap1R, 0, sizeof(ap1R));
        std::memset (ap2L, 0, sizeof(ap2L)); std::memset (ap2R, 0, sizeof(ap2R));
        std::memset (ap3L, 0, sizeof(ap3L)); std::memset (ap3R, 0, sizeof(ap3R));
        p0 = p1 = p2 = p3 = 0;
    }
};

//==============================================================================
// OuieEngine — the main engine class.
//==============================================================================
class OuieEngine : public SynthEngine
{
public:
    static constexpr float kTwoPi = 6.28318530717958647692f;

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr  = sampleRate;
        srf = static_cast<float> (sr);

        silenceGate.prepare (sampleRate, maxBlockSize);
        silenceGate.setHoldTime (200.0f);  // standard hold

        smoothCoeff = 1.0f - fastExp (-kTwoPi * 200.0f / srf);  // ~5ms smooth

        outputCacheL.assign (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.assign (static_cast<size_t> (maxBlockSize), 0.0f);

        for (int v = 0; v < 2; ++v)
        {
            voices[v].reset();
            voices[v].filter.reset();
            voices[v].filter.setMode (CytomicSVF::Mode::LowPass);
        }

        masterEnv.reset();
        reverb.reset();
    }

    void releaseResources() override {}

    void reset() override
    {
        for (int v = 0; v < 2; ++v)
            voices[v].reset();

        masterEnv.reset();
        reverb.reset();

        couplingHammerMod  = 0.0f;
        couplingFilterMod  = 0.0f;
        envelopeOutput     = 0.0f;
        modWheelAmount     = 0.0f;
        aftertouchAmount   = 0.0f;

        smoothedHammerPos  = 0.0f;

        outputCacheL.assign (outputCacheL.size(), 0.0f);
        outputCacheR.assign (outputCacheR.size(), 0.0f);
    }

    //==========================================================================
    // Audio
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        //----------------------------------------------------------------------
        // ParamSnapshot — read all parameters once per block
        //----------------------------------------------------------------------
        const int   pVoiceAAlgo   = static_cast<int> (loadParam (paramVoiceAAlgo,   0.0f));
        const int   pVoiceBAlgo   = static_cast<int> (loadParam (paramVoiceBAlgo,   1.0f));
        const float pVoiceAPitch  = loadParam (paramVoiceAPitch,  0.0f);
        const float pVoiceBPitch  = loadParam (paramVoiceBPitch,  7.0f);   // 5th default
        const float pVoiceALevel  = loadParam (paramVoiceALevel,  0.8f);
        const float pVoiceBLevel  = loadParam (paramVoiceBLevel,  0.8f);

        const float pFilterACutoff = loadParam (paramFilterACutoff, 4000.0f);
        const float pFilterARes    = loadParam (paramFilterARes,    0.2f);
        const float pFilterBCutoff = loadParam (paramFilterBCutoff, 4000.0f);
        const float pFilterBRes    = loadParam (paramFilterBRes,    0.2f);

        const float pAmpAtk  = loadParam (paramAmpAtk,  0.01f);
        const float pAmpDec  = loadParam (paramAmpDec,  0.1f);
        const float pAmpSus  = loadParam (paramAmpSus,  0.8f);
        const float pAmpRel  = loadParam (paramAmpRel,  0.3f);

        const float pHammerPos   = loadParam (paramHammerPos,   0.0f);
        const float pHammerDepth = loadParam (paramHammerDepth, 0.5f);

        const int   pVoiceMode   = static_cast<int> (loadParam (paramVoiceMode,   1.0f));
        const int   pSplitPoint  = static_cast<int> (loadParam (paramSplitPoint,  60.0f));

        const float pVelCutoffAmt = loadParam (paramVelCutoffAmt, 0.5f);
        const float pPortaTime    = loadParam (paramPortaTime,    0.0f);
        const float pPortaAuto    = loadParam (paramPortaAuto,    0.0f);

        const float pLfo1Rate   = loadParam (paramLfo1Rate,   1.0f);
        const float pLfo1Depth  = loadParam (paramLfo1Depth,  0.3f);
        const int   pLfo1Target = static_cast<int> (loadParam (paramLfo1Target, 0.0f));
        const float pLfo2Rate   = loadParam (paramLfo2Rate,   3.0f);
        const float pLfo2Depth  = loadParam (paramLfo2Depth,  0.0f);

        const float pReverbMix  = loadParam (paramReverbMix, 0.2f);

        // Macros
        const float pMacroHammer    = loadParam (paramMacroHammer,    0.0f);
        const float pMacroAmpullae  = loadParam (paramMacroAmpullae,  0.0f);
        const float pMacroCartilage = loadParam (paramMacroCartilage, 0.0f);
        const float pMacroCurrent   = loadParam (paramMacroCurrent,   0.0f);

        //----------------------------------------------------------------------
        // Apply macro modulation
        //   macroHammer   — bipolar: negative → STRIFE, positive → LOVE
        //   macroAmpullae — sensitivity: velocity response + filter resonance
        //   macroCartilage— flexibility: portamento + pitch mod depth
        //   macroCurrent  — environment: reverb + filter cutoff sweep
        //----------------------------------------------------------------------
        // Effective HAMMER position — mod wheel (D006), macro, param, coupling
        float effectiveHammerPos = clamp (
            pHammerPos
            + pMacroHammer
            + modWheelAmount * 2.0f - 1.0f   // mod wheel: 0→1 maps to -1→+1
            + couplingHammerMod,
            -1.0f, 1.0f);

        float effectiveReverbMix = clamp (pReverbMix + pMacroCurrent * 0.4f, 0.0f, 1.0f);

        // Per-voice filter cutoff boosted by macroCurrent
        float effectiveCutoffA = clamp (pFilterACutoff + pMacroCurrent * 4000.0f + couplingFilterMod * 3000.0f,
                                        200.0f, 20000.0f);
        float effectiveCutoffB = clamp (pFilterBCutoff + pMacroCurrent * 4000.0f + couplingFilterMod * 3000.0f,
                                        200.0f, 20000.0f);
        float effectiveResA    = clamp (pFilterARes + pMacroAmpullae * 0.4f, 0.0f, 0.95f);
        float effectiveResB    = clamp (pFilterBRes + pMacroAmpullae * 0.4f, 0.0f, 0.95f);

        float effectiveLfo1Depth = clamp (pLfo1Depth + pMacroCartilage * 0.4f, 0.0f, 1.0f);

        // Portamento coefficient — aftertouch increases glide (D006)
        float portaTimeFinal = pPortaTime + aftertouchAmount * pMacroCartilage * 2.0f;
        float portaCoeff = 1.0f;
        if (portaTimeFinal > 0.001f)
            portaCoeff = 1.0f - fastExp (-1.0f / (portaTimeFinal * srf));

        // Reset coupling accumulators each block
        couplingHammerMod  = 0.0f;
        couplingFilterMod  = 0.0f;

        //----------------------------------------------------------------------
        // Process MIDI — MUST happen before SilenceGate check
        //----------------------------------------------------------------------
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                masterEnv.noteOn();
                handleNoteOn (msg.getNoteNumber(), msg.getFloatVelocity(),
                              pVoiceMode, pSplitPoint, portaCoeff,
                              pPortaAuto > 0.5f,
                              pAmpAtk, pAmpDec, pAmpSus, pAmpRel,
                              pLfo1Rate, pLfo2Rate,
                              pVoiceAAlgo, pVoiceBAlgo,
                              pVoiceAPitch, pVoiceBPitch);
            }
            else if (msg.isNoteOff())
            {
                handleNoteOff (msg.getNoteNumber(), pVoiceMode, pSplitPoint);
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                reset();
            }
            else if (msg.isController())
            {
                if (msg.getControllerNumber() == 1)
                    modWheelAmount = msg.getControllerValue() / 127.0f;
            }
            else if (msg.isAftertouch() || msg.isChannelPressure())
            {
                aftertouchAmount = msg.isAftertouch()
                    ? msg.getAfterTouchValue() / 127.0f
                    : msg.getChannelPressureValue() / 127.0f;
            }
        }

        //----------------------------------------------------------------------
        // SilenceGate bypass
        //----------------------------------------------------------------------
        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        //----------------------------------------------------------------------
        // Update voice portamento coefficients
        //----------------------------------------------------------------------
        for (int v = 0; v < 2; ++v)
            voices[v].portaCoeff = portaCoeff;

        //----------------------------------------------------------------------
        // Update filter coefficients once per block (per D001)
        //----------------------------------------------------------------------
        for (int v = 0; v < 2; ++v)
        {
            if (!voices[v].active) continue;
            float velMod = pVelCutoffAmt * voices[v].velocity * 4000.0f;
            float cutoff = (v == 0) ? clamp (effectiveCutoffA + velMod, 200.0f, 20000.0f)
                                    : clamp (effectiveCutoffB + velMod, 200.0f, 20000.0f);
            float res    = (v == 0) ? effectiveResA : effectiveResB;
            voices[v].filter.setCoefficients (cutoff, res, srf);
        }

        //----------------------------------------------------------------------
        // Update master envelope timing
        //----------------------------------------------------------------------
        masterEnv.setParams (pAmpAtk, pAmpDec, pAmpSus, pAmpRel, srf);

        //----------------------------------------------------------------------
        // Determine Voice A fundamental for harmonic lock (LOVE mode)
        //----------------------------------------------------------------------
        float voiceAFreq = voices[0].active ? voices[0].currentFreq : 0.0f;

        //----------------------------------------------------------------------
        // Render sample loop
        //----------------------------------------------------------------------
        float* bufL = buffer.getWritePointer (0);
        float* bufR = (buffer.getNumChannels() > 1) ? buffer.getWritePointer (1) : nullptr;

        float peakEnv = 0.0f;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Smooth HAMMER position
            smoothedHammerPos += (effectiveHammerPos - smoothedHammerPos) * smoothCoeff;
            float hammerPos = smoothedHammerPos;

            // Process master amp envelope
            float masterGain = masterEnv.process();
            if (!masterEnv.isActive() && masterGain <= 0.0001f)
            {
                // If master envelope is done and no voice is active, let gate sleep
                masterGain = 0.0f;
            }
            peakEnv = std::max (peakEnv, masterGain);

            // --- Render Voice A ---
            float sigA = 0.0f;
            if (voices[0].active)
            {
                // Portamento
                voices[0].currentFreq += (voices[0].targetFreq - voices[0].currentFreq)
                                          * voices[0].portaCoeff;
                voices[0].currentFreq = flushDenormal (voices[0].currentFreq);
                voiceAFreq = voices[0].currentFreq;

                // LFOs
                float lfo1ValA = voices[0].lfo1.process() * effectiveLfo1Depth;
                float lfo2ValA = voices[0].lfo2.process() * pLfo2Depth;

                // LFO1 target routing
                float lfo1PitchA   = (pLfo1Target == 0) ? lfo1ValA * 0.02f : 0.0f;
                float lfo1FilterA  = (pLfo1Target == 1) ? lfo1ValA * 2000.0f : 0.0f;
                float lfo1HammerA  = (pLfo1Target == 2) ? lfo1ValA * 0.3f : 0.0f;

                float freqA = voices[0].currentFreq
                              * fastPow2 ((pVoiceAPitch + lfo1PitchA) / 12.0f);
                freqA = std::max (20.0f, freqA);

                // Per-voice ADSR
                float envA = voices[0].voiceEnv.process();
                if (!voices[0].voiceEnv.isActive())
                    voices[0].active = false;

                // Oscillator
                float rawA = renderOscillator (voices[0], 0, freqA, pVoiceAAlgo);

                // Filter (LFO2 modulates filter cutoff)
                {
                    float cutoffWithLFO = clamp (effectiveCutoffA + lfo2ValA * 3000.0f + lfo1FilterA,
                                                 20.0f, 20000.0f);
                    float resA = clamp (effectiveResA + pMacroAmpullae * 0.2f, 0.0f, 0.95f);
                    voices[0].filter.setCoefficients (cutoffWithLFO, resA, srf);
                }
                rawA = voices[0].filter.processSample (rawA);
                rawA = flushDenormal (rawA);

                sigA = rawA * envA * pVoiceALevel * voices[0].velocity;
                (void) lfo1HammerA;  // consumed by HAMMER stage below
            }

            // --- Render Voice B ---
            float sigB = 0.0f;
            if (voices[1].active)
            {
                // Portamento
                voices[1].currentFreq += (voices[1].targetFreq - voices[1].currentFreq)
                                          * voices[1].portaCoeff;
                voices[1].currentFreq = flushDenormal (voices[1].currentFreq);

                // Harmonic lock in LOVE territory: quantize Voice B pitch to nearest
                // harmonic of Voice A fundamental
                float targetBFreq = voices[1].targetFreq;
                if (hammerPos > 0.0f && voiceAFreq > 20.0f)
                {
                    // Find nearest harmonic ratio
                    float ratio = targetBFreq / voiceAFreq;
                    float nearestHarmonic = std::round (ratio);
                    nearestHarmonic = std::max (1.0f, nearestHarmonic);
                    float harmonicFreq = voiceAFreq * nearestHarmonic;

                    // Blend toward harmonic with LOVE amount
                    float loveAmt = clamp (hammerPos, 0.0f, 1.0f) * pHammerDepth;
                    targetBFreq = lerp (targetBFreq, harmonicFreq, loveAmt);
                }
                voices[1].targetFreq = targetBFreq;

                // LFOs
                float lfo1ValB = voices[1].lfo1.process() * effectiveLfo1Depth;
                float lfo2ValB = voices[1].lfo2.process() * pLfo2Depth;

                float lfo1PitchB  = (pLfo1Target == 0) ? lfo1ValB * 0.02f : 0.0f;
                float lfo1FilterB = (pLfo1Target == 1) ? lfo1ValB * 2000.0f : 0.0f;

                float freqB = voices[1].currentFreq
                              * fastPow2 ((pVoiceBPitch + lfo1PitchB) / 12.0f);
                freqB = std::max (20.0f, freqB);

                // Per-voice ADSR
                float envB = voices[1].voiceEnv.process();
                if (!voices[1].voiceEnv.isActive())
                    voices[1].active = false;

                // If in STRIFE territory: Voice A FM-modulates Voice B
                float fmModInput = 0.0f;
                if (hammerPos < 0.0f && voices[0].active)
                {
                    float strifeAmt = clamp (-hammerPos, 0.0f, 1.0f) * pHammerDepth;
                    // Use sigA as FM modulator (scaled)
                    fmModInput = sigA * strifeAmt * 4.0f;
                }

                // Oscillator (with STRIFE FM injection into carrier phase)
                float rawB = renderOscillatorFMInput (voices[1], 1, freqB, pVoiceBAlgo, fmModInput);

                // Filter
                {
                    float cutoffWithLFO = clamp (effectiveCutoffB + lfo2ValB * 3000.0f + lfo1FilterB,
                                                 20.0f, 20000.0f);
                    float resB = clamp (effectiveResB + pMacroAmpullae * 0.2f, 0.0f, 0.95f);
                    voices[1].filter.setCoefficients (cutoffWithLFO, resB, srf);
                }
                rawB = voices[1].filter.processSample (rawB);
                rawB = flushDenormal (rawB);

                sigB = rawB * envB * pVoiceBLevel * voices[1].velocity;
            }

            //------------------------------------------------------------------
            // HAMMER interaction stage
            //------------------------------------------------------------------
            float outL = 0.0f, outR = 0.0f;

            if (hammerPos <= 0.0f)
            {
                // STRIFE territory: ring mod blend between A and B
                float strifeAmt = clamp (-hammerPos, 0.0f, 1.0f) * pHammerDepth;
                float ringMod   = sigA * sigB;  // ring modulation product
                float mixAB     = (sigA + sigB) * 0.5f;
                float strife    = lerp (mixAB, ringMod, strifeAmt);

                outL = strife;
                outR = strife;
            }
            else
            {
                // LOVE territory: spectral crossfade (simple stereo spread)
                float loveAmt = clamp (hammerPos, 0.0f, 1.0f) * pHammerDepth;
                // Voice A pans left, Voice B pans right as LOVE increases
                float panL = 1.0f - loveAmt * 0.5f;
                float panR = 1.0f - loveAmt * 0.5f;

                outL = sigA * panL + sigB * (1.0f - panL * 0.5f);
                outR = sigB * panR + sigA * (1.0f - panR * 0.5f);
            }

            // Apply master envelope and master gain
            outL *= masterGain;
            outR *= masterGain;

            // Reverb
            if (effectiveReverbMix > 0.001f)
            {
                float rvL = 0.0f, rvR = 0.0f;
                reverb.processStereo (outL * 0.5f, outR * 0.5f, rvL, rvR);
                outL = outL * (1.0f - effectiveReverbMix) + rvL * effectiveReverbMix;
                outR = outR * (1.0f - effectiveReverbMix) + rvR * effectiveReverbMix;
            }

            outL = flushDenormal (outL);
            outR = flushDenormal (outR);

            // Write to buffer
            if (buffer.getNumChannels() >= 2)
            {
                bufL[sample] += outL;
                bufR[sample] += outR;
            }
            else if (buffer.getNumChannels() == 1)
            {
                bufL[sample] += (outL + outR) * 0.5f;
            }

            // Cache for coupling
            if (sample < static_cast<int> (outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t> (sample)] = outL;
                outputCacheR[static_cast<size_t> (sample)] = outR;
            }
        }

        envelopeOutput = peakEnv;

        // Update active voice count
        int count = 0;
        for (int v = 0; v < 2; ++v)
            if (voices[v].active) ++count;
        activeVoiceCount_ = count;

        // SilenceGate analysis
        silenceGate.analyzeBlock (buffer.getReadPointer (0),
                                  buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : nullptr,
                                  numSamples);
    }

    //==========================================================================
    // Coupling
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
            case CouplingType::AmpToFilter:
                couplingFilterMod += amount;
                break;
            case CouplingType::AudioToFM:
                // Modulate HAMMER position via cross-FM amount
                couplingHammerMod += amount * 0.5f;
                break;
            case CouplingType::LFOToPitch:
                // External LFO shifts HAMMER toward STRIFE
                couplingHammerMod += amount * -0.3f;
                break;
            case CouplingType::EnvToMorph:
                // Envelope from another engine morphs wavetable position
                for (int v = 0; v < 2; ++v)
                    voices[v].wtMorph = clamp (voices[v].wtMorph + amount * 0.5f, 0.0f, 3.0f);
                break;
            case CouplingType::AudioToRing:
                // Ring mod source from external engine — boost STRIFE
                couplingHammerMod += amount * -0.2f;
                break;
            default:
                break;
        }
    }

    //==========================================================================
    // Parameters
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
        // --- Voice A ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "ouie_voiceAAlgo", 1 }, "Ouie Voice A Algorithm",
            juce::StringArray { "VA Saw", "Wavetable", "FM", "KS Pluck" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_voiceAPitch", 1 }, "Ouie Voice A Pitch (st)",
            juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_voiceALevel", 1 }, "Ouie Voice A Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        // --- Voice B ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "ouie_voiceBAlgo", 1 }, "Ouie Voice B Algorithm",
            juce::StringArray { "VA Saw", "Wavetable", "FM", "KS Pluck" }, 1));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_voiceBPitch", 1 }, "Ouie Voice B Pitch (st)",
            juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 7.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_voiceBLevel", 1 }, "Ouie Voice B Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        // --- Voice A Filter ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_filterACutoff", 1 }, "Ouie Filter A Cutoff",
            juce::NormalisableRange<float> (200.0f, 12000.0f, 1.0f, 0.3f), 4000.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_filterARes", 1 }, "Ouie Filter A Resonance",
            juce::NormalisableRange<float> (0.0f, 0.95f, 0.001f), 0.2f));

        // --- Voice B Filter ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_filterBCutoff", 1 }, "Ouie Filter B Cutoff",
            juce::NormalisableRange<float> (200.0f, 12000.0f, 1.0f, 0.3f), 4000.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_filterBRes", 1 }, "Ouie Filter B Resonance",
            juce::NormalisableRange<float> (0.0f, 0.95f, 0.001f), 0.2f));

        // --- Master Amp Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_ampAtk", 1 }, "Ouie Amp Attack",
            juce::NormalisableRange<float> (0.001f, 2.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_ampDec", 1 }, "Ouie Amp Decay",
            juce::NormalisableRange<float> (0.01f, 3.0f, 0.001f, 0.3f), 0.2f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_ampSus", 1 }, "Ouie Amp Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_ampRel", 1 }, "Ouie Amp Release",
            juce::NormalisableRange<float> (0.05f, 5.0f, 0.001f, 0.3f), 0.4f));

        // --- HAMMER ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_hammerPos", 1 }, "Ouie Hammer Position",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_hammerDepth", 1 }, "Ouie Hammer Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        // --- Voice Mode ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "ouie_voiceMode", 1 }, "Ouie Voice Mode",
            juce::StringArray { "Duo", "Layer", "Split" }, 1));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_splitPoint", 1 }, "Ouie Split Point",
            juce::NormalisableRange<float> (0.0f, 127.0f, 1.0f), 60.0f));

        // --- Velocity / Expression (D001/D006) ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_velCutoffAmt", 1 }, "Ouie Velocity \xe2\x86\x92 Brightness",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        // --- Portamento ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_portaTime", 1 }, "Ouie Portamento Time",
            juce::NormalisableRange<float> (0.0f, 5.0f, 0.001f, 0.3f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { "ouie_portaAuto", 1 }, "Ouie Portamento Auto", false));

        // --- LFO 1 (pitch vibrato) ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_lfo1Rate", 1 }, "Ouie LFO1 Rate",
            juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.3f), 5.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_lfo1Depth", 1 }, "Ouie LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "ouie_lfo1Target", 1 }, "Ouie LFO1 Target",
            juce::StringArray { "Pitch", "Filter", "Hammer" }, 0));

        // --- LFO 2 (filter wobble) ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_lfo2Rate", 1 }, "Ouie LFO2 Rate",
            juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.3f), 3.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_lfo2Depth", 1 }, "Ouie LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // --- Reverb ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_reverbMix", 1 }, "Ouie Reverb Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.2f));

        // --- Macros ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_macroHammer", 1 }, "Ouie Macro HAMMER",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_macroAmpullae", 1 }, "Ouie Macro AMPULLAE",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_macroCartilage", 1 }, "Ouie Macro CARTILAGE",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ouie_macroCurrent", 1 }, "Ouie Macro CURRENT",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramVoiceAAlgo    = apvts.getRawParameterValue ("ouie_voiceAAlgo");
        paramVoiceBAlgo    = apvts.getRawParameterValue ("ouie_voiceBAlgo");
        paramVoiceAPitch   = apvts.getRawParameterValue ("ouie_voiceAPitch");
        paramVoiceBPitch   = apvts.getRawParameterValue ("ouie_voiceBPitch");
        paramVoiceALevel   = apvts.getRawParameterValue ("ouie_voiceALevel");
        paramVoiceBLevel   = apvts.getRawParameterValue ("ouie_voiceBLevel");

        paramFilterACutoff = apvts.getRawParameterValue ("ouie_filterACutoff");
        paramFilterARes    = apvts.getRawParameterValue ("ouie_filterARes");
        paramFilterBCutoff = apvts.getRawParameterValue ("ouie_filterBCutoff");
        paramFilterBRes    = apvts.getRawParameterValue ("ouie_filterBRes");

        paramAmpAtk        = apvts.getRawParameterValue ("ouie_ampAtk");
        paramAmpDec        = apvts.getRawParameterValue ("ouie_ampDec");
        paramAmpSus        = apvts.getRawParameterValue ("ouie_ampSus");
        paramAmpRel        = apvts.getRawParameterValue ("ouie_ampRel");

        paramHammerPos     = apvts.getRawParameterValue ("ouie_hammerPos");
        paramHammerDepth   = apvts.getRawParameterValue ("ouie_hammerDepth");

        paramVoiceMode     = apvts.getRawParameterValue ("ouie_voiceMode");
        paramSplitPoint    = apvts.getRawParameterValue ("ouie_splitPoint");

        paramVelCutoffAmt  = apvts.getRawParameterValue ("ouie_velCutoffAmt");
        paramPortaTime     = apvts.getRawParameterValue ("ouie_portaTime");
        paramPortaAuto     = apvts.getRawParameterValue ("ouie_portaAuto");

        paramLfo1Rate      = apvts.getRawParameterValue ("ouie_lfo1Rate");
        paramLfo1Depth     = apvts.getRawParameterValue ("ouie_lfo1Depth");
        paramLfo1Target    = apvts.getRawParameterValue ("ouie_lfo1Target");
        paramLfo2Rate      = apvts.getRawParameterValue ("ouie_lfo2Rate");
        paramLfo2Depth     = apvts.getRawParameterValue ("ouie_lfo2Depth");

        paramReverbMix     = apvts.getRawParameterValue ("ouie_reverbMix");

        paramMacroHammer    = apvts.getRawParameterValue ("ouie_macroHammer");
        paramMacroAmpullae  = apvts.getRawParameterValue ("ouie_macroAmpullae");
        paramMacroCartilage = apvts.getRawParameterValue ("ouie_macroCartilage");
        paramMacroCurrent   = apvts.getRawParameterValue ("ouie_macroCurrent");
    }

    //==========================================================================
    // Identity
    //==========================================================================

    juce::String  getEngineId()      const override { return "Ouie"; }
    juce::Colour  getAccentColour()  const override { return juce::Colour (0xFF708090); }
    int           getMaxVoices()     const override { return 2; }
    int           getActiveVoiceCount() const override { return activeVoiceCount_; }

private:
    //==========================================================================
    // Safe parameter load
    //==========================================================================
    static float loadParam (std::atomic<float>* p, float fallback) noexcept
    {
        return (p != nullptr) ? p->load() : fallback;
    }

    //==========================================================================
    // Oscillator render — per voice, no FM injection
    //==========================================================================
    float renderOscillator (OuieVoice& voice, int /*voiceIdx*/,
                            float freq, int algo) noexcept
    {
        return renderOscillatorFMInput (voice, 0, freq, algo, 0.0f);
    }

    //==========================================================================
    // Oscillator render with optional FM phase injection (STRIFE cross-FM)
    //==========================================================================
    float renderOscillatorFMInput (OuieVoice& voice, int /*voiceIdx*/,
                                   float freq, int algo, float fmInput) noexcept
    {
        switch (algo)
        {
            //------------------------------------------------------------------
            // 0 = VA anti-aliased sawtooth (naive + BLEP correction weight)
            //------------------------------------------------------------------
            case 0:
            {
                voice.vaPhaseInc = freq / srf;
                voice.vaPhase += voice.vaPhaseInc;
                if (voice.vaPhase >= 1.0f) voice.vaPhase -= 1.0f;

                // Naive sawtooth
                float naive = 2.0f * voice.vaPhase - 1.0f;

                // Simple polynomial BLEP correction at discontinuity
                float blep = 0.0f;
                float t = voice.vaPhase / std::max (0.0001f, voice.vaPhaseInc);
                if (t < 1.0f)
                    blep = t * t - 2.0f * t + 1.0f;  // trailing correction
                else
                {
                    t = (voice.vaPhase - 1.0f) / std::max (0.0001f, voice.vaPhaseInc);
                    if (t > -1.0f && t < 0.0f)
                        blep = -(t * t + 2.0f * t + 1.0f);  // leading correction
                }

                float out = naive - blep;
                // FM input modulates phase (cross-FM from STRIFE)
                if (fmInput != 0.0f)
                    out = fastSin ((voice.vaPhase + fmInput * 0.3f) * kTwoPi);

                return softClip (out * 0.8f);
            }

            //------------------------------------------------------------------
            // 1 = Wavetable: 4 waves morphing sine→tri→saw→square
            //------------------------------------------------------------------
            case 1:
            {
                voice.wtPhaseInc = freq / srf;
                voice.wtPhase += voice.wtPhaseInc + fmInput * voice.wtPhaseInc * 0.5f;
                if (voice.wtPhase >= 1.0f) voice.wtPhase -= 1.0f;
                if (voice.wtPhase < 0.0f)  voice.wtPhase += 1.0f;

                float p = voice.wtPhase;
                float morph = voice.wtMorph;  // 0–3 continuous

                // Wave 0: sine
                float w0 = fastSin (p * kTwoPi);
                // Wave 1: triangle
                float w1 = (p < 0.5f) ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p);
                // Wave 2: sawtooth
                float w2 = 2.0f * p - 1.0f;
                // Wave 3: square
                float w3 = (p < 0.5f) ? 1.0f : -1.0f;

                // Piecewise crossfade across the 4 waves
                float out = 0.0f;
                if (morph <= 1.0f)
                    out = lerp (w0, w1, morph);
                else if (morph <= 2.0f)
                    out = lerp (w1, w2, morph - 1.0f);
                else
                    out = lerp (w2, w3, morph - 2.0f);

                return out * 0.8f;
            }

            //------------------------------------------------------------------
            // 2 = FM: 2-operator (carrier + modulator at ratio 2:1)
            //------------------------------------------------------------------
            case 2:
            {
                float ratio    = voice.fmRatio;
                float fmIndex  = voice.fmDepth + std::fabs (fmInput) * 2.0f;

                voice.fmCarrierInc = freq / srf;
                voice.fmModInc     = (freq * ratio) / srf;

                voice.fmModPhase += voice.fmModInc;
                if (voice.fmModPhase >= 1.0f) voice.fmModPhase -= 1.0f;

                float modSig = fastSin (voice.fmModPhase * kTwoPi) * fmIndex;

                voice.fmCarrierPhase += voice.fmCarrierInc + modSig * voice.fmCarrierInc;
                if (voice.fmCarrierPhase >= 1.0f) voice.fmCarrierPhase -= 1.0f;
                if (voice.fmCarrierPhase < 0.0f)  voice.fmCarrierPhase += 1.0f;

                return fastSin (voice.fmCarrierPhase * kTwoPi) * 0.8f;
            }

            //------------------------------------------------------------------
            // 3 = Karplus-Strong pluck
            //------------------------------------------------------------------
            case 3:
            {
                // KS only outputs if the delay line has been excited (noteOn)
                if (!voice.ksActive)
                    return 0.0f;

                voice.ksDelay.setFrequency (freq, srf);
                float out = voice.ksDelay.process();
                return flushDenormal (out * 0.8f);
            }

            default:
                return 0.0f;
        }
    }

    //==========================================================================
    // MIDI note routing
    //==========================================================================

    void handleNoteOn (int noteNum, float velocity,
                       int voiceMode, int splitPoint,
                       float portaCoeff, bool portaAuto,
                       float ampA, float ampD, float ampS, float ampR,
                       float lfo1Rate, float lfo2Rate,
                       int algoA, int algoB,
                       float pitchOffA, float pitchOffB) noexcept
    {
        (void) pitchOffA; (void) pitchOffB;  // used at render time

        // Determine which voice(s) to trigger
        bool triggerA = false;
        bool triggerB = false;

        switch (voiceMode)
        {
            case 0:  // Duo — A takes lower notes, B takes higher notes
                if (!voices[0].active && !voices[1].active)
                {
                    triggerA = true;  // first note always goes to A
                }
                else if (voices[0].active && !voices[1].active)
                {
                    // Send the new note to the appropriate voice based on pitch
                    if (noteNum < voices[0].noteNumber)
                    {
                        // New note is lower — reassign: B gets old A note, A gets new note
                        triggerA = true;
                    }
                    else
                    {
                        triggerB = true;
                    }
                }
                else if (!voices[0].active && voices[1].active)
                {
                    if (noteNum > voices[1].noteNumber)
                        triggerB = true;
                    else
                        triggerA = true;
                }
                else
                {
                    // Both active — steal whichever was playing longer (just retrigger A)
                    triggerA = true;
                }
                break;

            case 1:  // Layer — both voices play same note
                triggerA = true;
                triggerB = true;
                break;

            case 2:  // Split
                if (noteNum < splitPoint)
                    triggerA = true;
                else
                    triggerB = true;
                break;

            default:
                triggerA = true;
                break;
        }

        if (triggerA)
            activateVoice (voices[0], noteNum, velocity, portaCoeff, portaAuto,
                           ampA, ampD, ampS, ampR, lfo1Rate, lfo2Rate, algoA);

        if (triggerB)
            activateVoice (voices[1], noteNum, velocity, portaCoeff, portaAuto,
                           ampA, ampD, ampS, ampR, lfo1Rate, lfo2Rate, algoB);
    }

    void activateVoice (OuieVoice& voice, int noteNum, float velocity,
                        float portaCoeff, bool portaAuto,
                        float ampA, float ampD, float ampS, float ampR,
                        float lfo1Rate, float lfo2Rate, int algo) noexcept
    {
        float freq = midiToFreq (noteNum);

        // Portamento: if voice is active, glide from current freq; otherwise snap
        if (!voice.active || !portaAuto)
            voice.currentFreq = freq;

        voice.targetFreq  = freq;
        voice.portaCoeff  = portaCoeff;
        voice.noteNumber  = noteNum;
        voice.velocity    = velocity;
        voice.active      = true;

        voice.voiceEnv.setParams (ampA, ampD, ampS, ampR, srf);
        voice.voiceEnv.noteOn();

        voice.lfo1.setRate (lfo1Rate, srf);
        voice.lfo2.setRate (lfo2Rate, srf);

        // Algorithm-specific init
        if (algo == 2)  // FM — set reasonable ratio and depth
        {
            voice.fmRatio = 2.0f;
            voice.fmDepth = 1.5f;
        }
        else if (algo == 3)  // KS — excite delay line
        {
            voice.ksDelay.setFrequency (freq, srf);
            voice.ksDelay.excite (velocity, voice.ksRng);
            voice.ksActive = true;
        }
    }

    void handleNoteOff (int noteNum, int voiceMode, int splitPoint) noexcept
    {
        for (int v = 0; v < 2; ++v)
        {
            if (!voices[v].active) continue;
            if (voices[v].noteNumber != noteNum) continue;

            // In split mode, only release the correct voice
            if (voiceMode == 2)
            {
                bool isA = (noteNum < splitPoint);
                if ((v == 0 && !isA) || (v == 1 && isA))
                    continue;
            }

            voices[v].voiceEnv.noteOff();
        }

        // If both voices released, begin master release
        bool anyActive = voices[0].voiceEnv.isActive() || voices[1].voiceEnv.isActive();
        if (!anyActive)
            masterEnv.noteOff();
    }

    //==========================================================================
    // State
    //==========================================================================

    double  sr  = 44100.0;
    float   srf = 44100.0f;
    float   smoothCoeff = 0.01f;

    OuieVoice      voices[2];
    OuieADSR       masterEnv;
    OuieSimpleReverb reverb;

    float smoothedHammerPos = 0.0f;
    float envelopeOutput    = 0.0f;
    int   activeVoiceCount_ = 0;

    // D006 expression
    float modWheelAmount   = 0.0f;
    float aftertouchAmount = 0.0f;

    // Coupling inputs (reset each block)
    float couplingHammerMod = 0.0f;
    float couplingFilterMod = 0.0f;

    // Output cache for coupling
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    //==========================================================================
    // Cached parameter pointers
    //==========================================================================
    std::atomic<float>* paramVoiceAAlgo    = nullptr;
    std::atomic<float>* paramVoiceBAlgo    = nullptr;
    std::atomic<float>* paramVoiceAPitch   = nullptr;
    std::atomic<float>* paramVoiceBPitch   = nullptr;
    std::atomic<float>* paramVoiceALevel   = nullptr;
    std::atomic<float>* paramVoiceBLevel   = nullptr;

    std::atomic<float>* paramFilterACutoff = nullptr;
    std::atomic<float>* paramFilterARes    = nullptr;
    std::atomic<float>* paramFilterBCutoff = nullptr;
    std::atomic<float>* paramFilterBRes    = nullptr;

    std::atomic<float>* paramAmpAtk        = nullptr;
    std::atomic<float>* paramAmpDec        = nullptr;
    std::atomic<float>* paramAmpSus        = nullptr;
    std::atomic<float>* paramAmpRel        = nullptr;

    std::atomic<float>* paramHammerPos     = nullptr;
    std::atomic<float>* paramHammerDepth   = nullptr;

    std::atomic<float>* paramVoiceMode     = nullptr;
    std::atomic<float>* paramSplitPoint    = nullptr;

    std::atomic<float>* paramVelCutoffAmt  = nullptr;
    std::atomic<float>* paramPortaTime     = nullptr;
    std::atomic<float>* paramPortaAuto     = nullptr;

    std::atomic<float>* paramLfo1Rate      = nullptr;
    std::atomic<float>* paramLfo1Depth     = nullptr;
    std::atomic<float>* paramLfo1Target    = nullptr;
    std::atomic<float>* paramLfo2Rate      = nullptr;
    std::atomic<float>* paramLfo2Depth     = nullptr;

    std::atomic<float>* paramReverbMix     = nullptr;

    std::atomic<float>* paramMacroHammer    = nullptr;
    std::atomic<float>* paramMacroAmpullae  = nullptr;
    std::atomic<float>* paramMacroCartilage = nullptr;
    std::atomic<float>* paramMacroCurrent   = nullptr;
};

} // namespace xomnibus
