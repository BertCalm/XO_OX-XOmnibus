#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "OwlfishParameters.h"
#include <atomic>

namespace xowlfish {

// OwlfishParamSnapshot -- cache all parameter pointers once per block.
// All DSP code reads from this struct; nothing reads apvts directly on the audio thread.
//
// Usage:
//   1. Call attachTo(apvts) once after construction (in prepareToPlay or constructor)
//   2. Call updateFrom() at the top of each processBlock, before any DSP
struct OwlfishParamSnapshot
{
    // -- Solitary Genus --
    float portamento   = 0.1f;
    int   legatoMode   = 1;     // 1 = Legato (default)
    float morphGlide   = 0.5f;

    // -- Abyss Habitat --
    float subMix       = 0.5f;
    int   subDiv1      = 1;     // 1:2
    int   subDiv2      = 2;     // 1:3
    int   subDiv3      = 0;     // Off
    int   subDiv4      = 0;     // Off
    float subLevel1    = 0.8f;
    float subLevel2    = 0.6f;
    float subLevel3    = 0.4f;
    float subLevel4    = 0.3f;
    float mixtur       = 0.5f;
    float fundWave     = 0.0f;
    float subWave      = 0.0f;
    float bodyFreq     = 40.0f;
    float bodyLevel    = 0.3f;

    // -- Owl Optics --
    float compRatio     = 0.4f;
    float compThreshold = 0.5f;
    float compAttack    = 0.2f;
    float compRelease   = 0.4f;
    float filterCutoff  = 0.6f;
    float filterReso    = 0.3f;
    float filterTrack   = 0.5f;

    // -- Diet --
    float grainSize    = 0.3f;
    float grainDensity = 0.5f;
    float grainPitch   = 0.5f;
    float grainMix     = 0.0f;
    float feedRate     = 0.3f;

    // -- Sacrificial Armor --
    float armorThreshold = 0.5f;
    float armorDecay     = 0.4f;
    float armorScatter   = 0.2f;
    float armorDuck      = 0.3f;
    float armorDelay     = 0.2f;

    // -- Abyss Reverb --
    float reverbSize     = 0.6f;
    float reverbDamp     = 0.4f;
    float reverbPreDelay = 0.1f;
    float reverbMix      = 0.2f;

    // -- Amp Envelope --
    float ampAttack    = 10.0f;
    float ampDecay     = 300.0f;
    float ampSustain   = 0.8f;
    float ampRelease   = 600.0f;

    // -- Macros --
    float macroDepth    = 0.0f;
    float macroFeeding  = 0.0f;
    float macroDefense  = 0.0f;
    float macroPressure = 0.0f;

    // -- Output --
    float outputLevel  = 0.8f;
    float outputPan    = 0.0f;

    // -- Coupling --
    float couplingLevel = 0.0f;
    int   couplingBus   = 0;

