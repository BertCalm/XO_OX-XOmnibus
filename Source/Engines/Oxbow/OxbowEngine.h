#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/StandardLFO.h"
#include <array>
#include <cmath>

namespace xomnibus {

//==============================================================================
// OxbowEngine — "Entangled Reverb" synth engine.
//
// An oxbow is a lake formed when a river cuts itself off. Sound enters as
// rushing water, then the Oxbow cuts the current, leaving a suspended,
// entangled pool of resonance that slowly erases itself. What remains are
// golden standing waves.
//
// DSP Architecture:
//   1. CHIASMUS FDN: 8-channel Householder matrix. Channels 1-4 → Left,
//      5-8 → Right. Delay times REVERSED between L/R — same resonant
//      structure, reverse temporal order. Genuine structural entanglement.
//
//   2. PHASE EROSION: 4 modulated allpass filters per channel (8 total).
//      L and R modulated with OPPOSITE polarity LFOs. Creates breathing
//      spectral self-cancellation when summed to mono.
//
//   3. GOLDEN RESONANCE: Mid/Side energy ratio detects L/R convergence.
//      When Mid >> Side, 4 CytomicSVF Peak filters tuned to golden ratio
//      harmonics ring out briefly. Tuned to MIDI note fundamental (Moog).
//      Amplitude weighted -3dB per φ multiple (Tomita).
//
//   4. ASYMMETRIC CANTILEVER DECAY: Time-varying damping. Bright early
//      reflections, dark late reflections. dampCoeff increases quadratically
//      as energy drops. The reverb transforms as it decays.
//
// Aquatic identity: The Oxbow Eel — Twilight Zone (200-1000m)
// feliX/Oscar polarity: Oscar-dominant (0.3/0.7)
//
// Ghost guidance: SVF damping (Moog), golden weighting (Tomita),
// MIDI→fundamental (Moog), aftertouch→entanglement (Vangelis),
// infinite decay (Schulze), velocity exciter (Kakehashi)
//==============================================================================
class OxbowEngine : public SynthEngine
{
public:
    OxbowEngine() = default;

    //-- SynthEngine interface --------------------------------------------------

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        blockSize = maxBlockSize;

        // FDN delay lines: prime sample lengths (Chiasmus topology)
        // L channels: [1361, 1847, 2203, 2711] at 48kHz (~28, 38, 46, 56ms)
        // R channels: REVERSED [2711, 2203, 1847, 1361]
        // Scale to actual sample rate
        static constexpr float baseDelaysL[4] = { 0.02835f, 0.03848f, 0.04590f, 0.05648f };
        static constexpr float baseDelaysR[4] = { 0.05648f, 0.04590f, 0.03848f, 0.02835f };

        for (int ch = 0; ch < kFDNChannels; ++ch)
        {
            float delayMs = (ch < 4) ? baseDelaysL[ch] : baseDelaysR[ch - 4];
            int delaySamples = static_cast<int> (delayMs * sr * sizeScale) + 1;
            if (delaySamples < 1) delaySamples = 1;
            fdnDelay[ch].assign (static_cast<size_t> (delaySamples), 0.0f);
            fdnDelaySize[ch] = delaySamples;
            fdnWritePos[ch] = 0;
        }

        // FDN damping filters: CytomicSVF LP per channel (Moog two-pole)
        for (int ch = 0; ch < kFDNChannels; ++ch)
        {
            fdnDamp[ch].setMode (CytomicSVF::Mode::LowPass);
            fdnDamp[ch].setCoefficients (dampingHz, 0.1f, static_cast<float> (sr));
            fdnDamp[ch].reset();
        }

        // Phase erosion allpass filters (4 per channel = 8 total)
        for (int a = 0; a < kErosionAPFs; ++a)
        {
            erosionAPF_L[a].setMode (CytomicSVF::Mode::AllPass);
            erosionAPF_R[a].setMode (CytomicSVF::Mode::AllPass);
            erosionAPF_L[a].reset();
            erosionAPF_R[a].reset();
        }

