# XOlokun Server Setup — Supabase

This sets up the backend for **CommunityInsights** (anonymous telemetry) and
**SharedRecipeVault** (community recipe sharing).

Total time: ~15 minutes. No server to manage. Free tier handles thousands of users.

---

## 1. Create a Supabase Project

1. Go to [supabase.com](https://supabase.com) and sign up (free)
2. Click **New Project**
3. Name it `xolokun` (or whatever you like)
4. Set a strong database password (save it somewhere)
5. Choose a region close to your users
6. Wait ~2 minutes for provisioning

## 2. Get Your Keys

From your project dashboard:

1. Go to **Settings → API**
2. Copy these two values:
   - **Project URL**: `https://abcdefg.supabase.co`
   - **anon/public key**: `eyJhbGciOiJI...` (the long one under "Project API keys")

These go into the C++ `SupabaseConfig` / `VaultConfig` structs. The anon key is
safe to embed in the app — Row Level Security controls what it can do.

## 3. Run the Database Migrations

1. Go to **SQL Editor** in the Supabase dashboard
2. Click **New Query**
3. Paste the contents of `Server/supabase/migrations/001_community_insights.sql`
4. Click **Run** — you should see "Success"
5. Click **New Query** again
6. Paste the contents of `Server/supabase/migrations/002_shared_recipe_vault.sql`
7. Click **Run** — you should see "Success"

Verify in **Table Editor** that you now have:
- `insight_batches` table
- `shared_recipes` table
- `recipe_thumbs` table

## 4. Deploy Edge Functions

Edge Functions handle the advanced search and thumbs-up logic.

### Install the Supabase CLI

```bash
# macOS
brew install supabase/tap/supabase

# npm (any platform)
npm install -g supabase
```

### Link and Deploy

```bash
cd Server/supabase

# Login to Supabase
supabase login

# Link to your project (use the project ref from your dashboard URL)
supabase link --project-ref abcdefg

# Deploy the functions
supabase functions deploy vault-search
supabase functions deploy vault-thumbsup
```

### Verify

```bash
# Test the search function (should return empty results)
curl "https://abcdefg.supabase.co/functions/v1/vault-search?page=0&pageSize=5" \
  -H "apikey: YOUR_ANON_KEY"
```

## 5. Configure XOlokun

In your C++ code (e.g., in the processor or editor initialization):

```cpp
// CommunityInsights
communityInsights.setSupabaseConfig ({
    "https://abcdefg.supabase.co",  // projectUrl
    "eyJhbGciOiJI..."               // anonKey
});

// SharedRecipeVault
recipeVault.configure ({
    "https://abcdefg.supabase.co",  // supabaseUrl
    "eyJhbGciOiJI...",              // supabaseAnonKey
    "Anonymous",                     // displayName
    deviceDerivedHash,               // authorToken (for delete rights)
    15000                            // timeoutMs
});
```

The anon key is safe to include in release builds — Row Level Security ensures:
- Insights: write-only (app can INSERT, only you can SELECT via dashboard)
- Recipes: read + write (app can browse + share, only author can delete)
- Thumbs: write-only with dedup (one vote per device per recipe)

## 6. View Your Data

### Supabase Dashboard (built-in)

- **Table Editor**: Browse raw data, filter, sort
- **SQL Editor**: Run custom queries

### Quick Queries

```sql
-- What engines are people using most?
SELECT * FROM engine_popularity;

-- What sounds are people asking AI to make?
SELECT * FROM ai_theme_totals;

-- Sweet vs grit?
SELECT * FROM vibe_knob_stats;

-- Top community recipes
SELECT * FROM top_recipes;

-- Most popular engine combos in shared recipes
SELECT * FROM vault_engine_combos;

-- What moods are trending?
SELECT * FROM vault_mood_distribution;

-- Weekly insights summary
SELECT
    date_trunc('week', received_at) AS week,
    COUNT(*) AS batches,
    SUM(total_sessions) AS sessions,
    SUM(ai_total) AS ai_queries
FROM insight_batches
GROUP BY week
ORDER BY week DESC;
```

## 7. Optional: Set Up Alerts

In Supabase Dashboard → **Database → Webhooks**, you can trigger
notifications when interesting things happen:

- New recipe shared → Slack notification
- Insights batch received → aggregate daily
- Recipe gets 10+ thumbs → flag for staff pick review

---

## Architecture Summary

```
XOlokun App (C++)
  │
  ├── CommunityInsights ──POST──→ Supabase PostgREST ──→ insight_batches table
  │                                                        └── Dashboard views
  │
  └── SharedRecipeVault
       ├── Share ──POST──→ PostgREST ──→ shared_recipes table
       ├── Browse ──GET──→ vault-search Edge Function ──→ Full-text search
       ├── Download ──GET──→ PostgREST ──→ recipe_json column
       ├── Thumbs Up ──POST──→ vault-thumbsup Edge Function ──→ Dedup + count
       └── Delete ──DELETE──→ PostgREST + RLS ──→ Author-only delete
```

## Costs

Supabase free tier includes:
- 500 MB database storage
- 5 GB bandwidth / month
- 500K Edge Function invocations / month
- Unlimited API requests

This comfortably handles tens of thousands of XOlokun users. If you outgrow
it, Supabase Pro is $25/month with 8 GB storage and 250 GB bandwidth.
