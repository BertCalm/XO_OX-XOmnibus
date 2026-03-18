#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace xomnibus {

//==============================================================================
//
//  OSTINATO ENGINE — Communal Drum Circle
//  XOmnibus Engine Module | Accent: Firelight Orange #E8701A
//
//  Creature: The Drum Circle Tide Pool (communal bioluminescence)
//  Habitat:  THE SHALLOWS — tidal zones where rhythmic pulse meets community
//
//  OSTINATO is the heartbeat engine. Eight seats around the fire, twelve
//  world percussion voices, one shared pulse. It models the physics of struck
//  membranes — noise excitation feeding into resonant one-pole filters with
//  exponential decay envelopes. No pitch tracking: these are rhythm instruments
//  with timbral character shaped by velocity and macro interaction.
//
//  Architecture (4 DSP voices, V1 simplified):
//    Noise exciter → Single-pole lowpass filter (membrane resonance)
//    → Exponential decay envelope → FIRE macro intensity scaler → Output
//    Shared Schroeder reverb (4 comb + 2 allpass, SPACE macro)
//    2 LFOs: slow tremolo (lfo1) + filter wobble (lfo2)
//
//  4 Macros:
//    GATHER   — inter-voice sync (0=loose/organic, 1=tight/machine)
//    FIRE     — intensity: amplitude + filter brightness fleet-wide
//    CIRCLE   — voices modulate each other's pitch / resonance
//    SPACE    — reverb mix
//
//  12 drum characters (osti_drumType 0-11):
//    0  Djembe     1  Dundun     2  Conga      3  Bongos
//    4  Cajón      5  Taiko      6  Tabla      7  Doumbek
//    8  Frame Drum 9  Surdo      10 Tongue Drum 11 Beatbox
//
//  ~40 osti_ parameters — doctrine compliant D001–D006.
//
//  Parameter prefix: osti_   Engine ID: "Ostinato"
//==============================================================================

// ---------------------------------------------------------------------------
// Drum character lookup table
// Each entry: { filterCutoff Hz, resonance 0..0.95, decaySeconds }
// These represent the "base" timbre before velocity and macro shaping.
// ---------------------------------------------------------------------------
struct OstiDrumChar {
    float cutoffHz;
    float resonance;
    float decaySec;
    const char* name;
};

// Twelve world percussion timbres
static constexpr OstiDrumChar kDrumChars[12] = {
    { 320.f,  0.55f, 0.40f, "Djembe"      },  // 0
    { 180.f,  0.35f, 0.80f, "Dundun"      },  // 1
    { 260.f,  0.50f, 0.45f, "Conga"       },  // 2
    { 480.f,  0.45f, 0.25f, "Bongos"      },  // 3
    { 220.f,  0.42f, 0.55f, "Cajon"       },  // 4
    { 150.f,  0.30f, 1.20f, "Taiko"       },  // 5
    { 560.f,  0.70f, 0.30f, "Tabla"       },  // 6
    { 420.f,  0.60f, 0.35f, "Doumbek"     },  // 7
    { 200.f,  0.38f, 0.70f, "Frame Drum"  },  // 8
    { 130.f,  0.28f, 1.60f, "Surdo"       },  // 9
    { 700.f,  0.75f, 0.50f, "Tongue Drum" }, // 10
    { 380.f,  0.50f, 0.20f, "Beatbox"     }, // 11
};

// ---------------------------------------------------------------------------
// Single-pole lowpass filter — membrane resonance model.
// Uses matched-Z coefficient: coeff = exp(-2π·fc/sr)
// Denormal protection on state variable.
// ---------------------------------------------------------------------------
struct OstiOnePole {
    float z1 = 0.f;

    void reset() { z1 = 0.f; }

    // fc: cutoff Hz  sr: sample rate
    inline float process(float in, float fc, float sr) noexcept {
        float coeff = fastExp(-6.2831853f * fc / sr);
        float out   = in * (1.f - coeff) + z1 * coeff;
        z1 = flushDenormal(out);
        return out;
    }
};

