// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OstinatoEngine.h — XOstinato | "The Fire Circle"
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOstinato is the fire at the center of the drum circle — the
//      communal gathering where eight musicians sit in a ring, each
//      with an instrument from a different tradition. The fire draws
//      them together: West African djembe alongside Japanese taiko,
//      Indian tabla beside Brazilian surdo. No hierarchy, no borders.
//      Pure rhythm, pure communion.
//
//      In the XO_OX aquatic mythology, Ostinato is a bioluminescent
//      campfire on the shore — where the ocean creatures come to land,
//      gather around the warmth, and speak in rhythm. The most complex
//      engine in the fleet: 8 seats x 12 instruments, each physically
//      modeled with modal membranes and waveguide body resonance.
//
//  ENGINE CONCEPT:
//      A communal drum circle with 8 seats arranged in a ring. Each seat
//      can hold any of 12 world instruments, each with 3-4 articulations.
//      The DSP chain per voice:
//
//          Exciter (noise burst + pitch spike, per articulation)
//            -> Modal Membrane (6-8 bandpass resonators, tuned per instrument)
//            -> Waveguide Body (cylindrical/conical/box/open, per shape)
//            -> Radiation Filter (open/mute character shaping)
//            -> Per-Seat SVF Filter -> Amp Envelope -> Pan
//
//      A 16-step pattern sequencer drives autonomous rhythms per seat,
//      with live MIDI override. The GATHER macro tightens the ensemble
//      from loose/organic to locked/quantized.
//
//  THE 12 INSTRUMENTS:
//      0. Djembe       — West Africa (tone, slap, bass, mute)
//      1. Dundun       — West Africa (open, mute, bell-on, bell-off)
//      2. Conga        — Cuba/Caribbean (open, slap, mute, fingertip)
//      3. Bongos       — Cuba (open, slap, mute)
//      4. Cajón        — Peru (bass, slap, ghost, fingertip)
//      5. Taiko        — Japan (center, edge, rim, flam)
//      6. Tabla        — India (na, tin, tun, ge)
//      7. Doumbek      — Middle East (doum, tek, ka, snap)
//      8. Frame Drum   — Mediterranean (open, edge, finger-roll, slap)
//      9. Surdo        — Brazil (open, mute, rim)
//     10. Tongue Drum  — Modern (strike, mallet, harmonic)
//     11. Beatbox      — Global (kick, snare, hi-hat, fx)
//
//  4 MACROS:
//      GATHER  — Ensemble tightness (loose/organic <-> tight/quantized)
//      FIRE    — Intensity (drives exciter energy, resonance, compression)
//      CIRCLE  — Inter-seat sympathetic resonance and ghost triggers
//      SPACE   — Environment (dry room <-> cathedral)
//
//  ACCENT COLOR: Firelight Orange #E8701A
//  PARAMETER PREFIX: osti_
//
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/StandardLFO.h"
#include <array>
#include <cmath>
#include <cstring>
#include <vector>

namespace xoceanus
{

//==============================================================================
//
//  I. PRIMITIVES — Low-level building blocks
//
//==============================================================================

//==============================================================================
// OstiNoiseGen — xorshift32 PRNG for excitation noise.
//==============================================================================
class OstiNoiseGen
{
public:
    void seed(uint32_t s) noexcept { state = s ? s : 1; }

    float process() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float>(static_cast<int32_t>(state)) / 2147483648.0f;
    }

private:
    uint32_t state = 1;
};

//==============================================================================
// OstiEnvelope — AD percussion envelope with coefficient caching.
//
// Percussion envelopes for world drums. Attack-Decay is the primary shape.
// The decay coefficient is cached so repeated triggers at the same decay
// skip the expensive exp() recomputation.
//==============================================================================
class OstiEnvelope
{
public:
    enum class Stage
    {
        Idle,
        Attack,
        Decay
    };

    void prepare(double sampleRate) noexcept { sr = static_cast<float>(sampleRate); }

    void trigger(float attackSec, float decaySec) noexcept
    {
        // FIX(stability): guard against sr=0 sentinel (engine called before prepare()).
        if (sr <= 0.0f) return;

        stage = Stage::Attack;
        level = 0.0f;

        float aSec = std::max(attackSec, 0.0001f);
        attackRate = 1.0f / (sr * aSec);

        float dSec = std::max(decaySec, 0.001f);
        if (dSec != lastDecay)
        {
            lastDecay = dSec;
            decayCoeff = 1.0f - std::exp(-4.6f / (sr * dSec));
        }
    }

    float process() noexcept
    {
        switch (stage)
        {
        case Stage::Idle:
            return 0.0f;
        case Stage::Attack:
            level += attackRate;
            if (level >= 1.0f)
            {
                level = 1.0f;
                stage = Stage::Decay;
            }
            return level;
        case Stage::Decay:
            level -= level * decayCoeff;
            level = flushDenormal(level);
            if (level < 1e-6f)
            {
                level = 0.0f;
                stage = Stage::Idle;
            }
            return level;
        }
        return 0.0f;
    }

    bool isActive() const noexcept { return stage != Stage::Idle; }
    float getLevel() const noexcept { return level; }
    void reset() noexcept
    {
        stage = Stage::Idle;
        level = 0.0f;
        lastDecay = -1.0f;
    }

private:
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)
    Stage stage = Stage::Idle;
    float level = 0.0f;
    float attackRate = 0.01f;
    float decayCoeff = 0.001f;
    float lastDecay = -1.0f;
};

// OstiBreathingLFO replaced by shared BreathingLFO (Source/DSP/StandardLFO.h)

//==============================================================================
//
//  II. INSTRUMENT DEFINITIONS — Modal frequency ratios and characteristics
//
//  Each instrument is defined by its modal membrane frequency ratios,
//  body model type, and per-articulation excitation parameters.
//  The modal ratios come from circular membrane Bessel function zeros
//  (Kinsler & Frey, "Fundamentals of Acoustics"), with per-instrument
//  tuning offsets derived from acoustic measurements.
//
//==============================================================================

//==============================================================================
// InstrumentType — the 12 world instruments of the drum circle.
//==============================================================================
enum class OstiInstrument : int
{
    Djembe = 0,
    Dundun,
    Conga,
    Bongos,
    Cajon,
    Taiko,
    Tabla,
    Doumbek,
    FrameDrum,
    Surdo,
    TongueDrum,
    Beatbox,
    Count = 12
};

//==============================================================================
// BodyModelType — waveguide body resonance shapes.
//==============================================================================
enum class BodyModelType : int
{
    Cylindrical = 0, // djembe, conga, surdo — delay + allpass + reflection
    Conical = 1,     // doumbek, bongos — shorter delay, brighter reflection
    Box = 2,         // cajon — multi-tap delay, wide reflection
    Open = 3         // frame drum, tongue drum — minimal body, direct radiation
};

//==============================================================================
// OstiInstrumentData — static data per instrument: mode ratios, body type,
// default frequency, number of articulations.
//==============================================================================
struct OstiInstrumentData
{
    static constexpr int kMaxModes = 8;
    static constexpr int kMaxArticulations = 4;

    float modeRatios[kMaxModes];
    float modeDecays[kMaxModes]; // relative decay per mode (1.0 = longest)
    int numModes;
    BodyModelType bodyType;
    float defaultFreqHz; // fundamental frequency
    int numArticulations;
    float bodyDelayMs;    // waveguide body delay in ms
    float bodyReflection; // reflection coefficient (0-1)

    // Per-articulation excitation character
    struct ArticulationData
    {
        float noiseDurationMs;      // exciter noise burst length
        float pitchSpikeRatio;      // pitch spike multiplier (e.g. 4x)
        float pitchSpikeDurationMs; // pitch spike length
        float brightnessOffset;     // radiation filter brightness adjustment
        float exciterMix;           // noise vs pitched balance (0=noise, 1=pitched)
    };
    ArticulationData articulations[kMaxArticulations];
};

