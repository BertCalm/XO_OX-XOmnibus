#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FamilyWaveguide.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>

namespace xomnibus {

struct OttoniAdapterVoice {
    bool active=false; int note=0; float vel=0, freq=440, ampEnv=0, sr=44100;
    bool releasing=false;
    FamilyDelayLine dl; FamilyDampingFilter df; FamilyBodyResonance body;
    FamilySympatheticBank symp; FamilyOrganicDrift drift; LipBuzzExciter lipBuzz;

    void prepare(double s){
        sr=(float)s;int md=(int)(sr/20)+8;
        dl.prepare(md);df.prepare();body.prepare(s);symp.prepare(s,512);drift.prepare(s);lipBuzz.prepare(s);
    }
    void reset(){dl.reset();df.reset();body.reset();symp.reset();drift.reset();lipBuzz.reset();active=false;ampEnv=0;}
    void noteOn(int n,float v){
        note=n;vel=v;freq=440*std::pow(2.f,(n-69)/12.f);
        dl.reset();df.reset();body.setParams(freq*0.8f,5);symp.tune(freq);
        ampEnv=v;releasing=false;active=true;
    }
    void noteOff(){releasing=true;}
};

class OttoniEngine : public SynthEngine {
public:
    void prepare(double sampleRate,int) override {sr=sampleRate;for(auto&v:voices)v.prepare(sampleRate);}
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
        float pEmb=emb?emb->load():0.5f, pAge=mGrow?mGrow->load():0;
        float pDa=damp?damp->load():0.995f, pSy=sympA?sympA->load():0.3f;
        float pDR=driftR?driftR->load():0.1f, pDD=driftD?driftD->load():3;
        float pFS=forStr?forStr->load():0, pFC=forCold?forCold->load():0;
        float pME=mEmb?mEmb->load():0.5f, pMF=mFor?mFor->load():0;
        auto*oL=buf.getWritePointer(0);auto*oR=buf.getWritePointer(1);
        for(int i=0;i<ns;++i){
            float sL=0,sR=0;
            for(auto&v:voices){
                if(!v.active)continue;
                float rr=v.releasing?1.f/(v.sr*0.3f):0;
                v.ampEnv=std::max(0.f,v.ampEnv-rr);
                if(v.ampEnv<0.0001f&&v.releasing){v.active=false;continue;}
                float ds=v.drift.tick(pDR,pDD);
                float df=v.freq*std::pow(2.f,ds/12.f);
                float stretch=1+pMF*pFS*0.1f;
                float dlen=v.sr/std::max(df*stretch,20.f);
                float out=v.dl.read(dlen);
                float exc=v.lipBuzz.tick(df,pEmb*pME,pAge);
                float damped=v.df.process(out+exc*0.3f,pDa);
                v.dl.write(damped);
                float bFreq=v.freq*(1+pFC*pMF*0.3f);
                v.body.setParams(bFreq,3+pFC*4);
                float bo=out+v.body.process(out)*0.2f;
                float so=v.symp.process(bo,pSy);
                float sig=(bo+so)*v.ampEnv*0.4f;
                float w=0.1f+pAge*0.4f;
                sL+=sig*(0.5f+w);sR+=sig*(1-w);
            }
            oL[i]+=sL;oR[i]+=sR;lastL=sL;lastR=sR;
        }
        ac=0;for(auto&v:voices)if(v.active)++ac;
    }

    float getSampleForCoupling(int ch,int) const override {return ch==0?lastL:lastR;}
    void applyCouplingInput(CouplingType,float,const float*,int) override {}

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
        using F=juce::AudioParameterFloat;using N=juce::NormalisableRange<float>;
        p.push_back(std::make_unique<F>("otto_teenEmbouchure","Embouchure",N{0,1},0.7f));
        p.push_back(std::make_unique<F>("otto_damping","Damping",N{0.8f,0.999f},0.995f));
        p.push_back(std::make_unique<F>("otto_sympatheticAmt","Sympathetic",N{0,1},0.3f));
        p.push_back(std::make_unique<F>("otto_driftRate","Drift Rate",N{0.05f,0.5f},0.1f));
        p.push_back(std::make_unique<F>("otto_driftDepth","Drift Depth",N{0,20},3.0f));
        p.push_back(std::make_unique<F>("otto_foreignStretch","Foreign Stretch",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("otto_foreignCold","Foreign Cold",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("otto_macroEmbouchure","EMBOUCHURE",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("otto_macroGrow","GROW",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("otto_macroForeign","FOREIGN",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("otto_macroLake","LAKE",N{0,1},0.3f));
        return {p.begin(),p.end()};
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override {
        emb=apvts.getRawParameterValue("otto_teenEmbouchure");
        damp=apvts.getRawParameterValue("otto_damping");
        sympA=apvts.getRawParameterValue("otto_sympatheticAmt");
        driftR=apvts.getRawParameterValue("otto_driftRate");
        driftD=apvts.getRawParameterValue("otto_driftDepth");
        forStr=apvts.getRawParameterValue("otto_foreignStretch");
        forCold=apvts.getRawParameterValue("otto_foreignCold");
        mEmb=apvts.getRawParameterValue("otto_macroEmbouchure");
        mGrow=apvts.getRawParameterValue("otto_macroGrow");
        mFor=apvts.getRawParameterValue("otto_macroForeign");
    }

    juce::String getEngineId() const override {return "Ottoni";}
    juce::Colour getAccentColour() const override {return juce::Colour(0xFF5B8A72);} // Patina
    int getMaxVoices() const override {return kV;}
    int getActiveVoiceCount() const override {return ac;}

private:
    static constexpr int kV=12;
    double sr=44100; int nv=0,ac=0; float lastL=0,lastR=0;
    std::array<OttoniAdapterVoice,kV> voices;
    std::atomic<float>*emb=nullptr,*damp=nullptr,*sympA=nullptr;
    std::atomic<float>*driftR=nullptr,*driftD=nullptr;
    std::atomic<float>*forStr=nullptr,*forCold=nullptr;
    std::atomic<float>*mEmb=nullptr,*mGrow=nullptr,*mFor=nullptr;
};

} // namespace xomnibus
