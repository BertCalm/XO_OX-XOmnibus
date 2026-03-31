#pragma once
#include <cmath>
#include <algorithm>
#include <array>

#include "../../DSP/FastMath.h"
#include "OxytocinLoveEnvelope.h"
#include "OxytocinThermal.h"
#include "OxytocinDrive.h"
#include "OxytocinReactive.h"
#include "OxytocinTriangle.h"
#include "OxytocinMemory.h"
#include "OxytocinParamSnapshot.h"

/// Simple per-sample amplitude ADSR
/// P1-2: coefficients are cached at block rate — never computed per-sample.
class AmpEnvelope
{
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    void prepare (double sampleRate) noexcept
    {
        sr    = sampleRate;
        // P1-7: guard against unprepared usage
        jassert (sr > 0.0);
        invSr = (sampleRate > 0.0) ? static_cast<float> (1.0 / sampleRate) : 0.0f;
        // Force coefficient recompute on first use
        lastDecay   = -1.0f;
        lastRelease = -1.0f;
    }

    void noteOn()  noexcept { stage = Stage::Attack; value = 0.0f; }
    void noteOff() noexcept { if (stage != Stage::Idle) { stage = Stage::Release; } }
    bool isActive() const noexcept { return stage != Stage::Idle; }
    float getValue() const noexcept { return value; }

    /// Call once per block to cache coefficients before the sample loop.
    /// P1-2: exp() computed here, not per-sample.
    void updateCoefficients (float decay, float release) noexcept
    {
        if (decay != lastDecay)
        {
            cachedDecayCoeff   = std::exp (-invSr / std::max (0.0001f, decay));
            lastDecay = decay;
        }
        if (release != lastRelease)
        {
            cachedReleaseCoeff = std::exp (-invSr / std::max (0.0001f, release));
            lastRelease = release;
        }
    }

    float tick (float attack, float decay, float sustain, float release) noexcept
    {
        switch (stage)
        {
            case Stage::Idle: value = 0.0f; break;
            case Stage::Attack:
                value += invSr / std::max (0.0001f, attack);
                if (value >= 1.0f) { value = 1.0f; stage = Stage::Decay; }
                break;
            case Stage::Decay:
            {
                // Use cached coeff — updateCoefficients() must be called once per block
                (void)decay;  // coeff already in cachedDecayCoeff
                value = sustain + (value - sustain) * cachedDecayCoeff;
                if (value <= sustain + 0.001f) { value = sustain; stage = Stage::Sustain; }
                break;
            }
            case Stage::Sustain:
                value = sustain;
                break;
            case Stage::Release:
            {
                // Use cached coeff
                (void)release;  // coeff already in cachedReleaseCoeff
                value *= cachedReleaseCoeff;
                if (value < 0.0001f) { value = 0.0f; stage = Stage::Idle; }
                break;
            }
        }
        return value;
    }

private:
    Stage  stage = Stage::Idle;
    float  value = 0.0f;
    double sr    = 0.0;   // P1-7: default 0
    float  invSr = 0.0f;  // PERF-2: cached 1/sr

    // P1-2: cached coefficients
    float  lastDecay         = -1.0f;
    float  lastRelease       = -1.0f;
    float  cachedDecayCoeff  = 0.0f;
    float  cachedReleaseCoeff = 0.0f;
};

/// PolyBLEP sawtooth oscillator
class PolyBLEPOsc
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; phase = 0.0f; }
    void reset()   noexcept { phase = 0.0f; }

    void setFrequency (float hz) noexcept
    {
        inc = std::clamp (static_cast<float> (hz / sr), 0.0f, 0.5f);
    }

    float tick() noexcept
    {
        // PolyBLEP correction for sawtooth
        float out = 2.0f * phase - 1.0f;
        out -= polyBLEP (phase, inc);
        phase += inc;
        if (phase >= 1.0f) phase -= 1.0f;
        return out;
    }

private:
    static float polyBLEP (float t, float dt) noexcept
    {
        if (t < dt)
        {
            float x = t / dt;
            return x + x - x * x - 1.0f;
        }
        else if (t > 1.0f - dt)
        {
            float x = (t - 1.0f) / dt;
            return x * x + x + x + 1.0f;
        }
        return 0.0f;
    }

    float  phase = 0.0f;
    float  inc   = 0.01f;
    double sr    = 44100.0;
};

