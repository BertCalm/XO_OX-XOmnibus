#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Core/PresetManager.h"
#include "../Core/EngineRegistry.h"
#include "../Core/MegaCouplingMatrix.h"
#include "XPNCoverArt.h"

#include <atomic>
#include <cmath>
#include <thread>
#include <mutex>
#include <future>

namespace xomnibus {

//==============================================================================
// SoundShapeClassifier — Analyzes preset DNA + engines to determine optimal
// render settings per the Sound Shape system (xpn_sound_shape_rendering.md).
//
// 6 shapes: Transient, Sustained, Evolving, Bass, Texture, Rhythmic
// Each shape maps to specific render durations, note strategies, and vel layers.
//
struct SoundShape {
    enum class Type { Transient, Sustained, Evolving, Bass, Texture, Rhythmic };

    Type  type           = Type::Sustained;
    float holdSeconds    = 4.0f;
    float tailSeconds    = 2.0f;
    int   velocityLayers = 1;
    const char* label    = "Sustained";
};

class SoundShapeClassifier {
public:
    static SoundShape classify(const PresetData& preset)
    {
        const auto& dna = preset.dna;

        // Check for ONSET engine → always Rhythmic
        for (const auto& eng : preset.engines)
        {
            auto upper = eng.toUpperCase();
            if (upper == "ONSET" || upper == "XONSET")
                return rhythmic();
        }

        // High aggression + low space + high density → Transient
        if (dna.aggression > 0.6f && dna.space < 0.3f && dna.density > 0.5f)
            return transient();

        // Low brightness + high warmth + low movement → Bass
        if (dna.brightness < 0.35f && dna.warmth > 0.5f && dna.movement < 0.4f)
            return bass();

        // High movement + high space → Evolving
        if (dna.movement > 0.65f && dna.space > 0.5f)
            return evolving();

        // High density + low movement → Texture
        if (dna.density > 0.7f && dna.movement < 0.35f)
            return texture();

        // Default → Sustained
        return sustained();
    }

private:
    static SoundShape transient()
    {
        return { SoundShape::Type::Transient, 1.0f, 0.5f, 3, "Transient" };
    }

    static SoundShape sustained()
    {
        return { SoundShape::Type::Sustained, 4.0f, 2.0f, 1, "Sustained" };
    }

    static SoundShape evolving()
    {
        return { SoundShape::Type::Evolving, 6.0f, 3.0f, 1, "Evolving" };
    }

    static SoundShape bass()
    {
        return { SoundShape::Type::Bass, 3.0f, 1.5f, 2, "Bass" };
    }

    static SoundShape texture()
    {
        return { SoundShape::Type::Texture, 5.0f, 2.5f, 1, "Texture" };
    }

