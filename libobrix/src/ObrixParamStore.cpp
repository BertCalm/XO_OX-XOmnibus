// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#include "obrix/ObrixParamStore.h"

namespace obrix
{

// Static parameter table extracted from ObrixEngine.h lines 1573-1801.
// Order matches the addParametersImpl() registration order exactly.
// Format: { "parameter_id", default_value }
static const ParamDef kObrixParams[] = {
    // Sources (Wave 1)
    {"obrix_src1Type",          1.0f},   // Choice: Off(0), Sine(1), Saw(2), Square(3), Triangle(4), Noise(5), Wavetable(6), Pulse(7), Driftwood(8)
    {"obrix_src2Type",          0.0f},
    {"obrix_src1Tune",          0.0f},   // -24..24 semitones
    {"obrix_src2Tune",          0.0f},
    {"obrix_src1PW",            0.5f},   // 0.05..0.95
    {"obrix_src2PW",            0.5f},
    {"obrix_srcMix",            0.5f},   // 0..1

    // Processors (Wave 1)
    {"obrix_proc1Type",         1.0f},   // Choice: Off(0), LP(1), HP(2), BP(3), Wavefolder(4), RingMod(5)
    {"obrix_proc1Cutoff",    8000.0f},   // 20..20000 Hz, skew 0.3
    {"obrix_proc1Reso",         0.0f},   // 0..1
    {"obrix_proc2Type",         0.0f},
    {"obrix_proc2Cutoff",    4000.0f},
    {"obrix_proc2Reso",         0.0f},
    {"obrix_proc3Type",         0.0f},
    {"obrix_proc3Cutoff",    4000.0f},
    {"obrix_proc3Reso",         0.0f},

    // Amplitude Envelope (Wave 1)
    {"obrix_ampAttack",        0.01f},   // 0..10s, skew 0.3
    {"obrix_ampDecay",          0.3f},
    {"obrix_ampSustain",        0.7f},   // 0..1
    {"obrix_ampRelease",        0.5f},   // 0..20s, skew 0.3

    // Modulators (Wave 1) — 4 slots x 4 params
    {"obrix_mod1Type",          1.0f},   // Choice: Off(0), Envelope(1), LFO(2), Velocity(3), Aftertouch(4)
    {"obrix_mod1Target",        2.0f},   // Choice: None(0), Pitch(1), FilterCutoff(2), FilterReso(3), Volume(4), ...
    {"obrix_mod1Depth",         0.5f},   // -1..1
    {"obrix_mod1Rate",          1.0f},   // 0.01..30 Hz, skew 0.3
    {"obrix_mod2Type",          2.0f},
    {"obrix_mod2Target",        2.0f},
    {"obrix_mod2Depth",         0.0f},
    {"obrix_mod2Rate",          1.0f},
    {"obrix_mod3Type",          3.0f},
    {"obrix_mod3Target",        4.0f},
    {"obrix_mod3Depth",         0.5f},
    {"obrix_mod3Rate",          1.0f},
    {"obrix_mod4Type",          4.0f},
    {"obrix_mod4Target",        2.0f},
    {"obrix_mod4Depth",         0.0f},
    {"obrix_mod4Rate",          1.0f},

    // Effects (Wave 1) — 3 slots x 3 params
    {"obrix_fx1Type",           0.0f},   // Choice: Off(0), Delay(1), Chorus(2), Reverb(3)
    {"obrix_fx1Mix",            0.0f},   // 0..1
    {"obrix_fx1Param",          0.3f},
    {"obrix_fx2Type",           0.0f},
    {"obrix_fx2Mix",            0.0f},
    {"obrix_fx2Param",          0.3f},
    {"obrix_fx3Type",           0.0f},
    {"obrix_fx3Mix",            0.0f},
    {"obrix_fx3Param",          0.3f},

    // Global (Wave 1)
    {"obrix_level",             0.8f},   // 0..1
    {"obrix_macroCharacter",    0.0f},   // 0..1
    {"obrix_macroMovement",     0.0f},
    {"obrix_macroCoupling",     0.0f},
    {"obrix_macroSpace",        0.0f},
    {"obrix_polyphony",         3.0f},   // Choice: Mono(0), Legato(1), Poly4(2), Poly8(3)
    {"obrix_pitchBendRange",    2.0f},   // 1..24 semitones
    {"obrix_glideTime",         0.0f},   // 0..1, skew 0.3
    {"obrix_gestureType",       0.0f},   // Choice: Ripple(0), Bioluminescent(1), Undertow(2), Surge(3)
    {"obrix_flashTrigger",      0.0f},   // 0 or 1 (trigger)

    // Wave 2 — FM, Feedback, Wavetable, Unison
    {"obrix_fmDepth",           0.0f},   // -1..1
    {"obrix_proc1Feedback",     0.0f},   // 0..1
    {"obrix_proc2Feedback",     0.0f},
    {"obrix_proc3Feedback",     0.0f},
    {"obrix_wtBank",            0.0f},   // Choice: Analog(0), Vocal(1), Metallic(2), Organic(3)
    {"obrix_unisonDetune",      0.0f},   // 0..50 cents

    // Wave 3 — Drift Bus, Journey, Spatial
    {"obrix_driftRate",       0.005f},   // 0.001..0.05, skew 0.3
    {"obrix_driftDepth",        0.0f},   // 0..1
    {"obrix_journeyMode",      0.0f},   // 0 or 1
    {"obrix_distance",          0.0f},   // 0..1
    {"obrix_air",               0.0f},   // 0..1

    // Wave 4 — Biophonic Synthesis
    {"obrix_fieldStrength",     0.0f},   // 0..1
    {"obrix_fieldPolarity",     0.0f},   // Bool: false(0), true(1)
    {"obrix_fieldRate",        0.01f},   // 0.001..0.1, skew 0.3
    {"obrix_fieldPrimeLimit",   0.0f},   // Choice: 3-Limit(0), 5-Limit(1), 7-Limit(2)
    {"obrix_envTemp",           0.0f},   // 0..1
    {"obrix_envPressure",       0.5f},   // 0..1 (0.5 = neutral)
    {"obrix_envCurrent",        0.0f},   // -1..1
    {"obrix_envTurbidity",      0.0f},   // 0..1
    {"obrix_competitionStrength", 0.0f},
    {"obrix_symbiosisStrength",   0.0f},
    {"obrix_stressDecay",       0.0f},   // 0..1
    {"obrix_bleachRate",        0.0f},   // 0..1
    {"obrix_stateReset",        0.0f},   // 0 or 1 (trigger)
    {"obrix_fxMode",            0.0f},   // Choice: Serial(0), Parallel(1)

    // Wave 5 — Reef Residency
    {"obrix_reefResident",      0.0f},   // Choice: Off(0), Competitor(1), Symbiote(2), Parasite(3)
    {"obrix_residentStrength",  0.3f},   // 0..1 (Guru Bin: 0.3 sweet spot)
};

static constexpr int kNumParams = sizeof(kObrixParams) / sizeof(kObrixParams[0]);

ObrixParamStore::ObrixParamStore()
    : values_(kNumParams)
{
    paramDefs_.reserve(kNumParams);
    for (int i = 0; i < kNumParams; ++i)
    {
        paramDefs_.push_back(kObrixParams[i]);
        values_[i].store(kObrixParams[i].defaultValue, std::memory_order_relaxed);
        idToIndex_[kObrixParams[i].id] = i;
    }
}

void ObrixParamStore::populateAPVTS(juce::AudioProcessorValueTreeState& apvts)
{
    for (int i = 0; i < kNumParams; ++i)
        apvts.registerParam(paramDefs_[i].id, &values_[i]);
}

bool ObrixParamStore::setParameter(const std::string& id, float value)
{
    auto it = idToIndex_.find(id);
    if (it == idToIndex_.end()) return false;
    values_[it->second].store(value, std::memory_order_relaxed);
    return true;
}

float ObrixParamStore::getParameter(const std::string& id) const
{
    auto it = idToIndex_.find(id);
    if (it == idToIndex_.end()) return 0.0f;
    return values_[it->second].load(std::memory_order_relaxed);
}

} // namespace obrix
