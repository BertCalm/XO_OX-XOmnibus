# XOnset — Percussive Synthesis Engine Specification

**Version:** 0.1 (Design Draft)
**Author:** XO_OX Designs
**Scope:** Percussive synth engine for the XO_OX Mega-Tool platform
**Status:** Concept design — awaiting approval
**Naming:** "Onset" = the technical term for a percussive attack transient. XO + nset.

---

## 1. Design Thesis

Every drum machine in history forces a choice: analog OR digital, circuit OR algorithm, warm OR complex. XOnset refuses to choose. Each voice holds **two synthesis paradigms simultaneously** — a circuit-modeled analog layer and an algorithmic digital layer — and a single **Blend** axis morphs between them continuously. The result is percussion that doesn't exist in any drum machine, sampler, or groovebox: the sub-weight of an 808 kick fused with the modal ring of a struck membrane, or the noise-burst snap of a 909 snare dissolving into Karplus-Strong metallic shimmer.

**Core Innovation:** Per-voice, per-step synthesis paradigm morphing — not mode switching, but continuous interpolation between fundamentally different DSP architectures.

**Brand Fit:** The X/O duality maps perfectly. Layer X = Circuit (analog warmth, physical punch). Layer O = Algorithm (digital complexity, metallic precision). The Blend axis IS the XO concept applied to rhythm.

---

## 2. Historical DNA — What We're Drawing From

### 2.1 Circuit Heritage (Layer X)

| Source | Technique Absorbed | What We Take |
|--------|-------------------|--------------|
| **TR-808** | Bridged-T bandpass oscillator + feedback buffer | The "ringing impulse" kick architecture, 6-oscillator metallic hat, multi-burst clap envelope |
| **TR-909** | Pitch-swept sawtooth→waveshaper + noise transient layer | The explicit pitch envelope + click injection for kick attack, dual-oscillator snare body |
| **Analog Rytm** | Dual VCO per voice + "snap/tick" transient parameters | Per-voice transient designer separate from body, analog noise source per voice |
| **Self-oscillating filters** | Resonance at threshold + pitch envelope | Filter-as-oscillator for sub kicks, the natural decay = amplitude envelope trick |

**Layer X Philosophy:** Every sound starts from an electrical impulse exciting a resonant circuit. The circuit's physical characteristics (capacitance, resistance, feedback) determine timbre. We model this with bridged-T networks, self-oscillating SVFs, and envelope-controlled VCAs — the proven vocabulary of analog drum machines.

### 2.2 Algorithm Heritage (Layer O)

| Source | Technique Absorbed | What We Take |
|--------|-------------------|--------------|
| **FM Synthesis (DX7/TX81Z)** | Carrier-modulator with independent envelopes, non-integer ratios | 2-op FM with decaying mod index for transient brightness, inharmonic ratios for metallic timbre |
| **Mutable Instruments Plaits** | Macro-oscillator with percussion-specific algorithms | The "one-knob-per-character" approach, band-limited everywhere |
| **Karplus-Strong** | Delay line + averaging filter | Plucked/struck string and membrane sounds, the blend factor controlling metallic vs tonal |
| **Modal Synthesis** | Parallel resonator banks with material-specific modes | 8-mode resonator for membrane/bar/plate, frequency-dependent damping |
| **Sonic Charge Microtonic** | 1-op FM oscillator + filtered noise + clap envelope | The elegance of OSC+NOISE as a complete percussion voice, audio-rate LFO as FM |
| **Phase Distortion (Casio CZ)** | Non-linear phase accumulator, resonant waveshapes | Sharp transient clicks, icy metallic textures, DCW as timbral brightness control |

**Layer O Philosophy:** Every sound starts from a mathematical model — a frequency ratio, a delay length, a resonator bank, a phase distortion curve. The algorithm's parameters determine timbre with surgical precision. We implement 4 selectable algorithms per voice, each tuned for percussion.

### 2.3 What Nobody Has Done (Our Innovation)

| Existing Approach | Limitation | XOnset Solution |
|-------------------|-----------|-----------------|
| 808/909 clones (many) | Fixed circuit, no evolution | Circuit layer is one axis of a morphable blend |
| Nord Drum (mode switching) | Choose VA or FM or modal — can't blend | All paradigms available simultaneously via Blend |
| Analog Rytm (analog+sample) | Sample is static, no synthesis on digital layer | Both layers are fully synthesized, no samples |
| Microtonic (osc+noise) | Single FM operator, no physical modeling | 4 algorithm modes including modal + K-S + FM |
| Plaits (macro algorithms) | One algorithm at a time | Two layers blended, plus cross-voice coupling |

