#pragma once
#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/VoiceAllocator.h"
#include <array>
#include <cmath>

namespace xolokun {

//==============================================================================
//
//  ODDFELIX ENGINE — "The Neon Tetra"
//
//  Creature Identity:
//    feliX is the neon tetra — a flash of electric scales darting through
//    sunlit shallows. He lives at the surface of the XO_OX water column,
//    where light is bright, transients are sharp, and every note is an
//    arrival, not a journey. He is pure X: surface, transient, rhythmic.
//
//  Synth Lineage:
//    Channels the transient-sculpting spirit of Mutable Instruments Rings
//    (Karplus-Strong physical modeling), the FM percussion of Yamaha DX7
//    electric pianos and Elektron Digitone drum patches, and the snappy
//    attack shaping of vintage Roland CR-78 / TR-808 percussion circuits.
//    The pitch-snap sweep descends from analog drum synths where a VCO
//    sweeps rapidly down from a high frequency to create the "snap" of
//    a kick or tom attack.
//
//  Water Column Position:
//    THE SURFACE — feliX himself. The first creature. The primordial X.
//    Coupling with Oscar-depth engines (OVERBITE, OUROBOROS) creates
//    vertical tension spanning the full water column.
//
//  Architecture:
//    3 oscillator modes (Sine+Noise, FM, Karplus-Strong) feed a pitch-snap
//    sweep into a tanh waveshaper, through an HPF->BPF filter cascade
//    (Cytomic SVF), shaped by a percussive decay envelope with no sustain.
//    Unison spread (1/2/4 sub-voices) adds stereo width. LRU voice stealing
//    with 5ms crossfade prevents clicks.
//
//  Accent: Neon Tetra Blue #00A6D6
//  Parameter prefix: snap_
//
//==============================================================================

//==============================================================================
//
//  KarplusStrongOscillator
//
//  A delay-line excitation model with damped feedback, implementing the
//  Karplus-Strong algorithm (1983). A burst of white noise fills a delay
//  line whose length sets the pitch; each cycle the signal passes through
//  a simple averaging lowpass filter that gradually removes high frequencies,
//  simulating the energy decay of a plucked or struck string.
//
//  Heritage: This is the same physical modeling principle used in
//  Mutable Instruments Rings ("sympathetic strings" mode) and the
//  Yamaha VL1 virtual acoustic synthesizer.
//
//==============================================================================
class KarplusStrongOscillator
{
public:
    static constexpr int kMaxDelaySamples = 4096;

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        std::fill (delayBuffer.begin(), delayBuffer.end(), 0.0f);
        writePosition = 0;
        previousOutput = 0.0f;
    }

    void setFrequency (double frequencyHz) noexcept
    {
        if (frequencyHz > 0.0 && sampleRate > 0.0)
        {
            // Delay length = sampleRate / frequency. This sets the fundamental
            // pitch of the resonating string model.
            delaySamples = std::max (2.0, std::min (
                static_cast<double> (kMaxDelaySamples - 1),
                sampleRate / frequencyHz));
        }
    }

    void setDamping (float newDamping) noexcept
    {
        damping = std::max (0.0f, std::min (1.0f, newDamping));
    }

    void trigger() noexcept
    {
        // Fill the delay line with white noise — the "excitation" that
        // the string model will filter into a pitched tone as it decays.
        int delayLength = std::min (static_cast<int> (delaySamples), kMaxDelaySamples);
        for (int i = 0; i < delayLength; ++i)
            delayBuffer[static_cast<size_t> (i)] = randomGenerator.nextFloat() * 2.0f - 1.0f;
        writePosition = 0;
    }

    float nextSample() noexcept
    {
        int delayLength = static_cast<int> (delaySamples);
        if (delayLength < 2) return 0.0f;

        // Guard against writePosition exceeding delay length when
        // setFrequency() shortened the delay mid-playback
        if (writePosition >= delayLength)
            writePosition = writePosition % delayLength;

        int readPosition = writePosition;
        int nextPosition = (readPosition + 1) % delayLength;

        float output = delayBuffer[static_cast<size_t> (readPosition)];

        // Two-point averaging lowpass: the core of Karplus-Strong.
        // Each cycle around the delay line, high frequencies are attenuated
        // by averaging adjacent samples, simulating string damping.
        float averaged = 0.5f * (output + delayBuffer[static_cast<size_t> (nextPosition)]);

        // One-pole smoothing controlled by damping parameter:
        // Higher damping = faster high-frequency decay = duller string tone
        float filtered = previousOutput + damping * (averaged - previousOutput);

        // Flush denormals to zero: feedback paths accumulate tiny sub-normal
        // floating-point values that waste CPU cycles on software emulation
        // of IEEE 754 gradual underflow. Forcing these to zero keeps the
        // FPU in fast hardware mode.
        previousOutput = flushDenormal (filtered);

        delayBuffer[static_cast<size_t> (writePosition)] = previousOutput;
        writePosition = (writePosition + 1) % delayLength;

        return output;
    }

    void reset() noexcept
    {
        std::fill (delayBuffer.begin(), delayBuffer.end(), 0.0f);
        writePosition = 0;
        previousOutput = 0.0f;
    }

private:
    std::array<float, kMaxDelaySamples> delayBuffer {};
    double sampleRate = 44100.0;
    double delaySamples = 100.0;
    float damping = 0.5f;
    float previousOutput = 0.0f;
    int writePosition = 0;
    juce::Random randomGenerator;
};


//==============================================================================
//
//  SnapVoice — Per-voice state for the ODDFELIX (Snap) engine.
//
//  Each voice holds its own oscillator instances, filter cascade, envelope
//  level, and unison spread parameters. The voice is inherently percussive:
//  there is no sustain stage — only a decay from peak to silence.
//
//==============================================================================
struct SnapVoice
{
    // --- Voice lifecycle ---
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;

    // --- Oscillators ---
    PolyBLEP sineOscillator;           // Used for Sine+Noise mode (carrier) and FM mode (carrier)
    PolyBLEP noiseOscillator;          // High-frequency saw used as noise-like source
    PolyBLEP fmModulatorOscillator;    // FM mode modulator oscillator
    KarplusStrongOscillator karplusStrongOscillators[4];  // One per unison sub-voice

