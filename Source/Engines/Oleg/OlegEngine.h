// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OlegEngine.h — XOleg | "The Sacred Bellows"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOleg is the amber organ — a sacred instrument from the Baltic and
//      Eastern European borderlands where Orthodox chant meets industrial
//      grit. Four organ models converge in a single engine: the commanding
//      Bayan concert accordion, the medieval hurdy-gurdy with its iconic
//      buzz bridge, the melancholic bandoneon of tango, and the raw folk
//      garmon. Each model carries a different spiritual weight, a different
//      relationship between breath and mechanism, between the sacred and
//      the mechanical.
//
//  ENGINE CONCEPT:
//      A switchable multi-organ synthesizer where four distinct bellows-driven
//      or wheel-driven instruments are selectable via a single parameter.
//      Each model implements its own oscillator topology, resonance structure,
//      and characteristic nonlinearity. The shared parameter vocabulary adapts
//      its weighting per model, creating a unified but deeply varied instrument.
//
//  THE 4 ORGAN MODELS:
//      0. BAYAN — Russian concert accordion. Rich oscillators through a
//         cassotto resonance chamber (comb filter + allpass). Full-range,
//         powerful, commanding. The concert hall instrument.
//
//      1. HURDY-GURDY — Vielle a roue. Melody oscillator + 2 drone
//         oscillators + buzz bridge (trompette). The buzz bridge is the
//         SIGNATURE sound: waveshaper activates above velocity/pressure
//         threshold, creating iconic rhythmic buzzing. Wheel speed maps
//         to vibrato rate. Medieval, droning, hypnotic.
//
//      2. BANDONEON — Bisonoric: different notes on push vs pull of bellows.
//         Two oscillator banks (push/pull), velocity direction selects which
//         bank sounds. Rich, warm, melancholic tango character. The push/pull
//         mechanic is implemented via velocity threshold: soft touch = pull
//         (darker, warmer), hard touch = push (brighter, more forward).
//
//      3. GARMON — Russian diatonic accordion. Simpler reed model, fewer
//         harmonics, more buzz. Raw, folk, dance-driven, earthy character.
//         Emphasis on rhythmic attack and dance energy.
//
//  DSP ARCHITECTURE:
//      - Per-model oscillator topology (2-5 oscillators depending on model)
//      - Cassotto resonance chamber (Bayan: comb + allpass chain)
//      - Buzz bridge waveshaper (Hurdy-gurdy: BPF + cubic soft-clip + threshold)
//      - Bisonoric push/pull banks (Bandoneon: dual detuned osc sets)
//      - Bellows pressure simulation (shared: amplitude envelope with breathing)
//      - Reed/string resonance (per-model formant filter)
//      - Drone system (hurdy-gurdy: 2 fixed-pitch drone oscillators)
//
//  HISTORICAL LINEAGE:
//      Russian Bayan (Jupiter/Lefèvre tradition), French vielle a roue
//      (Pignol & Music 2014), Argentine bandoneon (Heinrich Band 1847,
//      Astor Piazzolla revolution), Russian garmon (Тульская гармонь).
//      Buzz bridge physics from Music & Science 2018 (Music, Pignol, Viaud).
//
//  Accent: Orthodox Gold #C5A036 (Baltic amber meets Orthodox iconography)
//  Parameter prefix: oleg_
//
//  Chef: Oleg — the third of the Chef Quad
//  Region: Baltic/Eastern Europe
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/StandardLFO.h"
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
// Organ Model Constants
//==============================================================================
enum OlegOrganModel : int
{
    Bayan = 0,
    HurdyGurdy = 1,
    Bandoneon = 2,
    Garmon = 3
};

//==============================================================================
// OlegCassotto — Bayan concert accordion cassotto resonance chamber.
//
// The cassotto is a wooden tone chamber inside the Bayan that gives it its
// distinctive warm, powerful, rounded sound. Reeds mounted inside the cassotto
// have their higher harmonics absorbed by the chamber walls while the fundamental
// and low harmonics are reinforced by the chamber's resonance.
//
// DSP: Short comb filter (chamber length) + 2 allpass filters (diffusion) +
// lowpass (wall absorption). The comb delay is tuned to reinforce ~200-400 Hz
// (the "body" of the Bayan sound).
//==============================================================================
struct OlegCassotto
{
    static constexpr int kMaxDelay = 2048;

    void prepare(float sampleRate) noexcept
    {
        sr = sampleRate;
        std::fill(combBuf.begin(), combBuf.end(), 0.0f);
        std::fill(ap1Buf.begin(), ap1Buf.end(), 0.0f);
        std::fill(ap2Buf.begin(), ap2Buf.end(), 0.0f);
        combWritePos = 0;
        ap1WritePos = 0;
        ap2WritePos = 0;

        // Chamber resonance: ~3ms delay (300 Hz fundamental)
        combDelaySamples = static_cast<int>(sr * 0.003f);
        if (combDelaySamples < 1)
            combDelaySamples = 1;
        if (combDelaySamples >= kMaxDelay)
            combDelaySamples = kMaxDelay - 1;

        // Allpass diffusion delays: shorter, prime-ish
        ap1DelaySamples = static_cast<int>(sr * 0.0011f);
        ap2DelaySamples = static_cast<int>(sr * 0.0017f);
        if (ap1DelaySamples < 1)
            ap1DelaySamples = 1;
        if (ap2DelaySamples < 1)
            ap2DelaySamples = 1;
        if (ap1DelaySamples >= kMaxDelay)
            ap1DelaySamples = kMaxDelay - 1;
        if (ap2DelaySamples >= kMaxDelay)
            ap2DelaySamples = kMaxDelay - 1;

        // Wall absorption LP
        wallFilter.setMode(CytomicSVF::Mode::LowPass);
        wallFilter.setCoefficients(3500.0f, 0.3f, sampleRate);
        wallFilter.reset();
    }