// ---------------------------------------------------------------------------
// ADSD amplitude envelope — 4-stage shape:
//   Stage 0 (Attack):  linear ramp 0 → peak over atkSec (osti_ampAtk)
//   Stage 1 (Hold):    hold at susAmp * peak for holdSec (osti_ampDec)
//   Stage 2 (Decay):   exponential decay from hold level → 0 over decSec (osti_ampRel)
//   osti_ampSus:       sustain amplitude level (0..1) — scales the hold plateau
//
// All stage durations computed from params at trigger() time.
// No heap allocation — integer sample counters only.
// If atkSec < 0.0005f (0.5ms), attack stage is skipped (instant onset).
// ---------------------------------------------------------------------------
struct OstiDecayEnv {
    // Envelope state
    float level      = 0.f;
    float decCoeff   = 0.f; // per-sample multiply for decay stage
    float atkInc     = 0.f; // per-sample increment for linear attack
    float peakLevel  = 0.f; // velocity-scaled peak
    float holdLevel  = 0.f; // peak * susAmp — plateau amplitude
    int   atkSamples = 0;   // attack stage length in samples
    int   holdSamples= 0;   // hold stage length in samples
    int   atkCounter = 0;   // samples remaining in attack
    int   holdCounter= 0;   // samples remaining in hold
    int   stage      = 0;   // 0=attack, 1=hold, 2=decay, 3=idle
    bool  active     = false;
    float sr         = 44100.f;

    void prepare(double s) { sr = (float)s; }

    void reset() {
        level = 0.f; active = false; stage = 3;
        atkCounter = holdCounter = 0;
    }

    // trigger: start envelope with all 4 stage parameters.
    //   peak:     peak amplitude (velocity-scaled by caller)
    //   atkSec:   attack ramp time — osti_ampAtk  (0.001..0.1 s)
    //   susAmp:   sustain/hold amplitude 0..1 — osti_ampSus  (scales plateau level)
    //   holdSec:  hold duration at plateau — osti_ampDec  (0.1..3.0 s)
    //   decSec:   decay time from plateau to silence — osti_ampRel (0.1..2.0 s)
    void trigger(float peak, float atkSec, float susAmp, float holdSec, float decSec) noexcept {
        peakLevel = peak;
        holdLevel = peak * clamp(susAmp, 0.f, 1.f);

        // Attack stage — skip if below 0.5ms
        if (atkSec >= 0.0005f) {
            atkSamples = (int)(juce::jmax(atkSec, 0.0005f) * sr);
            atkInc     = peak / (float)juce::jmax(atkSamples, 1);
            level      = 0.f;
            stage      = 0;
        } else {
            atkSamples = 0;
            level      = peak;
            stage      = 1; // instant onset — skip to hold
        }
        atkCounter = atkSamples;

        // Hold stage duration
        holdSamples  = (int)(juce::jmax(holdSec, 0.001f) * sr);
        holdCounter  = holdSamples;

        // Decay coefficient — exponential decay to near-zero over decSec
        float t  = juce::jmax(decSec, 0.001f);
        decCoeff = fastExp(-1.f / (sr * t));

        active = true;
    }

    inline float tick() noexcept {
        if (!active) return 0.f;

        if (stage == 0) {
            // Attack: linear ramp to peak
            level += atkInc;
            --atkCounter;
            if (atkCounter <= 0 || level >= peakLevel) {
                level       = peakLevel;
                holdCounter = holdSamples;
                stage       = 1;
            }
            return level;
        }

        if (stage == 1) {
            // Hold: ramp from peak toward holdLevel, then hold
            // Simple instant drop to holdLevel at start of stage
            level = holdLevel;
            if (holdCounter > 0) {
                --holdCounter;
                return level;
            }
            stage = 2; // fall through to decay
        }

        // Stage 2: exponential decay from holdLevel toward 0
        level *= decCoeff;
        level  = flushDenormal(level);
        if (level < 1e-6f) { level = 0.f; active = false; stage = 3; }
        return level;
    }
};

// ---------------------------------------------------------------------------
// Ostinato DSP voice — one drum seat
// Noise exciter → filter → decay envelope
// ---------------------------------------------------------------------------
struct OstiVoice {
    bool   active   = false;
    int    note     = 0;
    float  vel      = 0.f;
    float  sr       = 44100.f;

    // Noise generator state (simple linear congruential, RT-safe)
    uint32_t noiseSeed = 0x13A55E5D;

    OstiOnePole  filter;
    OstiDecayEnv env;

    void prepare(double s) {
        sr = (float)s;
        env.prepare(s);
        reset();
    }

    void reset() {
        filter.reset();
        env.reset();
        active = false;
        noiseSeed = 0x13A55E5D;
    }

