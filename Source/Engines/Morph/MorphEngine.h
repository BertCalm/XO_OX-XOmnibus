#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>

namespace xomnibus {

//==============================================================================
// MorphOscillator — Built-in wavetable morph: sine → saw → square → noise.
//
// Morph parameter (0.0–3.0) crossfades between four timbres stored in
// pre-built 2048-sample wavetables. This is specific to the ODDOSCAR engine —
// the shared WavetableOscillator is for loading external wavetable files.
//==============================================================================
class MorphOscillator
{
public:
    static constexpr int kTableSize = 2048;

    MorphOscillator() { buildTables(); }

    void setFrequency (float freqHz, float sampleRate) noexcept
    {
        if (sampleRate > 0.0f)
            phaseInc = static_cast<double> (freqHz) / static_cast<double> (sampleRate);
    }

    void setMorph (float m) noexcept
    {
        morph = std::max (0.0f, std::min (3.0f, m));
    }

    float processSample() noexcept
    {
        float out = 0.0f;

        int tableIdx = static_cast<int> (std::floor (morph));
        float frac = morph - static_cast<float> (tableIdx);

        if (tableIdx >= 3)
        {
            // Pure noise region
            out = rng.nextFloat() * 2.0f - 1.0f;
        }
        else if (tableIdx == 2 && frac > 0.0f)
        {
            // Square → noise crossfade
            float tableVal = readTable (tableIdx);
            float noiseVal = rng.nextFloat() * 2.0f - 1.0f;
            out = tableVal * (1.0f - frac) + noiseVal * frac;
        }
        else
        {
            float a = readTable (tableIdx);
            float b = readTable (tableIdx + 1);
            out = a * (1.0f - frac) + b * frac;
        }

        phase += phaseInc;
        while (phase >= 1.0) phase -= 1.0;
        while (phase < 0.0) phase += 1.0;

        return out;
    }

    void reset() noexcept { phase = 0.0; }

private:
    void buildTables()
    {
        constexpr double twoPi = 6.28318530717958647692;
        for (int i = 0; i < kTableSize; ++i)
        {
            double p = static_cast<double> (i) / kTableSize;
            sineTable[i]   = static_cast<float> (std::sin (twoPi * p));
            sawTable[i]    = static_cast<float> (2.0 * p - 1.0);
            squareTable[i] = (p < 0.5) ? 1.0f : -1.0f;
        }
    }

    float readTable (int tableIndex) const noexcept
    {
        jassert (tableIndex >= 0 && tableIndex <= 2);
        if (tableIndex < 0 || tableIndex > 2)
            tableIndex = 0; // defensive fallback to sine

        double pos = phase * kTableSize;
        int idx0 = static_cast<int> (pos) & (kTableSize - 1);
        int idx1 = (idx0 + 1) & (kTableSize - 1);
        float frac = static_cast<float> (pos - std::floor (pos));

        const float* table = sineTable;
        if (tableIndex == 1) table = sawTable;
        else if (tableIndex == 2) table = squareTable;

        return table[idx0] * (1.0f - frac) + table[idx1] * frac;
    }

