#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/VoiceAllocator.h"
#include "../../DSP/ParameterSmoother.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace xolokun {

//==============================================================================
//
//  O B S C U R A   E N G I N E
//  Scanned Synthesis via Mass-Spring Chain Physics
//
//  ---- Creature Identity ----
//  The Giant Squid. Lives in THE ABYSS of the XO_OX water column.
//  Pure Oscar polarity -- sustained, deep, massive, ancient.
//  Stiff resonant body, tentacles probing the dark where nothing was
//  thought to live. Sound emerges not from oscillators but from the
//  physical motion of a vibrating chain -- the squid's body resonating
//  under impossible pressure.
//
//  Accent: Daguerreotype Silver #8A9BA8
//  Parameter prefix: obscura_
//
//  ---- Synth Lineage ----
//  Implements the scanned synthesis technique pioneered by Bill Verplank
//  and Max Mathews at CCRMA (Stanford, late 1990s). Their insight: instead
//  of computing waveforms mathematically, simulate a physical object (a
//  vibrating string, membrane, or chain of masses) and "scan" its shape
//  at audio rate to produce sound. The timbral character comes from the
//  physics itself -- stiffness, damping, nonlinearity, and boundary
//  conditions determine the harmonic content. This is the same principle
//  behind the Csound scanned synthesis opcodes (scanu/scans) and the
//  Max/MSP scanning~ external.
//
//  ---- Architecture ----
//  A 128-mass spring chain is simulated using Verlet integration at a
//  ~4 kHz control rate. A scanner sweeps across the chain at MIDI note
//  frequency, reading displacements via cubic Hermite interpolation to
//  produce audio-rate output. The physics simulation runs at a reduced
//  rate (1/11th of 44.1 kHz) for CPU efficiency, with linear interpolation
//  between snapshots for smooth audio output.
//
//  Signal Flow:
//    MIDI Note -> Chain Initialization (shape + Gaussian impulse)
//              -> Verlet Physics Simulation (stiffness, damping, nonlinear)
//              -> Scanner Readout (cubic interpolation, Hann-windowed averaging)
//              -> DC Blocker -> Soft Limiter -> Amp Envelope -> Output
//
//  Features:
//    - 128-mass spring chain with configurable stiffness, damping, nonlinearity
//    - Verlet integration (energy-stable, no velocity storage needed)
//    - Excitation: Gaussian impulse (note-on) + continuous bowing force
//    - 3 boundary modes: Fixed / Free / Periodic
//    - 4 initial chain shapes: Sine / Saw / Random / Flat
//    - Scanner with variable width (bright to dark timbres)
//    - Cubic Hermite interpolation for smooth audio-rate chain readout
//    - Stereo imaging: L channel scans forward, R channel scans backward
//    - DC blocker + soft limiter on output
//    - Dual ADSR envelopes: amplitude + physics excitation
//    - Mono/Legato/Poly4/Poly8 voice modes with LRU stealing + 5ms crossfade
//    - 2 LFOs (Sine/Tri/Saw/Square/S&H) -> scan width + excite position
//    - Full XOlokun coupling support
//
//  Coupling:
//    - Output: post-output stereo audio via getSampleForCoupling
//    - Input: AudioToFM (force on chain masses), AmpToFilter (modulates
//             stiffness), RhythmToBlend (triggers impulse excitation)
//
//==============================================================================


//==============================================================================
// Chain Constants
//==============================================================================

// Number of discrete masses in the spring chain. 128 provides a good balance
// between timbral resolution (more masses = richer harmonics) and CPU cost.
// This matches the Csound scanu opcode's default table size.
static constexpr int kChainSize = 128;


//==============================================================================
//
//  ObscuraADSR -- Envelope Generator
//
//  Replaced with fleet-standard StandardADSR (true exponential decay/release,
//  denormal-safe, sample-rate-aware). Drop-in compatible: same setParams(),
//  noteOn(), noteOff(), isActive(), reset() API.
//
//==============================================================================
using ObscuraADSR = StandardADSR;


//==============================================================================
//
//  ObscuraLFO -- Low-Frequency Oscillator
//
//  Replaced with fleet-standard StandardLFO (same 5 shapes, same LCG S&H,
//  same bipolar [-1,+1] output). Drop-in compatible: same setRate(),
//  setShape(int), process(), reset() API.
//
//==============================================================================
using ObscuraLFO = StandardLFO;


//==============================================================================
//
//  ObscuraDCBlocker -- Single-Pole High-Pass Filter (~5 Hz)
//
//  Removes DC offset that accumulates from asymmetric chain displacement,
//  nonlinear spring forces, and boundary condition artifacts. Without this,
//  the scanned synthesis output would drift away from zero over time,
//  causing clicks on note boundaries and wasting headroom.
//
//  Transfer function: H(z) = (1 - z^-1) / (1 - R * z^-1)
//  where R = 1 - (2*pi*fc / sr), fc = 5 Hz.
//
//==============================================================================
struct ObscuraDCBlocker
{
    float previousInput  = 0.0f;
    float previousOutput = 0.0f;

    // Feedback coefficient R. Higher values = lower cutoff frequency.
    // Range [0.9, 0.9999] keeps the filter stable and inaudible.
    float feedbackCoefficient = 0.995f;

    void setCoefficient (float sampleRate) noexcept
    {
        constexpr float kDCBlockerCutoffHz = 5.0f;
        constexpr float kTwoPi = 6.28318530718f;

        // R = 1 - (2*pi*fc / sr): places the pole just inside the unit circle.
        // At 44.1 kHz: R ~ 0.99929, giving a -3dB point near 5 Hz.
        feedbackCoefficient = 1.0f - (kTwoPi * kDCBlockerCutoffHz
                                      / std::max (1.0f, sampleRate));

        // Clamp to safe range to prevent instability at extreme sample rates
        if (feedbackCoefficient < 0.9f)    feedbackCoefficient = 0.9f;
        if (feedbackCoefficient > 0.9999f) feedbackCoefficient = 0.9999f;
    }

    float process (float input) noexcept
    {
        float output = input - previousInput + feedbackCoefficient * previousOutput;
        previousInput = input;
        // Flush denormals: the feedback path can produce subnormal floats that
        // cause massive CPU spikes on x86 (up to 100x slower per operation)
        // when the FPU falls back to microcode-assisted denormal handling.
        previousOutput = flushDenormal (output);
        return output;
    }

    void reset() noexcept
    {
        previousInput  = 0.0f;
        previousOutput = 0.0f;
    }
};


//==============================================================================
//
//  ObscuraVoice -- Per-Voice State
//
//  Each voice contains a complete 128-mass spring chain, dual control-rate
//  snapshots for audio-rate interpolation, a phase-accumulating scanner,
//  dual envelopes (amplitude + physics), dual LFOs, and stereo DC blockers.
//
//  The Verlet integrator requires two state arrays (current position and
//  previous position) -- velocity is implicit as the difference between
//  the two. This is more energy-stable than explicit Euler integration,
//  which tends to gain energy over time and blow up.
//
//==============================================================================
struct ObscuraVoice
{
    //-- Voice identity --------------------------------------------------------
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;

    //-- Mass-spring chain state -----------------------------------------------
    // Verlet integration needs both current and previous positions.
    // Velocity is derived implicitly: v[i] = chain[i] - chainPrevious[i].
    float chain[kChainSize] = {};
    float chainPrevious[kChainSize] = {};

    //-- Control-rate snapshots for audio-rate interpolation --------------------
    // The physics simulation runs at ~4 kHz (every ~11 audio samples at 44.1k).
    // Between physics steps, we linearly interpolate between snapshotA (the
    // previous physics frame) and snapshotB (the current physics frame) to
    // produce smooth audio-rate output without artifacts.
    float chainSnapshotA[kChainSize] = {};  // previous physics frame
    float chainSnapshotB[kChainSize] = {};  // current physics frame

    //-- Scanner ---------------------------------------------------------------
    float scannerPhase = 0.0f;  // [0, 1) phase accumulator for chain sweep

    //-- Glide (portamento) ----------------------------------------------------
    float currentFrequency = 440.0f;
    float targetFrequency  = 440.0f;
    float glideCoefficient = 1.0f;  // 1.0 = instant, smaller = slower glide

    //-- Control-rate accumulator ----------------------------------------------
    float controlPhaseAccumulator = 0.0f;

