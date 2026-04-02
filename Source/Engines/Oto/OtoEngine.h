#pragma once
//==============================================================================
//
//  OtoEngine.h -- XOto | "The Breath Between Worlds"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOto (音 = "sound" in Japanese) is the first of the Chef Quad --
//      the Kitchen Essentials Collection. Oto is the disciplined minimalist,
//      the chef of elemental sound itself. Where other engines sculpt novelty,
//      Oto distills the ancient breath-controlled free-reed instruments of
//      East Asia into a single, switchable instrument. Four organ models,
//      one breath, one truth: sound is the fundamental ingredient.
//
//  ENGINE CONCEPT:
//      A multi-model organ synthesizer hosting four distinct mouth organ
//      traditions -- Sho (Japanese), Sheng (Chinese), Khene (Lao/Thai),
//      and Melodica (modern/global). Each model uses additive synthesis
//      with instrument-specific partial ratios, beating patterns, chiff
//      transients, and breath-pressure instability. The result spans
//      ethereal sustained clusters to raw buzzy folk drones.
//
//  THE 4 ORGAN MODELS:
//      0: Sho (笙) -- Japanese mouth organ. 11-note aitake cluster chord.
//         Free-reed aerophone. Ethereal, sustained, otherworldly.
//      1: Sheng (笙) -- Chinese mouth organ. Sharper attack, brighter.
//         Gourd resonator. Melodic, precise.
//      2: Khene (แคน) -- Lao/Thai mouth organ. Paired pipes, natural beating.
//         Buzzy, raw, folk, rhythmic.
//      3: Melodica -- Breath-controlled keyboard. Saw/square through body
//         resonance. Warm, intimate, lo-fi, human.
//
//  HISTORICAL LINEAGE:
//      Sho: Gagaku court music, Toru Takemitsu. Sheng: one of the oldest
//      instruments (~3000 years), precursor to accordion/harmonica. Khene:
//      Isan folk music, 14-18 bamboo pipes. Melodica: Augustus Pablo,
//      Jon Batiste, Gorillaz.
//
//  Accent: Bamboo Green #7BA05B (living bamboo -- the material of all
//          four instruments' pipes/reeds)
//  Parameter prefix: oto_
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/FilterEnvelope.h"
#include "../../DSP/GlideProcessor.h"
#include "../../DSP/ParameterSmoother.h"
#include "../../DSP/VoiceAllocator.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace xoceanus {

//==============================================================================
// Aitake cluster chord partial ratios for Sho (from gagaku tuning theory).
// The aitake is a fixed 11-note cluster that sustains as a single shimmering
// voice. Ratios are relative to the fundamental, derived from the 17-pipe
// configuration of the traditional sho.
//==============================================================================
namespace {  // anonymous namespace — prevents ODR violations when multiple engine headers are included

constexpr float kShoAitakeRatios[11] = {
    1.000f,   // fundamental (root)
    1.0595f,  // minor 2nd (beating with root)
    1.1892f,  // minor 3rd
    1.3348f,  // perfect 4th (approximate)
    1.4983f,  // perfect 5th (approximate)
    1.5874f,  // augmented 5th
    1.7818f,  // minor 7th
    2.000f,   // octave
    2.3784f,  // minor 3rd + octave
    2.9966f,  // 5th + octave
    3.5636f   // min 7th + octave
};

//==============================================================================
// Sheng partial ratios -- brighter, more defined harmonics.
// Based on measured spectra of 17-pipe sheng (Zhou, 2010).
//==============================================================================
constexpr float kShengRatios[11] = {
    1.000f,
    2.000f,   // strong octave
    3.000f,   // 12th
    4.000f,   // double octave
    5.000f,   // major 3rd + 2 octaves
    6.000f,
    7.000f,
    8.000f,
    9.000f,
    10.00f,
    11.00f
};

//==============================================================================
// Khene paired-pipe ratios -- deliberately non-harmonic for beating.
// Each "pipe pair" has slight detuning (~3-8 cents) built in.
//==============================================================================
constexpr float kKheneRatios[11] = {
    1.000f,
    1.003f,   // detuned unison (paired pipe beating)
    2.000f,
    2.007f,   // detuned octave pair
    3.000f,
    3.005f,
    4.000f,
    4.009f,
    5.000f,
    6.000f,
    7.000f
};

//==============================================================================
// Melodica has simpler harmonic content (reed + plastic body resonance).
// Mix of saw-like harmonics with body resonance shaping.
//==============================================================================
constexpr float kMelodicaRatios[11] = {
    1.000f,
    2.000f,
    3.000f,
    4.000f,
    5.000f,
    6.000f,
    7.000f,
    8.000f,
    0.0f,    // unused (melodica has fewer strong partials)
    0.0f,
    0.0f
};

//==============================================================================
// Per-model default amplitudes for the partials.
// Sho: relatively even (cluster chord), Sheng: falling harmonics,
// Khene: paired strong, Melodica: saw-like 1/n rolloff.
//==============================================================================
constexpr float kShoAmps[11] = {
    1.0f, 0.9f, 0.85f, 0.8f, 0.75f, 0.65f, 0.6f, 0.5f, 0.4f, 0.3f, 0.25f
};
constexpr float kShengAmps[11] = {
    1.0f, 0.7f, 0.5f, 0.35f, 0.25f, 0.18f, 0.13f, 0.09f, 0.06f, 0.04f, 0.03f
};
constexpr float kKheneAmps[11] = {
    1.0f, 0.95f, 0.7f, 0.65f, 0.5f, 0.45f, 0.35f, 0.3f, 0.2f, 0.15f, 0.1f
};
constexpr float kMelodicaAmps[11] = {
    1.0f, 0.5f, 0.33f, 0.25f, 0.2f, 0.16f, 0.14f, 0.12f, 0.0f, 0.0f, 0.0f
};

}  // anonymous namespace

