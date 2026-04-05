// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <algorithm>
#include "../FastMath.h"
#include "../CytomicSVF.h"
#include "../StandardLFO.h"
#include "../../Engines/Oxidize/OxidizeCorrosion.h"

namespace xoceanus
{

//==============================================================================
// fXOxide — "Entropy as Aesthetic" Singularity FX.
//
// Applies OXIDIZE's six-stage degradation pipeline to any engine's stereo output.
// Where the OXIDIZE engine uses note age (per-voice, MIDI-driven), fXOxide uses
// an envelope follower: silence corrodes, loud signal stays fresh. Energy
// preserves; entropy fills the vacuum.
//
// Signal chain per sample:
//   input
//     → envelope follower → currentAge_ derivation (quiet = older)
//     → [Patina Noise Mix]        vinyl crackle / hiss layered in
//     → [Corrosion Waveshaper]    6 modes, drive scaled by age × corrosionDepth
//     → [Erosion Filter]          CytomicSVF LP/BP/Notch, cutoff falls with age
//     → [Entropy Quantizer]       bitcrush + analog noise floor, depth × age
//     → [Wobble]                  wow (slow) + flutter (fast) pitch drift × age
//     → [Dropout Gate]            probability dropout, quadratic with age
//     → dry/wet mix
//     → output
//
// No Sediment Reverb — the host engine already has its own FX chain.
//
// Key design difference from OXIDIZE engine:
//   - No voices, no MIDI, no note age accumulator
//   - Envelope follower replaces note lifetime as the "age" source
//   - Single fxo_magnitude parameter scales all six stages simultaneously
//     (RC-20 MAGNITUDE insight: one knob to rule decay depth)
//
// CPU budget: ~20-25 ops/sample stereo (~0.1% @ 48kHz)
// Bricks: CytomicSVF ×2 (erosion L+R), StandardLFO ×2 (wow + flutter),
//         OxidizeCorrosion (inline static functions, no allocation)
//
// Usage:
//   fXOxide fx;
//   fx.prepare(48000.0, 512);
//   fx.setMagnitude(0.5f);
//   fx.setCorrosionMode(1);    // Transformer
//   fx.setMix(0.8f);
//   fx.processBlock(left, right, numSamples);
//==============================================================================
class fXOxide
{
public:
    fXOxide() = default;

    //--------------------------------------------------------------------------
    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sampleRate_ = sampleRate;
        const float srF = static_cast<float>(sampleRate_);

        erosionFilterL_.setMode(CytomicSVF::Mode::LowPass);
        erosionFilterR_.setMode(CytomicSVF::Mode::LowPass);
        erosionFilterL_.setCoefficients(20000.0f, 0.1f, srF);
        erosionFilterR_.setCoefficients(20000.0f, 0.1f, srF);
        erosionFilterL_.reset();
        erosionFilterR_.reset();

        // Wow: very slow pitch drift (0.3 Hz default)
        wowLFO_.setShape(StandardLFO::Sine);
        wowLFO_.setRate(0.3f, srF);
        wowLFO_.reset();

        // Flutter: fast tape chatter (12 Hz default)
        flutterLFO_.setShape(StandardLFO::Triangle);
        flutterLFO_.setRate(12.0f, srF);
        flutterLFO_.setPhaseOffset(0.25f); // quarter-cycle offset from wow
        flutterLFO_.reset();

        envFollowerL_ = 0.0f;
        envFollowerR_ = 0.0f;
        currentAge_   = 0.0f;
        dropoutEnv_   = 1.0f;
        prngState_    = 0xDEADBEEFu;
        patinaPhaseL_ = 0.0f;
        patinaPhaseR_ = 0.25f; // slight stereo offset
        digitalHoldL_ = 0.0f;
        digitalHoldR_ = 0.0f;
        digitalCounter_ = 0;
        wobblePhaseL_ = 0.0f;
        wobblePhaseR_ = 0.0f;
    }

