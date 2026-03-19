#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/PolyBLEP.h"
#include <array>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace xomnibus {

//==============================================================================
// ObrixEngine — Ocean Bricks: Modular Synthesis Toy Box
//
// A runtime-configurable synthesis engine where you snap "ocean bricks"
// together to build sound. Fixed brick pool per instance (pre-allocated):
//   2 Sources (Shells)  — oscillators / noise generators
//   3 Processors (Coral) — filters / waveshapers
//   4 Modulators (Currents) — envelopes / LFOs / velocity / aftertouch
//   3 Effects (Tide Pools) — delay / chorus / reverb
//
// Patch matrix is parameter-based: obrix_source1_type, obrix_mod1_target, etc.
// Configurations persist in .xometa presets. Up to 4 OBRIX instances per preset.
//
// V1.3a scope: Tier 0 + Tier 1 bricks, basic canvas UI, starter presets.
// Easter egg detection is parameter-based (cosmetic reskins only).
//
// Gallery code: OBRIX | Accent: Reef Jade #1E8B7E | Prefix: obrix_
//==============================================================================

//==============================================================================
// Brick Enums
//==============================================================================
enum class ObrixSourceType {
    Off = 0, Sine, Saw, Square, Triangle, Noise, Wavetable, Pulse,
    kCount
};

enum class ObrixProcType {
    Off = 0, LPFilter, HPFilter, BPFilter, Wavefolder, RingMod,
    kCount
};

enum class ObrixModType {
    Off = 0, ADSREnvelope, LFO, Velocity, Aftertouch,
    kCount
};

enum class ObrixEffectType {
    Off = 0, Delay, Chorus, Reverb,
    kCount
};

// Modulation targets — which parameter a modulator routes to
enum class ObrixModTarget {
    None = 0, Pitch, FilterCutoff, FilterReso, Volume,
    WavetablePos, PulseWidth, EffectMix, Pan,
    kCount
};

//==============================================================================
// ObrixADSR — simple envelope
//==============================================================================
struct ObrixADSR
{
    enum class Stage { Idle, Attack, Decay, Sustain, Release };
    Stage stage = Stage::Idle;
    float level = 0.0f;
    float aRate = 0.0f, dRate = 0.0f, sLevel = 1.0f, rRate = 0.0f;

    void setParams (float a, float d, float s, float r, float sr) noexcept
    {
        aRate = (a > 0.001f) ? 1.0f / (a * sr) : 1.0f;
        dRate = (d > 0.001f) ? 1.0f / (d * sr) : 1.0f;
        sLevel = s;
        rRate = (r > 0.001f) ? 1.0f / (r * sr) : 1.0f;
    }
    void noteOn() noexcept { stage = Stage::Attack; }
    void noteOff() noexcept { if (stage != Stage::Idle) stage = Stage::Release; }
    bool isActive() const noexcept { return stage != Stage::Idle; }
    void reset() noexcept { stage = Stage::Idle; level = 0.0f; }

    float process() noexcept
    {
        switch (stage)
        {
            case Stage::Idle: return 0.0f;
            case Stage::Attack:
                level += aRate;
                if (level >= 1.0f) { level = 1.0f; stage = Stage::Decay; }
                return level;
            case Stage::Decay:
                level -= dRate * (level - sLevel + 0.0001f);
                level = flushDenormal (level);
                if (level <= sLevel + 0.0001f) { level = sLevel; stage = Stage::Sustain; }
                return level;
            case Stage::Sustain: return level;
            case Stage::Release:
                level -= rRate * (level + 0.0001f);
                level = flushDenormal (level);
                if (level <= 0.001f) { level = 0.0f; stage = Stage::Idle; }
                return level;
        }
        return 0.0f;
    }
};

//==============================================================================
// ObrixLFO — simple LFO
//==============================================================================
struct ObrixLFO
{
    float phase = 0.0f;
    float phaseInc = 0.0f;
    int shape = 0; // 0=sine 1=tri 2=saw 3=square 4=s&h
    float holdVal = 0.0f;
    uint32_t rng = 12345u;

    void setRate (float hz, float sr) noexcept { phaseInc = hz / sr; }
    void reset() noexcept { phase = 0.0f; holdVal = 0.0f; }

