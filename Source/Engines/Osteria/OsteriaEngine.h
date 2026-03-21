#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/ShoreSystem/ShoreSystem.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/VoiceAllocator.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace xomnibus {

//==============================================================================
//
//  O S T E R I A   E N G I N E
//  Ensemble Synthesis with Elastic Coupling & Timbral Memory
//
//  Gallery code: OSTERIA  |  Accent: Porto Wine #722F37
//  Companion engine: OSPREY (the ocean; OSTERIA is the shore)
//
//  -------------------------------------------------------------------------
//
//  Creature Identity:
//  In the XO_OX aquatic mythology, OSTERIA is the human answer to the
//  ocean's inhuman vastness. It lives in the Open Water zone of the water
//  column — the same depth as XOdyssey and XObese — but its nature is
//  fundamentally communal. Where OSPREY hears the ocean's voice, OSTERIA
//  hears the human voice answering it. The osteria is where the fisherman
//  goes after the sea. Where stories become songs. Where strangers become
//  an ensemble.
//
//  -------------------------------------------------------------------------
//
//  Synth Lineage & Technique:
//  OSTERIA channels the ensemble-synthesis tradition: modal resonator banks
//  (Physical Modelling Synthesis, a la Yamaha VL1 / Modartt Pianoteq) shaped
//  by formant profiles from real folk instrument spectral analysis (Guitarra
//  Portuguesa, Kora, Oud, Shakuhachi, Gamelan, etc.). The elastic coupling
//  between quartet voices draws on spring-mass physics models used in
//  physical string simulation (Julius O. Smith, Stanford CCRMA). The Tavern
//  Room is a Feedback Delay Network (FDN) reverb in the tradition of
//  Jot/Gerzon, with Householder-like mixing and per-shore absorption.
//
//  -------------------------------------------------------------------------
//
//  Architecture:
//  A jazz quartet (Bass, Harmony, Melody, Rhythm) stretches across 5
//  coastal cultures via the Shore System (Atlantic, Nordic, Mediterranean,
//  Pacific, Southern). Each voice independently absorbs the local folk
//  instrument character through formant resonator banks. Voices are
//  connected by spring forces — elastic rubber-band coupling pulls them
//  toward a shared centroid, creating tension when stretched and musical
//  unity when tight. Cross-pollination memory means borrowed influences
//  persist — the quartet accumulates a living history of everywhere
//  it's been.
//
//  Signal chain:
//    MIDI -> 4 Quartet Channels (excitation -> formant resonators)
//         -> Sympathy crossfeed -> DC blocker -> Soft limiter
//         -> Patina / Porto / Smoke character stages
//         -> Tavern Room (FDN reverb) + Murmur (crowd texture)
//         -> Session Delay -> Hall (allpass) -> Chorus -> Tape
//         -> Stereo output
//
//  Features:
//    - 4 quartet channels per MIDI voice (Bass, Harmony, Melody, Rhythm)
//    - Per-channel shore position (0-4) with continuous morphing
//    - Elastic coupling: spring-force model between channels
//    - Timbral memory: circular buffer of shore history per channel
//    - Tavern room model: FDN reverb with per-shore acoustic character
//    - Murmur generator: crowd/conversation texture via filtered noise
//    - Character stages: Patina (harmonic fold), Porto (tanh warmth),
//      Smoke (HF haze lowpass)
//    - Session delay, chorus, hall, tape FX
//    - 8-voice polyphony with LRU stealing + 5ms crossfade
//
//  Coupling (the OSPREY x OSTERIA diptych):
//    - Output: post-room stereo audio via getSampleForCoupling
//    - Input: AudioToWavetable (any engine becomes a shore the quartet
//             absorbs), AmpToFilter (external dynamics modulate elastic
//             tightness), AudioToFM (external audio excites the tavern
//             room), EnvToMorph (external envelope drives shore drift)
//
//==============================================================================

//==============================================================================
// Constants
//==============================================================================

// Maximum polyphony — 8 notes, each spawning a full 4-channel quartet
// (= 32 active resonator channels, 128 formant filters at peak load).
static constexpr int kOsteriaMaxVoices = 8;

static constexpr float kOsteriaPI = 3.14159265358979323846f;
static constexpr float kOsteriaTwoPi = 6.28318530717958647692f;

// Timbral memory depth — each quartet channel records its last 32 shore
// positions. At control rate (~2kHz), this captures roughly 16ms of
// positional history, enough for the palimpsest effect where accumulated
// travel stains the current timbre.
static constexpr int kMemoryBufferSize = 32;

//==============================================================================
// Quartet roles
//==============================================================================
enum class QuartetRole : int { Bass = 0, Harmony = 1, Melody = 2, Rhythm = 3 };

//==============================================================================
// QuartetChannel — One of four ensemble voices with shore-morphed formants.
//
// Each channel represents one member of the travelling jazz quartet:
//   Bass    — the low-end anchor (Guitarra, Hardingfele, Bouzouki, Koto, Cavaquinho)
//   Harmony — the chordal voice (Kora, Langspil, Oud, Conch, Valiha)
//   Melody  — the lead voice  (Uilleann Pipes, Kulning, Ney, Singing Bowl, Gamelan)
//   Rhythm  — the percussive pulse (Bodhran, Sami Drum, Darbuka, Taiko, Djembe)
//
// Each channel carries its own position on the continuous Shore axis (0.0-4.0),
// its own formant resonator bank shaped by the local folk instrument, and its
// own timbral memory — a palimpsest of every coastline the voice has visited.
//==============================================================================
struct QuartetChannel
{
    QuartetRole role = QuartetRole::Bass;

    // --- Shore position & elastic dynamics ---
    float shorePos = 0.0f;          // Current position on the Shore axis (0.0=Atlantic, 4.0=Southern)
    float targetShorePos = 0.0f;    // Where the user/macro wants this voice to be
    float shoreVelocity = 0.0f;     // Spring-mass velocity for elastic overshoot

    // --- Formant resonator bank ---
    // 4 Cytomic SVF bandpass filters model the spectral fingerprint of the
    // current shore's folk instrument. Frequencies, gains, and bandwidths are
    // derived from ShoreSystem.h's ResonatorProfile data — real spectral
    // analysis of instruments like Guitarra Portuguesa, Oud, Shakuhachi, etc.
    CytomicSVF formants[4];
    float formantFreqs[4] = { 300.0f, 1200.0f, 2800.0f, 5500.0f };
    float formantGains[4] = { 1.0f, 0.7f, 0.5f, 0.3f };
    float formantBandwidths[4] = { 80.0f, 150.0f, 200.0f, 300.0f };

    // --- Timbral memory ---
    // Circular buffer of recent shore positions. When memory > 0, the voice
    // blends its current formants with a weighted average of where it has been.
    // This is the "palimpsest" effect: the oud warmth from a Mediterranean
    // visit persists in the bass voice's timbre long after it has moved on.
    float memoryBuffer[kMemoryBufferSize] = {};
    int memoryWritePos = 0;
    float memoryCachedFormantFreqs[4] = { 300.0f, 1200.0f, 2800.0f, 5500.0f };

    // --- Per-channel oscillators ---
    float oscPhase = 0.0f;          // Primary oscillator phase accumulator
    float oscPhase2 = 0.0f;         // Secondary oscillator for shimmer/detuned partials
    float noiseState = 0.0f;        // Noise generator state

    // --- Rhythm channel transient ---
    float transientEnv = 0.0f;      // Sharp attack envelope for percussive bursts
    float transientPhase = 0.0f;    // Pulse rate phase (driven by ShoreRhythm data)

    // --- Mix ---
    float level = 1.0f;
    float pan = 0.0f;

    // --- Per-channel output (cached for sympathy crossfeed) ---
    float lastOutputL = 0.0f;
    float lastOutputR = 0.0f;

    // Linear congruential generator (LCG) for per-channel random values.
    // Constants from Numerical Recipes: multiplier 1664525, increment 1013904223.
    uint32_t rng = 22222u;

    float nextRandom() noexcept
    {
        rng = rng * 1664525u + 1013904223u;   // LCG (Numerical Recipes constants)
        return static_cast<float> (rng & 0xFFFF) / 32768.0f - 1.0f;  // bipolar [-1, +1)
    }

    void recordShorePosition() noexcept
    {
        memoryBuffer[memoryWritePos] = shorePos;
        memoryWritePos = (memoryWritePos + 1) % kMemoryBufferSize;
    }

    void applyMemory (float memoryAmount, float sampleRate) noexcept
    {
        if (memoryAmount < 0.001f) return;

        // Compute average historical shore position
        float avgShore = 0.0f;
        for (int i = 0; i < kMemoryBufferSize; ++i)
            avgShore += memoryBuffer[i];
        avgShore /= static_cast<float> (kMemoryBufferSize);

        // Morph memory shore's resonator into cached formants
        ShoreMorphState memMorph = decomposeShore (avgShore);
        int slot = static_cast<int> (role);
        if (slot > 2) slot = 0; // rhythm uses bass slot
        ResonatorProfile memProfile = morphResonator (memMorph, slot);

        for (int i = 0; i < 4; ++i)
        {
            memoryCachedFormantFreqs[i] = lerp (formantFreqs[i],
                                                memProfile.formantFreqs[i],
                                                memoryAmount);
        }
    }

    void updateFormants (float sampleRate, float memoryAmount) noexcept
    {
        for (int i = 0; i < 4; ++i)
        {
            float freq = (memoryAmount > 0.001f) ? memoryCachedFormantFreqs[i] : formantFreqs[i];
            float bw = formantBandwidths[i];
            float resonance = clamp (1.0f - (bw / std::max (freq, 20.0f)), 0.0f, 0.95f);
            formants[i].setMode (CytomicSVF::Mode::BandPass);
            formants[i].setCoefficients (freq, resonance, sampleRate);
        }
    }

