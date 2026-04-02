// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_core/juce_core.h>
#include <functional>
#include <mutex>
#include <vector>

namespace xoceanus {

//==============================================================================
// SharedRecipeVault — Opt-in community recipe sharing.
//
// Allows users to share their .xorecipe creations with the community and
// browse/download recipes others have shared. This creates a living library
// of sound architectures that grows with the user base.
//
// For XO_OX Designs, this is a goldmine:
//   - See what combinations users discover that we never imagined
//   - Identify popular engine pairings for optimization
//   - Curate the best community recipes into future factory releases
//   - Understand the gap between what users want and what we provide
//
// Privacy & consent:
//   - 100% opt-in (sharing is a deliberate action, never automatic)
//   - Users choose a display name (can be anonymous/pseudonymous)
//   - No personal data attached to shared recipes
//   - Users can delete their shared recipes at any time
//   - All transmission over TLS only
//   - Content moderation: recipes are just JSON configs, no executable code
//
// Features:
//   - Share: Upload a recipe with optional description and tags
//   - Browse: Search/filter community recipes by mood, engine, tags
//   - Featured: XO_OX-curated "Staff Picks" from community submissions
//   - Remix: Download a community recipe, modify it, re-share as remix
//   - Ratings: Simple thumbs-up system (no comments to moderate)
//   - Versioning: Recipes track which XOceanus version they were created with
//
// Integration with CommunityInsights:
//   - Popular shared recipes inform factory preset priorities
//   - Engine/coupling patterns from shared recipes feed aggregate stats
//   - AI assistant can reference community favorites for suggestions
//==============================================================================
class SharedRecipeVault
{
public:
    //--------------------------------------------------------------------------
    // Data types

    struct RecipeMetadata
    {
        juce::String recipeId;          // Server-assigned UUID
        juce::String title;             // Recipe name (from .xorecipe)
        juce::String description;       // Optional user description
        juce::String authorName;        // Display name (can be "Anonymous")
        juce::String mood;              // Mood category
        juce::StringArray engines;      // Engine IDs used
        juce::StringArray tags;         // User-defined tags
        juce::String appVersion;        // XOceanus version it was created with
        int thumbsUp = 0;
        bool isStaffPick = false;
        bool isRemix = false;
        juce::String remixOfId;         // If remix, original recipe ID
        int64_t sharedTimestamp = 0;
    };

    struct BrowseFilter
    {
        juce::String mood;              // Filter by mood (empty = all)
        juce::String engineId;          // Must include this engine
        juce::String searchText;        // Text search in title/description/tags
        bool staffPicksOnly = false;
        enum class SortBy { Newest, Popular, StaffPicks } sort = SortBy::Newest;
        int page = 0;
        int pageSize = 20;
    };

    struct BrowseResult
    {
        std::vector<RecipeMetadata> recipes;
        int totalResults = 0;
        int totalPages = 0;
        bool success = false;
        juce::String errorMessage;
    };

    struct ShareResult
    {
        bool success = false;
        juce::String recipeId;          // Assigned ID on success
        juce::String errorMessage;
    };

    struct DownloadResult
    {
        bool success = false;
        juce::String recipeJSON;        // The full .xorecipe content
        RecipeMetadata metadata;
        juce::String errorMessage;
    };

    //--------------------------------------------------------------------------
    // Configuration

    struct VaultConfig
    {
        juce::String supabaseUrl;       // e.g. "https://abcdefg.supabase.co"
        juce::String supabaseAnonKey;   // Supabase anon/public key
        juce::String displayName = "Anonymous";
        juce::String authorToken;       // Hashed token for delete rights (device-derived)
        int timeoutMs = 15000;
    };

    //--------------------------------------------------------------------------
    // Callbacks for async operations

    std::function<void (const BrowseResult&)> onBrowseComplete;
    std::function<void (const ShareResult&)> onShareComplete;
    std::function<void (const DownloadResult&)> onDownloadComplete;

    //--------------------------------------------------------------------------
    // Public API

    SharedRecipeVault() = default;

