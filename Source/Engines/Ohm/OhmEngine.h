#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FamilyWaveguide.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>

namespace xomnibus {

//==============================================================================
// OhmEngine — XOmnibus adapter for XOhm (OHM).
//
// Dad (physical modeling) + In-Laws (spectral interference) + Obed (FM).
// MEDDLING + COMMUNE dual-axis interaction system.
// Uses FamilyWaveguide primitives: PickExciter, BowExciter, DelayLine,
// DampingFilter, BodyResonance, SympatheticBank, OrganicDrift.
//==============================================================================

// 2-op FM for Obed
struct OhmObedFM {
    static constexpr float kRatios[8][2] = {
        {1,1},{3,2},{5,4},{2,1},{5,3},{7,4},{9,5},{11,6}
    };
    float phase=0, modPhase=0, envLevel=0, sr=44100;
    void prepare(double s){sr=(float)s;} void reset(){phase=modPhase=envLevel=0;}
    void trigger(){envLevel=1.0f;}
    float tick(float freq, int ratio, float index, float decay){
        int r=std::clamp(ratio,0,7);
        float mf=freq*kRatios[r][0]/kRatios[r][1];
        envLevel*=(1.0f-1.0f/(sr*std::max(decay,0.01f)));
        modPhase+=mf/sr; if(modPhase>=1)modPhase-=1;
        float mod=std::sin(modPhase*6.2831853f)*index*envLevel;
        phase+=freq/sr; if(phase>=1)phase-=1;
        return std::sin((phase+mod)*6.2831853f)*envLevel;
    }
};
constexpr float OhmObedFM::kRatios[8][2];

// Theremin interference
struct OhmInLaw {
    float phase=0, wobPhase=0, sr=44100;
    void prepare(double s){sr=(float)s;} void reset(){phase=wobPhase=0;}
    float tick(float freq, float scale, float wobble){
        wobPhase+=0.3f/sr; if(wobPhase>=1)wobPhase-=1;
        float wob=std::sin(wobPhase*6.2831853f)*wobble*0.05f;
        phase+=(freq*scale*(1+wob))/sr; if(phase>=1)phase-=1;
        return std::sin(phase*6.2831853f);
    }
};

// Single OHM voice
struct OhmAdapterVoice {
    bool active=false; int note=0; float vel=0, freq=440, ampEnv=0;
    bool releasing=false, bowed=false;
    FamilyDelayLine dl; FamilyDampingFilter df; FamilyBodyResonance body;
    FamilySympatheticBank symp; FamilyOrganicDrift drift;
    PickExciter pick; BowExciter bow; OhmObedFM obed; OhmInLaw inlaw;
    float lastOut=0, sr=44100;

    void prepare(double s){
        sr=(float)s; int md=(int)(sr/20)+8;
        dl.prepare(md); df.prepare(); body.prepare(s); symp.prepare(s,512);
        drift.prepare(s); pick.prepare(s); bow.prepare(s); obed.prepare(s); inlaw.prepare(s);
    }
    void reset(){
        dl.reset();df.reset();body.reset();symp.reset();drift.reset();
        pick.reset();bow.reset();obed.reset();inlaw.reset();
        active=false;ampEnv=0;
    }
    void noteOn(int n,float v){
        note=n;vel=v;freq=440*std::pow(2.f,(n-69)/12.f);
        dl.reset();df.reset();body.setParams(freq*1.5f,3);symp.tune(freq);
        if(!bowed)pick.trigger(1.5f); obed.trigger();
        ampEnv=v;releasing=false;active=true;
    }
    void noteOff(){releasing=true;}
};

class OhmEngine : public SynthEngine {
public:
    void prepare(double sampleRate, int maxBlockSize) override {
        sr = sampleRate;
        for(auto&v:voices)v.prepare(sampleRate);
        couplingBuf.resize(maxBlockSize*2,0);
    }
    void releaseResources() override { for(auto&v:voices)v.reset(); }
    void reset() override { for(auto&v:voices)v.reset(); lastSampleL=lastSampleR=0; }

