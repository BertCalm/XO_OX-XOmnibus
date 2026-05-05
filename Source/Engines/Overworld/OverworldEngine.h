// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/ModMatrix.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/SRO/SilenceGate.h"

// XOverworld DSP — included via target_include_directories in CMakeLists.txt
#include "engine/VoicePool.h"
#include "dsp/SVFilter.h"
#include "dsp/GlitchEngine.h"
#include "dsp/FIREcho.h"
#include "dsp/BitCrusher.h"

namespace xoceanus
{

//==============================================================================
// OverworldEngine — XOverworld chip synthesis engine for XOceanus
//
// Wraps XOverworld's 6-engine chip synthesizer (NES / FM Genesis / SNES /
// Game Boy / PC Engine / Neo Geo) behind the SynthEngine interface.
//
// The ERA triangle pad lets any 3 of the 6 chip engines sit at vertices A/B/C,
// with barycentric blending across the space. ERA Portamento and ERA Memory
// are preserved. 8-voice polyphony with drum mode via DrumKitManager.
//
// Signal chain: VoicePool → SVFilter → BitCrusher → GlitchEngine → FIREcho → Out
//
// Coupling inputs mapped:
//   AmpToFilter  → external filter cutoff offset (Hz)
//   EnvToMorph   → external ERA X offset (chip blend position)
//   AudioToFM    → external ERA Y offset (drives SNES/FM vertex blend within era)
//                  Note: AudioToFM drives ERA Y, not literal FM index, because
//                  Overworld's FM is inside the chip era — ERA Y is the closest
//                  coupling target for "external audio drives FM character".
//
class OverworldEngine : public SynthEngine
{
public:
    OverworldEngine() = default;
    ~OverworldEngine() = default;

    //-- Identity ---------------------------------------------------------------

    juce::String getEngineId() const override { return "Overworld"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF39FF14); }
    int getMaxVoices() const override { return 8; }

    //-- Lifecycle --------------------------------------------------------------

    void prepare(double sampleRate, int maxBlockSize) override
    {
        // P37: guard against sr=0 — avoids freq/sr=Inf in VoicePool phase increments
        if (sampleRate <= 0.0 || !std::isfinite(sampleRate))
        {
            jassertfalse;
            return;
        }
        sr = static_cast<float>(sampleRate);
        voicePool.prepare(sr);
        filter.prepare(sr);
        glitch.prepare(sr);
        echo.prepare(sr);
        crusher.reset();
        aftertouch.prepare(sampleRate);
        eraSmooth = 0.0f;
        eraYSmooth = 0.0f;
        // P0-05 fix: allocate per-sample output cache for coupling reads
        outputCacheLeft.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheRight.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        // Haas micro-delay: ~0.3ms at any SR (16 samples @ 44.1kHz, 29 @ 96kHz)
        haasDelaySamples = static_cast<int>(0.0003 * sampleRate) + 1;
        haasDelayBuf.assign(static_cast<size_t>(haasDelaySamples), 0.0f);
        haasWritePos = 0;
        silenceGate.prepare(sampleRate, maxBlockSize);
    }

    void releaseResources() override
    {
        voicePool.allNotesOff();
        // FIX-V2: clear FX tail buffers on stream stop to prevent stale state
        // appearing on next prepare() call. GlitchEngine and FIREcho hold large
        // static arrays that don't auto-clear between audio sessions.
        if (sr > 0.0f)
        {
            glitch.prepare(sr);
            echo.prepare(sr);
        }
    }

    void reset() override
    {
        voicePool.reset();
        // FIX-V1: reset FX state on preset change to prevent resonance bleed
        // across presets — filter, glitch buffer, and echo delay line all
        // contain state from the previous preset without this reset.
        filter.reset();
        glitch.prepare(sr);
        echo.prepare(sr);
        eraSmooth = 0.0f;
        eraYSmooth = 0.0f;
        eraPhase = 0.0f;
        lastSample = 0.0f;
        filterEnvLevel = 0.0f;
        // OVW-02/OVW-03/OVW-06/OVW-08/OVW-09/OVW-10 fix: reset all expression, mod matrix,
        // ghost ERA, filter delta-guard sentinel, and coupling accumulator state on
        // preset change to prevent bleed across presets.
        pitchBendNorm       = 0.0f;
        lastFilterCutoffHz  = -1.0f; // force coefficient recompute on first block
        modWheelAmount    = 0.0f;
        owModEraOffset    = 0.0f;
        owModGlitchOffset = 0.0f;
        owModPitchOffset  = 0.0f;
        owModLevelOffset  = 0.0f;
        ghostEraLast      = 0.0f;
        ghostEraYLast     = 0.0f;
        eraDriftSHValue   = 0.0f;
        externalFilterMod = 0.0f;
        externalEraMod    = 0.0f;
        externalEraYMod   = 0.0f;
        // Wrap haasWritePos to prevent int overflow on long sessions
        haasWritePos = 0;
        if (!haasDelayBuf.empty())
            std::fill(haasDelayBuf.begin(), haasDelayBuf.end(), 0.0f);
    }

    //-- Parameters ------------------------------------------------------------

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        xoverworld::addParameters(params); // delegates to Parameters.h

