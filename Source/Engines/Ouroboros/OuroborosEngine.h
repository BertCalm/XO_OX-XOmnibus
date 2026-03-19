#pragma once
#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/EngineProfiler.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>
#include <cstring>
#include <atomic>
#include <vector>

//==============================================================================
//
//  OuroborosEngine.h — Chaotic Attractor Synthesis
//  XO_OX Designs | Engine: OUROBOROS | Accent: Strange Attractor Red #FF2D2D
//
//  ---------------------------------------------------------------------------
//
//  AQUATIC MYTHOLOGY
//    Creature:  The Hydrothermal Vent
//    Depth:     The Abyss — pure Oscar polarity
//    Identity:  Self-feeding, recursive, primordial energy. Where raw
//               thermodynamic chaos becomes the origin of life itself.
//    Lineage:   Fourth Generation — Deep Evolution. Born from the coupling
//               of feliX's electric transients and Oscar's patient depth,
//               OUROBOROS descended to where no other species survives.
//               It feeds on its own output. It never repeats.
//
//  ---------------------------------------------------------------------------
//
//  SYNTH HERITAGE & TECHNIQUE
//    This engine generates audio by continuously solving chaotic ordinary
//    differential equations (ODEs), producing sounds completely alien to
//    subtractive or wavetable synthesis.
//
//    The four attractor topologies are drawn from the canon of nonlinear
//    dynamics:
//
//      Lorenz   — Edward Lorenz (1963). Originally a simplified atmospheric
//                 convection model. The "butterfly effect" system. Produces
//                 the iconic double-scroll attractor with broad harmonic
//                 spectrum.
//
//      Rossler  — Otto Rossler (1976). Designed as a minimal chaotic system
//                 with only one nonlinear term (sz * sx). Produces a
//                 single-scroll spiral with softer, more tonal character.
//
//      Chua     — Leon Chua (1983). The first physical electronic circuit
//                 proven to exhibit chaos. Uses a piecewise-linear diode
//                 function. Produces double-scroll chaos with a distinctive
//                 buzzy, electronic timbre.
//
//      Aizawa   — Yoji Aizawa (1982). A toroidal chaotic system with rich
//                 geometric structure. Produces spiraling trajectories with
//                 complex modulation character.
//
//    Integration uses the classical 4th-order Runge-Kutta method (RK4)
//    at 4x oversampled audio rate for stability and alias suppression.
//    Pitch tracking is achieved via Phase-Locked Chaos ("The Leash") —
//    a master phasor that periodically resets the attractor to a Poincare
//    section, forcing a fundamental frequency while preserving chaotic
//    timbral evolution.
//
//    The 3D attractor trajectory is projected to stereo via rotation
//    matrices (theta/phi), creating spatial movement that is intrinsic
//    to the chaos rather than applied as a post-process.
//
//    Historical precedent: Chua's circuit has been built as a hardware
//    synthesizer module. Academic work by Rodet, Bilotta, and others
//    explored chaotic oscillators for sound synthesis in the 1990s-2000s.
//    OUROBOROS brings these ideas to a playable instrument context with
//    pitch control, polyphony, and real-time parameter modulation.
//
//  ---------------------------------------------------------------------------
//
//  SIGNAL FLOW
//    Per Voice (x6 polyphonic):
//      MIDI Note -> Leash Phasor (pitch reference)
//                -> Attractor ODE (Lorenz/Rossler/Chua/Aizawa)
//                -> RK4 Integration at 4x oversample
//                -> Leash Blend (free-running <-> hard-synced)
//                -> 3D-to-Stereo Projection (theta/phi rotation)
//                -> Two-Stage Half-Band Decimation (4x -> 1x)
//                -> DC Blocker (5 Hz HPF)
//                -> Damping (LP accumulator)
//                -> Soft Clip (tanh saturation)
//                -> ADSR Envelope
//                -> Voice Sum
//                         |
//                   Coupling Bus -> dx/dt, dy/dt velocity outputs
//
//  ===========================================================================

namespace xomnibus {

//==============================================================================
// AttractorTopology — Enum for the four chaotic ODE systems.
//
// Each topology produces a qualitatively different attractor geometry and
// therefore a different timbral character. The chaos index parameter sweeps
// the bifurcation parameter of each system, moving from periodic orbits
// through period-doubling cascades into full chaos.
//==============================================================================
enum class AttractorTopology : int
{
    Lorenz  = 0,    // Double-scroll, broad spectrum, "butterfly" chaos
    Rossler = 1,    // Single-scroll spiral, softer/more tonal
    Chua    = 2,    // Electronic double-scroll, buzzy/metallic timbre
    Aizawa  = 3     // Toroidal spiral, complex modulation character
};

//==============================================================================
// AttractorState — State vector and RK4 integration for one attractor instance.
//
// Each voice holds two of these: one active, one for topology crossfade.
// All state is pre-allocated — zero dynamic memory on the audio thread.
//
// The attractor state (x, y, z) represents a point in 3D phase space.
// As the ODE is integrated forward in time, this point traces the chaotic
// trajectory that becomes our audio signal after projection to stereo.
//==============================================================================
struct AttractorState
{
    float x = 0.1f, y = 0.1f, z = 0.1f;
    AttractorTopology topology = AttractorTopology::Lorenz;

    //--------------------------------------------------------------------------
    // Per-topology bounding boxes for normalization.
    //
    // These define the expected range of each state variable for each topology.
    // Derived empirically by running each attractor at maximum chaos index
    // for extended periods and observing the attractor's basin of attraction.
    // Used to normalize the raw ODE output to [-1, 1] for audio projection,
    // and to constrain runaway trajectories via the soft-clip saturator.
    //--------------------------------------------------------------------------
    struct BoundingBox { float xMin, xMax, yMin, yMax, zMin, zMax; };

    static constexpr BoundingBox bounds[4] = {
        //                 X range        Y range         Z range
        { -25.0f, 25.0f, -30.0f, 30.0f,   0.0f, 55.0f },  // Lorenz: wide X/Y wings, Z always positive
        { -12.0f, 12.0f, -12.0f, 12.0f,   0.0f, 25.0f },  // Rossler: symmetric X/Y spiral, Z spikes
        {  -3.0f,  3.0f,  -0.5f,  0.5f,  -4.0f,  4.0f },  // Chua: narrow Y (diode clipping), wide X/Z
        {  -1.5f,  1.5f,  -1.5f,  1.5f,  -0.5f,  2.5f }   // Aizawa: compact toroidal envelope
    };

    //--------------------------------------------------------------------------
    // Per-topology calibration constants for step size derivation.
    //
    // baseStepSize:       The integration step h that produces the calibration
    //                     frequency for each topology. Tuned so that the
    //                     attractor's fundamental orbital period matches a
    //                     known pitch, allowing frequency-proportional scaling.
    //
    // calibrationFreqHz:  The pitch (Hz) at which baseStepSize was measured.
    //                     Step size scales as (targetFreq / calibrationFreq) * baseStepSize.
    //
    // maxSafeStepSize:    Upper bound on h to prevent numerical instability.
    //                     If h exceeds this, the RK4 integrator can diverge.
    //                     Reduced further at high chaos index (more sensitive).
    //
    // maxVelocityNorm:    Normalization constant for coupling velocity output.
    //                     The derivative magnitudes differ vastly between
    //                     topologies (Lorenz peaks ~200, Aizawa ~10), so we
    //                     normalize to [-1, 1] for consistent coupling behavior.
    //--------------------------------------------------------------------------
    static constexpr float baseStepSize[4]      = { 0.005f, 0.01f,  0.002f, 0.005f };
    static constexpr float calibrationFreqHz[4] = { 130.0f, 95.0f,  200.0f, 110.0f };
    static constexpr float maxSafeStepSize[4]   = { 0.02f,  0.05f,  0.008f, 0.015f };
    static constexpr float maxVelocityNorm[4]   = { 200.0f, 30.0f,  50.0f,  10.0f  };