    float process(float input, float depth) noexcept
    {
        if (depth < 0.001f)
            return input;

        // Comb filter: read delayed sample
        int combReadPos = (combWritePos - combDelaySamples + kMaxDelay) % kMaxDelay;
        float combOut = combBuf[combReadPos];

        // Feedback with wall absorption
        float absorbed = wallFilter.processSample(combOut);
        combBuf[combWritePos] = input + absorbed * 0.55f;
        combWritePos = (combWritePos + 1) % kMaxDelay;

        // Allpass 1
        int ap1ReadPos = (ap1WritePos - ap1DelaySamples + kMaxDelay) % kMaxDelay;
        float ap1Delayed = ap1Buf[ap1ReadPos];
        float ap1Out = -input * 0.5f + ap1Delayed;
        ap1Buf[ap1WritePos] = input + ap1Delayed * 0.5f;
        ap1WritePos = (ap1WritePos + 1) % kMaxDelay;

        // Allpass 2
        int ap2ReadPos = (ap2WritePos - ap2DelaySamples + kMaxDelay) % kMaxDelay;
        float ap2Delayed = ap2Buf[ap2ReadPos];
        float ap2Out = -ap1Out * 0.5f + ap2Delayed;
        ap2Buf[ap2WritePos] = ap1Out + ap2Delayed * 0.5f;
        ap2WritePos = (ap2WritePos + 1) % kMaxDelay;

        float chamberOut = (combOut + ap2Out) * 0.5f;
        return input * (1.0f - depth) + chamberOut * depth;
    }

    void reset() noexcept
    {
        std::fill(combBuf.begin(), combBuf.end(), 0.0f);
        std::fill(ap1Buf.begin(), ap1Buf.end(), 0.0f);
        std::fill(ap2Buf.begin(), ap2Buf.end(), 0.0f);
        combWritePos = 0;
        ap1WritePos = 0;
        ap2WritePos = 0;
        wallFilter.reset();
    }

    float sr = 0.0f;  // Sentinel: must be set by prepare() before use
    std::array<float, kMaxDelay> combBuf{};
    std::array<float, kMaxDelay> ap1Buf{};
    std::array<float, kMaxDelay> ap2Buf{};
    int combWritePos = 0, ap1WritePos = 0, ap2WritePos = 0;
    int combDelaySamples = 144, ap1DelaySamples = 53, ap2DelaySamples = 82;
    CytomicSVF wallFilter;
};

//==============================================================================
// OlegBuzzBridge — Hurdy-gurdy trompette (buzz bridge) model.
//
// The trompette string on a hurdy-gurdy is set against a loose bridge (the
// chien, or "dog") that buzzes against the soundboard when the wheel pressure
// exceeds a threshold. The player uses wrist snaps (coups de poignet) to
// create rhythmic buzzing patterns. This is THE signature sound of the
// hurdy-gurdy — without it, it's just a drone instrument.
//
// DSP: The trompette signal is extracted via bandpass filter (100-800 Hz
// range, the buzz bridge resonance), then passed through a threshold-gated
// waveshaper. Below the threshold, the bridge sits quietly on the soundboard.
// Above it, cubic soft-clipping creates the characteristic rattle.
//
// Reference: Pignol & Music, "The hurdy-gurdy's buzz bridge" (2014)
//==============================================================================
struct OlegBuzzBridge
{
    void prepare(float sampleRate) noexcept
    {
        sr = sampleRate;
        buzzBPF.setMode(CytomicSVF::Mode::BandPass);
        buzzBPF.setCoefficients(350.0f, 0.8f, sampleRate);
        buzzBPF.reset();

        // Post-buzz resonance (the bridge rattling against the table)
        rattleBPF.setMode(CytomicSVF::Mode::BandPass);
        rattleBPF.setCoefficients(600.0f, 1.5f, sampleRate);
        rattleBPF.reset();
    }

    // buzzIntensity: 0-1, controls the buzz amount
    // pressure: 0-1, current bellows/wheel pressure (from velocity + aftertouch)
    // threshold: 0-1, the activation threshold for the buzz bridge
    float process(float input, float buzzIntensity, float pressure, float threshold) noexcept
    {
        if (buzzIntensity < 0.001f)
            return input;

        // Extract the buzz-bridge frequency band
        float buzzBand = buzzBPF.processSample(input);

        // Threshold gate: buzz only activates above pressure threshold
        // This is the key mechanic — the chien lifts off the soundboard
        float gateAmount = 0.0f;
        if (pressure > threshold)
        {
            float excess = (pressure - threshold) / (1.0f - threshold + 0.001f);
            gateAmount = excess * excess; // quadratic onset — smooth activation
        }

        // Waveshaper: cubic soft-clip creates the rattle character
        // The buzz bridge hits the soundboard repeatedly — each impact is
        // a nonlinear event, creating odd harmonics and sub-harmonics
        float drive = 3.0f + buzzIntensity * 12.0f;
        float shaped = softClip(buzzBand * drive * gateAmount);

        // Post-buzz resonance (the table/soundboard ringing)
        float rattleOut = rattleBPF.processSample(shaped);

        // Mix: original signal + buzz artifacts + rattle
        return input + (shaped * 0.6f + rattleOut * 0.4f) * buzzIntensity * gateAmount;
    }

    void reset() noexcept
    {
        buzzBPF.reset();
        rattleBPF.reset();
    }

    float sr = 0.0f;  // Sentinel: must be set by prepare() before use
    CytomicSVF buzzBPF;
    CytomicSVF rattleBPF;
};

//==============================================================================
// OlegReedOscillator — Multi-model reed/string oscillator.
//
// Generates the raw tone for each organ model. The waveform character varies:
//   Bayan: Rich saw + pulse mix (free-reed, cassotto-ready)
//   Hurdy-gurdy: Sawtooth-ish (bowed string via wheel)
//   Bandoneon: Warm triangle + saw mix (free-reed, softer than Bayan)
//   Garmon: Buzzy square-ish (simpler reed, more harmonics, rawer)
//
// Each model can have multiple oscillators per voice for richness:
//   Bayan: 2 detuned oscillators (unison richness)
//   Hurdy-gurdy: 1 melody + 2 drones (fixed or tracking pitch)
//   Bandoneon: 2 oscillator banks (push vs pull — different detune/character)
//   Garmon: 1 main + 1 slightly detuned (folk beating)
//==============================================================================
struct OlegReedOscillator
{
    // PolyBLEP oscillators for anti-aliased sawtooth and square waveforms.
    // Bayan uses saw1/saw2/saw3 (three-oscillator unison), Garmon uses sq1/sq2/saw1,
    // HurdyGurdy melody is saw1.
    // Drone strings remain as naive phase accumulators (sub-Nyquist at their pitch).
    PolyBLEP saw1, saw2, saw3, sq1, sq2;

