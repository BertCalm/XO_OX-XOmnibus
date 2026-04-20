// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OctaveEngine.h — XOctave | "The Four Cathedrals"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOctave is the stone cathedral submerged at the continental shelf — four
//      chambers, four centuries, four breaths of air. In the Romantic nave,
//      Cavaille-Coll's pipes exhale dark harmonic towers that take seconds to
//      fill the space. In the Baroque positiv, chiff bursts precede transparent
//      principals like light through leaded glass. In the Musette alcove, three
//      reeds beat against each other with Parisian warmth. In the transistor
//      crypt, Farfisa squares buzz with garage-rock immediacy.
//
//      Octave is the Chef — classically trained, structurally precise, every
//      note placed with music-theory intent. Where Oto improvises, Octave
//      knows exactly where the resolution lies.
//
//  ENGINE CONCEPT:
//      A four-model organ synthesizer spanning 400 years of keyboard history.
//      Each model (Cavaille-Coll, Baroque Positiv, French Musette, Farfisa)
//      occupies a distinct DSP architecture — additive partials with wind noise,
//      chiff transients, triple-reed beating, or bandlimited square waves — yet
//      all share the same 6 Chef parameters (cluster, chiff, detune, buzz,
//      pressure, crosstalk) weighted differently per organ model. The contrast
//      between models IS the instrument: MASSIVE vs IMMEDIATE, dark vs bright,
//      sustained vs percussive.
//
//  THE 4 ORGAN MODELS:
//      0: Cavaille-Coll Romantic — additive synthesis, 12 drawbar partials,
//         wind noise, room resonance, slow attack. Dark, symphonic, massive.
//      1: Baroque Positiv — additive with chiff transient (noise burst before
//         tone), bright partials, fast attack. Articulate, transparent.
//      2: French Musette — 3 detuned oscillators per voice with controlled
//         beating. Bellows dynamics via velocity/aftertouch. Warm, vibrant.
//      3: Farfisa Compact — PolyBLEP square wave through resonant filter,
//         transistor vibrato. Buzzy, raw, immediate, punk-elegant.
//
//  HISTORICAL LINEAGE:
//      Aristide Cavaille-Coll (Saint-Sulpice, 1862), Arp Schnitger (Hamburg
//      Baroque, 1693), Paolo Soprani (Castelfidardo, 1863), Silvio Nascimbeni
//      (Farfisa, 1964). Organ acoustics from Audsley (1905), Jaffe & Smith
//      (1983), Smith (1986). Accordion reed beating from Millot et al. (2001).
//
//  Accent: Bordeaux #6B2D3E (cathedral wine, dark stone, organ wood)
//  Parameter prefix: oct_
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/FilterEnvelope.h"
#include "../../DSP/GlideProcessor.h"
#include "../../DSP/ParameterSmoother.h"
#include "../../DSP/VoiceAllocator.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
// Cavaille-Coll drawbar harmonic amplitudes — from Audsley "The Art of Organ
// Building" (1905) and Jaffe & Smith (1983) pipe organ synthesis.
// 12 partials at footages 16', 8', 5-1/3', 4', 2-2/3', 2', 1-3/5', 1-1/3',
// 1-1/7', 1', 8/9', 4/5' — harmonic series 1-12.
//==============================================================================
static constexpr float kCCPartialAmps[12] = {1.000f, 0.800f, 0.500f, 0.400f, 0.250f, 0.200f,
                                             0.120f, 0.100f, 0.070f, 0.060f, 0.040f, 0.030f};

// Baroque Positiv partials: brighter, less fundamental weight
static constexpr float kBaroquePartialAmps[12] = {0.700f, 1.000f, 0.600f, 0.800f, 0.400f, 0.500f,
                                                  0.300f, 0.250f, 0.200f, 0.150f, 0.100f, 0.080f};

// Registration mixing ratios for rank blending (8', 4', 2')
static constexpr float kRegistration8ft = 1.0f;
static constexpr float kRegistration4ft = 0.5f;
static constexpr float kRegistration2ft = 0.25f;

//==============================================================================
// OctaveChiffGenerator — Air burst transient before pipe speech.
// Models the initial "chiff" heard in tracker-action organs when the pallet
// valve opens and air first strikes the pipe lip. Duration 5-30ms, broadband
// noise shaped by pipe resonance. Ref: Nolle (1979) "Sound of the organ pipe."
//==============================================================================
struct OctaveChiffGenerator
{
    void trigger(float velocity, float chiffAmount, float baseFreq, float sampleRate) noexcept
    {
        if (chiffAmount < 0.001f)
        {
            active = false;
            return;
        }
        active = true;
        sampleCounter = 0;
        // Chiff duration: 5ms (bright) to 30ms (dark, Cavaille-Coll)
        float durationMs = 5.0f + (1.0f - chiffAmount) * 25.0f;
        totalSamples = std::max(4, static_cast<int>(durationMs * 0.001f * sampleRate));
        amplitude = velocity * chiffAmount;
        noiseState = static_cast<uint32_t>(baseFreq * 1000.0f) + 54321u;

        // Chiff filter: centered around pipe resonance (brighter for short pipes)
        // F11-fix: remove redundant double-clamp (same limit applied twice)
        // F10-fix: use full 2π constant
        constexpr float kTwoPi = 6.28318530717958647692f;
        float fc = std::min(baseFreq * 3.0f, sampleRate * 0.49f);
        chiffFilterFreq = fc;
        filterState = 0.0f;
        filterCoeff = 1.0f - std::exp(-kTwoPi * fc / sampleRate);
    }

    float process() noexcept
    {
        if (!active)
            return 0.0f;
        if (sampleCounter >= totalSamples)
        {
            active = false;
            return 0.0f;
        }

        // Windowed noise burst — half-sine envelope
        float phase = static_cast<float>(sampleCounter) / static_cast<float>(totalSamples);
        float envelope = fastSin(phase * 3.14159265f) * amplitude;

        // Broadband noise
        noiseState = noiseState * 1664525u + 1013904223u;
        float noise = (static_cast<float>(noiseState & 0xFFFF) / 32768.0f - 1.0f);

        float out = noise * envelope;

        // Shape noise to pipe resonance
        filterState += filterCoeff * (out - filterState);
        ++sampleCounter;
        return filterState;
    }

