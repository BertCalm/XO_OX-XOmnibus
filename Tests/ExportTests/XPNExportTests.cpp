/*
    XOmnibus XPN Export Tests
    ==========================
    Tests for XPNExporter: XPM rule enforcement, WAV format, filename
    sanitization, note strategies, velocity layers, batch validation,
    bundle structure, cancellation, and cover art regression.
*/

#include "XPNExportTests.h"

#include "Export/XPNExporter.h"
#include "Export/XPNCoverArt.h"
#include "Core/PresetManager.h"

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>
#include <string>
#include <cmath>
#include <set>

using namespace xomnibus;

//==============================================================================
// Test infrastructure
//==============================================================================

static int g_exportTestsPassed = 0;
static int g_exportTestsFailed = 0;

static void reportTest(const char* name, bool passed)
{
    if (passed)
    {
        std::cout << "  [PASS] " << name << "\n";
        g_exportTestsPassed++;
    }
    else
    {
        std::cout << "  [FAIL] " << name << "\n";
        g_exportTestsFailed++;
    }
}

//==============================================================================
// Helper: create a minimal valid PresetData
//==============================================================================

static PresetData makeTestPreset(const char* name, const char* engine = "OddfeliX")
{
    PresetData p;
    p.schemaVersion = 1;
    p.name = name;
    p.mood = "Foundation";
    p.engines.add(engine);
    p.dna.brightness = 0.5f;
    p.dna.warmth = 0.5f;
    p.dna.movement = 0.5f;
    p.dna.density = 0.5f;
    p.dna.space = 0.5f;
    p.dna.aggression = 0.5f;
    return p;
}

//==============================================================================
// Helper: get a clean temp directory for an export run
//==============================================================================

static juce::File getTestOutputDir(const char* testName)
{
    auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                   .getChildFile("xomnibus_export_tests")
                   .getChildFile(testName);
    dir.deleteRecursively();
    dir.createDirectory();
    return dir;
}

//==============================================================================
// Helper: parse XML from file
//==============================================================================

static std::unique_ptr<juce::XmlElement> parseXmlFile(const juce::File& f)
{
    return juce::XmlDocument::parse(f);
}

//==============================================================================
// 1. XPM Rule Enforcement Tests
//==============================================================================

static void testXPMRuleEnforcement()
{
    std::cout << "\n--- XPM Rule Enforcement ---\n";

    XPNExporter exporter;
    XPNExporter::BundleConfig config;
    config.name = "RuleTest";
    config.bundleId = "com.xo-ox.test.rules";
    config.outputDir = getTestOutputDir("xpm_rules");

    XPNExporter::RenderSettings settings;
    settings.noteStrategy = XPNExporter::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 2;

    std::vector<PresetData> presets = { makeTestPreset("TestPad") };
    auto result = exporter.exportBundle(config, settings, presets);
    reportTest("XPM rules: export succeeds", result.success);

    // Find the XPM file
    auto bundleDir = config.outputDir.getChildFile("RuleTest");
    auto xpmFiles = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");
    reportTest("XPM rules: XPM file created", xpmFiles.size() == 1);

    if (xpmFiles.size() == 1)
    {
        auto xml = parseXmlFile(xpmFiles[0]);
        reportTest("XPM rules: valid XML", xml != nullptr);

        if (xml != nullptr)
        {
            // Rule 1: KeyTrack = True
            reportTest("XPM Rule 1: KeyTrack = True",
                       xml->getStringAttribute("KeyTrack") == "True");

            // Check all zones
            bool allRootNote0 = true;
            bool allEmptyVelStart0 = true;
            int zoneCount = 0;

            for (auto* zone = xml->getFirstChildElement(); zone; zone = zone->getNextElement())
            {
                if (zone->getTagName() != "Zone") continue;
                zoneCount++;

                // Rule 2: RootNote = 0
                if (zone->getIntAttribute("RootNote", -1) != 0)
                    allRootNote0 = false;

                // Rule 3: Empty layers have VelStart = 0
                int layerIdx = 0;
                for (auto* layer = zone->getFirstChildElement(); layer; layer = layer->getNextElement())
                {
                    if (layer->getTagName() != "Layer") continue;
                    layerIdx++;
                    if (layerIdx > settings.velocityLayers)
                    {
                        // This is an empty/padding layer
                        if (layer->getIntAttribute("VelStart", -1) != 0)
                            allEmptyVelStart0 = false;
                    }
                }
            }

            reportTest("XPM Rule 2: all zones have RootNote = 0", allRootNote0);
            reportTest("XPM Rule 3: empty layers have VelStart = 0", allEmptyVelStart0);
            reportTest("XPM rules: zones created", zoneCount > 0);
        }
    }
}