    //-- Excitation state ------------------------------------------------------
    bool impulseTriggered = false;
    float excitationForce = 0.0f;  // continuous bowing force amplitude

    //-- Envelopes -------------------------------------------------------------
    ObscuraADSR amplitudeEnvelope;
    ObscuraADSR physicsEnvelope;

    //-- LFOs (per-voice for phase independence) -------------------------------
    ObscuraLFO lfo1;
    ObscuraLFO lfo2;

    //-- DC blockers (stereo pair) ---------------------------------------------
    ObscuraDCBlocker dcBlockerLeft;
    ObscuraDCBlocker dcBlockerRight;

    //-- Voice-stealing crossfade ----------------------------------------------
    float crossfadeGain = 1.0f;
    bool fadingOut = false;

    //-- Pseudo-random number generator ----------------------------------------
    // Used for S&H LFO and random chain initialization shape.
    // LCG with Numerical Recipes constants (period 2^32).
    uint32_t randomGeneratorState = 42u;

    float nextRandom() noexcept
    {
        // Linear congruential generator: x[n+1] = (a * x[n] + c) mod 2^32
        // Multiplier a = 1664525 (Numerical Recipes), increment c = 1013904223
        randomGeneratorState = randomGeneratorState * 1664525u + 1013904223u;
        // Extract 16 bits, scale to [-1, +1] bipolar range
        return static_cast<float> (randomGeneratorState & 0xFFFF) / 32768.0f - 1.0f;
    }

    //--------------------------------------------------------------------------
    // Initialize the chain shape before a note begins.
    // The shape determines the initial harmonic content:
    //   0 = Sine:   fundamental-heavy, mellow attack
    //   1 = Saw:    harmonically rich, bright attack
    //   2 = Random: chaotic, noisy, unpredictable timbre
    //   3 = Flat:   silent until excitation -- pure impulse response
    //--------------------------------------------------------------------------
    void initChainShape (int shapeIndex, float excitationPosition,
                         float excitationWidth) noexcept
    {
        constexpr float kPi = 3.14159265358979323846f;

        // Initial displacement amplitude. Kept small (0.1 for deterministic,
        // 0.05 for random) to avoid immediate soft-clipping on the chain.
        constexpr float kDeterministicAmplitude = 0.1f;
        constexpr float kRandomAmplitude = 0.05f;

        for (int i = 0; i < kChainSize; ++i)
        {
            float normalizedPosition = static_cast<float> (i)
                                      / static_cast<float> (kChainSize - 1);
            float displacement = 0.0f;

            switch (shapeIndex)
            {
                case 0: // Sine -- one full period across the chain
                    displacement = std::sin (kPi * normalizedPosition)
                                 * kDeterministicAmplitude;
                    break;

                case 1: // Saw -- linear ramp from -1 to +1
                    displacement = (2.0f * normalizedPosition - 1.0f)
                                 * kDeterministicAmplitude;
                    break;

                case 2: // Random -- white noise displacement
                    displacement = nextRandom() * kRandomAmplitude;
                    break;

                case 3: // Flat -- zero displacement, excitation only
                default:
                    displacement = 0.0f;
                    break;
            }

            chain[i] = displacement;
            chainPrevious[i] = displacement;
        }

        // Apply initial Gaussian excitation impulse — D001: velocity scales
        // impulse amplitude so harder strikes excite more modes (brighter timbre).
        constexpr float kBaseImpulseAmplitude = 0.15f;
        float impulseAmplitude = kBaseImpulseAmplitude * (0.3f + 0.7f * velocity);
        applyImpulse (excitationPosition, excitationWidth,
                      impulseAmplitude);

        // Copy initial state to both snapshots so audio-rate interpolation
        // starts from a consistent state (no discontinuity on first frame)
        for (int i = 0; i < kChainSize; ++i)
        {
            chainSnapshotA[i] = chain[i];
            chainSnapshotB[i] = chain[i];
        }
    }

    //--------------------------------------------------------------------------
    // Apply a Gaussian impulse to the chain -- the "pluck" or "strike"
    // that sets masses in motion. The impulse is bell-curve shaped,
    // centered at `position` with width controlled by `width`. This
    // mimics how a physical string is excited: not at a single point,
    // but across a small region determined by the exciter's geometry
    // (finger, pick, hammer, bow).
    //--------------------------------------------------------------------------
    void applyImpulse (float position, float width, float amplitude) noexcept
    {
        float centerMass = position * static_cast<float> (kChainSize - 1);

        // Gaussian sigma: width=0 gives a narrow impulse (0.5 masses),
        // width=1 gives a broad impulse covering ~25% of the chain.
        float sigma = std::max (0.5f, width * static_cast<float> (kChainSize)
                                    * 0.25f);
        float inverseTwoSigmaSquared = 1.0f / (2.0f * sigma * sigma);

        // Skip endpoints (i=0, i=N-1) -- they are controlled by boundary
        // conditions and should not receive excitation force
        for (int i = 1; i < kChainSize - 1; ++i)
        {
            float distance = static_cast<float> (i) - centerMass;
            float gaussianEnvelope = amplitude
                                   * fastExp (-distance * distance
                                              * inverseTwoSigmaSquared);
            chain[i] += gaussianEnvelope;
        }
    }

    //--------------------------------------------------------------------------
    // Reset all voice state to silence.
    //--------------------------------------------------------------------------
    void reset() noexcept
    {
        active = false;
        noteNumber = -1;
        velocity = 0.0f;
        scannerPhase = 0.0f;
        currentFrequency = 440.0f;
        targetFrequency = 440.0f;
        glideCoefficient = 1.0f;
        controlPhaseAccumulator = 0.0f;
        impulseTriggered = false;
        excitationForce = 0.0f;
        crossfadeGain = 1.0f;
        fadingOut = false;
        randomGeneratorState = 42u;
        amplitudeEnvelope.reset();
        physicsEnvelope.reset();
        lfo1.reset();
        lfo2.reset();
        dcBlockerLeft.reset();
        dcBlockerRight.reset();

        for (int i = 0; i < kChainSize; ++i)
        {
            chain[i] = 0.0f;
            chainPrevious[i] = 0.0f;
            chainSnapshotA[i] = 0.0f;
            chainSnapshotB[i] = 0.0f;
        }
    }
};


//==============================================================================
//
//  O B S C U R A   E N G I N E
//
//  The giant squid of the XO_OX water column. Scanned synthesis engine
//  implementing Verplank/Mathews mass-spring chain physics. Each note
//  creates a vibrating chain whose shape IS the waveform -- the scanner
//  reads the chain's displacement like a record needle reading a groove,
//  but the groove is alive, evolving under physical forces.
//
//  The stiffness parameter controls how tightly the masses are coupled:
//  low stiffness = floppy string (warm, fundamental-heavy), high stiffness
//  = rigid bar (bright, inharmonic partials). The nonlinear spring adds
//  cubic restoring force that creates pitch-dependent harmonics -- louder
//  notes have brighter timbre, just like a real struck object.
//
//==============================================================================
class ObscuraEngine : public SynthEngine
{
public:

    //==========================================================================
    // Constants
    //==========================================================================

    static constexpr int   kMaxVoices  = 8;
    static constexpr float kPi         = 3.14159265358979323846f;
    static constexpr float kTwoPi      = 6.28318530717958647692f;

    // Physics simulation rate (~4 kHz). This is the rate at which Verlet
    // integration steps are computed. At 44.1 kHz audio rate, this means
    // roughly 1 physics step per 11 audio samples. Higher rates give more
    // accurate physics but cost more CPU; 4 kHz is the sweet spot identified
    // by Verplank's original scanned synthesis research.
    static constexpr float kPhysicsControlRate = 4000.0f;

    // Verlet stability limit: the spring constant k must satisfy k * dt^2 < 1.0.
    // Since we normalize dt^2 = 1.0 (the spring constant directly encodes the
    // effective k*dt^2 product), the maximum safe spring constant is < 1.0.
    // We use 0.95 to provide a 5% safety margin against instability.
    static constexpr float kMaxSpringConstant = 0.95f;

    // Soft-clip constants for chain displacement.
    // Keeps chain displacement bounded to prevent Verlet blowup.
    // tanh(x * 4.0) * 0.25 maps any displacement to [-0.25, +0.25] range,
    // with linear behavior near zero and smooth saturation at extremes.
    static constexpr float kChainSoftClipDrive = 4.0f;
    static constexpr float kChainSoftClipCeiling = 0.25f;