    void reset() noexcept
    {
        shorePos = 0.0f;
        targetShorePos = 0.0f;
        shoreVelocity = 0.0f;
        oscPhase = 0.0f;
        oscPhase2 = 0.0f;
        noiseState = 0.0f;
        transientEnv = 0.0f;
        transientPhase = 0.0f;
        lastOutputL = 0.0f;
        lastOutputR = 0.0f;
        memoryWritePos = 0;
        for (int i = 0; i < kMemoryBufferSize; ++i) memoryBuffer[i] = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            formants[i].reset();
            memoryCachedFormantFreqs[i] = formantFreqs[i];
        }
    }
};

//==============================================================================
// TavernRoom — FDN reverb modeling seaside tavern acoustics.
//
// A 4-line Feedback Delay Network (FDN) in the tradition of Jot (1992) and
// Gerzon. Each delay line represents a reflection path within the tavern
// space. The mixing matrix is Householder-like (average minus half of self),
// which provides energy-preserving decorrelation between lines.
//
// Per-shore tavern character (from ShoreSystem.h TavernCharacter data):
//   Atlantic: stone walls, low ceiling, wood bar, fireplace
//   Nordic:   deep timber paneling, heavy insulation, warm hearth
//   Mediterranean: open-air terrace, tile floor, sea breeze
//   Pacific:  paper screens, tatami absorption, garden beyond
//   Southern: corrugated roof, open sides, tropical air
//==============================================================================
struct TavernRoom
{
    static constexpr int kMaxDelay = 4096;
    float delayBuf[4][kMaxDelay] = {};
    int delayWritePos = 0;

    // Base delay lengths in samples — chosen as co-prime values to minimize
    // periodic coloration. These are the "room shape" at the reference room
    // size, then scaled by the shore's roomSizeMs.
    int delayLengths[4] = { 337, 509, 677, 883 };

    float feedback = 0.3f;           // Derived from RT60 via Sabine-like calculation
    CytomicSVF absorptionFilter;     // Models high-frequency air/wall absorption
    float warmthGain = 1.0f;         // Low-frequency boost from tavern warmth

    void setCharacter (const TavernCharacter& tavernCharacter, float mix, float sampleRate) noexcept
    {
        // Scale base delay lengths by room size ratio.
        // Reference room size is 15ms (small pub). Larger rooms stretch all delays.
        float roomScale = tavernCharacter.roomSizeMs / 15.0f;
        delayLengths[0] = std::max (1, std::min (kMaxDelay - 1, static_cast<int> (337.0f * roomScale)));
        delayLengths[1] = std::max (1, std::min (kMaxDelay - 1, static_cast<int> (509.0f * roomScale)));
        delayLengths[2] = std::max (1, std::min (kMaxDelay - 1, static_cast<int> (677.0f * roomScale)));
        delayLengths[3] = std::max (1, std::min (kMaxDelay - 1, static_cast<int> (883.0f * roomScale)));

        // Derive feedback gain from RT60 (time for reverb tail to decay by 60dB).
        // Uses the relation: feedback = 10^(-3 * delayLength / (RT60 * sampleRate))
        // which ensures the tail decays to -60dB in the specified time.
        float rt60Samples = tavernCharacter.decayMs * 0.001f * sampleRate;
        feedback = (rt60Samples > 0.0f)
            ? std::pow (0.001f, static_cast<float> (delayLengths[0]) / rt60Samples)
            : 0.0f;
        feedback = clamp (feedback, 0.0f, 0.85f);   // Cap at 0.85 for stability

        // Absorption filter: lowpass that models HF energy loss per reflection.
        // High absorption (stone, soft furnishings) = low cutoff = darker tail.
        float cutoff = lerp (2000.0f, 12000.0f, 1.0f - tavernCharacter.absorption);
        absorptionFilter.setMode (CytomicSVF::Mode::LowPass);
        absorptionFilter.setCoefficients (cutoff, 0.0f, sampleRate);

        // Warmth boost: up to +50% gain on low-frequency content.
        warmthGain = 1.0f + tavernCharacter.warmth * 0.5f;
    }

    void processSample (float& inL, float& inR, float mix) noexcept
    {
        if (mix < 0.001f) return;

        float input = (inL + inR) * 0.5f;

        // Read from each delay line
        float d0 = readDelay (0);
        float d1 = readDelay (1);
        float d2 = readDelay (2);
        float d3 = readDelay (3);

        // Householder-like mixing matrix: each feedback signal is the average
        // of all lines minus half of itself. This provides good decorrelation
        // without requiring a full unitary matrix multiply.
        float sum = (d0 + d1 + d2 + d3) * 0.25f;
        float fb0 = sum - d0 * 0.5f;
        float fb1 = sum - d1 * 0.5f;
        float fb2 = sum - d2 * 0.5f;
        float fb3 = sum - d3 * 0.5f;

        // Apply absorption filter to the mixed signal
        float filtered = absorptionFilter.processSample (sum);

        // Write to delays: staggered input gain (1.0, 0.7, 0.5, 0.3) creates
        // early reflection density variation across delay lines.
        // flushDenormal prevents feedback paths from accumulating subnormal
        // floats, which cause severe CPU spikes on x86 (up to 100x slowdown)
        // when the FPU falls back to microcode for denormal arithmetic.
        writeDelay (0, input + flushDenormal (fb0 * feedback));
        writeDelay (1, input * 0.7f + flushDenormal (fb1 * feedback));
        writeDelay (2, input * 0.5f + flushDenormal (fb2 * feedback));
        writeDelay (3, input * 0.3f + flushDenormal (fb3 * feedback));

        delayWritePos = (delayWritePos + 1) % kMaxDelay;

        // Stereo wet signal: odd/even delay lines panned L/R
        float wetL = (d0 + d2) * 0.5f * warmthGain;
        float wetR = (d1 + d3) * 0.5f * warmthGain;

        inL = lerp (inL, inL + wetL, mix);
        inR = lerp (inR, inR + wetR, mix);
    }

    float readDelay (int line) const noexcept
    {
        int readPos = delayWritePos - delayLengths[line];
        if (readPos < 0) readPos += kMaxDelay;
        return delayBuf[line][readPos];
    }

    void writeDelay (int line, float value) noexcept
    {
        delayBuf[line][delayWritePos] = value;
    }

    void reset() noexcept
    {
        delayWritePos = 0;
        for (int l = 0; l < 4; ++l)
            for (int i = 0; i < kMaxDelay; ++i)
                delayBuf[l][i] = 0.0f;
        absorptionFilter.reset();
    }
};

//==============================================================================
// MurmurGenerator — Crowd and conversation texture.
//
// Models the ambient sound of a tavern full of people: the low rumble of
// conversation, the bright edge of laughter. Two formant bandpass filters
// shape white noise into a vowel-like texture:
//   Formant 1 (~350 Hz, Q=0.4): the chest resonance of voices, the hum
//   Formant 2 (~2-4 kHz, Q=0.3): sibilance, glass clinks, laughter brightness
//
// A slow 0.5 Hz LFO modulates formant positions to prevent static texture,
// simulating the natural ebb and flow of conversation volume.
//==============================================================================
struct MurmurGenerator
{
    uint32_t rng = 77777u;           // LCG state (Numerical Recipes constants)
    CytomicSVF formant1;             // Low vocal resonance band (~350 Hz)
    CytomicSVF formant2;             // High sibilance/brightness band (~2-4 kHz)
    float modPhase = 0.0f;           // Slow modulation LFO phase

    void prepare (float sampleRate) noexcept
    {
        formant1.setMode (CytomicSVF::Mode::BandPass);
        formant1.setCoefficients (350.0f, 0.4f, sampleRate);
        formant2.setMode (CytomicSVF::Mode::BandPass);
        formant2.setCoefficients (2500.0f, 0.3f, sampleRate);
    }

    float process (float brightness, float sampleRate) noexcept
    {
        // Generate white noise via LCG
        rng = rng * 1664525u + 1013904223u;
        float noise = static_cast<float> (rng & 0xFFFF) / 32768.0f - 1.0f;

        // Slow 0.5 Hz modulation — the ebb and flow of tavern conversation
        modPhase += 0.5f / std::max (1.0f, sampleRate);
        if (modPhase >= 1.0f) modPhase -= 1.0f;
        float mod = fastSin (modPhase * kOsteriaTwoPi);

        // Formant 1: vocal chest resonance, gently modulated +/- 50 Hz
        float lowFormantFreq = 350.0f + mod * 50.0f;
        // Formant 2: sibilance band, brightness-dependent (2-4 kHz range),
        // modulated +/- 200 Hz for natural variation
        float highFormantFreq = lerp (2000.0f, 4000.0f, brightness) + mod * 200.0f;

        formant1.setCoefficients (lowFormantFreq, 0.4f, sampleRate);
        formant2.setCoefficients (highFormantFreq, 0.3f, sampleRate);

        // Blend: 60% low vocal body, 40% high brightness
        float out = formant1.processSample (noise) * 0.6f
                  + formant2.processSample (noise) * 0.4f;

        // 0.15 scale factor keeps murmur well below instrument level —
        // it should be felt more than heard, like real background conversation
        return out * 0.15f;
    }

    void reset() noexcept
    {
        formant1.reset();
        formant2.reset();
        modPhase = 0.0f;
        rng = 77777u;
    }
};

//==============================================================================
// OsteriaVoice — Per-MIDI-note state containing the full quartet ensemble.
//
// Each MIDI note activates all 4 quartet channels simultaneously — Bass,
// Harmony, Melody, and Rhythm all respond to the same pitch, each
// interpreting it through their current shore's instrument character.
// Up to 8 voices can be active; the oldest is stolen with a 5ms crossfade
// when polyphony is exhausted.
//==============================================================================
struct OsteriaVoice
{
    // --- Voice state ---
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;         // Monotonic counter for LRU voice stealing

