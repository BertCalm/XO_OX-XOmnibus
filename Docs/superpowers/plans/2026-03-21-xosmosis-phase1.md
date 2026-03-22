# XOsmosis Phase 1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add external audio input to XOmnibus via a new membrane engine (XOsmosis) and two Membrane Collection FX modules (fXFormant, fXBreath), enabling voice/guitar/instrument processing through the fleet.

**Architecture:** XOmnibusProcessor gains a stereo input bus. OsmosisEngine captures the input into a ring buffer and produces coupling signals (envelope, pitch, spectral). fXFormant and fXBreath are added to MasterFXChain as dual-purpose FX (work on both external audio and synth output). External audio routes through the existing 25+ stage MasterFX chain (Option C — no engine-specific FX).

**Tech Stack:** JUCE 8, C++17, inline DSP in `.h` headers, CytomicSVF, StandardLFO, AudioRingBuffer, IAudioBufferSink

**Key Reference Files:**
- `Source/Core/SynthEngine.h` — engine interface (14 CouplingTypes, SilenceGate)
- `Source/XOmnibusProcessor.cpp` — registration (line ~81), processBlock (line ~1101), getBusesProperties (line ~401)
- `Source/Core/MegaCouplingMatrix.h` — coupling routing, processAudioRoute uses OpalEngine hard-cast
- `Source/Core/MasterFXChain.h` — 22 stages, processBlock signature
- `Source/DSP/Effects/fXOnslaught.h` — FX module pattern (prepare/setters/processBlock)
- `Source/Core/IAudioBufferSink.h` — audio buffer interface (for Phase 2)
- `Source/Core/AudioRingBuffer.h` — ring buffer with freeze support

---

### Task 1: Add Stereo Input Bus to XOmnibusProcessor

**Files:**
- Modify: `Source/XOmnibusProcessor.h`
- Modify: `Source/XOmnibusProcessor.cpp`

This is the plumbing change that opens XOmnibus to external audio. The input bus is optional (instruments can still use XOmnibus as MIDI-only).

- [ ] **Step 1: Add input bus to BusesProperties**

In `XOmnibusProcessor.cpp`, find the constructor's `BusesProperties()` call and add an optional stereo input:

```cpp
AudioProcessor(BusesProperties()
    .withInput("Input", juce::AudioChannelSet::stereo(), false)  // optional input
    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
```

The `false` makes the input bus disabled by default — DAW users opt in by enabling the sidechain/input.

- [ ] **Step 1b: Override isBusesLayoutSupported (P0-B fix — required for AU validation)**

In `XOmnibusProcessor.h`, add to the public section:

```cpp
bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
```

In `XOmnibusProcessor.cpp`, implement:

```cpp
bool XOmnibusProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Output must be stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Input can be disabled (no channels) or stereo
    const auto& inputSet = layouts.getMainInputChannelSet();
    if (inputSet != juce::AudioChannelSet::disabled()
        && inputSet != juce::AudioChannelSet::stereo())
        return false;

    return true;
}
```

This allows AU/AUv3 hosts to enable or disable the input bus dynamically.

- [ ] **Step 2: Add external audio buffer member to XOmnibusProcessor.h**

```cpp
// External audio input capture — sized once in prepareToPlay, NEVER resized in processBlock.
// OsmosisEngine reads raw pointers into this buffer within the same processBlock call.
juce::AudioBuffer<float> externalInputBuffer;
```

Add this in the private section near the `engineBuffers` array.

- [ ] **Step 3: Allocate externalInputBuffer in prepareToPlay (P0-A fix — no allocation on audio thread)**

In `prepareToPlay()`, add:

```cpp
externalInputBuffer.setSize(2, samplesPerBlock);
```

- [ ] **Step 4: Capture external audio in processBlock before buffer.clear()**

In `XOmnibusProcessor.cpp` `processBlock()`, BEFORE the existing `buffer.clear()` call, add:

```cpp
// Capture external audio input before clearing the buffer.
// externalInputBuffer was pre-allocated in prepareToPlay — no setSize here (P0-A).
// ORDERING CONTRACT: setExternalInput() on Osmosis must be called AFTER this
// and BEFORE renderBlock() in the same processBlock call. The pointers are only
// valid within this block (P0-C).
const int totalInputChannels = getTotalNumInputChannels();
if (totalInputChannels >= 2)
{
    for (int ch = 0; ch < 2; ++ch)
        externalInputBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
}
else
{
    externalInputBuffer.clear();
}
```

- [ ] **Step 5: Build and verify**

Run: `cmake --build build` (or your build command)
Expected: Clean compile. No audio changes yet — input is captured but not routed anywhere.

- [ ] **Step 6: Commit**

```bash
git add Source/XOmnibusProcessor.h Source/XOmnibusProcessor.cpp
git commit -m "feat: add optional stereo input bus to XOmnibusProcessor

Captures external audio before buffer.clear() for Osmosis engine routing.
Input bus disabled by default — DAW users opt in."
```

