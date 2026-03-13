#pragma once

#include <cmath>
#include <algorithm>
#include <array>
#include <cstdint>
#include "OcelotParamSnapshot.h"
#include "BiomeMorph.h"

namespace xocelot {

// OcelotEmergent — 3-Formant Creature Call Generator
//
// Pipeline: noise source → 3 SVF bandpass formant filters → call envelope → output
//
// Creature types (0-5): Bird / Whale / Insect / Frog / Wolf / Synth
// Biome transforms: Underwater = whale-scale (lower formants, long tails)
//                   Winter = wolf/wind (narrower sweep, longer tail)
//
// Triggers: auto-periodic (creatureRate) OR threshold-gated from EcosystemMatrix
//           (Floor→Emergent: loud percussion fires a creature call)
//
// TODO Phase 2: Voiced source (LF pitch oscillator), full Klatt-style vocal tract.
// Stub: noise excitation through resonant bandpass triples.

class OcelotEmergent
{
public:
    static constexpr int kNumFormants = 3;

    void prepare(double sampleRate)
    {
        sr          = static_cast<float>(sampleRate);
        for (auto& f : formants) f.reset();
        callEnvSample   = -1.0f;
        callTimer       = 0.0f;
        lastAmplitude   = 0.0f;
        lastPattern     = 0.0f;
        noiseSeed       = 0xDEADBEEF;
    }

    void noteOn(int midiNote, float velocity)
    {
        baseNote     = midiNote;
        baseVelocity = velocity;
        active       = true;
        callTimer    = 0.0f;   // trigger a call immediately on noteOn
        callEnvSample = 0.0f;
    }

    void noteOff() { active = false; }

    float renderBlock(float* outL, float* outR, int numSamples,
                      const OcelotParamSnapshot& snap,
                      const BiomeProfile& biome,
                      const struct StrataModulation& mod)
    {
        if (!active)
        {
            std::fill(outL, outL + numSamples, 0.0f);
            std::fill(outR, outR + numSamples, 0.0f);
            return 0.0f;
        }

        // Formant frequencies — computed once per block (slow-moving parameters)
        float f1, f2, f3;
        computeFormantFreqs(snap, biome, mod, f1, f2, f3);

        // Q from spread: low spread = narrow formants (more resonant, focused)
        float q = 3.0f + (1.0f - snap.creatureSpread) * 22.0f; // 3–25

        // Call period: creatureRate 0→1 maps 4.0s → 0.1s
        float rateClamped    = std::clamp(snap.creatureRate, 0.01f, 1.0f);
        float callPeriodSamp = sr * (4.0f - rateClamped * 3.9f);

        // Envelope timing (in samples)
        float attackMs  = 1.0f + snap.creatureAttack * 49.0f;  // 1–50 ms
        float decayAmt  = std::clamp(snap.creatureDecay + biome.emergentDecayBase, 0.0f, 1.0f);
        float decayMs   = 50.0f + decayAmt * 1950.0f;          // 50–2000 ms
        float attackSamp = attackMs  * 0.001f * sr;
        float decaySamp  = decayMs   * 0.001f * sr;
        float totalSamp  = attackSamp + decaySamp;

        // Block-level trigger source selection
        int trigSrc = snap.creatureTrigger; // 0=midi, 1=floor_amp, 2=canopy_peaks

        // External trigger (only active when source is floor_amp or canopy_peaks)
        if (trigSrc >= 1 && mod.emergentTriggerMod > 0.6f && callEnvSample < 0.0f)
        {
            callEnvSample = 0.0f;
            callTimer     = 0.0f;
        }

        float sumSq      = 0.0f;
        float callsFired = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            // Auto-trigger timer
            callTimer += 1.0f;
            if (callTimer >= callPeriodSamp)
            {
                callTimer    = 0.0f;
                callEnvSample = 0.0f;
                callsFired   += 1.0f;
            }

            // Compute call envelope amplitude
            float envAmp = 0.0f;
            if (callEnvSample >= 0.0f)
            {
                if (callEnvSample < attackSamp)
                {
                    envAmp = callEnvSample / attackSamp;
                }
                else
                {
                    float decayPos = (callEnvSample - attackSamp) / decaySamp;
                    envAmp = std::exp(-5.0f * decayPos);
                    if (envAmp < 0.0001f)
                        callEnvSample = -1.0f; // call finished
                }
                if (callEnvSample >= 0.0f)
                    callEnvSample += 1.0f;
            }

            // Noise excitation via LCG (fast, deterministic, no allocation)
            noiseSeed = noiseSeed * 1664525u + 1013904223u;
            float noise = static_cast<float>(static_cast<int32_t>(noiseSeed))
                          * 4.656612e-10f; // / 2^31

            // Pitch sweep: formants glide up during attack, relax during decay
            float envPhaseNorm = (callEnvSample >= 0.0f && totalSamp > 0.0f)
                                 ? std::clamp(callEnvSample / totalSamp, 0.0f, 1.0f)
                                 : 0.0f;
            float sweep    = std::sin(envPhaseNorm * juce::MathConstants<float>::pi)
                             * biome.emergentPitchRange;
            float sweepMul = std::pow(2.0f, sweep);

            // 3 SVF bandpass formant filters in parallel
            float sample = formants[0].process(noise, f1 * sweepMul, q, sr)
                         + formants[1].process(noise, f2 * sweepMul, q * 1.2f, sr)
                         + formants[2].process(noise, f3 * sweepMul, q * 0.8f, sr);
            sample       *= (1.0f / kNumFormants);

            // Apply envelope, level, velocity
            sample *= envAmp * snap.creatureLevel * baseVelocity;

            outL[i] = sample;
            outR[i] = sample * (1.0f + snap.creatureSpread * 0.08f); // subtle stereo spread
            sumSq  += sample * sample;
        }

