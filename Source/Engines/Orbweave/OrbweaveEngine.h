// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/PolyBLEP.h"
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace xoceanus
{

//==============================================================================
// OrbweaveEngine — The Kelp Knot: Topological Knot-Coupling Synthesis
//
// Phase-braided oscillator synthesis. 4 oscillator "strands" whose phases
// are coupled through a configurable knot-topology matrix. Different knot
// types (Trefoil, Figure-Eight, Torus, Solomon's) produce different braid
// patterns, creating unique timbres impossible with standard detuning.
// The web structure determines the resonant character.
//
// Novel DSP — Knot Phase Coupling:
//   Each strand has a phase accumulator. Each sample, strands influence
//   each other's phase increments through the knot matrix:
//
//     for each strand i:
//       phaseOffset[i] = sum(matrix[i][j] * sin(strand[j].phase * TWO_PI) * braidDepth)
//       strand[i].phase += (strand[i].freq + phaseOffset[i] * couplingScale) / sr
//
//   Matrix coefficients differ per knot type:
//     Trefoil:      Asymmetric 3-strand coupling (strand 4 free)
//     Figure-Eight: Alternating over/under pattern
//     Torus(P,Q):   P wraps around the hole, Q wraps through it
//     Solomon:      Doubly-linked pair of rings
//
// Gallery code: ORBWEAVE | Accent: Kelp Knot Purple #8E4585 | Prefix: weave_
//==============================================================================

//==============================================================================
// OrbweaveADSR / OrbweaveLFO — Migrated to shared DSP utilities (2026-03-21).
// Using aliases preserve all call-sites unchanged.
//==============================================================================
using OrbweaveADSR = StandardADSR;
using OrbweaveLFO = StandardLFO;

//==============================================================================
// OrbweaveFXState — 3 independent FX slots (Delay / Chorus / Reverb)
//==============================================================================
struct OrbweaveFXState
{
    std::vector<float> delayBufL, delayBufR;
    int delayWritePos = 0;
    std::vector<float> chorusBufL, chorusBufR;
    int chorusWritePos = 0;
    float chorusLFOPhase = 0.0f;
    std::vector<float> reverbBuf[4];
    int reverbPos[4]{};
    float reverbFilt[4]{};

    void prepare(float sr)
    {
        int maxDelay = static_cast<int>(sr * 0.5f) + 1;
        delayBufL.assign(static_cast<size_t>(maxDelay), 0.0f);
        delayBufR.assign(static_cast<size_t>(maxDelay), 0.0f);
        delayWritePos = 0;
        int chorusMax = static_cast<int>(sr * 0.03f) + 16;
        chorusBufL.assign(static_cast<size_t>(chorusMax), 0.0f);
        chorusBufR.assign(static_cast<size_t>(chorusMax), 0.0f);
        chorusWritePos = 0;
        chorusLFOPhase = 0.0f;
        float srScale = sr / 44100.0f;
        static constexpr int kRevLens[4] = {1087, 1283, 1511, 1789};
        for (int i = 0; i < 4; ++i)
        {
            int len = static_cast<int>(static_cast<float>(kRevLens[i]) * srScale) + 1;
            reverbBuf[i].assign(static_cast<size_t>(len), 0.0f);
            reverbPos[i] = 0;
            reverbFilt[i] = 0.0f;
        }
    }

    void reset()
    {
        std::fill(delayBufL.begin(), delayBufL.end(), 0.0f);
        std::fill(delayBufR.begin(), delayBufR.end(), 0.0f);
        delayWritePos = 0;
        std::fill(chorusBufL.begin(), chorusBufL.end(), 0.0f);
        std::fill(chorusBufR.begin(), chorusBufR.end(), 0.0f);
        chorusWritePos = 0;
        for (int i = 0; i < 4; ++i)
        {
            std::fill(reverbBuf[i].begin(), reverbBuf[i].end(), 0.0f);
            reverbFilt[i] = 0.0f;
        }
    }
};

//==============================================================================
// Knot Topology Matrices — the soul of the engine
//==============================================================================
enum class KnotType
{
    Trefoil = 0,
    FigureEight,
    Torus,
    Solomon,
    kCount
};

// 4x4 coupling matrices. Row i, column j = how much strand j's phase
// modulates strand i's phase increment. Diagonal is always 0 (no self-coupling).
// Values are design constants that define each knot's sonic fingerprint.
struct KnotMatrices
{
    // Trefoil: asymmetric 3-strand ring, strand 4 floats free.
    // The trefoil has a writhe of +3, so coupling flows in one direction around the loop.
    static constexpr float trefoil[4][4] = {
        {0.0f, 0.7f, 0.0f, -0.3f}, // strand 0 ← strand 1 (strong), strand 3 (weak inverse)
        {0.0f, 0.0f, 0.7f, 0.0f},  // strand 1 ← strand 2
        {0.7f, 0.0f, 0.0f, 0.0f},  // strand 2 ← strand 0 (completes the ring)
        {0.2f, 0.0f, 0.0f, 0.0f}   // strand 3 ← strand 0 (gentle tether)
    };

    // Figure-Eight: alternating over/under braid pattern (all 4 strands).
    // The figure-eight knot has crossing number 4 — each strand crosses two others.
    static constexpr float figureEight[4][4] = {
        {0.0f, 0.6f, -0.4f, 0.0f}, // over 1, under 2
        {0.0f, 0.0f, 0.6f, -0.4f}, // over 2, under 3
        {-0.4f, 0.0f, 0.0f, 0.6f}, // under 0, over 3
        {0.6f, -0.4f, 0.0f, 0.0f}  // over 0, under 1
    };

    // Torus: P wraps around the hole, Q wraps through it.
    // Base coupling with dynamic P/Q modulation applied in DSP.
    static constexpr float torus[4][4] = {{0.0f, 0.5f, 0.0f, 0.5f}, // linked to adjacent strands on torus surface
                                          {0.5f, 0.0f, 0.5f, 0.0f},
                                          {0.0f, 0.5f, 0.0f, 0.5f},
                                          {0.5f, 0.0f, 0.5f, 0.0f}};

    // Solomon: two doubly-linked rings (strands 0-1 form ring A, strands 2-3 form ring B).
    // The Solomon's knot has linking number 2 — rings pass through each other twice.
    static constexpr float solomon[4][4] = {
        {0.0f, 0.8f, 0.3f, 0.0f}, // Ring A: 0↔1 strong, 0→2 cross-link
        {0.8f, 0.0f, 0.0f, 0.3f}, // Ring A: 1↔0 strong, 1→3 cross-link
        {0.3f, 0.0f, 0.0f, 0.8f}, // Ring B: 2→0 cross-link, 2↔3 strong
        {0.0f, 0.3f, 0.8f, 0.0f}  // Ring B: 3→1 cross-link, 3↔2 strong
    };
};

//==============================================================================
// LFO Target enum for weave_ LFO slots
//==============================================================================
enum class WeaveLFOTarget
{
    None = 0,
    Pitch,
    FilterCutoff,
    FilterReso,
    Volume,
    BraidDepth,
    kCount
};

//==============================================================================
// OrbweaveVoice — 4 oscillator strands per voice
//==============================================================================
struct OrbweaveVoice
{
    bool active = false;
    int note = -1;
    float velocity = 0.0f;
    float aftertouch = 0.0f;
    uint64_t startTime = 0;

    // 4 oscillator strands
    PolyBLEP strandOsc[4];  // for PolyBLEP waveforms (saw/square/triangle)
    float strandPhase[4]{}; // manual phase accumulators (for sine + coupling reads)
    float strandFreq[4]{};  // base frequencies
    float targetFreq[4]{};  // portamento targets

    // Amplitude envelope
    OrbweaveADSR ampEnv;

    // 2 LFO slots
    OrbweaveLFO lfos[2];

    // Filter
    CytomicSVF filter;

    // Pan
    float pan = 0.0f;

    // Voice-steal crossfade (5ms fade-out of stolen voice amplitude)
    float stealFadeGain = 0.0f; // 1.0 → 0.0 ramp applied to voice output on steal
    float stealFadeStep = 0.0f; // decrement per sample = 1 / (0.005 * sr)

    void reset() noexcept
    {
        active = false;
        note = -1;
        velocity = 0.0f;
        aftertouch = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            strandOsc[i].reset();
            strandPhase[i] = 0.0f;
            strandFreq[i] = 440.0f;
            targetFreq[i] = 440.0f;
        }
        ampEnv.reset();
        for (auto& l : lfos)
            l.reset();
        filter.reset();
        pan = 0.0f;
        stealFadeGain = 0.0f;
        stealFadeStep = 0.0f;
    }
};

