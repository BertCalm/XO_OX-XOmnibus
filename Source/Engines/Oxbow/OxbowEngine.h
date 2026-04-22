// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../Core/SynthEngine.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/StandardLFO.h"
#include <array>
#include <cmath>

namespace xoceanus
{

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

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        // blockSize stored for future sub-blocking use; not consumed in renderBlock() today.

        // FDN delay lines: prime sample lengths (Chiasmus topology)
        // L channels: [1361, 1847, 2203, 2711] at 48kHz (~28, 38, 46, 56ms)
        // R channels: REVERSED [2711, 2203, 1847, 1361]
        // Scale to actual sample rate
        static constexpr float baseDelaysL[4] = {0.02835f, 0.03848f, 0.04590f, 0.05648f};
        static constexpr float baseDelaysR[4] = {0.05648f, 0.04590f, 0.03848f, 0.02835f};

        for (int ch = 0; ch < kFDNChannels; ++ch)
        {
            // Values are in seconds (e.g. 0.02835 s = 28.35 ms); variable renamed delaySec
            // to avoid confusion with the old "delayMs" label that incorrectly implied
            // milliseconds (stability fix: variable name matched unit comment, not actual value).
            float delaySec = (ch < 4) ? baseDelaysL[ch] : baseDelaysR[ch - 4];
            int delaySamples = static_cast<int>(delaySec * sr) + 1;
            if (delaySamples < 1)
                delaySamples = 1;
            fdnDelay[ch].assign(static_cast<size_t>(delaySamples), 0.0f);
            fdnDelaySize[ch] = delaySamples;
            fdnWritePos[ch] = 0;
        }

        // FDN damping filters: CytomicSVF LP per channel (Moog two-pole)
        for (int ch = 0; ch < kFDNChannels; ++ch)
        {
            fdnDamp[ch].setMode(CytomicSVF::Mode::LowPass);
            fdnDamp[ch].setCoefficients(dampingHz, 0.1f, static_cast<float>(sr));
            fdnDamp[ch].reset();
        }

        // Phase erosion allpass filters (4 per channel = 8 total)
        for (int a = 0; a < kErosionAPFs; ++a)
        {
            erosionAPF_L[a].setMode(CytomicSVF::Mode::AllPass);
            erosionAPF_R[a].setMode(CytomicSVF::Mode::AllPass);
            erosionAPF_L[a].reset();
            erosionAPF_R[a].reset();
        }

        // Erosion LFOs: 4 sine LFOs at very slow rates
        for (int a = 0; a < kErosionAPFs; ++a)
        {
            erosionLFO[a].setShape(StandardLFO::Sine);
            erosionLFO[a].setRate(0.03f + 0.02f * a, static_cast<float>(sr));
            erosionLFO[a].setPhaseOffset(static_cast<float>(a) * 0.25f);
            erosionLFO[a].reset();
        }

        // Golden resonance: 4 CytomicSVF Peak filters
        for (int g = 0; g < kGoldenFilters; ++g)
        {
            goldenL[g].setMode(CytomicSVF::Mode::Peak);
            goldenR[g].setMode(CytomicSVF::Mode::Peak);
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

        // Pre-delay buffer — fix #176: derive size from the buffer itself so
        // predelaySize can never fall out of sync with predelayBuf.size().
        int predelaySamples = static_cast<int>(0.2 * sr) + 1;
        predelayBuf.assign(static_cast<size_t>(predelaySamples), 0.0f);
        predelayPos = 0;

        // Silence gate configured by the processor via prepareSilenceGate()
        // which is called after prepare(). Hold time: 500ms (reverb-tail
        // category — Chiasmus FDN + pre-delay). See silenceGateHoldMs() in
        // XOceanusProcessor.cpp. Do NOT call silenceGate.setHoldTime() here;
        // the processor call overwrites it and is the single source of truth.

        // Smoothers for zipper-prone params: 20ms ramp at current sample rate
        smoothDryWet.reset(sampleRate, 0.020);
        smoothDryWet.setCurrentAndTargetValue(0.5f);
        smoothDecay.reset(sampleRate, 0.020);
        smoothDecay.setCurrentAndTargetValue(4.0f);

        // Cache convergence/resonance smoother coefficients — these use
        // hardcoded time constants so they need only recompute on sample rate
        // changes (i.e., here in prepare()), not every block.
        const float srF = static_cast<float>(sampleRate);
        cachedConvAttack  = smoothCoeffFromTime(0.01f,  srF);
        cachedConvRelease = smoothCoeffFromTime(0.1f,   srF);
        cachedResAttack   = smoothCoeffFromTime(0.005f, srF);
        cachedResRelease  = smoothCoeffFromTime(0.05f,  srF);

        // Reseed noise RNG from sample-rate value so multi-engine instances
        // produce decorrelated exciter bursts.
        noiseRng = static_cast<uint32_t>(static_cast<int>(sampleRate)) ^ 0xDEAD1234u;
    }

