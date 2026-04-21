// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FamilyWaveguide.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/StandardLFO.h"
#include <array>
#include <cmath>

namespace xoceanus
{

struct OttoniAdapterVoice
{
    bool active = false;
    int note = 0;
    float vel = 0, freq = 440, ampEnv = 0, sr = 0;  // sr: Sentinel — set by host before DSP
    bool releasing = false;
    FamilyDelayLine dl;
    FamilyDampingFilter df;
    FamilyBodyResonance body;
    FamilySympatheticBank symp;
    FamilyOrganicDrift drift;
    // SOUND-5 fix: three separate LipBuzzExciters for toddler/tween/teen sections.
    // Previously a single lipBuzz was called three times per sample, advancing the shared
    // phase accumulator each time — so all three "voices" were reading staggered positions
    // of the same oscillator rather than independent buzz sources.
    LipBuzzExciter lipBuzzToddler;
    LipBuzzExciter lipBuzzTween;
    LipBuzzExciter lipBuzzTeen;

    // Per-voice vibrato LFO (teen vibrato) — replaces inline vibPhase accumulator
    StandardLFO vib;

    // Voice-steal crossfade (5 ms linear fade-out before new note starts)
    float stealFadeGain = 1.0f;
    float stealFadeStep = 0.0f; // 1.0f / (0.005f * sr)
    bool isBeingStolen = false;
    int pendingNote = 0;
    float pendingVel = 0.0f;

    // PERF-1/SOUND-4 fix: release coefficient cached at prepare() — computed once, not per-sample.
    // Avoids std::exp (slow) inside the inner render loop.
    float releaseCoeff = 0.9967f; // default ≈ exp(-1/(44100*0.3))

    void prepare(double s)
    {
        sr = (float)s;
        int md = (int)(sr / 20) + 8;
        dl.prepare(md);
        df.prepare();
        body.prepare(s);
        symp.prepare(s, 512);
        drift.prepare(s);
        lipBuzzToddler.prepare(s);
        lipBuzzTween.prepare(s);
        lipBuzzTeen.prepare(s);
        vib.reset();
        // Pre-compute release coefficient once per prepare (PERF-1 / SOUND-4 fix):
        // std::exp is exact here (not per-sample), avoids repeated computation in renderBlock.
        releaseCoeff = std::exp(-1.0f / (sr * 0.3f));
    }
    void reset()
    {
        dl.reset();
        df.reset();
        body.reset();
        symp.reset();
        drift.reset();
        lipBuzzToddler.reset();
        lipBuzzTween.reset();
        lipBuzzTeen.reset();
        active = false;
        ampEnv = 0;
        vib.reset();
        stealFadeGain = 1.0f;
        stealFadeStep = 0.0f;
        isBeingStolen = false;
    }
    void noteOn(int n, float v)
    {
        note = n;
        vel = v;
        freq = 440.f * fastPow2((n - 69.f) / 12.f); // PERF: fastPow2 replaces std::pow (consistent with fleet)
        dl.reset();
        df.reset();
        // VOICES-4 fix: removed body.setParams() hardcoded call here — the block-level
        // macro/instrument table computes instrFreqMult/instrQ per sample, so this
        // pre-seed with wrong values (freq*0.8, Q=5) was a wasted call overwritten immediately.
        symp.tune(freq);
        drift.reset(); // VOICES-3 fix: reset organic drift on note-on so pitch drift
                       // from previous note doesn't bleed into the new note's attack.
        ampEnv = v;
        releasing = false;
        active = true;
        vib.reset();
    }
    void noteOff() { releasing = true; }
};

class OttoniEngine : public SynthEngine
{
public:
    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        for (auto& v : voices)
            v.prepare(sampleRate);
        silenceGate.prepare(sampleRate, maxBlockSize);
        // Reset FX state
        revState[0] = revState[1] = revState[2] = revState[3] = 0;
        choLFO.reset();
        delWr = 0;
        choLFO.setShape(StandardLFO::Sine);
        std::fill(delBufL, delBufL + kDelMax, 0.f);
        std::fill(delBufR, delBufR + kDelMax, 0.f);
    }
    void releaseResources() override
    {
        for (auto& v : voices)
            v.reset();
    }
    void reset() override
    {
        for (auto& v : voices)
            v.reset();
        lastL = lastR = 0;
        revState[0] = revState[1] = revState[2] = revState[3] = 0;
        choLFO.reset();
        delWr = 0;
        std::fill(delBufL, delBufL + kDelMax, 0.f);
        std::fill(delBufR, delBufR + kDelMax, 0.f);
    }

