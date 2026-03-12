#pragma once
#include "../Core/RecipeEngine.h"
#include <juce_core/juce_core.h>
#include <map>
#include <vector>
#include <cmath>

namespace xomnibus {

//==============================================================================
// NaturalLanguageInterpreter — Maps creative, abstract, and plain language
// descriptions to Sonic DNA adjustments and parameter intent.
//
// This is the bridge between how people THINK about sound and how XOmnibus
// represents it. It handles three tiers of input:
//
//   Tier 1 — Technical: "reduce filter resonance on engine 2, increase FM depth"
//            → Direct parameter mapping (pass through to AI as-is)
//
//   Tier 2 — Musical: "make it warmer", "more aggressive", "add shimmer"
//            → DNA delta adjustments + engine/FX hints
//
//   Tier 3 — Abstract: "make it sound like the color blue", "sounds like rain
//            on a tin roof", "underwater cathedral"
//            → Synesthetic mapping to DNA + mood + texture descriptors
//
// Design:
//   - Stateless utility class — all methods are static or const
//   - No allocations per call beyond the return struct
//   - Deterministic: same input always produces same DNA delta
//   - The AI still generates the actual parameters — this just pre-processes
//     the user's intent into structured guidance the AI can work with
//
// Research basis:
//   - Synesthetic mappings from audio-visual crossmodal studies
//   - Bouba/Kiki effect (rounded sounds ↔ warm words, angular ↔ aggressive)
//   - Music Information Retrieval (MIR) mood/valence/arousal models
//   - LLM2Fx (Sony): natural language → effect chain parameters
//==============================================================================
class NaturalLanguageInterpreter
{
public:
    //--------------------------------------------------------------------------
    // Expertise level detection

    enum class ExpertiseLevel
    {
        Novice,      // "make it sound cool", "more vibey"
        Intermediate, // "add reverb", "make the bass deeper"
        Expert        // "reduce Q on the lowpass to 0.3", "increase FM index"
    };

    //--------------------------------------------------------------------------
    // Interpretation result

    struct InterpretedIntent
    {
        // Detected expertise level
        ExpertiseLevel expertise = ExpertiseLevel::Intermediate;

        // DNA delta: how each dimension should shift (-1 to +1, 0 = no change)
        struct DNADelta
        {
            float brightness = 0.0f;
            float warmth     = 0.0f;
            float movement   = 0.0f;
            float density    = 0.0f;
            float space      = 0.0f;
            float aggression = 0.0f;

            bool isZero() const
            {
                return std::abs (brightness) < 0.01f && std::abs (warmth) < 0.01f
                    && std::abs (movement) < 0.01f && std::abs (density) < 0.01f
                    && std::abs (space) < 0.01f && std::abs (aggression) < 0.01f;
            }
        };
        DNADelta dnaDelta;

        // Suggested mood (empty if no clear mapping)
        juce::String suggestedMood;

        // Texture/character keywords extracted for the AI prompt
        juce::StringArray textureHints;   // e.g. "glassy", "granular", "pad-like"

        // Engine hints (empty = let AI decide)
        juce::StringArray suggestedEngines;

        // FX hints (empty = let AI decide)
        juce::StringArray suggestedFX;

        // Whether the input contained direct parameter references
        bool hasTechnicalContent = false;

        // The original input, cleaned and normalized
        juce::String normalizedInput;

        // Rewritten prompt optimized for the AI (enriched with DNA guidance)
        juce::String enrichedPrompt;

        // Confidence in the interpretation (0-1)
        float confidence = 0.5f;
    };

    //--------------------------------------------------------------------------
    // Refinement direction — for iterative "make it more X" requests

    struct RefinementDirection
    {
        juce::String dimension;  // Which aspect to change
        float magnitude = 0.3f;  // How much (-1 to +1, negative = less)
        juce::String explanation; // Human-readable interpretation
    };

    //--------------------------------------------------------------------------
    // Core methods

    /// Interpret a natural language description into structured intent.
    /// This is the main entry point — handles all three tiers.
    static InterpretedIntent interpret (const juce::String& userInput)
    {
        InterpretedIntent result;
        auto input = userInput.trim();
        result.normalizedInput = input;
        auto lower = input.toLowerCase();

        // Step 1: Detect expertise level
        result.expertise = detectExpertise (lower);

        // Step 2: Check for direct parameter references (Tier 1)
        result.hasTechnicalContent = containsTechnicalContent (lower);

        // Step 3: Apply modifier mappings (Tier 2 — musical adjectives)
        applyModifierMappings (lower, result);

        // Step 4: Apply synesthetic mappings (Tier 3 — abstract/creative)
        applySynestheticMappings (lower, result);

        // Step 5: Detect mood hints
        result.suggestedMood = detectMood (lower);

        // Step 6: Suggest engines based on texture hints
        suggestEngines (lower, result);

        // Step 7: Suggest FX
        suggestFX (lower, result);

        // Step 8: Build enriched prompt
        result.enrichedPrompt = buildEnrichedPrompt (result, input);

        // Step 9: Calculate confidence
        result.confidence = calculateConfidence (result);

        return result;
    }

