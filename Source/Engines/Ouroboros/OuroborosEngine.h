#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>
#include <cstring>
#include <atomic>
#include <vector>

namespace xomnibus {

//==============================================================================
// AttractorTopology — Enum for the four chaotic ODE systems.
//==============================================================================
enum class AttractorTopology : int
{
    Lorenz  = 0,
    Rossler = 1,
    Chua    = 2,
    Aizawa  = 3
};

//==============================================================================
// AttractorState — State vector and RK4 integration for one attractor instance.
//
// Each voice holds two of these: one active, one for topology crossfade.
// All state is pre-allocated — zero dynamic memory.
//==============================================================================
struct AttractorState
{
    float x = 0.1f, y = 0.1f, z = 0.1f;
    AttractorTopology topology = AttractorTopology::Lorenz;

    // Per-topology bounding boxes for normalization
    struct BBox { float xMin, xMax, yMin, yMax, zMin, zMax; };

    static constexpr BBox bounds[4] = {
        { -25.0f, 25.0f, -30.0f, 30.0f,   0.0f, 55.0f },  // Lorenz
        { -12.0f, 12.0f, -12.0f, 12.0f,   0.0f, 25.0f },  // Rössler
        {  -3.0f,  3.0f,  -0.5f,  0.5f,  -4.0f,  4.0f },  // Chua
        {  -1.5f,  1.5f,  -1.5f,  1.5f,  -0.5f,  2.5f }   // Aizawa
    };

    // Per-topology calibration for step size derivation
    static constexpr float baseH[4]        = { 0.005f, 0.01f,  0.002f, 0.005f };
    static constexpr float calibFreq[4]    = { 130.0f, 95.0f,  200.0f, 110.0f };
    static constexpr float maxSafeH[4]     = { 0.02f,  0.05f,  0.008f, 0.015f };
    static constexpr float maxVelocity[4]  = { 200.0f, 30.0f,  50.0f,  10.0f  };

    // Compute derivatives for the selected topology
    void derivatives (float sx, float sy, float sz, float chaosIndex,
                      float& dx, float& dy, float& dz) const noexcept
    {
        switch (topology)
        {
            case AttractorTopology::Lorenz:
            {
                constexpr float sigma = 10.0f;
                constexpr float beta  = 8.0f / 3.0f;
                float rho = 20.0f + chaosIndex * 12.0f;
                dx = sigma * (sy - sx);
                dy = sx * (rho - sz) - sy;
                dz = sx * sy - beta * sz;
                break;
            }
            case AttractorTopology::Rossler:
            {
                constexpr float a = 0.2f;
                constexpr float b = 0.2f;
                float c = 3.0f + chaosIndex * 15.0f;
                dx = -(sy + sz);
                dy = sx + a * sy;
                dz = b + sz * (sx - c);
                break;
            }
            case AttractorTopology::Chua:
            {
                constexpr float beta_c  = 28.0f;
                constexpr float m0 = -1.143f;
                constexpr float m1 = -0.714f;
                float alpha_c = 9.0f + chaosIndex * 7.0f;
                // Chua's diode function
                float h_x = m1 * sx + 0.5f * (m0 - m1) * (std::fabs (sx + 1.0f) - std::fabs (sx - 1.0f));
                dx = alpha_c * (sy - sx - h_x);
                dy = sx - sy + sz;
                dz = -beta_c * sy;
                break;
            }
            case AttractorTopology::Aizawa:
            {
                constexpr float a_ai = 0.95f, b_ai = 0.7f, g_ai = 0.6f;
                constexpr float d_ai = 3.5f, z_ai = 0.1f;
                float e_ai = 0.1f + chaosIndex * 0.85f;
                dx = (sz - b_ai) * sx - d_ai * sy;
                dy = d_ai * sx + (sz - b_ai) * sy;
                float r2 = sx * sx + sy * sy;
                dz = g_ai + a_ai * sz - (sz * sz * sz) / 3.0f
                     - r2 * (1.0f + e_ai * sz) + z_ai * sz * sx * sx * sx;
                break;
            }
        }
    }

    // Compute step size h from target pitch frequency and chaos index
    float computeStepSize (float targetFreqHz, float chaosIndex) const noexcept
    {
        int ti = static_cast<int> (topology);
        float h = (targetFreqHz / calibFreq[ti]) * baseH[ti];
        float safeMax = maxSafeH[ti] * (1.0f - chaosIndex * 0.3f);
        if (h > safeMax) h = safeMax;
        if (h < 0.0001f) h = 0.0001f;
        return h;
    }

