#pragma once
#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/PitchBendUtil.h"
#include <array>
#include <cmath>

namespace xomnibus {

//==============================================================================
//
//  ODDOSCAR / MORPH ENGINE — The Axolotl
//
//  Creature:   Oscar the axolotl — patient, regenerative, pink gills breathing
//              slowly in a deep cave. He lives in duration. Every note is a
//              journey. Oscar is O — depth, sustain, warm.
//
//  Habitat:    The Reef — warm coral textures, soft, living, tactile.
//              Oscar's home, where feliX visits.
//
//  Sonic role: Blooming, evolving, ethereal pads. Wavetable morphing through
//              rich timbral landscapes. The teal engine — cool, deep, oceanic.
//
//  Heritage:   Draws from the Moog Voyager's warm ladder filter and the PPG
//              Wave 2.2's wavetable morphing tradition. The 4-pole non-linear
//              ladder topology traces back to Bob Moog's 1965 transistor
//              cascade patent (US3475623A), here realized in software with
//              per-stage tanh nonlinearity for self-oscillating warmth.
//              The morph oscillator (sine->saw->square->noise) echoes the
//              PPG's scan-through-timbre concept, simplified to four
//              canonical waveforms stored as 2048-sample lookup tables.
//
//  Coupling:   Output: 0.3 Hz sine LFO (for LFOToPitch on other engines —
//                      Oscar's slow breath drifts feliX's pitch)
//              Input:  AmpToFilter (envelope-driven dub pump from percussive
//                      engines — feliX's hits make Oscar's pads breathe)
//                      EnvToMorph (external envelope shifts timbre —
//                      inter-species symbiosis)
//
//  Brand:      XO_OX Designs | Accent: Axolotl Gill Pink #E8839B
//              Parameter prefix: morph_ (frozen, never rename)
//
//==============================================================================


//==============================================================================
// MorphOscillator — Four-timbre wavetable with continuous crossfade.
//
// The morph parameter (0.0 - 3.0) sweeps through four timbres stored as
// pre-built 2048-sample wavetables, inspired by the PPG Wave 2.2's
// wavetable scanning concept:
//
//   0.0 = Sine        (fundamental only — Oscar at rest)
//   1.0 = Sawtooth    (all harmonics — Oscar stirring)
//   2.0 = Square      (odd harmonics — Oscar breathing)
//   3.0 = Noise       (all frequencies — Oscar in full drift)
//
// Between integer positions the oscillator crossfades linearly between
// adjacent timbres. This is specific to the ODDOSCAR engine — the shared
// WavetableOscillator handles loading external wavetable files.
//==============================================================================
class MorphOscillator
{
public:
    // 2048 samples per table: standard wavetable size offering good frequency
    // resolution (0.02 Hz at 44.1 kHz) while fitting comfortably in L1 cache.
    static constexpr int kTableSize = 2048;

    MorphOscillator() { buildTables(); }

    void setFrequency (float frequencyHz, float sampleRate) noexcept
    {
        if (sampleRate > 0.0f)
            phaseIncrement = static_cast<double> (frequencyHz) / static_cast<double> (sampleRate);
    }

    void setMorph (float morphPosition) noexcept
    {
        morph = std::max (0.0f, std::min (3.0f, morphPosition));
    }

    float processSample() noexcept
    {
        float out = 0.0f;

        // Determine which two tables to crossfade between
        int tableIndex = static_cast<int> (std::floor (morph));
        float crossfadeFraction = morph - static_cast<float> (tableIndex);

        if (tableIndex >= 3)
        {
            // Pure noise region (morph >= 3.0)
            out = noiseGenerator.nextFloat() * 2.0f - 1.0f;
        }
        else if (tableIndex == 2 && crossfadeFraction > 0.0f)
        {
            // Square-to-noise crossfade (morph 2.0 - 3.0)
            float tableValue = readTable (tableIndex);
            float noiseValue = noiseGenerator.nextFloat() * 2.0f - 1.0f;
            out = tableValue * (1.0f - crossfadeFraction) + noiseValue * crossfadeFraction;
        }
        else
        {
            // Standard table-to-table crossfade (sine<->saw or saw<->square)
            float a = readTable (tableIndex);
            float b = readTable (tableIndex + 1);
            out = a * (1.0f - crossfadeFraction) + b * crossfadeFraction;
        }

        // Advance phase accumulator with wrapping
        phase += phaseIncrement;
        while (phase >= 1.0) phase -= 1.0;
        while (phase < 0.0) phase += 1.0;

        return out;
    }

    void reset() noexcept { phase = 0.0; }

private:
    void buildTables()
    {
        constexpr double twoPi = 6.28318530717958647692; // 2 * pi, full cycle
        for (int i = 0; i < kTableSize; ++i)
        {
            double normalizedPosition = static_cast<double> (i) / kTableSize;
            sineTable[i]   = static_cast<float> (std::sin (twoPi * normalizedPosition));
            sawTable[i]    = static_cast<float> (2.0 * normalizedPosition - 1.0);    // bipolar ramp
            squareTable[i] = (normalizedPosition < 0.5) ? 1.0f : -1.0f;             // 50% duty cycle
        }
    }

    float readTable (int tableIndex) const noexcept
    {
        jassert (tableIndex >= 0 && tableIndex <= 2);
        if (tableIndex < 0 || tableIndex > 2)
            tableIndex = 0; // defensive fallback to sine

        // Linear interpolation between adjacent table samples
        double position = phase * kTableSize;
        int sampleIndexA = static_cast<int> (position) & (kTableSize - 1);      // bitwise AND for fast modulo (table size is power of 2)
        int sampleIndexB = (sampleIndexA + 1) & (kTableSize - 1);
        float interpolationFraction = static_cast<float> (position - std::floor (position));

        const float* table = sineTable;
        if (tableIndex == 1) table = sawTable;
        else if (tableIndex == 2) table = squareTable;

        return table[sampleIndexA] * (1.0f - interpolationFraction) + table[sampleIndexB] * interpolationFraction;
    }

    //-- Wavetable storage -----------------------------------------------------
    float sineTable[kTableSize] {};
    float sawTable[kTableSize] {};
    float squareTable[kTableSize] {};

