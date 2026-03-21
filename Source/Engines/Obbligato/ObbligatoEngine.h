#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FamilyWaveguide.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/StandardLFO.h"
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
        note=n;vel=v;freq=440*std::pow(2.f,(n-69.f)/12.f);
        dl.reset();df.reset();body.setParams(freq*1.3f,3.5f);symp.tune(freq);
        ampEnv=v;releasing=false;active=true;
    }
    void noteOff(){releasing=true;}
};

class ObbligatoEngine : public SynthEngine {
public:
    void prepare(double sampleRate,int maxBlockSize) override {
        sr=sampleRate;
        for(auto&v:voices)v.prepare(sampleRate);
        silenceGate.prepare(sampleRate,maxBlockSize);
        chorusLFO.setRate (0.7f, (float)sampleRate);
        chorusLFO.setShape (StandardLFO::Sine);
        phaserLFO.setRate (0.3f, (float)sampleRate);
        phaserLFO.setShape (StandardLFO::Sine);
    }
    void releaseResources() override {for(auto&v:voices)v.reset();}
    void reset() override {for(auto&v:voices)v.reset();lastL=lastR=0;chorusLFO.reset();phaserLFO.reset();}

    void renderBlock(juce::AudioBuffer<float>&buf,juce::MidiBuffer&midi,int ns) override {
        for(const auto m:midi){
            auto msg=m.getMessage();
            if(msg.isNoteOn()){
                silenceGate.wake();
                int t=-1;for(int i=0;i<kV;++i)if(!voices[i].active){t=i;break;}
                if(t<0)t=nv%kV;nv=(t+1)%kV;

                // Voice routing: 0=Alternate, 1=Split(C4), 2=Layer, 3=RoundRobin, 4=Velocity
                int routing=static_cast<int>(pVoiceRouting?pVoiceRouting->load():0);
                bool useBroA=true;
                switch(routing){
                    case 0: useBroA=(t%2==0); break;                          // Alternate
                    case 1: useBroA=(msg.getNoteNumber()<60); break;           // Split at C4
                    case 2: useBroA=true; break;                               // Layer (handled below)
                    case 3: useBroA=(rrCounter++%2==0); break;                 // Round Robin
                    case 4: useBroA=(msg.getVelocity()<80); break;             // Velocity: soft=A, hard=B
                }
                voices[t].isBroA=useBroA;
                voices[t].noteOn(msg.getNoteNumber(),msg.getVelocity()/127.f);

                // Layer mode: steal a second voice for the other brother
                if(routing==2){
                    int t2=-1;for(int i=0;i<kV;++i)if(!voices[i].active&&i!=t){t2=i;break;}
                    if(t2>=0){
                        voices[t2].isBroA=false; // second voice is always Brother B
                        voices[t2].noteOn(msg.getNoteNumber(),msg.getVelocity()/127.f);
                    }
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

        // --- Read all parameter values once per block ---
        // Section A: Brother A
        float pBreathA   = pBrA?pBrA->load():0.7f;
        float pEmbA      = pEmA?pEmA->load():0.5f;
        float pFlutterA  = pFlA?pFlA->load():0.2f;
        float pInstA     = pInA?pInA->load():0;
        float pBodyA     = pBoA?pBoA->load():0.5f;
        // Section B: Brother B
        float pBreathB   = pBrB?pBrB->load():0.7f;
        float pStiff     = pSt?pSt->load():0.5f;
        float pBite      = pBi?pBi->load():0.3f;
        float pInstB     = pInB?pInB->load():0;
        float pBodyB     = pBoB?pBoB->load():0.5f;
        // Section C: Shared voice
        float pDa        = pDamp?pDamp->load():0.995f;
        float pSy        = pSymp?pSymp->load():0.3f;
        float pDR        = pDriftR?pDriftR->load():0.1f;
        float pDD        = pDriftD?pDriftD->load():3;
        // Section D: BOND
        float pBStage    = pBondStg?pBondStg->load():0;
        float pBInt      = pBondInt?pBondInt->load():0.5f;
        float pBRate     = pBondRt?pBondRt->load():0.2f;
        // Section E: FX Chain A "The Air"
        float pChorus    = pFxACh?pFxACh->load():0.3f;
        float pBrDelay   = pFxABD?pFxABD->load():0.2f;
        float pPlate     = pFxAPl?pFxAPl->load():0.2f;
        float pExciter   = pFxAEx?pFxAEx->load():0.1f;
        // Section E: FX Chain B "The Water"
        float pPhaser    = pFxBPh?pFxBPh->load():0.2f;
        float pDkDelay   = pFxBDD?pFxBDD->load():0.3f;
        float pSpring    = pFxBSp?pFxBSp->load():0.2f;
        float pTapeSat   = pFxBTS?pFxBTS->load():0.1f;
        // Section F: Macros
        float pMB        = pMBreath?pMBreath->load():0.5f;
        float pMBond     = pMBond_?pMBond_->load():0;
        float pMMisc     = pMMisc_?pMMisc_->load():0;
        float pMWind     = pMWind_?pMWind_->load():0.3f;

        // --- BOND modulation: 8 emotional stages ---
        // bondStage 0..1 maps to: Harmony, Play, Dare, Fight, Cry, Forgive, Protect, Transcend
        // bondIntensity scales the effect; bondRate controls transition smoothing
        float bondTarget = pBStage * 8.0f; // 0..8 stage range
        bondSmoothed += (bondTarget - bondSmoothed) * std::min(pBRate, 1.0f);
        int bondIdx = std::min(static_cast<int>(bondSmoothed), 7);
        float bondFrac = bondSmoothed - bondIdx;
        // Emotional modulations per stage: [breathMod, detuneMod, sympatheticMod, panSpread]
        // Harmony(0): gentle unison; Fight(3): max detune; Cry(4): max sympathetic; Transcend(7): pure unison
        static constexpr float bondTable[8][4] = {
            {0.0f, 0.00f, 0.2f, 0.1f},   // 0: Harmony
            {0.1f, 0.02f, 0.3f, 0.2f},    // 1: Play
            {0.2f, 0.05f, 0.2f, 0.3f},    // 2: Dare
            {0.3f, 0.10f, 0.1f, 0.5f},    // 3: Fight
            {0.1f, 0.03f, 0.6f, 0.2f},    // 4: Cry
            {0.0f, 0.01f, 0.4f, 0.15f},   // 5: Forgive
            {-0.1f,0.00f, 0.5f, 0.1f},    // 6: Protect
            {0.0f, 0.00f, 0.3f, 0.05f}    // 7: Transcend
        };
        int bondNext = std::min(bondIdx+1, 7);
        float bBreathMod = bondTable[bondIdx][0]*(1-bondFrac) + bondTable[bondNext][0]*bondFrac;
        float bDetuneMod = bondTable[bondIdx][1]*(1-bondFrac) + bondTable[bondNext][1]*bondFrac;
        float bSympMod   = bondTable[bondIdx][2]*(1-bondFrac) + bondTable[bondNext][2]*bondFrac;
        float bPanSpread = bondTable[bondIdx][3]*(1-bondFrac) + bondTable[bondNext][3]*bondFrac;
        // Scale all BOND mods by bondIntensity and macroBond
        float bondScale = pBInt * (pMBond * 2.0f); // macroBond 0.5 = neutral
        bBreathMod *= bondScale; bDetuneMod *= bondScale;
        bSympMod   *= bondScale; bPanSpread *= bondScale;

        // MISCHIEF macro: cross-brother detuning chaos
        float mischiefDetune = pMMisc * 0.08f; // up to ~8 cents of chaos detune

        // Effective breath per brother (macroBreath scales both)
        float effBreathA = std::clamp(pBreathA * pMB + bBreathMod, 0.0f, 1.0f);
        float effBreathB = std::clamp(pBreathB * pMB + bBreathMod, 0.0f, 1.0f);

        // Effective sympathetic (BOND stage modulates)
        float effSymp = std::clamp(pSy + bSympMod, 0.0f, 1.0f);

        // Body resonance scaling per instrument selection
        // InstrumentA: 0=flute,1=piccolo,2=pan flute,3=shakuhachi,4=bansuri,5=ney,6=recorder,7=ocarina
        // Each instrument tweaks body resonance frequency ratio
        static constexpr float bodyRatioA[8] = {1.3f, 2.0f, 0.8f, 1.0f, 0.9f, 1.1f, 1.5f, 0.6f};
        static constexpr float bodyQA[8]     = {3.5f, 4.0f, 2.5f, 5.0f, 3.0f, 4.5f, 3.0f, 6.0f};
        int instAIdx = std::clamp(static_cast<int>(pInstA), 0, 7);
        float bodyFreqRatioA = bodyRatioA[instAIdx] * (0.5f + pBodyA);
        float bodyQValA      = bodyQA[instAIdx];

        // InstrumentB: 0=clarinet,1=oboe,2=bassoon,3=soprano sax,4=duduk,5=zurna,6=shawm,7=musette
        static constexpr float bodyRatioB[8] = {1.2f, 1.8f, 0.6f, 1.4f, 0.7f, 2.0f, 1.6f, 1.0f};
        static constexpr float bodyQB[8]     = {4.0f, 5.0f, 3.0f, 3.5f, 6.0f, 4.0f, 3.5f, 5.5f};
        int instBIdx = std::clamp(static_cast<int>(pInstB), 0, 7);
        float bodyFreqRatioB = bodyRatioB[instBIdx] * (0.5f + pBodyB);
        float bodyQValB      = bodyQB[instBIdx];

        auto*oL=buf.getWritePointer(0);auto*oR=buf.getWritePointer(1);
        for(int i=0;i<ns;++i){
            float sL=0,sR=0;
            for(auto&v:voices){
                if(!v.active)continue;
                float rr=v.releasing?1.f/(v.sr*0.4f):0;
                v.ampEnv=std::max(0.f,v.ampEnv-rr);
                if(v.ampEnv<0.0001f&&v.releasing){v.active=false;continue;}

                // Drift + BOND detune + MISCHIEF chaos
                float mischiefOff = v.isBroA ? mischiefDetune : -mischiefDetune;
                float ds=v.drift.tick(pDR,pDD) + bDetuneMod + mischiefOff;
                float df=v.freq*fastPow2((ds+extPitchMod)/12.f)*PitchBendUtil::semitonesToFreqRatio(pitchBendNorm*2.0f);
                float dlen=v.sr/std::max(df,20.f);
                float out=v.dl.read(dlen);

                // Exciter: Brother A (air jet) or Brother B (reed)
                float velIntens = 0.5f + v.vel * 0.5f; // velocity 0→1 maps to 0.5→1.0x intensity
                float effIntens = extIntens * velIntens;
                float exc;
                if(v.isBroA){
                    // Air flutter modulates breath pressure for flute vibrato
                    float flutterMod = pFlutterA * 0.15f * std::sin(v.drift.tick(5.0f, 1.0f) * 20.0f);
                    exc=v.airJet.tick(std::clamp(effBreathA*velIntens + flutterMod * pEmbA, 0.0f, 1.0f), v.freq)*effIntens;
                }else{
                    // Reed bite adds harmonic edge on top of stiffness
                    float effStiff = std::clamp(pStiff + pBite * 0.3f, 0.0f, 1.0f);
                    exc=v.reed.tick(effBreathB*velIntens, effStiff)*effIntens;
                }

                float damped=v.df.process(out+exc*0.3f,std::clamp(pDa+extDampMod,0.f,1.f));
                v.dl.write(damped);

                // Body resonance tuned per instrument type + body size
                float bfr = v.isBroA ? bodyFreqRatioA : bodyFreqRatioB;
                float bqv = v.isBroA ? bodyQValA : bodyQValB;
                v.body.setParams(v.freq * bfr, bqv);
                float bo=out+v.body.process(out)*0.2f;
                float so=v.symp.process(bo,effSymp);

                // DSP Fix Wave 2B: D001 — velocity shapes brightness, not just amplitude.
                // Higher velocity increases body Q (brighter harmonics) and opens the
                // sympathetic resonance (richer overtone coupling). This was the seance
                // finding: "Constellation-wide pattern: intensity not brightness".
                float velBrightScale = 0.7f + v.vel * 0.6f; // 0.7→1.3x brightness at full vel
                float sig=(bo+so*velBrightScale)*v.ampEnv*0.4f;

                // Stereo panning: A left-ish, B right-ish, modulated by BOND pan spread
                float basePan = v.isBroA ? 0.35f : 0.65f;
                float pan = basePan + (v.isBroA ? -bPanSpread : bPanSpread);
                pan = std::clamp(pan, 0.05f, 0.95f);
                sL+=sig*(1-pan);sR+=sig*pan;
            }

            // --- FX processing (per-sample, lightweight) ---
            // FX Chain A "The Air": chorus + bright delay + plate + exciter
            // Brother A's signal goes through air chain; Brother B through water chain
            // For simplicity in the adapter, apply as parallel wet mix on summed signal

            // Chorus: subtle pitch modulation via LFO
            float chorusMod = chorusLFO.process() * pChorus * 0.003f;
            float chorusL = sL * (1.0f + chorusMod);
            float chorusR = sR * (1.0f - chorusMod);

            // Bright delay: short feedback delay (bright character)
            int bdIdx = brightDelayPos % kDelayLen;
            float bdL = brightDelayBufL[bdIdx];
            float bdR = brightDelayBufR[bdIdx];
            brightDelayBufL[bdIdx] = flushDenormal(chorusL + bdL * 0.4f * pBrDelay);
            brightDelayBufR[bdIdx] = flushDenormal(chorusR + bdR * 0.4f * pBrDelay);
            brightDelayPos++;
            chorusL += bdL * pBrDelay;
            chorusR += bdR * pBrDelay;

            // Plate reverb: simple allpass diffusion
            float plateIn = (chorusL + chorusR) * 0.5f * pPlate;
            plateState = flushDenormal(plateState * 0.85f + plateIn * 0.15f);
            chorusL += plateState * pPlate * 0.5f;
            chorusR += plateState * pPlate * 0.5f;

            // Air exciter: subtle HF harmonic enhancement
            float excIn = (chorusL + chorusR) * 0.5f;
            float excHF = excIn - exciterLP;
            exciterLP = flushDenormal(exciterLP * 0.92f + excIn * 0.08f);
            chorusL += excHF * pExciter * 0.3f;
            chorusR += excHF * pExciter * 0.3f;

            // FX Chain B "The Water": phaser + dark delay + spring + tape sat
            // Phaser: notch sweep
            float phaserMod = phaserLFO.process();
            float notchFreq = 0.1f + phaserMod * 0.05f;
            phaserStateL = flushDenormal(phaserStateL + (chorusL - phaserStateL) * notchFreq);
            phaserStateR = flushDenormal(phaserStateR + (chorusR - phaserStateR) * notchFreq);
            float wetL = chorusL - phaserStateL * pPhaser;
            float wetR = chorusR - phaserStateR * pPhaser;

            // Dark delay: longer, filtered feedback delay
            int ddIdx = darkDelayPos % kDarkDelayLen;
            float ddL = darkDelayBufL[ddIdx];
            float ddR = darkDelayBufR[ddIdx];
            // LP filter the feedback for dark character
            darkDelayLP_L = flushDenormal(darkDelayLP_L * 0.7f + (wetL + ddL * 0.45f * pDkDelay) * 0.3f);
            darkDelayLP_R = flushDenormal(darkDelayLP_R * 0.7f + (wetR + ddR * 0.45f * pDkDelay) * 0.3f);
            darkDelayBufL[ddIdx] = darkDelayLP_L;
            darkDelayBufR[ddIdx] = darkDelayLP_R;
            darkDelayPos++;
            wetL += ddL * pDkDelay;
            wetR += ddR * pDkDelay;

            // Spring reverb: comb-based metallic resonance
            int spIdx = springPos % kSpringLen;
            float spL = springBufL[spIdx];
            float spR = springBufR[spIdx];
            springBufL[spIdx] = flushDenormal(wetL * 0.3f + spL * 0.6f);
            springBufR[spIdx] = flushDenormal(wetR * 0.3f + spR * 0.6f);
            springPos++;
            wetL += spL * pSpring * 0.4f;
            wetR += spR * pSpring * 0.4f;

            // Tape saturation: soft-clip warmth
            if(pTapeSat > 0.001f){
                float drive = 1.0f + pTapeSat * 3.0f;
                wetL = fastTanh(wetL * drive) / drive;
                wetR = fastTanh(wetR * drive) / drive;
            }

            // WIND macro: blends in atmospheric noise floor
            if(pMWind > 0.01f){
                windSeed = windSeed * 1664525u + 1013904223u;
                float windNoise = static_cast<float>(static_cast<int32_t>(windSeed)) * 4.656612e-10f;
                windLP = flushDenormal(windLP * 0.95f + windNoise * 0.05f);
                wetL += windLP * pMWind * 0.08f;
                wetR += windLP * pMWind * 0.08f;
            }

            oL[i]+=wetL;oR[i]+=wetR;lastL=wetL;lastR=wetR;
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

        // Section A: Brother A — Flute Family
        p.push_back(std::make_unique<F>("obbl_breathA","Breath A",N{0,1},0.7f));
        p.push_back(std::make_unique<F>("obbl_embouchureA","Embouchure A",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("obbl_airFlutterA","Air Flutter A",N{0,1},0.2f));
        p.push_back(std::make_unique<C>("obbl_instrumentA","Instrument A",juce::StringArray{"Flute","Piccolo","Pan Flute","Shakuhachi","Bansuri","Ney","Recorder","Ocarina"},0));
        p.push_back(std::make_unique<F>("obbl_bodySizeA","Body A",N{0,1},0.5f));

        // Section B: Brother B — Reed Family
        p.push_back(std::make_unique<F>("obbl_breathB","Breath B",N{0,1},0.7f));
        p.push_back(std::make_unique<F>("obbl_reedStiffness","Reed Stiffness",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("obbl_reedBite","Reed Bite",N{0,1},0.3f));
        p.push_back(std::make_unique<C>("obbl_instrumentB","Instrument B",juce::StringArray{"Clarinet","Oboe","Bassoon","Soprano Sax","Duduk","Zurna","Shawm","Musette"},0));
        p.push_back(std::make_unique<F>("obbl_bodySizeB","Body B",N{0,1},0.5f));

        // Section C: Shared voice controls
        p.push_back(std::make_unique<C>("obbl_voiceRouting","Voice Routing",juce::StringArray{"Alternate","Split","Layer","Round Robin","Velocity"},0));
        p.push_back(std::make_unique<F>("obbl_damping","Damping",N{0.8f,0.999f},0.995f));
        p.push_back(std::make_unique<F>("obbl_sympatheticAmt","Sympathetic",N{0,1},0.3f));
        p.push_back(std::make_unique<F>("obbl_driftRate","Drift Rate",N{0.005f,0.5f},0.1f));
        p.push_back(std::make_unique<F>("obbl_driftDepth","Drift Depth",N{0,20},3.0f));

        // Section D: BOND interaction
        p.push_back(std::make_unique<F>("obbl_bondStage","Bond Stage",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("obbl_bondIntensity","Bond Intensity",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("obbl_bondRate","Bond Rate",N{0.01f,2.0f},0.2f));

        // Section E: FX Chain A "The Air"
        p.push_back(std::make_unique<F>("obbl_fxAChorus","Air Chorus",N{0,1},0.3f));
        p.push_back(std::make_unique<F>("obbl_fxABrightDelay","Bright Delay",N{0,1},0.2f));
        p.push_back(std::make_unique<F>("obbl_fxAPlate","Air Plate",N{0,1},0.2f));
        p.push_back(std::make_unique<F>("obbl_fxAExciter","Air Exciter",N{0,1},0.1f));

        // Section E: FX Chain B "The Water"
        p.push_back(std::make_unique<F>("obbl_fxBPhaser","Water Phaser",N{0,1},0.2f));
        p.push_back(std::make_unique<F>("obbl_fxBDarkDelay","Dark Delay",N{0,1},0.3f));
        p.push_back(std::make_unique<F>("obbl_fxBSpring","Water Spring",N{0,1},0.2f));
        p.push_back(std::make_unique<F>("obbl_fxBTapeSat","Water Tape",N{0,1},0.1f));

        // Section F: Macros
        p.push_back(std::make_unique<F>("obbl_macroBreath","BREATH",N{0,1},0.5f));
        p.push_back(std::make_unique<F>("obbl_macroBond","BOND",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("obbl_macroMischief","MISCHIEF",N{0,1},0.0f));
        p.push_back(std::make_unique<F>("obbl_macroWind","WIND",N{0,1},0.3f));
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
        addParameters(p);
        return {p.begin(),p.end()};
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override {
        // Section A: Brother A
        pBrA      = apvts.getRawParameterValue("obbl_breathA");
        pEmA      = apvts.getRawParameterValue("obbl_embouchureA");
        pFlA      = apvts.getRawParameterValue("obbl_airFlutterA");
        pInA      = apvts.getRawParameterValue("obbl_instrumentA");
        pBoA      = apvts.getRawParameterValue("obbl_bodySizeA");
        // Section B: Brother B
        pBrB      = apvts.getRawParameterValue("obbl_breathB");
        pSt       = apvts.getRawParameterValue("obbl_reedStiffness");
        pBi       = apvts.getRawParameterValue("obbl_reedBite");
        pInB      = apvts.getRawParameterValue("obbl_instrumentB");
        pBoB      = apvts.getRawParameterValue("obbl_bodySizeB");
        // Section C: Shared voice
        pVoiceRouting = apvts.getRawParameterValue("obbl_voiceRouting");
        pDamp     = apvts.getRawParameterValue("obbl_damping");
        pSymp     = apvts.getRawParameterValue("obbl_sympatheticAmt");
        pDriftR   = apvts.getRawParameterValue("obbl_driftRate");
        pDriftD   = apvts.getRawParameterValue("obbl_driftDepth");
        // Section D: BOND
        pBondStg  = apvts.getRawParameterValue("obbl_bondStage");
        pBondInt  = apvts.getRawParameterValue("obbl_bondIntensity");
        pBondRt   = apvts.getRawParameterValue("obbl_bondRate");
        // Section E: FX Chain A
        pFxACh    = apvts.getRawParameterValue("obbl_fxAChorus");
        pFxABD    = apvts.getRawParameterValue("obbl_fxABrightDelay");
        pFxAPl    = apvts.getRawParameterValue("obbl_fxAPlate");
        pFxAEx    = apvts.getRawParameterValue("obbl_fxAExciter");
        // Section E: FX Chain B
        pFxBPh    = apvts.getRawParameterValue("obbl_fxBPhaser");
        pFxBDD    = apvts.getRawParameterValue("obbl_fxBDarkDelay");
        pFxBSp    = apvts.getRawParameterValue("obbl_fxBSpring");
        pFxBTS    = apvts.getRawParameterValue("obbl_fxBTapeSat");
        // Section F: Macros
        pMBreath  = apvts.getRawParameterValue("obbl_macroBreath");
        pMBond_   = apvts.getRawParameterValue("obbl_macroBond");
        pMMisc_   = apvts.getRawParameterValue("obbl_macroMischief");
        pMWind_   = apvts.getRawParameterValue("obbl_macroWind");
    }

    juce::String getEngineId() const override {return "Obbligato";}
    juce::Colour getAccentColour() const override {return juce::Colour(0xFFFF8A7A);} // Rascal Coral
    int getMaxVoices() const override {return kV;}
    int getActiveVoiceCount() const override {return ac;}

private:
    SilenceGate silenceGate;
    static constexpr int kV=12;
    double sr=44100; int nv=0,ac=0; float lastL=0,lastR=0;
    std::array<ObbligatoAdapterVoice,kV> voices;
    int rrCounter=0; // round-robin counter for voice routing mode 3

    float pitchBendNorm = 0.0f; // MIDI pitch wheel [-1, +1]; ±2 semitone range

    // Family coupling ext mods (SP7.3)
    float extPitchMod = 0.f;   // semitones from LFOToPitch
    float extDampMod  = 0.f;   // 0–1 from AmpToFilter
    float extIntens   = 1.f;   // multiplier from EnvToMorph

    // BOND smoothing state
    float bondSmoothed=0;

    // FX LFOs (shared utilities)
    StandardLFO chorusLFO;  // 0.7 Hz sine — chorus pitch modulation
    StandardLFO phaserLFO;  // 0.3 Hz sine — phaser notch sweep

    // FX state: Bright delay (FX Chain A) — ~15ms at 44100
    static constexpr int kDelayLen=661;
    float brightDelayBufL[kDelayLen]={};
    float brightDelayBufR[kDelayLen]={};
    int brightDelayPos=0;

    // FX state: Plate
    float plateState=0;

    // FX state: Air Exciter
    float exciterLP=0;

    // FX state: Phaser (FX Chain B)
    float phaserStateL=0, phaserStateR=0;

    // FX state: Dark delay — ~35ms at 44100
    static constexpr int kDarkDelayLen=1543;
    float darkDelayBufL[kDarkDelayLen]={};
    float darkDelayBufR[kDarkDelayLen]={};
    int darkDelayPos=0;
    float darkDelayLP_L=0, darkDelayLP_R=0;

    // FX state: Spring reverb — short comb ~7ms
    static constexpr int kSpringLen=307;
    float springBufL[kSpringLen]={};
    float springBufR[kSpringLen]={};
    int springPos=0;

    // FX state: Wind noise
    uint32_t windSeed=33333u;
    float windLP=0;

    // --- Cached parameter pointers (30 total) ---
    // Section A: Brother A
    std::atomic<float>*pBrA=nullptr,*pEmA=nullptr,*pFlA=nullptr,*pInA=nullptr,*pBoA=nullptr;
    // Section B: Brother B
    std::atomic<float>*pBrB=nullptr,*pSt=nullptr,*pBi=nullptr,*pInB=nullptr,*pBoB=nullptr;
    // Section C: Shared voice
    std::atomic<float>*pVoiceRouting=nullptr,*pDamp=nullptr,*pSymp=nullptr,*pDriftR=nullptr,*pDriftD=nullptr;
    // Section D: BOND
    std::atomic<float>*pBondStg=nullptr,*pBondInt=nullptr,*pBondRt=nullptr;
    // Section E: FX Chain A "The Air"
    std::atomic<float>*pFxACh=nullptr,*pFxABD=nullptr,*pFxAPl=nullptr,*pFxAEx=nullptr;
    // Section E: FX Chain B "The Water"
    std::atomic<float>*pFxBPh=nullptr,*pFxBDD=nullptr,*pFxBSp=nullptr,*pFxBTS=nullptr;
    // Section F: Macros
    std::atomic<float>*pMBreath=nullptr,*pMBond_=nullptr,*pMMisc_=nullptr,*pMWind_=nullptr;
};

} // namespace xomnibus
