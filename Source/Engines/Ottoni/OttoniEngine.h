#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FamilyWaveguide.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>

namespace xomnibus {

struct OttoniAdapterVoice {
    bool active=false; int note=0; float vel=0, freq=440, ampEnv=0, sr=44100;
    bool releasing=false;
    FamilyDelayLine dl; FamilyDampingFilter df; FamilyBodyResonance body;
    FamilySympatheticBank symp; FamilyOrganicDrift drift; LipBuzzExciter lipBuzz;

    // Per-voice vibrato phase (teen vibrato)
    float vibPhase=0;

    void prepare(double s){
        sr=(float)s;int md=(int)(sr/20)+8;
        dl.prepare(md);df.prepare();body.prepare(s);symp.prepare(s,512);drift.prepare(s);lipBuzz.prepare(s);
        vibPhase=0;
    }
    void reset(){dl.reset();df.reset();body.reset();symp.reset();drift.reset();lipBuzz.reset();active=false;ampEnv=0;vibPhase=0;}
    void noteOn(int n,float v){
        note=n;vel=v;freq=440*std::pow(2.f,(n-69)/12.f);
        dl.reset();df.reset();body.setParams(freq*0.8f,5);symp.tune(freq);
        ampEnv=v;releasing=false;active=true;vibPhase=0;
    }
    void noteOff(){releasing=true;}
};

class OttoniEngine : public SynthEngine {
public:
    void prepare(double sampleRate,int maxBlockSize) override {sr=sampleRate;for(auto&v:voices)v.prepare(sampleRate);silenceGate.prepare(sampleRate,maxBlockSize);
        // Reset FX state
        revState[0]=revState[1]=revState[2]=revState[3]=0;
        choPhase=0; delWr=0;
        std::fill(delBufL,delBufL+kDelMax,0.f);
        std::fill(delBufR,delBufR+kDelMax,0.f);
    }
    void releaseResources() override {for(auto&v:voices)v.reset();}
    void reset() override {for(auto&v:voices)v.reset();lastL=lastR=0;
        revState[0]=revState[1]=revState[2]=revState[3]=0;
        choPhase=0; delWr=0;
        std::fill(delBufL,delBufL+kDelMax,0.f);
        std::fill(delBufR,delBufR+kDelMax,0.f);
    }

