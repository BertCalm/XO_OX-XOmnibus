#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Core/PresetManager.h"
#include "../Core/EngineRegistry.h"
#include "../Core/MegaCouplingMatrix.h"

namespace xomnibus {

//==============================================================================
// XPNExporter — Renders XOmnibus presets to WAV samples and packages them
// as MPC-compatible .xpn expansion packs.
//
// IMPORTANT: All rendering runs on a worker thread (never the audio thread).
// The exporter creates a temporary processor instance for offline rendering.
//
// 3 Critical XPM Rules (non-negotiable):
//   1. KeyTrack = True      — samples transpose across keygroup zones
//   2. RootNote = 0         — MPC auto-detect convention
//   3. Empty VelStart = 0   — prevents ghost triggering
//
class XPNExporter {
public:

    //==========================================================================
    // Configuration
    //==========================================================================

    struct RenderSettings {
        double sampleRate    = 48000.0;
        int    bitDepth      = 24;             // 16 or 24
        float  renderSeconds = 4.0f;           // note hold time
        float  tailSeconds   = 2.0f;           // after noteOff
        float  normCeiling   = -0.3f;          // dBFS normalization target
        int    velocityLayers = 1;             // 1-3

        // Note sampling strategy
        enum class NoteStrategy { EveryMinor3rd, Chromatic, EveryFifth, OctavesOnly };
        NoteStrategy noteStrategy = NoteStrategy::EveryMinor3rd;
    };

    struct BundleConfig {
        juce::String name;                     // e.g. "XOmnibus - Foundation"
        juce::String manufacturer = "XO_OX Designs";
        juce::String version      = "1.0.0";
        juce::String bundleId;                 // e.g. "com.xo-ox.xomnibus.foundation"
        juce::String description;
        juce::File   outputDir;
    };

    //==========================================================================
    // Progress callback
    //==========================================================================

    struct Progress {
        int    currentPreset = 0;
        int    totalPresets  = 0;
        int    currentNote   = 0;
        int    totalNotes    = 0;
        juce::String presetName;
        float  overallProgress = 0.0f; // 0-1
        bool   cancelled = false;
    };

    using ProgressCallback = std::function<void(const Progress&)>;

    //==========================================================================
    // Export entry point — call from worker thread
    //==========================================================================

    struct ExportResult {
        bool   success = false;
        juce::String errorMessage;
        juce::File   outputFile;
        int    presetsExported = 0;
        int    samplesRendered = 0;
        int64_t totalSizeBytes = 0;
    };

    ExportResult exportBundle(
        const BundleConfig& config,
        const RenderSettings& settings,
        const std::vector<PresetData>& presets,
        ProgressCallback progressCb = nullptr)
    {
        ExportResult result;
        Progress progress;
        progress.totalPresets = (int)presets.size();

        if (presets.empty())
        {
            result.errorMessage = "No presets to export";
            return result;
        }

        // Create output directory structure
        auto bundleDir = config.outputDir.getChildFile(config.name.replace(" ", "_"));
        bundleDir.createDirectory();
        auto keyGroupDir = bundleDir.getChildFile("Keygroups");
        keyGroupDir.createDirectory();

        for (int pi = 0; pi < (int)presets.size(); ++pi)
        {
            const auto& preset = presets[(size_t)pi];
            progress.currentPreset = pi + 1;
            progress.presetName = preset.name;
            progress.overallProgress = (float)pi / (float)presets.size();

            if (progressCb)
            {
                progressCb(progress);
                if (progress.cancelled) break;
            }

            // Render preset to WAV files
            auto presetDir = keyGroupDir.getChildFile(sanitizeFilename(preset.name));
            presetDir.createDirectory();

            auto notes = getNotesToRender(settings);
            progress.totalNotes = (int)notes.size() * settings.velocityLayers;
            int noteIdx = 0;

            for (int note : notes)
            {
                for (int vel = 0; vel < settings.velocityLayers; ++vel)
                {
                    progress.currentNote = ++noteIdx;
                    float velocity = velocityForLayer(vel, settings.velocityLayers);

                    auto wavFile = presetDir.getChildFile(
                        wavFilename(preset.name, note, vel));

                    renderNoteToWav(preset, note, velocity, settings, wavFile);
                    result.samplesRendered++;
                    result.totalSizeBytes += wavFile.getSize();
                }
            }

            // Generate XPM keygroup program
            auto xpmFile = keyGroupDir.getChildFile(sanitizeFilename(preset.name) + ".xpm");
            writeXPM(xpmFile, preset, notes, settings);

            result.presetsExported++;
        }

        // Write bundle manifest
        writeManifest(bundleDir, config, result.presetsExported);

        result.success = !progress.cancelled;
        result.outputFile = bundleDir;
        return result;
    }