//==============================================================================
// kInstrumentTable — The 12 instruments, fully characterized.
//
// Modal ratios derived from:
//   - Circular membrane Bessel zeros: Kinsler & Frey, Table 9.3
//   - Tabla harmonic modes: Raman, C.V. "Indian Musical Drums" (1934)
//   - Taiko thick membrane: Fletcher & Rossing, "Physics of Musical Instruments"
//   - Djembe measurements: Brindle et al., JASA 2005
//==============================================================================
static constexpr OstiInstrumentData kInstrumentTable[static_cast<int>(OstiInstrument::Count)] = {
    // 0. DJEMBE — goblet drum, circular membrane, cylindrical body
    // Modes: classic Bessel zeros for a clamped circular membrane
    {{1.00f, 1.59f, 2.14f, 2.65f, 3.16f, 3.65f, 4.06f, 4.15f}, // mode ratios
     {1.00f, 0.85f, 0.70f, 0.55f, 0.45f, 0.35f, 0.28f, 0.22f}, // mode decays
     8,
     BodyModelType::Cylindrical,
     150.0f,
     4,
     8.0f,
     0.65f,
     {
         {2.0f, 2.0f, 1.5f, 0.0f, 0.3f},  // tone: medium noise, mild spike
         {0.8f, 6.0f, 0.5f, 0.4f, 0.15f}, // slap: short sharp noise, big spike, bright
         {3.0f, 1.5f, 2.0f, -0.3f, 0.5f}, // bass: long noise, gentle spike, dark
         {0.5f, 4.0f, 0.3f, -0.6f, 0.1f}, // mute: very short, dampened
     }},

    // 1. DUNDUN — double-headed cylindrical drum + bell
    // Modes: slightly detuned from ideal membrane (thick hide)
    {{1.00f, 1.52f, 2.05f, 2.50f, 2.98f, 3.40f, 3.82f, 4.10f},
     {1.00f, 0.90f, 0.75f, 0.60f, 0.50f, 0.38f, 0.30f, 0.24f},
     8,
     BodyModelType::Cylindrical,
     80.0f,
     4,
     12.0f,
     0.70f,
     {
         {3.0f, 1.8f, 2.0f, 0.0f, 0.4f},  // open
         {1.0f, 2.5f, 0.8f, -0.5f, 0.2f}, // mute
         {0.3f, 8.0f, 0.2f, 0.6f, 0.05f}, // bell-on (metallic strike)
         {0.3f, 6.0f, 0.2f, 0.3f, 0.05f}, // bell-off (damped bell)
     }},

    // 2. CONGA — Afro-Cuban, tall cylindrical, skin head
    // Modes: stretched circular membrane (high tension)
    {{1.00f, 1.58f, 2.12f, 2.61f, 3.10f, 3.55f, 3.98f, 4.08f},
     {1.00f, 0.82f, 0.68f, 0.52f, 0.42f, 0.33f, 0.26f, 0.20f},
     8,
     BodyModelType::Cylindrical,
     200.0f,
     4,
     6.5f,
     0.55f,
     {
         {2.5f, 2.0f, 1.5f, 0.1f, 0.35f},  // open
         {0.6f, 7.0f, 0.4f, 0.5f, 0.1f},   // slap
         {0.8f, 3.0f, 0.5f, -0.5f, 0.15f}, // mute
         {1.5f, 1.5f, 1.0f, 0.2f, 0.4f},   // fingertip
     }},

    // 3. BONGOS — paired small drums, high-pitched
    // Modes: small diameter = higher fundamental, tighter mode spacing
    {{1.00f, 1.56f, 2.08f, 2.55f, 3.02f, 3.48f, 0.0f, 0.0f},
     {1.00f, 0.80f, 0.65f, 0.50f, 0.38f, 0.28f, 0.0f, 0.0f},
     6,
     BodyModelType::Conical,
     350.0f,
     3,
     3.5f,
     0.40f,
     {
         {1.8f, 2.5f, 1.0f, 0.1f, 0.3f},  // open
         {0.5f, 8.0f, 0.3f, 0.5f, 0.1f},  // slap
         {0.4f, 3.0f, 0.3f, -0.6f, 0.1f}, // mute
         {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},  // (unused)
     }},

    // 4. CAJON — Peruvian box drum, rectangular resonance
    // Modes: rectangular membrane modes (different from circular)
    {{1.00f, 1.41f, 1.73f, 2.00f, 2.24f, 2.45f, 2.65f, 2.83f},
     {1.00f, 0.88f, 0.72f, 0.60f, 0.48f, 0.38f, 0.30f, 0.24f},
     8,
     BodyModelType::Box,
     100.0f,
     4,
     5.0f,
     0.50f,
     {
         {3.5f, 1.5f, 2.5f, -0.3f, 0.5f}, // bass (center)
         {0.6f, 7.0f, 0.4f, 0.4f, 0.1f},  // slap (top edge)
         {1.0f, 2.0f, 0.8f, -0.1f, 0.3f}, // ghost (soft)
         {1.2f, 1.8f, 0.6f, 0.2f, 0.35f}, // fingertip
     }},

    // 5. TAIKO — Japanese, thick membrane, large diameter
    // Modes: compressed ratio spacing due to thick hide + barrel body
    {{1.00f, 1.34f, 1.66f, 2.00f, 2.30f, 2.65f, 2.95f, 3.20f},
     {1.00f, 0.92f, 0.80f, 0.68f, 0.56f, 0.45f, 0.36f, 0.28f},
     8,
     BodyModelType::Cylindrical,
     60.0f,
     4,
     15.0f,
     0.75f,
     {
         {4.0f, 1.8f, 3.0f, 0.0f, 0.45f},  // center (full tone)
         {2.0f, 3.0f, 1.5f, 0.2f, 0.3f},   // edge (brighter)
         {0.4f, 10.0f, 0.2f, 0.5f, 0.05f}, // rim (sharp wood crack)
         {3.5f, 2.0f, 2.5f, 0.1f, 0.4f},   // flam (double-strike)
     }},

    // 6. TABLA — Indian, syahi loaded membrane = harmonic modes
    // Modes: uniquely harmonic due to the loaded membrane (syahi paste)
    // Raman (1934): the syahi forces modes into near-harmonic series
    {{1.00f, 1.50f, 2.00f, 3.00f, 3.50f, 4.00f, 0.0f, 0.0f},
     {1.00f, 0.90f, 0.78f, 0.55f, 0.42f, 0.32f, 0.0f, 0.0f},
     6,
     BodyModelType::Conical,
     260.0f,
     4,
     4.0f,
     0.35f,
     {
         {1.5f, 3.0f, 0.8f, 0.3f, 0.2f},  // na (rim, bright)
         {1.0f, 4.0f, 0.5f, 0.5f, 0.15f}, // tin (center, ringing)
         {2.5f, 1.5f, 1.5f, -0.2f, 0.4f}, // tun (bass, open)
         {3.0f, 1.2f, 2.0f, -0.5f, 0.5f}, // ge (bayan bass, deep)
     }},

    // 7. DOUMBEK — Middle Eastern goblet drum, thin skin, ceramic body
    // Modes: similar to djembe but brighter due to thinner membrane
    {{1.00f, 1.62f, 2.20f, 2.72f, 3.25f, 3.78f, 4.15f, 4.30f},
     {1.00f, 0.78f, 0.62f, 0.48f, 0.38f, 0.28f, 0.22f, 0.18f},
     8,
     BodyModelType::Conical,
     220.0f,
     4,
     4.5f,
     0.45f,
     {
         {3.0f, 1.5f, 2.0f, -0.2f, 0.45f}, // doum (bass center)
         {0.5f, 8.0f, 0.3f, 0.5f, 0.08f},  // tek (rim, sharp)
         {0.4f, 6.0f, 0.3f, 0.3f, 0.1f},   // ka (edge, medium)
         {0.3f, 10.0f, 0.2f, 0.6f, 0.05f}, // snap (rim snap)
     }},

    // 8. FRAME DRUM — large open drum, very resonant, long sustain
    // Modes: similar to ideal circular membrane (thin, well-tensioned)
    {{1.00f, 1.59f, 2.14f, 2.30f, 2.65f, 2.92f, 3.16f, 3.50f},
     {1.00f, 0.88f, 0.75f, 0.65f, 0.55f, 0.45f, 0.38f, 0.30f},
     8,
     BodyModelType::Open,
     120.0f,
     4,
     2.0f,
     0.20f,
     {
         {2.5f, 2.0f, 1.5f, 0.0f, 0.35f}, // open (center)
         {1.5f, 3.0f, 1.0f, 0.2f, 0.25f}, // edge
         {5.0f, 1.0f, 4.0f, 0.1f, 0.3f},  // finger-roll (long)
         {0.6f, 7.0f, 0.3f, 0.4f, 0.1f},  // slap
     }},

    // 9. SURDO — Brazilian bass drum, very low, deep resonance
    // Modes: large diameter shifts everything lower, more energy in fundamental
    {{1.00f, 1.48f, 1.95f, 2.40f, 2.82f, 3.22f, 0.0f, 0.0f},
     {1.00f, 0.85f, 0.70f, 0.55f, 0.42f, 0.32f, 0.0f, 0.0f},
     6,
     BodyModelType::Cylindrical,
     55.0f,
     3,
     14.0f,
     0.72f,
     {
         {4.0f, 1.5f, 3.0f, -0.2f, 0.5f}, // open
         {1.5f, 2.0f, 1.0f, -0.5f, 0.3f}, // mute
         {0.3f, 9.0f, 0.2f, 0.5f, 0.05f}, // rim
         {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},  // (unused)
     }},

    // 10. TONGUE DRUM — steel, strongly harmonic, long sustain
    // Modes: near-harmonic (designed tongues act like tuning forks)
    {{1.00f, 2.00f, 3.00f, 4.00f, 5.00f, 6.00f, 0.0f, 0.0f},
     {1.00f, 0.95f, 0.88f, 0.78f, 0.65f, 0.50f, 0.0f, 0.0f},
     6,
     BodyModelType::Open,
     330.0f,
     3,
     1.5f,
     0.15f,
     {
         {1.5f, 2.5f, 1.0f, 0.0f, 0.3f},  // strike
         {2.0f, 2.0f, 1.5f, -0.1f, 0.4f}, // mallet (softer)
         {0.8f, 3.5f, 0.5f, 0.3f, 0.2f},  // harmonic (overtone emphasis)
         {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},  // (unused)
     }},

    // 11. BEATBOX — synthetic/vocal percussion, wide range
    // Modes: non-physical, designed for electronic character
    {{1.00f, 1.50f, 2.50f, 3.50f, 4.50f, 5.50f, 0.0f, 0.0f},
     {1.00f, 0.80f, 0.60f, 0.45f, 0.30f, 0.20f, 0.0f, 0.0f},
     6,
     BodyModelType::Open,
     100.0f,
     4,
     1.0f,
     0.10f,
     {
         {4.0f, 1.5f, 3.0f, -0.4f, 0.6f},  // kick (sub-heavy)
         {0.8f, 6.0f, 0.5f, 0.3f, 0.1f},   // snare (noise-heavy)
         {0.3f, 10.0f, 0.2f, 0.6f, 0.05f}, // hi-hat (short bright)
         {2.0f, 4.0f, 1.0f, 0.2f, 0.25f},  // fx (weird)
     }},
};

//==============================================================================
//
//  III. MODAL MEMBRANE — The soul of each drum sound
//
//  A bank of parallel bandpass resonators, each tuned to a vibrational
//  mode of the drum membrane. Excited by a noise burst (the strike
//  impulse), the modes ring and decay at different rates, creating the
//  characteristic timbre of each instrument.
//
//==============================================================================

//==============================================================================
// OstiModalMembrane — 8-mode parallel bandpass resonator bank.
//
// Each resonator is a CytomicSVF in bandpass mode, tuned to a mode
// frequency derived from the instrument's Bessel zero ratios. The
// excitation is a shaped noise burst whose duration and character
// vary by articulation.
//==============================================================================
class OstiModalMembrane
{
public:
    static constexpr int kMaxModes = 8;

    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        for (auto& r : resonators)
            r.setMode(CytomicSVF::Mode::BandPass);
        reset();
    }

    void trigger(float baseFreqHz, const OstiInstrumentData& inst, int articulation, float brightness,
                 float tuningOffset, float velocityScale, float userExciterMix = 0.5f) noexcept
    {
        // FIX(stability): guard against sr=0 sentinel.
        if (sr <= 0.0f) return;

        // FIX(stability): also clamp below 0 — a negative articulation would be UB.
        articulation = std::min(std::max(articulation, 0), inst.numArticulations - 1);
        const auto& art = inst.articulations[articulation];

        // D004: blend instrument's characteristic exciter mix with user control.
        // userExciterMix 0.5 = instrument default, 0 = force noise, 1 = force pitched.
        // blendedExciterMix is a pure mix fraction [0,1]; velocity is carried separately.
        const float blendedExciterMix =
            lerp(art.exciterMix, userExciterMix < 0.5f ? 0.0f : 1.0f, std::abs(userExciterMix - 0.5f) * 2.0f);

        numActiveModes = inst.numModes;
        float tuneMultiplier = fastExp(tuningOffset * (0.693147f / 12.0f));

        for (int i = 0; i < numActiveModes; ++i)
        {
            float modeFreq = clamp(baseFreqHz * inst.modeRatios[i] * tuneMultiplier, 20.0f, sr * 0.45f);
            // Q derived from mode decay: longer-decaying modes are more resonant.
            // 0.995 base Q near self-oscillation, scaled by brightness and mode decay.
            float modeQ =
                clamp(0.990f * inst.modeDecays[i] + brightness * 0.008f + art.brightnessOffset * 0.005f, 0.5f, 0.9999f);
            resonators[i].setCoefficients(modeFreq, modeQ, sr);

            // Mode amplitude: fundamental strongest, overtones taper.
            // Brightness shifts energy toward higher modes.
            float modePos = static_cast<float>(i) / static_cast<float>(std::max(numActiveModes - 1, 1));
            modeAmplitudes[i] = lerp(1.0f - modePos * 0.6f, 0.5f + modePos * 0.5f, brightness) * inst.modeDecays[i];
        }

        // Excitation: noise burst with pitch spike
        excitationLevel = velocityScale;
        float excDurationSec = art.noiseDurationMs * 0.001f;
        excitationDecay = 1.0f - fastExp(-1.0f / (sr * std::max(excDurationSec, 0.0005f) * 0.368f));

        // Pitch spike: brief high-frequency sine burst before the noise
        if (art.pitchSpikeRatio > 0.1f)
        {
            spikeActive = true;
            spikePhase = 0.0f;
            spikeFreq = baseFreqHz * art.pitchSpikeRatio * tuneMultiplier;
            spikeSamplesLeft = static_cast<int>(sr * art.pitchSpikeDurationMs * 0.001f);
            // FIX(sound): spikeLevel is a velocity-scaled amplitude; excitationLevel is NOT
            // involved in the spike path, so velocity is applied here directly.
            spikeLevel = velocityScale * (1.0f - blendedExciterMix);
        }
        else
        {
            spikeActive = false;
        }

        // FIX(sound): noiseLevel is a pure mix fraction [0,1]. excitationLevel already
        // carries velocityScale; multiplying noiseLevel by velocityScale again would
        // square the velocity in the hot path (noise * excitationLevel * noiseLevel).
        // Keep noiseLevel as the noise/pitched split fraction only:
        //   blendedExciterMix → 1 = pure pitched (spike), 0 = pure noise.
        //   Residual 0.5*(1-mix) preserves some noise colour even at high mix.
        noiseLevel = blendedExciterMix + (1.0f - blendedExciterMix) * 0.5f;
        active = true;
    }

    float process() noexcept
    {
        if (!active)
            return 0.0f;

        // Excitation signal: noise burst + optional pitch spike
        float excitation = 0.0f;

        if (excitationLevel > 1e-6f)
        {
            excitation += noise.process() * excitationLevel * noiseLevel;
            excitationLevel -= excitationLevel * excitationDecay;
            excitationLevel = flushDenormal(excitationLevel);
        }

        if (spikeActive && spikeSamplesLeft > 0)
        {
            spikePhase += spikeFreq / sr;
            while (spikePhase >= 1.0f)
                spikePhase -= 1.0f;
            // FIX(sound): clamp ramp to [0,1] so spikes longer than 3ms don't
            // produce initial amplitudes >1 and clip the excitation bus.
            float spikeRamp = std::min(1.0f, static_cast<float>(spikeSamplesLeft) / std::max(1.0f, sr * 0.003f));
            excitation += fastSin(spikePhase * 6.28318530718f) * spikeLevel * spikeRamp;
            --spikeSamplesLeft;
            if (spikeSamplesLeft <= 0)
                spikeActive = false;
        }

        // Feed excitation through all modes in parallel
        float sum = 0.0f;
        for (int i = 0; i < numActiveModes; ++i)
            sum += resonators[i].processSample(excitation) * modeAmplitudes[i];

        return sum * (1.0f / static_cast<float>(std::max(numActiveModes, 1)));
    }

    void reset() noexcept
    {
        for (auto& r : resonators)
            r.reset();
        excitationLevel = 0.0f;
        spikeActive = false;
        spikeSamplesLeft = 0;
        active = false;
    }

    bool isActive() const noexcept { return active; }

private:
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)
    std::array<CytomicSVF, kMaxModes> resonators;
    float modeAmplitudes[kMaxModes] = {};
    int numActiveModes = 6;
    float excitationLevel = 0.0f;
    float excitationDecay = 0.01f;
    float noiseLevel = 0.5f;
    bool spikeActive = false;
    float spikePhase = 0.0f;
    float spikeFreq = 1000.0f;
    float spikeLevel = 0.5f;
    int spikeSamplesLeft = 0;
    bool active = false;
    OstiNoiseGen noise;
};

//==============================================================================
//
//  IV. WAVEGUIDE BODY — Physical resonance of the drum shell
//
//  Each instrument has a characteristic body shape that colors the sound:
//    Cylindrical — djembe, conga, surdo: long delay, strong reflection
//    Conical     — doumbek, bongos: shorter delay, brighter
//    Box         — cajon: multi-tap delay, wide resonance
//    Open        — frame drum, tongue drum: minimal body coloring
//
//==============================================================================

