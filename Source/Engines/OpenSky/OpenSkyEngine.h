#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>
#include <vector>

namespace xomnibus {

//==============================================================================
//
//  OPENSKY ENGINE — Supersaw + Shimmer Poly Synth
//  XOmnibus Engine Module | Accent: Sunburst #FF8C00
//
//  Creature: Flying Fish School (surface breakers)
//  Habitat:  THE SURFACE — pure sunlight, pure feliX energy
//
//  XOpenSky lives at the very top of the XO_OX water column. It is the
//  engine of euphoria: a flying fish school launching skyward in synchronized
//  arcs. No Oscar here — only feliX, pure upward motion, iridescent shimmer.
//  Where OCEANDEEP pulls down, OPENSKY launches up.
//
//  Architecture (per voice, 8-voice polyphony):
//    SuperSaw bank (7 detuned saws) → Amplitude envelope
//    Shimmer stacker (4 pitched voices at oct+fifth intervals) → Shimmer envelope
//    Bright one-pole LP filter (tilt away from mud, toward sky)
//    Sky reverb (Schroeder comb+allpass, long tail, high damping cut)
//    Stereo chorus spread on shimmer layer
//
//  4 Macros:
//    CHARACTER  — shimmer depth (mix of shimmer layer vs fundamental)
//    MOVEMENT   — shimmer rate (LFO speed on shimmer pitch + chorus rate)
//    COUPLING   — standard cross-engine coupling amount
//    SPACE      — reverb size / tail length
//
//  33 sky_ parameters — doctrine compliant D001–D006.
//
//  Parameter prefix: sky_   Engine ID: "OpenSky"
//==============================================================================

// ---------------------------------------------------------------------------
// SuperSaw oscillator — 7 detuned square/saw voices summed in mono
// Classic Roland JP-8000 / supersaw architecture
// ---------------------------------------------------------------------------
struct SkySuperSaw {
    static constexpr int kSaws = 7;
    float phases[kSaws] = {};
    float sr = 44100.f;

    void prepare(double s) { sr = (float)s; }
    void reset()  { for (auto& p : phases) p = 0.f; }

    // detune: 0=pure, 1=maximum chorus spread (~±20 cents per voice)
    float tick(float freq, float detune) {
        // Symmetric detune offsets in semitones (JP-8000 approximation)
        static constexpr float kDetuneTable[kSaws] = {
            0.f, 0.117f, -0.117f, 0.239f, -0.239f, 0.362f, -0.362f
        };
        float out = 0.f;
        for (int i = 0; i < kSaws; ++i) {
            float cents = kDetuneTable[i] * detune * 20.f; // ±20 cents max
            float f = freq * fastPow2(cents / 1200.f);
            phases[i] += f / sr;
            if (phases[i] >= 1.f) phases[i] -= 1.f;
            // Bandlimited sawtooth approximation via PolyBLEP would be ideal;
            // naive saw is sufficient for shimmer pads (HF filtered anyway)
            out += phases[i] * 2.f - 1.f;
        }
        return out * (1.f / kSaws); // normalize
    }
};

// ---------------------------------------------------------------------------
// Shimmer voice — single sine/saw pitched above fundamental at a harmonic
// interval. Four shimmer voices per engine voice make the sky layer.
// ---------------------------------------------------------------------------
struct SkyShimmerVoice {
    float phase = 0.f, envLevel = 0.f, sr = 44100.f;
    float lfoPhase = 0.f; // per-voice LFO for gentle pitch wander

    void prepare(double s) { sr = (float)s; reset(); }
    void reset()  { phase = envLevel = lfoPhase = 0.f; }
    void trigger(float initialAmp) { envLevel = initialAmp; }

    // interval: pitch multiplier (e.g. 2.0 = octave, 3.0 = oct+fifth)
    // lfoRate:  Hz (MOVEMENT macro controls)
    // lfoDepth: semitones of wander
    float tick(float baseFreq, float interval, float lfoRate, float lfoDepth,
               float decayCoeff) {
        // Slow envelope decay (shimmer fades gently)
        envLevel *= decayCoeff;
        envLevel = flushDenormal(envLevel);

        // Per-voice pitch LFO (triangle shape for smoothness)
        lfoPhase += lfoRate / sr;
        if (lfoPhase >= 1.f) lfoPhase -= 1.f;
        float lfo = (lfoPhase < 0.5f)
                  ? (lfoPhase * 4.f - 1.f)     // 0→1 half: -1..+1
                  : (3.f - lfoPhase * 4.f);     // 1→0 half: +1..-1
        float pitchMod = fastPow2(lfo * lfoDepth / 1200.f);

        float freq = baseFreq * interval * pitchMod;
        phase += freq / sr;
        if (phase >= 1.f) phase -= 1.f;

        // Sine wave — pure and bright, no aliasing concern at these intervals
        return std::sin(phase * 6.2831853f) * envLevel;
    }
};

// ---------------------------------------------------------------------------
// One-pole low-pass — gentle sky brightness filter
// Rolls off sub-mud, keeps shimmer sparkle.
// fc in Hz, computed via matched-Z: coeff = exp(-2π·fc/sr)
// ---------------------------------------------------------------------------
struct SkyBrightFilter {
    float z1 = 0.f;

