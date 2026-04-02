// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FamilyWaveguide.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/StandardLFO.h"
#include <array>
#include <cmath>

namespace xoceanus {

struct OleAdapterVoice {
    bool active=false; int note=0; float vel=0, freq=440, ampEnv=0, sr=44100;
    bool releasing=false; int auntIdx=0; bool isHusband=false;
    int husbandType=0; // 0=Oud, 1=Bouzouki, 2=Pin
    FamilyDelayLine dl; FamilyDampingFilter df; FamilyBodyResonance body;
    FamilySympatheticBank symp; FamilyOrganicDrift drift;
    StrumExciter strum; PluckExciter pluck;
    StandardLFO tremoloLFO; // per-voice tremolo LFO for Aunt 3 (Charango) — replaces inline tremoloPhase

    // Voice-steal crossfade (5 ms linear fade-out before new note starts)
    float stealFadeGain = 1.0f;
    float stealFadeStep = 0.0f; // computed as 1.0f / (0.005f * sr) on steal
    bool  isBeingStolen = false;
    int   pendingNote = 0;
    float pendingVel  = 0.0f;
    float pendingStrumRateMs = 8.0f;

    void prepare(double s){
        sr=(float)s;int md=(int)(sr/20)+8;
        dl.prepare(md);df.prepare();body.prepare(s);symp.prepare(s,512);drift.prepare(s);
        strum.prepare(s);pluck.prepare(s);
        tremoloLFO.setShape(StandardLFO::Sine);
        // Rate will be set per-block from the ole_aunt3Tremolo param (5-25 Hz)
    }
    void reset(){dl.reset();df.reset();body.reset();symp.reset();drift.reset();strum.reset();pluck.reset();active=false;ampEnv=0;tremoloLFO.reset();stealFadeGain=1.0f;stealFadeStep=0.0f;isBeingStolen=false;}
    void noteOn(int n,float v, float strumRateMs=8.0f){
        note=n;vel=v;freq=440*std::pow(2.f,(n-69.f)/12.f);
        dl.reset();df.reset();body.setParams(freq*1.1f,3);symp.tune(freq);
        strum.trigger(3,strumRateMs,1);pluck.trigger(2);
        ampEnv=v;releasing=false;active=true;
        tremoloLFO.setShape(StandardLFO::Sine);
        tremoloLFO.reset();
    }
    void noteOff(){releasing=true;}
};

class OleEngine : public SynthEngine {
public:
    void prepare(double sampleRate,int maxBlockSize) override {sr=sampleRate;for(auto&v:voices)v.prepare(sampleRate);silenceGate.prepare(sampleRate,maxBlockSize);}
    void releaseResources() override {for(auto&v:voices)v.reset();}
    void reset() override {for(auto&v:voices)v.reset();lastL=lastR=0;}