    ~SharedRecipeVault()
    {
        // Signal all in-flight background threads not to dereference this
        // object. Threads capture a pointer to destroyed_ and check it
        // before invoking callbacks, preventing a use-after-free if the
        // vault is destroyed while a network request is still in flight.
        destroyed_.store (true);
    }

    void configure (VaultConfig config)
    {
        std::lock_guard<std::mutex> lock (configMutex);
        cfg = std::move (config);
    }

    bool isConfigured() const
    {
        std::lock_guard<std::mutex> lock (configMutex);
        return cfg.supabaseUrl.isNotEmpty() && cfg.supabaseAnonKey.isNotEmpty();
    }

    //--------------------------------------------------------------------------
    // Share a recipe (background thread)

    /// Share a recipe with the community.
    /// recipeJSON: the full .xorecipe file content
    /// description: optional user description
    /// tags: optional tags for discoverability
    /// MUST be called from background thread.
    ShareResult shareRecipe (const juce::String& recipeJSON,
                             const juce::String& description,
                             const juce::StringArray& tags)
    {
        VaultConfig localCfg;
        {
            std::lock_guard<std::mutex> lock (configMutex);
            localCfg = cfg;
        }

        if (localCfg.supabaseUrl.isEmpty())
            return { false, {}, "Vault not configured" };
        if (! requireHttps (localCfg.supabaseUrl))
            return { false, {}, "HTTPS required" };

        // Parse recipe to extract metadata for the row
        auto recipe = juce::JSON::parse (recipeJSON);
        if (! recipe.isObject())
            return { false, {}, "Invalid recipe JSON" };

        // Extract title, mood, engines from recipe content
        auto title = recipe.getProperty ("name", "Untitled").toString();
        auto mood = recipe.getProperty ("mood", "").toString();

        juce::Array<juce::var> enginesArray;
        if (auto* engArr = recipe.getProperty ("engines", {}).getArray())
            for (const auto& e : *engArr)
                enginesArray.add (e.getProperty ("engineId", "").toString());

        juce::Array<juce::var> tagsArray;
        for (const auto& tag : tags)
            tagsArray.add (tag);

        // Build PostgREST row
        auto row = std::make_unique<juce::DynamicObject>();
        row->setProperty ("title", title);
        row->setProperty ("description", description);
        row->setProperty ("author_name", localCfg.displayName);
        row->setProperty ("author_token", localCfg.authorToken);
        row->setProperty ("mood", mood);
        row->setProperty ("engines", enginesArray);
        row->setProperty ("tags", tagsArray);
        row->setProperty ("app_version", "1.0.0");
        row->setProperty ("recipe_json", recipe);

        juce::String jsonBody = juce::JSON::toString (juce::var (row.release()));

        // POST to Supabase PostgREST
        juce::String endpoint = localCfg.supabaseUrl + "/rest/v1/shared_recipes";
        juce::URL url (endpoint);
        url = url.withPOSTData (jsonBody);

        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostBody)
                           .withConnectionTimeoutMs (localCfg.timeoutMs)
                           .withExtraHeaders (supabaseHeaders (localCfg)
                                              + "Prefer: return=representation\r\n");

        auto stream = url.createInputStream (options);
        if (stream == nullptr)
            return { false, {}, "Connection failed" };

        auto response = stream->readEntireStreamAsString();
        auto responseJSON = juce::JSON::parse (response);

        // PostgREST returns an array with the inserted row
        if (auto* arr = responseJSON.getArray())
        {
            if (arr->size() > 0)
            {
                auto id = (*arr)[0].getProperty ("recipe_id", "").toString();
                if (id.isNotEmpty())
                    return { true, id, {} };
            }
        }

        return { false, {}, "Server error: " + response.substring (0, 200) };
    }

    //--------------------------------------------------------------------------
    // Browse community recipes (background thread)
    // Uses the vault-search Edge Function for full-text search + pagination