    // --- Filter cascade (HPF -> BPF) ---
    CytomicSVF highPassFilter;
    CytomicSVF bandPassFilter;

    // --- Envelope (AD shape: instant attack, exponential decay to zero) ---
    StandardADSR ampEnv;

    // --- Voice stealing crossfade ---
    float fadeOutLevel = 0.0f;

    // --- Unison spread ---
    float detuneOffsets[4] = {};       // Cents offset per sub-voice (semitones)
    float detunedRatios[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // CPU fix: precomputed fastExp(detune) per sub-voice
    float panOffsets[4] = {};          // Stereo position per sub-voice

    // --- Pitch sweep (the "snap" transient) ---
    float currentPitch = 0.0f;        // Sweeping from (target + snapOffset)
    float targetPitch = 0.0f;         // Final resting pitch (MIDI note)
    float pitchSweepPhase = 0.0f;     // 0..1 progress through sweep

    // --- Frequency cache (CPU fix: avoids midiToHz/std::pow per sample) ---
    float cachedFreq = 440.0f;        // Last computed base frequency
    float lastMidiNote = 69.0f;       // MIDI note at which cachedFreq was computed
};


//==============================================================================
//
//  SnapEngine — The ODDFELIX synthesis engine
//
//  Percussive, transient-rich synthesis inspired by the neon tetra's
//  electric dart through sunlit water. Every note is a flash — bright
//  attack, rapid decay, no sustain.
//
//  Signal Flow:
//    MIDI Note -> Pitch Snap Sweep (+24st -> target, speed controlled by snap)
//              -> Oscillator (3 modes: Sine+Noise / FM / Karplus-Strong)
//              -> Unison Spread (1/2/4 sub-voices with detune + pan scatter)
//              -> Snap Waveshaper (tanh saturation driven by snap amount)
//              -> Filter Cascade (HPF -> BPF, Cytomic SVF topology)
//              -> Decay Envelope (pure decay, no sustain — percussive DNA)
//              -> Stereo Output -> Coupling Bus
//
//  Coupling:
//    Output: envelope level (ch2) for AmpToFilter/AmpToPitch on partner engines
//    Input:  AmpToFilter  — partner amplitude multiplies BPF center (clamped [0.1, 2.0])
//            AmpToPitch, LFOToPitch, PitchToPitch — pitch modulation from
//            partner engines (max +/-0.5 semitones at amount=1.0)
//
//  Macros:
//    M1 DART    — Sharper transient, shorter decay (feliX at his most electric)
//    M2 SCHOOL  — Wider unison spread (a school of tetras, not a solo fish)
//    M3 SURFACE — Brighter filter, more resonance (sunlit shallow water)
//    M4 DEPTH   — Reserved for echo/FX presence (descending the water column)
//
//==============================================================================
class SnapEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    //==========================================================================
    //  SynthEngine Interface
    //==========================================================================

    void prepare (double newSampleRate, int maxBlockSize) override
    {
        sampleRate = newSampleRate;
        sampleRateFloat = static_cast<float> (sampleRate);
        silenceGate.prepare (newSampleRate, maxBlockSize);

        // Pre-allocate output cache buffers for coupling reads.
        // Other engines read our output via getSampleForCoupling().
        outputCacheLeft.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheRight.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        aftertouch.prepare (newSampleRate);

        for (auto& voice : voices)
        {
            voice.active = false;
            voice.fadeOutLevel = 0.0f;

            voice.ampEnv.prepare (sampleRateFloat);
            voice.ampEnv.setShape (StandardADSR::Shape::AD);
            voice.ampEnv.kill();

            // Reset oscillators
            voice.sineOscillator.reset();
            voice.noiseOscillator.reset();
            voice.noiseOscillator.setWaveform (PolyBLEP::Waveform::Saw); // High-freq saw approximates noise
            voice.fmModulatorOscillator.reset();

            for (auto& karplusStrong : voice.karplusStrongOscillators)
                karplusStrong.prepare (sampleRate);

            // Initialize filter cascade
            voice.highPassFilter.reset();
            voice.highPassFilter.setMode (CytomicSVF::Mode::HighPass);
            voice.bandPassFilter.reset();
            voice.bandPassFilter.setMode (CytomicSVF::Mode::BandPass);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& voice : voices)
        {
            voice.active = false;
            voice.ampEnv.kill();
            voice.fadeOutLevel = 0.0f;

            voice.sineOscillator.reset();
            voice.noiseOscillator.reset();
            voice.fmModulatorOscillator.reset();

            for (auto& karplusStrong : voice.karplusStrongOscillators)
                karplusStrong.reset();

            voice.highPassFilter.reset();
            voice.bandPassFilter.reset();
        }

        envelopeOutput = 0.0f;
        externalPitchModulation = 0.0f;
        couplingCutoffMod = 1.0f;

        std::fill (outputCacheLeft.begin(), outputCacheLeft.end(), 0.0f);
        std::fill (outputCacheRight.begin(), outputCacheRight.end(), 0.0f);
    }

