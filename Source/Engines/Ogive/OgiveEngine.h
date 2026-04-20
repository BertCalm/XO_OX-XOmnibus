// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/VoiceAllocator.h"
#include "../../DSP/ModMatrix.h"
#include "../../DSP/ParameterSmoother.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/GlideProcessor.h"
#include "../../DSP/Effects/DCBlocker.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
//
//  O G I V E   E N G I N E
//  Scanned Glass Synthesis
//
//  XO_OX Aquatic Identity: Glass Sponge (hexactinellid) — Twilight Zone.
//  A living cathedral of silica lattice that channels ocean current through
//  its tracery. Sound passes through its body as light through stained glass.
//  Accent: Selenium Ruby #9B1B30.
//
//  Architecture: Spring-mass glass surface initialized with synthwave waveforms,
//  scanned by a read-head moving along an ogive (Gothic arch) trajectory.
//  Verlet integration with nonlinear cubic springs. Neon filter with tanh
//  saturation. Juno-style stereo chorus. Gated reverb.
//
//  Signal Flow:
//    MIDI Note → Init surface (waveform shape × velocity) → Verlet physics
//      → Ogive scan trajectory → Cubic Hermite surface read
//      → Neon saturation (tanh) → Per-voice CytomicSVF LP filter (L + R)
//      → Amplitude envelope × velocity
//      → Stereo accumulate → Shared chorus → Shared gated reverb
//      → DC blocking → Output
//
//  Coupling (XOceanus inter-engine modulation):
//    Output: post-filter audio via getSampleForCoupling (ch0=L, ch1=R)
//    Input:  AudioToFM    → modulates scan rate
//            SpectralShaping → adds surface excitation
//            PhaseSync    → nudges scan phase
//            AmpToFilter  → fallback filter cutoff modulation
//
//  Parameter prefix:  ogv_
//  Gallery code:      OGIV
//  Max voices:        8
//
//==============================================================================

// ---- Engine-level constants ----
static constexpr int   kOgiveMaxPanes       = 64;
static constexpr int   kOgiveMaxVoices      = 8;
static constexpr float kOgivePI             = 3.14159265358979323846f;
static constexpr float kOgiveTwoPI          = 6.28318530717958647692f;
static constexpr float kOgiveMaxVel         = 4.0f;
static constexpr float kOgiveMaxPos         = 4.0f;
static constexpr float kOgivePhaseSyncMax   = 0.1f;
static constexpr float kOgiveCouplingScale  = 0.25f;

//==============================================================================
//  Waveform enum
//==============================================================================
enum OgiveWaveform { OGV_SAW = 0, OGV_PULSE, OGV_TRI, OGV_SINE, OGV_NOISE };

//==============================================================================
//  OgiveVoice — Per-Voice State
//==============================================================================
struct OgiveVoice
{
    bool     active        = false;
    bool     isFadingOut   = false;
    int      midiNote      = 60;
    float    velocity      = 0.0f;
    float    frequency     = 440.0f;
    float    targetFrequency  = 440.0f;
    float    currentFrequency = 440.0f;
    float    glideCoefficient = 1.0f;
    uint64_t startTime     = 0;
    float    crossfadeGain = 1.0f;

    // Glass surface state
    float surfacePos[kOgiveMaxPanes];
    float surfaceVel[kOgiveMaxPanes];
    float thermalDrift[kOgiveMaxPanes];   // Schulze temporal memory
    int   currentPaneCount = 32;

    // Scan state
    float scanPhase    = 0.0f;    // 0..1 position in scan cycle
    float scanPosition = 0.5f;   // actual read-head position on surface

    // Per-voice modulation (D002/D005)
    StandardLFO  lfo1;
    StandardADSR ampEnv;
    StandardADSR modEnv;   // routes to neon filter cutoff

    // Per-voice neon filter (L+R independent)
    CytomicSVF neonFilterL;
    CytomicSVF neonFilterR;

    // DC blocking post-surface
    float dcBlockStateL = 0.0f;
    float dcBlockStateR = 0.0f;

    // Coupling output cache
    float lastCouplingOut = 0.0f;

    void reset() noexcept
    {
        active        = false;
        isFadingOut   = false;
        midiNote      = 60;
        velocity      = 0.0f;
        frequency     = 440.0f;
        targetFrequency   = 440.0f;
        currentFrequency  = 440.0f;
        glideCoefficient  = 1.0f;
        startTime     = 0;
        crossfadeGain = 1.0f;
        scanPhase     = 0.0f;
        scanPosition  = 0.5f;
        std::memset(surfacePos,    0, sizeof(surfacePos));
        std::memset(surfaceVel,    0, sizeof(surfaceVel));
        std::memset(thermalDrift,  0, sizeof(thermalDrift));
        currentPaneCount = 32;
        lfo1.reset();
        ampEnv.reset();
        modEnv.reset();
        neonFilterL.reset();
        neonFilterR.reset();
        dcBlockStateL = dcBlockStateR = 0.0f;
        lastCouplingOut = 0.0f;
    }
};

//==============================================================================
//
//  OgiveEngine
//
//  Implements SynthEngine for XOceanus integration.
//  All DSP is inline per project convention — the .cpp is a one-line stub.
//
//==============================================================================
class OgiveEngine : public SynthEngine
{
public:

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  L I F E C Y C L E
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sampleRateDouble = sampleRate;
        sampleRateFloat  = static_cast<float>(sampleRate);

        // 5ms smoothing coefficient (zipper noise prevention)
        paramSmoothCoeff = 1.0f - std::exp(-kOgiveTwoPI * (1.0f / 0.005f) / sampleRateFloat);

        // 5ms voice crossfade rate (linear ramp)
        voiceFadeRate = 1.0f / (0.005f * sampleRateFloat);

        // Output cache for coupling reads
        outputCacheLeft.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheRight.assign(static_cast<size_t>(maxBlockSize), 0.0f);

        // Chorus buffers — allocate 2048 samples per channel
        chorusBufL.assign(2048, 0.0f);
        chorusBufR.assign(2048, 0.0f);
        chorusWriteIdx = 0;
        chorusReadPos  = 0.0f;

        // Gated reverb delay lines (prime lengths for decorrelation)
        fdnBuf0.assign(1481, 0.0f);
        fdnBuf1.assign(1823, 0.0f);
        fdnBuf2.assign(2141, 0.0f);
        fdnBuf3.assign(2503, 0.0f);
        fdnIdx0 = fdnIdx1 = fdnIdx2 = fdnIdx3 = 0;
        fdnState0 = fdnState1 = fdnState2 = fdnState3 = 0.0f;
        fdnGateLevel = 0.0f;
        fdnGateOpen  = false;

        // 200ms hold for gated reverb tail
        prepareSilenceGate(sampleRate, maxBlockSize, 200.0f);

        // Reset all voices
        for (auto& v : voices)
            v.reset();

        // Reset smoothed params
        smoothedCutoff    = 4000.0f;
        smoothedReso      = 0.3f;
        smoothedDrive     = 0.15f;
        smoothedScanRate  = 1.0f;
        smoothedScanPos   = 0.5f;
        smoothedStiffness = 0.45f;
        smoothedDamping   = 0.02f;
        smoothedGlassQ    = 60.0f;
        modWheelValue     = 0.0f;
        aftertouchValue   = 0.0f;
        pitchBendNorm     = 0.0f;

        couplingFMAccum      = 0.0f;
        couplingExciteAccum  = 0.0f;
        couplingSyncAccum    = 0.0f;
        couplingFilterAccum  = 0.0f;
        cachedCouplingOutput = 0.0f;

        // Chorus LFO phase
        chorusLfoPhase = 0.0f;
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();

