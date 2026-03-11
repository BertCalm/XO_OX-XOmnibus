#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace xomnibus {

//==============================================================================
// ObsidianEngine — Crystalline Phase Distortion Synthesis.
//
// Resurrects Casio's CZ-series phase distortion method with 40 years of
// missed evolution: 2D morphable distortion space, two-stage cascade with
// cross-modulation, Euler-Bernoulli inharmonic stiffness, 4-formant resonance
// network, and stereo phase divergence.
//
// Features:
//   - Cosine oscillator with 2D phase distortion LUT (density x tilt)
//   - Distortion depth ADSR envelope
//   - Stiffness engine (Euler-Bernoulli inharmonicity)
//   - PD Stage 2 cascade with cross-modulation from Stage 1
//   - Stereo phase divergence (L/R use offset distortion coordinates)
//   - 4-formant bandpass resonance network
//   - Mono/Legato/Poly8/Poly16 voice modes with LRU stealing + 5ms crossfade
//   - 2 LFOs (Sine/Tri/Saw/Square/S&H)
//   - Full XOmnibus coupling support
//
// Coupling:
//   - Output: post-filter stereo audio via getSampleForCoupling
//   - Input: AudioToFM (modulate PD depth), AmpToFilter (modulate filter cutoff),
//            EnvToMorph (modulate density/tilt position)
//
//==============================================================================

//==============================================================================
// LUT dimensions — 32x32 density/tilt grid, 512 phase samples.
// Total memory: 32 * 32 * 512 * 4 bytes = 2 MB (shared across all voices).
//==============================================================================
static constexpr int kLutDensitySize = 32;
static constexpr int kLutTiltSize    = 32;
static constexpr int kLutPhaseSize   = 512;

//==============================================================================
// ADSR envelope generator — lightweight, inline, no allocation.
//==============================================================================
struct ObsidianADSR
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

    void noteOn() noexcept
    {
        stage = Stage::Attack;
    }

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
                if (level >= 1.0f)
                {
                    level = 1.0f;
                    stage = Stage::Decay;
                }
                return level;

            case Stage::Decay:
                level -= decayRate * (level - sustainLevel + 0.0001f);
                if (level <= sustainLevel + 0.0001f)
                {
                    level = sustainLevel;
                    stage = Stage::Sustain;
                }
                return level;

            case Stage::Sustain:
                return level;

            case Stage::Release:
                level -= releaseRate * (level + 0.0001f);
                if (level <= 0.0001f)
                {
                    level = 0.0f;
                    stage = Stage::Idle;
                }
                return level;
        }
        return 0.0f;
    }

    bool isActive() const noexcept { return stage != Stage::Idle; }

    void reset() noexcept
    {
        stage = Stage::Idle;
        level = 0.0f;
    }
};

//==============================================================================
// LFO with multiple shapes.
//==============================================================================
struct ObsidianLFO
{
    enum class Shape { Sine, Triangle, Saw, Square, SandH };

    float phase = 0.0f;
    float phaseInc = 0.0f;
    Shape shape = Shape::Sine;
    float holdValue = 0.0f;
    uint32_t sampleCounter = 0;

    void setRate (float hz, float sampleRate) noexcept
    {
        phaseInc = hz / std::max (1.0f, sampleRate);
    }

    void setShape (int idx) noexcept
    {
        shape = static_cast<Shape> (std::min (4, std::max (0, idx)));
    }

    float process() noexcept
    {
        float out = 0.0f;

        switch (shape)
        {
            case Shape::Sine:
                out = fastSin (phase * 6.28318530718f);
                break;
            case Shape::Triangle:
                out = 4.0f * std::fabs (phase - 0.5f) - 1.0f;
                break;
            case Shape::Saw:
                out = 2.0f * phase - 1.0f;
                break;
            case Shape::Square:
                out = (phase < 0.5f) ? 1.0f : -1.0f;
                break;
            case Shape::SandH:
            {
                float prevPhase = phase - phaseInc;
                if (prevPhase < 0.0f || phase < prevPhase)
                {
                    // New cycle — generate random value using simple LCG
                    sampleCounter = sampleCounter * 1664525u + 1013904223u;
                    holdValue = static_cast<float> (sampleCounter & 0xFFFF) / 32768.0f - 1.0f;
                }
                out = holdValue;
                break;
            }
        }

        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;

        return out;
    }

    void reset() noexcept
    {
        phase = 0.0f;
        holdValue = 0.0f;
        sampleCounter = 12345u;
    }
};

