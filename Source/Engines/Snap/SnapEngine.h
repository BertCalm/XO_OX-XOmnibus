#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>

namespace xomnibus {

//==============================================================================
// Karplus-Strong string model — embedded in SNAP engine.
// Delay-line excitation with damped feedback, used for KarplusStrong osc mode.
//==============================================================================
class KarplusStrongOsc
{
public:
    static constexpr int kMaxDelaySamples = 4096;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        prevOut = 0.0f;
    }

    void setFrequency (double freqHz) noexcept
    {
        if (freqHz > 0.0 && sr > 0.0)
            delaySamples = std::max (2.0, std::min (static_cast<double> (kMaxDelaySamples - 1), sr / freqHz));
    }

    void setDamping (float d) noexcept { damping = std::max (0.0f, std::min (1.0f, d)); }

    void trigger() noexcept
    {
        int len = std::min (static_cast<int> (delaySamples), kMaxDelaySamples);
        for (int i = 0; i < len; ++i)
            buffer[static_cast<size_t> (i)] = rng.nextFloat() * 2.0f - 1.0f;
        writePos = 0;
    }

    float nextSample() noexcept
    {
        int len = static_cast<int> (delaySamples);
        if (len < 2) return 0.0f;

        // Clamp writePos in case setFrequency shortened the delay mid-playback
        if (writePos >= len)
            writePos = writePos % len;

        int readPos = writePos;
        int nextPos = (readPos + 1) % len;

        float out = buffer[static_cast<size_t> (readPos)];
        float avg = 0.5f * (out + buffer[static_cast<size_t> (nextPos)]);
        float filtered = prevOut + damping * (avg - prevOut);
        prevOut = flushDenormal (filtered);

        buffer[static_cast<size_t> (writePos)] = prevOut;
        writePos = (writePos + 1) % len;

        return out;
    }

    void reset() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        prevOut = 0.0f;
    }

private:
    std::array<float, kMaxDelaySamples> buffer {};
    double sr = 44100.0;
    double delaySamples = 100.0;
    float damping = 0.5f;
    float prevOut = 0.0f;
    int writePos = 0;
    juce::Random rng;
};

//==============================================================================
// SnapVoice — per-voice state for the SNAP (percussive) engine.
//==============================================================================
struct SnapVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;

    // Oscillators
    PolyBLEP sineOsc;
    PolyBLEP noiseOsc;
    PolyBLEP fmOsc;
    KarplusStrongOsc ksOsc[4]; // one per unison sub-voice

    // Filter cascade (HPF -> BPF)
    CytomicSVF hpf;
    CytomicSVF bpf;

    // Envelope
    float envLevel = 0.0f;

    // Voice stealing crossfade
    float fadeOutLevel = 0.0f;

    // Unison
    float detuneOffsets[4] = {};
    float panOffsets[4] = {};

    // Pitch sweep
    float currentPitch = 0.0f;
    float targetPitch = 0.0f;
    float pitchSweepPhase = 0.0f;
};

