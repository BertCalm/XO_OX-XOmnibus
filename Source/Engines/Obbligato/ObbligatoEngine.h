#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FamilyWaveguide.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/StandardLFO.h"
#include <array>
#include <cmath>
#include <vector>

//==============================================================================
// ObbligatoEngine — Dual-voice wind instrument synthesizer
//
// XObbligato ("obligation") models two wind instrument families in a single
// engine: Brother A (flute family) driven by AirJetExciter, and Brother B
// (reed family) driven by ReedExciter. Both voices share waveguide synthesis
// primitives from FamilyWaveguide.h (also used by OHM, ORPHICA, OTTONI, OLE).
//
// Concept: the two brothers are emotionally entangled through the BOND system —
// 8 emotional stages (Harmony → Transcend) that modulate breath, detune,
// sympathetic resonance, and stereo spread simultaneously. MISCHIEF adds
// controlled chaos to the inter-brother tuning relationship.
//
// Architecture:
//   FamilyDelayLine    — waveguide pitch delay (Lagrange interpolated)
//   FamilyDampingFilter — one-pole LP for energy absorption in feedback path
//   FamilyBodyResonance — 2-pole biquad instrument-body modes
//   FamilySympatheticBank — 8-comb sympathetic resonance network
//   FamilyOrganicDrift  — golden-ratio dual-LFO pitch wander (humanization)
//   AirJetExciter      — filtered noise burst with breath pressure modulation
//   ReedExciter        — tanh nonlinear valve model with mechanical LP filter
//
// Voice routing: 5 modes (Alternate / Split at C4 / Layer / Round Robin / Velocity)
// Voice stealing: 5ms linear crossfade before incoming note starts
// FX chain A "The Air":   chorus → bright delay → plate → air exciter
// FX chain B "The Water": phaser → dark delay → spring reverb → tape saturation
//
// Coupling inputs (SP7.3):
//   LFOToPitch  → semitone pitch offset (extPitchMod)
//   AmpToFilter → damping coefficient increase (extDampMod)
//   EnvToMorph  → intensity multiplier on exciter output (extIntens)
//
// Accent colour: Rascal Coral #FF8A7A
// Parameter prefix: obbl_
//==============================================================================

namespace xoceanus {

//==============================================================================
// ObbligatoAdapterVoice — single polyphonic voice
//
// Each voice carries its own complete signal chain:
//   AirJetExciter or ReedExciter → FamilyDelayLine ↔ FamilyDampingFilter
//   → FamilyBodyResonance + FamilySympatheticBank + FamilyOrganicDrift
//
// isBroA=true  → Brother A (flute family), AirJetExciter
// isBroA=false → Brother B (reed family), ReedExciter
//
// Voice-steal crossfade (5 ms): when a new note hits an active voice, the
// voice ramps its output to zero over 5ms via stealFadeGain/stealFadeStep,
// then calls noteOn(pendingNote, pendingVel) and resumes normally.
//==============================================================================
struct ObbligatoAdapterVoice
{
    // --- State flags ---
    bool  active    = false;
    bool  isBroA    = true;     // true = Brother A (flute), false = Brother B (reed)

    // --- Note state ---
    int   note  = 0;
    float vel   = 0.0f;
    float freq  = 440.0f;
    float sr    = 44100.0f;

    // --- Amplitude envelope: StandardADSR replaces the former linear ampEnv float ---
    // Default: 10ms attack, 100ms decay, 0.7 sustain, 400ms release.
    // Removes the noteOn click (flat sustain with no attack stage).
    StandardADSR adsr;

    // --- Voice-steal crossfade (5 ms linear fade-out before new note starts) ---
    float stealFadeGain  = 1.0f;    // multiplied on the output sample
    float stealFadeStep  = 0.0f;    // amount subtracted from stealFadeGain each sample
    bool  isBeingStolen  = false;   // true while fading out for incoming note
    int   pendingNote    = 0;       // MIDI note for the incoming note
    float pendingVel     = 0.0f;    // velocity for the incoming note (0..1)

    // --- DSP modules ---
    FamilyDelayLine      delayLine;     // waveguide pitch delay
    FamilyDampingFilter  dampFilter;    // one-pole LP for energy absorption
    FamilyBodyResonance  bodyResonator; // 2-pole biquad instrument-body modes
    FamilySympatheticBank sympBank;     // 8-comb sympathetic resonance
    FamilyOrganicDrift   organicDrift;  // humanizing pitch wander
    AirJetExciter        airJet;        // Brother A exciter (flute family)
    ReedExciter          reed;          // Brother B exciter (reed family)

    void prepare (double sampleRate)
    {
        sr = static_cast<float> (sampleRate);
        int maxDelaySamples = static_cast<int> (sr / 20.0f) + 8; // 20 Hz floor
        delayLine.prepare    (maxDelaySamples);
        dampFilter.prepare   ();
        bodyResonator.prepare (sampleRate);
        sympBank.prepare     (sampleRate, 512);
        organicDrift.prepare  (sampleRate);
        airJet.prepare        (sampleRate);
        reed.prepare          (sampleRate);
        // Prepare ADSR with default parameters (overridden per-noteOn with velocity scaling)
        adsr.prepare (sr);
        adsr.setADSR (0.010f, 0.100f, 0.7f, 0.400f);
        adsr.setShape (StandardADSR::Shape::ADSR);
    }

    void reset()
    {
        delayLine.reset();
        dampFilter.reset();
        bodyResonator.reset();
        sympBank.reset();
        organicDrift.reset();
        airJet.reset();
        reed.reset();
        adsr.reset();
        active          = false;
        stealFadeGain   = 1.0f;
        stealFadeStep   = 0.0f;
        isBeingStolen   = false;
    }

