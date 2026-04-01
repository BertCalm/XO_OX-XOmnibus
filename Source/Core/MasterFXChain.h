#pragma once
#include "../DSP/Effects/Saturator.h"
#include "../DSP/Effects/Corroder.h"
#include "../DSP/Effects/VibeKnob.h"
#include "../DSP/Effects/SpectralTilt.h"
#include "../DSP/Effects/TransientDesigner.h"
#include "../DSP/Effects/MasterDelay.h"
#include "../DSP/Effects/Combulator.h"
#include "../DSP/Effects/DopplerEffect.h"
#include "../DSP/Effects/LushReverb.h"
#include "../DSP/Effects/FrequencyShifter.h"
#include "../DSP/Effects/MasterModulation.h"
#include "../DSP/Effects/GranularSmear.h"
#include "../DSP/Effects/HarmonicExciter.h"
#include "../DSP/Effects/StereoSculptor.h"
#include "../DSP/Effects/PsychoacousticWidth.h"
#include "../DSP/Effects/MultibandCompressor.h"
#include "../DSP/Effects/Compressor.h"
#include "../DSP/Effects/fXOsmosis.h"
#include "../DSP/Effects/fXOneiric.h"
#include "../DSP/Effects/fXObscura.h"
#include "../DSP/Effects/fXOratory.h"
#include "../DSP/Effects/fXOnslaught.h"
#include "../DSP/Effects/fXFormant.h"
#include "../DSP/Effects/fXBreath.h"
#include "../DSP/Effects/ParametricEQ.h"
#include "../DSP/Effects/BrickwallLimiter.h"
#include "../DSP/Effects/DCBlocker.h"
#include "MasterFXSequencer.h"
#include "ShaperRegistry.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace xolokun {

//==============================================================================
// MasterFXChain — Post-mix master effects for XOlokun.
//
// Signal chain (24 stages, fixed order):
//   0.  Bus Shapers       — ShaperRegistry bus slots [0-1] process the mixed bus
//                           before all other master stages (Oxide/Observe by default).
//                           Shapers are loaded via loadBusShaper()/loadInsertShaper().
//   1.  Saturation        — Tape/Tube/Digital/FoldBack via Saturator (mode select)
//   2.  Corroder          — Digital erosion: bitcrush + SR reduce + FM distortion
//   3.  Vibe Knob         — Bipolar character: GRIT(+) ←→ SWEET(−) in one dial
//   4.  Spectral Tilt     — Cascaded shelving: shift spectral energy up/down
//   5.  Transient Designer— Attack/sustain shaping via envelope followers
//   6.  fXOsmosis         — Membrane transfer: dynamic filter bank breathes with input
//   7.  Multiband OTT     — 3-band upward+downward compression (pre-spatial)
//   8.  Stereo Delay      — Spatial echo with diffusion, autoclear, tape feedback
//   9.  Combulator        — Tuned comb bank with noise exciter (pitched resonance)
//   10. Doppler           — Distance-based filtering + pitch micro-shift + level
//   11. Space Reverb      — Algorithmic reverb (Schroeder-Moorer)
//   12. Freq Shifter      — Hilbert transform frequency shifter (metallic/alien)
//   13. fXOneiric         — Dream state: pitch-shifted feedback delay (spectral spirals)
//   14. Modulation FX     — Chorus/Flanger/Ensemble/Drift
//   15. Granular Smear    — Micro-granular buffer: transients → texture
//   16. Harmonic Exciter  — Parallel HF saturation for air & presence
//   17. Stereo Sculptor   — M/S frequency-dependent stereo field shaping
//   18. Psychoacoustic Width — Haas + complementary comb decorrelation
//   19. Bus Compressor    — Single-band output glue (parallel blend)
//   19.5 Parametric EQ   — 4-band surgical EQ (shelf + 2×peak + shelf)
//   20. Brickwall Limiter — True-peak safety limiter (1ms lookahead, ∞:1)
//   21. DC Blocker        — 10 Hz HPF removes any residual DC offset
//   22. Sequenced Mod     — Rhythmic parameter animation (non-audio)
//
// All stages bypass at zero CPU when their mix/depth/enable = 0.
// Sequencer modulates stages 1-19 parameters rhythmically (17 targets).
// Safety stages 20-21 are always active (no bypass).
//==============================================================================
class MasterFXChain
{
public:
    MasterFXChain() = default;

    //--------------------------------------------------------------------------
    // Shaper slot management — delegates to ShaperRegistry.
    // Call from the message thread (before or after prepare); the registry
    // stores unique_ptr<ShaperEngine> and calls prepare() internally.

    /// Load a shaper into a bus slot (0-1) by registry ID (e.g. "Oxide", "Observe").
    /// Pass an empty string to clear the slot. prepare() must have been called first
    /// so that sampleRate/blockSize are valid.
    void loadBusShaper (int slot, const std::string& shaperId)
    {
        ShaperRegistry::instance().loadBus (slot, shaperId, sr, blockSize);
    }

    /// Load a shaper into an insert slot (0-3) by registry ID.
    /// Insert shapers process a single engine's output before it reaches the mix.
    void loadInsertShaper (int slot, const std::string& shaperId)
    {
        ShaperRegistry::instance().loadInsert (slot, shaperId, sr, blockSize);
    }

    /// Returns the registered shaper IDs available for selection in the UI.
    std::vector<std::string> getAvailableShaperIds() const
    {
        return ShaperRegistry::instance().getRegisteredShaperIds();
    }

    //--------------------------------------------------------------------------
    void prepare (double sampleRate, int samplesPerBlock,
                  juce::AudioProcessorValueTreeState& apvts)
    {
        sr = sampleRate;
        blockSize = samplesPerBlock;

        // Stage 1: Saturator
        saturator.setMode (Saturator::SaturationMode::Tape);
        saturator.setOutputGain (0.85f);
        saturator.reset();

        // Stage 2: Corroder
        corroder.prepare (sampleRate);

        // Stage 3: Vibe Knob
        vibeKnob.prepare (sampleRate);

        // Stage 4: Spectral Tilt
        spectralTilt.prepare (sampleRate);

        // Stage 5: Transient Designer
        transientDesigner.prepare (sampleRate);

        // Stage 5.5: fXObscura (chiaroscuro — inverse-dynamic degradation)
        obscura.prepare (sampleRate);

        // Stage 5.6: fXFormant (Membrane Collection — formant filter)
        formantFX_.prepare (sampleRate, samplesPerBlock);

        // Stage 5.7: fXBreath (Membrane Collection — breath texture)
        breathFX_.prepare (sampleRate, samplesPerBlock);

        // Stage 6: fXOsmosis (membrane transfer)
        osmosis.prepare (sampleRate);

        // Stage 7: Multiband OTT (pre-spatial dynamics)
        multibandComp.prepare (sampleRate);

        // Stage 8: Delay
        delay.prepare (sampleRate, samplesPerBlock);

        // Stage 8.5: fXOratory (poetic meter delay)
        oratory.prepare (sampleRate, samplesPerBlock);

        // Stage 9: Combulator
        combulator.prepare (sampleRate);

        // Stage 10: Doppler
        doppler.prepare (sampleRate);

        // Stage 11: Reverb
        reverb.prepare (sampleRate);
        reverb.setWidth (1.0f);

        // Stage 12: Frequency Shifter
        freqShifter.prepare (sampleRate);

        // Stage 13: fXOneiric (dream state)
        oneiric.prepare (sampleRate, samplesPerBlock);

        // Stage 14: Modulation
        modulation.prepare (sampleRate, samplesPerBlock);

        // Stage 14.5: fXOnslaught (transient-reactive chorus → PM collapse)
        onslaught.prepare (sampleRate, samplesPerBlock);

        // Stage 15: Granular Smear
        granularSmear.prepare (sampleRate);

        // Stage 16: Harmonic Exciter
        harmonicExciter.prepare (sampleRate);

        // Stage 17: Stereo Sculptor
        stereoSculptor.prepare (sampleRate);

        // Stage 18: Psychoacoustic Width
        psychoWidth.prepare (sampleRate);

        // Stage 19: Bus Compressor
        compressor.prepare (sampleRate);
        compressor.setThreshold (-12.0f);
        compressor.setKnee (6.0f);
        compressor.setMakeupGain (0.0f);

        // Stage 19.5: Parametric EQ (post-compressor, pre-limiter)
        parametricEQ.prepare (sampleRate);

        // Stage 20: Brickwall Limiter
        limiter.prepare (sampleRate, samplesPerBlock);

        // Stage 21: DC Blocker
        dcBlocker.prepare (sampleRate);

        // Stage 22: Sequencer (non-audio)
        sequencer.prepare (sampleRate);

        // Dry buffer for compressor parallel blend
        dryBuffer.setSize (2, samplesPerBlock);

        // Stage 0: ShaperRegistry — prepare any already-loaded bus/insert shapers,
        // then load defaults if no shapers are currently registered.
        // Default: "Oxide" (saturation character) on bus slot 0,
        //          "Observe" (6-band EQ) on bus slot 1.
        // Both start bypassed so they are zero-CPU until the user engages them.
        auto& reg = ShaperRegistry::instance();
        reg.prepareAll (sampleRate, samplesPerBlock);

        const auto ids = reg.getRegisteredShaperIds();
        const bool hasOxide   = std::find (ids.begin(), ids.end(), "Oxide")   != ids.end();
        const bool hasObserve = std::find (ids.begin(), ids.end(), "Observe") != ids.end();

        if (reg.getBus (0) == nullptr && hasOxide)
        {
            reg.loadBus (0, "Oxide", sampleRate, samplesPerBlock);
            if (auto* s = reg.getBus (0)) s->setBypassed (true); // off by default
        }
        if (reg.getBus (1) == nullptr && hasObserve)
        {
            reg.loadBus (1, "Observe", sampleRate, samplesPerBlock);
            if (auto* s = reg.getBus (1)) s->setBypassed (true); // off by default
        }

        cacheParameterPointers (apvts);
    }

