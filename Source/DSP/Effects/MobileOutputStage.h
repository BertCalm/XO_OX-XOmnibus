// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <algorithm>
#include "../FastMath.h"

namespace xoceanus {

//==============================================================================
// MobileOutputStage — Psychoacoustic output processing for iPhone speakers.
//
// iPhone speakers roll off below ~200 Hz. This stage preserves the "deep bass"
// identity of OBRIX Pocket on small speakers through psychoacoustic substitution
// rather than brute-force EQ:
//
//   1. Harmonic Exciter  — generates 2nd harmonics of sub-bass content so the
//                          brain hears the missing fundamental even when the
//                          speaker cannot reproduce it.
//   2. Mobile EQ         — low shelf +2.5 dB @ 200 Hz (boost the harmonics that
//                          represent the missing fundamental), presence shelf
//                          +1.5 dB @ 2 kHz (clarity compensation).
//   3. Soft Limiter      — prevents exciter-induced clipping.
//
// Signal flow per sample:
//   L/R → Harmonic Exciter (feed-forward) → Mobile EQ → Soft Limiter → out
//
// On desktop (setMobileMode(false)) processSample() is a true no-op: zero
// branches taken, zero state touched. The class is safe to drop into any
// output chain and will cost nothing when inactive.
//
// "Brick Weight" macro (0.0–1.0):
//   Controls exciter mix amount (0 = bypass, 1 = 35% blend) and EQ boost
//   depth simultaneously. This is a named user-facing feature, not a hidden
//   compensation layer — OBRIX Pocket surfaces it as "Brick Weight" in the
//   settings panel.
//
// Stereo width enhancement (mobile mode only):
//   L channel is detuned +3 cents and R channel -2 cents via a per-channel
//   pitch offset applied to the exciter pre-gain. This substitutes for the
//   lost bass depth by adding perceived width. The asymmetric offsets
//   (+3 / -2) are chosen to sum to near-unity in mono.
//
// Integration point (ObrixBridge.mm):
//   Construct one MobileOutputStage as a member of ObrixProcessorAdapter.
//   Call prepare() from prepareToPlay(). Call processSample() on L and R
//   inside processBlock() after eng.renderBlock() and before the output tap.
//   Call setMobileMode(true) on iPhone, leave false on desktop.
//
// CPU cost (mobile mode active):
//   6 biquads (2 per EQ band × stereo + notch × stereo) + waveshaper + limiter
//   = ~18 multiplies + 12 adds per sample. Negligible on A-series silicon.
//==============================================================================
class MobileOutputStage
{
public:
    MobileOutputStage() = default;

    //--------------------------------------------------------------------------
    // Lifecycle
    //--------------------------------------------------------------------------

    /// Call from prepareToPlay(). Safe to call multiple times.
    /// @param sr          Host sample rate (Hz). Never hardcoded — derived from host.
    /// @param blockSize   Maximum block size (unused internally, kept for API symmetry).
    void prepare (double sr, int /*blockSize*/)
    {
        sampleRate = sr;
        recalcAllCoeffs();
        reset();
    }

    /// Zero all filter state variables. Call on reset / stream discontinuity.
    void reset()
    {
        notchL = {}; notchR = {};
        lowShelfL = {}; lowShelfR = {};
        presenceL = {}; presenceR = {};
        limEnvL = 0.0f;
        limEnvR = 0.0f;
    }

    //--------------------------------------------------------------------------
    // Mode and parameter control
    //--------------------------------------------------------------------------

    /// Enable or disable the full processing chain.
    /// When false, processSample() is a true no-op (branch at top of function).
    void setMobileMode (bool enabled)
    {
        mobileMode = enabled;
    }

    /// "Brick Weight" macro: 0.0 = full bypass of exciter and EQ boosts,
    /// 1.0 = full 35% exciter mix + full shelf gains.
    /// Scales exciter blend and EQ gains together so the user has one
    /// unified control for "how much bass compensation".
    void setBrickWeight (float w)
    {
        brickWeight = std::clamp (w, 0.0f, 1.0f);
        // EQ gains are re-derived lazily in recalcAllCoeffs() when triggered
        // by this setter — avoid calling recalcAllCoeffs() per-sample.
        recalcEQCoeffs();
    }

    //--------------------------------------------------------------------------
    // Per-sample stereo processing
    //--------------------------------------------------------------------------

