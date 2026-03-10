#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>
#include <cstring>

namespace xomnibus {

//==============================================================================
// OnsetNoiseGen — xorshift32 PRNG for percussion noise.
//==============================================================================
class OnsetNoiseGen
{
public:
    void seed (uint32_t s) noexcept { state = s ? s : 1; }
    float process() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float> (static_cast<int32_t> (state)) / 2147483648.0f;
    }
private:
    uint32_t state = 1;
};

//==============================================================================
// OnsetEnvelope — AD/AHD/ADSR percussion envelope with coefficient caching.
//==============================================================================
class OnsetEnvelope
{
public:
    enum class Stage { Idle, Attack, Hold, Decay, Sustain, Release };
    enum class Shape { AD = 0, AHD = 1, ADSR = 2 };

    void prepare (double sampleRate) noexcept { sr = static_cast<float> (sampleRate); }

    void trigger (float attackSec, float decaySec, Shape shape = Shape::AD,
                  float holdSec = 0.0f, float sustainLvl = 0.0f) noexcept
    {
        currentShape = shape;
        sustainLevel = sustainLvl;
        stage = Stage::Attack;
        level = 0.0f;
        float aSec = std::max (attackSec, 0.0001f);
        attackRate = 1.0f / (sr * aSec);
        holdSamplesLeft = (shape != Shape::AD) ? std::max (0, static_cast<int> (sr * holdSec)) : 0;
        float dSec = std::max (decaySec, 0.001f);
        if (dSec != lastDecay)
        {
            lastDecay = dSec;
            decayCoeff = 1.0f - std::exp (-4.6f / (sr * dSec));
        }
    }

    float process() noexcept
    {
        switch (stage)
        {
            case Stage::Idle: return 0.0f;
            case Stage::Attack:
                level += attackRate;
                if (level >= 1.0f)
                {
                    level = 1.0f;
                    stage = (holdSamplesLeft > 0) ? Stage::Hold : Stage::Decay;
                }
                return level;
            case Stage::Hold:
                if (--holdSamplesLeft <= 0)
                    stage = Stage::Decay;
                return level;
            case Stage::Decay:
                if (currentShape == Shape::ADSR && sustainLevel > 0.0f)
                {
                    level -= (level - sustainLevel) * decayCoeff;
                    level = flushDenormal (level);
                    if (level <= sustainLevel + 0.001f) { level = sustainLevel; stage = Stage::Sustain; }
                }
                else
                {
                    level -= level * decayCoeff;
                    level = flushDenormal (level);
                    if (level < 1e-6f) { level = 0.0f; stage = Stage::Idle; }
                }
                return level;
            case Stage::Sustain:
                return level;
            case Stage::Release:
                level -= level * decayCoeff;
                level = flushDenormal (level);
                if (level < 1e-6f) { level = 0.0f; stage = Stage::Idle; }
                return level;
        }
        return 0.0f;
    }

    void release() noexcept
    {
        if (stage == Stage::Sustain || stage == Stage::Hold || stage == Stage::Decay)
            stage = Stage::Release;
    }

    bool isActive() const noexcept { return stage != Stage::Idle; }
    float getLevel() const noexcept { return level; }
    void reset() noexcept { stage = Stage::Idle; level = 0.0f; lastDecay = -1.0f; holdSamplesLeft = 0; }

private:
    float sr = 44100.0f;
    Stage stage = Stage::Idle;
    Shape currentShape = Shape::AD;
    float level = 0.0f;
    float attackRate = 0.01f;
    float decayCoeff = 0.001f;
    float lastDecay = -1.0f;
    float sustainLevel = 0.0f;
    int holdSamplesLeft = 0;
};

//==============================================================================
// OnsetTransient — Pre-blend click/snap: pitch spike + noise burst.
//==============================================================================
class OnsetTransient
{
public:
    void prepare (double sampleRate) noexcept { sr = static_cast<float> (sampleRate); }

    void trigger (float snapAmount, float baseFreqHz) noexcept
    {
        if (snapAmount < 0.01f) { active = false; return; }
        active = true;
        snapLevel = snapAmount;
        phase = 0.0f;
        spikeFreq = baseFreqHz * (4.0f + snapAmount * 12.0f);
        spikeSamples = static_cast<int> (sr * (0.001f + (1.0f - snapAmount) * 0.005f));
        noiseSamples = static_cast<int> (sr * (0.001f + snapAmount * 0.002f));
    }

    float process() noexcept
    {
        if (!active) return 0.0f;
        float out = 0.0f;
        if (spikeSamples > 0)
        {
            phase += spikeFreq / sr;
            while (phase >= 1.0f) phase -= 1.0f;
            out += fastSin (phase * 6.28318530718f) * snapLevel * 0.4f;
            --spikeSamples;
        }
        if (noiseSamples > 0)
        {
            out += noise.process() * snapLevel * 0.25f;
            --noiseSamples;
        }
        if (spikeSamples <= 0 && noiseSamples <= 0) active = false;
        return out;
    }

    void reset() noexcept { active = false; spikeSamples = 0; noiseSamples = 0; }

private:
    float sr = 44100.0f;
    bool active = false;
    float snapLevel = 0.0f;
    float phase = 0.0f;
    float spikeFreq = 1000.0f;
    int spikeSamples = 0;
    int noiseSamples = 0;
    OnsetNoiseGen noise;
};

//==============================================================================
// LAYER X: CIRCUIT MODELS
//==============================================================================

//==============================================================================
// BridgedTOsc — TR-808 kick/tom: decaying sine + pitch envelope + sub.
//==============================================================================
class BridgedTOsc
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; reset(); }

    void trigger (float freqHz, float snapAmt, float bodyAmt, float charAmt) noexcept
    {
        baseFreq = static_cast<double> (freqHz);
        subLevel = bodyAmt;
        satAmt = charAmt;
        pitchEnvSt = snapAmt * 48.0f;
        double decayMs = 5.0 + (1.0 - static_cast<double> (snapAmt)) * 45.0;
        pitchDecayCoeff = 1.0f - fastExp (static_cast<float> (-1.0 / (sr * decayMs * 0.001))); // QA W2
        phase = 0.0;
        subPhase = 0.0;
        active = true;
    }

    float process() noexcept
    {
        if (!active) return 0.0f;
        pitchEnvSt *= (1.0f - pitchDecayCoeff);
        pitchEnvSt = flushDenormal (pitchEnvSt);
        float freqMul = (pitchEnvSt > 0.01f) ? fastExp (pitchEnvSt * (0.693147f / 12.0f)) : 1.0f;
        double freq = baseFreq * static_cast<double> (freqMul);
        double inc = freq / sr;
        phase += inc;
        while (phase >= 1.0) phase -= 1.0;
        float out = fastSin (static_cast<float> (phase) * 6.28318530718f);
        subPhase += inc * 0.5;
        while (subPhase >= 1.0) subPhase -= 1.0;
        float tri = 2.0f * std::abs (2.0f * static_cast<float> (subPhase) - 1.0f) - 1.0f;
        out += tri * subLevel;
        if (satAmt > 0.01f) out = fastTanh (out * (1.0f + satAmt * 4.0f));
        return out;
    }

    void reset() noexcept { phase = 0.0; subPhase = 0.0; pitchEnvSt = 0.0f; active = false; }
    bool isActive() const noexcept { return active; }

private:
    double sr = 44100.0, baseFreq = 55.0, phase = 0.0, subPhase = 0.0;
    float pitchEnvSt = 0.0f, pitchDecayCoeff = 0.01f, subLevel = 0.5f, satAmt = 0.0f;
    bool active = false;
};

