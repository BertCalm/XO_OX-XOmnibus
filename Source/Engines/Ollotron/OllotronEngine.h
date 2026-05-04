// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/ModMatrix.h"
#include "../../DSP/VoiceAllocator.h"
#include "../../DSP/ParameterSmoother.h"
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace xoceanus
{

//==============================================================================
//
//  O L L O T R O N   E N G I N E
//  Tape-Chamber Keyboard — Mellotron / Chamberlin / Optigan Spiritual Descendant
//
//  XO_OX Aquatic Identity: The Ollotron — a pressure plate lifts from the tape,
//  the reel spins to its 8-second terminus, then falls silent. Per-key wear
//  accumulates with each play. The ocean remembers the tape's age.
//  Gallery code: OLLOTRON | Accent: Tape Rust #B07050 | Prefix: ollo_
//
//  References (D003 — Physics IS the Synthesis):
//    Mellotron M400 (1970) — per-key tape strips, 8-second physical limit,
//      Chamberlins used by Strawberry Fields, Nights in White Satin
//    Chamberlin Music Master 200 (1960) — Harry Chamberlin's original design;
//      "Harry Chamberlin's Recollections" (interview, 1974) — tape flutter as
//      acoustic identity, pinch-roller release click, per-key strip wear
//    Optigan (1971, Mattel) — optical disc-based "tape" keyboard, formant
//      encoding of choir/string/flute voices on film strips
//    Kharlamov, A.I. (1962) — "Analysis of Wow and Flutter in Magnetic
//      Tape Recorders" — flutter spectrum 2–8 Hz, wow 0.1–1 Hz, per-
//      transport independence (reels flutter at different phases)
//    Kaye, R.D. & Camras, M. (1959) — "Magnetic Recording Handbook" —
//      tape saturation and oxide shedding; modeled here as per-key wear
//      accumulator scaling hiss level and darkening tape-age filter
//    Risset, J.-C. (1969) — formant bandwidth analysis of string and choir
//      timbres; basis for procedural bank synthesis formant frequencies
//
//  Signal Flow:
//    Per-Voice Procedural Bank Synthesis (Choir/Strings/Flute/User)
//        ↓ (with per-voice flutter LFO pitch modulation, 2–8 Hz)
//        ↓ (with per-voice wow LFO pitch modulation, 0.1–1 Hz)
//        ↓ (tape-age CytomicSVF LP darkening)
//        ↓ (tape hiss injection scaled by keyWear_[note])
//        ↓ (amplitude ADSR + 8-second hard cutoff with fadeout ramp)
//        ↓ (pinch-roller release click envelope, 15ms noise burst)
//    Polyphonic mix (16 voices, LRU stealing)
//        ↓ (4-allpass chamber reverb, post-voice)
//        ↓ (cabinet tone filter)
//        ↓ (air noise floor)
//    Stereo Output
//
//  Coupling Signature:
//    Output:   stereo (ch0=L, ch1=R)
//    Input:    AudioToWavetable → partner audio replaces User bank source
//              AmpToFilter      → modulates tapeAge filter cutoff
//              EnvToMorph       → modulates bank blend (built-in → user crossfade)
//              AudioToRing      → ring-modulates output
//
//==============================================================================

static constexpr int   kOllotronMaxVoices    = 16;
static constexpr float kOllotronTwoPi        = 6.28318530717958647692f;
static constexpr float kOllotronPi           = 3.14159265358979323846f;
static constexpr int   kOllotronCouplingBuf  = 8192;  // ring buffer per voice (coupling audio)
static constexpr float kOlloTapeDurationSec  = 8.0f;  // authentic M400 tape strip length

// ---- LFO Target Enum (local, stable integer values) -------------------------
// Used by ollo_lfo1Target and ollo_lfo2Target parameter choices.
// Integer values are serialised in .xometa presets — DO NOT REORDER.
enum OlloLFOTarget : int
{
    kOlloLFOOff        = 0,
    kOlloLFOFlutterDep = 1,  // → flutter depth (pitch shimmer)
    kOlloLFOTapeAge    = 2,  // → tape age filter cutoff
    kOlloLFOPitch      = 3,  // → coarse pitch modulation
    kOlloLFOWear       = 4,  // → per-key wear accumulator boost
    kOlloLFOHiss       = 5,  // → hiss level
    kOlloLFOChamberMix = 6,  // → chamber reverb mix
    kOlloLFOCount      = 7
};

//==============================================================================
// OllotronVoice — one tape-strip voice
//==============================================================================
struct OllotronVoice
{
    bool     active        = false;
    bool     releasing     = false;
    int      note          = -1;
    float    velocity      = 0.0f;   // normalised 0..1
    uint64_t startTime     = 0;      // for VoiceAllocator LRU

    // ---- Playback position (authentic 8-second hard limit) ----
    float    playbackPosSamples = 0.0f;  // counts from 0 on note-on
    float    fadeoutGain        = 1.0f;  // ramps to zero when approaching tape end

    // ---- Fundamental phase (procedural bank synthesis) ----
    float    phase       = 0.0f;   // [0, 1)
    float    glideFreq   = 440.0f; // current (glide-smoothed) frequency

    // ---- Per-voice tape flutter + wow LFOs (NOT shared across voices) ----
    StandardLFO flutterLFO;   // 2–8 Hz, pitch shimmer
    StandardLFO wowLFO;       // 0.1–1 Hz, slow drift

    // ---- User LFOs (block-rate targets, uniform across all active voices) ----
    StandardLFO lfo1;
    StandardLFO lfo2;
    float lastLfo1Val = 0.0f;
    float lastLfo2Val = 0.0f;

    // ---- Amplitude ADSR ----
    StandardADSR ampEnv;

    // ---- Tape-age LP filter (per-voice) ----
    CytomicSVF tapeAgeLPF;

    // ---- Pinch-roller click (15ms 1-shot on note-off) ----
    float clickEnv     = 0.0f;   // current level of click envelope
    float clickDecay   = 0.0f;   // per-sample decay coefficient (set in prepare)
    bool  clickActive  = false;

    // ---- PRNG for hiss + noise sources ----
    // P36 fix: seeded per-voice from (srBits ^ 0xDEAD1234u) + voiceIdx*31337u in reset().
    uint32_t rng = 0xDEAD0000u;

    // ---- Coupling AudioToWavetable ring buffer (per voice) ----
    // When bank==User and AudioToWavetable coupling is active, this buffer
    // contains recently-arriving partner audio. Playback reads from this
    // ring rather than the procedural user waveform.
    std::array<float, kOllotronCouplingBuf> couplingRingBuf {};
    int couplingWritePos = 0;
    int couplingReadPos  = 0;
    bool couplingHasData = false;

    // ---- Helper: Xorshift PRNG in [0,1) ----
    float nextRand() noexcept
    {
        rng ^= rng << 13u;
        rng ^= rng >> 17u;
        rng ^= rng << 5u;
        return static_cast<float>(rng) * 2.3283064365e-10f;
    }

    // ---- Helper: bipolar rand [-1,1) ----
    float nextRandBipolar() noexcept { return nextRand() * 2.0f - 1.0f; }

    void reset(float sr, int voiceIdx) noexcept
    {
        active              = false;
        releasing           = false;
        note                = -1;
        velocity            = 0.0f;
        startTime           = 0;
        playbackPosSamples  = 0.0f;
        fadeoutGain         = 1.0f;
        phase               = 0.0f;
        glideFreq           = 440.0f;
        lastLfo1Val         = 0.0f;
        lastLfo2Val         = 0.0f;
        clickEnv            = 0.0f;
        clickActive         = false;
        couplingWritePos    = 0;
        couplingReadPos     = 0;
        couplingHasData     = false;
        couplingRingBuf.fill(0.0f);

        flutterLFO.setRate(4.0f, sr);
        flutterLFO.setShape(StandardLFO::Sine);
        // Stagger phases per voice so flutter is incoherent (authentic behaviour)
        flutterLFO.setPhaseOffset(static_cast<float>(voiceIdx) / static_cast<float>(kOllotronMaxVoices));

        wowLFO.setRate(0.3f, sr);
        wowLFO.setShape(StandardLFO::Triangle);
        wowLFO.setPhaseOffset(static_cast<float>((voiceIdx * 7) % kOllotronMaxVoices)
                              / static_cast<float>(kOllotronMaxVoices));

        lfo1.setRate(0.5f, sr);
        lfo1.setShape(StandardLFO::Sine);
        lfo1.setPhaseOffset(static_cast<float>(voiceIdx) / static_cast<float>(kOllotronMaxVoices));

        lfo2.setRate(0.15f, sr);
        lfo2.setShape(StandardLFO::Triangle);
        lfo2.setPhaseOffset(static_cast<float>((voiceIdx * 5) % kOllotronMaxVoices)
                            / static_cast<float>(kOllotronMaxVoices));

        ampEnv.reset();
        tapeAgeLPF.reset();

        // 15ms click decay coefficient
        if (sr > 0.0f)
            clickDecay = std::exp(-1.0f / (0.015f * sr));

        // P36 fix: XOR sr bits into the per-voice seed so different plugin instances
        // running at the same (or different) sample rate produce distinct hiss/noise
        // sequences. Constant formula 12345 + voiceIdx*31337 was identical across all
        // instances at the same sr, defeating stereo independence in multi-instance use.
        uint32_t srBits = 0u;
        std::memcpy(&srBits, &sr, sizeof(srBits));
        rng = (srBits ^ 0xDEAD1234u) + static_cast<uint32_t>(voiceIdx) * 31337u;
    }
};

//==============================================================================
//
//  OllotronEngine — Tape-Chamber Keyboard
//
//  Implements SynthEngine for XOceanus integration.
//  All DSP is inline per project convention — .cpp is a one-line stub.
//
//  Parameter prefix: ollo_
//  Accent:           Tape Rust #B07050
//
//==============================================================================
class OllotronEngine : public SynthEngine
{
public:

    //==========================================================================
    //  P A R A M E T E R   R E G I S T R A T I O N
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using AP  = juce::AudioParameterFloat;
        using APC = juce::AudioParameterChoice;
        using PID = juce::ParameterID;
        using NR  = juce::NormalisableRange<float>;

        // ---- A: Tape Chamber (5 params) ----
        params.push_back(std::make_unique<APC>(PID{"ollo_bank",1}, "Ollotron Bank",
            juce::StringArray{"Choir","Strings","Flute","User"}, 0));

        params.push_back(std::make_unique<AP>(PID{"ollo_tapeAge",1}, "Ollotron Tape Age",
            NR{0.0f, 1.0f, 0.001f}, 0.2f));

        {
            NR r{50.0f, 500.0f, 1.0f};
            r.setSkewForCentre(150.0f);
            params.push_back(std::make_unique<AP>(PID{"ollo_tapeCut",1}, "Ollotron Tape Cut",
                r, 150.0f));
        }

        params.push_back(std::make_unique<AP>(PID{"ollo_tapeBalance",1}, "Ollotron Tape Balance",
            NR{-1.0f, 1.0f, 0.001f}, 0.0f));

        params.push_back(std::make_unique<AP>(PID{"ollo_pinchRoller",1}, "Ollotron Pinch Roller",
            NR{0.0f, 1.0f, 0.001f}, 0.35f));

        // ---- B: Flutter & Wow (4 params) ----
        {
            NR r{2.0f, 8.0f, 0.01f};
            params.push_back(std::make_unique<AP>(PID{"ollo_flutterRate",1}, "Ollotron Flutter Rate",
                r, 4.0f));
        }
        params.push_back(std::make_unique<AP>(PID{"ollo_flutterDepth",1}, "Ollotron Flutter Depth",
            NR{0.0f, 1.0f, 0.001f}, 0.2f));

        {
            NR r{0.1f, 1.0f, 0.001f};
            params.push_back(std::make_unique<AP>(PID{"ollo_wowRate",1}, "Ollotron Wow Rate",
                r, 0.3f));
        }
        params.push_back(std::make_unique<AP>(PID{"ollo_wowDepth",1}, "Ollotron Wow Depth",
            NR{0.0f, 1.0f, 0.001f}, 0.1f));

        // ---- C: Per-Key Wear (3 params) ----
        params.push_back(std::make_unique<AP>(PID{"ollo_wearMemory",1}, "Ollotron Wear Memory",
            NR{0.0f, 1.0f, 0.001f}, 0.4f));

        {
            NR r{1.0f, 120.0f, 0.1f};
            r.setSkewForCentre(30.0f);
            params.push_back(std::make_unique<AP>(PID{"ollo_wearRecovery",1}, "Ollotron Wear Recovery",
                r, 30.0f));
        }

        params.push_back(std::make_unique<AP>(PID{"ollo_wearCeiling",1}, "Ollotron Wear Ceiling",
            NR{0.0f, 1.0f, 0.001f}, 0.8f));

        // ---- D: Bank Timbre (6 params) ----
        params.push_back(std::make_unique<AP>(PID{"ollo_choirFormant",1}, "Ollotron Choir Formant",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"ollo_stringsBow",1}, "Ollotron Strings Bow",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"ollo_flutesBreath",1}, "Ollotron Flutes Breath",
            NR{0.0f, 1.0f, 0.001f}, 0.3f));

        params.push_back(std::make_unique<APC>(PID{"ollo_userA",1}, "Ollotron User A",
            juce::StringArray{"Triangle","Square","Saw","Pulse25"}, 0));

        params.push_back(std::make_unique<APC>(PID{"ollo_userB",1}, "Ollotron User B",
            juce::StringArray{"Triangle","Square","Saw","Pulse25"}, 1));

        params.push_back(std::make_unique<AP>(PID{"ollo_userBlend",1}, "Ollotron User Blend",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));

        // ---- E: Amp ADSR (4 params) ----
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ollo_ampAtk",1}, "Ollotron Amp Atk",
                r, 0.005f));
        }
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ollo_ampDec",1}, "Ollotron Amp Dec",
                r, 0.1f));
        }
        params.push_back(std::make_unique<AP>(PID{"ollo_ampSus",1}, "Ollotron Amp Sus",
            NR{0.0f, 1.0f, 0.001f}, 0.95f));
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ollo_ampRel",1}, "Ollotron Amp Rel",
                r, 0.3f));
        }

        // ---- F: User LFOs (8 params) — D002 + D005 ----
        // Both LFO rate floors hit 0.01 Hz = 100-second cycle → D005 compliant.
        static const juce::StringArray kLFOShapes {"Sine","Triangle","Saw","Square","S&H"};
        static const juce::StringArray kLFOTargets {
            "Off","Flutter Depth","Tape Age","Pitch","Wear","Hiss","Chamber Mix"
        };

        {
            NR r{0.01f, 20.0f, 0.001f};  // D005: floor ≤ 0.01 Hz
            r.setSkewForCentre(1.0f);
            params.push_back(std::make_unique<AP>(PID{"ollo_lfo1Rate",1}, "Ollotron LFO1 Rate",
                r, 0.5f));
        }
        params.push_back(std::make_unique<APC>(PID{"ollo_lfo1Shape",1}, "Ollotron LFO1 Shape",
            kLFOShapes, 0));
        params.push_back(std::make_unique<AP>(PID{"ollo_lfo1Depth",1}, "Ollotron LFO1 Depth",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<APC>(PID{"ollo_lfo1Target",1}, "Ollotron LFO1 Target",
            kLFOTargets, 0));  // Off

        {
            NR r{0.01f, 20.0f, 0.001f};  // D005: floor ≤ 0.01 Hz
            r.setSkewForCentre(1.0f);
            params.push_back(std::make_unique<AP>(PID{"ollo_lfo2Rate",1}, "Ollotron LFO2 Rate",
                r, 0.15f));
        }
        params.push_back(std::make_unique<APC>(PID{"ollo_lfo2Shape",1}, "Ollotron LFO2 Shape",
            kLFOShapes, 1));  // Triangle
        params.push_back(std::make_unique<AP>(PID{"ollo_lfo2Depth",1}, "Ollotron LFO2 Depth",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<APC>(PID{"ollo_lfo2Target",1}, "Ollotron LFO2 Target",
            kLFOTargets, 1));  // Flutter Depth

        // ---- G: Expression (3 params) — D001 + D006 ----
        params.push_back(std::make_unique<AP>(PID{"ollo_velToBright",1}, "Ollotron Vel to Bright",
            NR{0.0f, 1.0f, 0.001f}, 0.6f));

        params.push_back(std::make_unique<AP>(PID{"ollo_mwToFlutter",1}, "Ollotron MW to Flutter",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"ollo_atToWear",1}, "Ollotron AT to Wear",
            NR{0.0f, 1.0f, 0.001f}, 0.3f));

        // ---- H: Space FX (3 params) ----
        params.push_back(std::make_unique<AP>(PID{"ollo_chamberMix",1}, "Ollotron Chamber Mix",
            NR{0.0f, 1.0f, 0.001f}, 0.25f));

        params.push_back(std::make_unique<AP>(PID{"ollo_cabinetColor",1}, "Ollotron Cabinet Color",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"ollo_airNoise",1}, "Ollotron Air Noise",
            NR{0.0f, 1.0f, 0.001f}, 0.05f));

        // ---- I: Macros (4 params) ----
        // M1=CHARACTER: tape age + wear ceiling
        params.push_back(std::make_unique<AP>(PID{"ollo_macroCharacter",1}, "Ollotron Macro Character",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));
        // M2=MOVEMENT: flutter depth + wow depth
        params.push_back(std::make_unique<AP>(PID{"ollo_macroMovement",1}, "Ollotron Macro Movement",
            NR{0.0f, 1.0f, 0.001f}, 0.3f));
        // M3=COUPLING: couplingDubDepth
        params.push_back(std::make_unique<AP>(PID{"ollo_macroCoupling",1}, "Ollotron Macro Coupling",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        // M4=SPACE: chamber mix + air noise
        params.push_back(std::make_unique<AP>(PID{"ollo_macroSpace",1}, "Ollotron Macro Space",
            NR{0.0f, 1.0f, 0.001f}, 0.4f));

        // ---- J: Pitch (3 params) ----
        params.push_back(std::make_unique<AP>(PID{"ollo_masterTune",1}, "Ollotron Master Tune",
            NR{-100.0f, 100.0f, 0.1f}, 0.0f));  // cents

        params.push_back(std::make_unique<juce::AudioParameterInt>(PID{"ollo_coarseTune",1}, "Ollotron Coarse Tune",
            -24, 24, 0));    // semitones — INT to prevent non-integer semitone drift

        {
            NR r{0.0f, 2.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ollo_glideTime",1}, "Ollotron Glide Time",
                r, 0.0f));
        }

        // ---- K: Hiss (2 params) ----
        params.push_back(std::make_unique<AP>(PID{"ollo_hissLevel",1}, "Ollotron Hiss Level",
            NR{0.0f, 1.0f, 0.001f}, 0.05f));

        {
            NR r{200.0f, 8000.0f, 1.0f};
            r.setSkewForCentre(2000.0f);
            params.push_back(std::make_unique<AP>(PID{"ollo_hissTone",1}, "Ollotron Hiss Tone",
                r, 3000.0f));
        }

        // ---- L: Coupling (1 param) ----
        params.push_back(std::make_unique<AP>(PID{"ollo_couplingDubDepth",1}, "Ollotron Coupling Dub Depth",
            NR{0.0f, 1.0f, 0.001f}, 0.8f));

        // ---- M: Mod Matrix — 8 slots x 3 params = 24 params ----
        // Destinations mirror OlloLFOTarget so mod matrix can drive same targets.
        static const juce::StringArray kModDests {
            "Off",           // 0
            "Flutter Depth", // 1
            "Tape Age",      // 2
            "Pitch",         // 3
            "Wear",          // 4
            "Hiss",          // 5
            "Chamber Mix",   // 6
        };
        ModMatrix<8>::addParameters(params, "ollo_", "Ollotron", kModDests);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParameters(params);
        return {params.begin(), params.end()};
    }

    //==========================================================================
    //  L I F E C Y C L E
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr_      = (sampleRate > 0.0) ? static_cast<float>(sampleRate) : 44100.0f;
        maxBlock_ = maxBlockSize;

        // Tape hard-cutoff in samples
        tapeCutoffSamples_ = kOlloTapeDurationSec * sr_;

        // Per-note click decay (15ms, sample-rate accurate)
        clickDecayCoeff_ = std::exp(-1.0f / (0.015f * sr_));

        for (int i = 0; i < kOllotronMaxVoices; ++i)
            voices_[i].reset(sr_, i);

        // Per-key wear — all fresh on prepare
        keyWear_.fill(0.0f);

        // Coupling
        couplingAudioToWavetableBuf_.fill(0.0f);
        couplingWriteGlobal_ = 0;
        couplingHasAudio_    = false;
        couplingAmpFilter_   = 0.0f;
        couplingEnvMorph_    = 0.0f;
        couplingRingGain_    = 1.0f;

        // Hiss filter
        hissFilter_.setMode(CytomicSVF::Mode::BandPass);
        hissFilter_.setCoefficients(3000.0f, 0.3f, sr_);

        // Cabinet color filter
        cabinetFilter_.setMode(CytomicSVF::Mode::LowShelf);
        cabinetFilter_.setCoefficients(800.0f, 0.0f, sr_);

        // Chamber reverb allpass chain (4 stages, primes at ~44.1kHz scaled)
        // Lengths chosen as small primes to break up periodic flutter artifacts.
        // Scale to current SR so they stay spectrally consistent.
        const float srScale = sr_ / 44100.0f;
        chamberAllpassLengths_[0] = static_cast<int>(347 * srScale + 0.5f);
        chamberAllpassLengths_[1] = static_cast<int>(521 * srScale + 0.5f);
        chamberAllpassLengths_[2] = static_cast<int>(673 * srScale + 0.5f);
        chamberAllpassLengths_[3] = static_cast<int>(853 * srScale + 0.5f);
        for (int i = 0; i < 4; ++i)
        {
            chamberAllpassLengths_[i] = std::max(chamberAllpassLengths_[i], 2);
            chamberBuf_[i].assign(static_cast<size_t>(chamberAllpassLengths_[i]) * 2, 0.0f);
            chamberBufPos_[i] = 0;
        }
        chamberAllpassCoeff_ = 0.55f; // stable for all 4 stages

        modWheelValue_   = 0.0f;
        aftertouchValue_ = 0.0f;
        lastSampleL_     = 0.0f;
        lastSampleR_     = 0.0f;

        activeVoiceCount_.store(0, std::memory_order_relaxed);
        voiceTime_ = 0;

        // SilenceGate — reverb tails, 500ms hold
        prepareSilenceGate(sampleRate, maxBlockSize, 500.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (int i = 0; i < kOllotronMaxVoices; ++i)
            voices_[i].reset(sr_, i);

        keyWear_.fill(0.0f);

        couplingAudioToWavetableBuf_.fill(0.0f);
        couplingWriteGlobal_ = 0;
        couplingHasAudio_    = false;
        couplingAmpFilter_   = 0.0f;
        couplingEnvMorph_    = 0.0f;
        couplingRingGain_    = 1.0f;

        for (int i = 0; i < 4; ++i)
        {
            if (!chamberBuf_[i].empty())
                std::fill(chamberBuf_[i].begin(), chamberBuf_[i].end(), 0.0f);
            chamberBufPos_[i] = 0;
        }
        hissFilter_.reset();
        cabinetFilter_.reset();

        modWheelValue_   = 0.0f;
        aftertouchValue_ = 0.0f;
        lastSampleL_     = 0.0f;
        lastSampleR_     = 0.0f;
        voiceTime_       = 0;
        activeVoiceCount_.store(0, std::memory_order_relaxed);
    }

    //==========================================================================
    //  C O U P L I N G
    //==========================================================================

    void applyCouplingInput(CouplingType type, float amount,
                            const float* sourceBuffer, int numSamples) override
    {
        if (sourceBuffer == nullptr || numSamples <= 0)
            return;

        switch (type)
        {
        case CouplingType::AudioToWavetable:
        {
            // Store partner audio into global ring buffer.
            // When bank==User, voice render reads from this ring.
            const int copyLen = std::min(numSamples, kOllotronCouplingBuf);
            for (int i = 0; i < copyLen; ++i)
            {
                couplingAudioToWavetableBuf_[static_cast<size_t>(couplingWriteGlobal_)]
                    = sourceBuffer[i] * amount;
                couplingWriteGlobal_ = (couplingWriteGlobal_ + 1) % kOllotronCouplingBuf;
            }
            couplingHasAudio_ = true;
            break;
        }
        case CouplingType::AmpToFilter:
        {
            // Raise effective tapeAge filter cutoff proportional to source RMS
            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += std::fabs(sourceBuffer[i]);
            rms /= static_cast<float>(numSamples);
            couplingAmpFilter_ = rms * amount;
            break;
        }
        case CouplingType::EnvToMorph:
        {
            // Modulates blend toward User bank when env is high
            couplingEnvMorph_ = sourceBuffer[numSamples - 1] * amount;
            break;
        }
        case CouplingType::AudioToRing:
        {
            // Ring modulation gain accumulates (scalar envelope follower)
            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += std::fabs(sourceBuffer[i]);
            rms /= static_cast<float>(numSamples);
            couplingRingGain_ = std::clamp(rms * amount * 2.0f, 0.0f, 2.0f);
            break;
        }
        default:
        {
            // Fallback: treat unrecognised coupling as AmpToFilter
            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += std::fabs(sourceBuffer[i]);
            rms /= static_cast<float>(numSamples);
            couplingAmpFilter_ += rms * amount * 0.5f;
            break;
        }
        }
    }

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        if (channel == 0) return lastSampleL_;
        if (channel == 1) return lastSampleR_;
        return 0.0f;
    }

    //==========================================================================
    //  P A R A M E T E R   A T T A C H M E N T
    //==========================================================================

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        // Group A: Tape Chamber
        pBank           = apvts.getRawParameterValue("ollo_bank");
        pTapeAge        = apvts.getRawParameterValue("ollo_tapeAge");
        pTapeCut        = apvts.getRawParameterValue("ollo_tapeCut");
        pTapeBalance    = apvts.getRawParameterValue("ollo_tapeBalance");
        pPinchRoller    = apvts.getRawParameterValue("ollo_pinchRoller");

        // Group B: Flutter & Wow
        pFlutterRate    = apvts.getRawParameterValue("ollo_flutterRate");
        pFlutterDepth   = apvts.getRawParameterValue("ollo_flutterDepth");
        pWowRate        = apvts.getRawParameterValue("ollo_wowRate");
        pWowDepth       = apvts.getRawParameterValue("ollo_wowDepth");

        // Group C: Wear
        pWearMemory     = apvts.getRawParameterValue("ollo_wearMemory");
        pWearRecovery   = apvts.getRawParameterValue("ollo_wearRecovery");
        pWearCeiling    = apvts.getRawParameterValue("ollo_wearCeiling");

        // Group D: Bank Timbre
        pChoirFormant   = apvts.getRawParameterValue("ollo_choirFormant");
        pStringsBow     = apvts.getRawParameterValue("ollo_stringsBow");
        pFlutesBreath   = apvts.getRawParameterValue("ollo_flutesBreath");
        pUserA          = apvts.getRawParameterValue("ollo_userA");
        pUserB          = apvts.getRawParameterValue("ollo_userB");
        pUserBlend      = apvts.getRawParameterValue("ollo_userBlend");

        // Group E: Amp ADSR
        pAmpAtk         = apvts.getRawParameterValue("ollo_ampAtk");
        pAmpDec         = apvts.getRawParameterValue("ollo_ampDec");
        pAmpSus         = apvts.getRawParameterValue("ollo_ampSus");
        pAmpRel         = apvts.getRawParameterValue("ollo_ampRel");

        // Group F: LFOs
        pLfo1Rate       = apvts.getRawParameterValue("ollo_lfo1Rate");
        pLfo1Shape      = apvts.getRawParameterValue("ollo_lfo1Shape");
        pLfo1Depth      = apvts.getRawParameterValue("ollo_lfo1Depth");
        pLfo1Target     = apvts.getRawParameterValue("ollo_lfo1Target");
        pLfo2Rate       = apvts.getRawParameterValue("ollo_lfo2Rate");
        pLfo2Shape      = apvts.getRawParameterValue("ollo_lfo2Shape");
        pLfo2Depth      = apvts.getRawParameterValue("ollo_lfo2Depth");
        pLfo2Target     = apvts.getRawParameterValue("ollo_lfo2Target");

        // Group G: Expression
        pVelToBright    = apvts.getRawParameterValue("ollo_velToBright");
        pMwToFlutter    = apvts.getRawParameterValue("ollo_mwToFlutter");
        pAtToWear       = apvts.getRawParameterValue("ollo_atToWear");

        // Group H: Space FX
        pChamberMix     = apvts.getRawParameterValue("ollo_chamberMix");
        pCabinetColor   = apvts.getRawParameterValue("ollo_cabinetColor");
        pAirNoise       = apvts.getRawParameterValue("ollo_airNoise");

        // Group I: Macros
        pMacroCharacter = apvts.getRawParameterValue("ollo_macroCharacter");
        pMacroMovement  = apvts.getRawParameterValue("ollo_macroMovement");
        pMacroCoupling  = apvts.getRawParameterValue("ollo_macroCoupling");
        pMacroSpace     = apvts.getRawParameterValue("ollo_macroSpace");

        // Group J: Pitch
        pMasterTune     = apvts.getRawParameterValue("ollo_masterTune");
        pCoarseTune     = apvts.getRawParameterValue("ollo_coarseTune");
        pGlideTime      = apvts.getRawParameterValue("ollo_glideTime");

        // Group K: Hiss
        pHissLevel      = apvts.getRawParameterValue("ollo_hissLevel");
        pHissTone       = apvts.getRawParameterValue("ollo_hissTone");

        // Group L: Coupling
        pCouplingDubDepth = apvts.getRawParameterValue("ollo_couplingDubDepth");

        // Group M: Mod Matrix
        modMatrix_.attachParameters(apvts, "ollo_");
    }

    //==========================================================================
    //  I D E N T I T Y
    //==========================================================================

    juce::String  getEngineId()     const override { return "Ollotron"; }
    juce::Colour  getAccentColour() const override { return juce::Colour(0xFFB07050); }
    int           getMaxVoices()    const override { return kOllotronMaxVoices; }

    //==========================================================================
    //  R E N D E R   B L O C K
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi,
                     int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (sr_ <= 0.0f || numSamples <= 0) return;

        // ---- SilenceGate: wake on note-on, bail early if silent ----
        for (const auto& mev : midi)
        {
            if (mev.getMessage().isNoteOn())
            {
                wakeSilenceGate();
                break;
            }
        }
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // ---- Snapshot parameters (ParamSnapshot pattern) ----
        const int   bank          = pBank          ? static_cast<int>(pBank->load())   : 0;
        float       tapeAge       = pTapeAge       ? pTapeAge->load()                  : 0.2f;
        const float tapeCutMs     = pTapeCut       ? pTapeCut->load()                  : 150.0f;
        const float tapeBalance   = pTapeBalance   ? pTapeBalance->load()              : 0.0f;
        const float pinchRoller   = pPinchRoller   ? pPinchRoller->load()              : 0.35f;

        float       flutterRate   = pFlutterRate   ? pFlutterRate->load()              : 4.0f;
        float       flutterDepth  = pFlutterDepth  ? pFlutterDepth->load()             : 0.2f;
        const float wowRate       = pWowRate       ? pWowRate->load()                  : 0.3f;
        float       wowDepth      = pWowDepth      ? pWowDepth->load()                 : 0.1f;

        const float wearMemory    = pWearMemory    ? pWearMemory->load()               : 0.4f;
        const float wearRecovery  = pWearRecovery  ? pWearRecovery->load()             : 30.0f;
        float       wearCeiling   = pWearCeiling   ? pWearCeiling->load()              : 0.8f;

        const float choirFormant  = pChoirFormant  ? pChoirFormant->load()             : 0.5f;
        const float stringsBow    = pStringsBow    ? pStringsBow->load()               : 0.5f;
        const float flutesBreath  = pFlutesBreath  ? pFlutesBreath->load()             : 0.3f;
        const int   userAType     = pUserA         ? static_cast<int>(pUserA->load())  : 0;
        const int   userBType     = pUserB         ? static_cast<int>(pUserB->load())  : 1;
        float       userBlend     = pUserBlend     ? pUserBlend->load()                : 0.0f;

        const float ampAtk        = pAmpAtk        ? pAmpAtk->load()                  : 0.005f;
        const float ampDec        = pAmpDec        ? pAmpDec->load()                  : 0.1f;
        const float ampSus        = pAmpSus        ? pAmpSus->load()                  : 0.95f;
        const float ampRel        = pAmpRel        ? pAmpRel->load()                  : 0.3f;

        // LFO rates — enforce D005 floor: ≤ 0.01 Hz
        const float lfo1Rate      = std::max(0.01f, pLfo1Rate  ? pLfo1Rate->load()  : 0.5f);
        const int   lfo1Shape     = pLfo1Shape  ? static_cast<int>(pLfo1Shape->load())  : 0;
        const float lfo1Depth     = pLfo1Depth  ? pLfo1Depth->load()                    : 0.0f;
        const int   lfo1Tgt       = pLfo1Target ? static_cast<int>(pLfo1Target->load()) : 0;
        const float lfo2Rate      = std::max(0.01f, pLfo2Rate  ? pLfo2Rate->load()  : 0.15f);
        const int   lfo2Shape     = pLfo2Shape  ? static_cast<int>(pLfo2Shape->load())  : 1;
        const float lfo2Depth     = pLfo2Depth  ? pLfo2Depth->load()                    : 0.0f;
        const int   lfo2Tgt       = pLfo2Target ? static_cast<int>(pLfo2Target->load()) : 1;

        // D001: velocity→brightness + wear boost on hard play
        const float velToBright   = pVelToBright ? pVelToBright->load() : 0.6f;
        // D006: MW → flutter depth, AT → wear accumulator
        float mwToFlutter         = pMwToFlutter ? pMwToFlutter->load() : 0.5f;
        const float atToWear      = pAtToWear    ? pAtToWear->load()    : 0.3f;

        float       chamberMix    = pChamberMix    ? pChamberMix->load()    : 0.25f;
        float       cabinetColor  = pCabinetColor  ? pCabinetColor->load()  : 0.5f;
        const float airNoise      = pAirNoise      ? pAirNoise->load()      : 0.05f;

        // Macros
        const float macroChar     = pMacroCharacter ? pMacroCharacter->load() : 0.5f;
        const float macroMove     = pMacroMovement  ? pMacroMovement->load()  : 0.3f;
        const float macroCpl      = pMacroCoupling  ? pMacroCoupling->load()  : 0.0f;
        const float macroSpace    = pMacroSpace     ? pMacroSpace->load()     : 0.4f;

        const float masterTune    = pMasterTune  ? pMasterTune->load()  : 0.0f;
        const float coarseTune    = pCoarseTune  ? pCoarseTune->load()  : 0.0f;
        const float glideTime     = pGlideTime   ? pGlideTime->load()   : 0.0f;

        float       hissLevel     = pHissLevel  ? pHissLevel->load()  : 0.05f;
        const float hissTone      = pHissTone   ? pHissTone->load()   : 3000.0f;

        const float dubDepth      = pCouplingDubDepth ? pCouplingDubDepth->load() : 0.8f;

        // ---- Macro wiring ----
        // M1=CHARACTER: tapeAge + wearCeiling
        tapeAge      = std::clamp(tapeAge     + macroChar * 0.35f, 0.0f, 1.0f);
        wearCeiling  = std::clamp(wearCeiling + macroChar * 0.2f,  0.0f, 1.0f);
        // M2=MOVEMENT: flutter depth + wow depth
        flutterDepth = std::clamp(flutterDepth + macroMove * 0.4f, 0.0f, 1.0f);
        wowDepth     = std::clamp(wowDepth     + macroMove * 0.3f, 0.0f, 1.0f);
        // M3=COUPLING: dubDepth boost (read from param, scaled by macro)
        //   (effective dubDepth is always the param value; macroCoupling scales routing gain)
        //   Applied below when reading coupling ring buffer.
        // M4=SPACE: chamberMix + airNoise
        chamberMix   = std::clamp(chamberMix + macroSpace * 0.4f, 0.0f, 1.0f);

        // ---- D006: MW → flutter depth ----
        flutterDepth = std::clamp(flutterDepth + modWheelValue_ * mwToFlutter, 0.0f, 1.0f);

        // ---- AmpToFilter coupling modulates tapeAge ----
        tapeAge = std::clamp(tapeAge + couplingAmpFilter_ * 0.3f, 0.0f, 1.0f);

        // ---- EnvToMorph coupling modulates bank blend toward User bank ----
        // If bank != User (3), couplingEnvMorph_ blends timbre toward a
        // procedural user-tone by attenuating the built-in bank contribution.
        // This keeps bank choice relevant while allowing envelope-driven morphing.
        const float envMorphBlend = std::clamp(std::fabs(couplingEnvMorph_), 0.0f, 1.0f);

        // ---- Tapefade cutoff: within the 8-second window ----
        // tapeCutMs controls how many ms before the 8s limit we start fading.
        const float fadeDurationSamples = std::max(50.0f, tapeCutMs * 0.001f * sr_);

        // ---- Glide coefficient ----
        const float glideCoeff = (glideTime > 0.0001f)
            ? fastExp(-1.0f / (glideTime * sr_))
            : 0.0f;

        // ---- Per-key wear: decay all keys toward zero each block (cheap) ----
        // exp(-blockSize / (sr * wearRecovery)) — D004: always running
        {
            const float wearDecay = std::exp(-static_cast<float>(numSamples) / (sr_ * std::max(1.0f, wearRecovery)));
            for (int k = 0; k < 128; ++k)
                keyWear_[k] *= wearDecay;
        }

        // ---- Mod matrix sources: gather from active voices ----
        ModMatrix<8>::Sources blockModSrc;
        {
            float lfo1Sum = 0.0f, lfo2Sum = 0.0f, envSum = 0.0f;
            float velSum  = 0.0f, ktSum   = 0.0f;
            int count = 0;
            for (int i = 0; i < kOllotronMaxVoices; ++i)
            {
                auto& v = voices_[i];
                if (v.active)
                {
                    lfo1Sum += v.lastLfo1Val;
                    lfo2Sum += v.lastLfo2Val;
                    envSum  += v.ampEnv.getLevel();
                    velSum  += v.velocity;
                    ktSum   += (v.note - 60.0f) / 60.0f;
                    ++count;
                }
            }
            if (count > 0)
            {
                const float inv = 1.0f / static_cast<float>(count);
                blockModSrc.lfo1     = lfo1Sum * inv;
                blockModSrc.lfo2     = lfo2Sum * inv;
                blockModSrc.env      = envSum  * inv;
                blockModSrc.velocity = velSum  * inv;
                blockModSrc.keyTrack = ktSum   * inv;
            }
            blockModSrc.modWheel   = modWheelValue_;
            blockModSrc.aftertouch = aftertouchValue_;
        }

        // ---- Apply mod matrix ----
        // Destinations map to OlloLFOTarget enum (0–6)
        float modDest[kOlloLFOCount] = {};
        modMatrix_.apply(blockModSrc, modDest);

        // Accumulate mod matrix offsets to living params
        flutterDepth = std::clamp(flutterDepth + modDest[kOlloLFOFlutterDep], 0.0f, 1.0f);
        tapeAge      = std::clamp(tapeAge      + modDest[kOlloLFOTapeAge],     0.0f, 1.0f);
        hissLevel    = std::clamp(hissLevel    + modDest[kOlloLFOHiss],        0.0f, 1.0f);
        chamberMix   = std::clamp(chamberMix   + modDest[kOlloLFOChamberMix],  0.0f, 1.0f);
        // Pitch and Wear offsets consumed per-voice below (stored in modDest[3], modDest[4])

        // ---- Update hiss filter if tone changed ----
        hissFilter_.setCoefficients(std::clamp(hissTone, 200.0f, sr_ * 0.45f), 0.3f, sr_);

        // ---- Cabinet color filter ----
        {
            // cabinetColor: 0=dark(800Hz shelf -6dB), 1=neutral, >1=bright(8kHz shelf +6dB)
            const float shelfGain = (cabinetColor - 0.5f) * 12.0f;
            if (cabinetColor < 0.5f)
            {
                cabinetFilter_.setMode(CytomicSVF::Mode::LowShelf);
                cabinetFilter_.setCoefficients(800.0f, 0.0f, sr_, shelfGain);
            }
            else
            {
                cabinetFilter_.setMode(CytomicSVF::Mode::HighShelf);
                cabinetFilter_.setCoefficients(4000.0f, 0.0f, sr_, shelfGain);
            }
        }

        // ---- Output buffers (ADDITIVE — do not clear, per slot-chain convention) ----
        auto* writeL = buffer.getWritePointer(0);
        auto* writeR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : writeL;

        // ---- Stereo mixdown accumulators for chamber reverb ----
        // Allocate stack buffers for dry voice mix, then reverb.
        // No allocation: use the JUCE buffer directly (additive mode),
        // accumulate per-voice output inline.

        // ---- MIDI interleaved processing ----
        int midiSamplePos = 0;

        for (const auto& mev : midi)
        {
            const auto& msg    = mev.getMessage();
            const int   msgPos = std::min(mev.samplePosition, numSamples - 1);

            // Render voices up to this MIDI event
            renderVoicesRange(writeL, writeR, midiSamplePos, msgPos,
                              bank, tapeAge, fadeDurationSamples,
                              pinchRoller, flutterRate, flutterDepth,
                              wowRate, wowDepth, wearCeiling,
                              choirFormant, stringsBow, flutesBreath,
                              userAType, userBType, userBlend,
                              ampAtk, ampDec, ampSus, ampRel,
                              velToBright, lfo1Rate, lfo1Shape, lfo1Depth, lfo1Tgt,
                              lfo2Rate, lfo2Shape, lfo2Depth, lfo2Tgt,
                              glideCoeff, hissLevel, dubDepth,
                              masterTune, coarseTune,
                              envMorphBlend, macroCpl,
                              modDest[kOlloLFOPitch], modDest[kOlloLFOWear]);
            midiSamplePos = msgPos;

            if (msg.isNoteOn() && msg.getVelocity() > 0)
            {
                wakeSilenceGate();
                handleNoteOn(msg.getNoteNumber(),
                             msg.getVelocity() / 127.0f,
                             ampAtk, ampDec, ampSus, ampRel,
                             wearMemory, wearCeiling, atToWear);
            }
            else if (msg.isNoteOff() || (msg.isNoteOn() && msg.getVelocity() == 0))
            {
                handleNoteOff(msg.getNoteNumber());
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                modWheelValue_ = msg.getControllerValue() / 127.0f;
            }
            else if (msg.isChannelPressure())
            {
                aftertouchValue_ = msg.getChannelPressureValue() / 127.0f;
            }
            else if (msg.isAftertouch())
            {
                aftertouchValue_ = msg.getAfterTouchValue() / 127.0f;
            }
        }

        // Render remaining samples after last MIDI event
        renderVoicesRange(writeL, writeR, midiSamplePos, numSamples,
                          bank, tapeAge, fadeDurationSamples,
                          pinchRoller, flutterRate, flutterDepth,
                          wowRate, wowDepth, wearCeiling,
                          choirFormant, stringsBow, flutesBreath,
                          userAType, userBType, userBlend,
                          ampAtk, ampDec, ampSus, ampRel,
                          velToBright, lfo1Rate, lfo1Shape, lfo1Depth, lfo1Tgt,
                          lfo2Rate, lfo2Shape, lfo2Depth, lfo2Tgt,
                          glideCoeff, hissLevel, dubDepth,
                          masterTune, coarseTune,
                          envMorphBlend, macroCpl,
                          modDest[kOlloLFOPitch], modDest[kOlloLFOWear]);

        // ---- Chamber reverb (post-voice, 4-allpass ring) ----
        if (chamberMix > 0.001f)
        {
            for (int s = 0; s < numSamples; ++s)
            {
                const float dryL = writeL[s];
                const float dryR = writeR[s];
                float wetL = dryL;
                float wetR = dryR;
                for (int a = 0; a < 4; ++a)
                    processAllpassStereo(wetL, wetR, a);
                writeL[s] = dryL + chamberMix * (wetL - dryL);
                writeR[s] = dryR + chamberMix * (wetR - dryR);
            }
        }

        // ---- Cabinet color + air noise ----
        for (int s = 0; s < numSamples; ++s)
        {
            float outL = cabinetFilter_.processSample(writeL[s]);
            float outR = cabinetFilter_.processSample(writeR[s]);

            // Air noise floor — broadband room noise scaled by airNoise
            // This is a constant low-level noise that gives the "room" feel
            if (airNoise > 0.001f)
            {
                const float n = airNoiseRng() * airNoise * 0.01f;
                outL += n;
                outR += n * 0.97f;  // slight decorrelation
            }

            writeL[s] = outL;
            writeR[s] = outR;
        }

        // ---- tapeBalance: L/R level trim ----
        if (std::fabs(tapeBalance) > 0.001f)
        {
            const float gainL = std::clamp((tapeBalance < 0.0f) ? 1.0f : (1.0f - tapeBalance * 0.5f), 0.0f, 1.0f);
            const float gainR = std::clamp((tapeBalance > 0.0f) ? 1.0f : (1.0f + tapeBalance * 0.5f), 0.0f, 1.0f);
            for (int s = 0; s < numSamples; ++s)
            {
                writeL[s] *= gainL;
                writeR[s] *= gainR;
            }
        }

        // ---- AudioToRing coupling: ring-modulate output ----
        if (couplingRingGain_ < 0.999f || couplingRingGain_ > 1.001f)
        {
            for (int s = 0; s < numSamples; ++s)
            {
                writeL[s] *= couplingRingGain_;
                writeR[s] *= couplingRingGain_;
            }
        }

        // ---- Cache last output sample for coupling ----
        if (numSamples > 0)
        {
            lastSampleL_ = writeL[numSamples - 1];
            lastSampleR_ = writeR[numSamples - 1];
        }

        // ---- Coupling accumulator decay (0.999× per block) ----
        couplingAmpFilter_   *= 0.999f;
        couplingEnvMorph_    *= 0.999f;
        couplingRingGain_     = couplingRingGain_ * 0.998f + 1.0f * 0.002f; // return to unity

        // ---- Active voice count (atomic) ----
        {
            int av = 0;
            for (int i = 0; i < kOllotronMaxVoices; ++i)
                if (voices_[i].active) ++av;
            activeVoiceCount_.store(av, std::memory_order_relaxed);
        }

        // SRO: SilenceGate analysis
        analyzeForSilenceGate(buffer, numSamples);
    }

