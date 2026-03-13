#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Core/PresetManager.h"
#include "XPNCoverArt.h"

namespace xomnibus {

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
//   Layer 1: vel 0–31   (ghost)
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
    };

    static constexpr int kNumPads = 8;
    static constexpr int kVelLayers = 4;

    static const PadVoice* getPadLayout()
    {
        static constexpr PadVoice pads[kNumPads] = {
            { "kick",       36, 0 },
            { "snare",      38, 0 },
            { "closed_hat", 42, 1 },
            { "open_hat",   46, 1 },
            { "clap",       39, 0 },
            { "tom",        41, 0 },
            { "perc",       43, 0 },
            { "fx",         49, 0 },
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
        double       sampleRate   = 48000.0;
        int          bitDepth     = 24;
        float        renderSeconds = 0.5f;     // Short for drums
        float        tailSeconds   = 0.3f;
        int          coverSeed    = 0;
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
            { 0, 31 }, { 32, 63 }, { 64, 95 }, { 96, 127 }
        };
        return ranges[juce::jlimit(0, 3, layer)];
    }

    //==========================================================================
    // Drum hit rendering (placeholder — same pattern as XPNExporter)
    //==========================================================================

    IOResult renderDrumHit(const PresetData& /*preset*/, int note, float velocity,
                           const DrumExportConfig& config, const juce::File& outputFile)
    {
        int totalSamples = (int)((config.renderSeconds + config.tailSeconds) * config.sampleRate);

        juce::AudioBuffer<float> buffer(2, totalSamples);
        buffer.clear();

        // NOTE: Full implementation will:
        // 1. Load ONSET engine with preset parameters
        // 2. Trigger the specific voice (kick/snare/etc.) at the given velocity
        // 3. Render the hit + tail

        (void)note;
        (void)velocity;

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
    // Drum program XPM generation
    //==========================================================================

    IOResult writeDrumXPM(const juce::File& file, const PresetData& preset,
                          const juce::String& presetSlug)
    {
        juce::XmlElement root("Program");
        root.setAttribute("type", "Drum");
        root.setAttribute("name", preset.name);

        const auto* pads = getPadLayout();

        // 128 instruments (MIDI notes 0-127), only 8 are active
        for (int midiNote = 0; midiNote < 128; ++midiNote)
        {
            auto* instrument = root.createNewChildElement("Instrument");
            instrument->setAttribute("number", midiNote);

            // Check if this note has an active pad
            int activePad = -1;
            for (int p = 0; p < kNumPads; ++p)
            {
                if (pads[p].midiNote == midiNote)
                {
                    activePad = p;
                    break;
                }
            }

            if (activePad >= 0)
            {
                const auto& pad = pads[activePad];
                instrument->setAttribute("name", juce::String(pad.name).toUpperCase());

                // Mute group
                if (pad.muteGroup > 0)
                    instrument->setAttribute("MuteGroup", pad.muteGroup);

                // Velocity layers
                for (int v = 0; v < kVelLayers; ++v)
                {
                    auto range = velRangeForLayer(v);
                    auto wavName = presetSlug + "_" + juce::String(pad.name)
                                 + "_v" + juce::String(v + 1) + ".wav";

                    auto* layer = instrument->createNewChildElement("Layer");
                    layer->setAttribute("SampleFile",
                        presetSlug + "/" + wavName);
                    layer->setAttribute("VelStart", range.start);
                    layer->setAttribute("VelEnd", range.end);
                    layer->setAttribute("RootNote", 0);
                }
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

} // namespace xomnibus