    /// Parse a refinement request ("make it more X", "less Y", "brighter").
    /// Returns the direction(s) to adjust.
    static std::vector<RefinementDirection> parseRefinement (const juce::String& userInput)
    {
        std::vector<RefinementDirection> directions;
        auto lower = userInput.toLowerCase().trim();

        // Pattern: "more X" / "less X"
        float polarity = 1.0f;
        if (lower.contains ("less ") || lower.contains ("reduce ") || lower.contains ("decrease ")
            || lower.contains ("lower ") || lower.contains ("softer ") || lower.contains ("subtler "))
            polarity = -1.0f;

        // Magnitude modifiers
        float magnitude = 0.3f;
        if (lower.contains ("much ") || lower.contains ("way ") || lower.contains ("lot "))
            magnitude = 0.5f;
        else if (lower.contains ("slightly ") || lower.contains ("a bit ") || lower.contains ("touch "))
            magnitude = 0.15f;
        else if (lower.contains ("extremely ") || lower.contains ("super ") || lower.contains ("insanely "))
            magnitude = 0.7f;

        // Map adjectives to DNA dimensions
        struct AdjMapping { const char* word; const char* dimension; float weight; };
        static const AdjMapping adjMappings[] = {
            // Brightness
            { "bright",   "brightness",  1.0f },
            { "dark",     "brightness", -1.0f },
            { "dull",     "brightness", -0.7f },
            { "crisp",    "brightness",  0.6f },
            { "sparkl",   "brightness",  0.8f },
            { "shimmer",  "brightness",  0.7f },
            { "murky",    "brightness", -0.6f },
            { "clear",    "brightness",  0.5f },

            // Warmth
            { "warm",     "warmth",      1.0f },
            { "cold",     "warmth",     -1.0f },
            { "cool",     "warmth",     -0.5f },
            { "analog",   "warmth",      0.7f },
            { "vintage",  "warmth",      0.6f },
            { "digital",  "warmth",     -0.5f },
            { "sterile",  "warmth",     -0.8f },
            { "lush",     "warmth",      0.8f },
            { "rich",     "warmth",      0.6f },
            { "thin",     "warmth",     -0.6f },

            // Movement
            { "moving",   "movement",    0.7f },
            { "static",   "movement",   -0.8f },
            { "evolving",  "movement",   0.9f },
            { "alive",    "movement",    0.8f },
            { "pulsing",  "movement",    0.7f },
            { "swirl",    "movement",    0.8f },
            { "morph",    "movement",    0.9f },
            { "still",    "movement",   -0.7f },
            { "steady",   "movement",   -0.5f },
            { "animated", "movement",    0.6f },
            { "flowing",  "movement",    0.7f },
            { "dynamic",  "movement",    0.6f },
            { "rhythmic", "movement",    0.5f },

            // Density
            { "thick",    "density",     0.8f },
            { "thin",     "density",    -0.7f },
            { "massive",  "density",     0.9f },
            { "huge",     "density",     0.8f },
            { "big",      "density",     0.6f },
            { "full",     "density",     0.7f },
            { "sparse",   "density",    -0.7f },
            { "minimal",  "density",    -0.6f },
            { "delicate", "density",    -0.5f },
            { "light",    "density",    -0.4f },
            { "heavy",    "density",     0.7f },
            { "powerful",  "density",    0.6f },
            { "dense",    "density",     0.8f },

            // Space
            { "spacious", "space",       0.8f },
            { "wide",     "space",       0.7f },
            { "vast",     "space",       0.9f },
            { "intimate", "space",      -0.5f },
            { "close",    "space",      -0.6f },
            { "tight",    "space",      -0.7f },
            { "dry",      "space",      -0.8f },
            { "wet",      "space",       0.7f },
            { "airy",     "space",       0.6f },
            { "cavernous","space",       0.9f },
            { "open",     "space",       0.5f },
            { "distant",  "space",       0.7f },
            { "room",     "space",       0.4f },

            // Aggression
            { "aggressive","aggression", 0.8f },
            { "gentle",   "aggression", -0.7f },
            { "harsh",    "aggression",  0.7f },
            { "soft",     "aggression", -0.6f },
            { "brutal",   "aggression",  0.9f },
            { "smooth",   "aggression", -0.7f },
            { "gritty",   "aggression",  0.6f },
            { "raw",      "aggression",  0.5f },
            { "nasty",    "aggression",  0.7f },
            { "clean",    "aggression", -0.5f },
            { "distort",  "aggression",  0.8f },
            { "biting",   "aggression",  0.6f },
            { "mellow",   "aggression", -0.6f },
            { "fierce",   "aggression",  0.8f },
            { "angry",    "aggression",  0.7f },
            { "peaceful", "aggression", -0.8f },
            { "sweet",    "aggression", -0.5f },
        };

        for (const auto& m : adjMappings)
        {
            if (lower.contains (m.word))
            {
                float dir = m.weight > 0 ? polarity : -polarity;
                float adjustedMagnitude = magnitude * std::abs (m.weight);

                // If the word itself implies a direction, use it directly
                // (e.g. "dark" is always -brightness, "bright" is always +brightness)
                if (lower.contains ("more ") || lower.contains ("extra "))
                    dir = m.weight > 0 ? 1.0f : -1.0f;

                // For standalone adjectives without "more/less", use the word's natural direction
                if (!lower.contains ("more ") && !lower.contains ("less ")
                    && !lower.contains ("reduce ") && !lower.contains ("increase "))
                    dir = m.weight > 0 ? 1.0f : -1.0f;

                directions.push_back ({
                    m.dimension,
                    dir * adjustedMagnitude,
                    juce::String ("Adjust ") + m.dimension + " by "
                        + juce::String (dir * adjustedMagnitude, 2)
                        + " based on \"" + m.word + "\""
                });
            }
        }

        // Deduplicate: if multiple words map to the same dimension, average them
        std::map<juce::String, std::pair<float, int>> dimensionSums;
        for (const auto& d : directions)
        {
            dimensionSums[d.dimension].first += d.magnitude;
            dimensionSums[d.dimension].second++;
        }

        std::vector<RefinementDirection> merged;
        for (const auto& [dim, sumCount] : dimensionSums)
        {
            float avg = sumCount.first / static_cast<float> (sumCount.second);
            avg = juce::jlimit (-1.0f, 1.0f, avg);
            merged.push_back ({
                dim, avg,
                "Adjust " + dim + " by " + juce::String (avg, 2)
            });
        }

        return merged;
    }