//==============================================================================
// OstiWaveguideBody — Delay line + allpass + reflection filter per drum body.
//==============================================================================
class OstiWaveguideBody
{
public:
    // 4096 samples: supports body delay up to ~93ms at 44.1kHz
    static constexpr int kMaxDelay = 4096;

    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        reflectionFilter.setMode(CytomicSVF::Mode::LowPass);
        allpassFilter.setMode(CytomicSVF::Mode::AllPass);
        reset();
    }

    void configure(BodyModelType type, float bodyDelayMs, float reflection, float bodyAmount, float brightness) noexcept
    {
        bodyType = type;
        currentBodyAmount = bodyAmount;

        // Delay length from body dimensions
        int delaySamples = static_cast<int>(bodyDelayMs * 0.001f * sr);
        delayLength = std::min(std::max(delaySamples, 1), kMaxDelay - 1);

        // Reflection and feedback depend on body type
        switch (type)
        {
        case BodyModelType::Cylindrical:
            feedbackGain = reflection * 0.65f;
            reflectionCutoff = 2000.0f + brightness * 6000.0f;
            allpassFreq = 800.0f + brightness * 2000.0f;
            break;
        case BodyModelType::Conical:
            feedbackGain = reflection * 0.50f;
            reflectionCutoff = 3000.0f + brightness * 8000.0f;
            allpassFreq = 1200.0f + brightness * 3000.0f;
            delayLength = std::max(delayLength * 2 / 3, 1); // shorter body
            break;
        case BodyModelType::Box:
            feedbackGain = reflection * 0.55f;
            reflectionCutoff = 2500.0f + brightness * 5000.0f;
            allpassFreq = 600.0f + brightness * 1500.0f;
            break;
        case BodyModelType::Open:
            feedbackGain = reflection * 0.20f;
            reflectionCutoff = 5000.0f + brightness * 10000.0f;
            allpassFreq = 2000.0f + brightness * 4000.0f;
            break;
        }

        feedbackGain = clamp(feedbackGain, 0.0f, 0.85f);
        reflectionFilter.setCoefficients(clamp(reflectionCutoff, 200.0f, sr * 0.45f), 0.2f, sr);
        allpassFilter.setCoefficients(clamp(allpassFreq, 100.0f, sr * 0.45f), 0.5f, sr);
    }

    float process(float input) noexcept
    {
        if (currentBodyAmount < 0.001f)
            return input;

        int readPos = writePos - delayLength;
        if (readPos < 0)
            readPos += kMaxDelay;

        float delayed = delayBuffer[readPos];

        // Reflection filter: low-pass in feedback path models energy loss
        float reflected = reflectionFilter.processSample(delayed);
        reflected = flushDenormal(reflected);

        // Allpass diffusion: adds body resonance character
        reflected = allpassFilter.processSample(reflected);
        reflected = flushDenormal(reflected);

        // Multi-tap for box body (cajon)
        float multiTap = 0.0f;
        if (bodyType == BodyModelType::Box)
        {
            int tap2Pos = writePos - (delayLength * 2 / 3);
            if (tap2Pos < 0)
                tap2Pos += kMaxDelay;
            int tap3Pos = writePos - (delayLength / 3);
            if (tap3Pos < 0)
                tap3Pos += kMaxDelay;
            multiTap = (delayBuffer[tap2Pos] + delayBuffer[tap3Pos]) * 0.15f;
        }

        // Write input + feedback into delay line.
        // FIX(stability): exclude multiTap from the feedback write — feeding taps
        // back into the delay creates a secondary feedback loop that can cause
        // instability at high reflection settings on the Box body type.
        delayBuffer[writePos] = input + reflected * feedbackGain;
        writePos = (writePos + 1) & (kMaxDelay - 1);

        // Blend dry input with body resonance
        return input * (1.0f - currentBodyAmount) + (delayed + multiTap) * currentBodyAmount;
    }

    void reset() noexcept
    {
        std::memset(delayBuffer, 0, sizeof(delayBuffer));
        writePos = 0;
        reflectionFilter.reset();
        allpassFilter.reset();
    }

private:
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)
    float delayBuffer[kMaxDelay] = {};
    int writePos = 0;
    int delayLength = 100;
    float feedbackGain = 0.5f;
    float reflectionCutoff = 4000.0f;
    float allpassFreq = 1000.0f;
    float currentBodyAmount = 0.5f;
    BodyModelType bodyType = BodyModelType::Cylindrical;
    CytomicSVF reflectionFilter;
    CytomicSVF allpassFilter;
};

//==============================================================================
// OstiRadiationFilter — Models how sound leaves the drum body.
//
// Open articulations: bright, high-shelf boost at radiation frequency.
// Muted articulations: low-pass, simulating hand dampening.
// The brightness offset from the articulation data controls this.
//==============================================================================
class OstiRadiationFilter
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        filter.setMode(CytomicSVF::Mode::LowPass);
        reset();
    }

    void configure(float brightnessOffset, float brightness) noexcept
    {
        // brightnessOffset: from articulation data. Negative = muted, positive = open.
        // brightness: user control 0-1.
        float cutoff = 4000.0f + (brightnessOffset + brightness) * 8000.0f;
        cutoff = clamp(cutoff, 200.0f, sr * 0.45f);
        float reso = clamp(0.1f + std::abs(brightnessOffset) * 0.2f, 0.0f, 0.8f);
        filter.setCoefficients(cutoff, reso, sr);
    }

    float process(float input) noexcept { return filter.processSample(input); }

    void reset() noexcept { filter.reset(); }

private:
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)
    CytomicSVF filter;
};

//==============================================================================
//
//  V. PATTERN SYSTEM — 16-step autonomous drum patterns
//
//  Each seat can run a 16-step pattern that triggers autonomously.
//  96 patterns total (8 per instrument). Live MIDI overrides the pattern.
//  The GATHER macro controls pattern density and timing humanization.
//
//==============================================================================

//==============================================================================
// OstiPatternData — Embedded patterns for all 12 instruments, 8 each.
//
// Each pattern is 16 steps. Each step has a velocity (0 = rest, >0 = trigger)
// and an articulation index.
//==============================================================================
struct OstiPatternStep
{
    float velocity;   // 0 = rest, 0.01-1.0 = trigger at this velocity
    int articulation; // which articulation to trigger (0-3)
};

struct OstiPattern
{
    OstiPatternStep steps[16];
};