    float sineTable[kTableSize] {};
    float sawTable[kTableSize] {};
    float squareTable[kTableSize] {};
    double phase = 0.0;
    double phaseInc = 0.0;
    float morph = 0.0f;
    juce::Random rng;
};

//==============================================================================
// MoogLadder — 4-pole non-linear ladder filter (Moog style).
//
// The resonant warmth of this filter is central to the ODDOSCAR engine's character.
// Embedded here rather than in the shared DSP library because it's
// architecturally distinct from the general-purpose CytomicSVF.
//==============================================================================
class MoogLadder
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        for (auto& s : stage) s = 0.0;
        for (auto& d : delay) d = 0.0;
    }

    void setCutoff (float freqHz) noexcept
    {
        cutoff = std::max (20.0f, std::min (20000.0f, freqHz));
    }

    void setResonance (float r) noexcept
    {
        resonance = std::max (0.0f, std::min (1.0f, r));
    }

    float processSample (float input) noexcept
    {
        double fc = std::max (0.0, std::min (0.98, 2.0 * cutoff / sr));
        double f = fc * 0.5 * (1.0 + (1.0 - fc));
        double fb = resonance * 4.0 * (1.0 - 0.15 * f * f);

        double inp = static_cast<double> (input) - fb * delay[3];
        inp = static_cast<double> (fastTanh (static_cast<float> (inp)));

        for (int i = 0; i < 4; ++i)
        {
            double s = (i == 0) ? inp : stage[i - 1];
            stage[i] = delay[i] + f * (static_cast<double> (fastTanh (static_cast<float> (s)))
                                      - static_cast<double> (fastTanh (static_cast<float> (delay[i]))));
            delay[i] = stage[i];
            // Prevent denormals in the 4-pole feedback chain
            if (std::fabs (delay[i]) < 1.0e-18) delay[i] = 0.0;
        }

        return static_cast<float> (stage[3]);
    }

private:
    double sr = 44100.0;
    float cutoff = 1000.0f;
    float resonance = 0.0f;
    double stage[4] {};
    double delay[4] {};
};

//==============================================================================
// MorphVoice — per-voice state for the ODDOSCAR (pad) engine.
//==============================================================================
struct MorphVoice
{
    bool active = false;
    bool releasing = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;

    // 3 detuned morph oscillators + sub
    MorphOscillator osc[3];
    PolyBLEP subOsc;

    // Moog ladder filter
    MoogLadder filter;

    // ADSR envelope state
    enum EnvStage { Attack, Decay, Sustain, Release, Off };
    EnvStage envStage = Off;
    float envLevel = 0.0f;

    // Voice stealing crossfade
    float fadeOutLevel = 0.0f;

    // Drift (Perlin-like smooth noise)
    float driftPhase = 0.0f;
    float driftValue = 0.0f;
};

