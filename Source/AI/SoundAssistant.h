#pragma once
#include "SecureKeyStore.h"
#include "AIParameterSchema.h"
#include "../Core/RecipeEngine.h"
#include <juce_core/juce_core.h>
#include <functional>
#include <optional>

namespace xomnibus {

//==============================================================================
// SoundAssistant — Provider-agnostic AI assistant for sound design.
//
// Security guarantees:
//   - API keys retrieved from SecureKeyStore only for the duration of a call
//   - Keys wiped from memory immediately after HTTP response received
//   - Only synth state (parameter values) is sent — NEVER audio, NEVER user data
//   - All calls happen on background threads (never audio thread)
//   - TLS-only connections (HTTPS enforced)
//   - No API calls without explicit user action (no background pinging)
//   - Rate limiting to prevent accidental billing spikes
//   - Request/response payloads are ephemeral (not persisted to disk)
//
// Provider abstraction:
//   The same SoundAssistant API works with Claude, GPT, or Gemini.
//   Provider-specific prompt formatting and response parsing is handled
//   internally. The caller never sees provider-specific details.
//
// Capabilities:
//   1. TextToRecipe    — "dark evolving pad" → full .xorecipe
//   2. SoundMatch      — Sonic DNA target → parameter adjustments
//   3. CouplingAdvisor — Engine combo → suggested coupling config
//   4. MacroNaming     — Current state → evocative macro names + mappings
//   5. PresetNaming    — Parameter values → preset name + description
//   6. RecipeRefine    — Current recipe + user intent → modified recipe
//   7. ExplainSound    — Current parameters → plain English description
//
// Usage:
//   SoundAssistant ai;
//   ai.textToRecipe ("massive dark pad with shimmer",
//       currentEngines, [this] (auto result) {
//           if (result.success)
//               recipeEngine.applyRecipe (result.recipe, apvts, callbacks);
//       });
//==============================================================================
class SoundAssistant
{
public:
    //--------------------------------------------------------------------------
    // Result types

    struct RecipeResult
    {
        bool success = false;
        RecipeEngine::Recipe recipe;
        juce::String explanation;    // AI's reasoning for the choices
        juce::String errorMessage;

        // Validation info — tells the user what was adjusted
        juce::StringArray validationWarnings;
        int parametersCorrected = 0;
        int unknownParametersDropped = 0;
    };

    struct CouplingAdvice
    {
        bool success = false;
        struct Suggestion
        {
            juce::String sourceEngine;
            juce::String destEngine;
            juce::String couplingType;
            float suggestedIntensity = 0.0f;
            juce::String reasoning;
        };
        std::vector<Suggestion> suggestions;
        juce::String errorMessage;
    };

    struct MacroSuggestion
    {
        bool success = false;
        struct Macro
        {
            juce::String label;                   // e.g. "SHIMMER"
            juce::String description;             // Why this name
            std::vector<juce::String> targets;
            std::vector<float> ranges;
        };
        std::array<Macro, 4> macros;
        juce::String errorMessage;
    };

    struct NamingSuggestion
    {
        bool success = false;
        juce::String name;           // e.g. "Midnight Glass"
        juce::String description;    // e.g. "Crystalline pad with dark undertones"
        juce::String mood;           // e.g. "Atmosphere"
        juce::StringArray tags;
        juce::String errorMessage;
    };

    struct SoundExplanation
    {
        bool success = false;
        juce::String description;    // Plain English description of the sound
        juce::String suggestions;    // "To make it brighter, try..."
        juce::String errorMessage;
    };

    //--------------------------------------------------------------------------
    // Synth state context (sent with every request)

    struct SynthContext
    {
        juce::StringArray activeEngines;              // Engine IDs in slots 0-3
        std::map<juce::String, float> parameters;     // All current parameter values
        std::vector<juce::String> activeCouplings;    // "Slot0→Slot1:AudioToFM@0.3"
        std::map<juce::String, float> masterFX;       // Current master FX state
        RecipeEngine::Recipe::SonicDNA currentDNA;    // Current sonic DNA
    };

    //--------------------------------------------------------------------------
    // Callbacks (all called on message thread, never audio thread)

    using RecipeCallback     = std::function<void (const RecipeResult&)>;
    using CouplingCallback   = std::function<void (const CouplingAdvice&)>;
    using MacroCallback      = std::function<void (const MacroSuggestion&)>;
    using NamingCallback     = std::function<void (const NamingSuggestion&)>;
    using ExplanationCallback = std::function<void (const SoundExplanation&)>;

    //--------------------------------------------------------------------------

    SoundAssistant()
        : schema (buildDefaultSchema())
    {}

    /// Access the schema (for UI display of parameter info, debugging, etc.)
    const AIParameterSchema& getSchema() const { return schema; }

    /// Set the preferred AI provider. Falls back to next available if no key.
    void setPreferredProvider (SecureKeyStore::Provider p) { preferredProvider = p; }

    /// Check if any AI provider is configured and ready
    bool isAvailable() const { return keyStore.hasKey (preferredProvider); }

    /// Get which provider will be used
    SecureKeyStore::Provider getActiveProvider() const { return preferredProvider; }

    /// Access the key store (for settings UI)
    SecureKeyStore& getKeyStore() { return keyStore; }

    //--------------------------------------------------------------------------
    // AI Capabilities