---

### Task 2: Build OsmosisEngine — Minimal Membrane

**Files:**
- Create: `Source/Engines/Osmosis/OsmosisEngine.h`

OsmosisEngine is the membrane. It receives external audio from the processor and produces coupling signals. Phase 1 is minimal: envelope follower, pitch tracker (zero-crossing), spectral RMS in 4 bands. No Membrane State Machine yet (Phase 2).

- [ ] **Step 1: Create the engine header**

Create `Source/Engines/Osmosis/OsmosisEngine.h`:

```cpp
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/FilterEnvelope.h"
#include "../../DSP/ParameterSmoother.h"
#include <cmath>
#include <array>

namespace xomnibus {

//==============================================================================
// OsmosisEngine — the membrane between XOmnibus and the outside world.
//
// Receives external audio via setExternalInput(). Analyzes envelope, pitch,
// and spectral content. Produces coupling signals that let other engines
// respond to the outside world.
//
// Does not generate its own sound — like Optic (B005), its primary output
// is coupling data, not audio. Wet/dry blend passes external audio through
// the membrane with subtle coloring (one-pole filter at membrane frequency).
//
// Macros: PERMEABILITY (wet/dry), SELECTIVITY (filter Q), REACTIVITY (env speed), MEMORY (decay)
// Param prefix: osmo_
// Accent: Surface Tension Silver #C0C0C0
//
class OsmosisEngine : public SynthEngine
{
public:
    OsmosisEngine() = default;

    //-- Identity --------------------------------------------------------------
    juce::String getEngineId() const override { return "Osmosis"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFC0C0C0); }
    int getMaxVoices() const override { return 1; } // mono analysis engine
    bool isAnalysisEngine() const override { return true; } // enables external audio routing without RTTI

    //-- Lifecycle -------------------------------------------------------------
    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr_ = sampleRate;
        blockSize_ = maxBlockSize;

        // Envelope follower — one-pole with variable attack/release
        envFollowerL_ = 0.0f;
        envFollowerR_ = 0.0f;

        // Pitch detection — zero-crossing rate
        prevSampleL_ = 0.0f;
        zeroCrossingAccum_ = 0.0f;
        detectedPitch_ = 0.0f;

        // Spectral bands (simple RMS in 4 frequency ranges via biquad)
        for (int i = 0; i < 4; ++i)
            bandRMS_[i] = 0.0f;

        // Band-split filters (approximate: 200, 800, 3000 Hz crossovers)
        setupBandFilters();

        // Coupling output cache
        couplingSampleL_ = 0.0f;
        couplingSampleR_ = 0.0f;

        // Smoothers for macro parameters
        permeabilitySmoother_.prepare(sr_, 5.0f);
        selectivitySmoother_.prepare(sr_, 5.0f);
        reactivitySmoother_.prepare(sr_, 5.0f);
        memorySmoother_.prepare(sr_, 5.0f);

        // Membrane filter (one-pole LP for coloring pass-through audio)
        membraneLPCoeff_ = 0.0f;
        membraneLPStateL_ = 0.0f;
        membraneLPStateR_ = 0.0f;

        // LFO for subtle membrane modulation
        lfo_.setShape(StandardLFO::Sine);
        lfo_.setRate(0.5f, static_cast<float>(sr_));
        lfo_.reset();

        prepareSilenceGate(sr_, maxBlockSize, 500.0f);

        externalBufferL_ = nullptr;
        externalBufferR_ = nullptr;
        externalNumSamples_ = 0;
    }

    void releaseResources() override {}

    void reset() override
    {
        envFollowerL_ = 0.0f;
        envFollowerR_ = 0.0f;
        prevSampleL_ = 0.0f;
        zeroCrossingAccum_ = 0.0f;
        detectedPitch_ = 0.0f;
        for (int i = 0; i < 4; ++i)
            bandRMS_[i] = 0.0f;
        membraneLPStateL_ = 0.0f;
        membraneLPStateR_ = 0.0f;
        lfo_.reset();
    }

    //-- External audio injection (called by XOmnibusProcessor) ----------------
    void setExternalInput(const float* left, const float* right, int numSamples)
    {
        externalBufferL_ = left;
        externalBufferR_ = right;
        externalNumSamples_ = numSamples;
    }

    //-- Analysis getters (for coupling and UI) --------------------------------
    float getEnvelopeLevel() const { return (envFollowerL_ + envFollowerR_) * 0.5f; }
    float getDetectedPitch() const { return detectedPitch_; }
    const float* getBandRMS() const { return bandRMS_; }

    //-- Audio -----------------------------------------------------------------
    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& /*midi*/,
                     int numSamples) override
    {
        // SilenceGate: early-out when no external audio is present (P1-E fix)
        if (isSilenceGateBypassed())
        {
            buffer.clear();
            return;
        }

        // Read macro parameters
        const float permeability = permeabilitySmoother_.process(
            pPermeability_ ? pPermeability_->load() : 0.5f);
        const float selectivity = selectivitySmoother_.process(
            pSelectivity_ ? pSelectivity_->load() : 0.5f);
        const float reactivity = reactivitySmoother_.process(
            pReactivity_ ? pReactivity_->load() : 0.5f);
        const float memory = memorySmoother_.process(
            pMemory_ ? pMemory_->load() : 0.3f);

        // Envelope follower coefficients from reactivity
        const float attackMs = 1.0f + (1.0f - reactivity) * 49.0f;  // 1-50ms
        const float releaseMs = 10.0f + memory * 990.0f;             // 10-1000ms
        const float attackCoeff = std::exp(-1.0f / (static_cast<float>(sr_) * attackMs * 0.001f));
        const float releaseCoeff = std::exp(-1.0f / (static_cast<float>(sr_) * releaseMs * 0.001f));

        // Membrane filter frequency from selectivity
        const float membraneFreq = 200.0f + selectivity * 18000.0f;
        membraneLPCoeff_ = std::exp(-2.0f * 3.14159265f * membraneFreq / static_cast<float>(sr_));

        auto* outL = buffer.getWritePointer(0);
        auto* outR = buffer.getWritePointer(1);

        // Zero-crossing counter for pitch detection
        int crossings = 0;

        for (int i = 0; i < numSamples; ++i)
        {
            // Read external input (or silence if none)
            const float inL = (externalBufferL_ && i < externalNumSamples_) ? externalBufferL_[i] : 0.0f;
            const float inR = (externalBufferR_ && i < externalNumSamples_) ? externalBufferR_[i] : 0.0f;

            // Envelope follower
            const float absL = std::fabs(inL);
            const float absR = std::fabs(inR);
            envFollowerL_ = (absL > envFollowerL_)
                ? attackCoeff * envFollowerL_ + (1.0f - attackCoeff) * absL
                : releaseCoeff * envFollowerL_ + (1.0f - releaseCoeff) * absL;
            envFollowerR_ = (absR > envFollowerR_)
                ? attackCoeff * envFollowerR_ + (1.0f - attackCoeff) * absR
                : releaseCoeff * envFollowerR_ + (1.0f - releaseCoeff) * absR;

            // Zero-crossing detection (left channel)
            if ((inL >= 0.0f && prevSampleL_ < 0.0f) || (inL < 0.0f && prevSampleL_ >= 0.0f))
                ++crossings;
            prevSampleL_ = inL;

            // Membrane filter (one-pole LP for pass-through coloring)
            membraneLPStateL_ = membraneLPCoeff_ * membraneLPStateL_ + (1.0f - membraneLPCoeff_) * inL;
            membraneLPStateR_ = membraneLPCoeff_ * membraneLPStateR_ + (1.0f - membraneLPCoeff_) * inR;

            // Output: blend dry external with filtered (permeability = wet/dry)
            outL[i] = inL * (1.0f - permeability) + membraneLPStateL_ * permeability;
            outR[i] = inR * (1.0f - permeability) + membraneLPStateR_ * permeability;

            // Cache for coupling
            couplingSampleL_ = outL[i];
            couplingSampleR_ = outR[i];
        }

        // Pitch estimation from zero-crossings (rough but CPU-free)
        if (numSamples > 0)
        {
            zeroCrossingAccum_ = zeroCrossingAccum_ * 0.7f +
                static_cast<float>(crossings) * 0.3f;
            detectedPitch_ = (zeroCrossingAccum_ * static_cast<float>(sr_))
                / (2.0f * static_cast<float>(numSamples));
        }

        // Simple 4-band RMS (approximate via block-rate analysis)
        // LIMITATION (P1-C): Phase 1 uses total RMS distributed to all 4 bands equally.
        // All bandRMS_[0..3] values are identical. DO NOT wire coupling consumers to
        // individual bands until Phase 2 adds real CytomicSVF band-split.
        updateBandRMS(numSamples);

        // Wake silence gate if external audio present
        if (envFollowerL_ > 0.001f || envFollowerR_ > 0.001f)
            wakeSilenceGate();

        // LFO tick (for future membrane modulation)
        for (int i = 0; i < numSamples; ++i)
            lfo_.tick();

        analyzeForSilenceGate(buffer, numSamples);
    }

    //-- Coupling --------------------------------------------------------------
    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? couplingSampleL_ : couplingSampleR_;
    }

    void applyCouplingInput(CouplingType /*type*/, float /*amount*/,
                            const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        // Phase 1: Osmosis is a source, not a destination.
        // Phase 2 will add membrane perturbation from coupled engines.
    }

    //-- Parameters ------------------------------------------------------------
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osmo_permeability", 1}, "Permeability",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osmo_selectivity", 1}, "Selectivity",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osmo_reactivity", 1}, "Reactivity",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osmo_memory", 1}, "Memory",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Macro mapping (M1-M4)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osmo_macro1", 1}, "PERMEABILITY",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osmo_macro2", 1}, "SELECTIVITY",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osmo_macro3", 1}, "REACTIVITY",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osmo_macro4", 1}, "MEMORY",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        return layout;
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pPermeability_ = apvts.getRawParameterValue("osmo_permeability");
        pSelectivity_ = apvts.getRawParameterValue("osmo_selectivity");
        pReactivity_ = apvts.getRawParameterValue("osmo_reactivity");
        pMemory_ = apvts.getRawParameterValue("osmo_memory");
    }

private:
    double sr_ = 44100.0;
    int blockSize_ = 512;

    // External audio pointers (set per block by processor, NOT owned)
    const float* externalBufferL_ = nullptr;
    const float* externalBufferR_ = nullptr;
    int externalNumSamples_ = 0;

    // Envelope follower state
    float envFollowerL_ = 0.0f;
    float envFollowerR_ = 0.0f;

    // Pitch detection (zero-crossing)
    float prevSampleL_ = 0.0f;
    // zeroCrossingCount_ removed (P1-D: was declared but never incremented)
    float zeroCrossingAccum_ = 0.0f;
    float detectedPitch_ = 0.0f;

    // 4-band spectral RMS
    float bandRMS_[4] = {};

    // Membrane filter
    float membraneLPCoeff_ = 0.0f;
    float membraneLPStateL_ = 0.0f;
    float membraneLPStateR_ = 0.0f;

    // Coupling output cache
    float couplingSampleL_ = 0.0f;
    float couplingSampleR_ = 0.0f;

    // Parameter pointers
    std::atomic<float>* pPermeability_ = nullptr;
    std::atomic<float>* pSelectivity_ = nullptr;
    std::atomic<float>* pReactivity_ = nullptr;
    std::atomic<float>* pMemory_ = nullptr;

    // Smoothers
    ParameterSmoother permeabilitySmoother_;
    ParameterSmoother selectivitySmoother_;
    ParameterSmoother reactivitySmoother_;
    ParameterSmoother memorySmoother_;

    // LFO
    StandardLFO lfo_;

    void setupBandFilters()
    {
        // Phase 1: simple block-rate RMS, no per-sample filtering
        // Phase 2 will add proper band-split via CytomicSVF
    }

    void updateBandRMS(int numSamples)
    {
        if (!externalBufferL_ || externalNumSamples_ == 0) return;

        // Approximate 4-band energy via simple frequency-dependent RMS
        // Band 0: sub-bass (<200 Hz) — use envelope follower low-pass approximation
        // Band 1: low-mid (200-800 Hz)
        // Band 2: mid (800-3000 Hz)
        // Band 3: high (>3000 Hz)
        // Phase 1: total RMS only, distribute evenly. Phase 2 adds real band-split.
        float totalRMS = 0.0f;
        const int n = std::min(numSamples, externalNumSamples_);
        for (int i = 0; i < n; ++i)
        {
            const float s = externalBufferL_[i];
            totalRMS += s * s;
        }
        totalRMS = std::sqrt(totalRMS / static_cast<float>(std::max(n, 1)));

        // Smooth and distribute (placeholder — Phase 2 will use real band-split)
        for (int b = 0; b < 4; ++b)
            bandRMS_[b] = bandRMS_[b] * 0.8f + totalRMS * 0.2f;
    }
};

} // namespace xomnibus
```