//==============================================================================
// NoiseBurstCircuit — Snare/clap: dual sine body + filtered noise.
//==============================================================================
class NoiseBurstCircuit
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        noiseHPF.setMode (CytomicSVF::Mode::HighPass);
        reset();
    }

    void trigger (float freqHz, float snapAmt, float toneAmt, float charAmt, bool isClap) noexcept
    {
        osc1Freq = 180.0 + static_cast<double> (freqHz - 180.0f) * 0.3;
        osc2Freq = 330.0 + static_cast<double> (freqHz - 180.0f) * 0.7;
        toneBal = toneAmt;
        noiseEnvLevel = 1.0f;
        float noiseDur = 0.01f + snapAmt * 0.04f;
        noiseDecayRate = 1.0f - fastExp (static_cast<float> (-1.0 / (sr * noiseDur * 0.368))); // QA W2
        bodyPitchSt = snapAmt * 12.0f;
        float pitchMs = 5.0f + (1.0f - snapAmt) * 20.0f;
        bodyPitchCoeff = 1.0f - fastExp (static_cast<float> (-1.0 / (sr * pitchMs * 0.001))); // QA W2
        float hpCutoff = 2000.0f + charAmt * 6000.0f;
        noiseHPF.setCoefficients (hpCutoff, 0.0f, static_cast<float> (sr));
        phase1 = 0.0; phase2 = 0.0;
        active = true;
        clapMode = isClap;
        clapBursts = isClap ? 3 : 0;
        clapCounter = 0;
        clapInterval = static_cast<int> (sr * 0.01);
    }

    float process() noexcept
    {
        if (!active) return 0.0f;
        bodyPitchSt *= (1.0f - bodyPitchCoeff);
        bodyPitchSt = flushDenormal (bodyPitchSt);
        float pMul = (bodyPitchSt > 0.01f) ? fastExp (bodyPitchSt * (0.693147f / 12.0f)) : 1.0f;
        phase1 += (osc1Freq * static_cast<double> (pMul)) / sr;
        while (phase1 >= 1.0) phase1 -= 1.0;
        phase2 += (osc2Freq * static_cast<double> (pMul)) / sr;
        while (phase2 >= 1.0) phase2 -= 1.0;
        float body = (fastSin (static_cast<float> (phase1) * 6.28318530718f)
                     + fastSin (static_cast<float> (phase2) * 6.28318530718f)) * 0.5f;
        noiseEnvLevel -= noiseEnvLevel * noiseDecayRate;
        noiseEnvLevel = flushDenormal (noiseEnvLevel);
        float nOut = noiseHPF.processSample (noise.process()) * noiseEnvLevel;
        if (clapMode && clapBursts > 0)
        {
            if (++clapCounter >= clapInterval)
            { clapCounter = 0; --clapBursts; noiseEnvLevel = 0.8f; }
        }
        return body * (1.0f - toneBal) + nOut * toneBal;
    }

    void reset() noexcept
    {
        phase1 = 0.0; phase2 = 0.0; noiseEnvLevel = 0.0f;
        bodyPitchSt = 0.0f; active = false; clapBursts = 0;
        noiseHPF.reset();
    }
    bool isActive() const noexcept { return active; }

private:
    double sr = 44100.0, osc1Freq = 180.0, osc2Freq = 330.0, phase1 = 0.0, phase2 = 0.0;
    float toneBal = 0.5f, noiseEnvLevel = 0.0f, noiseDecayRate = 0.01f;
    float bodyPitchSt = 0.0f, bodyPitchCoeff = 0.01f;
    bool active = false, clapMode = false;
    int clapBursts = 0, clapCounter = 0, clapInterval = 441;
    OnsetNoiseGen noise;
    CytomicSVF noiseHPF;
};

//==============================================================================
// MetallicOsc — 808-style 6-oscillator metallic hat/cymbal.
//==============================================================================
class MetallicOsc
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        bp1.setMode (CytomicSVF::Mode::BandPass);
        bp2.setMode (CytomicSVF::Mode::BandPass);
        hpf.setMode (CytomicSVF::Mode::HighPass);
        reset();
    }

    void trigger (float pitchRatio, float toneAmt, float charAmt) noexcept
    {
        static constexpr float baseF[6] = { 205.3f, 304.4f, 369.6f, 522.7f, 800.0f, 1048.0f };
        for (int i = 0; i < 6; ++i) { freqs[i] = baseF[i] * pitchRatio; phases[i] = 0.0; }
        pw = 0.5f - charAmt * 0.3f;
        toneBal = toneAmt;
        float fsr = static_cast<float> (sr);
        bp1.setCoefficients (3440.0f * pitchRatio, 0.7f, fsr);
        bp2.setCoefficients (std::min (7100.0f * pitchRatio, fsr * 0.45f), 0.7f, fsr);
        hpf.setCoefficients (std::min (6000.0f * pitchRatio, fsr * 0.45f), 0.0f, fsr);
        active = true;
    }

    float process() noexcept
    {
        if (!active) return 0.0f;
        float sum = 0.0f;
        for (int i = 0; i < 6; ++i)
        {
            phases[i] += static_cast<double> (freqs[i]) / sr;
            while (phases[i] >= 1.0) phases[i] -= 1.0;
            sum += (phases[i] < static_cast<double> (pw)) ? 1.0f : -1.0f;
        }
        sum *= (1.0f / 6.0f);
        float lo = bp1.processSample (sum);
        float hi = bp2.processSample (sum);
        float mixed = lo * (1.0f - toneBal) + hi * toneBal;
        return hpf.processSample (mixed);
    }

    void reset() noexcept
    {
        for (auto& p : phases) p = 0.0;
        active = false; bp1.reset(); bp2.reset(); hpf.reset();
    }
    bool isActive() const noexcept { return active; }

private:
    double sr = 44100.0;
    float freqs[6] = {}, toneBal = 0.5f, pw = 0.5f;
    double phases[6] = {};
    bool active = false;
    CytomicSVF bp1, bp2, hpf;
};

//==============================================================================
// LAYER O: ALGORITHMIC SYNTHESIS
//==============================================================================

//==============================================================================
// FMPercussion — 2-operator FM with self-feedback.
//==============================================================================
class FMPercussion
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; reset(); }

    void trigger (float freqHz, float snapAmt, float charAmt, float toneAmt) noexcept
    {
        carrierFreq = static_cast<double> (freqHz);
        ratio = 1.4;
        modIndex = charAmt * 8.0f;
        modEnvLevel = 1.0f;
        float modDecay = 0.005f + (1.0f - snapAmt) * 0.1f;
        modEnvDecayRate = static_cast<float> (1.0 - std::exp (-1.0 / (sr * static_cast<double> (modDecay) * 0.368)));
        feedback = toneAmt * 0.3f;
        carrierPhase = 0.0; modPhase = 0.0; lastOut = 0.0f;
        active = true;
    }

    float process() noexcept
    {
        if (!active) return 0.0f;
        modEnvLevel -= modEnvLevel * modEnvDecayRate;
        modEnvLevel = flushDenormal (modEnvLevel);
        modPhase += (carrierFreq * ratio) / sr;
        while (modPhase >= 1.0) modPhase -= 1.0;
        float modSig = fastSin (static_cast<float> (modPhase) * 6.28318530718f) * modIndex * modEnvLevel;
        carrierPhase += carrierFreq / sr;
        while (carrierPhase >= 1.0) carrierPhase -= 1.0;
        float out = fastSin (static_cast<float> (carrierPhase) * 6.28318530718f + modSig + lastOut * feedback);
        lastOut = flushDenormal (out); // QA C4: denormal protection on FM feedback
        return out;
    }

    void reset() noexcept { carrierPhase = 0.0; modPhase = 0.0; modEnvLevel = 0.0f; lastOut = 0.0f; active = false; }
    bool isActive() const noexcept { return active; }

private:
    double sr = 44100.0, carrierFreq = 200.0, carrierPhase = 0.0, modPhase = 0.0, ratio = 1.4;
    float modIndex = 1.0f, modEnvLevel = 0.0f, modEnvDecayRate = 0.01f, feedback = 0.0f, lastOut = 0.0f;
    bool active = false;
};