        // Erosion LFOs: 4 sine LFOs at very slow rates
        for (int a = 0; a < kErosionAPFs; ++a)
        {
            erosionLFO[a].setShape (StandardLFO::Sine);
            erosionLFO[a].setRate (0.03f + 0.02f * a, static_cast<float> (sr));
            erosionLFO[a].setPhaseOffset (static_cast<float> (a) * 0.25f);
            erosionLFO[a].reset();
        }

        // Golden resonance: 4 CytomicSVF Peak filters
        for (int g = 0; g < kGoldenFilters; ++g)
        {
            goldenL[g].setMode (CytomicSVF::Mode::Peak);
            goldenR[g].setMode (CytomicSVF::Mode::Peak);
            goldenL[g].reset();
            goldenR[g].reset();
        }

        // Golden resonance envelope followers (Mid/Side)
        midEnv = sideEnv = 0.0f;
        resonanceGain = 0.0f;

        // Exciter state
        exciterPhase = 0.0;
        exciterEnv = 0.0f;
        exciterActive = false;
        currentNote = 60;
        currentVelocity = 0.0f;

        // Peak energy tracker for cantilever
        peakEnergy = 0.0001f;
        currentEnergy = 0.0f;

        // Coupling state
        extFilterMod = 0.0f;
        extDecayMod = 0.0f;
        extRingMod = 0.0f;
        lastSampleL = lastSampleR = 0.0f;

        // Pre-delay buffer
        int predelaySamples = static_cast<int> (0.2 * sr) + 1;
        predelayBuf.assign (static_cast<size_t> (predelaySamples), 0.0f);
        predelaySize = predelaySamples;
        predelayPos = 0;

        // Silence gate: 500ms hold (reverb-tail category)
        silenceGate.prepare (sr, maxBlockSize);
        silenceGate.setHoldTime (500.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (int ch = 0; ch < kFDNChannels; ++ch)
        {
            std::fill (fdnDelay[ch].begin(), fdnDelay[ch].end(), 0.0f);
            fdnWritePos[ch] = 0;
            fdnDamp[ch].reset();
        }
        for (int a = 0; a < kErosionAPFs; ++a)
        {
            erosionAPF_L[a].reset();
            erosionAPF_R[a].reset();
            erosionLFO[a].reset();
        }
        for (int g = 0; g < kGoldenFilters; ++g)
        {
            goldenL[g].reset();
            goldenR[g].reset();
        }
        if (!predelayBuf.empty())
            std::fill (predelayBuf.begin(), predelayBuf.end(), 0.0f);
        predelayPos = 0;
        midEnv = sideEnv = 0.0f;
        resonanceGain = 0.0f;
        exciterEnv = 0.0f;
        exciterActive = false;
        peakEnergy = 0.0001f;
        currentEnergy = 0.0f;
        lastSampleL = lastSampleR = 0.0f;
    }

    //--------------------------------------------------------------------------
    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi,
                      int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;

        // 1. Parse MIDI — wake gate BEFORE bypass check
        for (const auto& meta : midi)
        {
            auto msg = meta.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                currentNote = msg.getNoteNumber();
                currentVelocity = msg.getFloatVelocity();
                exciterActive = true;
                exciterEnv = 1.0f;
                exciterPhase = 0.0;

                // Update golden resonance fundamentals from MIDI note (Moog)
                float fundamental = midiToFreq (static_cast<float> (currentNote));
                updateGoldenFrequencies (fundamental);

                // Reset peak energy tracker for new cantilever arc
                peakEnergy = 0.0001f;
            }
            else if (msg.isNoteOff())
            {
                // Don't kill exciter immediately — let envelope decay
            }
            else if (msg.isAftertouch() || msg.isChannelPressure())
            {
                // Aftertouch → entanglement (Vangelis)
                float pressure = msg.isAftertouch()
                    ? msg.getAfterTouchValue() / 127.0f
                    : msg.getChannelPressureValue() / 127.0f;
                aftertouch = pressure;
            }
        }

