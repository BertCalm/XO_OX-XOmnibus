// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OxidizeVoice.h — XOxidize | Per-voice DSP processor
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  One instance per polyphonic voice (max 8). Owns all per-voice state:
//  age accumulator, patina+basic oscillators, corrosion waveshaper,
//  erosion filter, entropy quantizer, wobble LFOs, dropout gate, VCA envelope.
//
//  Signal chain (per sample):
//    [Patina Osc] + [Basic Osc]
//         └──→ [Corrosion Waveshaper]
//                    └──→ sedimentSend tap 1  (post-corrosion: early character)
//                    └──→ [Erosion Filter]
//                               └──→ [Entropy Quantizer]
//                                        └──→ sedimentSend tap 2  (post-entropy: degraded texture)
//                                        └──→ [Wobble]  (pitch mod — applied via LFO into phase inc)
//                                                 └──→ [Dropout Gate]
//                                                          └──→ sedimentSend tap 3  (post-dropout: rhythmic imprint)
//                                                          └──→ [VCA Envelope]
//                                                                    └──→ outputCacheL/R
//
//  Age accumulator + AgeDerivedState derivation: block-rate (once per block).
//  All per-sample paths: allocation-free, denormal-safe (flushDenormal on all
//  feedback state), ParamSnapshot pattern (reads OxidizeParamSnapshot, not raw
//  APVTS).
//
//==============================================================================

#include "OxidizeLookupTables.h"  // xoceanus::oxidize::LookupTables, lookup(), ageCurveFunction()
#include "OxidizeCorrosion.h"     // xoceanus::oxidize::processCorrosion(), CorrosionMode
#include "OxidizeParamSnapshot.h" // xooxidize::OxidizeParamSnapshot
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
// AgeDerivedState — all age-dependent parameters derived once per block.
// Lives in the voice (not in LookupTables) so it can hold derived floats
// without complicating the shared LUT struct.
//==============================================================================
struct AgeDerivedState
{
    float corrosionDrive     = 0.0f; // waveshaper intensity
    float erosionCutoff      = 20000.0f; // Hz — filter cutoff
    float erosionRes         = 0.0f; // filter resonance [0,1]
    float entropyBits        = 16.0f; // effective bit depth
    float entropyRate        = 44100.0f; // effective sample rate (for S&H)
    float analogNoiseLevel   = 0.0f; // noise floor rise [0,1] (pre-scaled)
    float wobbleDepth        = 0.0f; // wow+flutter intensity multiplier
    float dropoutProbability = 0.0f; // per-block dropout probability
    float patinaLevel        = 0.0f; // noise oscillator mix [0,1]
    float sedimentSend       = 0.0f; // reverb send master amount
};

//==============================================================================
// OxidizeVoice — per-voice DSP state + render methods.
//
// Lifecycle:
//   1. prepare(sampleRate, maxBlockSize)           — init / SR change
//   2. noteOn(note, velocity, snapshotAge, ...)    — MIDI note-on
//   3. updateAge(dt, snap, luts, couplingBoost)    — block-rate, before render
//   4. renderSamples(outL, outR, ...)              — per block, adds into buffers
//   5. noteOff()                                   — MIDI note-off
//   6. isActive() → false                          — voice can be stolen
//==============================================================================
struct OxidizeVoice
{
    //==========================================================================
    // Identity & LRU stealing
    //==========================================================================
    int      note      = -1;
    float    velocity  = 0.0f;
    bool     active    = false;
    uint64_t startTime = 0;   // sample-clock at note-on (LRU voice stealing)

    // Voice-steal crossfade gain: 0→1 ramp over ~5ms when a voice is stolen.
    // New note starts at 0.0f and increments each sample at stealFadeRate_
    // to prevent a click from abrupt DSP state reset on the stolen voice.
    float stealFadeGain = 1.0f;

    //==========================================================================
    // Age state (Schulze mandate: fully independent per voice)
    //==========================================================================
    float noteAge      = 0.0f;  // 0.0 = pristine, 1.0 = fully oxidized
    float snapshotAge  = 0.0f;  // preset-stored initial age
    bool  reverseMode  = false; // Buchla: start at preserveAmount, decay toward 0

    //==========================================================================
    // Oscillator state
    //==========================================================================
    float    oscPhase    = 0.0f;   // basic oscillator phase [0, 1)
    float    patinaPhase = 0.0f;   // noise tilt filter state (IIR)
    uint32_t noiseState  = 12345u; // Knuth TAOCP LCG for crackle/BrokenSpeaker noise

    //==========================================================================
    // Erosion filter (CytomicSVF)
    // Two filters needed for Tape mode (LP − BP subtraction)
    //==========================================================================
    CytomicSVF erosionFilter;
    CytomicSVF erosionBpFilter; // Tape mode: BandPass for mid-scoop