//==============================================================================
// OtoChiffGenerator -- brief harmonic burst on note-on that simulates the
// initial air rush / reed activation transient. Each model has different
// chiff characteristics: Sho = breathy, Sheng = sharp, Khene = buzzy,
// Melodica = airy plastic.
//==============================================================================
struct OtoChiffGenerator
{
    void trigger (float velocity, float chiffAmount, int organModel, float sampleRate) noexcept
    {
        if (chiffAmount < 0.001f) { active = false; return; }
        active = true;
        sampleCounter = 0;

        // Chiff duration: 10-60ms depending on model
        float durationMs;
        switch (organModel)
        {
            case 0: durationMs = 40.0f; break;  // Sho: slow, breathy
            case 1: durationMs = 15.0f; break;  // Sheng: sharp, quick
            case 2: durationMs = 25.0f; break;  // Khene: medium, buzzy
            default: durationMs = 35.0f; break;  // Melodica: airy
        }
        totalSamples = std::max (static_cast<int> (durationMs * 0.001f * sampleRate), 8);
        // Store velocity only; chiff amount is applied per-sample from the smoother
        // so that macro-boosted effective chiff is reflected in real-time.
        amplitude = velocity;
        noiseMix = (organModel == 2) ? 0.7f : 0.3f;  // Khene = more noise
        noiseState = static_cast<uint32_t> (velocity * 65535.0f + organModel * 9973) + 12345u;
    }

    // chiffScale: pass the per-sample smoothed chiff amount (macro-modified).
    float process (float chiffScale = 1.0f) noexcept
    {
        if (!active) return 0.0f;
        if (sampleCounter >= totalSamples) { active = false; return 0.0f; }

        float phase = static_cast<float> (sampleCounter) / static_cast<float> (totalSamples);
        // Hann-windowed burst; amplitude holds velocity, chiffScale carries macro-modified amount.
        float env = std::sin (phase * 3.14159265f) * amplitude * chiffScale;

        // Noise component
        noiseState = noiseState * 1664525u + 1013904223u;
        float noise = (static_cast<float> (noiseState & 0xFFFF) / 32768.0f - 1.0f);

        // Pitched component (high harmonics burst)
        float pitched = fastSin (phase * 3.14159265f * 8.0f);

        float out = env * (pitched * (1.0f - noiseMix) + noise * noiseMix);
        ++sampleCounter;
        return out;
    }

    void reset() noexcept { active = false; sampleCounter = 0; }

    bool active = false;
    int sampleCounter = 0;
    int totalSamples = 100;
    float amplitude = 0.5f;
    float noiseMix = 0.3f;
    uint32_t noiseState = 12345u;
};

//==============================================================================
// OtoBreathSource -- models breath pressure instability (pitch + amplitude
// drift). Creates the organic, human quality of breath-driven instruments.
//==============================================================================
struct OtoBreathSource
{
    void prepare (float sampleRate) noexcept
    {
        sr = sampleRate;
        driftLFO.setRate (0.3f, sampleRate);  // slow breath wander
        driftLFO.setShape (StandardLFO::Sine);
        tremLFO.setRate (5.5f, sampleRate);   // faster tremolo
        tremLFO.setShape (StandardLFO::Sine);
    }

    // Returns { pitchDriftCents, amplitudeMod }
    void process (float pressure, float& pitchDrift, float& ampMod) noexcept
    {
        if (pressure < 0.001f) { pitchDrift = 0.0f; ampMod = 0.0f; return; }

        float drift = driftLFO.process();
        float trem  = tremLFO.process();

        // Pitch drift: up to +/-8 cents at full pressure instability
        pitchDrift = drift * pressure * 8.0f;

        // Amplitude modulation: subtle tremolo from unsteady breath
        ampMod = trem * pressure * 0.15f;
    }

    void reset() noexcept
    {
        driftLFO.reset();
        tremLFO.reset();
    }

    float sr = 48000.0f;
    StandardLFO driftLFO;
    StandardLFO tremLFO;
};

