#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>
#include <vector>

namespace xomnibus {

//==============================================================================
// OmbreMemoryBuffer — Circular delay-line with decay-on-read.
//
// The heart of Oubli (forgetting). Captures audio into a buffer that gradually
// fades. Multiple read heads at staggered positions create a granular
// reconstruction from dissolving memories. The longer the decay, the more
// the past persists. Short decay = fleeting traces. Long = ghost echoes.
//
// Decay is applied lazily at read time using age-based attenuation:
//   amplitude = e^(-age * decayRate)
// where age is the circular distance from readPos to writePos.
// This avoids traversing the full 96k buffer every block (O(N) → O(1) per
// read head) while producing identical results and requiring zero extra
// memory beyond the audio buffer itself.
//
// "Memory is not a recording; it's a reconstruction that decays."
//==============================================================================
class OmbreMemoryBuffer
{
public:
    static constexpr int kMaxBufferSamples = 96000; // ~2 seconds at 48kHz

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }

    void reset() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        for (auto& h : readPhases)
            h = 0.0f;
    }

    // Write a sample into the memory buffer
    void writeSample (float sample) noexcept
    {
        buffer[static_cast<size_t> (writePos)] = sample;
        writePos = (writePos + 1) % kMaxBufferSamples;
    }

    // Read from multiple grain heads with decay-on-read — returns blended reconstruction.
    // decayRate = 1 / (decaySeconds * sampleRate), precomputed by caller.
    // Age is derived from circular distance (writePos - readPos), no timestamps needed.
    float readGrains (float grainSizeMs, float driftAmount, float sampleRate,
                      float decayRate) noexcept
    {
        static constexpr int kNumHeads = 4;
        float grainSamples = (grainSizeMs / 1000.0f) * sampleRate;
        grainSamples = clamp (grainSamples, 8.0f, static_cast<float> (kMaxBufferSamples / 2));

        float output = 0.0f;
        for (int h = 0; h < kNumHeads; ++h)
        {
            // Each head reads from a different offset in the past
            float headOffset = static_cast<float> (h + 1) * grainSamples;

            // Drift shifts the read position slowly (memory distortion)
            readPhases[h] += driftAmount * 0.001f;
            if (readPhases[h] > 1.0f) readPhases[h] -= 1.0f;
            float driftOffset = readPhases[h] * grainSamples * 0.5f;

            int readPos = writePos - static_cast<int> (headOffset + driftOffset);
            while (readPos < 0) readPos += kMaxBufferSamples;
            readPos = readPos % kMaxBufferSamples;

            // Linear interpolation between adjacent samples
            int readPosNext = (readPos + 1) % kMaxBufferSamples;
            float frac = (headOffset + driftOffset) - std::floor (headOffset + driftOffset);

            float rawA = buffer[static_cast<size_t> (readPos)];
            float rawB = buffer[static_cast<size_t> (readPosNext)];

            // Decay-on-read: age = circular distance from readPos to writePos
            int ageA = (writePos - readPos + kMaxBufferSamples) % kMaxBufferSamples;
            float attenA = flushDenormal (fastExp (-static_cast<float> (ageA) * decayRate));
            int ageB = (writePos - readPosNext + kMaxBufferSamples) % kMaxBufferSamples;
            float attenB = flushDenormal (fastExp (-static_cast<float> (ageB) * decayRate));

            float sample = rawA * attenA * (1.0f - frac)
                         + rawB * attenB * frac;

            // Triangular window based on head position within grain
            float headPhase = static_cast<float> (h) / static_cast<float> (kNumHeads);
            float window = 1.0f - std::abs (2.0f * headPhase - 1.0f);
            output += sample * window;
        }

        return output / static_cast<float> (kNumHeads);
    }

private:
    std::array<float, kMaxBufferSamples> buffer {};
    double sr = 44100.0;
    int writePos = 0;
    float readPhases[4] = {};
};