        // XOceanus standard macros — added here (not in standalone Parameters.h)
        // to preserve standalone XOverworld preset compatibility.
        // All default to 0.0 so existing presets are unaffected.
        //
        // M1 ERA: sweeps ERA X-axis (0→1), cross-fading the chip engine mix.
        //         At max: full ERA X traversal — NES→FM character shift.
        // M2 CRUSH: increases BitCrusher mix (0→0.85) — lo-fi degradation.
        //         At max: heavy 8-bit crunch imposed over the chip sound.
        // M3 GLITCH: increases GlitchEngine amount (0→0.9) and mix (0→0.8).
        //         At max: extreme glitch artefacts dominate the texture.
        // M4 SPACE: increases FIREcho mix (0→0.7) — deep echo tail.
        //         At max: long reverberant echo fills the stereo field.
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ow_macroEra", 1}, "Overworld ERA",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ow_macroCrush", 1}, "Overworld Crush",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ow_macroGlitch", 1}, "Overworld Glitch",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ow_macroSpace", 1}, "Overworld Space",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        // D001: filter envelope depth — note-on velocity × decaying envelope boosts
        // the master SVF cutoff. Default 0.25: at full velocity (1.0) and attack peak,
        // adds +2000 Hz above the static filter cutoff, mapping touch to brightness.
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ow_filterEnvDepth", 1}, "Overworld Filter Env Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.25f));

        // Drum kit mode — when 1.0, forces one-shot chip percussion behaviour:
        // ampSustain → 0, ampRelease → 50 ms so each hit plays out without sustain.
        // Enables correct Rhythmic classification in SoundShapeClassifier.
        // Default 0.0: existing melodic presets are unaffected.
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ow_drumMode", 1}, "Overworld Drum Mode",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f), 0.0f));

        // D002 mod matrix — 4-slot source→destination routing
        static const juce::StringArray kOwModDests {"Off", "ERA X", "Glitch Mix", "Pitch", "Amp Level"};
        ModMatrix<4>::addParameters(params, "ow_", "Overworld", kOwModDests);
    }
    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        namespace PID = xoverworld::ParamID;
        p_era = apvts.getRawParameterValue(PID::ERA);
        p_eraY = apvts.getRawParameterValue(PID::ERA_Y);
        p_voiceMode = apvts.getRawParameterValue(PID::VOICE_MODE);
        p_masterVol = apvts.getRawParameterValue(PID::MASTER_VOL);
        p_masterTune = apvts.getRawParameterValue(PID::MASTER_TUNE);

        // NES
        p_pulseDuty = apvts.getRawParameterValue(PID::PULSE_DUTY);
        p_pulseSweep = apvts.getRawParameterValue(PID::PULSE_SWEEP);
        p_triEnable = apvts.getRawParameterValue(PID::TRI_ENABLE);
        p_noiseMode = apvts.getRawParameterValue(PID::NOISE_MODE);
        p_noisePeriod = apvts.getRawParameterValue(PID::NOISE_PERIOD);
        p_dpcmEnable = apvts.getRawParameterValue(PID::DPCM_ENABLE);
        p_dpcmRate = apvts.getRawParameterValue(PID::DPCM_RATE);
        p_nesMix = apvts.getRawParameterValue(PID::NES_MIX);

        // FM
        p_fmAlgorithm = apvts.getRawParameterValue(PID::FM_ALGORITHM);
        p_fmFeedback = apvts.getRawParameterValue(PID::FM_FEEDBACK);
        p_fmOp1Level = apvts.getRawParameterValue(PID::FM_OP1_LEVEL);
        p_fmOp2Level = apvts.getRawParameterValue(PID::FM_OP2_LEVEL);
        p_fmOp3Level = apvts.getRawParameterValue(PID::FM_OP3_LEVEL);
        p_fmOp4Level = apvts.getRawParameterValue(PID::FM_OP4_LEVEL);
        p_fmOp1Mult = apvts.getRawParameterValue(PID::FM_OP1_MULT);
        p_fmOp2Mult = apvts.getRawParameterValue(PID::FM_OP2_MULT);
        p_fmOp3Mult = apvts.getRawParameterValue(PID::FM_OP3_MULT);
        p_fmOp4Mult = apvts.getRawParameterValue(PID::FM_OP4_MULT);
        p_fmOp1Detune = apvts.getRawParameterValue(PID::FM_OP1_DETUNE);
        p_fmOp2Detune = apvts.getRawParameterValue(PID::FM_OP2_DETUNE);
        p_fmOp3Detune = apvts.getRawParameterValue(PID::FM_OP3_DETUNE);
        p_fmOp4Detune = apvts.getRawParameterValue(PID::FM_OP4_DETUNE);
        p_fmAttack = apvts.getRawParameterValue(PID::FM_ATTACK);
        p_fmDecay = apvts.getRawParameterValue(PID::FM_DECAY);
        p_fmSustain = apvts.getRawParameterValue(PID::FM_SUSTAIN);
        p_fmRelease = apvts.getRawParameterValue(PID::FM_RELEASE);
        p_fmLfoRate = apvts.getRawParameterValue(PID::FM_LFO_RATE);
        p_fmLfoDepth = apvts.getRawParameterValue(PID::FM_LFO_DEPTH);

        // SNES
        p_brrSample = apvts.getRawParameterValue(PID::BRR_SAMPLE);
        p_brrInterp = apvts.getRawParameterValue(PID::BRR_INTERP);
        p_snesAttack = apvts.getRawParameterValue(PID::SNES_ATTACK);
        p_snesDecay = apvts.getRawParameterValue(PID::SNES_DECAY);
        p_snesSustain = apvts.getRawParameterValue(PID::SNES_SUSTAIN);
        p_snesRelease = apvts.getRawParameterValue(PID::SNES_RELEASE);
        p_pitchMod = apvts.getRawParameterValue(PID::PITCH_MOD);
        p_noiseReplace = apvts.getRawParameterValue(PID::NOISE_REPLACE);

        // Echo
        p_echoDelay = apvts.getRawParameterValue(PID::ECHO_DELAY);
        p_echoFeedback = apvts.getRawParameterValue(PID::ECHO_FEEDBACK);
        p_echoMix = apvts.getRawParameterValue(PID::ECHO_MIX);
        for (int i = 0; i < 8; ++i)
        {
            auto fid = "ow_echoFir" + std::to_string(i);
            p_echoFir[i] = apvts.getRawParameterValue(fid);
        }

        // Glitch
        p_glitchAmount = apvts.getRawParameterValue(PID::GLITCH_AMOUNT);
        p_glitchType = apvts.getRawParameterValue(PID::GLITCH_TYPE);
        p_glitchRate = apvts.getRawParameterValue(PID::GLITCH_RATE);
        p_glitchDepth = apvts.getRawParameterValue(PID::GLITCH_DEPTH);
        p_glitchMix = apvts.getRawParameterValue(PID::GLITCH_MIX);

        // Envelope
        p_ampAttack = apvts.getRawParameterValue(PID::AMP_ATTACK);
        p_ampDecay = apvts.getRawParameterValue(PID::AMP_DECAY);
        p_ampSustain = apvts.getRawParameterValue(PID::AMP_SUSTAIN);
        p_ampRelease = apvts.getRawParameterValue(PID::AMP_RELEASE);

        // Filter
        p_filterCutoff = apvts.getRawParameterValue(PID::FILTER_CUTOFF);
        p_filterReso = apvts.getRawParameterValue(PID::FILTER_RESO);
        p_filterType = apvts.getRawParameterValue(PID::FILTER_TYPE);

        // BitCrusher
        p_crushBits = apvts.getRawParameterValue(PID::CRUSH_BITS);
        p_crushRate = apvts.getRawParameterValue(PID::CRUSH_RATE);
        p_crushMix = apvts.getRawParameterValue(PID::CRUSH_MIX);

        // ERA extended
        p_eraDriftRate = apvts.getRawParameterValue(PID::ERA_DRIFT_RATE);
        p_eraDriftDepth = apvts.getRawParameterValue(PID::ERA_DRIFT_DEPTH);
        p_eraDriftShape = apvts.getRawParameterValue(PID::ERA_DRIFT_SHAPE);
        p_eraPortaTime = apvts.getRawParameterValue(PID::ERA_PORTA_TIME);
        p_eraMemTime = apvts.getRawParameterValue(PID::ERA_MEM_TIME);
        p_eraMemMix = apvts.getRawParameterValue(PID::ERA_MEM_MIX);

        // Vertices
        p_vertexA = apvts.getRawParameterValue(PID::VERTEX_A);
        p_vertexB = apvts.getRawParameterValue(PID::VERTEX_B);
        p_vertexC = apvts.getRawParameterValue(PID::VERTEX_C);

        // Game Boy + PC Engine
        p_gbWaveSlot = apvts.getRawParameterValue(PID::GB_WAVE_SLOT);
        p_gbPulseDuty = apvts.getRawParameterValue(PID::GB_PULSE_DUTY);
        p_pceWaveSlot = apvts.getRawParameterValue(PID::PCE_WAVE_SLOT);

        // XOceanus macros (declared above in addParameters)
        p_macroEra = apvts.getRawParameterValue("ow_macroEra");
        p_macroCrush = apvts.getRawParameterValue("ow_macroCrush");
        p_macroGlitch = apvts.getRawParameterValue("ow_macroGlitch");
        p_macroSpace = apvts.getRawParameterValue("ow_macroSpace");

        // D001: filter envelope depth
        p_filterEnvDepth = apvts.getRawParameterValue("ow_filterEnvDepth");

        p_drumMode = apvts.getRawParameterValue("ow_drumMode");

        // D002 mod matrix
        modMatrix.attachParameters(apvts, "ow_");
    }

    //-- Audio -----------------------------------------------------------------

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        // P37: sr=0.0 sentinel — renderBlock must never run without a valid prepare()
        jassert(sr > 0.0f);
        if (sr <= 0.0f) { buffer.clear(); return; }
        if (numSamples <= 0)
            return;

        // Build ParamSnapshot from cached atomics
        auto snap = buildSnapshot();

        // Drum kit mode: force one-shot chip percussion behaviour.
        // Sustain=0 ensures the voice starts releasing immediately after decay;
        // Release=50ms gives a crisp chip hit tail consistent with NES/FM percussion.
        if (snap.drumMode)
        {
            snap.ampSustain = 0.0f;
            snap.ampRelease = 0.05f;
        }

        // -- Apply XOceanus macros -------------------------------------------
        // M1 ERA: sweep ERA X-axis by up to 1.0 (full chip engine cross-fade).
        // M2 CRUSH: increase BitCrusher wet mix up to 0.85 (heavy lo-fi crunch).
        // M3 GLITCH: increase glitch amount up to 0.9 and mix up to 0.8.
        // M4 SPACE: increase FIREcho mix up to 0.7 (deep echo tail).
        // All macros default to 0.0, so existing presets are unchanged.
        const float macEra = (p_macroEra != nullptr) ? p_macroEra->load() : 0.0f;
        const float macCrush = (p_macroCrush != nullptr) ? p_macroCrush->load() : 0.0f;
        const float macGlitch = (p_macroGlitch != nullptr) ? p_macroGlitch->load() : 0.0f;
        const float macSpace = (p_macroSpace != nullptr) ? p_macroSpace->load() : 0.0f;

        // Apply macro offsets (additive, clamped to valid ranges).
        // FIX-C2: macro ERA was added into externalEraMod without clamp, letting
        // macroEra=1.0 push ERA well past 1.0 before the per-block jlimit fence.
        // Now the macro applies a direct snap offset, not a coupling accumulation.
        if (macEra > 0.001f)
            snap.era = juce::jlimit(0.0f, 1.0f, snap.era + macEra);

        const float effectiveCrushMix = juce::jlimit(0.0f, 1.0f, snap.crushMix + macCrush * 0.85f);
        const float effectiveGlitchAmt = juce::jlimit(0.0f, 1.0f, snap.glitchAmount + macGlitch * 0.9f);
        const float effectiveEchoMix = juce::jlimit(0.0f, 1.0f, snap.echoMix + macSpace * 0.7f);

        // OVW-07 fix: apply mod matrix HERE — before filter/glitch setters — so that
        // owModPitchOffset and owModGlitchOffset reflect the current block's aftertouch
        // and mod wheel, not last block's stale values. Previously the mod matrix ran
        // ~130 lines later (after the silence-gate return path) causing one-block lag
        // on all mod destinations including filter pitch and glitch mix.
        //
        // Pre-scan MIDI for expression messages (mod wheel, aftertouch, pitch bend)
        // so these take effect in the current block's mod matrix. Note-on/off are
        // processed in the second MIDI pass below (after silence-gate check).
        for (const auto meta : midi)
        {
            const auto msg = meta.getMessage();
            if (msg.isChannelPressure())
                aftertouch.setChannelPressure(msg.getChannelPressureValue() / 127.0f);
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount = msg.getControllerValue() / 127.0f;
            else if (msg.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
        }
        {
            // Aftertouch must be updated before mod matrix (needs smoothed pressure).
            aftertouch.updateBlock(numSamples);
            const float atPressureForMod = aftertouch.getSmoothedPressure(0);

            // D002 mod matrix — apply per-block.
            // Destinations: 0=Off, 1=ERA_X, 2=GlitchMix, 3=Pitch, 4=AmpLevel
            ModMatrix<4>::Sources mSrc;
            mSrc.lfo1       = 0.0f;
            mSrc.lfo2       = 0.0f;
            mSrc.env        = 0.0f;
            mSrc.velocity   = 0.0f;
            mSrc.keyTrack   = 0.0f;
            mSrc.modWheel   = modWheelAmount;
            mSrc.aftertouch = atPressureForMod;
            float mDst[5]   = {};
            modMatrix.apply(mSrc, mDst);
            owModEraOffset    = mDst[1] * 0.5f;
            owModGlitchOffset = mDst[2] * 0.4f;
            owModPitchOffset  = mDst[3] * 12.0f;
            owModLevelOffset  = mDst[4] * 0.5f;
        }

        // D006: mod wheel adds up to +0.4 to glitch mix (CC#1 introduces chip artifacts progressively)
        // D002: owModGlitchOffset adds mod matrix contribution (now current-block values)
        const float effectiveGlitchMix =
            juce::jlimit(0.0f, 1.0f, snap.glitchMix + macGlitch * 0.8f + modWheelAmount * 0.4f + owModGlitchOffset);

        // D001: filter envelope — simple one-pole decay tracks note-on velocity.
        // filterEnvLevel is set to lastNoteOnVelocity on noteOn (detected below),
        // then decays per-block at a fixed rate (~400ms half-life at typical block sizes).
        // kOwFilterEnvDecay = 0.9975 per sample ≈ 200ms at 44.1kHz/128 block.
        // kOwFilterEnvMaxHz = 8000 Hz sweep range for the master SVF cutoff.
        {
            static constexpr float kOwFilterEnvMaxHz = 8000.0f;
            const float filterEnvDepth = (p_filterEnvDepth != nullptr) ? p_filterEnvDepth->load() : 0.25f;

            // Scan MIDI for incoming noteOn to update velocity target.
            // Note: only retrigger if a noteOn is found; don't reset between
            // multiple noteOns — we take the last velocity in the block
            // (authentic chip behaviour: last event wins).
            for (const auto meta : midi)
            {
                const auto msg = meta.getMessage();
                if (msg.isNoteOn())
                    filterEnvLevel = msg.getFloatVelocity();
            }
            // Decay: per-block coefficient = perSampleCoeff^numSamples.
            // FIX-S1: previously applied per-sample coefficient once per block,
            // making decay speed block-size-dependent (64-sample block decayed
            // same as 512-sample block). Now correct regardless of buffer size.
            // Block-form decay: exp(-numSamples / (0.2*sr)) is exact for any block size.
            const float blockDecayCoeff = fastExp(
                static_cast<float>(-numSamples) * (1.0f / (0.2f * sr)));
            filterEnvLevel *= blockDecayCoeff;

            const float filterEnvBoost = filterEnvDepth * filterEnvLevel * kOwFilterEnvMaxHz;

            // Update FX units from snapshot (cheap, param-only, no alloc)
            filter.setMode(snap.filterType);
            // OVW-01 fix: clamp to sr*0.49 not 20000Hz — at 96kHz the old ceiling
            // let the cutoff sit well below Nyquist; at 44.1kHz it was fine but users
            // with 48kHz/96kHz interfaces got a 20kHz hard clip instead of Nyquist-safe.
            {
                const float newCutoff =
                    juce::jlimit(20.0f, sr * 0.49f,
                                 snap.filterCutoff * PitchBendUtil::semitonesToFreqRatio(pitchBendNorm * 2.0f + owModPitchOffset) +
                                     externalFilterMod + filterEnvBoost);
                // OVW-03 fix: delta-guard — skip recomputing TPT coefficients (fastTan)
                // when the cutoff has not changed by more than 0.5 Hz since last block.
                if (std::abs(newCutoff - lastFilterCutoffHz) > 0.5f)
                {
                    filter.setCutoff(newCutoff);
                    lastFilterCutoffHz = newCutoff;
                }
            }
            filter.setResonance(snap.filterReso);
        }

        glitch.setAmount(effectiveGlitchAmt);
        glitch.setType(snap.glitchType);
        glitch.setRate(snap.glitchRate);
        glitch.setDepth(snap.glitchDepth);
        glitch.setMix(effectiveGlitchMix);

        echo.setDelay(snap.echoDelay);
        echo.setFeedback(snap.echoFeedback);
        echo.setMix(effectiveEchoMix);
        echo.setFIRCoefficients(snap.echoFir);

        crusher.setBits(snap.crushBits);
        crusher.setRateDivider(snap.crushRate);
        crusher.setMix(effectiveCrushMix);

        voicePool.applyParams(snap);

        // Process MIDI
        // Second MIDI pass: note-on / note-off only.
        // Expression messages (aftertouch, mod wheel, pitch bend) were already
        // consumed by the pre-scan above so all mod destinations use current values.
        for (const auto meta : midi)
        {
            const auto msg = meta.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                voicePool.noteOn(msg.getNoteNumber(), msg.getVelocity() / 127.0f);
            }
            else if (msg.isNoteOff())
                voicePool.noteOff(msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                voicePool.allNotesOff();
        }

        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // OVW-07 fix: aftertouch.updateBlock() and mod matrix are now applied earlier
        // (before the filter/glitch setup section) so all mod destinations use
        // current-block values. Read the smoothed pressure computed there for ERA Y.
        const float atPressure = aftertouch.getSmoothedPressure(0);

        // D006: aftertouch raises ERA Y — more SNES chip character under pressure (sensitivity 0.2).
        // ERA Y drives the SNES vertex weight in the 3-chip barycentric blend.
        // Full pressure adds up to +0.2 to targetEraY, nudging the triangle toward SNES.
        // Clamped to [0.0, 1.0] so it never exceeds valid ERA range.

        // D005 fix: advance ERA drift phase per block.
        // FIX-P1: removed redundant `if (eraPhase >= 1.0f) eraPhase -= 1.0f`
        // that was dead code after std::fmod — fmod guarantees [0, 1) output.
        // FIX-PA3: track phase wrap for S&H shape refresh.
        if (snap.eraDriftRate > 0.001f)
        {
            float newPhase = eraPhase + snap.eraDriftRate * numSamples / sr;
            if (newPhase >= 1.0f && snap.eraDriftShape == 3)
            {
                // S&H: generate a new random value each time phase wraps.
                // LCG random in [-1, 1]. State persists in eraDriftSHSeed.
                eraDriftSHSeed = eraDriftSHSeed * 1664525u + 1013904223u;
                eraDriftSHValue = (float)(int32_t)(eraDriftSHSeed >> 8) * (1.0f / 8388608.0f);
                eraDriftSHValue = juce::jlimit(-1.0f, 1.0f, eraDriftSHValue);
            }
            eraPhase = std::fmod(newPhase, 1.0f);
        }

        // ERA portamento: one-pole IIR smoothing (owModEraOffset adds D002 mod matrix contribution).
        // FIX-PA3: eraDriftShape was attached but ignored — drift was always sine.
        // Now respects the shape selector:
        //   0 = sine       (smooth, organic era drift)
        //   1 = triangle   (linear ramp up/down — linear sweep feel)
        //   2 = square     (abrupt era jumps — glitchy step character)
        //   3 = S&H        (per-cycle random step — stochastic chip morphing)
        float driftLFO = 0.0f;
        {
            const float driftPhaseRad = eraPhase * juce::MathConstants<float>::twoPi;
            switch (snap.eraDriftShape)
            {
            case 1: // triangle: 0→1→0 over one cycle
                driftLFO = (eraPhase < 0.5f) ? (4.0f * eraPhase - 1.0f) : (3.0f - 4.0f * eraPhase);
                break;
            case 2: // square: +1 first half, -1 second half
                driftLFO = (eraPhase < 0.5f) ? 1.0f : -1.0f;
                break;
            case 3: // S&H: re-seed when phase wraps (new random value each cycle)
                // eraDriftSH is updated once per phase reset (tracked via sign of phase increment)
                driftLFO = eraDriftSHValue;
                break;
            default: // 0 = sine
                driftLFO = fastSin(driftPhaseRad);
                break;
            }
        }
        float targetEra =
            juce::jlimit(0.0f, 1.0f,
                         snap.era + externalEraMod + owModEraOffset +
                             snap.eraDriftDepth * 0.35f * driftLFO);
        float targetEraY = juce::jlimit(0.0f, 1.0f, snap.eraY + externalEraYMod + atPressure * 0.2f);

        float portaCoeff = 1.0f;
        if (snap.eraPortaTime > 0.001f)
            portaCoeff = 1.0f - fastExp(-1.0f / (sr * snap.eraPortaTime));

        // ERA Memory ghost layer position: intentionally uses the *previous* block's
        // smoothed ERA to create a temporal echo of the blend position. This implements
        // the "memory" semantics — ghost lags behind current by one block.
        // FIX-S2: was capturing eraSmooth BEFORE the per-sample smoothing loop
        // but eraSmooth is updated inside the loop, so for all but the first block
        // the ghost was already up-to-date (defeating the shimmer effect).
        // Now ghostEra is captured at the top of renderBlock() and updated AFTER
        // the loop, ensuring it always trails by exactly one block.
        float ghostEra  = ghostEraLast;
        float ghostEraY = ghostEraYLast;

        // Render per sample
        auto* L = buffer.getWritePointer(0);
        auto* R = buffer.getWritePointer(1 % buffer.getNumChannels());

        for (int s = 0; s < numSamples; ++s)
        {
            eraSmooth = flushDenormal(eraSmooth + portaCoeff * (targetEra - eraSmooth));
            eraYSmooth = flushDenormal(eraYSmooth + portaCoeff * (targetEraY - eraYSmooth));

            float sample = voicePool.process(eraSmooth, eraYSmooth, ghostEra, ghostEraY, snap.eraMemMix);

            sample = filter.process(sample);
            sample = crusher.process(sample);
            sample = glitch.process(sample);
            sample = echo.process(sample);

            // FIX-S3: lower clamp was 0.05 — prevented silence when user sets
            // masterVol to 0.0 or when owModLevelOffset pushes below threshold.
            // Use 0.0 minimum; 1.5 headroom ceiling retained for loud patches.
            sample *= juce::jlimit(0.0f, 1.5f, snap.masterVol + owModLevelOffset);

            // DSP Fix Wave 2B: Stereo widening — Haas-style micro-delay for width.
            // OVERWORLD's VoicePool is mono by design (chip engines sum to one channel).
            // We add a subtle stereo image via a tiny delay on the right channel (~0.3ms)
            // plus phase-inverted ERA drift for organic width. This lifts the mono
            // collapse flagged in the 7.6 seance without altering the mono character.
            {
                // Micro-delay right channel: ~0.3ms Haas effect (SR-scaled in prepare).
                // FIX-ST1: haasWritePos is now modulo-wrapped each sample to prevent
                // int overflow after ~50 days of continuous 44.1kHz operation.
                float haasOut = haasDelayBuf[static_cast<size_t>(haasWritePos)];
                haasDelayBuf[static_cast<size_t>(haasWritePos)] = sample;
                haasWritePos = (haasWritePos + 1) % haasDelaySamples;

                // ERA drift adds subtle stereo decorrelation
                float eraStereo =
                    snap.eraDriftDepth * 0.02f * fastSin(eraPhase * juce::MathConstants<float>::twoPi * 1.3f);

                L[s] = sample * (1.0f + eraStereo);
                R[s] = haasOut * (1.0f - eraStereo);
            }

            // P0-05 fix: cache per-sample output for getSampleForCoupling.
            if (s < static_cast<int>(outputCacheLeft.size()))
            {
                outputCacheLeft[static_cast<size_t>(s)] = L[s];
                outputCacheRight[static_cast<size_t>(s)] = R[s];
            }
        }

        lastSample = L[numSamples - 1];

        // Update ghost ERA for next block (one block of trailing delay)
        ghostEraLast  = eraSmooth;
        ghostEraYLast = eraYSmooth;

        silenceGate.analyzeBlock(buffer.getReadPointer(0),
                                 buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : buffer.getReadPointer(0),
                                 numSamples);

        // Reset per-block coupling accumulators
        externalFilterMod = 0.0f;
        externalEraMod = 0.0f;
        externalEraYMod = 0.0f;
    }

    //-- Coupling --------------------------------------------------------------

    // P0-05 fix: return proper per-sample, per-channel output from cache
    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        if (sampleIndex < 0)
            return 0.0f;
        auto index = static_cast<size_t>(sampleIndex);
        if (channel == 0 && index < outputCacheLeft.size())
            return outputCacheLeft[index];
        if (channel == 1 && index < outputCacheRight.size())
            return outputCacheRight[index];
        return lastSample;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
        case CouplingType::AmpToFilter:
            // Treat amount as a fractional filter cutoff offset (±8000 Hz range)
            externalFilterMod += amount * 8000.0f;
            break;

        case CouplingType::EnvToMorph:
            // Drive ERA X position (cross-era blend).
            // FIX-C1: clamp accumulated ERA mod to ±1 so back-to-back coupling
            // calls can't drive the era position beyond the [0,1] jlimit fence.
            externalEraMod = juce::jlimit(-1.0f, 1.0f, externalEraMod + amount * 0.5f);
            break;

        case CouplingType::AudioToFM:
            // Drive ERA Y position (raises FM/SNES vertex blend within era).
            // FIX-C1: same clamp as externalEraMod above.
            externalEraYMod = juce::jlimit(-1.0f, 1.0f, externalEraYMod + amount * 0.5f);
            break;

        default:
            break;
        }
    }

