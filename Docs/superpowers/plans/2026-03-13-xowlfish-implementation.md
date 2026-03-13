# XOwlfish Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build XOwlfish — a monophonic subharmonic organism synth — as a standalone JUCE plugin, then integrate it into XOmnibus as engine #22.

**Architecture:** Single monophonic voice with 5 interdependent organs (Abyss Habitat subharmonic oscillator → Owl Optics compressor/filter → Diet micro-granular → Sacrificial Armor velocity-triggered grain capture). Follows XOcelot's proven dual-target architecture: DSP in inline `.h` headers, standalone processor wrapper, thin SynthEngine adapter for XOmnibus.

**Tech Stack:** C++17, JUCE 8.0.12, CMake + Ninja, macOS (AU + Standalone)

**Spec:** `Docs/concepts/xowlfish_concept_brief.md`

**Pattern Reference:** `~/Documents/GitHub/XOcelot/` (same file structure, same build system, same adapter pattern)

---

## File Structure

### Standalone Project: `~/Documents/GitHub/XOwlfish/`

```
XOwlfish/
├── CMakeLists.txt                    # JUCE 8 build config
├── CLAUDE.md                         # Project guide
├── Presets/
│   ├── init.xometa                   # Default init preset
│   └── [15 factory presets].xometa
└── src/
    ├── Parameters.h                  # 50 param IDs + layout factory
    ├── PluginProcessor.h             # Standalone audio processor
    ├── PluginProcessor.cpp           # processBlock + MIDI handling
    ├── PluginEditor.h                # UI skeleton (minimal)
    ├── PluginEditor.cpp              # Stub
    ├── engine/
    │   ├── OwlfishParamSnapshot.h    # Read-once-per-block param cache
    │   ├── OwlfishVoice.h            # Monophonic voice (5 organs wired)
    │   └── AmpEnvelope.h             # ADSR envelope
    ├── dsp/
    │   ├── SubharmonicOsc.h          # Mixtur-Trautonium inspired subharmonic generator
    │   ├── OwlfishCompressor.h       # Feed-forward compressor with auto makeup
    │   ├── MicroGranular.h           # Ultra-short grain engine (2-10ms)
    │   ├── ArmorBuffer.h             # Velocity-triggered grain capture + delay
    │   └── AbyssReverb.h             # Dark algorithmic reverb (heavy LP damping)
    ├── adapter/
    │   ├── SynthEngine.h             # XOmnibus interface (copied from XOcelot)
    │   └── OwlfishEngine.h           # SynthEngine adapter
    └── preset/
        └── PresetManager.h           # .xometa loader (copied from XOcelot)
```

### XOmnibus Integration: `Source/Engines/Owlfish/`

All headers from `XOwlfish/src/` copied flat + include paths adjusted + `OwlfishEngine.cpp` registration stub.

---

## Modified Files (XOmnibus repo)

| File | Change |
|------|--------|
| `Source/Engines/Owlfish/` | New directory — all XOwlfish headers + registration .cpp |
| `CMakeLists.txt` | Add Owlfish source files to `target_sources` |
| `CLAUDE.md` | Add OWLFISH to engine table + parameter prefix table |

---

## Chunk 1: Project Scaffold + Parameters

### Task 1: Create XOwlfish standalone project scaffold

**Files:**
- Create: `~/Documents/GitHub/XOwlfish/CMakeLists.txt`
- Create: `~/Documents/GitHub/XOwlfish/CLAUDE.md`
- Create: `~/Documents/GitHub/XOwlfish/src/PluginEditor.h`
- Create: `~/Documents/GitHub/XOwlfish/src/PluginEditor.cpp`

- [ ] **Step 1: Create project directory and git repo**

```bash
mkdir -p ~/Documents/GitHub/XOwlfish/src/{engine,dsp,adapter,preset}
mkdir -p ~/Documents/GitHub/XOwlfish/Presets
cd ~/Documents/GitHub/XOwlfish && git init
```

- [ ] **Step 2: Write CMakeLists.txt**

Follow XOcelot pattern exactly. Key differences:
- `project(XOwlfish ...)`
- `PLUGIN_CODE XOwl`
- `BUNDLE_ID "com.xo-ox.xowlfish"`
- `PRODUCT_NAME "XOwlfish"`
- Source files: `src/PluginProcessor.cpp src/PluginEditor.cpp`
- Include dirs: `src`
- Link: `juce::juce_audio_utils juce::juce_dsp`

- [ ] **Step 3: Write minimal CLAUDE.md**

Content: XOwlfish project guide referencing the concept brief. Parameter prefix `owl_`, accent color `#B8860B`, 50 parameters, monophonic.

- [ ] **Step 4: Write PluginEditor.h/.cpp skeleton**

Minimal editor — just a component with `setSize(400, 300)` and engine name label. Copy pattern from XOcelot's `PluginEditor.h/.cpp`.

- [ ] **Step 5: Commit scaffold**

