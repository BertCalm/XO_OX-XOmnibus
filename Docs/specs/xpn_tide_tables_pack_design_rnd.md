# TIDE TABLES — XPN Pack Design Spec
**Date**: 2026-03-16
**Status**: Draft
**Type**: Free gateway pack — Patreon Signal tier conversion tool

---

## 1. Pack Identity

**Name**: TIDE TABLES

**Tagline**: *Rhythms pulled by something larger than the grid.*

**Concept**: A tide table is a prediction document — it tells you exactly when the water will arrive, how high it will climb, and when it will recede. Tide tables are precise and mechanical, but what they describe is immense, ancient, and indifferent to any individual's plans. That tension — clockwork precision serving something that could drown you — is the sonic center of this pack.

TIDE TABLES gives producers four programs, each calibrated to a different tidal phase. NEAP is the quiet pull between extremes. SPRING is the full-force surge when sun, moon, and earth align. SURGE is the forward edge of breaking water. UNDERTOW is what happens below the surface after the wave passes. Together they cover the rhythmic vocabulary of XO_OX: percussive and textural, mechanical and organic, controlled and threatening.

**Target user**: MPC producer, intermediate level. They make beats. They're comfortable with drum kits but they want something that sounds different from every other MPC pack — character over commodity. They may not know what ONSET is. After TIDE TABLES, they will.

---

## 2. Contents Specification

**Programs**: 4 (all Drum type, ONSET engine)

ONSET is the correct engine for a gateway pack. Every MPC producer needs drum kits. ONSET is the most legible XO_OX engine for a first-time user — it maps directly to how producers already think (Kick, Snare, Hat, Perc). The sonic character — physical modeling, resonant bodies, aquatic processing — is immediately recognizable as different without requiring any explanation.

**Engine featured**: ONSET (primary). One program incorporates OPAL-processed samples as the FX layer, giving the producer a first taste of granular without naming it.

**Presets per program**: 3-4 velocity/cycle variants per pad, 16 pads per program.

**Mood distribution** (Sonic DNA targets):

| Program | Movement | Aggression | Brightness | Character |
|---------|----------|------------|------------|-----------|
| NEAP | 0.70 | 0.30 | 0.45 | Sparse, controlled |
| SPRING | 0.90 | 0.65 | 0.70 | Full-force, bright |
| SURGE | 0.85 | 0.55 | 0.60 | Driving, forward |
| UNDERTOW | 0.60 | 0.40 | 0.30 | Dark, sub-heavy |

**Total approximate size**: 35-50 MB. Tight enough to download without hesitation, large enough to feel substantial. ONSET's synthesis is primarily engine-generated; sample content is minimal (transient captures, short resonance tails only).

---

## 3. The 4 Programs

### NEAP
**Engine**: ONSET — all 8 voice types used at low density
**Sounds like**: A drum machine that has been submerged and dried out. Sounds are present but receded — hat ticks that flutter, kicks with long sub tails, snares that bloom slowly. Gaps feel intentional, not empty.
**Pads**: Standard 4-corner quad (Kick TL, Snare TR, CHat BL, OHat BR) with Perc row across the middle. 4 alternate kicks across the bottom row with progressively longer decay. Velocity switching gives the producer two distinct characters per pad without needing to think about it.
**Distinctive**: The silence is designed. Most drum kits compete for pad real estate. NEAP is the first kit that sounds better when you leave pads un-triggered.

### SPRING
**Engine**: ONSET — MACHINE macro at 0.85, PUNCH at 0.75
**Sounds like**: Coastal flood season. Every hit is a wave arriving with full force — kick that displaces air, snare with crack and spray, hats that churn. This is the kit for drops, climaxes, and anything that needs to move a room.
**Pads**: Full 16-pad layout. Kick column (4 weight variants), Snare column (dry/verb/rimshot/clap), Hat row (closed/open/pedal/roll), and a bottom FX row with Perc/wave hits for fills. CHOKE groups assigned so the hat row behaves like a real instrument — open hat cuts on closed.
**Distinctive**: The CHOKE architecture is intentional and musical. Most MPC kits assign CHOKE as an afterthought. SPRING's choke groups mirror how a real high-hat pedal works, so the playing feels natural even at high velocity.

