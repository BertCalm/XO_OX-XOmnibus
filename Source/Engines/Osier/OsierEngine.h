#pragma once
//==============================================================================
//
//  OsierEngine.h — XOsier | "The Herb Garden"
//  XO_OX Designs | XOlokun Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOsier is the carefully tended herb garden — intimate, precise, every
//      plant placed with intention. Four voices, each with a named role in
//      the quartet (soprano, alto, tenor, bass), each with slightly different
//      tonal character. The companion planting mechanism: voices that sound
//      together develop harmonic affinity over time.
//
//  ENGINE CONCEPT:
//      A 4-voice chamber string quartet synthesizer. Each voice has named
//      roles with distinct tonal character: Soprano (bright, narrow vibrato),
//      Alto (warm, medium vibrato), Tenor (neutral, wider vibrato), Bass
//      (dark, slow vibrato). Companion planting: voices that play together
//      develop mutual harmonic influence over the session.
//
//  DSP ARCHITECTURE:
//      Per-voice: 2 detuned sawtooth oscillators (thinner than Orchard's 4)
//      → per-role tonal shaping (EQ/filter character) → shared filter →
//      amp envelope. Companion planting via frequency-domain pitch attractor
//      at control rate. Each voice has slightly different filter character
//      to represent the instrument's tonal role.
//
//  GARDEN QUAD ROLE:
//      Later intermediate — chamber strings emerging as conditions stabilize.
//      The quartet needs warmth (W > threshold) before it sounds fully
//      itself. The herb garden where every plant has a purpose.
//
//  Accent: Willow Green #6B8E23 (avoiding Sage #87AE73 conflict with XOhm)
//  Parameter prefix: osier_
//  Voices: 4 (fixed — one per quartet role)
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/FilterEnvelope.h"
#include "../../DSP/GlideProcessor.h"
#include "../../DSP/ParameterSmoother.h"
#include "../../DSP/VoiceAllocator.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/GardenAccumulators.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace xolokun {

//==============================================================================
// Quartet role identity — each voice has distinct tonal character
//==============================================================================
enum class OsierQuartetRole : int
{
    Soprano = 0,  // Bright, narrow vibrato, highest register
    Alto    = 1,  // Warm, medium vibrato, mezzo register
    Tenor   = 2,  // Neutral, wider vibrato, middle register
    Bass    = 3   // Dark, slow vibrato, lowest register — the nutrient provider
};

struct OsierQuartetRoleConfig
{
    float vibratoRateMult;     // multiplier on global vibrato rate
    float vibratoDepthMult;    // multiplier on global vibrato depth
    float filterBiasCents;     // filter cutoff offset in Hz
    float brightnessOffset;    // brightness character offset [-1, 1]
    float detuneCents;         // slight per-role detuning for individuality
};

static constexpr OsierQuartetRoleConfig kRoleConfigs[4] = {
    { 1.1f,  0.7f,  800.0f,  0.3f,  1.5f },   // Soprano: fast, narrow, bright
    { 1.0f,  0.9f,  200.0f,  0.0f,  0.5f },   // Alto: medium, warm
    { 0.9f,  1.1f, -200.0f, -0.1f, -0.5f },   // Tenor: wider, neutral
    { 0.7f,  0.8f, -600.0f, -0.3f, -1.5f }    // Bass: slow, dark — the root
};

//==============================================================================
// OsierSaw — lightweight PolyBLEP sawtooth (same as Orchard)
//==============================================================================
struct OsierSaw
{
    void setFrequency (float freqHz, float sampleRate) noexcept
    {
        phaseInc = freqHz / sampleRate;
        if (phaseInc > 0.49f) phaseInc = 0.49f;
    }

    float process() noexcept
    {
        float naive = 2.0f * phase - 1.0f;
        float t = phase;
        float blep = 0.0f;
        if (t < phaseInc)
        {
            t /= phaseInc;
            blep = t + t - t * t - 1.0f;
        }
        else if (t > 1.0f - phaseInc)
        {
            t = (t - 1.0f) / phaseInc;
            blep = t * t + t + t + 1.0f;
        }
        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;
        return naive - blep;
    }

    void reset (float startPhase = 0.0f) noexcept { phase = startPhase; }

