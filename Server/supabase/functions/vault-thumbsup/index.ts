// =============================================================================
// vault-thumbsup — Rate-limited thumbs-up with dedup
// =============================================================================
// Deploy: supabase functions deploy vault-thumbsup
// =============================================================================

import { createClient } from "https://esm.sh/@supabase/supabase-js@2";
import { createHash } from "node:crypto";

const corsHeaders = {
  "Access-Control-Allow-Origin": "https://xo-ox.org",
  "Access-Control-Allow-Methods": "POST, OPTIONS",
  "Access-Control-Allow-Headers": "Content-Type, apikey, Authorization, X-XOceanus-Vault",
};

Deno.serve(async (req: Request) => {
  if (req.method === "OPTIONS") {
    return new Response(null, { headers: corsHeaders });
  }

  if (req.method !== "POST") {
    return new Response(
      JSON.stringify({ error: "POST required" }),
      { status: 405, headers: { ...corsHeaders, "Content-Type": "application/json" } }
    );
  }

  try {
    const body = await req.json();
    const recipeId = body.recipeId;

    if (!recipeId) {
      return new Response(
        JSON.stringify({ error: "recipeId required" }),
        { status: 400, headers: { ...corsHeaders, "Content-Type": "application/json" } }
      );
    }

    const supabaseUrl = Deno.env.get("SUPABASE_URL")!;
    // Using anon key — RLS policies handle access control
    const supabaseKey = Deno.env.get("SUPABASE_ANON_KEY")!;
    const supabase = createClient(supabaseUrl, supabaseKey);

    // Derive voterHash server-side from the client IP + recipeId + a server secret.
    // This prevents clients from forging or replaying arbitrary hashes.
    const voterSecret = Deno.env.get("VOTER_HASH_SECRET") ?? "xo-ox-vault-default-secret";
    const clientIp = req.headers.get("x-forwarded-for")?.split(",")[0].trim()
      ?? req.headers.get("cf-connecting-ip")
      ?? "unknown";
    const voterHash = createHash("sha256")
      .update(voterSecret + "|" + clientIp + "|" + recipeId)
      .digest("hex");

    // Call the dedup function
    const { data, error } = await supabase.rpc("thumbs_up_recipe", {
      p_recipe_id: recipeId,
      p_voter_hash: voterHash,
    });

    if (error) {
      return new Response(
        JSON.stringify({ error: error.message }),
        { status: 500, headers: { ...corsHeaders, "Content-Type": "application/json" } }
      );
    }

    const wasNew = data === true;

    return new Response(
      JSON.stringify({ success: true, newVote: wasNew }),
      { status: 200, headers: { ...corsHeaders, "Content-Type": "application/json" } }
    );
  } catch (err) {
    console.error("vault-thumbsup error:", err);
    return new Response(
      JSON.stringify({ error: "Operation failed. Please try again." }),
      { status: 500, headers: { ...corsHeaders, "Content-Type": "application/json" } }
    );
  }
});
