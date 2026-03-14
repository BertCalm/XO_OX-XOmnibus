#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace xocelot {
namespace ParamIDs {

// ── Global ────────────────────────────────────────
constexpr const char* biome            = "ocelot_biome";
constexpr const char* strataBalance    = "ocelot_strataBalance";
constexpr const char* ecosystemDepth   = "ocelot_ecosystemDepth";
constexpr const char* humidity         = "ocelot_humidity";
constexpr const char* swing            = "ocelot_swing";
constexpr const char* density          = "ocelot_density";

// ── Cross-Feed Matrix (12 routes) ─────────────────
// Naming: xf_{from}{To}
// Continuous routes: linear bipolar (-1 to +1)
// Threshold routes:  sigmoid, negative = inverse trigger
// Rhythmic routes:   stepped, negative = rhythmic opposition
constexpr const char* xfFloorUnder    = "ocelot_xf_floorUnder";   // Floor → Understory chop rate  [Rhythmic]
constexpr const char* xfFloorCanopy   = "ocelot_xf_floorCanopy";  // Floor → Canopy filter         [Continuous]
constexpr const char* xfFloorEmerg    = "ocelot_xf_floorEmerg";   // Floor → Emergent trigger      [Threshold]
constexpr const char* xfUnderFloor    = "ocelot_xf_underFloor";   // Understory → Floor swing      [Continuous]
constexpr const char* xfUnderCanopy   = "ocelot_xf_underCanopy";  // Understory → Canopy morph     [Continuous]
constexpr const char* xfUnderEmerg    = "ocelot_xf_underEmerg";   // Understory → Emergent pitch   [Continuous]
constexpr const char* xfCanopyFloor   = "ocelot_xf_canopyFloor";  // Canopy → Floor damp           [Continuous]
constexpr const char* xfCanopyUnder   = "ocelot_xf_canopyUnder";  // Canopy → Understory grain pos [Continuous]
constexpr const char* xfCanopyEmerg   = "ocelot_xf_canopyEmerg";  // Canopy → Emergent formant     [Continuous]
constexpr const char* xfEmergFloor    = "ocelot_xf_emergFloor";   // Emergent → Floor accent       [Threshold]
constexpr const char* xfEmergUnder    = "ocelot_xf_emergUnder";   // Emergent → Understory scatter [Rhythmic]
constexpr const char* xfEmergCanopy   = "ocelot_xf_emergCanopy";  // Emergent → Canopy shimmer     [Continuous]

// ── Floor (physical percussion) ───────────────────
constexpr const char* floorModel      = "ocelot_floorModel";
constexpr const char* floorTension    = "ocelot_floorTension";
constexpr const char* floorStrike     = "ocelot_floorStrike";
constexpr const char* floorDamping    = "ocelot_floorDamping";
constexpr const char* floorPattern    = "ocelot_floorPattern";
constexpr const char* floorLevel      = "ocelot_floorLevel";
constexpr const char* floorPitch      = "ocelot_floorPitch";
constexpr const char* floorVelocity   = "ocelot_floorVelocity";

// ── Understory (sample mangler) ───────────────────
constexpr const char* chopRate        = "ocelot_chopRate";
constexpr const char* chopSwing       = "ocelot_chopSwing";
constexpr const char* bitDepth        = "ocelot_bitDepth";
constexpr const char* sampleRateRed   = "ocelot_sampleRate";
constexpr const char* tapeWobble      = "ocelot_tapeWobble";
constexpr const char* tapeAge         = "ocelot_tapeAge";
constexpr const char* dustLevel       = "ocelot_dustLevel";
constexpr const char* understoryLevel = "ocelot_understoryLevel";
constexpr const char* understorySrc   = "ocelot_understorySrc";

// ── Canopy (spectral pad) ─────────────────────────
constexpr const char* canopyWavefold       = "ocelot_canopyWavefold";
constexpr const char* canopyPartials       = "ocelot_canopyPartials";
constexpr const char* canopyDetune         = "ocelot_canopyDetune";
constexpr const char* canopySpectralFilter = "ocelot_canopySpectralFilter";
constexpr const char* canopyBreathe        = "ocelot_canopyBreathe";
constexpr const char* canopyShimmer        = "ocelot_canopyShimmer";
constexpr const char* canopyLevel          = "ocelot_canopyLevel";
constexpr const char* canopyPitch          = "ocelot_canopyPitch";

// ── Emergent (creature calls) ─────────────────────
constexpr const char* creatureType    = "ocelot_creatureType";
constexpr const char* creatureRate    = "ocelot_creatureRate";
constexpr const char* creaturePitch   = "ocelot_creaturePitch";
constexpr const char* creatureSpread  = "ocelot_creatureSpread";
constexpr const char* creatureTrigger = "ocelot_creatureTrigger";
constexpr const char* creatureLevel   = "ocelot_creatureLevel";
constexpr const char* creatureAttack  = "ocelot_creatureAttack";
constexpr const char* creatureDecay   = "ocelot_creatureDecay";

// ── FX ────────────────────────────────────────────
constexpr const char* reverbSize      = "ocelot_reverbSize";
constexpr const char* reverbMix       = "ocelot_reverbMix";
constexpr const char* delayTime       = "ocelot_delayTime";
constexpr const char* delayFeedback   = "ocelot_delayFeedback";
constexpr const char* delayMix        = "ocelot_delayMix";

// ── Amp Envelope ─────────────────────────────────
constexpr const char* ampAttack       = "ocelot_ampAttack";
constexpr const char* ampDecay        = "ocelot_ampDecay";
constexpr const char* ampSustain      = "ocelot_ampSustain";
constexpr const char* ampRelease      = "ocelot_ampRelease";

// ── Macros ────────────────────────────────────────
constexpr const char* macroProwl      = "ocelot_prowl";
constexpr const char* macroFoliage    = "ocelot_foliage";
constexpr const char* macroEcosystem  = "ocelot_ecosystem";
constexpr const char* macroCanopy     = "ocelot_canopy";

// ── Coupling ─────────────────────────────────────
constexpr const char* couplingLevel   = "ocelot_couplingLevel";
constexpr const char* couplingBus     = "ocelot_couplingBus";

} // namespace ParamIDs