    void renderBlock(juce::AudioBuffer<float>&buf,juce::MidiBuffer&midi,int ns) override {
        // MIDI
        for(const auto m:midi){
            auto msg=m.getMessage();
            if(msg.isNoteOn()){
                int t=-1;
                for(int i=0;i<kVoices;++i)if(!voices[i].active){t=i;break;}
                if(t<0)t=nextV%kVoices; nextV=(t+1)%kVoices;
                voices[t].bowed=(dadInst&&*dadInst>=3.5f&&*dadInst<=4.5f);
                voices[t].noteOn(msg.getNoteNumber(),msg.getVelocity()/127.f);
            } else if(msg.isNoteOff()){
                for(auto&v:voices)if(v.active&&v.note==msg.getNoteNumber())v.noteOff();
            }
        }
        // Param reads
        float pBright=bright?bright->load():0.5f;
        float pBowP=bowP?bowP->load():0;
        float pBowS=bowS?bowS->load():0.5f;
        float pDamp=damp?damp->load():0.995f;
        float pSymp=sympA?sympA->load():0.3f;
        float pDriftR=driftR?driftR->load():0.1f;
        float pDriftD=driftD?driftD->load():3;
        float pMeddling=mMeddling?mMeddling->load():0;
        float pCommune=mCommune?mCommune->load():0;
        float pJam=mJam?mJam->load():0.5f;
        float pInlaw=inlawLvl?inlawLvl->load():0;
        float pThScale=thScale?thScale->load():1;
        float pThWob=thWob?thWob->load():0.3f;
        float pObedLvl=obedLvl?obedLvl->load():0;
        float pFmRatio=(int)(fmRatio?fmRatio->load():0);
        float pFmIdx=fmIdx?fmIdx->load():2;
        float pFmDec=fmDec?fmDec->load():0.3f;
        float pMedTh=medTh?medTh->load():0.5f;

        auto*outL=buf.getWritePointer(0);
        auto*outR=buf.getWritePointer(1);
        for(int i=0;i<ns;++i){
            float sL=0,sR=0;
            for(auto&v:voices){
                if(!v.active)continue;
                float relRate=v.releasing?1.f/(v.sr*0.3f):0;
                v.ampEnv=std::max(0.f,v.ampEnv-relRate);
                if(v.ampEnv<0.0001f&&v.releasing){v.active=false;continue;}
                float ds=v.drift.tick(pDriftR,pDriftD);
                float df=v.freq*std::pow(2.f,ds/12.f);
                float dlen=v.sr/std::max(df,20.f);
                float out=v.dl.read(dlen);
                float exc=v.bowed?v.bow.tick(pBowP,pBowS,v.lastOut):v.pick.tick(pBright);
                float damped=v.df.process(out+exc*0.3f,pDamp);
                v.dl.write(damped);
                float bo=out+v.body.process(out)*0.25f;
                float so=v.symp.process(bo,pSymp);
                float dad=(bo+so)*pJam;
                v.lastOut=out;
                // Interference
                float inlawSig=0;
                if(pMeddling>pMedTh){
                    float amt=(pMeddling-pMedTh)/(1-pMedTh+0.001f);
                    inlawSig=v.inlaw.tick(v.freq,pThScale,pThWob)*amt*pInlaw;
                }
                float obedSig=0;
                if(pMeddling>0.7f){
                    float amt=(pMeddling-0.7f)/0.3f;
                    obedSig=v.obed.tick(v.freq,(int)pFmRatio,pFmIdx,pFmDec)*amt*pObedLvl;
                }
                float interference=(inlawSig+obedSig)*(1-pCommune);
                float jammed=dad+dad*pCommune*(inlawSig+obedSig)*0.2f;
                float sig=(jammed+interference)*v.ampEnv*0.35f;
                sL+=sig;sR+=sig;
            }
            outL[i]+=sL; outR[i]+=sR;
            lastSampleL=sL; lastSampleR=sR;
        }
        activeCount=0;for(auto&v:voices)if(v.active)++activeCount;
    }

    float getSampleForCoupling(int ch,int) const override { return ch==0?lastSampleL:lastSampleR; }