    //--------------------------------------------------------------------------
    void reset()
    {
        erosionFilterL_.reset();
        erosionFilterR_.reset();
        wowLFO_.reset();
        flutterLFO_.reset();

        envFollowerL_ = 0.0f;
        envFollowerR_ = 0.0f;
        currentAge_   = 0.0f;
        dropoutEnv_   = 1.0f;
        prngState_    = 0xDEADBEEFu;
        patinaPhaseL_ = 0.0f;
        patinaPhaseR_ = 0.25f;
        digitalHoldL_ = 0.0f;
        digitalHoldR_ = 0.0f;
        digitalCounter_ = 0;
        wobblePhaseL_ = 0.0f;
        wobblePhaseR_ = 0.0f;
    }

    //--------------------------------------------------------------------------
    // Parameter setters — called per-block from engine adapters.
    // All setters clamp to valid range internally.

    /// Master intensity (0-1). Scales all degradation stages simultaneously.
    /// This is THE main control — the RC-20 MAGNITUDE concept.
    void setMagnitude(float mag)
    {
        magnitude_ = clamp(mag, 0.0f, 1.0f);
    }

    /// How quickly silence drives aging (0-1). Higher = corrodes faster during quiet.
    void setAgeSpeed(float speed)
    {
        ageSpeed_ = clamp(speed, 0.0f, 1.0f);
    }

    /// Corrosion waveshaper mode (0-5):
    /// Valve / Transformer / BrokenSpeaker / TapeSat / Rust / Acid
    void setCorrosionMode(int mode)
    {
        corrosionMode_ = static_cast<oxidize::CorrosionMode>(
            clamp(mode, 0, 5));
    }

    /// Corrosion waveshaper intensity (0-1), scaled further by magnitude × age.
    void setCorrosionDepth(float depth)
    {
        corrosionDepth_ = clamp(depth, 0.0f, 1.0f);
    }

    /// Minimum erosion filter cutoff at full age (20-5000 Hz).
    void setErosionFloor(float hz)
    {
        erosionFloor_ = clamp(hz, 20.0f, 5000.0f);
    }

    /// Erosion filter mode: 0=Vinyl (LP), 1=Tape (BP notch), 2=Failure (Notch+random res).
    void setErosionMode(int mode)
    {
        erosionMode_ = clamp(mode, 0, 2);
    }

    /// Entropy (bitcrush + noise floor) depth (0-1), age-modulated.
    void setEntropyDepth(float depth)
    {
        entropyDepth_ = clamp(depth, 0.0f, 1.0f);
    }

    /// Entropy smoothing — anti-aliasing / noise texture character (0-1).
    /// Higher = softer, more analog noise character.
    void setEntropySmooth(float smooth)
    {
        entropySmooth_ = clamp(smooth, 0.0f, 1.0f);
    }

    /// Wow (slow pitch drift) depth (0-1), age-modulated.
    void setWowDepth(float depth)
    {
        wowDepth_ = clamp(depth, 0.0f, 1.0f);
    }

    /// Flutter (fast pitch chatter) depth (0-1), age-modulated.
    void setFlutterDepth(float depth)
    {
        flutterDepth_ = clamp(depth, 0.0f, 1.0f);
    }

    /// Dropout probability (0-1), scales quadratically with age for rare-early / frequent-late behavior.
    void setDropoutRate(float rate)
    {
        dropoutRate_ = clamp(rate, 0.0f, 1.0f);
    }

    /// Dropout envelope shape: 0=hard click, 1=soft tape lift (0-1).
    void setDropoutSmear(float smear)
    {
        dropoutSmear_ = clamp(smear, 0.0f, 1.0f);
    }

    /// Patina (vinyl crackle/hiss) noise density (0-1), age-modulated.
    void setPatinaDensity(float density)
    {
        patinaDensity_ = clamp(density, 0.0f, 1.0f);
    }

    /// Dry/wet mix (0-1). 0 = fully dry, 1 = fully wet.
    void setMix(float m)
    {
        mix_ = clamp(m, 0.0f, 1.0f);
    }