    // Cache all std::atomic<float>* pointers from the APVTS.
    // Call once after APVTS construction (e.g., in prepareToPlay).
    void attachTo(juce::AudioProcessorValueTreeState& apvts)
    {
        namespace P = xowlfish::ParamIDs;

        pPortamento     = apvts.getRawParameterValue(P::portamento);
        pLegatoMode     = apvts.getRawParameterValue(P::legatoMode);
        pMorphGlide     = apvts.getRawParameterValue(P::morphGlide);

        pSubMix         = apvts.getRawParameterValue(P::subMix);
        pSubDiv1        = apvts.getRawParameterValue(P::subDiv1);
        pSubDiv2        = apvts.getRawParameterValue(P::subDiv2);
        pSubDiv3        = apvts.getRawParameterValue(P::subDiv3);
        pSubDiv4        = apvts.getRawParameterValue(P::subDiv4);
        pSubLevel1      = apvts.getRawParameterValue(P::subLevel1);
        pSubLevel2      = apvts.getRawParameterValue(P::subLevel2);
        pSubLevel3      = apvts.getRawParameterValue(P::subLevel3);
        pSubLevel4      = apvts.getRawParameterValue(P::subLevel4);
        pMixtur         = apvts.getRawParameterValue(P::mixtur);
        pFundWave       = apvts.getRawParameterValue(P::fundWave);
        pSubWave        = apvts.getRawParameterValue(P::subWave);
        pBodyFreq       = apvts.getRawParameterValue(P::bodyFreq);
        pBodyLevel      = apvts.getRawParameterValue(P::bodyLevel);

        pCompRatio      = apvts.getRawParameterValue(P::compRatio);
        pCompThreshold  = apvts.getRawParameterValue(P::compThreshold);
        pCompAttack     = apvts.getRawParameterValue(P::compAttack);
        pCompRelease    = apvts.getRawParameterValue(P::compRelease);
        pFilterCutoff   = apvts.getRawParameterValue(P::filterCutoff);
        pFilterReso     = apvts.getRawParameterValue(P::filterReso);
        pFilterTrack    = apvts.getRawParameterValue(P::filterTrack);

        pGrainSize      = apvts.getRawParameterValue(P::grainSize);
        pGrainDensity   = apvts.getRawParameterValue(P::grainDensity);
        pGrainPitch     = apvts.getRawParameterValue(P::grainPitch);
        pGrainMix       = apvts.getRawParameterValue(P::grainMix);
        pFeedRate       = apvts.getRawParameterValue(P::feedRate);

        pArmorThreshold = apvts.getRawParameterValue(P::armorThreshold);
        pArmorDecay     = apvts.getRawParameterValue(P::armorDecay);
        pArmorScatter   = apvts.getRawParameterValue(P::armorScatter);
        pArmorDuck      = apvts.getRawParameterValue(P::armorDuck);
        pArmorDelay     = apvts.getRawParameterValue(P::armorDelay);

        pReverbSize     = apvts.getRawParameterValue(P::reverbSize);
        pReverbDamp     = apvts.getRawParameterValue(P::reverbDamp);
        pReverbPreDelay = apvts.getRawParameterValue(P::reverbPreDelay);
        pReverbMix      = apvts.getRawParameterValue(P::reverbMix);

        pAmpAttack      = apvts.getRawParameterValue(P::ampAttack);
        pAmpDecay       = apvts.getRawParameterValue(P::ampDecay);
        pAmpSustain     = apvts.getRawParameterValue(P::ampSustain);
        pAmpRelease     = apvts.getRawParameterValue(P::ampRelease);

        pMacroDepth     = apvts.getRawParameterValue(P::macroDepth);
        pMacroFeeding   = apvts.getRawParameterValue(P::macroFeeding);
        pMacroDefense   = apvts.getRawParameterValue(P::macroDefense);
        pMacroPressure  = apvts.getRawParameterValue(P::macroPressure);

        pOutputLevel    = apvts.getRawParameterValue(P::outputLevel);
        pOutputPan      = apvts.getRawParameterValue(P::outputPan);

        pCouplingLevel  = apvts.getRawParameterValue(P::couplingLevel);
        pCouplingBus    = apvts.getRawParameterValue(P::couplingBus);
    }

    // Read all cached pointers into plain members -- call once per processBlock.
    void updateFrom()
    {
        portamento     = pPortamento->load();
        legatoMode     = static_cast<int>(pLegatoMode->load());
        morphGlide     = pMorphGlide->load();

        subMix         = pSubMix->load();
        subDiv1        = static_cast<int>(pSubDiv1->load());
        subDiv2        = static_cast<int>(pSubDiv2->load());
        subDiv3        = static_cast<int>(pSubDiv3->load());
        subDiv4        = static_cast<int>(pSubDiv4->load());
        subLevel1      = pSubLevel1->load();
        subLevel2      = pSubLevel2->load();
        subLevel3      = pSubLevel3->load();
        subLevel4      = pSubLevel4->load();
        mixtur         = pMixtur->load();
        fundWave       = pFundWave->load();
        subWave        = pSubWave->load();
        bodyFreq       = pBodyFreq->load();
        bodyLevel      = pBodyLevel->load();

        compRatio      = pCompRatio->load();
        compThreshold  = pCompThreshold->load();
        compAttack     = pCompAttack->load();
        compRelease    = pCompRelease->load();
        filterCutoff   = pFilterCutoff->load();
        filterReso     = pFilterReso->load();
        filterTrack    = pFilterTrack->load();

        grainSize      = pGrainSize->load();
        grainDensity   = pGrainDensity->load();
        grainPitch     = pGrainPitch->load();
        grainMix       = pGrainMix->load();
        feedRate       = pFeedRate->load();

        armorThreshold = pArmorThreshold->load();
        armorDecay     = pArmorDecay->load();
        armorScatter   = pArmorScatter->load();
        armorDuck      = pArmorDuck->load();
        armorDelay     = pArmorDelay->load();

        reverbSize     = pReverbSize->load();
        reverbDamp     = pReverbDamp->load();
        reverbPreDelay = pReverbPreDelay->load();
        reverbMix      = pReverbMix->load();

        ampAttack      = pAmpAttack->load();
        ampDecay       = pAmpDecay->load();
        ampSustain     = pAmpSustain->load();
        ampRelease     = pAmpRelease->load();

        macroDepth     = pMacroDepth->load();
        macroFeeding   = pMacroFeeding->load();
        macroDefense   = pMacroDefense->load();
        macroPressure  = pMacroPressure->load();

        outputLevel    = pOutputLevel->load();
        outputPan      = pOutputPan->load();

        couplingLevel  = pCouplingLevel->load();
        couplingBus    = static_cast<int>(pCouplingBus->load());
    }

private:
    // -- Cached atomic pointers (set once by attachTo) --
    std::atomic<float>* pPortamento     = nullptr;
    std::atomic<float>* pLegatoMode     = nullptr;
    std::atomic<float>* pMorphGlide     = nullptr;

