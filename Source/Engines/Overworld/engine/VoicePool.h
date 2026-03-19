#pragma once
#include "../Parameters.h"
#include <cmath>
#include <algorithm>
#include <array>

//==============================================================================
// VoicePool — 8-voice polyphonic chip synthesis engine for XOverworld.
//
// Each voice renders a blend of 3 chip engines selected by the ERA triangle:
//   Vertex A, B, C select from: NES, FM (YM2612), SNES, Game Boy, PC Engine, Neo Geo
//
// ERA triangle: barycentric blend of the 3 vertex engine outputs.
//   eraX in [0,1] blends A→B weight; eraY in [0,1] blends C weight.
//
// ERA Memory: a ghost layer at a delayed ERA position (eraMemMix controls blend).
//
// Signal chain per voice:
//   [Chip A × wA + Chip B × wB + Chip C × wC] × ampEnv → sum to mono out
//==============================================================================

namespace xoverworld {

//------------------------------------------------------------------------------
// ChipOscillator — simple chip-engine voice oscillator.
// Supports 6 types corresponding to the 6 chip engines.
//------------------------------------------------------------------------------
struct ChipOscillator
{
    float phase   = 0.0f;
    float phase2  = 0.0f; // FM op2
    float sr      = 44100.0f;

    void prepare(float sampleRate) { sr = sampleRate; }

    // type: 0=NES Pulse, 1=FM(YM2612), 2=SNES BRR, 3=Game Boy, 4=PC Engine, 5=Neo Geo
    // Returns one sample in [-1, +1]
    float process(int type, float freq, const ParamSnapshot& p)
    {
        const float phaseInc = freq / sr;

        switch (type)
        {
            //-- NES: pulse + triangle mix ----------------------------------------
            case 0:
            {
                // Pulse wave with duty cycle
                static constexpr float dutyCycles[] = { 0.125f, 0.25f, 0.5f, 0.75f };
                const int dutyIdx = std::clamp(p.pulseDuty, 0, 3);
                const float duty  = dutyCycles[dutyIdx];

                phase = std::fmod(phase + phaseInc, 1.0f);
                float pulse = (phase < duty) ? 1.0f : -1.0f;

                // Triangle
                float tri = 0.0f;
                if (p.triEnable)
                    tri = (phase < 0.5f) ? (phase * 4.0f - 1.0f) : (3.0f - phase * 4.0f);

                return (pulse * p.nesMix + tri * (1.0f - p.nesMix)) * 0.5f;
            }

            //-- FM (YM2612): 2-operator approximation ----------------------------
            case 1:
            {
                // Modulator
                phase2 = std::fmod(phase2 + phaseInc * static_cast<float>(std::max(1, p.fmOpMult[1])), 1.0f);
                const float fbAmt   = static_cast<float>(p.fmFeedback) / 7.0f * 0.3f;
                const float modOut  = std::sin(phase2 * 6.283185f) * fbAmt * static_cast<float>(p.fmOpLevel[1]) / 31.0f;

                // Carrier
                phase = std::fmod(phase + phaseInc * static_cast<float>(std::max(1, p.fmOpMult[0])), 1.0f);
                return std::sin((phase + modOut) * 6.283185f) * static_cast<float>(p.fmOpLevel[0]) / 31.0f * 0.7f;
            }

            //-- SNES SPC700: sampled wavetable approximation ---------------------
            case 2:
            {
                phase = std::fmod(phase + phaseInc, 1.0f);
                // Approximation: bandlimited saw with Gaussian softening
                float saw = phase * 2.0f - 1.0f;
                // Soft clip to mimic BRR 16-bit compression artefacts
                return std::tanh(saw * 1.5f) * 0.6f;
            }

            //-- Game Boy: wave channel (4-bit wavetable) -------------------------
            case 3:
            {
                phase = std::fmod(phase + phaseInc, 1.0f);
                // 4-bit wave — approx with a triangle + 4-bit quantise
                float wave = (phase < 0.5f) ? (phase * 4.0f - 1.0f) : (3.0f - phase * 4.0f);
                float levels = 15.0f; // 4-bit = 16 levels
                wave = std::round(wave * levels) / levels;
                return wave * 0.5f;
            }

            //-- PC Engine: 5-bit wavetable ---------------------------------------
            case 4:
            {
                phase = std::fmod(phase + phaseInc, 1.0f);
                float wave = (phase < 0.5f) ? (phase * 4.0f - 1.0f) : (3.0f - phase * 4.0f);
                float levels = 31.0f; // 5-bit = 32 levels
                wave = std::round(wave * levels) / levels;
                return wave * 0.55f;
            }

            //-- Neo Geo: ADPCM-A approximation (square + saw blend) --------------
            case 5:
            default:
            {
                phase = std::fmod(phase + phaseInc, 1.0f);
                float sq  = (phase < 0.5f) ? 1.0f : -1.0f;
                float saw = phase * 2.0f - 1.0f;
                return (sq * 0.6f + saw * 0.4f) * 0.5f;
            }
        }
    }

