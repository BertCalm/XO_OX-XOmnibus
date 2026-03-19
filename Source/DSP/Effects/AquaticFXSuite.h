#pragma once
#include <cmath>
#include <array>
#include <algorithm>
#include <cstring>
#include "../FastMath.h"
#include "../LCG.h"
#include "../FDNReverb.h"

namespace xomnibus {

//==============================================================================
// AquaticFXSuite — The Aquarium. Six master effects themed around water
// phenomena that give every XOmnibus engine the brand-defining "underwater"
// character. Placed in the signal path BEFORE MasterFXChain.
//
// Signal flow (fixed order):
//   Engine Out → Fathom → Drift → Tide → Reef → Surface → Biolume → MasterFX
//
// Three conceptual pairs:
//   Depth:  Fathom (sinks)  ↔  Surface (emerges)
//   Motion: Drift (random)  ↔  Tide (cyclical)
//   Color:  Reef (space)    ↔  Biolume (light)
//
// 22 parameters, all prefixed `aqua_`. Each stage zero-CPU when mix = 0.
//
// Macro responsiveness:
//   CHARACTER → Fathom depth + Biolume glow
//   SPACE     → Reef size + Drift width
//   MOVEMENT  → Tide rate + Drift rate
//   COUPLING  → Surface level
//==============================================================================
class AquaticFXSuite
{
public:
    AquaticFXSuite() = default;

    //--------------------------------------------------------------------------
    void prepare (double sampleRate)
    {
        sr = static_cast<float> (sampleRate);

        // --- Fathom cached coefficients (constant — computed once) ---
        fathomLPCoeff      = 1.0f - std::exp (-6.28318f * 200.0f  / sr);
        fathomHPCoeff      = 1.0f - std::exp (-6.28318f * 3000.0f / sr);
        fathomCompAttack   = 1.0f - std::exp (-1.0f / (0.01f * sr));
        fathomCompRelease  = 1.0f - std::exp (-1.0f / (0.1f  * sr));

        // --- Tide cached coefficient (constant) ---
        tideSmoothRate     = 1.0f - std::exp (-6.28318f * 30.0f / sr);

        // --- Surface cached coefficients (constant) ---
        surfaceShelfCoeff  = 1.0f - std::exp (-6.28318f * 2000.0f / sr);
        surfaceTransAttack = 1.0f - std::exp (-1.0f / (0.001f * sr));

        // --- Biolume cached coefficient (constant) ---
        biolumeHPCoeff     = 1.0f - std::exp (-6.28318f * 3500.0f / sr);

        // Fathom — clear filter/compressor state
        for (auto& s : fathomLPState)  s = 0.0f;
        for (auto& s : fathomHPState)  s = 0.0f;
        for (auto& s : fathomCompEnv)  s = 0.0f;

        // Drift — Brownian LFO + chorus delay lines
        driftRng = LCG { 48271u };
        driftPhaseL = 0.0f;
        driftPhaseR = 0.37f; // offset for stereo decorrelation
        int maxDelaySamples = static_cast<int> (sr * 0.03f) + 16; // 30ms max
        driftDelayL.assign (static_cast<size_t> (maxDelaySamples), 0.0f);
        driftDelayR.assign (static_cast<size_t> (maxDelaySamples), 0.0f);
        driftWritePos = 0;

        // Tide — LFO state
        tideLFOPhase = 0.0f;
        tideSmoothGain = 1.0f;
        tideSmoothCutoff = 8000.0f;
        tideFilterStateL = 0.0f;
        tideFilterStateR = 0.0f;

        // Reef — delegate to shared FDNReverb
        reef.prepare (sampleRate);

        // Surface — filter state
        surfaceShelfStateL = 0.0f;
        surfaceShelfStateR = 0.0f;
        surfaceTransEnvL = 0.0f;
        surfaceTransEnvR = 0.0f;

        // Biolume — exciter state
        biolumeHPStateL = 0.0f;
        biolumeHPStateR = 0.0f;
        biolumeDecayEnvL = 0.0f;
        biolumeDecayEnvR = 0.0f;
    }