    void noteOn (int midiNote, float velocity)
    {
        note      = midiNote;
        vel       = velocity;
        freq      = 440.0f * std::pow (2.0f, (midiNote - 69.0f) / 12.0f);
        delayLine.reset();
        dampFilter.reset();
        bodyResonator.setParams (freq * 1.3f, 3.5f); // initial body mode — overridden per sample
        sympBank.tune (freq);
        // Velocity-scaled ADSR: softer notes sustain at a lower level for natural dynamics.
        // Attack 10ms, Decay 100ms, Sustain scales with velocity (0.3..0.7), Release 400ms.
        adsr.setADSR (0.010f, 0.100f, 0.3f + velocity * 0.4f, 0.400f);
        adsr.setShape (StandardADSR::Shape::ADSR);
        adsr.noteOn();
        active    = true;
    }

    void noteOff()
    {
        adsr.noteOff();
    }
};

//==============================================================================
// ObbligatoEngine
//==============================================================================
class ObbligatoEngine : public SynthEngine
{
public:

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;

        // Prepare all voices
        for (auto& voice : voices)
            voice.prepare (sampleRate);

        prepareSilenceGate (sampleRate, maxBlockSize);

        // FX LFOs
        chorusLFO.setRate  (0.7f, static_cast<float> (sampleRate));
        chorusLFO.setShape (StandardLFO::Sine);
        phaserLFO.setRate  (0.3f, static_cast<float> (sampleRate));
        phaserLFO.setShape (StandardLFO::Sine);

        // FX delay buffers — scale with sample rate so 192kHz gets the same
        // wall-clock delay lengths as 44100Hz (D003 / buffer-overrun guard).
        float srScale  = static_cast<float> (sampleRate) / 44100.0f;
        int brightLen  = static_cast<int> (661.0f  * srScale) + 1; // ~15ms bright delay
        int darkLen    = static_cast<int> (1543.0f * srScale) + 1; // ~35ms dark delay
        int springLen  = static_cast<int> (307.0f  * srScale) + 1; // ~7ms spring reverb

        brightDelayBufL.assign (brightLen, 0.0f);
        brightDelayBufR.assign (brightLen, 0.0f);
        darkDelayBufL.assign   (darkLen,   0.0f);
        darkDelayBufR.assign   (darkLen,   0.0f);
        springBufL.assign      (springLen, 0.0f);
        springBufR.assign      (springLen, 0.0f);

        brightDelayPos = 0;
        darkDelayPos   = 0;
        springPos      = 0;
    }

    void releaseResources() override
    {
        for (auto& voice : voices)
            voice.reset();
    }

    void reset() override
    {
        for (auto& voice : voices)
            voice.reset();

        lastL = lastR = 0.0f;
        chorusLFO.reset();
        phaserLFO.reset();
    }

    //==========================================================================
    // renderBlock — MIDI handling + per-sample synthesis + FX
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;

        // -----------------------------------------------------------------------
        // MIDI event processing
        // -----------------------------------------------------------------------
        for (const auto midiEvent : midi)
        {
            auto msg = midiEvent.getMessage();

            if (msg.isNoteOn())
            {
                wakeSilenceGate();

                // Find a free voice slot; fall back to round-robin steal
                int targetSlot = -1;
                for (int i = 0; i < kNumVoices; ++i)
                {
                    if (!voices[i].active)
                    {
                        targetSlot = i;
                        break;
                    }
                }
                if (targetSlot < 0)
                    targetSlot = rrVoiceIndex % kNumVoices;

                rrVoiceIndex = (targetSlot + 1) % kNumVoices;

                // --- Voice routing: determine which brother this voice plays ---
                // 0=Alternate, 1=Split(C4), 2=Layer, 3=RoundRobin, 4=Velocity
                int routingMode = static_cast<int> (pVoiceRouting ? pVoiceRouting->load() : 0);
                bool assignBroA = true;

                switch (routingMode)
                {
                    case 0: assignBroA = (targetSlot % 2 == 0);              break; // Alternate
                    case 1: assignBroA = (msg.getNoteNumber() < 60);         break; // Split at C4
                    case 2: assignBroA = true;                                break; // Layer (second voice below)
                    case 3: assignBroA = (rrCounter++ % 2 == 0);             break; // Round Robin
                    case 4: assignBroA = (msg.getVelocity() < 80);           break; // Velocity: soft→A, hard→B
                    default: break;
                }

                voices[targetSlot].isBroA = assignBroA;

                // Trigger with crossfade steal if the slot is currently active
                if (voices[targetSlot].active && !voices[targetSlot].isBeingStolen)
                {
                    voices[targetSlot].isBeingStolen = true;
                    voices[targetSlot].stealFadeStep = 1.0f / (0.005f * voices[targetSlot].sr);
                    voices[targetSlot].stealFadeGain = 1.0f;
                    voices[targetSlot].pendingNote   = msg.getNoteNumber();
                    voices[targetSlot].pendingVel    = msg.getVelocity() / 127.0f;
                }
                else if (!voices[targetSlot].isBeingStolen)
                {
                    voices[targetSlot].noteOn (msg.getNoteNumber(), msg.getVelocity() / 127.0f);
                }

                // Layer mode: allocate a second voice for Brother B in parallel
                if (routingMode == 2)
                {
                    int layerSlot = -1;
                    for (int i = 0; i < kNumVoices; ++i)
                    {
                        if (!voices[i].active && i != targetSlot)
                        {
                            layerSlot = i;
                            break;
                        }
                    }

                    if (layerSlot >= 0)
                    {
                        voices[layerSlot].isBroA = false; // Layer mode always uses Brother B

                        if (voices[layerSlot].active && !voices[layerSlot].isBeingStolen)
                        {
                            voices[layerSlot].isBeingStolen = true;
                            voices[layerSlot].stealFadeStep = 1.0f / (0.005f * voices[layerSlot].sr);
                            voices[layerSlot].stealFadeGain = 1.0f;
                            voices[layerSlot].pendingNote   = msg.getNoteNumber();
                            voices[layerSlot].pendingVel    = msg.getVelocity() / 127.0f;
                        }
                        else if (!voices[layerSlot].isBeingStolen)
                        {
                            voices[layerSlot].noteOn (msg.getNoteNumber(), msg.getVelocity() / 127.0f);
                        }
                    }
                }
            }
            else if (msg.isNoteOff())
            {
                for (auto& voice : voices)
                    if (voice.active && voice.note == msg.getNoteNumber())
                        voice.noteOff();
            }
            else if (msg.isChannelPressure())
            {
                // Aftertouch raises effective breath/intensity (D006)
                float atPressure = static_cast<float> (msg.getChannelPressureValue()) / 127.0f;
                for (auto& voice : voices)
                    if (voice.active)
                        voice.vel = juce::jmax (voice.vel, atPressure);
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                // Mod wheel raises breath/intensity with 70% ceiling (D006)
                float modWheel = static_cast<float> (msg.getControllerValue()) / 127.0f;
                for (auto& voice : voices)
                    if (voice.active)
                        voice.vel = juce::jmax (voice.vel, modWheel * 0.7f);
            }
            else if (msg.isPitchWheel())
            {
                pitchBendNorm = PitchBendUtil::parsePitchWheel (msg.getPitchWheelValue());
            }
        }