### SURGE
**Engine**: ONSET + OPAL-processed transient samples on the FX pads
**Sounds like**: The forward wall of breaking water — kinetic, directional, with a granular shimmer on the back edge of every hit. The Kick punches through; the tail opens into OPAL grain-cloud texture that fades before it becomes ambient. The producer hears something on the FX pads they can't identify. That's the hook.
**Pads**: Standard quad plus a 4-pad "foam row" — FX pads built from OPAL granular renders of ONSET transients. The foam row contains sounds that are percussive but undefined: a kick grain cloud, a snare scatter, a hat bloom, a full-kit avalanche. These are the pads that make producers ask what engine that came from.
**Distinctive**: The OPAL foam row is the single element in TIDE TABLES that doesn't feel like a drum kit. It's the first exposure to granular synthesis in the pack, delivered without explanation. Producers who gravitate to it are the ones who will subscribe for DRIFT SAMPLES.

### UNDERTOW
**Engine**: ONSET — SPACE macro at 0.80, MACHINE at 0.40
**Sounds like**: Everything happening below the surface after the wave passes. Sub-dominant, resonant, with long reverb tails and physical body sounds — bowed metal, resonant pipe, deep skin hits. Tempo-indifferent; this kit works at any BPM because the tails are long enough to blur the grid.
**Pads**: 16 pads arranged as a textural kit — 4 kicks across the top (tuned to A, D, F, G), 4 tom variants, 2 snare tones, 2 clap/snap pair, and 4 FX pads with long resonant tails. The tuned kick row is intentional — producers who play melodically will discover they can build basslines with it.
**Distinctive**: The tuned kick row is the element that breaks the expectation. It is simultaneously a drum kit and a bass instrument. Producers who find it are the ones who will want ONSET's full keygroup expansion programs in a paid pack.

---

## 4. XPN Tool Pipeline

Build order for TIDE TABLES:

1. **`xpn_drum_export.py`** — Core export. Runs against each of the 4 ONSET program definitions, outputs `.xpm` files with pad assignments, velocity zones, choke groups, and cycle data. This is the primary build tool. All 4 programs go through this step.

2. **`xpn_choke_group_assigner.py`** — Post-process SPRING's hat row to wire the CHOKE group logic. Run after drum_export, before validation. NEAP, SURGE, and UNDERTOW do not require this step.

3. **`xpn_keygroup_export.py`** — Used only for UNDERTOW's tuned kick row. Exports the 4-note tuned kick pads as a keygroup layer within the drum program so MPC recognizes the pitch mapping.

4. **`xpn_auto_dna.py`** — Compute and embed Sonic DNA metadata (movement, aggression, brightness, depth values) for all 4 programs. Run after all `.xpm` files are finalized.

5. **`xpn_manifest_generator.py`** — Build the pack manifest: program list, DNA summary, engine attribution, version string. Required before packaging.

6. **`xpn_cover_art_generator_v2.py`** — Generate cover art from pack manifest. TIDE TABLES visual: deep blue-green water column with tide table grid lines overlaid in the XO_OX typography. 1000×1000px.

7. **`xpn_packager.py`** — Final step. Bundles all `.xpm` files, samples, manifest, cover art, and README.txt into a distributable `.zip`. Validates file count, size, and manifest integrity before writing the archive.

---

## 5. What TIDE TABLES Opens the Door To

A producer who loves TIDE TABLES has demonstrated specific preferences that map cleanly to paid content:

**They gravitate to NEAP** (sparse, controlled) → They want **UNDERTOW EXTENDED** — a full ONSET pack focused on sparse, space-heavy drum design. Natural upsell: a Current-tier pack called SLACK TIDE, 12 programs, all in the low-aggression quadrant.

**They gravitate to SPRING** (full-force)  → They want power and density. Natural upsell: **SURGE DOCTRINE** — an ONSET pack built around maximum MACHINE + PUNCH macro territory, high aggression, high movement. This is the pack for trap, drill, and hard boom-bap producers.

**They find the SURGE foam row** (OPAL granular) → They want to know what that sound is. Natural upsell: **DRIFT SAMPLES** (OPAL free pack) as the next free download, followed by a paid OPAL keygroup pack — **GRAIN ATLAS** — that explains granular in full.

**They discover UNDERTOW's tuned kicks** (pitched bass) → They want melodic percussion. Natural upsell: a Current-tier ONSET pack focused on tuned percussion and drum-as-melody design — **RESONANT BODY**.

In every case, TIDE TABLES does not tease — it satisfies. The appetite it creates is for *more of what they already love*, which is a fundamentally different (and more effective) conversion mechanism than withholding content. The producer subscribes because they want to go deeper, not because they felt shortchanged.
