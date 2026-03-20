#pragma once
// XOverworld Parameters stub — build-only placeholder.
#include <vector>
#include <memory>
#include <string>
#include <juce_audio_processors/juce_audio_processors.h>

namespace xoverworld {

struct ParamSnapshot {
    float era = 0.0f, eraY = 0.0f;
    int   voiceMode = 0;
    float masterVol = 1.0f, masterTune = 0.0f;
    int   pulseDuty = 0; float pulseSweep = 0.0f;
    bool  triEnable = false;
    int   noiseMode = 0, noisePeriod = 0;
    bool  dpcmEnable = false; int dpcmRate = 0;
    float nesMix = 1.0f;
    int   fmAlgorithm = 0, fmFeedback = 0;
    int   fmOpLevel[4] = {}, fmOpMult[4] = {}, fmOpDetune[4] = {};
    int   fmAttack = 0, fmDecay = 0, fmSustain = 0, fmRelease = 0;
    int   fmLfoRate = 0, fmLfoDepth = 0;
    int   brrSample = 0, brrInterp = 0;
    int   snesAttack = 0, snesDecay = 0, snesSustain = 0, snesRelease = 0;
    bool  pitchMod = false, noiseReplace = false;
    int   echoDelay = 0; float echoFeedback = 0.0f, echoMix = 0.0f;
    float echoFir[8] = {};
    float glitchAmount = 0.0f; int glitchType = 0;
    float glitchRate = 0.0f, glitchDepth = 0.0f, glitchMix = 0.0f;
    float ampAttack = 0.01f, ampDecay = 0.1f, ampSustain = 0.8f, ampRelease = 0.2f;
    float filterCutoff = 8000.0f, filterReso = 0.5f; int filterType = 0;
    float crushBits = 16.0f; int crushRate = 1; float crushMix = 0.0f;
    float eraDriftRate = 0.0f, eraDriftDepth = 0.0f; int eraDriftShape = 0;
    float eraPortaTime = 0.0f, eraMemTime = 0.0f, eraMemMix = 0.0f;
    int   vertexA = 0, vertexB = 1, vertexC = 2;
    int   gbWaveSlot = 0, gbPulseDuty = 0, pceWaveSlot = 0;
};

namespace ParamID {
    inline const char* ERA           = "ow_era";
    inline const char* ERA_Y         = "ow_eraY";
    inline const char* VOICE_MODE    = "ow_voiceMode";
    inline const char* MASTER_VOL    = "ow_masterVol";
    inline const char* MASTER_TUNE   = "ow_masterTune";
    inline const char* PULSE_DUTY    = "ow_pulseDuty";
    inline const char* PULSE_SWEEP   = "ow_pulseSweep";
    inline const char* TRI_ENABLE    = "ow_triEnable";
    inline const char* NOISE_MODE    = "ow_noiseMode";
    inline const char* NOISE_PERIOD  = "ow_noisePeriod";
    inline const char* DPCM_ENABLE   = "ow_dpcmEnable";
    inline const char* DPCM_RATE     = "ow_dpcmRate";
    inline const char* NES_MIX       = "ow_nesMix";
    inline const char* FM_ALGORITHM  = "ow_fmAlgorithm";
    inline const char* FM_FEEDBACK   = "ow_fmFeedback";
    inline const char* FM_OP1_LEVEL  = "ow_fmOp1Level";
    inline const char* FM_OP2_LEVEL  = "ow_fmOp2Level";
    inline const char* FM_OP3_LEVEL  = "ow_fmOp3Level";
    inline const char* FM_OP4_LEVEL  = "ow_fmOp4Level";
    inline const char* FM_OP1_MULT   = "ow_fmOp1Mult";
    inline const char* FM_OP2_MULT   = "ow_fmOp2Mult";
    inline const char* FM_OP3_MULT   = "ow_fmOp3Mult";
    inline const char* FM_OP4_MULT   = "ow_fmOp4Mult";
    inline const char* FM_OP1_DETUNE = "ow_fmOp1Detune";
    inline const char* FM_OP2_DETUNE = "ow_fmOp2Detune";
    inline const char* FM_OP3_DETUNE = "ow_fmOp3Detune";
    inline const char* FM_OP4_DETUNE = "ow_fmOp4Detune";
    inline const char* FM_ATTACK     = "ow_fmAttack";
    inline const char* FM_DECAY      = "ow_fmDecay";
    inline const char* FM_SUSTAIN    = "ow_fmSustain";
    inline const char* FM_RELEASE    = "ow_fmRelease";
    inline const char* FM_LFO_RATE   = "ow_fmLfoRate";
    inline const char* FM_LFO_DEPTH  = "ow_fmLfoDepth";
    inline const char* BRR_SAMPLE    = "ow_brrSample";
    inline const char* BRR_INTERP    = "ow_brrInterp";
    inline const char* SNES_ATTACK   = "ow_snesAttack";
    inline const char* SNES_DECAY    = "ow_snesDecay";
    inline const char* SNES_SUSTAIN  = "ow_snesSustain";
    inline const char* SNES_RELEASE  = "ow_snesRelease";
    inline const char* PITCH_MOD     = "ow_pitchMod";
    inline const char* NOISE_REPLACE = "ow_noiseReplace";
    inline const char* ECHO_DELAY    = "ow_echoDelay";
    inline const char* ECHO_FEEDBACK = "ow_echoFeedback";
    inline const char* ECHO_MIX      = "ow_echoMix";
    inline const char* GLITCH_AMOUNT = "ow_glitchAmount";
    inline const char* GLITCH_TYPE   = "ow_glitchType";
    inline const char* GLITCH_RATE   = "ow_glitchRate";
    inline const char* GLITCH_DEPTH  = "ow_glitchDepth";
    inline const char* GLITCH_MIX    = "ow_glitchMix";
    inline const char* AMP_ATTACK    = "ow_ampAttack";
    inline const char* AMP_DECAY     = "ow_ampDecay";
    inline const char* AMP_SUSTAIN   = "ow_ampSustain";
    inline const char* AMP_RELEASE   = "ow_ampRelease";
    inline const char* FILTER_CUTOFF = "ow_filterCutoff";
    inline const char* FILTER_RESO   = "ow_filterReso";
    inline const char* FILTER_TYPE   = "ow_filterType";
    inline const char* CRUSH_BITS    = "ow_crushBits";
    inline const char* CRUSH_RATE    = "ow_crushRate";
    inline const char* CRUSH_MIX     = "ow_crushMix";
    inline const char* ERA_DRIFT_RATE  = "ow_eraDriftRate";
    inline const char* ERA_DRIFT_DEPTH = "ow_eraDriftDepth";
    inline const char* ERA_DRIFT_SHAPE = "ow_eraDriftShape";
    inline const char* ERA_PORTA_TIME  = "ow_eraPortaTime";
    inline const char* ERA_MEM_TIME    = "ow_eraMemTime";
    inline const char* ERA_MEM_MIX     = "ow_eraMemMix";
    inline const char* VERTEX_A       = "ow_vertexA";
    inline const char* VERTEX_B       = "ow_vertexB";
    inline const char* VERTEX_C       = "ow_vertexC";
    inline const char* GB_WAVE_SLOT   = "ow_gbWaveSlot";
    inline const char* GB_PULSE_DUTY  = "ow_gbPulseDuty";
    inline const char* PCE_WAVE_SLOT  = "ow_pceWaveSlot";
} // namespace ParamID

inline void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
{
    auto add = [&](const char* id, const char* name, float lo, float hi, float def) {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float>(lo, hi, 0.01f), def));
    };
    add(ParamID::ERA,           "Overworld ERA",           0.0f, 1.0f, 0.5f);
    add(ParamID::ERA_Y,         "Overworld ERA Y",         0.0f, 1.0f, 0.5f);
    add(ParamID::VOICE_MODE,    "Overworld Voice Mode",    0.0f, 3.0f, 0.0f);
    add(ParamID::MASTER_VOL,    "Overworld Master Vol",    0.0f, 1.0f, 0.8f);
    add(ParamID::MASTER_TUNE,   "Overworld Master Tune",  -1.0f, 1.0f, 0.0f);
    add(ParamID::PULSE_DUTY,    "Overworld Pulse Duty",    0.0f, 3.0f, 2.0f);
    add(ParamID::PULSE_SWEEP,   "Overworld Pulse Sweep",   0.0f, 1.0f, 0.0f);
    add(ParamID::TRI_ENABLE,    "Overworld Tri Enable",    0.0f, 1.0f, 1.0f);
    add(ParamID::NOISE_MODE,    "Overworld Noise Mode",    0.0f, 1.0f, 0.0f);
    add(ParamID::NOISE_PERIOD,  "Overworld Noise Period",  0.0f, 15.0f, 7.0f);
    add(ParamID::DPCM_ENABLE,   "Overworld DPCM Enable",   0.0f, 1.0f, 0.0f);
    add(ParamID::DPCM_RATE,     "Overworld DPCM Rate",     0.0f, 15.0f, 0.0f);
    add(ParamID::NES_MIX,       "Overworld NES Mix",       0.0f, 1.0f, 1.0f);
    add(ParamID::FM_ALGORITHM,  "Overworld FM Algorithm",  0.0f, 7.0f, 0.0f);
    add(ParamID::FM_FEEDBACK,   "Overworld FM Feedback",   0.0f, 7.0f, 0.0f);
    add(ParamID::FM_OP1_LEVEL,  "Overworld FM Op1 Level",  0.0f, 127.0f, 100.0f);
    add(ParamID::FM_OP2_LEVEL,  "Overworld FM Op2 Level",  0.0f, 127.0f, 80.0f);
    add(ParamID::FM_OP3_LEVEL,  "Overworld FM Op3 Level",  0.0f, 127.0f, 60.0f);
    add(ParamID::FM_OP4_LEVEL,  "Overworld FM Op4 Level",  0.0f, 127.0f, 40.0f);
    add(ParamID::FM_OP1_MULT,   "Overworld FM Op1 Mult",   1.0f, 15.0f, 1.0f);
    add(ParamID::FM_OP2_MULT,   "Overworld FM Op2 Mult",   1.0f, 15.0f, 2.0f);
    add(ParamID::FM_OP3_MULT,   "Overworld FM Op3 Mult",   1.0f, 15.0f, 3.0f);
    add(ParamID::FM_OP4_MULT,   "Overworld FM Op4 Mult",   1.0f, 15.0f, 1.0f);
    add(ParamID::FM_OP1_DETUNE, "Overworld FM Op1 Detune", 0.0f, 7.0f, 0.0f);
    add(ParamID::FM_OP2_DETUNE, "Overworld FM Op2 Detune", 0.0f, 7.0f, 0.0f);
    add(ParamID::FM_OP3_DETUNE, "Overworld FM Op3 Detune", 0.0f, 7.0f, 0.0f);
    add(ParamID::FM_OP4_DETUNE, "Overworld FM Op4 Detune", 0.0f, 7.0f, 0.0f);
    add(ParamID::FM_ATTACK,     "Overworld FM Attack",     0.0f, 31.0f, 5.0f);
    add(ParamID::FM_DECAY,      "Overworld FM Decay",      0.0f, 15.0f, 4.0f);
    add(ParamID::FM_SUSTAIN,    "Overworld FM Sustain",    0.0f, 15.0f, 8.0f);
    add(ParamID::FM_RELEASE,    "Overworld FM Release",    0.0f, 15.0f, 4.0f);
    add(ParamID::FM_LFO_RATE,   "Overworld FM LFO Rate",   0.0f, 7.0f, 0.0f);
    add(ParamID::FM_LFO_DEPTH,  "Overworld FM LFO Depth",  0.0f, 127.0f, 0.0f);
    add(ParamID::BRR_SAMPLE,    "Overworld BRR Sample",    0.0f, 31.0f, 0.0f);
    add(ParamID::BRR_INTERP,    "Overworld BRR Interp",    0.0f, 3.0f, 1.0f);
    add(ParamID::SNES_ATTACK,   "Overworld SNES Attack",   0.0f, 15.0f, 7.0f);
    add(ParamID::SNES_DECAY,    "Overworld SNES Decay",    0.0f, 7.0f, 3.0f);
    add(ParamID::SNES_SUSTAIN,  "Overworld SNES Sustain",  0.0f, 7.0f, 6.0f);
    add(ParamID::SNES_RELEASE,  "Overworld SNES Release",  0.0f, 31.0f, 10.0f);
    add(ParamID::PITCH_MOD,     "Overworld Pitch Mod",     0.0f, 1.0f, 0.0f);
    add(ParamID::NOISE_REPLACE, "Overworld Noise Replace", 0.0f, 1.0f, 0.0f);
    add(ParamID::ECHO_DELAY,    "Overworld Echo Delay",    0.0f, 15.0f, 3.0f);
    add(ParamID::ECHO_FEEDBACK, "Overworld Echo Feedback", 0.0f, 1.0f, 0.5f);
    add(ParamID::ECHO_MIX,      "Overworld Echo Mix",      0.0f, 1.0f, 0.3f);
    for (int i = 0; i < 8; ++i) {
        std::string id = std::string("ow_echoFir") + std::to_string(i);
        std::string nm = std::string("Overworld Echo FIR ") + std::to_string(i);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id, 1 }, nm,
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), (i == 0) ? 0.7f : 0.0f));
    }
    add(ParamID::GLITCH_AMOUNT, "Overworld Glitch Amount", 0.0f, 1.0f, 0.0f);
    add(ParamID::GLITCH_TYPE,   "Overworld Glitch Type",   0.0f, 12.0f, 0.0f);
    add(ParamID::GLITCH_RATE,   "Overworld Glitch Rate",   0.0f, 1.0f, 0.3f);
    add(ParamID::GLITCH_DEPTH,  "Overworld Glitch Depth",  0.0f, 1.0f, 0.5f);
    add(ParamID::GLITCH_MIX,    "Overworld Glitch Mix",    0.0f, 1.0f, 0.0f);
    add(ParamID::AMP_ATTACK,    "Overworld Amp Attack",    0.001f, 4.0f, 0.01f);
    add(ParamID::AMP_DECAY,     "Overworld Amp Decay",     0.001f, 4.0f, 0.1f);
    add(ParamID::AMP_SUSTAIN,   "Overworld Amp Sustain",   0.0f, 1.0f, 0.8f);
    add(ParamID::AMP_RELEASE,   "Overworld Amp Release",   0.001f, 4.0f, 0.2f);
    add(ParamID::FILTER_CUTOFF, "Overworld Filter Cutoff", 20.0f, 20000.0f, 8000.0f);
    add(ParamID::FILTER_RESO,   "Overworld Filter Reso",   0.0f, 1.0f, 0.3f);
    add(ParamID::FILTER_TYPE,   "Overworld Filter Type",   0.0f, 3.0f, 0.0f);
    add(ParamID::CRUSH_BITS,    "Overworld Crush Bits",    1.0f, 16.0f, 16.0f);
    add(ParamID::CRUSH_RATE,    "Overworld Crush Rate",    1.0f, 32.0f, 1.0f);
    add(ParamID::CRUSH_MIX,     "Overworld Crush Mix",     0.0f, 1.0f, 0.0f);
    add(ParamID::ERA_DRIFT_RATE,  "Overworld ERA Drift Rate",  0.0f, 4.0f, 0.0f);
    add(ParamID::ERA_DRIFT_DEPTH, "Overworld ERA Drift Depth", 0.0f, 1.0f, 0.0f);
    add(ParamID::ERA_DRIFT_SHAPE, "Overworld ERA Drift Shape", 0.0f, 3.0f, 0.0f);
    add(ParamID::ERA_PORTA_TIME,  "Overworld ERA Porta Time",  0.0f, 2.0f, 0.0f);
    add(ParamID::ERA_MEM_TIME,    "Overworld ERA Mem Time",    0.0f, 4.0f, 0.5f);
    add(ParamID::ERA_MEM_MIX,     "Overworld ERA Mem Mix",     0.0f, 1.0f, 0.2f);
    add(ParamID::VERTEX_A,      "Overworld Vertex A",      0.0f, 5.0f, 0.0f);
    add(ParamID::VERTEX_B,      "Overworld Vertex B",      0.0f, 5.0f, 1.0f);
    add(ParamID::VERTEX_C,      "Overworld Vertex C",      0.0f, 5.0f, 2.0f);
    add(ParamID::GB_WAVE_SLOT,  "Overworld GB Wave Slot",  0.0f, 7.0f, 0.0f);
    add(ParamID::GB_PULSE_DUTY, "Overworld GB Pulse Duty", 0.0f, 3.0f, 0.0f);
    add(ParamID::PCE_WAVE_SLOT, "Overworld PCE Wave Slot", 0.0f, 7.0f, 0.0f);
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    addParameters(params);
    return juce::AudioProcessorValueTreeState::ParameterLayout(
        std::make_move_iterator(params.begin()),
        std::make_move_iterator(params.end()));
}

} // namespace xoverworld