    /// Apply a DNA delta to an existing DNA, clamping results to [0, 1].
    static RecipeEngine::Recipe::SonicDNA applyDNADelta (
        const RecipeEngine::Recipe::SonicDNA& current,
        const InterpretedIntent::DNADelta& delta)
    {
        auto clamp01 = [] (float v) { return juce::jlimit (0.0f, 1.0f, v); };

        RecipeEngine::Recipe::SonicDNA result;
        result.brightness  = clamp01 (current.brightness + delta.brightness);
        result.warmth      = clamp01 (current.warmth + delta.warmth);
        result.movement    = clamp01 (current.movement + delta.movement);
        result.density     = clamp01 (current.density + delta.density);
        result.space       = clamp01 (current.space + delta.space);
        result.aggression  = clamp01 (current.aggression + delta.aggression);
        return result;
    }

    /// Convert refinement directions into a DNA delta.
    static InterpretedIntent::DNADelta refinementsToDNADelta (
        const std::vector<RefinementDirection>& directions)
    {
        InterpretedIntent::DNADelta delta;
        for (const auto& d : directions)
        {
            if (d.dimension == "brightness")  delta.brightness  += d.magnitude;
            else if (d.dimension == "warmth")      delta.warmth      += d.magnitude;
            else if (d.dimension == "movement")    delta.movement    += d.magnitude;
            else if (d.dimension == "density")     delta.density     += d.magnitude;
            else if (d.dimension == "space")       delta.space       += d.magnitude;
            else if (d.dimension == "aggression")  delta.aggression  += d.magnitude;
        }
        // Clamp deltas to [-1, 1]
        auto clamp = [] (float& v) { v = juce::jlimit (-1.0f, 1.0f, v); };
        clamp (delta.brightness);
        clamp (delta.warmth);
        clamp (delta.movement);
        clamp (delta.density);
        clamp (delta.space);
        clamp (delta.aggression);
        return delta;
    }

private:
    //--------------------------------------------------------------------------
    // Expertise detection

    static ExpertiseLevel detectExpertise (const juce::String& lower)
    {
        // Expert indicators: specific parameter names, technical terms, numeric values
        static const char* expertTerms[] = {
            "cutoff", "resonance", "q ", "lfo", "fm ", "oscillator", "envelope",
            "adsr", "detune", "unison", "wavetable", "filter", "modulation",
            "frequency", "amplitude", "phase", "harmonic", "partial", "overtone",
            "formant", "grain", "granular", "bitcrush", "sample rate", "wavefold",
            "feedback", "delay time", "reverb size", "saturation", "drive",
            "coupling", "index", "ratio", "operator", "morph position"
        };

        static const char* intermediateTerms[] = {
            "bass", "treble", "reverb", "delay", "chorus", "distortion",
            "echo", "sustain", "attack", "release", "decay", "pitch",
            "octave", "note", "key", "chord", "arpeggio", "vibrato",
            "tremolo", "pan", "stereo", "mono", "volume", "tone",
            "eq", "compress", "sidechain", "layer", "stack"
        };

        int expertScore = 0;
        int intermediateScore = 0;

        for (const auto* term : expertTerms)
            if (lower.contains (term)) expertScore++;

        for (const auto* term : intermediateTerms)
            if (lower.contains (term)) intermediateScore++;

        // Check for numeric values with parameter context (expert)
        if (lower.containsAnyOf ("0123456789") && lower.containsAnyOf ("=@."))
            expertScore += 2;

        // Check for parameter ID patterns (prefix_paramName)
        if (lower.contains ("_"))
            expertScore += 2;

        if (expertScore >= 2) return ExpertiseLevel::Expert;
        if (intermediateScore >= 2 || expertScore >= 1) return ExpertiseLevel::Intermediate;
        return ExpertiseLevel::Novice;
    }

    //--------------------------------------------------------------------------
    // Technical content detection

    static bool containsTechnicalContent (const juce::String& lower)
    {
        // Check for parameter ID patterns
        static const char* prefixes[] = {
            "snap_", "morph_", "dub_", "odyssey_", "bob_", "fat_", "poss_",
            "onset_", "era_", "opal_", "orbital_", "organon_", "ouroboros_",
            "obsidian_", "origami_", "oracle_", "obscura_", "oceanic_",
            "optic_", "oblq_"
        };

        for (const auto* prefix : prefixes)
            if (lower.contains (prefix)) return true;

        // Check for coupling type references
        if (lower.contains ("audiotofm") || lower.contains ("audiotoring")
            || lower.contains ("amptopitch") || lower.contains ("lfotopitch"))
            return true;

        return false;
    }

    //--------------------------------------------------------------------------
    // Modifier mappings (Tier 2 — musical adjectives → DNA)