```bash
git add -A && git commit -m "feat: scaffold XOwlfish standalone project"
```

### Task 2: Create Parameters.h with all 50 parameter IDs

**Files:**
- Create: `~/Documents/GitHub/XOwlfish/src/Parameters.h`

**Reference:** XOcelot's `src/Parameters.h` for the exact pattern.

- [ ] **Step 1: Write Parameters.h**

Two parts:

**Part A — ParamIDs namespace:**
```cpp
namespace xowlfish {
namespace ParamIDs {
    // Solitary Genus (3)
    constexpr const char* portamento    = "owl_portamento";
    constexpr const char* legatoMode    = "owl_legatoMode";
    constexpr const char* morphGlide    = "owl_morphGlide";

    // Abyss Habitat (14)
    constexpr const char* subMix        = "owl_subMix";
    constexpr const char* subDiv1       = "owl_subDiv1";
    constexpr const char* subDiv2       = "owl_subDiv2";
    constexpr const char* subDiv3       = "owl_subDiv3";
    constexpr const char* subDiv4       = "owl_subDiv4";
    constexpr const char* subLevel1     = "owl_subLevel1";
    constexpr const char* subLevel2     = "owl_subLevel2";
    constexpr const char* subLevel3     = "owl_subLevel3";
    constexpr const char* subLevel4     = "owl_subLevel4";
    constexpr const char* mixtur        = "owl_mixtur";
    constexpr const char* fundWave      = "owl_fundWave";
    constexpr const char* subWave       = "owl_subWave";
    constexpr const char* bodyFreq      = "owl_bodyFreq";
    constexpr const char* bodyLevel     = "owl_bodyLevel";

    // Owl Optics (7)
    constexpr const char* compRatio     = "owl_compRatio";
    constexpr const char* compThreshold = "owl_compThreshold";
    constexpr const char* compAttack    = "owl_compAttack";
    constexpr const char* compRelease   = "owl_compRelease";
    constexpr const char* filterCutoff  = "owl_filterCutoff";
    constexpr const char* filterReso    = "owl_filterReso";
    constexpr const char* filterTrack   = "owl_filterTrack";

    // Diet (5)
    constexpr const char* grainSize     = "owl_grainSize";
    constexpr const char* grainDensity  = "owl_grainDensity";
    constexpr const char* grainPitch    = "owl_grainPitch";
    constexpr const char* grainMix      = "owl_grainMix";
    constexpr const char* feedRate      = "owl_feedRate";

    // Sacrificial Armor (5)
    constexpr const char* armorThreshold = "owl_armorThreshold";
    constexpr const char* armorDecay     = "owl_armorDecay";
    constexpr const char* armorScatter   = "owl_armorScatter";
    constexpr const char* armorDuck      = "owl_armorDuck";
    constexpr const char* armorDelay     = "owl_armorDelay";

    // Abyss Reverb (4)
    constexpr const char* reverbSize     = "owl_reverbSize";
    constexpr const char* reverbDamp     = "owl_reverbDamp";
    constexpr const char* reverbPreDelay = "owl_reverbPreDelay";
    constexpr const char* reverbMix      = "owl_reverbMix";

    // Amp Envelope (4)
    constexpr const char* ampAttack      = "owl_ampAttack";
    constexpr const char* ampDecay       = "owl_ampDecay";
    constexpr const char* ampSustain     = "owl_ampSustain";
    constexpr const char* ampRelease     = "owl_ampRelease";

    // Macros (4)
    constexpr const char* depth          = "owl_depth";
    constexpr const char* feeding        = "owl_feeding";
    constexpr const char* defense        = "owl_defense";
    constexpr const char* pressure       = "owl_pressure";

    // Output (2)
    constexpr const char* outputLevel    = "owl_outputLevel";
    constexpr const char* outputPan      = "owl_outputPan";

    // Coupling (2)
    constexpr const char* couplingLevel  = "owl_couplingLevel";
    constexpr const char* couplingBus    = "owl_couplingBus";
}
}
```

**Part B — `createParameterLayout()` function:**
- Use `juce::AudioParameterFloat` for continuous params (0-1 normalized)
- Use `juce::AudioParameterChoice` for `subDiv1-4` (8 choices: off, 1:2, 1:3, 1:4, 1:5, 1:6, 1:7, 1:8)
- Use `juce::AudioParameterChoice` for `legatoMode` (2 choices: retrigger, legato)
- Use `juce::AudioParameterChoice` for `couplingBus` (4 choices: A, B, C, D)
- All float params use `juce::NormalisableRange<float>(0.0f, 1.0f)` unless noted
- `bodyFreq` uses range `(20.0f, 80.0f)` with skew
- `outputPan` uses range `(-1.0f, 1.0f)`

Provide Enums namespace with StringArrays for all choice params.

- [ ] **Step 2: Verify parameter count matches 50**

Count all `constexpr const char*` lines — must be exactly 50.

- [ ] **Step 3: Commit**