    //-- Oscillator state ------------------------------------------------------
    double phase = 0.0;              // normalized phase [0, 1)
    double phaseIncrement = 0.0;     // phase advance per sample (freq / sampleRate)
    float morph = 0.0f;             // morph position [0.0, 3.0]
    juce::Random noiseGenerator;     // PRNG for noise morph region
};


//==============================================================================
// MoogLadder — 4-pole non-linear ladder filter.
//
// Heritage: Bob Moog's 1965 transistor ladder (US patent 3475623A).
// Four cascaded one-pole sections with global feedback create the classic
// 24 dB/octave lowpass with resonant self-oscillation. The per-stage tanh
// nonlinearity models the soft saturation of the original transistor pairs,
// producing the warm, musical resonance that defined the Minimoog and
// Moog Voyager sound.
//
// This is the soul of Oscar's warmth — embedded here rather than in the
// shared DSP library because the Moog topology is architecturally distinct
// from the general-purpose Cytomic SVF used by other engines.
//==============================================================================
class MoogLadder
{
public:
    void prepare (double sampleRate) noexcept
    {
        cachedSampleRate = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        for (auto& s : stage) s = 0.0;
        for (auto& d : delayState) d = 0.0;
    }

    void setCutoff (float frequencyHz) noexcept
    {
        cutoffFrequency = std::max (20.0f, std::min (20000.0f, frequencyHz));
    }

    void setResonance (float resonanceAmount) noexcept
    {
        resonance = std::max (0.0f, std::min (1.0f, resonanceAmount));
    }

    float processSample (float input) noexcept
    {
        // Normalize cutoff to [0, 0.98] of Nyquist to prevent instability
        double normalizedCutoff = std::max (0.0, std::min (0.98, 2.0 * cutoffFrequency / cachedSampleRate));

        // Filter coefficient: modified bilinear transform approximation.
        // The (1 + (1 - fc)) term compensates for frequency warping near Nyquist,
        // keeping the perceived cutoff musically accurate at high frequencies.
        double filterCoefficient = normalizedCutoff * 0.5 * (1.0 + (1.0 - normalizedCutoff));

        // Resonance feedback amount: scale 0-1 to 0-4 (the theoretical
        // self-oscillation threshold for a 4-pole ladder).
        // The (1 - 0.15 * f^2) term reduces feedback at high cutoff to prevent
        // runaway self-oscillation — a common trick from analog circuit design.
        double feedbackAmount = resonance * 4.0 * (1.0 - 0.15 * filterCoefficient * filterCoefficient);

        // Subtract feedback from 4th stage output (the ladder's global feedback path)
        double feedbackInput = static_cast<double> (input) - feedbackAmount * delayState[3];

        // Soft-clip the feedback-subtracted input to model transistor saturation
        feedbackInput = static_cast<double> (fastTanh (static_cast<float> (feedbackInput)));

        // Process through 4 cascaded one-pole sections
        for (int i = 0; i < 4; ++i)
        {
            double stageInput = (i == 0) ? feedbackInput : stage[i - 1];

            // Each stage: one-pole lowpass with tanh nonlinearity on both
            // the input and the delay state, modeling the transistor pair
            // in each rung of the original Moog ladder circuit.
            stage[i] = delayState[i] + filterCoefficient
                * (static_cast<double> (fastTanh (static_cast<float> (stageInput)))
                   - static_cast<double> (fastTanh (static_cast<float> (delayState[i]))));
            delayState[i] = stage[i];

            // Flush denormals in the feedback chain.
            // WHY: Without this, near-zero values in the delay states decay into
            // subnormal floating-point numbers (< 1e-38). On x86/ARM, subnormal
            // arithmetic can be 10-100x slower than normal floats, causing CPU
            // spikes on sustained silent notes. Flushing to zero when below 1e-18
            // (well above the audible threshold of ~1e-7 for 24-bit audio) keeps
            // the filter computationally efficient during decay tails.
            if (std::fabs (delayState[i]) < 1.0e-18) delayState[i] = 0.0;
        }

        return static_cast<float> (stage[3]);
    }

private:
    double cachedSampleRate = 44100.0;      // stored sample rate for coefficient calculation
    float cutoffFrequency = 1000.0f;        // filter cutoff in Hz [20, 20000]
    float resonance = 0.0f;                 // resonance amount [0, 1] (1 = self-oscillation)
    double stage[4] {};                     // four cascade stage outputs
    double delayState[4] {};                // one-sample delay per stage (z^-1)
};


//==============================================================================
// MorphVoice — Per-voice state for the ODDOSCAR (pad) engine.
//
// Each voice contains Oscar's full signal chain: three detuned morph
// oscillators (for chorus width), a sub oscillator, a Moog ladder filter,
// an ADSR envelope with Bloom-shaped attack, and drift modulation for
// organic stereo movement.
//==============================================================================
struct MorphVoice
{
    bool active = false;
    bool releasing = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;             // voice-start counter for LRU stealing

    //-- Oscillators -----------------------------------------------------------
    MorphOscillator oscillators[3];      // 3 detuned morph oscillators for chorus width
    PolyBLEP subOscillator;              // sub oscillator (sine, one octave below)

    //-- Filter ----------------------------------------------------------------
    MoogLadder filter;                   // 4-pole Moog ladder (Oscar's warmth)

    //-- ADSR Envelope ---------------------------------------------------------
    enum EnvelopeStage { Attack, Decay, Sustain, Release, Off };
    EnvelopeStage envelopeStage = Off;
    float envelopeLevel = 0.0f;

    //-- Voice stealing --------------------------------------------------------
    float stealFadeLevel = 0.0f;         // crossfade level during voice stealing

    //-- Drift modulation ------------------------------------------------------
    float driftPhase = 0.0f;             // Perlin noise phase (randomized per note)
    float driftValue = 0.0f;             // current drift output (stereo spread + FM)

