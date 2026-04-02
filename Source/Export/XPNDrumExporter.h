#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Core/PresetManager.h"
#include "../Engines/Onset/OnsetEngine.h"
#include "XPNCoverArt.h"

namespace xolokun {

//==============================================================================
// XPNDrumExporter — C++ port of Tools/xpn_drum_export.py.
//
// Generates MPC-compatible drum expansion packs from XOnset (ONSET engine)
// presets. Produces <Program type="Drum"> XPM files with 8 active pads,
// 4 velocity layers per pad, and GM-convention MIDI note mapping.
//
// Pad layout (General MIDI convention):
//   V1 Kick         → Note 36 (C2)
//   V2 Snare        → Note 38 (D2)
//   V3 Closed Hat   → Note 42 (F#2)  MuteGroup 1
//   V4 Open Hat     → Note 46 (A#2)  MuteGroup 1
//   V5 Clap         → Note 39 (D#2)
//   V6 Tom          → Note 41 (F2)
//   V7 Percussion   → Note 43 (G2)
//   V8 FX/Cymbal    → Note 49 (C#3)
//
// Velocity layers per pad:
//   Layer 1: vel 1–31   (ghost)
//   Layer 2: vel 32–63  (soft)
//   Layer 3: vel 64–95  (medium)
//   Layer 4: vel 96–127 (hard)
//
class XPNDrumExporter {
public:

    //==========================================================================
    // Voice definitions
    //==========================================================================

    struct PadVoice {
        const char* name;
        int         midiNote;
        int         muteGroup;   // 0 = none
        int         muteTarget;  // MIDI note this pad mutes (-1 = none)
        bool        oneShot;     // true = play to end regardless of note-off
    };

    //==========================================================================
    // Choke/mute group definitions — configurable beyond just hi-hat
    //==========================================================================
    enum ChokeGroup {
        kChokeNone    = 0,
        kChokeHiHat   = 1,  // closed hat <-> open hat
        kChokeKick    = 2,  // kick choke group (layered kicks)
        kChokeSnare   = 3,  // snare choke group (snare + clap mutual exclusion)
        kChokeTom     = 4,  // tom choke group (multiple toms)
    };

    static constexpr int kNumPads = 8;
    static constexpr int kVelLayers = 4;

    static const PadVoice* getPadLayout()
    {
        //                  name           note  muteGrp        muteTarget  oneShot
        static constexpr PadVoice pads[kNumPads] = {
            { "kick",       36, kChokeKick,    -1, true  },
            { "snare",      38, kChokeSnare,   -1, true  },
            { "closed_hat", 42, kChokeHiHat,   46, true  },  // closed hat mutes open hat
            { "open_hat",   46, kChokeHiHat,   -1, true  },  // open hat does NOT mute closed hat
            { "clap",       39, kChokeSnare,   -1, true  },  // clap chokes with snare
            { "tom",        41, kChokeTom,     -1, true  },
            { "perc",       43, kChokeNone,    -1, true  },
            { "fx",         49, kChokeNone,    -1, true  },
        };
        return pads;
    }

    //==========================================================================
    // Configuration
    //==========================================================================

    struct DrumExportConfig {
        juce::String name;                     // Bundle name
        juce::String manufacturer = "XO_OX Designs";
        juce::String version      = "1.0.0";
        juce::String bundleId;
        juce::File   outputDir;
        double       sampleRate   = 44100.0;
        int          bitDepth     = 24;
        float        renderSeconds = 0.5f;     // Short for drums
        float        tailSeconds   = 0.3f;
        int          coverSeed    = 0;
        bool         generatePreviews = false;  // generate low-quality .mp3 preview per WAV
    };

    //==========================================================================
    // Progress callback
    //==========================================================================

    struct Progress {
        int    currentPreset = 0;
        int    totalPresets  = 0;
        int    currentPad    = 0;
        int    totalPads     = kNumPads;
        juce::String presetName;
        float  overallProgress = 0.0f;
        bool   cancelled = false;
    };

    using ProgressCallback = std::function<void(Progress&)>;

    //==========================================================================
    // Export result
    //==========================================================================

    struct ExportResult {
        bool   success = false;
        juce::String errorMessage;
        juce::File   outputFile;
        int    presetsExported = 0;
        int    samplesRendered = 0;
        int64_t totalSizeBytes = 0;
    };

    //==========================================================================
    // Export entry point — call from worker thread
    //==========================================================================