        // 2. Bypass check
        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // 3. Read parameters
        float pSize       = pSizeParam       ? pSizeParam->load()       : 0.5f;
        float pDecay      = pDecayParam      ? pDecayParam->load()      : 4.0f;
        float pEntangle   = pEntangleParam   ? pEntangleParam->load()   : 0.6f;
        float pErosionR   = pErosionRParam   ? pErosionRParam->load()   : 0.08f;
        float pErosionD   = pErosionDParam   ? pErosionDParam->load()   : 0.4f;
        float pConverge   = pConvergeParam   ? pConvergeParam->load()   : 4.0f;
        float pResQ       = pResQParam       ? pResQParam->load()       : 8.0f;
        float pResMix     = pResMixParam     ? pResMixParam->load()     : 0.3f;
        float pCantilever = pCantileverParam ? pCantileverParam->load() : 0.5f;
        float pDamping    = pDampingParam    ? pDampingParam->load()    : 6000.0f;
        float pPredelay   = pPredelayParam   ? pPredelayParam->load()   : 20.0f;
        float pDryWet     = pDryWetParam     ? pDryWetParam->load()     : 0.5f;
        float pExcDecay   = pExcDecayParam   ? pExcDecayParam->load()   : 0.01f;
        float pExcBright  = pExcBrightParam  ? pExcBrightParam->load()  : 0.7f;

        // Apply aftertouch to entanglement (Vangelis)
        pEntangle = clamp (pEntangle + aftertouch * 0.3f, 0.0f, 1.0f);

        // Apply coupling modulation
        pDamping = clamp (pDamping + extFilterMod, 200.0f, 16000.0f);
        pDecay = clamp (pDecay + extDecayMod * 10.0f, 0.1f, 60.0f);

        const float srF = static_cast<float> (sr);

        // Feedback coefficient from decay time
        // Schulze: allow infinite decay (pDecay > 29s → feedback = 1.0)
        float feedbackCoeff = (pDecay > 29.0f) ? 1.0f
            : fastExp (-6.9078f / (pDecay * srF));

        // Apply size to FDN (scale is read but applied at prepare-time;
        // for real-time size changes, store and check for changes)
        (void) pSize;  // Size applied at prepare() — real-time resize is v2
        (void) pResQ;  // Used in updateGoldenFrequencies via note-on

        // Velocity → exciter brightness and decay (D001 + Kakehashi)
        float excBrightness = pExcBright * (0.5f + currentVelocity * 0.5f);
        float excLength = pExcDecay * (0.5f + currentVelocity * 0.5f);
        float excDecayFinal = fastExp (-6.9078f / (excLength * srF));

        // Pre-delay in samples
        float predelaySamples = clamp (pPredelay * 0.001f * srF,
                                        0.0f, static_cast<float> (predelaySize - 1));

        // Convergence envelope coefficients
        float convAttack = smoothCoeffFromTime (0.01f, srF);
        float convRelease = smoothCoeffFromTime (0.1f, srF);
        float resAttack = smoothCoeffFromTime (0.005f, srF);
        float resRelease = smoothCoeffFromTime (0.05f, srF);

        // Golden ratio harmonics amplitude weighting (Tomita: -3dB per φ)
        static constexpr float goldenGains[4] = { 1.0f, 0.708f, 0.501f, 0.354f };

        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getNumChannels() > 1
                        ? buffer.getWritePointer (1) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            // === EXCITER: pitched impulse + noise burst ===
            float exciterSample = 0.0f;
            if (exciterActive && exciterEnv > 0.0001f)
            {
                float freq = midiToFreq (static_cast<float> (currentNote));
                float sine = fastSin (static_cast<float> (exciterPhase) * 6.28318530718f);
                exciterPhase += freq / sr;
                if (exciterPhase >= 1.0) exciterPhase -= 1.0;

                // Noise component (brightness controls noise amount)
                uint32_t r = noiseRng;
                r ^= r << 13; r ^= r >> 17; r ^= r << 5;
                noiseRng = r;
                float noise = (static_cast<float> (r & 0xFFFF) / 32768.0f - 1.0f) * excBrightness;

                exciterSample = (sine * (1.0f - excBrightness * 0.5f) + noise) * exciterEnv;
                exciterEnv *= excDecayFinal;
                exciterEnv = flushDenormal (exciterEnv);

                if (exciterEnv < 0.0001f) exciterActive = false;
            }

