// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <functional>
#include <atomic>
#include <mutex>

namespace xoceanus
{

//==============================================================================
// CommunityInsights — Anonymous telemetry to understand what users create.
//
// Captures anonymized aggregate data about how XOceanus is used:
//   - Which engines are popular (by slot frequency)
//   - Which coupling types resonate with users
//   - What kinds of sounds people are designing (Sonic DNA distributions)
//   - Recipe usage patterns (which recipes get loaded, modified, saved)
//   - AI assistant query themes (what sounds people ask for)
//
// Privacy principles:
//   - 100% opt-in (off by default, explicit consent required)
//   - No personal data, no IP logging, no device fingerprinting
//   - No audio, no preset content, no parameter values
//   - Only aggregate counts and distributions
//   - Data is batched locally and sent periodically (not per-action)
//   - User can view exactly what will be sent before it goes
//   - User can delete local data at any time
//   - All transmission over TLS only
//
// What we learn:
//   - "40% of users are trying to make ambient pads" → improve pad presets
//   - "Origami+Opal coupling is the most popular" → optimize that path
//   - "Users keep asking AI for 'dark bass'" → add more dark bass recipes
//   - "Nobody uses the Doppler effect" → improve discoverability or docs
//
// This feeds directly into product decisions:
//   - Factory preset priorities
//   - Engine development roadmap
//   - Recipe creation focus
//   - Documentation and tutorial topics
//==============================================================================
class CommunityInsights
{
public:
    //--------------------------------------------------------------------------
    // Consent and privacy

    enum class ConsentLevel
    {
        Disabled = 0, // No data collected (default)
        BasicUsage,   // Engine/coupling popularity only
        Full          // All anonymous insights including DNA + AI themes
    };

    struct PrivacyConfig
    {
        ConsentLevel consent = ConsentLevel::Disabled;
        bool showBeforeSend = true;  // Let user review data before sending
        int batchIntervalHours = 24; // How often to send batched data
    };

    //--------------------------------------------------------------------------
    // Data structures — what we collect (all anonymous, no PII)

    struct EngineUsageData
    {
        std::array<int, 21> engineSlotCounts{};    // How often each engine is loaded
        std::array<int, 21> enginePrimaryCounts{}; // How often each is in slot 0
        int totalSessions = 0;
    };

    struct CouplingUsageData
    {
        std::array<int, 12> couplingTypeCounts{}; // Usage of each coupling type
        int totalCouplingRoutes = 0;
        int avgRoutesPerSession = 0;
    };

    struct SonicDNADistribution
    {
        // Histogram bins (0-10 range, 5 bins each)
        std::array<int, 5> brightness{};
        std::array<int, 5> warmth{};
        std::array<int, 5> movement{};
        std::array<int, 5> density{};
        std::array<int, 5> space{};
        std::array<int, 5> aggression{};
        int totalPresetsSaved = 0;
    };

    struct RecipeUsageData
    {
        std::array<int, 6> moodCounts{}; // Foundation, Atmosphere, etc.
        int factoryRecipesLoaded = 0;
        int userRecipesCreated = 0;
        int recipesModifiedAfterLoad = 0;
    };

    struct AIQueryThemes
    {
        // Categorized query counts (no actual query text stored)
        int bassQueries = 0;
        int padQueries = 0;
        int leadQueries = 0;
        int fxQueries = 0;
        int percussionQueries = 0;
        int ambientQueries = 0;
        int aggressiveQueries = 0;
        int experimentalQueries = 0;
        int totalQueries = 0;
    };

    struct FXUsageData
    {
        std::array<int, 18> fxStageCounts{}; // Which FX stages are active
        int vibeKnobSweetCount = 0;          // How often vibe goes sweet vs grit
        int vibeKnobGritCount = 0;
    };

    /// Complete batch ready for transmission
    struct InsightsBatch
    {
        juce::String batchId; // Random UUID (not device-linked)
        juce::String appVersion;
        juce::String platform; // "macOS" / "iOS" (no version specifics)
        int64_t timestamp = 0; // Unix timestamp of batch creation

