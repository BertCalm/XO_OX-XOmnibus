#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FamilyWaveguide.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>

namespace xomnibus {

struct OrphicaAdapterVoice {
    bool active=false; int note=0; float vel=0, freq=440, ampEnv=0, sr=44100;
    bool releasing=false;
    FamilyDelayLine dl; FamilyDampingFilter df; FamilyBodyResonance body;
    FamilySympatheticBank symp; FamilyOrganicDrift drift; PluckExciter pluck;

    void prepare(double s){
        sr=(float)s;int md=(int)(sr/20)+8;
        dl.prepare(md);df.prepare();body.prepare(s);symp.prepare(s,512);drift.prepare(s);pluck.prepare(s);
    }
    void reset(){dl.reset();df.reset();body.reset();symp.reset();drift.reset();pluck.reset();active=false;ampEnv=0;}
    void noteOn(int n,float v){
        note=n;vel=v;freq=440*std::pow(2.f,(n-69)/12.f);
        dl.reset();df.reset();body.setParams(freq*1.2f,4);symp.tune(freq);
        pluck.trigger(2.5f);ampEnv=v;releasing=false;active=true;
    }
    void noteOff(){releasing=true;}
};

class OrphicaEngine : public SynthEngine {
public:
    void prepare(double sampleRate,int maxBlockSize) override {
        sr=sampleRate;for(auto&v:voices)v.prepare(sampleRate);
    }
    void releaseResources() override {for(auto&v:voices)v.reset();}
    void reset() override {for(auto&v:voices)v.reset();lastL=lastR=0;}

    void renderBlock(juce::AudioBuffer<float>&buf,juce::MidiBuffer&midi,int ns) override {
        for(const auto m:midi){
            auto msg=m.getMessage();
            if(msg.isNoteOn()){
                int t=-1;for(int i=0;i<kV;++i)if(!voices[i].active){t=i;break;}
                if(t<0)t=nv%kV;nv=(t+1)%kV;
                voices[t].noteOn(msg.getNoteNumber(),msg.getVelocity()/127.f);
            }else if(msg.isNoteOff())
                for(auto&v:voices)if(v.active&&v.note==msg.getNoteNumber())v.noteOff();
        }
        float pBr=bright?bright->load():0.5f, pDa=damp?damp->load():0.995f;
        float pSy=sympA?sympA->load():0.3f, pDR=driftR?driftR->load():0.1f;
        float pDD=driftD?driftD->load():3, pCN=crossN?crossN->load():60;
        float pCB=crossB?crossB->load():6, pLL=lowLvl?lowLvl->load():0.8f;
        float pHL=hiLvl?hiLvl->load():0.8f, pMP=mPluck?mPluck->load():0.5f;
        float pMS=mSurf?mSurf->load():0.5f, pMD=mDiv?mDiv->load():0.2f;
        auto*oL=buf.getWritePointer(0);auto*oR=buf.getWritePointer(1);
        for(int i=0;i<ns;++i){
            float sL=0,sR=0;
            for(auto&v:voices){
                if(!v.active)continue;
                float rr=v.releasing?1.f/(v.sr*0.5f):0;
                v.ampEnv=std::max(0.f,v.ampEnv-rr);
                if(v.ampEnv<0.0001f&&v.releasing){v.active=false;continue;}
                float ds=v.drift.tick(pDR,pDD);
                float df=v.freq*std::pow(2.f,ds/12.f);
                float dlen=v.sr/std::max(df,20.f);
                float out=v.dl.read(dlen);
                float exc=v.pluck.tick(pBr*pMP);
                float damped=v.df.process(out+exc*0.4f,pDa);
                v.dl.write(damped);
                float bo=out+v.body.process(out)*0.2f;
                float so=v.symp.process(bo,pSy);
                float sig=(bo+so)*v.ampEnv*0.4f;
                // Crossover
                float nf=(float)v.note;
                float hb=std::clamp((nf-pCN)/std::max(pCB,1.f)+0.5f+(pMS-0.5f)*0.4f,0.f,1.f);
                float lo=sig*(1-hb)*pLL, hi=sig*hb*pHL*(1+pMD*0.3f);
                sL+=lo*0.7f+hi*0.3f; sR+=lo*0.3f+hi*0.7f;
            }
            oL[i]+=sL;oR[i]+=sR;lastL=sL;lastR=sR;
        }
        ac=0;for(auto&v:voices)if(v.active)++ac;
    }