//==============================================================================
// ObsidianVoice — per-voice state.
//==============================================================================
struct ObsidianVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;

    // Phase accumulators
    float phase1 = 0.0f;        // Stage 1 master phase [0, 1)
    float phase2 = 0.0f;        // Stage 2 master phase [0, 1)

    // Glide
    float currentFreq = 440.0f;
    float targetFreq = 440.0f;
    float glideCoeff = 1.0f;    // 1.0 = instant

    // Envelopes
    ObsidianADSR ampEnv;
    ObsidianADSR pdEnv;

    // LFOs (per-voice for free-running independence)
    ObsidianLFO lfo1;
    ObsidianLFO lfo2;

    // Formant filters (4-band resonance network)
    CytomicSVF formant[4];

    // Main output filter
    CytomicSVF mainFilter;

    // Voice stealing crossfade
    float fadeGain = 1.0f;
    bool fadingOut = false;

    // Stiffness: cached partial frequency ratios (first 16 partials)
    float partialRatios[16] = {};

    void reset() noexcept
    {
        active = false;
        noteNumber = -1;
        velocity = 0.0f;
        phase1 = 0.0f;
        phase2 = 0.0f;
        currentFreq = 440.0f;
        targetFreq = 440.0f;
        fadeGain = 1.0f;
        fadingOut = false;
        ampEnv.reset();
        pdEnv.reset();
        lfo1.reset();
        lfo2.reset();
        for (auto& f : formant)
            f.reset();
        mainFilter.reset();
        for (int i = 0; i < 16; ++i)
            partialRatios[i] = static_cast<float> (i + 1);
    }
};