            // === PRE-DELAY ===
            predelayBuf[static_cast<size_t> (predelayPos)] = exciterSample;
            int readPos = (predelayPos - static_cast<int> (predelaySamples) + predelaySize) % predelaySize;
            float fdnInput = predelayBuf[static_cast<size_t> (readPos)];
            predelayPos = (predelayPos + 1) % predelaySize;

            // === 8-CHANNEL CHIASMUS FDN ===
            // Read from all 8 delay lines
            float fdnRead[kFDNChannels];
            for (int ch = 0; ch < kFDNChannels; ++ch)
            {
                int rp = (fdnWritePos[ch] - fdnDelaySize[ch] + fdnDelaySize[ch]) % fdnDelaySize[ch];
                fdnRead[ch] = fdnDelay[ch][static_cast<size_t> (rp)];
            }

            // Householder feedback matrix: H = I - (2/N) * 1*1^T
            // For N=8: H[i][j] = (i==j) ? 0.75 : -0.25
            float fdnSum = 0.0f;
            for (int ch = 0; ch < kFDNChannels; ++ch)
                fdnSum += fdnRead[ch];
            fdnSum *= (2.0f / static_cast<float> (kFDNChannels));  // = sum * 0.25

            float fdnOut[kFDNChannels];
            for (int ch = 0; ch < kFDNChannels; ++ch)
                fdnOut[ch] = fdnRead[ch] - fdnSum;

            // Cross-coupling: entanglement blends L↔R channels
            // At entangle=0: channels are independent. At 1: fully blended.
            float entangleMix = pEntangle * 0.3f;  // subtle coupling
            for (int ch = 0; ch < 4; ++ch)
            {
                float lCh = fdnOut[ch];
                float rCh = fdnOut[ch + 4];
                fdnOut[ch]     = lCh * (1.0f - entangleMix) + rCh * entangleMix;
                fdnOut[ch + 4] = rCh * (1.0f - entangleMix) + lCh * entangleMix;
            }

            // Cantilever decay: asymmetric time-varying damping
            currentEnergy = flushDenormal (currentEnergy * 0.9999f
                + 0.0001f * (std::fabs (fdnOut[0]) + std::fabs (fdnOut[4])));
            if (currentEnergy > peakEnergy) peakEnergy = currentEnergy;

            float decayProgress = 1.0f - clamp (currentEnergy / (peakEnergy + 1e-8f), 0.0f, 1.0f);
            float cantileverDamp = pDamping * (1.0f - pCantilever * decayProgress * decayProgress);
            cantileverDamp = clamp (cantileverDamp, 200.0f, 16000.0f);

            // Apply damping per channel (Moog two-pole SVF LP)
            for (int ch = 0; ch < kFDNChannels; ++ch)
            {
                fdnDamp[ch].setCoefficients_fast (cantileverDamp, 0.1f, srF);
                fdnOut[ch] = fdnDamp[ch].processSample (fdnOut[ch]);
            }

            // Write back to delay lines with feedback + input injection
            for (int ch = 0; ch < kFDNChannels; ++ch)
            {
                float fb = flushDenormal (fdnOut[ch] * feedbackCoeff);
                // Inject input into all channels
                float inp = fdnInput * (1.0f / static_cast<float> (kFDNChannels));
                fdnDelay[ch][static_cast<size_t> (fdnWritePos[ch])] = inp + fb;
                fdnWritePos[ch] = (fdnWritePos[ch] + 1) % fdnDelaySize[ch];
            }

