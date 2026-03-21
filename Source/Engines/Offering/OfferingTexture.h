#pragma once
//==============================================================================
//
//  OfferingTexture.h — Stochastic Micro-Texture Layer (DUST)
//  XO_OX Designs | XOmnibus Multi-Engine Synthesizer
//
//  Simulates the analog medium chain of crate digging:
//    Vinyl crackle (Poisson impulse noise)
//    Tape hiss + saturation (pink noise + soft clip)
//    Bit crush (12-bit SP-1200 to 16-bit clean)
//    Sample rate reduction (26040 Hz SP-1200 to 48000 Hz clean)
//    Motor drift wobble (pitch/speed instability)
//
//  Controlled by the DUST macro (M4).
//  Accent: Crate Wax Yellow #E5B80B | Prefix: ofr_
//
//==============================================================================

#include "../../DSP/FastMath.h"
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace xomnibus {

//==============================================================================
// OfferingTexture — Micro-texture processor applied per-voice after transient.
//==============================================================================
class OfferingTexture
{
public:
    void prepare (float sampleRate) noexcept
    {
        sr_ = sampleRate;
        noise_.seed (54321);
        pinkState_[0] = pinkState_[1] = pinkState_[2] = pinkState_[3] = 0.0f;
        srHoldSample_ = 0.0f;
        srPhase_ = 0.0f;
        wobblePhase_ = 0.0f;
    }

    //--------------------------------------------------------------------------
    // Process one sample through the texture chain.
    //
    // vinyl: crackle intensity [0, 1]
    // tape: hiss + saturation [0, 1]
    // bits: bit depth [4, 16]
    // targetSR: sample rate reduction target [8000, 48000]
    // wobble: motor drift amount [0, 1]
    //--------------------------------------------------------------------------
    float process (float input, float vinyl, float tape, int bits,
                   float targetSR, float wobble) noexcept
    {
        float out = input;

        // 1. Vinyl crackle — Poisson-distributed impulse noise
        if (vinyl > 0.001f)
            out = processVinyl (out, vinyl);

        // 2. Tape hiss + saturation — pink noise + soft clip
        if (tape > 0.001f)
            out = processTape (out, tape);

        // 3. Bit crush — reduce bit depth
        if (bits < 16)
            out = processBitCrush (out, bits);

        // 4. Sample rate reduction — zero-order hold
        if (targetSR < sr_ - 100.0f)
            out = processSRReduction (out, targetSR);

        // 5. Motor drift wobble — pitch/speed instability
        if (wobble > 0.001f)
            out = processWobble (out, wobble);

        return out;
    }

    void reset() noexcept
    {
        pinkState_[0] = pinkState_[1] = pinkState_[2] = pinkState_[3] = 0.0f;
        srHoldSample_ = 0.0f;
        srPhase_ = 0.0f;
        wobblePhase_ = 0.0f;
    }

private:
    //--------------------------------------------------------------------------
    // Vinyl crackle: Poisson-distributed impulse noise.
    // At low intensity, occasional pops. At high intensity, dense crackle.
    //--------------------------------------------------------------------------
    float processVinyl (float input, float intensity) noexcept
    {
        // Poisson process: probability of crackle per sample
        // At intensity=1, ~200 crackles/sec at 44.1kHz = p ≈ 0.0045
        float p = intensity * intensity * 0.005f;
        float rng = (noise_.process() + 1.0f) * 0.5f; // [0, 1]

        if (rng < p)
        {
            // Crackle: short impulse with random amplitude
            float crackleAmp = noise_.process() * intensity * 0.15f;
            return input + crackleAmp;
        }
        return input;
    }

    //--------------------------------------------------------------------------
    // Tape hiss + saturation: pink noise floor + soft-clip warmth.
    //--------------------------------------------------------------------------
    float processTape (float input, float amount) noexcept
    {
        // Pink noise via Paul Kellet's filtered white noise method
        float white = noise_.process();
        pinkState_[0] = 0.99886f * pinkState_[0] + white * 0.0555179f;
        pinkState_[1] = 0.99332f * pinkState_[1] + white * 0.0750759f;
        pinkState_[2] = 0.96900f * pinkState_[2] + white * 0.1538520f;
        pinkState_[3] = 0.86650f * pinkState_[3] + white * 0.3104856f;
        float pink = (pinkState_[0] + pinkState_[1] + pinkState_[2] + pinkState_[3]) * 0.25f;

        // Flush denormals in pink noise state
        pinkState_[0] = flushDenormal (pinkState_[0]);
        pinkState_[1] = flushDenormal (pinkState_[1]);
        pinkState_[2] = flushDenormal (pinkState_[2]);
        pinkState_[3] = flushDenormal (pinkState_[3]);

        // Add hiss at scaled level
        float withHiss = input + pink * amount * 0.04f;

        // Soft-clip saturation (tape warmth)
        float drive = 1.0f + amount * 2.0f;
        return fastTanh (withHiss * drive) / drive; // normalized to preserve level
    }

    //--------------------------------------------------------------------------
    // Bit crush: quantize to target bit depth.
    // SP-1200 = 12 bits, MPC60 = 12 bits, clean = 16.
    //--------------------------------------------------------------------------
    float processBitCrush (float input, int bits) noexcept
    {
        float levels = std::pow (2.0f, static_cast<float> (bits));
        float quantized = std::round (input * levels) / levels;
        return quantized;
    }

    //--------------------------------------------------------------------------
    // Sample rate reduction: zero-order hold at target rate.
    // SP-1200 = 26040 Hz, MPC3000 = 44100 Hz.
    //--------------------------------------------------------------------------
    float processSRReduction (float input, float targetSR) noexcept
    {
        float ratio = targetSR / sr_;
        srPhase_ += ratio;
        if (srPhase_ >= 1.0f)
        {
            srPhase_ -= 1.0f;
            srHoldSample_ = input;
        }
        return srHoldSample_;
    }

    //--------------------------------------------------------------------------
    // Motor drift wobble: slow random pitch modulation simulating
    // turntable/tape machine motor instability.
    //--------------------------------------------------------------------------
    float processWobble (float input, float amount) noexcept
    {
        // Very slow random wobble (0.3-3 Hz)
        wobblePhase_ += 1.5f / sr_;
        if (wobblePhase_ >= 1.0f) wobblePhase_ -= 1.0f;

        // Sinusoidal drift with noise modulation
        float drift = std::sin (wobblePhase_ * 6.2831853f);
        drift += noise_.process() * 0.3f; // add randomness

        // Apply as amplitude modulation (simulates speed variation)
        // Amount controls depth: max ±5% amplitude variation
        float mod = 1.0f + drift * amount * 0.05f;
        return input * mod;
    }

    //--------------------------------------------------------------------------
    // State
    //--------------------------------------------------------------------------
    float sr_ = 44100.0f;
    OfferingNoiseGen noise_;

    // Pink noise state (Paul Kellet method)
    float pinkState_[4] = {};

    // Sample rate reduction state
    float srHoldSample_ = 0.0f;
    float srPhase_ = 0.0f;

    // Wobble state
    float wobblePhase_ = 0.0f;
};

} // namespace xomnibus