    //--------------------------------------------------------------------------
    void reset()
    {
        for (auto& s : fathomLPState)  s = 0.0f;
        for (auto& s : fathomHPState)  s = 0.0f;
        for (auto& s : fathomCompEnv)  s = 0.0f;
        driftPhaseL = 0.0f;
        driftPhaseR = 0.37f;
        std::fill (driftDelayL.begin(), driftDelayL.end(), 0.0f);
        std::fill (driftDelayR.begin(), driftDelayR.end(), 0.0f);
        driftWritePos = 0;
        tideLFOPhase = 0.0f;
        tideSmoothGain = 1.0f;
        tideSmoothCutoff = 8000.0f;
        tideFilterStateL = 0.0f;
        tideFilterStateR = 0.0f;
        reef.reset();
        surfaceShelfStateL = 0.0f;
        surfaceShelfStateR = 0.0f;
        surfaceTransEnvL = 0.0f;
        surfaceTransEnvR = 0.0f;
        biolumeHPStateL = 0.0f;
        biolumeHPStateR = 0.0f;
        biolumeDecayEnvL = 0.0f;
        biolumeDecayEnvR = 0.0f;
    }

    //--------------------------------------------------------------------------
    // Process a stereo block in-place. All parameters are pre-read by caller.
    void processBlock (float* L, float* R, int numSamples,
                       float fathomDepth, float fathomPressure, float fathomMix,
                       float driftRate, float driftWidth, float driftDepth, float driftMix,
                       float tideRate, int tideShape, int tideTarget, float tideMix,
                       float reefSize, float reefDamping, float reefDensity, float reefMix,
                       float surfaceLevel, float surfaceTension, float surfaceMix,
                       float biolumeGlow, float biolumeSpectrum, float biolumeDecay, float biolumeMix)
    {
        // Parameter-dependent coefficients — compute once per block, not per sample
        const float ratio            = 1.0f + fathomDepth * 5.0f;
        const float fathomPressureCoeff = (fathomPressure > 0.001f)
            ? 1.0f - std::exp (-1.0f / ((0.005f + fathomPressure * 0.05f) * sr)) : 0.0f;

        const float tideFilterCoeff  = 1.0f - std::exp (-6.28318f * (tideSmoothCutoff / sr));

        const float surfaceTransRelease = (surfaceTension > 0.001f)
            ? 1.0f - std::exp (-1.0f / ((0.05f + (1.0f - surfaceTension) * 0.2f) * sr)) : 0.0f;

        const float biolumeDecayCoeff = 1.0f - std::exp (-1.0f / ((0.05f + biolumeDecay * 0.5f) * sr));

        for (int s = 0; s < numSamples; ++s)
        {
            float inL = L[s];
            float inR = R[s];

            // ==================================================================
            // 1. FATHOM — Hydrostatic Pressure
            //    Multiband compression + shelf warmth + high roll-off.
            //    Deeper = more submerged: lows gain mass, highs attenuate.
            // ==================================================================
            if (fathomMix > 0.001f)
            {
                // 3-band split using pre-cached coefficients
                fathomLPState[0] += (inL - fathomLPState[0]) * fathomLPCoeff;
                fathomLPState[1] += (inR - fathomLPState[1]) * fathomLPCoeff;
                float lowL = fathomLPState[0];
                float lowR = fathomLPState[1];

                fathomHPState[0] += (inL - fathomHPState[0]) * fathomHPCoeff;
                fathomHPState[1] += (inR - fathomHPState[1]) * fathomHPCoeff;
                float highL = inL - fathomHPState[0];
                float highR = inR - fathomHPState[1];

                float midL = inL - lowL - highL;
                float midR = inR - lowR - highR;

                // Low band: boost at depth
                float absLow = std::max (std::fabs (lowL), std::fabs (lowR));
                float envCoeff = (absLow > fathomCompEnv[0]) ? fathomCompAttack : fathomCompRelease;
                fathomCompEnv[0] += (absLow - fathomCompEnv[0]) * envCoeff;
                float lowGain = 1.0f + fathomDepth * 0.5f;
                lowL *= lowGain;
                lowR *= lowGain;

                // Mid band: gentle compression
                float absMid = std::max (std::fabs (midL), std::fabs (midR));
                envCoeff = (absMid > fathomCompEnv[1]) ? fathomCompAttack : fathomCompRelease;
                fathomCompEnv[1] += (absMid - fathomCompEnv[1]) * envCoeff;
                float midComp = (fathomCompEnv[1] > 0.01f)
                    ? std::pow (fathomCompEnv[1], 1.0f - 1.0f / ratio) : 1.0f;
                midL *= midComp;
                midR *= midComp;

                // High band: attenuate at depth
                float highAtten = 1.0f - fathomDepth * 0.85f;
                highL *= highAtten;
                highR *= highAtten;

                // Transient softening (pressure parameter)
                if (fathomPressure > 0.001f)
                {
                    fathomCompEnv[2] += ((lowL + midL + highL) - fathomCompEnv[2]) * fathomPressureCoeff;
                    fathomCompEnv[3] += ((lowR + midR + highR) - fathomCompEnv[3]) * fathomPressureCoeff;
                    float dryL2 = lowL + midL + highL;
                    float dryR2 = lowR + midR + highR;
                    float pMix = fathomPressure * 0.6f;
                    lowL = dryL2 * (1.0f - pMix) + fathomCompEnv[2] * pMix;
                    lowR = dryR2 * (1.0f - pMix) + fathomCompEnv[3] * pMix;
                    midL = 0.0f; highL = 0.0f;
                    midR = 0.0f; highR = 0.0f;
                }

                float fathomL = lowL + midL + highL;
                float fathomR = lowR + midR + highR;

                inL = inL * (1.0f - fathomMix) + fathomL * fathomMix;
                inR = inR * (1.0f - fathomMix) + fathomR * fathomMix;
                inL = flushDenormal (inL);
                inR = flushDenormal (inR);
            }

            // ==================================================================
            // 2. DRIFT — Ocean Currents
            //    Stereo chorus with Brownian motion LFO (random walk, not sine).
            //    Sound drifts like kelp in a current. Never the same path twice.
            // ==================================================================
            if (driftMix > 0.001f)
            {
                // Brownian LFO via shared LCG
                float stepSize = driftRate * 0.0002f;
                driftPhaseL += driftRng.nextFloat() * stepSize;
                driftPhaseL = std::max (-1.0f, std::min (1.0f, driftPhaseL));
                driftPhaseR += driftRng.nextFloat() * stepSize;
                driftPhaseR = std::max (-1.0f, std::min (1.0f, driftPhaseR));

                float centerDelay = 0.010f * sr;
                float modRange = driftDepth * 0.005f * sr;

                float delayL = centerDelay + driftPhaseL * modRange;
                float delayR = centerDelay + driftPhaseR * modRange;
                int bufSize = static_cast<int> (driftDelayL.size());
                delayL = std::max (1.0f, std::min (delayL, static_cast<float> (bufSize - 2)));
                delayR = std::max (1.0f, std::min (delayR, static_cast<float> (bufSize - 2)));

                if (bufSize > 0)
                {
                    driftDelayL[static_cast<size_t> (driftWritePos)] = inL;
                    driftDelayR[static_cast<size_t> (driftWritePos)] = inR;

                    float readPosL = static_cast<float> (driftWritePos) - delayL;
                    if (readPosL < 0.0f) readPosL += static_cast<float> (bufSize);
                    int idxL = static_cast<int> (readPosL);
                    float fracL = readPosL - static_cast<float> (idxL);
                    int nextL = (idxL + 1) % bufSize;
                    float chorusL = driftDelayL[static_cast<size_t> (idxL)] * (1.0f - fracL)
                                  + driftDelayL[static_cast<size_t> (nextL)] * fracL;

                    float readPosR = static_cast<float> (driftWritePos) - delayR;
                    if (readPosR < 0.0f) readPosR += static_cast<float> (bufSize);
                    int idxR = static_cast<int> (readPosR);
                    float fracR = readPosR - static_cast<float> (idxR);
                    int nextR = (idxR + 1) % bufSize;
                    float chorusR = driftDelayR[static_cast<size_t> (idxR)] * (1.0f - fracR)
                                  + driftDelayR[static_cast<size_t> (nextR)] * fracR;

                    float widthL = chorusL * (1.0f - driftWidth * 0.5f) + chorusR * driftWidth * 0.5f;
                    float widthR = chorusR * (1.0f - driftWidth * 0.5f) + chorusL * driftWidth * 0.5f;

                    inL = inL * (1.0f - driftMix) + widthL * driftMix;
                    inR = inR * (1.0f - driftMix) + widthR * driftMix;

                    driftWritePos = (driftWritePos + 1) % bufSize;
                }
                inL = flushDenormal (inL);
                inR = flushDenormal (inR);
            }

            // ==================================================================
            // 3. TIDE — Tidal Rhythm
            //    Tremolo / auto-filter with configurable LFO.
            //    "Lunar" shape: asymmetric rise/fall (fast in, slow out).
            // ==================================================================
            if (tideMix > 0.001f)
            {
                // Capture pre-Tide signal for correct dry/wet blend
                const float preTideL = inL;
                const float preTideR = inR;

                // LFO — 3 shapes: 0=sine, 1=triangle, 2=lunar (asymmetric)
                float lfoVal = 0.0f;
                switch (tideShape)
                {
                    case 0: // Sine
                        lfoVal = fastSin (tideLFOPhase * 6.28318f);
                        break;
                    case 1: // Triangle
                        lfoVal = 4.0f * std::fabs (tideLFOPhase - 0.5f) - 1.0f;
                        break;
                    case 2: // Lunar (fast rise, slow fall — like tide coming in)
                    {
                        float p = tideLFOPhase;
                        if (p < 0.3f)
                            lfoVal = p / 0.3f;
                        else
                            lfoVal = 1.0f - (p - 0.3f) / 0.7f;
                        lfoVal = lfoVal * 2.0f - 1.0f;
                        break;
                    }
                    default:
                        lfoVal = fastSin (tideLFOPhase * 6.28318f);
                        break;
                }

                tideLFOPhase += tideRate / sr;
                if (tideLFOPhase >= 1.0f) tideLFOPhase -= 1.0f;

                float lfoUni = (lfoVal + 1.0f) * 0.5f;

                if (tideTarget == 0 || tideTarget == 2)
                {
                    float targetGain = 1.0f - lfoUni * 0.8f;
                    tideSmoothGain += (targetGain - tideSmoothGain) * tideSmoothRate;
                    inL *= tideSmoothGain;
                    inR *= tideSmoothGain;
                }

                if (tideTarget == 1 || tideTarget == 2)
                {
                    float targetCutoff = 200.0f + lfoUni * 7800.0f;
                    tideSmoothCutoff += (targetCutoff - tideSmoothCutoff) * tideSmoothRate;
                    // tideFilterCoeff is recomputed per block; good enough for a slow sweep
                    tideFilterStateL += (inL - tideFilterStateL) * tideFilterCoeff;
                    tideFilterStateR += (inR - tideFilterStateR) * tideFilterCoeff;
                    inL = tideFilterStateL;
                    inR = tideFilterStateR;
                }

                // Blend pre-Tide dry with processed wet
                inL = preTideL * (1.0f - tideMix) + inL * tideMix;
                inR = preTideR * (1.0f - tideMix) + inR * tideMix;
                inL = flushDenormal (inL);
                inR = flushDenormal (inR);
            }

            // ==================================================================
            // 4. REEF — Enclosed Underwater Space
            //    4-tap Householder FDN with LP-filtered diffusion.
            //    Delegates to shared FDNReverb.
            // ==================================================================
            if (reefMix > 0.001f)
            {
                float dryL = inL;
                float dryR = inR;

                float fdnInput = (inL + inR) * 0.5f * reefDensity;
                float reefL, reefR;
                reef.processSample (fdnInput, reefSize, reefDamping, reefSize, reefL, reefR);

                inL = dryL * (1.0f - reefMix) + reefL * reefMix;
                inR = dryR * (1.0f - reefMix) + reefR * reefMix;
                inL = flushDenormal (inL);
                inR = flushDenormal (inR);
            }

            // ==================================================================
            // 5. SURFACE — The Air-Water Boundary
            //    Dynamic high-shelf filter + transient shaper.
            //    -1 = deep submerged (LP warmth), 0 = at surface, +1 = bright air.
            // ==================================================================
            if (surfaceMix > 0.001f)
            {
                float dryL = inL;
                float dryR = inR;

                // High-shelf using pre-cached coefficient
                surfaceShelfStateL += (inL - surfaceShelfStateL) * surfaceShelfCoeff;
                surfaceShelfStateR += (inR - surfaceShelfStateR) * surfaceShelfCoeff;
                float lpL = surfaceShelfStateL;
                float lpR = surfaceShelfStateR;
                float hpL = inL - lpL;
                float hpR = inR - lpR;

                float highGain, lowGain;
                if (surfaceLevel < 0.0f)
                {
                    highGain = 1.0f + surfaceLevel * 0.9f;
                    lowGain  = 1.0f - surfaceLevel * 0.15f;
                }
                else
                {
                    highGain = 1.0f + surfaceLevel * 1.5f;
                    lowGain  = 1.0f - surfaceLevel * 0.2f;
                }

                float surfL = lpL * lowGain + hpL * highGain;
                float surfR = lpR * lowGain + hpR * highGain;

                if (surfaceTension > 0.001f)
                {
                    float envInL = std::fabs (surfL);
                    float envInR = std::fabs (surfR);
                    float coeffL = (envInL > surfaceTransEnvL) ? surfaceTransAttack : surfaceTransRelease;
                    float coeffR = (envInR > surfaceTransEnvR) ? surfaceTransAttack : surfaceTransRelease;
                    surfaceTransEnvL += (envInL - surfaceTransEnvL) * coeffL;
                    surfaceTransEnvR += (envInR - surfaceTransEnvR) * coeffR;

                    float transGainL = 1.0f + (envInL - surfaceTransEnvL) * surfaceTension * 3.0f;
                    float transGainR = 1.0f + (envInR - surfaceTransEnvR) * surfaceTension * 3.0f;
                    transGainL = std::max (0.5f, std::min (transGainL, 2.0f));
                    transGainR = std::max (0.5f, std::min (transGainR, 2.0f));

                    surfL *= transGainL;
                    surfR *= transGainR;
                }

                inL = dryL * (1.0f - surfaceMix) + surfL * surfaceMix;
                inR = dryR * (1.0f - surfaceMix) + surfR * surfaceMix;
                inL = flushDenormal (inL);
                inR = flushDenormal (inR);
            }

            // ==================================================================
            // 6. BIOLUME — Bioluminescence
            //    Spectral exciter + harmonic shimmer. Adds luminous overtones
            //    that weren't in the original signal — deep-sea light.
            // ==================================================================
            if (biolumeMix > 0.001f)
            {
                float dryL = inL;
                float dryR = inR;

                // HP filter using pre-cached coefficient (3.5 kHz)
                biolumeHPStateL += (inL - biolumeHPStateL) * biolumeHPCoeff;
                biolumeHPStateR += (inR - biolumeHPStateR) * biolumeHPCoeff;
                float hfL = inL - biolumeHPStateL;
                float hfR = inR - biolumeHPStateR;

                float excitedL, excitedR;
                if (biolumeSpectrum < 0.5f)
                {
                    float blend = biolumeSpectrum * 2.0f;
                    float rectL = std::max (0.0f, hfL);
                    float rectR = std::max (0.0f, hfR);
                    float symL = fastTanh (hfL * (1.0f + biolumeGlow * 3.0f));
                    float symR = fastTanh (hfR * (1.0f + biolumeGlow * 3.0f));
                    excitedL = rectL * (1.0f - blend) + symL * blend;
                    excitedR = rectR * (1.0f - blend) + symR * blend;
                }
                else
                {
                    float drive = 1.0f + biolumeGlow * 4.0f;
                    excitedL = fastTanh (hfL * drive);
                    excitedR = fastTanh (hfR * drive);
                }

                // Shimmer decay envelope using pre-computed per-block coefficient
                float absExcL = std::fabs (excitedL);
                float absExcR = std::fabs (excitedR);
                if (absExcL > biolumeDecayEnvL)
                    biolumeDecayEnvL = absExcL;
                else
                    biolumeDecayEnvL += (absExcL - biolumeDecayEnvL) * biolumeDecayCoeff;
                if (absExcR > biolumeDecayEnvR)
                    biolumeDecayEnvR = absExcR;
                else
                    biolumeDecayEnvR += (absExcR - biolumeDecayEnvR) * biolumeDecayCoeff;

                excitedL *= std::min (1.0f, biolumeDecayEnvL * 2.0f + 0.3f);
                excitedR *= std::min (1.0f, biolumeDecayEnvR * 2.0f + 0.3f);
                excitedL *= biolumeGlow;
                excitedR *= biolumeGlow;

                inL = dryL + excitedL * biolumeMix;
                inR = dryR + excitedR * biolumeMix;
                inL = flushDenormal (inL);
                inR = flushDenormal (inR);
            }

            L[s] = inL;
            R[s] = inR;
        }
    }

