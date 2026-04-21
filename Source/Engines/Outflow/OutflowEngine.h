// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OutflowEngine.h — XOutflow | "The Predictive Spatial Engine"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOutflow is a generative fluid-dynamics spatial engine that anticipates
//      transients, weaponizing prediction errors to create sudden acoustic
//      vacuums and stochastic granular rhythm bursts. It doesn't react to
//      sound — it *predicts* it, and the space between prediction and reality
//      becomes the instrument.
//
//  ENGINE CONCEPT:
//      A 20ms lookahead delay line enables pre-transient detection. When the
//      Bayesian envelope predictor anticipates a transient, the VCA drops to
//      zero just before impact — the Pre-Wake Vacuum. The Undertow reverb
//      fights the vacuum with optical saturation: the harder the gate clamps,
//      the warmer the tail. Tidal Drift ages the room geometry over minutes
//      with a sub-audible LFO. The engine is monophonic — it processes audio
//      from its own exciter or from coupling input.
//
//  Accent: Deep Storm Indigo #1A1A40
//  Parameter prefix: out_
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/PitchBendUtil.h"
#include <array>
#include <cmath>
#include <vector>

namespace xoceanus
{

//==============================================================================
class OutflowEngine : public SynthEngine
{
public:
    OutflowEngine() = default;

    //-- Identity --------------------------------------------------------------
    juce::String getEngineId() const override { return "Outflow"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF1A1A40); }
    int getMaxVoices() const override { return 1; }

    //-- Lifecycle -------------------------------------------------------------
    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr_ = sampleRate;
        srF_ = static_cast<float>(sampleRate);
        blockSize_ = maxBlockSize;

        // 20ms lookahead buffer (Pre-Wake Vacuum)
        lookaheadSamples_ = static_cast<int>(0.020 * sr_) + 1;
        lookaheadBufL_.assign(static_cast<size_t>(lookaheadSamples_), 0.0f);
        lookaheadBufR_.assign(static_cast<size_t>(lookaheadSamples_), 0.0f);
        lookaheadPos_ = 0;

        // FDN Reverb (4-channel Householder)
        static constexpr float baseDelaysMs[kFDNChannels] = {29.3f, 37.1f, 43.7f, 53.9f};
        for (int ch = 0; ch < kFDNChannels; ++ch)
        {
            int delaySamples = static_cast<int>(baseDelaysMs[ch] * 0.001f * srF_) + 1;
            fdnDelay_[ch].assign(static_cast<size_t>(delaySamples), 0.0f);
            fdnDelaySize_[ch] = delaySamples;
            fdnWritePos_[ch] = 0;

            fdnDamp_[ch].setMode(CytomicSVF::Mode::LowPass);
            fdnDamp_[ch].setCoefficients(8000.0f, 0.1f, srF_);
            fdnDamp_[ch].reset();
        }

        // Bayesian envelope follower state
        envFollower_ = 0.0f;
        envPrediction_ = 0.0f;
        predictionError_ = 0.0f;
        vacuumVCA_ = 1.0f;

        // Tidal drift LFO (minutes-scale, D005 compliant)
        tidalLFO_.setShape(StandardLFO::Sine);
        tidalLFO_.setRate(0.001f, srF_); // ~16 minute full cycle
        tidalLFO_.reset();

        // Wind chaos LFO (stochastic reverb tail modulation)
        windLFO_.setShape(StandardLFO::Sine);
        windLFO_.setRate(0.3f, srF_);
        windLFO_.reset();

        // Exciter state
        exciterPhase_ = 0.0;
        exciterEnv_ = 0.0f;
        exciterActive_ = false;
        currentNote_ = 60;
        currentVelocity_ = 0.0f;
        aftertouch_ = 0.0f;
        modWheel_ = 0.0f;
        pitchBendNorm_ = 0.0f;

        // Coupling state
        lastSampleL_ = lastSampleR_ = 0.0f;
        extFilterMod_ = 0.0f;
        extRingMod_ = 0.0f;
        couplingEnvDecayMod_ = 0.0f;

        // Output saturation filters (L and R — must be separate instances for stereo symmetry)
        satFilter_.setMode(CytomicSVF::Mode::LowPass);
        satFilter_.setCoefficients(6000.0f, 0.5f, srF_);
        satFilter_.reset();
        satFilterR_.setMode(CytomicSVF::Mode::LowPass);
        satFilterR_.setCoefficients(6000.0f, 0.5f, srF_);
        satFilterR_.reset();