    void renderBlock(juce::AudioBuffer<float>&buf,juce::MidiBuffer&midi,int ns) override {
        juce::ScopedNoDenormals noDenormals;
        for(const auto m:midi){
            auto msg=m.getMessage();
            if(msg.isNoteOn()){
                silenceGate.wake();
                int nn=msg.getNoteNumber(); float vel=msg.getVelocity()/127.f;
                float strumRateMs=a1StrumRate?a1StrumRate->load():8.0f;

                // Aunt slot (0-11) — always
                int t=-1;for(int i=0;i<12;++i)if(!voices[i].active){t=i;break;}
                if(t<0){t=nv%12;}
                nv=(t+1)%12;
                voices[t].auntIdx=t%3; voices[t].isHusband=false;
                if(voices[t].active && !voices[t].isBeingStolen){
                    // Voice steal: queue new note behind a 5ms crossfade
                    voices[t].isBeingStolen  = true;
                    voices[t].stealFadeStep  = 1.0f / (0.005f * voices[t].sr);
                    voices[t].stealFadeGain  = 1.0f;
                    voices[t].pendingNote    = nn;
                    voices[t].pendingVel     = vel;
                    voices[t].pendingStrumRateMs = strumRateMs;
                } else if(!voices[t].isBeingStolen){
                    voices[t].noteOn(nn, vel, strumRateMs);
                }

                // Husband slot (12-17) — always allocated; muted in render loop when DRAMA<0.7
                int h=-1;for(int i=12;i<kV;++i)if(!voices[i].active){h=i;break;}
                if(h<0){h=12+nhv%6;}
                nhv=(nhv+1)%6;
                voices[h].isHusband=true; voices[h].husbandType=(h-12)%3;
                if(voices[h].active && !voices[h].isBeingStolen){
                    voices[h].isBeingStolen  = true;
                    voices[h].stealFadeStep  = 1.0f / (0.005f * voices[h].sr);
                    voices[h].stealFadeGain  = 1.0f;
                    voices[h].pendingNote    = nn;
                    voices[h].pendingVel     = vel;
                    voices[h].pendingStrumRateMs = strumRateMs;
                } else if(!voices[h].isBeingStolen){
                    voices[h].noteOn(nn, vel, strumRateMs);
                }
            } else if (msg.isNoteOff()) {
                for (auto& v : voices) if (v.active && v.note == msg.getNoteNumber()) v.noteOff();
            } else if (msg.isChannelPressure()) {
                float atPressure = (float)msg.getChannelPressureValue() / 127.f;
                for (auto& v : voices)
                    if (v.active) v.vel = juce::jmax(v.vel, atPressure);
            }
            else if (msg.isController() && msg.getControllerNumber() == 1) {
                float modWheel = (float)msg.getControllerValue() / 127.f;
                for (auto& v : voices)
                    if (v.active) v.vel = juce::jmax(v.vel, modWheel * 0.7f);
            }
            else if (msg.isPitchWheel()) pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
        }

        // SilenceGate: skip all DSP if engine has been silent long enough
        if(silenceGate.isBypassed() && midi.isEmpty()){buf.clear();return;}

        // ---- Read ALL parameters ----
        // Aunt levels
        float pA1Lv = a1Level ? a1Level->load() : 0.7f;
        float pA2Lv = a2Level ? a2Level->load() : 0.7f;
        float pA3Lv = a3Level ? a3Level->load() : 0.7f;
        // Aunt 1 character
        float pA1Sr = a1StrumRate ? a1StrumRate->load() : 8.0f;
        float pA1Br = a1Bright ? a1Bright->load() : 0.6f;
        // Aunt 2 character
        float pA2Cp = a2CoinPress ? a2CoinPress->load() : 0.0f;
        float pA2Gs = a2GourdSize ? a2GourdSize->load() : 0.5f;
        // Aunt 3 character
        float pA3Tr = a3Tremolo ? a3Tremolo->load() : 12.0f;
        float pA3Br = a3Bright ? a3Bright->load() : 0.5f;
        // Shared waveguide
        float pDa = damp ? damp->load() : 0.995f;
        float pSy = sympA ? sympA->load() : 0.3f;
        float pDR = driftR ? driftR->load() : 0.1f;
        float pDD = driftD ? driftD->load() : 3.0f;
        // Alliance
        float pAlCfg = allianceCfg ? allianceCfg->load() : 0.0f;
        float pAlBl  = allianceBlend ? allianceBlend->load() : 0.5f;
        // Husband levels
        float pHOud  = hOudLvl ? hOudLvl->load() : 0.0f;
        float pHBouz = hBouzLvl ? hBouzLvl->load() : 0.0f;
        float pHPin  = hPinLvl ? hPinLvl->load() : 0.0f;
        // Macros
        float pFu = mFuego ? mFuego->load() : 0.5f;
        float pDr = mDrama ? mDrama->load() : 0.0f;
        float pSi = mSides ? mSides->load() : 0.0f;
        float pIs = mIsla  ? mIsla->load()  : 0.3f;

        // ---- Derive alliance gains per aunt (2-vs-1 shifting alliances) ----
        // allianceConfig: 0 = Aunt1 isolated, 1 = Aunt2 isolated, 2 = Aunt3 isolated
        // allianceBlend: smooth crossfade within pair vs isolated
        // SIDES macro rotates the alliance configuration continuously
        float alliancePos = pAlCfg + pSi * 2.99f; // SIDES sweeps through all 3 configs
        alliancePos = std::fmod(alliancePos, 3.0f);
        int alCfgInt = (int)alliancePos;
        float alFrac = alliancePos - (float)alCfgInt;

        // Base gains: isolated aunt gets (1-blend), paired aunts get blend
        float auntGains[3] = {1.0f, 1.0f, 1.0f};
        auto applyAlliance = [&](int isolated, float strength) {
            auntGains[isolated] *= (1.0f - strength * pAlBl * 0.5f);
            int p1 = (isolated + 1) % 3;
            int p2 = (isolated + 2) % 3;
            auntGains[p1] *= (1.0f + strength * pAlBl * 0.3f);
            auntGains[p2] *= (1.0f + strength * pAlBl * 0.3f);
        };
        applyAlliance(alCfgInt % 3, 1.0f - alFrac);
        if (alFrac > 0.001f) applyAlliance((alCfgInt + 1) % 3, alFrac);

        // Scale aunt gains by their individual level params
        auntGains[0] *= pA1Lv;
        auntGains[1] *= pA2Lv;
        auntGains[2] *= pA3Lv;

        // ---- Husband level lookup ----
        float husbandLvl[3] = {pHOud, pHBouz, pHPin};

        // Update tremolo LFO rate once per block for all active Aunt 3 voices
        for(auto&v:voices)
            if(v.active && !v.isHusband && v.auntIdx==2)
                v.tremoloLFO.setRate(pA3Tr, v.sr);

        auto*oL=buf.getWritePointer(0);auto*oR=buf.getNumChannels()>1?buf.getWritePointer(1):buf.getWritePointer(0);
        for(int i=0;i<ns;++i){
            float sL=0,sR=0;
            for(auto&v:voices){
                if(!v.active)continue;

                // Voice-steal crossfade: fade outgoing voice over 5ms, then start pending note.
                // Processed before the DRAMA guard so husband steals always complete.
                if(v.isBeingStolen){
                    v.stealFadeGain -= v.stealFadeStep;
                    if(v.stealFadeGain <= 0.0f){
                        // Fade complete — start the new note now
                        v.isBeingStolen  = false;
                        v.stealFadeGain  = 1.0f;
                        v.stealFadeStep  = 0.0f;
                        v.noteOn(v.pendingNote, v.pendingVel, v.pendingStrumRateMs);
                        // auntIdx/isHusband flags remain valid from the steal assignment above
                        continue; // skip this sample to avoid artefact on first new-note sample
                    }
                }

                // Husbands only active when DRAMA > 0.7
                if(v.isHusband && pDr < 0.7f)continue;

                // Exponential release: avoids the "soft note releases in fraction
                // of stated time" bug caused by linear subtraction on small ampEnv values.
                if(v.releasing){
                    float releaseCoeff=1.0f-(1.0f/(v.sr*0.4f));
                    v.ampEnv*=releaseCoeff;
                } // (no action during sustain — natural waveguide decay governs)
                v.ampEnv=flushDenormal(v.ampEnv);
                if(v.ampEnv<0.0001f&&v.releasing){v.active=false;continue;}

                // ---- Per-aunt brightness selection ----
                float voiceBright;
                if (v.isHusband) {
                    // Husbands use average of aunt brightnesses, slightly darker
                    voiceBright = (pA1Br + pA3Br) * 0.5f * 0.6f;
                } else if (v.auntIdx == 0) {
                    voiceBright = pA1Br;
                } else if (v.auntIdx == 2) {
                    voiceBright = pA3Br;
                } else {
                    // Aunt 2 (Berimbau) brightness derived from gourd size (inverse)
                    voiceBright = 1.0f - pA2Gs * 0.6f;
                }

                // ---- Drift ----
                float ds=v.drift.tick(pDR,pDD);
                float df=v.freq*fastPow2((ds+extPitchMod)/12.f)*PitchBendUtil::semitonesToFreqRatio(pitchBendNorm*2.0f);

                // ---- Aunt 2: Coin press pitch bend ----
                if (!v.isHusband && v.auntIdx == 1) {
                    // Coin press bends pitch up by up to a major 3rd (4 semitones)
                    df *= fastPow2(pA2Cp * 4.0f / 12.0f);
                }

                float dlen=v.sr/std::max(df,20.f);
                float out=v.dl.read(dlen);

                // ---- Excitation: per-aunt strum rate + FUEGO intensity ----
                float velIntens = 0.5f + v.vel * 0.5f; // velocity 0→1 maps to 0.5→1.0x intensity
                float effIntens = extIntens * velIntens;
                float exc;
                if (v.isHusband) {
                    exc = v.pluck.tick(voiceBright * 0.6f) * effIntens;
                } else if (v.auntIdx == 0) {
                    // Aunt 1 (Tres Cubano): strum rate shapes brightness — faster strum (higher pA1Sr)
                    // produces tighter, brighter pluck energy. Boost is zero at default rate (8 Hz)
                    // and ramps to 1.0 at max (30 Hz), preserving existing preset compatibility.
                    // (Audit P0-6: original used min(pA1Sr/30, 1) which was non-zero at default)
                    float strumBrightBoost = std::min (std::max (pA1Sr - 8.0f, 0.0f) / 22.0f, 1.0f);
                    exc = v.strum.tick(voiceBright * pFu * (1.0f + strumBrightBoost * 0.3f)) * effIntens;
                } else {
                    exc = v.strum.tick(voiceBright * pFu) * effIntens;
                }

                // ---- Aunt 2: Gourd body resonance ----
                float bodyGain = 0.2f;
                if (!v.isHusband && v.auntIdx == 1) {
                    // Gourd size scales the body resonance (bigger gourd = more body, lower resonance)
                    float gourdFreq = v.freq * (1.5f - pA2Gs * 0.5f); // gourd shifts resonance
                    v.body.setParams(gourdFreq, 2.0f + pA2Gs * 4.0f);
                    bodyGain = 0.2f + pA2Gs * 0.3f; // bigger gourd = more resonance
                }

                float damped=v.df.process(out+exc*0.3f,std::clamp(pDa+extDampMod,0.f,1.f));
                v.dl.write(flushDenormal(damped));

                // ---- Aunt 3: Charango tremolo ----
                float tremoloMod = 1.0f;
                if (!v.isHusband && v.auntIdx == 2) {
                    // Rapid tremolo oscillation (5-25 Hz) modulates amplitude
                    tremoloMod = 0.7f + 0.3f * v.tremoloLFO.process();
                }

                float bo=out+v.body.process(out)*bodyGain;
                float so=v.symp.process(bo,pSy);

                // ---- Per-voice level from alliance or husband level ----
                float voiceLevel;
                if (v.isHusband) {
                    // Individual husband level scaled by DRAMA intensity above threshold
                    float dramaScale = (pDr - 0.7f) / 0.3f; // 0-1 over DRAMA 0.7-1.0
                    dramaScale = std::min(std::max(dramaScale, 0.0f), 1.0f);
                    voiceLevel = husbandLvl[v.husbandType] * dramaScale;
                } else {
                    voiceLevel = auntGains[v.auntIdx];
                }

                float sig=(bo+so)*v.ampEnv*voiceLevel*tremoloMod*v.stealFadeGain*0.4f;

                // ---- ISLA: stereo width / tropical space ----
                // ISLA widens the stereo spread and adds a subtle reverb-like tail
                float pan=(v.auntIdx==0)?0.2f:(v.auntIdx==1)?0.5f:0.8f;
                if(v.isHusband)pan=0.5f;
                // ISLA widens: push panned voices further from center
                float islaSpread = pIs * 0.3f;
                pan = 0.5f + (pan - 0.5f) * (1.0f + islaSpread);
                pan = std::min(std::max(pan, 0.0f), 1.0f);

                sL+=sig*(1-pan);sR+=sig*pan;
            }
            oL[i]+=sL;oR[i]+=sR;lastL=sL;lastR=sR;
        }
        {int c=0;for(auto&v:voices)if(v.active)++c;activeVoiceCount_.store(c,std::memory_order_relaxed);}
        // SilenceGate: analyze output level for next-block bypass decision
        silenceGate.analyzeBlock(buf.getReadPointer(0),buf.getNumChannels()>1?buf.getReadPointer(1):nullptr,ns);
    }