//==============================================================================
// OtoVoice -- a single voice with 11 additive partials, chiff, breath source,
// filter envelope, main filter, and LFO.
//==============================================================================
struct OtoVoice
{
    static constexpr int kMaxPartials = 11;

    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    float velocity = 0.0f;

    // Partial oscillator phases (additive synthesis)
    std::array<float, kMaxPartials> partialPhases {};

    GlideProcessor glide;
    OtoChiffGenerator chiff;
    OtoBreathSource breath;
    FilterEnvelope ampEnv;
    FilterEnvelope filterEnv;
    CytomicSVF svf;
    StandardLFO lfo1;

    // Crossfade state for seamless organ model switching (~50ms linear crossfade)
    float prevOrganGain = 0.0f;
    int prevOrganModel = -1;
    int crossfadeCounter = 0;    // counts down from crossfadeSamples to 0
    int crossfadeSamples = 0;    // set to 50ms worth of samples when model changes
    std::array<float, kMaxPartials> prevPartialPhases {};  // phase snapshot of old model

    // Sustain pedal hold: voice stays active while CC64 is down
    bool sustainHeld = false;

    // BUG-2: PolyBLEP oscillator for Melodica fundamental (model 3)
    PolyBLEP melodicaOsc;

    // Per-voice cached pan gains
    float panL = 0.707f, panR = 0.707f;

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        prevOrganGain = 0.0f;
        prevOrganModel = -1;
        crossfadeCounter = 0;
        crossfadeSamples = 0;
        sustainHeld = false;
        partialPhases.fill (0.0f);
        prevPartialPhases.fill (0.0f);
        melodicaOsc.reset();
        glide.reset();
        chiff.reset();
        breath.reset();
        ampEnv.kill();
        filterEnv.kill();
        svf.reset();
        lfo1.reset();
    }
};

