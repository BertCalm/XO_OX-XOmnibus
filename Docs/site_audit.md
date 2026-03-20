# Site Directory Audit

**Generated**: 2026-03-19 Sonnet Audit  
**Directory**: `site/`  
**Pages**: 7 HTML files

---

## Internal Link Integrity ✅

All internal `href` references resolve to existing files:

| Link | File Exists? |
|------|-------------|
| `index.html` | ✅ |
| `aquarium.html` | ✅ |
| `guide.html` | ✅ |
| `guide-oracle.html` | ✅ |
| `packs.html` | ✅ |
| `manifesto.html` | ✅ |
| `updates.html` | ✅ |

**No broken internal links.**

---

## Placeholder / TODO Content ✅

**No** `[TODO]`, `lorem ipsum`, `coming soon`, or `[TBD]` text found in any page body.

One CSS `::placeholder` found in `index.html:1451` — this is a form input styling pseudo-element, not placeholder content. Correct.

---

## Audio Assets ⚠️ KNOWN GAP

`index.html` references audio files dynamically:

```js
// Tries real samples first (site/audio/{engine}.mp3), falls back to Web Audio
const resp = await fetch(`audio/${name}.mp3`, { method: 'HEAD' });
```

**Status**: `site/audio/` directory does not exist. The code falls back gracefully to Web Audio API synthesis if files are absent.

**This is a known pending item**: See `memory/site-sample-recordings.md` — 20 hero preset clips need to be recorded and placed in `site/audio/`. Not a bug, expected gap.

---

## Static Image Assets ✅

No static image files referenced in site HTML. The site uses CSS-only styling and inline SVG/emoji — no broken image links.

---

## External URLs

No audit performed on external URLs (CDN, Patreon, etc.) — requires network access. Known issue: Patreon URL is still placeholder (`patreon.com/xoox`) — update when real URL is confirmed.

---

## Summary

| Check | Result |
|-------|--------|
| Internal links | ✅ All valid |
| Placeholder content | ✅ Clean |
| Audio assets | ⚠️ Missing (known, graceful fallback) |
| Image assets | ✅ None referenced |
| Large files in site/ | ✅ None |

**Site is clean.** Only open item is the audio preview clips (tracked separately).