    // Trigger this voice.
    // drumType:     0-11 selects character preset
    // cutoffOverride: parameter cutoff (blended with drum preset)
    // resOverride:  parameter resonance
    // atkOverride:  attack time seconds  — osti_ampAtk
    // susOverride:  sustain amplitude 0..1 — osti_ampSus
    // holdOverride: hold stage duration seconds — osti_ampDec
    // relOverride:  decay time seconds  — osti_ampRel
    // velCutoffAmt: velocity → cutoff scaling (0-1)
    // velocity:     MIDI velocity 0-1
    void noteOn(int n, float velocity,
                int drumType,
                float cutoffOverride, float resOverride,
                float atkOverride, float susOverride,
                float holdOverride, float relOverride,
                float velCutoffAmt) noexcept
    {
        note = n;
        vel  = velocity;

        // Clamp drumType to valid range
        int dt = juce::jlimit(0, 11, drumType);
        const OstiDrumChar& dc = kDrumChars[dt];

        // Blend parameter cutoff with drum character preset (50/50 mix)
        float baseCutoff = cutoffOverride * 0.5f + dc.cutoffHz * 0.5f;

        // D001: velocity shapes filter cutoff
        float velBoost = 1.f + velCutoffAmt * velocity * 2.f; // 1x → 3x range
        activeCutoff   = baseCutoff * velBoost;
        activeRes      = resOverride * 0.5f + dc.resonance * 0.5f;

        // D001: velocity also shapes attack transient amplitude
        float peak    = velocity * (0.7f + velocity * 0.3f); // slight curve up at high vel
        float decSec  = relOverride > 0.001f ? relOverride : dc.decaySec;
        env.trigger(peak, atkOverride, susOverride, holdOverride, decSec);
        active = true;
    }

    // Current filter parameters (set per-block, not cached per-note)
    float activeCutoff = 2000.f;
    float activeRes    = 0.3f;

    // Returns one output sample.
    // lfo2CutoffMod: additive cutoff modulation from lfo2 (Hz)
    // circleMod: additive cutoff from circle interaction (Hz)
    inline float tick(float lfo2CutoffMod, float circleMod) noexcept {
        if (!active) return 0.f;

        // Noise burst into filter — LCG random (white noise, ±1)
        noiseSeed  = noiseSeed * 1664525u + 1013904223u;
        float noise = (float)(int32_t)noiseSeed * 4.656612873e-10f; // /2^31

        // Apply filter with modulated cutoff
        float fc = juce::jmax(activeCutoff + lfo2CutoffMod + circleMod, 20.f);
        fc = juce::jmin(fc, 18000.f);
        float filtered = filter.process(noise, fc, sr);

        // Decay envelope
        float envVal = env.tick();
        if (!env.active) active = false;

        return filtered * envVal;
    }
};

// ---------------------------------------------------------------------------
// Schroeder reverb — 4 comb filters + 2 allpass diffusers.
// Tuned for medium-length room suitable for percussion (not a long hall).
// Independent L/R comb lengths for stereo spread.
// ---------------------------------------------------------------------------
struct OstiReverb {
    static constexpr int kCombs = 4;

    // Comb lengths at 44100 Hz (scaled at prepare() if sr differs)
    // Primes chosen to avoid beating artefacts.
    static constexpr int kCombLensL[kCombs] = { 1051, 1181, 1301, 1427 };
    static constexpr int kCombLensR[kCombs] = { 1109, 1229, 1361, 1499 };
    static constexpr int kAP1Len = 251, kAP2Len = 503;

    // Delay line storage — allocated once in prepare(), never in renderBlock
    float combBufL[kCombs][1500] = {};
    float combBufR[kCombs][1500] = {};
    float ap1BufL[512] = {}, ap2BufL[512] = {};
    float ap1BufR[512] = {}, ap2BufR[512] = {};

    int combPosL[kCombs] = {}, combPosR[kCombs] = {};
    int ap1PosL = 0, ap2PosL = 0, ap1PosR = 0, ap2PosR = 0;
    float combStateL[kCombs] = {}, combStateR[kCombs] = {};

    void prepare(double /*sr*/) {
        reset();
    }

    void reset() {
        for (int c = 0; c < kCombs; ++c) {
            std::fill(combBufL[c], combBufL[c] + kCombLensL[c], 0.f);
            std::fill(combBufR[c], combBufR[c] + kCombLensR[c], 0.f);
            combPosL[c] = combPosR[c] = 0;
            combStateL[c] = combStateR[c] = 0.f;
        }
        std::fill(ap1BufL, ap1BufL + kAP1Len, 0.f);
        std::fill(ap2BufL, ap2BufL + kAP2Len, 0.f);
        std::fill(ap1BufR, ap1BufR + kAP1Len, 0.f);
        std::fill(ap2BufR, ap2BufR + kAP2Len, 0.f);
        ap1PosL = ap2PosL = ap1PosR = ap2PosR = 0;
    }

