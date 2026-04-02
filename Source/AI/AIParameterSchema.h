#pragma once
#include <juce_core/juce_core.h>
#include <map>
#include <vector>
#include <optional>
#include <cmath>

namespace xoceanus {

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
    // Advanced validation (research-backed)

    /// Check for "silent patch" — a preset that produces no audible output.
    /// Research Layer 1: detect zero-output configurations before they reach audio.
    struct SilentPatchCheck
    {
        bool isSilent = false;
        juce::StringArray reasons;
    };

    SilentPatchCheck checkSilentPatch (const std::map<juce::String, float>& params) const
    {
        SilentPatchCheck result;

        // Check all engines for zero level
        bool anyEngineAudible = false;
        for (const auto& [engineId, profile] : engineProfiles)
        {
            for (const auto& p : profile.parameters)
            {
                if (p.role == AIParamRole::Level)
                {
                    auto it = params.find (p.paramId);
                    if (it != params.end() && it->second > 0.01f)
                    {
                        anyEngineAudible = true;
                        break;
                    }
                }
            }
            if (anyEngineAudible) break;
        }

        if (! anyEngineAudible && ! params.empty())
        {
            // Check if any level-like params were set at all
            bool hasLevelParams = false;
            for (const auto& [id, profile] : engineProfiles)
                for (const auto& p : profile.parameters)
                    if (p.role == AIParamRole::Level)
                        if (params.count (p.paramId))
                            hasLevelParams = true;

            if (hasLevelParams)
            {
                result.isSilent = true;
                result.reasons.add ("All engine output levels are at or near zero.");
            }
        }

        return result;
    }

    /// Validate Sonic DNA matches the described intent.
    /// Research Section 6: "if user asks for warm, constrain brightness"
    /// Returns warnings if DNA values contradict the mood category.
    struct DNAValidation
    {
        bool adjusted = false;
        RecipeEngine::Recipe::SonicDNA correctedDNA;
        juce::StringArray warnings;
    };

    static DNAValidation validateDNAForMood (const RecipeEngine::Recipe::SonicDNA& dna,
                                              const juce::String& mood)
    {
        DNAValidation result;
        result.correctedDNA = dna;

        // Mood-appropriate DNA bounds (research: constrain perceptual space by intent)
        // These are soft guidelines — warn but don't hard-override
        struct DNABounds
        {
            float minBrightness = 0, maxBrightness = 1;
            float minWarmth = 0, maxWarmth = 1;
            float minMovement = 0, maxMovement = 1;
            float minDensity = 0, maxDensity = 1;
            float minSpace = 0, maxSpace = 1;
            float minAggression = 0, maxAggression = 1;
        };

        DNABounds bounds;

        if (mood == "Foundation")
        {
            // Foundation = solid, grounded, reliable
            bounds.minWarmth = 0.3f;
            bounds.maxAggression = 0.5f;
            bounds.maxMovement = 0.6f;
        }
        else if (mood == "Atmosphere")
        {
            // Atmosphere = spacious, evolving, ambient
            bounds.minSpace = 0.4f;
            bounds.minMovement = 0.2f;
            bounds.maxAggression = 0.4f;
        }
        else if (mood == "Entangled")
        {
            // Entangled = complex, interwoven, dense
            bounds.minDensity = 0.4f;
            bounds.minMovement = 0.3f;
        }
        else if (mood == "Prism")
        {
            // Prism = bright, colorful, refractive
            bounds.minBrightness = 0.4f;
        }
        else if (mood == "Flux")
        {
            // Flux = changing, dynamic, restless
            bounds.minMovement = 0.5f;
        }
        else if (mood == "Aether")
        {
            // Aether = ethereal, otherworldly, transcendent
            bounds.minSpace = 0.5f;
            bounds.maxAggression = 0.3f;
        }

        auto check = [&] (float& val, float minV, float maxV, const char* name)
        {
            if (val < minV)
            {
                result.warnings.add (juce::String ("DNA ") + name + " (" + juce::String (val, 2)
                                     + ") is low for mood '" + mood + "' — expected >= "
                                     + juce::String (minV, 2) + ". Nudging up.");
                val = minV;
                result.adjusted = true;
            }
            if (val > maxV)
            {
                result.warnings.add (juce::String ("DNA ") + name + " (" + juce::String (val, 2)
                                     + ") is high for mood '" + mood + "' — expected <= "
                                     + juce::String (maxV, 2) + ". Nudging down.");
                val = maxV;
                result.adjusted = true;
            }
        };

        check (result.correctedDNA.brightness, bounds.minBrightness, bounds.maxBrightness, "brightness");
        check (result.correctedDNA.warmth, bounds.minWarmth, bounds.maxWarmth, "warmth");
        check (result.correctedDNA.movement, bounds.minMovement, bounds.maxMovement, "movement");
        check (result.correctedDNA.density, bounds.minDensity, bounds.maxDensity, "density");
        check (result.correctedDNA.space, bounds.minSpace, bounds.maxSpace, "space");
        check (result.correctedDNA.aggression, bounds.minAggression, bounds.maxAggression, "aggression");

        return result;
    }