    float phase = 0.0f;
    float phaseInc = 0.01f;
};

//==============================================================================
// CompanionPlanting — voices that play together develop harmonic affinity
//==============================================================================
struct CompanionPlanting
{
    // Per-voice-pair affinity accumulator: rises when both voices are active,
    // decays slowly when either is silent. Higher affinity = tighter tuning.
    float affinity[6] = {};  // C(4,2) = 6 pairs

    void update (const bool* voiceActive, float companionStrength,
                 float blockSizeSec) noexcept
    {
        int pairIdx = 0;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = i + 1; j < 4; ++j)
            {
                if (voiceActive[i] && voiceActive[j])
                    affinity[pairIdx] += companionStrength * blockSizeSec * 0.5f;
                else
                    affinity[pairIdx] -= blockSizeSec * 0.02f;

                affinity[pairIdx] = std::clamp (affinity[pairIdx], 0.0f, 1.0f);
                pairIdx++;
            }
        }
    }

    /// Get total affinity for a specific voice (sum of all pair affinities it's in)
    float getVoiceAffinity (int voiceIdx) const noexcept
    {
        float total = 0.0f;
        int pairIdx = 0;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = i + 1; j < 4; ++j)
            {
                if (i == voiceIdx || j == voiceIdx)
                    total += affinity[pairIdx];
                pairIdx++;
            }
        }
        return total / 3.0f;  // normalize by max pairs per voice
    }

    void reset() noexcept
    {
        for (auto& a : affinity) a = 0.0f;
    }
};

//==============================================================================
// OsierVoice — chamber quartet voice with named role
//==============================================================================
struct OsierVoice
{
    static constexpr int kNumOscs = 2;

    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    float velocity = 0.0f;
    OsierQuartetRole role = OsierQuartetRole::Soprano;

    GlideProcessor glide;
    std::array<OsierSaw, kNumOscs> oscs;
    CytomicSVF filter;
    CytomicSVF toneShaper;  // per-role tonal character
    FilterEnvelope ampEnv;
    FilterEnvelope filterEnv;
    StandardLFO vibratoLFO;
    StandardLFO lfo1, lfo2;

    // Growth Mode
    bool growthMode = false;
    float growthPhase = 0.0f;
    float growthTimer = 0.0f;
    float growthDuration = 20.0f;

    // Dormancy
    float dormancyPitchCents = 0.0f;

    // Companion pitch influence (cents of pull toward other voices)
    float companionPitchCents = 0.0f;

    float panL = 0.707f, panR = 0.707f;

    void reset() noexcept
    {
        active = false;
        velocity = 0.0f;
        growthMode = false;
        growthPhase = 0.0f;
        growthTimer = 0.0f;
        dormancyPitchCents = 0.0f;
        companionPitchCents = 0.0f;
        glide.reset();
        for (auto& o : oscs) o.reset();
        filter.reset();
        toneShaper.reset();
        ampEnv.kill();
        filterEnv.kill();
        vibratoLFO.reset();
    }
};