**The gap:** No drum synth lets you continuously morph between an analog circuit model and an algorithmic model on the same voice. XOnset fills this gap.

---

## 3. Architecture

### 3.1 Voice Structure (8 Voices)

```
                    XOnset Voice Architecture (per voice)
                    ═══════════════════════════════════════

MIDI Trigger ──► Transient Designer ──────────────────────────┐
    │                (snap, click, pitch spike)                │
    │                                                          │
    ├──► Layer X (Circuit) ──────────┐                        │
    │    ┌─────────────────────┐     │                        │
    │    │ Analog Core:        │     │    ┌──────────────┐    │
    │    │  • Bridged-T osc    │     ├───►│              │    │
    │    │  • Self-osc filter  │     │    │   BLEND      │    │
    │    │  • Pitch envelope   │     │    │   CROSSFADE  │◄───┘ Transient
    │    │  • Noise burst      │     │    │   (0.0-1.0)  │      injection
    │    │  • Feedback VCA     │     │    │              │
    │    └─────────────────────┘     │    │  X ◄────► O  │
    │                                │    │              │──► Per-Voice Filter
    ├──► Layer O (Algorithm) ────────┘    │  Circuit  Algo│      (Cytomic SVF)
    │    ┌─────────────────────┐          │              │         │
    │    │ Algorithm Core:     │          └──────────────┘         │
    │    │  Mode select:       │                                   │
    │    │  • FM (2-op + fb)   │                                   ▼
    │    │  • Modal (8 res.)   │                              Amp Envelope
    │    │  • Karplus-Strong   │                              (AD/AHD/ADSR)
    │    │  • Phase Distortion │                                   │
    │    └─────────────────────┘                                   ▼
                                                             Voice Output
                                                          (to mixer + coupling)
```

### 3.2 Global Signal Flow

```
Voice 1 ──┐
Voice 2 ──┤
Voice 3 ──┤                    ┌─────────────────┐
Voice 4 ──┼──► Voice Mixer ──►│ Cross-Voice      │──► Character Stage
Voice 5 ──┤   (level, pan)    │ Coupling Matrix  │    (Grit/Warmth)
Voice 6 ──┤                    └─────────────────┘         │
Voice 7 ──┤                                                ▼
Voice 8 ──┘                                          Shared FX Rack
                                                    [Delay][Reverb][LoFi]
                                                           │
                                                     Master Output
                                                    ──► SynthEngine interface
                                                    ──► Coupling bus (mega-tool)
```

### 3.3 Per-Voice Parameters (~12 per voice, ~96 total + globals)

| Parameter | Range | Default | Purpose |
|-----------|-------|---------|---------|
| `v{n}_blend` | 0.0–1.0 | 0.5 | Circuit (X) ↔ Algorithm (O) mix |
| `v{n}_algo_mode` | 0–3 | 0 (FM) | Algorithm layer mode select |
| `v{n}_pitch` | -24 to +24 st | 0 | Coarse pitch offset from MIDI note |
| `v{n}_fine` | -100 to +100 ct | 0 | Fine pitch |
| `v{n}_decay` | 1ms–8s | voice-dependent | Primary decay time |
| `v{n}_tone` | 0.0–1.0 | 0.5 | Brightness / filter cutoff |
| `v{n}_snap` | 0.0–1.0 | 0.3 | Transient sharpness (pitch env depth + noise burst) |
| `v{n}_body` | 0.0–1.0 | 0.5 | Low-frequency content / sub weight |
| `v{n}_character` | 0.0–1.0 | 0.0 | Layer-specific timbre control (see §3.4) |
| `v{n}_level` | -inf–0 dB | -6 dB | Voice output level |
| `v{n}_pan` | -1.0 to +1.0 | 0.0 | Stereo position |
| `v{n}_env_shape` | 0–2 | 0 (AD) | Envelope mode: AD, AHD, ADSR |

### 3.4 The `character` Parameter — Layer-Adaptive

The `character` knob changes meaning based on which layer dominates (blend position):

| Blend Position | Character Controls (Layer X) | Character Controls (Layer O) |
|---------------|-------|-------|
| X-dominant (0.0–0.3) | Feedback amount (808-style sustain) | — |
| Blended (0.3–0.7) | Interpolated: feedback ↔ algo param | Interpolated |
| O-dominant (0.7–1.0) | — | FM: mod index / Modal: inharmonicity / K-S: blend factor / PD: DCW amount |

This means the same knob always does "make it more interesting" regardless of synthesis method — but the DSP behind it adapts.

### 3.5 Default Voice Assignments