    static SoundShape rhythmic()
    {
        return { SoundShape::Type::Rhythmic, 0.5f, 0.3f, 3, "Rhythmic" };
    }
};

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
        bool   useSoundShapes = false;         // auto-adjust per preset via SoundShapeClassifier

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
        juce::String coverEngine;              // Engine ID for cover art style (e.g. "ONSET")
        juce::File   outputDir;
        int          coverSeed    = 0;         // RNG seed for reproducible artwork
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
        juce::String soundShapeLabel;          // Sound Shape classification
        float  overallProgress = 0.0f;         // 0-1
        bool   cancelled = false;
    };

    using ProgressCallback = std::function<void(Progress&)>;

    //==========================================================================
    // Size estimation — call before export to inform the user
    //==========================================================================

    struct SizeEstimate {
        int64_t totalBytes       = 0;
        int     totalWavFiles    = 0;
        int     notesPerPreset   = 0;
        int     samplesPerNote   = 0;
        double  durationPerNote  = 0.0;        // seconds
    };

    static SizeEstimate estimateExportSize(const RenderSettings& settings, int presetCount)
    {
        SizeEstimate est;
        auto notes = getNotesToRender(settings);
        est.notesPerPreset = (int)notes.size();
        est.totalWavFiles = est.notesPerPreset * settings.velocityLayers * presetCount;
        est.durationPerNote = (double)(settings.renderSeconds + settings.tailSeconds);
        est.samplesPerNote = (int)(est.durationPerNote * settings.sampleRate);

        // WAV size: header(44) + samples * channels * bytesPerSample
        int bytesPerSample = settings.bitDepth / 8;
        int64_t wavBodySize = (int64_t)est.samplesPerNote * 2 * bytesPerSample;
        int64_t wavFileSize = 44 + wavBodySize;

        est.totalBytes = wavFileSize * est.totalWavFiles;
        return est;
    }

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

        // Atomic export: write to temp dir first, rename on success
        auto finalBundleDir = config.outputDir.getChildFile(config.name.replace(" ", "_"));
        auto tempBundleDir = config.outputDir.getChildFile(
            "." + config.name.replace(" ", "_") + "_tmp_" +
            juce::String(juce::Time::currentTimeMillis()));

        if (!tempBundleDir.createDirectory())
        {
            result.errorMessage = "Failed to create temp directory: " + tempBundleDir.getFullPathName();
            return result;
        }

        auto keyGroupDir = tempBundleDir.getChildFile("Keygroups");
        if (!keyGroupDir.createDirectory())
        {
            tempBundleDir.deleteRecursively();
            result.errorMessage = "Failed to create Keygroups directory";
            return result;
        }

        std::atomic<bool> cancelled { false };

        for (int pi = 0; pi < (int)presets.size() && !cancelled.load(); ++pi)
        {
            const auto& preset = presets[(size_t)pi];
            progress.currentPreset = pi + 1;
            progress.presetName = preset.name;

            // Sound Shape classification
            RenderSettings effectiveSettings = settings;
            if (settings.useSoundShapes)
            {
                auto shape = SoundShapeClassifier::classify(preset);
                effectiveSettings.renderSeconds = shape.holdSeconds;
                effectiveSettings.tailSeconds = shape.tailSeconds;
                effectiveSettings.velocityLayers = shape.velocityLayers;
                progress.soundShapeLabel = shape.label;
            }

            // Render preset to WAV files
            auto presetDir = keyGroupDir.getChildFile(sanitizeFilename(preset.name));
            if (!presetDir.createDirectory())
            {
                result.errorMessage = "Failed to create preset directory: " + preset.name;
                tempBundleDir.deleteRecursively();
                return result;
            }

            auto notes = getNotesToRender(effectiveSettings);
            int totalNotesForPreset = (int)notes.size() * effectiveSettings.velocityLayers;
            progress.totalNotes = totalNotesForPreset;

            // Build work items for parallel rendering
            struct RenderJob {
                int note;
                int velLayer;
                float velocity;
                juce::File wavFile;
            };
            std::vector<RenderJob> jobs;
            jobs.reserve((size_t)totalNotesForPreset);

            for (int ni = 0; ni < (int)notes.size(); ++ni)
            {
                int note = notes[(size_t)ni];
                for (int vel = 0; vel < effectiveSettings.velocityLayers; ++vel)
                {
                    jobs.push_back({
                        note, vel,
                        velocityForLayer(vel, effectiveSettings.velocityLayers),
                        presetDir.getChildFile(wavFilename(preset.name, note, vel))
                    });
                }
            }

            // Parallel rendering with thread pool
            unsigned int numWorkers = juce::jmax(1u, std::thread::hardware_concurrency() - 1);
            std::atomic<int> completedJobs { 0 };
            std::atomic<bool> renderError { false };
            juce::String firstError;
            std::mutex errorMutex;
            std::mutex progressMutex;

            auto renderBatch = [&](size_t startIdx, size_t endIdx)
            {
                for (size_t j = startIdx; j < endIdx && !cancelled.load() && !renderError.load(); ++j)
                {
                    const auto& job = jobs[j];
                    auto wavResult = renderNoteToWav(preset, job.note, job.velocity,
                                                     effectiveSettings, job.wavFile);
                    if (!wavResult.success)
                    {
                        std::lock_guard<std::mutex> lock(errorMutex);
                        if (!renderError.load())
                        {
                            renderError.store(true);
                            firstError = wavResult.error;
                        }
                        return;
                    }

                    int done = completedJobs.fetch_add(1) + 1;

                    // Progress update — serialized via mutex to avoid data races
                    if (progressCb && (done % juce::jmax(1, (int)numWorkers) == 0 || done == (int)jobs.size()))
                    {
                        std::lock_guard<std::mutex> lock(progressMutex);
                        progress.currentNote = done;
                        float presetFrac = (float)pi / (float)presets.size();
                        float noteFrac = (float)done / (float)totalNotesForPreset;
                        progress.overallProgress = presetFrac + noteFrac / (float)presets.size();
                        progressCb(progress);
                        if (progress.cancelled)
                            cancelled.store(true);
                    }
                }
            };

            if (numWorkers <= 1 || jobs.size() <= 4)
            {
                // Small batch: run sequentially
                renderBatch(0, jobs.size());
            }
            else
            {
                // Partition jobs across worker threads
                std::vector<std::future<void>> futures;
                size_t chunkSize = (jobs.size() + numWorkers - 1) / numWorkers;

                for (unsigned int w = 0; w < numWorkers && w * chunkSize < jobs.size(); ++w)
                {
                    size_t start = w * chunkSize;
                    size_t end = juce::jmin(start + chunkSize, jobs.size());
                    futures.push_back(std::async(std::launch::async, renderBatch, start, end));
                }

                for (auto& f : futures)
                    f.get();
            }

            if (renderError.load())
            {
                result.errorMessage = "WAV render failed for " + preset.name + ": " + firstError;
                tempBundleDir.deleteRecursively();
                return result;
            }

            // Tally results
            for (const auto& job : jobs)
            {
                result.samplesRendered++;
                result.totalSizeBytes += job.wavFile.getSize();
            }

            if (!cancelled)
            {
                // Generate XPM keygroup program
                auto xpmFile = keyGroupDir.getChildFile(sanitizeFilename(preset.name) + ".xpm");
                auto xpmResult = writeXPM(xpmFile, preset, notes, effectiveSettings);
                if (!xpmResult.success)
                {
                    result.errorMessage = "XPM write failed for " + preset.name + ": " + xpmResult.error;
                    tempBundleDir.deleteRecursively();
                    return result;
                }

                result.presetsExported++;
            }
        }

        if (cancelled)
        {
            tempBundleDir.deleteRecursively();
            result.errorMessage = "Export cancelled by user";
            return result;
        }

        // Generate cover art (procedural, engine-specific)
        auto coverEngine = config.coverEngine.isNotEmpty()
            ? config.coverEngine
            : (presets[0].engines.isEmpty() ? juce::String("DEFAULT")
               : presets[0].engines[0]);

        auto coverResult = XPNCoverArt::generate(
            coverEngine, config.name, tempBundleDir,
            result.presetsExported, config.version, config.coverSeed);

        // Copy 1000x1000 as Preview.png (MPC convention)
        if (coverResult.success)
            coverResult.cover1000.copyFileTo(tempBundleDir.getChildFile("Preview.png"));

        // Write bundle manifest
        auto manifestResult = writeManifest(tempBundleDir, config, result.presetsExported);
        if (!manifestResult.success)
        {
            result.errorMessage = "Manifest write failed: " + manifestResult.error;
            tempBundleDir.deleteRecursively();
            return result;
        }

        // Atomic swap: remove old bundle dir if it exists, rename temp to final
        if (finalBundleDir.exists())
            finalBundleDir.deleteRecursively();

        if (!tempBundleDir.moveFileTo(finalBundleDir))
        {
            // Fallback: if rename fails (e.g., cross-device), try copy
            if (tempBundleDir.copyDirectoryTo(finalBundleDir))
            {
                tempBundleDir.deleteRecursively();
            }
            else
            {
                result.errorMessage = "Failed to finalize bundle directory";
                return result;
            }
        }

        result.success = true;
        result.outputFile = finalBundleDir;
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
        if (preset.engines.isEmpty())
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
        for (const auto& cp : preset.couplingPairs)
        {
            if (cp.amount < -1.0f || cp.amount > 1.0f)
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
    // Internal result types for error propagation
    //==========================================================================

    struct IOResult {
        bool success = true;
        juce::String error;
    };

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

        // Only attenuate or boost to ceiling — never amplify silence
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.applyGain(ch, 0, buffer.getNumSamples(), gain);
    }

    //==========================================================================
    // WAV rendering (offline, worker thread)
    //==========================================================================

    IOResult renderNoteToWav(const PresetData& /*preset*/, int note, float velocity,
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

        // Apply normalization
        normalizeBuffer(buffer, settings.normCeiling);

        // Write WAV
        return writeWav(outputFile, buffer, settings.sampleRate, settings.bitDepth);
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
            return { false, "Cannot create WAV writer for: " + file.getFullPathName() };

        if (!writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()))
            return { false, "Failed to write audio data: " + file.getFullPathName() };

        return { true, {} };
    }

    //==========================================================================
    // XPM generation (keygroup program XML)
    //==========================================================================

    IOResult writeXPM(const juce::File& file, const PresetData& preset,
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

        if (!root.writeTo(file))
            return { false, "Failed to write XPM: " + file.getFullPathName() };

        return { true, {} };
    }

    //==========================================================================
    // Manifest
    //==========================================================================

    static IOResult writeManifest(const juce::File& bundleDir,
                                  const BundleConfig& config, int presetCount)
    {
        juce::XmlElement manifest("Expansion");
        manifest.setAttribute("Name", config.name);
        manifest.setAttribute("Manufacturer", config.manufacturer);
        manifest.setAttribute("Version", config.version);
        manifest.setAttribute("ID", config.bundleId);
        manifest.setAttribute("Description", config.description);
        manifest.setAttribute("PresetCount", presetCount);

        auto manifestFile = bundleDir.getChildFile("Manifest.xml");
        if (!manifest.writeTo(manifestFile))
            return { false, "Failed to write Manifest.xml" };

        return { true, {} };
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