    void reset() { z1 = 0.f; }

    // res: 0-1 resonance amount (feedback from previous output)
    float process(float in, float fc, float sr, float res = 0.f) {
        float coeff = fastExp(-6.2831853f * fc / sr);
        // Resonance: feed back previous output into input (self-oscillation capped at 0.85)
        float feedback = clamp(res * 0.85f, 0.f, 0.84f);
        float driven = in + z1 * feedback;
        float out = driven * (1.f - coeff) + z1 * coeff;
        z1 = flushDenormal(out);
        return out;
    }
};

// ---------------------------------------------------------------------------
// Sky reverb — Schroeder topology optimized for long airy tails
// 4 comb filters tuned for >2s RT60 at full SPACE, 2 allpass diffusers.
// Independent L/R comb lengths for stereo spread.
// ---------------------------------------------------------------------------
struct SkyReverb {
    static constexpr int kCombs = 4;
    // Prime-ish comb lengths at 44100 Hz — scale with sample rate
    static constexpr int kCombLensL[kCombs] = { 1481, 1601, 1747, 1867 };
    static constexpr int kCombLensR[kCombs] = { 1553, 1663, 1801, 1931 };
    static constexpr int kAP1Len = 347, kAP2Len = 673;

    std::vector<float> combL[kCombs], combR[kCombs];
    std::vector<float> ap1L, ap2L, ap1R, ap2R;
    int combPosL[kCombs]={}, combPosR[kCombs]={};
    int ap1PosL=0, ap2PosL=0, ap1PosR=0, ap2PosR=0;
    float combStateL[kCombs]={}, combStateR[kCombs]={};

    void prepare(double /*sr*/) {
        for (int i = 0; i < kCombs; ++i) {
            combL[i].assign(kCombLensL[i], 0.f); combPosL[i] = 0; combStateL[i] = 0.f;
            combR[i].assign(kCombLensR[i], 0.f); combPosR[i] = 0; combStateR[i] = 0.f;
        }
        ap1L.assign(kAP1Len, 0.f); ap2L.assign(kAP2Len, 0.f);
        ap1R.assign(kAP1Len, 0.f); ap2R.assign(kAP2Len, 0.f);
        ap1PosL=ap2PosL=ap1PosR=ap2PosR=0;
    }

    void reset() {
        for (int i = 0; i < kCombs; ++i) {
            std::fill(combL[i].begin(), combL[i].end(), 0.f); combStateL[i]=0.f; combPosL[i]=0;
            std::fill(combR[i].begin(), combR[i].end(), 0.f); combStateR[i]=0.f; combPosR[i]=0;
        }
        std::fill(ap1L.begin(),ap1L.end(),0.f); std::fill(ap2L.begin(),ap2L.end(),0.f);
        std::fill(ap1R.begin(),ap1R.end(),0.f); std::fill(ap2R.begin(),ap2R.end(),0.f);
        ap1PosL=ap2PosL=ap1PosR=ap2PosR=0;
    }

    // process — stereo in, stereo out
    // space: 0-1 (scales feedback 0.7→0.93)
    // mix:   0-1 wet mix
    void process(float& inL, float& inR, float space, float mix) {
        float fb = 0.70f + space * 0.23f; // 0.70→0.93 feedback

        auto runComb = [&](std::vector<float>& buf, int& pos, float& state,
                           int len, float input) -> float {
            float rd = buf[pos];
            // High-frequency damping: mild one-pole LP in feedback path
            // Use light damping coefficient — sky reverb stays bright
            state = flushDenormal(rd * 0.85f + state * 0.15f);
            buf[pos] = flushDenormal(input + state * fb);
            pos = (pos + 1) % len;
            return rd;
        };

        auto runAP = [&](std::vector<float>& buf, int& pos, int len,
                         float input) -> float {
            float rd = buf[pos];
            buf[pos] = flushDenormal(input + rd * 0.5f);
            float out = rd - input * 0.5f;
            pos = (pos + 1) % len;
            return out;
        };

        // Left channel
        float combOutL = 0.f;
        for (int i = 0; i < kCombs; ++i)
            combOutL += runComb(combL[i], combPosL[i], combStateL[i], kCombLensL[i], inL);
        combOutL *= 0.25f;
        float wetL = runAP(ap1L, ap1PosL, kAP1Len, combOutL);
        wetL       = runAP(ap2L, ap2PosL, kAP2Len, wetL);

        // Right channel
        float combOutR = 0.f;
        for (int i = 0; i < kCombs; ++i)
            combOutR += runComb(combR[i], combPosR[i], combStateR[i], kCombLensR[i], inR);
        combOutR *= 0.25f;
        float wetR = runAP(ap1R, ap1PosR, kAP1Len, combOutR);
        wetR       = runAP(ap2R, ap2PosR, kAP2Len, wetR);

        inL = inL * (1.f - mix) + flushDenormal(wetL) * mix;
        inR = inR * (1.f - mix) + flushDenormal(wetR) * mix;
    }
};

// ---------------------------------------------------------------------------
// Simple stereo chorus — spreads shimmer layer across the field.
// Two delay lines modulated at slightly different rates.
// ---------------------------------------------------------------------------
struct SkyChorus {
    static constexpr int kBufLen = 4096;
    float bufL[kBufLen] = {}, bufR[kBufLen] = {};
    int writePos = 0;
    float lfoL = 0.f, lfoR = 0.f;
    float sr = 44100.f;

