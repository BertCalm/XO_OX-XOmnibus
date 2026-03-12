#pragma once
#include <juce_core/juce_core.h>
#include <map>
#include <vector>
#include <optional>
#include <cmath>

namespace xomnibus {

//==============================================================================
// AIParameterSchema — Structured parameter knowledge for the AI assistant.
//
// This is the bridge between SoundAssistant and the actual DSP parameter space.
// It serves three critical purposes:
//
// 1. PROMPT ENRICHMENT — Tells the AI exactly what parameters exist, their
//    types, ranges, and musical meaning so it can make informed choices.
//
// 2. OUTPUT VALIDATION — Clamps, rounds, and verifies every value the AI
//    returns before it touches any parameter. Prevents clicks, DC offset,
//    feedback loops, and out-of-range values.
//
// 3. SAFETY CONSTRAINTS — Enforces parameter interdependencies (e.g.,
//    "if feedback > 0.9, clamp delay time to > 10ms to prevent runaway"),
//    denormal-prone zones, and known dangerous combinations.
//
// Design:
//   - Populated once at startup from each engine's createParameterLayout()
//   - Augmented with hand-tuned musical metadata (sweet spots, descriptions)
//   - Immutable after construction — safe to read from any thread
//   - Serializes to JSON for injection into AI prompts
//
// Key insight from research:
//   Modern AI-synth approaches (RAVE, Magenta, Neural Audio Synthesis) all
//   agree: you get FAR better results constraining the output space tightly
//   and giving the model explicit parameter semantics than letting it hallucinate
//   values from text descriptions alone. This schema is that constraint.
//==============================================================================

//------------------------------------------------------------------------------
// Parameter type classification — informs how the AI should reason about values
enum class AIParamType
{
    Continuous,     // Float 0-1 or similar (filter cutoff, level, etc.)
    Frequency,      // Hz range (20-20000 typically, log scaling)
    Time,           // Seconds/ms (decay, delay time, attack)
    Semitones,      // Pitch offset (-24 to +24 typically)
    Percentage,     // 0-100%
    Choice,         // Discrete options (osc mode, filter type, etc.)
    Toggle,         // Boolean on/off
    Unipolar,       // 0 to 1
    Bipolar,        // -1 to +1
    Integer         // Discrete integer (voice count, etc.)
};

//------------------------------------------------------------------------------
// Musical role — tells the AI what this parameter does in sound design terms
enum class AIParamRole
{
    // Tone shaping
    OscillatorPitch,
    OscillatorShape,
    FilterCutoff,
    FilterResonance,

    // Dynamics
    Level,
    Attack,
    Decay,
    Sustain,
    Release,

    // Movement
    LFORate,
    LFODepth,
    Drift,
    ModAmount,

    // Space/FX
    DelayTime,
    DelayFeedback,
    ReverbSize,
    ReverbMix,

    // Character
    Drive,
    BitDepth,
    SampleRate,

    // Engine-specific
    GrainSize,
    GrainDensity,
    MorphPosition,
    CouplingAmount,

    // Meta
    VoiceCount,
    UnisonDetune,
    Mix,
    Other
};

//------------------------------------------------------------------------------
// Single parameter definition
struct AIParamDef
{
    juce::String paramId;           // Exact JUCE parameter ID (e.g., "snap_filterCutoff")
    juce::String displayName;       // Human name (e.g., "Filter Cutoff")
    juce::String engineId;          // Owning engine (e.g., "OddfeliX")

    AIParamType type = AIParamType::Continuous;
    AIParamRole role = AIParamRole::Other;

    // Range
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.5f;
    float stepSize = 0.01f;
    float skew = 1.0f;             // >1 = log-like (frequencies), <1 = exp-like

    // For Choice/Integer types
    juce::StringArray choices;      // Option labels for Choice type

    // Musical metadata (hand-tuned)
    float sweetSpotMin = 0.0f;      // Typical useful minimum
    float sweetSpotMax = 1.0f;      // Typical useful maximum
    juce::String musicDescription;  // e.g., "Controls brightness. Low = warm, high = bright and thin."