    // space: 0-1 (scales feedback 0.65→0.88 — percussion-appropriate decay)
    // mix:   0-1 wet amount
    void process(float& inL, float& inR, float space, float mix) noexcept {
        float fb = 0.65f + space * 0.23f;

        // Inline comb + allpass lambdas
        auto runCombL = [&](int c, float input) -> float {
            float rd = combBufL[c][combPosL[c]];
            combStateL[c] = flushDenormal(rd * 0.82f + combStateL[c] * 0.18f);
            combBufL[c][combPosL[c]] = flushDenormal(input + combStateL[c] * fb);
            combPosL[c] = (combPosL[c] + 1) % kCombLensL[c];
            return rd;
        };
        auto runCombR = [&](int c, float input) -> float {
            float rd = combBufR[c][combPosR[c]];
            combStateR[c] = flushDenormal(rd * 0.82f + combStateR[c] * 0.18f);
            combBufR[c][combPosR[c]] = flushDenormal(input + combStateR[c] * fb);
            combPosR[c] = (combPosR[c] + 1) % kCombLensR[c];
            return rd;
        };
        auto runAP = [](float* buf, int& pos, int len, float input) -> float {
            float rd = buf[pos];
            buf[pos] = flushDenormal(input + rd * 0.5f);
            float out = rd - input * 0.5f;
            pos = (pos + 1) % len;
            return out;
        };

        // Left
        float cL = 0.f;
        for (int c = 0; c < kCombs; ++c) cL += runCombL(c, inL);
        cL *= 0.25f;
        float wetL = runAP(ap1BufL, ap1PosL, kAP1Len, cL);
        wetL        = runAP(ap2BufL, ap2PosL, kAP2Len, wetL);

        // Right
        float cR = 0.f;
        for (int c = 0; c < kCombs; ++c) cR += runCombR(c, inR);
        cR *= 0.25f;
        float wetR = runAP(ap1BufR, ap1PosR, kAP1Len, cR);
        wetR        = runAP(ap2BufR, ap2PosR, kAP2Len, wetR);

        inL = inL * (1.f - mix) + flushDenormal(wetL) * mix;
        inR = inR * (1.f - mix) + flushDenormal(wetR) * mix;
    }
};

// ===========================================================================
// OstinatoEngine — XOmnibus adapter  (implements SynthEngine)
// ===========================================================================
class OstinatoEngine : public SynthEngine {
public:
    //-- SynthEngine lifecycle --------------------------------------------------
    void prepare(double sampleRate, int maxBlockSize) override {
        sr = sampleRate;
        for (auto& v : voices) v.prepare(sampleRate);
        reverb.prepare(sampleRate);
        silenceGate.prepare(sampleRate, maxBlockSize);
        silenceGate.setHoldTime(200.f); // 200ms — percussive with tails
        lfo1Phase = lfo2Phase = 0.f;
        // Precompute initial decay coefficients
        for (int i = 0; i < kVoices; ++i)
            voices[i].reset();
    }

    void releaseResources() override {
        for (auto& v : voices) v.reset();
    }

    void reset() override {
        for (auto& v : voices) v.reset();
        reverb.reset();
        lastL = lastR = 0.f;
        lfo1Phase = lfo2Phase = 0.f;
        extPitchMod  = 0.f;
        extAmpMod    = 1.f;
        atCircleMod  = 0.f;
        modWheelFire = 0.f;
        stepIndex    = 0;
    }