    static void applyModifierMappings (const juce::String& lower, InterpretedIntent& result)
    {
        struct ModMapping
        {
            const char* word;
            float br, wa, mv, de, sp, ag;  // DNA deltas
            const char* texture;            // Optional texture hint
        };

        static const ModMapping modifiers[] = {
            // Brightness axis
            { "bright",     0.4f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, "crisp" },
            { "dark",      -0.4f,  0.2f,  0.0f,  0.1f,  0.0f,  0.0f, "warm" },
            { "sparkl",     0.5f,  0.0f,  0.2f,  0.0f,  0.1f,  0.0f, "glassy" },
            { "shimmer",    0.4f,  0.0f,  0.3f,  0.0f,  0.2f,  0.0f, "ethereal" },
            { "glitter",    0.5f, -0.1f,  0.3f,  0.0f,  0.1f,  0.0f, "sparkling" },

            // Warmth axis
            { "warm",       0.0f,  0.5f,  0.0f,  0.1f,  0.0f, -0.1f, "analog" },
            { "cold",       0.1f, -0.4f,  0.0f,  0.0f,  0.2f,  0.0f, "digital" },
            { "lush",       0.1f,  0.4f,  0.2f,  0.2f,  0.1f,  0.0f, "rich" },
            { "vintage",    0.0f,  0.4f,  0.0f,  0.1f,  0.0f,  0.0f, "analog" },
            { "retro",     -0.1f,  0.3f,  0.0f,  0.1f,  0.0f,  0.0f, "lo-fi" },

            // Movement axis
            { "evolving",   0.0f,  0.0f,  0.5f,  0.0f,  0.1f,  0.0f, "morphing" },
            { "pulsing",    0.0f,  0.0f,  0.5f,  0.0f,  0.0f,  0.1f, "rhythmic" },
            { "swirling",   0.0f,  0.0f,  0.5f,  0.0f,  0.2f,  0.0f, "phasing" },
            { "static",     0.0f,  0.0f, -0.4f,  0.0f,  0.0f,  0.0f, "sustained" },
            { "flowing",    0.0f,  0.1f,  0.4f,  0.0f,  0.1f,  0.0f, "liquid" },
            { "breathing",  0.0f,  0.1f,  0.4f,  0.0f,  0.0f, -0.1f, "organic" },

            // Density axis
            { "massive",    0.0f,  0.1f,  0.0f,  0.5f,  0.1f,  0.2f, "wall-of-sound" },
            { "huge",       0.0f,  0.1f,  0.0f,  0.5f,  0.2f,  0.1f, "epic" },
            { "big",        0.0f,  0.0f,  0.0f,  0.3f,  0.1f,  0.0f, "full" },
            { "powerful",   0.0f,  0.0f,  0.0f,  0.4f,  0.0f,  0.3f, "impactful" },
            { "delicate",   0.1f,  0.0f,  0.1f, -0.3f,  0.1f, -0.2f, "fragile" },
            { "minimal",    0.0f,  0.0f, -0.1f, -0.4f,  0.1f, -0.1f, "sparse" },
            { "thin",       0.1f, -0.2f,  0.0f, -0.3f,  0.0f,  0.0f, "narrow" },

            // Space axis
            { "spacious",   0.0f,  0.0f,  0.0f,  0.0f,  0.5f,  0.0f, "reverberant" },
            { "vast",       0.0f,  0.0f,  0.1f,  0.0f,  0.6f,  0.0f, "ambient" },
            { "intimate",   0.0f,  0.1f,  0.0f,  0.0f, -0.4f,  0.0f, "close" },
            { "cavernous",  0.0f,  0.0f,  0.1f,  0.1f,  0.5f,  0.0f, "echoing" },
            { "airy",       0.2f,  0.0f,  0.1f, -0.1f,  0.4f, -0.1f, "floating" },
            { "dry",        0.0f,  0.0f,  0.0f,  0.0f, -0.4f,  0.0f, "direct" },

            // Aggression axis
            { "aggressive", 0.0f, -0.1f,  0.1f,  0.2f,  0.0f,  0.5f, "distorted" },
            { "gentle",     0.0f,  0.2f, -0.1f, -0.1f,  0.1f, -0.4f, "soft" },
            { "harsh",      0.1f, -0.2f,  0.0f,  0.0f,  0.0f,  0.5f, "abrasive" },
            { "smooth",     0.0f,  0.2f,  0.0f,  0.0f,  0.1f, -0.4f, "polished" },
            { "gritty",    -0.1f,  0.0f,  0.0f,  0.1f,  0.0f,  0.4f, "textured" },
            { "brutal",     0.0f, -0.2f,  0.1f,  0.3f,  0.0f,  0.6f, "crushing" },
            { "raw",       -0.1f,  0.0f,  0.0f,  0.1f, -0.1f,  0.3f, "unprocessed" },
            { "punchy",     0.0f,  0.0f,  0.1f,  0.2f, -0.1f,  0.3f, "transient" },

            // Compound qualities
            { "dreamy",     0.1f,  0.2f,  0.3f,  0.0f,  0.4f, -0.3f, "ethereal" },
            { "ethereal",   0.2f,  0.0f,  0.2f, -0.2f,  0.5f, -0.3f, "otherworldly" },
            { "haunting",  -0.1f,  0.0f,  0.2f,  0.0f,  0.4f, -0.1f, "eerie" },
            { "cinematic",  0.0f,  0.1f,  0.2f,  0.3f,  0.4f,  0.0f, "epic" },
            { "epic",       0.0f,  0.1f,  0.2f,  0.4f,  0.3f,  0.2f, "grandiose" },
            { "ambient",    0.0f,  0.1f,  0.2f, -0.1f,  0.5f, -0.3f, "atmospheric" },
            { "industrial", 0.0f, -0.2f,  0.2f,  0.2f, -0.1f,  0.5f, "mechanical" },
            { "organic",    0.0f,  0.3f,  0.2f,  0.0f,  0.1f, -0.2f, "natural" },
            { "glassy",     0.4f, -0.1f,  0.1f, -0.1f,  0.2f, -0.2f, "crystalline" },
            { "metallic",   0.2f, -0.3f,  0.1f,  0.1f,  0.1f,  0.3f, "resonant" },
            { "creamy",     0.0f,  0.4f,  0.0f,  0.1f,  0.0f, -0.3f, "smooth" },
            { "punchy",     0.0f,  0.0f,  0.1f,  0.3f, -0.1f,  0.3f, "impactful" },
            { "lo-fi",     -0.2f,  0.2f,  0.0f,  0.0f, -0.1f,  0.1f, "degraded" },
            { "hi-fi",      0.3f,  0.0f,  0.0f,  0.0f,  0.0f, -0.2f, "pristine" },
            { "underwater", -0.2f,  0.2f,  0.2f,  0.1f,  0.3f, -0.2f, "filtered" },
            { "celestial",  0.3f,  0.0f,  0.3f, -0.1f,  0.5f, -0.3f, "heavenly" },
        };

        for (const auto& m : modifiers)
        {
            if (lower.contains (m.word))
            {
                result.dnaDelta.brightness  += m.br;
                result.dnaDelta.warmth      += m.wa;
                result.dnaDelta.movement    += m.mv;
                result.dnaDelta.density     += m.de;
                result.dnaDelta.space       += m.sp;
                result.dnaDelta.aggression  += m.ag;

                if (m.texture[0] != '\0')
                    result.textureHints.addIfNotAlreadyThere (m.texture);
            }
        }

        // Clamp accumulated deltas
        auto clamp = [] (float& v) { v = juce::jlimit (-1.0f, 1.0f, v); };
        clamp (result.dnaDelta.brightness);
        clamp (result.dnaDelta.warmth);
        clamp (result.dnaDelta.movement);
        clamp (result.dnaDelta.density);
        clamp (result.dnaDelta.space);
        clamp (result.dnaDelta.aggression);
    }