//==============================================================================
// SnapEngine — Percussive, transient-rich synthesis (from XOddCouple Engine X).
//
// Features:
//   - 3 oscillator modes: Sine+Noise, FM, Karplus-Strong
//   - Pitch snap sweep (slides from +24 semitones down to target)
//   - Unison spread (1, 2, or 4 sub-voices with detune + pan scatter)
//   - Tanh waveshaper driven by snap amount
//   - HPF → BPF filter cascade
//   - Simple decay envelope (no sustain — percussive character)
//   - LRU voice stealing with 5ms crossfade
//
// Coupling:
//   - Output: envelope level (for AmpToFilter, AmpToPitch on other engines)
//   - Input: AmpToPitch (pitch modulation from other engines' envelopes)
//           LFOToPitch (LFO-driven pitch drift from other engines)
//
//==============================================================================
class SnapEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    //-- SynthEngine interface -------------------------------------------------

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        // Allocate output cache for coupling reads
        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        for (auto& voice : voices)
        {
            voice.active = false;
            voice.fadeOutLevel = 0.0f;
            voice.sineOsc.reset();
            voice.noiseOsc.reset();
            voice.noiseOsc.setWaveform (PolyBLEP::Waveform::Saw); // noise-like via high freq
            voice.fmOsc.reset();
            for (auto& ks : voice.ksOsc)
                ks.prepare (sr);
            voice.hpf.reset();
            voice.hpf.setMode (CytomicSVF::Mode::HighPass);
            voice.bpf.reset();
            voice.bpf.setMode (CytomicSVF::Mode::BandPass);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& voice : voices)
        {
            voice.active = false;
            voice.envLevel = 0.0f;
            voice.fadeOutLevel = 0.0f;
            voice.sineOsc.reset();
            voice.noiseOsc.reset();
            voice.fmOsc.reset();
            for (auto& ks : voice.ksOsc)
                ks.reset();
            voice.hpf.reset();
            voice.bpf.reset();
        }
        envelopeOutput = 0.0f;
        externalPitchMod = 0.0f;
        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        // --- ParamSnapshot: read all parameters once per block ---
        const int oscModeIdx = (pOscMode != nullptr)
            ? static_cast<int> (pOscMode->load()) : 0;
        const float snap = (pSnap != nullptr) ? pSnap->load() : 0.4f;
        const float decay = (pDecay != nullptr) ? pDecay->load() : 0.5f;
        const float cutoff = (pFilterCutoff != nullptr) ? pFilterCutoff->load() : 2000.0f;
        const float reso = (pFilterReso != nullptr) ? pFilterReso->load() : 0.3f;
        const float detuneCents = (pDetune != nullptr) ? pDetune->load() : 10.0f;
        const float level = (pLevel != nullptr) ? pLevel->load() : 0.8f;
        const bool pitchLocked = (pPitchLock != nullptr) && (pPitchLock->load() >= 0.5f);
        const int unisonCount = (pUnison != nullptr)
            ? (1 << static_cast<int> (pUnison->load())) : 1; // 0→1, 1→2, 2→4
        const int maxPoly = (pPolyphony != nullptr)
            ? (1 << static_cast<int> (pPolyphony->load())) : 4; // 0→1, 1→2, 2→4, 3→8

        // --- Process MIDI events ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), snap, pitchLocked, detuneCents, unisonCount, cutoff, reso, maxPoly, oscModeIdx);
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
        }

        // Consume coupling accumulators (reset after use, per contract)
        float pitchMod = externalPitchMod;
        externalPitchMod = 0.0f;

        // --- Render audio ---
        // Fade out voices beyond current polyphony limit
        for (int i = maxPoly; i < kMaxVoices; ++i)
            if (voices[static_cast<size_t> (i)].active)
                voices[static_cast<size_t> (i)].envLevel = 0.001f; // force quick fade

        float peakEnv = 0.0f;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                // Decay envelope
                float decayRate = (decay > 0.001f)
                    ? 1.0f / (decay * srf) : 1.0f;
                voice.envLevel -= decayRate;
                if (voice.envLevel <= 0.0f)
                {
                    voice.envLevel = 0.0f;
                    voice.active = false;
                    continue;
                }

                // Voice-stealing crossfade (5ms)
                float stealFade = 1.0f;
                if (voice.fadeOutLevel > 0.0f)
                {
                    voice.fadeOutLevel -= 1.0f / (0.005f * srf);
                    if (voice.fadeOutLevel <= 0.0f)
                        voice.fadeOutLevel = 0.0f;
                    stealFade = 1.0f - voice.fadeOutLevel;
                }

                // Pitch sweep (snap effect)
                voice.pitchSweepPhase += 4.0f / srf;
                float sweepMix = std::min (voice.pitchSweepPhase, 1.0f);
                float currentMidi = voice.currentPitch * (1.0f - sweepMix)
                                  + voice.targetPitch * sweepMix;
                currentMidi += pitchMod;

                float freq = midiToHz (currentMidi);

                // Update filters
                voice.hpf.setCoefficients (cutoff, reso, srf);
                voice.bpf.setCoefficients (cutoff, reso, srf);

                // Generate oscillator output with unison
                int uCount = std::min (unisonCount, 4);
                float voiceL = 0.0f, voiceR = 0.0f;

                for (int u = 0; u < uCount; ++u)
                {
                    float detunedFreq = freq * std::pow (2.0f, voice.detuneOffsets[u] / 12.0f);
                    float uniOut = 0.0f;

                    switch (oscModeIdx)
                    {
                        case 0: // Sine + Noise
                        {
                            voice.sineOsc.setFrequency (detunedFreq, srf);
                            voice.sineOsc.setWaveform (PolyBLEP::Waveform::Sine);
                            float sine = voice.sineOsc.processSample();
                            // Use high-frequency saw as noise-like source
                            voice.noiseOsc.setFrequency (srf * 0.49f, srf);
                            voice.noiseOsc.setWaveform (PolyBLEP::Waveform::Saw);
                            float noise = voice.noiseOsc.processSample() * snap;
                            uniOut = sine + noise * 0.3f;
                            break;
                        }
                        case 1: // FM
                        {
                            voice.fmOsc.setFrequency (detunedFreq * 2.0f, srf);
                            voice.fmOsc.setWaveform (PolyBLEP::Waveform::Sine);
                            float mod = voice.fmOsc.processSample() * snap * 4.0f;
                            float modFreq = detunedFreq + mod * detunedFreq;
                            if (modFreq < 0.0f) modFreq = 0.0f;
                            voice.sineOsc.setFrequency (modFreq, srf);
                            voice.sineOsc.setWaveform (PolyBLEP::Waveform::Sine);
                            uniOut = voice.sineOsc.processSample();
                            break;
                        }
                        case 2: // Karplus-Strong
                        {
                            uniOut = voice.ksOsc[u].nextSample();
                            break;
                        }
                    }

                    // Per-unison panning for stereo spread
                    float pan = 0.5f;
                    if (unisonCount > 1)
                        pan = 0.5f + voice.panOffsets[u] * 0.3f;

                    voiceL += uniOut * (1.0f - pan);
                    voiceR += uniOut * pan;
                }
                voiceL /= static_cast<float> (uCount);
                voiceR /= static_cast<float> (uCount);

                // Snap waveshaper (pre-filter tanh saturation)
                if (snap > 0.0f)
                {
                    float drive = 1.0f + snap * 8.0f;
                    voiceL = fastTanh (voiceL * drive);
                    voiceR = fastTanh (voiceR * drive);
                }

                // Filter cascade: HPF -> BPF (mono sum, then restore stereo)
                float mono = (voiceL + voiceR) * 0.5f;
                float filtered = voice.hpf.processSample (mono);
                filtered = voice.bpf.processSample (filtered);

                // Restore stereo balance
                float filtL, filtR;
                float sum = std::abs (voiceL) + std::abs (voiceR);
                if (sum > 1e-6f)
                {
                    float ratioL = voiceL / (sum * 0.5f);
                    float ratioR = voiceR / (sum * 0.5f);
                    filtL = filtered * ratioL;
                    filtR = filtered * ratioR;
                }
                else
                {
                    filtL = filtered;
                    filtR = filtered;
                }

                // Apply envelope, velocity, and steal fade
                float outL = filtL * voice.envLevel * voice.velocity * stealFade;
                float outR = filtR * voice.envLevel * voice.velocity * stealFade;

                mixL += outL;
                mixR += outR;

                peakEnv = std::max (peakEnv, voice.envLevel);
            }

            // Write to output buffer
            float outL = mixL * level;
            float outR = mixR * level;

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

        envelopeOutput = peakEnv;
    }

    //-- Coupling --------------------------------------------------------------

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        auto si = static_cast<size_t> (sampleIndex);
        if (channel == 0 && si < outputCacheL.size()) return outputCacheL[si];
        if (channel == 1 && si < outputCacheR.size()) return outputCacheR[si];

        // Channel 2 = envelope level (for AmpToFilter, AmpToChoke coupling)
        if (channel == 2) return envelopeOutput;
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
            case CouplingType::AmpToPitch:
            case CouplingType::LFOToPitch:
            case CouplingType::PitchToPitch:
                // Accumulate pitch modulation (max ±0.5 semitones at amount=1.0)
                externalPitchMod += amount * 0.5f;
                break;

            default:
                break; // Other coupling types not supported by SNAP
        }
    }

    //-- Parameters ------------------------------------------------------------

    // Static helper: add SNAP parameters to a shared vector (used by processor).
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
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "snap_oscMode", 1 }, "Snap Osc Mode",
            juce::StringArray { "Sine+Noise", "FM", "Karplus-Strong" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_snap", 1 }, "Snap",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.4f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_decay", 1 }, "Snap Decay",
            juce::NormalisableRange<float> (0.0f, 8.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_filterCutoff", 1 }, "Snap Filter Cutoff",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f), 2000.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_filterReso", 1 }, "Snap Filter Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_detune", 1 }, "Snap Detune",
            juce::NormalisableRange<float> (0.0f, 50.0f, 0.1f), 10.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_level", 1 }, "Snap Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { "snap_pitchLock", 1 }, "Snap Pitch Lock", false));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "snap_unison", 1 }, "Snap Unison",
            juce::StringArray { "1", "2", "4" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "snap_polyphony", 1 }, "Snap Polyphony",
            juce::StringArray { "1", "2", "4", "8" }, 2));
    }

