#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FamilyWaveguide.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/Effects/Saturator.h"
#include "../../DSP/Effects/MasterDelay.h"
#include "../../DSP/Effects/LushReverb.h"
#include "../../DSP/Effects/MasterModulation.h"
#include "../../DSP/Effects/GranularSmear.h"
#include <array>
#include <cmath>

namespace xomnibus {

//==============================================================================
// OrphicaMicrosound — Per-voice granular microsound engine.
//
// 4 modes: Stutter (repeat), Scatter (randomized position), Freeze (hold),
// Reverse (backward grain read). Operates on the voice's own waveguide output,
// capturing it into a small per-voice circular buffer and reading back grains.
//==============================================================================
struct OrphicaMicrosound {
    static constexpr int kBufSize = 8192;  // ~186ms @ 44.1k
    static constexpr int kGrains = 4;

    float buffer[kBufSize] {};
    int writePos = 0;
    uint32_t seed = 12345u;

    struct Grain {
        int readPos = 0;
        int remaining = 0;
        int length = 256;
    };
    Grain grains[kGrains] {};

    void reset() {
        std::fill(buffer, buffer + kBufSize, 0.0f);
        writePos = 0;
        for (auto& g : grains) { g.readPos = 0; g.remaining = 0; g.length = 256; }
    }

    // mode: 0=Stutter, 1=Scatter, 2=Freeze, 3=Reverse
    // rate: grain trigger rate (controls grain retrigger interval)
    // size: grain window in samples
    // density: overlap amount 0-1 (controls stagger)
    // scatter: position randomization 0-1
    // mix: dry/wet 0-1
    float process(float input, int mode, float sr, float rate, int sizeSamples,
                  int density, float scatter, float mix) {
        if (mix < 0.001f) {
            buffer[writePos] = input;
            writePos = (writePos + 1) % kBufSize;
            return input;
        }

        // Write input (Freeze mode: stop writing)
        if (mode != 2) {
            buffer[writePos] = input;
            writePos = (writePos + 1) % kBufSize;
        }

        float wet = 0.0f;
        int grainSize = std::max(16, std::min(sizeSamples, kBufSize / 2));
        int retrigInterval = std::max(1, static_cast<int>(sr / std::max(rate, 0.5f)));

        for (int g = 0; g < kGrains; ++g) {
            auto& gr = grains[g];

            if (gr.remaining <= 0) {
                // Retrigger
                gr.length = grainSize;
                gr.remaining = grainSize;

                // Stagger by density: low density = more spacing between grains
                int stagger = static_cast<int>((1.0f - std::min(static_cast<float>(density) / 20.0f, 1.0f))
                              * static_cast<float>(grainSize) * (static_cast<float>(g) / kGrains));
                gr.remaining += stagger;

                // Set read position based on mode
                switch (mode) {
                    case 0: // Stutter: read from just-written position
                        gr.readPos = (writePos - grainSize + kBufSize) % kBufSize;
                        break;
                    case 1: // Scatter: randomized read position
                    {
                        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
                        float rnd = static_cast<float>(seed & 0xFFFF) / 65535.0f;
                        int maxBack = static_cast<int>(scatter * static_cast<float>(kBufSize - grainSize - 1));
                        gr.readPos = (writePos - static_cast<int>(rnd * static_cast<float>(maxBack)) - grainSize + kBufSize) % kBufSize;
                        break;
                    }
                    case 2: // Freeze: read from frozen position
                        gr.readPos = (writePos + g * (kBufSize / kGrains)) % kBufSize;
                        break;
                    case 3: // Reverse: start at write head, read backwards
                        gr.readPos = writePos;
                        break;
                    default:
                        gr.readPos = (writePos - grainSize + kBufSize) % kBufSize;
                        break;
                }
            }

            if (gr.remaining > 0 && gr.remaining <= gr.length) {
                // Hann window
                float phase = static_cast<float>(gr.length - gr.remaining)
                            / static_cast<float>(gr.length);
                float window = 0.5f * (1.0f - std::cos(2.0f * 3.14159265f * phase));

                int rp = gr.readPos % kBufSize;
                if (rp < 0) rp += kBufSize;
                wet += buffer[rp] * window;

                // Advance read position (reverse mode goes backwards)
                if (mode == 3)
                    gr.readPos = (gr.readPos - 1 + kBufSize) % kBufSize;
                else
                    gr.readPos = (gr.readPos + 1) % kBufSize;
            }

            gr.remaining--;
        }

        wet *= (1.0f / kGrains);
        return input * (1.0f - mix) + wet * mix;
    }
};

//==============================================================================
struct OrphicaAdapterVoice {
    bool active=false; int note=0; float vel=0, freq=440, ampEnv=0, sr=44100;
    bool releasing=false;
    FamilyDelayLine dl; FamilyDampingFilter df; FamilyBodyResonance body;
    FamilySympatheticBank symp; FamilyOrganicDrift drift; PluckExciter pluck;
    OrphicaMicrosound micro;

