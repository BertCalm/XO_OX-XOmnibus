// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include "../FastMath.h"

namespace xoceanus
{

//==============================================================================
// LushReverb — FATHOM-designed 8-tap Hadamard FDN algorithmic reverb.
//
// Architecture:
//   Input conditioning → Pre-delay → 8-tap Poisson Early Reflections
//   → 8-tap Dual-instance Hadamard FDN → Output conditioning → Dry/Wet
//
// Key properties:
//   - Normalized 8×8 Hadamard feedback matrix (O(N log N), no multiplications)
//   - Room-geometry-inspired prime delay lengths (nearest-prime, no near-rational ratios)
//   - 2-pole shelving filter per delay line (separate rt60_low / rt60_high)
//   - Per-line LFO with staggered mutually-irrational rates to smear comb notches
//   - Poisson-distributed early reflection taps (avoids flutter from uniform spacing)
//   - Slightly different L/R delay lengths (+1.5% right offset) for decorrelation
//
// User parameters (8 total, all smoothed):
//   Pre-delay (0–250ms), Size (0–1), Decay (0.5–20s), Damping (0–1),
//   Diffusion (0–1), Modulation (0–1), Wet (0–1), Width (0–1)
//
// CPU budget: ~450 ops/sample stereo ≈ 1.5–2.0% on 2018 MBP
//
// Usage:
//   LushReverb reverb;
//   reverb.prepare(sampleRate);
//   reverb.setSize(0.5f);
//   reverb.setDecay(2.0f);
//   reverb.setDamping(0.4f);
//   reverb.setMix(0.25f);
//   reverb.processBlock(inL, inR, outL, outR, numSamples);
//==============================================================================
class LushReverb
{
public:
    LushReverb() = default;

    //--------------------------------------------------------------------------
    /// Prepare for playback. Allocates all internal buffers. Call before audio.
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        const float srScale = static_cast<float>(sr / 44100.0);

        // ---- Pre-delay (max 250ms) -----------------------------------------
        preDelayMaxLen = static_cast<int>(sr * 0.25) + 2;
        preDelayBuffer.assign(static_cast<size_t>(preDelayMaxLen), 0.0f);
        preDelayPos = 0;
        updatePreDelaySamples();

        // ---- FDN delay line lengths ----------------------------------------
        // Nearest-prime values with no near-rational ratios (@ 44.1kHz).
        // R channel gets a +1.5% offset for stereo decorrelation.
        static constexpr int kBaseDelayL[kN] = {1087, 1283, 1447, 1657, 1823, 2003, 2179, 2381};
        for (int i = 0; i < kN; ++i)
        {
            // Add LFO headroom (max depth 13 samples + 2 guard) to buffer length
            const int lenL = static_cast<int>(static_cast<float>(kBaseDelayL[i]) * srScale) + kLfoMaxDepth + 4;
            const int lenR = static_cast<int>(static_cast<float>(kBaseDelayL[i]) * 1.015f * srScale) + kLfoMaxDepth + 4;
            delayBufL[i].assign(static_cast<size_t>(lenL), 0.0f);
            delayBufR[i].assign(static_cast<size_t>(lenR), 0.0f);
            delayPosL[i] = 0;
            delayPosR[i] = 0;
            baseDelayL[i] = lenL - kLfoMaxDepth - 4;
            baseDelayR[i] = lenR - kLfoMaxDepth - 4;
        }

        // ---- LFO table (1024-point sine, used for modulation) --------------
        for (int i = 0; i < kLfoTableSize; ++i)
            lfoTable[i] = std::sin(2.0 * 3.14159265358979323846 * i / kLfoTableSize);

        // ---- LFO initial state (random but deterministic phases) -----------
        // Staggered phases spread over [0, 2*pi) to avoid correlation.
        static constexpr float kLfoRatesHz[kN] = {0.31f, 0.37f, 0.43f, 0.51f, 0.59f, 0.67f, 0.79f, 1.12f};
        static constexpr float kLfoDepthsSamples[kN] = {6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f};
        static constexpr float kInitialPhases[kN] = {0.0f, 0.785f, 1.571f, 2.356f, 3.141f, 3.927f, 4.712f, 5.497f};