```bash
git add src/Parameters.h && git commit -m "feat: add 50 frozen owl_ parameter IDs"
```

### Task 3: Create OwlfishParamSnapshot.h

**Files:**
- Create: `~/Documents/GitHub/XOwlfish/src/engine/OwlfishParamSnapshot.h`

**Reference:** XOcelot's `src/engine/OcelotParamSnapshot.h` for the exact pattern.

- [ ] **Step 1: Write OwlfishParamSnapshot.h**

Struct with one member per parameter. `updateFrom(apvts)` method that reads all 50 params from `AudioProcessorValueTreeState` using `getRawParameterValue()`. Store raw `std::atomic<float>*` pointers, read them once per block into plain `float`/`int` members.

Pattern:
```cpp
struct OwlfishParamSnapshot {
    // All 50 members with sensible defaults
    float portamento = 0.0f;
    int   legatoMode = 1;  // legato by default
    float morphGlide = 0.5f;
    // ... all 50

    void attachTo(juce::AudioProcessorValueTreeState& apvts) {
        // Cache all std::atomic<float>* pointers
        portamentoPtr = apvts.getRawParameterValue(ParamIDs::portamento);
        // ... all 50
    }

    void updateFrom() {
        // Read all cached pointers once per block
        portamento = portamentoPtr->load();
        legatoMode = static_cast<int>(legatoModePtr->load());
        // ... all 50
    }

private:
    std::atomic<float>* portamentoPtr = nullptr;
    // ... all 50 pointers
};
```

- [ ] **Step 2: Commit**

```bash
git add src/engine/OwlfishParamSnapshot.h && git commit -m "feat: add OwlfishParamSnapshot — 50 param read-once cache"
```

---

## Chunk 2: DSP Primitives

### Task 4: Create SubharmonicOsc.h — the Mixtur-Trautonium oscillator

**Files:**
- Create: `~/Documents/GitHub/XOwlfish/src/dsp/SubharmonicOsc.h`

This is the novel DSP — the core of the engine. No existing XOmnibus DSP module does subharmonic generation.

- [ ] **Step 1: Write SubharmonicOsc.h**

Architecture:
- **Fundamental oscillator:** Phase accumulator generating sine or triangle at MIDI frequency
- **4 subharmonic generators:** Each is a phase accumulator stepping at `freq / N` where N is the selected division ratio (2, 3, 4, 5, 6, 7, or 8). When division is 0 (off), output is 0.
- **Waveform selection:** Fundamental uses sine↔triangle blend (`fundWave` param). Subharmonics use triangle↔saw blend (`subWave` param).
- **Mixtur waveshaping:** At low `mixtur`, subharmonics layer cleanly. At high `mixtur`, the summed subharmonics pass through `tanh()` soft-clipping for inter-modulation growl.
- **Sub-bass body:** Separate sine oscillator at fixed `bodyFreq` Hz (not MIDI-tracked), mixed in at `bodyLevel`.
- **Output:** `processSample(float midiFreq, const OwlfishParamSnapshot& snap)` returns mono float.
- **Denormal protection:** `flushDenormal()` on all phase accumulators and output.

Key implementation detail — phase accumulator:
```cpp
float phase = 0.0f;       // 0.0 to 1.0
float phaseInc = freq / sampleRate;

float tick() {
    float out = generateWaveform(phase, waveBlend);
    phase += phaseInc;
    if (phase >= 1.0f) phase -= 1.0f;
    return out;
}
```

For subharmonic N: `phaseInc = (freq / N) / sampleRate`.

- [ ] **Step 2: Build validation**

```bash
cmake -B build -G Ninja && cmake --build build 2>&1 | head -20
```

Should compile (header-only, included transitively later).

- [ ] **Step 3: Commit**

```bash
git add src/dsp/SubharmonicOsc.h && git commit -m "feat: add SubharmonicOsc — Mixtur-Trautonium subharmonic generator"
```

### Task 5: Create OwlfishCompressor.h — extreme dynamics processor

**Files:**
- Create: `~/Documents/GitHub/XOwlfish/src/dsp/OwlfishCompressor.h`

- [ ] **Step 1: Write OwlfishCompressor.h**

Feed-forward compressor with envelope follower:
- **Detection:** RMS envelope follower with configurable attack/release
- **Gain computation:** `gainDB = (inputDB - threshold) * (1 - 1/ratio)` when input > threshold
- **Ratio range:** 1:1 to 100:1 (map `compRatio` 0-1 → 1.0 to 100.0; at 1.0 treat as limiter ∞:1)
- **Threshold:** Map `compThreshold` 0-1 → -60dB to 0dB
- **Attack/Release:** Map to time constants for envelope follower (ballistics)
- **Auto makeup gain:** `makeupDB = threshold * (1 - 1/ratio) * 0.5` — compensates for average gain reduction
- **Per-sample processing:** `processSample(float input)` returns compressed output
- **Denormal flush** on envelope state

