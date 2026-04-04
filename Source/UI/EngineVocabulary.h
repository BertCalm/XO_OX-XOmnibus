// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_core/juce_core.h>
#include <unordered_map>
#include <string>

//==============================================================================
// EngineVocabulary — per-engine parameter display label overrides.
//
// Overrides are SHORT (≤8 chars ideally, ≤9 chars max), ALL-CAPS, and
// mythologically appropriate for each engine's character identity.
//
// Falls back to `fallback` (or a cleaned-up version of paramId if fallback
// is empty: strip engine prefix, title-case last camelCase word, uppercase,
// truncate to 8 chars).
//
// DISPLAY ONLY — never stored or used in preset serialisation.
// Parameter IDs are frozen and must never be modified here.
//
// Usage:
//   juce::String label = EngineVocabulary::labelFor("Opera", "opera_drama", "DRAMA");
//==============================================================================

namespace xoceanus
{

struct EngineVocabulary
{
    // Returns a short display label for (engineId, paramId).
    // engineId  — engine's canonical ID (e.g. "Opera", "Organism")
    // paramId   — frozen APVTS parameter ID (e.g. "opera_drama")
    // fallback  — label to use when no override exists; if empty, a best-effort
    //             label is derived from paramId (strip prefix, uppercase last
    //             camelCase word, truncate to 8 chars).
    static juce::String labelFor(const juce::String& engineId, const juce::String& paramId,
                                 const juce::String& fallback = {})
    {
        // Check the override table — case-insensitive engine ID lookup.
        const auto& table = kOverrides();
        auto it = table.find(engineId.toLowerCase().toStdString());
        if (it != table.end())
        {
            auto jt = it->second.find(paramId.toStdString());
            if (jt != it->second.end())
                return juce::String(jt->second);
        }

        // No override found — use fallback or derive from paramId.
        if (fallback.isNotEmpty())
            return fallback;

        return derivedLabel(paramId);
    }

private:
    // Derive a best-effort label from a paramId when no override or fallback
    // is provided.  "opera_formantShift" → "FORMANT", "org_ruleSet" → "RULESET"
    //               "oxy_warmth_rate"   → "RATE"   (multi-underscore handled below)
    static juce::String derivedLabel(const juce::String& paramId)
    {
        // Strip prefix up to (and including) the first underscore.
        int under = paramId.indexOf("_");
        juce::String inner = (under >= 0) ? paramId.substring(under + 1) : paramId;

        // Fix: if inner still contains an underscore (multi-segment IDs like
        // "warmth_rate"), take only the last segment so callers get "RATE"
        // rather than "WARMTH_RATE" (which would be truncated awkwardly).
        int lastUnder = inner.lastIndexOfChar('_');
        if (lastUnder >= 0)
            inner = inner.substring(lastUnder + 1);

        // Extract last camelCase segment: find last uppercase transition.
        juce::String best = inner;
        for (int i = (int)inner.length() - 1; i >= 1; --i)
        {
            if (juce::CharacterFunctions::isUpperCase(inner[i]))
            {
                best = inner.substring(i);
                break;
            }
        }

        return best.toUpperCase().substring(0, 8);
    }