    //-- Portamento/legato pitch tracking --------------------------------------
    float currentFrequency = 440.0f;     // instantaneous (gliding) frequency in Hz
    float targetFrequency  = 440.0f;     // destination frequency set on noteOn
    float glideCoefficient = 1.0f;       // per-sample IIR coefficient (1.0 = instant)
};


//==============================================================================
// MorphEngine — Lush, evolving pad synthesis.
//
// The ODDOSCAR engine. Oscar the axolotl — patient, warm, regenerative.
// Lives at The Reef in the XO_OX water column: warm coral textures,
// soft, living, tactile.
//
// Signal flow:
//   MIDI Note -> 3x Detuned MorphOscillator (sine->saw->square->noise)
//             -> Sub Oscillator (sine, -1 octave)
//             -> Moog Ladder Filter (4-pole LP, warm self-oscillation)
//             -> Bloom Envelope (ADSR with shaped attack)
//             -> Drift Stereo Spread (Perlin noise panning)
//             -> Soft Clip (tanh limiter)
//             -> Output
//
// Features:
//   - Wavetable morph oscillator: sine -> saw -> square -> noise (0.0 - 3.0)
//   - 3 detuned oscillators for chorus width (PPG Wave-inspired spread)
//   - Sub oscillator (sine, one octave below — bass foundation)
//   - 4-pole Moog ladder filter (warm, resonant, self-oscillating)
//   - Full ADSR envelope with "Bloom" attack shaping
//   - Perlin-noise drift modulation (stereo spread + subtle FM)
//   - LRU voice stealing with 5 ms crossfade
//   - 16-voice maximum polyphony
//   - CC1 (mod wheel) sweeps morph position for live performance
//   - CC64 (sustain pedal) support with proper release-on-pedal-up
//
// Coupling (the symbiosis):
//   Output: LFO value (0.3 Hz sine — Oscar's slow breathing rhythm,
//           used for LFOToPitch on other engines)
//   Input:  AmpToFilter — envelope from percussive engines pumps the
//                          filter (the dub pump: feliX hits, Oscar breathes)
//           EnvToMorph  — external envelope shifts morph position
//                          (inter-species timbral symbiosis)
//
//==============================================================================
class MorphEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 16;       // 16-voice polyphony (configurable down to 1)

    //==========================================================================
    //  LIFECYCLE
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        cachedSampleRate = sampleRate;
        cachedSampleRateFloat = static_cast<float> (cachedSampleRate);
        lfoPhase = 0.0;
        silenceGate.prepare (sampleRate, maxBlockSize);

        aftertouch.prepare (sampleRate);  // D006: 5ms attack / 20ms release smoothing