    void reset() { phase = 0.0f; phase2 = 0.0f; }
};

//------------------------------------------------------------------------------
// Voice — a single polyphonic voice with ADSR amp envelope and 3 chip oscs.
//------------------------------------------------------------------------------
struct Voice
{
    static constexpr float kOff = -1.0f;

    float sr          = 44100.0f;
    float freq        = 440.0f;
    float velocity    = 1.0f;

    // ADSR state
    enum class State { Off, Attack, Decay, Sustain, Release } envState = State::Off;
    float envLevel    = 0.0f;
    float releaseFrom = 0.0f;

    // Three chip oscillators (one per ERA vertex)
    ChipOscillator oscA, oscB, oscC;

    void prepare(float sampleRate)
    {
        sr = sampleRate;
        oscA.prepare(sr);
        oscB.prepare(sr);
        oscC.prepare(sr);
    }

    bool isActive() const { return envState != State::Off; }

    void noteOn(int noteNumber, float vel, float sampleRate)
    {
        sr = sampleRate;
        // MIDI note → freq
        freq = 440.0f * std::pow(2.0f, (static_cast<float>(noteNumber) - 69.0f) / 12.0f);
        velocity = vel;
        envState = State::Attack;
        envLevel = 0.0f;
        oscA.reset(); oscB.reset(); oscC.reset();
    }

    void noteOff()
    {
        if (envState != State::Off)
        {
            releaseFrom = envLevel;
            envState = State::Release;
        }
    }

    void kill()
    {
        envState = State::Off;
        envLevel = 0.0f;
    }

    // Returns one sample. eraWA/WB/WC are barycentric weights summing to ~1.
    float process(float eraWA, float eraWB, float eraWC,
                  float ghostWA, float ghostWB, float ghostWC, float ghostMix,
                  int vtxA, int vtxB, int vtxC,
                  const ParamSnapshot& p)
    {
        if (envState == State::Off) return 0.0f;

        // Advance amp envelope
        advanceEnv(p);

        const float envOut = envLevel * velocity;
        if (envOut < 1e-6f && envState == State::Release)
        {
            envState = State::Off;
            return 0.0f;
        }

        // Live ERA layer
        float chipA = oscA.process(vtxA, freq, p);
        float chipB = oscB.process(vtxB, freq, p);
        float chipC = oscC.process(vtxC, freq, p);
        float live  = chipA * eraWA + chipB * eraWB + chipC * eraWC;

        // Ghost layer (same oscs re-used at different weights — approximation)
        float ghost = chipA * ghostWA + chipB * ghostWB + chipC * ghostWC;

        float out = live + (ghost - live) * ghostMix;
        return out * envOut;
    }

private:
    void advanceEnv(const ParamSnapshot& p)
    {
        const float atk = std::max(0.001f, p.ampAttack);
        const float dec = std::max(0.001f, p.ampDecay);
        const float sus = p.ampSustain;
        const float rel = std::max(0.001f, p.ampRelease);

        switch (envState)
        {
            case State::Attack:
            {
                float rate = 1.0f / (atk * sr);
                envLevel += rate;
                if (envLevel >= 1.0f) { envLevel = 1.0f; envState = State::Decay; }
                break;
            }
            case State::Decay:
            {
                float rate = (1.0f - sus) / (dec * sr);
                envLevel -= rate;
                if (envLevel <= sus) { envLevel = sus; envState = State::Sustain; }
                break;
            }
            case State::Sustain:
                envLevel = sus;
                break;
            case State::Release:
            {
                float rate = releaseFrom / (rel * sr);
                envLevel -= rate;
                if (envLevel <= 0.0f) { envLevel = 0.0f; envState = State::Off; }
                break;
            }
            default:
                break;
        }
    }
};

//------------------------------------------------------------------------------
// VoicePool — manages 8 voices and mixes them to a mono output sample.
//------------------------------------------------------------------------------
class VoicePool
{
public:
    static constexpr int kMaxVoices = 8;

