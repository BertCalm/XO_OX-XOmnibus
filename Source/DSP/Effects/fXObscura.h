// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include "../FastMath.h"
#include "../CytomicSVF.h"
#include "../StandardLFO.h"

namespace xoceanus
{

//==============================================================================
// fXObscura — "Chiaroscuro" boutique effect.
//
// Inverse-dynamic spectral degradation. Loud signals pass through pristine;
// quiet signals sink into spectral decay — sub-harmonic ring mod, saturation,
// sample-rate reduction, and resonant filtering that intensify as the signal
// fades. The sound doesn't just get quieter — it weathers, corrodes, and
// reveals hidden sub-harmonic shadows as it decays.
//
// Signal flow:
//   input → envelope follower
//         │
//         ├─ LIGHT PATH (loud): clean passthrough
//         │
//         └─ DARK PATH (quiet): sub-harmonic ring mod (f/2 with jitter)
//                              → fastTanh saturation
//                              → dynamic SR reduction
//                              → resonant bandpass (envelope-driven)
//                              → tone LP filter
//                              → DC blocker
//         │
//         └─ dynamic crossfade: darkMix = (1 - env)^erosionCurve
//         │
//         └─ patina breathing LFO modulates erosion
//
// Ghost guidance baked in:
//   Moog    — resonant bandpass in dark path (CytomicSVF BP, envelope-driven)
//   Buchla  — sub-harmonic jitter (stochastic f/3, f/5 undertones)
//   Tomita  — patina breathing LFO on erosion curve
//   Pearlman — HP guard (~80Hz) in dark path to prevent bass mud
//   Vangelis — hold threshold for performance contexts
//
// CPU budget: ~35 ops/sample (~0.18% @ 48kHz)
// Bricks: CytomicSVF ×2 (resonant BP + HP guard), StandardLFO ×1, FastMath
//
// Usage:
//   fXObscura fx;
//   fx.prepare(48000.0);
//   fx.setThreshold(-24.0f);     // dB — below this, weathering begins
//   fx.setHoldLevel(-12.0f);     // dB — above this, always clean (Vangelis)
//   fx.setRelease(100.0f);       // ms — envelope follower release
//   fx.setErosionCurve(2.0f);    // 1=linear, 4=aggressive
//   fx.setSubHarmonic(0.5f);     // ring mod intensity
//   fx.setSaturation(0.4f);      // drive
//   fx.setDecimate(0.3f);        // max SR reduction
//   fx.setResonance(0.5f);       // dark path resonant BP Q (Moog)
//   fx.setTone(3000.0f);         // Hz — weathering tone LP cutoff
//   fx.setPatina(0.03f);         // Hz — breathing LFO rate (Tomita)
//   fx.setMix(0.5f);
//   fx.processBlock(L, R, numSamples);
//==============================================================================
class fXObscura
{
public:
    fXObscura() = default;

    //--------------------------------------------------------------------------
    void prepare(double sampleRate)
    {
        sr = sampleRate;

        // Resonant bandpass in dark path (Moog)
        darkBPL.setMode(CytomicSVF::Mode::BandPass);
        darkBPR.setMode(CytomicSVF::Mode::BandPass);
        darkBPL.reset();
        darkBPR.reset();

        // HP guard in dark path to prevent bass mud (Pearlman)
        darkHPL.setMode(CytomicSVF::Mode::HighPass);
        darkHPR.setMode(CytomicSVF::Mode::HighPass);
        darkHPL.setCoefficients(80.0f, 0.5f, static_cast<float>(sr));
        darkHPR.setCoefficients(80.0f, 0.5f, static_cast<float>(sr));
        darkHPL.reset();
        darkHPR.reset();

        // Patina breathing LFO (Tomita)
        patinaLFO.setShape(StandardLFO::Sine);
        patinaLFO.setRate(patinaRate, static_cast<float>(sr));
        patinaLFO.reset();

        // Reset state
        envState = 0.0f;
        holdL = holdR = 0.0f;
        holdCounter = 0.0f;
        toneStateL = toneStateR = 0.0f;
        dcStateL = dcStateR = 0.0f;
        dcPrevL = dcPrevR = 0.0f;
        lastPositiveL = lastPositiveR = true;
        subOscPhaseL = subOscPhaseR = false;
        jitterRng = 12345u;
        jitterCountL = jitterCountR = 0;
    }