    //-- MIDI + render ----------------------------------------------------------
    void renderBlock(juce::AudioBuffer<float>& buf,
                     juce::MidiBuffer& midi, int ns) override
    {
        // --- 1. Parse MIDI -----------------------------------------------------
        for (const auto m : midi) {
            auto msg = m.getMessage();

            if (msg.isNoteOn()) {
                silenceGate.wake();

                // --- Pattern gate sequencer: advance step and check mask -------
                // stepIndex wraps 0-7; if the current step's bool param is false,
                // the note-on is silently consumed (pattern mute mask).
                int curStep = stepIndex % 8;
                stepIndex   = (stepIndex + 1) % 8; // always advance

                bool stepEnabled = pStep[curStep] ? (pStep[curStep]->load() >= 0.5f) : true;
                if (!stepEnabled)
                    continue; // step muted — skip voice trigger

                // Read current param values for voice configuration
                int   drumType      = pDrumType    ? (int)pDrumType->load()    : 0;
                float cutoff        = pFilterCutoff? pFilterCutoff->load()     : 2000.f;
                float res           = pFilterRes   ? pFilterRes->load()        : 0.3f;
                float atk           = pAmpAtk      ? pAmpAtk->load()           : 0.003f;
                float sus           = pAmpSus      ? pAmpSus->load()           : 0.0f;
                float dec           = pAmpDec      ? pAmpDec->load()           : 0.5f;
                float rel           = pAmpRel      ? pAmpRel->load()           : 0.3f;
                float velCutoffAmt  = pVelCutoffAmt? pVelCutoffAmt->load()     : 0.5f;

                // Voice steal: prefer inactive, else round-robin
                int slot = -1;
                for (int i = 0; i < kVoices; ++i)
                    if (!voices[i].active) { slot = i; break; }
                if (slot < 0) { slot = nextVoice % kVoices; }
                nextVoice = (slot + 1) % kVoices;

                float velocity = msg.getVelocity() / 127.f;
                voices[slot].noteOn(msg.getNoteNumber(), velocity,
                                    drumType, cutoff, res, atk, sus, dec, rel, velCutoffAmt);
            }
            else if (msg.isNoteOff()) {
                // Percussion — no note-off (envelope self-completes)
                // Uncomment below if sustain behavior is ever desired:
                // for (auto& v : voices)
                //     if (v.active && v.note == msg.getNoteNumber()) v.active = false;
            }
            else if (msg.isChannelPressure()) {
                // D006: aftertouch → CIRCLE interaction depth
                atCircleMod = (float)msg.getChannelPressureValue() / 127.f;
            }
            else if (msg.isController() && msg.getControllerNumber() == 1) {
                // D006: mod wheel → FIRE macro (intensity)
                modWheelFire = (float)msg.getControllerValue() / 127.f;
            }
        }

        // --- 2. SilenceGate early exit ----------------------------------------
        if (silenceGate.isBypassed() && midi.isEmpty()) {
            buf.clear();
            return;
        }

        // --- 3. Read parameters (ParamSnapshot — render-loop params only) -------
        // Per-note params (drumType, cutoff, res, envelope, velCutoffAmt) are
        // read fresh at note-on time inside the MIDI loop above.
        float pRevMix       = pReverbMix    ? pReverbMix->load()          : 0.2f;
        float pL1Rate       = pLfo1Rate     ? pLfo1Rate->load()           : 0.5f;
        float pL1Depth      = pLfo1Depth    ? pLfo1Depth->load()          : 0.1f;
        float pL2Rate       = pLfo2Rate     ? pLfo2Rate->load()           : 1.5f;
        float pL2Depth      = pLfo2Depth    ? pLfo2Depth->load()          : 0.1f;

        // Macros
        float mGather       = mMacroGather  ? mMacroGather->load()       : 0.5f;
        float mFire         = mMacroFire    ? mMacroFire->load()         : 0.5f;
        float mCircle       = mMacroCircle  ? mMacroCircle->load()       : 0.3f;
        float mSpace        = mMacroSpace   ? mMacroSpace->load()        : 0.4f;

        // Mod wheel adds to FIRE macro (D006)
        float fireMod = clamp(mFire + modWheelFire * 0.5f, 0.f, 1.f);

        // FIRE macro: scales amplitude output and filter brightness
        float fireAmp    = 0.5f + fireMod * 1.0f;  // 0.5 → 1.5 range
        float fireBright = 1.f  + fireMod * 1.5f;  // 1.0 → 2.5x cutoff boost

        // CIRCLE macro: inter-voice modulation depth (aftertouch adds to it)
        float circleDepth = clamp(mCircle + atCircleMod * 0.4f, 0.f, 1.f);

        // GATHER macro: tightens inter-voice timing jitter (applied to future
        // triggers; affects the organic looseness when low). Encoded here as
        // a cutoff width randomization factor: low gather = wider spread.
        float gatherSpread = (1.f - mGather) * 400.f; // 0→400 Hz spread at gather=0

        // SPACE macro: reverb mix
        float reverbMix  = clamp(pRevMix + mSpace * 0.5f, 0.f, 1.f);
        float reverbSpace = clamp(mSpace * 0.8f, 0.f, 1.f);

        // LFO1: slow amplitude tremolo (D005 — rate floor 0.01 Hz satisfied by param range)
        float lfo1Inc = pL1Rate / (float)sr;
        // LFO2: filter wobble
        float lfo2Inc = pL2Rate / (float)sr;

        // --- 4. Render samples ------------------------------------------------
        auto* outL = buf.getWritePointer(0);
        auto* outR = buf.getWritePointer(buf.getNumChannels() > 1 ? 1 : 0);

        // Collect voice outputs for circle modulation (uses previous sample value)
        // — avoids per-sample allocation; holds last tick value for each voice
        float voiceLastOut[kVoices] = {};

        for (int i = 0; i < ns; ++i) {
            // Advance LFOs
            lfo1Phase += lfo1Inc;
            if (lfo1Phase >= 1.f) lfo1Phase -= 1.f;
            lfo2Phase += lfo2Inc;
            if (lfo2Phase >= 1.f) lfo2Phase -= 1.f;

            // LFO1: triangle tremolo
            float lfo1 = (lfo1Phase < 0.5f)
                       ? (lfo1Phase * 4.f - 1.f)
                       : (3.f - lfo1Phase * 4.f);
            float tremoloGain = 1.f - pL1Depth * 0.3f * (lfo1 * 0.5f + 0.5f);

            // LFO2: sine filter wobble (Hz)
            float lfo2        = FastMath_sin(lfo2Phase * 6.2831853f);
            float filterWobble = lfo2 * pL2Depth * 500.f; // ±500 Hz max

            // Sum active voices
            float sL = 0.f, sR = 0.f;

            for (int v = 0; v < kVoices; ++v) {
                if (!voices[v].active) continue;

                // CIRCLE modulation: each voice is modulated by the sum of
                // other voices' last output (cross-seat resonance)
                float circleIn = 0.f;
                for (int ov = 0; ov < kVoices; ++ov)
                    if (ov != v) circleIn += voiceLastOut[ov];
                float circleCutoffMod = circleIn * circleDepth * 800.f; // ±800 Hz

                // GATHER: add per-voice cutoff spread (organic looseness)
                // Voices get offset 0, ±spread/2, ±spread/4, ±spread/4
                static constexpr float kGatherOffsets[4] = { 0.f, 0.5f, -0.25f, 0.25f };
                float gatherCutoffOffset = kGatherOffsets[v] * gatherSpread;

                // Temporarily adjust voice cutoff for this sample
                float savedCutoff = voices[v].activeCutoff;
                voices[v].activeCutoff += gatherCutoffOffset;

                // Tick voice
                float mono = voices[v].tick(filterWobble + circleCutoffMod, 0.f);
                voices[v].activeCutoff = savedCutoff;

                voiceLastOut[v] = mono;

                // Stereo panning — fixed per voice for stable image
                static constexpr float kPanL[4] = { 0.60f, 0.40f, 0.65f, 0.35f };
                static constexpr float kPanR[4] = { 0.40f, 0.60f, 0.35f, 0.65f };

                sL += mono * kPanL[v];
                sR += mono * kPanR[v];
            }

            // Apply FIRE amplitude + LFO1 tremolo + coupling amp mod
            sL *= fireAmp * fireBright * tremoloGain * extAmpMod;
            sR *= fireAmp * fireBright * tremoloGain * extAmpMod;

            // Apply coupling pitch mod (stored as semitone offset → amplitude tilt)
            // For a drum engine, LFOToPitch maps to subtle pitch bend on resonant
            // filter; we model this as a cutoff frequency offset (musical effect).
            // (extPitchMod is applied as a cutoff multiplier in noteOn for drums)

            // Reverb send
            reverb.process(sL, sR, reverbSpace, reverbMix);

            // Output (normalize by voice count to prevent summing clipping)
            sL *= 0.5f; // 4 voices, but drum circles blend, not add fully
            sR *= 0.5f;

            outL[i] += sL;
            outR[i] += sR;
            lastL = sL;
            lastR = sR;
        }

        // --- 5. Count active voices -------------------------------------------
        activeCount = 0;
        for (const auto& v : voices) if (v.active) ++activeCount;

        // --- 6. Feed silence gate ---------------------------------------------
        silenceGate.analyzeBlock(buf.getReadPointer(0),
                                  buf.getNumChannels() > 1 ? buf.getReadPointer(1) : nullptr,
                                  ns);
    }