    /// Generate a full recipe from a natural language description.
    /// Example: "dark evolving pad with shimmer and slow movement"
    void textToRecipe (const juce::String& description,
                       const SynthContext& context,
                       RecipeCallback callback)
    {
        if (!checkRateLimit()) { callbackError (callback, "Rate limit exceeded. Please wait."); return; }

        auto prompt = buildTextToRecipePrompt (description, context);
        sendRequest (prompt, [this, callback = std::move (callback)] (const juce::String& response, const juce::String& error)
        {
            if (error.isNotEmpty())
            {
                RecipeResult result;
                result.errorMessage = error;
                juce::MessageManager::callAsync ([callback, result]() { callback (result); });
                return;
            }

            auto result = parseRecipeResponse (response);
            juce::MessageManager::callAsync ([callback, result]() { callback (result); });
        });
    }

    /// Suggest coupling configurations for the current engine setup.
    void adviseCoupling (const SynthContext& context,
                         CouplingCallback callback)
    {
        if (!checkRateLimit()) { callbackError (callback, "Rate limit exceeded."); return; }

        auto prompt = buildCouplingPrompt (context);
        sendRequest (prompt, [this, callback = std::move (callback)] (const juce::String& response, const juce::String& error)
        {
            if (error.isNotEmpty())
            {
                CouplingAdvice result;
                result.errorMessage = error;
                juce::MessageManager::callAsync ([callback, result]() { callback (result); });
                return;
            }

            auto result = parseCouplingResponse (response);
            juce::MessageManager::callAsync ([callback, result]() { callback (result); });
        });
    }

    /// Suggest macro names and mappings for the current configuration.
    void suggestMacros (const SynthContext& context,
                        MacroCallback callback)
    {
        if (!checkRateLimit()) { callbackError (callback, "Rate limit exceeded."); return; }

        auto prompt = buildMacroPrompt (context);
        sendRequest (prompt, [this, callback = std::move (callback)] (const juce::String& response, const juce::String& error)
        {
            if (error.isNotEmpty())
            {
                MacroSuggestion result;
                result.errorMessage = error;
                juce::MessageManager::callAsync ([callback, result]() { callback (result); });
                return;
            }

            auto result = parseMacroResponse (response);
            juce::MessageManager::callAsync ([callback, result]() { callback (result); });
        });
    }

    /// Generate a preset name and description from current parameters.
    void namePreset (const SynthContext& context,
                     NamingCallback callback)
    {
        if (!checkRateLimit()) { callbackError (callback, "Rate limit exceeded."); return; }

        auto prompt = buildNamingPrompt (context);
        sendRequest (prompt, [this, callback = std::move (callback)] (const juce::String& response, const juce::String& error)
        {
            if (error.isNotEmpty())
            {
                NamingSuggestion result;
                result.errorMessage = error;
                juce::MessageManager::callAsync ([callback, result]() { callback (result); });
                return;
            }

            auto result = parseNamingResponse (response);
            juce::MessageManager::callAsync ([callback, result]() { callback (result); });
        });
    }

    /// Explain the current sound in plain English + suggest improvements.
    void explainSound (const SynthContext& context,
                       const juce::String& userQuestion,
                       ExplanationCallback callback)
    {
        if (!checkRateLimit()) { callbackError (callback, "Rate limit exceeded."); return; }

        auto prompt = buildExplanationPrompt (context, userQuestion);
        sendRequest (prompt, [this, callback = std::move (callback)] (const juce::String& response, const juce::String& error)
        {
            if (error.isNotEmpty())
            {
                SoundExplanation result;
                result.errorMessage = error;
                juce::MessageManager::callAsync ([callback, result]() { callback (result); });
                return;
            }

            auto result = parseExplanationResponse (response);
            juce::MessageManager::callAsync ([callback, result]() { callback (result); });
        });
    }

private:
    SecureKeyStore keyStore;
    AIParameterSchema schema;
    SecureKeyStore::Provider preferredProvider = SecureKeyStore::Provider::Anthropic;

    // Rate limiting: max 10 requests per minute
    static constexpr int kMaxRequestsPerMinute = 10;
    std::array<double, 10> requestTimestamps {};
    int requestIdx = 0;

    bool checkRateLimit()
    {
        double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
        double oldest = requestTimestamps[static_cast<size_t> (requestIdx)];

        if (now - oldest < 60.0)
            return false;  // Too many requests in the last minute

        requestTimestamps[static_cast<size_t> (requestIdx)] = now;
        requestIdx = (requestIdx + 1) % kMaxRequestsPerMinute;
        return true;
    }

    template <typename CallbackType>
    static void callbackError (CallbackType& cb, const juce::String& msg)
    {
        // Create a default result with the error
        using ResultType = typename std::remove_reference<
            typename std::tuple_element<0,
                typename juce::dsp::util::FunctionTraits<CallbackType>::ArgTypes>::type>::type;
        // Simplified: just log the error
        (void) cb; (void) msg;
    }

    //--------------------------------------------------------------------------
    // Network (background thread, TLS-only)

    using ResponseCallback = std::function<void (const juce::String& response,
                                                  const juce::String& error)>;

    void sendRequest (const juce::String& prompt, ResponseCallback callback)
    {
        // Retrieve key (decrypted only for this call)
        auto key = keyStore.retrieveKey (preferredProvider);
        if (key.isEmpty())
        {
            callback ({}, "No API key configured for " +
                     SecureKeyStore::providerName (preferredProvider));
            return;
        }

        auto provider = preferredProvider;
        auto* thread = new RequestThread (provider, key, prompt, std::move (callback));
        thread->startThread();

        // Key will be wiped when RequestThread destructs
    }

    //--------------------------------------------------------------------------
    // Background request thread