| Voice | Name | Default Blend | Default Algo | Default Note | Character |
|-------|------|--------------|-------------|-------------|-----------|
| V1 | **Kick** | 0.2 (X-heavy) | Modal | C1 (36) | Sub-weight, long pitch sweep |
| V2 | **Snare** | 0.5 (blended) | FM | D1 (38) | Tonal body + noise rattle |
| V3 | **Hat Closed** | 0.7 (O-heavy) | FM | F#1 (42) | Metallic, short decay |
| V4 | **Hat Open** | 0.7 (O-heavy) | FM | A#1 (46) | Metallic, long decay |
| V5 | **Clap** | 0.4 (slight X) | Phase Dist. | D#1 (39) | Multi-burst envelope |
| V6 | **Tom** | 0.3 (slight X) | Modal | A1 (45) | Pitched body, warm |
| V7 | **Perc A** | 0.6 (slight O) | Karplus-Strong | C#1 (37) | Metallic/plucked |
| V8 | **Perc B** | 0.8 (O-heavy) | Modal | G#1 (44) | Bell/bar resonance |

---

## 4. Synthesis Layer Details

### 4.1 Layer X: Circuit Modeling

Four circuit topologies per voice, auto-selected based on voice type (user can override):

#### 4.1.1 Bridged-T Kick (Voices 1, 6)

Direct descendant of the TR-808 kick circuit:

```
Trigger pulse (1ms, sharp)
    ──► Bridged-T bandpass network (center freq from pitch param)
    ──► Feedback buffer (amount from character param)
    ──► Pitch envelope: exponential sweep from snap×4 octaves above fundamental
    ──► Output through tone filter (Cytomic SVF LP)
```

**Key behaviors:**
- `snap` controls pitch envelope depth: 0 = pure thud, 1 = dramatic 4-octave sweep
- `body` controls sub-harmonic emphasis via parallel sub-oscillator (one octave below)
- `decay` controls feedback buffer gain (more feedback = longer ring)
- `character` controls feedback nonlinearity (clean sine → saturated harmonics)
- Diode-starvation pitch drift modeled: subtle downward pitch creep through decay (the 808 signature)

#### 4.1.2 Noise-Burst Snare (Voices 2, 5)

Hybrid of 808 and 909 snare circuits:

```
Trigger
    ├──► Bridged-T oscillator #1 (~180 Hz, 0,1 membrane mode)
    ├──► Bridged-T oscillator #2 (~330 Hz, tuned by pitch param)
    ├──► White noise ──► HPF ──► VCA (envelope)
    └──► Mix (tone param controls oscillator/noise ratio)
        ──► Output through tone filter
```

**Key behaviors:**
- `tone` = tonal vs noise balance (0 = pure body, 1 = pure rattle)
- `snap` = noise burst attack sharpness + oscillator pitch envelope
- `character` = noise color (white → pink → bandpass-filtered "vintage")
- For clap voices: multi-burst envelope (4 rapid gates at 10ms intervals + decay tail)

#### 4.1.3 Six-Oscillator Metallic (Voices 3, 4)

Direct 808 hi-hat/cymbal topology:

```
6 square-wave oscillators at non-harmonic frequencies
    ──► Sum
    ──► Dual bandpass filters (3440 Hz + 7100 Hz)
    ──► Gating circuits with separate time constants
    ──► HPF
    ──► Output (decay param selects open/closed character)
```

**Key behaviors:**
- `pitch` shifts all 6 frequencies proportionally (preserving inharmonic relationships)
- `tone` controls bandpass filter balance (low BP emphasis vs high BP emphasis)
- `decay` = closed hat (~50ms) to open hat (~600ms) to cymbal (~1200ms)
- `character` = oscillator waveshape (square → pulse → noise-modulated)

#### 4.1.4 Self-Oscillating Filter Kick (Alternative for V1)

```
SVF set to resonance ~0.99 (just below self-oscillation)
    ──► Impulse trigger pushes filter into momentary oscillation
    ──► Cutoff envelope: exponential sweep controlled by snap
    ──► Natural energy dissipation = amplitude decay
```

Provides a different kick character than bridged-T: cleaner fundamental, less harmonic content, purer sub weight.

### 4.2 Layer O: Algorithmic Synthesis

Four selectable algorithms per voice:

#### 4.2.1 FM Percussion (2-Operator + Feedback)

```
Modulator (freq = carrier × ratio)
    │  Envelope: fast attack, decay controlled by snap
    │  Index: controlled by character param
    ▼
Carrier (freq from pitch param)
    │  Envelope: attack + decay from voice envelope
    │  Self-feedback: adds noise-like complexity
    ▼
Output
```