    /// Read current age (0-1) for UI metering.
    float getCurrentAge() const noexcept { return currentAge_; }

    //--------------------------------------------------------------------------
    /// Process stereo audio in-place.
    void processBlock(float* left, float* right, int numSamples)
    {
        if (mix_ < 0.001f)
            return;

        const float srF = static_cast<float>(sampleRate_);

        // --- Envelope follower coefficients (one-pole) ---
        // Attack fast (1ms) — loud signal quickly resets age toward freshness
        // Release slow (200ms) — silence gradually accumulates as corrosive energy
        const float attackCoeff  = smoothCoeffFromTime(0.001f, srF);
        const float releaseCoeff = smoothCoeffFromTime(0.20f,  srF);

        // Age smoothing coefficient (10ms lag — prevents zipper noise on abrupt level changes)
        const float ageSmoothCoeff = smoothCoeffFromTime(0.010f, srF);

        // --- Dropout: decide per-block whether a dropout event starts ---
        // Quadratic probability: rare at low age, frequent at high age
        float dropoutProb = currentAge_ * currentAge_ * dropoutRate_ * magnitude_;
        bool triggerDropout = false;
        {
            uint32_t r = lcgNext();
            float rF = static_cast<float>(r) * 2.3283064e-10f; // / 2^32 → [0,1)
            if (rF < dropoutProb * (static_cast<float>(numSamples) / srF))
                triggerDropout = true;
        }

        // Dropout envelope shape: hard clip (smear≈0) vs. soft ramp (smear≈1)
        // Target: 0.0 = full silence, 1.0 = full signal
        const float dropoutTarget = triggerDropout ? 0.0f : 1.0f;
        // Smear maps to envelope time: 0 smear = 0.1ms, 1 smear = 60ms
        const float dropoutTimeMs = 0.1f + dropoutSmear_ * 60.0f;
        const float dropoutCoeff  = smoothCoeffFromTime(dropoutTimeMs * 0.001f, srF);

        // --- Determine erosion filter mode (once per block) ---
        CytomicSVF::Mode svfMode = CytomicSVF::Mode::LowPass;
        if (erosionMode_ == 1)
            svfMode = CytomicSVF::Mode::BandPass;
        else if (erosionMode_ == 2)
            svfMode = CytomicSVF::Mode::Notch;

        erosionFilterL_.setMode(svfMode);
        erosionFilterR_.setMode(svfMode);

        // --- Digital entropy: sample-and-hold rate scales with age ---
        // At age=0: hold rate = 1 (no effect). At age=1 + full entropy: ~4000 Hz effective SR.
        // digitalHoldPeriod is how many samples before refreshing the held value.
        float digitalHoldRateHz = srF - (srF - 4000.0f) * currentAge_ * entropyDepth_ * magnitude_;
        digitalHoldRateHz = std::max(digitalHoldRateHz, 4000.0f);
        int digitalHoldPeriod = std::max(1, static_cast<int>(srF / digitalHoldRateHz));

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = left[i];
            float inR = right[i];

            // ================================================================
            // 1. ENVELOPE FOLLOWER → age derivation
            // ================================================================
            float envIn = std::max(std::abs(inL), std::abs(inR));

            float envCoeffL = (envIn > envFollowerL_) ? attackCoeff : releaseCoeff;
            envFollowerL_ = flushDenormal(envFollowerL_ + envCoeffL * (envIn - envFollowerL_));
            // R tracks same mono signal (env from max(L,R) already), but keep separate
            // follower so stereo dropouts can diverge slightly in future
            float envCoeffR = (envIn > envFollowerR_) ? attackCoeff : releaseCoeff;
            envFollowerR_ = flushDenormal(envFollowerR_ + envCoeffR * (envIn - envFollowerR_));

            float envLevel = std::max(envFollowerL_, envFollowerR_);

            // Target age: louder = fresher. Quiet passages corrode toward ageSpeed × magnitude.
            // envLevel close to 0 → targetAge near ageSpeed * magnitude (full aging drive)
            // envLevel close to 1 → targetAge near 0 (signal is keeping it fresh)
            float freshness = std::min(envLevel * 4.0f, 1.0f); // 0.25 is "full fresh" threshold
            float targetAge = (1.0f - freshness) * ageSpeed_ * magnitude_;

            // Smooth age toward target (prevents abrupt transitions)
            currentAge_ = flushDenormal(currentAge_ + ageSmoothCoeff * (targetAge - currentAge_));
            float age = currentAge_; // local alias for this sample

            // ================================================================
            // 2. PATINA NOISE MIX — vinyl crackle / hiss
            // ================================================================
            float patinaLevel = age * patinaDensity_ * magnitude_;

            float noiseL = 0.0f;
            float noiseR = 0.0f;

            if (patinaLevel > 0.001f)
            {
                // Occasional crackle spike (random sparse events) + continuous hiss
                // Crackle: sparse PRNG-gated impulses
                uint32_t crackleRand = lcgNext();
                float crackleProb = age * patinaDensity_ * 0.02f; // ~2% at full age+density
                float crackleL = 0.0f, crackleR = 0.0f;
                if (static_cast<float>(crackleRand & 0xFFFF) / 65535.0f < crackleProb)
                {
                    crackleL = (static_cast<float>(crackleRand >> 16) / 32767.5f - 1.0f);
                    crackleR = crackleL * (0.8f + 0.4f * (static_cast<float>(lcgNext() & 0xFF) / 255.0f));
                }

                // Hiss: low-level broadband noise (just PRNG scaled down)
                uint32_t hissRand = lcgNext();
                float hissL = (static_cast<float>(hissRand & 0xFFFF)  / 32767.5f - 1.0f) * 0.12f;
                float hissR = (static_cast<float>(hissRand >> 16)      / 32767.5f - 1.0f) * 0.12f;

                noiseL = (crackleL + hissL) * patinaLevel;
                noiseR = (crackleR + hissR) * patinaLevel;
            }

            float sigL = inL + noiseL;
            float sigR = inR + noiseR;

            // ================================================================
            // 3. CORROSION WAVESHAPER
            // ================================================================
            float corrosionDrive = age * corrosionDepth_ * magnitude_;

            if (corrosionDrive > 0.001f)
            {
                // Per-sample PRNG noise for BrokenSpeaker mode
                float bsNoise = static_cast<float>(lcgNext()) * 2.3283064e-10f; // [0,1)

                sigL = oxidize::processCorrosion(sigL, corrosionDrive, corrosionMode_, age, bsNoise);
                sigR = oxidize::processCorrosion(sigR, corrosionDrive, corrosionMode_, age, bsNoise);

                // Tape mode (mode 1): subtract BP component to produce mid-scoop
                if (erosionMode_ == 1)
                {
                    // Handled by SVF mode selection above (BandPass applied post-waveshaper)
                }
            }

            // ================================================================
            // 4. EROSION FILTER
            // ================================================================
            // Cutoff falls from 20kHz (fresh) to erosionFloor_ (fully aged)
            float erosionCutoff = 20000.0f - (20000.0f - erosionFloor_) * age * magnitude_;
            erosionCutoff = std::max(erosionCutoff, erosionFloor_);

            // Resonance: slight peak at eroding cutoff (character per Moog)
            float erosionRes = age * 0.25f * magnitude_;

            // Failure mode: random resonance sweep for unstable collapse character
            if (erosionMode_ == 2 && age > 0.2f)
            {
                float rndRes = static_cast<float>(lcgNext() & 0xFF) / 255.0f;
                erosionRes = erosionRes + rndRes * age * 0.3f * magnitude_;
                erosionRes = std::min(erosionRes, 0.9f);
            }

            erosionFilterL_.setCoefficients(erosionCutoff, erosionRes, srF);
            erosionFilterR_.setCoefficients(erosionCutoff, erosionRes, srF);

            sigL = flushDenormal(erosionFilterL_.processSample(sigL));
            sigR = flushDenormal(erosionFilterR_.processSample(sigR));

            // Tape mode: invert BP output for mid-scoop character
            if (erosionMode_ == 1)
            {
                sigL = inL - sigL; // dry minus bandpass = band-reject character
                sigR = inR - sigR;
            }

            // ================================================================
            // 5. ENTROPY QUANTIZER (digital bitcrush + analog noise floor)
            // ================================================================
            float entropyAmount = age * entropyDepth_ * magnitude_;

            if (entropyAmount > 0.001f)
            {
                // Digital: sample-and-hold at reduced effective sample rate
                ++digitalCounter_;
                if (digitalCounter_ >= digitalHoldPeriod)
                {
                    digitalHoldL_   = sigL;
                    digitalHoldR_   = sigR;
                    digitalCounter_ = 0;
                }
                // Blend toward held value (mix based on entropyAmount)
                sigL = sigL + entropyAmount * (digitalHoldL_ - sigL);
                sigR = sigR + entropyAmount * (digitalHoldR_ - sigR);

                // Analog: noise floor rise — smoothed by entropySmooth_
                // Higher smooth = less aggressive noise injection, more haze character
                float noiseFloor = entropyAmount * 0.12f * (1.0f - entropySmooth_ * 0.7f);
                if (noiseFloor > 0.001f)
                {
                    float aN = (static_cast<float>(lcgNext() & 0xFFFF) / 32767.5f - 1.0f) * noiseFloor;
                    float bN = (static_cast<float>(lcgNext() & 0xFFFF) / 32767.5f - 1.0f) * noiseFloor;
                    sigL = flushDenormal(sigL + aN);
                    sigR = flushDenormal(sigR + bN);
                }
            }

            // ================================================================
            // 6. WOBBLE — wow (slow) + flutter (fast) pitch drift
            // ================================================================
            float ageWow     = age * wowDepth_     * magnitude_;
            float ageFlutter = age * flutterDepth_ * magnitude_;

            if (ageWow > 0.001f || ageFlutter > 0.001f)
            {
                float wowVal     = wowLFO_.process();
                float flutterVal = flutterLFO_.process();

                // Pitch deviation in fractional samples — applied as fractional delay
                // (1 sample max deviation at full depth — tape head wobble range)
                float wobbleSamplesL = wowVal * ageWow * 0.8f + flutterVal * ageFlutter * 0.3f;

                // Stereo spread: right channel is phase-shifted by wobble in opposite direction
                // This creates a subtle stereo width wobble character
                float wobbleSamplesR = -wowVal * ageWow * 0.6f + flutterVal * ageFlutter * 0.3f;

                // Apply as simple phase modulation on signal (no delay line needed for <1 sample)
                // Use fractional delay approximation: linear blend between current and prev sample
                // We accumulate a "virtual phase" and use it to scale the amplitude slightly
                // This is a first-order approximation but captures the pitch-modulation character
                wobblePhaseL_ = flushDenormal(wobblePhaseL_ + wobbleSamplesL);
                wobblePhaseR_ = flushDenormal(wobblePhaseR_ + wobbleSamplesR);

                // PM-style: sin of the accumulated phase modulates the signal
                // Small depth: sounds like a wavering tape
                float pmL = fastSin(wobblePhaseL_ * 0.1f); // gentle radians
                float pmR = fastSin(wobblePhaseR_ * 0.1f);

                sigL = sigL * (1.0f + pmL * ageWow * 0.4f);
                sigR = sigR * (1.0f + pmR * ageWow * 0.4f);

                sigL = flushDenormal(sigL);
                sigR = flushDenormal(sigR);
            }
            else
            {
                // Still advance LFOs to keep phase state consistent
                wowLFO_.process();
                flutterLFO_.process();
            }

            // ================================================================
            // 7. DROPOUT GATE
            // ================================================================
            // Smooth the dropout envelope toward target (smear controls speed)
            dropoutEnv_ = flushDenormal(dropoutEnv_ + dropoutCoeff * (dropoutTarget - dropoutEnv_));

            sigL *= dropoutEnv_;
            sigR *= dropoutEnv_;

            // ================================================================
            // 8. DRY / WET MIX
            // ================================================================
            left[i]  = inL * (1.0f - mix_) + sigL * mix_;
            right[i] = inR * (1.0f - mix_) + sigR * mix_;
        }
    }