        couplingFMAccum      = 0.0f;
        couplingExciteAccum  = 0.0f;
        couplingSyncAccum    = 0.0f;
        couplingFilterAccum  = 0.0f;
        cachedCouplingOutput = 0.0f;
        modWheelValue        = 0.0f;
        aftertouchValue      = 0.0f;
        pitchBendNorm        = 0.0f;

        std::fill(outputCacheLeft.begin(),  outputCacheLeft.end(),  0.0f);
        std::fill(outputCacheRight.begin(), outputCacheRight.end(), 0.0f);
        std::fill(chorusBufL.begin(), chorusBufL.end(), 0.0f);
        std::fill(chorusBufR.begin(), chorusBufR.end(), 0.0f);
        std::fill(fdnBuf0.begin(), fdnBuf0.end(), 0.0f);
        std::fill(fdnBuf1.begin(), fdnBuf1.end(), 0.0f);
        std::fill(fdnBuf2.begin(), fdnBuf2.end(), 0.0f);
        std::fill(fdnBuf3.begin(), fdnBuf3.end(), 0.0f);
        fdnState0 = fdnState1 = fdnState2 = fdnState3 = 0.0f;
        fdnGateLevel = 0.0f;
    }

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  A U D I O
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0 || sampleRateFloat <= 0.0f)
            return;

        // ---- ParamSnapshot: read all parameter values once per block ----
        const int   paramWaveform      = static_cast<int>(loadParam(pWaveform,      0.0f));
        const float paramWaveAsym      = loadParam(pWaveAsym,      0.5f);
        const int   paramPaneChoice    = static_cast<int>(loadParam(pPaneCount,     3.0f));
        const float paramStiffness     = loadParam(pTraceryStiff,  0.45f);
        const float paramDamping       = loadParam(pLeadDamp,      0.02f);
        const float paramGlassQ        = loadParam(pGlassQ,        60.0f);

        const float paramScanRate      = loadParam(pScanRate,      1.0f);
        const float paramScanPos       = loadParam(pScanPos,       0.5f);
        const float paramLancetShape   = loadParam(pLancetShape,   1.5f);
        const int   paramScanMode      = static_cast<int>(loadParam(pScanMode,      0.0f));
        const int   paramSyncDiv       = static_cast<int>(loadParam(pSyncDiv,       2.0f));
        const float paramSurfaceMemory = loadParam(pSurfaceMemory, 0.0f);
        const float paramThermal       = loadParam(pThermal,       0.0f);

        const float paramNeonCutoff    = loadParam(pNeonCutoff,    4000.0f);
        const float paramNeonReso      = loadParam(pNeonReso,      0.3f);
        const float paramNeonDrive     = loadParam(pNeonDrive,     0.15f);
        const float paramChorusDepth   = loadParam(pChorusDepth,   0.4f);

        const float paramChorusRate    = loadParam(pChorusRate,    0.3f);
        const float paramGateTime      = loadParam(pGateTime,      0.3f);
        const float paramNaveSize      = loadParam(pNaveSize,      0.5f);
        const float paramNaveMix       = loadParam(pNaveMix,       0.25f);

        const float paramLfo1Rate      = loadParam(pLfo1Rate,      0.5f);
        const int   paramLfo1Shape     = static_cast<int>(loadParam(pLfo1Shape, 0.0f));
        const float paramLfo1Depth     = loadParam(pLfo1Depth,     0.35f);

        const float paramEnvAttack     = loadParam(pEnvAttack,     0.01f);
        const float paramEnvDecay      = loadParam(pEnvDecay,      0.5f);
        const float paramEnvSustain    = loadParam(pEnvSustain,    0.7f);
        const float paramEnvRelease    = loadParam(pEnvRelease,    0.8f);

        const int   paramVoiceMode     = static_cast<int>(loadParam(pVoiceMode, 0.0f));

        const float macroChar  = loadParam(pMacro1, 0.0f);  // CHARACTER
        const float macroMove  = loadParam(pMacro2, 0.0f);  // MOVEMENT
        const float macroCoup  = loadParam(pMacro3, 0.0f);  // COUPLING
        const float macroSpace = loadParam(pMacro4, 0.0f);  // SPACE

        // ---- Resolve pane count from choice index ----
        static constexpr int kPaneCounts[5] = { 4, 8, 16, 32, 64 };
        const int paneCount = kPaneCounts[juce::jlimit(0, 4, paramPaneChoice)];

        // ---- Sync div values (in beats — treated as multiplier for scanRate) ----
        static constexpr float kSyncDivs[7] = { 0.0625f, 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
        const float syncDiv = kSyncDivs[juce::jlimit(0, 6, paramSyncDiv)];

        // ---- Voice mode ----
        const bool monoMode   = (paramVoiceMode == 1 || paramVoiceMode == 2);
        const bool legatoMode = (paramVoiceMode == 2);
        const int  maxPoly    = monoMode ? 1 : kOgiveMaxVoices;

        // ---- Macro application ----
        // M1 CHARACTER: stiffness ↑, glass_q ↑, wave_asym sweep
        float effectiveStiffness = juce::jlimit(0.01f, 1.0f, paramStiffness + macroChar * 0.55f);
        float effectiveGlassQ    = juce::jlimit(1.0f, 200.0f, paramGlassQ + macroChar * 140.0f);

        // M2 MOVEMENT: scan_rate ↑, lancet_shape ↑
        float effectiveScanRate  = juce::jlimit(0.01f, 20.0f, paramScanRate + macroMove * 10.0f);
        float effectiveLancet    = juce::jlimit(0.5f,  4.0f,  paramLancetShape + macroMove * 1.5f);

        // M3 COUPLING: looser (tracery ↓, damp ↓)
        float effectiveDamping   = juce::jlimit(0.001f, 0.2f, paramDamping - macroCoup * 0.01f);

        // M4 SPACE: neon drive ↑, chorus ↑, nave mix ↑
        float effectiveNeonDrive = juce::jlimit(0.0f, 1.0f, paramNeonDrive + macroSpace * 0.5f);
        float effectiveChorusDepth = juce::jlimit(0.0f, 1.0f, paramChorusDepth + macroSpace * 0.3f);
        float effectiveNaveMix   = juce::jlimit(0.0f, 1.0f, paramNaveMix + macroSpace * 0.4f);

        // ---- Apply coupling inputs ----
        // FM coupling modulates scan rate
        float effectiveScanRateWithCoupling = juce::jlimit(
            0.01f, 40.0f, effectiveScanRate * (1.0f + couplingFMAccum));
        // Filter coupling modulates neon cutoff
        float effectiveCutoff = juce::jlimit(80.0f, 20000.0f,
            paramNeonCutoff + couplingFilterAccum * 4000.0f + modWheelValue * 8000.0f);

        // Consume coupling accumulators
        couplingFMAccum     = 0.0f;
        couplingExciteAccum = 0.0f;
        couplingSyncAccum   = 0.0f;
        couplingFilterAccum = 0.0f;

        // Nonlinear spring k2 from glass_q
        const float nonlinearK2 = effectiveGlassQ / 200.0f * 0.5f;

        // Pitch bend ratio (±2 semitones)
        const float pitchBendRatio = std::pow(2.0f, pitchBendNorm * 2.0f / 12.0f);

        // ---- Process MIDI ----
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                wakeSilenceGate();
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity(),
                       maxPoly, monoMode, legatoMode,
                       paramEnvAttack, paramEnvDecay, paramEnvSustain, paramEnvRelease,
                       paramLfo1Rate, paramLfo1Shape,
                       paneCount, paramWaveform, paramWaveAsym,
                       effectiveCutoff, paramNeonReso, paramSurfaceMemory);
            }
            else if (msg.isNoteOff())
            {
                noteOff(msg.getNoteNumber());
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                reset();
            }
            else if (msg.isController())
            {
                if (msg.getControllerNumber() == 1)
                {
                    // CC1 mod wheel → scan pos (D006)
                    modWheelValue = msg.getControllerValue() / 127.0f;
                }
                else if (msg.getControllerNumber() == 11)
                {
                    // CC11 expression → neon drive (D006)
                    aftertouchValue = msg.getControllerValue() / 127.0f;
                }
            }
            else if (msg.isChannelPressure())
            {
                // Aftertouch → scan position offset
                aftertouchValue = msg.getChannelPressureValue() / 127.0f;
            }
            else if (msg.isPitchWheel())
            {
                int raw = msg.getPitchWheelValue();
                pitchBendNorm = static_cast<float>(raw - 8192) / 8192.0f;
            }
        }

        // Silence gate bypass: if idle and no MIDI, clear and return
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : outL;

        // ---- Effective scan position (base + mod wheel + aftertouch) ----
        float effectiveScanPos = juce::jlimit(0.0f, 1.0f,
            paramScanPos + modWheelValue * 0.3f + aftertouchValue * 0.2f);

        // ---- Per-sample render loop ----
        for (int sampleIdx = 0; sampleIdx < numSamples; ++sampleIdx)
        {
            const bool updateFilter = ((sampleIdx & 15) == 0);
            int activeCount = 0;
            // One-pole smoothing for control parameters
            smoothedCutoff    += 0.001f * (effectiveCutoff    - smoothedCutoff);
            smoothedReso      += 0.001f * (paramNeonReso       - smoothedReso);
            smoothedDrive     += 0.001f * (effectiveNeonDrive  - smoothedDrive);
            smoothedScanRate  += 0.001f * (effectiveScanRateWithCoupling - smoothedScanRate);
            smoothedScanPos   += 0.001f * (effectiveScanPos   - smoothedScanPos);
            smoothedStiffness += 0.001f * (effectiveStiffness  - smoothedStiffness);
            smoothedDamping   += 0.001f * (effectiveDamping    - smoothedDamping);

            smoothedCutoff    = flushDenormal(smoothedCutoff);
            smoothedReso      = flushDenormal(smoothedReso);
            smoothedDrive     = flushDenormal(smoothedDrive);
            smoothedScanRate  = flushDenormal(smoothedScanRate);
            smoothedScanPos   = flushDenormal(smoothedScanPos);
            smoothedStiffness = flushDenormal(smoothedStiffness);
            smoothedDamping   = flushDenormal(smoothedDamping);

            float mixLeft  = 0.0f;
            float mixRight = 0.0f;

            for (auto& voice : voices)
            {
                // Architect Contract #8: SilenceGate — skip idle voices entirely
                if (!voice.active)
                    continue;

                ++activeCount;

                // Voice stealing crossfade
                if (voice.isFadingOut)
                {
                    voice.crossfadeGain -= voiceFadeRate;
                    voice.crossfadeGain  = flushDenormal(voice.crossfadeGain);
                    if (voice.crossfadeGain <= 0.0f)
                    {
                        voice.crossfadeGain = 0.0f;
                        voice.active        = false;
                        continue;
                    }
                }

                // Portamento glide
                voice.currentFrequency += (voice.targetFrequency - voice.currentFrequency)
                                          * voice.glideCoefficient;
                voice.currentFrequency = flushDenormal(voice.currentFrequency);

                // Base frequency with pitch bend
                float freq = voice.currentFrequency * pitchBendRatio;

                // ---- Tick envelopes ----
                float ampLevel = voice.ampEnv.process();
                float modLevel = voice.modEnv.process();

                // Check if voice is done
                if (!voice.ampEnv.isActive() && !voice.isFadingOut)
                {
                    voice.active = false;
                    continue;
                }

                // ---- Per-voice LFO (D002/D005) ----
                voice.lfo1.setRate(paramLfo1Rate, sampleRateFloat);
                voice.lfo1.setShape(paramLfo1Shape);
                float lfo1Out = flushDenormal(voice.lfo1.process() * paramLfo1Depth);

                // ---- D001: Velocity shapes timbre — key tracking on damping ----
                float keyTrackDamp = smoothedDamping * (freq / 440.0f) * 0.5f;
                float effectiveDamp = juce::jlimit(0.001f, 0.4f, keyTrackDamp);

                // ---- Update glass surface physics ----
                // Thermal drift (Schulze memory)
                if (paramThermal > 0.001f)
                    updateThermalDrift(voice.thermalDrift, voice.currentPaneCount,
                                       paramThermal, sampleRateFloat);

                updateSurface(voice.surfacePos, voice.surfaceVel, voice.currentPaneCount,
                              smoothedStiffness, effectiveDamp, nonlinearK2,
                              voice.thermalDrift);

                // ---- Update scan position via ogive trajectory ----
                // Scan rate is in Hz. Phase increments accordingly.
                float scanPhaseInc = smoothedScanRate / sampleRateFloat;

                switch (paramScanMode)
                {
                    case 0: // Trigger — reset on noteOn; already handled
                    case 1: // Free — always advancing
                    default:
                        voice.scanPhase += scanPhaseInc;
                        if (voice.scanPhase >= 1.0f)
                            voice.scanPhase -= 1.0f;
                        break;
                    case 2: // Expr — scan position driven by aftertouch
                        voice.scanPhase += scanPhaseInc;
                        if (voice.scanPhase >= 1.0f)
                            voice.scanPhase -= 1.0f;
                        // Blend expr scan position with trajectory
                        break;
                    case 3: // Sync — scanRate is quantized to syncDiv beats
                        // Use syncDiv-scaled rate (host BPM not available here —
                        // treat syncDiv as a relative rate multiplier)
                        voice.scanPhase += scanPhaseInc * syncDiv;
                        if (voice.scanPhase >= 1.0f)
                            voice.scanPhase -= 1.0f;
                        break;
                }

                // Ogive (Gothic arch) trajectory — slow at apex, fast at base
                float ogivePos = ogiveCurve(voice.scanPhase, effectiveLancet);

                // Base scan position: blend param position with ogive trajectory
                float scanReadPos = smoothedScanPos * 0.5f + ogivePos * 0.5f;
                // LFO modulates scan position slightly
                scanReadPos = juce::jlimit(0.0f, 1.0f, scanReadPos + lfo1Out * 0.1f);

                // Expr scan mode: aftertouch pulls toward one end
                if (paramScanMode == 2)
                    scanReadPos = juce::jlimit(0.0f, 1.0f,
                        scanReadPos + aftertouchValue * 0.3f);

                voice.scanPosition = scanReadPos;

                // ---- Read surface via cubic Hermite interpolation ----
                float surfaceSample = scanSurface(voice.surfacePos, voice.currentPaneCount,
                                                  voice.scanPosition);
                surfaceSample = flushDenormal(surfaceSample);

                // ---- D001: Velocity shapes harmonic content via drive ----
                // Higher velocity = more neon drive = richer harmonics
                float velocityDrive = smoothedDrive + voice.velocity * 0.3f;
                velocityDrive = juce::jlimit(0.0f, 1.0f, velocityDrive);

                // ---- Apply neon saturation (odd harmonics) ----
                float saturated = applyNeonSaturation(surfaceSample, velocityDrive);

                // ---- Modulation envelope routes to neon filter cutoff ----
                float modEnvCutoff = smoothedCutoff + modLevel * paramNeonCutoff * 0.5f;
                modEnvCutoff = juce::jlimit(80.0f, 20000.0f, modEnvCutoff);

                // ---- Per-voice neon filter (L + R independent; coeff refresh decimated) ----
                if (updateFilter)
                {
                    voice.neonFilterL.setMode(CytomicSVF::Mode::LowPass);
                    voice.neonFilterL.setCoefficients_fast(modEnvCutoff, smoothedReso, sampleRateFloat);
                    voice.neonFilterR.setMode(CytomicSVF::Mode::LowPass);
                    voice.neonFilterR.setCoefficients_fast(modEnvCutoff, smoothedReso, sampleRateFloat);
                }

                float filtL = voice.neonFilterL.processSample(saturated);
                float filtR = voice.neonFilterR.processSample(saturated);

                filtL = flushDenormal(filtL);
                filtR = flushDenormal(filtR);

                // ---- Simple per-voice DC block ----
                float dcL = filtL - voice.dcBlockStateL + 0.9995f * voice.dcBlockStateL;
                voice.dcBlockStateL = filtL;
                float dcR = filtR - voice.dcBlockStateR + 0.9995f * voice.dcBlockStateR;
                voice.dcBlockStateR = filtR;
                dcL = flushDenormal(dcL);
                dcR = flushDenormal(dcR);

                // ---- Amplitude envelope × velocity ----
                float gain = ampLevel * voice.velocity * voice.crossfadeGain;

                // Slight stereo spread (±5%)
                float spreadL = 1.0f + lfo1Out * 0.05f;
                float spreadR = 1.0f - lfo1Out * 0.05f;

                mixLeft  += dcL * gain * spreadL;
                mixRight += dcR * gain * spreadR;

                // Cache for coupling reads
                voice.lastCouplingOut = (dcL + dcR) * 0.5f * gain;
            }

            // ---- Normalize by voice count ----
            float normFactor = (activeCount > 0) ? (1.0f / static_cast<float>(activeCount)) : 1.0f;
            mixLeft  *= normFactor;
            mixRight *= normFactor;

            // Cache output for coupling
            if (static_cast<size_t>(sampleIdx) < outputCacheLeft.size())
            {
                outputCacheLeft[static_cast<size_t>(sampleIdx)]  = mixLeft;
                outputCacheRight[static_cast<size_t>(sampleIdx)] = mixRight;
            }

            outL[sampleIdx] = mixLeft;
            outR[sampleIdx] = mixRight;
        }

        // Update coupling output cache (average of active voices)
        {
            float coupSum = 0.0f;
            int coupCount = 0;
            for (const auto& v : voices)
            {
                if (v.active) { coupSum += v.lastCouplingOut; ++coupCount; }
            }
            cachedCouplingOutput = (coupCount > 0) ? (coupSum / static_cast<float>(coupCount)) : 0.0f;
        }

        // Update active voice count (atomic, read by UI thread)
        {
            int cnt = 0;
            for (const auto& v : voices)
                if (v.active) ++cnt;
            activeVoiceCount_.store(cnt, std::memory_order_relaxed);
        }

        // ---- Apply shared Juno-style chorus ----
        applyChorus(outL, outR, numSamples, effectiveChorusDepth, paramChorusRate);

        // ---- Apply shared gated reverb ----
        applyGatedReverb(outL, outR, numSamples, paramNaveSize, effectiveNaveMix, paramGateTime);

        // Update output cache after effects
        for (int i = 0; i < numSamples; ++i)
        {
            if (static_cast<size_t>(i) < outputCacheLeft.size())
            {
                outputCacheLeft[static_cast<size_t>(i)]  = outL[i];
                outputCacheRight[static_cast<size_t>(i)] = outR[i];
            }
        }

        // ---- Feed silence gate ----
        analyzeForSilenceGate(buffer, numSamples);
    }

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  C O U P L I N G
    //==========================================================================

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        if (channel == 0)
            return cachedCouplingOutput;
        if (channel == 1)
            return cachedCouplingOutput;
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount,
                            const float* sourceBuffer, int numSamples) override
    {
        // Architect Contract #9: scale and clamp coupling input
        if (type == CouplingType::AudioToFM)
        {
            // Modulates scan rate
            for (int i = 0; i < numSamples; ++i)
            {
                float s = (sourceBuffer != nullptr ? sourceBuffer[i] : 0.0f) * amount;
                float scaled = juce::jlimit(-1.0f, 1.0f, s * kOgiveCouplingScale);
                couplingFMAccum += (scaled - couplingFMAccum) * 0.01f;
            }
        }
        else if (type == CouplingType::EnvToMorph)
        {
            // Spectral shaping: adds excitation to surface (best match)
            for (int i = 0; i < numSamples; ++i)
            {
                float s = (sourceBuffer != nullptr ? sourceBuffer[i] : 0.0f) * amount;
                float scaled = juce::jlimit(-1.0f, 1.0f, s * kOgiveCouplingScale);
                couplingExciteAccum += (scaled - couplingExciteAccum) * 0.01f;
            }
        }
        else if (type == CouplingType::LFOToPitch)
        {
            // PhaseSync: nudges scan phase
            for (int i = 0; i < numSamples; ++i)
            {
                float s = (sourceBuffer != nullptr ? sourceBuffer[i] : 0.0f) * amount;
                float scaled = juce::jlimit(-kOgivePhaseSyncMax, kOgivePhaseSyncMax,
                                            s * kOgiveCouplingScale);
                couplingSyncAccum += (scaled - couplingSyncAccum) * 0.01f;
            }
        }
        else
        {
            // AmpToFilter fallback — modulates neon cutoff
            for (int i = 0; i < numSamples; ++i)
            {
                float s = (sourceBuffer != nullptr ? sourceBuffer[i] : 0.0f) * amount;
                float scaled = juce::jlimit(-1.0f, 1.0f, s * kOgiveCouplingScale);
                couplingFilterAccum += (scaled - couplingFilterAccum) * 0.01f;
            }
        }
    }

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  P A R A M E T E R S
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // ---- Glass Surface (6) ----

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"ogv_waveform", 1}, "XOgive Waveform",
            juce::StringArray{"Saw", "Pulse", "Tri", "Sine", "Noise"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_wave_asym", 1}, "XOgive Wave Asymmetry",
            juce::NormalisableRange<float>(0.1f, 0.9f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"ogv_pane_count", 1}, "XOgive Panes",
            juce::StringArray{"4", "8", "16", "32", "64"}, 3));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_tracery_stiff", 1}, "XOgive Tracery",
            juce::NormalisableRange<float>(0.01f, 1.0f, 0.001f), 0.45f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_lead_damp", 1}, "XOgive Lead",
            juce::NormalisableRange<float>(0.001f, 0.2f, 0.0001f, 0.4f), 0.02f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_glass_q", 1}, "XOgive Glass Q",
            juce::NormalisableRange<float>(1.0f, 200.0f, 0.1f, 0.5f), 60.0f));

        // ---- Scan (6) ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_scan_rate", 1}, "XOgive Scan Rate",
            juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.3f), 1.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_scan_pos", 1}, "XOgive Scan Position",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_lancet_shape", 1}, "XOgive Lancet",
            juce::NormalisableRange<float>(0.5f, 4.0f, 0.01f), 1.5f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"ogv_scan_mode", 1}, "XOgive Scan Mode",
            juce::StringArray{"Trigger", "Free", "Expr", "Sync"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"ogv_sync_div", 1}, "XOgive Sync Div",
            juce::StringArray{"1/16", "1/8", "1/4", "1/2", "1", "2", "4"}, 2));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_surface_memory", 1}, "XOgive Surface Memory",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // ---- Neon (4) ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_neon_cutoff", 1}, "XOgive Neon Cutoff",
            juce::NormalisableRange<float>(80.0f, 20000.0f, 0.1f, 0.25f), 4000.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_neon_reso", 1}, "XOgive Neon Reso",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_neon_drive", 1}, "XOgive Neon Drive",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.15f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_chorus_depth", 1}, "XOgive Chorus",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.4f));

        // ---- Chorus + Space (4) ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_chorus_rate", 1}, "XOgive Chorus Rate",
            juce::NormalisableRange<float>(0.05f, 5.0f, 0.01f, 0.5f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_gate_time", 1}, "XOgive Gate Time",
            juce::NormalisableRange<float>(0.05f, 2.0f, 0.001f, 0.5f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_nave_size", 1}, "XOgive Nave Size",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_nave_mix", 1}, "XOgive Nave Mix",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.25f));

        // ---- Modulation (7) ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_lfo1_rate", 1}, "XOgive LFO Rate",
            juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.3f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"ogv_lfo1_shape", 1}, "XOgive LFO Shape",
            juce::StringArray{"Sin", "Tri", "Saw", "Sq", "S&H"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_lfo1_depth", 1}, "XOgive LFO Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.35f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_env_attack", 1}, "XOgive Env Attack",
            juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.01f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_env_decay", 1}, "XOgive Env Decay",
            juce::NormalisableRange<float>(0.01f, 10.0f, 0.001f, 0.3f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_env_sustain", 1}, "XOgive Env Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.7f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_env_release", 1}, "XOgive Env Release",
            juce::NormalisableRange<float>(0.01f, 10.0f, 0.001f, 0.3f), 0.8f));

        // ---- Performance (2) ----

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"ogv_voice_mode", 1}, "XOgive Voice Mode",
            juce::StringArray{"Poly", "Mono", "Legato"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_thermal", 1}, "XOgive Thermal",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // ---- Standard macros ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_macro1", 1}, "XOgive Macro CHARACTER",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_macro2", 1}, "XOgive Macro MOVEMENT",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_macro3", 1}, "XOgive Macro COUPLING",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ogv_macro4", 1}, "XOgive Macro SPACE",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pWaveform       = apvts.getRawParameterValue("ogv_waveform");
        pWaveAsym       = apvts.getRawParameterValue("ogv_wave_asym");
        pPaneCount      = apvts.getRawParameterValue("ogv_pane_count");
        pTraceryStiff   = apvts.getRawParameterValue("ogv_tracery_stiff");
        pLeadDamp       = apvts.getRawParameterValue("ogv_lead_damp");
        pGlassQ         = apvts.getRawParameterValue("ogv_glass_q");

        pScanRate       = apvts.getRawParameterValue("ogv_scan_rate");
        pScanPos        = apvts.getRawParameterValue("ogv_scan_pos");
        pLancetShape    = apvts.getRawParameterValue("ogv_lancet_shape");
        pScanMode       = apvts.getRawParameterValue("ogv_scan_mode");
        pSyncDiv        = apvts.getRawParameterValue("ogv_sync_div");
        pSurfaceMemory  = apvts.getRawParameterValue("ogv_surface_memory");

        pNeonCutoff     = apvts.getRawParameterValue("ogv_neon_cutoff");
        pNeonReso       = apvts.getRawParameterValue("ogv_neon_reso");
        pNeonDrive      = apvts.getRawParameterValue("ogv_neon_drive");
        pChorusDepth    = apvts.getRawParameterValue("ogv_chorus_depth");

        pChorusRate     = apvts.getRawParameterValue("ogv_chorus_rate");
        pGateTime       = apvts.getRawParameterValue("ogv_gate_time");
        pNaveSize       = apvts.getRawParameterValue("ogv_nave_size");
        pNaveMix        = apvts.getRawParameterValue("ogv_nave_mix");

        pLfo1Rate       = apvts.getRawParameterValue("ogv_lfo1_rate");
        pLfo1Shape      = apvts.getRawParameterValue("ogv_lfo1_shape");
        pLfo1Depth      = apvts.getRawParameterValue("ogv_lfo1_depth");

        pEnvAttack      = apvts.getRawParameterValue("ogv_env_attack");
        pEnvDecay       = apvts.getRawParameterValue("ogv_env_decay");
        pEnvSustain     = apvts.getRawParameterValue("ogv_env_sustain");
        pEnvRelease     = apvts.getRawParameterValue("ogv_env_release");

        pVoiceMode      = apvts.getRawParameterValue("ogv_voice_mode");
        pThermal        = apvts.getRawParameterValue("ogv_thermal");

        pMacro1         = apvts.getRawParameterValue("ogv_macro1");
        pMacro2         = apvts.getRawParameterValue("ogv_macro2");
        pMacro3         = apvts.getRawParameterValue("ogv_macro3");
        pMacro4         = apvts.getRawParameterValue("ogv_macro4");
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl(params);
        return juce::AudioProcessorValueTreeState::ParameterLayout(
            std::make_move_iterator(params.begin()),
            std::make_move_iterator(params.end()));
    }

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  I D E N T I T Y
    //==========================================================================

    juce::String getEngineId()          const override { return "Ogive"; }
    juce::Colour getAccentColour()      const override { return juce::Colour(0x9B, 0x1B, 0x30); }
    int          getMaxVoices()         const override { return kOgiveMaxVoices; }

    int getActiveVoiceCount() const override
    {
        return activeVoiceCount_.load(std::memory_order_relaxed);
    }

    //==========================================================================
    //  M A C R O   M A P P I N G
    //==========================================================================

    // Macros are applied in renderBlock via the effectiveXxx variables.
    // This method is provided for UI feedback / external macro binding.
    void setMacro(int macroIndex, float value)
    {
        // Macros are handled via APVTS parameters ogv_macro1..4.
        // Direct parameter modulation (non-APVTS path) not used.
        (void)macroIndex; (void)value;
    }