**Ratio presets for percussion:**
- 1:1 — warm, bell-like kick body
- 1:1.4 — inharmonic, clangy cowbell
- 1:2.76 — metallic, cymbal-adjacent
- 1:7.12 — harsh, industrial
- `character` = modulation index (brightness/complexity)
- `tone` = carrier feedback amount

#### 4.2.2 Modal Resonator (8-Mode Bank)

```
Excitation signal (noise burst or impulse, shaped by snap)
    ──► 8 parallel bandpass resonators
        Each resonator: frequency, amplitude, decay time
        Frequencies follow material model (membrane/bar/plate)
    ──► Sum
    ──► Output
```

**Material models:**
- **Membrane** (drum head): Bessel function zero ratios — 1.0, 1.59, 2.14, 2.30, 2.65, 2.92, 3.16, 3.50
- **Bar** (xylophone/marimba): Stiffness ratios — 1.0, 2.76, 5.40, 8.93, 13.3, 18.6, 24.8, 31.9
- **Plate** (cymbal-like): f_mn proportional to (m² + αn²) — dense, complex
- `character` = inharmonicity factor B (0 = pure harmonic, 1 = metallic/bell)
- `tone` = excitation position (changes which modes are excited: center = low modes, edge = high modes)
- Frequency-dependent damping: higher modes decay faster (the key "naturalness" cue)

#### 4.2.3 Karplus-Strong (Plucked/Struck)

```
Delay line (length = 1/frequency from pitch param)
    ──► Initialize with noise burst (length/color from snap + tone)
    ──► Averaging filter at delay line output (controls decay rate)
    ──► Feedback loop
    ──► Output
```

**Key behaviors:**
- `character` = blend factor b (0.0 = snare-like, 0.5 = metallic, 1.0 = tonal/string)
- `decay` = feedback coefficient (more feedback = longer sustain)
- `snap` = excitation noise burst length (short = pluck, long = strike)
- `tone` = averaging filter cutoff (high = bright/metallic, low = dull/woody)
- Sign inversion probability creates drum-like vs string-like character

#### 4.2.4 Phase Distortion (Sharp Transients)

```
DDS oscillator with non-linear phase accumulator
    ──► Phase distortion curve (controlled by character = DCW)
    ──► Resonant waveshape selection (saw-res, tri-res, trap-res)
    ──► Amplitude envelope
    ──► Output
```

**Key behaviors:**
- `character` = DCW (distortion amount: 0 = pure waveform, 1 = maximum harmonic content)
- `tone` = waveshape select (smoothly morphable between 3 resonant shapes)
- `snap` = DCW envelope depth (fast DCW sweep = bright transient → dark body)
- Excellent for: sharp clicks, icy metallic transients, tuned percussion, electronic rim shots

---

## 5. The Blend Engine — How Morphing Works

### 5.1 Equal-Power Crossfade (Not Linear)

Linear crossfading creates a -6dB dip at center. We use equal-power (sine/cosine) curves:

```cpp
float blendX = std::cos(blend * juce::MathConstants<float>::halfPi);
float blendO = std::sin(blend * juce::MathConstants<float>::halfPi);
float output = layerX_sample * blendX + layerO_sample * blendO;
```

At blend = 0.5, both layers contribute at -3dB each — perceived as equal loudness.

### 5.2 Parameter Coupling Across Layers

When blend is not at an extreme, shared parameters affect both layers simultaneously:

- `pitch` → both layers track the same fundamental
- `decay` → both layer envelopes use the same decay time
- `snap` → Layer X pitch envelope depth AND Layer O mod index/excitation burst length
- `tone` → Layer X noise filter AND Layer O brightness/filter cutoff

This ensures the blend sounds **cohesive**, not like two unrelated sounds layered.

### 5.3 Transient Designer (Layer-Independent)

The transient stage fires **before** the blend, injecting a synthesis-agnostic click/snap:

```
Trigger ──► Pitch spike (1-6ms, 2-4 octaves above fundamental)
         ──► Noise burst (white, 1-3ms, HPF at 2kHz)
         ──► Mix (snap param controls both depth)
         ──► Inject into blend output
```

This ensures every voice has consistent attack character regardless of blend position. The 808's "retriggering circuit" and 909's "click" are both doing this — we make it explicit and controllable.

### 5.4 Per-Step Blend Modulation (Sequencer Integration)

The step sequencer can modulate blend per step:

```
Step 1: blend=0.0 (pure 808 kick)
Step 2: blend=0.0
Step 3: blend=0.5 (hybrid kick)
Step 4: blend=1.0 (modal membrane kick)
```