    /// Verify macro targets are valid continuous parameters (not choices/toggles)
    /// and that macro range + base value stays within parameter bounds.
    struct MacroValidation
    {
        bool valid = true;
        juce::StringArray warnings;
        juce::StringArray droppedTargets;
    };

    MacroValidation validateMacroTargets (const juce::String& macroLabel,
                                           const std::vector<juce::String>& targets,
                                           const std::vector<float>& ranges,
                                           const std::map<juce::String, float>& currentParams) const
    {
        MacroValidation result;

        for (size_t i = 0; i < targets.size(); ++i)
        {
            auto* def = getParam (targets[i]);
            if (def == nullptr) continue;  // Unknown param — can't validate

            // Macros shouldn't target discrete params
            if (def->type == AIParamType::Choice || def->type == AIParamType::Toggle)
            {
                result.warnings.add ("Macro " + macroLabel + " targets '"
                                     + targets[i] + "' which is a " +
                                     (def->type == AIParamType::Choice ? "choice" : "toggle")
                                     + " parameter — macros should target continuous params.");
                result.droppedTargets.add (targets[i]);
                result.valid = false;
            }

            // Check that base value + range doesn't go out of bounds
            if (i < ranges.size())
            {
                float range = ranges[i];
                auto it = currentParams.find (targets[i]);
                if (it != currentParams.end())
                {
                    float base = it->second;
                    float atMin = base - std::abs (range) * (def->maxValue - def->minValue);
                    float atMax = base + std::abs (range) * (def->maxValue - def->minValue);

                    if (atMin < def->minValue || atMax > def->maxValue)
                    {
                        result.warnings.add ("Macro " + macroLabel + " range on '"
                                             + targets[i] + "' may exceed parameter bounds at extremes.");
                    }
                }
            }
        }

        return result;
    }

    //--------------------------------------------------------------------------
    // Few-shot factory preset examples for AI prompts
    // Research (LLM2Fx): 5-10 examples per mood dramatically improve output quality

    struct FewShotExample
    {
        juce::String mood;
        juce::String description;
        juce::String recipeSnippet;  // Compact JSON showing engines + key params
    };

    void addFewShotExample (FewShotExample example)
    {
        fewShotExamples.push_back (std::move (example));
    }