    //==========================================================================
    // Entropy
    //==========================================================================
    float digitalHoldL   = 0.0f; // sample-and-hold value (left)
    float digitalHoldR   = 0.0f; // sample-and-hold value (right)
    int   digitalCounter = 0;    // counts samples between holds
    float entroIIR       = 0.0f; // IIR state for entropy smooth pass

    //==========================================================================
    // Wobble (2 × 2 LFOs — independent L/R for stereo spread)
    //==========================================================================
    StandardLFO wowLFO_L;
    StandardLFO wowLFO_R;
    StandardLFO flutterLFO_L;
    StandardLFO flutterLFO_R;

    //==========================================================================
    // General-purpose LFOs (wired to oxidize_lfo1*/lfo2* params — issue #835)
    //==========================================================================
    StandardLFO lfo1_;
    StandardLFO lfo2_;

    //==========================================================================
    // Dropout gate
    //==========================================================================
    float    dropoutEnv  = 1.0f;   // 1.0 = fully open, dropoutFloor = silenced
    bool     inDropout   = false;  // true while fading out/in
    bool     recovering  = false;  // true while recovering (fade back up)
    uint32_t dropoutPRNG = 98765u; // PRNG for timing decisions

    //==========================================================================
    // VCA envelope
    //==========================================================================
    StandardADSR ampEnvelope;

    //==========================================================================
    // Coupling output cache
    //==========================================================================
    float lastSampleL = 0.0f;
    float lastSampleR = 0.0f;

    //==========================================================================
    // Block-rate derived age state (computed in updateAge, used in renderSamples)
    //==========================================================================
    AgeDerivedState ageDerived;

    //==========================================================================
    // Corrosion tilt EQ state (one-pole IIR)
    //==========================================================================
    float corrosionTiltState_ = 0.0f;

    //==========================================================================
    // Private: sample rate + steal crossfade rate
    //==========================================================================
    float sampleRate_    = 44100.0f;
    float stealFadeRate_ = 1.0f / (0.005f * 44100.0f); // 5ms linear ramp

    //==========================================================================
    // isActive — false once envelope completes and voice is recyclable
    //==========================================================================
    bool isActive() const noexcept { return active; }

    //==========================================================================
    // randomFloat — LCG helper using voice noiseState (advances the PRNG).
    //   Returns a value in [0.0, 1.0)
    //==========================================================================
    static inline float randomFloat(uint32_t& state) noexcept
    {
        state = state * 1664525u + 1013904223u;  // LCG (Knuth TAOCP)
        return static_cast<float>(state >> 8) / 16777216.0f;  // 0.0 to 1.0
    }

    //==========================================================================
    // getLastSampleL/R — coupling output cache accessor
    //==========================================================================
    float getLastSampleL() const noexcept { return lastSampleL; }
    float getLastSampleR() const noexcept { return lastSampleR; }

    //==========================================================================
    // prepare — call once on init or sample-rate change
    //==========================================================================
    void prepare(double sampleRate, int /*maxBlockSize*/) noexcept
    {
        sampleRate_    = static_cast<float>(sampleRate);
        stealFadeRate_ = 1.0f / (0.005f * sampleRate_); // 5ms linear ramp
        ampEnvelope.prepare(sampleRate_);

        wowLFO_L.setRate(0.5f, sampleRate_);
        wowLFO_L.setShape(StandardLFO::Sine);
        wowLFO_R.setRate(0.5f, sampleRate_);
        wowLFO_R.setShape(StandardLFO::Sine);
        flutterLFO_L.setRate(12.0f, sampleRate_);
        flutterLFO_L.setShape(StandardLFO::Sine);
        flutterLFO_R.setRate(12.0f, sampleRate_);
        flutterLFO_R.setShape(StandardLFO::Sine);

        lfo1_.setRate(0.3f, sampleRate_);
        lfo1_.setShape(StandardLFO::Sine);
        lfo2_.setRate(1.5f, sampleRate_);
        lfo2_.setShape(StandardLFO::Sine);

        resetState();
    }

    //==========================================================================
    // resetState — zeroes all DSP state (used internally + on voice steal)
    //==========================================================================
    void resetState() noexcept
    {
        oscPhase       = 0.0f;
        patinaPhase    = 0.0f;
        noiseState     = 12345u;
        dropoutPRNG    = 98765u;
        dropoutEnv     = 1.0f;
        inDropout      = false;
        recovering     = false;
        digitalCounter = 0;
        digitalHoldL   = 0.0f;
        digitalHoldR   = 0.0f;
        entroIIR       = 0.0f;
        lastSampleL    = 0.0f;
        lastSampleR    = 0.0f;
        stealFadeGain  = 1.0f;  // full gain by default; engine sets 0.0f on steal

        erosionFilter       = CytomicSVF{};
        erosionBpFilter     = CytomicSVF{};
        ageDerived          = AgeDerivedState{};
        corrosionTiltState_ = 0.0f;
    }