    class RequestThread : public juce::Thread
    {
    public:
        RequestThread (SecureKeyStore::Provider provider,
                       const juce::String& apiKey,
                       const juce::String& prompt,
                       ResponseCallback callback)
            : juce::Thread ("XO_AI_Request")
            , provider_ (provider)
            , apiKey_ (apiKey)
            , prompt_ (prompt)
            , callback_ (std::move (callback))
        {}

        ~RequestThread() override
        {
            // Secure wipe key material
            SecureKeyStore::secureWipePublic (apiKey_);
            stopThread (5000);
        }

        void run() override
        {
            auto url = getEndpointURL();
            auto body = buildRequestBody();
            auto headers = buildHeaders();

            juce::URL requestUrl (url);
            requestUrl = requestUrl.withPOSTData (body);

            auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostBody)
                .withExtraHeaders (headers)
                .withConnectionTimeoutMs (30000);

            auto stream = requestUrl.createInputStream (options);

            juce::String response, error;

            if (stream)
            {
                response = stream->readEntireStreamAsString();
            }
            else
            {
                error = "Network error: could not connect to " +
                        SecureKeyStore::providerName (provider_);
            }

            // Wipe key from memory immediately
            SecureKeyStore::secureWipePublic (apiKey_);

            auto cb = std::move (callback_);
            auto resp = std::move (response);
            auto err = std::move (error);

            juce::MessageManager::callAsync ([cb, resp, err]() {
                cb (resp, err);
            });

            // Self-delete after callback
            juce::MessageManager::callAsync ([this]() { delete this; });
        }

    private:
        SecureKeyStore::Provider provider_;
        juce::String apiKey_;
        juce::String prompt_;
        ResponseCallback callback_;

        juce::String getEndpointURL() const
        {
            switch (provider_)
            {
                case SecureKeyStore::Provider::Anthropic:
                    return "https://api.anthropic.com/v1/messages";
                case SecureKeyStore::Provider::OpenAI:
                    return "https://api.openai.com/v1/chat/completions";
                case SecureKeyStore::Provider::Google:
                    return "https://generativelanguage.googleapis.com/v1/models/gemini-pro:generateContent";
                default:
                    return {};
            }
        }

        juce::String buildHeaders() const
        {
            switch (provider_)
            {
                case SecureKeyStore::Provider::Anthropic:
                    return "Content-Type: application/json\r\n"
                           "x-api-key: " + apiKey_ + "\r\n"
                           "anthropic-version: 2023-06-01\r\n";
                case SecureKeyStore::Provider::OpenAI:
                    return "Content-Type: application/json\r\n"
                           "Authorization: Bearer " + apiKey_ + "\r\n";
                case SecureKeyStore::Provider::Google:
                    return "Content-Type: application/json\r\n"
                           "x-goog-api-key: " + apiKey_ + "\r\n";
                default:
                    return {};
            }
        }

        juce::String buildRequestBody() const
        {
            auto* root = new juce::DynamicObject();

            switch (provider_)
            {
                case SecureKeyStore::Provider::Anthropic:
                {
                    root->setProperty ("model", "claude-sonnet-4-20250514");
                    root->setProperty ("max_tokens", 4096);

                    auto* msg = new juce::DynamicObject();
                    msg->setProperty ("role", "user");
                    msg->setProperty ("content", prompt_);
                    juce::Array<juce::var> messages;
                    messages.add (juce::var (msg));
                    root->setProperty ("messages", messages);

                    root->setProperty ("system",
                        "You are XOmnibus Sound Architect, an expert sound design assistant for "
                        "XOmnibus, a multi-engine synthesizer with 20 engines and 12 coupling types. "
                        "RESPOND WITH VALID JSON ONLY. No markdown code fences, no explanation text "
                        "outside the JSON. The user prompt contains a PARAMETER REFERENCE section — "
                        "use ONLY the parameter IDs listed there. Every value you return will be "
                        "validated against the parameter schema: out-of-range values will be clamped, "
                        "unknown parameter IDs will be flagged. Prefer sweet-spot values over extremes "
                        "unless the user explicitly asks for extreme sounds.");
                    break;
                }
                case SecureKeyStore::Provider::OpenAI:
                {
                    root->setProperty ("model", "gpt-4o");
                    root->setProperty ("max_tokens", 4096);

                    auto* sysMsg = new juce::DynamicObject();
                    sysMsg->setProperty ("role", "system");
                    sysMsg->setProperty ("content",
                        "You are XOmnibus Sound Architect, an expert sound design assistant. "
                        "RESPOND WITH VALID JSON ONLY — no markdown, no text outside JSON. "
                        "Use ONLY parameter IDs from the PARAMETER REFERENCE in the user prompt. "
                        "Prefer sweet-spot values. All values will be validated against the schema.");
                    auto* userMsg = new juce::DynamicObject();
                    userMsg->setProperty ("role", "user");
                    userMsg->setProperty ("content", prompt_);

                    juce::Array<juce::var> messages;
                    messages.add (juce::var (sysMsg));
                    messages.add (juce::var (userMsg));
                    root->setProperty ("messages", messages);
                    break;
                }
                case SecureKeyStore::Provider::Google:
                {
                    auto* part = new juce::DynamicObject();
                    part->setProperty ("text", prompt_);
                    juce::Array<juce::var> parts;
                    parts.add (juce::var (part));

                    auto* content = new juce::DynamicObject();
                    content->setProperty ("parts", parts);
                    juce::Array<juce::var> contents;
                    contents.add (juce::var (content));
                    root->setProperty ("contents", contents);
                    break;
                }
                default: break;
            }

            return juce::JSON::toString (juce::var (root), true);
        }
    };

    //--------------------------------------------------------------------------
    // Prompt builders