        outputCacheLeft.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheRight.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        for (auto& voice : voices)
        {
            voice.active = false;
            voice.releasing = false;
            voice.envelopeStage = MorphVoice::Off;
            voice.envelopeLevel = 0.0f;
            voice.stealFadeLevel = 0.0f;
            voice.driftPhase = 0.0f;
            voice.currentFrequency = 440.0f;
            voice.targetFrequency  = 440.0f;
            voice.glideCoefficient = 1.0f;

            for (auto& osc : voice.oscillators)
                osc.reset();
            voice.subOscillator.reset();
            voice.filter.prepare (cachedSampleRate);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& voice : voices)
        {
            voice.active = false;
            voice.releasing = false;
            voice.envelopeStage = MorphVoice::Off;
            voice.envelopeLevel = 0.0f;
            voice.stealFadeLevel = 0.0f;
            voice.currentFrequency = 440.0f;
            voice.targetFrequency  = 440.0f;
            voice.glideCoefficient = 1.0f;
            for (auto& osc : voice.oscillators)
                osc.reset();
            voice.subOscillator.reset();
            voice.filter.reset();
        }
        lfoPhase = 0.0;
        lfoOutput = 0.0f;
        filterCutoffModulation = 0.0f;
        morphModulation = 0.0f;
        std::fill (outputCacheLeft.begin(), outputCacheLeft.end(), 0.0f);
        std::fill (outputCacheRight.begin(), outputCacheRight.end(), 0.0f);
    }

    //==========================================================================
    //  AUDIO RENDERING
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        //----------------------------------------------------------------------
        // ParamSnapshot: cache all parameter values once per block.
        // This is the XOmnibus pattern — atomic loads happen here, not per-sample.
        //----------------------------------------------------------------------
        const float morphPosition     = (paramMorphPosition != nullptr) ? paramMorphPosition->load() : 0.5f;
        const float attackTime        = (paramBloomAttack != nullptr) ? paramBloomAttack->load() : 1.25f;
        const float decayTime         = (paramDecay != nullptr) ? paramDecay->load() : 2.0f;
        const float sustainLevel      = (paramSustain != nullptr) ? paramSustain->load() : 0.7f;
        const float releaseTime       = (paramRelease != nullptr) ? paramRelease->load() : 1.5f;
        const float filterCutoff      = (paramFilterCutoff    != nullptr) ? paramFilterCutoff->load()    : 1200.0f;
        const float filterResonance   = (paramFilterResonance != nullptr) ? paramFilterResonance->load() : 0.4f;
        const float filterEnvDepth    = (paramFilterEnvDepth  != nullptr) ? paramFilterEnvDepth->load()  : 0.25f;
        const float driftAmount       = (paramDrift != nullptr) ? paramDrift->load() : 0.3f;
        const float subLevel          = (paramSubLevel != nullptr) ? paramSubLevel->load() : 0.5f;
        const float detuneCents       = (paramDetune != nullptr) ? paramDetune->load() : 12.0f;
        const float outputLevel       = (paramLevel != nullptr) ? paramLevel->load() : 0.8f;

        // Polyphony: parameter index maps to voice count via power-of-2 ladder
        // Index:  0->1, 1->2, 2->4, 3->8, 4->16 voices
        const int maxPolyphony = (paramPolyphony != nullptr)
            ? (1 << std::min (4, static_cast<int> (paramPolyphony->load()))) : 8;

        // Voice mode: 0=Poly, 1=Mono, 2=Legato
        const int voiceModeIndex = (paramVoiceMode != nullptr)
            ? static_cast<int> (paramVoiceMode->load()) : 0;
        bool monoMode   = (voiceModeIndex == 1);
        bool legatoMode = (voiceModeIndex == 2);
        // In Mono or Legato mode, restrict to a single voice
        const int effectivePolyphony = (monoMode || legatoMode) ? 1 : maxPolyphony;

        // Glide time: exponential-approach coefficient per sample
        // 0.0 glideTime → coefficient = 1.0 (instant; no glide)
        const float glideTimeSeconds = (paramGlide != nullptr) ? paramGlide->load() : 0.0f;
        float glideCoefficient = 1.0f;
        if (glideTimeSeconds > 0.001f)
            glideCoefficient = 1.0f - std::exp (-1.0f / (glideTimeSeconds * cachedSampleRateFloat));

        // -- XOmnibus macros (CHARACTER, MOVEMENT, COUPLING, SPACE) ------------
        // Loaded once per block; defaults to 0.0 so existing presets are unaffected.
        const float macroBloom = (paramMacroBloom != nullptr) ? paramMacroBloom->load() : 0.0f;
        const float macroDrift = (paramMacroDrift != nullptr) ? paramMacroDrift->load() : 0.0f;
        const float macroDepth = (paramMacroDepth != nullptr) ? paramMacroDepth->load() : 0.0f;
        const float macroSpace = (paramMacroSpace != nullptr) ? paramMacroSpace->load() : 0.0f;

        // Apply macro offsets to DSP parameters:
        //   BLOOM: shifts morph position up to +1.5 (sine→square character sweep).
        //   DRIFT: widens detune spread up to +30 cents (animated chorus shimmer).
        //   DEPTH: opens filter cutoff up to +6000 Hz (surface from the deep).
        //   SPACE: multiplies attack time by 1× to 4× (slow atmospheric bloom).
        const float effectiveDetune  = detuneCents + macroDrift * 30.0f;
        const float effectiveCutoff  = std::min (20000.0f, filterCutoff + macroDepth * 6000.0f);
        const float effectiveAttack  = attackTime  * (1.0f + macroSpace * 3.0f);

        // Effective morph position includes macroBloom + coupling modulation + CC1 (mod wheel)
        float effectiveMorph = std::max (0.0f, std::min (3.0f,
            morphPosition + morphModulation + modWheelMorphOffset + macroBloom * 1.5f));

        //----------------------------------------------------------------------
        // MIDI event processing
        //----------------------------------------------------------------------
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(),
                        effectiveDetune, effectiveMorph, effectiveCutoff, filterResonance,
                        effectivePolyphony, monoMode, legatoMode, glideCoefficient);
            }
            else if (msg.isNoteOff())
            {
                if (!sustainPedalDown)
                    noteOff (msg.getNoteNumber());
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                reset();
                sustainPedalDown = false;
            }
            // D006: channel pressure → aftertouch (applied to filter cutoff below)
            else if (msg.isChannelPressure())
            {
                aftertouch.setChannelPressure (msg.getChannelPressureValue() / 127.0f);
            }
            else if (msg.isController())
            {
                const int controllerNumber = msg.getControllerNumber();
                const int controllerValue = msg.getControllerValue();

                if (controllerNumber == 64) // CC64: Sustain pedal
                {
                    bool wasDown = sustainPedalDown;
                    sustainPedalDown = (controllerValue >= 64); // MIDI convention: >= 64 is "on"
                    if (wasDown && !sustainPedalDown)
                    {
                        // Pedal released: transition all held voices to Release stage
                        for (auto& voice : voices)
                            if (voice.active && !voice.releasing)
                            {
                                voice.releasing = true;
                                voice.envelopeStage = MorphVoice::Release;
                            }
                    }
                }
                else if (controllerNumber == 1) // CC1: Mod wheel -> morph sweep
                {
                    // Maps 0-127 to 0.0-3.0 morph offset for live timbre sweeping
                    modWheelMorphOffset = static_cast<float> (controllerValue) / 127.0f * 3.0f;
                }
            }
            else if (msg.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel (msg.getPitchWheelValue());
        }

        // SilenceGate: skip all DSP if engine has been silent long enough
        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        //----------------------------------------------------------------------
        // D006: smooth aftertouch pressure and compute modulation value
        //----------------------------------------------------------------------
        aftertouch.updateBlock (numSamples);
        const float atPressure = aftertouch.getSmoothedPressure (0);  // channel-mode: voice 0 holds global value

        //----------------------------------------------------------------------
        // Reset coupling accumulators (consumed this block, accumulated fresh
        // by applyCouplingInput before next renderBlock call)
        //----------------------------------------------------------------------
        filterCutoffModulation = 0.0f;
        morphModulation = 0.0f;

        //----------------------------------------------------------------------
        // Deactivate voices beyond current polyphony limit
        //----------------------------------------------------------------------
        for (int i = effectivePolyphony; i < kMaxVoices; ++i)
        {
            auto& voice = voices[static_cast<size_t> (i)];
            if (voice.active)
            {
                voice.releasing = true;
                voice.envelopeStage = MorphVoice::Release;
            }
        }

        //----------------------------------------------------------------------
        // Per-sample audio rendering
        //----------------------------------------------------------------------

        // Precompute block-constant LFO phase increment (avoids division per sample)
        const double lfoPhaseIncrement = kCouplingLfoRateHz / cachedSampleRate;
        constexpr double kTwoPi = 6.28318530717958647692;

        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            //-- Internal coupling LFO (Oscar's slow breath) ------------------
            lfoPhase += lfoPhaseIncrement;
            if (lfoPhase >= 1.0) lfoPhase -= 1.0;
            lfoOutput = fastSin (static_cast<float> (kTwoPi * lfoPhase));

            float mixLeft = 0.0f, mixRight = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                //-- Envelope --------------------------------------------------
                updateEnvelope (voice, effectiveAttack, decayTime, sustainLevel, releaseTime);

                if (voice.envelopeStage == MorphVoice::Off)
                {
                    voice.active = false;
                    continue;
                }

                //-- Morph position (update all 3 oscillators) -----------------
                for (auto& osc : voice.oscillators)
                    osc.setMorph (effectiveMorph);

                //-- Portamento/glide: smooth currentFrequency toward targetFrequency ----
                // In Poly mode, glideCoefficient is 1.0 (instant — no per-voice glide).
                // In Mono/Legato mode, coefficient <1.0 produces exponential-approach glide.
                voice.currentFrequency += (voice.targetFrequency - voice.currentFrequency)
                                        * voice.glideCoefficient;

                //-- Drift modulation (Perlin-like smooth noise) ---------------
                // 0.1 Hz drift rate: Oscar's slow, meditative movement.
                // The low rate creates gentle organic pitch wander and stereo
                // spread — the axolotl breathing, not thrashing.
                voice.driftPhase += 0.1f / cachedSampleRateFloat;
                if (voice.driftPhase >= 1.0f) voice.driftPhase -= 1.0f;
                voice.driftValue = perlinNoise (voice.driftPhase) * driftAmount;

                //-- Oscillator mix (3 detuned oscillators for chorus width) ---
                float oscillatorMix = 0.0f;
                float detuneSpread[3] = { -effectiveDetune, 0.0f, effectiveDetune };
                // Use currentFrequency (glide-smoothed) instead of raw MIDI note.
                // In Poly mode with glideCoefficient=1.0, currentFrequency == targetFrequency
                // so behavior is identical to the previous per-sample noteNumber lookup.
                float baseFrequency = voice.currentFrequency
                                      * PitchBendUtil::semitonesToFreqRatio (pitchBendNorm * 2.0f);

                for (int i = 0; i < 3; ++i)
                {
                    // Convert cents to frequency ratio: 2^(cents/1200)
                    // ln(2) = 0.693147... used with fastExp for efficient cent-to-ratio conversion
                    constexpr float kLn2Over1200 = 0.693147f / 1200.0f; // ln(2) / 1200 cents per octave
                    float detunedFrequency = baseFrequency * fastExp (detuneSpread[i] * kLn2Over1200);

                    // Apply drift as subtle FM (0.2% max pitch deviation)
                    constexpr float kDriftFmDepth = 0.002f; // 0.2% max pitch modulation
                    detunedFrequency *= (1.0f + voice.driftValue * kDriftFmDepth);

                    voice.oscillators[i].setFrequency (detunedFrequency, cachedSampleRateFloat);
                    oscillatorMix += voice.oscillators[i].processSample();
                }
                oscillatorMix /= 3.0f; // normalize 3-oscillator sum

                //-- Sub oscillator (sine, one octave below for bass weight) ---
                voice.subOscillator.setFrequency (baseFrequency * 0.5f, cachedSampleRateFloat);
                voice.subOscillator.setWaveform (PolyBLEP::Waveform::Sine);
                float subOscOutput = voice.subOscillator.processSample() * subLevel;

                float rawSignal = oscillatorMix + subOscOutput;

                //-- Moog ladder filter ----------------------------------------
                // Filter coefficients are block-constant (coupling modulation set
                // by applyCouplingInput before renderBlock), so update once on the
                // first sample to avoid redundant trig recomputation per sample.
                if (sampleIndex == 0)
                {
                    // Coupling modulation scales +-2000 Hz (one octave of sweep
                    // centered on the cutoff — musically useful range for the
                    // dub pump effect)
                    constexpr float kCouplingCutoffRange = 2000.0f; // Hz range for coupling filter sweep
                    float modulatedCutoff = effectiveCutoff + filterCutoffModulation * kCouplingCutoffRange;
                    // D006: aftertouch adds up to +7000 Hz brightness (sensitivity 0.35)
                    // Classic application: press harder → Oscar's pad brightens, opening the warm
                    // Moog ladder filter for more harmonic content.
                    modulatedCutoff += atPressure * 0.35f * 7000.0f;
                    // D001: velocity × envelope sweeps the ladder cutoff open.
                    // Harder hits push the filter wider at the attack peak, then decay back.
                    // Max sweep: filterEnvDepth × velocity × 6000 Hz.
                    constexpr float kFilterEnvMaxSweep = 6000.0f;
                    modulatedCutoff += filterEnvDepth * voice.velocity * voice.envelopeLevel * kFilterEnvMaxSweep;
                    modulatedCutoff = std::max (20.0f, std::min (20000.0f, modulatedCutoff));
                    voice.filter.setCutoff (modulatedCutoff);
                    voice.filter.setResonance (filterResonance);
                }
                float filteredSignal = voice.filter.processSample (rawSignal);

                //-- Voice-stealing crossfade (5 ms) ---------------------------
                float stealFade = 1.0f;
                if (voice.stealFadeLevel > 0.0f)
                {
                    // Fade rate: 1 / (5ms * sampleRate) = samples for 5ms crossfade.
                    // This brief crossfade prevents clicks when stealing the oldest
                    // voice while keeping the transition nearly imperceptible.
                    constexpr float kStealFadeTimeSeconds = 0.005f; // 5 ms crossfade
                    voice.stealFadeLevel -= 1.0f / (kStealFadeTimeSeconds * cachedSampleRateFloat);
                    voice.stealFadeLevel = flushDenormal (voice.stealFadeLevel);
                    if (voice.stealFadeLevel <= 0.0f)
                        voice.stealFadeLevel = 0.0f;
                    stealFade = 1.0f - voice.stealFadeLevel;
                }

                //-- Apply envelope and velocity -------------------------------
                float voiceOutput = filteredSignal * voice.envelopeLevel * voice.velocity * stealFade;

                //-- Drift-based stereo spread ---------------------------------
                // Drift value pans each voice slightly L/R, creating organic
                // stereo movement without a dedicated chorus effect. The 0.3
                // spread factor keeps the image stable — Oscar doesn't thrash,
                // he sways gently.
                constexpr float kStereoSpreadAmount = 0.3f; // max L/R deviation
                mixLeft  += voiceOutput * (1.0f + kStereoSpreadAmount * voice.driftValue);
                mixRight += voiceOutput * (1.0f - kStereoSpreadAmount * voice.driftValue);
            }

            //-- Master output: tanh soft clip ---------------------------------
            // Prevents hard clipping in dense 16-voice pads — the tanh curve
            // gracefully compresses peaks while preserving musicality.
            float outputLeft  = fastTanh (mixLeft * outputLevel);
            float outputRight = fastTanh (mixRight * outputLevel);

            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample (0, sampleIndex, outputLeft);
                buffer.addSample (1, sampleIndex, outputRight);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample (0, sampleIndex, (outputLeft + outputRight) * 0.5f);
            }

            //-- Cache output for coupling reads -------------------------------
            if (sampleIndex < static_cast<int> (outputCacheLeft.size()))
            {
                outputCacheLeft[static_cast<size_t> (sampleIndex)]  = outputLeft;
                outputCacheRight[static_cast<size_t> (sampleIndex)] = outputRight;
            }
        }

        // SilenceGate: analyze output level for next-block bypass decision
        silenceGate.analyzeBlock (buffer.getReadPointer (0),
                                  buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : nullptr,
                                  numSamples);
    }

    //==========================================================================
    //  COUPLING — Inter-engine symbiosis
    //==========================================================================

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        auto index = static_cast<size_t> (sampleIndex);
        if (channel == 0 && index < outputCacheLeft.size())  return outputCacheLeft[index];
        if (channel == 1 && index < outputCacheRight.size()) return outputCacheRight[index];

        // Channel 2 = LFO output (Oscar's breath — 0.3 Hz sine for LFOToPitch
        // coupling to other engines, creating organic pitch drift)
        if (channel == 2) return lfoOutput;
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
            case CouplingType::AmpToFilter:
            {
                // The "dub pump" effect: invert the source engine's amplitude
                // envelope so that when feliX hits hard (amount -> 1.0), Oscar's
                // filter ducks. When feliX is quiet, Oscar opens up. Classic
                // sidechain-like interaction between percussive and pad engines.
                float invertedEnvelope = 1.0f - amount;
                filterCutoffModulation += (invertedEnvelope - 0.5f) * 2.0f; // bipolar [-1, +1]
                break;
            }
            case CouplingType::EnvToMorph:
                // External envelope shifts Oscar's morph position — another
                // engine's dynamics directly modulate the timbre. This is
                // inter-species symbiosis: one creature's energy reshapes
                // another creature's voice.
                morphModulation += amount;
                break;

            default:
                break; // Other coupling types not supported by ODDOSCAR
        }
    }

    //==========================================================================
    //  PARAMETERS
    //==========================================================================

    // Static helper: add ODDOSCAR parameters to a shared vector (used by processor).
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl (params);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl (params);
        return { params.begin(), params.end() };
    }