//==============================================================================
// OrbweaveEngine
//==============================================================================
class OrbweaveEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = static_cast<float>(sampleRate);
        for (auto& v : voices)
            v.reset();
        for (auto& fx : fxSlots)
            fx.prepare(sr);
        matrixDirty = true; // invalidate cached matrix on sample-rate change
        // SRO SilenceGate: phase-braided oscillators with reverb FX — 500ms hold
        prepareSilenceGate(sampleRate, maxBlockSize, 500.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();
        for (auto& fx : fxSlots)
            fx.reset();
        activeVoices.store(0, std::memory_order_relaxed);
        lastSampleL = lastSampleR = 0.0f;
        matrixDirty = true; // invalidate cached matrix on reset
        couplingPitchMod = 0.0f;
        couplingCutoffMod = 0.0f;
    }

    //==========================================================================
    // Audio — Phase-Braided Oscillator Synthesis
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0 || sr <= 0.0f) // guard: sr must be set by prepare()
            return;

        // SRO SilenceGate: wake on note-on, bypass when silent
        for (const auto& md : midi)
            if (md.getMessage().isNoteOn())
            {
                wakeSilenceGate();
                break;
            }
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // === ParamSnapshot: cache all parameters once per block ===

        // Strand type (shared — all 4 strands use same waveform for coherent braiding)
        const auto strandType = static_cast<int>(loadP(pStrandType, 1.0f));
        const float strandTune = loadP(pStrandTune, 0.0f); // fine tune for strand pair offset

        // Knot topology
        const auto knotType = static_cast<int>(loadP(pKnotType, 0.0f));
        const float braidDepth = loadP(pBraidDepth, 0.2f); // matches param declaration default
        const float torusP = loadP(pTorusP, 2.0f);
        const float torusQ = loadP(pTorusQ, 3.0f);

        // Filter
        const float filterCutoff = loadP(pFilterCutoff, 8000.0f);
        const float filterReso = loadP(pFilterReso, 0.0f);
        const auto filterType = static_cast<int>(loadP(pFilterType, 0.0f));

        // ADSR
        const float ampA = loadP(pAmpA, 0.01f);
        const float ampD = loadP(pAmpD, 0.3f);
        const float ampS = loadP(pAmpS, 0.7f);
        const float ampR = loadP(pAmpR, 0.5f);

        // LFOs (2 slots)
        const int lfoType[2] = {static_cast<int>(loadP(pLfoType[0], 0.0f)), static_cast<int>(loadP(pLfoType[1], 0.0f))};
        const int lfoTarget[2] = {static_cast<int>(loadP(pLfoTarget[0], 0.0f)),
                                  static_cast<int>(loadP(pLfoTarget[1], 0.0f))};
        const float lfoDepth[2] = {loadP(pLfoDepth[0], 0.0f), loadP(pLfoDepth[1], 0.0f)};
        const float lfoRate[2] = {loadP(pLfoRate[0], 1.0f), loadP(pLfoRate[1], 1.0f)};

        // FX (3 slots)
        const int fxType[3] = {static_cast<int>(loadP(pFxType[0], 0.0f)), static_cast<int>(loadP(pFxType[1], 0.0f)),
                               static_cast<int>(loadP(pFxType[2], 0.0f))};
        const float fxMix[3] = {loadP(pFxMix[0], 0.0f), loadP(pFxMix[1], 0.0f), loadP(pFxMix[2], 0.0f)};
        const float fxParam[3] = {loadP(pFxParam[0], 0.3f), loadP(pFxParam[1], 0.3f), loadP(pFxParam[2], 0.3f)};

        // Macros
        const float macroWeave = loadP(pMacroWeave, 0.0f);
        const float macroTension = loadP(pMacroTension, 0.0f);
        const float macroKnot = loadP(pMacroKnot, 0.0f);
        const float macroSpace = loadP(pMacroSpace, 0.0f);

        // Level + expression
        const float level = loadP(pLevel, 0.8f);
        const float bendRange = loadP(pPitchBendRange, 2.0f);
        const float glideTime = loadP(pGlideTime, 0.0f);

        const int voiceModeIdx = static_cast<int>(loadP(pVoiceMode, 3.0f));
        const int polyLimit = (voiceModeIdx <= 1) ? 1 : (voiceModeIdx == 2) ? 4 : 8;
        polyLimit_ = polyLimit;

        const float glideCoeff = (glideTime > 0.001f) ? 1.0f - std::exp(-1.0f / (glideTime * sr)) : 1.0f;

        // === MACRO WEAVE: modulates braid depth ===
        const float effBraidDepth = clamp(braidDepth + macroWeave * (1.0f - braidDepth), 0.0f, 1.0f);

        // === MACRO TENSION: filter resonance boost + coupling feedback ===
        const float tensionReso = macroTension * 0.4f;

        // === MACRO KNOT: morphs between knot types (smooth interpolation) ===
        // macroKnot 0→1 blends current knot toward the next knot type.
        // knotB wraps to (knotType+1)%4 — the adjacent knot in the enum.
        const int knotA = knotType;
        const int knotB = (knotType + 1) % 4;
        const float knotMorph = macroKnot;

        // Rebuild effective coupling matrix only when topology parameters change
        // (avoids 48-float blend + sin each block when knot is static).
        if (matrixDirty || knotA != cachedKnotA || knotB != cachedKnotB
            || knotMorph != cachedKnotMorph || torusP != cachedTorusP || torusQ != cachedTorusQ)
        {
            buildEffectiveMatrix(knotA, knotB, knotMorph, torusP, torusQ, cachedMatrix);
            cachedKnotA = knotA; cachedKnotB = knotB; cachedKnotMorph = knotMorph;
            cachedTorusP = torusP; cachedTorusQ = torusQ;
            matrixDirty = false;
        }
        const float (&matrix)[4][4] = cachedMatrix;

        // Coupling scale: controls the frequency range of phase influence
        // Higher values = more dramatic timbral effect
        static constexpr float kCouplingScale = 200.0f; // Hz range of phase coupling

        // === MIDI ===
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), ampA, ampD, ampS, ampR, strandTune, lfoType,
                       lfoRate, voiceModeIdx, glideCoeff);
            }
            else if (msg.isNoteOff())
            {
                noteOff(msg.getNoteNumber());
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
            else if (msg.isChannelPressure())
            {
                for (auto& v : voices)
                    if (v.active)
                        v.aftertouch = msg.getChannelPressureValue() / 127.0f;
            }
            else if (msg.isPitchWheel())
                pitchBend_ = (msg.getPitchWheelValue() - 8192) / 8192.0f;
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheel_ = msg.getControllerValue() / 127.0f;
        }

        // === Consume coupling accumulators ===
        float blockPitchCoupling = couplingPitchMod;
        couplingPitchMod = 0.0f;
        float blockCutoffCoupling = couplingCutoffMod;
        couplingCutoffMod = 0.0f;

        // Hoist LFO rate/shape and filter mode configuration out of the per-sample loop.
        // Rate coefficient and filter mode are parameter-rate quantities; setting them
        // once per block per voice is correct and avoids redundant work per sample.
        const CytomicSVF::Mode blockFilterMode = [&]() -> CytomicSVF::Mode {
            switch (filterType)
            {
            case 1:  return CytomicSVF::Mode::HighPass;
            case 2:  return CytomicSVF::Mode::BandPass;
            default: return CytomicSVF::Mode::LowPass;
            }
        }();

        for (int vi = 0; vi < kMaxVoices; ++vi)
        {
            auto& voice = voices[vi];
            if (!voice.active)
                continue;
            voice.filter.setMode(blockFilterMode); // once per block, not per sample
            for (int l = 0; l < 2; ++l)
            {
                if (lfoType[l] == 0)
                    continue;
                voice.lfos[l].shape = lfoType[l] - 1;
                voice.lfos[l].setRate(lfoRate[l], sr);
            }
        }

        // === Sample Loop ===
        for (int s = 0; s < numSamples; ++s)
        {
            float mixL = 0.0f, mixR = 0.0f;

            for (int vi = 0; vi < kMaxVoices; ++vi)
            {
                auto& voice = voices[vi];
                if (!voice.active)
                    continue;

                // --- Portamento ---
                for (int i = 0; i < 4; ++i)
                    voice.strandFreq[i] += (voice.targetFreq[i] - voice.strandFreq[i]) * glideCoeff;

                // --- LFO processing ---
                float pitchMod = 0.0f, cutoffMod = 0.0f, resoMod = 0.0f;
                float volMod = 0.0f, braidMod = 0.0f;

                for (int l = 0; l < 2; ++l)
                {
                    if (lfoType[l] == 0)
                        continue; // Off
                    // shape and setRate already applied once per block above
                    float lfoVal = voice.lfos[l].process() * lfoDepth[l];

                    switch (lfoTarget[l])
                    {
                    case 1:
                        pitchMod += lfoVal * 1200.0f;
                        break; // cents
                    case 2:
                        cutoffMod += lfoVal * 6000.0f;
                        break; // Hz
                    case 3:
                        resoMod += lfoVal;
                        break;
                    case 4:
                        volMod += lfoVal;
                        break;
                    case 5:
                        braidMod += lfoVal;
                        break;
                    default:
                        break;
                    }
                }

                // D001: Velocity shapes timbre (filter cutoff). Scaled to 1500 Hz
                // so a hard-pressed note at max cutoff (8 kHz default) opens to
                // ~9.5 kHz — expressive but leaves headroom before the 20 kHz wall.
                float velTimbre = voice.velocity * 1500.0f;

                // D006: Mod wheel → filter cutoff sweep
                cutoffMod += modWheel_ * 4000.0f;

                // D006: Aftertouch → braid depth modulation
                braidMod += voice.aftertouch * 0.3f;

                // Coupling modulation
                pitchMod += blockPitchCoupling * 100.0f;
                cutoffMod += blockCutoffCoupling * 3000.0f;

                // Pitch bend
                pitchMod += pitchBend_ * bendRange * 100.0f;

                // TENSION macro → resonance
                resoMod += tensionReso;

                // === THE KNOT PHASE COUPLING — the novel DSP ===

                // Effective braid depth with LFO/aftertouch modulation
                float currentBraid = clamp(effBraidDepth + braidMod, 0.0f, 1.0f);

                // Precompute sin(phase * 2π) for each strand once per sample
                // so the 4×4 coupling matrix loop reuses them (4 calls vs 16).
                float sinPhase[4];
                for (int j = 0; j < 4; ++j)
                    sinPhase[j] = fastSin(voice.strandPhase[j] * kTwoPi);

                // Compute phase coupling offsets from the knot matrix
                float phaseOffset[4]{};
                for (int i = 0; i < 4; ++i)
                {
                    for (int j = 0; j < 4; ++j)
                    {
                        if (i == j)
                            continue;
                        phaseOffset[i] += matrix[i][j] * sinPhase[j] * currentBraid;
                    }
                }

                // Generate each strand's output and advance phase with coupling
                float strandOut[4]{};
                for (int i = 0; i < 4; ++i)
                {
                    float freq = voice.strandFreq[i] * fastPow2(pitchMod / 1200.0f);
                    float coupledFreq = freq + phaseOffset[i] * kCouplingScale;

                    // Clamp to prevent negative or extreme frequencies
                    coupledFreq = clamp(coupledFreq, 1.0f, sr * 0.49f);

                    strandOut[i] = renderStrand(strandType, voice, i, coupledFreq);
                }

                // Mix all 4 strands equally
                float signal = (strandOut[0] + strandOut[1] + strandOut[2] + strandOut[3]) * 0.25f;

                // === Filter ===
                // Filter mode was set once per block above (blockFilterMode).
                // setCoefficients() is called per-sample because cutoffMod varies
                // with LFO output; mode is param-rate only.
                float effCut = clamp(filterCutoff + cutoffMod + velTimbre, 20.0f, 20000.0f);
                float effRes = clamp(filterReso + resoMod, 0.0f, 1.0f);
                voice.filter.setCoefficients(effCut, effRes, sr);
                signal = voice.filter.processSample(signal);

                // === Amp envelope ===
                float ampLevel = voice.ampEnv.process();
                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // volMod is bipolar [-1, +1]; clamp effective sum so LFO at full
                // depth can silence (mod=-1→gain=0) but not hard-clip above 1.0×.
                float gain = ampLevel * voice.velocity * clamp(1.0f + volMod, 0.0f, 1.0f);
                signal *= gain;
                signal = flushDenormal(signal);

                // Voice-steal crossfade: multiply output by stealFadeGain (1→0 over 5ms)
                // while the incoming note's envelope ramps up, avoiding a hard click.
                if (voice.stealFadeGain > 0.0f)
                {
                    signal *= voice.stealFadeGain;
                    voice.stealFadeGain -= voice.stealFadeStep;
                    if (voice.stealFadeGain < 0.0f)
                        voice.stealFadeGain = 0.0f;
                }

                // Panning
                float effPan = clamp(voice.pan, -1.0f, 1.0f);
                float panL = 0.5f - effPan * 0.5f;
                float panR = 0.5f + effPan * 0.5f;
                mixL += signal * panL;
                mixR += signal * panR;
            }

            // --- FX chain (FX1 → FX2 → FX3 in series) ---
            for (int fx = 0; fx < 3; ++fx)
            {
                if (fxType[fx] > 0)
                {
                    float effMix = clamp(fxMix[fx] + macroSpace * (1.0f - fxMix[fx]), 0.0f, 1.0f);
                    if (effMix > 0.001f)
                        applyEffect(fxType[fx], mixL, mixR, effMix, fxParam[fx], macroSpace, fxSlots[fx]);
                }
            }

            mixL *= level;
            mixR *= level;

            // Store for coupling output
            lastSampleL = mixL;
            lastSampleR = mixR;

            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample(0, s, mixL);
                buffer.addSample(1, s, mixR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample(0, s, (mixL + mixR) * 0.5f);
            }
        }

        int count = 0;
        for (const auto& v : voices)
            if (v.active)
                ++count;
        activeVoices.store(count, std::memory_order_relaxed);

        // SRO SilenceGate: feed output to the gate for silence detection
        analyzeForSilenceGate(buffer, numSamples);
    }

    //==========================================================================
    // Coupling
    //==========================================================================

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        if (channel == 0)
            return lastSampleL;
        if (channel == 1)
            return lastSampleR;
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount, const float*, int) override
    {
        switch (type)
        {
        case CouplingType::AudioToFM:
            couplingPitchMod += amount * 0.5f;
            break;
        case CouplingType::AmpToFilter:
            couplingCutoffMod += amount;
            break;
        case CouplingType::LFOToPitch:
            couplingPitchMod += amount * 0.5f;
            break;
        case CouplingType::AmpToPitch:
            couplingPitchMod += amount * 0.3f;
            break;
        case CouplingType::AmpToChoke:
            for (auto& v : voices)
                if (v.active)
                    v.ampEnv.noteOff();
            break;
        case CouplingType::KnotTopology:
            // KnotTopology: bidirectional braid coupling. The incoming `amount`
            // encodes linking number (1-5) scaled to [0.25, 1.25]. Modulates
            // both pitch (oscillator entanglement) and braid cutoff simultaneously,
            // producing the mutual co-evolution the spec describes.
            couplingPitchMod += amount * 0.4f;
            couplingCutoffMod += amount * 0.6f;
            matrixDirty = true; // force matrix rebuild — topology has changed
            break;
        default:
            break;
        }
    }

    //==========================================================================
    // Parameters (~33 total)
    //==========================================================================
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PC = juce::AudioParameterChoice;

        auto strandChoices = juce::StringArray{"Off", "Sine", "Saw", "Square", "Triangle"};
        auto knotChoices = juce::StringArray{"Trefoil", "Figure-Eight", "Torus", "Solomon"};
        auto filterChoices = juce::StringArray{"Low Pass", "High Pass", "Band Pass"};
        auto lfoShapes = juce::StringArray{"Off", "Sine", "Triangle", "Saw", "Square", "S&H"};
        auto lfoTargets = juce::StringArray{"None", "Pitch", "Filter Cutoff", "Filter Reso", "Volume", "Braid Depth"};
        auto fxChoices = juce::StringArray{"Off", "Delay", "Chorus", "Reverb"};
        auto voiceChoices = juce::StringArray{"Mono", "Legato", "Poly4", "Poly8"};

        // Strand waveform + tuning (2)
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"weave_strandType", 1}, "Orbweave Strand Type", strandChoices, 1));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_strandTune", 1}, "Orbweave Strand Tune",
                                              juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));

        // Knot topology (4)
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"weave_knotType", 1}, "Orbweave Knot Type", knotChoices, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_braidDepth", 1}, "Orbweave Braid Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.2f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_torusP", 1}, "Orbweave Torus P",
                                              juce::NormalisableRange<float>(1.0f, 8.0f, 1.0f), 2.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_torusQ", 1}, "Orbweave Torus Q",
                                              juce::NormalisableRange<float>(1.0f, 8.0f, 1.0f), 3.0f));

        // Filter (3)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_filterCutoff", 1}, "Orbweave Filter Cutoff",
                                              juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 8000.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_filterReso", 1}, "Orbweave Filter Reso",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"weave_filterType", 1}, "Orbweave Filter Type", filterChoices, 0));

        // Amp ADSR (4)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_ampA", 1}, "Orbweave Amp Attack",
                                              juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.01f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_ampD", 1}, "Orbweave Amp Decay",
                                              juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_ampS", 1}, "Orbweave Amp Sustain",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_ampR", 1}, "Orbweave Amp Release",
                                              juce::NormalisableRange<float>(0.0f, 20.0f, 0.001f, 0.3f), 0.5f));

        // LFO 1 (4)
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"weave_lfo1Type", 1}, "Orbweave LFO 1 Type", lfoShapes, 0));
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"weave_lfo1Target", 1}, "Orbweave LFO 1 Target", lfoTargets, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_lfo1Depth", 1}, "Orbweave LFO 1 Depth",
                                              juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_lfo1Rate", 1}, "Orbweave LFO 1 Rate",
                                              juce::NormalisableRange<float>(0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        // LFO 2 (4)
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"weave_lfo2Type", 1}, "Orbweave LFO 2 Type", lfoShapes, 0));
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"weave_lfo2Target", 1}, "Orbweave LFO 2 Target", lfoTargets, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_lfo2Depth", 1}, "Orbweave LFO 2 Depth",
                                              juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_lfo2Rate", 1}, "Orbweave LFO 2 Rate",
                                              juce::NormalisableRange<float>(0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        // FX slots (9: 3 x 3)
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"weave_fx1Type", 1}, "Orbweave FX 1 Type", fxChoices, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_fx1Mix", 1}, "Orbweave FX 1 Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_fx1Param", 1}, "Orbweave FX 1 Param",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"weave_fx2Type", 1}, "Orbweave FX 2 Type", fxChoices, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_fx2Mix", 1}, "Orbweave FX 2 Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_fx2Param", 1}, "Orbweave FX 2 Param",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"weave_fx3Type", 1}, "Orbweave FX 3 Type", fxChoices, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_fx3Mix", 1}, "Orbweave FX 3 Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_fx3Param", 1}, "Orbweave FX 3 Param",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        // Macros (4): WEAVE, TENSION, KNOT, SPACE
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_macroWeave", 1}, "Orbweave Macro WEAVE",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_macroTension", 1}, "Orbweave Macro TENSION",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_macroKnot", 1}, "Orbweave Macro KNOT",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_macroSpace", 1}, "Orbweave Macro SPACE",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        // Level (1)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_level", 1}, "Orbweave Level",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

        // Voice mode (1)
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"weave_voiceMode", 1}, "Orbweave Voice Mode", voiceChoices, 3));

        // Expression (2)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_pitchBendRange", 1}, "Orbweave Pitch Bend Range",
                                              juce::NormalisableRange<float>(1.0f, 24.0f, 1.0f), 2.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"weave_glideTime", 1}, "Orbweave Glide Time",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.3f), 0.0f));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pStrandType = apvts.getRawParameterValue("weave_strandType");
        pStrandTune = apvts.getRawParameterValue("weave_strandTune");

        pKnotType = apvts.getRawParameterValue("weave_knotType");
        pBraidDepth = apvts.getRawParameterValue("weave_braidDepth");
        pTorusP = apvts.getRawParameterValue("weave_torusP");
        pTorusQ = apvts.getRawParameterValue("weave_torusQ");

        pFilterCutoff = apvts.getRawParameterValue("weave_filterCutoff");
        pFilterReso = apvts.getRawParameterValue("weave_filterReso");
        pFilterType = apvts.getRawParameterValue("weave_filterType");

        pAmpA = apvts.getRawParameterValue("weave_ampA");
        pAmpD = apvts.getRawParameterValue("weave_ampD");
        pAmpS = apvts.getRawParameterValue("weave_ampS");
        pAmpR = apvts.getRawParameterValue("weave_ampR");

        const char* lfoIds[2][4] = {{"weave_lfo1Type", "weave_lfo1Target", "weave_lfo1Depth", "weave_lfo1Rate"},
                                    {"weave_lfo2Type", "weave_lfo2Target", "weave_lfo2Depth", "weave_lfo2Rate"}};
        for (int i = 0; i < 2; ++i)
        {
            pLfoType[i] = apvts.getRawParameterValue(lfoIds[i][0]);
            pLfoTarget[i] = apvts.getRawParameterValue(lfoIds[i][1]);
            pLfoDepth[i] = apvts.getRawParameterValue(lfoIds[i][2]);
            pLfoRate[i] = apvts.getRawParameterValue(lfoIds[i][3]);
        }

        const char* fxIds[3][3] = {{"weave_fx1Type", "weave_fx1Mix", "weave_fx1Param"},
                                   {"weave_fx2Type", "weave_fx2Mix", "weave_fx2Param"},
                                   {"weave_fx3Type", "weave_fx3Mix", "weave_fx3Param"}};
        for (int i = 0; i < 3; ++i)
        {
            pFxType[i] = apvts.getRawParameterValue(fxIds[i][0]);
            pFxMix[i] = apvts.getRawParameterValue(fxIds[i][1]);
            pFxParam[i] = apvts.getRawParameterValue(fxIds[i][2]);
        }

        pMacroWeave = apvts.getRawParameterValue("weave_macroWeave");
        pMacroTension = apvts.getRawParameterValue("weave_macroTension");
        pMacroKnot = apvts.getRawParameterValue("weave_macroKnot");
        pMacroSpace = apvts.getRawParameterValue("weave_macroSpace");

        pLevel = apvts.getRawParameterValue("weave_level");
        pVoiceMode = apvts.getRawParameterValue("weave_voiceMode");
        pPitchBendRange = apvts.getRawParameterValue("weave_pitchBendRange");
        pGlideTime = apvts.getRawParameterValue("weave_glideTime");
    }

    //==========================================================================
    // Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Orbweave"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF8E4585); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoices.load(std::memory_order_relaxed); }

