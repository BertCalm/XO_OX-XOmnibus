// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OfferingEngine.h — XOffering | "The Crate Digger"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOffering is the Mantis Shrimp — a hyper-perceptive crustacean living
//      in the Rubble Zone (5-15m depth), seeing colors no other creature can
//      perceive. Just as the mantis shrimp processes visual information through
//      16 photoreceptor types (vs. 3 in humans), XOffering processes drum
//      sounds through the lens of psychological research — perceiving timbral
//      dimensions that simpler engines cannot.
//
//  ENGINE CONCEPT:
//      Psychology-driven boom bap drum synthesis. Living drums that dig through
//      imaginary crates, filtered through the psychoacoustic DNA of hip hop's
//      global cities. The drums are generated, not played back. The curiosity
//      engine uses published research (Berlyne, Wundt, Csikszentmihalyi) to
//      create aesthetic judgment without AI.
//
//  SIGNAL FLOW:
//      MIDI Trigger → Transient Generator (per-type topology)
//                   → Texture Layer (vinyl/tape/bit/wobble — DUST)
//                   → Collage Engine (layers/chop/stretch/ring — FLIP)
//                   → City Processing Chain (5 cities — CITY)
//                   → Voice Mixer (8 voices → stereo)
//                   → Curiosity Engine (Berlyne/Wundt/Flow — DIG)
//                   → Master Character (tape sat + output)
//                   → getSampleForCoupling()
//
//  84 parameters: 36 global + 48 per-voice (6 × 8)
//  4 macros: DIG (M1), CITY (M2), FLIP (M3), DUST (M4)
//
//  Accent: Crate Wax Yellow #E5B80B | Prefix: ofr_
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/ParameterSmoother.h"
#include "OfferingTransient.h"
#include "OfferingTexture.h"
#include "OfferingCollage.h"
#include "OfferingCity.h"
#include "OfferingCuriosity.h"
#include <array>
#include <cmath>
#include <cstring>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
// Voice slot MIDI note mapping — GM drum map standard positions.
//==============================================================================
static constexpr int kOfferingVoiceNotes[8] = {
    36, // C2  — Kick
    38, // D2  — Snare
    42, // F#2 — Closed Hat
    46, // A#2 — Open Hat
    39, // D#2 — Clap
    37, // C#2 — Rim
    45, // A2  — Tom
    49  // C#3 — Perc (crash position, used for perc)
};

//==============================================================================
// OfferingVoice — Single drum voice: transient → texture → collage → city.
//==============================================================================
struct OfferingVoice
{
    OfferingTransient transient;
    OfferingTexture texture;
    OfferingCollage collage;

    float lastSample = 0.0f;
    bool active = false;

    // Detroit drunk timing: per-voice trigger delay counter (NOT buffer delay).
    // When Detroit city is active, voice start is displaced by ±15ms random offset.
    // This models Dilla's sequencer-level timing, not audio-path delay.
    int drunkDelaySamples = 0;
    bool pendingTrigger = false;
    // Cached trigger params for delayed start
    int pendType = 0;
    float pendVelocity = 0.0f;
    float pendTune = 0.0f;
    float pendDecay = 0.3f;
    float pendBody = 0.5f;
    float pendSnap = 0.5f;
    float pendPitchEnv = 0.3f;
    float pendSat = 0.15f;
    int pendLayers = 1;
    float pendChop = 0.0f;
    float pendStretch = 1.0f;
    float pendRingMod = 0.0f;

    void prepare(float sampleRate) noexcept
    {
        transient.prepare(sampleRate);
        texture.prepare(sampleRate);
        collage.prepare(sampleRate);
    }

    void reset() noexcept
    {
        transient.reset();
        texture.reset();
        collage.reset();
        lastSample = 0.0f;
        active = false;
        drunkDelaySamples = 0;
        pendingTrigger = false;
    }
};

//==============================================================================
// ParamSnapshot — cache all 84 parameters once per processBlock.
//==============================================================================
struct OfferingParamSnapshot
{
    // Transient Generator (3)
    float transientSnap = 0.5f;
    float transientPitch = 0.3f;
    float transientSat = 0.15f;

    // Texture / DUST (5)
    float dustVinyl = 0.2f;
    float dustTape = 0.1f;
    int dustBits = 16;
    float dustSampleRate = 48000.0f; // Loaded from param; default matches param default
    float dustWobble  = 0.05f;

    // Collage / FLIP (4)
    int flipLayers = 1;
    float flipChop = 0.0f;
    float flipStretch = 1.0f;
    float flipRingMod = 0.0f;

    // City (3)
    int cityMode = 0;
    float cityBlend = 0.0f;
    float cityIntensity = 0.5f;

    // Curiosity / DIG (3)
    float digCuriosity = 0.5f;
    float digComplexity = 0.4f;
    float digFlow = 0.6f;

    // Expression & Modulation (12)
    float velToSnap = 0.5f;
    float velToBody = 0.3f;
    float lfo1Rate = 0.067f;
    float lfo1Depth = 0.05f;
    int lfo1Shape = 0;
    float lfo2Rate = 2.0f;
    float lfo2Depth = 0.0f;
    float aftertouch = 0.3f;
    float modWheel = 0.5f;
    float envFilterAmt = 0.5f;
    float velToAttack = 0.2f;
    float envToPitch = 0.0f;

    // Master (2)
    float masterLevel = 0.75f;
    float masterWidth = 0.5f;

    // Per-voice (6 × 8 = 48)
    struct VoiceSnap
    {
        int type = 0;
        float tune = 0.0f;
        float decay = 0.3f;
        float body = 0.5f;
        float level = 0.8f;
        float pan = 0.0f;
    } voice[8];
};

//==============================================================================
// OfferingEngine — Master engine implementing SynthEngine interface.
//==============================================================================
class OfferingEngine : public SynthEngine
{
public:
    OfferingEngine() = default;

