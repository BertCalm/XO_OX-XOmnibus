> ⚠️ **RETIRED — DO NOT USE AS A LIVE PLANNING DOC (2026-04-16)**
>
> The "V1 launch" / "V1 release" framing has been retired. XOceanus does not
> operate on a fixed-version release model. Build and refine until ready; ship
> when ready. This document is preserved for historical reference only.
> See `CLAUDE.md` → *Release Philosophy*.
>
> ---

# XO_OX V1 Expansion Pack Launch Checklist
**R&D Spec** | 2026-03-16

## Overview

V1 is the first public XO_OX expansion pack release. The flagship pack is **TIDE TABLES** (ONSET, free) followed within two weeks by **MACHINE GUN REEF** (ONSET, FORM tier, $15). This document is the master go/no-go checklist. Nothing ships until every BLOCKER is cleared.

---

## BLOCKERS — Must be done before any pack goes live

- [x] **1. Patreon URL updated.** DONE 2026-03-22 — https://www.patreon.com/cw/XO_OX live. All placeholders replaced across site, Docs, Tools.
- [ ] **2. XO-OX.org pack download page live and tested.** Gumroad embed or direct download link functional. Test the full flow: land on page → click download → file arrives intact.
- [ ] **3. TIDE TABLES passes `full_qa_runner.py` with score ≥ 70.** Run against the release candidate, not the dev copy. Log the score and attach output to this spec.
- [ ] **4. TIDE TABLES recorded audio demos.** Minimum 3 programs demoed on hardware MPC. Captures must be from the actual `.xpn` file loaded on device — not a DAW render. At least one demo should showcase velocity expression.
- [ ] **5. TIDE TABLES cover art finalized.** 800×800px, ONSET engine color (`#FF6B35`) themed. Delivered as PNG. Verify rendering at thumbnail size (300×300) and full size.
- [ ] **6. MACHINE GUN REEF passes `full_qa_runner.py` with score ≥ 80.** Higher bar because it is a paid product. Same rule: release candidate file, not dev copy.
- [ ] **7. Gumroad account set up, payment processing verified.** Run a real test purchase (use a personal card). Confirm receipt email arrives, download link works, and payout routing is set.
- [ ] **8. Email capture mechanism active.** Gumroad auto-capture on purchase is the minimum. If free download bypasses checkout, a separate sign-up form must be wired and tested.

---

## IMPORTANT — Should be done before or immediately at launch

- [ ] **9. Field Guide article scheduled for launch day.** Article 1 (velocity and expression in ONSET). Publish time should coincide with pack announcement — not a week later.
- [ ] **10. Instagram content queued.** Two weeks of Sound Posts featuring ONSET presets. Minimum: one post per pack per week. Captions drafted, hashtags confirmed, scheduling tool armed.
- [ ] **11. MPCE_SETUP.md included in both packs.** Verify the file is present in each `.xpn` bundle and that the instructions are accurate for current MPC firmware.
- [ ] **12. Hardware test on MPC Live III complete for both packs.** Load from a fresh download (not from the dev machine filesystem). Test pad response, velocity layers, and program switching.
- [ ] **13. YouTube pack walkthrough video produced for TIDE TABLES.** Screen + hardware capture. No minimum length, but must cover: loading the pack, at least 2 programs played through, and one velocity demo.
- [ ] **14. Pack README files reviewed for typos and accuracy.** Second set of eyes. Check that credit lines, tempo references, and usage notes are correct.
- [ ] **15. Gumroad UTM tracking links created for each social platform.** At minimum: Instagram, YouTube, and the Field Guide. This is the only way to know where buyers come from on day one.

---

## NICE-TO-HAVE — Beneficial but not blocking

- [ ] **16. Drum Broker submission prepared** for MACHINE GUN REEF. Draft the submission package (description, cover art, audio preview) so it can be sent within the first week post-launch.
- [ ] **17. r/makinghiphop community intro post drafted.** Non-promotional framing. Lead with the free pack and the ONSET origin story. Do not post until TIDE TABLES has at least 3 public reviews or downloads.
- [ ] **18. Patreon launch post + first Architect-tier content ready.** If Patreon is live at launch, have at least one piece of exclusive content staged — a behind-the-scenes preset build, a raw session, or a design note.
- [ ] **19. `pack_registry.json` updated** with both packs. Tier, price, engine, QA score, and release date fields populated.
- [ ] **20. XO-OX.org packs page updated** with both entries including DNA charts and audio embeds.

---

## Pre-Launch Validation Sequence

Run in this exact order. Do not skip steps or reorder.

1. Run `xpn_pack_release_checklist.py` on each `.xpn` file. Confirm score thresholds met.
2. Hardware test on MPC Live III using the finalized `.xpn` files (not intermediate builds).
3. Upload final files to Gumroad. Run a test purchase with a real card. Confirm receipt and download.
4. On the MPC, load the downloaded pack from the Gumroad link — not the dev copy. Play through all programs. Verify nothing is missing.
5. Go live: publish download page, post announcement, activate email capture.

---

## Risk Log

**Risk 1 — Render pipeline not complete (BLOCKER level)**
The automated render pipeline may not be ready for V1. Mitigation: manual renders for every program in both packs. Budget 2–3 hours per pack. Do not delay launch waiting for automation.

**Risk 2 — Site down on launch day**
XO-OX.org going down during peak traffic would kill momentum. Mitigation: Gumroad is the primary distribution point. The site links to Gumroad, not the other way around. A site outage does not block purchases.

**Risk 3 — Pack format incompatible with some MPC firmware versions**
`.xpn` format behavior can vary between firmware 2.14 and 2.16. Mitigation: test both versions before launch. If 2.14 breaks, document the minimum supported firmware prominently in the README and on the download page.

**Risk 4 — Patreon status**
Patreon URL resolved — patreon.com/cw/XO_OX live as of 2026-03-22 (see BLOCKER #1).

**Risk 5 — Audio demos recorded from DAW instead of hardware**
Demos that do not come from the actual hardware MPC miss the point of the product. Mitigation: the audio demo BLOCKER (#4) explicitly requires hardware capture. Gate all demo uploads against this requirement before scheduling social posts.

---

*This checklist is owned by the XPN Tools workstream. Update scores and check boxes in place as items clear. Do not archive until both packs are live and at least 10 downloads confirmed.*