    void applyCouplingInput(CouplingType,float,const float*,int) override {}

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
        using F=juce::AudioParameterFloat; using C=juce::AudioParameterChoice;
        using N=juce::NormalisableRange<float>;
        p.push_back(std::make_unique<C>("ohm_dadInstrument","Dad Instrument",
            juce::StringArray{"Banjo","Guitar","Mandolin","Dobro","Fiddle","Harmonica","Djembe","Kalimba","Sitar"},0));
        p.push_back(std::make_unique<F>("ohm_dadLevel","Dad Level",N{0,1},0.8f));
        p.push_back(std::make_unique<F>("ohm_pluckBrightness","Pick Brightness",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("ohm_bowPressure","Bow Pressure",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("ohm_bowSpeed","Bow Speed",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("ohm_damping","Damping",N{0.8f,0.999f},0.995f));
        p.push_back(std::make_unique<F>("ohm_sympatheticAmt","Sympathetic",N{0,1},0.3f));
        p.push_back(std::make_unique<F>("ohm_driftRate","Drift Rate",N{0.05f,0.5f},0.1f));
        p.push_back(std::make_unique<F>("ohm_driftDepth","Drift Depth",N{0,20},3.0f));
        p.push_back(std::make_unique<F>("ohm_inlawLevel","In-Law Level",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("ohm_thereminScale","Theremin Scale",N{0.5f,2},1.0f));
        p.push_back(std::make_unique<F>("ohm_thereminWobble","Theremin Wobble",N{0,1},0.3f));
        p.push_back(std::make_unique<F>("ohm_obedLevel","Obed Level",N{0,1},0.0f));
        p.push_back(std::make_unique<C>("ohm_fmRatioPreset","FM Ratio",
            juce::StringArray{"H 1:1","C 3:2","N 5:4","O 2:1","Fe 5:3","Au 7:4","U 9:5","Pu 11:6"},0));
        p.push_back(std::make_unique<F>("ohm_fmIndex","FM Index",N{0,8},2.0f));
        p.push_back(std::make_unique<F>("ohm_fmDecay","FM Decay",N{0.01f,4},0.3f));
        p.push_back(std::make_unique<F>("ohm_meddlingThresh","Meddling Thresh",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("ohm_macroJam","JAM",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("ohm_macroMeddling","MEDDLING",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("ohm_macroCommune","COMMUNE",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("ohm_macroMeadow","MEADOW",N{0,1},0.3f));
        return {p.begin(),p.end()};
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override {
        dadInst=apvts.getRawParameterValue("ohm_dadInstrument");
        bright=apvts.getRawParameterValue("ohm_pluckBrightness");
        bowP=apvts.getRawParameterValue("ohm_bowPressure");
        bowS=apvts.getRawParameterValue("ohm_bowSpeed");
        damp=apvts.getRawParameterValue("ohm_damping");
        sympA=apvts.getRawParameterValue("ohm_sympatheticAmt");
        driftR=apvts.getRawParameterValue("ohm_driftRate");
        driftD=apvts.getRawParameterValue("ohm_driftDepth");
        inlawLvl=apvts.getRawParameterValue("ohm_inlawLevel");
        thScale=apvts.getRawParameterValue("ohm_thereminScale");
        thWob=apvts.getRawParameterValue("ohm_thereminWobble");
        obedLvl=apvts.getRawParameterValue("ohm_obedLevel");
        fmRatio=apvts.getRawParameterValue("ohm_fmRatioPreset");
        fmIdx=apvts.getRawParameterValue("ohm_fmIndex");
        fmDec=apvts.getRawParameterValue("ohm_fmDecay");
        medTh=apvts.getRawParameterValue("ohm_meddlingThresh");
        mJam=apvts.getRawParameterValue("ohm_macroJam");
        mMeddling=apvts.getRawParameterValue("ohm_macroMeddling");
        mCommune=apvts.getRawParameterValue("ohm_macroCommune");
    }

    juce::String getEngineId() const override { return "Ohm"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF87AE73); } // Sage
    int getMaxVoices() const override { return kVoices; }
    int getActiveVoiceCount() const override { return activeCount; }

private:
    static constexpr int kVoices=12;
    double sr=44100;
    std::array<OhmAdapterVoice,kVoices> voices;
    int nextV=0, activeCount=0;
    float lastSampleL=0, lastSampleR=0;
    std::vector<float> couplingBuf;

    // Param pointers
    std::atomic<float>*dadInst=nullptr,*bright=nullptr,*bowP=nullptr,*bowS=nullptr;
    std::atomic<float>*damp=nullptr,*sympA=nullptr,*driftR=nullptr,*driftD=nullptr;
    std::atomic<float>*inlawLvl=nullptr,*thScale=nullptr,*thWob=nullptr;
    std::atomic<float>*obedLvl=nullptr,*fmRatio=nullptr,*fmIdx=nullptr,*fmDec=nullptr;
    std::atomic<float>*medTh=nullptr,*mJam=nullptr,*mMeddling=nullptr,*mCommune=nullptr;
};

} // namespace xomnibus