    //--------------------------------------------------------------------------
    // Compute derivatives for the selected topology.
    //
    // Each case implements the canonical ODE system with its standard
    // parameter names from the literature. The `chaosIndex` parameter
    // (0..1) sweeps the bifurcation parameter that controls the transition
    // from periodic to chaotic behavior.
    //--------------------------------------------------------------------------
    void derivatives (float stateX, float stateY, float stateZ, float chaosIndex,
                      float& dxdt, float& dydt, float& dzdt) const noexcept
    {
        switch (topology)
        {
            case AttractorTopology::Lorenz:
            {
                // Lorenz (1963): Atmospheric convection model.
                // sigma = Prandtl number, rho = Rayleigh number, beta = geometric factor.
                // Chaos onset at rho ~ 24.74 (we sweep 20..32 via chaosIndex).
                constexpr float sigma = 10.0f;               // Prandtl number (standard value)
                constexpr float beta  = 8.0f / 3.0f;         // Geometric factor (standard value)
                float rho = 20.0f + chaosIndex * 12.0f;      // Rayleigh number: 20 (periodic) -> 32 (chaotic)
                dxdt = sigma * (stateY - stateX);
                dydt = stateX * (rho - stateZ) - stateY;
                dzdt = stateX * stateY - beta * stateZ;
                break;
            }
            case AttractorTopology::Rossler:
            {
                // Rossler (1976): Minimal chaotic system.
                // a, b = coupling constants, c = nonlinearity strength.
                // Chaos onset around c ~ 5 (we sweep 3..18 via chaosIndex).
                constexpr float a = 0.2f;                    // X-Y coupling (standard value)
                constexpr float b = 0.2f;                    // Z offset (standard value)
                float c = 3.0f + chaosIndex * 15.0f;         // Nonlinearity: 3 (limit cycle) -> 18 (chaotic)
                dxdt = -(stateY + stateZ);
                dydt = stateX + a * stateY;
                dzdt = b + stateZ * (stateX - c);
                break;
            }
            case AttractorTopology::Chua:
            {
                // Chua (1983): First electronic chaos circuit.
                // alpha = main bifurcation parameter, beta_c = secondary coupling.
                // m0, m1 = slopes of Chua's piecewise-linear diode function.
                constexpr float betaChua  = 28.0f;           // Secondary coupling (standard value)
                constexpr float m0 = -1.143f;                // Inner diode slope (standard value)
                constexpr float m1 = -0.714f;                // Outer diode slope (standard value)
                float alphaChua = 9.0f + chaosIndex * 7.0f;  // Main bifurcation: 9 (periodic) -> 16 (double-scroll)

                // Chua's diode: piecewise-linear characteristic function
                // h(x) = m1*x + 0.5*(m0-m1)*(|x+1| - |x-1|)
                // This creates the distinctive breakpoints at x = +/-1
                float diodeOutput = m1 * stateX
                    + 0.5f * (m0 - m1) * (std::fabs (stateX + 1.0f) - std::fabs (stateX - 1.0f));
                dxdt = alphaChua * (stateY - stateX - diodeOutput);
                dydt = stateX - stateY + stateZ;
                dzdt = -betaChua * stateY;
                break;
            }
            case AttractorTopology::Aizawa:
            {
                // Aizawa (1982): Toroidal chaotic system.
                // Parameters named per Aizawa's original paper notation.
                constexpr float aCoeff = 0.95f;              // Linear Z growth rate
                constexpr float bCoeff = 0.7f;               // Z offset in X/Y coupling
                constexpr float gammaCoeff = 0.6f;           // Z constant forcing term
                constexpr float deltaCoeff = 3.5f;           // X-Y rotation rate
                constexpr float zetaCoeff = 0.1f;            // Cubic X-Z cross-coupling
                float epsilonCoeff = 0.1f + chaosIndex * 0.85f; // Toroidal distortion: 0.1 (torus) -> 0.95 (chaotic)
                dxdt = (stateZ - bCoeff) * stateX - deltaCoeff * stateY;
                dydt = deltaCoeff * stateX + (stateZ - bCoeff) * stateY;
                float radiusSquared = stateX * stateX + stateY * stateY;
                dzdt = gammaCoeff + aCoeff * stateZ - (stateZ * stateZ * stateZ) / 3.0f
                     - radiusSquared * (1.0f + epsilonCoeff * stateZ)
                     + zetaCoeff * stateZ * stateX * stateX * stateX;
                break;
            }
        }
    }

    //--------------------------------------------------------------------------
    // Compute integration step size h from target pitch frequency.
    //
    // The step size is proportional to frequency: higher pitch = larger steps
    // = faster traversal of the attractor = higher fundamental frequency.
    // Clamped to maxSafeStepSize to prevent RK4 divergence. The safety
    // margin tightens at high chaos index because chaotic trajectories are
    // more sensitive to integration error.
    //--------------------------------------------------------------------------
    float computeStepSize (float targetFreqHz, float chaosIndex) const noexcept
    {
        int topologyIndex = static_cast<int> (topology);
        float stepSize = (targetFreqHz / calibrationFreqHz[topologyIndex]) * baseStepSize[topologyIndex];

        // Reduce maximum step size at high chaos (30% tighter at chaosIndex=1.0)
        // because chaotic trajectories magnify integration errors exponentially
        float safeMaximum = maxSafeStepSize[topologyIndex] * (1.0f - chaosIndex * 0.3f);

        if (stepSize > safeMaximum) stepSize = safeMaximum;
        if (stepSize < 0.0001f)     stepSize = 0.0001f;  // Floor: prevent stalled integration
        return stepSize;
    }

    //--------------------------------------------------------------------------
    // Single RK4 integration step with external velocity injection.
    //
    // Classical 4th-order Runge-Kutta: evaluates derivatives at 4 points
    // per step (start, two midpoints, end) and combines them with the
    // standard 1/6, 1/3, 1/3, 1/6 weighting for O(h^5) local error.
    //
    // External injection (injDx, injDy) adds a perturbation force to the
    // derivatives, allowing coupled engines to push the attractor's
    // trajectory without modifying the underlying ODE system.
    //--------------------------------------------------------------------------
    void step (float stepSize, float chaosIndex, float injectionDx, float injectionDy) noexcept
    {
        float dx1, dy1, dz1, dx2, dy2, dz2, dx3, dy3, dz3, dx4, dy4, dz4;

        // k1: derivatives at current state
        derivatives (x, y, z, chaosIndex, dx1, dy1, dz1);
        dx1 += injectionDx; dy1 += injectionDy;

        // k2: derivatives at midpoint using k1
        float halfStep = stepSize * 0.5f;
        derivatives (x + halfStep * dx1, y + halfStep * dy1, z + halfStep * dz1, chaosIndex, dx2, dy2, dz2);
        dx2 += injectionDx; dy2 += injectionDy;

        // k3: derivatives at midpoint using k2
        derivatives (x + halfStep * dx2, y + halfStep * dy2, z + halfStep * dz2, chaosIndex, dx3, dy3, dz3);
        dx3 += injectionDx; dy3 += injectionDy;

        // k4: derivatives at endpoint using k3
        derivatives (x + stepSize * dx3, y + stepSize * dy3, z + stepSize * dz3, chaosIndex, dx4, dy4, dz4);
        dx4 += injectionDx; dy4 += injectionDy;

        // Weighted combination: (k1 + 2*k2 + 2*k3 + k4) / 6
        float weightedStep = stepSize / 6.0f;
        x += weightedStep * (dx1 + 2.0f * dx2 + 2.0f * dx3 + dx4);
        y += weightedStep * (dy1 + 2.0f * dy2 + 2.0f * dy3 + dy4);
        z += weightedStep * (dz1 + 2.0f * dz2 + 2.0f * dz3 + dz4);

        // Store velocities for coupling output (other engines can read dx/dt, dy/dt)
        lastDxDt = dx1;
        lastDyDt = dy1;

        // Flush denormals: chaotic systems can produce extremely small values
        // near fixed points or during transient decay. Without flushing, these
        // subnormal floats cause 10-100x CPU spikes on x86 due to microcode
        // assist traps in the FPU pipeline.
        x = flushDenormal (x);
        y = flushDenormal (y);
        z = flushDenormal (z);
    }

    //--------------------------------------------------------------------------
    // Normalize state to [0,1]^3 using this topology's bounding box.
    // Used during topology crossfade to map one attractor's state into
    // another's coordinate system via a topology-independent unit space.
    //--------------------------------------------------------------------------
    void normalizeToUnit (float& normalizedX, float& normalizedY, float& normalizedZ) const noexcept
    {
        int topologyIndex = static_cast<int> (topology);
        normalizedX = (x - bounds[topologyIndex].xMin) / (bounds[topologyIndex].xMax - bounds[topologyIndex].xMin);
        normalizedY = (y - bounds[topologyIndex].yMin) / (bounds[topologyIndex].yMax - bounds[topologyIndex].yMin);
        normalizedZ = (z - bounds[topologyIndex].zMin) / (bounds[topologyIndex].zMax - bounds[topologyIndex].zMin);
        normalizedX = clamp (normalizedX, 0.0f, 1.0f);
        normalizedY = clamp (normalizedY, 0.0f, 1.0f);
        normalizedZ = clamp (normalizedZ, 0.0f, 1.0f);
    }

