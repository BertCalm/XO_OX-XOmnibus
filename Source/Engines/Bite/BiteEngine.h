#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PolyBLEP.h"
#include <array>
#include <cmath>

namespace xomnibus {

//==============================================================================
// BiteEngine — XOpossum bass-forward character synth for XOmnibus
//
// "Plush weight meets feral bite" — a bass engine with 4 character stages
// that transform clean low-end into living, breathing bass textures.
//
// Signal chain:
//   OscA (Belly) + OscB (Bite) + Sub + Noise → Mix
//   → Fur (pre-filter saturation)
//   → Filter (Burrow LP / Snarl BP / Wire HP / Hollow Notch)
//   → Chew (post-filter contour)
//   → Gnash (asymmetric bite)
//   → Trash (dirt modes)
//   → Amp Envelope → Output
//
// Coupling inputs:
//   AmpToFilter  → external amplitude opens filter (FAT width → bass filter)
//   AudioToFM    → external audio FM-modulates oscillators (SNAP pluck → bass FM)
//   AmpToChoke   → external amplitude chokes bass (ducking)
//
// Coupling output:
//   getSampleForCoupling() → post-character stereo, feeds DUB / FAT
//
// Accent: Moss Green #4A7C59
// Parameter prefix: poss_
// Max voices: 16
// CPU budget: <10%
//==============================================================================

//==============================================================================
// BiteVoice — per-voice state
//==============================================================================
struct BiteVoice
{
    bool  active      = false;
    int   noteNumber  = -1;
    float velocity    = 0.0f;
    uint64_t startTime = 0;

    // Oscillators
    PolyBLEP oscA;     // Belly oscillator (sine, tri, saw, pulse)
    PolyBLEP oscB;     // Bite oscillator (saw, square, sync-saw, fm-saw, noise-mod)
    PolyBLEP subOsc;   // Sub oscillator (sine, -1 octave)

    // Filter per voice
    CytomicSVF filter;

    // Amp envelope
    enum class EnvStage { Attack, Decay, Sustain, Release, Off };
    EnvStage envStage = EnvStage::Off;
    float envLevel    = 0.0f;

    // Filter envelope
    float filtEnvLevel = 0.0f;
    EnvStage filtEnvStage = EnvStage::Off;

    // Glide
    float currentFreq = 0.0f;
    float targetFreq  = 0.0f;
    float glideRate   = 0.0f; // 0 = instant
};

class BiteEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 16;

    //-- Identity ---------------------------------------------------------------