private:

    //==========================================================================
    //  H E L P E R S
    //==========================================================================

    static float loadParam(const std::atomic<float>* ptr, float fallback) noexcept
    {
        return (ptr != nullptr) ? ptr->load(std::memory_order_relaxed) : fallback;
    }

    static float midiNoteToHz(int noteNum) noexcept
    {
        return 440.0f * std::pow(2.0f, (static_cast<float>(noteNum) - 69.0f) / 12.0f);
    }

    //==========================================================================
    //  C O R E   D S P   M E T H O D S
    //==========================================================================

    // 1. initSurface — Initialize glass surface with synthwave waveform
    inline void initSurface(float* pos, int N, int waveform, float asymmetry) noexcept
    {
        for (int i = 0; i < N; ++i)
        {
            float t = static_cast<float>(i) / static_cast<float>(N - 1);
            switch (waveform)
            {
                case OGV_SAW:
                    pos[i] = 2.0f * std::fmod(t * (asymmetry + 0.5f) + 0.5f, 1.0f) - 1.0f;
                    break;
                case OGV_PULSE:
                    pos[i] = (t < asymmetry) ? 1.0f : -1.0f;
                    break;
                case OGV_TRI:
                {
                    float phase = std::fmod(t + (asymmetry - 0.5f) * 0.5f, 1.0f);
                    if (phase < 0.0f) phase += 1.0f;
                    if (phase < asymmetry)
                        pos[i] = 2.0f * phase / asymmetry - 1.0f;
                    else
                        pos[i] = 1.0f - 2.0f * (phase - asymmetry) / (1.0f - asymmetry);
                    break;
                }
                case OGV_SINE:
                    pos[i] = fastSin(t * kOgiveTwoPI);
                    break;
                case OGV_NOISE:
                default:
                    pos[i] = rng.nextFloat() * 2.0f - 1.0f;
                    break;
            }
        }
    }

    // 2. exciteSurface — Nonlinear velocity excitation (D001)
    // Quadratic velocity scaling: higher velocity excites more modes nonlinearly
    inline void exciteSurface(float* vel, int N, float velocity) noexcept
    {
        float energy = velocity * velocity;  // D001: quadratic, not linear
        for (int i = 1; i < N - 1; ++i)
        {
            float impulse = energy * fastSin(
                static_cast<float>(i) / static_cast<float>(N) * kOgivePI);
            vel[i] += impulse;
        }
    }

    // 3. updateSurface — Verlet integration with nonlinear springs (D001 + D003)
    // Verlet integration with magnitude clamping (Architect Contract #13)
    inline void updateSurface(float* pos, float* vel, int N,
                               float stiffness, float damping, float nonlinearK2,
                               float* thermalDrift) noexcept
    {
        // Fixed boundary conditions: pos[0] and pos[N-1] remain at 0
        for (int i = 1; i < N - 1; ++i)
        {
            float spring = stiffness * (pos[i - 1] - 2.0f * pos[i] + pos[i + 1]);
            float cubic  = nonlinearK2 * pos[i] * pos[i] * pos[i];
            float damp   = -damping * vel[i];
            float drift  = thermalDrift[i];

            vel[i] += spring + cubic + damp + drift;
            pos[i] += vel[i];

            // Verlet magnitude clamp (prevents NaN at high Q + stiffness)
            vel[i] = juce::jlimit(-kOgiveMaxVel, kOgiveMaxVel, vel[i]);
            pos[i] = juce::jlimit(-kOgiveMaxPos, kOgiveMaxPos, pos[i]);

            vel[i] = flushDenormal(vel[i]);
            pos[i] = flushDenormal(pos[i]);
        }
    }

    // 4. ogiveCurve — Non-linear Gothic arch scan trajectory
    // Slow at apex, fast at base. shape > 1 = more pointed arch.
    inline float ogiveCurve(float phase, float shape) noexcept
    {
        float t = std::pow(phase, shape);
        return 0.5f * (1.0f - fastCos(kOgivePI * t));
    }

    // 5. scanSurface — Cubic Hermite interpolation read-head (Architect Contract #2)
    inline float scanSurface(const float* pos, int N, float normalizedPos) noexcept
    {
        float floatIdx = normalizedPos * static_cast<float>(N - 1);
        int idx = static_cast<int>(floatIdx);
        float frac = floatIdx - static_cast<float>(idx);

        int i0 = juce::jmax(0, idx - 1);
        int i1 = juce::jmax(0, juce::jmin(idx,     N - 1));
        int i2 =              juce::jmin(idx + 1,  N - 1);
        int i3 =              juce::jmin(idx + 2,  N - 1);

        float y0 = pos[i0], y1 = pos[i1], y2 = pos[i2], y3 = pos[i3];

        // Cubic Hermite coefficients
        float c0 =  y1;
        float c1 =  0.5f * (y2 - y0);
        float c2 =  y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 =  0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    // 6. updateThermalDrift — Schulze temporal memory (Brownian drift)
    inline void updateThermalDrift(float* thermalDrift, int N, float thermalAmount,
                                    float /*sampleRate*/) noexcept
    {
        if (thermalAmount < 0.001f) return;
        float driftRate = thermalAmount * 0.00001f;
        for (int i = 1; i < N - 1; ++i)
        {
            thermalDrift[i] += (rng.nextFloat() * 2.0f - 1.0f) * driftRate;
            thermalDrift[i] *= 0.9999f;  // soft limit to prevent runaway
            thermalDrift[i]  = flushDenormal(thermalDrift[i]);
        }
    }

    // 7. applyNeonSaturation — tanh-family odd harmonics (Guru Bin specification)
    inline float applyNeonSaturation(float input, float drive) noexcept
    {
        if (drive < 0.001f) return input;
        float gained = input * (1.0f + drive * 4.0f);
        return fastTanh(gained);  // odd harmonics — neon tube character
    }

    //==========================================================================
    //  J U N O - S T Y L E   C H O R U S
    //
    //  Two modulated delay lines (one per channel) with quadrature sine LFO.
    //  Classic BBD bucket-brigade chorus character. Keeps CPU minimal.
    //==========================================================================

    void applyChorus(float* outL, float* outR, int numSamples,
                     float depth, float rate) noexcept
    {
        if (depth < 0.001f) return;

        const int bufSize = static_cast<int>(chorusBufL.size()); // 2048

        // Chorus parameters: centre delay ~7ms, modulation depth ±3ms
        const float centreDelay = 0.007f * sampleRateFloat;  // ~315 samples at 44.1k
        const float modDepth    = depth * 0.003f * sampleRateFloat;  // ±132 samples at 44.1k

        // Chorus LFO frequency
        const float lfoInc = rate / sampleRateFloat;

        for (int i = 0; i < numSamples; ++i)
        {
            // Write input to both delay buffers (dry + wet)
            chorusBufL[static_cast<size_t>(chorusWriteIdx)] = outL[i];
            chorusBufR[static_cast<size_t>(chorusWriteIdx)] = outR[i];

            // Advance chorus LFO (quadrature: L=sin, R=cos)
            chorusLfoPhase += lfoInc;
            if (chorusLfoPhase >= 1.0f) chorusLfoPhase -= 1.0f;

            float sinMod = fastSin(chorusLfoPhase * kOgiveTwoPI);
            float cosMod = fastCos(chorusLfoPhase * kOgiveTwoPI);

            // Modulated delay times (in samples, fractional)
            float delayL = centreDelay + sinMod * modDepth;
            float delayR = centreDelay + cosMod * modDepth;

            // Clamp to valid buffer range
            delayL = juce::jlimit(1.0f, static_cast<float>(bufSize - 2), delayL);
            delayR = juce::jlimit(1.0f, static_cast<float>(bufSize - 2), delayR);

            // Read from delay buffer with linear interpolation
            auto readDelayed = [&](std::vector<float>& buf, float delay) -> float
            {
                float readPos = static_cast<float>(chorusWriteIdx) - delay;
                if (readPos < 0.0f) readPos += static_cast<float>(bufSize);
                int   ri0  = static_cast<int>(readPos) % bufSize;
                int   ri1  = (ri0 + 1) % bufSize;
                float frac = readPos - std::floor(readPos);
                return buf[static_cast<size_t>(ri0)]
                     + frac * (buf[static_cast<size_t>(ri1)] - buf[static_cast<size_t>(ri0)]);
            };

            float wetL = readDelayed(chorusBufL, delayL);
            float wetR = readDelayed(chorusBufR, delayR);

            // Mix: depth controls wet amount (dry level = 1 - depth * 0.5)
            float wet  = depth * 0.5f;
            float dry  = 1.0f - wet;
            outL[i] = outL[i] * dry + wetL * wet;
            outR[i] = outR[i] * dry + wetR * wet;

            // Advance write pointer
            chorusWriteIdx = (chorusWriteIdx + 1) % bufSize;
        }
    }

    //==========================================================================
    //  G A T E D   R E V E R B
    //
    //  FDN (Feedback Delay Network) with 4 allpass-filtered delay lines.
    //  After the tail falls below the gate threshold, hard-cut the reverb.
    //  Prime delay lengths give diffuse, non-repeating reflections.
    //==========================================================================

    void applyGatedReverb(float* outL, float* outR, int numSamples,
                           float size, float mix, float gateTimeSec) noexcept
    {
        if (mix < 0.001f) return;

        // FDN feedback coefficient: larger = longer reverb
        const float feedback = 0.5f + size * 0.45f;  // [0.5, 0.95]

        // Gate threshold: -40 dB
        const float gateThresh = 0.01f;

        // Gate release coefficient: hard-close over gateTimeSec
        const float gateReleaseRate = 1.0f / (gateTimeSec * sampleRateFloat);

        for (int i = 0; i < numSamples; ++i)
        {
            float inMono = (outL[i] + outR[i]) * 0.5f;

            // ---- FDN tick ----
            // Read from all four delay lines
            int rd0 = (fdnIdx0 + 1) % static_cast<int>(fdnBuf0.size());
            int rd1 = (fdnIdx1 + 1) % static_cast<int>(fdnBuf1.size());
            int rd2 = (fdnIdx2 + 1) % static_cast<int>(fdnBuf2.size());
            int rd3 = (fdnIdx3 + 1) % static_cast<int>(fdnBuf3.size());

            float d0 = fdnBuf0[static_cast<size_t>(rd0)];
            float d1 = fdnBuf1[static_cast<size_t>(rd1)];
            float d2 = fdnBuf2[static_cast<size_t>(rd2)];
            float d3 = fdnBuf3[static_cast<size_t>(rd3)];

            // Hadamard mixing matrix (lossless, decorrelates channels)
            float s0 =  d0 + d1 + d2 + d3;
            float s1 =  d0 + d1 - d2 - d3;
            float s2 =  d0 - d1 + d2 - d3;
            float s3 =  d0 - d1 - d2 + d3;
            s0 *= 0.5f; s1 *= 0.5f; s2 *= 0.5f; s3 *= 0.5f;

            // First-order allpass per channel (one-pole, coefficient 0.5)
            const float allpassCoeff = 0.5f;
            float ap0 = s0 + allpassCoeff * fdnState0;
            fdnState0 = flushDenormal(s0 - allpassCoeff * ap0);
            float ap1 = s1 + allpassCoeff * fdnState1;
            fdnState1 = flushDenormal(s1 - allpassCoeff * ap1);
            float ap2 = s2 + allpassCoeff * fdnState2;
            fdnState2 = flushDenormal(s2 - allpassCoeff * ap2);
            float ap3 = s3 + allpassCoeff * fdnState3;
            fdnState3 = flushDenormal(s3 - allpassCoeff * ap3);

            // Write back with feedback and input injection
            fdnBuf0[static_cast<size_t>(fdnIdx0)] = inMono + ap0 * feedback;
            fdnBuf1[static_cast<size_t>(fdnIdx1)] = inMono + ap1 * feedback;
            fdnBuf2[static_cast<size_t>(fdnIdx2)] = inMono + ap2 * feedback;
            fdnBuf3[static_cast<size_t>(fdnIdx3)] = inMono + ap3 * feedback;

            // Advance FDN write pointers
            fdnIdx0 = (fdnIdx0 + 1) % static_cast<int>(fdnBuf0.size());
            fdnIdx1 = (fdnIdx1 + 1) % static_cast<int>(fdnBuf1.size());
            fdnIdx2 = (fdnIdx2 + 1) % static_cast<int>(fdnBuf2.size());
            fdnIdx3 = (fdnIdx3 + 1) % static_cast<int>(fdnBuf3.size());

            // Reverb output (stereo decorrelation: L/R from different lines)
            float revL = (ap0 + ap2) * 0.5f;
            float revR = (ap1 + ap3) * 0.5f;
            revL = flushDenormal(revL);
            revR = flushDenormal(revR);

            // ---- Gate logic ----
            // Level detection on the reverb signal
            float reverbLevel = std::fabs(revL) + std::fabs(revR);
            if (reverbLevel > gateThresh)
            {
                fdnGateOpen  = true;
                fdnGateLevel = 1.0f;
            }
            else if (fdnGateOpen)
            {
                // Hard-close gate over gateTimeSec
                fdnGateLevel -= gateReleaseRate;
                if (fdnGateLevel <= 0.0f)
                {
                    fdnGateLevel = 0.0f;
                    fdnGateOpen  = false;
                }
            }

            // Apply gate to reverb
            revL *= fdnGateLevel;
            revR *= fdnGateLevel;

            // Mix wet into output
            outL[i] += revL * mix;
            outR[i] += revR * mix;
        }
    }

    //==========================================================================
    //  M I D I   H A N D L I N G
    //==========================================================================

    void noteOn(int noteNum, float velocity,
                int maxPoly, bool monoMode, bool legatoMode,
                float envA, float envD, float envS, float envR,
                float lfoRate, int lfoShape,
                int paneCount, int waveform, float waveAsym,
                float cutoff, float reso,
                float surfaceMemory)
    {
        float freq = midiNoteToHz(noteNum);
        lastNoteVelocity = velocity;

        if (monoMode)
        {
            auto& voice  = voices[0];
            bool wasActive = voice.active;
            voice.targetFrequency = freq;

            if (legatoMode && wasActive)
            {
                // Legato: glide without surface reinit
                voice.midiNote  = noteNum;
                voice.velocity  = velocity;
                // Soft retrigger: envelope picks up from current level
                voice.ampEnv.noteOn();
                voice.modEnv.noteOn();
            }
            else
            {
                // Re-init surface with Surface Memory blend (Architect Contract #10)
                // surfaceMemory=0: full reinit; surfaceMemory=1: keep existing surface
                float blend = surfaceMemory;

                float newPos[kOgiveMaxPanes];
                initSurface(newPos, paneCount, waveform, waveAsym);

                // Blend with existing surface
                for (int j = 0; j < paneCount; ++j)
                    voice.surfacePos[j] = newPos[j] * (1.0f - blend) + voice.surfacePos[j] * blend;

                // Reset velocities (always)
                std::memset(voice.surfaceVel, 0, sizeof(float) * static_cast<size_t>(paneCount));

                // Excite with velocity²
                exciteSurface(voice.surfaceVel, paneCount, velocity);

                voice.active            = true;
                voice.midiNote          = noteNum;
                voice.velocity          = velocity;
                voice.startTime         = voiceTimestamp++;
                voice.currentFrequency  = freq;
                voice.glideCoefficient  = 0.1f;  // gentle portamento in mono
                voice.isFadingOut       = false;
                voice.crossfadeGain     = 1.0f;
                voice.currentPaneCount  = paneCount;
                voice.scanPhase         = 0.0f;  // reset scan on Trigger mode

                voice.ampEnv.setParams(envA, envD, envS, envR, sampleRateFloat);
                voice.ampEnv.noteOn();
                voice.modEnv.setParams(envA * 0.5f, envD, envS * 0.8f, envR * 1.2f, sampleRateFloat);
                voice.modEnv.noteOn();

                voice.lfo1.setRate(lfoRate, sampleRateFloat);
                voice.lfo1.setShape(lfoShape);
                voice.lfo1.reset();

                voice.neonFilterL.reset();
                voice.neonFilterL.setMode(CytomicSVF::Mode::LowPass);
                voice.neonFilterL.setCoefficients(cutoff, reso, sampleRateFloat);
                voice.neonFilterR.reset();
                voice.neonFilterR.setMode(CytomicSVF::Mode::LowPass);
                voice.neonFilterR.setCoefficients(cutoff, reso, sampleRateFloat);
            }
            return;
        }

        // ---- Polyphonic mode ----
        int voiceIdx = findVoiceForNoteOn(maxPoly);
        auto& voice  = voices[static_cast<size_t>(voiceIdx)];

        // If stealing an active voice, crossfade to prevent click
        if (voice.active)
        {
            voice.isFadingOut   = true;
            voice.crossfadeGain = std::min(voice.crossfadeGain, 0.5f);
        }

        // Re-init surface
        float blend = surfaceMemory;
        float newPos[kOgiveMaxPanes];
        initSurface(newPos, paneCount, waveform, waveAsym);
        for (int j = 0; j < paneCount; ++j)
            voice.surfacePos[j] = newPos[j] * (1.0f - blend) + voice.surfacePos[j] * blend;

        std::memset(voice.surfaceVel, 0, sizeof(float) * static_cast<size_t>(paneCount));
        exciteSurface(voice.surfaceVel, paneCount, velocity);

        voice.active            = true;
        voice.midiNote          = noteNum;
        voice.velocity          = velocity;
        voice.startTime         = voiceTimestamp++;
        voice.currentFrequency  = freq;
        voice.targetFrequency   = freq;
        voice.glideCoefficient  = 1.0f;  // snap in poly mode
        voice.isFadingOut       = false;
        voice.crossfadeGain     = 1.0f;
        voice.currentPaneCount  = paneCount;
        voice.scanPhase         = 0.0f;

        voice.ampEnv.setParams(envA, envD, envS, envR, sampleRateFloat);
        voice.ampEnv.noteOn();
        voice.modEnv.setParams(envA * 0.5f, envD, envS * 0.8f, envR * 1.2f, sampleRateFloat);
        voice.modEnv.noteOn();

        voice.lfo1.setRate(lfoRate, sampleRateFloat);
        voice.lfo1.setShape(lfoShape);
        voice.lfo1.reset();

        voice.neonFilterL.reset();
        voice.neonFilterL.setMode(CytomicSVF::Mode::LowPass);
        voice.neonFilterL.setCoefficients(cutoff, reso, sampleRateFloat);
        voice.neonFilterR.reset();
        voice.neonFilterR.setMode(CytomicSVF::Mode::LowPass);
        voice.neonFilterR.setCoefficients(cutoff, reso, sampleRateFloat);
    }

    void noteOff(int noteNum)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.midiNote == noteNum && !voice.isFadingOut)
            {
                voice.ampEnv.noteOff();
                voice.modEnv.noteOff();
            }
        }
    }

    // Find best voice slot for a new note (LRU steal)
    int findVoiceForNoteOn(int maxPoly) noexcept
    {
        const int limit = juce::jmin(maxPoly, static_cast<int>(voices.size()));
        // Pass 1: find an inactive slot
        for (int i = 0; i < limit; ++i)
            if (!voices[static_cast<size_t>(i)].active)
                return i;
        // Pass 2: steal the oldest (LRU)
        int      oldest     = 0;
        uint64_t oldestTime = UINT64_MAX;
        for (int i = 0; i < limit; ++i)
        {
            if (voices[static_cast<size_t>(i)].startTime < oldestTime)
            {
                oldestTime = voices[static_cast<size_t>(i)].startTime;
                oldest     = i;
            }
        }
        return oldest;
    }

    //==========================================================================
    //  M E M B E R   D A T A
    //==========================================================================

    // ---- Audio configuration (set in prepare()) ----
    // Do not default-init — must be set by prepare() on the live sample rate.
    // Sentinel 0.0 makes misuse before prepare() a crash instead of silent wrong-rate DSP.
    double sampleRateDouble = 0.0;
    float  sampleRateFloat  = 0.0f;
    float  paramSmoothCoeff = 0.1f;
    float  voiceFadeRate    = 0.01f;

    // ---- Voice pool ----
    std::array<OgiveVoice, kOgiveMaxVoices> voices;
    uint64_t voiceTimestamp = 0;

    // ---- Smoothed control parameters ----
    float smoothedCutoff    = 4000.0f;
    float smoothedReso      = 0.3f;
    float smoothedDrive     = 0.15f;
    float smoothedScanRate  = 1.0f;
    float smoothedScanPos   = 0.5f;
    float smoothedStiffness = 0.45f;
    float smoothedDamping   = 0.02f;
    float smoothedGlassQ    = 60.0f;

    // ---- D006: expression ----
    float modWheelValue   = 0.0f;  // CC1 → scan pos
    float aftertouchValue = 0.0f;  // aftertouch / CC11 expression → drive
    float pitchBendNorm   = 0.0f;  // ±1

    // ---- Last noteOn state ----
    float lastNoteVelocity = 0.0f;

    // ---- Coupling accumulators (reset each block) ----
    float couplingFMAccum     = 0.0f;  // scan rate modulation
    float couplingExciteAccum = 0.0f;  // surface excitation
    float couplingSyncAccum   = 0.0f;  // phase sync nudge
    float couplingFilterAccum = 0.0f;  // neon cutoff modulation
    float cachedCouplingOutput = 0.0f; // O(1) coupling read

    // ---- Output cache for coupling reads ----
    std::vector<float> outputCacheLeft;
    std::vector<float> outputCacheRight;

    // ---- Chorus (Juno-style) ----
    std::vector<float> chorusBufL;
    std::vector<float> chorusBufR;
    int   chorusWriteIdx = 0;
    float chorusReadPos  = 0.0f;
    float chorusLfoPhase = 0.0f;

    // ---- Gated Reverb (FDN 4-line) ----
    std::vector<float> fdnBuf0;  // 1481 samples
    std::vector<float> fdnBuf1;  // 1823 samples
    std::vector<float> fdnBuf2;  // 2141 samples
    std::vector<float> fdnBuf3;  // 2503 samples
    int   fdnIdx0 = 0, fdnIdx1 = 0, fdnIdx2 = 0, fdnIdx3 = 0;
    float fdnState0 = 0.0f, fdnState1 = 0.0f, fdnState2 = 0.0f, fdnState3 = 0.0f;
    float fdnGateLevel = 0.0f;
    bool  fdnGateOpen  = false;

    // ---- Random number generator (for noise waveform + thermal drift) ----
    juce::Random rng;

    // ---- Cached APVTS parameter pointers (ParamSnapshot pattern) ----
    // Glass surface
    std::atomic<float>* pWaveform      = nullptr;
    std::atomic<float>* pWaveAsym      = nullptr;
    std::atomic<float>* pPaneCount     = nullptr;
    std::atomic<float>* pTraceryStiff  = nullptr;
    std::atomic<float>* pLeadDamp      = nullptr;
    std::atomic<float>* pGlassQ        = nullptr;
    // Scan
    std::atomic<float>* pScanRate      = nullptr;
    std::atomic<float>* pScanPos       = nullptr;
    std::atomic<float>* pLancetShape   = nullptr;
    std::atomic<float>* pScanMode      = nullptr;
    std::atomic<float>* pSyncDiv       = nullptr;
    std::atomic<float>* pSurfaceMemory = nullptr;
    // Neon
    std::atomic<float>* pNeonCutoff    = nullptr;
    std::atomic<float>* pNeonReso      = nullptr;
    std::atomic<float>* pNeonDrive     = nullptr;
    std::atomic<float>* pChorusDepth   = nullptr;
    // Chorus + Space
    std::atomic<float>* pChorusRate    = nullptr;
    std::atomic<float>* pGateTime      = nullptr;
    std::atomic<float>* pNaveSize      = nullptr;
    std::atomic<float>* pNaveMix       = nullptr;
    // Modulation
    std::atomic<float>* pLfo1Rate      = nullptr;
    std::atomic<float>* pLfo1Shape     = nullptr;
    std::atomic<float>* pLfo1Depth     = nullptr;
    std::atomic<float>* pEnvAttack     = nullptr;
    std::atomic<float>* pEnvDecay      = nullptr;
    std::atomic<float>* pEnvSustain    = nullptr;
    std::atomic<float>* pEnvRelease    = nullptr;
    // Performance
    std::atomic<float>* pVoiceMode     = nullptr;
    std::atomic<float>* pThermal       = nullptr;
    // Macros
    std::atomic<float>* pMacro1        = nullptr;
    std::atomic<float>* pMacro2        = nullptr;
    std::atomic<float>* pMacro3        = nullptr;
    std::atomic<float>* pMacro4        = nullptr;
};

} // namespace xoceanus
