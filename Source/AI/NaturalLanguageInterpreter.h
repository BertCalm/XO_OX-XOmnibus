// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "RecipeEngine.h"
#include <juce_core/juce_core.h>
#include <map>
#include <vector>
#include <cmath>

namespace xoceanus {

//==============================================================================
// NaturalLanguageInterpreter — Maps creative, abstract, and plain language
// descriptions to Sonic DNA adjustments and parameter intent.
//
// This is the bridge between how people THINK about sound and how XOceanus
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
            //================================================================
            // BRIGHTNESS — spectral tilt, presence, high-frequency energy
            //================================================================
            { "bright",    "brightness",  1.0f },
            { "dark",      "brightness", -1.0f },
            { "dull",      "brightness", -0.7f },
            { "crisp",     "brightness",  0.6f },
            { "sparkl",    "brightness",  0.8f },
            { "shimmer",   "brightness",  0.7f },
            { "murky",     "brightness", -0.6f },
            { "clear",     "brightness",  0.5f },
            { "luminous",  "brightness",  0.9f },
            { "radiant",   "brightness",  0.8f },
            { "brilliant", "brightness",  0.9f },
            { "dim",       "brightness", -0.5f },
            { "hazy",      "brightness", -0.4f },
            { "foggy",     "brightness", -0.5f },
            { "vivid",     "brightness",  0.7f },
            { "muted",     "brightness", -0.5f },
            { "shiny",     "brightness",  0.7f },
            { "gleam",     "brightness",  0.6f },
            { "twinkl",    "brightness",  0.7f },
            { "piercing",  "brightness",  0.8f },
            { "tinny",     "brightness",  0.6f },
            { "sizzl",     "brightness",  0.7f },
            { "airy",      "brightness",  0.5f },
            { "glisten",   "brightness",  0.7f },
            { "glow",      "brightness",  0.5f },
            { "blinding",  "brightness",  1.0f },
            { "shadowy",   "brightness", -0.7f },
            { "opaque",    "brightness", -0.6f },
            { "transluc",  "brightness",  0.4f },
            { "sun",       "brightness",  0.6f },
            { "lunar",     "brightness",  0.3f },
            { "starlight", "brightness",  0.5f },
            { "neon",      "brightness",  0.8f },
            { "phosphor",  "brightness",  0.7f },
            { "incandesc", "brightness",  0.6f },
            { "fluoresc",  "brightness",  0.7f },
            { "lustrous",  "brightness",  0.6f },
            { "pearly",    "brightness",  0.5f },

            //================================================================
            // WARMTH — analog character, saturation, low-mid presence
            //================================================================
            { "warm",      "warmth",      1.0f },
            { "cold",      "warmth",     -1.0f },
            { "cool",      "warmth",     -0.5f },
            { "analog",    "warmth",      0.7f },
            { "vintage",   "warmth",      0.6f },
            { "digital",   "warmth",     -0.5f },
            { "sterile",   "warmth",     -0.8f },
            { "lush",      "warmth",      0.8f },
            { "rich",      "warmth",      0.6f },
            { "thin",      "warmth",     -0.6f },
            { "retro",     "warmth",      0.6f },
            { "tube",      "warmth",      0.7f },
            { "tape",      "warmth",      0.6f },
            { "vinyl",     "warmth",      0.5f },
            { "saturated", "warmth",      0.7f },
            { "clinical",  "warmth",     -0.7f },
            { "synthetic", "warmth",     -0.4f },
            { "cozy",      "warmth",      0.7f },
            { "toasty",    "warmth",      0.6f },
            { "icy",       "warmth",     -0.8f },
            { "frosty",    "warmth",     -0.7f },
            { "arctic",    "warmth",     -0.9f },
            { "glacial",   "warmth",     -0.8f },
            { "soulful",   "warmth",      0.8f },
            { "earthy",    "warmth",      0.6f },
            { "honeyed",   "warmth",      0.7f },
            { "buttery",   "warmth",      0.7f },
            { "velvety",   "warmth",      0.7f },
            { "creamy",    "warmth",      0.6f },
            { "silky",     "warmth",      0.5f },
            { "wiry",      "warmth",     -0.5f },
            { "brittle",   "warmth",     -0.6f },
            { "frigid",    "warmth",     -0.8f },
            { "tropical",  "warmth",      0.5f },
            { "autumnal",  "warmth",      0.5f },
            { "fireside",  "warmth",      0.8f },
            { "embers",    "warmth",      0.6f },
            { "candlelit", "warmth",      0.6f },

            //================================================================
            // MOVEMENT — modulation, temporal evolution, rhythmic activity
            //================================================================
            { "moving",    "movement",    0.7f },
            { "static",    "movement",   -0.8f },
            { "evolving",  "movement",    0.9f },
            { "alive",     "movement",    0.8f },
            { "pulsing",   "movement",    0.7f },
            { "swirl",     "movement",    0.8f },
            { "morph",     "movement",    0.9f },
            { "still",     "movement",   -0.7f },
            { "steady",    "movement",   -0.5f },
            { "animated",  "movement",    0.6f },
            { "flowing",   "movement",    0.7f },
            { "dynamic",   "movement",    0.6f },
            { "rhythmic",  "movement",    0.5f },
            { "shifting",  "movement",    0.7f },
            { "undulat",   "movement",    0.7f },
            { "rippl",     "movement",    0.6f },
            { "wobbl",     "movement",    0.6f },
            { "flutter",   "movement",    0.5f },
            { "throb",     "movement",    0.6f },
            { "churning",  "movement",    0.7f },
            { "writhing",  "movement",    0.8f },
            { "frozen",    "movement",   -0.9f },
            { "frenetic",  "movement",    0.9f },
            { "hypnotic",  "movement",    0.7f },
            { "cascading", "movement",    0.7f },
            { "surging",   "movement",    0.8f },
            { "kinetic",   "movement",    0.7f },
            { "breathing", "movement",    0.5f },
            { "drifting",  "movement",    0.4f },
            { "spinning",  "movement",    0.7f },
            { "oscillat",  "movement",    0.6f },
            { "vibrat",    "movement",    0.5f },
            { "shimmying", "movement",    0.5f },
            { "rolling",   "movement",    0.5f },
            { "galloping", "movement",    0.7f },
            { "tidal",     "movement",    0.6f },
            { "wandering", "movement",    0.4f },
            { "restless",  "movement",    0.6f },
            { "turbulent", "movement",    0.8f },
            { "motionless","movement",   -0.9f },
            { "inert",     "movement",   -0.8f },
            { "suspended", "movement",   -0.4f },
            { "erratic",   "movement",    0.8f },
            { "languid",   "movement",    0.2f },
            { "sluggish",  "movement",    0.1f },

            //================================================================
            // DENSITY — thickness, layering, harmonic content
            //================================================================
            { "thick",     "density",     0.8f },
            { "thin",      "density",    -0.7f },
            { "massive",   "density",     0.9f },
            { "huge",      "density",     0.8f },
            { "big",       "density",     0.6f },
            { "full",      "density",     0.7f },
            { "sparse",    "density",    -0.7f },
            { "minimal",   "density",    -0.6f },
            { "delicate",  "density",    -0.5f },
            { "light",     "density",    -0.4f },
            { "heavy",     "density",     0.7f },
            { "powerful",  "density",     0.6f },
            { "dense",     "density",     0.8f },
            { "fat",       "density",     0.7f },
            { "chunky",    "density",     0.6f },
            { "meaty",     "density",     0.6f },
            { "beefy",     "density",     0.7f },
            { "layered",   "density",     0.7f },
            { "stacked",   "density",     0.7f },
            { "monolithic","density",     0.9f },
            { "colossal",  "density",     0.9f },
            { "towering",  "density",     0.7f },
            { "wispy",     "density",    -0.7f },
            { "gossamer",  "density",    -0.8f },
            { "feathery",  "density",    -0.6f },
            { "hollow",    "density",    -0.6f },
            { "empty",     "density",    -0.8f },
            { "immense",   "density",     0.8f },
            { "overwhelming","density",   0.9f },
            { "compact",   "density",     0.5f },
            { "bloated",   "density",     0.6f },
            { "lean",      "density",    -0.4f },
            { "skeletal",  "density",    -0.7f },
            { "robust",    "density",     0.5f },
            { "titanic",   "density",     0.9f },
            { "microscopic","density",   -0.8f },
            { "substantial","density",    0.5f },
            { "paper-thin","density",    -0.8f },

            //================================================================
            // SPACE — reverb, stereo width, depth, distance
            //================================================================
            { "spacious",  "space",       0.8f },
            { "wide",      "space",       0.7f },
            { "vast",      "space",       0.9f },
            { "intimate",  "space",      -0.5f },
            { "close",     "space",      -0.6f },
            { "tight",     "space",      -0.7f },
            { "dry",       "space",      -0.8f },
            { "wet",       "space",       0.7f },
            { "airy",      "space",       0.6f },
            { "cavernous", "space",       0.9f },
            { "open",      "space",       0.5f },
            { "distant",   "space",       0.7f },
            { "room",      "space",       0.4f },
            { "expansive", "space",       0.8f },
            { "immersive", "space",       0.8f },
            { "panoramic", "space",       0.7f },
            { "claustrophobic","space",  -0.8f },
            { "enclosed",  "space",      -0.6f },
            { "cathedral", "space",       0.9f },
            { "arena",     "space",       0.7f },
            { "stadium",   "space",       0.8f },
            { "drenched",  "space",       0.8f },
            { "soaked",    "space",       0.7f },
            { "submerged", "space",       0.6f },
            { "echoing",   "space",       0.7f },
            { "reverberant","space",      0.8f },
            { "infinite",  "space",       0.9f },
            { "boundless", "space",       0.8f },
            { "enveloping","space",       0.7f },
            { "3d",        "space",       0.6f },
            { "surround",  "space",       0.7f },
            { "mono",      "space",      -0.6f },
            { "narrow",    "space",      -0.5f },
            { "deep",      "space",       0.5f },
            { "shallow",   "space",      -0.4f },
            { "faraway",   "space",       0.7f },
            { "cramped",   "space",      -0.7f },
            { "boxed",     "space",      -0.5f },
            { "cavernlike","space",       0.8f },