    void reset() noexcept
    {
        phase1 = 0.0f;
        phase2 = 0.0f;
        phase3 = 0.0f;
        drone1Phase = 0.0f;
        drone2Phase = 0.0f;
        saw1.reset();
        saw2.reset();
        saw3.reset();
        sq1.reset();
        sq2.reset();
    }

    // Generate one sample for the given model.
    // freq: fundamental frequency in Hz
    // model: 0=Bayan, 1=HurdyGurdy, 2=Bandoneon, 3=Garmon
    // detune: cents of detuning for richness (0-50)
    // droneLevel: 0-1, level of drone oscillators (hurdy-gurdy only)
    // droneInterval1/2: semitone intervals for drone pitches
    // isPush: true = push bellows (bandoneon), false = pull
    // wheelSpeed: 0-1, hurdy-gurdy wheel speed → vibrato rate
    float process(float freq, int model, float detune, float droneLevel, float droneInterval1, float droneInterval2,
                  bool isPush, float wheelSpeed, float sampleRate) noexcept
    {
        float inc1 = freq / sampleRate;
        float detuneFactor = fastPow2(detune / 1200.0f);
        float inc2 = freq * detuneFactor / sampleRate;
        float inc3 = freq / (detuneFactor) / sampleRate; // opposite detune

        float out = 0.0f;

        switch (model)
        {
        case OlegOrganModel::Bayan:
        {
            // Rich saw + asymmetric pulse mix, 3-oscillator unison:
            //   saw1 = fundamental, saw2 = detuned up (+detune cents),
            //   saw3 = detuned down (−detune cents, i.e. freq / detuneFactor).
            // PolyBLEP anti-aliased sawtooth fixes aliasing above A4-A5.
            saw1.setWaveform(PolyBLEP::Waveform::Saw);
            saw2.setWaveform(PolyBLEP::Waveform::Saw);
            saw3.setWaveform(PolyBLEP::Waveform::Saw);
            saw1.setFrequency(freq, sampleRate);
            saw2.setFrequency(freq * detuneFactor, sampleRate);
            saw3.setFrequency(freq / detuneFactor, sampleRate);
            float sawOut1 = saw1.processSample();
            float sawOut2 = saw2.processSample();
            float sawOut3 = saw3.processSample();

            // Pulse component (reed opening/closing asymmetry).
            // Naive phase accumulator is acceptable here — it provides a low-level
            // body component that adds reed texture without the sharp discontinuities.
            phase1 += inc1;
            if (phase1 >= 1.0f)
                phase1 -= 1.0f;
            float pulse1 = (phase1 < 0.45f) ? 1.0f : -1.0f; // asymmetric reed gate

            // Advance phase3 with inc3 (opposite-detune oscillator).
            phase3 += inc3;
            if (phase3 >= 1.0f)
                phase3 -= 1.0f;

            out = sawOut1 * 0.35f + sawOut2 * 0.25f + sawOut3 * 0.25f + pulse1 * 0.15f;
            break;
        }

        case OlegOrganModel::HurdyGurdy:
        {
            // Melody string: wheel-bowed sawtooth with vibrato.
            // PolyBLEP saw for the melody (main aliasing offender above A4).
            // Drone strings remain as naive saws — they pitch-track well below Nyquist.
            float vibRate = 3.0f + wheelSpeed * 8.0f;      // 3-11 Hz vibrato
            float vibDepth = 0.002f + wheelSpeed * 0.008f; // subtle to moderate
            float vibMod = fastSin(vibratoPhase * 6.28318530718f) * vibDepth;
            vibratoPhase += vibRate / sampleRate;
            if (vibratoPhase >= 1.0f)
                vibratoPhase -= 1.0f;

            float melodyFreq = freq * (1.0f + vibMod);
            saw1.setWaveform(PolyBLEP::Waveform::Saw);
            saw1.setFrequency(melodyFreq, sampleRate);
            float melodySaw = saw1.processSample();

            // Drone strings: fixed pitch intervals (naive — sub-Nyquist)
            float drone1Freq = freq * fastPow2(droneInterval1 / 12.0f);
            float drone2Freq = freq * fastPow2(droneInterval2 / 12.0f);
            float drone1Inc = drone1Freq / sampleRate;
            float drone2Inc = drone2Freq / sampleRate;

            float drone1Saw = 2.0f * drone1Phase - 1.0f;
            float drone2Saw = 2.0f * drone2Phase - 1.0f;

            out = melodySaw * 0.5f + drone1Saw * droneLevel * 0.3f + drone2Saw * droneLevel * 0.2f;

            drone1Phase += drone1Inc;
            drone2Phase += drone2Inc;
            if (drone1Phase >= 1.0f)
                drone1Phase -= 1.0f;
            if (drone2Phase >= 1.0f)
                drone2Phase -= 1.0f;
            break;
        }

        case OlegOrganModel::Bandoneon:
        {
            // Bisonoric: push and pull produce different timbres.
            // Push: brighter, more forward, triangle-saw blend
            // Pull: warmer, darker, rounder triangle
            // The velocity determines push/pull — this is the unique mechanic.
            if (isPush)
            {
                // Push bank: triangle + saw blend, brighter
                float tri1 = 4.0f * std::fabs(phase1 - 0.5f) - 1.0f;
                float saw2 = 2.0f * phase2 - 1.0f;
                out = tri1 * 0.5f + saw2 * 0.5f;

                phase1 += inc1;
                phase2 += inc2; // slightly detuned for richness
            }
            else
            {
                // Pull bank: rounder, warmer, more detuned pair
                float tri1 = 4.0f * std::fabs(phase1 - 0.5f) - 1.0f;
                float tri2 = 4.0f * std::fabs(phase3 - 0.5f) - 1.0f;

                // Pull reeds are typically a bit flatter (wider detune)
                float pullDetuneInc = freq * fastPow2(detune * 1.5f / 1200.0f) / sampleRate;
                out = tri1 * 0.55f + tri2 * 0.45f;

                phase1 += inc1;
                phase3 += pullDetuneInc;
            }

            if (phase1 >= 1.0f)
                phase1 -= 1.0f;
            if (phase2 >= 1.0f)
                phase2 -= 1.0f;
            if (phase3 >= 1.0f)
                phase3 -= 1.0f;
            break;
        }

        case OlegOrganModel::Garmon:
        {
            // Buzzy reed: PolyBLEP asymmetric pulse + saw for grittiness.
            // Asymmetric duty (0.35) gives garmon's distinctive buzzy character
            // without aliasing edge artifacts above A4-A5.
            sq1.setWaveform(PolyBLEP::Waveform::Pulse);
            sq1.setPulseWidth(0.35f); // asymmetric duty for garmon character
            sq1.setFrequency(freq, sampleRate);

            sq2.setWaveform(PolyBLEP::Waveform::Pulse);
            sq2.setPulseWidth(0.35f);
            sq2.setFrequency(freq * detuneFactor, sampleRate);

            saw1.setWaveform(PolyBLEP::Waveform::Saw);
            saw1.setFrequency(freq, sampleRate);

            float sqOut1 = sq1.processSample();
            float sqOut2 = sq2.processSample();
            float sawOut1 = saw1.processSample();

            out = sqOut1 * 0.45f + sqOut2 * 0.3f + sawOut1 * 0.25f;
            break;
        }
        }

        return out;
    }