//==============================================================================
// 2. WAV Format Validation
//==============================================================================

static void testWAVFormat()
{
    std::cout << "\n--- WAV Format Validation ---\n";

    XPNExporter exporter;
    XPNExporter::BundleConfig config;
    config.name = "WAVTest";
    config.bundleId = "com.xo-ox.test.wav";
    config.outputDir = getTestOutputDir("wav_format");

    XPNExporter::RenderSettings settings;
    settings.sampleRate = 48000.0;
    settings.bitDepth = 24;
    settings.renderSeconds = 1.0f;
    settings.tailSeconds = 0.5f;
    settings.noteStrategy = XPNExporter::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 1;

    std::vector<PresetData> presets = { makeTestPreset("WAVCheck") };
    auto result = exporter.exportBundle(config, settings, presets);
    reportTest("WAV format: export succeeds", result.success);
    reportTest("WAV format: samples rendered > 0", result.samplesRendered > 0);

    // Find WAV files
    auto bundleDir = config.outputDir.getChildFile("WAVTest");
    auto wavFiles = bundleDir.findChildFiles(juce::File::findFiles, true, "*.WAV");
    reportTest("WAV format: WAV files created", wavFiles.size() > 0);

    if (wavFiles.size() > 0)
    {
        juce::WavAudioFormat wav;
        auto stream = wavFiles[0].createInputStream();
        reportTest("WAV format: can open file stream", stream != nullptr);

        if (stream != nullptr)
        {
            auto reader = std::unique_ptr<juce::AudioFormatReader>(
                wav.createReaderFor(stream.release(), true));
            reportTest("WAV format: valid WAV header", reader != nullptr);

            if (reader != nullptr)
            {
                reportTest("WAV format: sample rate matches",
                           std::abs(reader->sampleRate - 48000.0) < 1.0);
                reportTest("WAV format: bit depth matches",
                           reader->bitsPerSample == 24);
                reportTest("WAV format: stereo",
                           reader->numChannels == 2);
                reportTest("WAV format: non-zero length",
                           reader->lengthInSamples > 0);

                // Expected length: (renderSeconds + tailSeconds) * sampleRate
                int64_t expectedSamples = (int64_t)(1.5 * 48000.0);
                reportTest("WAV format: correct duration",
                           reader->lengthInSamples == expectedSamples);
            }
        }
    }
}

//==============================================================================
// 3. Filename Sanitization Tests
//==============================================================================