    //--------------------------------------------------------------------------
    // Initialize state from [0,1]^3 mapped to this topology's bounding box.
    // Inverse of normalizeToUnit — used to seed a new topology's state from
    // the normalized position of the previous topology during crossfade.
    //--------------------------------------------------------------------------
    void initFromUnit (float normalizedX, float normalizedY, float normalizedZ) noexcept
    {
        int topologyIndex = static_cast<int> (topology);
        x = bounds[topologyIndex].xMin + normalizedX * (bounds[topologyIndex].xMax - bounds[topologyIndex].xMin);
        y = bounds[topologyIndex].yMin + normalizedY * (bounds[topologyIndex].yMax - bounds[topologyIndex].yMin);
        z = bounds[topologyIndex].zMin + normalizedZ * (bounds[topologyIndex].zMax - bounds[topologyIndex].zMin);
    }

    //--------------------------------------------------------------------------
    // Apply cubic saturator (Hermite soft-clip) to constrain state within
    // the topology's bounding box. This prevents runaway trajectories at
    // high chaos index or large injection forces from producing NaN/Inf,
    // while preserving smooth waveform continuity (no hard clipping artifacts).
    //--------------------------------------------------------------------------
    void saturate() noexcept
    {
        int topologyIndex = static_cast<int> (topology);
        x = softClipScaled (x, bounds[topologyIndex].xMin, bounds[topologyIndex].xMax);
        y = softClipScaled (y, bounds[topologyIndex].yMin, bounds[topologyIndex].yMax);
        z = softClipScaled (z, bounds[topologyIndex].zMin, bounds[topologyIndex].zMax);
    }

    void reset() noexcept
    {
        // Initial state (0.1, 0.1, 0.1) places the point near the center of
        // all topologies' basins of attraction, ensuring rapid convergence
        // to the attractor rather than a long transient spiral-in.
        x = 0.1f; y = 0.1f; z = 0.1f;
        lastDxDt = 0.0f; lastDyDt = 0.0f;
    }

    float lastDxDt = 0.0f;     // Last computed dx/dt (for coupling velocity output)
    float lastDyDt = 0.0f;     // Last computed dy/dt (for coupling velocity output)

private:
    //--------------------------------------------------------------------------
    // Soft clip within a scaled range, then return in original scale.
    // Uses the Hermite cubic saturator: f(x) = x * (1.5 - 0.5 * x^2)
    // which has unity gain at the origin and smoothly limits to +/-1.
    //--------------------------------------------------------------------------
    static float softClipScaled (float value, float lowerBound, float upperBound) noexcept
    {
        float range = upperBound - lowerBound;
        float midpoint = (upperBound + lowerBound) * 0.5f;
        float halfRange = range * 0.5f;
        if (halfRange < 0.001f) return midpoint;          // Degenerate range guard

        // Normalize to [-1, 1] relative to the bounding box
        float normalized = (value - midpoint) / halfRange;

        // Hermite cubic saturator: smooth limiting with C1 continuity
        if (normalized > 1.0f) normalized = 1.0f;
        else if (normalized < -1.0f) normalized = -1.0f;
        else normalized = normalized * (1.5f - 0.5f * normalized * normalized);

        return midpoint + normalized * halfRange;
    }
};

//==============================================================================
// HalfBandFilter — 12-tap half-band FIR for 2:1 downsampling.
//
// Half-band symmetry: every other coefficient is zero, so only 6 multiplies
// per output sample instead of 12. Used in a two-stage cascade (4x -> 2x -> 1x)
// for efficient 4:1 decimation after oversampled attractor integration.
//
// Coefficients designed via Parks-McClellan (Remez exchange) algorithm
// for equiripple stopband attenuation. The center tap is always 0.5 by
// the half-band property.
//==============================================================================
struct HalfBandFilter
{
    static constexpr int kNumTaps = 12;

    // Non-zero Parks-McClellan half-band coefficients.
    // Full kernel: [c0, 0, c2, 0, c4, 0.5, c4, 0, c2, 0, c0, 0]
    // Only the 6 unique values are stored; zero-valued taps are implicit.
    static constexpr float coefficients[6] = {
        -0.0157914f,    // Tap 0/10: outer sidelobe suppression
         0.0f,          // Tap 1/9:  zero (half-band property)
         0.0907024f,    // Tap 2/8:  transition band shaping
         0.0f,          // Tap 3/7:  zero (half-band property)
        -0.3135643f,    // Tap 4/6:  main lobe shaping
         0.5f           // Tap 5:    center tap (half-band identity)
    };

    float delayLine[kNumTaps] {};

    void reset() noexcept { std::memset (delayLine, 0, sizeof (delayLine)); }

    //--------------------------------------------------------------------------
    // Push two input samples, return one output sample (2:1 decimation).
    // Processes a pair of samples and produces one filtered output,
    // effectively halving the sample rate while suppressing aliases.
    //--------------------------------------------------------------------------
    float process (float inputEven, float inputOdd) noexcept
    {
        // Shift delay line by 2 positions (we consume 2 samples per output)
        for (int i = kNumTaps - 1; i >= 2; --i)
            delayLine[i] = delayLine[i - 2];
        delayLine[1] = inputOdd;
        delayLine[0] = inputEven;

        // Compute output using half-band symmetry (only 3 multiplies + center)
        float output = delayLine[5] * 0.5f;                          // Center tap
        output += (delayLine[0] + delayLine[10]) * coefficients[0];  // Outer pair
        output += (delayLine[2] + delayLine[8])  * coefficients[2];  // Middle pair
        output += (delayLine[4] + delayLine[6])  * coefficients[4];  // Inner pair
        return output;
    }
};

//==============================================================================
// OuroborosDCBlocker — First-order high-pass at 5 Hz.
//
// Removes DC offset that accumulates from asymmetric attractor trajectories
// and the soft-clip saturator. Without this, speakers would receive a
// sustained DC component that wastes headroom and can damage monitors.
//
// Transfer function: H(z) = (1 - z^-1) / (1 - R * z^-1)
// where R = 1 - 2*pi*fc/fs determines the -3dB cutoff frequency.
//==============================================================================
struct OuroborosDCBlocker
{
    float previousInput = 0.0f;
    float previousOutput = 0.0f;
    float feedbackCoefficient = 0.9993f;    // Default for 44100 Hz, fc ~= 5 Hz

    void prepare (double sampleRate) noexcept
    {
        // R = 1 - (2 * pi * cutoffHz / sampleRate)
        // At 5 Hz cutoff: passes all audible content, blocks DC and sub-infrasonic drift.
        // Floor at 0.9 prevents instability if sampleRate is unexpectedly low.
        constexpr double twoPi = 6.283185307179586;
        constexpr double cutoffHz = 5.0;
        feedbackCoefficient = static_cast<float> (1.0 - twoPi * cutoffHz / sampleRate);
        if (feedbackCoefficient < 0.9f) feedbackCoefficient = 0.9f;
    }

    void reset() noexcept { previousInput = 0.0f; previousOutput = 0.0f; }

    float process (float input) noexcept
    {
        float output = input - previousInput + feedbackCoefficient * previousOutput;
        previousInput = input;
        // Flush denormals: the feedback path (R * y[n-1]) can produce subnormal
        // values during silence, causing CPU spikes without audible benefit.
        previousOutput = flushDenormal (output);
        return output;
    }
};

//==============================================================================
// OuroborosVoice — One chaotic attractor organism.
//
// Each voice is a self-contained chaos engine: dual attractor states for
// topology crossfade, a leash phasor for pitch control, ADSR envelope,
// two-stage decimation filters, and DC blocking. The voice is the
// hydrothermal vent — self-sustaining, recursive, alive.
//==============================================================================
struct OuroborosVoice
{
    //-- Voice State -----------------------------------------------------------
    bool active = false;
    bool released = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;
    float stealFadeGain = 1.0f;
    float stealFadeStep = 0.0f;

    //-- Dual Attractor States (for topology crossfade) ------------------------
    // Two attractors allow seamless topology switching: A fades out while B
    // fades in over 50ms, with B initialized to A's normalized position in
    // the new topology's coordinate system.
    AttractorState attractorA;
    AttractorState attractorB;
    bool crossfading = false;
    float crossfadeGain = 0.0f;    // 0.0 = full A, 1.0 = full B
    float crossfadeStep = 0.0f;

    //-- Leash: Master Phasor for Phase-Locked Chaos ---------------------------
    // The leash is OUROBOROS's pitch control mechanism. A simple ramp phasor
    // at the target MIDI frequency periodically resets the synced attractor
    // to a Poincare section, forcing a fundamental period while preserving
    // chaotic timbral evolution above the fundamental.
    double leashPhasorPhase = 0.0;
    double leashPhasorIncrement = 0.0;
    AttractorState syncedAttractor;     // Separate attractor for the leashed path

    //-- Envelope (fixed ADSR) -------------------------------------------------
    enum class EnvelopeStage { Attack, Decay, Sustain, Release, Off };
    EnvelopeStage envelopeStage = EnvelopeStage::Off;
    float envelopeLevel = 0.0f;
    float envelopeAttackCoeff = 0.0f;
    float envelopeDecayCoeff = 0.0f;
    float envelopeReleaseCoeff = 0.0f;
    static constexpr float kSustainLevel = 0.8f;   // -1.9 dB sustain