- [ ] **Step 2: Commit**

```bash
git add src/dsp/OwlfishCompressor.h && git commit -m "feat: add OwlfishCompressor — extreme feed-forward compressor"
```

### Task 6: Create MicroGranular.h — ultra-short grain engine

**Files:**
- Create: `~/Documents/GitHub/XOwlfish/src/dsp/MicroGranular.h`

- [ ] **Step 1: Write MicroGranular.h**

Granular processor optimized for ultra-short grains (2-10ms):
- **Circular buffer:** 4096 samples (~93ms at 44.1k) for source capture
- **Grain scheduling:** LCG random trigger at `grainDensity` grains/sec
- **Grain playback:** Each grain reads from buffer with Hann window envelope
- **Pitch scatter:** Per-grain random pitch offset (0-12 semitones) via playback rate change
- **Feed rate:** Controls how fast the write pointer advances vs read pointer — the "appetite"
- **Max simultaneous grains:** 16 (each grain is 2-10ms, at 200/sec overlap is minimal)
- **Interface:** `writeSample(float input)` feeds the buffer; `readSample()` returns granulated output
- **Mix:** External (caller blends wet/dry via `grainMix` param)
- **Denormal flush** on all grain output accumulators

- [ ] **Step 2: Commit**

```bash
git add src/dsp/MicroGranular.h && git commit -m "feat: add MicroGranular — 2-10ms predatory grain engine"
```

### Task 7: Create ArmorBuffer.h — velocity-triggered grain capture + delay

**Files:**
- Create: `~/Documents/GitHub/XOwlfish/src/dsp/ArmorBuffer.h`

- [ ] **Step 1: Write ArmorBuffer.h**

Velocity-triggered signal destruction system:
- **Capture buffer:** 2048 samples (~46ms at 44.1k) — captures current audio on trigger
- **Trigger:** `trigger(float velocity, float threshold)` — activates when velocity > threshold
- **On trigger:**
  1. Copy last 2048 samples from input history into capture buffer
  2. Set `active = true`, reset decay timer
  3. Begin grain playback from captured buffer with random pitch scatter
- **Grain cloud:** 8 grains playing from capture buffer with random start positions + scatter
- **Delay line:** Captured grains feed through 22050-sample delay with feedback
- **Ducking:** While active, output a `duckAmount` (0-1) that the caller uses to attenuate main signal
- **Decay:** `armorDecay` controls exponential fade of grain cloud amplitude. When amplitude < 0.001, set `active = false`
- **Interface:**
  - `feedInput(float sample)` — continuously writes to input history ring buffer
  - `trigger(float velocity, float threshold)` — checks and activates
  - `processSample()` — returns armor grain cloud output + duck amount
  - `isActive()` — query state
- **Denormal flush** on delay line and decay envelope

- [ ] **Step 2: Commit**

```bash
git add src/dsp/ArmorBuffer.h && git commit -m "feat: add ArmorBuffer — velocity-triggered sacrificial grain capture"
```

### Task 8: Create AbyssReverb.h — dark algorithmic reverb

**Files:**
- Create: `~/Documents/GitHub/XOwlfish/src/dsp/AbyssReverb.h`

- [ ] **Step 1: Write AbyssReverb.h**

Dark algorithmic reverb tuned for deep-sea sound:
- **Algorithm:** Feedback delay network (FDN) with 4 delay lines at prime-ratio lengths
- **Pre-delay:** 20-200ms (mapped from `reverbPreDelay` 0-1)
- **Decay:** Map `reverbSize` 0-1 → 0.5s to 30s (very long tails)
- **Damping:** Heavy LP filter in feedback path. Map `reverbDamp` 0-1 → 20kHz to 800Hz cutoff (at 1.0, only sub-bass survives the reverb tail)
- **LP filter:** One-pole lowpass in each delay line feedback: `y = y + coeff * (x - y)`
- **Mix:** External (caller blends via `reverbMix` param)
- **Interface:** `processSample(float inputL, float inputR, float& outL, float& outR)`
- **Denormal flush** on all delay line and filter state

Can adapt the FDN pattern from `Source/DSP/Effects/LushReverb.h` in XOmnibus if it exists, but tune all defaults dark (low damping cutoff, long decay, long pre-delay).

- [ ] **Step 2: Commit**

```bash
git add src/dsp/AbyssReverb.h && git commit -m "feat: add AbyssReverb — dark FDN reverb for bathypelagic depths"
```

---

## Chunk 3: Voice + Signal Flow

### Task 9: Create AmpEnvelope.h

**Files:**
- Create: `~/Documents/GitHub/XOwlfish/src/engine/AmpEnvelope.h`

- [ ] **Step 1: Write AmpEnvelope.h**

Copy the ADSR envelope pattern from XOcelot's `AmpEnvelope.h`:
- States: Idle, Attack, Decay, Sustain, Release
- `noteOn()` triggers attack (or re-triggers depending on legato mode)
- `noteOff()` triggers release
- `processSample()` returns current envelope level (0-1)
- Time parameters mapped from 0-1 normalized to actual seconds
- Denormal flush on envelope state