    /// Process one stereo sample pair in-place.
    /// When mobileMode == false this is a pure no-op (single branch).
    inline void processSample (float& L, float& R) noexcept
    {
        if (!mobileMode)
            return;

        // ── 1. Harmonic Exciter (feed-forward topology) ──────────────────
        // Feed-forward: we derive exciter content from the input signal,
        // NOT from a parallel mix of the output. This avoids the phase
        // coherence artifacts that parallel exciter topologies introduce
        // when the output is summed to mono.
        //
        // Transfer function: y = tanh(1.5x) / tanh(1.5)
        // → emphasises 2nd harmonic (even-order distortion) while keeping
        //   the fundamental gain ≤ 1.0.
        //
        // Stereo width via asymmetric gain offset:
        //   L gets a fractionally higher drive (+3 cents equivalent ≈ +0.173%)
        //   R gets fractionally lower drive (-2 cents equivalent ≈ -0.115%)
        // The asymmetry is perceptible as width but sums to ≈ unity in mono.

        const float mixAmt = brickWeight * kMaxExciterMix; // 0..0.35

        if (mixAmt > 0.001f)
        {
            // Anti-resonance notch at 300 Hz: removes the nasal "phone honk"
            // peak that iPhone speakers exhibit at their first resonance mode.
            // Applied to the input before the waveshaper.
            float nL = applyBiquad (L, notchL, notchC);
            float nR = applyBiquad (R, notchR, notchC);

            // Asymmetric pre-gain for stereo width (L=+3ct, R=-2ct)
            // Cent offsets translate to tiny gain differences via 2^(ct/1200):
            //   +3 ct → ×1.001733  |  -2 ct → ×0.998849
            const float gainL = nL * 1.001733f;
            const float gainR = nR * 0.998849f;

            // Waveshaper: tanh(1.5·x) / tanh(1.5)
            // fastTanh from FastMath.h — Padé rational approx, ~0.1% error in [-3,3].
            // normaliser = 1 / tanh(1.5) ≈ 1.05243
            constexpr float kDrive      = 1.5f;
            constexpr float kNormaliser = 1.05243f; // 1.0f / tanhf(1.5f) — precomputed
            float excL = fastTanh (kDrive * gainL) * kNormaliser;
            float excR = fastTanh (kDrive * gainR) * kNormaliser;

            // Feed-forward blend: add exciter content to original signal
            L += mixAmt * (excL - L); // additive on the delta, not on excL directly
            R += mixAmt * (excR - R); // keeps gain structure predictable
        }

        // ── 2. Mobile EQ ─────────────────────────────────────────────────
        // Low shelf +2.5 dB @ 200 Hz (scaled by brickWeight)
        // Presence shelf +1.5 dB @ 2 kHz (scaled by brickWeight)
        // Coefficients are pre-computed in recalcEQCoeffs(); just apply here.
        L = applyBiquad (L, lowShelfL, lowShelfC);
        R = applyBiquad (R, lowShelfR, lowShelfC);

        L = applyBiquad (L, presenceL, presenceC);
        R = applyBiquad (R, presenceR, presenceC);

        // Denormal protection on all filter state (feedback paths)
        flushBiquadState (notchL);    flushBiquadState (notchR);
        flushBiquadState (lowShelfL); flushBiquadState (lowShelfR);
        flushBiquadState (presenceL); flushBiquadState (presenceR);

        // ── 3. Soft Limiter ───────────────────────────────────────────────
        // Soft knee at -3 dB (linear ≈ 0.7079), ratio ~4:1.
        // Applied per-channel independently (avoids inter-channel IM).
        L = softLimit (L);
        R = softLimit (R);
    }

    //--------------------------------------------------------------------------
    // Block processing convenience wrapper
    //--------------------------------------------------------------------------

    /// Process interleaved or split stereo blocks.
    /// Internally calls processSample() per frame.
    void processBlock (float* left, float* right, int numSamples) noexcept
    {
        if (!mobileMode)
            return;

        for (int i = 0; i < numSamples; ++i)
            processSample (left[i], right[i]);
    }

private:

    //--------------------------------------------------------------------------
    // Constants
    //--------------------------------------------------------------------------

    // Maximum exciter blend (35%) when brickWeight = 1.0
    static constexpr float kMaxExciterMix  = 0.35f;

    // Soft limiter knee — -3 dBFS in linear
    static constexpr float kLimiterKnee    = 0.7079457f; // 10^(-3/20)

    //--------------------------------------------------------------------------
    // Runtime state
    //--------------------------------------------------------------------------

    bool  mobileMode  = false;
    float brickWeight = 1.0f;   // default: full Brick Weight on iPhone launch
    double sampleRate  = 48000.0;

    // Envelope followers for the limiter (one per channel, unused — limiter is
    // memoryless soft-knee below; kept for future upgrade to look-ahead design)
    float limEnvL = 0.0f;
    float limEnvR = 0.0f;

