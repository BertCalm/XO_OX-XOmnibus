# XPN Pack Distribution Channels — R&D

**Date**: 2026-03-16
**Status**: Research / Pre-launch planning
**Scope**: Where and how to distribute XO_OX XPN expansion packs (.xpn bundles + sample ZIPs)

---

## XPN Format Notes (applies to all channels)

MPC `.xpn` files are ZIP archives containing samples, programs (.xpm), and a manifest. File sizes typically range 50MB–2GB depending on sample depth. Most file-hosting platforms accept ZIP/binary uploads at these sizes, but a few (Gumroad free tier, Bandcamp) have practical limits worth noting. No format conversion is required for the buyer — they drag the .xpn into MPC Software or transfer to hardware SD card. This simplicity is a distribution advantage: no license managers, no installers.

---

## Channel Analysis

### 1. Gumroad

**Setup**: Account creation, Stripe or PayPal connect, upload product file (max 16GB per file on paid plan; free plan limited to $10 products and lower caps). Custom domain embed available.

**Revenue split**: 10% flat fee to Gumroad + Stripe processing (~2.9% + $0.30). Effective take-home ~87% on a $20 pack.

**Audience**: Gumroad's own discover feed is thin — the platform is neutral infrastructure. Audience quality depends entirely on traffic you drive. Strong email list integration (follow-seller feature builds a list automatically).

**SEO/Discoverability**: Low organic discovery through Gumroad's marketplace. Your SEO comes from external links pointing to your Gumroad page.

**Pros**: Fastest setup (hours not days). Email capture on every purchase. Handles EU VAT automatically. PDF/ZIP/binary all work. Embeds cleanly into XO-OX.org.

**Cons**: No built-in MPC audience. Discovery is zero unless you drive traffic. Branding is Gumroad-adjacent, not fully owned.

**XO-OX recommendation**: Primary storefront for Year 1. Embed on XO-OX.org. Every buyer becomes an email subscriber — this compounds.

---

### 2. MPC-Forums.com

**Setup**: Forum account required; marketplace access typically requires posting history/trust level. No known formal marketplace with payment processing — sales are negotiated via DM or external link (e.g., link to your Gumroad). Free section allows sharing freebie packs directly.

**Audience**: Highest intent of any channel. These are active MPC users who speak the format natively. Small but extremely qualified — a post in the right thread can convert at rates that dwarf broader platforms.

**SEO/Discoverability**: Forum threads index well in Google for MPC-specific searches ("MPC expansion free download," "MPC XPN soul samples"). Long tail but high purchase intent.

**Pros**: Zero cost to participate. Community trust transfers to brand. Free pack drops build reputation fast. Direct feedback loop from power users.

**Cons**: No payment infrastructure — you're still sending buyers to Gumroad/XO-OX.org. Requires genuine community participation, not just promotional posts.

**XO-OX recommendation**: Essential free channel. Drop one free pack in the free section before any paid launch. Participate in threads. Let MPC-Forums be the word-of-mouth engine, not the checkout.

---

### 3. Splice

**Setup**: Requires a formal partnership agreement with Splice. Content is ingested into their library and licensed under their subscription royalty pool model. Splice does not natively support `.xpn` format distribution — their model is individual sample/loop delivery, not MPC program bundles.

**XPN compatibility**: Splice would not distribute `.xpn` files as-is. Samples could theoretically be uploaded as individual WAVs and licensed through their royalty pool, but buyers would not receive the programmed kits or XOM engine mappings. The core value proposition of XO_OX packs (curated velocity layers, Q-Link assignments, Sonic DNA) would be lost.

**Audience**: Massive (millions of subscribers), but oriented toward DAW-based producers, not MPC hardware users specifically.

**Pros**: Volume exposure if samples are strong enough to surface in search.

**Cons**: Royalty pool payouts are low and opaque. XPN format not supported. Brand dilution — your packs become undifferentiated sample content. Partnership barrier is high for an independent label.

**XO-OX recommendation**: Skip for XPN packs. Revisit only if XO_OX releases a standalone sample library divorced from the MPC program format.

---

### 4. Drum Broker / MSXII

**Setup**: Both Drum Broker and MSXII (now operating as separate entities) have historically carried third-party packs on a consignment or revenue-share basis. Typical model: 50/50 or 60/40 split in favor of the creator, with the platform handling payment and hosting. Requires outreach, a pitch with demo content, and sometimes an exclusivity window.

**Audience**: Drum Broker reaches a broad beatmaker audience; MSXII skews toward MPC users specifically and has a strong community trust. Both have existing email lists in the tens of thousands.

**SEO/Discoverability**: These platforms rank for competitive search terms ("MPC drum kits," "drum broker expansion"). Placement here provides borrowed SEO authority.