    // Single RK4 step with external velocity injection
    void step (float h, float chaosIndex, float injDx, float injDy) noexcept
    {
        float dx1, dy1, dz1, dx2, dy2, dz2, dx3, dy3, dz3, dx4, dy4, dz4;

        derivatives (x, y, z, chaosIndex, dx1, dy1, dz1);
        dx1 += injDx; dy1 += injDy;

        float hh = h * 0.5f;
        derivatives (x + hh * dx1, y + hh * dy1, z + hh * dz1, chaosIndex, dx2, dy2, dz2);
        dx2 += injDx; dy2 += injDy;

        derivatives (x + hh * dx2, y + hh * dy2, z + hh * dz2, chaosIndex, dx3, dy3, dz3);
        dx3 += injDx; dy3 += injDy;

        derivatives (x + h * dx3, y + h * dy3, z + h * dz3, chaosIndex, dx4, dy4, dz4);
        dx4 += injDx; dy4 += injDy;

        float h6 = h / 6.0f;
        x += h6 * (dx1 + 2.0f * dx2 + 2.0f * dx3 + dx4);
        y += h6 * (dy1 + 2.0f * dy2 + 2.0f * dy3 + dy4);
        z += h6 * (dz1 + 2.0f * dz2 + 2.0f * dz3 + dz4);

        // Store velocities for coupling output
        lastDxDt = dx1;
        lastDyDt = dy1;

        // Flush denormals
        x = flushDenormal (x);
        y = flushDenormal (y);
        z = flushDenormal (z);
    }

    // Normalize state to [0,1]^3 using this topology's bounding box
    void normalizeToUnit (float& nx, float& ny, float& nz) const noexcept
    {
        int ti = static_cast<int> (topology);
        nx = (x - bounds[ti].xMin) / (bounds[ti].xMax - bounds[ti].xMin);
        ny = (y - bounds[ti].yMin) / (bounds[ti].yMax - bounds[ti].yMin);
        nz = (z - bounds[ti].zMin) / (bounds[ti].zMax - bounds[ti].zMin);
        nx = clamp (nx, 0.0f, 1.0f);
        ny = clamp (ny, 0.0f, 1.0f);
        nz = clamp (nz, 0.0f, 1.0f);
    }

    // Initialize state from [0,1]^3 mapped to this topology's bounding box
    void initFromUnit (float nx, float ny, float nz) noexcept
    {
        int ti = static_cast<int> (topology);
        x = bounds[ti].xMin + nx * (bounds[ti].xMax - bounds[ti].xMin);
        y = bounds[ti].yMin + ny * (bounds[ti].yMax - bounds[ti].yMin);
        z = bounds[ti].zMin + nz * (bounds[ti].zMax - bounds[ti].zMin);
    }

    // Apply cubic saturator (Hermite soft-clip) to constrain to bounding box
    void saturate() noexcept
    {
        int ti = static_cast<int> (topology);
        x = softClipScaled (x, bounds[ti].xMin, bounds[ti].xMax);
        y = softClipScaled (y, bounds[ti].yMin, bounds[ti].yMax);
        z = softClipScaled (z, bounds[ti].zMin, bounds[ti].zMax);
    }

    void reset() noexcept
    {
        x = 0.1f; y = 0.1f; z = 0.1f;
        lastDxDt = 0.0f; lastDyDt = 0.0f;
    }

    float lastDxDt = 0.0f;
    float lastDyDt = 0.0f;

private:
    // Soft clip within a scaled range, then return in original scale
    static float softClipScaled (float val, float lo, float hi) noexcept
    {
        float range = hi - lo;
        float mid = (hi + lo) * 0.5f;
        float halfRange = range * 0.5f;
        if (halfRange < 0.001f) return mid;
        // Normalize to [-1, 1]
        float norm = (val - mid) / halfRange;
        // Cubic saturator: x * (1.5 - 0.5 * x * x)
        if (norm > 1.0f) norm = 1.0f;
        else if (norm < -1.0f) norm = -1.0f;
        else norm = norm * (1.5f - 0.5f * norm * norm);
        return mid + norm * halfRange;
    }
};

//==============================================================================
// HalfBandFilter — 12-tap half-band FIR for 2:1 downsampling.
//
// Half-band symmetry: every other coefficient is zero, so only 6 multiplies
// per output sample. Used in a two-stage cascade for 4:1 downsampling.
//==============================================================================
struct HalfBandFilter
{
    // 12-tap half-band FIR coefficients (Parks-McClellan optimized)
    // Only non-zero taps stored; the zero-valued taps are implicit.
    static constexpr int kNumTaps = 12;
    static constexpr float coeffs[6] = {
        -0.0157914f,  0.0f,  0.0907024f,  0.0f,  -0.3135643f,  0.5f
        // Symmetric: mirror for second half. Center tap = 0.5.
        // Full kernel: [c0, 0, c2, 0, c4, 0.5, c4, 0, c2, 0, c0, 0]
    };

    float delay[kNumTaps] {};

    void reset() noexcept { std::memset (delay, 0, sizeof (delay)); }