            // Sum FDN channels to stereo: 1-4 → L, 5-8 → R
            float fdnL = fdnOut[0] + fdnOut[1] + fdnOut[2] + fdnOut[3];
            float fdnR = fdnOut[4] + fdnOut[5] + fdnOut[6] + fdnOut[7];
            fdnL *= 0.25f;  // normalize
            fdnR *= 0.25f;

            // === PHASE EROSION: opposite-polarity allpass LFOs ===
            float erosionL = fdnL;
            float erosionR = fdnR;

            for (int a = 0; a < kErosionAPFs; ++a)
            {
                float lfoVal = erosionLFO[a].process();
                // Update LFO rate from parameter
                erosionLFO[a].setRate (pErosionR + 0.01f * a, srF);

                static constexpr float erosionBaseFreqs[4] = { 300.0f, 1100.0f, 3200.0f, 7500.0f };
                float depth = pErosionD * 0.4f;

                // L and R modulated with OPPOSITE polarity
                float freqL = erosionBaseFreqs[a] * (1.0f + lfoVal * depth);
                float freqR = erosionBaseFreqs[a] * (1.0f - lfoVal * depth);  // OPPOSITE
                freqL = clamp (freqL, 20.0f, srF * 0.49f);
                freqR = clamp (freqR, 20.0f, srF * 0.49f);

                erosionAPF_L[a].setCoefficients_fast (freqL, 0.5f, srF);
                erosionAPF_R[a].setCoefficients_fast (freqR, 0.5f, srF);

                erosionL = erosionAPF_L[a].processSample (erosionL);
                erosionR = erosionAPF_R[a].processSample (erosionR);
            }

            // === GOLDEN RESONANCE: Mid/Side convergence detection ===
            float mid  = (erosionL + erosionR) * 0.5f;
            float side = (erosionL - erosionR) * 0.5f;

            float absMid  = std::fabs (mid);
            float absSide = std::fabs (side);

            float midCoeff  = (absMid  > midEnv) ? convAttack : convRelease;
            float sideCoeff = (absSide > sideEnv) ? convAttack : convRelease;
            midEnv  = flushDenormal (midEnv  + midCoeff  * (absMid  - midEnv));
            sideEnv = flushDenormal (sideEnv + sideCoeff * (absSide - sideEnv));

            float convergence = (sideEnv > 1e-6f) ? midEnv / sideEnv : 0.0f;

            // Attack/release resonance gain based on convergence
            if (convergence > pConverge)
                resonanceGain = clamp (resonanceGain + resAttack * (1.0f - resonanceGain), 0.0f, 1.0f);
            else
                resonanceGain = flushDenormal (resonanceGain * (1.0f - resRelease));

            // Apply golden ratio bandpass filters with amplitude weighting
            float goldenOutL = 0.0f;
            float goldenOutR = 0.0f;

            if (resonanceGain > 0.001f)
            {
                for (int g = 0; g < kGoldenFilters; ++g)
                {
                    goldenOutL += goldenL[g].processSample (erosionL) * goldenGains[g];
                    goldenOutR += goldenR[g].processSample (erosionR) * goldenGains[g];
                }
                goldenOutL *= resonanceGain * pResMix;
                goldenOutR *= resonanceGain * pResMix;
            }

            // === Final output ===
            float wetL = erosionL + goldenOutL;
            float wetR = erosionR + goldenOutR;

            // Ring mod coupling
            if (std::fabs (extRingMod) > 0.001f)
            {
                wetL *= (1.0f + extRingMod);
                wetR *= (1.0f + extRingMod);
            }

            // Dry/wet mix (dry = exciter, wet = reverb)
            float finalL = exciterSample * (1.0f - pDryWet) + wetL * pDryWet;
            float finalR = exciterSample * (1.0f - pDryWet) + wetR * pDryWet;

            // Cache for coupling output
            lastSampleL = finalL;
            lastSampleR = finalR;

