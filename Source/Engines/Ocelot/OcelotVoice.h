#pragma once

#include <array>
#include <algorithm>
#include <cmath>
#include "../../DSP/FastMath.h"
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
        noteNumber    = -1;
        active        = false;
        lastAmplitude = 0.0f;
        stealFadeGain = 0.0f;

        // Compute sample-rate-aware steal fade rate (5ms ramp at any sample rate)
        stealFadeRate_ = 1.0f / (0.005f * static_cast<float> (sampleRate));

        // D005 fix: ecosystem drift LFO — ecological population dynamics
        ecosystemDriftPhase = 0.0f;
    }

    void noteOn(int note, float vel, const OcelotParamSnapshot& snap)
    {
        // Voice-stealing crossfade: if this voice was active when stolen by the
        // VoicePool, preserve the outgoing amplitude and ramp it to 0 over 5ms
        // in renderBlock. The new note's attack ramps up independently, so the
        // two signals blend briefly rather than producing a hard click.
        stealFadeGain = active ? lastAmplitude : 0.0f;

        noteNumber    = note;
        velocity      = vel;   // D001: cache for filter env computation
        active        = true;
        lastAmplitude = 0.0f;

        floor.noteOn(note, vel, snap);
        canopy.noteOn(note, vel);
        emergent.noteOn(note, vel);
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

        // ── 2. Ecosystem drift LFO (D005 fix: autonomous predator-prey cycle at 0.07 Hz)
        // Slowly modulates ecosystemDepth to simulate population dynamics.
        // Rate: ~14-second cycle. Depth: ±20% of current ecosystemDepth.
        // Silent at ecosystemDepth=0 (no matrix active = no drift either).
        ecosystemDriftPhase += static_cast<float>(numSamples) * 0.07f
                               / static_cast<float>(sr);
        if (ecosystemDriftPhase > 1.0f) ecosystemDriftPhase -= 1.0f;
        // SRO: fastSin replaces std::sin (per-block LFO)
        float driftSine = xoceanus::fastSin(ecosystemDriftPhase * juce::MathConstants<float>::twoPi);
        float lfoEcosystemDepth = std::clamp(
            snap.ecosystemDepth + snap.ecosystemDepth * 0.20f * driftSine, 0.0f, 1.0f);

        // ── 3. Cross-feed matrix (one block lag — inaudible, avoids per-sample cost)
        StrataModulation mod = ecosystemMatrix.process(lastStrataSignals, snap,
                                                       lfoEcosystemDepth);

        // ── 4. Update amp envelope parameters ─────────────────────────
        ampEnv.setParameters(snap.ampAttack, snap.ampDecay, snap.ampSustain, snap.ampRelease);

        // D001: filter envelope — inject velocity × ampEnvLevel boost into canopy
        // spectral filter position via mod.canopyFilterMod.
        // The canopy uses spectralPos = clamp(canopySpectralFilter + canopyFilterMod, 0,1),
        // so adding here raises the spectral LP cutoff on harder/fresher notes.
        // kOcelotFilterEnvScale = 0.3: at depth=1, full vel, full envLevel → +0.3 spectralPos,
        // equivalent to roughly +2500 Hz shift at mid position (spectralPos ~0.7).
        {
            static constexpr float kOcelotFilterEnvScale = 0.3f;
            mod.canopyFilterMod += snap.filterEnvDepth * velocity * ampEnv.getLevel() * kOcelotFilterEnvScale;
        }

        // ── 5. Render each stratum into private temp buffers ──────────
        floor    .renderBlock(floorL.data(),   floorR.data(),   numSamples, snap, biome, mod);
        understory.renderBlock(underL.data(),  underR.data(),   numSamples, snap, biome, mod);
        canopy   .renderBlock(canopyL.data(),  canopyR.data(),  numSamples, snap, biome, mod);
        emergent .renderBlock(emergentL.data(),emergentR.data(),numSamples, snap, biome, mod);

        // ── 6. Update StrataSignals for next block ─────────────────────
        lastStrataSignals.floorAmplitude    = floor.getLastAmplitude();
        // Real spectral proxy: model brightness base + tension boost - damping reduction.
        // Model brightness (0-5): berimbau=0.5, cuica=0.55, agogo=0.78, kalimba=0.42, pandeiro=0.72, log_drum=0.28
        // Tension lifts brightness (stiffer membrane = more high partials); damping reduces it.
        {
            static constexpr float kModelBrightness[6] = { 0.50f, 0.55f, 0.78f, 0.42f, 0.72f, 0.28f };
            int m = std::clamp(snap.floorModel, 0, 5);
            float baseTimbre = kModelBrightness[m];
            float timbreWithMods = baseTimbre
                                 + snap.floorTension * 0.25f   // tighter membrane = brighter
                                 - snap.floorDamping * 0.18f;  // more damping = darker
            lastStrataSignals.floorTimbre = std::clamp(timbreWithMods, 0.0f, 1.0f);
        }
        lastStrataSignals.understoryEnergy  = understory.getLastEnergy();
        lastStrataSignals.understoryPitch   = understory.getLastPitch();
        lastStrataSignals.canopyAmplitude   = canopy.getLastAmplitude();
        lastStrataSignals.canopySpectral    = canopy.getLastSpectral();
        lastStrataSignals.emergentAmplitude = emergent.getLastAmplitude();
        lastStrataSignals.emergentPattern   = emergent.getLastPattern();

        // ── 7. Mix strata + humidity + amp envelope ───────────────────
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
                // SRO: fastTanh replaces std::tanh (per-sample saturation)
                l = xoceanus::fastTanh(l * drive) / drive;
                r = xoceanus::fastTanh(r * drive) / drive;
            }

            // Amp envelope (per-sample)
            float envGain = ampEnv.process();

            // Voice-stealing crossfade: ramp the outgoing signal to 0 over 5ms.
            // stealFadeGain > 0 only during the first ~220 samples after a steal.
            // We subtract from 1.0 so that as stealFadeGain falls from its initial
            // value toward 0, the effective multiplier rises from (1 - initial) to 1.0,
            // smoothly blending out the outgoing voice level.
            if (stealFadeGain > 0.0f)
            {
                stealFadeGain -= stealFadeRate_;
                if (stealFadeGain < 0.0f) stealFadeGain = 0.0f;
                envGain *= (1.0f - stealFadeGain);
            }

            outL[i] += l * envGain;
            outR[i] += r * envGain;
            sumSq   += l * l;
        }

        // SRO: fast sqrt via fastPow2/fastLog2 (per-block RMS)
        float rmsArg = sumSq / static_cast<float>(numSamples);
        lastAmplitude = (rmsArg > 1e-10f)
            ? xoceanus::fastPow2(0.5f * xoceanus::fastLog2(rmsArg))
            : 0.0f;

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
    double sr           = 0.0;
    bool   active       = false;
    int    noteNumber   = -1;
    float  velocity     = 0.0f;    // D001: stored at noteOn for filter env computation
    float  lastAmplitude= 0.0f;

    // Voice-stealing crossfade state. Set to lastAmplitude in noteOn() when the
    // voice is being reused (stolen). Decremented per-sample in renderBlock over
    // a 5ms ramp, then stays at 0. Prevents hard-cut clicks on voice-full polyphony.
    float  stealFadeGain = 0.0f;
    float  stealFadeRate_ = 1.0f / (0.005f * static_cast<float> (sr));  // overwritten by prepare()

    // D005 fix: autonomous ecosystem drift LFO — predator-prey population cycle
    // Modulates ecosystemDepth at 0.07 Hz (~14 sec cycle). Requires no parameter.
    float ecosystemDriftPhase = 0.0f;
};

} // namespace xocelot