//==============================================================================
// OmbreVoice — per-voice state for the dual-narrative engine.
//
// Each voice contains both halves:
//   - Oubli: memory buffer that records and forgets
//   - Opsis: direct oscillator that reacts immediately
//
// The blend parameter crossfades between them.
// The interference parameter feeds one into the other.
//==============================================================================
struct OmbreVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;

    // --- Opsis (perception) ---
    PolyBLEP oscPrimary;      // Main oscillator
    PolyBLEP oscSub;          // Sub oscillator (one octave down)
    float opsisTransient = 0.0f;  // Transient burst on note-on, decays fast

    // --- Oubli (memory) ---
    OmbreMemoryBuffer memory;

    // --- Shared ---
    CytomicSVF lpf;           // Low-pass filter
    CytomicSVF hpf;           // High-pass for Opsis clarity

    // --- ADSR envelope ---
    enum class EnvStage { Attack, Decay, Sustain, Release, Off };
    EnvStage envStage = EnvStage::Off;
    float envLevel = 0.0f;

    // Voice stealing crossfade
    float fadeOutLevel = 0.0f;
};

//==============================================================================
// OmbreEngine — Dual-narrative synthesis. The 21st XOmnibus engine.
//
// XOmbre: where forgetting meets seeing.
//
// Two halves, one instrument:
//   OUBLI  — Memory buffer that captures and gradually forgets.
//            Granular reconstruction from dissolving traces.
//            The longer you hold, the more ghosts accumulate.
//
//   OPSIS  — Reactive oscillator shaped by velocity and transients.
//            Zero-latency perception. Present-tense sound.
//            Immediate, raw, alive.
//
// The BLEND parameter is the soul — it controls where you sit between
// remembering and perceiving. At 0.0: pure ghost. At 1.0: pure now.
// In between: the interference pattern where past modulates present.
//
// INTERFERENCE feeds the halves into each other:
//   - Opsis output → Oubli memory (what you hear becomes a memory)
//   - Oubli output → Opsis pitch modulation (memories haunt the present)
//
// Coupling:
//   - Output: envelope level (channel 2), audio (channels 0-1)
//   - Input: AmpToFilter (envelope → filter cutoff)
//            LFOToPitch (pitch drift from other engines)
//            AudioToWavetable (external audio → Oubli memory feed)
//            AudioToFM (FM modulation on Opsis oscillator)
//
// Accent: Shadow Mauve #7B6B8A — the color of shadow in warm light.
//
//==============================================================================
class OmbreEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    //-- SynthEngine interface -------------------------------------------------

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        for (auto& voice : voices)
        {
            voice.active = false;
            voice.fadeOutLevel = 0.0f;
            voice.oscPrimary.reset();
            voice.oscSub.reset();
            voice.memory.prepare (sr);
            voice.lpf.reset();
            voice.lpf.setMode (CytomicSVF::Mode::LowPass);
            voice.hpf.reset();
            voice.hpf.setMode (CytomicSVF::Mode::HighPass);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& voice : voices)
        {
            voice.active = false;
            voice.envLevel = 0.0f;
            voice.envStage = OmbreVoice::EnvStage::Off;
            voice.fadeOutLevel = 0.0f;
            voice.oscPrimary.reset();
            voice.oscSub.reset();
            voice.memory.reset();
            voice.lpf.reset();
            voice.hpf.reset();
            voice.opsisTransient = 0.0f;
        }
        envelopeOutput = 0.0f;
        externalPitchMod = 0.0f;
        externalFilterMod = 0.0f;
        externalFMMod = 0.0f;
        externalMemoryFeed = 0.0f;
        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        // --- ParamSnapshot: read all parameters once per block ---
        const float blend       = (pBlend != nullptr) ? pBlend->load() : 0.5f;
        const float memDecaySec = (pMemoryDecay != nullptr) ? pMemoryDecay->load() : 4.0f;
        const float memGrain    = (pMemoryGrain != nullptr) ? pMemoryGrain->load() : 80.0f;
        const float memDrift    = (pMemoryDrift != nullptr) ? pMemoryDrift->load() : 0.2f;
        const int   oscShapeIdx = (pOscShape != nullptr) ? static_cast<int> (pOscShape->load()) : 1;
        const float reactivity  = (pReactivity != nullptr) ? pReactivity->load() : 0.5f;
        const float interference = (pInterference != nullptr) ? pInterference->load() : 0.3f;
        const float cutoff      = (pFilterCutoff != nullptr) ? pFilterCutoff->load() : 4000.0f;
        const float reso        = (pFilterReso != nullptr) ? pFilterReso->load() : 0.3f;
        const float attack      = (pAttack != nullptr) ? pAttack->load() : 0.01f;
        const float decay       = (pDecay != nullptr) ? pDecay->load() : 0.3f;
        const float sustain     = (pSustain != nullptr) ? pSustain->load() : 0.6f;
        const float release     = (pRelease != nullptr) ? pRelease->load() : 0.4f;
        const float level       = (pLevel != nullptr) ? pLevel->load() : 0.8f;
        const float subLevel    = (pSubLevel != nullptr) ? pSubLevel->load() : 0.3f;

        // D002: Macro reads — centered at 0.5, bipolar offset is (value - 0.5)
        const float macroChar  = (pMacroCharacter != nullptr) ? pMacroCharacter->load() : 0.5f;
        const float macroMove  = (pMacroMovement != nullptr)  ? pMacroMovement->load()  : 0.5f;
        const float macroCoup  = (pMacroCoupling != nullptr)  ? pMacroCoupling->load()  : 0.5f;
        const float macroSpace = (pMacroSpace != nullptr)     ? pMacroSpace->load()     : 0.5f;
        // macroCoup: scales coupling receive gain | macroSpace: expands ghost stereo field
        // (applied below in memory feed and stereo spread sections)

        // --- Process MIDI events ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), reactivity);
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount_ = msg.getControllerValue() / 127.0f;
            else if (msg.isChannelPressure())
                aftertouch_ = msg.getChannelPressureValue() / 127.0f; // D006: deepens interference haunting
        }

        // Mod wheel: sweeps blend toward pure Opsis (perception) at full throw.
        // At modWheel=0: blend unchanged. At modWheel=1: blend pushed fully to 1.0.
        // This lets the player shift live between ghost-memory and sharp-present.
        const float effectiveBlend = clamp (blend + modWheelAmount_ * (1.0f - blend), 0.0f, 1.0f);

        // Aftertouch: deepens Oubli haunting — pressure deepens memory interference. (D006)
        const float effectiveInterference = clamp (interference + aftertouch_ * (1.0f - interference), 0.0f, 1.0f);

        // Consume coupling accumulators
        float pitchMod = externalPitchMod;
        externalPitchMod = 0.0f;
        float filterMod = externalFilterMod;
        externalFilterMod = 0.0f;
        float fmMod = externalFMMod;
        externalFMMod = 0.0f;
        float memFeed = externalMemoryFeed;
        externalMemoryFeed = 0.0f;

        // Decay-on-read rate: inverse of (decaySeconds * sampleRate).
        // Each sample's attenuation = exp(-age * decayRate) computed at read time.
        float decayRate = (memDecaySec > 0.001f)
            ? 1.0f / (memDecaySec * srf)
            : 100.0f; // instant decay

        // Map waveform index
        PolyBLEP::Waveform waveform = PolyBLEP::Waveform::Saw;
        switch (oscShapeIdx)
        {
            case 0: waveform = PolyBLEP::Waveform::Sine;     break;
            case 1: waveform = PolyBLEP::Waveform::Saw;      break;
            case 2: waveform = PolyBLEP::Waveform::Square;   break;
            case 3: waveform = PolyBLEP::Waveform::Triangle; break;
        }

        // D005: breathing LFO — modulates filter cutoff
        const float lfoRate  = (pLfoRate != nullptr) ? pLfoRate->load() : 0.08f;
        const float lfoDepth = (pLfoDepth != nullptr) ? pLfoDepth->load() : 0.15f;
        lfoPhase_ += lfoRate * static_cast<float> (numSamples) / srf;
        if (lfoPhase_ >= 1.0f) lfoPhase_ -= 1.0f;
        float lfoVal = std::sin (lfoPhase_ * 6.28318530f) * lfoDepth;

        // Effective filter cutoff with coupling modulation
        // D002: CHARACTER macro offsets filter cutoff (±4000 Hz from center)
        // D005: LFO modulates cutoff (±2000 Hz at full depth)
        float effectiveCutoff = clamp (cutoff + filterMod + (macroChar - 0.5f) * 8000.0f + lfoVal * 2000.0f, 20.0f, 20000.0f);

        // Hoist filter coefficient updates outside sample loop
        for (auto& voice : voices)
        {
            if (!voice.active) continue;
            voice.lpf.setCoefficients (effectiveCutoff, reso, srf);
            voice.hpf.setCoefficients (80.0f, 0.0f, srf); // 80Hz HPF for Opsis clarity
        }

        float peakEnv = 0.0f;

        // --- Render audio ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                // --- ADSR envelope ---
                float envTarget = 0.0f;
                float envRate = 0.0f;
                switch (voice.envStage)
                {
                    case OmbreVoice::EnvStage::Attack:
                        envTarget = 1.0f;
                        envRate = (attack > 0.001f) ? (1.0f / (attack * srf)) : 1.0f;
                        voice.envLevel += envRate;
                        if (voice.envLevel >= 1.0f)
                        {
                            voice.envLevel = 1.0f;
                            voice.envStage = OmbreVoice::EnvStage::Decay;
                        }
                        break;
                    case OmbreVoice::EnvStage::Decay:
                        envRate = (decay > 0.001f) ? (1.0f / (decay * srf)) : 1.0f;
                        voice.envLevel -= envRate * (voice.envLevel - sustain);
                        if (voice.envLevel <= sustain + 0.001f)
                        {
                            voice.envLevel = sustain;
                            voice.envStage = OmbreVoice::EnvStage::Sustain;
                        }
                        break;
                    case OmbreVoice::EnvStage::Sustain:
                        voice.envLevel = sustain;
                        break;
                    case OmbreVoice::EnvStage::Release:
                        envRate = (release > 0.001f) ? (1.0f / (release * srf)) : 1.0f;
                        voice.envLevel -= envRate;
                        if (voice.envLevel <= 0.0f)
                        {
                            voice.envLevel = 0.0f;
                            voice.envStage = OmbreVoice::EnvStage::Off;
                            voice.active = false;
                            continue;
                        }
                        break;
                    case OmbreVoice::EnvStage::Off:
                        voice.active = false;
                        continue;
                }
                voice.envLevel = flushDenormal (voice.envLevel);

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

                // --- Pitch ---
                float midiNote = static_cast<float> (voice.noteNumber) + pitchMod;
                float freq = midiToHz (midiNote);

                // ============================================================
                // OPSIS — perception, the present moment
                // ============================================================
                float opsisOut = 0.0f;
                {
                    // Apply FM coupling modulation
                    float fmFreq = freq + fmMod * freq * 0.5f;
                    if (fmFreq < 1.0f) fmFreq = 1.0f;

                    voice.oscPrimary.setFrequency (fmFreq, srf);
                    voice.oscPrimary.setWaveform (waveform);
                    float primary = voice.oscPrimary.processSample();

                    // Sub oscillator (one octave down, always sine)
                    voice.oscSub.setFrequency (fmFreq * 0.5f, srf);
                    voice.oscSub.setWaveform (PolyBLEP::Waveform::Sine);
                    float sub = voice.oscSub.processSample() * subLevel;

                    // Transient burst — sharp attack that decays exponentially
                    // Reactivity controls how much velocity shapes the transient
                    float transientGain = voice.opsisTransient * reactivity * 4.0f;
                    voice.opsisTransient *= 0.999f - reactivity * 0.003f;
                    voice.opsisTransient = flushDenormal (voice.opsisTransient);

                    // Transient adds saturation intensity
                    float raw = primary + sub;
                    if (transientGain > 0.01f)
                        raw = fastTanh (raw * (1.0f + transientGain));

                    // HPF to keep Opsis crisp
                    opsisOut = voice.hpf.processSample (raw);
                }

                // ============================================================
                // OUBLI — memory, the dissolving past
                // ============================================================
                float oubliOut = 0.0f;
                {
                    // Feed Opsis output into memory (modulated by aftertouch-deepened interference)
                    float feedSample = opsisOut * effectiveInterference;

                    // Also feed external coupling audio into memory.
                    // COUPLING macro scales receive gain — more coupling = more porous to incoming signals.
                    feedSample += memFeed * (0.3f + (macroCoup - 0.5f) * 0.4f);

                    voice.memory.writeSample (feedSample);

                    // Read back granular reconstruction from fading memory
                    // D002: MOVEMENT macro increases memory drift (more modulation)
                    float effectiveMemDrift = std::max (0.0f, std::min (1.0f, memDrift + (macroMove - 0.5f) * 0.6f));
                    oubliOut = voice.memory.readGrains (memGrain, effectiveMemDrift, srf, decayRate);
                }

                // ============================================================
                // BLEND — where past meets present
                // ============================================================

                // Interference: Oubli output modulates Opsis pitch slightly
                // (memories haunt the present)
                if (effectiveInterference > 0.01f)
                {
                    float hauntMod = oubliOut * effectiveInterference * 0.02f;
                    float hauntedFreq = freq * (1.0f + hauntMod);
                    if (hauntedFreq > 1.0f)
                        voice.oscPrimary.setFrequency (hauntedFreq, srf);
                }

                // Crossfade: blend 0.0 = pure Oubli (ghost), 1.0 = pure Opsis (now)
                // effectiveBlend incorporates mod wheel — player can sweep live
                float blended = opsisOut * effectiveBlend + oubliOut * (1.0f - effectiveBlend);

                // Shared low-pass filter
                float filtered = voice.lpf.processSample (blended);

                // Apply envelope and velocity
                float out = filtered * voice.envLevel * voice.velocity * stealFade;

                // Stereo spread — Oubli wider, Opsis centered.
                // SPACE macro expands the ghost stereo field (up to +0.5 additional width).
                float stereoWidth = (1.0f - effectiveBlend) * (0.3f + macroSpace * 0.5f);
                float panMod = oubliOut * stereoWidth;
                float outL = out * (1.0f - panMod * 0.5f);
                float outR = out * (1.0f + panMod * 0.5f);

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

        // Channel 2 = envelope level
        if (channel == 2) return envelopeOutput;
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
            case CouplingType::AmpToFilter:
                // Envelope → filter cutoff modulation
                externalFilterMod += amount * 6000.0f;
                break;

            case CouplingType::LFOToPitch:
            case CouplingType::AmpToPitch:
            case CouplingType::PitchToPitch:
                // Pitch modulation (±0.5 semitones at amount=1.0)
                externalPitchMod += amount * 0.5f;
                break;

            case CouplingType::AudioToFM:
                // FM modulation on Opsis oscillator
                externalFMMod += amount * 0.3f;
                break;

            case CouplingType::AudioToWavetable:
                // External audio feeds into Oubli memory buffer
                externalMemoryFeed += amount;
                break;

            default:
                break;
        }
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
        // --- Dual-narrative core ---

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_blend", 1 }, "Ombre Blend",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_interference", 1 }, "Ombre Interference",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        // --- Oubli (memory) ---

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_memoryDecay", 1 }, "Ombre Memory Decay",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.4f), 4.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_memoryGrain", 1 }, "Ombre Memory Grain",
            juce::NormalisableRange<float> (5.0f, 500.0f, 0.1f, 0.5f), 80.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_memoryDrift", 1 }, "Ombre Memory Drift",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.2f));

        // --- Opsis (perception) ---

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "ombre_oscShape", 1 }, "Ombre Osc Shape",
            juce::StringArray { "Sine", "Saw", "Square", "Triangle" }, 1));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_reactivity", 1 }, "Ombre Reactivity",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_subLevel", 1 }, "Ombre Sub Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        // --- Shared ---

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_filterCutoff", 1 }, "Ombre Filter Cutoff",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f), 4000.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_filterReso", 1 }, "Ombre Filter Reso",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_attack", 1 }, "Ombre Attack",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_decay", 1 }, "Ombre Decay",
            juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.3f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_sustain", 1 }, "Ombre Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_release", 1 }, "Ombre Release",
            juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.3f), 0.4f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_level", 1 }, "Ombre Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        // D005: breathing LFO for autonomous modulation
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_lfoRate", 1 }, "Ombre LFO Rate",
            juce::NormalisableRange<float> (0.005f, 10.0f, 0.001f, 0.3f), 0.08f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_lfoDepth", 1 }, "Ombre LFO Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.15f));

        // D002: 4 macros
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_macroCharacter", 1 }, "CHARACTER",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_macroMovement", 1 }, "MOVEMENT",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_macroCoupling", 1 }, "COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "ombre_macroSpace", 1 }, "SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    }