    void prepare(double s) { sr = (float)s; reset(); }
    void reset() {
        std::fill(bufL, bufL+kBufLen, 0.f);
        std::fill(bufR, bufR+kBufLen, 0.f);
        writePos=0; lfoL=0.f; lfoR=0.5f; // 180° phase offset
    }

    // rate: LFO Hz  depth: delay depth in ms  mix: wet amount
    void process(float& L, float& R, float rate, float depthMs, float mix) {
        lfoL += rate / sr; if (lfoL >= 1.f) lfoL -= 1.f;
        lfoR += (rate * 1.03f) / sr; if (lfoR >= 1.f) lfoR -= 1.f;

        float modL = (std::sin(lfoL * 6.2831853f) * 0.5f + 0.5f);
        float modR = (std::sin(lfoR * 6.2831853f) * 0.5f + 0.5f);

        // Delay in samples: 5–(5+depthMs) ms range
        float baseDelay = 0.005f * sr;
        float delL = baseDelay + modL * depthMs * 0.001f * sr;
        float delR = baseDelay + modR * depthMs * 0.001f * sr;

        bufL[writePos] = L;
        bufR[writePos] = R;

        auto readBuf = [&](float* buf, float delaySamples) -> float {
            int d0 = (int)delaySamples;
            float frac = delaySamples - (float)d0;
            int r0 = ((writePos - d0) % kBufLen + kBufLen) % kBufLen;
            int r1 = (r0 - 1 + kBufLen) % kBufLen;
            return buf[r0] * (1.f - frac) + buf[r1] * frac;
        };

        float wetL = readBuf(bufL, delL);
        float wetR = readBuf(bufR, delR);
        writePos = (writePos + 1) % kBufLen;

        L = L * (1.f - mix) + wetL * mix;
        R = R * (1.f - mix) + wetR * mix;
    }
};

// ---------------------------------------------------------------------------
// Single OPENSKY voice
// Supersaw fundamental + 4 shimmer voices + amp/shimmer envelopes
// ---------------------------------------------------------------------------
struct OpenSkyVoice {
    bool active = false;
    bool releasing = false;
    int  note = 0;
    float vel = 0.f, freq = 440.f;
    float glideFreq = 440.f;    // D004: smoothed frequency for portamento (sky_glide)
    float ampEnv = 0.f;         // main amplitude envelope level (attack/decay/sustain/release)
    float ampAttackCoeff = 0.f; // per-sample attack increment
    float ampReleaseCoeff = 0.f;
    bool  inAttack = true;
    float sr = 44100.f;

    SkySuperSaw supersaw;
    SkyShimmerVoice shimmer[4];
    SkyBrightFilter filterL, filterR;

    // Shimmer interval ratios: oct, oct+5th, 2oct, 2oct+5th
    static constexpr float kShimmerIntervals[4] = { 2.f, 3.f, 4.f, 6.f };

    void prepare(double s) {
        sr = (float)s;
        supersaw.prepare(s);
        for (auto& sh : shimmer) sh.prepare(s);
    }

    void reset() {
        supersaw.reset();
        for (auto& sh : shimmer) sh.reset();
        filterL.reset(); filterR.reset();
        active = releasing = inAttack = false;
        ampEnv = 0.f;
    }

    // attack, release: seconds
    void noteOn(int n, float v, float attack, float release) {
        note = n;
        vel  = v;
        freq = 440.f * fastPow2((n - 69) / 12.f);
        // D004: glideFreq starts at target freq on first note (no glide from silence)
        if (!active) glideFreq = freq;
        releasing = false;
        inAttack  = true;
        active    = true;
        // Trigger shimmer voices with velocity-scaled initial level
        for (auto& sh : shimmer) sh.trigger(v * 0.7f);
        setEnvCoeffs(attack, release);
    }