//==============================================================================
// ModalResonator — 8-mode parallel bandpass resonator bank.
//==============================================================================
class ModalResonator
{
public:
    static constexpr int kModes = 8;

    void prepare (double sampleRate) noexcept
    {
        sr = static_cast<float> (sampleRate);
        for (auto& r : res) r.setMode (CytomicSVF::Mode::BandPass);
        reset();
    }

    void trigger (float baseFreqHz, float charAmt, float toneAmt, float snapAmt) noexcept
    {
        static constexpr float membraneR[kModes] = { 1.0f, 1.59f, 2.14f, 2.30f, 2.65f, 2.92f, 3.16f, 3.50f };
        float inharm = 1.0f + charAmt * 0.5f;
        float logInharm = std::log (inharm); // QA C3: one log instead of 8 pows
        for (int i = 0; i < kModes; ++i)
        {
            float r = membraneR[i] * fastExp (logInharm * (static_cast<float> (i) * 0.1f));
            float freq = clamp (baseFreqHz * r, 20.0f, sr * 0.45f);
            float q = 0.99f - static_cast<float> (i) * 0.02f;
            res[i].setCoefficients (freq, q, sr);
            float edge = static_cast<float> (i) / static_cast<float> (kModes - 1);
            amps[i] = lerp (1.0f - edge * 0.5f, edge + 0.3f, toneAmt);
        }
        exciteLevel = 1.0f;
        float exciteDur = 0.001f + (1.0f - snapAmt) * 0.01f;
        exciteDecay = 1.0f - fastExp (-1.0f / (sr * exciteDur * 0.368f)); // QA W1
        active = true;
    }

    float process() noexcept
    {
        if (!active) return 0.0f;
        exciteLevel -= exciteLevel * exciteDecay;
        exciteLevel = flushDenormal (exciteLevel);
        float exc = noise.process() * exciteLevel;
        float sum = 0.0f;
        for (int i = 0; i < kModes; ++i)
            sum += res[i].processSample (exc * amps[i]);
        return sum * (1.0f / static_cast<float> (kModes));
    }

    void reset() noexcept { for (auto& r : res) r.reset(); exciteLevel = 0.0f; active = false; }
    bool isActive() const noexcept { return active; }

private:
    float sr = 44100.0f;
    std::array<CytomicSVF, kModes> res;
    float amps[kModes] = {}, exciteLevel = 0.0f, exciteDecay = 0.01f;
    bool active = false;
    OnsetNoiseGen noise;
};

//==============================================================================
// KarplusStrongPerc — Plucked/struck string delay line + averaging filter.
//==============================================================================
class KarplusStrongPerc
{
public:
    static constexpr int kMaxDelay = 4096;

    void prepare (double sampleRate) noexcept { sr = sampleRate; reset(); }

    void trigger (float freqHz, float snapAmt, float toneAmt, float charAmt, float decayAmt) noexcept
    {
        float delaySamp = static_cast<float> (sr) / std::max (freqHz, 20.0f);
        delayLen = std::min (std::max (static_cast<int> (delaySamp), 1), kMaxDelay - 1);
        int burstLen = std::min (delayLen, std::max (1, static_cast<int> (sr * (0.001f + snapAmt * 0.01f))));
        for (int i = 0; i < kMaxDelay; ++i)
            buf[i] = (i < burstLen) ? noise.process() : 0.0f;
        writePos = 0;
        filterCoeff = 0.3f + toneAmt * 0.5f;
        blendFactor = charAmt;
        fbGain = clamp (0.95f + decayAmt * 0.049f, 0.9f, 0.999f);
        lastOut = 0.0f;
        active = true;
    }

    float process() noexcept
    {
        if (!active) return 0.0f;
        int readPos = writePos - delayLen;
        if (readPos < 0) readPos += kMaxDelay;
        float cur = buf[readPos];
        float filt = filterCoeff * cur + (1.0f - filterCoeff) * lastOut;
        filt = flushDenormal (filt);
        if (blendFactor < 0.5f && noise.process() > blendFactor * 2.0f)
            filt = -filt;
        buf[writePos] = filt * fbGain;
        lastOut = filt;
        writePos = (writePos + 1) & (kMaxDelay - 1);
        if (std::abs (filt) < 1e-7f && std::abs (cur) < 1e-7f) { active = false; return 0.0f; }
        return filt;
    }

    void reset() noexcept { std::memset (buf, 0, sizeof (buf)); writePos = 0; lastOut = 0.0f; active = false; }
    bool isActive() const noexcept { return active; }

private:
    double sr = 44100.0;
    float buf[kMaxDelay] = {};
    int delayLen = 100, writePos = 0;
    float filterCoeff = 0.5f, fbGain = 0.99f, blendFactor = 0.5f, lastOut = 0.0f;
    bool active = false;
    OnsetNoiseGen noise;
};

//==============================================================================
// PhaseDistPerc — Casio CZ-inspired phase distortion percussion.
//==============================================================================
class PhaseDistPerc
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; reset(); }

    void trigger (float freqHz, float snapAmt, float charAmt, float toneAmt) noexcept
    {
        freq = static_cast<double> (freqHz);
        phase = 0.0;
        dcw = charAmt;
        dcwEnvLevel = snapAmt;
        float dcwDecay = 0.01f + (1.0f - snapAmt) * 0.1f;
        dcwEnvRate = static_cast<float> (1.0 - std::exp (-1.0 / (sr * static_cast<double> (dcwDecay) * 0.368)));
        waveShape = toneAmt;
        active = true;
    }

    float process() noexcept
    {
        if (!active) return 0.0f;
        dcwEnvLevel -= dcwEnvLevel * dcwEnvRate;
        dcwEnvLevel = flushDenormal (dcwEnvLevel);
        // QA C5: early-out when DCW envelope is silent and no base DCW
        if (dcwEnvLevel < 1e-6f && dcw < 0.01f) return 0.0f;
        float totalDCW = clamp (dcw + dcwEnvLevel, 0.0f, 2.0f);
        phase += freq / sr;
        while (phase >= 1.0) phase -= 1.0;
        float p = static_cast<float> (phase);
        float dp;
        if (totalDCW > 0.01f)
        {
            float d = totalDCW / (1.0f + totalDCW);
            dp = (p < 0.5f) ? p * (0.5f + d) / 0.5f
                            : (0.5f + d) + (p - 0.5f) * (0.5f - d) / 0.5f;
            dp = clamp (dp, 0.0f, 1.0f);
        }
        else { dp = p; }
        float out = fastSin (dp * 6.28318530718f);
        if (waveShape > 0.33f && waveShape < 0.67f)
            out = 2.0f * std::abs (out) - 1.0f;
        else if (waveShape >= 0.67f)
            out = clamp (out * 2.0f, -1.0f, 1.0f);
        return out;
    }

    void reset() noexcept { phase = 0.0; dcwEnvLevel = 0.0f; active = false; }
    bool isActive() const noexcept { return active; }

private:
    double sr = 44100.0, freq = 200.0, phase = 0.0;
    float dcw = 0.0f, dcwEnvLevel = 0.0f, dcwEnvRate = 0.01f, waveShape = 0.0f;
    bool active = false;
};