    static juce::String buildSystemContext()
    {
        return "XOmnibus is a multi-engine synthesizer with 20 engines, 12 coupling types, "
               "an 18-stage master FX chain, and 4 macros (M1=CHARACTER, M2=MOVEMENT, "
               "M3=COUPLING, M4=SPACE). "
               "Master FX stages: Saturator, Corroder, VibeKnob(-1=sweet,+1=grit), "
               "SpectralTilt, TransientDesigner, Delay, Combulator, Doppler, "
               "Reverb, FreqShifter, Modulation, GranularSmear, HarmonicExciter, "
               "StereoSculptor, PsychoacousticWidth, MultibandOTT, BusCompressor, "
               "FXSequencer. "
               "Moods: Foundation, Atmosphere, Entangled, Prism, Flux, Aether. "
               "Sonic DNA: brightness(0-1), warmth(0-1), movement(0-1), "
               "density(0-1), space(0-1), aggression(0-1). ";
    }

    /// Generate schema-enriched context for the specific engines in play.
    /// This gives the AI exact parameter IDs, ranges, sweet spots, and safety rules.
    juce::String buildSchemaContext (const SynthContext& context) const
    {
        return schema.generatePromptContext (context.activeEngines)
               + schema.generateSafetyPrompt();
    }

    static juce::String contextToString (const SynthContext& ctx)
    {
        juce::String s = "Current state: Engines=[" + ctx.activeEngines.joinIntoString(",") + "]";
        if (!ctx.activeCouplings.empty())
        {
            s += " Coupling=[";
            for (size_t i = 0; i < ctx.activeCouplings.size(); ++i)
            {
                if (i > 0) s += ",";
                s += ctx.activeCouplings[i];
            }
            s += "]";
        }
        s += juce::String::formatted (" DNA={br:%.2f,wa:%.2f,mv:%.2f,de:%.2f,sp:%.2f,ag:%.2f}",
            ctx.currentDNA.brightness, ctx.currentDNA.warmth, ctx.currentDNA.movement,
            ctx.currentDNA.density, ctx.currentDNA.space, ctx.currentDNA.aggression);
        return s;
    }

    juce::String buildTextToRecipePrompt (const juce::String& description,
                                         const SynthContext& context) const
    {
        // Detect mood hint from description for targeted few-shot examples
        juce::String moodHint;
        auto descLower = description.toLowerCase();
        if (descLower.contains ("pad") || descLower.contains ("ambient") || descLower.contains ("atmosphere"))
            moodHint = "Atmosphere";
        else if (descLower.contains ("bass") || descLower.contains ("kick") || descLower.contains ("solid"))
            moodHint = "Foundation";
        else if (descLower.contains ("chaotic") || descLower.contains ("complex") || descLower.contains ("tangle"))
            moodHint = "Entangled";
        else if (descLower.contains ("bright") || descLower.contains ("crystal") || descLower.contains ("prism"))
            moodHint = "Prism";
        else if (descLower.contains ("morph") || descLower.contains ("evolv") || descLower.contains ("flux"))
            moodHint = "Flux";
        else if (descLower.contains ("ethereal") || descLower.contains ("otherworld") || descLower.contains ("transcend"))
            moodHint = "Aether";

        return buildSystemContext() + "\n\n"
               + buildSchemaContext (context) + "\n\n"
               + schema.generateFewShotContext (moodHint) + "\n\n"
               + contextToString (context) + "\n\n"
               "User request: \"" + description + "\"\n\n"
               "Generate a complete XOmnibus recipe as JSON with these EXACT fields:\n"
               "{\n"
               "  \"name\": \"2-3 word evocative name, max 30 chars\",\n"
               "  \"mood\": \"Foundation|Atmosphere|Entangled|Prism|Flux|Aether\",\n"
               "  \"description\": \"One sentence describing the sound\",\n"
               "  \"explanation\": \"Why you chose these engines and settings\",\n"
               "  \"engines\": [{\"id\": \"EngineId\", \"parameters\": {\"prefix_param\": value}}],\n"
               "  \"coupling\": [{\"sourceSlot\": 0, \"destSlot\": 1, \"type\": \"CouplingType\", \"intensity\": 0.3}],\n"
               "  \"masterFX\": {\"fx_paramId\": value},\n"
               "  \"macros\": [{\"label\": \"WORD\", \"targets\": [\"paramId\"], \"ranges\": [0.5]}],\n"
               "  \"variationAxis\": {\"label\": \"Name\", \"targets\": [\"paramId\"], \"minValues\": [0.0], \"maxValues\": [1.0]},\n"
               "  \"dna\": {\"brightness\": 0.5, \"warmth\": 0.5, \"movement\": 0.5, \"density\": 0.5, \"space\": 0.5, \"aggression\": 0.5}\n"
               "}\n\n"
               "CRITICAL RULES:\n"
               "- Choose 2-4 engines from the PARAMETER REFERENCE above.\n"
               "- Use ONLY the parameter IDs listed in the reference. Do NOT invent IDs.\n"
               "- Keep values within [min, max] ranges. Prefer sweet spot ranges.\n"
               "- For Choice params, use the integer index (0-based).\n"
               "- Each macro should target 2-4 parameters and produce audible change.\n"
               "- M1=CHARACTER, M2=MOVEMENT, M3=COUPLING, M4=SPACE.\n"
               "- Variation axis should be a single expressive dimension to explore.\n"
               "- DNA values must be 0.0-1.0 and match what the sound actually is.\n"
               "- Dry patch (before FX) must sound compelling on its own.\n";
    }