    //--------------------------------------------------------------------------
    void processBlock (juce::AudioBuffer<float>& buffer, int numSamples,
                       double ppqPosition = -1.0, double bpm = 0.0)
    {
        if (!pSatDrive || buffer.getNumChannels() < 2)
            return;

        float* L = buffer.getWritePointer (0);
        float* R = buffer.getWritePointer (1);

        // ====================================================================
        // ParamSnapshot: read ALL params once per block
        // ====================================================================

        // Stage 1: Saturation
        const float satDrive     = pSatDrive->load();
        const float satMode      = pSatMode      ? pSatMode->load()      : 1.0f;

        // Stage 2: Corroder
        const float corrMix      = pCorrMix      ? pCorrMix->load()      : 0.0f;
        const float corrBits     = pCorrBits     ? pCorrBits->load()     : 24.0f;
        const float corrSR       = pCorrSR       ? pCorrSR->load()       : 44100.0f;
        const float corrFM       = pCorrFM       ? pCorrFM->load()       : 0.0f;
        const float corrTone     = pCorrTone     ? pCorrTone->load()     : 1.0f;

        // Stage 3: Vibe Knob
        const float vibeAmount   = pVibeAmount   ? pVibeAmount->load()   : 0.0f;

        // Stage 4: Spectral Tilt
        const float tiltAmount   = pTiltAmount   ? pTiltAmount->load()   : 0.0f;
        const float tiltMix      = pTiltMix      ? pTiltMix->load()      : 1.0f;

        // Stage 5: Transient Designer
        const float tdAttack     = pTDAttack     ? pTDAttack->load()     : 0.0f;
        const float tdSustain    = pTDSustain    ? pTDSustain->load()    : 0.0f;
        const float tdMix        = pTDMix        ? pTDMix->load()        : 0.0f;

        // Stage 5.5: fXObscura
        const float obsThresh    = pObsThresh    ? pObsThresh->load()    : -24.0f;
        const float obsHold      = pObsHold      ? pObsHold->load()      : -12.0f;
        const float obsRelease   = pObsRelease   ? pObsRelease->load()   : 100.0f;
        const float obsErosion   = pObsErosion   ? pObsErosion->load()   : 2.0f;
        const float obsSubHarm   = pObsSubHarm   ? pObsSubHarm->load()   : 0.5f;
        const float obsSat       = pObsSat       ? pObsSat->load()       : 0.4f;
        const float obsDecimate  = pObsDecimate  ? pObsDecimate->load()  : 0.3f;
        const float obsRes       = pObsRes       ? pObsRes->load()       : 0.5f;
        const float obsTone      = pObsTone      ? pObsTone->load()      : 3000.0f;
        const float obsPatina    = pObsPatina    ? pObsPatina->load()    : 0.03f;
        const float obsMix       = pObsMix       ? pObsMix->load()       : 0.0f;

        // Stage 6: fXOsmosis
        const float osmMembrane  = pOsmMembrane  ? pOsmMembrane->load()  : 0.5f;
        const float osmReact     = pOsmReact     ? pOsmReact->load()     : 0.5f;
        const float osmRes       = pOsmRes       ? pOsmRes->load()       : 0.4f;
        const float osmSat       = pOsmSat       ? pOsmSat->load()       : 0.3f;
        const float osmMix       = pOsmMix       ? pOsmMix->load()       : 0.0f;

        // Stage 7: Multiband OTT (pre-spatial)
        const float ottMix       = pOttMix       ? pOttMix->load()       : 0.0f;
        const float ottDepth     = pOttDepth     ? pOttDepth->load()     : 0.7f;

        // Stage 8: Delay
        const float delayTime    = pDelayTime    ? pDelayTime->load()    : 375.0f;
        const float delayFB      = pDelayFB      ? pDelayFB->load()      : 0.3f;
        const float delayMix     = pDelayMix     ? pDelayMix->load()     : 0.0f;
        const float delayPP      = pDelayPP      ? pDelayPP->load()      : 0.5f;
        const float delayDamp    = pDelayDamp    ? pDelayDamp->load()    : 0.3f;
        const float delayDiff    = pDelayDiff    ? pDelayDiff->load()    : 0.0f;
        const float delaySync    = pDelaySync    ? pDelaySync->load()    : 0.0f;

        // Stage 8.5: fXOratory
        const float oraPattern   = pOraPattern   ? pOraPattern->load()   : 6.0f;
        const float oraSyllable  = pOraSyllable  ? pOraSyllable->load()  : 80.0f;
        const float oraAccent    = pOraAccent    ? pOraAccent->load()    : 0.7f;
        const float oraSpread    = pOraSpread    ? pOraSpread->load()    : 0.6f;
        const float oraFeedback  = pOraFeedback  ? pOraFeedback->load()  : 0.4f;
        const float oraDamping   = pOraDamping   ? pOraDamping->load()   : 3000.0f;
        const float oraDampRes   = pOraDampRes   ? pOraDampRes->load()   : 0.2f;
        const float oraDrift     = pOraDrift     ? pOraDrift->load()     : 0.0f;
        const float oraMix       = pOraMix       ? pOraMix->load()       : 0.0f;

        // Stage 6: Combulator
        const float combMix      = pCombMix      ? pCombMix->load()      : 0.0f;
        const float combFreq     = pCombFreq     ? pCombFreq->load()     : 220.0f;
        const float combFB       = pCombFB       ? pCombFB->load()       : 0.85f;
        const float combDamp     = pCombDamp     ? pCombDamp->load()     : 0.3f;
        const float combNoise    = pCombNoise    ? pCombNoise->load()    : 0.0f;
        const float combSpread   = pCombSpread   ? pCombSpread->load()   : 0.5f;
        const float combOff2     = pCombOff2     ? pCombOff2->load()     : 7.0f;
        const float combOff3     = pCombOff3     ? pCombOff3->load()     : 12.0f;

        // Stage 7: Doppler
        const float dopplerDist  = pDopplerDist  ? pDopplerDist->load()  : 0.0f;
        const float dopplerSpeed = pDopplerSpeed ? pDopplerSpeed->load() : 0.3f;
        const float dopplerMix   = pDopplerMix   ? pDopplerMix->load()   : 0.0f;

        // Stage 11: Reverb (FATHOM 8-tap Hadamard FDN)
        const float reverbSize      = pReverbSize  ? pReverbSize->load()  : 0.5f;
        const float reverbMix       = pReverbMix   ? pReverbMix->load()   : 0.0f;
        const float reverbPreDelay  = pReverbPreDelay  ? pReverbPreDelay->load()  : 0.0f;
        const float reverbDecay     = pReverbDecay     ? pReverbDecay->load()     : 2.0f;
        const float reverbDamping   = pReverbDamping   ? pReverbDamping->load()   : 0.4f;
        const float reverbDiffusion = pReverbDiffusion ? pReverbDiffusion->load() : 0.5f;
        const float reverbMod       = pReverbMod       ? pReverbMod->load()       : 0.3f;
        const float reverbWidth     = pReverbWidth     ? pReverbWidth->load()     : 1.0f;

        // Stage 9: Frequency Shifter
        const float fshiftHz     = pFShiftHz     ? pFShiftHz->load()     : 0.0f;
        const float fshiftMix    = pFShiftMix    ? pFShiftMix->load()    : 0.0f;
        const float fshiftMode   = pFShiftMode   ? pFShiftMode->load()   : 0.0f;
        const float fshiftFB     = pFShiftFB     ? pFShiftFB->load()     : 0.0f;

        // Stage 12: fXOneiric
        const float onDelayMs    = pOnDelayMs    ? pOnDelayMs->load()    : 350.0f;
        const float onShiftHz    = pOnShiftHz    ? pOnShiftHz->load()    : 5.0f;
        const float onFeedback   = pOnFeedback   ? pOnFeedback->load()   : 0.6f;
        const float onDamping    = pOnDamping    ? pOnDamping->load()    : 0.3f;
        const float onSpread     = pOnSpread     ? pOnSpread->load()     : 0.3f;
        const float onMix        = pOnMix        ? pOnMix->load()        : 0.0f;

        // Stage 14.5: fXOnslaught
        const float onslFlowRate  = pOnslFlowRate  ? pOnslFlowRate->load()  : 0.8f;
        const float onslFlowDepth = pOnslFlowDepth ? pOnslFlowDepth->load() : 0.5f;
        const float onslThresh    = pOnslThresh    ? pOnslThresh->load()    : 4.0f;
        const float onslModDepth  = pOnslModDepth  ? pOnslModDepth->load()  : 0.6f;
        const float onslModRate   = pOnslModRate   ? pOnslModRate->load()   : 2.0f;
        const float onslDecay     = pOnslDecay     ? pOnslDecay->load()     : 300.0f;
        const float onslSCHP      = pOnslSCHP      ? pOnslSCHP->load()      : 200.0f;
        const float onslMix       = pOnslMix       ? pOnslMix->load()       : 0.0f;

        // Stage 13: Modulation
        const float modRate      = pModRate      ? pModRate->load()      : 0.8f;
        const float modDepth     = pModDepth     ? pModDepth->load()     : 0.0f;
        const float modMix       = pModMix       ? pModMix->load()       : 0.0f;
        const float modMode      = pModMode      ? pModMode->load()      : 0.0f;
        const float modFeedback  = pModFB        ? pModFB->load()        : 0.0f;

        // Stage 11: Granular Smear
        const float smearAmt     = pSmearAmt     ? pSmearAmt->load()     : 0.0f;
        const float smearGrain   = pSmearGrain   ? pSmearGrain->load()   : 60.0f;
        const float smearDensity = pSmearDensity ? pSmearDensity->load() : 0.5f;
        const float smearMix     = pSmearMix     ? pSmearMix->load()     : 0.0f;

        // Stage 12: Harmonic Exciter
        const float excDrive     = pExcDrive     ? pExcDrive->load()     : 0.0f;
        const float excFreq      = pExcFreq      ? pExcFreq->load()      : 3500.0f;
        const float excTone      = pExcTone      ? pExcTone->load()      : 0.7f;
        const float excMix       = pExcMix       ? pExcMix->load()       : 0.0f;

        // Stage 13: Stereo Sculptor
        const float sculLowW     = pSculLowW     ? pSculLowW->load()     : 0.0f;
        const float sculMidW     = pSculMidW     ? pSculMidW->load()     : 1.0f;
        const float sculHighW    = pSculHighW    ? pSculHighW->load()    : 1.5f;
        const float sculLowX     = pSculLowX     ? pSculLowX->load()     : 200.0f;
        const float sculHighX    = pSculHighX    ? pSculHighX->load()    : 4000.0f;
        const float sculMix      = pSculMix      ? pSculMix->load()      : 0.0f;

        // Stage 14: Psychoacoustic Width
        const float pWidthAmt    = pPWidthAmt    ? pPWidthAmt->load()    : 0.0f;
        const float pWidthHaas   = pPWidthHaas   ? pPWidthHaas->load()   : 8.0f;
        const float pWidthComb   = pPWidthComb   ? pPWidthComb->load()   : 600.0f;
        const float pWidthMono   = pPWidthMono   ? pPWidthMono->load()   : 0.5f;
        const float pWidthMix    = pPWidthMix    ? pPWidthMix->load()    : 0.0f;

        // Stage 19: Bus Compressor
        const float compRatio    = pCompRatio   ? pCompRatio->load()   : 4.0f;
        const float compAttack   = pCompAttack  ? pCompAttack->load()  : 10.0f;
        const float compRelease  = pCompRelease ? pCompRelease->load() : 100.0f;
        const float compMix      = pCompMix     ? pCompMix->load()     : 0.0f;

        // Stage 19.5: Parametric EQ
        const float eqB1Freq  = pEQB1Freq  ? pEQB1Freq->load()  : 100.0f;
        const float eqB1Gain  = pEQB1Gain  ? pEQB1Gain->load()  : 0.0f;
        const float eqB1Q     = pEQB1Q     ? pEQB1Q->load()     : 0.707f;
        const float eqB2Freq  = pEQB2Freq  ? pEQB2Freq->load()  : 400.0f;
        const float eqB2Gain  = pEQB2Gain  ? pEQB2Gain->load()  : 0.0f;
        const float eqB2Q     = pEQB2Q     ? pEQB2Q->load()     : 1.0f;
        const float eqB3Freq  = pEQB3Freq  ? pEQB3Freq->load()  : 3000.0f;
        const float eqB3Gain  = pEQB3Gain  ? pEQB3Gain->load()  : 0.0f;
        const float eqB3Q     = pEQB3Q     ? pEQB3Q->load()     : 1.0f;
        const float eqB4Freq  = pEQB4Freq  ? pEQB4Freq->load()  : 10000.0f;
        const float eqB4Gain  = pEQB4Gain  ? pEQB4Gain->load()  : 0.0f;
        const float eqB4Q     = pEQB4Q     ? pEQB4Q->load()     : 0.707f;

        // Stage 20: Brickwall Limiter
        const float limCeiling   = pLimCeiling   ? pLimCeiling->load()   : -0.3f;
        const float limRelease   = pLimRelease   ? pLimRelease->load()   : 50.0f;

        // Stage 17: Sequencer
        const float seqEnabled   = pSeqEnabled   ? pSeqEnabled->load()   : 0.0f;
        const float seqRate      = pSeqRate      ? pSeqRate->load()      : 2.0f;
        const float seqSteps     = pSeqSteps     ? pSeqSteps->load()     : 8.0f;
        const float seqDepth     = pSeqDepth     ? pSeqDepth->load()     : 0.5f;
        const float seqSmooth    = pSeqSmooth    ? pSeqSmooth->load()    : 0.3f;
        const float seqTarget1   = pSeqTarget1   ? pSeqTarget1->load()   : 0.0f;
        const float seqTarget2   = pSeqTarget2   ? pSeqTarget2->load()   : 0.0f;
        const float seqPattern   = pSeqPattern   ? pSeqPattern->load()   : 0.0f;
        const float seqEnvFollow = pSeqEnvFollow ? pSeqEnvFollow->load() : 0.0f;
        const float seqEnvAmt    = pSeqEnvAmt    ? pSeqEnvAmt->load()    : 0.5f;

        // ====================================================================
        // Stage 17: Sequencer (non-audio, runs first to compute mod offsets)
        // ====================================================================
        float seqMod1 = 0.0f;
        float seqMod2 = 0.0f;

        if (seqEnabled > 0.5f)
        {
            sequencer.setEnabled (true);
            sequencer.setPattern (static_cast<MasterFXSequencer::Pattern> (static_cast<int> (seqPattern)));
            sequencer.setSteps (static_cast<int> (seqSteps));
            sequencer.setDepth (seqDepth);
            sequencer.setSmooth (seqSmooth);
            sequencer.setRate (static_cast<MasterFXSequencer::ClockDiv> (static_cast<int> (seqRate)));
            sequencer.setTarget1 (static_cast<MasterFXSequencer::Target> (static_cast<int> (seqTarget1)));
            sequencer.setTarget2 (static_cast<MasterFXSequencer::Target> (static_cast<int> (seqTarget2)));
            sequencer.setEnvFollowEnabled (seqEnvFollow > 0.5f);
            sequencer.setEnvFollowAmount (seqEnvAmt);

            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += L[i] * L[i] + R[i] * R[i];
            // SRO: fast sqrt via 2^(0.5 * log2(x)) replaces std::sqrt (per-block, sequencer only)
            float rmsArg = rms / static_cast<float> (numSamples * 2);
            rms = (rmsArg > 1e-10f) ? fastPow2 (0.5f * fastLog2 (rmsArg)) : 0.0f;

            sequencer.updateBlock (ppqPosition, bpm, numSamples, rms);
            seqMod1 = sequencer.getModValue1();
            seqMod2 = sequencer.getModValue2();
        }
        else
        {
            sequencer.setEnabled (false);
            sequencer.updateBlock (ppqPosition, bpm, numSamples, 0.0f);
        }

        // Helper: apply sequencer modulation offset to a parameter
        auto applySeqMod = [&] (float baseValue, MasterFXSequencer::Target paramTarget,
                                float maxOffset) -> float
        {
            float offset = 0.0f;
            if (sequencer.getTarget1() == paramTarget) offset += seqMod1;
            if (sequencer.getTarget2() == paramTarget) offset += seqMod2;
            return clamp (baseValue + offset * maxOffset, 0.0f, 1.0f);
        };

        // Bipolar variant (for spectral tilt: -1..+1)
        auto applySeqModBipolar = [&] (float baseValue, MasterFXSequencer::Target paramTarget,
                                       float maxOffset) -> float
        {
            float offset = 0.0f;
            if (sequencer.getTarget1() == paramTarget) offset += seqMod1;
            if (sequencer.getTarget2() == paramTarget) offset += seqMod2;
            return clamp (baseValue + offset * maxOffset, -1.0f, 1.0f);
        };

        // ====================================================================
        // Stage 1: Saturation (with mode select)
        // ====================================================================
        float effectiveSatDrive = applySeqMod (satDrive,
            MasterFXSequencer::Target::SatDrive, 0.5f);

        if (effectiveSatDrive > 0.001f)
        {
            auto mode = static_cast<Saturator::SaturationMode> (
                clamp (static_cast<int> (satMode), 0, 3));
            saturator.setMode (mode);
            saturator.setOutputGain (mode == Saturator::SaturationMode::FoldBack ? 0.7f : 0.85f);
            saturator.setDrive (effectiveSatDrive);
            saturator.setMix (effectiveSatDrive);
            saturator.processBlock (L, numSamples);
            saturator.processBlock (R, numSamples);
        }

        // ====================================================================
        // Stage 2: Corroder (digital erosion)
        // ====================================================================
        if (corrMix > 0.001f)
        {
            corroder.setBitDepth (corrBits);
            corroder.setSampleRate (corrSR);
            corroder.setFMDepth (corrFM);
            corroder.setTone (corrTone);
            corroder.setMix (corrMix);
            corroder.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 3: Vibe Knob (GRIT ←→ SWEET)
        // ====================================================================
        float effectiveVibe = vibeAmount;
        {
            float vibeOffset = 0.0f;
            if (sequencer.getTarget1() == MasterFXSequencer::Target::VibeKnob)
                vibeOffset += seqMod1;
            if (sequencer.getTarget2() == MasterFXSequencer::Target::VibeKnob)
                vibeOffset += seqMod2;
            effectiveVibe = clamp (vibeAmount + vibeOffset * 0.8f, -1.0f, 1.0f);
        }

        if (std::abs (effectiveVibe) > 0.001f)
        {
            vibeKnob.setVibe (effectiveVibe);
            vibeKnob.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 4: Spectral Tilt
        // ====================================================================
        float effectiveTilt = applySeqModBipolar (tiltAmount,
            MasterFXSequencer::Target::SpectralTilt, 0.8f);

        if (std::abs (effectiveTilt) > 0.001f && tiltMix > 0.001f)
        {
            spectralTilt.setTilt (effectiveTilt);
            spectralTilt.setMix (tiltMix);
            spectralTilt.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 5: Transient Designer
        // ====================================================================
        float effectiveTDAttack = applySeqModBipolar (tdAttack,
            MasterFXSequencer::Target::TransientAttack, 0.6f);

        if (tdMix > 0.001f && (std::abs (effectiveTDAttack) > 0.001f || std::abs (tdSustain) > 0.001f))
        {
            transientDesigner.setAttack (effectiveTDAttack);
            transientDesigner.setSustain (tdSustain);
            transientDesigner.setMix (tdMix);
            transientDesigner.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 5.5: fXObscura (Chiaroscuro — Inverse-Dynamic Degradation)
        // ====================================================================
        if (obsMix > 0.001f)
        {
            obscura.setThreshold (obsThresh);
            obscura.setHoldLevel (obsHold);
            obscura.setRelease (obsRelease);
            obscura.setErosionCurve (obsErosion);
            obscura.setSubHarmonic (obsSubHarm);
            obscura.setSaturation (obsSat);
            obscura.setDecimate (obsDecimate);
            obscura.setResonance (obsRes);
            obscura.setTone (obsTone);
            obscura.setPatina (obsPatina);
            obscura.setMix (obsMix);
            obscura.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 5.6: fXFormant (Membrane Collection — Formant Filter)
        // ====================================================================
        {
            const float fmtMix = pFormantMix_ ? pFormantMix_->load() : 0.0f;
            if (fmtMix > 0.001f)
            {
                formantFX_.setShift     (pFormantShift_ ? pFormantShift_->load() : 0.0f);
                formantFX_.setVowel     (pFormantVowel_ ? pFormantVowel_->load() : 0.0f);
                formantFX_.setResonance (pFormantQ_     ? pFormantQ_->load()     : 8.0f);
                formantFX_.setMix       (fmtMix);
                formantFX_.processBlock (buffer.getWritePointer (0),
                                         buffer.getWritePointer (1),
                                         numSamples);
            }
        }

        // ====================================================================
        // Stage 5.7: fXBreath (Membrane Collection — Breath Texture)
        // ====================================================================
        {
            const float brMix = pBreathMix_ ? pBreathMix_->load() : 0.0f;
            if (brMix > 0.001f)
            {
                breathFX_.setBreathAmount (pBreathAmount_ ? pBreathAmount_->load() : 0.0f);
                breathFX_.setTilt         (pBreathTilt_   ? pBreathTilt_->load()   : 0.5f);
                breathFX_.setSensitivity  (pBreathSens_   ? pBreathSens_->load()   : 0.5f);
                breathFX_.setMix          (brMix);
                breathFX_.processBlock (buffer.getWritePointer (0),
                                        buffer.getWritePointer (1),
                                        numSamples);
            }
        }

        // ====================================================================
        // Stage 6: fXOsmosis (Membrane Transfer)
        // ====================================================================
        float effectiveOsmMix = applySeqMod (osmMix,
            MasterFXSequencer::Target::OsmosisMix, 0.6f);

        if (effectiveOsmMix > 0.001f)
        {
            osmosis.setMembraneTone (osmMembrane);
            osmosis.setReactivity (osmReact);
            osmosis.setResonance (osmRes);
            osmosis.setSaturation (osmSat);
            osmosis.setMix (effectiveOsmMix);
            osmosis.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 7: Multiband OTT (pre-spatial dynamics)
        // ====================================================================
        if (ottMix > 0.001f)
        {
            multibandComp.setDepth (ottDepth);
            multibandComp.setMix (ottMix);
            multibandComp.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 8: Stereo Delay
        // ====================================================================
        float effectiveDelayMix = applySeqMod (delayMix,
            MasterFXSequencer::Target::DelayMix, 0.8f);
        float effectiveDelayFB = applySeqMod (delayFB,
            MasterFXSequencer::Target::DelayFeedback, 0.4f);

        if (effectiveDelayMix > 0.001f)
        {
            delay.setDelayTime (delayTime);
            delay.setFeedback (effectiveDelayFB);
            delay.setMix (effectiveDelayMix);
            delay.setPingPong (delayPP);
            delay.setDamping (delayDamp);
            delay.setDiffusion (delayDiff);
            delay.setSyncDiv (static_cast<MasterDelay::SyncDiv> (static_cast<int> (delaySync)));
            delay.setBPM (bpm);
            delay.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 8.5: fXOratory (Poetic Meter Delay)
        // ====================================================================
        if (oraMix > 0.001f)
        {
            oratory.setPattern (static_cast<int> (oraPattern));
            oratory.setSyllable (oraSyllable);
            oratory.setAccent (oraAccent);
            oratory.setSpread (oraSpread);
            oratory.setFeedback (oraFeedback);
            oratory.setDamping (oraDamping);
            oratory.setDampingResonance (oraDampRes);
            oratory.setDrift (oraDrift);
            oratory.setMix (oraMix);
            oratory.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 6: Combulator (tuned comb bank)
        // ====================================================================
        if (combMix > 0.001f)
        {
            combulator.setFrequency (combFreq);
            combulator.setFeedback (combFB);
            combulator.setDamping (combDamp);
            combulator.setNoiseLevel (combNoise);
            combulator.setStereoSpread (combSpread);
            combulator.setComb2Offset (combOff2);
            combulator.setComb3Offset (combOff3);
            combulator.setMix (combMix);
            combulator.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 7: Doppler Effect
        // ====================================================================
        float effectiveDopplerDist = applySeqMod (dopplerDist,
            MasterFXSequencer::Target::DopplerDistance, 0.7f);

        if (dopplerMix > 0.001f && effectiveDopplerDist > 0.001f)
        {
            doppler.setDistance (effectiveDopplerDist);
            doppler.setSpeed (dopplerSpeed);
            doppler.setMix (dopplerMix);
            doppler.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 11: Space Reverb (FATHOM 8-tap Hadamard FDN)
        // ====================================================================
        float effectiveReverbMix = applySeqMod (reverbMix,
            MasterFXSequencer::Target::ReverbMix, 0.6f);

        if (effectiveReverbMix > 0.001f)
        {
            // Map reverbSize [0,1] → Size param; reverbDecay [0.5,20]s
            reverb.setPreDelay  (reverbPreDelay);
            reverb.setSize      (reverbSize);
            reverb.setDecay     (reverbDecay);
            reverb.setDamping   (reverbDamping);
            reverb.setDiffusion (reverbDiffusion);
            reverb.setModulation(reverbMod);
            reverb.setMix       (effectiveReverbMix);
            reverb.setWidth     (reverbWidth);
            reverb.processBlock (L, R, L, R, numSamples);
        }

        // ====================================================================
        // Stage 9: Frequency Shifter
        // ====================================================================
        if (fshiftMix > 0.001f)
        {
            freqShifter.setShift (fshiftHz);
            freqShifter.setMix (fshiftMix);
            freqShifter.setMode (static_cast<FrequencyShifter::Mode> (
                clamp (static_cast<int> (fshiftMode), 0, 2)));
            freqShifter.setFeedback (fshiftFB);
            freqShifter.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 12: fXOneiric (Dream State)
        // ====================================================================
        float effectiveOnMix = applySeqMod (onMix,
            MasterFXSequencer::Target::OneiricMix, 0.6f);

        if (effectiveOnMix > 0.001f)
        {
            oneiric.setDelayTime (onDelayMs);
            oneiric.setShift (onShiftHz);
            oneiric.setFeedback (onFeedback);
            oneiric.setDamping (onDamping);
            oneiric.setSpread (onSpread);
            oneiric.setMix (effectiveOnMix);
            oneiric.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 13: Modulation FX
        // ====================================================================
        float effectiveModDepth = applySeqMod (modDepth,
            MasterFXSequencer::Target::ModDepth, 0.7f);
        float effectiveModRate = modRate;
        {
            float rateOffset = 0.0f;
            if (sequencer.getTarget1() == MasterFXSequencer::Target::ModRate)
                rateOffset += seqMod1;
            if (sequencer.getTarget2() == MasterFXSequencer::Target::ModRate)
                rateOffset += seqMod2;
            effectiveModRate = clamp (modRate + rateOffset * 5.0f, 0.01f, 15.0f);
        }

        if (effectiveModDepth > 0.001f && modMix > 0.001f)
        {
            modulation.setMode (static_cast<MasterModulation::Mode> (static_cast<int> (modMode)));
            modulation.setRate (effectiveModRate);
            modulation.setDepth (effectiveModDepth);
            modulation.setMix (modMix);
            modulation.setFeedback (modFeedback);
            modulation.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 14.5: fXOnslaught (Transient-Reactive Chorus → PM Collapse)
        // ====================================================================
        if (onslMix > 0.001f)
        {
            onslaught.setFlowRate (onslFlowRate);
            onslaught.setFlowDepth (onslFlowDepth);
            onslaught.setThreshold (onslThresh);
            onslaught.setCollapseDepth (onslModDepth);
            onslaught.setCollapseRate (onslModRate);
            onslaught.setCollapseDecay (onslDecay);
            onslaught.setSidechainHP (onslSCHP);
            onslaught.setMix (onslMix);
            onslaught.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 11: Granular Smear
        // ====================================================================
        float effectiveSmear = applySeqMod (smearAmt,
            MasterFXSequencer::Target::GranularSmear, 0.6f);

        if (smearMix > 0.001f && effectiveSmear > 0.001f)
        {
            granularSmear.setSmear (effectiveSmear);
            granularSmear.setGrainSize (smearGrain);
            granularSmear.setDensity (smearDensity);
            granularSmear.setMix (smearMix);
            granularSmear.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 12: Harmonic Exciter
        // ====================================================================
        float effectiveExcDrive = applySeqMod (excDrive,
            MasterFXSequencer::Target::ExciterDrive, 0.5f);

        if (excMix > 0.001f && effectiveExcDrive > 0.001f)
        {
            harmonicExciter.setDrive (effectiveExcDrive);
            harmonicExciter.setFreq (excFreq);
            harmonicExciter.setTone (excTone);
            harmonicExciter.setMix (excMix);
            harmonicExciter.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 13: Stereo Sculptor
        // ====================================================================
        float effectiveSculMidW = sculMidW;
        {
            float widthOffset = 0.0f;
            if (sequencer.getTarget1() == MasterFXSequencer::Target::StereoWidth)
                widthOffset += seqMod1;
            if (sequencer.getTarget2() == MasterFXSequencer::Target::StereoWidth)
                widthOffset += seqMod2;
            effectiveSculMidW = clamp (sculMidW + widthOffset * 1.0f, 0.0f, 2.0f);
        }

        if (sculMix > 0.001f)
        {
            stereoSculptor.setLowWidth (sculLowW);
            stereoSculptor.setMidWidth (effectiveSculMidW);
            stereoSculptor.setHighWidth (sculHighW);
            stereoSculptor.setLowCrossover (sculLowX);
            stereoSculptor.setHighCrossover (sculHighX);
            stereoSculptor.setMix (sculMix);
            stereoSculptor.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 14: Psychoacoustic Width
        // ====================================================================
        float effectivePWidth = applySeqMod (pWidthAmt,
            MasterFXSequencer::Target::PsychoWidth, 0.6f);

        if (pWidthMix > 0.001f && effectivePWidth > 0.001f)
        {
            psychoWidth.setWidth (effectivePWidth);
            psychoWidth.setHaasMs (pWidthHaas);
            psychoWidth.setCombFreq (pWidthComb);
            psychoWidth.setMonoSafe (pWidthMono);
            psychoWidth.setMix (pWidthMix);
            psychoWidth.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 19: Bus Compressor (with wet/dry blend)
        // ====================================================================
        float effectiveCompMix = applySeqMod (compMix,
            MasterFXSequencer::Target::CompMix, 0.5f);
        effectiveCompMix = clamp (effectiveCompMix, 0.0f, 1.0f);

        if (effectiveCompMix > 0.001f)
        {
            compressor.setRatio (compRatio);
            compressor.setAttack (compAttack);
            compressor.setRelease (compRelease);

            if (effectiveCompMix >= 0.999f)
            {
                compressor.processBlock (L, R, numSamples);
            }
            else
            {
                const int safeSamples = juce::jmin (numSamples, dryBuffer.getNumSamples());
                dryBuffer.copyFrom (0, 0, buffer, 0, 0, safeSamples);
                dryBuffer.copyFrom (1, 0, buffer, 1, 0, safeSamples);
                compressor.processBlock (L, R, numSamples);
                buffer.applyGain (effectiveCompMix);
                const float dryGain = 1.0f - effectiveCompMix;
                buffer.addFrom (0, 0, dryBuffer, 0, 0, safeSamples, dryGain);
                buffer.addFrom (1, 0, dryBuffer, 1, 0, safeSamples, dryGain);
            }
        }

        // ====================================================================
        // Stage 19.5: Parametric EQ (post-compressor, pre-limiter)
        // ====================================================================
        {
            // Only engage if at least one band has a non-trivial gain.
            // setBand() is cheap (just sets smoother targets) so we always push
            // the current values — the EQ itself handles the zero-CPU bypass
            // when all gains are flat.
            parametricEQ.setBand (0, eqB1Freq, eqB1Gain, eqB1Q);
            parametricEQ.setBand (1, eqB2Freq, eqB2Gain, eqB2Q);
            parametricEQ.setBand (2, eqB3Freq, eqB3Gain, eqB3Q);
            parametricEQ.setBand (3, eqB4Freq, eqB4Gain, eqB4Q);
            parametricEQ.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 20: Brickwall Limiter (always active — safety net)
        // ====================================================================
        limiter.setCeiling (limCeiling);
        limiter.setRelease (limRelease);
        limiter.processBlock (L, R, numSamples);

        // ====================================================================
        // Stage 21: DC Blocker (always active — safety net)
        // ====================================================================
        dcBlocker.processBlock (L, R, numSamples);
    }

    //--------------------------------------------------------------------------
    /// Legacy processBlock (no transport info)
    void processBlock (juce::AudioBuffer<float>& buffer, int numSamples)
    {
        processBlock (buffer, numSamples, -1.0, 0.0);
    }

    //--------------------------------------------------------------------------
    float getCompGainReduction() const { return compressor.getGainReduction(); }
    float getLimiterGainReduction() const { return limiter.getGainReduction(); }
    int getSeqCurrentStep() const { return sequencer.getCurrentStep(); }
    bool isSeqEnabled() const { return sequencer.isEnabled(); }
    MasterDelay& getDelay() { return delay; }

    //--------------------------------------------------------------------------
    void reset()
    {
        saturator.reset();
        corroder.reset();
        vibeKnob.reset();
        spectralTilt.reset();
        transientDesigner.reset();
        obscura.reset();
        formantFX_.reset();
        breathFX_.reset();
        osmosis.reset();
        multibandComp.reset();
        delay.reset();
        oratory.reset();
        combulator.reset();
        doppler.reset();
        reverb.reset();
        freqShifter.reset();
        oneiric.reset();
        modulation.reset();
        onslaught.reset();
        granularSmear.reset();
        harmonicExciter.reset();
        stereoSculptor.reset();
        psychoWidth.reset();
        compressor.reset();
        parametricEQ.reset();
        limiter.reset();
        dcBlocker.reset();
        sequencer.reset();
    }

private:
    void cacheParameterPointers (juce::AudioProcessorValueTreeState& apvts)
    {
        // Stage 1: Saturation
        pSatDrive     = apvts.getRawParameterValue ("master_satDrive");
        pSatMode      = apvts.getRawParameterValue ("master_satMode");

        // Stage 2: Corroder
        pCorrMix      = apvts.getRawParameterValue ("master_corrMix");
        pCorrBits     = apvts.getRawParameterValue ("master_corrBits");
        pCorrSR       = apvts.getRawParameterValue ("master_corrSR");
        pCorrFM       = apvts.getRawParameterValue ("master_corrFM");
        pCorrTone     = apvts.getRawParameterValue ("master_corrTone");

        // Stage 3: Vibe Knob
        pVibeAmount   = apvts.getRawParameterValue ("master_vibeAmount");

        // Stage 4: Spectral Tilt
        pTiltAmount   = apvts.getRawParameterValue ("master_tiltAmount");
        pTiltMix      = apvts.getRawParameterValue ("master_tiltMix");

        // Stage 5: Transient Designer
        pTDAttack     = apvts.getRawParameterValue ("master_tdAttack");
        pTDSustain    = apvts.getRawParameterValue ("master_tdSustain");
        pTDMix        = apvts.getRawParameterValue ("master_tdMix");

        // Stage 5.5: fXObscura
        pObsThresh    = apvts.getRawParameterValue ("master_obsThreshold");
        pObsHold      = apvts.getRawParameterValue ("master_obsHold");
        pObsRelease   = apvts.getRawParameterValue ("master_obsRelease");
        pObsErosion   = apvts.getRawParameterValue ("master_obsErosion");
        pObsSubHarm   = apvts.getRawParameterValue ("master_obsSubHarm");
        pObsSat       = apvts.getRawParameterValue ("master_obsSaturation");
        pObsDecimate  = apvts.getRawParameterValue ("master_obsDecimate");
        pObsRes       = apvts.getRawParameterValue ("master_obsResonance");
        pObsTone      = apvts.getRawParameterValue ("master_obsTone");
        pObsPatina    = apvts.getRawParameterValue ("master_obsPatina");
        pObsMix       = apvts.getRawParameterValue ("master_obsMix");

        // Stage 5.6: fXFormant (Membrane Collection)
        pFormantShift_ = apvts.getRawParameterValue ("mfx_formantShift");
        pFormantVowel_ = apvts.getRawParameterValue ("mfx_formantVowel");
        pFormantQ_     = apvts.getRawParameterValue ("mfx_formantQ");
        pFormantMix_   = apvts.getRawParameterValue ("mfx_formantMix");

        // Stage 5.7: fXBreath (Membrane Collection)
        pBreathAmount_ = apvts.getRawParameterValue ("mfx_breathAmount");
        pBreathTilt_   = apvts.getRawParameterValue ("mfx_breathTilt");
        pBreathSens_   = apvts.getRawParameterValue ("mfx_breathSens");
        pBreathMix_    = apvts.getRawParameterValue ("mfx_breathMix");

        // Stage 6: fXOsmosis
        pOsmMembrane  = apvts.getRawParameterValue ("master_osmMembrane");
        pOsmReact     = apvts.getRawParameterValue ("master_osmReactivity");
        pOsmRes       = apvts.getRawParameterValue ("master_osmResonance");
        pOsmSat       = apvts.getRawParameterValue ("master_osmSaturation");
        pOsmMix       = apvts.getRawParameterValue ("master_osmMix");

        // Stage 7: Multiband OTT
        pOttMix       = apvts.getRawParameterValue ("master_ottMix");
        pOttDepth     = apvts.getRawParameterValue ("master_ottDepth");

        // Stage 8: Delay
        pDelayTime    = apvts.getRawParameterValue ("master_delayTime");
        pDelayFB      = apvts.getRawParameterValue ("master_delayFeedback");
        pDelayMix     = apvts.getRawParameterValue ("master_delayMix");
        pDelayPP      = apvts.getRawParameterValue ("master_delayPingPong");
        pDelayDamp    = apvts.getRawParameterValue ("master_delayDamping");
        pDelayDiff    = apvts.getRawParameterValue ("master_delayDiffusion");
        pDelaySync    = apvts.getRawParameterValue ("master_delaySync");

        // Stage 8.5: fXOratory
        pOraPattern   = apvts.getRawParameterValue ("master_oraPattern");
        pOraSyllable  = apvts.getRawParameterValue ("master_oraSyllable");
        pOraAccent    = apvts.getRawParameterValue ("master_oraAccent");
        pOraSpread    = apvts.getRawParameterValue ("master_oraSpread");
        pOraFeedback  = apvts.getRawParameterValue ("master_oraFeedback");
        pOraDamping   = apvts.getRawParameterValue ("master_oraDamping");
        pOraDampRes   = apvts.getRawParameterValue ("master_oraDampRes");
        pOraDrift     = apvts.getRawParameterValue ("master_oraDrift");
        pOraMix       = apvts.getRawParameterValue ("master_oraMix");

        // Stage 6: Combulator
        pCombMix      = apvts.getRawParameterValue ("master_combMix");
        pCombFreq     = apvts.getRawParameterValue ("master_combFreq");
        pCombFB       = apvts.getRawParameterValue ("master_combFeedback");
        pCombDamp     = apvts.getRawParameterValue ("master_combDamping");
        pCombNoise    = apvts.getRawParameterValue ("master_combNoise");
        pCombSpread   = apvts.getRawParameterValue ("master_combSpread");
        pCombOff2     = apvts.getRawParameterValue ("master_combOffset2");
        pCombOff3     = apvts.getRawParameterValue ("master_combOffset3");

        // Stage 7: Doppler
        pDopplerDist  = apvts.getRawParameterValue ("master_dopplerDist");
        pDopplerSpeed = apvts.getRawParameterValue ("master_dopplerSpeed");
        pDopplerMix   = apvts.getRawParameterValue ("master_dopplerMix");

        // Stage 11: Reverb (FATHOM 8-tap Hadamard FDN)
        pReverbSize       = apvts.getRawParameterValue ("master_reverbSize");
        pReverbMix        = apvts.getRawParameterValue ("master_reverbMix");
        pReverbPreDelay   = apvts.getRawParameterValue ("master_reverbPreDelay");
        pReverbDecay      = apvts.getRawParameterValue ("master_reverbDecay");
        pReverbDamping    = apvts.getRawParameterValue ("master_reverbDamping");
        pReverbDiffusion  = apvts.getRawParameterValue ("master_reverbDiffusion");
        pReverbMod        = apvts.getRawParameterValue ("master_reverbMod");
        pReverbWidth      = apvts.getRawParameterValue ("master_reverbWidth");

        // Stage 9: Frequency Shifter
        pFShiftHz     = apvts.getRawParameterValue ("master_fshiftHz");
        pFShiftMix    = apvts.getRawParameterValue ("master_fshiftMix");
        pFShiftMode   = apvts.getRawParameterValue ("master_fshiftMode");
        pFShiftFB     = apvts.getRawParameterValue ("master_fshiftFeedback");

        // Stage 12: fXOneiric
        pOnDelayMs    = apvts.getRawParameterValue ("master_onDelayTime");
        pOnShiftHz    = apvts.getRawParameterValue ("master_onShiftHz");
        pOnFeedback   = apvts.getRawParameterValue ("master_onFeedback");
        pOnDamping    = apvts.getRawParameterValue ("master_onDamping");
        pOnSpread     = apvts.getRawParameterValue ("master_onSpread");
        pOnMix        = apvts.getRawParameterValue ("master_onMix");

        // Stage 13: Modulation
        pModRate      = apvts.getRawParameterValue ("master_modRate");
        pModDepth     = apvts.getRawParameterValue ("master_modDepth");
        pModMix       = apvts.getRawParameterValue ("master_modMix");
        pModMode      = apvts.getRawParameterValue ("master_modMode");
        pModFB        = apvts.getRawParameterValue ("master_modFeedback");

        // Stage 14.5: fXOnslaught
        pOnslFlowRate  = apvts.getRawParameterValue ("master_onslFlowRate");
        pOnslFlowDepth = apvts.getRawParameterValue ("master_onslFlowDepth");
        pOnslThresh    = apvts.getRawParameterValue ("master_onslThreshold");
        pOnslModDepth  = apvts.getRawParameterValue ("master_onslModDepth");
        pOnslModRate   = apvts.getRawParameterValue ("master_onslModRate");
        pOnslDecay     = apvts.getRawParameterValue ("master_onslDecay");
        pOnslSCHP      = apvts.getRawParameterValue ("master_onslSCHP");
        pOnslMix       = apvts.getRawParameterValue ("master_onslMix");

        // Stage 11: Granular Smear
        pSmearAmt     = apvts.getRawParameterValue ("master_smearAmount");
        pSmearGrain   = apvts.getRawParameterValue ("master_smearGrainSize");
        pSmearDensity = apvts.getRawParameterValue ("master_smearDensity");
        pSmearMix     = apvts.getRawParameterValue ("master_smearMix");

        // Stage 12: Harmonic Exciter
        pExcDrive     = apvts.getRawParameterValue ("master_excDrive");
        pExcFreq      = apvts.getRawParameterValue ("master_excFreq");
        pExcTone      = apvts.getRawParameterValue ("master_excTone");
        pExcMix       = apvts.getRawParameterValue ("master_excMix");

        // Stage 13: Stereo Sculptor
        pSculLowW     = apvts.getRawParameterValue ("master_sculLowWidth");
        pSculMidW     = apvts.getRawParameterValue ("master_sculMidWidth");
        pSculHighW    = apvts.getRawParameterValue ("master_sculHighWidth");
        pSculLowX     = apvts.getRawParameterValue ("master_sculLowCross");
        pSculHighX    = apvts.getRawParameterValue ("master_sculHighCross");
        pSculMix      = apvts.getRawParameterValue ("master_sculMix");

        // Stage 14: Psychoacoustic Width
        pPWidthAmt    = apvts.getRawParameterValue ("master_pwidthAmount");
        pPWidthHaas   = apvts.getRawParameterValue ("master_pwidthHaas");
        pPWidthComb   = apvts.getRawParameterValue ("master_pwidthComb");
        pPWidthMono   = apvts.getRawParameterValue ("master_pwidthMono");
        pPWidthMix    = apvts.getRawParameterValue ("master_pwidthMix");

        // Stage 19: Bus Compressor
        pCompRatio    = apvts.getRawParameterValue ("master_compRatio");
        pCompAttack   = apvts.getRawParameterValue ("master_compAttack");
        pCompRelease  = apvts.getRawParameterValue ("master_compRelease");
        pCompMix      = apvts.getRawParameterValue ("master_compMix");

        // Stage 19.5: Parametric EQ
        pEQB1Freq     = apvts.getRawParameterValue ("master_eqB1Freq");
        pEQB1Gain     = apvts.getRawParameterValue ("master_eqB1Gain");
        pEQB1Q        = apvts.getRawParameterValue ("master_eqB1Q");
        pEQB2Freq     = apvts.getRawParameterValue ("master_eqB2Freq");
        pEQB2Gain     = apvts.getRawParameterValue ("master_eqB2Gain");
        pEQB2Q        = apvts.getRawParameterValue ("master_eqB2Q");
        pEQB3Freq     = apvts.getRawParameterValue ("master_eqB3Freq");
        pEQB3Gain     = apvts.getRawParameterValue ("master_eqB3Gain");
        pEQB3Q        = apvts.getRawParameterValue ("master_eqB3Q");
        pEQB4Freq     = apvts.getRawParameterValue ("master_eqB4Freq");
        pEQB4Gain     = apvts.getRawParameterValue ("master_eqB4Gain");
        pEQB4Q        = apvts.getRawParameterValue ("master_eqB4Q");

        // Stage 20: Brickwall Limiter
        pLimCeiling   = apvts.getRawParameterValue ("master_limCeiling");
        pLimRelease   = apvts.getRawParameterValue ("master_limRelease");

        // Stage 22: Sequencer
        pSeqEnabled   = apvts.getRawParameterValue ("master_seqEnabled");
        pSeqRate      = apvts.getRawParameterValue ("master_seqRate");
        pSeqSteps     = apvts.getRawParameterValue ("master_seqSteps");
        pSeqDepth     = apvts.getRawParameterValue ("master_seqDepth");
        pSeqSmooth    = apvts.getRawParameterValue ("master_seqSmooth");
        pSeqTarget1   = apvts.getRawParameterValue ("master_seqTarget1");
        pSeqTarget2   = apvts.getRawParameterValue ("master_seqTarget2");
        pSeqPattern   = apvts.getRawParameterValue ("master_seqPattern");
        pSeqEnvFollow = apvts.getRawParameterValue ("master_seqEnvFollow");
        pSeqEnvAmt    = apvts.getRawParameterValue ("master_seqEnvAmount");
    }

    //--------------------------------------------------------------------------
    // DSP processors (19 stages + sequencer)
    Saturator            saturator;          // 1
    Corroder             corroder;           // 2
    VibeKnob             vibeKnob;           // 3
    SpectralTilt         spectralTilt;       // 4
    TransientDesigner    transientDesigner;  // 5
    fXObscura            obscura;            // 5.5 — Chiaroscuro
    fXFormant            formantFX_;         // 5.6 — Membrane Collection: Formant Filter
    fXBreath             breathFX_;          // 5.7 — Membrane Collection: Breath Texture
    fXOsmosis            osmosis;            // 6  — Membrane Transfer
    MultibandCompressor  multibandComp;      // 7  — OTT (pre-spatial)
    MasterDelay          delay;              // 8
    fXOratory            oratory;            // 8.5 — Poetic Meter Delay
    Combulator           combulator;         // 9
    DopplerEffect        doppler;            // 10
    LushReverb           reverb;             // 11
    FrequencyShifter     freqShifter;        // 12
    fXOneiric            oneiric;            // 13 — Dream State
    MasterModulation     modulation;         // 14
    fXOnslaught          onslaught;          // 14.5 — Wave Function Collapse
    GranularSmear        granularSmear;      // 15
    HarmonicExciter      harmonicExciter;    // 16
    StereoSculptor       stereoSculptor;     // 17
    PsychoacousticWidth  psychoWidth;        // 18
    Compressor           compressor;         // 19
    ParametricEQ         parametricEQ;       // 19.5 — 4-band surgical EQ
    BrickwallLimiter     limiter;            // 20 — Safety
    DCBlocker            dcBlocker;          // 21 — Safety
    MasterFXSequencer    sequencer;          // 22

    juce::AudioBuffer<float> dryBuffer;
    double sr = 48000.0;
    int blockSize = 512;

    // Cached APVTS raw pointers (null until prepare() called)
    // Stage 1: Saturation
    std::atomic<float>* pSatDrive    = nullptr;
    std::atomic<float>* pSatMode     = nullptr;
    // Stage 2: Corroder
    std::atomic<float>* pCorrMix     = nullptr;
    std::atomic<float>* pCorrBits    = nullptr;
    std::atomic<float>* pCorrSR      = nullptr;
    std::atomic<float>* pCorrFM      = nullptr;
    std::atomic<float>* pCorrTone    = nullptr;
    // Stage 3: Vibe Knob
    std::atomic<float>* pVibeAmount  = nullptr;
    // Stage 4: Spectral Tilt
    std::atomic<float>* pTiltAmount  = nullptr;
    std::atomic<float>* pTiltMix     = nullptr;
    // Stage 5: Transient Designer
    std::atomic<float>* pTDAttack    = nullptr;
    std::atomic<float>* pTDSustain   = nullptr;
    std::atomic<float>* pTDMix       = nullptr;
    // Stage 5.5: fXObscura
    std::atomic<float>* pObsThresh   = nullptr;
    std::atomic<float>* pObsHold     = nullptr;
    std::atomic<float>* pObsRelease  = nullptr;
    std::atomic<float>* pObsErosion  = nullptr;
    std::atomic<float>* pObsSubHarm  = nullptr;
    std::atomic<float>* pObsSat      = nullptr;
    std::atomic<float>* pObsDecimate = nullptr;
    std::atomic<float>* pObsRes      = nullptr;
    std::atomic<float>* pObsTone     = nullptr;
    std::atomic<float>* pObsPatina   = nullptr;
    std::atomic<float>* pObsMix      = nullptr;
    // Stage 5.6: fXFormant (Membrane Collection)
    std::atomic<float>* pFormantShift_ = nullptr;
    std::atomic<float>* pFormantVowel_ = nullptr;
    std::atomic<float>* pFormantQ_     = nullptr;
    std::atomic<float>* pFormantMix_   = nullptr;
    // Stage 5.7: fXBreath (Membrane Collection)
    std::atomic<float>* pBreathAmount_ = nullptr;
    std::atomic<float>* pBreathTilt_   = nullptr;
    std::atomic<float>* pBreathSens_   = nullptr;
    std::atomic<float>* pBreathMix_    = nullptr;
    // Stage 6: fXOsmosis
    std::atomic<float>* pOsmMembrane = nullptr;
    std::atomic<float>* pOsmReact    = nullptr;
    std::atomic<float>* pOsmRes      = nullptr;
    std::atomic<float>* pOsmSat      = nullptr;
    std::atomic<float>* pOsmMix      = nullptr;
    // Stage 7: Multiband OTT
    std::atomic<float>* pOttMix      = nullptr;
    std::atomic<float>* pOttDepth    = nullptr;
    // Stage 8: Delay
    std::atomic<float>* pDelayTime   = nullptr;
    std::atomic<float>* pDelayFB     = nullptr;
    std::atomic<float>* pDelayMix    = nullptr;
    std::atomic<float>* pDelayPP     = nullptr;
    std::atomic<float>* pDelayDamp   = nullptr;
    std::atomic<float>* pDelayDiff   = nullptr;
    std::atomic<float>* pDelaySync   = nullptr;
    // Stage 8.5: fXOratory
    std::atomic<float>* pOraPattern  = nullptr;
    std::atomic<float>* pOraSyllable = nullptr;
    std::atomic<float>* pOraAccent   = nullptr;
    std::atomic<float>* pOraSpread   = nullptr;
    std::atomic<float>* pOraFeedback = nullptr;
    std::atomic<float>* pOraDamping  = nullptr;
    std::atomic<float>* pOraDampRes  = nullptr;
    std::atomic<float>* pOraDrift    = nullptr;
    std::atomic<float>* pOraMix      = nullptr;
    // Stage 6: Combulator
    std::atomic<float>* pCombMix     = nullptr;
    std::atomic<float>* pCombFreq    = nullptr;
    std::atomic<float>* pCombFB      = nullptr;
    std::atomic<float>* pCombDamp    = nullptr;
    std::atomic<float>* pCombNoise   = nullptr;
    std::atomic<float>* pCombSpread  = nullptr;
    std::atomic<float>* pCombOff2    = nullptr;
    std::atomic<float>* pCombOff3    = nullptr;
    // Stage 7: Doppler
    std::atomic<float>* pDopplerDist  = nullptr;
    std::atomic<float>* pDopplerSpeed = nullptr;
    std::atomic<float>* pDopplerMix      = nullptr;
    // Stage 11: Reverb (FATHOM 8-tap Hadamard FDN)
    std::atomic<float>* pReverbSize      = nullptr;
    std::atomic<float>* pReverbMix       = nullptr;
    std::atomic<float>* pReverbPreDelay  = nullptr;
    std::atomic<float>* pReverbDecay     = nullptr;
    std::atomic<float>* pReverbDamping   = nullptr;
    std::atomic<float>* pReverbDiffusion = nullptr;
    std::atomic<float>* pReverbMod       = nullptr;
    std::atomic<float>* pReverbWidth     = nullptr;
    // Stage 9: Frequency Shifter
    std::atomic<float>* pFShiftHz    = nullptr;
    std::atomic<float>* pFShiftMix   = nullptr;
    std::atomic<float>* pFShiftMode  = nullptr;
    std::atomic<float>* pFShiftFB    = nullptr;
    // Stage 12: fXOneiric
    std::atomic<float>* pOnDelayMs   = nullptr;
    std::atomic<float>* pOnShiftHz   = nullptr;
    std::atomic<float>* pOnFeedback  = nullptr;
    std::atomic<float>* pOnDamping   = nullptr;
    std::atomic<float>* pOnSpread    = nullptr;
    std::atomic<float>* pOnMix       = nullptr;
    // Stage 13: Modulation
    std::atomic<float>* pModRate     = nullptr;
    std::atomic<float>* pModDepth    = nullptr;
    std::atomic<float>* pModMix      = nullptr;
    std::atomic<float>* pModMode     = nullptr;
    std::atomic<float>* pModFB       = nullptr;
    // Stage 14.5: fXOnslaught
    std::atomic<float>* pOnslFlowRate  = nullptr;
    std::atomic<float>* pOnslFlowDepth = nullptr;
    std::atomic<float>* pOnslThresh    = nullptr;
    std::atomic<float>* pOnslModDepth  = nullptr;
    std::atomic<float>* pOnslModRate   = nullptr;
    std::atomic<float>* pOnslDecay     = nullptr;
    std::atomic<float>* pOnslSCHP      = nullptr;
    std::atomic<float>* pOnslMix       = nullptr;
    // Stage 11: Granular Smear
    std::atomic<float>* pSmearAmt    = nullptr;
    std::atomic<float>* pSmearGrain  = nullptr;
    std::atomic<float>* pSmearDensity= nullptr;
    std::atomic<float>* pSmearMix    = nullptr;
    // Stage 12: Harmonic Exciter
    std::atomic<float>* pExcDrive    = nullptr;
    std::atomic<float>* pExcFreq     = nullptr;
    std::atomic<float>* pExcTone     = nullptr;
    std::atomic<float>* pExcMix      = nullptr;
    // Stage 13: Stereo Sculptor
    std::atomic<float>* pSculLowW    = nullptr;
    std::atomic<float>* pSculMidW    = nullptr;
    std::atomic<float>* pSculHighW   = nullptr;
    std::atomic<float>* pSculLowX    = nullptr;
    std::atomic<float>* pSculHighX   = nullptr;
    std::atomic<float>* pSculMix     = nullptr;
    // Stage 14: Psychoacoustic Width
    std::atomic<float>* pPWidthAmt   = nullptr;
    std::atomic<float>* pPWidthHaas  = nullptr;
    std::atomic<float>* pPWidthComb  = nullptr;
    std::atomic<float>* pPWidthMono  = nullptr;
    std::atomic<float>* pPWidthMix   = nullptr;
    // Stage 19: Bus Compressor
    std::atomic<float>* pCompRatio   = nullptr;
    std::atomic<float>* pCompAttack  = nullptr;
    std::atomic<float>* pCompRelease = nullptr;
    std::atomic<float>* pCompMix     = nullptr;
    // Stage 19.5: Parametric EQ
    std::atomic<float>* pEQB1Freq    = nullptr;
    std::atomic<float>* pEQB1Gain    = nullptr;
    std::atomic<float>* pEQB1Q       = nullptr;
    std::atomic<float>* pEQB2Freq    = nullptr;
    std::atomic<float>* pEQB2Gain    = nullptr;
    std::atomic<float>* pEQB2Q       = nullptr;
    std::atomic<float>* pEQB3Freq    = nullptr;
    std::atomic<float>* pEQB3Gain    = nullptr;
    std::atomic<float>* pEQB3Q       = nullptr;
    std::atomic<float>* pEQB4Freq    = nullptr;
    std::atomic<float>* pEQB4Gain    = nullptr;
    std::atomic<float>* pEQB4Q       = nullptr;
    // Stage 20: Brickwall Limiter
    std::atomic<float>* pLimCeiling  = nullptr;
    std::atomic<float>* pLimRelease  = nullptr;
    // Stage 22: Sequencer
    std::atomic<float>* pSeqEnabled  = nullptr;
    std::atomic<float>* pSeqRate     = nullptr;
    std::atomic<float>* pSeqSteps    = nullptr;
    std::atomic<float>* pSeqDepth    = nullptr;
    std::atomic<float>* pSeqSmooth   = nullptr;
    std::atomic<float>* pSeqTarget1  = nullptr;
    std::atomic<float>* pSeqTarget2  = nullptr;
    std::atomic<float>* pSeqPattern  = nullptr;
    std::atomic<float>* pSeqEnvFollow = nullptr;
    std::atomic<float>* pSeqEnvAmt   = nullptr;
};

} // namespace xolokun
