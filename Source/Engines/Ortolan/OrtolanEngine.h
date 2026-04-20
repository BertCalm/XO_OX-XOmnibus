// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/ModMatrix.h"
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <vector>

namespace xoceanus
{

//==============================================================================
//
//  O R T O L A N   E N G I N E
//  VOSIM Hierarchical Pulse Synthesis
//
//  XO_OX Aquatic Identity: The Ortolan songbird — pure pulse-phrased melody.
//  The ocean's voice echoed from above. Four formant generators build the bird's
//  timbral signature; the hierarchical whale song structure organises pulses into
//  phrases, themes, and full songs.
//
//  Gallery code: ORTOLAN | Accent: Birdsong Amber #D4A017 | Prefix: ort_
//
//  References (D003):
//    Werner Kaegi, "VOSIM — A New Sound Synthesis System" (1978)
//    Roger Payne & Scott McVay, "Songs of Humpback Whales" (1971)
//    Gunnar Fant, "Acoustic Theory of Speech Production" (1960) — formant tables
//
//  Signal Flow:
//    4x VOSIM Generators (F1-F4, summed)
//        |
//    Song Structure (hierarchical grouping when songLevel > 0)
//        |
//    DC Block (cos^2 is unipolar, mandatory)
//        |
//    CytomicSVF Output Filter
//        |
//    VCA (amp envelope x velocity)
//        |
//    Output (stereo)
//
//  Coupling:
//    Input:  AudioToFM      -> coupling audio modulates pulse formant frequency
//            AmpToFilter    -> output filter cutoff modulation
//            EnvToMorph     -> coupling envelope modulates vowel position
//            RhythmToBlend  -> coupling rhythm triggers phrase resets
//    Output: ch0=L, ch1=R, ch2=current song phase [0..1]
//
//==============================================================================

static constexpr int   kOrtolanMaxVoices = 8;
static constexpr float kOrtolanTwoPi     = 6.28318530717958647692f;
static constexpr float kOrtolanPi        = 3.14159265358979323846f;

// Formant frequencies from Kaegi's VOSIM paper + Fant tables
// F1, F2, F3, F4 in Hz for vowels A / E / I / O / U
static constexpr float kOrtolanVowelFormants[5][4] = {
    {800.0f,  1150.0f, 2900.0f, 3900.0f},  // 0: A
    {350.0f,  2000.0f, 2800.0f, 3600.0f},  // 1: E
    {270.0f,  2140.0f, 2950.0f, 3900.0f},  // 2: I
    {450.0f,   800.0f, 2830.0f, 3800.0f},  // 3: O
    {325.0f,   700.0f, 2700.0f, 3800.0f},  // 4: U
};

//==============================================================================
// VosimGenerator -- one squared-cosine pulse-burst generator (Kaegi 1978)
//
// Each generator produces a burst of N squared-cosine pulses at formantFreq
// followed by silence until the next pitch period begins. The 4 generators
// together implement the VOSIM formant synthesis model.
//==============================================================================
struct VosimGenerator
{
    // Phase within the current cosine period (in samples)
    float phase          = 0.0f;
    int   currentPulse   = 0;     // which pulse in the burst (0..pulseCount-1)
    float amplitude      = 1.0f;  // decays each pulse by decayFactor
    bool  inSilence      = false;
    int   silenceSamples = 0;
    float level          = 0.0f;  // generator output mix level (set per-block)

    void reset() noexcept
    {
        phase          = 0.0f;
        currentPulse   = 0;
        amplitude      = 1.0f;
        inSilence      = false;
        silenceSamples = 0;
        level          = 0.0f;
    }

    // Produce one output sample.
    //
    //   formantFreq     -- target spectral peak (Hz), clamped to >= noteFreq
    //   pitchPeriodSamp -- samples per pitch period (sampleRate / noteFreq)
    //   pulseCount      -- N pulses per burst (1..32)
    //   decayFactor     -- per-pulse amplitude decay [0.5, 1.0]
    //   pulseShape      -- 0=cos^2, 1=sin^2, 2=tri^2, 3=half-sine
    //   sampleRate      -- current sample rate
    //   fmMod           -- additional Hz addend from coupling AudioToFM
    //
    // Returns the sample value (unipolar, [0..amplitude] before level scaling).
    inline float tick(float formantFreq, float pitchPeriodSamp,
                      int pulseCount, float decayFactor,
                      int pulseShape, float sampleRate,
                      float fmMod) noexcept
    {
        if (inSilence)
        {
            --silenceSamples;
            if (silenceSamples <= 0)
            {
                inSilence = false;
                phase     = 0.0f;
            }
            return 0.0f;
        }

        // Effective formant with FM modulation; keep at least 20 Hz and below
        // 0.45·sr to avoid aliasing when heavy FM pushes the formant toward Nyquist.
        const float effFormant  = std::clamp(formantFreq + fmMod, 20.0f, sampleRate * 0.45f);
        const float periodSamp  = std::max(sampleRate / effFormant, 2.0f);  // samples per cosine cycle

        // Generate squared waveform sample from current phase position
        const float t = phase / std::max(periodSamp, 1.0f);  // 0..1 over one period
        float s = 0.0f;
        switch (pulseShape)
        {
        case 0: // cos^2 -- always unipolar [0,1]
        {
            const float c = fastCos(t * kOrtolanTwoPi);
            s = c * c;
            break;
        }
        case 1: // sin^2 -- always unipolar [0,1]
        {
            const float sv = fastSin(t * kOrtolanTwoPi);
            s = sv * sv;
            break;
        }
        case 2: // tri^2 -- bipolar triangle squared, then scaled to [0,1]
        {
            const float tri = 4.0f * std::fabs(t - 0.5f) - 1.0f;
            s = (tri * tri) * 0.5f + 0.5f;
            break;
        }
        case 3: // half-sine -- positive half of sine cycle [0,1]
        {
            const float hs = fastSin(t * kOrtolanPi);
            s = hs * hs;
            break;
        }
        default:
        {
            const float c = fastCos(t * kOrtolanTwoPi);
            s = c * c;
            break;
        }
        }

        const float out = s * amplitude * level;

        // Advance phase
        phase += 1.0f;

        // Check for end of current cosine period (one pulse completed)
        if (phase >= periodSamp)
        {
            phase -= periodSamp;
            ++currentPulse;
            amplitude *= decayFactor;

            if (currentPulse >= pulseCount)
            {
                // Burst complete -- enter silence until next pitch period.
                // silenceLen = pitchPeriod - total burst duration
                // Clamp to 0 if burst is longer than pitch period.
                const float burstSamples = static_cast<float>(pulseCount) * periodSamp;
                const int   silLen       = static_cast<int>(pitchPeriodSamp - burstSamples);
                silenceSamples = std::max(1, silLen);  // at least 1 sample of silence
                inSilence      = true;
                // Reset for next burst
                currentPulse = 0;
                amplitude    = 1.0f;
                phase        = 0.0f;
            }
        }

        return out;
    }
};

//==============================================================================
// SongStructure -- hierarchical phrase/theme organiser (Payne & McVay 1971)
//
// Tracks position within the whale-song hierarchy:
//   Level 0 (songLevel~0):   continuous pulses, no hierarchy
//   Level 1 (songLevel~0.3): phrase-level grouping
//   Level 2 (songLevel~0.6): theme-level with formant drift
//   Level 3 (songLevel~1.0): full song with long-term evolution
//==============================================================================
struct SongStructure
{
    int  currentBurst    = 0;   // burst index within current phrase
    int  currentPhrase   = 0;   // phrase index within current theme
    int  phraseGapLeft   = 0;   // samples remaining in inter-phrase silence
    int  themeGapLeft    = 0;   // samples remaining in inter-theme silence
    bool inPhraseGap     = false;
    bool inThemeGap      = false;

    // Accumulated drift values for theme evolution
    float pitchDriftAcc    = 0.0f;  // accumulated pitch offset in semitones
    float formantDriftAcc  = 0.0f;  // accumulated formant offset in Hz

