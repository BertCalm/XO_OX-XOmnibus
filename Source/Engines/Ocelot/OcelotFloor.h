#pragma once

#include <cmath>
#include <algorithm>
#include <array>
#include "OcelotParamSnapshot.h"
#include "BiomeMorph.h"
#include "KarplusStrong.h"
#include "ModalResonator.h"
#include "WaveFolder.h"

namespace xocelot {

// Forward declaration (defined in EcosystemMatrix.h, but we only need the struct here)
struct StrataModulation;

// OcelotFloor -- Physical-Modeled Percussion
//
// 6 models: berimbau / cuica / agogo / kalimba / pandeiro / log drum
//
// Each model is a distinct physical topology using KarplusStrong and ModalResonator primitives.
// State for ALL models is pre-allocated (no allocation on audio thread).
//
// Biome transforms: Jungle = neutral, Underwater = submerged (damping+, brightness-),
//                   Winter = crystalline (damping-, brightness+, cold timbres).

class OcelotFloor
{
public:
    static constexpr int kDelayLineSize = 4096; // preserved for API compat

    void prepare(double sampleRate)
    {
        sr = static_cast<float>(sampleRate);

        // KarplusStrong (used by Berimbau)
        ks.prepare(sr);

        // Reset all modal resonators
        gourdRes.reset();
        for (auto& r : modal) r.reset();

        // Cuica state
        cuicaPhase  = 0.0f;
        cuicaBendEnv = 0.0f;
        cuicaBpState = 0.0f;
        cuicaLpState = 0.0f;

        // Noise burst state
        noiseBurstCounter = 0;
        noiseBurstSamples = 0;

        // Sweep state (Berimbau)
        sweepPhase = 0.0f;

        // Common
        active = false;
        amplitude = 0.0f;
        lastAmplitude = 0.0f;
        currentNote = 60;
        currentVelocity = 0.0f;
        baseFreq = 261.63f;
        noiseSeed = 42u;

        // Pandeiro noise buffer state
        pandeiroNoisePhase = 0;
    }

    void noteOn(int midiNote, float velocity, const OcelotParamSnapshot& snap)
    {
        currentNote     = midiNote;
        currentVelocity = velocity;

        // Compute base frequency with pitch offset
        float pitchOffset = (snap.floorPitch - 0.5f) * 24.0f; // +/-12 semitones
        baseFreq = 440.0f * std::pow(2.0f, (midiNote - 69 + pitchOffset / 12.0f) / 12.0f);

        int model = std::clamp(snap.floorModel, 0, 5);

        // Reset common state
        amplitude = velocity;
        active = true;
        noiseSeed = static_cast<uint32_t>(midiNote * 7919 + 12345);

        switch (model)
        {
            case 0: noteOnBerimbau(velocity, snap); break;
            case 1: noteOnCuica(velocity, snap);    break;
            case 2: noteOnAgogo(velocity, snap);    break;
            case 3: noteOnKalimba(velocity, snap);  break;
            case 4: noteOnPandeiro(velocity, snap); break;
            case 5: noteOnLogDrum(velocity, snap);  break;
            default: break;
        }
    }

    void noteOff() { /* All models decay naturally */ }

    // renderBlock: fill outL/outR with numSamples of Floor audio.
    // Returns block RMS amplitude for EcosystemMatrix.
    float renderBlock(float* outL, float* outR, int numSamples,
                      const OcelotParamSnapshot& snap,
                      const BiomeProfile& biome,
                      const struct StrataModulation& mod)
    {
        if (!active)
        {
            std::fill(outL, outL + numSamples, 0.0f);
            std::fill(outR, outR + numSamples, 0.0f);
            return 0.0f;
        }

        // ---- Shared parameter computation (biome offsets applied once per block) ----
        float damp  = std::clamp(snap.floorDamping + biome.floorDampingBase
                                 + mod.floorDampingMod, 0.0f, 0.999f);
        float level = snap.floorLevel * (1.0f + mod.floorAccentMod * 0.5f);
        float brightness = std::clamp(biome.floorBrightnessBase, -1.0f, 1.0f);

        int model = std::clamp(snap.floorModel, 0, 5);

        switch (model)
        {
            case 0: renderBerimbau(outL, outR, numSamples, snap, biome, damp, level, brightness); break;
            case 1: renderCuica(outL, outR, numSamples, snap, biome, damp, level, brightness);    break;
            case 2: renderAgogo(outL, outR, numSamples, snap, biome, damp, level, brightness);    break;
            case 3: renderKalimba(outL, outR, numSamples, snap, biome, damp, level, brightness);  break;
            case 4: renderPandeiro(outL, outR, numSamples, snap, biome, damp, level, brightness); break;
            case 5: renderLogDrum(outL, outR, numSamples, snap, biome, damp, level, brightness);  break;
            default:
                std::fill(outL, outL + numSamples, 0.0f);
                std::fill(outR, outR + numSamples, 0.0f);
                break;
        }

        // Compute RMS and auto-silence
        float sumSq = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            sumSq += outL[i] * outL[i];

        lastAmplitude = std::sqrt(sumSq / std::max(1.0f, static_cast<float>(numSamples)));

        // Track amplitude decay for auto-silence
        amplitude *= std::pow(1.0f - damp * 0.002f, static_cast<float>(numSamples));
        if (amplitude < 0.0001f && lastAmplitude < 0.0001f)
            active = false;

        return lastAmplitude;
    }