        // SilenceGate: skip all DSP when the engine has been silent long enough
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            buf.clear();
            return;
        }

        // Reset coupling accumulators each block — stale values from disconnected
        // routes must not persist into the next block.
        extPitchMod = 0.0f;
        extDampMod  = 0.0f;
        extIntens   = 1.0f;

        // -----------------------------------------------------------------------
        // Parameter snapshot — read all atomics once per block (ParamSnapshot pattern)
        // -----------------------------------------------------------------------

        // Section A: Brother A — Flute Family
        float breathA    = pBrA    ? pBrA->load()    : 0.7f;
        float embouchureA = pEmA   ? pEmA->load()    : 0.5f;
        float airFlutterA = pFlA   ? pFlA->load()    : 0.2f;
        float instrumentA = pInA   ? pInA->load()    : 0.0f;
        float bodySizeA   = pBoA   ? pBoA->load()    : 0.5f;

        // Section B: Brother B — Reed Family
        float breathB    = pBrB    ? pBrB->load()    : 0.7f;
        float reedStiffness = pSt  ? pSt->load()     : 0.5f;
        float reedBite   = pBi     ? pBi->load()     : 0.3f;
        float instrumentB = pInB   ? pInB->load()    : 0.0f;
        float bodySizeB   = pBoB   ? pBoB->load()    : 0.5f;

        // Section C: Shared voice controls
        float damping        = pDamp   ? pDamp->load()   : 0.995f;
        float sympatheticAmt = pSymp   ? pSymp->load()   : 0.3f;
        float driftRate      = pDriftR ? pDriftR->load() : 0.1f;
        float driftDepth     = pDriftD ? pDriftD->load() : 3.0f;

        // Section D: BOND interaction
        float bondStage     = pBondStg  ? pBondStg->load()  : 0.0f;
        float bondIntensity = pBondInt  ? pBondInt->load()  : 0.5f;
        float bondRate      = pBondRt   ? pBondRt->load()   : 0.2f;

        // Section E: FX Chain A "The Air"
        float fxChorus      = pFxACh  ? pFxACh->load()  : 0.3f;
        float fxBrightDelay = pFxABD  ? pFxABD->load()  : 0.2f;
        float fxPlate       = pFxAPl  ? pFxAPl->load()  : 0.2f;
        float fxAirExciter  = pFxAEx  ? pFxAEx->load()  : 0.1f;

        // Section E: FX Chain B "The Water"
        float fxPhaser    = pFxBPh  ? pFxBPh->load()  : 0.2f;
        float fxDarkDelay = pFxBDD  ? pFxBDD->load()  : 0.3f;
        float fxSpring    = pFxBSp  ? pFxBSp->load()  : 0.2f;
        float fxTapeSat   = pFxBTS  ? pFxBTS->load()  : 0.1f;

        // Section F: Macros
        float macroBreath   = pMBreath ? pMBreath->load() : 0.5f;
        float macroBond     = pMBond_  ? pMBond_->load()  : 0.0f;
        float macroMischief = pMMisc_  ? pMMisc_->load()  : 0.0f;
        float macroWind     = pMWind_  ? pMWind_->load()  : 0.3f;

        // -----------------------------------------------------------------------
        // BOND system — 8 emotional stages modulate breath, detune, sympathetic, pan
        //
        // bondStage 0..1 maps to: Harmony, Play, Dare, Fight, Cry, Forgive, Protect,
        // Transcend. bondIntensity scales the effect; bondRate controls smoothing.
        //
        // Per-stage values: [breathMod, detuneMod, sympatheticMod, panSpread]
        //   Harmony(0): gentle unison, minimal spread
        //   Fight(3):   maximum detune, wide spread
        //   Cry(4):     maximum sympathetic resonance bloom
        //   Protect(6): negative breathMod (one brother quiets for the other)
        //   Transcend(7): back to unison, minimal spread
        // -----------------------------------------------------------------------
        float bondTarget = bondStage * 8.0f; // 0..8 stage range
        bondSmoothed += (bondTarget - bondSmoothed) * std::min (bondRate, 1.0f);
        bondSmoothed = flushDenormal(bondSmoothed);

        int   bondIdx  = std::min (static_cast<int> (bondSmoothed), 7);
        float bondFrac = bondSmoothed - static_cast<float> (bondIdx);
        int   bondNext = std::min (bondIdx + 1, 7);

        // [breathMod, detuneMod, sympatheticMod, panSpread]
        static constexpr float kBondTable[8][4] = {
            { 0.0f,  0.00f, 0.2f, 0.10f },  // 0: Harmony
            { 0.1f,  0.02f, 0.3f, 0.20f },  // 1: Play
            { 0.2f,  0.05f, 0.2f, 0.30f },  // 2: Dare
            { 0.3f,  0.10f, 0.1f, 0.50f },  // 3: Fight
            { 0.1f,  0.03f, 0.6f, 0.20f },  // 4: Cry
            { 0.0f,  0.01f, 0.4f, 0.15f },  // 5: Forgive
            {-0.1f,  0.00f, 0.5f, 0.10f },  // 6: Protect
            { 0.0f,  0.00f, 0.3f, 0.05f },  // 7: Transcend
        };

        // Interpolate between adjacent stages
        float bondBreathMod = kBondTable[bondIdx][0] * (1.0f - bondFrac) + kBondTable[bondNext][0] * bondFrac;
        float bondDetuneMod = kBondTable[bondIdx][1] * (1.0f - bondFrac) + kBondTable[bondNext][1] * bondFrac;
        float bondSympMod   = kBondTable[bondIdx][2] * (1.0f - bondFrac) + kBondTable[bondNext][2] * bondFrac;
        float bondPanSpread = kBondTable[bondIdx][3] * (1.0f - bondFrac) + kBondTable[bondNext][3] * bondFrac;

        // Scale all BOND mods by bondIntensity * macroBond (0.5 = neutral)
        float bondScale = bondIntensity * (macroBond * 2.0f);
        bondBreathMod *= bondScale;
        bondDetuneMod *= bondScale;
        bondSympMod   *= bondScale;
        bondPanSpread *= bondScale;

        // MISCHIEF macro: cross-brother detuning chaos (up to ~8 cents)
        float mischiefDetune = macroMischief * 0.08f;

        // Effective breath per brother — macroBreath scales both, BOND offsets
        float effBreathA = std::clamp (breathA * macroBreath + bondBreathMod, 0.0f, 1.0f);
        float effBreathB = std::clamp (breathB * macroBreath + bondBreathMod, 0.0f, 1.0f);

        // Effective sympathetic amount — BOND stage can bloom the sympBank
        float effSymp = std::clamp (sympatheticAmt + bondSympMod, 0.0f, 1.0f);

        // -----------------------------------------------------------------------
        // Instrument-body parameters per selection
        //
        // Brother A selections: 0=Flute, 1=Piccolo, 2=Pan Flute, 3=Shakuhachi,
        //                       4=Bansuri, 5=Ney, 6=Recorder, 7=Ocarina
        // Brother B selections: 0=Clarinet, 1=Oboe, 2=Bassoon, 3=Soprano Sax,
        //                       4=Duduk, 5=Zurna, 6=Shawm, 7=Musette
        //
        // Each instrument tweaks the FamilyBodyResonance frequency ratio and Q
        // to approximate the characteristic overtone bloom of that instrument family.
        // -----------------------------------------------------------------------
        static constexpr float kBodyFreqRatioA[8] = { 1.3f, 2.0f, 0.8f, 1.0f, 0.9f, 1.1f, 1.5f, 0.6f };
        static constexpr float kBodyQA[8]          = { 3.5f, 4.0f, 2.5f, 5.0f, 3.0f, 4.5f, 3.0f, 6.0f };

        static constexpr float kBodyFreqRatioB[8] = { 1.2f, 1.8f, 0.6f, 1.4f, 0.7f, 2.0f, 1.6f, 1.0f };
        static constexpr float kBodyQB[8]          = { 4.0f, 5.0f, 3.0f, 3.5f, 6.0f, 4.0f, 3.5f, 5.5f };

        int   instAIdx       = std::clamp (static_cast<int> (instrumentA), 0, 7);
        float bodyFreqRatioA = kBodyFreqRatioA[instAIdx] * (0.5f + bodySizeA);
        float bodyQValA      = kBodyQA[instAIdx];

        int   instBIdx       = std::clamp (static_cast<int> (instrumentB), 0, 7);
        float bodyFreqRatioB = kBodyFreqRatioB[instBIdx] * (0.5f + bodySizeB);
        float bodyQValB      = kBodyQB[instBIdx];

        // -----------------------------------------------------------------------
        // Per-sample synthesis loop
        // -----------------------------------------------------------------------
        auto* outL = buf.getWritePointer (0);
        auto* outR = buf.getWritePointer (1);

        for (int sampleIdx = 0; sampleIdx < numSamples; ++sampleIdx)
        {
            float sumL = 0.0f;
            float sumR = 0.0f;

            // --- Voice loop ---
            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                // Voice-steal crossfade: fade outgoing voice over 5ms, then
                // restart with the pending note.
                if (voice.isBeingStolen)
                {
                    voice.stealFadeGain -= voice.stealFadeStep;

                    if (voice.stealFadeGain <= 0.0f)
                    {
                        voice.isBeingStolen = false;
                        voice.stealFadeGain = 1.0f;
                        voice.stealFadeStep = 0.0f;
                        voice.noteOn (voice.pendingNote, voice.pendingVel);
                        continue; // start rendering at full gain next sample
                    }
                }

                // --- Amplitude envelope: StandardADSR (10ms A, 100ms D, vel-scaled S, 400ms R) ---
                float envLevel = voice.adsr.process();

                if (!voice.adsr.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // --- Pitch: drift + BOND detune + MISCHIEF chaos + pitch bend ---
                // MISCHIEF pushes A sharp, B flat (opposing detune for tension)
                float mischiefOffset = voice.isBroA ? mischiefDetune : -mischiefDetune;
                float driftSemitones = voice.organicDrift.tick (driftRate, driftDepth)
                                       + bondDetuneMod + mischiefOffset;

                float pitchedFreq = voice.freq
                    * fastPow2 ((driftSemitones + extPitchMod) / 12.0f)
                    * PitchBendUtil::semitonesToFreqRatio (pitchBendNorm * 2.0f);

                float delayLengthSamples = voice.sr / std::max (pitchedFreq, 20.0f);

                // --- Waveguide read: output from delay line before this sample ---
                float waveguideOut = voice.delayLine.read (delayLengthSamples);

                // --- Exciter: Brother A (AirJet) or Brother B (Reed) ---
                // Velocity maps 0→1 to 0.5→1.0 intensity (D001: velocity shapes timbre)
                float velIntensity = 0.5f + voice.vel * 0.5f;
                float coupledIntensity = extIntens * velIntensity;

                float exciterOut;

                if (voice.isBroA)
                {
                    // Air flutter: organic LFO modulates embouchure opening for flute vibrato.
                    // The second organicDrift call uses a higher rate to act as flutter LFO.
                    // fastSin replaces std::sin here — ~0.02% error, no measurable sonic change.
                    float flutterMod = airFlutterA * 0.15f
                                       * fastSin (voice.organicDrift.tick (5.0f, 1.0f) * 20.0f);
                    float effectiveBreathA = std::clamp (
                        effBreathA * velIntensity + flutterMod * embouchureA, 0.0f, 1.0f);
                    exciterOut = voice.airJet.tick (effectiveBreathA, voice.freq) * coupledIntensity;
                }
                else
                {
                    // Reed bite adds harmonic edge on top of stiffness (brighter overtones)
                    float effectiveStiffness = std::clamp (reedStiffness + reedBite * 0.3f, 0.0f, 1.0f);
                    exciterOut = voice.reed.tick (effBreathB * velIntensity, effectiveStiffness) * coupledIntensity;
                }

                // --- Waveguide feedback: exciter + delay output → damping filter → write ---
                // The 0.3f mix weight on the exciter keeps energy injection stable.
                float dampedSample = voice.dampFilter.process (
                    waveguideOut + exciterOut * 0.3f,
                    std::clamp (damping + extDampMod, 0.0f, 1.0f));
                voice.delayLine.write (dampedSample);

                // --- Body resonance: adds instrument-body coloration per instrument type ---
                float bodyFreqRatio = voice.isBroA ? bodyFreqRatioA : bodyFreqRatioB;
                float bodyQVal      = voice.isBroA ? bodyQValA      : bodyQValB;
                voice.bodyResonator.setParams (voice.freq * bodyFreqRatio, bodyQVal);
                float bodyOut = waveguideOut + voice.bodyResonator.process (waveguideOut) * 0.2f;

                // --- Sympathetic resonance: shimmer from 8-comb bank ---
                float sympOut = voice.sympBank.process (bodyOut, effSymp);

                // --- D001: velocity shapes brightness, not just amplitude.
                // Higher velocity increases sympathetic resonance (richer overtones).
                // Seance finding: "Constellation-wide pattern: intensity not brightness". ---
                float velBrightScale = 0.7f + voice.vel * 0.6f; // 0.7→1.3x at full velocity
                float voiceSignal = (bodyOut + sympOut * velBrightScale)
                                    * envLevel * voice.vel * voice.stealFadeGain * 0.4f;

                // --- Stereo panning: A left-ish, B right-ish, modulated by BOND pan spread ---
                float basePan = voice.isBroA ? 0.35f : 0.65f;
                float pan     = std::clamp (basePan + (voice.isBroA ? -bondPanSpread : bondPanSpread),
                                            0.05f, 0.95f);
                sumL += voiceSignal * (1.0f - pan);
                sumR += voiceSignal * pan;
            }

            // -----------------------------------------------------------------------
            // FX Chain A "The Air" — on summed signal
            // Chorus → Bright Delay → Plate Reverb → Air Exciter
            // -----------------------------------------------------------------------

            // Chorus: subtle pitch modulation via slow LFO (0.7 Hz)
            float chorusMod = chorusLFO.process() * fxChorus * 0.003f;
            float airL = sumL * (1.0f + chorusMod);
            float airR = sumR * (1.0f - chorusMod);

            // Bright delay: ~15ms feedback delay with bright character
            {
                int rdIdx = brightDelayBufL.empty() ? 0 : brightDelayPos % static_cast<int> (brightDelayBufL.size());
                float delayedL = brightDelayBufL[rdIdx];
                float delayedR = brightDelayBufR[rdIdx];
                brightDelayBufL[rdIdx] = flushDenormal (airL + delayedL * 0.4f * fxBrightDelay);
                brightDelayBufR[rdIdx] = flushDenormal (airR + delayedR * 0.4f * fxBrightDelay);
                brightDelayPos++;
                airL += delayedL * fxBrightDelay;
                airR += delayedR * fxBrightDelay;
            }

            // Plate reverb: simple one-pole allpass diffusion for air bloom
            {
                float plateIn = (airL + airR) * 0.5f * fxPlate;
                plateState = flushDenormal (plateState * 0.85f + plateIn * 0.15f);
                airL += plateState * fxPlate * 0.5f;
                airR += plateState * fxPlate * 0.5f;
            }

            // Air exciter: subtle HF harmonic enhancement via HP shelving
            {
                float monoIn = (airL + airR) * 0.5f;
                float hfComponent = monoIn - exciterLP;
                exciterLP = flushDenormal (exciterLP * 0.92f + monoIn * 0.08f);
                airL += hfComponent * fxAirExciter * 0.3f;
                airR += hfComponent * fxAirExciter * 0.3f;
            }

            // -----------------------------------------------------------------------
            // FX Chain B "The Water" — continues from airL/airR
            // Phaser → Dark Delay → Spring Reverb → Tape Saturation
            // -----------------------------------------------------------------------

            // Phaser: one-pole notch sweep driven by slow LFO (0.3 Hz)
            float phaserMod = phaserLFO.process();
            float notchCoeff = 0.1f + phaserMod * 0.05f;
            phaserStateL = flushDenormal (phaserStateL + (airL - phaserStateL) * notchCoeff);
            phaserStateR = flushDenormal (phaserStateR + (airR - phaserStateR) * notchCoeff);
            float wetL = airL - phaserStateL * fxPhaser;
            float wetR = airR - phaserStateR * fxPhaser;

            // Dark delay: ~35ms LP-filtered feedback delay for warm depth
            {
                int rdIdx = darkDelayBufL.empty() ? 0 : darkDelayPos % static_cast<int> (darkDelayBufL.size());
                float delayedL = darkDelayBufL[rdIdx];
                float delayedR = darkDelayBufR[rdIdx];
                // LP filter in the feedback path gives the "dark" character
                darkDelayLP_L = flushDenormal (darkDelayLP_L * 0.7f + (wetL + delayedL * 0.45f * fxDarkDelay) * 0.3f);
                darkDelayLP_R = flushDenormal (darkDelayLP_R * 0.7f + (wetR + delayedR * 0.45f * fxDarkDelay) * 0.3f);
                darkDelayBufL[rdIdx] = darkDelayLP_L;
                darkDelayBufR[rdIdx] = darkDelayLP_R;
                darkDelayPos++;
                wetL += delayedL * fxDarkDelay;
                wetR += delayedR * fxDarkDelay;
            }

            // Spring reverb: ~7ms comb filter for metallic resonance
            {
                int rdIdx = springBufL.empty() ? 0 : springPos % static_cast<int> (springBufL.size());
                float spL = springBufL[rdIdx];
                float spR = springBufR[rdIdx];
                springBufL[rdIdx] = flushDenormal (wetL * 0.3f + spL * 0.6f);
                springBufR[rdIdx] = flushDenormal (wetR * 0.3f + spR * 0.6f);
                springPos++;
                wetL += spL * fxSpring * 0.4f;
                wetR += spR * fxSpring * 0.4f;
            }

            // Tape saturation: soft tanh clip for warmth (scaled to avoid loudness increase)
            if (fxTapeSat > 0.001f)
            {
                float drive = 1.0f + fxTapeSat * 3.0f;
                wetL = fastTanh (wetL * drive) / drive;
                wetR = fastTanh (wetR * drive) / drive;
            }

            // -----------------------------------------------------------------------
            // WIND macro: blends in atmospheric breath-noise floor
            // Only active above threshold to avoid wasting CPU at zero
            // -----------------------------------------------------------------------
            if (macroWind > 0.01f)
            {
                windSeed = windSeed * 1664525u + 1013904223u;
                float windNoise = static_cast<float> (static_cast<int32_t> (windSeed)) * 4.656612e-10f;
                windLP = flushDenormal (windLP * 0.95f + windNoise * 0.05f);
                wetL += windLP * macroWind * 0.08f;
                wetR += windLP * macroWind * 0.08f;
            }

            // --- Output accumulate ---
            outL[sampleIdx] += wetL;
            outR[sampleIdx] += wetR;
            lastL = wetL;
            lastR = wetR;
        }

        // Update active voice count for display
        int activeCount = 0;
        for (const auto& voice : voices)
            if (voice.active)
                ++activeCount;
        activeVoiceCount_.store (activeCount, std::memory_order_relaxed);

        // SilenceGate: analyze output level for next-block bypass decision
        analyzeForSilenceGate (buf, numSamples);
    }

    //==========================================================================
    // Coupling I/O
    //==========================================================================

    float getSampleForCoupling (int channel, int /*sampleIndex*/) const override
    {
        return channel == 0 ? lastL : lastR;
    }

    void applyCouplingInput (CouplingType couplingType, float amount,
                             const float* couplingBuf, int /*numSamples*/) override
    {
        switch (couplingType)
        {
            // LFOToPitch: semitone pitch offset from external LFO
            case CouplingType::LFOToPitch:
                extPitchMod = (couplingBuf ? couplingBuf[0] : 0.0f) * amount * 2.0f;
                break;

            // AmpToFilter: coupling amplitude increases waveguide damping (darker tone)
            case CouplingType::AmpToFilter:
                extDampMod = amount * 0.08f;
                break;

            // EnvToMorph: coupling envelope multiplies exciter intensity
            case CouplingType::EnvToMorph:
                extIntens = 1.0f + amount * 0.5f;
                break;

            default:
                break;
        }
    }

    //==========================================================================
    // Parameter registration (30 total — all parameters wired to DSP: D004)
    //==========================================================================

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using Float  = juce::AudioParameterFloat;
        using Choice = juce::AudioParameterChoice;
        using Range  = juce::NormalisableRange<float>;

        // Section A: Brother A — Flute Family
        params.push_back (std::make_unique<Float>  ("obbl_breathA",     "Breath A",      Range { 0.0f, 1.0f },        0.7f));
        params.push_back (std::make_unique<Float>  ("obbl_embouchureA", "Embouchure A",  Range { 0.0f, 1.0f },        0.5f));
        params.push_back (std::make_unique<Float>  ("obbl_airFlutterA", "Air Flutter A", Range { 0.0f, 1.0f },        0.2f));
        params.push_back (std::make_unique<Choice> ("obbl_instrumentA", "Instrument A",
            juce::StringArray { "Flute", "Piccolo", "Pan Flute", "Shakuhachi",
                                "Bansuri", "Ney", "Recorder", "Ocarina" }, 0));
        params.push_back (std::make_unique<Float>  ("obbl_bodySizeA",   "Body A",        Range { 0.0f, 1.0f },        0.5f));

        // Section B: Brother B — Reed Family
        params.push_back (std::make_unique<Float>  ("obbl_breathB",      "Breath B",       Range { 0.0f, 1.0f },       0.7f));
        params.push_back (std::make_unique<Float>  ("obbl_reedStiffness","Reed Stiffness", Range { 0.0f, 1.0f },       0.5f));
        params.push_back (std::make_unique<Float>  ("obbl_reedBite",     "Reed Bite",      Range { 0.0f, 1.0f },       0.3f));
        params.push_back (std::make_unique<Choice> ("obbl_instrumentB",  "Instrument B",
            juce::StringArray { "Clarinet", "Oboe", "Bassoon", "Soprano Sax",
                                "Duduk", "Zurna", "Shawm", "Musette" }, 0));
        params.push_back (std::make_unique<Float>  ("obbl_bodySizeB",    "Body B",         Range { 0.0f, 1.0f },       0.5f));

        // Section C: Shared voice controls
        params.push_back (std::make_unique<Choice> ("obbl_voiceRouting", "Voice Routing",
            juce::StringArray { "Alternate", "Split", "Layer", "Round Robin", "Velocity" }, 0));
        params.push_back (std::make_unique<Float>  ("obbl_damping",        "Damping",    Range { 0.8f,   0.999f },     0.995f));
        params.push_back (std::make_unique<Float>  ("obbl_sympatheticAmt", "Sympathetic",Range { 0.0f,   1.0f },       0.3f));
        params.push_back (std::make_unique<Float>  ("obbl_driftRate",      "Drift Rate", Range { 0.005f, 0.5f },       0.1f));
        params.push_back (std::make_unique<Float>  ("obbl_driftDepth",     "Drift Depth",Range { 0.0f,   20.0f },      3.0f));

        // Section D: BOND interaction
        params.push_back (std::make_unique<Float>  ("obbl_bondStage",    "Bond Stage",    Range { 0.0f,   1.0f },      0.0f));
        params.push_back (std::make_unique<Float>  ("obbl_bondIntensity","Bond Intensity",Range { 0.0f,   1.0f },      0.5f));
        params.push_back (std::make_unique<Float>  ("obbl_bondRate",     "Bond Rate",     Range { 0.01f,  2.0f },      0.2f));

        // Section E: FX Chain A "The Air"
        params.push_back (std::make_unique<Float>  ("obbl_fxAChorus",      "Air Chorus",   Range { 0.0f, 1.0f },       0.3f));
        params.push_back (std::make_unique<Float>  ("obbl_fxABrightDelay", "Bright Delay", Range { 0.0f, 1.0f },       0.2f));
        params.push_back (std::make_unique<Float>  ("obbl_fxAPlate",       "Air Plate",    Range { 0.0f, 1.0f },       0.2f));
        params.push_back (std::make_unique<Float>  ("obbl_fxAExciter",     "Air Exciter",  Range { 0.0f, 1.0f },       0.1f));

        // Section E: FX Chain B "The Water"
        params.push_back (std::make_unique<Float>  ("obbl_fxBPhaser",    "Water Phaser",  Range { 0.0f, 1.0f },        0.2f));
        params.push_back (std::make_unique<Float>  ("obbl_fxBDarkDelay", "Dark Delay",    Range { 0.0f, 1.0f },        0.3f));
        params.push_back (std::make_unique<Float>  ("obbl_fxBSpring",    "Water Spring",  Range { 0.0f, 1.0f },        0.2f));
        params.push_back (std::make_unique<Float>  ("obbl_fxBTapeSat",   "Water Tape",    Range { 0.0f, 1.0f },        0.1f));

        // Section F: Macros (M1–M4)
        params.push_back (std::make_unique<Float>  ("obbl_macroBreath",   "BREATH",   Range { 0.0f, 1.0f }, 0.5f));
        params.push_back (std::make_unique<Float>  ("obbl_macroBond",     "BOND",     Range { 0.0f, 1.0f }, 0.0f));
        params.push_back (std::make_unique<Float>  ("obbl_macroMischief", "MISCHIEF", Range { 0.0f, 1.0f }, 0.0f));
        params.push_back (std::make_unique<Float>  ("obbl_macroWind",     "WIND",     Range { 0.0f, 1.0f }, 0.3f));
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParameters (params);
        return { params.begin(), params.end() };
    }

    //==========================================================================
    // Parameter attachment — cache raw pointers for lock-free block-rate reads
    //==========================================================================

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        // Section A: Brother A
        pBrA = apvts.getRawParameterValue ("obbl_breathA");
        pEmA = apvts.getRawParameterValue ("obbl_embouchureA");
        pFlA = apvts.getRawParameterValue ("obbl_airFlutterA");
        pInA = apvts.getRawParameterValue ("obbl_instrumentA");
        pBoA = apvts.getRawParameterValue ("obbl_bodySizeA");

        // Section B: Brother B
        pBrB = apvts.getRawParameterValue ("obbl_breathB");
        pSt  = apvts.getRawParameterValue ("obbl_reedStiffness");
        pBi  = apvts.getRawParameterValue ("obbl_reedBite");
        pInB = apvts.getRawParameterValue ("obbl_instrumentB");
        pBoB = apvts.getRawParameterValue ("obbl_bodySizeB");

        // Section C: Shared voice
        pVoiceRouting = apvts.getRawParameterValue ("obbl_voiceRouting");
        pDamp         = apvts.getRawParameterValue ("obbl_damping");
        pSymp         = apvts.getRawParameterValue ("obbl_sympatheticAmt");
        pDriftR       = apvts.getRawParameterValue ("obbl_driftRate");
        pDriftD       = apvts.getRawParameterValue ("obbl_driftDepth");

        // Section D: BOND
        pBondStg = apvts.getRawParameterValue ("obbl_bondStage");
        pBondInt = apvts.getRawParameterValue ("obbl_bondIntensity");
        pBondRt  = apvts.getRawParameterValue ("obbl_bondRate");

        // Section E: FX Chain A "The Air"
        pFxACh = apvts.getRawParameterValue ("obbl_fxAChorus");
        pFxABD = apvts.getRawParameterValue ("obbl_fxABrightDelay");
        pFxAPl = apvts.getRawParameterValue ("obbl_fxAPlate");
        pFxAEx = apvts.getRawParameterValue ("obbl_fxAExciter");

        // Section E: FX Chain B "The Water"
        pFxBPh = apvts.getRawParameterValue ("obbl_fxBPhaser");
        pFxBDD = apvts.getRawParameterValue ("obbl_fxBDarkDelay");
        pFxBSp = apvts.getRawParameterValue ("obbl_fxBSpring");
        pFxBTS = apvts.getRawParameterValue ("obbl_fxBTapeSat");

        // Section F: Macros
        pMBreath = apvts.getRawParameterValue ("obbl_macroBreath");
        pMBond_  = apvts.getRawParameterValue ("obbl_macroBond");
        pMMisc_  = apvts.getRawParameterValue ("obbl_macroMischief");
        pMWind_  = apvts.getRawParameterValue ("obbl_macroWind");
    }

    //==========================================================================
    // Engine identity
    //==========================================================================

    juce::String  getEngineId()        const override { return "Obbligato"; }
    juce::Colour  getAccentColour()    const override { return juce::Colour (0xFFFF8A7A); } // Rascal Coral
    int           getMaxVoices()       const override { return kNumVoices; }
    int           getActiveVoiceCount() const override
    {
        return activeVoiceCount_.load (std::memory_order_relaxed);
    }