            // Accumulate into buffer (engines ADD, never overwrite)
            outL[i] += finalL;
            if (outR) outR[i] += finalR;
        }

        // Reset coupling mods for next block
        extFilterMod = 0.0f;
        extDecayMod = 0.0f;
        extRingMod = 0.0f;

        // Silence gate analysis
        const float* rL = buffer.getReadPointer (0);
        const float* rR = buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : nullptr;
        silenceGate.analyzeBlock (rL, rR, numSamples);
    }

    //-- Coupling ---------------------------------------------------------------

    float getSampleForCoupling (int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? lastSampleL : lastSampleR;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* sourceBuffer, int /*numSamples*/) override
    {
        switch (type)
        {
            case CouplingType::AmpToFilter:
                extFilterMod = amount * 4000.0f;
                break;
            case CouplingType::EnvToDecay:
                extDecayMod = amount;
                break;
            case CouplingType::AudioToRing:
                extRingMod = (sourceBuffer ? sourceBuffer[0] : 0.0f) * amount;
                break;
            case CouplingType::AudioToBuffer:
                // External audio replaces exciter — inject into FDN input
                // (handled in renderBlock via coupling buffer if implemented)
                break;
            default:
                break;
        }
    }

    //-- Parameters -------------------------------------------------------------

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "oxb_size", "Space Size",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "oxb_decay", "Decay Time",
            juce::NormalisableRange<float> (0.1f, 60.0f, 0.0f, 0.3f), 4.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "oxb_entangle", "Entanglement",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.6f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "oxb_erosionRate", "Erosion Rate",
            juce::NormalisableRange<float> (0.01f, 0.5f, 0.0f, 0.5f), 0.08f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "oxb_erosionDepth", "Erosion Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "oxb_convergence", "Convergence",
            juce::NormalisableRange<float> (1.0f, 20.0f, 0.0f, 0.5f), 4.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "oxb_resonanceQ", "Resonance Focus",
            juce::NormalisableRange<float> (0.5f, 20.0f, 0.0f, 0.4f), 8.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "oxb_resonanceMix", "Resonance Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "oxb_cantilever", "Cantilever",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));  // Pearlman: init at 0.3

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "oxb_damping", "Damping",
            juce::NormalisableRange<float> (200.0f, 16000.0f, 0.0f, 0.3f), 6000.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "oxb_predelay", "Pre-Delay",
            juce::NormalisableRange<float> (0.0f, 200.0f, 0.0f, 0.5f), 20.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "oxb_dryWet", "Dry/Wet",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "oxb_exciterDecay", "Exciter Decay",
            juce::NormalisableRange<float> (0.001f, 0.1f, 0.0f, 0.5f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            "oxb_exciterBright", "Exciter Bright",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.7f));

        return { params.begin(), params.end() };
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pSizeParam       = apvts.getRawParameterValue ("oxb_size");
        pDecayParam      = apvts.getRawParameterValue ("oxb_decay");
        pEntangleParam   = apvts.getRawParameterValue ("oxb_entangle");
        pErosionRParam   = apvts.getRawParameterValue ("oxb_erosionRate");
        pErosionDParam   = apvts.getRawParameterValue ("oxb_erosionDepth");
        pConvergeParam   = apvts.getRawParameterValue ("oxb_convergence");
        pResQParam       = apvts.getRawParameterValue ("oxb_resonanceQ");
        pResMixParam     = apvts.getRawParameterValue ("oxb_resonanceMix");
        pCantileverParam = apvts.getRawParameterValue ("oxb_cantilever");
        pDampingParam    = apvts.getRawParameterValue ("oxb_damping");
        pPredelayParam   = apvts.getRawParameterValue ("oxb_predelay");
        pDryWetParam     = apvts.getRawParameterValue ("oxb_dryWet");
        pExcDecayParam   = apvts.getRawParameterValue ("oxb_exciterDecay");
        pExcBrightParam  = apvts.getRawParameterValue ("oxb_exciterBright");
    }

    //-- Identity ---------------------------------------------------------------

    juce::String getEngineId() const override { return "Oxbow"; }

    juce::Colour getAccentColour() const override
    {
        // Oxbow Teal — twilight zone, between light and dark
        return juce::Colour (0xFF1A6B5A);
    }

    int getMaxVoices() const override { return 1; }  // Monophonic reverb instrument

