// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "OwlfishParamSnapshot.h"
#include "OwlfishAmpEnvelope.h"
#include "SubharmonicOsc.h"
#include "OwlfishCompressor.h"
#include "OwlfishCytomicSVF.h"
#include "MicroGranular.h"
#include "ArmorBuffer.h"
#include "AbyssReverb.h"
#include "OwlfishFastMath.h"
#include <cmath>

namespace xowlfish {

//==============================================================================
// OwlfishVoice -- The organism. Monophonic voice wiring 5 organs into one body.
//
// Signal flow:
//   1. SOLITARY GENUS:  Portamento / legato / morph glide
//   2. ABYSS HABITAT:   SubharmonicOsc (Mixtur-Trautonium oscillator)
//   3. OWL OPTICS:      OwlfishCompressor -> CytomicSVF (resonant LP filter)
//   4. DIET:            MicroGranular (2-10ms predatory grains)
//   5. SACRIFICIAL ARMOR: ArmorBuffer (velocity-triggered grain burst + ducking)
//   6. AMP ENVELOPE:    ADSR amplitude shaping
//   7. ABYSS REVERB:    AbyssReverb (dark FDN reverb)
//   8. OUTPUT:          Level + pan
//
// Coupling cache: lastOutL/R available via getLastSampleL/R() for XOceanus
// cross-engine modulation.
//
// All DSP inline. No allocations in process(). Denormal-safe.
//==============================================================================

class OwlfishVoice
{
public:
    OwlfishVoice() = default;

    //--------------------------------------------------------------------------
    /// Prepare all organs for playback at the given sample rate.
    void prepare (double sampleRate)
    {
        this->sampleRate = sampleRate;

        abyssOsc.prepare (sampleRate);
        optics.prepare (sampleRate);
        opticsFilter.reset();
        diet.prepare (sampleRate);
        armor.prepare (sampleRate);
        reverb.prepare (sampleRate);
        ampEnv.prepare (sampleRate);

        currentFreq  = 440.0f;
        targetFreq   = 440.0f;
        baseNoteFreq = 440.0f;
        portaCoeff   = 0.0f;
        currentNote  = -1;
        lastVelocity = 0.0f;
        lastOutL     = 0.0f;
        lastOutR     = 0.0f;
    }

    //--------------------------------------------------------------------------
    /// Trigger a note. Handles legato vs retrigger modes.
    void noteOn (int note, float velocity, const OwlfishParamSnapshot& snap)
    {
        baseNoteFreq = midiToFreq (note);
        targetFreq   = baseNoteFreq;
        lastVelocity = velocity;

        // Trigger armor check (velocity vs threshold)
        armor.trigger (velocity, snap.armorThreshold);

        if (currentNote < 0 || snap.legatoMode == 0)
        {
            // First note or retrigger mode: start from scratch
            currentFreq = targetFreq;
            ampEnv.noteOn();
        }
        // Else: legato mode, already playing -- just change target freq (glide only)

        currentNote = note;
    }

    //--------------------------------------------------------------------------
    /// Apply pitch bend ratio to the target frequency (called per block from engine).
    void applyPitchBend (float ratio) noexcept
    {
        targetFreq = baseNoteFreq * ratio;
    }

    //--------------------------------------------------------------------------
    /// Release the current note.
    void noteOff()
    {
        ampEnv.noteOff();
        // currentNote stays set so isActive() works until envelope finishes
    }

    //--------------------------------------------------------------------------
    /// Is the voice still producing output?
    bool isActive() const
    {
        return ampEnv.isActive();
    }

