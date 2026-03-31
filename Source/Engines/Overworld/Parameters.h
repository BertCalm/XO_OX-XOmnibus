#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include <memory>

//==============================================================================
// XOverworld — Parameter IDs, ParamSnapshot, and parameter registration.
//
// Self-contained copy housed in Source/Engines/Overworld/ so XOlokun does not
// need the sibling XOverworld repository on disk.
//
// Parameter prefix: ow_
// 6 chip engines: NES (2A03), FM Genesis (YM2612), SNES SPC700,
//                 Game Boy, PC Engine, Neo Geo
//==============================================================================

namespace xoverworld {

//------------------------------------------------------------------------------
// ParamID — compile-time string constants for every APVTS parameter ID.
//------------------------------------------------------------------------------
namespace ParamID {
    // Core / ERA
    static constexpr const char* ERA            = "ow_era";
    static constexpr const char* ERA_Y          = "ow_eraY";
    static constexpr const char* VOICE_MODE     = "ow_voiceMode";
    static constexpr const char* MASTER_VOL     = "ow_masterVol";
    static constexpr const char* MASTER_TUNE    = "ow_masterTune";
    static constexpr const char* VERTEX_A       = "ow_vertexA";
    static constexpr const char* VERTEX_B       = "ow_vertexB";
    static constexpr const char* VERTEX_C       = "ow_vertexC";

    // ERA extended
    static constexpr const char* ERA_DRIFT_RATE  = "ow_eraDriftRate";
    static constexpr const char* ERA_DRIFT_DEPTH = "ow_eraDriftDepth";
    static constexpr const char* ERA_DRIFT_SHAPE = "ow_eraDriftShape";
    static constexpr const char* ERA_PORTA_TIME  = "ow_eraPortaTime";
    static constexpr const char* ERA_MEM_TIME    = "ow_eraMemTime";
    static constexpr const char* ERA_MEM_MIX     = "ow_eraMemMix";

    // NES
    static constexpr const char* PULSE_DUTY     = "ow_pulseDuty";
    static constexpr const char* PULSE_SWEEP    = "ow_pulseSweep";
    static constexpr const char* TRI_ENABLE     = "ow_triEnable";
    static constexpr const char* NOISE_MODE     = "ow_noiseMode";
    static constexpr const char* NOISE_PERIOD   = "ow_noisePeriod";
    static constexpr const char* DPCM_ENABLE    = "ow_dpcmEnable";
    static constexpr const char* DPCM_RATE      = "ow_dpcmRate";
    static constexpr const char* NES_MIX        = "ow_nesMix";

    // FM (YM2612)
    static constexpr const char* FM_ALGORITHM   = "ow_fmAlgorithm";
    static constexpr const char* FM_FEEDBACK    = "ow_fmFeedback";
    static constexpr const char* FM_OP1_LEVEL   = "ow_fmOp1Level";
    static constexpr const char* FM_OP2_LEVEL   = "ow_fmOp2Level";
    static constexpr const char* FM_OP3_LEVEL   = "ow_fmOp3Level";
    static constexpr const char* FM_OP4_LEVEL   = "ow_fmOp4Level";
    static constexpr const char* FM_OP1_MULT    = "ow_fmOp1Mult";
    static constexpr const char* FM_OP2_MULT    = "ow_fmOp2Mult";
    static constexpr const char* FM_OP3_MULT    = "ow_fmOp3Mult";
    static constexpr const char* FM_OP4_MULT    = "ow_fmOp4Mult";
    static constexpr const char* FM_OP1_DETUNE  = "ow_fmOp1Detune";
    static constexpr const char* FM_OP2_DETUNE  = "ow_fmOp2Detune";
    static constexpr const char* FM_OP3_DETUNE  = "ow_fmOp3Detune";
    static constexpr const char* FM_OP4_DETUNE  = "ow_fmOp4Detune";
    static constexpr const char* FM_ATTACK      = "ow_fmAttack";
    static constexpr const char* FM_DECAY       = "ow_fmDecay";
    static constexpr const char* FM_SUSTAIN     = "ow_fmSustain";
    static constexpr const char* FM_RELEASE     = "ow_fmRelease";
    static constexpr const char* FM_LFO_RATE    = "ow_fmLfoRate";
    static constexpr const char* FM_LFO_DEPTH   = "ow_fmLfoDepth";