    juce::String getEngineId()     const override { return "Bite"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF4A7C59); } // Moss Green
    int          getMaxVoices()    const override { return kMaxVoices; }

    //-- Lifecycle --------------------------------------------------------------

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr  = sampleRate;
        srf = static_cast<float>(sr);

        outputCacheL.resize(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheR.resize(static_cast<size_t>(maxBlockSize), 0.0f);

        for (auto& v : voices)
        {
            v.active = false;
            v.envLevel = 0.0f;
            v.envStage = BiteVoice::EnvStage::Off;
            v.oscA.reset();
            v.oscB.reset();
            v.subOsc.reset();
            v.filter.reset();
            v.filter.setMode(CytomicSVF::Mode::LowPass);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
        {
            v.active = false;
            v.envLevel = 0.0f;
            v.envStage = BiteVoice::EnvStage::Off;
            v.filtEnvLevel = 0.0f;
            v.filtEnvStage = BiteVoice::EnvStage::Off;
            v.oscA.reset();
            v.oscB.reset();
            v.subOsc.reset();
            v.filter.reset();
        }
        envelopeOutput = 0.0f;
        externalFilterMod = 0.0f;
        externalFMMod = 0.0f;
        externalChokeMod = 0.0f;
        std::fill(outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill(outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    //-- Parameters ------------------------------------------------------------

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using juce::AudioParameterFloat;
        using juce::AudioParameterChoice;
        using juce::ParameterID;
        using juce::NormalisableRange;

        // OscA (Belly)
        params.push_back(std::make_unique<AudioParameterChoice>(
            ParameterID{"poss_oscAWave", 1}, "Bite OscA Wave",
            juce::StringArray{"Sine", "Triangle", "Saw", "Pulse"}, 0));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_oscALevel", 1}, "Bite OscA Level",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_oscAPulseWidth", 1}, "Bite OscA PW",
            NormalisableRange<float>(0.05f, 0.95f, 0.01f), 0.5f));

        // OscB (Bite)
        params.push_back(std::make_unique<AudioParameterChoice>(
            ParameterID{"poss_oscBWave", 1}, "Bite OscB Wave",
            juce::StringArray{"Saw", "Square", "SyncSaw", "FM-Saw", "NoiseMod"}, 0));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_oscBLevel", 1}, "Bite OscB Level",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_oscBDetune", 1}, "Bite OscB Detune",
            NormalisableRange<float>(0.0f, 50.0f, 0.1f), 5.0f));

        // Sub
        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_subLevel", 1}, "Bite Sub Level",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.4f));

        // Noise
        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_noiseLevel", 1}, "Bite Noise Level",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        // Filter
        params.push_back(std::make_unique<AudioParameterChoice>(
            ParameterID{"poss_filterMode", 1}, "Bite Filter Mode",
            juce::StringArray{"Burrow", "Snarl", "Wire", "Hollow"}, 0));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_filterCutoff", 1}, "Bite Filter Cutoff",
            NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 1200.0f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_filterReso", 1}, "Bite Filter Resonance",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_filterEnvAmt", 1}, "Bite Filter Env Amount",
            NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.4f));

        // Character stages
        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_fur", 1}, "Bite Fur",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.2f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_chew", 1}, "Bite Chew",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_gnash", 1}, "Bite Gnash",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<AudioParameterChoice>(
            ParameterID{"poss_trashMode", 1}, "Bite Trash Mode",
            juce::StringArray{"Off", "Rust", "Splatter", "Fold", "Crushed"}, 0));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_trashAmount", 1}, "Bite Trash Amount",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        // Amp envelope
        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_ampAttack", 1}, "Bite Amp Attack",
            NormalisableRange<float>(0.001f, 4.0f, 0.001f, 0.3f), 0.01f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_ampDecay", 1}, "Bite Amp Decay",
            NormalisableRange<float>(0.01f, 4.0f, 0.01f, 0.4f), 0.3f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_ampSustain", 1}, "Bite Amp Sustain",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_ampRelease", 1}, "Bite Amp Release",
            NormalisableRange<float>(0.01f, 8.0f, 0.01f, 0.3f), 0.15f));

        // Filter envelope
        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_filtEnvAttack", 1}, "Bite Filt Env Attack",
            NormalisableRange<float>(0.001f, 4.0f, 0.001f, 0.3f), 0.005f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_filtEnvDecay", 1}, "Bite Filt Env Decay",
            NormalisableRange<float>(0.01f, 4.0f, 0.01f, 0.4f), 0.4f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_filtEnvSustain", 1}, "Bite Filt Env Sustain",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_filtEnvRelease", 1}, "Bite Filt Env Release",
            NormalisableRange<float>(0.01f, 4.0f, 0.01f, 0.3f), 0.2f));

        // Glide
        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_glide", 1}, "Bite Glide",
            NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // Macros (M1-M4: Belly, Bite, Scurry, Space)
        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_macroBelly", 1}, "Bite M1 Belly",
            NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_macroBite", 1}, "Bite M2 Bite",
            NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_macroScurry", 1}, "Bite M3 Scurry",
            NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_macroSpace", 1}, "Bite M4 Space",
            NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // Output
        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID{"poss_masterVolume", 1}, "Bite Volume",
            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParameters(params);
        return { params.begin(), params.end() };
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        p_oscAWave      = apvts.getRawParameterValue("poss_oscAWave");
        p_oscALevel     = apvts.getRawParameterValue("poss_oscALevel");
        p_oscAPW        = apvts.getRawParameterValue("poss_oscAPulseWidth");
        p_oscBWave      = apvts.getRawParameterValue("poss_oscBWave");
        p_oscBLevel     = apvts.getRawParameterValue("poss_oscBLevel");
        p_oscBDetune    = apvts.getRawParameterValue("poss_oscBDetune");
        p_subLevel      = apvts.getRawParameterValue("poss_subLevel");
        p_noiseLevel    = apvts.getRawParameterValue("poss_noiseLevel");
        p_filterMode    = apvts.getRawParameterValue("poss_filterMode");
        p_filterCutoff  = apvts.getRawParameterValue("poss_filterCutoff");
        p_filterReso    = apvts.getRawParameterValue("poss_filterReso");
        p_filterEnvAmt  = apvts.getRawParameterValue("poss_filterEnvAmt");
        p_fur           = apvts.getRawParameterValue("poss_fur");
        p_chew          = apvts.getRawParameterValue("poss_chew");
        p_gnash         = apvts.getRawParameterValue("poss_gnash");
        p_trashMode     = apvts.getRawParameterValue("poss_trashMode");
        p_trashAmount   = apvts.getRawParameterValue("poss_trashAmount");
        p_ampAttack     = apvts.getRawParameterValue("poss_ampAttack");
        p_ampDecay      = apvts.getRawParameterValue("poss_ampDecay");
        p_ampSustain    = apvts.getRawParameterValue("poss_ampSustain");
        p_ampRelease    = apvts.getRawParameterValue("poss_ampRelease");
        p_filtAttack    = apvts.getRawParameterValue("poss_filtEnvAttack");
        p_filtDecay     = apvts.getRawParameterValue("poss_filtEnvDecay");
        p_filtSustain   = apvts.getRawParameterValue("poss_filtEnvSustain");
        p_filtRelease   = apvts.getRawParameterValue("poss_filtEnvRelease");
        p_glide         = apvts.getRawParameterValue("poss_glide");
        p_macroBelly    = apvts.getRawParameterValue("poss_macroBelly");
        p_macroBite     = apvts.getRawParameterValue("poss_macroBite");
        p_macroScurry   = apvts.getRawParameterValue("poss_macroScurry");
        p_macroSpace    = apvts.getRawParameterValue("poss_macroSpace");
        p_masterVolume  = apvts.getRawParameterValue("poss_masterVolume");
    }

    //-- Coupling --------------------------------------------------------------

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        auto si = static_cast<size_t>(sampleIndex);
        if (channel == 0 && si < outputCacheL.size()) return outputCacheL[si];
        if (channel == 1 && si < outputCacheR.size()) return outputCacheR[si];
        if (channel == 2) return envelopeOutput; // envelope for AmpToFilter
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount,
                            const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
            case CouplingType::AmpToFilter:
                externalFilterMod += amount * 6000.0f; // ±6kHz
                break;

            case CouplingType::AudioToFM:
                externalFMMod += amount * 0.3f; // FM depth
                break;

            case CouplingType::AmpToChoke:
                externalChokeMod = std::max(externalChokeMod, amount);
                break;

            default:
                break;
        }
    }

    //-- Audio -----------------------------------------------------------------

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                     int numSamples) override
    {
        if (numSamples <= 0 || p_oscAWave == nullptr) return;

        juce::ScopedNoDenormals noDenormals;
        buffer.clear();

        // --- ParamSnapshot ---
        int   oscAWave     = static_cast<int>(p_oscAWave->load());
        float oscALevel    = p_oscALevel->load();
        float oscAPW       = p_oscAPW->load();
        int   oscBWave     = static_cast<int>(p_oscBWave->load());
        float oscBLevel    = p_oscBLevel->load();
        float oscBDetune   = p_oscBDetune->load();
        float subLevel     = p_subLevel->load();
        float noiseLevel   = p_noiseLevel->load();
        int   filterMode   = static_cast<int>(p_filterMode->load());
        float filterCutoff = p_filterCutoff->load() + externalFilterMod;
        float filterReso   = p_filterReso->load();
        float filterEnvAmt = p_filterEnvAmt->load();
        float fur          = p_fur->load();
        float chew         = p_chew->load();
        float gnash        = p_gnash->load();
        int   trashMode    = static_cast<int>(p_trashMode->load());
        float trashAmount  = p_trashAmount->load();
        float ampAttack    = p_ampAttack->load();
        float ampDecay     = p_ampDecay->load();
        float ampSustain   = p_ampSustain->load();
        float ampRelease   = p_ampRelease->load();
        float filtAttack   = p_filtAttack->load();
        float filtDecay    = p_filtDecay->load();
        float filtSustain  = p_filtSustain->load();
        float filtRelease  = p_filtRelease->load();
        float glide        = p_glide->load();
        float masterVolume = p_masterVolume->load();

        // --- Apply macros ---
        float belly  = p_macroBelly->load();
        float bite   = p_macroBite->load();
        float scurry = p_macroScurry->load();
        float space  = p_macroSpace->load();

        // M1 BELLY: plush warmth — raises sub, lowers cutoff, softens fur
        if (belly > 0.001f)
        {
            subLevel     = std::max(subLevel, belly * 0.8f);
            filterCutoff = filterCutoff * (1.0f - belly * 0.4f);
            fur          = std::max(fur, belly * 0.3f);
        }

        // M2 BITE: feral aggression — raises oscB, gnash, trash
        if (bite > 0.001f)
        {
            oscBLevel    = std::max(oscBLevel, bite * 0.7f);
            gnash        = std::max(gnash, bite * 0.6f);
            trashAmount  = std::max(trashAmount, bite * 0.4f);
            filterCutoff = filterCutoff * (1.0f + bite * 0.5f);
        }

        // M3 SCURRY: nervous movement — raises filter env, detune
        if (scurry > 0.001f)
        {
            filterEnvAmt = std::max(filterEnvAmt, scurry * 0.7f);
            oscBDetune   = std::max(oscBDetune, scurry * 30.0f);
            noiseLevel   = std::max(noiseLevel, scurry * 0.2f);
        }

        // M4 SPACE: spatial expansion (reserved for FX integration)
        // Currently minimal: slight stereo width via detune
        if (space > 0.001f)
        {
            oscBDetune = std::max(oscBDetune, space * 15.0f);
        }

        // Clamp after macro
        filterCutoff = std::max(20.0f, std::min(20000.0f, filterCutoff));

        // Choke: reduce volume based on external amplitude
        float chokeGain = 1.0f - std::min(1.0f, externalChokeMod);

        // --- Set filter mode for all voices ---
        CytomicSVF::Mode fMode;
        switch (filterMode)
        {
            case 0: fMode = CytomicSVF::Mode::LowPass;  break; // Burrow
            case 1: fMode = CytomicSVF::Mode::BandPass;  break; // Snarl
            case 2: fMode = CytomicSVF::Mode::HighPass;  break; // Wire
            case 3: fMode = CytomicSVF::Mode::Notch;     break; // Hollow
            default: fMode = CytomicSVF::Mode::LowPass;  break;
        }

        // --- MIDI ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), glide);
            else if (msg.isNoteOff())
                noteOff(msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
        }

        // Reset coupling accumulators
        float fmMod = externalFMMod;
        externalFilterMod = 0.0f;
        externalFMMod = 0.0f;
        externalChokeMod = 0.0f;

        float peakEnv = 0.0f;

        // --- Per-sample rendering ---
        for (int n = 0; n < numSamples; ++n)
        {
            float mixL = 0.0f, mixR = 0.0f;

            for (auto& v : voices)
            {
                if (!v.active) continue;

                // Amp envelope
                tickEnvelope(v.envStage, v.envLevel,
                             ampAttack, ampDecay, ampSustain, ampRelease);
                if (v.envStage == BiteVoice::EnvStage::Off)
                {
                    v.active = false;
                    continue;
                }

                // Filter envelope
                tickEnvelope(v.filtEnvStage, v.filtEnvLevel,
                             filtAttack, filtDecay, filtSustain, filtRelease);

                // Glide
                if (v.currentFreq != v.targetFreq)
                {
                    float diff = v.targetFreq - v.currentFreq;
                    if (glide > 0.001f)
                    {
                        float rate = 1.0f / (glide * srf * 0.5f);
                        v.currentFreq += diff * std::min(1.0f, rate);
                    }
                    else
                    {
                        v.currentFreq = v.targetFreq;
                    }
                }

                float freq = v.currentFreq;
                float freqB = freq * std::pow(2.0f, oscBDetune / 1200.0f);

                // FM modulation from coupling
                if (fmMod > 0.001f)
                    freq *= (1.0f + fmMod * std::sin(static_cast<float>(n) * 0.01f));

                // --- OscA (Belly) ---
                float oscASample = 0.0f;
                {
                    PolyBLEP::Waveform wf;
                    switch (oscAWave)
                    {
                        case 0: wf = PolyBLEP::Waveform::Sine;     break;
                        case 1: wf = PolyBLEP::Waveform::Triangle; break;
                        case 2: wf = PolyBLEP::Waveform::Saw;      break;
                        case 3: wf = PolyBLEP::Waveform::Square;   break;
                        default: wf = PolyBLEP::Waveform::Sine;    break;
                    }
                    v.oscA.setWaveform(wf);
                    v.oscA.setFrequency(freq, srf);
                    v.oscA.setPulseWidth(oscAPW);
                    oscASample = v.oscA.processSample();
                }

                // --- OscB (Bite) ---
                float oscBSample = 0.0f;
                {
                    PolyBLEP::Waveform wf;
                    switch (oscBWave)
                    {
                        case 0: wf = PolyBLEP::Waveform::Saw;     break;
                        case 1: wf = PolyBLEP::Waveform::Square;   break;
                        case 2: wf = PolyBLEP::Waveform::Saw;      break; // SyncSaw (simulated)
                        case 3: wf = PolyBLEP::Waveform::Saw;      break; // FM-Saw
                        case 4: wf = PolyBLEP::Waveform::Saw;      break; // NoiseMod
                        default: wf = PolyBLEP::Waveform::Saw;     break;
                    }
                    v.oscB.setWaveform(wf);
                    v.oscB.setFrequency(freqB, srf);

                    oscBSample = v.oscB.processSample();

                    // FM-Saw: modulate with sine
                    if (oscBWave == 3)
                        oscBSample = fastTanh(oscBSample * (1.0f + oscASample * 2.0f));

                    // NoiseMod: multiply with noise
                    if (oscBWave == 4)
                    {
                        noiseState = noiseState * 1103515245u + 12345u;
                        float noise = (static_cast<float>(noiseState >> 16) / 32768.0f) - 1.0f;
                        oscBSample *= (1.0f + noise * 0.5f);
                    }
                }

                // --- Sub (-1 octave sine) ---
                float subSample = 0.0f;
                if (subLevel > 0.001f)
                {
                    v.subOsc.setWaveform(PolyBLEP::Waveform::Sine);
                    v.subOsc.setFrequency(freq * 0.5f, srf);
                    subSample = v.subOsc.processSample();
                }

                // --- Noise ---
                float noiseSample = 0.0f;
                if (noiseLevel > 0.001f)
                {
                    noiseState = noiseState * 1103515245u + 12345u;
                    noiseSample = (static_cast<float>(noiseState >> 16) / 32768.0f) - 1.0f;
                }

                // --- Mix oscillators ---
                float mix = oscASample * oscALevel
                          + oscBSample * oscBLevel
                          + subSample * subLevel
                          + noiseSample * noiseLevel;

                // --- FUR (pre-filter soft saturation) ---
                if (fur > 0.001f)
                {
                    float drive = 1.0f + fur * 4.0f;
                    mix = fastTanh(mix * drive) / drive * (1.0f + fur * 0.5f);
                }

                // --- FILTER ---
                float envCutoff = filterCutoff + v.filtEnvLevel * filterEnvAmt * 10000.0f;
                envCutoff = std::max(20.0f, std::min(20000.0f, envCutoff));
                v.filter.setMode(fMode);
                v.filter.setCoefficients(envCutoff, filterReso, srf);
                mix = v.filter.processSample(mix);

                // --- CHEW (post-filter contour / compression) ---
                if (chew > 0.001f)
                {
                    float threshold = 1.0f - chew * 0.6f;
                    if (std::abs(mix) > threshold)
                    {
                        float sign = (mix > 0.0f) ? 1.0f : -1.0f;
                        float excess = std::abs(mix) - threshold;
                        mix = sign * (threshold + excess / (1.0f + excess * chew * 4.0f));
                    }
                }

                // --- GNASH (asymmetric bite — positive peaks clipped harder) ---
                if (gnash > 0.001f)
                {
                    if (mix > 0.0f)
                        mix = fastTanh(mix * (1.0f + gnash * 6.0f));
                    else
                        mix = fastTanh(mix * (1.0f + gnash * 2.0f));
                }

                // --- TRASH ---
                if (trashMode > 0 && trashAmount > 0.001f)
                    mix = applyTrash(mix, trashMode, trashAmount);

                // Apply envelope + velocity
                float amp = v.envLevel * v.velocity * chokeGain;

                // Slight stereo spread from detune
                float spread = oscBDetune / 100.0f;
                float panL = 0.5f - spread * 0.15f;
                float panR = 0.5f + spread * 0.15f;

                mixL += mix * amp * (1.0f - panL);
                mixR += mix * amp * panR;

                peakEnv = std::max(peakEnv, v.envLevel);
            }

            float outL = mixL * masterVolume;
            float outR = mixR * masterVolume;

            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample(0, n, outL);
                buffer.addSample(1, n, outR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample(0, n, (outL + outR) * 0.5f);
            }

            // Cache for coupling
            if (n < static_cast<int>(outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t>(n)] = outL;
                outputCacheR[static_cast<size_t>(n)] = outR;
            }
        }

        envelopeOutput = peakEnv;
    }

