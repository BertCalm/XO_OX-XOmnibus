# XPN Accessibility R&D
## Making XO_OX Packs Usable by the Broadest Range of MPC Producers

*2026-03-16*

---

## 1. Skill Level Tiering — Complexity Mode

The core tension: beginners need instant wins, advanced users need depth. The solution is a **Bank A / Bank B split** baked into every pack as a structural convention.

**Bank A (Pads 1–8): The Curated Layer**
Eight pads, hand-picked for immediate playability. Each pad has a clear sonic identity, obvious musical purpose, and works with the MPC's default settings out of the box. No macro-diving required. Labels are plain — "Kick," "Lead Hook," "Texture" — not engine jargon.

**Bank B (Pads 1–16): The Expert Kit**
The full 16-pad layout with all velocity layers, choke groups, Q-Link assignments, and deeper variations. Users who flip to Bank B get the complete character of the pack. The complexity is exposed, not hidden.

Document this convention in every pack README: *"Start in Bank A. When you're ready, Bank B has the full picture."* No content is removed for beginners — only the entry point changes.

---

## 2. Hardware Accessibility — MPC Model Tiers

Three relevant tiers in the current MPC lineup:

| Hardware | Price | RAM | Key Limitation |
|---|---|---|---|
| MPC One+ | ~$800 | 2GB | No MPCe quad, fewer simultaneous programs |
| MPC Live III | ~$1200 | 4GB | Full MPCe quad support |
| MPC X | ~$2000 | 4GB+ | Largest screen, full controller surface |

**Pack feature requirements by tier:**
- **8-velocity layers**: Requires Live III or MPC X. MPC One users should receive a 4-layer fallback variant in the same .xpn bundle — same sounds, trimmed layer count.
- **MPCe quad programs** (4-engine coupling): Live III only. Flag this clearly on pack listings. Don't remove quads from packs, but don't let it be a surprise.
- **Round-robin cycle groups**: Supported on all tiers. Safe to use freely.
- **Large sample counts (100+ per program)**: Test on MPC One before shipping. If load times exceed 8 seconds, thin the sample pool.

**Practical rule**: Every pack ships with an MPC One-compatible program as the baseline. Advanced programs are additive, not replacements.

---

## 3. Genre Accessibility — Common Ground Presets

XO_OX is experimental by identity. That's the brand. But experimental doesn't mean inaccessible — it means the gateway needs to be designed deliberately.

**Common Ground category**: 10% of each pack's presets (roughly 15 out of 150) should be presets that work immediately in hip-hop, trap, or R&B without modification. Not watered down — just calibrated. A bass preset from OVERBITE that hits like a classic 808. A pad from OPAL that sounds like a flip. These aren't compromises; they're bridges.

**Naming convention**: Common Ground presets get a `[CG]` tag in the preset name. Producers searching for accessible sounds can filter or scan by tag. Advanced users ignore the tag. The experimental character of the engine is still present — the preset is just mapped to familiar territory.

The goal is not to make XO_OX sound like everything else. It's to give a hip-hop producer one moment of "I could use this right now" before the experimental side reveals itself.

---

## 4. Price Accessibility — Free Tier Strategy

The $9–35 range is already accessible for most producers. For those who can't spend at all, three practical options:

**Monthly free preset drop**: One standalone .xpn preset released free on the first of each month. Hosted on GitHub releases (no Gumroad friction). Rotates across engines to expose the breadth of XO_OX. Twelve free presets per year adds up to a meaningful free library.

**GitHub-hosted singles**: Permanent free presets in the `XO_OX-XOmnibus` repo under a `FreePresets/` directory. Low maintenance, discoverable via GitHub search, builds goodwill in communities that distrust paywalls.

**ONSET flagship stays free**: The existing ONSET flagship pack as a permanent free entry point is the strongest single move. Keep it free, keep it updated. It functions as the best advertisement for paid packs.

Avoid a "freemium" model where free presets are intentionally inferior. Free presets should be genuinely good — they're marketing, not charity.

---

## 5. Documentation Accessibility — Scannable, Not Skimmable

Most producers don't read READMEs. Design documentation assuming the reader will spend 90 seconds on it.

**Structural fixes:**
- Lead with a 3-bullet "Quick Start" block at the top of every README
- Use headers that are questions: "Which MPC do I need?" not "Hardware Requirements"
- One screenshot or diagram per README showing pad layout — visual beats prose

**Video alternatives**: A 60-second screen recording per pack (phone-quality is fine) showing pad layout and three key sounds. Host on YouTube, link from README. Producers who don't read will watch 60 seconds.

**MPCE_SETUP.md**: Keep it as a reference document, but add a "TL;DR" box at the top: four steps, no jargon, copy-paste ready.

---

## 6. Language Accessibility — Minimum Viable Localization

Four MPC communities where non-English content would land:

- **Japan**: Active beat-making community, MPC hardware historically popular, Japanese-language YouTube beat tutorials have large followings
- **Brazil**: Large trap/funk carioca producer base, Portuguese-language MPC groups on Facebook and Discord
- **Germany**: Strong hardware synthesizer culture, German-language production forums
- **France**: Paris beat scene, French hip-hop production community

**Minimum viable localization**: Translate the pack Quick Start blocks only (3–5 sentences per pack). Machine translation + one native speaker review pass. Post translated Quick Starts as separate files: `README_JP.md`, `README_PT.md`. This is a weekend of work per language and removes the largest barrier for non-English producers.

Full documentation translation is a phase-two problem. Start with the 90-second Quick Start.