        lastAmplitude = std::sqrt(sumSq / static_cast<float>(numSamples));
        lastPattern   = std::clamp(callsFired * 4.0f, 0.0f, 1.0f); // calls/block → density
        return lastAmplitude;
    }

    bool  isActive()          const { return active; }
    float getLastAmplitude()  const { return lastAmplitude; }
    float getLastPattern()    const { return lastPattern; }

private:
    // Analog-style state-variable filter (EDP topology).
    // Bandpass output selected; tanh on BP prevents runaway during high Q.
    struct FormantFilter
    {
        float bp = 0.0f, lp = 0.0f;

        float process(float x, float freq, float q, float sr)
        {
            // freq coefficient: approximate, clamp well below Nyquist
            float f = std::min(1.99f,
                               2.0f * std::sin(juce::MathConstants<float>::pi
                                               * std::min(freq, sr * 0.45f) / sr));
            float qInv = 1.0f / std::max(0.5f, q);
            float h    = x - lp - qInv * bp;
            bp         = f * h + bp;
            lp         = f * bp + lp;
            // Soft-saturate BP output to prevent numerical blowup at high Q
            bp         = std::tanh(bp);
            return bp;
        }

        void reset() { bp = lp = 0.0f; }
    };

    // Formant frequency table [Hz]: { F1, F2, F3 } per creature type
    // Types: 0=Bird  1=Whale  2=Insect  3=Frog  4=Wolf  5=Synth
    static constexpr float kFormantTable[6][3] =
    {
        {  800.f,  2500.f,  5000.f },  // Bird
        {   80.f,   220.f,   540.f },  // Whale
        { 1800.f,  3800.f,  7600.f },  // Insect
        {  350.f,  1000.f,  2200.f },  // Frog
        {  260.f,   720.f,  1600.f },  // Wolf
        {  500.f,  1500.f,  4000.f },  // Synth (user-controlled via pitch + spread)
    };

    void computeFormantFreqs(const OcelotParamSnapshot& snap,
                             const BiomeProfile& biome,
                             const struct StrataModulation& mod,
                             float& f1, float& f2, float& f3) const
    {
        int t = std::clamp(snap.creatureType, 0, 5);
        f1 = kFormantTable[t][0];
        f2 = kFormantTable[t][1];
        f3 = kFormantTable[t][2];

        // MIDI note transposition from baseNote (C3=60 = concert pitch)
        float noteSemitones = static_cast<float>(baseNote - 60);
        float noteMul       = std::pow(2.0f, noteSemitones / 12.0f);

        // User creaturePitch: -1..+1 (centered at 0.5) → ±1 octave range
        float pitchUser  = (snap.creaturePitch - 0.5f) * 2.0f;
        float pitchMod   = std::clamp(pitchUser + mod.emergentPitchMod, -1.0f, 1.0f);
        float pitchMul   = std::pow(2.0f, pitchMod * 0.5f); // ±0.5 oct from user pitch

        // Formant additive mod from Canopy→Emergent (spectral route)
        float formantMod = mod.emergentFormantMod;
        float formantMul = std::pow(2.0f, formantMod * 0.3f);

        // Biome: emergentPitchRange encodes the "creature character scale"
        // 0.35 (Winter/wolf) → lower formants; 0.8 (Underwater/whale) → much lower
        float biomeTilt = (biome.emergentPitchRange - 0.5f);     // -0.15 to +0.30
        float biomeMul  = std::pow(2.0f, -biomeTilt * 1.2f);     // Underwater = lower

        float mul = noteMul * pitchMul * formantMul * biomeMul;
        f1 = std::clamp(f1 * mul, 40.0f, 16000.0f);
        f2 = std::clamp(f2 * mul, 80.0f, 18000.0f);
        f3 = std::clamp(f3 * mul, 120.0f, 20000.0f);
    }

    std::array<FormantFilter, kNumFormants> formants;
    float    sr             = 44100.0f;
    float    callEnvSample  = -1.0f;   // -1 = idle; >=0 = position in current call
    float    callTimer      = 0.0f;
    uint32_t noiseSeed      = 0xDEADBEEF;
    float    lastAmplitude  = 0.0f;
    float    lastPattern    = 0.0f;
    bool     active         = false;
    int      baseNote       = 60;
    float    baseVelocity   = 0.8f;
};

} // namespace xocelot
