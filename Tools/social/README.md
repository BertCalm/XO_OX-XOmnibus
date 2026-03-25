# XOlokun Sustainable Social Engine

Automated content pipeline for XOlokun's social presence.
All scripts use Python stdlib only — no pip installs required.

---

## Overview

The pipeline runs once per week (Sunday assembly) and produces ready-to-post files for Twitter, Reddit, Discord, Patreon, and forums. A daily signal cron job posts scripture verses automatically.

```
Sunday assembly:
  extract_content.py  →  format_posts.py  →  output/week-NNN/
  (or just: generate_week.py does both)

Daily:
  daily_signal.py  →  stdout  →  discord_webhook.py (optional)

Monthly:
  compile_digest.py  →  output/digests/
```

---

## Scripts

### `generate_week.py` — Master weekly script

Runs extraction + formatting in one command. Start here every Sunday.

```bash
# Standard Sunday assembly
python3 Tools/social/generate_week.py

# Also post the surface Discord post immediately
python3 Tools/social/generate_week.py --post-discord

# Preview without writing files
python3 Tools/social/generate_week.py --dry-run

# Only generate surface-depth posts
python3 Tools/social/generate_week.py --depth surface

# Override week (for backfilling)
python3 Tools/social/generate_week.py --week 2026-W12
```

Output: `output/week-NNN/` with files named `{platform}-{zone}.txt`.

---

### `extract_content.py` — Content extraction

Reads git log, counts new presets, picks a recipe (round-robin), selects a scripture verse, reads breadcrumbs.

```bash
python3 Tools/social/extract_content.py
python3 Tools/social/extract_content.py --week 2026-W12
python3 Tools/social/extract_content.py --dry-run   # prints JSON, no files written
```

Output: `output/content-{YYYY-WNN}.json`

**State:** Tracks recipe rotation and used verses in `output/.state.json`. Delete this file to reset.

---

### `format_posts.py` — Post formatter

Reads a content JSON and templates, generates platform-specific posts.

```bash
python3 Tools/social/format_posts.py
python3 Tools/social/format_posts.py --week 2026-W12
python3 Tools/social/format_posts.py --depth twilight
python3 Tools/social/format_posts.py --dry-run      # prints to stdout
```

---

### `daily_signal.py` — Daily scripture verse

Picks an unused verse from all scripture files, formats it for the target platform.

```bash
# Print today's verse in Discord format (default)
python3 Tools/social/daily_signal.py

# Twitter format
python3 Tools/social/daily_signal.py --format twitter

# Preview 5 upcoming verses without updating state
python3 Tools/social/daily_signal.py --preview 5

# Check pool statistics
python3 Tools/social/daily_signal.py --stats

# Pipe directly to Discord
python3 Tools/social/daily_signal.py | python3 Tools/social/discord_webhook.py --stdin
```

**Cron setup (8am daily):**

```cron
0 8 * * * cd /path/to/XO_OX-XOmnibus && python3 Tools/social/daily_signal.py | python3 Tools/social/discord_webhook.py --stdin >> Tools/social/output/daily-signals.log 2>&1
```

---

### `discord_webhook.py` — Discord poster

Posts any message string to a Discord channel via webhook.

```bash
# Direct message
python3 Tools/social/discord_webhook.py "Message here"

# From stdin (pipe-friendly)
python3 Tools/social/discord_webhook.py --stdin

# From file
python3 Tools/social/discord_webhook.py --file output/week-001/discord-surface.txt

# Dry run (no POST)
python3 Tools/social/discord_webhook.py "Test" --dry-run
```

**Environment variable required:**

```bash
export DISCORD_WEBHOOK_URL="https://discord.com/api/webhooks/YOUR_WEBHOOK_ID/YOUR_TOKEN"
```

Get a webhook URL: Discord server → Channel Settings → Integrations → Webhooks → New Webhook.

Optional overrides:
```bash
export DISCORD_USERNAME="XOlokun"   # Bot display name
export DISCORD_AVATAR_URL="..."     # Bot avatar
```

---

### `compile_digest.py` — Monthly digest

Compiles the last 4 weeks of posts into a Patreon-ready monthly update.

```bash
# Current month digest (Patreon format)
python3 Tools/social/compile_digest.py

# Specific month
python3 Tools/social/compile_digest.py --month 2026-04

# Discord format
python3 Tools/social/compile_digest.py --month 2026-03 --format discord

# Custom week range
python3 Tools/social/compile_digest.py --weeks 1 2 3 4

# Preview without writing
python3 Tools/social/compile_digest.py --dry-run
```

Output: `output/digests/digest-{month}-{format}.txt`

---

## Sunday Assembly Workflow

1. **Drop notes** into `breadcrumbs/` as `.txt` files (optional):
   ```
   breadcrumbs/note-2026-03-30.txt
   ```
   Anything in this file appears as `{breadcrumb}` in Abyss-zone posts.