    BrowseResult browse (const BrowseFilter& filter)
    {
        VaultConfig localCfg;
        {
            std::lock_guard<std::mutex> lock (configMutex);
            localCfg = cfg;
        }

        if (localCfg.supabaseUrl.isEmpty())
            return { {}, 0, 0, false, "Vault not configured" };
        if (! requireHttps (localCfg.supabaseUrl))
            return { {}, 0, 0, false, "HTTPS required" };

        // Call the vault-search Edge Function
        juce::String endpoint = localCfg.supabaseUrl + "/functions/v1/vault-search?";
        endpoint += "page=" + juce::String (filter.page);
        endpoint += "&pageSize=" + juce::String (filter.pageSize);

        if (filter.mood.isNotEmpty())
            endpoint += "&mood=" + juce::URL::addEscapeChars (filter.mood, true);
        if (filter.engineId.isNotEmpty())
            endpoint += "&engine=" + juce::URL::addEscapeChars (filter.engineId, true);
        if (filter.searchText.isNotEmpty())
            endpoint += "&q=" + juce::URL::addEscapeChars (filter.searchText, true);
        if (filter.staffPicksOnly)
            endpoint += "&staffPicks=true";

        switch (filter.sort)
        {
            case BrowseFilter::SortBy::Newest:    endpoint += "&sort=newest"; break;
            case BrowseFilter::SortBy::Popular:    endpoint += "&sort=popular"; break;
            case BrowseFilter::SortBy::StaffPicks: endpoint += "&sort=staffpicks"; break;
        }

        juce::URL url (endpoint);
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                           .withConnectionTimeoutMs (localCfg.timeoutMs)
                           .withExtraHeaders (supabaseHeaders (localCfg));

        auto stream = url.createInputStream (options);
        if (stream == nullptr)
            return { {}, 0, 0, false, "Connection failed" };

        auto response = stream->readEntireStreamAsString();
        auto json = juce::JSON::parse (response);

        if (! json.isObject())
            return { {}, 0, 0, false, "Invalid response" };

        BrowseResult result;
        result.success = true;
        result.totalResults = static_cast<int> (json.getProperty ("totalResults", 0));
        result.totalPages = static_cast<int> (json.getProperty ("totalPages", 0));

        if (auto* arr = json.getProperty ("recipes", {}).getArray())
        {
            for (const auto& item : *arr)
                result.recipes.push_back (parseMetadata (item));
        }

        return result;
    }

    //--------------------------------------------------------------------------
    // Download a specific recipe (background thread)
    // Uses PostgREST direct query

    DownloadResult downloadRecipe (const juce::String& recipeId)
    {
        VaultConfig localCfg;
        {
            std::lock_guard<std::mutex> lock (configMutex);
            localCfg = cfg;
        }

        if (localCfg.supabaseUrl.isEmpty())
            return { false, {}, {}, "Vault not configured" };
        if (! requireHttps (localCfg.supabaseUrl))
            return { false, {}, {}, "HTTPS required" };

        // PostgREST query: select single row by recipe_id
        juce::String endpoint = localCfg.supabaseUrl + "/rest/v1/shared_recipes"
                                "?recipe_id=eq." + juce::URL::addEscapeChars (recipeId, true)
                                + "&select=*";

        juce::URL url (endpoint);
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                           .withConnectionTimeoutMs (localCfg.timeoutMs)
                           .withExtraHeaders (supabaseHeaders (localCfg));

        auto stream = url.createInputStream (options);
        if (stream == nullptr)
            return { false, {}, {}, "Connection failed" };

        auto response = stream->readEntireStreamAsString();
        auto json = juce::JSON::parse (response);

        // PostgREST returns an array
        auto* arr = json.getArray();
        if (arr == nullptr || arr->size() == 0)
            return { false, {}, {}, "Recipe not found" };

        auto row = (*arr)[0];

        DownloadResult result;
        result.success = true;
        result.metadata = parseMetadataFromRow (row);
        result.recipeJSON = juce::JSON::toString (row.getProperty ("recipe_json", {}));

        return result;
    }

    //--------------------------------------------------------------------------
    // Thumbs up (background thread)
    // Uses the vault-thumbsup Edge Function for dedup