- [ ] **Step 2: Commit**

```bash
git add src/engine/AmpEnvelope.h && git commit -m "feat: add AmpEnvelope — ADSR for monophonic voice"
```

### Task 10: Create OwlfishVoice.h — the organism

**Files:**
- Create: `~/Documents/GitHub/XOwlfish/src/engine/OwlfishVoice.h`

This is the core file — wires all 5 organs into a single monophonic voice.

- [ ] **Step 1: Write OwlfishVoice.h**

**Class structure:**
```cpp
class OwlfishVoice {
public:
    void prepare(double sampleRate);
    void noteOn(int note, float velocity, const OwlfishParamSnapshot& snap);
    void noteOff();
    bool isActive() const;
    void process(float* outL, float* outR, int numSamples,
                 const OwlfishParamSnapshot& snap);
    float getLastAmplitude() const;

private:
    // SOLITARY GENUS state
    float currentFreq = 440.0f;
    float targetFreq  = 440.0f;
    float portaCoeff  = 1.0f;  // exponential glide coefficient
    bool  legato      = true;
    int   currentNote = -1;
    float lastVelocity = 0.0f;

    // Organ morphing state (for morphGlide)
    float currentFilterCutoff = 1.0f;
    float targetFilterCutoff  = 1.0f;
    float currentGrainDensity = 0.0f;
    float targetGrainDensity  = 0.0f;
    // ... more morph targets as needed

    // DSP modules (the organs)
    SubharmonicOsc  abyssOsc;          // ABYSS HABITAT
    OwlfishCompressor optics;          // OWL OPTICS
    CytomicSVF      opticsFilter;      // OWL OPTICS (filter)
    MicroGranular   diet;              // DIET
    ArmorBuffer     armor;             // SACRIFICIAL ARMOR
    AbyssReverb     reverb;            // ABYSS REVERB
    AmpEnvelope     ampEnv;            // Amp envelope

    double sampleRate = 44100.0;
};
```

**Signal flow in `process()`:**
```
for each sample:
    1. SOLITARY GENUS: Update portamento (exponential glide toward targetFreq)
       - If morphGlide > 0: interpolate filter/grain/body params too
    2. ABYSS HABITAT: osc = abyssOsc.processSample(currentFreq, snap)
    3. OWL OPTICS: compressed = optics.processSample(osc)
                    filtered = opticsFilter.processSample(compressed)
    4. DIET: diet.writeSample(filtered)
             grained = diet.readSample()
             mixed = filtered * (1 - grainMix) + grained * grainMix
    5. SACRIFICIAL ARMOR: armor.feedInput(mixed)
                          armor.trigger(lastVelocity, armorThreshold)
                          armorOut = armor.processSample()
                          duckAmount = armor.getDuckAmount()
                          mixed *= (1.0f - duckAmount * armorDuck)
                          mixed += armorOut
    6. AMP ENVELOPE: mixed *= ampEnv.processSample()
    7. ABYSS REVERB: reverb.processSample(mixed, mixed, &revL, &revR)
                      outL = mixed * (1-reverbMix) + revL * reverbMix
                      outR = mixed * (1-reverbMix) + revR * reverbMix
    8. OUTPUT: apply outputLevel and outputPan
```

**Portamento:**
```cpp
// Exponential glide: currentFreq approaches targetFreq
float glideSpeed = std::exp(-1.0f / (portamentoMs * 0.001f * sampleRate));
currentFreq = targetFreq + (currentFreq - targetFreq) * glideSpeed;
```

**Legato handling:**
- `noteOn` with `legatoMode == 1` and voice already active: only change `targetFreq`, don't retrigger envelope
- `noteOn` with `legatoMode == 0` or voice idle: set `targetFreq` AND retrigger envelope

**CytomicSVF:** Use from XOmnibus shared DSP (`Source/DSP/CytomicSVF.h`). Copy it into `src/dsp/CytomicSVF.h`. If unavailable, implement inline — 2nd order SVF with LP/BP/HP modes, resonance, and key tracking.

- [ ] **Step 2: Build validation**

```bash
cmake -B build -G Ninja && cmake --build build 2>&1 | tail -5
```

Expected: May fail because PluginProcessor doesn't exist yet. That's OK — we're validating header syntax.

- [ ] **Step 3: Commit**

```bash
git add src/engine/OwlfishVoice.h && git commit -m "feat: add OwlfishVoice — monophonic organism with 5 wired organs"
```

---

## Chunk 4: Standalone Integration

### Task 11: Create PluginProcessor.h/.cpp

**Files:**
- Create: `~/Documents/GitHub/XOwlfish/src/PluginProcessor.h`
- Create: `~/Documents/GitHub/XOwlfish/src/PluginProcessor.cpp`