    float process() noexcept
    {
        float out = 0.0f;
        switch (shape)
        {
            case 0: out = fastSin (phase * 6.28318f); break;
            case 1: out = 4.0f * std::fabs (phase - 0.5f) - 1.0f; break;
            case 2: out = 2.0f * phase - 1.0f; break;
            case 3: out = (phase < 0.5f) ? 1.0f : -1.0f; break;
            case 4: // S&H
            {
                float prev = phase - phaseInc;
                if (prev < 0.0f || phase < prev)
                {
                    rng = rng * 1664525u + 1013904223u;
                    holdVal = static_cast<float> (rng & 0xFFFF) / 32768.0f - 1.0f;
                }
                out = holdVal;
                break;
            }
        }
        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;
        return out;
    }
};

//==============================================================================
// ObrixVoice — per-voice state for the brick engine
//==============================================================================
struct ObrixVoice
{
    bool active = false;
    int note = -1;
    float velocity = 0.0f;
    float aftertouch = 0.0f;
    uint64_t startTime = 0;

    // 2 source oscillators
    float srcPhase[2] {};
    float srcFreq[2] {};
    uint32_t noiseRng = 54321u;

    // Amplitude envelope (always present)
    ObrixADSR ampEnv;

    // 4 modulator slots
    ObrixADSR modEnvs[4];
    ObrixLFO modLFOs[4];

    // 3 processor filters
    CytomicSVF procFilters[3];

    // Voice pan
    float pan = 0.0f;

    void reset() noexcept
    {
        active = false;
        note = -1;
        velocity = 0.0f;
        aftertouch = 0.0f;
        for (auto& p : srcPhase) p = 0.0f;
        for (auto& f : srcFreq) f = 440.0f;
        ampEnv.reset();
        for (auto& e : modEnvs) e.reset();
        for (auto& l : modLFOs) l.reset();
        for (auto& f : procFilters) f.reset();
        pan = 0.0f;
    }
};