    //-- Identity ---------------------------------------------------------------

    juce::String getEngineId() const override { return "Offering"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFE5B80B); }
    int getMaxVoices() const override { return 8; }

    int getActiveVoiceCount() const override
    {
        int count = 0;
        for (int i = 0; i < 8; ++i)
            if (voices_[i].active)
                count++;
        return count;
    }

    //-- Lifecycle ---------------------------------------------------------------

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr_ = static_cast<float>(sampleRate);
        blockSize_ = maxBlockSize;

        for (int i = 0; i < 8; ++i)
        {
            voices_[i].prepare(sr_);
            // Seed each voice's texture noise differently so crackle/hiss patterns
            // are decorrelated across pads. All voices previously used seed 54321,
            // producing identical vinyl/tape textures on every drum hit.
            voices_[i].texture.reseedNoise(static_cast<uint32_t>(54321 + i * 7919));
        }

        cityProcessor_.prepare(sr_);
        curiosity_.reset();
        lfo1_.setShape(StandardLFO::Sine);
        lfo2_.setShape(StandardLFO::Sine);

        // SilenceGate: percussive engine, 100ms hold
        prepareSilenceGate(sampleRate, maxBlockSize, 100.0f);

        couplingCacheL_ = 0.0f;
        couplingCacheR_ = 0.0f;
    }

    void releaseResources() override {}

    void reset() override
    {
        for (int i = 0; i < 8; ++i)
            voices_[i].reset();
        cityProcessor_.reset();
        curiosity_.reset();
        couplingCacheL_ = 0.0f;
        couplingCacheR_ = 0.0f;
        pitchBendNorm_ = 0.0f;
        aftertouchValue_ = 0.0f;
        modWheelValue_ = 0.0f;
        // Re-seed drunk timing PRNG so playback is repeatable on reset.
        static constexpr uint32_t kDrunkSeeds[8] = { 7919, 15839, 23759, 31679, 39599, 47519, 55439, 63359 };
        for (int i = 0; i < 8; ++i)
            drunkRngState_[i] = kDrunkSeeds[i];
    }

    //-- Audio -------------------------------------------------------------------

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        // ── 1. Parse MIDI ──────────────────────────────────────────────
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();

            if (msg.isNoteOn())
            {
                wakeSilenceGate();
                int voiceIdx = noteToVoice(msg.getNoteNumber());
                if (voiceIdx < 0)
                    continue;

                triggerVoice(voiceIdx, msg.getFloatVelocity());
            }
            else if (msg.isNoteOff())
            {
                // Drums don't really "release" — they're AD envelopes
            }
            else if (msg.isPitchWheel())
            {
                pitchBendNorm_ = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
            }
            else if (msg.isChannelPressure())
            {
                aftertouchValue_ = msg.getChannelPressureValue() / 127.0f;
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                modWheelValue_ = msg.getControllerValue() / 127.0f;
            }
        }

        // ── 2. SilenceGate bypass ──────────────────────────────────────
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            couplingCacheL_ = couplingCacheR_ = 0.0f;
            // Reset coupling accumulators on bypass path too — coupling inputs
            // may still arrive even when engine is silent; discard them cleanly.
            couplingCityMod_ = 0.0f;
            couplingChokeMod_ = 0.0f;
            couplingFlipMod_ = 0.0f;
            couplingDecayMod_ = 0.0f;
            couplingFMMod_ = 0.0f;
            return;
        }

        // ── 3. Cache parameters (ParamSnapshot) ──────────────────────
        OfferingParamSnapshot snap;
        loadSnapshot(snap);

        // ── 3b. Wire coupling modulators into snapshot ────────────────
        // couplingCityMod_: external amplitude modulates city processing intensity
        snap.cityIntensity = std::clamp(snap.cityIntensity + couplingCityMod_, 0.0f, 1.0f);
        // couplingFlipMod_: external rhythm modulates stretch amount (additive)
        snap.flipStretch = std::clamp(snap.flipStretch + couplingFlipMod_, 0.5f, 2.0f);
        // couplingFMMod_ — wired in triggerVoice(): deepens pitchEnvDepth on the next triggered hit

        // ── 4. Apply macros ──────────────────────────────────────────
        float macroDig = loadParam(paramMacroDig_, 0.0f);
        float macroCity = loadParam(paramMacroCity_, 0.0f);
        float macroFlip = loadParam(paramMacroFlip_, 0.0f);
        float macroDust = loadParam(paramMacroDust_, 0.0f);

        // M1: DIG → curiosity + complexity at 50%
        snap.digCuriosity = std::clamp(snap.digCuriosity + macroDig, 0.0f, 1.0f);
        snap.digComplexity = std::clamp(snap.digComplexity + macroDig * 0.5f, 0.0f, 1.0f);

        // M2: CITY → city mode morph
        float cityMacroVal = macroCity;
        if (std::abs(cityMacroVal) > 0.01f)
        {
            float totalCity = static_cast<float>(snap.cityMode) + cityMacroVal * 4.0f;
            snap.cityMode = std::clamp(static_cast<int>(totalCity), 0, 4);
            snap.cityBlend = totalCity - std::floor(totalCity);
        }

        // M3: FLIP → layers + chop + ring mod
        snap.flipLayers = std::clamp(snap.flipLayers + static_cast<int>(macroFlip * 3.0f), 1, 4);
        snap.flipChop = std::clamp(snap.flipChop + macroFlip * 0.5f, 0.0f, 1.0f);
        snap.flipRingMod = std::clamp(snap.flipRingMod + macroFlip * 0.3f, 0.0f, 1.0f);

        // M4: DUST → vinyl + tape + bits
        snap.dustVinyl = std::clamp(snap.dustVinyl + macroDust * 0.5f, 0.0f, 1.0f);
        snap.dustTape = std::clamp(snap.dustTape + macroDust * 0.3f, 0.0f, 1.0f);
        if (macroDust > 0.5f)
            snap.dustBits = std::max(4, snap.dustBits - static_cast<int>((macroDust - 0.5f) * 8.0f));

        // D006: mod wheel → curiosity drive
        snap.digCuriosity = std::clamp(snap.digCuriosity + modWheelValue_ * snap.modWheel * 0.3f, 0.0f, 1.0f);

        // D006: aftertouch → texture intensity
        float atMod = aftertouchValue_ * snap.aftertouch;

        // ── 5. Update LFOs ──────────────────────────────────────────
        lfo1_.setRate(snap.lfo1Rate, sr_);
        lfo1_.setShape(snap.lfo1Shape);
        lfo2_.setRate(snap.lfo2Rate, sr_);
        lfo2_.setShape(StandardLFO::Sine); // LFO2 is always sine for groove pump

        // ── 6. Render voices ─────────────────────────────────────────
        // ADDITIVE: do not clear — engine adds to existing buffer (slot chain convention).
        // Render Offering into scratch arrays first, then mix into the output buffer.
        float* bufL = buffer.getWritePointer(0);
        float* bufR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : bufL;

        // Per-sample scratch buffers for this engine's render + FX chain.
        // 4096 samples handles 96kHz at large buffer sizes (~42ms); guarded by safeSamples.
        // NOTE: OfferingCityProcessor::process() allocates a shadow[2048] for blend mode,
        // so the effective city-blend limit is 2048 samples. safeSamples is capped there too.
        float scrL[4096];
        float scrR[4096];
        // Per-sample mono mix buffer for city processing.
        float monoMix[4096];
        jassert(numSamples <= 4096); // warn in debug if host sends unexpectedly large blocks
        int safeSamples = std::min(numSamples, 4096);
        std::fill(scrL, scrL + safeSamples, 0.0f);
        std::fill(scrR, scrR + safeSamples, 0.0f);
        std::fill(monoMix, monoMix + safeSamples, 0.0f);
        // Alias for readability — rest of code uses outL/outR unchanged
        float* outL = scrL;
        float* outR = scrR;

        // SRO (2026-03-21): Precompute per-voice pan gains once per block.
        // vs.pan comes from param snapshot — block-constant. Moving cos/sin
        // outside the sample loop eliminates 16 trig calls/sample at 8-voice load.
        float precomputedPanL[8], precomputedPanR[8];
        for (int v = 0; v < 8; ++v)
        {
            float angle = (snap.voice[v].pan + 1.0f) * 0.25f * 3.14159265f;
            precomputedPanL[v] = fastCos(angle);
            precomputedPanR[v] = fastSin(angle);
        }

        for (int s = 0; s < safeSamples; ++s)
        {
            float lfo1Val = lfo1_.process();
            float lfo2Val = lfo2_.process();

            float sampleL = 0.0f;
            float sampleR = 0.0f;

            for (int v = 0; v < 8; ++v)
            {
                if (!voices_[v].active && !voices_[v].transient.isActive())
                    continue;

                // Detroit drunk timing: countdown pending trigger delay
                if (voices_[v].pendingTrigger)
                {
                    if (--voices_[v].drunkDelaySamples <= 0)
                    {
                        voices_[v].pendingTrigger = false;
                        voices_[v].transient.trigger(voices_[v].pendType, voices_[v].pendVelocity, voices_[v].pendTune,
                                                     voices_[v].pendDecay, voices_[v].pendBody, voices_[v].pendSnap,
                                                     voices_[v].pendPitchEnv, voices_[v].pendSat);
                        voices_[v].collage.trigger(voices_[v].pendLayers, voices_[v].pendChop, voices_[v].pendStretch,
                                                   voices_[v].pendRingMod);
                    }
                    continue; // silence during drunk delay countdown
                }

                auto& vs = snap.voice[v];

                // envToPitch: modulate transient frequency by the running amp-envelope level.
                // Bipolar: positive = pitch-up sweep (rising drums), negative = pitch-down
                // sweep (natural drum behaviour — kick starts high, falls to baseFreq).
                // At |envToPitch|=1.0 and env=1.0 the ratio is 2.0 (one octave shift).
                // Applied before process() so the frequency change is sampled this cycle.
                // Always set (even to 1.0) so stale ratios never persist across blocks.
                {
                    float envLevel = voices_[v].transient.getAmpEnvLevel();
                    // Use != 0 check (CLAUDE.md: bipolar modulation must use != 0)
                    float freqRatio = snap.envToPitch != 0.0f
                                          ? (1.0f + envLevel * snap.envToPitch * 2.0f)
                                          : 1.0f;
                    // Clamp to valid multiplier range (avoid negative/zero frequency)
                    freqRatio = std::max(0.1f, freqRatio);
                    voices_[v].transient.setFreqMod(freqRatio);
                }

                // Transient synthesis
                float sample = voices_[v].transient.process();
                if (!voices_[v].transient.isActive())
                {
                    voices_[v].active = false;
                    continue;
                }

                // Texture layer (DUST + aftertouch modulation + envFilterAmt)
                // envFilterAmt: envelope→filter — voice amplitude modulates texture intensity
                float envMod = std::abs(sample) * snap.envFilterAmt;
                float effectiveVinyl = snap.dustVinyl + atMod * 0.2f + envMod * 0.15f;
                float effectiveTape = snap.dustTape + atMod * 0.15f + envMod * 0.1f;
                sample = voices_[v].texture.process(sample, effectiveVinyl, effectiveTape, snap.dustBits,
                                                    snap.dustSampleRate, snap.dustWobble);

                // Collage layer
                sample = voices_[v].collage.process(sample);

                // LFO1 → filter modulation (applied as amplitude mod for simplicity in V1)
                if (snap.lfo1Depth > 0.001f)
                    sample *= 1.0f + lfo1Val * snap.lfo1Depth * 0.5f;

                // LFO2 → amplitude modulation (groove pump)
                if (snap.lfo2Depth > 0.001f)
                    sample *= 1.0f + lfo2Val * snap.lfo2Depth * 0.5f;

                // Apply voice level
                // couplingChokeMod_: external amplitude ducks (sidechains) drum voices
                float chokeGain = std::clamp(1.0f - couplingChokeMod_, 0.0f, 1.0f);
                sample *= vs.level * chokeGain;

                // Stereo panning (equal-power) — SRO: use block-precomputed values
                sampleL += sample * precomputedPanL[v];
                sampleR += sample * precomputedPanR[v];

                voices_[v].lastSample = sample;
            }

            monoMix[s] = (sampleL + sampleR) * 0.5f;
            outL[s] = sampleL;
            outR[s] = sampleR;
        }

        // ── 7. City processing (mono → applied to both channels) ──────
        // City chain processes a mono mix. To preserve stereo width at full
        // intensity, we apply the city's GAIN CHARACTER (ratio of processed/dry)
        // to each stereo channel independently rather than replacing with mono.
        if (snap.cityIntensity > 0.001f)
        {
            // Save pre-city mono levels for gain-ratio computation
            float preCityMono[4096];
            for (int s = 0; s < safeSamples; ++s)
                preCityMono[s] = monoMix[s];

            cityProcessor_.process(monoMix, safeSamples, snap.cityMode, snap.cityBlend, snap.cityIntensity);

            // Apply city processing as stereo-preserving gain multiplier.
            // The ratio cityMono/preCity captures compression, saturation, and
            // filtering character; multiplying each stereo channel by the same
            // ratio retains the original stereo image at all intensities.
            for (int s = 0; s < safeSamples; ++s)
            {
                float cityMono = monoMix[s];
                float wet = snap.cityIntensity;

                // Compute city gain ratio safely (avoid divide-by-zero on silence).
                float denom = preCityMono[s];
                float cityGain = (std::abs(denom) > 1e-9f) ? (cityMono / denom) : 1.0f;
                cityGain = std::clamp(cityGain, -4.0f, 4.0f); // guard against extreme ratios

                // Blend: dry (1-wet) + city-processed-at-original-stereo-ratio * wet
                outL[s] = outL[s] * (1.0f - wet) + outL[s] * cityGain * wet;
                outR[s] = outR[s] * (1.0f - wet) + outR[s] * cityGain * wet;
            }
        }

        // ── 8. Master character (tape sat + width + level) ────────────
        for (int s = 0; s < safeSamples; ++s)
        {
            // Subtle master tape saturation
            outL[s] = fastTanh(outL[s] * 1.1f);
            outR[s] = fastTanh(outR[s] * 1.1f);

            // Stereo width (mid-side)
            float mid = (outL[s] + outR[s]) * 0.5f;
            float side = (outL[s] - outR[s]) * 0.5f;
            side *= snap.masterWidth * 2.0f; // 0=mono, 0.5=normal, 1=wide
            outL[s] = (mid + side) * snap.masterLevel;
            outR[s] = (mid - side) * snap.masterLevel;

            // Coupling cache: store last sample for getSampleForCoupling
            couplingCacheL_ = outL[s];
            couplingCacheR_ = outR[s];
        }

        // ── 9. Additive mix: combine Offering's processed signal into the output buffer ──
        for (int s = 0; s < safeSamples; ++s)
        {
            bufL[s] += outL[s];
            bufR[s] += outR[s];
        }

        // ── 10. SilenceGate analysis ──────────────────────────────────
        analyzeForSilenceGate(buffer, safeSamples);

        // ── 11. Reset coupling accumulators ──────────────────────────
        // Reset AFTER consuming so next block starts clean.
        // applyCouplingInput() is called BEFORE renderBlock() each cycle;
        // resetting here ensures stale values never persist across missed blocks.
        couplingCityMod_ = 0.0f;
        couplingChokeMod_ = 0.0f;
        couplingFlipMod_ = 0.0f;
        couplingDecayMod_ = 0.0f;
        couplingFMMod_ = 0.0f;
    }

    //-- Coupling ----------------------------------------------------------------

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? couplingCacheL_ : couplingCacheR_;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* sourceBuffer, int numSamples) override
    {
        if (std::abs(amount) < 0.001f || sourceBuffer == nullptr)
            return;

        // Compute RMS of source for block-level coupling
        float rms = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            rms += sourceBuffer[i] * sourceBuffer[i];
        rms = std::sqrt(rms / static_cast<float>(numSamples));

        switch (type)
        {
        case CouplingType::AmpToFilter:
            // External amplitude modulates city processing intensity
            couplingCityMod_ = rms * amount;
            break;
        case CouplingType::AmpToChoke:
            // External amplitude chokes drum voices (sidechain duck)
            couplingChokeMod_ = rms * amount;
            break;
        case CouplingType::RhythmToBlend:
            // External rhythm modulates FLIP amount
            couplingFlipMod_ = rms * amount;
            break;
        case CouplingType::EnvToDecay:
            // External envelope modulates voice decay times
            couplingDecayMod_ = rms * amount;
            break;
        case CouplingType::AudioToFM:
            // External audio FM-modulates transient generator
            couplingFMMod_ = rms * amount;
            break;
        default:
            break;
        }
    }

    //-- Parameters ---------------------------------------------------------------

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;

        // ── Per-voice parameters (6 × 8 = 48) ────────────────────────
        static const int defTypes[8] = {0, 1, 2, 3, 4, 5, 6, 7};
        static const float defDecay[8] = {0.35f, 0.22f, 0.05f, 0.4f, 0.25f, 0.04f, 0.35f, 0.15f};
        static const float defBody[8] = {0.7f, 0.5f, 0.3f, 0.3f, 0.2f, 0.4f, 0.6f, 0.4f};
        static const float defPan[8] = {0.0f, 0.0f, 0.3f, -0.3f, 0.15f, -0.15f, 0.4f, -0.4f};

        for (int v = 0; v < 8; ++v)
        {
            juce::String prefix = "ofr_v" + juce::String(v) + "_";
            juce::String name = "Offering V" + juce::String(v) + " ";

            params.push_back(
                std::make_unique<PI>(juce::ParameterID{prefix + "type", 1}, name + "Type", 0, 7, defTypes[v]));
            params.push_back(std::make_unique<PF>(juce::ParameterID{prefix + "tune", 1}, name + "Tune",
                                                  juce::NormalisableRange<float>(-24.0f, 24.0f), 0.0f));
            params.push_back(std::make_unique<PF>(juce::ParameterID{prefix + "decay", 1}, name + "Decay",
                                                  juce::NormalisableRange<float>(0.001f, 2.0f, 0.001f, 0.5f),
                                                  defDecay[v]));
            params.push_back(std::make_unique<PF>(juce::ParameterID{prefix + "body", 1}, name + "Body",
                                                  juce::NormalisableRange<float>(0.0f, 1.0f), defBody[v]));
            params.push_back(std::make_unique<PF>(juce::ParameterID{prefix + "level", 1}, name + "Level",
                                                  juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));
            params.push_back(std::make_unique<PF>(juce::ParameterID{prefix + "pan", 1}, name + "Pan",
                                                  juce::NormalisableRange<float>(-1.0f, 1.0f), defPan[v]));
        }

        // ── Transient Generator (3) ───────────────────────────────────
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_transientSnap", 1}, "Offering Transient Snap",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.65f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_transientPitch", 1}, "Offering Transient Pitch",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_transientSat", 1}, "Offering Transient Sat",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.15f));

        // ── Texture / DUST (5) ────────────────────────────────────────
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_dustVinyl", 1}, "Offering Dust Vinyl",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_dustTape", 1}, "Offering Dust Tape",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.08f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"ofr_dustBits", 1}, "Offering Dust Bits", 4, 16, 16));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_dustSampleRate", 1}, "Offering Dust SR",
                                              juce::NormalisableRange<float>(8000.0f, 48000.0f, 1.0f), 48000.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_dustWobble", 1}, "Offering Dust Wobble",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.05f));

        // ── Collage / FLIP (4) ────────────────────────────────────────
        params.push_back(std::make_unique<PI>(juce::ParameterID{"ofr_flipLayers", 1}, "Offering Flip Layers", 1, 4, 1));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_flipChop", 1}, "Offering Flip Chop",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_flipStretch", 1}, "Offering Flip Stretch",
                                              juce::NormalisableRange<float>(0.5f, 2.0f), 1.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_flipRingMod", 1}, "Offering Flip Ring Mod",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // ── City Processing (3) ───────────────────────────────────────
        params.push_back(std::make_unique<PI>(juce::ParameterID{"ofr_cityMode", 1}, "Offering City Mode", 0, 4, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_cityBlend", 1}, "Offering City Blend",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_cityIntensity", 1}, "Offering City Intensity",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));

        // ── Curiosity / DIG (3) ───────────────────────────────────────
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_digCuriosity", 1}, "Offering Dig Curiosity",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_digComplexity", 1}, "Offering Dig Complexity",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.35f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_digFlow", 1}, "Offering Dig Flow",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.6f));

        // ── Expression & Modulation (12) ──────────────────────────────
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_velToSnap", 1}, "Offering Vel→Snap",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.6f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_velToBody", 1}, "Offering Vel→Body",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_lfo1Rate", 1}, "Offering LFO1 Rate",
                                              juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.4f), 0.067f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_lfo1Depth", 1}, "Offering LFO1 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.05f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"ofr_lfo1Shape", 1}, "Offering LFO1 Shape", 0, 4, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_lfo2Rate", 1}, "Offering LFO2 Rate",
                                              juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.4f), 2.17f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_lfo2Depth", 1}, "Offering LFO2 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.08f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_aftertouch", 1}, "Offering Aftertouch Amt",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_modWheel", 1}, "Offering Mod Wheel Amt",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_envFilterAmt", 1}, "Offering Env→Filter",
                                              juce::NormalisableRange<float>(-1.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_velToAttack", 1}, "Offering Vel→Attack",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_envToPitch", 1}, "Offering Env→Pitch",
                                              juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));

        // ── Master (2) ────────────────────────────────────────────────
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_masterLevel", 1}, "Offering Master Level",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.75f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_masterWidth", 1}, "Offering Master Width",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.6f));

        // ── Macros (4): DIG, CITY, FLIP, DUST ─────────────────────────
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_macroDig", 1}, "Offering Macro DIG",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_macroCity", 1}, "Offering Macro CITY",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_macroFlip", 1}, "Offering Macro FLIP",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"ofr_macroDust", 1}, "Offering Macro DUST",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        // Per-voice params
        for (int v = 0; v < 8; ++v)
        {
            juce::String prefix = "ofr_v" + juce::String(v) + "_";
            paramVoiceType_[v] = apvts.getRawParameterValue(prefix + "type");
            paramVoiceTune_[v] = apvts.getRawParameterValue(prefix + "tune");
            paramVoiceDecay_[v] = apvts.getRawParameterValue(prefix + "decay");
            paramVoiceBody_[v] = apvts.getRawParameterValue(prefix + "body");
            paramVoiceLevel_[v] = apvts.getRawParameterValue(prefix + "level");
            paramVoicePan_[v] = apvts.getRawParameterValue(prefix + "pan");
        }

        // Global params
        paramTransientSnap_ = apvts.getRawParameterValue("ofr_transientSnap");
        paramTransientPitch_ = apvts.getRawParameterValue("ofr_transientPitch");
        paramTransientSat_ = apvts.getRawParameterValue("ofr_transientSat");
        paramDustVinyl_ = apvts.getRawParameterValue("ofr_dustVinyl");
        paramDustTape_ = apvts.getRawParameterValue("ofr_dustTape");
        paramDustBits_ = apvts.getRawParameterValue("ofr_dustBits");
        paramDustSR_ = apvts.getRawParameterValue("ofr_dustSampleRate");
        paramDustWobble_ = apvts.getRawParameterValue("ofr_dustWobble");
        paramFlipLayers_ = apvts.getRawParameterValue("ofr_flipLayers");
        paramFlipChop_ = apvts.getRawParameterValue("ofr_flipChop");
        paramFlipStretch_ = apvts.getRawParameterValue("ofr_flipStretch");
        paramFlipRingMod_ = apvts.getRawParameterValue("ofr_flipRingMod");
        paramCityMode_ = apvts.getRawParameterValue("ofr_cityMode");
        paramCityBlend_ = apvts.getRawParameterValue("ofr_cityBlend");
        paramCityIntensity_ = apvts.getRawParameterValue("ofr_cityIntensity");
        paramDigCuriosity_ = apvts.getRawParameterValue("ofr_digCuriosity");
        paramDigComplexity_ = apvts.getRawParameterValue("ofr_digComplexity");
        paramDigFlow_ = apvts.getRawParameterValue("ofr_digFlow");
        paramVelToSnap_ = apvts.getRawParameterValue("ofr_velToSnap");
        paramVelToBody_ = apvts.getRawParameterValue("ofr_velToBody");
        paramLfo1Rate_ = apvts.getRawParameterValue("ofr_lfo1Rate");
        paramLfo1Depth_ = apvts.getRawParameterValue("ofr_lfo1Depth");
        paramLfo1Shape_ = apvts.getRawParameterValue("ofr_lfo1Shape");
        paramLfo2Rate_ = apvts.getRawParameterValue("ofr_lfo2Rate");
        paramLfo2Depth_ = apvts.getRawParameterValue("ofr_lfo2Depth");
        paramAftertouch_ = apvts.getRawParameterValue("ofr_aftertouch");
        paramModWheel_ = apvts.getRawParameterValue("ofr_modWheel");
        paramEnvFilterAmt_ = apvts.getRawParameterValue("ofr_envFilterAmt");
        paramVelToAttack_ = apvts.getRawParameterValue("ofr_velToAttack");
        paramEnvToPitch_ = apvts.getRawParameterValue("ofr_envToPitch");
        paramMasterLevel_ = apvts.getRawParameterValue("ofr_masterLevel");
        paramMasterWidth_ = apvts.getRawParameterValue("ofr_masterWidth");
        paramMacroDig_ = apvts.getRawParameterValue("ofr_macroDig");
        paramMacroCity_ = apvts.getRawParameterValue("ofr_macroCity");
        paramMacroFlip_ = apvts.getRawParameterValue("ofr_macroFlip");
        paramMacroDust_ = apvts.getRawParameterValue("ofr_macroDust");
    }