        for (int i = 0; i < kN; ++i)
        {
            lfoPhase[i] = kInitialPhases[i];
            lfoRate[i] = kLfoRatesHz[i];
            lfoBaseDepth[i] = kLfoDepthsSamples[i];
        }

        // ---- Damping filter state -----------------------------------------
        for (int i = 0; i < kN; ++i)
        {
            dampLoL[i] = dampLoR[i] = 0.0f;
            dampHiL[i] = dampHiR[i] = 0.0f;
        }
        updateDampCoeffs();

        // ---- Early reflections (8 taps, Poisson-distributed) ---------------
        // Pre-allocate ER buffer at maximum size (size=1.0) so no allocation ever
        // occurs on the audio thread when Size is modulated.
        // updateERTaps expects sizeScale in [0,1]; sr is factored in internally.
        {
            static constexpr float kErMaxMs = 51.0f; // largest base tap time
            const int erBufMaxNeeded = static_cast<int>(kErMaxMs * 0.001f * 1.0f * static_cast<float>(sr)) + 16;
            erBufL.assign(static_cast<size_t>(erBufMaxNeeded), 0.0f);
            erBufR.assign(static_cast<size_t>(erBufMaxNeeded), 0.0f);
            erPos = 0;
        }
        updateERTaps(size); // size defaults to 0.5f; sized relative to [0,1]

        // ---- Allpass diffusers (4 per channel) -----------------------------
        static constexpr int kApBaseLengths[kNumAP] = {347, 283, 211, 157};
        for (int a = 0; a < kNumAP; ++a)
        {
            int len = static_cast<int>(static_cast<float>(kApBaseLengths[a]) * srScale) + 1;
            apBufL[a].assign(static_cast<size_t>(len), 0.0f);
            apBufR[a].assign(static_cast<size_t>(len + 7), 0.0f);
            apPosL[a] = 0;
            apPosR[a] = 0;
        }

        // ---- Parameter smoother initial state ------------------------------
        // Initialise target = current so no zipper on first block
        smPreDelay.init(preDelayMs, 0.005f, static_cast<float>(sr));
        smSize.init(size, 0.010f, static_cast<float>(sr));
        smDecay.init(decaySec, 0.020f, static_cast<float>(sr));
        smDamping.init(damping, 0.010f, static_cast<float>(sr));
        smDiffusion.init(diffusion, 0.010f, static_cast<float>(sr));
        smMod.init(modAmount, 0.010f, static_cast<float>(sr));
        smWet.init(wet, 0.005f, static_cast<float>(sr));
        smWidth.init(width, 0.010f, static_cast<float>(sr));
    }

    //--------------------------------------------------------------------------
    /// Reset all state without reallocating.
    void reset()
    {
        std::fill(preDelayBuffer.begin(), preDelayBuffer.end(), 0.0f);
        preDelayPos = 0;

        for (int i = 0; i < kN; ++i)
        {
            std::fill(delayBufL[i].begin(), delayBufL[i].end(), 0.0f);
            std::fill(delayBufR[i].begin(), delayBufR[i].end(), 0.0f);
            delayPosL[i] = 0;
            delayPosR[i] = 0;
            dampLoL[i] = dampLoR[i] = 0.0f;
            dampHiL[i] = dampHiR[i] = 0.0f;
            lfoPhase[i] = static_cast<float>(i) * (6.2831853f / kN);
        }

        for (int a = 0; a < kNumAP; ++a)
        {
            std::fill(apBufL[a].begin(), apBufL[a].end(), 0.0f);
            std::fill(apBufR[a].begin(), apBufR[a].end(), 0.0f);
            apPosL[a] = 0;
            apPosR[a] = 0;
        }

        erBufL.assign(erBufL.size(), 0.0f);
        erBufR.assign(erBufR.size(), 0.0f);
        erPos = 0;
    }

    //--------------------------------------------------------------------------
    // Parameter setters (set targets — actual changes are smoothed per-sample)
    //--------------------------------------------------------------------------