    //==========================================================================
    // Validation
    //==========================================================================

    struct ValidationResult {
        bool valid = true;
        juce::StringArray warnings;
        juce::StringArray errors;
    };

    static ValidationResult validatePreset(const PresetData& preset)
    {
        ValidationResult result;

        if (preset.name.isEmpty())
            result.errors.add("Preset name is empty");
        if (preset.name.length() > 30)
            result.warnings.add("Preset name exceeds 30 chars: " + preset.name);
        if (preset.engines.empty())
            result.errors.add("No engines specified");
        if (preset.mood.isEmpty())
            result.warnings.add("No mood category set");

        // DNA validation
        auto checkDNA = [&](float val, const char* dim)
        {
            if (val < 0.0f || val > 1.0f)
                result.errors.add(juce::String("DNA ") + dim + " out of range: " + juce::String(val));
        };
        checkDNA(preset.dna.brightness,  "brightness");
        checkDNA(preset.dna.warmth,      "warmth");
        checkDNA(preset.dna.movement,    "movement");
        checkDNA(preset.dna.density,     "density");
        checkDNA(preset.dna.space,       "space");
        checkDNA(preset.dna.aggression,  "aggression");

        // Coupling validation
        for (const auto& cp : preset.coupling)
        {
            if (cp.amount < 0.0f || cp.amount > 1.0f)
                result.warnings.add("Coupling amount out of range: " + juce::String(cp.amount));
        }

        result.valid = result.errors.isEmpty();
        return result;
    }

    static ValidationResult validateBatch(const std::vector<PresetData>& presets)
    {
        ValidationResult combined;
        juce::StringArray seenNames;

        for (const auto& p : presets)
        {
            auto r = validatePreset(p);
            for (const auto& e : r.errors)
                combined.errors.add("[" + p.name + "] " + e);
            for (const auto& w : r.warnings)
                combined.warnings.add("[" + p.name + "] " + w);

            if (seenNames.contains(p.name))
                combined.errors.add("Duplicate preset name: " + p.name);
            seenNames.add(p.name);
        }

        combined.valid = combined.errors.isEmpty();
        return combined;
    }

private:

    //==========================================================================
    // XPM Rules — hardcoded, non-negotiable
    //==========================================================================
    static constexpr bool KEY_TRACK      = true;
    static constexpr int  ROOT_NOTE      = 0;
    static constexpr int  EMPTY_VEL_START = 0;

    //==========================================================================
    // Note strategy
    //==========================================================================

    static std::vector<int> getNotesToRender(const RenderSettings& settings)
    {
        std::vector<int> notes;
        switch (settings.noteStrategy)
        {
            case RenderSettings::NoteStrategy::EveryMinor3rd:
                for (int n = 24; n <= 96; n += 3) notes.push_back(n);
                break;
            case RenderSettings::NoteStrategy::Chromatic:
                for (int n = 24; n <= 96; ++n) notes.push_back(n);
                break;
            case RenderSettings::NoteStrategy::EveryFifth:
                for (int n = 24; n <= 96; n += 7) notes.push_back(n);
                break;
            case RenderSettings::NoteStrategy::OctavesOnly:
                for (int n = 24; n <= 96; n += 12) notes.push_back(n);
                break;
        }
        return notes;
    }

    static float velocityForLayer(int layer, int totalLayers)
    {
        if (totalLayers <= 1) return 0.8f;
        if (totalLayers == 2) return layer == 0 ? 0.5f : 1.0f;
        // 3 layers: soft, medium, hard
        static constexpr float vels[] = { 0.3f, 0.7f, 1.0f };
        return vels[juce::jlimit(0, 2, layer)];
    }

    //==========================================================================
    // WAV rendering (offline, worker thread)
    //==========================================================================

    void renderNoteToWav(const PresetData& /*preset*/, int note, float velocity,
                         const RenderSettings& settings, const juce::File& outputFile)
    {
        // Calculate total samples
        int totalSamples = (int)((settings.renderSeconds + settings.tailSeconds) * settings.sampleRate);
        int holdSamples  = (int)(settings.renderSeconds * settings.sampleRate);

        // Render buffer (stereo)
        juce::AudioBuffer<float> buffer(2, totalSamples);
        buffer.clear();

        // NOTE: In a full implementation, this would:
        // 1. Create a temporary processor instance
        // 2. Load the preset's engines and parameters
        // 3. Set up coupling routes
        // 4. Send MIDI noteOn(note, velocity)
        // 5. Render holdSamples of audio
        // 6. Send MIDI noteOff
        // 7. Render tailSamples for release/FX decay
        //
        // For now, generate a silent placeholder WAV.
        // Integration with the real processor is the next step.

        (void)note;
        (void)velocity;
        (void)holdSamples;

        // Write WAV
        writeWav(outputFile, buffer, settings.sampleRate, settings.bitDepth);
    }

