#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "OcelotParameters.h"
#include <atomic>

namespace xocelot {

// OcelotParamSnapshot — cache all parameter pointers once per block.
// All DSP code reads from this struct; nothing reads apvts directly on the audio thread.
struct OcelotParamSnapshot
{
    // ── Global ────────────────────────────────────
    int   biome            = 0;  // 0=Jungle 1=Underwater 2=Winter
    float strataBalance    = 0.5f;
    float ecosystemDepth   = 0.3f;
    float humidity         = 0.4f;
    float swing            = 0.2f;
    float density          = 0.5f;

    // ── Cross-Feed Matrix ─────────────────────────
    float xfFloorUnder     = 0.0f;
    float xfFloorCanopy    = 0.0f;
    float xfFloorEmerg     = 0.0f;
    float xfUnderFloor     = 0.0f;
    float xfUnderCanopy    = 0.0f;
    float xfUnderEmerg     = 0.0f;
    float xfCanopyFloor    = 0.0f;
    float xfCanopyUnder    = 0.0f;
    float xfCanopyEmerg    = 0.0f;
    float xfEmergFloor     = 0.0f;
    float xfEmergUnder     = 0.0f;
    float xfEmergCanopy    = 0.0f;

    // ── Floor ─────────────────────────────────────
    int   floorModel       = 3;  // kalimba
    float floorTension     = 0.5f;
    float floorStrike      = 0.5f;
    float floorDamping     = 0.4f;
    int   floorPattern     = 0;
    float floorLevel       = 0.7f;
    float floorPitch       = 0.5f;
    float floorVelocity    = 0.8f;

    // ── Understory ────────────────────────────────
    int   chopRate         = 8;
    float chopSwing        = 0.1f;
    float bitDepth         = 16.0f;
    float sampleRateRed    = 44100.0f;
    float tapeWobble       = 0.0f;
    float tapeAge          = 0.1f;
    float dustLevel        = 0.1f;
    float understoryLevel  = 0.6f;
    float understorySrc    = 0.0f;

    // ── Canopy ────────────────────────────────────
    float canopyWavefold      = 0.2f;
    int   canopyPartials      = 4;
    float canopyDetune        = 0.15f;
    float canopySpectralFilter= 0.7f;
    float canopyBreathe       = 0.3f;
    float canopyShimmer       = 0.0f;
    float canopyLevel         = 0.5f;
    float canopyPitch         = 0.5f;

    // ── Emergent ──────────────────────────────────
    int   creatureType     = 0;
    float creatureRate     = 0.3f;
    float creaturePitch    = 0.5f;
    float creatureSpread   = 0.4f;
    int   creatureTrigger  = 0;
    float creatureLevel    = 0.4f;
    float creatureAttack   = 0.3f;
    float creatureDecay    = 0.5f;

    // ── FX ────────────────────────────────────────
    float reverbSize       = 0.6f;
    float reverbMix        = 0.3f;
    float delayTime        = 0.4f;
    float delayFeedback    = 0.35f;
    float delayMix         = 0.2f;

    // ── Amp Envelope ─────────────────────────────
    float ampAttack        = 10.0f;
    float ampDecay         = 300.0f;
    float ampSustain       = 0.8f;
    float ampRelease       = 600.0f;

    // ── Macros ────────────────────────────────────
    float macroProwl       = 0.0f;
    float macroFoliage     = 0.0f;
    float macroEcosystem   = 0.0f;
    float macroCanopy      = 0.0f;

    // ── Coupling ─────────────────────────────────
    float couplingLevel    = 0.0f;
    int   couplingBus      = 0;