//==============================================================================
// MorphEngine — Lush, evolving pad synthesis (OddOscar — the axolotl).
//
// Features:
//   - Wavetable morph oscillator: sine → saw → square → noise (0.0–3.0)
//   - 3 detuned oscillators for chorus width
//   - Sub oscillator (sine, one octave below)
//   - 4-pole Moog ladder filter (warm, resonant, self-oscillating)
//   - Full ADSR envelope with "Bloom" attack control
//   - Perlin-noise drift modulation (stereo spread + subtle FM)
//   - LRU voice stealing with 5ms crossfade
//   - 16-voice max polyphony
//
// Coupling:
//   - Output: LFO value (0.3 Hz sine, for LFOToPitch on other engines)
//   - Input: AmpToFilter (envelope-driven filter modulation from other engines)
//           EnvToMorph (external envelope → morph position)
//
//==============================================================================
class MorphEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 16;

    //-- SynthEngine interface -------------------------------------------------

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);
        lfoPhase = 0.0;

        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        for (auto& voice : voices)
        {
            voice.active = false;
            voice.releasing = false;
            voice.envStage = MorphVoice::Off;
            voice.envLevel = 0.0f;
            voice.fadeOutLevel = 0.0f;
            voice.driftPhase = 0.0f;

            for (auto& osc : voice.osc)
                osc.reset();
            voice.subOsc.reset();
            voice.filter.prepare (sr);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& voice : voices)
        {
            voice.active = false;
            voice.releasing = false;
            voice.envStage = MorphVoice::Off;
            voice.envLevel = 0.0f;
            voice.fadeOutLevel = 0.0f;
            for (auto& osc : voice.osc)
                osc.reset();
            voice.subOsc.reset();
            voice.filter.reset();
        }
        lfoPhase = 0.0;
        lfoOutput = 0.0f;
        filterCutoffMod = 0.0f;
        morphMod = 0.0f;
        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        // --- ParamSnapshot ---
        const float morphPos   = (pMorph != nullptr) ? pMorph->load() : 0.5f;
        const float attack     = (pBloom != nullptr) ? pBloom->load() : 1.25f;
        const float decayTime  = (pDecay != nullptr) ? pDecay->load() : 2.0f;
        const float sustainLvl = (pSustain != nullptr) ? pSustain->load() : 0.7f;
        const float relTime    = (pRelease != nullptr) ? pRelease->load() : 1.5f;
        const float cutoff     = (pFilterCutoff != nullptr) ? pFilterCutoff->load() : 1200.0f;
        const float reso       = (pFilterReso != nullptr) ? pFilterReso->load() : 0.4f;
        const float driftAmt   = (pDrift != nullptr) ? pDrift->load() : 0.3f;
        const float subLvl     = (pSub != nullptr) ? pSub->load() : 0.5f;
        const float detuneCts  = (pDetune != nullptr) ? pDetune->load() : 12.0f;
        const float level      = (pLevel != nullptr) ? pLevel->load() : 0.8f;
        const int maxPoly      = (pPolyphony != nullptr)
            ? (1 << std::min (4, static_cast<int> (pPolyphony->load()))) : 8; // 0→1,1→2,2→4,3→8,4→16

        // Effective morph position with coupling modulation + CC1 modwheel
        float effectiveMorph = std::max (0.0f, std::min (3.0f, morphPos + morphMod + modWheelMorph));

        // --- Process MIDI events ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), detuneCts, effectiveMorph, cutoff, reso, maxPoly);
            else if (msg.isNoteOff())
            {
                if (!sustainPedalDown)
                    noteOff (msg.getNoteNumber());
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                reset();
                sustainPedalDown = false;
            }
            else if (msg.isController())
            {
                if (msg.getControllerNumber() == 64)
                {
                    bool wasDown = sustainPedalDown;
                    sustainPedalDown = (msg.getControllerValue() >= 64);
                    if (wasDown && !sustainPedalDown)
                    {
                        // Release all held voices on pedal up
                        for (auto& v : voices)
                            if (v.active && !v.releasing)
                            {
                                v.releasing = true;
                                v.envStage = MorphVoice::Release;
                            }
                    }
                }
                else if (msg.getControllerNumber() == 1)
                {
                    // CC1 modwheel → sweep morph position (0→no change, 1→morph+=3)
                    modWheelMorph = static_cast<float> (msg.getControllerValue()) / 127.0f * 3.0f;
                }
            }
        }

        // Reset coupling accumulators (consumed this block)
        filterCutoffMod = 0.0f;
        morphMod = 0.0f;

        // Deactivate voices beyond current polyphony limit
        for (int i = maxPoly; i < kMaxVoices; ++i)
        {
            auto& v = voices[static_cast<size_t> (i)];
            if (v.active)
            {
                v.releasing = true;
                v.envStage = MorphVoice::Release;
            }
        }

        // --- Render audio ---
        // Precompute block-constant LFO increment (avoids division per sample)
        const double lfoPhaseInc = lfoRate / sr;
        constexpr double twoPi = 6.28318530717958647692;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Internal LFO for coupling output (0.3 Hz sine)
            lfoPhase += lfoPhaseInc;
            if (lfoPhase >= 1.0) lfoPhase -= 1.0;
            lfoOutput = fastSin (static_cast<float> (twoPi * lfoPhase));

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                updateEnvelope (voice, attack, decayTime, sustainLvl, relTime);

                if (voice.envStage == MorphVoice::Off)
                {
                    voice.active = false;
                    continue;
                }

                // Update morph on all oscillators
                for (auto& osc : voice.osc)
                    osc.setMorph (effectiveMorph);

                // Drift modulation (Perlin-like smooth noise)
                voice.driftPhase += 0.1f / srf;
                if (voice.driftPhase >= 1.0f) voice.driftPhase -= 1.0f;
                voice.driftValue = perlinNoise (voice.driftPhase) * driftAmt;

                // Generate oscillator mix (3 detuned oscillators)
                float oscMix = 0.0f;
                float detuneSpread[3] = { -detuneCts, 0.0f, detuneCts };
                float baseFreq = midiToHz (static_cast<float> (voice.noteNumber));

                for (int i = 0; i < 3; ++i)
                {
                    float detunedFreq = baseFreq * fastExp (detuneSpread[i] * (0.693147f / 1200.0f));
                    detunedFreq *= (1.0f + voice.driftValue * 0.002f);
                    voice.osc[i].setFrequency (detunedFreq, srf);
                    oscMix += voice.osc[i].processSample();
                }
                oscMix /= 3.0f;

                // Sub oscillator (sine, one octave below)
                voice.subOsc.setFrequency (baseFreq * 0.5f, srf);
                voice.subOsc.setWaveform (PolyBLEP::Waveform::Sine);
                float sub = voice.subOsc.processSample() * subLvl;

                float raw = oscMix + sub;

                // Apply filter with coupling modulation.
                // Note: filterCutoffMod is applied per-sample in the render loop,
                // not at NoteOn — this is intentional. Coupling should continuously
                // modulate running voices, not just stamp initial filter state.
                float modCutoff = cutoff + filterCutoffMod * 2000.0f;
                modCutoff = std::max (20.0f, std::min (20000.0f, modCutoff));
                voice.filter.setCutoff (modCutoff);
                voice.filter.setResonance (reso);
                float filtered = voice.filter.processSample (raw);

                // Voice-stealing crossfade (5ms)
                float stealFade = 1.0f;
                if (voice.fadeOutLevel > 0.0f)
                {
                    voice.fadeOutLevel -= 1.0f / (0.005f * srf);
                    voice.fadeOutLevel = flushDenormal (voice.fadeOutLevel);
                    if (voice.fadeOutLevel <= 0.0f)
                        voice.fadeOutLevel = 0.0f;
                    stealFade = 1.0f - voice.fadeOutLevel;
                }

                // Apply envelope
                float out = filtered * voice.envLevel * voice.velocity * stealFade;

                // Drift-based stereo spread
                constexpr float spread = 0.3f;
                mixL += out * (1.0f + spread * voice.driftValue);
                mixR += out * (1.0f - spread * voice.driftValue);
            }

            // Write to output buffer — soft clip (tanh) prevents hard clips in dense pads
            float outL = fastTanh (mixL * level);
            float outR = fastTanh (mixR * level);

            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample (0, sample, outL);
                buffer.addSample (1, sample, outR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample (0, sample, (outL + outR) * 0.5f);
            }

            // Cache output for coupling reads
            if (sample < static_cast<int> (outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t> (sample)] = outL;
                outputCacheR[static_cast<size_t> (sample)] = outR;
            }
        }
    }

    //-- Coupling --------------------------------------------------------------

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        auto si = static_cast<size_t> (sampleIndex);
        if (channel == 0 && si < outputCacheL.size()) return outputCacheL[si];
        if (channel == 1 && si < outputCacheR.size()) return outputCacheR[si];

        // Channel 2 = LFO output (for LFOToPitch coupling to other engines)
        if (channel == 2) return lfoOutput;
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
            case CouplingType::AmpToFilter:
            {
                // Inverted envelope drives filter (dub pump effect)
                // amount is the source engine's envelope level
                float ducked = 1.0f - amount;
                filterCutoffMod += (ducked - 0.5f) * 2.0f;
                break;
            }
            case CouplingType::EnvToMorph:
                // External envelope shifts morph position
                morphMod += amount;
                break;

            default:
                break; // Other coupling types not supported by ODDOSCAR
        }
    }

    //-- Parameters ------------------------------------------------------------

    // Static helper: add ODDOSCAR parameters to a shared vector (used by processor).
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

