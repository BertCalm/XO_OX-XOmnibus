#pragma once
// XOverlap Voice — bell oscillator with ADSR + pulse envelope.
//
// Synthesis: sine fundamental + 3 inharmonic partials.
//   Partial ratios: 2.17, 3.71, 5.43 (inharmonic bell character)
//   Brightness = 0.3 + 0.7*velocity  (D001: velocity directly scales partials)
//
// Bell-pulse gate: cosine contraction at pulseRate Hz.
//   gate = 0.35 + 0.65 * cos(2π·pulsePhase)
// Kuramoto entrainment (Entrainment.h) nudges pulsePhase toward mutual lock.

#include <cmath>
#include <cstdint>

namespace xoverlap {

//==============================================================================
struct AmpEnvelope {
    float attack  = 0.05f;
    float decay   = 1.0f;
    float sustain = 0.7f;
    float release = 2.0f;

    void setParams(float a, float d, float s, float r) noexcept
    {
        attack = a; decay = d; sustain = s; release = r;
    }
};

//==============================================================================
struct Voice {
    // ── Public fields (read by Entrainment, adapter) ─────────────────────────
    float    pulsePhase   = 0.0f;   // bell-pulse phase [0, 1) — modified by Entrainment
    float    pulseRate    = 0.5f;   // Hz, set each process() call
    bool     held         = false;
    uint64_t noteOnOrder  = 0;
    uint64_t noteOffOrder = 0;
    int      midiNote     = 0;
    AmpEnvelope env;

    //==========================================================================
    void prepare(float sr) noexcept { sampleRate = sr; }

    void reset() noexcept
    {
        held = false;  noteOnOrder = 0;  noteOffOrder = 0;  midiNote = 0;
        envLevel = 0.0f;  envStage = 0;
        oscPhase = 0.0f;  pulsePhase = 0.0f;  pulseRate = 0.5f;
    }

    void setVoiceIndex(int i) noexcept { voiceIndex = i; }

    bool isActive() const noexcept
    {
        return envStage != 0 || envLevel > 0.001f;
    }

    //==========================================================================
    void noteOn(int note, float vel, uint64_t order) noexcept
    {
        midiNote = note;   velocity = vel;
        noteOnOrder = order;  noteOffOrder = 0;  held = true;
        freq     = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
        envLevel = 0.0f;  envStage = 1;  // start attack
        pulsePhase = 0.0f;               // clean articulation
    }

    void noteOff(uint64_t order) noexcept
    {
        held = false;  noteOffOrder = order;
        if (envStage != 0 && envStage != 4)
            envStage = 4;  // release
    }

    //==========================================================================
    // Returns one sample of bell oscillator output for the FDN input bus.
    float process(float pulseRateHz) noexcept
    {
        pulseRate = pulseRateHz;
        if (envStage == 0 && !isActive()) return 0.0f;

        float invSr = 1.0f / sampleRate;

        // — Amplitude ADSR ────────────────────────────────────────────────────
        switch (envStage)
        {
            case 1: // attack
            {
                float inc = invSr / (env.attack > 0.001f ? env.attack : 0.001f);
                envLevel += inc;
                if (envLevel >= 1.0f) { envLevel = 1.0f; envStage = 2; }
                break;
            }
            case 2: // decay → sustain
            {
                float span = (1.0f - env.sustain);
                float inc  = span * invSr / (env.decay > 0.001f ? env.decay : 0.001f);
                envLevel -= inc;
                if (envLevel <= env.sustain) { envLevel = env.sustain; envStage = 3; }
                break;
            }
            case 3: // sustain
                envLevel = env.sustain;
                break;
            case 4: // release
            {
                float inc = env.sustain * invSr / (env.release > 0.001f ? env.release : 0.001f);
                envLevel -= inc;
                if (envLevel <= 0.0f) { envLevel = 0.0f; envStage = 0; }
                break;
            }
            default:
                envLevel = 0.0f;
                break;
        }

        if (envStage == 0) return 0.0f;

        // — Bell oscillator (D001: brightness from velocity) ──────────────────
        constexpr float twoPi    = 6.28318530f;
        float           brightness = 0.3f + 0.7f * velocity;
        float           phRad      = oscPhase * twoPi;

        float osc = std::sin(phRad);
        osc      += std::sin(phRad * 2.17f) * 0.30f * brightness;
        osc      += std::sin(phRad * 3.71f) * 0.15f * (brightness * brightness);
        osc      += std::sin(phRad * 5.43f) * 0.08f * (brightness * brightness);

        oscPhase += freq * invSr;
        if (oscPhase >= 1.0f) oscPhase -= 1.0f;

        // — Bell-pulse gate (cosine contraction) ──────────────────────────────
        float gate = 0.35f + 0.65f * std::cos(pulsePhase * twoPi);
        pulsePhase += pulseRateHz * invSr;
        if (pulsePhase >= 1.0f) pulsePhase -= 1.0f;

        // Output: amp env × pulse gate × oscillator (scaled for 6-voice mix)
        return osc * envLevel * gate * 0.15f;
    }

private:
    float sampleRate = 44100.0f;
    float freq       = 440.0f;
    float velocity   = 1.0f;
    float envLevel   = 0.0f;
    int   envStage   = 0;     // 0=idle 1=attack 2=decay 3=sustain 4=release
    float oscPhase   = 0.0f;  // [0, 1)
    int   voiceIndex = 0;
};

} // namespace xoverlap