- [ ] **Step 2: Register OsmosisEngine in XOmnibusProcessor.cpp**

Add with the other engine registrations (near line ~396):

```cpp
#include "Engines/Osmosis/OsmosisEngine.h"
static bool registered_Osmosis = EngineRegistry::instance().registerEngine(
    "Osmosis", []() { return std::make_unique<OsmosisEngine>(); });
```

Note: include path is relative to `Source/`, matching all other engine includes in this file.

- [ ] **Step 3: Route external audio to Osmosis in processBlock (P1-A fix — no dynamic_cast)**

After the engine slot loading loop, before `renderBlock` calls, add:

```cpp
// Feed external audio to Osmosis if loaded in any slot.
// Uses virtual isAnalysisEngine() instead of dynamic_cast to avoid RTTI on audio thread.
for (int slot = 0; slot < MaxSlots; ++slot)
{
    if (enginePtrs[slot] && enginePtrs[slot]->isAnalysisEngine())
    {
        static_cast<OsmosisEngine*>(enginePtrs[slot])->setExternalInput(
            externalInputBuffer.getReadPointer(0),
            externalInputBuffer.getReadPointer(1),
            numSamples);
    }
}
```

This requires adding to `SynthEngine.h`:
```cpp
// Override in analysis engines (Osmosis) to receive external audio without RTTI.
virtual bool isAnalysisEngine() const { return false; }
```