    // Push two input samples, return one output sample (2:1 decimation)
    float process (float in0, float in1) noexcept
    {
        // Shift delay line
        for (int i = kNumTaps - 1; i >= 2; --i)
            delay[i] = delay[i - 2];
        delay[1] = in1;
        delay[0] = in0;

        // Compute output using half-band symmetry
        float out = delay[5] * 0.5f;  // center tap
        out += (delay[0] + delay[10]) * coeffs[0];
        out += (delay[2] + delay[8])  * coeffs[2];
        out += (delay[4] + delay[6])  * coeffs[4];
        return out;
    }
};

//==============================================================================
// DCBlocker — First-order high-pass at 5 Hz.
//==============================================================================
struct DCBlocker
{
    float x1 = 0.0f, y1 = 0.0f;
    float R = 0.9993f; // default for 44100 Hz

    void prepare (double sampleRate) noexcept
    {
        constexpr double twoPi = 6.283185307179586;
        R = static_cast<float> (1.0 - twoPi * 5.0 / sampleRate);
        if (R < 0.9f) R = 0.9f;
    }

    void reset() noexcept { x1 = 0.0f; y1 = 0.0f; }

    float process (float in) noexcept
    {
        float out = in - x1 + R * y1;
        x1 = in;
        y1 = flushDenormal (out);
        return out;
    }
};

//==============================================================================
// OuroborosVoice — One chaotic attractor organism.
//==============================================================================
struct OuroborosVoice
{
    bool active = false;
    bool released = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;
    float stealFadeGain = 1.0f;
    float stealFadeStep = 0.0f;

    // Dual attractor states (for topology crossfade)
    AttractorState attractorA;
    AttractorState attractorB;
    bool crossfading = false;
    float crossfadeGain = 0.0f;   // 0 = full A, 1 = full B
    float crossfadeStep = 0.0f;

    // Leash: master phasor for hard-sync
    double phasorPhase = 0.0;
    double phasorInc = 0.0;
    // Separate attractor for synced path
    AttractorState syncedAttractor;

    // Envelope (fixed ADSR)
    enum class EnvStage { Attack, Decay, Sustain, Release, Off };
    EnvStage envStage = EnvStage::Off;
    float envLevel = 0.0f;
    float envAttackCoeff = 0.0f;
    float envDecayCoeff = 0.0f;
    float envReleaseCoeff = 0.0f;
    static constexpr float kSustainLevel = 0.8f;

    // Downsampling (two-stage 2:1 for 4:1 total)
    HalfBandFilter downsampleStage1L, downsampleStage1R;
    HalfBandFilter downsampleStage2L, downsampleStage2R;

    // DC blocking
    DCBlocker dcBlockerL, dcBlockerR;

    // Damping accumulators
    float dampL = 0.0f, dampR = 0.0f;

    // Velocity injection transient (50ms boost on note-on)
    float injectionBoost = 0.0f;
    float injectionBoostDecay = 0.0f;

    void prepare (double sampleRate) noexcept
    {
        dcBlockerL.prepare (sampleRate);
        dcBlockerR.prepare (sampleRate);

        // ADSR coefficients (fixed values)
        float sr = static_cast<float> (sampleRate);
        envAttackCoeff  = 1.0f / (0.005f * sr);  // 5ms attack
        envDecayCoeff   = 1.0f / (0.100f * sr);  // 100ms decay
        envReleaseCoeff = 1.0f / (0.200f * sr);  // 200ms release

        resetVoice();
    }

    void resetVoice() noexcept
    {
        active = false;
        released = false;
        noteNumber = -1;
        velocity = 0.0f;
        stealFadeGain = 1.0f;
        stealFadeStep = 0.0f;
        crossfading = false;
        crossfadeGain = 0.0f;
        crossfadeStep = 0.0f;
        envStage = EnvStage::Off;
        envLevel = 0.0f;
        phasorPhase = 0.0;
        phasorInc = 0.0;
        dampL = 0.0f;
        dampR = 0.0f;
        injectionBoost = 0.0f;

        attractorA.reset();
        attractorB.reset();
        syncedAttractor.reset();
        downsampleStage1L.reset(); downsampleStage1R.reset();
        downsampleStage2L.reset(); downsampleStage2R.reset();
        dcBlockerL.reset(); dcBlockerR.reset();
    }

    void noteOn (int note, float vel, uint64_t time, double sampleRate) noexcept
    {
        active = true;
        released = false;
        noteNumber = note;
        velocity = vel;
        startTime = time;
        stealFadeGain = 1.0f;
        stealFadeStep = 0.0f;

        // Leash phasor resets to 0 on note-on
        float freq = midiToFreq (note);
        phasorPhase = 0.0;
        phasorInc = static_cast<double> (freq) / sampleRate;

        // Sync attractor starts from current attractor A position
        syncedAttractor = attractorA;

        // ADSR starts
        envStage = EnvStage::Attack;

        // Velocity → injection boost for 50ms transient
        injectionBoost = vel * 0.5f;
        injectionBoostDecay = injectionBoost / (0.050f * static_cast<float> (sampleRate));

        // Reset downsampling and DC blocking for clean start
        downsampleStage1L.reset(); downsampleStage1R.reset();
        downsampleStage2L.reset(); downsampleStage2R.reset();
        dcBlockerL.reset(); dcBlockerR.reset();
        dampL = 0.0f; dampR = 0.0f;
    }