    float getSampleForCoupling(int ch,int) const override {return ch==0?lastL:lastR;}
    void applyCouplingInput(CouplingType t, float amount,
                            const float* buf, int /*ns*/) override
    {
        switch (t)
        {
            case CouplingType::LFOToPitch:
                extPitchMod = (buf ? buf[0] : 0.f) * amount * 2.f;
                break;
            case CouplingType::AmpToFilter:
                extDampMod = amount * 0.08f;
                break;
            case CouplingType::EnvToMorph:
                extIntens = 1.f + amount * 0.5f;
                break;
            default: break;
        }
    }

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& p)
    {
        using F=juce::AudioParameterFloat;using C=juce::AudioParameterChoice;using N=juce::NormalisableRange<float>;
        // Aunt 1: Tres Cubano
        p.push_back(std::make_unique<F>("ole_aunt1Level","Aunt 1 Level",N{0,1},0.7f));
        p.push_back(std::make_unique<F>("ole_aunt1StrumRate","Aunt 1 Strum Rate",N{1,30},8.0f));
        p.push_back(std::make_unique<F>("ole_aunt1Brightness","Aunt 1 Brightness",N{0,1},0.6f));
        // Aunt 2: Berimbau
        p.push_back(std::make_unique<F>("ole_aunt2Level","Aunt 2 Level",N{0,1},0.7f));
        p.push_back(std::make_unique<F>("ole_aunt2CoinPress","Coin Press",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("ole_aunt2GourdSize","Gourd Size",N{0,1},0.5f));
        // Aunt 3: Charango
        p.push_back(std::make_unique<F>("ole_aunt3Level","Aunt 3 Level",N{0,1},0.7f));
        p.push_back(std::make_unique<F>("ole_aunt3Tremolo","Aunt 3 Tremolo",N{5,25},12.0f));
        p.push_back(std::make_unique<F>("ole_aunt3Brightness","Aunt 3 Brightness",N{0,1},0.5f));
        // Shared waveguide
        p.push_back(std::make_unique<F>("ole_damping","Damping",N{0.8f,0.999f},0.995f));
        p.push_back(std::make_unique<F>("ole_sympatheticAmt","Sympathetic",N{0,1},0.3f));
        p.push_back(std::make_unique<F>("ole_driftRate","Drift Rate",N{0.005f,0.5f},0.1f));
        p.push_back(std::make_unique<F>("ole_driftDepth","Drift Depth",N{0,20},3.0f));
        // Alliance
        p.push_back(std::make_unique<C>("ole_allianceConfig","Alliance",juce::StringArray{"1 vs 2+3","2 vs 1+3","3 vs 1+2"},0));
        p.push_back(std::make_unique<F>("ole_allianceBlend","Alliance Blend",N{0,1},0.5f));
        // Husbands
        p.push_back(std::make_unique<F>("ole_husbandOudLevel","Oud Level",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("ole_husbandBouzLevel","Bouzouki Level",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("ole_husbandPinLevel","Pin Level",N{0,1},0.0f));
        // Macros
        p.push_back(std::make_unique<F>("ole_macroFuego","FUEGO",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("ole_macroDrama","DRAMA",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("ole_macroSides","SIDES",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("ole_macroIsla","ISLA",N{0,1},0.3f));
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
        addParameters(p);
        return {p.begin(),p.end()};
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override {
        // Aunt levels
        a1Level    = apvts.getRawParameterValue("ole_aunt1Level");
        a2Level    = apvts.getRawParameterValue("ole_aunt2Level");
        a3Level    = apvts.getRawParameterValue("ole_aunt3Level");
        // Aunt 1 character
        a1StrumRate= apvts.getRawParameterValue("ole_aunt1StrumRate");
        a1Bright   = apvts.getRawParameterValue("ole_aunt1Brightness");
        // Aunt 2 character
        a2CoinPress= apvts.getRawParameterValue("ole_aunt2CoinPress");
        a2GourdSize= apvts.getRawParameterValue("ole_aunt2GourdSize");
        // Aunt 3 character
        a3Tremolo  = apvts.getRawParameterValue("ole_aunt3Tremolo");
        a3Bright   = apvts.getRawParameterValue("ole_aunt3Brightness");
        // Shared waveguide
        damp       = apvts.getRawParameterValue("ole_damping");
        sympA      = apvts.getRawParameterValue("ole_sympatheticAmt");
        driftR     = apvts.getRawParameterValue("ole_driftRate");
        driftD     = apvts.getRawParameterValue("ole_driftDepth");
        // Alliance
        allianceCfg= apvts.getRawParameterValue("ole_allianceConfig");
        allianceBlend= apvts.getRawParameterValue("ole_allianceBlend");
        // Husband levels
        hOudLvl    = apvts.getRawParameterValue("ole_husbandOudLevel");
        hBouzLvl   = apvts.getRawParameterValue("ole_husbandBouzLevel");
        hPinLvl    = apvts.getRawParameterValue("ole_husbandPinLevel");
        // Macros
        mFuego     = apvts.getRawParameterValue("ole_macroFuego");
        mDrama     = apvts.getRawParameterValue("ole_macroDrama");
        mSides     = apvts.getRawParameterValue("ole_macroSides");
        mIsla      = apvts.getRawParameterValue("ole_macroIsla");
    }

    juce::String getEngineId() const override {return "Ole";}
    juce::Colour getAccentColour() const override {return juce::Colour(0xFFC9377A);} // Hibiscus
    int getMaxVoices() const override {return kV;}
    int getActiveVoiceCount() const override {return activeVoiceCount_.load(std::memory_order_relaxed);}

private:
    SilenceGate silenceGate;
    static constexpr int kV=18;
    double sr=44100; int nv=0,nhv=0; float lastL=0,lastR=0; // ac promoted to base class activeVoiceCount_
    std::array<OleAdapterVoice,kV> voices;

    float pitchBendNorm = 0.0f; // MIDI pitch wheel [-1, +1]; ±2 semitone range

    // Family coupling ext mods (SP7.3)
    float extPitchMod = 0.f;   // semitones from LFOToPitch
    float extDampMod  = 0.f;   // 0–1 from AmpToFilter
    float extIntens   = 1.f;   // multiplier from EnvToMorph
    // Aunt levels
    std::atomic<float>*a1Level=nullptr,*a2Level=nullptr,*a3Level=nullptr;
    // Aunt 1 character
    std::atomic<float>*a1StrumRate=nullptr,*a1Bright=nullptr;
    // Aunt 2 character
    std::atomic<float>*a2CoinPress=nullptr,*a2GourdSize=nullptr;
    // Aunt 3 character
    std::atomic<float>*a3Tremolo=nullptr,*a3Bright=nullptr;
    // Shared waveguide
    std::atomic<float>*damp=nullptr,*sympA=nullptr;
    std::atomic<float>*driftR=nullptr,*driftD=nullptr;
    // Alliance
    std::atomic<float>*allianceCfg=nullptr,*allianceBlend=nullptr;
    // Husband levels
    std::atomic<float>*hOudLvl=nullptr,*hBouzLvl=nullptr,*hPinLvl=nullptr;
    // Macros
    std::atomic<float>*mFuego=nullptr,*mDrama=nullptr;
    std::atomic<float>*mSides=nullptr,*mIsla=nullptr;
};

} // namespace xoceanus