    //-- Downsampling (two-stage 2:1 for 4:1 total) ----------------------------
    HalfBandFilter decimationStage1Left,  decimationStage1Right;
    HalfBandFilter decimationStage2Left,  decimationStage2Right;

    //-- DC Blocking -----------------------------------------------------------
    OuroborosDCBlocker dcBlockerLeft, dcBlockerRight;

    //-- Damping Accumulators --------------------------------------------------
    // Single-pole lowpass accumulators that smooth the raw chaotic output.
    // The damping parameter controls how much high-frequency chaos passes
    // through versus being absorbed into a warmer, slower-evolving texture.
    float dampingAccumulatorLeft = 0.0f;
    float dampingAccumulatorRight = 0.0f;

    //-- Velocity Injection Transient ------------------------------------------
    // 50ms boost on note-on that kicks the attractor harder, creating a
    // percussive onset transient proportional to velocity.
    float injectionBoost = 0.0f;
    float injectionBoostDecayRate = 0.0f;

    //--------------------------------------------------------------------------
    // Prepare: compute time-domain coefficients from sample rate.
    //--------------------------------------------------------------------------
    void prepare (double sampleRate) noexcept
    {
        dcBlockerLeft.prepare (sampleRate);
        dcBlockerRight.prepare (sampleRate);

        // ADSR time constants (fixed values — these define the organism's breath)
        float sampleRateFloat = static_cast<float> (sampleRate);
        envelopeAttackCoeff  = 1.0f / (0.005f * sampleRateFloat);  // 5ms attack:   fast onset, snap of the vent opening
        envelopeDecayCoeff   = 1.0f / (0.100f * sampleRateFloat);  // 100ms decay:  initial energy burst subsides
        envelopeReleaseCoeff = 1.0f / (0.200f * sampleRateFloat);  // 200ms release: thermal dissipation tail

        resetVoice();
    }

    //--------------------------------------------------------------------------
    // Full voice reset — returns the organism to dormant state.
    //--------------------------------------------------------------------------
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
        envelopeStage = EnvelopeStage::Off;
        envelopeLevel = 0.0f;
        leashPhasorPhase = 0.0;
        leashPhasorIncrement = 0.0;
        dampingAccumulatorLeft = 0.0f;
        dampingAccumulatorRight = 0.0f;
        injectionBoost = 0.0f;

        attractorA.reset();
        attractorB.reset();
        syncedAttractor.reset();
        decimationStage1Left.reset();  decimationStage1Right.reset();
        decimationStage2Left.reset();  decimationStage2Right.reset();
        dcBlockerLeft.reset();         dcBlockerRight.reset();
    }

    //--------------------------------------------------------------------------
    // Note on — awaken the organism.
    //--------------------------------------------------------------------------
    void noteOn (int note, float vel, uint64_t time, double sampleRate) noexcept
    {
        active = true;
        released = false;
        noteNumber = note;
        velocity = vel;
        startTime = time;
        stealFadeGain = 1.0f;
        stealFadeStep = 0.0f;

        // Leash phasor resets to 0 on note-on (new pitch cycle)
        float frequency = midiToFreq (note);
        leashPhasorPhase = 0.0;
        leashPhasorIncrement = static_cast<double> (frequency) / sampleRate;

        // Sync attractor starts from current attractor A position
        // (preserves timbral continuity across notes)
        syncedAttractor = attractorA;

        // Envelope: begin attack phase
        envelopeStage = EnvelopeStage::Attack;

        // Velocity injection: 50ms transient boost proportional to velocity.
        // This kicks the attractor harder on strong strikes, creating a
        // percussive onset that decays into the sustained chaos.
        injectionBoost = vel * 0.5f;
        injectionBoostDecayRate = injectionBoost / (0.050f * static_cast<float> (sampleRate));

        // Reset downsampling and DC blocking for clean start
        // (prevents clicks from stale filter state)
        decimationStage1Left.reset();  decimationStage1Right.reset();
        decimationStage2Left.reset();  decimationStage2Right.reset();
        dcBlockerLeft.reset();         dcBlockerRight.reset();
        dampingAccumulatorLeft = 0.0f;
        dampingAccumulatorRight = 0.0f;
    }

    //--------------------------------------------------------------------------
    // Note off — begin thermal dissipation (release phase).
    //--------------------------------------------------------------------------
    void noteOff() noexcept
    {
        released = true;
        envelopeStage = EnvelopeStage::Release;
    }

    //--------------------------------------------------------------------------
    // Begin voice steal fade (5ms crossfade to silence).
    //--------------------------------------------------------------------------
    void beginStealFade (double sampleRate) noexcept
    {
        int fadeSamples = static_cast<int> (sampleRate * 0.005);  // 5ms fade
        if (fadeSamples < 1) fadeSamples = 1;
        stealFadeStep = stealFadeGain / static_cast<float> (fadeSamples);
    }

    //--------------------------------------------------------------------------
    // Legato retrigger: retune without resetting attractor state.
    // The organism changes pitch but its chaotic trajectory continues
    // unbroken — preserving timbral evolution across legato passages.
    //--------------------------------------------------------------------------
    void legatoRetrigger (int note, float vel, uint64_t time, double sampleRate) noexcept
    {
        noteNumber = note;
        velocity = vel;
        startTime = time;
        released = false;

        // Update phasor to new frequency (attractor state preserved)
        float frequency = midiToFreq (note);
        leashPhasorIncrement = static_cast<double> (frequency) / sampleRate;

        // Envelope continues from current level (no retrigger)
        if (envelopeStage == EnvelopeStage::Release || envelopeStage == EnvelopeStage::Off)
            envelopeStage = EnvelopeStage::Attack;
    }

    //--------------------------------------------------------------------------
    // Advance ADSR envelope one sample.
    //--------------------------------------------------------------------------
    float advanceEnvelope() noexcept
    {
        switch (envelopeStage)
        {
            case EnvelopeStage::Attack:
                envelopeLevel += envelopeAttackCoeff;
                if (envelopeLevel >= 1.0f) { envelopeLevel = 1.0f; envelopeStage = EnvelopeStage::Decay; }
                break;
            case EnvelopeStage::Decay:
                envelopeLevel -= envelopeDecayCoeff * (envelopeLevel - kSustainLevel);
                if (envelopeLevel <= kSustainLevel + 0.001f) { envelopeLevel = kSustainLevel; envelopeStage = EnvelopeStage::Sustain; }
                break;
            case EnvelopeStage::Sustain:
                envelopeLevel = kSustainLevel;
                break;
            case EnvelopeStage::Release:
                envelopeLevel -= envelopeReleaseCoeff * envelopeLevel;
                if (envelopeLevel < 0.001f) { envelopeLevel = 0.0f; envelopeStage = EnvelopeStage::Off; active = false; }
                break;
            case EnvelopeStage::Off:
                envelopeLevel = 0.0f;
                break;
        }
        return envelopeLevel;
    }
};