This creates **evolving drum patterns** where the synthesis method itself changes rhythmically — something no existing drum machine can do.

---

## 6. Cross-Voice Coupling Matrix

### 6.1 The XO_OX Signature Applied to Drums

Just as OddfeliX/OddOscar couples Engine X and Engine O, XOnset couples drum voices with each other. This is what makes it an XO_OX instrument rather than just another drum synth.

```
┌─────────────────────────────────────────┐
│         Cross-Voice Coupling Matrix     │
│                                         │
│         Kick  Snare  HH-C  HH-O  ...   │
│  Kick    ─     ○      ○     ○          │
│  Snare   ○     ─      ○     ○          │
│  HH-C    ○     ○      ─     ●←choke   │
│  HH-O    ○     ○      ●     ─          │
│                                         │
│  ○ = available coupling point           │
│  ● = default normalled connection       │
│  ─ = self (N/A)                         │
└─────────────────────────────────────────┘
```

### 6.2 Coupling Types

| Type | Signal Path | Musical Effect | Example |
|------|------------|---------------|---------|
| **Amp→Filter** | Voice A amplitude → Voice B filter cutoff | Rhythmic filtering | Kick ducks snare brightness |
| **Amp→Pitch** | Voice A amplitude → Voice B pitch | Pitch ducking | Kick pushes tom pitch down |
| **Amp→Choke** | Voice A trigger → Voice B kill | Exclusive grouping | Closed hat chokes open hat |
| **Rhythm→Blend** | Voice A trigger pattern → Voice B blend | Synthesis morphing | Kick pattern controls snare's analog/digital balance |
| **Env→Decay** | Voice A envelope → Voice B decay time | Dynamic interaction | Snare shortens when kick is long |
| **Audio→Ring** | Voice A output × Voice B output | Metallic fusion | Tom × perc = bell texture |

### 6.3 Default Normalled Connections (Moog Matriarch Pattern)

These are pre-wired but overridable — the kit sounds musical without any user patching:

1. **HH-Closed chokes HH-Open** (standard exclusive group)
2. **Kick Amp → Snare Filter** at 15% (subtle brightness ducking — the dub pump for drums)
3. **Snare Amp → HH-Closed Decay** at 10% (snare slightly tightens hat — creates groove cohesion)

All other coupling points default to 0% (disconnected).

---

## 7. Mega-Tool Integration

### 7.1 SynthEngine Interface Implementation

```cpp
namespace xonset {

class XOnsetEngine : public SynthEngine {
public:
    void prepare(double sampleRate, int blockSize) override;
    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi) override;

    // Cross-engine coupling (mega-tool)
    float getSampleForCoupling() const override {
        return masterOutput; // summed drum output for coupling
    }

    void applyCouplingInput(float value, CouplingType type) override {
        // External engine modulates XOnset parameters
        // e.g., pad engine envelope → kick decay
        // e.g., bass engine amplitude → hat filter
    }

    int getParameterCount() const override { return kTotalParams; }
    juce::StringArray getParameterIDs() const override;

    // XOnset-specific: per-voice coupling output for granular cross-engine mod
    float getVoiceOutput(int voiceIndex) const;

private:
    static constexpr int kNumVoices = 8;
    std::array<DrumVoice, kNumVoices> voices;
    CrossVoiceCouplingMatrix couplingMatrix;
    ParamSnapshot snap;

    // Shared FX (or routed to mega-tool shared rack)
    bool useSharedFX = false;
};

} // namespace xonset
```

### 7.2 Cross-Engine Coupling Scenarios

When XOnset is loaded alongside other engines in the mega-tool:

| Scenario | XOnset Sends | Other Engine Receives | Musical Effect |
|----------|-------------|---------------------|---------------|
| **Drum + Pad** | Kick amplitude envelope | Pad engine filter cutoff | Dub pump — pads breathe with the kick |
| **Drum + Bass** | Kick trigger | Bass engine pitch reset | Bass re-triggers on every kick (lock groove) |
| **Drum + Lead** | Hat pattern triggers | Lead engine blend/morph | Lead timbre shifts with hat rhythm |
| **Drum + Drone** | Snare amplitude | Drone engine FM index | Snare hits brighten the drone momentarily |
| **Pad + Drum** | Pad engine LFO | XOnset kick decay time | Kick decay breathes with pad movement |
| **Bass + Drum** | Bass engine amplitude | XOnset snare blend | When bass is loud, snare goes more analog; when bass drops, snare goes digital |

### 7.3 Parameter Namespacing

Following the mega-tool convention from the feasibility study:

```
// Voice parameters
perc_v1_blend, perc_v1_pitch, perc_v1_decay, perc_v1_tone, ...
perc_v2_blend, perc_v2_pitch, perc_v2_decay, perc_v2_tone, ...
...
perc_v8_blend, perc_v8_pitch, perc_v8_decay, perc_v8_tone, ...

// Global parameters
perc_master_level, perc_master_drive, perc_master_tone
perc_coupling_amount   // global coupling intensity
perc_swing, perc_bpm   // sequencer

// Cross-voice coupling
perc_xvc_kick_to_snare_filter, perc_xvc_kick_to_snare_amount
perc_xvc_hat_choke_group
...
```

### 7.4 XO Pad Integration

When XOnset is active in the mega-tool, the XO Pad switches to **Drum Mode**:

```
┌────────┬────────┬────────┬────────┐
│ Tom    │ Perc A │ Perc B │ (acc.) │  ← Row 3: auxiliary voices
├────────┼────────┼────────┼────────┤
│ Clap   │ HH-C   │ HH-O   │ (acc.) │  ← Row 2: upper kit
├────────┼────────┼────────┼────────┤
│ Kick   │ Snare  │ Kick   │ Snare  │  ← Row 1: core (doubled for finger drumming)
├────────┼────────┼────────┼────────┤
│        Performance Strip            │  ← Swing + coupling macro
└─────────────────────────────────────┘

Per-pad X-axis: synthesis BLEND (left = Circuit, right = Algorithm)
Per-pad Y-axis: DECAY time (bottom = short, top = long)
Velocity: mapped to snap (harder hit = sharper transient)
```

This means every finger drum hit can have a different synthesis character based on WHERE within the pad you touch. Hit the left side of the kick pad for a pure 808. Hit the right side for a modal membrane. Hit the center for something new.

---

## 8. Macro System

### 8.1 Four Performance Macros (Gold Knobs)

Following the OddfeliX/OddOscar pattern of 4 macros on the header bar:

| Macro | Name | What It Controls | Musical Intent |
|-------|------|-----------------|---------------|
| M1 | **MACHINE** | All voice blend positions simultaneously | Morph entire kit from analog to digital |
| M2 | **PUNCH** | All voice snap + body parameters | Global transient intensity (soft → aggressive) |
| M3 | **SPACE** | Shared reverb send + delay feedback | Dry, tight kit → drowned in dub echo |
| M4 | **MUTATE** | Randomize blend + character within ±20% of current | Evolve the kit — each turn of the knob nudges every voice slightly |

### 8.2 MACHINE Macro — The Signature Control

The MACHINE macro is XOnset's equivalent of OddfeliX/OddOscar's Coupling Amount knob — the single control that defines the instrument's identity.

```
MACHINE = 0.0        MACHINE = 0.5          MACHINE = 1.0
╔══════════════╗     ╔══════════════╗       ╔══════════════╗
║  Pure 808 Kit ║     ║  Hybrid Kit  ║       ║  Modal/FM Kit ║
║  Warm, sub-   ║     ║  Best of     ║       ║  Metallic,    ║
║  heavy,       ║     ║  both —      ║       ║  complex,     ║
║  classic      ║     ║  new sounds  ║       ║  physical     ║
╚══════════════╝     ╚══════════════╝       ╚══════════════╝
```

Turning MACHINE from 0→1 during a performance transforms an entire drum kit from analog warmth to algorithmic complexity **in real time**.

---

## 9. Step Sequencer Integration

### 9.1 Per-Voice Lanes

```
Voice   | Step 1 | Step 2 | Step 3 | Step 4 | Step 5 | ...
─────────┼────────┼────────┼────────┼────────┼────────┼─────
Kick     |  ●     |        |        |  ●     |        |
Snare    |        |        |  ●     |        |        |  ●
HH-C     |  ●     |  ●     |  ●     |  ●     |  ●     |  ●
HH-O     |        |        |        |        |  ◐     |
Clap     |        |        |  ●     |        |        |
```

### 9.2 Per-Step Parameter Locks

Each step can override any voice parameter (Elektron-style):

- **Blend lock:** Step 3 kick = blend 0.8 (algorithmic), all other steps = default 0.2
- **Pitch lock:** Tom pattern with descending pitch per step
- **Decay lock:** Hat opening over 4 steps (decay 0.05 → 0.1 → 0.2 → 0.6)
- **Snap lock:** Snare gets harder transient on beat 3 (accent)

### 9.3 Probability + Condition Triggers

Borrowed from OddfeliX/OddOscar's sequencer:

- Per-step probability (0-100%)
- Condition triggers: "play every 2nd time", "play only if previous step played"
- Ratchets: 1-4 micro-repeats within a step (jungle-style)

