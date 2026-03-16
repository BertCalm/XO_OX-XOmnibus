#pragma once

#include <cmath>
#include <algorithm>
#include "../../DSP/FastMath.h"

namespace xocelot {

// Standard ADSR amplitude envelope.
// Ported from XOdyssey — namespace adapted to xocelot.
class AmpEnvelope
{
public:
    enum class State { Idle, Attack, Decay, Sustain, Release };

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        reset();
    }

    void reset()
    {
        state = State::Idle;
        level = 0.0f;
    }

    void setParameters(float attackMs, float decayMs, float sustain, float releaseMs)
    {
        attackTime  = std::max(0.001f, attackMs);
        decayTime   = std::max(0.001f, decayMs);
        sustainLvl  = std::clamp(sustain, 0.0f, 1.0f);
        releaseTime = std::max(0.001f, releaseMs);
    }

    void gate(bool on)
    {
        if (on)
            state = State::Attack;
        else if (state != State::Idle)
            state = State::Release;
    }

    float process()
    {
        switch (state)
        {
            case State::Idle:
                return 0.0f;

            case State::Attack:
            {
                float inc = 1.0f / (attackTime * 0.001f * static_cast<float>(sr));
                level += inc;
                if (level >= 1.0f) { level = 1.0f; state = State::Decay; }
                break;
            }
            case State::Decay:
            {
                float coeff = calcCoeff(decayTime);
                level = level * coeff + sustainLvl * (1.0f - coeff);
                if (std::abs(level - sustainLvl) < 0.0001f)
                    { level = sustainLvl; state = State::Sustain; }
                break;
            }
            case State::Sustain:
                level = sustainLvl;
                break;

            case State::Release:
            {
                float coeff = calcCoeff(releaseTime);
                level *= coeff;
                if (level < 0.0001f) { level = 0.0f; state = State::Idle; }
                break;
            }
        }
        return level;
    }

    bool isActive() const   { return state != State::Idle; }
    float getLevel() const  { return level; }

private:
    double sr          = 44100.0;
    State  state       = State::Idle;
    float  level       = 0.0f;
    float  attackTime  = 10.0f;
    float  decayTime   = 300.0f;
    float  sustainLvl  = 0.8f;
    float  releaseTime = 600.0f;

    float calcCoeff(float timeMs) const
    {
        return xomnibus::fastExp(-1.0f / (timeMs * 0.001f * static_cast<float>(sr)));
    }
};

} // namespace xocelot
