# Oblate — Demo Preset Configurations

**Chain:** `OblateChain` (FX Pack 1, Sidechain Creative) · ChainID `32` · prefix `obla_`
**Spec:** `Docs/specs/2026-04-27-fx-pack-1-sidechain-creative.md` §3
**Status:** Configurations documented for the real-DSP PR. Persistence in `.xometa`
awaits the FX-chain preset-schema extension (tracked alongside the same gap for
Otrium). Each entry below is a complete parameter set ready for that migration.

The Oblate spectral gate opens FFT bins where the *partner engine's spectrum*
is loud. Brightness DNA tilts the threshold curve so a bright partner exposes
high-frequency detail and a warm partner gates everything except the body.

---

## 1. Spectral Gate

The reference patch — clean, audible, every parameter doing visible work.

| Parameter | Value | Notes |
|---|---|---|
| `obla_threshold` | -28 dB | Mid-floor; partner peaks easily clear it |
| `obla_ratio` | 6.0 | Strong gate, not brick-wall |
| `obla_attack` | 2 ms | Fast enough to keep transients sharp |
| `obla_release` | 120 ms | Long enough to avoid chatter |
| `obla_keyEngine` | 1 | Slot B is the key |
| `obla_fftSize` | 1024 (idx 2) | ~23 ms latency at 44.1 kHz |
| `obla_tilt` | 0.0 | Flat threshold across frequency |
| `obla_dnaCoupling` | 0.0 | Pure spectral gate, no DNA influence |
| `obla_smoothing` | 0.5 | Anti-zip mid-setting |
| `obla_breathRate` | 0.08 Hz | Slow ±2 dB threshold drift |
| `obla_mix` | 1.0 | Fully wet |
| `obla_hqMode` | false | 1024 cap |

**Engine pairing:** Slot A = `OddOscar` (carrier, sustained pad). Slot B = `Onset` (key,
percussive transients). Result: pad sounds only when drums hit.

---

## 2. Vocal Carve

Frequency-mirror style: the partner's vocal-range energy carves a window in
the carrier. Tilt opens the high end so consonants pass cleanly.

| Parameter | Value | Notes |
|---|---|---|
| `obla_threshold` | -36 dB | Low floor — most partner content opens the gate |
| `obla_ratio` | 12.0 | Hard carve |
| `obla_attack` | 1 ms | Pre-vocal click preserved |
| `obla_release` | 80 ms | Tight to vocal phrasing |
| `obla_keyEngine` | 0 | Slot A vocal-like source |
| `obla_fftSize` | 2048 (idx 3) | Resolution for sibilants — requires HQ |
| `obla_tilt` | 0.35 | Highs need less key energy to open |
| `obla_dnaCoupling` | 0.6 | Brighter partners open more highs |
| `obla_smoothing` | 0.3 | Light smoothing — preserve consonant transients |
| `obla_breathRate` | 0.001 Hz | Effectively static (D005 floor) |
| `obla_mix` | 0.85 | Slight dry blend retains body |
| `obla_hqMode` | true | Unlocks 2048 |

**Engine pairing:** Slot A = `Obbligato` (vocal-like timbre, key). Slot B = `OpenSky` (lush
pad, carrier). Bright vocal phrasing carves matching brightness in the pad.

---

## 3. DNA-Tilted Gate

The wildcard preset — partner brightness DNA *visibly* steers the gate curve.
M1 CHARACTER on the key engine warps brightness, which retunes Oblate live.

| Parameter | Value | Notes |
|---|---|---|
| `obla_threshold` | -22 dB | Centred mid |
| `obla_ratio` | 4.0 | Soft, musical |
| `obla_attack` | 5 ms | Smooth opens |
| `obla_release` | 200 ms | Long tails |
| `obla_keyEngine` | 1 | Slot B partner |
| `obla_fftSize` | 1024 | Default |
| `obla_tilt` | 0.0 | Tilt comes entirely from DNA |
| `obla_dnaCoupling` | 1.0 | Maximum DNA influence (±1.0 effective tilt swing) |
| `obla_smoothing` | 0.7 | Strong anti-zip — slow morphs sound musical |
| `obla_breathRate` | 0.25 Hz | Audible breathing on sustained notes |
| `obla_mix` | 1.0 | Full wet |
| `obla_hqMode` | false | 1024 cap |