    //--------------------------------------------------------------------------
    /// Threshold in dB (-60 to 0). Below this, weathering begins.
    void setThreshold(float dB) { thresholdDb = clamp(dB, -60.0f, 0.0f); }

    /// Hold level in dB (-60 to 0). Above this, always clean (Vangelis).
    void setHoldLevel(float dB) { holdLevelDb = clamp(dB, -60.0f, 0.0f); }

    /// Envelope follower release in ms (10-500).
    void setRelease(float ms) { releaseMs = clamp(ms, 10.0f, 500.0f); }

    /// Erosion curve exponent (0.5-4.0). 1=linear, higher=more aggressive snap.
    void setErosionCurve(float c) { erosionCurve = clamp(c, 0.5f, 4.0f); }

    /// Sub-harmonic ring mod amount (0-1).
    void setSubHarmonic(float a) { subHarmAmount = clamp(a, 0.0f, 1.0f); }

    /// Dark path saturation drive (0-1).
    void setSaturation(float s) { satDrive = clamp(s, 0.0f, 1.0f); }

    /// Dynamic SR reduction amount (0-1). At 1.0, SR reduces to sr/16.
    void setDecimate(float d) { decimateAmount = clamp(d, 0.0f, 1.0f); }

    /// Dark path resonant bandpass Q (0-1) — Moog.
    void setResonance(float r) { darkResonance = clamp(r, 0.0f, 0.9f); }

    /// Dark path tone LP cutoff in Hz (200-12000).
    void setTone(float hz) { toneFreq = clamp(hz, 200.0f, 12000.0f); }

    /// Patina breathing LFO rate in Hz (0.005-0.15) — Tomita.
    void setPatina(float hz)
    {
        patinaRate = clamp(hz, 0.005f, 0.15f);
        patinaLFO.setRate(patinaRate, static_cast<float>(sr));
    }

    /// Dry/wet mix (0-1).
    void setMix(float m) { mix = clamp(m, 0.0f, 1.0f); }