//==============================================================================
// Embedded pattern library: 96 patterns (8 per instrument).
// These are world-rhythm archetypes distilled into 16-step sequences.
//
// Convention:
//   Pattern 0 = basic/foundational
//   Pattern 1 = variation
//   Pattern 2 = fill/accent
//   Pattern 3 = sparse/ambient
//   Pattern 4-7 = style variations
//==============================================================================
static const OstiPattern kPatternLibrary[12][8] = {
    // INSTRUMENT 0: DJEMBE (tone=0, slap=1, bass=2, mute=3)
    {
        // 0: Basic djembe rhythm — West African 12/8 feel in 16 steps
        {{{0.8f, 2},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.4f, 0},
          {0.3f, 3},
          {0.7f, 2},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.4f, 0},
          {0.0f, 0}}},
        // 1: Variation with slaps
        {{{0.7f, 2},
          {0.0f, 0},
          {0.4f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.6f, 1},
          {0.3f, 0},
          {0.0f, 0},
          {0.8f, 2},
          {0.0f, 0},
          {0.4f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.6f, 1},
          {0.4f, 3},
          {0.0f, 0}}},
        // 2: Fill — dense
        {{{0.9f, 2},
          {0.4f, 0},
          {0.6f, 1},
          {0.3f, 0},
          {0.7f, 1},
          {0.4f, 0},
          {0.5f, 0},
          {0.6f, 1},
          {0.8f, 2},
          {0.3f, 0},
          {0.7f, 1},
          {0.4f, 0},
          {0.6f, 1},
          {0.5f, 0},
          {0.7f, 1},
          {0.8f, 2}}},
        // 3: Sparse ambient
        {{{0.6f, 2},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.3f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.4f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0}}},
        // 4: Kuku rhythm approximation
        {{{0.7f, 2},
          {0.0f, 0},
          {0.5f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.4f, 0},
          {0.7f, 2},
          {0.5f, 1},
          {0.0f, 0},
          {0.5f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.4f, 0},
          {0.0f, 0}}},
        // 5: Djembe muted groove
        {{{0.5f, 3},
          {0.3f, 3},
          {0.5f, 0},
          {0.0f, 0},
          {0.6f, 3},
          {0.3f, 3},
          {0.5f, 0},
          {0.0f, 0},
          {0.5f, 3},
          {0.3f, 3},
          {0.5f, 0},
          {0.4f, 1},
          {0.6f, 3},
          {0.3f, 3},
          {0.5f, 0},
          {0.0f, 0}}},
        // 6: Sangban-style
        {{{0.8f, 2},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.7f, 2},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.4f, 0}}},
        // 7: Double-time
        {{{0.7f, 2},
          {0.4f, 0},
          {0.6f, 1},
          {0.4f, 0},
          {0.7f, 2},
          {0.4f, 0},
          {0.6f, 1},
          {0.4f, 0},
          {0.7f, 2},
          {0.4f, 0},
          {0.6f, 1},
          {0.4f, 0},
          {0.7f, 2},
          {0.4f, 0},
          {0.6f, 1},
          {0.5f, 3}}},
    },

    // INSTRUMENT 1: DUNDUN (open=0, mute=1, bell-on=2, bell-off=3)
    {
        {{{0.8f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.0f, 0},
          {0.6f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0}}},
        {{{0.7f, 0},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.6f, 0},
          {0.0f, 0},
          {0.5f, 3},
          {0.0f, 0}}},
        {{{0.9f, 0},
          {0.4f, 2},
          {0.6f, 0},
          {0.5f, 2},
          {0.7f, 0},
          {0.4f, 2},
          {0.6f, 0},
          {0.5f, 2},
          {0.8f, 0},
          {0.4f, 2},
          {0.6f, 1},
          {0.5f, 2},
          {0.7f, 0},
          {0.5f, 2},
          {0.6f, 0},
          {0.8f, 0}}},
        {{{0.6f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0}}},
        {{{0.7f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.0f, 0},
          {0.6f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.0f, 0},
          {0.7f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.0f, 0},
          {0.6f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.5f, 3}}},
        {{{0.6f, 1},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.5f, 2},
          {0.4f, 3}}},
        {{{0.8f, 0},
          {0.0f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.0f, 0},
          {0.6f, 0},
          {0.0f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.6f, 0},
          {0.4f, 2},
          {0.0f, 0}}},
        {{{0.7f, 0},
          {0.3f, 2},
          {0.5f, 0},
          {0.3f, 2},
          {0.7f, 0},
          {0.3f, 2},
          {0.5f, 0},
          {0.3f, 2},
          {0.7f, 0},
          {0.3f, 2},
          {0.5f, 0},
          {0.3f, 2},
          {0.7f, 0},
          {0.3f, 2},
          {0.5f, 0},
          {0.4f, 3}}},
    },

    // INSTRUMENT 2: CONGA (open=0, slap=1, mute=2, fingertip=3)
    {
        {{{0.7f, 0},
          {0.0f, 0},
          {0.5f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.4f, 0},
          {0.0f, 0},
          {0.5f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.5f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.4f, 0},
          {0.3f, 3},
          {0.0f, 0}}},
        {{{0.6f, 0},
          {0.3f, 3},
          {0.5f, 1},
          {0.0f, 0},
          {0.6f, 0},
          {0.3f, 3},
          {0.5f, 1},
          {0.0f, 0},
          {0.6f, 0},
          {0.3f, 3},
          {0.5f, 1},
          {0.0f, 0},
          {0.7f, 0},
          {0.4f, 3},
          {0.5f, 1},
          {0.4f, 3}}},
        {{{0.8f, 0},
          {0.4f, 3},
          {0.6f, 1},
          {0.3f, 3},
          {0.7f, 0},
          {0.4f, 3},
          {0.6f, 1},
          {0.5f, 0},
          {0.8f, 0},
          {0.4f, 3},
          {0.7f, 1},
          {0.3f, 3},
          {0.6f, 0},
          {0.5f, 3},
          {0.7f, 1},
          {0.6f, 0}}},
        {{{0.5f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.3f, 3},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.4f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.3f, 3},
          {0.0f, 0}}},
        {{{0.7f, 0},
          {0.0f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0}}},
        {{{0.5f, 2},
          {0.3f, 2},
          {0.5f, 0},
          {0.0f, 0},
          {0.5f, 2},
          {0.3f, 2},
          {0.5f, 0},
          {0.0f, 0},
          {0.5f, 2},
          {0.3f, 2},
          {0.5f, 0},
          {0.4f, 1},
          {0.5f, 2},
          {0.3f, 2},
          {0.5f, 0},
          {0.0f, 0}}},
        {{{0.6f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.6f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.4f, 3}}},
        {{{0.7f, 0},
          {0.4f, 3},
          {0.6f, 0},
          {0.4f, 3},
          {0.7f, 1},
          {0.4f, 3},
          {0.6f, 0},
          {0.4f, 3},
          {0.7f, 0},
          {0.4f, 3},
          {0.6f, 0},
          {0.4f, 3},
          {0.7f, 1},
          {0.4f, 3},
          {0.6f, 0},
          {0.5f, 1}}},
    },

    // INSTRUMENT 3: BONGOS (open=0, slap=1, mute=2)
    {
        {{{0.7f, 0},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.5f, 0},
          {0.4f, 2}}},
        {{{0.6f, 0},
          {0.4f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.6f, 0},
          {0.4f, 0},
          {0.5f, 1},
          {0.3f, 2},
          {0.6f, 0},
          {0.4f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.6f, 0},
          {0.4f, 0},
          {0.5f, 1},
          {0.3f, 2}}},
        {{{0.8f, 0},
          {0.5f, 0},
          {0.6f, 1},
          {0.4f, 0},
          {0.7f, 1},
          {0.5f, 0},
          {0.6f, 0},
          {0.4f, 1},
          {0.8f, 0},
          {0.5f, 0},
          {0.7f, 1},
          {0.4f, 0},
          {0.6f, 0},
          {0.5f, 1},
          {0.7f, 0},
          {0.6f, 1}}},
        {{{0.4f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.3f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.4f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0}}},
        {{{0.6f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.6f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.6f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.4f, 0}}},
        {{{0.5f, 2},
          {0.4f, 2},
          {0.5f, 0},
          {0.4f, 2},
          {0.5f, 2},
          {0.4f, 2},
          {0.5f, 0},
          {0.4f, 2},
          {0.5f, 2},
          {0.4f, 2},
          {0.5f, 0},
          {0.4f, 2},
          {0.6f, 1},
          {0.4f, 2},
          {0.5f, 0},
          {0.4f, 2}}},
        {{{0.7f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.4f, 0},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.4f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.3f, 2}}},
        {{{0.6f, 0},
          {0.3f, 0},
          {0.5f, 1},
          {0.3f, 0},
          {0.6f, 0},
          {0.3f, 0},
          {0.5f, 1},
          {0.3f, 0},
          {0.6f, 0},
          {0.3f, 0},
          {0.5f, 1},
          {0.3f, 0},
          {0.7f, 0},
          {0.3f, 0},
          {0.6f, 1},
          {0.4f, 0}}},
    },

    // INSTRUMENT 4: CAJON (bass=0, slap=1, ghost=2, fingertip=3)
    {
        {{{0.8f, 0},
          {0.0f, 0},
          {0.3f, 2},
          {0.0f, 0},
          {0.7f, 1},
          {0.0f, 0},
          {0.3f, 2},
          {0.0f, 0},
          {0.8f, 0},
          {0.0f, 0},
          {0.3f, 2},
          {0.0f, 0},
          {0.7f, 1},
          {0.0f, 0},
          {0.3f, 2},
          {0.4f, 3}}},
        {{{0.7f, 0},
          {0.3f, 2},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.3f, 2},
          {0.7f, 0},
          {0.0f, 0},
          {0.3f, 2},
          {0.6f, 1},
          {0.0f, 0},
          {0.3f, 2},
          {0.7f, 0},
          {0.0f, 0},
          {0.6f, 1},
          {0.3f, 2}}},
        {{{0.9f, 0},
          {0.4f, 2},
          {0.6f, 1},
          {0.4f, 2},
          {0.7f, 1},
          {0.4f, 2},
          {0.6f, 0},
          {0.4f, 2},
          {0.8f, 0},
          {0.5f, 2},
          {0.7f, 1},
          {0.4f, 2},
          {0.6f, 0},
          {0.5f, 2},
          {0.7f, 1},
          {0.8f, 0}}},
        {{{0.5f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.4f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0}}},
        {{{0.8f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.7f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.3f, 2},
          {0.8f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.3f, 2},
          {0.7f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.3f, 2}}},
        {{{0.6f, 0},
          {0.3f, 3},
          {0.0f, 0},
          {0.5f, 1},
          {0.3f, 3},
          {0.0f, 0},
          {0.6f, 0},
          {0.3f, 3},
          {0.0f, 0},
          {0.5f, 1},
          {0.3f, 3},
          {0.0f, 0},
          {0.6f, 0},
          {0.3f, 3},
          {0.5f, 1},
          {0.0f, 0}}},
        {{{0.7f, 0},
          {0.0f, 0},
          {0.4f, 2},
          {0.5f, 1},
          {0.0f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.4f, 2},
          {0.5f, 1},
          {0.0f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.4f, 3}}},
        {{{0.7f, 0},
          {0.3f, 2},
          {0.5f, 1},
          {0.3f, 2},
          {0.7f, 0},
          {0.3f, 2},
          {0.5f, 1},
          {0.3f, 2},
          {0.7f, 0},
          {0.3f, 2},
          {0.5f, 1},
          {0.3f, 2},
          {0.8f, 0},
          {0.3f, 2},
          {0.6f, 1},
          {0.4f, 2}}},
    },

    // INSTRUMENT 5: TAIKO (center=0, edge=1, rim=2, flam=3)
    {
        {{{0.9f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.8f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0}}},
        {{{0.9f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.8f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0}}},
        {{{0.9f, 3},
          {0.6f, 0},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.6f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.9f, 3},
          {0.6f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.7f, 0},
          {0.6f, 0},
          {0.8f, 0},
          {0.9f, 3}}},
        {{{0.7f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0}}},
        {{{0.8f, 0},
          {0.0f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.8f, 0},
          {0.0f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.0f, 0}}},
        {{{0.9f, 0},
          {0.0f, 0},
          {0.6f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.9f, 0},
          {0.0f, 0},
          {0.6f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.9f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.7f, 0}}},
        {{{0.8f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.8f, 0},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0}}},
        {{{0.8f, 0},
          {0.5f, 0},
          {0.7f, 0},
          {0.5f, 0},
          {0.8f, 0},
          {0.5f, 0},
          {0.7f, 0},
          {0.5f, 0},
          {0.9f, 3},
          {0.5f, 0},
          {0.7f, 0},
          {0.5f, 0},
          {0.8f, 0},
          {0.5f, 0},
          {0.7f, 0},
          {0.6f, 0}}},
    },

    // INSTRUMENT 6: TABLA (na=0, tin=1, tun=2, ge=3)
    {
        {{{0.6f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.4f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.3f, 0},
          {0.6f, 2},
          {0.0f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.4f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.4f, 3}}},
        {{{0.5f, 0},
          {0.4f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.5f, 0},
          {0.4f, 0},
          {0.6f, 1},
          {0.3f, 3},
          {0.5f, 0},
          {0.4f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.5f, 2},
          {0.4f, 0},
          {0.6f, 1},
          {0.3f, 3}}},
        {{{0.7f, 0},
          {0.5f, 0},
          {0.6f, 1},
          {0.4f, 0},
          {0.6f, 0},
          {0.5f, 1},
          {0.6f, 0},
          {0.4f, 3},
          {0.7f, 2},
          {0.5f, 0},
          {0.6f, 1},
          {0.4f, 0},
          {0.6f, 0},
          {0.5f, 1},
          {0.7f, 0},
          {0.6f, 3}}},
        {{{0.4f, 2},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.3f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.3f, 3},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0}}},
        {{{0.6f, 0},
          {0.3f, 0},
          {0.5f, 1},
          {0.3f, 0},
          {0.6f, 2},
          {0.3f, 0},
          {0.5f, 1},
          {0.3f, 3},
          {0.6f, 0},
          {0.3f, 0},
          {0.5f, 1},
          {0.3f, 0},
          {0.6f, 2},
          {0.3f, 0},
          {0.5f, 1},
          {0.4f, 3}}},
        {{{0.5f, 3},
          {0.0f, 0},
          {0.5f, 3},
          {0.0f, 0},
          {0.5f, 3},
          {0.0f, 0},
          {0.5f, 3},
          {0.0f, 0},
          {0.5f, 3},
          {0.0f, 0},
          {0.5f, 3},
          {0.0f, 0},
          {0.6f, 3},
          {0.0f, 0},
          {0.5f, 3},
          {0.0f, 0}}},
        {{{0.6f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.4f, 3}}},
        {{{0.6f, 0},
          {0.4f, 0},
          {0.5f, 1},
          {0.4f, 0},
          {0.5f, 2},
          {0.4f, 0},
          {0.5f, 1},
          {0.4f, 3},
          {0.6f, 0},
          {0.4f, 0},
          {0.5f, 1},
          {0.4f, 0},
          {0.6f, 2},
          {0.4f, 0},
          {0.5f, 1},
          {0.5f, 3}}},
    },

    // INSTRUMENT 7: DOUMBEK (doum=0, tek=1, ka=2, snap=3)
    {
        {{{0.8f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.4f, 2},
          {0.0f, 0},
          {0.5f, 1},
          {0.4f, 2},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.4f, 2},
          {0.0f, 0},
          {0.5f, 1},
          {0.4f, 2},
          {0.0f, 0}}},
        {{{0.7f, 0},
          {0.4f, 2},
          {0.5f, 1},
          {0.0f, 0},
          {0.6f, 0},
          {0.4f, 2},
          {0.5f, 1},
          {0.3f, 3},
          {0.7f, 0},
          {0.4f, 2},
          {0.5f, 1},
          {0.0f, 0},
          {0.6f, 0},
          {0.4f, 2},
          {0.5f, 1},
          {0.3f, 3}}},
        {{{0.8f, 0},
          {0.5f, 1},
          {0.5f, 2},
          {0.5f, 1},
          {0.7f, 0},
          {0.5f, 1},
          {0.5f, 2},
          {0.5f, 1},
          {0.8f, 0},
          {0.5f, 1},
          {0.6f, 2},
          {0.5f, 1},
          {0.7f, 0},
          {0.5f, 3},
          {0.6f, 1},
          {0.7f, 0}}},
        {{{0.6f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.4f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0}}},
        {{{0.7f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.4f, 2},
          {0.7f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.4f, 2},
          {0.0f, 0}}},
        {{{0.6f, 0},
          {0.3f, 2},
          {0.5f, 1},
          {0.3f, 2},
          {0.6f, 0},
          {0.3f, 2},
          {0.5f, 1},
          {0.3f, 2},
          {0.6f, 0},
          {0.3f, 2},
          {0.5f, 1},
          {0.3f, 3},
          {0.6f, 0},
          {0.3f, 2},
          {0.5f, 1},
          {0.3f, 2}}},
        {{{0.8f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.4f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.4f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.4f, 3}}},
        {{{0.7f, 0},
          {0.4f, 2},
          {0.6f, 1},
          {0.4f, 2},
          {0.7f, 0},
          {0.4f, 2},
          {0.6f, 1},
          {0.4f, 2},
          {0.7f, 0},
          {0.4f, 2},
          {0.6f, 1},
          {0.4f, 2},
          {0.8f, 0},
          {0.4f, 3},
          {0.6f, 1},
          {0.5f, 2}}},
    },

    // INSTRUMENT 8: FRAME DRUM (open=0, edge=1, finger-roll=2, slap=3)
    {
        {{{0.7f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.6f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0}}},
        {{{0.6f, 0},
          {0.0f, 0},
          {0.4f, 1},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.4f, 1},
          {0.3f, 2},
          {0.6f, 0},
          {0.0f, 0},
          {0.4f, 1},
          {0.0f, 0},
          {0.5f, 3},
          {0.0f, 0},
          {0.4f, 1},
          {0.0f, 0}}},
        {{{0.8f, 0},
          {0.4f, 1},
          {0.5f, 0},
          {0.4f, 1},
          {0.6f, 3},
          {0.4f, 1},
          {0.5f, 0},
          {0.5f, 2},
          {0.7f, 0},
          {0.4f, 1},
          {0.5f, 0},
          {0.4f, 1},
          {0.6f, 3},
          {0.4f, 1},
          {0.6f, 0},
          {0.7f, 3}}},
        {{{0.5f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.3f, 2},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0}}},
        {{{0.6f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.4f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.4f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.6f, 0},
          {0.0f, 0},
          {0.4f, 1},
          {0.0f, 0}}},
        {{{0.5f, 2},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.6f, 2},
          {0.0f, 0},
          {0.5f, 2},
          {0.4f, 0}}},
        {{{0.7f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 3},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.6f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 3},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.4f, 1}}},
        {{{0.6f, 0},
          {0.3f, 1},
          {0.5f, 0},
          {0.3f, 1},
          {0.6f, 3},
          {0.3f, 1},
          {0.5f, 0},
          {0.3f, 1},
          {0.6f, 0},
          {0.3f, 1},
          {0.5f, 0},
          {0.3f, 1},
          {0.7f, 3},
          {0.3f, 1},
          {0.5f, 0},
          {0.4f, 2}}},
    },

    // INSTRUMENT 9: SURDO (open=0, mute=1, rim=2)
    {
        {{{0.9f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.8f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0}}},
        {{{0.8f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0}}},
        {{{0.9f, 0},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.5f, 2},
          {0.6f, 0},
          {0.8f, 0},
          {0.0f, 0},
          {0.5f, 2},
          {0.6f, 0},
          {0.7f, 0},
          {0.5f, 2},
          {0.6f, 0},
          {0.8f, 0}}},
        {{{0.6f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0}}},
        {{{0.8f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.8f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.6f, 1},
          {0.0f, 0},
          {0.4f, 2},
          {0.0f, 0}}},
        {{{0.7f, 1},
          {0.0f, 0},
          {0.7f, 1},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.7f, 1},
          {0.0f, 0},
          {0.7f, 1},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.7f, 1},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0}}},
        {{{0.8f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.6f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.7f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 0}}},
        {{{0.8f, 0},
          {0.0f, 0},
          {0.6f, 0},
          {0.0f, 0},
          {0.8f, 0},
          {0.0f, 0},
          {0.6f, 0},
          {0.0f, 0},
          {0.8f, 0},
          {0.0f, 0},
          {0.6f, 0},
          {0.0f, 0},
          {0.9f, 0},
          {0.0f, 0},
          {0.7f, 0},
          {0.5f, 2}}},
    },

    // INSTRUMENT 10: TONGUE DRUM (strike=0, mallet=1, harmonic=2)
    {
        {{{0.6f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.6f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0}}},
        {{{0.5f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.4f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.4f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.4f, 1},
          {0.0f, 0}}},
        {{{0.7f, 0},
          {0.4f, 0},
          {0.5f, 2},
          {0.4f, 0},
          {0.6f, 0},
          {0.4f, 0},
          {0.5f, 2},
          {0.4f, 0},
          {0.7f, 0},
          {0.4f, 0},
          {0.5f, 2},
          {0.4f, 0},
          {0.6f, 0},
          {0.5f, 2},
          {0.6f, 0},
          {0.5f, 2}}},
        {{{0.4f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.3f, 2},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0}}},
        {{{0.5f, 0},
          {0.0f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.4f, 2},
          {0.0f, 0}}},
        {{{0.5f, 2},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.6f, 2},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0}}},
        {{{0.6f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 1},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.6f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.4f, 2}}},
        {{{0.5f, 0},
          {0.3f, 1},
          {0.4f, 0},
          {0.3f, 1},
          {0.5f, 2},
          {0.3f, 1},
          {0.4f, 0},
          {0.3f, 1},
          {0.5f, 0},
          {0.3f, 1},
          {0.4f, 0},
          {0.3f, 1},
          {0.6f, 2},
          {0.3f, 1},
          {0.5f, 0},
          {0.4f, 1}}},
    },

    // INSTRUMENT 11: BEATBOX (kick=0, snare=1, hi-hat=2, fx=3)
    {
        {{{0.9f, 0},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.7f, 1},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.8f, 0},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.7f, 1},
          {0.0f, 0},
          {0.5f, 2},
          {0.4f, 2}}},
        {{{0.8f, 0},
          {0.0f, 0},
          {0.4f, 2},
          {0.5f, 2},
          {0.7f, 1},
          {0.0f, 0},
          {0.4f, 2},
          {0.5f, 2},
          {0.8f, 0},
          {0.4f, 3},
          {0.4f, 2},
          {0.5f, 2},
          {0.7f, 1},
          {0.0f, 0},
          {0.4f, 2},
          {0.5f, 2}}},
        {{{0.9f, 0},
          {0.4f, 2},
          {0.6f, 1},
          {0.4f, 2},
          {0.7f, 0},
          {0.4f, 2},
          {0.6f, 1},
          {0.5f, 2},
          {0.8f, 0},
          {0.4f, 2},
          {0.7f, 1},
          {0.4f, 2},
          {0.6f, 3},
          {0.5f, 2},
          {0.7f, 1},
          {0.8f, 0}}},
        {{{0.6f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.5f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0},
          {0.4f, 3},
          {0.0f, 0},
          {0.0f, 0},
          {0.0f, 0}}},
        {{{0.8f, 0},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.7f, 1},
          {0.0f, 0},
          {0.5f, 2},
          {0.3f, 2},
          {0.8f, 0},
          {0.0f, 0},
          {0.5f, 2},
          {0.0f, 0},
          {0.7f, 1},
          {0.0f, 0},
          {0.5f, 2},
          {0.4f, 3}}},
        {{{0.7f, 0},
          {0.3f, 2},
          {0.0f, 0},
          {0.5f, 1},
          {0.3f, 2},
          {0.0f, 0},
          {0.7f, 0},
          {0.3f, 2},
          {0.0f, 0},
          {0.5f, 1},
          {0.3f, 2},
          {0.0f, 0},
          {0.7f, 0},
          {0.3f, 2},
          {0.6f, 1},
          {0.3f, 2}}},
        {{{0.8f, 0},
          {0.0f, 0},
          {0.4f, 2},
          {0.5f, 3},
          {0.7f, 1},
          {0.0f, 0},
          {0.4f, 2},
          {0.0f, 0},
          {0.8f, 0},
          {0.0f, 0},
          {0.4f, 2},
          {0.5f, 3},
          {0.7f, 1},
          {0.0f, 0},
          {0.4f, 2},
          {0.5f, 3}}},
        {{{0.8f, 0},
          {0.4f, 2},
          {0.6f, 1},
          {0.4f, 2},
          {0.8f, 0},
          {0.4f, 2},
          {0.6f, 1},
          {0.4f, 2},
          {0.8f, 0},
          {0.4f, 2},
          {0.6f, 1},
          {0.4f, 2},
          {0.9f, 0},
          {0.4f, 2},
          {0.7f, 1},
          {0.5f, 3}}},
    },
};