public:
    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pBlend        = apvts.getRawParameterValue ("ombre_blend");
        pInterference = apvts.getRawParameterValue ("ombre_interference");
        pMemoryDecay  = apvts.getRawParameterValue ("ombre_memoryDecay");
        pMemoryGrain  = apvts.getRawParameterValue ("ombre_memoryGrain");
        pMemoryDrift  = apvts.getRawParameterValue ("ombre_memoryDrift");
        pOscShape     = apvts.getRawParameterValue ("ombre_oscShape");
        pReactivity   = apvts.getRawParameterValue ("ombre_reactivity");
        pSubLevel     = apvts.getRawParameterValue ("ombre_subLevel");
        pFilterCutoff = apvts.getRawParameterValue ("ombre_filterCutoff");
        pFilterReso   = apvts.getRawParameterValue ("ombre_filterReso");
        pAttack       = apvts.getRawParameterValue ("ombre_attack");
        pDecay        = apvts.getRawParameterValue ("ombre_decay");
        pSustain      = apvts.getRawParameterValue ("ombre_sustain");
        pRelease      = apvts.getRawParameterValue ("ombre_release");
        pLevel        = apvts.getRawParameterValue ("ombre_level");
        pLfoRate      = apvts.getRawParameterValue ("ombre_lfoRate");
        pLfoDepth     = apvts.getRawParameterValue ("ombre_lfoDepth");
        pMacroCharacter = apvts.getRawParameterValue ("ombre_macroCharacter");
        pMacroMovement  = apvts.getRawParameterValue ("ombre_macroMovement");
        pMacroCoupling  = apvts.getRawParameterValue ("ombre_macroCoupling");
        pMacroSpace     = apvts.getRawParameterValue ("ombre_macroSpace");
    }

    //-- Identity --------------------------------------------------------------

    juce::String getEngineId() const override { return "Ombre"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF7B6B8A); } // Shadow Mauve
    int getMaxVoices() const override { return kMaxVoices; }