//==============================================================================
//
//  OuroborosEngine — Chaotic Attractor Synthesis
//
//  The hydrothermal vent of the XO_OX ecosystem. Living in The Abyss at
//  pure Oscar polarity, OUROBOROS generates sound by continuously solving
//  chaotic differential equations — producing swirling, organic,
//  never-repeating waveforms that can be dialed from sludgy primordial
//  chaos to perfectly pitched complex timbres.
//
//  The serpent eats its own tail: the output feeds back into the system,
//  each cycle slightly different from the last, yet always recognizably
//  itself. This is synthesis at the thermodynamic boundary — where
//  mathematical chaos becomes musical order.
//
//  KEY PARAMETERS
//    Topology     Selects the chaotic ODE system (Lorenz/Rossler/Chua/Aizawa)
//    Chaos Index  Sweeps the bifurcation parameter from periodic to chaotic
//    Leash        Blend between free-running chaos and pitch-locked chaos
//    Theta/Phi    3D-to-stereo projection angles (spatial rotation)
//    Damping      LP accumulator smoothing (tames high-frequency chaos)
//    Injection    Coupling perturbation depth (how much other engines push)
//
//==============================================================================
class OuroborosEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 6;       // 6 simultaneous chaotic organisms
    static constexpr int kOversample = 4;      // 4x oversampling for RK4 stability + alias suppression

    //==========================================================================
    //  IDENTITY
    //==========================================================================

    juce::String getEngineId() const override { return "Ouroboros"; }

    juce::Colour getAccentColour() const override
    {
        return juce::Colour (0xFFFF2D2D);   // Strange Attractor Red — the glow of the hydrothermal vent
    }

    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override
    {
        return activeVoiceCount.load (std::memory_order_relaxed);
    }

    //==========================================================================
    //  LIFECYCLE
    //==========================================================================

    const EngineProfiler& getProfiler() const noexcept { return profiler; }

    void prepare (double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = sampleRate;
        currentBlockSize = maxBlockSize;
        noteCounter = 0;
        currentTopology = AttractorTopology::Lorenz;

        for (auto& voice : voices)
            voice.prepare (sampleRate);

        // 4-channel output cache: L audio, R audio, dx/dt velocity, dy/dt velocity
        outputCacheLeft.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheRight.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheVelocityX.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheVelocityY.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        profiler.prepare (sampleRate, maxBlockSize);
        profiler.setCpuBudgetFraction (0.22f);  // 22% CPU budget — generous for RK4 at 4x oversample
        aftertouch.prepare (sampleRate);

        silenceGate.prepare (sampleRate, maxBlockSize);
        silenceGate.setHoldTime (1000.0f);  // Ouroboros has infinite-sustain attractor voices
    }

    void releaseResources() override
    {
        outputCacheLeft.clear();
        outputCacheRight.clear();
        outputCacheVelocityX.clear();
        outputCacheVelocityY.clear();
    }

    void reset() override
    {
        for (auto& voice : voices)
            voice.resetVoice();

        std::fill (outputCacheLeft.begin(), outputCacheLeft.end(), 0.0f);
        std::fill (outputCacheRight.begin(), outputCacheRight.end(), 0.0f);
        std::fill (outputCacheVelocityX.begin(), outputCacheVelocityX.end(), 0.0f);
        std::fill (outputCacheVelocityY.begin(), outputCacheVelocityY.end(), 0.0f);

        couplingPitchModulation = 0.0f;
        couplingDampingModulation = 0.0f;
        couplingThetaModulation = 0.0f;
        couplingChaosModulation = 0.0f;
        couplingAudioActive = false;
    }

    //==========================================================================
    //  AUDIO RENDERING
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi,
                      int numSamples) override
    {
        EngineProfiler::ScopedMeasurement measurement (profiler);

        //----------------------------------------------------------------------
        // ParamSnapshot: read all parameters once per block (zero per-sample cost)
        //----------------------------------------------------------------------
        const int   topologySelection = paramTopology    ? static_cast<int> (paramTopology->load())   : 0;
        const float orbitRate         = paramRate         ? paramRate->load()         : 130.0f;
        const float chaosIndex        = paramChaosIndex   ? paramChaosIndex->load()   : 0.3f;
        const float leashAmount       = paramLeash        ? paramLeash->load()        : 0.5f;
        const float projectionTheta   = paramTheta        ? paramTheta->load()        : 0.0f;
        const float projectionPhi     = paramPhi          ? paramPhi->load()          : 0.0f;
        float dampingAmount     = paramDamping      ? paramDamping->load()      : 0.3f;
        float injectionDepth    = paramInjection    ? paramInjection->load()    : 0.0f;

        // D002: Macro reads — centered at 0.5, bipolar offset is (value - 0.5)
        const float macroChar  = paramMacroCharacter ? paramMacroCharacter->load() : 0.5f;
        const float macroMove  = paramMacroMovement  ? paramMacroMovement->load()  : 0.5f;
        const float macroCoup  = paramMacroCoupling  ? paramMacroCoupling->load()  : 0.5f;
        const float macroSpace = paramMacroSpace     ? paramMacroSpace->load()     : 0.5f;

        // D002: CHARACTER → damping (filter/tone control: low=bright resonant, high=muted)
        //       MOVEMENT → chaos index boost (modulation amount)
        //       COUPLING → injection depth (coupling send)
        //       SPACE → damping (also smooths output = more spatial)
        dampingAmount   = clamp (dampingAmount + (macroChar - 0.5f) * 0.4f, 0.01f, 0.99f);
        injectionDepth  = clamp (injectionDepth + (macroCoup - 0.5f) * 0.6f, 0.0f, 1.0f);
        // macroMove and macroSpace applied to effectiveChaos and damping below
        juce::ignoreUnused (macroSpace);

        const auto newTopology = static_cast<AttractorTopology> (
            clamp (static_cast<float> (topologySelection), 0.0f, 3.0f));

        //----------------------------------------------------------------------
        // Detect topology change (triggers 50ms crossfade on all active voices)
        //----------------------------------------------------------------------
        bool topologyChanged = (newTopology != currentTopology);
        if (topologyChanged)
            currentTopology = newTopology;

        //----------------------------------------------------------------------
        // Handle MIDI events
        //----------------------------------------------------------------------
        for (const auto metadata : midi)
        {
            auto message = metadata.getMessage();
            if (message.isNoteOn())
            {
                silenceGate.wake();
                handleNoteOn (message.getNoteNumber(), message.getFloatVelocity());
            }
            else if (message.isNoteOff())
                handleNoteOff (message.getNoteNumber());
            else if (message.isAllNotesOff() || message.isAllSoundOff())
            {
                for (auto& voice : voices)
                    voice.resetVoice();
            }
            // D006: channel pressure → aftertouch (applied to leash/chaos below)
            else if (message.isChannelPressure())
                aftertouch.setChannelPressure (message.getChannelPressureValue() / 127.0f);
            // D006: mod wheel (CC#1) → leash tension
            // Wheel up = tighter leash on the attractor — pitch snaps closer to
            // the played note, chaos becomes more musical and controlled.
            // Wheel down = default free-running chaos. Full wheel adds +0.4 leash.
            else if (message.isController() && message.getControllerNumber() == 1)
                modWheelAmount = message.getControllerValue() / 127.0f;
        }

        if (silenceGate.isBypassed() && midi.isEmpty()) { buffer.clear(); return; }

        // D006: smooth aftertouch pressure and compute modulation value
        aftertouch.updateBlock (numSamples);
        const float atPressure = aftertouch.getSmoothedPressure (0);

        //----------------------------------------------------------------------
        // Apply coupling modulation to chaos index
        //----------------------------------------------------------------------
        // D006: aftertouch deepens chaos (sensitivity 0.3) and loosens the leash
        // Full pressure adds up to +0.3 chaos and reduces leash by up to -0.3
        // D006: mod wheel tightens leash (sensitivity 0.4) — wheel up = more musical pitch control
        // Wheel creates counterpoint to aftertouch: one loosens chaos, the other reins it in.
        // D005: breathing LFO — modulates chaos for autonomous evolution
        const float lfoRate_  = paramLfoRate  ? paramLfoRate->load()  : 0.07f;
        const float lfoDepth_ = paramLfoDepth ? paramLfoDepth->load() : 0.2f;
        lfoPhase_ += lfoRate_ * static_cast<float> (numSamples) / static_cast<float> (currentSampleRate);
        if (lfoPhase_ >= 1.0f) lfoPhase_ -= 1.0f;
        float lfoVal = std::sin (lfoPhase_ * 6.28318530f) * lfoDepth_;

        // D002: MOVEMENT macro boosts chaos (±0.3 from center)
        // D005: breathing LFO modulates chaos index (±0.15 at full depth)
        float effectiveChaos = clamp (chaosIndex + couplingChaosModulation + atPressure * 0.3f + (macroMove - 0.5f) * 0.6f + lfoVal * 0.15f, 0.0f, 1.0f);
        float effectiveLeash = clamp (leashAmount - atPressure * 0.3f + modWheelAmount * 0.4f, 0.0f, 1.0f);

        //----------------------------------------------------------------------
        // Precompute 3D-to-stereo projection rotation matrix.
        // Rx(theta) * Ry(phi) projects the 3D attractor trajectory to L/R.
        // Theta rotates around X-axis (tilts Y/Z plane),
        // Phi rotates around Y-axis (tilts X/Z plane).
        //----------------------------------------------------------------------
        float cosTheta = std::cos (projectionTheta + couplingThetaModulation);
        float sinTheta = std::sin (projectionTheta + couplingThetaModulation);
        float cosPhi   = std::cos (projectionPhi);
        float sinPhi   = std::sin (projectionPhi);

        //----------------------------------------------------------------------
        // Damping coefficient for LP accumulator.
        // dampAlpha near 1.0 = no damping (raw chaos passes through).
        // dampAlpha near 0.05 = heavy damping (only slow evolution survives).
        // The 0.95 multiplier ensures we never reach alpha=0 (frozen state).
        //----------------------------------------------------------------------
        float dampAlpha = 1.0f - (dampingAmount + couplingDampingModulation) * 0.95f;
        dampAlpha = clamp (dampAlpha, 0.05f, 1.0f);

        //----------------------------------------------------------------------
        // Per-sample rendering loop
        //----------------------------------------------------------------------
        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            float voiceSumLeft = 0.0f;
            float voiceSumRight = 0.0f;
            float voiceSumVelocityX = 0.0f;
            float voiceSumVelocityY = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                //--------------------------------------------------------------
                // Topology crossfade initiation (50ms equal-power blend)
                //--------------------------------------------------------------
                if (topologyChanged && sampleIndex == 0 && !voice.crossfading)
                {
                    // Normalize A's state to topology-independent [0,1]^3
                    float unitX, unitY, unitZ;
                    voice.attractorA.normalizeToUnit (unitX, unitY, unitZ);

                    // Initialize B with new topology at equivalent position
                    voice.attractorB.topology = newTopology;
                    voice.attractorB.initFromUnit (unitX, unitY, unitZ);
                    voice.syncedAttractor.topology = newTopology;
                    voice.syncedAttractor.initFromUnit (unitX, unitY, unitZ);

                    // Begin 50ms crossfade (smooth topology transition)
                    voice.crossfading = true;
                    voice.crossfadeGain = 0.0f;
                    int crossfadeSamples = static_cast<int> (currentSampleRate * 0.050);
                    voice.crossfadeStep = 1.0f / static_cast<float> (crossfadeSamples);
                }

                //--------------------------------------------------------------
                // Target frequency: MIDI note overrides orbit rate parameter.
                // If noteNumber is valid, use MIDI pitch; otherwise fall back
                // to the rate parameter (drone/modular mode).
                //--------------------------------------------------------------
                float targetFrequency = (voice.noteNumber >= 0)
                    ? midiToFreq (voice.noteNumber)
                    : orbitRate;
                targetFrequency += couplingPitchModulation * 20.0f;   // +/- 20 Hz pitch mod range
                if (targetFrequency < 20.0f) targetFrequency = 20.0f; // Sub-audible floor

                //--------------------------------------------------------------
                // Coupling injection (external audio perturbs the attractor)
                //--------------------------------------------------------------
                float injectionForceX = 0.0f, injectionForceY = 0.0f;
                if (couplingAudioActive && injectionDepth > 0.001f)
                {
                    float injectionScale = injectionDepth;

                    // Velocity injection boost: 50ms transient that decays
                    // from note-on, adding percussive onset energy
                    if (voice.injectionBoost > 0.0f)
                    {
                        injectionScale += voice.injectionBoost;
                        voice.injectionBoost -= voice.injectionBoostDecayRate;
                        if (voice.injectionBoost < 0.0f) voice.injectionBoost = 0.0f;
                    }

                    if (sampleIndex < couplingBufferSize)
                    {
                        injectionForceX = couplingAudioBufferLeft[sampleIndex] * injectionScale;
                        injectionForceY = couplingAudioBufferRight[sampleIndex] * injectionScale;
                    }
                }

                //--------------------------------------------------------------
                // 4x OVERSAMPLED ODE INTEGRATION
                //
                // We compute 4 attractor steps per output sample, then
                // decimate with the half-band FIR cascade. This serves
                // two purposes:
                //   1. Numerical stability: RK4 needs small step sizes
                //   2. Anti-aliasing: chaotic signals have broadband content
                //--------------------------------------------------------------

                float oversampledLeft[4], oversampledRight[4];

                for (int oversampleIndex = 0; oversampleIndex < kOversample; ++oversampleIndex)
                {
                    float freeRunX = 0.0f, freeRunY = 0.0f, freeRunZ = 0.0f;
                    float syncedX = 0.0f, syncedY = 0.0f, syncedZ = 0.0f;
                    float freeRunVelocityX = 0.0f, freeRunVelocityY = 0.0f;
                    float syncedVelocityX = 0.0f, syncedVelocityY = 0.0f;

                    //----------------------------------------------------------
                    // FREE-RUNNING PATH: attractor evolves without pitch lock.
                    // Pure chaos — the vent's raw thermodynamic output.
                    //----------------------------------------------------------
                    if (effectiveLeash < 0.99f)
                    {
                        AttractorState& freeState = voice.crossfading ? voice.attractorB : voice.attractorA;
                        float stepSize = freeState.computeStepSize (targetFrequency, effectiveChaos);
                        freeState.step (stepSize, effectiveChaos, injectionForceX, injectionForceY);
                        freeState.saturate();
                        freeRunX = freeState.x; freeRunY = freeState.y; freeRunZ = freeState.z;
                        freeRunVelocityX = freeState.lastDxDt;
                        freeRunVelocityY = freeState.lastDyDt;

                        // Also step A during crossfade (fading out)
                        if (voice.crossfading)
                        {
                            float stepSizeA = voice.attractorA.computeStepSize (targetFrequency, effectiveChaos);
                            voice.attractorA.step (stepSizeA, effectiveChaos, injectionForceX, injectionForceY);
                            voice.attractorA.saturate();
                        }
                    }

                    //----------------------------------------------------------
                    // HARD-SYNCED PATH: leash phasor periodically resets the
                    // attractor to a Poincare section, forcing a fundamental
                    // frequency. The chaos still evolves between resets,
                    // producing rich harmonics above the forced fundamental.
                    //----------------------------------------------------------
                    if (effectiveLeash > 0.01f)
                    {
                        // Advance leash phasor at oversampled rate
                        voice.leashPhasorPhase += voice.leashPhasorIncrement / static_cast<double> (kOversample);

                        // Poincare reset on phasor wrap (one full cycle complete)
                        if (voice.leashPhasorPhase >= 1.0)
                        {
                            voice.leashPhasorPhase -= 1.0;

                            // Reset X to 10% into the bounding box (small perturbation
                            // from the boundary, ensuring the trajectory re-enters the
                            // attractor basin rather than landing at a fixed point).
                            // Y and Z continue from current position (preserves timbre).
                            int topologyIndex = static_cast<int> (voice.syncedAttractor.topology);
                            float perturbedX = AttractorState::bounds[topologyIndex].xMin
                                + 0.1f * (AttractorState::bounds[topologyIndex].xMax
                                         - AttractorState::bounds[topologyIndex].xMin);
                            voice.syncedAttractor.x = perturbedX;
                        }

                        float syncStepSize = voice.syncedAttractor.computeStepSize (targetFrequency, effectiveChaos);
                        voice.syncedAttractor.step (syncStepSize, effectiveChaos, injectionForceX, injectionForceY);
                        voice.syncedAttractor.saturate();
                        syncedX = voice.syncedAttractor.x;
                        syncedY = voice.syncedAttractor.y;
                        syncedZ = voice.syncedAttractor.z;
                        syncedVelocityX = voice.syncedAttractor.lastDxDt;
                        syncedVelocityY = voice.syncedAttractor.lastDyDt;
                    }

                    //----------------------------------------------------------
                    // LEASH BLEND: crossfade between free-running chaos and
                    // pitch-locked chaos. The "Leash" parameter is the
                    // serpent's collar — at 0 it roams free, at 1 it is
                    // perfectly tethered to MIDI pitch.
                    //----------------------------------------------------------
                    float blendedX, blendedY, blendedZ, velocityOutX, velocityOutY;
                    if (effectiveLeash <= 0.01f)
                    {
                        blendedX = freeRunX; blendedY = freeRunY; blendedZ = freeRunZ;
                        velocityOutX = freeRunVelocityX; velocityOutY = freeRunVelocityY;
                    }
                    else if (effectiveLeash >= 0.99f)
                    {
                        blendedX = syncedX; blendedY = syncedY; blendedZ = syncedZ;
                        velocityOutX = syncedVelocityX; velocityOutY = syncedVelocityY;
                    }
                    else
                    {
                        float freeWeight = 1.0f - effectiveLeash;
                        blendedX = freeWeight * freeRunX + effectiveLeash * syncedX;
                        blendedY = freeWeight * freeRunY + effectiveLeash * syncedY;
                        blendedZ = freeWeight * freeRunZ + effectiveLeash * syncedZ;
                        velocityOutX = freeWeight * freeRunVelocityX + effectiveLeash * syncedVelocityX;
                        velocityOutY = freeWeight * freeRunVelocityY + effectiveLeash * syncedVelocityY;
                    }

                    //----------------------------------------------------------
                    // TOPOLOGY CROSSFADE: blend between old (A) and new (B)
                    // topology states during a 50ms transition.
                    //----------------------------------------------------------
                    if (voice.crossfading && effectiveLeash < 0.99f)
                    {
                        float oldX = voice.attractorA.x;
                        float oldY = voice.attractorA.y;
                        float oldZ = voice.attractorA.z;
                        float crossfadePosition = voice.crossfadeGain;
                        blendedX = (1.0f - crossfadePosition) * oldX + crossfadePosition * blendedX;
                        blendedY = (1.0f - crossfadePosition) * oldY + crossfadePosition * blendedY;
                        blendedZ = (1.0f - crossfadePosition) * oldZ + crossfadePosition * blendedZ;
                    }

                    //----------------------------------------------------------
                    // NORMALIZE TO [-1, 1] FOR STEREO PROJECTION
                    // Map from topology-specific bounding box to centered
                    // unit range for consistent projection behavior.
                    //----------------------------------------------------------
                    int activeTopologyIndex = voice.crossfading
                             ? static_cast<int> (voice.attractorB.topology)
                             : static_cast<int> (voice.attractorA.topology);
                    float normalizedX = (blendedX - (AttractorState::bounds[activeTopologyIndex].xMin + AttractorState::bounds[activeTopologyIndex].xMax) * 0.5f)
                                / ((AttractorState::bounds[activeTopologyIndex].xMax - AttractorState::bounds[activeTopologyIndex].xMin) * 0.5f);
                    float normalizedY = (blendedY - (AttractorState::bounds[activeTopologyIndex].yMin + AttractorState::bounds[activeTopologyIndex].yMax) * 0.5f)
                                / ((AttractorState::bounds[activeTopologyIndex].yMax - AttractorState::bounds[activeTopologyIndex].yMin) * 0.5f);
                    float normalizedZ = (blendedZ - (AttractorState::bounds[activeTopologyIndex].zMin + AttractorState::bounds[activeTopologyIndex].zMax) * 0.5f)
                                / ((AttractorState::bounds[activeTopologyIndex].zMax - AttractorState::bounds[activeTopologyIndex].zMin) * 0.5f);

                    //----------------------------------------------------------
                    // 3D-TO-STEREO PROJECTION (rotation matrix)
                    //
                    // Apply Rx(theta) then Ry(phi) to the normalized 3D point.
                    // Left channel = X component after both rotations.
                    // Right channel = Y component after X-rotation only.
                    // This creates stereo width that is intrinsic to the
                    // attractor's geometry rather than a post-process effect.
                    //----------------------------------------------------------
                    float rotatedY = cosTheta * normalizedY - sinTheta * normalizedZ;
                    float rotatedZ = sinTheta * normalizedY + cosTheta * normalizedZ;
                    float projectedLeft  = cosPhi * normalizedX + sinPhi * rotatedZ;
                    float projectedRight = rotatedY;

                    oversampledLeft[oversampleIndex]  = projectedLeft;
                    oversampledRight[oversampleIndex] = projectedRight;

                    // Store velocity for coupling output (use last sub-sample only)
                    if (oversampleIndex == kOversample - 1)
                    {
                        int velocityTopologyIndex = static_cast<int> (voice.crossfading
                                       ? voice.attractorB.topology
                                       : voice.attractorA.topology);
                        float velocityNormFactor = AttractorState::maxVelocityNorm[velocityTopologyIndex];
                        voiceSumVelocityX += clamp (velocityOutX / velocityNormFactor, -1.0f, 1.0f);
                        voiceSumVelocityY += clamp (velocityOutY / velocityNormFactor, -1.0f, 1.0f);
                    }
                }

                //--------------------------------------------------------------
                // DOWNSAMPLE 4:1 (two-stage half-band FIR cascade)
                //
                // Stage 1: 4x -> 2x (process oversampled pairs 0+1, 2+3)
                // Stage 2: 2x -> 1x (process stage-1 outputs)
                //--------------------------------------------------------------
                float intermediate1Left  = voice.decimationStage1Left.process  (oversampledLeft[0],  oversampledLeft[1]);
                float intermediate1Right = voice.decimationStage1Right.process (oversampledRight[0], oversampledRight[1]);
                float intermediate2Left  = voice.decimationStage1Left.process  (oversampledLeft[2],  oversampledLeft[3]);
                float intermediate2Right = voice.decimationStage1Right.process (oversampledRight[2], oversampledRight[3]);
                float outputLeft  = voice.decimationStage2Left.process  (intermediate1Left,  intermediate2Left);
                float outputRight = voice.decimationStage2Right.process (intermediate1Right, intermediate2Right);

                //--------------------------------------------------------------
                // DC BLOCKING
                //--------------------------------------------------------------
                outputLeft  = voice.dcBlockerLeft.process  (outputLeft);
                outputRight = voice.dcBlockerRight.process (outputRight);

                //--------------------------------------------------------------
                // DAMPING (single-pole LP accumulator)
                //
                // Flush denormals in the feedback path: during quiet passages,
                // the accumulator value can decay into subnormal range. On x86
                // CPUs, subnormal arithmetic triggers microcode assists that
                // cost 10-100x normal FPU throughput — an inaudible value
                // causing very audible CPU spikes.
                //--------------------------------------------------------------
                voice.dampingAccumulatorLeft  = flushDenormal (voice.dampingAccumulatorLeft  * (1.0f - dampAlpha) + outputLeft  * dampAlpha);
                voice.dampingAccumulatorRight = flushDenormal (voice.dampingAccumulatorRight * (1.0f - dampAlpha) + outputRight * dampAlpha);
                outputLeft  = voice.dampingAccumulatorLeft;
                outputRight = voice.dampingAccumulatorRight;

                //--------------------------------------------------------------
                // SOFT CLIP (tanh saturation — prevents hard digital clipping)
                //--------------------------------------------------------------
                outputLeft  = fastTanh (outputLeft);
                outputRight = fastTanh (outputRight);

                //--------------------------------------------------------------
                // ENVELOPE
                //--------------------------------------------------------------
                float envelopeGain = voice.advanceEnvelope();
                outputLeft  *= envelopeGain * voice.velocity;
                outputRight *= envelopeGain * voice.velocity;

                //--------------------------------------------------------------
                // VOICE STEAL CROSSFADE
                //--------------------------------------------------------------
                if (voice.stealFadeStep > 0.0f)
                {
                    voice.stealFadeGain -= voice.stealFadeStep;
                    if (voice.stealFadeGain <= 0.0f) { voice.active = false; continue; }
                    outputLeft  *= voice.stealFadeGain;
                    outputRight *= voice.stealFadeGain;
                }

                //--------------------------------------------------------------
                // TOPOLOGY CROSSFADE ADVANCE
                //--------------------------------------------------------------
                if (voice.crossfading)
                {
                    voice.crossfadeGain += voice.crossfadeStep;
                    if (voice.crossfadeGain >= 1.0f)
                    {
                        // Crossfade complete: B becomes the new A
                        voice.attractorA = voice.attractorB;
                        voice.crossfading = false;
                        voice.crossfadeGain = 0.0f;
                    }
                }

                voiceSumLeft  += outputLeft;
                voiceSumRight += outputRight;
            }

            //------------------------------------------------------------------
            // Cache output for coupling (4 channels: L, R, dx/dt, dy/dt)
            //------------------------------------------------------------------
            outputCacheLeft[static_cast<size_t> (sampleIndex)]      = voiceSumLeft;
            outputCacheRight[static_cast<size_t> (sampleIndex)]     = voiceSumRight;
            outputCacheVelocityX[static_cast<size_t> (sampleIndex)] = voiceSumVelocityX;
            outputCacheVelocityY[static_cast<size_t> (sampleIndex)] = voiceSumVelocityY;

            //------------------------------------------------------------------
            // Write to output buffer (additive — other engines may be summing)
            //------------------------------------------------------------------
            if (buffer.getNumChannels() > 0)
                buffer.getWritePointer (0)[sampleIndex] += voiceSumLeft;
            if (buffer.getNumChannels() > 1)
                buffer.getWritePointer (1)[sampleIndex] += voiceSumRight;
        }

        //----------------------------------------------------------------------
        // Reset coupling accumulators for next block
        //----------------------------------------------------------------------
        couplingPitchModulation = 0.0f;
        couplingDampingModulation = 0.0f;
        couplingThetaModulation = 0.0f;
        couplingChaosModulation = 0.0f;
        couplingAudioActive = false;

        //----------------------------------------------------------------------
        // Update active voice count (atomic, relaxed — UI display only)
        //----------------------------------------------------------------------
        int count = 0;
        for (const auto& voice : voices)
            if (voice.active) ++count;
        activeVoiceCount.store (count, std::memory_order_relaxed);

        silenceGate.analyzeBlock (buffer.getReadPointer (0), buffer.getReadPointer (1), numSamples);
    }

    //==========================================================================
    //  COUPLING — The Symbiosis Interface
    //
    //  OUROBOROS provides 4 coupling output channels:
    //    Channel 0: Left audio output
    //    Channel 1: Right audio output
    //    Channel 2: dx/dt (X-axis attractor velocity, normalized)
    //    Channel 3: dy/dt (Y-axis attractor velocity, normalized)
    //
    //  The velocity outputs (channels 2-3) are unique to OUROBOROS and allow
    //  other engines to be modulated by the *rate of change* of the chaos,
    //  not just its instantaneous value.
    //==========================================================================

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        if (sampleIndex < 0 || sampleIndex >= static_cast<int> (outputCacheLeft.size()))
            return 0.0f;

        auto index = static_cast<size_t> (sampleIndex);
        switch (channel)
        {
            case 0:  return outputCacheLeft[index];       // Left audio
            case 1:  return outputCacheRight[index];      // Right audio
            case 2:  return outputCacheVelocityX[index];  // dx/dt (attractor velocity X)
            case 3:  return outputCacheVelocityY[index];  // dy/dt (attractor velocity Y)
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
                // Audio injected into dx/dt — perturbation force along X-axis.
                // Other engines' audio pushes the attractor sideways in phase space.
                if (sourceBuffer != nullptr && numSamples > 0)
                {
                    couplingBufferSize = numSamples;
                    for (int i = 0; i < numSamples && i < kMaxCouplingBuffer; ++i)
                        couplingAudioBufferLeft[i] = sourceBuffer[i] * amount;
                    couplingAudioActive = true;
                }
                break;
            }
            case CouplingType::AudioToWavetable:
            {
                // Audio injected into dy/dt — orthogonal perturbation force.
                // Combined with AudioToFM, this gives 2D steering of chaos.
                if (sourceBuffer != nullptr && numSamples > 0)
                {
                    couplingBufferSize = numSamples;
                    for (int i = 0; i < numSamples && i < kMaxCouplingBuffer; ++i)
                        couplingAudioBufferRight[i] = sourceBuffer[i] * amount;
                    couplingAudioActive = true;
                }
                break;
            }
            case CouplingType::RhythmToBlend:
                // Rhythm modulates chaos index (drives bifurcation parameter)
                couplingChaosModulation += amount * 0.3f;
                break;
            case CouplingType::EnvToDecay:
            case CouplingType::AmpToFilter:
                // Envelope/amplitude modulates damping (tames or unleashes chaos)
                couplingDampingModulation += amount * 0.5f;
                break;
            case CouplingType::EnvToMorph:
                // Envelope modulates projection theta (rotates the stereo image)
                couplingThetaModulation += amount * 1.0f;
                break;
            case CouplingType::LFOToPitch:
            case CouplingType::PitchToPitch:
                // LFO/pitch modulates orbit frequency
                couplingPitchModulation += amount * 0.5f;
                break;
            default:
                break;  // AudioToRing, FilterToFilter, AmpToChoke, AmpToPitch: not applicable to chaos synthesis
        }
    }

    //==========================================================================
    //  PARAMETERS
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
        paramLfoRate    = apvts.getRawParameterValue ("ouro_lfoRate");
        paramLfoDepth   = apvts.getRawParameterValue ("ouro_lfoDepth");
        paramMacroCharacter = apvts.getRawParameterValue ("ouro_macroCharacter");
        paramMacroMovement  = apvts.getRawParameterValue ("ouro_macroMovement");
        paramMacroCoupling  = apvts.getRawParameterValue ("ouro_macroCoupling");
        paramMacroSpace     = apvts.getRawParameterValue ("ouro_macroSpace");
    }