    float phase1 = 0.0f, phase2 = 0.0f, phase3 = 0.0f;
    float drone1Phase = 0.0f, drone2Phase = 0.0f;
    float vibratoPhase = 0.0f;
};

//==============================================================================
// OlegBellowsEnvelope — Simulates bellows/wheel pressure dynamics.
//
// Unlike a standard ADSR, bellows instruments have a continuous breath-like
// pressure that affects both amplitude and timbre. The bellows can run out
// of air (the "bellows reversal" in accordion playing), creating natural
// phrasing. This envelope provides:
//   - Smooth attack (bellows opening / wheel contact)
//   - Sustained pressure with optional breathing modulation
//   - Soft release (bellows closing / wheel slowing)
//   - Bellows pressure affects filter brightness (more air = brighter)
//==============================================================================
struct OlegBellowsEnvelope
{
    enum class Stage
    {
        Idle,
        Attack,
        Sustain,
        Release
    };

    void prepare(float sampleRate) noexcept
    {
        sr = sampleRate;
        recalc();
    }

    void setTimes(float attackSec, float releaseSec, float sustainLevel) noexcept
    {
        atkTime = std::max(attackSec, 0.001f);
        relTime = std::max(releaseSec, 0.005f);
        susLevel = std::clamp(sustainLevel, 0.0f, 1.0f);
        recalc();
    }

    void trigger() noexcept { stage = Stage::Attack; }

    void release() noexcept
    {
        if (stage != Stage::Idle)
            stage = Stage::Release;
    }

    void kill() noexcept
    {
        level = 0.0f;
        stage = Stage::Idle;
    }

    float process() noexcept
    {
        switch (stage)
        {
        case Stage::Idle:
            return 0.0f;

        case Stage::Attack:
            level += attackRate;
            if (level >= 1.0f)
            {
                level = 1.0f;
                stage = Stage::Sustain;
            }
            return level;

        case Stage::Sustain:
            // Bellows sustain: maintain pressure with slight breathing
            level += (susLevel - level) * 0.0001f;
            level = flushDenormal(level);
            return level;

        case Stage::Release:
            level *= releaseCoeff;
            level = flushDenormal(level);
            if (level < 1e-5f)
            {
                level = 0.0f;
                stage = Stage::Idle;
            }
            return level;
        }
        return 0.0f;
    }

    bool isActive() const noexcept { return stage != Stage::Idle; }
    float getLevel() const noexcept { return level; }

    Stage stage = Stage::Idle;
    float level = 0.0f;

private:
    float sr = 0.0f;  // Sentinel: must be set by prepare() before use
    float atkTime = 0.02f;
    float relTime = 0.15f;
    float susLevel = 0.85f;
    float attackRate = 0.0f;
    float releaseCoeff = 0.999f;

    void recalc() noexcept
    {
        if (sr <= 0.0f)
            return;
        attackRate = 1.0f / (sr * atkTime);
        releaseCoeff = std::exp(-4.6f / (sr * relTime));
    }
};

//==============================================================================
// OlegVoice — Per-voice state for the 4 organ models.
//==============================================================================
struct OlegVoice
{
    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    float velocity = 0.0f;
    bool isPush = true; // Bandoneon: push or pull bellows direction

    GlideProcessor glide;
    OlegReedOscillator osc;
    OlegBellowsEnvelope bellowsEnv;
    FilterEnvelope filterEnv;
    OlegCassotto cassotto;
    OlegBuzzBridge buzzBridge;
    CytomicSVF voiceFilter;
    CytomicSVF formantFilter; // model-specific formant resonance

    // Per-voice LFOs
    StandardLFO lfo1, lfo2;
    StandardLFO breathingLFO; // D005: autonomous breathing

    // Per-voice pressure tracking (aftertouch + velocity + mod wheel blend)
    float pressure = 0.0f;
    float pressureSmoothed = 0.0f;

    // Pan position (computed at note-on, cached)
    float panL = 0.707f, panR = 0.707f;

    // Voice-steal crossfade (5ms fade-out of stolen voice amplitude)
    float stealFadeGain = 0.0f; // 1.0 → 0.0 ramp applied to voice output on steal
    float stealFadeStep = 0.0f; // decrement per sample = 1 / (0.005 * sr)

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        isPush = true;
        pressure = 0.0f;
        pressureSmoothed = 0.0f;
        stealFadeGain = 0.0f;
        stealFadeStep = 0.0f;
        glide.reset();
        osc.reset();
        bellowsEnv.kill();
        filterEnv.kill();
        cassotto.reset();
        buzzBridge.reset();
        voiceFilter.reset();
        formantFilter.reset();
        lfo1.reset();
        lfo2.reset();
        breathingLFO.reset();
    }
};