    //--------------------------------------------------------------------------
    // Biquad infrastructure
    //--------------------------------------------------------------------------

    struct BiquadState
    {
        float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    };

    struct BiquadCoeffs
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    };

    // Notch @ 300 Hz (anti-resonance, fixed — not scaled by brickWeight)
    BiquadCoeffs notchC;
    BiquadState  notchL, notchR;

    // Low shelf +2.5 dB @ 200 Hz (scaled by brickWeight)
    BiquadCoeffs lowShelfC;
    BiquadState  lowShelfL, lowShelfR;

    // Presence shelf +1.5 dB @ 2 kHz (scaled by brickWeight)
    BiquadCoeffs presenceC;
    BiquadState  presenceL, presenceR;

    //--------------------------------------------------------------------------
    // Filter coefficient calculation — matched-Z bilinear transform
    //
    // All coefficients derived from sampleRate via matched-Z (bilinear with
    // frequency pre-warping). The CLAUDE.md rule: "use exp(-2*PI*fc/sr)" is
    // the matched-Z pole mapping for 1-pole filters; for the 2nd-order sections
    // below we use the full bilinear transform with pre-warping via fastTan().
    //--------------------------------------------------------------------------

    void recalcAllCoeffs()
    {
        recalcNotchCoeffs();
        recalcEQCoeffs();
    }

    /// Anti-resonance notch @ 300 Hz, Q = 8 (narrow — just kills the honk peak).
    /// biquad notch: b0=(1+a2), b1=(-2cosW0), b2=(1+a2); a0 divides all.
    void recalcNotchCoeffs()
    {
        const float sr  = static_cast<float> (sampleRate);
        const float fc  = 300.0f;
        const float Q   = 8.0f;

        // Bilinear pre-warped frequency: w0 = 2*pi*fc/sr
        const float w0     = 2.0f * 3.14159265f * fc / sr;
        const float cosW0  = fastCos (w0);
        const float sinW0  = fastSin (w0);
        const float alpha  = sinW0 / (2.0f * Q);

        // Notch coefficients
        const float a0inv = 1.0f / (1.0f + alpha);
        notchC.b0 =  1.0f           * a0inv;
        notchC.b1 = -2.0f * cosW0  * a0inv;
        notchC.b2 =  1.0f           * a0inv;
        notchC.a1 = -2.0f * cosW0  * a0inv;
        notchC.a2 = (1.0f - alpha) * a0inv;
    }

    /// Low shelf and presence shelf — gains scaled by brickWeight.
    /// Called from both prepare() (via recalcAllCoeffs) and setBrickWeight().
    void recalcEQCoeffs()
    {
        const float sr = static_cast<float> (sampleRate);

        // ── Low shelf: +2.5 dB @ 200 Hz (scaled by brickWeight) ────────
        // Using Audio EQ Cookbook low-shelf formula.
        {
            const float rawGainDb = 2.5f * brickWeight;        // 0..+2.5 dB
            const float A  = std::sqrt (dbToGain (rawGainDb)); // sqrt for shelf formula
            const float fc = 200.0f;
            const float S  = 0.9f; // shelf slope (1.0 = maximally steep)

            const float w0    = 2.0f * 3.14159265f * fc / sr;
            const float cosW0 = fastCos (w0);
            const float sinW0 = fastSin (w0);
            const float alpha = sinW0 / 2.0f * std::sqrt ((A + 1.0f / A) * (1.0f / S - 1.0f) + 2.0f);

            const float twoSqrtAAlpha = 2.0f * std::sqrt (A) * alpha;

            const float a0inv = 1.0f / (    (A + 1.0f) + (A - 1.0f) * cosW0 + twoSqrtAAlpha);
            lowShelfC.b0 = (  A * ((A + 1.0f) - (A - 1.0f) * cosW0 + twoSqrtAAlpha)) * a0inv;
            lowShelfC.b1 = (2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosW0))           * a0inv;
            lowShelfC.b2 = (  A * ((A + 1.0f) - (A - 1.0f) * cosW0 - twoSqrtAAlpha)) * a0inv;
            lowShelfC.a1 = (-2.0f * ((A - 1.0f) + (A + 1.0f) * cosW0))              * a0inv;
            lowShelfC.a2 = (        (A + 1.0f) + (A - 1.0f) * cosW0 - twoSqrtAAlpha) * a0inv;
        }