    void releaseResources() override {}

    void reset() override
    {
        for (int ch = 0; ch < kFDNChannels; ++ch)
        {
            std::fill(fdnDelay[ch].begin(), fdnDelay[ch].end(), 0.0f);
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
            std::fill(predelayBuf.begin(), predelayBuf.end(), 0.0f);
        predelayPos = 0;
        midEnv = sideEnv = 0.0f;
        resonanceGain = 0.0f;
        exciterEnv = 0.0f;
        exciterActive = false;
        peakEnergy = 0.0001f;
        currentEnergy = 0.0f;
        lastSampleL = lastSampleR = 0.0f;
        lastCantileverDamp = dampingHz; // force coefficient recompute on next block
        smoothDryWet.setCurrentAndTargetValue(smoothDryWet.getTargetValue());
        smoothDecay.setCurrentAndTargetValue(smoothDecay.getTargetValue());
    }

    //--------------------------------------------------------------------------
    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
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

                // Reseed noise RNG per note-on so successive notes produce
                // varied exciter bursts rather than the same deterministic
                // XOR-shift sequence every time (was fixed seed 42u / prepare seed).
                noiseRng ^= static_cast<uint32_t>(currentNote * 1664525u + 1013904223u);
                if (noiseRng == 0u)
                    noiseRng = 0xBADC0FFEu; // guard against zero state

                // Update golden resonance fundamentals from MIDI note (Moog)
                float fundamental = midiToFreq(currentNote) *
                                    PitchBendUtil::semitonesToFreqRatio(
                                        PitchBendUtil::bendToSemitones(pitchBendNorm, 2.0f));
                updateGoldenFrequencies(fundamental);

                // Reset peak energy tracker for new cantilever arc.
                // Seed with current energy so a re-triggered note during a live
                // FDN tail does not immediately set decayProgress=1.0 and apply
                // maximum cantilever damping (was: hard reset to 0.0001f which
                // makes currentEnergy >> peakEnergy on first sample after retrigger).
                peakEnergy = std::max(currentEnergy, 0.0001f);
            }
            else if (msg.isNoteOff())
            {
                // Don't kill exciter immediately — let envelope decay
            }
            else if (msg.isAftertouch() || msg.isChannelPressure())
            {
                // Aftertouch → entanglement (Vangelis)
                float pressure =
                    msg.isAftertouch() ? msg.getAfterTouchValue() / 127.0f : msg.getChannelPressureValue() / 127.0f;
                aftertouch = pressure;
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                // Mod wheel → resonance mix scale (D006)
                modWheel_ = msg.getControllerValue() / 127.0f;
            }
            else if (msg.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
        }

