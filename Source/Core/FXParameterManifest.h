// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// FXParameterManifest.h — Central mapping from EpicChainSlotController ChainIDs to
// their APVTS display parameters.
//
// Design decisions for Session 1G:
//   D1.c: 5-6 primary inline params + ADV button for full set
//   D2.c: One expanded slot at a time (accordion)
//   D3.a: Central manifest (this file) as single source of truth
//   D4.b: Bounded 120px expansion per expanded slot
//   D5.a: No parameter row when slot is empty / Off
//
// IMPORTANT — Parameter ID architecture note:
//   Chains in Group A (Aquatic, Math, Boutique, Onslaught, Obscura, Oratory)
//   use FLAT parameter IDs with no slot prefix (e.g. "aqua_drift_depth").
//   All chain parameters are shared across all 3 slot instances by design.
//   (See EpicChainSlotController.h comment at cacheParameterPointers, line 818:
//   "all slot instances share the same fixed IDs.")
//
//   Chains in Group B (Onrush through Oligo) also use FLAT IDs because
//   EpicChainSlotController::addParameters() calls each chain with slotPrefix="".
//   (The design comment at line 58 mentioning "slot2_onr_swellThresh" is aspirational
//   documentation, not current implementation.)
//
//   The fullParamId field is therefore the COMPLETE APVTS parameter ID — no further
//   prefix construction needed. Use apvts.getParameter(param.fullParamId) directly.
//
// Verification: every fullParamId here was confirmed against actual addParameters()
// registrations in the respective chain headers. See Phase 1 stop-gate log.

#include <array>