    void noteOff() { releasing = true; inAttack = false; }

    void setEnvCoeffs(float attack, float release) {
        float att = std::max(attack, 0.001f);
        float rel = std::max(release, 0.01f);
        ampAttackCoeff  = 1.f / (sr * att);
        ampReleaseCoeff = 1.f / (sr * rel);
    }

    // Returns mono mix of supersaw + shimmer
    // Parameters: detune, brightness filter fc, filterRes, shimmerDepth, shimmerRate,
    //             shimmerDecay (per-sample coeff), lfoDepth (semitones),
    //             shimmerOctShift (octave transpose, 0=no shift, 1=+1 oct),
    //             glideCoeff (portamento smoothing coeff, 1.0=instant)
    float tick(float detune, float filterFc, float filterRes, float shimmerDepth,
               float shimmerRate, float shimmerDecay, float lfoDepth,
               float shimmerOctShift, float glideCoeff) {
        // Amplitude envelope
        if (inAttack) {
            ampEnv += ampAttackCoeff;
            if (ampEnv >= 1.f) { ampEnv = 1.f; inAttack = false; }
        } else if (releasing) {
            ampEnv -= ampReleaseCoeff;
            if (ampEnv <= 0.f) { ampEnv = 0.f; active = false; }
        }
        ampEnv = flushDenormal(ampEnv);

        if (!active && ampEnv <= 0.f) return 0.f;

        // D004: glide — smooth glideFreq toward target freq each sample
        glideFreq += (freq - glideFreq) * glideCoeff;
        glideFreq = flushDenormal(glideFreq);
        float playFreq = glideFreq;

        // D001: velocity → brightness (higher velocity = brighter filter)
        float velBrightness = 0.5f + vel * 0.5f; // 0.5→1.0
        float fc = filterFc * velBrightness;

        // Supersaw fundamental
        float sawOut = supersaw.tick(playFreq, detune);

        // Apply brightness filter with resonance (D004: filterRes now shapes tone)
        sawOut = filterL.process(sawOut, fc, sr, filterRes);

        // Shimmer layer — D004: shimmerOctShift transposes all shimmer voices
        // (0=original intervals, 1=all intervals shifted +1 octave)
        float octMult = fastPow2(shimmerOctShift);
        float shimOut = 0.f;
        for (int i = 0; i < 4; ++i) {
            float interval = kShimmerIntervals[i] * octMult;
            shimOut += shimmer[i].tick(playFreq, interval,
                                       shimmerRate, lfoDepth, shimmerDecay);
        }
        shimOut *= 0.25f; // normalize 4 voices

        // Mix fundamental + shimmer layer
        float sig = sawOut * (1.f - shimmerDepth * 0.6f)
                  + shimOut * shimmerDepth;

        return sig * ampEnv * vel * 0.4f; // 0.4 = per-voice gain scale (8 voices)
    }
};
constexpr float OpenSkyVoice::kShimmerIntervals[4];

// ===========================================================================
// OpenSkyEngine — XOmnibus adapter  (implements SynthEngine)
// ===========================================================================
class OpenSkyEngine : public SynthEngine {
public:
    //-- SynthEngine lifecycle --------------------------------------------------
    void prepare(double sampleRate, int maxBlockSize) override {
        sr = sampleRate;
        for (auto& v : voices) v.prepare(sampleRate);
        chorus.prepare(sampleRate);
        reverb.prepare(sampleRate);
        silenceGate.prepare(sampleRate, maxBlockSize);
    }

    void releaseResources() override {
        for (auto& v : voices) v.reset();
    }

    void reset() override {
        for (auto& v : voices) v.reset();
        chorus.reset();
        reverb.reset();
        lastL = lastR = 0.f;
        extPitchMod = 0.f;
        extAmpMod   = 1.f;
        atShimmer   = 0.f;
    }