    //--------------------------------------------------------------------------
    /// Process a block of samples through the full organism signal chain.
    /// Writes interleaved stereo output to outL/outR buffers.
    void process (float* outL, float* outR, int numSamples,
                  const OwlfishParamSnapshot& snap)
    {
        // ================================================================
        // Per-block parameter updates (read once, not per-sample)
        // ================================================================

        // -- Portamento coefficient --
        float portaMs = snap.portamento * 2000.0f;  // 0-1 -> 0-2000ms
        if (portaMs < 0.5f)
        {
            portaCoeff = 0.0f;  // instant
        }
        else
        {
            portaCoeff = std::exp (-1.0f / (portaMs * 0.001f * static_cast<float> (sampleRate)));
        }

        // D004 fix: morphGlide modulates mixtur (waveshaper blend) during portamento.
        // Compute block-rate glide progress from current pitch distance to target.
        // glideProgress ~0 = mid-glide, ~1 = arrived at target pitch.
        // morphGlide=0: no modulation. morphGlide=1: mixtur sweeps up to +0.5 mid-glide,
        // creating a timbral swell as the organism glides to its new pitch.
        float blockGlideProgress = (portaCoeff > 0.0f && targetFreq > 1.0f)
            ? std::max (0.0f, std::min (1.0f,
                1.0f - std::fabs (currentFreq - targetFreq) / targetFreq))
            : 1.0f;
        float glideModMixtur = snap.mixtur + snap.morphGlide * 0.5f * (1.0f - blockGlideProgress);
        glideModMixtur = std::max (0.0f, std::min (1.0f, glideModMixtur));

        // -- Abyss Habitat (oscillator) params --
        abyssOsc.setParams (snap.subDiv1, snap.subDiv2, snap.subDiv3, snap.subDiv4,
                            snap.subLevel1, snap.subLevel2, snap.subLevel3, snap.subLevel4,
                            snap.subMix, glideModMixtur, snap.fundWave, snap.subWave,
                            snap.bodyFreq, snap.bodyLevel);

        // -- Owl Optics (compressor) params --
        optics.setParams (snap.compRatio, snap.compThreshold, snap.compAttack, snap.compRelease);

        // D005 fix: minimal LFO added — advance grain size LFO (0.05 Hz, ±12%)
        grainLfoPhase += (0.05f * juce::MathConstants<float>::twoPi) / static_cast<float>(sampleRate);
        if (grainLfoPhase >= juce::MathConstants<float>::twoPi) grainLfoPhase -= juce::MathConstants<float>::twoPi;
        const float lfoGrainSize = snap.grainSize * (1.0f + 0.12f * std::sin(grainLfoPhase));

        // -- Diet (micro-granular) params --
        diet.setParams (lfoGrainSize, snap.grainDensity, snap.grainPitch, snap.feedRate);

        // -- Sacrificial Armor params --
        armor.setParams (snap.armorDecay, snap.armorScatter, snap.armorDelay);

        // -- Abyss Reverb params --
        reverb.setParams (snap.reverbSize, snap.reverbDamp, snap.reverbPreDelay);

        // -- Amp Envelope params (in milliseconds) --
        ampEnv.setParams (snap.ampAttack, snap.ampDecay, snap.ampSustain, snap.ampRelease);

        // -- Filter setup --
        // Cutoff mapping: 0-1 -> 20Hz-20kHz (logarithmic via pow2)
        float cutoffHz = 20.0f * std::pow (2.0f, snap.filterCutoff * 10.0f);
        // Add key tracking: offset by fundamental frequency * tracking amount
        if (currentNote >= 0)
            cutoffHz += midiToFreq (currentNote) * snap.filterTrack;
        // D001: filter envelope depth — amp envelope level × velocity sweeps cutoff.
        // kOwlFilterEnvMaxHz = 5500 Hz: maps the 0-1 normalized cutoff parameter space.
        // At default depth 0.25, full velocity at attack peak adds +1375 Hz of brightness.
        static constexpr float kOwlFilterEnvMaxHz = 5500.0f;
        cutoffHz += snap.filterEnvDepth * ampEnv.getLevel() * lastVelocity * kOwlFilterEnvMaxHz;
        cutoffHz = std::max (20.0f, std::min (20000.0f, cutoffHz));
        opticsFilter.setCoefficients (cutoffHz, snap.filterReso,
                                      static_cast<float> (sampleRate));

        // -- Output gain + pan --
        float level = snap.outputLevel;
        float panL = (snap.outputPan <= 0.0f) ? 1.0f : (1.0f - snap.outputPan);
        float panR = (snap.outputPan >= 0.0f) ? 1.0f : (1.0f + snap.outputPan);

        // ================================================================
        // Per-sample processing loop -- the organism body
        // ================================================================
        for (int i = 0; i < numSamples; ++i)
        {
            // ---- 1. SOLITARY GENUS: Portamento glide ----
            currentFreq = targetFreq + (currentFreq - targetFreq) * portaCoeff;
            currentFreq = flushDenormal (currentFreq);

            // ---- 2. ABYSS HABITAT: Subharmonic oscillator ----
            float osc = abyssOsc.processSample (currentFreq);

            // ---- 3. OWL OPTICS: Compressor + resonant filter ----
            float compressed = optics.processSample (osc);
            float filtered   = opticsFilter.processSample (compressed);

            // ---- 4. DIET: Micro-granular processing ----
            diet.writeSample (filtered);
            float grained = diet.readSample();
            float mixed = filtered * (1.0f - snap.grainMix) + grained * snap.grainMix;

            // ---- 5. SACRIFICIAL ARMOR: Burst + ducking ----
            armor.feedInput (mixed);
            float armorOut   = armor.processSample();
            float duckAmount = armor.getDuckAmount();
            mixed *= (1.0f - duckAmount * snap.armorDuck);
            mixed += armorOut;

            // ---- 6. AMP ENVELOPE ----
            mixed *= ampEnv.processSample();

            // ---- 7. ABYSS REVERB ----
            float revL = 0.0f, revR = 0.0f;
            reverb.processSample (mixed, mixed, revL, revR);
            float finalL = mixed * (1.0f - snap.reverbMix) + revL * snap.reverbMix;
            float finalR = mixed * (1.0f - snap.reverbMix) + revR * snap.reverbMix;

            // ---- 8. OUTPUT: Level + pan ----
            outL[i] = finalL * level * panL;
            outR[i] = finalR * level * panR;
        }

        // ---- Cache last output for coupling ----
        if (numSamples > 0)
        {
            lastOutL = outL[numSamples - 1];
            lastOutR = outR[numSamples - 1];
        }
    }