//==============================================================================
// OstiPatternSequencer — Per-seat pattern player with humanization.
//==============================================================================
class OstiPatternSequencer
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        currentStep = 0;
        sampleCounter = 0.0;
        pendingTrigger = false;
        pendingVelocity = 0.0f;
        pendingArticulation = 0;
        liveOverrideActive = false;
        liveOverrideFadeCounter = 0;
    }

    // Set pattern from library — no-op if unchanged (called every block).
    void setPattern(int instrumentIdx, int patternIdx) noexcept
    {
        instrumentIdx = std::min(std::max(instrumentIdx, 0), 11);
        patternIdx    = std::min(std::max(patternIdx, 0), 7);
        // FIX(perf): cache last indices so re-setting the same pattern each block
        // is a fast compare rather than a pointer write that aliases the cache line.
        if (instrumentIdx == lastInstrumentIdx && patternIdx == lastPatternIdx)
            return;
        lastInstrumentIdx = instrumentIdx;
        lastPatternIdx    = patternIdx;
        pattern = &kPatternLibrary[instrumentIdx][patternIdx];
    }

    // Call once per block to advance the sequencer
    void advance(int numSamples, double bpm, float swing, float gatherAmount, float patternVolume,
                 float humanize) noexcept
    {
        if (pattern == nullptr || patternVolume < 0.001f)
            return;
        if (bpm < 1.0)
            bpm = 120.0;

        // Steps per second: 16 steps per bar, 4 beats per bar
        double stepsPerSecond = (bpm / 60.0) * 4.0; // 16th notes per second
        double samplesPerStep = sr / stepsPerSecond;

        // Live override fade: when user plays MIDI, pattern fades out.
        // After 1 bar of silence, it fades back in.
        if (liveOverrideActive)
        {
            liveOverrideFadeCounter -= numSamples;
            if (liveOverrideFadeCounter <= 0)
            {
                liveOverrideActive = false;
                liveOverrideFadeCounter = 0;
            }
            return; // Pattern silenced during live override
        }

        sampleCounter += static_cast<double>(numSamples);

        // Swing: even steps are delayed by swing amount (0-50% of step duration)
        double swingDelay = 0.0;
        if (currentStep % 2 == 1)
            swingDelay = samplesPerStep * static_cast<double>(swing) * 0.005;

        // Humanization: random timing offset
        double humanizeOffset = 0.0;
        if (humanize > 0.001f)
        {
            // Simple deterministic-ish offset from step index
            float hash = fastSin(static_cast<float>(currentStep * 7 + 13) * 0.618f);
            humanizeOffset = static_cast<double>(hash * humanize) * samplesPerStep * 0.1;
        }

        double threshold = samplesPerStep + swingDelay + humanizeOffset;

        if (sampleCounter >= threshold)
        {
            sampleCounter -= threshold;

            const auto& step = pattern->steps[currentStep];

            // GATHER macro controls pattern density: at low gather, skip some steps.
            // At high gather, all steps play (tight/quantized).
            float densityThreshold = (1.0f - gatherAmount) * 0.6f;

            if (step.velocity > 0.001f && step.velocity >= densityThreshold)
            {
                pendingTrigger = true;
                pendingVelocity = step.velocity * patternVolume;
                pendingArticulation = step.articulation;
            }

            currentStep = (currentStep + 1) & 15; // wrap 0-15
        }
    }

    // Signal that live MIDI input has occurred — suppress pattern for 1 bar
    void signalLiveInput(double bpm) noexcept
    {
        liveOverrideActive = true;
        // 1 bar = 4 beats = 4 * (sr * 60 / bpm) samples
        double samplesPerBar = sr * (240.0 / std::max(bpm, 1.0));
        liveOverrideFadeCounter = static_cast<int>(samplesPerBar);
    }

    // Check and consume pending trigger
    bool consumeTrigger(float& velocity, int& articulation) noexcept
    {
        if (!pendingTrigger)
            return false;
        velocity = pendingVelocity;
        articulation = pendingArticulation;
        pendingTrigger = false;
        return true;
    }

private:
    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    const OstiPattern* pattern = nullptr;
    int currentStep = 0;
    double sampleCounter = 0.0;
    bool pendingTrigger = false;
    float pendingVelocity = 0.0f;
    int pendingArticulation = 0;
    bool liveOverrideActive = false;
    int liveOverrideFadeCounter = 0;
    int lastInstrumentIdx = -1; // cached for setPattern change-guard
    int lastPatternIdx    = -1;
};

//==============================================================================
//
//  VI. EFFECTS — Reverb and Compressor
//
//==============================================================================

//==============================================================================
// OstiReverb — Schroeder reverb: 4 combs + 2 allpass diffusers.
// Same topology as OnsetReverb but with SPACE macro integration.
//==============================================================================
class OstiReverb
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        float scale = sr / 44100.0f;
        combLen[0] = static_cast<int>(1116 * scale);
        combLen[1] = static_cast<int>(1188 * scale);
        combLen[2] = static_cast<int>(1277 * scale);
        combLen[3] = static_cast<int>(1356 * scale);
        apLen[0] = static_cast<int>(556 * scale);
        apLen[1] = static_cast<int>(441 * scale);

        for (int c = 0; c < 4; ++c)
        {
            combBufLen[c] = combLen[c] + 1;
            combBuf[c].assign(static_cast<size_t>(combBufLen[c]), 0.0f);
        }
        for (int a = 0; a < 2; ++a)
        {
            apBufLen[a] = apLen[a] + 1;
            apBuf[a].assign(static_cast<size_t>(apBufLen[a]), 0.0f);
        }
        reset();
    }

    void process(float& left, float& right, float roomSize, float damping, float mix) noexcept
    {
        if (mix < 0.001f)
            return;

        float input = (left + right) * 0.5f;
        float fb = 0.5f + roomSize * 0.45f;
        float damp = 0.3f + damping * 0.5f;

        float sum = 0.0f;
        for (int c = 0; c < 4; ++c)
        {
            int rp = combPos[c] - combLen[c];
            if (rp < 0)
                rp += combBufLen[c];
            float del = combBuf[c][static_cast<size_t>(rp)];
            combLP[c] = combLP[c] + damp * (del - combLP[c]);
            combLP[c] = flushDenormal(combLP[c]);
            combBuf[c][static_cast<size_t>(combPos[c])] = input + combLP[c] * fb;
            combPos[c] = (combPos[c] + 1) % combBufLen[c];
            sum += del;
        }
        sum *= 0.25f;

        for (int a = 0; a < 2; ++a)
        {
            int rp = apPos[a] - apLen[a];
            if (rp < 0)
                rp += apBufLen[a];
            float del = apBuf[a][static_cast<size_t>(rp)];
            float apIn = sum + del * 0.5f;
            apBuf[a][static_cast<size_t>(apPos[a])] = apIn;
            sum = del - apIn * 0.5f;
            apPos[a] = (apPos[a] + 1) % apBufLen[a];
        }

        left += sum * mix;
        right += sum * mix;
    }

    void reset() noexcept
    {
        for (int c = 0; c < 4; ++c)
        {
            std::fill(combBuf[c].begin(), combBuf[c].end(), 0.0f);
            combPos[c] = 0;
            combLP[c] = 0.0f;
        }
        for (int a = 0; a < 2; ++a)
        {
            std::fill(apBuf[a].begin(), apBuf[a].end(), 0.0f);
            apPos[a] = 0;
        }
    }

private:
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)
    std::vector<float> combBuf[4];
    float combLP[4] = {};
    int combLen[4] = {1116, 1188, 1277, 1356};
    int combBufLen[4] = {1117, 1189, 1278, 1357};
    int combPos[4] = {};
    std::vector<float> apBuf[2];
    int apLen[2] = {556, 441};
    int apBufLen[2] = {557, 442};
    int apPos[2] = {};
};

//==============================================================================
// OstiCompressor — Simple feed-forward compressor for drum bus.
// Keeps the circle punchy without clipping.
//==============================================================================
class OstiCompressor
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        envLevel = 0.0f;
    }

    void setParams(float threshDb, float ratio, float attackMs, float releaseMs) noexcept
    {
        threshold = dbToGain(threshDb);
        invRatio = 1.0f / std::max(ratio, 1.0f);
        attackCoeff = 1.0f - fastExp(-1.0f / (sr * std::max(attackMs, 0.1f) * 0.001f));
        releaseCoeff = 1.0f - fastExp(-1.0f / (sr * std::max(releaseMs, 1.0f) * 0.001f));
    }

    void process(float& left, float& right) noexcept
    {
        float peak = std::max(std::abs(left), std::abs(right));
        float coeff = (peak > envLevel) ? attackCoeff : releaseCoeff;
        envLevel += coeff * (peak - envLevel);
        envLevel = flushDenormal(envLevel);

        float gain = 1.0f;
        if (envLevel > threshold && threshold > 0.0f)
        {
            float overshoot = envLevel / threshold;
            // FIX(sound): use std::pow instead of fastExp(std::log(x)*r) — the
            // latter mixes an approximate exp with an exact log producing asymmetric
            // error. std::pow is equivalent and avoids the log→fastExp mismatch.
            float compressedOvershoot = std::pow(overshoot, invRatio);
            gain = compressedOvershoot / overshoot;
        }

        left *= gain;
        right *= gain;
    }

    void reset() noexcept { envLevel = 0.0f; }