    //--------------------------------------------------------------------------
    // Synesthetic mappings (Tier 3 — colors, textures, nature, emotions → DNA)

    static void applySynestheticMappings (const juce::String& lower, InterpretedIntent& result)
    {
        // Color → sound mappings (based on crossmodal research)
        struct ColorMapping
        {
            const char* color;
            float br, wa, mv, de, sp, ag;
            const char* mood;
            const char* texture;
        };

        static const ColorMapping colors[] = {
            // Primary colors
            { "red",     -0.1f,  0.2f,  0.1f,  0.3f,  0.0f,  0.5f, "Foundation", "aggressive" },
            { "blue",     0.1f, -0.1f,  0.2f,  0.0f,  0.4f, -0.3f, "Atmosphere", "cool" },
            { "yellow",   0.5f,  0.2f,  0.2f,  0.0f,  0.1f,  0.0f, "Prism", "bright" },
            { "green",    0.0f,  0.3f,  0.2f,  0.1f,  0.2f, -0.2f, "Atmosphere", "organic" },
            { "purple",   0.0f,  0.1f,  0.3f,  0.2f,  0.3f,  0.0f, "Aether", "mystical" },
            { "orange",   0.2f,  0.3f,  0.2f,  0.2f,  0.0f,  0.2f, "Flux", "warm" },
            { "pink",     0.3f,  0.2f,  0.1f, -0.1f,  0.1f, -0.2f, "Prism", "soft" },
            { "black",   -0.4f,  0.0f,  0.1f,  0.3f,  0.0f,  0.3f, "Foundation", "deep" },
            { "white",    0.5f, -0.2f,  0.0f, -0.2f,  0.3f, -0.3f, "Prism", "pure" },
            { "gold",     0.3f,  0.4f,  0.1f,  0.2f,  0.1f,  0.0f, "Aether", "rich" },
            { "silver",   0.4f, -0.1f,  0.1f,  0.0f,  0.2f, -0.1f, "Prism", "metallic" },
            { "neon",     0.5f, -0.2f,  0.3f,  0.0f,  0.0f,  0.2f, "Flux", "electric" },
        };

        // Nature → sound mappings
        struct NatureMapping
        {
            const char* word;
            float br, wa, mv, de, sp, ag;
            const char* mood;
            const char* texture;
        };

        static const NatureMapping nature[] = {
            { "ocean",    0.0f,  0.1f,  0.4f,  0.3f,  0.5f, -0.1f, "Atmosphere", "waves" },
            { "rain",     0.2f,  0.0f,  0.3f,  0.2f,  0.3f, -0.2f, "Atmosphere", "granular" },
            { "thunder",  -0.2f,  0.1f,  0.1f,  0.5f,  0.3f,  0.6f, "Foundation", "rumbling" },
            { "wind",     0.1f,  0.0f,  0.4f, -0.1f,  0.3f, -0.1f, "Flux", "breathy" },
            { "fire",     0.1f,  0.2f,  0.3f,  0.2f,  0.0f,  0.5f, "Flux", "crackling" },
            { "ice",      0.4f, -0.4f,  0.0f,  0.1f,  0.2f,  0.0f, "Prism", "crystalline" },
            { "forest",   0.0f,  0.3f,  0.2f,  0.2f,  0.3f, -0.2f, "Atmosphere", "organic" },
            { "desert",   0.1f,  0.2f,  0.1f, -0.2f,  0.4f,  0.0f, "Aether", "dry" },
            { "space",    0.1f, -0.1f,  0.2f, -0.1f,  0.6f, -0.2f, "Aether", "cosmic" },
            { "cosmos",   0.2f, -0.1f,  0.3f,  0.1f,  0.6f, -0.2f, "Aether", "vast" },
            { "crystal",  0.5f, -0.1f,  0.1f,  0.0f,  0.2f, -0.1f, "Prism", "glassy" },
            { "volcano",  -0.2f,  0.2f,  0.2f,  0.4f,  0.1f,  0.7f, "Foundation", "explosive" },
            { "waterfall",0.1f,  0.1f,  0.3f,  0.3f,  0.4f,  0.0f, "Flux", "cascading" },
            { "sunrise",  0.3f,  0.3f,  0.2f,  0.0f,  0.3f, -0.2f, "Prism", "glowing" },
            { "sunset",   0.0f,  0.4f,  0.2f,  0.1f,  0.3f, -0.1f, "Atmosphere", "fading" },
            { "midnight", -0.3f,  0.1f,  0.1f,  0.1f,  0.2f,  0.0f, "Aether", "nocturnal" },
            { "moonlight", 0.2f,  0.0f,  0.1f, -0.1f,  0.3f, -0.2f, "Aether", "luminous" },
            { "storm",    -0.1f,  0.0f,  0.4f,  0.4f,  0.2f,  0.5f, "Entangled", "chaotic" },
        };

        // Emotion → sound mappings
        struct EmotionMapping
        {
            const char* word;
            float br, wa, mv, de, sp, ag;
            const char* mood;
        };

        static const EmotionMapping emotions[] = {
            { "happy",     0.3f,  0.2f,  0.2f,  0.0f,  0.1f, -0.2f, "Prism" },
            { "sad",      -0.2f,  0.1f,  0.0f,  0.0f,  0.3f, -0.3f, "Atmosphere" },
            { "angry",    -0.1f, -0.1f,  0.2f,  0.3f,  0.0f,  0.6f, "Foundation" },
            { "peaceful",  0.1f,  0.3f,  0.0f, -0.1f,  0.3f, -0.5f, "Atmosphere" },
            { "anxious",   0.1f, -0.1f,  0.3f,  0.1f,  0.0f,  0.2f, "Entangled" },
            { "euphoric",  0.3f,  0.1f,  0.4f,  0.2f,  0.2f,  0.1f, "Aether" },
            { "melanchol", 0.0f,  0.2f,  0.1f,  0.0f,  0.3f, -0.2f, "Atmosphere" },
            { "nostalg",  -0.1f,  0.4f,  0.1f,  0.0f,  0.2f, -0.2f, "Atmosphere" },
            { "tense",     0.0f, -0.1f,  0.2f,  0.2f, -0.1f,  0.3f, "Entangled" },
            { "mysterious",-0.2f,  0.0f,  0.2f,  0.1f,  0.3f,  0.0f, "Aether" },
            { "joyful",    0.4f,  0.2f,  0.3f,  0.0f,  0.1f, -0.1f, "Prism" },
            { "eerie",    -0.1f, -0.1f,  0.2f,  0.0f,  0.4f,  0.1f, "Aether" },
            { "triumphant",0.1f,  0.1f,  0.2f,  0.4f,  0.3f,  0.3f, "Foundation" },
            { "lonely",   -0.1f,  0.0f,  0.0f, -0.3f,  0.4f, -0.2f, "Atmosphere" },
            { "playful",   0.3f,  0.1f,  0.3f, -0.1f,  0.0f, -0.1f, "Prism" },
            { "ominous",  -0.3f,  0.0f,  0.1f,  0.3f,  0.2f,  0.2f, "Entangled" },
        };

        // Material textures
        struct MaterialMapping
        {
            const char* word;
            float br, wa, mv, de, sp, ag;
            const char* texture;
        };

        static const MaterialMapping materials[] = {
            { "glass",     0.5f, -0.1f,  0.0f, -0.1f,  0.2f, -0.1f, "glassy" },
            { "metal",     0.2f, -0.2f,  0.1f,  0.2f,  0.1f,  0.3f, "metallic" },
            { "wood",     -0.1f,  0.4f,  0.0f,  0.1f,  0.1f, -0.1f, "woody" },
            { "silk",      0.1f,  0.2f,  0.1f, -0.1f,  0.0f, -0.3f, "silky" },
            { "velvet",    0.0f,  0.4f,  0.0f,  0.2f,  0.0f, -0.3f, "plush" },
            { "concrete",  -0.2f, -0.1f,  0.0f,  0.3f, -0.1f,  0.3f, "gritty" },
            { "rubber",   -0.2f,  0.1f,  0.1f,  0.2f, -0.1f,  0.1f, "bouncy" },
            { "cotton",    0.0f,  0.3f,  0.0f, -0.1f,  0.0f, -0.3f, "soft" },
            { "steel",     0.3f, -0.3f,  0.0f,  0.2f,  0.1f,  0.3f, "sharp" },
            { "cloud",     0.1f,  0.1f,  0.1f, -0.2f,  0.5f, -0.4f, "fluffy" },
            { "liquid",    0.0f,  0.2f,  0.3f,  0.1f,  0.1f, -0.1f, "fluid" },
            { "smoke",    -0.1f,  0.1f,  0.2f,  0.0f,  0.3f, -0.2f, "hazy" },
        };

        // Apply color mappings
        for (const auto& c : colors)
        {
            // Require "color" context — "blue" alone might mean sad, not the color
            // But "color blue" or "like blue" or "the blue" clearly means color
            bool isColorContext = lower.contains ("color") || lower.contains ("colour")
                || lower.contains ("like " + juce::String (c.color))
                || lower.contains ("the " + juce::String (c.color))
                || lower.contains (juce::String (c.color) + " light")
                || lower.contains (juce::String (c.color) + " glow");

            if (lower.contains (c.color) && (isColorContext || !isAmbiguousColor (c.color)))
            {
                result.dnaDelta.brightness  += c.br;
                result.dnaDelta.warmth      += c.wa;
                result.dnaDelta.movement    += c.mv;
                result.dnaDelta.density     += c.de;
                result.dnaDelta.space       += c.sp;
                result.dnaDelta.aggression  += c.ag;
                if (result.suggestedMood.isEmpty() && c.mood[0] != '\0')
                    result.suggestedMood = c.mood;
                result.textureHints.addIfNotAlreadyThere (c.texture);
            }
        }

        // Apply nature mappings
        for (const auto& n : nature)
        {
            if (lower.contains (n.word))
            {
                result.dnaDelta.brightness  += n.br;
                result.dnaDelta.warmth      += n.wa;
                result.dnaDelta.movement    += n.mv;
                result.dnaDelta.density     += n.de;
                result.dnaDelta.space       += n.sp;
                result.dnaDelta.aggression  += n.ag;
                if (result.suggestedMood.isEmpty() && n.mood[0] != '\0')
                    result.suggestedMood = n.mood;
                result.textureHints.addIfNotAlreadyThere (n.texture);
            }
        }

        // Apply emotion mappings
        for (const auto& e : emotions)
        {
            if (lower.contains (e.word))
            {
                result.dnaDelta.brightness  += e.br;
                result.dnaDelta.warmth      += e.wa;
                result.dnaDelta.movement    += e.mv;
                result.dnaDelta.density     += e.de;
                result.dnaDelta.space       += e.sp;
                result.dnaDelta.aggression  += e.ag;
                if (result.suggestedMood.isEmpty() && e.mood[0] != '\0')
                    result.suggestedMood = e.mood;
            }
        }

        // Apply material mappings
        for (const auto& m : materials)
        {
            if (lower.contains (m.word))
            {
                result.dnaDelta.brightness  += m.br;
                result.dnaDelta.warmth      += m.wa;
                result.dnaDelta.movement    += m.mv;
                result.dnaDelta.density     += m.de;
                result.dnaDelta.space       += m.sp;
                result.dnaDelta.aggression  += m.ag;
                result.textureHints.addIfNotAlreadyThere (m.texture);
            }
        }

        // Re-clamp after all accumulations
        auto clamp = [] (float& v) { v = juce::jlimit (-1.0f, 1.0f, v); };
        clamp (result.dnaDelta.brightness);
        clamp (result.dnaDelta.warmth);
        clamp (result.dnaDelta.movement);
        clamp (result.dnaDelta.density);
        clamp (result.dnaDelta.space);
        clamp (result.dnaDelta.aggression);
    }