    //--------------------------------------------------------------------------
    // Parameter declarations for APVTS
    //--------------------------------------------------------------------------
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PC = juce::AudioParameterChoice;

        // 1. Fathom — Hydrostatic Pressure (3 params)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_fathomDepth", 1 }, "Aqua Fathom Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_fathomPressure", 1 }, "Aqua Fathom Pressure",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_fathomMix", 1 }, "Aqua Fathom Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // 2. Drift — Ocean Currents (4 params)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_driftRate", 1 }, "Aqua Drift Rate",
            juce::NormalisableRange<float> (0.01f, 5.0f, 0.01f, 0.3f), 0.5f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_driftWidth", 1 }, "Aqua Drift Width",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_driftDepth", 1 }, "Aqua Drift Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_driftMix", 1 }, "Aqua Drift Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // 3. Tide — Tidal Rhythm (4 params)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_tideRate", 1 }, "Aqua Tide Rate",
            juce::NormalisableRange<float> (0.01f, 4.0f, 0.01f, 0.3f), 0.5f));
        params.push_back (std::make_unique<PC> (
            juce::ParameterID { "aqua_tideShape", 1 }, "Aqua Tide Shape",
            juce::StringArray { "Sine", "Triangle", "Lunar" }, 0));
        params.push_back (std::make_unique<PC> (
            juce::ParameterID { "aqua_tideTarget", 1 }, "Aqua Tide Target",
            juce::StringArray { "Amplitude", "Filter", "Both" }, 0));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_tideMix", 1 }, "Aqua Tide Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // 4. Reef — Enclosed Underwater Space (4 params)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_reefSize", 1 }, "Aqua Reef Size",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.4f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_reefDamping", 1 }, "Aqua Reef Damping",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_reefDensity", 1 }, "Aqua Reef Density",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_reefMix", 1 }, "Aqua Reef Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // 5. Surface — Air-Water Boundary (3 params)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_surfaceLevel", 1 }, "Aqua Surface Level",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_surfaceTension", 1 }, "Aqua Surface Tension",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_surfaceMix", 1 }, "Aqua Surface Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // 6. Biolume — Bioluminescence (4 params)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_biolumeGlow", 1 }, "Aqua Biolume Glow",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_biolumeSpectrum", 1 }, "Aqua Biolume Spectrum",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_biolumeDecay", 1 }, "Aqua Biolume Decay",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "aqua_biolumeMix", 1 }, "Aqua Biolume Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    }