//==============================================================================
// OtoEngine -- "The Breath Between Worlds"
// Chef Quad #1: East Asia | Disciplined Minimalism
//==============================================================================
class OtoEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;
    static constexpr float kTwoPi = 6.28318530717958647692f;

    juce::String getEngineId() const override { return "Oto"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFFF5F0E8); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load(); }

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            voices[i].reset();
            voices[i].breath.prepare (srf);
            voices[i].ampEnv.prepare (srf);
            voices[i].filterEnv.prepare (srf);
            voices[i].lfo1.reseed (static_cast<uint32_t> (i * 7919 + 31));
        }

        smoothCluster.prepare (srf);
        smoothChiff.prepare (srf);
        smoothDetune.prepare (srf);
        smoothBuzz.prepare (srf);
        smoothPressure.prepare (srf);
        smoothCrosstalk.prepare (srf);
        smoothCutoff.prepare (srf, 0.010f);  // 10ms for filter to avoid clicks
        smoothResonance.prepare (srf);

        prepareSilenceGate (sr, maxBlockSize, 500.0f);  // sustained organ: 500ms hold
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices) v.reset();
        pitchBendNorm = 0.0f;
        modWheelAmount = 0.0f;
        aftertouchAmount = 0.0f;
        sustainPedalDown = false;
    }

    float getSampleForCoupling (int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? couplingCacheL : couplingCacheR;
    }

    void applyCouplingInput (CouplingType type, float amount,
                            const float* buf, int numSamples) override
    {
        if (!buf || numSamples <= 0) return;
        float val = buf[numSamples - 1] * amount;
        switch (type) {
            case CouplingType::AmpToFilter:  couplingFilterMod += val * 3000.0f; break;
            case CouplingType::LFOToPitch:   couplingPitchMod += val * 2.0f; break;
            case CouplingType::AmpToPitch:   couplingPitchMod += val; break;
            case CouplingType::EnvToMorph:   couplingOrganMod += val; break;
            default: break;
        }
    }

    //==========================================================================
    // Render -- all 4 organ models + shared DSP chain
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        // ---- MIDI parsing ----
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
                wakeSilenceGate();
            }
            else if (msg.isNoteOff())
            {
                // BUG-3: CC64 sustain pedal — hold voice while pedal is down
                if (!sustainPedalDown)
                    noteOff (msg.getNoteNumber());
                else
                    sustainNoteOff (msg.getNoteNumber());  // mark sustained, don't release
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                reset();
                sustainPedalDown = false;
            }
            else if (msg.isPitchWheel())
            {
                pitchBendNorm = PitchBendUtil::parsePitchWheel (msg.getPitchWheelValue());
            }
            else if (msg.isChannelPressure())
            {
                aftertouchAmount = msg.getChannelPressureValue() / 127.0f;
            }
            else if (msg.isController())
            {
                const int cc  = msg.getControllerNumber();
                const int val = msg.getControllerValue();
                if (cc == 1)   // mod wheel
                {
                    modWheelAmount = static_cast<float> (val) / 127.0f;
                }
                else if (cc == 64)  // CC64: Sustain pedal
                {
                    bool wasDown = sustainPedalDown;
                    sustainPedalDown = (val >= 64);
                    if (wasDown && !sustainPedalDown)
                    {
                        // Pedal released: release all voices that were held by the pedal
                        for (auto& v : voices)
                        {
                            if (v.active && v.sustainHeld)
                            {
                                v.sustainHeld = false;
                                v.ampEnv.release();
                                v.filterEnv.release();
                            }
                        }
                    }
                }
            }
        }

        // ---- SRO: silence gate bypass ----
        if (isSilenceGateBypassed()) {
            buffer.clear (0, numSamples);
            couplingCacheL = couplingCacheR = 0.0f;
            return;
        }

        // ---- Parameter snapshot (ParamSnapshot pattern) ----
        auto loadP = [] (std::atomic<float>* p, float def) {
            return p ? p->load (std::memory_order_relaxed) : def;
        };

        const int   pOrgan       = static_cast<int> (loadP (paramOrgan, 0.0f));
        const float pCluster     = loadP (paramCluster, 0.7f);
        const float pChiff       = loadP (paramChiff, 0.3f);
        const float pDetune      = loadP (paramDetune, 0.15f);
        const float pBuzz        = loadP (paramBuzz, 0.0f);
        const float pPressure    = loadP (paramPressure, 0.2f);
        const float pCrosstalk   = loadP (paramCrosstalk, 0.0f);
        const float pCutoff      = loadP (paramFilterCutoff, 8000.0f);
        const float pRes         = loadP (paramFilterRes, 0.1f);
        const float pLFO1Rate    = loadP (paramLfo1Rate, 0.3f);
        const float pLFO1Depth   = loadP (paramLfo1Depth, 0.0f);
        const float pCompetition = loadP (paramCompetition, 0.0f);

        const float macroA       = loadP (paramMacroA, 0.0f);
        const float macroB       = loadP (paramMacroB, 0.0f);
        const float macroC       = loadP (paramMacroC, 0.0f);
        const float macroD       = loadP (paramMacroD, 0.0f);

        // ---- Macro mappings ----
        // Macro A (CHARACTER) -> cluster + chiff + buzz blend
        float effCluster  = std::clamp (pCluster + macroA * 0.5f, 0.0f, 1.0f);
        float effChiff    = std::clamp (pChiff + macroA * 0.3f, 0.0f, 1.0f);
        float effBuzz     = std::clamp (pBuzz + macroA * 0.4f, 0.0f, 1.0f);

        // Macro B (MOVEMENT) -> LFO depth + pressure instability
        float effLFODepth = std::clamp (pLFO1Depth + macroB * 0.5f, 0.0f, 1.0f);
        float effPressure = std::clamp (pPressure + macroB * 0.4f, 0.0f, 1.0f);

        // Macro C (COUPLING) -> competition + crosstalk
        float effCompetition = std::clamp (pCompetition + macroC * 0.5f, 0.0f, 1.0f);
        float effCrosstalk   = std::clamp (pCrosstalk + macroC * 0.4f, 0.0f, 1.0f);

        // Macro D (SPACE) -> reverb/delay send (passed to coupling matrix)
        // (macroD is available for host-level FX routing; here it subtly opens filter)
        float effCutoff = std::clamp (pCutoff + macroD * 4000.0f + couplingFilterMod, 20.0f, 20000.0f);

        // D006: mod wheel -> filter cutoff modulation (breath pressure expression)
        effCutoff = std::clamp (effCutoff + modWheelAmount * 5000.0f, 20.0f, 20000.0f);

        // D006: aftertouch -> pressure instability + filter brightness
        effPressure = std::clamp (effPressure + aftertouchAmount * 0.3f, 0.0f, 1.0f);
        effCutoff = std::clamp (effCutoff + aftertouchAmount * 3000.0f, 20.0f, 20000.0f);

        float effRes = std::clamp (pRes + couplingOrganMod * 0.2f, 0.0f, 1.0f);

        smoothCluster.set (effCluster);
        smoothChiff.set (effChiff);
        smoothDetune.set (pDetune);
        smoothBuzz.set (effBuzz);
        smoothPressure.set (effPressure);
        smoothCrosstalk.set (effCrosstalk);
        smoothCutoff.set (effCutoff);
        smoothResonance.set (effRes);

        const float bendSemitones = pitchBendNorm * 2.0f;  // +-2 semitone range

        // ---- Determine current organ model (clamped) ----
        int organModel = std::clamp (pOrgan, 0, 3);

        // Partial tables are selected per-voice inside the synthesiseModel lambda below.

        // ---- Compute crosstalk mix (adjacent voice leakage) ----
        // Pre-compute voice outputs from last block for crosstalk injection
        // (crosstalk uses one-block delay to avoid feedback loops)
        std::array<float, kMaxVoices> prevVoiceOut {};
        for (int i = 0; i < kMaxVoices; ++i)
            prevVoiceOut[i] = lastVoiceOutputs[i];

        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

        for (int s = 0; s < numSamples; ++s)
        {
            float clusterNow   = smoothCluster.process();
            float chiffNow     = smoothChiff.process();
            float detuneNow    = smoothDetune.process();
            float buzzNow      = smoothBuzz.process();
            float pressureNow  = smoothPressure.process();
            float crosstalkNow = smoothCrosstalk.process();
            float cutoffNow    = smoothCutoff.process();
            float resNow       = smoothResonance.process();

            float mixL = 0.0f, mixR = 0.0f;

            // Count active voices for competition (adversarial coupling)
            int voiceCount = 0;
            for (const auto& v : voices) if (v.active) ++voiceCount;

            for (int vi = 0; vi < kMaxVoices; ++vi)
            {
                auto& voice = voices[vi];
                if (!voice.active) continue;

                float freq = voice.glide.process();
                freq *= PitchBendUtil::semitonesToFreqRatio (bendSemitones + couplingPitchMod);

                // ---- Breath pressure instability ----
                float pitchDriftCents = 0.0f, ampMod = 0.0f;
                voice.breath.process (pressureNow, pitchDriftCents, ampMod);
                freq *= fastPow2 (pitchDriftCents / 1200.0f);

                // ---- LFO1 -> pitch vibrato ----
                voice.lfo1.setRate (std::max (0.01f, pLFO1Rate), srf);  // D005: rate floor
                voice.lfo1.setShape (StandardLFO::Sine);
                float lfo1Val = voice.lfo1.process() * effLFODepth;
                freq *= fastPow2 (lfo1Val * 0.5f / 12.0f);  // max +/-0.5 semitone vibrato

                // ---- Competition: adversarial coupling ----
                // More voices = less amplitude per voice (shared breath)
                float competitionScale = 1.0f;
                if (effCompetition > 0.001f && voiceCount > 1)
                {
                    float reduction = effCompetition * (static_cast<float> (voiceCount - 1) / static_cast<float> (kMaxVoices));
                    competitionScale = 1.0f - reduction * 0.6f;
                    competitionScale = std::max (competitionScale, 0.2f);
                }

                // ---- BUG-1: Organ crossfade — detect model change and initiate 50ms fade ----
                if (voice.prevOrganModel == -1)
                {
                    // First sample for this voice: initialise with no crossfade
                    voice.prevOrganModel = organModel;
                    voice.crossfadeCounter = 0;
                    voice.crossfadeSamples = 0;
                }
                else if (voice.prevOrganModel != organModel && voice.crossfadeCounter == 0)
                {
                    // Model just changed: snapshot prev phases and start crossfade
                    voice.prevPartialPhases = voice.partialPhases;
                    voice.crossfadeSamples = static_cast<int> (0.050f * srf);  // 50ms
                    voice.crossfadeCounter = voice.crossfadeSamples;
                    voice.prevOrganGain = 1.0f;
                }

                // Helper lambda: synthesise one model into a signal value.
                // Uses voice.prevPartialPhases for old model, voice.partialPhases for current.
                auto synthesiseModel = [&] (int model, bool usePrev) -> float
                {
                    const float* mRatios = nullptr;
                    const float* mAmps   = nullptr;
                    int mPartials = 11;
                    switch (model)
                    {
                        case 0: mRatios = kShoAitakeRatios; mAmps = kShoAmps;     mPartials = 11; break;
                        case 1: mRatios = kShengRatios;      mAmps = kShengAmps;   mPartials = 11; break;
                        case 2: mRatios = kKheneRatios;      mAmps = kKheneAmps;   mPartials = 11; break;
                        default: mRatios = kMelodicaRatios;  mAmps = kMelodicaAmps; mPartials = 8; break;
                    }

                    int nP = static_cast<int> (1.0f + clusterNow * static_cast<float> (mPartials - 1));
                    nP = std::clamp (nP, 1, mPartials);

                    float sig = 0.0f;
                    auto& phases = usePrev ? voice.prevPartialPhases : voice.partialPhases;

                    for (int p = 0; p < nP; ++p)
                    {
                        float ratio = mRatios[p];
                        if (ratio < 0.001f) continue;

                        float partialFreq = freq * ratio;
                        if (p > 0 && detuneNow > 0.001f)
                        {
                            float detuneOffset = (static_cast<float> (p) - 0.5f * static_cast<float> (nP))
                                               * detuneNow * 0.5f;
                            partialFreq += detuneOffset;
                        }

                        float phaseInc = partialFreq / srf;
                        phases[p] += phaseInc;
                        if (phases[p] >= 1.0f) phases[p] -= 1.0f;

                        float partialSample;
                        if (model == 3 && p == 0)
                        {
                            // BUG-2: Melodica fundamental — PolyBLEP anti-aliased sawtooth
                            // (only for current model; prev-model phases use simple saw for
                            //  the brief crossfade window to avoid PolyBLEP state issues)
                            if (!usePrev)
                            {
                                voice.melodicaOsc.setFrequency (partialFreq, srf);
                                voice.melodicaOsc.setWaveform (PolyBLEP::Waveform::Saw);
                                partialSample = voice.melodicaOsc.processSample();
                            }
                            else
                            {
                                // During crossfade out of old Melodica model: simple saw is fine
                                // (transient is already fading to silence)
                                partialSample = 2.0f * phases[p] - 1.0f;
                            }
                        }
                        else
                        {
                            partialSample = fastSin (phases[p] * kTwoPi);
                        }

                        float ampScale = mAmps[p];
                        if (p >= nP - 1 && nP < mPartials)
                        {
                            float frac = (clusterNow * static_cast<float> (mPartials - 1))
                                       - static_cast<float> (nP - 1);
                            ampScale *= std::clamp (frac, 0.0f, 1.0f);
                        }
                        sig += partialSample * ampScale;
                    }
                    if (nP > 1) sig /= std::sqrt (static_cast<float> (nP));
                    return sig;
                };

                // ---- Synthesise current model ----
                float signal = synthesiseModel (organModel, false);

                if (voice.crossfadeCounter > 0)
                {
                    // Linear crossfade: new model fades in, old model fades out.
                    // Apply buzz per-branch before blending so timbral character
                    // of each model is preserved throughout the transition.
                    float fadeIn  = 1.0f - static_cast<float> (voice.crossfadeCounter)
                                              / static_cast<float> (voice.crossfadeSamples);
                    float fadeOut = 1.0f - fadeIn;

                    // Buzz the new-model signal
                    if (buzzNow > 0.001f)
                    {
                        float newBuzzGain = (organModel == 2) ? buzzNow * 8.0f : buzzNow * 4.0f;
                        signal = fastTanh (signal * (1.0f + newBuzzGain));
                    }

                    float oldSignal = synthesiseModel (voice.prevOrganModel, true);

                    // Buzz the old-model signal with its own scale
                    if (buzzNow > 0.001f)
                    {
                        float oldBuzzGain = (voice.prevOrganModel == 2) ? buzzNow * 8.0f : buzzNow * 4.0f;
                        oldSignal = fastTanh (oldSignal * (1.0f + oldBuzzGain));
                    }

                    signal = oldSignal * fadeOut + signal * fadeIn;

                    --voice.crossfadeCounter;
                    if (voice.crossfadeCounter == 0)
                    {
                        // Crossfade complete: commit new model
                        voice.prevOrganModel = organModel;
                        voice.prevOrganGain = 0.0f;
                    }
                }
                else
                {
                    // Steady-state: apply buzz to the final signal and keep model in sync
                    if (buzzNow > 0.001f)
                    {
                        float buzzGain = (organModel == 2) ? buzzNow * 8.0f : buzzNow * 4.0f;
                        signal = fastTanh (signal * (1.0f + buzzGain));
                    }
                    voice.prevOrganModel = organModel;
                }

                // ---- Chiff transient ----
                signal += voice.chiff.process (chiffNow);

                // ---- Crosstalk: adjacent voice leakage ----
                if (crosstalkNow > 0.001f)
                {
                    int prev = (vi > 0) ? vi - 1 : kMaxVoices - 1;
                    int next = (vi < kMaxVoices - 1) ? vi + 1 : 0;
                    float crosstalkSignal = (prevVoiceOut[prev] + prevVoiceOut[next]) * 0.5f;
                    signal += crosstalkSignal * crosstalkNow * 0.3f;
                }

                // ---- Amp envelope ----
                float envLevel = voice.ampEnv.process();
                if (!voice.ampEnv.isActive()) { voice.active = false; continue; }

                // Apply breath amplitude modulation
                envLevel *= (1.0f + ampMod);

                // Competition scaling
                envLevel *= competitionScale;

                // ---- D001: velocity -> filter brightness ----
                float velCutoffBoost = voice.velocity * 4000.0f;
                float voiceCutoff = std::clamp (cutoffNow + velCutoffBoost, 20.0f, 20000.0f);

                // Filter envelope -> cutoff
                float filterEnvLevel = voice.filterEnv.process();
                voiceCutoff = std::clamp (voiceCutoff + filterEnvLevel * voice.velocity * 3000.0f,
                                          20.0f, 20000.0f);

                // LFO1 -> filter modulation (secondary target)
                voiceCutoff = std::clamp (voiceCutoff + lfo1Val * 2000.0f, 20.0f, 20000.0f);

                voice.svf.setMode (CytomicSVF::Mode::LowPass);
                voice.svf.setCoefficients_fast (voiceCutoff, resNow, srf);
                float filtered = voice.svf.processSample (signal);

                float output = filtered * envLevel;

                // Cache for crosstalk
                lastVoiceOutputs[vi] = output;

                // Pan spread (cached at note-on)
                mixL += output * voice.panL;
                mixR += output * voice.panR;
            }

            outL[s] = mixL;
            if (outR) outR[s] = mixR;
            couplingCacheL = mixL;
            couplingCacheR = mixR;
        }

        // Update active voice count (atomic for UI thread reads)
        int count = 0;
        for (const auto& v : voices) if (v.active) ++count;
        activeVoiceCount.store (count);

        // Clear coupling accumulators at end of block (after per-sample loop consumes them).
        // Moved from mid-block so couplingPitchMod is live during the per-sample loop.
        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;
        couplingOrganMod = 0.0f;

        analyzeForSilenceGate (buffer, numSamples);
    }

    //==========================================================================
    // Note management
    //==========================================================================

    void noteOn (int note, float vel) noexcept
    {
        int idx = VoiceAllocator::findFreeVoice (voices, kMaxVoices);
        auto& v = voices[idx];

        float freq = 440.0f * std::pow (2.0f, (static_cast<float> (note) - 69.0f) / 12.0f);
        int organModel = paramOrgan ? static_cast<int> (paramOrgan->load()) : 0;
        organModel = std::clamp (organModel, 0, 3);

        v.active = true;
        v.sustainHeld = false;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        v.glide.snapTo (freq);
        v.partialPhases.fill (0.0f);
        v.prevPartialPhases.fill (0.0f);
        v.crossfadeCounter = 0;
        v.crossfadeSamples = 0;
        v.prevOrganGain = 0.0f;
        v.prevOrganModel = -1;  // will be initialised on first render sample

        // BUG-2: Reset PolyBLEP oscillator for Melodica model (phase continuity per note)
        v.melodicaOsc.reset();
        if (organModel == 3)
        {
            float freq0 = freq;  // fundamental
            v.melodicaOsc.setFrequency (freq0, srf);
            v.melodicaOsc.setWaveform (PolyBLEP::Waveform::Saw);
        }

        // Amp envelope -- organ-specific attack characteristics
        float attack, decay, sustain, release;
        float atkParam   = paramAttack  ? paramAttack->load()  : 0.08f;
        float decParam   = paramDecay   ? paramDecay->load()   : 0.5f;
        float susParam   = paramSustain ? paramSustain->load() : 0.8f;
        float relParam   = paramRelease ? paramRelease->load() : 0.4f;

        switch (organModel)
        {
            case 0:  // Sho: slow, gentle attack
                attack  = std::max (atkParam, 0.05f);
                decay   = decParam;
                sustain = susParam;
                release = std::max (relParam, 0.3f);
                break;
            case 1:  // Sheng: quick, precise attack
                attack  = std::max (atkParam * 0.5f, 0.005f);
                decay   = decParam * 0.7f;
                sustain = susParam;
                release = relParam;
                break;
            case 2:  // Khene: medium attack, rhythmic
                attack  = atkParam;
                decay   = decParam;
                sustain = susParam * 0.9f;
                release = std::max (relParam * 0.6f, 0.05f);
                break;
            default:  // Melodica: responsive
                attack  = std::max (atkParam * 0.8f, 0.003f);
                decay   = decParam;
                sustain = susParam;
                release = relParam;
                break;
        }

        v.ampEnv.prepare (srf);
        v.ampEnv.setADSR (attack, decay, sustain, release);
        v.ampEnv.triggerHard();

        // Filter envelope
        v.filterEnv.prepare (srf);
        v.filterEnv.setADSR (0.001f, 0.3f + (1.0f - vel) * 0.5f, 0.0f, 0.3f);
        v.filterEnv.triggerHard();

        // Chiff transient
        float chiffAmt = paramChiff ? paramChiff->load() : 0.3f;
        v.chiff.trigger (vel, chiffAmt, organModel, srf);

        // Breath source reset
        v.breath.prepare (srf);

        // LFO reset with voice-stagger
        v.lfo1.reset (static_cast<float> (idx) / static_cast<float> (kMaxVoices));

        // Pan: stereo spread based on voice index
        float panAngle = (static_cast<float> (idx) / static_cast<float> (kMaxVoices - 1) - 0.5f) * 0.6f;
        v.panL = std::cos ((panAngle + 0.5f) * 1.5707963f);
        v.panR = std::sin ((panAngle + 0.5f) * 1.5707963f);

        v.svf.reset();
    }

    void noteOff (int note) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && v.currentNote == note && !v.sustainHeld)
            {
                v.ampEnv.release();
                v.filterEnv.release();
            }
        }
    }

    // BUG-3: Mark voice as sustained (pedal held) without releasing envelope
    void sustainNoteOff (int note) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && v.currentNote == note)
                v.sustainHeld = true;
        }
    }

    //==========================================================================
    // Parameters -- 20 total
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl(params);
        return { params.begin(), params.end() };
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;

        // Organ select (0=Sho, 1=Sheng, 2=Khene, 3=Melodica)
        params.push_back (std::make_unique<PI> (juce::ParameterID { "oto_organ", 1 }, "Oto Organ Model", 0, 3, 0));

        // Engine-specific timbral controls
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_cluster", 1 }, "Oto Cluster Density",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.7f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_chiff", 1 }, "Oto Chiff Transient",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_detune", 1 }, "Oto Unison Detune",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.15f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_buzz", 1 }, "Oto Buzz Intensity",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_pressure", 1 }, "Oto Pressure Instability",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.2f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_crosstalk", 1 }, "Oto Crosstalk",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // Filter
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_filterCutoff", 1 }, "Oto Filter Cutoff",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.3f), 8000.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_filterRes", 1 }, "Oto Filter Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.1f));

        // Amp ADSR
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_attack", 1 }, "Oto Attack",
            juce::NormalisableRange<float> (0.001f, 2.0f, 0.0f, 0.4f), 0.08f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_decay", 1 }, "Oto Decay",
            juce::NormalisableRange<float> (0.01f, 5.0f, 0.0f, 0.4f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_sustain", 1 }, "Oto Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.8f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_release", 1 }, "Oto Release",
            juce::NormalisableRange<float> (0.01f, 8.0f, 0.0f, 0.4f), 0.4f));

        // LFO
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_lfo1Rate", 1 }, "Oto LFO1 Rate",
            juce::NormalisableRange<float> (0.01f, 20.0f, 0.0f, 0.3f), 0.3f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_lfo1Depth", 1 }, "Oto LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // Coupling
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_competition", 1 }, "Oto Competition",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // Macros (4 standard)
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_macroA", 1 }, "Oto Macro CHARACTER",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_macroB", 1 }, "Oto Macro MOVEMENT",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_macroC", 1 }, "Oto Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "oto_macroD", 1 }, "Oto Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramOrgan          = apvts.getRawParameterValue ("oto_organ");
        paramCluster        = apvts.getRawParameterValue ("oto_cluster");
        paramChiff          = apvts.getRawParameterValue ("oto_chiff");
        paramDetune         = apvts.getRawParameterValue ("oto_detune");
        paramBuzz           = apvts.getRawParameterValue ("oto_buzz");
        paramPressure       = apvts.getRawParameterValue ("oto_pressure");
        paramCrosstalk      = apvts.getRawParameterValue ("oto_crosstalk");
        paramFilterCutoff   = apvts.getRawParameterValue ("oto_filterCutoff");
        paramFilterRes      = apvts.getRawParameterValue ("oto_filterRes");
        paramAttack         = apvts.getRawParameterValue ("oto_attack");
        paramDecay          = apvts.getRawParameterValue ("oto_decay");
        paramSustain        = apvts.getRawParameterValue ("oto_sustain");
        paramRelease        = apvts.getRawParameterValue ("oto_release");
        paramLfo1Rate       = apvts.getRawParameterValue ("oto_lfo1Rate");
        paramLfo1Depth      = apvts.getRawParameterValue ("oto_lfo1Depth");
        paramCompetition    = apvts.getRawParameterValue ("oto_competition");
        paramMacroA         = apvts.getRawParameterValue ("oto_macroA");
        paramMacroB         = apvts.getRawParameterValue ("oto_macroB");
        paramMacroC         = apvts.getRawParameterValue ("oto_macroC");
        paramMacroD         = apvts.getRawParameterValue ("oto_macroD");
    }

