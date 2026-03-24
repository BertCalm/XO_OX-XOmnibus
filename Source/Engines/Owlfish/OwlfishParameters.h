#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace xowlfish {
namespace ParamIDs {

// -- Solitary Genus (3) --
constexpr const char* portamento     = "owl_portamento";
constexpr const char* legatoMode     = "owl_legatoMode";
constexpr const char* morphGlide     = "owl_morphGlide";

// -- Abyss Habitat (14) --
constexpr const char* subMix         = "owl_subMix";
constexpr const char* subDiv1        = "owl_subDiv1";
constexpr const char* subDiv2        = "owl_subDiv2";
constexpr const char* subDiv3        = "owl_subDiv3";
constexpr const char* subDiv4        = "owl_subDiv4";
constexpr const char* subLevel1      = "owl_subLevel1";
constexpr const char* subLevel2      = "owl_subLevel2";
constexpr const char* subLevel3      = "owl_subLevel3";
constexpr const char* subLevel4      = "owl_subLevel4";
constexpr const char* mixtur         = "owl_mixtur";
constexpr const char* fundWave       = "owl_fundWave";
constexpr const char* subWave        = "owl_subWave";
constexpr const char* bodyFreq       = "owl_bodyFreq";
constexpr const char* bodyLevel      = "owl_bodyLevel";

// -- Owl Optics (7) --
constexpr const char* compRatio      = "owl_compRatio";
constexpr const char* compThreshold  = "owl_compThreshold";
constexpr const char* compAttack     = "owl_compAttack";
constexpr const char* compRelease    = "owl_compRelease";
constexpr const char* filterCutoff   = "owl_filterCutoff";
constexpr const char* filterReso     = "owl_filterReso";
constexpr const char* filterTrack    = "owl_filterTrack";
constexpr const char* filterEnvDepth = "owl_filterEnvDepth";

// -- Diet (5) --
constexpr const char* grainSize      = "owl_grainSize";
constexpr const char* grainDensity   = "owl_grainDensity";
constexpr const char* grainPitch     = "owl_grainPitch";
constexpr const char* grainMix       = "owl_grainMix";
constexpr const char* feedRate       = "owl_feedRate";

// -- Sacrificial Armor (5) --
constexpr const char* armorThreshold = "owl_armorThreshold";
constexpr const char* armorDecay     = "owl_armorDecay";
constexpr const char* armorScatter   = "owl_armorScatter";
constexpr const char* armorDuck      = "owl_armorDuck";
constexpr const char* armorDelay     = "owl_armorDelay";

// -- Abyss Reverb (4) --
constexpr const char* reverbSize     = "owl_reverbSize";
constexpr const char* reverbDamp     = "owl_reverbDamp";
constexpr const char* reverbPreDelay = "owl_reverbPreDelay";
constexpr const char* reverbMix      = "owl_reverbMix";

// -- Amp Envelope (4) --
constexpr const char* ampAttack      = "owl_ampAttack";
constexpr const char* ampDecay       = "owl_ampDecay";
constexpr const char* ampSustain     = "owl_ampSustain";
constexpr const char* ampRelease     = "owl_ampRelease";

// -- Macros (4) --
constexpr const char* macroDepth     = "owl_depth";
constexpr const char* macroFeeding   = "owl_feeding";
constexpr const char* macroDefense   = "owl_defense";
constexpr const char* macroPressure  = "owl_pressure";

// -- Output (2) --
constexpr const char* outputLevel    = "owl_outputLevel";
constexpr const char* outputPan      = "owl_outputPan";

// -- Coupling (2) --
constexpr const char* couplingLevel  = "owl_couplingLevel";
constexpr const char* couplingBus    = "owl_couplingBus";

} // namespace ParamIDs

// -- Enum string arrays --

namespace Enums {

inline const juce::StringArray subDivision {
    "Off", "1:2", "1:3", "1:4", "1:5", "1:6", "1:7", "1:8"
};

inline const juce::StringArray legatoMode {
    "Retrigger", "Legato"
};

inline const juce::StringArray couplingBus {
    "A", "B", "C", "D"
};

} // namespace Enums

// -- Parameter Registration (vector-based, for XOlokun shared APVTS) --