**Reference:** XOcelot's `src/PluginProcessor.h` and `src/PluginProcessor.cpp` for the exact pattern.

- [ ] **Step 1: Write PluginProcessor.h**

```cpp
class XOwlfishProcessor : public juce::AudioProcessor {
public:
    XOwlfishProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // ... standard AudioProcessor overrides (getName, channels, etc.)

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    xowlfish::OwlfishParamSnapshot paramSnapshot;
    xowlfish::OwlfishVoice voice;
    juce::MidiKeyboardState keyboardState;

    // Coupling cache for XOmnibus integration
    std::vector<float> outputCacheL, outputCacheR;
};
```

- [ ] **Step 2: Write PluginProcessor.cpp**

`processBlock()` follows the pattern:
```cpp
void XOwlfishProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    keyboardState.processNextMidiBuffer(midi, 0, buffer.getNumSamples(), true);
    paramSnapshot.updateFrom();

    // Process MIDI
    for (auto metadata : midi) {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
            voice.noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), paramSnapshot);
        else if (msg.isNoteOff())
            voice.noteOff();
        else if (msg.isAllNotesOff())
            voice.noteOff();
    }

    // Render voice
    auto* outL = buffer.getWritePointer(0);
    auto* outR = buffer.getWritePointer(1);
    voice.process(outL, outR, buffer.getNumSamples(), paramSnapshot);

    // Cache for coupling
    outputCacheL.resize(buffer.getNumSamples());
    outputCacheR.resize(buffer.getNumSamples());
    std::copy(outL, outL + buffer.getNumSamples(), outputCacheL.begin());
    std::copy(outR, outR + buffer.getNumSamples(), outputCacheR.begin());
}
```

Constructor initializes APVTS with `xowlfish::createParameterLayout()` and calls `paramSnapshot.attachTo(apvts)`.

State save/load: serialize APVTS to XML (standard JUCE pattern).

- [ ] **Step 3: Build and verify clean compile**

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build 2>&1 | tail -10
```

Expected: `ninja: no work to do` or clean build with 0 errors.

- [ ] **Step 4: Commit**

```bash
git add src/PluginProcessor.h src/PluginProcessor.cpp && git commit -m "feat: add XOwlfish standalone processor — monophonic subharmonic synth"
```

### Task 12: Build validation + init preset

**Files:**
- Create: `~/Documents/GitHub/XOwlfish/Presets/init.xometa`

- [ ] **Step 1: Verify build is clean**

```bash
cd ~/Documents/GitHub/XOwlfish && cmake -B build -G Ninja && cmake --build build 2>&1
```

Fix any compile errors. Common issues:
- Missing includes (ensure all `.h` files include their dependencies)
- Missing `flushDenormal()` — add inline helper if not available
- Namespace issues — ensure `xowlfish::` is consistent

- [ ] **Step 2: Write init.xometa preset**

Default init preset with all parameters at sensible starting values:
```json
{
  "schema_version": 1,
  "name": "Init",
  "mood": "Foundation",
  "engines": ["XOwlfish"],
  "author": "XO_OX",
  "version": "1.0.0",
  "description": "Default initialization preset",
  "tags": ["init", "default"],
  "macroLabels": ["DEPTH", "FEEDING", "DEFENSE", "PRESSURE"],
  "couplingIntensity": "None",
  "parameters": {
    "XOwlfish": {
      "owl_portamento": 0.2,
      "owl_legatoMode": 1,
      "owl_morphGlide": 0.5,
      "owl_subMix": 0.5,
      "owl_subDiv1": 1,
      "owl_subDiv2": 2,
      "owl_subDiv3": 0,
      "owl_subDiv4": 0,
      "owl_subLevel1": 0.7,
      "owl_subLevel2": 0.4,
      "owl_subLevel3": 0.0,
      "owl_subLevel4": 0.0,
      "owl_mixtur": 0.3,
      "owl_fundWave": 0.2,
      "owl_subWave": 0.3,
      "owl_bodyFreq": 40,
      "owl_bodyLevel": 0.3,
      "owl_compRatio": 0.4,
      "owl_compThreshold": 0.5,
      "owl_compAttack": 0.3,
      "owl_compRelease": 0.4,
      "owl_filterCutoff": 0.6,
      "owl_filterReso": 0.2,
      "owl_filterTrack": 0.5,
      "owl_grainSize": 0.3,
      "owl_grainDensity": 0.0,
      "owl_grainPitch": 0.1,
      "owl_grainMix": 0.0,
      "owl_feedRate": 0.5,
      "owl_armorThreshold": 0.7,
      "owl_armorDecay": 0.4,
      "owl_armorScatter": 0.3,
      "owl_armorDuck": 0.5,
      "owl_armorDelay": 0.3,
      "owl_reverbSize": 0.5,
      "owl_reverbDamp": 0.6,
      "owl_reverbPreDelay": 0.3,
      "owl_reverbMix": 0.3,
      "owl_ampAttack": 0.1,
      "owl_ampDecay": 0.3,
      "owl_ampSustain": 0.8,
      "owl_ampRelease": 0.4,
      "owl_depth": 0.5,
      "owl_feeding": 0.0,
      "owl_defense": 0.3,
      "owl_pressure": 0.4,
      "owl_outputLevel": 0.8,
      "owl_outputPan": 0.0,
      "owl_couplingLevel": 0.5,
      "owl_couplingBus": 0
    }
  },
  "coupling": { "pairs": [] }
}
```

- [ ] **Step 3: Commit**

```bash
git add Presets/init.xometa && git commit -m "feat: add init preset with sensible defaults"
```

---

## Chunk 5: XOmnibus Adapter + Integration

### Task 13: Create OwlfishEngine.h — SynthEngine adapter

**Files:**
- Create: `~/Documents/GitHub/XOwlfish/src/adapter/OwlfishEngine.h`
- Copy: `~/Documents/GitHub/XOwlfish/src/adapter/SynthEngine.h` (from XOcelot or XOmnibus)

**Reference:** XOcelot's `src/adapter/OcelotEngine.h` for the exact pattern.

- [ ] **Step 1: Copy SynthEngine.h from XOcelot**

```bash
cp ~/Documents/GitHub/XOcelot/src/adapter/SynthEngine.h \
   ~/Documents/GitHub/XOwlfish/src/adapter/SynthEngine.h