    //==========================================================================
    // noteOn — triggered by MIDI note-on
    //   midiNote       : 0-127
    //   vel            : normalized [0, 1]
    //   snapshotAgeIn  : preset initial age (Smith design)
    //   reverseModeIn  : Buchla reverse-age flag
    //   snap           : current param snapshot (read ageVelSens from here)
    //   clock          : sample-clock tick (for LRU stealing)
    //==========================================================================
    void noteOn(int midiNote, float vel, float snapshotAgeIn, bool reverseModeIn,
                const xooxidize::OxidizeParamSnapshot& snap, uint64_t clock) noexcept
    {
        note      = midiNote;
        velocity  = vel;
        active    = true;
        startTime = clock;

        snapshotAge = snapshotAgeIn;
        reverseMode = reverseModeIn;

        // Initial age = preset age + velocity-mapped offset
        float velOffset = vel * snap.ageVelSens * 0.4f;
        noteAge = std::clamp(snapshotAge + velOffset, 0.0f, 1.0f);

        if (reverseMode)
            noteAge = snap.preserveAmount; // start fully oxidized, decay toward pristine

        // VCA envelope — no dedicated A/D/S/R params in spec, use fixed musical defaults
        // scaled by velocity (D001: velocity shapes timbre via amplitude floor)
        float ampScale = 0.3f + 0.7f * vel;
        ampEnvelope.setParams(0.005f,              // 5ms attack
                               0.15f,              // 150ms decay
                               0.75f * ampScale,   // sustain level
                               0.4f,               // 400ms release
                               sampleRate_);
        ampEnvelope.noteOn();

        // Configure wobble LFOs from snapshot
        wowLFO_L.setRate(snap.wowRate, sampleRate_);
        wowLFO_R.setRate(snap.wowRate, sampleRate_);
        flutterLFO_L.setRate(snap.flutterRate, sampleRate_);
        flutterLFO_R.setRate(snap.flutterRate, sampleRate_);

        // Stagger L/R phases by wobbleSpread + per-note offset for ensemble width
        float notePhaseOffset = static_cast<float>(midiNote % 8) / 8.0f * 0.25f;
        wowLFO_L.setPhaseOffset(0.0f);
        wowLFO_R.setPhaseOffset(snap.wobbleSpread * 0.5f + notePhaseOffset);
        flutterLFO_L.setPhaseOffset(notePhaseOffset * 0.5f);
        flutterLFO_R.setPhaseOffset(snap.wobbleSpread * 0.25f + notePhaseOffset * 0.75f);

        // Reset general-purpose LFOs, randomize phase for ensemble width
        lfo1_.reset(static_cast<float>(midiNote % 16) / 16.0f);
        lfo2_.reset(static_cast<float>((midiNote + 7) % 16) / 16.0f);

        // Reset dropout
        dropoutEnv = 1.0f;
        inDropout  = false;
        recovering = false;

        // Reset entropy sample-hold counter
        digitalCounter = 0;

        // Reset corrosion tilt IIR state to avoid transient glitch at note start
        corrosionTiltState_ = 0.0f;
    }

    //==========================================================================
    // noteOff — releases the VCA envelope
    //==========================================================================
    void noteOff() noexcept
    {
        ampEnvelope.noteOff();
    }