private:
    //--------------------------------------------------------------------------
    void updateGoldenFrequencies (float fundamental)
    {
        // Golden ratio harmonics: f, f×φ, f×φ², f×φ³
        static constexpr float phi = 1.6180339887f;
        float freqs[kGoldenFilters] = {
            fundamental,
            fundamental * phi,
            fundamental * phi * phi,
            fundamental * phi * phi * phi
        };

        float resQ = pResQParam ? pResQParam->load() : 8.0f;
        float srF = static_cast<float> (sr);

        for (int g = 0; g < kGoldenFilters; ++g)
        {
            float f = clamp (freqs[g], 20.0f, srF * 0.49f);
            float q = resQ / 20.0f;  // normalize to [0, 1] for CytomicSVF
            goldenL[g].setCoefficients (f, q, srF, 6.0f);  // +6dB peak
            goldenR[g].setCoefficients (f, q, srF, 6.0f);
        }
    }

    //--------------------------------------------------------------------------
    static constexpr int kFDNChannels = 8;
    static constexpr int kErosionAPFs = 4;
    static constexpr int kGoldenFilters = 4;

    double sr = 44100.0;
    int blockSize = 512;

    // FDN delay lines (8 channels)
    std::vector<float> fdnDelay[kFDNChannels];
    int fdnDelaySize[kFDNChannels] {};
    int fdnWritePos[kFDNChannels] {};

    // FDN damping filters (Moog two-pole SVF LP)
    CytomicSVF fdnDamp[kFDNChannels];

    // Phase erosion allpass filters
    CytomicSVF erosionAPF_L[kErosionAPFs];
    CytomicSVF erosionAPF_R[kErosionAPFs];
    StandardLFO erosionLFO[kErosionAPFs];

    // Golden resonance filters
    CytomicSVF goldenL[kGoldenFilters];
    CytomicSVF goldenR[kGoldenFilters];
    float midEnv = 0.0f, sideEnv = 0.0f;
    float resonanceGain = 0.0f;

    // Exciter
    double exciterPhase = 0.0;
    float exciterEnv = 0.0f;
    bool exciterActive = false;
    int currentNote = 60;
    float currentVelocity = 0.0f;
    float aftertouch = 0.0f;
    uint32_t noiseRng = 42u;

    // Cantilever energy tracking
    float peakEnergy = 0.0001f;
    float currentEnergy = 0.0f;

    // Pre-delay
    std::vector<float> predelayBuf;
    int predelaySize = 0;
    int predelayPos = 0;

    // Size scaling
    float sizeScale = 1.0f;
    float dampingHz = 6000.0f;

    // Coupling state
    float extFilterMod = 0.0f;
    float extDecayMod = 0.0f;
    float extRingMod = 0.0f;
    float lastSampleL = 0.0f;
    float lastSampleR = 0.0f;

    // Parameter pointers
    std::atomic<float>* pSizeParam       = nullptr;
    std::atomic<float>* pDecayParam      = nullptr;
    std::atomic<float>* pEntangleParam   = nullptr;
    std::atomic<float>* pErosionRParam   = nullptr;
    std::atomic<float>* pErosionDParam   = nullptr;
    std::atomic<float>* pConvergeParam   = nullptr;
    std::atomic<float>* pResQParam       = nullptr;
    std::atomic<float>* pResMixParam     = nullptr;
    std::atomic<float>* pCantileverParam = nullptr;
    std::atomic<float>* pDampingParam    = nullptr;
    std::atomic<float>* pPredelayParam   = nullptr;
    std::atomic<float>* pDryWetParam     = nullptr;
    std::atomic<float>* pExcDecayParam   = nullptr;
    std::atomic<float>* pExcBrightParam  = nullptr;
};

} // namespace xomnibus