    // Safety constraints
    bool canCauseRunaway = false;   // True for feedback, resonance at extremes
    float safeMax = 1.0f;           // AI should not exceed this (e.g., feedback < 0.95)
    juce::String safetyNote;        // e.g., "Above 0.9 causes self-oscillation"

    //--------------------------------------------------------------------------
    // Validation

    /// Clamp a value to the valid range
    float clamp (float value) const noexcept
    {
        return std::max (minValue, std::min (maxValue, value));
    }

    /// Clamp to the safe range (AI-suggested values go through this)
    float clampSafe (float value) const noexcept
    {
        return std::max (minValue, std::min (safeMax, value));
    }

    /// Round to step size
    float quantize (float value) const noexcept
    {
        if (stepSize <= 0.0f) return value;
        return std::round ((value - minValue) / stepSize) * stepSize + minValue;
    }

    /// Full validation pipeline: clamp → safety check → quantize
    float validate (float value) const noexcept
    {
        return quantize (clampSafe (value));
    }

    /// Check if a value is in the sweet spot
    bool isInSweetSpot (float value) const noexcept
    {
        return value >= sweetSpotMin && value <= sweetSpotMax;
    }
};

//------------------------------------------------------------------------------
// Engine-level metadata
struct AIEngineProfile
{
    juce::String engineId;
    juce::String displayName;       // e.g., "OddfeliX"
    juce::String paramPrefix;       // e.g., "snap_"
    juce::String character;         // e.g., "Percussive transient synthesis"

    // What this engine is good for (sound design guidance)
    juce::StringArray strengths;    // e.g., "kicks", "plucks", "metallic hits"
    juce::StringArray weaknesses;   // e.g., "sustained pads", "slow evolving textures"

    // Coupling compatibility
    juce::StringArray bestSendTypes;    // CouplingType names this engine sends well
    juce::StringArray bestReceiveTypes; // CouplingType names this engine receives well
    juce::StringArray recommendedPairings; // Engine IDs that pair well

    // All parameters for this engine
    std::vector<AIParamDef> parameters;

    /// Find a parameter by ID
    const AIParamDef* findParam (const juce::String& paramId) const
    {
        for (const auto& p : parameters)
            if (p.paramId == paramId)
                return &p;
        return nullptr;
    }
};

//------------------------------------------------------------------------------
// Coupling constraint (safety rule for specific coupling combinations)
struct AICouplingConstraint
{
    juce::String couplingType;
    float maxSafeIntensity = 1.0f;
    juce::String description;       // Musical description for the AI
    juce::StringArray incompatibleWith; // Other coupling types that conflict
    juce::String safetyNote;        // e.g., "AudioToFM above 0.7 becomes harsh noise"
};

//------------------------------------------------------------------------------
// Cross-parameter safety rule (e.g., "high feedback + short delay = runaway")
struct AISafetyRule
{
    juce::String name;
    juce::String description;

    // The condition that triggers the rule
    struct Condition
    {
        juce::String paramId;
        float threshold = 0.0f;
        bool above = true;          // true = "if param > threshold", false = "if param < threshold"
    };
    std::vector<Condition> conditions; // ALL must be true (AND logic)

    // The fix to apply
    struct Fix
    {
        juce::String paramId;
        float clampMin = 0.0f;
        float clampMax = 1.0f;
    };
    std::vector<Fix> fixes;
};


//==============================================================================
// The complete schema
//==============================================================================
class AIParameterSchema
{
public:
    AIParameterSchema() = default;

    //--------------------------------------------------------------------------
    // Registration (called at startup)

    void registerEngine (AIEngineProfile profile)
    {
        engineProfiles[profile.engineId] = std::move (profile);
    }

    void registerCouplingConstraint (AICouplingConstraint constraint)
    {
        couplingConstraints[constraint.couplingType] = std::move (constraint);
    }

    void addSafetyRule (AISafetyRule rule)
    {
        safetyRules.push_back (std::move (rule));
    }

    //--------------------------------------------------------------------------
    // Query

    const AIEngineProfile* getEngine (const juce::String& engineId) const
    {
        auto it = engineProfiles.find (engineId);
        return it != engineProfiles.end() ? &it->second : nullptr;
    }