    //--------------------------------------------------------------------------
    /// Process stereo audio in-place.
    void processBlock(float* L, float* R, int numSamples)
    {
        if (mix < 0.001f)
            return;

        const float srF = static_cast<float>(sr);

        // Convert dB thresholds to linear
        const float threshLin = dbToGain(thresholdDb);
        const float holdLin = dbToGain(holdLevelDb);

        // Envelope follower coefficients
        const float attackCoeff = smoothCoeffFromTime(0.001f, srF); // 1ms attack
        const float releaseCoeff = smoothCoeffFromTime(releaseMs * 0.001f, srF);

        // Tone LP coefficient (matched-Z one-pole)
        const float toneCoeff = fastExp(-6.28318530718f * toneFreq / srF);

        // DC blocker coefficient (~20Hz HP)
        const float dcCoeff = 1.0f - (125.66f / srF); // 2*pi*20/sr

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = L[i];
            float inR = R[i];

            // === Envelope follower (mono, fast attack, adjustable release) ===
            float inputLevel = (std::fabs(inL) + std::fabs(inR)) * 0.5f;
            float envCoeff = (inputLevel > envState) ? attackCoeff : releaseCoeff;
            envState = flushDenormal(envState + envCoeff * (inputLevel - envState));

            // === Patina breathing LFO (Tomita) ===
            float patinaVal = patinaLFO.process();
            float patinaModulation = 1.0f + patinaVal * 0.15f; // ±15% modulation

            // === Dynamic crossfade calculation ===
            // Normalize envelope between hold and threshold
            float envNorm;
            if (envState >= holdLin)
            {
                envNorm = 1.0f; // above hold: always clean
            }
            else if (envState <= threshLin)
            {
                envNorm = 0.0f; // below threshold: full dark
            }
            else
            {
                envNorm = (envState - threshLin) / (holdLin - threshLin + 1e-6f);
            }
            envNorm = clamp(envNorm, 0.0f, 1.0f);

            // Erosion curve with patina breathing
            // Guard against NaN (Architect condition #3): clamp inputs
            float safeBase = clamp(1.0f - envNorm, 0.0f, 1.0f);
            float darkMix = (safeBase > 1e-6f) ? fastExp(erosionCurve * patinaModulation * fastLog(safeBase)) : 0.0f;
            darkMix = clamp(darkMix, 0.0f, 1.0f);

            // === DARK PATH ===
            float darkL = inL;
            float darkR = inR;

            if (darkMix > 0.001f)
            {
                // Stage A: Sub-harmonic ring mod (f/2 square wave with jitter)
                // Buchla jitter: occasionally skip/double zero-crossing count
                bool curPosL = (darkL > 0.0f);
                if (curPosL != lastPositiveL)
                {
                    lastPositiveL = curPosL;
                    jitterCountL++;

                    // Buchla jitter: ~5% chance of skipping (creates f/3, f/5)
                    jitterRng = jitterRng * 1664525u + 1013904223u;
                    bool jitter = (jitterRng & 0xFF) < 13; // ~5% probability

                    if (!jitter)
                        subOscPhaseL = (jitterCountL % 2 == 0);
                    // When jittering, skip the phase toggle — creates non-octave sub
                }
                bool curPosR = (darkR > 0.0f);
                if (curPosR != lastPositiveR)
                {
                    lastPositiveR = curPosR;
                    jitterCountR++;
                    jitterRng = jitterRng * 1664525u + 1013904223u;
                    bool jitter = (jitterRng & 0xFF) < 13;
                    if (!jitter)
                        subOscPhaseR = (jitterCountR % 2 == 0);
                }
                float subL = subOscPhaseL ? 1.0f : -1.0f;
                float subR = subOscPhaseR ? 1.0f : -1.0f;
                darkL = darkL + subHarmAmount * (darkL * subL - darkL);
                darkR = darkR + subHarmAmount * (darkR * subR - darkR);

                // Stage B: Saturation (fastTanh)
                if (satDrive > 0.01f)
                {
                    float drive = 1.0f + satDrive * 8.0f;
                    darkL = fastTanh(darkL * drive);
                    darkR = fastTanh(darkR * drive);
                }

                // Stage C: Dynamic SR reduction (increases as level drops)
                float effectiveSR = srF * (1.0f - decimateAmount * darkMix);
                if (effectiveSR < srF / 16.0f)
                    effectiveSR = srF / 16.0f;
                float holdRatio = srF / effectiveSR;

                holdCounter += 1.0f;
                if (holdCounter >= holdRatio)
                {
                    holdL = darkL;
                    holdR = darkR;
                    holdCounter -= holdRatio;
                }
                darkL = holdL;
                darkR = holdR;

                // Stage D: Resonant bandpass — sweeps down with decay (Moog)
                float bpFreq = 400.0f + envNorm * 4000.0f; // 400Hz (dark) to 4400Hz (bright)
                bpFreq = clamp(bpFreq, 20.0f, srF * 0.49f);
                darkBPL.setCoefficients_fast(bpFreq, darkResonance, srF);
                darkBPR.setCoefficients_fast(bpFreq, darkResonance, srF);
                float bpL = darkBPL.processSample(darkL);
                float bpR = darkBPR.processSample(darkR);
                // Mix BP with direct for parallel character (not fully series)
                darkL = darkL * 0.5f + bpL * 0.7f;
                darkR = darkR * 0.5f + bpR * 0.7f;

                // Stage E: Tone LP filter (one-pole)
                toneStateL = flushDenormal(toneStateL + (1.0f - toneCoeff) * (darkL - toneStateL));
                toneStateR = flushDenormal(toneStateR + (1.0f - toneCoeff) * (darkR - toneStateR));
                darkL = toneStateL;
                darkR = toneStateR;

                // HP guard: remove sub-bass mud from ring mod artifacts (Pearlman)
                darkL = darkHPL.processSample(darkL);
                darkR = darkHPR.processSample(darkR);

                // DC blocker after ring mod (Architect condition #1)
                float dcOutL = darkL - dcPrevL + dcCoeff * dcStateL;
                float dcOutR = darkR - dcPrevR + dcCoeff * dcStateR;
                dcPrevL = darkL;
                dcPrevR = darkR;
                dcStateL = flushDenormal(dcOutL);
                dcStateR = flushDenormal(dcOutR);
                darkL = dcOutL;
                darkR = dcOutR;
            }

            // === Dynamic crossfade: light ←→ dark ===
            float wetL = inL * (1.0f - darkMix) + darkL * darkMix;
            float wetR = inR * (1.0f - darkMix) + darkR * darkMix;

            // === Mix dry/wet ===
            L[i] = inL * (1.0f - mix) + wetL * mix;
            R[i] = inR * (1.0f - mix) + wetR * mix;
        }
    }