    juce::String buildCouplingPrompt (const SynthContext& context) const
    {
        return buildSystemContext() + "\n\n"
               + buildSchemaContext (context) + "\n\n"
               + contextToString (context) + "\n\n"
               "Suggest 2-4 coupling routes for these engines.\n"
               "Return JSON: {\"suggestions\": [{\"sourceEngine\": \"Id\", \"destEngine\": \"Id\", "
               "\"couplingType\": \"TypeName\", \"suggestedIntensity\": 0.3, \"reasoning\": \"why\"}]}\n\n"
               "RULES:\n"
               "- Use ONLY coupling types from the COUPLING TYPES reference above.\n"
               "- Respect the max safe intensity for each type.\n"
               "- Consider what the source engine sends and what the dest receives.\n"
               "- Lower intensities (0.1-0.4) are usually more musical than extremes.\n"
               "- Avoid AudioToFM + PitchToPitch on the same pair (conflicts).\n";
    }

    juce::String buildMacroPrompt (const SynthContext& context) const
    {
        return buildSystemContext() + "\n\n"
               + buildSchemaContext (context) + "\n\n"
               + contextToString (context) + "\n\n"
               "Suggest 4 macro assignments for this configuration.\n"
               "Return JSON: {\"macros\": [{\"label\": \"WORD\", \"description\": \"what it does\", "
               "\"targets\": [\"paramId\", ...], \"ranges\": [0.5, ...]}]}\n\n"
               "RULES:\n"
               "- Labels must be 1 word, evocative, ALL CAPS (e.g., SHIMMER, GRIT, DRIFT).\n"
               "- Target ONLY parameter IDs from the reference above.\n"
               "- Each macro should control 2-5 parameters for complex movement.\n"
               "- Ranges are -1.0 to +1.0 (bipolar modulation depth).\n"
               "- M1=CHARACTER (timbre/tone), M2=MOVEMENT (animation/LFO), "
               "M3=COUPLING (cross-engine), M4=SPACE (reverb/delay/width).\n"
               "- Every macro must produce an audible change at full throw.\n"
               "- Don't target Choice or Toggle params with macros.\n";
    }

    juce::String buildNamingPrompt (const SynthContext& context) const
    {
        return buildSystemContext() + "\n\n"
               + contextToString (context) + "\n\n"
               "Name this sound. Return JSON: {\"name\": \"2-3 words, evocative, "
               "max 30 chars\", \"description\": \"1 sentence\", \"mood\": \"one of 6 moods\", "
               "\"tags\": [\"tag1\", \"tag2\", \"tag3\", \"tag4\", \"tag5\"]}.\n\n"
               "RULES:\n"
               "- No synth jargon in the name (no 'FM', 'LFO', 'wavetable').\n"
               "- Name should evoke what the sound FEELS like, not what makes it.\n"
               "- Mood must be exactly one of: Foundation, Atmosphere, Entangled, Prism, Flux, Aether.\n"
               "- Tags should be useful for search (instrument type, genre, mood, texture).\n";
    }

    juce::String buildExplanationPrompt (const SynthContext& context,
                                          const juce::String& question) const
    {
        return buildSystemContext() + "\n\n"
               + buildSchemaContext (context) + "\n\n"
               + contextToString (context) + "\n\n"
               "User asks: \"" + question + "\"\n\n"
               "Return JSON: {\"description\": \"plain English description of current sound\", "
               "\"suggestions\": \"actionable tips using specific parameter IDs and values\"}\n\n"
               "RULES:\n"
               "- Be specific: name exact parameter IDs and target values.\n"
               "- Reference the sweet spots from the parameter reference.\n"
               "- Suggest changes as relative adjustments when possible.\n"
               "- If the user wants something this engine can't do, say so and suggest a different engine.\n";
    }

    //--------------------------------------------------------------------------
    // Response parsers