//==============================================================================
// OlegEngine — "The Sacred Bellows" (Chef Quad #3)
//==============================================================================
class OlegEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    juce::String getEngineId() const override { return "Oleg"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFC0392B); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load(); }

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float>(sr);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            voices[i].reset();
            voices[i].cassotto.prepare(srf);
            voices[i].buzzBridge.prepare(srf);
            voices[i].bellowsEnv.prepare(srf);
            voices[i].filterEnv.prepare(srf);
            voices[i].breathingLFO.setShape(StandardLFO::Sine);
            voices[i].breathingLFO.setRate(0.08f + static_cast<float>(i) * 0.01f, srf); // D005: ~12s cycle
            voices[i].breathingLFO.setPhaseOffset(static_cast<float>(i) / static_cast<float>(kMaxVoices));
        }

        smoothBuzz.prepare(srf);
        smoothBrightness.prepare(srf);
        smoothDrone.prepare(srf);
        smoothBellows.prepare(srf);
        smoothDetune.prepare(srf);
        smoothFormant.prepare(srf);

        prepareSilenceGate(sr, maxBlockSize, 300.0f); // moderate hold — sustained organ sounds
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();
        pitchBendNorm = 0.0f;
        modWheelAmount = 0.0f;
        aftertouchAmount = 0.0f;
    }

    //==========================================================================
    // Coupling interface
    //==========================================================================

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
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
            couplingBuzzMod += val;
            break;
        default:
            break;
        }
    }

    //==========================================================================
    // Render — all 4 organ models in a unified per-voice loop
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        // Step 1: parse MIDI — wake gate BEFORE bypass check
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

        // Step 2: bypass check
        if (isSilenceGateBypassed())
        {
            buffer.clear(0, numSamples);
            couplingCacheL = couplingCacheR = 0.0f;
            return;
        }

        auto loadP = [](std::atomic<float>* p, float def) { return p ? p->load(std::memory_order_relaxed) : def; };

        // Read parameters
        const int pOrgan = static_cast<int>(loadP(paramOrgan, 0.0f));
        const float pBuzz = loadP(paramBuzz, 0.3f);
        const float pBrightness = loadP(paramBrightness, 6000.0f);
        const float pDrone = loadP(paramDrone, 0.0f);
        const float pBellows = loadP(paramBellows, 0.7f);
        const float pDetune = loadP(paramDetune, 15.0f);
        const float pFormant = loadP(paramFormant, 0.5f);
        const float pFilterEnvAmt = loadP(paramFilterEnvAmt, 0.3f);
        const float pBendRange = loadP(paramBendRange, 2.0f);
        const float pCassottoDepth = loadP(paramCassottoDepth, 0.5f);
        const float pBuzzThreshold = loadP(paramBuzzThreshold, 0.4f);
        const float pWheelSpeed = loadP(paramWheelSpeed, 0.3f);
        const float pDroneInt1 = loadP(paramDroneInterval1, -7.0f);
        const float pDroneInt2 = loadP(paramDroneInterval2, -12.0f);
        const float pGlideTime = loadP(paramGlideTime, 0.0f);

        const float macroCharacter = loadP(paramMacroCharacter, 0.0f);
        const float macroMovement = loadP(paramMacroMovement, 0.0f);
        const float macroCoupling = loadP(paramMacroCoupling, 0.0f);
        const float macroSpace = loadP(paramMacroSpace, 0.0f);

        // LFO params
        const float lfo1Rate = loadP(paramLfo1Rate, 0.5f);
        const float lfo1Depth = loadP(paramLfo1Depth, 0.0f);
        const int lfo1Shape = static_cast<int>(loadP(paramLfo1Shape, 0.0f));
        const float lfo2Rate = loadP(paramLfo2Rate, 1.0f);
        const float lfo2Depth = loadP(paramLfo2Depth, 0.0f);
        const int lfo2Shape = static_cast<int>(loadP(paramLfo2Shape, 0.0f));

        // Macro modulation targets:
        // CHARACTER → brightness + formant + buzz intensity
        // MOVEMENT → LFO depth + bellows breathing + wheel speed
        // COUPLING → coupling sensitivity
        // SPACE → cassotto depth + release time + detune
        float effectiveBuzz = std::clamp(pBuzz + macroCharacter * 0.4f + couplingBuzzMod, 0.0f, 1.0f);
        float effectiveBright = std::clamp(
            pBrightness + macroCharacter * 4000.0f + aftertouchAmount * 3000.0f + couplingFilterMod, 200.0f, 20000.0f);
        float effectiveDrone = std::clamp(pDrone + macroCoupling * 0.3f, 0.0f, 1.0f);
        float effectiveBellows = std::clamp(pBellows + macroMovement * 0.3f, 0.0f, 1.0f);
        float effectiveDetune = std::clamp(pDetune + macroSpace * 20.0f, 0.0f, 50.0f);
        float effectiveFormant = std::clamp(pFormant + macroCharacter * 0.3f, 0.0f, 1.0f);
        float effectiveCasDepth = std::clamp(pCassottoDepth + macroSpace * 0.3f, 0.0f, 1.0f);
        float effectiveWheelSpd = std::clamp(pWheelSpeed + macroMovement * 0.4f + modWheelAmount * 0.6f, 0.0f, 1.0f);

        smoothBuzz.set(effectiveBuzz);
        smoothBrightness.set(effectiveBright);
        smoothDrone.set(effectiveDrone);
        smoothBellows.set(effectiveBellows);
        smoothDetune.set(effectiveDetune);
        smoothFormant.set(effectiveFormant);

        // Reset coupling accumulators
        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;
        couplingBuzzMod = 0.0f;

        const float bendSemitones = pitchBendNorm * pBendRange;

        // Model-specific formant Q (reed/string resonance character, block-rate — no smoother for Q).
        // Formant frequency is computed per-sample inside the loop using smoothFormant.process()
        // (formantNow) so that parameter changes are zipper-free.
        float formantQ = 1.0f;
        switch (pOrgan)
        {
        case OlegOrganModel::Bayan:
            formantQ = 0.8f + effectiveFormant * 0.6f;
            break;
        case OlegOrganModel::HurdyGurdy:
            formantQ = 1.0f + effectiveFormant * 1.0f;
            break;
        case OlegOrganModel::Bandoneon:
            formantQ = 0.6f + effectiveFormant * 0.8f;
            break;
        case OlegOrganModel::Garmon:
            formantQ = 0.5f + effectiveFormant * 1.5f; // more resonant for folk character
            break;
        }

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        // Hoist LFO rate/shape configuration out of the per-sample loop.
        // These are block-rate parameters; calling setRate/setShape once per block
        // per voice is correct and avoids redundant coefficient updates each sample.
        for (auto& voice : voices)
        {
            if (!voice.active)
                continue;
            voice.lfo1.setRate(lfo1Rate, srf);
            voice.lfo1.setShape(lfo1Shape);
            voice.lfo2.setRate(lfo2Rate, srf);
            voice.lfo2.setShape(lfo2Shape);
        }

        for (int s = 0; s < numSamples; ++s)
        {
            float buzzNow = smoothBuzz.process();
            float brightNow = smoothBrightness.process();
            float droneNow = smoothDrone.process();
            float bellowsNow = smoothBellows.process();
            float detuneNow = smoothDetune.process();
            float formantNow = smoothFormant.process();

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                float freq = voice.glide.process();
                freq *= PitchBendUtil::semitonesToFreqRatio(bendSemitones + couplingPitchMod);

                // LFO processing (rate/shape already set once per block above)
                float lfo1Val = voice.lfo1.process() * lfo1Depth;
                float lfo2Val = voice.lfo2.process() * lfo2Depth;

                // D005: breathing modulation (always-on organic evolution)
                float breathMod = voice.breathingLFO.process();

                // LFO1 → pitch modulation (vibrato)
                freq *= PitchBendUtil::semitonesToFreqRatio(lfo1Val * 0.5f);

                // D005: breathing → subtle pitch drift
                freq *= 1.0f + breathMod * 0.001f;

                // Update pressure tracking (smooth for continuous control)
                float targetPressure = std::clamp(
                    voice.velocity * bellowsNow + aftertouchAmount * 0.4f + modWheelAmount * 0.3f, 0.0f, 1.0f);
                voice.pressureSmoothed += (targetPressure - voice.pressureSmoothed) * 0.001f;
                voice.pressureSmoothed = flushDenormal(voice.pressureSmoothed);
                voice.pressure = voice.pressureSmoothed;

                // Glide time update
                voice.glide.setTime(pGlideTime, srf);

                // Generate raw oscillator output
                float raw = voice.osc.process(freq, pOrgan, detuneNow, droneNow, pDroneInt1, pDroneInt2, voice.isPush,
                                              effectiveWheelSpd, srf);

                // D001: velocity shapes timbre — harder velocity = brighter initial filter sweep
                float velBright = voice.velocity * voice.velocity * 3000.0f;

                // Model-specific processing
                float processed = raw;

                switch (pOrgan)
                {
                case OlegOrganModel::Bayan:
                {
                    // Cassotto resonance chamber
                    processed = voice.cassotto.process(raw, effectiveCasDepth);

                    // Formant filter: Bayan has a warm, rounded resonance.
                    // formantNow is the per-sample smoothed formant parameter — use it
                    // directly so the smoother actually drives the coefficient each sample.
                    voice.formantFilter.setMode(CytomicSVF::Mode::Peak);
                    voice.formantFilter.setCoefficients(600.0f + formantNow * 2400.0f, formantQ, srf);
                    processed = raw * 0.6f + voice.formantFilter.processSample(processed) * 0.4f;
                    break;
                }

                case OlegOrganModel::HurdyGurdy:
                {
                    // Buzz bridge — THE signature sound
                    // Buzz activates above pressure threshold
                    processed = voice.buzzBridge.process(raw, buzzNow, voice.pressure, pBuzzThreshold);

                    // Wooden body resonance (formant).
                    // formantNow is the per-sample smoothed formant parameter.
                    voice.formantFilter.setMode(CytomicSVF::Mode::Peak);
                    voice.formantFilter.setCoefficients(400.0f + formantNow * 1600.0f, formantQ, srf);
                    processed = processed * 0.7f + voice.formantFilter.processSample(processed) * 0.3f;
                    break;
                }

                case OlegOrganModel::Bandoneon:
                {
                    // Bandoneon: warm tango character, slight saturation
                    float warmSat = fastTanh(processed * (1.0f + buzzNow * 2.0f));
                    processed = processed * (1.0f - buzzNow * 0.5f) + warmSat * buzzNow * 0.5f;

                    // Reed chamber resonance.
                    // formantNow is the per-sample smoothed formant parameter.
                    voice.formantFilter.setMode(CytomicSVF::Mode::Peak);
                    voice.formantFilter.setCoefficients(500.0f + formantNow * 2500.0f, formantQ, srf);
                    processed = processed * 0.65f + voice.formantFilter.processSample(processed) * 0.35f;
                    break;
                }

                case OlegOrganModel::Garmon:
                {
                    // Garmon: raw, buzzy, folk character — more aggressive nonlinearity
                    float rawBuzz = softClip(processed * (1.5f + buzzNow * 4.0f));
                    processed = processed * (1.0f - buzzNow * 0.6f) + rawBuzz * buzzNow * 0.6f;

                    // Open box resonance (narrower, more colored).
                    // formantNow is the per-sample smoothed formant parameter.
                    voice.formantFilter.setMode(CytomicSVF::Mode::BandPass);
                    voice.formantFilter.setCoefficients(350.0f + formantNow * 1650.0f, formantQ, srf);
                    processed = processed * 0.5f + voice.formantFilter.processSample(processed) * 0.5f;
                    break;
                }
                }

                // Bellows amplitude envelope
                float ampLevel = voice.bellowsEnv.process();
                if (ampLevel < 1e-6f)
                {
                    voice.active = false;
                    continue;
                }

                // Bellows pressure → amplitude modulation (breathing)
                float bellowsAmp = 0.5f + voice.pressure * 0.5f;
                // D005: add breathing LFO to bellows
                bellowsAmp *= 1.0f + breathMod * 0.05f;

                // Filter envelope (D001: velocity scales filter sweep)
                float filterEnvLevel = voice.filterEnv.process();
                float filterMod = filterEnvLevel * pFilterEnvAmt * velBright;

                // LFO2 → filter cutoff modulation
                float cutoff =
                    std::clamp(brightNow + filterMod + lfo2Val * 2000.0f + voice.pressure * 1500.0f, 200.0f, 20000.0f);

                // Per-model voice filter Q — each instrument has a distinct resonance character:
                //   Bayan Q=0.4  (concert cassotto chamber, warmly resonant)
                //   HurdyGurdy Q=0.5 (emphasize buzz overtones through wooden body)
                //   Bandoneon Q=0.3 (warm tango — keep current, no coloration needed)
                //   Garmon Q=0.6  (raw, resonant folk accordion character)
                static constexpr float kModelFilterQ[4] = {0.4f, 0.5f, 0.3f, 0.6f};
                float voiceFilterQ = kModelFilterQ[std::clamp(pOrgan, 0, 3)];

                voice.voiceFilter.setMode(CytomicSVF::Mode::LowPass);
                voice.voiceFilter.setCoefficients(cutoff, voiceFilterQ, srf);
                float filtered = voice.voiceFilter.processSample(processed);

                float output = filtered * ampLevel * bellowsAmp * voice.velocity;

                // Voice-steal crossfade: multiply output by stealFadeGain (1→0 over 5ms)
                // while the incoming note's envelope ramps up, avoiding a hard click.
                if (voice.stealFadeGain > 0.0f)
                {
                    output *= voice.stealFadeGain;
                    voice.stealFadeGain -= voice.stealFadeStep;
                    if (voice.stealFadeGain < 0.0f)
                        voice.stealFadeGain = 0.0f;
                }

                mixL += output * voice.panL;
                mixR += output * voice.panR;
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

        float freq = 440.0f * std::pow(2.0f, (static_cast<float>(note) - 69.0f) / 12.0f);
        int organ = paramOrgan ? static_cast<int>(paramOrgan->load()) : 0;

        // Voice steal: if this slot was already active, begin a 5ms crossfade from its
        // current amplitude down to 0 before the new note takes over.
        if (v.active)
        {
            v.stealFadeGain = v.bellowsEnv.getLevel() * 0.5f + v.pressure * 0.5f;
            v.stealFadeGain = std::clamp(v.stealFadeGain, 0.0f, 1.0f);
            v.stealFadeStep = 1.0f / (0.005f * srf);
        }
        else
        {
            v.stealFadeGain = 0.0f;
            v.stealFadeStep = 0.0f;
        }

        v.active = true;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.pressure = vel;
        v.pressureSmoothed = vel;

        // Bandoneon bisonoric mechanic: velocity determines push/pull
        // Low velocity (< 0.5) = pull (warmer, darker)
        // High velocity (>= 0.5) = push (brighter, more forward)
        v.isPush = (vel >= 0.5f);

        // Glide: snap for first note of voice, glide for legato
        float glideTime = paramGlideTime ? paramGlideTime->load() : 0.0f;
        if (glideTime > 0.001f)
            v.glide.setTargetOrSnap(freq);
        else
            v.glide.snapTo(freq);
        v.glide.setTime(glideTime, srf);

        v.osc.reset();

        // Bellows envelope — model-specific attack/release
        float atkTime = paramAttack ? paramAttack->load() : 0.02f;
        float relTime = paramRelease ? paramRelease->load() : 0.15f;

        switch (organ)
        {
        case OlegOrganModel::Bayan:
            v.bellowsEnv.setTimes(atkTime, relTime, 0.9f); // strong sustain
            break;
        case OlegOrganModel::HurdyGurdy:
            v.bellowsEnv.setTimes(atkTime * 2.0f, relTime * 1.5f, 0.85f); // wheel spin-up
            break;
        case OlegOrganModel::Bandoneon:
            v.bellowsEnv.setTimes(atkTime * 0.8f, relTime * 1.2f, 0.8f); // quick bellows
            break;
        case OlegOrganModel::Garmon:
            v.bellowsEnv.setTimes(atkTime * 0.5f, relTime * 0.8f, 0.85f); // snappy, dance
            break;
        }
        v.bellowsEnv.trigger();

        // Filter envelope — D001: velocity shapes filter decay
        v.filterEnv.prepare(srf);
        float filterDecay = 0.1f + (1.0f - vel) * 0.4f; // fast at high velocity
        v.filterEnv.setADSR(0.005f, filterDecay, 0.2f, 0.3f);
        v.filterEnv.triggerHard();

        // Cassotto preparation (Bayan-specific, but kept warm for other models)
        v.cassotto.prepare(srf);
        v.buzzBridge.prepare(srf);
        v.voiceFilter.reset();
        v.formantFilter.reset();

        // Breathing LFO phase stagger
        v.breathingLFO.reset(static_cast<float>(idx) / static_cast<float>(kMaxVoices));

        // Pan: slight stereo spread across voices
        float panAngle = (static_cast<float>(idx) / static_cast<float>(kMaxVoices) - 0.5f) * 0.6f;
        float panRad = (panAngle + 0.5f) * 1.5707963f;
        v.panL = std::cos(panRad);
        v.panR = std::sin(panRad);
    }

    void noteOff(int note) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && v.currentNote == note)
            {
                v.bellowsEnv.release();
                v.filterEnv.release();
            }
        }
    }

    //==========================================================================
    // Parameters — 30 total
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;

        // Core organ selector
        params.push_back(std::make_unique<PI>(juce::ParameterID{"oleg_organ", 1}, "Oleg Organ Model", 0, 3,
                                              0)); // 0=Bayan, 1=HurdyGurdy, 2=Bandoneon, 3=Garmon

        // Shared parameters with Oleg-specific weighting
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_buzz", 1}, "Oleg Buzz Intensity",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_brightness", 1}, "Oleg Brightness",
                                              juce::NormalisableRange<float>(200.0f, 20000.0f, 0.0f, 0.3f), 6000.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_drone", 1}, "Oleg Drone Level",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_bellows", 1}, "Oleg Bellows Pressure",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_detune", 1}, "Oleg Detune",
                                              juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f), 15.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_formant", 1}, "Oleg Formant",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        // Envelope times
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_attack", 1}, "Oleg Attack",
                                              juce::NormalisableRange<float>(0.001f, 1.0f, 0.0f, 0.3f), 0.02f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_release", 1}, "Oleg Release",
                                              juce::NormalisableRange<float>(0.005f, 3.0f, 0.0f, 0.4f), 0.15f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_filterEnvAmt", 1}, "Oleg Filter Env Amount",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Pitch
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_bendRange", 1}, "Oleg Pitch Bend Range",
                                              juce::NormalisableRange<float>(1.0f, 24.0f, 1.0f), 2.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_glideTime", 1}, "Oleg Glide Time",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 0.4f), 0.0f));

        // Model-specific parameters
        // Bayan: cassotto depth
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_cassottoDepth", 1}, "Oleg Cassotto Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        // Hurdy-gurdy: buzz threshold, wheel speed
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_buzzThreshold", 1}, "Oleg Buzz Threshold",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_wheelSpeed", 1}, "Oleg Wheel Speed",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        // Hurdy-gurdy: drone intervals (in semitones)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_droneInterval1", 1}, "Oleg Drone Interval 1",
                                              juce::NormalisableRange<float>(-24.0f, 0.0f, 1.0f),
                                              -7.0f)); // default: 5th below
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_droneInterval2", 1}, "Oleg Drone Interval 2",
                                              juce::NormalisableRange<float>(-24.0f, 0.0f, 1.0f),
                                              -12.0f)); // default: octave below

        // Macros
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_macroCharacter", 1}, "Oleg Macro CHARACTER",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_macroMovement", 1}, "Oleg Macro MOVEMENT",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_macroCoupling", 1}, "Oleg Macro COUPLING",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_macroSpace", 1}, "Oleg Macro SPACE",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // LFOs — D002 compliance
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_lfo1Rate", 1}, "Oleg LFO1 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_lfo1Depth", 1}, "Oleg LFO1 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.1f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"oleg_lfo1Shape", 1}, "Oleg LFO1 Shape", 0, 4, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_lfo2Rate", 1}, "Oleg LFO2 Rate",
                                              juce::NormalisableRange<float>(0.005f, 20.0f, 0.0f, 0.3f), 1.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oleg_lfo2Depth", 1}, "Oleg LFO2 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"oleg_lfo2Shape", 1}, "Oleg LFO2 Shape", 0, 4, 0));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        paramOrgan = apvts.getRawParameterValue("oleg_organ");
        paramBuzz = apvts.getRawParameterValue("oleg_buzz");
        paramBrightness = apvts.getRawParameterValue("oleg_brightness");
        paramDrone = apvts.getRawParameterValue("oleg_drone");
        paramBellows = apvts.getRawParameterValue("oleg_bellows");
        paramDetune = apvts.getRawParameterValue("oleg_detune");
        paramFormant = apvts.getRawParameterValue("oleg_formant");
        paramAttack = apvts.getRawParameterValue("oleg_attack");
        paramRelease = apvts.getRawParameterValue("oleg_release");
        paramFilterEnvAmt = apvts.getRawParameterValue("oleg_filterEnvAmt");
        paramBendRange = apvts.getRawParameterValue("oleg_bendRange");
        paramGlideTime = apvts.getRawParameterValue("oleg_glideTime");
        paramCassottoDepth = apvts.getRawParameterValue("oleg_cassottoDepth");
        paramBuzzThreshold = apvts.getRawParameterValue("oleg_buzzThreshold");
        paramWheelSpeed = apvts.getRawParameterValue("oleg_wheelSpeed");
        paramDroneInterval1 = apvts.getRawParameterValue("oleg_droneInterval1");
        paramDroneInterval2 = apvts.getRawParameterValue("oleg_droneInterval2");
        paramMacroCharacter = apvts.getRawParameterValue("oleg_macroCharacter");
        paramMacroMovement = apvts.getRawParameterValue("oleg_macroMovement");
        paramMacroCoupling = apvts.getRawParameterValue("oleg_macroCoupling");
        paramMacroSpace = apvts.getRawParameterValue("oleg_macroSpace");
        paramLfo1Rate = apvts.getRawParameterValue("oleg_lfo1Rate");
        paramLfo1Depth = apvts.getRawParameterValue("oleg_lfo1Depth");
        paramLfo1Shape = apvts.getRawParameterValue("oleg_lfo1Shape");
        paramLfo2Rate = apvts.getRawParameterValue("oleg_lfo2Rate");
        paramLfo2Depth = apvts.getRawParameterValue("oleg_lfo2Depth");
        paramLfo2Shape = apvts.getRawParameterValue("oleg_lfo2Shape");
    }