    //==========================================================================
    //  Audio Rendering
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0) return;

        // ---- ParamSnapshot: cache all parameter values once per block --------
        // This pattern avoids repeated atomic loads in the per-sample loop.
        // Each parameter pointer was set during attachParameters().

        const int oscillatorModeIndex = (pOscMode != nullptr)
            ? static_cast<int> (pOscMode->load()) : 0;
        const float snapAmount     = (pSnap != nullptr) ? pSnap->load() : 0.4f;
        const float decayTime      = (pDecay != nullptr) ? pDecay->load() : 0.5f;
        const float filterCutoff      = (pFilterCutoff    != nullptr) ? pFilterCutoff->load()    : 2000.0f;
        const float filterResonance   = (pFilterReso      != nullptr) ? pFilterReso->load()      : 0.3f;
        const float filterEnvDepth    = (pFilterEnvDepth  != nullptr) ? pFilterEnvDepth->load()  : 0.3f;
        const float detuneCents    = (pDetune != nullptr) ? pDetune->load() : 10.0f;
        const float outputLevel    = (pLevel != nullptr) ? pLevel->load() : 0.8f;
        const bool pitchLocked        = (pPitchLock != nullptr) && (pPitchLock->load() >= 0.5f);

        // Sweep direction: -1.0 = classic downward sweep, +1.0 = upward sweep.
        // Default -1.0 keeps legacy preset behaviour unchanged.
        const float sweepDirection = (pSweepDirection != nullptr) ? pSweepDirection->load() : -1.0f;

        // Unison count: parameter values 0/1/2 map to 1/2/4 sub-voices via bit shift
        const int unisonCount = (pUnison != nullptr)
            ? (1 << static_cast<int> (pUnison->load())) : 1;

        // Polyphony: parameter values 0/1/2/3 map to 1/2/4/8 voices via bit shift
        const int maxPolyphony = (pPolyphony != nullptr)
            ? (1 << std::min (3, static_cast<int> (pPolyphony->load()))) : 4;

        // ---- Macro processing -----------------------------------------------

        // D002: LFO depths — user-controllable modulation depth for both LFOs.
        // Defaults match pre-fix hardcoded values (0.08) for backward compatibility.
        const float lfoDepth  = (pLfoDepth  != nullptr) ? pLfoDepth->load()  : 0.08f;
        const float lfo2Depth = (pLfo2Depth != nullptr) ? pLfo2Depth->load() : 0.08f;

        // D005: user-controllable LFO rates with floor ≤ 0.01 Hz.
        // lfoRate default 4.0 Hz — neon tetra tail-flick energy (fast but user can slow to breathe).
        // lfo2Rate default 0.08 Hz — preserves legacy stereo-pan wobble speed for old presets.
        const float lfoRate  = (pLfoRate  != nullptr) ? pLfoRate->load()  : 4.0f;
        const float lfo2Rate = (pLfo2Rate != nullptr) ? pLfo2Rate->load() : 0.08f;

        const float macroDart    = (pMacroDart != nullptr)    ? pMacroDart->load()    : 0.0f;
        const float macroSchool  = (pMacroSchool != nullptr)  ? pMacroSchool->load()  : 0.0f;
        const float macroSurface = (pMacroSurface != nullptr) ? pMacroSurface->load() : 0.0f;
        const float macroDepth   = (pMacroDepth != nullptr)   ? pMacroDepth->load()   : 0.0f;

        // M1 DART: sharper transient (+40% snap), shorter decay (-30% time)
        // The neon tetra's fastest dart — maximum attack, minimum hang time
        const float effectiveSnap  = std::max (0.0f, std::min (1.0f, snapAmount + macroDart * 0.4f));
        const float effectiveDecay = std::max (0.001f, decayTime - macroDart * 0.3f);

        // M2 SCHOOL: wider unison detune (+20 cents) — a school of tetras, not one
        const float effectiveDetune = std::max (0.0f, std::min (50.0f, detuneCents + macroSchool * 20.0f));

        // M3 SURFACE: brighter filter (+4kHz cutoff), more resonance (+0.3)
        // Moving toward the sunlit surface of the water column
        const float effectiveCutoff    = std::max (20.0f, std::min (20000.0f, filterCutoff + macroSurface * 4000.0f));
        const float effectiveResonance = std::max (0.0f, std::min (1.0f, filterResonance + macroSurface * 0.3f));

        // M4 DEPTH: increases unison stereo spread — descending the water column
        // widens the stereo image, like feliX diving deeper into a wider, more
        // spacious acoustic environment.
        // D004 fix: macroDepth now modulates unison pan spread (kUnisonPanSpread target)
        const float effectivePanSpread = std::max (0.0f, std::min (1.0f, 0.3f + macroDepth * 0.7f));

        // ---- Process MIDI events --------------------------------------------

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();
            if (message.isNoteOn())
            {
                silenceGate.wake();
                noteOn (message.getNoteNumber(), message.getFloatVelocity(),
                        effectiveSnap, effectiveDecay, pitchLocked, sweepDirection, effectiveDetune, unisonCount,
                        effectiveCutoff, effectiveResonance, maxPolyphony,
                        oscillatorModeIndex);
            }
            else if (message.isNoteOff())
            {
                noteOff (message.getNoteNumber());
            }
            else if (message.isAllNotesOff() || message.isAllSoundOff())
            {
                reset();
            }
            // D006: channel pressure → aftertouch (applied to BPF cutoff below)
            else if (message.isChannelPressure())
            {
                aftertouch.setChannelPressure (message.getChannelPressureValue() / 127.0f);
            }
            // D006: CC1 mod wheel → BPF resonance (more ring/peak with wheel; sensitivity 0.4)
            else if (message.isController() && message.getControllerNumber() == 1)
            {
                modWheelValue = message.getControllerValue() / 127.0f;
            }
            else if (message.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel (message.getPitchWheelValue());
        }

        // SilenceGate: skip all DSP if engine has been silent long enough
        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // D006: smooth aftertouch pressure and compute modulation values
        aftertouch.updateBlock (numSamples);
        const float atPressure = aftertouch.getSmoothedPressure (0);  // Channel-mode: voice 0 holds the global value

        // Consume coupling pitch modulation (reset after use per coupling contract)
        float pitchModulation = externalPitchModulation;
        externalPitchModulation = 0.0f;

        // Consume coupling cutoff multiplier (reset to unity after use)
        float cutoffMod = couplingCutoffMod;
        couplingCutoffMod = 1.0f;

        // ---- Fade out voices beyond current polyphony limit -----------------

        for (int i = maxPolyphony; i < kMaxVoices; ++i)
        {
            if (voices[static_cast<size_t> (i)].active)
                voices[static_cast<size_t> (i)].ampEnv.noteOff();  // Force quick release/fade
        }

        // ---- Pre-compute filter coefficients (block-rate) -------------------
        // Cytomic SVF coefficient calculation involves tan(), which is expensive.
        // Computing once per block (not per sample) saves significant CPU while
        // being perceptually transparent for block sizes up to ~128 samples.

        // D005: autonomous LFO1 — BPF center wobble.
        // Rate = user snap_lfoRate (0.01–20 Hz, default 4 Hz for neon tetra dart energy).
        // D006: aftertouch boosts LFO rate up to +8 Hz — faster tail-flick under pressure.
        // DART macro adds further speed boost (×2 at max DART) for maximum agitation.
        const double effectiveLfoRateHz = static_cast<double> (lfoRate)
                                        + static_cast<double> (atPressure) * 8.0
                                        + static_cast<double> (macroDart) * static_cast<double> (lfoRate);
        lfoPhase += (effectiveLfoRateHz * juce::MathConstants<double>::twoPi) / sampleRate;
        if (lfoPhase >= juce::MathConstants<double>::twoPi) lfoPhase -= juce::MathConstants<double>::twoPi;

        // D005: Secondary LFO2 — slow stereo pan wobble (triangle).
        // Rate = user snap_lfo2Rate (0.01–8 Hz, default 0.08 Hz preserves legacy preset feel).
        lfo2Phase += static_cast<double> (lfo2Rate) / sampleRate;
        if (lfo2Phase >= 1.0) lfo2Phase -= 1.0;

        // D006: mod wheel boosts LFO1 depth (schooling behavior — more shimmer in the school).
        // Wheel at full adds +0.25 LFO depth on top of lfoDepth; clamped at 0.5.
        const float effectiveLfoDepth = std::min (0.5f, lfoDepth + modWheelValue * 0.25f);

        float lfo2Stereo = static_cast<float> (4.0 * std::fabs (lfo2Phase - 0.5) - 1.0) * lfo2Depth; // ±lfo2Depth pan offset

        // AmpToFilter coupling multiplier applied here — partner engine amplitude
        // opens/closes feliX's BPF center in tandem with the LFO wobble.
        // D006: aftertouch adds up to +6kHz brightness on full pressure (sensitivity 0.3)
        const float effectiveBpfCenter = std::max (20.0f, std::min (20000.0f,
                                             effectiveCutoff
                                             * (1.0f + effectiveLfoDepth * (float)std::sin(lfoPhase))
                                             * cutoffMod
                                             + atPressure * 0.3f * 6000.0f));

        // D006: mod wheel adds up to +0.4 resonance — more ring/peak with wheel (sensitivity 0.4)
        // (mod wheel already contributes to LFO depth above; resonance boost is additive and complementary)
        const float modWheelResonance = std::min (1.0f, effectiveResonance + modWheelValue * 0.4f);

        // D001: Filter envelope depth — per-voice BPF center scaled by envelope × velocity.
        // A higher velocity hit opens the filter wider at the transient peak; the center
        // falls back toward effectiveBpfCenter as the decay envelope decays.
        // Max additional sweep: filterEnvDepth × velocity × 8000 Hz.
        static constexpr float kFilterEnvMaxSweep = 8000.0f;
        for (auto& voice : voices)
        {
            if (!voice.active) continue;
            float envVelBoost = filterEnvDepth * voice.velocity * voice.ampEnv.getLevel() * kFilterEnvMaxSweep;
            float voiceCutoff = std::max (20.0f, std::min (20000.0f, effectiveBpfCenter + envVelBoost));
            voice.highPassFilter.setCoefficients (voiceCutoff, modWheelResonance, sampleRateFloat);
            voice.bandPassFilter.setCoefficients (voiceCutoff, modWheelResonance, sampleRateFloat);
        }

        // ---- Block-rate envelope decay coefficient update -------------------
        // effectiveDecay is computed once per block from parameters + DART macro.
        // Propagate it to all active voice envelopes now, before the sample loop,
        // so setADSR (which calls exp()) runs at block-rate, not per-sample.
        for (auto& voice : voices)
            if (voice.active)
                voice.ampEnv.setADSR (0.0001f, effectiveDecay, 0.0f, 0.001f);

        // ---- Per-sample rendering -------------------------------------------

        float peakEnvelopeLevel = 0.0f;

        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            float mixLeft = 0.0f, mixRight = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                // ---- Decay envelope (percussive AD shape) -------------------
                // StandardADSR in AD mode: instant attack at noteOn, then
                // exponential decay to zero (natural percussive tail).
                float envLevel = voice.ampEnv.process();

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // ---- Voice-stealing crossfade (5ms ramp) --------------------
                // When a new note steals an active voice, the old signal fades
                // out over 5ms to prevent click artifacts.
                float stealFadeGain = 1.0f;
                if (voice.fadeOutLevel > 0.0f)
                {
                    // 5ms = 0.005 seconds. Rate = 1/(0.005 * sampleRate) per sample.
                    voice.fadeOutLevel -= 1.0f / (0.005f * sampleRateFloat);
                    voice.fadeOutLevel = flushDenormal (voice.fadeOutLevel);
                    if (voice.fadeOutLevel <= 0.0f)
                        voice.fadeOutLevel = 0.0f;
                    stealFadeGain = 1.0f - voice.fadeOutLevel;
                }

                // ---- Pitch snap sweep ---------------------------------------
                // The signature "snap" effect: pitch starts high (up to +24
                // semitones above target) and sweeps down in ~250ms.
                // This descends from analog drum synths (TR-808, Simmons SDS-V)
                // where a VCO sweeps from a high frequency to create the
                // percussive attack character of kicks, toms, and synth drums.
                //
                // sweepRate = 4.0 means the sweep completes in 0.25 seconds
                // (sampleRate/4 samples). This matches the "snap" time constant
                // of classic analog drum circuits.
                static constexpr float kPitchSweepRate = 4.0f;  // Completes in 250ms

                voice.pitchSweepPhase += kPitchSweepRate / sampleRateFloat;
                float sweepProgress = std::min (voice.pitchSweepPhase, 1.0f);
                float currentMidiNote = voice.currentPitch * (1.0f - sweepProgress)
                                      + voice.targetPitch * sweepProgress;
                currentMidiNote += pitchModulation;

                // CPU fix: only call midiToHz when pitch changes by more than 0.01 semitones.
                // midiToHz calls std::pow — expensive per sample per voice.
                if (std::fabs (currentMidiNote - voice.lastMidiNote) > 0.01f)
                {
                    voice.cachedFreq   = midiToHz (currentMidiNote);
                    voice.lastMidiNote = currentMidiNote;
                }
                float frequency = voice.cachedFreq
                                  * PitchBendUtil::semitonesToFreqRatio (pitchBendNorm * 2.0f);

                // ---- Oscillator output with unison --------------------------
                int activeUnisonCount = std::min (unisonCount, 4);
                float voiceLeft = 0.0f, voiceRight = 0.0f;

                for (int unisonIndex = 0; unisonIndex < activeUnisonCount; ++unisonIndex)
                {
                    // CPU fix: detunedRatios precomputed at noteOn; multiply base frequency.
                    float detunedFrequency = frequency * voice.detunedRatios[unisonIndex];

                    float unisonOutput = 0.0f;

                    switch (oscillatorModeIndex)
                    {
                        case 0: // Sine + Noise (feliX's clean dart with spray)
                        {
                            voice.sineOscillator.setFrequency (detunedFrequency, sampleRateFloat);
                            voice.sineOscillator.setWaveform (PolyBLEP::Waveform::Sine);
                            float sineSignal = voice.sineOscillator.processSample();

                            // Generate pseudo-noise by running a PolyBLEP saw at near-Nyquist.
                            // At 0.49 * sampleRate, the saw's dense harmonics create a
                            // noise-like texture without needing a separate noise generator.
                            static constexpr float kNyquistRatio = 0.49f;
                            static constexpr float kNoiseMixLevel = 0.3f;  // Noise sits 10dB below sine

                            voice.noiseOscillator.setFrequency (sampleRateFloat * kNyquistRatio, sampleRateFloat);
                            voice.noiseOscillator.setWaveform (PolyBLEP::Waveform::Saw);
                            float noiseSignal = voice.noiseOscillator.processSample() * effectiveSnap;
                            unisonOutput = sineSignal + noiseSignal * kNoiseMixLevel;
                            break;
                        }

                        case 1: // FM Percussion (DX7-lineage metallic transients)
                        {
                            // 2:1 modulator ratio — classic FM percussion recipe.
                            // Index scaled by snap amount (0-4x) for controllable metallicity.
                            static constexpr float kFmModulatorRatio = 2.0f;
                            static constexpr float kFmIndexScale = 4.0f;

                            voice.fmModulatorOscillator.setFrequency (
                                detunedFrequency * kFmModulatorRatio, sampleRateFloat);
                            voice.fmModulatorOscillator.setWaveform (PolyBLEP::Waveform::Sine);
                            float modulatorSignal = voice.fmModulatorOscillator.processSample()
                                                  * effectiveSnap * kFmIndexScale;

                            float modulatedFrequency = detunedFrequency
                                                     + modulatorSignal * detunedFrequency;
                            if (modulatedFrequency < 0.0f) modulatedFrequency = 0.0f;

                            voice.sineOscillator.setFrequency (modulatedFrequency, sampleRateFloat);
                            voice.sineOscillator.setWaveform (PolyBLEP::Waveform::Sine);
                            unisonOutput = voice.sineOscillator.processSample();
                            break;
                        }

                        case 2: // Karplus-Strong (plucked/struck string physical model)
                        {
                            unisonOutput = voice.karplusStrongOscillators[unisonIndex].nextSample();
                            break;
                        }
                    }

                    // ---- Per-unison stereo panning ---------------------------
                    // Center pan = 0.5. Unison voices spread +/- (kUnisonPanSpread * effectivePanSpread)
                    // from center. effectivePanSpread is modulated by M4 DEPTH macro (0.3 -> 1.0),
                    // widening the stereo image as feliX "descends" into deeper sonic space.
                    // D004 fix: effectivePanSpread (from macroDepth) controls spread width here.
                    static constexpr float kUnisonPanSpread = 1.0f;  // base multiplier; actual spread is effectivePanSpread

                    float panPosition = 0.5f;
                    if (unisonCount > 1)
                        panPosition = 0.5f + voice.panOffsets[unisonIndex] * kUnisonPanSpread * effectivePanSpread;

                    voiceLeft  += unisonOutput * (1.0f - panPosition);
                    voiceRight += unisonOutput * panPosition;
                }

                // Normalize by unison count to maintain consistent volume
                float unisonNormalization = 1.0f / static_cast<float> (activeUnisonCount);
                voiceLeft  *= unisonNormalization;
                voiceRight *= unisonNormalization;

                // ---- Snap waveshaper (pre-filter tanh saturation) ------------
                // The snap parameter simultaneously controls pitch sweep range,
                // noise level, FM index, AND drive amount. This coupling creates
                // a single musical gesture: more snap = sharper, brighter, dirtier.
                // Drive range: 1x (clean) to 9x (heavy saturation).
                if (effectiveSnap > 0.0f)
                {
                    static constexpr float kMaxDriveMultiplier = 8.0f;  // 1 + 8 = 9x max drive

                    float driveAmount = 1.0f + effectiveSnap * kMaxDriveMultiplier;
                    voiceLeft  = fastTanh (voiceLeft * driveAmount);
                    voiceRight = fastTanh (voiceRight * driveAmount);
                }

                // ---- Filter cascade: HPF -> BPF -----------------------------
                // Process as mono sum then restore stereo balance. This saves
                // one filter pass per sample while preserving the stereo image.
                float monoSignal = (voiceLeft + voiceRight) * 0.5f;
                float filteredSignal = voice.highPassFilter.processSample (monoSignal);
                filteredSignal = voice.bandPassFilter.processSample (filteredSignal);

                // Restore stereo balance from original L/R ratio
                float filteredLeft, filteredRight;
                float amplitudeSum = std::abs (voiceLeft) + std::abs (voiceRight);

                // Guard against division by zero when both channels are silent
                static constexpr float kSilenceThreshold = 1e-6f;

                if (amplitudeSum > kSilenceThreshold)
                {
                    float leftRatio  = voiceLeft / (amplitudeSum * 0.5f);
                    float rightRatio = voiceRight / (amplitudeSum * 0.5f);
                    filteredLeft  = filteredSignal * leftRatio;
                    filteredRight = filteredSignal * rightRatio;
                }
                else
                {
                    filteredLeft  = filteredSignal;
                    filteredRight = filteredSignal;
                }

                // ---- Apply envelope, velocity, and steal fade ---------------
                float outputLeft  = filteredLeft  * envLevel * voice.velocity * stealFadeGain;
                float outputRight = filteredRight * envLevel * voice.velocity * stealFadeGain;

                mixLeft  += outputLeft;
                mixRight += outputRight;

                peakEnvelopeLevel = std::max (peakEnvelopeLevel, envLevel);
            }

            // ---- Write to output buffer -------------------------------------
            // DSP FIX: Apply LFO2 stereo pan wobble — subtle lateral movement
            float finalLeft  = (mixLeft  * (1.0f + lfo2Stereo) + mixRight * (-lfo2Stereo * 0.5f)) * outputLevel;
            float finalRight = (mixRight * (1.0f - lfo2Stereo) + mixLeft  * ( lfo2Stereo * 0.5f)) * outputLevel;

            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample (0, sampleIndex, finalLeft);
                buffer.addSample (1, sampleIndex, finalRight);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample (0, sampleIndex, (finalLeft + finalRight) * 0.5f);
            }

            // ---- Cache output for coupling reads ----------------------------
            if (sampleIndex < static_cast<int> (outputCacheLeft.size()))
            {
                outputCacheLeft[static_cast<size_t> (sampleIndex)] = finalLeft;
                outputCacheRight[static_cast<size_t> (sampleIndex)] = finalRight;
            }
        }

        // Store peak envelope for coupling output (channel 2)
        envelopeOutput = peakEnvelopeLevel;

        // Cache active voice count for getActiveVoiceCount() (message-thread safe).
        {
            int count = 0;
            for (const auto& voice : voices)
                if (voice.active) ++count;
            activeVoiceCount.store (count);
        }

        // SilenceGate: analyze output level for next-block bypass decision
        silenceGate.analyzeBlock (buffer.getReadPointer (0),
                                  buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : nullptr,
                                  numSamples);
    }

    //==========================================================================
    //  Coupling Interface
    //==========================================================================

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        if (sampleIndex < 0) return 0.0f;
        auto index = static_cast<size_t> (sampleIndex);

        // Channel 0: left audio output (post-filter, post-envelope)
        if (channel == 0 && index < outputCacheLeft.size())
            return outputCacheLeft[index];

        // Channel 1: right audio output (post-filter, post-envelope)
        if (channel == 1 && index < outputCacheRight.size())
            return outputCacheRight[index];

        // Channel 2: envelope level — the "hit" signal for AmpToFilter,
        // AmpToChoke coupling. Other engines use this to feel feliX's darts.
        if (channel == 2) return envelopeOutput;

        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
            case CouplingType::AmpToFilter:
                // Partner engine amplitude (ch2 "hit signal") modulates feliX's
                // BPF center frequency. Amount is a multiplier in [0, 2]:
                //   amount = 0   → cutoff fully closed (silence-gate effect)
                //   amount = 1   → no change (unity)
                //   amount > 1   → cutoff boosted (brightness coupling)
                // Clamp to [0.1, 2.0] to avoid fully closing the filter or
                // aliasing from extreme cutoff boost.
                couplingCutoffMod = std::max (0.1f, std::min (2.0f, amount));
                break;

            case CouplingType::AmpToPitch:
            case CouplingType::LFOToPitch:
            case CouplingType::PitchToPitch:
                // Accumulate pitch modulation from partner engines.
                // Max +/-0.5 semitones at amount=1.0 — subtle organic drift,
                // like Oscar's deep current gently nudging feliX's trajectory.
                externalPitchModulation += amount * 0.5f;
                break;

            default:
                break;  // Other coupling types not supported by ODDFELIX
        }
    }

    //==========================================================================
    //  Parameter Layout
    //==========================================================================

    /// Static helper: add ODDFELIX parameters to a shared vector (used by processor).
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

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // ---- Oscillator mode ----
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "snap_oscMode", 1 }, "Snap Osc Mode",
            juce::StringArray { "Sine+Noise", "FM", "Karplus-Strong" }, 0));

        // ---- Core sound parameters ----
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_snap", 1 }, "Snap",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.4f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_decay", 1 }, "Snap Decay",
            juce::NormalisableRange<float> (0.0f, 8.0f, 0.001f, 0.3f), 0.5f));

        // ---- Filter ----
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_filterCutoff", 1 }, "Snap Filter Cutoff",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f), 2000.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_filterReso", 1 }, "Snap Filter Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        // D001: Filter envelope depth — scales how much the decay envelope opens
        // the BPF center on each hit. Velocity × envLevel × depth pushes the
        // filter open at the transient peak and closes it as the note decays.
        // Default 0.3 gives a noticeable velocity-to-brightness response without
        // overwhelming the base cutoff setting.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_filterEnvDepth", 1 }, "Snap Filter Env Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));

        // ---- Unison & tuning ----
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_detune", 1 }, "Snap Detune",
            juce::NormalisableRange<float> (0.0f, 50.0f, 0.1f), 10.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_level", 1 }, "Snap Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { "snap_pitchLock", 1 }, "Snap Pitch Lock", false));

        // ---- Pitch sweep direction ------------------------------------------
        // Controls whether the pitch snap sweeps downward (classic drum synth
        // behaviour, -1.0) or upward (+1.0 for effect snares / pitched toms),
        // or any blend between the two.  Default -1.0 preserves legacy presets.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_sweepDirection", 1 }, "Snap Sweep Direction",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), -1.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "snap_unison", 1 }, "Snap Unison",
            juce::StringArray { "1", "2", "4" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "snap_polyphony", 1 }, "Snap Polyphony",
            juce::StringArray { "1", "2", "4", "8" }, 2));

        // ---- LFO depth — D002: user-exposed LFO modulation depth ----
        // D002 fix: BPF wobble depth was hardcoded at 8%; now user-controllable.
        // At default (0.08) behaviour is identical to pre-fix presets.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_lfoDepth", 1 }, "Snap LFO Depth",
            juce::NormalisableRange<float> (0.0f, 0.5f, 0.001f), 0.08f));

        // D002 fix: stereo pan wobble depth (LFO2) was hardcoded at 8%; now user-controllable.
        // At default (0.08) behaviour is identical to pre-fix presets.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_lfo2Depth", 1 }, "Snap LFO2 Pan Depth",
            juce::NormalisableRange<float> (0.0f, 0.2f, 0.001f), 0.08f));

        // ---- LFO rates — D005: floor ≤ 0.01 Hz so the engine can breathe ----
        // snap_lfoRate default 4.0 Hz: neon tetra tail-flick default is fast and electric.
        // Mod wheel adds +0 to +8 Hz on top; aftertouch adds further burst for tail-flick.
        // Floor 0.01 Hz allows ultra-slow breath for ambient textures.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_lfoRate", 1 }, "Snap LFO Rate",
            juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.3f), 4.0f));

        // snap_lfo2Rate default 0.08 Hz: preserves original slow stereo-pan wobble speed.
        // Can be raised for faster lateral darting across the stereo field.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_lfo2Rate", 1 }, "Snap LFO2 Pan Rate",
            juce::NormalisableRange<float> (0.01f, 8.0f, 0.01f, 0.4f), 0.08f));

        // ---- Macros (the four gestures of the neon tetra) ----
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_macroDart", 1 }, "Snap Dart",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_macroSchool", 1 }, "Snap School",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_macroSurface", 1 }, "Snap Surface",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "snap_macroDepth", 1 }, "Snap Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    }

    //==========================================================================
    //  Parameter Attachment
    //==========================================================================