    void prepare(double s){
        sr=(float)s;int md=(int)(sr/20)+8;
        dl.prepare(md);df.prepare();body.prepare(s);symp.prepare(s,512);drift.prepare(s);pluck.prepare(s);
        micro.reset();
    }
    void reset(){
        dl.reset();df.reset();body.reset();symp.reset();drift.reset();pluck.reset();
        micro.reset();active=false;ampEnv=0;
    }
    void noteOn(int n,float v){
        note=n;vel=v;freq=440*std::pow(2.f,(n-69)/12.f);
        dl.reset();df.reset();body.setParams(freq*1.2f,4);symp.tune(freq);
        pluck.trigger(2.5f);ampEnv=v;releasing=false;active=true;
    }
    void noteOff(){releasing=true;}
};

//==============================================================================
class OrphicaEngine : public SynthEngine {
public:
    void prepare(double sampleRate,int maxBlockSize) override {
        sr=sampleRate;
        for(auto&v:voices)v.prepare(sampleRate);
        // FX LOW path
        tapeSatFx.setMode(Saturator::SaturationMode::Tape);
        tapeSatFx.setOutputGain(0.85f);
        tapeSatFx.reset();
        darkDelay.prepare(sampleRate,maxBlockSize);
        darkDelay.setDamping(0.6f);
        darkDelay.setPingPong(0.0f);
        darkDelay.setDiffusion(0.0f);
        deepPlate.prepare(sampleRate);
        deepPlate.setRoomSize(0.85f);
        deepPlate.setDamping(0.55f);
        deepPlate.setWidth(0.8f);
        // FX HIGH path
        shimmerVerb.prepare(sampleRate);
        shimmerVerb.setRoomSize(0.92f);
        shimmerVerb.setDamping(0.15f);
        shimmerVerb.setWidth(1.0f);
        microDelay.prepare(sampleRate,maxBlockSize);
        microDelay.setFeedback(0.2f);
        microDelay.setPingPong(0.6f);
        microDelay.setDamping(0.1f);
        spectralSmear.prepare(sampleRate);
        crystalChorus.prepare(sampleRate,maxBlockSize);
        crystalChorus.setMode(MasterModulation::Mode::Chorus);
        // Sub oscillator state
        subPhaseL=subPhaseR=0.0f;
    }

    void releaseResources() override {for(auto&v:voices)v.reset();}

    void reset() override {
        for(auto&v:voices)v.reset();
        lastL=lastR=0;
        tapeSatFx.reset();darkDelay.reset();deepPlate.reset();
        shimmerVerb.reset();microDelay.reset();spectralSmear.reset();crystalChorus.reset();
        subPhaseL=subPhaseR=0;
    }