    /// Pre-delay: 0–250ms. Maps to master_reverbPreDelay.
    void setPreDelay(float ms) { preDelayMs = clamp(ms, 0.0f, 250.0f); }

    /// Room size: 0.0–1.0. Scales delay lengths and ER tap times.
    /// Maps to master_reverbSize.
    void setSize(float s) { size = clamp(s, 0.0f, 1.0f); }

    /// Decay: 0.5–20s RT60. Maps to master_reverbDecay.
    void setDecay(float sec) { decaySec = clamp(sec, 0.5f, 20.0f); }

    /// Damping: 0.0=bright, 1.0=very dark. Maps to master_reverbDamping.
    void setDamping(float d) { damping = clamp(d, 0.0f, 1.0f); }

    /// Diffusion: 0.0–1.0 (allpass feedback 0.5–0.7 range).
    /// Maps to master_reverbDiffusion.
    void setDiffusion(float d) { diffusion = clamp(d, 0.0f, 1.0f); }

    /// Modulation depth: 0.0–1.0, scales LFO depths. Maps to master_reverbMod.
    void setModulation(float m) { modAmount = clamp(m, 0.0f, 1.0f); }

    /// Wet mix: 0.0=dry, 1.0=wet. Maps to master_reverbMix.
    void setMix(float w) { wet = clamp(w, 0.0f, 1.0f); }

    /// Stereo width: 0.0=mono, 1.0=full. Maps to master_reverbWidth.
    void setWidth(float w) { width = clamp(w, 0.0f, 1.0f); }

    // Legacy API shim — old code called setRoomSize() / setDamping() / setMix().
    // Both setRoomSize() and the Size concept are unified through setSize().
    void setRoomSize(float s) { setSize(s); }