        // Seed noise RNG from sampleRate bits to avoid all instances producing identical
        // noise sequences on cold start (fixed seed 77 was identical across all plugin instances).
        uint32_t srBits;
        std::memcpy(&srBits, &srF_, sizeof(srBits));
        noiseRng_ = srBits ^ 0xDEAD1234u;

        // Silence gate: 1000ms (infinite-sustain category — reverb tail + vacuum)
        silenceGate.prepare(sr_, maxBlockSize);
        silenceGate.setHoldTime(1000.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        std::fill(lookaheadBufL_.begin(), lookaheadBufL_.end(), 0.0f);
        std::fill(lookaheadBufR_.begin(), lookaheadBufR_.end(), 0.0f);
        lookaheadPos_ = 0;

        for (int ch = 0; ch < kFDNChannels; ++ch)
        {
            std::fill(fdnDelay_[ch].begin(), fdnDelay_[ch].end(), 0.0f);
            fdnWritePos_[ch] = 0;
            fdnDamp_[ch].reset();
        }

        envFollower_ = 0.0f;
        envPrediction_ = 0.0f;
        predictionError_ = 0.0f;
        vacuumVCA_ = 1.0f;
        exciterEnv_ = 0.0f;
        exciterActive_ = false;
        tidalLFO_.reset();
        windLFO_.reset();
        satFilter_.reset();
        satFilterR_.reset();
        lastSampleL_ = lastSampleR_ = 0.0f;
    }

    //-- Audio -----------------------------------------------------------------
    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;