    //==========================================================================
    // updateAge — block-rate age accumulation + AgeDerivedState derivation.
    //   Call ONCE per block before renderSamples().
    //
    //   dt              : block duration in seconds (blockSize / sampleRate)
    //   snap            : current parameter snapshot
    //   luts            : shared pre-computed LUT struct
    //   couplingAgeBoost: extra age rate from coupling RMS (computed in engine)
    //==========================================================================
    void updateAge(float dt,
                   const xooxidize::OxidizeParamSnapshot& snap,
                   const oxidize::LookupTables& luts,
                   float couplingAgeBoost) noexcept
    {
        if (!active)
            return;

        // ── Age rate ─────────────────────────────────────────────────────────
        // ageRate=1.0 → full oxidation in ~30s (normalized: /30 per second)
        float rate = snap.ageRate * (1.0f / 30.0f);

        // Aftertouch + velSens → fight entropy (Vangelis: performer resists decay)
        float fightBack = snap.aftertouch * snap.velSens * 0.8f;
        rate = std::max(0.0f, rate - fightBack);

        // Mod wheel boosts age rate (mapped as additional aging speed)
        rate += snap.modWheel * snap.ageRate * (1.0f / 60.0f);

        // Coupling-driven aging (louder source = faster oxidation)
        rate += couplingAgeBoost;

        // ── Age curve warping ─────────────────────────────────────────────────
        // ageCurveFunction: curve=-1 → log (fast early), 0=linear, +1=exp (fast late)
        float curveMult = oxidize::ageCurveFunction(noteAge, snap.ageCurve);
        float delta     = dt * rate * curveMult;

        if (reverseMode)
            noteAge = std::max(0.0f, noteAge - delta);
        else
            noteAge = std::min(noteAge + delta, snap.preserveAmount);

        // ── Derive all age-dependent values from LUTs ─────────────────────────
        // Normalize age to [0,1] relative to preserveAmount ceiling
        float normalAge = (snap.preserveAmount > 0.001f)
                          ? noteAge / snap.preserveAmount
                          : 0.0f;
        normalAge = std::clamp(normalAge, 0.0f, 1.0f);

        using oxidize::lookup;

        ageDerived.corrosionDrive     = snap.corrosionDepth * lookup(luts.corrosion, normalAge);
        ageDerived.erosionCutoff      = 20000.0f * (1.0f - snap.erosionRate * lookup(luts.erosion, normalAge))
                                        + snap.erosionFloor * snap.erosionRate * lookup(luts.erosion, normalAge);
        // Simpler linear interp that respects erosionFloor param:
        //   cutoff = lerp(20000, erosionFloor, erosionRate * erosionLUT)
        {
            float erosionAmt = snap.erosionRate * lookup(luts.erosion, normalAge);
            ageDerived.erosionCutoff = 20000.0f + erosionAmt * (snap.erosionFloor - 20000.0f);
        }
        ageDerived.erosionRes         = snap.erosionRes * normalAge;
        ageDerived.entropyBits        = 16.0f - 14.0f * snap.entropyDepth * lookup(luts.entropy, normalAge);
        ageDerived.entropyBits        = std::max(2.0f, ageDerived.entropyBits);
        ageDerived.entropyRate        = sampleRate_ - (sampleRate_ - 4000.0f)
                                        * snap.entropyDepth * lookup(luts.entropy, normalAge);
        ageDerived.entropyRate        = std::max(4000.0f, ageDerived.entropyRate);
        ageDerived.analogNoiseLevel   = snap.entropyDepth * 0.3f * lookup(luts.entropy, normalAge);
        ageDerived.wobbleDepth        = lookup(luts.wobble, normalAge);
        ageDerived.dropoutProbability = snap.dropoutRate * lookup(luts.dropout, normalAge);
        ageDerived.patinaLevel        = snap.patinaDensity * lookup(luts.patina, normalAge);
        ageDerived.sedimentSend       = snap.sedimentMix * (0.3f + 0.7f * lookup(luts.sediment, normalAge));
    }