private:
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)
    float threshold = 0.5f;
    float invRatio = 0.25f;
    float attackCoeff = 0.01f;
    float releaseCoeff = 0.001f;
    float envLevel = 0.0f;
};

//==============================================================================
//
//  VII. VOICE — One seat in the drum circle
//
//  Each voice contains the full DSP chain:
//    Exciter -> Modal Membrane -> Waveguide Body -> Radiation Filter
//    -> SVF Filter -> Amp Envelope -> Output
//
//  Two sub-voices per seat allow rolls and flams (overlapping triggers).
//
//==============================================================================

//==============================================================================
// OstiSubVoice — Single triggered instance of a drum sound.
// Two sub-voices per seat enables rolls and flams.
//==============================================================================
struct OstiSubVoice
{
    OstiModalMembrane membrane;
    OstiWaveguideBody body;
    OstiRadiationFilter radiation;
    OstiEnvelope ampEnv;
    CytomicSVF voiceFilter;
    BreathingLFO breathLFO;

    bool active = false;
    float lastOutput = 0.0f;
    float velocity = 1.0f;
    float baseCutoff = 4000.0f;
    float userExciterMix = 0.5f;
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)
    float stealFade = 1.0f; // voice-steal fade-in: 0→1 over ~2ms to prevent click on steal

    void prepare(double sampleRate)
    {
        sr = static_cast<float>(sampleRate);
        membrane.prepare(sampleRate);
        body.prepare(sampleRate);
        radiation.prepare(sampleRate);
        ampEnv.prepare(sampleRate);
        voiceFilter.setMode(CytomicSVF::Mode::LowPass);
        breathLFO.setRate(0.06f, static_cast<float>(sampleRate));
    }

    void trigger(float vel, int instrument, int articulation, float tuning, float decay, float brightness,
                 float bodyAmount, int bodyModelOverride, float exciterMix, float pitchEnvAmount,
                 float velSens) noexcept
    {
        instrument = std::min(std::max(instrument, 0), static_cast<int>(OstiInstrument::Count) - 1);
        const auto& inst = kInstrumentTable[instrument];
        articulation = std::min(std::max(articulation, 0), inst.numArticulations - 1);

        // Velocity sensitivity: blend between full velocity and fixed 0.8
        float effectiveVel = lerp(0.8f, vel, velSens);
        velocity = effectiveVel;
        active = true;

        // Compute base frequency with tuning offset and pitch envelope.
        // NOTE(design): pitchEnvAmount shifts the resonator tuning at strike time
        // by ±12 semitones. The shift is static for the voice's duration — it acts
        // as a per-hit detune rather than a decaying pitch envelope. A true decaying
        // pitch envelope would require per-sample resonator retuning (expensive).
        // This static-shift approach matches the fast "thwack" pitch of real drums.
        float freq = inst.defaultFreqHz * fastExp(tuning * (0.693147f / 12.0f));
        float initialTuning = tuning + pitchEnvAmount * 12.0f;

        // D004: user exciterMix blends with the articulation's characteristic noise/pitched balance.
        // At exciterMix=0.5 (default), pure instrument character. At 0 = all noise, at 1 = all pitched.
        userExciterMix = exciterMix;

        // Configure modal membrane
        membrane.trigger(freq, inst, articulation, brightness, initialTuning, effectiveVel, exciterMix);

        // Configure body resonance
        BodyModelType bType = (bodyModelOverride >= 0 && bodyModelOverride <= 3)
                                  ? static_cast<BodyModelType>(bodyModelOverride)
                                  : inst.bodyType;
        body.configure(bType, inst.bodyDelayMs, inst.bodyReflection, bodyAmount, brightness);

        // Configure radiation filter
        radiation.configure(inst.articulations[articulation].brightnessOffset, brightness);

        // Amplitude envelope
        float attackTime = 0.0005f + (1.0f - effectiveVel) * 0.002f;
        ampEnv.trigger(attackTime, decay);

        // D001: velocity opens filter — harder hits are brighter
        baseCutoff = 400.0f + brightness * effectiveVel * 16000.0f;
        voiceFilter.reset();
        voiceFilter.setCoefficients(clamp(baseCutoff, 20.0f, sr * 0.45f), 0.15f, sr);
    }

    float processSample() noexcept
    {
        if (!active)
            return 0.0f;

        float envLevel = ampEnv.process();
        if (!ampEnv.isActive())
        {
            active = false;
            lastOutput = 0.0f;
            return 0.0f;
        }

        // Modal membrane -> waveguide body -> radiation filter
        float sample = membrane.process();
        sample = body.process(sample);
        sample = radiation.process(sample);

        // D005: breathing LFO modulates filter cutoff continuously
        float breathMod = breathLFO.process();
        float modCutoff = clamp(baseCutoff * (1.0f + breathMod * 0.12f), 20.0f, sr * 0.45f);
        voiceFilter.setCoefficients_fast(modCutoff, 0.15f, sr);

        sample = voiceFilter.processSample(sample);
        sample *= envLevel * velocity;

        // Voice-steal fade-in: ramp stealFade from 0→1 over ~2ms to mask click
        if (stealFade < 1.0f)
        {
            sample *= stealFade;
            // Rate: reach 1.0 in ~2ms. Use per-sample increment derived from sr.
            stealFade = std::min(1.0f, stealFade + (1.0f / (sr * 0.002f)));
        }

        lastOutput = sample;
        return sample;
    }

    void choke() noexcept
    {
        active = false;
        ampEnv.reset();
        membrane.reset();
        body.reset();
        radiation.reset();
        voiceFilter.reset();
        lastOutput = 0.0f;
    }

    void reset() noexcept
    {
        choke();
        breathLFO.reset();
    }
};

//==============================================================================
// OstiSeat — One seat in the circle: 2 sub-voices + pattern sequencer.
//==============================================================================
struct OstiSeat
{
    static constexpr int kSubVoices = 2;
    OstiSubVoice subVoices[kSubVoices];
    OstiPatternSequencer sequencer;
    int nextSubVoice = 0;
    float peakLevel = 0.0f; // for inter-seat coupling

    void prepare(double sampleRate)
    {
        for (auto& sv : subVoices)
            sv.prepare(sampleRate);
        sequencer.prepare(sampleRate);
        nextSubVoice = 0;
        peakLevel = 0.0f;
    }

    void triggerSeat(float vel, int instrument, int articulation, float tuning, float decay, float brightness,
                     float bodyAmount, int bodyModelOverride, float exciterMix, float pitchEnvAmount,
                     float velSens) noexcept
    {
        auto& sv = subVoices[nextSubVoice];
        // FIX 1: voice stealing — if sub-voice is still active, apply a brief
        // fade-in to the new voice so the hard cut-off doesn't produce a click.
        const bool isSteal = sv.active;
        sv.trigger(vel, instrument, articulation, tuning, decay, brightness, bodyAmount, bodyModelOverride, exciterMix,
                   pitchEnvAmount, velSens);
        sv.stealFade = isSteal ? 0.0f : 1.0f;
        nextSubVoice = (nextSubVoice + 1) % kSubVoices;
    }

    float processSample() noexcept
    {
        float sum = 0.0f;
        for (auto& sv : subVoices)
        {
            if (sv.active)
                sum += sv.processSample();
        }
        float absSample = std::abs(sum);
        if (absSample > peakLevel)
            peakLevel = absSample;
        return sum;
    }

    bool isActive() const noexcept
    {
        for (const auto& sv : subVoices)
            if (sv.active)
                return true;
        return false;
    }

    void choke() noexcept
    {
        for (auto& sv : subVoices)
            sv.choke();
    }

    void reset() noexcept
    {
        for (auto& sv : subVoices)
            sv.reset();
        sequencer.reset();
        nextSubVoice = 0;
        peakLevel = 0.0f;
    }
};

//==============================================================================
//
//  VIII. ENGINE — The complete XOstinato: 8-seat communal drum circle
//
//==============================================================================

class OstinatoEngine : public SynthEngine
{
public:
    static constexpr int kNumSeats = 8;
    // MIDI mapping: seats 0-7 mapped to notes 36-43 (C2-G#2)
    // Additional octave: notes 48-55 (C3-G#3) trigger same seats with different articulation
    static constexpr int kBaseMidiNote = 36;
    static constexpr int kAltMidiNote = 48;

    //-- Static parameter registration -------------------------------------------
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    //-- SynthEngine lifecycle ---------------------------------------------------
    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        blockSize = maxBlockSize;
        couplingBuffer.setSize(3, maxBlockSize); // L, R, envelope follower
        couplingBuffer.clear();

        masterFilterL.setMode(CytomicSVF::Mode::LowPass);
        masterFilterR.setMode(CytomicSVF::Mode::LowPass);
        reverb.prepare(sampleRate);
        compressor.prepare(sampleRate);
        aftertouch.prepare(sampleRate);

        for (int s = 0; s < kNumSeats; ++s)
            seats[s].prepare(sampleRate);