    void noteOff() noexcept
    {
        released = true;
        envStage = EnvStage::Release;
    }

    void beginStealFade (double sampleRate) noexcept
    {
        int fadeSamples = static_cast<int> (sampleRate * 0.005); // 5ms
        if (fadeSamples < 1) fadeSamples = 1;
        stealFadeStep = stealFadeGain / static_cast<float> (fadeSamples);
    }

    // Legato: retune without resetting attractor
    void legatoRetrigger (int note, float vel, uint64_t time, double sampleRate) noexcept
    {
        noteNumber = note;
        velocity = vel;
        startTime = time;
        released = false;

        // Update phasor to new frequency (attractor state preserved)
        float freq = midiToFreq (note);
        phasorInc = static_cast<double> (freq) / sampleRate;

        // ADSR continues from current level (no retrigger)
        if (envStage == EnvStage::Release || envStage == EnvStage::Off)
            envStage = EnvStage::Attack;
    }

    // Advance ADSR one sample
    float advanceEnvelope() noexcept
    {
        switch (envStage)
        {
            case EnvStage::Attack:
                envLevel += envAttackCoeff;
                if (envLevel >= 1.0f) { envLevel = 1.0f; envStage = EnvStage::Decay; }
                break;
            case EnvStage::Decay:
                envLevel -= envDecayCoeff * (envLevel - kSustainLevel);
                if (envLevel <= kSustainLevel + 0.001f) { envLevel = kSustainLevel; envStage = EnvStage::Sustain; }
                break;
            case EnvStage::Sustain:
                envLevel = kSustainLevel;
                break;
            case EnvStage::Release:
                envLevel -= envReleaseCoeff * envLevel;
                if (envLevel < 0.001f) { envLevel = 0.0f; envStage = EnvStage::Off; active = false; }
                break;
            case EnvStage::Off:
                envLevel = 0.0f;
                break;
        }
        return envLevel;
    }
};

//==============================================================================
// OuroborosEngine — Chaotic Attractor Synthesis.
//
// "XO-Ouroboros generates audio by continuously solving chaotic differential
//  equations, producing sounds completely alien to subtractive or wavetable
//  synthesis."
//
// Sound is produced by:
//   1. Selecting a chaotic ODE system (Lorenz, Rössler, Chua, Aizawa)
//   2. Integrating it with RK4 at 4x oversampled audio rate
//   3. Taming pitch via Phase-Locked Chaos ("The Leash")
//   4. Projecting the 3D trajectory to stereo via rotation matrix
//
// The result: swirling, organic, never-repeating waveforms that can be
// dialed from sludgy chaos to perfectly pitched complex wavetables.
//==============================================================================
class OuroborosEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 6;
    static constexpr int kOversample = 4;

    //-- Identity --------------------------------------------------------------

    juce::String getEngineId() const override { return "Ouroboros"; }

    juce::Colour getAccentColour() const override
    {
        return juce::Colour (0xFFFF2D2D); // Strange Attractor Red
    }

    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override
    {
        return activeVoiceCount.load (std::memory_order_relaxed);
    }

    //-- Lifecycle -------------------------------------------------------------

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        blockSize = maxBlockSize;
        noteCounter = 0;
        currentTopology = AttractorTopology::Lorenz;

        for (auto& voice : voices)
            voice.prepare (sampleRate);