public:
    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pOscMode      = apvts.getRawParameterValue ("snap_oscMode");
        pSnap         = apvts.getRawParameterValue ("snap_snap");
        pDecay        = apvts.getRawParameterValue ("snap_decay");
        pFilterCutoff = apvts.getRawParameterValue ("snap_filterCutoff");
        pFilterReso   = apvts.getRawParameterValue ("snap_filterReso");
        pDetune       = apvts.getRawParameterValue ("snap_detune");
        pLevel        = apvts.getRawParameterValue ("snap_level");
        pPitchLock    = apvts.getRawParameterValue ("snap_pitchLock");
        pUnison       = apvts.getRawParameterValue ("snap_unison");
        pPolyphony    = apvts.getRawParameterValue ("snap_polyphony");
    }

    //-- Identity --------------------------------------------------------------

    juce::String getEngineId() const override { return "Snap"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFFC8553D); }
    int getMaxVoices() const override { return kMaxVoices; }

private:
    //--------------------------------------------------------------------------
    void noteOn (int noteNumber, float velocity, float snap, bool pitchLocked,
                 float detuneCents, int unisonCount, float cutoff, float reso,
                 int maxPoly, int oscModeIdx)
    {
        int idx = findFreeVoice (maxPoly);
        auto& voice = voices[static_cast<size_t> (idx)];

        // Smooth fade-out if stealing
        if (voice.active)
            voice.fadeOutLevel = voice.envLevel;
        else
            voice.fadeOutLevel = 0.0f;

        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.startTime = voiceCounter++;
        voice.envLevel = 1.0f;

        float baseMidi = pitchLocked ? 60.0f : static_cast<float> (noteNumber);
        voice.currentPitch = baseMidi + 24.0f * snap;
        voice.targetPitch = baseMidi;
        voice.pitchSweepPhase = 0.0f;

        float freq = midiToHz (baseMidi);

        // Unison detune and pan scatter
        for (int u = 0; u < 4; ++u)
        {
            voice.detuneOffsets[u] = (static_cast<float> (u) - 1.5f) * detuneCents / 100.0f;
            voice.panOffsets[u] = (static_cast<float> (u) - 1.5f) / 2.0f;
        }

        // Reset oscillators for new note
        voice.sineOsc.reset();
        voice.noiseOsc.reset();
        voice.fmOsc.reset();

        if (oscModeIdx == 2) // Karplus-Strong
        {
            int uCount = std::min (unisonCount, 4);
            for (int u = 0; u < uCount; ++u)
            {
                float detunedFreq = freq * std::pow (2.0f, voice.detuneOffsets[u] / 12.0f);
                voice.ksOsc[u].setFrequency (static_cast<double> (detunedFreq));
                voice.ksOsc[u].setDamping (1.0f - snap);
                voice.ksOsc[u].trigger();
            }
        }

        voice.hpf.reset();
        voice.hpf.setMode (CytomicSVF::Mode::HighPass);
        voice.hpf.setCoefficients (cutoff, reso, srf);
        voice.bpf.reset();
        voice.bpf.setMode (CytomicSVF::Mode::BandPass);
        voice.bpf.setCoefficients (cutoff, reso, srf);
    }

    void noteOff (int noteNumber)
    {
        for (auto& voice : voices)
            if (voice.active && voice.noteNumber == noteNumber)
                voice.envLevel = std::min (voice.envLevel, 0.01f); // quick fade for percussive
    }

    int findFreeVoice (int maxPoly)
    {
        int poly = std::min (maxPoly, kMaxVoices);

        // Find inactive voice within polyphony limit
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

    //--------------------------------------------------------------------------
    double sr = 44100.0;
    float srf = 44100.0f;
    std::array<SnapVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;

    // Coupling state
    float envelopeOutput = 0.0f;
    float externalPitchMod = 0.0f;

    // Output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Cached APVTS parameter pointers
    std::atomic<float>* pOscMode = nullptr;
    std::atomic<float>* pSnap = nullptr;
    std::atomic<float>* pDecay = nullptr;
    std::atomic<float>* pFilterCutoff = nullptr;
    std::atomic<float>* pFilterReso = nullptr;
    std::atomic<float>* pDetune = nullptr;
    std::atomic<float>* pLevel = nullptr;
    std::atomic<float>* pPitchLock = nullptr;
    std::atomic<float>* pUnison = nullptr;
    std::atomic<float>* pPolyphony = nullptr;
};

} // namespace xomnibus