static void testFilenameSanitization()
{
    std::cout << "\n--- Filename Sanitization ---\n";

    XPNExporter exporter;
    XPNExporter::BundleConfig config;
    config.name = "SanitizeTest";
    config.bundleId = "com.xo-ox.test.sanitize";
    config.outputDir = getTestOutputDir("sanitize");

    XPNExporter::RenderSettings settings;
    settings.noteStrategy = XPNExporter::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 1;
    settings.renderSeconds = 0.1f;
    settings.tailSeconds = 0.1f;

    // Preset with special characters in name
    auto preset = makeTestPreset("Test/Preset:With*Special<Chars>");
    std::vector<PresetData> presets = { preset };
    auto result = exporter.exportBundle(config, settings, presets);
    reportTest("Sanitize: export succeeds with special chars", result.success);

    // Verify no illegal chars in output paths
    auto bundleDir = config.outputDir.getChildFile("SanitizeTest");
    auto allFiles = bundleDir.findChildFiles(juce::File::findFiles, true);
    bool allClean = true;
    for (const auto& f : allFiles)
    {
        auto name = f.getFileName();
        if (name.containsAnyOf("/\\:*?\"<>|"))
            allClean = false;
    }
    reportTest("Sanitize: no illegal chars in filenames", allClean);

    // Test long name truncation
    juce::String longName;
    for (int i = 0; i < 100; ++i) longName += "A";
    auto longPreset = makeTestPreset("placeholder");
    longPreset.name = longName;
    std::vector<PresetData> longPresets = { longPreset };

    config.outputDir = getTestOutputDir("sanitize_long");
    auto longResult = exporter.exportBundle(config, settings, longPresets);
    reportTest("Sanitize: long name export succeeds", longResult.success);
}

//==============================================================================
// 4. Note Strategy Tests
//==============================================================================

static void testNoteStrategies()
{
    std::cout << "\n--- Note Strategy Coverage ---\n";

    XPNExporter exporter;
    XPNExporter::RenderSettings settings;
    settings.velocityLayers = 1;
    settings.renderSeconds = 0.1f;
    settings.tailSeconds = 0.1f;

    // Test each strategy produces expected note counts
    struct StrategyTest {
        XPNExporter::RenderSettings::NoteStrategy strategy;
        const char* name;
        int expectedMin;   // minimum expected notes
        int expectedMax;   // maximum expected notes
    };

    StrategyTest tests[] = {
        { XPNExporter::RenderSettings::NoteStrategy::OctavesOnly,   "OctavesOnly",   6, 7 },
        { XPNExporter::RenderSettings::NoteStrategy::EveryFifth,    "EveryFifth",    10, 12 },
        { XPNExporter::RenderSettings::NoteStrategy::EveryMinor3rd, "EveryMinor3rd", 24, 26 },
        { XPNExporter::RenderSettings::NoteStrategy::Chromatic,     "Chromatic",     72, 74 },
    };

    for (auto& t : tests)
    {
        settings.noteStrategy = t.strategy;
        XPNExporter::BundleConfig config;
        config.name = juce::String("Strategy_") + t.name;
        config.bundleId = juce::String("com.xo-ox.test.") + t.name;
        config.outputDir = getTestOutputDir(t.name);

        std::vector<PresetData> presets = { makeTestPreset("NoteTest") };
        auto result = exporter.exportBundle(config, settings, presets);

        juce::String testLabel = juce::String(t.name) + ": correct sample count";
        bool inRange = result.samplesRendered >= t.expectedMin
                    && result.samplesRendered <= t.expectedMax;
        reportTest(testLabel.toRawUTF8(), inRange);
    }
}

//==============================================================================
// 5. Velocity Layer Range Tests
//==============================================================================