    bool thumbsUp (const juce::String& recipeId, const juce::String& voterHash)
    {
        VaultConfig localCfg;
        {
            std::lock_guard<std::mutex> lock (configMutex);
            localCfg = cfg;
        }

        if (localCfg.supabaseUrl.isEmpty()) return false;
        if (! requireHttps (localCfg.supabaseUrl)) return false;

        auto body = std::make_unique<juce::DynamicObject>();
        body->setProperty ("recipeId", recipeId);
        body->setProperty ("voterHash", voterHash);

        juce::String jsonBody = juce::JSON::toString (juce::var (body.release()));

        juce::String endpoint = localCfg.supabaseUrl + "/functions/v1/vault-thumbsup";
        juce::URL url (endpoint);
        url = url.withPOSTData (jsonBody);

        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostBody)
                           .withConnectionTimeoutMs (localCfg.timeoutMs)
                           .withExtraHeaders (supabaseHeaders (localCfg));

        auto stream = url.createInputStream (options);
        return stream != nullptr;
    }

    //--------------------------------------------------------------------------
    // Delete own recipe (background thread)
    // Uses PostgREST with author_token match via RLS

    bool deleteOwnRecipe (const juce::String& recipeId)
    {
        VaultConfig localCfg;
        {
            std::lock_guard<std::mutex> lock (configMutex);
            localCfg = cfg;
        }

        if (localCfg.supabaseUrl.isEmpty() || localCfg.authorToken.isEmpty())
            return false;
        if (! requireHttps (localCfg.supabaseUrl))
            return false;

        // PostgREST DELETE with filter
        juce::String endpoint = localCfg.supabaseUrl + "/rest/v1/shared_recipes"
                                "?recipe_id=eq." + juce::URL::addEscapeChars (recipeId, true);

        juce::URL url (endpoint);

        // Pass author token via header for RLS policy check
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                           .withConnectionTimeoutMs (localCfg.timeoutMs)
                           .withExtraHeaders (supabaseHeaders (localCfg)
                                              + "X-HTTP-Method-Override: DELETE\r\n"
                                              + "x-author-token: " + localCfg.authorToken + "\r\n");

        auto stream = url.createInputStream (options);
        return stream != nullptr;
    }

    //--------------------------------------------------------------------------
    // Async wrappers (launch background threads)

    void browseAsync (const BrowseFilter& filter)
    {
        auto* thread = new BrowseThread (*this, filter);
        thread->startThread();
    }

    void shareAsync (const juce::String& recipeJSON,
                     const juce::String& description,
                     const juce::StringArray& tags)
    {
        auto* thread = new ShareThread (*this, recipeJSON, description, tags);
        thread->startThread();
    }

    void downloadAsync (const juce::String& recipeId)
    {
        auto* thread = new DownloadThread (*this, recipeId);
        thread->startThread();
    }

