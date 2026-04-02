# .xogenome Schema — Behavioral Fingerprint v0.1

**Status:** Draft
**Created:** 2026-03-22
**Location:** `Docs/genomes/*.xogenome`
**Project:** VQ 009 — Engine Genome Project

---

## Purpose

A `.xogenome` file captures an engine's **behavioral personality** — not its DSP code, but how
it responds to the musician. Velocity sensitivity, filter topology, modulation range, coupling
behavior, sonic character.

The genome is the engine's identity card as heard, not as programmed. It enables:

- **Discovery**: find engines that match a desired behavior profile
- **Pairing**: predict which engines will couple productively
- **Documentation**: single-file summary a producer can read in 60 seconds
- **Tooling**: future CLI/API for genome-based engine search and comparison

Genomes are NOT a substitute for presets or documentation. They are a structured summary
of behavioral tendencies derived from source code and seance findings.

---

## File Format

`.xogenome` files are JSON. One file per engine. Filename is `{engine_id_lowercase}.xogenome`
(e.g., `oware.xogenome`). Located in `Docs/genomes/`.

---

## Schema

```json
{
  "schema_version": "0.1",
  "engine_id": "string — canonical uppercase name (e.g., 'OWARE')",
  "engine_prefix": "string — parameter ID prefix with trailing underscore (e.g., 'owr_')",
  "collection": "string|null — expansion family name, or null for base fleet",

  "oscillator": {
    "type": "enum: subtractive | additive | wavetable | granular | fm | physical_model | spectral_stft | drum_circuit | noise_layered | cellular_automata | hybrid",
    "polyphony": "integer — max simultaneous voices (kMaxVoices)",
    "architecture": "string — one sentence describing oscillator topology"
  },

  "filter": {
    "topology": "enum: svf | ladder | comb | formant | bpf_tanh | none | custom",
    "modes": ["array of: lowpass | highpass | bandpass | notch | allpass"],
    "resonance_range": [0.0, 1.0],
    "key_tracking": "boolean"
  },

  "envelope": {
    "type": "enum: adsr | ad | ahd | ar | multi_stage | custom",
    "stages": "integer — stage count in primary amplitude envelope",
    "curve": "enum: linear | exponential | mixed | linear_attack_exp_decay"
  },

  "modulation": {
    "lfo_count": "integer — number of independent LFOs",
    "lfo_shapes": ["array of: sine | triangle | saw | square | s_and_h | random"],
    "mod_matrix_depth": "integer — assignable modulation sources (0 if fixed routing)",
    "audio_rate_mod": "boolean — any mod source capable of audio-rate operation (>20 Hz)"
  },

  "response_surface": {
    "velocity_sensitivity": "float 0.0-1.0 — how strongly velocity reshapes timbre (0=volume only, 1=dramatic timbral shift)",
    "velocity_targets": ["array of: amplitude | filter | attack | brightness | decay | pitch | morph | effort | mallet_hardness | body_resonance | transient"],
    "pitch_bend_range": "integer — default pitch bend range in semitones",
    "aftertouch_response": "enum: none | filter | amplitude | vibrato | multi | custom",
    "dynamic_range_db": "number — approximate usable dynamic range in dB"
  },

  "macro_personality": {
    "macro1": {
      "name": "string — display name as shown in UI",
      "behavior": "string — what it controls and how (1-2 sentences)"
    },
    "macro2": { "name": "string", "behavior": "string" },
    "macro3": { "name": "string", "behavior": "string" },
    "macro4": { "name": "string", "behavior": "string" }
  },

  "sonic_character": {
    "primary_category": "enum: bass | lead | pad | keys | drums | strings | brass | vocal | texture | fx | tuned_percussion | spectral | modular | generative | hybrid",
    "brightness": "float 0.0-1.0 — spectral centroid tendency (0=dark/subby, 1=bright/fizzy)",
    "warmth": "float 0.0-1.0 — analog roundness and harmonic density in low-mids",
    "movement": "float 0.0-1.0 — how much sound evolves over time without player input",
    "density": "float 0.0-1.0 — spectral/polyphonic density (0=sparse, 1=dense/lush)",
    "transient_character": "enum: sharp | soft | struck | plucked | bowed | electronic | complex | none"
  },

  "coupling_affinity": {
    "best_as_source": ["array of CouplingType values — what this engine sends strongly"],
    "best_as_target": ["array of CouplingType values — what this engine receives productively"],
    "recommended_partners": ["array of ENGINE_ID strings"],
    "coupling_personality": "string — how this engine behaves in coupling relationships (1-2 sentences)"
  },

  "mythology": {
    "creature": "string — creature or object identity from XO_OX lore",
    "depth_zone": "enum: surface | shallows | open_water | reef | twilight | deep | abyss | hadal",
    "felix_oscar_polarity": "float 0.0-1.0 — 0.0=pure feliX (bright/transient/electric), 1.0=pure Oscar (dark/sustained/abyssal)",
    "accent_color": "string — #RRGGBB hex"
  },

  "metadata": {
    "param_count": "integer — total declared parameters including macros",
    "preset_count": "integer — factory preset count at genome creation time",
    "seance_score": "float|null — ghost council score, null if unscored",
    "guru_bin_retreat": "boolean — has a completed Guru Bin retreat doc",
    "version_added": "string — XOceanus version shipped in (e.g., 'V1.0')"
  }
}
```