            //================================================================
            // AGGRESSION — distortion, edge, intensity, attitude
            //================================================================
            { "aggressive","aggression",  0.8f },
            { "gentle",    "aggression", -0.7f },
            { "harsh",     "aggression",  0.7f },
            { "soft",      "aggression", -0.6f },
            { "brutal",    "aggression",  0.9f },
            { "smooth",    "aggression", -0.7f },
            { "gritty",    "aggression",  0.6f },
            { "raw",       "aggression",  0.5f },
            { "nasty",     "aggression",  0.7f },
            { "clean",     "aggression", -0.5f },
            { "distort",   "aggression",  0.8f },
            { "biting",    "aggression",  0.6f },
            { "mellow",    "aggression", -0.6f },
            { "fierce",    "aggression",  0.8f },
            { "angry",     "aggression",  0.7f },
            { "peaceful",  "aggression", -0.8f },
            { "sweet",     "aggression", -0.5f },
            { "punchy",    "aggression",  0.6f },
            { "ferocious", "aggression",  0.9f },
            { "savage",    "aggression",  0.9f },
            { "vicious",   "aggression",  0.8f },
            { "dirty",     "aggression",  0.5f },
            { "filthy",    "aggression",  0.6f },
            { "crunchy",   "aggression",  0.5f },
            { "crushing",  "aggression",  0.7f },
            { "razor",     "aggression",  0.7f },
            { "caustic",   "aggression",  0.8f },
            { "abrasive",  "aggression",  0.7f },
            { "jagged",    "aggression",  0.6f },
            { "angular",   "aggression",  0.5f },
            { "tender",    "aggression", -0.6f },
            { "polished",  "aggression", -0.5f },
            { "refined",   "aggression", -0.5f },
            { "elegant",   "aggression", -0.5f },
            { "searing",   "aggression",  0.8f },
            { "explosive", "aggression",  0.8f },
            { "thunderous","aggression",  0.7f },
            { "menacing",  "aggression",  0.6f },
            { "tame",      "aggression", -0.5f },
            { "subdued",   "aggression", -0.5f },
            { "serene",    "aggression", -0.8f },
            { "tranquil",  "aggression", -0.7f },
            { "violent",   "aggression",  0.9f },
            { "venomous",  "aggression",  0.7f },
            { "scorching", "aggression",  0.7f },
            { "relentless","aggression",  0.6f },
            { "restrained","aggression", -0.4f },
            { "delicate",  "aggression", -0.6f },
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
            // Synthesis parameters
            "cutoff", "resonance", "q ", "lfo", "fm ", "oscillator", "envelope",
            "adsr", "detune", "unison", "wavetable", "filter", "modulation",
            "frequency", "amplitude", "phase", "harmonic", "partial", "overtone",
            "formant", "grain", "granular", "bitcrush", "sample rate", "wavefold",
            "feedback", "delay time", "reverb size", "saturation", "drive",
            "coupling", "index", "ratio", "operator", "morph position",
            // Advanced DSP terms
            "convolution", "impulse response", "fft", "spectral", "comb filter",
            "allpass", "karplus", "waveguide", "phase distort", "ring mod",
            "vocoder", "cross-modulation", "sideband", "nyquist", "aliasing",
            "oversampl", "denormal", "dc offset", "zero-crossing", "polyphon",
            // XOceanus-specific
            "sonic dna", "mega coupling", "coupling matrix", "engine slot",
            "macro assign", "dna delta", "crossfade", "hot-swap",
            "mod depth", "mod rate", "key track", "velocity sens",
            // Specific value patterns
            "hz", "khz", "ms ", "db ", "semitone", "cent"
        };

        static const char* intermediateTerms[] = {
            // Basic audio terms
            "bass", "treble", "reverb", "delay", "chorus", "distortion",
            "echo", "sustain", "attack", "release", "decay", "pitch",
            "octave", "note", "key", "chord", "arpeggio", "vibrato",
            "tremolo", "pan", "stereo", "mono", "volume", "tone",
            "eq", "compress", "sidechain", "layer", "stack",
            // Additional intermediate terms
            "preset", "patch", "waveform", "tempo", "bpm", "sync",
            "portamento", "glide", "velocity", "aftertouch", "mod wheel",
            "expression", "pitch bend", "midi", "automation", "macro",
            "wet/dry", "mix knob", "input", "output", "gain",
            "highpass", "lowpass", "bandpass", "shelf", "peak",
            "phaser", "flanger", "overdrive", "fuzz", "limiter",
            "gate", "noise gate", "send", "return", "bus"
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
            "snap_", "morph_", "dub_", "drift_", "bob_", "fat_", "poss_",
            "perc_", "ow_", "opal_", "orb_", "organon_", "ouro_",
            "obsidian_", "origami_", "oracle_", "obscura_", "ocean_",
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
            //================================================================
            // BRIGHTNESS AXIS — light/dark, high frequency presence
            //================================================================
            { "bright",     0.4f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, "crisp" },
            { "dark",      -0.4f,  0.2f,  0.0f,  0.1f,  0.0f,  0.0f, "warm" },
            { "sparkl",     0.5f,  0.0f,  0.2f,  0.0f,  0.1f,  0.0f, "glassy" },
            { "shimmer",    0.4f,  0.0f,  0.3f,  0.0f,  0.2f,  0.0f, "ethereal" },
            { "glitter",    0.5f, -0.1f,  0.3f,  0.0f,  0.1f,  0.0f, "sparkling" },
            { "dull",      -0.3f,  0.1f,  0.0f,  0.0f,  0.0f,  0.0f, "muted" },
            { "crisp",      0.3f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, "defined" },
            { "murky",     -0.3f,  0.1f,  0.0f,  0.1f,  0.0f,  0.0f, "cloudy" },
            { "luminous",   0.4f,  0.0f,  0.1f,  0.0f,  0.1f, -0.1f, "radiant" },
            { "radiant",    0.4f,  0.1f,  0.1f,  0.0f,  0.1f, -0.1f, "glowing" },
            { "brilliant",  0.5f,  0.0f,  0.1f,  0.0f,  0.0f,  0.0f, "vivid" },
            { "dim",       -0.3f,  0.1f,  0.0f,  0.0f,  0.0f, -0.1f, "subdued" },
            { "hazy",      -0.2f,  0.1f,  0.1f,  0.0f,  0.2f, -0.1f, "diffused" },
            { "foggy",     -0.3f,  0.1f,  0.1f,  0.0f,  0.2f, -0.1f, "obscured" },
            { "vivid",      0.4f,  0.0f,  0.1f,  0.0f,  0.0f,  0.1f, "intense" },
            { "muted",     -0.2f,  0.1f,  0.0f, -0.1f,  0.0f, -0.1f, "restrained" },
            { "shiny",      0.4f, -0.1f,  0.1f,  0.0f,  0.1f, -0.1f, "polished" },
            { "gleam",      0.4f,  0.0f,  0.1f,  0.0f,  0.1f, -0.1f, "lustrous" },
            { "twinkl",     0.4f,  0.0f,  0.3f, -0.1f,  0.1f, -0.1f, "sparkling" },
            { "incandesc",  0.4f,  0.2f,  0.1f,  0.0f,  0.1f,  0.0f, "glowing" },
            { "phosphor",   0.3f, -0.1f,  0.2f,  0.0f,  0.2f, -0.1f, "glowing" },
            { "shadowy",   -0.3f,  0.0f,  0.1f,  0.1f,  0.1f,  0.0f, "obscured" },
            { "opaque",    -0.2f,  0.1f,  0.0f,  0.2f, -0.1f,  0.0f, "dense" },
            { "translucen", 0.3f, -0.1f,  0.1f, -0.2f,  0.2f, -0.1f, "see-through" },
            { "transparent",0.3f, -0.1f,  0.0f, -0.2f,  0.1f, -0.1f, "clear" },
            { "sheer",      0.2f, -0.1f,  0.0f, -0.2f,  0.1f, -0.1f, "delicate" },
            { "blinding",   0.6f, -0.1f,  0.1f,  0.0f,  0.0f,  0.2f, "extreme" },
            { "piercing",   0.5f, -0.2f,  0.0f,  0.0f,  0.0f,  0.3f, "sharp" },
            { "tinny",      0.3f, -0.3f,  0.0f, -0.2f,  0.0f,  0.1f, "thin" },
            { "sizzl",      0.4f, -0.1f,  0.1f,  0.0f,  0.0f,  0.2f, "hot" },
            { "twinkling",  0.4f,  0.0f,  0.3f, -0.1f,  0.1f, -0.1f, "sparkling" },

            //================================================================
            // WARMTH AXIS — analog character, harmonic richness, saturation
            //================================================================
            { "warm",       0.0f,  0.5f,  0.0f,  0.1f,  0.0f, -0.1f, "analog" },
            { "cold",       0.1f, -0.4f,  0.0f,  0.0f,  0.2f,  0.0f, "digital" },
            { "lush",       0.1f,  0.4f,  0.2f,  0.2f,  0.1f,  0.0f, "rich" },
            { "vintage",    0.0f,  0.4f,  0.0f,  0.1f,  0.0f,  0.0f, "analog" },
            { "retro",     -0.1f,  0.3f,  0.0f,  0.1f,  0.0f,  0.0f, "lo-fi" },
            { "analogue",   0.0f,  0.4f,  0.0f,  0.1f,  0.0f,  0.0f, "analog" },
            { "tube",      -0.1f,  0.4f,  0.0f,  0.1f,  0.0f,  0.1f, "saturated" },
            { "tape",      -0.1f,  0.3f,  0.0f,  0.1f,  0.0f,  0.0f, "saturated" },
            { "vinyl",     -0.1f,  0.3f,  0.0f,  0.0f,  0.0f,  0.0f, "crackly" },
            { "digital",    0.1f, -0.4f,  0.0f,  0.0f,  0.0f,  0.0f, "precise" },
            { "sterile",    0.1f, -0.5f,  0.0f,  0.0f,  0.0f, -0.1f, "clinical" },
            { "clinical",   0.1f, -0.4f,  0.0f,  0.0f,  0.0f, -0.1f, "precise" },
            { "synthetic",  0.1f, -0.3f,  0.0f,  0.0f,  0.0f,  0.0f, "artificial" },
            { "rich",       0.0f,  0.4f,  0.0f,  0.2f,  0.0f, -0.1f, "harmonically-rich" },
            { "saturated",  0.0f,  0.3f,  0.0f,  0.1f,  0.0f,  0.2f, "overdriven" },
            { "cozy",       0.0f,  0.4f,  0.0f,  0.1f, -0.1f, -0.2f, "comfortable" },
            { "toasty",     0.0f,  0.4f,  0.0f,  0.1f,  0.0f,  0.0f, "warm" },
            { "icy",        0.2f, -0.5f,  0.0f,  0.0f,  0.1f,  0.0f, "frozen" },
            { "frosty",     0.2f, -0.4f,  0.0f,  0.0f,  0.1f,  0.0f, "frozen" },
            { "arctic",     0.1f, -0.5f,  0.0f,  0.0f,  0.2f,  0.0f, "glacial" },
            { "glacial",    0.1f, -0.5f,  0.0f,  0.1f,  0.3f,  0.0f, "vast" },
            { "frigid",     0.1f, -0.5f,  0.0f,  0.0f,  0.1f,  0.0f, "frozen" },
            { "wintry",     0.1f, -0.3f,  0.1f,  0.0f,  0.2f, -0.1f, "seasonal" },
            { "inviting",   0.0f,  0.3f,  0.0f,  0.0f,  0.0f, -0.2f, "welcoming" },
            { "soulful",    0.0f,  0.4f,  0.1f,  0.1f,  0.1f, -0.1f, "expressive" },
            { "earthy",    -0.1f,  0.3f,  0.0f,  0.1f,  0.0f, -0.1f, "grounded" },
            { "woody",     -0.1f,  0.3f,  0.0f,  0.1f,  0.1f, -0.1f, "acoustic" },
            { "honeyed",    0.1f,  0.4f,  0.0f,  0.1f,  0.0f, -0.2f, "sweet" },
            { "buttery",    0.0f,  0.4f,  0.0f,  0.1f,  0.0f, -0.2f, "smooth" },
            { "velvety",    0.0f,  0.4f,  0.0f,  0.2f,  0.0f, -0.3f, "plush" },

            //================================================================
            // MOVEMENT AXIS — modulation, animation, temporal change
            //================================================================
            { "evolving",   0.0f,  0.0f,  0.5f,  0.0f,  0.1f,  0.0f, "morphing" },
            { "pulsing",    0.0f,  0.0f,  0.5f,  0.0f,  0.0f,  0.1f, "rhythmic" },
            { "swirling",   0.0f,  0.0f,  0.5f,  0.0f,  0.2f,  0.0f, "phasing" },
            { "static",     0.0f,  0.0f, -0.4f,  0.0f,  0.0f,  0.0f, "sustained" },
            { "flowing",    0.0f,  0.1f,  0.4f,  0.0f,  0.1f,  0.0f, "liquid" },
            { "breathing",  0.0f,  0.1f,  0.4f,  0.0f,  0.0f, -0.1f, "organic" },
            { "morphing",   0.0f,  0.0f,  0.5f,  0.0f,  0.0f,  0.0f, "transforming" },
            { "shifting",   0.0f,  0.0f,  0.4f,  0.0f,  0.0f,  0.0f, "changing" },
            { "undulating", 0.0f,  0.1f,  0.5f,  0.0f,  0.1f, -0.1f, "wavy" },
            { "rippling",   0.1f,  0.0f,  0.4f,  0.0f,  0.1f, -0.1f, "watery" },
            { "wobbl",      0.0f,  0.0f,  0.5f,  0.1f,  0.0f,  0.1f, "unstable" },
            { "flutter",    0.2f,  0.0f,  0.4f, -0.1f,  0.0f, -0.1f, "trembling" },
            { "throb",      0.0f,  0.1f,  0.4f,  0.2f,  0.0f,  0.2f, "pulsing" },
            { "sway",       0.0f,  0.1f,  0.3f,  0.0f,  0.1f, -0.1f, "gentle" },
            { "churning",   0.0f,  0.0f,  0.4f,  0.2f,  0.0f,  0.2f, "turbulent" },
            { "writhing",   0.0f,  0.0f,  0.5f,  0.1f,  0.0f,  0.2f, "tortured" },
            { "seething",   0.0f, -0.1f,  0.4f,  0.2f,  0.0f,  0.3f, "agitated" },
            { "frozen",     0.1f, -0.2f, -0.5f,  0.0f,  0.1f,  0.0f, "static" },
            { "suspended",  0.0f,  0.0f, -0.3f,  0.0f,  0.3f, -0.1f, "floating" },
            { "restless",   0.0f,  0.0f,  0.4f,  0.0f,  0.0f,  0.1f, "anxious" },
            { "languid",    0.0f,  0.2f,  0.2f,  0.0f,  0.1f, -0.2f, "slow" },
            { "lazy",       0.0f,  0.2f,  0.1f,  0.0f,  0.1f, -0.2f, "relaxed" },
            { "frenetic",   0.0f, -0.1f,  0.6f,  0.1f,  0.0f,  0.3f, "frantic" },
            { "frantic",    0.0f, -0.1f,  0.5f,  0.1f,  0.0f,  0.3f, "chaotic" },
            { "hypnotic",   0.0f,  0.1f,  0.4f,  0.0f,  0.1f, -0.1f, "mesmerizing" },
            { "trance",     0.0f,  0.0f,  0.4f,  0.0f,  0.1f, -0.1f, "repetitive" },
            { "gyrating",   0.0f,  0.0f,  0.4f,  0.0f,  0.0f,  0.1f, "spinning" },
            { "oscillat",   0.0f,  0.0f,  0.4f,  0.0f,  0.0f,  0.0f, "waveform" },
            { "vibrat",     0.0f,  0.0f,  0.3f,  0.0f,  0.0f,  0.0f, "tremolo" },
            { "shimmy",     0.1f,  0.0f,  0.3f,  0.0f,  0.0f,  0.0f, "shaking" },
            { "rolling",    0.0f,  0.1f,  0.3f,  0.1f,  0.0f,  0.0f, "continuous" },
            { "cascading",  0.1f,  0.0f,  0.4f,  0.1f,  0.1f, -0.1f, "falling" },
            { "surging",    0.0f,  0.0f,  0.4f,  0.2f,  0.0f,  0.2f, "rising" },
            { "animated",   0.1f,  0.0f,  0.4f,  0.0f,  0.0f,  0.0f, "lively" },
            { "kinetic",    0.0f,  0.0f,  0.4f,  0.1f,  0.0f,  0.1f, "energetic" },
            { "dynamic",    0.0f,  0.0f,  0.3f,  0.1f,  0.0f,  0.1f, "expressive" },

            //================================================================
            // DENSITY AXIS — thickness, layering, harmonic content
            //================================================================
            { "massive",    0.0f,  0.1f,  0.0f,  0.5f,  0.1f,  0.2f, "wall-of-sound" },
            { "huge",       0.0f,  0.1f,  0.0f,  0.5f,  0.2f,  0.1f, "epic" },
            { "big",        0.0f,  0.0f,  0.0f,  0.3f,  0.1f,  0.0f, "full" },
            { "powerful",   0.0f,  0.0f,  0.0f,  0.4f,  0.0f,  0.3f, "impactful" },
            { "delicate",   0.1f,  0.0f,  0.1f, -0.3f,  0.1f, -0.2f, "fragile" },
            { "minimal",    0.0f,  0.0f, -0.1f, -0.4f,  0.1f, -0.1f, "sparse" },
            { "thin",       0.1f, -0.2f,  0.0f, -0.3f,  0.0f,  0.0f, "narrow" },
            { "dense",      0.0f,  0.0f,  0.0f,  0.5f, -0.1f,  0.1f, "packed" },
            { "thick",     -0.1f,  0.2f,  0.0f,  0.4f, -0.1f,  0.1f, "heavy" },
            { "fat",       -0.1f,  0.2f,  0.0f,  0.4f,  0.0f,  0.0f, "wide" },
            { "chunky",    -0.1f,  0.1f,  0.0f,  0.3f,  0.0f,  0.1f, "blocky" },
            { "meaty",     -0.1f,  0.2f,  0.0f,  0.3f,  0.0f,  0.1f, "substantial" },
            { "beefy",     -0.1f,  0.2f,  0.0f,  0.4f,  0.0f,  0.1f, "heavy" },
            { "juicy",      0.0f,  0.2f,  0.1f,  0.3f,  0.0f,  0.0f, "wet" },
            { "full",       0.0f,  0.1f,  0.0f,  0.3f,  0.0f, -0.1f, "complete" },
            { "hollow",     0.1f, -0.2f,  0.0f, -0.3f,  0.2f,  0.0f, "empty" },
            { "sparse",     0.0f,  0.0f, -0.1f, -0.4f,  0.2f, -0.1f, "open" },
            { "empty",      0.0f, -0.1f, -0.1f, -0.4f,  0.2f, -0.1f, "vacant" },
            { "layered",    0.0f,  0.0f,  0.1f,  0.4f,  0.0f,  0.0f, "stacked" },
            { "stacked",    0.0f,  0.0f,  0.0f,  0.4f,  0.0f,  0.0f, "layered" },
            { "monolithic", 0.0f,  0.0f,  0.0f,  0.5f,  0.0f,  0.2f, "solid" },
            { "colossal",   0.0f,  0.0f,  0.0f,  0.5f,  0.2f,  0.2f, "enormous" },
            { "towering",   0.0f,  0.0f,  0.0f,  0.4f,  0.2f,  0.1f, "imposing" },
            { "titanic",    0.0f,  0.0f,  0.0f,  0.5f,  0.2f,  0.2f, "immense" },
            { "wispy",      0.2f, -0.1f,  0.1f, -0.4f,  0.2f, -0.2f, "ethereal" },
            { "gossamer",   0.2f, -0.1f,  0.1f, -0.4f,  0.2f, -0.3f, "delicate" },
            { "feathery",   0.1f,  0.0f,  0.1f, -0.3f,  0.1f, -0.2f, "light" },
            { "airy",       0.2f,  0.0f,  0.1f, -0.1f,  0.4f, -0.1f, "floating" },
            { "featherweight",0.1f,0.0f,  0.0f, -0.3f,  0.1f, -0.2f, "ultralight" },
            { "weighty",    0.0f,  0.1f,  0.0f,  0.3f,  0.0f,  0.1f, "substantial" },
            { "substantial",0.0f,  0.1f,  0.0f,  0.3f,  0.0f,  0.0f, "solid" },
            { "immense",    0.0f,  0.0f,  0.0f,  0.5f,  0.2f,  0.1f, "enormous" },
            { "overwhelming",0.0f, 0.0f,  0.0f,  0.5f,  0.1f,  0.3f, "overpowering" },

            //================================================================
            // SPACE AXIS — reverb, stereo width, depth, distance
            //================================================================
            { "spacious",   0.0f,  0.0f,  0.0f,  0.0f,  0.5f,  0.0f, "reverberant" },
            { "vast",       0.0f,  0.0f,  0.1f,  0.0f,  0.6f,  0.0f, "ambient" },
            { "intimate",   0.0f,  0.1f,  0.0f,  0.0f, -0.4f,  0.0f, "close" },
            { "cavernous",  0.0f,  0.0f,  0.1f,  0.1f,  0.5f,  0.0f, "echoing" },
            { "dry",        0.0f,  0.0f,  0.0f,  0.0f, -0.4f,  0.0f, "direct" },
            { "wide",       0.0f,  0.0f,  0.0f,  0.0f,  0.4f,  0.0f, "stereo" },
            { "narrow",     0.0f,  0.0f,  0.0f,  0.0f, -0.3f,  0.0f, "mono" },
            { "expansive",  0.0f,  0.0f,  0.1f,  0.0f,  0.5f,  0.0f, "wide" },
            { "immersive",  0.0f,  0.0f,  0.1f,  0.1f,  0.5f,  0.0f, "surrounding" },
            { "enveloping", 0.0f,  0.1f,  0.1f,  0.1f,  0.5f, -0.1f, "surrounding" },
            { "panoramic",  0.0f,  0.0f,  0.0f,  0.0f,  0.5f,  0.0f, "ultra-wide" },
            { "claustrophobic",0.0f,0.0f, 0.0f,  0.2f, -0.5f,  0.1f, "confined" },
            { "confined",   0.0f,  0.0f,  0.0f,  0.1f, -0.4f,  0.0f, "small" },
            { "enclosed",   0.0f,  0.0f,  0.0f,  0.1f, -0.3f,  0.0f, "boxed" },
            { "open",       0.0f,  0.0f,  0.0f, -0.1f,  0.4f, -0.1f, "free" },
            { "distant",    0.0f,  0.0f,  0.0f, -0.1f,  0.4f, -0.1f, "faraway" },
            { "faraway",   -0.1f,  0.0f,  0.0f, -0.1f,  0.4f, -0.1f, "remote" },
            { "close",      0.0f,  0.1f,  0.0f,  0.0f, -0.3f,  0.0f, "near" },
            { "deep",      -0.1f,  0.1f,  0.0f,  0.1f,  0.3f,  0.0f, "profound" },
            { "shallow",    0.1f, -0.1f,  0.0f, -0.1f, -0.2f,  0.0f, "surface" },
            { "echoing",    0.0f,  0.0f,  0.1f,  0.0f,  0.4f, -0.1f, "reflective" },
            { "reverberant",0.0f,  0.0f,  0.1f,  0.1f,  0.5f, -0.1f, "echoing" },
            { "infinite",   0.0f,  0.0f,  0.1f,  0.0f,  0.6f, -0.1f, "boundless" },
            { "boundless",  0.0f,  0.0f,  0.1f,  0.0f,  0.5f, -0.1f, "limitless" },
            { "cathedral",  0.0f,  0.1f,  0.1f,  0.1f,  0.5f, -0.2f, "reverberant" },
            { "arena",      0.0f,  0.0f,  0.0f,  0.2f,  0.4f,  0.1f, "huge" },
            { "stadium",    0.0f,  0.0f,  0.0f,  0.2f,  0.5f,  0.1f, "massive" },
            { "wet",        0.0f,  0.0f,  0.0f,  0.0f,  0.4f,  0.0f, "reverberant" },
            { "drenched",   0.0f,  0.0f,  0.1f,  0.0f,  0.5f, -0.1f, "soaked" },
            { "soaked",     0.0f,  0.0f,  0.1f,  0.0f,  0.5f, -0.1f, "saturated" },
            { "submerged",  -0.2f, 0.1f,  0.1f,  0.1f,  0.4f, -0.2f, "underwater" },
            { "3d",         0.0f,  0.0f,  0.0f,  0.0f,  0.4f,  0.0f, "spatial" },

            //================================================================
            // AGGRESSION AXIS — distortion, edge, intensity
            //================================================================
            { "aggressive", 0.0f, -0.1f,  0.1f,  0.2f,  0.0f,  0.5f, "distorted" },
            { "gentle",     0.0f,  0.2f, -0.1f, -0.1f,  0.1f, -0.4f, "soft" },
            { "harsh",      0.1f, -0.2f,  0.0f,  0.0f,  0.0f,  0.5f, "abrasive" },
            { "smooth",     0.0f,  0.2f,  0.0f,  0.0f,  0.1f, -0.4f, "polished" },
            { "gritty",    -0.1f,  0.0f,  0.0f,  0.1f,  0.0f,  0.4f, "textured" },
            { "brutal",     0.0f, -0.2f,  0.1f,  0.3f,  0.0f,  0.6f, "crushing" },
            { "raw",       -0.1f,  0.0f,  0.0f,  0.1f, -0.1f,  0.3f, "unprocessed" },
            { "punchy",     0.0f,  0.0f,  0.1f,  0.2f, -0.1f,  0.3f, "transient" },
            { "fierce",     0.0f, -0.1f,  0.1f,  0.2f,  0.0f,  0.5f, "ferocious" },
            { "ferocious",  0.0f, -0.2f,  0.1f,  0.2f,  0.0f,  0.6f, "savage" },
            { "savage",     0.0f, -0.2f,  0.1f,  0.2f,  0.0f,  0.6f, "wild" },
            { "vicious",    0.0f, -0.2f,  0.1f,  0.2f,  0.0f,  0.5f, "nasty" },
            { "nasty",      0.0f, -0.1f,  0.0f,  0.1f,  0.0f,  0.4f, "dirty" },
            { "dirty",     -0.1f,  0.0f,  0.0f,  0.1f,  0.0f,  0.3f, "gritty" },
            { "filthy",    -0.1f, -0.1f,  0.0f,  0.1f,  0.0f,  0.4f, "grimy" },
            { "crunchy",    0.0f, -0.1f,  0.0f,  0.1f,  0.0f,  0.3f, "bitcrushed" },
            { "crushing",   0.0f, -0.1f,  0.0f,  0.3f,  0.0f,  0.5f, "compressed" },
            { "biting",     0.1f, -0.1f,  0.0f,  0.0f,  0.0f,  0.4f, "sharp" },
            { "razor",      0.2f, -0.2f,  0.0f,  0.0f,  0.0f,  0.4f, "cutting" },
            { "caustic",    0.1f, -0.2f,  0.0f,  0.0f,  0.0f,  0.5f, "corrosive" },
            { "corrosive",  0.0f, -0.2f,  0.1f,  0.1f,  0.0f,  0.5f, "eroding" },
            { "abrasive",   0.0f, -0.2f,  0.0f,  0.1f,  0.0f,  0.5f, "rough" },
            { "rough",     -0.1f, -0.1f,  0.0f,  0.1f,  0.0f,  0.3f, "unpolished" },
            { "jagged",     0.0f, -0.2f,  0.1f,  0.0f,  0.0f,  0.4f, "angular" },
            { "angular",    0.0f, -0.2f,  0.1f,  0.0f,  0.0f,  0.3f, "geometric" },
            { "mellow",     0.0f,  0.2f,  0.0f,  0.0f,  0.0f, -0.4f, "relaxed" },
            { "tender",     0.0f,  0.2f,  0.0f, -0.1f,  0.0f, -0.4f, "soft" },
            { "soft",       0.0f,  0.1f,  0.0f, -0.1f,  0.0f, -0.3f, "gentle" },
            { "silky",      0.1f,  0.2f,  0.0f,  0.0f,  0.0f, -0.3f, "smooth" },
            { "polished",   0.1f,  0.1f,  0.0f,  0.0f,  0.0f, -0.3f, "refined" },
            { "refined",    0.1f,  0.1f,  0.0f,  0.0f,  0.0f, -0.3f, "elegant" },
            { "elegant",    0.1f,  0.1f,  0.0f,  0.0f,  0.1f, -0.3f, "graceful" },
            { "clean",      0.1f,  0.0f,  0.0f,  0.0f,  0.0f, -0.3f, "pristine" },
            { "pristine",   0.2f,  0.0f,  0.0f,  0.0f,  0.0f, -0.3f, "immaculate" },
            { "searing",    0.1f, -0.2f,  0.0f,  0.1f,  0.0f,  0.5f, "burning" },
            { "scorching",  0.1f, -0.2f,  0.0f,  0.1f,  0.0f,  0.5f, "intense" },
            { "explosive",  0.0f, -0.1f,  0.1f,  0.3f,  0.1f,  0.6f, "detonating" },
            { "thunderous", -0.1f, 0.1f,  0.1f,  0.4f,  0.2f,  0.5f, "rumbling" },
            { "menacing",  -0.2f,  0.0f,  0.1f,  0.2f,  0.1f,  0.3f, "threatening" },
            { "tame",       0.0f,  0.1f,  0.0f, -0.1f,  0.0f, -0.3f, "restrained" },
            { "subdued",   -0.1f,  0.1f,  0.0f, -0.1f,  0.0f, -0.3f, "quiet" },

            //================================================================
            // COMPOUND QUALITIES — multi-dimensional descriptors
            //================================================================
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
            { "lo-fi",     -0.2f,  0.2f,  0.0f,  0.0f, -0.1f,  0.1f, "degraded" },
            { "hi-fi",      0.3f,  0.0f,  0.0f,  0.0f,  0.0f, -0.2f, "pristine" },
            { "underwater", -0.2f,  0.2f,  0.2f,  0.1f,  0.3f, -0.2f, "filtered" },
            { "celestial",  0.3f,  0.0f,  0.3f, -0.1f,  0.5f, -0.3f, "heavenly" },
            { "alien",     -0.1f, -0.2f,  0.3f,  0.0f,  0.3f,  0.1f, "otherworldly" },
            { "futuristic", 0.2f, -0.2f,  0.2f,  0.0f,  0.2f,  0.0f, "sci-fi" },
            { "sci-fi",     0.2f, -0.2f,  0.2f,  0.0f,  0.2f,  0.0f, "futuristic" },
            { "robotic",    0.1f, -0.3f,  0.2f,  0.0f,  0.0f,  0.1f, "mechanical" },
            { "mechanical", 0.0f, -0.2f,  0.2f,  0.1f, -0.1f,  0.2f, "clockwork" },
            { "tribal",    -0.1f,  0.2f,  0.3f,  0.2f, -0.1f,  0.2f, "primal" },
            { "primal",    -0.1f,  0.2f,  0.2f,  0.2f, -0.1f,  0.3f, "raw" },
            { "ancient",   -0.1f,  0.3f,  0.1f,  0.0f,  0.2f, -0.1f, "archaic" },
            { "medieval",  -0.1f,  0.2f,  0.0f,  0.1f,  0.2f,  0.0f, "archaic" },
            { "gothic",    -0.2f,  0.0f,  0.1f,  0.2f,  0.3f,  0.1f, "dark" },
            { "noir",      -0.2f,  0.1f,  0.1f,  0.0f,  0.2f,  0.0f, "moody" },
            { "neon",       0.4f, -0.2f,  0.2f,  0.0f,  0.0f,  0.1f, "electric" },
            { "electric",   0.3f, -0.2f,  0.2f,  0.0f,  0.0f,  0.2f, "charged" },
            { "psychedelic",0.1f,  0.0f,  0.4f,  0.1f,  0.3f,  0.0f, "trippy" },
            { "trippy",     0.1f,  0.0f,  0.4f,  0.0f,  0.3f,  0.0f, "hallucinatory" },
            { "hypnagogic", 0.0f,  0.1f,  0.3f,  0.0f,  0.3f, -0.2f, "half-asleep" },
            { "surreal",    0.0f,  0.0f,  0.3f,  0.0f,  0.3f,  0.0f, "dreamlike" },
            { "mystical",  -0.1f,  0.1f,  0.2f,  0.0f,  0.3f, -0.1f, "enchanted" },
            { "enchanted",  0.1f,  0.1f,  0.2f,  0.0f,  0.3f, -0.2f, "magical" },
            { "magical",    0.1f,  0.1f,  0.2f,  0.0f,  0.3f, -0.1f, "fantastical" },
            { "spooky",    -0.2f,  0.0f,  0.2f,  0.0f,  0.3f,  0.1f, "creepy" },
            { "creepy",    -0.2f, -0.1f,  0.2f,  0.0f,  0.2f,  0.1f, "unsettling" },
            { "sinister",  -0.3f, -0.1f,  0.1f,  0.2f,  0.1f,  0.3f, "dark" },
            { "demonic",   -0.3f, -0.2f,  0.2f,  0.3f,  0.0f,  0.5f, "hellish" },
            { "angelic",    0.3f,  0.1f,  0.1f, -0.1f,  0.4f, -0.4f, "heavenly" },
            { "heavenly",   0.2f,  0.1f,  0.1f, -0.1f,  0.4f, -0.3f, "divine" },
            { "divine",     0.2f,  0.1f,  0.1f,  0.0f,  0.3f, -0.3f, "transcendent" },
            { "sacred",     0.0f,  0.2f,  0.0f,  0.0f,  0.3f, -0.3f, "spiritual" },
            { "profane",   -0.1f, -0.1f,  0.1f,  0.1f,  0.0f,  0.3f, "dark" },
            { "gloomy",    -0.3f,  0.0f,  0.0f,  0.1f,  0.1f, -0.1f, "somber" },
            { "somber",    -0.2f,  0.1f,  0.0f,  0.0f,  0.1f, -0.1f, "grave" },
            { "brooding",  -0.2f,  0.0f,  0.1f,  0.2f,  0.1f,  0.1f, "dark" },
            { "sultry",     0.0f,  0.3f,  0.1f,  0.1f,  0.0f, -0.1f, "sensual" },
            { "seductive",  0.0f,  0.3f,  0.1f,  0.0f,  0.0f, -0.1f, "alluring" },
            { "soothing",   0.0f,  0.2f,  0.0f, -0.1f,  0.2f, -0.4f, "calming" },
            { "calming",    0.0f,  0.2f,  0.0f, -0.1f,  0.2f, -0.4f, "peaceful" },
            { "meditative", 0.0f,  0.1f,  0.1f, -0.1f,  0.3f, -0.4f, "zen" },
            { "zen",        0.0f,  0.1f,  0.0f, -0.2f,  0.2f, -0.4f, "minimal" },
            { "chaotic",    0.0f, -0.1f,  0.5f,  0.2f,  0.0f,  0.3f, "disordered" },
            { "glitchy",    0.1f, -0.2f,  0.3f,  0.0f,  0.0f,  0.2f, "broken" },
            { "broken",    -0.1f, -0.1f,  0.2f,  0.0f,  0.0f,  0.2f, "damaged" },
            { "corrupted", -0.1f, -0.2f,  0.2f,  0.0f,  0.0f,  0.3f, "degraded" },
            { "mangled",   -0.1f, -0.2f,  0.2f,  0.1f,  0.0f,  0.4f, "destroyed" },
            { "detuned",    0.0f,  0.0f,  0.2f,  0.1f,  0.0f,  0.0f, "wobbly" },
            { "dissonant",  0.0f, -0.2f,  0.1f,  0.1f,  0.0f,  0.3f, "clashing" },
            { "consonant",  0.0f,  0.2f,  0.0f,  0.0f,  0.0f, -0.3f, "harmonious" },
            { "harmoniou",  0.0f,  0.2f,  0.0f,  0.1f,  0.1f, -0.3f, "balanced" },
            { "balanced",   0.0f,  0.1f,  0.0f,  0.0f,  0.0f, -0.1f, "even" },
            { "unbalanced", 0.0f, -0.1f,  0.1f,  0.0f,  0.0f,  0.1f, "asymmetric" },
            { "wonky",      0.0f,  0.0f,  0.2f,  0.0f,  0.0f,  0.1f, "quirky" },
            { "quirky",     0.1f,  0.0f,  0.2f,  0.0f,  0.0f,  0.0f, "playful" },

            //================================================================
            // GENRE/ERA descriptors — common user vocabulary
            //================================================================
            { "synthwave",  0.1f,  0.2f,  0.2f,  0.1f,  0.2f,  0.0f, "retro-80s" },
            { "vaporwave", -0.1f,  0.2f,  0.1f,  0.0f,  0.3f, -0.2f, "nostalgic" },
            { "cyberpunk",  0.1f, -0.2f,  0.2f,  0.1f,  0.1f,  0.3f, "dystopian" },
            { "steampunk", -0.1f,  0.2f,  0.2f,  0.1f,  0.0f,  0.1f, "mechanical" },
            { "lofi",      -0.2f,  0.2f,  0.0f,  0.0f, -0.1f,  0.0f, "degraded" },
            { "trap",       0.0f, -0.1f,  0.2f,  0.2f, -0.1f,  0.3f, "808" },
            { "dubstep",   -0.1f, -0.1f,  0.3f,  0.3f,  0.0f,  0.5f, "wobbly" },
            { "dnb",        0.1f, -0.1f,  0.4f,  0.2f,  0.0f,  0.3f, "jungle" },
            { "jungle",     0.0f,  0.0f,  0.4f,  0.2f,  0.0f,  0.3f, "breakbeat" },
            { "dub",       -0.2f,  0.2f,  0.2f,  0.1f,  0.4f, -0.1f, "reggae" },
            { "shoegaze",   0.0f,  0.2f,  0.2f,  0.3f,  0.5f, -0.1f, "layered" },
            { "noise",     -0.1f, -0.2f,  0.2f,  0.3f,  0.0f,  0.5f, "chaotic" },
            { "post-rock",  0.0f,  0.1f,  0.3f,  0.2f,  0.4f,  0.0f, "cinematic" },
            { "chillwave",  0.1f,  0.2f,  0.2f,  0.0f,  0.3f, -0.3f, "hazy" },
            { "downtempo",  0.0f,  0.2f,  0.1f,  0.0f,  0.2f, -0.2f, "slow" },
            { "uptempo",    0.1f,  0.0f,  0.3f,  0.1f,  0.0f,  0.1f, "fast" },

            //================================================================
            // FOOD/TASTE synesthesia — surprisingly common in user descriptions
            //================================================================
            { "sweet",      0.1f,  0.3f,  0.0f,  0.0f,  0.0f, -0.3f, "pleasant" },
            { "bitter",    -0.1f, -0.2f,  0.0f,  0.0f,  0.0f,  0.2f, "harsh" },
            { "sour",       0.1f, -0.2f,  0.0f,  0.0f,  0.0f,  0.2f, "acidic" },
            { "spicy",      0.1f,  0.0f,  0.1f,  0.0f,  0.0f,  0.3f, "hot" },
            { "salty",      0.0f,  0.0f,  0.0f,  0.1f,  0.0f,  0.1f, "coarse" },
            { "tangy",      0.2f, -0.1f,  0.1f,  0.0f,  0.0f,  0.1f, "zesty" },
            { "savory",    -0.1f,  0.3f,  0.0f,  0.2f,  0.0f,  0.0f, "rich" },
            { "umami",     -0.1f,  0.3f,  0.0f,  0.2f,  0.0f, -0.1f, "deep" },
            { "crunchy",    0.1f, -0.1f,  0.1f,  0.0f,  0.0f,  0.2f, "textured" },
            { "fizzy",      0.3f, -0.1f,  0.3f, -0.1f,  0.0f,  0.0f, "effervescent" },
            { "bubbly",     0.3f,  0.0f,  0.3f, -0.1f,  0.0f, -0.1f, "sparkling" },

            //================================================================
            // TEMPO/RHYTHM descriptors
            //================================================================
            { "fast",       0.0f,  0.0f,  0.4f,  0.0f,  0.0f,  0.1f, "rapid" },
            { "slow",       0.0f,  0.1f,  0.1f,  0.0f,  0.1f, -0.1f, "gradual" },
            { "groovy",     0.0f,  0.1f,  0.3f,  0.1f,  0.0f,  0.0f, "funky" },
            { "funky",      0.0f,  0.1f,  0.3f,  0.1f,  0.0f,  0.1f, "rhythmic" },
            { "swung",      0.0f,  0.1f,  0.2f,  0.0f,  0.0f,  0.0f, "shuffle" },
            { "syncopat",   0.0f,  0.0f,  0.3f,  0.0f,  0.0f,  0.0f, "offbeat" },
            { "staccato",   0.1f,  0.0f,  0.2f,  0.0f,  0.0f,  0.1f, "short" },
            { "legato",     0.0f,  0.2f,  0.0f,  0.0f,  0.1f, -0.1f, "sustained" },
            { "bouncy",     0.1f,  0.0f,  0.3f,  0.0f,  0.0f,  0.0f, "playful" },
            { "driving",    0.0f,  0.0f,  0.3f,  0.2f,  0.0f,  0.2f, "propulsive" },
            { "plodding",  -0.1f,  0.0f,  0.1f,  0.2f,  0.0f,  0.0f, "heavy" },
            { "galloping",  0.0f,  0.0f,  0.4f,  0.1f,  0.0f,  0.1f, "rhythmic" },
            { "marching",   0.0f,  0.0f,  0.3f,  0.2f,  0.0f,  0.1f, "regimented" },

            //================================================================
            // INSTRUMENT references — users often describe by instrument character
            //================================================================
            { "piano",      0.1f,  0.2f,  0.0f,  0.0f,  0.1f, -0.1f, "acoustic" },
            { "guitar",     0.0f,  0.2f,  0.0f,  0.0f,  0.0f,  0.0f, "stringed" },
            { "violin",     0.2f,  0.1f,  0.1f,  0.0f,  0.1f, -0.1f, "bowed" },
            { "cello",     -0.1f,  0.3f,  0.1f,  0.2f,  0.1f, -0.1f, "deep-bowed" },
            { "flute",      0.4f,  0.0f,  0.0f, -0.2f,  0.1f, -0.2f, "breathy" },
            { "trumpet",    0.2f,  0.0f,  0.0f,  0.1f,  0.0f,  0.2f, "brassy" },
            { "saxophone",  0.0f,  0.3f,  0.1f,  0.0f,  0.0f,  0.0f, "reedy" },
            { "harp",       0.3f,  0.1f,  0.1f, -0.1f,  0.2f, -0.3f, "plucked" },
            { "organ",     -0.1f,  0.2f,  0.0f,  0.3f,  0.2f, -0.1f, "tonewheel" },
            { "choir",      0.0f,  0.2f,  0.1f,  0.3f,  0.3f, -0.2f, "vocal" },
            { "vocal",      0.0f,  0.2f,  0.1f,  0.0f,  0.1f, -0.1f, "voice" },
            { "voice",      0.0f,  0.2f,  0.1f,  0.0f,  0.1f, -0.1f, "human" },
            { "horn",       0.0f,  0.1f,  0.0f,  0.1f,  0.0f,  0.1f, "brassy" },
            { "marimba",    0.2f,  0.2f,  0.0f,  0.0f,  0.0f, -0.1f, "mallet" },
            { "kalimba",    0.3f,  0.1f,  0.0f, -0.1f,  0.1f, -0.2f, "thumb-piano" },
            { "tabla",      0.0f,  0.1f,  0.2f,  0.0f,  0.0f,  0.1f, "percussive" },
            { "gamelan",    0.3f,  0.0f,  0.2f,  0.1f,  0.2f, -0.1f, "metallic" },
            { "sitar",      0.1f,  0.1f,  0.2f,  0.0f,  0.1f,  0.0f, "resonant" },
            { "didgeridoo",-0.2f,  0.3f,  0.2f,  0.2f,  0.1f,  0.0f, "droning" },
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
            { "red",       -0.1f,  0.2f,  0.1f,  0.3f,  0.0f,  0.5f, "Foundation", "aggressive" },
            { "blue",       0.1f, -0.1f,  0.2f,  0.0f,  0.4f, -0.3f, "Atmosphere", "cool" },
            { "yellow",     0.5f,  0.2f,  0.2f,  0.0f,  0.1f,  0.0f, "Prism", "bright" },
            { "green",      0.0f,  0.3f,  0.2f,  0.1f,  0.2f, -0.2f, "Atmosphere", "organic" },
            { "purple",     0.0f,  0.1f,  0.3f,  0.2f,  0.3f,  0.0f, "Aether", "mystical" },
            { "orange",     0.2f,  0.3f,  0.2f,  0.2f,  0.0f,  0.2f, "Flux", "warm" },
            { "pink",       0.3f,  0.2f,  0.1f, -0.1f,  0.1f, -0.2f, "Prism", "soft" },
            { "black",     -0.4f,  0.0f,  0.1f,  0.3f,  0.0f,  0.3f, "Foundation", "deep" },
            { "white",      0.5f, -0.2f,  0.0f, -0.2f,  0.3f, -0.3f, "Prism", "pure" },
            { "gold",       0.3f,  0.4f,  0.1f,  0.2f,  0.1f,  0.0f, "Aether", "rich" },
            { "silver",     0.4f, -0.1f,  0.1f,  0.0f,  0.2f, -0.1f, "Prism", "metallic" },
            { "neon",       0.5f, -0.2f,  0.3f,  0.0f,  0.0f,  0.2f, "Flux", "electric" },

            // Extended palette
            { "crimson",   -0.1f,  0.2f,  0.1f,  0.3f,  0.0f,  0.6f, "Foundation", "blood" },
            { "scarlet",   -0.1f,  0.2f,  0.1f,  0.2f,  0.0f,  0.5f, "Foundation", "fierce" },
            { "magenta",    0.2f, -0.1f,  0.2f,  0.0f,  0.1f,  0.1f, "Prism", "vivid" },
            { "cyan",       0.4f, -0.2f,  0.2f,  0.0f,  0.3f, -0.2f, "Prism", "digital" },
            { "turquoise",  0.3f, -0.1f,  0.2f,  0.0f,  0.3f, -0.2f, "Atmosphere", "tropical" },
            { "teal",       0.1f,  0.0f,  0.2f,  0.1f,  0.3f, -0.1f, "Atmosphere", "oceanic" },
            { "indigo",    -0.1f,  0.0f,  0.2f,  0.2f,  0.3f,  0.0f, "Aether", "deep" },
            { "violet",     0.1f,  0.0f,  0.3f,  0.1f,  0.3f, -0.1f, "Aether", "psychedelic" },
            { "coral",      0.2f,  0.3f,  0.1f,  0.0f,  0.0f, -0.1f, "Prism", "tropical" },
            { "amber",      0.1f,  0.4f,  0.1f,  0.1f,  0.0f,  0.0f, "Foundation", "resinous" },
            { "maroon",    -0.2f,  0.2f,  0.0f,  0.2f,  0.0f,  0.2f, "Foundation", "vintage" },
            { "ivory",      0.4f,  0.2f,  0.0f, -0.1f,  0.2f, -0.3f, "Prism", "antique" },
            { "charcoal",  -0.3f,  0.0f,  0.0f,  0.2f,  0.0f,  0.1f, "Foundation", "smoked" },
            { "grey",      -0.1f, -0.1f,  0.0f,  0.0f,  0.1f,  0.0f, "Atmosphere", "neutral" },
            { "gray",      -0.1f, -0.1f,  0.0f,  0.0f,  0.1f,  0.0f, "Atmosphere", "neutral" },
            { "lavender",   0.2f,  0.1f,  0.1f, -0.1f,  0.3f, -0.3f, "Atmosphere", "delicate" },
            { "periwinkle", 0.2f,  0.0f,  0.1f, -0.1f,  0.3f, -0.2f, "Atmosphere", "pastel" },
            { "rose",       0.1f,  0.3f,  0.0f,  0.0f,  0.1f, -0.2f, "Atmosphere", "romantic" },
            { "burgundy",  -0.2f,  0.3f,  0.0f,  0.2f,  0.0f,  0.1f, "Foundation", "rich" },
            { "olive",     -0.1f,  0.2f,  0.0f,  0.1f,  0.0f, -0.1f, "Atmosphere", "earthy" },
            { "emerald",    0.1f,  0.2f,  0.1f,  0.1f,  0.2f, -0.1f, "Prism", "jeweled" },
            { "sapphire",   0.2f, -0.1f,  0.1f,  0.1f,  0.3f, -0.1f, "Aether", "jeweled" },
            { "ruby",       0.0f,  0.2f,  0.1f,  0.2f,  0.0f,  0.3f, "Foundation", "jeweled" },
            { "copper",    -0.1f,  0.3f,  0.0f,  0.1f,  0.0f,  0.1f, "Foundation", "oxidized" },
            { "bronze",    -0.1f,  0.3f,  0.0f,  0.2f,  0.0f,  0.0f, "Foundation", "ancient" },
            { "pastel",     0.3f,  0.1f,  0.0f, -0.2f,  0.2f, -0.4f, "Prism", "soft" },
            { "iridescent", 0.3f,  0.0f,  0.3f,  0.0f,  0.2f, -0.1f, "Prism", "shifting" },
            { "holographic",0.3f, -0.1f,  0.4f,  0.0f,  0.2f,  0.0f, "Flux", "prismatic" },
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
            // Water phenomena
            { "ocean",     0.0f,  0.1f,  0.4f,  0.3f,  0.5f, -0.1f, "Atmosphere", "waves" },
            { "rain",      0.2f,  0.0f,  0.3f,  0.2f,  0.3f, -0.2f, "Atmosphere", "granular" },
            { "waterfall", 0.1f,  0.1f,  0.3f,  0.3f,  0.4f,  0.0f, "Flux", "cascading" },
            { "river",     0.0f,  0.1f,  0.3f,  0.1f,  0.3f, -0.1f, "Flux", "flowing" },
            { "stream",    0.2f,  0.1f,  0.2f, -0.1f,  0.2f, -0.2f, "Atmosphere", "trickling" },
            { "wave",      0.0f,  0.1f,  0.4f,  0.2f,  0.4f, -0.1f, "Flux", "surging" },
            { "tide",      0.0f,  0.1f,  0.3f,  0.2f,  0.3f, -0.1f, "Flux", "cyclical" },
            { "pond",      0.0f,  0.2f,  0.0f, -0.1f,  0.2f, -0.3f, "Atmosphere", "still" },
            { "lake",      0.0f,  0.1f,  0.1f,  0.0f,  0.3f, -0.2f, "Atmosphere", "placid" },
            { "swamp",    -0.2f,  0.2f,  0.1f,  0.2f,  0.1f,  0.1f, "Entangled", "murky" },
            { "marsh",    -0.2f,  0.2f,  0.1f,  0.1f,  0.1f,  0.0f, "Atmosphere", "damp" },
            { "mist",      0.1f,  0.0f,  0.1f, -0.1f,  0.4f, -0.3f, "Atmosphere", "hazy" },
            { "fog",      -0.1f,  0.0f,  0.1f,  0.0f,  0.3f, -0.2f, "Atmosphere", "diffused" },
            { "drizzle",   0.1f,  0.0f,  0.2f, -0.1f,  0.2f, -0.2f, "Atmosphere", "fine" },
            { "tsunami",  -0.1f,  0.0f,  0.4f,  0.5f,  0.3f,  0.7f, "Entangled", "overwhelming" },

            // Weather & atmosphere
            { "thunder",  -0.2f,  0.1f,  0.1f,  0.5f,  0.3f,  0.6f, "Foundation", "rumbling" },
            { "storm",    -0.1f,  0.0f,  0.4f,  0.4f,  0.2f,  0.5f, "Entangled", "chaotic" },
            { "wind",      0.1f,  0.0f,  0.4f, -0.1f,  0.3f, -0.1f, "Flux", "breathy" },
            { "lightning",  0.5f, -0.2f,  0.3f,  0.1f,  0.1f,  0.6f, "Entangled", "electric" },
            { "blizzard", -0.1f, -0.4f,  0.4f,  0.3f,  0.2f,  0.4f, "Entangled", "frozen" },
            { "hurricane",-0.1f,  0.0f,  0.5f,  0.4f,  0.2f,  0.6f, "Entangled", "violent" },
            { "tornado",   0.0f, -0.1f,  0.5f,  0.3f,  0.1f,  0.6f, "Entangled", "spinning" },
            { "breeze",    0.2f,  0.1f,  0.2f, -0.2f,  0.3f, -0.3f, "Atmosphere", "gentle" },
            { "rainbow",   0.4f,  0.1f,  0.2f,  0.0f,  0.3f, -0.2f, "Prism", "prismatic" },
            { "aurora",    0.3f,  0.0f,  0.4f,  0.0f,  0.5f, -0.2f, "Aether", "shimmering" },

            // Earth & terrain
            { "fire",      0.1f,  0.2f,  0.3f,  0.2f,  0.0f,  0.5f, "Flux", "crackling" },
            { "ice",       0.4f, -0.4f,  0.0f,  0.1f,  0.2f,  0.0f, "Prism", "crystalline" },
            { "forest",    0.0f,  0.3f,  0.2f,  0.2f,  0.3f, -0.2f, "Atmosphere", "organic" },
            { "desert",    0.1f,  0.2f,  0.1f, -0.2f,  0.4f,  0.0f, "Aether", "dry" },
            { "volcano",  -0.2f,  0.2f,  0.2f,  0.4f,  0.1f,  0.7f, "Foundation", "explosive" },
            { "crystal",   0.5f, -0.1f,  0.1f,  0.0f,  0.2f, -0.1f, "Prism", "glassy" },
            { "earthquake",-0.3f,  0.1f,  0.3f,  0.5f,  0.0f,  0.6f, "Foundation", "seismic" },
            { "mountain",  -0.1f,  0.1f,  0.0f,  0.4f,  0.3f,  0.0f, "Foundation", "massive" },
            { "canyon",    -0.1f,  0.1f,  0.1f,  0.0f,  0.5f,  0.0f, "Atmosphere", "echoing" },
            { "cave",      -0.2f,  0.1f,  0.1f,  0.1f,  0.4f,  0.0f, "Atmosphere", "resonant" },
            { "glacier",   0.2f, -0.4f,  0.0f,  0.3f,  0.3f, -0.1f, "Prism", "frozen" },
            { "meadow",    0.2f,  0.3f,  0.1f, -0.1f,  0.3f, -0.3f, "Atmosphere", "pastoral" },
            { "jungle",   -0.1f,  0.2f,  0.3f,  0.3f,  0.2f,  0.1f, "Entangled", "dense" },
            { "coral reef",0.2f,  0.1f,  0.2f,  0.2f,  0.3f, -0.1f, "Prism", "aquatic" },
            { "lava",      -0.2f,  0.3f,  0.2f,  0.4f,  0.0f,  0.6f, "Foundation", "molten" },
            { "ember",     -0.1f,  0.3f,  0.1f,  0.0f,  0.0f,  0.1f, "Foundation", "smoldering" },

            // Celestial & time
            { "space",     0.1f, -0.1f,  0.2f, -0.1f,  0.6f, -0.2f, "Aether", "cosmic" },
            { "cosmos",    0.2f, -0.1f,  0.3f,  0.1f,  0.6f, -0.2f, "Aether", "vast" },
            { "sunrise",   0.3f,  0.3f,  0.2f,  0.0f,  0.3f, -0.2f, "Prism", "glowing" },
            { "sunset",    0.0f,  0.4f,  0.2f,  0.1f,  0.3f, -0.1f, "Atmosphere", "fading" },
            { "midnight", -0.3f,  0.1f,  0.1f,  0.1f,  0.2f,  0.0f, "Aether", "nocturnal" },
            { "moonlight", 0.2f,  0.0f,  0.1f, -0.1f,  0.3f, -0.2f, "Aether", "luminous" },
            { "twilight", -0.1f,  0.2f,  0.1f,  0.0f,  0.3f, -0.1f, "Atmosphere", "liminal" },
            { "dawn",      0.3f,  0.2f,  0.1f, -0.1f,  0.3f, -0.2f, "Prism", "emerging" },
            { "dusk",     -0.1f,  0.2f,  0.1f,  0.0f,  0.2f, -0.1f, "Atmosphere", "fading" },
            { "nebula",    0.1f,  0.0f,  0.3f,  0.1f,  0.6f, -0.1f, "Aether", "gaseous" },
            { "comet",     0.3f, -0.1f,  0.4f,  0.0f,  0.4f,  0.1f, "Flux", "streaking" },
            { "star",      0.4f,  0.0f,  0.1f,  0.0f,  0.4f, -0.1f, "Aether", "twinkling" },
            { "supernova", 0.5f,  0.0f,  0.3f,  0.5f,  0.3f,  0.6f, "Entangled", "exploding" },
            { "eclipse",  -0.3f,  0.0f,  0.2f,  0.2f,  0.2f,  0.1f, "Aether", "darkening" },
            { "meteor",    0.2f, -0.1f,  0.4f,  0.1f,  0.2f,  0.3f, "Flux", "burning" },

            // Plant & organic
            { "flower",    0.2f,  0.2f,  0.0f, -0.1f,  0.1f, -0.3f, "Prism", "blooming" },
            { "vine",      0.0f,  0.2f,  0.2f,  0.1f,  0.0f, -0.1f, "Flux", "creeping" },
            { "moss",     -0.1f,  0.3f,  0.0f,  0.0f,  0.1f, -0.2f, "Atmosphere", "soft" },
            { "petal",     0.2f,  0.2f,  0.0f, -0.2f,  0.1f, -0.4f, "Prism", "fragile" },
            { "thorn",     0.1f, -0.1f,  0.0f,  0.0f,  0.0f,  0.4f, "Foundation", "sharp" },
        };

        // Emotion → sound mappings
        struct EmotionMapping
        {
            const char* word;
            float br, wa, mv, de, sp, ag;
            const char* mood;
        };

        static const EmotionMapping emotions[] = {
            // Positive / uplifting
            { "happy",      0.3f,  0.2f,  0.2f,  0.0f,  0.1f, -0.2f, "Prism" },
            { "joyful",     0.4f,  0.2f,  0.3f,  0.0f,  0.1f, -0.1f, "Prism" },
            { "euphoric",   0.3f,  0.1f,  0.4f,  0.2f,  0.2f,  0.1f, "Aether" },
            { "ecstatic",   0.4f,  0.1f,  0.5f,  0.2f,  0.2f,  0.2f, "Aether" },
            { "blissful",   0.2f,  0.3f,  0.1f,  0.0f,  0.3f, -0.4f, "Atmosphere" },
            { "elated",     0.3f,  0.1f,  0.3f,  0.1f,  0.2f,  0.0f, "Prism" },
            { "playful",    0.3f,  0.1f,  0.3f, -0.1f,  0.0f, -0.1f, "Prism" },
            { "hopeful",    0.2f,  0.2f,  0.1f,  0.0f,  0.2f, -0.2f, "Prism" },
            { "triumphant", 0.1f,  0.1f,  0.2f,  0.4f,  0.3f,  0.3f, "Foundation" },
            { "proud",      0.1f,  0.1f,  0.1f,  0.3f,  0.2f,  0.1f, "Foundation" },
            { "exhilarat",  0.3f,  0.0f,  0.4f,  0.1f,  0.2f,  0.2f, "Flux" },

            // Calm / peaceful
            { "peaceful",   0.1f,  0.3f,  0.0f, -0.1f,  0.3f, -0.5f, "Atmosphere" },
            { "serene",     0.1f,  0.2f,  0.0f, -0.1f,  0.3f, -0.4f, "Atmosphere" },
            { "tranquil",   0.0f,  0.2f,  0.0f, -0.1f,  0.3f, -0.4f, "Atmosphere" },
            { "content",    0.1f,  0.2f,  0.0f,  0.0f,  0.1f, -0.3f, "Atmosphere" },
            { "soothed",    0.0f,  0.3f,  0.0f, -0.1f,  0.2f, -0.4f, "Atmosphere" },
            { "meditative", 0.0f,  0.1f,  0.1f, -0.1f,  0.3f, -0.4f, "Aether" },
            { "zen",        0.0f,  0.1f,  0.0f, -0.2f,  0.2f, -0.4f, "Aether" },

            // Sad / dark
            { "sad",       -0.2f,  0.1f,  0.0f,  0.0f,  0.3f, -0.3f, "Atmosphere" },
            { "melanchol",  0.0f,  0.2f,  0.1f,  0.0f,  0.3f, -0.2f, "Atmosphere" },
            { "lonely",    -0.1f,  0.0f,  0.0f, -0.3f,  0.4f, -0.2f, "Atmosphere" },
            { "wistful",    0.0f,  0.2f,  0.1f, -0.1f,  0.3f, -0.2f, "Atmosphere" },
            { "bittersweet",0.0f,  0.2f,  0.1f,  0.0f,  0.2f, -0.1f, "Atmosphere" },
            { "grief",     -0.2f,  0.1f,  0.0f,  0.1f,  0.3f, -0.2f, "Atmosphere" },
            { "sorrow",    -0.2f,  0.1f,  0.0f,  0.1f,  0.3f, -0.2f, "Atmosphere" },
            { "heartbreak",-0.1f,  0.1f,  0.0f,  0.0f,  0.3f, -0.1f, "Atmosphere" },
            { "desolat",   -0.2f, -0.1f,  0.0f, -0.2f,  0.4f, -0.1f, "Atmosphere" },
            { "gloomy",    -0.3f,  0.0f,  0.0f,  0.1f,  0.1f, -0.1f, "Atmosphere" },
            { "despair",   -0.3f, -0.1f,  0.0f,  0.1f,  0.2f,  0.0f, "Entangled" },
            { "yearn",      0.0f,  0.2f,  0.1f,  0.0f,  0.3f, -0.1f, "Atmosphere" },

            // Nostalgia / memory
            { "nostalg",   -0.1f,  0.4f,  0.1f,  0.0f,  0.2f, -0.2f, "Atmosphere" },
            { "rememb",    -0.1f,  0.3f,  0.0f,  0.0f,  0.2f, -0.2f, "Atmosphere" },
            { "longing",   -0.1f,  0.3f,  0.1f,  0.0f,  0.3f, -0.1f, "Atmosphere" },
            { "homesick",  -0.1f,  0.3f,  0.0f, -0.1f,  0.2f, -0.2f, "Atmosphere" },
            { "sentimental",0.0f,  0.3f,  0.0f,  0.0f,  0.2f, -0.2f, "Atmosphere" },

            // Tense / anxious / dark
            { "angry",     -0.1f, -0.1f,  0.2f,  0.3f,  0.0f,  0.6f, "Foundation" },
            { "furious",   -0.1f, -0.2f,  0.3f,  0.3f,  0.0f,  0.7f, "Foundation" },
            { "rage",      -0.1f, -0.2f,  0.3f,  0.4f,  0.0f,  0.8f, "Foundation" },
            { "anxious",    0.1f, -0.1f,  0.3f,  0.1f,  0.0f,  0.2f, "Entangled" },
            { "tense",      0.0f, -0.1f,  0.2f,  0.2f, -0.1f,  0.3f, "Entangled" },
            { "nervous",    0.1f, -0.1f,  0.3f,  0.0f,  0.0f,  0.1f, "Entangled" },
            { "dread",     -0.3f, -0.1f,  0.1f,  0.2f,  0.2f,  0.3f, "Entangled" },
            { "fearful",   -0.1f, -0.1f,  0.2f,  0.1f,  0.1f,  0.2f, "Entangled" },
            { "paranoid",   0.0f, -0.2f,  0.3f,  0.1f,  0.0f,  0.3f, "Entangled" },
            { "restless",   0.0f,  0.0f,  0.3f,  0.0f,  0.0f,  0.1f, "Flux" },
            { "ominous",   -0.3f,  0.0f,  0.1f,  0.3f,  0.2f,  0.2f, "Entangled" },
            { "menacing",  -0.2f, -0.1f,  0.1f,  0.2f,  0.1f,  0.3f, "Entangled" },

            // Wonder / mystery
            { "mysterious",-0.2f,  0.0f,  0.2f,  0.1f,  0.3f,  0.0f, "Aether" },
            { "eerie",     -0.1f, -0.1f,  0.2f,  0.0f,  0.4f,  0.1f, "Aether" },
            { "awe",        0.1f,  0.1f,  0.2f,  0.2f,  0.4f, -0.1f, "Aether" },
            { "wonder",     0.2f,  0.1f,  0.2f,  0.0f,  0.4f, -0.1f, "Aether" },
            { "majestic",   0.1f,  0.1f,  0.1f,  0.3f,  0.4f,  0.0f, "Aether" },
            { "transcend",  0.1f,  0.0f,  0.3f,  0.0f,  0.5f, -0.2f, "Aether" },
            { "sublime",    0.1f,  0.1f,  0.2f,  0.2f,  0.4f, -0.1f, "Aether" },
            { "enchant",    0.1f,  0.1f,  0.2f,  0.0f,  0.3f, -0.2f, "Prism" },
            { "bewitch",   -0.1f,  0.1f,  0.2f,  0.1f,  0.2f,  0.0f, "Aether" },
            { "haunting",  -0.1f,  0.0f,  0.2f,  0.0f,  0.4f, -0.1f, "Aether" },

            // Romantic / sensual
            { "romantic",   0.0f,  0.3f,  0.1f,  0.0f,  0.2f, -0.2f, "Atmosphere" },
            { "sensual",    0.0f,  0.3f,  0.1f,  0.1f,  0.0f, -0.1f, "Atmosphere" },
            { "seductive",  0.0f,  0.3f,  0.1f,  0.0f,  0.0f, -0.1f, "Atmosphere" },
            { "sultry",     0.0f,  0.3f,  0.1f,  0.1f,  0.0f, -0.1f, "Atmosphere" },
            { "passionate",-0.1f,  0.2f,  0.2f,  0.2f,  0.0f,  0.2f, "Flux" },
            { "tender",     0.0f,  0.3f,  0.0f, -0.1f,  0.1f, -0.4f, "Atmosphere" },
            { "affection",  0.1f,  0.3f,  0.0f,  0.0f,  0.1f, -0.3f, "Atmosphere" },
        };

        // Material textures
        struct MaterialMapping
        {
            const char* word;
            float br, wa, mv, de, sp, ag;
            const char* texture;
        };

        static const MaterialMapping materials[] = {
            // Transparent / fragile
            { "glass",      0.5f, -0.1f,  0.0f, -0.1f,  0.2f, -0.1f, "glassy" },
            { "crystal",    0.5f, -0.1f,  0.1f,  0.0f,  0.2f, -0.1f, "crystalline" },
            { "diamond",    0.5f, -0.2f,  0.0f,  0.1f,  0.2f,  0.0f, "brilliant" },
            { "porcelain",  0.3f, -0.1f,  0.0f, -0.1f,  0.1f, -0.2f, "delicate" },
            { "ceramic",    0.2f, -0.1f,  0.0f,  0.0f,  0.1f,  0.0f, "resonant" },

            // Metals
            { "metal",      0.2f, -0.2f,  0.1f,  0.2f,  0.1f,  0.3f, "metallic" },
            { "steel",      0.3f, -0.3f,  0.0f,  0.2f,  0.1f,  0.3f, "sharp" },
            { "iron",      -0.1f, -0.2f,  0.0f,  0.3f,  0.0f,  0.3f, "industrial" },
            { "copper",     0.0f,  0.2f,  0.0f,  0.1f,  0.1f,  0.0f, "warm-metal" },
            { "bronze",    -0.1f,  0.2f,  0.0f,  0.2f,  0.1f,  0.0f, "ancient" },
            { "titanium",   0.2f, -0.3f,  0.0f,  0.3f,  0.0f,  0.2f, "ultra-hard" },
            { "mercury",    0.2f, -0.2f,  0.3f,  0.1f,  0.0f,  0.0f, "liquid-metal" },
            { "gold",       0.2f,  0.3f,  0.0f,  0.2f,  0.1f,  0.0f, "rich" },
            { "platinum",   0.3f, -0.1f,  0.0f,  0.1f,  0.1f,  0.0f, "pristine" },
            { "tin",        0.3f, -0.2f,  0.0f, -0.1f,  0.0f,  0.1f, "tinny" },

            // Organic / natural
            { "wood",      -0.1f,  0.4f,  0.0f,  0.1f,  0.1f, -0.1f, "woody" },
            { "bone",      -0.1f, -0.1f,  0.0f,  0.1f,  0.0f,  0.1f, "skeletal" },
            { "leather",   -0.1f,  0.3f,  0.0f,  0.1f,  0.0f,  0.0f, "supple" },
            { "paper",      0.1f, -0.1f,  0.0f, -0.2f,  0.0f, -0.1f, "thin" },
            { "bamboo",     0.2f,  0.2f,  0.0f, -0.1f,  0.0f, -0.1f, "hollow" },
            { "coral",      0.1f,  0.1f,  0.0f,  0.1f,  0.1f, -0.1f, "textured" },
            { "shell",      0.2f,  0.0f,  0.0f,  0.0f,  0.2f, -0.1f, "pearlescent" },
            { "amber",      0.1f,  0.3f,  0.0f,  0.0f,  0.0f, -0.1f, "resinous" },
            { "ivory",      0.3f,  0.2f,  0.0f,  0.0f,  0.1f, -0.2f, "antique" },
            { "wax",       -0.1f,  0.2f,  0.0f,  0.0f,  0.0f, -0.2f, "muted" },

            // Soft / fabric
            { "silk",       0.1f,  0.2f,  0.1f, -0.1f,  0.0f, -0.3f, "silky" },
            { "velvet",     0.0f,  0.4f,  0.0f,  0.2f,  0.0f, -0.3f, "plush" },
            { "cotton",     0.0f,  0.3f,  0.0f, -0.1f,  0.0f, -0.3f, "soft" },
            { "wool",      -0.1f,  0.3f,  0.0f,  0.1f,  0.0f, -0.2f, "fuzzy" },
            { "linen",      0.0f,  0.2f,  0.0f, -0.1f,  0.0f, -0.2f, "breathable" },
            { "lace",       0.2f,  0.1f,  0.0f, -0.2f,  0.1f, -0.3f, "intricate" },
            { "feather",    0.1f,  0.1f,  0.1f, -0.3f,  0.2f, -0.3f, "weightless" },
            { "fur",       -0.1f,  0.3f,  0.0f,  0.1f,  0.0f, -0.2f, "textured" },

            // Hard / mineral
            { "concrete",  -0.2f, -0.1f,  0.0f,  0.3f, -0.1f,  0.3f, "gritty" },
            { "stone",     -0.2f,  0.0f,  0.0f,  0.3f,  0.1f,  0.1f, "solid" },
            { "marble",     0.1f,  0.0f,  0.0f,  0.2f,  0.2f, -0.1f, "smooth-hard" },
            { "granite",   -0.2f, -0.1f,  0.0f,  0.3f,  0.0f,  0.2f, "coarse" },
            { "obsidian",  -0.2f, -0.1f,  0.0f,  0.2f,  0.0f,  0.2f, "volcanic" },
            { "slate",     -0.1f, -0.1f,  0.0f,  0.2f,  0.0f,  0.0f, "flat" },
            { "sand",       0.1f,  0.1f,  0.1f, -0.1f,  0.1f, -0.1f, "granular" },
            { "clay",      -0.2f,  0.2f,  0.0f,  0.2f,  0.0f, -0.1f, "moldable" },
            { "pearl",      0.3f,  0.2f,  0.0f,  0.0f,  0.1f, -0.2f, "lustrous" },

            // Fluid / gas
            { "rubber",    -0.2f,  0.1f,  0.1f,  0.2f, -0.1f,  0.1f, "bouncy" },
            { "cloud",      0.1f,  0.1f,  0.1f, -0.2f,  0.5f, -0.4f, "fluffy" },
            { "liquid",     0.0f,  0.2f,  0.3f,  0.1f,  0.1f, -0.1f, "fluid" },
            { "smoke",     -0.1f,  0.1f,  0.2f,  0.0f,  0.3f, -0.2f, "hazy" },
            { "vapor",      0.0f,  0.0f,  0.2f, -0.1f,  0.3f, -0.2f, "gaseous" },
            { "steam",     -0.1f,  0.1f,  0.2f,  0.0f,  0.2f, -0.1f, "misty" },
            { "oil",       -0.2f,  0.2f,  0.1f,  0.1f,  0.0f,  0.0f, "viscous" },
            { "honey",     -0.1f,  0.4f,  0.1f,  0.1f,  0.0f, -0.2f, "sticky" },
            { "tar",       -0.3f,  0.1f,  0.0f,  0.3f,  0.0f,  0.2f, "viscous" },
            { "molten",    -0.1f,  0.3f,  0.2f,  0.3f,  0.0f,  0.4f, "flowing-hot" },
            { "plasma",     0.3f, -0.2f,  0.3f,  0.1f,  0.1f,  0.3f, "energized" },
            { "frost",      0.3f, -0.3f,  0.0f,  0.0f,  0.1f, -0.1f, "icy" },
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
        // Direct mood mentions (highest priority)
        if (lower.contains ("foundation")) return "Foundation";
        if (lower.contains ("atmosphere")) return "Atmosphere";
        if (lower.contains ("entangled"))  return "Entangled";
        if (lower.contains ("prism"))      return "Prism";
        if (lower.contains ("flux"))       return "Flux";
        if (lower.contains ("aether"))     return "Aether";

        // Foundation — solid, grounded, driving, rhythmic
        if (lower.contains ("solid") || lower.contains ("grounded") || lower.contains ("punchy")
            || lower.contains ("driving") || lower.contains ("powerful") || lower.contains ("heavy")
            || lower.contains ("thumping") || lower.contains ("stomping") || lower.contains ("bass-heavy")
            || lower.contains ("hard-hitting") || lower.contains ("muscular") || lower.contains ("backbone")
            || lower.contains ("anchor") || lower.contains ("bedrock") || lower.contains ("fundamental"))
            return "Foundation";

        // Atmosphere — ambient, spacious, slow, immersive
        if (lower.contains ("atmospheric") || lower.contains ("ambient") || lower.contains ("spacious")
            || lower.contains ("floaty") || lower.contains ("drifting") || lower.contains ("peaceful")
            || lower.contains ("serene") || lower.contains ("tranquil") || lower.contains ("meditat")
            || lower.contains ("contemplat") || lower.contains ("introspect") || lower.contains ("gentle")
            || lower.contains ("hushed") || lower.contains ("whisper") || lower.contains ("subdued")
            || lower.contains ("mist") || lower.contains ("fog") || lower.contains ("haze"))
            return "Atmosphere";

        // Entangled — chaotic, complex, layered, unpredictable
        if (lower.contains ("chaotic") || lower.contains ("complex") || lower.contains ("tangled")
            || lower.contains ("mangled") || lower.contains ("glitch") || lower.contains ("broken")
            || lower.contains ("corrupt") || lower.contains ("dissonant") || lower.contains ("clashing")
            || lower.contains ("fractured") || lower.contains ("collision") || lower.contains ("unstable")
            || lower.contains ("erratic") || lower.contains ("unpredictab") || lower.contains ("frenetic")
            || lower.contains ("turbulent") || lower.contains ("conflicting") || lower.contains ("tension"))
            return "Entangled";

        // Prism — bright, colorful, clear, sparkling
        if (lower.contains ("prismatic") || lower.contains ("bright") || lower.contains ("crystal")
            || lower.contains ("sparkl") || lower.contains ("shimmer") || lower.contains ("colorful")
            || lower.contains ("colourful") || lower.contains ("vivid") || lower.contains ("radiant")
            || lower.contains ("luminous") || lower.contains ("brilliant") || lower.contains ("gleam")
            || lower.contains ("dazzl") || lower.contains ("iridescent") || lower.contains ("rainbow")
            || lower.contains ("neon") || lower.contains ("glitter") || lower.contains ("jewel"))
            return "Prism";

        // Flux — morphing, evolving, transforming, in motion
        if (lower.contains ("morph") || lower.contains ("evolv") || lower.contains ("transform")
            || lower.contains ("shifting") || lower.contains ("changing") || lower.contains ("mutating")
            || lower.contains ("transition") || lower.contains ("journey") || lower.contains ("progress")
            || lower.contains ("unfold") || lower.contains ("develop") || lower.contains ("growing")
            || lower.contains ("becoming") || lower.contains ("metamorph") || lower.contains ("shapeshif")
            || lower.contains ("flowing") || lower.contains ("liquid") || lower.contains ("mercurial"))
            return "Flux";

        // Aether — ethereal, otherworldly, transcendent, mystical
        if (lower.contains ("ethereal") || lower.contains ("otherworld") || lower.contains ("transcend")
            || lower.contains ("mystical") || lower.contains ("cosmic") || lower.contains ("celestial")
            || lower.contains ("divine") || lower.contains ("spirit") || lower.contains ("astral")
            || lower.contains ("dream") || lower.contains ("surreal") || lower.contains ("phantom")
            || lower.contains ("ghost") || lower.contains ("void") || lower.contains ("liminal")
            || lower.contains ("sacred") || lower.contains ("metaphys") || lower.contains ("nebul"))
            return "Aether";

        // Genre-based mood inference
        if (lower.contains ("edm") || lower.contains ("techno") || lower.contains ("house")
            || lower.contains ("drum and bass") || lower.contains ("dubstep") || lower.contains ("trap")
            || lower.contains ("hardstyle") || lower.contains ("industrial"))
            return "Foundation";
        if (lower.contains ("drone") || lower.contains ("meditation") || lower.contains ("lofi")
            || lower.contains ("downtempo") || lower.contains ("chillout") || lower.contains ("shoegaze"))
            return "Atmosphere";
        if (lower.contains ("idm") || lower.contains ("experimental") || lower.contains ("noise")
            || lower.contains ("breakcore") || lower.contains ("avant-garde") || lower.contains ("deconstructed"))
            return "Entangled";
        if (lower.contains ("pop") || lower.contains ("future bass") || lower.contains ("synth wave")
            || lower.contains ("synthpop") || lower.contains ("electropop") || lower.contains ("disco"))
            return "Prism";
        if (lower.contains ("progressive") || lower.contains ("psytrance") || lower.contains ("trance")
            || lower.contains ("minimal techno") || lower.contains ("deep house"))
            return "Flux";
        if (lower.contains ("new age") || lower.contains ("healing") || lower.contains ("spiritual")
            || lower.contains ("shamanic") || lower.contains ("ritual") || lower.contains ("ceremony"))
            return "Aether";

        return {};
    }

    //--------------------------------------------------------------------------
    // Engine suggestion based on content

    static void suggestEngines (const juce::String& lower, InterpretedIntent& result)
    {
        // Opal — granular synthesis, texture, scatter, clouds
        if (lower.contains ("grain") || lower.contains ("granular") || lower.contains ("scatter")
            || lower.contains ("cloud") || lower.contains ("particle") || lower.contains ("dust")
            || lower.contains ("spray") || lower.contains ("microsound") || lower.contains ("texture")
            || lower.contains ("smear") || lower.contains ("time-stretch"))
            result.suggestedEngines.addIfNotAlreadyThere ("Opal");

        // Odyssey — FM synthesis, bells, metallic, complex timbres
        if (lower.contains ("fm ") || lower.contains ("bell") || lower.contains ("metallic")
            || lower.contains ("chime") || lower.contains ("gong") || lower.contains ("clang")
            || lower.contains ("frequency modulation") || lower.contains ("operator")
            || lower.contains ("ratio") || lower.contains ("sideband") || lower.contains ("inharmonic"))
            result.suggestedEngines.addIfNotAlreadyThere ("Odyssey");

        // OddfeliX — analog, vintage, classic subtractive
        if (lower.contains ("analog") || lower.contains ("vintage") || lower.contains ("classic")
            || lower.contains ("subtractive") || lower.contains ("moog") || lower.contains ("minimoog")
            || lower.contains ("juno") || lower.contains ("jupiter") || lower.contains ("prophet")
            || lower.contains ("retro synth") || lower.contains ("old school") || lower.contains ("70s")
            || lower.contains ("80s") || lower.contains ("saw wave") || lower.contains ("square wave"))
            result.suggestedEngines.addIfNotAlreadyThere ("OddfeliX");

        // OddOscar — morphing, wavetable, spectral morphing
        if (lower.contains ("morph") || lower.contains ("transform") || lower.contains ("shapeshif")
            || lower.contains ("wavetable") || lower.contains ("scan") || lower.contains ("interpolat")
            || lower.contains ("crossfade") || lower.contains ("mutation") || lower.contains ("hybrid")
            || lower.contains ("between"))
            result.suggestedEngines.addIfNotAlreadyThere ("OddOscar");

        // Overdub — layering, stacking, blending, multitrack
        if (lower.contains ("layer") || lower.contains ("stack") || lower.contains ("blend")
            || lower.contains ("multitrack") || lower.contains ("overdub") || lower.contains ("combine")
            || lower.contains ("fuse") || lower.contains ("mix") || lower.contains ("ensemble")
            || lower.contains ("unison") || lower.contains ("supersaw"))
            result.suggestedEngines.addIfNotAlreadyThere ("Overdub");

        // Oblong — bass, sub, low end, weight
        if (lower.contains ("bass") || lower.contains ("sub") || lower.contains ("low end")
            || lower.contains ("bottom") || lower.contains ("rumble") || lower.contains ("808")
            || lower.contains ("deep bass") || lower.contains ("sub bass") || lower.contains ("wobble")
            || lower.contains ("reese") || lower.contains ("growl"))
            result.suggestedEngines.addIfNotAlreadyThere ("Oblong");

        // Onset — drums, percussion, transients, hits
        if (lower.contains ("drum") || lower.contains ("percuss") || lower.contains ("hit")
            || lower.contains ("kick") || lower.contains ("snare") || lower.contains ("hihat")
            || lower.contains ("hi-hat") || lower.contains ("cymbal") || lower.contains ("tom")
            || lower.contains ("clap") || lower.contains ("transient") || lower.contains ("impact")
            || lower.contains ("click") || lower.contains ("snap") || lower.contains ("rim"))
            result.suggestedEngines.addIfNotAlreadyThere ("Onset");

        // Overworld — retro, 8-bit, chiptune, game, pixel
        if (lower.contains ("retro") || lower.contains ("8-bit") || lower.contains ("chiptune")
            || lower.contains ("pixel") || lower.contains ("game") || lower.contains ("nes")
            || lower.contains ("snes") || lower.contains ("gameboy") || lower.contains ("arcade")
            || lower.contains ("16-bit") || lower.contains ("bitcrush") || lower.contains ("lo-fi digital"))
            result.suggestedEngines.addIfNotAlreadyThere ("Overworld");

        // Ouroboros — feedback, recursion, self-similar, infinite
        if (lower.contains ("feedback") || lower.contains ("recursion") || lower.contains ("self-similar")
            || lower.contains ("infinite") || lower.contains ("ouroboros") || lower.contains ("loop")
            || lower.contains ("self-oscillat") || lower.contains ("runaway") || lower.contains ("howl")
            || lower.contains ("screech") || lower.contains ("no-input"))
            result.suggestedEngines.addIfNotAlreadyThere ("Ouroboros");

        // Organon — organ, additive, harmonic series, tonewheel
        if (lower.contains ("organ") || lower.contains ("additive") || lower.contains ("harmonic series")
            || lower.contains ("tonewheel") || lower.contains ("drawbar") || lower.contains ("partial")
            || lower.contains ("overtone") || lower.contains ("harmonic") || lower.contains ("sine stack")
            || lower.contains ("fourier") || lower.contains ("resynthes"))
            result.suggestedEngines.addIfNotAlreadyThere ("Organon");

        // Obscura — physical modeling, string, pluck, bow, resonant body
        if (lower.contains ("physical") || lower.contains ("string") || lower.contains ("pluck")
            || lower.contains ("bow") || lower.contains ("struck") || lower.contains ("hammered")
            || lower.contains ("karplus") || lower.contains ("waveguide") || lower.contains ("exciter")
            || lower.contains ("resonant body") || lower.contains ("acoustic model")
            || lower.contains ("stiffness") || lower.contains ("damping"))
            result.suggestedEngines.addIfNotAlreadyThere ("Obscura");

        // Obsidian — wavefold, phase distortion, nonlinear
        if (lower.contains ("wavefold") || lower.contains ("phase distort") || lower.contains ("nonlinear")
            || lower.contains ("waveshap") || lower.contains ("fold") || lower.contains ("wrap")
            || lower.contains ("casio") || lower.contains ("pd synth") || lower.contains ("harsh digital"))
            result.suggestedEngines.addIfNotAlreadyThere ("Obsidian");

        // Origami — folding, paper, geometric, angular
        if (lower.contains ("origami") || lower.contains ("paper") || lower.contains ("crease")
            || lower.contains ("geometric") || lower.contains ("angular") || lower.contains ("tessellat")
            || lower.contains ("faceted") || lower.contains ("polyhedral"))
            result.suggestedEngines.addIfNotAlreadyThere ("Origami");

        // Optic — visual modulation, light, pulse, strobe
        if (lower.contains ("visual") || lower.contains ("light") || lower.contains ("pulse")
            || lower.contains ("strobe") || lower.contains ("flash") || lower.contains ("blink")
            || lower.contains ("phosphor") || lower.contains ("laser") || lower.contains ("beam")
            || lower.contains ("autopulse") || lower.contains ("flicker"))
            result.suggestedEngines.addIfNotAlreadyThere ("Optic");

        // Oblique — prismatic bounce, refraction, diffraction
        if (lower.contains ("bounce") || lower.contains ("refract") || lower.contains ("diffract")
            || lower.contains ("oblique") || lower.contains ("ricochet") || lower.contains ("pinball")
            || lower.contains ("deflect") || lower.contains ("scatter") || lower.contains ("split"))
            result.suggestedEngines.addIfNotAlreadyThere ("Oblique");

        // Orbital — spectral, partial, additive, orbiting
        if (lower.contains ("orbit") || lower.contains ("spectral") || lower.contains ("partial tilt")
            || lower.contains ("rotating") || lower.contains ("revolv") || lower.contains ("planetary")
            || lower.contains ("centrifug") || lower.contains ("gyroscop"))
            result.suggestedEngines.addIfNotAlreadyThere ("Orbital");

        // Oceanic — ocean, water, tide, aquatic, fluid
        if (lower.contains ("ocean") || lower.contains ("water") || lower.contains ("tide")
            || lower.contains ("aquatic") || lower.contains ("marine") || lower.contains ("underwater")
            || lower.contains ("submarine") || lower.contains ("coral") || lower.contains ("whale")
            || lower.contains ("dolphin") || lower.contains ("current") || lower.contains ("wave pool"))
            result.suggestedEngines.addIfNotAlreadyThere ("Oceanic");

        // Overbite — bite, teeth, jaw, gnaw, chomp
        if (lower.contains ("bite") || lower.contains ("teeth") || lower.contains ("jaw")
            || lower.contains ("gnaw") || lower.contains ("chomp") || lower.contains ("gnash")
            || lower.contains ("crunch") || lower.contains ("chew") || lower.contains ("mangle"))
            result.suggestedEngines.addIfNotAlreadyThere ("Overbite");

        // Obese — fat, thick, saturated, heavy processing
        if (lower.contains ("fat") || lower.contains ("obese") || lower.contains ("saturated")
            || lower.contains ("overdriven") || lower.contains ("thicc") || lower.contains ("phat")
            || lower.contains ("juicy") || lower.contains ("meaty") || lower.contains ("beefy"))
            result.suggestedEngines.addIfNotAlreadyThere ("Obese");

        // Oracle — breakpoint, function generator, envelope
        if (lower.contains ("oracle") || lower.contains ("breakpoint") || lower.contains ("function gen")
            || lower.contains ("arbitrary") || lower.contains ("custom envelope") || lower.contains ("complex lfo"))
            result.suggestedEngines.addIfNotAlreadyThere ("Oracle");
    }

    //--------------------------------------------------------------------------
    // FX suggestion

    static void suggestFX (const juce::String& lower, InterpretedIntent& result)
    {
        // Reverb — space, depth, room, hall, plate
        if (lower.contains ("reverb") || lower.contains ("hall") || lower.contains ("room")
            || lower.contains ("cavernous") || lower.contains ("vast") || lower.contains ("plate")
            || lower.contains ("cathedral") || lower.contains ("chamber") || lower.contains ("spring reverb")
            || lower.contains ("spacious") || lower.contains ("ambient") || lower.contains ("cave")
            || lower.contains ("canyon") || lower.contains ("arena") || lower.contains ("stadium"))
            result.suggestedFX.addIfNotAlreadyThere ("Reverb");

        // Delay — echo, repeat, rhythmic reflection
        if (lower.contains ("delay") || lower.contains ("echo") || lower.contains ("repeat")
            || lower.contains ("ping-pong") || lower.contains ("tape delay") || lower.contains ("slapback")
            || lower.contains ("dub delay") || lower.contains ("multi-tap") || lower.contains ("feedback delay")
            || lower.contains ("dotted") || lower.contains ("triplet delay"))
            result.suggestedFX.addIfNotAlreadyThere ("Delay");

        // Saturator — distortion, overdrive, warmth, grit, fuzz
        if (lower.contains ("distort") || lower.contains ("saturate") || lower.contains ("overdrive")
            || lower.contains ("grit") || lower.contains ("fuzz") || lower.contains ("clip")
            || lower.contains ("crunch") || lower.contains ("tube") || lower.contains ("tape")
            || lower.contains ("warm") || lower.contains ("analog") || lower.contains ("drive")
            || lower.contains ("dirty") || lower.contains ("harsh") || lower.contains ("bitcrush"))
            result.suggestedFX.addIfNotAlreadyThere ("Saturator");

        // Modulation — chorus, flanger, phaser, vibrato, tremolo
        if (lower.contains ("chorus") || lower.contains ("flanger") || lower.contains ("phaser")
            || lower.contains ("vibrato") || lower.contains ("tremolo") || lower.contains ("ensemble")
            || lower.contains ("detuned") || lower.contains ("leslie") || lower.contains ("rotary")
            || lower.contains ("jet") || lower.contains ("swirl") || lower.contains ("whoosh")
            || lower.contains ("barber-pole") || lower.contains ("through-zero"))
            result.suggestedFX.addIfNotAlreadyThere ("Modulation");

        // HarmonicExciter — shimmer, sparkle, presence, air, brilliance
        if (lower.contains ("shimmer") || lower.contains ("sparkle") || lower.contains ("exciter")
            || lower.contains ("air") || lower.contains ("presence") || lower.contains ("brilliance")
            || lower.contains ("enhancer") || lower.contains ("sizzle") || lower.contains ("sheen"))
            result.suggestedFX.addIfNotAlreadyThere ("HarmonicExciter");

        // StereoSculptor — width, stereo, spread, panorama
        if (lower.contains ("wide") || lower.contains ("stereo") || lower.contains ("spread")
            || lower.contains ("panoram") || lower.contains ("spatial") || lower.contains ("3d")
            || lower.contains ("surround") || lower.contains ("immersive") || lower.contains ("binaural")
            || lower.contains ("mid-side") || lower.contains ("haas"))
            result.suggestedFX.addIfNotAlreadyThere ("StereoSculptor");

        // GranularSmear — freeze, blur, smear, stretch, grain
        if (lower.contains ("smear") || lower.contains ("blur") || lower.contains ("freeze")
            || lower.contains ("stretch") || lower.contains ("stutter") || lower.contains ("glitch")
            || lower.contains ("granular fx") || lower.contains ("time-stretch") || lower.contains ("paulstretch")
            || lower.contains ("spectral freeze") || lower.contains ("buffer"))
            result.suggestedFX.addIfNotAlreadyThere ("GranularSmear");

        // Doppler — spinning, rotating, spatial movement
        if (lower.contains ("doppler") || lower.contains ("spinning") || lower.contains ("rotating")
            || lower.contains ("orbit") || lower.contains ("circular") || lower.contains ("spiral")
            || lower.contains ("auto-pan") || lower.contains ("autopan"))
            result.suggestedFX.addIfNotAlreadyThere ("Doppler");

        // Compressor — punch, glue, squash, pump, sidechain
        if (lower.contains ("compress") || lower.contains ("punch") || lower.contains ("glue")
            || lower.contains ("squash") || lower.contains ("pump") || lower.contains ("sidechain")
            || lower.contains ("limiter") || lower.contains ("dynamics"))
            result.suggestedFX.addIfNotAlreadyThere ("Compressor");

        // EQ / Filter — tone shaping, frequency carving
        if (lower.contains ("eq") || lower.contains ("filter") || lower.contains ("tone")
            || lower.contains ("low-pass") || lower.contains ("high-pass") || lower.contains ("band-pass")
            || lower.contains ("notch") || lower.contains ("resonan") || lower.contains ("formant"))
            result.suggestedFX.addIfNotAlreadyThere ("Filter");
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

} // namespace xoceanus