private:
    std::atomic<bool> destroyed_ { false };
    mutable std::mutex configMutex;
    VaultConfig cfg;

    // Enforce TLS-only connections — reject any non-HTTPS URL.
    static bool requireHttps (const juce::String& url)
    {
        return url.startsWithIgnoreCase ("https://");
    }

    static juce::String supabaseHeaders (const VaultConfig& c)
    {
        juce::String headers;
        headers += "Content-Type: application/json\r\n";
        headers += "apikey: " + c.supabaseAnonKey + "\r\n";
        headers += "Authorization: Bearer " + c.supabaseAnonKey + "\r\n";
        return headers;
    }

    /// Parse metadata from Edge Function response (camelCase keys)
    static RecipeMetadata parseMetadata (const juce::var& json)
    {
        RecipeMetadata m;
        m.recipeId        = json.getProperty ("recipeId", "").toString();
        m.title           = json.getProperty ("title", "").toString();
        m.description     = json.getProperty ("description", "").toString();
        m.authorName      = json.getProperty ("authorName", "Anonymous").toString();
        m.mood            = json.getProperty ("mood", "").toString();
        m.thumbsUp        = static_cast<int> (json.getProperty ("thumbsUp", 0));
        m.isStaffPick     = static_cast<bool> (json.getProperty ("staffPick", false));
        m.isRemix         = static_cast<bool> (json.getProperty ("isRemix", false));
        m.remixOfId       = json.getProperty ("remixOfId", "").toString();
        m.appVersion      = json.getProperty ("appVersion", "").toString();
        m.sharedTimestamp  = static_cast<int64_t> (json.getProperty ("timestamp", 0));

        if (auto* arr = json.getProperty ("engines", {}).getArray())
            for (const auto& e : *arr)
                m.engines.add (e.toString());

        if (auto* arr = json.getProperty ("tags", {}).getArray())
            for (const auto& t : *arr)
                m.tags.add (t.toString());

        return m;
    }

    /// Parse metadata from PostgREST row (snake_case keys)
    static RecipeMetadata parseMetadataFromRow (const juce::var& row)
    {
        RecipeMetadata m;
        m.recipeId        = row.getProperty ("recipe_id", "").toString();
        m.title           = row.getProperty ("title", "").toString();
        m.description     = row.getProperty ("description", "").toString();
        m.authorName      = row.getProperty ("author_name", "Anonymous").toString();
        m.mood            = row.getProperty ("mood", "").toString();
        m.thumbsUp        = static_cast<int> (row.getProperty ("thumbs_up", 0));
        m.isStaffPick     = static_cast<bool> (row.getProperty ("is_staff_pick", false));
        m.isRemix         = static_cast<bool> (row.getProperty ("is_remix", false));
        m.remixOfId       = row.getProperty ("remix_of_id", "").toString();
        m.appVersion      = row.getProperty ("app_version", "").toString();
        m.sharedTimestamp  = static_cast<int64_t> (juce::Time::fromISO8601 (
            row.getProperty ("shared_at", "").toString()).toMilliseconds());

        if (auto* arr = row.getProperty ("engines", {}).getArray())
            for (const auto& e : *arr)
                m.engines.add (e.toString());

        if (auto* arr = row.getProperty ("tags", {}).getArray())
            for (const auto& t : *arr)
                m.tags.add (t.toString());

        return m;
    }

    //--------------------------------------------------------------------------
    // Background thread helpers

    class BrowseThread : public juce::Thread
    {
    public:
        BrowseThread (SharedRecipeVault& v, BrowseFilter f)
            : juce::Thread ("VaultBrowse"), vault (v), filter (std::move (f)) {}

        ~BrowseThread() override { stopThread (5000); }

        void run() override
        {
            auto result = vault.browse (filter);
            juce::MessageManager::callAsync ([this, result, destroyedFlag = &vault.destroyed_]()
            {
                if (! destroyedFlag->load())
                {
                    if (vault.onBrowseComplete)
                        vault.onBrowseComplete (result);
                }
                delete this;
            });
        }

    private:
        SharedRecipeVault& vault;
        BrowseFilter filter;
    };

    class ShareThread : public juce::Thread
    {
    public:
        ShareThread (SharedRecipeVault& v, juce::String json,
                     juce::String desc, juce::StringArray t)
            : juce::Thread ("VaultShare"), vault (v),
              recipeJSON (std::move (json)), description (std::move (desc)),
              tags (std::move (t)) {}

        ~ShareThread() override { stopThread (5000); }

        void run() override
        {
            auto result = vault.shareRecipe (recipeJSON, description, tags);
            juce::MessageManager::callAsync ([this, result, destroyedFlag = &vault.destroyed_]()
            {
                if (! destroyedFlag->load())
                {
                    if (vault.onShareComplete)
                        vault.onShareComplete (result);
                }
                delete this;
            });
        }

    private:
        SharedRecipeVault& vault;
        juce::String recipeJSON, description;
        juce::StringArray tags;
    };

    class DownloadThread : public juce::Thread
    {
    public:
        DownloadThread (SharedRecipeVault& v, juce::String id)
            : juce::Thread ("VaultDownload"), vault (v), recipeId (std::move (id)) {}

        ~DownloadThread() override { stopThread (5000); }

        void run() override
        {
            auto result = vault.downloadRecipe (recipeId);
            juce::MessageManager::callAsync ([this, result, destroyedFlag = &vault.destroyed_]()
            {
                if (! destroyedFlag->load())
                {
                    if (vault.onDownloadComplete)
                        vault.onDownloadComplete (result);
                }
                delete this;
            });
        }

    private:
        SharedRecipeVault& vault;
        juce::String recipeId;
    };
};

} // namespace xoceanus