inline void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& p)
{
    auto F = [&](const char* id, const char* name, float lo, float hi, float def, float skew = 1.0f) {
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float>(lo, hi, 0.0f, skew), def));
    };
    auto C = [&](const char* id, const char* name, const juce::StringArray& ch, int def) {
        p.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { id, 1 }, name, ch, def));
    };

    using namespace ParamIDs;

    // -- Solitary Genus (3) --
    F(portamento,  "Portamento",   0.0f, 1.0f, 0.1f);
    C(legatoMode,  "Legato Mode",  Enums::legatoMode, 1);
    F(morphGlide,  "Morph Glide",  0.0f, 1.0f, 0.5f);

    // -- Abyss Habitat (14) --
    F(subMix,      "Sub Mix",      0.0f, 1.0f, 0.5f);
    C(subDiv1,     "Sub Div 1",    Enums::subDivision, 1);
    C(subDiv2,     "Sub Div 2",    Enums::subDivision, 2);
    C(subDiv3,     "Sub Div 3",    Enums::subDivision, 0);
    C(subDiv4,     "Sub Div 4",    Enums::subDivision, 0);
    F(subLevel1,   "Sub Level 1",  0.0f, 1.0f, 0.8f);
    F(subLevel2,   "Sub Level 2",  0.0f, 1.0f, 0.6f);
    F(subLevel3,   "Sub Level 3",  0.0f, 1.0f, 0.4f);
    F(subLevel4,   "Sub Level 4",  0.0f, 1.0f, 0.3f);
    F(mixtur,      "Mixtur",       0.0f, 1.0f, 0.5f);
    F(fundWave,    "Fund Wave",    0.0f, 1.0f, 0.0f);
    F(subWave,     "Sub Wave",     0.0f, 1.0f, 0.0f);
    F(bodyFreq,    "Body Freq",    20.0f, 80.0f, 40.0f, 0.5f);
    F(bodyLevel,   "Body Level",   0.0f, 1.0f, 0.3f);

    // -- Owl Optics (7) --
    F(compRatio,     "Comp Ratio",     0.0f, 1.0f, 0.4f);
    F(compThreshold, "Comp Threshold", 0.0f, 1.0f, 0.5f);
    F(compAttack,    "Comp Attack",    0.0f, 1.0f, 0.2f);
    F(compRelease,   "Comp Release",   0.0f, 1.0f, 0.4f);
    F(filterCutoff,   "Filter Cutoff",   0.0f, 1.0f, 0.6f);
    F(filterReso,     "Filter Reso",     0.0f, 1.0f, 0.3f);
    F(filterTrack,    "Filter Track",    0.0f, 1.0f, 0.5f);
    // D001: filter envelope depth — amp envelope level × velocity boosts LP cutoff.
    // Default 0.25: at full velocity and attack peak, adds +1375 Hz via the amp
    // envelope signal, making harder hits brighter through the organism's optics.
    F(filterEnvDepth, "Filter Env Depth", 0.0f, 1.0f, 0.25f);

    // -- Diet (5) --
    F(grainSize,    "Grain Size",    0.0f, 1.0f, 0.3f);
    F(grainDensity, "Grain Density", 0.0f, 1.0f, 0.5f);
    F(grainPitch,   "Grain Pitch",   0.0f, 1.0f, 0.5f);
    F(grainMix,     "Grain Mix",     0.0f, 1.0f, 0.0f);
    F(feedRate,     "Feed Rate",     0.0f, 1.0f, 0.3f);

    // -- Sacrificial Armor (5) --
    F(armorThreshold, "Armor Threshold", 0.0f, 1.0f, 0.5f);
    F(armorDecay,     "Armor Decay",     0.0f, 1.0f, 0.4f);
    F(armorScatter,   "Armor Scatter",   0.0f, 1.0f, 0.2f);
    F(armorDuck,      "Armor Duck",      0.0f, 1.0f, 0.3f);
    F(armorDelay,     "Armor Delay",     0.0f, 1.0f, 0.2f);

    // -- Abyss Reverb (4) --
    F(reverbSize,     "Reverb Size",      0.0f, 1.0f, 0.6f);
    F(reverbDamp,     "Reverb Damp",      0.0f, 1.0f, 0.4f);
    F(reverbPreDelay, "Reverb Pre-Delay", 0.0f, 1.0f, 0.1f);
    F(reverbMix,      "Reverb Mix",       0.0f, 1.0f, 0.2f);

    // -- Amp Envelope (4) --
    F(ampAttack,  "Amp Attack",  0.001f, 8000.0f,  10.0f,  0.3f);
    F(ampDecay,   "Amp Decay",   50.0f,  4000.0f, 300.0f,  0.3f);
    F(ampSustain, "Amp Sustain", 0.0f,   1.0f,     0.8f);
    F(ampRelease, "Amp Release", 50.0f,  8000.0f, 600.0f,  0.3f);

    // -- Macros (4) --
    F(macroDepth,    "DEPTH",    0.0f, 1.0f, 0.0f);
    F(macroFeeding,  "FEEDING",  0.0f, 1.0f, 0.0f);
    F(macroDefense,  "DEFENSE",  0.0f, 1.0f, 0.0f);
    F(macroPressure, "PRESSURE", 0.0f, 1.0f, 0.0f);

    // -- Output (2) --
    F(outputLevel, "Output Level", 0.0f, 1.0f,  0.8f);
    F(outputPan,   "Output Pan",  -1.0f, 1.0f,  0.0f);

    // -- Coupling (2) --
    F(couplingLevel, "Coupling Level", 0.0f, 1.0f, 0.0f);
    C(couplingBus,   "Coupling Bus",   Enums::couplingBus, 0);
}

// -- Standalone Parameter Layout (wraps addParameters) --

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    addParameters(params);
    return { params.begin(), params.end() };
}

} // namespace xowlfish