    bool isActive() const          { return active; }
    float getLastAmplitude() const { return lastAmplitude; }

private:
    // ====================================================================
    //  State (pre-allocated for ALL 6 models -- no audio-thread allocation)
    // ====================================================================

    float sr = 44100.0f;
    bool active = false;
    float amplitude = 0.0f;
    float lastAmplitude = 0.0f;
    int currentNote = 60;
    float currentVelocity = 0.0f;
    float baseFreq = 261.63f;
    uint32_t noiseSeed = 42u;

    // Berimbau: KS string + gourd resonator + sweep
    KarplusStrong ks;
    ModalResonator gourdRes;
    float sweepPhase = 0.0f;
    float sweepAmount = 0.0f;

    // Cuica: friction oscillator + SVF bandpass
    float cuicaPhase = 0.0f;
    float cuicaBendEnv = 0.0f;
    float cuicaBpState = 0.0f;
    float cuicaLpState = 0.0f;

    // Shared modal resonators (reused across Agogo/Kalimba/Pandeiro/LogDrum)
    // modal[0..2] = primary partials, modal[3] = auxiliary (wooden body / 3rd agogo partial)
    static constexpr int kMaxModals = 4;
    ModalResonator modal[kMaxModals];

    // Pandeiro / noise burst
    int noiseBurstCounter = 0;
    int noiseBurstSamples = 0;
    float noiseLpState = 0.0f;
    int pandeiroNoisePhase = 0;

    // ====================================================================
    //  Utility
    // ====================================================================

    inline float lcgNoise()
    {
        noiseSeed = noiseSeed * 1664525u + 1013904223u;
        return static_cast<float>(static_cast<int32_t>(noiseSeed)) * 4.656612e-10f;
    }

    inline static float flushDenormal(float x)
    {
        return (std::abs(x) < 1.0e-15f) ? 0.0f : x;
    }

    inline static float midiToFreq(int note)
    {
        return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    }

    // Simple 1-pole lowpass for brightness tilt on output
    // state is caller-managed
    inline static float onePoleLP(float x, float coeff, float& state)
    {
        state = state + coeff * (x - state);
        state = flushDenormal(state);
        return state;
    }

    // Apply brightness tilt: negative = darker (more LP), positive = brighter (less LP)
    // Returns a LP coefficient for the 1-pole filter
    inline float brightnessTiltCoeff(float brightness) const
    {
        // brightness -1..+1; at +1 = no filtering (coeff~1), at -1 = heavy LP (coeff~0.05)
        float t = (brightness + 1.0f) * 0.5f; // 0..1
        return 0.05f + t * 0.95f;  // 0.05 .. 1.0
    }

    // ====================================================================
    //  Model 0: BERIMBAU -- Plucked String + Gourd Resonator
    // ====================================================================

    void noteOnBerimbau(float velocity, const OcelotParamSnapshot& snap)
    {
        // Reset and configure KS string
        ks.reset();
        ks.setFreq(baseFreq);
        ks.excite(velocity, noiseSeed);

        // Sweep: floorStrike controls pitch contour depth on strike
        sweepPhase  = 0.0f;
        sweepAmount = snap.floorStrike * 0.15f; // up to 15% pitch bend

        // Gourd resonator (will be configured per-block based on biome)
        gourdRes.reset();
    }