private:
    //-- Helpers ---------------------------------------------------------------

    void noteOn(int noteNumber, float velocity, float glide)
    {
        int idx = findFreeVoice();
        auto& v = voices[static_cast<size_t>(idx)];

        float freq = midiToHz(static_cast<float>(noteNumber));

        v.active     = true;
        v.noteNumber = noteNumber;
        v.velocity   = velocity;
        v.startTime  = voiceCounter++;
        v.envStage   = BiteVoice::EnvStage::Attack;
        v.envLevel   = 0.0f;
        v.filtEnvStage = BiteVoice::EnvStage::Attack;
        v.filtEnvLevel = 0.0f;

        if (glide > 0.001f && v.currentFreq > 0.0f)
            v.targetFreq = freq; // glide from current
        else
        {
            v.currentFreq = freq;
            v.targetFreq  = freq;
        }

        v.oscA.reset();
        v.oscB.reset();
        v.subOsc.reset();
        v.filter.reset();
    }

    void noteOff(int noteNumber)
    {
        for (auto& v : voices)
        {
            if (v.active && v.noteNumber == noteNumber &&
                v.envStage != BiteVoice::EnvStage::Release)
            {
                v.envStage = BiteVoice::EnvStage::Release;
                v.filtEnvStage = BiteVoice::EnvStage::Release;
            }
        }
    }

    int findFreeVoice()
    {
        for (int i = 0; i < kMaxVoices; ++i)
            if (!voices[static_cast<size_t>(i)].active)
                return i;

        // Steal oldest
        int oldest = 0;
        uint64_t oldestTime = UINT64_MAX;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (voices[static_cast<size_t>(i)].startTime < oldestTime)
            {
                oldestTime = voices[static_cast<size_t>(i)].startTime;
                oldest = i;
            }
        }
        return oldest;
    }

    void tickEnvelope(BiteVoice::EnvStage& stage, float& level,
                      float attack, float decay, float sustain, float release) noexcept
    {
        switch (stage)
        {
            case BiteVoice::EnvStage::Attack:
                level += 1.0f / (attack * srf);
                if (level >= 1.0f) { level = 1.0f; stage = BiteVoice::EnvStage::Decay; }
                break;

            case BiteVoice::EnvStage::Decay:
                level -= (1.0f - sustain) / (decay * srf);
                if (level <= sustain) { level = sustain; stage = BiteVoice::EnvStage::Sustain; }
                break;

            case BiteVoice::EnvStage::Sustain:
                level = sustain;
                break;

            case BiteVoice::EnvStage::Release:
                level -= level / (release * srf + 1.0f);
                if (level < 0.001f) { level = 0.0f; stage = BiteVoice::EnvStage::Off; }
                break;

            case BiteVoice::EnvStage::Off:
                break;
        }
    }

    static float midiToHz(float midiNote) noexcept
    {
        return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
    }

    float applyTrash(float x, int mode, float amount) noexcept
    {
        switch (mode)
        {
            case 1: // Rust — asymmetric clipping with harmonics
            {
                float drive = 1.0f + amount * 8.0f;
                float y = x * drive;
                if (y > 0.0f)
                    return fastTanh(y * 1.5f) * (1.0f / drive);
                else
                    return fastTanh(y * 0.8f) * (1.0f / drive);
            }

            case 2: // Splatter — sample-rate-dependent distortion
            {
                float drive = 1.0f + amount * 12.0f;
                return fastTanh(x * drive) * (1.0f / (1.0f + amount * 2.0f));
            }

            case 3: // Fold — wavefolder
            {
                float folded = x * (1.0f + amount * 4.0f);
                while (folded > 1.0f || folded < -1.0f)
                {
                    if (folded > 1.0f) folded = 2.0f - folded;
                    if (folded < -1.0f) folded = -2.0f - folded;
                }
                return folded;
            }

            case 4: // Crushed — bit reduction
            {
                float steps = std::max(2.0f, 256.0f * (1.0f - amount));
                return std::round(x * steps) / steps;
            }

            default:
                return x;
        }
    }

    //-- Members ---------------------------------------------------------------
    double sr  = 44100.0;
    float  srf = 44100.0f;
    std::array<BiteVoice, kMaxVoices> voices {};
    uint64_t voiceCounter = 0;

    // Coupling
    float envelopeOutput    = 0.0f;
    float externalFilterMod = 0.0f;
    float externalFMMod     = 0.0f;
    float externalChokeMod  = 0.0f;

    // Output cache
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Noise state (audio-safe)
    uint32_t noiseState = 98765u;

    // Cached parameter pointers
    std::atomic<float>* p_oscAWave     = nullptr;
    std::atomic<float>* p_oscALevel    = nullptr;
    std::atomic<float>* p_oscAPW       = nullptr;
    std::atomic<float>* p_oscBWave     = nullptr;
    std::atomic<float>* p_oscBLevel    = nullptr;
    std::atomic<float>* p_oscBDetune   = nullptr;
    std::atomic<float>* p_subLevel     = nullptr;
    std::atomic<float>* p_noiseLevel   = nullptr;
    std::atomic<float>* p_filterMode   = nullptr;
    std::atomic<float>* p_filterCutoff = nullptr;
    std::atomic<float>* p_filterReso   = nullptr;
    std::atomic<float>* p_filterEnvAmt = nullptr;
    std::atomic<float>* p_fur          = nullptr;
    std::atomic<float>* p_chew         = nullptr;
    std::atomic<float>* p_gnash        = nullptr;
    std::atomic<float>* p_trashMode    = nullptr;
    std::atomic<float>* p_trashAmount  = nullptr;
    std::atomic<float>* p_ampAttack    = nullptr;
    std::atomic<float>* p_ampDecay     = nullptr;
    std::atomic<float>* p_ampSustain   = nullptr;
    std::atomic<float>* p_ampRelease   = nullptr;
    std::atomic<float>* p_filtAttack   = nullptr;
    std::atomic<float>* p_filtDecay    = nullptr;
    std::atomic<float>* p_filtSustain  = nullptr;
    std::atomic<float>* p_filtRelease  = nullptr;
    std::atomic<float>* p_glide        = nullptr;
    std::atomic<float>* p_macroBelly   = nullptr;
    std::atomic<float>* p_macroBite    = nullptr;
    std::atomic<float>* p_macroScurry  = nullptr;
    std::atomic<float>* p_macroSpace   = nullptr;
    std::atomic<float>* p_masterVolume = nullptr;
};

} // namespace xomnibus