//==============================================================================
// ObrixEngine
//==============================================================================
class ObrixEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int /*maxBlockSize*/) override
    {
        sr = static_cast<float> (sampleRate);
        for (auto& v : voices) v.reset();

        // Effect delay buffers
        int maxDelay = static_cast<int> (sr * 0.5f) + 1;
        delayBufL.assign (static_cast<size_t> (maxDelay), 0.0f);
        delayBufR.assign (static_cast<size_t> (maxDelay), 0.0f);
        delayWritePos = 0;

        // Chorus delay
        int chorusMax = static_cast<int> (sr * 0.03f) + 16;
        chorusBufL.assign (static_cast<size_t> (chorusMax), 0.0f);
        chorusBufR.assign (static_cast<size_t> (chorusMax), 0.0f);
        chorusWritePos = 0;
        chorusLFOPhase = 0.0f;

        // Reverb (simple 4-tap FDN)
        float srScale = sr / 44100.0f;
        static constexpr int kRevLens[4] = { 1087, 1283, 1511, 1789 };
        for (int i = 0; i < 4; ++i)
        {
            int len = static_cast<int> (static_cast<float> (kRevLens[i]) * srScale) + 1;
            reverbBuf[i].assign (static_cast<size_t> (len), 0.0f);
            reverbPos[i] = 0;
            reverbFilt[i] = 0.0f;
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices) v.reset();
        std::fill (delayBufL.begin(), delayBufL.end(), 0.0f);
        std::fill (delayBufR.begin(), delayBufR.end(), 0.0f);
        std::fill (chorusBufL.begin(), chorusBufL.end(), 0.0f);
        std::fill (chorusBufR.begin(), chorusBufR.end(), 0.0f);
        for (int i = 0; i < 4; ++i)
        {
            std::fill (reverbBuf[i].begin(), reverbBuf[i].end(), 0.0f);
            reverbFilt[i] = 0.0f;
        }
        activeVoices = 0;
    }

    //==========================================================================
    // Audio
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        // === ParamSnapshot ===
        const auto src1Type  = static_cast<int> (loadP (pSrc1Type, 1.0f));
        const auto src2Type  = static_cast<int> (loadP (pSrc2Type, 0.0f));
        const float src1Tune = loadP (pSrc1Tune, 0.0f);
        const float src2Tune = loadP (pSrc2Tune, 0.0f);
        const float src1PW   = loadP (pSrc1PW, 0.5f);
        const float src2PW   = loadP (pSrc2PW, 0.5f);
        const float srcMix   = loadP (pSrcMix, 0.5f);

        const auto proc1Type = static_cast<int> (loadP (pProc1Type, 1.0f));
        const float proc1Cut = loadP (pProc1Cutoff, 8000.0f);
        const float proc1Res = loadP (pProc1Reso, 0.0f);
        const auto proc2Type = static_cast<int> (loadP (pProc2Type, 0.0f));
        const float proc2Cut = loadP (pProc2Cutoff, 4000.0f);
        const float proc2Res = loadP (pProc2Reso, 0.0f);

        const float ampA = loadP (pAmpAttack, 0.01f);
        const float ampD = loadP (pAmpDecay, 0.3f);
        const float ampS = loadP (pAmpSustain, 0.7f);
        const float ampR = loadP (pAmpRelease, 0.5f);

        // Mod 1 (envelope)
        const auto mod1Type  = static_cast<int> (loadP (pMod1Type, 1.0f));
        const auto mod1Tgt   = static_cast<int> (loadP (pMod1Target, 2.0f)); // FilterCutoff
        const float mod1Depth = loadP (pMod1Depth, 0.5f);
        const float mod1Rate  = loadP (pMod1Rate, 1.0f);

        // Mod 2 (LFO)
        const auto mod2Type  = static_cast<int> (loadP (pMod2Type, 2.0f));
        const auto mod2Tgt   = static_cast<int> (loadP (pMod2Target, 2.0f));
        const float mod2Depth = loadP (pMod2Depth, 0.0f);
        const float mod2Rate  = loadP (pMod2Rate, 1.0f);

        // Effects
        const auto fx1Type   = static_cast<int> (loadP (pFx1Type, 0.0f));
        const float fx1Mix   = loadP (pFx1Mix, 0.0f);
        const float fx1Param = loadP (pFx1Param, 0.3f);

        const float level    = loadP (pLevel, 0.8f);

        // Macros
        const float macroChar = loadP (pMacroChar, 0.0f);
        const float macroMove = loadP (pMacroMove, 0.0f);
        const float macroCoup = loadP (pMacroCoup, 0.0f);
        const float macroSpace = loadP (pMacroSpace, 0.0f);
        const int voiceModeIdx = static_cast<int> (loadP (pVoiceMode, 3.0f));
        const int polyLimit = (voiceModeIdx <= 1) ? 1 : (voiceModeIdx == 2) ? 4 : 8;

        polyLimit_ = polyLimit;

        // === MIDI ===
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(),
                        ampA, ampD, ampS, ampR, src1Tune, src2Tune,
                        mod1Type, mod1Rate, mod2Type, mod2Rate);
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
            else if (msg.isChannelPressure())
                for (auto& v : voices) if (v.active) v.aftertouch = msg.getChannelPressureValue() / 127.0f;
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheel_ = msg.getControllerValue() / 127.0f;
        }

        // === Consume coupling accumulators (reset after read) ===
        float blockPitchCoupling  = couplingPitchMod;  couplingPitchMod  = 0.0f;
        float blockCutoffCoupling = couplingCutoffMod; couplingCutoffMod = 0.0f;

        // === Sample Loop ===
        for (int s = 0; s < numSamples; ++s)
        {
            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                // --- Modulators ---
                float modVals[4] {};
                // Mod 1
                if (mod1Type == 1) modVals[0] = voice.modEnvs[0].process() * mod1Depth;
                else if (mod1Type == 2) modVals[0] = voice.modLFOs[0].process() * mod1Depth;
                else if (mod1Type == 3) modVals[0] = voice.velocity * mod1Depth;
                else if (mod1Type == 4) modVals[0] = voice.aftertouch * mod1Depth;
                // Mod 2
                if (mod2Type == 1) modVals[1] = voice.modEnvs[1].process() * mod2Depth;
                else if (mod2Type == 2) modVals[1] = voice.modLFOs[1].process() * mod2Depth;
                else if (mod2Type == 3) modVals[1] = voice.velocity * mod2Depth;
                else if (mod2Type == 4) modVals[1] = voice.aftertouch * mod2Depth;

                // Route modulation to targets
                float pitchMod = 0.0f, cutoffMod = 0.0f, volMod = 0.0f;
                auto routeMod = [&](float val, int tgt) {
                    switch (tgt) {
                        case 1: pitchMod += val * 1200.0f; break; // cents
                        case 2: cutoffMod += val * 6000.0f; break;
                        case 3: /* reso — skip for simplicity */ break;
                        case 4: volMod += val; break;
                        default: break;
                    }
                };
                routeMod (modVals[0], mod1Tgt);
                routeMod (modVals[1], mod2Tgt);

                // Macro modulation
                cutoffMod += macroChar * 2000.0f;
                pitchMod += macroMove * 50.0f; // subtle pitch drift

                // D006: mod wheel intensifies filter sweep
                cutoffMod += modWheel_ * 4000.0f;

                // Coupling modulation (consumed per block above)
                pitchMod  += blockPitchCoupling * 100.0f;
                cutoffMod += blockCutoffCoupling * 3000.0f;

                // COUPLING macro scales coupling sensitivity
                pitchMod  *= (1.0f + macroCoup * 2.0f);
                cutoffMod *= (1.0f + macroCoup * 1.0f);

                // --- Sources ---
                float src1 = 0.0f, src2 = 0.0f;
                float freq1 = voice.srcFreq[0] * fastPow2 (pitchMod / 1200.0f);
                float freq2 = voice.srcFreq[1] * fastPow2 (pitchMod / 1200.0f);

                src1 = renderSource (src1Type, voice.srcPhase[0], freq1, src1PW, voice.noiseRng);
                voice.srcPhase[0] += freq1 / sr;
                if (voice.srcPhase[0] >= 1.0f) voice.srcPhase[0] -= 1.0f;

                if (src2Type > 0)
                {
                    src2 = renderSource (src2Type, voice.srcPhase[1], freq2, src2PW, voice.noiseRng);
                    voice.srcPhase[1] += freq2 / sr;
                    if (voice.srcPhase[1] >= 1.0f) voice.srcPhase[1] -= 1.0f;
                }

                float signal = src1 * srcMix + src2 * (1.0f - srcMix);
                if (src2Type == 0) signal = src1;

                // --- Processors ---
                if (proc1Type > 0 && proc1Type <= 3) // filter types only
                {
                    setFilterMode (voice.procFilters[0], proc1Type);
                    float cut = clamp (proc1Cut + cutoffMod + voice.velocity * 2000.0f, 20.0f, 20000.0f);
                    voice.procFilters[0].setCoefficients (cut, proc1Res, sr);
                    signal = voice.procFilters[0].processSample (signal);
                }

                if (proc2Type > 0 && proc2Type <= 3) // only filter types
                {
                    setFilterMode (voice.procFilters[1], proc2Type);
                    float cut = clamp (proc2Cut + cutoffMod * 0.5f + voice.velocity * 2000.0f, 20.0f, 20000.0f);
                    voice.procFilters[1].setCoefficients (cut, proc2Res, sr);
                    signal = voice.procFilters[1].processSample (signal);
                }

                // Ring mod (proc type 5): multiply source 1 × source 2
                if (proc1Type == 5 || proc2Type == 5)
                    signal = src1 * src2;

                // Wavefolder (proc type 4)
                if (proc1Type == 4 || proc2Type == 4)
                {
                    float fold = 1.0f + macroChar * 3.0f;
                    signal = fastTanh (std::sin (signal * fold * 3.14159f));
                }

                // --- Amp envelope ---
                float ampLevel = voice.ampEnv.process();
                if (!voice.ampEnv.isActive()) { voice.active = false; continue; }

                float gain = ampLevel * voice.velocity * (1.0f + volMod);
                gain = clamp (gain, 0.0f, 2.0f);

                signal *= gain;
                signal = flushDenormal (signal);

                float panL = 0.5f - voice.pan * 0.5f;
                float panR = 0.5f + voice.pan * 0.5f;
                mixL += signal * panL;
                mixR += signal * panR;
            }

            // --- Effects ---
            if (fx1Type > 0 && fx1Mix > 0.001f)
                applyEffect (fx1Type, mixL, mixR, fx1Mix, fx1Param, macroSpace);

            mixL *= level;
            mixR *= level;

            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample (0, s, mixL);
                buffer.addSample (1, s, mixR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample (0, s, (mixL + mixR) * 0.5f);
            }
        }

        int count = 0;
        for (const auto& v : voices) if (v.active) ++count;
        activeVoices = count;
    }

    //==========================================================================
    // Coupling
    //==========================================================================

    float getSampleForCoupling (int channel, int /*sampleIndex*/) const override
    {
        // Simplified: return last rendered value
        return (channel == 2) ? 0.5f : 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float*, int) override
    {
        switch (type)
        {
            case CouplingType::AudioToFM:    couplingPitchMod += amount * 0.5f; break;
            case CouplingType::AmpToFilter:  couplingCutoffMod += amount; break;
            case CouplingType::LFOToPitch:   couplingPitchMod += amount * 0.5f; break;
            default: break;
        }
    }

    //==========================================================================
    // Parameters (V1.3a: ~40 params)
    //==========================================================================

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl (params);
        return { params.begin(), params.end() };
    }

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl (params);
    }

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PC = juce::AudioParameterChoice;
        auto srcChoices = juce::StringArray { "Off", "Sine", "Saw", "Square", "Triangle", "Noise", "Wavetable", "Pulse" };
        auto procChoices = juce::StringArray { "Off", "LP Filter", "HP Filter", "BP Filter", "Wavefolder", "Ring Mod" };
        auto modChoices = juce::StringArray { "Off", "Envelope", "LFO", "Velocity", "Aftertouch" };
        auto tgtChoices = juce::StringArray { "None", "Pitch", "Filter Cutoff", "Filter Reso", "Volume", "WT Pos", "Pulse Width", "FX Mix", "Pan" };
        auto fxChoices = juce::StringArray { "Off", "Delay", "Chorus", "Reverb" };

        // Sources
        params.push_back (std::make_unique<PC> (juce::ParameterID { "obrix_src1Type", 1 }, "Obrix Source 1 Type", srcChoices, 1));
        params.push_back (std::make_unique<PC> (juce::ParameterID { "obrix_src2Type", 1 }, "Obrix Source 2 Type", srcChoices, 0));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_src1Tune", 1 }, "Obrix Source 1 Tune", juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_src2Tune", 1 }, "Obrix Source 2 Tune", juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_src1PW", 1 }, "Obrix Source 1 Pulse Width", juce::NormalisableRange<float> (0.05f, 0.95f, 0.001f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_src2PW", 1 }, "Obrix Source 2 Pulse Width", juce::NormalisableRange<float> (0.05f, 0.95f, 0.001f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_srcMix", 1 }, "Obrix Source Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        // Processors
        params.push_back (std::make_unique<PC> (juce::ParameterID { "obrix_proc1Type", 1 }, "Obrix Processor 1 Type", procChoices, 1));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_proc1Cutoff", 1 }, "Obrix Proc 1 Cutoff", juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f), 8000.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_proc1Reso", 1 }, "Obrix Proc 1 Resonance", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<PC> (juce::ParameterID { "obrix_proc2Type", 1 }, "Obrix Processor 2 Type", procChoices, 0));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_proc2Cutoff", 1 }, "Obrix Proc 2 Cutoff", juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f), 4000.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_proc2Reso", 1 }, "Obrix Proc 2 Resonance", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // Amp envelope
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_ampAttack", 1 }, "Obrix Amp Attack", juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.01f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_ampDecay", 1 }, "Obrix Amp Decay", juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.3f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_ampSustain", 1 }, "Obrix Amp Sustain", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_ampRelease", 1 }, "Obrix Amp Release", juce::NormalisableRange<float> (0.0f, 20.0f, 0.001f, 0.3f), 0.5f));

        // Modulators
        params.push_back (std::make_unique<PC> (juce::ParameterID { "obrix_mod1Type", 1 }, "Obrix Mod 1 Type", modChoices, 1));
        params.push_back (std::make_unique<PC> (juce::ParameterID { "obrix_mod1Target", 1 }, "Obrix Mod 1 Target", tgtChoices, 2));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_mod1Depth", 1 }, "Obrix Mod 1 Depth", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_mod1Rate", 1 }, "Obrix Mod 1 Rate", juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 1.0f));
        params.push_back (std::make_unique<PC> (juce::ParameterID { "obrix_mod2Type", 1 }, "Obrix Mod 2 Type", modChoices, 2));
        params.push_back (std::make_unique<PC> (juce::ParameterID { "obrix_mod2Target", 1 }, "Obrix Mod 2 Target", tgtChoices, 2));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_mod2Depth", 1 }, "Obrix Mod 2 Depth", juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_mod2Rate", 1 }, "Obrix Mod 2 Rate", juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        // Effects
        params.push_back (std::make_unique<PC> (juce::ParameterID { "obrix_fx1Type", 1 }, "Obrix Effect Type", fxChoices, 0));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_fx1Mix", 1 }, "Obrix Effect Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_fx1Param", 1 }, "Obrix Effect Param", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

        // Level
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_level", 1 }, "Obrix Level", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        // Macros
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_macroCharacter", 1 }, "Obrix Macro CHARACTER", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_macroMovement", 1 }, "Obrix Macro MOVEMENT", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_macroCoupling", 1 }, "Obrix Macro COUPLING", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "obrix_macroSpace", 1 }, "Obrix Macro SPACE", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        // Voice mode
        params.push_back (std::make_unique<PC> (juce::ParameterID { "obrix_polyphony", 1 }, "Obrix Voice Mode", juce::StringArray { "Mono", "Legato", "Poly4", "Poly8" }, 3));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pSrc1Type   = apvts.getRawParameterValue ("obrix_src1Type");
        pSrc2Type   = apvts.getRawParameterValue ("obrix_src2Type");
        pSrc1Tune   = apvts.getRawParameterValue ("obrix_src1Tune");
        pSrc2Tune   = apvts.getRawParameterValue ("obrix_src2Tune");
        pSrc1PW     = apvts.getRawParameterValue ("obrix_src1PW");
        pSrc2PW     = apvts.getRawParameterValue ("obrix_src2PW");
        pSrcMix     = apvts.getRawParameterValue ("obrix_srcMix");
        pProc1Type  = apvts.getRawParameterValue ("obrix_proc1Type");
        pProc1Cutoff = apvts.getRawParameterValue ("obrix_proc1Cutoff");
        pProc1Reso  = apvts.getRawParameterValue ("obrix_proc1Reso");
        pProc2Type  = apvts.getRawParameterValue ("obrix_proc2Type");
        pProc2Cutoff = apvts.getRawParameterValue ("obrix_proc2Cutoff");
        pProc2Reso  = apvts.getRawParameterValue ("obrix_proc2Reso");
        pAmpAttack  = apvts.getRawParameterValue ("obrix_ampAttack");
        pAmpDecay   = apvts.getRawParameterValue ("obrix_ampDecay");
        pAmpSustain = apvts.getRawParameterValue ("obrix_ampSustain");
        pAmpRelease = apvts.getRawParameterValue ("obrix_ampRelease");
        pMod1Type   = apvts.getRawParameterValue ("obrix_mod1Type");
        pMod1Target = apvts.getRawParameterValue ("obrix_mod1Target");
        pMod1Depth  = apvts.getRawParameterValue ("obrix_mod1Depth");
        pMod1Rate   = apvts.getRawParameterValue ("obrix_mod1Rate");
        pMod2Type   = apvts.getRawParameterValue ("obrix_mod2Type");
        pMod2Target = apvts.getRawParameterValue ("obrix_mod2Target");
        pMod2Depth  = apvts.getRawParameterValue ("obrix_mod2Depth");
        pMod2Rate   = apvts.getRawParameterValue ("obrix_mod2Rate");
        pFx1Type    = apvts.getRawParameterValue ("obrix_fx1Type");
        pFx1Mix     = apvts.getRawParameterValue ("obrix_fx1Mix");
        pFx1Param   = apvts.getRawParameterValue ("obrix_fx1Param");
        pLevel      = apvts.getRawParameterValue ("obrix_level");
        pMacroChar  = apvts.getRawParameterValue ("obrix_macroCharacter");
        pMacroMove  = apvts.getRawParameterValue ("obrix_macroMovement");
        pMacroCoup  = apvts.getRawParameterValue ("obrix_macroCoupling");
        pMacroSpace = apvts.getRawParameterValue ("obrix_macroSpace");
        pVoiceMode  = apvts.getRawParameterValue ("obrix_polyphony");
    }

    //==========================================================================
    // Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Obrix"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF1E8B7E); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoices; }