public:
    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pOscMode      = apvts.getRawParameterValue ("snap_oscMode");
        pSnap         = apvts.getRawParameterValue ("snap_snap");
        pDecay        = apvts.getRawParameterValue ("snap_decay");
        pFilterCutoff     = apvts.getRawParameterValue ("snap_filterCutoff");
        pFilterReso       = apvts.getRawParameterValue ("snap_filterReso");
        pFilterEnvDepth   = apvts.getRawParameterValue ("snap_filterEnvDepth");
        pDetune       = apvts.getRawParameterValue ("snap_detune");
        pLevel        = apvts.getRawParameterValue ("snap_level");
        pPitchLock       = apvts.getRawParameterValue ("snap_pitchLock");
        pSweepDirection  = apvts.getRawParameterValue ("snap_sweepDirection");
        pUnison          = apvts.getRawParameterValue ("snap_unison");
        pPolyphony    = apvts.getRawParameterValue ("snap_polyphony");
        pLfoDepth     = apvts.getRawParameterValue ("snap_lfoDepth");
        pLfo2Depth    = apvts.getRawParameterValue ("snap_lfo2Depth");
        pLfoRate      = apvts.getRawParameterValue ("snap_lfoRate");
        pLfo2Rate     = apvts.getRawParameterValue ("snap_lfo2Rate");
        pMacroDart    = apvts.getRawParameterValue ("snap_macroDart");
        pMacroSchool  = apvts.getRawParameterValue ("snap_macroSchool");
        pMacroSurface = apvts.getRawParameterValue ("snap_macroSurface");
        pMacroDepth   = apvts.getRawParameterValue ("snap_macroDepth");
    }

    //==========================================================================
    //  Engine Identity
    //==========================================================================

    juce::String getEngineId() const override { return "OddfeliX"; }

    juce::Colour getAccentColour() const override
    {
        return juce::Colour (0xFF00A6D6);  // Neon Tetra Blue — feliX's scales
    }

    int getMaxVoices() const override { return kMaxVoices; }

    // Safe to call from message thread — returns a cached count updated at the
    // end of each renderBlock(), so it is never iterated from the wrong thread.
    int getActiveVoiceCount() const override { return activeVoiceCount.load(); }