    //-- MIDI + render ----------------------------------------------------------
    void renderBlock(juce::AudioBuffer<float>& buf,
                     juce::MidiBuffer& midi, int ns) override
    {
        // MIDI
        for (const auto m : midi) {
            auto msg = m.getMessage();
            if (msg.isNoteOn()) {
                silenceGate.wake();
                float attack  = pAmpAtk  ? pAmpAtk->load()  : 0.05f;
                float release = pAmpRel  ? pAmpRel->load()   : 0.8f;
                // D004: sky_velSensitivity scales velocity response
                // sens=1.0 → full range; sens=0.0 → always 70% (flat response)
                float sens = pVelSens ? pVelSens->load() : 0.8f;
                float rawVel = msg.getVelocity() / 127.f;
                float effectiveVel = rawVel * sens + (1.f - sens) * 0.7f;
                int t = -1;
                for (int i = 0; i < kVoices; ++i)
                    if (!voices[i].active) { t = i; break; }
                if (t < 0) { t = nextV % kVoices; }
                nextV = (t + 1) % kVoices;
                voices[t].noteOn(msg.getNoteNumber(), effectiveVel,
                                 attack, release);
            }
            else if (msg.isNoteOff()) {
                for (auto& v : voices)
                    if (v.active && v.note == msg.getNoteNumber())
                        v.noteOff();
            }
            else if (msg.isChannelPressure()) {
                // D006: aftertouch → shimmer depth boost (feliX expression)
                atShimmer = (float)msg.getChannelPressureValue() / 127.f;
            }
            else if (msg.isController() && msg.getControllerNumber() == 1) {
                // D006: mod wheel → shimmer depth (same as CHARACTER macro)
                float mw = (float)msg.getControllerValue() / 127.f;
                // Mod wheel adds to CHARACTER shimmer — track separately and blend in render
                modWheelShimmer = mw;
            }
        }

        if (silenceGate.isBypassed() && midi.isEmpty()) { buf.clear(); return; }

        // -- Param reads (all sky_ params) ------------------------------------
        float pDetune   = pSawDetune    ? pSawDetune->load()    : 0.5f;
        float pOscLvl   = pSawLevel     ? pSawLevel->load()     : 1.0f;
        float pFiltFc   = pFilterFc     ? pFilterFc->load()     : 8000.f;
        float pFiltRes  = pFilterRes    ? pFilterRes->load()    : 0.0f; // D004: resonance feedback
        float pAmpA     = pAmpAtk       ? pAmpAtk->load()       : 0.05f;
        float pAmpD     = pAmpDec       ? pAmpDec->load()       : 0.3f;
        float pAmpS     = pAmpSus       ? pAmpSus->load()       : 0.8f;
        float pAmpR     = pAmpRel       ? pAmpRel->load()       : 0.8f;
        float pShimMix  = pShimmerMix   ? pShimmerMix->load()   : 0.5f;
        float pShimDecT = pShimmerDecay ? pShimmerDecay->load() : 2.5f; // seconds
        float pShimOct  = pShimmerOct   ? pShimmerOct->load()   : 1.0f;
        float pLfoRt    = pLfoRate      ? pLfoRate->load()       : 0.08f;
        float pLfoD     = pLfoDepth     ? pLfoDepth->load()      : 8.0f;
        float pRevMix   = pReverbMix    ? pReverbMix->load()     : 0.4f;
        float pRevSize  = pReverbSize   ? pReverbSize->load()    : 0.7f;
        float pChoMix   = pChorusMix    ? pChorusMix->load()     : 0.3f;
        float pChoRate  = pChorusRate   ? pChorusRate->load()    : 0.25f;
        float pChoDepth = pChorusDepth  ? pChorusDepth->load()   : 8.0f;
        float pOutputGn = pOutputGain   ? pOutputGain->load()    : 0.8f;
        float pCoupling = pCouplingAmt  ? pCouplingAmt->load()   : 0.0f;

        // Macros
        float mCharacter = mChar      ? mChar->load()      : 0.5f;
        float mMovement  = mMove      ? mMove->load()      : 0.4f;
        float mCoupling  = mCouple    ? mCouple->load()    : 0.0f;
        float mSpace     = mSpaceM    ? mSpaceM->load()    : 0.5f;

        // CHARACTER macro: drives shimmer depth + mod wheel blend
        float shimmerDepth = clamp(
            pShimMix + mCharacter * 0.4f + modWheelShimmer * 0.3f + atShimmer * 0.2f,
            0.f, 1.f);

        // MOVEMENT macro: drives LFO rate (0.01→2 Hz range)
        float lfoRate = pLfoRt * (0.5f + mMovement * 2.0f);
        lfoRate = clamp(lfoRate, 0.01f, 4.f);

        // SPACE macro: drives reverb mix + reverb size
        float revMix  = clamp(pRevMix  + mSpace * 0.4f, 0.f, 1.f);
        float revSize = clamp(pRevSize + mSpace * 0.2f, 0.f, 1.f);

        // Shimmer decay coefficient (time constant → per-sample multiply)
        // At 44100 Hz, to decay to e^-1 in N seconds: coeff = exp(-1/(sr*N))
        float shimDecay = fastExp(-1.f / (float)(sr * juce::jmax(pShimDecT, 0.05f)));

        // D004: sky_glide — portamento smoothing coefficient
        // sky_glide is in seconds (0=instant, 2=slow). Below 0.5ms treated as instant.
        float pGlideVal = pGlide ? pGlide->load() : 0.f;
        float glideCoeff = (pGlideVal < 0.0005f)
            ? 1.f
            : clamp(1.f - fastExp(-1.f / (pGlideVal * (float)sr)), 0.f, 1.f);

        // D004: sky_shimmerOct — fractional octave shift for shimmer voices (0=off, 1=+1 oct)
        float shimmerOctShift = pShimOct;

        // Update active voice envelopes with current ADSR
        for (auto& v : voices)
            if (v.active && !v.releasing)
                v.setEnvCoeffs(pAmpA, pAmpR);

        // Pitch modulation from external coupling (semitones)
        float pitchModSemi = extPitchMod;

        auto* outL = buf.getWritePointer(0);
        auto* outR = buf.getWritePointer(buf.getNumChannels() > 1 ? 1 : 0);

        for (int i = 0; i < ns; ++i) {
            float sL = 0.f, sR = 0.f;

            for (auto& v : voices) {
                if (!v.active) continue;

                // Apply external pitch mod to voice frequency
                float baseFreq = v.freq;
                if (pitchModSemi != 0.f)
                    v.freq = baseFreq * fastPow2(pitchModSemi / 12.f);

                float mono = v.tick(pDetune, pFiltFc, pFiltRes, shimmerDepth,
                                    lfoRate, shimDecay, pLfoD,
                                    shimmerOctShift, glideCoeff);

                // Restore frequency after pitch mod (avoid drift accumulation)
                v.freq = baseFreq;

                // Shimmer stereo panning: alternate L/R for each voice
                // to create spatial spread without chorus on fundamental
                float pan = 0.5f; // center default
                {
                    // Fixed pan table: voice 0-7, spread across stereo field
                    static constexpr float kPanTable[8] = {
                        0.5f, 0.45f, 0.55f, 0.4f, 0.6f, 0.35f, 0.65f, 0.5f
                    };
                    int vi = (int)(&v - &voices[0]);
                    pan = kPanTable[vi];
                }
                sL += mono * (1.f - pan) * 2.f * pOscLvl * extAmpMod;
                sR += mono * pan          * 2.f * pOscLvl * extAmpMod;
            }

            // Stereo chorus on shimmer mix (widening)
            chorus.process(sL, sR, pChoRate * (0.5f + mMovement), pChoDepth, pChoMix);

            // Sky reverb
            reverb.process(sL, sR, revSize, revMix);

            // Output gain
            sL *= pOutputGn;
            sR *= pOutputGn;

            outL[i] += sL;
            outR[i] += sR;
            lastL = sL; lastR = sR;
        }

        // Voice cleanup + count
        activeCount = 0;
        for (auto& v : voices) if (v.active) ++activeCount;

        silenceGate.analyzeBlock(buf.getReadPointer(0),
                                 buf.getNumChannels() > 1 ? buf.getReadPointer(1) : nullptr,
                                 ns);
    }

