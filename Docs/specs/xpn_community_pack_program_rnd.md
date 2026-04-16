# XPN Community Pack Program — R&D Spec
**Date**: 2026-03-16
**Status**: Proposal

---

## 1. Program Philosophy

XO_OX ships 34 engines. A full expansion pack for each engine requires 150+ hours of sound design — more hours than a small team can sustain while simultaneously building new instruments. The gap is real: there are genres, workflows, and cultural traditions the house team will never reach with full depth.

Community packs close that gap. A producer who has spent years designing hip-hop drums on an MPC brings knowledge no internal sprint can replicate. A sound designer rooted in Afrobeats or Afropop or Andean folk music brings ears shaped by those traditions. The XO_OX platform provides the tools, the QA infrastructure, and the distribution channel; the community provides the range.

The "Community Series" badge is not a participation award. It means the pack was submitted, survived automated QA, passed a DNA review, and was played on hardware by people who know what XO_OX sounds like. Community does not mean unvetted. It means the circle is wider.

---

## 2. Submission Requirements

Packs submitted for consideration must meet all of the following before entering the curation queue:

- **Minimum 16 programs** — across at least 3 distinct moods or use-case categories
- **QA gate**: full_qa_runner.py score of 80 or higher; zero critical errors
- **Cover art**: 400x400px minimum, original artwork, no stock photography
- **README**: pack story (what it is, what inspired it, how to use it), full credits, sample source declaration
- **Creator bio**: 100-200 words, plus a photo for the XO_OX site feature
- **Sample clearance**: royalty-free or creator-owned samples only — no Splice credits, no sample pack samples whose licenses prohibit redistribution. Creator signs a declaration confirming this.

Packs that fail the sample clearance declaration or fall below the QA threshold are returned before human review begins.

---

## 3. Curation Process

**Stage 1 — Automated QA**
full_qa_runner.py + pack_score.py run against the submitted .xpn bundle. This is a hard pass/fail gate. Failures return a report to the creator with specific line items to fix.

**Stage 2 — DNA Review**
Barry OB's team (community curator role) evaluates sonic diversity: does the pack cover distinct enough territory within its claimed genre or tradition? Is there range across moods, tempos, and intensity levels? Is the engine used in a way that makes sense? This stage produces a short written note — not a score — flagging any homogeneity or misalignment.

**Stage 3 — Ear Test**
Three members of the XO_OX team play the pack on hardware (Akai MPC). No DAW, no headphones-at-a-desk. The test is simple: does this pack earn its place? Does it feel like XO_OX?

**Stage 4 — Acceptance or Feedback Loop**
Packs that pass receive an acceptance letter and move to release prep. Packs that need work receive specific, actionable feedback. Two rounds of revision are allowed. After two rounds without passing Stage 3, the pack is declined — with an invitation to resubmit a new pack in a future cohort.

---

## 4. Revenue Share

- **Creator**: 50% of net revenue from pack sales
- **XO_OX**: 50% (platform, QA infrastructure, hosting, marketing, distribution)
- **Payment**: monthly via Stripe, minimum payout threshold $25
- Revenue share terms are fixed for the life of the pack; no renegotiation after acceptance

The split is designed to be sustainable on both sides. XO_OX is not a marketplace taking a small clip — the platform investment in QA, curation, and promotion is real. The creator rate reflects that this is a partnership, not a listing service.

---

## 5. Credit and Visibility

Every accepted community pack receives:

- Creator name and photo on the pack's product page
- "Community Series" badge distinguishing it from XO_OX house packs
- Option for a Field Guide interview post (500-800 words, Q&A format, published on XO-OX.org)
- Feature in the Patreon monthly update for the release month

The Field Guide interview is optional but encouraged. It gives the creator a platform to talk about their practice, their cultural context, and what drew them to XO_OX — and it gives the XO_OX audience a richer story than a product page can hold.

---

## 6. First Cohort Target — Q2 2026

Three community packs by end of Q2 2026, representing distinct creative territories:

1. **Hip-hop / beat producer** — MPC-native workflow, drums + melodic programs, contemporary production sensibility
2. **Experimental / ambient producer** — textural, generative-friendly, designed for patience and space
3. **Non-Western cultural producer** — African, Asian, or Latin American focus; instruments and rhythmic frameworks outside the default Western pop palette

These three are not a quota. They are a signal about what "range" means in this program. The ideal first cohort demonstrates that XO_OX community packs can go places the house team has not yet been.

---

## 7. Tools Available to Community Creators

The full Tools/ suite is open source and available to any producer building for XO_OX:

- `drum_export.py`, `keygroup_export.py`, `kit_expander.py`, `bundle_builder.py`
- `cover_art_generator.py`, `packager.py`, `sample_categorizer.py`, `render_spec.py`
- `MPCE_SETUP.md` — hardware setup template for MPC exports
- Pack story template (README scaffold)
- QA checklist (mirrors full_qa_runner.py criteria in plain language)

Creators are expected to run QA locally before submitting. The automated gate at Stage 1 is not a first-pass filter — it is a confirmation that the creator already knows the pack is ready.

---

## Open Questions for Launch

- Does Barry OB's team need a formal brief / rubric for Stage 2, or is the DNA review intentionally qualitative?
- Stripe payout setup — does XO_OX need a creator portal, or is manual monthly export sufficient for first cohort?
- Should the Field Guide interview be published before or after pack launch? (Pre-launch generates anticipation; post-launch closes the story arc.)
- Submission intake: GitHub PR against a community-packs repo, or a Google Form + file upload?