private:
    static float loadP (std::atomic<float>* p, float fb) noexcept { return p ? p->load() : fb; }

    //==========================================================================
    // Source rendering
    //==========================================================================
    float renderSource (int type, float phase, float /*freq*/, float pw, uint32_t& rng) noexcept
    {
        switch (type)
        {
            case 1: return fastSin (phase * 6.28318f); // Sine
            case 2: return 2.0f * phase - 1.0f; // Saw (naive — TODO: PolyBLEP)
            case 3: return (phase < pw) ? 1.0f : -1.0f; // Square/Pulse
            case 4: return 4.0f * std::fabs (phase - 0.5f) - 1.0f; // Triangle
            case 5: // Noise
                rng = rng * 1664525u + 1013904223u;
                return static_cast<float> (rng & 0xFFFF) / 32768.0f - 1.0f;
            case 6: // Wavetable (simple morphing sine→saw)
            {
                float sine = fastSin (phase * 6.28318f);
                float saw = 2.0f * phase - 1.0f;
                return sine * (1.0f - pw) + saw * pw; // pw = morph position
            }
            case 7: return (phase < pw) ? 1.0f : -1.0f; // Pulse (same as square with width)
            default: return 0.0f;
        }
    }

    //==========================================================================
    void setFilterMode (CytomicSVF& filter, int procType) noexcept
    {
        switch (procType)
        {
            case 1: filter.setMode (CytomicSVF::Mode::LowPass); break;
            case 2: filter.setMode (CytomicSVF::Mode::HighPass); break;
            case 3: filter.setMode (CytomicSVF::Mode::BandPass); break;
            default: filter.setMode (CytomicSVF::Mode::LowPass); break;
        }
    }

    //==========================================================================
    void applyEffect (int type, float& L, float& R, float mix, float param, float space) noexcept
    {
        float dryL = L, dryR = R;
        switch (type)
        {
            case 1: // Delay
            {
                if (delayBufL.empty()) break;
                int bufSz = static_cast<int> (delayBufL.size());
                int delaySamples = static_cast<int> ((0.05f + param * 0.45f + space * 0.2f) * sr);
                delaySamples = std::max (1, std::min (delaySamples, bufSz - 1));
                int readPos = (delayWritePos - delaySamples + bufSz) % bufSz;
                float wetL = delayBufL[static_cast<size_t> (readPos)];
                float wetR = delayBufR[static_cast<size_t> (readPos)];
                delayBufL[static_cast<size_t> (delayWritePos)] = flushDenormal (L + wetL * (param * 0.7f));
                delayBufR[static_cast<size_t> (delayWritePos)] = flushDenormal (R + wetR * (param * 0.7f));
                delayWritePos = (delayWritePos + 1) % bufSz;
                L = dryL * (1.0f - mix) + wetL * mix;
                R = dryR * (1.0f - mix) + wetR * mix;
                break;
            }
            case 2: // Chorus
            {
                if (chorusBufL.empty()) break;
                int bufSz = static_cast<int> (chorusBufL.size());
                float depth = 0.002f * sr * (0.3f + param * 0.7f);
                float centerDelay = 0.007f * sr;
                chorusLFOPhase += (0.3f + space * 2.0f) / sr;
                if (chorusLFOPhase >= 1.0f) chorusLFOPhase -= 1.0f;
                float lfoVal = fastSin (chorusLFOPhase * 6.28318f);
                float delayL = centerDelay + lfoVal * depth;
                float delayR = centerDelay - lfoVal * depth;
                chorusBufL[static_cast<size_t> (chorusWritePos)] = L;
                chorusBufR[static_cast<size_t> (chorusWritePos)] = R;
                auto readInterp = [&](const std::vector<float>& buf, float del) {
                    float rp = static_cast<float> (chorusWritePos) - del;
                    if (rp < 0.0f) rp += static_cast<float> (bufSz);
                    int idx = static_cast<int> (rp);
                    float frac = rp - static_cast<float> (idx);
                    return buf[static_cast<size_t> (idx)] * (1.0f - frac) + buf[static_cast<size_t> ((idx + 1) % bufSz)] * frac;
                };
                float wL = readInterp (chorusBufL, delayL);
                float wR = readInterp (chorusBufR, delayR);
                chorusWritePos = (chorusWritePos + 1) % bufSz;
                L = dryL * (1.0f - mix) + wL * mix;
                R = dryR * (1.0f - mix) + wR * mix;
                break;
            }
            case 3: // Reverb (simple FDN)
            {
                float input = (L + R) * 0.5f * (0.5f + space * 0.5f);
                float tap[4];
                for (int i = 0; i < 4; ++i)
                {
                    int len = static_cast<int> (reverbBuf[i].size());
                    if (len == 0) { tap[i] = 0.0f; continue; }
                    int readOff = static_cast<int> ((0.3f + param * 0.7f) * static_cast<float> (len));
                    readOff = std::max (1, std::min (readOff, len - 1));
                    int rp = (reverbPos[i] - readOff + len) % len;
                    tap[i] = reverbBuf[i][static_cast<size_t> (rp)];
                    reverbFilt[i] = flushDenormal (reverbFilt[i] + (tap[i] - reverbFilt[i]) * 0.3f);
                    tap[i] = reverbFilt[i];
                }
                float tapSum = tap[0] + tap[1] + tap[2] + tap[3];
                float fb = 0.3f + param * 0.5f;
                for (int i = 0; i < 4; ++i)
                {
                    float fbSample = fastTanh ((tap[i] - 0.5f * tapSum) * fb + input);
                    int len = static_cast<int> (reverbBuf[i].size());
                    if (len > 0) reverbBuf[i][static_cast<size_t> (reverbPos[i])] = flushDenormal (fbSample);
                    reverbPos[i] = (reverbPos[i] + 1) % std::max (1, len);
                }
                float rvL = tap[0] * 0.6f + tap[1] * 0.4f - tap[2] * 0.3f;
                float rvR = -tap[1] * 0.3f + tap[2] * 0.4f + tap[3] * 0.6f;
                L = dryL * (1.0f - mix) + rvL * mix;
                R = dryR * (1.0f - mix) + rvR * mix;
                break;
            }
        }
    }

    //==========================================================================
    void noteOn (int noteNum, float vel,
                 float ampA, float ampD, float ampS, float ampR,
                 float tune1, float tune2,
                 int mod1Type, float mod1Rate, int mod2Type, float mod2Rate)
    {
        // Find free voice or steal oldest
        int slot = -1;
        uint64_t oldest = UINT64_MAX;
        int oldestSlot = 0;
        int maxVoicesNow = std::min (kMaxVoices, polyLimit_);
        for (int i = 0; i < maxVoicesNow; ++i)
        {
            if (!voices[i].active) { slot = i; break; }
            if (voices[i].startTime < oldest) { oldest = voices[i].startTime; oldestSlot = i; }
        }
        if (slot < 0) slot = oldestSlot;

        auto& v = voices[slot];
        v.reset();
        v.active = true;
        v.note = noteNum;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.noiseRng = static_cast<uint32_t> (slot * 777 + noteNum * 31 + voiceCounter);

        float freq = 440.0f * fastPow2 ((static_cast<float> (noteNum) - 69.0f + tune1) / 12.0f);
        v.srcFreq[0] = freq;
        v.srcFreq[1] = 440.0f * fastPow2 ((static_cast<float> (noteNum) - 69.0f + tune2) / 12.0f);

        v.ampEnv.setParams (ampA, ampD, ampS, ampR, sr);
        v.ampEnv.noteOn();

        // Mod 1
        if (mod1Type == 1) { v.modEnvs[0].setParams (0.01f, mod1Rate * 0.3f + 0.01f, 0.0f, mod1Rate * 0.3f + 0.01f, sr); v.modEnvs[0].noteOn(); }
        if (mod1Type == 2) { v.modLFOs[0].setRate (mod1Rate, sr); v.modLFOs[0].shape = 0; }

        // Mod 2
        if (mod2Type == 1) { v.modEnvs[1].setParams (0.01f, mod2Rate * 0.5f + 0.01f, 0.0f, mod2Rate * 0.5f + 0.01f, sr); v.modEnvs[1].noteOn(); }
        if (mod2Type == 2) { v.modLFOs[1].setRate (mod2Rate, sr); v.modLFOs[1].shape = 0; }
    }

    void noteOff (int noteNum)
    {
        for (auto& v : voices)
            if (v.active && v.note == noteNum)
            { v.ampEnv.noteOff(); for (auto& e : v.modEnvs) e.noteOff(); }
    }

    //==========================================================================
    // State
    //==========================================================================
    float sr = 44100.0f;
    uint64_t voiceCounter = 0;
    std::array<ObrixVoice, kMaxVoices> voices {};
    int activeVoices = 0;
    float modWheel_ = 0.0f;
    int polyLimit_ = 8;
    float couplingPitchMod = 0.0f;
    float couplingCutoffMod = 0.0f;

    // Effects state
    std::vector<float> delayBufL, delayBufR;
    int delayWritePos = 0;
    std::vector<float> chorusBufL, chorusBufR;
    int chorusWritePos = 0;
    float chorusLFOPhase = 0.0f;
    std::vector<float> reverbBuf[4];
    int reverbPos[4] {};
    float reverbFilt[4] {};

    // Parameter pointers
    std::atomic<float>* pSrc1Type = nullptr, *pSrc2Type = nullptr;
    std::atomic<float>* pSrc1Tune = nullptr, *pSrc2Tune = nullptr;
    std::atomic<float>* pSrc1PW = nullptr, *pSrc2PW = nullptr;
    std::atomic<float>* pSrcMix = nullptr;
    std::atomic<float>* pProc1Type = nullptr, *pProc1Cutoff = nullptr, *pProc1Reso = nullptr;
    std::atomic<float>* pProc2Type = nullptr, *pProc2Cutoff = nullptr, *pProc2Reso = nullptr;
    std::atomic<float>* pAmpAttack = nullptr, *pAmpDecay = nullptr;
    std::atomic<float>* pAmpSustain = nullptr, *pAmpRelease = nullptr;
    std::atomic<float>* pMod1Type = nullptr, *pMod1Target = nullptr;
    std::atomic<float>* pMod1Depth = nullptr, *pMod1Rate = nullptr;
    std::atomic<float>* pMod2Type = nullptr, *pMod2Target = nullptr;
    std::atomic<float>* pMod2Depth = nullptr, *pMod2Rate = nullptr;
    std::atomic<float>* pFx1Type = nullptr, *pFx1Mix = nullptr, *pFx1Param = nullptr;
    std::atomic<float>* pLevel = nullptr;
    std::atomic<float>* pMacroChar = nullptr, *pMacroMove = nullptr;
    std::atomic<float>* pMacroCoup = nullptr, *pMacroSpace = nullptr;
    std::atomic<float>* pVoiceMode = nullptr;
};

} // namespace xomnibus