    void renderBlock(juce::AudioBuffer<float>&buf,juce::MidiBuffer&midi,int ns) override {
        for(const auto m:midi){
            auto msg=m.getMessage();
            if(msg.isNoteOn()){
                int t=-1;for(int i=0;i<kV;++i)if(!voices[i].active){t=i;break;}
                if(t<0)t=nv%kV;nv=(t+1)%kV;
                voices[t].noteOn(msg.getNoteNumber(),msg.getVelocity()/127.f);
            }else if(msg.isNoteOff())
                for(auto&v:voices)if(v.active&&v.note==msg.getNoteNumber())v.noteOff();
            else if (msg.isChannelPressure()) {
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

        // ---- Read all parameters ------------------------------------------------
        // Section A: Harp strings
        float pMat = strMat  ? strMat->load()  : 0.0f;
        float pBr  = bright  ? bright->load()  : 0.5f;
        float pPos = plkPos  ? plkPos->load()  : 0.5f;
        float pSC  = strCnt  ? strCnt->load()  : 4.0f;
        float pBS  = bodySz  ? bodySz->load()  : 0.5f;
        float pDa  = damp    ? damp->load()    : 0.995f;
        float pSy  = sympA   ? sympA->load()   : 0.3f;
        float pDR  = driftR  ? driftR->load()  : 0.1f;
        float pDD  = driftD  ? driftD->load()  : 3.0f;

        // Section B: Microsound
        float pMiMo = miMode    ? miMode->load()    : 0.0f;
        float pMiRa = miRate    ? miRate->load()     : 5.0f;
        float pMiSz = miSize    ? miSize->load()     : 50.0f;
        float pMiDn = miDens    ? miDens->load()     : 4.0f;
        float pMiSc = miScat    ? miScat->load()     : 0.3f;
        float pMiMx = miMix     ? miMix->load()      : 0.0f;

        // Section C: Crossover
        float pCN  = crossN  ? crossN->load()  : 60.0f;
        float pCB  = crossB  ? crossB->load()  : 6.0f;
        float pLL  = lowLvl  ? lowLvl->load()  : 0.8f;
        float pHL  = hiLvl   ? hiLvl->load()   : 0.8f;

        // Section D: FX LOW
        float pSub = subAmt   ? subAmt->load()   : 0.0f;
        float pTpS = tapSat   ? tapSat->load()    : 0.0f;
        float pDDT = dkDelT   ? dkDelT->load()    : 0.4f;
        float pDDF = dkDelFb  ? dkDelFb->load()   : 0.3f;
        float pDPM = dpPlate  ? dpPlate->load()    : 0.2f;

        // Section E: FX HIGH
        float pShM = shimMix  ? shimMix->load()    : 0.1f;
        float pMDT = miDelT   ? miDelT->load()     : 5.0f;
        float pSpS = specSmr  ? specSmr->load()     : 0.0f;
        float pCCR = crChR    ? crChR->load()       : 1.0f;
        float pCCD = crChD    ? crChD->load()       : 0.3f;

        // Section F: Macros
        float pMP  = mPluck   ? mPluck->load()     : 0.5f;
        float pMF  = mFrac    ? mFrac->load()      : 0.0f;
        float pMS  = mSurf    ? mSurf->load()      : 0.5f;
        float pMD  = mDiv     ? mDiv->load()        : 0.2f;

        // ---- Apply macro modulations --------------------------------------------
        // PLUCK (M1): increases brightness and pluck intensity
        float effBright  = std::min(1.0f, pBr + pMP * 0.3f);
        float pluckGain  = 0.4f + pMP * 0.4f;

        // FRACTURE (M2): drives microsound mix from per-voice toward global
        float effMicroMix = std::min(1.0f, pMiMx + pMF * 0.6f);
        float effMicroScatter = std::min(1.0f, pMiSc + pMF * 0.4f);

        // SURFACE (M3): shifts crossover bias (positive = more treble)
        float surfBias = (pMS - 0.5f) * 0.4f;

        // DIVINE (M4): increases shimmer, deep plate, and spectral effects
        float divShimmer  = std::min(1.0f, pShM + pMD * 0.5f);
        float divPlate    = std::min(1.0f, pDPM + pMD * 0.3f);
        float divSmear    = std::min(1.0f, pSpS + pMD * 0.4f);

        // ---- Material affects damping and pluck character -----------------------
        // 0=Nylon(warm), 1=Steel(bright), 2=Crystal(glassy), 3=Light(ethereal)
        int mat = static_cast<int>(pMat);
        float matBrightMod = (mat == 0) ? -0.1f : (mat == 1) ? 0.05f : (mat == 2) ? 0.15f : 0.2f;
        float matDampMod   = (mat == 0) ? 0.002f : (mat == 1) ? 0.0f : (mat == 2) ? -0.003f : -0.005f;
        effBright = std::clamp(effBright + matBrightMod, 0.0f, 1.0f);
        float effDamp = std::clamp(pDa + matDampMod, 0.8f, 0.999f);

        // ---- Microsound params (pre-compute in samples) -------------------------
        int miSizeSamples = std::max(1, static_cast<int>(pMiSz * 0.001f * static_cast<float>(sr)));
        int microModeInt = static_cast<int>(pMiMo);
        int miDensInt = static_cast<int>(pMiDn);

        // ---- Configure FX LOW ---------------------------------------------------
        tapeSatFx.setDrive(pTpS);
        tapeSatFx.setMix(pTpS > 0.001f ? 1.0f : 0.0f);
        darkDelay.setDelayTime(pDDT * 1000.0f);  // seconds → ms
        darkDelay.setFeedback(pDDF);
        darkDelay.setMix(pDDF > 0.001f || pDDT > 0.001f ? 0.4f : 0.0f);
        deepPlate.setMix(divPlate);

        // ---- Configure FX HIGH --------------------------------------------------
        shimmerVerb.setMix(divShimmer);
        microDelay.setDelayTime(pMDT);  // already in ms
        microDelay.setMix(pMDT > 0.6f ? 0.3f : 0.0f);
        spectralSmear.setSmear(divSmear);
        spectralSmear.setMix(divSmear);
        crystalChorus.setRate(pCCR);
        crystalChorus.setDepth(pCCD);
        crystalChorus.setMix(pCCD > 0.001f ? 0.5f : 0.0f);

        // ---- Prepare LOW/HIGH accumulators --------------------------------------
        // We accumulate voice output into temp buffers, then process FX paths
        std::fill(lowBufL, lowBufL + ns, 0.0f);
        std::fill(lowBufR, lowBufR + ns, 0.0f);
        std::fill(hiBufL, hiBufL + ns, 0.0f);
        std::fill(hiBufR, hiBufR + ns, 0.0f);

        // ---- Per-voice synthesis ------------------------------------------------
        for(int i=0;i<ns;++i){
            float sLowL=0,sLowR=0,sHiL=0,sHiR=0;
            for(auto&v:voices){
                if(!v.active)continue;
                // Amp envelope
                float rr=v.releasing?1.f/(v.sr*0.5f):0;
                v.ampEnv=std::max(0.f,v.ampEnv-rr);
                if(v.ampEnv<0.0001f&&v.releasing){v.active=false;continue;}

                // Organic drift
                float ds=v.drift.tick(pDR,pDD);
                float df=v.freq*fastPow2((ds+extPitchMod)/12.f);
                float dlen=v.sr/std::max(df,20.f);

                // Waveguide read
                float out=v.dl.read(dlen);

                // Pluck exciter — position modulates brightness (bridge=bright, nut=dark)
                float posBright = effBright * (1.0f - pPos * 0.4f);
                float velIntens = 0.5f + v.vel * 0.5f; // velocity 0→1 maps to 0.5→1.0x intensity
                float effIntens = extIntens * velIntens;
                float exc=v.pluck.tick(posBright*velIntens)*effIntens;

                // Damped feedback write
                float damped=v.df.process(out+exc*pluckGain,std::clamp(effDamp+extDampMod,0.f,1.f));
                v.dl.write(damped);

                // Body resonance (size controls frequency and Q)
                float bodyFreq = v.freq * (0.6f + pBS * 0.8f);
                float bodyQ = 2.0f + pBS * 6.0f;
                v.body.setParams(bodyFreq, bodyQ);
                float bo=out+v.body.process(out)*(0.1f+pBS*0.2f);

                // Sympathetic strings (count scales amplitude)
                float sympScale = std::min(pSC / 6.0f, 1.0f);
                float so=v.symp.process(bo,pSy*sympScale);
                float sig=(bo+so)*v.ampEnv*0.4f;

                // Per-voice microsound processing
                sig = v.micro.process(sig, microModeInt, v.sr, pMiRa, miSizeSamples,
                                      miDensInt, effMicroScatter, effMicroMix);

                // Crossover split
                float nf=(float)v.note;
                float hb=std::clamp((nf-pCN)/std::max(pCB,1.f)+0.5f+surfBias,0.f,1.f);
                float lo=sig*(1.0f-hb)*pLL;
                float hi=sig*hb*pHL;

                // Stereo spread: LOW leans L, HIGH leans R
                sLowL+=lo*0.7f; sLowR+=lo*0.3f;
                sHiL+=hi*0.3f;  sHiR+=hi*0.7f;
            }
            lowBufL[i]=sLowL; lowBufR[i]=sLowR;
            hiBufL[i]=sHiL;   hiBufR[i]=sHiR;
        }

        // ---- FX Path LOW: sub → tape sat → dark delay → deep plate --------------

        // Sub harmonic: add octave-below sine to LOW path
        if(pSub>0.001f){
            // Estimate fundamental from most recent active voice
            float subFreq=55.0f;
            for(auto&v:voices)if(v.active){subFreq=v.freq*0.5f;break;}
            float subInc=subFreq/static_cast<float>(sr);
            for(int i=0;i<ns;++i){
                subPhaseL=std::fmod(subPhaseL+subInc,1.0f);
                float sub=std::sin(subPhaseL*6.2831853f)*pSub*0.3f;
                lowBufL[i]+=sub; lowBufR[i]+=sub;
            }
        }

        // Tape saturation
        if(pTpS>0.001f){
            tapeSatFx.processBlock(lowBufL,ns);
            tapeSatFx.processBlock(lowBufR,ns);
        }

        // Dark delay (long, dark repeats)
        darkDelay.processBlock(lowBufL,lowBufR,ns);

        // Deep plate reverb
        deepPlate.processBlock(lowBufL,lowBufR,lowBufL,lowBufR,ns);

        // ---- FX Path HIGH: shimmer → micro delay → spectral smear → crystal chorus

        // Shimmer reverb (octave-up reverb via pitch-shifted feedback)
        shimmerVerb.processBlock(hiBufL,hiBufR,hiBufL,hiBufR,ns);

        // Micro delay (very short, widening)
        microDelay.processBlock(hiBufL,hiBufR,ns);

        // Spectral smear (granular texture dissolve)
        spectralSmear.processBlock(hiBufL,hiBufR,ns);

        // Crystal chorus
        crystalChorus.processBlock(hiBufL,hiBufR,ns);

        // ---- Mix LOW + HIGH to output -------------------------------------------
        auto*oL=buf.getWritePointer(0);auto*oR=buf.getWritePointer(1);
        for(int i=0;i<ns;++i){
            float mL=lowBufL[i]+hiBufL[i];
            float mR=lowBufR[i]+hiBufR[i];
            oL[i]+=mL;oR[i]+=mR;
            lastL=mL;lastR=mR;
        }
        ac=0;for(auto&v:voices)if(v.active)++ac;
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

        // A: Harp Strings (9 params)
        p.push_back(std::make_unique<C>("orph_stringMaterial","String Material",juce::StringArray{"Nylon","Steel","Crystal","Light"},0));
        p.push_back(std::make_unique<F>("orph_pluckBrightness","Pluck Brightness",N{0.0f,1.0f},0.5f));
        p.push_back(std::make_unique<F>("orph_pluckPosition","Pluck Position",N{0.0f,1.0f},0.5f));
        p.push_back(std::make_unique<F>("orph_stringCount","String Count",N{1.0f,6.0f,1.0f},4.0f));
        p.push_back(std::make_unique<F>("orph_bodySize","Body Size",N{0.0f,1.0f},0.5f));
        p.push_back(std::make_unique<F>("orph_sympatheticAmt","Sympathetic",N{0.0f,1.0f},0.3f));
        p.push_back(std::make_unique<F>("orph_damping","Damping",N{0.8f,0.999f},0.995f));
        p.push_back(std::make_unique<F>("orph_driftRate","Drift Rate",N{0.005f,0.5f},0.1f));
        p.push_back(std::make_unique<F>("orph_driftDepth","Drift Depth",N{0.0f,20.0f},3.0f));

        // B: Microsound (6 params)
        p.push_back(std::make_unique<C>("orph_microMode","Micro Mode",juce::StringArray{"Stutter","Scatter","Freeze","Reverse"},0));
        p.push_back(std::make_unique<F>("orph_microRate","Micro Rate",N{0.5f,50.0f},5.0f));
        p.push_back(std::make_unique<F>("orph_microSize","Micro Size",N{5.0f,200.0f},50.0f));
        p.push_back(std::make_unique<F>("orph_microDensity","Micro Density",N{1.0f,20.0f},4.0f));
        p.push_back(std::make_unique<F>("orph_microScatter","Micro Scatter",N{0.0f,1.0f},0.3f));
        p.push_back(std::make_unique<F>("orph_microMix","Micro Mix",N{0.0f,1.0f},0.0f));

        // C: Crossover (4 params)
        p.push_back(std::make_unique<F>("orph_crossoverNote","Crossover Note",N{36.0f,84.0f,1.0f},60.0f));
        p.push_back(std::make_unique<F>("orph_crossoverBlend","Crossover Blend",N{0.0f,12.0f,1.0f},6.0f));
        p.push_back(std::make_unique<F>("orph_lowLevel","Low Level",N{0.0f,1.0f},0.8f));
        p.push_back(std::make_unique<F>("orph_highLevel","High Level",N{0.0f,1.0f},0.8f));

        // D: FX LOW (5 params)
        p.push_back(std::make_unique<F>("orph_subAmount","Sub Amount",N{0.0f,1.0f},0.0f));
        p.push_back(std::make_unique<F>("orph_tapeSat","Tape Sat",N{0.0f,1.0f},0.0f));
        p.push_back(std::make_unique<F>("orph_darkDelayTime","Dark Delay Time",N{0.1f,1.5f},0.4f));
        p.push_back(std::make_unique<F>("orph_darkDelayFb","Dark Delay FB",N{0.0f,0.85f},0.3f));
        p.push_back(std::make_unique<F>("orph_deepPlateMix","Deep Plate",N{0.0f,1.0f},0.2f));

        // E: FX HIGH (5 params)
        p.push_back(std::make_unique<F>("orph_shimmerMix","Shimmer Mix",N{0.0f,1.0f},0.1f));
        p.push_back(std::make_unique<F>("orph_microDelayTime","Micro Delay",N{0.5f,30.0f},5.0f));
        p.push_back(std::make_unique<F>("orph_spectralSmear","Spectral Smear",N{0.0f,1.0f},0.0f));
        p.push_back(std::make_unique<F>("orph_crystalChorusRate","Crystal Rate",N{0.1f,5.0f},1.0f));
        p.push_back(std::make_unique<F>("orph_crystalChorusDpth","Crystal Depth",N{0.0f,1.0f},0.3f));

        // F: Macros (4 params)
        p.push_back(std::make_unique<F>("orph_macroPluck","PLUCK",N{0.0f,1.0f},0.5f));
        p.push_back(std::make_unique<F>("orph_macroFracture","FRACTURE",N{0.0f,1.0f},0.0f));
        p.push_back(std::make_unique<F>("orph_macroSurface","SURFACE",N{0.0f,1.0f},0.5f));
        p.push_back(std::make_unique<F>("orph_macroDivine","DIVINE",N{0.0f,1.0f},0.2f));
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
        addParameters(p);
        return {p.begin(),p.end()};
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override {
        // Section A: Harp Strings
        strMat  = apvts.getRawParameterValue("orph_stringMaterial");
        bright  = apvts.getRawParameterValue("orph_pluckBrightness");
        plkPos  = apvts.getRawParameterValue("orph_pluckPosition");
        strCnt  = apvts.getRawParameterValue("orph_stringCount");
        bodySz  = apvts.getRawParameterValue("orph_bodySize");
        sympA   = apvts.getRawParameterValue("orph_sympatheticAmt");
        damp    = apvts.getRawParameterValue("orph_damping");
        driftR  = apvts.getRawParameterValue("orph_driftRate");
        driftD  = apvts.getRawParameterValue("orph_driftDepth");

        // Section B: Microsound
        miMode  = apvts.getRawParameterValue("orph_microMode");
        miRate  = apvts.getRawParameterValue("orph_microRate");
        miSize  = apvts.getRawParameterValue("orph_microSize");
        miDens  = apvts.getRawParameterValue("orph_microDensity");
        miScat  = apvts.getRawParameterValue("orph_microScatter");
        miMix   = apvts.getRawParameterValue("orph_microMix");

        // Section C: Crossover
        crossN  = apvts.getRawParameterValue("orph_crossoverNote");
        crossB  = apvts.getRawParameterValue("orph_crossoverBlend");
        lowLvl  = apvts.getRawParameterValue("orph_lowLevel");
        hiLvl   = apvts.getRawParameterValue("orph_highLevel");

        // Section D: FX LOW
        subAmt  = apvts.getRawParameterValue("orph_subAmount");
        tapSat  = apvts.getRawParameterValue("orph_tapeSat");
        dkDelT  = apvts.getRawParameterValue("orph_darkDelayTime");
        dkDelFb = apvts.getRawParameterValue("orph_darkDelayFb");
        dpPlate = apvts.getRawParameterValue("orph_deepPlateMix");

        // Section E: FX HIGH
        shimMix = apvts.getRawParameterValue("orph_shimmerMix");
        miDelT  = apvts.getRawParameterValue("orph_microDelayTime");
        specSmr = apvts.getRawParameterValue("orph_spectralSmear");
        crChR   = apvts.getRawParameterValue("orph_crystalChorusRate");
        crChD   = apvts.getRawParameterValue("orph_crystalChorusDpth");

        // Section F: Macros
        mPluck  = apvts.getRawParameterValue("orph_macroPluck");
        mFrac   = apvts.getRawParameterValue("orph_macroFracture");
        mSurf   = apvts.getRawParameterValue("orph_macroSurface");
        mDiv    = apvts.getRawParameterValue("orph_macroDivine");
    }

    juce::String getEngineId() const override {return "Orphica";}
    juce::Colour getAccentColour() const override {return juce::Colour(0xFF7FDBCA);} // Siren Seafoam
    int getMaxVoices() const override {return kV;}
    int getActiveVoiceCount() const override {return ac;}

private:
    static constexpr int kV=16;
    static constexpr int kMaxBlock=4096;
    double sr=44100; int nv=0,ac=0; float lastL=0,lastR=0;

    // Family coupling ext mods (SP7.3)
    float extPitchMod = 0.f;   // semitones from LFOToPitch
    float extDampMod  = 0.f;   // 0–1 from AmpToFilter
    float extIntens   = 1.f;   // multiplier from EnvToMorph
    std::array<OrphicaAdapterVoice,kV> voices;

    // FX LOW processors
    Saturator tapeSatFx;
    MasterDelay darkDelay;
    LushReverb deepPlate;
    float subPhaseL=0, subPhaseR=0;

    // FX HIGH processors
    LushReverb shimmerVerb;
    MasterDelay microDelay;
    GranularSmear spectralSmear;
    MasterModulation crystalChorus;

    // Per-block accumulation buffers (stack-safe at 4096 max)
    float lowBufL[kMaxBlock]{}, lowBufR[kMaxBlock]{};
    float hiBufL[kMaxBlock]{},  hiBufR[kMaxBlock]{};

    // ---- Parameter pointers (34 total) ----------------------------------------
    // Section A: Harp Strings (9)
    std::atomic<float>*strMat=nullptr,*bright=nullptr,*plkPos=nullptr;
    std::atomic<float>*strCnt=nullptr,*bodySz=nullptr;
    std::atomic<float>*sympA=nullptr,*damp=nullptr;
    std::atomic<float>*driftR=nullptr,*driftD=nullptr;

    // Section B: Microsound (6)
    std::atomic<float>*miMode=nullptr,*miRate=nullptr,*miSize=nullptr;
    std::atomic<float>*miDens=nullptr,*miScat=nullptr,*miMix=nullptr;

    // Section C: Crossover (4)
    std::atomic<float>*crossN=nullptr,*crossB=nullptr;
    std::atomic<float>*lowLvl=nullptr,*hiLvl=nullptr;

    // Section D: FX LOW (5)
    std::atomic<float>*subAmt=nullptr,*tapSat=nullptr,*dkDelT=nullptr;
    std::atomic<float>*dkDelFb=nullptr,*dpPlate=nullptr;

    // Section E: FX HIGH (5)
    std::atomic<float>*shimMix=nullptr,*miDelT=nullptr,*specSmr=nullptr;
    std::atomic<float>*crChR=nullptr,*crChD=nullptr;

    // Section F: Macros (4)
    std::atomic<float>*mPluck=nullptr,*mFrac=nullptr;
    std::atomic<float>*mSurf=nullptr,*mDiv=nullptr;
};

} // namespace xomnibus