        // 2. Bypass check — reset coupling mods even when bypassed so they
        // don't accumulate across blocks and cause a step transient on wake.
        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            extFilterMod = 0.0f;
            extDecayMod  = 0.0f;
            extRingMod   = 0.0f;
            return;
        }

        // 3. Read parameters
        float pSize = pSizeParam ? pSizeParam->load() : 0.5f;
        float pDecay = pDecayParam ? pDecayParam->load() : 4.0f;
        float pEntangle = pEntangleParam ? pEntangleParam->load() : 0.6f;
        float pErosionR = pErosionRParam ? pErosionRParam->load() : 0.08f;
        float pErosionD = pErosionDParam ? pErosionDParam->load() : 0.4f;
        float pConverge = pConvergeParam ? pConvergeParam->load() : 4.0f;
        float pResQ = pResQParam ? pResQParam->load() : 8.0f;
        float pResMix = pResMixParam ? pResMixParam->load() : 0.3f;
        float pCantilever = pCantileverParam ? pCantileverParam->load() : 0.5f;
        float pDamping = pDampingParam ? pDampingParam->load() : 6000.0f;
        float pPredelay = pPredelayParam ? pPredelayParam->load() : 20.0f;
        float pDryWet = pDryWetParam ? pDryWetParam->load() : 0.5f;
        float pExcDecay = pExcDecayParam ? pExcDecayParam->load() : 0.01f;
        float pExcBright = pExcBrightParam ? pExcBrightParam->load() : 0.7f;

        // Macros
        float pMacroCharacter = pMacroCharacterParam ? pMacroCharacterParam->load() : 0.0f;
        float pMacroMovement = pMacroMovementParam ? pMacroMovementParam->load() : 0.0f;
        float pMacroCoupling = pMacroCouplingParam ? pMacroCouplingParam->load() : 0.0f;
        float pMacroSpace = pMacroSpaceParam ? pMacroSpaceParam->load() : 0.0f;

        // CHARACTER → exciter brightness + resonance timbral focus
        pExcBright = clamp(pExcBright + pMacroCharacter * 0.3f, 0.0f, 1.0f);
        pResMix = clamp(pResMix + pMacroCharacter * 0.25f, 0.0f, 1.0f);

        // MOVEMENT → phase erosion depth + rate
        pErosionD = clamp(pErosionD + pMacroMovement * 0.4f, 0.0f, 1.0f);
        pErosionR = clamp(pErosionR + pMacroMovement * 0.15f, 0.01f, 0.5f);

        // COUPLING → entanglement (summed with aftertouch below)
        pEntangle = clamp(pEntangle + pMacroCoupling * 0.4f, 0.0f, 1.0f);

        // SPACE → dry/wet mix + room size
        pDryWet = clamp(pDryWet + pMacroSpace * 0.25f, 0.0f, 1.0f);
        pSize = clamp(pSize + pMacroSpace * 0.3f, 0.0f, 1.0f);
        smoothDryWet.setTargetValue(pDryWet); // set after all macro contributions

        // Mod wheel → resonance mix (D006)
        pResMix = clamp(pResMix + modWheel_ * 0.5f, 0.0f, 1.0f);

        // Apply aftertouch to entanglement (Vangelis)
        pEntangle = clamp(pEntangle + aftertouch * 0.3f, 0.0f, 1.0f);

        // Apply coupling modulation
        pDamping = clamp(pDamping + extFilterMod, 200.0f, 16000.0f);
        pDecay = clamp(pDecay + extDecayMod * 10.0f, 0.1f, 60.0f);
        smoothDecay.setTargetValue(pDecay); // set after coupling modulation

        const float srF = static_cast<float>(sr);

        // Feedback coefficient from decay time.
        // Schulze: allow infinite decay (pDecay > 29s → feedback = 1.0).
        // Use std::exp here — called once per block, not per sample.  fastExp
        // carries ~4% error near the infinite-decay threshold where accuracy matters most.
        // NOTE: smoothDecay.getNextValue() advances the smoother by exactly one step —
        // calling it only once per block means the full per-sample smoothing benefit is
        // lost within a block. We use getTargetValue() for the block-level coefficient and
        // accept one-block latency, which is inaudible at typical block sizes (64–512 samples).
        float smoothedDecayVal = smoothDecay.getTargetValue();
        // Advance the smoother so its internal state tracks correctly for the next block.
        smoothDecay.getNextValue();
        float feedbackCoeff = (smoothedDecayVal > 29.0f) ? 1.0f : std::exp(-6.9078f / (smoothedDecayVal * srF));
        feedbackCoeff = std::min(feedbackCoeff, 0.9999f); // fleet standard: prevent FDN divergence

        // Size → room dimension (D004: dead param resolved)
        // Size 0 = intimate (short predelay, dark/absorptive)
        // Size 1 = vast hall (full predelay, bright/reflective)
        float effectivePredelay = pPredelay * (0.1f + pSize * 0.9f);
        pDamping = clamp(pDamping * (0.5f + pSize * 1.0f), 200.0f, 16000.0f);
        // pResQ is used per-sample below (golden resonator Q tracks knob in real-time).

        // Velocity → exciter brightness and decay (D001 + Kakehashi)
        float excBrightness = pExcBright * (0.5f + currentVelocity * 0.5f);
        float excLength = pExcDecay * (0.5f + currentVelocity * 0.5f);
        float excDecayFinal = fastExp(-6.9078f / (excLength * srF));

        // Pre-delay in samples — clamp against actual buffer size (fix #176)
        float predelaySamples =
            clamp(effectivePredelay * 0.001f * srF, 0.0f, static_cast<float>((int)predelayBuf.size() - 1));

        // Convergence envelope coefficients — use values cached in prepare()
        // to avoid 4× fastExp() calls per block (smoothCoeffFromTime calls
        // fastExp internally; time constants are fixed so caching is exact).
        const float convAttack  = cachedConvAttack;
        const float convRelease = cachedConvRelease;
        const float resAttack   = cachedResAttack;
        const float resRelease  = cachedResRelease;

        // Golden ratio harmonics amplitude weighting (Tomita: -3dB per φ)
        static constexpr float goldenGains[4] = {1.0f, 0.708f, 0.501f, 0.354f};

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        // Pre-compute pitch-bent exciter frequency for this block.
        // midiToFreq() takes int — the old float cast was a type mismatch (compiled
        // via implicit float→int truncation, always correct since currentNote is
        // already integer, but misleading). Use PitchBendUtil::bendToSemitones()
        // for the range, consistent with updateGoldenFrequencies().
        // Nyquist guard: clamp to 0.45 × sample rate so very high notes + pitch
        // bend can't push fastSin past the sample rate (would alias even though
        // phase wraps). 0.45 leaves headroom for any coupling-driven pitch wobble.
        const float exciterFreqHz = std::min(
            midiToFreq(static_cast<float>(currentNote)) * PitchBendUtil::semitonesToFreqRatio(pitchBendNorm * 2.0f),
            srF * 0.45f);

        // Block-constant cross-channel entanglement mix (was recomputed per sample
        // inside the FDN channel loop, even though pEntangle is block-rate).
        const float entangleMix = pEntangle * 0.3f;

        // Pre-scaled FDN input divisor — kFDNChannels is a compile-time constant (8),
        // so this is just `0.125f`. Named for clarity; any sane compiler folds it.
        constexpr float fdnInputScale = 1.0f / static_cast<float>(kFDNChannels);

        // Issue #917: hoist erosionLFO.setRate() out of per-sample loop.
        // pErosionR is block-stable (read from atomic above). StandardLFO::setRate()
        // only updates phaseInc — it does NOT reset phase or lastPhase — so hoisting
        // is behaviorally identical and saves ~4 * numSamples calls per block.
        for (int a = 0; a < kErosionAPFs; ++a)
            erosionLFO[a].setRate(pErosionR + 0.01f * a, srF);

        // Hoist golden resonator coefficients out of the per-sample loop.
        // Both inputs are block-constant:
        //   goldenFreqHz[g] is cached at note-on (line ~696) and stable until next noteOn
        //   liveQ = pResQ / 20.0f comes from a block-rate atomic load
        // Previously these setCoefficients_fast calls ran kGoldenFilters × numSamples
        // times per block when resonance gate was open. Now they run kGoldenFilters.
        {
            const float liveQ = pResQ / 20.0f;
            for (int g = 0; g < kGoldenFilters; ++g)
            {
                goldenL[g].setCoefficients_fast(goldenFreqHz[g], liveQ, srF);
                goldenR[g].setCoefficients_fast(goldenFreqHz[g], liveQ, srF);
            }
        }

        for (int i = 0; i < numSamples; ++i)
        {
            // === EXCITER: pitched impulse + noise burst ===
            float exciterSample = 0.0f;
            if (exciterActive && exciterEnv > 0.0001f)
            {
                float sine = fastSin(static_cast<float>(exciterPhase) * 6.28318530718f);
                exciterPhase += exciterFreqHz / sr;
                if (exciterPhase >= 1.0)
                    exciterPhase -= 1.0;

                // Noise component (brightness controls noise amount)
                uint32_t r = noiseRng;
                r ^= r << 13;
                r ^= r >> 17;
                r ^= r << 5;
                noiseRng = r;
                float noise = (static_cast<float>(r & 0xFFFF) / 32768.0f - 1.0f) * excBrightness;

                exciterSample = (sine * (1.0f - excBrightness * 0.5f) + noise) * exciterEnv;
                exciterEnv *= excDecayFinal;
                exciterEnv = flushDenormal(exciterEnv);

                if (exciterEnv < 0.0001f)
                    exciterActive = false;
            }

            // === PRE-DELAY ===
            predelayBuf[static_cast<size_t>(predelayPos)] = exciterSample;
            const int predelayBufSize = (int)predelayBuf.size(); // fix #176: always in sync
            int readPos = (predelayPos - static_cast<int>(predelaySamples) + predelayBufSize) % predelayBufSize;
            float fdnInput = predelayBuf[static_cast<size_t>(readPos)];
            predelayPos = (predelayPos + 1) % predelayBufSize;

            // === 8-CHANNEL CHIASMUS FDN ===
            // Read from all 8 delay lines.
            // Execution order each sample: READ → COMPUTE → WRITE → ADVANCE.
            // Since WRITE and ADVANCE happen after this read, fdnWritePos[ch]
            // currently points to the oldest sample in the ring — the slot we
            // are about to overwrite.  That slot was last written fdnDelaySize[ch]
            // samples ago, giving the full intended delay.
            float fdnRead[kFDNChannels];
            for (int ch = 0; ch < kFDNChannels; ++ch)
            {
                // fdnWritePos[ch] is the oldest slot (read-before-write-before-advance).
                // flushDenormal before matrix multiply: prevents denormal CPU spikes in long-decay tails (T60 up to 60s).
                fdnRead[ch] = flushDenormal(fdnDelay[ch][static_cast<size_t>(fdnWritePos[ch])]);
            }

            // Householder feedback matrix: H = I - (2/N) * 1*1^T
            // For N=8: H[i][j] = (i==j) ? 0.75 : -0.25
            float fdnSum = 0.0f;
            for (int ch = 0; ch < kFDNChannels; ++ch)
                fdnSum += fdnRead[ch];
            fdnSum *= (2.0f / static_cast<float>(kFDNChannels)); // = sum * 0.25

            float fdnOut[kFDNChannels];
            for (int ch = 0; ch < kFDNChannels; ++ch)
                fdnOut[ch] = fdnRead[ch] - fdnSum;

            // Cross-coupling: entanglement blends L↔R channels
            // At entangle=0: channels are independent. At 1: fully blended.
            // entangleMix precomputed above per block.
            for (int ch = 0; ch < 4; ++ch)
            {
                float lCh = fdnOut[ch];
                float rCh = fdnOut[ch + 4];
                fdnOut[ch] = lCh * (1.0f - entangleMix) + rCh * entangleMix;
                fdnOut[ch + 4] = rCh * (1.0f - entangleMix) + lCh * entangleMix;
            }

            // Cantilever decay: asymmetric time-varying damping
            currentEnergy =
                flushDenormal(currentEnergy * 0.9999f + 0.0001f * (std::fabs(fdnOut[0]) + std::fabs(fdnOut[4])));
            if (currentEnergy > peakEnergy)
                peakEnergy = currentEnergy;

            float decayProgress = 1.0f - clamp(currentEnergy / (peakEnergy + 1e-8f), 0.0f, 1.0f);
            float cantileverDamp = pDamping * (1.0f - pCantilever * decayProgress * decayProgress);
            cantileverDamp = clamp(cantileverDamp, 200.0f, 16000.0f);

            // Apply damping per channel (Moog two-pole SVF LP).
            // Only recompute SVF coefficients when cantileverDamp changes by more
            // than 1 Hz — this avoids 8 setCoefficients_fast() calls per sample
            // when the damping is stable (cantilever damp is a slow quadratic
            // function of currentEnergy that changes by only a few Hz per sample
            // during normal decay; updating every sample wastes ~8 fastTan calls).
            if (std::fabs(cantileverDamp - lastCantileverDamp) > 1.0f)
            {
                for (int ch = 0; ch < kFDNChannels; ++ch)
                    fdnDamp[ch].setCoefficients_fast(cantileverDamp, 0.1f, srF);
                lastCantileverDamp = cantileverDamp;
            }
            for (int ch = 0; ch < kFDNChannels; ++ch)
                fdnOut[ch] = fdnDamp[ch].processSample(fdnOut[ch]);

            // Write back to delay lines with feedback + input injection.
            // fdnInputScaled is block-constant-per-sample — hoist the divide out
            // of the channel loop.
            const float fdnInputScaled = fdnInput * fdnInputScale;
            for (int ch = 0; ch < kFDNChannels; ++ch)
            {
                float fb = flushDenormal(fdnOut[ch] * feedbackCoeff);
                fdnDelay[ch][static_cast<size_t>(fdnWritePos[ch])] = fdnInputScaled + fb;
                fdnWritePos[ch] = (fdnWritePos[ch] + 1) % fdnDelaySize[ch];
            }

            // Sum FDN channels to stereo: 1-4 → L, 5-8 → R
            float fdnL = fdnOut[0] + fdnOut[1] + fdnOut[2] + fdnOut[3];
            float fdnR = fdnOut[4] + fdnOut[5] + fdnOut[6] + fdnOut[7];
            fdnL *= 0.25f; // normalize
            fdnR *= 0.25f;

            // === PHASE EROSION: opposite-polarity allpass LFOs ===
            float erosionL = fdnL;
            float erosionR = fdnR;

            for (int a = 0; a < kErosionAPFs; ++a)
            {
                float lfoVal = erosionLFO[a].process();
                // setRate() hoisted above the per-sample loop (Issue #917)

                static constexpr float erosionBaseFreqs[4] = {300.0f, 1100.0f, 3200.0f, 7500.0f};
                float depth = pErosionD * 0.4f;

                // L and R modulated with OPPOSITE polarity
                float freqL = erosionBaseFreqs[a] * (1.0f + lfoVal * depth);
                float freqR = erosionBaseFreqs[a] * (1.0f - lfoVal * depth); // OPPOSITE
                freqL = clamp(freqL, 20.0f, srF * 0.49f);
                freqR = clamp(freqR, 20.0f, srF * 0.49f);

                erosionAPF_L[a].setCoefficients_fast(freqL, 0.5f, srF);
                erosionAPF_R[a].setCoefficients_fast(freqR, 0.5f, srF);

                erosionL = erosionAPF_L[a].processSample(erosionL);
                erosionR = erosionAPF_R[a].processSample(erosionR);
            }

            // === GOLDEN RESONANCE: Mid/Side convergence detection ===
            float mid = (erosionL + erosionR) * 0.5f;
            float side = (erosionL - erosionR) * 0.5f;

            float absMid = std::fabs(mid);
            float absSide = std::fabs(side);

            float midCoeff = (absMid > midEnv) ? convAttack : convRelease;
            float sideCoeff = (absSide > sideEnv) ? convAttack : convRelease;
            midEnv = flushDenormal(midEnv + midCoeff * (absMid - midEnv));
            sideEnv = flushDenormal(sideEnv + sideCoeff * (absSide - sideEnv));

            float convergence = (sideEnv > 1e-6f) ? midEnv / sideEnv : 0.0f;

            // Attack/release resonance gain based on convergence
            if (convergence > pConverge)
                resonanceGain = clamp(resonanceGain + resAttack * (1.0f - resonanceGain), 0.0f, 1.0f);
            else
                resonanceGain = flushDenormal(resonanceGain * (1.0f - resRelease));

            // Apply golden ratio bandpass filters with amplitude weighting
            float goldenOutL = 0.0f;
            float goldenOutR = 0.0f;

            if (resonanceGain > 0.001f)
            {
                // Update golden resonator Q from live parameter so Q knob responds
                // during a held note (D004: dead param fix — pResQ was previously only
                // read at note-on, making the knob inert until the next key strike).
                // Cap at 0.95 to prevent self-oscillation: CytomicSVF Peak at
                // resonance=1.0 → k=0 → self-oscillating sinusoid injected into the
                // FDN tail (stability fix: unbound Q caused runaway energy buildup).
                // Golden resonator coefficients are set once per block above (hoisted
                // out of this per-sample loop — goldenFreqHz + liveQ are both block-
                // constant). Per-sample processing still runs; only the expensive
                // coefficient refresh moved.
                for (int g = 0; g < kGoldenFilters; ++g)
                {
                    goldenOutL += goldenL[g].processSample(erosionL) * goldenGains[g];
                    goldenOutR += goldenR[g].processSample(erosionR) * goldenGains[g];
                }
                goldenOutL *= resonanceGain * pResMix;
                goldenOutR *= resonanceGain * pResMix;
            }

            // === Final output ===
            float wetL = erosionL + goldenOutL;
            float wetR = erosionR + goldenOutR;

            // Ring mod coupling
            if (std::fabs(extRingMod) > 0.001f)
            {
                wetL *= (1.0f + extRingMod);
                wetR *= (1.0f + extRingMod);
            }

            // Dry/wet mix (dry = exciter, wet = reverb).
            // Use per-sample smoothed value to eliminate zipper noise on fader moves.
            float dryWet = smoothDryWet.getNextValue();
            float finalL = exciterSample * (1.0f - dryWet) + wetL * dryWet;
            float finalR = exciterSample * (1.0f - dryWet) + wetR * dryWet;

            // Cache for coupling output
            lastSampleL = finalL;
            lastSampleR = finalR;

            // Accumulate into buffer (engines ADD, never overwrite)
            outL[i] += finalL;
            if (outR)
                outR[i] += finalR;
        }

        // Reset coupling mods for next block
        extFilterMod = 0.0f;
        extDecayMod = 0.0f;
        extRingMod = 0.0f;

        // Silence gate analysis
        const float* rL = buffer.getReadPointer(0);
        const float* rR = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : nullptr;
        silenceGate.analyzeBlock(rL, rR, numSamples);
    }

    //-- Coupling ---------------------------------------------------------------

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? lastSampleL : lastSampleR;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* sourceBuffer, int /*numSamples*/) override
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
            // Clamp to [-0.9, 0.9] so the gain factor (1 + extRingMod) stays
            // in [0.1, 1.9] — prevents polarity inversion and runaway amplification
            // from loud source engines (e.g. ONSET transients at +6 dB would produce
            // extRingMod ≈ 2.0 → gain ≈ 3.0 without this guard).
            extRingMod = clamp((sourceBuffer ? sourceBuffer[0] : 0.0f) * amount, -0.9f, 0.9f);
            break;
        case CouplingType::AudioToBuffer:
            // DEFERRED: inject external audio as exciter signal into the FDN.
            // Intended design: sourceBuffer samples accumulate into a small ring
            // and are fed into fdnInput in renderBlock, replacing or mixing with
            // the exciterSample. Requires a dedicated coupling buffer member and
            // block-count tracking. Currently a no-op — external audio is silently
            // ignored. Tracked for implementation in a future pass.
            break;
        default:
            break;
        }
    }

    //-- Parameters -------------------------------------------------------------
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxb_size", 1}, "Space Size",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxb_decay", 1}, "Decay Time",
                                              juce::NormalisableRange<float>(0.1f, 60.0f, 0.0f, 0.3f), 4.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxb_entangle", 1}, "Entanglement",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.6f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxb_erosionRate", 1}, "Erosion Rate",
                                              juce::NormalisableRange<float>(0.01f, 0.5f, 0.0f, 0.5f), 0.08f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxb_erosionDepth", 1}, "Erosion Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxb_convergence", 1}, "Convergence",
                                              juce::NormalisableRange<float>(1.0f, 20.0f, 0.0f, 0.5f), 4.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxb_resonanceQ", 1}, "Resonance Focus",
                                              juce::NormalisableRange<float>(0.5f, 20.0f, 0.0f, 0.4f), 8.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxb_resonanceMix", 1}, "Resonance Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxb_cantilever", 1}, "Cantilever",
                                              juce::NormalisableRange<float>(0.0f, 1.0f),
                                              0.3f)); // Pearlman: init at 0.3

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxb_damping", 1}, "Damping",
                                              juce::NormalisableRange<float>(200.0f, 16000.0f, 0.0f, 0.3f), 6000.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxb_predelay", 1}, "Pre-Delay",
                                              juce::NormalisableRange<float>(0.0f, 200.0f, 0.0f, 0.5f), 20.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxb_dryWet", 1}, "Dry/Wet",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxb_exciterDecay", 1}, "Exciter Decay",
                                              juce::NormalisableRange<float>(0.001f, 0.1f, 0.0f, 0.5f), 0.01f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxb_exciterBright", 1}, "Exciter Bright",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));

        // Macros (CHARACTER / MOVEMENT / COUPLING / SPACE)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxb_macroCharacter", 1}, "Character",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxb_macroMovement", 1}, "Movement",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxb_macroCoupling", 1}, "Coupling",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxb_macroSpace", 1}, "Space",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pSizeParam = apvts.getRawParameterValue("oxb_size");
        pDecayParam = apvts.getRawParameterValue("oxb_decay");
        pEntangleParam = apvts.getRawParameterValue("oxb_entangle");
        pErosionRParam = apvts.getRawParameterValue("oxb_erosionRate");
        pErosionDParam = apvts.getRawParameterValue("oxb_erosionDepth");
        pConvergeParam = apvts.getRawParameterValue("oxb_convergence");
        pResQParam = apvts.getRawParameterValue("oxb_resonanceQ");
        pResMixParam = apvts.getRawParameterValue("oxb_resonanceMix");
        pCantileverParam = apvts.getRawParameterValue("oxb_cantilever");
        pDampingParam = apvts.getRawParameterValue("oxb_damping");
        pPredelayParam = apvts.getRawParameterValue("oxb_predelay");
        pDryWetParam = apvts.getRawParameterValue("oxb_dryWet");
        pExcDecayParam = apvts.getRawParameterValue("oxb_exciterDecay");
        pExcBrightParam = apvts.getRawParameterValue("oxb_exciterBright");
        pMacroCharacterParam = apvts.getRawParameterValue("oxb_macroCharacter");
        pMacroMovementParam = apvts.getRawParameterValue("oxb_macroMovement");
        pMacroCouplingParam = apvts.getRawParameterValue("oxb_macroCoupling");
        pMacroSpaceParam = apvts.getRawParameterValue("oxb_macroSpace");
    }

    //-- Identity ---------------------------------------------------------------

    juce::String getEngineId() const override { return "Oxbow"; }

    juce::Colour getAccentColour() const override
    {
        // Oxbow Teal — twilight zone, between light and dark
        return juce::Colour(0xFF1A6B5A);
    }

    int getMaxVoices() const override { return 1; } // Monophonic reverb instrument

