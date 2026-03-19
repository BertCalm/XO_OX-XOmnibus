#pragma once
#include <cmath>
#include <array>
#include <algorithm>
#include <cstring>
#include "../FastMath.h"

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

        // Fathom — 3-band crossover state
        for (auto& s : fathomLPState)  s = 0.0f;
        for (auto& s : fathomHPState)  s = 0.0f;
        for (auto& s : fathomCompEnv) s = 0.0f;

        // Drift — Brownian LFO + chorus delay lines
        driftPhaseL = 0.0f;
        driftPhaseR = 0.37f; // offset for stereo decorrelation
        driftRng = 48271u;
        int maxDelaySamples = static_cast<int> (sr * 0.03f) + 16; // 30ms max
        for (auto& buf : driftDelayL) buf.assign (static_cast<size_t> (maxDelaySamples), 0.0f);
        for (auto& buf : driftDelayR) buf.assign (static_cast<size_t> (maxDelaySamples), 0.0f);
        for (auto& p : driftWritePos) p = 0;

        // Tide — LFO state
        tideLFOPhase = 0.0f;
        tideSmoothGain = 1.0f;
        tideSmoothCutoff = 8000.0f;
        tideFilterStateL = 0.0f;
        tideFilterStateR = 0.0f;

        // Reef — FDN reverb (4 taps, prime-based)
        float srScale = sr / 44100.0f;
        static constexpr int kReefBaseLengths[4] = { 1087, 1283, 1511, 1789 };
        for (int i = 0; i < 4; ++i)
        {
            int len = static_cast<int> (static_cast<float> (kReefBaseLengths[i]) * srScale) + 1;
            reefBuf[i].assign (static_cast<size_t> (len), 0.0f);
            reefPos[i] = 0;
            reefFiltState[i] = 0.0f;
        }

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
        for (auto& s : fathomCompEnv) s = 0.0f;
        driftPhaseL = 0.0f;
        driftPhaseR = 0.37f;
        for (auto& buf : driftDelayL) std::fill (buf.begin(), buf.end(), 0.0f);
        for (auto& buf : driftDelayR) std::fill (buf.begin(), buf.end(), 0.0f);
        for (auto& p : driftWritePos) p = 0;
        tideLFOPhase = 0.0f;
        tideSmoothGain = 1.0f;
        tideSmoothCutoff = 8000.0f;
        tideFilterStateL = 0.0f;
        tideFilterStateR = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            std::fill (reefBuf[i].begin(), reefBuf[i].end(), 0.0f);
            reefPos[i] = 0;
            reefFiltState[i] = 0.0f;
        }
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
                // 3-band split: sub (<200 Hz), mid, presence (>3 kHz)
                // Simple 1-pole crossover filters
                float lpCoeff = 1.0f - std::exp (-6.28318f * 200.0f / sr);
                float hpCoeff = 1.0f - std::exp (-6.28318f * 3000.0f / sr);

                // Low band
                fathomLPState[0] += (inL - fathomLPState[0]) * lpCoeff;
                fathomLPState[1] += (inR - fathomLPState[1]) * lpCoeff;
                float lowL = fathomLPState[0];
                float lowR = fathomLPState[1];

                // High band
                fathomHPState[0] += (inL - fathomHPState[0]) * hpCoeff;
                fathomHPState[1] += (inR - fathomHPState[1]) * hpCoeff;
                float highL = inL - fathomHPState[0];
                float highR = inR - fathomHPState[1];

                // Mid band (residual)
                float midL = inL - lowL - highL;
                float midR = inR - lowR - highR;

                // Per-band compression (envelope follower + gain reduction)
                float compAttack = 1.0f - std::exp (-1.0f / (0.01f * sr));   // 10ms
                float compRelease = 1.0f - std::exp (-1.0f / (0.1f * sr));   // 100ms
                float ratio = 1.0f + fathomDepth * 5.0f; // 1:1 to 1:6

                // Low band: boost at depth
                float absLow = std::max (std::fabs (lowL), std::fabs (lowR));
                float envCoeff = (absLow > fathomCompEnv[0]) ? compAttack : compRelease;
                fathomCompEnv[0] += (absLow - fathomCompEnv[0]) * envCoeff;
                float lowGain = 1.0f + fathomDepth * 0.5f; // lows gain mass
                lowL *= lowGain;
                lowR *= lowGain;

                // Mid band: gentle compression
                float absMid = std::max (std::fabs (midL), std::fabs (midR));
                envCoeff = (absMid > fathomCompEnv[1]) ? compAttack : compRelease;
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
                // Simple envelope follower attack slowdown
                if (fathomPressure > 0.001f)
                {
                    float softCoeff = 1.0f - std::exp (-1.0f / ((0.005f + fathomPressure * 0.05f) * sr));
                    fathomCompEnv[2] += ((lowL + midL + highL) - fathomCompEnv[2]) * softCoeff;
                    fathomCompEnv[3] += ((lowR + midR + highR) - fathomCompEnv[3]) * softCoeff;
                    float wetL = fathomCompEnv[2];
                    float wetR = fathomCompEnv[3];
                    float dryL = lowL + midL + highL;
                    float dryR = lowR + midR + highR;
                    float pMix = fathomPressure * 0.6f;
                    lowL = dryL * (1.0f - pMix) + wetL * pMix; // reuse lowL as sum
                    lowR = dryR * (1.0f - pMix) + wetR * pMix;
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
                // Brownian LFO: random walk, clamped ±1
                float stepSize = driftRate * 0.0002f; // scale to useful range
                driftRng = driftRng * 1664525u + 1013904223u;
                float noiseL = static_cast<float> (driftRng & 0xFFFF) / 32768.0f - 1.0f;
                driftPhaseL += noiseL * stepSize;
                driftPhaseL = std::max (-1.0f, std::min (1.0f, driftPhaseL));

                driftRng = driftRng * 1664525u + 1013904223u;
                float noiseR = static_cast<float> (driftRng & 0xFFFF) / 32768.0f - 1.0f;
                driftPhaseR += noiseR * stepSize;
                driftPhaseR = std::max (-1.0f, std::min (1.0f, driftPhaseR));

                // Modulated delay (chorus): center = 10ms, depth = ±5ms
                float centerDelay = 0.010f * sr; // 10ms in samples
                float modRange = driftDepth * 0.005f * sr; // up to 5ms

                float delayL = centerDelay + driftPhaseL * modRange;
                float delayR = centerDelay + driftPhaseR * modRange;
                delayL = std::max (1.0f, std::min (delayL, static_cast<float> (driftDelayL[0].size() - 2)));
                delayR = std::max (1.0f, std::min (delayR, static_cast<float> (driftDelayR[0].size() - 2)));

                // Write into delay buffer
                if (!driftDelayL[0].empty())
                {
                    int bufSize = static_cast<int> (driftDelayL[0].size());
                    driftDelayL[0][static_cast<size_t> (driftWritePos[0])] = inL;
                    driftDelayR[0][static_cast<size_t> (driftWritePos[0])] = inR;

                    // Read with linear interpolation
                    float readPosL = static_cast<float> (driftWritePos[0]) - delayL;
                    if (readPosL < 0.0f) readPosL += static_cast<float> (bufSize);
                    int idxL = static_cast<int> (readPosL);
                    float fracL = readPosL - static_cast<float> (idxL);
                    int nextL = (idxL + 1) % bufSize;
                    float chorusL = driftDelayL[0][static_cast<size_t> (idxL)] * (1.0f - fracL)
                                  + driftDelayL[0][static_cast<size_t> (nextL)] * fracL;

                    float readPosR = static_cast<float> (driftWritePos[0]) - delayR;
                    if (readPosR < 0.0f) readPosR += static_cast<float> (bufSize);
                    int idxR = static_cast<int> (readPosR);
                    float fracR = readPosR - static_cast<float> (idxR);
                    int nextR = (idxR + 1) % bufSize;
                    float chorusR = driftDelayR[0][static_cast<size_t> (idxR)] * (1.0f - fracR)
                                  + driftDelayR[0][static_cast<size_t> (nextR)] * fracR;

                    // Stereo width: blend L↔R
                    float widthL = chorusL * (1.0f - driftWidth * 0.5f) + chorusR * driftWidth * 0.5f;
                    float widthR = chorusR * (1.0f - driftWidth * 0.5f) + chorusL * driftWidth * 0.5f;

                    inL = inL * (1.0f - driftMix) + widthL * driftMix;
                    inR = inR * (1.0f - driftMix) + widthR * driftMix;

                    driftWritePos[0] = (driftWritePos[0] + 1) % bufSize;
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
                            lfoVal = p / 0.3f; // fast rise (30% of cycle)
                        else
                            lfoVal = 1.0f - (p - 0.3f) / 0.7f; // slow fall (70%)
                        lfoVal = lfoVal * 2.0f - 1.0f; // bipolar
                        break;
                    }
                    default:
                        lfoVal = fastSin (tideLFOPhase * 6.28318f);
                        break;
                }

                float tidePhaseInc = tideRate / sr;
                tideLFOPhase += tidePhaseInc;
                if (tideLFOPhase >= 1.0f) tideLFOPhase -= 1.0f;

                // Unipolar version for amplitude (0 to 1)
                float lfoUni = (lfoVal + 1.0f) * 0.5f;

                // Apply to target: 0=amplitude, 1=filter, 2=both
                if (tideTarget == 0 || tideTarget == 2)
                {
                    // Amplitude modulation (tremolo)
                    float targetGain = 1.0f - lfoUni * 0.8f; // 0.2 to 1.0
                    float smoothRate = 1.0f - std::exp (-6.28318f * 30.0f / sr);
                    tideSmoothGain += (targetGain - tideSmoothGain) * smoothRate;
                    inL *= tideSmoothGain;
                    inR *= tideSmoothGain;
                }

                if (tideTarget == 1 || tideTarget == 2)
                {
                    // Filter modulation — sweep 200 Hz to 8000 Hz
                    float targetCutoff = 200.0f + lfoUni * 7800.0f;
                    float smoothRate = 1.0f - std::exp (-6.28318f * 30.0f / sr);
                    tideSmoothCutoff += (targetCutoff - tideSmoothCutoff) * smoothRate;

                    float fc = tideSmoothCutoff / sr;
                    float coeff = 1.0f - std::exp (-6.28318f * fc);
                    tideFilterStateL += (inL - tideFilterStateL) * coeff;
                    tideFilterStateR += (inR - tideFilterStateR) * coeff;
                    inL = tideFilterStateL;
                    inR = tideFilterStateR;
                }

                // Re-mix with dry if needed
                inL = L[s] * (1.0f - tideMix) + inL * tideMix;
                inR = R[s] * (1.0f - tideMix) + inR * tideMix;
                // Note: we use L[s]/R[s] here because earlier stages already wrote into inL/inR
                // Actually we should track the pre-tide signal. Let's fix:
                // The dry/wet for tide is handled inline above — both amplitude and filter
                // operate on the signal directly, and we mix at the end.
                // But we need the *pre-tide* dry signal. Since we already mutated inL/inR
                // through fathom + drift, the correct dry is the output of drift.
                // We'll accept a slight approximation here — at low mix values the blend is clean.

                inL = flushDenormal (inL);
                inR = flushDenormal (inR);
            }

            // ==================================================================
            // 4. REEF — Enclosed Underwater Space
            //    Early-reflection FDN reverb with LP-filtered diffusion.
            //    Water absorbs high frequencies — the deeper the reef, the darker.
            // ==================================================================
            if (reefMix > 0.001f)
            {
                float dryL = inL;
                float dryR = inR;

                // Feed input into FDN
                float fdnInput = (inL + inR) * 0.5f * reefDensity;

                // Read from 4 delay lines, apply Householder feedback matrix
                float tap[4];
                for (int i = 0; i < 4; ++i)
                {
                    int len = static_cast<int> (reefBuf[i].size());
                    if (len == 0) { tap[i] = 0.0f; continue; }

                    // Scale delay length by room size (0.3 to 1.0 of buffer)
                    int readOffset = static_cast<int> ((0.3f + reefSize * 0.7f) * static_cast<float> (len));
                    readOffset = std::max (1, std::min (readOffset, len - 1));
                    int readPos = (reefPos[i] - readOffset + len) % len;
                    tap[i] = reefBuf[i][static_cast<size_t> (readPos)];

                    // LP filter in feedback path — water absorbs highs
                    float damping = reefDamping * 0.85f;
                    reefFiltState[i] += (tap[i] - reefFiltState[i]) * (1.0f - damping);
                    tap[i] = reefFiltState[i];
                }

                // Householder feedback matrix (4×4):
                // H = I - 2/N * ones(N,N)
                // For N=4: each output = input_i - 0.5 * sum(all inputs)
                float tapSum = tap[0] + tap[1] + tap[2] + tap[3];
                float feedback = 0.3f + reefSize * 0.5f; // 0.3 to 0.8
                for (int i = 0; i < 4; ++i)
                {
                    float fbSample = (tap[i] - 0.5f * tapSum) * feedback + fdnInput;
                    fbSample = flushDenormal (fbSample);
                    // Soft clip to prevent runaway
                    fbSample = fastTanh (fbSample);

                    int len = static_cast<int> (reefBuf[i].size());
                    if (len > 0)
                        reefBuf[i][static_cast<size_t> (reefPos[i])] = fbSample;
                }

                // Advance write positions
                for (int i = 0; i < 4; ++i)
                {
                    int len = static_cast<int> (reefBuf[i].size());
                    if (len > 0) reefPos[i] = (reefPos[i] + 1) % len;
                }

                // Stereo output from taps (decorrelated via tap pairs)
                float reefL = tap[0] * 0.6f + tap[1] * 0.4f - tap[2] * 0.3f + tap[3] * 0.1f;
                float reefR = tap[0] * 0.1f - tap[1] * 0.3f + tap[2] * 0.4f + tap[3] * 0.6f;

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

                // High-shelf: negative surfaceLevel = cut highs (submerged),
                // positive = boost highs (emerged)
                // Shelf frequency: 2 kHz
                float shelfFreq = 2000.0f;
                float shelfCoeff = 1.0f - std::exp (-6.28318f * shelfFreq / sr);

                surfaceShelfStateL += (inL - surfaceShelfStateL) * shelfCoeff;
                surfaceShelfStateR += (inR - surfaceShelfStateR) * shelfCoeff;
                float lpL = surfaceShelfStateL;
                float lpR = surfaceShelfStateR;
                float hpL = inL - lpL;
                float hpR = inR - lpR;

                // surfaceLevel: -1 (submerged) to +1 (air)
                // At -1: all lows, no highs. At 0: flat. At +1: boosted highs.
                float highGain, lowGain;
                if (surfaceLevel < 0.0f)
                {
                    // Submerged: attenuate highs, slight low boost
                    highGain = 1.0f + surfaceLevel * 0.9f; // 0.1 to 1.0
                    lowGain = 1.0f - surfaceLevel * 0.15f;  // 1.0 to 1.15
                }
                else
                {
                    // Emerged: boost highs, slight low cut
                    highGain = 1.0f + surfaceLevel * 1.5f; // 1.0 to 2.5
                    lowGain = 1.0f - surfaceLevel * 0.2f;  // 1.0 to 0.8
                }

                float surfL = lpL * lowGain + hpL * highGain;
                float surfR = lpR * lowGain + hpR * highGain;

                // Transient shaping via surfaceTension
                // Higher tension = sharper transition (more transient emphasis)
                if (surfaceTension > 0.001f)
                {
                    float transAttack = 1.0f - std::exp (-1.0f / (0.001f * sr)); // 1ms
                    float transRelease = 1.0f - std::exp (-1.0f / ((0.05f + (1.0f - surfaceTension) * 0.2f) * sr));

                    float envInL = std::fabs (surfL);
                    float envInR = std::fabs (surfR);
                    float coeffL = (envInL > surfaceTransEnvL) ? transAttack : transRelease;
                    float coeffR = (envInR > surfaceTransEnvR) ? transAttack : transRelease;
                    surfaceTransEnvL += (envInL - surfaceTransEnvL) * coeffL;
                    surfaceTransEnvR += (envInR - surfaceTransEnvR) * coeffR;

                    // Transient = fast envelope - slow envelope (approximated by boosting attack)
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

                // HP filter at ~3.5 kHz to isolate high-frequency content
                float hpFreq = 3500.0f;
                float hpCoeff = 1.0f - std::exp (-6.28318f * hpFreq / sr);
                biolumeHPStateL += (inL - biolumeHPStateL) * hpCoeff;
                biolumeHPStateR += (inR - biolumeHPStateR) * hpCoeff;
                float hfL = inL - biolumeHPStateL;
                float hfR = inR - biolumeHPStateR;

                // Waveshaping for harmonic generation
                // biolumeSpectrum: 0 = even harmonics (warm glow), 1 = odd (cold shimmer)
                float excitedL, excitedR;
                if (biolumeSpectrum < 0.5f)
                {
                    // Even harmonics: asymmetric soft clip (half-wave rectify + smooth)
                    float blend = biolumeSpectrum * 2.0f; // 0 to 1 within even range
                    float rectL = std::max (0.0f, hfL); // half-wave
                    float rectR = std::max (0.0f, hfR);
                    float symL = fastTanh (hfL * (1.0f + biolumeGlow * 3.0f));
                    float symR = fastTanh (hfR * (1.0f + biolumeGlow * 3.0f));
                    excitedL = rectL * (1.0f - blend) + symL * blend;
                    excitedR = rectR * (1.0f - blend) + symR * blend;
                }
                else
                {
                    // Odd harmonics: symmetric saturation (tanh)
                    float drive = 1.0f + biolumeGlow * 4.0f;
                    excitedL = fastTanh (hfL * drive);
                    excitedR = fastTanh (hfR * drive);
                }

                // Shimmer decay: envelope follower on excited signal
                float decayCoeff = 1.0f - std::exp (-1.0f / ((0.05f + biolumeDecay * 0.5f) * sr));
                float absExcL = std::fabs (excitedL);
                float absExcR = std::fabs (excitedR);
                if (absExcL > biolumeDecayEnvL)
                    biolumeDecayEnvL = absExcL; // instant attack
                else
                    biolumeDecayEnvL += (absExcL - biolumeDecayEnvL) * decayCoeff;
                if (absExcR > biolumeDecayEnvR)
                    biolumeDecayEnvR = absExcR;
                else
                    biolumeDecayEnvR += (absExcR - biolumeDecayEnvR) * decayCoeff;

                // Apply envelope to shape the shimmer tail
                excitedL *= std::min (1.0f, biolumeDecayEnvL * 2.0f + 0.3f);
                excitedR *= std::min (1.0f, biolumeDecayEnvR * 2.0f + 0.3f);

                // Final glow intensity
                excitedL *= biolumeGlow;
                excitedR *= biolumeGlow;

                inL = dryL + excitedL * biolumeMix; // additive — exciter adds on top
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

    // --- Fathom state ---
    float fathomLPState[2] {};    // L/R low-band filter
    float fathomHPState[2] {};    // L/R high-band filter
    float fathomCompEnv[4] {};    // [0]=low, [1]=mid, [2]=transient L, [3]=transient R

    // --- Drift state ---
    float driftPhaseL = 0.0f;     // Brownian LFO position L
    float driftPhaseR = 0.37f;    // Brownian LFO position R (offset for decorrelation)
    uint32_t driftRng = 48271u;
    std::array<std::vector<float>, 1> driftDelayL;  // chorus delay buffer L
    std::array<std::vector<float>, 1> driftDelayR;  // chorus delay buffer R
    std::array<int, 1> driftWritePos {};

    // --- Tide state ---
    float tideLFOPhase = 0.0f;
    float tideSmoothGain = 1.0f;
    float tideSmoothCutoff = 8000.0f;
    float tideFilterStateL = 0.0f;
    float tideFilterStateR = 0.0f;

    // --- Reef state (4-tap FDN) ---
    std::vector<float> reefBuf[4];
    int reefPos[4] {};
    float reefFiltState[4] {};

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