    std::atomic<float>* pSubMix         = nullptr;
    std::atomic<float>* pSubDiv1        = nullptr;
    std::atomic<float>* pSubDiv2        = nullptr;
    std::atomic<float>* pSubDiv3        = nullptr;
    std::atomic<float>* pSubDiv4        = nullptr;
    std::atomic<float>* pSubLevel1      = nullptr;
    std::atomic<float>* pSubLevel2      = nullptr;
    std::atomic<float>* pSubLevel3      = nullptr;
    std::atomic<float>* pSubLevel4      = nullptr;
    std::atomic<float>* pMixtur         = nullptr;
    std::atomic<float>* pFundWave       = nullptr;
    std::atomic<float>* pSubWave        = nullptr;
    std::atomic<float>* pBodyFreq       = nullptr;
    std::atomic<float>* pBodyLevel      = nullptr;

    std::atomic<float>* pCompRatio      = nullptr;
    std::atomic<float>* pCompThreshold  = nullptr;
    std::atomic<float>* pCompAttack     = nullptr;
    std::atomic<float>* pCompRelease    = nullptr;
    std::atomic<float>* pFilterCutoff   = nullptr;
    std::atomic<float>* pFilterReso     = nullptr;
    std::atomic<float>* pFilterTrack    = nullptr;

    std::atomic<float>* pGrainSize      = nullptr;
    std::atomic<float>* pGrainDensity   = nullptr;
    std::atomic<float>* pGrainPitch     = nullptr;
    std::atomic<float>* pGrainMix       = nullptr;
    std::atomic<float>* pFeedRate       = nullptr;

    std::atomic<float>* pArmorThreshold = nullptr;
    std::atomic<float>* pArmorDecay     = nullptr;
    std::atomic<float>* pArmorScatter   = nullptr;
    std::atomic<float>* pArmorDuck      = nullptr;
    std::atomic<float>* pArmorDelay     = nullptr;

    std::atomic<float>* pReverbSize     = nullptr;
    std::atomic<float>* pReverbDamp     = nullptr;
    std::atomic<float>* pReverbPreDelay = nullptr;
    std::atomic<float>* pReverbMix      = nullptr;

    std::atomic<float>* pAmpAttack      = nullptr;
    std::atomic<float>* pAmpDecay       = nullptr;
    std::atomic<float>* pAmpSustain     = nullptr;
    std::atomic<float>* pAmpRelease     = nullptr;

    std::atomic<float>* pMacroDepth     = nullptr;
    std::atomic<float>* pMacroFeeding   = nullptr;
    std::atomic<float>* pMacroDefense   = nullptr;
    std::atomic<float>* pMacroPressure  = nullptr;

    std::atomic<float>* pOutputLevel    = nullptr;
    std::atomic<float>* pOutputPan      = nullptr;

    std::atomic<float>* pCouplingLevel  = nullptr;
    std::atomic<float>* pCouplingBus    = nullptr;
};

} // namespace xowlfish