/// OxytocinVoice — one polyphonic voice.
class OxytocinVoice
{
public:
    OxytocinVoice() = default;

    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        osc.prepare (sampleRate);
        ampEnv.prepare (sampleRate);
        passionEnv.prepare (sampleRate);
        intimacyEnv.prepare (sampleRate);
        commitmentEnv.prepare (sampleRate);
        thermal.prepare (sampleRate);
        drive.prepare (sampleRate);
        reactive.prepare (sampleRate);
    }

    void noteOn (int midiNote, float velocity) noexcept
    {
        note          = midiNote;
        vel           = velocity;
        pitchBendRatio = 1.0f;
        active        = true;

        // P1-5: reset ALL DSP state on voice steal/retrigger to prevent contamination
        fbSample  = 0.0f;
        cToICarry = 0.0f;   // FIX 2: clear C→I carry register on voice reset
        osc.reset();
        thermal.reset();
        drive.reset();
        reactive.reset();  // also resets obsessionDriftPhase via P1-8 fix in reactive

        ampEnv.noteOn();
        passionEnv.noteOn();
        intimacyEnv.noteOn();
        commitmentEnv.noteOn();

        // Voice steal click prevention: fade in over 5 ms instead of jumping.
        // stealFadeLevel ramps from 0 → 1 in processBlock(); once it reaches 1
        // it stays there for the lifetime of the note, so there is no per-sample
        // branch overhead after the ramp completes.
        stealFadeLevel = 0.0f;
        // stealFadeInc is set in processBlock() once sr is known.
        stealFading    = true;
    }

    void noteOff() noexcept
    {
        ampEnv.noteOff();
        passionEnv.noteOff();
        intimacyEnv.noteOff();
        commitmentEnv.noteOff();
    }

    bool isActive() const noexcept { return active && ampEnv.isActive(); }

    void setPitchBend (float ratio) noexcept { pitchBendRatio = ratio; }

    /// Last-use stamp for LRU voice stealing
    uint64_t lastUseTick = 0;
    int      note        = 60;

    // Effective love values for the last processed block (read by engine for memory)
    // P1-6: these are now block averages, not last-sample values
    float lastEffI = 0.0f, lastEffP = 0.0f, lastEffC = 0.0f;

    /// Process one block.  Output is ADDED to outBuffer.
    /// D004: voiceIndex and totalVoices are passed so detune can spread voices.
    void processBlock (float*              outBuffer,
                       int                 numSamples,
                       const ParamSnapshot& snap,
                       const OxytocinMemory& memory,
                       int                 voiceIndex,
                       int                 totalVoices) noexcept
    {
        if (!isActive()) return;

        // --- Base frequency ---
        float semitones = static_cast<float> (note - 69) + snap.pitch;
        float baseHz    = 440.0f * std::pow (2.0f, semitones / 12.0f) * pitchBendRatio;

        // D004: per-voice detune spread
        // Voice 0 is lowest, voice (totalVoices-1) is highest.
        // Centre is at (totalVoices-1)*0.5 so the spread is symmetric.
        float detuneOffset = 0.0f;
        if (totalVoices > 1 && snap.detune != 0.0f)
        {
            float spread = (static_cast<float> (voiceIndex) - static_cast<float> (totalVoices - 1) * 0.5f)
                           / static_cast<float> (totalVoices - 1);
            detuneOffset = spread * snap.detune;  // detune is in cents (0–100)
        }
        float noteHz = baseHz * std::pow (2.0f, detuneOffset / 1200.0f);

        osc.setFrequency (noteHz);

        // --- Update thermal block-rate coefficients (OPERA P0 lesson) ---
        float blockTimeSec = static_cast<float> (numSamples) / static_cast<float> (sr);
        thermal.updateWarmth (snap.intimacy, snap.warmthRate, blockTimeSec);

        // Fix 1 (FATHOM): Reactive coefficients are now updated per-sample inside
        // the loop, passing boostedC so resonance breathes with the love envelope.
        // The change-guard in updateCoefficients() still avoids redundant tan() calls
        // when boostedC is stable between samples.

        // --- P1-2: cache AmpEnvelope exp coefficients once per block ---
        ampEnv.updateCoefficients (snap.decay, snap.release);

        // --- P1-1: cache LoveEnvelope exp coefficients once per block ---
        passionEnv.updateCoefficients (snap.passionRate, snap.warmthRate, snap.commitRate);
        intimacyEnv.updateCoefficients (snap.passionRate, snap.warmthRate, snap.commitRate);
        commitmentEnv.updateCoefficients (snap.passionRate, snap.warmthRate, snap.commitRate);

        // PERF-3: update obsession sine at block rate
        reactive.updateObsession (blockTimeSec);

        // E5: velocity → passion attack speed.
        // 0.5 at pp (vel≈0), 1.0 at ff (vel=1) → hard hits bloom faster.
        float velAttackScale = 0.5f + 0.5f * vel;
        passionEnv.setAttackScale (velAttackScale);

        // Voice-steal click prevention: compute per-sample ramp increment.
        // stealFadeInc is only used while stealFading == true; after the ramp
        // reaches 1.0 the branch is skipped for the remainder of the note.
        const float stealFadeInc = (sr > 0.0)
                                   ? 1.0f / (kStealFadeTimeSec * static_cast<float> (sr))
                                   : 1.0f;

        // P1-6: accumulators for block-average I/P/C
        float accI = 0.0f, accP = 0.0f, accC = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            // Love envelope ticks
            float pEnv = passionEnv.tick  (LoveEnvelopeType::Passion,    snap.passionRate, snap.warmthRate, snap.commitRate);
            float iEnv = intimacyEnv.tick (LoveEnvelopeType::Intimacy,   snap.passionRate, snap.warmthRate, snap.commitRate);
            float cEnv = commitmentEnv.tick(LoveEnvelopeType::Commitment, snap.passionRate, snap.warmthRate, snap.commitRate);

            // Effective I/P/C = param * envelope
            float effI = snap.intimacy   * iEnv;
            float effP = snap.passion    * pEnv;
            float effC = snap.commitment * cEnv;

            // Memory boost
            float boostedI, boostedP, boostedC;
            memory.applyBoost (effI, effP, effC, snap.memoryDepth, boostedI, boostedP, boostedC);

            // Fix 1 (FATHOM): update Reactive coefficients with live boostedC so
            // the Moog ladder's resonance breathes with the love envelope rather than
            // staying anchored to the static snap.commitment knob position.
            reactive.updateCoefficients (snap.cutoff, boostedC);

            // Topology lock override
            if (snap.topologyLock > 0)
            {
                auto coords = OxytocinTriangle::coordsForLockType (snap.topologyLock);
                // Scale by parameter magnitude to preserve overall amplitude intent
                float mag = (snap.intimacy + snap.passion + snap.commitment) / 3.0f;
                boostedI = coords.I * mag;
                boostedP = coords.P * mag;
                boostedC = coords.C * mag;
            }

            // Velocity: shapes passion peak
            float velScale = 0.5f + 0.5f * vel;   // always some volume
            float passionVelScale = vel;            // D001: velocity → passion peak
            boostedP *= passionVelScale;

            // P1-6: accumulate block average
            accI += boostedI;
            accP += boostedP;
            accC += boostedC;

            // --- Oscillator ---
            float oscSample = osc.tick();

            // Circuit noise floor
            if (snap.circuitNoise > 0.0f)
            {
                // Simple pseudo-random noise (Xorshift)
                noiseSeed ^= noiseSeed << 13;
                noiseSeed ^= noiseSeed >> 17;
                noiseSeed ^= noiseSeed << 5;
                float noiseVal = static_cast<float> (static_cast<int32_t>(noiseSeed)) * (1.0f / 2147483648.0f);
                oscSample += noiseVal * snap.circuitNoise * 0.02f;
            }

            // --- DSP topology routing ---
            float thermalOut = 0.0f, driveOut = 0.0f, reactiveOut = 0.0f;

            // Cross-modulation amounts
            float entAmt = snap.entanglement;

            switch (snap.topology)
            {
                case 0:  // SERIES: Osc → Thermal(I) → Drive(P) → Reactive(C)
                {
                    // P0-3: fold cross-mod into the input BEFORE the single drive call.
                    // Previously drive+reactive were called twice (first call discarded) — fixed.
                    //
                    // FIX 2: C→I circular carry from the PREVIOUS sample is applied here,
                    // before the thermal stage processes this sample.  This closes the
                    // I→P→C→I Serge ring correctly: commitment output at sample N injects
                    // into the intimacy (thermal) input at sample N+1, not after thermalOut
                    // has already been consumed (which was the dead-code bug).
                    thermalOut  = thermal.processSample (oscSample + cToICarry, snap.circuitAge) * boostedI;
                    float dmExtraDrive = thermalOut * entAmt * 0.5f;
                    driveOut    = drive.processSample (thermalOut + dmExtraDrive, boostedP, snap.cutoff, snap.circuitAge);
                    reactiveOut = reactive.processSample (driveOut, boostedC, boostedI, boostedP,
                                                          snap.commitRate, snap.release);

                    // Serge circular routing — complete the I→P→C→I ring.
                    if (entAmt > 0.0f)
                    {
                        // P→C leg: drive output (passion saturation) injects a small
                        // signal into the reactive output.  Simulates the Serge P→C
                        // cross-patch: passion's harmonic energy bleeds into the
                        // commitment filter's output, giving high-entanglement patches
                        // a passion-tinged resonance tail.
                        // NOTE: we ADD to reactiveOut — do NOT call processSample() again,
                        // as that would advance the ladder integrator states twice and
                        // produce sample-doubled distortion.
                        float pToCBleed = std::clamp (driveOut * entAmt * 0.15f, -0.25f, 0.25f);
                        reactiveOut += pToCBleed * boostedP * 0.1f;

                        // FIX 2: C→I leg — store bleed into cToICarry for the NEXT sample.
                        // reactiveOut is now fully computed (including P→C), so we can
                        // derive the carry without any ordering ambiguity.  The carry is
                        // added to oscSample at the top of the next iteration (see above),
                        // meaning it enters the thermal stage input — correctly simulating
                        // the Serge C→I patch where commitment's output influences intimacy
                        // warming on the following sample.
                        cToICarry = std::clamp (reactiveOut * entAmt * 0.2f, -0.25f, 0.25f)
                                    * boostedC * 0.1f;
                    }
                    else
                    {
                        cToICarry = 0.0f;
                    }
                    break;
                }

                case 1:  // PARALLEL: mix of three parallel paths
                {
                    thermalOut  = thermal.processSample  (oscSample, snap.circuitAge) * boostedI;
                    driveOut    = drive.processSample    (oscSample, boostedP, snap.cutoff, snap.circuitAge) * boostedP;
                    reactiveOut = reactive.processSample (oscSample, boostedC, boostedI, boostedP,
                                                          snap.commitRate, snap.release) * boostedC;

                    // Cross-mod in parallel: thermal modulates drive gain
                    float crossScale = 1.0f + thermalOut * entAmt * 0.3f;
                    driveOut *= crossScale;

                    reactiveOut = reactiveOut + driveOut * entAmt * 0.1f;
                    thermalOut  = thermalOut  + reactiveOut * entAmt * 0.1f;

                    // Sum and normalise
                    float sumAmt = boostedI + boostedP + boostedC;
                    if (sumAmt < 0.001f) sumAmt = 1.0f;
                    reactiveOut = (thermalOut + driveOut + reactiveOut) / sumAmt;
                    break;
                }

                case 2:  // FEEDBACK: SERIES + feedback from reactive back to thermal
                {
                    thermalOut  = thermal.processSample  (oscSample + fbSample * snap.feedback, snap.circuitAge) * boostedI;
                    driveOut    = drive.processSample    (thermalOut,  boostedP, snap.cutoff, snap.circuitAge);
                    reactiveOut = reactive.processSample (driveOut, boostedC, boostedI, boostedP,
                                                          snap.commitRate, snap.release);
                    fbSample    = reactiveOut;

                    // Cross-mod in feedback mode: reactive→thermal feedback depth modulated by C
                    float entFB = snap.feedback * (1.0f + boostedC * entAmt);
                    entFB = std::clamp (entFB, 0.0f, 0.95f);
                    fbSample *= entFB;
                    fbSample = xolokun::flushDenormal(fbSample);
                    break;
                }

                default:
                    reactiveOut = oscSample;
                    break;
            }

            // Amplitude envelope
            float ampVal  = ampEnv.tick (snap.attack, snap.decay, snap.sustain, snap.release);
            float sample  = reactiveOut * ampVal * velScale;

            // Voice-steal click prevention: apply fade-in ramp while active.
            if (stealFading)
            {
                sample *= stealFadeLevel;
                stealFadeLevel += stealFadeInc;
                if (stealFadeLevel >= 1.0f)
                {
                    stealFadeLevel = 1.0f;
                    stealFading    = false;  // ramp complete — branch skipped hereafter
                }
            }

            outBuffer[i] += sample;
        }

        // P1-6: store block averages so the engine's memory accumulator gets accurate data
        if (numSamples > 0)
        {
            float invN = 1.0f / static_cast<float> (numSamples);
            lastEffI = accI * invN;
            lastEffP = accP * invN;
            lastEffC = accC * invN;
        }

        if (!isActive())
            active = false;
    }

private:
    bool           active    = false;
    float          vel       = 0.8f;
    float          pitchBendRatio = 1.0f;
    double         sr        = 0.0;   // P1-7: default 0
    float          fbSample  = 0.0f;
    float          cToICarry = 0.0f;  // FIX 2: C→I circular bleed carry register (one-sample delay)
    uint32_t       noiseSeed = 12345678u;

    // Voice-steal click prevention: 5 ms fade-in ramp on every noteOn.
    bool           stealFading    = false;   // true while ramp is active
    float          stealFadeLevel = 1.0f;    // current ramp gain [0..1]; starts at 1 (no fade until noteOn)
    static constexpr float kStealFadeTimeSec = 0.005f;  // 5 ms

    PolyBLEPOsc    osc;
    AmpEnvelope    ampEnv;
    LoveEnvelope   passionEnv;
    LoveEnvelope   intimacyEnv;
    LoveEnvelope   commitmentEnv;
    OxytocinThermal thermal;
    OxytocinDrive   drive;
    OxytocinReactive reactive;
};
