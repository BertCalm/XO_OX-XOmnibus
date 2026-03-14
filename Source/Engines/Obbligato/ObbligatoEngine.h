#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FamilyWaveguide.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>

namespace xomnibus {

struct ObbligatoAdapterVoice {
    bool active=false; int note=0; float vel=0, freq=440, ampEnv=0, sr=44100;
    bool releasing=false, isBroA=true;
    FamilyDelayLine dl; FamilyDampingFilter df; FamilyBodyResonance body;
    FamilySympatheticBank symp; FamilyOrganicDrift drift;
    AirJetExciter airJet; ReedExciter reed;

    void prepare(double s){
        sr=(float)s;int md=(int)(sr/20)+8;
        dl.prepare(md);df.prepare();body.prepare(s);symp.prepare(s,512);drift.prepare(s);
        airJet.prepare(s);reed.prepare(s);
    }
    void reset(){dl.reset();df.reset();body.reset();symp.reset();drift.reset();airJet.reset();reed.reset();active=false;ampEnv=0;}
    void noteOn(int n,float v){
        note=n;vel=v;freq=440*std::pow(2.f,(n-69)/12.f);
        dl.reset();df.reset();body.setParams(freq*1.3f,3.5f);symp.tune(freq);
        ampEnv=v;releasing=false;active=true;
    }
    void noteOff(){releasing=true;}
};

class ObbligatoEngine : public SynthEngine {
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
                voices[t].isBroA=(t%2==0); // alternate
                voices[t].noteOn(msg.getNoteNumber(),msg.getVelocity()/127.f);
            }else if(msg.isNoteOff())
                for(auto&v:voices)if(v.active&&v.note==msg.getNoteNumber())v.noteOff();
        }
        float pBreath=breath?breath->load():0.7f, pDa=damp?damp->load():0.995f;
        float pStiff=stiff?stiff->load():0.5f, pSy=sympA?sympA->load():0.3f;
        float pDR=driftR?driftR->load():0.1f, pDD=driftD?driftD->load():3;
        float pMB=mBreath?mBreath->load():0.5f;
        auto*oL=buf.getWritePointer(0);auto*oR=buf.getWritePointer(1);
        for(int i=0;i<ns;++i){
            float sL=0,sR=0;
            for(auto&v:voices){
                if(!v.active)continue;
                float rr=v.releasing?1.f/(v.sr*0.4f):0;
                v.ampEnv=std::max(0.f,v.ampEnv-rr);
                if(v.ampEnv<0.0001f&&v.releasing){v.active=false;continue;}
                float ds=v.drift.tick(pDR,pDD);
                float df=v.freq*std::pow(2.f,ds/12.f);
                float dlen=v.sr/std::max(df,20.f);
                float out=v.dl.read(dlen);
                float exc=v.isBroA?v.airJet.tick(pBreath*pMB,v.freq):v.reed.tick(pBreath*pMB,pStiff);
                float damped=v.df.process(out+exc*0.3f,pDa);
                v.dl.write(damped);
                float bo=out+v.body.process(out)*0.2f;
                float so=v.symp.process(bo,pSy);
                float sig=(bo+so)*v.ampEnv*0.4f;
                float pan=v.isBroA?0.35f:0.65f;
                sL+=sig*(1-pan);sR+=sig*pan;
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
        p.push_back(std::make_unique<F>("obbl_breathA","Breath A",N{0,1},0.7f));
        p.push_back(std::make_unique<F>("obbl_reedStiffness","Reed Stiffness",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("obbl_damping","Damping",N{0.8f,0.999f},0.995f));
        p.push_back(std::make_unique<F>("obbl_sympatheticAmt","Sympathetic",N{0,1},0.3f));
        p.push_back(std::make_unique<F>("obbl_driftRate","Drift Rate",N{0.05f,0.5f},0.1f));
        p.push_back(std::make_unique<F>("obbl_driftDepth","Drift Depth",N{0,20},3.0f));
        p.push_back(std::make_unique<F>("obbl_macroBreath","BREATH",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("obbl_macroBond","BOND",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("obbl_macroMischief","MISCHIEF",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("obbl_macroWind","WIND",N{0,1},0.3f));
        return {p.begin(),p.end()};
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override {
        breath=apvts.getRawParameterValue("obbl_breathA");
        stiff=apvts.getRawParameterValue("obbl_reedStiffness");
        damp=apvts.getRawParameterValue("obbl_damping");
        sympA=apvts.getRawParameterValue("obbl_sympatheticAmt");
        driftR=apvts.getRawParameterValue("obbl_driftRate");
        driftD=apvts.getRawParameterValue("obbl_driftDepth");
        mBreath=apvts.getRawParameterValue("obbl_macroBreath");
    }

    juce::String getEngineId() const override {return "Obbligato";}
    juce::Colour getAccentColour() const override {return juce::Colour(0xFFFF8A7A);} // Rascal Coral
    int getMaxVoices() const override {return kV;}
    int getActiveVoiceCount() const override {return ac;}

private:
    static constexpr int kV=12;
    double sr=44100; int nv=0,ac=0; float lastL=0,lastR=0;
    std::array<ObbligatoAdapterVoice,kV> voices;
    std::atomic<float>*breath=nullptr,*stiff=nullptr,*damp=nullptr,*sympA=nullptr;
    std::atomic<float>*driftR=nullptr,*driftD=nullptr,*mBreath=nullptr;
};

} // namespace xomnibus