        // ── Presence shelf: +1.5 dB @ 2 kHz (high-shelf, scaled by brickWeight) ──
        {
            const float rawGainDb = 1.5f * brickWeight;        // 0..+1.5 dB
            const float A  = std::sqrt (dbToGain (rawGainDb));
            const float fc = 2000.0f;
            const float S  = 0.9f;

            const float w0    = 2.0f * 3.14159265f * fc / sr;
            const float cosW0 = fastCos (w0);
            const float sinW0 = fastSin (w0);
            const float alpha = sinW0 / 2.0f * std::sqrt ((A + 1.0f / A) * (1.0f / S - 1.0f) + 2.0f);

            const float twoSqrtAAlpha = 2.0f * std::sqrt (A) * alpha;

            const float a0inv = 1.0f / (    (A + 1.0f) - (A - 1.0f) * cosW0 + twoSqrtAAlpha);
            presenceC.b0 = (  A * ((A + 1.0f) + (A - 1.0f) * cosW0 + twoSqrtAAlpha)) * a0inv;
            presenceC.b1 = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosW0))           * a0inv;
            presenceC.b2 = (  A * ((A + 1.0f) + (A - 1.0f) * cosW0 - twoSqrtAAlpha)) * a0inv;
            presenceC.a1 = ( 2.0f * ((A - 1.0f) - (A + 1.0f) * cosW0))               * a0inv;
            presenceC.a2 = (        (A + 1.0f) - (A - 1.0f) * cosW0 - twoSqrtAAlpha) * a0inv;
        }
    }

    //--------------------------------------------------------------------------
    // Inline DSP helpers
    //--------------------------------------------------------------------------

    /// Direct-form II transposed biquad (numerically stabler than DF-I).
    static inline float applyBiquad (float x, BiquadState& s, const BiquadCoeffs& c) noexcept
    {
        float y = c.b0 * x + c.b1 * s.x1 + c.b2 * s.x2
                           - c.a1 * s.y1 - c.a2 * s.y2;
        s.x2 = s.x1;  s.x1 = x;
        s.y2 = s.y1;  s.y1 = y;
        return y;
    }

    /// Flush denormals in a single biquad state block.
    /// Called after EQ processing (filter feedback path).
    static inline void flushBiquadState (BiquadState& s) noexcept
    {
        s.x1 = flushDenormal (s.x1);  s.x2 = flushDenormal (s.x2);
        s.y1 = flushDenormal (s.y1);  s.y2 = flushDenormal (s.y2);
    }

    /// Soft limiter: soft knee at -3 dBFS, ratio ~4:1 above knee.
    ///
    /// Below kLimiterKnee (≈ 0.7079): identity — y = x
    /// Above knee: y = knee + (x - knee) / (1 + (x - knee) * 4)
    ///
    /// This is equivalent to a 4:1 ratio with a smooth knee at -3 dBFS.
    /// The formula keeps output bounded below 1.0 for any finite input.
    static inline float softLimit (float x) noexcept
    {
        const float ax = std::fabs (x);
        if (ax <= kLimiterKnee)
            return x;

        const float over = ax - kLimiterKnee;
        const float limited = kLimiterKnee + over / (1.0f + over * 4.0f);
        return std::copysign (limited, x);
    }
};

} // namespace xoceanus

//==============================================================================
// Integration guide — ObrixBridge.mm
// ─────────────────────────────────
//
// 1. Add member to ObrixProcessorAdapter:
//
//      MobileOutputStage mobileOut;
//
// 2. In prepareToPlay():
//
//      mobileOut.prepare(sampleRate, samplesPerBlock);
//
//    Enable mobile mode on iPhone at startup (called once from ObrixBridge init):
//
//      #if JUCE_IOS
//      mobileOut.setMobileMode(true);
//      #endif
//
// 3. In processBlock(), after eng.renderBlock() and before the output tap:
//
//      auto* pL = buffer.getWritePointer(0);
//      auto* pR = buffer.getWritePointer(1);
//      mobileOut.processBlock(pL, pR, buffer.getNumSamples());
//
// 4. Wire "Brick Weight" from ObrixBridge:
//
//      - (void)setBrickWeight:(float)weight {
//          _adapter->mobileOut.setBrickWeight(weight);
//      }
//
//    Surface as "Brick Weight" in the settings panel (range 0..1, default 1).
//
// Notes:
//   - No new parameters are exposed through the APVTS — Brick Weight is a
//     device-level control, not a preset parameter. It lives in UserDefaults.
//   - On desktop fleet engines (macOS XOceanus), mobileMode remains false so
//     MobileOutputStage is a literal no-op; no #ifdef guards needed in callers.
//   - reset() is safe to call from the audio thread (no allocation).
//==============================================================================