        // 1. Parse MIDI
        for (const auto& meta : midi)
        {
            auto msg = meta.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                currentNote_ = msg.getNoteNumber();
                noteFreqRatio_ = std::pow(2.0f, (currentNote_ - 69) / 12.0f); // cache once per note
                currentVelocity_ = msg.getFloatVelocity();
                exciterActive_ = true;
                exciterEnv_ = 1.0f;
                exciterPhase_ = 0.0;
            }
            else if (msg.isNoteOff())
            {
                // Let exciter decay naturally
            }
            else if (msg.isAftertouch() || msg.isChannelPressure())
            {
                aftertouch_ =
                    msg.isAftertouch() ? msg.getAfterTouchValue() / 127.0f : msg.getChannelPressureValue() / 127.0f;
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                modWheel_ = msg.getControllerValue() / 127.0f;
            }
            else if (msg.isPitchWheel())
            {
                pitchBendNorm_ = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
            }
        }

        // Update active voice count for UI display (monophonic: 1 when exciter active, else 0)
        activeVoiceCount_.store(exciterActive_ ? 1 : 0, std::memory_order_relaxed);

        // 2. Silence gate bypass
        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // 3. Read parameters (ParamSnapshot)
        float pAnchor = pAnchorParam_ ? pAnchorParam_->load() : 0.8f;
        float pWindChaos = pWindChaosParam_ ? pWindChaosParam_->load() : 0.5f;
        float pUndertowPull = pUndertowPullParam_ ? pUndertowPullParam_->load() : 0.5f;
        float pSplashSize = pSplashSizeParam_ ? pSplashSizeParam_->load() : 0.5f;
        float pDecay = pDecayParam_ ? pDecayParam_->load() : 4.0f;
        float pDamping = pDampingParam_ ? pDampingParam_->load() : 6000.0f;
        float pDryWet = pDryWetParam_ ? pDryWetParam_->load() : 0.5f;
        float pExcDecay = pExcDecayParam_ ? pExcDecayParam_->load() : 0.02f;
        float pVacuumAttack = pVacuumAttackParam_ ? pVacuumAttackParam_->load() : 0.5f;
        float pOpticalWarmth = pOpticalWarmthParam_ ? pOpticalWarmthParam_->load() : 0.5f;

        // Macros
        float pM1 = pMacroCharacterParam_ ? pMacroCharacterParam_->load() : 0.0f;
        float pM2 = pMacroMovementParam_ ? pMacroMovementParam_->load() : 0.0f;
        float pM3 = pMacroCouplingParam_ ? pMacroCouplingParam_->load() : 0.0f;
        float pM4 = pMacroSpaceParam_ ? pMacroSpaceParam_->load() : 0.0f;

        // M1 CHARACTER → anchor + optical warmth
        pAnchor = clamp(pAnchor + pM1 * 0.2f, 0.0f, 1.0f);
        pOpticalWarmth = clamp(pOpticalWarmth + pM1 * 0.3f, 0.0f, 1.0f);

        // M2 MOVEMENT → wind chaos + tidal drift speed
        pWindChaos = clamp(pWindChaos + pM2 * 0.4f, 0.0f, 1.0f);

        // M3 COUPLING → undertow pull (vacuum aggressiveness)
        pUndertowPull = clamp(pUndertowPull + pM3 * 0.4f, 0.0f, 1.0f);

        // M4 SPACE → splash size + decay + dry/wet
        pSplashSize = clamp(pSplashSize + pM4 * 0.3f, 0.0f, 1.0f);
        pDecay = clamp(pDecay + pM4 * 5.0f, 0.1f, 30.0f);
        pDryWet = clamp(pDryWet + pM4 * 0.2f, 0.0f, 1.0f);

        // D006: aftertouch → undertow aggression
        pUndertowPull = clamp(pUndertowPull + aftertouch_ * 0.3f, 0.0f, 1.0f);
        // Mod wheel → wind chaos (D006)
        pWindChaos = clamp(pWindChaos + modWheel_ * 0.4f, 0.0f, 1.0f);

        // Coupling modulation
        pDamping = clamp(pDamping + extFilterMod_, 200.0f, 16000.0f);

        // EnvToDecay coupling: external envelope signal extends or shortens the reverb tail.
        // couplingEnvDecayMod_ in [-1, 1]: >0 lengthens decay, <0 shortens it.
        // Clamp mod to [-0.45, 1] before multiply so pDecay never goes negative
        // (previously couplingEnvDecayMod_ = -0.6 → multiplier = -0.2 → effectiveDecay < 0
        // before the post-clamp; feedbackCoeff then saw a negative argument to fastExp).
        float safeDecayMod = clamp(couplingEnvDecayMod_, -0.45f, 1.0f);
        float effectiveDecay = pDecay * (1.0f + safeDecayMod * 2.0f);
        effectiveDecay = clamp(effectiveDecay, 0.1f, 30.0f);

        // Feedback coefficient from (possibly coupled) decay
        float feedbackCoeff = (effectiveDecay > 29.0f) ? 1.0f : fastExp(-6.9078f / (effectiveDecay * srF_));

        // Velocity → exciter brightness (D001)
        float excBrightness = 0.3f + currentVelocity_ * 0.7f;
        // Fix D002/stability: decay coeff must use sr_ (double), not srF_ (float), for
        // sample-rate independence. At 96kHz vs 44.1kHz the coeff was 6× faster,
        // making exciter decay time shrink by 2× per doubling of sample rate.
        float excDecayCoeff = fastExp(-6.9078f / (pExcDecay * static_cast<float>(sr_)));

        // Pitch-bent exciter frequency — noteFreqRatio_ is cached in noteOn; bend applied per-block
        float exciterFreq = 440.0f * noteFreqRatio_ *
                            PitchBendUtil::semitonesToFreqRatio(pitchBendNorm_ * 2.0f);

        // Update wind LFO rate based on chaos
        windLFO_.setRate(0.1f + pWindChaos * 3.0f, srF_);

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        // ---- Hoist block-rate smooth coefficients outside per-sample loop ----
        // These depend only on fixed time constants and sample rate — computing
        // them per-sample via fastExp was unnecessary CPU work (#616 fix).
        const float blockAttackCoeff  = smoothCoeffFromTime(0.001f, srF_);
        const float blockReleaseCoeff = smoothCoeffFromTime(0.02f,  srF_);
        // vacuumAttack semantics: 0.0 = fastest vacuum snap (0.01s), 1.0 = slowest (0.10s).
        // Inverted mapping is intentional for this engine: high values = more gradual vacuum fade,
        // low values = aggressive, sudden Pre-Wake snap. This differs from ADSR "Attack" convention
        // (where 0=instant); document here to avoid surprise.
        const float blockVacAttackBase = 0.01f + (1.0f - pVacuumAttack) * 0.09f;
        const float blockVacCoeff     = smoothCoeffFromTime(blockVacAttackBase, srF_);

        // Hoist sub-Hz LFO ticks to block-rate: tidalLFO (~0.001 Hz) and windLFO (0.1–3.1 Hz)
        // change negligibly within a single block. One tick per block saves 4× setCoefficients_fast
        // calls per sample across all FDN channels and eliminates the per-sample fastTan calls.
        // For tidalLFO (~200-second period) the error from one tick/block is < 0.001% of period.
        // windLFO at max 3.1 Hz / 512 samples introduces <0.04 radian error per block — inaudible.
        const float blockTidalMod = tidalLFO_.process();
        const float blockWindMod  = windLFO_.process();

        // Damping frequency is block-constant (driven by tidalMod which just moved above).
        // Pre-compute coefficients once per block for all 4 FDN channels.
        const float blockDampFreq = clamp(pDamping * (1.0f - blockTidalMod * 0.1f), 200.0f, srF_ * 0.49f);
        for (int ch = 0; ch < kFDNChannels; ++ch)
            fdnDamp_[ch].setCoefficients_fast(blockDampFreq, 0.1f, srF_);

        // Saturation filter cutoff is also function of VCA state (vacuumVCA_, gateTension),
        // which changes per-sample. Keep setCoefficients_fast inside the loop for sat filters.
        const float blockRoomScale = 1.0f + pSplashSize * 0.5f + blockTidalMod * 0.05f;
        const float blockWindAmount = pWindChaos * blockWindMod * 0.3f;

        for (int i = 0; i < numSamples; ++i)
        {
            // === EXCITER ===
            float exciterSample = 0.0f;
            if (exciterActive_ && exciterEnv_ > 0.0001f)
            {
                static constexpr float kTwoPi = 6.28318530718f;
                float sine = fastSin(static_cast<float>(exciterPhase_) * kTwoPi);
                exciterPhase_ += exciterFreq / sr_;
                if (exciterPhase_ >= 1.0)
                    exciterPhase_ -= 1.0;

                // Noise burst component (velocity-scaled brightness)
                noiseRng_ ^= noiseRng_ << 13;
                noiseRng_ ^= noiseRng_ >> 17;
                noiseRng_ ^= noiseRng_ << 5;
                float noise = (static_cast<float>(noiseRng_ & 0xFFFF) / 32768.0f - 1.0f) * excBrightness;

                exciterSample = (sine * (1.0f - excBrightness * 0.4f) + noise * 0.5f) * exciterEnv_;
                exciterEnv_ *= excDecayCoeff;
                exciterEnv_ = flushDenormal(exciterEnv_);
                // Clamp to [0,1]: float multiply can produce tiny negatives near zero
                if (exciterEnv_ < 0.0f) exciterEnv_ = 0.0f;
                if (exciterEnv_ < 0.0001f)
                    exciterActive_ = false;
            }

            // === BAYESIAN ENVELOPE PREDICTION ===
            // Simple predictive envelope: one-pole follower + linear predictor
            float absInput = std::fabs(exciterSample);
            float attackCoeff  = blockAttackCoeff;
            float releaseCoeff = blockReleaseCoeff;
            float envCoeff = (absInput > envFollower_) ? attackCoeff : releaseCoeff;
            envFollower_ = envFollower_ + envCoeff * (absInput - envFollower_);
            envFollower_ = flushDenormal(envFollower_);

            // Prediction: simple leaky integrator of envelope derivative
            float envDelta = absInput - envPrediction_;
            envPrediction_ = envPrediction_ + 0.01f * envDelta;
            envPrediction_ = flushDenormal(envPrediction_);

            // Prediction error drives the vacuum
            predictionError_ = std::fabs(envDelta);

            // === PRE-WAKE VACUUM ===
            // Write current sample into lookahead buffer
            lookaheadBufL_[static_cast<size_t>(lookaheadPos_)] = exciterSample;
            lookaheadBufR_[static_cast<size_t>(lookaheadPos_)] = exciterSample;

            // Read delayed sample (20ms behind)
            int readPos = (lookaheadPos_ + 1) % lookaheadSamples_;
            float delayedL = lookaheadBufL_[static_cast<size_t>(readPos)];
            float delayedR = lookaheadBufR_[static_cast<size_t>(readPos)];
            lookaheadPos_ = (lookaheadPos_ + 1) % lookaheadSamples_;

            // Vacuum VCA: prediction error triggers vacuum (drops VCA)
            float vacuumTarget = 1.0f;
            if (predictionError_ > 0.05f)
            {
                // Transient predicted — drop VCA proportional to undertow pull
                vacuumTarget = 1.0f - pUndertowPull * clamp(predictionError_ * 10.0f, 0.0f, 1.0f);
                vacuumTarget = clamp(vacuumTarget, 0.0f, 1.0f);
            }

            // Smooth VCA (fast attack, medium release) — coefficient is block-rate (blockVacCoeff)
            vacuumVCA_ = vacuumVCA_ + blockVacCoeff * (vacuumTarget - vacuumVCA_);
            vacuumVCA_ = flushDenormal(vacuumVCA_);

            // Apply vacuum to delayed signal
            float vacuumL = delayedL * vacuumVCA_;
            float vacuumR = delayedR * vacuumVCA_;

            // === TIDAL DRIFT / WIND (block-rate snapshot used — see hoist above) ===

            // === FDN REVERB (4-channel Householder) ===
            float fdnRead[kFDNChannels];
            for (int ch = 0; ch < kFDNChannels; ++ch)
            {
                int effectiveSize = clamp(static_cast<int>(fdnDelaySize_[ch] * blockRoomScale), 1, fdnDelaySize_[ch]);
                int rp = (fdnWritePos_[ch] - effectiveSize + fdnDelaySize_[ch]) % fdnDelaySize_[ch];
                fdnRead[ch] = fdnDelay_[ch][static_cast<size_t>(rp)];
            }

            // Householder matrix: H = I - (2/N) * 1*1^T
            float fdnSum = 0.0f;
            for (int ch = 0; ch < kFDNChannels; ++ch)
                fdnSum += fdnRead[ch];
            fdnSum *= (2.0f / static_cast<float>(kFDNChannels));

            float fdnOut[kFDNChannels];
            for (int ch = 0; ch < kFDNChannels; ++ch)
                fdnOut[ch] = fdnRead[ch] - fdnSum;

            // Wind chaos: stochastic modulation of FDN channels (block-rate windAmount)
            fdnOut[0] *= (1.0f + blockWindAmount);
            fdnOut[1] *= (1.0f - blockWindAmount);
            fdnOut[2] *= (1.0f + blockWindAmount * 0.7f);
            fdnOut[3] *= (1.0f - blockWindAmount * 0.7f);

            // Damping per channel — coefficients already set at block-rate above
            for (int ch = 0; ch < kFDNChannels; ++ch)
                fdnOut[ch] = fdnDamp_[ch].processSample(fdnOut[ch]);

            // Write back with feedback + input injection
            float inputMono = (vacuumL + vacuumR) * 0.5f;
            for (int ch = 0; ch < kFDNChannels; ++ch)
            {
                float fb = flushDenormal(fdnOut[ch] * feedbackCoeff);
                float inp = inputMono * (1.0f / static_cast<float>(kFDNChannels));
                fdnDelay_[ch][static_cast<size_t>(fdnWritePos_[ch])] = inp + fb;
                fdnWritePos_[ch] = (fdnWritePos_[ch] + 1) % fdnDelaySize_[ch];
            }

            // Sum to stereo: ch 0,1 → L; ch 2,3 → R
            float reverbL = (fdnOut[0] + fdnOut[1]) * 0.5f;
            float reverbR = (fdnOut[2] + fdnOut[3]) * 0.5f;

            // === OPTICAL SATURATION ===
            // The harder the vacuum fights the tail, the warmer it gets
            float gateTension = pUndertowPull * (1.0f - vacuumVCA_);
            float opticalDrive = 1.0f + gateTension * pOpticalWarmth * 5.0f;
            float satL = fastTanh(reverbL * opticalDrive);
            float satR = fastTanh(reverbR * opticalDrive);

            // Optical warmth filter: tension drives LP cutoff down (applied symmetrically to L and R)
            float satCutoff = 8000.0f - gateTension * pOpticalWarmth * 5000.0f;
            satCutoff = clamp(satCutoff, 400.0f, srF_ * 0.49f);
            satFilter_.setCoefficients_fast(satCutoff, 0.5f, srF_);
            satFilterR_.setCoefficients_fast(satCutoff, 0.5f, srF_);
            satL = satFilter_.processSample(satL);
            satR = satFilterR_.processSample(satR);

            // Ring mod coupling
            if (std::fabs(extRingMod_) > 0.001f)
            {
                satL *= (1.0f + extRingMod_);
                satR *= (1.0f + extRingMod_);
            }

            // === ANCHOR BLEND ===
            // Anchor preserves phase-accurate dry signal alongside wet
            float dryL = delayedL * pAnchor;
            float dryR = delayedR * pAnchor;

            float finalL = dryL * (1.0f - pDryWet) + satL * pDryWet;
            float finalR = dryR * (1.0f - pDryWet) + satR * pDryWet;

            // Cache for coupling
            lastSampleL_ = finalL;
            lastSampleR_ = finalR;

            // Accumulate
            outL[i] += finalL;
            if (outR)
                outR[i] += finalR;
        }

        // Reset coupling mods
        extFilterMod_ = 0.0f;
        extRingMod_ = 0.0f;
        couplingEnvDecayMod_ = 0.0f;

        // Silence gate analysis
        const float* rL = buffer.getReadPointer(0);
        const float* rR = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : nullptr;
        silenceGate.analyzeBlock(rL, rR, numSamples);
    }

    //-- Coupling ---------------------------------------------------------------
    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? lastSampleL_ : lastSampleR_;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* sourceBuffer, int numSamples) override
    {
        switch (type)
        {
        case CouplingType::AmpToFilter:
            // Use source amplitude (RMS over block) to drive filter mod, not a bare scalar.
            if (sourceBuffer && numSamples > 0)
            {
                float sum = 0.0f;
                for (int i = 0; i < numSamples; ++i)
                    sum += sourceBuffer[i] * sourceBuffer[i];
                float rms = std::sqrt(sum / static_cast<float>(numSamples));
                extFilterMod_ = rms * amount * 4000.0f;
            }
            else
            {
                extFilterMod_ = amount * 4000.0f;
            }
            break;
        case CouplingType::AudioToRing:
            // Use RMS of block for block-rate ring mod (avoids single-sample aliasing).
            // Per-sample ring mod requires tighter coupling integration; block-RMS is
            // the correct contract for the current applyCouplingInput architecture.
            if (sourceBuffer && numSamples > 0)
            {
                float sum = 0.0f;
                for (int i = 0; i < numSamples; ++i)
                    sum += sourceBuffer[i] * sourceBuffer[i];
                extRingMod_ = std::sqrt(sum / static_cast<float>(numSamples)) * amount;
            }
            else
            {
                extRingMod_ = 0.0f;
            }
            break;
        case CouplingType::EnvToDecay:
            // External envelope extends or shortens the FDN reverb tail.
            // Positive amount lengthens decay; negative tightens it (clamped in renderBlock).
            couplingEnvDecayMod_ = amount;
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

    /// Override so test infra (ParameterSweepTests, DoctrineTests) can build
    /// a per-engine APVTS without going through XOceanusProcessor.
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl(params);
        return juce::AudioProcessorValueTreeState::ParameterLayout(
            std::make_move_iterator(params.begin()),
            std::make_move_iterator(params.end()));
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;

        // Core spatial params
        params.push_back(std::make_unique<PF>(juce::ParameterID{"out_anchor", 1}, "Anchor",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"out_windChaos", 1}, "Wind Chaos",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"out_undertowPull", 1}, "Undertow Pull",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"out_splashSize", 1}, "Splash Size",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Reverb shaping
        params.push_back(std::make_unique<PF>(juce::ParameterID{"out_decay", 1}, "Decay Time",
                                              juce::NormalisableRange<float>(0.1f, 30.0f, 0.0f, 0.3f), 4.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"out_damping", 1}, "Damping",
                                              juce::NormalisableRange<float>(200.0f, 16000.0f, 0.0f, 0.3f), 6000.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"out_dryWet", 1}, "Dry/Wet",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Exciter
        params.push_back(std::make_unique<PF>(juce::ParameterID{"out_exciterDecay", 1}, "Exciter Decay",
                                              juce::NormalisableRange<float>(0.001f, 0.1f, 0.0f, 0.5f), 0.02f));

        // Vacuum / Optical
        params.push_back(std::make_unique<PF>(juce::ParameterID{"out_vacuumAttack", 1}, "Vacuum Attack",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"out_opticalWarmth", 1}, "Optical Warmth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Macros (CHARACTER / MOVEMENT / COUPLING / SPACE)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"out_macroCharacter", 1}, "Character",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"out_macroMovement", 1}, "Movement",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"out_macroCoupling", 1}, "Coupling",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"out_macroSpace", 1}, "Space",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pAnchorParam_ = apvts.getRawParameterValue("out_anchor");
        pWindChaosParam_ = apvts.getRawParameterValue("out_windChaos");
        pUndertowPullParam_ = apvts.getRawParameterValue("out_undertowPull");
        pSplashSizeParam_ = apvts.getRawParameterValue("out_splashSize");
        pDecayParam_ = apvts.getRawParameterValue("out_decay");
        pDampingParam_ = apvts.getRawParameterValue("out_damping");
        pDryWetParam_ = apvts.getRawParameterValue("out_dryWet");
        pExcDecayParam_ = apvts.getRawParameterValue("out_exciterDecay");
        pVacuumAttackParam_ = apvts.getRawParameterValue("out_vacuumAttack");
        pOpticalWarmthParam_ = apvts.getRawParameterValue("out_opticalWarmth");
        pMacroCharacterParam_ = apvts.getRawParameterValue("out_macroCharacter");
        pMacroMovementParam_ = apvts.getRawParameterValue("out_macroMovement");
        pMacroCouplingParam_ = apvts.getRawParameterValue("out_macroCoupling");
        pMacroSpaceParam_ = apvts.getRawParameterValue("out_macroSpace");
    }

private:
    //--------------------------------------------------------------------------
    static constexpr int kFDNChannels = 4;

    double sr_ = 0.0;  // Sentinel: must be set by prepare() before use
    float srF_ = 0.0f;  // Sentinel: must be set by prepare() before use
    int blockSize_ = 512;

    // Lookahead buffer (Pre-Wake Vacuum)
    std::vector<float> lookaheadBufL_;
    std::vector<float> lookaheadBufR_;
    int lookaheadSamples_ = 0;
    int lookaheadPos_ = 0;

    // FDN reverb
    std::vector<float> fdnDelay_[kFDNChannels];
    int fdnDelaySize_[kFDNChannels]{};
    int fdnWritePos_[kFDNChannels]{};
    CytomicSVF fdnDamp_[kFDNChannels];

    // Bayesian predictor state
    float envFollower_ = 0.0f;
    float envPrediction_ = 0.0f;
    float predictionError_ = 0.0f;
    float vacuumVCA_ = 1.0f;

    // LFOs
    StandardLFO tidalLFO_;
    StandardLFO windLFO_;

    // Exciter
    double exciterPhase_ = 0.0;
    float exciterEnv_ = 0.0f;
    bool exciterActive_ = false;
    int currentNote_ = 60;
    float noteFreqRatio_ = 1.0f; // cached: 2^((note-69)/12) — updated in noteOn, read per-block
    float currentVelocity_ = 0.0f;
    uint32_t noiseRng_ = 77u;

    // Saturation filters (separate L/R instances for symmetric stereo optical warmth)
    CytomicSVF satFilter_;
    CytomicSVF satFilterR_;

    // Expression
    float aftertouch_ = 0.0f;
    float modWheel_ = 0.0f;
    float pitchBendNorm_ = 0.0f;

    // Coupling state
    float lastSampleL_ = 0.0f;
    float lastSampleR_ = 0.0f;
    float extFilterMod_ = 0.0f;
    float extRingMod_ = 0.0f;
    float couplingEnvDecayMod_ = 0.0f; // EnvToDecay: ±1 extends/shortens reverb tail (×2 range)

    // Parameter pointers
    std::atomic<float>* pAnchorParam_ = nullptr;
    std::atomic<float>* pWindChaosParam_ = nullptr;
    std::atomic<float>* pUndertowPullParam_ = nullptr;
    std::atomic<float>* pSplashSizeParam_ = nullptr;
    std::atomic<float>* pDecayParam_ = nullptr;
    std::atomic<float>* pDampingParam_ = nullptr;
    std::atomic<float>* pDryWetParam_ = nullptr;
    std::atomic<float>* pExcDecayParam_ = nullptr;
    std::atomic<float>* pVacuumAttackParam_ = nullptr;
    std::atomic<float>* pOpticalWarmthParam_ = nullptr;
    std::atomic<float>* pMacroCharacterParam_ = nullptr;
    std::atomic<float>* pMacroMovementParam_ = nullptr;
    std::atomic<float>* pMacroCouplingParam_ = nullptr;
    std::atomic<float>* pMacroSpaceParam_ = nullptr;

    //--------------------------------------------------------------------------
    static float clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
    static float smoothCoeffFromTime(float timeSec, float sampleRate)
    {
        return 1.0f - fastExp(-1.0f / (timeSec * sampleRate));
    }
};

} // namespace xoceanus
