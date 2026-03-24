# XOstinato — Concept Brief

**Date:** March 12, 2026
**Phase:** 0 — Ideation complete
**Status:** Approved for Phase 1 architecture

---

## Identity

- **Name:** XOstinato
- **Gallery code:** OSTINATO
- **Thesis:** XOstinato is a communal drum circle engine where 8 physically-modeled world percussion instruments play interlocking ostinato patterns — and you can sit down and join.
- **Sound family:** Rhythmic / Percussive / World / Groove
- **Unique capability:** A self-playing world drum circle with inter-seat interaction. Djembe, tabla, taiko, and beatbox players reacting to each other's accents in real time. The circle is always playing. You sit down and join. You leave, it carries on.
- **Values:** Multiculturalism, peace, unity, love, community, family

---

## Character

- **Personality in 3 words:** Communal, Welcoming, Alive
- **Engine approach:** Hybrid physical modeling — noise/impulse exciter → modal membrane resonators → waveguide body cavity → radiation filter
- **Why hybrid serves the character:**
  A drum circle is acoustic, physical, human. The synthesis must feel like real hands on real skin on real wood. Modal resonators capture the membrane expressiveness. Waveguide bodies give each drum its physical cavity — the goblet of a djembe, the barrel of a taiko, the box of a cajón. Together they produce the organic, living quality that samples can't match and pure synthesis can't reach.
- **The pattern thesis:**
  An ostinato is a repeating musical pattern. The magic of a drum circle comes from how individual ostinato patterns interlock, breathe, and evolve together. The engine doesn't just synthesize drums — it synthesizes the communal interaction between players.

---

## The XO Concept Test

1. **XO word:** XOstinato ✓ — Ostinato: a repeating musical pattern, the heartbeat of every drum circle
2. **One-sentence thesis:** "XOstinato is a communal drum circle engine where 8 physically-modeled world percussion instruments play interlocking ostinato patterns — and you can sit down and join." ✓
3. **Sound only this can make:** A self-playing world drum circle with physically-modeled instruments reacting to each other — djembe call triggering doumbek response while taiko provides thunder underneath. No DAW plugin or XOlokun engine does this. ✓

---

## Gallery Gap Filled

| Existing engines | Synthesis dimension |
|-----------------|---------------------|
| ONSET | Algorithmic drum synthesis (drum machine) |
| **OSTINATO** | **Communal world percussion (drum gathering)** |

ONSET is a drum machine. OSTINATO is a drum gathering.

---

## Key Design Decisions

- **8 seats** in a circle, any instrument in any seat (no tradition restrictions)
- **12 instruments** × 3-4 articulations each (48 total articulations)
- **96 patterns** (8 per instrument) rooted in authentic rhythmic traditions
- **Live override** — MIDI plays over patterns, circle fades back in when you stop
- **4 macros:** GATHER (sync), FIRE (energy), CIRCLE (interaction), SPACE (environment)
- **FX chain:** Circle Spatial Engine → Fire Stage → Gathering Reverb → Pulse Compressor
- **16 voices** (2 per seat for rolls/flams)
- **120 factory presets** across 7 categories
- **~140 canonical parameters**

---

## Visual Identity

- **Accent color:** Firelight Orange `#E8701A` — the bonfire at the center
- **UI concept:** Circle View — 8 seats around a central fire, top-down
- **Color palette:** Warm charcoal brown, firelight orange accents, cream text. Nighttime around a fire.

---

## Coupling

- **Signature routes:** OSTINATO × OVERDUB (World Dub), OSTINATO × OPAL (Scattered Gathering), ONSET × OSTINATO (Machine Meets Human)
- **Design doc:** `Docs/plans/2026-03-12-xostinato-design.md`

---

*XO_OX Designs | Engine: OSTINATO | Accent: #E8701A | Prefix: osti_*
