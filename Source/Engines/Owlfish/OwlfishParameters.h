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

// -- Parameter Layout Factory --

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto floatParam = [](const juce::String& id, const juce::String& name,
                         float min, float max, float def, float skew = 1.0f) {
        return std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float>(min, max, 0.0f, skew), def);
    };

    auto choiceParam = [](const juce::String& id, const juce::String& name,
                          const juce::StringArray& choices, int defIndex) {
        return std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { id, 1 }, name, choices, defIndex);
    };

    using namespace ParamIDs;

    // -- Solitary Genus (3) --
    layout.add(floatParam (portamento,  "Portamento",   0.0f, 1.0f, 0.1f));
    layout.add(choiceParam(legatoMode,  "Legato Mode",  Enums::legatoMode, 1));  // Legato default
    layout.add(floatParam (morphGlide,  "Morph Glide",  0.0f, 1.0f, 0.5f));

    // -- Abyss Habitat (14) --
    layout.add(floatParam (subMix,      "Sub Mix",      0.0f, 1.0f, 0.5f));
    layout.add(choiceParam(subDiv1,     "Sub Div 1",    Enums::subDivision, 1));  // 1:2
    layout.add(choiceParam(subDiv2,     "Sub Div 2",    Enums::subDivision, 2));  // 1:3
    layout.add(choiceParam(subDiv3,     "Sub Div 3",    Enums::subDivision, 0));  // Off
    layout.add(choiceParam(subDiv4,     "Sub Div 4",    Enums::subDivision, 0));  // Off
    layout.add(floatParam (subLevel1,   "Sub Level 1",  0.0f, 1.0f, 0.8f));
    layout.add(floatParam (subLevel2,   "Sub Level 2",  0.0f, 1.0f, 0.6f));
    layout.add(floatParam (subLevel3,   "Sub Level 3",  0.0f, 1.0f, 0.4f));
    layout.add(floatParam (subLevel4,   "Sub Level 4",  0.0f, 1.0f, 0.3f));
    layout.add(floatParam (mixtur,      "Mixtur",       0.0f, 1.0f, 0.5f));
    layout.add(floatParam (fundWave,    "Fund Wave",    0.0f, 1.0f, 0.0f));
    layout.add(floatParam (subWave,     "Sub Wave",     0.0f, 1.0f, 0.0f));
    layout.add(floatParam (bodyFreq,    "Body Freq",    20.0f, 80.0f, 40.0f, 0.5f));
    layout.add(floatParam (bodyLevel,   "Body Level",   0.0f, 1.0f, 0.3f));

    // -- Owl Optics (7) --
    layout.add(floatParam (compRatio,     "Comp Ratio",     0.0f, 1.0f, 0.4f));
    layout.add(floatParam (compThreshold, "Comp Threshold", 0.0f, 1.0f, 0.5f));
    layout.add(floatParam (compAttack,    "Comp Attack",    0.0f, 1.0f, 0.2f));
    layout.add(floatParam (compRelease,   "Comp Release",   0.0f, 1.0f, 0.4f));
    layout.add(floatParam (filterCutoff,  "Filter Cutoff",  0.0f, 1.0f, 0.6f));
    layout.add(floatParam (filterReso,    "Filter Reso",    0.0f, 1.0f, 0.3f));
    layout.add(floatParam (filterTrack,   "Filter Track",   0.0f, 1.0f, 0.5f));

    // -- Diet (5) --
    layout.add(floatParam (grainSize,    "Grain Size",    0.0f, 1.0f, 0.3f));
    layout.add(floatParam (grainDensity, "Grain Density", 0.0f, 1.0f, 0.5f));
    layout.add(floatParam (grainPitch,   "Grain Pitch",   0.0f, 1.0f, 0.5f));
    layout.add(floatParam (grainMix,     "Grain Mix",     0.0f, 1.0f, 0.0f));
    layout.add(floatParam (feedRate,     "Feed Rate",     0.0f, 1.0f, 0.3f));

    // -- Sacrificial Armor (5) --
    layout.add(floatParam (armorThreshold, "Armor Threshold", 0.0f, 1.0f, 0.5f));
    layout.add(floatParam (armorDecay,     "Armor Decay",     0.0f, 1.0f, 0.4f));
    layout.add(floatParam (armorScatter,   "Armor Scatter",   0.0f, 1.0f, 0.2f));
    layout.add(floatParam (armorDuck,      "Armor Duck",      0.0f, 1.0f, 0.3f));
    layout.add(floatParam (armorDelay,     "Armor Delay",     0.0f, 1.0f, 0.2f));

    // -- Abyss Reverb (4) --
    layout.add(floatParam (reverbSize,     "Reverb Size",      0.0f, 1.0f, 0.6f));
    layout.add(floatParam (reverbDamp,     "Reverb Damp",      0.0f, 1.0f, 0.4f));
    layout.add(floatParam (reverbPreDelay, "Reverb Pre-Delay", 0.0f, 1.0f, 0.1f));
    layout.add(floatParam (reverbMix,      "Reverb Mix",       0.0f, 1.0f, 0.2f));

    // -- Amp Envelope (4) --
    layout.add(floatParam (ampAttack,  "Amp Attack",  0.001f, 8000.0f,  10.0f,  0.3f));
    layout.add(floatParam (ampDecay,   "Amp Decay",   50.0f,  4000.0f, 300.0f,  0.3f));
    layout.add(floatParam (ampSustain, "Amp Sustain", 0.0f,   1.0f,     0.8f));
    layout.add(floatParam (ampRelease, "Amp Release", 50.0f,  8000.0f, 600.0f,  0.3f));

    // -- Macros (4) --
    layout.add(floatParam (macroDepth,    "DEPTH",    0.0f, 1.0f, 0.0f));
    layout.add(floatParam (macroFeeding,  "FEEDING",  0.0f, 1.0f, 0.0f));
    layout.add(floatParam (macroDefense,  "DEFENSE",  0.0f, 1.0f, 0.0f));
    layout.add(floatParam (macroPressure, "PRESSURE", 0.0f, 1.0f, 0.0f));

    // -- Output (2) --
    layout.add(floatParam (outputLevel, "Output Level", 0.0f, 1.0f,  0.8f));
    layout.add(floatParam (outputPan,   "Output Pan",  -1.0f, 1.0f,  0.0f));

    // -- Coupling (2) --
    layout.add(floatParam (couplingLevel, "Coupling Level", 0.0f, 1.0f, 0.0f));
    layout.add(choiceParam(couplingBus,   "Coupling Bus",   Enums::couplingBus, 0));

    return layout;
}

} // namespace xowlfish
