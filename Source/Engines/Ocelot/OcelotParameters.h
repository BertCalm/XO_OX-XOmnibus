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

// ── Filter Envelope ───────────────────────────────
// D001: canopy spectral filter env depth — velocity × envelope level × depth.
constexpr const char* filterEnvDepth  = "ocelot_filterEnvDepth";

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

// ── Parameter Registration (vector-based, for XOlokun shared APVTS) ────

inline void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& p)
{
    auto F = [&](const char* id, const char* name, float lo, float hi, float def, float skew = 1.0f) {
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float>(lo, hi, 0.0f, skew), def));
    };
    auto I = [&](const char* id, const char* name, int lo, int hi, int def) {
        p.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID { id, 1 }, name, lo, hi, def));
    };
    auto C = [&](const char* id, const char* name, const juce::StringArray& ch, int def) {
        p.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID { id, 1 }, name, ch, def));
    };

    using namespace ParamIDs;

    // ── Global ────────────────────────────────────
    C(biome,          "Biome",           Enums::biome,    0);
    F(strataBalance,  "Strata Balance",  0.0f, 1.0f, 0.5f);
    F(ecosystemDepth, "Ecosystem Depth", 0.0f, 1.0f, 0.3f);
    F(humidity,       "Humidity",        0.0f, 1.0f, 0.4f);
    F(swing,          "Swing",           0.0f, 1.0f, 0.2f);
    F(density,        "Density",         0.0f, 1.0f, 0.5f);

    // ── Cross-Feed Matrix ─────────────────────────
    // Default values: the 4 most ecologically expressive routes are pre-wired
    // at low levels so the EcosystemMatrix has audible default behavior.
    // Users and macros can deepen or redirect any route.
    //   Floor→Canopy: percussion transients open the spectral filter (+0.2)
    //   Floor→Emerg:  loud hits trigger creature calls (+0.25)
    //   Emerg→Canopy: creature calls add shimmer to the canopy (+0.15)
    //   Canopy→Floor: spectral brightness slightly damps floor resonance (-0.1)
    // All others default to 0 (clean slate for sound design).
    F(xfFloorUnder,  "XF Floor->Under",  -1.0f, 1.0f, 0.0f);
    F(xfFloorCanopy, "XF Floor->Canopy", -1.0f, 1.0f, 0.2f);   // audible default: percussion opens canopy filter
    F(xfFloorEmerg,  "XF Floor->Emerg",  -1.0f, 1.0f, 0.25f);  // audible default: drum hits trigger creature calls
    F(xfUnderFloor,  "XF Under->Floor",  -1.0f, 1.0f, 0.0f);
    F(xfUnderCanopy, "XF Under->Canopy", -1.0f, 1.0f, 0.0f);
    F(xfUnderEmerg,  "XF Under->Emerg",  -1.0f, 1.0f, 0.0f);
    F(xfCanopyFloor, "XF Canopy->Floor", -1.0f, 1.0f, -0.1f);  // audible default: bright canopy slightly damps floor
    F(xfCanopyUnder, "XF Canopy->Under", -1.0f, 1.0f, 0.0f);
    F(xfCanopyEmerg, "XF Canopy->Emerg", -1.0f, 1.0f, 0.0f);
    F(xfEmergFloor,  "XF Emerg->Floor",  -1.0f, 1.0f, 0.0f);
    F(xfEmergUnder,  "XF Emerg->Under",  -1.0f, 1.0f, 0.0f);
    F(xfEmergCanopy, "XF Emerg->Canopy", -1.0f, 1.0f, 0.15f);  // audible default: creature calls add canopy shimmer

    // ── Floor ─────────────────────────────────────
    C(floorModel,   "Floor Model",   Enums::floorModel, 3);
    F(floorTension, "Floor Tension", 0.0f, 1.0f, 0.5f);
    F(floorStrike,  "Floor Strike",  0.0f, 1.0f, 0.5f);
    F(floorDamping, "Floor Damping", 0.0f, 1.0f, 0.4f);
    I(floorPattern, "Floor Pattern", 0, 15,       0);
    F(floorLevel,   "Floor Level",   0.0f, 1.0f, 0.7f);
    F(floorPitch,   "Floor Pitch",   0.0f, 1.0f, 0.5f);
    F(floorVelocity,"Floor Velocity",0.0f, 1.0f, 0.8f);

    // ── Understory ────────────────────────────────
    I(chopRate,        "Chop Rate",       1, 32,          8);
    F(chopSwing,       "Chop Swing",      0.0f, 1.0f,   0.1f);
    F(bitDepth,        "Bit Depth",       4.0f, 16.0f,  16.0f);
    F(sampleRateRed,   "Sample Rate Red", 4000.0f, 44100.0f, 44100.0f, 0.3f);
    F(tapeWobble,      "Tape Wobble",     0.0f, 1.0f,   0.0f);
    F(tapeAge,         "Tape Age",        0.0f, 1.0f,   0.1f);
    F(dustLevel,       "Dust Level",      0.0f, 1.0f,   0.1f);
    F(understoryLevel, "Understory Level",0.0f, 1.0f,   0.6f);
    F(understorySrc,   "Understory Src",  0.0f, 1.0f,   0.0f);

    // ── Canopy ────────────────────────────────────
    F(canopyWavefold,       "Canopy Wavefold",  0.0f, 1.0f, 0.2f);
    I(canopyPartials,       "Canopy Partials",  1, 8,       4);
    F(canopyDetune,         "Canopy Detune",   0.0f, 1.0f, 0.15f);
    F(canopySpectralFilter, "Canopy Spectral", 0.0f, 1.0f, 0.7f);
    F(canopyBreathe,        "Canopy Breathe",  0.0f, 1.0f, 0.3f);
    F(canopyShimmer,        "Canopy Shimmer",  0.0f, 1.0f, 0.0f);
    F(canopyLevel,          "Canopy Level",    0.0f, 1.0f, 0.5f);
    F(canopyPitch,          "Canopy Pitch",    0.0f, 1.0f, 0.5f);

    // ── Emergent ──────────────────────────────────
    C(creatureType,    "Creature Type",    Enums::creatureType,    0);
    F(creatureRate,    "Creature Rate",    0.0f, 1.0f,  0.3f);
    F(creaturePitch,   "Creature Pitch",   0.0f, 1.0f,  0.5f);
    F(creatureSpread,  "Creature Spread",  0.0f, 1.0f,  0.4f);
    C(creatureTrigger, "Creature Trigger", Enums::creatureTrigger, 0);
    F(creatureLevel,   "Creature Level",   0.0f, 1.0f,  0.4f);
    F(creatureAttack,  "Creature Attack",  0.0f, 1.0f,  0.3f, 0.3f);
    F(creatureDecay,   "Creature Decay",   0.0f, 1.0f,  0.5f, 0.3f);

    // ── FX ────────────────────────────────────────
    F(reverbSize,    "Reverb Size",     0.0f, 1.0f,   0.6f);
    F(reverbMix,     "Reverb Mix",      0.0f, 1.0f,   0.3f);
    F(delayTime,     "Delay Time",      0.0f, 1.0f,   0.4f);
    F(delayFeedback, "Delay Feedback",  0.0f, 0.92f,  0.35f);
    F(delayMix,      "Delay Mix",       0.0f, 1.0f,   0.2f);

    // ── Filter Envelope ──────────────────────────
    // D001: spectral LP cutoff boost = depth × velocity × ampEnvLevel.
    // Default 0.25: at full velocity and attack peak, spectral filter position
    // moves up by +0.075 (about +1500 Hz in the 200-20kHz log range at mid position),
    // brightening hard hits through the canopy's partial LP filter.
    F(filterEnvDepth, "Filter Env Depth", 0.0f, 1.0f, 0.25f);

    // ── Amp Envelope ─────────────────────────────
    F(ampAttack,  "Amp Attack",  0.001f, 8000.0f,  10.0f,  0.3f);
    F(ampDecay,   "Amp Decay",   50.0f,  4000.0f, 300.0f,  0.3f);
    F(ampSustain, "Amp Sustain", 0.0f,   1.0f,      0.8f);
    F(ampRelease, "Amp Release", 50.0f,  8000.0f, 600.0f,  0.3f);

    // ── Macros ────────────────────────────────────
    F(macroProwl,     "PROWL",     0.0f, 1.0f, 0.0f);
    F(macroFoliage,   "FOLIAGE",   0.0f, 1.0f, 0.0f);
    F(macroEcosystem, "ECOSYSTEM", 0.0f, 1.0f, 0.0f);
    F(macroCanopy,    "CANOPY",    0.0f, 1.0f, 0.0f);

    // ── Coupling ─────────────────────────────────
    F(couplingLevel, "Coupling Level", 0.0f, 1.0f, 0.0f);
    C(couplingBus,   "Coupling Bus",   Enums::couplingBus, 0);
}

// ── Standalone Parameter Layout (wraps addParameters) ────

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    addParameters(params);
    return { params.begin(), params.end() };
}

} // namespace xocelot