---

## CouplingType Reference

Valid values for `best_as_source` and `best_as_target`:

| Value | Meaning |
|-------|---------|
| `AmpToFilter` | Amplitude → filter cutoff |
| `AmpToPitch` | Amplitude → pitch |
| `LFOToPitch` | LFO → pitch modulation |
| `EnvToMorph` | Envelope → wavetable/morph position |
| `AudioToFM` | Audio → FM input |
| `AudioToRing` | Audio × audio ring modulation |
| `FilterToFilter` | Filter output → another filter input |
| `AmpToChoke` | Amplitude chokes another engine |
| `RhythmToBlend` | Rhythm pattern → blend parameter |
| `EnvToDecay` | Envelope → decay time |
| `PitchToPitch` | Pitch → pitch (harmony/interval) |
| `AudioToWavetable` | Audio → wavetable source |
| `AudioToBuffer` | Audio → ring buffer (continuous streaming) |
| `KnotTopology` | Bidirectional irreducible entanglement |

---

## FeliX-Oscar Polarity Reference

| Range | Description |
|-------|-------------|
| 0.0–0.2 | Pure feliX: surface, bright, transient-rich, electric. Electric Blue / Neon Green. |
| 0.2–0.4 | feliX-leaning: upper water column, energetic with some warmth. Vermillion / Amber. |
| 0.4–0.6 | Balanced: mid-water, complex character, both brightness and depth. Gold tones. |
| 0.6–0.8 | Oscar-leaning: deep water, dark and sustained, slow evolution. Teal / Slate. |
| 0.8–1.0 | Pure Oscar: abyssal, very dark, long sustain, sub-sonic tendencies. Trench Violet. |

---

## Sonic Character Calibration

Calibrated against the XO_OX fleet center of mass:
- **Brightness 0.1**: OCEANDEEP (Darkness Filter 50–800 Hz ceiling)
- **Brightness 0.5**: OWARE (midrange marimba/gamelan character)
- **Brightness 0.9**: OPENSKY, ORIGAMI (bright shimmer and spectral content)
- **Movement 0.1**: OBBLIGATO (slow choir, nearly static when held)
- **Movement 0.5**: ORIGAMI (spectral modulation via LFOs)
- **Movement 0.9**: ORGANISM (cellular automata, always evolving)
- **Warmth 0.9**: OWARE, OSPREY (woody/acoustic resonance)
- **Warmth 0.1**: OBRIX (modular/digital — cold by default, warmed by configuration)

---

## Versioning

Schema version `"0.1"` is this draft. Breaking changes (removed or renamed fields) increment
to `"1.0"`. Additive fields increment the minor version only.

The genome project is VQ 009 (Vision Quest 009 — Engine Genome Project). See `Docs/genomes/README.md`.

---

## Usage Examples

**Find all engines bright enough to pair with a dark sub bass:**
```bash
jq 'select(.sonic_character.brightness > 0.7)' Docs/genomes/*.xogenome
```

**Find all engines that welcome EnvToMorph coupling input:**
```bash
jq 'select(.coupling_affinity.best_as_target[] | contains("EnvToMorph"))' Docs/genomes/*.xogenome
```

**Find engines with Guru Bin retreat and seance score at or above 9.0:**
```bash
jq 'select(.metadata.seance_score >= 9.0 and .metadata.guru_bin_retreat == true)' Docs/genomes/*.xogenome
```

**List all engines by felix_oscar_polarity (brightest first):**
```bash
jq -s 'sort_by(.mythology.felix_oscar_polarity) | .[] | {id: .engine_id, polarity: .mythology.felix_oscar_polarity}' Docs/genomes/*.xogenome
```