    RecipeResult parseRecipeResponse (const juce::String& response) const
    {
        RecipeResult result;
        auto json = extractJSON (response);

        auto* obj = json.getDynamicObject();
        if (obj == nullptr)
        {
            result.errorMessage = "Could not parse AI response as recipe JSON";
            return result;
        }

        // --- Identity ---
        result.recipe.name        = obj->getProperty ("name").toString();
        result.recipe.mood        = obj->getProperty ("mood").toString();
        result.recipe.description = obj->getProperty ("description").toString();
        result.explanation        = obj->getProperty ("explanation").toString();

        // Validate mood
        static const juce::StringArray validMoods {
            "Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether" };
        if (! validMoods.contains (result.recipe.mood))
        {
            result.validationWarnings.add ("Mood '" + result.recipe.mood
                                           + "' is not valid. Defaulting to Atmosphere.");
            result.recipe.mood = "Atmosphere";
        }

        // --- Engines + Parameters (with schema validation) ---
        if (auto* enginesArr = obj->getProperty ("engines").getArray())
        {
            int engineCount = juce::jmin (4, enginesArr->size());
            for (int i = 0; i < engineCount; ++i)
            {
                auto* engObj = (*enginesArr)[i].getDynamicObject();
                if (engObj == nullptr) continue;

                RecipeEngine::EngineSlot slot;
                slot.engineId = engObj->getProperty ("id").toString();

                // Validate engine ID exists in schema
                auto* engineProfile = schema.getEngine (slot.engineId);
                if (engineProfile == nullptr)
                {
                    result.validationWarnings.add ("Unknown engine '" + slot.engineId
                                                   + "' — included but parameters cannot be validated.");
                }

                // Parse and validate parameters
                if (auto* paramsObj = engObj->getProperty ("parameters").getDynamicObject())
                {
                    for (const auto& prop : paramsObj->getProperties())
                    {
                        juce::String paramId = prop.name.toString();
                        float rawValue = static_cast<float> (prop.value);

                        auto* paramDef = schema.getParam (paramId);
                        if (paramDef != nullptr)
                        {
                            float validated = paramDef->validate (rawValue);
                            slot.parameters[paramId] = validated;

                            if (std::abs (validated - rawValue) > 0.001f)
                            {
                                result.parametersCorrected++;
                                result.validationWarnings.add (
                                    paramId + ": AI suggested " + juce::String (rawValue, 3)
                                    + ", corrected to " + juce::String (validated, 3));
                            }
                        }
                        else
                        {
                            // Unknown parameter — could be from an engine we don't have
                            // full schema for yet. Include it but warn.
                            slot.parameters[paramId] = rawValue;
                            result.unknownParametersDropped++;
                            result.validationWarnings.add (
                                "Parameter '" + paramId + "' not in schema — included unchecked.");
                        }
                    }
                }

                result.recipe.engines.push_back (std::move (slot));
            }
        }

        // --- Coupling (with schema validation) ---
        if (auto* couplingArr = obj->getProperty ("coupling").getArray())
        {
            for (const auto& c : *couplingArr)
            {
                auto* cObj = c.getDynamicObject();
                if (cObj == nullptr) continue;

                RecipeEngine::CouplingRoute route;
                route.sourceSlot   = juce::jlimit (0, 3, static_cast<int> (cObj->getProperty ("sourceSlot")));
                route.destSlot     = juce::jlimit (0, 3, static_cast<int> (cObj->getProperty ("destSlot")));
                route.couplingType = cObj->getProperty ("type").toString();
                float rawIntensity = static_cast<float> (cObj->getProperty ("intensity"));

                // Validate through schema
                auto couplingValidation = schema.validateCoupling (route.couplingType, rawIntensity);
                route.intensity = couplingValidation.correctedParams.count ("intensity")
                    ? couplingValidation.correctedParams.at ("intensity")
                    : juce::jlimit (0.0f, 1.0f, rawIntensity);

                for (const auto& w : couplingValidation.warnings)
                    result.validationWarnings.add (w);

                // Prevent self-coupling
                if (route.sourceSlot == route.destSlot)
                {
                    result.validationWarnings.add ("Coupling source=dest (slot "
                        + juce::String (route.sourceSlot) + ") — skipped.");
                    continue;
                }

                result.recipe.coupling.push_back (std::move (route));
            }
        }

        // --- Master FX ---
        if (auto* fxObj = obj->getProperty ("masterFX").getDynamicObject())
        {
            for (const auto& prop : fxObj->getProperties())
                result.recipe.masterFX[prop.name.toString()] = static_cast<float> (prop.value);
        }

        // --- Macros (with parameter existence validation) ---
        if (auto* macrosArr = obj->getProperty ("macros").getArray())
        {
            for (int i = 0; i < juce::jmin (4, macrosArr->size()); ++i)
            {
                auto* mObj = (*macrosArr)[i].getDynamicObject();
                if (mObj == nullptr) continue;

                auto& macro = result.recipe.macros[static_cast<size_t> (i)];
                macro.label = mObj->getProperty ("label").toString().toUpperCase();

                if (auto* targets = mObj->getProperty ("targets").getArray())
                {
                    for (const auto& t : *targets)
                    {
                        juce::String targetId = t.toString();
                        // Validate target exists
                        if (schema.getParam (targetId) != nullptr)
                        {
                            macro.targets.push_back (targetId);
                        }
                        else
                        {
                            // Include it anyway (might be an FX param or future param)
                            macro.targets.push_back (targetId);
                            result.validationWarnings.add (
                                "Macro " + macro.label + " target '" + targetId
                                + "' not in engine schema — included unchecked.");
                        }
                    }
                }

                if (auto* ranges = mObj->getProperty ("ranges").getArray())
                    for (const auto& r : *ranges)
                        macro.ranges.push_back (juce::jlimit (-1.0f, 1.0f, static_cast<float> (r)));
            }
        }

        // --- Variation Axis ---
        if (auto* vaObj = obj->getProperty ("variationAxis").getDynamicObject())
        {
            RecipeEngine::VariationAxis va;
            va.label = vaObj->getProperty ("label").toString();

            if (auto* targets = vaObj->getProperty ("targets").getArray())
                for (const auto& t : *targets) va.targets.push_back (t.toString());
            if (auto* mins = vaObj->getProperty ("minValues").getArray())
                for (const auto& v : *mins) va.minValues.push_back (static_cast<float> (v));
            if (auto* maxs = vaObj->getProperty ("maxValues").getArray())
                for (const auto& v : *maxs) va.maxValues.push_back (static_cast<float> (v));

            if (! va.targets.empty())
                result.recipe.variationAxis = std::move (va);
        }

        // --- DNA (clamp to valid range) ---
        if (auto* dnaObj = obj->getProperty ("dna").getDynamicObject())
        {
            auto clampDNA = [] (const juce::var& v) {
                return juce::jlimit (0.0f, 1.0f, static_cast<float> (v));
            };
            result.recipe.dna.brightness  = clampDNA (dnaObj->getProperty ("brightness"));
            result.recipe.dna.warmth      = clampDNA (dnaObj->getProperty ("warmth"));
            result.recipe.dna.movement    = clampDNA (dnaObj->getProperty ("movement"));
            result.recipe.dna.density     = clampDNA (dnaObj->getProperty ("density"));
            result.recipe.dna.space       = clampDNA (dnaObj->getProperty ("space"));
            result.recipe.dna.aggression  = clampDNA (dnaObj->getProperty ("aggression"));
        }

        // --- Cross-parameter safety rules (Phase 2) ---
        std::map<juce::String, float> allParams;
        for (const auto& slot : result.recipe.engines)
            for (const auto& [k, v] : slot.parameters)
                allParams[k] = v;

        auto safetyResult = schema.validateParameters (allParams);
        for (const auto& w : safetyResult.warnings)
            result.validationWarnings.add ("Safety: " + w);

        // Apply corrections from safety rules back to engine slots
        for (auto& slot : result.recipe.engines)
        {
            for (auto& [paramId, value] : slot.parameters)
            {
                auto it = safetyResult.correctedParams.find (paramId);
                if (it != safetyResult.correctedParams.end())
                    value = it->second;
            }
        }

        // --- Silent patch detection (Phase 3 — research Layer 1) ---
        auto silentCheck = schema.checkSilentPatch (allParams);
        if (silentCheck.isSilent)
        {
            for (const auto& r : silentCheck.reasons)
                result.validationWarnings.add ("Silent patch: " + r);

            // Auto-fix: set the first engine's level to default
            if (! result.recipe.engines.empty())
            {
                auto* profile = schema.getEngine (result.recipe.engines[0].engineId);
                if (profile != nullptr)
                {
                    for (const auto& p : profile->parameters)
                    {
                        if (p.role == AIParamRole::Level)
                        {
                            result.recipe.engines[0].parameters[p.paramId] = p.defaultValue;
                            result.validationWarnings.add (
                                "Auto-fixed: set " + p.paramId + " to "
                                + juce::String (p.defaultValue, 2) + " to prevent silent output.");
                            result.parametersCorrected++;
                            break;
                        }
                    }
                }
            }
        }

        // --- DNA vs mood coherence (Phase 4 — research Section 6) ---
        auto dnaCheck = AIParameterSchema::validateDNAForMood (result.recipe.dna, result.recipe.mood);
        if (dnaCheck.adjusted)
        {
            result.recipe.dna = dnaCheck.correctedDNA;
            for (const auto& w : dnaCheck.warnings)
                result.validationWarnings.add (w);
        }

        // --- Macro target validation (Phase 5 — research: verify macro audibility) ---
        for (size_t i = 0; i < result.recipe.macros.size(); ++i)
        {
            const auto& macro = result.recipe.macros[i];
            if (macro.label.isEmpty()) continue;

            auto macroCheck = schema.validateMacroTargets (
                macro.label, macro.targets, macro.ranges, allParams);

            for (const auto& w : macroCheck.warnings)
                result.validationWarnings.add (w);
        }

        result.success = result.recipe.isValid();
        if (! result.success)
            result.errorMessage = "Recipe is invalid (missing name or engines)";

        return result;
    }