---

## 10. Preset Philosophy

### 10.1 Category System

| Category | Count | Character | Blend Range |
|----------|-------|-----------|-------------|
| **Circuit Kits** | 15 | Pure analog character, warm, classic | 0.0–0.3 |
| **Algorithm Kits** | 15 | Digital precision, metallic, complex | 0.7–1.0 |
| **Hybrid Kits** | 20 | The sweet spot — new sounds neither layer produces alone | 0.3–0.7 |
| **Coupled Kits** | 15 | Heavy cross-voice coupling, interactive, living patterns | Any |
| **Morphing Kits** | 10 | Sequencer with per-step blend locks, evolving within a pattern | Varies |
| **XO Fusion Kits** | 10 | Designed for mega-tool coupling with other engines | Context-dependent |
| **Total** | **85** | | |

### 10.2 Hero Presets (First Impressions)

| Name | Category | Description |
|------|----------|-------------|
| **808 Reborn** | Circuit | Classic 808 kit — but with modern sub weight and the snap control the original never had |
| **Membrane Theory** | Algorithm | Modal resonator kit — every hit sounds like a physically struck object |
| **Quantum Kit** | Hybrid | Blend at 0.5 for every voice — sounds unlike any analog or digital drum machine |
| **Living Machine** | Coupled | Kick↔Snare coupling at 40%, hat↔tom at 25% — the kit breathes and interacts |
| **Time Stretch** | Morphing | 16-step pattern where blend sweeps from 0→1 across the bar — kit evolves in real time |
| **Dub Pressure** | XO Fusion | Designed to couple with OddfeliX/OddOscar's Engine O — drum hits pump the pad engine |
| **Future 909** | Hybrid | 909-inspired architecture but with FM metallic hats and modal toms |
| **Particle Drums** | Algorithm | Karplus-Strong + Phase Distortion — crystalline, icy, microscopic |
| **Analog Heart** | Circuit | Self-oscillating filter kicks, noise-burst snares, pure circuit character |
| **Mutant Factory** | Morphing | MUTATE macro pre-mapped to sequencer — every bar the kit shifts slightly |

---

## 11. Implementation Phases

### Phase 1: Core Voices (Foundation)

**Goal:** 8 working drum voices with Layer X (circuit) only. No blend, no algorithms yet.

1. Define `xonset` namespace + `Parameters.h` (all ~110 parameter IDs)
2. Implement `ParamSnapshot` for XOnset parameters
3. Implement `DrumVoice` base class with:
   - Bridged-T oscillator (kick/tom/conga)
   - Noise-burst snare circuit
   - 6-oscillator metallic hat/cymbal
   - Self-oscillating filter kick (alternative)
4. Implement `VoicePool` (8 voices, trigger-per-note, no polyphony per voice)
5. Implement per-voice AD envelope + Cytomic SVF filter
6. Wire up to MIDI (General MIDI drum map by default)
7. **Test:** Each circuit voice sounds correct in isolation

**Reusable modules from existing XO_OX code:**
- `CytomicSVF` (filter) — direct reuse
- `AdsrEnvelope` (adapted to AD/AHD modes) — 90% reuse
- `ParamSnapshot` pattern — 100% reuse
- `PolyBLEP` (for metallic oscillators) — 80% reuse

### Phase 2: Algorithm Layer + Blend

**Goal:** Layer O algorithms implemented, blend morphing working.

8. Implement FM percussion (2-op + feedback)
9. Implement Modal resonator (8-mode bank with 3 material presets)
10. Implement Karplus-Strong percussion
11. Implement Phase Distortion percussion
12. Implement equal-power blend crossfade engine
13. Implement Transient Designer (pre-blend click/snap injection)
14. Implement adaptive `character` parameter routing
15. **Test:** Blend smoothly morphs between layers with no artifacts

### Phase 3: Coupling + Sequencer

**Goal:** Cross-voice coupling and step sequencer with parameter locks.

16. Implement `CrossVoiceCouplingMatrix` (6 coupling types)
17. Implement default normalled connections (hat choke, kick→snare, snare→hat)
18. Implement 16-step sequencer with per-voice lanes
19. Implement per-step parameter locks (blend, pitch, decay, snap)
20. Implement probability triggers + condition system
21. Implement 4 macros (MACHINE, PUNCH, SPACE, MUTATE)
22. **Test:** Coupling creates audible, musical interaction between voices

### Phase 4: Mega-Tool Integration

**Goal:** XOnset works as a registered `SynthEngine` in the mega-tool.