    // SNES SPC700
    static constexpr const char* BRR_SAMPLE     = "ow_brrSample";
    static constexpr const char* BRR_INTERP     = "ow_brrInterp";
    static constexpr const char* SNES_ATTACK    = "ow_snesAttack";
    static constexpr const char* SNES_DECAY     = "ow_snesDecay";
    static constexpr const char* SNES_SUSTAIN   = "ow_snesSustain";
    static constexpr const char* SNES_RELEASE   = "ow_snesRelease";
    static constexpr const char* PITCH_MOD      = "ow_pitchMod";
    static constexpr const char* NOISE_REPLACE  = "ow_noiseReplace";

    // Echo / FIR (SNES echo DSP)
    static constexpr const char* ECHO_DELAY     = "ow_echoDelay";
    static constexpr const char* ECHO_FEEDBACK  = "ow_echoFeedback";
    static constexpr const char* ECHO_MIX       = "ow_echoMix";

    // Glitch
    static constexpr const char* GLITCH_AMOUNT  = "ow_glitchAmount";
    static constexpr const char* GLITCH_TYPE    = "ow_glitchType";
    static constexpr const char* GLITCH_RATE    = "ow_glitchRate";
    static constexpr const char* GLITCH_DEPTH   = "ow_glitchDepth";
    static constexpr const char* GLITCH_MIX     = "ow_glitchMix";

    // Amp envelope
    static constexpr const char* AMP_ATTACK     = "ow_ampAttack";
    static constexpr const char* AMP_DECAY      = "ow_ampDecay";
    static constexpr const char* AMP_SUSTAIN    = "ow_ampSustain";
    static constexpr const char* AMP_RELEASE    = "ow_ampRelease";

    // Filter
    static constexpr const char* FILTER_CUTOFF  = "ow_filterCutoff";
    static constexpr const char* FILTER_RESO    = "ow_filterReso";
    static constexpr const char* FILTER_TYPE    = "ow_filterType";

    // BitCrusher
    static constexpr const char* CRUSH_BITS     = "ow_crushBits";
    static constexpr const char* CRUSH_RATE     = "ow_crushRate";
    static constexpr const char* CRUSH_MIX      = "ow_crushMix";

    // Game Boy
    static constexpr const char* GB_WAVE_SLOT   = "ow_gbWaveSlot";
    static constexpr const char* GB_PULSE_DUTY  = "ow_gbPulseDuty";

    // PC Engine
    static constexpr const char* PCE_WAVE_SLOT  = "ow_pceWaveSlot";
} // namespace ParamID

//------------------------------------------------------------------------------
// ParamSnapshot — a plain struct capturing all parameter values for one block.
// Populated by OverworldEngine::buildSnapshot() from cached atomic pointers.
//------------------------------------------------------------------------------
struct ParamSnapshot
{
    // Core / ERA
    float era           = 0.5f;
    float eraY          = 0.5f;
    int   voiceMode     = 0;
    float masterVol     = 0.8f;
    float masterTune    = 0.0f;
    int   vertexA       = 0;  // chip engine index at vertex A
    int   vertexB       = 1;  // chip engine index at vertex B
    int   vertexC       = 2;  // chip engine index at vertex C

    // ERA extended
    float eraDriftRate   = 0.1f;
    float eraDriftDepth  = 0.0f;
    int   eraDriftShape  = 0;
    float eraPortaTime   = 0.05f;
    float eraMemTime     = 0.5f;
    float eraMemMix      = 0.0f;

    // NES
    int   pulseDuty     = 2;   // 0=12.5% 1=25% 2=50% 3=75%
    float pulseSweep    = 0.0f;
    bool  triEnable     = true;
    int   noiseMode     = 0;   // 0=periodic 1=random
    int   noisePeriod   = 8;
    bool  dpcmEnable    = false;
    int   dpcmRate      = 0;
    float nesMix        = 0.5f;

    // FM (YM2612) — operators 0-3
    int   fmAlgorithm   = 4;
    int   fmFeedback    = 0;
    int   fmOpLevel[4]  = {31, 31, 31, 31};
    int   fmOpMult[4]   = { 1,  1,  2,  4};
    int   fmOpDetune[4] = { 3,  3,  3,  3};
    int   fmAttack      = 31;
    int   fmDecay       = 8;
    int   fmSustain     = 8;
    int   fmRelease     = 4;
    int   fmLfoRate     = 0;
    int   fmLfoDepth    = 0;