private:
    //--------------------------------------------------------------------------
    // Helper: one-pole smoothing coefficient from time constant in seconds.
    // Returns c such that state += c * (target - state) converges in ~5τ.
    static float smoothCoeffFromTime(float timeSeconds, float sampleRate) noexcept
    {
        if (timeSeconds <= 0.0f || sampleRate <= 0.0f)
            return 1.0f;
        // c = 1 - exp(-1 / (τ * sr)), approximated as min(1, dt/τ) for safety
        float tau = timeSeconds * sampleRate;
        return std::min(1.0f, 1.0f / tau);
    }

    // Knuth LCG — deterministic per-sample noise (same LCG as OXIDIZE engine)
    inline uint32_t lcgNext() noexcept
    {
        prngState_ = prngState_ * 1664525u + 1013904223u;
        return prngState_;
    }

    static float clamp(float v, float lo, float hi) noexcept
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    static int clamp(int v, int lo, int hi) noexcept
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    //--------------------------------------------------------------------------
    // DSP modules
    CytomicSVF erosionFilterL_;
    CytomicSVF erosionFilterR_;
    StandardLFO wowLFO_;
    StandardLFO flutterLFO_;

    // Envelope follower state
    float envFollowerL_ = 0.0f;
    float envFollowerR_ = 0.0f;

    // Derived age (0=pristine/loud, 1=fully corroded/silent)
    float currentAge_ = 0.0f;

    // Dropout gate
    float dropoutEnv_ = 1.0f;

    // PRNG
    uint32_t prngState_ = 0xDEADBEEFu;

    // Patina phase (not currently used for phase-based noise, reserved for
    // future tonal crackle oscillator if patinaTone param is added)
    float patinaPhaseL_ = 0.0f;
    float patinaPhaseR_ = 0.25f;

    // Wobble accumulated phase
    float wobblePhaseL_ = 0.0f;
    float wobblePhaseR_ = 0.0f;

    // Digital entropy state
    float digitalHoldL_   = 0.0f;
    float digitalHoldR_   = 0.0f;
    int   digitalCounter_ = 0;

    double sampleRate_ = 44100.0;

    //--------------------------------------------------------------------------
    // Parameters (with sensible defaults matching OXIDIZE spec)
    float magnitude_      = 0.0f;   // master intensity — default 0 (bypass until set)
    float ageSpeed_       = 0.3f;   // how fast silence causes aging
    oxidize::CorrosionMode corrosionMode_ = oxidize::CorrosionMode::Valve;
    float corrosionDepth_ = 0.35f;  // waveshaper intensity
    float erosionFloor_   = 200.0f; // Hz — minimum cutoff at full age
    int   erosionMode_    = 0;      // 0=Vinyl, 1=Tape, 2=Failure
    float entropyDepth_   = 0.25f;  // bitcrush + noise floor depth
    float entropySmooth_  = 0.6f;   // analog/digital noise character
    float wowDepth_       = 0.20f;  // slow pitch drift
    float flutterDepth_   = 0.15f;  // fast tape chatter
    float dropoutRate_    = 0.10f;  // per-block dropout probability at full age
    float dropoutSmear_   = 0.50f;  // envelope shape (0=click, 1=soft tape lift)
    float patinaDensity_  = 0.15f;  // crackle / hiss level
    float mix_            = 0.0f;   // dry/wet (default 0 = bypassed)
};

} // namespace xoceanus
