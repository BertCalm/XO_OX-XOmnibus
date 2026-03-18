#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Core/PresetManager.h"
#include "../Core/EngineRegistry.h"
#include "../Core/MegaCouplingMatrix.h"
#include "XPNCoverArt.h"

#include <array>
#include <atomic>
#include <cmath>
#include <random>
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
// XOriginate — Renders XOmnibus presets to WAV samples and packages them
// as MPC-compatible .xpn expansion packs.
// (Formerly XPNExporter — renamed to XOriginate: XO + O-word convention)
//
// IMPORTANT: All rendering runs on a worker thread (never the audio thread).
// The exporter creates a temporary processor instance for offline rendering.
//
// 3 Critical XPM Rules (non-negotiable):
//   1. KeyTrack = True      — samples transpose across keygroup zones
//   2. RootNote = 0         — MPC auto-detect convention
//   3. Empty VelStart = 0   — prevents ghost triggering
//
class XOriginate {
public:

    //==========================================================================
    // Configuration
    //==========================================================================

    struct RenderSettings {
        double sampleRate    = 44100.0;
        int    bitDepth      = 24;             // 16 or 24
        float  renderSeconds = 4.0f;           // note hold time
        float  tailSeconds   = 2.0f;           // after noteOff
        float  normCeiling   = -0.3f;          // dBFS normalization target
        int    velocityLayers = 4;             // 1-4 (default 4: ghost/light/mid/hard)
        bool   useSoundShapes = false;         // auto-adjust per preset via SoundShapeClassifier
        bool   generatePreviews = false;       // generate low-quality .mp3 preview per WAV
        bool   dnaAdaptiveVelocity = true;     // shift velocity splits based on preset DNA aggression

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

            // Build note-group work items for parallel rendering.
            // Each note group contains all velocity layers so we can group-normalize
            // (preserving dynamic relationships between soft and loud hits).
            struct NoteGroupJob {
                int note;
                std::vector<VelocityGroupJob> velJobs;
            };
            std::vector<NoteGroupJob> noteGroups;
            noteGroups.reserve(notes.size());

            // Also track all WAV files for tally
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
                NoteGroupJob group;
                group.note = note;

                for (int vel = 0; vel < effectiveSettings.velocityLayers; ++vel)
                {
                    auto wavFile = presetDir.getChildFile(wavFilename(preset.name, note, vel));
                    group.velJobs.push_back({
                        vel,
                        velocityForLayer(vel, effectiveSettings.velocityLayers),
                        wavFile
                    });
                    jobs.push_back({ note, vel,
                        velocityForLayer(vel, effectiveSettings.velocityLayers),
                        wavFile });
                }
                noteGroups.push_back(std::move(group));
            }

            // Parallel rendering with thread pool — one job per note group.
            // Group rendering ensures all velocity layers for a note are rendered
            // together, enabling group normalization (loudest layer hits ceiling,
            // softer layers stay proportionally quieter).
            unsigned int numWorkers = juce::jmax(1u, std::thread::hardware_concurrency() - 1);
            std::atomic<int> completedNoteGroups { 0 };
            std::atomic<bool> renderError { false };
            juce::String firstError;
            std::mutex errorMutex;
            std::mutex progressMutex;

            auto renderBatch = [&](size_t startIdx, size_t endIdx)
            {
                for (size_t j = startIdx; j < endIdx && !cancelled.load() && !renderError.load(); ++j)
                {
                    const auto& group = noteGroups[j];
                    auto groupResult = renderNoteGroupToWav(preset, group.note,
                                                            group.velJobs, effectiveSettings);
                    if (!groupResult.success)
                    {
                        std::lock_guard<std::mutex> lock(errorMutex);
                        if (!renderError.load())
                        {
                            renderError.store(true);
                            firstError = groupResult.error;
                        }
                        return;
                    }

                    int doneGroups = completedNoteGroups.fetch_add(1) + 1;
                    int doneJobs = doneGroups * effectiveSettings.velocityLayers;

                    // Progress update — serialized via mutex to avoid data races
                    if (progressCb && (doneGroups % juce::jmax(1, (int)numWorkers) == 0
                                       || doneGroups == (int)noteGroups.size()))
                    {
                        std::lock_guard<std::mutex> lock(progressMutex);
                        progress.currentNote = doneJobs;
                        float presetFrac = (float)pi / (float)presets.size();
                        float noteFrac = (float)doneJobs / (float)totalNotesForPreset;
                        progress.overallProgress = presetFrac + noteFrac / (float)presets.size();
                        progressCb(progress);
                        if (progress.cancelled)
                            cancelled.store(true);
                    }
                }
            };

