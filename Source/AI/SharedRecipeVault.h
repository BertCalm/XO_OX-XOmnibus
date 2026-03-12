#pragma once
#include <juce_core/juce_core.h>
#include <functional>
#include <mutex>
#include <vector>

namespace xomnibus {

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
//   - Versioning: Recipes track which XOmnibus version they were created with
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
        juce::String appVersion;        // XOmnibus version it was created with
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
        juce::String apiBaseUrl;        // e.g. "https://api.xomnibus.com/vault/v1"
        juce::String displayName = "Anonymous";
        juce::String userToken;         // Auth token (from optional XO account)
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

    void configure (VaultConfig config)
    {
        std::lock_guard<std::mutex> lock (configMutex);
        cfg = std::move (config);
    }

    bool isConfigured() const
    {
        std::lock_guard<std::mutex> lock (configMutex);
        return cfg.apiBaseUrl.isNotEmpty();
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

        if (localCfg.apiBaseUrl.isEmpty())
            return { false, {}, "Vault not configured" };

        // Parse recipe to extract metadata
        auto recipe = juce::JSON::parse (recipeJSON);
        if (! recipe.isObject())
            return { false, {}, "Invalid recipe JSON" };

        // Build share payload
        auto payload = std::make_unique<juce::DynamicObject>();
        payload->setProperty ("recipe", recipe);
        payload->setProperty ("description", description);
        payload->setProperty ("authorName", localCfg.displayName);

        juce::Array<juce::var> tagArray;
        for (const auto& tag : tags)
            tagArray.add (tag);
        payload->setProperty ("tags", tagArray);

        payload->setProperty ("appVersion", "1.0.0");
        payload->setProperty ("timestamp", juce::Time::currentTimeMillis());

        juce::String jsonBody = juce::JSON::toString (juce::var (payload.release()));

        // Send
        juce::String endpoint = localCfg.apiBaseUrl + "/recipes";
        juce::URL url (endpoint);
        url = url.withPOSTData (jsonBody);

        auto headers = buildHeaders (localCfg);

        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostBody)
                           .withConnectionTimeoutMs (localCfg.timeoutMs)
                           .withExtraHeaders (headers);

        auto stream = url.createInputStream (options);

        if (stream == nullptr)
            return { false, {}, "Connection failed" };

        auto response = stream->readEntireStreamAsString();
        auto responseJSON = juce::JSON::parse (response);

        if (responseJSON.isObject())
        {
            auto id = responseJSON.getProperty ("recipeId", "").toString();
            if (id.isNotEmpty())
                return { true, id, {} };
        }

        return { false, {}, "Server error: " + response.substring (0, 200) };
    }

    //--------------------------------------------------------------------------
    // Browse community recipes (background thread)

    BrowseResult browse (const BrowseFilter& filter)
    {
        VaultConfig localCfg;
        {
            std::lock_guard<std::mutex> lock (configMutex);
            localCfg = cfg;
        }

        if (localCfg.apiBaseUrl.isEmpty())
            return { {}, 0, 0, false, "Vault not configured" };

        // Build query URL
        juce::String endpoint = localCfg.apiBaseUrl + "/recipes?";
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
                           .withExtraHeaders (buildHeaders (localCfg));

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

    DownloadResult downloadRecipe (const juce::String& recipeId)
    {
        VaultConfig localCfg;
        {
            std::lock_guard<std::mutex> lock (configMutex);
            localCfg = cfg;
        }

        if (localCfg.apiBaseUrl.isEmpty())
            return { false, {}, {}, "Vault not configured" };

        juce::String endpoint = localCfg.apiBaseUrl + "/recipes/"
                                + juce::URL::addEscapeChars (recipeId, true);

        juce::URL url (endpoint);
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                           .withConnectionTimeoutMs (localCfg.timeoutMs)
                           .withExtraHeaders (buildHeaders (localCfg));

        auto stream = url.createInputStream (options);
        if (stream == nullptr)
            return { false, {}, {}, "Connection failed" };

        auto response = stream->readEntireStreamAsString();
        auto json = juce::JSON::parse (response);

        if (! json.isObject())
            return { false, {}, {}, "Invalid response" };

        DownloadResult result;
        result.success = true;
        result.metadata = parseMetadata (json.getProperty ("metadata", {}));

        auto recipeData = json.getProperty ("recipe", {});
        result.recipeJSON = juce::JSON::toString (recipeData);

        return result;
    }

    //--------------------------------------------------------------------------
    // Thumbs up (background thread)

    bool thumbsUp (const juce::String& recipeId)
    {
        VaultConfig localCfg;
        {
            std::lock_guard<std::mutex> lock (configMutex);
            localCfg = cfg;
        }

        if (localCfg.apiBaseUrl.isEmpty()) return false;

        juce::String endpoint = localCfg.apiBaseUrl + "/recipes/"
                                + juce::URL::addEscapeChars (recipeId, true)
                                + "/thumbsup";

        juce::URL url (endpoint);
        url = url.withPOSTData ("{}");

        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostBody)
                           .withConnectionTimeoutMs (localCfg.timeoutMs)
                           .withExtraHeaders (buildHeaders (localCfg));

        auto stream = url.createInputStream (options);
        return stream != nullptr;
    }

    //--------------------------------------------------------------------------
    // Delete own recipe (background thread)

    bool deleteOwnRecipe (const juce::String& recipeId)
    {
        VaultConfig localCfg;
        {
            std::lock_guard<std::mutex> lock (configMutex);
            localCfg = cfg;
        }

        if (localCfg.apiBaseUrl.isEmpty() || localCfg.userToken.isEmpty())
            return false;

        // DELETE requires auth token — only works for user's own recipes
        juce::String endpoint = localCfg.apiBaseUrl + "/recipes/"
                                + juce::URL::addEscapeChars (recipeId, true);

        juce::URL url (endpoint);

        // Use custom header to signal DELETE (JUCE URL doesn't have native DELETE)
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                           .withConnectionTimeoutMs (localCfg.timeoutMs)
                           .withExtraHeaders (buildHeaders (localCfg)
                                              + "X-HTTP-Method-Override: DELETE\r\n");

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
    mutable std::mutex configMutex;
    VaultConfig cfg;

    static juce::String buildHeaders (const VaultConfig& c)
    {
        juce::String headers;
        headers += "Content-Type: application/json\r\n";
        headers += "X-XOmnibus-Vault: v1\r\n";
        if (c.userToken.isNotEmpty())
            headers += "Authorization: Bearer " + c.userToken + "\r\n";
        return headers;
    }

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
            juce::MessageManager::callAsync ([this, result]()
            {
                if (vault.onBrowseComplete)
                    vault.onBrowseComplete (result);
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
            juce::MessageManager::callAsync ([this, result]()
            {
                if (vault.onShareComplete)
                    vault.onShareComplete (result);
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
            juce::MessageManager::callAsync ([this, result]()
            {
                if (vault.onDownloadComplete)
                    vault.onDownloadComplete (result);
                delete this;
            });
        }

    private:
        SharedRecipeVault& vault;
        juce::String recipeId;
    };
};

} // namespace xomnibus