//==============================================================================
// OnsetCharacterStage — Post-mixer grit (tanh saturation) + warmth (LP filter).
//==============================================================================
class OnsetCharacterStage
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = static_cast<float> (sampleRate);
        warmthLPL.setMode (CytomicSVF::Mode::LowPass);
        warmthLPR.setMode (CytomicSVF::Mode::LowPass);
    }

    // Call once per block with the current warmth value to update filter coefficients.
    // Keeps setCoefficients() (which computes 2*sin(pi*f/sr)) out of the per-sample path.
    void setWarmth (float warmth) noexcept
    {
        if (warmth > 0.01f)
        {
            float cutoff = 18000.0f - warmth * 14000.0f;
            warmthLPL.setCoefficients (cutoff, 0.1f, sr);
            warmthLPR.setCoefficients (cutoff, 0.1f, sr);
        }
    }

    void process (float& left, float& right, float grit, float warmth) noexcept
    {
        if (grit > 0.01f)
        {
            float drive = 1.0f + grit * 6.0f;
            left  = fastTanh (left * drive) / drive * (1.0f + grit * 2.0f);
            right = fastTanh (right * drive) / drive * (1.0f + grit * 2.0f);
        }
        if (warmth > 0.01f)
        {
            // Coefficients are block-constant — caller should call setWarmth()
            // once per block before process() to avoid per-sample coefficient recompute.
            left  = warmthLPL.processSample (left);
            right = warmthLPR.processSample (right);
        }
    }

    void reset() noexcept { warmthLPL.reset(); warmthLPR.reset(); }

private:
    float sr = 44100.0f;
    CytomicSVF warmthLPL, warmthLPR;
};

//==============================================================================
// OnsetDelay — Dark tape-style delay with LP in feedback path.
//==============================================================================
class OnsetDelay
{
public:
    static constexpr int kMaxDelaySamples = 88200; // 2 seconds at 44.1k

    void prepare (double sampleRate) noexcept
    {
        sr = static_cast<float> (sampleRate);
        std::memset (bufL, 0, sizeof (bufL));
        std::memset (bufR, 0, sizeof (bufR));
        writePos = 0;
        fbFilterL.setMode (CytomicSVF::Mode::LowPass);
        fbFilterR.setMode (CytomicSVF::Mode::LowPass);
        fbFilterL.setCoefficients (4000.0f, 0.0f, sr);
        fbFilterR.setCoefficients (4000.0f, 0.0f, sr);
    }

    void process (float& left, float& right, float timeSec, float feedback, float mix) noexcept
    {
        if (mix < 0.001f) return;

        int delaySamp = std::max (1, std::min (static_cast<int> (timeSec * sr), kMaxDelaySamples - 1));
        int readPos = writePos - delaySamp;
        if (readPos < 0) readPos += kMaxDelaySamples;

        float delL = bufL[readPos];
        float delR = bufR[readPos];

        float fbL = fbFilterL.processSample (delL) * feedback;
        float fbR = fbFilterR.processSample (delR) * feedback;
        fbL = flushDenormal (fbL);
        fbR = flushDenormal (fbR);

        bufL[writePos] = left + fbL;
        bufR[writePos] = right + fbR;
        writePos = (writePos + 1) % kMaxDelaySamples;

        left  += delL * mix;
        right += delR * mix;
    }

    void reset() noexcept
    {
        std::memset (bufL, 0, sizeof (bufL));
        std::memset (bufR, 0, sizeof (bufR));
        writePos = 0;
        fbFilterL.reset(); fbFilterR.reset();
    }

private:
    float sr = 44100.0f;
    float bufL[kMaxDelaySamples] = {};
    float bufR[kMaxDelaySamples] = {};
    int writePos = 0;
    CytomicSVF fbFilterL, fbFilterR;
};

//==============================================================================
// OnsetReverb — Tight room reverb using 4 comb filters + 2 allpass diffusers.
//==============================================================================
class OnsetReverb
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = static_cast<float> (sampleRate);
        float scale = sr / 44100.0f;
        combLen[0] = static_cast<int> (1116 * scale);
        combLen[1] = static_cast<int> (1188 * scale);
        combLen[2] = static_cast<int> (1277 * scale);
        combLen[3] = static_cast<int> (1356 * scale);
        apLen[0]   = static_cast<int> (556 * scale);
        apLen[1]   = static_cast<int> (441 * scale);

        // Allocate comb and allpass buffers to exactly the needed length.
        // Static arrays sized for 44.1 kHz would overflow at higher sample rates.
        for (int c = 0; c < 4; ++c)
        {
            combBufLen[c] = combLen[c] + 1;
            combBuf[c].assign (static_cast<size_t> (combBufLen[c]), 0.0f);
        }
        for (int a = 0; a < 2; ++a)
        {
            apBufLen[a] = apLen[a] + 1;
            apBuf[a].assign (static_cast<size_t> (apBufLen[a]), 0.0f);
        }

        reset();
    }

    void process (float& left, float& right, float roomSize, float decay, float mix) noexcept
    {
        if (mix < 0.001f) return;

        float input = (left + right) * 0.5f;
        float fb = 0.5f + decay * 0.45f;
        float damp = 0.3f + (1.0f - roomSize) * 0.5f;

        float sum = 0.0f;
        for (int c = 0; c < 4; ++c)
        {
            int rp = combPos[c] - combLen[c];
            if (rp < 0) rp += combBufLen[c];
            float del = combBuf[c][static_cast<size_t> (rp)];
            combLP[c] = combLP[c] + damp * (del - combLP[c]);
            combLP[c] = flushDenormal (combLP[c]);
            combBuf[c][static_cast<size_t> (combPos[c])] = input + combLP[c] * fb;
            combPos[c] = (combPos[c] + 1) % combBufLen[c];
            sum += del;
        }
        sum *= 0.25f;

        for (int a = 0; a < 2; ++a)
        {
            int rp = apPos[a] - apLen[a];
            if (rp < 0) rp += apBufLen[a];
            float del = apBuf[a][static_cast<size_t> (rp)];
            float apIn = sum + del * 0.5f;
            apBuf[a][static_cast<size_t> (apPos[a])] = apIn;
            sum = del - apIn * 0.5f;
            apPos[a] = (apPos[a] + 1) % apBufLen[a];
        }

        left  += sum * mix;
        right += sum * mix;
    }

    void reset() noexcept
    {
        for (int c = 0; c < 4; ++c)
        {
            std::fill (combBuf[c].begin(), combBuf[c].end(), 0.0f);
            combPos[c] = 0;
            combLP[c]  = 0.0f;
        }
        for (int a = 0; a < 2; ++a)
        {
            std::fill (apBuf[a].begin(), apBuf[a].end(), 0.0f);
            apPos[a] = 0;
        }
    }

private:
    float sr = 44100.0f;
    std::vector<float> combBuf[4];
    float combLP[4]  = {};
    int combLen[4]   = { 1116, 1188, 1277, 1356 };
    int combBufLen[4]= { 1117, 1189, 1278, 1357 };  // len + 1, sized in prepare()
    int combPos[4]   = {};
    std::vector<float> apBuf[2];
    int apLen[2]     = { 556, 441 };
    int apBufLen[2]  = { 557, 442 };                 // len + 1, sized in prepare()
    int apPos[2]     = {};
};

//==============================================================================
// OnsetLoFi — Bitcrush + sample rate reduction.
//==============================================================================
class OnsetLoFi
{
public:
    // Process with precomputed steps/invSteps to avoid per-sample std::pow.
    // steps = std::pow(2.0f, bits), invSteps = 1.0f / steps (block-constant).
    void process (float& left, float& right, float steps, float invSteps, float mix) noexcept
    {
        if (mix < 0.001f) return;

        float crushL = std::round (left  * steps) * invSteps;
        float crushR = std::round (right * steps) * invSteps;

        left  = left  + (crushL - left)  * mix;
        right = right + (crushR - right) * mix;
    }
};

//==============================================================================
// OnsetVoice — Single drum voice: Layer X + Layer O + blend + transient + env.
//==============================================================================
struct OnsetVoice
{
    // Circuit type: 0=BridgedT, 1=NoiseBurst, 2=Metallic
    int circuitType = 0;
    bool isClap = false;
    float baseFreq = 220.0f;
    float sr = 44100.0f;

    // Layer X
    BridgedTOsc bridgedT;
    NoiseBurstCircuit noiseBurst;
    MetallicOsc metallic;