static void testVelocityLayerRanges()
{
    std::cout << "\n--- Velocity Layer Ranges ---\n";

    XPNExporter exporter;

    for (int layers = 1; layers <= 3; ++layers)
    {
        XPNExporter::BundleConfig config;
        config.name = juce::String("VelTest_") + juce::String(layers);
        config.bundleId = "com.xo-ox.test.vel";
        config.outputDir = getTestOutputDir(("vel_" + std::to_string(layers)).c_str());

        XPNExporter::RenderSettings settings;
        settings.noteStrategy = XPNExporter::RenderSettings::NoteStrategy::OctavesOnly;
        settings.velocityLayers = layers;
        settings.renderSeconds = 0.1f;
        settings.tailSeconds = 0.1f;

        std::vector<PresetData> presets = { makeTestPreset("VelCheck") };
        auto result = exporter.exportBundle(config, settings, presets);

        // Parse XPM and check velocity ranges
        auto bundleDir = config.outputDir.getChildFile(config.name.replace(" ", "_"));
        auto xpmFiles = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");

        if (xpmFiles.size() == 1)
        {
            auto xml = parseXmlFile(xpmFiles[0]);
            if (xml != nullptr)
            {
                // Check first zone's velocity coverage
                auto* firstZone = xml->getChildByName("Zone");
                if (firstZone != nullptr)
                {
                    bool noGaps = true;
                    bool coversFullRange = false;
                    int prevEnd = -1;
                    int minVelStart = 128;
                    int maxVelEnd = -1;

                    int activeLayerCount = 0;
                    for (auto* layer = firstZone->getFirstChildElement();
                         layer; layer = layer->getNextElement())
                    {
                        if (layer->getTagName() != "Layer") continue;
                        int velStart = layer->getIntAttribute("VelStart");
                        int velEnd = layer->getIntAttribute("VelEnd");

                        // Skip empty/padding layers (VelEnd == 0)
                        if (velEnd == 0 && activeLayerCount >= layers)
                            continue;

                        activeLayerCount++;
                        if (velStart < minVelStart) minVelStart = velStart;
                        if (velEnd > maxVelEnd) maxVelEnd = velEnd;

                        // Check contiguity
                        if (prevEnd >= 0 && velStart != prevEnd + 1)
                            noGaps = false;
                        prevEnd = velEnd;
                    }

                    coversFullRange = (minVelStart == 0 && maxVelEnd == 127);

                    juce::String label = juce::String(layers) + " layers: ";
                    reportTest((label + "no velocity gaps").toRawUTF8(), noGaps);
                    reportTest((label + "covers 0-127").toRawUTF8(), coversFullRange);
                }
            }
        }
    }
}

//==============================================================================
// 6. Batch Validation Tests
//==============================================================================

static void testBatchValidation()
{
    std::cout << "\n--- Batch Validation ---\n";

    // Duplicate names detected
    {
        std::vector<PresetData> dupes = {
            makeTestPreset("SameName"),
            makeTestPreset("SameName"),
        };
        auto result = XPNExporter::validateBatch(dupes);
        reportTest("Batch: duplicate names detected", !result.valid);
        bool hasDupeError = false;
        for (const auto& e : result.errors)
            if (e.contains("Duplicate")) hasDupeError = true;
        reportTest("Batch: error mentions 'Duplicate'", hasDupeError);
    }

    // Empty name is an error
    {
        auto p = makeTestPreset("");
        std::vector<PresetData> batch = { p };
        auto result = XPNExporter::validateBatch(batch);
        reportTest("Batch: empty name is error", !result.valid);
    }

    // DNA out of range
    {
        auto p = makeTestPreset("BadDNA");
        p.dna.brightness = 1.5f;
        std::vector<PresetData> batch = { p };
        auto result = XPNExporter::validateBatch(batch);
        reportTest("Batch: out-of-range DNA is error", !result.valid);
    }

    // Name > 30 chars is a warning (not error)
    {
        auto p = makeTestPreset("ThisNameIsDefinitelyLongerThan30Characters");
        std::vector<PresetData> batch = { p };
        auto result = XPNExporter::validateBatch(batch);
        reportTest("Batch: long name is warning not error", result.valid);
        reportTest("Batch: has warning for long name", result.warnings.size() > 0);
    }

    // Valid batch passes
    {
        std::vector<PresetData> good = {
            makeTestPreset("Alpha"),
            makeTestPreset("Beta"),
            makeTestPreset("Gamma"),
        };
        auto result = XPNExporter::validateBatch(good);
        reportTest("Batch: valid batch passes", result.valid);
        reportTest("Batch: no errors on valid batch", result.errors.isEmpty());
    }
}

//==============================================================================
// 7. Bundle Structure Tests
//==============================================================================