private:

    //==========================================================================
    // Engine state
    //==========================================================================

    static constexpr int kNumVoices = 12;   // maximum polyphony

    double sr    = 44100.0;
    int    rrVoiceIndex = 0;    // next victim for round-robin voice stealing
    int    rrCounter    = 0;    // separate counter for round-robin routing mode
    float  lastL = 0.0f;        // last rendered left sample (for coupling output)
    float  lastR = 0.0f;        // last rendered right sample

    std::array<ObbligatoAdapterVoice, kNumVoices> voices;

    // MIDI continuous controllers
    float pitchBendNorm = 0.0f; // pitch wheel [-1, +1]; ±2 semitone range applied in DSP

    //==========================================================================
    // Coupling modulation values (reset each block by renderBlock)
    //==========================================================================
    float extPitchMod = 0.0f;   // semitones from LFOToPitch coupling
    float extDampMod  = 0.0f;   // damping coefficient offset from AmpToFilter coupling
    float extIntens   = 1.0f;   // exciter intensity multiplier from EnvToMorph coupling

    //==========================================================================
    // BOND smoothing state
    //==========================================================================
    float bondSmoothed = 0.0f;  // smoothed bond stage value (IIR toward bondStage target)

    //==========================================================================
    // FX LFOs — shared across all voices (one oscillator per FX module)
    //==========================================================================
    StandardLFO chorusLFO;  // 0.7 Hz sine — drives chorus pitch modulation
    StandardLFO phaserLFO;  // 0.3 Hz sine — drives phaser notch sweep

    //==========================================================================
    // FX state: Bright delay (FX Chain A) — ~15ms at 44100Hz
    // Buffer sized dynamically in prepare() for sample-rate independence.
    //==========================================================================
    std::vector<float> brightDelayBufL;
    std::vector<float> brightDelayBufR;
    int brightDelayPos = 0;

    //==========================================================================
    // FX state: Plate reverb (FX Chain A) — one-pole allpass diffusion
    //==========================================================================
    float plateState = 0.0f;

    //==========================================================================
    // FX state: Air exciter (FX Chain A) — HP shelving for HF enhancement
    //==========================================================================
    float exciterLP = 0.0f;     // low-pass state for the HP shelf

    //==========================================================================
    // FX state: Phaser (FX Chain B) — one-pole notch sweep
    //==========================================================================
    float phaserStateL = 0.0f;
    float phaserStateR = 0.0f;

    //==========================================================================
    // FX state: Dark delay (FX Chain B) — ~35ms LP-filtered feedback delay
    // Buffer sized dynamically in prepare() for sample-rate independence.
    //==========================================================================
    std::vector<float> darkDelayBufL;
    std::vector<float> darkDelayBufR;
    int   darkDelayPos  = 0;
    float darkDelayLP_L = 0.0f; // per-channel LP filter state in feedback path
    float darkDelayLP_R = 0.0f;

    //==========================================================================
    // FX state: Spring reverb (FX Chain B) — ~7ms comb filter
    // Buffer sized dynamically in prepare() for sample-rate independence.
    //==========================================================================
    std::vector<float> springBufL;
    std::vector<float> springBufR;
    int springPos = 0;

    //==========================================================================
    // FX state: Wind noise (WIND macro)
    //==========================================================================
    uint32_t windSeed = 33333u; // LCG state for real-time-safe noise generation
    float    windLP   = 0.0f;   // one-pole LP for breath-noise floor smoothing

    //==========================================================================
    // Cached parameter pointers (30 total — one per declared parameter)
    // Populated by attachParameters(); null-checked at all call sites.
    //==========================================================================

    // Section A: Brother A
    std::atomic<float>* pBrA = nullptr;    // obbl_breathA
    std::atomic<float>* pEmA = nullptr;    // obbl_embouchureA
    std::atomic<float>* pFlA = nullptr;    // obbl_airFlutterA
    std::atomic<float>* pInA = nullptr;    // obbl_instrumentA  (Choice → float index)
    std::atomic<float>* pBoA = nullptr;    // obbl_bodySizeA

    // Section B: Brother B
    std::atomic<float>* pBrB = nullptr;    // obbl_breathB
    std::atomic<float>* pSt  = nullptr;    // obbl_reedStiffness
    std::atomic<float>* pBi  = nullptr;    // obbl_reedBite
    std::atomic<float>* pInB = nullptr;    // obbl_instrumentB  (Choice → float index)
    std::atomic<float>* pBoB = nullptr;    // obbl_bodySizeB

    // Section C: Shared voice
    std::atomic<float>* pVoiceRouting = nullptr;  // obbl_voiceRouting
    std::atomic<float>* pDamp         = nullptr;  // obbl_damping
    std::atomic<float>* pSymp         = nullptr;  // obbl_sympatheticAmt
    std::atomic<float>* pDriftR       = nullptr;  // obbl_driftRate
    std::atomic<float>* pDriftD       = nullptr;  // obbl_driftDepth

    // Section D: BOND
    std::atomic<float>* pBondStg = nullptr;  // obbl_bondStage
    std::atomic<float>* pBondInt = nullptr;  // obbl_bondIntensity
    std::atomic<float>* pBondRt  = nullptr;  // obbl_bondRate

    // Section E: FX Chain A "The Air"
    std::atomic<float>* pFxACh = nullptr;  // obbl_fxAChorus
    std::atomic<float>* pFxABD = nullptr;  // obbl_fxABrightDelay
    std::atomic<float>* pFxAPl = nullptr;  // obbl_fxAPlate
    std::atomic<float>* pFxAEx = nullptr;  // obbl_fxAExciter

    // Section E: FX Chain B "The Water"
    std::atomic<float>* pFxBPh = nullptr;  // obbl_fxBPhaser
    std::atomic<float>* pFxBDD = nullptr;  // obbl_fxBDarkDelay
    std::atomic<float>* pFxBSp = nullptr;  // obbl_fxBSpring
    std::atomic<float>* pFxBTS = nullptr;  // obbl_fxBTapeSat

    // Section F: Macros
    std::atomic<float>* pMBreath = nullptr;  // obbl_macroBreath
    std::atomic<float>* pMBond_  = nullptr;  // obbl_macroBond
    std::atomic<float>* pMMisc_  = nullptr;  // obbl_macroMischief
    std::atomic<float>* pMWind_  = nullptr;  // obbl_macroWind
};

} // namespace xoceanus