        // FIX(sound): derive envelope follower coefficient from SR (~10ms time constant).
        // 1 - exp(-1/(sr * 0.010)) ≈ the correct one-pole coefficient.
        envFollowerCoeff = 1.0f - std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.010f));

        // SRO SilenceGate: drum synthesis with reverb tail — 500ms hold.
        // Note: OSTINATO also fires via its autonomous pattern sequencer,
        // so the gate is woken on sequencer triggers (see renderBlock step 3).
        prepareSilenceGate(sampleRate, maxBlockSize, 500.0f);
    }

    void releaseResources() override
    {
        for (auto& s : seats)
            s.reset();
    }

    void reset() override
    {
        for (auto& s : seats)
            s.reset();
        couplingBuffer.clear();
        couplingFilterMod = 0.0f;
        couplingDecayMod = 0.0f;
        reverb.reset();
        compressor.reset();
        envFollowerLevel = 0.0f;
        std::memset(seatPeaks, 0, sizeof(seatPeaks));
    }

    //-- Render ------------------------------------------------------------------
    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0)
            return;

        // SRO SilenceGate: wake on live MIDI note-on
        for (const auto& md : midi)
            if (md.getMessage().isNoteOn())
            {
                wakeSilenceGate();
                break;
            }
        // NOTE: bypass check is intentionally NOT applied here for OSTINATO.
        // The autonomous pattern sequencer can fire triggers even when the
        // MIDI buffer is empty. Bypassing on empty MIDI would silence patterns.
        // The gate is still analyzed at the end of the block to correctly track
        // actual audio output and enable bypass when the sequencer is also silent.

        // ---- 1. ParamSnapshot: cache all parameters once per block ----
        float sInstr[kNumSeats], sArtic[kNumSeats], sTune[kNumSeats], sDecay[kNumSeats];
        float sBright[kNumSeats], sBody[kNumSeats], sLevel[kNumSeats], sPan[kNumSeats];
        float sPatVol[kNumSeats], sVelSens[kNumSeats], sPitchEnv[kNumSeats];
        float sExcMix[kNumSeats], sBodyModel[kNumSeats];
        int sPat[kNumSeats];

        for (int s = 0; s < kNumSeats; ++s)
        {
            auto& sp = seatParams[s];
            sInstr[s] = sp.instrument ? sp.instrument->load() : 0.0f;
            sArtic[s] = sp.articulation ? sp.articulation->load() : 0.0f;
            sTune[s] = sp.tuning ? sp.tuning->load() : 0.0f;
            sDecay[s] = clamp((sp.decay ? sp.decay->load() : 0.5f) + couplingDecayMod, 0.01f, 2.0f);
            sBright[s] = clamp((sp.brightness ? sp.brightness->load() : 0.5f) + couplingFilterMod, 0.0f, 1.0f);
            sBody[s] = sp.body ? sp.body->load() : 0.5f;
            sLevel[s] = sp.level ? sp.level->load() : 0.7f;
            sPan[s] = sp.pan ? sp.pan->load() : 0.0f;
            sPat[s] = sp.pattern ? static_cast<int>(sp.pattern->load()) : 0;
            sPatVol[s] = sp.patternVol ? sp.patternVol->load() : 0.5f;
            sVelSens[s] = sp.velSens ? sp.velSens->load() : 0.7f;
            sPitchEnv[s] = sp.pitchEnv ? sp.pitchEnv->load() : 0.0f;
            sExcMix[s] = sp.exciterMix ? sp.exciterMix->load() : 0.5f;
            // Choice: 0=Auto, 1=Cylindrical, 2=Conical, 3=Box, 4=Open
            // Subtract 1 so Auto==-1 (use instrument default), Cylindrical==0, etc.
            sBodyModel[s] = sp.bodyModel ? sp.bodyModel->load() - 1.0f : -1.0f;
        }

        // --- Macro snapshots ---
        float mGather = macroGather ? macroGather->load() : 0.5f;
        float mFire = macroFire ? macroFire->load() : 0.5f;
        float mCircle = macroCircle ? macroCircle->load() : 0.0f;
        float mSpace = macroSpace ? macroSpace->load() : 0.0f;

        // --- Global parameter snapshots ---
        float gTempo = pTempo ? pTempo->load() : 120.0f;
        float gSwing = pSwing ? pSwing->load() : 0.0f;
        float gMasterTune = pMasterTune ? pMasterTune->load() : 0.0f;
        float gMasterDecay = pMasterDecay ? pMasterDecay->load() : 1.0f;
        float gMasterFilt = pMasterFilter ? pMasterFilter->load() : 18000.0f;
        float gMasterReso = pMasterReso ? pMasterReso->load() : 0.1f;
        float gRevSize = pReverbSize ? pReverbSize->load() : 0.4f;
        float gRevDamp = pReverbDamp ? pReverbDamp->load() : 0.3f;
        float gRevMix = pReverbMix ? pReverbMix->load() : 0.15f;
        float gCompThresh = pCompThresh ? pCompThresh->load() : -12.0f;
        float gCompRatio = pCompRatio ? pCompRatio->load() : 4.0f;
        float gCompAttack = pCompAttack ? pCompAttack->load() : 5.0f;
        float gCompRelease = pCompRelease ? pCompRelease->load() : 50.0f;
        float gCircleAmt = pCircleAmt ? pCircleAmt->load() : 0.0f;
        float gHumanize = pHumanize ? pHumanize->load() : 0.3f;
        float gMasterLevel = pMasterLevel ? pMasterLevel->load() : 0.8f;

        // --- Apply macros ---

        // GATHER: tightness — controls humanization and pattern density
        gHumanize *= (1.0f - mGather); // tight gather = less humanize

        // FIRE: intensity — drives exciter energy, filter resonance, compression
        float fireBoost = mFire * 2.0f; // 0-2x drive
        for (int s = 0; s < kNumSeats; ++s)
        {
            sBright[s] = clamp(sBright[s] + mFire * 0.3f, 0.0f, 1.0f);
            sExcMix[s] = clamp(sExcMix[s] + mFire * 0.2f, 0.0f, 1.0f);
        }
        gMasterReso = clamp(gMasterReso + mFire * 0.3f, 0.0f, 0.9f);
        gCompThresh = clamp(gCompThresh - mFire * 6.0f, -40.0f, 0.0f);

        // CIRCLE: inter-seat interaction
        gCircleAmt = clamp(gCircleAmt + mCircle, 0.0f, 1.0f);

        // SPACE: environment
        gRevMix = clamp(gRevMix + mSpace * 0.6f, 0.0f, 1.0f);
        gRevSize = clamp(gRevSize + mSpace * 0.4f, 0.0f, 1.0f);

        // D006: aftertouch -> FIRE boost
        aftertouch.updateBlock(numSamples);
        const float atPressure = aftertouch.getSmoothedPressure(0);
        fireBoost = clamp(fireBoost + atPressure * 0.5f, 0.0f, 3.0f);

        // D006: mod wheel -> CIRCLE depth
        gCircleAmt = clamp(gCircleAmt + modWheelAmount * 0.3f, 0.0f, 1.0f);

        // Apply master tuning to all seats
        for (int s = 0; s < kNumSeats; ++s)
            sTune[s] += gMasterTune;

        // Apply master decay multiplier
        for (int s = 0; s < kNumSeats; ++s)
            sDecay[s] *= gMasterDecay;

        // --- Inter-seat coupling (CIRCLE): previous-block peaks trigger ghosts ---
        if (gCircleAmt > 0.01f)
        {
            for (int s = 0; s < kNumSeats; ++s)
            {
                // Adjacent seats influence each other (circular topology)
                int leftSeat = (s + kNumSeats - 1) % kNumSeats;
                int rightSeat = (s + 1) % kNumSeats;

                float neighborPeak = (seatPeaks[leftSeat] + seatPeaks[rightSeat]) * 0.5f;

                // Sympathetic brightness boost
                sBright[s] = clamp(sBright[s] + neighborPeak * gCircleAmt * 0.2f, 0.0f, 1.0f);

                // Ghost trigger: if neighbor was loud enough, trigger a quiet ghost note.
                // FIX(sound): removed the !isActive() guard — at high CIRCLE with dense
                // patterns every seat is always active, so ghosts never fired. Use the
                // seat's two-subvoice round-robin instead; the voice-steal fade prevents clicks.
                if (neighborPeak * gCircleAmt > 0.4f)
                {
                    int inst = static_cast<int>(sInstr[s]);
                    int art = static_cast<int>(sArtic[s]);
                    // FIX 2: clamp ghost velocity to prevent positive feedback avalanche
                    // at high CIRCLE amounts — ghost triggers feeding more ghost triggers.
                    float ghostVel = std::min(neighborPeak * gCircleAmt * 0.25f, 0.3f);
                    seats[s].triggerSeat(ghostVel, inst, art, sTune[s] + pitchBendNorm * 2.0f, sDecay[s], sBright[s],
                                         sBody[s], static_cast<int>(sBodyModel[s]), sExcMix[s], sPitchEnv[s],
                                         sVelSens[s]);
                }
            }
        }

        // --- Master filter (recompute coefficients only when values change) ---
        // FIX(perf): cache last-used filter params to avoid redundant setCoefficients()
        // calls every block — CytomicSVF coeff computation touches trig functions.
        float filtCutoff = clamp(gMasterFilt, 200.0f, static_cast<float>(sr) * 0.45f);
        if (filtCutoff != lastMasterFiltCutoff || gMasterReso != lastMasterFiltReso)
        {
            masterFilterL.setCoefficients(filtCutoff, gMasterReso, static_cast<float>(sr));
            masterFilterR.setCoefficients(filtCutoff, gMasterReso, static_cast<float>(sr));
            lastMasterFiltCutoff = filtCutoff;
            lastMasterFiltReso   = gMasterReso;
        }

        // --- Compressor (recompute only when params change) ---
        // FIX(perf): setParams calls fastExp twice per block; gate on change.
        if (gCompThresh != lastCompThresh || gCompRatio != lastCompRatio ||
            gCompAttack != lastCompAttack || gCompRelease != lastCompRelease)
        {
            compressor.setParams(gCompThresh, gCompRatio, gCompAttack, gCompRelease);
            lastCompThresh  = gCompThresh;
            lastCompRatio   = gCompRatio;
            lastCompAttack  = gCompAttack;
            lastCompRelease = gCompRelease;
        }

        // --- Clear coupling buffer ---
        couplingBuffer.clear();

        // --- Configure pattern sequencers ---
        for (int s = 0; s < kNumSeats; ++s)
        {
            seats[s].sequencer.setPattern(static_cast<int>(sInstr[s]), sPat[s]);
        }

        // ---- 2. Process MIDI ----
        for (const auto metadata : midi)
        {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                int note = msg.getNoteNumber();
                int seatIdx = -1;
                int artOverride = -1;

                // Primary octave: C2-G#2 (notes 36-43)
                if (note >= kBaseMidiNote && note < kBaseMidiNote + kNumSeats)
                {
                    seatIdx = note - kBaseMidiNote;
                    artOverride = static_cast<int>(sArtic[seatIdx]);
                }
                // Alternate octave: C3-G#3 (notes 48-55) — uses articulation+1
                else if (note >= kAltMidiNote && note < kAltMidiNote + kNumSeats)
                {
                    seatIdx = note - kAltMidiNote;
                    // FIX(stability): clamp to instrument's actual numArticulations, not
                    // the hardcoded 4 — Bongos/Surdo/TongueDrum have only 3 articulations
                    // and index 3 points to the all-zero unused slot in the table.
                    int inst = static_cast<int>(sInstr[seatIdx]);
                    inst = std::min(std::max(inst, 0), static_cast<int>(OstiInstrument::Count) - 1);
                    int numArt = kInstrumentTable[inst].numArticulations;
                    artOverride = (static_cast<int>(sArtic[seatIdx]) + 1) % numArt;
                }

                if (seatIdx >= 0 && seatIdx < kNumSeats)
                {
                    float vel = msg.getFloatVelocity();
                    int inst = static_cast<int>(sInstr[seatIdx]);

                    seats[seatIdx].triggerSeat(vel, inst, artOverride, sTune[seatIdx] + pitchBendNorm * 2.0f,
                                               sDecay[seatIdx], sBright[seatIdx], sBody[seatIdx],
                                               static_cast<int>(sBodyModel[seatIdx]), sExcMix[seatIdx],
                                               sPitchEnv[seatIdx], sVelSens[seatIdx]);

                    // Signal live input to suppress pattern
                    seats[seatIdx].sequencer.signalLiveInput(gTempo);
                }
            }
            else if (msg.isChannelPressure())
            {
                aftertouch.setChannelPressure(msg.getChannelPressureValue() / 127.0f);
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                modWheelAmount = msg.getControllerValue() / 127.0f;
            }
            else if (msg.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
        }

        // ---- 3. Advance pattern sequencers and trigger ----
        for (int s = 0; s < kNumSeats; ++s)
        {
            seats[s].sequencer.advance(numSamples, gTempo, gSwing, mGather, sPatVol[s], gHumanize);
            float patVel = 0.0f;
            int patArt = 0;
            if (seats[s].sequencer.consumeTrigger(patVel, patArt))
            {
                // SRO SilenceGate: sequencer-fired trigger also wakes the gate
                wakeSilenceGate();
                int inst = static_cast<int>(sInstr[s]);
                seats[s].triggerSeat(patVel, inst, patArt, sTune[s] + pitchBendNorm * 2.0f, sDecay[s], sBright[s],
                                     sBody[s], static_cast<int>(sBodyModel[s]), sExcMix[s], sPitchEnv[s], sVelSens[s]);
            }
        }

        // ---- 4. Precompute block-constant pan gains ----
        float panGainL[kNumSeats], panGainR[kNumSeats];
        for (int s = 0; s < kNumSeats; ++s)
        {
            // Default circular pan: seats distributed around the stereo field
            float circlePan = sPan[s];
            panGainL[s] = std::sqrt(0.5f * (1.0f - circlePan));
            panGainR[s] = std::sqrt(0.5f * (1.0f + circlePan));
        }

        // ---- 5. Render all seats ----
        auto* outL = buffer.getWritePointer(0);
        auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
        auto* cplL = couplingBuffer.getWritePointer(0);
        auto* cplR = couplingBuffer.getWritePointer(1);
        auto* cplEnv = couplingBuffer.getWritePointer(2);

        float blockPeaks[kNumSeats] = {};

        for (int i = 0; i < numSamples; ++i)
        {
            float sumL = 0.0f, sumR = 0.0f;

            for (int s = 0; s < kNumSeats; ++s)
            {
                float sample = seats[s].processSample();
                float absSamp = std::abs(sample * sLevel[s]);
                if (absSamp > blockPeaks[s])
                    blockPeaks[s] = absSamp;

                sumL += sample * sLevel[s] * panGainL[s];
                sumR += sample * sLevel[s] * panGainR[s];
            }

            // FIRE: soft-clip drive
            if (fireBoost > 0.1f)
            {
                float driveGain = 1.0f + fireBoost * 2.0f;
                sumL = fastTanh(sumL * driveGain) / driveGain * (1.0f + fireBoost * 0.5f);
                sumR = fastTanh(sumR * driveGain) / driveGain * (1.0f + fireBoost * 0.5f);
            }

            // Master filter
            sumL = masterFilterL.processSample(sumL);
            sumR = masterFilterR.processSample(sumR);

            // Compressor
            compressor.process(sumL, sumR);

            // Reverb
            reverb.process(sumL, sumR, gRevSize, gRevDamp, gRevMix);

            // Master level
            sumL *= gMasterLevel;
            sumR *= gMasterLevel;

            // Envelope follower for coupling output (one-pole, ~10ms)
            // FIX(sound): derive coefficient from SR so the ~10ms time constant is
            // correct at 48kHz/96kHz — hardcoded 0.002f was 3-5x too slow at those rates.
            float monoAbs = std::abs(sumL + sumR) * 0.5f;
            envFollowerLevel += envFollowerCoeff * (monoAbs - envFollowerLevel);
            envFollowerLevel = flushDenormal(envFollowerLevel);

            outL[i] += sumL;
            if (outR)
                outR[i] += sumR;
            cplL[i] = sumL;
            cplR[i] = sumR;
            cplEnv[i] = envFollowerLevel;
        }

        // Store seat peaks for next block's CIRCLE coupling
        for (int s = 0; s < kNumSeats; ++s)
        {
            seatPeaks[s] = blockPeaks[s];
            seats[s].peakLevel = 0.0f; // reset for next block
        }

        // Clear per-block coupling accumulators
        couplingFilterMod = 0.0f;
        couplingDecayMod = 0.0f;

        activeVoiceCounter.store(countActiveVoices(), std::memory_order_relaxed);

        // SRO SilenceGate: feed output to the gate for silence detection
        analyzeForSilenceGate(buffer, numSamples);
    }

    //-- Coupling ----------------------------------------------------------------
    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        if (channel < couplingBuffer.getNumChannels() && sampleIndex < couplingBuffer.getNumSamples())
            return couplingBuffer.getSample(channel, sampleIndex);
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* sourceBuffer, int numSamples) override
    {
        float sum = 0.0f;
        if (sourceBuffer != nullptr)
            for (int i = 0; i < numSamples; ++i)
                sum += sourceBuffer[i];
        float avgMod =
            (numSamples > 0 && sourceBuffer != nullptr) ? (sum / static_cast<float>(numSamples)) * amount : amount;

        switch (type)
        {
        case CouplingType::AmpToFilter:
            couplingFilterMod += avgMod;
            break;
        case CouplingType::EnvToDecay:
            couplingDecayMod += avgMod;
            break;
        case CouplingType::RhythmToBlend: /* modulates pattern density externally */
            break;
        case CouplingType::AmpToChoke:
            // ONSET x OSTINATO "Machine Meets Human":
            // ONSET triggers ghost notes in OSTINATO via AmpToChoke
            if (avgMod > 0.3f)
            {
                // Don't choke — instead trigger ghost notes on random seats
                int ghostSeat = static_cast<int>(avgMod * 7.99f) % kNumSeats;
                // FIX(stability): clamp injected peak to 1.0 to prevent the
                // coupling accumulator from stacking across multiple calls and
                // overdriving the CIRCLE ghost threshold.
                seatPeaks[ghostSeat] = std::min(1.0f, std::max(seatPeaks[ghostSeat], avgMod * 0.5f));
            }
            break;
        case CouplingType::AudioToFM:
            // Audio from source modulates exciter brightness
            couplingFilterMod += avgMod * 0.3f;
            break;
        default:
            break;
        }
    }

    //-- Parameters --------------------------------------------------------------

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        for (int s = 0; s < kNumSeats; ++s)
        {
            juce::String pre = "osti_seat" + juce::String(s + 1) + "_";
            auto& sp = seatParams[s];
            sp.instrument = apvts.getRawParameterValue(pre + "instrument");
            sp.articulation = apvts.getRawParameterValue(pre + "articulation");
            sp.tuning = apvts.getRawParameterValue(pre + "tuning");
            sp.decay = apvts.getRawParameterValue(pre + "decay");
            sp.brightness = apvts.getRawParameterValue(pre + "brightness");
            sp.body = apvts.getRawParameterValue(pre + "body");
            sp.level = apvts.getRawParameterValue(pre + "level");
            sp.pan = apvts.getRawParameterValue(pre + "pan");
            sp.pattern = apvts.getRawParameterValue(pre + "pattern");
            sp.patternVol = apvts.getRawParameterValue(pre + "patternVol");
            sp.velSens = apvts.getRawParameterValue(pre + "velSens");
            sp.pitchEnv = apvts.getRawParameterValue(pre + "pitchEnv");
            sp.exciterMix = apvts.getRawParameterValue(pre + "exciterMix");
            sp.bodyModel = apvts.getRawParameterValue(pre + "bodyModel");
        }

        // Macros
        macroGather = apvts.getRawParameterValue("osti_macroGather");
        macroFire = apvts.getRawParameterValue("osti_macroFire");
        macroCircle = apvts.getRawParameterValue("osti_macroCircle");
        macroSpace = apvts.getRawParameterValue("osti_macroSpace");

        // Globals
        pTempo = apvts.getRawParameterValue("osti_tempo");
        pSwing = apvts.getRawParameterValue("osti_swing");
        pMasterTune = apvts.getRawParameterValue("osti_masterTune");
        pMasterDecay = apvts.getRawParameterValue("osti_masterDecay");
        pMasterFilter = apvts.getRawParameterValue("osti_masterFilter");
        pMasterReso = apvts.getRawParameterValue("osti_masterReso");
        pReverbSize = apvts.getRawParameterValue("osti_reverbSize");
        pReverbDamp = apvts.getRawParameterValue("osti_reverbDamp");
        pReverbMix = apvts.getRawParameterValue("osti_reverbMix");
        pCompThresh = apvts.getRawParameterValue("osti_compThresh");
        pCompRatio = apvts.getRawParameterValue("osti_compRatio");
        pCompAttack = apvts.getRawParameterValue("osti_compAttack");
        pCompRelease = apvts.getRawParameterValue("osti_compRelease");
        pCircleAmt = apvts.getRawParameterValue("osti_circleAmount");
        pHumanize = apvts.getRawParameterValue("osti_humanize");
        pMasterLevel = apvts.getRawParameterValue("osti_masterLevel");
    }

    //-- Identity ----------------------------------------------------------------
    juce::String getEngineId() const override { return "Ostinato"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFE8701A); }
    int getMaxVoices() const override { return kNumSeats * 2; } // 2 sub-voices per seat
    int getActiveVoiceCount() const override { return activeVoiceCounter.load(std::memory_order_relaxed); }