- [ ] **Step 4: Add to PresetManager validEngineNames and frozenPrefixForEngine**

In `Source/Core/PresetManager.h`, add `"Osmosis"` to `validEngineNames` and `{"Osmosis", "osmo_"}` to `frozenPrefixForEngine`.

- [ ] **Step 5: Build and verify**

Run: `cmake --build build`
Expected: Clean compile. Osmosis is now loadable as an engine. External audio flows through its membrane filter.

- [ ] **Step 6: Commit**

```bash
git add Source/Engines/Osmosis/OsmosisEngine.h Source/XOmnibusProcessor.cpp Source/Core/PresetManager.h
git commit -m "feat: add OsmosisEngine — external audio membrane (#47)

4 params (permeability/selectivity/reactivity/memory), osmo_ prefix.
Envelope follower, zero-crossing pitch detect, 4-band RMS.
Receives external audio via setExternalInput() from processor."
```

---

### Task 3: Build fXFormant — Formant Filter FX

**Files:**
- Create: `Source/DSP/Effects/fXFormant.h`
- Modify: `Source/Core/MasterFXChain.h` (add stage)

4-band parallel bandpass formant filter. On voice: shift vocal character. On synth: make it talk.

- [ ] **Step 1: Create fXFormant.h**

Create `Source/DSP/Effects/fXFormant.h` following the fXOnslaught pattern:

```cpp
#pragma once
#include <cmath>
#include <array>
#include "../CytomicSVF.h"
#include "../ParameterSmoother.h"

namespace xomnibus {

//==============================================================================
// fXFormant — Membrane Collection formant filter.
//
// 4 parallel bandpass filters at vocal formant frequencies (F1-F4).
// Each band has independent frequency and gain. Shift knob moves all
// formant frequencies up/down together (vocal character change).
// On voice: male→female, age, size shifts without pitch change.
// On synth: vowel sweeps, talk-box territory, "speaking" synths.
//
// CPU budget: ~30 ops/sample (4× CytomicSVF bandpass)
//
class fXFormant
{
public:
    fXFormant() = default;

    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr_ = static_cast<float>(sampleRate);
        for (int b = 0; b < kNumBands; ++b)
        {
            bpL_[b].setMode(CytomicSVF::Mode::BandPass);
            bpR_[b].setMode(CytomicSVF::Mode::BandPass);
            bpL_[b].reset();
            bpR_[b].reset();
        }
        shiftSmoother_.prepare(sampleRate, 5.0f);
        mixSmoother_.prepare(sampleRate, 5.0f);
        updateFormants(0.0f);
    }

    // Shift: -1.0 to +1.0 (shifts all formants down/up by up to an octave)
    void setShift(float shift) { shift_ = juce::jlimit(-1.0f, 1.0f, shift); }

    // Vowel: 0-4 selects A/E/I/O/U base formant table
    void setVowel(float vowel) { vowel_ = juce::jlimit(0.0f, 4.0f, vowel); }

    // Resonance: Q of the bandpass filters (0.5 - 20.0)
    void setResonance(float q) { resonance_ = juce::jlimit(0.5f, 20.0f, q); }

    void setMix(float m) { mix_ = juce::jlimit(0.0f, 1.0f, m); }

    void processBlock(float* left, float* right, int numSamples)
    {
        if (mix_ < 0.001f) return; // zero CPU at mix=0

        const float smoothMix = mixSmoother_.process(mix_);
        const float smoothShift = shiftSmoother_.process(shift_);

        updateFormants(smoothShift);

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = left[i];
            const float dryR = right[i];

            float wetL = 0.0f, wetR = 0.0f;
            for (int b = 0; b < kNumBands; ++b)
            {
                wetL += bpL_[b].processSample(dryL) * bandGain_[b];
                wetR += bpR_[b].processSample(dryR) * bandGain_[b];
            }

            left[i] = dryL + (wetL - dryL) * smoothMix;
            right[i] = dryR + (wetR - dryR) * smoothMix;
        }
    }

private:
    static constexpr int kNumBands = 4;
    float sr_ = 44100.0f;

    CytomicSVF bpL_[kNumBands], bpR_[kNumBands];
    float bandGain_[kNumBands] = {1.0f, 0.8f, 0.6f, 0.4f};

    float shift_ = 0.0f;
    float vowel_ = 0.0f; // 0=A, 1=E, 2=I, 3=O, 4=U
    float resonance_ = 8.0f;
    float mix_ = 0.0f;

    ParameterSmoother shiftSmoother_;
    ParameterSmoother mixSmoother_;

    // Vowel formant frequency tables (Hz) — F1, F2, F3, F4
    // Based on Hillenbrand et al. (1995) adult male averages
    static constexpr float kVowelTable[5][4] = {
        { 730.0f, 1090.0f, 2440.0f, 3400.0f },  // A (father)
        { 530.0f, 1840.0f, 2480.0f, 3520.0f },  // E (bed)
        { 390.0f, 1990.0f, 2550.0f, 3600.0f },  // I (beet)
        { 570.0f,  840.0f, 2410.0f, 3400.0f },  // O (boat)
        { 440.0f, 1020.0f, 2240.0f, 3400.0f },  // U (boot)
    };

    void updateFormants(float shift)
    {
        // Interpolate between vowel tables
        const int vowelA = juce::jlimit(0, 3, static_cast<int>(vowel_));
        const int vowelB = vowelA + 1;
        const float frac = vowel_ - static_cast<float>(vowelA);

        // Shift multiplier (1 octave range: 0.5x to 2.0x)
        const float shiftMult = std::pow(2.0f, shift);

        for (int b = 0; b < kNumBands; ++b)
        {
            const float freqA = kVowelTable[vowelA][b];
            const float freqB = kVowelTable[std::min(vowelB, 4)][b];
            float freq = (freqA + (freqB - freqA) * frac) * shiftMult;
            freq = juce::jlimit(20.0f, sr_ * 0.45f, freq);

            bpL_[b].setCoefficients(freq, resonance_, sr_);
            bpR_[b].setCoefficients(freq, resonance_, sr_);
        }
    }
};

} // namespace xomnibus
```