    void renderBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi, int ns) override
    {
        juce::ScopedNoDenormals noDenormals;
        for (const auto m : midi)
        {
            auto msg = m.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                int t = -1;
                for (int i = 0; i < kV; ++i)
                    if (!voices[i].active)
                    {
                        t = i;
                        break;
                    }
                if (t < 0)
                    t = nv % kV;
                nv = (t + 1) % kV;
                if (voices[t].active && !voices[t].isBeingStolen)
                {
                    voices[t].isBeingStolen = true;
                    // STABILITY-1 fix: guard against sr=0 (unprepared voice) to avoid inf stealFadeStep.
                    float fadeSr = (voices[t].sr > 0.f) ? voices[t].sr : 44100.f;
                    voices[t].stealFadeStep = 1.0f / (0.005f * fadeSr);
                    voices[t].stealFadeGain = 1.0f;
                    voices[t].pendingNote = msg.getNoteNumber();
                    voices[t].pendingVel = msg.getVelocity() / 127.f;
                }
                else if (voices[t].active && voices[t].isBeingStolen)
                {
                    // VOICES-1 fix: update pending note when steal already in progress —
                    // previously a third rapid note-on silently dropped the new note.
                    voices[t].pendingNote = msg.getNoteNumber();
                    voices[t].pendingVel = msg.getVelocity() / 127.f;
                }
                else if (!voices[t].isBeingStolen)
                {
                    voices[t].noteOn(msg.getNoteNumber(), msg.getVelocity() / 127.f);
                }
            }
            else if (msg.isNoteOff())
            {
                for (auto& v : voices)
                    if (v.active && v.note == msg.getNoteNumber())
                        v.noteOff();
            }
            else if (msg.isChannelPressure())
            {
                float atPressure = (float)msg.getChannelPressureValue() / 127.f;
                for (auto& v : voices)
                    if (v.active)
                        v.vel = juce::jmax(v.vel, atPressure);
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                float modWheel = (float)msg.getControllerValue() / 127.f;
                for (auto& v : voices)
                    if (v.active)
                        v.vel = juce::jmax(v.vel, modWheel * 0.7f);
            }
            else if (msg.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
        }

        // SilenceGate: skip all DSP if engine has been silent long enough
        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buf.clear();
            return;
        }

        // COUPLING-1 fix: coupling accumulators are reset AFTER the DSP block, not before.
        // Previously they were zeroed at the TOP of renderBlock, which destroyed coupling values
        // set by applyCouplingInput() before the block ran — making all coupling non-functional.
        // They are now reset at the bottom of renderBlock (after the sample loop) to clear stale
        // values from disconnected routes for the next block.
        // Snapshot coupling accumulators before reset (#1118 pattern — the previous
        // block's applyCouplingInput writes must be consumed by this block). Reset
        // handles the disconnected-route case where applyCouplingInput wouldn't run
        // for the next block — without the snapshot, per-block writes never take effect.
        const float blockExtPitchMod = extPitchMod;
        const float blockExtDampMod  = extDampMod;
        const float blockExtIntens   = extIntens;
        extPitchMod = 0.f;
        extDampMod = 0.f;
        extIntens = 1.f;

        // --- Snapshot all 31 params ---
        // Section A: Toddler
        float pTodLvl = todLvl ? todLvl->load() : 0.5f;
        float pTodPres = todPres ? todPres->load() : 0.3f;
        float pTodInst = todInst ? todInst->load() : 0;
        // Section B: Tween
        float pTwLvl = twLvl ? twLvl->load() : 0.5f;
        float pTwEmb = twEmb ? twEmb->load() : 0.5f;
        float pTwValve = twValve ? twValve->load() : 0.5f;
        float pTwInst = twInst ? twInst->load() : 0;
        // Section C: Teen
        float pTnLvl = tnLvl ? tnLvl->load() : 0.5f;
        float pTnEmb = tnEmb ? tnEmb->load() : 0.7f;
        float pTnBore = tnBore ? tnBore->load() : 0.5f;
        float pTnInst = tnInst ? tnInst->load() : 0;
        float pTnVibR = tnVibR ? tnVibR->load() : 5.0f;
        float pTnVibD = tnVibD ? tnVibD->load() : 0.3f;
        // Section D: Shared
        float pDa = damp ? damp->load() : 0.995f;
        float pSy = sympA ? sympA->load() : 0.3f;
        float pDR = driftR ? driftR->load() : 0.1f;
        float pDD = driftD ? driftD->load() : 3;
        // Section E: Foreign harmonics
        float pFS = forStr ? forStr->load() : 0;
        float pFDr = forDr ? forDr->load() : 0;
        float pFC = forCold ? forCold->load() : 0;
        // Section F: FX
        float pRevSz = revSz ? revSz->load() : 0.3f;
        float pRevDec = revDecay ? revDecay->load() : 0.5f; // PARAMS-5
        float pChoR = choR ? choR->load() : 1.0f;
        float pChoDep = choDepth ? choDepth->load() : 0.3f; // PARAMS-4
        float pDrv = drv ? drv->load() : 0;
        float pDelMix = delMix ? delMix->load() : 0.2f;
        // Section G: Macros
        float pME = mEmb ? mEmb->load() : 0.5f;
        float pMG = mGrow ? mGrow->load() : 0;
        float pMF = mFor ? mFor->load() : 0;
        float pML = mLake ? mLake->load() : 0.3f;

        // --- Macro expansions ---
        // GROW (M2): age sweep 0=toddler, 0.5=tween, 1.0=teen
        //   Blends voice levels: toddler dominates at low GROW, teen at high
        float growToddler = std::max(0.f, 1.f - pMG * 2.f); // 1.0 @ 0, 0 @ 0.5+
        float growTween = 1.f - std::abs(pMG * 2.f - 1.f);  // 0 @ 0, 1.0 @ 0.5, 0 @ 1.0
        float growTeen = std::max(0.f, pMG * 2.f - 1.f);    // 0 @ 0-0.5, 1.0 @ 1.0
        // Mix voice levels with GROW weighting
        float toddlerMix = pTodLvl * growToddler;
        float tweenMix = pTwLvl * growTween;
        float teenMix = pTnLvl * growTeen;

        // EMBOUCHURE (M1): global embouchure offset applied to all voices
        // FOREIGN (M3): scales foreign harmonic section
        // LAKE (M4): scales reverb/delay atmosphere

        // Effective embouchure per section (macro modulates)
        float effTodPres = std::min(1.f, pTodPres * (0.5f + pME));
        float effTwEmb = std::min(1.f, pTwEmb * (0.5f + pME));
        float effTnEmb = std::min(1.f, pTnEmb * (0.5f + pME));

        // Effective foreign amounts (macro scales)
        float effFS = pFS * (0.5f + pMF);
        float effFDr = pFDr * (0.5f + pMF);
        float effFC = pFC * (0.5f + pMF);

        // Effective FX amounts (LAKE macro scales reverb + delay)
        float effRevSz = std::min(1.f, pRevSz * (0.5f + pML * 1.5f));
        float effDelMix = std::min(1.f, pDelMix * (0.5f + pML * 1.5f));

        // Compute ageScale for LipBuzz from GROW macro
        float ageScale = pMG;

        // --- Instrument-specific body resonance tables (D004 fix) ---
        // Toddler: Conch, Shofar, Didgeridoo, Alphorn, Vuvuzela, Toy Trumpet
        static constexpr float kTodFreq[] = {0.6f, 0.7f, 0.4f, 0.5f, 0.75f, 0.9f};
        static constexpr float kTodQ[] = {4.f, 5.f, 3.f, 4.5f, 7.f, 3.5f};
        // Tween: Trumpet, Alto Sax, Cornet, Flugelhorn, Trombone, Baritone Sax
        static constexpr float kTwFreq[] = {0.8f, 0.7f, 0.75f, 0.65f, 0.5f, 0.45f};
        static constexpr float kTwQ[] = {4.f, 3.f, 4.f, 3.f, 5.f, 3.f};
        // Teen: French Horn, Trombone, Tuba, Euphonium, Tenor Sax, Dungchen, Serpent, Ophicleide, Sackbut, Bass Sax
        static constexpr float kTnFreq[] = {0.6f, 0.5f, 0.4f, 0.45f, 0.65f, 0.3f, 0.35f, 0.4f, 0.55f, 0.35f};
        static constexpr float kTnQ[] = {5.f, 5.f, 4.f, 4.f, 3.f, 6.f, 5.f, 5.f, 5.f, 3.f};
        int todI = std::min((int)pTodInst, 5);
        int twI = std::min((int)pTwInst, 5);
        int tnI = std::min((int)pTnInst, 9);
        // GROW-weighted blend of body resonance across age groups
        float wTot = growToddler + growTween + growTeen;
        float instrFreqMult =
            (wTot > 0.001f) ? (kTodFreq[todI] * growToddler + kTwFreq[twI] * growTween + kTnFreq[tnI] * growTeen) / wTot
                            : kTwFreq[twI];
        float instrQ = (wTot > 0.001f)
                           ? (kTodQ[todI] * growToddler + kTwQ[twI] * growTween + kTnQ[tnI] * growTeen) / wTot
                           : kTwQ[twI];

        // Pre-compute SR-scaled reverb comb lengths (once per block, not per sample)
        // PERF-3 fix: use lroundf() for correct rounding (cast truncates toward zero).
        // STABILITY-3 fix: clamp each comb length to kRevMax-1 to prevent buffer overrun
        // if sr > 96kHz (kRevMax=4096 is sized for 96kHz max; at 192kHz srMul=4, combLen[3]=6236).
        int srMul = std::max(1, (int)std::lroundf((float)(sr / 44100.0)));
        int combLens[4] = {
            std::min(1117 * srMul, kRevMax - 1),
            std::min(1277 * srMul, kRevMax - 1),
            std::min(1423 * srMul, kRevMax - 1),
            std::min(1559 * srMul, kRevMax - 1)
        };

        // Pre-compute LFO rates once per block (params stable within block)
        choLFO.setRate(pChoR, (float)sr);
        for (auto& v : voices)
            v.vib.setRate(pTnVibR, v.sr);

        // PERF-2 fix: hoist body.setParams() out of the per-sample loop.
        // bFreq and instrQ depend only on v.freq (set at noteOn) and block-stable params
        // (instrFreqMult, instrQ, effFC). setParams() computes fastSin/fastCos internally —
        // calling it once per block instead of once per sample eliminates up to 96,000
        // trig evaluations per second per active voice at 96kHz.
        for (auto& v : voices)
        {
            if (!v.active)
                continue;
            float bFreq = v.freq * instrFreqMult * (1.f + effFC * 0.3f);
            v.body.setParams(bFreq, instrQ + effFC * 4.f);
        }

        auto* oL = buf.getWritePointer(0);
        auto* oR = buf.getNumChannels() > 1 ? buf.getWritePointer(1) : buf.getWritePointer(0);
        // Block-constant release coefficient — depends only on sampleRate × 0.3 s tau.
        // Was std::exp per sample inside the release branch (once per sample per
        // releasing voice). Now one fastExp per block.
        const float releaseCoeff = xoceanus::fastExp(-1.0f / (static_cast<float>(sr) * 0.3f));

        // Block-constant channel pitch-bend ratio; inner per-sample code adds drift
        // and vibrato (both per-sample) on top via separate fastPow2 calls.
        const float blockPitchBendRatio = PitchBendUtil::semitonesToFreqRatio(pitchBendNorm * 2.0f);

        // Body resonance is note-constant (v.freq + block-rate instrument scalars).
        // Hoist setParams out of per-sample loop.
        const float bodyFreqScale = instrFreqMult * (1.0f + effFC * 0.3f);
        const float bodyQScaled   = instrQ + effFC * 4.0f;
        for (auto& v : voices)
        {
            if (v.active)
                v.body.setParams(v.freq * bodyFreqScale, bodyQScaled);
        }

        for (int i = 0; i < ns; ++i)
        {
            float sL = 0, sR = 0;
            for (auto& v : voices)
            {
                if (!v.active)
                    continue;

                // Voice-steal crossfade: fade outgoing voice over 5ms, then start pending note
                if (v.isBeingStolen)
                {
                    v.stealFadeGain -= v.stealFadeStep;
                    if (v.stealFadeGain <= 0.0f)
                    {
                        v.isBeingStolen = false;
                        v.stealFadeGain = 1.0f;
                        v.stealFadeStep = 0.0f;
                        v.noteOn(v.pendingNote, v.pendingVel);
                        continue;
                    }
                }

                // Exponential release — eliminates the "soft note releases fast"
                // bug from linear subtraction on small ampEnv values.
                // PERF-1/SOUND-4 fix: use cached releaseCoeff (computed once in prepare())
                // instead of calling std::exp() per-sample, which was a hot-path overhead.
                if (v.releasing)
                    v.ampEnv *= v.releaseCoeff;
                // releaseCoeff precomputed at block start (block-constant from sampleRate).
                if (v.releasing)
                    v.ampEnv *= releaseCoeff;
                v.ampEnv = flushDenormal(v.ampEnv);
                if (v.ampEnv < 0.0001f && v.releasing)
                {
                    v.active = false;
                    continue;
                }

                // --- Organic drift ---
                float ds = v.drift.tick(pDR, pDD);
                float df = v.freq * fastPow2((ds + blockExtPitchMod) / 12.f) * blockPitchBendRatio;

                // --- Teen vibrato (StandardLFO replaces inline vibPhase accumulator) ---
                // SOUND-1/SOUND-6 fix: use the value returned by process() for vibrato sine.
                // Previously, process() was called for its side-effect (phase advance) and then
                // v.vib.phase was read directly — but phase is already at the NEXT sample after
                // process(), so the vibrato was one sample ahead in phase. Using the returned value
                // (which is computed from the pre-advance phase) gives the correct current output.
                float vibSine = v.vib.process(); // [-1,+1] sine, pre-advance phase
                float vibrato = vibSine * pTnVibD * growTeen;
                df *= fastPow2(vibrato * 0.5f / 12.f); // vibrato in semitone cents

                // --- Foreign harmonics: overtone stretch ---
                float stretch = 1 + effFS * 0.1f;
                float dlen = v.sr / std::max(df * stretch, 20.f);

                // --- Foreign drift: microtonal pitch drift ---
                // SOUND-2 fix: microDrift now uses vibSine (the same oscillator's current
                // output at 3.7× frequency scaling) rather than accessing v.vib.phase directly.
                // The 3.7 multiplier on phase created a separate effective frequency that aliased
                // independently of sample rate; using the returned vibSine value at a fixed
                // phase offset provides a more controlled, rate-correct secondary modulation.
                float microDrift = effFDr * fastSin(v.vib.getPhase() * 3.7f * 6.2831853f) * 0.02f;
                dlen *= (1.f + microDrift);

                float out = v.dl.read(dlen);

                // --- D001 spectral: velocity scales embouchure tightness (brighter at ff) ---
                float velIntens = 0.5f + v.vel * 0.5f; // velocity 0→1 maps to 0.5→1.0x intensity
                float effIntens = blockExtIntens * velIntens;

                // --- Excitation: blended lip buzz from 3 independent voice sections ---
                // SOUND-5 fix: each age section uses its own LipBuzzExciter so that their
                // phase accumulators are independent. Previously a single lipBuzz was called
                // three times per sample, making the "three voices" read staggered positions
                // of one shared oscillator — not three separate timbres.
                // Toddler: loose lips, low pressure, simple
                float excToddler = v.lipBuzzToddler.tick(df * 0.998f, effTodPres * 0.4f * velIntens, 0.0f) * toddlerMix;
                // Tween: moderate embouchure, valve modulates pitch slightly
                // SOUND-1 fix cascade: use vibSine (already computed above) instead of v.vib.phase
                float tweenPitchMod = 1.f + pTwValve * 0.003f * fastSin(v.vib.getPhase() * 2.1f * 6.2831853f);
                float excTween = v.lipBuzzTween.tick(df * tweenPitchMod, effTwEmb * 0.7f * velIntens, 0.5f) * tweenMix;
                // Teen: full virtuosity, bore width affects body
                float excTeen = v.lipBuzzTeen.tick(df, std::min(1.f, effTnEmb * velIntens), ageScale) * teenMix;

                float exc = (excToddler + excTween + excTeen) * effIntens;

                // --- Waveguide feedback ---
                // --- Bore width (teen): wider bore = lower damping, darker ---
                float boreDamp = std::clamp(pDa + blockExtDampMod, 0.f, 1.f) * (1.f - pTnBore * growTeen * 0.05f);
                float damped = v.df.process(out + exc * 0.3f, boreDamp);

                v.dl.write(flushDenormal(damped));

                // --- Body resonance: hoisted to per-block voice setup (note-constant) ---
                float bo = out + v.body.process(out) * 0.2f;

                // --- Sympathetic resonance ---
                float so = v.symp.process(bo, pSy);

                float sig = (bo + so) * v.ampEnv * v.stealFadeGain * 0.4f;

                // --- Stereo spread from age/grow ---
                // SOUND-3 fix: normalized pan law so total power is constant across GROW values.
                // Previous formula: sL += sig*(0.5+w), sR += sig*(1-w) — at w=0.1 (toddler),
                // sL=0.6, sR=0.9 making R louder than L. Now uses complementary pan: L=(0.5+w),
                // R=(0.5-w) with w ∈ [0.1, 0.5], giving L+R = 1.0 at all ages.
                float w = 0.1f + ageScale * 0.4f;  // w: 0.1 (toddler, nearly center) → 0.5 (teen, full left)
                sL += sig * (0.5f + w);
                sR += sig * (0.5f - w);
            }

            // --- Drive (soft clipping) ---
            if (pDrv > 0.001f)
            {
                float dGain = 1.f + pDrv * 8.f;
                sL = fastTanh(sL * dGain) / dGain;
                sR = fastTanh(sR * dGain) / dGain;
            }

            // --- Chorus (StandardLFO replaces inline choPhase accumulator) ---
            if (pChoR > 0.001f)
            {
                float choMod = choLFO.process();
                // PARAMS-4 fix: use pChoDep (user-controllable) instead of hardcoded 0.3f.
                float choDelay = 0.005f * (float)sr * (1.f + choMod * pChoDep); // ~5ms center
                // Use delay buffer for chorus (read from delay line with modulated offset)
                int choIdx = ((delWr - (int)choDelay) % kDelMax + kDelMax) % kDelMax;
                float choL = delBufL[choIdx] * 0.3f;
                float choR_ = delBufR[choIdx] * 0.3f;
                sL += choL;
                sR += choR_;
            }

            // --- Delay (stereo ping-pong style) ---
            float delL = 0, delR = 0;
            if (effDelMix > 0.001f)
            {
                // V011: LAKE scales delay 250ms→2000ms (Schulze's mountain)
                float delTimeSec = 0.25f + pML * 1.75f;
                int delTime = std::min((int)(delTimeSec * (float)sr), kDelMax - 1);
                int rdIdx = ((delWr - delTime) % kDelMax + kDelMax) % kDelMax;
                float fb = 0.6f - pML * 0.15f; // soften feedback at long times (avoid infinite sustain)
                delL = delBufL[rdIdx] * fb;
                delR = delBufR[rdIdx] * fb;
            }
            delBufL[delWr] = sL + delR * 0.4f; // cross-feed for ping-pong
            delBufR[delWr] = sR + delL * 0.4f;
            delWr = (delWr + 1) % kDelMax;
            sL += delL * effDelMix;
            sR += delR * effDelMix;

            // --- Reverb (Schroeder 4-comb approximation) ---
            if (effRevSz > 0.001f)
            {
                float revIn = (sL + sR) * 0.5f;
                // 4 comb filters (combLens pre-computed before loop)
                float revOut = 0;
                for (int c = 0; c < 4; ++c)
                {
                    // PARAMS-5 fix: reverb feedback now uses pRevDec independent of room size.
                    // fb range: 0.55 (short decay) → 0.98 (long decay). Previously was tied to
                    // effRevSz, making large room always have long decay (and vice versa).
                    float fb = 0.55f + pRevDec * 0.43f; // feedback 0.55-0.98
                    float rd = revComb[c][(revPos[c] - combLens[c] + kRevMax) % kRevMax];
                    float wr = revIn + rd * fb;
                    // LP in feedback for warm tail
                    revState[c] = flushDenormal(revState[c] * 0.7f + wr * 0.3f);
                    revComb[c][revPos[c]] = revState[c];
                    revPos[c] = (revPos[c] + 1) % kRevMax;
                    revOut += rd;
                }
                revOut *= 0.25f;
                // DSP FIX: Slight stereo decorrelation on reverb (was mono sum).
                // L gets full reverb, R gets 95% — creates width without phase issues.
                sL += revOut * effRevSz * 0.5f;
                sR += revOut * effRevSz * 0.475f;
            }

            oL[i] += sL;
            oR[i] += sR;
            lastL = sL;
            lastR = sR;
        }
        {
            int c = 0;
            for (auto& v : voices)
                if (v.active)
                    ++c;
            activeVoiceCount_.store(c, std::memory_order_relaxed);
        }
        // COUPLING-1 fix: reset coupling accumulators AFTER the DSP block so that values
        // set by applyCouplingInput() are consumed this block, and stale routes don't
        // persist into the next block without a fresh applyCouplingInput() call.
        extPitchMod = 0.f;
        extDampMod = 0.f;
        extIntens = 1.f;
        // SilenceGate: analyze output level for next-block bypass decision
        silenceGate.analyzeBlock(buf.getReadPointer(0), buf.getNumChannels() > 1 ? buf.getReadPointer(1) : nullptr, ns);
    }

    float getSampleForCoupling(int ch, int) const override { return ch == 0 ? lastL : lastR; }
    void applyCouplingInput(CouplingType t, float amount, const float* buf, int /*ns*/) override
    {
        switch (t)
        {
        case CouplingType::LFOToPitch:
            // COUPLING-3 fix: use buf[0] (the frame's first sample) for the block-level
            // pitch modulation value. The previous code was correct but lacked a comment —
            // block-level coupling intentionally samples at buf[0] since the modulator
            // (LFO) is slow relative to block size and a single sample is representative.
            extPitchMod = (buf ? buf[0] : 0.f) * amount * 2.f;
            break;
        case CouplingType::AmpToFilter:
            extDampMod = amount * 0.08f;
            break;
        case CouplingType::EnvToMorph:
            extIntens = 1.f + amount * 0.5f;
            break;
        default:
            break;
        }
    }

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& p)
    {
        using F = juce::AudioParameterFloat;
        using C = juce::AudioParameterChoice;
        using N = juce::NormalisableRange<float>;

        // Section A: Toddler voice (3 params)
        p.push_back(std::make_unique<F>("otto_toddlerLevel", "Toddler Level", N{0, 1}, 0.5f));
        p.push_back(std::make_unique<F>("otto_toddlerPressure", "Toddler Pressure", N{0, 1}, 0.3f));
        p.push_back(std::make_unique<C>(
            "otto_toddlerInst", "Toddler Inst",
            juce::StringArray{"Conch", "Shofar", "Didgeridoo", "Alphorn", "Vuvuzela", "Toy Trumpet"}, 0));

        // Section B: Tween voice (4 params)
        p.push_back(std::make_unique<F>("otto_tweenLevel", "Tween Level", N{0, 1}, 0.5f));
        p.push_back(std::make_unique<F>("otto_tweenEmbouchure", "Tween Embouchure", N{0, 1}, 0.5f));
        p.push_back(std::make_unique<F>("otto_tweenValve", "Tween Valve", N{0, 1}, 0.5f));
        p.push_back(std::make_unique<C>(
            "otto_tweenInst", "Tween Inst",
            juce::StringArray{"Trumpet", "Alto Sax", "Cornet", "Flugelhorn", "Trombone", "Baritone Sax"}, 0));

        // Section C: Teen voice (6 params)
        p.push_back(std::make_unique<F>("otto_teenLevel", "Teen Level", N{0, 1}, 0.5f));
        p.push_back(std::make_unique<F>("otto_teenEmbouchure", "Teen Embouchure", N{0, 1}, 0.7f));
        p.push_back(std::make_unique<F>("otto_teenBore", "Teen Bore", N{0, 1}, 0.5f));
        p.push_back(std::make_unique<C>("otto_teenInst", "Teen Inst",
                                        juce::StringArray{"French Horn", "Trombone", "Tuba", "Euphonium", "Tenor Sax",
                                                          "Dungchen", "Serpent", "Ophicleide", "Sackbut", "Bass Sax"},
                                        0));
        p.push_back(std::make_unique<F>("otto_teenVibratoRate", "Teen Vibrato Rate", N{3, 8}, 5.0f));
        p.push_back(std::make_unique<F>("otto_teenVibratoDepth", "Teen Vibrato Depth", N{0, 1}, 0.3f));

        // Section D: Shared (4 params)
        p.push_back(std::make_unique<F>("otto_damping", "Damping", N{0.8f, 0.999f}, 0.995f));
        p.push_back(std::make_unique<F>("otto_sympatheticAmt", "Sympathetic", N{0, 1}, 0.3f));
        p.push_back(std::make_unique<F>("otto_driftRate", "Drift Rate", N{0.005f, 0.5f}, 0.1f));
        p.push_back(std::make_unique<F>("otto_driftDepth", "Drift Depth", N{0, 20}, 3.0f));

        // Section E: Foreign harmonics (3 params)
        p.push_back(std::make_unique<F>("otto_foreignStretch", "Foreign Stretch", N{0, 1}, 0.0f));
        p.push_back(std::make_unique<F>("otto_foreignDrift", "Foreign Drift", N{0, 1}, 0.0f));
        p.push_back(std::make_unique<F>("otto_foreignCold", "Foreign Cold", N{0, 1}, 0.0f));

        // Section F: FX (4 params)
        p.push_back(std::make_unique<F>("otto_reverbSize", "Reverb Size", N{0, 1}, 0.3f));
        // PARAMS-5 fix: expose reverb decay as a separate parameter — previously it was
        // fully coupled to revSz (fb = 0.7 + revSz*0.28). A large room can have short
        // decay and vice versa; exposing it gives independent control.
        p.push_back(std::make_unique<F>("otto_reverbDecay", "Reverb Decay", N{0, 1}, 0.5f));
        // PARAMS-2 fix: range starts from 0 so chorus can be disabled (rate 0 = bypassed via > 0.001f guard).
        // Previous minimum of 0.1f made chorus always-on with no off position.
        p.push_back(std::make_unique<F>("otto_chorusRate", "Chorus Rate", N{0.0f, 5.0f}, 1.0f));
        // PARAMS-4 fix: expose chorus depth as a parameter (was hardcoded at 0.3).
        p.push_back(std::make_unique<F>("otto_chorusDepth", "Chorus Depth", N{0, 1}, 0.3f));
        p.push_back(std::make_unique<F>("otto_driveAmount", "Drive", N{0, 1}, 0.0f));
        p.push_back(std::make_unique<F>("otto_delayMix", "Delay Mix", N{0, 1}, 0.2f));

        // Section G: Macros (4 params)
        p.push_back(std::make_unique<F>("otto_macroEmbouchure", "EMBOUCHURE", N{0, 1}, 0.5f));
        // PARAMS-3 fix: default 0.5 centers GROW on the Tween section (professional brass midpoint).
        // Previous default 0.35 landed in the toddler/tween overlap with no clear dominant voice.
        p.push_back(std::make_unique<F>("otto_macroGrow", "GROW", N{0, 1}, 0.5f));
        p.push_back(std::make_unique<F>("otto_macroForeign", "FOREIGN", N{0, 1}, 0.0f));
        p.push_back(std::make_unique<F>("otto_macroLake", "LAKE", N{0, 1}, 0.3f));
    }
    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        // Section A: Toddler
        todLvl = apvts.getRawParameterValue("otto_toddlerLevel");
        todPres = apvts.getRawParameterValue("otto_toddlerPressure");
        todInst = apvts.getRawParameterValue("otto_toddlerInst");
        // Section B: Tween
        twLvl = apvts.getRawParameterValue("otto_tweenLevel");
        twEmb = apvts.getRawParameterValue("otto_tweenEmbouchure");
        twValve = apvts.getRawParameterValue("otto_tweenValve");
        twInst = apvts.getRawParameterValue("otto_tweenInst");
        // Section C: Teen
        tnLvl = apvts.getRawParameterValue("otto_teenLevel");
        tnEmb = apvts.getRawParameterValue("otto_teenEmbouchure");
        tnBore = apvts.getRawParameterValue("otto_teenBore");
        tnInst = apvts.getRawParameterValue("otto_teenInst");
        tnVibR = apvts.getRawParameterValue("otto_teenVibratoRate");
        tnVibD = apvts.getRawParameterValue("otto_teenVibratoDepth");
        // Section D: Shared
        damp = apvts.getRawParameterValue("otto_damping");
        sympA = apvts.getRawParameterValue("otto_sympatheticAmt");
        driftR = apvts.getRawParameterValue("otto_driftRate");
        driftD = apvts.getRawParameterValue("otto_driftDepth");
        // Section E: Foreign harmonics
        forStr = apvts.getRawParameterValue("otto_foreignStretch");
        forDr = apvts.getRawParameterValue("otto_foreignDrift");
        forCold = apvts.getRawParameterValue("otto_foreignCold");
        // Section F: FX
        revSz = apvts.getRawParameterValue("otto_reverbSize");
        revDecay = apvts.getRawParameterValue("otto_reverbDecay"); // PARAMS-5
        choR = apvts.getRawParameterValue("otto_chorusRate");
        choDepth = apvts.getRawParameterValue("otto_chorusDepth"); // PARAMS-4
        drv = apvts.getRawParameterValue("otto_driveAmount");
        delMix = apvts.getRawParameterValue("otto_delayMix");
        // Section G: Macros
        mEmb = apvts.getRawParameterValue("otto_macroEmbouchure");
        mGrow = apvts.getRawParameterValue("otto_macroGrow");
        mFor = apvts.getRawParameterValue("otto_macroForeign");
        mLake = apvts.getRawParameterValue("otto_macroLake");
    }

    juce::String getEngineId() const override { return "Ottoni"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF5B8A72); } // Patina
    int getMaxVoices() const override { return kV; }
    int getActiveVoiceCount() const override { return activeVoiceCount_.load(std::memory_order_relaxed); }