    void reset() noexcept
    {
        currentBurst      = 0;
        currentPhrase     = 0;
        phraseGapLeft     = 0;
        themeGapLeft      = 0;
        inPhraseGap       = false;
        inThemeGap        = false;
        pitchDriftAcc     = 0.0f;
        formantDriftAcc   = 0.0f;
    }
};

//==============================================================================
// OrtolanVoice -- one polyphonic voice
//==============================================================================
struct OrtolanVoice
{
    bool  active    = false;
    bool  releasing = false;
    int   note      = -1;
    float velocity  = 0.0f;  // normalized 0..1
    float keyTrack  = 0.0f;  // (note-60)/60, bipolar

    // Current (glide-smoothed) note frequency
    float glideFreq = 440.0f;

    // 4 VOSIM generators (one per formant F1-F4)
    VosimGenerator gen[4];

    // Song structure state
    SongStructure song;

    // DC blocker state: y[n] = x[n] - x[n-1] + 0.995 * y[n-1]
    float dcBlockX = 0.0f;
    float dcBlockY = 0.0f;

    // Output filter (stereo independent instances for slight stereo widening)
    CytomicSVF outputFilterL;
    CytomicSVF outputFilterR;

    // Envelopes
    StandardADSR ampEnv;
    StandardADSR filterEnv;

    // Per-voice LFOs
    StandardLFO lfo1;
    StandardLFO lfo2;

    // Last LFO output values (for mod matrix source population)
    float lastLfo1Val = 0.0f;
    float lastLfo2Val = 0.0f;

    // Coupling AudioToFM accumulator (per-sample tight coupling)
    float couplingFmAccum = 0.0f;

    // Filter coefficient update counter (updates every 16 samples)
    int filterUpdateCounter = 0;

    // Per-voice xorshift32 PRNG for song structure drift
    uint32_t rng = 12345u;

    // Song phase accumulator (0..1, normalized position in hierarchy)
    float songPhase = 0.0f;

    // Pitch period sample counter for song structure advancement
    float pitchPeriodAccum = 0.0f;

    void reset(float /*sampleRate*/) noexcept
    {
        active    = false;
        releasing = false;
        note      = -1;
        velocity  = 0.0f;
        keyTrack  = 0.0f;
        glideFreq = 440.0f;

        for (int i = 0; i < 4; ++i)
            gen[i].reset();

        song.reset();

        dcBlockX = 0.0f;
        dcBlockY = 0.0f;

        outputFilterL.reset();
        outputFilterR.reset();

        ampEnv.reset();
        filterEnv.reset();
        lfo1.reset();
        lfo2.reset();
        lastLfo1Val       = 0.0f;
        lastLfo2Val       = 0.0f;
        couplingFmAccum   = 0.0f;
        filterUpdateCounter = 0;
        songPhase         = 0.0f;
        pitchPeriodAccum  = 0.0f;
    }
};

//==============================================================================
//
//  OrtolanEngine -- VOSIM Hierarchical Pulse Synthesis
//
//  Implements SynthEngine for XOceanus integration.
//  All DSP is inline per project convention -- .cpp is a one-line stub.
//
//  Parameter prefix: ort_
//  Gallery accent:   Birdsong Amber #D4A017
//
//==============================================================================
class OrtolanEngine : public SynthEngine
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

        // ---- A: VOSIM Core (7 params) ----
        params.push_back(std::make_unique<AP>(PID{"ort_vowel",1}, "Ortolan Vowel",
            NR{0.0f, 4.0f, 0.001f}, 0.0f));

        params.push_back(std::make_unique<AP>(PID{"ort_pulseCount",1}, "Ortolan Pulse Count",
            NR{1.0f, 32.0f, 1.0f}, 8.0f));

        params.push_back(std::make_unique<AP>(PID{"ort_decay",1}, "Ortolan Decay",
            NR{0.5f, 1.0f, 0.001f}, 0.85f));

        params.push_back(std::make_unique<AP>(PID{"ort_formantShift",1}, "Ortolan Formant Shift",
            NR{-2000.0f, 2000.0f, 0.5f}, 0.0f));

        params.push_back(std::make_unique<AP>(PID{"ort_formantSpread",1}, "Ortolan Formant Spread",
            NR{0.5f, 2.0f, 0.001f}, 1.0f));

        params.push_back(std::make_unique<AP>(PID{"ort_generatorMix",1}, "Ortolan Generator Mix",
            NR{0.0f, 1.0f, 0.001f}, 1.0f));