    // Layer O
    FMPercussion fm;
    ModalResonator modal;
    KarplusStrongPerc karplusStrong;
    PhaseDistPerc phaseDist;

    // Shared
    OnsetEnvelope ampEnv;
    OnsetTransient transient;
    CytomicSVF voiceFilter;

    // State
    bool active = false;
    float lastOutput = 0.0f;
    float velocity = 1.0f;
    bool triggeredThisBlock = false;

    void prepare (double sampleRate)
    {
        sr = static_cast<float> (sampleRate);
        bridgedT.prepare (sampleRate);
        noiseBurst.prepare (sampleRate);
        metallic.prepare (sampleRate);
        fm.prepare (sampleRate);
        modal.prepare (sampleRate);
        karplusStrong.prepare (sampleRate);
        phaseDist.prepare (sampleRate);
        ampEnv.prepare (sampleRate);
        transient.prepare (sampleRate);
        voiceFilter.setMode (CytomicSVF::Mode::LowPass);
    }

    void triggerVoice (float vel, float blend, int algoMode,
                       float pitch, float decay, float tone,
                       float snap, float body, float character,
                       int envShape = 0)
    {
        velocity = vel;
        active = true;
        triggeredThisBlock = true;

        float freq = baseFreq * fastExp (pitch * (0.693147f / 12.0f));

        // Trigger Layer X
        switch (circuitType)
        {
            case 0: bridgedT.trigger (freq, snap, body, character); break;
            case 1: noiseBurst.trigger (freq, snap, tone, character, isClap); break;
            case 2: metallic.trigger (fastExp (pitch * (0.693147f / 12.0f)), tone, character); break;
        }

        // Trigger Layer O
        switch (algoMode)
        {
            case 0: fm.trigger (freq, snap, character, tone); break;
            case 1: modal.trigger (freq, character, tone, snap); break;
            case 2: karplusStrong.trigger (freq, snap, tone, character, decay); break;
            case 3: phaseDist.trigger (freq, snap, character, tone); break;
        }

        // Envelope: shape determines AD / AHD / ADSR behavior
        auto shape = static_cast<OnsetEnvelope::Shape> (std::min (envShape, 2));
        float holdTime = (shape != OnsetEnvelope::Shape::AD) ? body * 0.05f : 0.0f;
        float sustainLvl = (shape == OnsetEnvelope::Shape::ADSR) ? 0.3f : 0.0f;
        ampEnv.trigger (0.001f, decay, shape, holdTime, sustainLvl);

        // Transient: snap scaled by velocity
        transient.trigger (snap * vel, freq);

        // QA I3: Reset filter state on retrigger to prevent artifacts
        voiceFilter.reset();
        float cutoff = 200.0f + tone * 18000.0f;
        voiceFilter.setCoefficients (cutoff, 0.1f, sr);
    }

    // QA C1: Blend gains precomputed per block, passed in to avoid per-sample trig
    float processSample (float blendGainX, float blendGainO, int algoMode) noexcept
    {
        if (!active) return 0.0f;

        float envLevel = ampEnv.process();
        if (!ampEnv.isActive())
        {
            active = false;
            lastOutput = 0.0f;
            return 0.0f;
        }

        // Render active Layer X
        float layerX = 0.0f;
        switch (circuitType)
        {
            case 0: layerX = bridgedT.process(); break;
            case 1: layerX = noiseBurst.process(); break;
            case 2: layerX = metallic.process(); break;
        }

        // Render active Layer O
        float layerO = 0.0f;
        switch (algoMode)
        {
            case 0: layerO = fm.process(); break;
            case 1: layerO = modal.process(); break;
            case 2: layerO = karplusStrong.process(); break;
            case 3: layerO = phaseDist.process(); break;
        }

        // Equal-power blend crossfade (gains precomputed per block)
        float blended = layerX * blendGainX + layerO * blendGainO;

        // Add transient (pre-filter)
        blended += transient.process();

        // Voice filter
        blended = voiceFilter.processSample (blended);

        // Apply envelope and velocity
        float out = blended * envLevel * velocity;
        lastOutput = out;
        return out;
    }

    void releaseVoice() noexcept { ampEnv.release(); }

    void choke() noexcept
    {
        active = false;
        ampEnv.reset();
        bridgedT.reset(); noiseBurst.reset(); metallic.reset();
        fm.reset(); modal.reset(); karplusStrong.reset(); phaseDist.reset();
        transient.reset(); voiceFilter.reset();
        lastOutput = 0.0f;
    }

    void reset() { choke(); }
};

//==============================================================================
// OnsetEngine — Percussive synthesis engine: 8 fixed voices, dual layers, blend.
// Implements SynthEngine for the XOmnibus mega-tool.
//==============================================================================
class OnsetEngine : public SynthEngine
{
public:
    static constexpr int kNumVoices = 8;

    //-- Voice configuration table -----------------------------------------------
    struct VoiceCfg { int circuit; int defAlgo; int midiNote; float baseFreq; float defBlend; float defDecay; bool clap; };

    static constexpr VoiceCfg kVoiceCfg[kNumVoices] = {
        { 0, 1, 36,  55.0f,  0.2f, 0.5f,  false },  // V1 Kick
        { 1, 0, 38, 180.0f,  0.5f, 0.3f,  false },  // V2 Snare
        { 2, 0, 42, 8000.0f, 0.7f, 0.05f, false },  // V3 HH-Closed
        { 2, 0, 46, 8000.0f, 0.7f, 0.4f,  false },  // V4 HH-Open
        { 1, 3, 39, 1200.0f, 0.4f, 0.25f, true  },  // V5 Clap
        { 0, 1, 45, 110.0f,  0.3f, 0.4f,  false },  // V6 Tom
        { 0, 2, 37, 220.0f,  0.6f, 0.3f,  false },  // V7 Perc A
        { 2, 1, 44, 440.0f,  0.8f, 0.35f, false },  // V8 Perc B
    };