    // Lazy-initialised override table: engineId (lowercase) → paramId → label.
    // Add new engine blocks here as vocabulary is defined.
    static const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& kOverrides()
    {
        static const std::unordered_map<std::string, std::unordered_map<std::string, std::string>> table = {
            //------------------------------------------------------------------
            // OBIONT (prefix: obnt_)  — Bioluminescent Amber #E8A030
            // Wolfram CA oscillator (1D cellular automata spatial projection)
            //------------------------------------------------------------------
            {"obiont",
             {
                 {"obnt_mode", "CA MODE"},
                 {"obnt_rule", "RULE"},
                 {"obnt_ruleMorph", "MORPH"},
                 {"obnt_evolutionRate", "EVOLVE"},
                 {"obnt_gridDensity", "DENSITY"},
                 {"obnt_chaos", "CHAOS"},
                 {"obnt_projection", "PROJECT"},
                 {"obnt_pitchMode", "PITCH"},
                 // Macros
                 {"obnt_macroChaos", "CHAOS"},
                 {"obnt_macroEvolution", "EVOLVE"},
                 {"obnt_macroCoupling", "SYMBIOSE"},
                 {"obnt_macroSpace", "LATTICE"},
             }},

            //------------------------------------------------------------------
            // OBESE (prefix: fat_)  — Hot Pink #FF1493
            // Saturation / lo-fi bass synth (Mojo analog-digital axis)
            //------------------------------------------------------------------
            {"obese",
             {
                 {"fat_subLevel", "SUBLVL"},
                 {"fat_groupMix", "GRPMIX"},
                 {"fat_fltKeyTrack", "KEYTRCK"},
                 {"fat_fltEnvAmt", "FLTENV"},
                 {"fat_fltEnvAttack", "FENVATT"},
                 {"fat_fltEnvDecay", "FENVDCY"},
                 {"fat_satDrive", "SATDRV"},
                 {"fat_crushDepth", "CRSHBIT"},
                 {"fat_crushRate", "CRSHRT"},
                 {"fat_lfo1Rate", "LFO1 RT"},
                 {"fat_lfo1Depth", "LFO1 DP"},
                 {"fat_lfo2Rate", "LFO2 RT"},
                 {"fat_lfo2Depth", "LFO2 DP"},
                 {"fat_arpRate", "ARPRATE"},
             }},

            //------------------------------------------------------------------
            // OBELISK (prefix: obel_)  — Grand Ivory #FFFFF0
            // Prepared piano (Piano/Plank/Bell/Stone material continuum)
            //------------------------------------------------------------------
            {"obelisk",
             {
                 {"obel_density", "DENSITY"},
                 {"obel_stoneTone", "STONE"},
                 {"obel_hardness", "HARDNS"},
                 {"obel_prepType", "PREP"},
                 {"obel_prepPosition", "PREPPOS"},
                 {"obel_prepDepth", "PREPDPTH"},
                 {"obel_thermalDrift", "DRIFT"},
                 // Macros — Obelisk has custom Stone/Prep names
                 {"obel_macroStone", "STONE"},
                 {"obel_macroPrep", "PREPARE"},
                 {"obel_macroCoupling", "RESONATE"},
                 {"obel_macroSpace", "HALL"},
             }},

            //------------------------------------------------------------------
            // OBLIQUE (prefix: oblq_)  — Prism Violet #BF40FF
            // Prismatic bounce synth (RTJ × Funk × Tame Impala)
            //------------------------------------------------------------------
            {"oblique",
             {
                 {"oblq_oscFold", "FOLD"},
                 {"oblq_bounceRate", "BNCRATE"},
                 {"oblq_bounceGravity", "GRAVITY"},
                 {"oblq_bounceDamp", "BNCDAMP"},
                 {"oblq_bounceCnt", "BNCCOUNT"},
                 {"oblq_bounceSwing", "SWING"},
                 {"oblq_prismDelay", "PRSMLY"},
                 {"oblq_prismSpread", "PRSMSPRD"},
                 {"oblq_prismColor", "COLOR"},
                 {"oblq_prismWidth", "PRSMWDT"},
                 {"oblq_prismFeedback", "PRSM FB"},
                 {"oblq_prismMix", "PRSMMIX"},
                 {"oblq_prismDamp", "PRSMDAM"},
                 {"oblq_phaserFeedback", "PHSR FB"},
                 // Macros
                 {"oblq_macroFold", "FOLD"},
                 {"oblq_macroBounce", "BOUNCE"},
                 {"oblq_macroColor", "COLOR"},
                 {"oblq_macroSpace", "PRISM"},
             }},

            //------------------------------------------------------------------
            // OBSCURA (prefix: obscura_)  — Daguerreotype Silver #8A9BA8
            // Bowed/resonant physical model (misleading SUSTAIN = bow force)
            //------------------------------------------------------------------
            {"obscura",
             {
                 {"obscura_sustain", "BOWFRC"},
                 {"obscura_excitePos", "EXCPOS"},
                 {"obscura_exciteWidth", "EXCWDTH"},
                 {"obscura_scanWidth", "SCANWDT"},
                 {"obscura_physEnvAttack", "PHYSATT"},
                 {"obscura_physEnvDecay", "PHYSDCY"},
                 {"obscura_physEnvSustain", "PHYSSUS"},
                 {"obscura_physEnvRelease", "PHYSREL"},
                 {"obscura_lfo1Rate", "LFO1 RT"},
                 {"obscura_lfo1Depth", "LFO1 DP"},
                 {"obscura_lfo2Rate", "LFO2 RT"},
                 {"obscura_lfo2Depth", "LFO2 DP"},
                 {"obscura_lfo2Shape", "LFO2 SH"},
                 {"obscura_initShape", "INITSHP"},
                 // Macros
                 {"obscura_macroCharacter", "SILVER"},
                 {"obscura_macroMovement", "GRAIN"},
                 {"obscura_macroCoupling", "STIFF"},
                 {"obscura_macroSpace", "ABYSS"},
             }},

            //------------------------------------------------------------------
            // OBSIDIAN (prefix: obsidian_)  — Crystal White #E8E0D8
            // Phase-distortion synth (density × tilt waveshaper cascade)
            //------------------------------------------------------------------
            {"obsidian",
             {
                 {"obsidian_densityX", "DENSITY"},
                 {"obsidian_tiltY", "TILT"},
                 {"obsidian_depth", "PDDEPTH"},
                 {"obsidian_stiffness", "STIFF"},
                 {"obsidian_cascadeBlend", "CASCADE"},
                 {"obsidian_crossModDepth", "XMOD"},
                 {"obsidian_formantResonance", "FRMNT"},
                 {"obsidian_formantIntensity", "FRMNTI"},
                 {"obsidian_depthAttack", "PDATT"},
                 {"obsidian_depthDecay", "PDDCY"},
                 {"obsidian_depthSustain", "PDSUS"},
                 {"obsidian_depthRelease", "PDREL"},
                 {"obsidian_lfo1Rate", "LFO1 RT"},
                 {"obsidian_lfo1Depth", "LFO1 DP"},
                 {"obsidian_lfo2Rate", "LFO2 RT"},
                 {"obsidian_lfo2Depth", "LFO2 DP"},
                 // Macros
                 {"obsidian_macroCharacter", "CRYSTAL"},
                 {"obsidian_macroMovement", "SHATTER"},
                 {"obsidian_macroCoupling", "EDGE"},
                 {"obsidian_macroSpace", "VOID"},
             }},

            //------------------------------------------------------------------
            // OBRIX (prefix: obrix_)  — Reef Jade #1E8B7E
            // Modular brick synthesis (81 params — coral reef ecology)
            //------------------------------------------------------------------
            {"obrix",
             {
                 {"obrix_src1Type", "SRC1"},
                 {"obrix_src2Type", "SRC2"},
                 {"obrix_src1Tune", "S1 TUNE"},
                 {"obrix_src2Tune", "S2 TUNE"},
                 {"obrix_src1PW", "S1 PW"},
                 {"obrix_src2PW", "S2 PW"},
                 {"obrix_srcMix", "SRC MIX"},
                 {"obrix_proc1Type", "PROC1"},
                 {"obrix_proc1Cutoff", "P1 CUT"},
                 {"obrix_proc1Reso", "P1 RES"},
                 {"obrix_proc2Type", "PROC2"},
                 {"obrix_proc2Cutoff", "P2 CUT"},
                 {"obrix_proc2Reso", "P2 RES"},
                 {"obrix_proc3Type", "PROC3"},
                 {"obrix_proc3Cutoff", "P3 CUT"},
                 {"obrix_proc3Reso", "P3 RES"},
                 {"obrix_proc1Feedback", "P1 FDBK"},
                 {"obrix_proc2Feedback", "P2 FDBK"},
                 {"obrix_fmDepth", "FM DPTH"},
                 {"obrix_wtBank", "WT BANK"},
                 {"obrix_unisonDetune", "UNISON"},
                 {"obrix_driftRate", "DRFT RT"},
                 {"obrix_driftDepth", "DRFT DP"},
                 {"obrix_journeyMode", "JOURNEY"},
                 {"obrix_distance", "DIST"},
                 {"obrix_air", "AIR"},
                 {"obrix_fieldStrength", "FIELD"},
                 {"obrix_fieldPolarity", "POLRITY"},
                 {"obrix_fieldRate", "FLD RT"},
                 {"obrix_fieldPrimeLimit", "PRIME"},
                 {"obrix_envTemp", "TEMP"},
                 {"obrix_envPressure", "PRESURE"},
                 {"obrix_envCurrent", "CURRENT"},
                 {"obrix_envTurbidity", "TURBID"},
                 {"obrix_competitionStrength", "COMPET"},
                 {"obrix_symbiosisStrength", "SYMBIOSE"},
                 {"obrix_stressDecay", "STRESS"},
                 {"obrix_bleachRate", "BLEACH"},
                 {"obrix_stateReset", "RESET"},
                 {"obrix_fxMode", "FX MODE"},
                 {"obrix_reefResident", "RESDT"},
                 {"obrix_residentStrength", "RSDT ST"},
                 {"obrix_gestureType", "GESTURE"},
                 {"obrix_flashTrigger", "FLASH"},
                 // Macros
                 {"obrix_macroCharacter", "REEF"},
                 {"obrix_macroMovement", "CURRENT"},
                 {"obrix_macroCoupling", "COLONY"},
                 {"obrix_macroSpace", "DEPTH"},
             }},

            //------------------------------------------------------------------
            // OBBLIGATO (prefix: obbl_)  — Rascal Coral #FF8A7A
            // Dual wind instrument (breath A/B: flute + reed)
            //------------------------------------------------------------------
            {"obbligato",
             {
                 {"obbl_breathA", "BREATH A"},
                 {"obbl_embouchureA", "EMBOUCH"},
                 {"obbl_airFlutterA", "FLUTTER"},
                 {"obbl_instrumentA", "INST A"},
                 {"obbl_bodySizeA", "BODY A"},
                 {"obbl_breathB", "BREATH B"},
                 {"obbl_reedStiffness", "STIFF"},
                 {"obbl_reedBite", "BITE"},
                 {"obbl_instrumentB", "INST B"},
                 {"obbl_bodySizeB", "BODY B"},
                 {"obbl_sympatheticAmt", "SYMPATH"},
                 {"obbl_bondStage", "BOND"},
                 {"obbl_bondIntensity", "BONDING"},
                 {"obbl_bondRate", "BOND RT"},
                 {"obbl_fxAChorus", "CHORUS"},
                 {"obbl_fxABrightDelay", "BRTDLY"},
                 {"obbl_fxAPlate", "PLATE"},
                 {"obbl_fxAExciter", "EXCITE"},
                 {"obbl_fxBPhaser", "PHASER"},
                 {"obbl_fxBDarkDelay", "DRKDLY"},
                 {"obbl_fxBSpring", "SPRING"},
                 {"obbl_fxBTapeSat", "TAPE"},
                 // Macros
                 {"obbl_macroBreath", "BREATH"},
                 {"obbl_macroBond", "BOND"},
                 {"obbl_macroMischief", "MISCHF"},
                 {"obbl_macroWind", "WIND"},
             }},

            //------------------------------------------------------------------
            // OBLONG (prefix: bob_)  — Amber #E9A84A
            //------------------------------------------------------------------
            {"oblong",
             {
                 {"bob_texMode", "TEXTURE"},
                 {"bob_texLevel", "TEX LVL"},
                 {"bob_texTone", "TEX TNE"},
                 {"bob_texWidth", "TEX WDT"},
                 {"bob_fltChar", "FLTCHAR"},
                 {"bob_fltDrive", "FLTDRV"},
                 {"bob_curMode", "CURRENT"},
                 {"bob_curAmount", "CURAMT"},
                 {"bob_bobMode", "BOBMODE"},
                 {"bob_dustAmount", "DUST"},
                 // Macros
                 {"bob_macroCharacter", "BOBBLE"},
                 {"bob_macroMovement", "CURRENT"},
                 {"bob_macroCoupling", "COUPLE"},
                 {"bob_macroSpace", "SPACE"},
             }},

            //------------------------------------------------------------------
            // OASIS (prefix: oas_)  — Desert Spring Teal #00827F
            // Rhodes-style electric piano (swarm oscillator + Spectral Fingerprint)
            //------------------------------------------------------------------
            {"oasis",
             {
                 {"oas_entropySens", "ENTROPY"},
                 {"oas_breeze", "BREEZE"},
                 {"oas_swarmDensity", "SWARM"},
                 {"oas_lagoonDepth", "LAGOON"},
                 {"oas_subDrive", "SUBDRVR"},
                 {"oas_resonatorQ", "RESONQ"},
                 {"oas_cullDecay", "CULL"},
                 {"oas_canopyFB", "CANOPY"},
                 {"oas_rhodesMix", "RHODES"},
                 {"oas_pickupPos", "PICKUP"},
                 {"oas_bellAmount", "BELL"},
                 {"oas_tremoloDepth", "TREM DP"},
                 {"oas_tremoloRate", "TREM RT"},
                 // Macros
                 {"oas_macroCharacter", "OASIS"},
                 {"oas_macroMovement", "BREEZE"},
                 {"oas_macroCoupling", "SWARM"},
                 {"oas_macroSpace", "LAGOON"},
             }},

            //------------------------------------------------------------------
            // OCEAN DEEP (prefix: deep_)  — Trench Violet #2D0A4E
            // Deep-ocean pressure synthesis (Hydrostatic + Bioluminescent)
            //------------------------------------------------------------------
            {"oceandeep",
             {
                 {"deep_macroPressure", "DEPTH"},
                 // Macros
                 {"deep_macroCharacter", "BIOLUM"},
                 {"deep_macroMovement", "DRIFT"},
                 {"deep_macroCoupling", "PRESSURE"},
                 {"deep_macroSpace", "ABYSS"},
             }},

            //------------------------------------------------------------------
            // OCEANIC (prefix: ocean_)  — Phosphorescent Teal #00B4A0
            // Chromatophore modulator (separation + bioluminescent FX)
            //------------------------------------------------------------------
            {"oceanic",
             {
                 {"ocean_separation", "SEPAR"},
                 // Macros — Oceanic uses chromatophore-themed macro names
                 {"ocean_macroCharacter", "CHROMA"},
                 {"ocean_macroMovement", "PULSE"},
                 {"ocean_macroCoupling", "MERGE"},
                 {"ocean_macroSpace", "OCEAN"},
             }},

            //------------------------------------------------------------------
            // OCELOT (prefix: ocelot_)  — Ocelot Tawny #C5832B
            // Cross-feed matrix engine (forest-layer biome routing)
            //------------------------------------------------------------------
            {"ocelot",
             {
                 {"ocelot_xf_floorUnder", "F>UNDER"},  {"ocelot_xf_floorCanopy", "F>CANOP"},
                 {"ocelot_xf_floorEmerg", "F>EMERG"},  {"ocelot_xf_underFloor", "U>FLOOR"},
                 {"ocelot_xf_underCanopy", "U>CANOP"}, {"ocelot_xf_underEmerg", "U>EMERG"},
                 {"ocelot_xf_canopyFloor", "C>FLOOR"}, {"ocelot_xf_canopyUnder", "C>UNDER"},
                 {"ocelot_xf_canopyEmerg", "C>EMERG"}, {"ocelot_xf_emergFloor", "E>FLOOR"},
                 {"ocelot_xf_emergUnder", "E>UNDER"},  {"ocelot_xf_emergCanopy", "E>CANOP"},
                 {"ocelot_floorLevel", "FLRLVL"},      {"ocelot_chopRate", "CHOPRT"},
                 {"ocelot_bitDepth", "BITDPTH"},       {"ocelot_sampleRate", "SMPRATE"},
                 {"ocelot_dustLevel", "DUSTLVL"},      {"ocelot_understoryLevel", "UNDRLVL"},
                 {"ocelot_canopyLevel", "CNPYLVL"},    {"ocelot_canopySpectralFilter", "SPECTRL"},
                 {"ocelot_creatureType", "CRTYPE"},    {"ocelot_creatureRate", "CRRATE"},
                 {"ocelot_creatureAttack", "CRATTK"},  {"ocelot_creatureDecay", "CRDECAY"},
                 {"ocelot_filterEnvDepth", "FLTENVD"}, {"ocelot_ecosystemDepth", "ECODPTH"},
             }},

            //------------------------------------------------------------------
            // OCTAVE (prefix: oct_)  — Hammond Teak #8B6914
            // Adversarial pipe organ — Kitchen Collection, Chef quad
            //------------------------------------------------------------------
            {"octave",
             {
                 {"oct_organ", "ORGAN"},
                 {"oct_cluster", "CLUSTER"},
                 {"oct_chiff", "CHIFF"},
                 {"oct_buzz", "BUZZ"},
                 {"oct_pressure", "PRESURE"},
                 {"oct_crosstalk", "XTALK"},
                 {"oct_registration", "REGIST"},
                 {"oct_roomDepth", "ROOM"},
                 {"oct_competition", "COMPET"},
                 // Macros
                 {"oct_macroCharacter", "REGIST"},
                 {"oct_macroMovement", "CHIFF"},
                 {"oct_macroCoupling", "RIVALRY"},
                 {"oct_macroSpace", "HALL"},
             }},

            //------------------------------------------------------------------
            // OCTOPUS (prefix: octo_)  — Chromatophore Magenta #E040FB
            // Decentralized alien intelligence (arms + chromatophores + ink)
            //------------------------------------------------------------------
            {"octopus",
             {
                 {"octo_armCount", "ARMS"},
                 {"octo_armSpread", "SPREAD"},
                 {"octo_armBaseRate", "ARMRATE"},
                 {"octo_armDepth", "ARMDPTH"},
                 {"octo_chromaSens", "CHRSENS"},
                 {"octo_chromaSpeed", "CHRSPD"},
                 {"octo_chromaMorph", "CHRMORPH"},
                 {"octo_chromaDepth", "CHRDPTH"},
                 {"octo_chromaFreq", "CHRFREQ"},
                 {"octo_inkThreshold", "INKTHRS"},
                 {"octo_inkDensity", "INKDENS"},
                 {"octo_inkDecay", "INKDCY"},
                 {"octo_inkMix", "INKMIX"},
                 {"octo_shiftMicro", "MICRO"},
                 {"octo_shiftGlide", "GLIDE"},
                 {"octo_shiftDrift", "DRIFT"},
                 {"octo_suckerReso", "SUCKRES"},
                 {"octo_suckerFreq", "SUCKFRQ"},
                 {"octo_suckerDecay", "SUCKDCY"},
                 {"octo_suckerMix", "SUCKMIX"},
                 {"octo_wtPosition", "WT POS"},
                 {"octo_wtScanRate", "WTSCAN"},
                 // Macros
                 {"octo_macroCharacter", "CHROMA"},
                 {"octo_macroMovement", "ARMS"},
                 {"octo_macroCoupling", "INK"},
                 {"octo_macroSpace", "DEEP"},
             }},

            //------------------------------------------------------------------
            // ODDFELLOW (prefix: oddf_)  — Fusion Copper #B87333
            // Spectral electric piano — Kitchen Collection, Fusion quad
            //------------------------------------------------------------------
            {"oddfellow",
             {
                 {"oddf_reed", "REED"},
                 {"oddf_drive", "DRIVE"},
                 {"oddf_tremRate", "TREM RT"},
                 {"oddf_tremDepth", "TREM DP"},
                 {"oddf_migration", "MIGRATE"},
                 // Macros
                 {"oddf_macroCharacter", "REED"},
                 {"oddf_macroMovement", "TREMOLO"},
                 {"oddf_macroCoupling", "FUSION"},
                 {"oddf_macroSpace", "SHIMMER"},
             }},

            //------------------------------------------------------------------
            // ODDFELIX (prefix: snap_)  — Neon Tetra Blue #00A6D6
            // Neon tetra (OddfeliX) — dart-school pluck engine
            //------------------------------------------------------------------
            {"oddfelix",
             {
                 {"snap_snap", "SNAP"},
                 {"snap_pitchLock", "LOCK"},
                 {"snap_sweepDirection", "SWEEP"},
                 {"snap_unison", "SCHOOL"},
                 // Macros — OddfeliX has themed macro names
                 {"snap_macroDart", "DART"},
                 {"snap_macroSchool", "SCHOOL"},
                 {"snap_macroSurface", "SURFACE"},
                 {"snap_macroDepth", "DEPTH"},
                 // Also expose the generic ones if used
                 {"snap_macroCharacter", "DART"},
                 {"snap_macroMovement", "SCHOOL"},
                 {"snap_macroCoupling", "SURFACE"},
                 {"snap_macroSpace", "DEPTH"},
             }},

            //------------------------------------------------------------------
            // ODDOSCAR (prefix: morph_)  — Axolotl Gill Pink #E8839B
            // OddOscar (axolotl) — morph/bloom regeneration engine
            //------------------------------------------------------------------
            {"oddoscar",
             {
                 {"morph_morph", "MORPH"},
                 {"morph_bloom", "BLOOM"},
                 {"morph_sub", "SUB"},
                 {"morph_drift", "DRIFT"},
                 // Macros
                 {"morph_macroBloom", "BLOOM"},
                 {"morph_macroDrift", "DRIFT"},
                 {"morph_macroDepth", "DEPTH"},
                 {"morph_macroSpace", "SPACE"},
             }},

            //------------------------------------------------------------------
            // OFFERING (prefix: ofr_)  — Crate Wax Yellow #E5B80B
            // Psychology-driven boom bap drum synthesis (Berlyne curiosity)
            //------------------------------------------------------------------
            {"offering",
             {
                 {"ofr_digCuriosity", "CURIOSITY"},
                 {"ofr_cityMode", "CITY"},
                 // Macros
                 {"ofr_macroCharacter", "CURIOUS"},
                 {"ofr_macroMovement", "CITY"},
                 {"ofr_macroCoupling", "GROOVE"},
                 {"ofr_macroSpace", "SPACE"},
             }},

            //------------------------------------------------------------------
            // OGRE (prefix: ogre_)  — Sub Bass Black #0D0D0D
            // Sub-bass physical model (Kitchen Collection, Cellar quad)
            //------------------------------------------------------------------
            {"ogre",
             {
                 {"ogre_rootDepth", "ROOT"},
                 {"ogre_bodyResonance", "BODY"},
                 {"ogre_soil", "SOIL"},
                 {"ogre_tectonicRate", "TECTRT"},
                 {"ogre_tectonicDepth", "TECTDP"},
                 {"ogre_gravity", "GRAVITY"},
                 // Macros
                 {"ogre_macroCharacter", "ROOT"},
                 {"ogre_macroMovement", "TECTNIC"},
                 {"ogre_macroCoupling", "QUAKE"},
                 {"ogre_macroSpace", "CAVERN"},
             }},

            //------------------------------------------------------------------
            // OHM (prefix: ohm_)  — Sage #87AE73
            // Eclectic acoustic family (dad/inlaw/obed/obedient + FM)
            //------------------------------------------------------------------
            {"ohm",
             {
                 {"ohm_dadInstrument", "DAD"},
                 {"ohm_dadLevel", "DAD LVL"},
                 {"ohm_bowPressure", "BOW"},
                 {"ohm_bowSpeed", "BWSPEED"},
                 {"ohm_bodyMaterial", "WOOD"},
                 {"ohm_sympatheticAmt", "SYMPATH"},
                 {"ohm_inlawLevel", "INLAW"},
                 {"ohm_thereminScale", "SCALE"},
                 {"ohm_thereminWobble", "WOBBLE"},
                 {"ohm_glassBrightness", "GLASS"},
                 {"ohm_spectralFreeze", "FREEZE"},
                 {"ohm_grainSize", "GRAIN"},
                 {"ohm_grainDensity", "GRNDNS"},
                 {"ohm_grainScatter", "SCAT"},
                 {"ohm_obedLevel", "OBED"},
                 {"ohm_fmRatioPreset", "FM RT"},
                 {"ohm_fmIndex", "FM IDX"},
                 {"ohm_fmAttack", "FMATT"},
                 {"ohm_fmDecay", "FMDCY"},
                 {"ohm_meddlingThresh", "MEDDL"},
                 {"ohm_communeAbsorb", "COMMUNE"},
                 // Macros
                 {"ohm_macroJam", "JAM"},
                 {"ohm_macroMeddling", "MEDDL"},
                 {"ohm_macroCommune", "COMMUNE"},
                 {"ohm_macroMeadow", "MEADOW"},
             }},

            //------------------------------------------------------------------
            // OKEANOS (prefix: okan_)  — Cardamom Gold #C49B3F
            // Warm keys engine (tonewheel-adjacent warmth + bell + tremolo)
            //------------------------------------------------------------------
            {"okeanos",
             {
                 {"okan_warmth", "WARMTH"},
                 {"okan_bell", "BELL"},
                 {"okan_tremRate", "TREM RT"},
                 {"okan_tremDepth", "TREM DP"},
                 {"okan_migration", "MIGRATE"},
                 {"okan_filterEnvAmt", "FLTENV"},
                 // Macros
                 {"okan_macroCharacter", "WARMTH"},
                 {"okan_macroMovement", "TIDE"},
                 {"okan_macroCoupling", "CURRENT"},
                 {"okan_macroSpace", "OCEAN"},
             }},

            //------------------------------------------------------------------
            // OLATE (prefix: olate_)  — Fretless Walnut #5C3317
            // Fretless bass (Kitchen Collection, Cellar quad)
            //------------------------------------------------------------------
            {"olate",
             {
                 {"olate_pulseWidth", "PW"},
                 {"olate_vintage", "VINTGE"},
                 {"olate_terroir", "TERROIR"},
                 {"olate_ageRate", "AGE RT"},
                 {"olate_gravity", "GRAVITY"},
                 // Macros
                 {"olate_macroCharacter", "VINTGE"},
                 {"olate_macroMovement", "GLIDE"},
                 {"olate_macroCoupling", "GRAVITY"},
                 {"olate_macroSpace", "WOOD"},
             }},

            //------------------------------------------------------------------
            // OLE (prefix: ole_)  — Hibiscus #C9377A
            // Mediterranean plucked strings (3 aunts + husband)
            //------------------------------------------------------------------
            {"ole",
             {
                 {"ole_aunt1Level", "AUNT1"},
                 {"ole_aunt2Level", "AUNT2"},
                 {"ole_aunt3Level", "AUNT3"},
                 {"ole_aunt1StrumRate", "STRUM"},
                 {"ole_aunt1Brightness", "BRIGHT1"},
                 {"ole_aunt2CoinPress", "COIN"},
                 {"ole_aunt2GourdSize", "GOURD"},
                 {"ole_aunt3Tremolo", "TREMOLO"},
                 {"ole_aunt3Brightness", "BRIGHT3"},
                 {"ole_sympatheticAmt", "SYMPATH"},
                 {"ole_allianceConfig", "ALLIANC"},
                 {"ole_allianceBlend", "BLEND"},
                 {"ole_husbandOudLevel", "OUD"},
                 {"ole_husbandBouzLevel", "BOUZOUKI"},
                 {"ole_husbandPinLevel", "PINHEIRO"},
                 // Macros
                 {"ole_macroFuego", "FUEGO"},
                 {"ole_macroDrama", "DRAMA"},
                 {"ole_macroSides", "SIDES"},
                 {"ole_macroIsla", "ISLA"},
             }},

            //------------------------------------------------------------------
            // OLEG (prefix: oleg_)  — Theatre Red #C0392B
            // Hurdy-gurdy (Kitchen Collection, Chef quad)
            //------------------------------------------------------------------
            {"oleg",
             {
                 {"oleg_buzz", "BUZZ"},
                 {"oleg_drone", "DRONE"},
                 {"oleg_bellows", "BELLOWS"},
                 {"oleg_formant", "FORMANT"},
                 {"oleg_cassottoDepth", "CASOTTO"},
                 {"oleg_buzzThreshold", "BUZZTHR"},
                 {"oleg_wheelSpeed", "WHEEL"},
                 {"oleg_droneInterval1", "DRONE 1"},
                 {"oleg_droneInterval2", "DRONE 2"},
                 // Macros
                 {"oleg_macroCharacter", "WHEEL"},
                 {"oleg_macroMovement", "DRONE"},
                 {"oleg_macroCoupling", "RIVALRY"},
                 {"oleg_macroSpace", "THEATRE"},
             }},

            //------------------------------------------------------------------
            // OMEGA (prefix: omega_)  — Synth Bass Blue #003366
            // FM bass synth (Kitchen Collection, Cellar quad)
            //------------------------------------------------------------------
            {"omega",
             {
                 {"omega_algorithm", "ALGO"},
                 {"omega_ratio", "RATIO"},
                 {"omega_modIndex", "MOD IDX"},
                 {"omega_feedback", "FDBK"},
                 {"omega_purity", "PURITY"},
                 {"omega_distillRate", "DISTIL"},
                 {"omega_gravity", "GRAVITY"},
                 // Macros
                 {"omega_macroCharacter", "ALGO"},
                 {"omega_macroMovement", "MOD IDX"},
                 {"omega_macroCoupling", "GRAVITY"},
                 {"omega_macroSpace", "ABYSS"},
             }},

            //------------------------------------------------------------------
            // OMBRE (prefix: ombre_)  — Shadow Mauve #7B6B8A
            // Dual-narrative (memory/forgetting + perception crossfade)
            //------------------------------------------------------------------
            {"ombre",
             {
                 {"ombre_blend", "BLEND"},
                 {"ombre_interference", "INTRFR"},
                 {"ombre_memoryDecay", "MEMDCY"},
                 {"ombre_memoryGrain", "MEMGRN"},
                 {"ombre_memoryDrift", "MEMDRT"},
                 {"ombre_reactivity", "REACT"},
                 // Macros
                 {"ombre_macroCharacter", "MEMORY"},
                 {"ombre_macroMovement", "FADE"},
                 {"ombre_macroCoupling", "BLEND"},
                 {"ombre_macroSpace", "SHADOW"},
             }},

            //------------------------------------------------------------------
            // ONKOLO (prefix: onko_)  — Spectral Amber #FFBF00
            // Spectral electric piano (Kitchen Collection, Fusion quad)
            //------------------------------------------------------------------
            {"onkolo",
             {
                 {"onko_funk", "FUNK"},
                 {"onko_pickup", "PICKUP"},
                 {"onko_clunk", "CLUNK"},
                 {"onko_migration", "MIGRATE"},
                 // Macros
                 {"onko_macroCharacter", "FUNK"},
                 {"onko_macroMovement", "TREMOLO"},
                 {"onko_macroCoupling", "FUSION"},
                 {"onko_macroSpace", "SHIMMER"},
             }},

            //------------------------------------------------------------------
            // ONSET (prefix: perc_)  — Electric Blue #0066FF
            // Percussion synthesis engine (111 params, XVC system, B002)
            //------------------------------------------------------------------
            {"onset",
             {
                 // Global controls
                 {"perc_level", "LEVEL"},
                 {"perc_drive", "DRIVE"},
                 {"perc_masterTone", "TONE"},
                 {"perc_breathRate", "BRTHRTE"},
                 {"perc_char_grit", "GRIT"},
                 {"perc_char_warmth", "WARMTH"},
                 // FX
                 {"perc_fx_delay_time", "DLY TM"},
                 {"perc_fx_delay_feedback", "DLY FB"},
                 {"perc_fx_delay_mix", "DLY MX"},
                 {"perc_fx_reverb_size", "VRB SZ"},
                 {"perc_fx_reverb_decay", "VRB DCY"},
                 {"perc_fx_reverb_mix", "VRB MX"},
                 {"perc_fx_lofi_bits", "LFI BIT"},
                 {"perc_fx_lofi_mix", "LFI MX"},
                 // XVC cross-voice coupling
                 {"perc_xvc_kick_to_snare_filter", "K>S FLT"},
                 {"perc_xvc_snare_to_hat_decay", "S>H DCY"},
                 {"perc_xvc_kick_to_tom_pitch", "K>T PCH"},
                 {"perc_xvc_snare_to_perc_blend", "S>P BLD"},
                 {"perc_xvc_hat_choke", "HAT CHK"},
                 {"perc_xvc_global_amount", "XVC AMT"},
                 // Macros — ONSET uses machine/punch/space/mutate names
                 {"perc_macro_machine", "MACHINE"},
                 {"perc_macro_punch", "PUNCH"},
                 {"perc_macro_space", "SPACE"},
                 {"perc_macro_mutate", "MUTATE"},
             }},

            //------------------------------------------------------------------
            // OPAL (prefix: opal_)  — Lavender #A78BFA
            // Granular cloud synth
            //------------------------------------------------------------------
            {"opal",
             {
                 {"opal_grainSize", "GRAIN"},
                 {"opal_grainDensity", "DENSITY"},
                 {"opal_grainScatter", "SCATTER"},
                 {"opal_grainMix", "GRNMIX"},
                 {"opal_macroCharacter", "GRAIN"},
                 {"opal_macroMovement", "SCATTER"},
                 {"opal_macroCoupling", "CLOUD"},
                 {"opal_macroSpace", "SPACE"},
             }},

            //------------------------------------------------------------------
            // OPALINE (prefix: opal2_)  — Prepared Rust #B7410E
            // Prepared piano / thermal shock piano (Kitchen Collection, Kitchen quad)
            //------------------------------------------------------------------
            {"opaline",
             {
                 {"opal2_fragility", "FRAGILE"},
                 {"opal2_hammerHardness", "HAMMER"},
                 {"opal2_inharmonicity", "INHARM"},
                 {"opal2_shimmerAmount", "SHIMMER"},
                 {"opal2_thermalShock", "THERMAL"},
                 {"opal2_hfNoise", "HF NOIS"},
                 {"opal2_crystalDrive", "CRYSTAL"},
                 // Macros
                 {"opal2_macroCharacter", "FRAGILE"},
                 {"opal2_macroMovement", "SHOCK"},
                 {"opal2_macroCoupling", "RESONATE"},
                 {"opal2_macroSpace", "HALL"},
             }},

            //------------------------------------------------------------------
            // OPCODE (prefix: opco_)  — Cadet Blue #5F9EA0
            // DX-style FM synth (Kitchen Collection, Fusion quad)
            //------------------------------------------------------------------
            {"opcode",
             {
                 {"opco_algorithm", "ALGO"},
                 {"opco_ratio", "RATIO"},
                 {"opco_index", "INDEX"},
                 {"opco_feedback", "FDBK"},
                 {"opco_velToIndex", "VEL>IDX"},
                 {"opco_migration", "MIGRATE"},
                 // Macros
                 {"opco_macroCharacter", "ALGO"},
                 {"opco_macroMovement", "RATIO"},
                 {"opco_macroCoupling", "CODE"},
                 {"opco_macroSpace", "SHIMMER"},
             }},

            //------------------------------------------------------------------
            // OPENSKY (prefix: sky_)  — Sunburst #FF8C00
            // Euphoric shimmer synth (supersaw + Shepard shimmer + B023/B024)
            //------------------------------------------------------------------
            {"opensky",
             {
                 {"sky_sawSpread", "SPREAD"},
                 {"sky_subLevel", "SUBLVL"},
                 {"sky_subWave", "SUBWAVE"},
                 {"sky_pitchEnvAmount", "PENVAMT"},
                 {"sky_pitchEnvDecay", "PENVDCY"},
                 {"sky_filterHP", "HP CUT"},
                 {"sky_filterEnvAmount", "FLTENV"},
                 {"sky_shimmerMix", "SHIMMER"},
                 {"sky_shimmerSize", "SHMSZ"},
                 {"sky_shimmerDamping", "SHMDMP"},
                 {"sky_shimmerFeedback", "SHMFDBK"},
                 {"sky_shimmerOctave", "SHMOCT"},
                 {"sky_chorusRate", "CHR RT"},
                 {"sky_chorusDepth", "CHR DP"},
                 {"sky_chorusMix", "CHR MX"},
                 {"sky_unisonDetune", "UNISON"},
                 {"sky_stereoWidth", "WIDTH"},
                 {"sky_modSlot1Src", "MOD1 SR"},
                 {"sky_modSlot1Dst", "MOD1 DS"},
                 {"sky_modSlot1Amt", "MOD1 AM"},
                 {"sky_modSlot2Src", "MOD2 SR"},
                 {"sky_modSlot2Dst", "MOD2 DS"},
                 {"sky_modSlot2Amt", "MOD2 AM"},
                 // Macros — OpenSky uses Rise/Width/Glow/Air
                 {"sky_macroRise", "RISE"},
                 {"sky_macroWidth", "WIDTH"},
                 {"sky_macroGlow", "GLOW"},
                 {"sky_macroAir", "AIR"},
             }},

            //------------------------------------------------------------------
            // OPERA (prefix: opera_)  — Aria Gold #D4AF37
            // Additive-vocal Kuramoto synchronicity engine
            //------------------------------------------------------------------
            {"opera",
             {
                 {"opera_drama", "DRAMA"},
                 {"opera_voiceCount", "VOICES"},
                 {"opera_chorusSync", "CHORUS"},
                 {"opera_stageWidth", "STAGE"},
                 {"opera_arcMode", "CONDUCT"},
                 {"opera_conductorTimescale", "ARC"},
                 {"opera_conductorPeak", "PEAK"},
                 {"opera_conductorJitter", "JITTER"},
                 {"opera_formantShift", "FORMANT"},
                 {"opera_vibrato", "VIBRATO"},
                 // Macros
                 {"opera_macroCharacter", "DRAMA"},
                 {"opera_macroMovement", "VOICE"},
                 {"opera_macroCoupling", "CHORUS"},
                 {"opera_macroSpace", "STAGE"},
             }},

            //------------------------------------------------------------------
            // OPTIC (prefix: optic_)  — Phosphor Green #00FF41
            // Visual modulation engine (Zero-Audio Identity, B005)
            //------------------------------------------------------------------
            {"optic",
             {
                 {"optic_reactivity", "REACT"},
                 {"optic_inputGain", "GAIN"},
                 {"optic_autoPulse", "AUTOPLS"},
                 {"optic_pulseRate", "PULSE RT"},
                 {"optic_pulseShape", "PLSSHP"},
                 {"optic_pulseSwing", "SWING"},
                 {"optic_pulseEvolve", "EVOLVE"},
                 {"optic_pulseSubdiv", "SUBDIV"},
                 {"optic_pulseAccent", "ACCENT"},
                 {"optic_modDepth", "MOD DP"},
                 {"optic_modMixPulse", "PLS MIX"},
                 {"optic_modMixSpec", "SPEC MX"},
                 {"optic_vizMode", "VIZ"},
                 {"optic_vizFeedback", "VIZFDBK"},
                 {"optic_vizSpeed", "VIZSPD"},
                 {"optic_vizIntensity", "VIZINT"},
                 // Optic has no traditional macros — visual engine
             }},

            //------------------------------------------------------------------
            // ORACLE (prefix: oracle_)  — Prophecy Indigo #4B0082
            // GENDY stochastic synthesis + Maqam modes (B010)
            //------------------------------------------------------------------
            {"oracle",
             {
                 {"oracle_breakpoints", "BRKPTS"},
                 {"oracle_timeStep", "TIMESTP"},
                 {"oracle_ampStep", "AMPSTEP"},
                 {"oracle_distribution", "DIST"},
                 {"oracle_barrierElasticity", "ELASTIC"},
                 {"oracle_maqam", "MAQAM"},
                 {"oracle_gravity", "GRAVITY"},
                 {"oracle_drift", "DRIFT"},
                 {"oracle_stochEnvAttack", "SATT"},
                 {"oracle_stochEnvDecay", "SDCY"},
                 {"oracle_stochEnvSustain", "SSUS"},
                 {"oracle_stochEnvRelease", "SREL"},
                 {"oracle_velDriftDepth", "VELDRT"},
                 // Macros
                 {"oracle_macroProphecy", "PROPHECY"},
                 {"oracle_macroEvolution", "EVOLVE"},
                 {"oracle_macroGravity", "GRAVITY"},
                 {"oracle_macroDrift", "DRIFT"},
             }},

            //------------------------------------------------------------------
            // ORCHARD (prefix: orch_)  — Orchard Blossom #FFB7C5
            // String ensemble growth synth (Kitchen Collection, Garden quad)
            //------------------------------------------------------------------
            {"orchard",
             {
                 {"orch_formant", "FORMANT"},
                 {"orch_ensembleWidth", "ENSEMBL"},
                 {"orch_season", "SEASON"},
                 {"orch_growthMode", "GROWTH"},
                 {"orch_growthTime", "GRWTIME"},
                 // Macros
                 {"orch_macroCharacter", "SEASON"},
                 {"orch_macroMovement", "GROWTH"},
                 {"orch_macroCoupling", "BLOSSOM"},
                 {"orch_macroSpace", "ORCHARD"},
             }},

            //------------------------------------------------------------------
            // ORBWEAVE (prefix: weave_)  — Kelp Knot Purple #8E4585
            // Topological knot coupling (B021/B022 Trefoil/Figure-Eight)
            //------------------------------------------------------------------
            {"orbweave",
             {
                 {"weave_strandType", "STRAND"},
                 {"weave_strandTune", "TUNE"},
                 {"weave_knotType", "KNOT"},
                 {"weave_braidDepth", "BRAID"},
                 {"weave_torusP", "TORUS P"},
                 {"weave_torusQ", "TORUS Q"},
                 // Macros
                 {"weave_macroWeave", "WEAVE"},
                 {"weave_macroTension", "TENSION"},
                 {"weave_macroKnot", "KNOT"},
                 {"weave_macroSpace", "TANGLE"},
             }},

            //------------------------------------------------------------------
            // ORCA (prefix: orca_)  — Deep Ocean #1B2838
            // Apex predator (wavetable + echolocation + breach)
            //------------------------------------------------------------------
            {"orca",
             {
                 {"orca_wtPosition", "WT POS"},
                 {"orca_wtScanRate", "SCAN"},
                 {"orca_formantIntensity", "FRMNTI"},
                 {"orca_formantShift", "FORMANT"},
                 {"orca_echoRate", "ECHO RT"},
                 {"orca_echoReso", "ECHO RS"},
                 {"orca_echoDamp", "ECHO DP"},
                 {"orca_echoMix", "ECHO MX"},
                 {"orca_breachSub", "BRCHSUB"},
                 {"orca_breachShape", "BRCHSHP"},
                 {"orca_breachThreshold", "BRCHTHR"},
                 {"orca_breachRatio", "BRCHRAT"},
                 {"orca_crushBits", "CRSHBIT"},
                 {"orca_crushDownsample", "CRSHDS"},
                 {"orca_crushMix", "CRSHMIX"},
                 {"orca_crushSplitFreq", "CRSHSPLT"},
                 {"orca_velCutoffAmt", "VEL CUT"},
                 // Macros — Orca uses huntMacro
                 {"orca_huntMacro", "HUNT"},
                 {"orca_macroCharacter", "HUNT"},
                 {"orca_macroMovement", "BREACH"},
                 {"orca_macroCoupling", "ECHO"},
                 {"orca_macroSpace", "OCEAN"},
             }},

            //------------------------------------------------------------------
            // ORGANISM (prefix: org_)  — Emergence Lime #C6E377
            // Cellular automata generative engine (Coral Colony)
            //------------------------------------------------------------------
            {"organism",
             {
                 {"org_ruleSet", "GENOME"},
                 {"org_cellSize", "CELL"},
                 {"org_mutationRate", "MUTATE"},
                 {"org_density", "COLONY"},
                 {"org_generation", "GEN"},
                 // Macros
                 {"org_macroCharacter", "RULES"},
                 {"org_macroMovement", "GROW"},
                 {"org_macroCoupling", "COLONY"},
                 {"org_macroSpace", "SPACE"},
             }},

            //------------------------------------------------------------------
            // ORGANON (prefix: organon_)  — Bioluminescent Cyan #00CED1
            // Variational free-energy metabolism engine (B011 — publishable)
            //------------------------------------------------------------------
            {"organon",
             {
                 {"organon_metabolicRate", "METABOL"},
                 {"organon_enzymeSelect", "ENZYME"},
                 {"organon_catalystDrive", "CATALST"},
                 {"organon_dampingCoeff", "DAMPING"},
                 {"organon_signalFlux", "FLUX"},
                 {"organon_phasonShift", "PHASON"},
                 {"organon_isotopeBalance", "ISOTOPE"},
                 {"organon_lockIn", "LOCK IN"},
                 {"organon_membrane", "MEMBRAN"},
                 {"organon_noiseColor", "NOISE"},
                 // Macros
                 {"organon_macroMetabolism", "METABOL"},
                 {"organon_macroSpectrum", "SPECTRM"},
                 {"organon_macroCoupling", "SYMBIOSE"},
                 {"organon_macroSpace", "VOID"},
             }},

            //------------------------------------------------------------------
            // ORIGAMI (prefix: origami_)  — Vermillion Fold #E63946
            // Wavefold / warp synthesis (fold-point morphology)
            //------------------------------------------------------------------
            {"origami",
             {
                 {"origami_foldPoint", "FOLD PT"},
                 {"origami_foldDepth", "FOLD DP"},
                 {"origami_foldCount", "FOLDS"},
                 {"origami_operation", "OP"},
                 {"origami_rotate", "ROTATE"},
                 {"origami_stretch", "STRETCH"},
                 {"origami_freeze", "FREEZE"},
                 {"origami_oscMix", "OSCMIX"},
                 {"origami_foldEnvAttack", "FLDATT"},
                 {"origami_foldEnvDecay", "FLDDCY"},
                 {"origami_foldEnvSustain", "FLDSUS"},
                 {"origami_foldEnvRelease", "FLDREL"},
                 // Macros — Origami has custom Fold/Motion names
                 {"origami_macroFold", "FOLD"},
                 {"origami_macroMotion", "MOTION"},
                 {"origami_macroCoupling", "CREASE"},
                 {"origami_macroSpace", "UNFOLD"},
             }},

            //------------------------------------------------------------------
            // ORPHICA (prefix: orph_)  — Siren Seafoam #7FDBCA
            // Karplus-Strong strings + micro-grain FX (velocity→body resonance)
            //------------------------------------------------------------------
            {"orphica",
             {
                 {"orph_stringMaterial", "MATERIAL"},
                 {"orph_pluckBrightness", "BRIGHT"},
                 {"orph_pluckPosition", "PLKPOS"},
                 {"orph_stringCount", "STRINGS"},
                 {"orph_bodySize", "BODY"},
                 {"orph_sympatheticAmt", "SYMPATH"},
                 {"orph_microMode", "MICRO"},
                 {"orph_microRate", "MIC RT"},
                 {"orph_microSize", "MIC SZ"},
                 {"orph_microDensity", "MIC DNS"},
                 {"orph_microScatter", "MIC SCT"},
                 {"orph_microMix", "MIC MX"},
                 {"orph_crossoverNote", "XOVNOTE"},
                 {"orph_crossoverBlend", "XOVBLND"},
                 {"orph_subAmount", "SUB"},
                 {"orph_tapeSat", "TAPE"},
                 {"orph_darkDelayTime", "DRK DLY"},
                 {"orph_darkDelayFb", "DRK FB"},
                 {"orph_deepPlateMix", "PLATE"},
                 {"orph_shimmerMix", "SHIMMER"},
                 {"orph_microDelayTime", "MIC DLY"},
                 {"orph_spectralSmear", "SMEAR"},
                 {"orph_crystalChorusRate", "CRY RT"},
                 {"orph_crystalChorusDpth", "CRY DP"},
                 // Macros — Orphica has pluck/fracture/surface/divine
                 {"orph_macroPluck", "PLUCK"},
                 {"orph_macroFracture", "FRACTUR"},
                 {"orph_macroSurface", "SURFACE"},
                 {"orph_macroDivine", "DIVINE"},
             }},

            //------------------------------------------------------------------
            // OSIER (prefix: osier_)  — Willow Silver #C0C8C8
            // Willow strings (Kitchen Collection, Garden quad)
            //------------------------------------------------------------------
            {"osier",
             {
                 {"osier_companion", "COMPAN"},
                 {"osier_intimacy", "INTIMCY"},
                 {"osier_growthMode", "GROWTH"},
                 {"osier_growthTime", "GRWTIME"},
                 // Macros
                 {"osier_macroCharacter", "WILLOW"},
                 {"osier_macroMovement", "SWAY"},
                 {"osier_macroCoupling", "INTIMCY"},
                 {"osier_macroSpace", "MEADOW"},
             }},

            //------------------------------------------------------------------
            // OSMOSIS (prefix: osmo_)  — Surface Tension Silver #C0C0C0
            // External audio membrane (envelope follower + pitch detect)
            //------------------------------------------------------------------
            {"osmosis",
             {
                 {"osmo_permeability", "PERMBLE"},
                 {"osmo_selectivity", "SELECT"},
                 {"osmo_reactivity", "REACT"},
                 {"osmo_memory", "MEMORY"},
                 // Osmosis macros not yet confirmed — use generic names
                 {"osmo_macroCharacter", "PERMBLE"},
                 {"osmo_macroMovement", "REACT"},
                 {"osmo_macroCoupling", "ABSORB"},
                 {"osmo_macroSpace", "MEMBRANE"},
             }},

            //------------------------------------------------------------------
            // OSPREY (prefix: osprey_)  — Azulejo Blue #1B4F8A
            // Shore system — coastal acoustic engine (B012 coastlines)
            //------------------------------------------------------------------
            {"osprey",
             {
                 {"osprey_shore", "SHORE"},
                 {"osprey_seaState", "SEA"},
                 {"osprey_swellPeriod", "SWELL"},
                 {"osprey_windDir", "WIND"},
                 {"osprey_depth", "DEPTH"},
                 {"osprey_resonatorBright", "RESBRT"},
                 {"osprey_resonatorDecay", "RESDCY"},
                 {"osprey_sympathyAmount", "SYMPATH"},
                 {"osprey_creatureRate", "CRATURE"},
                 {"osprey_creatureDepth", "CRDEPTH"},
                 {"osprey_coherence", "COHERE"},
                 {"osprey_foam", "FOAM"},
                 {"osprey_brine", "BRINE"},
                 {"osprey_hull", "HULL"},
                 {"osprey_filterTilt", "TILT"},
                 {"osprey_harborVerb", "HARBOR"},
                 {"osprey_fog", "FOG"},
                 // Macros
                 {"osprey_macroCharacter", "SHORE"},
                 {"osprey_macroMovement", "SWELL"},
                 {"osprey_macroCoupling", "TIDE"},
                 {"osprey_macroSpace", "HARBOR"},
             }},

            //------------------------------------------------------------------
            // OSTERIA (prefix: osteria_)  — Porto Wine #722F37
            // Porto wine tavern ensemble (B012 ShoreSystem)
            //------------------------------------------------------------------
            {"osteria",
             {
                 {"osteria_qBassShore", "BASS SH"},
                 {"osteria_qHarmShore", "HARM SH"},
                 {"osteria_qMelShore", "MEL SH"},
                 {"osteria_qRhythmShore", "RHY SH"},
                 {"osteria_qElastic", "ELASTIC"},
                 {"osteria_qStretch", "STRETCH"},
                 {"osteria_qMemory", "MEMORY"},
                 {"osteria_qSympathy", "SYMPATH"},
                 {"osteria_bassLevel", "BASS"},
                 {"osteria_harmLevel", "HARM"},
                 {"osteria_melLevel", "MEL"},
                 {"osteria_rhythmLevel", "RHYTHM"},
                 {"osteria_ensWidth", "WIDTH"},
                 {"osteria_blendMode", "BLEND"},
                 {"osteria_tavernMix", "TAVERN"},
                 {"osteria_tavernShore", "TVSHR"},
                 {"osteria_murmur", "MURMUR"},
                 {"osteria_warmth", "WARMTH"},
                 {"osteria_oceanBleed", "OCEAN"},
                 {"osteria_patina", "PATINA"},
                 {"osteria_porto", "PORTO"},
                 {"osteria_smoke", "SMOKE"},
                 {"osteria_filterEnvDepth", "FLTENV"},
                 {"osteria_sessionDelay", "DELAY"},
                 {"osteria_hall", "HALL"},
                 {"osteria_chorus", "CHORUS"},
                 {"osteria_tape", "TAPE"},
                 // Macros
                 {"osteria_macroCharacter", "PORTO"},
                 {"osteria_macroMovement", "SWELL"},
                 {"osteria_macroCoupling", "TAVERN"},
                 {"osteria_macroSpace", "CELLAR"},
             }},

            //------------------------------------------------------------------
            // OSTINATO (prefix: osti_)  — Firelight Orange #E8701A
            // Modal membrane world-rhythm engine (B017-B020, 96 patterns)
            //------------------------------------------------------------------
            {"ostinato",
             {
                 // Global
                 {"osti_tempo", "TEMPO"},
                 {"osti_swing", "SWING"},
                 {"osti_masterTune", "TUNE"},
                 {"osti_masterDecay", "DECAY"},
                 {"osti_masterFilter", "FILTER"},
                 {"osti_masterReso", "RESO"},
                 {"osti_reverbSize", "VRB SZ"},
                 {"osti_reverbDamp", "VRB DMP"},
                 {"osti_reverbMix", "VRB MX"},
                 {"osti_compThresh", "CMPTHRS"},
                 {"osti_compRatio", "CMPRATIO"},
                 {"osti_compAttack", "CMPATTK"},
                 {"osti_compRelease", "CMPRLS"},
                 {"osti_circleAmount", "CIRCLE"},
                 {"osti_humanize", "HUMANIZE"},
                 // Macros — Ostinato has Gather/Fire/Circle/Space
                 {"osti_macroGather", "GATHER"},
                 {"osti_macroFire", "FIRE"},
                 {"osti_macroCircle", "CIRCLE"},
                 {"osti_macroSpace", "SPACE"},
             }},

            //------------------------------------------------------------------
            // OTIS (prefix: otis_)  — Gospel Gold #D4A017
            // Gospel Hammond B3 (Kitchen Collection, Chef quad)
            //------------------------------------------------------------------
            {"otis",
             {
                 {"otis_organ", "ORGAN"},
                 {"otis_leslie", "LESLIE"},
                 {"otis_keyClick", "CLICK"},
                 {"otis_percussion", "PERC"},
                 {"otis_percHarmonic", "PERCHM"},
                 {"otis_percDecay", "PERCDCY"},
                 {"otis_crosstalk", "XTALK"},
                 {"otis_drive", "DRIVE"},
                 {"otis_instability", "INSTBL"},
                 {"otis_musette", "MUSETTE"},
                 {"otis_bendAmount", "BEND"},
                 // Macros
                 {"otis_macroCharacter", "DRAWBAR"},
                 {"otis_macroMovement", "LESLIE"},
                 {"otis_macroCoupling", "RIVALRY"},
                 {"otis_macroSpace", "CHAPEL"},
             }},

            //------------------------------------------------------------------
            // OTO (prefix: oto_)  — Pipe Organ Ivory #F5F0E8
            // Pipe organ (Kitchen Collection, Chef quad)
            //------------------------------------------------------------------
            {"oto",
             {
                 {"oto_organ", "ORGAN"},
                 {"oto_cluster", "CLUSTER"},
                 {"oto_chiff", "CHIFF"},
                 {"oto_buzz", "BUZZ"},
                 {"oto_pressure", "PRESURE"},
                 {"oto_crosstalk", "XTALK"},
                 {"oto_competition", "COMPET"},
                 // Macros — OTO uses A/B/C/D
                 {"oto_macroA", "PIPES"},
                 {"oto_macroB", "CHIFF"},
                 {"oto_macroC", "RIVALRY"},
                 {"oto_macroD", "HALL"},
             }},

            //------------------------------------------------------------------
            // OTTONI (prefix: otto_)  — Patina #5B8A72
            // Brass family trio (toddler/tween/teen + foreign interval)
            //------------------------------------------------------------------
            {"ottoni",
             {
                 {"otto_toddlerLevel", "TODLR"},
                 {"otto_toddlerPressure", "TDLRPRS"},
                 {"otto_toddlerInst", "TDLRINST"},
                 {"otto_tweenLevel", "TWEEN"},
                 {"otto_tweenEmbouchure", "TWNEMB"},
                 {"otto_tweenValve", "TWENVAL"},
                 {"otto_tweenInst", "TWNINST"},
                 {"otto_teenLevel", "TEEN"},
                 {"otto_teenEmbouchure", "TNEMB"},
                 {"otto_teenBore", "TNBORE"},
                 {"otto_teenInst", "TNINST"},
                 {"otto_teenVibratoRate", "TNVBRT"},
                 {"otto_teenVibratoDepth", "TNVBDP"},
                 {"otto_sympatheticAmt", "SYMPATH"},
                 {"otto_foreignStretch", "FORGSTR"},
                 {"otto_foreignDrift", "FORGDRT"},
                 {"otto_foreignCold", "FORGCLD"},
                 // Macros
                 {"otto_macroEmbouchure", "EMBOUCH"},
                 {"otto_macroGrow", "GROW"},
                 {"otto_macroForeign", "FOREIGN"},
                 {"otto_macroLake", "LAKE"},
             }},

            //------------------------------------------------------------------
            // OUIE (prefix: ouie_)  — Hammerhead Steel #708090
            // Duophonic hammerhead (2 voices × 8 algorithms, B025-B027)
            //------------------------------------------------------------------
            {"ouie",
             {
                 // Voice 1
                 {"ouie_algo1", "ALGO 1"},
                 {"ouie_waveform1", "WAVE 1"},
                 {"ouie_algoParam1", "PARAM1"},
                 {"ouie_wtPos1", "WT POS1"},
                 {"ouie_fmRatio1", "FM RT1"},
                 {"ouie_fmIndex1", "FM IDX1"},
                 {"ouie_pw1", "PW 1"},
                 // Voice 2
                 {"ouie_algo2", "ALGO 2"},
                 {"ouie_waveform2", "WAVE 2"},
                 {"ouie_algoParam2", "PARAM2"},
                 {"ouie_wtPos2", "WT POS2"},
                 {"ouie_fmRatio2", "FM RT2"},
                 {"ouie_fmIndex2", "FM IDX2"},
                 {"ouie_pw2", "PW 2"},
                 // Voice mix & filters
                 {"ouie_voiceMix", "V MIX"},
                 {"ouie_cutoff1", "CUT 1"},
                 {"ouie_reso1", "RES 1"},
                 {"ouie_filterMode1", "FLTMD1"},
                 {"ouie_cutoff2", "CUT 2"},
                 {"ouie_reso2", "RES 2"},
                 {"ouie_filterMode2", "FLTMD2"},
                 {"ouie_filterLink", "FLTLNK"},
                 // Envelopes
                 {"ouie_ampA1", "AMP A1"},
                 {"ouie_ampD1", "AMP D1"},
                 {"ouie_ampS1", "AMP S1"},
                 {"ouie_ampR1", "AMP R1"},
                 {"ouie_ampA2", "AMP A2"},
                 {"ouie_ampD2", "AMP D2"},
                 {"ouie_ampS2", "AMP S2"},
                 {"ouie_ampR2", "AMP R2"},
                 {"ouie_modDepth", "MOD DP"},
                 // LFOs
                 {"ouie_lfo1Rate", "LFO1 RT"},
                 {"ouie_lfo1Depth", "LFO1 DP"},
                 {"ouie_lfo1Shape", "LFO1 SH"},
                 {"ouie_lfo2Rate", "LFO2 RT"},
                 {"ouie_lfo2Depth", "LFO2 DP"},
                 {"ouie_lfo2Shape", "LFO2 SH"},
                 {"ouie_breathRate", "BRTHRT"},
                 {"ouie_breathDepth", "BRTHDP"},
                 {"ouie_unisonDetune", "UNISON"},
                 {"ouie_voiceMode", "VOICMD"},
                 // Macros — Ouïe has anatomical shark names
                 {"ouie_macroHammer", "HAMMER"},
                 {"ouie_macroAmpullae", "AMPULL"},
                 {"ouie_macroCartilage", "CARTILG"},
                 {"ouie_macroCurrent", "CURRENT"},
             }},

            //------------------------------------------------------------------
            // OUROBOROS (prefix: ouro_)  — Strange Attractor Red #FF2D2D
            // Lorenz-attractor synth (B003 Leash + B007 velocity coupling)
            //------------------------------------------------------------------
            {"ouroboros",
             {
                 {"ouro_topology", "TOPOL"},
                 {"ouro_rate", "RATE"},
                 {"ouro_chaosIndex", "CHAOS"},
                 {"ouro_leash", "LEASH"},
                 {"ouro_theta", "THETA"},
                 {"ouro_phi", "PHI"},
                 {"ouro_damping", "DAMP"},
                 {"ouro_injection", "INJECT"},
                 {"ouro_breathRate", "BRTHRT"},
                 {"ouro_velTimbre", "VELTMB"},
                 // Macros — Ouroboros has char/move/coup/space
                 {"ouro_macroChar", "CHAOS"},
                 {"ouro_macroMove", "ORBIT"},
                 {"ouro_macroCoup", "LEASH"},
                 {"ouro_macroSpace", "STRANGE"},
             }},

            //------------------------------------------------------------------
            // OUTFLOW (prefix: out_)  — Deep Storm Indigo #1A1A40
            // Storm surge / pressure release engine
            //------------------------------------------------------------------
            {"outflow",
             {
                 {"out_anchor", "ANCHOR"},
                 {"out_windChaos", "WIND"},
                 {"out_undertowPull", "UNDRTOW"},
                 {"out_splashSize", "SPLASH"},
                 {"out_exciterDecay", "EXCDCY"},
                 {"out_vacuumAttack", "VACUUM"},
                 {"out_opticalWarmth", "WARMTH"},
                 // Macros
                 {"out_macroCharacter", "STORM"},
                 {"out_macroMovement", "SURGE"},
                 {"out_macroCoupling", "UNDRTOW"},
                 {"out_macroSpace", "ABYSS"},
             }},

            //------------------------------------------------------------------
            // OUTLOOK (prefix: look_)  — Horizon Indigo #4169E1
            // Panoramic dual-wavetable (parallax stereo + vista filter)
            //------------------------------------------------------------------
            {"outlook",
             {
                 {"look_horizonScan", "HORIZN"},
                 {"look_parallaxAmount", "PARALLX"},
                 {"look_vistaLine", "VISTA"},
                 {"look_waveShape1", "WAVE 1"},
                 {"look_waveShape2", "WAVE 2"},
                 {"look_oscMix", "OSC MIX"},
                 {"look_lfo1Rate", "LFO1 RT"},
                 {"look_lfo1Depth", "LFO1 DP"},
                 {"look_lfo2Rate", "LFO2 RT"},
                 {"look_lfo2Depth", "LFO2 DP"},
                 {"look_modWheelDepth", "MW DP"},
                 {"look_aftertouchDepth", "AT DP"},
                 // Macros
                 {"look_macroCharacter", "HORIZN"},
                 {"look_macroMovement", "SCAN"},
                 {"look_macroCoupling", "PARALLX"},
                 {"look_macroSpace", "VISTA"},
             }},

            //------------------------------------------------------------------
            // OUTWIT (prefix: owit_)  — Chromatophore Amber #CC6600
            // Chromatophore step sequencer (camouflage neural solver)
            //------------------------------------------------------------------
            {"outwit",
             {
                 {"owit_stepRate", "STEP RT"},
                 {"owit_stepSync", "SYNC"},
                 {"owit_stepDiv", "SUBDIV"},
                 {"owit_synapse", "SYNAPSE"},
                 {"owit_chromAmount", "CHROMA"},
                 {"owit_solve", "SOLVE"},
                 {"owit_huntRate", "HUNT RT"},
                 {"owit_targetBrightness", "TGT BRT"},
                 {"owit_targetWarmth", "TGT WRM"},
                 {"owit_targetMovement", "TGT MOV"},
                 {"owit_targetDensity", "TGT DNS"},
                 {"owit_targetSpace", "TGT SPC"},
                 {"owit_targetAggression", "TGT AGG"},
                 {"owit_inkCloud", "INK"},
                 {"owit_inkDecay", "INKDCY"},
                 {"owit_triggerThresh", "TRIGTHRS"},
                 // Macros
                 {"owit_macroSolve", "SOLVE"},
                 {"owit_macroSynapse", "SYNAPSE"},
                 {"owit_macroChromatophore", "CHROMA"},
                 {"owit_macroDen", "DEN"},
             }},

            //------------------------------------------------------------------
            // OVERBITE (prefix: poss_)  — Fang White #F0EDE8
            // Five-macro opossum synth (B008 BELLY/BITE/SCURRY/TRASH/PLAY DEAD)
            //------------------------------------------------------------------
            {"overbite",
             {
                 {"poss_oscADrift", "DRIFT"},
                 {"poss_oscBInstability", "INSTBL"},
                 {"poss_oscInteractMode", "IXMODE"},
                 {"poss_oscInteractAmount", "IXAMT"},
                 {"poss_weightShape", "WGTSHP"},
                 {"poss_weightOctave", "WGTOCT"},
                 {"poss_weightLevel", "WGTLVL"},
                 {"poss_weightTune", "WGTUNE"},
                 {"poss_furAmount", "FUR"},
                 {"poss_chewAmount", "CHEW"},
                 {"poss_chewFreq", "CHEWFRQ"},
                 {"poss_gnashAmount", "GNASH"},
                 {"poss_trashMode", "TRASH"},
                 {"poss_trashAmount", "TRSHAMT"},
                 {"poss_filterKeyTrack", "KEYTRAK"},
                 {"poss_filterDrive", "FLTDRV"},
                 {"poss_modEnvDest", "MODDEST"},
             }},

            //------------------------------------------------------------------
            // OVERDUB (prefix: dub_)  — Olive #6B7B3A
            // Tape dub / spring delay engine (B004 Spring Reverb)
            //------------------------------------------------------------------
            {"overdub",
             {
                 {"dub_pitchEnvDepth", "PTCHENV"},
                 {"dub_pitchEnvDecay", "PTCHDCY"},
                 {"dub_sendLevel", "SEND"},
                 {"dub_returnLevel", "RETURN"},
                 {"dub_dryLevel", "DRY"},
                 {"dub_driveAmount", "DRIVE"},
                 {"dub_delayTime", "DLY TM"},
                 {"dub_delayFeedback", "DLY FB"},
                 {"dub_delayWear", "WEAR"},
                 {"dub_delayWow", "WOW"},
                 {"dub_delayMix", "DLY MX"},
                 {"dub_drift", "DRIFT"},
                 // Macros
                 {"dub_macroCharacter", "TAPE"},
                 {"dub_macroMovement", "DRIFT"},
                 {"dub_macroCoupling", "SEND"},
                 {"dub_macroSpace", "SPRING"},
             }},

            //------------------------------------------------------------------
            // OVERGROW (prefix: grow_)  — Forest Green #228B22
            // Bowed strings growth (Kitchen Collection, Garden quad)
            //------------------------------------------------------------------
            {"overgrow",
             {
                 {"grow_bowNoise", "BWNOISE"},
                 {"grow_wildness", "WILD"},
                 {"grow_growthMode", "GROWTH"},
                 {"grow_growthTime", "GRWTIME"},
                 // Macros
                 {"grow_macroCharacter", "WILD"},
                 {"grow_macroMovement", "GROWTH"},
                 {"grow_macroCoupling", "TANGLE"},
                 {"grow_macroSpace", "FOREST"},
             }},

            //------------------------------------------------------------------
            // OVERLAP (prefix: olap_)  — Bioluminescent Cyan-Green #00FFB4
            // KnotMatrix FDN entangled reverb (B021 topology)
            //------------------------------------------------------------------
            {"overlap",
             {
                 // Macros
                 {"olap_macroKnot", "KNOT"},
                 {"olap_macroPulse", "PULSE"},
                 {"olap_macroEntrain", "ENTRAIN"},
                 {"olap_macroBloom", "BLOOM"},
             }},

            //------------------------------------------------------------------
            // OVERCAST (prefix: cast_)  — Light Slate Gray #778899
            // Crystalline freezing (Kitchen Collection, Broth quad)
            //------------------------------------------------------------------
            {"overcast",
             {
                 {"cast_freezeRate", "FREEZE"},
                 {"cast_crystalSize", "CRYSTAL"},
                 {"cast_numPeaks", "PEAKS"},
                 {"cast_transition", "TRANSIT"},
                 {"cast_latticeSnap", "LATTICE"},
                 {"cast_purity", "PURITY"},
                 {"cast_crackle", "CRACKLE"},
                 {"cast_shatterGap", "SHATTER"},
                 // Macros
                 {"cast_macroCharacter", "CRYSTAL"},
                 {"cast_macroMovement", "FREEZE"},
                 {"cast_macroCoupling", "LATTICE"},
                 {"cast_macroSpace", "CLOUD"},
             }},

            //------------------------------------------------------------------
            // OVERFLOW (prefix: flow_)  — Deep Current Blue #1A3A5C
            // Vessel overflow (Kitchen Collection, Broth quad)
            //------------------------------------------------------------------
            {"overflow",
             {
                 {"flow_threshold", "THRESH"},
                 {"flow_accumRate", "ACCUM"},
                 {"flow_valveType", "VALVE"},
                 {"flow_vesselSize", "VESSEL"},
                 {"flow_strainColor", "STRAIN"},
                 {"flow_releaseTime", "RELTIME"},
                 {"flow_whistlePitch", "WHISTLE"},
                 // Macros
                 {"flow_macroCharacter", "VESSEL"},
                 {"flow_macroMovement", "CURRENT"},
                 {"flow_macroCoupling", "VALVE"},
                 {"flow_macroSpace", "DEEP"},
             }},

            //------------------------------------------------------------------
            // OVERTONE (prefix: over_)  — Spectral Ice #A8D8EA
            // Continued fraction convergent synthesis (B028: π, e, φ, √2)
            //------------------------------------------------------------------
            {"overtone",
             {
                 {"over_constant", "CNSTANT"},
                 {"over_depth", "CF DPTH"},
                 {"over_velBright", "VELBRT"},
                 {"over_resoMix", "RESONX"},
                 // Macros — Overtone uses depth/color
                 {"over_macroDepth", "DEPTH"},
                 {"over_macroColor", "COLOR"},
                 {"over_macroCoupling", "PARTIAL"},
                 {"over_macroSpace", "SPECTRA"},
             }},

            //------------------------------------------------------------------
            // OVERWASH (prefix: wash_)  — Tide Foam White #F0F8FF
            // Multi-scale diffusion (Kitchen Collection, Broth quad)
            //------------------------------------------------------------------
            {"overwash",
             {
                 {"wash_diffusionRate", "DIFFUSE"},
                 {"wash_viscosity", "VISCOUS"},
                 {"wash_harmonics", "HARMONI"},
                 {"wash_spreadMax", "SPREAD"},
                 {"wash_diffusionTime", "DIFFTM"},
                 {"wash_interference", "INTRFR"},
                 // Macros
                 {"wash_macroCharacter", "TIDE"},
                 {"wash_macroMovement", "WASH"},
                 {"wash_macroCoupling", "DIFFUSE"},
                 {"wash_macroSpace", "FOAM"},
             }},

            //------------------------------------------------------------------
            // OVERWORN (prefix: worn_)  — Worn Felt Grey #808080
            // Caramelization / thermal reduction (Kitchen Collection, Broth quad)
            //------------------------------------------------------------------
            {"overworn",
             {
                 {"worn_reductionRate", "REDUCE"},
                 {"worn_heat", "HEAT"},
                 {"worn_richness", "RICHNS"},
                 {"worn_maillard", "MAILLRD"},
                 {"worn_umamiDepth", "UMAMI"},
                 {"worn_concentrate", "CONCNTR"},
                 {"worn_sessionTarget", "TARGET"},
                 {"worn_sessionAge", "AGE"},
                 {"worn_stateReset", "RESET"},
                 // Macros
                 {"worn_macroCharacter", "HEAT"},
                 {"worn_macroMovement", "REDUCE"},
                 {"worn_macroCoupling", "UMAMI"},
                 {"worn_macroSpace", "CARAMEL"},
             }},

            //------------------------------------------------------------------
            // OVERWORLD (prefix: ow_)  — Neon Green #39FF14
            // Chip synth (NES/Genesis/SNES), ERA Triangle
            //------------------------------------------------------------------
            {"overworld",
             {
                 {"ow_era", "ERA"},
                 // Macros -- keyed on actual registered param IDs (fixes #305).
                 // Overworld registers ow_macroEra/Crush/Glitch/Space, not the
                 // generic macroCharacter/Movement/Coupling/Space names.
                 {"ow_macroEra", "ERA"},       // M1 CHARACTER
                 {"ow_macroCrush", "CRUSH"},   // M2 MOVEMENT
                 {"ow_macroGlitch", "GLITCH"}, // M3 COUPLING
                 {"ow_macroSpace", "SPACE"},   // M4 SPACE
             }},

            //------------------------------------------------------------------
            // OAKEN (prefix: oaken_)  — Upright Oak #9C6B30
            // Upright bass (Kitchen Collection, Cellar quad)
            //------------------------------------------------------------------
            {"oaken",
             {
                 {"oaken_exciter", "EXCITE"},
                 {"oaken_bowPressure", "BOW"},
                 {"oaken_stringTension", "TENSN"},
                 {"oaken_woodAge", "WOOD AGE"},
                 {"oaken_room", "ROOM"},
                 {"oaken_curingRate", "CURING"},
                 // Macros
                 {"oaken_macroCharacter", "OAK"},
                 {"oaken_macroMovement", "BOW"},
                 {"oaken_macroCoupling", "GRAVITY"},
                 {"oaken_macroSpace", "ROOM"},
             }},

            //------------------------------------------------------------------
            // OCHRE (prefix: ochre_)  — Ochre Pigment #CC7722
            // Prepared/extended piano (Kitchen Collection, Kitchen quad)
            //------------------------------------------------------------------
            {"ochre",
             {
                 {"ochre_conductivity", "CONDUCT"},
                 {"ochre_bodyType", "BODY"},
                 {"ochre_caramel", "CARAMEL"},
                 {"ochre_sympathy", "SYMPATH"},
                 {"ochre_hfCharacter", "HFCHAR"},
                 {"ochre_thermalDrift", "THERMAL"},
                 // Macros
                 {"ochre_macroCharacter", "OCHRE"},
                 {"ochre_macroMovement", "THERMAL"},
                 {"ochre_macroCoupling", "SYMPATH"},
                 {"ochre_macroSpace", "HALL"},
             }},

            //------------------------------------------------------------------
            // ODYSSEY (prefix: drift_)  — Violet #7B2D8B
            // Dual oscillator drift synth
            //------------------------------------------------------------------
            {"odyssey",
             {
                 {"drift_oscA_mode", "OSC A"},
                 {"drift_oscA_fmDepth", "FM A DP"},
                 {"drift_oscB_fmDepth", "FM B DP"},
                 {"drift_hazeAmount", "HAZE"},
                 {"drift_formantMorph", "FRMMORPH"},
                 {"drift_formantMix", "FRMMIX"},
                 {"drift_shimmerAmount", "SHIMMER"},
                 {"drift_shimmerTone", "SHMMTNE"},
                 {"drift_tidalDepth", "TIDAL"},
                 {"drift_tidalRate", "TIDALRT"},
                 {"drift_fractureEnable", "FRACTUR"},
                 {"drift_fractureIntensity", "FRCINT"},
                 {"drift_fractureRate", "FRCRT"},
                 // Macros
                 {"drift_macroCharacter", "DRIFT"},
                 {"drift_macroMovement", "HAZE"},
                 {"drift_macroCoupling", "TIDAL"},
                 {"drift_macroSpace", "ODYSSEY"},
             }},

            //------------------------------------------------------------------
            // OPERA (prefix: opera_)  — already defined above
            //------------------------------------------------------------------

            //------------------------------------------------------------------
            // OPTIC (prefix: optic_)  — already defined above
            //------------------------------------------------------------------

            //------------------------------------------------------------------
            // ORBITAL (prefix: orb_)  — Warm Red #FF6B6B
            // Group envelope additive synth (B001)
            //------------------------------------------------------------------
            {"orbital",
             {
                 {"orb_profileA", "PROF A"},
                 {"orb_profileB", "PROF B"},
                 {"orb_morph", "MORPH"},
                 {"orb_oddEven", "ODD/EVN"},
                 {"orb_formantShape", "FRMSHP"},
                 {"orb_formantShift", "FRMSHFT"},
                 {"orb_inharm", "INHARM"},
                 {"orb_fmIndex", "FM IDX"},
                 {"orb_fmRatio", "FM RT"},
                 {"orb_groupAttack1", "GRP ATK1"},
                 {"orb_groupDecay1", "GRP DCY1"},
                 {"orb_groupAttack2", "GRP ATK2"},
                 {"orb_groupDecay2", "GRP DCY2"},
                 {"orb_groupAttack3", "GRP ATK3"},
                 {"orb_groupDecay3", "GRP DCY3"},
                 {"orb_groupAttack4", "GRP ATK4"},
                 {"orb_groupDecay4", "GRP DCY4"},
                 {"orb_filterEnvDepth", "FLTENV"},
                 {"orb_saturation", "SAT"},
                 // Macros
                 {"orb_macroSpectrum", "SPECTRM"},
                 {"orb_macroEvolve", "EVOLVE"},
                 {"orb_macroCoupling", "ORBIT"},
                 {"orb_macroSpace", "SPACE"},
             }},

            //------------------------------------------------------------------
            // OXALIS (prefix: oxal_)  — Wood Sorrel Lilac #9B59B6
            // Fibonacci strings (Kitchen Collection, Garden quad)
            //------------------------------------------------------------------
            {"oxalis",
             {
                 {"oxal_phi", "PHI"},
                 {"oxal_symmetry", "SYMMETRY"},
                 {"oxal_growthMode", "GROWTH"},
                 {"oxal_growthTime", "GRWTIME"},
                 // Macros
                 {"oxal_macroCharacter", "PHI"},
                 {"oxal_macroMovement", "GROWTH"},
                 {"oxal_macroCoupling", "SPIRAL"},
                 {"oxal_macroSpace", "MEADOW"},
             }},

            //------------------------------------------------------------------
            // OXBOW (prefix: oxb_)  — Oxbow Teal #1A6B5A
            // Entangled reverb synth (Chiasmus FDN + phase erosion)
            //------------------------------------------------------------------
            {"oxbow",
             {
                 {"oxb_entangle", "ENTANGL"},
                 {"oxb_chiasmus", "CHIASM"},
                 {"oxb_erosion", "ERODE"},
                 {"oxb_resonance", "GOLDEN"},
                 // Macros
                 {"oxb_macroCharacter", "ENTANGL"},
                 {"oxb_macroMovement", "ERODE"},
                 {"oxb_macroCoupling", "CHIASM"},
                 {"oxb_macroSpace", "SPACE"},
             }},

            //------------------------------------------------------------------
            // OXYTOCIN (prefix: oxy_)  — Synapse Violet #9B5DE5
            // Circuit-modeling love-triangle synth (fleet leader, 9.5/10)
            // Note Duration as Synthesis Parameter (B040)
            //------------------------------------------------------------------
            {"oxytocin",
             {
                 {"oxy_intimacy", "INTIMCY"},
                 {"oxy_passion", "PASSION"},
                 {"oxy_commitment", "COMMIT"},
                 {"oxy_warmth_rate", "WARMTH"},
                 {"oxy_passion_rate", "FERVOR"},
                 {"oxy_commit_rate", "BONDING"},
                 {"oxy_entanglement", "ENTANGL"},
                 {"oxy_circuit_age", "VINTAGE"},
                 {"oxy_circuit_noise", "HISS"},
                 {"oxy_memory_depth", "RECALL"},
                 {"oxy_memory_decay", "FORGET"},
                 {"oxy_topology", "TOPOLOGY"},
                 {"oxy_topology_lock", "LOCK"},
                 {"oxy_lfo_rate", "LFO1 RT"},
                 {"oxy_lfo_depth", "LFO1 DP"},
                 {"oxy_lfo_shape", "LFO1 SH"},
                 {"oxy_lfo2_rate", "LFO2 RT"},
                 {"oxy_lfo2_depth", "LFO2 DP"},
                 // Macros: Oxytocin has no oxy_macro* params (fixes #306).
                 // The four expressive controls above ARE the M1-M4 macros:
                 //   M1 CHARACTER = oxy_intimacy   (already mapped -> "INTIMCY")
                 //   M2 MOVEMENT  = oxy_passion    (already mapped -> "PASSION")
                 //   M3 COUPLING  = oxy_commitment (already mapped -> "COMMIT")
                 //   M4 SPACE     = oxy_entanglement (already mapped -> "ENTANGL")
             }},

            //------------------------------------------------------------------
            // OVEN (prefix: oven_)  — Steinway Ebony #1C1C1C
            // Grand piano physical model (Kitchen Collection, Kitchen quad)
            //------------------------------------------------------------------
            {"oven",
             {
                 {"oven_bodyResonance", "BODY"},
                 {"oven_hfAmount", "HF"},
                 {"oven_temperature", "TEMP"},
                 {"oven_sympathetic", "SYMPATH"},
                 {"oven_bloomTime", "BLOOM"},
                 {"oven_competition", "COMPET"},
                 {"oven_couplingResonance", "CPLRES"},
                 // Macros
                 {"oven_macroCharacter", "TOUCH"},
                 {"oven_macroMovement", "BLOOM"},
                 {"oven_macroCoupling", "RESONAT"},
                 {"oven_macroSpace", "HALL"},
             }},

            //------------------------------------------------------------------
            // OWARE (prefix: owr_)  — Akan Goldweight #B5883E
            // Tuned percussion (material continuum + mallet physics)
            //------------------------------------------------------------------
            {"oware",
             {
                 {"owr_material", "MATTER"},
                 {"owr_mallet", "STRIKE"},
                 {"owr_sympathetic", "RESONATE"},
                 {"owr_tuningMode", "TUNING"},
                 {"owr_buzzAmount", "BUZZ"},
                 // Macros
                 {"owr_macroCharacter", "MATTER"},
                 {"owr_macroMovement", "STRIKE"},
                 {"owr_macroCoupling", "SYMPATH"},
                 {"owr_macroSpace", "SPACE"},
             }},

            //------------------------------------------------------------------
            // OWLFISH (prefix: owl_)  — Abyssal Gold #B8860B
            // Mixtur-Trautonium + bioluminescent sub-harmonic engine
            // (triple MIX params, double THRESHOLD — all disambiguated)
            //------------------------------------------------------------------
            {"owlfish",
             {
                 {"owl_subMix", "SUBMIX"},          {"owl_fundWave", "FUNDWAV"},       {"owl_subWave", "SUBWAVE"},
                 {"owl_bodyFreq", "BODYFRQ"},       {"owl_bodyLevel", "BODYLVL"},      {"owl_compRatio", "CMPRAT"},
                 {"owl_compThreshold", "CMPTHRS"},  {"owl_compAttack", "CMPATTK"},     {"owl_compRelease", "CMPRLS"},
                 {"owl_filterEnvDepth", "ENVDPTH"}, {"owl_grainSize", "GRNSZ"},        {"owl_grainMix", "GRNMIX"},
                 {"owl_feedRate", "FEEDRT"},        {"owl_armorThreshold", "ARMRTHS"}, {"owl_armorDecay", "ARMRDCY"},
                 {"owl_reverbSize", "VERBSZ"},      {"owl_reverbPreDelay", "PREDLY"},  {"owl_reverbMix", "VERBMIX"},
                 {"owl_legatoMode", "LEGATO"},
             }},
        };
        return table;
    }

    JUCE_DECLARE_NON_COPYABLE(EngineVocabulary)
};

} // namespace xoceanus