    //==========================================================================
    // SynthEngine Interface -- Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sampleRateDouble = sampleRate;
        sampleRateFloat = static_cast<float> (sampleRateDouble);
        silenceGate.prepare (sampleRate, maxBlockSize);

        // How many audio samples elapse between physics simulation steps.
        // At 44.1 kHz with 4 kHz control rate: ~11.025 samples per step.
        controlStepSamples = sampleRateFloat / kPhysicsControlRate;
        if (controlStepSamples < 1.0f) controlStepSamples = 1.0f;

        // Normalized dt^2 for Verlet integration. Set to 1.0 because
        // the spring constant already encodes the effective k*dt^2 product.
        // This simplifies the Verlet update to: x_new = 2x - x_old + F.
        normalizedDtSquared = 1.0f;

        // Parameter smoothing: 5ms time constant — prevents zipper noise.
        smoothedStiffness.prepare    (sampleRateFloat);
        smoothedDamping.prepare      (sampleRateFloat);
        smoothedNonlinearity.prepare (sampleRateFloat);
        smoothedScanWidth.prepare    (sampleRateFloat);
        smoothedSustainForce.prepare (sampleRateFloat);

        // Voice-stealing crossfade rate: linear ramp over 5ms.
        // Prevents clicks when a new note steals an active voice.
        constexpr float kCrossfadeTimeSeconds = 0.005f;
        voiceCrossfadeRate = 1.0f / (kCrossfadeTimeSeconds * sampleRateFloat);

        // Allocate output cache for coupling reads
        outputCacheLeft.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheRight.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        aftertouch.prepare (sampleRate);

        // Initialize all voices
        for (auto& voice : voices)
        {
            voice.reset();
            voice.dcBlockerLeft.setCoefficient (sampleRateFloat);
            voice.dcBlockerRight.setCoefficient (sampleRateFloat);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& voice : voices)
            voice.reset();

        envelopeOutput = 0.0f;
        couplingForceModulation = 0.0f;
        couplingStiffnessModulation = 0.0f;
        couplingImpulseTrigger = 0.0f;

        smoothedStiffness.snapTo    (0.5f);
        smoothedDamping.snapTo      (0.3f);
        smoothedNonlinearity.snapTo (0.0f);
        smoothedScanWidth.snapTo    (0.5f);
        smoothedSustainForce.snapTo (0.0f);