2. **Run the assembly:**
   ```bash
   python3 Tools/social/generate_week.py
   ```

3. **Review posts** in `output/week-NNN/`:
   - `twitter-surface.txt` → tweet
   - `twitter-twilight.txt` → thread follow-up
   - `reddit-twilight.txt` → r/XO_OX or relevant sub
   - `reddit-midnight.txt` → technical communities
   - `discord-surface.txt` → #general or #weekly-signal
   - `discord-twilight.txt` → #coupling-lab
   - `discord-midnight.txt` → #build-log
   - `discord-abyss.txt` → #the-deep (if channel exists)
   - `patreon-midnight.txt` → Patreon weekly update
   - `forum-twilight.txt` → KVR / Elektronauts

4. **Edit** any file directly before posting — the generated text is a starting point.

5. **Post.** Paste or schedule via your preferred tool (Buffer, Typefully, etc.).

6. On a slow week, use a **skip card** instead:
   ```bash
   # See options for today
   cat Tools/social/skip-cards/monday.json
   ```

---

## Skip Cards

When there's nothing substantial to post, skip cards provide ready-made alternatives.
Each day has 4 options across depth zones:

```
skip-cards/monday.json    — Open-week energy
skip-cards/tuesday.json   — Coupling type education
skip-cards/wednesday.json — Midweek lore + WIP prompts
skip-cards/thursday.json  — Deep listening + developer content
skip-cards/friday.json    — Week wrap + UGC prompts
```

Usage: `cat Tools/social/skip-cards/friday.json` and pick any option.

---

## Depth Zones

Every post is tagged with a depth zone determining audience and tone:

| Zone | Audience | Tone | Channels |
|------|----------|------|----------|
| **Surface** | General / casual | No jargon, broad appeal | Twitter, Discord #general |
| **Twilight** | Working producers | Some terminology OK | Reddit, Discord #coupling-lab, Forums |
| **Midnight** | Technical users | DSP/architecture detail | Reddit #deep-dive, Discord #build-log |
| **Abyss** | Inner community | Lore, scripture, ritual | Discord #the-deep, Patreon inner circle |

---

## Directional Reef (Cross-Platform Links)

The pipeline enforces cross-platform references automatically:
- Twitter posts link to Reddit (more depth)
- Reddit posts link to Discord (community)
- Discord posts link to Patreon (support)

These are set in `format_posts.py` at the top:
```python
REDDIT_LINK  = "reddit.com/r/XO_OX"
DISCORD_LINK = "discord.gg/xolokun"
PATREON_LINK = "patreon.com/cw/XO_OX"
```
Update these when channels are finalized.

---

## Templates

Templates live in `templates/`. Each file has sections per depth zone:

```markdown
## Surface Zone
Post content with {variables}

## Twilight Zone
Different content, more detail
```

Edit templates to adjust tone, length, or hashtags. Variables are documented at the top of each template file.

---

## Breadcrumbs

Drop `.txt` files in `breadcrumbs/` at any point during the week.
The pipeline picks up files modified in the last 7 days.

Example:
```
breadcrumbs/note-2026-03-30.txt
---
Spent the session working on the Cellar quad. OLATE's fretless glide combined
with OGRE's sub-bass coupling creates something you feel more than hear.
```

The first line becomes `{breadcrumb}` in Abyss-zone templates.

---

## State Files

Two state files in `output/`:
- `.state.json` — recipe rotation index + used verse hashes + week counter
- `.daily_signal_state.json` — used verse hashes for daily signal

Delete either file to reset that rotation.

---

## Directory Structure

```
Tools/social/
├── extract_content.py       # Step 1: git log, presets, recipe, verse
├── format_posts.py          # Step 2: fill templates, write post files
├── daily_signal.py          # Daily verse: stdout → webhook
├── discord_webhook.py       # Post any message to Discord
├── generate_week.py         # Master: runs extract + format
├── compile_digest.py        # Monthly digest for Patreon
├── templates/
│   ├── twitter.md           # Surface + Twilight
│   ├── reddit.md            # Twilight + Midnight
│   ├── discord.md           # All 4 zones
│   ├── patreon.md           # Midnight
│   └── forum.md             # Twilight (KVR, Elektronauts)
├── skip-cards/
│   ├── monday.json
│   ├── tuesday.json
│   ├── wednesday.json
│   ├── thursday.json
│   └── friday.json
├── breadcrumbs/             # Drop .txt notes here during the week
├── output/                  # Generated files (git-ignored)
│   ├── .state.json          # Recipe/verse rotation state
│   ├── .daily_signal_state.json
│   ├── content-YYYY-WNN.json
│   ├── week-NNN/
│   │   ├── index.json
│   │   ├── twitter-surface.txt
│   │   ├── discord-surface.txt
│   │   └── ...
│   ├── digests/
│   └── daily-signals.log
└── README.md
```