    //--------------------------------------------------------------------------
    void reset()
    {
        darkBPL.reset();
        darkBPR.reset();
        darkHPL.reset();
        darkHPR.reset();
        patinaLFO.reset();
        envState = 0.0f;
        holdL = holdR = 0.0f;
        holdCounter = 0.0f;
        toneStateL = toneStateR = 0.0f;
        dcStateL = dcStateR = 0.0f;
        dcPrevL = dcPrevR = 0.0f;
        lastPositiveL = lastPositiveR = true;
        subOscPhaseL = subOscPhaseR = false;
        jitterRng = 12345u;
        jitterCountL = jitterCountR = 0;
    }

    //--------------------------------------------------------------------------
    // addParameters — register all 11 Obscura params into a ParameterLayout
    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
    {
        using AP = juce::AudioParameterFloat;
        using NR = juce::NormalisableRange<float>;
        layout.add(std::make_unique<AP>("master_obsThreshold", "Obscura Threshold",  NR(-60.0f,0.0f,0.1f),-24.0f));
        layout.add(std::make_unique<AP>("master_obsHold",      "Obscura Hold Level", NR(-60.0f,0.0f,0.1f),-12.0f));
        layout.add(std::make_unique<AP>("master_obsRelease",   "Obscura Release",    NR(10.0f,500.0f,0.1f),100.0f));
        layout.add(std::make_unique<AP>("master_obsErosion",   "Obscura Erosion",    NR(0.5f,4.0f,0.001f),2.0f));
        layout.add(std::make_unique<AP>("master_obsSubHarm",   "Obscura Sub Harm",   NR(0.0f,1.0f,0.001f),0.5f));
        layout.add(std::make_unique<AP>("master_obsSaturation","Obscura Saturation", NR(0.0f,1.0f,0.001f),0.4f));
        layout.add(std::make_unique<AP>("master_obsDecimate",  "Obscura Decimate",   NR(0.0f,1.0f,0.001f),0.3f));
        layout.add(std::make_unique<AP>("master_obsResonance", "Obscura Resonance",  NR(0.0f,0.9f,0.001f),0.5f));
        layout.add(std::make_unique<AP>("master_obsTone",      "Obscura Tone",       NR(200.0f,12000.0f,1.0f),3000.0f));
        layout.add(std::make_unique<AP>("master_obsPatina",    "Obscura Patina",     NR(0.005f,0.15f,0.001f),0.03f));
        layout.add(std::make_unique<AP>("master_obsMix",       "Obscura Mix",        NR(0.0f,1.0f,0.001f),0.0f));
    }