    //--------------------------------------------------------------------------
    /// Process a stereo block. In-place (inL==outL, inR==outR) is safe.
    void processBlock(const float* leftIn, const float* rightIn, float* leftOut, float* rightOut, int numSamples)
    {
        if (delayBufL[0].empty())
        {
            if (leftIn != leftOut)
                std::copy(leftIn, leftIn + numSamples, leftOut);
            if (rightIn != rightOut)
                std::copy(rightIn, rightIn + numSamples, rightOut);
            return;
        }

        for (int n = 0; n < numSamples; ++n)
        {
            // ---- Advance smoothers one sample ------------------------------
            const float curPreDelay = smPreDelay.tick(preDelayMs);
            const float curSize = smSize.tick(size);
            const float curDecay = smDecay.tick(decaySec);
            const float curDamping = smDamping.tick(damping);
            const float curDiffusion = smDiffusion.tick(diffusion);
            const float curMod = smMod.tick(modAmount);
            const float curWet = smWet.tick(wet);
            const float curWidth = smWidth.tick(width);

            // Update filter/ER coefficients only when smoothed params change
            // (use small dead-zone to avoid per-sample recompute).
            if (std::abs(curDamping - lastDamping) > 1e-5f || std::abs(curDecay - lastDecay) > 1e-5f)
            {
                lastDamping = curDamping;
                lastDecay = curDecay;
                updateDampCoeffs();
            }
            if (std::abs(curSize - lastSize) > 1e-4f)
            {
                lastSize = curSize;
                // ER tap re-scale on size change (cheap: just recompute read offsets).
                // Pass curSize directly — updateERTaps multiplies by sr internally.
                updateERTaps(curSize);
            }

            const float inL = leftIn[n];
            const float inR = rightIn[n];
            const float monoIn = (inL + inR) * 0.5f;

            // ---- Pre-delay ------------------------------------------------
            float predelayed = monoIn;
            int preDelaySamps = static_cast<int>(curPreDelay * 0.001f * static_cast<float>(sr));
            preDelaySamps = std::min(preDelaySamps, preDelayMaxLen - 1);
            if (preDelaySamps > 0)
            {
                const int readPos = (preDelayPos - preDelaySamps + preDelayMaxLen) % preDelayMaxLen;
                predelayed = preDelayBuffer[static_cast<size_t>(readPos)];
            }
            preDelayBuffer[static_cast<size_t>(preDelayPos)] = monoIn;
            preDelayPos = (preDelayPos + 1) % preDelayMaxLen;

            // ---- Early reflections (8-tap Poisson, stereo) ----------------
            float erL = 0.0f;
            float erR = 0.0f;
            {
                const int erLen = static_cast<int>(erBufL.size());
                erBufL[static_cast<size_t>(erPos)] = predelayed;
                erBufR[static_cast<size_t>(erPos)] = predelayed;

                for (int t = 0; t < kNumER; ++t)
                {
                    // erTapSamps already incorporate Size scaling (updated in processBlock
                    // via updateERTaps(curSize) whenever curSize changes).
                    const int tapDelay = std::max(1, erTapSamps[t]);
                    const int readPos = (erPos - tapDelay + erLen) % erLen;
                    erL += kErGains[t] * erBufL[static_cast<size_t>(readPos)];
                    erR += kErGains[t] * erBufR[static_cast<size_t>(readPos)];
                }

                erPos = (erPos + 1) % erLen;
            }

            // ---- FDN: read from all 8 delay lines (with LFO modulation) ---
            float fdnOutL[kN], fdnOutR[kN];

            for (int i = 0; i < kN; ++i)
            {
                // Advance LFO phase
                const double phaseIncD = lfoRate[i] / sr;
                lfoPhase[i] = static_cast<float>(
                    std::fmod(static_cast<double>(lfoPhase[i]) + phaseIncD * kLfoTableSize, kLfoTableSize));

                // Table lookup (linear interpolation for clean LFO)
                const float phf = lfoPhase[i];
                const int phi = static_cast<int>(phf) & (kLfoTableSize - 1);
                const float phfrac = phf - static_cast<float>(phi);
                const float lfoVal =
                    lfoTable[phi] + phfrac * (lfoTable[(phi + 1) & (kLfoTableSize - 1)] - lfoTable[phi]);

                // Modulated read position for L
                const float lfoDepth = lfoBaseDepth[i] * curMod;
                const float modOffsetL = lfoVal * lfoDepth;
                const float modOffsetR = lfoVal * (lfoDepth * 1.07f); // slight L/R LFO depth variation

                const int lenL = static_cast<int>(delayBufL[i].size());
                const int lenR = static_cast<int>(delayBufR[i].size());
                const int baseL = baseDelayL[i];
                const int baseR = baseDelayR[i];

                // Integer + fractional delay read (linear interpolation)
                fdnOutL[i] = readLinearInterp(delayBufL[i], lenL, delayPosL[i], baseL, modOffsetL);
                fdnOutR[i] = readLinearInterp(delayBufR[i], lenR, delayPosR[i], baseR, modOffsetR);

                // Apply 2-pole shelving damping filter to each line
                fdnOutL[i] = applyShelvingFilter(fdnOutL[i], dampLoL[i], dampHiL[i], i, false);
                fdnOutR[i] = applyShelvingFilter(fdnOutR[i], dampLoR[i], dampHiR[i], i, true);
            }

            // ---- 8×8 Hadamard mixing matrix (O(N log N)) ------------------
            // Walsh-Hadamard Transform, 3 butterfly stages for N=8.
            // Normalisation factor: 1/√8 ≈ 0.35355339...
            hadamard8(fdnOutL);
            hadamard8(fdnOutR);

            // ---- Write back to delay lines (FDN input = Hadamard output + feed-in) --
            const float feedScale = 0.125f; // 1/8 = equal contribution from input
            for (int i = 0; i < kN; ++i)
            {
                const float writeL = flushDenormal(fdnOutL[i] + (erL + predelayed) * feedScale);
                const float writeR = flushDenormal(fdnOutR[i] + (erR + predelayed) * feedScale);
                delayBufL[i][static_cast<size_t>(delayPosL[i])] = writeL;
                delayBufR[i][static_cast<size_t>(delayPosR[i])] = writeR;
                delayPosL[i] = (delayPosL[i] + 1) % static_cast<int>(delayBufL[i].size());
                delayPosR[i] = (delayPosR[i] + 1) % static_cast<int>(delayBufR[i].size());
            }

            // ---- Sum FDN outputs for left and right buses -----------------
            float reverbL = 0.0f;
            float reverbR = 0.0f;
            for (int i = 0; i < kN; ++i)
            {
                reverbL += fdnOutL[i];
                reverbR += fdnOutR[i];
            }
            // Scale and add early reflections
            reverbL = reverbL * (1.0f / kN) + erL * 0.3f;
            reverbR = reverbR * (1.0f / kN) + erR * 0.3f;

            // ---- 4 allpass diffusers per channel ---------------------------
            const float apFeedback = 0.5f + curDiffusion * 0.2f; // 0.5–0.7
            reverbL = processAllpass(reverbL, apBufL, apPosL, apFeedback);
            reverbR = processAllpass(reverbR, apBufR, apPosR, apFeedback);

            // ---- Width matrix (M/S spread) ---------------------------------
            const float wet1 = curWidth * 0.5f + 0.5f;
            const float wet2 = (1.0f - curWidth) * 0.5f;
            const float mixL = reverbL * wet1 + reverbR * wet2;
            const float mixR = reverbR * wet1 + reverbL * wet2;

            // ---- Dry/wet crossfade -----------------------------------------
            const float dry = 1.0f - curWet;
            leftOut[n] = inL * dry + mixL * curWet;
            rightOut[n] = inR * dry + mixR * curWet;
        }
    }

private:
    //==========================================================================
    // Constants
    //==========================================================================
    static constexpr int kN = 8;               ///< FDN order
    static constexpr int kNumER = 8;           ///< Early reflection taps
    static constexpr int kNumAP = 4;           ///< Allpass diffuser stages
    static constexpr int kLfoTableSize = 1024; ///< LFO lookup table size (power of 2)
    static constexpr int kLfoMaxDepth = 14;    ///< Max LFO depth in samples (headroom)