```

- [ ] **Step 2: Write OwlfishEngine.h**

Implements `xomnibus::SynthEngine` interface. Key differences from OcelotEngine:
- `getEngineId()` returns `"Owlfish"`
- `getAccentColour()` returns `juce::Colour(0xFFB8860B)` (Abyssal Gold)
- `getMaxVoices()` returns `1`
- `getActiveVoiceCount()` returns `voice.isActive() ? 1 : 0`
- `renderBlock()` wraps the single `OwlfishVoice`
- `getSampleForCoupling()` uses `outputCacheL/R` vectors (per-sample coupling)
- `applyCouplingInput()` handles 7 coupling types: AudioToWavetable, AudioToFM, AmpToFilter, RhythmToBlend, EnvToDecay, EnvToMorph, LFOToPitch
- Guard with `#ifdef XOMNIBUS_INTEGRATION` (will be removed in XOmnibus copy)
- Use `xowlfish` namespace, with `using xowlfish::OwlfishEngine;` before REGISTER_ENGINE

- [ ] **Step 3: Build validation**

```bash
cd ~/Documents/GitHub/XOwlfish && cmake --build build 2>&1 | tail -5
```

- [ ] **Step 4: Commit**

```bash
git add src/adapter/ && git commit -m "feat: add OwlfishEngine adapter — SynthEngine interface for XOmnibus"
```

### Task 14: Register OWLFISH in XOmnibus

**Files:**
- Create: `~/Documents/GitHub/XO_OX-XOmnibus/Source/Engines/Owlfish/` (all headers copied flat)
- Create: `~/Documents/GitHub/XO_OX-XOmnibus/Source/Engines/Owlfish/OwlfishEngine.cpp`
- Modify: `~/Documents/GitHub/XO_OX-XOmnibus/CMakeLists.txt`

**Reference:** How OCELOT was integrated (Task 10 from previous session).

- [ ] **Step 1: Copy all XOwlfish headers to XOmnibus**

```bash
mkdir -p ~/Documents/GitHub/XO_OX-XOmnibus/Source/Engines/Owlfish/
# Copy all .h files from XOwlfish/src/ into flat directory
# Flatten include paths (../dsp/X.h → X.h, ../engine/X.h → X.h)
```

Files to copy (13 headers):
- `Parameters.h`
- `OwlfishParamSnapshot.h`
- `OwlfishVoice.h`
- `AmpEnvelope.h`
- `SubharmonicOsc.h`
- `OwlfishCompressor.h`
- `MicroGranular.h`
- `ArmorBuffer.h`
- `AbyssReverb.h`
- `CytomicSVF.h` (if separate copy needed)
- `OwlfishEngine.h` (remove `#ifdef XOMNIBUS_INTEGRATION` guard)
- `SynthEngine.h` (interface reference)
- `PresetManager.h` (if used)

- [ ] **Step 2: Create OwlfishEngine.cpp registration stub**

```cpp
#include "OwlfishEngine.h"
using xowlfish::OwlfishEngine;
REGISTER_ENGINE(OwlfishEngine)
```

- [ ] **Step 3: Add to CMakeLists.txt**

Add all Owlfish source files to `target_sources()`.

- [ ] **Step 4: Verify OcelotEngine.cpp compiles clean**

```bash
cd ~/Documents/GitHub/XO_OX-XOmnibus
cmake -B build -G Ninja && cmake --build build -- Source/Engines/Owlfish/OwlfishEngine.cpp 2>&1
```

