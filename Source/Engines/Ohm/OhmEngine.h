#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FamilyWaveguide.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/SRO/SilenceGate.h"
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
//
// 32 canonical ohm_ parameters — matches standalone Parameters.h exactly.
//==============================================================================

// 2-op FM for Obed
struct OhmObedFM {
    static constexpr float kRatios[8][2] = {
        {1,1},{3,2},{5,4},{2,1},{5,3},{7,4},{9,5},{11,6}
    };
    float phase=0, modPhase=0, envLevel=0, sr=44100;
    void prepare(double s){sr=(float)s;} void reset(){phase=modPhase=envLevel=0;}
    void trigger(){envLevel=1.0f;}
    float tick(float freq, int ratio, float index, float attack, float decay){
        int r=std::clamp(ratio,0,7);
        float mf=freq*kRatios[r][0]/kRatios[r][1];
        // Attack/decay envelope: rise during attack phase, then decay
        float rate = (envLevel < 1.0f && attack > 0.001f)
                   ? (1.0f / (sr * attack))    // attack phase
                   : (1.0f / (sr * std::max(decay, 0.01f)));  // decay phase
        if (envLevel >= 0.999f) envLevel = 1.0f; // snap to peak
        envLevel *= (1.0f - rate);
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

// Glass harmonica partial generator
struct OhmGlassPartial {
    float phase=0, sr=44100;
    void prepare(double s){sr=(float)s;} void reset(){phase=0;}
    float tick(float freq, float brightness){
        // Higher brightness = higher partials emphasized
        float partialFreq = freq * (2.0f + brightness * 6.0f);
        phase += partialFreq / sr;
        if (phase >= 1.0f) phase -= 1.0f;
        return std::sin(phase * 6.2831853f);
    }
};

// Simple spectral freeze — holds a single sample value and crossfades
struct OhmSpectralFreeze {
    float held = 0.0f;
    void reset() { held = 0.0f; }
    float process(float input, float freezeAmt) {
        if (freezeAmt < 0.01f) { held = input; return input; }
        held = held * 0.9999f + input * 0.0001f; // very slow update when frozen
        return input * (1.0f - freezeAmt) + held * freezeAmt;
    }
};

// Granular scatter for in-law interference
struct OhmGrainEngine {
    float grainPhase = 0, grainEnv = 0, grainFreq = 440, sr = 44100;
    float scatterPhase = 0;
    int grainCounter = 0, grainLen = 0;
    uint32_t seed = 33333u;

    void prepare(double s) { sr = (float)s; }
    void reset() { grainPhase = grainEnv = 0; grainCounter = 0; }

    float tick(float baseFreq, float grainSizeMs, float density, float scatter) {
        int samplesPerGrain = std::max(1, (int)(sr * grainSizeMs * 0.001f));
        int triggerInterval = std::max(1, (int)(sr / std::max(density, 1.0f)));

        ++grainCounter;
        if (grainCounter >= triggerInterval) {
            grainCounter = 0;
            grainLen = samplesPerGrain;
            // Scatter randomizes the grain pitch
            seed = seed * 1664525u + 1013904223u;
            float rnd = (float)((int32_t)seed) * 4.656612e-10f;
            grainFreq = baseFreq * (1.0f + rnd * scatter * 0.5f);
            grainEnv = 1.0f;
        }

        if (grainLen > 0) {
            --grainLen;
            grainPhase += grainFreq / sr;
            if (grainPhase >= 1.0f) grainPhase -= 1.0f;
            // Hann-ish grain envelope
            grainEnv *= (1.0f - 1.0f / (float)(grainLen + 2));
            return std::sin(grainPhase * 6.2831853f) * grainEnv;
        }
        return 0.0f;
    }
};

// Simple delay effect for master FX
struct OhmDelay {
    std::vector<float> bufL, bufR;
    int writePos = 0, maxLen = 0;
    float sr = 44100;

    void prepare(double sampleRate, int maxSamples) {
        sr = (float)sampleRate;
        maxLen = maxSamples;
        bufL.assign(maxLen, 0.0f);
        bufR.assign(maxLen, 0.0f);
        writePos = 0;
    }
    void reset() {
        std::fill(bufL.begin(), bufL.end(), 0.0f);
        std::fill(bufR.begin(), bufR.end(), 0.0f);
        writePos = 0;
    }
    void process(float& l, float& r, float timeSec, float feedback, float mix) {
        if (maxLen == 0) return;
        int delaySamples = std::clamp((int)(timeSec * sr), 1, maxLen - 1);
        int readPos = ((writePos - delaySamples) % maxLen + maxLen) % maxLen;
        float dlL = bufL[readPos];
        float dlR = bufR[readPos];
        bufL[writePos] = flushDenormal(l + dlL * feedback);
        bufR[writePos] = flushDenormal(r + dlR * feedback);
        writePos = (writePos + 1) % maxLen;
        l += dlL * mix;
        r += dlR * mix;
    }
};

// Simple reverb (Schroeder-style allpass + comb)
struct OhmReverb {
    // 4 comb filters + 2 allpass
    static constexpr int kNumCombs = 4;
    static constexpr int kCombLens[4] = { 1116, 1188, 1277, 1356 };
    static constexpr int kAP1Len = 225, kAP2Len = 556;

    std::vector<float> combBuf[kNumCombs], ap1Buf, ap2Buf;
    int combPos[kNumCombs] = {}, ap1Pos = 0, ap2Pos = 0;
    float combState[kNumCombs] = {};

    void prepare(double /*sampleRate*/) {
        for (int i = 0; i < kNumCombs; ++i) {
            combBuf[i].assign(kCombLens[i], 0.0f);
            combPos[i] = 0;
            combState[i] = 0.0f;
        }
        ap1Buf.assign(kAP1Len, 0.0f); ap1Pos = 0;
        ap2Buf.assign(kAP2Len, 0.0f); ap2Pos = 0;
    }
    void reset() {
        for (int i = 0; i < kNumCombs; ++i) {
            std::fill(combBuf[i].begin(), combBuf[i].end(), 0.0f);
            combState[i] = 0.0f; combPos[i] = 0;
        }
        std::fill(ap1Buf.begin(), ap1Buf.end(), 0.0f); ap1Pos = 0;
        std::fill(ap2Buf.begin(), ap2Buf.end(), 0.0f); ap2Pos = 0;
    }
    float process(float in, float mix) {
        float comb = 0.0f;
        for (int i = 0; i < kNumCombs; ++i) {
            float rd = combBuf[i][combPos[i]];
            combState[i] = flushDenormal(rd * 0.84f + combState[i] * 0.16f);
            combBuf[i][combPos[i]] = flushDenormal(in + combState[i] * 0.75f);
            combPos[i] = (combPos[i] + 1) % kCombLens[i];
            comb += rd;
        }
        comb *= 0.25f;
        // Allpass 1
        float ap1rd = ap1Buf[ap1Pos];
        ap1Buf[ap1Pos] = flushDenormal(comb + ap1rd * 0.5f);
        float ap1out = ap1rd - comb * 0.5f;
        ap1Pos = (ap1Pos + 1) % kAP1Len;
        // Allpass 2
        float ap2rd = ap2Buf[ap2Pos];
        ap2Buf[ap2Pos] = flushDenormal(ap1out + ap2rd * 0.5f);
        float ap2out = ap2rd - ap1out * 0.5f;
        ap2Pos = (ap2Pos + 1) % kAP2Len;

        return in * (1.0f - mix) + flushDenormal(ap2out) * mix;
    }
};

constexpr int OhmReverb::kCombLens[4];

// Single OHM voice
struct OhmAdapterVoice {
    bool active=false; int note=0; float vel=0, freq=440, ampEnv=0;
    bool releasing=false, bowed=false;
    FamilyDelayLine dl; FamilyDampingFilter df; FamilyBodyResonance body;
    FamilySympatheticBank symp; FamilyOrganicDrift drift;
    PickExciter pick; BowExciter bow; OhmObedFM obed; OhmInLaw inlaw;
    OhmGlassPartial glass; OhmSpectralFreeze specFreeze; OhmGrainEngine grain;
    float lastOut=0, sr=44100;

    void prepare(double s){
        sr=(float)s; int md=(int)(sr/20)+8;
        dl.prepare(md); df.prepare(); body.prepare(s); symp.prepare(s,512);
        drift.prepare(s); pick.prepare(s); bow.prepare(s); obed.prepare(s); inlaw.prepare(s);
        glass.prepare(s); grain.prepare(s);
    }
    void reset(){
        dl.reset();df.reset();body.reset();symp.reset();drift.reset();
        pick.reset();bow.reset();obed.reset();inlaw.reset();
        glass.reset();specFreeze.reset();grain.reset();
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
        silenceGate.prepare(sampleRate, maxBlockSize);
        // Prepare master FX
        delay.prepare(sampleRate, (int)(sampleRate * 2.5)); // max 2.5s delay buffer
        reverb.prepare(sampleRate);
    }
    void releaseResources() override { for(auto&v:voices)v.reset(); }
    void reset() override {
        for(auto&v:voices)v.reset();
        delay.reset(); reverb.reset();
        lastSampleL=lastSampleR=0;
    }

    void renderBlock(juce::AudioBuffer<float>&buf,juce::MidiBuffer&midi,int ns) override {
        // MIDI
        for(const auto m:midi){
            auto msg=m.getMessage();
            if(msg.isNoteOn()){
                silenceGate.wake();
                int t=-1;
                for(int i=0;i<kVoices;++i)if(!voices[i].active){t=i;break;}
                if(t<0)t=nextV%kVoices; nextV=(t+1)%kVoices;
                voices[t].bowed=(dadInst&&*dadInst>=3.5f&&*dadInst<=4.5f);
                voices[t].noteOn(msg.getNoteNumber(),msg.getVelocity()/127.f);
            } else if(msg.isNoteOff()){
                for(auto&v:voices)if(v.active&&v.note==msg.getNoteNumber())v.noteOff();
            }
            else if (msg.isChannelPressure()) {
                float atPressure = (float)msg.getChannelPressureValue() / 127.f;
                for (auto& v : voices)
                    if (v.active) v.vel = juce::jmax(v.vel, atPressure);
                atCommune = atPressure; // V010: leaning in pushes COMMUNE upward
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

        // ------- Param reads (all 33 ohm_ params) -------
        // Section A: The Dad
        float pDadInst    = dadInst   ? dadInst->load()   : 0.0f;
        float pDadLevel   = dadLvl    ? dadLvl->load()    : 0.8f;
        float pBright     = bright    ? bright->load()    : 0.5f;
        float pBowP       = bowP      ? bowP->load()      : 0.0f;
        float pBowS       = bowS      ? bowS->load()      : 0.5f;
        float pBodyMat    = bodyMat   ? bodyMat->load()   : 0.0f;
        float pSymp       = sympA     ? sympA->load()     : 0.3f;
        float pDriftR     = driftR    ? driftR->load()    : 0.1f;
        float pDriftD     = driftD    ? driftD->load()    : 3.0f;
        float pDamp       = damp      ? damp->load()      : 0.995f;

        // Section B: The In-Laws
        float pInlaw      = inlawLvl  ? inlawLvl->load()  : 0.0f;
        float pThScale    = thScale   ? thScale->load()   : 1.0f;
        float pThWob      = thWob     ? thWob->load()     : 0.3f;
        float pGlassBrt   = glassBrt  ? glassBrt->load()  : 0.5f;
        float pSpecFreeze = specFrz   ? specFrz->load()   : 0.0f;
        float pGrainSz    = grainSz   ? grainSz->load()   : 80.0f;
        float pGrainDen   = grainDen  ? grainDen->load()  : 12.0f;
        float pGrainScat  = grainScat ? grainScat->load() : 0.3f;

        // Section C: Obed
        float pObedLvl    = obedLvl   ? obedLvl->load()   : 0.0f;
        int   pFmRatio    = (int)(fmRatio ? fmRatio->load() : 0.0f);
        float pFmIdx      = fmIdx     ? fmIdx->load()     : 2.0f;
        float pFmAtk      = fmAtk     ? fmAtk->load()     : 0.01f;
        float pFmDec      = fmDec     ? fmDec->load()     : 0.3f;

        // Section D: Interaction
        float pMedTh      = medTh     ? medTh->load()     : 0.5f;
        float pCommAbsorb = commAbs   ? commAbs->load()   : 0.5f;

        // Section E: FX + Macros
        float pRevMix     = revMix    ? revMix->load()    : 0.2f;
        float pDelTime    = delTime   ? delTime->load()   : 0.4f;
        float pDelFb      = delFb     ? delFb->load()     : 0.35f;
        float pJam        = mJam      ? mJam->load()      : 0.5f;
        float pMeddling   = mMeddling ? mMeddling->load() : 0.0f;
        float pCommune    = mCommune  ? mCommune->load()  : 0.0f;
        float pMeadow     = mMeadow   ? mMeadow->load()  : 0.3f;

        // Combine commune macro with commune absorb + aftertouch (V010)
        float communeTotal = std::min(1.0f, pCommune + pCommAbsorb * 0.5f + atCommune * 0.4f);

        // Body material shapes body resonance Q and frequency scaling
        // 0=Wood(warm,low Q), 1=Metal(bright,high Q), 2=Gourd(mid bloom), 3=Air(airy,diffuse)
        float bodyQ = 3.0f;
        float bodyFreqMul = 1.5f;
        int matIdx = (int)pBodyMat;
        switch(matIdx) {
            case 0: bodyQ = 3.0f;  bodyFreqMul = 1.0f; break;  // Wood: warm, low
            case 1: bodyQ = 8.0f;  bodyFreqMul = 3.0f; break;  // Metal: bright, ringy
            case 2: bodyQ = 5.0f;  bodyFreqMul = 1.5f; break;  // Gourd: mid bloom
            case 3: bodyQ = 1.5f;  bodyFreqMul = 2.5f; break;  // Air: diffuse
        }

        // Meadow macro scales reverb mix and delay feedback
        float meadowRev = pRevMix * (0.5f + pMeadow * 0.5f);   // meadow boosts reverb
        float meadowDelFb = pDelFb * (0.7f + pMeadow * 0.3f);  // meadow extends echo tail

        auto*outL=buf.getWritePointer(0);
        auto*outR=buf.getWritePointer(1);
        // DSP FIX: Autonomous LFO for in-law modulation (SIDES breathing).
        // 0.12 Hz triangle = ~8 second cycle. Modulates in-law interference level
        // so the SIDES of the family breathe even with static macro settings.
        sidesLfoPhase += 0.12 / sr;
        if (sidesLfoPhase >= 1.0) sidesLfoPhase -= 1.0;
        float sidesLfo = 4.0f * std::fabs((float)sidesLfoPhase - 0.5f) - 1.0f; // triangle [-1,1]
        float sidesLfoMod = sidesLfo * 0.15f; // ±15% modulation on in-law level

        for(int i=0;i<ns;++i){
            float sL=0,sR=0;
            int voiceIdx = 0;
            for(auto&v:voices){
                if(!v.active){ ++voiceIdx; continue; }
                float relRate=v.releasing?1.f/(v.sr*0.3f):0;
                v.ampEnv=std::max(0.f,v.ampEnv-relRate);
                if(v.ampEnv<0.0001f&&v.releasing){v.active=false;++voiceIdx;continue;}

                // Update body resonance based on material
                v.body.setParams(v.freq * bodyFreqMul, bodyQ);

                float ds=v.drift.tick(pDriftR,pDriftD);
                float df=v.freq*fastPow2((ds+extPitchMod)/12.f)*PitchBendUtil::semitonesToFreqRatio(pitchBendNorm*2.0f);
                float dlen=v.sr/std::max(df,20.f);
                float out=v.dl.read(dlen);
                float velIntens = 0.5f + v.vel * 0.5f; // velocity 0→1 maps to 0.5→1.0x intensity
                float effIntens = extIntens * velIntens;

                // DSP FIX D001: velocity scales BRIGHTNESS (damping), not just intensity.
                // Higher velocity = higher damping coefficient = brighter string tone.
                // velBright: vel=0 → 0.97 (dull), vel=1 → 1.0 (bright, minimal damping)
                float velBright = 0.97f + v.vel * 0.03f;
                float effDamp = std::clamp(pDamp * velBright + extDampMod, 0.f, 1.f);

                float exc=(v.bowed?v.bow.tick(pBowP*velIntens,pBowS,v.lastOut):v.pick.tick(pBright*velIntens))*effIntens;
                float damped=v.df.process(out+exc*0.3f,effDamp);
                v.dl.write(damped);
                float bo=out+v.body.process(out)*0.25f;
                float so=v.symp.process(bo,pSymp);
                float dad=(bo+so)*pJam*pDadLevel;
                v.lastOut=out;

                // In-law interference (theremin + glass + grain)
                // DSP FIX: SIDES LFO modulates in-law level for autonomous breathing
                float inlawSig=0;
                float effInlaw = std::clamp(pInlaw + sidesLfoMod, 0.f, 1.f);
                if(pMeddling>pMedTh){
                    float amt=(pMeddling-pMedTh)/(1-pMedTh+0.001f);
                    float theremin=v.inlaw.tick(v.freq,pThScale,pThWob);
                    float glassSig=v.glass.tick(v.freq,pGlassBrt)*0.5f;
                    float grainSig=v.grain.tick(v.freq,pGrainSz,pGrainDen,pGrainScat)*0.4f;
                    inlawSig=(theremin+glassSig+grainSig)*amt*effInlaw;
                }
                // Apply spectral freeze to in-law signal
                inlawSig = v.specFreeze.process(inlawSig, pSpecFreeze);

                // Obed FM (uses attack + decay)
                float obedSig=0;
                if(pMeddling>0.7f){
                    float amt=(pMeddling-0.7f)/0.3f;
                    obedSig=v.obed.tick(v.freq,pFmRatio,pFmIdx,pFmAtk,pFmDec)*amt*pObedLvl;
                }

                // COMMUNE interaction: low commune = separate, high commune = Dad absorbs interference
                float interference=(inlawSig+obedSig)*(1-communeTotal);
                float jammed=dad+dad*communeTotal*(inlawSig+obedSig)*0.2f;
                float sig=(jammed+interference)*v.ampEnv*0.35f;
                // V009: per-instrument spatial pan (Tomita — Dad's instruments have positions)
                // Pan table: 0=banjo(L) 1=guitar(C) 2=mandolin(R) 3=dobro(L) 4=fiddle(R)
                //            5=harmonica(C) 6=djembe(L) 7=kalimba(R) 8=sitar(C) 9=ukulele(R)
                static constexpr float kDadPan[]={0.25f,0.5f,0.75f,0.3f,0.7f,0.5f,0.28f,0.72f,0.5f,0.68f};
                int instIdx=std::clamp((int)pDadInst,0,9);
                float pan=kDadPan[instIdx];

                // DSP FIX: Stereo spread — offset pan per voice index so polyphonic voices
                // fan out across the stereo field instead of all summing to the same pan.
                // voiceIdx 0-11 maps to ±0.15 spread around instrument pan position.
                float voiceSpread = ((float)(voiceIdx % kVoices) / (float)(kVoices - 1) - 0.5f) * 0.3f;
                float spreadPan = std::clamp(pan + voiceSpread, 0.05f, 0.95f);

                sL+=sig*(1.f-spreadPan)*2.f;
                sR+=sig*spreadPan*2.f;
                ++voiceIdx;
            }

            // Master FX: reverb then delay (meadow macro scales both)
            sL = reverb.process(sL, meadowRev);
            sR = reverb.process(sR, meadowRev);
            delay.process(sL, sR, pDelTime, meadowDelFb, pDelTime > 0.051f ? 0.5f : 0.0f);

            outL[i]+=sL; outR[i]+=sR;
            lastSampleL=sL; lastSampleR=sR;
        }
        activeCount=0;for(auto&v:voices)if(v.active)++activeCount;
        // SilenceGate: analyze output level for next-block bypass decision
        silenceGate.analyzeBlock(buf.getReadPointer(0),buf.getNumChannels()>1?buf.getReadPointer(1):nullptr,ns);
    }

    float getSampleForCoupling(int ch,int) const override { return ch==0?lastSampleL:lastSampleR; }

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
        using F=juce::AudioParameterFloat; using C=juce::AudioParameterChoice;
        using N=juce::NormalisableRange<float>;

        // Section A: The Dad (10 params)
        p.push_back(std::make_unique<C>("ohm_dadInstrument","Dad Instrument",
            juce::StringArray{"Banjo","Guitar","Mandolin","Dobro","Fiddle","Harmonica","Djembe","Kalimba","Sitar"},0));
        p.push_back(std::make_unique<F>("ohm_dadLevel","Dad Level",N{0.0f,1.0f},0.8f));
        p.push_back(std::make_unique<F>("ohm_pluckBrightness","Pick Brightness",N{0.0f,1.0f},0.5f));
        p.push_back(std::make_unique<F>("ohm_bowPressure","Bow Pressure",N{0.0f,1.0f},0.0f));
        p.push_back(std::make_unique<F>("ohm_bowSpeed","Bow Speed",N{0.0f,1.0f},0.5f));
        p.push_back(std::make_unique<C>("ohm_bodyMaterial","Body Material",
            juce::StringArray{"Wood","Metal","Gourd","Air"},0));
        p.push_back(std::make_unique<F>("ohm_sympatheticAmt","Sympathetic",N{0.0f,1.0f},0.3f));
        p.push_back(std::make_unique<F>("ohm_driftRate","Drift Rate",N{0.005f,0.5f},0.1f));
        p.push_back(std::make_unique<F>("ohm_driftDepth","Drift Depth",N{0.0f,20.0f},3.0f));
        p.push_back(std::make_unique<F>("ohm_damping","Damping",N{0.8f,0.999f},0.995f));

        // Section B: The In-Laws (8 params)
        p.push_back(std::make_unique<F>("ohm_inlawLevel","In-Law Level",N{0.0f,1.0f},0.0f));
        p.push_back(std::make_unique<F>("ohm_thereminScale","Theremin Scale",N{0.5f,2.0f},1.0f));
        p.push_back(std::make_unique<F>("ohm_thereminWobble","Theremin Wobble",N{0.0f,1.0f},0.3f));
        p.push_back(std::make_unique<F>("ohm_glassBrightness","Glass Bright",N{0.0f,1.0f},0.5f));
        p.push_back(std::make_unique<F>("ohm_spectralFreeze","Spectral Freeze",N{0.0f,1.0f},0.0f));
        p.push_back(std::make_unique<F>("ohm_grainSize","Grain Size",N{10.0f,500.0f},80.0f));
        p.push_back(std::make_unique<F>("ohm_grainDensity","Grain Density",N{1.0f,40.0f},12.0f));
        p.push_back(std::make_unique<F>("ohm_grainScatter","Grain Scatter",N{0.0f,1.0f},0.3f));

        // Section C: Obed (5 params)
        p.push_back(std::make_unique<F>("ohm_obedLevel","Obed Level",N{0.0f,1.0f},0.0f));
        p.push_back(std::make_unique<C>("ohm_fmRatioPreset","FM Ratio",
            juce::StringArray{"H 1:1","C 3:2","N 5:4","O 2:1","Fe 5:3","Au 7:4","U 9:5","Pu 11:6"},0));
        p.push_back(std::make_unique<F>("ohm_fmIndex","FM Index",N{0.0f,8.0f},2.0f));
        p.push_back(std::make_unique<F>("ohm_fmAttack","FM Attack",N{0.001f,2.0f},0.01f));
        p.push_back(std::make_unique<F>("ohm_fmDecay","FM Decay",N{0.01f,4.0f},0.3f));

        // Section D: Interaction (2 params)
        p.push_back(std::make_unique<F>("ohm_meddlingThresh","Meddling Thresh",N{0.0f,1.0f},0.5f));
        p.push_back(std::make_unique<F>("ohm_communeAbsorb","Commune Absorb",N{0.0f,1.0f},0.5f));

        // Section E: FX + Macros (7 params)
        p.push_back(std::make_unique<F>("ohm_reverbMix","Reverb Mix",N{0.0f,1.0f},0.2f));
        p.push_back(std::make_unique<F>("ohm_delayTime","Delay Time",N{0.05f,2.0f},0.4f));
        p.push_back(std::make_unique<F>("ohm_delayFeedback","Delay Feedback",N{0.0f,0.9f},0.35f));
        p.push_back(std::make_unique<F>("ohm_macroJam","JAM",N{0.0f,1.0f},0.5f));
        p.push_back(std::make_unique<F>("ohm_macroMeddling","MEDDLING",N{0.0f,1.0f},0.0f));
        p.push_back(std::make_unique<F>("ohm_macroCommune","COMMUNE",N{0.0f,1.0f},0.0f));
        p.push_back(std::make_unique<F>("ohm_macroMeadow","MEADOW",N{0.0f,1.0f},0.3f));
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
        addParameters(p);
        return {p.begin(),p.end()};
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override {
        // Section A: The Dad (10)
        dadInst   = apvts.getRawParameterValue("ohm_dadInstrument");
        dadLvl    = apvts.getRawParameterValue("ohm_dadLevel");
        bright    = apvts.getRawParameterValue("ohm_pluckBrightness");
        bowP      = apvts.getRawParameterValue("ohm_bowPressure");
        bowS      = apvts.getRawParameterValue("ohm_bowSpeed");
        bodyMat   = apvts.getRawParameterValue("ohm_bodyMaterial");
        sympA     = apvts.getRawParameterValue("ohm_sympatheticAmt");
        driftR    = apvts.getRawParameterValue("ohm_driftRate");
        driftD    = apvts.getRawParameterValue("ohm_driftDepth");
        damp      = apvts.getRawParameterValue("ohm_damping");

        // Section B: The In-Laws (8)
        inlawLvl  = apvts.getRawParameterValue("ohm_inlawLevel");
        thScale   = apvts.getRawParameterValue("ohm_thereminScale");
        thWob     = apvts.getRawParameterValue("ohm_thereminWobble");
        glassBrt  = apvts.getRawParameterValue("ohm_glassBrightness");
        specFrz   = apvts.getRawParameterValue("ohm_spectralFreeze");
        grainSz   = apvts.getRawParameterValue("ohm_grainSize");
        grainDen  = apvts.getRawParameterValue("ohm_grainDensity");
        grainScat = apvts.getRawParameterValue("ohm_grainScatter");

        // Section C: Obed (5)
        obedLvl   = apvts.getRawParameterValue("ohm_obedLevel");
        fmRatio   = apvts.getRawParameterValue("ohm_fmRatioPreset");
        fmIdx     = apvts.getRawParameterValue("ohm_fmIndex");
        fmAtk     = apvts.getRawParameterValue("ohm_fmAttack");
        fmDec     = apvts.getRawParameterValue("ohm_fmDecay");

        // Section D: Interaction (2)
        medTh     = apvts.getRawParameterValue("ohm_meddlingThresh");
        commAbs   = apvts.getRawParameterValue("ohm_communeAbsorb");

        // Section E: FX + Macros (8)
        revMix    = apvts.getRawParameterValue("ohm_reverbMix");
        delTime   = apvts.getRawParameterValue("ohm_delayTime");
        delFb     = apvts.getRawParameterValue("ohm_delayFeedback");
        mJam      = apvts.getRawParameterValue("ohm_macroJam");
        mMeddling = apvts.getRawParameterValue("ohm_macroMeddling");
        mCommune  = apvts.getRawParameterValue("ohm_macroCommune");
        mMeadow   = apvts.getRawParameterValue("ohm_macroMeadow");
    }

    juce::String getEngineId() const override { return "Ohm"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF87AE73); } // Sage
    int getMaxVoices() const override { return kVoices; }
    int getActiveVoiceCount() const override { return activeCount; }

private:
    SilenceGate silenceGate;
    static constexpr int kVoices=12;
    double sr=44100;
    std::array<OhmAdapterVoice,kVoices> voices;
    int nextV=0, activeCount=0;
    float lastSampleL=0, lastSampleR=0;
    std::vector<float> couplingBuf;

    float pitchBendNorm = 0.0f; // MIDI pitch wheel [-1, +1]; ±2 semitone range

    // Family coupling ext mods (SP7.3)
    float extPitchMod = 0.f;   // semitones from LFOToPitch
    float extDampMod  = 0.f;   // 0–1 from AmpToFilter
    float extIntens   = 1.f;   // multiplier from EnvToMorph

    // V010: aftertouch → COMMUNE (leaning into the note merges the family)
    float atCommune = 0.f;

    // DSP FIX: autonomous LFO for SIDES breathing (in-law interference modulation)
    double sidesLfoPhase = 0.0;

    // Master FX
    OhmDelay delay;
    OhmReverb reverb;

    // Param pointers — all 33 ohm_ params
    // Section A: The Dad (10)
    std::atomic<float>*dadInst=nullptr, *dadLvl=nullptr, *bright=nullptr;
    std::atomic<float>*bowP=nullptr, *bowS=nullptr, *bodyMat=nullptr;
    std::atomic<float>*sympA=nullptr, *driftR=nullptr, *driftD=nullptr, *damp=nullptr;
    // Section B: The In-Laws (8)
    std::atomic<float>*inlawLvl=nullptr, *thScale=nullptr, *thWob=nullptr;
    std::atomic<float>*glassBrt=nullptr, *specFrz=nullptr;
    std::atomic<float>*grainSz=nullptr, *grainDen=nullptr, *grainScat=nullptr;
    // Section C: Obed (5)
    std::atomic<float>*obedLvl=nullptr, *fmRatio=nullptr, *fmIdx=nullptr;
    std::atomic<float>*fmAtk=nullptr, *fmDec=nullptr;
    // Section D: Interaction (2)
    std::atomic<float>*medTh=nullptr, *commAbs=nullptr;
    // Section E: FX + Macros (8)
    std::atomic<float>*revMix=nullptr, *delTime=nullptr, *delFb=nullptr;
    std::atomic<float>*mJam=nullptr, *mMeddling=nullptr, *mCommune=nullptr, *mMeadow=nullptr;
};

} // namespace xomnibus