private:
    //--------------------------------------------------------------------------
    // MIDI note → voice index mapping (GM drum map positions)
    //--------------------------------------------------------------------------
    int noteToVoice(int midiNote) const noexcept
    {
        for (int i = 0; i < 8; ++i)
            if (kOfferingVoiceNotes[i] == midiNote)
                return i;
        // Alternate kick trigger (GM: Acoustic Bass Drum)
        if (midiNote == 35)
            return 0;
        return -1;
    }

    //--------------------------------------------------------------------------
    // Trigger a voice with curiosity-driven parameter variation.
    //--------------------------------------------------------------------------
    void triggerVoice(int voiceIdx, float velocity) noexcept
    {
        auto& vs = voices_[voiceIdx];
        int type = static_cast<int>(loadParam(paramVoiceType_[voiceIdx], 0.0f));
        float tune = loadParam(paramVoiceTune_[voiceIdx], 0.0f);
        float decay = loadParam(paramVoiceDecay_[voiceIdx], 0.3f);
        float body = loadParam(paramVoiceBody_[voiceIdx], 0.5f);
        float snap = loadParam(paramTransientSnap_, 0.5f);
        float pitchEnv = loadParam(paramTransientPitch_, 0.3f);
        float sat = loadParam(paramTransientSat_, 0.15f);

        // D001: velocity → timbre (not just volume)
        float velSnap = loadParam(paramVelToSnap_, 0.5f);
        float velBody = loadParam(paramVelToBody_, 0.3f);
        float velAttack = loadParam(paramVelToAttack_, 0.2f);
        snap = std::clamp(snap + (velocity - 0.5f) * velSnap * 0.6f, 0.0f, 1.0f);
        body = std::clamp(body + (velocity - 0.5f) * velBody * 0.4f, 0.0f, 1.0f);

        // D001: velToAttack — higher velocity tightens the transient attack (shorter decay).
        // Scales decay proportionally: full velocity + full velToAttack = 80% shorter decay.
        decay = std::clamp(decay * (1.0f - velocity * velAttack * 0.8f), 0.001f, 2.0f);

        // AudioToFM coupling: external FM signal modulates transient pitch-envelope depth.
        // couplingFMMod_ > 0 deepens the pitch sweep, adding FM character to the hit.
        if (std::abs(couplingFMMod_) > 0.001f)
            pitchEnv = std::clamp(pitchEnv + couplingFMMod_ * 0.5f, 0.0f, 1.0f);

        // envToPitch is stored for per-sample use in renderBlock (dynamic freq modulation).
        // Cache it into the voice so renderBlock can read it without a param lookup per sample.
        (void)loadParam(paramEnvToPitch_, 0.0f); // consumed in renderBlock via snap.envToPitch

        // Curiosity variation
        float curiosity = loadParam(paramDigCuriosity_, 0.5f);
        float complexity = loadParam(paramDigComplexity_, 0.4f);
        float flow = loadParam(paramDigFlow_, 0.6f);
        auto variation = curiosity_.generateVariation(curiosity, complexity, flow, voiceIdx);

        tune += variation.tuneDelta;
        decay = std::clamp(decay + variation.decayDelta, 0.001f, 2.0f);
        body = std::clamp(body + variation.bodyDelta, 0.0f, 1.0f);
        snap = std::clamp(snap + variation.snapDelta, 0.0f, 1.0f);
        pitchEnv = std::clamp(pitchEnv + variation.pitchEnvDelta, 0.0f, 1.0f);
        sat = std::clamp(sat + variation.satDelta, 0.0f, 1.0f);

        // couplingDecayMod_: external envelope multiplies into voice decay at trigger time
        // couplingDecayMod_ > 0 shortens decay (sidechain tighten); clamped to valid range
        if (std::abs(couplingDecayMod_) > 0.001f)
            decay = std::clamp(decay * std::max(0.1f, 1.0f - couplingDecayMod_), 0.001f, 2.0f);

        // Hat choke: closed hat kills open hat
        if (type == OfferingTransient::CHat)
            voices_[3].transient.choke();

        // Collage params (cached for potential delayed trigger)
        int layers = static_cast<int>(loadParam(paramFlipLayers_, 1.0f));
        float chop = loadParam(paramFlipChop_, 0.0f);
        float stretch = loadParam(paramFlipStretch_, 1.0f);
        float ringMod = loadParam(paramFlipRingMod_, 0.0f);

        // Detroit drunk timing: per-voice trigger delay counter.
        // When Detroit city is active, each voice gets a random ±15ms offset
        // on its trigger time. This models Dilla's sequencer-level swing.
        int cityMode = static_cast<int>(loadParam(paramCityMode_, 0.0f));
        float cityIntensity = loadParam(paramCityIntensity_, 0.5f);

        if (cityMode == 1 && cityIntensity > 0.01f) // Detroit
        {
            // Per-voice xorshift PRNG for drunk timing. Seed advances each trigger
            // so consecutive hits on the same voice get different offsets (was: fixed
            // seed from voiceIdx alone, giving identical delay every hit).
            drunkRngState_[voiceIdx] ^= drunkRngState_[voiceIdx] << 13;
            drunkRngState_[voiceIdx] ^= drunkRngState_[voiceIdx] >> 17;
            drunkRngState_[voiceIdx] ^= drunkRngState_[voiceIdx] << 5;
            float rng = static_cast<float>(drunkRngState_[voiceIdx]) / 4294967296.0f; // [0, 1)
            float offsetMs = (rng * 2.0f - 1.0f) * 15.0f * cityIntensity; // ±15ms
            int delaySamples = static_cast<int>(std::abs(offsetMs) * 0.001f * sr_);
            delaySamples = std::min(delaySamples, static_cast<int>(sr_ * 0.02f)); // cap at 20ms

            if (delaySamples > 0)
            {
                // Cache trigger params and start countdown
                vs.pendingTrigger = true;
                vs.drunkDelaySamples = delaySamples;
                vs.pendType = type;
                vs.pendVelocity = velocity;
                vs.pendTune = tune + pitchBendNorm_ * 2.0f;
                vs.pendDecay = decay;
                vs.pendBody = body;
                vs.pendSnap = snap;
                vs.pendPitchEnv = pitchEnv;
                vs.pendSat = sat;
                vs.pendLayers = layers;
                vs.pendChop = chop;
                vs.pendStretch = stretch;
                vs.pendRingMod = ringMod;
                vs.active = true; // mark active so render loop processes the countdown
                return;
            }
        }

        // Immediate trigger (non-Detroit or zero delay)
        vs.transient.trigger(type, velocity, tune + pitchBendNorm_ * 2.0f, decay, body, snap, pitchEnv, sat);
        vs.active = true;
        vs.collage.trigger(layers, chop, stretch, ringMod);
    }

    //--------------------------------------------------------------------------
    // Load parameter from cached pointer with fallback default.
    //--------------------------------------------------------------------------
    static float loadParam(std::atomic<float>* p, float def) noexcept
    {
        return p ? p->load(std::memory_order_relaxed) : def;
    }

    //--------------------------------------------------------------------------
    // Load all parameters into snapshot struct.
    //--------------------------------------------------------------------------
    void loadSnapshot(OfferingParamSnapshot& s) const noexcept
    {
        s.transientSnap = loadParam(paramTransientSnap_, 0.5f);
        s.transientPitch = loadParam(paramTransientPitch_, 0.3f);
        s.transientSat = loadParam(paramTransientSat_, 0.15f);
        s.dustVinyl = loadParam(paramDustVinyl_, 0.2f);
        s.dustTape = loadParam(paramDustTape_, 0.1f);
        s.dustBits = static_cast<int>(loadParam(paramDustBits_, 16.0f));
        s.dustSampleRate = loadParam(paramDustSR_, 48000.0f);
        s.dustWobble = loadParam(paramDustWobble_, 0.05f);
        s.flipLayers = static_cast<int>(loadParam(paramFlipLayers_, 1.0f));
        s.flipChop = loadParam(paramFlipChop_, 0.0f);
        s.flipStretch = loadParam(paramFlipStretch_, 1.0f);
        s.flipRingMod = loadParam(paramFlipRingMod_, 0.0f);
        s.cityMode = static_cast<int>(loadParam(paramCityMode_, 0.0f));
        s.cityBlend = loadParam(paramCityBlend_, 0.0f);
        s.cityIntensity = loadParam(paramCityIntensity_, 0.5f);
        s.digCuriosity = loadParam(paramDigCuriosity_, 0.5f);
        s.digComplexity = loadParam(paramDigComplexity_, 0.4f);
        s.digFlow = loadParam(paramDigFlow_, 0.6f);
        s.velToSnap = loadParam(paramVelToSnap_, 0.5f);
        s.velToBody = loadParam(paramVelToBody_, 0.3f);
        s.lfo1Rate = loadParam(paramLfo1Rate_, 0.067f);
        s.lfo1Depth = loadParam(paramLfo1Depth_, 0.05f);
        s.lfo1Shape = static_cast<int>(loadParam(paramLfo1Shape_, 0.0f));
        s.lfo2Rate = loadParam(paramLfo2Rate_, 2.0f);
        s.lfo2Depth = loadParam(paramLfo2Depth_, 0.0f);
        s.aftertouch = loadParam(paramAftertouch_, 0.3f);
        s.modWheel = loadParam(paramModWheel_, 0.5f);
        s.envFilterAmt = loadParam(paramEnvFilterAmt_, 0.5f);
        s.velToAttack = loadParam(paramVelToAttack_, 0.2f);
        s.envToPitch = loadParam(paramEnvToPitch_, 0.0f);
        s.masterLevel = loadParam(paramMasterLevel_, 0.75f);
        s.masterWidth = loadParam(paramMasterWidth_, 0.5f);

        for (int v = 0; v < 8; ++v)
        {
            s.voice[v].type = static_cast<int>(loadParam(paramVoiceType_[v], static_cast<float>(v)));
            s.voice[v].tune = loadParam(paramVoiceTune_[v], 0.0f);
            s.voice[v].decay = loadParam(paramVoiceDecay_[v], 0.3f);
            s.voice[v].body = loadParam(paramVoiceBody_[v], 0.5f);
            s.voice[v].level = loadParam(paramVoiceLevel_[v], 0.8f);
            s.voice[v].pan = loadParam(paramVoicePan_[v], 0.0f);
        }
    }

    //--------------------------------------------------------------------------
    // State
    //--------------------------------------------------------------------------
    float sr_ = 0.0f;  // Sentinel: must be set by prepare() before use
    int blockSize_ = 512;

    // Voices
    OfferingVoice voices_[8];

    // DSP modules
    OfferingCityProcessor cityProcessor_;
    OfferingCuriosity curiosity_;
    StandardLFO lfo1_;
    StandardLFO lfo2_;

    // MIDI expression state
    float pitchBendNorm_ = 0.0f;
    float aftertouchValue_ = 0.0f;
    float modWheelValue_ = 0.0f;

    // Coupling cache
    float couplingCacheL_ = 0.0f;
    float couplingCacheR_ = 0.0f;

    // Coupling modulation accumulators
    float couplingCityMod_ = 0.0f;
    float couplingChokeMod_ = 0.0f;
    float couplingFlipMod_ = 0.0f;
    float couplingDecayMod_ = 0.0f;
    float couplingFMMod_ = 0.0f;

    // Per-voice drunk-timing PRNG state (Detroit city mode).
    // Evolves each trigger so consecutive hits get different delays.
    uint32_t drunkRngState_[8] = { 7919, 15839, 23759, 31679, 39599, 47519, 55439, 63359 };

    //--------------------------------------------------------------------------
    // Cached parameter pointers (83 + 4 macros = 87 total)
    //--------------------------------------------------------------------------

    // Per-voice (6 × 8 = 48)
    std::atomic<float>* paramVoiceType_[8] = {};
    std::atomic<float>* paramVoiceTune_[8] = {};
    std::atomic<float>* paramVoiceDecay_[8] = {};
    std::atomic<float>* paramVoiceBody_[8] = {};
    std::atomic<float>* paramVoiceLevel_[8] = {};
    std::atomic<float>* paramVoicePan_[8] = {};

    // Global params (35)
    std::atomic<float>* paramTransientSnap_ = nullptr;
    std::atomic<float>* paramTransientPitch_ = nullptr;
    std::atomic<float>* paramTransientSat_ = nullptr;
    std::atomic<float>* paramDustVinyl_ = nullptr;
    std::atomic<float>* paramDustTape_ = nullptr;
    std::atomic<float>* paramDustBits_ = nullptr;
    std::atomic<float>* paramDustSR_ = nullptr;
    std::atomic<float>* paramDustWobble_ = nullptr;
    std::atomic<float>* paramFlipLayers_ = nullptr;
    std::atomic<float>* paramFlipChop_ = nullptr;
    std::atomic<float>* paramFlipStretch_ = nullptr;
    std::atomic<float>* paramFlipRingMod_ = nullptr;
    std::atomic<float>* paramCityMode_ = nullptr;
    std::atomic<float>* paramCityBlend_ = nullptr;
    std::atomic<float>* paramCityIntensity_ = nullptr;
    std::atomic<float>* paramDigCuriosity_ = nullptr;
    std::atomic<float>* paramDigComplexity_ = nullptr;
    std::atomic<float>* paramDigFlow_ = nullptr;
    std::atomic<float>* paramVelToSnap_ = nullptr;
    std::atomic<float>* paramVelToBody_ = nullptr;
    std::atomic<float>* paramLfo1Rate_ = nullptr;
    std::atomic<float>* paramLfo1Depth_ = nullptr;
    std::atomic<float>* paramLfo1Shape_ = nullptr;
    std::atomic<float>* paramLfo2Rate_ = nullptr;
    std::atomic<float>* paramLfo2Depth_ = nullptr;
    std::atomic<float>* paramAftertouch_ = nullptr;
    std::atomic<float>* paramModWheel_ = nullptr;
    std::atomic<float>* paramEnvFilterAmt_ = nullptr;
    std::atomic<float>* paramVelToAttack_ = nullptr;
    std::atomic<float>* paramEnvToPitch_ = nullptr;
    std::atomic<float>* paramMasterLevel_ = nullptr;
    std::atomic<float>* paramMasterWidth_ = nullptr;

    // Macros (4)
    std::atomic<float>* paramMacroDig_ = nullptr;
    std::atomic<float>* paramMacroCity_ = nullptr;
    std::atomic<float>* paramMacroFlip_ = nullptr;
    std::atomic<float>* paramMacroDust_ = nullptr;
};

} // namespace xoceanus