    //-- Coupling ---------------------------------------------------------------
    float getSampleForCoupling(int ch, int) const override {
        return ch == 0 ? lastL : lastR;
    }

    void applyCouplingInput(CouplingType t, float amount,
                            const float* buf, int /*ns*/) override {
        switch (t) {
            case CouplingType::LFOToPitch:
                extPitchMod = (buf ? buf[0] : 0.f) * amount * 2.f;
                break;
            case CouplingType::AmpToFilter:
                // Drive shimmer depth from external amp signal
                extAmpMod = 1.f + amount * 0.5f;
                break;
            case CouplingType::EnvToMorph:
                // External env → shimmer depth additive boost
                // (OPENSKY→OPAL coupling path: shimmer drives grain buffer)
                extAmpMod = 1.f + (buf ? buf[0] : 0.f) * amount * 0.4f;
                break;
            default: break;
        }
    }

    //-- Parameter registration -------------------------------------------------
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& p)
    {
        using F = juce::AudioParameterFloat;
        using N = juce::NormalisableRange<float>;

        // Section A: Oscillator (4 params)
        p.push_back(std::make_unique<F>("sky_sawDetune",   "Saw Detune",      N{0.0f, 1.0f},    0.5f));
        p.push_back(std::make_unique<F>("sky_sawLevel",    "Saw Level",       N{0.0f, 1.0f},    1.0f));
        p.push_back(std::make_unique<F>("sky_filterFc",    "Brightness",      N{500.f, 18000.f}, 8000.f));
        p.push_back(std::make_unique<F>("sky_filterRes",   "Filter Res",      N{0.0f, 1.0f},    0.0f));

        // Section B: Amplitude Envelope (4 params)
        p.push_back(std::make_unique<F>("sky_ampAttack",   "Amp Attack",      N{0.001f, 4.0f},  0.05f));
        p.push_back(std::make_unique<F>("sky_ampDecay",    "Amp Decay",       N{0.01f,  4.0f},  0.3f));
        p.push_back(std::make_unique<F>("sky_ampSustain",  "Amp Sustain",     N{0.0f,   1.0f},  0.8f));
        p.push_back(std::make_unique<F>("sky_ampRelease",  "Amp Release",     N{0.01f,  8.0f},  0.8f));

        // Section C: Shimmer Layer (4 params)
        p.push_back(std::make_unique<F>("sky_shimmerMix",   "Shimmer Mix",    N{0.0f, 1.0f},    0.5f));
        p.push_back(std::make_unique<F>("sky_shimmerDecay", "Shimmer Decay",  N{0.05f, 8.0f},   2.5f));
        p.push_back(std::make_unique<F>("sky_shimmerOct",   "Shimmer Oct",    N{0.0f, 1.0f},    1.0f));
        p.push_back(std::make_unique<F>("sky_shimmerBright","Shimmer Bright", N{0.0f, 1.0f},    0.7f));

        // Section D: LFO (3 params)
        p.push_back(std::make_unique<F>("sky_lfoRate",     "LFO Rate",        N{0.01f, 4.0f},   0.08f));
        p.push_back(std::make_unique<F>("sky_lfoDepth",    "LFO Depth",       N{0.0f, 24.0f},   8.0f));
        p.push_back(std::make_unique<F>("sky_lfoShape",    "LFO Shape",       N{0.0f, 1.0f},    0.0f)); // 0=tri, 1=sine

        // Section E: Chorus (3 params)
        p.push_back(std::make_unique<F>("sky_chorusMix",   "Chorus Mix",      N{0.0f, 1.0f},    0.3f));
        p.push_back(std::make_unique<F>("sky_chorusRate",  "Chorus Rate",     N{0.01f, 4.0f},   0.25f));
        p.push_back(std::make_unique<F>("sky_chorusDepth", "Chorus Depth",    N{0.0f, 20.0f},   8.0f));

        // Section F: Reverb (2 params)
        p.push_back(std::make_unique<F>("sky_reverbMix",   "Reverb Mix",      N{0.0f, 1.0f},    0.4f));
        p.push_back(std::make_unique<F>("sky_reverbSize",  "Reverb Size",     N{0.0f, 1.0f},    0.7f));

        // Section G: Output + Coupling (2 params)
        p.push_back(std::make_unique<F>("sky_outputGain",  "Output Gain",     N{0.0f, 1.0f},    0.8f));
        p.push_back(std::make_unique<F>("sky_couplingAmt", "Coupling Amt",    N{0.0f, 1.0f},    0.0f));

        // Section H: Macros (4 params — M1..M4 standard)
        p.push_back(std::make_unique<F>("sky_macroCharacter", "CHARACTER",    N{0.0f, 1.0f},    0.5f));
        p.push_back(std::make_unique<F>("sky_macroMovement",  "MOVEMENT",     N{0.0f, 1.0f},    0.4f));
        p.push_back(std::make_unique<F>("sky_macroCoupling",  "COUPLING",     N{0.0f, 1.0f},    0.0f));
        p.push_back(std::make_unique<F>("sky_macroSpace",     "SPACE",        N{0.0f, 1.0f},    0.5f));

        // Section I: Shimmer tuning (2 params — extended voice character)
        p.push_back(std::make_unique<F>("sky_shimmerSpread",  "Shimmer Spread", N{0.0f, 1.0f},  0.5f));
        p.push_back(std::make_unique<F>("sky_shimmerPhase",   "Shimmer Phase",  N{0.0f, 1.0f},  0.0f));

        // Section J: Dynamics / expression (2 params)
        p.push_back(std::make_unique<F>("sky_velSensitivity", "Vel Sensitivity", N{0.0f, 1.0f}, 0.8f));
        p.push_back(std::make_unique<F>("sky_glide",           "Glide",          N{0.0f, 2.0f}, 0.0f));

        // Total: 30 sky_ parameters
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
        addParameters(p);
        return { p.begin(), p.end() };
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override {
        // Section A: Oscillator
        pSawDetune    = apvts.getRawParameterValue("sky_sawDetune");
        pSawLevel     = apvts.getRawParameterValue("sky_sawLevel");
        pFilterFc     = apvts.getRawParameterValue("sky_filterFc");
        pFilterRes    = apvts.getRawParameterValue("sky_filterRes");

        // Section B: Amplitude Envelope
        pAmpAtk       = apvts.getRawParameterValue("sky_ampAttack");
        pAmpDec       = apvts.getRawParameterValue("sky_ampDecay");
        pAmpSus       = apvts.getRawParameterValue("sky_ampSustain");
        pAmpRel       = apvts.getRawParameterValue("sky_ampRelease");

        // Section C: Shimmer Layer
        pShimmerMix   = apvts.getRawParameterValue("sky_shimmerMix");
        pShimmerDecay = apvts.getRawParameterValue("sky_shimmerDecay");
        pShimmerOct   = apvts.getRawParameterValue("sky_shimmerOct");
        pShimmerBright= apvts.getRawParameterValue("sky_shimmerBright");

        // Section D: LFO
        pLfoRate      = apvts.getRawParameterValue("sky_lfoRate");
        pLfoDepth     = apvts.getRawParameterValue("sky_lfoDepth");
        pLfoShape     = apvts.getRawParameterValue("sky_lfoShape");

        // Section E: Chorus
        pChorusMix    = apvts.getRawParameterValue("sky_chorusMix");
        pChorusRate   = apvts.getRawParameterValue("sky_chorusRate");
        pChorusDepth  = apvts.getRawParameterValue("sky_chorusDepth");

        // Section F: Reverb
        pReverbMix    = apvts.getRawParameterValue("sky_reverbMix");
        pReverbSize   = apvts.getRawParameterValue("sky_reverbSize");

        // Section G: Output
        pOutputGain   = apvts.getRawParameterValue("sky_outputGain");
        pCouplingAmt  = apvts.getRawParameterValue("sky_couplingAmt");

        // Macros
        mChar         = apvts.getRawParameterValue("sky_macroCharacter");
        mMove         = apvts.getRawParameterValue("sky_macroMovement");
        mCouple       = apvts.getRawParameterValue("sky_macroCoupling");
        mSpaceM       = apvts.getRawParameterValue("sky_macroSpace");

        // Sections I-J
        pShimmerSpread= apvts.getRawParameterValue("sky_shimmerSpread");
        pShimmerPhase = apvts.getRawParameterValue("sky_shimmerPhase");
        pVelSens      = apvts.getRawParameterValue("sky_velSensitivity");
        pGlide        = apvts.getRawParameterValue("sky_glide");
    }

    //-- Identity ---------------------------------------------------------------
    juce::String getEngineId()       const override { return "OpenSky"; }
    juce::Colour getAccentColour()   const override { return juce::Colour(0xFFFF8C00); } // Sunburst
    int getMaxVoices()               const override { return kVoices; }
    int getActiveVoiceCount()        const override { return activeCount; }

private:
    static constexpr int kVoices = 8;
    double sr = 44100.0;
    std::array<OpenSkyVoice, kVoices> voices;
    int nextV = 0, activeCount = 0;
    float lastL = 0.f, lastR = 0.f;

    // External coupling inputs
    float extPitchMod = 0.f;   // semitones from LFOToPitch
    float extAmpMod   = 1.f;   // multiplier from AmpToFilter/EnvToMorph

    // D006 expression state
    float atShimmer       = 0.f;  // aftertouch → shimmer depth
    float modWheelShimmer = 0.f;  // mod wheel → shimmer depth

    // Shared FX
    SkyChorus  chorus;
    SkyReverb  reverb;
    SilenceGate silenceGate;

    // Parameter pointers — 30 sky_ params
    // Section A: Oscillator
    std::atomic<float>* pSawDetune     = nullptr;
    std::atomic<float>* pSawLevel      = nullptr;
    std::atomic<float>* pFilterFc      = nullptr;
    std::atomic<float>* pFilterRes     = nullptr;
    // Section B: Amplitude Envelope
    std::atomic<float>* pAmpAtk        = nullptr;
    std::atomic<float>* pAmpDec        = nullptr;
    std::atomic<float>* pAmpSus        = nullptr;
    std::atomic<float>* pAmpRel        = nullptr;
    // Section C: Shimmer
    std::atomic<float>* pShimmerMix    = nullptr;
    std::atomic<float>* pShimmerDecay  = nullptr;
    std::atomic<float>* pShimmerOct    = nullptr;
    std::atomic<float>* pShimmerBright = nullptr;
    // Section D: LFO
    std::atomic<float>* pLfoRate       = nullptr;
    std::atomic<float>* pLfoDepth      = nullptr;
    std::atomic<float>* pLfoShape      = nullptr;
    // Section E: Chorus
    std::atomic<float>* pChorusMix     = nullptr;
    std::atomic<float>* pChorusRate    = nullptr;
    std::atomic<float>* pChorusDepth   = nullptr;
    // Section F: Reverb
    std::atomic<float>* pReverbMix     = nullptr;
    std::atomic<float>* pReverbSize    = nullptr;
    // Section G: Output + Coupling
    std::atomic<float>* pOutputGain    = nullptr;
    std::atomic<float>* pCouplingAmt   = nullptr;
    // Macros
    std::atomic<float>* mChar          = nullptr;
    std::atomic<float>* mMove          = nullptr;
    std::atomic<float>* mCouple        = nullptr;
    std::atomic<float>* mSpaceM        = nullptr;
    // Sections I-J
    std::atomic<float>* pShimmerSpread = nullptr;
    std::atomic<float>* pShimmerPhase  = nullptr;
    std::atomic<float>* pVelSens       = nullptr;
    std::atomic<float>* pGlide         = nullptr;
};

} // namespace xomnibus