- [ ] **Step 2: Add fXFormant to MasterFXChain (4 touch-points)**

In `Source/Core/MasterFXChain.h`, make these 4 changes:

1. **Include** (top of file, with other FX includes):
   ```cpp
   #include "../DSP/Effects/fXFormant.h"
   ```

2. **Member declaration** (private section, after fXObscura member):
   ```cpp
   fXFormant formantFX_;
   ```

3. **prepare()** (in the `prepare` method, after fXObscura prepare):
   ```cpp
   formantFX_.prepare(sampleRate, samplesPerBlock);
   ```

4. **processBlock()** (after fXObscura processBlock call, before OTT stage):
   ```cpp
   // Membrane Collection: Formant Filter
   {
       const float fmtMix = pFormantMix_ ? pFormantMix_->load() : 0.0f;
       if (fmtMix > 0.001f)
       {
           formantFX_.setShift(pFormantShift_ ? pFormantShift_->load() : 0.0f);
           formantFX_.setVowel(pFormantVowel_ ? pFormantVowel_->load() : 0.0f);
           formantFX_.setResonance(pFormantQ_ ? pFormantQ_->load() : 8.0f);
           formantFX_.setMix(fmtMix);
           formantFX_.processBlock(buffer.getWritePointer(0), buffer.getWritePointer(1), numSamples);
       }
   }
   ```

- [ ] **Step 3: Add MasterFX parameters and cache pointers for fXFormant**

Add parameters to the MasterFX parameter layout creation, AND add cached atomic pointers in `cacheParameterPointers()`:

Parameters:
```
mfx_formantShift    — Formant Shift (-1.0 to 1.0, default 0.0)
mfx_formantVowel    — Vowel (0.0-4.0, default 0.0 = A)
mfx_formantQ        — Formant Resonance (NormalisableRange 0.5-20.0, skew 0.4, default 8.0)
mfx_formantMix      — Formant Mix (0.0-1.0, default 0.0)
```

Note: `mfx_formantQ` uses skew factor 0.4 so the knob concentrates on the musically useful 0.5-8.0 range (P2-C fix).

Pointer cache (add to private section + cacheParameterPointers):
```cpp
std::atomic<float>* pFormantShift_ = nullptr;
std::atomic<float>* pFormantVowel_ = nullptr;
std::atomic<float>* pFormantQ_ = nullptr;
std::atomic<float>* pFormantMix_ = nullptr;
```

- [ ] **Step 4: Build and verify**

Run: `cmake --build build`
Expected: Clean compile. fXFormant appears in MasterFX chain. Mix=0 means zero CPU.

- [ ] **Step 5: Commit**

```bash
git add Source/DSP/Effects/fXFormant.h Source/Core/MasterFXChain.h
git commit -m "feat: add fXFormant — Membrane Collection formant filter

4-band parallel bandpass at vocal formant frequencies (Hillenbrand 1995).
5 vowel presets (A/E/I/O/U), shift, resonance, mix controls.
Works on both external audio and synth output. Zero CPU at mix=0."
```

---

### Task 4: Build fXBreath — Organic Air Texture FX

**Files:**
- Create: `Source/DSP/Effects/fXBreath.h`
- Modify: `Source/Core/MasterFXChain.h` (add stage)

Shaped noise generator driven by envelope follower. On voice: adds breath/air texture. On synth: adds organic analog feel.

- [ ] **Step 1: Create fXBreath.h**

Create `Source/DSP/Effects/fXBreath.h`:

```cpp
#pragma once
#include <cmath>
#include <cstdint>
#include "../CytomicSVF.h"
#include "../ParameterSmoother.h"

namespace xomnibus {

//==============================================================================
// fXBreath — Membrane Collection organic air texture.
//
// Generates noise shaped by the input signal's envelope, adding breath
// and aspirant texture. On voice: realistic breath noise, "air."
// On synth: organic feel on sterile digital sources.
//
// Signal flow:
//   input → envelope follower → scale noise amplitude
//         → spectral tilt (LP filter on noise) → shape noise color
//         → mix with dry signal
//
// CPU budget: ~15 ops/sample
//
class fXBreath
{
public:
    fXBreath() = default;

    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr_ = static_cast<float>(sampleRate);
        envState_ = 0.0f;

        noiseTiltL_.setMode(CytomicSVF::Mode::LowPass);
        noiseTiltR_.setMode(CytomicSVF::Mode::LowPass);
        noiseTiltL_.reset();
        noiseTiltR_.reset();

        breathSmoother_.prepare(sampleRate, 5.0f);
        tiltSmoother_.prepare(sampleRate, 5.0f);
        mixSmoother_.prepare(sampleRate, 5.0f);

        rngState_ = 0x12345678u;
    }

    void setBreathAmount(float b) { breathAmount_ = juce::jlimit(0.0f, 1.0f, b); }
    void setTilt(float t) { tilt_ = juce::jlimit(0.0f, 1.0f, t); } // 0=bright, 1=dark
    void setSensitivity(float s) { sensitivity_ = juce::jlimit(0.0f, 1.0f, s); }
    void setMix(float m) { mix_ = juce::jlimit(0.0f, 1.0f, m); }

    void processBlock(float* left, float* right, int numSamples)
    {
        if (mix_ < 0.001f) return;

        const float smoothMix = mixSmoother_.process(mix_);
        const float smoothBreath = breathSmoother_.process(breathAmount_);
        const float smoothTilt = tiltSmoother_.process(tilt_);

        // Tilt filter: 500Hz (dark) to 12kHz (bright)
        const float tiltFreq = 500.0f + (1.0f - smoothTilt) * 11500.0f;
        noiseTiltL_.setCoefficients(tiltFreq, 0.5f, sr_);
        noiseTiltR_.setCoefficients(tiltFreq, 0.5f, sr_);

        // Envelope follower coefficients
        const float attack = std::exp(-1.0f / (sr_ * 0.005f));   // 5ms
        const float release = std::exp(-1.0f / (sr_ * 0.05f));   // 50ms

        for (int i = 0; i < numSamples; ++i)
        {
            const float mono = (std::fabs(left[i]) + std::fabs(right[i])) * 0.5f;

            // Envelope follower
            const float coeff = (mono > envState_) ? attack : release;
            envState_ = coeff * envState_ + (1.0f - coeff) * mono;

            // Scale by sensitivity
            const float envScaled = std::min(envState_ * (1.0f + sensitivity_ * 9.0f), 1.0f);

            // Generate shaped noise
            const float noiseL = whiteNoise() * envScaled * smoothBreath;
            const float noiseR = whiteNoise() * envScaled * smoothBreath;

            // Apply spectral tilt
            const float filteredL = noiseTiltL_.processSample(noiseL);
            const float filteredR = noiseTiltR_.processSample(noiseR);

            // Mix
            left[i] += filteredL * smoothMix;
            right[i] += filteredR * smoothMix;
        }
    }

private:
    float sr_ = 44100.0f;
    float envState_ = 0.0f;

    float breathAmount_ = 0.0f;
    float tilt_ = 0.5f;
    float sensitivity_ = 0.5f;
    float mix_ = 0.0f;

    CytomicSVF noiseTiltL_, noiseTiltR_;
    ParameterSmoother breathSmoother_, tiltSmoother_, mixSmoother_;

    uint32_t rngState_ = 0x12345678u;

    // Fast white noise — Lehmer LCG (same RNG as StandardLFO S&H)
    float whiteNoise()
    {
        rngState_ = rngState_ * 196314165u + 907633515u;
        return static_cast<float>(static_cast<int32_t>(rngState_)) / 2147483648.0f;
    }
};

} // namespace xomnibus
```

- [ ] **Step 2: Add fXBreath to MasterFXChain (4 touch-points)**

In `Source/Core/MasterFXChain.h`:

1. **Include**: `#include "../DSP/Effects/fXBreath.h"`
2. **Member**: `fXBreath breathFX_;`
3. **prepare()**: `breathFX_.prepare(sampleRate, samplesPerBlock);`
4. **processBlock()** (after fXFormant block):
   ```cpp
   // Membrane Collection: Breath Texture
   {
       const float brMix = pBreathMix_ ? pBreathMix_->load() : 0.0f;
       if (brMix > 0.001f)
       {
           breathFX_.setBreathAmount(pBreathAmount_ ? pBreathAmount_->load() : 0.0f);
           breathFX_.setTilt(pBreathTilt_ ? pBreathTilt_->load() : 0.5f);
           breathFX_.setSensitivity(pBreathSens_ ? pBreathSens_->load() : 0.5f);
           breathFX_.setMix(brMix);
           breathFX_.processBlock(buffer.getWritePointer(0), buffer.getWritePointer(1), numSamples);
       }
   }
   ```

- [ ] **Step 3: Add MasterFX parameters and cache pointers for fXBreath**

