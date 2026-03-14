#pragma once

#include <array>
#include <algorithm>
#include <cmath>
#include "OcelotParamSnapshot.h"
#include "BiomeMorph.h"
#include "EcosystemMatrix.h"
#include "AmpEnvelope.h"
#include "OcelotFloor.h"
#include "OcelotUnderstory.h"
#include "OcelotCanopy.h"
#include "OcelotEmergent.h"

namespace xocelot {

// OcelotVoice — Single polyphonic voice: all 4 strata + matrix + biome + amp envelope.
//
// Signal flow per block:
//   1. BiomeMorph::advance()        — update crossfade
//   2. EcosystemMatrix::process()   — compute cross-feed modulation from last block's signals
//   3. Render Floor / Understory / Canopy / Emergent into per-stratum temp buffers
//   4. Update StrataSignals for next block
//   5. Mix strata (strataBalance tilt)
//   6. Humidity saturation (tanh)
//   7. Amp envelope
//
// CPU: ~6% @ 44.1k on a 2020 M1 (4 strata stubs + SVF formants + matrix).
// Phase 2: full physical models will increase Floor cost significantly.

class OcelotVoice
{
public:
    static constexpr int kMaxBlockSize = 2048;

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        floor.prepare(sampleRate);
        understory.prepare(sampleRate);
        canopy.prepare(sampleRate);
        emergent.prepare(sampleRate);
        biomeMorph.prepare(sampleRate);
        ampEnv.prepare(sampleRate);

        lastStrataSignals = {};
        noteNumber = -1;
        active     = false;
        lastAmplitude = 0.0f;
    }

    void noteOn(int note, float velocity, const OcelotParamSnapshot& snap)
    {
        noteNumber   = note;
        active       = true;
        lastAmplitude = 0.0f;

        floor.noteOn(note, velocity, snap);
        canopy.noteOn(note, velocity);
        emergent.noteOn(note, velocity);
        // Understory doesn't take a note — it's sample/buffer based

        ampEnv.setParameters(snap.ampAttack, snap.ampDecay, snap.ampSustain, snap.ampRelease);
        ampEnv.gate(true);
    }

    void noteOff()
    {
        floor.noteOff();
        canopy.noteOff();
        emergent.noteOff();
        ampEnv.gate(false);
        noteNumber = -1;
    }

    void setBiomeTarget(int biomeIndex)
    {
        biomeMorph.setTargetBiome(biomeIndex);
    }