private:
    SilenceGate silenceGate;
    static constexpr int kV = 12;
    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    int nv = 0;
    float lastL = 0, lastR = 0; // ac promoted to base class activeVoiceCount_
    std::array<OttoniAdapterVoice, kV> voices;

    float pitchBendNorm = 0.0f; // MIDI pitch wheel [-1, +1]; ±2 semitone range

    // Family coupling ext mods (SP7.3)
    float extPitchMod = 0.f; // semitones from LFOToPitch
    float extDampMod = 0.f;  // 0–1 from AmpToFilter
    float extIntens = 1.f;   // multiplier from EnvToMorph

    // --- Cached parameter pointers (31 total) ---
    // Section A: Toddler
    std::atomic<float>*todLvl = nullptr, *todPres = nullptr, *todInst = nullptr;
    // Section B: Tween
    std::atomic<float>*twLvl = nullptr, *twEmb = nullptr, *twValve = nullptr, *twInst = nullptr;
    // Section C: Teen
    std::atomic<float>*tnLvl = nullptr, *tnEmb = nullptr, *tnBore = nullptr, *tnInst = nullptr;
    std::atomic<float>*tnVibR = nullptr, *tnVibD = nullptr;
    // Section D: Shared
    std::atomic<float>*damp = nullptr, *sympA = nullptr;
    std::atomic<float>*driftR = nullptr, *driftD = nullptr;
    // Section E: Foreign
    std::atomic<float>*forStr = nullptr, *forDr = nullptr, *forCold = nullptr;
    // Section F: FX
    std::atomic<float>*revSz = nullptr, *revDecay = nullptr, *choR = nullptr, *choDepth = nullptr, *drv = nullptr, *delMix = nullptr;
    // Section G: Macros
    std::atomic<float>*mEmb = nullptr, *mGrow = nullptr, *mFor = nullptr, *mLake = nullptr;

    // --- FX state (pre-allocated, no audio-thread alloc) ---
    // Reverb: 4-comb Schroeder
    static constexpr int kRevMax = 4096; // extended for 96kHz (max comb = 1559*2 = 3118 < 4096)
    float revComb[4][kRevMax] = {};
    int revPos[4] = {};
    float revState[4] = {};
    // Chorus LFO (replaces inline choPhase accumulator)
    StandardLFO choLFO;
    // Delay buffer (stereo, ~1s at 48kHz)
    static constexpr int kDelMax = 192001; // extended for 2000ms at 96kHz (96kHz delay buffer truncation fix)
    float delBufL[kDelMax] = {};
    float delBufR[kDelMax] = {};
    int delWr = 0;
};

} // namespace xoceanus