    // Update all fields from APVTS — call once per processBlock, before any DSP.
    // NOTE: ParamIDs constants have the same names as struct fields, so we must
    // use explicit xocelot::ParamIDs:: qualification to avoid member shadowing.
    void updateFrom(const juce::AudioProcessorValueTreeState& apvts)
    {
        namespace P = xocelot::ParamIDs;

        auto g = [&](const char* id) -> float {
            auto* p = apvts.getRawParameterValue(id);
            return p ? p->load() : 0.0f;
        };
        auto gi = [&](const char* id) -> int {
            return static_cast<int>(g(id));
        };

        biome           = gi(P::biome);
        strataBalance   = g(P::strataBalance);
        ecosystemDepth  = g(P::ecosystemDepth);
        humidity        = g(P::humidity);
        swing           = g(P::swing);
        density         = g(P::density);

        xfFloorUnder    = g(P::xfFloorUnder);
        xfFloorCanopy   = g(P::xfFloorCanopy);
        xfFloorEmerg    = g(P::xfFloorEmerg);
        xfUnderFloor    = g(P::xfUnderFloor);
        xfUnderCanopy   = g(P::xfUnderCanopy);
        xfUnderEmerg    = g(P::xfUnderEmerg);
        xfCanopyFloor   = g(P::xfCanopyFloor);
        xfCanopyUnder   = g(P::xfCanopyUnder);
        xfCanopyEmerg   = g(P::xfCanopyEmerg);
        xfEmergFloor    = g(P::xfEmergFloor);
        xfEmergUnder    = g(P::xfEmergUnder);
        xfEmergCanopy   = g(P::xfEmergCanopy);

        this->floorModel    = gi(P::floorModel);
        floorTension        = g(P::floorTension);
        floorStrike         = g(P::floorStrike);
        floorDamping        = g(P::floorDamping);
        floorPattern        = gi(P::floorPattern);
        floorLevel          = g(P::floorLevel);
        floorPitch          = g(P::floorPitch);
        floorVelocity       = g(P::floorVelocity);

        chopRate            = gi(P::chopRate);
        chopSwing           = g(P::chopSwing);
        bitDepth            = g(P::bitDepth);
        sampleRateRed       = g(P::sampleRateRed);
        tapeWobble          = g(P::tapeWobble);
        tapeAge             = g(P::tapeAge);
        dustLevel           = g(P::dustLevel);
        understoryLevel     = g(P::understoryLevel);
        understorySrc       = g(P::understorySrc);

        canopyWavefold       = g(P::canopyWavefold);
        this->canopyPartials = gi(P::canopyPartials);
        canopyDetune         = g(P::canopyDetune);
        canopySpectralFilter = g(P::canopySpectralFilter);
        canopyBreathe        = g(P::canopyBreathe);
        canopyShimmer        = g(P::canopyShimmer);
        canopyLevel          = g(P::canopyLevel);
        canopyPitch          = g(P::canopyPitch);

        this->creatureType    = gi(P::creatureType);
        creatureRate          = g(P::creatureRate);
        creaturePitch         = g(P::creaturePitch);
        creatureSpread        = g(P::creatureSpread);
        this->creatureTrigger = gi(P::creatureTrigger);
        creatureLevel         = g(P::creatureLevel);
        creatureAttack        = g(P::creatureAttack);
        creatureDecay         = g(P::creatureDecay);

        reverbSize      = g(P::reverbSize);
        reverbMix       = g(P::reverbMix);
        delayTime       = g(P::delayTime);
        delayFeedback   = g(P::delayFeedback);
        delayMix        = g(P::delayMix);

        ampAttack       = g(P::ampAttack);
        ampDecay        = g(P::ampDecay);
        ampSustain      = g(P::ampSustain);
        ampRelease      = g(P::ampRelease);

        macroProwl      = g(P::macroProwl);
        macroFoliage    = g(P::macroFoliage);
        macroEcosystem  = g(P::macroEcosystem);
        macroCanopy     = g(P::macroCanopy);

        couplingLevel     = g(P::couplingLevel);
        this->couplingBus = gi(P::couplingBus);

        // D004 fix: apply macro modulations to DSP parameters after raw load.
        // Each macro maps 0-1 to an audible offset on its target parameters.
        //
        // PROWL (hunting movement) → filter cutoff modulation via ecosystemDepth + density.
        //   Higher PROWL = deeper filter sweep + increased density (more active, hunting).
        if (macroProwl > 0.001f)
        {
            ecosystemDepth = std::clamp(ecosystemDepth + macroProwl * 0.5f,  0.0f, 1.0f);
            density        = std::clamp(density        + macroProwl * 0.4f,  0.0f, 1.0f);
        }

        // FOLIAGE (environment depth) → reverb density via reverbSize + reverbMix.
        //   Higher FOLIAGE = larger, lusher reverb space (denser canopy overhead).
        if (macroFoliage > 0.001f)
        {
            reverbSize = std::clamp(reverbSize + macroFoliage * 0.4f, 0.0f, 1.0f);
            reverbMix  = std::clamp(reverbMix  + macroFoliage * 0.3f, 0.0f, 1.0f);
        }

        // ECOSYSTEM (cross-voice interaction depth) → xfFloorCanopy + xfCanopyFloor + xfUnderEmerg.
        //   Higher ECOSYSTEM = all strata interact more (the whole ecosystem comes alive).
        if (macroEcosystem > 0.001f)
        {
            xfFloorCanopy = std::clamp(xfFloorCanopy + macroEcosystem * 0.5f, -1.0f, 1.0f);
            xfCanopyFloor = std::clamp(xfCanopyFloor + macroEcosystem * 0.3f, -1.0f, 1.0f);
            xfUnderEmerg  = std::clamp(xfUnderEmerg  + macroEcosystem * 0.4f, -1.0f, 1.0f);
        }

        // CANOPY (high-frequency content / brightness) → canopyLevel + canopyShimmer + canopySpectralFilter.
        //   Higher CANOPY = brighter, airier top layer (more light through the canopy).
        if (macroCanopy > 0.001f)
        {
            canopyLevel          = std::clamp(canopyLevel          + macroCanopy * 0.4f, 0.0f, 1.0f);
            canopyShimmer        = std::clamp(canopyShimmer        + macroCanopy * 0.5f, 0.0f, 1.0f);
            canopySpectralFilter = std::clamp(canopySpectralFilter + macroCanopy * 0.3f, 0.0f, 1.0f);
        }
    }
};

} // namespace xocelot