        // 4-channel output cache: L, R, dx/dt, dy/dt
        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheDxDt.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheDyDt.resize (static_cast<size_t> (maxBlockSize), 0.0f);
    }

    void releaseResources() override
    {
        outputCacheL.clear();
        outputCacheR.clear();
        outputCacheDxDt.clear();
        outputCacheDyDt.clear();
    }

    void reset() override
    {
        for (auto& voice : voices)
            voice.resetVoice();

        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
        std::fill (outputCacheDxDt.begin(), outputCacheDxDt.end(), 0.0f);
        std::fill (outputCacheDyDt.begin(), outputCacheDyDt.end(), 0.0f);

        externalPitchMod = 0.0f;
        externalDampingMod = 0.0f;
        externalThetaMod = 0.0f;
        externalChaosMod = 0.0f;
        couplingAudioActive = false;
    }

    //-- Audio -----------------------------------------------------------------

    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi,
                      int numSamples) override
    {
        // ParamSnapshot: read all parameters once per block
        const int   topology_i    = paramTopology    ? static_cast<int> (paramTopology->load())   : 0;
        const float rate          = paramRate         ? paramRate->load()         : 130.0f;
        const float chaosIndex    = paramChaosIndex   ? paramChaosIndex->load()   : 0.3f;
        const float leash         = paramLeash        ? paramLeash->load()        : 0.5f;
        const float theta         = paramTheta        ? paramTheta->load()        : 0.0f;
        const float phi           = paramPhi          ? paramPhi->load()          : 0.0f;
        const float damping       = paramDamping      ? paramDamping->load()      : 0.3f;
        const float injection     = paramInjection    ? paramInjection->load()    : 0.0f;

        const auto newTopology = static_cast<AttractorTopology> (clamp (static_cast<float> (topology_i), 0.0f, 3.0f));

        // Detect topology change
        bool topologyChanged = (newTopology != currentTopology);
        if (topologyChanged)
            currentTopology = newTopology;

        // Effective chaos index with coupling modulation
        float effectiveChaos = clamp (chaosIndex + externalChaosMod, 0.0f, 1.0f);

        // Precompute projection rotation matrix
        float cosTheta = std::cos (theta + externalThetaMod);
        float sinTheta = std::sin (theta + externalThetaMod);
        float cosPhi   = std::cos (phi);
        float sinPhi   = std::sin (phi);

        // Damping alpha for LP accumulator
        float dampAlpha = 1.0f - (damping + externalDampingMod) * 0.95f;
        dampAlpha = clamp (dampAlpha, 0.05f, 1.0f);

        // Handle MIDI
        for (const auto metadata : midi)
        {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                handleNoteOn (msg.getNoteNumber(), msg.getFloatVelocity());
            else if (msg.isNoteOff())
                handleNoteOff (msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                for (auto& voice : voices)
                    voice.resetVoice();
            }
        }

        // Process each output sample
        for (int s = 0; s < numSamples; ++s)
        {
            float mixL = 0.0f;
            float mixR = 0.0f;
            float mixDxDt = 0.0f;
            float mixDyDt = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                // Handle topology crossfade initiation
                if (topologyChanged && s == 0 && !voice.crossfading)
                {
                    // Normalize A's state to [0,1]^3
                    float nx, ny, nz;
                    voice.attractorA.normalizeToUnit (nx, ny, nz);
                    // Initialize B with new topology
                    voice.attractorB.topology = newTopology;
                    voice.attractorB.initFromUnit (nx, ny, nz);
                    voice.syncedAttractor.topology = newTopology;
                    voice.syncedAttractor.initFromUnit (nx, ny, nz);
                    // Start crossfade (50ms)
                    voice.crossfading = true;
                    voice.crossfadeGain = 0.0f;
                    int fadeSamples = static_cast<int> (sr * 0.050);
                    voice.crossfadeStep = 1.0f / static_cast<float> (fadeSamples);
                }

                // Target frequency: MIDI note overrides rate parameter
                float targetFreq = midiToFreq (voice.noteNumber) + externalPitchMod * 20.0f;
                if (targetFreq < 20.0f) targetFreq = 20.0f;

                // Coupling injection (from external audio)
                float injDx = 0.0f, injDy = 0.0f;
                if (couplingAudioActive && injection > 0.001f)
                {
                    float injScale = injection;
                    // Velocity injection boost (decays over 50ms from note-on)
                    if (voice.injectionBoost > 0.0f)
                    {
                        injScale += voice.injectionBoost;
                        voice.injectionBoost -= voice.injectionBoostDecay;
                        if (voice.injectionBoost < 0.0f) voice.injectionBoost = 0.0f;
                    }
                    if (s < couplingBufferSize)
                    {
                        injDx = couplingAudioL[s] * injScale;
                        injDy = couplingAudioR[s] * injScale;
                    }
                }

                // --- 4x OVERSAMPLED ODE INTEGRATION ---
                // We need 4 oversampled points per output sample.
                // Accumulate projected L/R across the 4 sub-samples,
                // then downsample.

                float osL[4], osR[4]; // oversampled L/R before downsample

                for (int os = 0; os < kOversample; ++os)
                {
                    float freeX = 0.0f, freeY = 0.0f, freeZ = 0.0f;
                    float syncX = 0.0f, syncY = 0.0f, syncZ = 0.0f;
                    float freeDxDt = 0.0f, freeDyDt = 0.0f;
                    float syncDxDt = 0.0f, syncDyDt = 0.0f;

                    // --- FREE-RUNNING PATH ---
                    if (leash < 0.99f)
                    {
                        AttractorState& freeState = voice.crossfading ? voice.attractorB : voice.attractorA;
                        float h = freeState.computeStepSize (targetFreq, effectiveChaos);
                        freeState.step (h, effectiveChaos, injDx, injDy);
                        freeState.saturate();
                        freeX = freeState.x; freeY = freeState.y; freeZ = freeState.z;
                        freeDxDt = freeState.lastDxDt;
                        freeDyDt = freeState.lastDyDt;

                        // Also step A during crossfade (fading out)
                        if (voice.crossfading)
                        {
                            float hA = voice.attractorA.computeStepSize (targetFreq, effectiveChaos);
                            voice.attractorA.step (hA, effectiveChaos, injDx, injDy);
                            voice.attractorA.saturate();
                        }
                    }

                    // --- HARD-SYNCED PATH ---
                    if (leash > 0.01f)
                    {
                        // Advance phasor (at 4x rate)
                        voice.phasorPhase += voice.phasorInc / static_cast<double> (kOversample);

                        // Poincaré reset on phasor wrap
                        if (voice.phasorPhase >= 1.0)
                        {
                            voice.phasorPhase -= 1.0;
                            // Reset to Poincaré section: x = small perturbation, keep y/z normalized
                            int ti = static_cast<int> (voice.syncedAttractor.topology);
                            float bx = AttractorState::bounds[ti].xMin
                                      + 0.1f * (AttractorState::bounds[ti].xMax - AttractorState::bounds[ti].xMin);
                            voice.syncedAttractor.x = bx;
                            // y and z continue from current position (keeps timbre variation)
                        }

                        float hSync = voice.syncedAttractor.computeStepSize (targetFreq, effectiveChaos);
                        voice.syncedAttractor.step (hSync, effectiveChaos, injDx, injDy);
                        voice.syncedAttractor.saturate();
                        syncX = voice.syncedAttractor.x;
                        syncY = voice.syncedAttractor.y;
                        syncZ = voice.syncedAttractor.z;
                        syncDxDt = voice.syncedAttractor.lastDxDt;
                        syncDyDt = voice.syncedAttractor.lastDyDt;
                    }

                    // --- LEASH BLEND ---
                    float rawX, rawY, rawZ, dxOut, dyOut;
                    if (leash <= 0.01f)
                    {
                        rawX = freeX; rawY = freeY; rawZ = freeZ;
                        dxOut = freeDxDt; dyOut = freeDyDt;
                    }
                    else if (leash >= 0.99f)
                    {
                        rawX = syncX; rawY = syncY; rawZ = syncZ;
                        dxOut = syncDxDt; dyOut = syncDyDt;
                    }
                    else
                    {
                        float il = 1.0f - leash;
                        rawX = il * freeX + leash * syncX;
                        rawY = il * freeY + leash * syncY;
                        rawZ = il * freeZ + leash * syncZ;
                        dxOut = il * freeDxDt + leash * syncDxDt;
                        dyOut = il * freeDyDt + leash * syncDyDt;
                    }

                    // --- TOPOLOGY CROSSFADE ---
                    if (voice.crossfading && leash < 0.99f)
                    {
                        float hA = voice.attractorA.computeStepSize (targetFreq, effectiveChaos);
                        (void) hA; // already stepped above
                        float aX = voice.attractorA.x, aY = voice.attractorA.y, aZ = voice.attractorA.z;
                        float cf = voice.crossfadeGain;
                        rawX = (1.0f - cf) * aX + cf * rawX;
                        rawY = (1.0f - cf) * aY + cf * rawY;
                        rawZ = (1.0f - cf) * aZ + cf * rawZ;
                    }

                    // --- NORMALIZE TO [-1, 1] FOR PROJECTION ---
                    int ti = voice.crossfading
                             ? static_cast<int> (voice.attractorB.topology)
                             : static_cast<int> (voice.attractorA.topology);
                    float normX = (rawX - (AttractorState::bounds[ti].xMin + AttractorState::bounds[ti].xMax) * 0.5f)
                                / ((AttractorState::bounds[ti].xMax - AttractorState::bounds[ti].xMin) * 0.5f);
                    float normY = (rawY - (AttractorState::bounds[ti].yMin + AttractorState::bounds[ti].yMax) * 0.5f)
                                / ((AttractorState::bounds[ti].yMax - AttractorState::bounds[ti].yMin) * 0.5f);
                    float normZ = (rawZ - (AttractorState::bounds[ti].zMin + AttractorState::bounds[ti].zMax) * 0.5f)
                                / ((AttractorState::bounds[ti].zMax - AttractorState::bounds[ti].zMin) * 0.5f);

                    // --- 3D → 2D PROJECTION (rotation matrix) ---
                    // Rx(theta) then Ry(phi)
                    float ry = cosTheta * normY - sinTheta * normZ;
                    float rz = sinTheta * normY + cosTheta * normZ;
                    float projL = cosPhi * normX + sinPhi * rz;
                    float projR = ry;

                    osL[os] = projL;
                    osR[os] = projR;

                    // Store velocity for coupling output (last sub-sample)
                    if (os == kOversample - 1)
                    {
                        int maxVelTi = static_cast<int> (voice.crossfading
                                       ? voice.attractorB.topology
                                       : voice.attractorA.topology);
                        float maxVel = AttractorState::maxVelocity[maxVelTi];
                        mixDxDt += clamp (dxOut / maxVel, -1.0f, 1.0f);
                        mixDyDt += clamp (dyOut / maxVel, -1.0f, 1.0f);
                    }
                }

                // --- DOWNSAMPLE 4:1 (two-stage half-band) ---
                // Stage 1: 4x → 2x (process pairs)
                float ds1L = voice.downsampleStage1L.process (osL[0], osL[1]);
                float ds1R = voice.downsampleStage1R.process (osR[0], osR[1]);
                float ds2L = voice.downsampleStage1L.process (osL[2], osL[3]);
                float ds2R = voice.downsampleStage1R.process (osR[2], osR[3]);
                // Stage 2: 2x → 1x
                float outL = voice.downsampleStage2L.process (ds1L, ds2L);
                float outR = voice.downsampleStage2R.process (ds1R, ds2R);

                // --- DC BLOCKING ---
                outL = voice.dcBlockerL.process (outL);
                outR = voice.dcBlockerR.process (outR);

                // --- DAMPING (LP accumulator) ---
                voice.dampL = voice.dampL * (1.0f - dampAlpha) + outL * dampAlpha;
                voice.dampR = voice.dampR * (1.0f - dampAlpha) + outR * dampAlpha;
                outL = voice.dampL;
                outR = voice.dampR;

                // --- SOFT CLIP ---
                outL = fastTanh (outL);
                outR = fastTanh (outR);

                // --- ENVELOPE ---
                float env = voice.advanceEnvelope();
                outL *= env * voice.velocity;
                outR *= env * voice.velocity;

                // --- VOICE STEAL CROSSFADE ---
                if (voice.stealFadeStep > 0.0f)
                {
                    voice.stealFadeGain -= voice.stealFadeStep;
                    if (voice.stealFadeGain <= 0.0f) { voice.active = false; continue; }
                    outL *= voice.stealFadeGain;
                    outR *= voice.stealFadeGain;
                }

                // --- TOPOLOGY CROSSFADE ADVANCE ---
                if (voice.crossfading)
                {
                    voice.crossfadeGain += voice.crossfadeStep;
                    if (voice.crossfadeGain >= 1.0f)
                    {
                        // Crossfade complete: B becomes A
                        voice.attractorA = voice.attractorB;
                        voice.crossfading = false;
                        voice.crossfadeGain = 0.0f;
                    }
                }

                mixL += outL;
                mixR += outR;
            }

            // Cache output for coupling
            outputCacheL[static_cast<size_t> (s)] = mixL;
            outputCacheR[static_cast<size_t> (s)] = mixR;
            outputCacheDxDt[static_cast<size_t> (s)] = mixDxDt;
            outputCacheDyDt[static_cast<size_t> (s)] = mixDyDt;

            // Write to output buffer
            if (buffer.getNumChannels() > 0)
                buffer.getWritePointer (0)[s] += mixL;
            if (buffer.getNumChannels() > 1)
                buffer.getWritePointer (1)[s] += mixR;
        }

        // Reset coupling accumulators
        externalPitchMod = 0.0f;
        externalDampingMod = 0.0f;
        externalThetaMod = 0.0f;
        externalChaosMod = 0.0f;
        couplingAudioActive = false;

        // Update active voice count
        int count = 0;
        for (const auto& voice : voices)
            if (voice.active) ++count;
        activeVoiceCount.store (count, std::memory_order_relaxed);
    }

    //-- Coupling --------------------------------------------------------------

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        if (sampleIndex < 0 || sampleIndex >= static_cast<int> (outputCacheL.size()))
            return 0.0f;

        auto idx = static_cast<size_t> (sampleIndex);
        switch (channel)
        {
            case 0:  return outputCacheL[idx];
            case 1:  return outputCacheR[idx];
            case 2:  return outputCacheDxDt[idx];
            case 3:  return outputCacheDyDt[idx];
            default: return 0.0f;
        }
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* sourceBuffer, int numSamples) override
    {
        switch (type)
        {
            case CouplingType::AudioToFM:
            {
                // Audio injected into dx/dt (perturbation force)
                if (sourceBuffer != nullptr && numSamples > 0)
                {
                    couplingBufferSize = numSamples;
                    for (int i = 0; i < numSamples && i < kMaxCouplingBuffer; ++i)
                        couplingAudioL[i] = sourceBuffer[i] * amount;
                    couplingAudioActive = true;
                }
                break;
            }
            case CouplingType::AudioToWavetable:
            {
                // Audio injected into dy/dt (orthogonal perturbation)
                if (sourceBuffer != nullptr && numSamples > 0)
                {
                    couplingBufferSize = numSamples;
                    for (int i = 0; i < numSamples && i < kMaxCouplingBuffer; ++i)
                        couplingAudioR[i] = sourceBuffer[i] * amount;
                    couplingAudioActive = true;
                }
                break;
            }
            case CouplingType::RhythmToBlend:
                externalChaosMod += amount * 0.3f;
                break;
            case CouplingType::EnvToDecay:
            case CouplingType::AmpToFilter:
                externalDampingMod += amount * 0.5f;
                break;
            case CouplingType::EnvToMorph:
                externalThetaMod += amount * 1.0f;
                break;
            case CouplingType::LFOToPitch:
            case CouplingType::PitchToPitch:
                externalPitchMod += amount * 0.5f;
                break;
            default:
                break; // AudioToRing, FilterToFilter, AmpToChoke, AmpToPitch unsupported
        }
    }

    //-- Parameters ------------------------------------------------------------

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

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramTopology   = apvts.getRawParameterValue ("ouro_topology");
        paramRate       = apvts.getRawParameterValue ("ouro_rate");
        paramChaosIndex = apvts.getRawParameterValue ("ouro_chaosIndex");
        paramLeash      = apvts.getRawParameterValue ("ouro_leash");
        paramTheta      = apvts.getRawParameterValue ("ouro_theta");
        paramPhi        = apvts.getRawParameterValue ("ouro_phi");
        paramDamping    = apvts.getRawParameterValue ("ouro_damping");
        paramInjection  = apvts.getRawParameterValue ("ouro_injection");
    }