    void prepare(float sampleRate)
    {
        sr = sampleRate;
        for (auto& v : voices) v.prepare(sampleRate);
    }

    void noteOn(int noteNumber, float velocity)
    {
        // Find free or steal oldest voice
        int slot = findFreeVoice(noteNumber);
        voices[slot].noteOn(noteNumber, velocity, sr);
        noteNumbers[slot] = noteNumber;
    }

    void noteOff(int noteNumber)
    {
        for (int i = 0; i < kMaxVoices; ++i)
            if (noteNumbers[i] == noteNumber && voices[i].isActive())
                voices[i].noteOff();
    }

    void allNotesOff()
    {
        for (int i = 0; i < kMaxVoices; ++i)
        {
            voices[i].noteOff();
            noteNumbers[i] = -1;
        }
    }

    void reset()
    {
        for (int i = 0; i < kMaxVoices; ++i)
        {
            voices[i].kill();
            noteNumbers[i] = -1;
        }
    }

    void applyParams(const ParamSnapshot& snap)
    {
        // Cache snapshot so voice oscillators can read chip parameters per-sample.
        currentSnap = snap;
    }

    // Returns one mixed mono sample.
    // eraX, eraY: live ERA position; ghostEraX, ghostEraY: ERA memory position.
    // eraMemMix: blend between live and ghost layers.
    float process(float eraX, float eraY,
                  float ghostEraX, float ghostEraY,
                  float eraMemMix)
    {
        // ERA barycentric weights:
        //   wC = eraY (C vertex on Y axis)
        //   wA + wB = 1 - wC; split by eraX
        //   wA = (1-eraX)*(1-eraY)  wB = eraX*(1-eraY)  wC = eraY
        const float wC  = std::clamp(eraY,  0.0f, 1.0f);
        const float wAB = 1.0f - wC;
        const float wA  = wAB * (1.0f - std::clamp(eraX, 0.0f, 1.0f));
        const float wB  = wAB * std::clamp(eraX, 0.0f, 1.0f);

        const float gwC  = std::clamp(ghostEraY, 0.0f, 1.0f);
        const float gwAB = 1.0f - gwC;
        const float gwA  = gwAB * (1.0f - std::clamp(ghostEraX, 0.0f, 1.0f));
        const float gwB  = gwAB * std::clamp(ghostEraX, 0.0f, 1.0f);

        float mixed = 0.0f;
        for (auto& v : voices)
        {
            if (!v.isActive()) continue;
            mixed += v.process(wA, wB, wC, gwA, gwB, gwC, eraMemMix,
                               currentSnap.vertexA, currentSnap.vertexB, currentSnap.vertexC,
                               currentSnap);
        }

        // Normalise by voice count to prevent clipping
        return mixed * (1.0f / static_cast<float>(kMaxVoices));
    }

    // Called before process() each block to give VoicePool access to params.
    // In XOmnibus OverworldEngine::renderBlock() this is called via applyParams().
    // However OverworldEngine also calls process() with era values directly.
    // We cache the snapshot so Voice::process() can access it.
    void setSnapshot(const ParamSnapshot& snap) { currentSnap = snap; }

private:
    int findFreeVoice(int noteNumber)
    {
        // 1. Prefer an already-silent slot
        for (int i = 0; i < kMaxVoices; ++i)
            if (!voices[i].isActive()) return i;

        // 2. Steal: retrigger same note if playing
        for (int i = 0; i < kMaxVoices; ++i)
            if (noteNumbers[i] == noteNumber) return i;

        // 3. Round-robin steal
        static int stealIdx = 0;
        stealIdx = (stealIdx + 1) % kMaxVoices;
        return stealIdx;
    }

    float sr = 44100.0f;
    std::array<Voice, kMaxVoices> voices;
    std::array<int,   kMaxVoices> noteNumbers { -1,-1,-1,-1,-1,-1,-1,-1 };
    ParamSnapshot currentSnap;
};

} // namespace xoverworld