        params.push_back(std::make_unique<AP>(PID{"ort_brightness",1}, "Ortolan Brightness",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        // ---- B: Song Structure (5 params) ----
        params.push_back(std::make_unique<AP>(PID{"ort_songLevel",1}, "Ortolan Song Level",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));

        params.push_back(std::make_unique<AP>(PID{"ort_phraseLength",1}, "Ortolan Phrase Length",
            NR{2.0f, 8.0f, 1.0f}, 4.0f));

        params.push_back(std::make_unique<AP>(PID{"ort_phraseGap",1}, "Ortolan Phrase Gap",
            NR{0.0f, 1.0f, 0.001f}, 0.3f));

        params.push_back(std::make_unique<AP>(PID{"ort_themeDrift",1}, "Ortolan Theme Drift",
            NR{0.0f, 1.0f, 0.001f}, 0.2f));

        params.push_back(std::make_unique<AP>(PID{"ort_pitchDrift",1}, "Ortolan Pitch Drift",
            NR{0.0f, 1.0f, 0.001f}, 0.1f));

        // ---- C: Waveform (4 params) ----
        params.push_back(std::make_unique<APC>(PID{"ort_pulseShape",1}, "Ortolan Pulse Shape",
            juce::StringArray{"Cos2","Sin2","Tri2","HalfSine"}, 0));

        params.push_back(std::make_unique<APC>(PID{"ort_dcBlock",1}, "Ortolan DC Block",
            juce::StringArray{"Off","On"}, 1));

        params.push_back(std::make_unique<AP>(PID{"ort_pitchTrack",1}, "Ortolan Pitch Track",
            NR{0.0f, 1.0f, 0.001f}, 1.0f));

        {
            NR r{20.0f, 800.0f, 0.1f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ort_baseFreq",1}, "Ortolan Base Freq", r, 110.0f));
        }

        // ---- D: Filter + Filter Envelope (9 params) ----
        {
            NR r{20.0f, 20000.0f, 0.1f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ort_fltCutoff",1}, "Ortolan Flt Cutoff", r, 10000.0f));
        }
        params.push_back(std::make_unique<AP>(PID{"ort_fltReso",1}, "Ortolan Flt Reso",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));

        params.push_back(std::make_unique<APC>(PID{"ort_fltType",1}, "Ortolan Flt Type",
            juce::StringArray{"LP","HP","BP","Notch"}, 0));

        params.push_back(std::make_unique<AP>(PID{"ort_fltEnvAmt",1}, "Ortolan Flt Env Amt",
            NR{-1.0f, 1.0f, 0.001f}, 0.2f));

        params.push_back(std::make_unique<AP>(PID{"ort_fltKeyTrack",1}, "Ortolan Flt Key Track",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ort_fltAtk",1}, "Ortolan Flt Atk", r, 0.01f));
        }
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ort_fltDec",1}, "Ortolan Flt Dec", r, 0.3f));
        }
        params.push_back(std::make_unique<AP>(PID{"ort_fltSus",1}, "Ortolan Flt Sus",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ort_fltRel",1}, "Ortolan Flt Rel", r, 0.4f));
        }

        // ---- E: Amp Envelope (5 params) ----
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ort_ampAtk",1}, "Ortolan Amp Atk", r, 0.005f));
        }
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ort_ampDec",1}, "Ortolan Amp Dec", r, 0.4f));
        }
        params.push_back(std::make_unique<AP>(PID{"ort_ampSus",1}, "Ortolan Amp Sus",
            NR{0.0f, 1.0f, 0.001f}, 0.7f));
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ort_ampRel",1}, "Ortolan Amp Rel", r, 0.5f));
        }
        // D001: velocity scales decay rate = brighter formants at high velocity
        params.push_back(std::make_unique<AP>(PID{"ort_velTimbre",1}, "Ortolan Vel Timbre",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        // ---- F: LFOs (8 params) ----
        static const juce::StringArray kLFOShapes   {"Sine","Tri","Saw","Square","S&H"};
        static const juce::StringArray kLFO1Targets {"Vowel","FormantShift","SongLevel","PulseCount"};
        static const juce::StringArray kLFO2Targets {"Vowel","FormantShift","SongLevel","PulseCount"};

        {
            NR r{0.01f, 20.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ort_lfo1Rate",1}, "Ortolan LFO1 Rate", r, 0.4f));
        }
        params.push_back(std::make_unique<AP>(PID{"ort_lfo1Depth",1}, "Ortolan LFO1 Depth",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<APC>(PID{"ort_lfo1Shape",1}, "Ortolan LFO1 Shape",
            kLFOShapes, 0));
        params.push_back(std::make_unique<APC>(PID{"ort_lfo1Target",1}, "Ortolan LFO1 Target",
            kLFO1Targets, 0));  // Vowel

        {
            NR r{0.01f, 20.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ort_lfo2Rate",1}, "Ortolan LFO2 Rate", r, 0.12f));
        }
        params.push_back(std::make_unique<AP>(PID{"ort_lfo2Depth",1}, "Ortolan LFO2 Depth",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<APC>(PID{"ort_lfo2Shape",1}, "Ortolan LFO2 Shape",
            kLFOShapes, 1));  // Triangle
        params.push_back(std::make_unique<APC>(PID{"ort_lfo2Target",1}, "Ortolan LFO2 Target",
            kLFO2Targets, 1)); // FormantShift

        // ---- G: Mod Matrix (4 slots x 3 params = 12 params) ----
        static const juce::StringArray kOrtolanModDests {
            "Off", "Filter Cutoff", "Vowel", "Formant Shift",
            "Song Level", "Pulse Count", "Amp Level"
        };
        ModMatrix<4>::addParameters(params, "ort_", "Ortolan", kOrtolanModDests);

        // ---- H: Macros + Voice (7 params) ----
        // M1=PLUMAGE: vowel position + brightness (bird's color = its voice)
        params.push_back(std::make_unique<AP>(PID{"ort_macro1",1}, "Ortolan Macro1",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));
        // M2=SONG: songLevel + themeDrift (hierarchical complexity)
        params.push_back(std::make_unique<AP>(PID{"ort_macro2",1}, "Ortolan Macro2",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        // M3=FLIGHT: formantShift + pitchDrift (migration arc)
        params.push_back(std::make_unique<AP>(PID{"ort_macro3",1}, "Ortolan Macro3",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        // M4=SPACE: filter cutoff + width
        params.push_back(std::make_unique<AP>(PID{"ort_macro4",1}, "Ortolan Macro4",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<APC>(PID{"ort_voiceMode",1}, "Ortolan Voice Mode",
            juce::StringArray{"Mono","Legato","Poly4","Poly8"}, 2));

        {
            NR r{0.0f, 2.0f, 0.001f};
            r.setSkewForCentre(0.5f);
            params.push_back(std::make_unique<AP>(PID{"ort_glide",1}, "Ortolan Glide", r, 0.0f));
        }

        params.push_back(std::make_unique<APC>(PID{"ort_glideMode",1}, "Ortolan Glide Mode",
            juce::StringArray{"Legato","Always"}, 0));
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
        sampleRateFloat = (sampleRate > 0.0) ? static_cast<float>(sampleRate) : 44100.0f;
        maxBlock = maxBlockSize;

        for (int i = 0; i < kOrtolanMaxVoices; ++i)
        {
            voices[i].rng = 12345u + static_cast<uint32_t>(i) * 31337u;
            voices[i].reset(sampleRateFloat);
        }

        couplingFmBuf.assign(static_cast<size_t>(std::max(maxBlockSize, 1)), 0.0f);
        couplingAmpFilter = 0.0f;
        couplingEnvVowel  = 0.0f;
        couplingRhythm    = 0.0f;
        couplingFmLevel   = 0.0f;

        modWheelValue   = 0.0f;
        aftertouchValue = 0.0f;

        lastSampleL   = 0.0f;
        lastSampleR   = 0.0f;
        lastSongPhase = 0.0f;

        activeVoiceCount_.store(0, std::memory_order_relaxed);

        // SilenceGate -- VOSIM tails are relatively short, use standard hold
        prepareSilenceGate(sampleRate, maxBlockSize, 300.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset(sampleRateFloat);

        std::fill(couplingFmBuf.begin(), couplingFmBuf.end(), 0.0f);
        couplingAmpFilter = 0.0f;
        couplingEnvVowel  = 0.0f;
        couplingRhythm    = 0.0f;
        couplingFmLevel   = 0.0f;
        modWheelValue     = 0.0f;
        aftertouchValue   = 0.0f;
        lastSampleL       = 0.0f;
        lastSampleR       = 0.0f;
        lastSongPhase     = 0.0f;
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
        case CouplingType::AudioToFM:
        {
            // Copy into FM buffer -- applied per-sample to formant frequencies
            const int copyLen = std::min(numSamples, maxBlock);
            for (int i = 0; i < copyLen; ++i)
                couplingFmBuf[static_cast<size_t>(i)] = sourceBuffer[i] * amount;
            couplingFmLevel = amount;
            // Update per-voice accumulator with the last sample
            for (auto& v : voices)
                if (v.active)
                    v.couplingFmAccum = sourceBuffer[numSamples - 1] * amount;
            break;
        }
        case CouplingType::AmpToFilter:
        {
            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += std::fabs(sourceBuffer[i]);
            rms /= static_cast<float>(numSamples);
            couplingAmpFilter = rms * amount;
            break;
        }
        case CouplingType::EnvToMorph:
        {
            // Last sample of the envelope modulates vowel position
            couplingEnvVowel = sourceBuffer[numSamples - 1] * amount;
            break;
        }
        case CouplingType::RhythmToBlend:
        {
            // Rhythm value -- strong hit (>0.5) resets phrase counter
            couplingRhythm = sourceBuffer[numSamples - 1] * amount;
            break;
        }
        default:
        {
            // Generic fallback: treat as AmpToFilter. Assign (not +=) so repeated
            // unknown coupling types cannot accumulate across blocks.
            float mav = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                mav += std::fabs(sourceBuffer[i]);
            mav /= static_cast<float>(numSamples);
            couplingAmpFilter = mav * amount;
            break;
        }
        }
    }

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        if (channel == 0) return lastSampleL;
        if (channel == 1) return lastSampleR;
        if (channel == 2) return lastSongPhase;  // current song phase [0..1]
        return 0.0f;
    }

    //==========================================================================
    //  P A R A M E T E R   A T T A C H M E N T
    //==========================================================================

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        // Group A
        pVowel         = apvts.getRawParameterValue("ort_vowel");
        pPulseCount    = apvts.getRawParameterValue("ort_pulseCount");
        pDecay         = apvts.getRawParameterValue("ort_decay");
        pFormantShift  = apvts.getRawParameterValue("ort_formantShift");
        pFormantSpread = apvts.getRawParameterValue("ort_formantSpread");
        pGeneratorMix  = apvts.getRawParameterValue("ort_generatorMix");
        pBrightness    = apvts.getRawParameterValue("ort_brightness");

        // Group B
        pSongLevel    = apvts.getRawParameterValue("ort_songLevel");
        pPhraseLength = apvts.getRawParameterValue("ort_phraseLength");
        pPhraseGap    = apvts.getRawParameterValue("ort_phraseGap");
        pThemeDrift   = apvts.getRawParameterValue("ort_themeDrift");
        pPitchDrift   = apvts.getRawParameterValue("ort_pitchDrift");

        // Group C
        pPulseShape  = apvts.getRawParameterValue("ort_pulseShape");
        pDcBlock     = apvts.getRawParameterValue("ort_dcBlock");
        pPitchTrack  = apvts.getRawParameterValue("ort_pitchTrack");
        pBaseFreq    = apvts.getRawParameterValue("ort_baseFreq");

        // Group D
        pFltCutoff   = apvts.getRawParameterValue("ort_fltCutoff");
        pFltReso     = apvts.getRawParameterValue("ort_fltReso");
        pFltType     = apvts.getRawParameterValue("ort_fltType");
        pFltEnvAmt   = apvts.getRawParameterValue("ort_fltEnvAmt");
        pFltKeyTrack = apvts.getRawParameterValue("ort_fltKeyTrack");
        pFltAtk      = apvts.getRawParameterValue("ort_fltAtk");
        pFltDec      = apvts.getRawParameterValue("ort_fltDec");
        pFltSus      = apvts.getRawParameterValue("ort_fltSus");
        pFltRel      = apvts.getRawParameterValue("ort_fltRel");

        // Group E
        pAmpAtk    = apvts.getRawParameterValue("ort_ampAtk");
        pAmpDec    = apvts.getRawParameterValue("ort_ampDec");
        pAmpSus    = apvts.getRawParameterValue("ort_ampSus");
        pAmpRel    = apvts.getRawParameterValue("ort_ampRel");
        pVelTimbre = apvts.getRawParameterValue("ort_velTimbre");

        // Group F
        pLfo1Rate   = apvts.getRawParameterValue("ort_lfo1Rate");
        pLfo1Depth  = apvts.getRawParameterValue("ort_lfo1Depth");
        pLfo1Shape  = apvts.getRawParameterValue("ort_lfo1Shape");
        pLfo1Target = apvts.getRawParameterValue("ort_lfo1Target");
        pLfo2Rate   = apvts.getRawParameterValue("ort_lfo2Rate");
        pLfo2Depth  = apvts.getRawParameterValue("ort_lfo2Depth");
        pLfo2Shape  = apvts.getRawParameterValue("ort_lfo2Shape");
        pLfo2Target = apvts.getRawParameterValue("ort_lfo2Target");

        // Group G
        modMatrix.attachParameters(apvts, "ort_");

        // Group H
        pMacro1    = apvts.getRawParameterValue("ort_macro1");
        pMacro2    = apvts.getRawParameterValue("ort_macro2");
        pMacro3    = apvts.getRawParameterValue("ort_macro3");
        pMacro4    = apvts.getRawParameterValue("ort_macro4");
        pVoiceMode = apvts.getRawParameterValue("ort_voiceMode");
        pGlide     = apvts.getRawParameterValue("ort_glide");
        pGlideMode = apvts.getRawParameterValue("ort_glideMode");
    }

    //==========================================================================
    //  I D E N T I T Y
    //==========================================================================

    juce::String  getEngineId()     const override { return "Ortolan"; }
    juce::Colour  getAccentColour() const override { return juce::Colour(0xFFD4A017); } // Birdsong Amber
    int           getMaxVoices()    const override { return kOrtolanMaxVoices; }

    //==========================================================================
    //  R E N D E R   B L O C K
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (sampleRateFloat <= 0.0f) return;
        if (numSamples <= 0) return;

        // ---- SilenceGate: wake on note-on, bail if truly silent ----
        for (const auto& md : midi)
        {
            if (md.getMessage().isNoteOn())
            {
                wakeSilenceGate();
                break;
            }
        }
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            return;
        }

        // ---- Snapshot all parameters once per block ----
        const float macro1 = pMacro1 ? pMacro1->load() : 0.5f;
        const float macro2 = pMacro2 ? pMacro2->load() : 0.0f;
        const float macro3 = pMacro3 ? pMacro3->load() : 0.0f;
        const float macro4 = pMacro4 ? pMacro4->load() : 0.5f;

        // M1=PLUMAGE: vowel shifts toward I (bright), brightness lifts upper formants
        float vowel     = pVowel      ? pVowel->load()      : 0.0f;
        float brightness= pBrightness ? pBrightness->load() : 0.5f;
        vowel      = std::clamp(vowel      + macro1 * 2.0f, 0.0f, 4.0f);
        brightness = std::clamp(brightness + macro1 * 0.4f, 0.0f, 1.0f);

        // Apply coupling vowel modulation (EnvToMorph)
        vowel = std::clamp(vowel + couplingEnvVowel * 2.0f, 0.0f, 4.0f);

        // Aftertouch (D006): modulates pulse count -> bandwidth change
        const float aftertouchPulseAdd = aftertouchValue * 8.0f;
        int pulseCountRaw = pPulseCount
            ? static_cast<int>(std::round(pPulseCount->load())) : 8;
        pulseCountRaw = std::clamp(
            pulseCountRaw + static_cast<int>(aftertouchPulseAdd), 1, 32);

        const float decayFactor    = pDecay         ? pDecay->load()         : 0.85f;
        float       formantShift   = pFormantShift  ? pFormantShift->load()  : 0.0f;
        float       formantSpread  = pFormantSpread ? pFormantSpread->load() : 1.0f;
        const float generatorMix   = pGeneratorMix  ? pGeneratorMix->load()  : 1.0f;
        const int   pulseShape     = pPulseShape    ? static_cast<int>(pPulseShape->load())  : 0;
        const bool  dcBlockOn      = pDcBlock       ? (pDcBlock->load() >= 0.5f)             : true;
        const float pitchTrack     = pPitchTrack    ? pPitchTrack->load()    : 1.0f;
        const float baseFreq       = pBaseFreq      ? pBaseFreq->load()      : 110.0f;

        // M3=FLIGHT: formantShift arc + formant spread widening
        formantShift  = formantShift  + macro3 * 600.0f;
        formantSpread = std::clamp(formantSpread + macro3 * 0.3f, 0.5f, 2.0f);

        // Song structure
        float songLevel   = pSongLevel   ? pSongLevel->load()   : 0.0f;
        float themeDrift  = pThemeDrift  ? pThemeDrift->load()  : 0.2f;
        float pitchDrift  = pPitchDrift  ? pPitchDrift->load()  : 0.1f;

        // M2=SONG: macro adds song complexity + drift
        songLevel  = std::clamp(songLevel  + macro2 * 0.8f, 0.0f, 1.0f);
        themeDrift = std::clamp(themeDrift + macro2 * 0.4f, 0.0f, 1.0f);
        pitchDrift = std::clamp(pitchDrift + macro3 * 0.3f, 0.0f, 1.0f);

        const int   phraseLength = pPhraseLength
            ? static_cast<int>(std::round(pPhraseLength->load())) : 4;
        const float phraseGapFac = pPhraseGap    ? pPhraseGap->load()    : 0.3f;

        // CC1 mod wheel (D006): modulates songLevel -> timbral structure change
        const float effectiveSongLevel = std::clamp(
            songLevel + modWheelValue * 0.5f, 0.0f, 1.0f);

        // Filter
        float baseCutoff = pFltCutoff ? pFltCutoff->load() : 10000.0f;
        baseCutoff += couplingAmpFilter * 8000.0f; // AmpToFilter coupling
        // M4=SPACE: macro opens filter
        baseCutoff = std::clamp(baseCutoff * (0.5f + macro4), 20.0f, 20000.0f);

        const float fltReso     = pFltReso     ? pFltReso->load()     : 0.0f;
        const int   fltType     = pFltType     ? static_cast<int>(pFltType->load()) : 0;
        const float fltEnvAmt   = pFltEnvAmt   ? pFltEnvAmt->load()   : 0.2f;
        const float fltKeyTrack = pFltKeyTrack ? pFltKeyTrack->load() : 0.5f;
        const float fltAtk      = pFltAtk      ? pFltAtk->load()      : 0.01f;
        const float fltDec      = pFltDec      ? pFltDec->load()      : 0.3f;
        const float fltSus      = pFltSus      ? pFltSus->load()      : 0.0f;
        const float fltRel      = pFltRel      ? pFltRel->load()      : 0.4f;

        // Amp envelope
        const float ampAtk    = pAmpAtk    ? pAmpAtk->load()    : 0.005f;
        const float ampDec    = pAmpDec    ? pAmpDec->load()    : 0.4f;
        const float ampSus    = pAmpSus    ? pAmpSus->load()    : 0.7f;
        const float ampRel    = pAmpRel    ? pAmpRel->load()    : 0.5f;
        const float velTimbre = pVelTimbre ? pVelTimbre->load() : 0.5f;

        // LFOs -- enforce 0.01 Hz floor (D005)
        const float lfo1Rate  = std::max(0.01f, pLfo1Rate  ? pLfo1Rate->load()  : 0.4f);
        const float lfo1Depth = pLfo1Depth ? pLfo1Depth->load() : 0.0f;
        const int   lfo1Shape = pLfo1Shape ? static_cast<int>(pLfo1Shape->load())  : 0;
        const int   lfo1Tgt   = pLfo1Target? static_cast<int>(pLfo1Target->load()) : 0;

        const float lfo2Rate  = std::max(0.01f, pLfo2Rate  ? pLfo2Rate->load()  : 0.12f);
        const float lfo2Depth = pLfo2Depth ? pLfo2Depth->load() : 0.0f;
        const int   lfo2Shape = pLfo2Shape ? static_cast<int>(pLfo2Shape->load())  : 1;
        const int   lfo2Tgt   = pLfo2Target? static_cast<int>(pLfo2Target->load()) : 1;

        const int   voiceMode = pVoiceMode ? static_cast<int>(pVoiceMode->load()) : 2;
        const float glideTime = pGlide     ? pGlide->load() : 0.0f;
        const int   glideMode = pGlideMode ? static_cast<int>(pGlideMode->load()) : 0;

        // Glide coefficient: glideMode 0 = legato only, 1 = always
        const bool  glideActive = (glideTime > 0.0001f) && (glideMode == 1 || voiceMode == 1);
        const float glideCoeff = glideActive
            ? fastExp(-1.0f / (glideTime * sampleRateFloat))
            : 0.0f;

        // ---- Build mod matrix sources from currently active voices ----
        {
            float lfo1Sum = 0.0f, lfo2Sum = 0.0f, envSum = 0.0f;
            float velSum  = 0.0f, ktSum   = 0.0f;
            int count = 0;
            for (auto& v : voices)
            {
                if (v.active)
                {
                    lfo1Sum += v.lastLfo1Val;
                    lfo2Sum += v.lastLfo2Val;
                    envSum  += v.ampEnv.getLevel();
                    velSum  += v.velocity;
                    ktSum   += v.keyTrack;
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
            blockModSrc.modWheel   = modWheelValue;
            blockModSrc.aftertouch = aftertouchValue;
        }

        // ---- Apply mod matrix ----
        // Destinations: 0=Off, 1=Filter Cutoff, 2=Vowel, 3=Formant Shift,
        //               4=Song Level, 5=Pulse Count, 6=Amp Level
        float modDestOffsets[7] = {};
        modMatrix.apply(blockModSrc, modDestOffsets);

        baseCutoff    = std::clamp(baseCutoff + modDestOffsets[1] * 8000.0f, 20.0f, 20000.0f);
        vowel         = std::clamp(vowel      + modDestOffsets[2] * 2.0f,   0.0f,  4.0f);
        formantShift += modDestOffsets[3] * 800.0f;
        const float modSongLevelAdd  = modDestOffsets[4];
        const float modPulseCountAdd = modDestOffsets[5] * 8.0f;
        const float modAmpLevel      = modDestOffsets[6];

        const float finalSongLevel = std::clamp(effectiveSongLevel + modSongLevelAdd, 0.0f, 1.0f);
        const int   finalPulseCount = std::clamp(
            pulseCountRaw + static_cast<int>(modPulseCountAdd), 1, 32);

        // ---- Compute vowel-interpolated formant frequencies ----
        float targetFormants[4];
        computeVowelFormants(vowel, formantShift, formantSpread, targetFormants);

        // ---- Compute per-generator level mix ----
        float genLevel[4];
        computeGeneratorLevels(generatorMix, brightness, genLevel);

        // ---- Output buffers ----
        auto* writeL = buffer.getWritePointer(0);
        auto* writeR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : writeL;
        // ADDITIVE: do not clear — engine adds to existing buffer (slot chain convention)

        // ---- MIDI + render interleaved ----
        int midiSamplePos = 0;

        for (const auto& midiEvent : midi)
        {
            const auto& msg    = midiEvent.getMessage();
            const int   msgPos = std::min(midiEvent.samplePosition, numSamples - 1);

            // Render up to this MIDI event
            renderVoicesRange(writeL, writeR, midiSamplePos, msgPos,
                              targetFormants, genLevel, finalPulseCount, decayFactor,
                              pulseShape, dcBlockOn, pitchTrack, baseFreq,
                              finalSongLevel, phraseLength, phraseGapFac,
                              themeDrift, pitchDrift,
                              ampAtk, ampDec, ampSus, ampRel,
                              baseCutoff, fltReso, fltType, fltEnvAmt, fltKeyTrack,
                              fltAtk, fltDec, fltSus, fltRel,
                              lfo1Rate, lfo1Depth, lfo1Shape, lfo1Tgt,
                              lfo2Rate, lfo2Depth, lfo2Shape, lfo2Tgt,
                              velTimbre, glideCoeff, modAmpLevel);
            midiSamplePos = msgPos;

            if (msg.isNoteOn())
            {
                handleNoteOn(msg.getNoteNumber(), msg.getVelocity(),
                             ampAtk, ampDec, ampSus, ampRel,
                             fltAtk, fltDec, fltSus, fltRel,
                             voiceMode, glideTime);
            }
            else if (msg.isNoteOff())
            {
                handleNoteOff(msg.getNoteNumber());
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                modWheelValue = msg.getControllerValue() / 127.0f;
            }
            else if (msg.isChannelPressure())
            {
                aftertouchValue = msg.getChannelPressureValue() / 127.0f;
            }
            else if (msg.isAftertouch())
            {
                aftertouchValue = msg.getAfterTouchValue() / 127.0f;
            }
        }

        // Render remaining samples after last MIDI event
        renderVoicesRange(writeL, writeR, midiSamplePos, numSamples,
                          targetFormants, genLevel, finalPulseCount, decayFactor,
                          pulseShape, dcBlockOn, pitchTrack, baseFreq,
                          finalSongLevel, phraseLength, phraseGapFac,
                          themeDrift, pitchDrift,
                          ampAtk, ampDec, ampSus, ampRel,
                          baseCutoff, fltReso, fltType, fltEnvAmt, fltKeyTrack,
                          fltAtk, fltDec, fltSus, fltRel,
                          lfo1Rate, lfo1Depth, lfo1Shape, lfo1Tgt,
                          lfo2Rate, lfo2Depth, lfo2Shape, lfo2Tgt,
                          velTimbre, glideCoeff, modAmpLevel);

        // ---- Cache last output samples for coupling ----
        if (numSamples > 0)
        {
            lastSampleL = writeL[numSamples - 1];
            lastSampleR = writeR[numSamples - 1];
        }

        // ---- Coupling accumulator decay — block-size-invariant (~1s tau) ----
        // Old `*= 0.999f` was block-rate so feel varied with DAW buffer size.
        const float couplingDecayCoeff = fastExp(-static_cast<float>(numSamples) / sampleRateFloat);
        couplingAmpFilter *= couplingDecayCoeff;
        couplingEnvVowel  *= couplingDecayCoeff;
        couplingRhythm    *= couplingDecayCoeff;
        couplingFmLevel   *= couplingDecayCoeff;

        // ---- Active voice count (atomic) ----
        int av = 0;
        for (auto& v : voices)
            if (v.active) ++av;
        activeVoiceCount_.store(av, std::memory_order_relaxed);

        // Update lastSongPhase from the first active voice
        for (auto& v : voices)
        {
            if (v.active)
            {
                lastSongPhase = v.songPhase;
                break;
            }
        }

        // SRO: SilenceGate analysis
        analyzeForSilenceGate(buffer, numSamples);
    }

private:

    //==========================================================================
    //  P R N G  (xorshift32)
    //==========================================================================
    static inline float xorRand(uint32_t& state) noexcept
    {
        state ^= state << 13u;
        state ^= state >> 17u;
        state ^= state << 5u;
        return static_cast<float>(state) * 2.3283064365e-10f; // [0, 1)
    }

    //==========================================================================
    //  V O W E L   F O R M A N T   I N T E R P O L A T I O N
    //
    //  vowel [0..4] indexes into the 5-vowel table with linear interpolation.
    //  formantShift adds a global Hz offset.
    //  formantSpread scales each formant's distance from F1.
    //==========================================================================
    static void computeVowelFormants(float vowel,
                                     float formantShift,
                                     float formantSpread,
                                     float outFreqs[4]) noexcept
    {
        vowel = std::clamp(vowel, 0.0f, 4.0f);
        const int   lo  = static_cast<int>(vowel);
        const int   hi  = std::min(lo + 1, 4);
        const float t   = vowel - static_cast<float>(lo);

        float baseFreqs[4];
        for (int f = 0; f < 4; ++f)
            baseFreqs[f] = lerp(kOrtolanVowelFormants[lo][f],
                                kOrtolanVowelFormants[hi][f], t);

        // Apply shift and spread (spread is relative to F1)
        const float f1 = baseFreqs[0];
        for (int f = 0; f < 4; ++f)
        {
            const float spread = f1 + (baseFreqs[f] - f1) * formantSpread;
            outFreqs[f] = std::max(20.0f, spread + formantShift);
        }
    }

    //==========================================================================
    //  G E N E R A T O R   L E V E L   M I X
    //
    //  generatorMix=0: F1 only
    //  generatorMix=1: all four, with brightness boosting F3/F4
    //==========================================================================
    static void computeGeneratorLevels(float generatorMix,
                                       float brightness,
                                       float outLevels[4]) noexcept
    {
        outLevels[0] = 1.0f;
        outLevels[1] = std::clamp(generatorMix * 1.5f,                          0.0f, 1.0f);
        outLevels[2] = std::clamp((generatorMix - 0.33f) * 1.5f + brightness * 0.5f, 0.0f, 1.0f);
        outLevels[3] = std::clamp((generatorMix - 0.66f) * 1.5f + brightness * 0.8f, 0.0f, 1.0f);

        // Normalise to prevent clipping when multiple generators are active
        const float sum = outLevels[0] + outLevels[1] + outLevels[2] + outLevels[3];
        if (sum > 0.001f)
        {
            const float inv = 1.0f / sum;
            for (int i = 0; i < 4; ++i)
                outLevels[i] *= inv;
        }
    }

    //==========================================================================
    //  S O N G   S T R U C T U R E   A D V A N C E
    //
    //  Called once per pitch period (approx.) to advance the hierarchical
    //  song state machine. Returns true if a gap is in progress (silence all).
    //
    //  targetFormants is updated in-place when theme drift is applied.
    //==========================================================================
    static bool advanceSongStructure(OrtolanVoice& v,
                                     float songLevel,
                                     int   phraseLength,
                                     float phraseGapFac,
                                     float themeDrift,
                                     float pitchDrift,
                                     float pitchPeriodSamp,
                                     float targetFormants[4]) noexcept
    {
        if (songLevel < 0.05f)
        {
            v.songPhase = 0.0f;
            return false;
        }

        // Currently in a phrase gap?
        if (v.song.inPhraseGap)
        {
            --v.song.phraseGapLeft;
            if (v.song.phraseGapLeft <= 0)
            {
                v.song.inPhraseGap = false;
                // Per-phrase pitch drift (level 1+)
                if (pitchDrift > 0.001f)
                {
                    const float driftSemitones =
                        (xorRand(v.rng) * 2.0f - 1.0f) * pitchDrift * 2.0f;
                    v.song.pitchDriftAcc = std::clamp(
                        v.song.pitchDriftAcc + driftSemitones, -3.0f, 3.0f);
                }
            }
            return true;
        }

        // Currently in a theme gap?
        if (v.song.inThemeGap)
        {
            --v.song.themeGapLeft;
            if (v.song.themeGapLeft <= 0)
            {
                v.song.inThemeGap    = false;
                v.song.currentPhrase = 0;
                v.song.currentBurst  = 0;

                // Per-theme formant drift (level 2+)
                if (songLevel > 0.55f && themeDrift > 0.001f)
                {
                    const float formantDriftHz =
                        (xorRand(v.rng) * 2.0f - 1.0f) * themeDrift * 400.0f;
                    v.song.formantDriftAcc = std::clamp(
                        v.song.formantDriftAcc + formantDriftHz, -800.0f, 800.0f);
                    for (int f = 0; f < 4; ++f)
                        targetFormants[f] = std::max(20.0f,
                            targetFormants[f] + v.song.formantDriftAcc * 0.05f);
                }
            }
            return true;
        }

        // Normal operation -- advance burst counter
        ++v.song.currentBurst;
        if (v.song.currentBurst >= phraseLength)
        {
            v.song.currentBurst = 0;
            ++v.song.currentPhrase;

            // Phrases per theme: 2 at songLevel=0.3, up to 6 at songLevel=1.0
            const int themePhraseCount = 2 + static_cast<int>(songLevel * 4.0f);

            if (v.song.currentPhrase >= themePhraseCount && songLevel > 0.25f)
            {
                // End of theme -- enter theme gap
                v.song.currentPhrase = 0;
                v.song.inThemeGap    = true;
                v.song.themeGapLeft  = static_cast<int>(pitchPeriodSamp * phraseGapFac * 4.0f);
                v.song.themeGapLeft  = std::max(1, v.song.themeGapLeft);
            }
            else
            {
                // End of phrase -- enter phrase gap
                v.song.inPhraseGap   = true;
                v.song.phraseGapLeft = static_cast<int>(pitchPeriodSamp * phraseGapFac * 2.0f);
                v.song.phraseGapLeft = std::max(1, v.song.phraseGapLeft);
            }
        }

        // Update song phase (0..1 within the full hierarchical cycle)
        const int themePhraseCount  = 2 + static_cast<int>(songLevel * 4.0f);
        const int totalBursts       = phraseLength * themePhraseCount;
        const int globalBurst       = v.song.currentPhrase * phraseLength + v.song.currentBurst;
        v.songPhase = static_cast<float>(globalBurst) / static_cast<float>(std::max(1, totalBursts));

        return false;
    }

    //==========================================================================
    //  N O T E   O N  /  O F F
    //==========================================================================

    void handleNoteOn(int noteNum, int midiVel,
                      float ampAtk, float ampDec, float ampSus, float ampRel,
                      float fltAtk, float fltDec, float fltSus, float fltRel,
                      int voiceMode, float glideTime) noexcept
    {
        const float vel    = static_cast<float>(midiVel) / 127.0f;
        const int   maxPoly = (voiceMode == 0 || voiceMode == 1) ? 1
                            : (voiceMode == 2) ? 4 : 8;

        // Legato: retrigger the current active voice without resetting generators
        if (voiceMode == 1)
        {
            for (int i = 0; i < maxPoly; ++i)
            {
                if (voices[i].active)
                {
                    voices[i].ampEnv.retriggerFrom(
                        voices[i].ampEnv.getLevel(), ampAtk, ampDec, ampSus, ampRel);
                    voices[i].filterEnv.noteOn();
                    voices[i].filterEnv.setADSR(fltAtk, fltDec, fltSus, fltRel);
                    if (glideTime < 0.0001f)
                        voices[i].glideFreq = midiToFreq(noteNum);
                    voices[i].note      = noteNum;
                    voices[i].velocity  = vel;
                    voices[i].keyTrack  = (static_cast<float>(noteNum) - 60.0f) / 60.0f;
                    voices[i].releasing = false;
                    return;
                }
            }
        }

        // Find a free voice
        int voiceIdx = -1;
        for (int i = 0; i < maxPoly; ++i)
        {
            if (!voices[i].active)
            {
                voiceIdx = i;
                break;
            }
        }
        // Steal the first releasing voice
        if (voiceIdx < 0)
        {
            for (int i = 0; i < maxPoly; ++i)
            {
                if (voices[i].releasing)
                {
                    voiceIdx = i;
                    break;
                }
            }
        }
        // Steal first voice
        if (voiceIdx < 0)
            voiceIdx = 0;

        auto& v = voices[voiceIdx];

        // Initialise glide frequency
        if (glideTime < 0.0001f || !v.active)
            v.glideFreq = midiToFreq(noteNum);
        // else: keep current glideFreq -- it will slide toward the new note

        v.note      = noteNum;
        v.velocity  = vel;
        v.keyTrack  = (static_cast<float>(noteNum) - 60.0f) / 60.0f;
        v.active    = true;
        v.releasing = false;

        // Reset VOSIM generators and song structure on new voice
        for (int i = 0; i < 4; ++i)
            v.gen[i].reset();
        v.song.reset();
        v.songPhase          = 0.0f;
        v.pitchPeriodAccum   = 0.0f;

        // Envelopes
        v.ampEnv.setParams(ampAtk, ampDec, ampSus, ampRel, sampleRateFloat);
        v.ampEnv.noteOn();
        v.filterEnv.setParams(fltAtk, fltDec, fltSus, fltRel, sampleRateFloat);
        v.filterEnv.noteOn();

        // Stagger LFO phases by voice index for ensemble width
        const float phaseOff = static_cast<float>(voiceIdx) / static_cast<float>(kOrtolanMaxVoices);
        v.lfo1.reset(phaseOff);
        v.lfo2.reset(1.0f - phaseOff);

        // Reset DC blocker
        v.dcBlockX = 0.0f;
        v.dcBlockY = 0.0f;
        v.filterUpdateCounter = 0;
    }

    void handleNoteOff(int noteNum) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && !v.releasing && v.note == noteNum)
            {
                v.releasing = true;
                v.ampEnv.noteOff();
                v.filterEnv.noteOff();
            }
        }
    }

    //==========================================================================
    //  R E N D E R   V O I C E S   R A N G E
    //==========================================================================
    void renderVoicesRange(float* writeL, float* writeR,
                           int startSample, int endSample,
                           const float targetFormants[4],
                           const float genLevel[4],
                           int pulseCount, float decayFactor,
                           int pulseShape, bool dcBlockOn,
                           float pitchTrack, float baseFreq,
                           float songLevel, int phraseLength, float phraseGapFac,
                           float themeDrift, float pitchDrift,
                           float ampAtk, float ampDec, float ampSus, float ampRel,
                           float baseCutoff, float fltReso, int fltType,
                           float fltEnvAmt, float fltKeyTrack,
                           float fltAtk, float fltDec, float fltSus, float fltRel,
                           float lfo1Rate, float lfo1Depth, int lfo1Shape, int lfo1Tgt,
                           float lfo2Rate, float lfo2Depth, int lfo2Shape, int lfo2Tgt,
                           float velTimbre, float glideCoeff,
                           float modAmpLevel) noexcept
    {
        if (startSample >= endSample) return;

        for (auto& v : voices)
        {
            if (!v.active) continue;

            // Configure envelopes and LFOs (block-rate)
            v.ampEnv.setParams(ampAtk, ampDec, ampSus, ampRel, sampleRateFloat);
            v.filterEnv.setParams(fltAtk, fltDec, fltSus, fltRel, sampleRateFloat);

            v.lfo1.setRate(lfo1Rate, sampleRateFloat);
            v.lfo1.setShape(lfo1Shape);
            v.lfo2.setRate(lfo2Rate, sampleRateFloat);
            v.lfo2.setShape(lfo2Shape);

            // Apply per-voice song drift to local formant copy
            float voiceFormants[4];
            for (int f = 0; f < 4; ++f)
            {
                // formantDriftAcc is in Hz; pitchDriftAcc in semitones
                const float pitchRatio = fastPow2(v.song.pitchDriftAcc / 12.0f);
                voiceFormants[f] = std::max(20.0f,
                    (targetFormants[f] + v.song.formantDriftAcc * 0.05f) * pitchRatio);
            }

            // Set generator levels
            for (int g = 0; g < 4; ++g)
                v.gen[g].level = genLevel[g];

            // D001: velocity scales decay factor -- harder velocity = brighter (less decay per pulse)
            const float velDecayMod   = 1.0f - velTimbre * v.velocity * 0.3f;
            const float effectiveDecay = std::clamp(decayFactor * velDecayMod, 0.5f, 1.0f);

            // Initial filter coefficient setup
            v.outputFilterL.setCoefficients(baseCutoff, fltReso, sampleRateFloat);
            v.outputFilterR.setCoefficients(baseCutoff, fltReso, sampleRateFloat);
            {
                CytomicSVF::Mode mode;
                switch (fltType)
                {
                case 1:  mode = CytomicSVF::Mode::HighPass; break;
                case 2:  mode = CytomicSVF::Mode::BandPass; break;
                case 3:  mode = CytomicSVF::Mode::Notch;    break;
                default: mode = CytomicSVF::Mode::LowPass;  break;
                }
                v.outputFilterL.setMode(mode);
                v.outputFilterR.setMode(mode);
            }

            for (int i = startSample; i < endSample; ++i)
            {
                // ---- Advance glide frequency (Lesson 8: USE the glide) ----
                const float targetFreq = midiToFreq(v.note);
                if (glideCoeff > 0.0001f)
                    v.glideFreq = glideCoeff * v.glideFreq + (1.0f - glideCoeff) * targetFreq;
                else
                    v.glideFreq = targetFreq;

                // Note frequency (blending MIDI pitch with fixed base frequency)
                const float noteFreq        = lerp(baseFreq, v.glideFreq, pitchTrack);
                const float pitchPeriodSamp = sampleRateFloat / std::max(noteFreq, 1.0f);

                // ---- LFOs (per-sample -- actual output values, not proxy) ----
                const float lfo1Val = v.lfo1.process();
                const float lfo2Val = v.lfo2.process();
                v.lastLfo1Val = lfo1Val; // stored for mod matrix source population
                v.lastLfo2Val = lfo2Val;

                // LFO routing helper -- accumulates modulation into named targets
                float lfoVowelMod        = 0.0f;
                float lfoFormantShiftMod = 0.0f;
                float lfoSongLevelMod    = 0.0f;
                float lfoPulseCountMod   = 0.0f;

                auto routeLfo = [&](float lfoVal, float lfoDepth, int lfoTgt) noexcept
                {
                    const float mod = lfoVal * lfoDepth;
                    switch (lfoTgt)
                    {
                    case 0: lfoVowelMod        += mod;          break;  // Vowel +-1
                    case 1: lfoFormantShiftMod  += mod * 400.0f; break; // FormantShift +-400 Hz
                    case 2: lfoSongLevelMod     += mod * 0.3f;  break;  // SongLevel +-0.3
                    case 3: lfoPulseCountMod    += mod * 4.0f;  break;  // PulseCount +-4
                    default: break;
                    }
                };
                routeLfo(lfo1Val, lfo1Depth, lfo1Tgt);
                routeLfo(lfo2Val, lfo2Depth, lfo2Tgt);

                // Per-sample effective formants (LFO modulated)
                float sampleFormants[4];
                for (int f = 0; f < 4; ++f)
                {
                    const float vowelMod = lfoVowelMod
                        * (kOrtolanVowelFormants[2][f] - kOrtolanVowelFormants[0][f]) * 0.25f;
                    sampleFormants[f] = std::max(noteFreq,
                        voiceFormants[f] + lfoFormantShiftMod + vowelMod);
                }

                // Per-sample effective pulse count
                const int samplePulseCount = std::clamp(
                    pulseCount + static_cast<int>(lfoPulseCountMod), 1, 32);

                // Per-sample song level (LFO-modulated, clamped)
                const float sampleSongLevel = std::clamp(
                    songLevel + lfoSongLevelMod, 0.0f, 1.0f);

                // ---- Coupling FM for this sample ----
                const float fmSample = (i < maxBlock)
                    ? couplingFmBuf[static_cast<size_t>(i)]
                    : v.couplingFmAccum;

                // ---- VOSIM: sum 4 generators ----
                float vosimOut = 0.0f;
                for (int g = 0; g < 4; ++g)
                {
                    vosimOut += v.gen[g].tick(sampleFormants[g], pitchPeriodSamp,
                                              samplePulseCount, effectiveDecay,
                                              pulseShape, sampleRateFloat, fmSample);
                }
                vosimOut = flushDenormal(vosimOut);

                // ---- Song structure silence (phrase/theme gaps) ----
                // Advance song structure once per pitch period (tracked with accumulator)
                if (sampleSongLevel > 0.05f)
                {
                    v.pitchPeriodAccum += 1.0f;
                    if (v.pitchPeriodAccum >= pitchPeriodSamp)
                    {
                        v.pitchPeriodAccum -= pitchPeriodSamp;
                        advanceSongStructure(v, sampleSongLevel, phraseLength, phraseGapFac,
                                             themeDrift, pitchDrift, pitchPeriodSamp,
                                             sampleFormants);
                    }

                    // Apply coupling rhythm: strong hit resets burst counter
                    if (couplingRhythm > 0.5f && !v.song.inPhraseGap && !v.song.inThemeGap)
                        v.song.currentBurst = 0;

                    // Silence output during phrase or theme gaps
                    if (v.song.inPhraseGap || v.song.inThemeGap)
                        vosimOut = 0.0f;
                }

                // ---- DC block (essential for unipolar cos^2) ----
                float dcOut = vosimOut;
                if (dcBlockOn)
                {
                    dcOut      = vosimOut - v.dcBlockX + 0.995f * v.dcBlockY;
                    v.dcBlockX = vosimOut;
                    v.dcBlockY = flushDenormal(dcOut);
                }

                // ---- Filter coefficient update every 16 samples (Lesson 4) ----
                if ((v.filterUpdateCounter & 15) == 0)
                {
                    const float fEnvLevel  = v.filterEnv.getLevel();
                    // Exponential keytracking: cutoff · 2^((note-60)·keyTrack/12).
                    // Was linear: keyTrack01 * fltKeyTrack * 5000 Hz, which was
                    // non-musical at the extremes (too much cut low, too much open high).
                    const float keyTrackMul = fastPow2(
                        static_cast<float>(v.note - 60) * fltKeyTrack * (1.0f / 12.0f));
                    // Bipolar env amt: |x| > eps (Lesson 7 -- negative sweeps are valid).
                    const float envHz = (std::fabs(fltEnvAmt) > 1e-6f)
                        ? fltEnvAmt * fEnvLevel * 8000.0f
                        : 0.0f;
                    const float effectiveCutoff = std::clamp(
                        baseCutoff * keyTrackMul + envHz, 20.0f, 20000.0f);

                    v.outputFilterL.setCoefficients_fast(effectiveCutoff, fltReso, sampleRateFloat);
                    v.outputFilterR.setCoefficients_fast(effectiveCutoff, fltReso, sampleRateFloat);
                }
                ++v.filterUpdateCounter;

                // ---- Filter ----
                const float filteredL = v.outputFilterL.processSample(dcOut);
                const float filteredR = v.outputFilterR.processSample(dcOut);

                // ---- Advance filter envelope per sample ----
                v.filterEnv.process();

                // ---- Amplitude envelope + VCA ----
                const float ampLevel = v.ampEnv.process();
                const float finalAmp = std::clamp(
                    ampLevel * v.velocity * (1.0f + modAmpLevel * 0.3f), 0.0f, 1.5f);

                writeL[i] += filteredL * finalAmp;
                writeR[i] += filteredR * finalAmp;

                // ---- Voice done? ----
                if (!v.ampEnv.isActive())
                {
                    v.active    = false;
                    v.releasing = false;
                    break;
                }
            }
        }
    }

    //==========================================================================
    //  M E M B E R S
    //==========================================================================

    // Runtime state (set in prepare())
    // Do not default-init — must be set by prepare() on the live sample rate.
    // Sentinel 0.0 makes misuse before prepare() a crash instead of silent wrong-rate DSP.
    float sampleRateFloat = 0.0f;
    int   maxBlock        = 512;

    // Polyphonic voices
    std::array<OrtolanVoice, kOrtolanMaxVoices> voices;

    // Coupling state
    std::vector<float> couplingFmBuf;   // per-sample FM coupling (AudioToFM)
    float couplingAmpFilter = 0.0f;     // block-rate AmpToFilter accumulator
    float couplingEnvVowel  = 0.0f;     // block-rate EnvToMorph accumulator
    float couplingRhythm    = 0.0f;     // block-rate RhythmToBlend accumulator
    float couplingFmLevel   = 0.0f;     // scalar scale for FM coupling

    // Expression state (CC1 mod wheel + aftertouch)
    float modWheelValue   = 0.0f;
    float aftertouchValue = 0.0f;

    // Coupling output cache (O(1) getSampleForCoupling reads)
    float lastSampleL   = 0.0f;
    float lastSampleR   = 0.0f;
    float lastSongPhase = 0.0f;

    // Mod matrix (4 slots)
    ModMatrix<4> modMatrix;
    ModMatrix<4>::Sources blockModSrc;

    //==========================================================================
    //  P A R A M E T E R   P O I N T E R S
    //==========================================================================

    // Group A: VOSIM Core
    std::atomic<float>* pVowel         = nullptr;
    std::atomic<float>* pPulseCount    = nullptr;
    std::atomic<float>* pDecay         = nullptr;
    std::atomic<float>* pFormantShift  = nullptr;
    std::atomic<float>* pFormantSpread = nullptr;
    std::atomic<float>* pGeneratorMix  = nullptr;
    std::atomic<float>* pBrightness    = nullptr;

    // Group B: Song Structure
    std::atomic<float>* pSongLevel    = nullptr;
    std::atomic<float>* pPhraseLength = nullptr;
    std::atomic<float>* pPhraseGap    = nullptr;
    std::atomic<float>* pThemeDrift   = nullptr;
    std::atomic<float>* pPitchDrift   = nullptr;

    // Group C: Waveform
    std::atomic<float>* pPulseShape  = nullptr;
    std::atomic<float>* pDcBlock     = nullptr;
    std::atomic<float>* pPitchTrack  = nullptr;
    std::atomic<float>* pBaseFreq    = nullptr;

    // Group D: Filter + Filter Envelope
    std::atomic<float>* pFltCutoff   = nullptr;
    std::atomic<float>* pFltReso     = nullptr;
    std::atomic<float>* pFltType     = nullptr;
    std::atomic<float>* pFltEnvAmt   = nullptr;
    std::atomic<float>* pFltKeyTrack = nullptr;
    std::atomic<float>* pFltAtk      = nullptr;
    std::atomic<float>* pFltDec      = nullptr;
    std::atomic<float>* pFltSus      = nullptr;
    std::atomic<float>* pFltRel      = nullptr;

    // Group E: Amp Envelope
    std::atomic<float>* pAmpAtk    = nullptr;
    std::atomic<float>* pAmpDec    = nullptr;
    std::atomic<float>* pAmpSus    = nullptr;
    std::atomic<float>* pAmpRel    = nullptr;
    std::atomic<float>* pVelTimbre = nullptr;

    // Group F: LFOs
    std::atomic<float>* pLfo1Rate   = nullptr;
    std::atomic<float>* pLfo1Depth  = nullptr;
    std::atomic<float>* pLfo1Shape  = nullptr;
    std::atomic<float>* pLfo1Target = nullptr;
    std::atomic<float>* pLfo2Rate   = nullptr;
    std::atomic<float>* pLfo2Depth  = nullptr;
    std::atomic<float>* pLfo2Shape  = nullptr;
    std::atomic<float>* pLfo2Target = nullptr;

    // Group H: Macros + Voice
    std::atomic<float>* pMacro1    = nullptr;
    std::atomic<float>* pMacro2    = nullptr;
    std::atomic<float>* pMacro3    = nullptr;
    std::atomic<float>* pMacro4    = nullptr;
    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlide     = nullptr;
    std::atomic<float>* pGlideMode = nullptr;
};

} // namespace xoceanus