    static bool isAmbiguousColor (const char* color)
    {
        // These color words have other common meanings in music context
        return juce::String (color) == "blue" || juce::String (color) == "orange"
            || juce::String (color) == "gold" || juce::String (color) == "silver";
    }

    //--------------------------------------------------------------------------
    // Mood detection

    static juce::String detectMood (const juce::String& lower)
    {
        // Direct mood mentions
        if (lower.contains ("foundation") || lower.contains ("solid") || lower.contains ("grounded"))
            return "Foundation";
        if (lower.contains ("atmosphere") || lower.contains ("atmospheric") || lower.contains ("ambient"))
            return "Atmosphere";
        if (lower.contains ("entangled") || lower.contains ("chaotic") || lower.contains ("complex"))
            return "Entangled";
        if (lower.contains ("prism") || lower.contains ("prismatic") || lower.contains ("bright") || lower.contains ("crystal"))
            return "Prism";
        if (lower.contains ("flux") || lower.contains ("morph") || lower.contains ("evolv") || lower.contains ("transform"))
            return "Flux";
        if (lower.contains ("aether") || lower.contains ("ethereal") || lower.contains ("otherworld") || lower.contains ("transcend"))
            return "Aether";

        // Genre-based mood inference
        if (lower.contains ("edm") || lower.contains ("techno") || lower.contains ("house"))
            return "Foundation";
        if (lower.contains ("ambient") || lower.contains ("drone") || lower.contains ("meditation"))
            return "Atmosphere";
        if (lower.contains ("glitch") || lower.contains ("idm") || lower.contains ("experimental"))
            return "Entangled";
        if (lower.contains ("pop") || lower.contains ("future bass") || lower.contains ("synth wave"))
            return "Prism";
        if (lower.contains ("progressive") || lower.contains ("psytrance") || lower.contains ("journey"))
            return "Flux";
        if (lower.contains ("new age") || lower.contains ("healing") || lower.contains ("cosmic"))
            return "Aether";

        return {};
    }