    // SNES SPC700
    int   brrSample     = 0;
    int   brrInterp     = 1;   // 0=none 1=linear 2=gaussian
    int   snesAttack    = 12;
    int   snesDecay     = 6;
    int   snesSustain   = 14;
    int   snesRelease   = 4;
    bool  pitchMod      = false;
    bool  noiseReplace  = false;

    // Echo / FIR
    int   echoDelay     = 4;   // taps × 16ms SNES units
    float echoFeedback  = 0.4f;
    float echoMix       = 0.3f;
    float echoFir[8]    = {0.7f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};

    // Glitch
    float glitchAmount  = 0.0f;
    int   glitchType    = 0;
    float glitchRate    = 1.0f;
    float glitchDepth   = 0.5f;
    float glitchMix     = 0.0f;

    // Amp envelope
    float ampAttack     = 0.005f;
    float ampDecay      = 0.1f;
    float ampSustain    = 0.7f;
    float ampRelease    = 0.2f;

    // Filter
    float filterCutoff  = 8000.0f;
    float filterReso    = 0.3f;
    int   filterType    = 0;   // 0=LP 1=HP 2=BP

    // BitCrusher
    float crushBits     = 16.0f;
    int   crushRate     = 1;   // sample rate divisor
    float crushMix      = 0.0f;

    // Game Boy
    int   gbWaveSlot    = 0;
    int   gbPulseDuty   = 2;