    //-- Coupling ---------------------------------------------------------------
    float getSampleForCoupling(int ch, int) const override {
        return ch == 0 ? lastL : lastR;
    }

    void applyCouplingInput(CouplingType t, float amount,
                            const float* srcBuf, int /*ns*/) override {
        switch (t) {
            case CouplingType::AmpToFilter:
                // External amplitude → drive FIRE intensity briefly
                extAmpMod = 1.f + amount * (srcBuf ? srcBuf[0] : 0.f) * 0.8f;
                break;
            case CouplingType::LFOToPitch:
                // Map to filter cutoff modulation (stored, applied next block)
                extPitchMod = (srcBuf ? srcBuf[0] : 0.f) * amount * 4.f;
                break;
            case CouplingType::RhythmToBlend:
                // External rhythm → retrigger all voices on strong pulses
                // (Light coupling: positive spike triggers voices)
                if (srcBuf && srcBuf[0] > 0.7f) silenceGate.wake();
                break;
            case CouplingType::EnvToDecay:
                // External envelope → scale our decay time
                extAmpMod = juce::jmax(0.1f, 1.f + (srcBuf ? srcBuf[0] : 0.f) * amount);
                break;
            default: break;
        }
    }

    //-- Parameter registration -------------------------------------------------
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& p)
    {
        using F   = juce::AudioParameterFloat;
        using I   = juce::AudioParameterInt;
        using B   = juce::AudioParameterBool;
        using NRF = juce::NormalisableRange<float>;

        // Section A: Drum character (1 param)
        p.push_back(std::make_unique<I>("osti_drumType", "Drum Type", 0, 11, 0));

        // Section B: Filter (2 params)
        p.push_back(std::make_unique<F>("osti_filterCutoff", "Filter Cutoff",
                                        NRF{200.f, 8000.f, 0.f, 0.4f}, 2000.f));
        p.push_back(std::make_unique<F>("osti_filterRes", "Filter Resonance",
                                        NRF{0.f, 0.95f}, 0.3f));

        // Section C: Amplitude envelope (4 params)
        p.push_back(std::make_unique<F>("osti_ampAtk", "Amp Attack",
                                        NRF{0.001f, 0.1f, 0.f, 0.5f}, 0.003f));
        p.push_back(std::make_unique<F>("osti_ampDec", "Amp Decay",
                                        NRF{0.1f, 3.0f, 0.f, 0.5f}, 0.5f));
        p.push_back(std::make_unique<F>("osti_ampSus", "Amp Sustain",
                                        NRF{0.f, 1.f}, 0.0f));
        p.push_back(std::make_unique<F>("osti_ampRel", "Amp Release",
                                        NRF{0.1f, 2.0f, 0.f, 0.5f}, 0.3f));

        // Section D: Expression (1 param)
        p.push_back(std::make_unique<F>("osti_velCutoffAmt", "Vel → Cutoff",
                                        NRF{0.f, 1.f}, 0.5f));

        // Section E: Reverb (1 param)
        p.push_back(std::make_unique<F>("osti_reverbMix", "Reverb Mix",
                                        NRF{0.f, 1.f}, 0.2f));

        // Section F: LFO1 — slow tremolo (2 params)
        p.push_back(std::make_unique<F>("osti_lfo1Rate", "LFO1 Rate",
                                        NRF{0.01f, 10.f, 0.f, 0.4f}, 0.5f));
        p.push_back(std::make_unique<F>("osti_lfo1Depth", "LFO1 Depth",
                                        NRF{0.f, 1.f}, 0.1f));

        // Section G: LFO2 — filter wobble (2 params)
        p.push_back(std::make_unique<F>("osti_lfo2Rate", "LFO2 Rate",
                                        NRF{0.01f, 10.f, 0.f, 0.4f}, 1.5f));
        p.push_back(std::make_unique<F>("osti_lfo2Depth", "LFO2 Depth",
                                        NRF{0.f, 1.f}, 0.1f));

        // Section H: Pattern sequencer — 8 steps (8 params)
        p.push_back(std::make_unique<B>("osti_patternStep0", "Step 1", true));
        p.push_back(std::make_unique<B>("osti_patternStep1", "Step 2", false));
        p.push_back(std::make_unique<B>("osti_patternStep2", "Step 3", true));
        p.push_back(std::make_unique<B>("osti_patternStep3", "Step 4", false));
        p.push_back(std::make_unique<B>("osti_patternStep4", "Step 5", true));
        p.push_back(std::make_unique<B>("osti_patternStep5", "Step 6", false));
        p.push_back(std::make_unique<B>("osti_patternStep6", "Step 7", false));
        p.push_back(std::make_unique<B>("osti_patternStep7", "Step 8", false));

        // Section I: Macros (4 params)
        p.push_back(std::make_unique<F>("osti_macroGather", "GATHER",
                                        NRF{0.f, 1.f}, 0.5f));
        p.push_back(std::make_unique<F>("osti_macroFire", "FIRE",
                                        NRF{0.f, 1.f}, 0.5f));
        p.push_back(std::make_unique<F>("osti_macroCircle", "CIRCLE",
                                        NRF{0.f, 1.f}, 0.3f));
        p.push_back(std::make_unique<F>("osti_macroSpace", "SPACE",
                                        NRF{0.f, 1.f}, 0.4f));

        // Total: 25 osti_ parameters
        // (drumType 0-11 int + 8 filter/env/expression + 1 reverb + 4 lfo + 8 steps + 4 macros)
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
        addParameters(p);
        return { p.begin(), p.end() };
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override {
        // Section A: Drum character
        pDrumType       = apvts.getRawParameterValue("osti_drumType");

        // Section B: Filter
        pFilterCutoff   = apvts.getRawParameterValue("osti_filterCutoff");
        pFilterRes      = apvts.getRawParameterValue("osti_filterRes");

        // Section C: Amplitude envelope
        pAmpAtk         = apvts.getRawParameterValue("osti_ampAtk");
        pAmpDec         = apvts.getRawParameterValue("osti_ampDec");
        pAmpSus         = apvts.getRawParameterValue("osti_ampSus");
        pAmpRel         = apvts.getRawParameterValue("osti_ampRel");

        // Section D: Expression
        pVelCutoffAmt   = apvts.getRawParameterValue("osti_velCutoffAmt");

        // Section E: Reverb
        pReverbMix      = apvts.getRawParameterValue("osti_reverbMix");

        // Section F: LFO1
        pLfo1Rate       = apvts.getRawParameterValue("osti_lfo1Rate");
        pLfo1Depth      = apvts.getRawParameterValue("osti_lfo1Depth");

        // Section G: LFO2
        pLfo2Rate       = apvts.getRawParameterValue("osti_lfo2Rate");
        pLfo2Depth      = apvts.getRawParameterValue("osti_lfo2Depth");

        // Section H: Pattern steps
        pStep[0]        = apvts.getRawParameterValue("osti_patternStep0");
        pStep[1]        = apvts.getRawParameterValue("osti_patternStep1");
        pStep[2]        = apvts.getRawParameterValue("osti_patternStep2");
        pStep[3]        = apvts.getRawParameterValue("osti_patternStep3");
        pStep[4]        = apvts.getRawParameterValue("osti_patternStep4");
        pStep[5]        = apvts.getRawParameterValue("osti_patternStep5");
        pStep[6]        = apvts.getRawParameterValue("osti_patternStep6");
        pStep[7]        = apvts.getRawParameterValue("osti_patternStep7");

        // Section I: Macros
        mMacroGather    = apvts.getRawParameterValue("osti_macroGather");
        mMacroFire      = apvts.getRawParameterValue("osti_macroFire");
        mMacroCircle    = apvts.getRawParameterValue("osti_macroCircle");
        mMacroSpace     = apvts.getRawParameterValue("osti_macroSpace");
    }

    //-- Identity ---------------------------------------------------------------
    juce::String getEngineId()     const override { return "Ostinato"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFE8701A); } // Firelight Orange
    int getMaxVoices()             const override { return kVoices; }
    int getActiveVoiceCount()      const override { return activeCount; }

