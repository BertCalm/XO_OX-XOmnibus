#pragma once
//==============================================================================
//
//  OfferingTransient.h — Per-Type Drum Synthesis Models
//  XO_OX Designs | XOlokun Multi-Engine Synthesizer
//
//  8 distinct drum synthesis topologies — each type has unique signal flow,
//  not shared oscillators with different parameter values.
//
//  Type 0: Kick    — Sine + pitch envelope + sub-harmonic + body resonance
//  Type 1: Snare   — Dual-source (triangle/noise) + transient click
//  Type 2: CHat    — 6-operator metallic FM network (TR-808 ratios)
//  Type 3: OHat    — Shared metallic network + extended decay + choke
//  Type 4: Clap    — Multi-burst noise (3-5 bursts) + reverb tail
//  Type 5: Rim     — Click impulse + high-Q resonant bandpass
//  Type 6: Tom     — Tunable sine + gentle pitch envelope
//  Type 7: Perc    — Comb-filtered noise (Karplus-Strong adjacent)
//
//  Accent: Crate Wax Yellow #E5B80B | Prefix: ofr_
//
//==============================================================================

#include "../../DSP/FastMath.h"
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace xolokun {

//==============================================================================
// OfferingNoiseGen — xorshift32 PRNG for percussion noise generation.
//==============================================================================
struct OfferingNoiseGen
{
    void seed (uint32_t s) noexcept { state_ = s ? s : 1; }

    float process() noexcept
    {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 17;
        state_ ^= state_ << 5;
        return static_cast<float> (static_cast<int32_t> (state_)) / 2147483648.0f;
    }

private:
    uint32_t state_ = 1;
};

//==============================================================================
// OfferingADEnvelope — Simple attack-decay envelope for drum synthesis.
// Exponential decay with coefficient caching.
//==============================================================================
struct OfferingADEnvelope
{
    void prepare (float sampleRate) noexcept { sr_ = sampleRate; }

    void trigger (float attackSec, float decaySec) noexcept
    {
        stage_ = Stage::Attack;
        level_ = 0.0f;

        float aSec = std::max (attackSec, 0.0001f);
        attackRate_ = 1.0f / (sr_ * aSec);

        float dSec = std::max (decaySec, 0.001f);
        if (dSec != lastDecay_)
        {
            lastDecay_ = dSec;
            decayCoeff_ = 1.0f - std::exp (-4.6f / (sr_ * dSec));
        }
    }

    float process() noexcept
    {
        switch (stage_)
        {
            case Stage::Idle: return 0.0f;
            case Stage::Attack:
                level_ += attackRate_;
                if (level_ >= 1.0f)
                {
                    level_ = 1.0f;
                    stage_ = Stage::Decay;
                }
                return level_;
            case Stage::Decay:
                level_ -= level_ * decayCoeff_;
                level_ = flushDenormal (level_);
                if (level_ < 1e-6f) { level_ = 0.0f; stage_ = Stage::Idle; }
                return level_;
        }
        return 0.0f;
    }

    bool isActive() const noexcept { return stage_ != Stage::Idle; }
    float getLevel() const noexcept { return level_; }
    void reset() noexcept { stage_ = Stage::Idle; level_ = 0.0f; lastDecay_ = -1.0f; }

private:
    enum class Stage { Idle, Attack, Decay };
    float sr_ = 44100.0f;
    Stage stage_ = Stage::Idle;
    float level_ = 0.0f;
    float attackRate_ = 0.01f;
    float decayCoeff_ = 0.001f;
    float lastDecay_ = -1.0f;
};

//==============================================================================
// OfferingOnePole — Simple one-pole filter for smoothing / lowpass / highpass.
//==============================================================================
struct OfferingOnePole
{
    void setLowpass (float cutoffHz, float sampleRate) noexcept
    {
        float w = 2.0f * 3.14159265f * cutoffHz / sampleRate;
        coeff_ = std::exp (-w);
        mode_ = 0;
    }

    void setHighpass (float cutoffHz, float sampleRate) noexcept
    {
        float w = 2.0f * 3.14159265f * cutoffHz / sampleRate;
        coeff_ = std::exp (-w);
        mode_ = 1;
    }

    float process (float x) noexcept
    {
        z_ = x + coeff_ * (z_ - x);
        z_ = flushDenormal (z_);
        return (mode_ == 0) ? z_ : (x - z_);
    }

    void reset() noexcept { z_ = 0.0f; }

private:
    float coeff_ = 0.99f;
    float z_ = 0.0f;
    int mode_ = 0; // 0 = LP, 1 = HP
};

//==============================================================================
// OfferingBandpass — 2nd-order bandpass via biquad (matched-Z design).
//==============================================================================
struct OfferingBandpass
{
    void setParams (float centerHz, float q, float sampleRate) noexcept
    {
        float w0 = 2.0f * 3.14159265f * centerHz / sampleRate;
        float sinW0 = std::sin (w0);
        float cosW0 = std::cos (w0);
        float alpha = sinW0 / (2.0f * q);

        float a0inv = 1.0f / (1.0f + alpha);
        b0_ = (alpha) * a0inv;
        b1_ = 0.0f;
        b2_ = (-alpha) * a0inv;
        a1_ = (-2.0f * cosW0) * a0inv;
        a2_ = (1.0f - alpha) * a0inv;
    }

    float process (float x) noexcept
    {
        float y = b0_ * x + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;
        y = flushDenormal (y);
        x2_ = x1_; x1_ = x;
        y2_ = y1_; y1_ = y;
        return y;
    }

    void reset() noexcept { x1_ = x2_ = y1_ = y2_ = 0.0f; }

private:
    float b0_ = 0.0f, b1_ = 0.0f, b2_ = 0.0f;
    float a1_ = 0.0f, a2_ = 0.0f;
    float x1_ = 0.0f, x2_ = 0.0f;
    float y1_ = 0.0f, y2_ = 0.0f;
};

//==============================================================================
// OfferingTransient — Master transient generator with 8 drum type topologies.
//
// Each type implements its own synthesis signal chain. The voiceBody parameter
// controls the tonal/noise balance within each type's specific topology.
//==============================================================================
class OfferingTransient
{
public:
    //--------------------------------------------------------------------------
    // Drum type enum — matches ofr_voiceType parameter range [0-7]
    //--------------------------------------------------------------------------
    enum Type : int
    {
        Kick     = 0,
        Snare    = 1,
        CHat     = 2,
        OHat     = 3,
        Clap     = 4,
        Rim      = 5,
        Tom      = 6,
        Perc     = 7
    };

    void prepare (float sampleRate) noexcept
    {
        sr_ = sampleRate;
        ampEnv_.prepare (sampleRate);
        pitchEnv_.prepare (sampleRate);
        noise_.seed (12345);
        bp1_.reset();
        bp2_.reset();
        lp_.reset();
        hp_.reset();
    }

    //--------------------------------------------------------------------------
    // Trigger a drum hit with type-specific synthesis.
    //
    // velocity: MIDI velocity normalized [0, 1]
    // tune: semitone offset [-24, +24]
    // decay: decay time in seconds [0.001, 2.0]
    // body: tonal/noise balance [0, 1] (0=pure noise, 1=pure tone)
    // snap: transient attack sharpness [0, 1]
    // pitchEnvDepth: pitch envelope depth [0, 1]
    // sat: saturation amount [0, 1]
    //--------------------------------------------------------------------------
    void trigger (int type, float velocity, float tune, float decay,
                  float body, float snap, float pitchEnvDepth, float sat) noexcept
    {
        type_ = type;
        velocity_ = velocity;
        body_ = body;
        snap_ = snap;
        pitchEnvDepth_ = pitchEnvDepth;
        sat_ = sat;
        phase_ = 0.0f;
        active_ = true;

        // Semitone to frequency ratio
        float tuneRatio = std::pow (2.0f, tune / 12.0f);

        switch (type)
        {
            case Kick:
                baseFreq_ = 60.0f * tuneRatio;
                startFreq_ = baseFreq_ + pitchEnvDepth * 600.0f; // 200-800 Hz start
                ampEnv_.trigger (0.0005f, decay);
                pitchEnv_.trigger (0.0005f, 0.05f + (1.0f - snap) * 0.1f);
                bp1_.setParams (baseFreq_, 2.0f + body * 6.0f, sr_);
                break;

            case Snare:
                baseFreq_ = 180.0f * tuneRatio;
                ampEnv_.trigger (0.0001f, decay);
                bp1_.setParams (baseFreq_, 3.0f, sr_);              // tonal body
                hp_.setHighpass (2000.0f, sr_);                      // noise body HP
                lp_.setLowpass (12000.0f, sr_);                      // noise body LP
                break;

            case CHat:
            case OHat:
            {
                // 6-operator metallic network — inharmonic ratios from TR-808
                float baseHat = 400.0f * tuneRatio;
                metalOscFreqs_[0] = baseHat * 1.0f;
                metalOscFreqs_[1] = baseHat * 1.4471f;
                metalOscFreqs_[2] = baseHat * 1.6818f;
                metalOscFreqs_[3] = baseHat * 1.9265f;
                metalOscFreqs_[4] = baseHat * 2.5028f;
                metalOscFreqs_[5] = baseHat * 2.6637f;
                metalPhases_[0] = metalPhases_[1] = metalPhases_[2] = 0.0f;
                metalPhases_[3] = metalPhases_[4] = metalPhases_[5] = 0.0f;

                float hatDecay = (type == CHat) ? std::min (decay, 0.08f) : decay;
                ampEnv_.trigger (0.0001f, hatDecay);

                float bpCenter = (type == CHat) ? 10000.0f : 8000.0f;
                float bpQ = (type == CHat) ? 1.0f : 0.7f;
                bp1_.setParams (bpCenter, bpQ, sr_);
                break;
            }

            case Clap:
                burstCount_ = 3 + static_cast<int> (snap * 2.0f); // 3-5 bursts
                burstSpacing_ = static_cast<int> (sr_ * (0.005f + (1.0f - snap) * 0.01f));
                burstIndex_ = 0;
                burstSample_ = 0;
                burstEnvLevel_ = 0.0f;
                ampEnv_.trigger (0.0001f, decay); // tail envelope
                bp1_.setParams (1500.0f + tune * 50.0f, 2.0f, sr_);
                break;

            case Rim:
                baseFreq_ = 600.0f * tuneRatio;
                clickSamples_ = static_cast<int> (sr_ * 0.0005f); // < 1ms click
                ampEnv_.trigger (0.0001f, std::min (decay, 0.06f));
                bp1_.setParams (baseFreq_, 12.0f, sr_); // high-Q resonance
                hp_.setHighpass (6000.0f, sr_);
                break;

            case Tom:
                baseFreq_ = 120.0f * tuneRatio; // wider range than kick
                startFreq_ = baseFreq_ + pitchEnvDepth * 200.0f; // gentler sweep
                ampEnv_.trigger (0.0005f, decay);
                pitchEnv_.trigger (0.0005f, 0.1f + (1.0f - snap) * 0.15f); // gentler
                break;

            case Perc:
                // Comb filter delay time sets pitch
                combDelayMs_ = 1000.0f / (100.0f * tuneRatio); // 50-500 Hz range
                combDelaySamples_ = static_cast<int> (sr_ * combDelayMs_ * 0.001f);
                combDelaySamples_ = std::max (1, std::min (combDelaySamples_, kMaxCombDelay - 1));
                combFeedback_ = 0.3f + body * 0.5f; // 0.3-0.8
                combWritePos_ = 0;
                std::fill (combBuffer_, combBuffer_ + kMaxCombDelay, 0.0f);
                ampEnv_.trigger (0.0001f, decay);
                hp_.setHighpass (200.0f, sr_);
                break;
        }

        // Reset filter state for clean attack
        bp1_.reset();
        bp2_.reset();
    }

    //--------------------------------------------------------------------------
    // Process one sample of drum synthesis output.
    //--------------------------------------------------------------------------
    float process() noexcept
    {
        if (!active_) return 0.0f;

        float env = ampEnv_.process();
        if (!ampEnv_.isActive()) { active_ = false; return 0.0f; }

        float out = 0.0f;

        switch (type_)
        {
            case Kick: out = processKick (env); break;
            case Snare: out = processSnare (env); break;
            case CHat:
            case OHat: out = processHat (env); break;
            case Clap: out = processClap (env); break;
            case Rim: out = processRim (env); break;
            case Tom: out = processTom (env); break;
            case Perc: out = processPerc (env); break;
        }

        // Apply velocity scaling
        out *= velocity_;

        // Apply saturation
        if (sat_ > 0.001f)
        {
            float drive = 1.0f + sat_ * 4.0f;
            out = fastTanh (out * drive) / fastTanh (drive); // normalized saturation
        }

        return out;
    }

    bool isActive() const noexcept { return active_; }
    float getAmpEnvLevel() const noexcept { return ampEnv_.getLevel(); }

    // envToPitch: per-sample frequency modulation driven by an external envelope signal.
    // ratio > 1.0 raises pitch, ratio < 1.0 lowers pitch.  Applied to pitched types only
    // (Kick, Snare, Tom); hat/clap/rim/perc are noise-based so modulation has no effect.
    // Call once per sample before process() to schedule the ratio for that sample.
    void setFreqMod (float ratio) noexcept { extFreqRatio_ = ratio; }

    void reset() noexcept { active_ = false; ampEnv_.reset(); pitchEnv_.reset(); extFreqRatio_ = 1.0f; }

    // For choke groups: open hat killed by closed hat
    void choke() noexcept
    {
        if (active_ && (type_ == OHat))
        {
            // Quick fade-out rather than abrupt silence
            choking_ = true;
            chokeGain_ = 1.0f;
        }
    }

private:
    //--------------------------------------------------------------------------
    // TYPE 0: KICK — Sine + pitch envelope + sub-harmonic + body resonance
    //--------------------------------------------------------------------------
    float processKick (float env) noexcept
    {
        // Pitch envelope: exponential sweep from startFreq to baseFreq.
        // extFreqRatio_ applies envToPitch coupling — modulates instantaneous frequency.
        float pitchEnvVal = pitchEnv_.process();
        float freq = (baseFreq_ + (startFreq_ - baseFreq_) * pitchEnvVal) * extFreqRatio_;

        // Main sine oscillator
        float phaseInc = freq / sr_;
        phase_ += phaseInc;
        if (phase_ >= 1.0f) phase_ -= 1.0f;
        float sine = std::sin (phase_ * 6.2831853f);

        // Body resonance bandpass
        float resonant = bp1_.process (sine);

        // Mix body (resonant) vs raw sine based on body param
        float tonal = sine * (1.0f - body_ * 0.5f) + resonant * body_ * 0.5f;

        // Sub-harmonic at half frequency, -6dB
        float subPhaseInc = freq * 0.5f / sr_;
        subPhase_ += subPhaseInc;
        if (subPhase_ >= 1.0f) subPhase_ -= 1.0f;
        float sub = std::sin (subPhase_ * 6.2831853f) * 0.5f;

        return (tonal + sub) * env;
    }

    //--------------------------------------------------------------------------
    // TYPE 1: SNARE — Dual-source (triangle + noise) + transient click
    //--------------------------------------------------------------------------
    float processSnare (float env) noexcept
    {
        // Source A: Triangle oscillator
        float phaseInc = baseFreq_ / sr_;
        phase_ += phaseInc;
        if (phase_ >= 1.0f) phase_ -= 1.0f;
        float tri = 4.0f * std::abs (phase_ - 0.5f) - 1.0f;
        float tonal = bp1_.process (tri);

        // Source B: Filtered noise
        float rawNoise = noise_.process();
        float noiseHP = hp_.process (rawNoise);
        float noiseBP = lp_.process (noiseHP);

        // Mix: body=1 is pure tonal, body=0 is pure noise
        float mixed = tonal * body_ + noiseBP * (1.0f - body_);

        // Transient click: impulse HP at 4kHz for snap
        float click = 0.0f;
        if (env > 0.95f) // Only during initial attack
            click = noise_.process() * snap_ * 0.5f;

        return (mixed + click) * env;
    }

    //--------------------------------------------------------------------------
    // TYPE 2/3: HAT — 6-operator metallic FM network (TR-808 ratios)
    //--------------------------------------------------------------------------
    float processHat (float env) noexcept
    {
        // 6 square oscillators at inharmonic ratios
        float sum = 0.0f;
        for (int i = 0; i < 6; ++i)
        {
            metalPhases_[i] += metalOscFreqs_[i] / sr_;
            if (metalPhases_[i] >= 1.0f) metalPhases_[i] -= 1.0f;
            float sq = (metalPhases_[i] < 0.5f) ? 1.0f : -1.0f;
            sum += sq;
        }

        // Ring-modulate in pairs: (0,1), (2,3), (4,5) then sum
        float pair0 = (metalPhases_[0] < 0.5f ? 1.0f : -1.0f) *
                       (metalPhases_[1] < 0.5f ? 1.0f : -1.0f);
        float pair1 = (metalPhases_[2] < 0.5f ? 1.0f : -1.0f) *
                       (metalPhases_[3] < 0.5f ? 1.0f : -1.0f);
        float pair2 = (metalPhases_[4] < 0.5f ? 1.0f : -1.0f) *
                       (metalPhases_[5] < 0.5f ? 1.0f : -1.0f);

        float metallic = (pair0 + pair1 + pair2) / 3.0f;

        // Bandpass to shape the metallic character
        float filtered = bp1_.process (metallic);

        // Body controls tonal content: 0=all metallic, 1=adds tonal sine
        float tonal = body_ * std::sin (metalPhases_[0] * 6.2831853f) * 0.3f;

        float out = filtered + tonal;

        // Choke handling for open hat
        if (choking_)
        {
            chokeGain_ *= 0.995f; // ~7ms fade at 44.1kHz
            out *= chokeGain_;
            if (chokeGain_ < 0.001f) { active_ = false; choking_ = false; }
        }

        return out * env;
    }

    //--------------------------------------------------------------------------
    // TYPE 4: CLAP — Multi-burst noise with reverb tail
    //--------------------------------------------------------------------------
    float processClap (float env) noexcept
    {
        float out = 0.0f;

        // Burst phase: 3-5 short noise bursts spaced apart
        if (burstIndex_ < burstCount_)
        {
            burstSample_++;
            int burstDuration = static_cast<int> (sr_ * 0.004f); // 4ms per burst

            if (burstSample_ <= burstDuration)
            {
                // Short noise burst with fast envelope
                float burstEnv = 1.0f - static_cast<float> (burstSample_) / static_cast<float> (burstDuration);
                out = noise_.process() * burstEnv * 0.8f;
            }
            else if (burstSample_ >= burstSpacing_)
            {
                burstSample_ = 0;
                burstIndex_++;
            }
        }

        // Reverb tail: filtered noise with longer decay (from amp envelope)
        float tail = noise_.process();
        tail = bp1_.process (tail);
        out += tail * env * 0.4f;

        return out;
    }

    //--------------------------------------------------------------------------
    // TYPE 5: RIM — Click impulse + high-Q resonant bandpass
    //--------------------------------------------------------------------------
    float processRim (float env) noexcept
    {
        float click = 0.0f;
        if (clickSamples_ > 0)
        {
            // Ultra-short impulse through HP filter
            float impulse = noise_.process();
            click = hp_.process (impulse) * 0.6f;
            clickSamples_--;
        }

        // High-Q resonance: sine at target freq through tight bandpass
        float phaseInc = baseFreq_ / sr_;
        phase_ += phaseInc;
        if (phase_ >= 1.0f) phase_ -= 1.0f;
        float resonance = bp1_.process (std::sin (phase_ * 6.2831853f));

        return (click * 0.7f + resonance * 0.5f) * env;
    }

    //--------------------------------------------------------------------------
    // TYPE 6: TOM — Tunable sine + gentle pitch envelope
    //--------------------------------------------------------------------------
    float processTom (float env) noexcept
    {
        // Gentler pitch envelope than kick.
        // extFreqRatio_ applies envToPitch coupling — modulates instantaneous frequency.
        float pitchEnvVal = pitchEnv_.process();
        float freq = (baseFreq_ + (startFreq_ - baseFreq_) * pitchEnvVal) * extFreqRatio_;

        float phaseInc = freq / sr_;
        phase_ += phaseInc;
        if (phase_ >= 1.0f) phase_ -= 1.0f;
        float sine = std::sin (phase_ * 6.2831853f);

        // Saturation applied in outer process() — no double-sat
        return sine * env;
    }

    //--------------------------------------------------------------------------
    // TYPE 7: PERC — Comb-filtered noise (Karplus-Strong adjacent)
    //--------------------------------------------------------------------------
    float processPerc (float env) noexcept
    {
        // White noise excitation
        float excitation = noise_.process() * env;

        // Comb filter: delay line with feedback
        int readPos = (combWritePos_ - combDelaySamples_ + kMaxCombDelay) % kMaxCombDelay;
        float delayed = combBuffer_[readPos];

        float combOut = excitation + delayed * combFeedback_;
        combOut = flushDenormal (combOut);

        combBuffer_[combWritePos_] = combOut;
        combWritePos_ = (combWritePos_ + 1) % kMaxCombDelay;

        // Highpass to remove sub-bass rumble
        float filtered = hp_.process (combOut);

        return filtered * env;
    }

    //--------------------------------------------------------------------------
    // State
    //--------------------------------------------------------------------------
    float sr_ = 44100.0f;
    int type_ = 0;
    float velocity_ = 0.0f;
    float body_ = 0.5f;
    float snap_ = 0.5f;
    float pitchEnvDepth_ = 0.3f;
    float sat_ = 0.15f;
    bool active_ = false;

    // External per-sample frequency modulation ratio (envToPitch coupling).
    // Multiplied into baseFreq/startFreq in pitched process functions each sample.
    float extFreqRatio_ = 1.0f;

    // Oscillator state
    float phase_ = 0.0f;
    float subPhase_ = 0.0f;
    float baseFreq_ = 60.0f;
    float startFreq_ = 200.0f;

    // Metallic oscillator bank (hats)
    float metalOscFreqs_[6] = {};
    float metalPhases_[6] = {};

    // Clap burst state
    int burstCount_ = 3;
    int burstSpacing_ = 200;
    int burstIndex_ = 0;
    int burstSample_ = 0;
    float burstEnvLevel_ = 0.0f;

    // Rim click counter
    int clickSamples_ = 0;

    // Comb filter state (perc)
    static constexpr int kMaxCombDelay = 2048;
    float combBuffer_[kMaxCombDelay] = {};
    int combWritePos_ = 0;
    int combDelaySamples_ = 100;
    float combDelayMs_ = 5.0f;
    float combFeedback_ = 0.5f;

    // Choke state (open hat)
    bool choking_ = false;
    float chokeGain_ = 1.0f;

    // DSP components
    OfferingADEnvelope ampEnv_;
    OfferingADEnvelope pitchEnv_;
    OfferingNoiseGen noise_;
    OfferingBandpass bp1_;
    OfferingBandpass bp2_;
    OfferingOnePole lp_;
    OfferingOnePole hp_;
};

} // namespace xolokun