        std::fill (outputCacheLeft.begin(), outputCacheLeft.end(), 0.0f);
        std::fill (outputCacheRight.begin(), outputCacheRight.end(), 0.0f);
    }


    //==========================================================================
    // SynthEngine Interface -- Audio Rendering
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        //----------------------------------------------------------------------
        // ParamSnapshot: read all parameters once per block.
        // This avoids repeated atomic loads in the per-sample loop (the
        // XOlokun ParamSnapshot pattern -- see Architecture Rules).
        //----------------------------------------------------------------------

        // -- Core Physics --
        const float paramStiffness       = loadParam (pStiffness, 0.5f);
        const float paramDamping         = loadParam (pDamping, 0.3f);
        const float paramNonlinearity    = loadParam (pNonlinear, 0.0f);
        const float paramExcitePosition  = loadParam (pExcitePos, 0.5f);
        const float paramExciteWidth     = loadParam (pExciteWidth, 0.3f);
        const float paramScanWidth       = loadParam (pScanWidth, 0.5f);
        const int   paramBoundaryMode    = static_cast<int> (loadParam (pBoundary, 0.0f));
        const float paramSustainForce    = loadParam (pSustain, 0.0f);

        // -- Amplitude Envelope --
        const float paramAmpAttack       = loadParam (pAmpAttack, 0.01f);
        const float paramAmpDecay        = loadParam (pAmpDecay, 0.1f);
        const float paramAmpSustain      = loadParam (pAmpSustain, 0.8f);
        const float paramAmpRelease      = loadParam (pAmpRelease, 0.3f);

        // -- Physics Envelope --
        const float paramPhysAttack      = loadParam (pPhysAttack, 0.01f);
        const float paramPhysDecay       = loadParam (pPhysDecay, 0.2f);
        const float paramPhysSustain     = loadParam (pPhysSustain, 0.5f);
        const float paramPhysRelease     = loadParam (pPhysRelease, 0.3f);

        // -- LFO 1 --
        const float paramLfo1Rate        = loadParam (pLfo1Rate, 1.0f);
        const float paramLfo1Depth       = loadParam (pLfo1Depth, 0.0f);
        const int   paramLfo1Shape       = static_cast<int> (loadParam (pLfo1Shape, 0.0f));

        // -- LFO 2 --
        const float paramLfo2Rate        = loadParam (pLfo2Rate, 1.0f);
        const float paramLfo2Depth       = loadParam (pLfo2Depth, 0.0f);
        const int   paramLfo2Shape       = static_cast<int> (loadParam (pLfo2Shape, 0.0f));

        // -- Voice Control --
        const int   voiceModeIndex       = static_cast<int> (loadParam (pVoiceMode, 2.0f));
        const float glideTime            = loadParam (pGlide, 0.0f);
        const int   initialShapeIndex    = static_cast<int> (loadParam (pInitShape, 0.0f));

        // -- Output --
        const float paramMasterLevel     = loadParam (pLevel, 0.8f);

        // -- Macros --
        const float macroCharacter       = loadParam (pMacroCharacter, 0.0f);
        const float macroMovement        = loadParam (pMacroMovement, 0.0f);
        const float macroCoupling        = loadParam (pMacroCoupling, 0.0f);
        const float macroSpace           = loadParam (pMacroSpace, 0.0f);

        //----------------------------------------------------------------------
        // Voice mode configuration
        //----------------------------------------------------------------------
        int maxPolyphony = kMaxVoices;
        bool monoMode = false;
        bool legatoMode = false;
        switch (voiceModeIndex)
        {
            case 0: maxPolyphony = 1; monoMode = true; break;                       // Mono
            case 1: maxPolyphony = 1; monoMode = true; legatoMode = true; break;    // Legato
            case 2: maxPolyphony = 4; break;                                         // Poly4
            case 3: maxPolyphony = 8; break;                                         // Poly8
            default: maxPolyphony = 4; break;
        }

        //----------------------------------------------------------------------
        // Glide coefficient: exponential approach to target frequency.
        // glideCoeff = 1 - exp(-1 / (glideTime * sr)), giving a smooth
        // portamento whose speed is independent of the interval size.
        //----------------------------------------------------------------------
        float glideCoefficient = 1.0f;
        constexpr float kMinGlideTimeSeconds = 0.001f;
        if (glideTime > kMinGlideTimeSeconds)
            glideCoefficient = 1.0f - std::exp (-1.0f / (glideTime * sampleRateFloat));

        //----------------------------------------------------------------------
        // Macro modulation offsets.
        // CHARACTER increases stiffness + nonlinearity (brighter, more inharmonic).
        // MOVEMENT increases scan width (wider scanning window = darker timbre).
        // COUPLING increases sustain/bowing force (more continuous excitation).
        // SPACE increases damping (faster energy loss = more reverberant decay).
        //----------------------------------------------------------------------
        constexpr float kMacroCharacterToStiffness    = 0.3f;
        constexpr float kMacroCharacterToNonlinearity = 0.2f;
        constexpr float kMacroMovementToScanWidth     = 0.3f;
        constexpr float kMacroCouplingToSustain       = 0.3f;
        constexpr float kMacroSpaceToDamping          = 0.2f;

        float effectiveStiffness   = clamp (paramStiffness
                                           + macroCharacter * kMacroCharacterToStiffness
                                           + couplingStiffnessModulation,
                                           0.0f, 1.0f);
        float effectiveDamping     = clamp (paramDamping
                                           + macroSpace * kMacroSpaceToDamping,
                                           0.0f, 1.0f);
        float effectiveNonlinearity = clamp (paramNonlinearity
                                            + macroCharacter * kMacroCharacterToNonlinearity,
                                            0.0f, 1.0f);
        float effectiveScanWidth   = clamp (paramScanWidth
                                           + macroMovement * kMacroMovementToScanWidth,
                                           0.0f, 1.0f);
        float effectiveSustain     = clamp (paramSustainForce
                                           + macroCoupling * kMacroCouplingToSustain
                                           + modWheelAmount * 0.4f,  // D006: mod wheel = bow speed
                                           0.0f, 1.0f);

        //----------------------------------------------------------------------
        // Map normalized parameters to physical simulation values
        //----------------------------------------------------------------------

        // Spring constant: quadratic mapping [0,1] -> [0, 0.95].
        // Quadratic curve gives finer control at low stiffness values,
        // where the timbral change is most audible.
        // D006: may be recomputed below after MIDI loop if aftertouch is active.
        float springConstant = effectiveStiffness * effectiveStiffness
                             * kMaxSpringConstant;

        // Damping coefficient: linear mapping [0,1] -> [0, 0.15].
        // 0.15 is the maximum before the chain becomes overdamped and
        // stops oscillating (all energy absorbed in ~1 cycle).
        constexpr float kMaxDampingCoefficient = 0.15f;
        float dampingCoefficient = effectiveDamping * kMaxDampingCoefficient;

        // Cubic spring coefficient: quadratic mapping [0,1] -> [0, 0.5].
        // The cubic term adds amplitude-dependent frequency shift --
        // louder displacements produce higher frequencies, like a real
        // stiff bar or struck bell.
        constexpr float kMaxCubicSpringCoefficient = 0.5f;
        float cubicSpringCoefficient = effectiveNonlinearity
                                     * effectiveNonlinearity
                                     * kMaxCubicSpringCoefficient;

        // Scanner width in masses: [0,1] -> [1, chainSize/4].
        // Width = 1: point scanner (reads a single mass, maximum brightness).
        // Width = 32: wide scanner (averages 32 masses, dark/muffled timbre).
        // This is analogous to pickup width on an electric guitar.
        float scanWidthInMasses = 1.0f + effectiveScanWidth
                                * (static_cast<float> (kChainSize) * 0.25f - 1.0f);

        //----------------------------------------------------------------------
        // Coupling accumulators: snapshot and reset
        //----------------------------------------------------------------------
        float localCouplingForce   = couplingForceModulation;
        float localCouplingImpulse = couplingImpulseTrigger;
        couplingForceModulation = 0.0f;
        couplingStiffnessModulation = 0.0f;
        couplingImpulseTrigger = 0.0f;

        //----------------------------------------------------------------------
        // MIDI event processing
        //----------------------------------------------------------------------
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), maxPolyphony,
                        monoMode, legatoMode, glideCoefficient,
                        paramAmpAttack, paramAmpDecay, paramAmpSustain, paramAmpRelease,
                        paramPhysAttack, paramPhysDecay, paramPhysSustain, paramPhysRelease,
                        paramLfo1Rate, paramLfo1Depth, paramLfo1Shape,
                        paramLfo2Rate, paramLfo2Depth, paramLfo2Shape,
                        initialShapeIndex, paramExcitePosition, paramExciteWidth);
            }
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
            // D006: channel pressure → aftertouch (applied to spring stiffness below)
            else if (msg.isChannelPressure())
                aftertouch.setChannelPressure (msg.getChannelPressureValue() / 127.0f);
            // D006: mod wheel (CC#1) → bow speed / excitation intensity
            // Wheel up = greater continuous excitation force — the bow presses deeper
            // into the string, increasing amplitude and sustain of the mass-spring chain.
            // Full wheel adds +0.4 to sustainForce (sensitivity 0.4).
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount = msg.getControllerValue() / 127.0f;
            else if (msg.isPitchWheel()) pitchBendNorm = PitchBendUtil::parsePitchWheel (msg.getPitchWheelValue());
        }

        // SilenceGate: skip all DSP if engine has been silent long enough
        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // D006: smooth aftertouch pressure and compute modulation value
        aftertouch.updateBlock (numSamples);
        const float atPressure = aftertouch.getSmoothedPressure (0);

        // D006: aftertouch adds up to +0.25 spring stiffness (sensitivity 0.25).
        // Pressing harder increases stiffness — brighter timbre, shorter decay.
        // springConstant must be recomputed here after atPressure is available.
        if (atPressure > 0.001f)
        {
            effectiveStiffness = clamp (effectiveStiffness + atPressure * 0.25f, 0.0f, 1.0f);
            springConstant = effectiveStiffness * effectiveStiffness * kMaxSpringConstant;
        }

        //----------------------------------------------------------------------
        // Apply coupling impulse to all active voices (if triggered by
        // another engine via RhythmToBlend coupling)
        //----------------------------------------------------------------------
        constexpr float kCouplingImpulseThreshold = 0.01f;
        constexpr float kCouplingImpulseScale = 0.1f;
        if (localCouplingImpulse > kCouplingImpulseThreshold)
        {
            for (auto& voice : voices)
            {
                if (voice.active && !voice.fadingOut)
                    voice.applyImpulse (paramExcitePosition, paramExciteWidth,
                                        localCouplingImpulse * kCouplingImpulseScale);
            }
        }

        // Set smoothing targets once per block
        smoothedStiffness.set    (effectiveStiffness);
        smoothedDamping.set      (effectiveDamping);
        smoothedNonlinearity.set (effectiveNonlinearity);
        smoothedScanWidth.set    (scanWidthInMasses);
        smoothedSustainForce.set (effectiveSustain);

        float peakEnvelopeLevel = 0.0f;

        //----------------------------------------------------------------------
        // Per-sample render loop
        //----------------------------------------------------------------------
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Advance smoothers one sample to avoid zipper noise.
            // Stiffness/damping/nonlinearity smoothers are advanced here so that
            // they track target values and are ready for per-sample use if the
            // physics hot-path is ever moved inside the sample loop.
            smoothedStiffness.process();
            smoothedDamping.process();
            smoothedNonlinearity.process();
            const float curSmoothedScanWidth    = smoothedScanWidth.process();
            const float curSmoothedSustainForce = smoothedSustainForce.process();

            float mixLeft = 0.0f, mixRight = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                //-- Voice-stealing crossfade (5ms linear ramp) ----------------
                if (voice.fadingOut)
                {
                    voice.crossfadeGain -= voiceCrossfadeRate;
                    if (voice.crossfadeGain <= 0.0f)
                    {
                        voice.crossfadeGain = 0.0f;
                        voice.active = false;
                        continue;
                    }
                }

                //-- Glide (portamento) ----------------------------------------
                voice.currentFrequency += (voice.targetFrequency - voice.currentFrequency)
                                        * voice.glideCoefficient;

                //-- Envelopes -------------------------------------------------
                float amplitudeLevel = voice.amplitudeEnvelope.process();
                float physicsLevel   = voice.physicsEnvelope.process();

                if (!voice.amplitudeEnvelope.isActive())
                {
                    voice.active = false;
                    continue;
                }

                //-- LFO modulation --------------------------------------------
                float lfo1Value = voice.lfo1.process() * paramLfo1Depth;
                float lfo2Value = voice.lfo2.process() * paramLfo2Depth;

                // LFO1 modulates scan width (timbral sweep)
                // LFO2 modulates excitation position (spatial sweep along chain)
                constexpr float kLfo1ScanWidthModDepth = 8.0f;
                constexpr float kLfo2ExcitePositionModDepth = 0.2f;
                float modulatedScanWidth = std::max (1.0f,
                    curSmoothedScanWidth + lfo1Value * kLfo1ScanWidthModDepth);
                float modulatedExcitePosition = clamp (
                    paramExcitePosition + lfo2Value * kLfo2ExcitePositionModDepth,
                    0.0f, 1.0f);

                //-- Physics simulation at control rate ------------------------
                voice.controlPhaseAccumulator += 1.0f;

                if (voice.controlPhaseAccumulator >= controlStepSamples)
                {
                    voice.controlPhaseAccumulator -= controlStepSamples;

                    // Rotate snapshots: current becomes previous
                    for (int i = 0; i < kChainSize; ++i)
                        voice.chainSnapshotA[i] = voice.chainSnapshotB[i];

                    //-- Continuous excitation ("bowing" force) -----------------
                    // This sustains the chain's vibration during held notes,
                    // like a bow drawn across a string. The force is shaped as
                    // a Gaussian centered at the excitation position.
                    constexpr float kBowingForceScale = 0.02f;
                    constexpr float kMinSustainThreshold = 0.001f;
                    if (curSmoothedSustainForce > kMinSustainThreshold)
                    {
                        float forceAmplitude = curSmoothedSustainForce * physicsLevel
                                             * kBowingForceScale;
                        float centerMass = modulatedExcitePosition
                                         * static_cast<float> (kChainSize - 1);

                        // Gaussian width: 15% of chain length at width=1
                        constexpr float kBowingWidthFraction = 0.15f;
                        float sigma = std::max (1.0f, paramExciteWidth
                                     * static_cast<float> (kChainSize)
                                     * kBowingWidthFraction);
                        float inverseTwoSigmaSquared = 1.0f / (2.0f * sigma * sigma);

                        for (int i = 1; i < kChainSize - 1; ++i)
                        {
                            float distance = static_cast<float> (i) - centerMass;
                            float gaussianForce = forceAmplitude
                                * fastExp (-distance * distance * inverseTwoSigmaSquared);
                            voice.chain[i] += gaussianForce;
                        }
                    }

                    //-- Coupling force modulation -----------------------------
                    // External engine audio (via AudioToFM coupling) is applied
                    // as a spatially-varying force across the chain, weighted by
                    // a sine function so that the force creates standing-wave
                    // patterns rather than uniform displacement.
                    constexpr float kCouplingForceScale = 0.01f;
                    constexpr float kMinCouplingForceThreshold = 0.0001f;
                    if (std::fabs (localCouplingForce) > kMinCouplingForceThreshold)
                    {
                        float forceScale = localCouplingForce * physicsLevel
                                         * kCouplingForceScale;
                        for (int i = 1; i < kChainSize - 1; ++i)
                        {
                            float normalizedPosition = static_cast<float> (i)
                                / static_cast<float> (kChainSize - 1);
                            voice.chain[i] += forceScale
                                * fastSin (normalizedPosition * kTwoPi);
                        }
                    }

                    //-- Verlet integration step -------------------------------
                    updateChain (voice.chain, voice.chainPrevious, kChainSize,
                                 springConstant, dampingCoefficient,
                                 cubicSpringCoefficient, paramBoundaryMode);

                    //-- Soft-clip chain displacement for stability -------------
                    // Without clipping, nonlinear springs can cause displacement
                    // to grow without bound, crashing the simulation. The tanh
                    // soft-clip keeps everything bounded while preserving the
                    // waveform shape for small displacements.
                    for (int i = 0; i < kChainSize; ++i)
                    {
                        float displacement = voice.chain[i];

                        // Flush denormals: subnormal floats in the chain state
                        // cause CPU spikes because the FPU falls back to slow
                        // microcode-assisted handling. At ~128 masses per voice
                        // times 8 voices = 1024 potential denormal sites, this
                        // protection is critical for real-time performance.
                        constexpr float kDenormalThreshold = 1e-15f;
                        if (std::fabs (displacement) < kDenormalThreshold)
                        {
                            voice.chain[i] = 0.0f;
                            voice.chainPrevious[i] = flushDenormal (voice.chainPrevious[i]);
                            continue;
                        }

                        // Soft clip: tanh(x * drive) * ceiling
                        voice.chain[i] = fastTanh (displacement * kChainSoftClipDrive)
                                       * kChainSoftClipCeiling;
                        voice.chainPrevious[i] = flushDenormal (voice.chainPrevious[i]);
                    }

                    // Update snapshot B with new chain state
                    for (int i = 0; i < kChainSize; ++i)
                        voice.chainSnapshotB[i] = voice.chain[i];
                }

                //-- Audio-rate interpolation between physics snapshots ---------
                float interpolationFraction = voice.controlPhaseAccumulator
                                            / controlStepSamples;
                interpolationFraction = clamp (interpolationFraction, 0.0f, 1.0f);

                //-- Scanner: sweep across chain at MIDI note frequency --------
                // The scanner reads the chain's shape at audio rate, producing
                // the output waveform. Scanner frequency = MIDI note frequency,
                // so the chain's displacement pattern becomes the waveform.
                float scannerFreq = voice.currentFrequency
                                    * PitchBendUtil::semitonesToFreqRatio (pitchBendNorm * 2.0f);
                float scannerIncrement = scannerFreq / sampleRateFloat;

                // Left channel: forward scan
                float scanPositionLeft = voice.scannerPhase;
                float outputLeft = scanChain (voice.chainSnapshotA,
                    voice.chainSnapshotB, interpolationFraction,
                    scanPositionLeft, modulatedScanWidth);

                // Right channel: backward scan (creates stereo width by
                // reading the chain in reverse -- the two channels see
                // slightly different waveform shapes due to chain asymmetry)
                float scanPositionRight = 1.0f - voice.scannerPhase;
                float outputRight = scanChain (voice.chainSnapshotA,
                    voice.chainSnapshotB, interpolationFraction,
                    scanPositionRight, modulatedScanWidth);

                // Advance scanner phase
                voice.scannerPhase += scannerIncrement;
                while (voice.scannerPhase >= 1.0f) voice.scannerPhase -= 1.0f;
                while (voice.scannerPhase < 0.0f)  voice.scannerPhase += 1.0f;

                //-- DC blocker ------------------------------------------------
                outputLeft  = voice.dcBlockerLeft.process (outputLeft);
                outputRight = voice.dcBlockerRight.process (outputRight);

                //-- Soft limiter ----------------------------------------------
                // tanh(x * 3) * 0.33: provides ~3x headroom before saturation,
                // then smoothly limits. The 0.33 scaling compensates for the
                // 3x drive so that unity input produces unity output.
                constexpr float kLimiterDrive   = 3.0f;
                constexpr float kLimiterCeiling = 0.33f;
                outputLeft  = fastTanh (outputLeft  * kLimiterDrive) * kLimiterCeiling;
                outputRight = fastTanh (outputRight * kLimiterDrive) * kLimiterCeiling;

                //-- Apply amplitude envelope, velocity, and crossfade ---------
                float voiceGain = amplitudeLevel * voice.velocity * voice.crossfadeGain;
                outputLeft  *= voiceGain;
                outputRight *= voiceGain;

                // Final denormal protection on voice outputs.
                // Even after all the above processing, the multiplication by
                // a near-zero envelope can produce subnormals.
                outputLeft  = flushDenormal (outputLeft);
                outputRight = flushDenormal (outputRight);

                mixLeft  += outputLeft;
                mixRight += outputRight;

                peakEnvelopeLevel = std::max (peakEnvelopeLevel, amplitudeLevel);
            }

            // Apply master level
            float finalLeft  = mixLeft  * paramMasterLevel;
            float finalRight = mixRight * paramMasterLevel;

            // Write to output buffer
            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample (0, sample, finalLeft);
                buffer.addSample (1, sample, finalRight);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample (0, sample, (finalLeft + finalRight) * 0.5f);
            }

            // Cache output for coupling reads by other engines
            if (sample < static_cast<int> (outputCacheLeft.size()))
            {
                outputCacheLeft[static_cast<size_t> (sample)]  = finalLeft;
                outputCacheRight[static_cast<size_t> (sample)] = finalRight;
            }
        }

        envelopeOutput = peakEnvelopeLevel;

        // Update active voice count for UI display
        int count = 0;
        for (const auto& voice : voices)
            if (voice.active) ++count;
        activeVoiceCount = count;

        // SilenceGate: analyze output level for next-block bypass decision
        silenceGate.analyzeBlock (buffer.getReadPointer (0),
                                  buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : nullptr,
                                  numSamples);
    }


    //==========================================================================
    // SynthEngine Interface -- Coupling
    //
    // OBSCURA's coupling personality:
    //   - As a source: provides its stiff, resonant, deep-water timbre
    //   - As a target: external forces excite the chain (AudioToFM),
    //     modify its stiffness (AmpToFilter), or trigger impulses
    //     (RhythmToBlend). The giant squid responding to the ocean's forces.
    //==========================================================================

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        auto index = static_cast<size_t> (sampleIndex);
        if (channel == 0 && index < outputCacheLeft.size())  return outputCacheLeft[index];
        if (channel == 1 && index < outputCacheRight.size()) return outputCacheRight[index];
        if (channel == 2) return envelopeOutput;  // envelope follower for amplitude coupling
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
            case CouplingType::AudioToFM:
                // External audio applied as distributed force on chain masses.
                // Scale factor 0.5: moderate sensitivity to prevent chain blowup.
                couplingForceModulation += amount * 0.5f;
                break;

            case CouplingType::AmpToFilter:
                // External amplitude modulates spring stiffness.
                // Scale factor 0.3: subtle timbral shift from coupling.
                couplingStiffnessModulation += amount * 0.3f;
                break;

            case CouplingType::RhythmToBlend:
                // External rhythm trigger fires an impulse on the chain.
                // Raw amount passed through -- impulse scaling happens at application.
                couplingImpulseTrigger += amount;
                break;

            default:
                break;
        }
    }


    //==========================================================================
    // SynthEngine Interface -- Parameter Layout
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
        //-- Core Physics Parameters -------------------------------------------

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_stiffness", 1 }, "Obscura Stiffness",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_damping", 1 }, "Obscura Damping",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_nonlinear", 1 }, "Obscura Nonlinear",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_excitePos", 1 }, "Obscura Excite Position",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_exciteWidth", 1 }, "Obscura Excite Width",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_scanWidth", 1 }, "Obscura Scan Width",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "obscura_boundary", 1 }, "Obscura Boundary Mode",
            juce::StringArray { "Fixed", "Free", "Periodic" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_sustain", 1 }, "Obscura Sustain Force",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        //-- Level -------------------------------------------------------------

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_level", 1 }, "Obscura Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        //-- Amplitude Envelope ------------------------------------------------

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_ampAttack", 1 }, "Obscura Amp Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_ampDecay", 1 }, "Obscura Amp Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.1f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_ampSustain", 1 }, "Obscura Amp Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_ampRelease", 1 }, "Obscura Amp Release",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        //-- Physics Envelope --------------------------------------------------

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_physEnvAttack", 1 }, "Obscura Phys Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_physEnvDecay", 1 }, "Obscura Phys Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.2f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_physEnvSustain", 1 }, "Obscura Phys Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_physEnvRelease", 1 }, "Obscura Phys Release",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        //-- LFO 1 -------------------------------------------------------------

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_lfo1Rate", 1 }, "Obscura LFO1 Rate",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_lfo1Depth", 1 }, "Obscura LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "obscura_lfo1Shape", 1 }, "Obscura LFO1 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));

        //-- LFO 2 -------------------------------------------------------------

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_lfo2Rate", 1 }, "Obscura LFO2 Rate",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_lfo2Depth", 1 }, "Obscura LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "obscura_lfo2Shape", 1 }, "Obscura LFO2 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));

        //-- Voice Parameters --------------------------------------------------

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "obscura_voiceMode", 1 }, "Obscura Voice Mode",
            juce::StringArray { "Mono", "Legato", "Poly4", "Poly8" }, 2));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_glide", 1 }, "Obscura Glide",
            juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f, 0.5f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "obscura_initShape", 1 }, "Obscura Init Shape",
            juce::StringArray { "Sine", "Saw", "Random", "Flat" }, 0));

        //-- Macros (XOlokun standard M1-M4) ----------------------------------

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_macroCharacter", 1 }, "Obscura Macro CHARACTER",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_macroMovement", 1 }, "Obscura Macro MOVEMENT",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_macroCoupling", 1 }, "Obscura Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_macroSpace", 1 }, "Obscura Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        //-- Core Physics --
        pStiffness       = apvts.getRawParameterValue ("obscura_stiffness");
        pDamping         = apvts.getRawParameterValue ("obscura_damping");
        pNonlinear       = apvts.getRawParameterValue ("obscura_nonlinear");
        pExcitePos       = apvts.getRawParameterValue ("obscura_excitePos");
        pExciteWidth     = apvts.getRawParameterValue ("obscura_exciteWidth");
        pScanWidth       = apvts.getRawParameterValue ("obscura_scanWidth");
        pBoundary        = apvts.getRawParameterValue ("obscura_boundary");
        pSustain         = apvts.getRawParameterValue ("obscura_sustain");
        pLevel           = apvts.getRawParameterValue ("obscura_level");

        //-- Amplitude Envelope --
        pAmpAttack       = apvts.getRawParameterValue ("obscura_ampAttack");
        pAmpDecay        = apvts.getRawParameterValue ("obscura_ampDecay");
        pAmpSustain      = apvts.getRawParameterValue ("obscura_ampSustain");
        pAmpRelease      = apvts.getRawParameterValue ("obscura_ampRelease");

        //-- Physics Envelope --
        pPhysAttack      = apvts.getRawParameterValue ("obscura_physEnvAttack");
        pPhysDecay       = apvts.getRawParameterValue ("obscura_physEnvDecay");
        pPhysSustain     = apvts.getRawParameterValue ("obscura_physEnvSustain");
        pPhysRelease     = apvts.getRawParameterValue ("obscura_physEnvRelease");

        //-- LFO 1 --
        pLfo1Rate        = apvts.getRawParameterValue ("obscura_lfo1Rate");
        pLfo1Depth       = apvts.getRawParameterValue ("obscura_lfo1Depth");
        pLfo1Shape       = apvts.getRawParameterValue ("obscura_lfo1Shape");

        //-- LFO 2 --
        pLfo2Rate        = apvts.getRawParameterValue ("obscura_lfo2Rate");
        pLfo2Depth       = apvts.getRawParameterValue ("obscura_lfo2Depth");
        pLfo2Shape       = apvts.getRawParameterValue ("obscura_lfo2Shape");

        //-- Voice --
        pVoiceMode       = apvts.getRawParameterValue ("obscura_voiceMode");
        pGlide           = apvts.getRawParameterValue ("obscura_glide");
        pInitShape       = apvts.getRawParameterValue ("obscura_initShape");

        //-- Macros --
        pMacroCharacter  = apvts.getRawParameterValue ("obscura_macroCharacter");
        pMacroMovement   = apvts.getRawParameterValue ("obscura_macroMovement");
        pMacroCoupling   = apvts.getRawParameterValue ("obscura_macroCoupling");
        pMacroSpace      = apvts.getRawParameterValue ("obscura_macroSpace");
    }


    //==========================================================================
    // SynthEngine Interface -- Engine Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Obscura"; }

    // Daguerreotype Silver #8A9BA8 -- the color of tarnished silver plate,
    // referencing both the camera obscura's mirror and the giant squid's
    // bioluminescent silver skin in the deep ocean.
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF8A9BA8); }

    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoiceCount; }