            if (numWorkers <= 1 || noteGroups.size() <= 4)
            {
                // Small batch: run sequentially
                renderBatch(0, noteGroups.size());
            }
            else
            {
                // Partition note groups across worker threads
                std::vector<std::future<void>> futures;
                size_t chunkSize = (noteGroups.size() + numWorkers - 1) / numWorkers;

                for (unsigned int w = 0; w < numWorkers && w * chunkSize < noteGroups.size(); ++w)
                {
                    size_t start = w * chunkSize;
                    size_t end = juce::jmin(start + chunkSize, noteGroups.size());
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
        if (totalLayers == 3)
        {
            static constexpr float vels3[] = { 0.3f, 0.7f, 1.0f };
            return vels3[juce::jlimit(0, 2, layer)];
        }
        // 4 layers: ghost, light, mid, hard (ported from Python toolchain)
        static constexpr float vels4[] = { 0.15f, 0.45f, 0.75f, 1.0f };
        return vels4[juce::jlimit(0, 3, layer)];
    }

    //==========================================================================
    // Velocity split points — 4-layer system ported from Python toolchain
    //
    // Default splits: ghost(1-20), light(21-50), mid(51-90), hard(91-127)
    // DNA-adaptive: high aggression shifts splits downward so hard hits
    // trigger earlier (more aggressive response curve)
    //==========================================================================

    struct VelSplit { int start; int end; };

    static std::array<VelSplit, 4> getVelocitySplits(float aggressionDNA = 0.5f,
                                                      bool dnaAdaptive = true)
    {
        // Base split points: ghost(1-20), light(21-50), mid(51-90), hard(91-127)
        int ghost_end = 20;
        int light_end = 50;
        int mid_end   = 90;

        if (dnaAdaptive && aggressionDNA > 0.5f)
        {
            // High aggression: shift splits downward (hard hits trigger sooner)
            float shift = (aggressionDNA - 0.5f) * 2.0f; // 0..1 for aggression 0.5..1.0
            ghost_end = juce::jmax(10, ghost_end - (int)(shift * 8));   // 20 -> 12
            light_end = juce::jmax(25, light_end - (int)(shift * 15));  // 50 -> 35
            mid_end   = juce::jmax(60, mid_end   - (int)(shift * 20)); // 90 -> 70
        }

        return {{
            { 1,             ghost_end },
            { ghost_end + 1, light_end },
            { light_end + 1, mid_end   },
            { mid_end + 1,   127       }
        }};
    }

    static VelSplit velSplitForLayer(int layer, int totalLayers,
                                      float aggressionDNA = 0.5f,
                                      bool dnaAdaptive = true)
    {
        if (totalLayers <= 1)
            return { 0, 127 };

        if (totalLayers == 2)
        {
            if (layer == 0) return { 0, 63 };
            return { 64, 127 };
        }

        if (totalLayers == 3)
        {
            auto splits = getVelocitySplits(aggressionDNA, dnaAdaptive);
            // Merge ghost+light for 3-layer mode
            if (layer == 0) return { 0, splits[1].end };
            if (layer == 1) return { splits[1].end + 1, splits[2].end };
            return { splits[2].end + 1, 127 };
        }

        // 4 layers: use full split system
        auto splits = getVelocitySplits(aggressionDNA, dnaAdaptive);
        auto s = splits[(size_t)juce::jlimit(0, 3, layer)];
        // First layer starts at 0 per MPC convention
        if (layer == 0) s.start = 0;
        return s;
    }

    //==========================================================================
    // Normalization — peak-scan + gain to target ceiling
    //==========================================================================

    // Find peak across a single buffer
    static float findBufferPeak(const juce::AudioBuffer<float>& buffer)
    {
        float peak = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto range = buffer.findMinMax(ch, 0, buffer.getNumSamples());
            float chPeak = juce::jmax(std::abs(range.getStart()), std::abs(range.getEnd()));
            if (chPeak > peak) peak = chPeak;
        }
        return peak;
    }