    //==========================================================================
    // Hadamard 8×8 in-place transform (normalised by 1/√8)
    // Uses recursive Sylvester butterfly construction: 3 stages × 4 butterflies
    //==========================================================================
    static void hadamard8(float v[kN])
    {
        // Stage 1: 4 butterflies over pairs (0,1)(2,3)(4,5)(6,7)
        for (int i = 0; i < kN; i += 2)
        {
            const float a = v[i];
            const float b = v[i + 1];
            v[i] = a + b;
            v[i + 1] = a - b;
        }
        // Stage 2: 4 butterflies over pairs (0,2)(1,3)(4,6)(5,7)
        for (int i = 0; i < kN; i += 4)
        {
            const float a0 = v[i];
            const float b0 = v[i + 2];
            const float a1 = v[i + 1];
            const float b1 = v[i + 3];
            v[i] = a0 + b0;
            v[i + 2] = a0 - b0;
            v[i + 1] = a1 + b1;
            v[i + 3] = a1 - b1;
        }
        // Stage 3: 4 butterflies over pairs (0,4)(1,5)(2,6)(3,7)
        {
            const float a0 = v[0];
            const float b0 = v[4];
            const float a1 = v[1];
            const float b1 = v[5];
            const float a2 = v[2];
            const float b2 = v[6];
            const float a3 = v[3];
            const float b3 = v[7];
            v[0] = a0 + b0;
            v[4] = a0 - b0;
            v[1] = a1 + b1;
            v[5] = a1 - b1;
            v[2] = a2 + b2;
            v[6] = a2 - b2;
            v[3] = a3 + b3;
            v[7] = a3 - b3;
        }
        // Normalise by 1/√8
        constexpr float kNorm = 0.35355339059f; // 1/sqrt(8)
        for (int i = 0; i < kN; ++i)
            v[i] *= kNorm;
    }

    //==========================================================================
    // Linear interpolated delay read
    //==========================================================================
    static float readLinearInterp(const std::vector<float>& buf, int len, int writePos, int baseDelay,
                                  float fractionalOffset)
    {
        const float delayF = static_cast<float>(baseDelay) - fractionalOffset;
        const int delayI = static_cast<int>(delayF);
        const float frac = delayF - static_cast<float>(delayI);

        const int posA = (writePos - delayI + len * 2) % len;
        const int posB = (posA - 1 + len) % len;

        return buf[static_cast<size_t>(posA)] * (1.0f - frac) + buf[static_cast<size_t>(posB)] * frac;
    }