private:
    //-- Parameter definitions -------------------------------------------------

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        constexpr float pi = 3.14159265358979323846f;

        // 1. Topology (discrete: 0=Lorenz, 1=Rössler, 2=Chua, 3=Aizawa)
        params.push_back (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID ("ouro_topology", 1), "Topology",
            0, 3, 0));

        // 2. Orbit Rate (target pitch frequency in Hz)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_rate", 1), "Orbit Rate",
            juce::NormalisableRange<float> (0.01f, 20000.0f, 0.0f, 0.3f), 130.0f));

        // 3. Chaos Index (bifurcation parameter)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_chaosIndex", 1), "Chaos Index",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

        // 4. Leash (free-running ↔ hard-synced)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_leash", 1), "Leash",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // 5. Theta projection (rotation X-axis)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_theta", 1), "Theta",
            juce::NormalisableRange<float> (-pi, pi), 0.0f));

        // 6. Phi projection (rotation Y-axis)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_phi", 1), "Phi",
            juce::NormalisableRange<float> (-pi, pi), 0.0f));

        // 7. Damping (LP accumulator smoothing)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_damping", 1), "Damping",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

        // 8. Injection (coupling perturbation depth)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_injection", 1), "Injection",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    }

    //-- Voice management ------------------------------------------------------

    void handleNoteOn (int note, float velocity) noexcept
    {
        ++noteCounter;

        // Find a free voice
        int freeSlot = -1;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (!voices[i].active)
            {
                freeSlot = i;
                break;
            }
        }

        // If no free voice, steal the oldest (LRU)
        if (freeSlot < 0)
        {
            uint64_t oldest = UINT64_MAX;
            freeSlot = 0;
            for (int i = 0; i < kMaxVoices; ++i)
            {
                if (voices[i].startTime < oldest)
                {
                    oldest = voices[i].startTime;
                    freeSlot = i;
                }
            }
            voices[freeSlot].beginStealFade (sr);
        }

        // Set topology on new voice
        voices[freeSlot].attractorA.topology = currentTopology;
        voices[freeSlot].syncedAttractor.topology = currentTopology;

        voices[freeSlot].noteOn (note, velocity, noteCounter, sr);
    }

    void handleNoteOff (int note) noexcept
    {
        for (auto& voice : voices)
        {
            if (voice.active && !voice.released && voice.noteNumber == note)
            {
                voice.noteOff();
                break;
            }
        }
    }

    //-- State -----------------------------------------------------------------

    double sr = 44100.0;
    int blockSize = 512;
    uint64_t noteCounter = 0;
    AttractorTopology currentTopology = AttractorTopology::Lorenz;

    std::array<OuroborosVoice, kMaxVoices> voices;
    std::atomic<int> activeVoiceCount { 0 };

    // 4-channel output cache for coupling
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;
    std::vector<float> outputCacheDxDt;
    std::vector<float> outputCacheDyDt;

    // Coupling accumulators (reset per block)
    float externalPitchMod = 0.0f;
    float externalDampingMod = 0.0f;
    float externalThetaMod = 0.0f;
    float externalChaosMod = 0.0f;
    bool couplingAudioActive = false;

    // Coupling audio buffer (pre-allocated)
    static constexpr int kMaxCouplingBuffer = 4096;
    float couplingAudioL[kMaxCouplingBuffer] {};
    float couplingAudioR[kMaxCouplingBuffer] {};
    int couplingBufferSize = 0;

    // Cached parameter pointers
    std::atomic<float>* paramTopology   = nullptr;
    std::atomic<float>* paramRate       = nullptr;
    std::atomic<float>* paramChaosIndex = nullptr;
    std::atomic<float>* paramLeash      = nullptr;
    std::atomic<float>* paramTheta      = nullptr;
    std::atomic<float>* paramPhi        = nullptr;
    std::atomic<float>* paramDamping    = nullptr;
    std::atomic<float>* paramInjection  = nullptr;
};

} // namespace xomnibus