    static void writeWav(const juce::File& file, const juce::AudioBuffer<float>& buffer,
                         double sampleRate, int bitDepth)
    {
        file.deleteFile();
        auto stream = file.createOutputStream();
        if (!stream) return;

        juce::WavAudioFormat wav;
        auto writer = std::unique_ptr<juce::AudioFormatWriter>(
            wav.createWriterFor(stream.release(), sampleRate,
                               (unsigned int)buffer.getNumChannels(),
                               bitDepth, {}, 0));
        if (writer)
            writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
    }

    //==========================================================================
    // XPM generation (keygroup program XML)
    //==========================================================================

    void writeXPM(const juce::File& file, const PresetData& preset,
                  const std::vector<int>& notes, const RenderSettings& settings)
    {
        juce::XmlElement root("Keygroup");

        // Required MPC attributes
        root.setAttribute("KeyTrack", KEY_TRACK ? "True" : "False");

        for (int i = 0; i < (int)notes.size(); ++i)
        {
            int note = notes[(size_t)i];
            int lowKey  = (i == 0) ? 0 : (notes[(size_t)i - 1] + note) / 2;
            int highKey = (i == (int)notes.size() - 1) ? 127 : (note + notes[(size_t)i + 1]) / 2;

            auto* zone = root.createNewChildElement("Zone");
            zone->setAttribute("RootNote", ROOT_NOTE);
            zone->setAttribute("LowKey", lowKey);
            zone->setAttribute("HighKey", highKey);

            for (int v = 0; v < settings.velocityLayers; ++v)
            {
                auto* layer = zone->createNewChildElement("Layer");
                layer->setAttribute("SampleFile",
                    sanitizeFilename(preset.name) + "/" + wavFilename(preset.name, note, v));

                // Velocity ranges
                if (settings.velocityLayers == 1)
                {
                    layer->setAttribute("VelStart", 0);
                    layer->setAttribute("VelEnd", 127);
                }
                else
                {
                    int velRange = 128 / settings.velocityLayers;
                    layer->setAttribute("VelStart", v * velRange);
                    layer->setAttribute("VelEnd", (v == settings.velocityLayers - 1) ? 127 : (v + 1) * velRange - 1);
                }
            }

            // Empty layers get VelStart = 0 (critical rule #3)
            if (settings.velocityLayers < 4)
            {
                for (int v = settings.velocityLayers; v < 4; ++v)
                {
                    auto* emptyLayer = zone->createNewChildElement("Layer");
                    emptyLayer->setAttribute("VelStart", EMPTY_VEL_START);
                    emptyLayer->setAttribute("VelEnd", 0);
                }
            }
        }

        root.writeTo(file);
    }

    //==========================================================================
    // Manifest
    //==========================================================================

    static void writeManifest(const juce::File& bundleDir,
                              const BundleConfig& config, int presetCount)
    {
        juce::XmlElement manifest("Expansion");
        manifest.setAttribute("Name", config.name);
        manifest.setAttribute("Manufacturer", config.manufacturer);
        manifest.setAttribute("Version", config.version);
        manifest.setAttribute("ID", config.bundleId);
        manifest.setAttribute("Description", config.description);
        manifest.setAttribute("PresetCount", presetCount);

        manifest.writeTo(bundleDir.getChildFile("Manifest.xml"));
    }

    //==========================================================================
    // Filename helpers
    //==========================================================================

    static juce::String sanitizeFilename(const juce::String& name)
    {
        return name.replaceCharacters(" /\\:*?\"<>|", "__________")
                   .substring(0, 50);
    }

    static juce::String wavFilename(const juce::String& presetName, int note, int velLayer)
    {
        static const char* noteNames[] = {"C","Db","D","Eb","E","F","Gb","G","Ab","A","Bb","B"};
        int octave = note / 12 - 1;
        juce::String noteName = juce::String(noteNames[note % 12]) + juce::String(octave);

        juce::String name = sanitizeFilename(presetName) + "__" + noteName;
        if (velLayer >= 0)
            name += "__v" + juce::String(velLayer + 1);
        return name + ".WAV";
    }
};

} // namespace xomnibus