private:
    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_morph", 1 }, "Morph Position",
            juce::NormalisableRange<float> (0.0f, 3.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_bloom", 1 }, "Morph Bloom (Attack)",
            juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.4f), 1.25f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_decay", 1 }, "Morph Decay",
            juce::NormalisableRange<float> (0.01f, 8.0f, 0.01f), 2.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_sustain", 1 }, "Morph Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_release", 1 }, "Morph Release",
            juce::NormalisableRange<float> (0.01f, 10.0f, 0.01f, 0.4f), 1.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_filterCutoff", 1 }, "Morph Filter Cutoff",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f), 1200.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_filterReso", 1 }, "Morph Filter Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.4f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_drift", 1 }, "Morph Drift",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_sub", 1 }, "Morph Sub Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_detune", 1 }, "Morph Detune",
            juce::NormalisableRange<float> (0.0f, 50.0f, 0.1f), 12.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_level", 1 }, "Morph Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "morph_polyphony", 1 }, "Morph Polyphony",
            juce::StringArray { "1", "2", "4", "8", "16" }, 3));
    }

public:
    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pMorph        = apvts.getRawParameterValue ("morph_morph");
        pBloom        = apvts.getRawParameterValue ("morph_bloom");
        pDecay        = apvts.getRawParameterValue ("morph_decay");
        pSustain      = apvts.getRawParameterValue ("morph_sustain");
        pRelease      = apvts.getRawParameterValue ("morph_release");
        pFilterCutoff = apvts.getRawParameterValue ("morph_filterCutoff");
        pFilterReso   = apvts.getRawParameterValue ("morph_filterReso");
        pDrift        = apvts.getRawParameterValue ("morph_drift");
        pSub          = apvts.getRawParameterValue ("morph_sub");
        pDetune       = apvts.getRawParameterValue ("morph_detune");
        pLevel        = apvts.getRawParameterValue ("morph_level");
        pPolyphony    = apvts.getRawParameterValue ("morph_polyphony");
    }

    //-- Identity --------------------------------------------------------------

    juce::String getEngineId() const override { return "OddOscar"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFFE8839B); } // Axolotl Gill Pink
    int getMaxVoices() const override { return kMaxVoices; }

