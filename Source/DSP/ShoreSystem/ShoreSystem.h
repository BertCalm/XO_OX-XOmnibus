#pragma once
#include "../FastMath.h"
#include <array>
#include <cmath>

namespace xoceanus {

//==============================================================================
// ShoreSystem — Shared coastal region model for OSPREY and OSTERIA engines.
//
// Defines 5 shore regions (Atlantic, Nordic, Mediterranean, Pacific, Southern)
// as continuous synthesis dimensions. Each shore encodes:
//   - Resonator partial profiles (folk instrument spectral fingerprints)
//   - Creature voice formant targets (animal/environment calls)
//   - Fluid character (swell shape, chop frequency, depth)
//   - Room character (tavern acoustic fingerprint)
//
// The shore parameter (0.0–4.0) morphs continuously between adjacent shores
// via coefficient interpolation. Integer values = pure shores. Fractional
// values = hybrid coastlines that exist nowhere on Earth.
//
// Thread safety: all data is constexpr/static. No allocation. No mutation.
//==============================================================================

//==============================================================================
// Shore indices
//==============================================================================
enum class Shore : int
{
    Atlantic      = 0,
    Nordic        = 1,
    Mediterranean = 2,
    Pacific       = 3,
    Southern      = 4,
    Count         = 5
};

//==============================================================================
// Resonator profile — spectral fingerprint of a folk instrument voice.
// 4 formant bands define the instrument character.
//==============================================================================
struct ResonatorProfile
{
    const char* name;
    float formantFreqs[4];     // Hz — 4 formant center frequencies
    float formantBandwidths[4]; // Hz — formant bandwidths (Q inverse)
    float formantGains[4];     // linear 0-1 — relative formant energy
    float attackMs;            // typical attack time
    float decayMs;             // typical ring-out time
    float brightness;          // 0-1 high-partial emphasis
    float inharmonicity;       // 0-1 how much partials deviate from harmonic series
};

//==============================================================================
// Per-shore resonator sets: 3 instruments per shore.
// [0] = bass/drone, [1] = chordal/harmonic, [2] = melodic/lead
//==============================================================================
struct ShoreResonators
{
    ResonatorProfile instruments[3];
};

static constexpr ShoreResonators kShoreResonators[5] = {
    // --- ATLANTIC (0.0) ---
    // Guitarra portuguesa, Kora, Uilleann pipes
    {{
        { "Guitarra",
          { 320.0f, 1200.0f, 2800.0f, 5500.0f },
          { 80.0f,  150.0f,  200.0f,  300.0f },
          { 1.0f, 0.7f, 0.5f, 0.3f },
          5.0f, 800.0f, 0.7f, 0.05f },
        { "Kora",
          { 250.0f, 900.0f, 2200.0f, 4800.0f },
          { 60.0f,  120.0f,  180.0f,  250.0f },
          { 1.0f, 0.8f, 0.4f, 0.2f },
          3.0f, 600.0f, 0.6f, 0.03f },
        { "Uilleann",
          { 400.0f, 1500.0f, 3200.0f, 6000.0f },
          { 100.0f, 200.0f,  250.0f,  350.0f },
          { 1.0f, 0.9f, 0.6f, 0.4f },
          15.0f, 2000.0f, 0.75f, 0.02f }
    }},
    // --- NORDIC (1.0) ---
    // Hardingfele, Langspil, Kulning
    {{
        { "Hardingfele",
          { 280.0f, 1100.0f, 2600.0f, 5200.0f },
          { 70.0f,  130.0f,  170.0f,  280.0f },
          { 1.0f, 0.8f, 0.6f, 0.5f },
          8.0f, 1200.0f, 0.8f, 0.04f },
        { "Langspil",
          { 200.0f, 700.0f, 1800.0f, 4000.0f },
          { 50.0f,  100.0f, 160.0f,  220.0f },
          { 1.0f, 0.6f, 0.3f, 0.15f },
          20.0f, 3000.0f, 0.4f, 0.01f },
        { "Kulning",
          { 500.0f, 1800.0f, 3500.0f, 7000.0f },
          { 120.0f, 250.0f,  300.0f,  400.0f },
          { 0.8f, 1.0f, 0.7f, 0.3f },
          10.0f, 1500.0f, 0.85f, 0.01f }
    }},
    // --- MEDITERRANEAN (2.0) ---
    // Bouzouki, Oud, Ney
    {{
        { "Bouzouki",
          { 350.0f, 1300.0f, 3000.0f, 5800.0f },
          { 90.0f,  160.0f,  220.0f,  320.0f },
          { 1.0f, 0.75f, 0.55f, 0.35f },
          2.0f, 500.0f, 0.8f, 0.06f },
        { "Oud",
          { 220.0f, 850.0f, 2100.0f, 4500.0f },
          { 55.0f,  110.0f,  170.0f,  240.0f },
          { 1.0f, 0.85f, 0.5f, 0.25f },
          4.0f, 700.0f, 0.55f, 0.08f },
        { "Ney",
          { 450.0f, 1600.0f, 3400.0f, 6500.0f },
          { 110.0f, 220.0f,  280.0f,  380.0f },
          { 0.9f, 1.0f, 0.65f, 0.35f },
          25.0f, 2500.0f, 0.7f, 0.01f }
    }},
    // --- PACIFIC (3.0) ---
    // Koto, Conch, Singing Bowl
    {{
        { "Koto",
          { 300.0f, 1150.0f, 2700.0f, 5400.0f },
          { 75.0f,  140.0f,  190.0f,  290.0f },
          { 1.0f, 0.7f, 0.45f, 0.2f },
          1.5f, 400.0f, 0.75f, 0.04f },
        { "Conch",
          { 180.0f, 650.0f, 1600.0f, 3800.0f },
          { 45.0f,  90.0f,  150.0f,  200.0f },
          { 1.0f, 0.6f, 0.35f, 0.15f },
          30.0f, 4000.0f, 0.35f, 0.12f },
        { "SingingBowl",
          { 260.0f, 980.0f, 2400.0f, 5000.0f },
          { 30.0f,  60.0f,  100.0f,  150.0f },
          { 1.0f, 0.9f, 0.7f, 0.5f },
          2.0f, 8000.0f, 0.65f, 0.15f }
    }},
    // --- SOUTHERN (4.0) ---
    // Cavaquinho, Valiha, Gamelan
    {{
        { "Cavaquinho",
          { 380.0f, 1400.0f, 3100.0f, 6200.0f },
          { 95.0f,  170.0f,  230.0f,  340.0f },
          { 1.0f, 0.7f, 0.5f, 0.3f },
          1.0f, 350.0f, 0.85f, 0.03f },
        { "Valiha",
          { 240.0f, 880.0f, 2000.0f, 4400.0f },
          { 60.0f,  110.0f,  165.0f,  230.0f },
          { 1.0f, 0.75f, 0.45f, 0.2f },
          3.0f, 500.0f, 0.6f, 0.07f },
        { "Gamelan",
          { 310.0f, 1050.0f, 2500.0f, 5100.0f },
          { 40.0f,  70.0f,  120.0f,  180.0f },
          { 1.0f, 0.85f, 0.7f, 0.55f },
          1.0f, 6000.0f, 0.7f, 0.2f }
    }}
};

//==============================================================================
// Creature voice definitions — formant sweep targets for ocean creatures/sounds.
// Each creature is defined by start/end formant pairs and sweep timing.
//==============================================================================
struct CreatureVoice
{
    const char* name;
    float startFreqs[3];    // Hz — formant start positions
    float endFreqs[3];      // Hz — formant end positions
    float bandwidths[3];    // Hz — formant widths
    float sweepMs;          // duration of the formant sweep
    float gapMs;            // silence between calls
    float amplitude;        // relative level
};

struct ShoreCreatures
{
    CreatureVoice creatures[3]; // per shore: [0]=bird, [1]=whale/deep, [2]=ambient
};

static constexpr ShoreCreatures kShoreCreatures[5] = {
    // ATLANTIC
    {{ { "StormPetrel",   { 2500.0f, 4200.0f, 6800.0f }, { 3800.0f, 5500.0f, 8000.0f },
                          { 300.0f, 400.0f, 500.0f }, 120.0f, 2000.0f, 0.4f },
       { "HumpbackWhale", { 80.0f, 250.0f, 600.0f },    { 200.0f, 500.0f, 1200.0f },
                          { 30.0f, 60.0f, 100.0f },  3000.0f, 8000.0f, 0.6f },
       { "HarborFoghorn", { 120.0f, 350.0f, 800.0f },   { 120.0f, 350.0f, 800.0f },
                          { 40.0f, 70.0f, 120.0f },  4000.0f, 15000.0f, 0.5f } }},
    // NORDIC
    {{ { "ArcticTern",    { 3000.0f, 5000.0f, 7500.0f }, { 4500.0f, 6500.0f, 9000.0f },
                          { 350.0f, 450.0f, 550.0f }, 80.0f, 3000.0f, 0.35f },
       { "BelugaWhale",   { 400.0f, 1200.0f, 3000.0f }, { 800.0f, 2000.0f, 5000.0f },
                          { 80.0f, 150.0f, 250.0f },  800.0f, 4000.0f, 0.5f },
       { "IceCrack",      { 50.0f, 200.0f, 500.0f },    { 2000.0f, 5000.0f, 10000.0f },
                          { 100.0f, 300.0f, 600.0f }, 50.0f, 10000.0f, 0.7f } }},
    // MEDITERRANEAN
    {{ { "MedGull",       { 2200.0f, 3800.0f, 6000.0f }, { 3200.0f, 5000.0f, 7500.0f },
                          { 280.0f, 380.0f, 480.0f }, 200.0f, 1500.0f, 0.4f },
       { "DolphinClick",  { 1000.0f, 4000.0f, 8000.0f }, { 1200.0f, 4500.0f, 9000.0f },
                          { 200.0f, 400.0f, 600.0f }, 15.0f, 300.0f, 0.3f },
       { "CicadaDrone",   { 4000.0f, 6000.0f, 8500.0f }, { 4000.0f, 6000.0f, 8500.0f },
                          { 500.0f, 700.0f, 900.0f }, 10000.0f, 2000.0f, 0.25f } }},
    // PACIFIC
    {{ { "Albatross",     { 1800.0f, 3200.0f, 5500.0f }, { 2200.0f, 3800.0f, 6500.0f },
                          { 250.0f, 350.0f, 450.0f }, 300.0f, 5000.0f, 0.3f },
       { "PacificWhale",  { 60.0f, 180.0f, 450.0f },    { 300.0f, 700.0f, 1500.0f },
                          { 25.0f, 50.0f, 90.0f },   5000.0f, 12000.0f, 0.65f },
       { "ReefCrackle",   { 800.0f, 3000.0f, 7000.0f }, { 1200.0f, 4000.0f, 9000.0f },
                          { 400.0f, 600.0f, 800.0f }, 30.0f, 100.0f, 0.2f } }},
    // SOUTHERN
    {{ { "Tropicbird",    { 2800.0f, 4500.0f, 7000.0f }, { 2000.0f, 3200.0f, 5000.0f },
                          { 320.0f, 420.0f, 520.0f }, 150.0f, 2500.0f, 0.35f },
       { "SouthernWhale", { 50.0f, 150.0f, 400.0f },    { 100.0f, 300.0f, 800.0f },
                          { 20.0f, 45.0f, 80.0f },   4000.0f, 10000.0f, 0.7f },
       { "TropicalRain",  { 2000.0f, 5000.0f, 9000.0f }, { 2000.0f, 5000.0f, 9000.0f },
                          { 800.0f, 1200.0f, 1500.0f }, 30000.0f, 5000.0f, 0.3f } }}
};

//==============================================================================
// Fluid character — how the ocean behaves at each shore.
//==============================================================================
struct FluidCharacter
{
    float swellPeriodBase;    // seconds — base swell cycle length
    float swellDepth;         // 0-1 how pronounced the swell is
    float chopFreqBase;       // Hz — surface chop frequency
    float chopAmount;         // 0-1 how much chop is added at high sea state
    float depthBias;          // 0-1 bias toward subsurface energy
    float turbulenceOnset;    // 0-1 sea state value where turbulence kicks in
};

static constexpr FluidCharacter kFluidCharacter[5] = {
    // ATLANTIC — long rolling powerful swells
    { 12.0f, 0.8f, 0.4f, 0.6f, 0.5f, 0.35f },
    // NORDIC — deep slow heavy waves
    { 18.0f, 0.9f, 0.2f, 0.4f, 0.7f, 0.4f },
    // MEDITERRANEAN — short choppy bright
    { 5.0f,  0.5f, 1.2f, 0.9f, 0.3f, 0.25f },
    // PACIFIC — vast gentle immense
    { 25.0f, 0.6f, 0.15f, 0.3f, 0.6f, 0.5f },
    // SOUTHERN — warm rolling rhythm
    { 8.0f,  0.7f, 0.6f, 0.7f, 0.4f, 0.3f }
};

//==============================================================================
// Tavern character — room acoustics for each shore's gathering place.
// Used by OSTERIA engine.
//==============================================================================
struct TavernCharacter
{
    const char* name;
    float roomSizeMs;         // early reflection delay
    float decayMs;            // RT60
    float absorption;         // 0-1 high-frequency absorption (wood=high, tile=low)
    float warmth;             // 0-1 low-frequency boost
    float murmurBrightness;   // 0-1 conversation texture brightness
    float density;            // 0-1 reflection density (stone=high, paper=low)
};

static constexpr TavernCharacter kTavernCharacter[5] = {
    // ATLANTIC — stone walls, low ceiling, wood bar, fireplace
    { "FadoHouse",       15.0f, 800.0f,  0.6f, 0.8f, 0.4f, 0.85f },
    // NORDIC — deep timber paneling, heavy insulation
    { "Sjohus",          20.0f, 600.0f,  0.8f, 0.9f, 0.3f, 0.7f },
    // MEDITERRANEAN — open-air terrace, tile floor
    { "RembetikaDen",    10.0f, 400.0f,  0.3f, 0.5f, 0.7f, 0.5f },
    // PACIFIC — paper screens, tatami, garden
    { "HarborIzakaya",   12.0f, 350.0f,  0.85f, 0.6f, 0.35f, 0.4f },
    // SOUTHERN — corrugated roof, open sides
    { "MornaBar",        8.0f,  300.0f,  0.4f, 0.7f, 0.6f, 0.55f }
};

//==============================================================================
// Rhythm patterns per shore — percussive pulse character.
// Used by OSTERIA's rhythm voice.
//==============================================================================
struct ShoreRhythm
{
    const char* name;
    float pulseRate;       // Hz — base pulse frequency
    float swing;           // 0-1 — rhythmic swing amount
    float accentPattern;   // encoded accent: 0=even, 0.33=waltz, 0.5=4/4, 0.67=6/8
};

static constexpr ShoreRhythm kShoreRhythm[5] = {
    { "Bodhran",     2.5f, 0.15f, 0.67f },  // Atlantic: 6/8 feel
    { "SamiDrum",    1.8f, 0.05f, 0.33f },  // Nordic: waltz/3-feel
    { "Darbuka",     3.5f, 0.3f,  0.5f },   // Med: 4/4 with heavy swing
    { "Taiko",       1.2f, 0.0f,  0.5f },   // Pacific: slow, even
    { "Djembe",      3.0f, 0.25f, 0.67f }   // Southern: 6/8 with swing
};

//==============================================================================
// Shore morphing utilities
//==============================================================================

/// Decompose a continuous shore value (0.0–4.0) into two adjacent shore
/// indices and an interpolation fraction.
struct ShoreMorphState
{
    int shoreA;     // lower shore index (0-4)
    int shoreB;     // upper shore index (0-4)
    float frac;     // interpolation fraction (0=pure A, 1=pure B)
};

inline ShoreMorphState decomposeShore (float shoreValue) noexcept
{
    float clamped = clamp (shoreValue, 0.0f, 4.0f);
    int lower = static_cast<int> (clamped);
    if (lower >= 4) lower = 3;
    int upper = lower + 1;
    if (upper > 4) upper = 4;
    float frac = clamped - static_cast<float> (lower);
    return { lower, upper, frac };
}

/// Interpolate a ResonatorProfile between two shores for a given instrument slot.
inline ResonatorProfile morphResonator (const ShoreMorphState& m, int slot) noexcept
{
    const auto& a = kShoreResonators[m.shoreA].instruments[slot];
    const auto& b = kShoreResonators[m.shoreB].instruments[slot];
    ResonatorProfile result;
    result.name = (m.frac < 0.5f) ? a.name : b.name;
    for (int i = 0; i < 4; ++i)
    {
        result.formantFreqs[i]      = lerp (a.formantFreqs[i],      b.formantFreqs[i],      m.frac);
        result.formantBandwidths[i] = lerp (a.formantBandwidths[i], b.formantBandwidths[i], m.frac);
        result.formantGains[i]      = lerp (a.formantGains[i],      b.formantGains[i],      m.frac);
    }
    result.attackMs      = lerp (a.attackMs,      b.attackMs,      m.frac);
    result.decayMs       = lerp (a.decayMs,       b.decayMs,       m.frac);
    result.brightness    = lerp (a.brightness,    b.brightness,    m.frac);
    result.inharmonicity = lerp (a.inharmonicity, b.inharmonicity, m.frac);
    return result;
}

/// Interpolate a CreatureVoice between two shores for a given creature slot.
inline CreatureVoice morphCreature (const ShoreMorphState& m, int slot) noexcept
{
    const auto& a = kShoreCreatures[m.shoreA].creatures[slot];
    const auto& b = kShoreCreatures[m.shoreB].creatures[slot];
    CreatureVoice result;
    result.name = (m.frac < 0.5f) ? a.name : b.name;
    for (int i = 0; i < 3; ++i)
    {
        result.startFreqs[i] = lerp (a.startFreqs[i], b.startFreqs[i], m.frac);
        result.endFreqs[i]   = lerp (a.endFreqs[i],   b.endFreqs[i],   m.frac);
        result.bandwidths[i] = lerp (a.bandwidths[i], b.bandwidths[i], m.frac);
    }
    result.sweepMs   = lerp (a.sweepMs,   b.sweepMs,   m.frac);
    result.gapMs     = lerp (a.gapMs,     b.gapMs,     m.frac);
    result.amplitude = lerp (a.amplitude, b.amplitude, m.frac);
    return result;
}

/// Interpolate fluid character between two shores.
inline FluidCharacter morphFluid (const ShoreMorphState& m) noexcept
{
    const auto& a = kFluidCharacter[m.shoreA];
    const auto& b = kFluidCharacter[m.shoreB];
    return {
        lerp (a.swellPeriodBase, b.swellPeriodBase, m.frac),
        lerp (a.swellDepth,      b.swellDepth,      m.frac),
        lerp (a.chopFreqBase,    b.chopFreqBase,    m.frac),
        lerp (a.chopAmount,      b.chopAmount,      m.frac),
        lerp (a.depthBias,       b.depthBias,       m.frac),
        lerp (a.turbulenceOnset, b.turbulenceOnset, m.frac)
    };
}

/// Interpolate tavern character between two shores.
inline TavernCharacter morphTavern (const ShoreMorphState& m) noexcept
{
    const auto& a = kTavernCharacter[m.shoreA];
    const auto& b = kTavernCharacter[m.shoreB];
    return {
        (m.frac < 0.5f) ? a.name : b.name,
        lerp (a.roomSizeMs,       b.roomSizeMs,       m.frac),
        lerp (a.decayMs,          b.decayMs,          m.frac),
        lerp (a.absorption,       b.absorption,       m.frac),
        lerp (a.warmth,           b.warmth,           m.frac),
        lerp (a.murmurBrightness, b.murmurBrightness, m.frac),
        lerp (a.density,          b.density,          m.frac)
    };
}

/// Interpolate shore rhythm between two shores.
inline ShoreRhythm morphRhythm (const ShoreMorphState& m) noexcept
{
    const auto& a = kShoreRhythm[m.shoreA];
    const auto& b = kShoreRhythm[m.shoreB];
    return {
        (m.frac < 0.5f) ? a.name : b.name,
        lerp (a.pulseRate,      b.pulseRate,      m.frac),
        lerp (a.swing,          b.swing,          m.frac),
        lerp (a.accentPattern,  b.accentPattern,  m.frac)
    };
}

} // namespace xoceanus
