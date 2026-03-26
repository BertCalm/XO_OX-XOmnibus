#pragma once
#include "RebirthDSP.h"
#include "SampleCategory.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace xolokun {

//==============================================================================
// RebirthProfileID — The 5 engine-inspired transformation profiles.
//
// Each profile encodes the sonic character of a XOlokun engine as a
// standalone DSP chain that transforms producer samples. The sample is always
// the primary audio source — the engine DSP acts as a transfer function.
//
// Architecture: profiles are pure data structures. The pipeline (RebirthPipeline.h)
// iterates the chain generically — there are NO if (profile == X) branches there.
//==============================================================================
enum class RebirthProfileID
{
    OBRIX,      ///< Harmonic Character — reef-modal saturation + comb + reverb
    ONSET,      ///< Percussive Crunch  — transient shaping + noise burst + filter
    OWARE,      ///< Resonant Body      — Akan-interval comb bank + sympathetic resonance
    OPERA,      ///< Harmonic Shimmer   — FFT formant estimation + resonator bank
    OVERWASH    ///< Deep Diffusion     — allpass cascade + spectral blur + LFO
};

//==============================================================================
// RebirthDSPModuleID — All DSP modules the pipeline can instantiate.
// Comments indicate the implementing header.
//==============================================================================
enum class RebirthDSPModuleID
{
    Saturator,          ///< Source/DSP/Effects/Saturator.h
    Combulator,         ///< Source/DSP/Effects/Combulator.h
    TransientDesigner,  ///< Source/DSP/Effects/TransientDesigner.h
    BrickwallLimiter,   ///< Source/DSP/Effects/BrickwallLimiter.h (used by pipeline, not in chains)
    LushReverb,         ///< Source/DSP/Effects/LushReverb.h
    SpectralTilt,       ///< Source/DSP/Effects/SpectralTilt.h
    AllpassDiffuser,    ///< Source/Export/RebirthDSP.h
    FormantResonator,   ///< Source/Export/RebirthDSP.h
    NoiseBurst,         ///< Source/Export/RebirthDSP.h
    SoftClipGuard,      ///< Source/Export/RebirthDSP.h — inline, no module instance needed
    LFOModulator,       ///< Inline LFO applied to prior module parameters; params: rate, depth
    BiquadLPFilter,     ///< Inline 2nd-order Butterworth LP — used by ONSET and OVERWASH
    WetDryMix           ///< Final wet/dry blend using the intensity parameter
};

//==============================================================================
// DSPModuleConfig — One module entry in a profile's FX chain.
//
// 'params' holds base parameter values (applied when no velocity scaling exists
// for a given key, or as the baseline when velocity scaling is present).
//
// 'velocityScale' maps parameter keys to {v0, v1} ranges for linear
// interpolation: effectiveValue = v0 + velocityNorm * (v1 - v0).
// This is the DOC-004 compliance mechanism — each velocity layer gets
// spectrally distinct processing, not just amplitude variation.
//==============================================================================
struct DSPModuleConfig
{
    RebirthDSPModuleID moduleId;
    std::unordered_map<std::string, float>                   params;
    std::unordered_map<std::string, std::pair<float, float>> velocityScale;
};

//==============================================================================
// RebirthSettings — embedded in OutshineSettings (integrated in Task 5).
// Phase 1B: chaosAmount field is present but MUST be ignored by the pipeline
// until Phase 1C implements the CHAOS parameter generation layer.
//==============================================================================
struct RebirthSettings
{
    RebirthProfileID profileId   = RebirthProfileID::OBRIX;
    float            intensity   = 0.7f;   ///< 0.0 = dry, 1.0 = full transformation
    float            chaosAmount = 0.0f;   ///< Phase 1B: always ignored (implemented in Phase 1C)
    bool             enabled     = false;
};