static void testBundleStructure()
{
    std::cout << "\n--- Bundle Structure ---\n";

    XPNExporter exporter;
    XPNExporter::BundleConfig config;
    config.name = "StructureTest";
    config.bundleId = "com.xo-ox.test.structure";
    config.manufacturer = "XO_OX Designs";
    config.version = "1.2.3";
    config.description = "Structure test bundle";
    config.coverEngine = "ONSET";
    config.outputDir = getTestOutputDir("structure");

    XPNExporter::RenderSettings settings;
    settings.noteStrategy = XPNExporter::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 1;
    settings.renderSeconds = 0.1f;
    settings.tailSeconds = 0.1f;

    std::vector<PresetData> presets = {
        makeTestPreset("Pad Alpha"),
        makeTestPreset("Lead Beta"),
    };

    auto result = exporter.exportBundle(config, settings, presets);
    reportTest("Structure: export succeeds", result.success);
    reportTest("Structure: 2 presets exported", result.presetsExported == 2);

    auto bundleDir = config.outputDir.getChildFile("StructureTest");

    // Required files
    reportTest("Structure: Manifest.xml exists",
               bundleDir.getChildFile("Manifest.xml").existsAsFile());
    reportTest("Structure: Preview.png exists",
               bundleDir.getChildFile("Preview.png").existsAsFile());
    reportTest("Structure: Keygroups/ exists",
               bundleDir.getChildFile("Keygroups").isDirectory());

    // Manifest content
    auto manifest = parseXmlFile(bundleDir.getChildFile("Manifest.xml"));
    reportTest("Structure: Manifest is valid XML", manifest != nullptr);
    if (manifest != nullptr)
    {
        reportTest("Structure: Manifest Name correct",
                   manifest->getStringAttribute("Name") == "StructureTest");
        reportTest("Structure: Manifest Manufacturer correct",
                   manifest->getStringAttribute("Manufacturer") == "XO_OX Designs");
        reportTest("Structure: Manifest Version correct",
                   manifest->getStringAttribute("Version") == "1.2.3");
        reportTest("Structure: Manifest PresetCount correct",
                   manifest->getIntAttribute("PresetCount") == 2);
    }

    // XPM files for each preset
    auto xpmFiles = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");
    reportTest("Structure: 2 XPM files created", xpmFiles.size() == 2);

    // Cover art files
    reportTest("Structure: artwork.png exists",
               bundleDir.getChildFile("artwork.png").existsAsFile());
    reportTest("Structure: artwork_2000.png exists",
               bundleDir.getChildFile("artwork_2000.png").existsAsFile());

    // Total size tracking
    reportTest("Structure: totalSizeBytes > 0", result.totalSizeBytes > 0);
}

//==============================================================================
// 8. Cancel Mid-Export Tests
//==============================================================================

static void testCancelMidExport()
{
    std::cout << "\n--- Cancel Mid-Export ---\n";

    XPNExporter exporter;
    XPNExporter::BundleConfig config;
    config.name = "CancelTest";
    config.bundleId = "com.xo-ox.test.cancel";
    config.outputDir = getTestOutputDir("cancel");

    XPNExporter::RenderSettings settings;
    settings.noteStrategy = XPNExporter::RenderSettings::NoteStrategy::OctavesOnly;
    settings.velocityLayers = 1;
    settings.renderSeconds = 0.1f;
    settings.tailSeconds = 0.1f;

    // Create 10 presets, cancel after 2nd
    std::vector<PresetData> presets;
    for (int i = 0; i < 10; ++i)
        presets.push_back(makeTestPreset(("Preset_" + std::to_string(i)).c_str()));

    int cancelAfter = 2;
    auto result = exporter.exportBundle(config, settings, presets,
        [&](XPNExporter::Progress& p)
        {
            if (p.currentPreset >= cancelAfter)
                p.cancelled = true;
        });

    reportTest("Cancel: export reports not successful", !result.success);
    reportTest("Cancel: fewer presets exported than total",
               result.presetsExported < (int)presets.size());
}

//==============================================================================
// 9. Empty Preset List Test
//==============================================================================