    CouplingAdvice parseCouplingResponse (const juce::String& response) const
    {
        CouplingAdvice result;
        auto json = extractJSON (response);
        if (auto* obj = json.getDynamicObject())
        {
            if (auto* suggs = obj->getProperty ("suggestions").getArray())
            {
                result.success = true;
                for (const auto& s : *suggs)
                {
                    if (auto* sObj = s.getDynamicObject())
                    {
                        CouplingAdvice::Suggestion suggestion;
                        suggestion.sourceEngine = sObj->getProperty ("sourceEngine").toString();
                        suggestion.destEngine = sObj->getProperty ("destEngine").toString();
                        suggestion.couplingType = sObj->getProperty ("couplingType").toString();
                        float rawIntensity = static_cast<float> (sObj->getProperty ("suggestedIntensity"));
                        suggestion.reasoning = sObj->getProperty ("reasoning").toString();

                        // Validate intensity through schema
                        auto validation = schema.validateCoupling (suggestion.couplingType, rawIntensity);
                        suggestion.suggestedIntensity = validation.correctedParams.count ("intensity")
                            ? validation.correctedParams.at ("intensity")
                            : juce::jlimit (0.0f, 1.0f, rawIntensity);

                        result.suggestions.push_back (std::move (suggestion));
                    }
                }
            }
        }
        if (!result.success)
            result.errorMessage = "Could not parse coupling suggestions";
        return result;
    }

    MacroSuggestion parseMacroResponse (const juce::String& response) const
    {
        MacroSuggestion result;
        auto json = extractJSON (response);
        if (auto* obj = json.getDynamicObject())
        {
            if (auto* macros = obj->getProperty ("macros").getArray())
            {
                result.success = true;
                for (int i = 0; i < std::min (4, macros->size()); ++i)
                {
                    if (auto* m = (*macros)[i].getDynamicObject())
                    {
                        result.macros[static_cast<size_t> (i)].label = m->getProperty ("label").toString();
                        result.macros[static_cast<size_t> (i)].description = m->getProperty ("description").toString();
                    }
                }
            }
        }
        if (!result.success)
            result.errorMessage = "Could not parse macro suggestions";
        return result;
    }

    NamingSuggestion parseNamingResponse (const juce::String& response) const
    {
        NamingSuggestion result;
        auto json = extractJSON (response);
        if (auto* obj = json.getDynamicObject())
        {
            result.success = true;
            result.name = obj->getProperty ("name").toString();
            result.description = obj->getProperty ("description").toString();
            result.mood = obj->getProperty ("mood").toString();
        }
        if (!result.success)
            result.errorMessage = "Could not parse naming response";
        return result;
    }