//==============================================================================
// RebirthProfile — complete description of one engine-inspired transformation.
//
// Headroom budget notes (per Spec Section 5.2):
//   - After any module that adds gain (Saturator, Combulator feedback > 0.9),
//     insert a SoftClipGuard module entry in the chain.
//   - tailSeconds: extra render time after sample ends for reverb/diffusion decay.
//   - recommendedTransientMax: pipeline shows UI warning if sample transient ratio
//     exceeds this value (0.0–1.0; 1.0 = no warning).
//==============================================================================
struct RebirthProfile
{
    RebirthProfileID   id;
    const char*        engineName;              ///< "OBRIX", "ONSET", etc.
    const char*        producerLabel;           ///< Human-facing name shown in UI
    const char*        materialCategory;        ///< "TONAL", "PERCUSSIVE", "HARMONIC", "TEXTURAL"
    const char*        description;             ///< 1-line sonic description for UI
    const char*        characterBrief;          ///< Acceptance test: what should the output sound like?
    const char*        intensityDescription;    ///< Per-profile intensity slider label for UI
    float              recommendedTransientMax; ///< 0.0–1.0; warn if sample transient ratio exceeds this
    float              tailSeconds;             ///< Extra render time after sample ends (reverb/diffusion)
    std::vector<DSPModuleConfig> chain;         ///< Ordered list of DSP modules to apply
};