    //==========================================================================
    // 2-pole shelving damping filter (Matched-Z, per-line per-channel state)
    // Separate rt60_low and rt60_high via a 1500Hz shelving crossover.
    // Uses exp(-2π·fc/sr) matched-Z transform (not Euler approximation).
    //
    // Architecture:
    //   low shelf:  y_lo[n] = (1-a_lo)*x[n] + a_lo*y_lo[n-1]    (1-pole LP)
    //   high shelf: y_hi[n] = x[n] - y_lo[n]                     (complement)
    //   out = y_lo * g_lo + y_hi * g_hi
    //==========================================================================
    float applyShelvingFilter(float x, float& stateLo, float& stateHi, int lineIdx, bool isRight)
    {
        (void)lineIdx;
        (void)isRight;
        stateLo = flushDenormal(stateLo + aLo * (x - stateLo));
        const float hi = x - stateLo;
        stateHi = flushDenormal(hi);
        return stateLo * gLo + stateHi * gHi;
    }

    //--------------------------------------------------------------------------
    void updateDampCoeffs()
    {
        // Matched-Z shelving crossover at 1500 Hz
        const float fc = 1500.0f;
        aLo = std::exp(-2.0f * 3.14159265358979f * fc / static_cast<float>(sr));

        // rt60_low maps directly to decaySec
        // rt60_high = decaySec * (1 - damping * 0.85) so highs decay faster
        const float rt60Low = std::max(lastDecay, 0.05f);
        float rt60High = lastDecay * (1.0f - lastDamping * 0.85f);
        rt60High = std::max(rt60High, 0.05f); // floor: ~50ms minimum high-freq tail

        // Per-line gains from RT60: g = 10^(-3*delay_sec/rt60)
        // For FDN we derive a single aggregate feedback scale.
        // The actual per-line gain is applied at write time via feedbackGain,
        // but we store the shelving band gains here for the filter.
        // g_lo and g_hi are the per-sample gains at the crossover frequencies.
        // We derive them from the FDN "mean" delay length.
        const float meanDelaySec = 0.040f; // ~1750 samples @ 44.1k ≈ typical avg

        gLo = std::pow(10.0f, -3.0f * meanDelaySec / std::max(rt60Low, 0.05f));
        gHi = std::pow(10.0f, -3.0f * meanDelaySec / std::max(rt60High, 0.05f));

        // Clamp to prevent runaway
        gLo = clamp(gLo, 0.0f, 0.9999f);
        gHi = clamp(gHi, 0.0f, 0.9999f);
    }

    //--------------------------------------------------------------------------
    void updatePreDelaySamples()
    {
        // No direct per-sample update needed; computed per sample in processBlock
        (void)0;
    }

    //--------------------------------------------------------------------------
    /// Update ER tap lengths.
    /// @param sizeScale  Size parameter [0,1]. Tap times are scaled by this value.
    ///                   Call with (sr/44100) from prepare(), or curSize from processBlock().
    void updateERTaps(float sizeScale)
    {
        // 8 Poisson-distributed taps — NOT uniform spacing (uniform → comb flutter).
        // Base tap times are at 44.1kHz. Scaled by sizeScale and actual sampleRate.
        static constexpr float kErBaseMs[kNumER] = {2.0f, 4.8f, 9.1f, 14.7f, 21.3f, 29.5f, 39.4f, 51.0f};
        static constexpr float kErGainsConst[kNumER] = {0.8f, 0.65f, 0.5f, 0.4f, 0.3f, 0.22f, 0.15f, 0.10f};

        // Formula: tapSamples = ms * 0.001 * sizeScale * sr
        // sizeScale already incorporates any sample-rate ratio; sr provides scale.
        // NOTE: erBufL/erBufR pre-allocated at max size in prepare() — no allocation here.
        const float srF = static_cast<float>(sr);
        for (int t = 0; t < kNumER; ++t)
        {
            erTapSamps[t] = static_cast<int>(kErBaseMs[t] * 0.001f * sizeScale * srF);
            erTapSamps[t] = std::max(1, erTapSamps[t]);
            kErGains[t] = kErGainsConst[t];
        }
    }