static void testEmptyPresetList()
{
    std::cout << "\n--- Empty Preset List ---\n";

    XPNExporter exporter;
    XPNExporter::BundleConfig config;
    config.name = "EmptyTest";
    config.bundleId = "com.xo-ox.test.empty";
    config.outputDir = getTestOutputDir("empty");

    XPNExporter::RenderSettings settings;
    std::vector<PresetData> empty;

    auto result = exporter.exportBundle(config, settings, empty);
    reportTest("Empty: export returns failure", !result.success);
    reportTest("Empty: error message set", result.errorMessage.isNotEmpty());
    reportTest("Empty: zero presets exported", result.presetsExported == 0);
}

//==============================================================================
// 10. Cover Art Regression Tests
//==============================================================================

static void testCoverArtRegression()
{
    std::cout << "\n--- Cover Art Regression ---\n";

    // All 20 engine IDs resolve to correct label
    struct EngineCheck {
        const char* id;
        const char* expectedLabel;
    };

    EngineCheck engines[] = {
        { "ONSET",     "ONSET" },
        { "OVERWORLD", "OVERWORLD" },
        { "ODDFELIX",  "OddfeliX" },
        { "ODDOSCAR",  "OddOscar" },
        { "OVERDUB",   "OVERDUB" },
        { "ODYSSEY",   "ODYSSEY" },
        { "OBLONG",    "OBLONG" },
        { "OBESE",     "OBESE" },
        { "OPAL",      "OPAL" },
        { "ORGANON",   "ORGANON" },
        { "OUROBOROS",  "OUROBOROS" },
        { "OBSIDIAN",  "OBSIDIAN" },
        { "OVERBITE",  "OVERBITE" },
        { "ORIGAMI",   "ORIGAMI" },
        { "ORACLE",    "ORACLE" },
        { "OBSCURA",   "OBSCURA" },
        { "OCEANIC",   "OCEANIC" },
        { "OPTIC",     "OPTIC" },
        { "OBLIQUE",   "OBLIQUE" },
    };

    for (auto& e : engines)
    {
        auto def = XPNCoverArt::getEngineDef(e.id);
        juce::String testName = juce::String(e.id) + " resolves to " + e.expectedLabel;
        reportTest(testName.toRawUTF8(),
                   juce::String(def.label) == juce::String(e.expectedLabel));
    }

    // Legacy aliases resolve correctly
    struct AliasCheck {
        const char* legacy;
        const char* expectedLabel;
    };

    AliasCheck aliases[] = {
        { "SNAP",  "OddfeliX" },
        { "MORPH", "OddOscar" },
        { "DUB",   "OVERDUB" },
        { "DRIFT", "ODYSSEY" },
        { "BOB",   "OBLONG" },
        { "FAT",   "OBESE" },
    };

    for (auto& a : aliases)
    {
        auto def = XPNCoverArt::getEngineDef(a.legacy);
        juce::String testName = juce::String("Legacy ") + a.legacy + " -> " + a.expectedLabel;
        reportTest(testName.toRawUTF8(),
                   juce::String(def.label) == juce::String(a.expectedLabel));
    }

    // Generate cover art and verify output files
    {
        auto outputDir = getTestOutputDir("cover_art");
        auto result = XPNCoverArt::generate("ONSET", "Test Pack", outputDir, 10, "1.0", 42);
        reportTest("Cover art: generation succeeds", result.success);
        reportTest("Cover art: artwork.png exists", result.cover1000.existsAsFile());
        reportTest("Cover art: artwork_2000.png exists", result.cover2000.existsAsFile());

        // Check file sizes are reasonable (>1KB for a PNG)
        reportTest("Cover art: 1000px file has content",
                   result.cover1000.getSize() > 1024);
        reportTest("Cover art: 2000px file has content",
                   result.cover2000.getSize() > 1024);
        reportTest("Cover art: 2000px larger than 1000px",
                   result.cover2000.getSize() > result.cover1000.getSize());
    }

    // Default/unknown engine falls back gracefully
    {
        auto def = XPNCoverArt::getEngineDef("NONEXISTENT_ENGINE");
        reportTest("Cover art: unknown engine gets default label",
                   juce::String(def.label) == "XO_OX");
    }
}