//==============================================================================
// getRebirthProfile() — returns the built-in profile for the given ID.
//
// All 5 profiles are defined here as static const data. The pipeline reads
// the chain vector and executes each module generically — no per-profile
// conditionals exist in RebirthPipeline.h.
//
// Velocity spectral shaping (DOC-004):
//   Each velocityScale entry defines {v0, v1} where:
//     v0 = parameter value at velocityNorm=0 (softest layer)
//     v1 = parameter value at velocityNorm=1 (hardest layer)
//   Linear interpolation: effectiveValue = v0 + velocityNorm * (v1 - v0)
//   The pipeline equalises LUFS across layers so the spectral difference
//   is what the player hears, not a volume ramp.
//==============================================================================
inline const RebirthProfile& getRebirthProfile (RebirthProfileID id)
{
    //--------------------------------------------------------------------------
    // OBRIX — "Harmonic Character"
    //
    // Sonic target: Reef-modal harmonic complexity. The sample gains overtone
    // density and comb-filtered resonance, as if resonating inside a coral reef.
    // At default intensity (0.7), the sample is clearly recognizable but richer.
    //
    // Velocity spectral scaling:
    //   Vel 0 (soft): Low drive (0.2), bright comb damping (0.5), short reverb (0.15)
    //                 → airy, fragile, transparent
    //   Vel 1 (hard): High drive (0.8), dark comb damping (0.1), longer reverb (0.35)
    //                 → dense, compressed, thick
    //
    // Headroom budget: Saturator adds gain → SoftClipGuard after step 1.
    //                  Combulator with feedback up to 0.95 is borderline →
    //                  pipeline SoftClipGuard + BrickwallLimiter handle output.
    //--------------------------------------------------------------------------
    static const RebirthProfile OBRIX_PROFILE = {
        RebirthProfileID::OBRIX,
        "OBRIX",
        "Harmonic Character",
        "TONAL",
        "Reef-modal saturation + comb resonance + ecological reverb",
        "Sample is recognizable but richer — dense overtone structure, comb shimmer, "
        "coral-reef warmth. At 0.7 intensity: clearly the original sample, harmonically alive.",
        "Controls harmonic density and comb resonance",
        0.7f,   // warn if transient ratio > 0.7 — percussive material may over-saturate
        0.5f,   // 0.5s reverb tail
        {
            // Step 1: Saturator — Tube mode (mode=0), vel-scaled drive 0.2→0.8
            {
                RebirthDSPModuleID::Saturator,
                { {"drive", 0.4f}, {"mode", 0.0f} /*Tube=0*/ },
                { {"drive", {0.2f, 0.8f}} }
            },
            // Step 2: SoftClipGuard — inter-stage limiter after Saturator gain
            // No params — pipeline applies softClip() inline to all channels
            {
                RebirthDSPModuleID::SoftClipGuard,
                {}, {}
            },
            // Step 3: Combulator — 3 comb filters, base feedback 0.85
            //         vel-scaled feedback 0.75→0.95, damping 0.5→0.1 (bright→dark)
            {
                RebirthDSPModuleID::Combulator,
                { {"feedback", 0.85f}, {"damping", 0.3f}, {"mix", 0.4f} },
                { {"feedback", {0.75f, 0.95f}}, {"damping", {0.5f, 0.1f}} }
            },
            // Step 4: LushReverb — decay 0.4s, vel-scaled mix 0.15→0.35
            {
                RebirthDSPModuleID::LushReverb,
                { {"roomSize", 0.5f}, {"damping", 0.4f}, {"mix", 0.25f} },
                { {"mix", {0.15f, 0.35f}} }
            },
            // Step 5: WetDryMix — final blend driven by RebirthSettings::intensity
            {
                RebirthDSPModuleID::WetDryMix,
                {}, {}
            }
        }
    };

    //--------------------------------------------------------------------------
    // ONSET — "Percussive Crunch"
    //
    // Sonic target: Transient emphasis with added noise texture. Attack is
    // sculpted and enhanced with a brief noise burst that adds "crack."
    // Body is filter-swept. Designed for drums and percussion one-shots.
    //
    // Velocity spectral scaling:
    //   Vel 0 (soft): Minimal boost (+2dB), quiet noise (-30dB), low sat, LP at 8kHz
    //                 → gentle, muted, controlled
    //   Vel 1 (hard): Maximum boost (+12dB), noise -18dB, high sat, LP at 18kHz (open)
    //                 → aggressive, punchy, crackling
    //
    // Headroom note: TransientDesigner adds up to +12dB — NoiseBurst adds level.
    // No SoftClipGuard between them (NoiseBurst output is low level). Saturator
    // after NoiseBurst provides additional soft saturation. BrickwallLimiter at
    // pipeline output handles final ceiling.
    //--------------------------------------------------------------------------
    static const RebirthProfile ONSET_PROFILE = {
        RebirthProfileID::ONSET,
        "ONSET",
        "Percussive Crunch",
        "PERCUSSIVE",
        "Transient emphasis + noise burst attack + filter sweep",
        "Attack is sculpted and crackles with noise — body is filter-swept and punchy. "
        "Vel 0: gentle. Vel 1: aggressive, snappy, full crack.",
        "Controls transient aggression and noise level",
        1.0f,   // no warning — designed for percussive material (all transient ratios welcome)
        0.15f,  // short tail — percussive content decays quickly
        {
            // Step 1: TransientDesigner — base attack +6dB (0.5 normalised), vel-scaled +2→+12dB
            //         sustain: -3dB constant (tighten body for punchiness)
            //         Normalised mapping: attack param 0.167 = +2dB, 1.0 = +12dB
            {
                RebirthDSPModuleID::TransientDesigner,
                { {"attack", 0.5f}, {"sustain", -0.25f} },
                { {"attack", {0.167f, 1.0f}} }
            },
            // Step 2: NoiseBurst — 5ms HP-filtered noise burst, vel-scaled level -30→-18 dBFS
            //         HP cutoff: 4kHz (adds "crack" texture above the fundamental)
            {
                RebirthDSPModuleID::NoiseBurst,
                { {"burstLengthMs", 5.0f}, {"burstLevelDb", -24.0f}, {"hpfCutoffHz", 4000.0f} },
                { {"burstLevelDb", {-30.0f, -18.0f}} }
            },
            // Step 3: Saturator — Digital mode (mode=2), vel-scaled drive 0.1→0.6
            {
                RebirthDSPModuleID::Saturator,
                { {"drive", 0.3f}, {"mode", 2.0f} /*Digital=2*/ },
                { {"drive", {0.1f, 0.6f}} }
            },
            // Step 4: BiquadLPFilter — vel-scaled cutoff 8kHz→18kHz (darker→brighter)
            //         Q = 0.7 (Butterworth — no resonant peak)
            {
                RebirthDSPModuleID::BiquadLPFilter,
                { {"cutoffHz", 13000.0f}, {"q", 0.7f} },
                { {"cutoffHz", {8000.0f, 18000.0f}} }
            },
            // Step 5: WetDryMix
            {
                RebirthDSPModuleID::WetDryMix,
                {}, {}
            }
        }
    };

    //--------------------------------------------------------------------------
    // OWARE — "Resonant Body"
    //
    // Sonic target: Sample excites a tuned resonant body based on Akan percussion
    // intervals from OwareEngine.h. Comb filters tuned to musical ratios create
    // pitched resonance from unpitched material. Sympathetic resonance adds shimmer.
    //
    // Akan interval sets (pipeline selects based on spectral centroid):
    //   Wood:  [1.0, 2.76, 5.40]  — spectral centroid < 1kHz
    //   Metal: [1.0, 2.0, 3.0]   — spectral centroid 1–4kHz
    //   Bell:  [1.0, 2.32, 4.18] — spectral centroid > 4kHz
    // (The pipeline reads the analysis.spectralCentroid and sets Combulator
    //  tuning ratios accordingly before executing the chain.)
    //
    // Velocity spectral scaling:
    //   Vel 0 (soft): Lower comb feedback (0.8), bright tilt (+3dB/oct) → clean, bright ring
    //   Vel 1 (hard): Higher comb feedback (0.98), dark tilt (-2dB/oct) → dense, dark, buzzy
    //
    // Headroom: Combulator feedback up to 0.98 is near-resonant.
    //           SoftClipGuard inserted after first Combulator.
    //--------------------------------------------------------------------------
    static const RebirthProfile OWARE_PROFILE = {
        RebirthProfileID::OWARE,
        "OWARE",
        "Resonant Body",
        "HARMONIC",
        "Akan-interval comb bank + sympathetic resonance + spectral tilt",
        "Sample excites a tuned resonant body — pitched ringing, dark shimmer. "
        "Vel 0: clean bright ring. Vel 1: dense dark buzzing resonance.",
        "Controls resonance depth and tonal darkness",
        1.0f,   // no warning — resonance works well with all material
        0.8f,   // 0.8s tail — Akan combs ring long
        {
            // Step 1: Combulator — primary Akan-interval comb bank
            //         Pipeline sets tuning from spectral centroid analysis.
            //         vel-scaled feedback 0.8→0.98 (deeper resonance at hard velocity)
            {
                RebirthDSPModuleID::Combulator,
                { {"feedback", 0.9f}, {"damping", 0.25f}, {"mix", 0.5f} },
                { {"feedback", {0.8f, 0.98f}} }
            },
            // Step 2: SoftClipGuard — protect against near-resonant comb feedback
            {
                RebirthDSPModuleID::SoftClipGuard,
                {}, {}
            },
            // Step 3: Combulator (sympathetic) — 2nd comb bank at harmonic intervals
            //         Lower feedback (0.3–0.5), lower mix (0.15) → shimmer layer
            //         Pipeline tunes this to sub-harmonics of the primary comb set
            {
                RebirthDSPModuleID::Combulator,
                { {"feedback", 0.4f}, {"damping", 0.6f}, {"mix", 0.15f} },
                { {"feedback", {0.3f, 0.5f}} }
            },
            // Step 4: SpectralTilt — vel-scaled tilt
            //         positive tilt = high-frequency boost (bright), negative = dark
            //         Vel 0: +0.6 (≈ +3dB/oct brightness)
            //         Vel 1: -0.4 (≈ -2dB/oct darkness)
            {
                RebirthDSPModuleID::SpectralTilt,
                { {"tilt", 0.0f} },
                { {"tilt", {0.6f, -0.4f}} }
            },
            // Step 5: WetDryMix
            {
                RebirthDSPModuleID::WetDryMix,
                {}, {}
            }
        }
    };

    //--------------------------------------------------------------------------
    // OPERA — "Harmonic Shimmer"
    //
    // Sonic target: Sample's formant structure is analyzed via FFT peak detection
    // and resynthesized through a parallel resonator bank inspired by OPERA's
    // Kuramoto additive architecture. Tonal material gains vocal/choral quality.
    // Non-tonal material gains harmonic focus.
    //
    // FormantResonator implementation:
    //   - 2048-point windowed FFT, top 4 spectral peaks as formant centers
    //   - Parallel bandpass biquad resonators at formant frequencies
    //   - Q: vel-scaled 4.0 (wide/warm) → 16.0 (narrow/bright)
    //   - Mix: 0.4 dry + 0.6 resonator blend
    //   NOTE: FFT peak detection, not LPC. LPC is too expensive for <3s preview.
    //
    // Slow LFO modulation (LFOModulator module):
    //   rate: 0.3 Hz, depth: ±50 Hz on resonator center frequencies
    //   Creates Kuramoto-inspired breathing shimmer motion.
    //
    // Velocity spectral scaling:
    //   Vel 0 (soft): Wide Q (4.0), no tilt, minimal LFO depth → subtle warmth
    //   Vel 1 (hard): Narrow Q (16.0), +2dB/oct tilt, more LFO depth → dramatic, vocal, shimmering
    //
    // Headroom: FormantResonator is passive (parallel sum normalized by N formants).
    //           No SoftClipGuard required. SpectralTilt may add up to +2dB/oct boost.
    //--------------------------------------------------------------------------
    static const RebirthProfile OPERA_PROFILE = {
        RebirthProfileID::OPERA,
        "OPERA",
        "Harmonic Shimmer",
        "TONAL",
        "FFT formant estimation + Kuramoto-inspired resonator bank + slow LFO shimmer",
        "Sample gains vocal/choral shimmer — tonal material sounds alive and evolving. "
        "Vel 0: subtle warmth. Vel 1: dramatic shimmering, vocal resonance.",
        "Controls resonator Q and spectral shimmer brightness",
        0.5f,   // warn if transient ratio > 0.5 — transient material gets attack blur from resonator ringing
        1.0f,   // 1.0s tail — resonators ring after sample ends
        {
            // Step 1: FormantResonator — FFT formant analysis + biquad resonator bank
            //         vel-scaled Q 4.0→16.0 (wide/warm → narrow/bright)
            //         vel-scaled mix 0.3→0.5 (subtle → prominent resonance)
            {
                RebirthDSPModuleID::FormantResonator,
                { {"q", 8.0f}, {"mix", 0.4f} },
                { {"q", {4.0f, 16.0f}}, {"mix", {0.3f, 0.5f}} }
            },
            // Step 2: SpectralTilt — vel-scaled: soft = flat (0), hard = +0.4 bright
            //         (+0.4 ≈ +2dB/oct high-frequency shimmer boost)
            {
                RebirthDSPModuleID::SpectralTilt,
                { {"tilt", 0.0f} },
                { {"tilt", {0.0f, 0.4f}} }
            },
            // Step 3: LFOModulator — slow Kuramoto-inspired shimmer
            //         rate: 0.3 Hz, depth: ±50 Hz on formant center frequencies
            //         vel-scaled depth 0.3→1.0 (subtle → dramatic modulation)
            {
                RebirthDSPModuleID::LFOModulator,
                { {"rate", 0.3f}, {"depth", 50.0f}, {"target", 0.0f} /*formant centers*/ },
                { {"depth", {15.0f, 50.0f}} }
            },
            // Step 4: WetDryMix
            {
                RebirthDSPModuleID::WetDryMix,
                {}, {}
            }
        }
    };

    //--------------------------------------------------------------------------
    // OVERWASH — "Deep Diffusion"
    //
    // Sonic target: Sample washed through multi-stage allpass diffusion network.
    // Transients dissolve. Result is ambient, evolving, textural. Like hearing
    // the sample through deep water.
    //
    // AllpassDiffuser delay times: [7.1ms, 11.3ms, 17.9ms, 23.7ms]
    // Prime-ish ratios avoid constructive resonance that would add visible
    // spectral peaks to the diffused output (per spec Section 4.5).
    //
    // Slow LFO on allpass delay times (LFOModulator):
    //   rate: 0.15 Hz, depth: ±1.5ms — creates chorus-like evolving movement.
    //
    // Velocity spectral scaling:
    //   Vel 0 (soft): Low diffusion (0.4), bright LP (12kHz), bright tilt → ethereal, open
    //   Vel 1 (hard): High diffusion (0.8), dark LP (6kHz), dark tilt → deep, submerged, oceanic
    //
    // Headroom: AllpassDiffuser is gain-neutral (allpass filters preserve energy).
    //           LP filter rolls off highs. SpectralTilt may darken further.
    //           No SoftClipGuard required mid-chain. Pipeline true-peak limiter handles output.
    //--------------------------------------------------------------------------
    static const RebirthProfile OVERWASH_PROFILE = {
        RebirthProfileID::OVERWASH,
        "OVERWASH",
        "Deep Diffusion",
        "TEXTURAL",
        "Multi-stage allpass diffusion + LP filter sweep + spectral tilt",
        "Transients dissolve into ambient wash — like hearing the sample through deep water. "
        "Vel 0: ethereal, open. Vel 1: deep, submerged, oceanic.",
        "Controls diffusion depth and submersion darkness",
        0.4f,   // warn if transient ratio > 0.4 — percussive attacks will dissolve (by design, but warn)
        2.0f,   // 2.0s tail — diffusion network rings significantly after source ends
        {
            // Step 1: AllpassDiffuser — 4-stage cascade
            //         Delay times: [7.1ms, 11.3ms, 17.9ms, 23.7ms] (prime-ish ratios)
            //         vel-scaled feedback 0.4→0.8 (light diffusion → deep wash)
            {
                RebirthDSPModuleID::AllpassDiffuser,
                { {"feedback", 0.6f},
                  {"delayMs0", 7.1f}, {"delayMs1", 11.3f},
                  {"delayMs2", 17.9f}, {"delayMs3", 23.7f} },
                { {"feedback", {0.4f, 0.8f}} }
            },
            // Step 2: BiquadLPFilter — vel-scaled cutoff 12kHz→6kHz (harder = darker)
            //         Q = 0.7 (Butterworth — smooth rolloff, no resonant peak)
            {
                RebirthDSPModuleID::BiquadLPFilter,
                { {"cutoffHz", 9000.0f}, {"q", 0.7f} },
                { {"cutoffHz", {12000.0f, 6000.0f}} }
            },
            // Step 3: LFOModulator — slow chorus-like movement on allpass delay times
            //         rate: 0.15 Hz, depth: ±1.5ms
            {
                RebirthDSPModuleID::LFOModulator,
                { {"rate", 0.15f}, {"depth", 1.5f}, {"target", 1.0f} /*allpass delays*/ },
                {}
            },
            // Step 4: SpectralTilt — vel-scaled: soft = +0.2 bright, hard = -0.6 dark
            //         (+0.2 ≈ +1dB/oct, -0.6 ≈ -3dB/oct — submerged quality at hard velocity)
            {
                RebirthDSPModuleID::SpectralTilt,
                { {"tilt", 0.0f} },
                { {"tilt", {0.2f, -0.6f}} }
            },
            // Step 5: WetDryMix
            {
                RebirthDSPModuleID::WetDryMix,
                {}, {}
            }
        }
    };

    switch (id)
    {
        case RebirthProfileID::OBRIX:    return OBRIX_PROFILE;
        case RebirthProfileID::ONSET:    return ONSET_PROFILE;
        case RebirthProfileID::OWARE:    return OWARE_PROFILE;
        case RebirthProfileID::OPERA:    return OPERA_PROFILE;
        case RebirthProfileID::OVERWASH: return OVERWASH_PROFILE;
        default:                         return OBRIX_PROFILE;
    }
}