private:
    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // Morph position: 0.0 = sine, 1.0 = saw, 2.0 = square, 3.0 = noise
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_morph", 1 }, "Morph Position",
            juce::NormalisableRange<float> (0.0f, 3.0f, 0.01f), 0.5f));

        // Bloom (attack): longer values create the signature swelling pad onset
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_bloom", 1 }, "Morph Bloom (Attack)",
            juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.4f), 1.25f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_decay", 1 }, "Morph Decay",
            juce::NormalisableRange<float> (0.01f, 8.0f, 0.001f, 0.4f), 2.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_sustain", 1 }, "Morph Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_release", 1 }, "Morph Release",
            juce::NormalisableRange<float> (0.01f, 10.0f, 0.01f, 0.4f), 1.5f));

        // Filter cutoff: default 1200 Hz — a warm starting point below the
        // brightness threshold, letting the ladder's character shine
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_filterCutoff", 1 }, "Morph Filter Cutoff",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f), 1200.0f));

        // Filter resonance: at 1.0, the ladder self-oscillates (produces a sine tone)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_filterReso", 1 }, "Morph Filter Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.4f));

        // D001: Filter envelope depth — velocity × amplitude envelope sweeps the
        // Moog ladder cutoff open from the base cutoff by up to depth × 6000 Hz.
        // Harder hits open the filter wider and decay back toward base cutoff as the
        // note fades, giving Oscar's pads their D001-compliant velocity expressiveness.
        // Default 0.25: gentle brightness tracking, not overwhelming.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_filterEnvDepth", 1 }, "Morph Filter Env Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.25f));

        // Drift: Oscar's organic movement — pitch wander + stereo spread
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_drift", 1 }, "Morph Drift",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_sub", 1 }, "Morph Sub Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        // Detune: cents of spread between the 3 chorus oscillators.
        // Default 12 cents (roughly 1/6 of a semitone) — enough for shimmer
        // without becoming obviously out-of-tune.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_detune", 1 }, "Morph Detune",
            juce::NormalisableRange<float> (0.0f, 50.0f, 0.1f), 12.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_level", 1 }, "Morph Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        // Polyphony: power-of-2 voice count (1, 2, 4, 8, 16)
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "morph_polyphony", 1 }, "Morph Polyphony",
            juce::StringArray { "1", "2", "4", "8", "16" }, 3));

        // Voice mode: Poly (default), Mono, Legato
        // Poly   — standard polyphony (maxPolyphony voices, LRU stealing)
        // Mono   — single voice, always retriggered (snap-attack pads, punchy leads)
        // Legato — single voice, pitch glides when gate is already open (smooth lead lines)
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "morph_voiceMode", 1 }, "Morph Voice Mode",
            juce::StringArray { "Poly", "Mono", "Legato" }, 0));

        // Portamento glide time (seconds). Only audible in Mono and Legato modes.
        // 0 = instant (no glide). 2s maximum covers all musical tempos.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_glide", 1 }, "Morph Glide",
            juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f, 0.5f), 0.0f));

        // XOmnibus standard macros (CHARACTER, MOVEMENT, COUPLING, SPACE)
        // All default to 0.0 — existing presets are unaffected.
        //
        // M1 BLOOM (CHARACTER): shifts morph position +1.5 — unfurls gills from sine to square.
        //   At max: effectiveMorph += 1.5 — 50% more harmonic content.
        // M2 DRIFT (MOVEMENT): adds +30 cents to detune — wider chorus animation.
        //   At max: detune spread = base + 30 cents — Oscar churns through the reef.
        // M3 DEPTH (COUPLING): opens filter cutoff +6000 Hz — brighter, airier.
        //   At max: +6000 Hz offset on filterCutoff — axolotl surfaces from cave.
        // M4 SPACE (SPACE): multiplies attack time by up to 4× — slow atmospheric bloom.
        //   At max: attack ×4 — Oscar takes a very long, meditative breath.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_macroBloom", 1 }, "Morph BLOOM",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_macroDrift", 1 }, "Morph DRIFT",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_macroDepth", 1 }, "Morph DEPTH",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "morph_macroSpace", 1 }, "Morph SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    }