private:

    SilenceGate silenceGate;

    //==========================================================================
    // Helper: safe atomic parameter load with fallback
    //==========================================================================

    static float loadParam (std::atomic<float>* param, float fallback) noexcept
    {
        return (param != nullptr) ? param->load() : fallback;
    }


    //==========================================================================
    // Mass-Spring Chain: Verlet Integration
    //
    // The Verlet integrator (also called Stormer-Verlet or leapfrog) is the
    // standard method for molecular dynamics and physical simulation because
    // it is symplectic -- it conserves energy over long time periods, unlike
    // explicit Euler which gains energy and eventually explodes. The update
    // rule is:
    //
    //   x_new = 2*x - x_old + F * dt^2
    //
    // where F is the total force (spring + nonlinear + damping) and dt^2 is
    // normalized to 1.0 (absorbed into the spring constant).
    //
    // Boundary conditions:
    //   Fixed:    endpoints pinned to zero (like a guitar string at the nut/bridge)
    //   Free:     endpoints mirror their neighbor (like an open pipe end)
    //   Periodic: endpoints wrap around (like a ring of masses)
    //==========================================================================

    static void updateChain (float* currentPosition, float* previousPosition,
                             int chainLength,
                             float springConstant, float dampingCoefficient,
                             float cubicSpringConstant,
                             int boundaryMode) noexcept
    {
        // Apply boundary conditions before integration
        switch (boundaryMode)
        {
            case 0: // Fixed: endpoints pinned to zero (vibrating string)
                currentPosition[0] = 0.0f;
                currentPosition[chainLength - 1] = 0.0f;
                previousPosition[0] = 0.0f;
                previousPosition[chainLength - 1] = 0.0f;
                break;

            case 1: // Free: endpoints mirror neighbors (open boundary)
                currentPosition[0] = currentPosition[1];
                currentPosition[chainLength - 1] = currentPosition[chainLength - 2];
                previousPosition[0] = previousPosition[1];
                previousPosition[chainLength - 1] = previousPosition[chainLength - 2];
                break;

            case 2: // Periodic: endpoints wrap around (circular chain)
                currentPosition[0] = currentPosition[chainLength - 2];
                currentPosition[chainLength - 1] = currentPosition[1];
                previousPosition[0] = previousPosition[chainLength - 2];
                previousPosition[chainLength - 1] = previousPosition[1];
                break;

            default: // Fallback to fixed endpoints
                currentPosition[0] = 0.0f;
                currentPosition[chainLength - 1] = 0.0f;
                previousPosition[0] = 0.0f;
                previousPosition[chainLength - 1] = 0.0f;
                break;
        }

        // Verlet integration for interior masses (skip endpoints)
        for (int i = 1; i < chainLength - 1; ++i)
        {
            float displacementLeft  = currentPosition[i] - currentPosition[i - 1];
            float displacementRight = currentPosition[i + 1] - currentPosition[i];

            // Linear spring force: F = k * (x[i+1] - 2*x[i] + x[i-1])
            // This is the discrete Laplacian -- the same operator used in
            // finite-difference wave equation solvers.
            float force = springConstant
                        * (currentPosition[i + 1] - 2.0f * currentPosition[i]
                           + currentPosition[i - 1]);

            // Cubic nonlinear spring: F += k3 * (dx_right^3 - dx_left^3)
            // This adds amplitude-dependent stiffness -- large displacements
            // create stronger restoring forces, producing inharmonic partials
            // and pitch brightening at high amplitudes (like a stiff piano string).
            force += cubicSpringConstant
                   * (displacementRight * displacementRight * displacementRight
                    - displacementLeft  * displacementLeft  * displacementLeft);

            // Velocity estimate from Verlet: v ~ x[n] - x[n-1]
            float velocity = currentPosition[i] - previousPosition[i];

            // Damping force: F -= d * v (viscous damping, proportional to velocity)
            force -= dampingCoefficient * velocity;

            // Verlet step: x_new = 2*x - x_old + F * dt^2
            // dt^2 is normalized to 1.0; k already encodes effective k*dt^2
            float newPosition = 2.0f * currentPosition[i]
                              - previousPosition[i] + force;

            previousPosition[i] = currentPosition[i];
            currentPosition[i] = newPosition;
        }
    }


    //==========================================================================
    // Scanner: Read Chain Displacement
    //
    // The scanner sweeps across the chain at MIDI note frequency, reading
    // the chain's displacement profile as a waveform. This is the core
    // of scanned synthesis -- the chain shape IS the waveform.
    //
    // For narrow scan widths (< 2 masses), a single cubic Hermite
    // interpolation is used (maximum brightness -- like a magnetic pickup
    // close to a guitar string). For wider widths, a Hann-windowed average
    // over multiple chain positions is computed (darker timbre -- like a
    // wide ribbon microphone averaging across a sound source).
    //
    // scanPosition: [0, 1) -- normalized position along the chain.
    // scanWidth: width of the averaging window in chain masses.
    //==========================================================================

    static float scanChain (const float* snapshotA, const float* snapshotB,
                            float interpolationFraction, float scanPosition,
                            float scanWidth) noexcept
    {
        // Map normalized position [0,1) to chain index [0, N-1]
        float chainPosition = scanPosition * static_cast<float> (kChainSize - 1);

        // Narrow width: single-point cubic interpolation (brightest timbre)
        if (scanWidth < 2.0f)
        {
            return cubicHermiteInterpolateChain (snapshotA, snapshotB,
                                                 interpolationFraction,
                                                 chainPosition);
        }

        // Wide width: Hann-windowed average (darker timbre)
        float halfWidth = scanWidth * 0.5f;
        float windowStart = chainPosition - halfWidth;
        float windowEnd   = chainPosition + halfWidth;

        // Number of sample points in the averaging window
        int windowSamples = static_cast<int> (scanWidth) + 1;
        if (windowSamples < 2)         windowSamples = 2;
        if (windowSamples > kChainSize) windowSamples = kChainSize;

        float sampleStep = (windowEnd - windowStart)
                         / static_cast<float> (windowSamples - 1);
        float weightedSum = 0.0f;
        float totalWeight = 0.0f;

        for (int s = 0; s < windowSamples; ++s)
        {
            float samplePosition = windowStart + static_cast<float> (s) * sampleStep;

            // Wrap position to valid chain range [0, N-1]
            float chainEnd = static_cast<float> (kChainSize - 1);
            float wrappedPosition = samplePosition;
            while (wrappedPosition < 0.0f)     wrappedPosition += chainEnd;
            while (wrappedPosition >= chainEnd) wrappedPosition -= chainEnd;

            // Hann window weight: w = 0.5 - 0.5*cos(2*pi*t)
            // This gives smooth rolloff at the edges of the averaging window,
            // preventing spectral leakage artifacts in the output.
            float normalizedWindowPos = static_cast<float> (s)
                                      / static_cast<float> (windowSamples - 1);
            float hannWeight = 0.5f - 0.5f * std::cos (kTwoPi * normalizedWindowPos);

            float sampleValue = cubicHermiteInterpolateChain (
                snapshotA, snapshotB, interpolationFraction, wrappedPosition);
            weightedSum += sampleValue * hannWeight;
            totalWeight += hannWeight;
        }

        constexpr float kMinWeightThreshold = 0.0001f;
        return (totalWeight > kMinWeightThreshold)
                   ? (weightedSum / totalWeight) : 0.0f;
    }


    //==========================================================================
    // Cubic Hermite Interpolation for Chain Readout
    //
    // Reads the chain displacement at a fractional position using 4-point
    // cubic Hermite (Catmull-Rom) interpolation, blended between two
    // control-rate snapshots. This provides smooth, artifact-free audio
    // output despite the chain only being updated at ~4 kHz.
    //
    // The Hermite basis functions ensure C1 continuity (smooth first
    // derivative), which eliminates the high-frequency aliasing that
    // linear interpolation would produce.
    //==========================================================================

    static float cubicHermiteInterpolateChain (const float* snapshotA,
                                               const float* snapshotB,
                                               float interpolationFraction,
                                               float position) noexcept
    {
        int chainLength = kChainSize;
        int index1 = static_cast<int> (position);
        float fractionalPart = position - static_cast<float> (index1);

        // Clamp base index to valid range
        if (index1 < 0)            index1 = 0;
        if (index1 >= chainLength) index1 = chainLength - 1;

        // Four neighboring indices for cubic interpolation
        int index0 = (index1 > 0)              ? index1 - 1 : 0;
        int index2 = (index1 < chainLength - 1) ? index1 + 1 : chainLength - 1;
        int index3 = (index2 < chainLength - 1) ? index2 + 1 : chainLength - 1;

        // Interpolate between snapshots A and B at each sample point
        float y0 = snapshotA[index0] + interpolationFraction * (snapshotB[index0] - snapshotA[index0]);
        float y1 = snapshotA[index1] + interpolationFraction * (snapshotB[index1] - snapshotA[index1]);
        float y2 = snapshotA[index2] + interpolationFraction * (snapshotB[index2] - snapshotA[index2]);
        float y3 = snapshotA[index3] + interpolationFraction * (snapshotB[index3] - snapshotA[index3]);

        // Cubic Hermite (Catmull-Rom) coefficients
        float c0 = y1;
        float c1 = 0.5f * (y2 - y0);
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        // Evaluate polynomial: ((c3*t + c2)*t + c1)*t + c0 (Horner's method)
        return ((c3 * fractionalPart + c2) * fractionalPart + c1)
               * fractionalPart + c0;
    }


    //==========================================================================
    // MIDI Note Handling
    //==========================================================================

    void noteOn (int noteNumber, float velocity, int maxPolyphony,
                 bool monoMode, bool legatoMode, float glideCoefficient,
                 float ampAttack, float ampDecay, float ampSustain, float ampRelease,
                 float physAttack, float physDecay, float physSustain, float physRelease,
                 float lfo1Rate, float lfo1Depth, int lfo1Shape,
                 float lfo2Rate, float lfo2Depth, int lfo2Shape,
                 int initialShapeIndex, float excitePosition, float exciteWidth)
    {
        float frequency = midiNoteToFrequencyHz (static_cast<float> (noteNumber));

        //-- Mono / Legato mode ------------------------------------------------
        if (monoMode)
        {
            auto& voice = voices[0];
            bool wasActive = voice.active;

            voice.targetFrequency = frequency;

            if (legatoMode && wasActive)
            {
                // Legato: glide to new pitch without retriggering envelopes
                voice.glideCoefficient = glideCoefficient;
                voice.noteNumber = noteNumber;
                voice.velocity = velocity;

                // Gentle re-excitation on legato retrigger (amplitude 0.05)
                // to add subtle articulation without full attack transient
                constexpr float kLegatoReexciteAmplitude = 0.05f;
                voice.applyImpulse (excitePosition, exciteWidth,
                                    kLegatoReexciteAmplitude);
            }
            else
            {
                // Full note-on: initialize voice from scratch
                voice.active = true;
                voice.noteNumber = noteNumber;
                voice.velocity = velocity;
                voice.startTime = voiceCounter++;
                voice.currentFrequency = frequency;
                voice.glideCoefficient = glideCoefficient;
                voice.scannerPhase = 0.0f;
                voice.controlPhaseAccumulator = 0.0f;
                voice.fadingOut = false;
                voice.crossfadeGain = 1.0f;

                voice.amplitudeEnvelope.setParams (ampAttack, ampDecay,
                    ampSustain, ampRelease, sampleRateFloat);
                voice.amplitudeEnvelope.noteOn();
                voice.physicsEnvelope.setParams (physAttack, physDecay,
                    physSustain, physRelease, sampleRateFloat);
                voice.physicsEnvelope.noteOn();

                voice.lfo1.setRate (lfo1Rate, sampleRateFloat);
                voice.lfo1.setShape (lfo1Shape);
                voice.lfo2.setRate (lfo2Rate, sampleRateFloat);
                voice.lfo2.setShape (lfo2Shape);

                voice.dcBlockerLeft.setCoefficient (sampleRateFloat);
                voice.dcBlockerRight.setCoefficient (sampleRateFloat);
                voice.dcBlockerLeft.reset();
                voice.dcBlockerRight.reset();

                // Seed RNG with note number for deterministic chain initialization.
                // 7919 is an arbitrary prime to spread seeds across the state space.
                voice.randomGeneratorState = static_cast<uint32_t> (noteNumber * 7919 + 42);
                voice.initChainShape (initialShapeIndex, excitePosition, exciteWidth);
            }
            return;
        }

        //-- Polyphonic mode ---------------------------------------------------
        int voiceIndex = VoiceAllocator::findFreeVoice (voices, std::min (maxPolyphony, kMaxVoices));
        auto& voice = voices[static_cast<size_t> (voiceIndex)];

        // If stealing an active voice, initiate crossfade to prevent click
        if (voice.active)
        {
            voice.fadingOut = true;
            // Cap crossfade gain at 0.5 to ensure the stolen voice fades
            // quickly enough that the new note's attack isn't masked
            voice.crossfadeGain = std::min (voice.crossfadeGain, 0.5f);
        }

        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.startTime = voiceCounter++;
        voice.currentFrequency = frequency;
        voice.targetFrequency = frequency;
        voice.glideCoefficient = 1.0f;  // no glide in poly mode
        voice.scannerPhase = 0.0f;
        voice.controlPhaseAccumulator = 0.0f;
        voice.fadingOut = false;
        voice.crossfadeGain = 1.0f;

        voice.amplitudeEnvelope.setParams (ampAttack, ampDecay,
            ampSustain, ampRelease, sampleRateFloat);
        voice.amplitudeEnvelope.noteOn();
        voice.physicsEnvelope.setParams (physAttack, physDecay,
            physSustain, physRelease, sampleRateFloat);
        voice.physicsEnvelope.noteOn();

        voice.lfo1.setRate (lfo1Rate, sampleRateFloat);
        voice.lfo1.setShape (lfo1Shape);
        voice.lfo1.reset();
        voice.lfo2.setRate (lfo2Rate, sampleRateFloat);
        voice.lfo2.setShape (lfo2Shape);
        voice.lfo2.reset();

        voice.dcBlockerLeft.setCoefficient (sampleRateFloat);
        voice.dcBlockerRight.setCoefficient (sampleRateFloat);
        voice.dcBlockerLeft.reset();
        voice.dcBlockerRight.reset();

        // Seed RNG: combine note number and voice counter for unique
        // random chain initialization per note event
        voice.randomGeneratorState = static_cast<uint32_t> (noteNumber * 7919 + voiceCounter);
        voice.initChainShape (initialShapeIndex, excitePosition, exciteWidth);
    }

    void noteOff (int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber && !voice.fadingOut)
            {
                voice.amplitudeEnvelope.noteOff();
                voice.physicsEnvelope.noteOff();
            }
        }
    }

    //--------------------------------------------------------------------------
    // Standard equal-temperament MIDI-to-frequency conversion.
    // A4 = 440 Hz, 12 semitones per octave.
    //--------------------------------------------------------------------------
    static float midiNoteToFrequencyHz (float midiNote) noexcept
    {
        return 440.0f * std::pow (2.0f, (midiNote - 69.0f) / 12.0f);
    }


    //==========================================================================
    // Member Data
    //==========================================================================

    //-- Sample rate -----------------------------------------------------------
    double sampleRateDouble = 44100.0;
    float  sampleRateFloat  = 44100.0f;

    //-- Timing coefficients ---------------------------------------------------
    float voiceCrossfadeRate = 0.01f;
    float controlStepSamples = 11.025f;  // ~44100 / 4000 (audio samples per physics step)
    float normalizedDtSquared = 1.0f;

    //-- Voice pool ------------------------------------------------------------
    std::array<ObscuraVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;     // monotonic counter for LRU voice stealing
    int activeVoiceCount = 0;

    //-- Smoothed control parameters (ParameterSmoother — 5ms time constant) ---
    ParameterSmoother smoothedStiffness;
    ParameterSmoother smoothedDamping;
    ParameterSmoother smoothedNonlinearity;
    ParameterSmoother smoothedScanWidth;
    ParameterSmoother smoothedSustainForce;

    //-- Coupling accumulators -------------------------------------------------
    float envelopeOutput              = 0.0f;
    float couplingForceModulation     = 0.0f;
    float couplingStiffnessModulation = 0.0f;
    float couplingImpulseTrigger      = 0.0f;

    //-- Output cache for coupling reads ---------------------------------------
    std::vector<float> outputCacheLeft;
    std::vector<float> outputCacheRight;

    // ---- D006 Aftertouch — pressure increases spring stiffness (physics coupling) ----
    PolyAftertouch aftertouch;

    // D006: mod wheel (CC#1) — bow speed / excitation intensity (+0.4 sustainForce at full wheel)
    float modWheelAmount = 0.0f;
    float pitchBendNorm  = 0.0f;  // MIDI pitch wheel [-1, +1]; ±2 semitone range

    //-- Cached APVTS parameter pointers (ParamSnapshot pattern) ---------------
    // Core physics
    std::atomic<float>* pStiffness   = nullptr;
    std::atomic<float>* pDamping     = nullptr;
    std::atomic<float>* pNonlinear   = nullptr;
    std::atomic<float>* pExcitePos   = nullptr;
    std::atomic<float>* pExciteWidth = nullptr;
    std::atomic<float>* pScanWidth   = nullptr;
    std::atomic<float>* pBoundary    = nullptr;
    std::atomic<float>* pSustain     = nullptr;
    std::atomic<float>* pLevel       = nullptr;

    // Amplitude envelope
    std::atomic<float>* pAmpAttack   = nullptr;
    std::atomic<float>* pAmpDecay    = nullptr;
    std::atomic<float>* pAmpSustain  = nullptr;
    std::atomic<float>* pAmpRelease  = nullptr;

    // Physics envelope
    std::atomic<float>* pPhysAttack  = nullptr;
    std::atomic<float>* pPhysDecay   = nullptr;
    std::atomic<float>* pPhysSustain = nullptr;
    std::atomic<float>* pPhysRelease = nullptr;

    // LFO 1
    std::atomic<float>* pLfo1Rate    = nullptr;
    std::atomic<float>* pLfo1Depth   = nullptr;
    std::atomic<float>* pLfo1Shape   = nullptr;

    // LFO 2
    std::atomic<float>* pLfo2Rate    = nullptr;
    std::atomic<float>* pLfo2Depth   = nullptr;
    std::atomic<float>* pLfo2Shape   = nullptr;

    // Voice control
    std::atomic<float>* pVoiceMode   = nullptr;
    std::atomic<float>* pGlide       = nullptr;
    std::atomic<float>* pInitShape   = nullptr;

    // Macros
    std::atomic<float>* pMacroCharacter = nullptr;
    std::atomic<float>* pMacroMovement  = nullptr;
    std::atomic<float>* pMacroCoupling  = nullptr;
    std::atomic<float>* pMacroSpace     = nullptr;
};

} // namespace xolokun