private:
    SilenceGate silenceGate;

    //==========================================================================
    //  Note Handling
    //==========================================================================

    void noteOn (int noteNumber, float velocity, float snapAmount, float decayTime,
                 bool pitchLocked, float sweepDir, float detuneCents, int unisonCount,
                 float cutoffFrequency, float resonance, int maxPolyphony, int oscillatorModeIndex)
    {
        int polyphonyLimit = std::min (maxPolyphony, kMaxVoices);
        int voiceIndex = VoiceAllocator::findFreeVoice (voices, polyphonyLimit);
        auto& voice = voices[static_cast<size_t> (voiceIndex)];

        // Smooth fade-out if stealing an active voice
        if (voice.active)
            voice.fadeOutLevel = voice.ampEnv.getLevel();
        else
            voice.fadeOutLevel = 0.0f;

        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.startTime = voiceCounter++;

        // Configure and trigger the AD envelope for this hit.
        // setADSR is also called per-sample in the render loop to track
        // effectiveDecay changes from the DART macro; this call primes
        // the decay coefficient before the first sample is processed.
        voice.ampEnv.prepare (sampleRateFloat);
        voice.ampEnv.setShape (StandardADSR::Shape::AD);
        voice.ampEnv.setADSR (0.0001f, decayTime, 0.0f, 0.001f);
        voice.ampEnv.noteOn();

        // Set up pitch snap sweep: start offset from target, sweep toward it.
        // sweepDir = -1.0 → classic downward sweep (starts 2 octaves above).
        // sweepDir = +1.0 → upward sweep (starts 2 octaves below — effect snares,
        //                    pitched toms, riser transients).
        // The sweep distance scales with snapAmount; at snap=0 there is no sweep
        // regardless of direction.
        static constexpr float kMaxPitchSweepSemitones = 24.0f;

        float baseMidiNote = pitchLocked ? 60.0f : static_cast<float> (noteNumber);
        // Negate sweepDir: direction = -1 means start ABOVE (positive offset),
        // direction = +1 means start BELOW (negative offset).
        voice.currentPitch = baseMidiNote - sweepDir * kMaxPitchSweepSemitones * snapAmount;
        voice.targetPitch = baseMidiNote;
        voice.pitchSweepPhase = 0.0f;

        float baseFrequency = midiToHz (baseMidiNote);

        // ---- Unison detune and pan scatter ----------------------------------
        // Spread sub-voices symmetrically around center. The offsets create
        // a stereo "school" effect when unison > 1.
        {
            static constexpr float kSemitonesToNatLog = 0.693147f / 12.0f;  // ln(2) / 12
            for (int unisonIndex = 0; unisonIndex < 4; ++unisonIndex)
            {
                // Offset pattern: -1.5, -0.5, +0.5, +1.5 (centered around 0)
                float normalizedOffset = (static_cast<float> (unisonIndex) - 1.5f);
                voice.detuneOffsets[unisonIndex] = normalizedOffset * detuneCents / 100.0f;
                voice.panOffsets[unisonIndex] = normalizedOffset / 2.0f;
                // CPU fix: precompute per-voice detune ratio once at noteOn.
                // detunedRatios[i] = fastExp(detuneOffsets[i] * ln2/12)
                // At sample time: detunedFrequency = frequency * detunedRatios[i]
                voice.detunedRatios[unisonIndex] = fastExp (voice.detuneOffsets[unisonIndex] * kSemitonesToNatLog);
            }
            // Initialise frequency cache for the new note.
            voice.cachedFreq  = midiToHz (baseMidiNote);
            voice.lastMidiNote = baseMidiNote;
        }

        // ---- Reset oscillators for new note ---------------------------------
        voice.sineOscillator.reset();
        voice.noiseOscillator.reset();
        voice.fmModulatorOscillator.reset();

        // Karplus-Strong mode: set frequency, damping, and trigger excitation
        if (oscillatorModeIndex == 2)
        {
            int activeUnisonCount = std::min (unisonCount, 4);
            for (int unisonIndex = 0; unisonIndex < activeUnisonCount; ++unisonIndex)
            {
                // CPU fix: use precomputed detunedRatios (set in unison loop above).
                float detunedFrequency = baseFrequency * voice.detunedRatios[unisonIndex];
                voice.karplusStrongOscillators[unisonIndex].setFrequency (
                    static_cast<double> (detunedFrequency));
                // Higher snap = less damping = brighter, more metallic string
                voice.karplusStrongOscillators[unisonIndex].setDamping (1.0f - snapAmount);
                voice.karplusStrongOscillators[unisonIndex].trigger();
            }
        }

        // ---- Reset and configure filter cascade for new note ----------------
        voice.highPassFilter.reset();
        voice.highPassFilter.setMode (CytomicSVF::Mode::HighPass);
        voice.highPassFilter.setCoefficients (cutoffFrequency, resonance, sampleRateFloat);

        voice.bandPassFilter.reset();
        voice.bandPassFilter.setMode (CytomicSVF::Mode::BandPass);
        voice.bandPassFilter.setCoefficients (cutoffFrequency, resonance, sampleRateFloat);
    }

    void noteOff (int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber)
            {
                // Percussive character: AD shape has no sustain or release.
                // noteOff on an AD envelope is a no-op (already decaying),
                // but calling it correctly signals intent and satisfies the
                // StandardADSR contract.
                voice.ampEnv.noteOff();
            }
        }
    }

    //==========================================================================
    //  Utilities
    //==========================================================================

    /// Convert MIDI note number to frequency in Hz.
    /// A4 (MIDI 69) = 440 Hz, equal temperament, 12-TET.
    static float midiToHz (float midiNote) noexcept
    {
        return 440.0f * std::pow (2.0f, (midiNote - 69.0f) / 12.0f);
    }

    //==========================================================================
    //  Member State
    //==========================================================================

    // ---- Active voice count — cached at end of renderBlock(), safe for message thread ----
    std::atomic<int> activeVoiceCount { 0 };

    // ---- Audio engine state ----
    double sampleRate = 44100.0;
    float sampleRateFloat = 44100.0f;
    std::array<SnapVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;

    // D005 fix: autonomous LFO
    double lfoPhase = 0.0;  // BPF center drift accumulator (0.15-0.6 Hz, scales with DART)
    double lfo2Phase = 0.0; // DSP FIX: stereo pan wobble LFO (0.08 Hz triangle)

    // ---- D006 Aftertouch — pressure opens BPF cutoff for brightness on pressure ----
    PolyAftertouch aftertouch;

    // ---- D006 Mod wheel — CC1 increases BPF resonance (more ring/peak; sensitivity 0.4) ----
    float modWheelValue = 0.0f;
    float pitchBendNorm = 0.0f;  // MIDI pitch wheel [-1, +1]; ±2 semitone range

    // ---- Coupling state ----
    float envelopeOutput = 0.0f;           // Peak envelope for coupling channel 2
    float externalPitchModulation = 0.0f;  // Accumulated pitch mod from partner engines
    float couplingCutoffMod = 1.0f;        // AmpToFilter multiplier for BPF center (unity = 1.0)

    // ---- Output cache for coupling reads ----
    std::vector<float> outputCacheLeft;
    std::vector<float> outputCacheRight;

    // ---- Cached APVTS parameter pointers (set once in attachParameters) ----
    std::atomic<float>* pOscMode      = nullptr;
    std::atomic<float>* pSnap         = nullptr;
    std::atomic<float>* pDecay        = nullptr;
    std::atomic<float>* pFilterCutoff    = nullptr;
    std::atomic<float>* pFilterReso      = nullptr;
    std::atomic<float>* pFilterEnvDepth  = nullptr;
    std::atomic<float>* pDetune       = nullptr;
    std::atomic<float>* pLevel        = nullptr;
    std::atomic<float>* pPitchLock       = nullptr;
    std::atomic<float>* pSweepDirection  = nullptr;
    std::atomic<float>* pUnison          = nullptr;
    std::atomic<float>* pPolyphony    = nullptr;
    // D002: user-exposed LFO depth pointers
    std::atomic<float>* pLfoDepth     = nullptr;  // snap_lfoDepth — BPF wobble depth (0–0.5)
    std::atomic<float>* pLfo2Depth    = nullptr;  // snap_lfo2Depth — stereo pan wobble depth (0–0.2)
    // D005: user-exposed LFO rate pointers (floor 0.01 Hz — engine can breathe)
    std::atomic<float>* pLfoRate      = nullptr;  // snap_lfoRate — BPF wobble rate (0.01–20 Hz, default 4.0)
    std::atomic<float>* pLfo2Rate     = nullptr;  // snap_lfo2Rate — stereo pan rate (0.01–8 Hz, default 0.08)
    std::atomic<float>* pMacroDart    = nullptr;
    std::atomic<float>* pMacroSchool  = nullptr;
    std::atomic<float>* pMacroSurface = nullptr;
    std::atomic<float>* pMacroDepth   = nullptr;
};

} // namespace xolokun