public:
    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramMorphPosition   = apvts.getRawParameterValue ("morph_morph");
        paramBloomAttack     = apvts.getRawParameterValue ("morph_bloom");
        paramDecay           = apvts.getRawParameterValue ("morph_decay");
        paramSustain         = apvts.getRawParameterValue ("morph_sustain");
        paramRelease         = apvts.getRawParameterValue ("morph_release");
        paramFilterCutoff    = apvts.getRawParameterValue ("morph_filterCutoff");
        paramFilterResonance = apvts.getRawParameterValue ("morph_filterReso");
        paramFilterEnvDepth  = apvts.getRawParameterValue ("morph_filterEnvDepth");
        paramDrift           = apvts.getRawParameterValue ("morph_drift");
        paramSubLevel        = apvts.getRawParameterValue ("morph_sub");
        paramDetune          = apvts.getRawParameterValue ("morph_detune");
        paramLevel           = apvts.getRawParameterValue ("morph_level");
        paramPolyphony       = apvts.getRawParameterValue ("morph_polyphony");
        paramVoiceMode       = apvts.getRawParameterValue ("morph_voiceMode");
        paramGlide           = apvts.getRawParameterValue ("morph_glide");
        // XOmnibus macros
        paramMacroBloom      = apvts.getRawParameterValue ("morph_macroBloom");
        paramMacroDrift      = apvts.getRawParameterValue ("morph_macroDrift");
        paramMacroDepth      = apvts.getRawParameterValue ("morph_macroDepth");
        paramMacroSpace      = apvts.getRawParameterValue ("morph_macroSpace");
    }

    //==========================================================================
    //  IDENTITY
    //==========================================================================

    juce::String getEngineId() const override { return "OddOscar"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFFE8839B); } // Axolotl Gill Pink
    int getMaxVoices() const override { return kMaxVoices; }