    const AIParamDef* getParam (const juce::String& paramId) const
    {
        for (const auto& [_, profile] : engineProfiles)
            if (auto* p = profile.findParam (paramId))
                return p;
        return nullptr;
    }

    const AICouplingConstraint* getCouplingConstraint (const juce::String& type) const
    {
        auto it = couplingConstraints.find (type);
        return it != couplingConstraints.end() ? &it->second : nullptr;
    }

    //--------------------------------------------------------------------------
    // Validation — call on every AI response before applying

    struct ValidationResult
    {
        bool valid = true;
        std::map<juce::String, float> correctedParams;  // paramId → corrected value
        juce::StringArray warnings;
        juce::StringArray errors;
    };

    /// Validate and correct a full set of AI-suggested parameters
    ValidationResult validateParameters (const std::map<juce::String, float>& params) const
    {
        ValidationResult result;

        // Phase 1: Individual parameter validation
        for (const auto& [paramId, value] : params)
        {
            auto* def = getParam (paramId);
            if (def == nullptr)
            {
                result.errors.add ("Unknown parameter: " + paramId);
                result.valid = false;
                continue;
            }

            float corrected = def->validate (value);
            result.correctedParams[paramId] = corrected;

            if (std::abs (corrected - value) > 0.001f)
            {
                result.warnings.add (paramId + ": " + juce::String (value, 3)
                                     + " → " + juce::String (corrected, 3)
                                     + " (clamped to safe range)");
            }

            if (! def->isInSweetSpot (corrected))
            {
                result.warnings.add (paramId + ": value " + juce::String (corrected, 3)
                                     + " is outside sweet spot ["
                                     + juce::String (def->sweetSpotMin, 2) + "-"
                                     + juce::String (def->sweetSpotMax, 2) + "]");
            }
        }

        // Phase 2: Cross-parameter safety rules
        for (const auto& rule : safetyRules)
        {
            bool triggered = true;
            for (const auto& cond : rule.conditions)
            {
                auto it = result.correctedParams.find (cond.paramId);
                if (it == result.correctedParams.end())
                {
                    triggered = false;
                    break;
                }

                float val = it->second;
                if (cond.above && val <= cond.threshold) triggered = false;
                if (!cond.above && val >= cond.threshold) triggered = false;
            }

            if (triggered)
            {
                for (const auto& fix : rule.fixes)
                {
                    auto it = result.correctedParams.find (fix.paramId);
                    if (it != result.correctedParams.end())
                    {
                        float before = it->second;
                        it->second = std::max (fix.clampMin, std::min (fix.clampMax, it->second));
                        if (std::abs (it->second - before) > 0.001f)
                        {
                            result.warnings.add ("Safety rule '" + rule.name + "': "
                                                 + fix.paramId + " adjusted to "
                                                 + juce::String (it->second, 3));
                        }
                    }
                }
            }
        }

        return result;
    }

    /// Validate coupling suggestions
    ValidationResult validateCoupling (const juce::String& couplingType,
                                        float intensity) const
    {
        ValidationResult result;

        auto* constraint = getCouplingConstraint (couplingType);
        if (constraint == nullptr)
        {
            // Unknown coupling type — not necessarily an error (future types)
            result.warnings.add ("Unknown coupling type: " + couplingType);
            result.correctedParams["intensity"] = std::max (0.0f, std::min (1.0f, intensity));
            return result;
        }

        float corrected = std::max (0.0f, std::min (constraint->maxSafeIntensity, intensity));
        result.correctedParams["intensity"] = corrected;

        if (std::abs (corrected - intensity) > 0.001f)
        {
            result.warnings.add (couplingType + " intensity clamped from "
                                 + juce::String (intensity, 3) + " to "
                                 + juce::String (corrected, 3)
                                 + " — " + constraint->safetyNote);
        }

        return result;
    }

    //--------------------------------------------------------------------------
    // Prompt generation — structured context for the AI

