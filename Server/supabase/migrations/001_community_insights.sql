-- =============================================================================
-- CommunityInsights — Anonymous telemetry storage
-- =============================================================================
-- Run this in Supabase SQL Editor (Dashboard → SQL → New Query)
-- =============================================================================

-- Insight batches: one row per periodic upload from a user session
CREATE TABLE IF NOT EXISTS insight_batches (
    id              BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    batch_id        UUID NOT NULL DEFAULT gen_random_uuid(),
    app_version     TEXT NOT NULL DEFAULT '1.0.0',
    platform        TEXT NOT NULL DEFAULT 'Unknown',
    received_at     TIMESTAMPTZ NOT NULL DEFAULT now(),

    -- Engine usage (21 engines)
    engine_slot_counts      INT[] NOT NULL DEFAULT '{}',
    engine_primary_counts   INT[] NOT NULL DEFAULT '{}',
    total_sessions          INT NOT NULL DEFAULT 0,

    -- Coupling usage (12 types)
    coupling_type_counts    INT[] NOT NULL DEFAULT '{}',
    coupling_total_routes   INT NOT NULL DEFAULT 0,

    -- Sonic DNA histograms (5 bins each)
    dna_brightness  INT[] NOT NULL DEFAULT '{}',
    dna_warmth      INT[] NOT NULL DEFAULT '{}',
    dna_movement    INT[] NOT NULL DEFAULT '{}',
    dna_density     INT[] NOT NULL DEFAULT '{}',
    dna_space       INT[] NOT NULL DEFAULT '{}',
    dna_aggression  INT[] NOT NULL DEFAULT '{}',
    dna_presets_saved INT NOT NULL DEFAULT 0,

    -- Recipe usage (6 moods)
    recipe_mood_counts          INT[] NOT NULL DEFAULT '{}',
    recipe_factory_loaded       INT NOT NULL DEFAULT 0,
    recipe_user_created         INT NOT NULL DEFAULT 0,
    recipe_modified_after_load  INT NOT NULL DEFAULT 0,

    -- AI query themes
    ai_bass         INT NOT NULL DEFAULT 0,
    ai_pad          INT NOT NULL DEFAULT 0,
    ai_lead         INT NOT NULL DEFAULT 0,
    ai_fx           INT NOT NULL DEFAULT 0,
    ai_percussion   INT NOT NULL DEFAULT 0,
    ai_ambient      INT NOT NULL DEFAULT 0,
    ai_aggressive   INT NOT NULL DEFAULT 0,
    ai_experimental INT NOT NULL DEFAULT 0,
    ai_total        INT NOT NULL DEFAULT 0,

    -- FX usage (18 stages)
    fx_stage_counts     INT[] NOT NULL DEFAULT '{}',
    fx_vibe_sweet       INT NOT NULL DEFAULT 0,
    fx_vibe_grit        INT NOT NULL DEFAULT 0
);

-- Index for time-based queries (dashboards, weekly reports)
CREATE INDEX IF NOT EXISTS idx_insights_received
    ON insight_batches (received_at DESC);

-- Index for version-based filtering
CREATE INDEX IF NOT EXISTS idx_insights_version
    ON insight_batches (app_version);

-- =============================================================================
-- Aggregate views for quick dashboard queries
-- =============================================================================

-- Total engine popularity across all batches
CREATE OR REPLACE VIEW engine_popularity AS
SELECT
    unnest(ARRAY[
        'OddfeliX','OddOscar','Overdub','Odyssey','Oblong','Obese','Onset',
        'Overworld','Opal','Orbital','Organon','Ouroboros','Obsidian',
        'Overbite','Origami','Oracle','Obscura','Oceanic','Optic','Oblique','_reserved'
    ]) AS engine_name,
    unnest(ARRAY(
        SELECT COALESCE(SUM(engine_slot_counts[i]), 0)
        FROM insight_batches, generate_series(1, 21) AS i
        GROUP BY i ORDER BY i
    )) AS total_loads;

-- AI query theme totals
CREATE OR REPLACE VIEW ai_theme_totals AS
SELECT
    SUM(ai_bass) AS bass,
    SUM(ai_pad) AS pad,
    SUM(ai_lead) AS lead,
    SUM(ai_fx) AS fx,
    SUM(ai_percussion) AS percussion,
    SUM(ai_ambient) AS ambient,
    SUM(ai_aggressive) AS aggressive,
    SUM(ai_experimental) AS experimental,
    SUM(ai_total) AS total
FROM insight_batches;

-- VibeKnob sweet vs grit
CREATE OR REPLACE VIEW vibe_knob_stats AS
SELECT
    SUM(fx_vibe_sweet) AS total_sweet,
    SUM(fx_vibe_grit) AS total_grit,
    CASE WHEN SUM(fx_vibe_sweet) + SUM(fx_vibe_grit) > 0
         THEN ROUND(100.0 * SUM(fx_vibe_sweet) / (SUM(fx_vibe_sweet) + SUM(fx_vibe_grit)), 1)
         ELSE 0 END AS sweet_pct
FROM insight_batches;

-- =============================================================================
-- Row Level Security — insights are write-only from anon, read-only for admin
-- =============================================================================
ALTER TABLE insight_batches ENABLE ROW LEVEL SECURITY;

-- Anyone can INSERT (anonymous telemetry)
CREATE POLICY "anon_insert_insights"
    ON insight_batches FOR INSERT
    TO anon
    WITH CHECK (true);

-- Only authenticated service role can SELECT (your dashboard)
CREATE POLICY "service_read_insights"
    ON insight_batches FOR SELECT
    TO authenticated
    USING (true);

-- Nobody can UPDATE or DELETE via API
-- (you can always query via Supabase Dashboard which uses service_role)