    void reset() noexcept
    {
        active = false;
        sampleCounter = 0;
        filterState = 0.0f;
    }

    bool active = false;
    int sampleCounter = 0, totalSamples = 200;
    float amplitude = 0.5f;
    uint32_t noiseState = 54321u;
    float chiffFilterFreq = 2000.0f;
    float filterCoeff = 0.1f, filterState = 0.0f;
};

//==============================================================================
// OctaveWindNoise — Continuous wind/air noise from the organ bellows.
// Low-level broadband noise filtered to match the pipe chest characteristics.
// Amount scales with pressure (velocity + aftertouch in bellows mode).
//==============================================================================
struct OctaveWindNoise
{
    // F13-fix: brightness in Hz → normalised [0,1] coefficient so the one-pole
    // filter stays stable.  At 200 Hz → coeff≈0.01 (very dark); 20 kHz → ≈0.29.
    // Uses exp(-2π·fc/sr) matched-Z pole; sr defaults to 44100 until prepare() sets it.
    void prepare(float sampleRate) noexcept
    {
        sr = (sampleRate > 0.0f) ? sampleRate : 44100.0f;
    }

    float process(float amount, float brightnessHz) noexcept
    {
        if (amount < 0.001f)
            return 0.0f;

        noiseState = noiseState * 1664525u + 1013904223u;
        float noise = (static_cast<float>(noiseState & 0xFFFF) / 32768.0f - 1.0f);

        // Shape wind noise — darker for pipe organs, brighter for accordion bellows.
        // Coefficient derived from cutoff frequency (matched-Z): stable for any sr.
        constexpr float kTwoPi = 6.28318530717958647692f;
        float fc = std::min(brightnessHz, sr * 0.49f);
        float coeff = 1.0f - std::exp(-kTwoPi * fc / sr);
        filterState += coeff * (noise - filterState);
        filterState = flushDenormal(filterState);
        return filterState * amount * 0.15f;
    }

    void reset() noexcept { filterState = 0.0f; }

    float sr = 44100.0f;
    uint32_t noiseState = 98765u;
    float filterState = 0.0f;
};

//==============================================================================
// OctaveRoomResonance — Simple room/chamber resonance for pipe organ models.
// 3 fixed body modes simulating the cathedral stone walls reflecting sound.
// Based on Jaffe & Smith (1983) "Extensions of the Karplus-Strong Algorithm."
//==============================================================================
struct OctaveRoomResonance
{
    void prepare(float sampleRate) noexcept
    {
        sr = sampleRate;
        // Cathedral body modes: low drone, mid presence, high shimmer
        mode1.setMode(CytomicSVF::Mode::BandPass);
        mode1.setCoefficients(120.0f, 0.7f, sampleRate); // stone resonance
        mode2.setMode(CytomicSVF::Mode::BandPass);
        mode2.setCoefficients(380.0f, 0.6f, sampleRate); // nave presence
        mode3.setMode(CytomicSVF::Mode::BandPass);
        mode3.setCoefficients(1200.0f, 0.5f, sampleRate); // vault shimmer
    }

    float process(float input, float depth) noexcept
    {
        if (depth < 0.001f)
            return input;
        float body =
            mode1.processSample(input) * 0.5f + mode2.processSample(input) * 0.3f + mode3.processSample(input) * 0.2f;
        return input * (1.0f - depth * 0.4f) + body * depth;
    }

    void reset() noexcept
    {
        mode1.reset();
        mode2.reset();
        mode3.reset();
    }

    float sr = 0.0f;  // Sentinel: must be set by prepare() before use
    CytomicSVF mode1, mode2, mode3;
};

//==============================================================================
// OctaveVoice — Per-voice state for all 4 organ models.
//==============================================================================
struct OctaveVoice
{
    static constexpr int kMaxPartials = 12;
    static constexpr int kMusetteOscs = 3; // triple-reed beating

    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    float velocity = 0.0f;
    bool releasing = false;

    GlideProcessor glide;

    // Amp envelope (ADSR — organ sustain is infinite while key held)
    StandardADSR ampEnv;

    // Filter envelope (D001: velocity shapes timbre)
    FilterEnvelope filterEnv;

    // Main output filter
    CytomicSVF svf;

    // Chiff transient (Baroque/Cavaille-Coll)
    OctaveChiffGenerator chiff;

    // Wind noise (per-voice for decorrelation)
    OctaveWindNoise wind;

    // Room resonance (per-voice for stereo spreading)
    OctaveRoomResonance room;

    // D002: LFOs — LFO1 for brightness, LFO2 for pitch/vibrato
    StandardLFO lfo1, lfo2;

    // F28-fix: stagger offsets preserved across voice steals (set once in prepare())
    float lfo1PhaseOffset = 0.0f;
    float lfo2PhaseOffset = 0.0f;

    // F17-fix: per-voice block-rate cluster detuning ratio cache
    float clusterFreqRatio = 1.0f;

    //--- Additive synthesis state (Cavaille-Coll + Baroque) ---
    std::array<float, kMaxPartials> partialPhases{};
    std::array<float, kMaxPartials> partialAmps{};

    //--- Musette triple-reed state ---
    std::array<float, kMusetteOscs> musettePhases{};
    std::array<float, kMusetteOscs> musettePhaseIncs{};

    //--- Farfisa state ---
    PolyBLEP farfisaOsc;
    float farfisaVibratoPhase = 0.0f;

    // Pan position (stereo)
    float panL = 0.707f, panR = 0.707f;

