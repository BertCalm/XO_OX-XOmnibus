// =============================================================================
// vault-search — Advanced recipe search with full-text, filtering, pagination
// =============================================================================
// Deploy: supabase functions deploy vault-search
// =============================================================================

import { createClient } from "https://esm.sh/@supabase/supabase-js@2";

const corsHeaders = {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Methods": "GET, OPTIONS",
  "Access-Control-Allow-Headers": "Content-Type, apikey, Authorization, X-XOceanus-Vault",
};

Deno.serve(async (req: Request) => {
  // CORS preflight
  if (req.method === "OPTIONS") {
    return new Response(null, { headers: corsHeaders });
  }

  try {
    const url = new URL(req.url);
    const params = url.searchParams;

    // Parse query parameters
    const page = parseInt(params.get("page") ?? "0");
    const pageSize = Math.min(parseInt(params.get("pageSize") ?? "20"), 50);
    const mood = params.get("mood") ?? "";
    const engine = params.get("engine") ?? "";
    const searchText = params.get("q") ?? "";
    const staffPicksOnly = params.get("staffPicks") === "true";
    const sort = params.get("sort") ?? "newest";

    // Connect to Supabase
    const supabaseUrl = Deno.env.get("SUPABASE_URL")!;
    const supabaseKey = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY")!;
    const supabase = createClient(supabaseUrl, supabaseKey);

    // Build query
    let query = supabase
      .from("shared_recipes")
      .select(
        "recipe_id, title, description, author_name, mood, engines, tags, " +
        "app_version, thumbs_up, is_staff_pick, is_remix, remix_of_id, shared_at",
        { count: "exact" }
      );

    // Apply filters
    if (mood) {
      query = query.eq("mood", mood);
    }

    if (engine) {
      query = query.contains("engines", [engine]);
    }

    if (staffPicksOnly) {
      query = query.eq("is_staff_pick", true);
    }

    if (searchText) {
      // Full-text search using the tsvector column
      query = query.textSearch("search_vector", searchText, {
        type: "websearch",
        config: "english",
      });
    }

    // Apply sorting
    switch (sort) {
      case "popular":
        query = query.order("thumbs_up", { ascending: false });
        break;
      case "staffpicks":
        query = query
          .order("is_staff_pick", { ascending: false })
          .order("thumbs_up", { ascending: false });
        break;
      case "newest":
      default:
        query = query.order("shared_at", { ascending: false });
        break;
    }

    // Pagination
    const from = page * pageSize;
    const to = from + pageSize - 1;
    query = query.range(from, to);

    const { data, count, error } = await query;

    if (error) {
      return new Response(
        JSON.stringify({ error: error.message }),
        { status: 500, headers: { ...corsHeaders, "Content-Type": "application/json" } }
      );
    }

    const totalResults = count ?? 0;
    const totalPages = Math.ceil(totalResults / pageSize);

    // Format response to match C++ BrowseResult expectations
    const recipes = (data ?? []).map((r: Record<string, unknown>) => ({
      recipeId: r.recipe_id,
      title: r.title,
      description: r.description,
      authorName: r.author_name,
      mood: r.mood,
      engines: r.engines,
      tags: r.tags,
      appVersion: r.app_version,
      thumbsUp: r.thumbs_up,
      staffPick: r.is_staff_pick,
      isRemix: r.is_remix,
      remixOfId: r.remix_of_id,
      timestamp: new Date(r.shared_at as string).getTime(),
    }));

    return new Response(
      JSON.stringify({ recipes, totalResults, totalPages }),
      { status: 200, headers: { ...corsHeaders, "Content-Type": "application/json" } }
    );
  } catch (err) {
    return new Response(
      JSON.stringify({ error: String(err) }),
      { status: 500, headers: { ...corsHeaders, "Content-Type": "application/json" } }
    );
  }
});