**Pros**: Credibility signal. Their audience already buys. Zero marketing cost on your end once placed.

**Cons**: 40–50% revenue share. Less email capture for XO_OX. Dependent on their promotion schedule. Not guaranteed placement.

**XO-OX recommendation**: Target for Year 1 Q3/Q4 once the catalog has 3+ strong packs. Lead with one pack as a test placement. Keep Gumroad as primary — Drum Broker is a reach channel, not the core.

---

### 5. Direct on XO-OX.org

**Setup**: Technical minimum is Stripe Checkout (hosted payment links, no backend required) + a static file host for delivery (Cloudflare R2 or an S3 bucket with a time-limited signed URL). A Cloudflare Pages + Workers setup can handle the payment webhook and generate download links serverlessly. Alternatively, a Gumroad embed on XO-OX.org achieves the same UX with near-zero backend work.

**Pros**: Full brand control. No platform fees beyond Stripe (~3%). Buyer stays in the XO-OX ecosystem. Best for upsell flows (buy pack → see related engine page).

**Cons**: Technical overhead for custom storefront. Gumroad embed is 95% of the benefit at 10% of the build cost.

**XO-OX recommendation**: Use Gumroad embed on XO-OX.org for Year 1. Build a custom Stripe checkout in Year 2 when volume justifies it.

---

### 6. Bandcamp

**Setup**: Free account, 15% fee (drops to 10% after $5K in sales). File upload limit is 291MB per track equivalent for digital items — larger files require splitting or linking to external download. XPN bundles over 300MB would need a workaround (e.g., Dropbox/Google Drive link in the thank-you note).

**Audience**: Bandcamp skews toward music listeners and independent artists, not primarily beatmakers. MPC pack sales on Bandcamp exist but are niche — it works better for "artist pack" positioning (e.g., "beats in the style of...") than tool/expansion sales.

**Pros**: Strong "independent artist" brand alignment. Good for XO_OX's conceptual/mythology angle. Bandcamp Fridays (waived fees) are a marketing moment.

**Cons**: File size limit is a real friction point for XPN bundles. Audience mismatch for hardware MPC users. Discovery algorithm does not surface sound design packs prominently.

**XO-OX recommendation**: Optional secondary channel for smaller packs (sub-300MB) or curated "artist edition" releases. Not a priority for Year 1.

---

### 7. Patreon

**Setup**: Already in the XO_OX plan. Recommended mechanic: exclusive pack releases as patron posts with a direct ZIP/XPN download attached (Patreon supports up to 200MB file attachments natively; larger files should use a Google Drive or Dropbox link embedded in the post body). Tier structure suggestion: $5/month (field notes + free samples), $15/month (monthly pack), $30/month (all packs + early access).

**Pros**: Recurring revenue. Deepens community. Patrons become evangelists. Patreon's discovery feed surfaces music/audio creators reasonably well.

**Cons**: 8–12% Patreon fee. Requires consistent monthly output to retain subscribers. Large XPN files need external hosting.

**XO-OX recommendation**: Core channel. Launch alongside the first paid pack. Monthly cadence of one mini-pack or teaser for $15 tier keeps churn low.

---

### 8. Free Channels

**Reddit r/mpc**: ~50K members, high activity. Free pack posts with a Gumroad 0-price link perform well and capture emails. Rules allow self-promotion with community participation context.

**Discord (MPC community servers + XO_OX server)**: Best for real-time feedback and superfan cultivation. Drop WIP previews, get naming input, build the mythology in public.

**MPC-Forums free section**: See Channel 2. Essential.

**SEO value**: Free pack landing pages on XO-OX.org (one per free pack) with descriptive copy accumulate long-tail search traffic over time. "Free MPC soul expansion XPN download" is a winnable keyword cluster.

---

## Year 1 Recommended Channel Stack

**Priority 1 — Gumroad (embedded on XO-OX.org)**
Fastest path to revenue. Every sale builds the email list. Low overhead. This is the foundation everything else feeds into.

**Priority 2 — Patreon**
Recurring revenue stabilizes the operation. Launch simultaneously with the first paid pack. Use it to reward the community that forms around MPC-Forums and Discord.

**Priority 3 — MPC-Forums + Reddit r/mpc (free drops)**
One free pack on MPC-Forums before the first paid launch. This seeds word-of-mouth in the highest-intent community on the internet for this format. It costs one pack and generates disproportionate credibility.

**Hold for Q3/Q4**: Drum Broker outreach once 3+ packs exist. Bandcamp for select small releases. Splice: indefinitely deferred unless XPN format support changes.

The logic: own the relationship (Gumroad email list + Patreon), seed the community (free drops), and reach out to established platforms only when there is a catalog worth pitching.
