# XPN AIR Plugin Architecture — R&D Spec
*Status: Research / Pre-Implementation | Date: 2026-03-16*

---

## 1. What AIR Does (Reference Model)

AIR Music Technology ships plugins directly inside MPC Expansion ZIP archives. The canonical examples are Hybrid 3 and Vacuum Pro, both bundled with premium MPC Expansions sold through Akai Pro's store.

**Inside the ZIP:**
The expansion archive includes a `Plugins/` directory alongside the standard `Programs/`, `Samples/`, and `Presets/` directories. That `Plugins/` folder contains platform-specific installers or component packages — typically `.pkg` (macOS) or `.exe` (Windows) — rather than raw `.component` or `.vst3` binaries. MPC Software on desktop platforms launches these installers during the expansion import flow.

**Discovery and loading:**
MPC Software's expansion importer reads an `expansion.json` (or equivalent metadata file at the archive root) that declares plugin dependencies by name and version. If a declared plugin is absent from the system, MPC Software presents an install prompt post-import: "This expansion includes a plugin — install now?" The user approves, the installer runs, and the plugin registers in the normal AU/VST3 locations. Subsequent Program loads in MPC reference the plugin by its registered component name, not a bundled path.

**Hardware MPC units:**
Standalone MPC (Live III, X, One, Key 61) run MPC OS on embedded Linux. AIR plugins that ship with expansions on these units are pre-installed firmware components — they are not user-installable via expansion ZIP on the hardware. The expansion ZIP on hardware only delivers samples and programs; plugin availability is entirely determined by the MPC OS version. This is the critical constraint for any XO_OX implementation.

---

## 2. XPN Plugin Bundle Spec (Proposed)

Proposed directory layout inside an XPN ZIP:

```
MyPack.xpn.zip
├── expansion.json
├── Programs/
├── Samples/
└── plugin_bundle/
    ├── manifest.json
    ├── AU/
    │   └── XOlokun.component      (macOS AU)
    └── VST3/
        └── XOlokun.vst3/          (cross-platform VST3)
```

### `plugin_bundle/manifest.json`

```json
{
  "plugin_name": "XOlokun",
  "plugin_id": "XO_OX.XOlokun",
  "version": "1.0.0",
  "formats": ["AU", "VST3"],
  "minimum_mpc_software": "3.4.0",
  "minimum_mpc_os": null,
  "hardware_compatible": false,
  "compatibility": {
    "MPC_Live_III": "firmware_only",
    "MPC_X": "firmware_only",
    "MPC_One": "firmware_only",
    "MPC_Key_61": "firmware_only",
    "MPC_Software_macOS": "bundled_install",
    "MPC_Software_Windows": "bundled_install"
  },
  "install_action": "prompt_user",
  "fallback": "params_sidecar"
}
```

### `expansion.json` plugin reference

Add a `plugin_bundle` key to the root expansion metadata:

```json
{
  "name": "XObese Character Pack",
  "version": "1.0",
  "plugin_bundle": {
    "required": false,
    "manifest": "plugin_bundle/manifest.json",
    "fallback_behavior": "load_without_plugin"
  }
}
```

### MPC expansion loader flow (desktop)

1. User imports XPN ZIP into MPC Software.
2. Loader detects `plugin_bundle/manifest.json`.
3. Checks if `plugin_id` is already registered on the system.
4. If absent: presents install dialog with plugin name, version, and "Install / Skip" options.
5. On Install: executes platform-specific installation from `AU/` or `VST3/` directory.
6. Programs referencing the plugin load normally post-install.

This flow mirrors AIR's model. It requires MPC Software support for the `plugin_bundle` key — currently absent, making this firmware-dependent (see Section 5).

---

## 3. XOlokun-as-XPN-Plugin Path

XOlokun is the natural companion plugin for XO_OX packs. The user flow would be:

1. User downloads XO_OX expansion (e.g., XObese Character Pack).
2. MPC Software detects XOlokun is not installed.
3. Prompts: "This expansion was designed for XOlokun. Install the free companion plugin?"
4. XOlokun installs to AU/VST3 locations.
5. Expansion Programs reference XOlokun engines directly via plugin parameter blocks.