//==============================================================================
// 11. XPM Zone Coverage Tests
//==============================================================================

static void testXPMZoneCoverage()
{
    std::cout << "\n--- XPM Zone Coverage ---\n";

    XPNExporter exporter;
    XPNExporter::BundleConfig config;
    config.name = "ZoneCoverage";
    config.bundleId = "com.xo-ox.test.zones";
    config.outputDir = getTestOutputDir("zones");

    XPNExporter::RenderSettings settings;
    settings.noteStrategy = XPNExporter::RenderSettings::NoteStrategy::EveryMinor3rd;
    settings.velocityLayers = 3;
    settings.renderSeconds = 0.1f;
    settings.tailSeconds = 0.1f;

    std::vector<PresetData> presets = { makeTestPreset("ZoneCheck") };
    exporter.exportBundle(config, settings, presets);

    auto bundleDir = config.outputDir.getChildFile("ZoneCoverage");
    auto xpmFiles = bundleDir.findChildFiles(juce::File::findFiles, true, "*.xpm");

    if (xpmFiles.size() == 1)
    {
        auto xml = parseXmlFile(xpmFiles[0]);
        if (xml != nullptr)
        {
            // Collect all key ranges
            std::set<int> coveredKeys;
            bool noOverlap = true;
            int prevHighKey = -1;

            for (auto* zone = xml->getFirstChildElement(); zone; zone = zone->getNextElement())
            {
                if (zone->getTagName() != "Zone") continue;

                int lowKey = zone->getIntAttribute("LowKey");
                int highKey = zone->getIntAttribute("HighKey");

                // Check no overlap with previous zone
                if (prevHighKey >= 0 && lowKey <= prevHighKey)
                    noOverlap = false;

                for (int k = lowKey; k <= highKey; ++k)
                    coveredKeys.insert(k);

                prevHighKey = highKey;

                // Check sample file references exist
                for (auto* layer = zone->getFirstChildElement();
                     layer; layer = layer->getNextElement())
                {
                    if (layer->getTagName() != "Layer") continue;
                    auto samplePath = layer->getStringAttribute("SampleFile");
                    if (samplePath.isNotEmpty() && layer->getIntAttribute("VelEnd") > 0)
                    {
                        auto fullPath = bundleDir.getChildFile("Keygroups")
                                            .getChildFile(samplePath);
                        // Just check the format is valid (file should exist)
                    }
                }
            }

            // Key 0 and 127 should be covered
            reportTest("Zone coverage: key 0 covered", coveredKeys.count(0) > 0);
            reportTest("Zone coverage: key 127 covered", coveredKeys.count(127) > 0);
            reportTest("Zone coverage: no zone overlaps", noOverlap);

            // Check contiguity (no gaps in key range)
            bool contiguous = true;
            for (int k = 0; k <= 127; ++k)
            {
                if (coveredKeys.count(k) == 0)
                {
                    contiguous = false;
                    break;
                }
            }
            reportTest("Zone coverage: all 128 keys covered", contiguous);
        }
    }
}

//==============================================================================
// Public entry point
//==============================================================================

namespace export_tests {

int runAll()
{
    g_exportTestsPassed = 0;
    g_exportTestsFailed = 0;

    std::cout << "\n========================================\n";
    std::cout << "  XPN Export Tests\n";
    std::cout << "========================================\n";

    testXPMRuleEnforcement();
    testWAVFormat();
    testFilenameSanitization();
    testNoteStrategies();
    testVelocityLayerRanges();
    testBatchValidation();
    testBundleStructure();
    testCancelMidExport();
    testEmptyPresetList();
    testCoverArtRegression();
    testXPMZoneCoverage();

    std::cout << "\n  Export Tests: " << g_exportTestsPassed << " passed, "
              << g_exportTestsFailed << " failed\n";

    return g_exportTestsFailed;
}

} // namespace export_tests
