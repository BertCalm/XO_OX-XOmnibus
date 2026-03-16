# AIR Plugin Research Brief — XOmnibus Compatibility
*Date: 2026-03-16 | Status: Research Brief*

---

## What AIR Plugins Are

AIR Music Technology (now inMusic / Akai) produces virtual instruments that ship bundled inside MPC
Expansion ZIP archives. Examples: AIR Solina (string machine), AIR Bassline (acid bass), AIR
WaveTable (wavetable synth). These plugins load natively inside MPC Software on desktop as standard
AU/VST3 plugins. On hardware MPC units (Live III, X, One, Key 61), AIR plugins are pre-installed
firmware components — the expansion ZIP delivers samples and programs, but plugin availability is
baked into the MPC OS firmware image. End users cannot install new plugins on hardware via expansion
import.

---

## AIR Plugin SDK Status

The AIR plugin SDK is **closed**. AIR plugins are AIR/inMusic proprietary software. There is no
public SDK, no developer program, and no documented API for writing "AIR-compatible" plugins. The
integration between AIR plugins and MPC expansions is an internal Akai/inMusic format not exposed
to third parties. Reverse-engineering the expansion format is feasible (the ZIP structure is
documented by the community) but reproducing the plugin loading protocol on hardware is not
achievable without an OEM firmware deal with Akai.

---

## Two Approaches

### Approach A — Native AIR-Compatible Plugin

Package XOmnibus as an AU/VST3 that MPC Software discovers and loads, mirroring how AIR plugins
work on desktop.

**What this requires:**
- XOmnibus already passes `auval` — the DSP is ready.
- Apple Developer ID + notarization ($99/yr) for signed macOS distribution.
- An expansion ZIP `plugin_bundle/` directory with a `manifest.json` declaring the plugin dependency
  (see `Docs/specs/xpn_air_plugin_architecture_rnd.md` for the full proposed spec).
- MPC Software support for the `plugin_bundle` expansion key — **currently absent**. This requires
  Akai to ship a MPC Software update acknowledging third-party plugin bundles.
- Hardware compatibility is blocked entirely until Akai offers a third-party plugin program (no
  current roadmap signal).

**Verdict:** Desktop feasible after notarization + Akai cooperation. Hardware is blocked indefinitely.

### Approach B — XPN Presets Designed for AIR Plugin Compatibility

Rather than replacing AIR plugins, design XPN packs that complement them. For example:
- Drum programs (ONSET-sourced samples) that pair rhythmically with an AIR Bassline acid line.
- Keygroup programs (OPAL / OVERWORLD samples) tuned to work in the same key/tempo context as
  AIR Solina pads.
- A `params_sidecar.json` file per program (see full spec in `xpn_air_plugin_architecture_rnd.md`)
  that XOmnibus parses to restore engine voice character alongside the MPC program.

This approach requires zero Akai cooperation. It positions XO_OX packs as "AIR-aware" — designed
to be loaded alongside AIR plugins, with complementary sonic roles documented in pack liner notes.

**Verdict:** Zero blockers. Implementable today with existing XPN tools.

---

## Sonnet-Ready Work

These tasks require no novel DSP or SDK access:

- Finalize `params_sidecar.json` format (spec exists in `xpn_air_plugin_architecture_rnd.md`).
- Add sidecar file generation to `xpn_bundle_builder.py` — emit one `.json` per program directory
  containing the engine name, preset name, and key parameter values from the `.xometa` source.
- Write an XOmnibus "Sidecar Import" feature: scan a user-selected directory for
  `params_sidecar.json` files and load matching engine presets automatically.
- Author a pack design guide: "XO_OX + AIR — Complementary Roles" covering sonic pairing
  (ONSET drums + AIR Bassline, OPAL pads + AIR Solina, etc.).
- Pursue Apple notarization so XOmnibus can be distributed as a signed free download, linkable
  from pack pages and expansion metadata.

---

## Opus-Level Work

These tasks require novel architecture decisions, DSP porting, or SDK reverse-engineering:

- **Full plugin bundle ZIP support**: Building the `plugin_bundle/` expansion architecture and
  getting MPC Software to honor it requires either Akai partnership outreach (business) or
  reverse-engineering the MPC expansion loader (technical risk). This is Opus-level coordination
  work, not raw DSP.
- **Hardware MPC native port**: Running XOmnibus natively on MPC hardware (ARM Linux, embedded
  audio constraints, no JUCE GUI layer) is a full platform port. Requires deep embedded Linux
  knowledge, JUCE headless builds, and an Akai OEM arrangement. Multi-month effort minimum.
- **AIR Bassline / Solina program generation**: If XO_OX wanted to generate *programs that drive
  AIR plugins* (not just complement them), that would require reverse-engineering AIR plugin
  parameter IDs from saved MPC projects — feasible but legally gray and format-fragile.

---

## Recommended Path

1. **Now**: Ship `params_sidecar.json` in all new XPN packs. Zero dependencies, immediate value
   for XOmnibus users.
2. **V1.1**: Build XOmnibus sidecar importer. Sonnet-buildable in one session.
3. **V1.x**: Notarize XOmnibus for signed distribution. Link from pack pages.
4. **V2**: Monitor MPC Software for third-party plugin bundle support. File a feature request with
   Akai. Full `plugin_bundle/` ZIP architecture is the target when that gate opens.

Full architecture detail (bundle layout, manifest spec, expansion.json format) lives in
`Docs/specs/xpn_air_plugin_architecture_rnd.md`.