private:
    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    float srf = 0.0f;  // Sentinel: must be set by prepare() before use

    std::array<OlegVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount{0};

    ParameterSmoother smoothBuzz, smoothBrightness, smoothDrone;
    ParameterSmoother smoothBellows, smoothDetune, smoothFormant;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;

    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f, couplingBuzzMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    // Parameter pointers
    std::atomic<float>* paramOrgan = nullptr;
    std::atomic<float>* paramBuzz = nullptr;
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramDrone = nullptr;
    std::atomic<float>* paramBellows = nullptr;
    std::atomic<float>* paramDetune = nullptr;
    std::atomic<float>* paramFormant = nullptr;
    std::atomic<float>* paramAttack = nullptr;
    std::atomic<float>* paramRelease = nullptr;
    std::atomic<float>* paramFilterEnvAmt = nullptr;
    std::atomic<float>* paramBendRange = nullptr;
    std::atomic<float>* paramGlideTime = nullptr;
    std::atomic<float>* paramCassottoDepth = nullptr;
    std::atomic<float>* paramBuzzThreshold = nullptr;
    std::atomic<float>* paramWheelSpeed = nullptr;
    std::atomic<float>* paramDroneInterval1 = nullptr;
    std::atomic<float>* paramDroneInterval2 = nullptr;
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
};

} // namespace xoceanus