    /// Generate a compact parameter reference for active engines only.
    /// This is injected into every AI prompt so the model knows exactly
    /// what parameters exist and what values are reasonable.
    juce::String generatePromptContext (const juce::StringArray& activeEngines) const
    {
        juce::String ctx;
        ctx += "=== PARAMETER REFERENCE (use ONLY these parameter IDs) ===\n\n";

        for (const auto& engineId : activeEngines)
        {
            auto* profile = getEngine (engineId);
            if (profile == nullptr) continue;

            ctx += "ENGINE: " + profile->displayName
                   + " (prefix: " + profile->paramPrefix + ")\n";
            ctx += "Character: " + profile->character + "\n";
            ctx += "Strengths: " + profile->strengths.joinIntoString (", ") + "\n";
            ctx += "Parameters:\n";

            for (const auto& p : profile->parameters)
            {
                ctx += "  " + p.paramId + " ";

                if (p.type == AIParamType::Choice)
                {
                    ctx += "[" + p.choices.joinIntoString ("|") + "] ";
                    ctx += "default=" + juce::String (static_cast<int> (p.defaultValue));
                }
                else if (p.type == AIParamType::Toggle)
                {
                    ctx += "[on/off] default=" + juce::String (p.defaultValue > 0.5f ? "on" : "off");
                }
                else
                {
                    ctx += "[" + juce::String (p.minValue, 1) + "-"
                           + juce::String (p.maxValue, 1) + "] ";
                    ctx += "default=" + juce::String (p.defaultValue, 2) + " ";
                    ctx += "sweet=[" + juce::String (p.sweetSpotMin, 2) + "-"
                           + juce::String (p.sweetSpotMax, 2) + "]";
                }

                if (p.musicDescription.isNotEmpty())
                    ctx += " — " + p.musicDescription;

                if (p.safetyNote.isNotEmpty())
                    ctx += " ⚠ " + p.safetyNote;

                ctx += "\n";
            }
            ctx += "\n";
        }

        // Coupling reference
        ctx += "=== COUPLING TYPES ===\n";
        for (const auto& [type, constraint] : couplingConstraints)
        {
            ctx += type + " (max intensity: "
                   + juce::String (constraint.maxSafeIntensity, 2) + ") — "
                   + constraint.description;
            if (constraint.safetyNote.isNotEmpty())
                ctx += " ⚠ " + constraint.safetyNote;
            ctx += "\n";
        }

        // Engine pairing hints for active engines
        ctx += "\n=== PAIRING HINTS ===\n";
        for (const auto& engineId : activeEngines)
        {
            auto* profile = getEngine (engineId);
            if (profile == nullptr) continue;

            if (! profile->recommendedPairings.isEmpty())
            {
                ctx += profile->displayName + " pairs well with: "
                       + profile->recommendedPairings.joinIntoString (", ") + "\n";
            }
            if (! profile->bestSendTypes.isEmpty())
            {
                ctx += profile->displayName + " sends: "
                       + profile->bestSendTypes.joinIntoString (", ") + "\n";
            }
            if (! profile->bestReceiveTypes.isEmpty())
            {
                ctx += profile->displayName + " receives: "
                       + profile->bestReceiveTypes.joinIntoString (", ") + "\n";
            }
        }

        return ctx;
    }

    /// Generate the safety rules section for the prompt
    juce::String generateSafetyPrompt() const
    {
        juce::String s;
        s += "\n=== SAFETY RULES (MUST FOLLOW) ===\n";
        s += "1. Use ONLY parameter IDs listed above. Do NOT invent parameter names.\n";
        s += "2. Every float value must be within [min, max] of its parameter.\n";
        s += "3. Sweet spots produce the best results — stay in range unless user asks for extremes.\n";
        s += "4. Choice parameters must use integer index (0-based), not string labels.\n";
        s += "5. Coupling intensity must be 0.0–1.0 unless the constraint says otherwise.\n";

        for (const auto& rule : safetyRules)
        {
            s += "6. " + rule.description + "\n";
        }

        s += "7. Every macro must map to parameters that actually exist in the active engines.\n";
        s += "8. Variation axis targets must be continuous parameters (not choices/toggles).\n";
        s += "9. DNA values must be 0.0–1.0.\n";
        s += "10. If unsure about a value, use the default.\n";

        return s;
    }

    //--------------------------------------------------------------------------
    // Serialization (for caching/debugging)