    ExportResult exportDrumBundle(
        const DrumExportConfig& config,
        const std::vector<PresetData>& onsetPresets,
        ProgressCallback progressCb = nullptr)
    {
        ExportResult result;
        Progress progress;
        progress.totalPresets = (int)onsetPresets.size();

        if (onsetPresets.empty())
        {
            result.errorMessage = "No ONSET presets to export";
            return result;
        }

        // Atomic export: write to temp dir, rename on success
        auto finalDir = config.outputDir.getChildFile(config.name.replace(" ", "_"));
        auto tempDir = config.outputDir.getChildFile(
            "." + config.name.replace(" ", "_") + "_drum_tmp_" +
            juce::String(juce::Time::currentTimeMillis()));

        if (!tempDir.createDirectory())
        {
            result.errorMessage = "Failed to create temp directory";
            return result;
        }

        auto drumsDir = tempDir.getChildFile("Drums");
        if (!drumsDir.createDirectory())
        {
            tempDir.deleteRecursively();
            result.errorMessage = "Failed to create Drums directory";
            return result;
        }

        const auto* pads = getPadLayout();

        for (int pi = 0; pi < (int)onsetPresets.size(); ++pi)
        {
            const auto& preset = onsetPresets[(size_t)pi];
            progress.currentPreset = pi + 1;
            progress.presetName = preset.name;
            progress.overallProgress = (float)pi / (float)onsetPresets.size();

            if (progressCb)
            {
                progressCb(progress);
                if (progress.cancelled)
                {
                    tempDir.deleteRecursively();
                    result.errorMessage = "Export cancelled";
                    return result;
                }
            }

            auto presetSlug = sanitize(preset.name);
            auto presetDir = drumsDir.getChildFile(presetSlug);
            if (!presetDir.createDirectory())
            {
                result.errorMessage = "Failed to create dir for: " + preset.name;
                tempDir.deleteRecursively();
                return result;
            }

            // Render WAVs for each pad × velocity layer
            for (int padIdx = 0; padIdx < kNumPads; ++padIdx)
            {
                progress.currentPad = padIdx + 1;
                const auto& pad = pads[padIdx];

                for (int v = 0; v < kVelLayers; ++v)
                {
                    float velocity = velocityForDrumLayer(v);

                    auto wavName = presetSlug + "_" + juce::String(pad.name)
                                 + "_v" + juce::String(v + 1) + ".wav";
                    auto wavFile = presetDir.getChildFile(wavName);

                    auto wavResult = renderDrumHit(preset, pad.midiNote, velocity,
                                                   config, wavFile);
                    if (!wavResult.success)
                    {
                        result.errorMessage = "Render failed: " + wavResult.error;
                        tempDir.deleteRecursively();
                        return result;
                    }

                    result.samplesRendered++;
                    result.totalSizeBytes += wavFile.getSize();
                }
            }

            // Generate drum program XPM
            auto xpmFile = drumsDir.getChildFile(presetSlug + ".xpm");
            auto xpmResult = writeDrumXPM(xpmFile, preset, presetSlug);
            if (!xpmResult.success)
            {
                result.errorMessage = "XPM write failed: " + xpmResult.error;
                tempDir.deleteRecursively();
                return result;
            }

            result.presetsExported++;
        }

        // Cover art
        auto coverResult = XPNCoverArt::generate(
            "ONSET", config.name, tempDir,
            result.presetsExported, config.version, config.coverSeed);
        if (coverResult.success)
            coverResult.cover1000.copyFileTo(tempDir.getChildFile("Preview.png"));

        // Manifest
        writeManifest(tempDir, config, result.presetsExported);

        // Atomic swap
        if (finalDir.exists())
            finalDir.deleteRecursively();

        if (!tempDir.moveFileTo(finalDir))
        {
            if (tempDir.copyDirectoryTo(finalDir))
                tempDir.deleteRecursively();
            else
            {
                result.errorMessage = "Failed to finalize drum bundle directory";
                return result;
            }
        }

        result.success = true;
        result.outputFile = finalDir;
        return result;
    }

    //==========================================================================
    // Filter ONSET presets from a library
    //==========================================================================

    static std::vector<PresetData> filterOnsetPresets(const std::vector<PresetData>& library)
    {
        std::vector<PresetData> onset;
        for (const auto& p : library)
        {
            for (const auto& eng : p.engines)
            {
                auto upper = eng.toUpperCase();
                if (upper == "ONSET" || upper == "XONSET")
                {
                    onset.push_back(p);
                    break;
                }
            }
        }
        return onset;
    }

private:

    struct IOResult {
        bool success = true;
        juce::String error;
    };

    //==========================================================================
    // Velocity mapping for drum layers
    //==========================================================================

    static float velocityForDrumLayer(int layer)
    {
        // ghost, soft, medium, hard
        static constexpr float vels[] = { 0.2f, 0.5f, 0.75f, 1.0f };
        return vels[juce::jlimit(0, 3, layer)];
    }

    //==========================================================================
    // Velocity ranges per layer
    //==========================================================================

    struct VelRange { int start; int end; };

    static VelRange velRangeForLayer(int layer)
    {
        static constexpr VelRange ranges[kVelLayers] = {
            { 1, 31 }, { 32, 63 }, { 64, 95 }, { 96, 127 }
        };
        return ranges[juce::jlimit(0, 3, layer)];
    }

    //==========================================================================
    // Drum hit rendering — ONSET engine DSP (Phase 1: direct DSP primitives)
    //
    // Instantiates OnsetVoice, reads per-voice preset parameters (or falls back
    // to engine defaults), triggers the voice, and renders sample-by-sample.
    // This is the same DSP path the live engine uses: BridgedTOsc / NoiseBurst /
    // MetallicOsc (Layer X) blended with FM / Modal / KarplusStrong / PhaseDist
    // (Layer O), per-voice amp envelope, transient designer, voice filter, and
    // character stage (grit + warmth).
    //
    // Phase 2 TODO: route through the full OnsetEngine (with XVC coupling,
    // reverb/delay FX, master filter) to capture the complete preset character.
    //==========================================================================