private:
    SilenceGate silenceGate;

    xoverworld::ParamSnapshot buildSnapshot() const
    {
        xoverworld::ParamSnapshot s;
        if (!p_era)
            return s; // not yet attached

        s.era = p_era->load();
        s.eraY = p_eraY->load();
        s.voiceMode = static_cast<int>(p_voiceMode->load());
        s.masterVol = p_masterVol->load();
        s.masterTune = p_masterTune->load();

        s.pulseDuty = static_cast<int>(p_pulseDuty->load());
        s.pulseSweep = p_pulseSweep->load();
        s.triEnable = p_triEnable->load() > 0.5f;
        s.noiseMode = static_cast<int>(p_noiseMode->load());
        s.noisePeriod = static_cast<int>(p_noisePeriod->load());
        s.dpcmEnable = p_dpcmEnable->load() > 0.5f;
        s.dpcmRate = static_cast<int>(p_dpcmRate->load());
        s.nesMix = p_nesMix->load();

        s.fmAlgorithm = static_cast<int>(p_fmAlgorithm->load());
        s.fmFeedback = static_cast<int>(p_fmFeedback->load());
        s.fmOpLevel[0] = static_cast<int>(p_fmOp1Level->load());
        s.fmOpLevel[1] = static_cast<int>(p_fmOp2Level->load());
        s.fmOpLevel[2] = static_cast<int>(p_fmOp3Level->load());
        s.fmOpLevel[3] = static_cast<int>(p_fmOp4Level->load());
        s.fmOpMult[0] = static_cast<int>(p_fmOp1Mult->load());
        s.fmOpMult[1] = static_cast<int>(p_fmOp2Mult->load());
        s.fmOpMult[2] = static_cast<int>(p_fmOp3Mult->load());
        s.fmOpMult[3] = static_cast<int>(p_fmOp4Mult->load());
        s.fmOpDetune[0] = static_cast<int>(p_fmOp1Detune->load());
        s.fmOpDetune[1] = static_cast<int>(p_fmOp2Detune->load());
        s.fmOpDetune[2] = static_cast<int>(p_fmOp3Detune->load());
        s.fmOpDetune[3] = static_cast<int>(p_fmOp4Detune->load());
        s.fmAttack = static_cast<int>(p_fmAttack->load());
        s.fmDecay = static_cast<int>(p_fmDecay->load());
        s.fmSustain = static_cast<int>(p_fmSustain->load());
        s.fmRelease = static_cast<int>(p_fmRelease->load());
        s.fmLfoRate = static_cast<int>(p_fmLfoRate->load());
        s.fmLfoDepth = static_cast<int>(p_fmLfoDepth->load());

        s.brrSample = static_cast<int>(p_brrSample->load());
        s.brrInterp = static_cast<int>(p_brrInterp->load());
        s.snesAttack = static_cast<int>(p_snesAttack->load());
        s.snesDecay = static_cast<int>(p_snesDecay->load());
        s.snesSustain = static_cast<int>(p_snesSustain->load());
        s.snesRelease = static_cast<int>(p_snesRelease->load());
        s.pitchMod = p_pitchMod->load() > 0.5f;
        s.noiseReplace = p_noiseReplace->load() > 0.5f;

        s.echoDelay = static_cast<int>(p_echoDelay->load());
        s.echoFeedback = p_echoFeedback->load();
        s.echoMix = p_echoMix->load();
        for (int i = 0; i < 8; ++i)
            if (p_echoFir[i])
                s.echoFir[i] = p_echoFir[i]->load();

        s.glitchAmount = p_glitchAmount->load();
        s.glitchType = static_cast<int>(p_glitchType->load());
        s.glitchRate = p_glitchRate->load();
        s.glitchDepth = p_glitchDepth->load();
        s.glitchMix = p_glitchMix->load();

        s.ampAttack = p_ampAttack->load();
        s.ampDecay = p_ampDecay->load();
        s.ampSustain = p_ampSustain->load();
        s.ampRelease = p_ampRelease->load();

        s.filterCutoff = p_filterCutoff->load();
        s.filterReso = p_filterReso->load();
        s.filterType = static_cast<int>(p_filterType->load());

        s.crushBits = p_crushBits->load();
        s.crushRate = static_cast<int>(p_crushRate->load());
        s.crushMix = p_crushMix->load();

        s.eraDriftRate = p_eraDriftRate->load();
        s.eraDriftDepth = p_eraDriftDepth->load();
        s.eraDriftShape = static_cast<int>(p_eraDriftShape->load());
        s.eraPortaTime = p_eraPortaTime->load();
        s.eraMemTime = p_eraMemTime->load();
        s.eraMemMix = p_eraMemMix->load();

        s.vertexA = static_cast<int>(p_vertexA->load());
        s.vertexB = static_cast<int>(p_vertexB->load());
        s.vertexC = static_cast<int>(p_vertexC->load());

        s.gbWaveSlot = static_cast<int>(p_gbWaveSlot->load());
        s.gbPulseDuty = static_cast<int>(p_gbPulseDuty->load());
        s.pceWaveSlot = static_cast<int>(p_pceWaveSlot->load());

        s.drumMode = p_drumMode ? p_drumMode->load() > 0.5f : false;

        // FIX-S4: pitch bend + mod-matrix pitch offset written into snapshot so
        // VoicePool::applyParams() can update live voice frequencies each block.
        // pitchBendNorm is ±1 with ±2 semitone range; owModPitchOffset is in semitones.
        s.pitchBendSemitones = pitchBendNorm * 2.0f + owModPitchOffset;

        return s;
    }

    // DSP subsystems
    xoverworld::VoicePool voicePool;
    xoverworld::SVFilter filter;
    xoverworld::GlitchEngine glitch;
    xoverworld::FIREcho echo;
    xoverworld::BitCrusher crusher;
    PolyAftertouch aftertouch;

    // ---- D006 Mod wheel — CC#1 increases glitch mix (+0–0.4, chip artifacts progressively) ----
    float modWheelAmount = 0.0f;

    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)
    float eraSmooth = 0.0f;
    float eraYSmooth = 0.0f;
    float lastSample = 0.0f;

    // D005 fix: minimal LFO added — ERA position drift at eraDriftRate Hz
    float eraPhase = 0.0f;

    // FIX-S2: ghost ERA lags by one block — captures eraSmooth at end of block,
    // feeds it as ghostEra at start of the next block for true temporal shimmer.
    float ghostEraLast  = 0.0f;
    float ghostEraYLast = 0.0f;

    // FIX-PA3: S&H state for eraDriftShape == 3
    uint32_t eraDriftSHSeed  = 0xACE1u;
    float    eraDriftSHValue = 0.0f;

    // DSP Fix Wave 2B: Haas stereo widening (~0.3ms delay on R channel)
    // Buffer size is computed in prepare() as ~0.3ms at the actual sample rate
    // so the Haas effect length is SR-independent.
    int haasDelaySamples = 16; // updated in prepare()
    std::vector<float> haasDelayBuf;
    int haasWritePos = 0;

    // P0-05 fix: per-sample output cache for getSampleForCoupling
    std::vector<float> outputCacheLeft;
    std::vector<float> outputCacheRight;

    float pitchBendNorm = 0.0f; // MIDI pitch wheel [-1, +1]; ±2 semitone range

    // OVW-03 fix: last computed filter cutoff for delta-guard.
    // setCutoff() calls computeCoefficients() which calls fastTan() — skipping this
    // when the cutoff hasn't moved by >0.5 Hz saves a non-trivial fastTan per block.
    float lastFilterCutoffHz = -1.0f; // sentinel: force first compute

    // Per-block coupling accumulators (reset each renderBlock)
    float externalFilterMod = 0.0f;
    float externalEraMod = 0.0f;
    float externalEraYMod = 0.0f;

    // Cached APVTS parameter pointers (ParamSnapshot pattern)
    std::atomic<float>* p_era = nullptr;
    std::atomic<float>* p_eraY = nullptr;
    std::atomic<float>* p_voiceMode = nullptr;
    std::atomic<float>* p_masterVol = nullptr;
    std::atomic<float>* p_masterTune = nullptr;

    std::atomic<float>* p_pulseDuty = nullptr;
    std::atomic<float>* p_pulseSweep = nullptr;
    std::atomic<float>* p_triEnable = nullptr;
    std::atomic<float>* p_noiseMode = nullptr;
    std::atomic<float>* p_noisePeriod = nullptr;
    std::atomic<float>* p_dpcmEnable = nullptr;
    std::atomic<float>* p_dpcmRate = nullptr;
    std::atomic<float>* p_nesMix = nullptr;

    std::atomic<float>* p_fmAlgorithm = nullptr;
    std::atomic<float>* p_fmFeedback = nullptr;
    std::atomic<float>* p_fmOp1Level = nullptr;
    std::atomic<float>* p_fmOp2Level = nullptr;
    std::atomic<float>* p_fmOp3Level = nullptr;
    std::atomic<float>* p_fmOp4Level = nullptr;
    std::atomic<float>* p_fmOp1Mult = nullptr;
    std::atomic<float>* p_fmOp2Mult = nullptr;
    std::atomic<float>* p_fmOp3Mult = nullptr;
    std::atomic<float>* p_fmOp4Mult = nullptr;
    std::atomic<float>* p_fmOp1Detune = nullptr;
    std::atomic<float>* p_fmOp2Detune = nullptr;
    std::atomic<float>* p_fmOp3Detune = nullptr;
    std::atomic<float>* p_fmOp4Detune = nullptr;
    std::atomic<float>* p_fmAttack = nullptr;
    std::atomic<float>* p_fmDecay = nullptr;
    std::atomic<float>* p_fmSustain = nullptr;
    std::atomic<float>* p_fmRelease = nullptr;
    std::atomic<float>* p_fmLfoRate = nullptr;
    std::atomic<float>* p_fmLfoDepth = nullptr;

    std::atomic<float>* p_brrSample = nullptr;
    std::atomic<float>* p_brrInterp = nullptr;
    std::atomic<float>* p_snesAttack = nullptr;
    std::atomic<float>* p_snesDecay = nullptr;
    std::atomic<float>* p_snesSustain = nullptr;
    std::atomic<float>* p_snesRelease = nullptr;
    std::atomic<float>* p_pitchMod = nullptr;
    std::atomic<float>* p_noiseReplace = nullptr;

    std::atomic<float>* p_echoDelay = nullptr;
    std::atomic<float>* p_echoFeedback = nullptr;
    std::atomic<float>* p_echoMix = nullptr;
    std::atomic<float>* p_echoFir[8] = {};

    std::atomic<float>* p_glitchAmount = nullptr;
    std::atomic<float>* p_glitchType = nullptr;
    std::atomic<float>* p_glitchRate = nullptr;
    std::atomic<float>* p_glitchDepth = nullptr;
    std::atomic<float>* p_glitchMix = nullptr;

    std::atomic<float>* p_ampAttack = nullptr;
    std::atomic<float>* p_ampDecay = nullptr;
    std::atomic<float>* p_ampSustain = nullptr;
    std::atomic<float>* p_ampRelease = nullptr;

    std::atomic<float>* p_filterCutoff = nullptr;
    std::atomic<float>* p_filterReso = nullptr;
    std::atomic<float>* p_filterType = nullptr;

    std::atomic<float>* p_crushBits = nullptr;
    std::atomic<float>* p_crushRate = nullptr;
    std::atomic<float>* p_crushMix = nullptr;

    std::atomic<float>* p_eraDriftRate = nullptr;
    std::atomic<float>* p_eraDriftDepth = nullptr;
    std::atomic<float>* p_eraDriftShape = nullptr;
    std::atomic<float>* p_eraPortaTime = nullptr;
    std::atomic<float>* p_eraMemTime = nullptr;
    std::atomic<float>* p_eraMemMix = nullptr;

    std::atomic<float>* p_vertexA = nullptr;
    std::atomic<float>* p_vertexB = nullptr;
    std::atomic<float>* p_vertexC = nullptr;

    std::atomic<float>* p_gbWaveSlot = nullptr;
    std::atomic<float>* p_gbPulseDuty = nullptr;
    std::atomic<float>* p_pceWaveSlot = nullptr;

    // XOceanus macros (registered in addParameters, attached in attachParameters)
    std::atomic<float>* p_macroEra = nullptr;
    std::atomic<float>* p_macroCrush = nullptr;
    std::atomic<float>* p_macroGlitch = nullptr;
    std::atomic<float>* p_macroSpace = nullptr;

    // D001: filter envelope depth + per-block envelope level tracker
    std::atomic<float>* p_filterEnvDepth = nullptr;
    float filterEnvLevel = 0.0f; // decays per-block, set to velocity on noteOn

    std::atomic<float>* p_drumMode = nullptr;

    // D002 mod matrix — 4-slot configurable modulation routing
    ModMatrix<4> modMatrix;
    float owModEraOffset    = 0.0f;
    float owModGlitchOffset = 0.0f;
    float owModPitchOffset  = 0.0f;
    float owModLevelOffset  = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OverworldEngine)
};

} // namespace xoceanus