        EngineUsageData engines;
        CouplingUsageData coupling;
        SonicDNADistribution dna;
        RecipeUsageData recipes;
        AIQueryThemes aiThemes;
        FXUsageData fx;
    };

    //--------------------------------------------------------------------------
    // Public API

    CommunityInsights() = default;

    void setPrivacyConfig(PrivacyConfig config)
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        privacy = config;
    }

    ConsentLevel getConsentLevel() const { return privacy.consent; }
    bool isEnabled() const { return privacy.consent != ConsentLevel::Disabled; }

    //--------------------------------------------------------------------------
    // Recording events (call from main/UI thread)

    void recordEngineLoaded(int engineIndex, int slotIndex)
    {
        if (!isEnabled())
            return;
        std::lock_guard<std::mutex> lock(dataMutex);

        if (engineIndex >= 0 && engineIndex < 21)
        {
            currentBatch.engines.engineSlotCounts[static_cast<size_t>(engineIndex)]++;
            if (slotIndex == 0)
                currentBatch.engines.enginePrimaryCounts[static_cast<size_t>(engineIndex)]++;
        }
    }

    void recordCouplingRoute(int couplingType)
    {
        if (!isEnabled())
            return;
        std::lock_guard<std::mutex> lock(dataMutex);

        if (couplingType >= 0 && couplingType < 12)
        {
            currentBatch.coupling.couplingTypeCounts[static_cast<size_t>(couplingType)]++;
            currentBatch.coupling.totalCouplingRoutes++;
        }
    }

    void recordPresetSaved(float brightness, float warmth, float movement, float density, float space, float aggression)
    {
        if (privacy.consent < ConsentLevel::Full)
            return;
        std::lock_guard<std::mutex> lock(dataMutex);

        auto bin = [](float v) -> int { return std::clamp(static_cast<int>(v * 5.0f), 0, 4); };

        currentBatch.dna.brightness[static_cast<size_t>(bin(brightness))]++;
        currentBatch.dna.warmth[static_cast<size_t>(bin(warmth))]++;
        currentBatch.dna.movement[static_cast<size_t>(bin(movement))]++;
        currentBatch.dna.density[static_cast<size_t>(bin(density))]++;
        currentBatch.dna.space[static_cast<size_t>(bin(space))]++;
        currentBatch.dna.aggression[static_cast<size_t>(bin(aggression))]++;
        currentBatch.dna.totalPresetsSaved++;
    }

    void recordRecipeLoaded(int moodIndex, bool isFactory)
    {
        if (!isEnabled())
            return;
        std::lock_guard<std::mutex> lock(dataMutex);

        if (moodIndex >= 0 && moodIndex < 6)
            currentBatch.recipes.moodCounts[static_cast<size_t>(moodIndex)]++;

        if (isFactory)
            currentBatch.recipes.factoryRecipesLoaded++;
    }

    void recordRecipeCreated()
    {
        if (!isEnabled())
            return;
        std::lock_guard<std::mutex> lock(dataMutex);
        currentBatch.recipes.userRecipesCreated++;
    }

    void recordRecipeModified()
    {
        if (!isEnabled())
            return;
        std::lock_guard<std::mutex> lock(dataMutex);
        currentBatch.recipes.recipesModifiedAfterLoad++;
    }

    void recordAIQuery(const juce::String& queryCategory)
    {
        if (privacy.consent < ConsentLevel::Full)
            return;
        std::lock_guard<std::mutex> lock(dataMutex);

        // Only store category counts — NEVER the actual query text
        if (queryCategory == "bass")
            currentBatch.aiThemes.bassQueries++;
        else if (queryCategory == "pad")
            currentBatch.aiThemes.padQueries++;
        else if (queryCategory == "lead")
            currentBatch.aiThemes.leadQueries++;
        else if (queryCategory == "fx")
            currentBatch.aiThemes.fxQueries++;
        else if (queryCategory == "percussion")
            currentBatch.aiThemes.percussionQueries++;
        else if (queryCategory == "ambient")
            currentBatch.aiThemes.ambientQueries++;
        else if (queryCategory == "aggressive")
            currentBatch.aiThemes.aggressiveQueries++;
        else
            currentBatch.aiThemes.experimentalQueries++;

        currentBatch.aiThemes.totalQueries++;
    }