private:
    //--------------------------------------------------------------------------
    void updateGoldenFrequencies(float fundamental)
    {
        // Golden ratio harmonics: f, f×φ, f×φ², f×φ³
        static constexpr float phi = 1.6180339887f;
        float freqs[kGoldenFilters] = {fundamental, fundamental * phi, fundamental * phi * phi,
                                       fundamental * phi * phi * phi};

        float resQ = pResQParam ? pResQParam->load() : 8.0f;
        float srF  = static_cast<float>(sr);

        for (int g = 0; g < kGoldenFilters; ++g)
        {
            float f = clamp(freqs[g], 20.0f, srF * 0.49f);
            goldenFreqHz[g] = f;                              // cache for per-sample Q updates
            // Cap at 0.95 — same guard as per-sample path to prevent self-oscillation
            // at note-on when setCoefficients() (full precision path) is used here.
            float q = clamp(resQ / 20.0f, 0.0f, 0.95f);
            goldenL[g].setCoefficients(f, q, srF, 6.0f);     // +6dB peak
            goldenR[g].setCoefficients(f, q, srF, 6.0f);
        }
    }

    //--------------------------------------------------------------------------
    static constexpr int kFDNChannels = 8;
    static constexpr int kErosionAPFs = 4;
    static constexpr int kGoldenFilters = 4;

    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    // blockSize removed — was set in prepare() but never read in renderBlock().

    // FDN delay lines (8 channels)
    std::vector<float> fdnDelay[kFDNChannels];
    int fdnDelaySize[kFDNChannels]{};
    int fdnWritePos[kFDNChannels]{};

    // FDN damping filters (Moog two-pole SVF LP)
    CytomicSVF fdnDamp[kFDNChannels];

    // Phase erosion allpass filters
    CytomicSVF erosionAPF_L[kErosionAPFs];
    CytomicSVF erosionAPF_R[kErosionAPFs];
    StandardLFO erosionLFO[kErosionAPFs];

    // Golden resonance filters
    CytomicSVF goldenL[kGoldenFilters];
    CytomicSVF goldenR[kGoldenFilters];
    // Golden resonator frequencies — updated at every note-on via updateGoldenFrequencies().
    // Defaults are φ-harmonics of C4 (261.63 Hz) so any pre-note-on trigger of the
    // golden resonance path (e.g. from audio-rate FDN convergence on a sustained tail)
    // produces a musically sensible ring rather than the old A3-arbitrary 220 Hz root.
    // (Previous default {220, 356, 576, 932} corresponded to A3 with no note played.)
    float goldenFreqHz[kGoldenFilters] = {261.63f, 423.14f, 684.27f, 1106.8f}; // C4 φ-harmonics
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

    // Pre-delay — fix #176: predelaySize removed; use predelayBuf.size() directly
    // to prevent the two values from diverging after a prepare() call.
    std::vector<float> predelayBuf;
    int predelayPos = 0;

    // sizeScale omitted — was constant 1.0f, never referenced in renderBlock().
    // pSize affects predelay and damping directly; FDN lengths fixed at prepare().
    float dampingHz = 6000.0f; // used only in prepare() initial filter setup

    // Last cantilever damp value — used for hysteresis-gated coefficient updates
    // in the per-sample FDN damping loop (avoids 8 setCoefficients_fast/sample).
    float lastCantileverDamp = 6000.0f;

    // Cached convergence/resonance smoother coefficients — computed once in
    // prepare() from hardcoded time constants.  Using fastExp per-block was
    // wasteful; these values only change when sample rate changes.
    float cachedConvAttack  = 1.0f;
    float cachedConvRelease = 1.0f;
    float cachedResAttack   = 1.0f;
    float cachedResRelease  = 1.0f;

    // Parameter smoothers for the two most audible zipper params (D002 / FATHOM fix)
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothDryWet;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothDecay;

    // Coupling state
    float extFilterMod = 0.0f;
    float extDecayMod = 0.0f;
    float extRingMod = 0.0f;
    float lastSampleL = 0.0f;
    float lastSampleR = 0.0f;

    // Parameter pointers
    float modWheel_ = 0.0f;
    float pitchBendNorm = 0.0f; // MIDI pitch wheel [-1, +1]; ±2 semitone range

    std::atomic<float>* pSizeParam = nullptr;
    std::atomic<float>* pDecayParam = nullptr;
    std::atomic<float>* pEntangleParam = nullptr;
    std::atomic<float>* pErosionRParam = nullptr;
    std::atomic<float>* pErosionDParam = nullptr;
    std::atomic<float>* pConvergeParam = nullptr;
    std::atomic<float>* pResQParam = nullptr;
    std::atomic<float>* pResMixParam = nullptr;
    std::atomic<float>* pCantileverParam = nullptr;
    std::atomic<float>* pDampingParam = nullptr;
    std::atomic<float>* pPredelayParam = nullptr;
    std::atomic<float>* pDryWetParam = nullptr;
    std::atomic<float>* pExcDecayParam = nullptr;
    std::atomic<float>* pExcBrightParam = nullptr;

    // Macro parameter pointers
    std::atomic<float>* pMacroCharacterParam = nullptr;
    std::atomic<float>* pMacroMovementParam = nullptr;
    std::atomic<float>* pMacroCouplingParam = nullptr;
    std::atomic<float>* pMacroSpaceParam = nullptr;
};

} // namespace xoceanus