private:
    double sr = 48000.0;
    float srf = 48000.0f;

    std::array<OtoVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount { 0 };

    ParameterSmoother smoothCluster, smoothChiff, smoothDetune, smoothBuzz;
    ParameterSmoother smoothPressure, smoothCrosstalk, smoothCutoff, smoothResonance;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;
    bool sustainPedalDown = false;  // BUG-3: CC64 sustain pedal state

    // Crosstalk: last block's per-voice outputs
    std::array<float, kMaxVoices> lastVoiceOutputs {};

    // Coupling state
    float couplingFilterMod = 0.0f;
    float couplingPitchMod = 0.0f;
    float couplingOrganMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    // Parameter pointers
    std::atomic<float>* paramOrgan        = nullptr;
    std::atomic<float>* paramCluster      = nullptr;
    std::atomic<float>* paramChiff        = nullptr;
    std::atomic<float>* paramDetune       = nullptr;
    std::atomic<float>* paramBuzz         = nullptr;
    std::atomic<float>* paramPressure     = nullptr;
    std::atomic<float>* paramCrosstalk    = nullptr;
    std::atomic<float>* paramFilterCutoff = nullptr;
    std::atomic<float>* paramFilterRes    = nullptr;
    std::atomic<float>* paramAttack       = nullptr;
    std::atomic<float>* paramDecay        = nullptr;
    std::atomic<float>* paramSustain      = nullptr;
    std::atomic<float>* paramRelease      = nullptr;
    std::atomic<float>* paramLfo1Rate     = nullptr;
    std::atomic<float>* paramLfo1Depth    = nullptr;
    std::atomic<float>* paramCompetition  = nullptr;
    std::atomic<float>* paramMacroA       = nullptr;
    std::atomic<float>* paramMacroB       = nullptr;
    std::atomic<float>* paramMacroC       = nullptr;
    std::atomic<float>* paramMacroD       = nullptr;
};

} // namespace xoceanus