    void reset() noexcept
    {
        active = false;
        releasing = false;
        velocity = 0.0f;
        glide.reset();
        ampEnv.reset();
        filterEnv.kill();
        svf.reset();
        chiff.reset();
        wind.reset();
        room.reset();
        // F28-fix: restore stored phase offsets so ensemble stagger survives voice steals
        lfo1.reset(lfo1PhaseOffset);
        lfo2.reset(lfo2PhaseOffset);
        partialPhases.fill(0.0f);
        partialAmps.fill(0.0f);
        musettePhases.fill(0.0f);
        musettePhaseIncs.fill(0.0f);
        farfisaOsc.reset();
        farfisaVibratoPhase = 0.0f;
    }
};

//==============================================================================
// OctaveEngine — "The Four Cathedrals"
//==============================================================================
class OctaveEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    juce::String getEngineId() const override { return "Octave"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF8B6914); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load(); }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float>(sr);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            voices[i].reset();
            voices[i].ampEnv.prepare(srf);
            voices[i].filterEnv.prepare(srf);
            voices[i].room.prepare(srf);
            // F13-fix: propagate sample rate into wind noise filter
            voices[i].wind.prepare(srf);
            voices[i].lfo1.setShape(StandardLFO::Sine);
            voices[i].lfo2.setShape(StandardLFO::Sine);
            // Stagger LFO phases for ensemble depth
            float offset1 = static_cast<float>(i) / static_cast<float>(kMaxVoices);
            float offset2 = static_cast<float>(i) * 0.37f; // golden ratio spread
            voices[i].lfo1.setPhaseOffset(offset1);
            voices[i].lfo2.setPhaseOffset(offset2);
            // F28-fix: store offsets so reset() can re-apply them on voice steal
            voices[i].lfo1PhaseOffset = offset1;
            voices[i].lfo2PhaseOffset = offset2;
        }

        smoothCluster.prepare(srf);
        smoothChiff.prepare(srf);
        smoothDetune.prepare(srf);
        smoothBuzz.prepare(srf);
        smoothPressure.prepare(srf);
        smoothCrosstalk.prepare(srf);
        smoothBrightness.prepare(srf);
        smoothRegistration.prepare(srf);

        // P2: Post-mix room resonance (single instance replaces per-voice models)
        postMixRoomL.prepare(srf);
        postMixRoomR.prepare(srf);

        // Organ sustains — long tail
        prepareSilenceGate(sr, maxBlockSize, 500.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();
        postMixRoomL.reset();
        postMixRoomR.reset();
        pitchBendNorm = 0.0f;
        modWheelAmount = 0.0f;
        aftertouchAmount = 0.0f;
    }

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        (void)sampleIndex;
        return (channel == 0) ? couplingCacheL : couplingCacheR;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* buf, int numSamples) override
    {
        if (!buf || numSamples <= 0)
            return;
        float val = buf[numSamples - 1] * amount;
        switch (type)
        {
        case CouplingType::AmpToFilter:
            couplingFilterMod += val * 2000.0f;
            break;
        case CouplingType::LFOToPitch:
            couplingPitchMod += val * 2.0f;
            break;
        case CouplingType::AmpToPitch:
            couplingPitchMod += val;
            break;
        case CouplingType::EnvToMorph:
            couplingOrganMod += val;
            break;
        default:
            break;
        }
    }

    //==========================================================================
    // Render — all 4 organ models
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        // MIDI processing
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
                wakeSilenceGate();
            }
            else if (msg.isNoteOff())
                noteOff(msg.getNoteNumber());
            else if (msg.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
            else if (msg.isChannelPressure())
                aftertouchAmount = msg.getChannelPressureValue() / 127.0f;
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount = msg.getControllerValue() / 127.0f;
        }

        if (isSilenceGateBypassed())
        {
            buffer.clear(0, numSamples);
            couplingCacheL = couplingCacheR = 0.0f;
            return;
        }

        auto loadP = [](std::atomic<float>* p, float def) { return p ? p->load(std::memory_order_relaxed) : def; };

        // Core organ model selector
        const int organModel = static_cast<int>(loadP(paramOrgan, 0.0f));

        // Shared Chef params — weighted per model
        const float pCluster = loadP(paramCluster, 0.0f);
        const float pChiff = loadP(paramChiff, 0.3f);
        const float pDetune = loadP(paramDetune, 0.0f);
        const float pBuzz = loadP(paramBuzz, 0.0f);
        const float pPressure = loadP(paramPressure, 0.7f);
        const float pCrosstalk = loadP(paramCrosstalk, 0.0f);

        // Standard params
        const float pBrightness = loadP(paramBrightness, 8000.0f);
        const float pRegistration = loadP(paramRegistration, 0.5f);
        const float pRoomDepth = loadP(paramRoomDepth, 0.3f);
        const float pAttack = loadP(paramAttack, 0.1f);
        const float pDecay = loadP(paramDecay, 0.3f);
        const float pSustain = loadP(paramSustain, 0.9f);
        const float pRelease = loadP(paramRelease, 0.5f);
        const float pFilterEnvAmt = loadP(paramFilterEnvAmount, 0.2f);
        const float pBendRange = loadP(paramBendRange, 2.0f);

        // Macros
        const float macroCharacter = loadP(paramMacroCharacter, 0.0f);
        const float macroMovement = loadP(paramMacroMovement, 0.0f);
        const float macroCoupling = loadP(paramMacroCoupling, 0.0f);
        const float macroSpace = loadP(paramMacroSpace, 0.0f);

        // LFO params
        const float lfo1Rate = loadP(paramLfo1Rate, 0.3f);
        const float lfo1Depth = loadP(paramLfo1Depth, 0.1f);
        const int lfo1Shape = static_cast<int>(loadP(paramLfo1Shape, 0.0f));
        const float lfo2Rate = loadP(paramLfo2Rate, 5.0f);
        const float lfo2Depth = loadP(paramLfo2Depth, 0.0f);
        const int lfo2Shape = static_cast<int>(loadP(paramLfo2Shape, 0.0f));

        // Competition: adversarial suppression between tonewheel voices.
        // When non-zero, louder voices slightly suppress quieter neighbours —
        // emulates the wind-pressure "robbing" that occurs on a real organ when
        // many ranks are drawn simultaneously.
        const float competition = loadP(paramCompetition, 0.0f);

        // D006: aftertouch modulation — pressure for organ models
        float effectivePressure = std::clamp(pPressure + aftertouchAmount * 0.4f + macroCharacter * 0.3f, 0.0f, 1.0f);
        // D006: mod wheel → registration blend (organist's swell pedal)
        float effectiveRegistration =
            std::clamp(pRegistration + modWheelAmount * 0.5f + macroMovement * 0.3f, 0.0f, 1.0f);
        float effectiveBrightness = std::clamp(
            pBrightness + macroCharacter * 4000.0f + aftertouchAmount * 2000.0f + couplingFilterMod, 200.0f, 20000.0f);
        float effectiveRoomDepth = std::clamp(pRoomDepth + macroSpace * 0.4f, 0.0f, 1.0f);

        // D004-1: COUPLING macro → effective crosstalk amount.
        // More coupling = more bleed between adjacent-voice registers,
        // turning the organ into a tighter polyphonic body (like multiple
        // ranks sharing a single windchest).
        float effectiveCrosstalk = std::clamp(pCrosstalk + macroCoupling * 0.5f + couplingOrganMod * 0.3f, 0.0f, 1.0f);

        // Model-dependent attack time weighting:
        // Cavaille-Coll: MASSIVE — slow attack (pAttack * 3)
        // Baroque: medium (pAttack * 1.5)
        // Musette: medium-fast (pAttack * 1)
        // Farfisa: IMMEDIATE — near-zero attack (pAttack * 0.1)
        float attackMultipliers[4] = {3.0f, 1.5f, 1.0f, 0.1f};
        float effectiveAttack = pAttack * attackMultipliers[std::clamp(organModel, 0, 3)];
        // Minimum attack floor per model
        float attackFloors[4] = {0.05f, 0.005f, 0.003f, 0.001f};
        effectiveAttack = std::max(effectiveAttack, attackFloors[std::clamp(organModel, 0, 3)]);

        smoothCluster.set(pCluster);
        smoothChiff.set(pChiff);
        smoothDetune.set(pDetune);
        smoothBuzz.set(pBuzz);
        smoothPressure.set(effectivePressure);
        smoothCrosstalk.set(effectiveCrosstalk); // D004-1: uses macro + coupling mod
        smoothBrightness.set(effectiveBrightness);
        smoothRegistration.set(effectiveRegistration);

        // D004-2: organ morph blend — interpolates partial tables between
        // Cavaille-Coll (dark, symphonic) and Baroque (bright, transparent).
        // EnvToMorph coupling drives this; macroCharacter provides manual control.
        // Positive = shift toward Baroque, negative = deepen toward CC.
        // Clamped to [0,1] so it works as a lerp factor.
        const float organMorphBlend = std::clamp(std::abs(couplingOrganMod) * 1.5f + macroCharacter * 0.3f, 0.0f, 1.0f);
        // When couplingOrganMod < 0, morph direction reverses (CC→Baroque vs Baroque→CC)
        const bool morphToBright = (couplingOrganMod >= 0.0f);

        // F06-fix: capture pitch mod before zero so per-sample loop gets the live value.
        // P25: CAPTURE-THEN-ZERO — couplingFilterMod and couplingOrganMod consumed above;
        // couplingPitchMod is consumed inside the voice loop (added to bendSemitones per-voice).
        const float capturedPitchMod = couplingPitchMod;
        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;
        couplingOrganMod = 0.0f;

        const float bendSemitones = pitchBendNorm * pBendRange;

        // Hoist block-constant ADSR update out of per-sample loop (P15 fix).
        // effectiveAttack, pDecay, pSustain, pRelease are all block-rate constants.
        // F02-fix: setWaveform is also block-constant for Farfisa — hoist here.
        // F03-fix: LFO setRate/setShape are block-constant — hoist before sample loop.
        // F17-fix: cluster freq ratio is per-voice constant — compute once per block.
        for (int vi = 0; vi < kMaxVoices; ++vi)
        {
            if (!voices[vi].active)
                continue;
            voices[vi].ampEnv.setADSR(effectiveAttack, pDecay, pSustain, pRelease);
            voices[vi].lfo1.setRate(lfo1Rate, srf);
            voices[vi].lfo1.setShape(lfo1Shape);
            voices[vi].lfo2.setRate(lfo2Rate, srf);
            voices[vi].lfo2.setShape(lfo2Shape);
            // F02-fix: Farfisa waveform is constant per model selection — not per-sample
            voices[vi].farfisaOsc.setWaveform(PolyBLEP::Waveform::Square);
            // F17-fix: cache per-voice cluster detuning ratio (vi is constant across block)
            if (pCluster > 0.001f)
            {
                float voiceOffset = (static_cast<float>(vi) - 3.5f) / 3.5f;
                float clusterCents = pCluster * voiceOffset * 15.0f;
                voices[vi].clusterFreqRatio = fastPow2(clusterCents / 1200.0f);
            }
            else
            {
                voices[vi].clusterFreqRatio = 1.0f;
            }
        }

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        // F26-fix: hoist contrib arrays outside sample loop — avoids re-init overhead
        // and avoids VLA semantics (kMaxVoices is constexpr, so this is fine either way).
        float voiceContribL[kMaxVoices];
        float voiceContribR[kMaxVoices];

        for (int s = 0; s < numSamples; ++s)
        {
            float clusterNow = smoothCluster.process();
            float chiffNow = smoothChiff.process();
            float detuneNow = smoothDetune.process();
            float buzzNow = smoothBuzz.process();
            float pressureNow = smoothPressure.process();
            float crosstalkNow = smoothCrosstalk.process();
            float brightNow = smoothBrightness.process();
            float regNow = smoothRegistration.process();

            float mixL = 0.0f, mixR = 0.0f;

            // Competition: per-voice stereo contributions cached for suppression pass.
            // Arrays hoisted above loop (F26-fix) — zero them each sample here.
            for (int i = 0; i < kMaxVoices; ++i) { voiceContribL[i] = 0.0f; voiceContribR[i] = 0.0f; }

            for (int vi = 0; vi < kMaxVoices; ++vi)
            {
                auto& voice = voices[vi];
                if (!voice.active)
                    continue;

                float freq = voice.glide.process();
                // F06-fix: use capturedPitchMod (zeroed coupling, but captured before zero)
                freq *= PitchBendUtil::semitonesToFreqRatio(bendSemitones + capturedPitchMod);

                // LFO processing — rate/shape hoisted to block-rate pre-loop (F03-fix)
                float lfo1Val = voice.lfo1.process() * lfo1Depth;
                float lfo2Val = voice.lfo2.process() * lfo2Depth;

                // LFO2 → vibrato (pitch modulation) — organist's tremulant
                float vibratoSemitones = lfo2Val * 0.5f; // max ±0.5 semitones
                freq *= PitchBendUtil::semitonesToFreqRatio(vibratoSemitones);

                // F17-fix: use pre-computed per-voice cluster ratio (block-rate constant)
                if (clusterNow > 0.001f)
                    freq *= voice.clusterFreqRatio;

                //--- Model-specific synthesis ---
                float sample = 0.0f;

                switch (organModel)
                {
                case 0: // Cavaille-Coll Romantic Pipe Organ
                {
                    // Additive synthesis with 12 drawbar partials
                    float addSum = 0.0f;
                    for (int p = 0; p < OctaveVoice::kMaxPartials; ++p)
                    {
                        float harmonicNum = static_cast<float>(p + 1);
                        float partialFreq = freq * harmonicNum;
                        if (partialFreq > srf * 0.49f)
                            break;

                        // D004-2: organ morph — blend CC partial amps toward Baroque
                        // when coupling (EnvToMorph) or CHARACTER macro drives morphBlend.
                        // morphToBright=true: CC→Baroque (lighter, more presence)
                        // morphToBright=false: CC stays darker (deepens the cave)
                        float baseAmp = morphToBright ? lerp(kCCPartialAmps[p], kBaroquePartialAmps[p], organMorphBlend)
                                                      : kCCPartialAmps[p] * (1.0f + organMorphBlend * 0.3f); // deepen

                        // Registration: blend 8', 4', 2' ranks
                        float regAmp = baseAmp;
                        if (p >= 0 && p < 4)
                            regAmp *= lerp(0.5f, 1.0f, regNow); // 8' rank
                        if (p >= 4 && p < 8)
                            regAmp *= regNow * kRegistration4ft; // 4' rank
                        if (p >= 8 && p < 12)
                            regAmp *= regNow * kRegistration2ft; // 2' rank

                        // D001: velocity shapes upper partial brightness
                        float velBright = 1.0f - (1.0f - voice.velocity) * 0.5f * (harmonicNum / 12.0f);
                        regAmp *= velBright;

                        // Pressure affects amplitude (bellows/wind pressure)
                        regAmp *= 0.5f + pressureNow * 0.5f;

                        float phaseInc = partialFreq / srf;
                        voice.partialPhases[p] += phaseInc;
                        if (voice.partialPhases[p] >= 1.0f)
                            voice.partialPhases[p] -= 1.0f;

                        addSum += fastSin(voice.partialPhases[p] * 6.28318530717958647692f) * regAmp;
                    }
                    sample = addSum * 0.2f; // scale to prevent clipping

                    // Wind noise — always present in pipe organs
                    sample += voice.wind.process(0.02f + pressureNow * 0.03f, brightNow);

                    // Chiff transient (subtle for Cavaille-Coll — more of a "bloom")
                    sample += voice.chiff.process() * 0.3f;

                    // Room resonance moved to post-mix (P2 CPU optimization)
                    break;
                }

                case 1: // Baroque Positiv Organ
                {
                    // Additive with brighter partial emphasis
                    float addSum = 0.0f;
                    for (int p = 0; p < OctaveVoice::kMaxPartials; ++p)
                    {
                        float harmonicNum = static_cast<float>(p + 1);
                        float partialFreq = freq * harmonicNum;
                        if (partialFreq > srf * 0.49f)
                            break;

                        // D004-2: organ morph — blend Baroque partial amps toward CC
                        // morphToBright=false means we deepen Baroque (toward CC)
                        float baseAmp = (!morphToBright)
                                            ? lerp(kBaroquePartialAmps[p], kCCPartialAmps[p], organMorphBlend)
                                            : kBaroquePartialAmps[p];

                        float regAmp = baseAmp;
                        // Baroque: Principal (8') and Flute (4') blend via registration
                        if (p < 4)
                            regAmp *= lerp(0.6f, 1.0f, regNow);
                        else
                            regAmp *= 0.5f + regNow * 0.5f;

                        // D001: velocity → brightness (harder touch = brighter)
                        float velBright = 0.7f + voice.velocity * 0.3f * (harmonicNum / 12.0f);
                        regAmp *= velBright;

                        regAmp *= 0.6f + pressureNow * 0.4f;

                        float phaseInc = partialFreq / srf;
                        voice.partialPhases[p] += phaseInc;
                        if (voice.partialPhases[p] >= 1.0f)
                            voice.partialPhases[p] -= 1.0f;

                        addSum += fastSin(voice.partialPhases[p] * 6.28318530717958647692f) * regAmp;
                    }
                    sample = addSum * 0.2f;

                    // Prominent chiff — the defining character of Baroque organs
                    sample += voice.chiff.process() * chiffNow * 1.5f;

                    // Room resonance moved to post-mix (P2 CPU optimization)
                    break;
                }

                case 2: // French Musette Accordion
                {
                    // Triple-reed beating: 3 oscillators with controlled detuning
                    // Central reed is on-pitch; flanking reeds are ± detune
                    float detuneHz = 1.0f + detuneNow * 8.0f; // 1-9 Hz beating

                    voice.musettePhaseIncs[0] = freq / srf;
                    voice.musettePhaseIncs[1] = (freq + detuneHz) / srf;
                    voice.musettePhaseIncs[2] = (freq - detuneHz) / srf;

                    float reedSum = 0.0f;
                    for (int r = 0; r < OctaveVoice::kMusetteOscs; ++r)
                    {
                        voice.musettePhases[r] += voice.musettePhaseIncs[r];
                        if (voice.musettePhases[r] >= 1.0f)
                            voice.musettePhases[r] -= 1.0f;

                        // Accordion reeds: odd-harmonic-heavy (between square and saw)
                        // F24-fix: guard upper harmonics against Nyquist — drop harmonics
                        // that would alias (especially audible at high MIDI notes).
                        float ph = voice.musettePhases[r] * 6.28318530717958647692f;
                        // reedFreq = phaseInc * srf (fundamental of this reed)
                        float reedFreq = voice.musettePhaseIncs[r] * srf;
                        float nyq = srf * 0.49f;
                        float reed = fastSin(ph);
                        if (reedFreq * 3.0f < nyq) reed += fastSin(ph * 3.0f) * 0.33f;
                        if (reedFreq * 5.0f < nyq) reed += fastSin(ph * 5.0f) * 0.15f;
                        if (reedFreq * 7.0f < nyq) reed += fastSin(ph * 7.0f) * 0.08f;
                        reedSum += reed;
                    }
                    sample = reedSum * 0.12f; // 3 reeds, scale down

                    // Bellows dynamics: velocity + aftertouch = bellows pressure
                    float bellows = voice.velocity * 0.6f + pressureNow * 0.4f;
                    sample *= bellows;

                    // Buzz: reed buzz/rattle at high pressure
                    if (buzzNow > 0.001f && bellows > 0.5f)
                    {
                        float buzzAmt = buzzNow * (bellows - 0.5f) * 2.0f;
                        sample = sample + fastTanh(sample * (3.0f + buzzAmt * 8.0f)) * buzzAmt * 0.3f;
                    }
                    break;
                }

                case 3: // Farfisa Compact Organ
                {
                    // Bandlimited square wave — transistor organ
                    // F02-fix: setWaveform hoisted to block-rate pre-loop (constant per model)

                    // Farfisa vibrato: fixed 5.5 Hz (original circuit), depth from detune param
                    voice.farfisaVibratoPhase += 5.5f / srf;
                    if (voice.farfisaVibratoPhase >= 1.0f)
                        voice.farfisaVibratoPhase -= 1.0f;
                    float vibratoDepth = detuneNow * 0.015f; // subtle pitch vibrato
                    float farfisaFreq =
                        freq * (1.0f + fastSin(voice.farfisaVibratoPhase * 6.28318530717958647692f) * vibratoDepth);

                    voice.farfisaOsc.setFrequency(farfisaFreq, srf);
                    sample = voice.farfisaOsc.processSample();

                    // Registration: tab/stop simulation via harmonic content
                    // Low registration = more fundamental, high = more upper harmonics
                    // Simulate by mixing in an octave-up square
                    float octaveUp = 0.0f;
                    if (regNow > 0.3f)
                    {
                        // Use phase-offset for octave-up without extra oscillator
                        voice.partialPhases[0] += (farfisaFreq * 2.0f) / srf;
                        if (voice.partialPhases[0] >= 1.0f)
                            voice.partialPhases[0] -= 1.0f;
                        octaveUp = (voice.partialPhases[0] < 0.5f ? 1.0f : -1.0f);
                    }
                    sample = sample * (1.0f - regNow * 0.3f) + octaveUp * regNow * 0.3f;

                    // Buzz: transistor saturation / overdrive
                    if (buzzNow > 0.001f)
                    {
                        float drive = 1.0f + buzzNow * 6.0f;
                        sample = fastTanh(sample * drive) * 0.8f;
                    }

                    // Farfisa is dry — no room resonance
                    break;
                }
                }

                //--- Crosstalk: bleed between adjacent voices (organ key crosstalk) ---
                if (crosstalkNow > 0.001f && vi > 0 && voices[vi - 1].active)
                {
                    // F19-fix: crosstalk reads ic2eq (LP state) via processSample(0) — this
                    // advances the adjacent voice's filter state, corrupting its output.
                    // Use the voice's last output level (ampLevel * panL) as a proxy instead.
                    // The adjacent voice's contribution is already in voiceContribL[vi-1].
                    float adjSig = (voiceContribL[vi - 1] + voiceContribR[vi - 1]) * 0.5f;
                    sample += adjSig * crosstalkNow * 0.05f;
                }

                //--- Amplitude envelope ---
                // setADSR hoisted to block-rate above (P15 fix)
                float ampLevel = voice.ampEnv.process();
                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                //--- Filter envelope (D001: velocity shapes timbre) ---
                float envMod = voice.filterEnv.process() * pFilterEnvAmt * 4000.0f * voice.velocity;
                // LFO1 modulates brightness (±3000 Hz at full depth)
                float cutoff = std::clamp(brightNow + envMod + lfo1Val * 3000.0f, 200.0f, 20000.0f);
                // F01-fix: setMode is constant (LowPass) — hoisted to noteOn; use
                // setCoefficients_fast() for modulated cutoff (avoids std::tan per-sample).
                voice.svf.setCoefficients_fast(cutoff, 0.3f, srf);
                float filtered = voice.svf.processSample(sample);

                float output = filtered * ampLevel;

                // Store per-voice panned contributions for competition pass below.
                voiceContribL[vi] = output * voice.panL;
                voiceContribR[vi] = output * voice.panR;

                mixL += voiceContribL[vi];
                mixR += voiceContribR[vi];
            }

            // Competition pass: adversarial suppression between tonewheel voices.
            // When competition > 0, each voice is attenuated in proportion to how
            // much amplitude the other active voices are contributing — mimicking
            // the wind-pressure "robbing" on a real organ when many ranks are drawn.
            // At competition=1.0, a voice half as loud as the total field is pulled
            // down to ~67% of its natural level; the loudest voice is least affected.
            // A lone active voice is never suppressed (otherAmp=0 → factor=1.0).
            if (competition > 0.001f)
            {
                float totalAmp = 0.0f;
                for (int vi = 0; vi < kMaxVoices; ++vi)
                    totalAmp += std::abs(voiceContribL[vi]) + std::abs(voiceContribR[vi]);

                if (totalAmp > 0.001f)
                {
                    float suppressedMixL = 0.0f, suppressedMixR = 0.0f;
                    for (int vi = 0; vi < kMaxVoices; ++vi)
                    {
                        float contribMag = std::abs(voiceContribL[vi]) + std::abs(voiceContribR[vi]);
                        if (contribMag < 0.000001f)
                            continue;

                        float otherAmp = totalAmp - contribMag;
                        // suppressionFactor ∈ (0, 1]: 1/(1 + competition * otherAmp/totalAmp)
                        float suppressionFactor = 1.0f / (1.0f + competition * otherAmp / totalAmp);

                        suppressedMixL += voiceContribL[vi] * suppressionFactor;
                        suppressedMixR += voiceContribR[vi] * suppressionFactor;
                    }
                    mixL = suppressedMixL;
                    mixR = suppressedMixR;
                }
            }

            // P2: Post-mix room resonance — single cathedral model instead of 8.
            // Baroque uses half depth (smaller positiv case); Farfisa stays dry.
            // roomModelScale: 0=CC(full), 1=Baroque(0.5), 2=Musette(0.25), 3=Farfisa(0)
            static constexpr float kRoomScales[4] = {1.0f, 0.5f, 0.25f, 0.0f};
            float roomScale = kRoomScales[std::clamp(organModel, 0, 3)];
            float roomMix = effectiveRoomDepth * roomScale;
            if (roomMix > 0.001f)
            {
                // Apply same room to both channels (mono room is fine for cathedral)
                float roomedL = postMixRoomL.process(mixL, roomMix);
                float roomedR = postMixRoomR.process(mixR, roomMix);
                mixL = roomedL;
                mixR = roomedR;
            }

            outL[s] = mixL;
            if (outR)
                outR[s] = mixR;
            couplingCacheL = mixL;
            couplingCacheR = mixR;
        }

        int count = 0;
        for (const auto& v : voices)
            if (v.active)
                ++count;
        activeVoiceCount.store(count);
        analyzeForSilenceGate(buffer, numSamples);
    }

    //==========================================================================
    // Note management
    //==========================================================================

    void noteOn(int note, float vel) noexcept
    {
        int idx = VoiceAllocator::findFreeVoice(voices, kMaxVoices);
        auto& v = voices[idx];

        // F08-fix: use midiToFreq (fastPow2) instead of std::pow on audio thread
        float freq = midiToFreq(note);
        int organModel = paramOrgan ? static_cast<int>(paramOrgan->load()) : 0;

        v.active = true;
        v.releasing = false;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.glide.snapTo(freq);

        // Amp envelope — F07-fix: prepare() already called in engine prepare(); skip here
        float attackMultipliers[4] = {3.0f, 1.5f, 1.0f, 0.1f};
        float attackFloors[4] = {0.05f, 0.005f, 0.003f, 0.001f};
        float atkBase = paramAttack ? paramAttack->load() : 0.1f;
        float effectiveAttack = std::max(atkBase * attackMultipliers[std::clamp(organModel, 0, 3)],
                                         attackFloors[std::clamp(organModel, 0, 3)]);
        float decVal = paramDecay ? paramDecay->load() : 0.3f;
        float susVal = paramSustain ? paramSustain->load() : 0.9f;
        float relVal = paramRelease ? paramRelease->load() : 0.5f;
        v.ampEnv.setADSR(effectiveAttack, decVal, susVal, relVal);
        v.ampEnv.noteOn();

        // Filter envelope — F07-fix: prepare() already called in engine prepare(); skip here
        // Filter attack matches organ model character
        float filterAtk = (organModel == 0) ? 0.05f : 0.005f;
        float filterDec = (organModel == 0) ? 0.8f : 0.3f;
        v.filterEnv.setADSR(filterAtk, filterDec, 0.0f, 0.5f);
        v.filterEnv.triggerHard();

        // F01-fix: hoist SVF mode (constant LowPass) to noteOn; setCoefficients_fast used per-sample
        v.svf.setMode(CytomicSVF::Mode::LowPass);

        // Chiff trigger — weighted per model
        float chiffAmt = paramChiff ? paramChiff->load() : 0.3f;
        float chiffWeights[4] = {0.3f, 1.0f, 0.0f, 0.0f}; // Baroque gets full chiff
        v.chiff.trigger(vel, chiffAmt * chiffWeights[std::clamp(organModel, 0, 3)], freq, srf);

        // F04-fix: room.prepare() already called in engine prepare(); not needed per-noteOn
        // (coefficients are fixed; per-voice room objects are dead weight — postMixRoom handles rendering)

        // Reset oscillator state
        v.partialPhases.fill(0.0f);
        v.musettePhases.fill(0.0f);
        v.farfisaOsc.reset();
        v.farfisaVibratoPhase = 0.0f;

        // Stereo spread: distribute voices across stereo field
        // F18-fix: use fastSin/fastCos instead of std::cos/std::sin on audio thread
        float panAngle = (static_cast<float>(note % 12) / 12.0f - 0.5f) * 0.6f + 0.5f;
        v.panL = fastCos(panAngle * 1.5707963f);
        v.panR = fastSin(panAngle * 1.5707963f);
    }

    void noteOff(int note) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && v.currentNote == note && !v.releasing)
            {
                v.releasing = true;
                v.ampEnv.noteOff();
                v.filterEnv.release();
            }
        }
    }

    //==========================================================================
    // Parameters — 27 total
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;

        // Organ model selector (0-3)
        params.push_back(std::make_unique<PI>(juce::ParameterID{"oct_organ", 1}, "Octave Organ Model", 0, 3, 0));

        // 6 shared Chef params
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_cluster", 1}, "Octave Cluster",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_chiff", 1}, "Octave Chiff",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_detune", 1}, "Octave Detune",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_buzz", 1}, "Octave Buzz",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_pressure", 1}, "Octave Pressure",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_crosstalk", 1}, "Octave Crosstalk",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // Tone shaping
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_brightness", 1}, "Octave Brightness",
                                              juce::NormalisableRange<float>(200.0f, 20000.0f, 0.0f, 0.3f), 8000.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_registration", 1}, "Octave Registration",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_roomDepth", 1}, "Octave Room Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // ADSR
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_attack", 1}, "Octave Attack",
                                              juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.3f), 0.1f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_decay", 1}, "Octave Decay",
                                              juce::NormalisableRange<float>(0.01f, 10.0f, 0.0f, 0.4f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_sustain", 1}, "Octave Sustain",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.9f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_release", 1}, "Octave Release",
                                              juce::NormalisableRange<float>(0.01f, 10.0f, 0.0f, 0.4f), 0.5f));

        // Filter envelope
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_filterEnvAmount", 1}, "Octave Filter Env Amount",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));

        // Pitch bend
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_bendRange", 1}, "Octave Pitch Bend Range",
                                              juce::NormalisableRange<float>(1.0f, 24.0f, 1.0f), 2.0f));

        // Macros
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_macroCharacter", 1}, "Octave Macro CHARACTER",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_macroMovement", 1}, "Octave Macro MOVEMENT",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_macroCoupling", 1}, "Octave Macro COUPLING",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_macroSpace", 1}, "Octave Macro SPACE",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // LFOs (D002/D005 compliance)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_lfo1Rate", 1}, "Octave LFO1 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_lfo1Depth", 1}, "Octave LFO1 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.1f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"oct_lfo1Shape", 1}, "Octave LFO1 Shape", 0, 4, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_lfo2Rate", 1}, "Octave LFO2 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 5.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_lfo2Depth", 1}, "Octave LFO2 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"oct_lfo2Shape", 1}, "Octave LFO2 Shape", 0, 4, 0));

        // Competition param (coupling awareness)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oct_competition", 1}, "Octave Competition",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        paramOrgan = apvts.getRawParameterValue("oct_organ");
        paramCluster = apvts.getRawParameterValue("oct_cluster");
        paramChiff = apvts.getRawParameterValue("oct_chiff");
        paramDetune = apvts.getRawParameterValue("oct_detune");
        paramBuzz = apvts.getRawParameterValue("oct_buzz");
        paramPressure = apvts.getRawParameterValue("oct_pressure");
        paramCrosstalk = apvts.getRawParameterValue("oct_crosstalk");
        paramBrightness = apvts.getRawParameterValue("oct_brightness");
        paramRegistration = apvts.getRawParameterValue("oct_registration");
        paramRoomDepth = apvts.getRawParameterValue("oct_roomDepth");
        paramAttack = apvts.getRawParameterValue("oct_attack");
        paramDecay = apvts.getRawParameterValue("oct_decay");
        paramSustain = apvts.getRawParameterValue("oct_sustain");
        paramRelease = apvts.getRawParameterValue("oct_release");
        paramFilterEnvAmount = apvts.getRawParameterValue("oct_filterEnvAmount");
        paramBendRange = apvts.getRawParameterValue("oct_bendRange");
        paramMacroCharacter = apvts.getRawParameterValue("oct_macroCharacter");
        paramMacroMovement = apvts.getRawParameterValue("oct_macroMovement");
        paramMacroCoupling = apvts.getRawParameterValue("oct_macroCoupling");
        paramMacroSpace = apvts.getRawParameterValue("oct_macroSpace");
        paramLfo1Rate = apvts.getRawParameterValue("oct_lfo1Rate");
        paramLfo1Depth = apvts.getRawParameterValue("oct_lfo1Depth");
        paramLfo1Shape = apvts.getRawParameterValue("oct_lfo1Shape");
        paramLfo2Rate = apvts.getRawParameterValue("oct_lfo2Rate");
        paramLfo2Depth = apvts.getRawParameterValue("oct_lfo2Depth");
        paramLfo2Shape = apvts.getRawParameterValue("oct_lfo2Shape");
        paramCompetition = apvts.getRawParameterValue("oct_competition");
    }