    void renderBerimbau(float* outL, float* outR, int numSamples,
                        const OcelotParamSnapshot& snap,
                        const BiomeProfile& biome,
                        float damp, float level, float brightness)
    {
        // Gourd center frequency: biome-driven
        // Jungle=300Hz, Underwater=200Hz, Winter=400Hz
        // Use brightness as proxy: -0.5 (underwater) -> 200, 0.1 (jungle) -> 300, 0.4 (winter) -> 400
        float gourdCenter = 300.0f + brightness * 200.0f;
        gourdCenter = std::clamp(gourdCenter, 150.0f, 500.0f);

        // Gourd Q: Underwater = high Q (heavy resonance), Winter = metallic (high Q but brighter)
        float gourdQ = 8.0f + (1.0f - damp) * 12.0f;
        gourdRes.setParams(gourdCenter, gourdQ, 0.4f, sr);

        // Sweep rate: linear decay over 20-50ms (use 35ms as midpoint)
        float sweepDecayPerSample = 1.0f / (sr * 0.035f);

        float brightCoeff = brightnessTiltCoeff(brightness);
        float tiltState = 0.0f; // per-block scratch

        for (int i = 0; i < numSamples; ++i)
        {
            // Pitch sweep: frequency modulation decaying from noteOn
            float sweepMul = 1.0f + sweepAmount * std::exp(-sweepPhase * 6.0f);
            sweepPhase += sweepDecayPerSample;

            // Update KS frequency with sweep
            ks.setFreq(baseFreq * sweepMul);

            // Process KS string
            float stringSample = ks.process(damp);

            // Pass through gourd resonator
            float gourdSample = gourdRes.process(stringSample * 0.5f);

            // Mix: string + gourd body
            float mixed = stringSample * 0.7f + gourdSample * 0.3f;

            // Brightness tilt
            mixed = onePoleLP(mixed, brightCoeff, tiltState);

            float out = mixed * level;
            outL[i] = out;
            outR[i] = out;
        }
    }

    // ====================================================================
    //  Model 1: CUICA -- Friction Oscillator
    // ====================================================================

    void noteOnCuica(float velocity, const OcelotParamSnapshot& snap)
    {
        cuicaPhase   = 0.0f;
        cuicaBendEnv = 1.0f;  // starts at max, decays
        cuicaBpState = 0.0f;
        cuicaLpState = 0.0f;
    }

    void renderCuica(float* outL, float* outR, int numSamples,
                     const OcelotParamSnapshot& snap,
                     const BiomeProfile& biome,
                     float damp, float level, float brightness)
    {
        constexpr float kPi = 3.14159265358979323846f;
        constexpr float kTwoPi = kPi * 2.0f;

        float tension = std::clamp(snap.floorTension, 0.0f, 1.0f);

        // Bend envelope decay rate: faster tension = slower decay (more sustained squeal)
        float bendDecay = 1.0f - (0.5f + tension * 0.4f) / (sr * 0.3f); // ~300ms envelope
        bendDecay = std::clamp(bendDecay, 0.99f, 0.99999f);

        // SVF parameters
        float svfQ = 15.0f + tension * 10.0f; // 15-25 for squealing quality
        float svfCutoff = baseFreq * 2.0f;
        svfCutoff = std::clamp(svfCutoff, 100.0f, sr * 0.45f);
        float svfF = 2.0f * std::sin(kPi * svfCutoff / sr);
        svfF = std::clamp(svfF, 0.001f, 0.999f);

        // Biome modifiers
        // Underwater: reduce pitch bend, increase resonance (sonar ping)
        float bendScale = 1.0f;
        float qBoost = 0.0f;
        // Winter: noise modulate pitch (metallic scrape)
        float noiseModAmt = 0.0f;

        if (brightness < -0.2f) // Underwater territory
        {
            bendScale = 0.3f;  // less pitch bend
            qBoost = 5.0f;     // more resonance
        }
        else if (brightness > 0.2f) // Winter territory
        {
            noiseModAmt = 0.01f * brightness; // noise on frequency
        }

        svfQ += qBoost;

        float brightCoeff = brightnessTiltCoeff(brightness);
        float tiltState = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            // Pitch bend: upward sweep decaying from noteOn
            float bendSemitones = cuicaBendEnv * tension * 12.0f * bendScale;
            float freq = baseFreq * std::pow(2.0f, bendSemitones / 12.0f);

            // Winter: noise modulate frequency
            if (noiseModAmt > 0.0f)
            {
                float noise = lcgNoise();
                freq *= (1.0f + noise * noiseModAmt);
            }

            freq = std::clamp(freq, 20.0f, sr * 0.45f);

            // Damped sine oscillator
            cuicaPhase += freq / sr;
            if (cuicaPhase >= 1.0f) cuicaPhase -= 1.0f;
            float osc = std::sin(kTwoPi * cuicaPhase);

            // Amplitude decay with damping
            float ampDecay = std::exp(-static_cast<float>(i) * damp * 0.0001f);
            osc *= currentVelocity * ampDecay;

            // Inline SVF bandpass for squealing quality
            // h = x - lp - (1/Q)*bp; bp += f*h; lp += f*bp; bp = tanh(bp);
            float h = osc - cuicaLpState - (1.0f / svfQ) * cuicaBpState;
            cuicaBpState += svfF * h;
            cuicaLpState += svfF * cuicaBpState;
            cuicaBpState = std::tanh(cuicaBpState); // saturation for character
            cuicaBpState = flushDenormal(cuicaBpState);
            cuicaLpState = flushDenormal(cuicaLpState);

            float sample = cuicaBpState;

            // Brightness tilt
            sample = onePoleLP(sample, brightCoeff, tiltState);

            float out = sample * level;
            outL[i] = out;
            outR[i] = out;

            // Decay the bend envelope
            cuicaBendEnv *= bendDecay;
            cuicaBendEnv = flushDenormal(cuicaBendEnv);
        }
    }