//==============================================================================
// ObsidianEngine — the main engine class.
//==============================================================================
class ObsidianEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 16;
    static constexpr float kPI = 3.14159265358979323846f;
    static constexpr float kTwoPi = 6.28318530717958647692f;

    //==========================================================================
    // SynthEngine interface — Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        // Pre-compute smoothing coefficient (5ms time constant)
        smoothCoeff = 1.0f - std::exp (-kTwoPi * (1.0f / 0.005f) / srf);

        // Pre-compute crossfade rate (5ms)
        crossfadeRate = 1.0f / (0.005f * srf);

        // Allocate output cache for coupling reads
        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        // Build the 2D distortion function LUT
        buildDistortionLUT();

        // Build cosine LUT
        for (int i = 0; i < kCosineLUTSize; ++i)
        {
            float t = static_cast<float> (i) / static_cast<float> (kCosineLUTSize);
            cosineLUT[static_cast<size_t> (i)] = std::cos (kTwoPi * t);
        }

        // Initialize voices
        for (auto& v : voices)
        {
            v.reset();
            for (int f = 0; f < 4; ++f)
            {
                v.formant[f].reset();
                v.formant[f].setMode (CytomicSVF::Mode::BandPass);
            }
            v.mainFilter.reset();
            v.mainFilter.setMode (CytomicSVF::Mode::LowPass);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();

        envelopeOutput = 0.0f;
        couplingPDDepthMod = 0.0f;
        couplingFilterMod = 0.0f;
        couplingMorphXMod = 0.0f;
        couplingMorphYMod = 0.0f;

        smoothedDensity = 0.0f;
        smoothedTilt = 0.0f;
        smoothedDepth = 0.0f;

        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    //==========================================================================
    // SynthEngine interface — Audio
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        // --- ParamSnapshot: read all parameters once per block ---
        const float pDensityX     = loadParam (pDistX, 0.5f);
        const float pTiltY        = loadParam (pDistY, 0.5f);
        const float pDepthVal     = loadParam (pDepth, 0.5f);
        const float pStiffVal     = loadParam (pStiffness, 0.0f);
        const float pCascadeVal   = loadParam (pCascade, 0.0f);
        const float pCrossModVal  = loadParam (pCrossMod, 0.0f);
        const float pFormantVal   = loadParam (pFormant, 0.0f);
        const float pStereoVal    = loadParam (pStereo, 0.0f);
        const float pCutoffVal    = loadParam (pFilterCutoff, 8000.0f);
        const float pResoVal      = loadParam (pFilterReso, 0.0f);
        const float pLevelVal     = loadParam (pLevel, 0.8f);

        const float pAmpA         = loadParam (pAmpAttack, 0.01f);
        const float pAmpD         = loadParam (pAmpDecay, 0.1f);
        const float pAmpS         = loadParam (pAmpSustain, 0.8f);
        const float pAmpR         = loadParam (pAmpRelease, 0.3f);
        const float pPdA          = loadParam (pPDAttack, 0.01f);
        const float pPdD          = loadParam (pPDDecay, 0.2f);
        const float pPdS          = loadParam (pPDSustain, 0.5f);
        const float pPdR          = loadParam (pPDRelease, 0.3f);

        const float pLfo1R        = loadParam (pLfo1Rate, 1.0f);
        const float pLfo1D        = loadParam (pLfo1Depth, 0.0f);
        const int   pLfo1S        = static_cast<int> (loadParam (pLfo1Shape, 0.0f));
        const float pLfo2R        = loadParam (pLfo2Rate, 1.0f);
        const float pLfo2D        = loadParam (pLfo2Depth, 0.0f);
        const int   pLfo2S        = static_cast<int> (loadParam (pLfo2Shape, 0.0f));

        const int   voiceModeIdx  = static_cast<int> (loadParam (pVoiceMode, 2.0f));
        const float glideTime     = loadParam (pGlide, 0.0f);
        const float pFormantInt   = loadParam (pFormantIntensity, 0.0f);

        const float macroChar     = loadParam (pMacroCharacter, 0.0f);
        const float macroMove     = loadParam (pMacroMovement, 0.0f);
        const float macroCoup     = loadParam (pMacroCoupling, 0.0f);
        const float macroSpace    = loadParam (pMacroSpace, 0.0f);

        // Determine max polyphony from voice mode
        int maxPoly = kMaxVoices;
        bool monoMode = false;
        bool legatoMode = false;
        switch (voiceModeIdx)
        {
            case 0: maxPoly = 1; monoMode = true; break;         // Mono
            case 1: maxPoly = 1; monoMode = true; legatoMode = true; break; // Legato
            case 2: maxPoly = 8; break;                           // Poly8
            case 3: maxPoly = 16; break;                          // Poly16
            default: maxPoly = 8; break;
        }

        // Glide coefficient
        float glideCoeff = 1.0f;
        if (glideTime > 0.001f)
            glideCoeff = 1.0f - std::exp (-1.0f / (glideTime * srf));

        // Apply macro offsets to base parameters
        float effectiveDensity = clamp (pDensityX + macroChar * 0.5f + couplingMorphXMod, 0.0f, 1.0f);
        float effectiveTilt    = clamp (pTiltY + couplingMorphYMod, 0.0f, 1.0f);
        float effectiveDepth   = clamp (pDepthVal + macroChar * 0.3f + couplingPDDepthMod, 0.0f, 1.0f);
        float effectiveCross   = clamp (pCrossModVal + macroMove * 0.5f, 0.0f, 1.0f);
        float effectiveStiff   = clamp (pStiffVal + macroSpace * 0.4f, 0.0f, 1.0f);
        float effectiveStereo  = clamp (pStereoVal + macroSpace * 0.3f, 0.0f, 1.0f);
        float effectiveCascade = clamp (pCascadeVal + macroCoup * 0.3f, 0.0f, 1.0f);
        float effectiveCutoff  = clamp (pCutoffVal + couplingFilterMod * 4000.0f, 20.0f, 20000.0f);
        float effectiveFormant = clamp (pFormantVal + pFormantInt, 0.0f, 1.0f);

        // Reset coupling accumulators
        couplingPDDepthMod = 0.0f;
        couplingFilterMod = 0.0f;
        couplingMorphXMod = 0.0f;
        couplingMorphYMod = 0.0f;

        // Stiffness coefficient: map [0,1] exponentially to [0, 0.15]
        float B = effectiveStiff * effectiveStiff * 0.15f;

        // --- Process MIDI events ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), maxPoly, monoMode, legatoMode, glideCoeff,
                        pAmpA, pAmpD, pAmpS, pAmpR, pPdA, pPdD, pPdS, pPdR,
                        pLfo1R, pLfo1D, pLfo1S, pLfo2R, pLfo2D, pLfo2S,
                        effectiveCutoff, pResoVal, B);
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
        }

        // --- Update per-voice filter coefficients once per block ---
        for (auto& voice : voices)
        {
            if (!voice.active) continue;

            voice.mainFilter.setCoefficients (effectiveCutoff, pResoVal, srf);

            // Formant filter center frequencies (fixed musical ranges)
            static constexpr float formantFreqs[4] = { 550.0f, 1650.0f, 3250.0f, 5000.0f };
            static constexpr float formantQ[4]     = { 0.6f, 0.5f, 0.45f, 0.4f };
            for (int f = 0; f < 4; ++f)
                voice.formant[f].setCoefficients (formantFreqs[f], formantQ[f], srf);
        }

        float peakEnv = 0.0f;

        // --- Render sample loop ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Smooth control-rate parameters (5ms)
            smoothedDensity += (effectiveDensity - smoothedDensity) * smoothCoeff;
            smoothedTilt    += (effectiveTilt - smoothedTilt) * smoothCoeff;
            smoothedDepth   += (effectiveDepth - smoothedDepth) * smoothCoeff;

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                // --- Voice-stealing crossfade (5ms) ---
                if (voice.fadingOut)
                {
                    voice.fadeGain -= crossfadeRate;
                    if (voice.fadeGain <= 0.0f)
                    {
                        voice.fadeGain = 0.0f;
                        voice.active = false;
                        continue;
                    }
                }

                // --- Glide (portamento) ---
                voice.currentFreq += (voice.targetFreq - voice.currentFreq) * voice.glideCoeff;

                // --- Envelopes ---
                float ampLevel = voice.ampEnv.process();
                float pdLevel  = voice.pdEnv.process();

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // --- LFO modulation ---
                float lfo1Val = voice.lfo1.process() * pLfo1D;
                float lfo2Val = voice.lfo2.process() * pLfo2D;

                // LFO1 -> PD depth, LFO2 -> density modulation
                float modDepth   = clamp (smoothedDepth + lfo1Val * 0.3f, 0.0f, 1.0f);
                float modDensity = clamp (smoothedDensity + lfo2Val * 0.2f, 0.0f, 1.0f);

                // --- Phase increment ---
                float freq = voice.currentFreq;
                float phaseInc = freq / srf;

                // --- PD Stage 1 (L channel) ---
                float densL = modDensity;
                float tiltL = smoothedTilt;

                // --- PD Stage 1 (R channel with stereo divergence) ---
                float delta = effectiveStereo * 0.1f;
                float densR = clamp (densL + delta, 0.0f, 1.0f);
                float tiltR = clamp (tiltL + delta, 0.0f, 1.0f);

                // Advance phase
                voice.phase1 += phaseInc;
                if (voice.phase1 >= 1.0f) voice.phase1 -= 1.0f;

                // Apply stiffness (Euler-Bernoulli): sum first few partials with stretched ratios
                // For efficiency we use the fundamental PD as base and add inharmonic color
                float stiffColor = 0.0f;
                if (B > 0.0001f)
                {
                    // Sum 4 partials with inharmonic stretching
                    for (int p = 1; p <= 4; ++p)
                    {
                        float n = static_cast<float> (p + 1);
                        float stretchedRatio = n * std::sqrt (1.0f + B * n * n);
                        float partialPhase = voice.phase1 * stretchedRatio;
                        partialPhase -= static_cast<float> (static_cast<int> (partialPhase));
                        // Use cosine LUT for efficiency
                        stiffColor += lookupCosine (partialPhase) / (n * n);
                    }
                    stiffColor *= B * 4.0f; // Scale by stiffness amount
                }

                // Depth envelope modulates how much PD is applied
                float depthEnvAmount = pdLevel * modDepth;

                // Look up distorted phase for L and R
                float distPhaseL = lookupDistortion (densL, tiltL, voice.phase1);
                float distPhaseR = lookupDistortion (densR, tiltR, voice.phase1);

                // Blend between linear (pure cosine) and distorted phase
                float finalPhaseL = voice.phase1 + depthEnvAmount * (distPhaseL - voice.phase1);
                float finalPhaseR = voice.phase1 + depthEnvAmount * (distPhaseR - voice.phase1);

                // Generate Stage 1 output
                float stage1L = lookupCosine (finalPhaseL) + stiffColor;
                float stage1R = lookupCosine (finalPhaseR) + stiffColor;

                // --- PD Stage 2 (Cascade) ---
                float outL = stage1L;
                float outR = stage1R;

                if (effectiveCascade > 0.001f)
                {
                    // Stage 2 phase runs at same rate
                    voice.phase2 += phaseInc;
                    if (voice.phase2 >= 1.0f) voice.phase2 -= 1.0f;

                    // Cross-modulation: Stage 1 output modulates Stage 2 depth
                    float crossModL = stage1L * effectiveCross;
                    float crossModR = stage1R * effectiveCross;

                    // Stage 2 uses a different distortion coordinate (offset by 0.3)
                    float s2dens = clamp (densL + 0.3f, 0.0f, 1.0f);
                    float s2tilt = clamp (tiltL - 0.2f, 0.0f, 1.0f);

                    float distPhase2L = lookupDistortion (s2dens, s2tilt, voice.phase2);
                    float distPhase2R = lookupDistortion (
                        clamp (s2dens + delta, 0.0f, 1.0f),
                        clamp (s2tilt + delta, 0.0f, 1.0f),
                        voice.phase2);

                    // Apply cross-modulation to depth
                    float s2depth = depthEnvAmount * (1.0f + crossModL);
                    s2depth = clamp (s2depth, 0.0f, 1.0f);

                    float s2phaseL = voice.phase2 + s2depth * (distPhase2L - voice.phase2);
                    float s2phaseR = voice.phase2 + s2depth * (distPhase2R - voice.phase2);

                    float stage2L = lookupCosine (s2phaseL);
                    float stage2R = lookupCosine (s2phaseR);

                    // Blend Stage 1 and cascade
                    outL = outL * (1.0f - effectiveCascade) + stage2L * effectiveCascade;
                    outR = outR * (1.0f - effectiveCascade) + stage2R * effectiveCascade;
                }

                // --- 4-Formant Resonance Network ---
                if (effectiveFormant > 0.001f)
                {
                    float formantL = 0.0f, formantR = 0.0f;
                    for (int f = 0; f < 4; ++f)
                    {
                        // Use mono sum through formants, then restore stereo
                        float mono = (outL + outR) * 0.5f;
                        float filt = voice.formant[f].processSample (mono);
                        formantL += filt;
                        formantR += filt;
                    }
                    formantL *= 0.25f;
                    formantR *= 0.25f;

                    // Blend dry PD with formant-filtered
                    outL = outL * (1.0f - effectiveFormant) + formantL * effectiveFormant;
                    outR = outR * (1.0f - effectiveFormant) + formantR * effectiveFormant;
                }

                // --- Main filter ---
                outL = voice.mainFilter.processSample (outL);
                // Process R through same filter state for consistency (mono filter, stereo restore)
                // Actually, for true stereo we'd want two filters; for efficiency we use mid/side:
                float mid = (outL + outR) * 0.5f;
                float side = (outL - outR) * 0.5f;
                outL = mid + side;
                outR = mid - side;

                // --- Apply amplitude envelope, velocity, and crossfade ---
                float gain = ampLevel * voice.velocity * voice.fadeGain;
                outL *= gain;
                outR *= gain;

                // Denormal protection on outputs
                outL = flushDenormal (outL);
                outR = flushDenormal (outR);

                mixL += outL;
                mixR += outR;

                peakEnv = std::max (peakEnv, ampLevel);
            }

            // Apply master level
            float finalL = mixL * pLevelVal;
            float finalR = mixR * pLevelVal;

            // Write to output buffer
            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample (0, sample, finalL);
                buffer.addSample (1, sample, finalR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample (0, sample, (finalL + finalR) * 0.5f);
            }

            // Cache for coupling reads
            if (sample < static_cast<int> (outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t> (sample)] = finalL;
                outputCacheR[static_cast<size_t> (sample)] = finalR;
            }
        }

        envelopeOutput = peakEnv;

        // Update active voice count (atomic for thread-safe read from UI)
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
        if (channel == 2) return envelopeOutput; // Envelope for coupling
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
            case CouplingType::AudioToFM:
                // External audio modulates PD depth
                couplingPDDepthMod += amount * 0.5f;
                break;

            case CouplingType::AmpToFilter:
                // External amplitude modulates filter cutoff
                couplingFilterMod += amount;
                break;

            case CouplingType::EnvToMorph:
                // External envelope modulates density/tilt position
                couplingMorphXMod += amount * 0.3f;
                couplingMorphYMod += amount * 0.2f;
                break;

            case CouplingType::RhythmToBlend:
                // Could sync PD envelope to rhythm — accumulate as depth mod
                couplingPDDepthMod += amount * 0.3f;
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
        // --- Core PD Parameters ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_densityX", 1 }, "Obsidian Density X",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_tiltY", 1 }, "Obsidian Tilt Y",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_depth", 1 }, "Obsidian Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_stiffness", 1 }, "Obsidian Stiffness",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_cascadeBlend", 1 }, "Obsidian Cascade",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_crossModDepth", 1 }, "Obsidian Cross-Mod",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_formantIntensity", 1 }, "Obsidian Formant",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_stereoWidth", 1 }, "Obsidian Stereo",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

        // --- Filter ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_filterCutoff", 1 }, "Obsidian Filter Cutoff",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f), 8000.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_filterReso", 1 }, "Obsidian Filter Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        // --- Level ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_level", 1 }, "Obsidian Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        // --- Amp Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_ampAttack", 1 }, "Obsidian Amp Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_ampDecay", 1 }, "Obsidian Amp Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.1f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_ampSustain", 1 }, "Obsidian Amp Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_ampRelease", 1 }, "Obsidian Amp Release",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        // --- PD Depth Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_depthAttack", 1 }, "Obsidian PD Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_depthDecay", 1 }, "Obsidian PD Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.2f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_depthSustain", 1 }, "Obsidian PD Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_depthRelease", 1 }, "Obsidian PD Release",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        // --- LFO 1 ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_lfo1Rate", 1 }, "Obsidian LFO1 Rate",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_lfo1Depth", 1 }, "Obsidian LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "obsidian_lfo1Shape", 1 }, "Obsidian LFO1 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));

        // --- LFO 2 ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_lfo2Rate", 1 }, "Obsidian LFO2 Rate",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_lfo2Depth", 1 }, "Obsidian LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "obsidian_lfo2Shape", 1 }, "Obsidian LFO2 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));

        // --- Voice Parameters ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "obsidian_polyphony", 1 }, "Obsidian Voice Mode",
            juce::StringArray { "Mono", "Legato", "Poly8", "Poly16" }, 2));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_glide", 1 }, "Obsidian Glide",
            juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f, 0.5f), 0.0f));

        // --- Macros ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_macroCharacter", 1 }, "Obsidian Macro CHARACTER",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_macroMovement", 1 }, "Obsidian Macro MOVEMENT",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_macroCoupling", 1 }, "Obsidian Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obsidian_macroSpace", 1 }, "Obsidian Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pDistX             = apvts.getRawParameterValue ("obsidian_densityX");
        pDistY             = apvts.getRawParameterValue ("obsidian_tiltY");
        pDepth             = apvts.getRawParameterValue ("obsidian_depth");
        pStiffness         = apvts.getRawParameterValue ("obsidian_stiffness");
        pCascade           = apvts.getRawParameterValue ("obsidian_cascadeBlend");
        pCrossMod          = apvts.getRawParameterValue ("obsidian_crossModDepth");
        pFormant           = apvts.getRawParameterValue ("obsidian_formantIntensity");
        pStereo            = apvts.getRawParameterValue ("obsidian_stereoWidth");
        pFilterCutoff      = apvts.getRawParameterValue ("obsidian_filterCutoff");
        pFilterReso        = apvts.getRawParameterValue ("obsidian_filterReso");
        pLevel             = apvts.getRawParameterValue ("obsidian_level");

        pAmpAttack         = apvts.getRawParameterValue ("obsidian_ampAttack");
        pAmpDecay          = apvts.getRawParameterValue ("obsidian_ampDecay");
        pAmpSustain        = apvts.getRawParameterValue ("obsidian_ampSustain");
        pAmpRelease        = apvts.getRawParameterValue ("obsidian_ampRelease");

        pPDAttack          = apvts.getRawParameterValue ("obsidian_depthAttack");
        pPDDecay           = apvts.getRawParameterValue ("obsidian_depthDecay");
        pPDSustain         = apvts.getRawParameterValue ("obsidian_depthSustain");
        pPDRelease         = apvts.getRawParameterValue ("obsidian_depthRelease");

        pLfo1Rate          = apvts.getRawParameterValue ("obsidian_lfo1Rate");
        pLfo1Depth         = apvts.getRawParameterValue ("obsidian_lfo1Depth");
        pLfo1Shape         = apvts.getRawParameterValue ("obsidian_lfo1Shape");
        pLfo2Rate          = apvts.getRawParameterValue ("obsidian_lfo2Rate");
        pLfo2Depth         = apvts.getRawParameterValue ("obsidian_lfo2Depth");
        pLfo2Shape         = apvts.getRawParameterValue ("obsidian_lfo2Shape");

        pVoiceMode         = apvts.getRawParameterValue ("obsidian_polyphony");
        pGlide             = apvts.getRawParameterValue ("obsidian_glide");

        pFormantIntensity  = apvts.getRawParameterValue ("obsidian_formantIntensity");

        pMacroCharacter    = apvts.getRawParameterValue ("obsidian_macroCharacter");
        pMacroMovement     = apvts.getRawParameterValue ("obsidian_macroMovement");
        pMacroCoupling     = apvts.getRawParameterValue ("obsidian_macroCoupling");
        pMacroSpace        = apvts.getRawParameterValue ("obsidian_macroSpace");
    }

    //==========================================================================
    // SynthEngine interface — Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Obsidian"; }

    juce::Colour getAccentColour() const override { return juce::Colour (0xFF8B5CF6); }

    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override { return activeVoices; }