    // Normalize a single buffer using a pre-computed global peak (for group normalization).
    // The loudest layer in the group hits the ceiling; softer layers stay proportionally quieter.
    static void normalizeBufferWithGlobalPeak(juce::AudioBuffer<float>& buffer,
                                               float globalPeak, float ceilingDb)
    {
        if (globalPeak < 1e-8f) return; // silence, nothing to normalize

        float targetGain = std::pow(10.0f, ceilingDb / 20.0f);
        float gain = targetGain / globalPeak;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.applyGain(ch, 0, buffer.getNumSamples(), gain);
    }

    // Legacy single-buffer normalization (used when only 1 velocity layer)
    static void normalizeBuffer(juce::AudioBuffer<float>& buffer, float ceilingDb)
    {
        normalizeBufferWithGlobalPeak(buffer, findBufferPeak(buffer), ceilingDb);
    }

    //==========================================================================
    // DC offset removal
    //==========================================================================

    static void removeDCOffset(juce::AudioBuffer<float>& buffer)
    {
        int numChannels = buffer.getNumChannels();
        int numSamples = buffer.getNumSamples();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            float sum = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                sum += data[i];
            float dcOffset = sum / (float)numSamples;
            for (int i = 0; i < numSamples; ++i)
                data[i] -= dcOffset;
        }
    }

    //==========================================================================
    // Fade guards — raised-cosine fade-in/out to prevent clicks
    //==========================================================================

    static void applyFadeGuards(juce::AudioBuffer<float>& buffer, double sampleRate)
    {
        int numChannels = buffer.getNumChannels();
        int numSamples = buffer.getNumSamples();

        // Fade-in (2ms)
        const int fadeInSamples = static_cast<int>(sampleRate * 0.002);
        for (int i = 0; i < fadeInSamples && i < numSamples; ++i)
        {
            float gain = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * i / fadeInSamples));
            for (int ch = 0; ch < numChannels; ++ch)
                buffer.getWritePointer(ch)[i] *= gain;
        }

        // Fade-out (10ms)
        const int fadeOutSamples = static_cast<int>(sampleRate * 0.01);
        for (int i = 0; i < fadeOutSamples && i < numSamples; ++i)
        {
            float gain = 0.5f * (1.0f + std::cos(juce::MathConstants<float>::pi * i / fadeOutSamples));
            int idx = numSamples - 1 - i;
            for (int ch = 0; ch < numChannels; ++ch)
                buffer.getWritePointer(ch)[idx] *= gain;
        }
    }

    //==========================================================================
    // WAV rendering (offline, worker thread)
    //==========================================================================

    //==========================================================================
    // Render a single note to an audio buffer using a real engine instance.
    // Does NOT normalize or write — caller handles group normalization.
    //==========================================================================

    IOResult renderNoteToBuffer(const PresetData& preset, int note, float velocity,
                                const RenderSettings& settings,
                                juce::AudioBuffer<float>& buffer)
    {
        int totalSamples = (int)((settings.renderSeconds + settings.tailSeconds) * settings.sampleRate);
        int holdSamples  = (int)(settings.renderSeconds * settings.sampleRate);
        int blockSize    = 512;

        buffer.setSize(2, totalSamples);
        buffer.clear();

        // Create engine instance for this render job
        juce::String engineId = preset.engines.isEmpty() ? juce::String("OddfeliX") : preset.engines[0];
        auto engine = EngineRegistry::instance().createEngine(engineId.toStdString());
        if (!engine)
            return { false, "Failed to create engine: " + engineId };

        // Prepare engine for offline rendering
        engine->prepare(settings.sampleRate, blockSize);

        // Apply preset parameters from the PresetData
        // Create a temporary APVTS to host the engine's parameters
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        auto engineLayout = engine->createParameterLayout();

        // We need a minimal AudioProcessor to host the APVTS
        struct MinimalProcessor : juce::AudioProcessor {
            MinimalProcessor() : AudioProcessor(BusesProperties()
                .withOutput("Output", juce::AudioChannelSet::stereo())) {}
            const juce::String getName() const override { return "XPNRenderer"; }
            void prepareToPlay(double, int) override {}
            void releaseResources() override {}
            void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
            double getTailLengthSeconds() const override { return 0; }
            bool acceptsMidi() const override { return true; }
            bool producesMidi() const override { return false; }
            juce::AudioProcessorEditor* createEditor() override { return nullptr; }
            bool hasEditor() const override { return false; }
            int getNumPrograms() override { return 1; }
            int getCurrentProgram() override { return 0; }
            void setCurrentProgram(int) override {}
            const juce::String getProgramName(int) override { return {}; }
            void changeProgramName(int, const juce::String&) override {}
            void getStateInformation(juce::MemoryBlock&) override {}
            void setStateInformation(const void*, int) override {}
        };

        MinimalProcessor tempProcessor;
        juce::AudioProcessorValueTreeState apvts(tempProcessor, nullptr, "XPNParams",
                                                  engine->createParameterLayout());
        engine->attachParameters(apvts);

        // Apply preset parameters for this engine
        for (const auto& [engName, params] : preset.parametersByEngine)
        {
            if (auto* obj = params.getDynamicObject())
            {
                for (const auto& prop : obj->getProperties())
                {
                    if (auto* param = apvts.getParameter(prop.name.toString()))
                    {
                        float normValue = param->convertTo0to1((float)prop.value);
                        param->setValueNotifyingHost(normValue);
                    }
                }
            }
        }

        engine->reset();

        // Send MIDI noteOn
        juce::MidiBuffer midiBuffer;
        int midiVelocity = juce::jlimit(1, 127, (int)(velocity * 127.0f));
        midiBuffer.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8)midiVelocity), 0);

        // Render blocks for the sustain phase
        int samplesRendered = 0;
        while (samplesRendered < holdSamples)
        {
            int samplesThisBlock = juce::jmin(blockSize, totalSamples - samplesRendered);
            juce::AudioBuffer<float> blockBuffer(buffer.getArrayOfWritePointers(),
                                                  2, samplesRendered, samplesThisBlock);

            // Only pass MIDI on the first block (noteOn already queued)
            engine->renderBlock(blockBuffer, midiBuffer, samplesThisBlock);
            midiBuffer.clear();
            samplesRendered += samplesThisBlock;
        }

        // Send MIDI noteOff at the start of the tail phase
        midiBuffer.addEvent(juce::MidiMessage::noteOff(1, note), 0);

        // Render tail (release) blocks
        while (samplesRendered < totalSamples)
        {
            int samplesThisBlock = juce::jmin(blockSize, totalSamples - samplesRendered);
            juce::AudioBuffer<float> blockBuffer(buffer.getArrayOfWritePointers(),
                                                  2, samplesRendered, samplesThisBlock);

            engine->renderBlock(blockBuffer, midiBuffer, samplesThisBlock);
            midiBuffer.clear();
            samplesRendered += samplesThisBlock;
        }

        // Apply fade guards (before DC removal and normalization)
        applyFadeGuards(buffer, settings.sampleRate);

        // Remove DC offset
        removeDCOffset(buffer);

        return { true, {} };
    }

    //==========================================================================
    // Render a note + write WAV (single velocity layer, legacy path)
    //==========================================================================

    IOResult renderNoteToWav(const PresetData& preset, int note, float velocity,
                             const RenderSettings& settings, const juce::File& outputFile)
    {
        juce::AudioBuffer<float> buffer;
        auto renderResult = renderNoteToBuffer(preset, note, velocity, settings, buffer);
        if (!renderResult.success)
            return renderResult;

        // Single-layer normalization (old path — only used when velocityLayers == 1)
        normalizeBuffer(buffer, settings.normCeiling);

        // Optional preview generation
        if (settings.generatePreviews)
        {
            auto previewFile = outputFile.getParentDirectory().getChildFile(
                outputFile.getFileNameWithoutExtension() + ".mp3");
            generatePreview(buffer, settings.sampleRate, previewFile);
        }

        return writeWav(outputFile, buffer, settings.sampleRate, settings.bitDepth);
    }

    //==========================================================================
    // Render all velocity layers for a note, group-normalize, then write WAVs.
    // This preserves dynamic relationships between velocity layers.
    //==========================================================================

    struct VelocityGroupJob {
        int velLayer;
        float velocity;
        juce::File wavFile;
    };

    IOResult renderNoteGroupToWav(const PresetData& preset, int note,
                                   const std::vector<VelocityGroupJob>& velJobs,
                                   const RenderSettings& settings)
    {
        // Phase 1: Render all velocity layers into buffers
        std::vector<juce::AudioBuffer<float>> buffers(velJobs.size());

        for (size_t v = 0; v < velJobs.size(); ++v)
        {
            auto renderResult = renderNoteToBuffer(preset, note, velJobs[v].velocity,
                                                    settings, buffers[v]);
            if (!renderResult.success)
                return renderResult;
        }

        // Phase 2: Find global peak across ALL velocity layers for this note
        float globalPeak = 0.0f;
        for (const auto& buf : buffers)
        {
            float p = findBufferPeak(buf);
            if (p > globalPeak) globalPeak = p;
        }

        // Phase 3: Normalize each layer relative to the global peak
        for (auto& buf : buffers)
            normalizeBufferWithGlobalPeak(buf, globalPeak, settings.normCeiling);

        // Phase 4: Generate previews and write WAVs
        for (size_t v = 0; v < velJobs.size(); ++v)
        {
            if (settings.generatePreviews)
            {
                auto previewFile = velJobs[v].wavFile.getParentDirectory().getChildFile(
                    velJobs[v].wavFile.getFileNameWithoutExtension() + ".mp3");
                generatePreview(buffers[v], settings.sampleRate, previewFile);
            }

            auto writeResult = writeWav(velJobs[v].wavFile, buffers[v],
                                         settings.sampleRate, settings.bitDepth);
            if (!writeResult.success)
                return writeResult;
        }

        return { true, {} };
    }

    static IOResult writeWav(const juce::File& file, const juce::AudioBuffer<float>& buffer,
                             double sampleRate, int bitDepth)
    {
        file.deleteFile();
        auto stream = file.createOutputStream();
        if (!stream)
            return { false, "Cannot create output stream: " + file.getFullPathName() };

        // Apply TPDF dithering before quantization.
        // We add triangular-PDF dither noise at the LSB level of the target bit depth
        // to the float data, then let JUCE's writer handle the final conversion.
        juce::AudioBuffer<float> dithered(buffer.getNumChannels(), buffer.getNumSamples());
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            dithered.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());

        // TPDF dither: sum of two uniform random values gives triangular distribution
        float maxVal = (bitDepth == 24) ? 8388607.0f : 32767.0f;
        std::mt19937 ditherRng(42 + std::hash<juce::String>{}(file.getFileName()));
        std::uniform_real_distribution<float> ditherDist(-1.0f, 1.0f);

        for (int ch = 0; ch < dithered.getNumChannels(); ++ch)
        {
            auto* data = dithered.getWritePointer(ch);
            for (int i = 0; i < dithered.getNumSamples(); ++i)
            {
                float dither = (ditherDist(ditherRng) + ditherDist(ditherRng)) / maxVal;
                data[i] = juce::jlimit(-1.0f, 1.0f, data[i] + dither);
            }
        }

        juce::WavAudioFormat wav;
        auto writer = std::unique_ptr<juce::AudioFormatWriter>(
            wav.createWriterFor(stream.release(), sampleRate,
                               (unsigned int)dithered.getNumChannels(),
                               bitDepth, {}, 0));
        if (!writer)
            return { false, "Cannot create WAV writer for: " + file.getFullPathName() };

        if (!writer->writeFromAudioSampleBuffer(dithered, 0, dithered.getNumSamples()))
            return { false, "Failed to write audio data: " + file.getFullPathName() };

        return { true, {} };
    }

    //==========================================================================
    // XPM generation (keygroup program XML)
    //==========================================================================

    IOResult writeXPM(const juce::File& file, const PresetData& preset,
                      const std::vector<int>& notes, const RenderSettings& settings)
    {
        // MPCVObject format — the correct MPC keygroup program structure
        juce::XmlElement root("MPCVObject");
        root.setAttribute("type", "com.akaipro.mpc.keygroup.program");
        root.setAttribute("version", "2.0");

        auto* program = root.createNewChildElement("Program");
        program->createNewChildElement("ProgramName")->addTextElement(preset.name);
        program->createNewChildElement("KeyTrack")->addTextElement(KEY_TRACK ? "True" : "False");
        program->createNewChildElement("NumKeygroups")->addTextElement(juce::String((int)notes.size()));

        // Expression mapping for keygroup programs
        auto* afterTouch = program->createNewChildElement("AfterTouch");
        afterTouch->createNewChildElement("Destination")->addTextElement("FilterCutoff");
        afterTouch->createNewChildElement("Amount")->addTextElement("50");

        auto* modWheel = program->createNewChildElement("ModWheel");
        modWheel->createNewChildElement("Destination")->addTextElement("FilterCutoff");
        modWheel->createNewChildElement("Amount")->addTextElement("70");

        program->createNewChildElement("PitchBendRange")->addTextElement("12");

        auto* keygroups = program->createNewChildElement("Keygroups");

        for (int i = 0; i < (int)notes.size(); ++i)
        {
            int note = notes[(size_t)i];
            int lowNote  = (i == 0) ? 0 : (notes[(size_t)i - 1] + note) / 2;
            int highNote = (i == (int)notes.size() - 1) ? 127 : (note + notes[(size_t)i + 1]) / 2;

            auto* kg = keygroups->createNewChildElement("Keygroup");
            kg->setAttribute("index", i);

            kg->createNewChildElement("LowNote")->addTextElement(juce::String(lowNote));
            kg->createNewChildElement("HighNote")->addTextElement(juce::String(highNote));
            kg->createNewChildElement("RootNote")->addTextElement(juce::String(ROOT_NOTE));

            auto* layers = kg->createNewChildElement("Layers");

            for (int v = 0; v < settings.velocityLayers; ++v)
            {
                auto* layer = layers->createNewChildElement("Layer");
                layer->setAttribute("index", v);

                auto sampleName = sanitizeFilename(preset.name) + "/" + wavFilename(preset.name, note, v);
                layer->createNewChildElement("SampleName")->addTextElement(sampleName);

                // Use DNA-adaptive velocity splits for 4-layer mode
                auto split = velSplitForLayer(v, settings.velocityLayers,
                                               preset.dna.aggression,
                                               settings.dnaAdaptiveVelocity);
                layer->createNewChildElement("VelStart")->addTextElement(juce::String(split.start));
                layer->createNewChildElement("VelEnd")->addTextElement(juce::String(split.end));
            }

            // Empty layers get VelStart = 0 (critical rule #3)
            for (int v = settings.velocityLayers; v < 4; ++v)
            {
                auto* emptyLayer = layers->createNewChildElement("Layer");
                emptyLayer->setAttribute("index", v);
                emptyLayer->createNewChildElement("VelStart")->addTextElement(juce::String(EMPTY_VEL_START));
                emptyLayer->createNewChildElement("VelEnd")->addTextElement("0");
            }
        }

        if (!root.writeTo(file))
            return { false, "Failed to write XPM: " + file.getFullPathName() };

        return { true, {} };
    }

    //==========================================================================
    // Preview generation — low-quality WAV with .mp3 extension
    // (8-bit, mono, 22050Hz, first 2 seconds — MPC browsers play it)
    //==========================================================================

    static IOResult generatePreview(const juce::AudioBuffer<float>& sourceBuffer,
                                    double sourceSampleRate,
                                    const juce::File& previewFile)
    {
        // Take first 2 seconds max
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

        // Write as 8-bit WAV with .mp3 extension
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