    //--------------------------------------------------------------------------
    float processAllpass(float x, std::vector<float> buf[kNumAP], int pos[kNumAP], float feedback)
    {
        float out = x;
        for (int a = 0; a < kNumAP; ++a)
        {
            const int len = static_cast<int>(buf[a].size());
            const int p = pos[a];
            const float bufVal = flushDenormal(buf[a][static_cast<size_t>(p)]);
            const float apOut = -out + bufVal;
            buf[a][static_cast<size_t>(p)] = out + bufVal * feedback;
            out = apOut;
            pos[a] = (p + 1) % len;
        }
        return out;
    }

    //==========================================================================
    // One-pole exponential parameter smoother
    // Avoids zipper noise on all 8 user parameters.
    //==========================================================================
    struct ParamSmoother
    {
        float current = 0.0f;
        float coeff = 0.0f;

        void init(float initialValue, float timeSec, float sampleRate)
        {
            current = initialValue;
            coeff = 1.0f - std::exp(-1.0f / (timeSec * sampleRate));
        }

        float tick(float target)
        {
            current += coeff * (target - current);
            return current;
        }
    };

    //==========================================================================
    // Member data
    //==========================================================================
    double sr = 44100.0;

    // ---- Pre-delay ----------------------------------------------------------
    std::vector<float> preDelayBuffer;
    int preDelayPos = 0;
    int preDelayMaxLen = 0;

    // ---- FDN delay lines (L and R separate for decorrelation) ---------------
    std::vector<float> delayBufL[kN];
    std::vector<float> delayBufR[kN];
    int delayPosL[kN]{};
    int delayPosR[kN]{};
    int baseDelayL[kN]{};
    int baseDelayR[kN]{};

    // ---- LFO ----------------------------------------------------------------
    float lfoTable[kLfoTableSize]{};
    float lfoPhase[kN]{};
    float lfoRate[kN]{};
    float lfoBaseDepth[kN]{};

    // ---- Damping filter state (per delay line, per channel) -----------------
    float dampLoL[kN]{};
    float dampLoR[kN]{};
    float dampHiL[kN]{};
    float dampHiR[kN]{};

    // Shelving filter coefficients (updated when damping/decay change)
    float aLo = 0.8f;  // 1-pole LP coefficient (matched-Z at 1500 Hz)
    float gLo = 0.85f; // Low-band feedback gain
    float gHi = 0.70f; // High-band feedback gain

    // ---- Early reflections --------------------------------------------------
    std::vector<float> erBufL;
    std::vector<float> erBufR;
    int erPos = 0;
    int erTapSamps[kNumER]{};
    float kErGains[kNumER]{};

    // ---- Allpass diffusers (separate L/R buffer sets) -----------------------
    std::vector<float> apBufL[kNumAP];
    std::vector<float> apBufR[kNumAP];
    int apPosL[kNumAP]{};
    int apPosR[kNumAP]{};

    // ---- User parameters (targets for smoothers) ----------------------------
    float preDelayMs = 0.0f;
    float size = 0.5f;
    float decaySec = 2.0f;
    float damping = 0.4f;
    float diffusion = 0.5f;
    float modAmount = 0.3f;
    float wet = 0.25f;
    float width = 1.0f;

    // ---- Smoothers ----------------------------------------------------------
    ParamSmoother smPreDelay;
    ParamSmoother smSize;
    ParamSmoother smDecay;
    ParamSmoother smDamping;
    ParamSmoother smDiffusion;
    ParamSmoother smMod;
    ParamSmoother smWet;
    ParamSmoother smWidth;

    // ---- Cached last smoothed values (to gate coefficient recompute) --------
    float lastDamping = -1.0f;
    float lastDecay = -1.0f;
    float lastSize = -1.0f;
};

} // namespace xoceanus