private:
    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    float srf = 0.0f;  // Sentinel: must be set by prepare() before use

    std::array<OctaveVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount{0};

    ParameterSmoother smoothCluster, smoothChiff, smoothDetune, smoothBuzz;
    ParameterSmoother smoothPressure, smoothCrosstalk, smoothBrightness, smoothRegistration;

    // P2: Single post-mix room resonance (replaces 8 per-voice instances)
    OctaveRoomResonance postMixRoomL, postMixRoomR;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;

    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f, couplingOrganMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    // Parameter pointers (28 params)
    std::atomic<float>* paramOrgan = nullptr;
    std::atomic<float>* paramCluster = nullptr;
    std::atomic<float>* paramChiff = nullptr;
    std::atomic<float>* paramDetune = nullptr;
    std::atomic<float>* paramBuzz = nullptr;
    std::atomic<float>* paramPressure = nullptr;
    std::atomic<float>* paramCrosstalk = nullptr;
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramRegistration = nullptr;
    std::atomic<float>* paramRoomDepth = nullptr;
    std::atomic<float>* paramAttack = nullptr;
    std::atomic<float>* paramDecay = nullptr;
    std::atomic<float>* paramSustain = nullptr;
    std::atomic<float>* paramRelease = nullptr;
    std::atomic<float>* paramFilterEnvAmount = nullptr;
    std::atomic<float>* paramBendRange = nullptr;
    std::atomic<float>* paramMacroCharacter = nullptr;
    std::atomic<float>* paramMacroMovement = nullptr;
    std::atomic<float>* paramMacroCoupling = nullptr;
    std::atomic<float>* paramMacroSpace = nullptr;
    std::atomic<float>* paramLfo1Rate = nullptr;
    std::atomic<float>* paramLfo1Depth = nullptr;
    std::atomic<float>* paramLfo1Shape = nullptr;
    std::atomic<float>* paramLfo2Rate = nullptr;
    std::atomic<float>* paramLfo2Depth = nullptr;
    std::atomic<float>* paramLfo2Shape = nullptr;
    std::atomic<float>* paramCompetition = nullptr;
};

} // namespace xoceanus