**What blocks this today:**

- **MPC OS sandboxing**: Hardware MPC units run a locked embedded Linux environment. Third-party plugin installation is not exposed to users. AIR plugins on hardware are pre-baked into firmware images — XO_OX cannot replicate this without an OEM deal with Akai.
- **AU validation**: macOS requires all AU plugins to pass `auval` and be code-signed with a valid Developer ID. XOlokun currently passes `auval` but must be notarized for distribution outside the Mac App Store. Notarization is achievable but requires an active Apple Developer account ($99/yr) and hardened runtime entitlements.
- **VST3 signing**: Windows VST3 distribution has no mandatory signing requirement, but MPC Software may whitelist or validate plugins by vendor.
- **MPC Software plugin discovery**: MPC Software scans standard AU/VST3 paths. A bundled installer dropping binaries into those paths would work for desktop, but Akai would need to whitelist the `plugin_bundle` expansion key in a Software update for the prompt flow to work at all.

**Near-term feasible path**: Ship XOlokun as a standalone free download with a landing page linked from pack metadata. No firmware changes needed. The `params_sidecar` approach (Section 4) handles the preset bridge.

---

## 4. Lightweight Alternative: Parameter Sidecar

If plugin bundling is blocked at the MPC Software level, a `params_sidecar.json` file ships inside each XPN Program directory:

```
Programs/
└── XObese_001_Concrete/
    ├── XObese_001_Concrete.xpm
    └── params_sidecar.json
```

### `params_sidecar.json` spec

```json
{
  "xo_ox_version": "1.0",
  "target_plugin": "XO_OX.XOlokun",
  "engine": "OVERBITE",
  "preset_name": "Concrete Jaw",
  "params": {
    "filter_cutoff": 0.62,
    "filter_res": 0.45,
    "env_attack": 0.08,
    "env_decay": 0.55,
    "env_sustain": 0.30,
    "env_release": 0.40,
    "drive": 0.70,
    "macro_1": 0.50
  },
  "qlinks": {
    "Q1": "filter_cutoff",
    "Q2": "drive",
    "Q3": "macro_1",
    "Q4": "env_decay"
  }
}
```

XOlokun would expose a **Sidecar Import** feature: scan a directory for `params_sidecar.json` files, parse them, and load the named preset into the matching engine. This is entirely within XOlokun's control — no MPC firmware changes required.

The sidecar functions as a "preset import protocol": the XPN pack ships sounds, the sidecar ships the engine voice character, and a user with XOlokun installed gets both layers automatically.

---

## 5. Implementation Priority

| Component | Effort Level | Buildable Now? | Blocker |
|---|---|---|---|
| `params_sidecar.json` spec (finalize format) | Sonnet | Yes | None |
| XOlokun sidecar importer (scan + load) | Sonnet | Yes | None |
| XOlokun notarization + signed distribution | Sonnet | Yes (process) | Apple Dev account |
| `plugin_bundle/manifest.json` spec | Sonnet | Yes (spec only) | None |
| MPC Software `plugin_bundle` key support | Firmware | No | Akai cooperation |
| Hardware MPC plugin install flow | Firmware | No | Akai OEM deal |
| Expansion ZIP with bundled AU/VST3 | Opus | Yes (build) | Notarization first |

**Recommended sequence:**
1. Ship `params_sidecar.json` in all XO_OX packs immediately — zero external dependencies.
2. Build XOlokun sidecar importer as a V1.1 feature.
3. Pursue Apple notarization so XOlokun can distribute as a signed free download linked from pack pages.
4. Monitor MPC Software changelog for third-party plugin bundle support — file a feature request with Akai.
5. Full `plugin_bundle/` ZIP architecture is the V2 target, contingent on MPC Software support.

The sidecar path delivers 80% of the user value (sounds + engine preset) with 0% of the firmware negotiation overhead.