    //--------------------------------------------------------------------------
    // Engine suggestion based on content

    static void suggestEngines (const juce::String& lower, InterpretedIntent& result)
    {
        // Texture-to-engine mappings
        if (lower.contains ("grain") || lower.contains ("granular") || lower.contains ("scatter"))
            result.suggestedEngines.addIfNotAlreadyThere ("Opal");
        if (lower.contains ("fm ") || lower.contains ("bell") || lower.contains ("metallic"))
            result.suggestedEngines.addIfNotAlreadyThere ("Odyssey");
        if (lower.contains ("analog") || lower.contains ("vintage") || lower.contains ("classic"))
            result.suggestedEngines.addIfNotAlreadyThere ("OddfeliX");
        if (lower.contains ("morph") || lower.contains ("transform") || lower.contains ("shapeshif"))
            result.suggestedEngines.addIfNotAlreadyThere ("OddOscar");
        if (lower.contains ("layer") || lower.contains ("stack") || lower.contains ("blend"))
            result.suggestedEngines.addIfNotAlreadyThere ("Overdub");
        if (lower.contains ("bass") || lower.contains ("sub") || lower.contains ("low end"))
            result.suggestedEngines.addIfNotAlreadyThere ("Oblong");
        if (lower.contains ("drum") || lower.contains ("percuss") || lower.contains ("hit"))
            result.suggestedEngines.addIfNotAlreadyThere ("Onset");
        if (lower.contains ("retro") || lower.contains ("8-bit") || lower.contains ("chiptune"))
            result.suggestedEngines.addIfNotAlreadyThere ("Overworld");
        if (lower.contains ("feedback") || lower.contains ("recursion") || lower.contains ("self-similar"))
            result.suggestedEngines.addIfNotAlreadyThere ("Ouroboros");
        if (lower.contains ("organ") || lower.contains ("additive") || lower.contains ("harmonic series"))
            result.suggestedEngines.addIfNotAlreadyThere ("Organon");
        if (lower.contains ("physical") || lower.contains ("string") || lower.contains ("pluck"))
            result.suggestedEngines.addIfNotAlreadyThere ("Obscura");
        if (lower.contains ("wavefold") || lower.contains ("phase distort"))
            result.suggestedEngines.addIfNotAlreadyThere ("Obsidian");
        if (lower.contains ("origami") || lower.contains ("fold"))
            result.suggestedEngines.addIfNotAlreadyThere ("Origami");
        if (lower.contains ("visual") || lower.contains ("light") || lower.contains ("pulse"))
            result.suggestedEngines.addIfNotAlreadyThere ("Optic");
        if (lower.contains ("bounce") || lower.contains ("prism") || lower.contains ("refract"))
            result.suggestedEngines.addIfNotAlreadyThere ("Oblique");
        if (lower.contains ("orbit") || lower.contains ("spectral") || lower.contains ("partial"))
            result.suggestedEngines.addIfNotAlreadyThere ("Orbital");
        if (lower.contains ("ocean") || lower.contains ("water") || lower.contains ("tide"))
            result.suggestedEngines.addIfNotAlreadyThere ("Oceanic");
    }