private:
    //--------------------------------------------------------------------------
    void noteOn (int noteNumber, float velocity, float detuneCents,
                 float morphPos, float cutoff, float reso, int maxPoly)
    {
        int idx = findFreeVoice (maxPoly);
        auto& voice = voices[static_cast<size_t> (idx)];

        if (voice.active)
            voice.fadeOutLevel = voice.envLevel;
        else
            voice.fadeOutLevel = 0.0f;

        voice.active = true;
        voice.releasing = false;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.startTime = voiceCounter++;
        voice.envStage = MorphVoice::Attack;
        voice.envLevel = 0.0f;

        float freq = midiToHz (static_cast<float> (noteNumber));

        // 3 detuned oscillators
        float detuneSpread[3] = { -detuneCents, 0.0f, detuneCents };
        for (int i = 0; i < 3; ++i)
        {
            float detunedFreq = freq * std::pow (2.0f, detuneSpread[i] / 1200.0f);
            voice.osc[i].setFrequency (detunedFreq, srf);
            voice.osc[i].setMorph (morphPos);
            voice.osc[i].reset();
        }

        // Sub oscillator one octave below
        voice.subOsc.reset();
        voice.subOsc.setFrequency (freq * 0.5f, srf);
        voice.subOsc.setWaveform (PolyBLEP::Waveform::Sine);

        // Filter
        voice.filter.reset();
        voice.filter.setCutoff (cutoff);
        voice.filter.setResonance (reso);

        // Drift — randomize starting phase
        voice.driftPhase = rng.nextFloat();
    }

    void noteOff (int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && !voice.releasing && voice.noteNumber == noteNumber)
            {
                voice.releasing = true;
                voice.envStage = MorphVoice::Release;
            }
        }
    }

    void updateEnvelope (MorphVoice& voice, float attack, float decayTime,
                         float sustainLvl, float relTime) noexcept
    {
        switch (voice.envStage)
        {
            case MorphVoice::Attack:
            {
                float rate = 1.0f / (std::max (0.001f, attack) * srf);
                voice.envLevel += rate;
                if (voice.envLevel >= 1.0f)
                {
                    voice.envLevel = 1.0f;
                    voice.envStage = MorphVoice::Decay;
                }
                break;
            }
            case MorphVoice::Decay:
            {
                float rate = 1.0f / (std::max (0.01f, decayTime) * srf);
                voice.envLevel -= rate;
                voice.envLevel = flushDenormal (voice.envLevel);
                if (voice.envLevel <= sustainLvl)
                {
                    voice.envLevel = sustainLvl;
                    voice.envStage = MorphVoice::Sustain;
                }
                break;
            }
            case MorphVoice::Sustain:
                voice.envLevel = sustainLvl;
                if (sustainLvl < 0.0001f)
                {
                    voice.envLevel = 0.0f;
                    voice.envStage = MorphVoice::Off;
                }
                break;

            case MorphVoice::Release:
            {
                float rate = 1.0f / (std::max (0.01f, relTime) * srf);
                voice.envLevel -= rate;
                voice.envLevel = flushDenormal (voice.envLevel);
                if (voice.envLevel <= 0.0f)
                {
                    voice.envLevel = 0.0f;
                    voice.envStage = MorphVoice::Off;
                }
                break;
            }
            case MorphVoice::Off:
                break;
        }
    }

    int findFreeVoice (int maxPoly)
    {
        int poly = std::min (maxPoly, kMaxVoices);

        for (int i = 0; i < poly; ++i)
            if (!voices[static_cast<size_t> (i)].active)
                return i;

        // LRU voice stealing
        int oldest = 0;
        uint64_t oldestTime = UINT64_MAX;
        for (int i = 0; i < poly; ++i)
        {
            if (voices[static_cast<size_t> (i)].startTime < oldestTime)
            {
                oldestTime = voices[static_cast<size_t> (i)].startTime;
                oldest = i;
            }
        }
        return oldest;
    }

    static float midiToHz (float midiNote) noexcept
    {
        return 440.0f * std::pow (2.0f, (midiNote - 69.0f) / 12.0f);
    }

    static float perlinNoise (float phase) noexcept
    {
        float x = phase * 8.0f;
        int xi = static_cast<int> (std::floor (x));
        float xf = x - static_cast<float> (xi);
        float u = xf * xf * (3.0f - 2.0f * xf); // smoothstep

        auto hash = [] (int n) -> float {
            n = (n << 13) ^ n;
            return 1.0f - static_cast<float> (
                (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff
            ) / 1073741824.0f;
        };

        float a = hash (xi);
        float b = hash (xi + 1);
        return a + u * (b - a);
    }

    //--------------------------------------------------------------------------
    double sr = 44100.0;
    float srf = 44100.0f;
    std::array<MorphVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;

    // Internal LFO for coupling output
    double lfoPhase = 0.0;
    float lfoOutput = 0.0f;
    static constexpr double lfoRate = 0.3; // Hz

    // Coupling accumulators (reset each block)
    float filterCutoffMod = 0.0f;
    float morphMod = 0.0f;

    // MIDI performance state
    bool sustainPedalDown = false; // CC64
    float modWheelMorph = 0.0f;   // CC1 [0, 3.0] — sweeps morph position

    // Output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // RNG for drift initialization
    juce::Random rng;

    // Cached APVTS parameter pointers
    std::atomic<float>* pMorph = nullptr;
    std::atomic<float>* pBloom = nullptr;
    std::atomic<float>* pDecay = nullptr;
    std::atomic<float>* pSustain = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pFilterCutoff = nullptr;
    std::atomic<float>* pFilterReso = nullptr;
    std::atomic<float>* pDrift = nullptr;
    std::atomic<float>* pSub = nullptr;
    std::atomic<float>* pDetune = nullptr;
    std::atomic<float>* pLevel = nullptr;
    std::atomic<float>* pPolyphony = nullptr;
};

} // namespace xomnibus
