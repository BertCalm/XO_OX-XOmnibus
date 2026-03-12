-- =============================================================================
-- SharedRecipeVault — Community recipe sharing
-- =============================================================================
-- Run this in Supabase SQL Editor AFTER 001_community_insights.sql
-- =============================================================================

-- Shared recipes
CREATE TABLE IF NOT EXISTS shared_recipes (
    id              BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    recipe_id       UUID NOT NULL DEFAULT gen_random_uuid() UNIQUE,
    title           TEXT NOT NULL,
    description     TEXT NOT NULL DEFAULT '',
    author_name     TEXT NOT NULL DEFAULT 'Anonymous',
    author_token    TEXT,                          -- Hashed token for delete rights
    mood            TEXT NOT NULL DEFAULT '',
    engines         TEXT[] NOT NULL DEFAULT '{}',   -- Engine IDs used in recipe
    tags            TEXT[] NOT NULL DEFAULT '{}',
    app_version     TEXT NOT NULL DEFAULT '1.0.0',
    recipe_json     JSONB NOT NULL,                -- The full .xorecipe content
    thumbs_up       INT NOT NULL DEFAULT 0,
    is_staff_pick   BOOLEAN NOT NULL DEFAULT false,
    is_remix        BOOLEAN NOT NULL DEFAULT false,
    remix_of_id     UUID,                          -- FK to original recipe
    shared_at       TIMESTAMPTZ NOT NULL DEFAULT now(),

    -- Full-text search column (auto-populated by trigger)
    search_vector   TSVECTOR
);

-- Indexes for common query patterns
CREATE INDEX IF NOT EXISTS idx_recipes_mood
    ON shared_recipes (mood);

CREATE INDEX IF NOT EXISTS idx_recipes_shared_at
    ON shared_recipes (shared_at DESC);

CREATE INDEX IF NOT EXISTS idx_recipes_thumbs
    ON shared_recipes (thumbs_up DESC);

CREATE INDEX IF NOT EXISTS idx_recipes_staff
    ON shared_recipes (is_staff_pick)
    WHERE is_staff_pick = true;

CREATE INDEX IF NOT EXISTS idx_recipes_engines
    ON shared_recipes USING GIN (engines);

CREATE INDEX IF NOT EXISTS idx_recipes_tags
    ON shared_recipes USING GIN (tags);

CREATE INDEX IF NOT EXISTS idx_recipes_search
    ON shared_recipes USING GIN (search_vector);

-- =============================================================================
-- Full-text search trigger
-- =============================================================================

CREATE OR REPLACE FUNCTION update_recipe_search_vector()
RETURNS TRIGGER AS $$
BEGIN
    NEW.search_vector :=
        setweight(to_tsvector('english', COALESCE(NEW.title, '')), 'A') ||
        setweight(to_tsvector('english', COALESCE(NEW.description, '')), 'B') ||
        setweight(to_tsvector('english', COALESCE(array_to_string(NEW.tags, ' '), '')), 'B') ||
        setweight(to_tsvector('english', COALESCE(NEW.author_name, '')), 'C') ||
        setweight(to_tsvector('english', COALESCE(NEW.mood, '')), 'C');
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_recipe_search_vector
    BEFORE INSERT OR UPDATE ON shared_recipes
    FOR EACH ROW EXECUTE FUNCTION update_recipe_search_vector();

-- =============================================================================
-- Thumbs-up tracking (one per device per recipe, no login required)
-- =============================================================================

CREATE TABLE IF NOT EXISTS recipe_thumbs (
    id          BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    recipe_id   UUID NOT NULL REFERENCES shared_recipes (recipe_id) ON DELETE CASCADE,
    voter_hash  TEXT NOT NULL,  -- Hash of some anonymous identifier (not PII)
    voted_at    TIMESTAMPTZ NOT NULL DEFAULT now(),
    UNIQUE (recipe_id, voter_hash)
);

-- Function to increment thumbs_up and prevent double-voting
CREATE OR REPLACE FUNCTION thumbs_up_recipe(
    p_recipe_id UUID,
    p_voter_hash TEXT
)
RETURNS BOOLEAN AS $$
DECLARE
    already_voted BOOLEAN;
BEGIN
    -- Check if already voted
    SELECT EXISTS(
        SELECT 1 FROM recipe_thumbs
        WHERE recipe_id = p_recipe_id AND voter_hash = p_voter_hash
    ) INTO already_voted;

    IF already_voted THEN
        RETURN false;
    END IF;

    -- Record vote
    INSERT INTO recipe_thumbs (recipe_id, voter_hash)
    VALUES (p_recipe_id, p_voter_hash);

    -- Increment counter
    UPDATE shared_recipes
    SET thumbs_up = thumbs_up + 1
    WHERE recipe_id = p_recipe_id;

    RETURN true;
END;
$$ LANGUAGE plpgsql;

-- =============================================================================
-- Row Level Security
-- =============================================================================
ALTER TABLE shared_recipes ENABLE ROW LEVEL SECURITY;
ALTER TABLE recipe_thumbs ENABLE ROW LEVEL SECURITY;

-- Anyone can read recipes
CREATE POLICY "anon_read_recipes"
    ON shared_recipes FOR SELECT
    TO anon
    USING (true);

-- Anyone can share a recipe (INSERT)
CREATE POLICY "anon_insert_recipes"
    ON shared_recipes FOR INSERT
    TO anon
    WITH CHECK (true);

-- Only the author can delete (matched by hashed token)
CREATE POLICY "author_delete_recipes"
    ON shared_recipes FOR DELETE
    TO anon
    USING (author_token IS NOT NULL AND author_token = current_setting('request.headers', true)::json->>'x-author-token');

-- Thumbs: anyone can insert (the function handles dedup)
CREATE POLICY "anon_insert_thumbs"
    ON recipe_thumbs FOR INSERT
    TO anon
    WITH CHECK (true);

CREATE POLICY "anon_read_thumbs"
    ON recipe_thumbs FOR SELECT
    TO anon
    USING (true);

-- =============================================================================
-- Helpful views for your dashboard
-- =============================================================================

-- Top recipes by thumbs
CREATE OR REPLACE VIEW top_recipes AS
SELECT
    recipe_id, title, author_name, mood, engines, tags,
    thumbs_up, is_staff_pick, shared_at
FROM shared_recipes
ORDER BY thumbs_up DESC
LIMIT 100;

-- Engine popularity from shared recipes
CREATE OR REPLACE VIEW vault_engine_popularity AS
SELECT
    engine,
    COUNT(*) AS recipe_count
FROM shared_recipes, unnest(engines) AS engine
GROUP BY engine
ORDER BY recipe_count DESC;

-- Mood distribution
CREATE OR REPLACE VIEW vault_mood_distribution AS
SELECT
    mood,
    COUNT(*) AS recipe_count,
    ROUND(AVG(thumbs_up), 1) AS avg_thumbs
FROM shared_recipes
WHERE mood != ''
GROUP BY mood
ORDER BY recipe_count DESC;

-- Most popular engine combinations
CREATE OR REPLACE VIEW vault_engine_combos AS
SELECT
    engines,
    COUNT(*) AS combo_count,
    ROUND(AVG(thumbs_up), 1) AS avg_thumbs
FROM shared_recipes
GROUP BY engines
ORDER BY combo_count DESC
LIMIT 50;