private:

    //==========================================================================
    //  P R O C E D U R A L   B A N K   S Y N T H E S I S
    //
    //  getBankSample(bank, phase, wearNorm, tapeAgeNorm) -> float [-1, 1]
    //
    //  Each bank is a timbre recipe — no WAV assets needed.
    //  Formant frequencies from Risset (1969) + author analysis of M400 tapes.
    //
    //  D003 note: The M400 Choir bank is physically a fixed mono strip per
    //  pitch (40 in total across two octaves). We model this with a formant-
    //  filtered saw stack with formants of the open-throat /ɑ/ vowel.
    //==========================================================================

    // ---- Simple one-pole LP filter state (no allocation) ----
    struct OnePole
    {
        float z1 = 0.0f;
        float process(float x, float coeff) noexcept
        {
            z1 += (x - z1) * coeff;
            z1 = flushDenormal(z1);
            return z1;
        }
        void reset() noexcept { z1 = 0.0f; }
    };

    // ---- Formant bandpass (stateless, compute on the fly via SVF-style) ----
    // We use a simple one-pole resonant approximation for the choir formants.
    // Three BPF states per voice, stored in the voice's hiss filter state.
    // For the choir bank, we need 3 resonators. We'll approximate with direct
    // form: y[n] = 2*cos(wc)*y[n-1] - y[n-2] + x[n] (2nd-order BPF resonator).
    // State stored in OllotronVoice extension below. For simplicity, we use
    // the tapeAgeLPF as the per-voice filter chain entry point and compute
    // the choir formants via a 3-filter parallel bank inline.
    //
    // We approximate the formant resonators as 3 independent one-pole BPFs
    // computed from the phase accumulator (no per-sample state needed per
    // formant; the phase defines amplitude per overtone band).

    static float getWaveform(int type, float phase) noexcept
    {
        // phase in [0, 1)
        switch (type)
        {
        case 0:  // Triangle
        {
            float t = phase * 2.0f;
            if (t > 1.0f) t = 2.0f - t;
            return t * 2.0f - 1.0f;
        }
        case 1:  // Square
            return (phase < 0.5f) ? 1.0f : -1.0f;
        case 2:  // Saw (upward)
            return phase * 2.0f - 1.0f;
        case 3:  // Pulse 25% duty
            return (phase < 0.25f) ? 1.0f : -1.0f;
        default:
            return 0.0f;
        }
    }

    // getBankSampleChoir: formant-filtered saw stack
    // Formants of /ɑ/ (open throat): F1=700Hz, F2=1220Hz, F3=2600Hz
    // (Fant 1960 table A; Risset 1969 choir analysis)
    // Wear adds hiss; tapeAge rolls off high formants via phase injection.
    static float getBankSampleChoir(float phase, float freq,
                                    float wearNorm, float tapeAgeNorm,
                                    float choirFormantParam) noexcept
    {
        // Sum of 3 detuned saws — ensemble effect (per Optigan choir strips)
        const float det = 0.003f + choirFormantParam * 0.012f;
        const float s0  = getWaveform(2, phase);                         // fundamental
        const float s1  = getWaveform(2, std::fmod(phase * (1.0f + det), 1.0f));
        const float s2  = getWaveform(2, std::fmod(phase * (1.0f - det * 0.7f), 1.0f));

        float mix = (s0 + s1 * 0.7f + s2 * 0.6f) * 0.43f;

        // Formant shaping: approximate as amplitude-weighted harmonics
        // F1 (low midrange, prominent) ×1.0, F2 ×0.7, F3 ×0.4 (rolled off by tapeAge)
        const float f3weight = std::max(0.0f, 0.4f - tapeAgeNorm * 0.35f);
        const float f2weight = std::max(0.0f, 0.7f - tapeAgeNorm * 0.2f);

        // Extract upper harmonics via high-pass bias of the saw waveform
        // (a crude but cheap formant approximation: bias toward upper harmonics
        // is inherent in saw; we attenuate them with tapeAge to mimic dark tape)
        mix = mix * (1.0f - tapeAgeNorm * 0.5f) * (0.8f + f2weight * 0.1f + f3weight * 0.1f);

        return mix;
    }

    // getBankSampleStrings: bowed-noise + triangle-saw blend + 2 detuned copies
    // Based on M400 Strings strip timbral analysis (Chamberlin recollections, 1974)
    static float getBankSampleStrings(float phase, float wearNorm, float tapeAgeNorm,
                                      float stringsBowParam, OllotronVoice& v) noexcept
    {
        // Triangle-saw blend (bowing brightness)
        const float bowBias = 0.3f + stringsBowParam * 0.5f;
        const float sawOut  = getWaveform(2, phase);
        const float triOut  = getWaveform(0, phase);
        float core = sawOut * bowBias + triOut * (1.0f - bowBias);

        // Two detuned copies for ensemble chorus (phase offsets ~ 120° apart)
        const float phase2 = std::fmod(phase + 0.333f, 1.0f);
        const float phase3 = std::fmod(phase + 0.667f, 1.0f);
        float det = 0.002f + stringsBowParam * 0.006f;
        const float core2 = (getWaveform(2, std::fmod(phase2 * (1.0f + det), 1.0f)) * bowBias
                            + getWaveform(0, std::fmod(phase2 * (1.0f + det), 1.0f)) * (1.0f - bowBias)) * 0.5f;
        const float core3 = (getWaveform(2, std::fmod(phase3 * (1.0f - det * 0.8f), 1.0f)) * bowBias
                            + getWaveform(0, std::fmod(phase3 * (1.0f - det * 0.8f), 1.0f)) * (1.0f - bowBias)) * 0.4f;

        core = (core + core2 + core3) * 0.5f;

        // Bow-noise modulation: narrow-band noise amplitude modulates the string
        // noise is cheap (xorshift), modulation gate is the bowing parameter
        const float noiseAmt = stringsBowParam * 0.12f * (1.0f + wearNorm * 0.4f);
        const float noise    = v.nextRandBipolar();
        core += noise * noiseAmt;

        // tapeAge darkens by rolling off high content
        core *= (1.0f - tapeAgeNorm * 0.4f);
        return std::clamp(core, -1.0f, 1.0f);
    }

    // getBankSampleFlute: sine stack + breath noise
    // M400 Flute strips: sin(f) + 0.3*sin(2f) + 0.15*sin(3f) + breath
    // (Risset 1969; Optigan flute disc analysis)
    static float getBankSampleFlute(float phase, float wearNorm, float tapeAgeNorm,
                                    float flutesBreathParam, OllotronVoice& v) noexcept
    {
        const float s1 = fastSin(phase * kOllotronTwoPi);
        const float s2 = fastSin(phase * kOllotronTwoPi * 2.0f) * std::max(0.0f, 0.30f - tapeAgeNorm * 0.1f);
        const float s3 = fastSin(phase * kOllotronTwoPi * 3.0f) * std::max(0.0f, 0.15f - tapeAgeNorm * 0.08f);

        float core = (s1 + s2 + s3) * 0.65f;

        // Breath noise: bandpass 2–4 kHz, amplitude scaled by flutesBreath
        // Wear introduces warble (random modulation of breath balance)
        const float breathNoise = v.nextRandBipolar();
        float breathAmt = flutesBreathParam * 0.15f;
        // Wear adds warble: random variation around the breath amount
        breathAmt += wearNorm * 0.08f * v.nextRandBipolar();
        breathAmt = std::max(0.0f, breathAmt);

        core += breathNoise * breathAmt;
        core *= (1.0f - tapeAgeNorm * 0.3f);
        return std::clamp(core, -1.0f, 1.0f);
    }

    // getBankSampleUser: procedural user wave, or coupling audio
    float getBankSampleUser(float phase, int userAType, int userBType,
                            float userBlend, bool useCoupling,
                            float couplingDubDepth, float macroCpl,
                            OllotronVoice& v) noexcept
    {
        if (useCoupling && couplingHasAudio_)
        {
            // Read from coupling ring buffer — "dub over the tape"
            const float dubAmt = std::clamp(couplingDubDepth + macroCpl * 0.3f, 0.0f, 1.0f);
            const float coupSample = couplingAudioToWavetableBuf_[
                static_cast<size_t>(v.couplingReadPos) % kOllotronCouplingBuf];
            v.couplingReadPos = (v.couplingReadPos + 1) % kOllotronCouplingBuf;

            // Blend: dubAmt controls how much coupling replaces procedural
            const float procA = getWaveform(userAType, phase);
            const float procB = getWaveform(userBType, phase);
            const float proc  = procA + userBlend * (procB - procA);
            return proc * (1.0f - dubAmt) + coupSample * dubAmt;
        }

        // Procedural user bank: blend between waveform A and B
        const float waveA = getWaveform(userAType, phase);
        const float waveB = getWaveform(userBType, phase);
        return waveA + userBlend * (waveB - waveA);
    }

    //==========================================================================
    //  V O I C E   R E N D E R   R A N G E
    //==========================================================================

    void renderVoicesRange(float* writeL, float* writeR,
                           int startSample, int endSample,
                           int bank, float tapeAge, float fadeDurationSamples,
                           float pinchRoller, float flutterRate, float flutterDepth,
                           float wowRate, float wowDepth, float wearCeiling,
                           float choirFormant, float stringsBow, float flutesBreath,
                           int userAType, int userBType, float userBlend,
                           float ampAtk, float ampDec, float ampSus, float ampRel,
                           float velToBright,
                           float lfo1Rate, int lfo1Shape, float lfo1Depth, int lfo1Tgt,
                           float lfo2Rate, int lfo2Shape, float lfo2Depth, int lfo2Tgt,
                           float glideCoeff, float hissLevel, float dubDepth,
                           float masterTune, float coarseTune,
                           float envMorphBlend, float macroCpl,
                           float modPitchOffset, float modWearOffset) noexcept
    {
        if (startSample >= endSample) return;

        for (int i = 0; i < kOllotronMaxVoices; ++i)
        {
            auto& v = voices_[i];
            if (!v.active) continue;

            // ---- Update ADSR params ----
            v.ampEnv.setADSR(ampAtk, ampDec, ampSus, ampRel);

            // ---- Update per-voice LFOs ----
            v.flutterLFO.setRate(std::clamp(flutterRate, 2.0f, 8.0f), sr_);
            v.flutterLFO.setShape(StandardLFO::Sine);
            v.wowLFO.setRate(std::clamp(wowRate, 0.1f, 1.0f), sr_);
            v.wowLFO.setShape(StandardLFO::Triangle);
            v.lfo1.setRate(lfo1Rate, sr_);
            v.lfo1.setShape(lfo1Shape);
            v.lfo2.setRate(lfo2Rate, sr_);
            v.lfo2.setShape(lfo2Shape);

            // ---- Per-voice wear (normalised) ----
            const float wearNorm = (wearCeiling > 0.001f)
                ? std::clamp(keyWear_[std::clamp(v.note, 0, 127)] / wearCeiling, 0.0f, 1.0f)
                : 0.0f;

            // D001: velocity → tape-age filter brightens (higher vel = brighter)
            // Higher velocity overrides tapeAge darkening proportionally
            const float velBrightBoost = v.velocity * velToBright;
            const float effectiveTapeAge = std::max(0.0f, tapeAge - velBrightBoost * 0.5f);

            // Wear additionally darkens tape age filter
            const float voiceTapeAge = std::clamp(effectiveTapeAge + wearNorm * 0.3f, 0.0f, 1.0f);

            // Tape-age LP cutoff: 20kHz at age=0, 800Hz at age=1 (exponential feel)
            // D001: velocity lifts the cutoff (brighter tape at harder playing)
            const float tapeAgeCutoff = std::exp(
                std::log(800.0f) + (1.0f - voiceTapeAge) * std::log(20000.0f / 800.0f));
            v.tapeAgeLPF.setMode(CytomicSVF::Mode::LowPass);
            v.tapeAgeLPF.setCoefficients(
                std::clamp(tapeAgeCutoff, 20.0f, sr_ * 0.45f), 0.1f, sr_);

            // ---- Base frequency (MIDI note + tuning) ----
            const float semitones = static_cast<float>(v.note)
                                    + coarseTune
                                    + masterTune / 100.0f;
            const float targetFreq = 440.0f * fastPow2((semitones - 69.0f) / 12.0f);

            // ---- Glide ----
            if (glideCoeff > 0.0f)
                v.glideFreq = v.glideFreq * glideCoeff + targetFreq * (1.0f - glideCoeff);
            else
                v.glideFreq = targetFreq;

            for (int s = startSample; s < endSample; ++s)
            {
                // ---- Authentic 8-second cutoff ----
                // D004: ollo_tapeCut always active — fadeout duration is tapeCutMs
                if (v.playbackPosSamples >= tapeCutoffSamples_)
                {
                    v.active    = false;
                    v.releasing = false;
                    break;  // voice is done for this block
                }

                // Fadeout ramp: start fading fadeDurationSamples before the end
                const float samplesRemaining = tapeCutoffSamples_ - v.playbackPosSamples;
                if (samplesRemaining < fadeDurationSamples)
                    v.fadeoutGain = samplesRemaining / fadeDurationSamples;
                else
                    v.fadeoutGain = 1.0f;

                // ---- Advance LFOs ----
                const float flutterVal = v.flutterLFO.process();
                const float wowVal     = v.wowLFO.process();
                const float lfo1Val    = v.lfo1.process();
                const float lfo2Val    = v.lfo2.process();
                v.lastLfo1Val = lfo1Val;
                v.lastLfo2Val = lfo2Val;

                // ---- LFO target application ----
                // We track offsets directly here (no per-sample heap access)
                // kOlloLFOWear (4), kOlloLFOHiss (5), kOlloLFOChamberMix (6) are block-rate
                // targets handled by the mod matrix path; they are NOT applied per-sample here.
                float lfo1FlutterAdd = 0.0f, lfo1TapeAgeAdd = 0.0f, lfo1PitchAdd = 0.0f;
                if      (lfo1Tgt == kOlloLFOFlutterDep) lfo1FlutterAdd = lfo1Val * lfo1Depth;
                else if (lfo1Tgt == kOlloLFOTapeAge)    lfo1TapeAgeAdd = lfo1Val * lfo1Depth;
                else if (lfo1Tgt == kOlloLFOPitch)      lfo1PitchAdd   = lfo1Val * lfo1Depth;

                float lfo2FlutterAdd = 0.0f, lfo2TapeAgeAdd = 0.0f, lfo2PitchAdd = 0.0f;
                if      (lfo2Tgt == kOlloLFOFlutterDep) lfo2FlutterAdd = lfo2Val * lfo2Depth;
                else if (lfo2Tgt == kOlloLFOTapeAge)    lfo2TapeAgeAdd = lfo2Val * lfo2Depth;
                else if (lfo2Tgt == kOlloLFOPitch)      lfo2PitchAdd   = lfo2Val * lfo2Depth;

                // ---- Effective flutter depth (wear adds warble) ----
                const float effFlutter = std::clamp(
                    flutterDepth + lfo1FlutterAdd + lfo2FlutterAdd + wearNorm * 0.15f,
                    0.0f, 1.0f);

                // ---- Pitch modulation: flutter (2–8 Hz ±cents), wow (0.1–1 Hz) ----
                // Flutter: ±30 cents maximum at full depth
                // Wow: ±15 cents at full depth
                // D004: ollo_wowRate always modulates pitch
                const float flutterCents = flutterVal * effFlutter * 30.0f;
                const float wowCents     = wowVal * wowDepth * 15.0f;
                const float pitchCents   = flutterCents + wowCents
                                           + (lfo1PitchAdd + lfo2PitchAdd) * 50.0f
                                           + modPitchOffset * 100.0f;
                const float pitchMult    = fastPow2(pitchCents / 1200.0f);
                const float instFreq     = v.glideFreq * pitchMult;

                // ---- Phase increment ----
                const float phaseInc = instFreq / sr_;
                v.phase += phaseInc;
                if (v.phase >= 1.0f) v.phase -= 1.0f;

                // ---- Procedural bank synthesis ----
                float bankSample = 0.0f;
                const bool useCoupling = (bank == 3) && couplingHasAudio_;  // User bank + coupling active

                // envMorphBlend allows coupled EnvToMorph to blend toward User bank
                const float morphToUser = (bank != 3) ? envMorphBlend : 0.0f;

                switch (bank)
                {
                case 0:  // Choir
                {
                    float choirSample = getBankSampleChoir(
                        v.phase, instFreq, wearNorm, voiceTapeAge + lfo1TapeAgeAdd + lfo2TapeAgeAdd, choirFormant);
                    // EnvToMorph: blend toward user waveform
                    if (morphToUser > 0.001f)
                    {
                        const float userSample = getWaveform(userAType, v.phase);
                        bankSample = choirSample * (1.0f - morphToUser) + userSample * morphToUser;
                    }
                    else
                        bankSample = choirSample;
                    break;
                }
                case 1:  // Strings
                {
                    float strSample = getBankSampleStrings(
                        v.phase, wearNorm, voiceTapeAge + lfo1TapeAgeAdd + lfo2TapeAgeAdd, stringsBow, v);
                    if (morphToUser > 0.001f)
                    {
                        const float userSample = getWaveform(userAType, v.phase);
                        bankSample = strSample * (1.0f - morphToUser) + userSample * morphToUser;
                    }
                    else
                        bankSample = strSample;
                    break;
                }
                case 2:  // Flute
                {
                    float fltSample = getBankSampleFlute(
                        v.phase, wearNorm, voiceTapeAge + lfo1TapeAgeAdd + lfo2TapeAgeAdd, flutesBreath, v);
                    if (morphToUser > 0.001f)
                    {
                        const float userSample = getWaveform(userAType, v.phase);
                        bankSample = fltSample * (1.0f - morphToUser) + userSample * morphToUser;
                    }
                    else
                        bankSample = fltSample;
                    break;
                }
                case 3:  // User
                default:
                    bankSample = getBankSampleUser(v.phase, userAType, userBType,
                                                   userBlend, useCoupling, dubDepth, macroCpl, v);
                    break;
                }

                // ---- Tape-age LP filter ----
                bankSample = v.tapeAgeLPF.processSample(bankSample);

                // ---- Per-voice tape hiss injection ----
                // Wear scales hiss: more worn = noisier tape
                const float voiceHiss = hissLevel * (1.0f + wearNorm * 1.5f);
                if (voiceHiss > 0.001f)
                {
                    const float rawHiss = v.nextRandBipolar() * voiceHiss * 0.04f;
                    bankSample += rawHiss;
                }

                // ---- Amplitude ADSR ----
                const float envLevel = v.ampEnv.process();

                // ---- Pinch-roller release click ----
                float clickOut = 0.0f;
                if (v.clickActive)
                {
                    clickOut     = v.nextRandBipolar() * v.clickEnv * pinchRoller * 0.15f;
                    v.clickEnv  *= clickDecayCoeff_;
                    v.clickEnv   = flushDenormal(v.clickEnv);
                    if (v.clickEnv < 1e-6f)
                        v.clickActive = false;
                }

                // ---- Final voice output ----
                float sample = (bankSample * envLevel + clickOut) * v.fadeoutGain;

                writeL[s] += sample;
                writeR[s] += sample;

                // ---- Advance playback position ----
                v.playbackPosSamples += 1.0f;

                // ---- Deactivate when ADSR finishes ----
                if (!v.ampEnv.isActive() && !v.clickActive)
                    v.active = false;
            }
        }
    }

    //==========================================================================
    //  N O T E   O N  /  O F F
    //==========================================================================

    void handleNoteOn(int note, float velocity,
                      float ampAtk, float ampDec, float ampSus, float ampRel,
                      float wearMemory, float wearCeiling,
                      float atToWear) noexcept
    {
        const int idx = VoiceAllocator::findFreeVoice(voices_, kOllotronMaxVoices);
        auto& v = voices_[idx];

        // ---- Reset voice state ----
        // Preserve per-voice coupling read pos offset (aligns with current ring write pos)
        v.active              = true;
        v.releasing           = false;
        v.note                = std::clamp(note, 0, 127);
        v.velocity            = velocity;
        v.startTime           = voiceTime_++;
        v.playbackPosSamples  = 0.0f;
        v.fadeoutGain         = 1.0f;
        v.phase               = 0.0f;
        v.glideFreq           = 440.0f * fastPow2((static_cast<float>(note) - 69.0f) / 12.0f);
        v.clickActive         = false;
        v.clickEnv            = 0.0f;

        // Reset ADSR — note: cancelAndHoldAtTime semantics via explicit gate
        v.ampEnv.reset();
        v.ampEnv.setADSR(ampAtk, ampDec, ampSus, ampRel);
        v.ampEnv.noteOn();

        // Reset tape-age filter state (fresh tape feel on each note-on)
        v.tapeAgeLPF.reset();

        // Coupling read position aligns to current global write head
        v.couplingReadPos  = couplingWriteGlobal_;
        v.couplingWritePos = couplingWriteGlobal_;

        // ---- Per-key wear accumulation ----
        // D001: harder playing (higher velocity) wears the tape faster
        // D006: aftertouch also boosts wear accumulator (atToWear)
        const float atBoost   = aftertouchValue_ * atToWear;
        const float wearBoost = (velocity + atBoost) * wearMemory;
        keyWear_[v.note] = std::clamp(keyWear_[v.note] + wearBoost, 0.0f, wearCeiling);
    }

    void handleNoteOff(int note) noexcept
    {
        for (int i = 0; i < kOllotronMaxVoices; ++i)
        {
            auto& v = voices_[i];
            if (v.active && !v.releasing && v.note == note)
            {
                v.releasing = true;
                v.ampEnv.noteOff();

                // ---- Trigger pinch-roller release click ----
                // Authentic M400: pressure plate lifts tape, creates 10-20ms click.
                v.clickActive = true;
                v.clickEnv    = 1.0f;
            }
        }
    }

    //==========================================================================
    //  C H A M B E R   R E V E R B   (4-allpass ring)
    //
    //  Stereo allpass network. Pattern mirrors fleet standard (Obstruent,
    //  Overdub, etc.). Denormal flushed on every state update.
    //
    //  4 prime-length allpass stages (lengths set in prepare() for SR accuracy).
    //  Coefficient: 0.55 — stable and avoids obvious comb coloration.
    //==========================================================================

    void processAllpassStereo(float& inL, float& inR, int stage) noexcept
    {
        const int len = chamberAllpassLengths_[stage];
        auto&     buf = chamberBuf_[stage];
        int&      pos     = chamberBufPos_[stage];

        const float xL = inL;
        const float xR = inR;
        const float dL = buf[static_cast<size_t>(pos)];
        const float dR = buf[static_cast<size_t>(pos + len)];

        // Schroeder allpass: y[n] = d[n-M] + x[n] - coeff*y[n-M]
        const float yL = dL + xL - chamberAllpassCoeff_ * (dL + xL * chamberAllpassCoeff_);
        const float yR = dR + xR - chamberAllpassCoeff_ * (dR + xR * chamberAllpassCoeff_);

        // Store input feedback
        buf[static_cast<size_t>(pos)]       = flushDenormal(xL + chamberAllpassCoeff_ * yL);
        buf[static_cast<size_t>(pos + len)] = flushDenormal(xR + chamberAllpassCoeff_ * yR);

        pos = (pos + 1) % len;

        inL = yL;
        inR = yR;
    }

    //==========================================================================
    //  A I R   N O I S E   P R N G
    //==========================================================================

    float airNoiseRng() noexcept
    {
        airNoiseRng_ ^= airNoiseRng_ << 13u;
        airNoiseRng_ ^= airNoiseRng_ >> 17u;
        airNoiseRng_ ^= airNoiseRng_ << 5u;
        return (static_cast<float>(airNoiseRng_) * 2.3283064365e-10f) * 2.0f - 1.0f;
    }

    //==========================================================================
    //  H E L P E R :  fast sin/pow2 wrappers
    //==========================================================================

    static float fastSin(float x) noexcept
    {
        // Normalise to [0, 2π] then approximate
        x -= kOllotronTwoPi * std::floor(x * (1.0f / kOllotronTwoPi));
        // Bhaskara I approximation: accurate to ~0.2%
        if (x < kOllotronPi)
            return (4.0f * x * (kOllotronPi - x)) / (kOllotronPi * kOllotronPi * 1.25f - x * (kOllotronPi - x));
        else
        {
            x -= kOllotronPi;
            return -((4.0f * x * (kOllotronPi - x)) / (kOllotronPi * kOllotronPi * 1.25f - x * (kOllotronPi - x)));
        }
    }

    // fastPow2: 2^x (wraps fleet FastMath.h fastPow2 if available, inline fallback)
    static float fastPow2(float x) noexcept
    {
        // Use the fleet's FastMath implementation (included via FastMath.h)
        return xoceanus::fastPow2(x);
    }

    //==========================================================================
    //  D O C T R I N E   C O M P L I A N C E   B L O C K
    //
    //  D001 — Velocity Must Shape Timbre (line references):
    //    - renderVoicesRange(): velBrightBoost = v.velocity * velToBright
    //      applied to voiceTapeAge (line ~700) → cutoff brightens at high velocity
    //    - handleNoteOn(): wearBoost = (velocity + atBoost) * wearMemory
    //      harder playing wears the tape faster (line ~800)
    //
    //  D002 — Modulation is the Lifeblood:
    //    - 2 per-voice internal LFOs: flutterLFO (2–8Hz) + wowLFO (0.1–1Hz)
    //    - 2 user LFOs: lfo1 + lfo2 (rate, shape, depth, target params)
    //    - Mod wheel (CC1) → flutter depth via mwToFlutter param
    //    - Aftertouch → wear accumulator boost via atToWear param
    //    - 4 macros: macroCharacter, macroMovement, macroCoupling, macroSpace
    //    - 8-slot ModMatrix: ollo_modSlot1Src … ollo_modSlot8Amt
    //
    //  D003 — The Physics IS the Synthesis:
    //    - Mellotron M400, Chamberlin 200, Optigan, Kharlamov (1962),
    //      Kaye & Camras (1959), Risset (1969) all cited in header block.
    //    - 8-second hard cutoff: tapeCutoffSamples_ = 8.0 * sr_
    //    - Flutter spectrum 2–8 Hz (Kharlamov 1962), Wow 0.1–1 Hz
    //    - Per-voice incoherent flutter phases (Chamberlin recollections, 1974)
    //    - Release click: pressure plate lifting tape (M400 mechanical design)
    //    - Per-key wear accumulator (Kaye & Camras tape oxide shedding)
    //
    //  D004 — Dead Parameters Are Broken Promises:
    //    - ollo_tapeCut: always controls fadeout duration (fadeDurationSamples),
    //      even when 8s limit is far away; shorter values give an audible
    //      ramp starting earlier.
    //    - ollo_wowRate: always modulates voice pitch (wowCents injected every sample)
    //    - ollo_bank==User with no coupling: plays procedural triangle/square blend
    //    - ollo_couplingDubDepth: only effective when AudioToWavetable coupling
    //      is present AND bank==User. Otherwise it is consumed but produces no sound
    //      change — this is intentional (the coupling itself is absent).
    //      RATIONALE: the param's name makes clear it is coupling-conditional;
    //      it is never dead by design (per fleet doctrine on coupling-gated params).
    //
    //  D005 — An Engine That Cannot Breathe Is a Photograph:
    //    - ollo_lfo1Rate: floor 0.01 Hz (100-second cycle)
    //    - ollo_lfo2Rate: floor 0.01 Hz (100-second cycle)
    //    Both LFOs enforced via std::max(0.01f, ...) in renderBlock snapshot.
    //
    //  D006 — Expression Input Is Not Optional:
    //    - Velocity → tape-age filter brightness (D001 cross-compliant)
    //    - Mod wheel (CC1) → flutter depth (+mwToFlutter)
    //    - Aftertouch (channel pressure + per-note AT) → wear accumulator boost
    //
    //==========================================================================

    //==========================================================================
    //  M E M B E R S
    //==========================================================================

    // ---- Audio state ----
    float    sr_       = 44100.0f;
    int      maxBlock_ = 512;
    float    tapeCutoffSamples_ = 0.0f; // set in prepare() as kOlloTapeDurationSec * sr_
    float    clickDecayCoeff_   = 0.0f;

    std::array<OllotronVoice, kOllotronMaxVoices> voices_;

    // Per-key wear accumulator — 128 MIDI notes, persists between note-on events
    // Decays slowly between plays (exponential decay in renderBlock loop)
    std::array<float, 128> keyWear_ {};

    // ---- Coupling state ----
    // AudioToWavetable: single global ring buffer — per-voice read position tracks it
    std::array<float, kOllotronCouplingBuf> couplingAudioToWavetableBuf_ {};
    int   couplingWriteGlobal_ = 0;
    bool  couplingHasAudio_    = false;
    float couplingAmpFilter_   = 0.0f;  // AmpToFilter accumulator
    float couplingEnvMorph_    = 0.0f;  // EnvToMorph accumulator
    float couplingRingGain_    = 1.0f;  // AudioToRing gain scalar

    // ---- Post-FX ----
    CytomicSVF hissFilter_;
    CytomicSVF cabinetFilter_;

    // Chamber reverb allpass stages (4 stages, stereo interleaved in each buf)
    std::array<int, 4>           chamberAllpassLengths_ {};
    std::array<std::vector<float>, 4> chamberBuf_;
    std::array<int, 4>           chamberBufPos_ {};
    float                        chamberAllpassCoeff_ = 0.55f;

    // ---- Mod matrix ----
    ModMatrix<8> modMatrix_;

    // ---- MIDI/expression state ----
    float modWheelValue_   = 0.0f;
    float aftertouchValue_ = 0.0f;

    // ---- Coupling output cache ----
    float lastSampleL_ = 0.0f;
    float lastSampleR_ = 0.0f;

    // ---- Voice time (LRU counter) ----
    uint64_t voiceTime_ = 0;

    // ---- Air noise PRNG ----
    uint32_t airNoiseRng_ = 0xBEEF1234u;

    //==========================================================================
    //  P A R A M E T E R   P O I N T E R S
    //==========================================================================

    // Group A: Tape Chamber
    std::atomic<float>* pBank           = nullptr;
    std::atomic<float>* pTapeAge        = nullptr;
    std::atomic<float>* pTapeCut        = nullptr;
    std::atomic<float>* pTapeBalance    = nullptr;
    std::atomic<float>* pPinchRoller    = nullptr;

    // Group B: Flutter & Wow
    std::atomic<float>* pFlutterRate    = nullptr;
    std::atomic<float>* pFlutterDepth   = nullptr;
    std::atomic<float>* pWowRate        = nullptr;
    std::atomic<float>* pWowDepth       = nullptr;

    // Group C: Wear
    std::atomic<float>* pWearMemory     = nullptr;
    std::atomic<float>* pWearRecovery   = nullptr;
    std::atomic<float>* pWearCeiling    = nullptr;

    // Group D: Bank Timbre
    std::atomic<float>* pChoirFormant   = nullptr;
    std::atomic<float>* pStringsBow     = nullptr;
    std::atomic<float>* pFlutesBreath   = nullptr;
    std::atomic<float>* pUserA          = nullptr;
    std::atomic<float>* pUserB          = nullptr;
    std::atomic<float>* pUserBlend      = nullptr;

    // Group E: Amp ADSR
    std::atomic<float>* pAmpAtk         = nullptr;
    std::atomic<float>* pAmpDec         = nullptr;
    std::atomic<float>* pAmpSus         = nullptr;
    std::atomic<float>* pAmpRel         = nullptr;

    // Group F: LFOs
    std::atomic<float>* pLfo1Rate       = nullptr;
    std::atomic<float>* pLfo1Shape      = nullptr;
    std::atomic<float>* pLfo1Depth      = nullptr;
    std::atomic<float>* pLfo1Target     = nullptr;
    std::atomic<float>* pLfo2Rate       = nullptr;
    std::atomic<float>* pLfo2Shape      = nullptr;
    std::atomic<float>* pLfo2Depth      = nullptr;
    std::atomic<float>* pLfo2Target     = nullptr;

    // Group G: Expression
    std::atomic<float>* pVelToBright    = nullptr;
    std::atomic<float>* pMwToFlutter    = nullptr;
    std::atomic<float>* pAtToWear       = nullptr;

    // Group H: Space FX
    std::atomic<float>* pChamberMix     = nullptr;
    std::atomic<float>* pCabinetColor   = nullptr;
    std::atomic<float>* pAirNoise       = nullptr;

    // Group I: Macros
    std::atomic<float>* pMacroCharacter = nullptr;
    std::atomic<float>* pMacroMovement  = nullptr;
    std::atomic<float>* pMacroCoupling  = nullptr;
    std::atomic<float>* pMacroSpace     = nullptr;

    // Group J: Pitch
    std::atomic<float>* pMasterTune     = nullptr;
    std::atomic<float>* pCoarseTune     = nullptr;
    std::atomic<float>* pGlideTime      = nullptr;

    // Group K: Hiss
    std::atomic<float>* pHissLevel      = nullptr;
    std::atomic<float>* pHissTone       = nullptr;

    // Group L: Coupling
    std::atomic<float>* pCouplingDubDepth = nullptr;

    // Group M: Mod Matrix (managed by ModMatrix<8>)
    // No raw pointers needed here — modMatrix_ holds them.
};

} // namespace xoceanus