    void recordFXStageActive(int stageIndex)
    {
        if (!isEnabled())
            return;
        std::lock_guard<std::mutex> lock(dataMutex);

        if (stageIndex >= 0 && stageIndex < 18)
            currentBatch.fx.fxStageCounts[static_cast<size_t>(stageIndex)]++;
    }

    void recordVibeKnobUsage(float vibeValue)
    {
        if (!isEnabled())
            return;
        std::lock_guard<std::mutex> lock(dataMutex);

        if (vibeValue < -0.05f)
            currentBatch.fx.vibeKnobSweetCount++;
        else if (vibeValue > 0.05f)
            currentBatch.fx.vibeKnobGritCount++;
    }

    void recordSessionStart()
    {
        if (!isEnabled())
            return;
        std::lock_guard<std::mutex> lock(dataMutex);
        currentBatch.engines.totalSessions++;
    }

    //--------------------------------------------------------------------------
    // Data access — let user see exactly what will be sent

    juce::String getPreviewJSON() const
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        return batchToJSON(currentBatch);
    }

    /// Build the batch and return it for transmission
    InsightsBatch prepareBatch()
    {
        std::lock_guard<std::mutex> lock(dataMutex);

        currentBatch.batchId = juce::Uuid().toString();
        currentBatch.timestamp = juce::Time::currentTimeMillis();
        currentBatch.appVersion = "1.0.0";

#if JUCE_MAC || JUCE_IOS
        currentBatch.platform = "Apple";
#else
        currentBatch.platform = "Other";
#endif

        InsightsBatch batch = currentBatch;

        // Reset for next collection period
        currentBatch = {};

        return batch;
    }

    /// Delete all collected data without sending
    void clearAllData()
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        currentBatch = {};
    }

    //--------------------------------------------------------------------------
    // Persistence — save/load pending batch to disk

    void savePendingToDisk(const juce::File& appDataDir) const
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        auto file = appDataDir.getChildFile("insights_pending.json");
        if (!file.replaceWithText(batchToJSON(currentBatch)))
            DBG("CommunityInsights: failed to write batch to " + file.getFullPathName());
    }

    void loadPendingFromDisk(const juce::File& appDataDir)
    {
        auto file = appDataDir.getChildFile("insights_pending.json");
        if (!file.existsAsFile())
            return;

        auto json = juce::JSON::parse(file.loadFileAsString());
        if (json.isObject())
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            currentBatch = batchFromJSON(json);
        }
    }

    //--------------------------------------------------------------------------
    // Transmission (call from background thread only)

    struct TransmitResult
    {
        bool success = false;
        juce::String message;
    };

    /// Supabase connection config
    struct SupabaseConfig
    {
        juce::String projectUrl; // e.g. "https://abcdefg.supabase.co"
        juce::String anonKey;    // Supabase anon/public key
    };

    void setSupabaseConfig(SupabaseConfig config)
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        supabase = std::move(config);
    }

    /// Send batch to Supabase PostgREST endpoint.
    /// MUST be called from a background thread — never from audio or UI thread.
    TransmitResult transmitBatch(const InsightsBatch& batch) const
    {
        SupabaseConfig cfg;
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            cfg = supabase;
        }

        if (cfg.projectUrl.isEmpty() || cfg.anonKey.isEmpty())
            return {false, "Supabase not configured"};

        // TLS only
        if (!cfg.projectUrl.startsWith("https://"))
            return {false, "HTTPS required"};

        // Format as Supabase PostgREST row insert
        juce::String jsonBody = batchToSupabaseRow(batch);

        juce::String endpoint = cfg.projectUrl + "/rest/v1/insight_batches";
        juce::URL url(endpoint);
        url = url.withPOSTData(jsonBody);

        auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostBody)
                           .withConnectionTimeoutMs(10000)
                           .withExtraHeaders("Content-Type: application/json\r\n"
                                             "apikey: " +
                                             cfg.anonKey +
                                             "\r\n"
                                             "Authorization: Bearer " +
                                             cfg.anonKey +
                                             "\r\n"
                                             "Prefer: return=minimal\r\n");

        auto stream = url.createInputStream(options);

        if (stream == nullptr)
            return {false, "Connection failed"};

        auto response = stream->readEntireStreamAsString();
        return {true, "Sent successfully"};
    }