private:
    float sr = 44100.0f;

    // --- Cached coefficients (computed in prepare, constant for the session) ---
    float fathomLPCoeff      = 0.0f;
    float fathomHPCoeff      = 0.0f;
    float fathomCompAttack   = 0.0f;
    float fathomCompRelease  = 0.0f;
    float tideSmoothRate     = 0.0f;
    float surfaceShelfCoeff  = 0.0f;
    float surfaceTransAttack = 0.0f;
    float biolumeHPCoeff     = 0.0f;

    // --- Fathom state ---
    float fathomLPState[2] {};    // L/R low-band filter
    float fathomHPState[2] {};    // L/R high-band filter
    float fathomCompEnv[4] {};    // [0]=low, [1]=mid, [2]=transient L, [3]=transient R

    // --- Drift state ---
    float driftPhaseL = 0.0f;
    float driftPhaseR = 0.37f;
    LCG driftRng { 48271u };
    std::vector<float> driftDelayL;   // chorus delay buffer L (plain vector)
    std::vector<float> driftDelayR;   // chorus delay buffer R
    int driftWritePos = 0;

    // --- Tide state ---
    float tideLFOPhase = 0.0f;
    float tideSmoothGain = 1.0f;
    float tideSmoothCutoff = 8000.0f;
    float tideFilterStateL = 0.0f;
    float tideFilterStateR = 0.0f;

    // --- Reef state — shared FDNReverb ---
    FDNReverb reef;

    // --- Surface state ---
    float surfaceShelfStateL = 0.0f;
    float surfaceShelfStateR = 0.0f;
    float surfaceTransEnvL = 0.0f;
    float surfaceTransEnvR = 0.0f;

    // --- Biolume state ---
    float biolumeHPStateL = 0.0f;
    float biolumeHPStateR = 0.0f;
    float biolumeDecayEnvL = 0.0f;
    float biolumeDecayEnvR = 0.0f;
};

} // namespace xomnibus