private:
    //--------------------------------------------------------------------------
    void noteOn (int noteNumber, float velocity, float reactivity)
    {
        int idx = findFreeVoice();
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
        voice.envLevel = 0.0f;
        voice.envStage = OmbreVoice::EnvStage::Attack;

        // Opsis transient burst — velocity controls intensity
        voice.opsisTransient = velocity * reactivity;

        // Reset oscillators
        voice.oscPrimary.reset();
        voice.oscSub.reset();

        // Don't reset memory — Oubli accumulates across notes.
        // That's the whole point: the ghost of the last note haunts this one.

        voice.lpf.reset();
        voice.hpf.reset();
        voice.lpf.setMode (CytomicSVF::Mode::LowPass);
        voice.hpf.setMode (CytomicSVF::Mode::HighPass);
    }

    void noteOff (int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber
                && voice.envStage != OmbreVoice::EnvStage::Release)
            {
                voice.envStage = OmbreVoice::EnvStage::Release;
            }
        }
    }

    int findFreeVoice()
    {
        // Find inactive voice
        for (int i = 0; i < kMaxVoices; ++i)
            if (!voices[static_cast<size_t> (i)].active)
                return i;

        // LRU voice stealing
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

    static float midiToHz (float midiNote) noexcept
    {
        return 440.0f * std::pow (2.0f, (midiNote - 69.0f) / 12.0f);
    }

    //--------------------------------------------------------------------------
    double sr = 44100.0;
    float srf = 44100.0f;
    std::array<OmbreVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;

    // D005: breathing LFO state
    float lfoPhase_ = 0.0f;

    // MIDI expression
    float modWheelAmount_ = 0.0f;   // CC#1 — sweeps blend toward Opsis (D006)
    float aftertouch_     = 0.0f;   // channel pressure — deepens Oubli interference haunting (D006)

    // Coupling state
    float envelopeOutput = 0.0f;
    float externalPitchMod = 0.0f;
    float externalFilterMod = 0.0f;
    float externalFMMod = 0.0f;
    float externalMemoryFeed = 0.0f;

    // Output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Cached APVTS parameter pointers
    std::atomic<float>* pBlend = nullptr;
    std::atomic<float>* pInterference = nullptr;
    std::atomic<float>* pMemoryDecay = nullptr;
    std::atomic<float>* pMemoryGrain = nullptr;
    std::atomic<float>* pMemoryDrift = nullptr;
    std::atomic<float>* pOscShape = nullptr;
    std::atomic<float>* pReactivity = nullptr;
    std::atomic<float>* pSubLevel = nullptr;
    std::atomic<float>* pFilterCutoff = nullptr;
    std::atomic<float>* pFilterReso = nullptr;
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pDecay = nullptr;
    std::atomic<float>* pSustain = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pLevel = nullptr;
    std::atomic<float>* pLfoRate = nullptr;
    std::atomic<float>* pLfoDepth = nullptr;
    std::atomic<float>* pMacroCharacter = nullptr;
    std::atomic<float>* pMacroMovement  = nullptr;
    std::atomic<float>* pMacroCoupling  = nullptr;
    std::atomic<float>* pMacroSpace     = nullptr;
};

} // namespace xomnibus