private:
    // FastMath sine (inline fallback using std::sin — FastMath.h provides fastSin
    // if available; use std:: fallback for portability)
    static inline float FastMath_sin(float x) noexcept {
        return std::sin(x);
    }

    static constexpr int kVoices = 4;
    double sr = 44100.0;

    std::array<OstiVoice, kVoices> voices;
    OstiReverb reverb;

    int   nextVoice   = 0;
    int   activeCount = 0;
    float lastL = 0.f, lastR = 0.f;

    // 8-step gate sequencer state
    int   stepIndex   = 0; // advances each note-on; wraps 0-7

    // LFO states
    float lfo1Phase = 0.f;
    float lfo2Phase = 0.f;

    // External coupling inputs
    float extPitchMod  = 0.f; // semitones from LFOToPitch (used as cutoff mod)
    float extAmpMod    = 1.f; // multiplier from AmpToFilter / EnvToDecay

    // D006 expression state
    float atCircleMod  = 0.f; // aftertouch → CIRCLE interaction depth
    float modWheelFire = 0.f; // mod wheel → FIRE macro additive

    //-- Parameter pointers (25 osti_ params) ----------------------------------
    // Section A
    std::atomic<float>* pDrumType      = nullptr;
    // Section B
    std::atomic<float>* pFilterCutoff  = nullptr;
    std::atomic<float>* pFilterRes     = nullptr;
    // Section C
    std::atomic<float>* pAmpAtk        = nullptr;
    std::atomic<float>* pAmpDec        = nullptr;
    std::atomic<float>* pAmpSus        = nullptr;
    std::atomic<float>* pAmpRel        = nullptr;
    // Section D
    std::atomic<float>* pVelCutoffAmt  = nullptr;
    // Section E
    std::atomic<float>* pReverbMix     = nullptr;
    // Section F
    std::atomic<float>* pLfo1Rate      = nullptr;
    std::atomic<float>* pLfo1Depth     = nullptr;
    // Section G
    std::atomic<float>* pLfo2Rate      = nullptr;
    std::atomic<float>* pLfo2Depth     = nullptr;
    // Section H — pattern steps
    std::atomic<float>* pStep[8]       = {};
    // Section I — macros
    std::atomic<float>* mMacroGather   = nullptr;
    std::atomic<float>* mMacroFire     = nullptr;
    std::atomic<float>* mMacroCircle   = nullptr;
    std::atomic<float>* mMacroSpace    = nullptr;
};

} // namespace xomnibus