    juce::String toJSON() const
    {
        auto* root = new juce::DynamicObject();

        juce::Array<juce::var> engines;
        for (const auto& [id, profile] : engineProfiles)
        {
            auto* eng = new juce::DynamicObject();
            eng->setProperty ("id", profile.engineId);
            eng->setProperty ("prefix", profile.paramPrefix);
            eng->setProperty ("character", profile.character);

            juce::Array<juce::var> params;
            for (const auto& p : profile.parameters)
            {
                auto* param = new juce::DynamicObject();
                param->setProperty ("id", p.paramId);
                param->setProperty ("name", p.displayName);
                param->setProperty ("min", p.minValue);
                param->setProperty ("max", p.maxValue);
                param->setProperty ("default", p.defaultValue);
                param->setProperty ("sweetMin", p.sweetSpotMin);
                param->setProperty ("sweetMax", p.sweetSpotMax);
                param->setProperty ("safeMax", p.safeMax);
                param->setProperty ("description", p.musicDescription);
                if (p.safetyNote.isNotEmpty())
                    param->setProperty ("safety", p.safetyNote);
                if (p.choices.size() > 0)
                    param->setProperty ("choices", p.choices.joinIntoString ("|"));
                params.add (juce::var (param));
            }
            eng->setProperty ("parameters", params);
            engines.add (juce::var (eng));
        }
        root->setProperty ("engines", engines);

        return juce::JSON::toString (juce::var (root), true);
    }

private:
    std::map<juce::String, AIEngineProfile> engineProfiles;
    std::map<juce::String, AICouplingConstraint> couplingConstraints;
    std::vector<AISafetyRule> safetyRules;
};


//==============================================================================
// Factory function — builds the schema with all engine knowledge.
// Call once at startup.
//==============================================================================
inline AIParameterSchema buildDefaultSchema()
{
    AIParameterSchema schema;

    //--------------------------------------------------------------------------
    // ODDFELIX (OddfeliX) — Percussive transient synthesis
    {
        AIEngineProfile p;
        p.engineId = "OddfeliX";
        p.displayName = "OddfeliX";
        p.paramPrefix = "snap_";
        p.character = "Percussive transient synthesis — punchy, clicky, snappy. "
                      "Every note starts with a pitch-sweep transient.";
        p.strengths = { "kicks", "toms", "plucks", "metallic hits", "percussion", "transients" };
        p.weaknesses = { "sustained pads", "slow evolving textures" };
        p.bestSendTypes = { "AmpToFilter", "AmpToChoke" };
        p.bestReceiveTypes = { "AmpToPitch", "LFOToPitch" };
        p.recommendedPairings = { "Overdub", "Odyssey", "Oblique" };

        p.parameters = {
            { "snap_oscMode", "Osc Mode", "OddfeliX", AIParamType::Choice, AIParamRole::OscillatorShape,
              0, 2, 0, 1, 1, { "Sine+Noise", "FM", "Karplus-Strong" }, 0, 2,
              "Sine+Noise=round, FM=metallic, KS=plucky strings", false, 2.0f, "" },
            { "snap_snap", "Snap", "OddfeliX", AIParamType::Unipolar, AIParamRole::Other,
              0, 1, 0.4f, 0.01f, 1, {}, 0.3f, 0.6f,
              "Transient intensity. Higher = more click.", false, 1.0f, "" },
            { "snap_decay", "Decay", "OddfeliX", AIParamType::Time, AIParamRole::Decay,
              0, 8, 0.5f, 0.01f, 1, {}, 0.1f, 0.4f,
              "Envelope decay time. Short for clicks, longer for toms.", false, 8.0f, "" },
            { "snap_filterCutoff", "Filter Cutoff", "OddfeliX", AIParamType::Frequency, AIParamRole::FilterCutoff,
              20, 20000, 2000, 0.1f, 0.3f, {}, 1000, 4000,
              "HPF→BPF cascade cutoff. Lower=warmer, higher=brighter.", false, 20000, "" },
            { "snap_filterReso", "Filter Resonance", "OddfeliX", AIParamType::Unipolar, AIParamRole::FilterResonance,
              0, 1, 0.3f, 0.01f, 1, {}, 0.2f, 0.5f,
              "Resonance. Higher adds ring to percussion.", true, 0.85f,
              "Above 0.85 can produce unwanted ringing on short sounds" },
            { "snap_detune", "Detune", "OddfeliX", AIParamType::Continuous, AIParamRole::UnisonDetune,
              0, 50, 10, 0.1f, 1, {}, 5, 15,
              "Unison detune spread in cents. Wider = bigger.", false, 50, "" },
            { "snap_level", "Level", "OddfeliX", AIParamType::Unipolar, AIParamRole::Level,
              0, 1, 0.8f, 0.01f, 1, {}, 0.5f, 0.9f,
              "Output level.", false, 1.0f, "" },
            { "snap_pitchLock", "Pitch Lock", "OddfeliX", AIParamType::Toggle, AIParamRole::Other,
              0, 1, 0, 1, 1, {}, 0, 1,
              "When on, pitch doesn't track keyboard — fixed pitch mode.", false, 1.0f, "" },
            { "snap_unison", "Unison", "OddfeliX", AIParamType::Choice, AIParamRole::VoiceCount,
              0, 2, 0, 1, 1, { "1", "2", "4" }, 0, 2,
              "Unison voice count. More = wider stereo.", false, 2.0f, "" },
            { "snap_polyphony", "Polyphony", "OddfeliX", AIParamType::Choice, AIParamRole::VoiceCount,
              0, 3, 2, 1, 1, { "1", "2", "4", "8" }, 0, 3,
              "Polyphony mode. 1=mono, 8=full poly.", false, 3.0f, "" }
        };

        schema.registerEngine (std::move (p));
    }

    //--------------------------------------------------------------------------
    // ODDOSCAR (OddOscar) — Lush pad synthesis
    {
        AIEngineProfile p;
        p.engineId = "OddOscar";
        p.displayName = "OddOscar";
        p.paramPrefix = "morph_";
        p.character = "Lush pad synthesis with wavetable morph. Three detuned oscillators "
                      "+ sub osc + Moog-style ladder filter with Perlin noise drift.";
        p.strengths = { "pads", "evolving textures", "warm analog", "wide stereo", "ambient beds" };
        p.weaknesses = { "sharp transients", "percussion", "rhythmic patterns" };
        p.bestSendTypes = { "LFOToPitch" };
        p.bestReceiveTypes = { "AmpToFilter", "EnvToMorph" };
        p.recommendedPairings = { "OddfeliX", "Oblong", "Optic" };

        p.parameters = {
            { "morph_scanPos", "Scan Position", "OddOscar", AIParamType::Unipolar, AIParamRole::MorphPosition,
              0, 1, 0.3f, 0.001f, 1, {}, 0.2f, 0.6f,
              "Wavetable morph. 0=Sine, 0.33=Saw, 0.66=Square, 1=Noise", false, 1.0f, "" },
            { "morph_filterCutoff", "Filter Cutoff", "OddOscar", AIParamType::Frequency, AIParamRole::FilterCutoff,
              20, 20000, 4000, 0.1f, 0.3f, {}, 2000, 6000,
              "Moog ladder filter cutoff", false, 20000, "" },
            { "morph_filterReso", "Filter Resonance", "OddOscar", AIParamType::Unipolar, AIParamRole::FilterResonance,
              0, 1, 0.3f, 0.01f, 1, {}, 0.3f, 0.6f,
              "Ladder resonance. Self-oscillates above 0.9.", true, 0.88f,
              "Above 0.88 causes self-oscillation — only use intentionally" },
            { "morph_drift", "Drift", "OddOscar", AIParamType::Unipolar, AIParamRole::Drift,
              0, 1, 0.15f, 0.01f, 1, {}, 0.1f, 0.3f,
              "Perlin noise pitch drift. Analog warmth feel.", false, 1.0f, "" },
            { "morph_subLevel", "Sub Level", "OddOscar", AIParamType::Unipolar, AIParamRole::Level,
              0, 1, 0.3f, 0.01f, 1, {}, 0.3f, 0.5f,
              "Sub oscillator level (one octave below). Adds warmth and bass.", false, 1.0f, "" }
        };

        schema.registerEngine (std::move (p));
    }

    //--------------------------------------------------------------------------
    // Coupling constraints
    {
        schema.registerCouplingConstraint ({
            "AmpToFilter", 1.0f,
            "Engine A amplitude modulates Engine B filter cutoff. Great for sidechain pump effects.",
            {}, ""
        });
        schema.registerCouplingConstraint ({
            "AmpToPitch", 0.8f,
            "Engine A amplitude modulates Engine B pitch. Subtle values (0.1-0.3) add organic movement.",
            {}, "Above 0.5 creates obvious pitch wobble"
        });
        schema.registerCouplingConstraint ({
            "LFOToPitch", 0.6f,
            "Engine A LFO modulates Engine B pitch. Classic vibrato/tremolo.",
            {}, "Above 0.4 creates obvious detune effects"
        });
        schema.registerCouplingConstraint ({
            "EnvToMorph", 1.0f,
            "Engine A envelope controls Engine B wavetable/morph position. Dynamic timbre changes.",
            {}, ""
        });
        schema.registerCouplingConstraint ({
            "AudioToFM", 0.7f,
            "Engine A audio directly FM-modulates Engine B. Creates metallic/bell tones at low intensity, noise at high.",
            {}, "Above 0.5 produces increasingly harsh/noisy results"
        });
        schema.registerCouplingConstraint ({
            "AudioToRing", 0.8f,
            "Ring modulation between Engine A and B. Creates sidebands. Low = subtle, high = atonal.",
            {}, "Above 0.6 becomes very atonal"
        });
        schema.registerCouplingConstraint ({
            "FilterToFilter", 1.0f,
            "Engine A filter output feeds Engine B filter input. Cascaded filtering.",
            {}, ""
        });
        schema.registerCouplingConstraint ({
            "AmpToChoke", 1.0f,
            "Engine A amplitude ducks Engine B. Sidechain/ducking effect.",
            {}, ""
        });
        schema.registerCouplingConstraint ({
            "RhythmToBlend", 1.0f,
            "Engine A rhythm pattern controls Engine B blend. Rhythmic crossfading.",
            {}, ""
        });
        schema.registerCouplingConstraint ({
            "EnvToDecay", 0.8f,
            "Engine A envelope modulates Engine B decay time. Interactive dynamics.",
            {}, "High values can make Engine B inaudible"
        });
        schema.registerCouplingConstraint ({
            "PitchToPitch", 0.5f,
            "Engine A pitch tracks to Engine B pitch. Harmony/tracking.",
            { "AudioToFM" }, "Above 0.3 creates strong forced harmony — can clash"
        });
        schema.registerCouplingConstraint ({
            "AudioToWavetable", 0.8f,
            "Engine A audio becomes Engine B wavetable source. Spectral transfer.",
            {}, "Above 0.6, original B character is lost"
        });
    }

    //--------------------------------------------------------------------------
    // Safety rules
    {
        // Rule: High resonance + low cutoff = boomy DC buildup risk
        schema.addSafetyRule ({
            "resonance_cutoff_boom",
            "High filter resonance with very low cutoff can cause boomy DC buildup",
            {
                { "morph_filterReso", 0.8f, true },
                { "morph_filterCutoff", 100.0f, false }
            },
            {
                { "morph_filterCutoff", 80.0f, 20000.0f }  // Bump cutoff to at least 80Hz
            }
        });

        // Rule: Multiple AudioToFM couplings compound — reduce intensity
        // (This would need a more complex check; represented as a prompt instruction)

        // Rule: Very high snap + very short decay = DC click on some DAWs
        schema.addSafetyRule ({
            "snap_dc_click",
            "Very high snap intensity with very short decay can produce DC clicks",
            {
                { "snap_snap", 0.9f, true },
                { "snap_decay", 0.02f, false }
            },
            {
                { "snap_decay", 0.03f, 8.0f }  // Minimum 30ms decay with max snap
            }
        });
    }

    // NOTE: Remaining engines (Overdub, Odyssey, Oblong, Obese, Onset, Overworld,
    // Opal, Orbital, Organon, Ouroboros, Obsidian, Overbite, Origami, Oracle,
    // Obscura, Oceanic, Optic, Oblique) follow the same pattern.
    // Each engine's profile is populated from:
    //   1. createParameterLayout() — exact ranges and defaults
    //   2. Sound design guide — sweet spots and descriptions
    //   3. Testing — safety constraints for dangerous parameter combos
    //
    // Stub registrations are added below so the schema knows they exist.
    // Full parameter definitions should be populated as each engine is finalized.

    auto registerStub = [&] (const juce::String& id, const juce::String& prefix,
                              const juce::String& character,
                              const juce::StringArray& strengths)
    {
        AIEngineProfile p;
        p.engineId = id;
        p.displayName = id;
        p.paramPrefix = prefix;
        p.character = character;
        p.strengths = strengths;
        schema.registerEngine (std::move (p));
    };

    registerStub ("Overdub", "dub_", "Dub synth with tape delay, spring reverb, and drive",
                  { "dub techno", "reggae", "tape echo", "lo-fi", "ambient dub" });
    registerStub ("Odyssey", "odyssey_", "FM synthesis with drift and slow evolving envelopes",
                  { "evolving pads", "ambient", "drones", "slow textures" });
    registerStub ("Oblong", "bob_", "Curious, bouncy synthesis with randomized elements",
                  { "weird basses", "experimental", "playful leads", "generative" });
    registerStub ("Obese", "fat_", "Massive distorted bass with saturation stages",
                  { "bass", "sub bass", "distorted bass", "aggressive", "fat sounds" });
    registerStub ("Onset", "onset_", "Noise-based percussion and transient design",
                  { "noise percussion", "hi-hats", "cymbals", "risers", "noise textures" });
    registerStub ("Overworld", "era_", "Retro/chiptune synthesis across console eras",
                  { "chiptune", "retro", "8-bit", "16-bit", "game sounds" });
    registerStub ("Opal", "opal_", "Granular synthesis with shimmer and frost effects",
                  { "granular textures", "ambient", "shimmer pads", "frozen sounds", "glitch" });
    registerStub ("Orbital", "orbital_", "Additive synthesis with partial control and spectral tilt",
                  { "bells", "organs", "harmonic textures", "spectral sounds", "overtone design" });
    registerStub ("Organon", "organon_", "Entropy-driven synthesis — chaos to order continuum",
                  { "chaos", "noise", "evolving textures", "experimental", "entropy" });
    registerStub ("Ouroboros", "ouroboros_", "Self-modulating feedback synthesis",
                  { "feedback drones", "self-oscillation", "dark ambient", "evolving noise" });
    registerStub ("Obsidian", "obsidian_", "Phase distortion with crystalline/dark character",
                  { "crystal tones", "metallic", "glass", "dark textures", "phase distortion" });
    registerStub ("Overbite", "poss_", "Aggressive bite synthesis with teeth/fang character",
                  { "aggressive", "bite", "distortion", "harsh textures", "industrial" });
    registerStub ("Origami", "origami_", "Wavefolding synthesis — paper-fold metaphor",
                  { "wavefolding", "harmonics", "metallic", "complex timbres" });
    registerStub ("Oracle", "oracle_", "Breakpoint function synthesis — prophetic contours",
                  { "custom envelopes", "complex modulation", "generative", "contour design" });
    registerStub ("Obscura", "obscura_", "Physical modeling — stiff string and membrane",
                  { "strings", "drums", "physical modeling", "resonant bodies", "mallets" });
    registerStub ("Oceanic", "oceanic_", "Spectral synthesis with ocean/water metaphor",
                  { "water textures", "spectral pads", "tidal movement", "fluid sounds" });
    registerStub ("Optic", "optic_", "Visual modulation synthesis with AutoPulse LFO matrix",
                  { "rhythmic modulation", "visual sync", "pulsing textures", "light patterns" });
    registerStub ("Oblique", "oblq_", "Prismatic bounce synthesis — RTJ x Funk x Tame Impala",
                  { "prismatic delays", "bouncing echoes", "funk bass", "psychedelic", "rhythmic" });

    return schema;
}

} // namespace xomnibus