Note: Full XOmnibus build has pre-existing errors (not caused by OWLFISH). Verify OwlfishEngine.cpp itself compiles.

- [ ] **Step 5: Update CLAUDE.md engine tables**

Add OWLFISH row to:
- Engine modules list (line ~9)
- Engine table (with Abyssal Gold `#B8860B`)
- Engine ID vs Parameter Prefix table (Owlfish → `owl_` → `owl_filterCutoff`)

- [ ] **Step 6: Commit**

```bash
git add Source/Engines/Owlfish/ CMakeLists.txt CLAUDE.md
git commit -m "feat(engines): add OWLFISH — monophonic subharmonic organism synth (engine #22)"
```

### Task 15: Author 15 factory presets

**Files:**
- Create: 15 `.xometa` files in `~/Documents/GitHub/XOwlfish/Presets/`
- Copy: To `~/Documents/GitHub/XO_OX-XOmnibus/Presets/XOmnibus/{mood}/`

**Reference:** XOwlfish concept brief preset strategy section.

- [ ] **Step 1: Author 15 presets**

Distribute across the 7 preset categories from the concept brief:

| # | Name | Category | Mood | Key Character |
|---|------|----------|------|--------------|
| 1 | Bathyal Floor | Abyssal Bass | Foundation | Pure subharmonic bass, 1/2 + 1/3 Mixtur, heavy sub-body |
| 2 | Trench Weight | Abyssal Bass | Foundation | Extreme compression + closed filter, crushing depth |
| 3 | Mixtur Growl | Abyssal Bass | Foundation | High Mixtur waveshaping, aggressive sub-bass |
| 4 | Mesopelagic Hum | Pressure Drones | Atmosphere | Long drone, all 4 subharmonics, massive reverb |
| 5 | Midnight Zone | Pressure Drones | Aether | Pure subharmonic drone, extreme damping, infinite decay |
| 6 | Copepod Swarm | Predator Textures | Atmosphere | High grain density, active feeding, crawling texture |
| 7 | Feeding Frenzy | Predator Textures | Flux | Maximum Diet + fast feed rate + Mixtur growl |
| 8 | Photon Gather | Photon Catcher | Prism | Extreme compression, open resonant filter, crystalline |
| 9 | Deep Light | Photon Catcher | Prism | High comp + filter resonance near self-osc, bright from dark |
| 10 | Scale Shed | Armor Shatter | Foundation | Low armor threshold, dramatic ducking, explosive fragments |
| 11 | Fragment Storm | Armor Shatter | Flux | Fast armor decay + high scatter + delay feedback |
| 12 | Deep Migration | Organism Glides | Atmosphere | Long portamento, full morphGlide, legato bass movement |
| 13 | Solitary Path | Organism Glides | Aether | Extreme portamento + reverb, the creature alone in the deep |
| 14 | Harmonic Predator | Coupling Showcase | Entangled | ORBITAL x OWLFISH — partials consumed as prey |
| 15 | Ecosystem Hunter | Coupling Showcase | Entangled | OCELOT x OWLFISH — ecosystem output as feeding ground |

Each preset must:
- Use all parameters (no missing keys)
- Have macros that produce audible change at every range point
- Follow `.xometa` schema from init.xometa pattern
- Use evocative naming (2-3 words, max 30 chars)

- [ ] **Step 2: Copy presets to XOmnibus mood folders**

```bash
# Foundation presets → Presets/XOmnibus/Foundation/
# Atmosphere → Atmosphere/
# Aether → Aether/
# Prism → Prism/
# Flux → Flux/
# Entangled → Entangled/
```

- [ ] **Step 3: Commit in both repos**

XOwlfish repo:
```bash
cd ~/Documents/GitHub/XOwlfish
git add Presets/ && git commit -m "feat(presets): add 15 factory presets across 7 categories"
```

XOmnibus repo:
```bash
cd ~/Documents/GitHub/XO_OX-XOmnibus
git add Presets/ && git commit -m "feat(presets): add 15 OWLFISH factory presets to mood folders"
```

---

## Verification Checklist

After all tasks complete:

- [ ] XOwlfish standalone builds clean (`ninja: no work to do`)
- [ ] All 50 parameters registered in Parameters.h
- [ ] ParamSnapshot reads all 50 params
- [ ] OwlfishVoice produces audio (not silence)
- [ ] Monophonic behavior: new note replaces old note
- [ ] Legato: connected notes glide without envelope retrigger
- [ ] Portamento: pitch glides smoothly
- [ ] Subharmonic oscillator produces audible sub-frequencies
- [ ] Compressor with extreme ratio reveals quiet harmonics
- [ ] MicroGranular produces texture at high density
- [ ] ArmorBuffer triggers on high velocity and ducks main signal
- [ ] AbyssReverb produces dark, long tails
- [ ] OwlfishEngine.cpp compiles in XOmnibus
- [ ] 15 presets authored and distributed to mood folders
- [ ] CLAUDE.md updated with OWLFISH engine entry