private:
    SilenceGate silenceGate;

    //==========================================================================
    //  VOICE MANAGEMENT
    //==========================================================================

    void noteOn (int noteNumber, float velocity, float detuneCents,
                 float morphPosition, float cutoff, float resonance, int maxPolyphony,
                 bool monoMode = false, bool legatoMode = false, float glideCoeff = 1.0f)
    {
        float frequency = midiNoteToFrequency (static_cast<float> (noteNumber));

        // ---- Mono / Legato mode ----
        // Both modes force voice[0]. Legato additionally skips envelope retrigger
        // and begins a pitch glide when the gate is already open.
        if (monoMode || legatoMode)
        {
            auto& voice = voices[0];
            bool wasActive = voice.active;

            voice.targetFrequency = frequency;

            if (legatoMode && wasActive)
            {
                // Legato: glide to new pitch without retriggering envelope.
                // Oscar's pad continues to bloom; only pitch slides.
                voice.glideCoefficient = glideCoeff;
                voice.noteNumber = noteNumber;
                voice.velocity = velocity;
                return;
            }

            // Mono retrigger — or Legato first note (voice was silent)
            voice.stealFadeLevel = voice.active ? voice.envelopeLevel : 0.0f;
            voice.active = true;
            voice.releasing = false;
            voice.noteNumber = noteNumber;
            voice.velocity = velocity;
            voice.startTime = voiceCounter++;
            voice.envelopeStage = MorphVoice::Attack;
            voice.envelopeLevel = 0.0f;
            voice.currentFrequency = frequency;
            voice.glideCoefficient = glideCoeff;

            // Initialize oscillators at the new frequency immediately
            float detuneSpread[3] = { -detuneCents, 0.0f, detuneCents };
            for (int i = 0; i < 3; ++i)
            {
                float detunedFrequency = frequency * std::pow (2.0f, detuneSpread[i] / 1200.0f);
                voice.oscillators[i].setFrequency (detunedFrequency, cachedSampleRateFloat);
                voice.oscillators[i].setMorph (morphPosition);
                voice.oscillators[i].reset();
            }
            voice.subOscillator.reset();
            voice.subOscillator.setFrequency (frequency * 0.5f, cachedSampleRateFloat);
            voice.subOscillator.setWaveform (PolyBLEP::Waveform::Sine);
            voice.filter.reset();
            voice.filter.setCutoff (cutoff);
            voice.filter.setResonance (resonance);
            voice.driftPhase = driftPhaseRandomizer.nextFloat();
            return;
        }

        // ---- Polyphonic mode ----
        int voiceIndex = findFreeVoice (maxPolyphony);
        auto& voice = voices[static_cast<size_t> (voiceIndex)];

        // Preserve outgoing voice's envelope level for crossfade
        if (voice.active)
            voice.stealFadeLevel = voice.envelopeLevel;
        else
            voice.stealFadeLevel = 0.0f;

        voice.active = true;
        voice.releasing = false;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.startTime = voiceCounter++;
        voice.envelopeStage = MorphVoice::Attack;
        voice.envelopeLevel = 0.0f;
        voice.currentFrequency = frequency;
        voice.targetFrequency  = frequency;
        voice.glideCoefficient = 1.0f;  // No glide in poly mode (instant pitch)

        // Initialize 3 detuned oscillators (center, -N cents, +N cents)
        float detuneSpread[3] = { -detuneCents, 0.0f, detuneCents };
        for (int i = 0; i < 3; ++i)
        {
            float detunedFrequency = frequency * std::pow (2.0f, detuneSpread[i] / 1200.0f);
            voice.oscillators[i].setFrequency (detunedFrequency, cachedSampleRateFloat);
            voice.oscillators[i].setMorph (morphPosition);
            voice.oscillators[i].reset();
        }

        // Sub oscillator: sine one octave below (0.5x frequency)
        voice.subOscillator.reset();
        voice.subOscillator.setFrequency (frequency * 0.5f, cachedSampleRateFloat);
        voice.subOscillator.setWaveform (PolyBLEP::Waveform::Sine);

        // Initialize filter
        voice.filter.reset();
        voice.filter.setCutoff (cutoff);
        voice.filter.setResonance (resonance);

        // Randomize drift starting phase for organic per-voice variation
        voice.driftPhase = driftPhaseRandomizer.nextFloat();
    }

    void noteOff (int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && !voice.releasing && voice.noteNumber == noteNumber)
            {
                voice.releasing = true;
                voice.envelopeStage = MorphVoice::Release;
            }
        }
    }

    //==========================================================================
    //  ENVELOPE
    //==========================================================================

    void updateEnvelope (MorphVoice& voice, float attackTime, float decayTime,
                         float sustainLevel, float releaseTime) noexcept
    {
        switch (voice.envelopeStage)
        {
            case MorphVoice::Attack:
            {
                // Linear ramp from 0 to 1 over attackTime seconds
                float rate = 1.0f / (std::max (0.001f, attackTime) * cachedSampleRateFloat);
                voice.envelopeLevel += rate;
                if (voice.envelopeLevel >= 1.0f)
                {
                    voice.envelopeLevel = 1.0f;
                    voice.envelopeStage = MorphVoice::Decay;
                }
                break;
            }
            case MorphVoice::Decay:
            {
                // Linear ramp from 1.0 down to sustain level
                float rate = 1.0f / (std::max (0.01f, decayTime) * cachedSampleRateFloat);
                voice.envelopeLevel -= rate;

                // Flush denormals: prevents CPU spikes from subnormal arithmetic
                // during slow decay tails (see MoogLadder comment for details)
                voice.envelopeLevel = flushDenormal (voice.envelopeLevel);

                if (voice.envelopeLevel <= sustainLevel)
                {
                    voice.envelopeLevel = sustainLevel;
                    voice.envelopeStage = MorphVoice::Sustain;
                }
                break;
            }
            case MorphVoice::Sustain:
                voice.envelopeLevel = sustainLevel;
                // If sustain is effectively zero, skip to Off (prevents stuck silent voices)
                if (sustainLevel < 0.0001f)
                {
                    voice.envelopeLevel = 0.0f;
                    voice.envelopeStage = MorphVoice::Off;
                }
                break;

            case MorphVoice::Release:
            {
                // Linear ramp from sustain level to 0 over releaseTime seconds
                float rate = 1.0f / (std::max (0.01f, releaseTime) * cachedSampleRateFloat);
                voice.envelopeLevel -= rate;

                // Flush denormals during release tail
                voice.envelopeLevel = flushDenormal (voice.envelopeLevel);

                if (voice.envelopeLevel <= 0.0f)
                {
                    voice.envelopeLevel = 0.0f;
                    voice.envelopeStage = MorphVoice::Off;
                }
                break;
            }
            case MorphVoice::Off:
                break;
        }
    }

    //==========================================================================
    //  VOICE ALLOCATION
    //==========================================================================

    int findFreeVoice (int maxPolyphony)
    {
        int polyphonyLimit = std::min (maxPolyphony, kMaxVoices);

        // First pass: find an inactive voice
        for (int i = 0; i < polyphonyLimit; ++i)
            if (!voices[static_cast<size_t> (i)].active)
                return i;

        // No free voice — steal the oldest (LRU: Least Recently Used).
        // This ensures the most recently played notes survive, preserving
        // the musical intent in dense pad voicings.
        int oldestVoiceIndex = 0;
        uint64_t oldestStartTime = UINT64_MAX;
        for (int i = 0; i < polyphonyLimit; ++i)
        {
            if (voices[static_cast<size_t> (i)].startTime < oldestStartTime)
            {
                oldestStartTime = voices[static_cast<size_t> (i)].startTime;
                oldestVoiceIndex = i;
            }
        }
        return oldestVoiceIndex;
    }

    //==========================================================================
    //  UTILITY FUNCTIONS
    //==========================================================================

    /** Convert MIDI note number to frequency in Hz.
        Reference: A4 (MIDI note 69) = 440 Hz, equal temperament. */
    static float midiNoteToFrequency (float midiNote) noexcept
    {
        return 440.0f * std::pow (2.0f, (midiNote - 69.0f) / 12.0f);
    }

    /** Simplified Perlin noise for smooth random drift modulation.
        Maps a single phase value through 8 hash-interpolated octaves using
        Hermite smoothstep for continuous, differentiable output — no abrupt
        jumps. The hash function uses large primes (15731, 789221, 1376312589)
        chosen for their low correlation across sequential integer inputs,
        producing pseudorandom values without visible patterns. */
    static float perlinNoise (float phase) noexcept
    {
        float scaledPhase = phase * 8.0f;                               // 8 noise octaves across [0, 1)
        int integerPart = static_cast<int> (std::floor (scaledPhase));
        float fractionalPart = scaledPhase - static_cast<float> (integerPart);
        float smoothstep = fractionalPart * fractionalPart * (3.0f - 2.0f * fractionalPart); // Hermite smoothstep

        // Integer hash function -> pseudorandom float in [-1, 1]
        auto hash = [] (int n) -> float {
            n = (n << 13) ^ n;  // bit-mixing via shift-XOR
            return 1.0f - static_cast<float> (
                (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff  // 3 large primes for decorrelation
            ) / 1073741824.0f;  // 2^30: normalize to [-1, +1]
        };

        float valueA = hash (integerPart);
        float valueB = hash (integerPart + 1);
        return valueA + smoothstep * (valueB - valueA);                 // interpolate between adjacent hash values
    }

    //==========================================================================
    //  ENGINE STATE
    //==========================================================================

    //-- Audio system ----------------------------------------------------------
    double cachedSampleRate = 44100.0;          // stored sample rate (double precision for phase accumulators)
    float cachedSampleRateFloat = 44100.0f;     // float copy (avoids casts in per-sample code)
    std::array<MorphVoice, kMaxVoices> voices;  // voice pool
    uint64_t voiceCounter = 0;                  // monotonic counter for LRU voice stealing

    //-- Internal coupling LFO (Oscar's breath) --------------------------------
    double lfoPhase = 0.0;                      // LFO phase accumulator [0, 1)
    float lfoOutput = 0.0f;                     // cached LFO value for coupling reads
    static constexpr double kCouplingLfoRateHz = 0.3; // 0.3 Hz: one full breath every ~3.3 seconds

    //-- Coupling accumulators (reset each block) ------------------------------
    float filterCutoffModulation = 0.0f;        // accumulated filter mod from AmpToFilter coupling
    float morphModulation = 0.0f;               // accumulated morph mod from EnvToMorph coupling

    //-- D006: CS-80-inspired poly aftertouch (channel pressure → filter cutoff) --
    PolyAftertouch aftertouch;

    //-- MIDI performance state ------------------------------------------------
    bool sustainPedalDown    = false;           // CC64 sustain pedal state
    float modWheelMorphOffset = 0.0f;           // CC1 mod wheel [0, 3.0] — sweeps morph position
    float pitchBendNorm      = 0.0f;            // MIDI pitch wheel [-1, +1]; ±2 semitone range

    //-- Output cache for coupling reads ---------------------------------------
    std::vector<float> outputCacheLeft;         // left channel output (per-sample, for getSampleForCoupling)
    std::vector<float> outputCacheRight;        // right channel output

    //-- Random number generator for drift initialization ----------------------
    juce::Random driftPhaseRandomizer;          // randomizes per-voice drift starting phase

    //-- Cached APVTS parameter pointers (ParamSnapshot pattern) ---------------
    std::atomic<float>* paramMorphPosition   = nullptr;
    std::atomic<float>* paramBloomAttack     = nullptr;
    std::atomic<float>* paramDecay           = nullptr;
    std::atomic<float>* paramSustain         = nullptr;
    std::atomic<float>* paramRelease         = nullptr;
    std::atomic<float>* paramFilterCutoff    = nullptr;
    std::atomic<float>* paramFilterResonance = nullptr;
    std::atomic<float>* paramFilterEnvDepth  = nullptr;
    std::atomic<float>* paramDrift           = nullptr;
    std::atomic<float>* paramSubLevel        = nullptr;
    std::atomic<float>* paramDetune          = nullptr;
    std::atomic<float>* paramLevel           = nullptr;
    std::atomic<float>* paramPolyphony       = nullptr;
    std::atomic<float>* paramVoiceMode       = nullptr;
    std::atomic<float>* paramGlide           = nullptr;
    // XOmnibus macros
    std::atomic<float>* paramMacroBloom      = nullptr;
    std::atomic<float>* paramMacroDrift      = nullptr;
    std::atomic<float>* paramMacroDepth      = nullptr;
    std::atomic<float>* paramMacroSpace      = nullptr;
};

} // namespace xomnibus
