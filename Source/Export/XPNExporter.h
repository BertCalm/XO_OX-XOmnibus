// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../Core/PresetManager.h"
#include "../Core/EngineRegistry.h"
#include "../Core/MegaCouplingMatrix.h"
#include "XPNCoverArt.h"
#include "XPNVelocityCurves.h"

#include "../DSP/ThreadInit.h"

#include <atomic>
#include <cmath>
#include <mutex>

namespace xoceanus
{

//==============================================================================
// SoundShapeClassifier — Analyzes preset DNA + engines to determine optimal
// render settings per the Sound Shape system (xpn_sound_shape_rendering.md).
//
// 6 shapes: Transient, Sustained, Evolving, Bass, Texture, Rhythmic
// Each shape maps to specific render durations, note strategies, and vel layers.
//
struct SoundShape
{
    enum class Type
    {
        Transient,
        Sustained,
        Evolving,
        Bass,
        Texture,
        Rhythmic
    };

    Type type = Type::Sustained;
    float holdSeconds = 4.0f;
    float tailSeconds = 2.0f;
    int velocityLayers = 1;
    const char* label = "Sustained";
};

class SoundShapeClassifier
{
public:
    static SoundShape classify(const PresetData& preset)
    {
        const auto& dna = preset.dna;

        // Check for engines that produce inherently rhythmic output.
        // ONSET/XONSET:       percussion engine — always Rhythmic
        // OBRIX/XOBRIX:       modular brick sequencer — always Rhythmic
        // OSTINATO/XOSTINATO: repeated-pattern engine (lit. "obstinate") — always Rhythmic
        // OVERWORLD:          chip synth — Rhythmic only when ow_drumMode=1
        // OUROBOROS:          chaotic attractor — Rhythmic when high movement + aggression
        for (const auto& eng : preset.engines)
        {
            auto upper = eng.toUpperCase();
            if (upper == "ONSET" || upper == "XONSET" || upper == "OBRIX" || upper == "XOBRIX" || upper == "OSTINATO" ||
                upper == "XOSTINATO")
                return rhythmic();

            if (upper == "OVERWORLD" || upper == "XOVERWORLD")
            {
                // Drum kit mode is an XOceanus-level param stored under the engine name
                auto it = preset.parametersByEngine.find(eng);
                if (it != preset.parametersByEngine.end())
                    if (auto* obj = it->second.getDynamicObject())
                        if ((float)obj->getProperty("ow_drumMode") > 0.5f)
                            return rhythmic();
            }

            if ((upper == "OUROBOROS" || upper == "XOUROBOROS") && dna.movement > 0.6f && dna.aggression > 0.5f)
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
    static SoundShape transient() { return {SoundShape::Type::Transient, 1.0f, 0.5f, 3, "Transient"}; }

    static SoundShape sustained() { return {SoundShape::Type::Sustained, 4.0f, 2.0f, 1, "Sustained"}; }

    static SoundShape evolving() { return {SoundShape::Type::Evolving, 6.0f, 3.0f, 1, "Evolving"}; }

    static SoundShape bass() { return {SoundShape::Type::Bass, 3.0f, 1.5f, 2, "Bass"}; }

    static SoundShape texture() { return {SoundShape::Type::Texture, 5.0f, 2.5f, 1, "Texture"}; }

    static SoundShape rhythmic() { return {SoundShape::Type::Rhythmic, 0.5f, 0.3f, 3, "Rhythmic"}; }
};

//==============================================================================
// XOriginate — Renders XOceanus presets to WAV samples and packages them
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
class XOriginate
{
public:
    //==========================================================================
    // Coupling Snapshot — freezes a live coupling state for composite export
    //==========================================================================

    struct CouplingSnapshot
    {
        struct CouplingRoute
        {
            int sourceSlot = -1;
            int destSlot = -1;
            int couplingType = 0; // maps to CouplingType enum
            float amount = 0.0f;
        };

        std::vector<CouplingRoute> activeRoutes;
        std::array<juce::String, MegaCouplingMatrix::MaxSlots> engineIds; // engines in each slot
        juce::String snapshotName;

        bool hasActiveCoupling() const { return !activeRoutes.empty(); }

        // Brief human-readable summary of active routes for UI display
        juce::String getSummary() const
        {
            if (activeRoutes.empty())
                return "No active coupling";

            static const char* typeNames[] = {
                "Amp>Filter", "Amp>Pitch",    "LFO>Pitch", "Env>Morph",   "Audio>FM", "Audio>Ring",   "Filter>Filter",
                "Amp>Choke",  "Rhythm>Blend", "Env>Decay", "Pitch>Pitch", "Audio>WT", "Audio>Buffer", "Knot"};

            juce::String summary;
            for (size_t i = 0; i < activeRoutes.size(); ++i)
            {
                const auto& r = activeRoutes[i];
                if (i > 0)
                    summary += "; ";

                auto srcName = (r.sourceSlot >= 0 && r.sourceSlot < MegaCouplingMatrix::MaxSlots)
                                   ? engineIds[(size_t)r.sourceSlot]
                                   : juce::String("?");
                auto dstName = (r.destSlot >= 0 && r.destSlot < MegaCouplingMatrix::MaxSlots)
                                   ? engineIds[(size_t)r.destSlot]
                                   : juce::String("?");

                int typeIdx = juce::jlimit(0, 13, r.couplingType);
                summary += srcName + " " + typeNames[typeIdx] + " " + dstName;
            }
            return summary;
        }
    };

    // Capture the current coupling state from the live matrix and registry.
    // Call from the message thread before starting an export.
    static CouplingSnapshot captureCouplingState(const EngineRegistry& registry, const MegaCouplingMatrix& matrix)
    {
        CouplingSnapshot snapshot;
        auto matrixRoutes = matrix.getRoutes();

        for (const auto& route : matrixRoutes)
        {
            if (!route.active || route.amount < 0.001f)
                continue;

            CouplingSnapshot::CouplingRoute snapRoute;
            snapRoute.sourceSlot = route.sourceSlot;
            snapRoute.destSlot = route.destSlot;
            snapRoute.couplingType = static_cast<int>(route.type);
            snapRoute.amount = route.amount;
            snapshot.activeRoutes.push_back(snapRoute);
        }

        // Populate engineIds so getSummary() can label each slot
        const auto& engines = matrix.getActiveEngines();
        for (int slot = 0; slot < MegaCouplingMatrix::MaxSlots; ++slot)
        {
            if (engines[(size_t)slot] != nullptr)
                snapshot.engineIds[(size_t)slot] = engines[(size_t)slot]->getEngineId().toUpperCase();
        }

        return snapshot;
    }

    //==========================================================================
    // Configuration
    //==========================================================================

    struct RenderSettings
    {
        double sampleRate = 48000.0;
        int bitDepth = 24;           // 16 or 24
        int numChannels = 2;         // 1 = mono, 2 = stereo
        float renderSeconds = 4.0f;  // note hold time
        float tailSeconds = 2.0f;    // after noteOff
        float normCeiling = -0.3f;   // dBFS normalization target
        int velocityLayers = 1;      // 1-3
        bool useSoundShapes = false; // auto-adjust per preset via SoundShapeClassifier

        // Note sampling strategy
        enum class NoteStrategy
        {
            EveryMinor3rd,
            Chromatic,
            EveryFifth,
            OctavesOnly
        };
        NoteStrategy noteStrategy = NoteStrategy::EveryMinor3rd;
    };

    struct BundleConfig
    {
        juce::String name; // e.g. "XOceanus - Foundation"
        juce::String manufacturer = "XO_OX Designs";
        juce::String version = "1.0.0";
        juce::String bundleId; // e.g. "com.xo-ox.xoceanus.foundation"
        juce::String description;
        juce::String coverEngine; // Engine ID for cover art style (e.g. "ONSET")
        juce::File outputDir;
        int coverSeed = 0; // RNG seed for reproducible artwork
    };

    //==========================================================================
    // Progress callback
    //==========================================================================

    struct Progress
    {
        int currentPreset = 0;
        int totalPresets = 0;
        int currentNote = 0;
        int totalNotes = 0;
        juce::String presetName;
        juce::String soundShapeLabel; // Sound Shape classification
        float overallProgress = 0.0f; // 0-1
        bool cancelled = false;
    };

    using ProgressCallback = std::function<void(Progress&)>;

    //==========================================================================
    // Size estimation — call before export to inform the user
    //==========================================================================

    struct SizeEstimate
    {
        int64_t totalBytes = 0;
        int totalWavFiles = 0;
        int notesPerPreset = 0;
        int samplesPerNote = 0;
        double durationPerNote = 0.0; // seconds
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
        int64_t wavBodySize = (int64_t)est.samplesPerNote * settings.numChannels * bytesPerSample;
        int64_t wavFileSize = 44 + wavBodySize;

        est.totalBytes = wavFileSize * est.totalWavFiles;
        return est;
    }

    //==========================================================================
    // APVTS injection — optional. When provided, preset parameters are applied
    // to the shared parameter tree before each render so engines produce output
    // that exactly matches the preset settings.
    //
    // When NOT provided (nullptr, the default), buildOfflineContext() creates a
    // temporary per-engine AudioProcessorValueTreeState from each engine's own
    // createParameterLayout(). Engines render with default values plus any
    // preset overrides stored in preset.parametersByEngine. This path is used
    // by test harnesses that have no live processor.
    //
    // Thread safety: preset parameter writes are atomic float stores.
    // Concurrent live audio rendering will see the export's parameter values
    // during the export window — this is acceptable since export is a batch
    // operation that occupies the full rendering context.
    //
    // Call from the message thread before starting the export worker.
    //==========================================================================
    void setAPVTS(juce::AudioProcessorValueTreeState* a) { sharedApvts = a; }

    //==========================================================================
    // Export entry point — call from worker thread
    //==========================================================================

    struct ExportResult
    {
        bool success = false;
        juce::String errorMessage;
        juce::File outputFile;
        int presetsExported = 0;
        int samplesRendered = 0;
        int64_t totalSizeBytes = 0;
    };

    ExportResult exportBundle(const BundleConfig& config, const RenderSettings& settings,
                              const std::vector<PresetData>& presets, ProgressCallback progressCb = nullptr)
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
        // Sanitize bundle name to prevent path traversal (issue #423):
        // config.name comes from user input; strip '/', '..', and filesystem
        // reserved chars before using it to construct directory paths.
        const auto safeBundleName = sanitizeFilename(config.name);
        auto finalBundleDir = config.outputDir.getChildFile(safeBundleName);
        auto tempBundleDir = config.outputDir.getChildFile("." + safeBundleName + "_tmp_" +
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

        std::atomic<bool> cancelled{false};

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

            // Build offline render context — one per preset, reused across all notes.
            // Applies preset parameters to the shared APVTS and creates engine instances.
            auto ctx = buildOfflineContext(preset, effectiveSettings.sampleRate);

            int notesDone = 0;
            bool renderFailed = false;
            juce::String renderError;

            for (int ni = 0; ni < (int)notes.size() && !cancelled.load(); ++ni)
            {
                int note = notes[(size_t)ni];
                for (int vel = 0; vel < effectiveSettings.velocityLayers && !cancelled.load(); ++vel)
                {
                    float velocity = velocityForLayer(vel, effectiveSettings.velocityLayers);
                    auto wavFile = presetDir.getChildFile(wavFilename(preset.name, note, vel));

                    auto wavResult = renderNoteToWav(ctx, note, velocity, effectiveSettings, wavFile);
                    if (!wavResult.success)
                    {
                        renderFailed = true;
                        renderError = wavResult.error;
                        break;
                    }

                    result.samplesRendered++;
                    result.totalSizeBytes += wavFile.getSize();
                    ++notesDone;

                    if (progressCb)
                    {
                        progress.currentNote = notesDone;
                        progress.overallProgress =
                            ((float)pi + (float)notesDone / (float)totalNotesForPreset) / (float)presets.size();
                        progressCb(progress);
                        if (progress.cancelled)
                            cancelled.store(true);
                    }
                }
                if (renderFailed)
                    break;
            }

            if (renderFailed)
            {
                result.errorMessage = "WAV render failed for " + preset.name + ": " + renderError;
                tempBundleDir.deleteRecursively();
                return result;
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
                               : (presets[0].engines.isEmpty() ? juce::String("DEFAULT") : presets[0].engines[0]);

        auto coverResult = XPNCoverArt::generate(coverEngine, config.name, tempBundleDir, result.presetsExported,
                                                 config.version, config.coverSeed);

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
    // Entangled export — renders coupled engine system as single composite
    //==========================================================================

    ExportResult exportCoupledSnapshot(const BundleConfig& config, const RenderSettings& settings,
                                       const CouplingSnapshot& snapshot, const std::vector<PresetData>& presets,
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

        if (!snapshot.hasActiveCoupling())
        {
            // Fall back to standard export if no coupling is active
            return exportBundle(config, settings, presets, progressCb);
        }

        // Atomic export: write to temp dir first, rename on success
        // Sanitize bundle name to prevent path traversal (issue #423).
        const auto safeBundleName = sanitizeFilename(config.name);
        auto finalBundleDir = config.outputDir.getChildFile(safeBundleName);
        auto tempBundleDir = config.outputDir.getChildFile("." + safeBundleName + "_tmp_" +
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

        std::atomic<bool> cancelled{false};

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

            // Create preset directory for WAV files
            juce::String entangledName = "Entangled_" + sanitizeFilename(preset.name);
            auto presetDir = keyGroupDir.getChildFile(entangledName);
            if (!presetDir.createDirectory())
            {
                result.errorMessage = "Failed to create preset directory: " + entangledName;
                tempBundleDir.deleteRecursively();
                return result;
            }

            auto notes = getNotesToRender(effectiveSettings);
            int totalNotesForPreset = (int)notes.size() * effectiveSettings.velocityLayers;
            progress.totalNotes = totalNotesForPreset;

            // Build offline context with coupling — creates all engines AND
            // restores coupling routes so the composite render captures interactions.
            auto ctx = buildCoupledOfflineContext(preset, snapshot, effectiveSettings.sampleRate);

            int notesDone = 0;
            bool renderFailed = false;
            juce::String renderError;

            for (int ni = 0; ni < (int)notes.size() && !cancelled.load(); ++ni)
            {
                int note = notes[(size_t)ni];
                for (int vel = 0; vel < effectiveSettings.velocityLayers && !cancelled.load(); ++vel)
                {
                    float velocity = velocityForLayer(vel, effectiveSettings.velocityLayers);
                    auto wavFile = presetDir.getChildFile(wavFilename(entangledName, note, vel));

                    // Render with coupling active — all engines interact,
                    // producing a single composite output
                    auto wavResult = renderCoupledNoteToWav(ctx, note, velocity, effectiveSettings, wavFile);

                    if (!wavResult.success)
                    {
                        renderFailed = true;
                        renderError = wavResult.error;
                        break;
                    }

                    result.samplesRendered++;
                    result.totalSizeBytes += wavFile.getSize();
                    ++notesDone;

                    if (progressCb)
                    {
                        progress.currentNote = notesDone;
                        progress.overallProgress =
                            ((float)pi + (float)notesDone / (float)totalNotesForPreset) / (float)presets.size();
                        progressCb(progress);
                        if (progress.cancelled)
                            cancelled.store(true);
                    }
                }
                if (renderFailed)
                    break;
            }

            if (renderFailed)
            {
                result.errorMessage = "WAV render failed for " + preset.name + ": " + renderError;
                tempBundleDir.deleteRecursively();
                return result;
            }

            if (!cancelled)
            {
                // Single composite XPM — "Entangled" prefix signals coupling snapshot
                auto xpmFile = keyGroupDir.getChildFile(entangledName + ".xpm");
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

        // Generate cover art
        auto coverEngine = config.coverEngine.isNotEmpty()
                               ? config.coverEngine
                               : (presets[0].engines.isEmpty() ? juce::String("DEFAULT") : presets[0].engines[0]);

        auto coverResult = XPNCoverArt::generate(coverEngine, config.name, tempBundleDir, result.presetsExported,
                                                 config.version, config.coverSeed);

        if (coverResult.success)
            coverResult.cover1000.copyFileTo(tempBundleDir.getChildFile("Preview.png"));

        auto manifestResult = writeManifest(tempBundleDir, config, result.presetsExported);
        if (!manifestResult.success)
        {
            result.errorMessage = "Manifest write failed: " + manifestResult.error;
            tempBundleDir.deleteRecursively();
            return result;
        }

        // Atomic swap
        if (finalBundleDir.exists())
            finalBundleDir.deleteRecursively();

        if (!tempBundleDir.moveFileTo(finalBundleDir))
        {
            if (tempBundleDir.copyDirectoryTo(finalBundleDir))
                tempBundleDir.deleteRecursively();
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

    struct ValidationResult
    {
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
        checkDNA(preset.dna.brightness, "brightness");
        checkDNA(preset.dna.warmth, "warmth");
        checkDNA(preset.dna.movement, "movement");
        checkDNA(preset.dna.density, "density");
        checkDNA(preset.dna.space, "space");
        checkDNA(preset.dna.aggression, "aggression");

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
    static constexpr bool KEY_TRACK = true;
    static constexpr int ROOT_NOTE = 0;
    static constexpr int EMPTY_VEL_START = 0;

    // Shared APVTS — injected via setAPVTS() before export.
    // Nullptr = per-engine minimal APVTSs are created automatically (see
    // buildOfflineContext), using each engine's createParameterLayout().
    // This allows the exporter to run in test environments that have no
    // live processor.
    juce::AudioProcessorValueTreeState* sharedApvts = nullptr;

    //==========================================================================
    // MinimalOfflineProcessor — lightweight AudioProcessor stub used when no
    // sharedApvts is available. Provides the AudioProcessor owner that
    // juce::AudioProcessorValueTreeState requires, without any DSP logic.
    //==========================================================================

    struct MinimalOfflineProcessor : juce::AudioProcessor
    {
        const juce::String getName() const override { return "XOriginateOffline"; }
        void prepareToPlay(double, int) override {}
        void releaseResources() override {}
        void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
        double getTailLengthSeconds() const override { return 0.0; }
        bool acceptsMidi() const override { return false; }
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

    //==========================================================================
    // Internal result types for error propagation
    //==========================================================================

    struct IOResult
    {
        bool success = true;
        juce::String error;
    };

    //==========================================================================
    // OfflineRenderContext — one per preset, reused across all notes.
    // Holds fresh engine instances attached to the shared APVTS.
    //==========================================================================

    struct OfflineRenderContext
    {
        std::vector<std::unique_ptr<SynthEngine>> engines;
        // Coupling matrix for entangled renders — nullptr for standard renders.
        // Owned by the context, prepared with the same block size.
        std::unique_ptr<MegaCouplingMatrix> couplingMatrix;

        // When no sharedApvts is available (e.g. in test environments),
        // buildOfflineContext creates one MinimalOfflineProcessor + one
        // AudioProcessorValueTreeState per engine. These must outlive the
        // engines they are attached to, so they live here alongside them.
        std::vector<std::unique_ptr<MinimalOfflineProcessor>> ownedProcs;
        std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState>> ownedApvts;

        bool valid = false;
    };

    static constexpr int kOfflineBlockSize = 512;

    //==========================================================================
    // Note strategy
    //==========================================================================

    static std::vector<int> getNotesToRender(const RenderSettings& settings)
    {
        std::vector<int> notes;
        switch (settings.noteStrategy)
        {
        case RenderSettings::NoteStrategy::EveryMinor3rd:
            for (int n = 24; n <= 96; n += 3)
                notes.push_back(n);
            break;
        case RenderSettings::NoteStrategy::Chromatic:
            for (int n = 24; n <= 96; ++n)
                notes.push_back(n);
            break;
        case RenderSettings::NoteStrategy::EveryFifth:
            for (int n = 24; n <= 96; n += 7)
                notes.push_back(n);
            break;
        case RenderSettings::NoteStrategy::OctavesOnly:
            for (int n = 24; n <= 96; n += 12)
                notes.push_back(n);
            break;
        }
        return notes;
    }

    static float velocityForLayer(int layer, int totalLayers)
    {
        return renderVelocityForLayer(layer, totalLayers, XPNVelocityCurve::Musical);
    }

    //==========================================================================
    // Offline render context — create once per preset
    //==========================================================================

    OfflineRenderContext buildOfflineContext(const PresetData& preset, double sampleRate)
    {
        OfflineRenderContext ctx;

        if (sharedApvts)
        {
            // ── Shared-APVTS path (live plugin context) ────────────────────────
            // Apply preset parameters to the shared APVTS.
            // Atomic stores are thread-safe; see setAPVTS() comment above.
            for (const auto& [engName, paramsVar] : preset.parametersByEngine)
            {
                if (auto* obj = paramsVar.getDynamicObject())
                {
                    for (const auto& prop : obj->getProperties())
                    {
                        juce::String paramId = prop.name.toString();
                        // Resolve OddfeliX legacy param aliases before writing
                        auto canonical = resolveEngineAlias(engName).equalsIgnoreCase("OddfeliX")
                                             ? resolveSnapParamAlias(paramId)
                                             : paramId;
                        if (canonical.isEmpty())
                            continue; // removed param

                        if (auto* raw = sharedApvts->getRawParameterValue(canonical))
                            raw->store((float)prop.value);
                    }
                }
            }

            // Create fresh engine instances for each engine in the preset
            for (const auto& engName : preset.engines)
            {
                auto canonical = resolveEngineAlias(engName);
                auto engine = EngineRegistry::instance().createEngine(canonical.toStdString());
                if (!engine)
                    continue;

                engine->attachParameters(*sharedApvts);
                engine->prepare(sampleRate, kOfflineBlockSize);
                engine->prepareSilenceGate(sampleRate, kOfflineBlockSize, 500.0f);
                engine->reset();
                ctx.engines.push_back(std::move(engine));
            }
        }
        else
        {
            // ── No-APVTS path (test / standalone context) ──────────────────────
            // Build a per-engine minimal APVTS from the engine's own parameter
            // layout so that attachParameters() succeeds and engines render with
            // their default values. Preset parameter overrides are applied on top.
            for (const auto& engName : preset.engines)
            {
                auto canonical = resolveEngineAlias(engName);
                auto engine = EngineRegistry::instance().createEngine(canonical.toStdString());
                if (!engine)
                    continue;

                // Build a temporary APVTS owned by the context.
                auto proc = std::make_unique<MinimalOfflineProcessor>();
                auto layout = engine->createParameterLayout();
                auto apvts = std::make_unique<juce::AudioProcessorValueTreeState>(
                    *proc, nullptr, "PARAMS", std::move(layout));

                // Apply any preset parameter overrides for this engine.
                auto it = preset.parametersByEngine.find(engName);
                if (it != preset.parametersByEngine.end())
                {
                    if (auto* obj = it->second.getDynamicObject())
                    {
                        for (const auto& prop : obj->getProperties())
                        {
                            juce::String paramId = prop.name.toString();
                            auto paramCanonical = canonical.equalsIgnoreCase("OddfeliX")
                                                      ? resolveSnapParamAlias(paramId)
                                                      : paramId;
                            if (paramCanonical.isEmpty())
                                continue;
                            if (auto* raw = apvts->getRawParameterValue(paramCanonical))
                                raw->store((float)prop.value);
                        }
                    }
                }

                engine->attachParameters(*apvts);
                engine->prepare(sampleRate, kOfflineBlockSize);
                engine->prepareSilenceGate(sampleRate, kOfflineBlockSize, 500.0f);
                engine->reset();

                ctx.engines.push_back(std::move(engine));
                ctx.ownedProcs.push_back(std::move(proc));
                ctx.ownedApvts.push_back(std::move(apvts));
            }
        }

        ctx.valid = !ctx.engines.empty();
        return ctx;
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
            if (chPeak > peak)
                peak = chPeak;
        }

        if (peak < 1e-8f)
            return; // silence, nothing to normalize

        float targetGain = std::pow(10.0f, ceilingDb / 20.0f);
        float gain = targetGain / peak;

        // Only attenuate or boost to ceiling — never amplify silence
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.applyGain(ch, 0, buffer.getNumSamples(), gain);
    }

    //==========================================================================
    // WAV rendering (offline, worker thread)
    //==========================================================================

    // Render a single note using the offline engine context.
    // ctx must have been prepared via buildOfflineContext() for this preset.
    // If ctx.valid is false (no engine could be created from the registry),
    // returns an error — never writes a silent placeholder that would corrupt
    // the export.
    IOResult renderNoteToWav(OfflineRenderContext& ctx, int note, float velocity, const RenderSettings& settings,
                             const juce::File& outputFile)
    {
        xoceanus::dsp::ScopedAudioThreadInit audioInit;
        int totalSamples = (int)((settings.renderSeconds + settings.tailSeconds) * settings.sampleRate);
        int holdSamples = (int)(settings.renderSeconds * settings.sampleRate);

        juce::AudioBuffer<float> buffer(settings.numChannels, totalSamples);
        buffer.clear();

        if (!ctx.valid)
        {
            // No engines were created (engine ID not registered in this build).
            // Return an error instead of writing silent audio that would
            // silently corrupt the export.
            DBG("XPNExporter::renderNoteToWav — ctx.valid is false (no engines registered for preset). Aborting render.");
            jassertfalse;
            return {false, "Render context is invalid: no engines could be created for this preset"};
        }

        // Reset all engine voices for a clean note render
        for (auto& engine : ctx.engines)
        {
            engine->reset();
            engine->wakeSilenceGate();
        }

        // Render in kOfflineBlockSize blocks: note-on → hold → note-off → tail
        int rendered = 0;
        bool noteOffSent = false;

        juce::MidiBuffer noteOnMsg;
        noteOnMsg.addEvent(juce::MidiMessage::noteOn(1, note, (uint8_t)juce::roundToInt(velocity * 127.0f)), 0);

        while (rendered < totalSamples)
        {
            int blockSize = juce::jmin(kOfflineBlockSize, totalSamples - rendered);

            juce::MidiBuffer midi;
            if (rendered == 0)
            {
                midi = noteOnMsg; // note-on at start of first block
            }
            else if (!noteOffSent && rendered >= holdSamples)
            {
                // Note-off at the first block boundary after hold time
                int offsetInBlock = holdSamples - (rendered - blockSize);
                offsetInBlock = juce::jlimit(0, blockSize - 1, offsetInBlock);
                midi.addEvent(juce::MidiMessage::noteOff(1, note), offsetInBlock);
                noteOffSent = true;
            }

            juce::AudioBuffer<float> engineBuf(settings.numChannels, blockSize);

            for (auto& engine : ctx.engines)
            {
                engineBuf.clear();
                engine->renderBlock(engineBuf, midi, blockSize);
                engine->analyzeForSilenceGate(engineBuf, blockSize);

                // Mix engine output into the accumulated buffer
                for (int ch = 0; ch < settings.numChannels; ++ch)
                    buffer.addFrom(ch, rendered, engineBuf, ch, 0, blockSize);
            }

            rendered += blockSize;
        }

        normalizeBuffer(buffer, settings.normCeiling);
        return writeWav(outputFile, buffer, settings.sampleRate, settings.bitDepth);
    }

    //==========================================================================
    // Coupled offline render context — creates engines + coupling matrix
    //==========================================================================

    OfflineRenderContext buildCoupledOfflineContext(const PresetData& preset, const CouplingSnapshot& snapshot,
                                                    double sampleRate)
    {
        // Start with the standard context (creates engines, applies params)
        OfflineRenderContext ctx = buildOfflineContext(preset, sampleRate);
        if (!ctx.valid || !snapshot.hasActiveCoupling())
            return ctx;

        // Create an offline coupling matrix and prepare its scratch buffers
        ctx.couplingMatrix = std::make_unique<MegaCouplingMatrix>();
        ctx.couplingMatrix->prepare(kOfflineBlockSize, sampleRate);

        // Wire engine pointers into the matrix's slot array
        std::array<SynthEngine*, MegaCouplingMatrix::MaxSlots> enginePtrs = {};
        for (size_t i = 0; i < ctx.engines.size() && i < 4; ++i)
            enginePtrs[i] = ctx.engines[i].get();
        ctx.couplingMatrix->setEngines(enginePtrs);

        // Restore the captured coupling routes into the offline matrix
        for (const auto& snapRoute : snapshot.activeRoutes)
        {
            MegaCouplingMatrix::CouplingRoute route;
            route.sourceSlot = snapRoute.sourceSlot;
            route.destSlot = snapRoute.destSlot;
            route.type = static_cast<CouplingType>(snapRoute.couplingType);
            route.amount = snapRoute.amount;
            route.isNormalled = false; // user-captured state
            route.active = true;
            ctx.couplingMatrix->addRoute(route);
        }

        return ctx;
    }

    //==========================================================================
    // Coupled WAV rendering — all engines interact via the coupling matrix,
    // producing a single composite output per note.
    //==========================================================================

    IOResult renderCoupledNoteToWav(OfflineRenderContext& ctx, int note, float velocity, const RenderSettings& settings,
                                    const juce::File& outputFile)
    {
        xoceanus::dsp::ScopedAudioThreadInit audioInit;
        // If no coupling matrix, fall back to standard render
        if (!ctx.couplingMatrix)
            return renderNoteToWav(ctx, note, velocity, settings, outputFile);

        int totalSamples = (int)((settings.renderSeconds + settings.tailSeconds) * settings.sampleRate);
        int holdSamples = (int)(settings.renderSeconds * settings.sampleRate);

        juce::AudioBuffer<float> buffer(settings.numChannels, totalSamples);
        buffer.clear();

        if (!ctx.valid)
            return writeWav(outputFile, buffer, settings.sampleRate, settings.bitDepth);

        // Reset all engine voices for a clean note render
        for (auto& engine : ctx.engines)
        {
            engine->reset();
            engine->wakeSilenceGate();
        }

        int rendered = 0;
        bool noteOffSent = false;

        juce::MidiBuffer noteOnMsg;
        noteOnMsg.addEvent(juce::MidiMessage::noteOn(1, note, (uint8_t)juce::roundToInt(velocity * 127.0f)), 0);

        while (rendered < totalSamples)
        {
            int blockSize = juce::jmin(kOfflineBlockSize, totalSamples - rendered);

            juce::MidiBuffer midi;
            if (rendered == 0)
            {
                midi = noteOnMsg;
            }
            else if (!noteOffSent && rendered >= holdSamples)
            {
                int offsetInBlock = holdSamples - (rendered - blockSize);
                offsetInBlock = juce::jlimit(0, blockSize - 1, offsetInBlock);
                midi.addEvent(juce::MidiMessage::noteOff(1, note), offsetInBlock);
                noteOffSent = true;
            }

            // Phase 1: Render each engine into its own buffer
            // This populates each engine's per-sample coupling output cache
            // (used by getSampleForCoupling in the matrix).
            juce::AudioBuffer<float> engineBuf(settings.numChannels, blockSize);

            for (auto& engine : ctx.engines)
            {
                engineBuf.clear();
                engine->renderBlock(engineBuf, midi, blockSize);
                engine->analyzeForSilenceGate(engineBuf, blockSize);

                // Mix into the composite output buffer
                for (int ch = 0; ch < settings.numChannels; ++ch)
                    buffer.addFrom(ch, rendered, engineBuf, ch, 0, blockSize);
            }

            // Phase 2: Process coupling routes — engines modulate each other.
            // This runs AFTER all engines have rendered for the block, so
            // getSampleForCoupling() returns valid data. The coupling effects
            // will manifest in the NEXT block's render (1-block latency,
            // inaudible at 512 samples / 48kHz = ~10.7ms).
            auto routes = ctx.couplingMatrix->loadRoutes();
            ctx.couplingMatrix->processBlock(blockSize, routes);

            rendered += blockSize;
        }

        normalizeBuffer(buffer, settings.normCeiling);
        return writeWav(outputFile, buffer, settings.sampleRate, settings.bitDepth);
    }

    static IOResult writeWav(const juce::File& file, const juce::AudioBuffer<float>& buffer, double sampleRate,
                             int bitDepth)
    {
        file.deleteFile();
        auto stream = file.createOutputStream();
        if (!stream)
            return {false, "Cannot create output stream: " + file.getFullPathName()};

        juce::WavAudioFormat wav;
        auto writer = std::unique_ptr<juce::AudioFormatWriter>(
            wav.createWriterFor(stream.release(), sampleRate, (unsigned int)buffer.getNumChannels(), bitDepth, {}, 0));
        if (!writer)
            return {false, "Cannot create WAV writer for: " + file.getFullPathName()};

        // Fix #243: Apply TPDF (Triangular Probability Density Function) dither
        // before quantization when writing 16-bit output. TPDF dither adds
        // triangular noise of ±1 LSB peak-to-peak, formed as the sum of two
        // independent uniform random values in [-0.5, +0.5) LSB. This converts
        // quantization distortion (audible harmonic aliasing in quiet tails) into
        // flat-spectrum white noise, which is perceptually far less objectionable.
        // 1 LSB at 16-bit = 1 / 32768 ≈ 3.05e-5 in normalised float.
        // 24-bit output does not need dither (quantization noise is inaudible).
        if (bitDepth == 16)
        {
            const float lsb = 1.0f / 32768.0f;
            juce::AudioBuffer<float> dithered(buffer.getNumChannels(), buffer.getNumSamples());

            // Per-channel TPDF: two independent LCG values, differenced to form
            // the triangle distribution. Separate state per channel keeps
            // L and R uncorrelated (important for stereo imaging at low levels).
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                const float* src = buffer.getReadPointer(ch);
                float* dst = dithered.getWritePointer(ch);
                uint32_t rng = static_cast<uint32_t>(0x12345678u ^ (uint32_t)ch * 0xDEADBEEFu);

                for (int i = 0; i < buffer.getNumSamples(); ++i)
                {
                    // LCG step 1
                    rng = rng * 1664525u + 1013904223u;
                    float r1 = (static_cast<float>(rng >> 8) / 16777216.0f) - 0.5f; // [-0.5, +0.5)

                    // LCG step 2
                    rng = rng * 1664525u + 1013904223u;
                    float r2 = (static_cast<float>(rng >> 8) / 16777216.0f) - 0.5f; // [-0.5, +0.5)

                    // Triangular noise: sum of two uniform ∈ [-0.5, +0.5) → ∈ [-1, +1) LSBs
                    dst[i] = src[i] + (r1 + r2) * lsb;
                }
            }

            if (!writer->writeFromAudioSampleBuffer(dithered, 0, dithered.getNumSamples()))
                return {false, "Failed to write audio data: " + file.getFullPathName()};
        }
        else
        {
            if (!writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()))
                return {false, "Failed to write audio data: " + file.getFullPathName()};
        }

        return {true, {}};
    }

    //==========================================================================
    // XPM generation (keygroup program XML)
    //==========================================================================

    IOResult writeXPM(const juce::File& file, const PresetData& preset, const std::vector<int>& notes,
                      const RenderSettings& settings)
    {
        // Canonical MPCVObject keygroup format — matches XOutshine and XPNDrumExporter
        juce::String xml;
        xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        xml << "<MPCVObject type=\"com.akaipro.mpc.keygroup.program\">\n";
        xml << "  <Version>1.7</Version>\n";
        xml << "  <ProgramName>" << xmlEscape(preset.name.substring(0, 30)) << "</ProgramName>\n";
        xml << "  <AfterTouch>\n";
        xml << "    <Destination>FilterCutoff</Destination>\n";
        xml << "    <Amount>50</Amount>\n";
        xml << "  </AfterTouch>\n";
        xml << "  <ModWheel>\n";
        xml << "    <Destination>FilterCutoff</Destination>\n";
        xml << "    <Amount>70</Amount>\n";
        xml << "  </ModWheel>\n";
        xml << "  <PitchBendRange>12</PitchBendRange>\n";
        xml << "  <Keygroups>\n";

        auto velSplits = getVelocitySplits(XPNVelocityCurve::Musical, settings.velocityLayers);

        for (int i = 0; i < (int)notes.size(); ++i)
        {
            int note = notes[(size_t)i];
            int lowKey = (i == 0) ? 0 : (notes[(size_t)i - 1] + note) / 2;
            int highKey = (i == (int)notes.size() - 1) ? 127 : (note + notes[(size_t)i + 1]) / 2;

            xml << "    <Keygroup index=\"" << i << "\">\n";
            xml << "      <LowNote>" << lowKey << "</LowNote>\n";
            xml << "      <HighNote>" << highKey << "</HighNote>\n";
            xml << "      <Layers>\n";

            int layerIdx = 0;
            for (int v = 0; v < (int)velSplits.size(); ++v)
            {
                xml << "        <Layer index=\"" << layerIdx++ << "\">\n";
                xml << "          <SampleName>" << xmlEscape(wavFilename(preset.name, note, v)) << "</SampleName>\n";
                xml << "          <VelStart>" << velSplits[(size_t)v].start << "</VelStart>\n";
                xml << "          <VelEnd>" << velSplits[(size_t)v].end << "</VelEnd>\n";
                xml << "          <Volume>" << juce::String(velSplits[(size_t)v].volume, 2) << "</Volume>\n";
                xml << "          <RootNote>" << ROOT_NOTE << "</RootNote>\n";
                xml << "          <KeyTrack>" << (KEY_TRACK ? "True" : "False") << "</KeyTrack>\n";
                xml << "          <TuneCoarse>0</TuneCoarse>\n";
                xml << "          <TuneFine>0</TuneFine>\n";
                xml << "          <LoopStart>-1</LoopStart>\n";
                xml << "          <LoopEnd>-1</LoopEnd>\n";
                xml << "        </Layer>\n";
            }

            // Empty layers get VelStart = 0 (critical XPM rule #3)
            for (int v = settings.velocityLayers; v < 4; ++v)
            {
                xml << "        <Layer index=\"" << layerIdx++ << "\">\n";
                xml << "          <VelStart>" << EMPTY_VEL_START << "</VelStart>\n";
                xml << "          <VelEnd>0</VelEnd>\n";
                xml << "        </Layer>\n";
            }

            xml << "      </Layers>\n";
            xml << "    </Keygroup>\n";
        }

        xml << "  </Keygroups>\n";
        xml << "</MPCVObject>\n";

        if (!file.replaceWithText(xml))
            return {false, "Failed to write XPM: " + file.getFullPathName()};

        return {true, {}};
    }

    //==========================================================================
    // Manifest
    //==========================================================================

    static IOResult writeManifest(const juce::File& bundleDir, const BundleConfig& config, int presetCount)
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
            return {false, "Failed to write Manifest.xml"};

        return {true, {}};
    }

    //==========================================================================
    // Filename helpers
    //==========================================================================

    static juce::String sanitizeFilename(const juce::String& name)
    {
        juce::String result = name.replaceCharacters(" /\\:*?\"<>|", "__________").substring(0, 50);
        // After existing character stripping:
        result = result.trimCharactersAtStart(".").trimCharactersAtEnd(".");
        if (result == ".." || result == ".")
            result = "unnamed";
        if (result.isEmpty())
            result = "unnamed";
        return result;
    }

    static juce::String xmlEscape(const juce::String& s)
    {
        return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace("\"", "&quot;");
    }

    static juce::String wavFilename(const juce::String& presetName, int note, int velLayer)
    {
        static const char* noteNames[] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
        int octave = note / 12 - 1;
        juce::String noteName = juce::String(noteNames[note % 12]) + juce::String(octave);

        juce::String name = sanitizeFilename(presetName) + "__" + noteName;
        if (velLayer >= 0)
            name += "__v" + juce::String(velLayer + 1);
        return name + ".WAV";
    }
};

// Backward-compatibility alias — use XOriginate in new code
using XPNExporter = XOriginate;

} // namespace xoceanus