// ── Enum string arrays ───────────────────────────

namespace Enums {

inline const juce::StringArray biome {
    "jungle", "underwater", "winter"
};
inline const juce::StringArray floorModel {
    "berimbau", "cuica", "agogo", "kalimba", "pandeiro", "log_drum"
};
inline const juce::StringArray creatureType {
    "bird_trill", "whale_song", "insect_drone", "frog_chirp", "wolf_howl", "synth_call"
};
inline const juce::StringArray creatureTrigger {
    "midi", "floor_amp", "canopy_peaks"
};
inline const juce::StringArray couplingBus {
    "understory", "canopy", "floor", "emergent"
};

} // namespace Enums

// ── Parameter Layout Factory ─────────────────────

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto floatParam = [](const juce::String& id, const juce::String& name,
                         float min, float max, float def, float skew = 1.0f) {
        return std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float>(min, max, 0.0f, skew), def);
    };

    auto intParam = [](const juce::String& id, const juce::String& name,
                       int min, int max, int def) {
        return std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID { id, 1 }, name, min, max, def);
    };

    auto choiceParam = [](const juce::String& id, const juce::String& name,
                          const juce::StringArray& choices, int defIndex) {
        return std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { id, 1 }, name, choices, defIndex);
    };

    using namespace ParamIDs;

    // ── Global ────────────────────────────────────
    layout.add(choiceParam(biome,          "Biome",           Enums::biome,    0));
    layout.add(floatParam (strataBalance,  "Strata Balance",  0.0f, 1.0f, 0.5f));
    layout.add(floatParam (ecosystemDepth, "Ecosystem Depth", 0.0f, 1.0f, 0.3f));
    layout.add(floatParam (humidity,       "Humidity",        0.0f, 1.0f, 0.4f));
    layout.add(floatParam (swing,          "Swing",           0.0f, 1.0f, 0.2f));
    layout.add(floatParam (density,        "Density",         0.0f, 1.0f, 0.5f));

    // ── Cross-Feed Matrix ─────────────────────────
    layout.add(floatParam(xfFloorUnder,  "XF Floor→Under",  -1.0f, 1.0f, 0.0f));
    layout.add(floatParam(xfFloorCanopy, "XF Floor→Canopy", -1.0f, 1.0f, 0.0f));
    layout.add(floatParam(xfFloorEmerg,  "XF Floor→Emerg",  -1.0f, 1.0f, 0.0f));
    layout.add(floatParam(xfUnderFloor,  "XF Under→Floor",  -1.0f, 1.0f, 0.0f));
    layout.add(floatParam(xfUnderCanopy, "XF Under→Canopy", -1.0f, 1.0f, 0.0f));
    layout.add(floatParam(xfUnderEmerg,  "XF Under→Emerg",  -1.0f, 1.0f, 0.0f));
    layout.add(floatParam(xfCanopyFloor, "XF Canopy→Floor", -1.0f, 1.0f, 0.0f));
    layout.add(floatParam(xfCanopyUnder, "XF Canopy→Under", -1.0f, 1.0f, 0.0f));
    layout.add(floatParam(xfCanopyEmerg, "XF Canopy→Emerg", -1.0f, 1.0f, 0.0f));
    layout.add(floatParam(xfEmergFloor,  "XF Emerg→Floor",  -1.0f, 1.0f, 0.0f));
    layout.add(floatParam(xfEmergUnder,  "XF Emerg→Under",  -1.0f, 1.0f, 0.0f));
    layout.add(floatParam(xfEmergCanopy, "XF Emerg→Canopy", -1.0f, 1.0f, 0.0f));

    // ── Floor ─────────────────────────────────────
    layout.add(choiceParam(floorModel,   "Floor Model",   Enums::floorModel, 3)); // kalimba default
    layout.add(floatParam (floorTension, "Floor Tension", 0.0f, 1.0f, 0.5f));
    layout.add(floatParam (floorStrike,  "Floor Strike",  0.0f, 1.0f, 0.5f));
    layout.add(floatParam (floorDamping, "Floor Damping", 0.0f, 1.0f, 0.4f));
    layout.add(intParam   (floorPattern, "Floor Pattern", 0, 15,       0));
    layout.add(floatParam (floorLevel,   "Floor Level",   0.0f, 1.0f, 0.7f));
    layout.add(floatParam (floorPitch,   "Floor Pitch",   0.0f, 1.0f, 0.5f));
    layout.add(floatParam (floorVelocity,"Floor Velocity",0.0f, 1.0f, 0.8f));

    // ── Understory ────────────────────────────────
    layout.add(intParam   (chopRate,        "Chop Rate",       1, 32,          8));
    layout.add(floatParam (chopSwing,       "Chop Swing",      0.0f, 1.0f,   0.1f));
    layout.add(floatParam (bitDepth,        "Bit Depth",       4.0f, 16.0f,  16.0f)); // float for smooth automation
    layout.add(floatParam (sampleRateRed,   "Sample Rate Red", 4000.0f, 44100.0f, 44100.0f, 0.3f));
    layout.add(floatParam (tapeWobble,      "Tape Wobble",     0.0f, 1.0f,   0.0f));
    layout.add(floatParam (tapeAge,         "Tape Age",        0.0f, 1.0f,   0.1f));
    layout.add(floatParam (dustLevel,       "Dust Level",      0.0f, 1.0f,   0.1f));
    layout.add(floatParam (understoryLevel, "Understory Level",0.0f, 1.0f,   0.6f));
    layout.add(floatParam (understorySrc,   "Understory Src",  0.0f, 1.0f,   0.0f)); // 0=internal, 1=coupling

    // ── Canopy ────────────────────────────────────
    layout.add(floatParam (canopyWavefold,       "Canopy Wavefold",  0.0f, 1.0f, 0.2f));
    layout.add(intParam   (canopyPartials,        "Canopy Partials",  1, 8,       4));
    layout.add(floatParam (canopyDetune,          "Canopy Detune",   0.0f, 1.0f, 0.15f));
    layout.add(floatParam (canopySpectralFilter,  "Canopy Spectral", 0.0f, 1.0f, 0.7f));
    layout.add(floatParam (canopyBreathe,         "Canopy Breathe",  0.0f, 1.0f, 0.3f));
    layout.add(floatParam (canopyShimmer,         "Canopy Shimmer",  0.0f, 1.0f, 0.0f));
    layout.add(floatParam (canopyLevel,           "Canopy Level",    0.0f, 1.0f, 0.5f));
    layout.add(floatParam (canopyPitch,           "Canopy Pitch",    0.0f, 1.0f, 0.5f));

    // ── Emergent ──────────────────────────────────
    layout.add(choiceParam(creatureType,    "Creature Type",    Enums::creatureType,    0));
    layout.add(floatParam (creatureRate,    "Creature Rate",    0.0f, 1.0f,  0.3f));
    layout.add(floatParam (creaturePitch,   "Creature Pitch",   0.0f, 1.0f,  0.5f));
    layout.add(floatParam (creatureSpread,  "Creature Spread",  0.0f, 1.0f,  0.4f));
    layout.add(choiceParam(creatureTrigger, "Creature Trigger", Enums::creatureTrigger, 0));
    layout.add(floatParam (creatureLevel,   "Creature Level",   0.0f, 1.0f,  0.4f));
    layout.add(floatParam (creatureAttack,  "Creature Attack",  0.0f, 1.0f,  0.3f));
    layout.add(floatParam (creatureDecay,   "Creature Decay",   0.0f, 1.0f,  0.5f));

    // ── FX ────────────────────────────────────────
    layout.add(floatParam (reverbSize,    "Reverb Size",     0.0f, 1.0f,   0.6f));
    layout.add(floatParam (reverbMix,     "Reverb Mix",      0.0f, 1.0f,   0.3f));
    layout.add(floatParam (delayTime,     "Delay Time",      0.0f, 1.0f,   0.4f));
    layout.add(floatParam (delayFeedback, "Delay Feedback",  0.0f, 0.92f,  0.35f));
    layout.add(floatParam (delayMix,      "Delay Mix",       0.0f, 1.0f,   0.2f));

    // ── Amp Envelope ─────────────────────────────
    layout.add(floatParam (ampAttack,  "Amp Attack",  0.001f, 8000.0f,  10.0f,  0.3f));
    layout.add(floatParam (ampDecay,   "Amp Decay",   50.0f,  4000.0f, 300.0f,  0.3f));
    layout.add(floatParam (ampSustain, "Amp Sustain", 0.0f,   1.0f,      0.8f));
    layout.add(floatParam (ampRelease, "Amp Release", 50.0f,  8000.0f, 600.0f,  0.3f));

    // ── Macros ────────────────────────────────────
    layout.add(floatParam (macroProwl,     "PROWL",     0.0f, 1.0f, 0.0f));
    layout.add(floatParam (macroFoliage,   "FOLIAGE",   0.0f, 1.0f, 0.0f));
    layout.add(floatParam (macroEcosystem, "ECOSYSTEM", 0.0f, 1.0f, 0.0f));
    layout.add(floatParam (macroCanopy,    "CANOPY",    0.0f, 1.0f, 0.0f));

    // ── Coupling ─────────────────────────────────
    layout.add(floatParam (couplingLevel, "Coupling Level", 0.0f, 1.0f, 0.0f));
    layout.add(choiceParam(couplingBus,   "Coupling Bus",   Enums::couplingBus, 0));

    return layout;
}

} // namespace xocelot