namespace xoceanus::fx_manifest {

//==============================================================================
// DisplayParam — one knob in the per-FX expanded row
//==============================================================================
struct DisplayParam
{
    const char* labelShort;   // Short knob label shown in UI (≤ 6 chars preferred)
    const char* fullParamId;  // Complete APVTS parameter ID (no slot prefix needed)
    float       defaultNorm;  // Approximate normalised default (informational)
};

//==============================================================================
// ChainSchema — per-FX display schema
//==============================================================================
struct ChainSchema
{
    int          chainId;       // Matches EpicChainSlotController::ChainID enum
    const char*  chainName;     // Display name ("Aquatic")
    int          primaryCount;  // Number of entries in primary[] (0-6)
    bool         hasAdvanced;   // True if chain has more params beyond primary[]
    DisplayParam primary[6];    // Up to 6 display knobs (pad with {} if fewer)
};

//==============================================================================
// kChainSchemas — 34 entries (0=Off, 1=Aquatic … 33=Oligo)
//
// Parameter IDs verified against actual addParameters() registrations.
// Group A (chains 1-6): flat legacy IDs, shared across all 3 slots.
// Group B (chains 7-33): flat IDs (slotPrefix="" in addParameters call).
//==============================================================================
// clang-format off
inline constexpr std::array<ChainSchema, 34> kChainSchemas = {

    // 0: Off — no params
    ChainSchema{ 0, "Off", 0, false, {} },

    // ====== GROUP A — Flat legacy IDs ======

    // 1: Aquatic — Complex multi-stage suite (Fathom/Drift/Tide/Reef/Surface).
    //    Showing 6 most prominent cross-stage params.
    ChainSchema{ 1, "Aquatic", 6, true, {
        { "DEPTH",    "aqua_drift_depth",     0.5f },
        { "RATE",     "aqua_drift_rate",      0.3f },
        { "DIFFUSE",  "aqua_drift_diffusion", 0.4f },
        { "FEEDBACK", "aqua_drift_feedback",  0.0f },
        { "SIZE",     "aqua_reef_size",       0.5f },
        { "DECAY",    "aqua_reef_decay",      0.4f },
    }},

    // 2: Math — 4 sub-engines (Entropy Cooler / Voronoi Shatter / Quantum Smear / Attractor Drive).
    //    Showing primary control from each sub-engine.
    ChainSchema{ 2, "Math", 6, true, {
        { "STABILITY","mfx_ecStability",     0.5f },
        { "EC MIX",   "mfx_ecMix",          0.0f },
        { "CRYSTALLZ","mfx_vsCrystallize",  0.0f },
        { "OBSERV",   "mfx_qsObservation",  0.3f },
        { "BIFURC",   "mfx_adBifurcation",  0.3f },
        { "AD MIX",   "mfx_adMix",          0.0f },
    }},

    // 3: Boutique — 4 sub-engines (Anomaly / Dissolving Archive / Artifact Cathedral / Spectral Morph).
    ChainSchema{ 3, "Boutique", 6, true, {
        { "TEXTURE",  "bfx_anTextureBlend", 0.5f },
        { "VERB SZ",  "bfx_anReverbSize",   0.5f },
        { "AN MIX",   "bfx_anMix",          0.0f },
        { "DISSOLVE", "bfx_daDissolve",     0.0f },
        { "DA MIX",   "bfx_daMix",          0.0f },
        { "SM MIX",   "bfx_smMix",          0.0f },
    }},

    // 4: Onslaught — sidechain gated drive/modulation beast
    ChainSchema{ 4, "Onslaught", 6, true, {
        { "FLOW RT",  "master_onslFlowRate",  0.5f },
        { "FLOW DP",  "master_onslFlowDepth", 0.5f },
        { "THRESH",   "master_onslThreshold", 0.5f },
        { "MOD DP",   "master_onslModDepth",  0.5f },
        { "MOD RT",   "master_onslModRate",   0.5f },
        { "MIX",      "master_onslMix",       0.0f },
    }},

    // 5: Obscura — noisy freeze/erosion processor
    ChainSchema{ 5, "Obscura", 6, true, {
        { "THRESH",   "master_obsThreshold",  0.3f },
        { "EROSION",  "master_obsErosion",    0.5f },
        { "SUB HRM",  "master_obsSubHarm",    0.5f },
        { "SAT",      "master_obsSaturation", 0.4f },
        { "DECIM",    "master_obsDecimate",   0.3f },
        { "MIX",      "master_obsMix",        0.0f },
    }},

    // 6: Oratory — rhythmic stutter / vowel processor
    ChainSchema{ 6, "Oratory", 6, true, {
        { "PATTERN",  "master_oraPattern",   0.75f },
        { "SYLLABLE", "master_oraSyllable",  0.3f },
        { "ACCENT",   "master_oraAccent",    0.7f },
        { "SPREAD",   "master_oraSpread",    0.6f },
        { "FEEDBACK", "master_oraFeedback",  0.4f },
        { "MIX",      "master_oraMix",       0.0f },
    }},

    // ====== GROUP B — Flat IDs (slotPrefix="" at addParameters call time) ======

    // 7: Onrush — swell/ring-mod/delay beast (17 params, onr_ prefix)
    ChainSchema{ 7, "Onrush", 6, true, {
        { "SWELL",    "onr_swellThresh",  0.3f },
        { "RING FQ",  "onr_ringFreq",     0.5f },
        { "RING MIX", "onr_ringMix",      0.0f },
        { "DIST",     "onr_distDrive",    0.0f },
        { "ENV DP",   "onr_envDepth",     0.5f },
        { "DLY MIX",  "onr_delayMix",     0.4f },
    }},

    // 8: Omnistereo — tape/chorus/vibrato widener (16 params, omni_ prefix)
    ChainSchema{ 8, "Omnistereo", 6, true, {
        { "TAPE DRV", "omni_tapeDrive",  0.5f },
        { "TAPE WOW", "omni_tapeWow",    0.0f },
        { "VIB RATE", "omni_vibRate",    0.5f },
        { "VIB DP",   "omni_vibDepth",   0.5f },
        { "CHO RATE", "omni_choRate",    0.5f },
        { "CHO DP",   "omni_choDepth",   0.5f },
    }},

    // 9: Obliterate — shimmer/fuzz/granular reverb (19 params, oblt_ prefix)
    ChainSchema{ 9, "Obliterate", 6, true, {
        { "SHIM INT", "oblt_shimInterval", 0.5f },
        { "SHIM DCY", "oblt_shimDecay",    0.5f },
        { "SHIM MIX", "oblt_shimMix",      0.5f },
        { "FUZZ",     "oblt_fuzzGain",     0.0f },
        { "GRAN DN",  "oblt_granDensity",  0.5f },
        { "GRAN SZ",  "oblt_granSize",     0.5f },
    }},

    // 10: Obscurity — PLL sub-synth + overdrive dimmer (20 params, obsc_ prefix)
    ChainSchema{ 10, "Obscurity", 6, true, {
        { "PLL GLD",  "obsc_pllGlide",    0.5f },
        { "PLL SUB1", "obsc_pllSub1",     0.5f },
        { "PLL SQ",   "obsc_pllSquare",   0.5f },
        { "OD DRIVE", "obsc_odDrive",     0.5f },
        { "OD FAT",   "obsc_odFatFreq",   0.5f },
        { "OD LEVEL", "obsc_odLevel",     0.5f },
    }},

    // 11: Oubliette — scan/PLL/slice tape effect (12 params, oubl_ prefix)
    ChainSchema{ 11, "Oubliette", 6, true, {
        { "SCAN RT",  "oubl_scanRate",    0.5f },
        { "SCAN SP",  "oubl_scanSpread",  0.5f },
        { "SCAN MX",  "oubl_scanMix",     0.5f },
        { "PLL SUB",  "oubl_pllSub",      0.5f },
        { "SLICE",    "oubl_slicePattern",0.5f },
        { "SPX WD",   "oubl_spxWidth",    0.5f },
    }},

    // 12: Osmium — sub/tape/VHS saturator (12 params, osmi_ prefix)
    ChainSchema{ 12, "Osmium", 6, true, {
        { "30HZ",     "osmi_meat30hz",   0.5f },
        { "60HZ",     "osmi_meat60hz",   0.5f },
        { "SDD DRV",  "osmi_sddDrive",   0.5f },
        { "TAPE SAT", "osmi_tapeSat",    0.5f },
        { "VHS WOW",  "osmi_vhsWow",     0.0f },
        { "VHS NZ",   "osmi_vhsNoise",   0.0f },
    }},

    // 13: Orogen — ring-mod/granular/resonator (12 params, orog_ prefix)
    ChainSchema{ 13, "Orogen", 6, true, {
        { "RING FQ",  "orog_ringFreq",   0.5f },
        { "RING FZ",  "orog_ringFuzz",   0.0f },
        { "RING MX",  "orog_ringMix",    0.5f },
        { "VERB SZ",  "orog_verbSize",   0.5f },
        { "GRAIN SZ", "orog_grainSize",  0.5f },
        { "DENSITY",  "orog_density",    0.5f },
    }},

    // 14: Oculus — plasma/seek/biphase/poly (13 params, ocul_ prefix)
    ChainSchema{ 14, "Oculus", 6, true, {
        { "PLASMA",   "ocul_plasmaVoltage", 0.7f },
        { "BLEND",    "ocul_plasmaBlend",   0.5f },
        { "SEEK SP",  "ocul_seekSpeed",     0.5f },
        { "BI RATE",  "ocul_biphaseRateA",  0.5f },
        { "BI DPTH",  "ocul_biphaseDepth",  0.5f },
        { "POLY TM",  "ocul_polyTime",      0.5f },
    }},

    // 15: Outage — LF7/k-field/LPG/CT5 combo (17 params, outg_ prefix)
    ChainSchema{ 15, "Outage", 6, true, {
        { "LF7 DRV",  "outg_drive",      0.5f },
        { "LO CUT",   "outg_lowCut",     0.2f },
        { "HI CUT",   "outg_highCut",    0.8f },
        { "K-FIELD",  "outg_kFieldDepth",0.5f },
        { "LPG SNS",  "outg_lpgSens",    0.5f },
        { "CT5 MIX",  "outg_ct5Mix",     0.5f },
    }},

    // 16: Override — Enzo/Buzz/Swarm/PM7/Tensor (14 params, ovrd_ prefix)
    ChainSchema{ 16, "Override", 6, true, {
        { "ENZO FE",  "ovrd_enzoFilterEnv",0.5f },
        { "ENZO MX",  "ovrd_enzoMix",      0.5f },
        { "BUZZ OCT", "ovrd_buzzOctave",   0.5f },
        { "SWARM",    "ovrd_swarmInterval",0.5f },
        { "PM7 SP",   "ovrd_pm7Speed",     0.5f },
        { "PM7 DP",   "ovrd_pm7Depth",     0.5f },
    }},

    // 17: Occlusion — XP300/FuzzWar/PolyEcho/Bitrman/MOOD (14 params, occl_ prefix)
    ChainSchema{ 17, "Occlusion", 6, true, {
        { "XP MIX",   "occl_xpReverseMix", 0.5f },
        { "FW FUZZ",  "occl_fuzzWarFuzz",  0.5f },
        { "FW TONE",  "occl_fuzzWarTone",  0.5f },
        { "POLY TL",  "occl_polyTimeL",    0.5f },
        { "POLY FB",  "occl_polyFeedback", 0.4f },
        { "BITR",     "occl_bitrDecimate", 0.0f },
    }},

    // 18: Obdurate — UF-01/Empress/BackTalk/Megabyte/Avalanche (14 params, obdr_ prefix)
    ChainSchema{ 18, "Obdurate", 6, true, {
        { "UF GAIN",  "obdr_ufGain",        0.5f },
        { "UF GATE",  "obdr_ufGate",        0.5f },
        { "EMP RT",   "obdr_empRate",        0.5f },
        { "EMP DP",   "obdr_empDepth",       0.5f },
        { "MEGA TM",  "obdr_megaTime",       0.5f },
        { "AVLN DY",  "obdr_avalancheDecay", 0.5f },
    }},

    // 19: Orison — verb/fuzz/Biscuit/Space delay (10 params, oris_ prefix)
    ChainSchema{ 19, "Orison", 6, true, {
        { "VERB DCY", "oris_verbDecay",     0.5f },
        { "VERB PRE", "oris_verbPredelay",  0.2f },
        { "FUZZ GT",  "oris_fuzzGate",      0.5f },
        { "BISCUIT",  "oris_biscuitBits",   0.75f },
        { "SPC TM",   "oris_spaceTime",     0.4f },
        { "SPC FB",   "oris_spaceFeedback", 0.4f },
    }},

    // 20: Overshoot — delay/synth-track/flange/granular (10 params, ovsh_ prefix)
    ChainSchema{ 20, "Overshoot", 6, true, {
        { "DLY TM",   "ovsh_delayTime",          0.5f },
        { "DLY FB",   "ovsh_delayFdbk",           0.4f },
        { "SYNTH",    "ovsh_synthTrackingError",  0.5f },
        { "FLNG RT",  "ovsh_flangeRate",          0.3f },
        { "FLNG DP",  "ovsh_flangeDepth",         0.5f },
        { "GRAN MX",  "ovsh_granMix",             0.4f },
    }},

    // 21: Obverse — compressor/reverb-swell/Mutron/trem (9 params, obvr_ prefix)
    ChainSchema{ 21, "Obverse", 5, true, {
        { "COMP",     "obvr_compSquash",   0.7f },
        { "MAKEUP",   "obvr_compMakeup",   0.5f },
        { "REV MX",   "obvr_revMix",       0.7f },
        { "MUTRON",   "obvr_mutronSens",   0.5f },
        { "TREM DP",  "obvr_tremDepth",    0.5f },
        {},
    }},

    // 22: Oxymoron — ring-mod/rotary/gate/shimmer (9 params, oxym_ prefix)
    ChainSchema{ 22, "Oxymoron", 6, true, {
        { "RING FQ",  "oxym_ringFreq",     0.5f },
        { "RING MX",  "oxym_ringMix",      0.7f },
        { "ROTARY",   "oxym_rotaryDepth",  0.7f },
        { "GATE TH",  "oxym_gateThresh",   0.3f },
        { "SHIM DY",  "oxym_shimmerDecay", 0.5f },
        { "SHIM MX",  "oxym_shimmerMix",   0.6f },
    }},

    // 23: Ornate — exciter/compressor/granular/phaser/drum-echo (11 params, orna_ prefix)
    ChainSchema{ 23, "Ornate", 6, true, {
        { "EXCIT TP", "orna_exciterTop",   0.5f },
        { "GRAIN SZ", "orna_grainSize",    0.5f },
        { "GRAIN DN", "orna_grainDensity", 0.5f },
        { "PHASE R1", "orna_phaseRate1",   0.5f },
        { "PHASE R2", "orna_phaseRate2",   0.5f },
        { "DRUM WR",  "orna_drumWear",     0.3f },
    }},

    // 24: Oration — Hedra pitch/Warp/FX25/reverb (10 params, orat_ prefix)
    ChainSchema{ 24, "Oration", 6, true, {
        { "PITCH 1",  "orat_hedraPitch1", 0.5f },
        { "PITCH 2",  "orat_hedraPitch2", 0.5f },
        { "PITCH 3",  "orat_hedraPitch3", 0.5f },
        { "WARP RT",  "orat_warpRate",    0.5f },
        { "FX25 SN",  "orat_fx25Sens",    0.5f },
        { "VERB MX",  "orat_verbMix",     0.5f },
    }},

    // 25: Offcut — Whammy/CT5/Cali/Tera/Merc (8 params, offc_ prefix)
    ChainSchema{ 25, "Offcut", 5, true, {
        { "WHAMMY",   "offc_whammyInterval", 0.5f },
        { "CT5 SP",   "offc_ct5Speed",       0.5f },
        { "CALI",     "offc_caliSquash",     0.5f },
        { "TERA RES", "offc_teraResonance",  0.5f },
        { "MERC DY",  "offc_mercDecay",      0.5f },
        {},
    }},

    // 26: Omen — split/spring/PLL/CF7/Deco (10 params, omen_ prefix)
    ChainSchema{ 26, "Omen", 6, true, {
        { "SPLIT",    "omen_splitBlend",   0.5f },
        { "SPRING",   "omen_springDwell",  0.5f },
        { "SPR TONE", "omen_springTone",   0.5f },
        { "PLL GLD",  "omen_pllGlide",     0.5f },
        { "CF7 RT",   "omen_cf7Rate",      0.5f },
        { "DECO SAT", "omen_decoSaturation",0.5f },
    }},

    // 27: Opus — VB2/micro/biphase/Habit/Merc (11 params, opus_ prefix)
    ChainSchema{ 27, "Opus", 6, true, {
        { "VB2 RT",   "opus_vb2Rate",      0.5f },
        { "VB2 DP",   "opus_vb2Depth",     0.5f },
        { "MICRO",    "opus_microActivity",0.3f },
        { "BI RT",    "opus_biphaseRate",   0.5f },
        { "BI DP",    "opus_biphaseDepth",  0.5f },
        { "HABIT",    "opus_habitScanRate", 0.2f },
    }},

    // 28: Outlaw — PLL/T-Wah/Plasma/pan/drum (12 params, outl_ prefix)
    ChainSchema{ 28, "Outlaw", 6, true, {
        { "PLL GLD",  "outl_pllGlide",     0.5f },
        { "PLL SUB",  "outl_pllSubVol",    0.5f },
        { "T-WAH",    "outl_twahSens",     0.6f },
        { "PLASMA",   "outl_plasmaVoltage",0.7f },
        { "PAN RT",   "outl_panRate",      0.5f },
        { "DRUM FB",  "outl_drumFeedback", 0.4f },
    }},

    // 29: Outbreak — GenLoss/FZ2/Bitrman/Rooms reverb (11 params, outb_ prefix)
    ChainSchema{ 29, "Outbreak", 6, true, {
        { "WOW",      "outb_genlossWow",    0.4f },
        { "FLUTTER",  "outb_genlossFlutter",0.3f },
        { "FZ2 GAIN", "outb_fz2Gain",       0.7f },
        { "BITR DEC", "outb_bitrDecimate",  0.0f },
        { "ROOMS DY", "outb_roomsDecay",    0.5f },
        { "ROOMS MX", "outb_roomsMix",      0.4f },
    }},

    // 30: Orrery — Freeze/XP-reverse/Dim/PolyEcho/NightSky (10 params, orry_ prefix)
    ChainSchema{ 30, "Orrery", 6, true, {
        { "FREEZE",   "orry_freezeDecay",   0.7f },
        { "XP MIX",   "orry_xpRevMix",      0.4f },
        { "XP SWELL", "orry_xpSwellTime",   0.5f },
        { "POLY TM",  "orry_polyTime",      0.5f },
        { "NSKY DY",  "orry_nightskyDecay", 0.5f },
        { "NSKY FLT", "orry_nightskyFilter",0.5f },
    }},

    // 31: Otrium — sidechain pump / DNA-coupled ducking (13 params, otrm_ prefix)
    ChainSchema{ 31, "Otrium", 6, true, {
        { "PUMP DP",  "otrm_pumpDepth",   0.5f },
        { "PUMP RT",  "otrm_pumpRate",    0.5f },
        { "ATTACK",   "otrm_attack",      0.3f },
        { "RELEASE",  "otrm_release",     0.4f },
        { "PHASE",    "otrm_phaseSkew",   0.0f },
        { "MIX",      "otrm_mix",         0.7f },
    }},

    // 32: Oblate — multiband spectral compressor (12 params, obla_ prefix)
    ChainSchema{ 32, "Oblate", 6, true, {
        { "THRESH",   "obla_threshold",   0.5f },
        { "RATIO",    "obla_ratio",       0.5f },
        { "ATTACK",   "obla_attack",      0.3f },
        { "RELEASE",  "obla_release",     0.4f },
        { "TILT",     "obla_tilt",        0.5f },
        { "MIX",      "obla_mix",         0.7f },
    }},

    // 33: Oligo — 4-band DNA-linked expander (13 params, olig_ prefix)
    ChainSchema{ 33, "Oligo", 6, true, {
        { "LO DEPTH", "olig_lowDepth",    0.5f },
        { "LM DEPTH", "olig_loMidDepth",  0.5f },
        { "HM DEPTH", "olig_hiMidDepth",  0.5f },
        { "HI DEPTH", "olig_highDepth",   0.5f },
        { "ATTACK",   "olig_attack",      0.3f },
        { "RELEASE",  "olig_release",     0.4f },
    }},

};
// clang-format on

//==============================================================================
// schemaForChain — safe lookup, returns Off schema if out of range
//==============================================================================
inline const ChainSchema& schemaForChain(int chainId) noexcept
{
    if (chainId >= 0 && chainId < static_cast<int>(kChainSchemas.size()))
        return kChainSchemas[static_cast<std::size_t>(chainId)];
    return kChainSchemas[0]; // Off
}

} // namespace xoceanus::fx_manifest