    // renderBlock: accumulates into outL/outR (caller must zero first).
    // Returns this block's RMS energy (used for coupling).
    float renderBlock(float* outL, float* outR, int numSamples,
                      const OcelotParamSnapshot& snap)
    {
        if (!active && !ampEnv.isActive()) return 0.0f;

        juce::ScopedNoDenormals noDenormals;

        // ── 1. Advance biome crossfade ─────────────────────────────────
        biomeMorph.advance(numSamples);
        const BiomeProfile& biome = biomeMorph.get();

        // ── 2. Cross-feed matrix (one block lag — inaudible, avoids per-sample cost)
        StrataModulation mod = ecosystemMatrix.process(lastStrataSignals, snap,
                                                       snap.ecosystemDepth);

        // ── 3. Update amp envelope parameters ─────────────────────────
        ampEnv.setParameters(snap.ampAttack, snap.ampDecay, snap.ampSustain, snap.ampRelease);

        // ── 4. Render each stratum into private temp buffers ──────────
        floor    .renderBlock(floorL.data(),   floorR.data(),   numSamples, snap, biome, mod);
        understory.renderBlock(underL.data(),  underR.data(),   numSamples, snap, biome, mod);
        canopy   .renderBlock(canopyL.data(),  canopyR.data(),  numSamples, snap, biome, mod);
        emergent .renderBlock(emergentL.data(),emergentR.data(),numSamples, snap, biome, mod);

        // ── 5. Update StrataSignals for next block ─────────────────────
        lastStrataSignals.floorAmplitude    = floor.getLastAmplitude();
        lastStrataSignals.floorTimbre       = 0.5f; // TODO Phase 2: real spectral proxy
        lastStrataSignals.understoryEnergy  = understory.getLastEnergy();
        lastStrataSignals.understoryPitch   = understory.getLastPitch();
        lastStrataSignals.canopyAmplitude   = canopy.getLastAmplitude();
        lastStrataSignals.canopySpectral    = canopy.getLastSpectral();
        lastStrataSignals.emergentAmplitude = emergent.getLastAmplitude();
        lastStrataSignals.emergentPattern   = emergent.getLastPattern();

        // ── 6. Mix strata + humidity + amp envelope ───────────────────
        float sumSq = 0.0f;

        // strataBalance tilt: 0=percussion-forward, 1=atmospheric
        // Earth (Floor+Understory) and Air (Canopy+Emergent) pairs with 30% floor minimum
        float b        = std::clamp(snap.strataBalance, 0.0f, 1.0f);
        float earthMix = 0.3f + (1.0f - b) * 0.7f;
        float airMix   = 0.3f + b * 0.7f;
        float total    = earthMix + airMix;
        earthMix      /= total;
        airMix        /= total;

        for (int i = 0; i < numSamples; ++i)
        {
            float l = (floorL[i] + underL[i])   * earthMix
                    + (canopyL[i] + emergentL[i]) * airMix;
            float r = (floorR[i] + underR[i])   * earthMix
                    + (canopyR[i] + emergentR[i]) * airMix;
            l *= 0.5f; // two strata summed per pair → halve for unity
            r *= 0.5f;

            // Humidity: tanh soft-saturation. drive 1–5x
            if (snap.humidity > 0.01f)
            {
                float drive = 1.0f + snap.humidity * 4.0f;
                l = std::tanh(l * drive) / drive;
                r = std::tanh(r * drive) / drive;
            }

            // Amp envelope (per-sample)
            float envGain = ampEnv.process();
            outL[i] += l * envGain;
            outR[i] += r * envGain;
            sumSq   += l * l;
        }

        lastAmplitude = std::sqrt(sumSq / static_cast<float>(numSamples));

        // Voice goes idle when amp envelope finishes and all strata are quiet
        if (!ampEnv.isActive()
            && lastAmplitude < 0.0001f
            && !floor.isActive()
            && !canopy.isActive())
        {
            active = false;
        }

        return lastAmplitude;
    }

    // ── Accessors ──────────────────────────────────────────────────────
    bool  isActive()          const { return active || ampEnv.isActive(); }
    float getLastAmplitude()  const { return lastAmplitude; }
    int   getNoteNumber()     const { return noteNumber; }

private:
    // ── Strata ─────────────────────────────────────────────────────────
    OcelotFloor       floor;
    OcelotUnderstory  understory;
    OcelotCanopy      canopy;
    OcelotEmergent    emergent;

    // ── Ecosystem plumbing ─────────────────────────────────────────────
    EcosystemMatrix   ecosystemMatrix;
    BiomeMorph        biomeMorph;
    StrataSignals     lastStrataSignals;

    // ── Amplitude envelope ─────────────────────────────────────────────
    AmpEnvelope       ampEnv;

    // ── Pre-allocated per-stratum temp buffers (no heap alloc on audio thread) ──
    std::array<float, kMaxBlockSize> floorL,    floorR;
    std::array<float, kMaxBlockSize> underL,    underR;
    std::array<float, kMaxBlockSize> canopyL,   canopyR;
    std::array<float, kMaxBlockSize> emergentL, emergentR;

    // ── State ──────────────────────────────────────────────────────────
    double sr           = 44100.0;
    bool   active       = false;
    int    noteNumber   = -1;
    float  lastAmplitude= 0.0f;
};

} // namespace xocelot