    /// Generate few-shot examples section for a given mood (or all moods if empty)
    juce::String generateFewShotContext (const juce::String& targetMood = {}) const
    {
        juce::String ctx;
        ctx += "\n=== EXAMPLE RECIPES (follow this style) ===\n";

        int count = 0;
        for (const auto& ex : fewShotExamples)
        {
            if (targetMood.isNotEmpty() && ex.mood != targetMood)
                continue;

            ctx += "\nExample (" + ex.mood + "): \"" + ex.description + "\"\n";
            ctx += ex.recipeSnippet + "\n";

            if (++count >= 3) break;  // Max 3 examples to control prompt size
        }

        if (count == 0)
            ctx += "(No examples available for this mood.)\n";

        return ctx;
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
    std::vector<FewShotExample> fewShotExamples;
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
    registerStub ("Odyssey", "drift_", "FM synthesis with drift and slow evolving envelopes",
                  { "evolving pads", "ambient", "drones", "slow textures" });
    registerStub ("Oblong", "bob_", "Curious, bouncy synthesis with randomized elements",
                  { "weird basses", "experimental", "playful leads", "generative" });
    registerStub ("Obese", "fat_", "Massive distorted bass with saturation stages",
                  { "bass", "sub bass", "distorted bass", "aggressive", "fat sounds" });
    registerStub ("Onset", "perc_", "Noise-based percussion and transient design",
                  { "noise percussion", "hi-hats", "cymbals", "risers", "noise textures" });
    registerStub ("Overworld", "ow_", "Retro/chiptune synthesis across console eras",
                  { "chiptune", "retro", "8-bit", "16-bit", "game sounds" });
    registerStub ("Opal", "opal_", "Granular synthesis with shimmer and frost effects",
                  { "granular textures", "ambient", "shimmer pads", "frozen sounds", "glitch" });
    registerStub ("Orbital", "orb_", "Additive synthesis with partial control and spectral tilt",
                  { "bells", "organs", "harmonic textures", "spectral sounds", "overtone design" });
    registerStub ("Organon", "organon_", "Entropy-driven synthesis — chaos to order continuum",
                  { "chaos", "noise", "evolving textures", "experimental", "entropy" });
    registerStub ("Ouroboros", "ouro_", "Self-modulating feedback synthesis",
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
    registerStub ("Oceanic", "ocean_", "Spectral synthesis with ocean/water metaphor",
                  { "water textures", "spectral pads", "tidal movement", "fluid sounds" });
    registerStub ("Optic", "optic_", "Visual modulation synthesis with AutoPulse LFO matrix",
                  { "rhythmic modulation", "visual sync", "pulsing textures", "light patterns" });
    registerStub ("Oblique", "oblq_", "Prismatic bounce synthesis — RTJ x Funk x Tame Impala",
                  { "prismatic delays", "bouncing echoes", "funk bass", "psychedelic", "rhythmic" });

    //--------------------------------------------------------------------------
    // Few-shot examples (research: LLM2Fx found 5+ examples dramatically improve output)
    // One per mood category — shows the AI what a good recipe looks like

    schema.addFewShotExample ({
        "Foundation",
        "Tight punchy kick with analog warmth",
        R"({"engines":[{"id":"OddfeliX","parameters":{"snap_oscMode":0,"snap_snap":0.8,"snap_decay":0.15,)"
        R"("snap_filterCutoff":400,"snap_filterReso":0.2,"snap_level":0.85,"snap_pitchLock":1}}],)"
        R"("coupling":[],"macros":[{"label":"PUNCH","targets":["snap_snap","snap_decay"],"ranges":[0.3,-0.2]}],)"
        R"("dna":{"brightness":0.2,"warmth":0.7,"movement":0.1,"density":0.5,"space":0.1,"aggression":0.3}})"
    });

    schema.addFewShotExample ({
        "Atmosphere",
        "Warm evolving pad with slow drift and shimmer",
        R"({"engines":[{"id":"OddOscar","parameters":{"morph_scanPos":0.3,"morph_filterCutoff":3000,)"
        R"("morph_filterReso":0.35,"morph_drift":0.25,"morph_subLevel":0.4}},)"
        R"({"id":"Opal","parameters":{"opal_grainSize":200,"opal_density":15,"opal_shimmer":0.6}}],)"
        R"("coupling":[{"sourceSlot":0,"destSlot":1,"type":"LFOToPitch","intensity":0.15}],)"
        R"("macros":[{"label":"DRIFT","targets":["morph_drift","morph_scanPos"],"ranges":[0.4,0.3]},)"
        R"({"label":"SHIMMER","targets":["opal_shimmer","morph_filterCutoff"],"ranges":[0.5,0.3]}],)"
        R"("dna":{"brightness":0.4,"warmth":0.7,"movement":0.6,"density":0.4,"space":0.7,"aggression":0.1}})"
    });

    schema.addFewShotExample ({
        "Entangled",
        "Two engines feeding back into each other with chaotic FM",
        R"({"engines":[{"id":"Ouroboros","parameters":{"ouroboros_feedback":0.6}},)"
        R"({"id":"Origami","parameters":{"origami_foldPoint":0.4}}],)"
        R"("coupling":[{"sourceSlot":0,"destSlot":1,"type":"AudioToFM","intensity":0.35},)"
        R"({"sourceSlot":1,"destSlot":0,"type":"AudioToRing","intensity":0.25}],)"
        R"("macros":[{"label":"CHAOS","targets":["ouroboros_feedback","origami_foldPoint"],"ranges":[0.3,0.4]}],)"
        R"("dna":{"brightness":0.5,"warmth":0.3,"movement":0.8,"density":0.7,"space":0.3,"aggression":0.6}})"
    });

    schema.addFewShotExample ({
        "Prism",
        "Bright crystalline bell with spectral shimmer",
        R"({"engines":[{"id":"Obsidian","parameters":{"obsidian_pdDepth":0.4}},)"
        R"({"id":"Orbital","parameters":{"orbital_partialTilt":0.6}}],)"
        R"("coupling":[{"sourceSlot":0,"destSlot":1,"type":"FilterToFilter","intensity":0.3}],)"
        R"("macros":[{"label":"CRYSTAL","targets":["obsidian_pdDepth","orbital_partialTilt"],"ranges":[0.3,0.4]}],)"
        R"("dna":{"brightness":0.8,"warmth":0.3,"movement":0.4,"density":0.5,"space":0.5,"aggression":0.2}})"
    });

    schema.addFewShotExample ({
        "Flux",
        "Restless morphing bass that never sits still",
        R"({"engines":[{"id":"Obese","parameters":{"fat_satDrive":0.5}},)"
        R"({"id":"Oblong","parameters":{"bob_fltCutoff":1200}}],)"
        R"("coupling":[{"sourceSlot":0,"destSlot":1,"type":"RhythmToBlend","intensity":0.5}],)"
        R"("macros":[{"label":"MORPH","targets":["fat_satDrive","bob_fltCutoff"],"ranges":[0.4,0.5]}],)"
        R"("dna":{"brightness":0.4,"warmth":0.5,"movement":0.8,"density":0.6,"space":0.2,"aggression":0.5}})"
    });

    schema.addFewShotExample ({
        "Aether",
        "Ethereal otherworldly texture floating in space",
        R"({"engines":[{"id":"Opal","parameters":{"opal_grainSize":400,"opal_density":8,"opal_shimmer":0.7}},)"
        R"({"id":"Oceanic","parameters":{"oceanic_separation":0.6}}],)"
        R"("coupling":[{"sourceSlot":0,"destSlot":1,"type":"AudioToWavetable","intensity":0.3}],)"
        R"("macros":[{"label":"FLOAT","targets":["opal_grainSize","oceanic_separation"],"ranges":[0.5,0.3]}],)"
        R"("dna":{"brightness":0.5,"warmth":0.4,"movement":0.5,"density":0.3,"space":0.9,"aggression":0.0}})"
    });

    return schema;
}

} // namespace xoceanus