    //-- Static parameter registration -------------------------------------------
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl (params);
    }

    //-- SynthEngine lifecycle ---------------------------------------------------
    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        blockSize = maxBlockSize;
        couplingBuffer.setSize (2, maxBlockSize);
        couplingBuffer.clear();

        masterFilter.setMode (CytomicSVF::Mode::LowPass);
        masterFilterR.setMode (CytomicSVF::Mode::LowPass);
        characterStage.prepare (sampleRate);
        fxDelay.prepare (sampleRate);
        fxReverb.prepare (sampleRate);

        for (int v = 0; v < kNumVoices; ++v)
        {
            voices[v].circuitType = kVoiceCfg[v].circuit;
            voices[v].isClap = kVoiceCfg[v].clap;
            voices[v].baseFreq = kVoiceCfg[v].baseFreq;
            voices[v].prepare (sampleRate);
        }
    }

    void releaseResources() override
    {
        for (auto& v : voices) v.reset();
    }

    void reset() override
    {
        for (auto& v : voices) v.reset();
        couplingBuffer.clear();
        couplingFilterMod = 0.0f;
        couplingDecayMod = 0.0f;
        couplingBlendMod = 0.0f;
        std::memset (voicePeaks, 0, sizeof (voicePeaks));
        characterStage.reset();
        fxDelay.reset();
        fxReverb.reset();
    }

    //-- Render ------------------------------------------------------------------
    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi, int numSamples) override
    {
        // 1. Snapshot parameters
        float vBlend[kNumVoices], vPitch[kNumVoices], vDecay[kNumVoices];
        float vTone[kNumVoices], vSnap[kNumVoices], vBody[kNumVoices];
        float vChar[kNumVoices], vLevel[kNumVoices], vPan[kNumVoices];
        int   vAlgo[kNumVoices];

        int   vEnvShape[kNumVoices];

        for (int v = 0; v < kNumVoices; ++v)
        {
            auto& vp = voiceParams[v];
            vBlend[v] = clamp (vp.blend ? vp.blend->load() + couplingBlendMod : kVoiceCfg[v].defBlend, 0.0f, 1.0f);
            vAlgo[v]  = vp.algoMode ? static_cast<int> (vp.algoMode->load()) : kVoiceCfg[v].defAlgo;
            vPitch[v] = vp.pitch  ? vp.pitch->load()  : 0.0f;
            vDecay[v] = clamp ((vp.decay ? vp.decay->load() : kVoiceCfg[v].defDecay) + couplingDecayMod, 0.001f, 8.0f);
            vTone[v]  = clamp ((vp.tone ? vp.tone->load() : 0.5f) + couplingFilterMod, 0.0f, 1.0f);
            vSnap[v]  = vp.snap   ? vp.snap->load()   : 0.3f;
            vBody[v]  = vp.body   ? vp.body->load()   : 0.5f;
            vChar[v]  = vp.character ? vp.character->load() : 0.0f;
            vLevel[v] = vp.level  ? vp.level->load()  : 0.7f;
            vPan[v]   = vp.pan    ? vp.pan->load()    : 0.0f;
            vEnvShape[v] = envShapeParams[v] ? static_cast<int> (envShapeParams[v]->load()) : 0;
        }

        // --- Macro snapshots ---
        float mMachine = macroMachine ? macroMachine->load() : 0.5f;
        float mPunch   = macroPunch   ? macroPunch->load()   : 0.5f;
        float mMutate  = macroMutate  ? macroMutate->load()  : 0.0f;
        // M3 SPACE: wired in Phase 4 (FX rack)

        // M1 MACHINE: bias all blends toward circuit (0) or algorithm (1)
        float machineBias = (mMachine - 0.5f) * 1.0f;
        for (int v = 0; v < kNumVoices; ++v)
            vBlend[v] = clamp (vBlend[v] + machineBias, 0.0f, 1.0f);

        // M2 PUNCH: bias snap and body (0=soft, 1=aggressive)
        float punchBias = (mPunch - 0.5f) * 0.6f;
        for (int v = 0; v < kNumVoices; ++v)
        {
            vSnap[v] = clamp (vSnap[v] + punchBias, 0.0f, 1.0f);
            vBody[v] = clamp (vBody[v] + punchBias, 0.0f, 1.0f);
        }

        // M4 MUTATE: per-block random drift on blend + character
        if (mMutate > 0.01f)
        {
            for (int v = 0; v < kNumVoices; ++v)
            {
                vBlend[v] = clamp (vBlend[v] + mutateRng.process() * mMutate * 0.2f, 0.0f, 1.0f);
                vChar[v]  = clamp (vChar[v]  + mutateRng.process() * mMutate * 0.2f, 0.0f, 1.0f);
            }
        }

        // --- Cross-Voice Coupling (XVC): previous-block peaks modulate targets ---
        float xvcGlobal = xvcGlobalAmount ? xvcGlobalAmount->load() : 0.5f;
        if (xvcGlobal > 0.01f)
        {
            float kickPk  = voicePeaks[0] * xvcGlobal;
            float snarePk = voicePeaks[1] * xvcGlobal;

            // Kick → Snare filter (tone offset)
            float ksfAmt = xvcKickSnareFilter ? xvcKickSnareFilter->load() : 0.15f;
            vTone[1] = clamp (vTone[1] + kickPk * ksfAmt, 0.0f, 1.0f);

            // Snare → Hat decay tighten
            float shdAmt = xvcSnareHatDecay ? xvcSnareHatDecay->load() : 0.10f;
            vDecay[2] = clamp (vDecay[2] - snarePk * shdAmt * 0.5f, 0.001f, 8.0f);

            // Kick → Tom pitch duck
            float ktpAmt = xvcKickTomPitch ? xvcKickTomPitch->load() : 0.0f;
            vPitch[5] = vPitch[5] - kickPk * ktpAmt * 6.0f;

            // Snare → Perc A blend shift
            float spbAmt = xvcSnarePercBlend ? xvcSnarePercBlend->load() : 0.0f;
            vBlend[6] = clamp (vBlend[6] + snarePk * spbAmt * 0.3f, 0.0f, 1.0f);
        }

        float masterLevel = percLevel  ? percLevel->load()  : 0.8f;
        float masterDrive = percDrive  ? percDrive->load()  : 0.0f;
        float masterTone  = percTone   ? percTone->load()   : 0.5f;

        // FX + Character stage snapshots
        float pCharGrit   = charGrit       ? charGrit->load()       : 0.0f;
        float pCharWarmth = charWarmth     ? charWarmth->load()     : 0.5f;
        float pDelTime    = fxDelayTime    ? fxDelayTime->load()    : 0.3f;
        float pDelFb      = fxDelayFeedback? fxDelayFeedback->load(): 0.3f;
        float pDelMix     = fxDelayMix     ? fxDelayMix->load()     : 0.0f;
        float pRevSize    = fxReverbSize   ? fxReverbSize->load()   : 0.4f;
        float pRevDecay   = fxReverbDecay  ? fxReverbDecay->load()  : 0.3f;
        float pRevMix     = fxReverbMix    ? fxReverbMix->load()    : 0.0f;
        float pLofiBits   = fxLofiBits     ? fxLofiBits->load()     : 16.0f;
        float pLofiMix    = fxLofiMix      ? fxLofiMix->load()      : 0.0f;
        // Precompute block-constant LoFi quantisation steps (avoids per-sample std::pow).
        const float lofiSteps    = std::pow (2.0f, pLofiBits);
        const float lofiInvSteps = 1.0f / lofiSteps;

        // M3 SPACE macro: drives reverb mix + delay feedback
        float mSpace = macroSpace ? macroSpace->load() : 0.0f;
        pRevMix = clamp (pRevMix + mSpace * 0.6f, 0.0f, 1.0f);
        pDelFb  = clamp (pDelFb  + mSpace * 0.3f, 0.0f, 0.95f);

        // QA I1: Apply master tone as LP filter on output
        float masterCutoff = 200.0f + masterTone * 18000.0f;
        masterFilter.setCoefficients (masterCutoff, 0.1f, static_cast<float> (sr));
        masterFilterR.setCoefficients (masterCutoff, 0.1f, static_cast<float> (sr));

        // Update character stage warmth filter coefficients once per block
        // (was previously recomputed inside every per-sample process() call).
        characterStage.setWarmth (pCharWarmth);

        // QA C6: Clear coupling buffer to prevent stale tail reads
        couplingBuffer.clear();

        // Hat choke mode (XVC parameter — can be disabled)
        bool hatChokeEnabled = xvcHatChoke ? xvcHatChoke->load() >= 0.5f : true;

        // 2. Process MIDI — trigger voices + note-off for ADSR
        for (const auto metadata : midi)
        {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                int voiceIdx = noteToVoice (msg.getNoteNumber());
                if (voiceIdx < 0) continue;

                // Hat choke: closed hat (V3) chokes open hat (V4) — XVC controlled
                if (voiceIdx == 2 && hatChokeEnabled) voices[3].choke();

                float vel = msg.getFloatVelocity();
                voices[voiceIdx].triggerVoice (vel, vBlend[voiceIdx], vAlgo[voiceIdx],
                    vPitch[voiceIdx], vDecay[voiceIdx], vTone[voiceIdx],
                    vSnap[voiceIdx], vBody[voiceIdx], vChar[voiceIdx],
                    vEnvShape[voiceIdx]);
            }
            else if (msg.isNoteOff())
            {
                int voiceIdx = noteToVoice (msg.getNoteNumber());
                if (voiceIdx >= 0) voices[voiceIdx].releaseVoice();
            }
        }

        // 3. Precompute block-constant voice gains (QA C1 + C2: hoist trig/sqrt)
        constexpr float halfPi = 1.5707963267948966f;
        float blendGainX[kNumVoices], blendGainO[kNumVoices];
        float panGainL[kNumVoices], panGainR[kNumVoices];
        for (int v = 0; v < kNumVoices; ++v)
        {
            blendGainX[v] = std::cos (vBlend[v] * halfPi);
            blendGainO[v] = std::sin (vBlend[v] * halfPi);
            panGainL[v] = std::sqrt (0.5f * (1.0f - vPan[v]));
            panGainR[v] = std::sqrt (0.5f * (1.0f + vPan[v]));
        }

        // 4. Render all voices (with peak tracking for XVC)
        auto* outL = buffer.getWritePointer (0);
        auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
        auto* cplL = couplingBuffer.getWritePointer (0);
        auto* cplR = couplingBuffer.getWritePointer (1);
        float blockPeaks[kNumVoices] = {};

        for (int s = 0; s < numSamples; ++s)
        {
            float sumL = 0.0f, sumR = 0.0f;

            for (int v = 0; v < kNumVoices; ++v)
            {
                if (!voices[v].active) continue;

                float sample = voices[v].processSample (blendGainX[v], blendGainO[v], vAlgo[v]);
                float absSamp = std::abs (sample * vLevel[v]);
                if (absSamp > blockPeaks[v]) blockPeaks[v] = absSamp;

                sumL += sample * vLevel[v] * panGainL[v];
                sumR += sample * vLevel[v] * panGainR[v];
            }

            // Master drive (soft clip)
            if (masterDrive > 0.01f)
            {
                float dg = 1.0f + masterDrive * 4.0f;
                sumL = fastTanh (sumL * dg);
                sumR = fastTanh (sumR * dg);
            }

            // QA I1: master tone LP filter
            sumL = masterFilter.processSample (sumL);
            sumR = masterFilterR.processSample (sumR);

            // Character stage: grit (saturation) + warmth (LP)
            characterStage.process (sumL, sumR, pCharGrit, pCharWarmth);

            // FX chain: Delay → Reverb → LoFi
            fxDelay.process (sumL, sumR, pDelTime, pDelFb, pDelMix);
            fxReverb.process (sumL, sumR, pRevSize, pRevDecay, pRevMix);
            fxLoFi.process (sumL, sumR, lofiSteps, lofiInvSteps, pLofiMix);

            sumL *= masterLevel;
            sumR *= masterLevel;

            outL[s] += sumL;
            if (outR) outR[s] += sumR;
            cplL[s] = sumL;
            cplR[s] = sumR;
        }

        // 4. Store voice peaks for next block's XVC
        for (int v = 0; v < kNumVoices; ++v) voicePeaks[v] = blockPeaks[v];

        // 5. Clear per-block state
        for (auto& v : voices) v.triggeredThisBlock = false;
        couplingFilterMod = 0.0f;
        couplingDecayMod = 0.0f;
        couplingBlendMod = 0.0f;
    }

    //-- Coupling ----------------------------------------------------------------
    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        if (channel < couplingBuffer.getNumChannels() && sampleIndex < couplingBuffer.getNumSamples())
            return couplingBuffer.getSample (channel, sampleIndex);
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* sourceBuffer, int numSamples) override
    {
        float sum = 0.0f;
        if (sourceBuffer != nullptr)
            for (int i = 0; i < numSamples; ++i) sum += sourceBuffer[i];
        float avgMod = (numSamples > 0 && sourceBuffer != nullptr)
                     ? (sum / static_cast<float> (numSamples)) * amount
                     : amount;  // fall back to scalar amount if no buffer

        switch (type)
        {
            case CouplingType::AmpToFilter:   couplingFilterMod += avgMod; break;
            case CouplingType::EnvToDecay:    couplingDecayMod  += avgMod; break;
            case CouplingType::RhythmToBlend: couplingBlendMod  += avgMod; break;
            case CouplingType::AmpToChoke:
                if (avgMod > 0.5f)
                    for (auto& v : voices) v.choke();
                break;
            default: break;
        }
    }

    //-- Parameters --------------------------------------------------------------
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override { return {}; }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        for (int v = 0; v < kNumVoices; ++v)
        {
            juce::String pre = "perc_v" + juce::String (v + 1) + "_";
            auto& vp = voiceParams[v];
            vp.blend     = apvts.getRawParameterValue (pre + "blend");
            vp.algoMode  = apvts.getRawParameterValue (pre + "algoMode");
            vp.pitch     = apvts.getRawParameterValue (pre + "pitch");
            vp.decay     = apvts.getRawParameterValue (pre + "decay");
            vp.tone      = apvts.getRawParameterValue (pre + "tone");
            vp.snap      = apvts.getRawParameterValue (pre + "snap");
            vp.body      = apvts.getRawParameterValue (pre + "body");
            vp.character = apvts.getRawParameterValue (pre + "character");
            vp.level     = apvts.getRawParameterValue (pre + "level");
            vp.pan       = apvts.getRawParameterValue (pre + "pan");
            envShapeParams[v] = apvts.getRawParameterValue (pre + "envShape");
        }
        percLevel = apvts.getRawParameterValue ("perc_level");
        percDrive = apvts.getRawParameterValue ("perc_drive");
        percTone  = apvts.getRawParameterValue ("perc_masterTone");

        macroMachine = apvts.getRawParameterValue ("perc_macro_machine");
        macroPunch   = apvts.getRawParameterValue ("perc_macro_punch");
        macroSpace   = apvts.getRawParameterValue ("perc_macro_space");
        macroMutate  = apvts.getRawParameterValue ("perc_macro_mutate");

        xvcKickSnareFilter = apvts.getRawParameterValue ("perc_xvc_kick_to_snare_filter");
        xvcSnareHatDecay   = apvts.getRawParameterValue ("perc_xvc_snare_to_hat_decay");
        xvcKickTomPitch    = apvts.getRawParameterValue ("perc_xvc_kick_to_tom_pitch");
        xvcSnarePercBlend  = apvts.getRawParameterValue ("perc_xvc_snare_to_perc_blend");
        xvcHatChoke        = apvts.getRawParameterValue ("perc_xvc_hat_choke");
        xvcGlobalAmount    = apvts.getRawParameterValue ("perc_xvc_global_amount");

        charGrit   = apvts.getRawParameterValue ("perc_char_grit");
        charWarmth = apvts.getRawParameterValue ("perc_char_warmth");

        fxDelayTime     = apvts.getRawParameterValue ("perc_fx_delay_time");
        fxDelayFeedback = apvts.getRawParameterValue ("perc_fx_delay_feedback");
        fxDelayMix      = apvts.getRawParameterValue ("perc_fx_delay_mix");
        fxReverbSize    = apvts.getRawParameterValue ("perc_fx_reverb_size");
        fxReverbDecay   = apvts.getRawParameterValue ("perc_fx_reverb_decay");
        fxReverbMix     = apvts.getRawParameterValue ("perc_fx_reverb_mix");
        fxLofiBits      = apvts.getRawParameterValue ("perc_fx_lofi_bits");
        fxLofiMix       = apvts.getRawParameterValue ("perc_fx_lofi_mix");
    }

    //-- Identity ----------------------------------------------------------------
    juce::String getEngineId() const override { return "Onset"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF0066FF); }
    int getMaxVoices() const override { return kNumVoices; }