    // --- Pitch ---
    float targetFreq = 440.0f;      // Target frequency from MIDI note
    float currentTargetFreq = 440.0f; // Smoothed frequency (for glide/portamento)
    float glideCoeff = 1.0f;        // Glide smoothing coefficient (1.0 = instant)

    // --- The quartet ---
    std::array<QuartetChannel, 4> quartet;

    // --- Amplitude envelope ---
    StandardADSR ampEnv;

    // --- Voice stealing crossfade ---
    float fadeGain = 1.0f;          // 1.0 = full volume, fades to 0 during stealing
    bool fadingOut = false;         // True when this voice is being stolen

    // --- Control-rate decimation ---
    int controlCounter = 0;         // Counts samples until next control-rate update

    // --- Per-voice PRNG (LCG, Numerical Recipes constants) ---
    uint32_t rng = 12345u;

    // --- DC blocker state ---
    // First-order DC blocker: y[n] = x[n] - x[n-1] + R * y[n-1]
    // Required because asymmetric saturation (Patina, Porto) and formant
    // resonance can introduce DC offset that causes speaker excursion.
    float dcPrevInL = 0.0f, dcPrevOutL = 0.0f;
    float dcPrevInR = 0.0f, dcPrevOutR = 0.0f;

    /** Bipolar random value in [-1, +1). */
    float nextRandom() noexcept
    {
        rng = rng * 1664525u + 1013904223u;
        return static_cast<float> (rng & 0xFFFF) / 32768.0f - 1.0f;
    }

    /** Unipolar random value in [0, 1). Used for initial phase randomization. */
    float nextRandomUni() noexcept
    {
        rng = rng * 1664525u + 1013904223u;
        return static_cast<float> (rng & 0xFFFF) / 65536.0f;
    }

    void reset() noexcept
    {
        active = false;
        noteNumber = -1;
        velocity = 0.0f;
        targetFreq = 440.0f;
        currentTargetFreq = 440.0f;
        fadeGain = 1.0f;
        fadingOut = false;
        controlCounter = 0;
        rng = 12345u;
        dcPrevInL = 0.0f; dcPrevOutL = 0.0f;
        dcPrevInR = 0.0f; dcPrevOutR = 0.0f;
        ampEnv.reset();
        for (auto& ch : quartet)
            ch.reset();
    }
};

//==============================================================================
// OsteriaEngine — The main engine class.
//
// OSTERIA is the human answer to the ocean's inhuman vastness. A travelling
// jazz quartet that absorbs the folk instrument character of every coastline
// it visits, connected by elastic rubber-band coupling. The tavern is the
// room, the murmur is the crowd, the patina is the century of music
// embedded in the wood.
//
// Gallery: OSTERIA | Accent: Porto Wine #722F37 | Prefix: osteria_
// Water column: Open Water (same depth as ODYSSEY, OBESE, OSTINATO)
// Companion: OSPREY (together they form "The Diptych" — ocean and shore)
//==============================================================================


class OsteriaEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = kOsteriaMaxVoices;

    //==========================================================================
    // SynthEngine interface — Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        // Control rate: ~2 kHz. Elastic coupling, formant updates, and shore
        // morphing run at this decimated rate to save CPU. At 44.1 kHz this
        // yields controlRateDiv = 22, meaning one control update per 22 audio
        // samples (~0.5 ms period).
        controlRateDiv = std::max (1, static_cast<int> (srf / 2000.0f));
        controlDt = static_cast<float> (controlRateDiv) / srf;

        // Voice-stealing crossfade: 5ms to prevent clicks when the oldest
        // voice is stolen. The rate is samples-to-silence in that window.
        crossfadeRate = 1.0f / (0.005f * srf);

        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        // --- Initialize voices and assign quartet roles ---
        for (auto& v : voices) v.reset();
        for (auto& v : voices)
        {
            v.quartet[0].role = QuartetRole::Bass;
            v.quartet[1].role = QuartetRole::Harmony;
            v.quartet[2].role = QuartetRole::Melody;
            v.quartet[3].role = QuartetRole::Rhythm;
        }

        // --- Tavern room and murmur ---
        tavernRoom.reset();
        murmur.prepare (srf);

        // --- Character stage filters ---
        // Smoke: lowpass that rolls off HF, modeling woodfire haze in the air
        smokeFilter.setMode (CytomicSVF::Mode::LowPass);
        smokeFilter.setCoefficients (8000.0f, 0.0f, srf);
        // Warmth: low shelf boost at 300 Hz, modeling nearfield proximity effect
        warmthFilter.setMode (CytomicSVF::Mode::LowShelf);
        warmthFilter.setCoefficients (300.0f, 0.0f, srf, 3.0f);

        // --- Session delay ---
        sessionDelayWritePos = 0;
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < kSessionDelayMax; ++i)
                sessionDelayBuf[c][i] = 0.0f;

        // --- Chorus ---
        chorusWritePos = 0;
        chorusPhase = 0.0f;
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < kChorusBufSize; ++i)
                chorusBuf[c][i] = 0.0f;

        // --- Hall (allpass diffusion chain) ---
        for (int i = 0; i < 4; ++i)
        {
            hallWritePos[i] = 0;
            for (int s = 0; s < kHallDelayMax; ++s)
                hallBuf[i][s] = 0.0f;
        }

        aftertouch.prepare (sampleRate);

        silenceGate.prepare (sampleRate, maxBlockSize);
        silenceGate.setHoldTime (500.0f);  // Osteria has hall reverb tails
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices) v.reset();
        envelopeOutput = 0.0f;
        couplingExcitationMod = 0.0f;
        couplingElasticMod = 0.0f;
        couplingRoomExcitation = 0.0f;
        couplingShoreDrift = 0.0f;
        tavernRoom.reset();
        murmur.reset();
        smokeFilter.reset();
        warmthFilter.reset();

        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);

        sessionDelayWritePos = 0;
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < kSessionDelayMax; ++i)
                sessionDelayBuf[c][i] = 0.0f;

        chorusWritePos = 0;
        chorusPhase = 0.0f;
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < kChorusBufSize; ++i)
                chorusBuf[c][i] = 0.0f;

        for (int i = 0; i < 4; ++i)
        {
            hallWritePos[i] = 0;
            for (int s = 0; s < kHallDelayMax; ++s)
                hallBuf[i][s] = 0.0f;
        }
    }

    //==========================================================================
    // SynthEngine interface — Audio
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        // =====================================================================
        // ParamSnapshot — cache all APVTS parameter values once per block.
        // This pattern (used across all XOmnibus engines) avoids atomic loads
        // per sample, which is critical at 128 formant filters peak load.
        // =====================================================================

        // -- Quartet shore positions (0.0=Atlantic .. 4.0=Southern) --
        const float pBassShore    = loadParam (paramBassShore, 0.0f);
        const float pHarmShore    = loadParam (paramHarmShore, 0.0f);
        const float pMelShore     = loadParam (paramMelShore, 0.0f);
        const float pRhythmShore  = loadParam (paramRhythmShore, 0.0f);

        // -- Elastic coupling --
        const float pElastic      = loadParam (paramElastic, 0.5f);   // Spring constant
        const float pStretch      = loadParam (paramStretch, 0.5f);   // Tension threshold
        const float pMemory       = loadParam (paramMemory, 0.3f);    // Timbral palimpsest amount
        const float pSympathy     = loadParam (paramSympathy, 0.3f);  // Inter-voice resonance

        // -- Voice balance --
        const float pBassLevel    = loadParam (paramBassLevel, 0.8f);
        const float pHarmonyLevel = loadParam (paramHarmLevel, 0.7f);
        const float pMelodyLevel  = loadParam (paramMelLevel, 0.7f);
        const float pRhythmLevel  = loadParam (paramRhythmLevel, 0.6f);
        const float pEnsWidth     = loadParam (paramEnsWidth, 0.5f);
        const float pBlend        = loadParam (paramBlendMode, 0.0f);

        // -- Tavern environment --
        const float pTavernMix    = loadParam (paramTavernMix, 0.3f);
        const float pTavernShore  = loadParam (paramTavernShore, 0.0f);
        const float pMurmur       = loadParam (paramMurmur, 0.2f);
        const float pWarmth       = loadParam (paramWarmth, 0.5f);
        const float pOceanBleed   = loadParam (paramOceanBleed, 0.1f);

        // -- Character stages --
        const float pPatina       = loadParam (paramPatina, 0.2f);    // Harmonic aging fold
        const float pPorto        = loadParam (paramPorto, 0.0f);     // Wine-dark saturation
        const float pSmoke        = loadParam (paramSmoke, 0.1f);     // HF haze lowpass

        // -- Amplitude envelope --
        const float pAmpAttack    = loadParam (paramAttack, 0.05f);
        const float pAmpDecay     = loadParam (paramDecay, 0.3f);
        const float pAmpSustain   = loadParam (paramSustain, 0.7f);
        const float pAmpRelease   = loadParam (paramRelease, 1.0f);

        // -- FX --
        const float pDelay        = loadParam (paramSessionDelay, 0.2f);
        const float pHall         = loadParam (paramHall, 0.2f);
        const float pChorus       = loadParam (paramChorus, 0.1f);
        const float pTape         = loadParam (paramTape, 0.0f);

        // -- Macros (M1-M4: CHARACTER, MOVEMENT, COUPLING, SPACE) --
        const float macroCharacter = loadParam (paramMacroCharacter, 0.0f);
        const float macroMovement  = loadParam (paramMacroMovement, 0.0f);
        const float macroCoupling  = loadParam (paramMacroCoupling, 0.0f);
        const float macroSpace     = loadParam (paramMacroSpace, 0.0f);

        // =====================================================================
        // Apply macros — each macro modulates multiple parameters to create
        // musically coherent sweeps across the engine's character space.
        // =====================================================================

        // M1 CHARACTER: At 0 = spread (diverse shores). At 1 = converged (unified folk ensemble).
        float effectiveBlend   = clamp (pBlend + macroCharacter * 0.8f, 0.0f, 1.0f);
        float convergence      = macroCharacter;

        // M2 MOVEMENT: At 0 = tight ensemble. At 1 = nomadic wandering.
        // Loosens elastic (inverse), widens stretch threshold.
        float effectiveElastic = clamp (pElastic - macroMovement * 0.7f, 0.0f, 1.0f);
        float effectiveStretch = clamp (pStretch + macroMovement * 0.4f, 0.01f, 1.0f);

        // M3 COUPLING: At 0 = independent voices. At 1 = deep inter-voice influence + heavy memory.
        float effectiveSympathy = clamp (pSympathy + macroCoupling * 0.5f, 0.0f, 1.0f);
        float effectiveMemory  = clamp (pMemory + macroCoupling * 0.5f, 0.0f, 1.0f);

        // M4 SPACE: At 0 = open air. At 1 = deep inside the pub (warm, intimate, sheltered).
        float effectiveTavern  = clamp (pTavernMix + macroSpace * 0.6f, 0.0f, 1.0f);
        float effectiveHall    = clamp (pHall + macroSpace * 0.5f, 0.0f, 1.0f);
        float effectiveBleed   = clamp (pOceanBleed + macroSpace * 0.5f, 0.0f, 1.0f);

        // Per-channel mix levels (Bass, Harmony, Melody, Rhythm)
        const float channelLevels[4] = { pBassLevel, pHarmonyLevel, pMelodyLevel, pRhythmLevel };

        // Per-channel stereo pan positions, spread proportionally by ensemble width.
        // Bass slightly left, Harmony center-left, Melody center-right, Rhythm right —
        // mimicking a natural stage layout for a quartet.
        const float channelPans[4] = {
            -0.3f * pEnsWidth,   // Bass: left of center
            -0.1f * pEnsWidth,   // Harmony: just left of center
             0.2f * pEnsWidth,   // Melody: right of center
             0.4f * pEnsWidth    // Rhythm: further right
        };

        // Shore targets per channel
        float shoreTargets[4] = { pBassShore, pHarmShore, pMelShore, pRhythmShore };

        // Apply convergence (M1): blend toward centroid
        if (convergence > 0.001f)
        {
            float centroid = (shoreTargets[0] + shoreTargets[1] + shoreTargets[2] + shoreTargets[3]) * 0.25f;
            for (int i = 0; i < 4; ++i)
                shoreTargets[i] = lerp (shoreTargets[i], centroid, convergence);
        }

        // Apply coupling shore drift
        float driftOffset = couplingShoreDrift;
        couplingShoreDrift = 0.0f;
        for (int i = 0; i < 4; ++i)
            shoreTargets[i] = clamp (shoreTargets[i] + driftOffset, 0.0f, 4.0f);

        // D005/D002: User LFO — shore drift breathing.
        // Rate: 0.005 Hz (200s cycle, deep breathing) at M2=0, up to 2 Hz at M2=1.
        // Depth: fixed at 0.4 shore units — enough to drift across ~1 coastline.
        // Each quartet channel gets a phase-offset version for organic spread.
        float userLfoRate = 0.005f + macroMovement * 1.995f;
        userLFO.setRate (userLfoRate, srf);
        float lfoOut = userLFO.process();
        static constexpr float kUserLfoShoreDepth = 0.4f;
        for (int i = 0; i < 4; ++i)
        {
            // Stagger channels by 0.25 of the LFO output using simple phase rotation
            float channelLfo = lfoOut * std::cos (kOsteriaPI * 0.5f * static_cast<float> (i))
                             + std::sin (kOsteriaPI * 0.5f * static_cast<float> (i)) * lfoOut * 0.5f;
            shoreTargets[i] = clamp (shoreTargets[i] + channelLfo * kUserLfoShoreDepth, 0.0f, 4.0f);
        }

        // Setup tavern room
        ShoreMorphState tavernMorph = decomposeShore (pTavernShore);
        TavernCharacter tc = morphTavern (tavernMorph);
        tavernRoom.setCharacter (tc, effectiveTavern, srf);

        // D001: filter envelope — compute peak velocity × ampLevel across active voices.
        // This is the primary mechanism for D001 compliance in Osteria: harder/fresher
        // hits open the smoke filter upward, making the ensemble brighter on attack.
        // kOsteriaFilterEnvMaxHz = 6000 Hz: at depth=0.25, full vel+env adds +1500 Hz.
        {
            static constexpr float kOsteriaFilterEnvMaxHz = 6000.0f;
            const float filterEnvDepth = loadParam (paramFilterEnvDepth, 0.25f);
            float peakVelEnv = 0.0f;
            for (const auto& voice : voices)
            {
                if (voice.active)
                {
                    float velEnv = voice.velocity * voice.ampEnv.level;
                    peakVelEnv = std::max (peakVelEnv, velEnv);
                }
            }
            filterEnvBoost = filterEnvDepth * peakVelEnv * kOsteriaFilterEnvMaxHz;
        }

        // Setup smoke filter
        // D006: mod wheel thickens the woodfire smoke — full wheel adds up to
        // 4 kHz of additional LPF roll-off, drawing the ensemble deeper into
        // the hazy warmth of the tavern interior.
        float smokeCutoff = lerp (18000.0f, 3000.0f, pSmoke) + filterEnvBoost
                            - modWheelAmount_ * 4000.0f;
        smokeCutoff = std::max (200.0f, std::min (20000.0f, smokeCutoff));
        smokeFilter.setCoefficients (smokeCutoff, 0.0f, srf);

        // Setup warmth filter
        float warmthDb = pWarmth * 8.0f;
        warmthFilter.setCoefficients (300.0f, 0.0f, srf, warmthDb);

        // Reset coupling accumulators
        float excitationMod = couplingExcitationMod;
        couplingExcitationMod = 0.0f;
        float elasticMod = couplingElasticMod;
        couplingElasticMod = 0.0f;
        float roomExcitation = couplingRoomExcitation;
        couplingRoomExcitation = 0.0f;

        // Effective elastic with coupling modulation
        effectiveElastic = clamp (effectiveElastic + elasticMod * 0.3f, 0.0f, 1.0f);

        // Glide coefficient
        float glideCoeff = 1.0f;

        // --- Process MIDI ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(),
                        shoreTargets, channelLevels, channelPans,
                        pAmpAttack, pAmpDecay, pAmpSustain, pAmpRelease);
            }
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
            else if (msg.isChannelPressure())
                aftertouch.setChannelPressure (msg.getChannelPressureValue() / 127.0f);
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount_ = msg.getControllerValue() / 127.0f;
            else if (msg.isPitchWheel()) pitchBendNorm = PitchBendUtil::parsePitchWheel (msg.getPitchWheelValue());
        }

        if (silenceGate.isBypassed() && midi.isEmpty()) { buffer.clear(); return; }

        aftertouch.updateBlock (numSamples);
        const float atPressure = aftertouch.getSmoothedPressure (0);

        // D006: aftertouch deepens tavern mix — more rhythmic folk character on pressure
        // (sensitivity 0.25). Full pressure adds up to +0.25 to effectiveTavern, pulling
        // the ensemble deeper into the FDN tavern room — the ensemble absorbs into the pub.
        effectiveTavern = clamp (effectiveTavern + atPressure * 0.25f, 0.0f, 1.0f);
        tavernRoom.setCharacter (tc, effectiveTavern, srf);

        float peakEnv = 0.0f;

        // --- Render sample loop ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                // Voice stealing crossfade
                if (voice.fadingOut)
                {
                    voice.fadeGain -= crossfadeRate;
                    if (voice.fadeGain <= 0.0f) { voice.fadeGain = 0.0f; voice.active = false; continue; }
                }

                // Glide
                voice.currentTargetFreq += (voice.targetFreq - voice.currentTargetFreq) * voice.glideCoeff;

                // Envelope
                float ampLevel = voice.ampEnv.process();
                if (!voice.ampEnv.isActive()) { voice.active = false; continue; }

                // --- Control-rate update ---
                voice.controlCounter++;
                if (voice.controlCounter >= controlRateDiv)
                {
                    voice.controlCounter = 0;

                    // --- Elastic coupling (spring-mass physics) ---
                    // Compute the ensemble centroid: the average shore position
                    // of all four quartet voices. This is the "center of gravity"
                    // that the rubber band pulls each voice toward.
                    float centroid = 0.0f;
                    for (int c = 0; c < 4; ++c)
                        centroid += voice.quartet[c].shorePos;
                    centroid *= 0.25f;

                    // Spring constant: scales with elastic parameter.
                    // At effectiveElastic=1.0, springK=4.0 (tight ensemble).
                    // At effectiveElastic=0.0, springK=0.0 (voices drift freely).
                    float springK = effectiveElastic * 4.0f;

                    // Stretch threshold: distance beyond which force increases
                    // quadratically (the rubber band "tightens"). Range 0.5-2.5
                    // shore units, controlled by the Stretch parameter.
                    float stretchThreshold = effectiveStretch * 2.0f + 0.5f;

                    for (int c = 0; c < 4; ++c)
                    {
                        auto& ch = voice.quartet[c];
                        ch.targetShorePos = shoreTargets[c];

                        // Linear spring force toward centroid (Hooke's law: F = -kx)
                        float dist = centroid - ch.shorePos;
                        float force = springK * dist;

                        // Quadratic force increase beyond stretch threshold —
                        // models the nonlinear stiffening of a real rubber band
                        // as it approaches its elastic limit.
                        float absDist = std::fabs (dist);
                        if (absDist > stretchThreshold)
                        {
                            float excess = absDist - stretchThreshold;
                            force += (dist > 0.0f ? 1.0f : -1.0f) * excess * excess * 2.0f;
                        }

                        // Additional pull toward user-set target position.
                        // Gain of 2.0 ensures voices converge on their targets
                        // within a musically useful timeframe (~200ms).
                        float targetPull = (ch.targetShorePos - ch.shorePos) * 2.0f;
                        force += targetPull;

                        // Integrate velocity and position (Euler integration)
                        ch.shoreVelocity += force * controlDt;
                        ch.shoreVelocity *= 0.95f; // Velocity damping (5% per step) prevents oscillation
                        ch.shorePos += ch.shoreVelocity * controlDt;
                        ch.shorePos = clamp (ch.shorePos, 0.0f, 4.0f);
                        // Flush denormals in velocity to prevent CPU spikes in
                        // the feedback loop when velocity decays toward zero.
                        ch.shoreVelocity = flushDenormal (ch.shoreVelocity);
                    }

                    // Update formant resonator coefficients per channel based
                    // on each voice's current shore position. The ShoreSystem
                    // provides 3 resonator slots per shore (bass/drone=0,
                    // chordal/harmonic=1, melodic/lead=2). The Rhythm channel
                    // (index 3) reuses the bass slot since its excitation is
                    // noise-based — the formants add body, not pitched content.
                    for (int c = 0; c < 4; ++c)
                    {
                        auto& ch = voice.quartet[c];
                        ShoreMorphState morph = decomposeShore (ch.shorePos);
                        int slot = c;
                        if (slot > 2) slot = 0;  // Rhythm channel uses bass resonator slot
                        ResonatorProfile prof = morphResonator (morph, slot);

                        // Scale formant frequencies relative to the note
                        float noteRatio = voice.currentTargetFreq / 440.0f;
                        for (int f = 0; f < 4; ++f)
                        {
                            ch.formantFreqs[f] = prof.formantFreqs[f] * noteRatio;
                            ch.formantFreqs[f] = clamp (ch.formantFreqs[f], 20.0f, 18000.0f);
                            ch.formantGains[f] = prof.formantGains[f];
                            ch.formantBandwidths[f] = prof.formantBandwidths[f];
                        }

                        // Apply timbral memory
                        ch.applyMemory (effectiveMemory, srf);
                        ch.updateFormants (srf, effectiveMemory);

                        // Record shore position
                        ch.recordShorePosition();
                    }

                    // Apply blend mode: at blend=1, force all channels toward centroid shore
                    if (effectiveBlend > 0.001f)
                    {
                        float blendCentroid = 0.0f;
                        for (int c = 0; c < 4; ++c)
                            blendCentroid += voice.quartet[c].shorePos;
                        blendCentroid *= 0.25f;

                        for (int c = 0; c < 4; ++c)
                        {
                            auto& ch = voice.quartet[c];
                            ch.shorePos = lerp (ch.shorePos, blendCentroid, effectiveBlend * 0.3f);
                        }
                    }
                }

                // ==========================================================
                // Audio-rate: render the four quartet channels.
                //
                // Each role has a distinct excitation spectrum fed into its
                // formant resonators. The formants do the heavy timbral
                // lifting — the excitation just provides the raw harmonic
                // content for the resonators to shape.
                // ==========================================================
                float voiceL = 0.0f, voiceR = 0.0f;
                float freq = voice.currentTargetFreq
                             * PitchBendUtil::semitonesToFreqRatio (pitchBendNorm * 2.0f);
                float phaseInc = freq / srf;

                for (int c = 0; c < 4; ++c)
                {
                    auto& ch = voice.quartet[c];
                    float excitation = 0.0f;

                    switch (static_cast<QuartetRole> (c))
                    {
                        case QuartetRole::Bass:
                        {
                            // Fundamental sine + sub-octave (0.5x frequency).
                            // 70/30 mix favors the fundamental while the sub
                            // adds low-end weight — like a bassist doubling
                            // with the left hand an octave below.
                            float fundamental = fastSin (ch.oscPhase * kOsteriaTwoPi);
                            float subOctave   = fastSin (ch.oscPhase * 0.5f * kOsteriaTwoPi);
                            excitation = fundamental * 0.7f + subOctave * 0.3f;
                            ch.oscPhase += phaseInc;
                            if (ch.oscPhase >= 1.0f) ch.oscPhase -= 1.0f;
                            break;
                        }
                        case QuartetRole::Harmony:
                        {
                            // Perfect fifth (1.5x) + slightly detuned octave (2.003x).
                            // The 0.003 detune creates paired-string shimmer,
                            // inspired by the Guitarra Portuguesa's paired courses
                            // and the Hardingfele's sympathetic strings.
                            float fifth        = fastSin (ch.oscPhase * kOsteriaTwoPi * 1.5f);
                            float detunedOctave = fastSin (ch.oscPhase2 * kOsteriaTwoPi * 2.003f);
                            excitation = fifth * 0.5f + detunedOctave * 0.5f;
                            ch.oscPhase += phaseInc;
                            ch.oscPhase2 += phaseInc;
                            if (ch.oscPhase >= 1.0f) ch.oscPhase -= 1.0f;
                            if (ch.oscPhase2 >= 1.0f) ch.oscPhase2 -= 1.0f;
                            break;
                        }
                        case QuartetRole::Melody:
                        {
                            // Upper partials: octave (2x), slightly sharp 5th+oct
                            // (3.01x), and double octave (4.02x). The micro-sharp
                            // detuning (0.01, 0.02) prevents phase-locked stasis
                            // and adds the natural inharmonicity of real
                            // instruments like Ney, Shakuhachi, and Kulning voices.
                            float octave    = fastSin (ch.oscPhase * kOsteriaTwoPi * 2.0f);
                            float twelfth   = fastSin (ch.oscPhase2 * kOsteriaTwoPi * 3.01f);
                            float dblOctave = fastSin (ch.oscPhase * kOsteriaTwoPi * 4.02f);
                            excitation = octave * 0.4f + twelfth * 0.35f + dblOctave * 0.25f;
                            ch.oscPhase += phaseInc;
                            ch.oscPhase2 += phaseInc;
                            if (ch.oscPhase >= 1.0f) ch.oscPhase -= 1.0f;
                            if (ch.oscPhase2 >= 1.0f) ch.oscPhase2 -= 1.0f;
                            break;
                        }
                        case QuartetRole::Rhythm:
                        {
                            // Noise burst with sharp transient envelope, driven
                            // by the shore's percussion pulse rate (Bodhran,
                            // Sami Drum, Darbuka, Taiko, Djembe).
                            ShoreMorphState rhythmMorph = decomposeShore (ch.shorePos);
                            ShoreRhythm rhythm = morphRhythm (rhythmMorph);

                            ch.transientPhase += rhythm.pulseRate / srf;
                            if (ch.transientPhase >= 1.0f)
                            {
                                ch.transientPhase -= 1.0f;
                                ch.transientEnv = 1.0f;  // Trigger a new transient
                            }
                            // Exponential decay: ~125 us time constant at 44.1 kHz
                            // (8.0 / srf), creating a sharp percussive attack.
                            ch.transientEnv *= (1.0f - 8.0f / srf);
                            // Flush denormals in the transient decay path — this
                            // decaying exponential will produce subnormal values
                            // as it approaches zero, causing CPU spikes without
                            // this guard.
                            ch.transientEnv = flushDenormal (ch.transientEnv);

                            float noise = ch.nextRandom();
                            excitation = noise * ch.transientEnv;
                            break;
                        }
                    }

                    // Add coupling excitation
                    excitation += excitationMod * 0.3f;

                    // Run through formant filters
                    float channelOut = 0.0f;
                    for (int f = 0; f < 4; ++f)
                    {
                        float filtered = ch.formants[f].processSample (excitation);
                        channelOut += filtered * ch.formantGains[f];
                    }

                    // Apply level
                    channelOut *= channelLevels[c];

                    // Stereo panning (constant power)
                    float panAngle = (channelPans[c] + 1.0f) * 0.25f * kOsteriaPI;
                    float panL = std::cos (panAngle);
                    float panR = std::sin (panAngle);

                    float chL = channelOut * panL;
                    float chR = channelOut * panR;

                    // Store for sympathy
                    ch.lastOutputL = chL;
                    ch.lastOutputR = chR;

                    voiceL += chL;
                    voiceR += chR;
                }

                // --- Sympathy crossfeed ---
                // Models how real acoustic instruments in close proximity
                // excite each other's resonators through air coupling (like
                // sympathetic strings on a Hardingfele or Sitar). Each
                // channel's output is fed into the other channels' primary
                // formant filter, creating subtle cross-resonance.
                if (effectiveSympathy > 0.001f)
                {
                    // Scale factor keeps sympathy subtle — it should add
                    // shimmer and life, not dominate the sound.
                    float sympathyGain = effectiveSympathy * 0.15f;
                    for (int c = 0; c < 4; ++c)
                    {
                        auto& ch = voice.quartet[c];
                        float sympathyInput = 0.0f;
                        for (int other = 0; other < 4; ++other)
                        {
                            if (other == c) continue;
                            sympathyInput += (voice.quartet[other].lastOutputL + voice.quartet[other].lastOutputR) * 0.5f;
                        }
                        // Feed through the primary (lowest) formant — this
                        // creates resonance at the instrument's fundamental
                        // frequency range, like a body resonance being excited.
                        float sympathyOut = ch.formants[0].processSample (sympathyInput * sympathyGain * 0.3f);
                        voiceL += sympathyOut * 0.3f;
                        voiceR += sympathyOut * 0.3f;
                    }
                }

                // --- DC Blocker ---
                // First-order high-pass: y[n] = x[n] - x[n-1] + R * y[n-1]
                // R = 0.9975 gives a -3dB point at ~17 Hz (inaudible), which
                // removes DC offset from formant resonance and asymmetric
                // saturation without affecting musical content.
                constexpr float dcBlockerCoeff = 0.9975f;
                float dcOutL = voiceL - voice.dcPrevInL + dcBlockerCoeff * voice.dcPrevOutL;
                float dcOutR = voiceR - voice.dcPrevInR + dcBlockerCoeff * voice.dcPrevOutR;
                voice.dcPrevInL = voiceL;
                // Flush denormals in the DC blocker feedback path — the
                // recursive coefficient 0.9975 creates a slowly decaying
                // series that will produce subnormals during silence.
                voice.dcPrevOutL = flushDenormal (dcOutL);
                voice.dcPrevInR = voiceR;
                voice.dcPrevOutR = flushDenormal (dcOutR);
                voiceL = dcOutL;
                voiceR = dcOutR;

                // --- Soft limiter ---
                // tanh(x * 1.5) provides gentle saturation that begins
                // compressing at ~0.67 and hard-limits at +/-1.0. The 1.5x
                // pre-gain pushes signal into the saturation knee, adding
                // warmth and preventing harsh digital clipping.
                voiceL = fastTanh (voiceL * 1.5f);
                voiceR = fastTanh (voiceR * 1.5f);

                // --- Apply envelope, velocity, and crossfade gain ---
                float gain = ampLevel * voice.velocity * voice.fadeGain;
                voiceL *= gain;
                voiceR *= gain;

                // Final denormal flush before voice summation
                voiceL = flushDenormal (voiceL);
                voiceR = flushDenormal (voiceR);

                mixL += voiceL;
                mixR += voiceR;

                peakEnv = std::max (peakEnv, ampLevel);
            }

            // ==========================================================
            // Post-voice processing — Character stages, Tavern, FX
            // ==========================================================

            // --- Character: Patina (gentle harmonic fold) ---
            // Models the aged quality of old wood and worn strings.
            // softClip + volume compensation (divide by drive) adds
            // subtle even harmonics without changing perceived volume.
            // Drive range: 1x-4x (at patina=1.0, drive=4.0).
            if (pPatina > 0.001f)
            {
                float patinaDrive = 1.0f + pPatina * 3.0f;
                mixL = softClip (mixL * patinaDrive) / patinaDrive;
                mixR = softClip (mixR * patinaDrive) / patinaDrive;
            }

            // --- Character: Porto (wine-dark warm saturation) ---
            // tanh waveshaping with volume compensation. Adds odd
            // harmonics for a richer, warmer drive character than
            // Patina. Named for Port wine — deep, warm, intoxicating.
            // Drive range: 1x-5x (at porto=1.0, drive=5.0).
            if (pPorto > 0.001f)
            {
                float portoDrive = 1.0f + pPorto * 4.0f;
                mixL = fastTanh (mixL * portoDrive) / portoDrive;
                mixR = fastTanh (mixR * portoDrive) / portoDrive;
            }

            // --- Character: Smoke (HF haze) ---
            // Lowpass filter modeling woodfire smoke in the tavern air.
            // At smoke=0, cutoff is 18 kHz (transparent). At smoke=1,
            // cutoff drops to 3 kHz (deeply hazy, lo-fi warmth).
            mixL = smokeFilter.processSample (mixL);
            mixR = smokeFilter.processSample (mixR);

            // --- Warmth (proximity EQ) ---
            // Low shelf boost at 300 Hz, modeling the nearfield proximity
            // effect of sitting close to the performers in a small room.
            // P0-02 fix: right channel now also passes through warmth filter
            mixL = warmthFilter.processSample (mixL);
            mixR = warmthFilter.processSample (mixR);

            // --- Tavern room (FDN reverb) ---
            tavernRoom.processSample (mixL, mixR, effectiveTavern);

            // --- Murmur (crowd texture) ---
            if (pMurmur > 0.001f)
            {
                ShoreMorphState murmurMorph = decomposeShore (pTavernShore);
                TavernCharacter murmurTavern = morphTavern (murmurMorph);
                float murmurSample = murmur.process (murmurTavern.murmurBrightness, srf);
                mixL += murmurSample * pMurmur;
                mixR += murmurSample * pMurmur * 0.9f; // 10% L/R difference for subtle stereo width
            }

            // --- Session Delay ---
            // Short conversational echo (~150ms), like the natural slap-back
            // in a stone-walled tavern. Feedback of 0.35 creates 2-3 audible
            // repeats before decay — enough for rhythmic interest without wash.
            if (pDelay > 0.001f)
            {
                int delayTimeSamples = std::max (1, std::min (kSessionDelayMax - 1,
                    static_cast<int> (0.15f * srf)));  // 150ms delay time
                int readPos = sessionDelayWritePos - delayTimeSamples;
                if (readPos < 0) readPos += kSessionDelayMax;

                float delayedL = sessionDelayBuf[0][readPos];
                float delayedR = sessionDelayBuf[1][readPos];

                // 0.35 feedback: ~2-3 audible repeats before -60dB
                sessionDelayBuf[0][sessionDelayWritePos] = mixL + delayedL * 0.35f;
                sessionDelayBuf[1][sessionDelayWritePos] = mixR + delayedR * 0.35f;
                sessionDelayWritePos = (sessionDelayWritePos + 1) % kSessionDelayMax;

                mixL += delayedL * pDelay;
                mixR += delayedR * pDelay;
            }

            // --- Hall (allpass diffusion chain) ---
            // 4-stage allpass chain creates late diffusion, scaling from
            // intimate pub corner (low hall) to cathedral harbor wall (high hall).
            if (effectiveHall > 0.001f)
            {
                float hallInput = (mixL + mixR) * 0.5f;
                float hallOutput = processHallAllpass (hallInput, effectiveHall);
                mixL += hallOutput * effectiveHall * 0.5f;
                mixR += hallOutput * effectiveHall * 0.5f;
            }

            // --- Chorus ---
            // Modulated delay (8ms center +/- 3ms at 0.5 Hz) creates
            // paired-string shimmer inspired by the Guitarra Portuguesa's
            // paired courses and the Hardingfele's sympathetic strings.
            if (pChorus > 0.001f)
            {
                chorusPhase += 0.5f / srf;  // 0.5 Hz LFO rate
                if (chorusPhase >= 1.0f) chorusPhase -= 1.0f;
                float modulatedDelayMs = 8.0f + fastSin (chorusPhase * kOsteriaTwoPi) * 3.0f;
                int modulatedDelaySamples = static_cast<int> (modulatedDelayMs * 0.001f * srf);
                modulatedDelaySamples = std::max (1, std::min (kChorusBufSize - 1, modulatedDelaySamples));

                int readPos = chorusWritePos - modulatedDelaySamples;
                if (readPos < 0) readPos += kChorusBufSize;

                float chorusL = chorusBuf[0][readPos];
                float chorusR = chorusBuf[1][readPos];

                chorusBuf[0][chorusWritePos] = mixL;
                chorusBuf[1][chorusWritePos] = mixR;
                chorusWritePos = (chorusWritePos + 1) % kChorusBufSize;

                mixL += chorusL * pChorus * 0.5f;
                mixR += chorusR * pChorus * 0.5f;
            }

            // --- Tape (lo-fi warmth) ---
            // Models field recording character: a one-pole lowpass (coeff 0.3
            // = ~2.1 kHz cutoff at 44.1 kHz) plus subtle noise floor (0.003
            // amplitude = ~-50 dB, typical of portable cassette recorders).
            // Gives the impression the session was captured on tape in the
            // tavern, not produced in a studio.
            if (pTape > 0.001f)
            {
                // One-pole lowpass: y[n] += (x[n] - y[n]) * alpha
                // alpha = 0.3 gives gentle HF rolloff without resonance
                tapeState[0] += (mixL - tapeState[0]) * 0.3f;
                tapeState[1] += (mixR - tapeState[1]) * 0.3f;
                // Flush denormals in the filter feedback path
                tapeState[0] = flushDenormal (tapeState[0]);
                tapeState[1] = flushDenormal (tapeState[1]);

                // Tape hiss: subtle noise floor at -50 dB
                murmur.rng = murmur.rng * 1664525u + 1013904223u;
                float tapeHiss = static_cast<float> (murmur.rng & 0xFFFF) / 65536.0f - 0.5f;

                mixL = lerp (mixL, tapeState[0] + tapeHiss * 0.003f, pTape);
                mixR = lerp (mixR, tapeState[1] + tapeHiss * 0.003f, pTape);
            }

            // Write output
            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample (0, sample, mixL);
                buffer.addSample (1, sample, mixR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample (0, sample, (mixL + mixR) * 0.5f);
            }

            // Coupling cache
            if (sample < static_cast<int> (outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t> (sample)] = mixL;
                outputCacheR[static_cast<size_t> (sample)] = mixR;
            }
        }

        envelopeOutput = peakEnv;

        int count = 0;
        for (const auto& v : voices)
            if (v.active) ++count;
        activeVoices = count;

        silenceGate.analyzeBlock (buffer.getReadPointer (0), buffer.getReadPointer (1), numSamples);
    }

    //==========================================================================
    // SynthEngine interface — Coupling
    //
    // OSTERIA's coupling interface implements "The Diptych" — the ocean-
    // shore relationship with OSPREY, and the universal absorption model
    // where any engine's output can become "the local music" of an
    // imaginary shore.
    //
    // Channel 0: post-room stereo left
    // Channel 1: post-room stereo right
    // Channel 2: peak envelope (for amplitude-driven coupling)
    //==========================================================================

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        auto sampleIdx = static_cast<size_t> (sampleIndex);
        if (channel == 0 && sampleIdx < outputCacheL.size()) return outputCacheL[sampleIdx];
        if (channel == 1 && sampleIdx < outputCacheR.size()) return outputCacheR[sampleIdx];
        if (channel == 2) return envelopeOutput;
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
            // AudioToWavetable: source audio shapes quartet excitation.
            // Any engine becomes a shore the quartet absorbs — feed OBSIDIAN
            // and the quartet plays crystal-inflected jazz.
            case CouplingType::AudioToWavetable:
                couplingExcitationMod += amount * 0.5f;
                break;

            // AmpToFilter: source amplitude modulates elastic tightness.
            // Storm (OSPREY) = frantic tight playing. Calm = relaxed session.
            case CouplingType::AmpToFilter:
                couplingElasticMod += amount * 0.3f;
                break;

            // AudioToFM: source audio excites the tavern room model.
            // Ocean turbulence (OSPREY) bleeds through the tavern walls.
            case CouplingType::AudioToFM:
                couplingRoomExcitation += amount * 0.4f;
                break;

            // EnvToMorph: source envelope drives shore drift.
            // External dynamics push voices between coastlines.
            case CouplingType::EnvToMorph:
                couplingShoreDrift += amount * 0.5f;
                break;

            default:
                break;
        }
    }

    //==========================================================================
    // SynthEngine interface — Parameters
    //==========================================================================

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl (params);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl (params);
        return { params.begin(), params.end() };
    }

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // --- Quartet ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_qBassShore", 1 }, "Osteria Bass Shore",
            juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_qHarmShore", 1 }, "Osteria Harmony Shore",
            juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_qMelShore", 1 }, "Osteria Melody Shore",
            juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_qRhythmShore", 1 }, "Osteria Rhythm Shore",
            juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_qElastic", 1 }, "Osteria Elastic",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_qStretch", 1 }, "Osteria Stretch",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_qMemory", 1 }, "Osteria Memory",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_qSympathy", 1 }, "Osteria Sympathy",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

        // --- Voice Balance ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_bassLevel", 1 }, "Osteria Bass Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_harmLevel", 1 }, "Osteria Harmony Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_melLevel", 1 }, "Osteria Melody Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_rhythmLevel", 1 }, "Osteria Rhythm Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_ensWidth", 1 }, "Osteria Ensemble Width",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_blendMode", 1 }, "Osteria Blend Mode",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        // --- Tavern ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_tavernMix", 1 }, "Osteria Tavern Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_tavernShore", 1 }, "Osteria Tavern Shore",
            juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_murmur", 1 }, "Osteria Murmur",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.2f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_warmth", 1 }, "Osteria Warmth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_oceanBleed", 1 }, "Osteria Ocean Bleed",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.1f));

        // --- Character ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_patina", 1 }, "Osteria Patina",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.2f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_porto", 1 }, "Osteria Porto",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_smoke", 1 }, "Osteria Smoke",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.1f));
        // D001: filter envelope depth — peak voice velocity × ampLevel boosts smoke cutoff.
        // The smokeFilter LPF (3kHz–18kHz) brightens on harder hits, satisfying D001.
        // Default 0.25: at full velocity and attack peak, adds +3750 Hz above smoke cutoff.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_filterEnvDepth", 1 }, "Osteria Filter Env Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.25f));

        // --- Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_attack", 1 }, "Osteria Attack",
            juce::NormalisableRange<float> (0.001f, 4.0f, 0.001f, 0.3f), 0.05f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_decay", 1 }, "Osteria Decay",
            juce::NormalisableRange<float> (0.05f, 4.0f, 0.001f, 0.3f), 0.3f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_sustain", 1 }, "Osteria Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_release", 1 }, "Osteria Release",
            juce::NormalisableRange<float> (0.05f, 8.0f, 0.001f, 0.3f), 1.0f));

        // --- FX ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_sessionDelay", 1 }, "Osteria Session Delay",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.2f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_hall", 1 }, "Osteria Hall",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.2f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_chorus", 1 }, "Osteria Chorus",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.1f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_tape", 1 }, "Osteria Tape",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        // --- Macros ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_macroCharacter", 1 }, "Osteria Macro CHARACTER",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_macroMovement", 1 }, "Osteria Macro MOVEMENT",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_macroCoupling", 1 }, "Osteria Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "osteria_macroSpace", 1 }, "Osteria Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramBassShore      = apvts.getRawParameterValue ("osteria_qBassShore");
        paramHarmShore      = apvts.getRawParameterValue ("osteria_qHarmShore");
        paramMelShore       = apvts.getRawParameterValue ("osteria_qMelShore");
        paramRhythmShore    = apvts.getRawParameterValue ("osteria_qRhythmShore");
        paramElastic        = apvts.getRawParameterValue ("osteria_qElastic");
        paramStretch        = apvts.getRawParameterValue ("osteria_qStretch");
        paramMemory         = apvts.getRawParameterValue ("osteria_qMemory");
        paramSympathy       = apvts.getRawParameterValue ("osteria_qSympathy");

        paramBassLevel      = apvts.getRawParameterValue ("osteria_bassLevel");
        paramHarmLevel      = apvts.getRawParameterValue ("osteria_harmLevel");
        paramMelLevel       = apvts.getRawParameterValue ("osteria_melLevel");
        paramRhythmLevel    = apvts.getRawParameterValue ("osteria_rhythmLevel");
        paramEnsWidth       = apvts.getRawParameterValue ("osteria_ensWidth");
        paramBlendMode      = apvts.getRawParameterValue ("osteria_blendMode");

        paramTavernMix      = apvts.getRawParameterValue ("osteria_tavernMix");
        paramTavernShore    = apvts.getRawParameterValue ("osteria_tavernShore");
        paramMurmur         = apvts.getRawParameterValue ("osteria_murmur");
        paramWarmth         = apvts.getRawParameterValue ("osteria_warmth");
        paramOceanBleed     = apvts.getRawParameterValue ("osteria_oceanBleed");

        paramPatina          = apvts.getRawParameterValue ("osteria_patina");
        paramPorto           = apvts.getRawParameterValue ("osteria_porto");
        paramSmoke           = apvts.getRawParameterValue ("osteria_smoke");
        paramFilterEnvDepth  = apvts.getRawParameterValue ("osteria_filterEnvDepth");

        paramAttack         = apvts.getRawParameterValue ("osteria_attack");
        paramDecay          = apvts.getRawParameterValue ("osteria_decay");
        paramSustain        = apvts.getRawParameterValue ("osteria_sustain");
        paramRelease        = apvts.getRawParameterValue ("osteria_release");

        paramSessionDelay   = apvts.getRawParameterValue ("osteria_sessionDelay");
        paramHall           = apvts.getRawParameterValue ("osteria_hall");
        paramChorus         = apvts.getRawParameterValue ("osteria_chorus");
        paramTape           = apvts.getRawParameterValue ("osteria_tape");

        paramMacroCharacter = apvts.getRawParameterValue ("osteria_macroCharacter");
        paramMacroMovement  = apvts.getRawParameterValue ("osteria_macroMovement");
        paramMacroCoupling  = apvts.getRawParameterValue ("osteria_macroCoupling");
        paramMacroSpace     = apvts.getRawParameterValue ("osteria_macroSpace");
    }

    //==========================================================================
    // SynthEngine interface — Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Osteria"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF722F37); } // Porto Wine #722F37
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoices; }