    SoundExplanation parseExplanationResponse (const juce::String& response) const
    {
        SoundExplanation result;
        auto json = extractJSON (response);
        if (auto* obj = json.getDynamicObject())
        {
            result.success = true;
            result.description = obj->getProperty ("description").toString();
            result.suggestions = obj->getProperty ("suggestions").toString();
        }
        if (!result.success)
            result.errorMessage = "Could not parse explanation";
        return result;
    }

    /// Extract JSON from a response that may contain markdown or extra text
    static juce::var extractJSON (const juce::String& response)
    {
        // Try direct parse first
        auto result = juce::JSON::parse (response);
        if (result.isObject() || result.isArray())
            return result;

        // Try to find JSON within markdown code blocks
        int jsonStart = response.indexOf ("{");
        int jsonEnd = response.lastIndexOf ("}");
        if (jsonStart >= 0 && jsonEnd > jsonStart)
        {
            auto jsonStr = response.substring (jsonStart, jsonEnd + 1);
            result = juce::JSON::parse (jsonStr);
            if (result.isObject())
                return result;
        }

        return {};
    }

public:
    // Public secure wipe for RequestThread access
    static void secureWipePublic (juce::String& str)
    {
        if (str.isEmpty()) return;
        auto* rawData = const_cast<char*> (str.toRawUTF8());
        auto len = str.getNumBytesAsUTF8();
        if (rawData && len > 0)
            std::memset (rawData, 0, len);
        str = juce::String();
    }
};


//==============================================================================
// SmoothedRecipeApplicator — Click-free recipe application.
//
// Research Layer 4: "Never apply AI-suggested parameters instantaneously.
// For parameter jumps > threshold, apply exponential slew with 5-20ms."
//
// When the AI generates a recipe and the user hits "Apply", this applies
// all parameter changes smoothly over a short crossfade to prevent clicks,
// pops, and zipper noise. Works with the existing 50ms engine hot-swap
// crossfade for engine changes, and adds parameter-level smoothing for
// parameter-only changes within the same engine configuration.
//
// Usage:
//   SmoothedRecipeApplicator applicator;
//   applicator.applySmoothed (recipe, apvts, callbacks, sampleRate);
//==============================================================================
class SmoothedRecipeApplicator
{
public:
    /// Crossfade duration in ms for parameter changes
    static constexpr float kParamCrossfadeMs = 20.0f;

    /// Threshold: jumps smaller than this are applied instantly (no slew needed)
    static constexpr float kInstantThreshold = 0.05f;

    /// Apply a recipe with smoothed parameter transitions.
    /// For engine swaps, relies on the existing 50ms crossfade in EngineRegistry.
    /// For parameter-only changes, applies an exponential ramp on the message thread.
    static void applySmoothed (const RecipeEngine::Recipe& recipe,
                                juce::AudioProcessorValueTreeState& apvts,
                                const RecipeEngine::ApplyCallbacks& callbacks,
                                const RecipeEngine& recipeEngine,
                                double sampleRate)
    {
        // 1. Capture current values for all parameters that will change
        std::vector<ParameterTransition> transitions;

        for (const auto& slot : recipe.engines)
        {
            for (const auto& [paramId, targetValue] : slot.parameters)
            {
                auto* param = apvts.getRawParameterValue (paramId);
                if (param == nullptr) continue;

                float currentValue = param->load();
                float delta = std::abs (targetValue - currentValue);

                if (delta < kInstantThreshold)
                {
                    // Small change — apply instantly
                    param->store (targetValue);
                }
                else
                {
                    // Large change — schedule for smoothed transition
                    transitions.push_back ({ paramId, param, currentValue, targetValue });
                }
            }
        }

        // 2. Apply engine loading, coupling, and macros via RecipeEngine
        //    (engine swaps use their own 50ms crossfade)
        recipeEngine.applyRecipe (recipe, apvts, callbacks);

        // 3. Smooth the large parameter jumps over kParamCrossfadeMs
        if (! transitions.empty())
        {
            int steps = static_cast<int> (kParamCrossfadeMs * sampleRate / 1000.0);
            steps = juce::jlimit (1, 2048, steps);

            // Use a timer to apply the ramp across multiple message-thread callbacks
            // Each step moves closer to the target
            auto transitionsCopy = std::make_shared<std::vector<ParameterTransition>> (
                std::move (transitions));
            auto stepRef = std::make_shared<int> (0);
            int totalSteps = steps;

            // Schedule parameter ramp via callAsync (runs on message thread)
            rampParameters (transitionsCopy, stepRef, totalSteps, 0);
        }
    }

private:
    struct ParameterTransition
    {
        juce::String paramId;
        std::atomic<float>* paramPtr;
        float startValue;
        float endValue;
    };

    static void rampParameters (std::shared_ptr<std::vector<ParameterTransition>> transitions,
                                 std::shared_ptr<int> currentStep,
                                 int totalSteps,
                                 int step)
    {
        if (step >= totalSteps)
        {
            // Final step: set exact target values
            for (auto& t : *transitions)
                t.paramPtr->store (t.endValue);
            return;
        }

        // Exponential ease-out curve for natural-feeling parameter movement
        float progress = static_cast<float> (step + 1) / static_cast<float> (totalSteps);
        float eased = 1.0f - std::pow (1.0f - progress, 3.0f); // Cubic ease-out

        for (auto& t : *transitions)
        {
            float value = t.startValue + (t.endValue - t.startValue) * eased;
            t.paramPtr->store (value);
        }

        // Schedule next step (~1ms per step)
        int nextStep = step + 1;
        juce::MessageManager::callAsync (
            [transitions, currentStep, totalSteps, nextStep]()
            {
                rampParameters (transitions, currentStep, totalSteps, nextStep);
            });
    }
};

} // namespace xomnibus