private:
    std::array<OstiSeat, kNumSeats> seats;
    juce::AudioBuffer<float> couplingBuffer;
    CytomicSVF masterFilterL;
    CytomicSVF masterFilterR;
    OstiReverb reverb;
    OstiCompressor compressor;
    PolyAftertouch aftertouch;
    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    int blockSize = 512;
    std::atomic<int> activeVoiceCounter{0};

    // Coupling modulation accumulators
    float couplingFilterMod = 0.0f;
    float couplingDecayMod = 0.0f;

    // Envelope follower for coupling output
    float envFollowerLevel = 0.0f;
    float envFollowerCoeff = 0.002f; // recomputed in prepare() from SR (target ~10ms)

    // Inter-seat coupling: previous-block peak levels
    float seatPeaks[kNumSeats] = {};

    // FIX(perf): cached params for change-gated recompute (master filter + compressor)
    float lastMasterFiltCutoff = -1.0f;
    float lastMasterFiltReso   = -1.0f;
    float lastCompThresh  = -999.0f;
    float lastCompRatio   = -1.0f;
    float lastCompAttack  = -1.0f;
    float lastCompRelease = -1.0f;

    // D006: mod wheel (CC#1) -> CIRCLE macro depth
    float modWheelAmount = 0.0f;
    float pitchBendNorm = 0.0f; // MIDI pitch wheel [-1, +1]; ±2 semitone range

    // Cached parameter pointers (ParamSnapshot pattern)
    struct SeatParameterPointers
    {
        std::atomic<float>* instrument = nullptr;
        std::atomic<float>* articulation = nullptr;
        std::atomic<float>* tuning = nullptr;
        std::atomic<float>* decay = nullptr;
        std::atomic<float>* brightness = nullptr;
        std::atomic<float>* body = nullptr;
        std::atomic<float>* level = nullptr;
        std::atomic<float>* pan = nullptr;
        std::atomic<float>* pattern = nullptr;
        std::atomic<float>* patternVol = nullptr;
        std::atomic<float>* velSens = nullptr;
        std::atomic<float>* pitchEnv = nullptr;
        std::atomic<float>* exciterMix = nullptr;
        std::atomic<float>* bodyModel = nullptr;
    };
    std::array<SeatParameterPointers, kNumSeats> seatParams;

    // Macro parameter pointers
    std::atomic<float>* macroGather = nullptr;
    std::atomic<float>* macroFire = nullptr;
    std::atomic<float>* macroCircle = nullptr;
    std::atomic<float>* macroSpace = nullptr;

    // Global parameter pointers
    std::atomic<float>* pTempo = nullptr;
    std::atomic<float>* pSwing = nullptr;
    std::atomic<float>* pMasterTune = nullptr;
    std::atomic<float>* pMasterDecay = nullptr;
    std::atomic<float>* pMasterFilter = nullptr;
    std::atomic<float>* pMasterReso = nullptr;
    std::atomic<float>* pReverbSize = nullptr;
    std::atomic<float>* pReverbDamp = nullptr;
    std::atomic<float>* pReverbMix = nullptr;
    std::atomic<float>* pCompThresh = nullptr;
    std::atomic<float>* pCompRatio = nullptr;
    std::atomic<float>* pCompAttack = nullptr;
    std::atomic<float>* pCompRelease = nullptr;
    std::atomic<float>* pCircleAmt = nullptr;
    std::atomic<float>* pHumanize = nullptr;
    std::atomic<float>* pMasterLevel = nullptr;

    //-- Voice counting ----------------------------------------------------------
    int countActiveVoices() const noexcept
    {
        int count = 0;
        for (const auto& seat : seats)
            for (const auto& sv : seat.subVoices)
                if (sv.active)
                    ++count;
        return count;
    }

    //-- Parameter registration --------------------------------------------------
    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using Float = juce::AudioParameterFloat;
        using Choice = juce::AudioParameterChoice;

        static const juce::StringArray instrumentNames{"Djembe",     "Dundun", "Conga",       "Bongos",
                                                       "Cajon",      "Taiko",  "Tabla",       "Doumbek",
                                                       "Frame Drum", "Surdo",  "Tongue Drum", "Beatbox"};

        // bodyModelNames intentionally removed — the bodyModel Choice uses a 5-entry
        // inline StringArray {"Auto","Cylindrical","Conical","Box","Open"} below.

        static const juce::StringArray patternNames{"Basic",   "Variation", "Fill",    "Sparse",
                                                    "Style A", "Style B",   "Style C", "Double"};

        // Default instruments per seat: a well-rounded circle
        static const int defInstrument[kNumSeats] = {0, 5, 2, 6, 4, 7, 8, 9};
        // Default: Djembe, Taiko, Conga, Tabla, Cajon, Doumbek, Frame, Surdo

        // Default pans: seats distributed in a circle around stereo field
        static const float defPan[kNumSeats] = {-0.7f, -0.4f, -0.1f, 0.2f, 0.5f, 0.3f, 0.0f, -0.5f};

        for (int s = 0; s < kNumSeats; ++s)
        {
            juce::String pre = "osti_seat" + juce::String(s + 1) + "_";
            juce::String name = "Seat " + juce::String(s + 1);

            params.push_back(std::make_unique<Choice>(juce::ParameterID(pre + "instrument", 1), name + " Instrument",
                                                      instrumentNames, defInstrument[s]));

            params.push_back(std::make_unique<Choice>(juce::ParameterID(pre + "articulation", 1),
                                                      name + " Articulation",
                                                      juce::StringArray{"Art 1", "Art 2", "Art 3", "Art 4"}, 0));

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "tuning", 1), name + " Tuning",
                                                     juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "decay", 1), name + " Decay",
                                                     juce::NormalisableRange<float>(0.01f, 2.0f, 0.0f, 0.4f), 0.5f));

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "brightness", 1), name + " Brightness",
                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "body", 1), name + " Body",
                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "level", 1), name + " Level",
                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "pan", 1), name + " Pan",
                                                     juce::NormalisableRange<float>(-1.0f, 1.0f), defPan[s]));

            params.push_back(
                std::make_unique<Choice>(juce::ParameterID(pre + "pattern", 1), name + " Pattern", patternNames, 0));

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "patternVol", 1), name + " Pattern Vol",
                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "velSens", 1), name + " Vel Sens",
                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "pitchEnv", 1), name + " Pitch Env",
                                                     juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));

            params.push_back(std::make_unique<Float>(juce::ParameterID(pre + "exciterMix", 1), name + " Exciter Mix",
                                                     juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

            params.push_back(
                std::make_unique<Choice>(juce::ParameterID(pre + "bodyModel", 1), name + " Body Model",
                                         juce::StringArray{"Auto", "Cylindrical", "Conical", "Box", "Open"}, 0));
        }

        // ---- Macros ----
        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_macroGather", 1), "Gather",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_macroFire", 1), "Fire",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_macroCircle", 1), "Circle",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_macroSpace", 1), "Space",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // ---- Global parameters ----
        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_tempo", 1), "Osti Tempo",
                                                 juce::NormalisableRange<float>(40.0f, 300.0f), 120.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_swing", 1), "Osti Swing",
                                                 juce::NormalisableRange<float>(0.0f, 100.0f), 0.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_masterTune", 1), "Osti Master Tune",
                                                 juce::NormalisableRange<float>(-24.0f, 24.0f), 0.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_masterDecay", 1), "Osti Master Decay",
                                                 juce::NormalisableRange<float>(0.5f, 2.0f), 1.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_masterFilter", 1), "Osti Master Filter",
                                                 juce::NormalisableRange<float>(200.0f, 20000.0f, 0.0f, 0.3f),
                                                 18000.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_masterReso", 1), "Osti Master Reso",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.1f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_reverbSize", 1), "Osti Reverb Size",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_reverbDamp", 1), "Osti Reverb Damp",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_reverbMix", 1), "Osti Reverb Mix",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.15f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_compThresh", 1), "Osti Comp Thresh",
                                                 juce::NormalisableRange<float>(-40.0f, 0.0f), -12.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_compRatio", 1), "Osti Comp Ratio",
                                                 juce::NormalisableRange<float>(1.0f, 20.0f, 0.0f, 0.5f), 4.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_compAttack", 1), "Osti Comp Attack",
                                                 juce::NormalisableRange<float>(0.1f, 50.0f, 0.0f, 0.5f), 5.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_compRelease", 1), "Osti Comp Release",
                                                 juce::NormalisableRange<float>(10.0f, 500.0f, 0.0f, 0.5f), 50.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_circleAmount", 1), "Osti Circle Amount",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_humanize", 1), "Osti Humanize",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        params.push_back(std::make_unique<Float>(juce::ParameterID("osti_masterLevel", 1), "Osti Master Level",
                                                 juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));
    }
};

//==============================================================================
// End of OstinatoEngine — The Fire Circle
// Eight seats, twelve traditions, one fire. Every rhythm a prayer.
//==============================================================================

} // namespace xoceanus