private:
    static constexpr float kTwoPi = 6.28318530717958647692f;
    static constexpr float kPi = 3.14159265358979323846f;

    static float loadP(std::atomic<float>* p, float fb) noexcept { return p ? p->load() : fb; }

    //==========================================================================
    // Strand rendering — PolyBLEP for anti-aliased types, manual phase for sine
    //==========================================================================
    float renderStrand(int type, OrbweaveVoice& voice, int idx, float freq) noexcept
    {
        float dt = freq / sr;

        switch (type)
        {
        case 1: // Sine — manual phase (coupling reads strandPhase directly)
        {
            float out = fastSin(voice.strandPhase[idx] * kTwoPi);
            voice.strandPhase[idx] += dt;
            if (voice.strandPhase[idx] >= 1.0f)
                voice.strandPhase[idx] -= 1.0f;
            if (voice.strandPhase[idx] < 0.0f)
                voice.strandPhase[idx] += 1.0f;
            return out;
        }
        case 2: // Saw — PolyBLEP
        {
            voice.strandOsc[idx].setFrequency(freq, sr);
            voice.strandOsc[idx].setWaveform(PolyBLEP::Waveform::Saw);
            float out = voice.strandOsc[idx].processSample();
            // Keep manual phase in sync for coupling reads
            voice.strandPhase[idx] += dt;
            if (voice.strandPhase[idx] >= 1.0f)
                voice.strandPhase[idx] -= 1.0f;
            return out;
        }
        case 3: // Square — PolyBLEP
        {
            voice.strandOsc[idx].setFrequency(freq, sr);
            voice.strandOsc[idx].setWaveform(PolyBLEP::Waveform::Square);
            float out = voice.strandOsc[idx].processSample();
            voice.strandPhase[idx] += dt;
            if (voice.strandPhase[idx] >= 1.0f)
                voice.strandPhase[idx] -= 1.0f;
            return out;
        }
        case 4: // Triangle — PolyBLEP
        {
            voice.strandOsc[idx].setFrequency(freq, sr);
            voice.strandOsc[idx].setWaveform(PolyBLEP::Waveform::Triangle);
            float out = voice.strandOsc[idx].processSample();
            voice.strandPhase[idx] += dt;
            if (voice.strandPhase[idx] >= 1.0f)
                voice.strandPhase[idx] -= 1.0f;
            return out;
        }
        default:
            return 0.0f;
        }
    }

    //==========================================================================
    // Build effective coupling matrix from knot type blend + Torus P/Q
    //==========================================================================
    static void buildEffectiveMatrix(int knotA, int knotB, float morph, float torusP, float torusQ,
                                     float out[4][4]) noexcept
    {
        const float (*matA)[4] = getKnotMatrix(knotA);
        const float (*matB)[4] = getKnotMatrix(knotB);

        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                out[i][j] = matA[i][j] * (1.0f - morph) + matB[i][j] * morph;
            }
        }

        // Apply Torus P/Q modulation: when either source or dest knot is Torus,
        // scale the coupling coefficients by harmonic relationships derived from P and Q.
        // P/Q creates a torus knot winding number that warps the coupling strengths.
        bool hasTorus = (knotA == static_cast<int>(KnotType::Torus)) || (knotB == static_cast<int>(KnotType::Torus));
        if (hasTorus)
        {
            // Torus winding ratio modulates the inter-strand coupling asymmetry.
            // P controls how many times the strand wraps around the torus hole (meridionally).
            // Q controls longitudinal wraps. Different P/Q combos create different knot families:
            //   (2,3) = trefoil  (2,5) = cinquefoil  (3,5) = complex
            float pqRatio = torusP / std::max(1.0f, torusQ);
            float pqScale = 0.5f + 0.5f * fastSin(pqRatio * kPi);

            // Apply P/Q modulation asymmetrically — even strands get P-scaled,
            // odd strands get Q-scaled, creating the torus winding geometry
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    if (i == j)
                        continue;
                    float scale = ((i + j) % 2 == 0) ? pqScale : (1.0f - pqScale * 0.5f);
                    out[i][j] *= scale;
                }
            }
        }
    }

    //==========================================================================
    static const float (*getKnotMatrix(int type))[4]
    {
        switch (type)
        {
        case 0:
            return KnotMatrices::trefoil;
        case 1:
            return KnotMatrices::figureEight;
        case 2:
            return KnotMatrices::torus;
        case 3:
            return KnotMatrices::solomon;
        default:
            return KnotMatrices::trefoil;
        }
    }

    //==========================================================================
    // FX processing — identical to OBRIX pattern
    //==========================================================================
    void applyEffect(int type, float& L, float& R, float mix, float param, float space, OrbweaveFXState& fx) noexcept
    {
        float dryL = L, dryR = R;
        float reverbDamp = 0.1f + param * 0.4f;

        switch (type)
        {
        case 1: // Delay
        {
            if (fx.delayBufL.empty())
                break;
            int bufSz = static_cast<int>(fx.delayBufL.size());
            int delaySamples = static_cast<int>((0.05f + param * 0.45f + space * 0.2f) * sr);
            delaySamples = std::max(1, std::min(delaySamples, bufSz - 1));
            int readPos = (fx.delayWritePos - delaySamples + bufSz) % bufSz;
            float wetL = fx.delayBufL[static_cast<size_t>(readPos)];
            float wetR = fx.delayBufR[static_cast<size_t>(readPos)];
            fx.delayBufL[static_cast<size_t>(fx.delayWritePos)] = flushDenormal(L + wetL * (param * 0.7f));
            fx.delayBufR[static_cast<size_t>(fx.delayWritePos)] = flushDenormal(R + wetR * (param * 0.7f));
            fx.delayWritePos = (fx.delayWritePos + 1) % bufSz;
            L = dryL * (1.0f - mix) + wetL * mix;
            R = dryR * (1.0f - mix) + wetR * mix;
            break;
        }
        case 2: // Chorus
        {
            if (fx.chorusBufL.empty())
                break;
            int bufSz = static_cast<int>(fx.chorusBufL.size());
            float depth = 0.002f * sr * (0.3f + param * 0.7f);
            float centerDelay = 0.007f * sr;
            fx.chorusLFOPhase += (0.3f + space * 2.0f) / sr;
            if (fx.chorusLFOPhase >= 1.0f)
                fx.chorusLFOPhase -= 1.0f;
            float lfoVal = fastSin(fx.chorusLFOPhase * kTwoPi);
            float delayL = centerDelay + lfoVal * depth;
            float delayR = centerDelay - lfoVal * depth;
            fx.chorusBufL[static_cast<size_t>(fx.chorusWritePos)] = L;
            fx.chorusBufR[static_cast<size_t>(fx.chorusWritePos)] = R;
            auto readInterp = [&](const std::vector<float>& buf, float del)
            {
                float rp = static_cast<float>(fx.chorusWritePos) - del;
                if (rp < 0.0f)
                    rp += static_cast<float>(bufSz);
                int i0 = static_cast<int>(rp) % bufSz;
                float frac = rp - std::floor(rp);
                return buf[static_cast<size_t>(i0)] * (1.0f - frac) + buf[static_cast<size_t>((i0 + 1) % bufSz)] * frac;
            };
            float wL = readInterp(fx.chorusBufL, delayL);
            float wR = readInterp(fx.chorusBufR, delayR);
            fx.chorusWritePos = (fx.chorusWritePos + 1) % bufSz;
            L = dryL * (1.0f - mix) + wL * mix;
            R = dryR * (1.0f - mix) + wR * mix;
            break;
        }
        case 3: // Reverb (4-tap FDN)
        {
            float input = (L + R) * 0.5f * (0.5f + space * 0.5f);
            float tap[4];
            for (int i = 0; i < 4; ++i)
            {
                int len = static_cast<int>(fx.reverbBuf[i].size());
                if (len == 0)
                {
                    tap[i] = 0.0f;
                    continue;
                }
                int readOff = static_cast<int>((0.3f + param * 0.7f) * static_cast<float>(len));
                readOff = std::max(1, std::min(readOff, len - 1));
                int rp = (fx.reverbPos[i] - readOff + len) % len;
                tap[i] = fx.reverbBuf[i][static_cast<size_t>(rp)];
                fx.reverbFilt[i] = flushDenormal(fx.reverbFilt[i] + (tap[i] - fx.reverbFilt[i]) * reverbDamp);
                tap[i] = fx.reverbFilt[i];
            }
            float tapSum = tap[0] + tap[1] + tap[2] + tap[3];
            float fb = 0.3f + param * 0.5f;
            for (int i = 0; i < 4; ++i)
            {
                float fbSample = fastTanh((tap[i] - 0.5f * tapSum) * fb + input);
                int len = static_cast<int>(fx.reverbBuf[i].size());
                if (len > 0)
                    fx.reverbBuf[i][static_cast<size_t>(fx.reverbPos[i])] = flushDenormal(fbSample);
                fx.reverbPos[i] = (fx.reverbPos[i] + 1) % std::max(1, len);
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
    // Voice management
    //==========================================================================
    void noteOn(int noteNum, float vel, float ampA, float ampD, float ampS, float ampR, float strandTune,
                const int lfoTypes[2], const float lfoRates[2], int voiceMode, float glideCoeff)
    {
        bool isLegato = (voiceMode == 1);
        int maxVoicesNow = std::min(kMaxVoices, polyLimit_);

        int slot = -1;
        uint64_t oldest = UINT64_MAX;
        int oldestSlot = 0;

        if (isLegato)
        {
            for (int i = 0; i < maxVoicesNow; ++i)
                if (voices[i].active)
                {
                    slot = i;
                    break;
                }
        }

        if (slot < 0)
        {
            for (int i = 0; i < maxVoicesNow; ++i)
            {
                if (!voices[i].active)
                {
                    slot = i;
                    break;
                }
                if (voices[i].startTime < oldest)
                {
                    oldest = voices[i].startTime;
                    oldestSlot = i;
                }
            }
            if (slot < 0)
                slot = oldestSlot;
        }

        auto& v = voices[slot];

        // 4 strands: strand 0 at root, strand 1 at +strandTune semitones,
        // strand 2 at -strandTune, strand 3 at +strandTune*2 (octave-displaced pair)
        float baseNote = static_cast<float>(noteNum) - 69.0f;
        float newFreq0 = 440.0f * fastPow2(baseNote / 12.0f);
        float newFreq1 = 440.0f * fastPow2((baseNote + strandTune) / 12.0f);
        float newFreq2 = 440.0f * fastPow2((baseNote - strandTune) / 12.0f);
        float newFreq3 = 440.0f * fastPow2((baseNote + strandTune * 2.0f) / 12.0f);

        if (isLegato && v.active)
        {
            v.targetFreq[0] = newFreq0;
            v.targetFreq[1] = newFreq1;
            v.targetFreq[2] = newFreq2;
            v.targetFreq[3] = newFreq3;
            v.note = noteNum;
            v.velocity = vel;
            // Legato retrigger: restart attack from current level (smooth crossfade).
            // Without this the envelope stays in sustain/release regardless of ADSR params.
            v.ampEnv.retriggerFrom(v.ampEnv.getLevel(), ampA, ampD, ampS, ampR);
            // Keep LFO params in sync with current block values on legato glide.
            for (int l = 0; l < 2; ++l)
            {
                if (lfoTypes[l] > 0)
                {
                    v.lfos[l].setRate(lfoRates[l], sr);
                    v.lfos[l].shape = lfoTypes[l] - 1;
                }
            }
        }
        else
        {
            // Voice steal: capture current amplitude before reset so the crossfade
            // ramp can smoothly fade the stolen voice out over 5ms.
            const float stolenLevel = v.active ? v.ampEnv.getLevel() : 0.0f;

            v.reset();
            v.active = true;
            v.note = noteNum;
            v.velocity = vel;
            v.startTime = ++voiceCounter;

            // Begin 5ms steal crossfade from captured level down to 0.
            if (stolenLevel > 0.0f)
            {
                v.stealFadeGain = stolenLevel;
                v.stealFadeStep = 1.0f / (0.005f * sr);
            }

            v.strandFreq[0] = newFreq0;
            v.strandFreq[1] = newFreq1;
            v.strandFreq[2] = newFreq2;
            v.strandFreq[3] = newFreq3;
            v.targetFreq[0] = newFreq0;
            v.targetFreq[1] = newFreq1;
            v.targetFreq[2] = newFreq2;
            v.targetFreq[3] = newFreq3;

            v.ampEnv.setParams(ampA, ampD, ampS, ampR, sr);
            v.ampEnv.noteOn();

            for (int l = 0; l < 2; ++l)
            {
                if (lfoTypes[l] > 0)
                {
                    v.lfos[l].setRate(lfoRates[l], sr);
                    v.lfos[l].shape = lfoTypes[l] - 1;
                }
            }
        }
    }

    void noteOff(int noteNum)
    {
        for (auto& v : voices)
            if (v.active && v.note == noteNum)
                v.ampEnv.noteOff();
    }

    //==========================================================================
    // State
    //==========================================================================
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)
    uint64_t voiceCounter = 0;
    std::array<OrbweaveVoice, kMaxVoices> voices{};
    std::atomic<int> activeVoices{0};
    float modWheel_ = 0.0f;
    float pitchBend_ = 0.0f;
    int polyLimit_ = 8;
    float couplingPitchMod = 0.0f;
    float couplingCutoffMod = 0.0f;

    // Coupling output
    float lastSampleL = 0.0f;
    float lastSampleR = 0.0f;

    // Coupling matrix cache — rebuilt only when topology parameters change
    float cachedMatrix[4][4]{};
    int   cachedKnotA = -1, cachedKnotB = -1;
    float cachedKnotMorph = -1.0f;
    float cachedTorusP = -1.0f, cachedTorusQ = -1.0f;
    bool  matrixDirty = true; // true until first prepare() + first block

    // 3 FX slots
    OrbweaveFXState fxSlots[3];

    // Parameter pointers
    std::atomic<float>*pStrandType = nullptr, *pStrandTune = nullptr;
    std::atomic<float>*pKnotType = nullptr, *pBraidDepth = nullptr;
    std::atomic<float>*pTorusP = nullptr, *pTorusQ = nullptr;
    std::atomic<float>*pFilterCutoff = nullptr, *pFilterReso = nullptr, *pFilterType = nullptr;
    std::atomic<float>*pAmpA = nullptr, *pAmpD = nullptr;
    std::atomic<float>*pAmpS = nullptr, *pAmpR = nullptr;
    std::atomic<float>*pLfoType[2]{}, *pLfoTarget[2]{}, *pLfoDepth[2]{}, *pLfoRate[2]{};
    std::atomic<float>*pFxType[3]{}, *pFxMix[3]{}, *pFxParam[3]{};
    std::atomic<float>*pMacroWeave = nullptr, *pMacroTension = nullptr;
    std::atomic<float>*pMacroKnot = nullptr, *pMacroSpace = nullptr;
    std::atomic<float>* pLevel = nullptr;
    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>*pPitchBendRange = nullptr, *pGlideTime = nullptr;
};

} // namespace xoceanus