Parameters:
```
mfx_breathAmount    — Breath Amount (0.0-1.0, default 0.0)
mfx_breathTilt      — Breath Tilt (0.0-1.0, default 0.5)
mfx_breathSens      — Breath Sensitivity (0.0-1.0, default 0.5)
mfx_breathMix       — Breath Mix (0.0-1.0, default 0.0)
```

Pointer cache:
```cpp
std::atomic<float>* pBreathAmount_ = nullptr;
std::atomic<float>* pBreathTilt_ = nullptr;
std::atomic<float>* pBreathSens_ = nullptr;
std::atomic<float>* pBreathMix_ = nullptr;
```

- [ ] **Step 4: Build and verify**

Run: `cmake --build build`
Expected: Clean compile. fXBreath in MasterFX chain. Zero CPU at mix=0.

- [ ] **Step 5: Commit**

```bash
git add Source/DSP/Effects/fXBreath.h Source/Core/MasterFXChain.h
git commit -m "feat: add fXBreath — Membrane Collection organic air texture

Envelope-following noise generator with spectral tilt.
On voice: breath/air. On synth: analog organic feel.
Lehmer LCG noise, CytomicSVF tilt filter. Zero CPU at mix=0."
```

---

### Task 5: Integration Test + First Preset

**Files:**
- Create: `Presets/XOmnibus/Atmosphere/Osmosis_First_Breath.xometa`
- Modify: `Source/Core/PresetManager.h` (if not done in Task 2)

- [ ] **Step 1: Create a test preset**

```json
{
  "version": 1,
  "name": "Osmosis First Breath",
  "mood": "Atmosphere",
  "engines": ["Osmosis", "Origami", "", ""],
  "parameters": {
    "osmo_permeability": 0.6,
    "osmo_selectivity": 0.7,
    "osmo_reactivity": 0.5,
    "osmo_memory": 0.4,
    "osmo_macro1": 0.6,
    "osmo_macro2": 0.7,
    "osmo_macro3": 0.5,
    "osmo_macro4": 0.4,
    "mfx_formantMix": 0.3,
    "mfx_formantShift": 0.1,
    "mfx_formantVowel": 0.0,
    "mfx_formantQ": 6.0,
    "mfx_breathMix": 0.15,
    "mfx_breathAmount": 0.4,
    "mfx_breathTilt": 0.6,
    "mfx_breathSens": 0.5
  },
  "coupling": [],
  "macros": {
    "M1": "PERMEABILITY",
    "M2": "SELECTIVITY",
    "M3": "REACTIVITY",
    "M4": "MEMORY"
  },
  "sonicDNA": {
    "brightness": 0.5,
    "warmth": 0.6,
    "movement": 0.3,
    "density": 0.4,
    "space": 0.5,
    "aggression": 0.1
  },
  "tags": ["membrane", "voice", "external", "air"],
  "description": "The membrane breathes. External audio passes through with gentle formant coloring and organic breath texture. Origami in slot 2 loaded with defaults — ready for coupling."
}
```

Note: Origami in slot 2 will load with default parameter values since no `origami_` params are specified. This is intentional — the preset focuses on Osmosis. The smoke test should confirm Origami loads correctly with defaults.
```

- [ ] **Step 2: Build full project**

Run: `cmake --build build`
Expected: Clean compile with all 4 tasks integrated.

- [ ] **Step 3: Manual smoke test (user action)**

Load XOmnibus in DAW. Enable input bus (sidechain). Load "Osmosis First Breath" preset. Send audio from a track. Verify:
- Audio passes through with membrane filter coloring
- fXFormant adds vowel character when mix > 0
- fXBreath adds air texture when mix > 0
- Osmosis appears in engine selector

- [ ] **Step 4: Commit**

```bash
git add Presets/XOmnibus/Atmosphere/Osmosis_First_Breath.xometa
git commit -m "feat: add first Osmosis preset — Osmosis First Breath

Atmosphere mood. Membrane + Origami slot, fXFormant + fXBreath active.
Integration test preset for external audio pipeline."
```

---

## Pre-Build Check (P2-A)

Verify that `CMakeLists.txt` glob pattern picks up `Source/Engines/Osmosis/`. The project uses header-only engines (`.h` only), but JUCE CMake may need the directory listed. Check the existing glob pattern — if it's `Source/Engines/**/*.h`, the new directory is auto-discovered. If directories are listed explicitly, add `Source/Engines/Osmosis/OsmosisEngine.h`. Also add `SynthEngine.h` modification (`isAnalysisEngine`) to the Task 2 file list.

## CLAUDE.md Updates Required After Build

After all tasks pass:
1. Add OSMOSIS to engine table in CLAUDE.md (engine count 70 → 71)
2. Add `osmo_` to parameter prefix table
3. Add `Source/Engines/Osmosis/OsmosisEngine.h` to Key Files table
4. Add fXFormant + fXBreath to FX chain documentation in MasterFXChain section
5. Update product identity header engine count and comma list
6. Update `Docs/xomnibus_master_specification.md` section 3.1