    IOResult renderDrumHit(const PresetData& preset, int note, float velocity,
                           const DrumExportConfig& config, const juce::File& outputFile)
    {
        // ---- 1. Resolve voice index from MIDI note via kVoiceCfg ----
        // kVoiceCfg is a constexpr table inside OnsetEngine — access it through
        // the engine's public static interface by matching on midiNote.
        // Voice 0=Kick(36), 1=Snare(38), 2=HH-C(42), 3=HH-O(46),
        //       4=Clap(39),  5=Tom(45),  6=PercA(37), 7=PercB(44)
        // Alternate kick note: 35 → voice 0.
        static constexpr int kNumVoices = 8;
        struct VoiceCfg {
            int circuit; int defaultAlgorithm; int midiNote;
            float baseFreq; float defaultBlend; float defaultDecay; bool isClap;
        };
        static constexpr VoiceCfg kVoiceDefs[kNumVoices] = {
            { 0, 1, 36,   55.0f, 0.2f, 0.5f,  false },  // Kick
            { 1, 0, 38,  180.0f, 0.5f, 0.3f,  false },  // Snare
            { 2, 0, 42, 8000.0f, 0.7f, 0.05f, false },  // HH-C
            { 2, 0, 46, 8000.0f, 0.7f, 0.4f,  false },  // HH-O
            { 1, 3, 39, 1200.0f, 0.4f, 0.25f, true  },  // Clap
            { 0, 1, 45,  110.0f, 0.3f, 0.4f,  false },  // Tom
            { 0, 2, 37,  220.0f, 0.6f, 0.3f,  false },  // PercA
            { 2, 1, 44,  440.0f, 0.8f, 0.35f, false },  // PercB
        };
        int voiceIdx = -1;
        for (int i = 0; i < kNumVoices; ++i)
        {
            if (kVoiceDefs[i].midiNote == note) { voiceIdx = i; break; }
        }
        if (voiceIdx < 0 && note == 35) voiceIdx = 0;  // alternate kick note
        if (voiceIdx < 0) voiceIdx = 0;  // fallback: render as kick for unmapped notes

        const auto& vcfg = kVoiceDefs[voiceIdx];

        // ---- 2. Read per-voice parameters from preset (with defaults) ----
        // Parameter keys: "perc_v{N}_{param}" where N is 1-based voice index.
        // Stored in preset.parametersByEngine["Onset"] as a JSON object.
        // Fall back to kVoiceDefs defaults when a parameter is absent.
        auto extractParam = [&](const juce::String& key, float fallback) -> float
        {
            const auto it = preset.parametersByEngine.find("Onset");
            if (it == preset.parametersByEngine.end()) return fallback;
            const auto& params = it->second;
            if (params.isObject() && params.hasProperty(key))
            {
                auto v = params.getProperty(juce::Identifier(key), juce::var());
                if (v.isDouble() || v.isInt()) return (float)(double)v;
            }
            return fallback;
        };

        juce::String vPre = "perc_v" + juce::String(voiceIdx + 1) + "_";
        float blend     = extractParam(vPre + "blend",     vcfg.defaultBlend);
        float pitch     = extractParam(vPre + "pitch",     0.0f);
        float decay     = extractParam(vPre + "decay",     vcfg.defaultDecay);
        float tone      = extractParam(vPre + "tone",      0.5f);
        float snap      = extractParam(vPre + "snap",      0.3f);
        float body      = extractParam(vPre + "body",      0.5f);
        float character = extractParam(vPre + "character", 0.0f);
        int   algoMode  = (int)extractParam(vPre + "algoMode",
                                            (float)vcfg.defaultAlgorithm);
        int   envShape  = (int)extractParam(vPre + "envShape", 0.0f);
        float grit      = extractParam("perc_char_grit",   0.0f);
        float warmth    = extractParam("perc_char_warmth", preset.dna.warmth * 0.4f);

        // Clamp algoMode to valid range [0, 3]
        algoMode = juce::jlimit(0, 3, algoMode);
        envShape = juce::jlimit(0, 2, envShape);

        // ---- 3. Build and prepare an OnsetVoice ----
        OnsetVoice voice;
        voice.circuitType = vcfg.circuit;
        voice.isClap      = vcfg.isClap;
        voice.baseFreq    = vcfg.baseFreq;
        voice.voiceIndex  = voiceIdx;
        voice.prepare(config.sampleRate);

        // ---- 4. Trigger the voice ----
        voice.triggerVoice(velocity, blend, algoMode,
                           pitch, decay, tone,
                           snap, body, character, envShape);

        // ---- 5. Precompute equal-power blend gains (same as live engine) ----
        constexpr float halfPi = 1.5707963267948966f;
        float blendGainX = std::cos(blend * halfPi);
        float blendGainO = std::sin(blend * halfPi);

        // ---- 6. Prepare character stage ----
        OnsetCharacterStage charStage;
        charStage.prepare(config.sampleRate);
        charStage.setWarmth(warmth);

        // ---- 7. Render sample-by-sample ----
        int totalSamples = (int)((config.renderSeconds + config.tailSeconds) * config.sampleRate);
        juce::AudioBuffer<float> buffer(2, totalSamples);
        buffer.clear();

        for (int i = 0; i < totalSamples; ++i)
        {
            if (!voice.active) break;  // early-out once voice is silent

            float sample = voice.processSample(blendGainX, blendGainO, algoMode);

            // Character stage: grit (tanh saturation) + warmth (LP filter)
            float left  = sample;
            float right = sample;
            charStage.process(left, right, grit, warmth);

            buffer.setSample(0, i, left);
            buffer.setSample(1, i, right);
        }

        // ---- 8. Peak normalize to -0.3 dBFS ----
        normalizeBuffer(buffer, -0.3f);

        // ---- 9. Optional preview generation ----
        if (config.generatePreviews)
        {
            auto previewFile = outputFile.getParentDirectory().getChildFile(
                outputFile.getFileNameWithoutExtension() + ".mp3");
            generatePreview(buffer, config.sampleRate, previewFile);
        }

        return writeWav(outputFile, buffer, config.sampleRate, config.bitDepth);
    }

    static IOResult writeWav(const juce::File& file, const juce::AudioBuffer<float>& buffer,
                             double sampleRate, int bitDepth)
    {
        file.deleteFile();
        auto stream = file.createOutputStream();
        if (!stream)
            return { false, "Cannot create output stream: " + file.getFullPathName() };

        juce::WavAudioFormat wav;
        auto writer = std::unique_ptr<juce::AudioFormatWriter>(
            wav.createWriterFor(stream.release(), sampleRate,
                               (unsigned int)buffer.getNumChannels(),
                               bitDepth, {}, 0));
        if (!writer)
            return { false, "Cannot create WAV writer" };

        if (!writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()))
            return { false, "Failed to write audio data" };

        return { true, {} };
    }

    //==========================================================================
    // Normalization — peak-scan + gain to target ceiling
    //==========================================================================