//==============================================================================
// resolveVelocityParam() — compute the velocity-scaled value for a parameter.
//
// velocityNorm: 0.0 = softest layer, 1.0 = hardest layer.
//
// If a velocityScale entry exists for the key, the base value in params is
// ignored — the velocity range {v0, v1} is used exclusively.
// If no velocityScale entry, returns the base param value (or 0.0 if absent).
//
// This matches the DOC-004 spec formula:
//   effectiveValue = v0 + velocityNorm * (v1 - v0)
//==============================================================================
inline float resolveVelocityParam (const DSPModuleConfig& cfg,
                                   const std::string& key,
                                   float velocityNorm)
{
    auto scaleIt = cfg.velocityScale.find (key);
    if (scaleIt != cfg.velocityScale.end())
    {
        float v0 = scaleIt->second.first;
        float v1 = scaleIt->second.second;
        return v0 + velocityNorm * (v1 - v0);
    }

    auto baseIt = cfg.params.find (key);
    return (baseIt != cfg.params.end()) ? baseIt->second : 0.0f;
}

//==============================================================================
// autoProfileForCategory() — map a SampleCategory to its default Rebirth profile.
//
// Per spec Section 9.3. Producer can override per-category or per-sample.
// Unknown/FX/Loop default to OBRIX (general-purpose, works on any material).
//==============================================================================
inline RebirthProfileID autoProfileForCategory (SampleCategory cat)
{
    switch (cat)
    {
        case SampleCategory::Kick:
        case SampleCategory::Snare:
        case SampleCategory::HiHatClosed:
        case SampleCategory::HiHatOpen:
        case SampleCategory::Clap:
        case SampleCategory::Tom:
        case SampleCategory::Percussion:
            return RebirthProfileID::ONSET;

        case SampleCategory::Bass:
        case SampleCategory::Lead:
        case SampleCategory::Keys:
        case SampleCategory::Pluck:
        case SampleCategory::String:
            return RebirthProfileID::OBRIX;

        case SampleCategory::Vocal:
        case SampleCategory::Woodwind:
        case SampleCategory::Brass:
            return RebirthProfileID::OPERA;

        case SampleCategory::Pad:
            return RebirthProfileID::OVERWASH;

        case SampleCategory::FX:
        case SampleCategory::Loop:
        case SampleCategory::Unknown:
        default:
            return RebirthProfileID::OBRIX;  // general-purpose default
    }
}

} // namespace xolokun