private:

    SilenceGate silenceGate;

    //==========================================================================
    //  PARAMETER DEFINITIONS
    //==========================================================================

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        constexpr float pi = 3.14159265358979323846f;

        // 1. Topology: selects the chaotic ODE system
        //    0=Lorenz (broad spectrum), 1=Rossler (tonal), 2=Chua (buzzy), 3=Aizawa (complex)
        params.push_back (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID ("ouro_topology", 1), "Topology",
            0, 3, 0));

        // 2. Orbit Rate: target pitch frequency in Hz (used in drone mode when no MIDI note active)
        //    Skew 0.3 gives finer control in the audible range (20-2000 Hz)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_rate", 1), "Orbit Rate",
            juce::NormalisableRange<float> (0.01f, 20000.0f, 0.0f, 0.3f), 130.0f));

        // 3. Chaos Index: sweeps the bifurcation parameter from periodic (0) to fully chaotic (1)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_chaosIndex", 1), "Chaos Index",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

        // 4. Leash: blend between free-running chaos (0) and hard-synced pitch-locked chaos (1)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_leash", 1), "Leash",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // 5. Theta: 3D projection rotation around X-axis (radians, -pi to +pi)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_theta", 1), "Theta",
            juce::NormalisableRange<float> (-pi, pi), 0.0f));

        // 6. Phi: 3D projection rotation around Y-axis (radians, -pi to +pi)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_phi", 1), "Phi",
            juce::NormalisableRange<float> (-pi, pi), 0.0f));

        // 7. Damping: LP accumulator smoothing (0=raw chaos, 1=heavy smoothing)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_damping", 1), "Damping",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

        // 8. Injection: coupling perturbation depth (how much external engines push the attractor)
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_injection", 1), "Injection",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // D005: breathing LFO for autonomous modulation
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_lfoRate", 1), "LFO Rate",
            juce::NormalisableRange<float> (0.005f, 5.0f, 0.001f, 0.3f), 0.07f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_lfoDepth", 1), "LFO Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.2f));

        // D002: 4 macros
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_macroCharacter", 1), "CHARACTER",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_macroMovement", 1), "MOVEMENT",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_macroCoupling", 1), "COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("ouro_macroSpace", 1), "SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    }

    //==========================================================================
    //  VOICE MANAGEMENT
    //==========================================================================

    void handleNoteOn (int note, float velocity) noexcept
    {
        ++noteCounter;

        // Find a free voice slot
        int freeSlot = -1;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (!voices[i].active)
            {
                freeSlot = i;
                break;
            }
        }

        // If no free voice, steal the oldest (LRU — least recently used)
        if (freeSlot < 0)
        {
            uint64_t oldestTime = UINT64_MAX;
            freeSlot = 0;
            for (int i = 0; i < kMaxVoices; ++i)
            {
                if (voices[i].startTime < oldestTime)
                {
                    oldestTime = voices[i].startTime;
                    freeSlot = i;
                }
            }
            voices[freeSlot].beginStealFade (currentSampleRate);
        }

        // Set topology on new voice
        voices[freeSlot].attractorA.topology = currentTopology;
        voices[freeSlot].syncedAttractor.topology = currentTopology;

        voices[freeSlot].noteOn (note, velocity, noteCounter, currentSampleRate);
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

    //==========================================================================
    //  ENGINE STATE
    //==========================================================================

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    uint64_t noteCounter = 0;
    AttractorTopology currentTopology = AttractorTopology::Lorenz;

    //-- Voice pool -------------------------------------------------------------
    std::array<OuroborosVoice, kMaxVoices> voices;
    std::atomic<int> activeVoiceCount { 0 };

    //-- Output cache for coupling (4 channels) --------------------------------
    std::vector<float> outputCacheLeft;          // Channel 0: Left audio
    std::vector<float> outputCacheRight;         // Channel 1: Right audio
    std::vector<float> outputCacheVelocityX;     // Channel 2: dx/dt velocity
    std::vector<float> outputCacheVelocityY;     // Channel 3: dy/dt velocity

    //-- Coupling accumulators (reset per block) -------------------------------
    float couplingPitchModulation = 0.0f;        // Accumulates LFOToPitch / PitchToPitch
    float couplingDampingModulation = 0.0f;      // Accumulates EnvToDecay / AmpToFilter
    float couplingThetaModulation = 0.0f;        // Accumulates EnvToMorph
    float couplingChaosModulation = 0.0f;        // Accumulates RhythmToBlend
    float lfoPhase_ = 0.0f;                     // D005: breathing LFO phase
    bool couplingAudioActive = false;

    //-- Coupling audio buffer (pre-allocated, no heap allocation on audio thread)
    static constexpr int kMaxCouplingBuffer = 4096;
    float couplingAudioBufferLeft[kMaxCouplingBuffer] {};
    float couplingAudioBufferRight[kMaxCouplingBuffer] {};
    int couplingBufferSize = 0;

    //-- Performance profiler --------------------------------------------------
    EngineProfiler profiler;

    // ---- D006 Aftertouch — pressure loosens the leash (more chaos depth) ----
    PolyAftertouch aftertouch;

    // D006: mod wheel (CC#1) — tightens leash tension (+0.4 at full wheel)
    float modWheelAmount = 0.0f;

    //-- Cached parameter pointers (attached once, read per-block) -------------
    std::atomic<float>* paramTopology   = nullptr;
    std::atomic<float>* paramRate       = nullptr;
    std::atomic<float>* paramChaosIndex = nullptr;
    std::atomic<float>* paramLeash      = nullptr;
    std::atomic<float>* paramTheta      = nullptr;
    std::atomic<float>* paramPhi        = nullptr;
    std::atomic<float>* paramDamping    = nullptr;
    std::atomic<float>* paramInjection  = nullptr;
    std::atomic<float>* paramLfoRate    = nullptr;
    std::atomic<float>* paramLfoDepth   = nullptr;
    std::atomic<float>* paramMacroCharacter = nullptr;
    std::atomic<float>* paramMacroMovement  = nullptr;
    std::atomic<float>* paramMacroCoupling  = nullptr;
    std::atomic<float>* paramMacroSpace     = nullptr;
};

} // namespace xomnibus