    static void normalizeBuffer(juce::AudioBuffer<float>& buffer, float ceilingDb)
    {
        float peak = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto range = buffer.findMinMax(ch, 0, buffer.getNumSamples());
            float chPeak = juce::jmax(std::abs(range.getStart()), std::abs(range.getEnd()));
            if (chPeak > peak) peak = chPeak;
        }

        if (peak < 1e-8f) return; // silence, nothing to normalize

        float targetGain = std::pow(10.0f, ceilingDb / 20.0f);
        float gain = targetGain / peak;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.applyGain(ch, 0, buffer.getNumSamples(), gain);
    }

    //==========================================================================
    // Preview generation — low-quality WAV with .mp3 extension
    // (8-bit, mono, 22050Hz, first 2 seconds — MPC browsers play it)
    //==========================================================================

    static IOResult generatePreview(const juce::AudioBuffer<float>& sourceBuffer,
                                    double sourceSampleRate,
                                    const juce::File& previewFile)
    {
        int previewRate = 22050;
        int maxSourceSamples = juce::jmin(sourceBuffer.getNumSamples(),
                                          (int)(2.0 * sourceSampleRate));
        int previewSamples = (int)((double)maxSourceSamples * previewRate / sourceSampleRate);

        // Mono mixdown + resample
        juce::AudioBuffer<float> preview(1, previewSamples);
        preview.clear();

        for (int i = 0; i < previewSamples; ++i)
        {
            int srcIdx = (int)((double)i * sourceSampleRate / previewRate);
            if (srcIdx >= maxSourceSamples) break;

            float mono = 0.0f;
            for (int ch = 0; ch < sourceBuffer.getNumChannels(); ++ch)
                mono += sourceBuffer.getSample(ch, srcIdx);
            mono /= (float)sourceBuffer.getNumChannels();

            preview.setSample(0, i, mono);
        }

        previewFile.deleteFile();
        auto stream = previewFile.createOutputStream();
        if (!stream)
            return { false, "Cannot create preview stream: " + previewFile.getFullPathName() };

        juce::WavAudioFormat wav;
        auto writer = std::unique_ptr<juce::AudioFormatWriter>(
            wav.createWriterFor(stream.release(), (double)previewRate, 1, 8, {}, 0));
        if (!writer)
            return { false, "Cannot create preview writer" };

        if (!writer->writeFromAudioSampleBuffer(preview, 0, previewSamples))
            return { false, "Failed to write preview data" };

        return { true, {} };
    }

    //==========================================================================
    // Drum program XPM generation (MPCVObject format)
    //==========================================================================

    //==========================================================================
    // Pad color from engine accent — converts engine ID to MPC RGB hex string
    //==========================================================================

    static juce::String padColorForEngine(const juce::String& engineId)
    {
        // Engine accent color lookup (mirrors GalleryColors::accentForEngine)
        struct EngineColor { const char* id; uint32_t rgb; };
        static constexpr EngineColor colors[] = {
            { "Onset",     0x0066FF },
            { "OddfeliX",  0x00A6D6 },
            { "OddOscar",  0xE8839B },
            { "Overdub",   0x6B7B3A },
            { "Odyssey",   0x7B2D8B },
            { "Oblong",    0xE9A84A },
            { "Obese",     0xFF1493 },
            { "Overworld", 0x39FF14 },
            { "Opal",      0xA78BFA },
            { "Orbital",   0xFF6B6B },
            { "Organon",   0x00CED1 },
            { "Ouroboros",  0xFF2D2D },
            { "Obsidian",  0xE8E0D8 },
            { "Overbite",  0xF0EDE8 },
            { "Origami",   0xE63946 },
            { "Oracle",    0x4B0082 },
            { "Obscura",   0x8A9BA8 },
            { "Oceanic",   0x00B4A0 },
            { "Optic",     0x00FF41 },
            { "Oblique",   0xBF40FF },
            { "Ocelot",    0xC5832B },
            { "Osprey",    0x1B4F8A },
            { "Osteria",   0x722F37 },
            { "Owlfish",   0xB8860B },
            { "Ohm",       0x87AE73 },
            { "Orphica",   0x7FDBCA },
            { "Obbligato", 0xFF8A7A },
            { "Ottoni",    0x5B8A72 },
            { "Ole",       0xC9377A },
            { "Ombre",     0x7B6B8A },
            { "Orca",      0x1B2838 },
            { "Octopus",   0xE040FB },
            { "Overlap",   0x00FFB4 },
            { "Outwit",    0xCC6600 },
        };

        auto upper = engineId.toUpperCase();
        for (const auto& c : colors)
        {
            if (juce::String(c.id).toUpperCase() == upper)
            {
                return juce::String::toHexString((int)c.rgb).paddedLeft('0', 6).toUpperCase();
            }
        }

        // Default: Electric Blue (ONSET accent)
        return "0066FF";
    }

    IOResult writeDrumXPM(const juce::File& file, const PresetData& preset,
                          const juce::String& presetSlug)
    {
        // MPCVObject format — the correct MPC drum program structure
        juce::XmlElement root("MPCVObject");
        root.setAttribute("type", "com.akaipro.mpc.drum.program");
        root.setAttribute("version", "2.0");

        auto* program = root.createNewChildElement("Program");
        program->createNewChildElement("ProgramName")->addTextElement(preset.name);

        // Expression mapping for drum programs — aftertouch controls filter
        auto* afterTouch = program->createNewChildElement("AfterTouch");
        afterTouch->createNewChildElement("Destination")->addTextElement("FilterCutoff");
        afterTouch->createNewChildElement("Amount")->addTextElement("30");

        auto* instruments = program->createNewChildElement("Instruments");

        const auto* pads = getPadLayout();

        // Determine engine ID for pad color
        juce::String engineId = "Onset"; // default for drum programs
        if (!preset.engines.isEmpty())
            engineId = preset.engines[0];
        auto padColor = padColorForEngine(engineId);

        // Only emit active pad instruments (not all 128 MIDI notes)
        for (int padIdx = 0; padIdx < kNumPads; ++padIdx)
        {
            const auto& pad = pads[padIdx];

            auto* instrument = instruments->createNewChildElement("Instrument");
            instrument->setAttribute("number", pad.midiNote);

            instrument->createNewChildElement("InstrumentName")
                ->addTextElement(juce::String(pad.name).toUpperCase());

            // One-shot mode: drum hits play to end regardless of note-off
            instrument->createNewChildElement("TriggerMode")
                ->addTextElement(pad.oneShot ? "OneShot" : "Note");

            instrument->createNewChildElement("MuteGroup")
                ->addTextElement(juce::String(pad.muteGroup));

            // Directional mute target: closed hat mutes open hat, not vice versa
            if (pad.muteTarget >= 0)
            {
                instrument->createNewChildElement("MuteTarget")
                    ->addTextElement(juce::String(pad.muteTarget));
            }

            // Pad color from engine accent
            instrument->createNewChildElement("PadColor")
                ->addTextElement(padColor);

            auto* layers = instrument->createNewChildElement("Layers");

            for (int v = 0; v < kVelLayers; ++v)
            {
                auto range = velRangeForLayer(v);
                auto wavName = presetSlug + "_" + juce::String(pad.name)
                             + "_v" + juce::String(v + 1) + ".wav";

                auto* layer = layers->createNewChildElement("Layer");
                layer->setAttribute("index", v);

                layer->createNewChildElement("SampleName")
                    ->addTextElement(presetSlug + "/" + wavName);
                layer->createNewChildElement("VelStart")
                    ->addTextElement(juce::String(range.start));
                layer->createNewChildElement("VelEnd")
                    ->addTextElement(juce::String(range.end));
                layer->createNewChildElement("RootNote")
                    ->addTextElement("0");
                layer->createNewChildElement("KeyTrack")
                    ->addTextElement("True");
            }
        }

        if (!root.writeTo(file))
            return { false, "Failed to write drum XPM: " + file.getFullPathName() };

        return { true, {} };
    }

    //==========================================================================
    // Manifest
    //==========================================================================

    static void writeManifest(const juce::File& bundleDir,
                              const DrumExportConfig& config, int presetCount)
    {
        juce::XmlElement manifest("Expansion");
        manifest.setAttribute("Name", config.name);
        manifest.setAttribute("Manufacturer", config.manufacturer);
        manifest.setAttribute("Version", config.version);
        manifest.setAttribute("ID", config.bundleId);
        manifest.setAttribute("Type", "Drums");
        manifest.setAttribute("PresetCount", presetCount);

        manifest.writeTo(bundleDir.getChildFile("Manifest.xml"));
    }

    //==========================================================================
    // Filename helpers
    //==========================================================================

    static juce::String sanitize(const juce::String& name)
    {
        return name.replaceCharacters(" /\\:*?\"<>|", "__________")
                   .substring(0, 50);
    }
};

} // namespace xolokun