**Engine pairing:** Slot A = `Orbital` (carrier). Slot B = `Optic` (key, brightness DNA
high). Sweep M1 CHARACTER on Slot B → Oblate's gate curve sweeps from
"only highs pass" to "only lows pass" as brightness warps.

---

## 4. Spectral Stutter

Aggressive, percussive — the gate opens and closes at musical rates set by
the partner's transients. Short release intentionally produces chatter as
character.

| Parameter | Value | Notes |
|---|---|---|
| `obla_threshold` | -18 dB | Higher — fewer hits open the gate |
| `obla_ratio` | 40.0 | Near-mute floor |
| `obla_attack` | 0.5 ms | Razor-edge opens |
| `obla_release` | 25 ms | Stutter zone |
| `obla_keyEngine` | 2 | Slot C — drum-like key |
| `obla_fftSize` | 512 (idx 1) | Tighter time resolution |
| `obla_tilt` | -0.4 | Lows open more easily — kick-driven |
| `obla_dnaCoupling` | 0.2 | Light DNA flavor |
| `obla_smoothing` | 0.0 | No anti-zip — chatter is the point |
| `obla_breathRate` | 0.001 Hz | Static |
| `obla_mix` | 1.0 | Full wet |
| `obla_hqMode` | false | 512 cap |

**Engine pairing:** Slot A = `Overdub` (carrier, sustained synth). Slot C = `Onset` (drum
key). The synth becomes a rhythmic stutter following the drum pattern.

---

## 5. Ghosted Choir

Slow-motion spectral mask — the partner's melodic content pulls a translucent
choir out of the carrier. Long FFT, heavy smoothing, low ratio.

| Parameter | Value | Notes |
|---|---|---|
| `obla_threshold` | -42 dB | Sensitive — sustained partner content opens |
| `obla_ratio` | 2.5 | Gentle, not gating to silence |
| `obla_attack` | 30 ms | Slow rise (no transient detail) |
| `obla_release` | 450 ms | Long tail |
| `obla_keyEngine` | 1 | Slot B key |
| `obla_fftSize` | 2048 (idx 3) | Maximum spectral resolution |
| `obla_tilt` | 0.6 | Highs open easily — airy ghost band |
| `obla_dnaCoupling` | 0.4 | Modest DNA influence |
| `obla_smoothing` | 0.95 | Heavy anti-zip — pure pad textures |
| `obla_breathRate` | 0.04 Hz | Slow inhale/exhale of the gate |
| `obla_mix` | 0.7 | 30 % dry retains anchoring |
| `obla_hqMode` | true | 2048 unlocked |

**Engine pairing:** Slot A = `Opaline` (carrier, prepared piano). Slot B = `Ondine` (key,
slow water-line melody). The piano harmonics become a ghost choir tracking
the water melody.

---

## Doctrine Trace

| Doctrine | How these presets satisfy it |
|---|---|
| D001 velocity → timbre | Velocity routed at the engine layer; Oblate's threshold tracks it via host CC mapping |
| D002 modulation | Breath LFO + DNA + sidechain + smoothing pole + M1 CHARACTER (5 sources) |
| D003 physics | STFT/ISTFT cited per spec — Hann window with periodic-N denominator gives perfect COLA at 50 % overlap |
| D004 dead params | All 12 parameters cached and audibly routed (verified per-preset above) |
| D005 must breathe | `obla_breathRate` floors at 0.001 Hz and is non-zero in 4 / 5 presets |
| D006 expression | Aftertouch routes to `obla_threshold` via host CC matrix; M1 warps DNA → tilt |

---

## Notes for Future Migration

When the FX-chain preset-schema extension lands (the same blocker tracked by
the Otrium seance), each preset above maps to one `.xometa` file with this
shape:

```json
{
  "name": "Spectral Gate",
  "mood": "Coupling",
  "engines": ["OddOscar", "Onset"],
  "parameters": { "OddOscar": {...}, "Onset": {...} },
  "fxChains": {
    "slot1": {
      "chainId": 32,
      "params": { "obla_threshold": -28, "obla_ratio": 6.0, ... }
    }
  }
}
```

Until then this document is the authoritative source for the Oblate demo set.