private:
    std::array<OnsetVoice, kNumVoices> voices;
    juce::AudioBuffer<float> couplingBuffer;
    CytomicSVF masterFilter;   // QA I1: master tone LP filter (L channel)
    CytomicSVF masterFilterR;  // R channel
    OnsetCharacterStage characterStage;
    OnsetDelay fxDelay;
    OnsetReverb fxReverb;
    OnsetLoFi fxLoFi;
    double sr = 44100.0;
    int blockSize = 512;
    int lastRenderedSamples = 0;

    // Coupling modulation accumulators
    float couplingFilterMod = 0.0f;
    float couplingDecayMod = 0.0f;
    float couplingBlendMod = 0.0f;

    // Cached parameter pointers
    struct VParams {
        std::atomic<float>* blend = nullptr;
        std::atomic<float>* algoMode = nullptr;
        std::atomic<float>* pitch = nullptr;
        std::atomic<float>* decay = nullptr;
        std::atomic<float>* tone = nullptr;
        std::atomic<float>* snap = nullptr;
        std::atomic<float>* body = nullptr;
        std::atomic<float>* character = nullptr;
        std::atomic<float>* level = nullptr;
        std::atomic<float>* pan = nullptr;
    };
    std::array<VParams, kNumVoices> voiceParams;
    std::atomic<float>* percLevel = nullptr;
    std::atomic<float>* percDrive = nullptr;
    std::atomic<float>* percTone  = nullptr;

    // Per-voice envelope shape pointers
    std::array<std::atomic<float>*, kNumVoices> envShapeParams {};

    // Macro parameter pointers
    std::atomic<float>* macroMachine = nullptr;
    std::atomic<float>* macroPunch   = nullptr;
    std::atomic<float>* macroSpace   = nullptr;
    std::atomic<float>* macroMutate  = nullptr;

    // Cross-Voice Coupling (XVC) parameter pointers
    std::atomic<float>* xvcKickSnareFilter = nullptr;
    std::atomic<float>* xvcSnareHatDecay   = nullptr;
    std::atomic<float>* xvcKickTomPitch    = nullptr;
    std::atomic<float>* xvcSnarePercBlend  = nullptr;
    std::atomic<float>* xvcHatChoke        = nullptr;
    std::atomic<float>* xvcGlobalAmount    = nullptr;

    // FX parameter pointers
    std::atomic<float>* fxDelayTime     = nullptr;
    std::atomic<float>* fxDelayFeedback = nullptr;
    std::atomic<float>* fxDelayMix      = nullptr;
    std::atomic<float>* fxReverbSize    = nullptr;
    std::atomic<float>* fxReverbDecay   = nullptr;
    std::atomic<float>* fxReverbMix     = nullptr;
    std::atomic<float>* fxLofiBits      = nullptr;
    std::atomic<float>* fxLofiMix       = nullptr;

    // Character stage parameter pointers
    std::atomic<float>* charGrit   = nullptr;
    std::atomic<float>* charWarmth = nullptr;

    // XVC state: previous-block voice peak amplitudes
    float voicePeaks[kNumVoices] = {};

    // MUTATE macro RNG
    OnsetNoiseGen mutateRng;

    //-- MIDI note to voice mapping ----------------------------------------------
    int noteToVoice (int note) const noexcept
    {
        for (int i = 0; i < kNumVoices; ++i)
            if (kVoiceCfg[i].midiNote == note) return i;
        // Extended: bass drum alternate (note 35)
        if (note == 35) return 0;
        return -1;
    }

    //-- Parameter registration --------------------------------------------------
    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using Float = juce::AudioParameterFloat;
        using Choice = juce::AudioParameterChoice;

        static const char* voiceNames[kNumVoices] =
            { "Kick", "Snare", "HH-C", "HH-O", "Clap", "Tom", "PercA", "PercB" };
        static const float defBlend[kNumVoices] = { 0.2f, 0.5f, 0.7f, 0.7f, 0.4f, 0.3f, 0.6f, 0.8f };
        static const int   defAlgo[kNumVoices]  = { 1, 0, 0, 0, 3, 1, 2, 1 };
        static const float defDecay[kNumVoices] = { 0.5f, 0.3f, 0.05f, 0.4f, 0.25f, 0.4f, 0.3f, 0.35f };

        for (int v = 0; v < kNumVoices; ++v)
        {
            juce::String pre = "perc_v" + juce::String (v + 1) + "_";
            juce::String name = juce::String (voiceNames[v]);

            params.push_back (std::make_unique<Float> (
                juce::ParameterID (pre + "blend", 1), name + " Blend",
                juce::NormalisableRange<float> (0.0f, 1.0f), defBlend[v]));

            params.push_back (std::make_unique<Choice> (
                juce::ParameterID (pre + "algoMode", 1), name + " Algo",
                juce::StringArray { "FM", "Modal", "K-S", "PhaseDist" }, defAlgo[v]));

            params.push_back (std::make_unique<Float> (
                juce::ParameterID (pre + "pitch", 1), name + " Pitch",
                juce::NormalisableRange<float> (-24.0f, 24.0f), 0.0f));

            params.push_back (std::make_unique<Float> (
                juce::ParameterID (pre + "decay", 1), name + " Decay",
                juce::NormalisableRange<float> (0.01f, 8.0f, 0.0f, 0.3f), defDecay[v]));

            params.push_back (std::make_unique<Float> (
                juce::ParameterID (pre + "tone", 1), name + " Tone",
                juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

            params.push_back (std::make_unique<Float> (
                juce::ParameterID (pre + "snap", 1), name + " Snap",
                juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

            params.push_back (std::make_unique<Float> (
                juce::ParameterID (pre + "body", 1), name + " Body",
                juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

            params.push_back (std::make_unique<Float> (
                juce::ParameterID (pre + "character", 1), name + " Character",
                juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

            params.push_back (std::make_unique<Float> (
                juce::ParameterID (pre + "level", 1), name + " Level",
                juce::NormalisableRange<float> (0.0f, 1.0f), 0.7f));

            params.push_back (std::make_unique<Float> (
                juce::ParameterID (pre + "pan", 1), name + " Pan",
                juce::NormalisableRange<float> (-1.0f, 1.0f), 0.0f));

            params.push_back (std::make_unique<Choice> (
                juce::ParameterID (pre + "envShape", 1), name + " Env",
                juce::StringArray { "AD", "AHD", "ADSR" }, 0));
        }

        // Global parameters
        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_level", 1), "Perc Level",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.8f));

        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_drive", 1), "Perc Drive",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_masterTone", 1), "Perc Tone",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // Macros
        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_macro_machine", 1), "Machine",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_macro_punch", 1), "Punch",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_macro_space", 1), "Space",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_macro_mutate", 1), "Mutate",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // Cross-Voice Coupling (XVC)
        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_xvc_kick_to_snare_filter", 1), "XVC Kick>Snare Flt",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.15f));

        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_xvc_snare_to_hat_decay", 1), "XVC Snare>Hat Dcy",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.10f));

        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_xvc_kick_to_tom_pitch", 1), "XVC Kick>Tom Pch",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_xvc_snare_to_perc_blend", 1), "XVC Snare>Perc Bld",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_xvc_hat_choke", 1), "XVC Hat Choke",
            juce::NormalisableRange<float> (0.0f, 1.0f), 1.0f));

        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_xvc_global_amount", 1), "XVC Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // Character stage
        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_char_grit", 1), "Perc Grit",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_char_warmth", 1), "Perc Warmth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // FX rack
        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_fx_delay_time", 1), "Perc Delay Time",
            juce::NormalisableRange<float> (0.01f, 1.0f, 0.0f, 0.5f), 0.3f));

        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_fx_delay_feedback", 1), "Perc Delay FB",
            juce::NormalisableRange<float> (0.0f, 0.95f), 0.3f));

        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_fx_delay_mix", 1), "Perc Delay Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_fx_reverb_size", 1), "Perc Reverb Size",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));

        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_fx_reverb_decay", 1), "Perc Reverb Decay",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_fx_reverb_mix", 1), "Perc Reverb Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_fx_lofi_bits", 1), "Perc LoFi Bits",
            juce::NormalisableRange<float> (4.0f, 16.0f), 16.0f));

        params.push_back (std::make_unique<Float> (
            juce::ParameterID ("perc_fx_lofi_mix", 1), "Perc LoFi Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    }
};

} // namespace xomnibus