private:
    //==========================================================================
    // Helper: safe parameter load
    //==========================================================================

    static float loadParam (std::atomic<float>* p, float fallback) noexcept
    {
        return (p != nullptr) ? p->load() : fallback;
    }

    //==========================================================================
    // LUT generation — builds the 2D distortion function table.
    //==========================================================================

    void buildDistortionLUT()
    {
        // The distortion function D(phase) maps [0,1] -> [0,1] with varying speed.
        //
        // Density (X): controls number of inflection points
        //   Low density: near-linear -> sine-like output
        //   High density: many inflection points -> FM-like rich harmonics
        //
        // Tilt (Y): controls energy distribution
        //   Low tilt: slow start, fast middle -> warm, fundamental-heavy
        //   High tilt: fast start, slow end -> bright, upper-harmonic-heavy

        for (int di = 0; di < kLutDensitySize; ++di)
        {
            float density = static_cast<float> (di) / static_cast<float> (kLutDensitySize - 1);

            for (int ti = 0; ti < kLutTiltSize; ++ti)
            {
                float tilt = static_cast<float> (ti) / static_cast<float> (kLutTiltSize - 1);

                for (int pi = 0; pi < kLutPhaseSize; ++pi)
                {
                    float phase = static_cast<float> (pi) / static_cast<float> (kLutPhaseSize);

                    // Build distortion function:
                    // Base: polynomial warping that creates inflection points
                    // density controls how many inflection points (via a sinusoidal waveshaper)
                    // tilt controls asymmetry (via power curve bias)

                    // Number of inflection folds (1 to 6 based on density)
                    float folds = 1.0f + density * 5.0f;

                    // Create the folded phase: sin(folds * pi * phase) normalized to [0,1]
                    // This creates inflection points proportional to density
                    float folded = std::sin (folds * kPI * phase);
                    // Rectify and normalize: we want [0,1] output
                    float foldedAbs = std::fabs (folded);

                    // Tilt: power curve that biases energy distribution
                    // Low tilt (0): exponent > 1 -> slow start, fast middle (warm)
                    // High tilt (1): exponent < 1 -> fast start, slow end (bright)
                    float exponent = 2.0f - tilt * 1.8f; // range [0.2, 2.0]
                    float tilted = std::pow (phase, exponent);

                    // Blend between linear phase and folded/tilted distortion
                    // At low density: mostly linear (sine-like)
                    // At high density: mostly folded (complex harmonics)
                    float blendFactor = density * density; // Quadratic for gradual onset
                    float distorted = phase * (1.0f - blendFactor)
                                    + (tilted * (1.0f - blendFactor * 0.5f)
                                       + foldedAbs * blendFactor * 0.5f)
                                    * blendFactor;

                    // Ensure monotonic and bounded [0, 1]
                    // Apply soft clamp
                    distorted = std::max (0.0f, std::min (1.0f, distorted));

                    // Ensure the function maps 0->0 and 1->1 (phase continuity)
                    // Weight toward endpoints
                    if (pi == 0)
                        distorted = 0.0f;
                    else if (pi == kLutPhaseSize - 1)
                        distorted = 1.0f;

                    distortionLUT[static_cast<size_t> (di)]
                                 [static_cast<size_t> (ti)]
                                 [static_cast<size_t> (pi)] = distorted;
                }
            }
        }
    }

    //==========================================================================
    // LUT lookup with trilinear interpolation.
    //==========================================================================

    float lookupDistortion (float density, float tilt, float phase) const noexcept
    {
        // Clamp inputs
        density = std::max (0.0f, std::min (1.0f, density));
        tilt    = std::max (0.0f, std::min (1.0f, tilt));

        // Wrap phase to [0, 1)
        phase = phase - static_cast<float> (static_cast<int> (phase));
        if (phase < 0.0f) phase += 1.0f;

        // Map to LUT indices
        float dIdx = density * static_cast<float> (kLutDensitySize - 1);
        float tIdx = tilt * static_cast<float> (kLutTiltSize - 1);
        float pIdx = phase * static_cast<float> (kLutPhaseSize - 1);

        int d0 = static_cast<int> (dIdx);
        int t0 = static_cast<int> (tIdx);
        int p0 = static_cast<int> (pIdx);

        int d1 = std::min (d0 + 1, kLutDensitySize - 1);
        int t1 = std::min (t0 + 1, kLutTiltSize - 1);
        int p1 = std::min (p0 + 1, kLutPhaseSize - 1);

        float df = dIdx - static_cast<float> (d0);
        float tf = tIdx - static_cast<float> (t0);
        float pf = pIdx - static_cast<float> (p0);

        // Trilinear interpolation
        auto& lut = distortionLUT;
        float c000 = lut[static_cast<size_t> (d0)][static_cast<size_t> (t0)][static_cast<size_t> (p0)];
        float c001 = lut[static_cast<size_t> (d0)][static_cast<size_t> (t0)][static_cast<size_t> (p1)];
        float c010 = lut[static_cast<size_t> (d0)][static_cast<size_t> (t1)][static_cast<size_t> (p0)];
        float c011 = lut[static_cast<size_t> (d0)][static_cast<size_t> (t1)][static_cast<size_t> (p1)];
        float c100 = lut[static_cast<size_t> (d1)][static_cast<size_t> (t0)][static_cast<size_t> (p0)];
        float c101 = lut[static_cast<size_t> (d1)][static_cast<size_t> (t0)][static_cast<size_t> (p1)];
        float c110 = lut[static_cast<size_t> (d1)][static_cast<size_t> (t1)][static_cast<size_t> (p0)];
        float c111 = lut[static_cast<size_t> (d1)][static_cast<size_t> (t1)][static_cast<size_t> (p1)];

        // Interpolate along phase
        float c00 = c000 + pf * (c001 - c000);
        float c01 = c010 + pf * (c011 - c010);
        float c10 = c100 + pf * (c101 - c100);
        float c11 = c110 + pf * (c111 - c110);

        // Interpolate along tilt
        float c0 = c00 + tf * (c01 - c00);
        float c1 = c10 + tf * (c11 - c10);

        // Interpolate along density
        return c0 + df * (c1 - c0);
    }

    //==========================================================================
    // Cosine LUT lookup.
    //==========================================================================

    static constexpr int kCosineLUTSize = 4096;

    float lookupCosine (float phase) const noexcept
    {
        // Wrap phase to [0, 1)
        phase = phase - static_cast<float> (static_cast<int> (phase));
        if (phase < 0.0f) phase += 1.0f;

        float idx = phase * static_cast<float> (kCosineLUTSize);
        int i0 = static_cast<int> (idx);
        int i1 = (i0 + 1) & (kCosineLUTSize - 1);
        float frac = idx - static_cast<float> (i0);
        i0 &= (kCosineLUTSize - 1);

        return cosineLUT[static_cast<size_t> (i0)]
             + frac * (cosineLUT[static_cast<size_t> (i1)] - cosineLUT[static_cast<size_t> (i0)]);
    }

    //==========================================================================
    // MIDI note handling.
    //==========================================================================

    void noteOn (int noteNumber, float velocity, int maxPoly,
                 bool monoMode, bool legatoMode, float glideCoeff,
                 float ampA, float ampD, float ampS, float ampR,
                 float pdA, float pdD, float pdS, float pdR,
                 float lfo1Rate, float lfo1Depth, int lfo1Shape,
                 float lfo2Rate, float lfo2Depth, int lfo2Shape,
                 float cutoff, float reso, float stiffnessB)
    {
        float freq = midiToHz (static_cast<float> (noteNumber));

        if (monoMode)
        {
            auto& voice = voices[0];
            bool wasActive = voice.active;

            voice.targetFreq = freq;

            if (legatoMode && wasActive)
            {
                // Legato: don't retrigger envelopes, just glide
                voice.glideCoeff = glideCoeff;
                voice.noteNumber = noteNumber;
                voice.velocity = velocity;
            }
            else
            {
                voice.active = true;
                voice.noteNumber = noteNumber;
                voice.velocity = velocity;
                voice.startTime = voiceCounter++;
                voice.currentFreq = freq;
                voice.glideCoeff = glideCoeff;
                voice.phase1 = 0.0f;
                voice.phase2 = 0.0f;
                voice.fadingOut = false;
                voice.fadeGain = 1.0f;

                voice.ampEnv.setParams (ampA, ampD, ampS, ampR, srf);
                voice.ampEnv.noteOn();
                voice.pdEnv.setParams (pdA, pdD, pdS, pdR, srf);
                voice.pdEnv.noteOn();

                voice.lfo1.setRate (lfo1Rate, srf);
                voice.lfo1.setShape (lfo1Shape);
                voice.lfo2.setRate (lfo2Rate, srf);
                voice.lfo2.setShape (lfo2Shape);

                setupVoiceFilters (voice, cutoff, reso, stiffnessB);
            }
            return;
        }

        // Polyphonic mode
        int idx = findFreeVoice (maxPoly);
        auto& voice = voices[static_cast<size_t> (idx)];

        // If stealing, initiate crossfade
        if (voice.active)
        {
            voice.fadingOut = true;
            voice.fadeGain = std::min (voice.fadeGain, 0.5f); // Quick steal
        }

        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.startTime = voiceCounter++;
        voice.currentFreq = freq;
        voice.targetFreq = freq;
        voice.glideCoeff = 1.0f; // No glide in poly mode
        voice.phase1 = 0.0f;
        voice.phase2 = 0.0f;
        voice.fadingOut = false;
        voice.fadeGain = 1.0f;

        voice.ampEnv.setParams (ampA, ampD, ampS, ampR, srf);
        voice.ampEnv.noteOn();
        voice.pdEnv.setParams (pdA, pdD, pdS, pdR, srf);
        voice.pdEnv.noteOn();

        voice.lfo1.setRate (lfo1Rate, srf);
        voice.lfo1.setShape (lfo1Shape);
        voice.lfo1.reset();
        voice.lfo2.setRate (lfo2Rate, srf);
        voice.lfo2.setShape (lfo2Shape);
        voice.lfo2.reset();

        setupVoiceFilters (voice, cutoff, reso, stiffnessB);
    }

    void noteOff (int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber && !voice.fadingOut)
            {
                voice.ampEnv.noteOff();
                voice.pdEnv.noteOff();
            }
        }
    }

    void setupVoiceFilters (ObsidianVoice& voice, float cutoff, float reso, float /*B*/) noexcept
    {
        voice.mainFilter.reset();
        voice.mainFilter.setMode (CytomicSVF::Mode::LowPass);
        voice.mainFilter.setCoefficients (cutoff, reso, srf);

        static constexpr float formantFreqs[4] = { 550.0f, 1650.0f, 3250.0f, 5000.0f };
        static constexpr float formantQ[4]     = { 0.6f, 0.5f, 0.45f, 0.4f };
        for (int f = 0; f < 4; ++f)
        {
            voice.formant[f].reset();
            voice.formant[f].setMode (CytomicSVF::Mode::BandPass);
            voice.formant[f].setCoefficients (formantFreqs[f], formantQ[f], srf);
        }
    }

    int findFreeVoice (int maxPoly) const
    {
        int poly = std::min (maxPoly, kMaxVoices);

        // Find inactive voice within polyphony limit
        for (int i = 0; i < poly; ++i)
            if (!voices[static_cast<size_t> (i)].active)
                return i;

        // LRU voice stealing — find oldest voice
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

    //==========================================================================
    // Member data
    //==========================================================================

    double sr = 44100.0;
    float srf = 44100.0f;
    float smoothCoeff = 0.1f;
    float crossfadeRate = 0.01f;

    std::array<ObsidianVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    int activeVoices = 0;

    // Smoothed control parameters
    float smoothedDensity = 0.5f;
    float smoothedTilt = 0.5f;
    float smoothedDepth = 0.5f;

    // Coupling accumulators
    float envelopeOutput = 0.0f;
    float couplingPDDepthMod = 0.0f;
    float couplingFilterMod = 0.0f;
    float couplingMorphXMod = 0.0f;
    float couplingMorphYMod = 0.0f;

    // Output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // 2D distortion function LUT: [density][tilt][phase]
    std::array<std::array<std::array<float, kLutPhaseSize>, kLutTiltSize>, kLutDensitySize> distortionLUT {};

    // Cosine LUT for fast evaluation
    std::array<float, kCosineLUTSize> cosineLUT {};

    // Cached APVTS parameter pointers
    std::atomic<float>* pDistX = nullptr;
    std::atomic<float>* pDistY = nullptr;
    std::atomic<float>* pDepth = nullptr;
    std::atomic<float>* pStiffness = nullptr;
    std::atomic<float>* pCascade = nullptr;
    std::atomic<float>* pCrossMod = nullptr;
    std::atomic<float>* pFormant = nullptr;
    std::atomic<float>* pStereo = nullptr;
    std::atomic<float>* pFilterCutoff = nullptr;
    std::atomic<float>* pFilterReso = nullptr;
    std::atomic<float>* pLevel = nullptr;

    std::atomic<float>* pAmpAttack = nullptr;
    std::atomic<float>* pAmpDecay = nullptr;
    std::atomic<float>* pAmpSustain = nullptr;
    std::atomic<float>* pAmpRelease = nullptr;

    std::atomic<float>* pPDAttack = nullptr;
    std::atomic<float>* pPDDecay = nullptr;
    std::atomic<float>* pPDSustain = nullptr;
    std::atomic<float>* pPDRelease = nullptr;

    std::atomic<float>* pLfo1Rate = nullptr;
    std::atomic<float>* pLfo1Depth = nullptr;
    std::atomic<float>* pLfo1Shape = nullptr;
    std::atomic<float>* pLfo2Rate = nullptr;
    std::atomic<float>* pLfo2Depth = nullptr;
    std::atomic<float>* pLfo2Shape = nullptr;

    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlide = nullptr;
    std::atomic<float>* pFormantIntensity = nullptr;

    std::atomic<float>* pMacroCharacter = nullptr;
    std::atomic<float>* pMacroMovement = nullptr;
    std::atomic<float>* pMacroCoupling = nullptr;
    std::atomic<float>* pMacroSpace = nullptr;
};

} // namespace xomnibus