23. Implement `SynthEngine` interface (`prepare`, `renderBlock`, coupling I/O)
24. Register XOnset in engine registry
25. Implement cross-engine coupling outputs (per-voice + master)
26. Implement cross-engine coupling inputs (external mod → XOnset params)
27. Implement XO Pad drum mode (pad layout + blend-on-X-axis)
28. Implement shared FX routing (use mega-tool FX rack when available)
29. **Test:** XOnset couples with OddfeliX/OddOscar — drum hits pump pad engine

### Phase 5: Polish + Presets

**Goal:** 85 factory presets, UI, performance optimization.

30. Design 10 hero presets
31. Design remaining 75 presets across all categories
32. Implement UI (terracotta/teal gradient per voice showing blend position)
33. CPU optimization pass (SIMD for modal resonators, lookup tables for FM)
34. auval validation
35. **Ship**

---

## 12. CPU Budget

| Component | Budget (44.1kHz/512) | Notes |
|-----------|---------------------|-------|
| Layer X (8 voices) | < 4% | Simple circuits, no polyphony per voice |
| Layer O (8 voices) | < 6% | Modal resonators are most expensive (8 biquads × 8 voices) |
| Blend engine | < 0.5% | Just multiply + add |
| Transient designer | < 0.5% | Simple envelope + noise burst |
| Cross-voice coupling | < 1% | Matrix multiply, 8×8 |
| Sequencer | < 0.5% | Event scheduling, negligible |
| Shared FX | < 3% | Reuse existing delay/reverb |
| **Total** | **< 15.5%** | Well within mega-tool budget (8-12% per engine target) |

**Key optimizations:**
- Modal resonators: use NEON/SSE intrinsics for parallel biquad processing
- FM: lookup table for sine, single multiply for modulation
- Karplus-Strong: circular buffer with simple averaging — extremely cheap
- Inactive voices skip processing entirely (no silent rendering)

---

## 13. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Blend morphing produces artifacts at extremes | Medium | Medium | Equal-power crossfade + shared transient designer ensures smooth transitions |
| Modal resonator CPU spikes at high Q | Medium | High | Clamp Q, limit to 8 modes, SIMD optimization |
| Cross-voice coupling creates feedback loops | Low | High | Coupling matrix is unidirectional by default; bidirectional requires explicit user action with gain limiting |
| Too many parameters overwhelm users | High | Medium | MACHINE macro controls everything with one knob; deep parameters hidden by default |
| Blend concept is confusing ("what does 0.5 mean?") | Medium | Medium | Visual gradient on each voice (terracotta→teal) + named blend positions ("808 ← Hybrid → Modal") |
| Mega-tool parameter ID collisions | Low | Critical | Strict `perc_v{n}_` namespacing from day 1 |

---

## 14. What Makes XOnset Unique

### 14.1 Against The Competition

| Feature | XOnset | TR-808 Clone | Microtonic | Analog Rytm | Nord Drum |
|---------|--------|-------------|------------|-------------|-----------|
| Analog circuit modeling | Yes | Yes | No | Yes | VA only |
| FM synthesis | Yes | No | 1-op | No | Yes |
| Physical modeling (modal) | Yes | No | No | No | Yes (limited) |
| Karplus-Strong | Yes | No | No | No | No |
| Phase distortion | Yes | No | No | No | No |
| **Continuous blend between paradigms** | **Yes** | **No** | **No** | **No** | **No** |
| Cross-voice coupling | Yes | No | No | No | No |
| Per-step synthesis morphing | Yes | No | No | Partial (p-lock) | No |
| Mega-tool engine coupling | Yes | No | No | No | No |

### 14.2 The One-Sentence Pitch

**"XOnset is the drum machine that morphs between an 808 and a physics simulation on every hit — and couples its voices together like neurons in a rhythm brain."**

---

## 15. Connection to XO_OX Brand Identity

| XO_OX Principle | XOnset Expression |
|----------------|-------------------|
| **Duality (X/O)** | Circuit layer (X) vs Algorithm layer (O) per voice |
| **Coupling** | Cross-voice coupling matrix + mega-tool cross-engine coupling |
| **Character over features** | MACHINE macro = one knob transforms everything |
| **Normalled routing** | Default couplings make the kit musical without patching |
| **Progressive disclosure** | Simple: 8 pads + MACHINE knob. Deep: 110 parameters + coupling matrix |
| **XO Pad integration** | Drum mode with blend-on-X-axis per hit |
| **Terracotta/Teal visual** | Per-voice gradient showing blend position |
| **Jungle intelligence** | Probability sequencer + ratchets + cross-voice coupling = living rhythms |

---

*CONFIDENTIAL — XO_OX Internal Design Document*