    //--------------------------------------------------------------------------
    // FX suggestion

    static void suggestFX (const juce::String& lower, InterpretedIntent& result)
    {
        if (lower.contains ("reverb") || lower.contains ("hall") || lower.contains ("room")
            || lower.contains ("cavernous") || lower.contains ("vast"))
            result.suggestedFX.addIfNotAlreadyThere ("Reverb");
        if (lower.contains ("delay") || lower.contains ("echo") || lower.contains ("repeat"))
            result.suggestedFX.addIfNotAlreadyThere ("Delay");
        if (lower.contains ("distort") || lower.contains ("saturate") || lower.contains ("overdrive") || lower.contains ("grit"))
            result.suggestedFX.addIfNotAlreadyThere ("Saturator");
        if (lower.contains ("chorus") || lower.contains ("flanger") || lower.contains ("phaser"))
            result.suggestedFX.addIfNotAlreadyThere ("Modulation");
        if (lower.contains ("shimmer") || lower.contains ("sparkle"))
            result.suggestedFX.addIfNotAlreadyThere ("HarmonicExciter");
        if (lower.contains ("wide") || lower.contains ("stereo") || lower.contains ("spread"))
            result.suggestedFX.addIfNotAlreadyThere ("StereoSculptor");
        if (lower.contains ("smear") || lower.contains ("blur") || lower.contains ("freeze"))
            result.suggestedFX.addIfNotAlreadyThere ("GranularSmear");
        if (lower.contains ("doppler") || lower.contains ("spinning") || lower.contains ("rotating"))
            result.suggestedFX.addIfNotAlreadyThere ("Doppler");
    }

    //--------------------------------------------------------------------------
    // Enriched prompt builder

    static juce::String buildEnrichedPrompt (const InterpretedIntent& result,
                                              const juce::String& originalInput)
    {
        juce::String prompt;

        // Start with the original request
        prompt += "User request: \"" + originalInput + "\"\n\n";

        // Add interpreted DNA guidance
        if (!result.dnaDelta.isZero())
        {
            prompt += "INTERPRETED SONIC DIRECTION (target DNA adjustments):\n";
            auto addIfNonZero = [&] (const char* name, float val)
            {
                if (std::abs (val) >= 0.05f)
                    prompt += juce::String ("  ") + name + ": "
                              + (val > 0 ? "+" : "") + juce::String (val, 2) + "\n";
            };
            addIfNonZero ("brightness", result.dnaDelta.brightness);
            addIfNonZero ("warmth", result.dnaDelta.warmth);
            addIfNonZero ("movement", result.dnaDelta.movement);
            addIfNonZero ("density", result.dnaDelta.density);
            addIfNonZero ("space", result.dnaDelta.space);
            addIfNonZero ("aggression", result.dnaDelta.aggression);
            prompt += "\n";
        }

        // Add mood guidance
        if (result.suggestedMood.isNotEmpty())
            prompt += "SUGGESTED MOOD: " + result.suggestedMood + "\n\n";

        // Add texture hints
        if (result.textureHints.size() > 0)
            prompt += "TEXTURE HINTS: " + result.textureHints.joinIntoString (", ") + "\n\n";

        // Add engine hints
        if (result.suggestedEngines.size() > 0)
            prompt += "SUGGESTED ENGINES (consider these): "
                      + result.suggestedEngines.joinIntoString (", ") + "\n\n";

        // Add FX hints
        if (result.suggestedFX.size() > 0)
            prompt += "SUGGESTED FX (consider these): "
                      + result.suggestedFX.joinIntoString (", ") + "\n\n";

        // Expertise-specific instructions
        switch (result.expertise)
        {
            case ExpertiseLevel::Novice:
                prompt += "USER LEVEL: Beginner. Prioritize presets that sound immediately "
                          "good with minimal tweaking. Use safe parameter ranges. Choose "
                          "engines that are forgiving and expressive. Keep coupling simple "
                          "(1-2 routes, low intensity). Make macros do dramatic, obvious "
                          "things. Name everything intuitively.\n\n";
                break;

            case ExpertiseLevel::Intermediate:
                prompt += "USER LEVEL: Intermediate. Balance accessibility with depth. "
                          "Include coupling that adds character. Macros should reveal "
                          "interesting parameter interactions. Use moderate complexity.\n\n";
                break;

            case ExpertiseLevel::Expert:
                prompt += "USER LEVEL: Expert. Full creative freedom. Complex coupling, "
                          "unusual engine combinations, and advanced parameter ranges are "
                          "welcome. If the user specified exact parameters, honor them. "
                          "Macros can reveal subtle interactions.\n\n";
                break;
        }

        return prompt;
    }

    //--------------------------------------------------------------------------
    // Confidence calculation

    static float calculateConfidence (const InterpretedIntent& result)
    {
        float confidence = 0.3f; // Base confidence

        // More DNA adjustments = more confident we understood intent
        if (!result.dnaDelta.isZero()) confidence += 0.2f;

        // Mood detected
        if (result.suggestedMood.isNotEmpty()) confidence += 0.1f;

        // Texture hints
        confidence += juce::jmin (0.15f, result.textureHints.size() * 0.05f);

        // Engine suggestions
        if (result.suggestedEngines.size() > 0) confidence += 0.1f;

        // Technical content (highest confidence — user knows what they want)
        if (result.hasTechnicalContent) confidence += 0.2f;

        return juce::jlimit (0.0f, 1.0f, confidence);
    }
};

} // namespace xomnibus