    //--------------------------------------------------------------------------
    // cacheParameterPointers — store atomic pointers for processBlockFromSlot()
    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts)
    {
        auto get = [&](const char* id) { return apvts.getRawParameterValue(id); };
        p_threshold  = get("master_obsThreshold");
        p_hold       = get("master_obsHold");
        p_release    = get("master_obsRelease");
        p_erosion    = get("master_obsErosion");
        p_subHarm    = get("master_obsSubHarm");
        p_saturation = get("master_obsSaturation");
        p_decimate   = get("master_obsDecimate");
        p_resonance  = get("master_obsResonance");
        p_tone       = get("master_obsTone");
        p_patina     = get("master_obsPatina");
        p_mix        = get("master_obsMix");
    }

    //--------------------------------------------------------------------------
    // processBlockFromSlot — applies cached params then runs processBlock
    void processBlockFromSlot(float* L, float* R, int numSamples)
    {
        auto rd = [](std::atomic<float>* p, float def) {
            return p ? p->load(std::memory_order_relaxed) : def;
        };
        setThreshold  (rd(p_threshold,  -24.0f));
        setHoldLevel  (rd(p_hold,       -12.0f));
        setRelease    (rd(p_release,    100.0f));
        setErosionCurve(rd(p_erosion,    2.0f));
        setSubHarmonic(rd(p_subHarm,     0.5f));
        setSaturation (rd(p_saturation,  0.4f));
        setDecimate   (rd(p_decimate,    0.3f));
        setResonance  (rd(p_resonance,   0.5f));
        setTone       (rd(p_tone,       3000.0f));
        setPatina     (rd(p_patina,      0.03f));
        setMix        (rd(p_mix,         0.0f));
        processBlock(L, R, numSamples);
    }

private:
    //--------------------------------------------------------------------------
    // Helper: dB to linear gain
    static float dbToGain(float dB) noexcept
    {
        return fastExp(dB * 0.11512925f); // ln(10)/20 ≈ 0.11513
    }

    // Helper: natural log via fast approximation
    static float fastLog(float x) noexcept
    {
        // Simple ln(x) for pow emulation: exp(curve * ln(x))
        // Approximation via Borchardt's method — sufficient for crossfade
        if (x <= 0.0f)
            return -20.0f;
        union
        {
            float f;
            uint32_t i;
        } u = {x};
        return (static_cast<float>(u.i) - 1064866805.0f) * 8.2629582881927490e-8f;
    }

    //--------------------------------------------------------------------------
    double sr = 0.0;  // Sentinel: must be set by prepare() before use

    // Dark path: resonant bandpass (Moog)
    CytomicSVF darkBPL, darkBPR;

    // Dark path: HP guard (Pearlman)
    CytomicSVF darkHPL, darkHPR;

    // Patina breathing LFO (Tomita)
    StandardLFO patinaLFO;

    // Envelope follower
    float envState = 0.0f;

    // SR reduction state
    float holdL = 0.0f, holdR = 0.0f;
    float holdCounter = 0.0f;

    // Tone LP state
    float toneStateL = 0.0f, toneStateR = 0.0f;

    // DC blocker state
    float dcStateL = 0.0f, dcStateR = 0.0f;
    float dcPrevL = 0.0f, dcPrevR = 0.0f;

    // Sub-harmonic ring mod state
    bool lastPositiveL = true, lastPositiveR = true;
    bool subOscPhaseL = false, subOscPhaseR = false;
    uint32_t jitterRng = 12345u; // Buchla jitter RNG
    int jitterCountL = 0, jitterCountR = 0;

    // Parameters
    float thresholdDb = -24.0f;
    float holdLevelDb = -12.0f; // Vangelis hold
    float releaseMs = 100.0f;
    float erosionCurve = 2.0f;
    float subHarmAmount = 0.5f;
    float satDrive = 0.4f;
    float decimateAmount = 0.3f;
    float darkResonance = 0.5f; // Moog resonant BP
    float toneFreq = 3000.0f;
    float patinaRate = 0.03f; // Tomita breathing
    float mix = 0.0f;

    // Cached param pointers (populated by cacheParameterPointers)
    std::atomic<float>* p_threshold  = nullptr;
    std::atomic<float>* p_hold       = nullptr;
    std::atomic<float>* p_release    = nullptr;
    std::atomic<float>* p_erosion    = nullptr;
    std::atomic<float>* p_subHarm    = nullptr;
    std::atomic<float>* p_saturation = nullptr;
    std::atomic<float>* p_decimate   = nullptr;
    std::atomic<float>* p_resonance  = nullptr;
    std::atomic<float>* p_tone       = nullptr;
    std::atomic<float>* p_patina     = nullptr;
    std::atomic<float>* p_mix        = nullptr;
};

} // namespace xoceanus