    void renderBlock(juce::AudioBuffer<float>&buf,juce::MidiBuffer&midi,int ns) override {
        for(const auto m:midi){
            auto msg=m.getMessage();
            if(msg.isNoteOn()){
                silenceGate.wake();
                int t=-1;for(int i=0;i<kV;++i)if(!voices[i].active){t=i;break;}
                if(t<0)t=nv%kV;nv=(t+1)%kV;
                voices[t].noteOn(msg.getNoteNumber(),msg.getVelocity()/127.f);
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
        }

        // SilenceGate: skip all DSP if engine has been silent long enough
        if(silenceGate.isBypassed() && midi.isEmpty()){buf.clear();return;}

        // --- Snapshot all 28 params ---
        // Section A: Toddler
        float pTodLvl  = todLvl  ? todLvl->load()  : 0.5f;
        float pTodPres = todPres ? todPres->load() : 0.3f;
        float pTodInst = todInst ? todInst->load() : 0;
        // Section B: Tween
        float pTwLvl   = twLvl   ? twLvl->load()   : 0.5f;
        float pTwEmb   = twEmb   ? twEmb->load()   : 0.5f;
        float pTwValve = twValve ? twValve->load() : 0.5f;
        float pTwInst  = twInst  ? twInst->load()  : 0;
        // Section C: Teen
        float pTnLvl   = tnLvl   ? tnLvl->load()   : 0.5f;
        float pTnEmb   = tnEmb   ? tnEmb->load()   : 0.7f;
        float pTnBore  = tnBore  ? tnBore->load()  : 0.5f;
        float pTnInst  = tnInst  ? tnInst->load()  : 0;
        float pTnVibR  = tnVibR  ? tnVibR->load()  : 5.0f;
        float pTnVibD  = tnVibD  ? tnVibD->load()  : 0.3f;
        // Section D: Shared
        float pDa      = damp    ? damp->load()    : 0.995f;
        float pSy      = sympA   ? sympA->load()   : 0.3f;
        float pDR      = driftR  ? driftR->load()  : 0.1f;
        float pDD      = driftD  ? driftD->load()  : 3;
        // Section E: Foreign harmonics
        float pFS      = forStr  ? forStr->load()  : 0;
        float pFDr     = forDr   ? forDr->load()   : 0;
        float pFC      = forCold ? forCold->load() : 0;
        // Section F: FX
        float pRevSz   = revSz   ? revSz->load()   : 0.3f;
        float pChoR    = choR    ? choR->load()    : 1.0f;
        float pDrv     = drv     ? drv->load()     : 0;
        float pDelMix  = delMix  ? delMix->load()  : 0.2f;
        // Section G: Macros
        float pME      = mEmb    ? mEmb->load()    : 0.5f;
        float pMG      = mGrow   ? mGrow->load()   : 0;
        float pMF      = mFor    ? mFor->load()    : 0;
        float pML      = mLake   ? mLake->load()   : 0.3f;

        // --- Macro expansions ---
        // GROW (M2): age sweep 0=toddler, 0.5=tween, 1.0=teen
        //   Blends voice levels: toddler dominates at low GROW, teen at high
        float growToddler = std::max(0.f, 1.f - pMG * 2.f);        // 1.0 @ 0, 0 @ 0.5+
        float growTween   = 1.f - std::abs(pMG * 2.f - 1.f);       // 0 @ 0, 1.0 @ 0.5, 0 @ 1.0
        float growTeen    = std::max(0.f, pMG * 2.f - 1.f);         // 0 @ 0-0.5, 1.0 @ 1.0
        // Mix voice levels with GROW weighting
        float toddlerMix = pTodLvl * growToddler;
        float tweenMix   = pTwLvl  * growTween;
        float teenMix    = pTnLvl  * growTeen;

        // EMBOUCHURE (M1): global embouchure offset applied to all voices
        // FOREIGN (M3): scales foreign harmonic section
        // LAKE (M4): scales reverb/delay atmosphere

        // Effective embouchure per section (macro modulates)
        float effTodPres = std::min(1.f, pTodPres * (0.5f + pME));
        float effTwEmb   = std::min(1.f, pTwEmb   * (0.5f + pME));
        float effTnEmb   = std::min(1.f, pTnEmb   * (0.5f + pME));

        // Effective foreign amounts (macro scales)
        float effFS   = pFS  * (0.5f + pMF);
        float effFDr  = pFDr * (0.5f + pMF);
        float effFC   = pFC  * (0.5f + pMF);

        // Effective FX amounts (LAKE macro scales reverb + delay)
        float effRevSz  = std::min(1.f, pRevSz  * (0.5f + pML * 1.5f));
        float effDelMix = std::min(1.f, pDelMix * (0.5f + pML * 1.5f));

        // Compute ageScale for LipBuzz from GROW macro
        float ageScale = pMG;

        // --- Instrument-specific body resonance tables (D004 fix) ---
        // Toddler: Conch, Shofar, Didgeridoo, Alphorn, Vuvuzela, Toy Trumpet
        static constexpr float kTodFreq[] = {0.6f, 0.7f, 0.4f, 0.5f, 0.75f, 0.9f};
        static constexpr float kTodQ[]    = {4.f,  5.f,  3.f,  4.5f, 7.f,   3.5f};
        // Tween: Trumpet, Alto Sax, Cornet, Flugelhorn, Trombone, Baritone Sax
        static constexpr float kTwFreq[]  = {0.8f, 0.7f, 0.75f, 0.65f, 0.5f, 0.45f};
        static constexpr float kTwQ[]     = {4.f,  3.f,  4.f,   3.f,   5.f,  3.f  };
        // Teen: French Horn, Trombone, Tuba, Euphonium, Tenor Sax, Dungchen, Serpent, Ophicleide, Sackbut, Bass Sax
        static constexpr float kTnFreq[]  = {0.6f, 0.5f, 0.4f, 0.45f, 0.65f, 0.3f, 0.35f, 0.4f, 0.55f, 0.35f};
        static constexpr float kTnQ[]     = {5.f,  5.f,  4.f,  4.f,   3.f,   6.f,  5.f,   5.f,  5.f,   3.f  };
        int todI = std::min((int)pTodInst, 5);
        int twI  = std::min((int)pTwInst,  5);
        int tnI  = std::min((int)pTnInst,  9);
        // GROW-weighted blend of body resonance across age groups
        float wTot = growToddler + growTween + growTeen;
        float instrFreqMult = (wTot > 0.001f) ?
            (kTodFreq[todI]*growToddler + kTwFreq[twI]*growTween + kTnFreq[tnI]*growTeen) / wTot
            : kTwFreq[twI];
        float instrQ = (wTot > 0.001f) ?
            (kTodQ[todI]*growToddler + kTwQ[twI]*growTween + kTnQ[tnI]*growTeen) / wTot
            : kTwQ[twI];

        // Pre-compute SR-scaled reverb comb lengths (once per block, not per sample)
        int srMul=std::max(1,(int)(sr/44100.0+0.5));
        int combLens[4]={1117*srMul,1277*srMul,1423*srMul,1559*srMul};

        auto*oL=buf.getWritePointer(0);auto*oR=buf.getWritePointer(1);
        for(int i=0;i<ns;++i){
            float sL=0,sR=0;
            for(auto&v:voices){
                if(!v.active)continue;
                float rr=v.releasing?1.f/(v.sr*0.3f):0;
                v.ampEnv=std::max(0.f,v.ampEnv-rr);
                if(v.ampEnv<0.0001f&&v.releasing){v.active=false;continue;}

                // --- Organic drift ---
                float ds=v.drift.tick(pDR,pDD);
                float df=v.freq*fastPow2((ds+extPitchMod)/12.f);

                // --- Teen vibrato ---
                v.vibPhase+=pTnVibR/v.sr;
                if(v.vibPhase>=1.f)v.vibPhase-=1.f;
                float vibrato=fastSin(v.vibPhase*6.2831853f)*pTnVibD*growTeen;
                df*=fastPow2(vibrato*0.5f/12.f); // vibrato in semitone cents

                // --- Foreign harmonics: overtone stretch ---
                float stretch=1+effFS*0.1f;
                float dlen=v.sr/std::max(df*stretch,20.f);

                // --- Foreign drift: microtonal pitch drift ---
                float microDrift=effFDr*std::sin(v.vibPhase*3.7f)*0.02f;
                dlen*=(1.f+microDrift);

                float out=v.dl.read(dlen);

                // --- D001 spectral: velocity scales embouchure tightness (brighter at ff) ---
                float velIntens = 0.5f + v.vel * 0.5f; // velocity 0→1 maps to 0.5→1.0x intensity
                float effIntens = extIntens * velIntens;

                // --- Excitation: blended lip buzz from 3 voice sections ---
                // Toddler: loose lips, low pressure, simple
                float excToddler=v.lipBuzz.tick(df*0.998f, effTodPres*0.4f*velIntens, 0.0f)*toddlerMix;
                // Tween: moderate embouchure, valve modulates pitch slightly
                float tweenPitchMod=1.f+pTwValve*0.003f*std::sin(v.vibPhase*2.1f);
                float excTween=v.lipBuzz.tick(df*tweenPitchMod, effTwEmb*0.7f*velIntens, 0.5f)*tweenMix;
                // Teen: full virtuosity, bore width affects body
                float excTeen=v.lipBuzz.tick(df, std::min(1.f,effTnEmb*velIntens), ageScale)*teenMix;

                float exc=(excToddler+excTween+excTeen)*effIntens;

                // --- Waveguide feedback ---
                // --- Bore width (teen): wider bore = lower damping, darker ---
                float boreDamp=std::clamp(pDa+extDampMod,0.f,1.f)*(1.f-pTnBore*growTeen*0.05f);
                float damped=v.df.process(out+exc*0.3f,boreDamp);

                v.dl.write(flushDenormal(damped));

                // --- Body resonance: instrument-specific freq/Q + foreign cold offset ---
                float bFreq=v.freq*instrFreqMult*(1+effFC*0.3f);
                v.body.setParams(bFreq, instrQ + effFC*4);
                float bo=out+v.body.process(out)*0.2f;

                // --- Sympathetic resonance ---
                float so=v.symp.process(bo,pSy);

                float sig=(bo+so)*v.ampEnv*0.4f;

                // --- Stereo spread from age/grow ---
                float w=0.1f+ageScale*0.4f;
                sL+=sig*(0.5f+w);sR+=sig*(1-w);
            }

            // --- Drive (soft clipping) ---
            if(pDrv>0.001f){
                float dGain=1.f+pDrv*8.f;
                sL=fastTanh(sL*dGain)/dGain;
                sR=fastTanh(sR*dGain)/dGain;
            }

            // --- Chorus (simple stereo modulated delay) ---
            if(pChoR>0.001f){
                choPhase+=pChoR/(float)sr;
                if(choPhase>=1.f)choPhase-=1.f;
                float choMod=std::sin(choPhase*6.2831853f);
                float choDelay=0.005f*(float)sr*(1.f+choMod*0.3f); // ~5ms center
                // Use delay buffer for chorus (read from delay line with modulated offset)
                int choIdx=((delWr-(int)choDelay)%kDelMax+kDelMax)%kDelMax;
                float choL=delBufL[choIdx]*0.3f;
                float choR_=delBufR[choIdx]*0.3f;
                sL+=choL;sR+=choR_;
            }

            // --- Delay (stereo ping-pong style) ---
            float delL=0,delR=0;
            if(effDelMix>0.001f){
                // V011: LAKE scales delay 250ms→2000ms (Schulze's mountain)
                float delTimeSec = 0.25f + pML * 1.75f;
                int delTime = std::min((int)(delTimeSec*(float)sr), kDelMax-1);
                int rdIdx=((delWr-delTime)%kDelMax+kDelMax)%kDelMax;
                float fb = 0.6f - pML * 0.15f; // soften feedback at long times (avoid infinite sustain)
                delL=delBufL[rdIdx]*fb;
                delR=delBufR[rdIdx]*fb;
            }
            delBufL[delWr]=sL+delR*0.4f; // cross-feed for ping-pong
            delBufR[delWr]=sR+delL*0.4f;
            delWr=(delWr+1)%kDelMax;
            sL+=delL*effDelMix;
            sR+=delR*effDelMix;

            // --- Reverb (Schroeder 4-comb approximation) ---
            if(effRevSz>0.001f){
                float revIn=(sL+sR)*0.5f;
                // 4 comb filters (combLens pre-computed before loop)
                float revOut=0;
                for(int c=0;c<4;++c){
                    float fb=0.7f+effRevSz*0.28f; // feedback 0.7-0.98
                    float rd=revComb[c][(revPos[c]-combLens[c]+kRevMax)%kRevMax];
                    float wr=revIn+rd*fb;
                    // LP in feedback for warm tail
                    revState[c]=flushDenormal(revState[c]*0.7f+wr*0.3f);
                    revComb[c][revPos[c]]=revState[c];
                    revPos[c]=(revPos[c]+1)%kRevMax;
                    revOut+=rd;
                }
                revOut*=0.25f;
                sL+=revOut*effRevSz*0.5f;
                sR+=revOut*effRevSz*0.5f;
            }

            oL[i]+=sL;oR[i]+=sR;lastL=sL;lastR=sR;
        }
        ac=0;for(auto&v:voices)if(v.active)++ac;
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

        // Section A: Toddler voice (3 params)
        p.push_back(std::make_unique<F>("otto_toddlerLevel","Toddler Level",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("otto_toddlerPressure","Toddler Pressure",N{0,1},0.3f));
        p.push_back(std::make_unique<C>("otto_toddlerInst","Toddler Inst",juce::StringArray{"Conch","Shofar","Didgeridoo","Alphorn","Vuvuzela","Toy Trumpet"},0));

        // Section B: Tween voice (4 params)
        p.push_back(std::make_unique<F>("otto_tweenLevel","Tween Level",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("otto_tweenEmbouchure","Tween Embouchure",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("otto_tweenValve","Tween Valve",N{0,1},0.5f));
        p.push_back(std::make_unique<C>("otto_tweenInst","Tween Inst",juce::StringArray{"Trumpet","Alto Sax","Cornet","Flugelhorn","Trombone","Baritone Sax"},0));

        // Section C: Teen voice (6 params)
        p.push_back(std::make_unique<F>("otto_teenLevel","Teen Level",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("otto_teenEmbouchure","Teen Embouchure",N{0,1},0.7f));
        p.push_back(std::make_unique<F>("otto_teenBore","Teen Bore",N{0,1},0.5f));
        p.push_back(std::make_unique<C>("otto_teenInst","Teen Inst",juce::StringArray{"French Horn","Trombone","Tuba","Euphonium","Tenor Sax","Dungchen","Serpent","Ophicleide","Sackbut","Bass Sax"},0));
        // D005: rate floor lowered to 0.005 Hz for ultra-slow breathing modulation
        p.push_back(std::make_unique<F>("otto_teenVibratoRate","Teen Vibrato Rate",N{0.005f,8.0f},5.0f));
        p.push_back(std::make_unique<F>("otto_teenVibratoDepth","Teen Vibrato Depth",N{0,1},0.3f));

        // Section D: Shared (4 params)
        p.push_back(std::make_unique<F>("otto_damping","Damping",N{0.8f,0.999f},0.995f));
        p.push_back(std::make_unique<F>("otto_sympatheticAmt","Sympathetic",N{0,1},0.3f));
        p.push_back(std::make_unique<F>("otto_driftRate","Drift Rate",N{0.005f,0.5f},0.1f));
        p.push_back(std::make_unique<F>("otto_driftDepth","Drift Depth",N{0,20},3.0f));

        // Section E: Foreign harmonics (3 params)
        p.push_back(std::make_unique<F>("otto_foreignStretch","Foreign Stretch",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("otto_foreignDrift","Foreign Drift",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("otto_foreignCold","Foreign Cold",N{0,1},0.0f));

        // Section F: FX (4 params)
        p.push_back(std::make_unique<F>("otto_reverbSize","Reverb Size",N{0,1},0.3f));
        p.push_back(std::make_unique<F>("otto_chorusRate","Chorus Rate",N{0.1f,5.0f},1.0f));
        p.push_back(std::make_unique<F>("otto_driveAmount","Drive",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("otto_delayMix","Delay Mix",N{0,1},0.2f));

        // Section G: Macros (4 params)
        p.push_back(std::make_unique<F>("otto_macroEmbouchure","EMBOUCHURE",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("otto_macroGrow","GROW",N{0,1},0.35f));
        p.push_back(std::make_unique<F>("otto_macroForeign","FOREIGN",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("otto_macroLake","LAKE",N{0,1},0.3f));
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
        addParameters(p);
        return {p.begin(),p.end()};
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override {
        // Section A: Toddler
        todLvl  = apvts.getRawParameterValue("otto_toddlerLevel");
        todPres = apvts.getRawParameterValue("otto_toddlerPressure");
        todInst = apvts.getRawParameterValue("otto_toddlerInst");
        // Section B: Tween
        twLvl   = apvts.getRawParameterValue("otto_tweenLevel");
        twEmb   = apvts.getRawParameterValue("otto_tweenEmbouchure");
        twValve = apvts.getRawParameterValue("otto_tweenValve");
        twInst  = apvts.getRawParameterValue("otto_tweenInst");
        // Section C: Teen
        tnLvl   = apvts.getRawParameterValue("otto_teenLevel");
        tnEmb   = apvts.getRawParameterValue("otto_teenEmbouchure");
        tnBore  = apvts.getRawParameterValue("otto_teenBore");
        tnInst  = apvts.getRawParameterValue("otto_teenInst");
        tnVibR  = apvts.getRawParameterValue("otto_teenVibratoRate");
        tnVibD  = apvts.getRawParameterValue("otto_teenVibratoDepth");
        // Section D: Shared
        damp    = apvts.getRawParameterValue("otto_damping");
        sympA   = apvts.getRawParameterValue("otto_sympatheticAmt");
        driftR  = apvts.getRawParameterValue("otto_driftRate");
        driftD  = apvts.getRawParameterValue("otto_driftDepth");
        // Section E: Foreign harmonics
        forStr  = apvts.getRawParameterValue("otto_foreignStretch");
        forDr   = apvts.getRawParameterValue("otto_foreignDrift");
        forCold = apvts.getRawParameterValue("otto_foreignCold");
        // Section F: FX
        revSz   = apvts.getRawParameterValue("otto_reverbSize");
        choR    = apvts.getRawParameterValue("otto_chorusRate");
        drv     = apvts.getRawParameterValue("otto_driveAmount");
        delMix  = apvts.getRawParameterValue("otto_delayMix");
        // Section G: Macros
        mEmb    = apvts.getRawParameterValue("otto_macroEmbouchure");
        mGrow   = apvts.getRawParameterValue("otto_macroGrow");
        mFor    = apvts.getRawParameterValue("otto_macroForeign");
        mLake   = apvts.getRawParameterValue("otto_macroLake");
    }

    juce::String getEngineId() const override {return "Ottoni";}
    juce::Colour getAccentColour() const override {return juce::Colour(0xFF5B8A72);} // Patina
    int getMaxVoices() const override {return kV;}
    int getActiveVoiceCount() const override {return ac;}

private:
    SilenceGate silenceGate;
    static constexpr int kV=12;
    double sr=44100; int nv=0,ac=0; float lastL=0,lastR=0;
    std::array<OttoniAdapterVoice,kV> voices;

    // Family coupling ext mods (SP7.3)
    float extPitchMod = 0.f;   // semitones from LFOToPitch
    float extDampMod  = 0.f;   // 0–1 from AmpToFilter
    float extIntens   = 1.f;   // multiplier from EnvToMorph

    // --- Cached parameter pointers (28 total) ---
    // Section A: Toddler
    std::atomic<float>*todLvl=nullptr,*todPres=nullptr,*todInst=nullptr;
    // Section B: Tween
    std::atomic<float>*twLvl=nullptr,*twEmb=nullptr,*twValve=nullptr,*twInst=nullptr;
    // Section C: Teen
    std::atomic<float>*tnLvl=nullptr,*tnEmb=nullptr,*tnBore=nullptr,*tnInst=nullptr;
    std::atomic<float>*tnVibR=nullptr,*tnVibD=nullptr;
    // Section D: Shared
    std::atomic<float>*damp=nullptr,*sympA=nullptr;
    std::atomic<float>*driftR=nullptr,*driftD=nullptr;
    // Section E: Foreign
    std::atomic<float>*forStr=nullptr,*forDr=nullptr,*forCold=nullptr;
    // Section F: FX
    std::atomic<float>*revSz=nullptr,*choR=nullptr,*drv=nullptr,*delMix=nullptr;
    // Section G: Macros
    std::atomic<float>*mEmb=nullptr,*mGrow=nullptr,*mFor=nullptr,*mLake=nullptr;

    // --- FX state (pre-allocated, no audio-thread alloc) ---
    // Reverb: 4-comb Schroeder
    static constexpr int kRevMax=4096; // extended for 96kHz (max comb = 1559*2 = 3118 < 4096)
    float revComb[4][kRevMax]={};
    int   revPos[4]={};
    float revState[4]={};
    // Chorus phase
    float choPhase=0;
    // Delay buffer (stereo, ~1s at 48kHz)
    static constexpr int kDelMax=96000; // V011: extended for 2000ms at 48kHz
    float delBufL[kDelMax]={};
    float delBufR[kDelMax]={};
    int   delWr=0;
};

} // namespace xomnibus
