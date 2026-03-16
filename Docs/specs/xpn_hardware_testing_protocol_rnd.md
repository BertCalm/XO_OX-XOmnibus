# XPN Hardware Testing Protocol — R&D Spec
**Date**: 2026-03-16
**Scope**: Physical and virtual device testing for XO_OX expansion packs (.xpn bundles)

---

## 1. Device Matrix

Testing priority order reflects installed base and MPCe opportunity:

| Priority | Device | Notes |
|---|---|---|
| 1 | MPC Live III | Primary target — MPCe pads, MPCe-native opportunity |
| 2 | MPC X | Original 16-pad flagship, largest installed base |
| 3 | MPC Software | Desktop/laptop users, broadest reach after hardware |
| 4 | MPC One+ | Portable form factor, different screen dimensions |
| 5 | MPC Key 61 | Keyboard-style workflow, different pad layout context |

Live III is the release blocker. MPC X is close second given installed base. Software can substitute for One+ and Key 61 in early R&D rounds.

---

## 2. Per-Device Test Checklist

Run this checklist for each device in the matrix. Mark each item PASS / FAIL / N/A.

**Pack Structure**
- [ ] Pack loads from expansion browser without error
- [ ] All programs visible and accessible in browser
- [ ] No sample rate mismatch warnings on load
- [ ] Cover art displays correctly in browser and program view

**Programs & Pads**
- [ ] Pad assignments correct — velocity response matches intended curve
- [ ] Note mapping correct — pads trigger correct pitches
- [ ] Program names render in full; if truncated, truncation is graceful (no mid-word cuts on critical names)
- [ ] Keygroup programs play correctly across full keyboard range (C1–C8)
- [ ] Choke groups working — open hat mutes on closed hat hit

**Q-Links**
- [ ] All 4 Q-Link knobs assigned and respond
- [ ] Q-Link labels match macro intent (MACHINE/PUNCH/SPACE/MUTATE or engine-equivalent)
- [ ] Q-Link ranges feel musical — no dead zones at extremes

**MPCe Quad Corners (Live III only — mark N/A for all other devices)**
- [ ] NW corner triggers correct sample independently
- [ ] NE corner triggers correct sample independently
- [ ] SW corner triggers correct sample independently
- [ ] SE corner triggers correct sample independently
- [ ] XY morphing across pad surface behaves as expected
- [ ] Z-axis (pressure) response active if configured
- [ ] MPCE_SETUP.md instructions match actual device behavior

---

## 3. Firmware Compatibility

**Minimum supported firmware**: MPC 2.10 — expansion format was locked at this version.

**Test targets**:
- Latest stable release (currently 2.16+)
- One version back from latest

**Known firmware quirks to check**:
- Velocity curve behavior shifted between 2.10 and 2.14 — soft hits that triggered at 2.10 may be clipped at 2.14. Verify velocity layers trigger at intended thresholds across both versions.
- Q-Link label display length changed in 2.12 — labels over 8 characters truncate differently. Verify all Q-Link names are either under 8 chars or truncate acceptably.
- Keygroup polyphony cap was raised in 2.15 — test on both sides of this boundary if the pack uses high voice counts.

Log the firmware version in every test report. Do not mark a pack DOCTRINE-tier without at least one test at minimum firmware and one at current stable.

---

## 4. MPCe-Specific Test Protocol (Live III Only)

The MPCe quad corner format is the primary differentiator for XO_OX packs on Live III. Test this section thoroughly before any release.

1. Load the pack and navigate to a drum program configured with MPCe quad layout.
2. Strike each corner of a pad in isolation — NW, NE, SW, SE — and confirm the correct sample fires. No bleed between corners.
3. Slowly drag a finger from corner to corner across the XY surface. Confirm morphing is smooth and samples crossfade as intended.
4. Apply sustained pressure (Z-axis) on a corner and confirm pressure response is active where configured, inactive where not.
5. Cross-reference MPCE_SETUP.md step-by-step while executing — flag any instruction that does not match actual device behavior as a MAJOR issue.
6. Test with both a fresh program load and after switching away and back to the program (state persistence check).

---

## 5. Test Report Template

Fill one report per device per pack version. Store in `Tools/test_reports/`.

```
Pack Name:
Pack Version:
Device:
Firmware Version:
Date Tested:
Tester:

CHECKLIST RESULTS
Pack Structure:         PASS / FAIL
Programs & Pads:        PASS / FAIL
Q-Links:                PASS / FAIL
MPCe Corners (Live III): PASS / FAIL / N/A

ISSUES FOUND
[severity] [item] — [description]
Example: BLOCKER — Pack load: expansion browser shows "format error" on MPC X 2.14

Screenshots/Video Attached: Y / N
Notes:
Overall Result: PASS / HOLD
```

---

## 6. Blocking vs. Non-Blocking Issues

**BLOCKER — hold release until resolved**
- Pack will not load in expansion browser
- Samples are silent on trigger
- Pads trigger wrong samples (note map corruption)
- Keygroup program unplayable across standard range

**MAJOR — fix before release, do not ship as-is**
- MPCe quad corners cross-triggering or mismapped
- Q-Links unassigned or out of range
- Sample rate mismatch warning fires on load
- Choke groups not functional

**MINOR — document in pack README, ship with note**
- Cover art display quirk on a specific firmware version
- Program name truncation that is still readable
- Q-Link label truncation
- Minor velocity curve difference across firmware versions

Severity is assigned by the tester. Any BLOCKER from MPC Live III or MPC X is a full release hold. BLOCKERs on lower-priority devices (One+, Key 61) are evaluated case-by-case — if the issue is device-specific and affects under 5% of the installed base, it may be downgraded to MAJOR pending a hotfix timeline.

---

## 7. Virtual Testing Fallback

When physical hardware is unavailable:

**MPC Software (desktop)** catches the majority of structural issues — program load, sample mapping, Q-Link assignment, keygroup range. Use this as the primary fallback for One+ and Key 61 testing when devices are not on hand.

**Validator scripts** (`Tools/xpn_kit_validator.py`, `Tools/xpn_manifest_validator.py`) catch format violations before any device test. Run both validators on every pack before opening MPC Software. A validator failure is equivalent to a BLOCKER — do not proceed to device testing until validators are clean.

**Limitation**: Virtual testing cannot catch MPCe quad corner behavior, pressure response, or firmware-version-specific quirks. Physical Live III testing is mandatory before DOCTRINE tier release. No substitution.

**Testing order for R&D rounds**: validators → MPC Software → Live III hardware → MPC X hardware → lower-priority devices as available.