//==============================================================================
// OsierEngine — "The Herb Garden"
//==============================================================================
class OsierEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 4;

    juce::String getEngineId() const override { return "Osier"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFFC0C8C8); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount.load(); }

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        for (int i = 0; i < kMaxVoices; ++i)
        {
            voices[i].reset();
            voices[i].role = static_cast<OsierQuartetRole> (i);
            voices[i].ampEnv.prepare (srf);
            voices[i].filterEnv.prepare (srf);
            voices[i].vibratoLFO.setShape (StandardLFO::Sine);
            voices[i].vibratoLFO.reseed (static_cast<uint32_t> (i * 5003 + 201));
            for (int o = 0; o < OsierVoice::kNumOscs; ++o)
                voices[i].oscs[o].reset (static_cast<float> (o) * 0.5f);
        }

        smoothCutoff.prepare (srf);
        smoothCompanion.prepare (srf, 0.05f);  // slower smoothing

        accumulators.reset();
        // Chamber: warmth threshold lower (intimate setting warms faster)
        accumulators.wRiseRate = 0.0025f;
        accumulators.aThreshold = 0.45f;

        companion.reset();
        network.configure (0.4f, 5.0f);

        prepareSilenceGate (sr, maxBlockSize, 500.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices) v.reset();
        pitchBendNorm = 0.0f;
        modWheelAmount = 0.0f;
        aftertouchAmount = 0.0f;
        accumulators.reset();
        companion.reset();
        network.reset();
    }

    float getSampleForCoupling (int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? couplingCacheL : couplingCacheR;
    }

    void applyCouplingInput (CouplingType type, float amount,
                            const float* buf, int numSamples) override
    {
        if (!buf || numSamples <= 0) return;
        float val = buf[numSamples - 1] * amount;
        switch (type)
        {
            case CouplingType::AmpToFilter:  couplingFilterMod += val * 3000.0f; break;
            case CouplingType::LFOToPitch:   couplingPitchMod += val * 2.0f; break;
            case CouplingType::AmpToPitch:   couplingPitchMod += val; break;
            default: break;
        }
    }

    //==========================================================================
    // Render
    //==========================================================================
    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        int noteOnCount = 0;

        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
                wakeSilenceGate();
                noteOnCount++;
            }
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber());
            else if (msg.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel (msg.getPitchWheelValue());
            else if (msg.isChannelPressure())
                aftertouchAmount = msg.getChannelPressureValue() / 127.0f;
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount = msg.getControllerValue() / 127.0f;
        }

        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            buffer.clear (0, numSamples);
            couplingCacheL = couplingCacheR = 0.0f;
            return;
        }

        auto loadP = [] (std::atomic<float>* p, float def) {
            return p ? p->load (std::memory_order_relaxed) : def;
        };

        const float pCutoff       = loadP (paramCutoff, 5000.0f);
        const float pResonance    = loadP (paramResonance, 0.25f);
        const float pDetune       = loadP (paramDetune, 5.0f);
        const float pCompanion    = loadP (paramCompanion, 0.4f);
        const float pVibratoRate  = loadP (paramVibratoRate, 5.0f);
        const float pVibratoDepth = loadP (paramVibratoDepth, 0.25f);
        const float pBendRange    = loadP (paramBendRange, 2.0f);
        const float pFilterEnvAmt = loadP (paramFilterEnvAmt, 0.35f);
        const float pBrightness   = loadP (paramBrightness, 0.5f);
        const float pIntimacy     = loadP (paramIntimacy, 0.5f);

        const float macroChar     = loadP (paramMacroCharacter, 0.0f);
        const float macroMove     = loadP (paramMacroMovement, 0.0f);
        const float macroCoupl    = loadP (paramMacroCoupling, 0.0f);
        const float macroSpace    = loadP (paramMacroSpace, 0.0f);

        const float lfo1Rate      = loadP (paramLfo1Rate, 0.5f);
        const float lfo1Depth     = loadP (paramLfo1Depth, 0.0f);
        const int   lfo1Shape     = static_cast<int> (loadP (paramLfo1Shape, 0.0f));
        const float lfo2Rate      = loadP (paramLfo2Rate, 1.0f);
        const float lfo2Depth     = loadP (paramLfo2Depth, 0.0f);
        const int   lfo2Shape     = static_cast<int> (loadP (paramLfo2Shape, 0.0f));

        //-- Accumulator update --
        float avgVel = 0.0f;
        int activeCount = 0;
        bool voiceActive[4] = {};
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (voices[i].active)
            {
                avgVel += voices[i].velocity;
                activeCount++;
                voiceActive[i] = true;
            }
        }
        if (activeCount > 0) avgVel /= static_cast<float> (activeCount);

        float blockSizeSec = static_cast<float> (numSamples) / srf;
        accumulators.update (blockSizeSec, activeCount, avgVel, noteOnCount);

        //-- Effective parameters --
        float effectiveCutoff = std::clamp (pCutoff + macroChar * 4000.0f
            + accumulators.getSeasonBrightness() * 1500.0f
            + pBrightness * 6000.0f + couplingFilterMod
            + aftertouchAmount * 2500.0f, 200.0f, 20000.0f);
        const float effectiveCompanion = std::clamp (pCompanion + macroCoupl * 0.4f, 0.0f, 1.0f);  // M3 COUPLING: deeper companion affinity
        const float effectiveWidth = 1.0f + macroSpace * 0.6f;  // M4 SPACE: stereo expansion

        //-- Companion planting update (block rate) --
        companion.update (voiceActive, effectiveCompanion, blockSizeSec);

        smoothCutoff.set (effectiveCutoff);
        smoothCompanion.set (effectiveCompanion);

        couplingFilterMod = 0.0f;
        couplingPitchMod = 0.0f;

        const float bendSemitones = pitchBendNorm * pBendRange;
        float effectiveVibratoDepth = pVibratoDepth + modWheelAmount * 0.4f
                                    + accumulators.getAggressionVibrato() * 0.25f;

        //-- Compute companion pitch influence (block rate) --
        // Each voice is pulled slightly toward its companion voices' pitches
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (!voices[i].active) continue;
            float pull = 0.0f;
            for (int j = 0; j < kMaxVoices; ++j)
            {
                if (i == j || !voices[j].active) continue;
                float freqI = voices[i].glide.getFreq();
                float freqJ = voices[j].glide.getFreq();
                if (freqI > 1.0f && freqJ > 1.0f)
                {
                    float cents = 1200.0f * fastLog2 (freqJ / freqI);
                    float aff = companion.getVoiceAffinity (i);
                    // Pull toward the other voice, scaled by affinity and companion strength
                    pull += cents * aff * effectiveCompanion * 0.01f * pIntimacy;
                }
            }
            voices[i].companionPitchCents = pull;
        }

        //-- Per-voice LFO config --
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (!voices[i].active) continue;
            const auto& cfg = kRoleConfigs[static_cast<int> (voices[i].role)];
            voices[i].vibratoLFO.setRate ((pVibratoRate + macroMove * 3.0f) * cfg.vibratoRateMult, srf);
            voices[i].lfo1.setRate (lfo1Rate, srf);
            voices[i].lfo1.setShape (lfo1Shape);
            voices[i].lfo2.setRate (lfo2Rate, srf);
            voices[i].lfo2.setShape (lfo2Shape);
        }

        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

        for (int s = 0; s < numSamples; ++s)
        {
            float cutNow = smoothCutoff.process();
            float compNow = smoothCompanion.process();

            float mixL = 0.0f, mixR = 0.0f;

            for (int vi = 0; vi < kMaxVoices; ++vi)
            {
                auto& voice = voices[vi];
                if (!voice.active) continue;

                const auto& cfg = kRoleConfigs[static_cast<int> (voice.role)];

                float baseFreq = voice.glide.process();
                float vibrato = voice.vibratoLFO.process() * effectiveVibratoDepth * cfg.vibratoDepthMult;

                float freq = baseFreq * PitchBendUtil::semitonesToFreqRatio (
                    bendSemitones + couplingPitchMod + vibrato * 0.12f
                    + voice.dormancyPitchCents / 100.0f
                    + voice.companionPitchCents / 100.0f
                    + cfg.detuneCents / 100.0f);

                float l1 = voice.lfo1.process() * lfo1Depth;
                float l2 = voice.lfo2.process() * lfo2Depth;

                // Growth Mode
                float growthGain = 1.0f;
                if (voice.growthMode)
                {
                    voice.growthTimer += 1.0f / srf;
                    voice.growthPhase = std::min (voice.growthTimer / voice.growthDuration, 1.0f);
                    // Chamber voices enter sequentially based on role
                    float roleDelay = static_cast<float> (vi) * 0.15f;
                    float effectivePhase = std::max (voice.growthPhase - roleDelay, 0.0f) / (1.0f - roleDelay);
                    growthGain = effectivePhase * effectivePhase;
                }

                // 2 detuned saws per voice (thinner than orchestral)
                float oscMix = 0.0f;
                for (int o = 0; o < OsierVoice::kNumOscs; ++o)
                {
                    float detHz = freq * pDetune * (o == 0 ? -1.0f : 1.0f) / 1200.0f;
                    voice.oscs[o].setFrequency (freq + detHz, srf);
                    oscMix += voice.oscs[o].process();
                }
                oscMix *= 0.5f;

                // Per-role tonal shaping — each instrument has distinct character
                float roleCutoff = std::clamp (cutNow + cfg.filterBiasCents
                    + cfg.brightnessOffset * 2000.0f, 200.0f, 20000.0f);
                voice.toneShaper.setMode (CytomicSVF::Mode::LowPass);
                voice.toneShaper.setCoefficients (roleCutoff, 0.2f, srf);
                float shaped = voice.toneShaper.processSample (oscMix);

                // Main filter with envelope
                float envLevel = voice.filterEnv.process();
                float fCut = std::clamp (cutNow + envLevel * pFilterEnvAmt * 5000.0f
                                        + l1 * 2500.0f, 200.0f, 20000.0f);
                voice.filter.setMode (CytomicSVF::Mode::LowPass);
                voice.filter.setCoefficients (fCut, std::clamp (pResonance + l2 * 0.15f, 0.0f, 1.0f), srf);  // l2 → resonance shimmer
                float filtered = voice.filter.processSample (shaped);

                // Amplitude
                float ampLevel = voice.ampEnv.process() * growthGain;
                if (ampLevel < 1e-6f && voice.ampEnv.getStage() == FilterEnvelope::Stage::Idle)
                {
                    voice.active = false;
                    continue;
                }

                // D001: velocity shapes timbre
                float output = filtered * ampLevel * (0.5f + voice.velocity * 0.5f);

                // Bass voice (nutrient provider): its warmth feeds the upper voices
                if (voice.role == OsierQuartetRole::Bass && accumulators.W > 0.3f)
                {
                    // Bass warmth boosts upper voices' sustain via accumulator
                    // (effect is in the companion planting, not direct audio)
                }

                // Companion affinity (compNow) narrows the stereo field in real-time:
                // at full companion, voices pull toward centre — the quartet breathes as one.
                float compPanL = voice.panL + compNow * (0.5f - voice.panL);
                float compPanR = voice.panR + compNow * (0.5f - voice.panR);
                mixL += output * compPanL;
                mixR += output * compPanR;
            }

            // M4 SPACE: mid/side width expansion
            const float mid  = (mixL + mixR) * 0.5f;
            const float side = (mixL - mixR) * 0.5f * effectiveWidth;
            outL[s] = mid + side;
            if (outR) outR[s] = mid - side;
            couplingCacheL = outL[s];
            couplingCacheR = outR ? outR[s] : mixR;
        }

        int count = 0;
        for (const auto& v : voices) if (v.active) ++count;
        activeVoiceCount.store (count);
        analyzeForSilenceGate (buffer, numSamples);
    }

    //==========================================================================
    // Note management — round-robin through quartet roles
    //==========================================================================

    void noteOn (int note, float vel) noexcept
    {
        int idx = VoiceAllocator::findFreeVoice (voices, kMaxVoices);
        auto& v = voices[idx];

        float freq = 440.0f * std::pow (2.0f, (static_cast<float> (note) - 69.0f) / 12.0f);
        bool isGrowthMode = paramGrowthMode && paramGrowthMode->load () > 0.5f;

        v.active = true;
        v.currentNote = note;
        v.velocity = vel;
        v.startTime = ++voiceCounter;
        // Role assigned by pitch so soprano always plays high notes, bass plays low notes.
        // Thresholds: Soprano ≥ G4 (67), Alto = D4–F#4 (62–66), Tenor = G3–C#4 (55–61), Bass < G3 (54).
        // This ensures tonal character matches the musical register being played.
        if      (note >= 67) v.role = OsierQuartetRole::Soprano;
        else if (note >= 62) v.role = OsierQuartetRole::Alto;
        else if (note >= 55) v.role = OsierQuartetRole::Tenor;
        else                 v.role = OsierQuartetRole::Bass;
        v.glide.setTargetOrSnap (freq);

        float attackTime = paramAttack ? paramAttack->load() : 0.1f;
        attackTime *= (1.2f - vel * 0.4f);

        v.ampEnv.prepare (srf);
        v.ampEnv.setADSR (attackTime,
                          paramDecay ? paramDecay->load() : 0.4f,
                          paramSustain ? paramSustain->load() : 0.75f,
                          paramRelease ? paramRelease->load() : 0.8f);
        v.ampEnv.triggerHard();

        v.filterEnv.prepare (srf);
        v.filterEnv.setADSR (attackTime * 0.4f, 0.6f, 0.15f, 0.7f);
        v.filterEnv.triggerHard();

        v.vibratoLFO.reset();

        v.growthMode = isGrowthMode;
        v.growthTimer = 0.0f;
        v.growthPhase = 0.0f;
        v.growthDuration = paramGrowthTime ? paramGrowthTime->load() : 20.0f;

        // Dormancy
        float dPitch = accumulators.getDormancyPitchVariance();
        uint32_t seed = static_cast<uint32_t> (note * 4999 + idx * 71);
        seed = seed * 1664525u + 1013904223u;
        v.dormancyPitchCents = (static_cast<float> (seed & 0xFFFF) / 32768.0f - 1.0f) * dPitch;

        // Quartet panning: traditional seating arrangement
        // Soprano = left, Alto = center-left, Tenor = center-right, Bass = right
        float panPositions[4] = { -0.6f, -0.2f, 0.2f, 0.6f };
        float angle = (panPositions[idx] + 1.0f) * 0.25f * 3.14159265f;
        v.panL = std::cos (angle);
        v.panR = std::sin (angle);
    }

    void noteOff (int note) noexcept
    {
        for (auto& v : voices)
            if (v.active && v.currentNote == note)
                v.ampEnv.release();
    }

    //==========================================================================
    // Parameters — 27 total
    //==========================================================================

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl (params);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl (params);
        return { params.begin(), params.end() };
    }

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;

        // Amp envelope
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_attack", 1 }, "Osier Attack",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.3f), 0.1f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_decay", 1 }, "Osier Decay",
            juce::NormalisableRange<float> (0.01f, 5.0f, 0.0f, 0.4f), 0.4f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_sustain", 1 }, "Osier Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.75f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_release", 1 }, "Osier Release",
            juce::NormalisableRange<float> (0.01f, 10.0f, 0.0f, 0.4f), 0.8f));

        // Filter
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_cutoff", 1 }, "Osier Cutoff",
            juce::NormalisableRange<float> (200.0f, 20000.0f, 0.0f, 0.3f), 5000.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_resonance", 1 }, "Osier Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.25f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_filterEnvAmt", 1 }, "Osier Filter Env Amt",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.35f));

        // Ensemble
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_detune", 1 }, "Osier Detune",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.1f), 5.0f));

        // Garden-specific
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_companion", 1 }, "Osier Companion Planting",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_intimacy", 1 }, "Osier Intimacy",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_brightness", 1 }, "Osier Brightness",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // Vibrato
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_vibratoRate", 1 }, "Osier Vibrato Rate",
            juce::NormalisableRange<float> (0.5f, 12.0f, 0.1f), 5.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_vibratoDepth", 1 }, "Osier Vibrato Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.25f));

        // Growth Mode
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_growthMode", 1 }, "Osier Growth Mode",
            juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_growthTime", 1 }, "Osier Growth Time",
            juce::NormalisableRange<float> (5.0f, 60.0f, 0.1f), 20.0f));

        // Pitch
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_bendRange", 1 }, "Osier Bend Range",
            juce::NormalisableRange<float> (1.0f, 24.0f, 1.0f), 2.0f));

        // Macros
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_macroCharacter", 1 }, "Osier Macro CHARACTER",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_macroMovement", 1 }, "Osier Macro MOVEMENT",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_macroCoupling", 1 }, "Osier Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_macroSpace", 1 }, "Osier Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // LFOs
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_lfo1Rate", 1 }, "Osier LFO1 Rate",
            juce::NormalisableRange<float> (0.005f, 20.0f, 0.0f, 0.3f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_lfo1Depth", 1 }, "Osier LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.1f));
        params.push_back (std::make_unique<PI> (juce::ParameterID { "osier_lfo1Shape", 1 }, "Osier LFO1 Shape", 0, 4, 0));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_lfo2Rate", 1 }, "Osier LFO2 Rate",
            juce::NormalisableRange<float> (0.005f, 20.0f, 0.0f, 0.3f), 1.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "osier_lfo2Depth", 1 }, "Osier LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PI> (juce::ParameterID { "osier_lfo2Shape", 1 }, "Osier LFO2 Shape", 0, 4, 0));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramAttack         = apvts.getRawParameterValue ("osier_attack");
        paramDecay          = apvts.getRawParameterValue ("osier_decay");
        paramSustain        = apvts.getRawParameterValue ("osier_sustain");
        paramRelease        = apvts.getRawParameterValue ("osier_release");
        paramCutoff         = apvts.getRawParameterValue ("osier_cutoff");
        paramResonance      = apvts.getRawParameterValue ("osier_resonance");
        paramFilterEnvAmt   = apvts.getRawParameterValue ("osier_filterEnvAmt");
        paramDetune         = apvts.getRawParameterValue ("osier_detune");
        paramCompanion      = apvts.getRawParameterValue ("osier_companion");
        paramIntimacy       = apvts.getRawParameterValue ("osier_intimacy");
        paramBrightness     = apvts.getRawParameterValue ("osier_brightness");
        paramVibratoRate    = apvts.getRawParameterValue ("osier_vibratoRate");
        paramVibratoDepth   = apvts.getRawParameterValue ("osier_vibratoDepth");
        paramGrowthMode     = apvts.getRawParameterValue ("osier_growthMode");
        paramGrowthTime     = apvts.getRawParameterValue ("osier_growthTime");
        paramBendRange      = apvts.getRawParameterValue ("osier_bendRange");
        paramMacroCharacter = apvts.getRawParameterValue ("osier_macroCharacter");
        paramMacroMovement  = apvts.getRawParameterValue ("osier_macroMovement");
        paramMacroCoupling  = apvts.getRawParameterValue ("osier_macroCoupling");
        paramMacroSpace     = apvts.getRawParameterValue ("osier_macroSpace");
        paramLfo1Rate       = apvts.getRawParameterValue ("osier_lfo1Rate");
        paramLfo1Depth      = apvts.getRawParameterValue ("osier_lfo1Depth");
        paramLfo1Shape      = apvts.getRawParameterValue ("osier_lfo1Shape");
        paramLfo2Rate       = apvts.getRawParameterValue ("osier_lfo2Rate");
        paramLfo2Depth      = apvts.getRawParameterValue ("osier_lfo2Depth");
        paramLfo2Shape      = apvts.getRawParameterValue ("osier_lfo2Shape");
    }