    float getSampleForCoupling(int ch,int) const override {return ch==0?lastL:lastR;}
    void applyCouplingInput(CouplingType,float,const float*,int) override {}

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
        using F=juce::AudioParameterFloat;using C=juce::AudioParameterChoice;using N=juce::NormalisableRange<float>;
        p.push_back(std::make_unique<C>("orph_stringMaterial","String Material",juce::StringArray{"Nylon","Steel","Crystal","Light"},0));
        p.push_back(std::make_unique<F>("orph_pluckBrightness","Pluck Brightness",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("orph_damping","Damping",N{0.8f,0.999f},0.995f));
        p.push_back(std::make_unique<F>("orph_sympatheticAmt","Sympathetic",N{0,1},0.3f));
        p.push_back(std::make_unique<F>("orph_driftRate","Drift Rate",N{0.05f,0.5f},0.1f));
        p.push_back(std::make_unique<F>("orph_driftDepth","Drift Depth",N{0,20},3.0f));
        p.push_back(std::make_unique<F>("orph_crossoverNote","Crossover Note",N{36,84,1},60.0f));
        p.push_back(std::make_unique<F>("orph_crossoverBlend","Crossover Blend",N{0,12,1},6.0f));
        p.push_back(std::make_unique<F>("orph_lowLevel","Low Level",N{0,1},0.8f));
        p.push_back(std::make_unique<F>("orph_highLevel","High Level",N{0,1},0.8f));
        p.push_back(std::make_unique<F>("orph_macroPluck","PLUCK",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("orph_macroFracture","FRACTURE",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("orph_macroSurface","SURFACE",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("orph_macroDivine","DIVINE",N{0,1},0.2f));
        return {p.begin(),p.end()};
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override {
        bright=apvts.getRawParameterValue("orph_pluckBrightness");
        damp=apvts.getRawParameterValue("orph_damping");
        sympA=apvts.getRawParameterValue("orph_sympatheticAmt");
        driftR=apvts.getRawParameterValue("orph_driftRate");
        driftD=apvts.getRawParameterValue("orph_driftDepth");
        crossN=apvts.getRawParameterValue("orph_crossoverNote");
        crossB=apvts.getRawParameterValue("orph_crossoverBlend");
        lowLvl=apvts.getRawParameterValue("orph_lowLevel");
        hiLvl=apvts.getRawParameterValue("orph_highLevel");
        mPluck=apvts.getRawParameterValue("orph_macroPluck");
        mSurf=apvts.getRawParameterValue("orph_macroSurface");
        mDiv=apvts.getRawParameterValue("orph_macroDivine");
    }

    juce::String getEngineId() const override {return "Orphica";}
    juce::Colour getAccentColour() const override {return juce::Colour(0xFF7FDBCA);} // Siren Seafoam
    int getMaxVoices() const override {return kV;}
    int getActiveVoiceCount() const override {return ac;}

private:
    static constexpr int kV=16;
    double sr=44100; int nv=0,ac=0; float lastL=0,lastR=0;
    std::array<OrphicaAdapterVoice,kV> voices;
    std::atomic<float>*bright=nullptr,*damp=nullptr,*sympA=nullptr;
    std::atomic<float>*driftR=nullptr,*driftD=nullptr;
    std::atomic<float>*crossN=nullptr,*crossB=nullptr,*lowLvl=nullptr,*hiLvl=nullptr;
    std::atomic<float>*mPluck=nullptr,*mSurf=nullptr,*mDiv=nullptr;
};

} // namespace xomnibus