    // PC Engine
    int   pceWaveSlot   = 0;
};

//------------------------------------------------------------------------------
// addParameters — appends all ow_ parameters to the provided vector.
// Called from OverworldEngine::addParameters().
//
// IMPORTANT: ALL parameters are AudioParameterFloat — even those that model
// integer / boolean values. OverworldEngine::attachParameters() stores them as
// std::atomic<float>* via getRawParameterValue(), which only works for Float
// params. Integer quantisation and bool thresholds (> 0.5f) are applied in
// OverworldEngine::buildSnapshot().
//------------------------------------------------------------------------------
inline void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
{
    using P = juce::AudioParameterFloat;
    using PI = juce::ParameterID;
    using NR = juce::NormalisableRange<float>;

    // ERA
    params.push_back(std::make_unique<P>(PI{ParamID::ERA,        1}, "ERA X",      NR(0.f,1.f,0.001f), 0.5f));
    params.push_back(std::make_unique<P>(PI{ParamID::ERA_Y,      1}, "ERA Y",      NR(0.f,1.f,0.001f), 0.5f));
    params.push_back(std::make_unique<P>(PI{ParamID::VOICE_MODE, 1}, "Voice Mode", NR(0.f,2.f,1.f),    0.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::MASTER_VOL, 1}, "Master Vol", NR(0.f,1.f,0.001f), 0.8f));
    params.push_back(std::make_unique<P>(PI{ParamID::MASTER_TUNE,1}, "Master Tune",NR(-2.f,2.f,0.001f),0.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::VERTEX_A,   1}, "Vertex A",   NR(0.f,5.f,1.f),    0.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::VERTEX_B,   1}, "Vertex B",   NR(0.f,5.f,1.f),    1.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::VERTEX_C,   1}, "Vertex C",   NR(0.f,5.f,1.f),    2.0f));

    // ERA extended
    params.push_back(std::make_unique<P>(PI{ParamID::ERA_DRIFT_RATE, 1}, "ERA Drift Rate",  NR(0.f,10.f,0.001f), 0.1f));
    params.push_back(std::make_unique<P>(PI{ParamID::ERA_DRIFT_DEPTH,1}, "ERA Drift Depth", NR(0.f,1.f,0.001f),  0.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::ERA_DRIFT_SHAPE,1}, "ERA Drift Shape", NR(0.f,3.f,1.f),     0.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::ERA_PORTA_TIME, 1}, "ERA Porta Time",  NR(0.f,2.f,0.001f),  0.05f));
    params.push_back(std::make_unique<P>(PI{ParamID::ERA_MEM_TIME,   1}, "ERA Mem Time",    NR(0.f,4.f,0.001f),  0.5f));
    params.push_back(std::make_unique<P>(PI{ParamID::ERA_MEM_MIX,    1}, "ERA Mem Mix",     NR(0.f,1.f,0.001f),  0.0f));

    // NES
    params.push_back(std::make_unique<P>(PI{ParamID::PULSE_DUTY,  1}, "Pulse Duty",  NR(0.f,3.f,1.f),     2.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::PULSE_SWEEP, 1}, "Pulse Sweep", NR(-1.f,1.f,0.001f), 0.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::TRI_ENABLE,  1}, "Tri Enable",  NR(0.f,1.f,1.f),     1.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::NOISE_MODE,  1}, "Noise Mode",  NR(0.f,1.f,1.f),     0.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::NOISE_PERIOD,1}, "Noise Period",NR(0.f,15.f,1.f),    8.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::DPCM_ENABLE, 1}, "DPCM Enable", NR(0.f,1.f,1.f),    0.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::DPCM_RATE,   1}, "DPCM Rate",   NR(0.f,15.f,1.f),   0.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::NES_MIX,     1}, "NES Mix",     NR(0.f,1.f,0.001f), 0.5f));

    // FM
    params.push_back(std::make_unique<P>(PI{ParamID::FM_ALGORITHM, 1}, "FM Algorithm", NR(0.f,7.f,1.f),   4.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_FEEDBACK,  1}, "FM Feedback",  NR(0.f,7.f,1.f),   0.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_OP1_LEVEL, 1}, "FM Op1 Level", NR(0.f,31.f,1.f),  31.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_OP2_LEVEL, 1}, "FM Op2 Level", NR(0.f,31.f,1.f),  31.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_OP3_LEVEL, 1}, "FM Op3 Level", NR(0.f,31.f,1.f),  31.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_OP4_LEVEL, 1}, "FM Op4 Level", NR(0.f,31.f,1.f),  31.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_OP1_MULT,  1}, "FM Op1 Mult",  NR(1.f,15.f,1.f),  1.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_OP2_MULT,  1}, "FM Op2 Mult",  NR(1.f,15.f,1.f),  1.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_OP3_MULT,  1}, "FM Op3 Mult",  NR(1.f,15.f,1.f),  2.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_OP4_MULT,  1}, "FM Op4 Mult",  NR(1.f,15.f,1.f),  4.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_OP1_DETUNE,1}, "FM Op1 Detune",NR(0.f,7.f,1.f),   3.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_OP2_DETUNE,1}, "FM Op2 Detune",NR(0.f,7.f,1.f),   3.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_OP3_DETUNE,1}, "FM Op3 Detune",NR(0.f,7.f,1.f),   3.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_OP4_DETUNE,1}, "FM Op4 Detune",NR(0.f,7.f,1.f),   3.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_ATTACK,    1}, "FM Attack",    NR(0.f,31.f,1.f),  31.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_DECAY,     1}, "FM Decay",     NR(0.f,31.f,1.f),  8.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_SUSTAIN,   1}, "FM Sustain",   NR(0.f,15.f,1.f),  8.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_RELEASE,   1}, "FM Release",   NR(0.f,15.f,1.f),  4.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_LFO_RATE,  1}, "FM LFO Rate",  NR(0.f,7.f,1.f),   0.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::FM_LFO_DEPTH, 1}, "FM LFO Depth", NR(0.f,127.f,1.f), 0.0f));

    // SNES
    params.push_back(std::make_unique<P>(PI{ParamID::BRR_SAMPLE,   1}, "BRR Sample",   NR(0.f,15.f,1.f),  0.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::BRR_INTERP,   1}, "BRR Interp",   NR(0.f,2.f,1.f),   1.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::SNES_ATTACK,  1}, "SNES Attack",  NR(0.f,15.f,1.f),  12.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::SNES_DECAY,   1}, "SNES Decay",   NR(0.f,7.f,1.f),   6.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::SNES_SUSTAIN, 1}, "SNES Sustain", NR(0.f,31.f,1.f),  14.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::SNES_RELEASE, 1}, "SNES Release", NR(0.f,31.f,1.f),  4.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::PITCH_MOD,    1}, "Pitch Mod",    NR(0.f,1.f,1.f),   0.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::NOISE_REPLACE,1}, "Noise Replace",NR(0.f,1.f,1.f),   0.0f));

    // Echo
    params.push_back(std::make_unique<P>(PI{ParamID::ECHO_DELAY,   1}, "Echo Delay",   NR(0.f,15.f,1.f),  4.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::ECHO_FEEDBACK, 1}, "Echo Feedback",NR(0.f,0.95f,0.001f), 0.4f));
    params.push_back(std::make_unique<P>(PI{ParamID::ECHO_MIX,      1}, "Echo Mix",     NR(0.f,1.f,0.001f), 0.3f));
    for (int i = 0; i < 8; ++i)
    {
        auto id   = "ow_echoFir" + std::to_string(i);
        auto name = "Echo FIR " + std::to_string(i);
        params.push_back(std::make_unique<P>(juce::ParameterID{id, 1}, name,
                         NR(-1.f, 1.f, 0.001f), i == 0 ? 0.7f : 0.0f));
    }

    // Glitch
    params.push_back(std::make_unique<P>(PI{ParamID::GLITCH_AMOUNT,1}, "Glitch Amount",NR(0.f,1.f,0.001f),  0.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::GLITCH_TYPE,  1}, "Glitch Type",  NR(0.f,12.f,1.f),    0.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::GLITCH_RATE,  1}, "Glitch Rate",  NR(0.01f,20.f,0.001f),1.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::GLITCH_DEPTH, 1}, "Glitch Depth", NR(0.f,1.f,0.001f),  0.5f));
    params.push_back(std::make_unique<P>(PI{ParamID::GLITCH_MIX,   1}, "Glitch Mix",   NR(0.f,1.f,0.001f),  0.0f));

    // Amp envelope
    params.push_back(std::make_unique<P>(PI{ParamID::AMP_ATTACK,  1}, "Amp Attack",   NR(0.001f,4.f,0.001f), 0.005f));
    params.push_back(std::make_unique<P>(PI{ParamID::AMP_DECAY,   1}, "Amp Decay",    NR(0.001f,4.f,0.001f), 0.1f));
    params.push_back(std::make_unique<P>(PI{ParamID::AMP_SUSTAIN, 1}, "Amp Sustain",  NR(0.f,1.f,0.001f),    0.7f));
    params.push_back(std::make_unique<P>(PI{ParamID::AMP_RELEASE, 1}, "Amp Release",  NR(0.001f,8.f,0.001f), 0.2f));

    // Filter
    params.push_back(std::make_unique<P>(PI{ParamID::FILTER_CUTOFF,1}, "Filter Cutoff",NR(20.f,20000.f,1.f,0.25f), 8000.f));
    params.push_back(std::make_unique<P>(PI{ParamID::FILTER_RESO,  1}, "Filter Reso",  NR(0.f,1.f,0.001f),    0.3f));
    params.push_back(std::make_unique<P>(PI{ParamID::FILTER_TYPE,  1}, "Filter Type",  NR(0.f,2.f,1.f),        0.0f));

    // BitCrusher
    params.push_back(std::make_unique<P>(PI{ParamID::CRUSH_BITS,  1}, "Crush Bits",   NR(4.f,16.f,0.01f),   16.f));
    params.push_back(std::make_unique<P>(PI{ParamID::CRUSH_RATE,  1}, "Crush Rate",   NR(1.f,32.f,1.f),      1.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::CRUSH_MIX,   1}, "Crush Mix",    NR(0.f,1.f,0.001f),    0.0f));

    // Game Boy
    params.push_back(std::make_unique<P>(PI{ParamID::GB_WAVE_SLOT, 1}, "GB Wave Slot", NR(0.f,31.f,1.f), 0.0f));
    params.push_back(std::make_unique<P>(PI{ParamID::GB_PULSE_DUTY,1}, "GB Pulse Duty",NR(0.f,3.f,1.f),  2.0f));

    // PC Engine
    params.push_back(std::make_unique<P>(PI{ParamID::PCE_WAVE_SLOT,1}, "PCE Wave Slot",NR(0.f,31.f,1.f), 0.0f));
}

//------------------------------------------------------------------------------
// createParameterLayout — wraps addParameters into an APVTS ParameterLayout.
//------------------------------------------------------------------------------
inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    addParameters(params);
    return juce::AudioProcessorValueTreeState::ParameterLayout(
        std::make_move_iterator(params.begin()),
        std::make_move_iterator(params.end()));
}

} // namespace xoverworld