    //==========================================================================
    // renderSamples — main per-sample DSP loop.
    //
    //   outL/R          : add-in output buffers (voice accumulates, not replaces)
    //   numSamples      : block size
    //   snap            : current parameter snapshot
    //   luts            : shared LUT struct (read-only)
    //   sedimentSendL/R : add-in send buffers for shared sediment reverb FDN
    //                     Voice writes 3 tap-weighted contributions per sample
    //==========================================================================
    void renderSamples(float* outL, float* outR, int numSamples,
                       const xooxidize::OxidizeParamSnapshot& snap,
                       const oxidize::LookupTables& /*luts*/,
                       float* sedimentSendL, float* sedimentSendR,
                       float  hostBPM      = 0.0f,
                       double hostBeatPos  = 0.0,
                       bool   hostIsPlaying = false) noexcept
    {
        if (!active)
            return;

        // ── Frequency ────────────────────────────────────────────────────────
        // MIDI note + semitone tune + cent fine + pitch bend → Hz
        // pitchBend in snap is the bend RANGE; actual bend position is in snap
        // as a normalized [-1,1] value times pitchBend semitones.
        // (Engine multiplies: snap.pitchBend = bendRange * bendPosition)
        float semiOffset  = snap.tune + snap.fine * 0.01f + snap.pitchBend;
        constexpr float kLn2Over12 = 0.05776226504f; // ln(2)/12
        float noteHz      = 440.0f * fastExp((static_cast<float>(note) - 69.0f + semiOffset)
                                              * kLn2Over12);
        float basePhaseInc = noteHz / sampleRate_;

        // ── Erosion filter coefficients (block-rate update) ──────────────────
        float erosionCutoff = ageDerived.erosionCutoff;
        float erosionRes    = ageDerived.erosionRes;

        // erosionVariance: add random offset to cutoff (block-rate jitter)
        {
            float cutoffJitter = (snap.erosionVariance > 0.001f)
                ? (randomFloat(noiseState) * 2.0f - 1.0f) * snap.erosionVariance * 500.0f
                : 0.0f;
            erosionCutoff = std::max(20.0f, erosionCutoff + cutoffJitter);
        }

        // Erosion filter mode: set once per block (mode changes reset SVF state).
        // Coefficients are updated per-sample inside the loop to apply LFO1 modulation.
        switch (snap.erosionMode)
        {
        case 0: // Vinyl — smooth LP roll-off, gentle resonance
            erosionFilter.setMode(CytomicSVF::Mode::LowPass);
            break;
        case 1: // Tape — LP base + BandPass subtraction (mid-scoop)
            erosionFilter.setMode(CytomicSVF::Mode::LowPass);
            erosionBpFilter.setMode(CytomicSVF::Mode::BandPass);
            break;
        case 2: // Failure — notch with high resonance → unstable collapse
            erosionFilter.setMode(CytomicSVF::Mode::Notch);
            break;
        default:
            erosionFilter.setMode(CytomicSVF::Mode::LowPass);
            break;
        }

        // ── Dropout: block-rate probability decision ─────────────────────────
        // P2-01: Tempo-sync mode (Kakehashi seance recommendation).
        //
        // dropoutSync == 0 (Free): original per-block random draw — unchanged.
        // dropoutSync == 1/2/3:    Only roll the dice when the host beat position
        //   crosses a grid boundary (1/8th, 1/16th, or 1/32nd note respectively).
        //   This means dropout events are quantized to the tempo grid — they can
        //   only START on a beat subdivision, creating a rhythmic rupture texture
        //   rather than a random scatter.  Once a dropout begins, it completes
        //   normally via the fade envelope regardless of sync mode.
        //
        // Fallback to free mode when: host is not playing, BPM is unknown (<=0),
        //   or dropoutSync == 0.  This preserves full backward compatibility.
        //
        // Grid boundary detection: compare the beat-grid phase at the START of
        //   this block vs the END.  If the phase wraps around 0 (i.e., start > end),
        //   a grid boundary fell within this block.
        //
        // beatDivision: beats per grid subdivision (quarter note = 1 beat PPQ).
        //   1/8th  note = 0.5 beats, 1/16th = 0.25 beats, 1/32nd = 0.125 beats.
        if (!inDropout && !recovering)
        {
            // Determine whether this block is allowed to roll for a new dropout.
            bool allowDraw = true; // free mode: always allow

            if (snap.dropoutSync > 0 && hostIsPlaying && hostBPM > 0.0f)
            {
                // Division sizes in quarter-note (PPQ) beats
                // mode 1 = 1/8th  → 0.5 PPQ per subdivision
                // mode 2 = 1/16th → 0.25 PPQ per subdivision
                // mode 3 = 1/32nd → 0.125 PPQ per subdivision
                double divisionBeats;
                switch (snap.dropoutSync)
                {
                case 1:  divisionBeats = 0.5;   break; // 1/8th note
                case 2:  divisionBeats = 0.25;  break; // 1/16th note
                case 3:  divisionBeats = 0.125; break; // 1/32nd note
                default: divisionBeats = 0.5;   break;
                }

                // Compute beat position at START and END of this block.
                // beatsPerSample = BPM / 60 / sampleRate
                const double beatsPerSample = static_cast<double>(hostBPM) / 60.0
                                              / static_cast<double>(sampleRate_);
                const double beatStart = hostBeatPos;
                const double beatEnd   = hostBeatPos + beatsPerSample * static_cast<double>(numSamples);

                // Phase within the current subdivision cycle at block boundaries.
                // A grid boundary is crossed when floor(beatEnd/div) > floor(beatStart/div).
                const double phaseStart = std::floor(beatStart / divisionBeats);
                const double phaseEnd   = std::floor(beatEnd   / divisionBeats);
                allowDraw = (phaseEnd > phaseStart);
            }

            if (allowDraw)
            {
                dropoutPRNG = dropoutPRNG * 1664525u + 1013904223u;
                float rng01 = static_cast<float>(dropoutPRNG & 0xFFFF) / 65536.0f;

                // dropoutVariance adds ±25% jitter to threshold
                uint32_t jitterBits = (dropoutPRNG >> 16) & 0xFF;
                float jitter = 1.0f + (static_cast<float>(jitterBits) / 255.0f - 0.5f)
                               * snap.dropoutVariance;
                float threshold = ageDerived.dropoutProbability * jitter;

                if (rng01 < threshold)
                    inDropout = true;
            }
        }

        // Dropout envelope fade rates — smear=0 → instant (1 sample), smear=1 → 200ms
        float smearSamples = std::max(1.0f, snap.dropoutSmear * 0.2f * sampleRate_);
        float dropFadeRate   = 1.0f / smearSamples;
        float dropRecoverRate = 1.0f / (smearSamples * 2.0f); // recover takes 2× longer
        float dropFloor      = 1.0f - snap.dropoutDepth;

        // ── Digital entropy: hold period (block-rate) ─────────────────────────
        // entropyVariance: jitter the hold period length
        float holdJitter = (snap.entropyVariance > 0.001f)
            ? randomFloat(noiseState) * snap.entropyVariance * 0.5f
            : 0.0f;
        int holdPeriod = static_cast<int>(sampleRate_ / ageDerived.entropyRate * (1.0f + holdJitter));
        holdPeriod = std::max(1, holdPeriod);

        // Bit depth quantization step size
        float levels = fastExp(ageDerived.entropyBits * kLn2Over12 * 12.0f); // 2^bits
        levels = std::max(4.0f, levels);
        float invLevels = 1.0f / levels;

        // Entropy smooth IIR coefficient
        float smoothAlpha = snap.entropySmooth * 0.6f;

        // ── Corrosion mode cast ───────────────────────────────────────────────
        auto corrMode = static_cast<oxidize::CorrosionMode>(
            std::clamp(snap.corrosionMode, 0, 5));

        // Rust mode asymmetry uses normalAge as the rust parameter
        float normalAge = (snap.preserveAmount > 0.001f)
                          ? noteAge / snap.preserveAmount : 0.0f;
        normalAge = std::clamp(normalAge, 0.0f, 1.0f);

        // ── General-purpose LFOs — block-rate rate update ────────────────────
        lfo1_.setRate(snap.lfo1Rate, sampleRate_);
        lfo2_.setRate(snap.lfo2Rate, sampleRate_);

        // ── Per-sample loop ───────────────────────────────────────────────────
        for (int i = 0; i < numSamples; ++i)
        {
            // ── Wobble LFOs — advance all four ───────────────────────────────
            float wowL    = wowLFO_L.process();
            float wowR    = wowLFO_R.process();
            float flutL   = flutterLFO_L.process();
            float flutR   = flutterLFO_R.process();

            // ── General-purpose LFOs — process per-sample (issue #835) ────────
            float lfo1Val = lfo1_.process(); // [-1, +1]
            float lfo2Val = lfo2_.process(); // [-1, +1]

            // LFO1 → erosion cutoff modulation (builds on top of block-rate jitter)
            float erosionCutoffModded = std::max(20.0f,
                                            erosionCutoff * (1.0f + lfo1Val * snap.lfo1Depth * 0.5f));

            // Update erosion filter coefficients per-sample to track LFO1
            switch (snap.erosionMode)
            {
            case 0:
                erosionFilter.setCoefficients(erosionCutoffModded,
                                              0.1f + 0.2f * erosionRes,
                                              sampleRate_);
                break;
            case 1:
                erosionFilter.setCoefficients(erosionCutoffModded, 0.1f, sampleRate_);
                erosionBpFilter.setCoefficients(std::min(erosionCutoffModded * 2.0f, sampleRate_ * 0.45f),
                                                0.4f + 0.5f * erosionRes,
                                                sampleRate_);
                break;
            case 2:
                erosionFilter.setCoefficients(erosionCutoffModded,
                                              0.45f + 0.5f * erosionRes,
                                              sampleRate_);
                break;
            default:
                erosionFilter.setCoefficients(erosionCutoffModded, 0.1f, sampleRate_);
                break;
            }

            // LFO2 → corrosion drive modulation
            float corrosionDriveModded = ageDerived.corrosionDrive
                                         * (1.0f + lfo2Val * snap.lfo2Depth * 0.3f);

            float wd = ageDerived.wobbleDepth;
            // Convert cents deviation to phase increment multiplier.
            // Max wow ~25 cents, max flutter ~10 cents (age-gated via wobbleDepth).
            float pitchModL = 1.0f + (wowL * snap.wowDepth * wd * 0.01449f
                                     + flutL * snap.flutterDepth * wd * 0.00578f);
            float pitchModR = 1.0f + (wowR * snap.wowDepth * wd * 0.01449f
                                     + flutR * snap.flutterDepth * wd * 0.00578f);

            // ── Basic oscillator — phase driven by average L/R increment ─────
            float avgPhaseInc = basePhaseInc * (pitchModL + pitchModR) * 0.5f;
            oscPhase += avgPhaseInc;
            if (oscPhase >= 1.0f) oscPhase -= 1.0f;

            float oscOut = 0.0f;
            switch (snap.waveform)
            {
            case 0: // Saw with lightweight PolyBLEP correction at phase wrap
            {
                oscOut = 2.0f * oscPhase - 1.0f;
                if (oscPhase < avgPhaseInc && avgPhaseInc > 1e-10f)
                {
                    float t = oscPhase / avgPhaseInc;
                    oscOut -= (t * t - 2.0f * t + 1.0f) * 0.5f; // BLEP correction
                }
                break;
            }
            case 1: // Pulse (50% duty cycle)
                oscOut = (oscPhase < 0.5f) ? 1.0f : -1.0f;
                break;
            case 2: // Triangle
                oscOut = 4.0f * std::fabs(oscPhase - 0.5f) - 1.0f;
                break;
            case 3: // White noise
                noiseState = noiseState * 1664525u + 1013904223u;
                oscOut = static_cast<float>(static_cast<int32_t>(noiseState)) * (1.0f / 2147483648.0f);
                break;
            case 4: // Hybrid: saw + noise blend driven by age
            {
                oscOut = 2.0f * oscPhase - 1.0f;
                noiseState = noiseState * 22695477u + 1u;
                float nz = static_cast<float>(static_cast<int32_t>(noiseState)) * (1.0f / 2147483648.0f);
                oscOut += nz * ageDerived.patinaLevel * 0.4f;
                break;
            }
            default:
                oscOut = 2.0f * oscPhase - 1.0f;
                break;
            }

            // ── Patina oscillator — PRNG crackle filtered by patinaTone ──────
            noiseState = noiseState * 1664525u + 1013904223u;
            float rawCrackle = static_cast<float>(static_cast<int32_t>(noiseState))
                               * (1.0f / 2147483648.0f);
            // patinaTone: 0 = low rumble (low-pass integrate), 1 = high crackle (pass-through)
            // IIR tilt filter: alpha=patinaTone blends integrated vs raw
            patinaPhase = flushDenormal(patinaPhase + snap.patinaTone * (rawCrackle - patinaPhase));
            float patinaOut = patinaPhase * ageDerived.patinaLevel;

            // ── Mix oscillators ───────────────────────────────────────────────
            float mixed = oscOut * snap.oscMix + patinaOut * (1.0f - snap.oscMix);

            // ── Corrosion waveshaper ───────────────────────────────────────────
            // corrosionVariance: per-sample drive jitter applied on top of LFO2 modulation
            float driveJitter = (snap.corrosionVariance > 0.001f)
                ? (randomFloat(noiseState) * 2.0f - 1.0f) * snap.corrosionVariance * 0.2f
                : 0.0f;
            float jitteredDrive = corrosionDriveModded * (1.0f + driveJitter);

            // BrokenSpeaker noise: derive fresh per-sample from noiseState
            float bsNoise = static_cast<float>((noiseState >> 16) & 0xFFFF) / 65536.0f;
            float corroded = oxidize::processCorrosion(mixed,
                                                       jitteredDrive,
                                                       corrMode,
                                                       normalAge,   // rust param for Rust mode
                                                       bsNoise);    // noise for BrokenSpeaker

            // Post-corrosion tilt EQ (corrosionTone: 0=dark, 0.5=flat, 1=bright)
            {
                float tiltCoeff = snap.corrosionTone * 2.0f - 1.0f; // -1 to +1
                // Simple tilt: blend between LP and HP character
                // LP: state += coeff * (input - state); output = state
                // HP: output = input - state
                float tiltAlpha = 0.05f + 0.1f * std::abs(tiltCoeff);
                corrosionTiltState_ = corrosionTiltState_ + tiltAlpha * (corroded - corrosionTiltState_);
                corrosionTiltState_ = flushDenormal(corrosionTiltState_);
                float tilted = (tiltCoeff >= 0.0f)
                    ? corroded + tiltCoeff * (corroded - corrosionTiltState_)  // bright: emphasize HF
                    : corrosionTiltState_ + (1.0f + tiltCoeff) * (corroded - corrosionTiltState_);  // dark: emphasize LF
                corroded = tilted;
            }

            // Sediment tap 1 — post-corrosion character
            float sed1 = ageDerived.sedimentSend * 0.4f;
            sedimentSendL[i] += corroded * sed1;
            sedimentSendR[i] += corroded * sed1; // mono corrosion feeds both; engine decorrelates

            // ── Erosion filter ────────────────────────────────────────────────
            float erosL, erosR;
            if (snap.erosionMode == 1)
            {
                // Tape: LP with mid-scoop via BandPass subtraction
                float lp  = erosionFilter.processSample(corroded);
                float bp  = erosionBpFilter.processSample(corroded);
                float scoop = 0.25f + 0.5f * erosionRes;
                erosL = lp - bp * scoop;
                erosR = erosL; // identical at this stage; wobble already separated in phase inc
            }
            else
            {
                erosL = erosionFilter.processSample(corroded);
                erosR = erosL;
            }
            erosL = flushDenormal(erosL);
            erosR = flushDenormal(erosR);

            // ── Entropy quantizer ─────────────────────────────────────────────
            float entroL = erosL;
            float entroR = erosR;

            // Digital: sample-and-hold + bit depth reduction
            if (snap.entropyMode == 0 || snap.entropyMode == 2)
            {
                if (digitalCounter <= 0)
                {
                    digitalHoldL = std::round(erosL * levels) * invLevels;
                    digitalHoldR = std::round(erosR * levels) * invLevels;
                    digitalCounter = holdPeriod;
                }
                --digitalCounter;

                float digitalMix = (snap.entropyMode == 2)
                                   ? std::clamp(0.5f + (snap.entropyBias - 0.5f) * 0.8f, 0.0f, 1.0f)
                                   : 1.0f;
                entroL = digitalHoldL * digitalMix + erosL * (1.0f - digitalMix);
                entroR = digitalHoldR * digitalMix + erosR * (1.0f - digitalMix);
            }

            // Analog: additive noise floor rises with age
            if (snap.entropyMode == 1 || snap.entropyMode == 2)
            {
                noiseState = noiseState * 1664525u + 1013904223u;
                float aNoise = static_cast<float>(static_cast<int32_t>(noiseState))
                               * (1.0f / 2147483648.0f)
                               * ageDerived.analogNoiseLevel;
                float analogMix = (snap.entropyMode == 2)
                                  ? std::clamp(1.0f - snap.entropyBias * 0.8f, 0.0f, 1.0f)
                                  : 1.0f;
                entroL += aNoise * analogMix;
                entroR += aNoise * analogMix;
            }

            // Entropy smooth — 1-pole IIR anti-alias after quantization
            entroIIR = flushDenormal(entroIIR + smoothAlpha * (entroL - entroIIR));
            entroL   = entroL * (1.0f - smoothAlpha) + entroIIR * smoothAlpha;
            entroR   = entroR * (1.0f - smoothAlpha) + entroIIR * smoothAlpha;

            // Sediment tap 2 — post-entropy degraded texture
            float sed2 = ageDerived.sedimentSend * 0.35f;
            sedimentSendL[i] += entroL * sed2;
            sedimentSendR[i] += entroR * sed2;

            // ── Dropout gate ───────────────────────────────────────────────────
            if (inDropout)
            {
                dropoutEnv -= dropFadeRate;
                if (dropoutEnv <= dropFloor)
                {
                    dropoutEnv = dropFloor;
                    inDropout  = false;
                    recovering = true;
                }
            }
            else if (recovering)
            {
                dropoutEnv += dropRecoverRate;
                if (dropoutEnv >= 1.0f)
                {
                    dropoutEnv = 1.0f;
                    recovering = false;
                }
            }
            dropoutEnv = flushDenormal(dropoutEnv);

            float dropL = entroL * dropoutEnv;
            float dropR = entroR * dropoutEnv;

            // Sediment tap 3 — post-dropout rhythmic silence imprint
            float sed3 = ageDerived.sedimentSend * 0.25f;
            sedimentSendL[i] += dropL * sed3;
            sedimentSendR[i] += dropR * sed3;

            // ── VCA envelope ───────────────────────────────────────────────────
            float envLevel   = ampEnvelope.process();
            float outSampleL = dropL * envLevel;
            float outSampleR = dropR * envLevel;

            // ── Voice-steal crossfade: ramp stealFadeGain 0→1 over 5ms ─────────
            // Prevents a click when resetState() zeros filter/DSP state on steal.
            // stealFadeGain is set to 0.0f by the engine on a stolen voice;
            // non-stolen notes start at 1.0f and this is a no-op.
            if (stealFadeGain < 1.0f)
            {
                outSampleL *= stealFadeGain;
                outSampleR *= stealFadeGain;
                stealFadeGain = std::min(1.0f, stealFadeGain + stealFadeRate_);
            }

            outL[i] += outSampleL;
            outR[i] += outSampleR;

            lastSampleL = outSampleL;
            lastSampleR = outSampleR;
        }

        // ── Deactivate when envelope reaches idle ─────────────────────────────
        if (!ampEnvelope.isActive())
        {
            active      = false;
            lastSampleL = 0.0f;
            lastSampleR = 0.0f;
        }
    }
};

} // namespace xoceanus