    //--------------------------------------------------------------------------
    /// Get last output sample (left) for XOceanus coupling cache.
    float getLastSampleL() const { return lastOutL; }

    /// Get last output sample (right) for XOceanus coupling cache.
    float getLastSampleR() const { return lastOutR; }

private:
    // ---- SOLITARY GENUS state ----
    float currentFreq  = 440.0f;
    float targetFreq   = 440.0f;
    float baseNoteFreq = 440.0f;  // un-bent note frequency; pitch bend multiplies this
    float portaCoeff   = 0.0f;   // exponential glide coefficient (0 = instant)
    int   currentNote  = -1;
    float lastVelocity = 0.0f;

    // ---- DSP modules (the organs) ----
    SubharmonicOsc    abyssOsc;         // ABYSS HABITAT
    OwlfishCompressor optics;           // OWL OPTICS (compressor)
    CytomicSVF        opticsFilter;     // OWL OPTICS (resonant LP filter)
    MicroGranular     diet;             // DIET
    ArmorBuffer       armor;            // SACRIFICIAL ARMOR
    AbyssReverb       reverb;           // ABYSS REVERB
    AmpEnvelope       ampEnv;           // Amp envelope

    double sampleRate = 44100.0;
    float  lastOutL   = 0.0f;
    float  lastOutR   = 0.0f;

    // D005 fix: minimal LFO added — grain size breathing at 0.05 Hz
    float grainLfoPhase = 0.0f;
};

} // namespace xowlfish