private:
    double sr = 48000.0;
    float srf = 48000.0f;

    std::array<OsierVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    std::atomic<int> activeVoiceCount { 0 };

    GardenAccumulators accumulators;
    CompanionPlanting companion;
    GardenMycorrhizalNetwork network;

    ParameterSmoother smoothCutoff, smoothCompanion;

    float pitchBendNorm = 0.0f;
    float modWheelAmount = 0.0f;
    float aftertouchAmount = 0.0f;

    float couplingFilterMod = 0.0f, couplingPitchMod = 0.0f;
    float couplingCacheL = 0.0f, couplingCacheR = 0.0f;

    std::atomic<float>* paramAttack = nullptr;
    std::atomic<float>* paramDecay = nullptr;
    std::atomic<float>* paramSustain = nullptr;
    std::atomic<float>* paramRelease = nullptr;
    std::atomic<float>* paramCutoff = nullptr;
    std::atomic<float>* paramResonance = nullptr;
    std::atomic<float>* paramFilterEnvAmt = nullptr;
    std::atomic<float>* paramDetune = nullptr;
    std::atomic<float>* paramCompanion = nullptr;
    std::atomic<float>* paramIntimacy = nullptr;
    std::atomic<float>* paramBrightness = nullptr;
    std::atomic<float>* paramVibratoRate = nullptr;
    std::atomic<float>* paramVibratoDepth = nullptr;
    std::atomic<float>* paramGrowthMode = nullptr;
    std::atomic<float>* paramGrowthTime = nullptr;
    std::atomic<float>* paramBendRange = nullptr;
    std::atomic<float>* paramMacroCharacter = nullptr;
    std::atomic<float>* paramMacroMovement = nullptr;
    std::atomic<float>* paramMacroCoupling = nullptr;
    std::atomic<float>* paramMacroSpace = nullptr;
    std::atomic<float>* paramLfo1Rate = nullptr;
    std::atomic<float>* paramLfo1Depth = nullptr;
    std::atomic<float>* paramLfo1Shape = nullptr;
    std::atomic<float>* paramLfo2Rate = nullptr;
    std::atomic<float>* paramLfo2Depth = nullptr;
    std::atomic<float>* paramLfo2Shape = nullptr;
};

} // namespace xolokun