private:

    SilenceGate silenceGate;

    //==========================================================================
    // Helpers
    //==========================================================================

    static float loadParam (std::atomic<float>* p, float fallback) noexcept
    {
        return (p != nullptr) ? p->load() : fallback;
    }

    static float midiToHz (float midiNote) noexcept
    {
        return 440.0f * std::pow (2.0f, (midiNote - 69.0f) / 12.0f);
    }

    //==========================================================================
    // Hall — Allpass diffusion chain.
    //
    // 4 cascaded allpass filters create late diffusion that scales from
    // intimate pub corner to cathedral harbor wall. The allpass structure
    // (Schroeder, 1962) preserves frequency balance while smearing time —
    // ideal for creating a sense of space without coloration.
    //
    // Delay lengths are co-prime to minimize periodic artifacts:
    //   1051, 1399, 1747, 2083 samples (~24ms, 32ms, 40ms, 47ms at 44.1kHz)
    //==========================================================================

    static constexpr int kHallDelayMax = 4096;
    static constexpr int kHallDelayLengths[4] = { 1051, 1399, 1747, 2083 };

    float processHallAllpass (float input, float feedbackAmount) noexcept
    {
        float signal = input;

        // Allpass feedback coefficient: capped at 0.7 for stability.
        // At g > 0.7, the allpass becomes unstable and self-oscillates.
        float allpassCoeff = clamp (feedbackAmount * 0.5f, 0.0f, 0.7f);

        for (int i = 0; i < 4; ++i)
        {
            int readPos = hallWritePos[i] - kHallDelayLengths[i];
            if (readPos < 0) readPos += kHallDelayMax;

            float delayed = hallBuf[i][readPos];
            float writeVal = signal + delayed * allpassCoeff;
            // Flush denormals in the allpass feedback path
            hallBuf[i][hallWritePos[i]] = flushDenormal (writeVal);
            signal = delayed - signal * allpassCoeff;

            hallWritePos[i] = (hallWritePos[i] + 1) % kHallDelayMax;
        }

        return signal;
    }

    //==========================================================================
    // MIDI note handling
    //==========================================================================

    void noteOn (int noteNumber, float velocity,
                 const float shoreTargets[4],
                 const float channelLevels[4],
                 const float channelPans[4],
                 float attackSec, float decaySec, float sustainLevel, float releaseSec)
    {
        float freq = midiToHz (static_cast<float> (noteNumber));

        int voiceIndex = VoiceAllocator::findFreeVoice (voices, kMaxVoices);
        auto& voice = voices[static_cast<size_t> (voiceIndex)];

        // If stealing an active voice, initiate crossfade-out
        if (voice.active)
        {
            voice.fadingOut = true;
            voice.fadeGain = std::min (voice.fadeGain, 0.5f);
        }

        // Initialize voice state
        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.startTime = voiceCounter++;
        voice.targetFreq = freq;
        voice.currentTargetFreq = freq;
        voice.glideCoeff = 1.0f;       // 1.0 = no glide (instant pitch change)
        voice.fadingOut = false;
        voice.fadeGain = 1.0f;
        voice.controlCounter = 0;
        voice.dcPrevInL = 0.0f; voice.dcPrevOutL = 0.0f;
        voice.dcPrevInR = 0.0f; voice.dcPrevOutR = 0.0f;

        // Seed PRNG uniquely per note using co-prime multipliers (7919, 104729)
        // to ensure each voice gets a different random sequence for phase
        // initialization and noise generation.
        voice.rng = static_cast<uint32_t> (noteNumber * 7919 + voiceCounter * 104729);

        voice.ampEnv.setParams (attackSec, decaySec, sustainLevel, releaseSec, srf);
        voice.ampEnv.noteOn();

        // --- Initialize all four quartet channels ---
        for (int c = 0; c < 4; ++c)
        {
            auto& ch = voice.quartet[c];
            ch.role = static_cast<QuartetRole> (c);
            ch.shorePos = shoreTargets[c];
            ch.targetShorePos = shoreTargets[c];
            ch.shoreVelocity = 0.0f;
            ch.level = channelLevels[c];
            ch.pan = channelPans[c];

            // Randomize initial oscillator phases to prevent phase-locked
            // unison artifacts when multiple notes trigger simultaneously
            ch.oscPhase = voice.nextRandomUni();
            ch.oscPhase2 = voice.nextRandomUni();
            ch.transientEnv = 0.0f;
            ch.transientPhase = voice.nextRandomUni();

            // Per-channel PRNG seed: co-prime multipliers (7919, 31337, 54321)
            // ensure each channel within each voice has a unique sequence
            ch.rng = static_cast<uint32_t> (noteNumber * 7919 + c * 31337 + voiceCounter * 54321);
            ch.lastOutputL = 0.0f;
            ch.lastOutputR = 0.0f;

            // Initialize formant resonators from the channel's starting shore
            ShoreMorphState morph = decomposeShore (ch.shorePos);
            int slot = c;
            if (slot > 2) slot = 0;  // Rhythm -> bass resonator slot
            ResonatorProfile prof = morphResonator (morph, slot);

            // Scale formant frequencies relative to the played note.
            // Reference is A4 (440 Hz) — formant profiles are defined at
            // this reference and scaled proportionally for other pitches.
            float noteRatio = freq / 440.0f;

            for (int f = 0; f < 4; ++f)
            {
                ch.formantFreqs[f] = clamp (prof.formantFreqs[f] * noteRatio, 20.0f, 18000.0f);
                ch.formantGains[f] = prof.formantGains[f];
                ch.formantBandwidths[f] = prof.formantBandwidths[f];
                ch.formants[f].reset();
            }
            ch.updateFormants (srf, 0.0f);

            // Pre-fill memory buffer with current shore position so the
            // voice starts with a clean slate — no inherited travel history.
            for (int m = 0; m < kMemoryBufferSize; ++m)
                ch.memoryBuffer[m] = ch.shorePos;
            ch.memoryWritePos = 0;
        }
    }

    void noteOff (int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber && !voice.fadingOut)
                voice.ampEnv.noteOff();
        }
    }

    //==========================================================================
    // Member data
    //==========================================================================

    // --- Audio engine state ---
    double sr = 44100.0;               // Sample rate (double precision for accuracy)
    float srf = 44100.0f;              // Sample rate (float, for DSP calculations)
    float crossfadeRate = 0.01f;       // Voice-stealing crossfade: samples to silence in 5ms

    // --- Control rate ---
    int controlRateDiv = 22;           // Audio samples per control update (~2 kHz)
    float controlDt = 0.0005f;         // Control period in seconds

    // --- Voice pool ---
    std::array<OsteriaVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;         // Monotonic counter for LRU voice stealing
    int activeVoices = 0;              // Current active voice count (reported to UI)

    // D006: aftertouch handler — CS-80-style channel pressure → tavern mix depth
    PolyAftertouch aftertouch;

    // D006: mod wheel (CC#1) — deepens smoke haze, pulling the ensemble into
    // the woodfire warmth of the tavern. Full wheel drops smoke cutoff by up
    // to 4 kHz, thickening the air between the listener and the quartet.
    float modWheelAmount_ = 0.0f;
    float pitchBendNorm   = 0.0f;  // MIDI pitch wheel [-1, +1]; ±2 semitone range

    // --- Coupling accumulators ---
    // These accumulate coupling input between renderBlock calls and are
    // consumed (reset to 0) at the start of each block.
    float envelopeOutput = 0.0f;              // Peak envelope for outbound coupling
    float couplingExcitationMod = 0.0f;       // AudioToWavetable: shapes quartet excitation
    float couplingElasticMod = 0.0f;          // AmpToFilter: modulates elastic tightness
    float couplingRoomExcitation = 0.0f;      // AudioToFM: excites tavern room
    float couplingShoreDrift = 0.0f;          // EnvToMorph: drives shore position drift

    // --- Output cache (for coupling output) ---
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // --- Tavern environment ---
    TavernRoom tavernRoom;                    // FDN reverb modeling tavern acoustics
    MurmurGenerator murmur;                   // Crowd/conversation noise texture

    // --- Character stage filters ---
    CytomicSVF smokeFilter;                   // Smoke: HF haze lowpass
    CytomicSVF warmthFilter;                  // Warmth: low shelf proximity EQ

    // --- Session Delay ---
    // 22050 samples = 500ms at 44.1 kHz (enough headroom for the 150ms delay)
    static constexpr int kSessionDelayMax = 22050;
    float sessionDelayBuf[2][kSessionDelayMax] = {};
    int sessionDelayWritePos = 0;

    // --- Chorus ---
    // 2048 samples = ~46ms at 44.1 kHz (covers the 8ms +/- 3ms modulated range)
    static constexpr int kChorusBufSize = 2048;
    float chorusBuf[2][kChorusBufSize] = {};
    int chorusWritePos = 0;
    float chorusPhase = 0.0f;                 // 0.5 Hz chorus LFO phase

    // --- Hall (allpass diffusion delays) ---
    float hallBuf[4][kHallDelayMax] = {};
    int hallWritePos[4] = {};

    // --- Tape (one-pole lowpass state) ---
    float tapeState[2] = {};

    //==========================================================================
    // Cached APVTS parameter pointers (ParamSnapshot pattern)
    //
    // These raw atomic pointers are set once in attachParameters() and read
    // via loadParam() at the start of each renderBlock(). This avoids
    // per-sample atomic loads and string lookups.
    //==========================================================================

    // -- Quartet --
    std::atomic<float>* paramBassShore = nullptr;
    std::atomic<float>* paramHarmShore = nullptr;
    std::atomic<float>* paramMelShore = nullptr;
    std::atomic<float>* paramRhythmShore = nullptr;
    std::atomic<float>* paramElastic = nullptr;
    std::atomic<float>* paramStretch = nullptr;
    std::atomic<float>* paramMemory = nullptr;
    std::atomic<float>* paramSympathy = nullptr;

    // -- Voice Balance --
    std::atomic<float>* paramBassLevel = nullptr;
    std::atomic<float>* paramHarmLevel = nullptr;
    std::atomic<float>* paramMelLevel = nullptr;
    std::atomic<float>* paramRhythmLevel = nullptr;
    std::atomic<float>* paramEnsWidth = nullptr;
    std::atomic<float>* paramBlendMode = nullptr;

    // -- Tavern --
    std::atomic<float>* paramTavernMix = nullptr;
    std::atomic<float>* paramTavernShore = nullptr;
    std::atomic<float>* paramMurmur = nullptr;
    std::atomic<float>* paramWarmth = nullptr;
    std::atomic<float>* paramOceanBleed = nullptr;

    // -- Character --
    std::atomic<float>* paramPatina = nullptr;
    std::atomic<float>* paramPorto = nullptr;
    std::atomic<float>* paramSmoke = nullptr;

    // -- Envelope --
    std::atomic<float>* paramAttack = nullptr;
    std::atomic<float>* paramDecay = nullptr;
    std::atomic<float>* paramSustain = nullptr;
    std::atomic<float>* paramRelease = nullptr;

    // -- FX --
    std::atomic<float>* paramSessionDelay = nullptr;
    std::atomic<float>* paramHall = nullptr;
    std::atomic<float>* paramChorus = nullptr;
    std::atomic<float>* paramTape = nullptr;

    // -- Macros (M1-M4) --
    std::atomic<float>* paramMacroCharacter = nullptr;
    std::atomic<float>* paramMacroMovement = nullptr;
    std::atomic<float>* paramMacroCoupling = nullptr;
    std::atomic<float>* paramMacroSpace = nullptr;

    // D001: filter envelope depth + per-block boost (Hz, applied to smoke filter)
    std::atomic<float>* paramFilterEnvDepth = nullptr;
    float filterEnvBoost = 0.0f;

    // D005/D002: User-controllable LFO — modulates shore drift for evolving timbres.
    // Rate controlled by M2 MOVEMENT: 0.005 Hz at rest (breathing) to 2 Hz active.
    StandardLFO userLFO;
};

} // namespace xomnibus