private:
    PrivacyConfig privacy;
    SupabaseConfig supabase;
    mutable std::mutex dataMutex;
    InsightsBatch currentBatch;

    //--------------------------------------------------------------------------
    // Supabase PostgREST row format (flat columns, Postgres arrays)

    static juce::String batchToSupabaseRow(const InsightsBatch& b)
    {
        auto obj = std::make_unique<juce::DynamicObject>();

        obj->setProperty("batch_id", b.batchId);
        obj->setProperty("app_version", b.appVersion);
        obj->setProperty("platform", b.platform);

        // Postgres INT[] as JSON arrays
        auto toIntArray = [](const auto& arr, int count)
        {
            juce::Array<juce::var> result;
            for (int i = 0; i < count; ++i)
                result.add(arr[static_cast<size_t>(i)]);
            return result;
        };

        // Engine usage
        obj->setProperty("engine_slot_counts", toIntArray(b.engines.engineSlotCounts, 21));
        obj->setProperty("engine_primary_counts", toIntArray(b.engines.enginePrimaryCounts, 21));
        obj->setProperty("total_sessions", b.engines.totalSessions);

        // Coupling
        obj->setProperty("coupling_type_counts", toIntArray(b.coupling.couplingTypeCounts, 12));
        obj->setProperty("coupling_total_routes", b.coupling.totalCouplingRoutes);

        // DNA histograms
        obj->setProperty("dna_brightness", toIntArray(b.dna.brightness, 5));
        obj->setProperty("dna_warmth", toIntArray(b.dna.warmth, 5));
        obj->setProperty("dna_movement", toIntArray(b.dna.movement, 5));
        obj->setProperty("dna_density", toIntArray(b.dna.density, 5));
        obj->setProperty("dna_space", toIntArray(b.dna.space, 5));
        obj->setProperty("dna_aggression", toIntArray(b.dna.aggression, 5));
        obj->setProperty("dna_presets_saved", b.dna.totalPresetsSaved);

        // Recipes
        obj->setProperty("recipe_mood_counts", toIntArray(b.recipes.moodCounts, 6));
        obj->setProperty("recipe_factory_loaded", b.recipes.factoryRecipesLoaded);
        obj->setProperty("recipe_user_created", b.recipes.userRecipesCreated);
        obj->setProperty("recipe_modified_after_load", b.recipes.recipesModifiedAfterLoad);

        // AI themes
        obj->setProperty("ai_bass", b.aiThemes.bassQueries);
        obj->setProperty("ai_pad", b.aiThemes.padQueries);
        obj->setProperty("ai_lead", b.aiThemes.leadQueries);
        obj->setProperty("ai_fx", b.aiThemes.fxQueries);
        obj->setProperty("ai_percussion", b.aiThemes.percussionQueries);
        obj->setProperty("ai_ambient", b.aiThemes.ambientQueries);
        obj->setProperty("ai_aggressive", b.aiThemes.aggressiveQueries);
        obj->setProperty("ai_experimental", b.aiThemes.experimentalQueries);
        obj->setProperty("ai_total", b.aiThemes.totalQueries);

        // FX
        obj->setProperty("fx_stage_counts", toIntArray(b.fx.fxStageCounts, 18));
        obj->setProperty("fx_vibe_sweet", b.fx.vibeKnobSweetCount);
        obj->setProperty("fx_vibe_grit", b.fx.vibeKnobGritCount);

        return juce::JSON::toString(juce::var(obj.release()));
    }

    //--------------------------------------------------------------------------
    // JSON serialization (preview/persistence format — nested for readability)

    static juce::String batchToJSON(const InsightsBatch& b)
    {
        auto obj = std::make_unique<juce::DynamicObject>();

        obj->setProperty("batchId", b.batchId);
        obj->setProperty("appVersion", b.appVersion);
        obj->setProperty("platform", b.platform);
        obj->setProperty("timestamp", b.timestamp);

        // Engines
        auto engObj = std::make_unique<juce::DynamicObject>();
        juce::Array<juce::var> slotCounts, primaryCounts;
        for (int i = 0; i < 21; ++i)
        {
            slotCounts.add(b.engines.engineSlotCounts[static_cast<size_t>(i)]);
            primaryCounts.add(b.engines.enginePrimaryCounts[static_cast<size_t>(i)]);
        }
        engObj->setProperty("slotCounts", slotCounts);
        engObj->setProperty("primaryCounts", primaryCounts);
        engObj->setProperty("totalSessions", b.engines.totalSessions);
        obj->setProperty("engines", engObj.release());

        // Coupling
        auto cplObj = std::make_unique<juce::DynamicObject>();
        juce::Array<juce::var> cplCounts;
        for (int i = 0; i < 12; ++i)
            cplCounts.add(b.coupling.couplingTypeCounts[static_cast<size_t>(i)]);
        cplObj->setProperty("typeCounts", cplCounts);
        cplObj->setProperty("totalRoutes", b.coupling.totalCouplingRoutes);
        obj->setProperty("coupling", cplObj.release());

        // DNA
        auto dnaObj = std::make_unique<juce::DynamicObject>();
        auto addHist = [&](const char* name, const std::array<int, 5>& bins)
        {
            juce::Array<juce::var> arr;
            for (auto v : bins)
                arr.add(v);
            dnaObj->setProperty(name, arr);
        };
        addHist("brightness", b.dna.brightness);
        addHist("warmth", b.dna.warmth);
        addHist("movement", b.dna.movement);
        addHist("density", b.dna.density);
        addHist("space", b.dna.space);
        addHist("aggression", b.dna.aggression);
        dnaObj->setProperty("totalPresetsSaved", b.dna.totalPresetsSaved);
        obj->setProperty("dna", dnaObj.release());

        // Recipes
        auto recObj = std::make_unique<juce::DynamicObject>();
        juce::Array<juce::var> moodCounts;
        for (int i = 0; i < 6; ++i)
            moodCounts.add(b.recipes.moodCounts[static_cast<size_t>(i)]);
        recObj->setProperty("moodCounts", moodCounts);
        recObj->setProperty("factoryLoaded", b.recipes.factoryRecipesLoaded);
        recObj->setProperty("userCreated", b.recipes.userRecipesCreated);
        recObj->setProperty("modifiedAfterLoad", b.recipes.recipesModifiedAfterLoad);
        obj->setProperty("recipes", recObj.release());

        // AI themes
        auto aiObj = std::make_unique<juce::DynamicObject>();
        aiObj->setProperty("bass", b.aiThemes.bassQueries);
        aiObj->setProperty("pad", b.aiThemes.padQueries);
        aiObj->setProperty("lead", b.aiThemes.leadQueries);
        aiObj->setProperty("fx", b.aiThemes.fxQueries);
        aiObj->setProperty("percussion", b.aiThemes.percussionQueries);
        aiObj->setProperty("ambient", b.aiThemes.ambientQueries);
        aiObj->setProperty("aggressive", b.aiThemes.aggressiveQueries);
        aiObj->setProperty("experimental", b.aiThemes.experimentalQueries);
        aiObj->setProperty("total", b.aiThemes.totalQueries);
        obj->setProperty("aiThemes", aiObj.release());

        // FX
        auto fxObj = std::make_unique<juce::DynamicObject>();
        juce::Array<juce::var> fxCounts;
        for (int i = 0; i < 18; ++i)
            fxCounts.add(b.fx.fxStageCounts[static_cast<size_t>(i)]);
        fxObj->setProperty("stageCounts", fxCounts);
        fxObj->setProperty("vibeSweet", b.fx.vibeKnobSweetCount);
        fxObj->setProperty("vibeGrit", b.fx.vibeKnobGritCount);
        obj->setProperty("fx", fxObj.release());

        return juce::JSON::toString(juce::var(obj.release()));
    }

    static InsightsBatch batchFromJSON(const juce::var& json)
    {
        InsightsBatch b;

        b.batchId = json.getProperty("batchId", "").toString();
        b.appVersion = json.getProperty("appVersion", "").toString();
        b.platform = json.getProperty("platform", "").toString();
        b.timestamp = static_cast<int64_t>(json.getProperty("timestamp", 0));

        auto readIntArray = [](const juce::var& arr, auto& target, int maxSize)
        {
            if (auto* a = arr.getArray())
                for (int i = 0; i < std::min(static_cast<int>(a->size()), maxSize); ++i)
                    target[static_cast<size_t>(i)] = static_cast<int>((*a)[i]);
        };

        // Engines
        auto eng = json.getProperty("engines", {});
        readIntArray(eng.getProperty("slotCounts", {}), b.engines.engineSlotCounts, 21);
        readIntArray(eng.getProperty("primaryCounts", {}), b.engines.enginePrimaryCounts, 21);
        b.engines.totalSessions = static_cast<int>(eng.getProperty("totalSessions", 0));

        // Coupling
        auto cpl = json.getProperty("coupling", {});
        readIntArray(cpl.getProperty("typeCounts", {}), b.coupling.couplingTypeCounts, 12);
        b.coupling.totalCouplingRoutes = static_cast<int>(cpl.getProperty("totalRoutes", 0));

        // DNA
        auto dna = json.getProperty("dna", {});
        readIntArray(dna.getProperty("brightness", {}), b.dna.brightness, 5);
        readIntArray(dna.getProperty("warmth", {}), b.dna.warmth, 5);
        readIntArray(dna.getProperty("movement", {}), b.dna.movement, 5);
        readIntArray(dna.getProperty("density", {}), b.dna.density, 5);
        readIntArray(dna.getProperty("space", {}), b.dna.space, 5);
        readIntArray(dna.getProperty("aggression", {}), b.dna.aggression, 5);
        b.dna.totalPresetsSaved = static_cast<int>(dna.getProperty("totalPresetsSaved", 0));

        // Recipes
        auto rec = json.getProperty("recipes", {});
        readIntArray(rec.getProperty("moodCounts", {}), b.recipes.moodCounts, 6);
        b.recipes.factoryRecipesLoaded = static_cast<int>(rec.getProperty("factoryLoaded", 0));
        b.recipes.userRecipesCreated = static_cast<int>(rec.getProperty("userCreated", 0));
        b.recipes.recipesModifiedAfterLoad = static_cast<int>(rec.getProperty("modifiedAfterLoad", 0));

        // AI themes
        auto ai = json.getProperty("aiThemes", {});
        b.aiThemes.bassQueries = static_cast<int>(ai.getProperty("bass", 0));
        b.aiThemes.padQueries = static_cast<int>(ai.getProperty("pad", 0));
        b.aiThemes.leadQueries = static_cast<int>(ai.getProperty("lead", 0));
        b.aiThemes.fxQueries = static_cast<int>(ai.getProperty("fx", 0));
        b.aiThemes.percussionQueries = static_cast<int>(ai.getProperty("percussion", 0));
        b.aiThemes.ambientQueries = static_cast<int>(ai.getProperty("ambient", 0));
        b.aiThemes.aggressiveQueries = static_cast<int>(ai.getProperty("aggressive", 0));
        b.aiThemes.experimentalQueries = static_cast<int>(ai.getProperty("experimental", 0));
        b.aiThemes.totalQueries = static_cast<int>(ai.getProperty("total", 0));

        // FX
        auto fx = json.getProperty("fx", {});
        readIntArray(fx.getProperty("stageCounts", {}), b.fx.fxStageCounts, 18);
        b.fx.vibeKnobSweetCount = static_cast<int>(fx.getProperty("vibeSweet", 0));
        b.fx.vibeKnobGritCount = static_cast<int>(fx.getProperty("vibeGrit", 0));

        return b;
    }
};

} // namespace xoceanus