    // ====================================================================
    //  Model 2: AGOGO -- Struck Two-Tone Bell
    // ====================================================================

    void noteOnAgogo(float velocity, const OcelotParamSnapshot& snap)
    {
        // Reset the 2 (or 3) modal resonators
        modal[0].reset();
        modal[1].reset();
        modal[2].reset(); // 3rd partial for Winter biome

        // Exciter: impulse on first sample (stored in velocity for renderAgogo)
        // We use the noiseBurstCounter to track if impulse has been delivered
        noiseBurstCounter = 0; // 0 = impulse not yet delivered
    }

    void renderAgogo(float* outL, float* outR, int numSamples,
                     const OcelotParamSnapshot& snap,
                     const BiomeProfile& biome,
                     float damp, float level, float brightness)
    {
        // Two tones: fundamental and 2.67x inharmonic
        float f1 = baseFreq;
        float f2 = baseFreq * 2.67f;

        // Q: 20 + (1-damping)*80 => range 20-100
        float q = 20.0f + (1.0f - damp) * 80.0f;

        // Strike balance: floorStrike 0=low dominant, 1=high dominant
        float strike = std::clamp(snap.floorStrike, 0.0f, 1.0f);
        float g1 = 1.0f - strike * 0.7f;  // low cone gain
        float g2 = 0.3f + strike * 0.7f;  // high cone gain

        modal[0].setParams(f1, q, g1, sr);
        modal[1].setParams(f2, q * 0.8f, g2, sr);

        // Winter: add 3rd partial at 4.5x, brighter
        bool useThirdPartial = (brightness > 0.2f);
        if (useThirdPartial)
        {
            float f3 = baseFreq * 4.5f;
            float winterQ = q * 0.6f; // faster decay for 3rd partial
            modal[2].setParams(f3, winterQ, 0.2f, sr);
        }

        // Underwater: LP attenuation
        float lpCoeff = brightnessTiltCoeff(brightness);
        float tiltState = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            // Impulse excitation: only on first sample
            float excitation = 0.0f;
            if (noiseBurstCounter == 0)
            {
                excitation = currentVelocity;
                noiseBurstCounter = 1;
            }

            float s1 = modal[0].process(excitation);
            float s2 = modal[1].process(excitation);
            float sample = s1 + s2;

            if (useThirdPartial)
                sample += modal[2].process(excitation);

            // Brightness tilt (underwater = darker)
            sample = onePoleLP(sample, lpCoeff, tiltState);

            float out = sample * level;
            outL[i] = out;
            outR[i] = out;
        }
    }

    // ====================================================================
    //  Model 3: KALIMBA -- Tine Modal Synthesis (DEFAULT)
    // ====================================================================

    void noteOnKalimba(float velocity, const OcelotParamSnapshot& snap)
    {
        modal[0].reset();
        modal[1].reset();
        modal[2].reset();

        noiseBurstCounter = 0; // impulse not yet delivered
    }

    void renderKalimba(float* outL, float* outR, int numSamples,
                       const OcelotParamSnapshot& snap,
                       const BiomeProfile& biome,
                       float damp, float level, float brightness)
    {
        // Tine partial ratios: 1x, 2.76x, 5.4x (inharmonic -- kalimba signature)
        float f1 = baseFreq;
        float f2 = baseFreq * 2.76f;
        float f3 = baseFreq * 5.4f;

        // Q: 50 + (1-damping)*150 => range 50-200 (metallic tine ring)
        float q = 50.0f + (1.0f - damp) * 150.0f;

        // Biome modifications
        float freqMul = 1.0f;
        float qMul = 1.0f;
        if (brightness < -0.2f)
        {
            // Underwater: glass marimba -- pitched lower, longer ring
            freqMul = 0.85f;
            qMul = 1.3f;
        }
        else if (brightness > 0.2f)
        {
            // Winter: ice chimes -- +12st, long ring, no damping
            freqMul = 2.0f; // +12 semitones
            qMul = 1.5f;
        }

        f1 *= freqMul;
        f2 *= freqMul;
        f3 *= freqMul;
        q  *= qMul;

        // Gains: decreasing with partial number
        float baseGain1 = 1.0f;
        float baseGain2 = 0.5f;
        float baseGain3 = 0.25f;

        // Comb notch via pickup position: floorStrike adjusts partial gains
        // strike=0: all partials present; strike=1: emphasize high partials
        float strike = std::clamp(snap.floorStrike, 0.0f, 1.0f);
        float pickupScaleHigh = 0.5f + strike * 0.5f;
        float pickupScaleLow  = 1.0f - strike * 0.3f;

        modal[0].setParams(f1, q, baseGain1 * pickupScaleLow, sr);
        modal[1].setParams(f2, q * 0.9f, baseGain2 * pickupScaleHigh, sr);
        modal[2].setParams(f3, q * 0.7f, baseGain3 * pickupScaleHigh, sr);

        float lpCoeff = brightnessTiltCoeff(brightness);
        float tiltState = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            float excitation = 0.0f;
            if (noiseBurstCounter == 0)
            {
                excitation = currentVelocity;
                noiseBurstCounter = 1;
            }

            float s1 = modal[0].process(excitation);
            float s2 = modal[1].process(excitation);
            float s3 = modal[2].process(excitation);

            float sample = s1 + s2 + s3;

            // Brightness tilt
            sample = onePoleLP(sample, lpCoeff, tiltState);

            float out = sample * level;
            outL[i] = out;
            outR[i] = out;
        }
    }

    // ====================================================================
    //  Model 4: PANDEIRO -- Membrane + Jingles
    // ====================================================================

    void noteOnPandeiro(float velocity, const OcelotParamSnapshot& snap)
    {
        modal[0].reset();
        modal[1].reset();

        noiseBurstCounter = 0; // impulse not yet delivered

        // Noise burst: 50 samples of noise for jingle component
        noiseBurstSamples = 50;
        noiseLpState = 0.0f;
        pandeiroNoisePhase = 0;
    }

    void renderPandeiro(float* outL, float* outR, int numSamples,
                        const OcelotParamSnapshot& snap,
                        const BiomeProfile& biome,
                        float damp, float level, float brightness)
    {
        // Drumhead modes: fundamental + 2.3x overtone
        float f1 = baseFreq;
        float f2 = baseFreq * 2.3f;

        // Q: 10 + (1-damping)*30 => range 10-40 (drum, not bell)
        float q = 10.0f + (1.0f - damp) * 30.0f;

        // Biome: Underwater = muffled + extended; Winter = softer, longer decay
        float qMul = 1.0f;
        if (brightness < -0.2f)
        {
            // Underwater: heavy damping, muffled
            qMul = 0.7f;
        }
        else if (brightness > 0.2f)
        {
            // Winter: longer decay
            qMul = 1.3f;
        }
        q *= qMul;

        modal[0].setParams(f1, q, 1.0f, sr);
        modal[1].setParams(f2, q * 0.7f, 0.6f, sr);

        // Jingle noise: level tied to damping (more damping = more jingle emphasis)
        float jingleLevel = 0.3f + damp * 0.4f;

        // Jingle brightness: floorStrike controls jingle LP cutoff
        float strike = std::clamp(snap.floorStrike, 0.0f, 1.0f);
        float jingleLpCoeff = 0.1f + strike * 0.8f; // 0.1 (dark) to 0.9 (bright)

        float lpCoeff = brightnessTiltCoeff(brightness);
        float tiltState = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            // Membrane excitation: impulse on first sample
            float excitation = 0.0f;
            if (noiseBurstCounter == 0)
            {
                excitation = currentVelocity;
                noiseBurstCounter = 1;
            }

            float membrane = modal[0].process(excitation) + modal[1].process(excitation);

            // Jingle noise burst: 50 samples of filtered noise
            float jingle = 0.0f;
            if (pandeiroNoisePhase < noiseBurstSamples)
            {
                float noise = lcgNoise() * currentVelocity;
                // 1-pole LP for jingle brightness
                noiseLpState = noiseLpState + jingleLpCoeff * (noise - noiseLpState);
                noiseLpState = flushDenormal(noiseLpState);
                jingle = noiseLpState * jingleLevel;
                pandeiroNoisePhase++;
            }

            float sample = membrane + jingle;

            // Brightness tilt
            sample = onePoleLP(sample, lpCoeff, tiltState);

            float out = sample * level;
            outL[i] = out;
            outR[i] = out;
        }
    }

    // ====================================================================
    //  Model 5: LOG DRUM -- Wooden Slit Resonator
    // ====================================================================

    void noteOnLogDrum(float velocity, const OcelotParamSnapshot& snap)
    {
        modal[0].reset();
        modal[1].reset();
        modal[2].reset(); // wooden body resonator

        noiseBurstCounter = 0;
    }

    void renderLogDrum(float* outL, float* outR, int numSamples,
                       const OcelotParamSnapshot& snap,
                       const BiomeProfile& biome,
                       float damp, float level, float brightness)
    {
        // Two slits: 1x and 1.5x MIDI pitch
        float f1 = baseFreq;
        float f2 = baseFreq * 1.5f;

        // Q: 15 + (1-damping)*35 => range 15-50
        float q = 15.0f + (1.0f - damp) * 35.0f;

        // Slit select: floorStrike crossfades between low and high slit
        float strike = std::clamp(snap.floorStrike, 0.0f, 1.0f);
        float g1 = 1.0f - strike; // low slit
        float g2 = strike;        // high slit

        modal[0].setParams(f1, q, g1, sr);
        modal[1].setParams(f2, q * 0.8f, g2, sr);

        // Wooden body resonator: fixed 400 Hz, low Q
        float bodyQ = 4.0f;
        float bodyGain = 0.25f;

        // Biome: Underwater = oceanic thud (heavy LP, extend decay)
        // Winter = frozen wood crack (more click, shorter resonance)
        if (brightness < -0.2f)
        {
            bodyGain = 0.35f; // heavier body
            // Q reduction handled by damp (already boosted by biome.floorDampingBase)
        }
        else if (brightness > 0.2f)
        {
            bodyQ = 2.0f;      // shorter body resonance
            bodyGain = 0.15f;  // less body
            // Winter: click emphasis -- slightly increase excitation gain
        }

        modal[2].setParams(400.0f, bodyQ, bodyGain, sr);

        float lpCoeff = brightnessTiltCoeff(brightness);
        float tiltState = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            // Impulse excitation on first sample
            float excitation = 0.0f;
            if (noiseBurstCounter == 0)
            {
                excitation = currentVelocity;
                noiseBurstCounter = 1;

                // Winter: add click component (short noise)
                if (brightness > 0.2f)
                    excitation *= 1.3f;
            }

            float slit1 = modal[0].process(excitation);
            float slit2 = modal[1].process(excitation);
            float slitMix = slit1 + slit2;

            // Wooden body resonance on the combined slit output
            float body = modal[2].process(slitMix * 0.3f);

            float sample = slitMix + body;

            // Brightness tilt
            sample = onePoleLP(sample, lpCoeff, tiltState);

            float out = sample * level;
            outL[i] = out;
            outR[i] = out;
        }
    }
};

} // namespace xocelot
